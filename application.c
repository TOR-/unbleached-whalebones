#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "application.h"

const char * mode_strs[] = {"GIFT", "WEASEL"};
const char * header_name[] = {"Data-length", "Timeout", "If-exists"};

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
	if(0 >= sprintf(newheaders, "%s\n", *headers))
	{
		fprintf(stderr, "finish_headers: failed to finish headers.\n");
		return EXIT_FAILURE;
	}
	*headers = newheaders;
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
			printf("Checking file parameters");
		fseek(input_file, 0L, SEEK_END);
		*size_of_file = ftell(input_file);
		if(verbose)
			printf("File is %ld bytes long", *size_of_file);
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
	int check_for_end = 0;
	size_t line_length = 0;
	
	long int header_length = strlen(*request_buf);
	
	char* data = (char *) malloc(size_of_file);
	if(data == NULL)
	{
		perror("Error appending data: ");
		return EXIT_FAILURE;
	}
		//Reallocate memory to account for the headers
	*request_buf = (char *) realloc(*request_buf, size_of_file + header_length);
	if(request_buf == NULL)
	{
		perror("Error appending data: ");
		return EXIT_FAILURE;
	}
	
	//Read in data from the file
	check_for_end = fread(data,1,size_of_file,input_file);
	strcat(*request_buf,data);
	
	if(data == NULL)
	{
		printf("Error in reading the file");
		return EXIT_FAILURE;
	}

	free(data);
	return 0;
	
	return EXIT_SUCCESS;
}
