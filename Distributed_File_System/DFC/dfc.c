/*
Distributed File System Client
Author: Mukund Madhusudan Atre
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <openssl/md5.h>

#define BUFFSIZE 256
#define NUM_MODULUS 4
#define NUM_SERVERS 4
#define NUM_FPART 2

typedef struct
{
  char dfs1_ip[40];
  char dfs2_ip[40];
  char dfs3_ip[40];
  char dfs4_ip[40];
  char username[40];
  char password[40];
  int dfs1_port;
  int dfs2_port;
  int dfs3_port;
  int dfs4_port;
}dfc_t;

dfc_t conf;
int dfc1_sock, dfc2_sock, dfc3_sock, dfc4_sock;
struct sockaddr_in dfs1_add, dfs2_add, dfs3_add, dfs4_add;
bool dfs1_status, dfs2_status, dfs3_status, dfs4_status;

int filesys_map [NUM_MODULUS] [NUM_SERVERS] [NUM_FPART] = {
                                                           {{1,2},{2,3},{3,4},{4,1}}
                                                          ,{{4,1},{1,2},{2,3},{3,4}}
                                                          ,{{3,4},{4,1},{1,2},{2,3}}
                                                          ,{{2,3},{3,4},{4,1},{1,2}}
                                                          };
int parse_dfc_conf(char* conf_name);
int req_auth(char* request, char* file);
int calculate_md5sum(FILE *file_ptr, char *md5_string);
int put_file(char *file);
int dfs_list();
int get_file(char* file_name);

int parse_dfc_conf(char* conf_name)
{
  FILE *conf_ptr;
  char *server_add;
  char *line = NULL;
  size_t length;
  conf_ptr = fopen(conf_name, "r");

  if (conf_ptr!=NULL)
  {
    while ((getline(&line, &length, conf_ptr))!=-1)
    {
      if ((server_add = strstr(line, "DFS1")))
      {
        sscanf(server_add, "%*s %[^:]%*c%d", conf.dfs1_ip, &conf.dfs1_port);
      }
      if ((server_add = strstr(line, "DFS2")))
      {
        sscanf(server_add, "%*s %[^:]%*c%d", conf.dfs2_ip, &conf.dfs2_port);
      }
      if ((server_add = strstr(line, "DFS3")))
      {
        sscanf(server_add, "%*s %[^:]%*c%d", conf.dfs3_ip, &conf.dfs3_port);
      }
      if ((server_add = strstr(line, "DFS4")))
      {
        sscanf(server_add, "%*s %[^:]%*c%d", conf.dfs4_ip, &conf.dfs4_port);
      }
      if ((server_add = strstr(line, "Username")))
      {
        sscanf(server_add, "%*[^:]%*c %s", conf.username);
      }
      if ((server_add = strstr(line, "Password")))
      {
        sscanf(server_add, "%*[^:]%*c %s", conf.password);
      }
    }
    return 0;
  }
  else
  {
    return 1;
  }
}

int establish_connection () {
  /*Establish connection to servers*/
    printf("Connecting to servers...\n");

    if (connect(dfc1_sock, (struct sockaddr *)&dfs1_add, sizeof(dfs1_add)) < 0 && errno!=EISCONN)
    {
      perror("\nunable to connect to dfs1");
      dfs1_status = false;
    }
    else
    {
      dfs1_status = true;
    }

    if (connect(dfc2_sock, (struct sockaddr *)&dfs2_add, sizeof(dfs2_add)) < 0 && errno != EISCONN)
    {
      perror("\nunable to connect to dfs2");
      dfs2_status = false;
    }
    else
    {
      dfs2_status = true;

    }

    if (connect(dfc3_sock, (struct sockaddr *)&dfs3_add, sizeof(dfs3_add)) < 0 && errno != EISCONN)
    {
      perror("\nunable to connect to dfs3");
      dfs3_status = false;
    }
    else
    {
      dfs3_status = true;
    }

    if (connect(dfc4_sock, (struct sockaddr *)&dfs4_add, sizeof(dfs4_add)) < 0 && errno != EISCONN)
    {
      perror("\nunable to connect to dfs4");
      dfs4_status = false;
    }
    else
    {
      dfs4_status = true;
    }

  /*Exiting if no server connected*/
    if(!(dfs1_status || dfs2_status || dfs3_status || dfs4_status))
    {
      printf("All servers offline, try again later\n");
      return 1;
    }
    return 0;

}



