#include "my_sockwrap.h"
#include "errlib.h"
#include "types.h"
#include <rpc/xdr.h>
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
//#define COPY "_new"

typedef int SOCKET;
char *prog_name;

char *print_time(uint32_t fdate);
void prot_a(SOCKET s);
void prot_x(SOCKET s);

int main(int argc, char *argv[]){
	
	// sock vars
	struct sockaddr_storage saddr;
	socklen_t saddr_len;
	SOCKET s;
	
	//protocol var
	char prot;
	
	// initialize variables
	memset(&saddr, 0, sizeof(saddr));
	saddr_len = sizeof(saddr);
	prot = 'a';
	prog_name = argv[0];
	
	if(argc < 3){
		err_quit("Usage: %s < -x (optional) > < address > < port > ", prog_name);
	}
	
	if(argc == 3){
		// connect to server
		s = Tcp_connect(argv[1], argv[2]);
		Getpeername(s, (SA *)&saddr, &saddr_len);
		printf("(%s) - Connected to: %s\n", prog_name, Sock_ntop((SA *)&saddr, saddr_len));
	}
	else{
		if(strcmp(argv[1], "-x") == 0){
			prot = 'x';
		}
		else{
			err_quit("Usage: %s < -x (optional) > < address > < port > ", prog_name);
		}
		// connect to server
		s = Tcp_connect(argv[2], argv[3]);
		Getpeername(s, (SA *)&saddr, &saddr_len);
		printf("(%s) - Connected to: %s\n", prog_name, Sock_ntop((SA *)&saddr, saddr_len));
	}

	switch(prot){
		
		case 'a':
			prot_a(s);
			break;
		case 'x':
			prot_x(s);
			break;
	}
	
	return 0;
}

