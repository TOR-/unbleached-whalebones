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

static int send_status(Status_code,  SOCKET);
static void end_connection(int);

static int gift_server(char * request, long int data_length, char * filepath,  SOCKET connectSocket);
static int weasel_server(Request reqRx, Header headerRx, SOCKET connectSocket);
static int list_server(Request reqRx, SOCKET connectSocket);

int main()
{
	int retVal;         		// return value from various functions //Count variable
	int index = 0;      		// interfunction index reference point
	int numRx = 0;      		// number of bytes received
	char request[MAXREQUEST];   // array to hold request from client
	verbose = true;				//For debugging purposes
	
	Header headerRx;
	Request reqRx;

	SOCKET listenSocket = INVALID_SOCKET;  // identifier for listening socket
	// ============== LISTENSOCKET SETUP ===========================================

	listenSocket = TCPSocket(AF_INET);		// initialise and create a socket
	if (listenSocket == INVALID_SOCKET)		// check for error
	{	
		if(verbose) printf("main: Error initialising listen socket. Exiting...\n");
		return EXIT_FAILURE;
	}

	// Set up the server to use this socket and the specified port
	retVal = TCPserverSetup(listenSocket, SERVER_PORT);
	if (retVal != EXIT_SUCCESS) 			// check for error
	{
		TCPcloseSocket(listenSocket);
		return retVal;   	// End session with err code EXIT_FAILURE
	}
	/*Server listens until a request to create a connection is received.
	  Once server obtains a request, the fidelity of the request command
	  and header values and arguments are tested. If an invalid value is
	  found, server closes connection and breaks from nested loop.
	  Server proceeds to listen again.*/

	do{ 
		do{
			//Re-init for each session
			index = 0;
			numRx = 0;
			
			SOCKET connectSocket = INVALID_SOCKET; // identifier for connection socket

		// ============== WAIT FOR CONNECTION ====================================

			// Listen for a client to try to connect, and accept the connection
			connectSocket = TCPserverConnect(listenSocket);
			if (connectSocket == INVALID_SOCKET)  // check for error
			{
				if(verbose) printf("main: Invalid connect socket. Exiting...\n");
				
				break;   // set the flag to prevent other things happening
			}

		// ============== RECEIVE REQUEST ======================================

			// Re-init struct members to store details of request
			headerRx.data_length = 0;
			headerRx.timeout = 0;
			reqRx.cmdRx = NUM_MODE;
			reqRx.filepath = NULL;
			reqRx.header = &headerRx;

			//Read in first chunk of request
			numRx = recv(connectSocket, request, MAXREQUEST, 0);

			if( numRx < 0)  // check for error
			{
				if(verbose) printf("Problem receiving, maybe connection closed by client?\n%s\n", strerror(errno));
				break;   // break from loop
			}
			else if (0 == numRx)  // indicates connection closing (but not an error)
			{
				if(verbose) printf("Connection closed by client\n");
				break;   // break from loop
			}
			else // request received
			{
				if(verbose) printf("main: REQ received\n");
				request[numRx] = 0;  // add 0 byte to make request into a string
				// Print details of the request
				if(verbose) printf("\nRequest received, %d bytes: \'%s\'\n", numRx, request);
				
				/*================================================
				================ PARSE REQUEST ===================
				================================================*/

				// Index tracks the position within the initial request buffer
				//Parse command returns 'true' for a valid command and false
				//for invalid. Invalid cmd ends session with client
				if(!parse_command(request, &(reqRx.cmdRx), &index))
				{
					if(verbose)
					{
						printf("main: Invalid command\n");
						printf("main: Err number >>%d<<\n", S_COMMAND_NOT_RECOGNISED);
					}
					
					if(!send_status(S_COMMAND_NOT_RECOGNISED, connectSocket)) 
						if(verbose) printf("main: Error status sent\n");
					
					if(verbose) printf("main: Terminating connection\n");
					end_connection(connectSocket);
					break;
				}

				// Extracts filepath. Not tested within this function.
				parse_filepath(request, &(reqRx.filepath), &index);

				// Parse header extracts all headers and args and returns 0
				// if everything goes well. Err code otherwise.
				if(parse_header(request, &headerRx, &index) > 1){ 
					if(verbose) printf("main: Bad header\n");
					if(!send_status( retVal, connectSocket)) printf("main: Error %d. Status sent\n", retVal);
					if(verbose) printf("main: Terminating connection\n");
					end_connection(connectSocket);
					break;
				}

				if(verbose)
				{
					printf("\nCommand:%s\n", mode_strs[reqRx.cmdRx]);
					printf("Filepath:%s\n", reqRx.filepath);
					printf("Data-length:%ld\n", headerRx.data_length);
					printf("Timeout:%ld\n\n", headerRx.timeout);
				} 

				switch(reqRx.cmdRx)
				{
					case GIFT:
						if(!gift_server((request + index), headerRx.data_length, reqRx.filepath, connectSocket))
						{	//If gift server exits with EXIT_SUCCESS, all good.
							send_status(S_COMMAND_RECOGNISED, connectSocket);
							if(verbose) printf("main: File written successfully\n");
						}
						else
						{
							// There has been an error writing the file
							send_status(S_SERVER_WRITE_ERROR, connectSocket);
							if(verbose) printf("main: Error writing file\n");
						}
						break;
					case WEASEL:
                        if(!weasel_server(reqRx, headerRx, connectSocket)) 
						{	//Illegal filepath
							if(verbose) printf("\nmain: Cannot access filepath specified\n");
							//send_status(S_ILLEGAL_FILE_PATH, connectSocket); Is status code sent from within weasel?
						}
						break;
					case LIST:
						if(!list_server(reqRx, connectSocket)) if(verbose) printf("\nmain: Contents of directory sent to client\n");
						break;
					case NUM_MODE:	//Never reaches this as cmd has already been tested, but gets rid of warning from gcc
						break;
				}
				end_connection(connectSocket);
			
			}
		}while(true);
	}while(true);
		
	return 0;
}

									
int weasel_server(Request reqRx, Header headerRx, SOCKET connectSocket)
{
    char filename[100];
    sprintf(filename, "Server_Files/%s", reqRx.filepath);

	send_status(100, connectSocket);

	return send_data(connectSocket, filename);
}