int req_auth(char* request,char* file)
{
  int conn_status;
  if ((conn_status = establish_connection()) == 1) {
    return 1;
  }

  char in_buffer[BUFFSIZE];
  char out_buffer[BUFFSIZE];
  int recv_bytes;
  bool dfs1_auth = false, dfs2_auth = false, dfs3_auth = false, dfs4_auth = false;

  bzero(out_buffer, sizeof(out_buffer));
  sprintf(out_buffer, "%s %s %s %s", request, file, conf.username, conf.password);
  printf("%s\n", out_buffer);

  if (dfs1_status == true) {
    send(dfc1_sock, out_buffer, strlen(out_buffer), 0);
    bzero(in_buffer, sizeof(in_buffer));
    if ((recv_bytes = recv(dfc1_sock, in_buffer, sizeof(in_buffer), 0))<0) {
      close(dfc1_sock);
    }
    else {
      if (strstr(in_buffer, "Authentication Success"))
      {
        dfs1_auth = true;
      }
      else if(strstr(in_buffer, "Authentication Failure"))
      {
        dfs1_auth = false;
      }
      else
      {
        printf("dfc1_sock.....\n");
        printf("Server error: Configuration failure at dfs1\n");
        close(dfc1_sock);
        exit(1);
      }
    }
  }



  if (dfs2_status == true) {
    send(dfc2_sock, out_buffer, strlen(out_buffer), 0);
    bzero(in_buffer, sizeof(in_buffer));
    if ((recv_bytes = recv(dfc2_sock, in_buffer, sizeof(in_buffer), 0))<0) {
      close(dfc2_sock);
    }
    else {
      if (strstr(in_buffer, "Authentication Success"))
      {
        dfs2_auth = true;
      }
      else if(strstr(in_buffer, "Authentication Failure"))
      {
        dfs2_auth = false;
      }
      else
      {
        printf("dfc2_sock.....\n");
        printf("Server error: Configuration failure at dfs2\n");
        close(dfc2_sock);
        exit(1);
      }
    }
  }


  if (dfs3_status == true) {
    send(dfc3_sock, out_buffer, strlen(out_buffer), 0);
    bzero(in_buffer, sizeof(in_buffer));
    if ((recv_bytes = recv(dfc3_sock, in_buffer, sizeof(in_buffer), 0))<0) {
      close(dfc3_sock);
    }
    else {
      if (strstr(in_buffer, "Authentication Success"))
      {
        dfs3_auth = true;
      }
      else if(strstr(in_buffer, "Authentication Failure"))
      {
        dfs3_auth = false;
      }
      else
      {
        printf("dfc3_sock.....\n");
        printf("Server error: Configuration failure at dfs3\n");
        close(dfc3_sock);
        exit(1);
      }
    }
  }


  if (dfs4_status == true) {
    send(dfc4_sock, out_buffer, strlen(out_buffer), 0);
    bzero(in_buffer, sizeof(in_buffer));
    if ((recv_bytes = recv(dfc4_sock, in_buffer, sizeof(in_buffer), 0))<0) {
      close(dfc4_sock);
    }
    else {
      if (strstr(in_buffer, "Authentication Success"))
      {
        dfs4_auth = true;
      }
      else if(strstr(in_buffer, "Authentication Failure"))
      {
        dfs4_auth = false;
      }
      else
      {
        printf("dfc4_sock.....\n");
        printf("Server error: Configuration failure at dfs4\n");
        close(dfc4_sock);
        exit(1);
      }
    }
  }


  if (dfs1_auth || dfs2_auth || dfs3_auth || dfs4_auth)
  {
    printf("Authentication Successful\n");
    fflush(stdout);
    return 0;
  }
  else
  {
    printf("Invalid Username/Password. Please try again.");
    fflush(stdout);
    exit(1);
  }

}


int calculate_md5sum(FILE *file_ptr, char md5[])
{
  int n;
  MD5_CTX c;
  char buf[512];
  int md5_int;
  ssize_t bytes;
  unsigned char out[MD5_DIGEST_LENGTH];

  MD5_Init(&c);
  do
  {
    bytes=fread(buf, 1, 512, file_ptr);
    MD5_Update(&c, buf, bytes);
  }while(bytes > 0);

  MD5_Final(out, &c);

  for(n=0; n<MD5_DIGEST_LENGTH; n++)
  {
    snprintf( &md5[n*2], 16*2, "%02x", (unsigned int)out[n]);
  }
  md5_int = (int)strtol(md5+28, NULL, 16);
  //fclose(file_ptr);
  return md5_int;
}



