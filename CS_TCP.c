/* Functions for use in TCP communications.  */

#include <errno.h>
#include <stdio.h>      // needed for printf
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <time.h>       // for timeval
#include "CS_TCP.h"

// Shared variables for all functions in this file
static int socketCount = 0;  // number of sockets created

// ======================================================================

/*  Function to create a socket for use with TCP and IPv4. */
SOCKET TCPSocket (int address_family)
{
    SOCKET mySocket = INVALID_SOCKET;  // identifier for socket
    int retVal = 0;     // return value from function

    // Now create the socket as requested.
    // SOCK_STREAM means the socket works with streams of bytes,
    // IPPROTO_TCP means TCP transport protocol.
    mySocket = socket(address_family, SOCK_STREAM, IPPROTO_TCP);

    if (mySocket == INVALID_SOCKET)  // check for error
    {
        printf("*** Failed to create socket\n");
        printf("TCPcreateSocket: %s\n", strerror(retVal));
        return INVALID_SOCKET;  // socket is invalid if error
    }
    else
    {
        printf("Socket created\n");
        socketCount++;    // increment the socket counter
        return mySocket;  // return the socket identifier
    }

}  // end of TCPcreateSocket


// ======================================================================

/*  Function to set up a server for use with TCP and IPv4.
    Arguments are an existing socket to use, and a port number.
    It first defines a service, in a struct of type sockaddr_in.
    Then it binds the socket to this service.   */
int TCPserverSetup (SOCKET lSocket, int port)
{
    int retVal;     // for return values from functions;
    struct sockaddr_in service;  // IP address and port structure

    // Configure the service structure with the service to be offered
    service.sin_family = AF_INET;  // specify IP version 4 family
    service.sin_addr.s_addr = htonl(INADDR_ANY);  // set IP address
    // function htonl() converts 32-bit integer to network format
    // INADDR_ANY means accept a connection on any IP address
    service.sin_port = htons(port);  // set port number on which to listen
    // function htons() converts a 16-bit integer to network format

    // Bind the socket to this service (IP address and port)
    retVal = bind(lSocket, (const struct sockaddr *) &service, sizeof(service));
    if( retVal == -1)  // check for error
    {
        fprintf(stderr,"TCPserverSetup: %s\n", strerror(errno));
        return EXIT_FAILURE;          // indicate failure
    }
    else printf("Socket bound to port %d, server is ready\n", port);

    return 0;

}  // end of TCPserverSetup


// ======================================================================

/*  Function to use in a server, to wait for a connection request.
    It will then accept the request, making a connection with the client.
    It reports the IP address and port number used by the client.
    Argument is an existing socket, bound to the required port.
    Return value is the socket used for the connection.  */
SOCKET TCPserverConnect (SOCKET lSocket)
{
    SOCKET cSocket = INVALID_SOCKET;  // socket to use for connection
    struct sockaddr_in client;  // structure to identify the client
    int len = sizeof(client);   // initial length of structure
    int clientPort;             // port number being used by client
    struct in_addr clientIP;    // structure to hold client IP address
    int retVal;     // for return values from functions;

    // Set this socket to listen for connection requests.
    // Second argument is maximum number of requests to allow in queue
    retVal = listen(lSocket, 2);
    if( retVal == -1)  // check for error
    {
        printf("TCPserverConnect: %s\n", strerror(errno));
        return EXIT_FAILURE;  // indicate failure
    }
    else printf("\nListening for connection requests...\n");

    // Wait until a connection is requested, then accept the connection.
    cSocket = accept(lSocket, (struct sockaddr *) &client, &len );
    if( cSocket == INVALID_SOCKET)  // check for error
    {
        printf("TCPserverConnect: %s\n", strerror(errno));
        return EXIT_FAILURE;  // indicate failure
    }
    else  // we have a connection, print details of the client
    {
        clientPort = client.sin_port;  // get client port number
        clientIP = client.sin_addr;    // get client IP address
        printf("\nAccepted connection from port %d on %s\n",
               ntohs(clientPort), inet_ntoa(clientIP));
        // function ntohs() converts 16-bit integer from network form to normal
        // function inet_ntoa() converts IP address structure to a string
        socketCount++;    // increment the active socket counter
        return cSocket;   // return the socket identifier
    }

}  // end of TCPserverConnect


// ======================================================================

/*  Function to use in a client, to request a connection to a server.
    The IP address is given as a string, in dotted decimal form.
    The port number is given as an integer, and converted to
    network form in this function. */
