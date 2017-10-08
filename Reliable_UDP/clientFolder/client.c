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

//Includes required for string manipulation and sockets
#include <string.h>
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
#include <errno.h>

//defining buffer size for taking commands
#define MAXBUFSIZE 1024
//defining the data size in packet structure
#define DATASIZE 1024
//defining Timeout value for recvfrom() function based on RTT
//acquired by running command on terminal: ping -c 10 -s 200 elra-01.cs.colorado.edu
#define TIMEOUT_SEC 0
#define TIMEOUT_MUSEC 60000

//Structure required for Packet
typedef struct PACKET
{
    int packnum;                    //Packet Number
    int acknum;                     //Acknowledgement Number
    int flen;                       //File length read
    char data[DATASIZE];            //data array for holding file data
}packet_t;


int main(int argc, char * argv[])
{
    //Check if IP Address and port number have been put as command line arguments
    if (argc < 3)
    {
        printf("USAGE:  <server_ip> <server_port>\n");
        exit(1);
    }


    //Pointer for file
    FILE *fileptr;
    //Declaring Client packet and Server packet structures
    packet_t client_pack;
    packet_t server_pack;
    //Time keeping structure for storing timeout values
    struct timeval tv;
    int nbytes, mbytes, i;                             // number of bytes send by sendto()
    //Declaring buffer for taking commands and reciving messages from server
    char buffer[MAXBUFSIZE];
    struct sockaddr_in remote;              //Internet socket address structure
    int client_sock, slen=sizeof(remote);
    //array for storing commands from user
    char command[100];
    //pointers to command and filename
    char *cptr;
    char *fnameptr;

    //Encryption key for XOR Encryption of data
    char crypt_key[64] = "DifferencesWereMadeForIdentificationEndedUpBeingUsedForJudgement";

    //Initializing socket with SOCK_DGRAM and 0 as arguments indicating UDP Protocol
    if ( (client_sock=socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    //Setting the remote structure parameters from command line arguments
    bzero(&remote, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_port = htons(atoi(argv[2]));

    if (inet_aton(argv[1], &remote.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }


    while(1)
    {
        //Setting initial timeout values to be 0 (meaning till indefinite time) for commands other than put
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        if (setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error");
        }

        bzero(buffer, sizeof(buffer));

        //Taking command from user
        printf("\nEnter command : ");
        scanf("%30[^\n]%*c", command);
        //Splitting string into command type and fine name
        cptr = strdup(command);
        strtok(cptr, " ");
        printf("Entered command: %s\n", cptr);
        fnameptr = strtok(NULL, " ");
        if (fnameptr!=NULL) {
        printf("Filename: %s\n", fnameptr);
        }

        //send the command
        if ((nbytes = sendto(client_sock, command, sizeof(command) , 0 , (struct sockaddr *) &remote, slen))==-1)
        {
            perror("sendto()");
            exit(1);
        }

        //Condition for get
        if(strcmp(cptr, "get") == 0){
            bzero(buffer,sizeof(buffer));
            //Setting structures to zero
            packet_t client_pack = {0};
      		packet_t server_pack = {0};

            //Receive indication from server if the file exists or not
            nbytes = recvfrom(client_sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote, &slen);
            if (strcmp(buffer, "Fail")==0) {
                printf("File does not exist on server\n");
                continue;
            }
            //opening file in binary mode to write and append data
            fileptr = fopen(fnameptr, "ab");

            do {
                client_pack.packnum++;
                if ((nbytes = recvfrom(client_sock, (packet_t *) &server_pack, sizeof(server_pack), 0, (struct sockaddr *) &remote, &slen)) == -1)
                {
                    perror("recvfrom()");
                    exit(1);
                }
                if(server_pack.flen==0){break;}
                //If received packet is the previous one(retransmitted), send acknowledgement again for that packet
                if(client_pack.packnum!=server_pack.packnum){
                        client_pack.packnum--;
                        goto retx_ack;
				}

                printf("Packet: %d\n", server_pack.packnum);
				printf("nbytes: %d\n", nbytes);
				printf("Data written %d bytes\n", server_pack.flen);
                //Decrypting data received from server using secret key
                for (i = 0; i < server_pack.flen; i++) {
        	           server_pack.data[i] ^= crypt_key[i%64];
                }
                //Writing received data to file
                fwrite(server_pack.data, server_pack.flen, 1, fileptr);

                retx_ack:
                //Send acknowledgement to server for received packet
                client_pack.acknum = server_pack.packnum;
                mbytes = sendto(client_sock, (packet_t *) &client_pack, sizeof(client_pack), 0, (struct sockaddr *)&remote, slen);

            } while(1);
            fclose(fileptr);
            printf("File received successfully\n");
            bzero(buffer,sizeof(buffer));
        }


        else if(strcmp(cptr, "put") == 0){
            bzero(buffer,sizeof(buffer));
            //Setting timeout for receiving acknowledgement
            tv.tv_sec = TIMEOUT_SEC;
            tv.tv_usec = TIMEOUT_MUSEC;
            if (setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            perror("Error");
            }

            packet_t client_pack = {0};
            packet_t server_pack = {0};
            //Opening file to read data in binary
            fileptr = fopen(fnameptr, "rb");
            //Send "Fail" to server if file does not exist or cannot be opened
            if(fileptr==NULL){
                printf("File does not exist\n");
                strcpy(buffer, "Fail");
                nbytes = sendto(client_sock, buffer, sizeof(buffer) , 0 , (struct sockaddr *) &remote, slen);
                continue;
            }
            else{printf("File opened\n" );
            strcpy(buffer, "Success");
            nbytes = sendto(client_sock, buffer, sizeof(buffer) , 0 , (struct sockaddr *) &remote, slen);
            }

            do {
                bzero(client_pack.data,sizeof(client_pack.data));
                //Read data from file
                client_pack.flen = fread(client_pack.data, 1, DATASIZE, fileptr);
                client_pack.packnum++;
                //Encrypt Data Before sending
                for (i = 0; i < client_pack.flen; i++) {
                    client_pack.data[i] ^= crypt_key[i%64];
                }
                if ((nbytes = sendto(client_sock, (packet_t *) &client_pack , sizeof(client_pack) , 0 , (struct sockaddr *) &remote, slen))==-1)
                {
                    perror("sendto()\n");
                    exit(1);
                }
                printf("Packet: %d\n", client_pack.packnum);
                printf("client sent %d bytes\n", client_pack.flen);
                printf("nbytes: %d\n", nbytes);
                //bzero(server_pack.data,sizeof(server_pack.data));
                //Wait for acknowledgement until timeout and then retransmit the packet if timeout happens
                if ((nbytes = recvfrom(client_sock, (packet_t *) &server_pack, sizeof(server_pack), 0, (struct sockaddr *) &remote, &slen)) < 0)
                {
                    fseek(fileptr, (-1 * client_pack.flen), SEEK_CUR);
                    client_pack.packnum--;
                    printf("Timeout\n");
                    continue;
                }
                printf("Some ack received\n");
                //if acknowledgement received, check if it is desired one or previous delayed acknowledgement
                //if previous acknowledgement is found, then retransmit the packet
                if(server_pack.acknum!=client_pack.packnum){
                    fseek(fileptr, (-1 * client_pack.flen), SEEK_CUR);
                    client_pack.packnum--;
                    continue;
                }
                if(client_pack.flen!=DATASIZE){break;}
            } while(1);
            fclose(fileptr);
            bzero(buffer,sizeof(buffer));
            //Wait for feedback from server
            if ((nbytes = recvfrom(client_sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote, &slen)) == -1)
            {
                perror("recvfrom()\n");
                exit(1);
            }
            printf("%s\n", buffer);
        }

        //Delete waits for feedback
        else if(strcmp(cptr, "delete") == 0){
            bzero(buffer, sizeof(buffer));
            if ((nbytes = recvfrom(client_sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote, &slen)) == -1)
            {
                perror("recvfrom()\n");
                exit(1);
            }
            printf("%s\n", buffer);
        }

        //ls command waits for list of files
        else if(strcmp(cptr, "ls") == 0){
            bzero(buffer, sizeof(buffer));
            if ((nbytes = recvfrom(client_sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote, &slen)) == -1)
            {
                perror("recvfrom()\n");
                exit(1);
            }
            printf("%s\n", buffer);
        }

        //Exit waits for message from server that is has exited
        else if(strcmp(cptr, "exit") == 0){
            bzero(buffer,sizeof(buffer));
            if ((nbytes = recvfrom(client_sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote, &slen)) == -1)
            {
                perror("recvfrom()\n");
                exit(1);
            }
            printf("%s\n", buffer);
        }

        //Feedback from server if Invalid command
        else{
            bzero(buffer,sizeof(buffer));
            if ((nbytes = recvfrom(client_sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote, &slen)) == -1)
            {
                perror("recvfrom()");
                exit(1);
            }
            printf("%s\n", buffer);
        }


        bzero(buffer,sizeof(buffer));
    }
    //Close socket
    close(client_sock);
    return 0;
}
