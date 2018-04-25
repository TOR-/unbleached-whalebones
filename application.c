#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "application.h"

const char * mode_strs[] = {"GIFT", "WEASEL", "LIST"};
const char * header_name[] = {"Data-length", "Timeout", "If-exists"};
const char *status_descriptions[] = 
{
								
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", 	  //1-9
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", //10-19
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", //20-29
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", //30-39
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", //40-49
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", //50-59
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", //60-69
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", //70-79
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", //80-89
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " " //90-99
	,
								
		"101 Command recognised", " ",
		" ", " ", " ", " ", " ", " ", " ", " ", 
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", 
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", 
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", 
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", 
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ",
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", 
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", 
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", 
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " " //199
	,
								
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", 
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", 
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", 
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", 
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", 
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", 
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", 
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", 
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", 
		" ", " ", " ", " ", " ", " ", " " , " ", " ", " " //299
	,
	
		"301 Command not recognised",
		" ",
		"",
		"304 Header argument invalid",
		"305 Header not recognised",
		" ", " ", " ", " ",							//Code 309
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", //319
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", //329
		" ", " " , " " , " ", " " , " ", " ", " ", " ", " ", //339
		"340 File not found",
		"341 Illegal file path"
	
};


/* Appends a header to a <LF> separated list of headers
 * appends <name>:<content>\n
 * returns status code */
int append_header(char ** headers, char * name, char * content)
{
	char * newheaders;
	if(NULL == (newheaders = (char *) malloc(strlen(*headers) + strlen(name) + strlen(content) + 3)))
		// +3 because of ':', '\n', '\0'
	{
		fprintf(stderr, "append-header: failed to allocate memory for header.\n");
		return EXIT_FAILURE;
	}
	if(sprintf(newheaders, "%s%s:%s\n", *headers, name, content) <= 0)
	{
		fprintf(stderr, "append-header: failed to add header.\n");
		return EXIT_FAILURE;
	}

	char * headers_tmp = (char *) realloc(*headers, strlen(newheaders) + 1);
	if(NULL == headers_tmp)
	{
		fprintf(stderr, "append-header: failed to allocate memory for header.\n");
		return EXIT_FAILURE;
	}
	*headers = headers_tmp;
	strcpy(*headers, newheaders);
	free(newheaders);
	return EXIT_SUCCESS;
}

/* Terminates header portion of message by appending a */
int finish_headers(char ** headers)
{
	char * newheaders;
	if(NULL == (newheaders = (char *) realloc(*headers, strlen(*headers) + 2)))
	{
		fprintf(stderr, "finish_headers: failed to reallocate memory for headers.\n");
		return EXIT_FAILURE;
	}
	if(NULL == (*headers = strncat(newheaders, "\n", 1)))
	{
		fprintf(stderr, "finish_headers: failed to finish headers.\n");
		return EXIT_FAILURE;
	}
	//*headers = newheaders;
	return EXIT_SUCCESS;
}

//Function to check the parameters of a file
//Returns NULL if file does not exist, or if there is an error in reading the file
FILE *file_parameters(char *filepath, long int *size_of_file)
{
	FILE* input_file;

	if( ( input_file = fopen(filepath, READ_ONLY) )== NULL )
	{
		if(verbose)
			printf("File does not exist\n");
		return input_file;
	}
	else
	{
		if(verbose)
			printf("Checking file parameters.\n");
		fseek(input_file, 0L, SEEK_END);
		*size_of_file = ftell(input_file);
		if(verbose)
			printf("File is %ld bytes long.\n", *size_of_file);
		if(*size_of_file < 0)
		{
			perror("Error in file_parameters: File size is less than zero: ");
			return NULL;
		}

		rewind(input_file);
	}

	return input_file;
}

//Function to append the data to the request
int append_data(FILE* input_file, char** request_buf, long int size_of_file)
{	
	long int header_length = strlen(*request_buf) + 1;

	uint8_t* data = malloc(size_of_file + 1);
	if(data == NULL)
	{
		perror("Error appending data: ");
		return EXIT_FAILURE;
	}
	data[size_of_file] = '\0';
	// Reallocate memory to account for the headers
	// Should be rewritten with temporary pointer to prevent losing access 
	// 	to memory
	*request_buf = (char *) realloc(*request_buf, size_of_file + header_length);
	if(request_buf == NULL)
	{
		perror("Error appending data: ");
		return EXIT_FAILURE;
	}

	//Read in data from the file
	if(size_of_file != fread(data, 1, size_of_file, input_file))
	{
		fprintf(stderr, "append_data: error in reading the file: %s", 
				strerror(ferror(input_file)));
		return EXIT_FAILURE;
	}
	strcat(*request_buf, (char *)data);
	free(data);
	return EXIT_SUCCESS;
}

//Returns true for valid command, 0 for invalid.
//Sets cmdRx to mode of operation. Stores filepath.
//Increments buff to next alphanumeric character.
bool parse_command(char * buff, Mode_t * cmdRx, int * index)
{
	int count = 0;
	char * cmdbuff;
	bool valid;
	
	while( buff[count++] != ' ');

	count--;

	if(NULL == (cmdbuff = (char *)malloc((count + 1)*sizeof(char))))
    {
		perror("parse command: Error allocating memory\n");    
		exit(EXIT_FAILURE);	
	}
	
	int i;
	for(i = 0; i < count; i++)
        cmdbuff[i] = buff[i];
	
	cmdbuff[count] = NULLBYTE;

	for(i = 0, valid = false; i < NUM_MODE; i++){
        if(!strcmp(cmdbuff, mode_strs[i]))
        {
            * cmdRx = i;
            valid = true;
        }
	}
	*index += count + 1;

	return valid;
} 

