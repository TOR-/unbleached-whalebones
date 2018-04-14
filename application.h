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

typedef struct{
	char * name;
	char * value;
}Header;

typedef struct {
  Header *array;
  size_t used;
  size_t size;
} Header_array_t;

void init_header_array(Header_array_t *a, size_t initial);
void insert_header_array(Header_array_t *a, Header element);
void free_header_array(Header_array_t *a);


typedef enum {GIFT, WEASEL} Mode_t;
extern const char * mode_strs[];
extern const char * header_name[];
typedef enum h_name { DATA_L, TIMEOUT, IF_EXISTS} H_name;

int append_header(char ** header, char * name, char * content);
int finish_headers(char ** headers);

FILE * file_parameters(char *filepath, long int *file_size);
int append_data(FILE* input_file, char** requestbuf, long int size_of_file);

#endif
