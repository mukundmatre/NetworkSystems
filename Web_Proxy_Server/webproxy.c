/*
Web Proxy server using C
*/
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/time.h>
#include<sys/select.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include<errno.h>
#include<libgen.h>
#include<unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <sys/stat.h>


#define BUFFSIZE 10240
#define MAX_PENDING 64
#define RSP_400BR "HTTP/1.1 400 Bad Request\r\nContent-Type:text/html\r\n\r\n"
#define BR_MESSAGE_M "<html><body>400 Bad Request Reason: Invalid Method :<<request method>></body></html>\r\n\r\n"
#define BR_MESSAGE_U "<html><body>400 Bad Request Reason: Invalid URL: <<requested url>></body></html>\r\n\r\n"

int main(int argc, char *argv[])
{
  char proxy_port[10] = "10001";
  struct sockaddr_in proxy_add;
  struct sockaddr_in client_add;
  struct sockaddr_in server_add;
  struct hostent *hp;
  unsigned int client_add_len;
  int proxy_sock, conn_sock, server_sock;
  int recv_bytes = 0;
  char request_type[10];
  char url[BUFFSIZE];
  char http_version[10];
  char host_name[50];
  char ctop_buffer[BUFFSIZE];
  char stop_buffer[BUFFSIZE];
  char ptoc_buffer[BUFFSIZE];
  int pid;


  if(argc!=2)
  {
    printf("usage: %s [Port]\n",argv[0]);
    exit(1);
  }
  else
  {
    strcpy(proxy_port, argv[1]);
  }

  /* build address data structure */
  bzero((char *)&proxy_add, sizeof(proxy_add));
  proxy_add.sin_family = AF_INET;
  proxy_add.sin_addr.s_addr = INADDR_ANY;
  proxy_add.sin_port = htons(atoi(proxy_port));

  /* build address data structure for server */
  bzero((char *)&server_add, sizeof(server_add));
  server_add.sin_family = AF_INET;
  server_add.sin_port = htons(atoi("80"));

/* setup passive open */
  if ((proxy_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("\nproxy_sock creation error");
    exit(1);
  }

  if ((bind(proxy_sock, (struct sockaddr *)&proxy_add, sizeof(proxy_add))) < 0) {
    perror("\nproxy_sock bind error");
    exit(1);
  }

  listen(proxy_sock, MAX_PENDING);

  while(1) {
    client_add_len = sizeof(client_add);
    if ((conn_sock = accept(proxy_sock, (struct sockaddr *)&client_add, &client_add_len)) < 0) {
      perror("\naccept error on proxy");
      continue;
    }

    //printf("\nRequest for Connection from Client at port:%d\n", ntohs(client_add.sin_port));
  // Fork for handling multiple connections
    pid = fork();

    if (pid == 0) {
      while(1) {
        bzero(ctop_buffer, sizeof(ctop_buffer));
        bzero(stop_buffer, sizeof(stop_buffer));
        bzero(ptoc_buffer, sizeof(ptoc_buffer));
        recv_bytes = recv(conn_sock, ctop_buffer, sizeof(ctop_buffer), 0);
        if (recv_bytes == 0)
        {
          printf("Request Complete\n");
          close(conn_sock);
          break;
        }
        sscanf(ctop_buffer, "%s %s %s", request_type, url, http_version);
        if (strcmp(request_type, "GET") == 0) {
          printf("Request:***%s***\n", ctop_buffer);
          sscanf(url, "%*[^/]%*c%*c%[^/]", host_name);
          printf("\nHostname:***%s***\n", host_name);
          /*Creating socket for communication to server*/
          if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("\nserver_sock creation error");
            exit(1);
          }
          hp = gethostbyname(host_name);
          if (hp == NULL) {
            perror("\nHost unknown");
            continue;
          }
          bcopy(hp->h_addr, (char *)&server_add.sin_addr, hp->h_length);
          if (connect(server_sock, (struct sockaddr *)&server_add, sizeof(server_add)) < 0 && errno!=EISCONN)
          {
            perror("\nunable to connect to host");
          }
          send(server_sock, ctop_buffer, strlen(ctop_buffer), 0);
          while((recv_bytes = recv(server_sock, stop_buffer, sizeof(stop_buffer), 0))) {
            printf("Recv bytes: %d\n", recv_bytes);
            send(conn_sock, stop_buffer, recv_bytes, 0);
            bzero(stop_buffer, sizeof(stop_buffer));
          }

        }
        else {
          bzero(ptoc_buffer, sizeof(ptoc_buffer));
          strcpy(ptoc_buffer, RSP_400BR);
          send(conn_sock, ptoc_buffer, strlen(ptoc_buffer), 0);
          bzero(ptoc_buffer, sizeof(ptoc_buffer));
          strcpy(ptoc_buffer, BR_MESSAGE_M);
          send(conn_sock, ptoc_buffer, strlen(ptoc_buffer), 0);
        }

    }
    exit(0);
  }
  }
  return 0;
}