void prot_x(SOCKET s){
	
	// data vars
	size_t limit;
	char rk_buf[BUFSZ];
	
	// xdr protocol vars
	XDR r_xdrs, w_xdrs;
	message r_msg, w_msg;
	
	// file vars
	//int fd;
	FILE *fPtr;
	unsigned int fsize;
	char fname[MAXFNAME];

	// streams
	FILE *w_stream, *r_stream;

	// create stream for writing
	w_stream = fdopen(s, "w");
	r_stream = fdopen(s, "r");
	xdrstdio_create(&w_xdrs, w_stream, XDR_ENCODE);
	xdrstdio_create(&r_xdrs, r_stream, XDR_DECODE);
	
	while(1){
		
		// initialize vars
		memset(&w_msg, 0, sizeof(message));
		memset(&r_msg, 0, sizeof(message));
		memset(&rk_buf, 0, BUFSZ * sizeof(char));	
		
		printf("\n*******************************************************\n");
		printf("(%s) - RETRIEVE FILE:\t\t get <filename>\n", prog_name);
		printf("(%s) - QUIT GRACEFULLY:\t\t q\n", prog_name);
		printf("(%s) - FORCE QUIT:\t\t a\n", prog_name);
		printf("*******************************************************\n\n");
	
		Fgets(rk_buf, BUFSZ, stdin);
		// QUIT request
		if(rk_buf[0] == 'Q' || rk_buf[0] == 'q'){
			// build QUIT message
			w_msg.tag = QUIT;	
			// send QUIT message
			if(!xdr_message(&w_xdrs, &w_msg)){
				err_msg("(%s) - data transmission error", prog_name);
				break;
			}
			fflush(w_stream);
			printf("(%s) - QUIT message sent\n", prog_name);
			break;
		}
		// EXIT request
		else if(rk_buf[0] == 'A' || rk_buf[0] == 'a'){
			break;
		}
		// request not recognised
		else if((strncmp(rk_buf, "GET ", 4) != 0) && (strncmp(rk_buf, "get ", 4) != 0)){
			printf("(%s) - command not recognised try again..\n", prog_name);
			continue;
		}
		// GET request
		else{
			// build GET message
			w_msg.tag = GET;
			w_msg.message_u.filename = (char *)malloc(MAXFNAME * sizeof(char));
			memcpy(w_msg.message_u.filename, &rk_buf[4], MAXFNAME);
			limit = strcspn(w_msg.message_u.filename, "\r\n");
			w_msg.message_u.filename[limit] = '\0';
			w_msg.message_u.filename[MAXFNAME-1] = '\0';
			// send GET message
			if(!xdr_message(&w_xdrs, &w_msg)){
				err_msg("(%s) - data transmission error", prog_name);
				free(w_msg.message_u.filename);
				break;
			}
			fflush(w_stream);
			printf("(%s) - GET message sent\n", prog_name);
			// store filename
			strncpy(fname, w_msg.message_u.filename, MAXFNAME);
			fname[MAXFNAME-1] = '\0';
			free(w_msg.message_u.filename);
			
			// read RESPONSE
			if(!xdr_message(&r_xdrs, &r_msg)){
				err_msg("(%s) - error in response format or connection closed by server", prog_name);
				break;
			}
			
			// ERROR RESPONSE
			if(r_msg.tag == ERR){
				printf("(%s) - error message from server, file may not exist\n", prog_name);
				continue;
			}
			
			// OK RESPONSE
			if(r_msg.tag == OK){
				printf("(%s) - file %s found, last mod: %s\n", prog_name, fname, print_time(r_msg.message_u.fdata.last_mod_time));
				// receive file size
				if(!xdr_u_int(&r_xdrs, &fsize)){
					err_msg("(%s) - error in file size format or connection closed by server", prog_name);
					break;
				}
				printf("(%s) - file size: %d\n",prog_name, fsize);
				
				// open a file for writing
				//if((fd = open(fname, O_CREAT | O_WRONLY, 0775)) == -1){
				if((fPtr = fopen(fname, "w")) == NULL){
					err_msg("(%s) - could not create a file for writing data...", prog_name);
					//close(fd);
					break;
				}
				// read data
				printf("(%s) - receiving file data...\n", prog_name);
				while(fsize > 0){
					// initialize vars
					memset(&r_msg, 0, sizeof(message));
					r_msg.message_u.fdata.contents.contents_val = NULL;
					if(!xdr_message(&r_xdrs, &r_msg)){
						err_msg("(%s) - could not read xdr message data...", prog_name);
						free(r_msg.message_u.fdata.contents.contents_val);
						break;
					}
					//if(write(fd, r_msg.message_u.fdata.contents.contents_val, r_msg.message_u.fdata.contents.contents_len) != r_msg.message_u.fdata.contents.contents_len){
					if(fwrite(r_msg.message_u.fdata.contents.contents_val, 1, r_msg.message_u.fdata.contents.contents_len, fPtr) != r_msg.message_u.fdata.contents.contents_len){
						err_msg("(%s) - write failed...", prog_name);
						free(r_msg.message_u.fdata.contents.contents_val);
						break;
					}
					fsize -= r_msg.message_u.fdata.contents.contents_len;
					free(r_msg.message_u.fdata.contents.contents_val);
				}
				//close(fd);
				fclose(fPtr);
				if(fsize > 0){
					err_msg("(%s) - data received is partial, file may be corrupted", prog_name);
					break;
				}
				printf("(%s) - file %s correctly received\n", prog_name, fname);
			}	
		}
	}
	xdr_destroy(&r_xdrs);
	xdr_destroy(&w_xdrs);
	fclose(w_stream);
	fclose(r_stream);
	close(s);
	return;
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
	//int fd;
	FILE *fPtr;
	uint32_t fsize, fdate, fsize_n, fdate_n;

	
	while(1){
		
		// initialize buffers
		memset(&rk_buf, 0, BUFSZ);
		memset(w_buf, 0, BUFSZ * sizeof(char));
		memset(r_buf, 0, BUFSZ * sizeof(char));
		memset(fname, 0, MAXFNAME * sizeof(char));
		
		printf("\n*******************************************************\n");
		printf("(%s) - RETRIEVE FILE:\t\t get <filename>\n", prog_name);
		printf("(%s) - QUIT GRACEFULLY:\t\t q\n", prog_name);
		printf("(%s) - FORCE QUIT:\t\t a\n", prog_name);
		printf("*******************************************************\n\n");
		
		Fgets(rk_buf, BUFSZ, stdin);
		
		// QUIT request
		if(rk_buf[0] == 'Q' || rk_buf[0] == 'q'){
			// build QUIT message
			strncpy(w_buf, QUIT_M, quit_sz);
			// send message
			Sendn(s, w_buf, strlen(w_buf), 0);
			printf("(%s) - QUIT message sent\n", prog_name);
			break;
		}
		
		// EXIT request
		else if(rk_buf[0] == 'A' || rk_buf[0] == 'a'){
			break;
		}
		
		// request not recognised
		else if((strncmp(rk_buf, "GET ", 4) != 0) && (strncmp(rk_buf, "get ", 4) != 0)){
			printf("(%s) - command not recognised try again..\n", prog_name);
			continue;
		}
		
		// GET request
		else{
			// store file name
			memcpy(fname, &rk_buf[4], MAXFNAME);
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
				printf("(%s) - error message from server, file may not exist\n", prog_name);
				continue;
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
			//close(fd);
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
	
