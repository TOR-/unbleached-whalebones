#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdbool.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>

#include "application.h"
#include "CS_TCP.h"

#define SERVER_PORT 6666	// port to be used by the server
#define MAXREQUEST 1024		// size of request array, in bytes
#define MAXRESPONSE 90		// size of response array (at least 35 bytes more)
#define ENDMARK 10			// the newline character

char *create_status(Status_code, SOCKET);
int send_error_status(Status_code,  SOCKET);
int parse_request(Request * , Header * , char * );
void end_connection(int, int);
//char * check_parse_error(int error, char * err_msg);

//char * check_parse_error(int error, char * err_msg)
// request processign functions:

//int send_error_response(int status_code, SOCKET connectSocket);

int gift_server(char * request, long int data_length, char * filepath,  SOCKET connectSocket);
int weasel_server(Request reqRx, Header headerRx, SOCKET connectSocket);
int list_server(Request reqRx, SOCKET connectSocket);

int main()
{
	int retVal;         // return value from various functions //Count variable
	int index = 0;      // interfunction index reference point
	int numRx = 0;      // number of bytes received
	char request[MAXREQUEST];   // array to hold request from client
	verbose = false;
	
	Header headerRx;
	Request reqRx;

	SOCKET listenSocket = INVALID_SOCKET;  // identifier for listening socket
	// ============== LISTENSOCKET SETUP ===========================================

	listenSocket = TCPSocket(AF_INET);  // initialise and create a socket
	if (listenSocket == INVALID_SOCKET)  // check for error
		return 1;       // no point in continuing

	// Set up the server to use this socket and the specified port
	retVal = TCPserverSetup(listenSocket, SERVER_PORT);
	if (retVal < 0) // check for error
		return 1;   // End session with err code 1

	do{ 
		do{
			//Re-init for each session
			index = 0;
			numRx = 0;
			// Create variables needed by this function
			// The server uses 2 sockets - one to listen for connection requests,
			// the other to make a connection with the client.
			
			SOCKET connectSocket = INVALID_SOCKET; // identifier for connection socket

		// ============== WAIT FOR CONNECTION ====================================

			// Listen for a client to try to connect, and accept the connection
			connectSocket = TCPserverConnect(listenSocket);
			if (connectSocket == INVALID_SOCKET)  // check for error
				break;   // set the flag to prevent other things happening


		// ============== RECEIVE REQUEST ======================================

			// Loop to receive data from the client, until the end marker is found
			headerRx.data_length = 0;
			headerRx.timeout = 0;
			reqRx.cmdRx = NUM_MODE;
			reqRx.filepath = NULL;
			reqRx.header = &headerRx;

			//Read in first chunk of request
			numRx = recv(connectSocket, request, MAXREQUEST, 0);
			// numRx will be number of bytes received, or an error indicator (negative value)

			if( numRx < 0)  // check for error
			{
				printf("Problem receiving, maybe connection closed by client?\n%s\n", strerror(errno));
				break;   // set the flag to end the loop
			}
			else if (0 == numRx)  // indicates connection closing (but not an error)
			{
				printf("Connection closed by client\n");
				break;   // set the flag to end the loop
			}
			else // numRx > 0 => we got some data from the client
			{
				printf("Server: REQ received\n");
				request[numRx] = 0;  // add 0 byte to make request into a string
				// Print details of the request
				printf("\nRequest received, %d bytes: \'%s\'\n", numRx, request);
				
				/*================================================
				========Parse the request received================
				================================================*/

				if(!parse_command(request, &(reqRx.cmdRx), &index))
				{
					printf("parse req: Invalid command\n");
					printf("parse req: %d\n", S_COMMAND_NOT_RECOGNISED);
					if(!send_error_status(S_COMMAND_NOT_RECOGNISED, connectSocket)) printf("main: Error status sent\n");
					end_connection(connectSocket, listenSocket);
					break;
				}

				parse_filepath(request, &(reqRx.filepath), &index);

				if(parse_header(request, &headerRx, &index) > 1){ 
					printf("main: Bad header\n");
					if(!send_error_status( retVal, connectSocket)) printf("main: Error %d. Status sent\n", retVal);
					printf("main: Terminating connection\n");
					end_connection(connectSocket, listenSocket);
					break;
				}

#ifdef DEBUG
				printf("\nCommand:%d\n", reqRx.cmdRx);
				printf("Filepath:%s\n", reqRx.filepath);
				printf("Data-length:%ld\n", headerRx.data_length);
				printf("Timeout:%ld\n\n", headerRx.timeout);
#endif 

				switch(reqRx.cmdRx)
				{
					case GIFT:
						printf(">>%c<<\n\n", request[index]);

						if(!gift_server((request + index), headerRx.data_length, reqRx.filepath, connectSocket))
						{
							send_error_status(S_COMMAND_RECOGNISED, connectSocket);
							printf("main: File written successfully\n");
						}
						else
						{
							printf("main: Error writing file\n");
						}
						break;
					case WEASEL:
                        if(!weasel_server(reqRx, headerRx, connectSocket)) printf("\nmain: Cannot access filepath specified\n");
						break;
					case LIST:
						if(!list_server(reqRx, connectSocket)) printf("\nmain: Contents of directory sent to client\n");
						break;
				}
				end_connection(connectSocket, listenSocket);
			
			}//Keep inside
		}while(true);
	}while(true);
		
	return 0;
}

									
int weasel_server(Request reqRx, Header headerRx, SOCKET connectSocket)
{
    char filename[100];
    sprintf(filename, "Server_Files/%s", reqRx.filepath);

	send_error_status(110, connectSocket);

	return send_data(connectSocket, filename);
}

