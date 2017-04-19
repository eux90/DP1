#include "sockwrap.h"
#include "errlib.h"
#include <inttypes.h>
#include <string.h>



#define BUFSZ 255


typedef int SOCKET;
char *prog_name;

int main(int argc, char *argv[]){
	
	SOCKET s;
	struct sockaddr_in saddr,caddr;
	uint16_t port_h;
	socklen_t saddr_len = sizeof(saddr);
	socklen_t caddr_len = sizeof(caddr);
	char msg[BUFSZ];
	int n;
	
	prog_name = argv[0];
	
	if(argc != 2){
		err_quit("Usage: %s <port>",prog_name);
	}
	
	// get port
	
	if(sscanf(argv[1], "%" SCNu16, &port_h) != 1){
		err_sys("(%s) - invlaid port number\n");
	}
	
	// populate sockaddr structure
	memset(&saddr, 0, saddr_len);
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port_h);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// create socket
	s = Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	//Bind socket
	Bind(s, (SA *) &saddr, saddr_len);
	
	
	printf("(%s) - Server started... \n",prog_name);
	while(1){
		
		if((n = recvfrom(s, msg, BUFSZ, 0, (SA *) &caddr, &caddr_len)) < 0){
			err_ret("(%s) - Error in reception of message from client...", prog_name);
			continue;
		}
		msg[n] = '\0';
		printf("(%s) - Message received: %s, sending back... \n", prog_name, msg);
		if(sendto(s, msg, n, 0, (SA *) &caddr, caddr_len) != n){
			err_ret("(%s) - Error in sendto...", prog_name);
			continue;
		}
		printf("(%s) - Message sent.\n", prog_name);
	}
	
	return 0;
}
	
	
	


