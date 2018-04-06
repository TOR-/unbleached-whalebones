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

static bool verbose;
enum Mode {NONE, GIFT, WEASEL, LIST};

#define IPV4LEN 12
#define OPTSTRING "vqg:w:l:hi:p:"

char *process_input(int argc, char ** argv,enum Mode * mode, bool *verbose, char *ip, uint16_t port);

int main(int argc, char ** argv)
{
    char * filepath, ip[IPV4LEN];
	uint16_t port
    enum Mode mode = NONE;

    // Set flag default values
    verbose = false;

    filepath = process_input(argc, argv, &mode, &verbose, ip, port);

    if(filepath == NULL)
        return EXIT_FAILURE;

    if(verbose) printf("client: verbose mode enabled\n");
    if(verbose) printf("Running in mode %d\n", mode);

	SOCKET sockfd = TCPSocket(AF_INET);

	// connect server
	// transfer file
	// 	read file TODO: transfer in chunks
	// 	construct headers
	// 	construct message
	// more files? repeat:close
    return EXIT_SUCCESS;
}

char *process_input(int argc, char ** argv,enum Mode * mode, bool *verbose, char *ip, uint16_t port)
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
				if(sscanf(optarg, "%hu", &port) <= 0)
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
