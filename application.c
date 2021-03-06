#include <errno.h>
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
	"Connection successful", "Command recognised", "Connection Terminated",		
	" ", " ", " ", " ", " ", " ", " ", "Command Successful", 
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
	"Client error"
	"Command not recognised",
	" ",
	"",
	"Header argument invalid",
	"Header not recognised",
	" ", " ", " ", " ",							//Code 309
	" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", //319
	" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", //329
	" ", " " , " " , " ", " " , " ", " ", " ", " ", " ", //339
	"File not found",
	"Illegal file path",
	" ", " ", " ", " ", " ", " ", " ", " ", 			//349
	" ", " ", " ", " ", " ", " ", " ", " ", " ", " ",	//359
	" ", " ", " ", " ", " ", " ", " ", " ", " ", " ",	//369
	" ", " ", " ", " ", " ", " ", " ", " ", " ", " ",	//379
	" ", " ", " ", " ", " ", " ", " ", " ", " ", " ",	//389
	" ", " ", " ", " ", " ", " ", " ", " ", " ", " ",	//399
	"Server error", " ", "Server write error"
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

/* Returns length of file or EXIT_FAILURE 
 * Opens and closes file at <filepath> */
long int file_length(char *filepath)
{
	FILE* file;
	int length = 0;

	if(verbose) printf("%s: checking length of %s in bytes.\n",
			__FUNCTION__, filepath);
	if((file = fopen(filepath, READ_ONLY)) == NULL )
	{
		fprintf(stderr, "%s:error opening %s: %s",
				__FUNCTION__, filepath, strerror(errno));
		return EXIT_FAILURE;
	}
	else
	{
		fseek(file, 0L, SEEK_END);
		length = ftell(file);
		if(length < 0)
		{
			fprintf(stderr, "%s: file length < 0 (%d < 0) %s.\n", __FUNCTION__, length, strerror(errno));
		} else {
			if(verbose)	
				printf("%s:%s is %d bytes long.\n", __FUNCTION__, filepath, length);
		}
		if(fclose(file) != 0)
			fprintf(stderr, "%s:failed to close %s, %s.\n", 
					__FUNCTION__, filepath, strerror(errno));
		return length;
	}
}

