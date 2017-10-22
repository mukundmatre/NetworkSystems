/*
ECEN 5273 Network Systems
Author: Mukund Madhusudan Atre
*/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include<errno.h>
#include<libgen.h>


#define MAX_CONNECT 32
#define BUFFSIZE 1024
#define RSP_404NF "HTTP/1.1 404 Not Found\nContent-Type:text/html\n\n"
#define NF_MESSAGE "<html><body>404 Not Found Reason URL does not exist :<<requested url>></body></html>"
#define RSP_501NI "HTTP/1.1 501 Not Implemented\nContent-Type:text/html\n\n"
#define NI_MESSAGE "<html><body>501 Not Implemented <<error type>>: <<requested data>></body></html>"


typedef struct
{
  int server_port;
  char doc_root[200];
  char content_type[200];
}Config_t;

typedef struct
{
  int id;
  Config_t sconf;
}Args_t;


// pthread_mutex_t lock;

void *Handler(void * args);
int read_conf(Config_t *sc_ptr);


int read_conf(Config_t *sc_ptr)
{
  FILE *conf_ptr;
  bzero(sc_ptr->doc_root, sizeof(sc_ptr->doc_root));
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  conf_ptr = fopen("ws.conf", "r");
  if(conf_ptr!=NULL)
  {
    while((read = getline(&line, &len, conf_ptr)) != -1)
    {

      if (strstr(line, "Listen") != NULL)
      {
        sc_ptr->server_port = atoi(line+6);
      }

      if (strstr(line, "DocumentRoot") != NULL)
      {
        strncpy(sc_ptr->doc_root, line+14, strlen(line+16));
        sc_ptr->doc_root[strlen(sc_ptr->doc_root)] = '\0';
      }

      if (line[0] == '.')
      {
        strcat(sc_ptr->content_type, line);
      }

    }
    fclose(conf_ptr);
  }

  else
  {
      return -1;
  }
  return 0;
}



void *Handler(void * args)
{

  Args_t conf_data = *(Args_t*) args;
  FILE *file_ptr;
  char in_buffer[BUFFSIZE];
  char out_buffer[BUFFSIZE];
  char http_request[BUFFSIZE];
  char req_type[10];
  char req_path[50];
  char req_version[10];
  char path[250];
  char ext[10];
  char cont_len[10];
  int conn_sock = 0;
  char *req;
  char *extension;
  char *con_type;
  int recv_bytes;
  int read_len;
  size_t file_size;
  char *line;
  char *read;


  conn_sock = conf_data.id;
  if(conn_sock==0)
  {
    perror("\n*******************************Conn_sock error");
    close(conn_sock);
    pthread_exit(NULL);
  }
  printf("Socket Id: %d\n", conn_sock);
  char response_ok[100] = "HTTP/1.1 200 Document Follows\nContent-Type:";
  bzero(in_buffer, sizeof(in_buffer));
  strcpy(path, conf_data.sconf.doc_root);
  printf("Doc_root: %s\n", path);

  recv_bytes = recv(conn_sock, in_buffer, sizeof(in_buffer), 0);
  if (recv_bytes < 1)
  {
    perror("\n\n########Receive Error########## ");
    printf("Socket [%d] closed\n", conn_sock);
    close(conn_sock);
    pthread_exit(NULL);
  }

  strcpy(http_request, in_buffer);
  req = strdup(http_request);
  strcpy(req_type, strtok_r(req, " ", &req));
  strcpy(req_path, strtok_r(req, " ", &req));
  strcpy(req_version, strtok_r(req, "\n", &req));
  if(strstr(http_request, "keep-alive")!=NULL)
  {
    printf("Keep Alive Detected\n");
  }
  printf("Request version: %s\n", req_version);


  if (!strcmp(req_type, "GET"))
  {
    if (!strcmp(req_path, "/"))
    {
      strcat(path, "/index.html");
      file_ptr = fopen(path, "r");
      if (file_ptr==NULL)
      {
        strcpy(out_buffer, RSP_404NF);
        send(conn_sock, out_buffer, strlen(out_buffer), 0);
        bzero(out_buffer, sizeof(out_buffer));
        strcpy(out_buffer, NF_MESSAGE);
        send(conn_sock, out_buffer, strlen(out_buffer), 0);
        bzero(out_buffer, sizeof(out_buffer));
        printf("Socket [%d] closed\n", conn_sock);
        close(conn_sock);
        pthread_exit(NULL);
      }
      else
      {
        con_type = "text/html";
      }
    }


    else
    {
      strcat(path, req_path);
      file_ptr = fopen(path, "r");
      if (file_ptr==NULL)
      {
        perror("\n###########File open error");
        strcpy(out_buffer, RSP_404NF);
        send(conn_sock, out_buffer, strlen(out_buffer), 0);
        bzero(out_buffer, sizeof(out_buffer));
        strcpy(out_buffer, NF_MESSAGE);
        send(conn_sock, out_buffer, strlen(out_buffer), 0);
        printf("Response: %s\n", out_buffer);
        close(conn_sock);
        pthread_exit(NULL);
      }
      else
      {
        read = strdup(conf_data.sconf.content_type);
        while(line = strtok_r(read, "\n", &read))
        {
          extension = strtok_r(line, " ", &line);
          //printf("\t\t\tExtension: %s\n", extension);
          if(strstr(path, extension) != NULL)
          {
            con_type = strtok_r(line, "\n", &line);
            //printf("content_type: %s\n", con_type);
            break;
          }
        }
        if(con_type==NULL)
        {
          close(conn_sock);
          pthread_exit(NULL);
        }
      }
    }

  printf("Path: %s\n", path);

  fseek(file_ptr, 0, SEEK_END); // seek to end of file
  file_size = ftell(file_ptr); // get current file pointer
  fseek(file_ptr, 0, SEEK_SET);
  sprintf(cont_len, "%lu", file_size);
  strcat(response_ok, con_type);
  strcat(response_ok, "\nContent-Length: ");
  strcat(response_ok, cont_len);
  strcat(response_ok, "\n\n");
  printf("Response: %s\n", response_ok);
  strcpy(out_buffer, response_ok);
  send(conn_sock, out_buffer, strlen(out_buffer), 0);
  bzero(out_buffer, sizeof(out_buffer));
  do {
    read_len = fread(out_buffer, 1, BUFFSIZE, file_ptr);
    send(conn_sock, out_buffer, sizeof(out_buffer), 0);
    bzero(out_buffer, sizeof(out_buffer));
  } while (read_len==BUFFSIZE);
  bzero(path, sizeof(path));
  fclose(file_ptr);
  }


  else if ((!strcmp(req_type, "POST"))||(!strcmp(req_type, "DELETE"))||(!strcmp(req_type, "OPTIONS"))||(!strcmp(req_type, "HEAD")))
  {
    strcpy(out_buffer, RSP_501NI);
    send(conn_sock, out_buffer, strlen(out_buffer), 0);
    bzero(out_buffer, sizeof(out_buffer));
    strcpy(out_buffer, NI_MESSAGE);
    send(conn_sock, out_buffer, strlen(out_buffer), 0);
    bzero(out_buffer, sizeof(out_buffer));
    printf("Socket [%d] closed\n", conn_sock);
    close(conn_sock);
    pthread_exit(NULL);
  }

  else
  {
    strcpy(out_buffer, RSP_501NI);
    send(conn_sock, out_buffer, strlen(out_buffer), 0);
    bzero(out_buffer, sizeof(out_buffer));
    strcpy(out_buffer, NI_MESSAGE);
    send(conn_sock, out_buffer, strlen(out_buffer), 0);
    bzero(out_buffer, sizeof(out_buffer));
    printf("Socket [%d] closed\n", conn_sock);
    close(conn_sock);
    pthread_exit(NULL);
  }

   printf("Socket %d Closed\n", conn_sock);
   close(conn_sock);
   pthread_exit(NULL);

}


