#ifndef APPLICATION_H
#define APPLICATION_H

#define NUM_MODES 3
#define NUM_HEADERS 3

bool verbose;

typedef enum mode {GIFT, WEASEL, LIST} Mode;
const char * mode_strs[] = {"GIFT", "WEASEL", "LIST"};
const char * header_name[] = {"Data-length", "Timout", "If-exists"};

int append_header(char ** header, char * name, char * content);
int finish_headers(char ** headers);

#endif
