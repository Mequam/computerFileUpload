#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <string.h>
#include <cstdio>
#include <fcntl.h>
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
			//5 letters in and we get to the path of a propper html packet
			//if you malform the packet, that sucks you get the undefined action response
			if (memcmp(buffer + 4,"/upload",7) == 0)
				return ACTION_UPLOAD_FILE;	
			return ACTION_GET_UPLOAD_PAGE;
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
	sock_addr.sin_port = htons(2645);
	sock_addr.sin_addr.s_addr = INADDR_ANY;

	bind(sock_fd,(struct sockaddr *) &sock_addr,sizeof(sock_addr));
	
	//set the socket up to listen
	listen(sock_fd,2);

	//re-use the sock_addr memory to store the addr
	//of the computer connecting to us
	//as we will not need that information saved again :D
	
	int client_fd = accept(sock_fd,(struct sockaddr *) & sock_addr,&addr_len);

	//just some basic test stuff to start out
	char buffer[BUFFER_SIZE];

	read(client_fd,buffer,BUFFER_SIZE);

	switch (parseHTTPAction(buffer,BUFFER_SIZE)) {
		case ACTION_GET_UPLOAD_PAGE:
			int local_fd = open("./index.html",O_RDONLY);
			sendHTTPHeader(client_fd,HTTP_T_OK);
			write(client_fd,"\r\n",2);
			writeClientFile(client_fd,local_fd);	
			break;
	}
	
	close(client_fd);
	close(sock_fd);
	return 0;	
}
