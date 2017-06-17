#include "my_sockwrap.h"
#include "errlib.h"
#include <inttypes.h>
#include <string.h>

#define MSGSZ 32
#define WAIT_SEC 5

typedef int SOCKET;

char *prog_name;

int main(int argc, char *argv[]){
	
	// socket vars
	SOCKET s;
	//struct in_addr received_addr;
	//struct sockaddr_in saddr;
	struct sockaddr_storage *saddr;
	socklen_t saddr_len;
	//uint16_t port_h;
	
	// select vars
	fd_set rfds;
	struct timeval tv;
	int retval;
	
	// data vars
	char msg[MSGSZ];
	int n;
	
	// vars initialization
	//bzero(&received_addr, sizeof(received_addr));
	//bzero(&saddr, sizeof(saddr));
	bzero(&msg, MSGSZ * sizeof(char));
	//saddr_len = sizeof(saddr);
	//saddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	saddr_len = sizeof(struct sockaddr_storage);
	prog_name = argv[0];
	
	if(argc != 4){
		err_quit("Usage: %s <address> <port> <message (max 31 characters)>", prog_name);
	}
	
	/*
	// get address
	Inet_pton(AF_INET, argv[1], &received_addr);
	
	// get port
	if(sscanf(argv[2], "%"SCNu16, &port_h) != 1){
		err_quit("(%s) - invalid port", prog_name);
	}
	*/
	// get message
	snprintf(msg, MSGSZ, "%s", argv[3]);
	
	/*
	// populate structure
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port_h);
	saddr.sin_addr = received_addr;
	
	// create socket
	s = Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	*/
	
	// make a client
	s = Udp_client(argv[1], argv[2], (SA **)&saddr, &saddr_len);
	printf("(%s) - Client for: %s\n", prog_name, Sock_ntop((SA *)saddr, saddr_len));
	
	// setup read socket set
	FD_ZERO(&rfds);
	FD_SET(s, &rfds);
	
	// set waiting time
	tv.tv_sec = WAIT_SEC;
	tv.tv_usec = 0;
	
	// send message
	//Sendto(s, msg, strlen(msg), 0, (SA *) &saddr, saddr_len);
	Sendto(s, msg, strlen(msg), 0, (SA *) saddr, saddr_len);
	printf("(%s) - message sent\n", prog_name);
	
	// wait for response
	retval = Select(FD_SETSIZE, &rfds, 0, 0, &tv);
	
	// no response exit
	if(retval == 0){
		err_quit("(%s) - no datagram received within %d seconds.", prog_name, WAIT_SEC);
	}
	
	// receive response
	//n = Recvfrom(s, &msg, MSGSZ-1, 0, (SA *) &saddr, &saddr_len);
	n = Recvfrom(s, &msg, MSGSZ-1, 0, (SA *) saddr, &saddr_len);
	// terminate received message
	msg[n] = '\0';
	
	// print received message
	printf("(%s) - received: %s\n", prog_name, msg);
	
	free(saddr);
	
	return 0;
}

