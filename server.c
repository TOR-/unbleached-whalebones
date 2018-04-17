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
<<<<<<< HEAD
#include <sys/types.h>
#include <dirent.h>
=======
#include "application.h"
>>>>>>> 3655ab7e5023da9bcb5b6d3f10c2b3b6a96e35fe

#include "application.h"
#include "CS_TCP.h"

#define SERVER_PORT 6666  // port to be used by the server
#define MAXREQUEST 64      // size of request array, in bytes
#define MAXRESPONSE 90     // size of response array (at least 35 bytes more)
#define ENDMARK 10         // the newline character

int parse_request(Request *reqRx, Header *headerRx, char *request);
<<<<<<< HEAD
=======
//char * check_parse_error(int error, char * err_msg);

//char * check_parse_error(int error, char * err_msg)
/* request processign functions:
>>>>>>> 3655ab7e5023da9bcb5b6d3f10c2b3b6a96e35fe
 
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
    int numRx = 0;      // number of bytes received
    int numResp;        // number of bytes in response string
    int stop = 0;       // flag to control the loop
    char request[MAXREQUEST+1];   // array to hold request from client
    char response[MAXRESPONSE+1]; // array to hold our response
    char welcome[] = "Welcome to the server\n";
    char goodbye[] = "Welcome to the server\n";

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
    
    Request reqRx;
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
            
            //function to parse request[] and store values in structure
            if((retVal = parse_request(&reqRx, &headerRx, request)) == 0)
                printf("All good\n");
            else{
                printf("Error: Invalid error returned by parse_request\n");
            }
<<<<<<< HEAD
        }
        */
       
    } // end of while loop
    //numRx = recv(connectSocket, request, MAXREQUEST, 0);

    //retVal = list(reqRx, headerRx, connectSocket);
=======
>>>>>>> 3655ab7e5023da9bcb5b6d3f10c2b3b6a96e35fe


        } 
    }
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

/*Takes a request, parses the data within and stores the data within
in useful formats within a struct for processing*/
int parse_request(Request *reqRx, Header *headerRx, char *request)
{
    int retVal;

    if(!parse_command(request, &(reqRx->cmdRx)))
    {
        perror("parse req: Invalid command\n");
        return 0;
    }
    //Switch case for all problems
    parse_filepath(request, (reqRx->filepath));

    while(!(retVal = parse_header(request, headerRx)))
        if(retVal > 1) printf("Parse Req: Error\n");
    
    return 0;
    //send error
}