int gift_server(char * buf, long int data_length, char * filepath, SOCKET connectSocket)
{
	char filename [1000];

	sprintf(filename, "Server_Files/%s", filepath);

	if( read_data( buf, PRINT, filepath, data_length, connectSocket) == EXIT_FAILURE )
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
	int retVal = 0;
	const char * server_root = "Server_Files/";
	strncpy(dir_name, server_root, strlen(server_root) + 1);
	if(strcmp(reqRx.filepath, ".")) // if subdirectory required append to file path
	{   
		strcat(dir_name, reqRx.filepath);
		dir_name[strlen(dir_name)-1] = '\0'; //remove extra SPACE character
	}

	dp = opendir(dir_name); // open directory
	//include length for joe
	if(dp != NULL)
	{
		int index = 0;
		while((ep = readdir(dp)))
		{
			if (ep->d_name[0] != '.')
			{ 
				char_count += strlen(ep->d_name) + 1;
				response = (char *)realloc(response, char_count);
				index += sprintf(response + index, "%s\n", ep->d_name);
			}
		}
		closedir(dp);
	}
	else
	{
		fprintf(stderr, "%s:can't open the directory %s: %s\n",
				__FUNCTION__, dir_name, strerror(errno));
		send_status(S_FILE_NOT_FOUND, connectSocket); // define new error
	}

	printf("REMOVE ONCE CHECKED THAT char_count == strlen(response) (%d %s %d)\n",
			char_count, char_count == strlen(response)? "==":"!=", strlen(response));
	char * headers, length[LONG_MAX_DIGITS +1];
	snprintf(length, LONG_MAX_DIGITS, "%ld", strlen(response)); 
	//append_header(&headers, header_name[DATA_LENGTH], length);
	//finish_headers(&headers);
	headers = malloc(strlen(header_name[DATA_LENGTH]) + 1 + LONG_MAX_DIGITS + 2 + 1);
	sprintf(headers, "%s:%s\n\n", header_name[DATA_LENGTH], length);
	char *res = (char *)malloc(strlen(headers) + strlen(response) + 1);
	sprintf(res, "%s%s", headers, response);

	if(verbose) printf("response array = |%s|\n", res);

	send_status(S_COMMAND_SUCCESSFUL, connectSocket);
	retVal = send(connectSocket, res, strlen(res), 0);  // send bytes

	if( retVal == -1)  // check for error
	{
		printf("LIST: Error sending response\n%s\n", strerror(errno));
		free(res);
	}
	else if(verbose) printf("Sent directory list message of %d bytes\n", retVal);
	free(res);
	return 0;
}

int send_status(Status_code status, SOCKET connectSocket)
{
	int str_size;
	//hold buffer to send status to client
	char * buff;

	str_size = strlen((const char *)status_descriptions[status - 1]);
	//+2 to store '\0' and '\n'
	buff = (char *)malloc((str_size + 1)*sizeof(char));
	
	strcpy(buff, (char *)status_descriptions[status - 1]);
	buff[str_size] = '\n';

	if(send(connectSocket, buff, str_size + 2, 0) == -1)
	{
		printf("send_status: Error using send()\n");
		return 1;
	}

	return 0;
}
void end_connection(int connectSocket)
{
	printf("\nServer is closing the connection...\n");
	
	// Close the connection socket
	TCPcloseSocket(connectSocket);
}
