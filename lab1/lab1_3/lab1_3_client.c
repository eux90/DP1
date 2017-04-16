#include "sockwrap.h"
#include "errlib.h"

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#define MSGSZ 255


typedef int SOCKET;

char *prog_name;

int main(int argc, char *argv[]){
	
	struct sockaddr_in saddr;
	struct in_addr received_addr;
	uint16_t port_h;
	SOCKET s;
	char msg[MSGSZ];
	socklen_t saddr_len = sizeof(saddr);
	
	prog_name = argv[0];
	
	if(argc != 3){
		err_quit("Usage: %s <address> <port>", prog_name);
	}
	
	// get port
	if(sscanf(argv[2], "%"SCNu16, &port_h) != 1){
		err_quit("(%s) - invalid port", prog_name);
	}
	
	// get and convert address
	Inet_pton(AF_INET, argv[1], &received_addr);
	
	// populate address structure
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = 	AF_INET;
	saddr.sin_port = 	htons(port_h);
	saddr.sin_addr		= received_addr;
	
	
	// create socket
	s = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	Connect(s, (SA*)&saddr, saddr_len);
	
	printf("%s Client connected to: %s\n", prog_name, Sock_ntop((SA*)&saddr, saddr_len));
	
	printf("Enter message for server, (two numbers separated by a space) type \"close\" to disconnect\n");
	
	int len = 0;
	Fgets(msg, MSGSZ-2, stdin);
	// truncates the string just before a \r, \n or \r\n (works on linux windows and mac)
	msg[strcspn(msg, "\r\n")] = '\0';
	while(strcmp(msg, "close") != 0){
		// terminate string with CRLF
		strcat(msg,"\r\n");
		len = strlen(msg);
		Sendn(s, msg, len, 0);
		len = Recv(s,msg,MSGSZ,0);
		msg[len] = '\0';
		printf("Received: %s", msg);
		printf("Enter message for server, (two numbers separated by a space) type \"close\" to disconnect\n");
		Fgets(msg, MSGSZ-2, stdin);
		msg[strcspn(msg, "\r\n")] = '\0';
	}
	
	Close(s);
	return 0;
}
		 
		
	
	