/*int parse_request(Request *reqRx, Header *headerRx, char *request){

    int index = 0; // current location within request[]
    int char_count = 0; // counts length of current substring
    int i = 0; //general loop counter
    bool end = 0; //flags end of header
    char headbuff[MAX_HEADER_SIZE]; //Create array to read in each header value
    char * cmdbuff; //Buffer to store command and filepath
    char * end_of_header; //Pointer used to store position of end of header
    bool valid; //flag invalid input
    
    //retrieve request 'command'
    while(request[index++] != ' ')
        char_count++;
    //Index is now at beginning of filepath

    //Allocate memory for command and leave space for NULLBYTE
    //to store as string
    if(cmdbuff = (char *)malloc((char_count + 1)*sizeof(char)) == NULL)
        return -1;
    
    //Store command as temporary string
    for(i=0; i<char_count; i++)
        cmdbuff[i] = request[i];
   //Append null byte
    cmdbuff[char_count] = NULLBYTE;

    //Assigns mode of operation to reqRx Mode enum.
    //Check for invalid mode/command.
    for(i = 0, valid = false; i < NUM_MODES; i++)
        if(!strcmp(cmdbuff, mode_strs[i]))
        {
            (reqRx->cmdRx) = i;
            valid = true;
        }

    #ifdef DEBUG
    printf("reqParse: valid == %d\n", valid);
    printf("reqParse: cmdRx: %s \n", cmdbuff);
    printf("reqParse: Operating in mode %u\n", (reqRx->cmdRx));
    #endif

    //Count number of bytes in filepath    
    for(char_count = 0, i = index; request[i++] != '\n'; char_count++);

    //Allocate memory for filepath
    reqRx->filepath = (char *)malloc((char_count + 1)*sizeof(char));
    //Store filepath as string
    for(i = 0; i < char_count; i++)
        (reqRx->filepath)[i] = request[i + index];

    (reqRx->filepath)[char_count] = NULLBYTE;

    //Set index to first letter of header first header string.
    index += i + 1;

    #ifdef DEBUG
     printf("reqParse: Filepath is %s\n", (reqRx->filepath));
     printf("reqParse: request[index] = %c\n", request[index]);
    #endif

    while(end == false)
    {
        char_count = 0;
        //strchr returns pointer to first ':' found after ip str
        //In this case, we only want to the value before ':'
        end_of_header = strchr((request + index), (int)END_HEAD);
        //Subtracting position of final byte from first byte gives
        //number of bytes to read in including null byte
        //Store this in char_count
        char_count = end_of_header - (request + index);

        #ifdef DEBUG
        printf("reqParse: Printing %d number of bytes to header.\n", char_count);
        #endif
        //Now copy header into headbuff str. 
        strncpy(headbuff, (request + index), char_count);
        headbuff[char_count] = NULLBYTE;
        //Incremenet index so it sits at header arg.
        //+1 for ':'
        index += char_count + 1;
        //Now need to read header value and increment index
        //Read in value as string
        //+================================================
        #ifdef DEBUG
        printf("reqParse: Header n is %s\n", headbuff);
        #endif
        //Now compare header and assign enum
        //Tests for invalid header
        //Loop runs for each possible iteration of i + 1 for
        //an invalid iteration.
        //RETURN POINTER TO LAST '\N' AND CHECK FOR FILE IN MAIN
        for(i = 0, valid = false; i < NUM_HEAD + 1; i++)//CHANGE IT BACK
        {
            if(!strcmp(headbuff, header_name[i]))
            {
                switch(i)
                {
                    case DATA_L:
                        //Count amount of char in header value
                        for(i = index, char_count = 0; request[i++] != '\n'; char_count++)    
                        //Need to read number here
                        (headerRx->data_length) = strtol((request + index), NULL, DEC);
                        //Index is now at next header value
                        index += char_count + 1;
                        valid = true;
                        
                        #ifdef DEBUG
                        printf("reqParse: Data length is %ld bytes\n", (headerRx->data_length));
                        printf("reqParse: Input is valid == %d\n", valid);
                        printf("reqParse: req[index] is %c\n", request[index]);
                        #endif
                        break;

                    case TIMEOUT:
                        //Count amount of char in header value
                        for(i = index, char_count = 0; request[i++] != '\n'; char_count++)    
                        //Need to read number here
                        (headerRx->timeout) = strtol((request + index), NULL, DEC);
                        //Index is now at beginning of next header value
                        index += char_count + 1;
                        valid = true;
                        
                        #ifdef DEBUG
                        printf("reqParse: Timeout is %ld bytes\n", (headerRx->timeout));
                        printf("reqParse: Input is valid == %d\n", valid);
                        printf("reqParse: req[index] is %c\n", request[index]);
                        #endif
                        break;

                    case IF_EXISTS:
                        //Count amount of char in header value
                        for(i = index, char_count = 0; request[i++] != '\n'; char_count++)    
                        //Need to read number here
                        (headerRx->ifexist) = strtol((request + index), NULL, DEC);
                        //Index is now at beginning of next header value
                        index += char_count + 1;
                        valid = true;

                        #ifdef DEBUG
                        printf("reqParse: If-exists is %ld bytes\n", (headerRx->ifexist));
                        printf("reqParse: Input is valid == %d\n", valid);
                        printf("reqParse: req[index] is %c\n", request[index]);
                        #endif
                        break;

                    default:
                        valid = false;
                        break;
                }
            }
        }
        //Check for invalid input
        //if(!valid) printf("reqParse: Input is shite\n"); //return appropriate error code
        //Check to see if header is final one.
        if(!valid){
            printf("reqParse: Bad header values\n");
            //return error
        }else{
            if(request[index + 1] == '\n')
            {
                printf("reqParse: End of headers. Nothing left to process\n");
                //Assign data position pointer value of last header byte
                (headerRx->data_pos) = request[index + 1];
                end = true;
            }
        }
<<<<<<< HEAD
    }
    return 0;
}
                                 
=======

    return 0;
}
*/                                    
>>>>>>> 3655ab7e5023da9bcb5b6d3f10c2b3b6a96e35fe
/*
int send_error_response(int status_code, SOCKET connectSocket){
	
	




}
							
int gift_server(Request reqRx, Header headerRx, SOCKET connectSocket){

use recv funvtion with FILE * as argument



}

int weasel_server(Request reqRx, Header headerRx, SOCKET connectSocket){





}


int list_server(Request reqRx, Header headerRx, SOCKET connectSocket)
{

    DIR *dp;
    struct dirent *ep;
    dp = opendir(reqRx.filepath);

    if(dp != NULL)
    {
        while(ep = readdir(dp))
            fprintf(stdout, "%s", ep->d_name);
        
        closedir(dp);
    }
    else
    {
        fprintf(stderr, "Can't open the directory\n");
    }
    return 0;
}*/