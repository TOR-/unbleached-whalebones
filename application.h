#ifndef APPLICATION_H
#define APPLICATION_H

bool verbose;

enum Mode {NONE, GIFT, WEASEL, LIST};
char * mode_strs[] = {"", "GIFT", "WEASEL", "LIST"};

int append_header(char ** header, char * name, char * content);
int finish_headers(char ** headers);

#endif
