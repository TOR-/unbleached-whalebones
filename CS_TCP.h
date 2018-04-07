#ifndef CS_TCP_H
#define CS_TCP_H

#define SOCKET int
#define INVALID_SOCKET -1

/*  Function to create a socket for use with TCP and IPv4.
    It first starts the WSA system if this has not been done already.  */
SOCKET TCPSocket (int address_family);

/*  Function to set up a server for use with TCP and IPv4.
    Arguments are an existing socket to use, and a port number.
    It first defines a service, in a struct of type sockaddr_in.
    Then it binds the socket to this service.   */
int TCPserverSetup (SOCKET lSocket, int port);

/*  Function to use in a server, to wait for a connection request.
    It will then accept the request, making a connection with the client.
    It reports the IP address and port number used by the client.
    Argument is an existing socket, bound to the required port.
    Return value is the socket used for the connection.  */
SOCKET TCPserverConnect (SOCKET lSocket);

/*  Function to use in a client, to request a connection to a server.
    The IP address is given as a string, in dotted decimal form.
    The port number is given as an integer, and converted to
    network form in this function. */
int TCPclientConnect (
            SOCKET cSocket,     // socket to use to make the connection
            char * IPaddrStr,   // IP address of the server as a string
            int port);          // port number for connection

/*  Function to close a socket.
    After closing the last socket, it tidies up the WSA system. */
int TCPcloseSocket (SOCKET socket2close);

/*  Function to find an IP address from the name of a server.  If the
    server has more than one address, only the first is returned.
    The WSA system must be initialised already, so call TCPcreateSocket()
    before calling this function.
    Arguments:  pointer to a string containing the name of the server (host);
    pointer to an in_addr struct to hold the IP address (or NULL if not needed);
    pointer to a string to hold the IP address in dotted decimal form.
    Return value is 0 for success, negative for error.  */
int getIPaddress(char * hostName, struct in_addr * IPaddr, char * IPaddrStr);

/* Function to print error messages when something goes wrong...  */
void printError(void);

#endif // CS_TCP_H
