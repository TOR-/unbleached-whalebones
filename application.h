#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdbool.h>

#define READ_ONLY "r"
#define DEBUG

#define END_HEAD ':'
#define NUM_MODES 3
#define NUM_HEADERS 3
#define MAX_HEADER_SIZE 20

#define HEADERINITBUFLEN 5

bool verbose;

typedef struct Response_t{
	char * header;
	char * body;
}response_t;

typedef struct head{ // Should members be character types?? Change before/after?
    int  data_length;
    int timeout ;
    //Change to enum
    char *ifexist;
} Header;

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


typedef enum {GIFT, WEASEL, LIST} Mode_t;
extern const char * mode_strs[];
extern const char * header_name[];
typedef enum h_name { DATA_L, TIMEOUT, IF_EXISTS} H_name;

int append_header(char ** header, char * name, char * content);
int finish_headers(char ** headers);

FILE * file_parameters(char *filepath, long int *file_size);
int append_data(FILE* input_file, char** requestbuf, long int size_of_file);

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
	S_CANNOT_SATISFY_REQUEST						= 410,
	S_UNRECOGNISED_ENCODING						
} Status_code;
extern const char * status_descriptions[];
#endif
