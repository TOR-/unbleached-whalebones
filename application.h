#ifndef APPLICATION_H
#define APPLICATION_H

bool verbose;

enum Mode {NONE, GIFT, WEASEL, LIST};
char * mode_strs[] = {"", "GIFT", "WEASEL", "LIST"};

int append_header(char ** header, char * name, char * content);
int finish_headers(char ** headers);

FILE* file_parameters(const char *filepath, long int *file_size);
int append_data(FILE* input_file, char** requestbuf, long int size_of_file);

#endif