int parse_filepath(char * buff, char ** filepath, int * index)
{
	int count;
	for(count = 0; buff[*index + count] != '\n'; count++);

	if(NULL == (*filepath = (char *)malloc((count + 1)*sizeof(char))))
	{
		perror("parse filepath: Error allocating memory\n");    
		exit(EXIT_FAILURE);	
	}
	
	for(int i = 0; i <= count; i++){
        (*filepath)[i] = buff[(*index) + i];
	}
	
	(*filepath)[count] = NULLBYTE;

	*index += count + 1;

	return 0;

}

/* Populates fields in <header> with first header found in <buf>
 * Returns pointer to the end of the header + 1 (skip \n) or NULL if not found */
char * extract_header(char * buf, Header_array_t * header_array, bool * finished)
{
	char * sep = buf, * term = buf; // Position of substring in string
	Header_t header;
	if(*buf == HEADER_TERMINATOR)
	{
		*finished = true;
		return buf + 1;
	}
	if(NULL == (sep = strchr(buf, HEADER_SEPARATOR))
			|| NULL == (term = strchr(buf, HEADER_TERMINATOR)))
	{
		fprintf(stderr, "extract_header: header not found in >>%s<<.\n", buf);
		return NULL;
	}
	if(NULL == (header.name = strndup(buf, sep - buf))
			|| NULL == (header.value = strndup(sep + 1, term - sep - 1)))
	{
		fprintf(stderr, "extract_header: no memory for header %s.\n", buf);
		return NULL;
	}
	if(verbose) printf("extract_header: header %s read, value %s.\n",
			header.name, header.value);
	insert_header_array(header_array, header);
	return term + 1;
}

/* <buff>	points to first header in received buffer
 * <head>	pointer to Header structure to populate wth parsed values 
 * <index>	pointer to integer that contains index of beginning of current header being processed */
int parse_header(char * buff, Header * head, int * index)
{
	// !!!!!!!!!!!!!!!!!!!TODO remove this
	verbose = true;
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	int count = 0;
	bool valid = false;
	/*
		char * headbuff = calloc(1, MAX_HEADER_SIZE+1);
		if(NULL == headbuff)
		{
			fprintf(stderr, "%s: couldn't allocate memory for header.\n", 
					__FUNCTION__);
			return EXIT_FAILURE;
		}
		for(count = 0; buff[*index + count] != ':'; count++){
			printf("[%c(%d)]", headbuff[count], count);
			headbuff[count] = buff[*index + count];
		}
		headbuff[count++] = NULLBYTE;
		
		*index += count;
		buff = buff + *index;
	*/
	// Extract headers
	Header_array_t headers;
	bool headers_finished;
	int n_headers;
	init_header_array(&headers, HEADERINITBUFLEN);

	char * next_start = buff + *index, * pos = buff + *index;
	for(n_headers = 0, headers_finished = false; false == headers_finished && NULL != pos; n_headers++)
	{
		// headers not finished, header was found
		// save position of last terminator found in headers
		next_start = pos;
		pos = extract_header(next_start, &headers, &headers_finished);
	}
	if(verbose) printf("%s:headers finished.\n", __FUNCTION__);
	
	// Process each possible header
	for(int i = 0; i < NUM_HEAD && (size_t) i < headers.used; i++)	
		if(!strcmp(headers.array[i].name, header_name[i]))
		{
			switch(i)
			{
				case DATA_LENGTH:
					head->data_length = atoi(headers.array[i].value);
					valid = true;

#ifdef DEBUG
					printf("parse_header: Data length is %ld bytes\n", (head->data_length));
					printf("parse_header: Input is valid == %d\n", valid);
#endif
					break;
				case TIMEOUT:
					// TODO convert this function
					//Count amount of char in header value
					for(count = 0; buff[count++] != '\n';)    
						//Need to read number here
						(head->timeout) = strtol(buff, NULL, DEC);
					//Index is now at beginning of next header value
					buff += count + 1;
					*index += count;
					valid = true;

#ifdef DEBUG
					printf("parse_header: Timeout is %ld bytes\n", (head->timeout));
					printf("parse_header: Input is valid == %d\n", valid);
#endif
					//printf("buff points to = %c\n", buff[0]);
					break;
			}
		}
	if(!valid){
		perror("head parse: Bad header value\n");
		return S_HEADER_NOT_RECOGNISED;
	}
	else
	{
		printf("%s(%d)\n", __FUNCTION__, __LINE__);
		if(buff[*index] == '\n')
		{
			printf("parse_header: End of headers. Nothing left to process\n");
			(*index) = (*index) + 1;// start of data index
			return 0;
		} 

	}
	return EXIT_FAILURE;
}

// TODO implement error checking here
void init_header_array(Header_array_t *a, size_t initial) 
{
	a->array = (Header_t *)malloc(initial * sizeof(Header_t));
	a->used = 0;
	a->size = initial;
}

void insert_header_array(Header_array_t *a, Header_t element) 
{
	// a->used is the number of used entries, because a->array[a->used++] 
	// updates a->used only *after* the array has been accessed.
	// Therefore a->used can go up to a->size
	if (a->used == a->size) {
		a->size *= 2;
		a->array = (Header_t *)realloc(a->array, a->size * sizeof(Header_t));
	}
	a->array[a->used++] = element;
}

void free_header_array(Header_array_t *a) 
{
	free(a->array);
	a->array = NULL;
	a->used = a->size = 0;
}
