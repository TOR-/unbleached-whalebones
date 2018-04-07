# Client Side:
## Identify Source File Parameters:
- Ensure it exists!
- Check size of source file

# Create the header
## Format:
```
GIFT<SPACE><filename><LF>
Data-length:<length><LF>
<optional header name>:<value><LF>
	...
<optional header name>:<value><LF>
<LF>
```
where:
- ```<LF>``` is the '\n' character
- ```<filename>``` is the name of the file being transferred
- ```<length>``` is the number of bytes in the file
- ```<optional header name>``` is the name of a header that is not necessary but possibly informational
- ```<value>``` is the value of the header ```<optional header name>```

# Read in file as blob

# Append blob to HEADER

## Format of final message:
```
GIFT<SPACE><filename><LF>
Data-length:<length><LF>
<optional header name>:<value><LF>
			...
<optional header name>:<value><LF>
<LF>
<data bytes>
```
where:
- ```<data bytes>``` is the array of data bytes to be sent

# Send big blob to Server
