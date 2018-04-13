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

typedef struct Response_t{
	char * header;
	char * body;
}response_t;

typedef struct{
	char * name;
	char * value;
}Header;

/* Mode-agnostic request construction */
static int request(enum Mode, char **, const char *);
/* Response parser */
static int response(enum Mode, char *, const char *);
/* Mode-specific request construction */
static int gift(char ** requestbuf, const char * filepath);
static int weasel(char ** requestbuf, const char * filepath);
static int list(char ** requestbuf, const char * filepath);
/* Array of pointers to mode-specific operation functions */
static int (*mode_funs[])(char **, const char *) = {NULL, gift, weasel, list};

#define IPV4LEN 12
#define OPTSTRING "vqg:w:l:hi:p:"
#define READ_ONLY "r"
#define MAXRESPONSE 90

#define HEADER_SEPARATOR ":"
#define HEADER_TERMINATOR "\n"
#define STATUS_SEPARATOR " "
#define STATUS_TERMINATOR "\n"

static FILE* file_parameters(char *filepath, long int *file_size, bool verbose);
static char *process_input(int argc, char ** argv, enum Mode * mode, bool *verbose, char *ip, uint16_t *port);
char * extract_header(const char * buf, Header * header, bool * finished);
char * extract_status(const char * buf, char ** description, int *status_code);

int main(int argc, char ** argv)
{
	char * filepath, ip[IPV4LEN];
	uint16_t port = 0;
	enum Mode mode = NONE;

	// Set flag default values
	verbose = false;

	filepath = process_input(argc, argv, &mode, &verbose, ip, &port);


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
	if(ret = send(sockfd, requestbuf, strlen(requestbuf), 0) < 1)
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
	if(NULL == (pos = extract_status((const char *)responsebuf, &status_description, &status_code)))
	{
		fprintf(stderr, "client: failed to find a status in response.\n");
		return EXIT_FAILURE;
	}
	if(verbose) printf("extract_status: response status \"%d %s\".\n",
			*status_code, *status_description);
	// Extract headers
	bool headers_finished = false;
	Header ** headers, ** headers_tmp;
	int n_headers;
	char * last_term = pos; // Position of last HEADER_TERMINATOR found
	for(n_headers = 0; false == headers_finished && ret > 0;)
	{
		// Copy unprocessed data into beginning of responsebuf
		// Receive <= (MAXRESPONSE - last_term) bytes, 
		// Append to responsebuf
		memmove(last_term + 1, responsebuf, MAXRESPONSE - (last_term - responsebuf));
		ret = recv(sockfd, responsebuf + last_term, MAXRESPONSE - (last_term - responsebuf));
		printf("client:RECEIVED>>>%s<<<\n", responsebuf);
		// find all headers in buffer
		for(; false == headers_finished && NULL != pos; i++)
		{
			// headers not finished, header was found
			headers_tmp = realloc(headers, sizeof(Header) * (1 + n_headers));
			if(headers_tmp == NULL) // No complete header found in buffer
			   continue;
			headers = headers_tmp;
			n_headers++;
			last_term = pos;
			pos = extract_header(pos + 1, headers[i], &headers_finished);
		}
	}
	// Process headers
	// 	Receive any data
	

	free(filepath);
	return EXIT_SUCCESS;
}
/* Populates fields in <header> with firse header found in <buf>
 * Returns pointer to the end of the header or NULL if not found */
char * extract_header(const char * buf, Header * header, bool * finished)
{
	char * sep = buf, * term = buf; // Position of substring in string
	if(0 == strncmp(buf + 1, HEADER_TERMINATOR, 1))
	{
		*finished = true;
		return buf + 1;
	}
	if(NULL == (sep = strstr(buf, HEADER_SEPARATOR))
			|| NULL == (term = strstr(buf, HEADER_TERMINATOR)))
	{
		fprintf(stderr, "extract_header: header not found in %s.\n", buf);
		return NULL;
	}
	if(NULL == (header->name = strndup(buf, sep - buf))
			|| NULL == (header->value = strndup(sep + 1, term - sep + 1)))
	{
		fprintf(stderr, "extract_header: no memory for header %s.\n", buf);
		return NULL;
	}
	if(verbose) printf("extract_header: header %s read, value %s.\n",
			header->name, header->value);
	return term;
}

