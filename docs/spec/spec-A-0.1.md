# Proposed DRAFT A v0.1: unbleached whale bones
Stateless - no state preserved on server between requests
Persistent - connection persists until terminated by client or server, i.e. when no more files are to be sent.
Control information sent with data in one message.
Control information in 7 bit ASCII.
Protocol consists of one or more requests to a server, followed by one response per request.

Connection stays open from client's connect() until client has no more data to send.
Connection may be terminated by server if no request is received from the client for a time specified by
the server in the Server-timeout header sent with the previous response.


## Request:
- One request per file.

### Format:
```
<command><SPC><filename><LF>
<header name>:<header value><LF>
	⋯
<header name>:<header value><LF>
<LF>
<data>
```
- ```<command>``` Command that the server is to execute. List of commands given in § COMMAND.
- ```<filename>``` Full name (path + name) of file that the server is to execute ```<command>``` on.
- ```<header name>``` and ```<header value``` : Supplementary information with request. List in § HEADER.
- ```<data>``` Should there be data necessary to transmit, this is where it goes.

## Response:
- One response per file.
### Format:
```
<status number><SPC><status message><LF>
<header name>:<header value><LF>
	⋯
<header name>:<header value><LF>
<LF>
<data>
```
- ```<status number>``` and ```<status message>``` A numerical code and a corresponding text description detailing how the server fared with processing the request.
- ```<header name>``` and ```<header value>``` See § Request.
- ```<data>``` If data was requested, this is where it goes.

## Transaction Format:
- Client connects to server
- Server accepts
- Client sends request message

## Definitions:
### COMMAND:

```GIFT<SPC><filepath>```  
Gift the server with a file located at ```<filepath>```. (PUT)
- Requires the ```Data-length``` header  
- Requires the ```If-exists``` header  
  
```WEASEL<SPC><filepath>```  
Weasel a file located at ```<filepath>``` from the server. (GET)  
```LIST<SPC><filepath>```  
List all files located at ```<filepath>```.  

### STATUS:

#### 1xx successful transaction  
100 Connection successful  
101 Command recognised  
102 Connection terminated  
110 Command successful  
  
#### 2xx informational  
200 Command not implemented  
201 Server preparing for shutdown  
210 Username OK, need password  
  
#### 3xx client error  
300 Command not recognised  
301 Command nonsensical at this time  
302 Command incomplete  
303 Command too divilish  
310 Not logged in  
311 Insufficiently privileged  
312 Credentials incorrect  
320 Timeout  
321 Server got bored and left this conversation  
330 Unplugged  
340 File not found  
  
#### 4xx server error  
400 Command not implemented  
410 Cannot satisfy request  
411 Unrecognised encoding  
499 Unspecified server error  
  
## HEADER:
```Data-length:<length>```  
Length of data portion of message  
```Data-type:<type of response data>```  
Filetype of data portion of message  
Preferably this would be a MIME type  
```Timeout: <max waiting time>```  
Length of time server/client will wait for next response before closing connection  
```If-exists: <action>```  
Action for server to take if the file exists  
Either ```overwrite``` or ```skip```  

# Proposed DRAFT A v0.2
## New headers:
```Data-encoding:<encoding>```  
Encoding of data portion  
```Date:<%Y-%m-%dT%H:%M:%SZ>```  
ISO 8601 UTC+0 time and date of message sending  
```Last-modified:<%Y-%m-%dT%H:%M:%SZ>```  
ISO 8601 UTC+0 time and date of last modification to file  
Used to enable caching  
Possibly unnecessary  
```Preferred-encoding:<encoding>[<COMMA><encoding>...]```  
List of encodings the client can handle, in order of preference.  

## New commands:  
```REQUEST<SPC><LF>```  
Send a query to check if enough space is available, i.e. by including a header ```Query-length:<length>``` to query for a  ```<length>``` amount of bytes.  
Could be expanded to query multiple things at once.  