int TCPclientConnect (
            SOCKET cSocket,     // socket to use to make the connection
            char * IPaddrStr,   // IP address of the server as a string
            int port)           // port number for connection
{
    int retVal;     // for return value of function
    unsigned long IPaddr;   // IP address as a 32-bit integer

    // Get the IP address in integer form
    IPaddr = inet_addr(IPaddrStr); // convert string to integer
    /* inet_addr() converts an IP address string (dotted decimal)
    to a 32-bit integer address in the required form.  */

    // Build a structure to identify the service required
    // This has to contain the IP address and port of the server
    struct sockaddr_in service;  // IP address and port structure
    service.sin_family = AF_INET;       // specify IP version 4 family
    service.sin_addr.s_addr = IPaddr;   // set IP address
    service.sin_port = htons(port);     // set port number
    // function htons() converts 16-bit integer to network format

    // Try to connect to the service required
    printf("Trying to connect to %s on port %d\n", IPaddrStr, port);
    retVal = connect(cSocket, (const struct sockaddr *) &service, sizeof(service));
    if( retVal != 0)  // check for error
    {
        printf("TCPclientConnect: %s\n", strerror(errno));
        return EXIT_FAILURE;  // indicate failure
    }
    else
    {
        return EXIT_SUCCESS;
    }

}  // end of TCPclientConnect


// ======================================================================

/*  Function to close a socket.	*/
int TCPcloseSocket (SOCKET socket2close)
{
    int retVal;     // for return values of functions

    // First block sending, also signals disconnection to the other end
    retVal = shutdown(socket2close, SHUT_RDWR);  // try to shut down sending
    if (retVal == 0) printf("Socket shutting down...\n");  // success
    else  // some problem occurred
    {
        printf("TCPserverConnect: %s\n", strerror(errno));
        return EXIT_FAILURE;  // indicate failure
    }

    // It is now safe to close the socket
    retVal = close(socket2close);  // close the socket
    if( retVal != 0)  // check for error
    {
        printf("TCPserverConnect: %s\n", strerror(errno));
        return EXIT_FAILURE;  // indicate failure
    }
    else
    {
        socketCount--;  // decrement the socket counter
        printf("Socket closed, %d remaining\n", socketCount);
		return EXIT_SUCCESS;
    }
}  // end of TCPcloseSocket


// ======================================================================

/*  Function to find an IP address from the name of a server.  If the
    server has more than one address, only the first is returned.
    The WSA system must be initialised already, so call TCPcreateSocket()
    before calling this function.
    Arguments:  pointer to a string containing the name of the server (host);
    pointer to an in_addr struct to hold the IP address (or NULL if not needed);
    pointer to a string to hold the IP address in dotted decimal form.
    Return value is 0 for success, negative for error.  */
int getIPaddress(char * hostName, struct in_addr * IPaddr, char * IPaddrStr)
{
    int retVal;     // for return value of function
    struct addrinfo hints;  // struct of type addrinfo
    struct addrinfo * result = NULL;  // pointer to struct of type addrinfo
    struct sockaddr_in * hostAddrPtr; // pointer to address structure
    struct in_addr hostAddr;          // struct to hold an IP address

    // Initialise the hints structure - all zero
    // ZeroMemory( &hints, sizeof(hints) );
	memset(&hints, 0, sizeof(hints));

    // Fill in some parts, to specify what type of address we want
    hints.ai_family = AF_INET;        // IPv4 address
    hints.ai_socktype = SOCK_STREAM;  // stream type socket
    hints.ai_protocol = IPPROTO_TCP;  // using TCP protocol

    // Try to find the address of the host computer
    retVal = getaddrinfo(hostName, NULL, &hints, &result);
    if (retVal != 0)  // attempt has failed
    {
        printf("Failed to get the IP address, error code %d\n", retVal);
        printf("getIPaddress: %s\n", gai_strerror(retVal));
        return EXIT_FAILURE;
    }
    else  // success

    // One or more result structures have been created by getaddrinfo(),
    // forming a linked list - a computer could have multiple addesses.
    // The result pointer points to the first structure, and this
    // example only uses the first address found.

    // The result structure has many elements - we only want the address
    hostAddrPtr = (struct sockaddr_in*) result->ai_addr; // get pointer to address

    // The address structure has many element - we only want the IP address
    hostAddr = hostAddrPtr->sin_addr;  // get IP address in network form

    // Provide this value if the user wants it
    if (IPaddr != NULL) *IPaddr = hostAddr;  // IP address as struct

    // Convert the IP address to a string (dotted decimal)
    IPaddrStr = inet_ntoa(hostAddr);

    // Free the memory that getaddrinfo allocated for result structure(s)
    freeaddrinfo(result);

    printf("Host %s has IP address %s\n", hostName, IPaddrStr);
    return 0;   // return success

}  // end of getIPaddress

