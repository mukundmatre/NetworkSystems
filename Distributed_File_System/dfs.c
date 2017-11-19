/*Distributed File System Server
Author: Mukund Madhusudan Atre*/

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>

#define SERVER_PORT 5575
#define MAX_PENDING 5
#define BUFFSIZE 256

int authenticate_user(char* username, char* password);
int put_fparts(char* user, char* server_root, int comm_socket);
int get_fparts(char* username, char* server_root, int conn_sock);
int list_files(char* user_name, char* server_root, int comm_socket);

int authenticate_user(char* username, char* password)
{
  if ((strcmp(username, "") == 0) || (strcmp(password, "") == 0)) {
    return 1;
  }
  FILE *conf_ptr = NULL;
  char *line = NULL;
  bool auth = false;
  size_t length;
  conf_ptr = fopen("dfs.conf", "r");
  if (conf_ptr!=NULL)
  {
    while ((getline(&line, &length, conf_ptr))!=-1)
    {
      if ((strstr(line, username)!=NULL) && (strstr(line, password)!=NULL))
      {
        auth = true;
        return 0;
      }
    }
    if (auth == false)
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }
  else
  {
    return -1;
  }
}


int put_fparts (char* user, char* server_root, int comm_socket) {
  FILE *file_ptr;
  FILE *file_ptr1;
  char file_name[20];
  unsigned long int expect_len;
  unsigned int num_iterations;
  unsigned long int final_iteration;
  char in_buffer[BUFFSIZE];
  int recv_bytes;
  char file_path[50];
  char dir_path[20];
  sprintf(dir_path, ".%s/%s",server_root, user);
  mkdir(dir_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  recv_bytes = recv(comm_socket, in_buffer, sizeof(in_buffer), 0);
  if (recv_bytes > 0) {
    sscanf(in_buffer, "%*s %s %lu", file_name, &expect_len);
    printf("Buffer:%s\n", in_buffer);
    printf("Expect length:%lu\n", expect_len);
  }
  bzero(in_buffer, sizeof(in_buffer));
  send(comm_socket, "ACK", 3, 0);
  num_iterations = expect_len/BUFFSIZE;
  printf("Num Iterations:%d\n", num_iterations);
  final_iteration = expect_len%BUFFSIZE;
  printf("final_iteration:%lu\n", final_iteration);
  sprintf(file_path, "%s/%s", dir_path, file_name);
  file_ptr = fopen(file_path, "a");
  for (size_t i = 0; i < num_iterations; i++) {
    recv_bytes = recv(comm_socket, in_buffer, BUFFSIZE, 0);
    fwrite(in_buffer, 1, recv_bytes, file_ptr);
    bzero(in_buffer, sizeof(in_buffer));
    send(comm_socket, "ACK", 3, 0);
  }
  recv_bytes = recv(comm_socket, in_buffer, final_iteration, 0);
  fwrite(in_buffer, 1, recv_bytes, file_ptr);
  send(comm_socket, "ACK", 3, 0);
  fclose(file_ptr);


  bzero(in_buffer, sizeof(in_buffer));
  bzero(file_path, sizeof(file_path));
  bzero(file_name, sizeof(file_name));
  recv_bytes = recv(comm_socket, in_buffer, sizeof(in_buffer), 0);
  if (recv_bytes > 0) {
    sscanf(in_buffer, "%*s %s %lu", file_name, &expect_len);
    printf("Buffer:%s\n", in_buffer);
    printf("Expect length:%lu\n", expect_len);
  }
  bzero(in_buffer, sizeof(in_buffer));
  send(comm_socket, "ACK", 3, 0);
  num_iterations = expect_len/BUFFSIZE;
  printf("Num Iterations:%d\n", num_iterations);
  final_iteration = expect_len%BUFFSIZE;
  printf("final_iteration:%lu\n", final_iteration);
  sprintf(file_path, "%s/%s", dir_path, file_name);
  file_ptr1 = fopen(file_path, "a");
  for (size_t i = 0; i < num_iterations; i++) {
    recv_bytes = recv(comm_socket, in_buffer, BUFFSIZE, 0);
    fwrite(in_buffer, 1, recv_bytes, file_ptr1);
    bzero(in_buffer, sizeof(in_buffer));
    send(comm_socket, "ACK", 3, 0);
  }
  recv_bytes = recv(comm_socket, in_buffer, final_iteration, 0);
  fwrite(in_buffer, 1, recv_bytes, file_ptr1);
  send(comm_socket, "ACK", 3, 0);
  fclose(file_ptr1);

  bzero(in_buffer, sizeof(in_buffer));
  recv_bytes = recv(comm_socket, in_buffer, sizeof(in_buffer), 0);
  if (strcmp(in_buffer, "PUT Complete")==0) {
    printf("PUT Successful:%d\n", recv_bytes);
  }
  else
  {
    printf("PUT failed:%d\n", recv_bytes);
  }
  return 0;
}


int list_files(char* user_name, char* server_root, int comm_socket) {
  DIR *dir;
  struct dirent *ent;
  char buffer[10];
  char file_list[100];
  bzero(file_list, sizeof(file_list));
  char dir_path[20];
  sprintf(dir_path, ".%s/%s",server_root, user_name);
  if ((dir = opendir (dir_path)) != NULL) {
  /* print all the files and directories within directory */
  while ((ent = readdir (dir)) != NULL) {
    if ((strcmp(ent->d_name, ".") == 0) || (strcmp(ent->d_name, "..") == 0) ) {
      continue;
    }
    strcat(file_list, ent->d_name);
    strcat(file_list, "\n");
  }
  closedir (dir);
  bzero(buffer, sizeof(buffer));
  recv(comm_socket, buffer, sizeof(buffer), 0);
  if (strstr(buffer, "Done")) {
    send(comm_socket, "ACK", 3, 0);
    return 2;
  }
  send(comm_socket, file_list, strlen(file_list), 0);
  bzero(buffer, sizeof(buffer));
  } else {
  /* could not open directory */
  perror ("");
  return 1;
  }
  return 0;
}


int get_fparts(char* username, char* server_root, int conn_sock) {
  FILE *file_ptr;
  char buffer[BUFFSIZE];
  char request[5];
  char file_name[30];
  char file_path[100];
  size_t read_bytes;
  int iter = 0;
  int list_ret;

  list_ret = list_files(username, server_root, conn_sock);
  if (list_ret == 2) {
    return 2;
  }

  while(iter<3) {
    bzero(file_name, sizeof(file_name));
    bzero(file_path, sizeof(file_path));
    bzero(request, sizeof(request));
    recv(conn_sock, buffer, sizeof(buffer), 0);
    printf("GET file name:%s\n", buffer);
    if (strstr(buffer, "Done")) {
      send(conn_sock, "ACK", 3, 0);
      break;
    }
    sscanf(buffer, "%s %s", request, file_name);
    printf("Filename:%s\n", file_name);
    sprintf(file_path, ".%s/%s/%s", server_root, username, file_name);
    file_ptr = fopen(file_path, "r");
    if (file_ptr == NULL) {
      printf("Cannot open file\n");
      exit(1);
    }
    send(conn_sock, "ACK", 3, 0);
    bzero(buffer, sizeof(buffer));
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file_ptr)) > 0) {
      send(conn_sock, buffer, read_bytes, 0);
      bzero(buffer, sizeof(buffer));
      recv(conn_sock, buffer, sizeof(buffer), 0);
      bzero(buffer, sizeof(buffer));
    }
    iter++;
  }
  return 0;
}



