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
#include <netdb.h>

#include "CS_TCP.h"
#include "application.h"


/* Mode-agnostic request construction */
static int request(Mode_t mode, char **, char * );
/* Response parser */
//static int response(Mode_t mode, char *, char * );
/* Mode-specific request construction */
static int gift(char ** requestbuf, char * filepath);
static int weasel(char ** requestbuf, char * filepath);
/* Array of pointers to mode-specific operation functions */
static int (*mode_funs[])(char **, char *) = {gift, weasel};

#define IPV4LEN 12
#define OPTSTRING "vqg:w:hi:p:"
#define READ_ONLY "r"
#define MAXRESPONSE 90

#define HEADER_SEPARATOR ':'
#define HEADER_TERMINATOR '\n'
#define STATUS_SEPARATOR " "
#define STATUS_TERMINATOR "\n"

FILE * file_parameters(char * filepath, long int *file_size);
static char * process_input(int argc, char ** argv, int * mode, char * ip, uint16_t * port);
char * extract_header(char * buf, Header_array_t * header_array, bool * finished);
char * extract_status(char * buf, char ** description, int *status_code);

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
	if(verbose)printf("client: sending request:\n>>>\n%s\n<<<\n", requestbuf);

	// Connect to server
	if(TCPclientConnect(sockfd, ip, port) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	int ret = 0;
	// Send request
	if((ret = send(sockfd, requestbuf, strlen(requestbuf), 0)) < 1)
	{
		fprintf(stderr, "client: %s.\n", strerror(errno));
		return EXIT_FAILURE;
	}
	free(requestbuf);

	char * responsebuf = (char *)malloc(MAXRESPONSE);
	if(NULL == responsebuf)
	{
		fprintf(stderr, "client: failed to allocate memory for response.\n%s",
				strerror(errno));
		return EXIT_FAILURE;
	}
	// Receive response
	char * pos; // Current position in response buffer
	// Extract status
	int status_code = 0;
	char * status_description;
	
	memset(responsebuf, 0, MAXRESPONSE);
	ret = recv(sockfd, responsebuf, MAXRESPONSE, 0);
	if(NULL == (pos = extract_status(responsebuf, &status_description, &status_code)))
	{
		fprintf(stderr, "client: failed to find a status in response.\n");
		return EXIT_FAILURE;
	}
	if(verbose) printf("extract_status: response status \"%d %s\".\n",
			status_code, status_description);
	// Extract headers
	bool headers_finished = false;
	Header_array_t headers;
	int n_headers;
	init_header_array(&headers, HEADERINITBUFLEN);

	char * last_term = pos; // Position of last HEADER_TERMINATOR found
	int spaces = 0;
	for(n_headers = 0; false == headers_finished && ret > 0;)
	{
		// Copy unprocessed data into beginning of responsebuf
		// Receive <= (MAXRESPONSE - last_term) bytes, 
		// Append to responsebuf

		// Empty spaces to fill from last loop ( +1 for \n )
		spaces = last_term - responsebuf + 1;
		memmove(responsebuf, last_term + 1, MAXRESPONSE - spaces);
		ret = recv(sockfd, responsebuf + (MAXRESPONSE - spaces), spaces, 0);
		// TODO zero unused space in responsebuf
		// find all headers in buffer
		for(pos = responsebuf; false == headers_finished && NULL != pos; n_headers++)
		{
			// headers not finished, header was found
			//responsebuf = pos;
			last_term = pos;//responsebuf;
			pos = extract_header(pos, &headers, &headers_finished);
		}

	}
	if(verbose) printf("client: finished receiveing headers.\n");
	// Process headers
	// 	Receive any data
	
	free_header_array(&headers);
	free(filepath);
	return EXIT_SUCCESS;
}
/* Populates fields in <header> with first header found in <buf>
 * Returns pointer to the end of the header + 1 (skip \n) or NULL if not found */
char * extract_header(char * buf, Header_array_t * header_array, bool * finished)
{
	char * sep = buf, * term = buf; // Position of substring in string
	Header header;
	/*
	if(0 == strncmp(buf, (char *)HEADER_TERMINATOR, 1))
	*/
	if(*buf == HEADER_TERMINATOR)
	{
		*finished = true;
		return buf + 1;
	}
	if(NULL == (sep = strchr(buf, HEADER_SEPARATOR))
			|| NULL == (term = strchr(buf, HEADER_TERMINATOR)))
	{
		fprintf(stderr, "extract_header: header not found in %s.\n", buf);
		return NULL;
	}
	if(NULL == (header.name = strndup(buf, sep - buf))
			|| NULL == (header.value = strndup(sep + 1, term - sep - 1)))
	{
		fprintf(stderr, "extract_header: no memory for header %s.\n", buf);
		return NULL;
	}
	if(verbose) printf("extract_header: header %s read, value %s.\n",
			header.name, header.value);
	insert_header_array(header_array, header);
	return term + 1;
}

/* Extracts a status number and description from a buffer <buf> */
char * extract_status(char * buf, char ** description, int *status_code)
{
	static bool status_found = false;
	if(status_found) return buf;

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
	status_found = true;
	return term;
}
/* Takes a response, block by block and parses it.
 * Extracts status code, status message, header names and values, and data */
//static int response(Mode_t mode, char * responsebuf, char * filepath){return EXIT_SUCCESS;}
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
	append_header(requestbuf, "Date", "2018-04-07T14:31:32Z");

	// Call individual request constructors
	if(0 != mode_funs[mode](requestbuf, filepath))
	{
		fprintf(stderr, "request: error in processing %s request.\n", mode_strs[mode]);
		return EXIT_FAILURE;
	}
	// Request has been constructed successfully
	return EXIT_SUCCESS;	
}

/* Appends weasel-specific headers to request in *requestbuf */
static int weasel(char ** requestbuf, char * filepath)
{
	// How many headers?
	// None?
	finish_headers(requestbuf);
	return 0;
}

static int gift(char ** requestbuf, char * filepath)
{
	FILE *input_file;
	long int size_of_file;
	int error_check = 0;
	
	input_file = file_parameters(filepath, &size_of_file);
	if(input_file == NULL)
	{
		printf("GIFT_CLIENT: Error opening file for transmission\n");
		return EXIT_FAILURE;
	}
	
	//Can add in function to append headers at a later date
	finish_headers(requestbuf);
	
	
	error_check = append_data(input_file, requestbuf, size_of_file);
	if(requestbuf == NULL || error_check == -1)
	{
		printf("GIFT_CLIENT: Error in reading in the data");
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}


static char * process_input(int argc, char ** argv, int * mode, char * ip, uint16_t * port)
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

