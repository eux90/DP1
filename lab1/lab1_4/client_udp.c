#include "sockwrap.h"
#include "errlib.h"
#include <inttypes.h>
#include <string.h>

#define MSGSZ 32
#define WAIT_SEC 5

typedef int SOCKET;

char *prog_name;

int main(int argc, char *argv[]){
	
	prog_name = argv[0];
	uint16_t port_h;
	struct in_addr received_addr;
	struct sockaddr_in saddr;
	socklen_t saddr_len = sizeof(saddr);
	SOCKET s;
	fd_set rfds;
	struct timeval tv;
	char msg[MSGSZ];
	
	if(argc != 4){
		err_quit("Usage: %s <address> <port> <message (max 31 characters)>", prog_name);
	}
	
	// get address
	Inet_pton(AF_INET, argv[1], &received_addr);
	
	// get port
	if(sscanf(argv[2], "%"SCNu16, &port_h) != 1){
		err_quit("(%s) - invalid port", prog_name);
	}
	
	// get message
	snprintf(msg, MSGSZ, "%s", argv[3]);
	
	// populate structure
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port_h);
	saddr.sin_addr = received_addr;
	
	// create socket
	s = Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	// setup read socket set
	FD_ZERO(&rfds);
	FD_SET(s, &rfds);
	
	// set waiting time
	tv.tv_sec = WAIT_SEC;
	tv.tv_usec = 0;
	
	// send message
	Sendto(s, msg, strlen(msg), 0, (SA *) &saddr, saddr_len);
	
	// wait for response
	int retval = Select(FD_SETSIZE, &rfds, 0, 0, &tv);
	
	// no response exit
	if(retval == 0){
		err_quit("(%s) - no datagram received within %d seconds.", prog_name, WAIT_SEC);
	}
	
	// receive response
	int n = Recvfrom(s, &msg, MSGSZ-1, 0, (SA *) &saddr, &saddr_len);
	// terminate received message
	msg[n] = '\0';
	
	// print received message
	printf("(%s) - received: %s\n", prog_name, msg);
	
	return 0;
}