int main(int argc, char *argv[])
{
  struct sockaddr_in dfs_add;
  char buffer[BUFFSIZE];
  unsigned int len;
  int recv_bytes;
  int auth;
  int dfs_sock, conn_sock;
  char username[40];
  char password[40];
  char request_type[10];
  char filename[40];
  char port[10] = "5575";
  char server_root[10] = "/DFS1";

  if(argc!=3)
  {
    printf("usage: %s [Server Root] [Port]\n",argv[0]);
    exit(1);
  }
  else
  {
    strcpy(server_root, argv[1]);
    printf("Server root:%s\n", server_root);
    strcpy(port, argv[2]);
  }

/* build address data structure */
  bzero((char *)&dfs_add, sizeof(dfs_add));
  dfs_add.sin_family = AF_INET;
  dfs_add.sin_addr.s_addr = INADDR_ANY;
  dfs_add.sin_port = htons(atoi(port));

/* setup passive open */
  if ((dfs_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("\ndfs_sock creation error");
    exit(1);
  }

  if ((bind(dfs_sock, (struct sockaddr *)&dfs_add, sizeof(dfs_add))) < 0) {
    perror("\ndfs_sock bind error");
    exit(1);
  }

  listen(dfs_sock, MAX_PENDING);

/* wait for connection, then receive and print text */
  while(1) {
    if ((conn_sock = accept(dfs_sock, (struct sockaddr *)&dfs_add, &len)) < 0) {
      perror("\naccept error on dfs");
      exit(1);
    }


    while (1) {
      bzero(request_type, sizeof(request_type));
      bzero(username, sizeof(username));
      bzero(password, sizeof(password));
      bzero(buffer, sizeof(buffer));
      recv_bytes = recv(conn_sock, buffer, sizeof(buffer), 0);
      if (recv_bytes == 0) {
        printf("Client Disconnected\n");
        break;
      }
      printf("\n-----%s-----\n", buffer);
      sscanf(buffer, "%s %s %s %s", request_type, filename, username, password);
      bzero(buffer, sizeof(buffer));
      printf("User:%s Pass:%s\n", username, password);
      auth = authenticate_user(username, password);
      if (auth == 0)
      {
        printf("User Authenticated\n");
        bzero(buffer, sizeof(buffer));
        sprintf(buffer, "Authentication Success");
        send(conn_sock, buffer, strlen(buffer), 0);
        if (strcmp(request_type, "PUT") == 0) {
          printf("PUT detected\n");
          put_fparts(username, server_root, conn_sock);
        }
        else if (strcmp(request_type, "GET") == 0) {
          printf("GET detected\n");
          get_fparts(username, server_root, conn_sock);
        }
        else if (strcmp(request_type, "LIST") == 0) {
          printf("LIST detected\n");
          list_files(username, server_root, conn_sock);
        }
        else {
          printf("Invalid Command\n");
        }
      }
      else if (auth == 1)
      {
        printf("User not found\n");
        sprintf(buffer, "Authentication Failure");
        send(conn_sock, buffer, sizeof(buffer), 0);
        close(conn_sock);
        exit(1);
      }
      else
      {
        printf("Config file parsing failed\n");
        sprintf(buffer, "Configuration Failure");
        send(conn_sock, buffer, sizeof(buffer), 0);
        close(conn_sock);
        exit(1);
      }
    }


    close(conn_sock);
  }
}
