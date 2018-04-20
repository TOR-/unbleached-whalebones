/*  EEEN20060 Communication Systems
    Simple TCP server program, to demonstrate the basic concepts,
    using IP version 4.

    It listens on a specified port until a client connects.
    Then it waits for a request from the client, and keeps trying
    to receive bytes until it finds the end marker.
    Then it sends a long response to the client, which includes
    the last part of the request received.
    Then it tidies up and stops.

    This program is not robust - if a problem occurs, it just
    tidies up and exits, with no attempt to fix the problem.  */

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

#include "application.h"
#include "CS_TCP.h"

#define SERVER_PORT 6666  // port to be used by the server
#define MAXREQUEST 300      // size of request array, in bytes
#define MAXRESPONSE 90     // size of response array (at least 35 bytes more)
#define ENDMARK 10         // the newline character

static int send_status(Status_code,  SOCKET);
int parse_request(Request * , Header * , char * );
//char * check_parse_error(int error, char * err_msg);

//char * check_parse_error(int error, char * err_msg)
// request processign functions:
 
//int send_error_response(int status_code, SOCKET connectSocket);
//int gift_server(Request reqRx, Header headerRx, SOCKET connectSocket);
//int weasel_server(Request reqRx, Header headerRx, SOCKET connectSocket;
//int list_server(Request reqRx, Header headerRx, SOCKET connectSocket);

int main()
{
    // Create variables needed by this function
    // The server uses 2 sockets - one to listen for connection requests,
    // the other to make a connection with the client.
    SOCKET listenSocket = INVALID_SOCKET;  // identifier for listening socket
    SOCKET connectSocket = INVALID_SOCKET; // identifier for connection socket
    int retVal;         // return value from various functions
    int index = 0;      // interfunction index reference point
    int numRx = 0;      // number of bytes received
    int numResp;        // number of bytes in response string
    int stop = 0;       // flag to control the loop
    char request[MAXREQUEST+1];   // array to hold request from client
    char response[MAXRESPONSE+1]; // array to hold our response
    char welcome[] = "Welcome to the server\n";
    char goodbye[] = "Welcome to the server\n";
    Header headerRx;
    Request reqRx;
    
// ============== SERVER SETUP ===========================================

    listenSocket = TCPSocket(AF_INET);  // initialise and create a socket
    if (listenSocket == INVALID_SOCKET)  // check for error
        return 1;       // no point in continuing

    // Set up the server to use this socket and the specified port
    retVal = TCPserverSetup(listenSocket, SERVER_PORT);
    if (retVal < 0) // check for error
        stop = 1;   // set the flag to prevent other things happening

// ============== WAIT FOR CONNECTION ====================================

    // Listen for a client to try to connect, and accept the connection
    connectSocket = TCPserverConnect(listenSocket);
    if (connectSocket == INVALID_SOCKET)  // check for error
        stop = 1;   // set the flag to prevent other things happening


// ============== RECEIVE REQUEST ======================================

    // Loop to receive data from the client, until the end marker is found
    while (!stop)   // loop is controlled by the stop flag
    {
        headerRx.data_length = 0;
        headerRx.timeout = 0;
        reqRx.cmdRx = INVALID;
        reqRx.filepath = NULL;
        reqRx.header = &headerRx;

        //SET REQUEST ARRAY ELEMENTS TO NULL
        // Wait to receive bytes from the client, using the recv function
        // recv() arguments: socket identifier, array to hold received bytes,
        // maximum number of bytes to receive, last argument 0.
        // Normally, this function will not return until it receives at least one byte...
        numRx = recv(connectSocket, request, MAXREQUEST, 0);
        // numRx will be number of bytes received, or an error indicator (negative value)

        if( numRx < 0)  // check for error
        {
            printf("Problem receiving, maybe connection closed by client?\n%s\n", strerror(errno));
            stop = 1;   // set the flag to end the loop
        }
        else if (0 == numRx)  // indicates connection closing (but not an error)
        {
            printf("Connection closed by client\n");
            stop = 1;   // set the flag to end the loop
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
                if(!send_status(S_COMMAND_NOT_RECOGNISED, connectSocket)) printf("main: Error status sent\n");
                break;
            }

            parse_filepath(request, &(reqRx.filepath), &index);
            
            while((retVal = parse_header(request, &headerRx, &index)))
            {    
                if(retVal > 1){ 
                    printf("Parse Req: Error\n");
                    //send status function( retVal);
                    break;
                }
            }
        }
            //================================================
    } // end of while loop
    
    //INDEX STORES INDEX OF START OF DATA
    #ifdef DEBUG
    printf("\nCommand:%d\n", reqRx.cmdRx);
    printf("Filepath:%s\n", reqRx.filepath);
    printf("Data-length:%ld\n", headerRx.data_length);
    printf("Timeout:%ld\n\n", headerRx.timeout);
    #endif  

    //testing function call
    list_server(reqRx, headerRx, connectSocket);

    // If we received a request, then we send a response
    if (numRx > 0)
    {
        // The response to the client is in three parts.
        // First send the welcome message to the client.
        // send() arguments: socket identifier, array of bytes to send,
        // number of bytes to send, and last argument of 0.
        retVal = send(connectSocket, welcome, strlen(welcome), 0);  // send bytes
        // retVal will be the number of bytes sent, or an error indicator

        if( retVal == -1)  // check for error
        {
            printf("*** Error sending response\n%s\n", gai_strerror(errno));
        }
        else printf("Sent welcome message of %d bytes\n", retVal);

        // Build the next part of the response as a string
        // sprintf() works like printf(), but puts the result in a string,
        // the return value is the number of bytes in the string
        numResp = sprintf(response, "Got your request: '%s' with %d bytes",
                           request, numRx);

        // Send it to the client.
        retVal = send(connectSocket, response, numResp, 0);  // send bytes
        if( retVal == -1)  // check for error
        {
            printf("*** Error sending response\n%s\n", gai_strerror(errno));
        }
        else printf("Sent response of %d bytes\n", retVal);

        // Then send the closing message to the client.
        retVal = send(connectSocket, goodbye, strlen(goodbye), 0);  // send bytes
        if( retVal == -1)  // check for error
        {
            printf("*** Error sending response\n%s\n", gai_strerror(errno));
        }
        else printf("Sent closing message of %d bytes\n", retVal);

    }  // end of if we received a request


