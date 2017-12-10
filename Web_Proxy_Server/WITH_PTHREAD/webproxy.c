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
#include <time.h>
#include <signal.h>


#define BUFFSIZE 10240
#define MAX_PENDING 1000
#define RSP_400BR "HTTP/1.1 400 Bad Request\r\nContent-Type:text/html\r\n\r\n"
#define RSP_403FORB "HTTP/1.1 403 Forbidden\r\nContent-Type:text/html\r\n"
#define FORB_MESSAGE "<html><body>403 Blocked Website</body></html>\r\n\r\n"
#define BR_MESSAGE_M "<html><body>400 Bad Request Reason: Invalid Method:<<request method>></body></html>\r\n\r\n"
#define BR_MESSAGE_U "<html><body>400 Bad Request Reason: Invalid URL: <<requested url>></body></html>\r\n\r\n"
#define CACHE_DIR "./cache/"
#define HOST_CACHE_PATH "hostname_cache.txt"
#define BLOCK_LIST_PATH "block_list.txt"

typedef struct {
  char file_path[50];
  char hostname[100];
  char ip_as_str[20];
}Args_t;

long int cache_timeout;


void calculate_md5sum(char* url, char* md5string);
int check_if_blocked(char* host_ip);
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


int check_if_blocked(char* host_ip) {
  FILE *block_list_ptr;
  block_list_ptr = fopen(BLOCK_LIST_PATH, "r");
  char *line = NULL;
  size_t length;
  while((getline(&line, &length, block_list_ptr))!=-1) {
    if (strstr(line, host_ip)) {
      return 1;
    }
  }
  return 0;
}


int send_cached_file(char* url, int client_sock) {
  FILE *file_ptr;
  char file_path[50];
  int read_bytes;
  char buffer[BUFFSIZE];
  char md5string[33];
  bzero(buffer, sizeof(buffer));
  bzero(md5string, sizeof(md5string));
  bzero(file_path, sizeof(file_path));
  char *line = NULL;
  size_t length;
  calculate_md5sum(url, md5string);
  sprintf(file_path,"%s%s", CACHE_DIR, md5string);
  file_ptr = fopen(file_path, "r");
  if (getline(&line, &length, file_ptr)==-1) {
    return 1;
  }
  while((read_bytes = fread(buffer, 1, sizeof(buffer), file_ptr))) {
    send(client_sock, buffer, read_bytes, MSG_NOSIGNAL);
    bzero(buffer, sizeof(buffer));
  }
  fclose(file_ptr);
  return 0;
}


int check_file_cache(char* url) {
  FILE *file_ptr;
  char md5string[33];
  char file_path[50];
  bzero(md5string, sizeof(md5string));
  bzero(file_path, sizeof(file_path));
  time_t now;
  time_t start;
  time_t diff;
  char *line = NULL;
  size_t length;
  now = time(NULL);
  calculate_md5sum(url, md5string);
  sprintf(file_path,"%s%s", CACHE_DIR, md5string);
  file_ptr = fopen(file_path, "r");
  if (file_ptr == NULL) {
    return 1;
  }
  else {
    getline(&line, &length, file_ptr);
    sscanf(line, "%ld", &start);
    diff = (now - start);
    if (diff>cache_timeout) {
      fclose(file_ptr);
      remove(file_path);
      return 1;
    }
    fclose(file_ptr);
    return 0;
  }
}