int put_file(char *filename)
{
  int md5_tail = 0;
  char md5_string[50];
  int mod;
  int out_sock;
  char buffer[BUFFSIZE];
  unsigned long int read_bytes;
  unsigned long int file_size;
  unsigned long int part_size;
  unsigned long int last_part;
  unsigned long int final_iteration;
  unsigned int num_iterations;

  //open file
  FILE *file_ptr;
  file_ptr = fopen(filename, "r");
  if (file_ptr==NULL)
  {
    perror("Error opening file");
    fflush(stdout);
    return -1;
  }

  fseek(file_ptr, 0, SEEK_END);
  file_size = (unsigned long int) ftell(file_ptr);
  fseek(file_ptr, 0, SEEK_SET);
  printf("Index:%ld\n", ftell(file_ptr));
  part_size = (file_size/4);
  printf("Part Size:%lu\n", part_size);
  last_part = (part_size + (file_size%4));
  num_iterations = part_size/BUFFSIZE;
  printf("Num Iterations:%d\n", num_iterations);
  final_iteration = part_size%BUFFSIZE;
  printf("final_iteration:%lu\n", final_iteration);

  md5_tail = calculate_md5sum(file_ptr, md5_string);
  printf("%s\n", md5_string);
  mod = md5_tail%4;
  printf("Mod: %d\n", mod);

  /*Loop for putting data to all the servers*/
  for (int i = 0; i < 4; i++) {

    if (i==0) {
      if (dfs1_status == false) {
        continue;
      }
      else {
        printf("Putting files on dfs1\n");
        out_sock = dfc1_sock;
      }

    }
    if (i==1) {
      if (dfs2_status == false) {
        continue;
      }
      else {
        printf("Putting files on dfs2\n");
        out_sock = dfc2_sock;
      }
    }
    if (i==2) {
      if (dfs3_status == false) {
        continue;
      }
      else {
        printf("Putting files on dfs3\n");
        out_sock = dfc3_sock;
      }
    }
    if (i==3) {
      if (dfs4_status == false) {
        continue;
      }
      else {
        printf("Putting files on dfs4\n");
        out_sock = dfc4_sock;
      }
    }

    if ((filesys_map[mod][i][0]==1)||(filesys_map[mod][i][1]==1)) {
      printf("---------------Part1--------------------\n");
      fseek(file_ptr, 0, SEEK_SET);
      bzero(buffer, sizeof(buffer));
      sprintf(buffer, "PUT .%s.1 %lu", filename, part_size);
      send(out_sock, buffer, strlen(buffer), 0);
      bzero(buffer, sizeof(buffer));
      recv(out_sock, buffer, sizeof(buffer), 0);
      bzero(buffer, sizeof(buffer));
      for (size_t j = 0; j < num_iterations; j++) {
        read_bytes = fread(buffer, 1, BUFFSIZE, file_ptr);
        send(out_sock, buffer, read_bytes, 0);
        bzero(buffer, sizeof(buffer));
        recv(out_sock, buffer, sizeof(buffer), 0);
        bzero(buffer, sizeof(buffer));
      }
      read_bytes = fread(buffer, 1, final_iteration, file_ptr);
      send(out_sock, buffer, read_bytes, 0);
      bzero(buffer, sizeof(buffer));
      recv(out_sock, buffer, sizeof(buffer), 0);
      bzero(buffer, sizeof(buffer));
    }

    if ((filesys_map[mod][i][0]==2)||(filesys_map[mod][i][1]==2)) {
      printf("---------------Part2--------------------\n");
      fseek(file_ptr, part_size, SEEK_SET);
      bzero(buffer, sizeof(buffer));
      sprintf(buffer, "PUT .%s.2 %lu", filename, part_size);
      send(out_sock, buffer, strlen(buffer), 0);
      bzero(buffer, sizeof(buffer));
      recv(out_sock, buffer, sizeof(buffer), 0);
      bzero(buffer, sizeof(buffer));
      for (size_t j = 0; j < num_iterations; j++) {
        read_bytes = fread(buffer, 1, BUFFSIZE, file_ptr);
        send(out_sock, buffer, read_bytes, 0);
        bzero(buffer, sizeof(buffer));
        recv(out_sock, buffer, sizeof(buffer), 0);
        bzero(buffer, sizeof(buffer));
      }
      read_bytes = fread(buffer, 1, final_iteration, file_ptr);
      send(out_sock, buffer, read_bytes, 0);
      bzero(buffer, sizeof(buffer));
      recv(out_sock, buffer, sizeof(buffer), 0);
      bzero(buffer, sizeof(buffer));
    }

    if ((filesys_map[mod][i][0]==3)||(filesys_map[mod][i][1]==3)) {
      printf("---------------Part3--------------------\n");
      fseek(file_ptr, (2*part_size), SEEK_SET);
      bzero(buffer, sizeof(buffer));
      sprintf(buffer, "PUT .%s.3 %lu", filename, part_size);
      send(out_sock, buffer, strlen(buffer), 0);
      bzero(buffer, sizeof(buffer));
      recv(out_sock, buffer, sizeof(buffer), 0);
      bzero(buffer, sizeof(buffer));
      for (size_t j = 0; j < num_iterations; j++) {
        read_bytes = fread(buffer,  1, BUFFSIZE, file_ptr);
        send(out_sock, buffer, read_bytes, 0);
        bzero(buffer, sizeof(buffer));
        recv(out_sock, buffer, sizeof(buffer), 0);
        bzero(buffer, sizeof(buffer));
      }
      read_bytes = fread(buffer, 1, final_iteration, file_ptr);
      send(out_sock, buffer, read_bytes, 0);
      bzero(buffer, sizeof(buffer));
      recv(out_sock, buffer, sizeof(buffer), 0);
      bzero(buffer, sizeof(buffer));
    }

    if ((filesys_map[mod][i][0]==4)||(filesys_map[mod][i][1]==4)) {
      printf("---------------Part4--------------------\n");
      fseek(file_ptr, (3*part_size), SEEK_SET);
      bzero(buffer, sizeof(buffer));
      sprintf(buffer, "PUT .%s.4 %lu", filename, last_part);
      send(out_sock, buffer, strlen(buffer), 0);
      bzero(buffer, sizeof(buffer));
      recv(out_sock, buffer, sizeof(buffer), 0);
      bzero(buffer, sizeof(buffer));
      for (size_t j = 0; j < num_iterations; j++) {
        read_bytes = fread(buffer, 1, BUFFSIZE, file_ptr);
        send(out_sock, buffer, read_bytes, 0);
        bzero(buffer, sizeof(buffer));
        recv(out_sock, buffer, sizeof(buffer), 0);
        bzero(buffer, sizeof(buffer));
      }
      read_bytes = fread(buffer, 1, final_iteration, file_ptr);
      send(out_sock, buffer, read_bytes, 0);
      bzero(buffer, sizeof(buffer));
      recv(out_sock, buffer, sizeof(buffer), 0);
      bzero(buffer, sizeof(buffer));
    }
    bzero(buffer, sizeof(buffer));
    sprintf(buffer, "PUT Complete");
    send(out_sock, buffer, strlen(buffer), 0);
  }
  fclose(file_ptr);

  return 0;
}

