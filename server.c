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
#include "application.h"
#include "application.c"

#include "CS_TCP.h"
#include "CS_TCP.c"

#define SERVER_PORT 6666  // port to be used by the server
#define MAXREQUEST 64      // size of request array, in bytes
#define MAXRESPONSE 90     // size of response array (at least 35 bytes more)
#define ENDMARK 10         // the newline character
#define NULLBYTE '\0'

typedef struct head{ // Should members be character types?? Change before/after?
    char *length;
    char *type;
    char *timeout;
    char *date;
    char *ifexist;
    //char *last_mod;
    //char *enc;
} Header;

typedef struct req{
    enum command;
    char *filename;
    Header *header;
} Request;

int parse_request(Request *reqRx, Header *headerRx, char *request);

int main()
{
    // Create variables needed by this function
    // The server uses 2 sockets - one to listen for connection requests,
    // the other to make a connection with the client.
    SOCKET listenSocket = INVALID_SOCKET;  // identifier for listening socket
    SOCKET connectSocket = INVALID_SOCKET; // identifier for connection socket
    int retVal;         // return value from various functions
    int numRx = 0;      // number of bytes received
    int numResp;        // number of bytes in response string
    int stop = 0;       // flag to control the loop
    char * loc = NULL;          // location of character
    char request[MAXREQUEST+1];   // array to hold request from client
    char response[MAXRESPONSE+1]; // array to hold our response
    char welcome[] = "Welcome to the Communication Systems server.";
    char goodbye[] = "Goodbye, and thank you for using the server. ###";

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
    
    Request reqRx; // 'request received', change if you want?
    Header headerRx;
    reqRx.header = &headerRx; //req member now points to header structure
    
    // Loop to receive data from the client, until the end marker is found
    while (!stop)   // loop is controlled by the stop flag
    {
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
        else if (numRx == 0)  // indicates connection closing (but not an error)
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
            
            //function to parse request[] and store values in structure
            if( parse_request(&reqRx, &headerRx, request) != 0 )
                fprintf(stderr, "Server: Unable to parse request received!");
            
            //function to process requests
            //...
            /*Search request and see how server should respond.*/

            // Check to see if the request contains the end marker
            loc = memchr(request, ENDMARK, numRx);  // search the array
            if (loc != NULL)  // end marker was found
            {
                printf("Request contains end marker\n");
                stop = 1;   // set the flag to end the loop
            }

        } // end of if data received

    } // end of while loop


// ============== SEND RESPONSE ======================================

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


int parse_request(Request *reqRx, Header *headerRx, char *request){

    int index = 0; // current location within request[]
    int char_count = 0; // counts length of current substring
    int i = 0; //general loop counter
    int end = 0; //flags end of header
    char *read_string;
    //retrieve request 'command'===========================================
    while(request[index++] != ' ')
        char_count++;
    
    //Allocate memory for command and leave space for NULLBYTE
    read_string = (char *)malloc((char_count + 1)*sizeof(char));
    
    //Store command as string
    for(i=0; i<char_count; i++)
        read_string[i] = request[i];
   //Append null byte
    read_string[char_count] = NULLBYTE;
        
    for(char_count = 0; request[index++] != '\n'; char_count++);
    //Allocate memory for filename
    reqRx->filename = (char *)malloc(char_count*sizeof(char));
    
    //Store filename as string
    for(i=0; i<char_count; i++)
        (reqRx->filename)[i] = request[i];
    //Append null byte
    (reqRx->filename)[char_count] = NULLBYTE;
    
    //Unnecessary. Will be reset upon next function call.
    char_count = 0; // reset to be used for next retrieval
    
    /* could have array to store all member names so this section could be put
    into a larger loop, may be too complicated but it'd be slick */
    
    //retrieve header components===========================================
    char current_string[20]; // stores <header name>
    
    while(!end){
        
        for(i=0; request[index] != ':'; i++)
            current_string[i] = request[index++];
            
        current_string[++i] = '\0';
        //can't use switch statement for strings :( any other way?
        
        if(strcmp(current_string, "Data-length")){
            
            while(request[index++] != '\n')
                char_count++;
            
            headerRx->length = (char *)malloc(char_count*sizeof(char));
            
            for(i=0; i<char_count; i++)
                (headerRx->length)[i] = request[(index-char_count)+i];
            
            char_count = 0; // reset to be used for next retrieval
        }
        //copy for other <header names>
        //
        //
        //
        //
        //
        //
        //
        //
        

        if(request[index] == '\n')
            end = 1;
    }
        
    return 0;
}



