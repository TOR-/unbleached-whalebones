#ifndef APPLICATION_H
#define APPLICATION_H
#define READ_ONLY "r"
#define DEBUG

#define END_HEAD ':'
#define NUM_MODES 3
// REMEMBER TO CHANGE THIS BACK
#define NUM_HEAD 3//(sizeof(header_name)/sizeof(*header_name) )
#define MAX_HEADER_SIZE 20
#define DEC 10  //Number base for use with strtol
#define NULLBYTE '\0'

#include <stdbool.h>

bool verbose;

typedef enum { DATA_L, TIMEOUT, IF_EXISTS} H_name;
typedef enum {GIFT, WEASEL, LIST} Mode;
typedef enum {ALLOC_FAIL = -1, } Error;

typedef struct head{ // Should members be character types?? Change before/after?
    long int  data_length;
    long int timeout ;
    //Change to enum
    //char *ifexist;
    //char * data_pos;
} Header;

typedef struct req{
    Mode cmdRx;
    char *filepath;
    Header *header;
} Request;

int append_header(char ** header, char * name, char * content);
int finish_headers(char ** headers);

//FILE* file_parameters(const char *filepath, long int *file_size);
//int append_data(FILE* input_file, char** requestbuf, long int size_of_file);

bool parse_command(char * buff, Mode * cmdRx, int * index);
int parse_filepath(char * buff, char ** filepath, int * index);
int parse_header(char * buff, Header * head, int * index);

#endif
