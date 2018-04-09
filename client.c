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
static int request(enum Mode mode, char ** requestbuf, const char * filepath);

/* Mode-specific request construction */
static int gift(char ** requestbuf, const char * filepath);
static int weasel(char ** requestbuf, const char * filepath);
static int list(char ** requestbuf, const char * filepath);
/* Array of pointers to mode-specific operation functions */
static int (*mode_funs[])(char **, const char *) = {NULL, gift, weasel, list};

#define IPV4LEN 12
#define OPTSTRING "vqg:w:l:hi:p:"
#define READ_ONLY "r"

static int   gift_client(char *filepath, bool verbose);
static char* gift_header(char *filepath, long int file_size, bool verbose);
static char* gift_data(FILE* input_file, bool verbose, char* gift_request);
static int   gift_send(char *gift_request, bool verbose);
static FILE* file_parameters(char *filepath, long int *file_size, bool verbose);
static char *process_input(int argc, char ** argv, enum Mode * mode, bool *verbose, char *ip, uint16_t *port);

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

	// connect server
	//if(TCPclientConnect(sockfd, ip, port) != EXIT_SUCCESS)
	//	return EXIT_FAILURE;
	// now connected
	// mode switch: WEASEL, GIFT, LIST
	char * requestbuf;
	if(0 != request(mode, &requestbuf, filepath))
		return EXIT_FAILURE;
	printf("client: sending request:\n>>>\n%s\n<<<\n", requestbuf);
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
{ return EXIT_FAILURE; }
static int list(char ** requestbuf, const char * filepath)
{ return EXIT_FAILURE; }



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
				if(NULL == (filepath = (char*) malloc(sizeof(optarg))))
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
				if(NULL == (filepath = (char*) malloc(sizeof(optarg))))
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
				if(NULL == (filepath = (char*) malloc(sizeof(optarg))))
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
							optarg, gai_strerror(errno));
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
