#include "my_sockwrap.h"
#include "errlib.h"

#define BUFSZ 512
#define MAXFNAME 255
#define GET "GET "
#define END "\r\n"

typedef int SOCKET;
char *prog_name;


int main(int argc, char *argv[]){
	
	struct sockaddr_storage saddr;
	socklen_t saddr_len;
	SOCKET s;
	char buf[BUFSZ];
	char message[BUFSZ];
	int n;
	
	prog_name = argv[0];
	
	memset(&saddr, 0, sizeof(saddr));
	saddr_len = sizeof(saddr);
	
	if(argc != 3){
		err_quit("Usage: %s <address> <port>", prog_name);
	}
	
	// connect to server
	s = Tcp_connect(argv[1], argv[2]);
	Getpeername(s, (SA *)&saddr, &saddr_len);
	printf("(%s) - Connected to: %s\n", prog_name, Sock_ntop((SA *)&saddr, saddr_len));
	
	while(1){
		printf("Write file name to request (max %d chars):\n", MAXFNAME);
		Fgets(buf, BUFSZ, stdin);
		n = strcspn(buf, "\r\n");
		buf[n] = '\0';
		printf("(%s) - Filename: %s\n", prog_name, buf);
		
		// build file request
		strcpy(message, GET);
		n = strlen(message);
		strncpy(&message[n], buf, MAXFNAME);
		n = strlen(message);
		strcpy(&message[n], END); 
		//printf("(%s) - Message: %s\n", prog_name, message);
		
		// send file request
		Sendn(s, message, strlen(message), 0);
		
	}

	return 0;
}
