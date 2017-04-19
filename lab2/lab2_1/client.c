#include "my_sockwrap.h"
#include "errlib.h"

#define MSGSZ 32
#define WAIT_SEC 3
#define MAX_RETRANSMIT 5

typedef int SOCKET;
char *prog_name;

int main(int argc, char *argv[]){
	
	SOCKET s;
	struct sockaddr_storage *saddr;
	socklen_t saddr_len = sizeof(struct sockaddr_storage);
	char msg[MSGSZ];
	int i=0;
	int retval;
	fd_set rfds;
	struct timeval tv;
	
	prog_name = argv[0];
	
	if(argc != 4){
		err_quit("Usage: %s <address> <port> <message (max 31 characters)>", prog_name);
	}
	
	// create socket
	s = Udp_client(argv[1], argv[2], (SA **)&saddr, &saddr_len);
	
	// get message
	snprintf(msg, MSGSZ, "%s", argv[3]);
	
	// setup read socket set
	FD_ZERO(&rfds);
	FD_SET(s, &rfds);
	
	// set waiting time
	tv.tv_sec = WAIT_SEC;
	tv.tv_usec = 0;
	
	// send message
	Sendto(s, msg, strlen(msg), 0, (SA *) saddr, saddr_len);
	
	// wait for response
	retval = Select(FD_SETSIZE, &rfds, 0, 0, &tv);
	
	// no response retransmit
	if(retval == 0){
		while(1){
			// setup read socket set
			FD_ZERO(&rfds);
			FD_SET(s, &rfds);
			// set waiting time
			tv.tv_sec = WAIT_SEC;
			tv.tv_usec = 0;
			err_msg("(%s) - no datagram received within %d seconds, retransmitting", prog_name, WAIT_SEC);
			Sendto(s, msg, strlen(msg), 0, (SA *) saddr, saddr_len);
			retval = Select(FD_SETSIZE, &rfds, 0, 0, &tv);
			if(retval == 0){
				i++;
				if(i >= MAX_RETRANSMIT){
					err_quit("(%s) - no datagram received, max of %d retrasmissions reached", prog_name, MAX_RETRANSMIT);
				}
			}
			else{
				break;
			}
		}
	}
	
	// receive response
	int n = Recvfrom(s, msg, MSGSZ-1, 0, (SA *) saddr, &saddr_len);
	// terminate received message
	msg[n] = '\0';
	
	// print received message
	printf("(%s) - received: %s\n", prog_name, msg);
	
	return 0;
}
	