int cache_host_ip(char* hostname, char* ip_string) {
  FILE *host_cache_ptr;
  host_cache_ptr = fopen(HOST_CACHE_PATH, "a");
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

void *LinkPrefetcher(void* args_struct) {
  Args_t arg_data = *(Args_t *)args_struct;
  int server_sock;
  char file_path[50];
  bzero(file_path, sizeof(file_path));
  strcpy(file_path, arg_data.file_path);
  char host_name[100];
  bzero(host_name, sizeof(host_name));
  strcpy(host_name, arg_data.hostname);
  char ip_string[20];
  bzero(ip_string, sizeof(ip_string));
  strcpy(ip_string, arg_data.ip_as_str);
  char cache_path[50];
  char request[BUFFSIZE];
  char ptof_buffer[BUFFSIZE];
  char md5string[33];
  FILE *html_ptr;
  FILE *cache_ptr;
  time_t start;
  char url[100];
  char short_link[100];
  char *link_start = NULL;
  char *href_start = NULL;
  char *line = NULL;
  size_t length;
  int recv_bytes;
  size_t file_size = 0;
  struct sockaddr_in server_add;

  bzero((char *)&server_add, sizeof(server_add));
  server_add.sin_family = AF_INET;
  server_add.sin_port = htons(atoi("80"));
  server_add.sin_addr.s_addr = inet_addr(ip_string);


  html_ptr = fopen(file_path, "r");
  if (html_ptr==NULL) {
    pthread_exit(NULL);
  }

  while((getline(&line, &length, html_ptr))!=-1) {
    link_start = NULL;
    href_start = NULL;
    bzero(url, sizeof(url));
    bzero(cache_path, sizeof(cache_path));
    bzero(short_link, sizeof(short_link));
    bzero(md5string, sizeof(md5string));
    bzero(request, sizeof(request));
    bzero(ptof_buffer, sizeof(ptof_buffer));
    if ((href_start = strstr(line, "href"))) {
      if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("\nserver_sock creation error");
        pthread_exit(NULL);
      }

      if (connect(server_sock, (struct sockaddr *)&server_add, sizeof(server_add)) < 0 && errno!=EISCONN)
      {
        perror("\nunable to connect to host");
        pthread_exit(NULL);
      }

      if((link_start = strstr(line, "http://"))) {
        sscanf(link_start, "%[^\"]", url);
      }
      else {
        sscanf(href_start, "%*[^=]%*c%*c%[^\"]", short_link);
        if (short_link[0] == '/') {
          sprintf(url, "http://%s%s", host_name, short_link);
        }
        else {
          sprintf(url, "http://%s/%s", host_name, short_link);
        }
      }
      sprintf(request, "GET %s HTTP/1.0\r\n\r\n", url);
      calculate_md5sum(url, md5string);
      sprintf(cache_path, "%s%s", CACHE_DIR, md5string);
      send(server_sock, request, strlen(request), MSG_NOSIGNAL);
      printf("Cache Path:%s\n", cache_path);
      cache_ptr = fopen(cache_path, "a");
      start = time(NULL);
      fprintf(cache_ptr, "%lu\n", start);
      while((recv_bytes = recv(server_sock, ptof_buffer, sizeof(ptof_buffer), 0))) {
        fwrite(ptof_buffer, 1, recv_bytes, cache_ptr);
        bzero(ptof_buffer, sizeof(ptof_buffer));
      }
      file_size = ftell(cache_ptr);
      fclose(cache_ptr);
      if (file_size<20) {
        remove(cache_path);
      }
      printf("Prefetched link:%s\n", url);
      close(server_sock);
    }
    else {
      continue;
    }
  }
  fclose(html_ptr);
  printf("***************Prefetch READY************\n");
  pthread_exit(NULL);
}


