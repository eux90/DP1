#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <string.h>


#include "errlib.h"
#include "sockwrap.h"

#define LISTENQ 5



char *prog_name;


int main(int argc, char *argv[]){
	
	
	struct sockaddr_in saddr, caddr;
	uint16_t port_h;
	socklen_t saddr_len = sizeof(saddr);
	socklen_t caddr_len = sizeof(caddr);
	int listen_fd, conn_fd;
	ssize_t received;
	char data;
	
	
	
	prog_name = argv[0];
	
	if(argc != 3){
		err_quit("Usage: %s <addr> <port>\n", prog_name);
	}
	
	// clean socket structure
	memset(&saddr, 0, sizeof(saddr));
	
	// set socket type
	saddr.sin_family = AF_INET;
	
	// get port
	if(sscanf(argv[2], "%" SCNu16, &port_h) != 1){
		err_sys("Wrong port number\n");
	}
	
	// convert and set port
	saddr.sin_port = htons(port_h);
	
	// get convert and set address
	Inet_pton(AF_INET, argv[1], &(saddr.sin_addr));

	
	printf("Address: %s\n", Sock_ntop((SA*)&saddr, saddr_len));
	
	// create socket
	listen_fd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	Bind(listen_fd, (SA*)&saddr, saddr_len);
	
	Listen(listen_fd, LISTENQ);
	
	while(1){
		
		conn_fd = accept(listen_fd, (SA*)&caddr, &caddr_len);
		if(conn_fd < 0){
			err_ret("(%s) error - accept failed", prog_name);
		}
		else{
			printf("(%s) new connection from client: %s\n",prog_name, Sock_ntop((SA*)&caddr, caddr_len));
		}
		
		for(;;){
			
			received = recv(conn_fd, &data, sizeof(data), 0);
			if(received == 0){
				printf("Connection closed by client\n");
				close(conn_fd);
				break;
			}
			if(received < 0){
				printf("Read error, socket closed\n");
				close(conn_fd);
				break;
			}
			else{
				if(data != '\r' && data != '\n')
					printf("Received: %c\n", data);
			}
		}
	}
	
	return 0;
}
