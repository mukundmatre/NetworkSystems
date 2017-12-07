/*
Web Proxy server using C
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <openssl/md5.h>


#define BUFFSIZE 10240
#define MAX_PENDING 100
#define RSP_400BR "HTTP/1.1 400 Bad Request\r\nContent-Type:text/html\r\n\r\n"
#define BR_MESSAGE_M "<html><body>400 Bad Request Reason: Invalid Method :<<request method>></body></html>\r\n\r\n"
#define BR_MESSAGE_U "<html><body>400 Bad Request Reason: Invalid URL: <<requested url>></body></html>\r\n\r\n"
#define CACHE_DIR "./cache/"
#define HOST_CACHE_PATH "./cache/hostname_cache.txt"

void calculate_md5sum(char* url, char* md5string);
int send_cached_file(char* url, int client_sock);
int check_file_cache(char* url);
int cache_host_ip(char* hostname, char* ip_string);
int check_host_cache(char *hostname, char* ip_string);


void calculate_md5sum(char* url, char* md5string) {
  unsigned char url_md5[16];
  MD5((unsigned char*)url, strlen(url), url_md5);
  for(int i = 0; i < 16; ++i) {
    sprintf(&md5string[i*2], "%02x", (unsigned int)url_md5[i]);
  }
}


int send_cached_file(char* url, int client_sock) {
  FILE *file_ptr;
  char file_path[50];
  int read_bytes;
  char buffer[BUFFSIZE];
  char md5string[33];
  calculate_md5sum(url, md5string);
  sprintf(file_path,"%s%s", CACHE_DIR, md5string);
  file_ptr = fopen(file_path, "r");
  while((read_bytes = fread(buffer, 1, sizeof(buffer), file_ptr))) {
    send(client_sock, buffer, read_bytes, 0);
    bzero(buffer, sizeof(buffer));
  }
  fclose(file_ptr);
  return 0;
}


int check_file_cache(char* url) {
  FILE *file_ptr;
  char md5string[33];
  char file_path[50];
  calculate_md5sum(url, md5string);
  sprintf(file_path,"%s%s", CACHE_DIR, md5string);
  file_ptr = fopen(file_path, "r");
  if (file_ptr == NULL) {
    return 1;
  }
  else {
    fclose(file_ptr);
    return 0;
  }
}


int cache_host_ip(char* hostname, char* ip_string) {
  FILE *host_cache_ptr;
  host_cache_ptr = fopen(HOST_CACHE_PATH, "a+");
  fprintf(host_cache_ptr, "%s,%s\n", hostname, ip_string);
  fclose(host_cache_ptr);
  return 0;
}

// Function to check hostname in cache for IP
int check_host_cache(char *hostname, char* ip_string) {
  strcpy(ip_string, "none");
  FILE *host_cache_ptr;
  char *line = NULL;
  size_t length;
  host_cache_ptr = fopen(HOST_CACHE_PATH, "r");
  while((getline(&line, &length, host_cache_ptr))!=-1) {
    if (strstr(line, hostname)) {
      sscanf(line, "%*[^,]%*c%s", ip_string);
      break;
    }
  }
  fclose(host_cache_ptr);
  if (strcmp(ip_string, "none") == 0) {
    return 1;
  }
  else {
    return 0;
  }
}



int main(int argc, char *argv[])
{
  FILE *file_ptr;
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
  char host_name[100];
  char ctop_buffer[BUFFSIZE];
  char stop_buffer[BUFFSIZE];
  char ptoc_buffer[BUFFSIZE];
  int pid;
  char md5string[33];
  char file_path[50];

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

  // Fork for handling multiple connections
    pid = fork();

    if (pid == 0) {
      while(1) {
        char ip_string[20];
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
        // Filter GET requests
        if (strcmp(request_type, "GET") == 0) {

          if (strstr(url, "https")) {
            strcpy(ptoc_buffer, RSP_400BR);
            send(conn_sock, ptoc_buffer, strlen(ptoc_buffer), 0);
            bzero(ptoc_buffer, sizeof(ptoc_buffer));
            strcpy(ptoc_buffer, BR_MESSAGE_U);
            send(conn_sock, ptoc_buffer, strlen(ptoc_buffer), 0);
            bzero(ptoc_buffer, sizeof(ptoc_buffer));
            continue;
          }

          if (check_file_cache(url) == 0) {
            printf("File found in cache\n");
            send_cached_file(url, conn_sock);
          }
          else {
            calculate_md5sum(url, md5string);
            sprintf(file_path, "%s%s", CACHE_DIR, md5string);
            printf("File NOT found in cache\n");
            sscanf(url, "%*[^/]%*c%*c%[^/]", host_name);
            printf("\nHostname:***%s***\n", host_name);

            if (check_host_cache(host_name, ip_string) == 0) {
              server_add.sin_addr.s_addr = inet_addr(ip_string);
            }
            else {
              // Get IP from host name
              hp = gethostbyname(host_name);
              if (hp == NULL) {
                perror("\nHost unknown");
                continue;
              }
              // Copy the IP address obtained from gethostbyname() into address structure
              bcopy(hp->h_addr, (char *)&server_add.sin_addr, hp->h_length);
              cache_host_ip(host_name, inet_ntoa(server_add.sin_addr));
            }

            /*Creating socket for communication to server*/
            if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
              perror("\nserver_sock creation error");
              exit(1);
            }

            // Connect to the server
            if (connect(server_sock, (struct sockaddr *)&server_add, sizeof(server_add)) < 0 && errno!=EISCONN)
            {
              perror("\nunable to connect to host");
              continue;
            }

            // Send the request obtained from client to the server
            send(server_sock, ctop_buffer, strlen(ctop_buffer), 0);

            file_ptr = fopen(file_path, "a");
            // Receive response from server and send it to the client
            while((recv_bytes = recv(server_sock, stop_buffer, sizeof(stop_buffer), 0))) {
              printf("Recv bytes: %d\n", recv_bytes);
              fwrite(stop_buffer, 1, recv_bytes, file_ptr);
              send(conn_sock, stop_buffer, recv_bytes, 0);
              bzero(stop_buffer, sizeof(stop_buffer));
            }
            fclose(file_ptr);
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
