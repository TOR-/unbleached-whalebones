#ifndef APPLICATION_H
#define APPLICATION_H

#define NUM_MODES 3
#define NUM_HEADERS 3

bool verbose;

enum Mode {GIFT, WEASEL, LIST};
char * mode_strs[] = {"GIFT", "WEASEL", "LIST"};
char * header_name[] = {"Data-length", "Timout", "If-exists"};

enum status_num {0, 2};
const char *status_strings{"swag", "yolo"};

int append_header(char ** header, char * name, char * content);
int finish_headers(char ** headers);

#endif
