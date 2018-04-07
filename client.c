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

static int   SEND(char **requestbuf, bool verbose);

static char *process_input(int argc, char ** argv, enum Mode * mode, bool *verbose, char *ip, uint16_t port);

int main(int argc, char ** argv)
{
    char * filepath, ip[IPV4LEN];
    uint16_t port = 0;
    enum Mode mode = NONE;

    // Set flag default values
    verbose = false;

    filepath = process_input(argc, argv, &mode, &verbose, ip, port);


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
    //    return EXIT_FAILURE;
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

    //Sedning the data to the server
    error_check = SEND(requestbuf, verbose);
    if(error_check == -1)
    {
        printf("GIFT_CLIENT: Error in sending the data");
        return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
}


static int list(char ** requestbuf, const char * filepath)
{ return EXIT_FAILURE; }



static char *process_input(int argc, char ** argv, enum Mode * mode, bool *verbose, char *ip, uint16_t port)
{
    char optc; // Option character
    int opti = 0; // Index into option array
    char * filepath = NULL;

    while(true)
    {
        static struct option options[] =
        {
            {"verbose",    no_argument,         0,    'v'},
            {"quiet",    no_argument,         0,    'q'},
            {"gift",    required_argument,    0,    'g'},
            {"weasel",    required_argument,    0,    'w'},
            {"list",    required_argument,    0,    'l'},
            {"help",    no_argument,        0,    'h'},
            {"ip",        required_argument,    0,    'i'},
            {"port",    required_argument,    0,    'p'}
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