//Function to send data 
int send_data(int sockfd, char * filepath)
{
	long int length = -1;
	long int data_unsent = 0;
	
	FILE * file;
	if((file = fopen(filepath, "rb")) == NULL )
	{	
		fprintf(stderr, "%s:error opening %s: %s.\n",
				__FUNCTION__, filepath, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if(EXIT_FAILURE == (length = file_length(filepath)))
	// No need to print an error here, error printed in file_length
		return length;
	rewind(file);

	data_unsent = length;
	
	if( verbose )
		printf("%s:%ld data bytes to be sent\n", __FUNCTION__, data_unsent);

	char * data_buf = malloc( BUFSIZE_SEND + 1 );
	if( data_buf == NULL)
	{
		fprintf(stderr, "%s:failed to allocate memory for data_buf:%s",
				__FUNCTION__, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if( verbose )
		printf("Data unread = %ld bytes\n", data_unsent);
	int totaltx = 0, read = 0, nTx;
	while( data_unsent >= BUFSIZE_SEND )
	{
		if(0 == (read = fread(data_buf, 1, BUFSIZE_SEND, file)))
			if(ferror(file))
			{
				fprintf(stderr,"%s:error reading from file %s:%s.\n",
						__FUNCTION__, filepath, strerror(errno));
				return EXIT_FAILURE;
			}
		if(-1 == (nTx = send(sockfd, data_buf, BUFSIZE_SEND, 0)))
		{
			fprintf(stderr, "%s:error calling send():%s.\n",
					__FUNCTION__, strerror(errno));
			return EXIT_FAILURE;
		}
		totaltx += nTx;
		data_unsent = data_unsent - nTx;
		if(verbose)
			printf("%s:%d bytes read, %d bytes sent, %d bytes sent total.\n",
					__FUNCTION__, read, nTx, totaltx);
	}
	
	if( data_unsent > 0)
	{
		if(0 == (read = fread(data_buf, 1, data_unsent, file)))
			if(ferror(file))
			{
				fprintf(stderr,"%s:error reading from file %s:%s.\n",
						__FUNCTION__, filepath, strerror(errno));
				return EXIT_FAILURE;
			}
		if(-1 == (nTx = send(sockfd, data_buf, read, 0)))
		{
			fprintf(stderr, "%s:error calling send():%s.\n",
					__FUNCTION__, strerror(errno));
			return EXIT_FAILURE;
		}
		totaltx += nTx;
		if(verbose)// maybe if !verbose
			printf("%s:SUMMARY: total sent: %d.\n", __FUNCTION__, totaltx);
		data_unsent = data_unsent - nTx;
	}
	free(data_buf);

	fclose(file);
	if(data_unsent > 0)		// checks if all data has been sent	
		return(EXIT_FAILURE);
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
 * <index>	pointer to integer that contains index of beginning of current header being processed 
 * used in server.c */
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

void init_header_array(Header_array_t *a, size_t initial) 
{
	a->array = (Header_t *)malloc(initial * sizeof(Header_t));
	if(a->array == NULL)
		fprintf(stderr, "%s:failed to allocate memory for header array:%s.\n",
				__FUNCTION__, strerror(errno));
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
int read_data(char * excess, Process mode_data, char *  filepath, int data_length, int sockfd, int excess_length)
{
	int buffer_size = BUFSIZE_REC;
	int data_unread = 0;
	int nrx;
	FILE * file = NULL;

	if( mode_data == WRITE)
	{
		file = fopen(filepath, "w+");
		if( NULL == file )
		{
			fprintf(stderr, 
					"read_data: failed to open file %s for writing.\n", filepath);
			return(EXIT_FAILURE);
		}
	}

	/*
	if(verbose)
	{
		if(mode_data == PRINT)
			printf("Your boy here printing out da list of all dem files\n");
		if(mode_data == WRITE)
			printf("Your boy here writing the stuff into dat file\n");
	}
	*/
	if(mode_data == PRINT)
		printf("%s", (char *)excess );
	if(mode_data == WRITE) 
		if(!fwrite(excess, 1, excess_length, file))
		{	// This is valid if used in client code 
			//( there is no remainder ∴ excess_length <- 0 )
			if(ferror(file))
				fprintf(stderr, "read_data:failed to write to %s:%s.\n",
						filepath, strerror(errno));
		}
	
	if( data_length > excess_length )
	{
		data_unread = data_length - excess_length;
		char * buf = malloc( buffer_size +1);
		if(verbose && mode_data == PRINT) 
			printf("read_data:%d bytes to read\n", data_unread);
		
		while( data_unread >= buffer_size )
		{
			nrx = recv(sockfd, buf, buffer_size, 0);
			buf[nrx] = '\0';
			data_unread -= buffer_size;

			if( mode_data == PRINT )
				printf("%s", (char *)buf );
			
			if( mode_data == WRITE )
			{
				fwrite(buf, 1, buffer_size, file);
				if(verbose) printf("%s:%d bytes read, %d bytes to write.\n",
						__FUNCTION__, nrx, data_unread);
			}
		}
		// Write any unwritten data
		if( data_unread > 0)
		{
			buffer_size = data_unread;
			nrx = recv(sockfd, buf, buffer_size, 0);
			buf[data_unread] = '\0';
			data_unread = data_unread - buffer_size;

			if( mode_data == PRINT )
				printf("%s", (char *)buf );
			if( mode_data == WRITE )
			{
				fwrite(buf, 1, nrx, file);
				if(verbose) printf("%s:%d bytes read, %d bytes to write.\n",
						__FUNCTION__, nrx, data_unread); 
			}
		}
		free(buf);
	}
  	if(mode_data == WRITE)
		  fclose(file);

	if(data_unread != 0)
		return(EXIT_FAILURE);
	return(EXIT_SUCCESS);
}
