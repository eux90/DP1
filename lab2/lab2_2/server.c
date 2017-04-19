#include "my_sockwrap.h"
#include "errlib.h"

#define N 10
#define BUFSZ 255
#define MAXREQ 3

typedef struct client_info{
	int requests;
	struct sockaddr_storage address;
	socklen_t address_len;
} client_info;

typedef int SOCKET;

char *prog_name;
client_info clients[N];

int check_client(struct sockaddr_storage caddr, socklen_t caddr_len, unsigned int *i);

int main(int argc, char *argv[]){
	
	
	SOCKET s;
	struct sockaddr_storage saddr,caddr;
	socklen_t saddr_len = sizeof(saddr);
	socklen_t caddr_len = sizeof(caddr);
	char msg[BUFSZ];
	unsigned int i=0;
	int n;
	
	prog_name = argv[0];
	
	if(argc != 2){
		err_quit("Usage: %s <port>",prog_name);
	}
	
	// clean clients info
	memset(&clients, 0, sizeof(clients));
	
	s = Udp_server(INADDR_ANY, argv[1], &saddr_len);
	Getsockname(s, (SA *)&saddr, &saddr_len);
	
	printf("(%s) - Server started at: %s... \n",prog_name, Sock_ntop((SA *)&saddr, saddr_len));
	while(1){
		
		if((n = recvfrom(s, msg, BUFSZ, 0, (SA *) &caddr, &caddr_len)) < 0){
			err_ret("(%s) - Error in reception of message from client...", prog_name);
			continue;
		}
		msg[n] = '\0';
		printf("(%s) - Received \"%s\" from %s\n", prog_name, msg, sock_ntop((SA *)&caddr, caddr_len));
		if(check_client(caddr, caddr_len, &i) != -1){
			if(sendto(s, msg, n, 0, (SA *) &caddr, caddr_len) != n){
				err_ret("(%s) - Error in sendto...", prog_name);
				continue;
			}
			printf("(%s) - Message sent back.\n", prog_name);
		}
		else{
			printf("(%s) - Client %s reached max request allowed (%d)\n", prog_name, sock_ntop((SA *)&caddr, caddr_len), MAXREQ);
		}
	}
	
	return 0;
	
}

// if connection number < threshold or address not present in db return 0 else -1
int check_client(struct sockaddr_storage caddr, socklen_t caddr_len, unsigned int *i){
	int j;
	int index;
	// check if address is already present
	for(j=0; j<N; j++){
		if(sock_cmp_addr((SA *)&caddr, (SA *)&(clients[j].address), caddr_len) != -1){
			// check connections #
			if(clients[j].requests < MAXREQ){
				(clients[j].requests)++;
				return 0;
			}
			return -1;
		}
	}
	// address is not present add it
	index = *i % N;
	clients[index].address = caddr;
	clients[index].address_len = caddr_len;
	clients[index].requests = 1;
	(*i)++;
	return 0;
}
			
	
