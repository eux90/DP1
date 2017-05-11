#include "my_sockwrap.h"
#include "errlib.h"

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <rpc/xdr.h>

#define MSGSZ 255


typedef int SOCKET;

char *prog_name;

int main(int argc, char *argv[]){
	
	struct sockaddr_in saddr;
	struct in_addr received_addr;
	char msg[MSGSZ];
	uint16_t port_h;
	SOCKET s;
	XDR xdrs_w, xdrs_r;
	int op1=0;
	int op2=0;
	int res=0;
	
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
	
	printf("Enter first operand\n");
	scanf("%d", &op1);
	printf("Enter second operand\n");
	scanf("%d", &op2);
	
	int len = 0;
	//while(!feof(stdin)){
		// terminate string with CRLF
		//strcat(msg,"\r\n");
		//len = strlen(msg);
		
		// send op1
		xdrmem_create(&xdrs_w, msg, MSGSZ, XDR_ENCODE);
		// encode and send
		xdr_int(&xdrs_w, &op1);
		len = xdr_getpos(&xdrs_w);
		Sendn(s, msg, len, 0);
		xdr_destroy(&xdrs_w);
		
		// send op2
		xdrmem_create(&xdrs_w, msg, MSGSZ, XDR_ENCODE);
		// encode and send
		xdr_int(&xdrs_w, &op2);
		len = xdr_getpos(&xdrs_w);
		Sendn(s, msg, len, 0);
		xdr_destroy(&xdrs_w);
		
		// receive response
		Recv(s,msg,MSGSZ,0);
		xdrmem_create(&xdrs_r, msg, MSGSZ, XDR_DECODE);
		// decode response
		xdr_int(&xdrs_r, &res);
		xdr_destroy(&xdrs_r);
		
		printf("Received: %d\n", res);

		/*
		printf("Enter first operand\n");
		scanf("%d", &op1);
		printf("Enter second operand\n");
		scanf("%d", &op2);
		*/
	//}
	
	Close(s);
	return 0;
}
		 
		
	
	
