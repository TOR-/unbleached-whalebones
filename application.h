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

#include <stdbool.h>

bool verbose;

typedef enum h_name { DATA_L, TIMEOUT, IF_EXISTS} H_name;
typedef enum mode {GIFT, WEASEL, LIST} Mode;
extern const char * mode_strs[];
extern const char * header_name[];

int append_header(char ** header, char * name, char * content);
int finish_headers(char ** headers);

FILE* file_parameters(const char *filepath, long int *file_size);
int append_data(FILE* input_file, char** requestbuf, long int size_of_file);

#endif
