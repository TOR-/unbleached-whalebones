#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

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
		"341 Illegal file path",
		" ", " ", " ", " ", " ", " ", " ", " ", 			//349
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ",	//359
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ",	//369
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ",	//379
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ",	//389
		" ", " ", " ", " ", " ", " ", " ", " ", " ", " ",	//399
		"400 Server error", " ", "402 Server write error"
};


/* Appends a header to a <LF> separated list of headers
 * appends <name>:<content>\n
 * returns status code */
int append_header(char ** headers, char * name, char * content)
{
	char * newheaders;
	if(NULL == (newheaders = (char *) malloc(strlen(*headers) + strlen(name) + strlen(content) + 6)))
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

	if( ( input_file = fopen(filepath, READ_ONLY) ) == NULL )
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
int send_data(int sockfd, char * filepath)
{
	long int size_of_file = -1;
	long int data_unsent = 0;
	int nrx;
	
	FILE * file = file_parameters(filepath, &size_of_file);
	
	if( size_of_file == -1 )
	{
		perror("Error opening file");
		return EXIT_FAILURE;
	}
	

	data_unsent = size_of_file;
	
	if( verbose )
		printf("Data Bytes to be sent: %ld", data_unsent);
	if( data_unsent > BUFSIZE )
	{
		char * data_buf = malloc( BUFSIZE + 1 );
		if( data_buf == NULL)
		{
			perror("Error in send_data");
			return EXIT_FAILURE;
		}
		
		if( verbose )
			printf("\nData unread = %ld\n", data_unsent);
		
		while( data_unsent >= BUFSIZE )
		{
			fread(data_buf, 1, BUFSIZE, file);
			nrx = send(sockfd, data_buf, BUFSIZE, 0);
			
			data_unsent = data_unsent - BUFSIZE;
		}
		
		if( data_unsent > 0)
		{
			fread(data_buf, 1, data_unsent, file);
			//buf[data_unread + 1] = '\0';
			nrx = send(sockfd, data_buf, BUFSIZE, 0);
			data_unsent = data_unsent - BUFSIZE;
		}
		free(data_buf);
		
		if(data_unsent != 0)
			return(EXIT_FAILURE);
		
		printf("\nData unread = %ld\n", data_unsent);
	}
	
	fclose(file);
	
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
	// extract all headers from <buff>
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
	*index = pos - buff;
	// all headers extracted

	// Process each header in <headers>
	for(int i = 0; i < NUM_HEAD && (size_t) i < headers.used; i++)	
		if(!strcmp(headers.array[i].name, header_name[i]))
		{
			switch(i)
			{
				case DATA_LENGTH:
					head->data_length = atoi(headers.array[i].value);
#ifdef DEBUG
					printf("parse_header: Data length is %ld bytes\n", (head->data_length));
#endif
					break;
				case TIMEOUT:
					head->timeout = strtol(headers.array[i].value, NULL, DEC);
					//Index is now at beginning of next header value
#ifdef DEBUG
					printf("parse_header: Timeout is %ld bytes\n", (head->timeout));
#endif
					break;
				default:
					return S_HEADER_NOT_RECOGNISED;				
			}
		}
	
	return EXIT_SUCCESS;

	
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

	//Function to read in the data after the headers
	//Returns -1 on failure, 0 on success
	//Two modes: Print, or Write:
int read_data(char * remainder, Process mode_data, char *  filepath, int data_length, int sockfd)
{
	int remainder_length;
	int buffer_size = BUFSIZE;
	int data_unread = 0;
	int nrx;
	FILE * file;
	
	remainder_length = strlen(remainder);
	
	if( mode_data == WRITE)
	{
		errno = 0;
		file = fopen(filepath, "w+");
		if( NULL == file )
		{
			fprintf(stderr, "read_data: failed to open file %s for writing.\n", filepath);
			printf("Error %d \n", errno);
			return(EXIT_FAILURE);
		}
	}

	if(verbose)
	{
		if(mode_data == PRINT)
			printf("Your boy here printing out da list of all dem files\n");
		if(mode_data == WRITE)
			printf("Your boy here writing the stuff into dat file\n");
			printf("Remainder Length: %d\n, Data Length: %d\n", remainder_length, data_length);
	}
	
	if(mode_data == PRINT)
		printf("%s", (char *)remainder );
	if(mode_data == WRITE)
		printf("characters written = %lu rem length = %d\n", fwrite(remainder, 1, remainder_length, file), remainder_length);
	
	if( data_length > remainder_length )
	{
		data_unread = data_length - remainder_length;
		char * buf = malloc( buffer_size + 1 );
		printf("\nData unread = %d\n", data_unread);
		
		while( data_unread >= buffer_size )
		{
			nrx = recv(sockfd, buf, buffer_size, 0);
			if( mode_data == PRINT )
				printf("%s", (char *)buf );
			
			if( mode_data == WRITE )
				fwrite(buf, 1, strlen(buf), file);
			
			data_unread = data_unread - buffer_size;
		}
		
		
		if( data_unread > 0)
		{
			buffer_size = data_unread;
			nrx = recv(sockfd, buf, buffer_size, 0);
			buf[data_unread] = '\0';
			if( mode_data == PRINT )
				printf("%s", (char *)buf );
			if( mode_data == WRITE )
				fwrite(buf, 1, strlen(buf), file);
			data_unread = data_unread - buffer_size;
		}
		free(buf);
			//printf("\nData unread = %d\n", data_unread);
	}
	if(data_unread != 0)
		return(EXIT_FAILURE);
	
	if(mode_data == WRITE)
		fclose(file);
	
	
	return(EXIT_SUCCESS);
}
