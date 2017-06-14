#include "sockwrap.h"
#include "errlib.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define LISTENQ 5
#define BUFLEN 255
#define MAX_UINT16T 0xffff
#define MSG_ERR "incorrect operands\r\n"
#define MSG_OVF "overflow\r\n"

char *prog_name;

void serve_client(int conn_fd);

int main(int argc, char *argv[]){
	
	// socket vars
	int listen_fd, conn_fd;
	struct sockaddr_in saddr,caddr;
	socklen_t caddrlen;
	uint16_t porth;
	
	// initialize vars
	memset(&saddr, 0, sizeof(saddr));
	memset(&caddr, 0, sizeof(caddr));
	caddrlen = sizeof(caddr);
	prog_name = argv[0];
	
	if(argc != 2){
		err_quit("Usage: %s <port>",prog_name);
	}
	
	if(sscanf(argv[1], "%" SCNu16, &porth) != 1){
		err_quit("(%s) - invalid port", prog_name);
	}
	
	// populate server socket structure
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(porth);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// create socket bind and set listen queue
	listen_fd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	Bind(listen_fd, (SA*) &saddr, sizeof(saddr));
	Listen(listen_fd, LISTENQ);
	
	while(1){
		int retry = 0;
		do {
			printf("Waiting for connection...\n");
			conn_fd = accept (listen_fd, (SA*) &caddr, &caddrlen);
			if (conn_fd<0) {
				if (INTERRUPTED_BY_SIGNAL ||
					errno == EPROTO || errno == ECONNABORTED ||
					errno == EMFILE || errno == ENFILE ||
					errno == ENOBUFS || errno == ENOMEM	) {
					retry = 1;
					err_ret ("(%s) error - accept() failed", prog_name);
				} else {
					err_ret ("(%s) error - accept() failed", prog_name);
					return 1;
				}
			} else {
				printf("(%s) - new connection from client %s\n", prog_name, sock_ntop((SA*)&caddr, caddrlen) );
				retry = 0;
			}
		} while (retry);
		
		serve_client(conn_fd);
		Close(conn_fd);	
	}
	return 0;
}

void serve_client(int conn_fd){
	
	// data vars
	char r_buf[BUFLEN];
	char w_buf[BUFLEN];
	uint16_t first;
	uint16_t second;
	int sum;
	int n;
	
	// sizes
	int err_sz;
	int ovf_sz;
	
	// initialize buffers and sizes
	bzero(&r_buf, BUFLEN * sizeof(char));
	bzero(&w_buf, BUFLEN * sizeof(char));
	err_sz = strlen(MSG_ERR);
	ovf_sz = strlen(MSG_OVF);
	
	
	while(1){
		// initialize buffers for next iteration
		bzero(&r_buf, BUFLEN * sizeof(char));
		bzero(&w_buf, BUFLEN * sizeof(char));
		printf("Waiting for operands...\n");
		n = recv(conn_fd, r_buf, BUFLEN, 0);
		if(n == -1){
			err_ret("(%s) - error in reading message from socket", prog_name);
			break;
		}
		if(n == 0){
			printf("Connection closed by client\n");
			break;
		}
		r_buf[n] = '\0';
		if(sscanf(r_buf, "%" SCNu16 " " "%" SCNu16, &first, &second) != 2){
			err_ret("(%s) - invalid operands", prog_name);
			if(sendn(conn_fd, MSG_ERR, err_sz, MSG_NOSIGNAL) != err_sz){
				err_ret("(%s) - could not send error message", prog_name);
				break;
			}
			continue;
		}
		sum = first + second;
		if(sum > MAX_UINT16T){
			if(sendn(conn_fd, MSG_OVF, ovf_sz, MSG_NOSIGNAL) != ovf_sz){
				err_ret("(%s) - could not send overflow message", prog_name);
				break;
			}
			continue;
		}
		if((n = snprintf(w_buf, BUFLEN, "%d\r\n", sum)) < 0){
			err_ret("(%s) - error in response creation", prog_name);
			break;
		}
		if(sendn(conn_fd, w_buf, n, MSG_NOSIGNAL) != n){
			err_ret("(%s) - could not send response message", prog_name);
			break;
		}
		printf("Response sent.\n");
	}
	return;
}
	
		
	
		
		
		
	
	
	
	
	


		