/* Extracts a status number and description from a buffer <buf> */
char * extract_status(const char * buf, char ** description, int *status_code)
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
static int response(enum Mode mode, char * responsebuf, const char * filepath)
{
	return EXIT_SUCCESS;
}
static int request(enum Mode mode, char ** requestbuf, const char * filepath)
{
	int err = 0;
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
static int weasel(char ** requestbuf, const char * filepath)
{
	// How many headers?
	// None?
	finish_headers(requestbuf);
	return 0;
}

static int gift(char ** requestbuf, const char * filepath)
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


static char *process_input(int argc, char ** argv,enum Mode * mode, bool *verbose, char *ip, uint16_t *port)
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
				*verbose = true;
				break;
			case 'q':
				*verbose = false;
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
				printf("-l\t--list\tList files from <filepath> and below.\n");
				printf("-h\t--help\tDisplay this help.\n");
				printf("-i\t--ip\tIPv4 of host to connect to.\n");
				printf("-p\t--port\tPort on host to connect to.\n");
				exit(EXIT_FAILURE);
		}
	}
	return filepath;
}

int gift_client(char *filepath, bool verbose)
{
	FILE *input_file;
	long int size_of_file;
	int error_check;
	char *gift_request;

	input_file = file_parameters(filepath, &size_of_file, verbose);
	if(input_file == NULL)
	{
		printf("GIFT_CLIENT: Error opening file for transmission\n");
		return EXIT_FAILURE;
	}

	//Setting up the header for the data
	gift_request = gift_header(filepath, size_of_file, verbose);
	if(gift_request == NULL)
	{
		printf("GIFT_CLIENT: Error in creating the header\n");
		return EXIT_FAILURE;
	}

	//Might be nicer to pass memory value of gift_request
	//Not too major, this is functional
	gift_request = gift_data(input_file, verbose, gift_request);
	if(gift_request == NULL)
	{
		printf("GIFT_CLIENT: Error in reading in the data");
		return EXIT_FAILURE;
	}

	//Sedning the data to the server
	error_check = gift_send(gift_request, verbose);
	if(error_check == -1)
	{
		printf("GIFT_CLIENT: Error in sending the data");
		return EXIT_FAILURE;
	}


	return EXIT_SUCCESS;
}

static char* gift_data(FILE* input_file, bool verbose, char* gift_request){return (char*) NULL;}
static int   gift_send(char *gift_request, bool verbose){return -1;}

char *gift_header(char *filepath, long int size_of_file, bool verbose)
{
	//Header will always contain: FILENAME <filepath> FILESIZE <size_of_file> LF
	//The characters require 21 bytes of memory, the size of long int is machine dependent,
	//the size of the path is variable
	//Using sizeof(long int), factors in padding, can switch to fixed length either
	//Memory must also be allocated for the filename and the size of the file

	//Currently the file name is the same as its path, see notes V2 for more details
	char* gift_header;
	long int size_of_name = strlen(filepath);
	//This won't work as of yet, will need to change long int into a string
	//Otherwise can't include in the gift request
	gift_header = (char*)malloc(21 + size_of_name + sizeof(long int));




	return gift_header;
}


//Function to check the parameters of a file
//Returns NULL if file does not exist, or if there is an error in reading the file
FILE *file_parameters(char *filepath,long int *size_of_file, bool verbose)
{
	FILE* input_file;

	if( ( input_file = fopen(filepath, READ_ONLY) )== NULL )
	{
		if(verbose)
			printf("File does not exist\n");
		return input_file;
	}
	else
	{
		if(verbose)
			printf("Checking file parameters");
		fseek(input_file, 0L, SEEK_END);
		*size_of_file = ftell(input_file);
		if(verbose)
			printf("File is %ld bytes long", *size_of_file);
		if(size_of_file < 0)
		{
			perror("Error in file_parameters: File size is less than zero: ");
			return NULL;
		}

		rewind(input_file);
	}

	return input_file;
}
