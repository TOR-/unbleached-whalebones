/* Client code to implement UWB DRAFT A v0.1 */

#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>

#include "CS_TCP.h"
#include "application.h"


/* Mode-agnostic request construction */
static int request(Mode_t mode, char **, char * );
/* Response parser */
static int response(SOCKET sockfd, Mode_t mode, char * filepath);
/* Mode-specific request construction */
int gift_request(char ** requestbuf, char * filepath);
int weasel_request(char ** requestbuf, char * filepath);
int list_request(char ** requestbuf, char * filepath);
int (*mode_request_funs[])(char **, char *) = {gift_request, weasel_request, list_request};
/* Mode-specific request dealiing with */
int gift_response(char * remainder, SOCKET sockfd, char * filepath, Header_array_t*);
int weasel_response(char * remainder, SOCKET sockfd, char * filepath, Header_array_t*);
int list_response(char * remainder, SOCKET sockfd, char * filepath, Header_array_t*);
int (*mode_response_funs[])(char * remainder, SOCKET sockfd, char * filepath, Header_array_t*) = {gift_response, weasel_response, list_response};

#define IPV4LEN 12
#define OPTSTRING "vqg:w:l:hi:p:"
#define READ_ONLY "r"
#define MAXRESPONSE 1024

char * process_input(int argc, char ** argv, int * mode, char * ip, uint16_t * port);
char * extract_header(char * buf, Header_array_t * header_array, bool * finished);
char * extract_status(char * buf, char ** description, int *status_code);
char * header_search(char * target_header, Header_array_t * header_array);