int main(void)
{
  //Declare Variables
  int res;
  int server_sock, client_sock;
  int cl_adlen;
  Args_t args;
  struct sockaddr_in server_add, client_add;
  pthread_t thread;
  pthread_attr_t attr;
  res = pthread_attr_init(&attr);
  if (res != 0) {
      perror("Attribute init failed");
      exit(EXIT_FAILURE);
  }

  res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (res != 0) {
      perror("\nSetting detached state failed");
      exit(EXIT_FAILURE);
  }

  // Create Socket
  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if(server_sock == -1)
  {
    perror("\nSocket Creation Failed: ");
  }

  printf("Parsing Configuration File.....\n");
  if(read_conf(&args.sconf) == -1)
  {
    printf("Config File parse failed\n");
    exit(1);
  }
  else
  {
    printf("%d\n", args.sconf.server_port);
    printf("%s\n", args.sconf.doc_root);
    printf("%s\n", args.sconf.content_type);
  }
  //Set server Address
  server_add.sin_family = AF_INET;
  server_add.sin_addr.s_addr = INADDR_ANY;
  server_add.sin_port = htons(args.sconf.server_port);

  printf("Initializing Server.....\n");

  //Bind socket to given Address
  if(bind(server_sock, (struct sockaddr *) &server_add, sizeof(server_add)) < 0)
  {
    perror("\nUnable to bind socket to given address ");
    return 1;
  }

  //Listen to socket for incoming connections
  if(listen(server_sock, MAX_CONNECT) == -1)
  {
    perror("\nListen Error");
  }



  while (1)
  {
    //Accept first client in the queue
    cl_adlen = sizeof(client_add);
    client_sock = accept(server_sock, (struct sockaddr *)&client_add, &cl_adlen);
    if(client_sock < 0)
    {
      perror("\n************Error creating connected socket: ");
      continue;
    }
    else
    {
      args.id = client_sock;
      if(pthread_create(&thread, &attr, &Handler, (void *)&args) != 0)
      {
        perror("\n\t\t\t+++++++++++++Thread Creation Failed: ");
      }
      printf("\n\nThread created for client_sock %d on port %d\n", args.id, ntohs(client_add.sin_port));

    }
    //pthread_join(thread, NULL);
  }
  pthread_attr_destroy(&attr);
  // pthread_mutex_destroy(&lock);
  close(server_sock);
  return (1);

}
