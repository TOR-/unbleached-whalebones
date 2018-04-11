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

#include "CS_TCP.h"
#include "CS_TCP.c"

#define SERVER_PORT 6666  // port to be used by the server
#define MAXREQUEST 52      // size of request array, in bytes
#define MAXRESPONSE 90     // size of response array (at least 35 bytes more)
#define ENDMARK 10         // the newline character

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

    // Loop to receive data from the client, until the end marker is found
<<<<<<< HEAD:refserver.c
    while (0 == stop)   // loop is controlled by the stop flag
=======
    while (!stop)   // loop is controlled by the stop flag
>>>>>>> status:server.c
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