int dfs_list() {
  FILE *list_file;
  char *line = NULL;
  size_t length;
  char fname[20] = "";
  int part_count = 1;
  list_file = fopen("list.txt", "a");
  char list_final[400];
  char buffer[BUFFSIZE];
  int recv_bytes = 0;
  bzero(list_final, sizeof(list_final));
  bzero(buffer, sizeof(buffer));
  if (dfs1_status == true) {
    send(dfc1_sock, "Send", 4, 0);
    recv_bytes = recv(dfc1_sock, buffer, sizeof(buffer), 0);
    fwrite(buffer, 1, recv_bytes, list_file);
    bzero(buffer, sizeof(buffer));
  }

  if (dfs2_status == true) {
    send(dfc2_sock, "Send", 4, 0);
    recv_bytes = recv(dfc2_sock, buffer, sizeof(buffer), 0);
    fwrite(buffer, 1, recv_bytes, list_file);
    bzero(buffer, sizeof(buffer));
  }

  if (dfs3_status == true) {
    send(dfc3_sock, "Send", 4, 0);
    recv_bytes = recv(dfc3_sock, buffer, sizeof(buffer), 0);
    fwrite(buffer, 1, recv_bytes, list_file);
    bzero(buffer, sizeof(buffer));
  }

  if (dfs4_status == true) {
    send(dfc4_sock, "Send", 4, 0);
    recv_bytes = recv(dfc4_sock, buffer, sizeof(buffer), 0);
    fwrite(buffer, 1, recv_bytes, list_file);
    bzero(buffer, sizeof(buffer));
  }
  fclose(list_file);
  usleep(100000);
  system("sort -u -o list.txt list.txt");
  usleep(100000);
  list_file = fopen("list.txt", "r");
  getline(&line, &length, list_file);
  strncpy(fname, (line+1), strlen(line)-4);
  while ((getline(&line, &length, list_file))!=-1) {
    if (strstr(line, fname)) {
      part_count++;
    }
    else {
      if (part_count == 4) {
        strcat(list_final, fname);
        strcat(list_final, "\n");
      }
      else {
        strcat(list_final, fname);
        strcat(list_final, "[Incomplete]\n");
      }
      bzero(fname, sizeof(fname));
      strncpy(fname, (line+1), strlen(line)-4);
      part_count = 1;
    }
  }
  if (part_count == 4) {
    strcat(list_final, fname);
    strcat(list_final, "\n");
  }
  else {
    strcat(list_final, fname);
    strcat(list_final, "[Incomplete]\n");
  }
  fclose(list_file);
  printf("list_final:\n%s \n", list_final);
  fflush(stdout);
  remove("list.txt");
  return 0;
}



