/*
ECEN 5273 Network Systems
Author: Mukund Madhusudan Atre
*/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include<errno.h>


#define SERVER_PORT 8888
#define MAX_CONNECT 3
#define BUFFSIZE 1024

void *Handler(void * sock_d);

void *Handler(void * sock_d)
{
  int conn_sock;
  int recv_bytes;
  conn_sock =  *(int *) sock_d;
  char in_buffer[BUFFSIZE];
  char out_buffer[BUFFSIZE];

  strcpy(out_buffer, "Hello from the server\n");
  write(conn_sock, out_buffer, strlen(out_buffer));
  bzero(out_buffer, sizeof(out_buffer));

  while(recv_bytes = recv(conn_sock, in_buffer, sizeof(in_buffer), 0) > 0)
  {
    if(strncmp(in_buffer, "exit", 4) == 0){break;}
    strcpy(out_buffer, in_buffer);
    write(conn_sock, out_buffer, strlen(out_buffer));
    bzero(out_buffer, sizeof(out_buffer));
    bzero(in_buffer, sizeof(in_buffer));
  }
  close(conn_sock);
  pthread_exit(NULL);

}


int main(void)
{
  //Declare Variables
  int server_sock, client_sock;
  int id, cl_adlen;
  struct sockaddr_in server_add, client_add;
  pthread_t thread;

  //Create Socket
  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if(server_sock == -1)
  {
    perror("Socket Creation Failed: ");
  }

  //Set server Address
  server_add.sin_family = AF_INET;
  server_add.sin_addr.s_addr = INADDR_ANY;
  server_add.sin_port = htons(SERVER_PORT);

  //Bind socket to given Address
  if(bind(server_sock, (struct sockaddr *) &server_add, sizeof(server_add)) < 0)
  {
    perror("Unable to bind socket to given address: ");
    return 1;
  }

  //Listen to socket for incoming connections
  listen(server_sock, MAX_CONNECT);

  while (1)
  {
    //Accept first client in the queue
    cl_adlen = sizeof(client_add);
    client_sock = accept(server_sock, (struct sockaddr *)&client_add, &cl_adlen);
    if(client_sock == -1)
    {
      perror("Error creating connected socket: ");
    }
    else
    {
      id = client_sock;
      if(pthread_create(&thread, NULL, &Handler, &id) != 0)
      {
        perror("Thread Creation Failed: ");
      }
      printf("Thread created for client_sock %d on port %d\n", id, ntohs(client_add.sin_port));

    }

  }
  close(server_sock);
  return (1);

}
