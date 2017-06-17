#include <stdio.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#include "errlib.h"
#include "my_sockwrap.h"

typedef int SOCKET;


char *prog_name;


int main(int argc, char *argv[]){
	
	
	struct sockaddr_in saddr;
	struct in_addr received_addr;
	socklen_t saddr_len;

	SOCKET s;
	uint16_t port_h;
	
	// initialize variables
	memset(&saddr, 0, sizeof(saddr));
	memset(&received_addr, 0, sizeof(received_addr));
	saddr_len = sizeof(saddr);
	prog_name = argv[0];
	
	
	if(argc != 3){
		err_quit("Usage: %s <address> <port>", prog_name);
	}
	
	// get port from input parameters
	if(sscanf(argv[2], "%"SCNu16, &port_h) != 1){
		err_quit("(%s) - Invalid port");
	}
	
	// get and convert address from input parameters
	Inet_pton(AF_INET, argv[1], &received_addr);
	
	// populate socket structure
	saddr.sin_family 	= AF_INET;
	saddr.sin_port		= htons(port_h);
	saddr.sin_addr		= received_addr;
	
	// create socket
	s = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	Connect(s, (SA*)&saddr, saddr_len);
	
	printf("%s Client connected to: %s", prog_name, Sock_ntop((SA*)&saddr, saddr_len));
	
	sleep(2);
	
	close(s);
	
	return 0;
}
	
	
	
	
	
	
	