int main(int argc, char ** argv)
{
	char * filepath, ip[IPV4LEN];
	uint16_t port = 0;
	int mode;

	// Set flag default values
	verbose = false;

	filepath = process_input(argc, argv, &mode, ip, &port);


	if(filepath == NULL)
	{
		fprintf(stderr, "No file specified.\n");
		return EXIT_FAILURE;
	}

	if(verbose) printf("client: verbose mode enabled\n");
	if(verbose) printf("Running in mode %d\n", mode);

	SOCKET sockfd = TCPSocket(AF_INET);

	// now connected
	// mode switch: WEASEL, GIFT, LIST
	char * requestbuf;
	// Construct request
	if(0 != request(mode, &requestbuf, filepath))
		return EXIT_FAILURE;
	if(verbose)printf("client: sending request:\n>>>%s<<<\n", requestbuf);

	// Connect to server
	if(TCPclientConnect(sockfd, ip, port) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	int ret = 0;
	// ============= Send Request ================================================
	if((ret = send(sockfd, requestbuf, strlen(requestbuf), 0)) < 1)
	{
		fprintf(stderr, "client: %s.\n", strerror(errno));
		return EXIT_FAILURE;
	}
	free(requestbuf);
	
	// ============= Sending Data ================================================
	if( mode == GIFT )
	{
		int check = send_data(sockfd, filepath);
		if(check == 0)
			printf("Data has been sent to the server\n");
		else
			printf("Error sending data to the server\n");
	}
	
	
	// ============= Request Sent ================================================

	// ============= Deal with Response ==========================================
	response(sockfd, mode, filepath);
	// FINISHED!!!!
	free(filepath);
	return EXIT_SUCCESS;
}

/* =========================================================================
 * Utility functions for processing responses from the server
 * ========================================================================= */

/* Extracts a status number and description from a buffer <buf> */
char * extract_status(char * buf, char ** description, int *status_code)
{
	char * sep = buf, * term = buf;
	if(NULL == (sep = strstr(buf, STATUS_SEPARATOR))
			|| NULL == (term = strstr(buf, STATUS_TERMINATOR)))
	{
		fprintf(stderr, "extract_status: status not found in %s.\n", buf);
		return NULL;
	}
	if(NULL == (*description = strndup(sep + 1, term - sep - 1)))
	{
		fprintf(stderr, "extract_status: no memory for status %s.\n", sep + 1);
		return NULL;
	}
	*status_code = atoi(strtok(buf, STATUS_SEPARATOR));
	return term;
}

/* ==========================================================================
 * Client processing of the response from the server
 * ========================================================================== */

/* Takes a response, block by block and parses it.
 * Extracts status code, status message, header names and values, and data */
static int response(SOCKET sockfd, Mode_t mode, char * filepath)
{
	int status_code;
   	char * status_description;
	Header_array_t headers;
	int ret = -1;	
	char * responsebuf = (char *)malloc(MAXRESPONSE);
	if(NULL == responsebuf)
	{
		fprintf(stderr, "client: failed to allocate memory for response. %s\n",
				strerror(errno));
		return EXIT_FAILURE;
	}
	// Receive response
	char * pos; // Current position in response buffer
	// Extract status	
	memset(responsebuf, 0, MAXRESPONSE);
	ret = recv(sockfd, responsebuf, MAXRESPONSE, 0);
	if(NULL == (pos = extract_status(responsebuf, &status_description, &status_code)))
	{
		fprintf(stderr, "client: failed to find a status in response.\n");
		return EXIT_FAILURE;
	}

	// Extract headers
	bool headers_finished = false;
	int n_headers;
	init_header_array(&headers, HEADERINITBUFLEN);

	char * last_term = pos; // Position of last HEADER_TERMINATOR found
	int spaces = 0;
	for(n_headers = 0; false == headers_finished && ret > 0;)
	{
		memmove(responsebuf, last_term + 1, MAXRESPONSE - spaces);
		if('\n' == *responsebuf)
		{
			headers_finished = true;
			break;
		}
		ret = recv(sockfd, responsebuf + (MAXRESPONSE - spaces), spaces, 0);
		// TODO zero unused space in responsebuf
		for(pos = responsebuf; false == headers_finished && NULL != pos && *(last_term + 1) != '\n'; n_headers++)
		{
			// headers not finished, header was found
			// save position of last terminator found in headers
			last_term = pos;
			pos = extract_header(pos, &headers, &headers_finished);
		}
		// Remaining unprocessed data
		spaces = last_term + 1 - responsebuf;
		if(MAXRESPONSE == spaces) // No header has been processed
		{
			fprintf(stderr, "response: header too long for processing.\n");
			exit(EXIT_FAILURE);
		}
	}
	if(verbose) printf("client: finished receiveing headers.\n");
	// Check status
	if( status_code > S_CLIENT_ERROR )
	{
		fprintf(stderr, "response: request failed: %d %s.\n...Exiting...\n",
				status_code, status_description);
		exit(EXIT_FAILURE);
	}
	else if(verbose) printf("response: response status \"%d %s\".\n",
			status_code, status_description);
	free(status_description);
	// Check mode to see if there should be data ( WEASEL )
	// 	Process headers
	//		search headers for data-length header	
	//		if present: 
	// 			Receive any data
	if(verbose) 
		printf("client: entering mode-specific response processing.\n Mode: %s.\n", 
				mode_strs[mode]);
	mode_response_funs[mode](last_term + 1, sockfd, filepath, &headers);

	free_header_array(&headers);
	free(responsebuf);
	return EXIT_SUCCESS;
}

int gift_response(char * remainder, SOCKET sockfd, char * filepath, Header_array_t * headers)
{
	return EXIT_SUCCESS;
}
int weasel_response(char * remainder, SOCKET sockfd, char * filepath, Header_array_t * headers)
{
	long int data_length = -1;
	char * data_length_str = NULL;
	char * target_header = "Data-length";
	int check = -1;

	
	
	
	data_length_str = header_search(target_header, headers);
	if( data_length_str == NULL )
	{
		printf("List Response: Error Data Length Header not found\n");
		exit(EXIT_FAILURE);
	}
	data_length = atoi( data_length_str );
	
	if( data_length < 0 )
	{
		printf("List Response: Error Data Length was negative\n");
		exit(EXIT_FAILURE);
	}
	
	check = read_data(remainder, WRITE, filepath, data_length, sockfd);
	
	if(check)
		printf("Error has occured in reading the attached data!");
	
	
	free(data_length_str);

	return EXIT_SUCCESS;
}

int list_response(char * remainder, SOCKET sockfd, char * filepath, Header_array_t * headers)
{
	long int data_length = -1;
	int check = -1;
	char * data_length_str = NULL;
	char * target_header = "Data-length";
	
	if(verbose)
		printf("Number of headers: %zu\n",headers->used);
	
	data_length_str = header_search(target_header, headers);
	if( data_length_str == NULL )
	{
		printf("List Response: Error Data Length Header not found\n");
		exit(EXIT_FAILURE);
	}
	
	data_length = atoi( data_length_str );
	
	if( data_length < 0 )
	{
		printf("List Response: Error Data Length was negative\n");
		exit(EXIT_FAILURE);
	}
	
	
	check = read_data(remainder, PRINT, filepath, data_length, sockfd);

	if(check)
		printf("Error has occured in reading the attached data!");
	
	free(data_length_str);
	
	return EXIT_SUCCESS;
}


/* ===========================================================================
 * Construction of the request to the server in a character buffer
 * =========================================================================== */

static int request(Mode_t mode, char ** requestbuf, char * filepath)
{
	// Add command to header
	// Allocate memory
	if(NULL == (*requestbuf = (char *) malloc(strlen(mode_strs[mode]) + 1 + strlen(filepath) + 2)))
	{
		fprintf(stderr, "request: failed to allocate memory.\n");
		exit(EXIT_FAILURE);
	}
	// Write command to memory
	if(1 > sprintf(*requestbuf, "%s %s\n", mode_strs[mode], filepath))
	{
		fprintf(stderr, "request: failed to create command line.\n");
		exit(EXIT_FAILURE);
	}
	// Append headers as appropriate
	// Headers common to all requests go here
	//time_t now;
    //time(&now);
    //char buf[sizeof "2018-04-01T01:00:09Z"];
    //strftime(buf, sizeof buf, "%FT%TZ", gmtime(&now));
	//append_header(requestbuf, "Date", buf);

	// Call individual request constructors
	if(0 != mode_request_funs[mode](requestbuf, filepath))
	{
		fprintf(stderr, "request: error in processing %s request.\n", mode_strs[mode]);
		return EXIT_FAILURE;
	}
	// Request has been constructed successfully
	return EXIT_SUCCESS;	
}

/* Appends weasel-specific headers to request in *requestbuf */
int weasel_request(char ** requestbuf, char * filepath)
{
	// How many headers?
	finish_headers(requestbuf);
	return 0;
}
int list_request(char ** requestbuf, char * filepath)
{
	// How many headers?
	finish_headers(requestbuf);
	return 0;
}

int gift_request(char ** requestbuf, char * filepath)
{
	FILE *file;
	long int size_of_file;
	
	//input_file = file_parameters(filepath, &size_of_file);

	if((file = fopen(filepath, READ_ONLY)) == NULL )
	{	
		fprintf(stderr, "%s:error opening %s: %s.\n",
				__FUNCTION__, filepath, strerror(errno));
		return EXIT_FAILURE;
	}
	size_of_file = file_length(filepath);
	char data_len_buf[LONG_MAX_DIGITS + 1];
	snprintf(data_len_buf, LONG_MAX_DIGITS, "%ld", size_of_file);
	append_header(requestbuf, header_name[DATA_LENGTH], data_len_buf);

	finish_headers(requestbuf);
	return EXIT_SUCCESS;
}

/* ========================= INPUT PROCESSING ================================*/
char * process_input(int argc, char ** argv, int * mode, char * ip, uint16_t * port)
{
	char optc; // Option character
	int opti = 0; // Index into option array
	char * filepath = NULL;

	while(true)
	{
		static struct option options[] =
		{
			{"verbose",	no_argument, 		0,	'v'},
			{"quiet",	no_argument, 		0,	'q'},
			{"gift",	required_argument,	0,	'g'},
			{"weasel",	required_argument,	0,	'w'},
			{"list",	required_argument,	0,	'l'},
			{"help",	no_argument,		0,	'h'},
			{"ip",		required_argument,	0,	'i'},
			{"port",	required_argument,	0,	'p'}
		};
		optc = getopt_long(argc, argv, OPTSTRING, options, &opti);
		if (-1 == optc) // End of options
		{
			if(1 == argc)
				optc = 'h';
			else
				break;
		}
		switch(optc)
		{
			case 0: // Flag has been set
				break;
			case 'v':
				verbose = true;
				break;
			case 'q':
				verbose = false;
				break;
			case 'g':
				if(NULL == (filepath = (char*) malloc(strlen(optarg) + 1)))
				{
					fprintf(stderr,
							"client: memory not available for filepath %s\n",
							(char *) optarg);
					exit(EXIT_FAILURE);
				}
				strcpy(filepath, optarg);
				*mode = GIFT;
				break;
			case 'w':
				if(NULL == (filepath = (char*) malloc(strlen(optarg) + 1)))
				{
					fprintf(stderr,
							"client: memory not available for filepath %s\n",
							(char *) optarg);
					exit(EXIT_FAILURE);
				}
				strcpy(filepath, optarg);
				*mode = WEASEL;
				break;
			case 'l':
				if(NULL == (filepath = (char*) malloc(strlen(optarg) + 1)))
				{
					fprintf(stderr,
							"client: memory not available for filepath %s\n",
							(char *) optarg);
					exit(EXIT_FAILURE);
				}
				strcpy(filepath, optarg);
				*mode = LIST;
				break;
			case 'i':
				strcpy(ip, optarg);
				break;
			case 'p':
				if(sscanf(optarg, "%hu", port) <= 0)
				{
					fprintf(stderr,
							"client: invalid port number %s %s\n",
							optarg, strerror(errno));
					exit(EXIT_FAILURE);
				}
				break;
			case 'h':
			case '?': // Unrecognised option, error message printed by getopt_long
			default:
				printf(
						"Usage: %s [-v|-q|-g <filepath>|-w <filepath>|-l <filepath>|-h] -i <ip address> -p <port>\n",
						argv[0]);
				printf("-v\t--verbose\tRun the program in verbose mode.\n");
				printf("-q\t--quiet\tRun the program in quiet mode. Default.\n\n");
				printf("-g\t--gift\tGIFT server with file <filepath>.\n");
				printf("-w\t--weasel\tWeasel(get) file <filepath> from server.\n");
//				printf("-l\t--list\tList files from <filepath> and below.\n");
				printf("-h\t--help\tDisplay this help.\n");
				printf("-i\t--ip\tIPv4 of host to connect to.\n");
				printf("-p\t--port\tPort on host to connect to.\n");
				exit(EXIT_FAILURE);
		}
	}
	return filepath;
}

//Returns the value associated with the target header as a string
//Returns NULL if target header is not found
char * header_search(char * target_header, Header_array_t * headers)
{
	int i, check = -1;
	char* needle;
	
	for(i = 0; i < headers->used ; i++)
	{
		check = strcmp(headers->array[i].name,target_header);
		if(check == 0)
		{
			needle = malloc( strlen(headers->array[i].value) );
			needle = headers->array[i].value;
			if( needle == NULL )
				perror("Malloc Error in header_search: ");
			return needle;
		}
	}
	return NULL;
}