void *Handler(void* socket_desc) {
  int conn_sock = *(int*)socket_desc;
  FILE *file_ptr;
  Args_t args;
  time_t start;
  int res;
  struct sockaddr_in server_add;
  struct hostent *hp;
  int server_sock;
  int recv_bytes = 0;
  char ctop_buffer[BUFFSIZE];
  char stop_buffer[BUFFSIZE];
  char ptoc_buffer[BUFFSIZE];
  char http_version[10];
  char host_name[100];
  char request_type[10];
  char url[BUFFSIZE];
  char md5string[33];
  char file_path[50];
  char url_ip[20];
  char url_port[10];
  char ip_string[20];
  bool prefetch_flag = false;
  pthread_t prefetch_thread;
  pthread_attr_t prefetch_attr;
  size_t file_size = 0;

  res = pthread_attr_init(&prefetch_attr);
  if (res != 0) {
      perror("Attribute init failed");
      exit(-1);
  }

//Detached state for threads
  res = pthread_attr_setdetachstate(&prefetch_attr, PTHREAD_CREATE_DETACHED);
  if (res != 0) {
      perror("\nSetting detached state failed");
      exit(-1);
  }

  /* build address data structure for server */
  bzero((char *)&server_add, sizeof(server_add));
  server_add.sin_family = AF_INET;
  server_add.sin_port = htons(atoi("80"));

  while(1) {
    bzero(ctop_buffer, sizeof(ctop_buffer));
    bzero(stop_buffer, sizeof(stop_buffer));
    bzero(ptoc_buffer, sizeof(ptoc_buffer));
    bzero(http_version, sizeof(http_version));
    bzero(host_name, sizeof(host_name));
    bzero(request_type, sizeof(request_type));
    bzero(url, sizeof(url));
    bzero(md5string, sizeof(md5string));
    bzero(file_path, sizeof(file_path));
    bzero(url_ip, sizeof(url_ip));
    bzero(url_port, sizeof(url_port));
    bzero(ip_string, sizeof(ip_string));

    recv_bytes = recv(conn_sock, ctop_buffer, sizeof(ctop_buffer), 0);
    if (recv_bytes == 0)
    {
      printf("Request Complete\n");
      break;
    }
    sscanf(ctop_buffer, "%s %s %s", request_type, url, http_version);

    // Filter GET requests
    if (strcmp(request_type, "GET") == 0) {

      if (strstr(url, "https")) {
        strcpy(ptoc_buffer, RSP_400BR);
        send(conn_sock, ptoc_buffer, strlen(ptoc_buffer), MSG_NOSIGNAL);
        bzero(ptoc_buffer, sizeof(ptoc_buffer));
        strcpy(ptoc_buffer, BR_MESSAGE_U);
        send(conn_sock, ptoc_buffer, strlen(ptoc_buffer), MSG_NOSIGNAL);
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
        if (strchr(host_name, ':')) {
          sscanf(host_name, "%[^:]%*c%[^/]", url_ip, url_port);
          if (check_if_blocked(url_ip) == 1) {
            bzero(ptoc_buffer, sizeof(ptoc_buffer));
            strcpy(ptoc_buffer, RSP_403FORB);
            send(conn_sock, ptoc_buffer, strlen(ptoc_buffer), MSG_NOSIGNAL);
            bzero(ptoc_buffer, sizeof(ptoc_buffer));
            strcpy(ptoc_buffer, FORB_MESSAGE);
            send(conn_sock, ptoc_buffer, strlen(ptoc_buffer), MSG_NOSIGNAL);
            continue;
          }
          server_add.sin_addr.s_addr = inet_addr(url_ip);
          server_add.sin_port = htons(atoi(url_port));
        }
        else {
          if (check_if_blocked(host_name) == 1) {
            bzero(ptoc_buffer, sizeof(ptoc_buffer));
            strcpy(ptoc_buffer, RSP_403FORB);
            send(conn_sock, ptoc_buffer, strlen(ptoc_buffer), MSG_NOSIGNAL);
            bzero(ptoc_buffer, sizeof(ptoc_buffer));
            strcpy(ptoc_buffer, FORB_MESSAGE);
            send(conn_sock, ptoc_buffer, strlen(ptoc_buffer), MSG_NOSIGNAL);
            continue;
          }
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
        }


        /*Creating socket for communication to server*/
        if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
          perror("\nserver_sock creation error");
          continue;
        }

        // Connect to the server
        if (connect(server_sock, (struct sockaddr *)&server_add, sizeof(server_add)) < 0 && errno!=EISCONN)
        {
          perror("\nunable to connect to host");
          continue;
        }

        // Send the request obtained from client to the server
        send(server_sock, ctop_buffer, strlen(ctop_buffer), MSG_NOSIGNAL);

        // Receive response from server and Write file to cache with timestamp
        // Send it to the client at the same time
        file_ptr = fopen(file_path, "a");
        start = time(NULL);
        fprintf(file_ptr, "%lu\n", start);
        while((recv_bytes = recv(server_sock, stop_buffer, sizeof(stop_buffer), 0))) {
          if (strstr(stop_buffer, "<html")) {
            prefetch_flag = true;
          }
          printf("Recv bytes: %d\n", recv_bytes);
          fwrite(stop_buffer, 1, recv_bytes, file_ptr);
          send(conn_sock, stop_buffer, recv_bytes, MSG_NOSIGNAL);
          bzero(stop_buffer, sizeof(stop_buffer));
        }
        file_size = ftell(file_ptr);
        fclose(file_ptr);
        if (file_size<20) {
          remove(file_path);
        }

        if (prefetch_flag == true) {
          strcpy(args.file_path, file_path);
          strcpy(args.hostname, host_name);
          strcpy(args.ip_as_str, inet_ntoa(server_add.sin_addr));
          if(pthread_create(&prefetch_thread, &prefetch_attr, &LinkPrefetcher,(void *) &args) != 0)
          {
            perror("\n\t\t\t+++++++++++++Thread Creation Failed: ");
          }
        }
      }

    }
    else {
      bzero(ptoc_buffer, sizeof(ptoc_buffer));
      strcpy(ptoc_buffer, RSP_400BR);
      send(conn_sock, ptoc_buffer, strlen(ptoc_buffer), MSG_NOSIGNAL);
      bzero(ptoc_buffer, sizeof(ptoc_buffer));
      strcpy(ptoc_buffer, BR_MESSAGE_M);
      send(conn_sock, ptoc_buffer, strlen(ptoc_buffer), MSG_NOSIGNAL);
    }

  }
  close(conn_sock);
  pthread_exit(NULL);
}



int main(int argc, char *argv[])
{
  int res;
  pthread_t thread;
  pthread_attr_t attr;
  char proxy_port[10] = "10001";
  struct sockaddr_in proxy_add;
  struct sockaddr_in client_add;
  unsigned int client_add_len;
  int proxy_sock, conn_sock;
  int *sock;

  if(argc!=3)
  {
    printf("usage: %s [Port] [Cache Timeout]\n",argv[0]);
    exit(1);
  }
  else
  {
    strcpy(proxy_port, argv[1]);
    cache_timeout = atol(argv[2]);
  }

  res = pthread_attr_init(&attr);
  if (res != 0) {
      perror("Attribute init failed");
      exit(-1);
  }

//Detached state for threads
  res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (res != 0) {
      perror("\nSetting detached state failed");
      exit(-1);
  }

  //signal(SIGPIPE, SIG_IGN);

  /* build address data structure */
  bzero((char *)&proxy_add, sizeof(proxy_add));
  proxy_add.sin_family = AF_INET;
  proxy_add.sin_addr.s_addr = INADDR_ANY;
  proxy_add.sin_port = htons(atoi(proxy_port));

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
    else {
      sock = (int *)malloc(1);
      *sock = conn_sock;
      if(pthread_create(&thread, &attr, &Handler, (void *) sock) != 0)
      {
        perror("\n\t\t\t+++++++++++++Thread Creation Failed: ");
      }
    }
  }
  return 0;
}