int gift_server(char * buf, long int data_length, char * filepath, SOCKET connectSocket)
{
	char filename [1000];

	sprintf(filename, "Server_Files/%s", filepath);

	if( read_data( buf, WRITE, filename, data_length, connectSocket) == EXIT_FAILURE )
	{
		printf("gift_server: Error reading data\n");
		return EXIT_FAILURE;
	}
	else
	{
		printf("gift_server: All good - file written\n");
		return EXIT_SUCCESS;
	}
}

int list_server(Request reqRx, SOCKET connectSocket)
{

	DIR *dp;
	struct dirent *ep; 
	int char_count = 0; // counts length of list to send
	char dir_name[100]; // large enough to store full file path
	char *response = NULL; // to be realloc memory
	char *new_response = NULL;
	int retVal = 0;

	strcat(dir_name, "Server_Files/");

	if(strcmp(reqRx.filepath, ".")) // if subdirectory required append to file path
	{   
		strcat(dir_name, reqRx.filepath);
		dir_name[strlen(dir_name)] = '\0'; 
	}
	printf("dir_name = %s\n", dir_name);
	dp = opendir(dir_name); // open directory
	
	if(dp != NULL)
	{
		int index = 0;
		while((ep = readdir(dp)))
		{
			if (ep->d_name[0] != '.')
			{ 
				char_count += strlen(ep->d_name) + 1; // account for line feed

				new_response = (char *)realloc(response, char_count);	// realloc assigns new address if segmentation occurs
				if(new_response != NULL)
					response = new_response;

				index += sprintf(response + index, "%s\n", ep->d_name);
			}
		}
		closedir(dp);
		if( response == NULL )
		{
			char_count = 16;
			new_response = (char *)realloc(response, char_count);	// realloc assigns new address if segmentation occurs
				if(new_response != NULL)
					response = new_response;

			sprintf(response, "Empty Directory\n");
		}
	}
	else
	{
		fprintf(stderr, "Can't open the directory\n");
		send_error_status(340, connectSocket); // define new error
	}

	char *dir_list = (char *)malloc(char_count + 20); // 20 provides room for length of header

	sprintf(dir_list, "%sData-length:%d\n\n%s", create_status(110, connectSocket), char_count, response); // CHANGE TO WHAT CLIENT WANTS TO RECEIVE

#ifdef DEBUG
	printf("response array = |%s|\n", dir_list);
#endif

	retVal = send(connectSocket, dir_list, strlen(dir_list), 0);  // send bytes

	if( retVal == -1)  // check for error
	{
		printf("LIST: Error sending response\n%s\n", gai_strerror(errno));
	}
	else printf("Sent directory list message of %d bytes\n", retVal);

	return 0;
}

char * create_status(Status_code status, SOCKET connectSocket)
{
	int str_size;
	//hold buffer to send status to client
	char * buff;

	str_size = strlen((const char *)status_descriptions[status - 1]);
	//+2 to store '\0' and '\n'
	buff = (char *)malloc(3 + 1 + str_size + 1);
	sprintf(buff, "%d %s\n", status, status_descriptions[status]);

	return buff;
}

int send_error_status(Status_code status, SOCKET connectSocket)
{
	int str_size;
	//hold buffer to send status to client
	char * buff;

	str_size = strlen((const char *)status_descriptions[status - 1]);
	//+2 to store '\0' and '\n'
	buff = (char *)malloc(3 + 1 + str_size + 1);
	sprintf(buff, "%d %s\n", status, status_descriptions[status]);

	if(send(connectSocket, buff, strlen(buff), 0) == -1)
	{
		printf("send_status: Error using send()\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void end_connection(int connectSocket, int listenSocket)
{
	printf("\nServer is closing the connection...\n");
	
	// Close the connection socket
	TCPcloseSocket(connectSocket);
}