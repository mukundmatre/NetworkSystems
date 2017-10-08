/*
Client Code
Author: Mukund Madhusudan Atre
ECEN-5273 Network Systems
Programming Assignment 1
Reliable Communication with UDP along with Data Encryption
References:
    Example codes provided by Professor Sangtae Ha
    Stack Overflow
    GeeksforGeeks.com
*/

//Necessary includes
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <dirent.h>

//defining buffer size for taking commands
#define MAXBUFSIZE 1024
//defining the data size in packet structure
#define DATASIZE 1024
//defining Timeout value for recvfrom() function based on RTT
//acquired by running command on terminal: ping -c 10 -s 200 elra-01.cs.colorado.edu
#define TIMEOUT_SEC 0
#define TIMEOUT_MUSEC 30000

//Structure required for Packet
typedef struct PACKET{
    int packnum;
    int acknum;
    int flen;
    char data[DATASIZE];
}packet_t;



int main (int argc, char * argv[] )
{
	int sock;                           //This will be our socket
	struct timeval tv;
	packet_t client_pack;
	packet_t server_pack;
	struct sockaddr_in server_sock, remote;     //"Internet socket address structure"
	struct dirent *direntry;
	unsigned int remote_length;         //length of the sockaddr_in structure
	int nbytes, mbytes, i;                        //number of bytes we receive in our message
	FILE *fileptr;
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	char *command;
	char *fnameptr;
	char crypt_key[64] = "DifferencesWereMadeForIdentificationEndedUpBeingUsedForJudgement";
	//Check if Port number entered
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	bzero(&server_sock,sizeof(server_sock));                    //zero the struct
	server_sock.sin_family = AF_INET;                   //address family
	server_sock.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	server_sock.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine


	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}

	/******************
	  Once we've created a socket, we must bind that socket to the
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sock, (struct sockaddr *)&server_sock, sizeof(server_sock)) < 0)
	{
		printf("unable to bind socket\n");
	}


	remote_length = sizeof(remote);
	char msg[50];


 	while (1)
	 {
		 //Setting indefinite timeout for command
		 tv.tv_sec = 0;
		 tv.tv_usec = 0;
		 setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

		 bzero(buffer,sizeof(buffer));
		 bzero(msg, sizeof(msg));
		//waits for an incoming message
    nbytes = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&remote, &remote_length);
		command = strdup(buffer);
		//Splitting string
		strtok(command, " ");
		fnameptr = strtok(NULL, " ");
		printf("\nCommand: %s\n", command);
		if(fnameptr!=NULL){
		printf("File Name: %s\n", fnameptr);
		}

		if(strcmp(command, "get") == 0){
			//Setting timeout for acknowledgement from client
			tv.tv_sec = TIMEOUT_SEC;
			tv.tv_usec = TIMEOUT_MUSEC;
			setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
			bzero(buffer,sizeof(buffer));

			//Initializing Structs
			packet_t client_pack = {0};
      packet_t server_pack = {0};
			printf("get received\n");

			//Opening file to read
			fileptr = fopen(fnameptr, "rb");

			//Check if file exists on server or not and send confirmation to client
			if(fileptr==NULL){
      	printf("File open failed\n");
				strcpy(buffer, "Fail");
				nbytes = sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&remote, remote_length);
				continue;
      }
			else{
				strcpy(buffer, "Success");
				nbytes = sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&remote, remote_length);
			}

			do {
				bzero(server_pack.data,sizeof(server_pack.data));
				//Read data from file
				server_pack.flen = fread(server_pack.data, 1, DATASIZE, fileptr);
				server_pack.packnum++;

				//Encrypt data before sending to client
				for (i = 0; i < server_pack.flen; i++) {
        	server_pack.data[i] ^= crypt_key[i%64];
        }
				nbytes = sendto(sock, (packet_t *) &server_pack, sizeof(server_pack), 0, (struct sockaddr *)&remote, remote_length);
				printf("Packet: %d\n", server_pack.packnum);
        printf("server sent %d bytes\n", server_pack.flen);
        printf("nbytes: %d\n", nbytes);

				//Wait for ack until timeout and then retransmit packet
				if ((nbytes = recvfrom(sock, (packet_t *) &client_pack, sizeof(client_pack), 0, (struct sockaddr *) &remote, &remote_length)) < 0)
        {
          fseek(fileptr, (-1 * server_pack.flen), SEEK_CUR);
          server_pack.packnum--;
          printf("Timeout\n");
          continue;
        }
				printf("Some ack received\n");
				//Check if correct ack was received
        if(client_pack.acknum!=server_pack.packnum){
            fseek(fileptr, (-1 * server_pack.flen), SEEK_CUR);
            server_pack.packnum--;
            continue;
          }
					if(server_pack.flen!=DATASIZE){break;}
			} while(1);
			fclose(fileptr);
		}


		else if(strcmp(command, "put") == 0){
			bzero(buffer,sizeof(buffer));
			//Wait for confirmation from client
			nbytes = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&remote, &remote_length);
			if (strcmp(buffer, "Fail")==0) {
				continue;
			}
			packet_t client_pack = {0};
			packet_t server_pack = {0};
			fileptr = fopen(fnameptr, "ab");
			do {
				server_pack.packnum++;
				//Receive data from client
				nbytes = recvfrom(sock, (packet_t *) &client_pack, sizeof(client_pack), 0, (struct sockaddr *)&remote, &remote_length);
				//if already have a packet, jump to acknowledgement
				if(server_pack.packnum!=client_pack.packnum){
					server_pack.packnum--;
					goto retx_ack;
				}
				printf("Packet: %d\n", server_pack.packnum);
				printf("nbytes: %d\n", nbytes);
				printf("Data written %d bytes\n", client_pack.flen);

				//Decrypting data before write
				for (i = 0; i < client_pack.flen; i++) {
        	client_pack.data[i] ^= crypt_key[i%64];
        }
				fwrite(client_pack.data, client_pack.flen, 1, fileptr);

				retx_ack:
				server_pack.acknum = server_pack.packnum;
				mbytes = sendto(sock,(packet_t *) &server_pack, sizeof(server_pack), 0, (struct sockaddr *)&remote, remote_length);
				if(client_pack.flen!=DATASIZE){break;}
			} while(1);
			fclose(fileptr);
			printf("File written\n" );
			bzero(buffer,sizeof(buffer));
			//Send confirmation
			strcpy(buffer, "File written successfully on server");
			nbytes = sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&remote, remote_length);
		}

		//Delete if file exists and send confirmation
		else if(strcmp(command, "delete") == 0){
			bzero(buffer,sizeof(buffer));
			printf("Deleting file\n");
			if(remove(fnameptr)){
				strcpy(buffer, "File could not be deleted");
			}
			else{
				strcpy(buffer, "File delete successful");
			}

			nbytes = sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&remote, remote_length);
		}

		//List files from directory and send it to client
		else if(strcmp(command, "ls") == 0){
			bzero(buffer,sizeof(buffer));
			printf("Listing Files\n");
			DIR *dirptr = opendir(".");
			int i = 0;
			if(dirptr==NULL){
				strcpy(buffer, "Files cannot be listed");
				nbytes = sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&remote, remote_length);

			}
			else{
			bzero(buffer,sizeof(buffer));
			while ((direntry = readdir(dirptr)) != NULL)
			{
				strcat(buffer, direntry->d_name);
				strcat(buffer, "\n");
			}
			//strcat(buffer, "\nFiles listed successfully");
			nbytes = sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&remote, remote_length);
		}
			closedir(dirptr);
		}

		//Exit gracefully
		else if(strcmp(command, "exit") == 0){
			bzero(buffer,sizeof(buffer));
			printf("exit received\n");
			strcpy(buffer, "Server exited\nConnection closed");
			nbytes = sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&remote, remote_length);
			close(sock);
			exit(0);
		}


		//Send feedback if command is invalid
		else{
			bzero(buffer,sizeof(buffer));
			printf("%s: Not a valid command\n", buffer);
			strcpy(buffer, "Invalid Command");
			nbytes = sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&remote, remote_length);
		}

    bzero(buffer,sizeof(buffer));

	}


	close(sock);
}
