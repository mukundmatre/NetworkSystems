	Mukund Madhusudan Atre - PROGRAMMING ASSIGNMENT 1
	DATE: 24 September, 2017



Folder Structure:
clientFolder
	Files: -client.c -Makefile
serverFolder
	Folder: -server.c -Makefile
README.txt


Instructions for Execution:
1. "make" the respective .c files for executables
2. "make clean" to erase the executables
3. ifconfig can be used for getting the server IP or "localhost" will work at the local machine server.


Client/user Commands/inputs
1. get <filename>
This asks the server for a file and if it is not present, it simply sends an error.
2. put <filename>
This puts the file on the server if it is present on the client, else it sends and error message that tells the server not to create a file.
3. ls
This commands requests the server to list all the files in the server directory. If there is error in opening this directory at server, the server simply sends an error message to the client.
4. exit
This Command request the server to close the socket i.e. close the connection.
5.For other commands
The server and the client throw error message as unsupported command.



Features:
Implemented Reliable data and acknowledgement transmission.
Implemented Data Encryption and Decryption both at client and server.

Assumptions:
There should be no spaces in file names.
Assuming that the file to be put or get does not already exist at the destination.
