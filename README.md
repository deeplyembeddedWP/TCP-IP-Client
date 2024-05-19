# TCP/IP Client
A simple tcp/ip client that sends a request containing the filename to the server to be transferred to the client

## Project structure
At present, all the source files are within a single directory called **Src** to keep things simple. Though at some point it'd be better to split the header and source files into two seperate folders e.g. *include* and *src*.

## Client Configuration
In **main.h**
```
#define PORT 12345                        // Server port to connect to
#define FILE_NAME_SIZE_MAX 32 + 1         // Maximum size supported for the requested filename
#define FILE_DOWNLOAD_TABLE "/home/vinay_divakar/file_download" // file download path
#define FILE_DOWNLOAD_PATH_NAME_SIZE_MAX 64 // Maximum size supported for the file path
```

## Building the project
1. In the Makefile, update the *SRCDIR* variable to point the path project is located on your local machine.
```
SRCDIR = /home/vinay_divakar/tcp-ip-client/src
```
2. Run **make** from within the project's root i.e.*tcp-ip-client* to build your project. This will generate an executable called *client_app*.
   
## Running the application
1. Run the *client_app* executable with the filename to request for. Ensure the file name is not greater than FILE_NAME_SIZE_MAX. And application only supports a single input arg.
```
vinay_divakar@vinay-divakar-Linux:~/Server$ ./client_app <filename> 
```
3. For debug purposes or visiblity, you can enable/uncomment the below line in *main.c* within the function *file_download()*. This prints what's being received from the sever.
```
// enable to whats being received from the server
// printf("recv: %.*s\r\n", err, (char *)buffer);
``` 

## How it works
The client does the follow:

1. Reads the <filename> arg from command-line.
2. Sets up the socket and connects to the server.
3. Sends the request to the server i.e. <filename>.
4. Configures the socket to recieve in non-blocking mode and enable receiving POLLIN events.
5. Polls of socket read events.
6. On a read event, receives the data and prints it on the terminal(if enabled).
7. **TBD:** store the contents to the file / download.

## Things to be implemented
1. *Download Receievd File Contents*
   
The receievd file contents from the server needs to written to a file for download on the loacal machine. On receiving the first response from the server, create a file with the requested <filename> and write the contents to it with start offset 0. On a successful writes, add the number of bytes written to the file to a <variable> which can be used as start offset for subsequent writes. Keep doing this until we have received the eof indication from the server.
*Another better way about this is to get the size of the file to be download from the server and use that to determine the progress of the download.*

2. *Tidy up*
   
Functions are all over the place in main and needs to be tidied up and better organized.   
