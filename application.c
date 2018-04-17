#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "application.h"

const char * mode_strs[] = {"GIFT", "WEASEL", "LIST"};
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
FILE *file_parameters(const char *filepath,long int *size_of_file)
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
		if(size_of_file < 0)
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
	fread(data,1,size_of_file,input_file);
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
//Returns true for valid command, 0 for invalid.
//Sets cmdRx to mode of operation. Stores filepath.
//Increments buff to next alphanumeric character.
bool parse_command(char * buff, Mode * cmdRx, int *index)
{
	int count = 0;
	char * cmdbuff;
	bool valid;

	//printf("first character received = %c\n", *(buff[count]));
	
	while( buff[count++] != ' ');

	count--;

	if(NULL == (cmdbuff = (char *)malloc((count + 1)*sizeof(char))))
    {
		perror("parse command: Error allocating memory\n");    
		exit(EXIT_FAILURE);	
	}
	//printf("buffer pints to = %c, count = %d\n", *(buff[0]), count);
	int i;
	for(i = 0; i < count; i++)
        cmdbuff[i] = buff[i];
	
	//printf("buffer points to = %c, count = %d\n", *buff[0], count);
	cmdbuff[count] = NULLBYTE;

	for(i = 0, valid = false; i < NUM_MODES; i++)
        if(!strcmp(cmdbuff, mode_strs[i]))
        {
            * cmdRx = i;
            valid = true;
        }
	
	//printf("%s_\n", cmdbuff);
	//printf("buffer pionts to = %c\n", *(buff[0]));
	*index += count + 1;
	//printf("buffer pints to = %c\n", *(buff[0]));

	return valid;
} 

int parse_filepath(char * buff, char * filepath, int * index)
{
	int count;
	for(count = 0; buff[count++] != '\n';)

	if(NULL == (filepath = (char *)malloc((count + 1)*sizeof(char))))
	{
		perror("parse filepath: Error allocating memory\n");    
		exit(EXIT_FAILURE);	
	}
	
	for(int i = 0; i < count; i++)
        filepath[i] = buff[i];

    filepath[count] = NULLBYTE;

	buff += count + 1;

	return 0;

}

int parse_header(char * buff, Header * head, int * index)
{
	int count = 0;
	bool valid = false;
	char headbuff[MAX_HEADER_SIZE];

	for(count = 0; buff[count++] != ':';)

	strncpy(headbuff, buff, count);
	headbuff[count] = NULLBYTE;

	buff += count + 1;

	for(int i = 0; i < NUM_HEAD + 1; i++)
            if(!strcmp(headbuff, header_name[i]))
            {
                switch(i)
                {
                    case DATA_L:
                        //Count amount of char in header value
                        for(count = 0; buff[count++] != '\n';)    
                        //Need to read number here
                        (head->data_length) = strtol(buff, NULL, DEC);
                        //Index is now at next header value
						buff += count + 1;
                        valid = true;
                        
                        #ifdef DEBUG
                        printf("reqParse: Data length is %ld bytes\n", (head->data_length));
                        printf("reqParse: Input is valid == %d\n", valid);
                        #endif

                        break;

                    case TIMEOUT:
                        //Count amount of char in header value
                        for(count = 0; buff[count++] != '\n';)    
                        //Need to read number here
                        (head->timeout) = strtol(buff, NULL, DEC);
                        //Index is now at beginning of next header value
                        buff += count + 1;
                        valid = true;
                        
                        #ifdef DEBUG
                        printf("reqParse: Timeout is %ld bytes\n", (head->timeout));
                        printf("reqParse: Input is valid == %d\n", valid);
                        #endif
                        break;

                    case IF_EXISTS:
                        //Count amount of char in header value
                        for(count = 0; buff[count++] != '\n';)    
                        //Need to read number here
                        //(head->ifexist) = strtol(buff, NULL, DEC);
                        //Index is now at beginning of next header value
                        buff += count + 1;
                        valid = true;

                        #ifdef DEBUG
                        //printf("reqParse: If-exists is %ld bytes\n", (head->ifexist));
                        //printf("reqParse: Input is valid == %d\n", valid);
                        #endif
                        break;
                }
            }
        if(!valid){
            perror("head parse: Bad header value\n");
            //return error
        }else{
            if(buff[1] == '\n')
            {
                printf("reqParse: End of headers. Nothing left to process\n");
                //Assign data position pointer value of last header byte
                //Buffer is now at beginning of data
				buff++;
				return 0;
            } 
		
        }
	return 1;
}