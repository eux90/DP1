#include "sockwrap.h"
#include "errlib.h"

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#define MSGSZ 255
#define END "close"


typedef int SOCKET;

char *prog_name;

int main(int argc, char *argv[]){
	
	// socket vars
	SOCKET s;
	struct sockaddr_in saddr;
	struct in_addr received_addr;
	socklen_t saddr_len;
	uint16_t port_h;
	
	// data vars
	char w_msg[MSGSZ];
	char r_msg[MSGSZ];
	int n;
	
	// initialize vars
	memset(&saddr, 0, sizeof(saddr));
	memset(&received_addr, 0, sizeof(received_addr));
	memset(&w_msg, 0, MSGSZ * sizeof(char));
	memset(&r_msg, 0, MSGSZ * sizeof(char));
	saddr_len = sizeof(saddr);
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
	saddr.sin_family = 	AF_INET;
	saddr.sin_port = 	htons(port_h);
	saddr.sin_addr		= received_addr;
	
	
	// create socket
	s = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	Connect(s, (SA*)&saddr, saddr_len);
	
	printf("(%s) - Client connected to: %s\n", prog_name, Sock_ntop((SA*)&saddr, saddr_len));
	
	while(strcmp(w_msg, "close") != 0){
		
		printf("(%s) - Enter message for server, (two numbers separated by a space) type \"%s\" to disconnect\n",prog_name, END);
		Fgets(w_msg, MSGSZ-2, stdin);
		// truncates the string just before a \r, \n or \r\n (works on linux windows and mac)
		w_msg[strcspn(w_msg, "\r\n")] = '\0';
		
		// evaluate if user requested to close connection
		if(strcmp(w_msg, END) == 0){
			break;
		}
		
		// send message
		strcat(w_msg,"\r\n");
		n = strlen(w_msg);
		Sendn(s, w_msg, n, 0);
		
		// receive response
		n = Recv(s, r_msg, MSGSZ, 0);
		r_msg[n] = '\0';
		printf("%s", r_msg);
		
		// set buffers to 0 for next iteration
		memset(&w_msg, 0, MSGSZ * sizeof(char));
		memset(&r_msg, 0, MSGSZ * sizeof(char));
	}
	
	Close(s);
	return 0;
}
		 
		
	
	
