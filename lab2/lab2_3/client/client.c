#include "my_sockwrap.h"
#include "errlib.h"
#include <time.h>

#define BUFSZ 300
#define FBUFSZ 8192
#define MAXFNAME 256
#define GET_M "GET "
#define END_M "\r\n"
#define OK_M "+OK\r\n"
#define ERROR_M "-ERR\r\n"
#define QUIT_M "QUIT\r\n"
#define CLOSE_M "CLOSE"

typedef int SOCKET;

char *prog_name;

void prot_a(SOCKET s);
char *print_time(uint32_t fdate);

int main(int argc, char *argv[]){
	
	SOCKET s;
	struct sockaddr_storage saddr;
	socklen_t saddr_len;
	
	
	prog_name = argv[0];
	
	// initialize vars
	bzero(&saddr, sizeof(saddr));
	saddr_len = sizeof(saddr);
	
	if(argc != 3){
		err_quit("Usage: %s <address> <port>", prog_name);
	}
	
	// connect to server
	s = Tcp_connect(argv[1], argv[2]);
	Getpeername(s, (SA *)&saddr, &saddr_len);
	printf("(%s) - Connected to: %s\n", prog_name, Sock_ntop((SA *)&saddr, saddr_len));
	
	prot_a(s);
	
	return 0;
}

void prot_a(SOCKET s){
	
	// data vars
	char rk_buf[BUFSZ];
	char w_buf[BUFSZ];
	char r_buf[BUFSZ];
	char fbuf[FBUFSZ];
	char fname[MAXFNAME];
	ssize_t n;
	
	// messages sizes
	int ok_sz = strlen(OK_M);
	int error_sz = strlen(ERROR_M);
	int end_sz = strlen(END_M);
	int quit_sz = strlen(QUIT_M);
	int get_sz = strlen(GET_M);
	
	// file vars
	FILE *fPtr = NULL;
	uint32_t fsize, fdate, fsize_n, fdate_n;

	
	while(1){
		
		// initialize buffers
		memset(&rk_buf, 0, BUFSZ);
		memset(w_buf, 0, BUFSZ * sizeof(char));
		memset(r_buf, 0, BUFSZ * sizeof(char));
		memset(fname, 0, MAXFNAME * sizeof(char));
		
		printf("\n*******************************************************\n");
		printf("(%s) - RETRIEVE FILE:\t\t <filename>\n", prog_name);
		printf("(%s) - QUIT GRACEFULLY:\t\t CTRL + D\n", prog_name);
		printf("*******************************************************\n\n");
		
		Fgets(rk_buf, BUFSZ, stdin);
		
		// QUIT request
		if(feof(stdin)){
			// build QUIT message
			strncpy(w_buf, QUIT_M, quit_sz);
			// send message
			Sendn(s, w_buf, strlen(w_buf), 0);
			printf("(%s) - QUIT message sent\n", prog_name);
			break;
		}
		
		// GET request
		else{
			// store file name
			memcpy(fname, rk_buf, MAXFNAME);
			n = strcspn(fname, "\r\n");
			fname[n] = '\0';
			fname[MAXFNAME-1] = '\0';
			// build GET message
			strncpy(w_buf, GET_M, get_sz);
			n = strlen(w_buf);
			strncpy(&w_buf[n], fname, MAXFNAME);
			n = strlen(w_buf);
			strncpy(&w_buf[n], END_M, end_sz);
			// send message
			Sendn(s, w_buf, strlen(w_buf), 0);
			printf("(%s) - GET message sent\n", prog_name);
			
			// receive response
			n = Recv(s, r_buf, ok_sz, MSG_WAITALL);
			
			// check if connection has been closed by server
			if(n == 0){
				printf("(%s) - connection closed by server\n", prog_name);
				break;
			}
		
			if(n != ok_sz){
				err_msg("(%s) - error recv() red less bytes than expected", prog_name);
				break;
			}
			
			// terminate received string
			r_buf[ok_sz] = '\0';
		
			// ERROR RESPONSE
			if(strcmp(r_buf, OK_M) != 0){
				// receive rest of message
				Recv(s, &r_buf[ok_sz], (error_sz - ok_sz), MSG_WAITALL);
				r_buf[error_sz] = '\0';
				if(strcmp(r_buf, ERROR_M) != 0){
					err_msg("(%s) - undefined response from server", prog_name);
					break;
				}
				printf("(%s) - -ERR message, file may not exist\n", prog_name);
				break;
			}
		
			// OK RESPONSE read file infos
			if(Recv(s, &fsize_n, sizeof(fsize_n), MSG_WAITALL) != sizeof(fsize_n)){
				err_msg("(%s) - error in reading file size from socket", prog_name);
				break;
			}
	
			if(Recv(s, &fdate_n, sizeof(fdate_n), MSG_WAITALL) != sizeof(fdate_n)){
				err_msg("(%s) - error in reading file last mod from socket", prog_name);
				break;
			}
			// convert variables in client byte order
			fsize = ntohl(fsize_n);
			fdate = ntohl(fdate_n);
			
			printf("(%s) - file %s found, last mod: %s\n", prog_name, fname, print_time(fdate));
			
			// open a file for writing
			//if((fd = open(fname, O_CREAT | O_WRONLY, 0775)) == -1){
			if((fPtr = fopen(fname, "w")) == NULL){
				err_msg("(%s) - could not create a file for writing...", prog_name);
				break;
			}
			
			// read data
			printf("(%s) - receiving file data...\n", prog_name);
			while(fsize > 0){
				// clen read buffer
				memset(fbuf, 0, FBUFSZ * sizeof(char));
				n = Recv(s, fbuf, FBUFSZ, 0);
				// write data to file
				//if(write(fd, fbuf, n) != n){
				if(fwrite(fbuf, 1, n, fPtr) != n){
					err_msg("(%s) - write failed...", prog_name);
					break;
				}
				fsize-=n;
			}
			fclose(fPtr);
			if(fsize > 0){
				err_msg("(%s) - data received is partial, file may be corrupted", prog_name);
				break;
			}
			printf("(%s) - file correctly received\n", prog_name);
		}
	}
	close(s);
	return;
}

char *print_time(uint32_t fdate){
	
	time_t time = fdate;
	return ctime(&time);
}
