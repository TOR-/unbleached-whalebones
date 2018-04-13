#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdbool.h>

#define READ_ONLY "r"
#define DEBUG

#define END_HEAD ':'
#define NUM_MODES 3
#define NUM_HEADERS 3
#define MAX_HEADER_SIZE 20

bool verbose;

typedef enum {GIFT, WEASEL} Mode_t;
extern const char * mode_strs[];
extern const char * header_name[];
typedef enum h_name { DATA_L, TIMEOUT, IF_EXISTS} H_name;

int append_header(char ** header, char * name, char * content);
int finish_headers(char ** headers);

FILE * file_parameters(char *filepath, long int *file_size);
int append_data(FILE* input_file, char** requestbuf, long int size_of_file);

#endif
