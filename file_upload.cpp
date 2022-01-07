#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <cstdio>
uint32_t ip_str_to_int(char **) {
	return INADDR_ANY; //listen on any address until we code this
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
	char buffer[1024];

	read(client_fd,buffer,1024);
	
	printf("%s\n",buffer);
	
	close(client_fd);
	close(sock_fd);
	return 0;	
}