//Advanced GET
int get_file(char* file_name) {
  char p1_name[20];
  char p2_name[20];
  char p3_name[20];
  char p4_name[20];
  sprintf(p1_name, ".%s.1", file_name);
  sprintf(p2_name, ".%s.2", file_name);
  sprintf(p3_name, ".%s.3", file_name);
  sprintf(p4_name, ".%s.4", file_name);
  char dfs1_list[BUFFSIZE];
  char dfs2_list[BUFFSIZE];
  char dfs3_list[BUFFSIZE];
  char dfs4_list[BUFFSIZE];
  if (dfs1_status == true) {
    recv(dfc1_sock, dfs1_list, sizeof(dfs1_list), 0);

  }

  if (dfs2_status == true) {
    recv(dfc2_sock, dfs2_list, sizeof(dfs2_list), 0);
  }

  if (dfs3_status == true) {
    recv(dfc3_sock, dfs3_list, sizeof(dfs3_list), 0);
  }

  if (dfs4_status == true) {
    recv(dfc4_sock, dfs4_list, sizeof(dfs4_list), 0);
  }

  return 0;
}




int main(int argc, char *argv[])
{

  int auth;
  char buffer[BUFFSIZE];
  char request_type[5];
  char file_name[40] = "all";

  if (argc!=2)
  {
    printf("usage: %s [Config file name]\n",argv[0]);
    exit(1);
  }

  printf("Parsing dfc.conf...\n");
/* Parsing Authentication File*/
  if(parse_dfc_conf(argv[1]))
  {
    printf("Configuration file parsing failed\n");
    exit(1);
  }


/* build address data structures for dfs1, dfs2, dfs3 and dfs4 */
  bzero((char *)&dfs1_add, sizeof(dfs1_add));
  dfs1_add.sin_family = AF_INET;
  dfs1_add.sin_addr.s_addr = inet_addr(conf.dfs1_ip);
  dfs1_add.sin_port = htons(conf.dfs1_port);

  bzero((char *)&dfs2_add, sizeof(dfs2_add));
  dfs2_add.sin_family = AF_INET;
  dfs2_add.sin_addr.s_addr = inet_addr(conf.dfs2_ip);
  dfs2_add.sin_port = htons(conf.dfs2_port);

  bzero((char *)&dfs3_add, sizeof(dfs3_add));
  dfs3_add.sin_family = AF_INET;
  dfs3_add.sin_addr.s_addr = inet_addr(conf.dfs3_ip);
  dfs3_add.sin_port = htons(conf.dfs3_port);

  bzero((char *)&dfs4_add, sizeof(dfs4_add));
  dfs4_add.sin_family = AF_INET;
  dfs4_add.sin_addr.s_addr = inet_addr(conf.dfs4_ip);
  dfs4_add.sin_port = htons(conf.dfs4_port);


/*Creating sockets for communication with servers*/
  if ((dfc1_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("\nsocket creation error");
    exit(1);
  }
  if ((dfc2_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("\nsocket creation error");
    exit(1);
  }
  if ((dfc3_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("\nsocket creation error");
    exit(1);
  }
  if ((dfc4_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("\nsocket creation error");
    exit(1);
  }


  printf("Distributed File System\nUsage:\nPUT <filename>\nGET <filename>\nLIST\n");
  while (scanf("%30[^\n]%*c", buffer))
  {
    sscanf(buffer, "%s %*s", request_type);
    auth = req_auth(request_type, file_name);
    if (auth == 0)
    {
      if ((strcmp(request_type, "PUT") == 0)) {
        sscanf(buffer, "%*s %s", file_name);
        printf("File :%s\n", file_name);
      }

      else if (strcmp(request_type, "GET") == 0) {
        sscanf(buffer, "%*s %s", file_name);
        printf("GET command\n");
      }

      else if (strcmp(request_type, "LIST") == 0) {
        printf("Listing files on servers\n");
        dfs_list();
      }

      else {
        printf("Invalid command, refer to Usage\n");
      }
    }
    else {
      continue;
    }

    bzero(buffer, sizeof(buffer));
    bzero(file_name, sizeof(file_name));
    strcpy(file_name, "all");
    bzero(request_type, sizeof(request_type));
  }
}
