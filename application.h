#ifndef APPLICATION_H
#define APPLICATION_H

#define NUM_MODES 3
#define NUM_HEADERS 3

bool verbose;

typedef enum mode {GIFT, WEASEL, LIST} Mode;
const char * mode_strs[] = {"GIFT", "WEASEL", "LIST"};
const char * header_name[] = {"Data-length", "Timeout", "If-exists"};

int append_header(char ** header, char * name, char * content);
int finish_headers(char ** headers);

FILE* file_parameters(const char *filepath, long int *file_size);
int append_data(FILE* input_file, char** requestbuf, long int size_of_file);

#endif
