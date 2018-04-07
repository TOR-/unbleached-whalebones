/*  EEEN20060 Communication Systems
    Simple TCP client program, to demonstrate the basic concepts,
    using IP version 4.

    It asks for details of the server, and tries to connect.
    Then it gets a request string from the user, and sends this
    to the server.  Then it waits for a response from the server,
    possibly quite long.  This continues until the string ###
    is found in the server response, or the server closes the
    connection.  Then the client program tidies up and exits.

    This program is not robust - if a problem occurs, it just
    tidies up and exits, with no attempt to fix the problem.  */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "CS_TCP.h"

#define MAXREQUEST 52    // maximum size of request, in bytes
#define MAXRESPONSE 70   // size of response array, in bytes

int main()
{
    // Create variables needed by this function
    SOCKET clientSocket = INVALID_SOCKET;  // identifier for the client socket
    int retVal;                 // return value from various functions
    int numRx;                  // number of bytes received
    int reqLen;                 // request string length
    int stop = 0;               // flag to control the loop
    char * loc = NULL;          // location of string
    char serverIPstr[20];       // IP address of server as a string
    int serverPort;             // server port number
    char request[MAXREQUEST];   // array to hold request from user
    char response[MAXRESPONSE+1]; // array to hold response from server
    char endMark[] = "###";     // string used as end marker in response

    // Print starting message
    printf("\nCommunication Systems client program\n\n");


// ============== CONNECT TO SERVER =============================================

    clientSocket = TCPSocket(AF_INET);  // initialise and create a socket
    if (clientSocket == INVALID_SOCKET)  // check for error
        return EXIT_FAILURE;       // no point in continuing

    // Get the details of the server from the user
    printf("Enter the IP address of the server: ");
    scanf("%20s", serverIPstr);  // get IP address as string

    printf("Enter the port number on which the server is listening: ");
    scanf("%d", &serverPort);     // get port number as integer
    fgets(request, MAXREQUEST, stdin);  // clear the input buffer

    // Now connect to the server
    retVal = TCPclientConnect(clientSocket, serverIPstr, serverPort);
    if (retVal < 0)  // failed to connect
        stop = 1;    // set the flag so skip to tidy up section


// ============== SEND REQUEST ======================================

    if (stop == 0)      // if we are connected
    {
        // Get user request and send it to the server
        printf("\nEnter request (max %d bytes): ", MAXREQUEST-2);
        fgets(request, MAXREQUEST, stdin);  // read in the request string
        /* the fgets() function reads characters until the enter key is
           pressed or the limit is reached.  If enter is pressed, the
           newline (\n) character is included in the string.  The null
           character is also included to mark the end of the string.
           MAXREQUEST must leave room for both of these, so the user is
           told not to enter more than MAXREQUEST-2 characters.  */

        reqLen = strlen(request);  // find the length of the request

        // send() arguments: socket identifier, array of bytes to send,
        // number of bytes to send, and last argument of 0.
        retVal = send(clientSocket, request, reqLen, 0);  // send bytes
        // retVal will be number of bytes sent, or an error indicator

        if( retVal == -1) // check for error
        {
			fprintf(stderr, "client: %s\n", gai_strerror(errno));
            stop = 1;       // set the flag to skip to tidy up
        }
        else printf("Sent %d bytes, waiting for reply...\n", retVal);
    }

// ============== RECEIVE RESPONSE ======================================

    // Loop to receive the entire response - it could be long!
    // This loop ends when the end marker is found in the response,
    // or when the server closes the connection, or when an error occurs.
    while (stop == 0)
    {
        // Wait to receive bytes from the server, using the recv function
        // recv() arguments: socket identifier, array to hold received bytes,
        // maximum number of bytes to receive, last argument 0.
        // Normally, recv() will not return until it receives at least one byte...
        numRx = recv(clientSocket, response, MAXRESPONSE, 0);
        // numRx will be number of bytes received, or an error indicator

        if( numRx == -1)  // check for error
        {
			fprintf(stderr, "client: %s\n", gai_strerror(errno));
            stop = 1;  // set flag to exit the loop
        }
        else if (numRx == 0)  // connection closed
        {
            printf("Connection closed by server\n");
            stop = 1;
        }
        else if (numRx > 0)  // we got some data from the server
        {
            // Print whatever has been received
            response[numRx] = 0; // convert the response array to a string
            // The array was made larger to leave room for this extra byte.
            printf("\nReceived %d bytes from the server:\n|%s|\n", numRx, response);

            // Check if the end marker string has been received
            loc = strstr(response, endMark);  // returns pointer to end marker if found
            if (loc != NULL)
            {
                printf("End marker found in string\n");
                stop = 1;
            }
        } // end of if (numRx > 0)

    }   // end of while loop - repeat if data received but end marker not found


// ============== TIDY UP AND END ======================================

    /* A better client might loop to allow another request from the client.
       This simple client just stops after receiving the full response
       has been received from the server. */

    printf("\nClient is closing the connection...\n");

    // Close the socket and shut down the WSA system
    TCPcloseSocket(clientSocket);
    return 0;
}
