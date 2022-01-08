#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <string.h>
#include <cstdio>
#include <fcntl.h>
//for file names
#include <time.h>
#define BUFFER_SIZE 1024

uint32_t ip_str_to_int(char **) {
	return INADDR_ANY; //listen on any address until we code this
}

enum ACTION {
	ACTION_GET_UPLOAD_PAGE=0,
	ACTION_UPLOAD_FILE,
	ACTION_UNDEFINED //we were asked to do an unimplimented feature
};

//VEEEEERY basic http header parser that does ONE thing and only one thing
//in an effort to minimize BUUUGZ in an already very bootleg setup
int parseHTTPAction(char * buffer,int buffer_size) {
	//if we have a decently sized buffer
	if (buffer_size >= 16) {
		if (memcmp(buffer,"GET",3) == 0) {
			return ACTION_GET_UPLOAD_PAGE;
		}
		if (memcmp(buffer,"POST",4) == 0) {
			if (memcmp(buffer + 5,"/upload",7) == 0)
				return ACTION_UPLOAD_FILE;	
		
		}
	}
	return ACTION_UNDEFINED;
}
//TODO:
//this makes me nervous because we dont have a size argument for fname
//need to look at preventing BOF here
void writeClientFile(int clientfd,int fd) {
	char * buffer[1024];
	int bytes_read = read(fd,buffer,1024);
	while (bytes_read != 0) {
		write(clientfd,buffer,bytes_read);
		bytes_read = read(fd,buffer,1024);
	}
}
enum HTTP_HEADER_TYPE {
	HTTP_T_OK=200
};
void getHeaderCodeStr(int header_code,char * str,int str_size) {
	switch (header_code) {
		case HTTP_T_OK:
			if (str_size >= 3)
				memcpy(str,"OK",3);
		break;
	}

}
int sendHTTPHeader(int client_fd,int header_code) {
	char header_code_str[32];
	getHeaderCodeStr(header_code,header_code_str,32);
	dprintf(client_fd,"HTTP/1.1 %d %s\r\n",header_code,header_code_str);
	return 0;
}
/*
 * seeks a html stream to the first line in the body
 */
void seekStreamToBody(int streamfd) {
	uint32_t bytes_read = 4;
	char readData[4];
	read(streamfd,readData,4);
	while (memcmp(readData,"\r\n\r\n",4) != 0 && bytes_read <= 2048) {
		read(streamfd,readData,4);
		bytes_read += 4;
	}
}
//returns all of the bytes up to the given delimiter 
int readToDilimeter(int fd,char *outbuffer,int buffer_size,char * delimiter,int del_size) {	
	int bytes_read = read(fd,outbuffer,del_size);
	int buffer_bytes_read = bytes_read;
	if (bytes_read < del_size)
		return -2;
	if (buffer_bytes_read == 0) {
		return 0;
	}
	while (memcmp(outbuffer+bytes_read-del_size,delimiter,del_size) != 0) {
		if (!(bytes_read < buffer_size))
			return -1;	
		buffer_bytes_read = read(fd,outbuffer+bytes_read,1);	 
		if (buffer_bytes_read <= 0) {	
			return bytes_read;
		}
		bytes_read += buffer_bytes_read;
	}
	return bytes_read;
}
/*
 * reads a line defined by \r\n into the specified buffer and returns the number of bytes read
 * returns -1 if the buffer would have been overflowed
 */
int readWebLine(int fd,char * buffer,int buffer_size) {
	char delim[2] = {'\r','\n'};
	return readToDilimeter(fd,buffer,buffer_size,delim,2);
}
//reads into the buffer until \r\n\r\n is detected or the buffer size is overblown
//returns the number of bytes read
int readWebDoubleNewLine(int fd,char * buffer,int buffer_size) {
	char delim[4] = {'\r','\n','\r','\n'};
	return readToDilimeter(fd,buffer,buffer_size,delim,4);
}
//returns the index of the given key in the given buffer
//-1 if the key does not exist
int checkBufferForKey(char * buffer,int buffer_size,char * key,int key_size) {
	for (int i = 0; i < buffer_size-key_size; i++) {
		if (memcmp(buffer+i,key,key_size) == 0)
			return i;
	}
	return -1;
}
/*
 * this function responds to the client with the specified http resonce
 * and serves them the given file
 */
void writeClientHTTPFile(int client_fd,int file_fd,int http_code) {
	sendHTTPHeader(client_fd,http_code);
	write(client_fd,"\r\n",2);
	writeClientFile(client_fd,file_fd);
}
void writeClientFileFromName(char * fname,int client_fd,int http_code) {
	int file_fd = open(fname,O_RDONLY);
	writeClientHTTPFile(client_fd,file_fd,HTTP_T_OK);
	close(file_fd);
}
int main(int argc, char ** argv) {
	printf("working program\n");
	
	//the socket file description can be thought of as "THE" socket
	int sock_fd = socket(AF_INET,SOCK_STREAM,0);
	//the size of our address data type used in later calls
	//this is just a fancy unsigned int that might not be an unsigned it on really old systems 
	socklen_t addr_len = sizeof(sockaddr_in);
	//the actual address of the socket
	struct sockaddr_in sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(2644);
	sock_addr.sin_addr.s_addr = INADDR_ANY;

	bind(sock_fd,(struct sockaddr *) &sock_addr,sizeof(sock_addr));
	
	//set the socket up to listen
	listen(sock_fd,2);
	
	//initilize variables used in the main loop
	int client_fd,file_fd;


	//this is the main loop of the server
	//TODO: safely close the sockets on sig interupt
	while (true) {
		//re-use the sock_addr memory to store the addr
		//of the computer connecting to us
		//as we will not need that information saved again :D
		
		client_fd = accept(sock_fd,(struct sockaddr *) & sock_addr,&addr_len);
		printf("[*] client connected\n");
	
		char buffer[BUFFER_SIZE];	
		readWebDoubleNewLine(client_fd,buffer,BUFFER_SIZE);

		switch (parseHTTPAction(buffer,BUFFER_SIZE)) {
			case ACTION_GET_UPLOAD_PAGE:
				printf("[*]\tclient requesting web page\n");	
				writeClientFileFromName("./index.html",client_fd,HTTP_T_OK);	
				break;
			case ACTION_UPLOAD_FILE:
				printf("[*]\tclient uploading file\n");

				int to_write = open("./uploads/test",O_CREAT | O_WRONLY | O_APPEND);	
				int borderLength = readWebLine(client_fd,buffer,BUFFER_SIZE);
				
				//this will seek to the data 
				readWebDoubleNewLine(client_fd,buffer+borderLength,BUFFER_SIZE-borderLength);
				

				char dataBuffer[1024];
				int data_bytes_read = read(client_fd,dataBuffer,1024);
				int key_index = checkBufferForKey(dataBuffer,1024,buffer,borderLength);	
				while (key_index < 0 && data_bytes_read > 0) {
					write(to_write,dataBuffer,1024);
					int data_bytes_read = read(client_fd,dataBuffer,1024);
					key_index = checkBufferForKey(dataBuffer,1024,buffer,borderLength);	
				}
				write(to_write,dataBuffer,key_index-2);
				close(to_write);
				
				//tell the client we finished
				writeClientFileFromName("./up.html",client_fd,HTTP_T_OK);		
				break;
		}
		printf("[*]\tclosing client connection\n");	
		close(client_fd);
	}
	//close the listening socket
	close(sock_fd);
	return 0;	
}