// ============== TIDY UP AND END ======================================

    /* A better server might loop to deal with another request from the
       client or to wait for another client to connect.  This one stops
       after dealing with one request from one client. */

    printf("\nServer is closing the connection...\n");

    // Close the connection socket first
    TCPcloseSocket(connectSocket);

    // Then close the listening socket
    TCPcloseSocket(listenSocket);
    return 0;
}

                                    
/*
int send_error_response(int status_code, SOCKET connectSocket){
	
	




}
							
int gift_server(Request reqRx, Header headerRx, SOCKET connectSocket){

use recv funvtion with FILE * as argument



}

int weasel_server(Request reqRx, Header headerRx, SOCKET connectSocket){





}

*/
int list_server(Request reqRx, Header headerRx, SOCKET connectSocket)
{

    DIR *dp;
    struct dirent *ep;
    dp = opendir(reqRx.filepath);
    int count = 0;

    if(dp != NULL)
    {
        while(ep = readdir(dp)){
            fprintf(stdout, "%s\n", ep->d_name);
            count += strlen(ep->d_name);
        }
        closedir(dp);
    }
    else
    {
        fprintf(stderr, "Can't open the directory\n");
    }
    return 0;
}

static int send_status(Status_code status, SOCKET connectSocket)
{
    int i, str_size;
    //hold buffer to send status to client
    char * buff;

    str_size = strlen((const char *)status_descriptions[status/100][status%100]);
    //+2 to store '\0' and '\n'
    buff = (const char *)malloc((str_size + 1)*sizeof(char));
    
    strcpy(buff, (char *)status_descriptions[status/100][status%100]);
    buff[str_size] = '\n';

    send(connectSocket, buff, str_size + 2, 0);
    
    return 0;
}