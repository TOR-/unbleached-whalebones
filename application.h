#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdbool.h>

#define READ_ONLY "r"
#define DEBUG

#define END_HEAD ':'
#define MAX_HEADER_SIZE 20
#define MAX_STATUS	341
#define DEC 10  //Number base for use with strtol
#define NULLBYTE '\0'
#define BUFSIZE 80


#define SWDIR	"Server_Files/"
#define HEADER_SEPARATOR ':'
#define HEADER_TERMINATOR '\n'
#define STATUS_SEPARATOR " "
#define STATUS_TERMINATOR "\n"

#define HEADERINITBUFLEN NUM_HEAD

bool verbose;

//typedef enum {ALLOC_FAIL = -1, } Error;
typedef enum {GIFT, WEASEL, LIST, NUM_MODE} Mode_t;
extern const char * mode_strs[];
extern const char * header_name[];
typedef enum { DATA_LENGTH, TIMEOUT, IF_EXISTS, NUM_HEAD} H_name;
typedef enum {PRINT, WRITE} Process;

typedef struct head{ // Should members be character types?? Change before/after?
    long int  data_length;
    long int timeout ;
    //Change to enum
    //char *ifexist;
    //char * data_pos;
} Header;

typedef struct req{
    Mode_t cmdRx;
    char * filepath;
    Header * header;
} Request;

typedef struct Response_t{
	char * header;
	char * body;
}response_t;

typedef struct{
	char * name;
	char * value;
}Header_t;

typedef struct {
  Header_t *array;
  size_t used;
  size_t size;
} Header_array_t;

void init_header_array(Header_array_t *a, size_t initial);
void insert_header_array(Header_array_t *a, Header_t element);
void free_header_array(Header_array_t *a);

int append_header(char ** header, char * name, char * content);
int finish_headers(char ** headers);


FILE* file_parameters(char *filepath, long int *file_size);
int send_data(int sockfd, char * filepath);


bool parse_command(char * buff, Mode_t * cmdRx, int * index);
int parse_filepath(char * buff, char ** filepath, int * index);
char * extract_header(char * buf, Header_array_t * header_array, bool * finished);
int parse_header(char * buff, Header * head, int * index);
int read_data(char * remainder,  Process mode_data, char * filepath, int data_length, int sockfd);
int send_data(int socket, char * filepath);

typedef enum {
/* ↓ 1xx successful transaction ↓ */
	S_CONNECTION_SUCCESSFUL 						= 100, 
	S_COMMAND_RECOGNISED 							,
	S_CONNECTION_TERMINATED							,
	S_COMMAND_SUCCESSFUL							= 110,
	  
/* ↓ 2xx informational ↓ */
	S_SERVER_PREPARING_FOR_SHUTDOWN 				= 200,
	S_USERNAME_OK_NEED_PASSWORD						= 210,
	  
/* ↓ 3xx client error ↓ */
	S_CLIENT_ERROR									= 300,
	S_COMMAND_NOT_RECOGNISED						,
	S_COMMAND_NONSENSICAL_AT_THIS_TIME				,
	S_COMMAND_INCOMPLETE							,
	S_HEADER_ARGUMENT_INVALID						,
	S_HEADER_NOT_RECOGNISED							,
	S_NOT_LOGGED_IN									= 310,
	S_INSUFFICIENTLY_PRIVILEGED						,
	S_CREDENTIALS_INCORRECT							,
	S_TIMEOUT										= 320,
	S_SERVER_GOT_BORED_AND_LEFT_THIS_CONVERSATION	,
	S_SOCKET_UNPLUGGED								= 330,
	S_FILE_NOT_FOUND								= 340,
	S_ILLEGAL_FILE_PATH								,
	  
/* ↓ 4xx server error ↓ */
	S_SERVER_ERROR									= 400,
	S_COMMAND_NOT_IMPLEMENTED						,
	S_SERVER_WRITE_ERROR							,
	S_CANNOT_SATISFY_REQUEST						= 410,
	S_UNRECOGNISED_ENCODING						
} Status_code;

//Client error status descriptions
//see application.c
extern const char * status_descriptions[];

#endif
