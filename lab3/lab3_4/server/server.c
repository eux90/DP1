#include "my_sockwrap.h"
#include "errlib.h"
#include "types.h"
#include <rpc/xdr.h>

#define BUFSZ 300
#define FBUFSZ 8192
#define MAXFNAME 256
#define GET_M "GET "
#define END_M "\r\n"
#define ERROR_M "-ERR\r\n"
#define OK_M "+OK\r\n"
#define QUIT_M "QUIT\r\n"

typedef int SOCKET;

char *prog_name;

void prot_a(int conn_fd);
void prot_x(int conn_fd);

int main(int argc, char *argv[]){
	
	// socket vars
	SOCKET listen_fd,conn_fd;
	struct sockaddr_storage saddr,caddr;
	socklen_t saddr_len,caddr_len;
	
	// protocol
	char prot;
	
	// initialize variables
	memset(&saddr, 0, sizeof(saddr));
	memset(&caddr, 0, sizeof(caddr));
	saddr_len = sizeof(saddr);
	caddr_len = sizeof(caddr);
	prot = 'a';
	prog_name = argv[0];
	
	if(argc < 2){
		err_quit("Usage: %s < -x (optional) > < port > ", prog_name);
	}
	
	if(argc == 2){
		// listen for connections
		listen_fd = Tcp_listen(INADDR_ANY, argv[1], &saddr_len);
		Getsockname(listen_fd, (SA *)&saddr, &saddr_len);
		printf("(%s) - server started at %s\n", prog_name, sock_ntop((SA *) &saddr, saddr_len));
	}
	else{
		if(strcmp(argv[1], "-x") == 0){
			prot = 'x';
		}
		else{
			err_quit("Usage: %s < -x (optional) > < port > ", prog_name);
		}
		// listen for connections
		listen_fd = Tcp_listen(INADDR_ANY, argv[2], &saddr_len);
		Getsockname(listen_fd, (SA *)&saddr, &saddr_len);
		printf("(%s) - server started at %s\n", prog_name, sock_ntop((SA *) &saddr, saddr_len));
	}
	

	
	while(1){		
		again:
		if ( (conn_fd = accept(listen_fd, (SA *)&caddr, &caddr_len)) < 0)
		{
			if (INTERRUPTED_BY_SIGNAL ||
				errno == EPROTO || errno == ECONNABORTED ||
				errno == EMFILE || errno == ENFILE ||
				errno == ENOBUFS || errno == ENOMEM			
				)
				goto again;
			else{
				err_msg ("(%s) error - accept() failed", prog_name);
				close(conn_fd);
				continue;
			}
		}

		printf("(%s) - client connected: %s\n", prog_name, sock_ntop((SA *) &caddr, caddr_len));
		
		switch(prot){
			
			case 'a':
				prot_a(conn_fd);
				break;
			case 'x' :
				prot_x(conn_fd);
				break;
		}
	}
	return 0;
}

void prot_x(int conn_fd){
	
	// xdr vars
	XDR r_xdrs,w_xdrs;
	FILE *r_stream, *w_stream;
	message r_msg, w_msg;
	
	// file vars
	int fd;
	ssize_t n;
	struct stat statbuf;
	unsigned int fsize,sent;
	char fname[MAXFNAME];
	char fbuf[FBUFSZ];
	
	// create read and write streams
	r_stream = fdopen(conn_fd, "r");
	w_stream = fdopen(conn_fd, "w");
	xdrstdio_create(&r_xdrs, r_stream, XDR_DECODE);
	xdrstdio_create(&w_xdrs, w_stream, XDR_ENCODE);
	
	while(1){
		
		// initialize vars for iteration
		memset(&r_msg, 0, sizeof(message));
		memset(&w_msg, 0, sizeof(message));
		memset(fbuf, 0, BUFSZ * sizeof(char));
		memset(fname, 0, MAXFNAME * sizeof(char));
		sent = 0;
		
		if(!xdr_message(&r_xdrs, &r_msg)){
			err_msg("(%s) - error in client request", prog_name);
			break;
		}
			
		// GET request
		if(r_msg.tag == GET){
			// store filename in fname and free xdr allocated data
			//memset(fname, 0, MAXFNAME);
			strncpy(fname, r_msg.message_u.filename, MAXFNAME);
			fname[MAXFNAME-1] = '\0';
			free(r_msg.message_u.filename);
			printf("(%s) - filename requested is: %s\n", prog_name, fname);
			
			// get file info
			if(stat(fname, &statbuf) != 0){
				err_msg("(%s) - file not found or error in retrieving file infos", prog_name);
				// build and sen error message
				w_msg.tag = ERR;
				if(!xdr_message(&w_xdrs, &w_msg)){
					err_msg("(%s) - error in transmission", prog_name);
					break;
				}
				fflush(w_stream);
				continue;
			}
			// store file size
			fsize = (unsigned int)statbuf.st_size;
			
			// open file for reading
			if((fd = open(fname, O_RDONLY)) == -1){
				err_msg("(%s) - error opening file for reading", prog_name);
				// build and send error message
				w_msg.tag = ERR;
				if(!xdr_message(&w_xdrs, &w_msg)){
					err_msg("(%s) - error in transmission", prog_name);
					break;
				}
				fflush(w_stream);
				continue;
			}
			
			// build and send info message
			printf("(%s) - file found, sending file infos..\n", prog_name);
			w_msg.tag = OK;
			w_msg.message_u.fdata.last_mod_time = (u_int)statbuf.st_mtim.tv_sec;
			if(!xdr_message(&w_xdrs, &w_msg)){
				err_msg("(%s) - error in transmission", prog_name);
				break;
			}
			fflush(w_stream);
			
			// send file size
			if(!xdr_u_int(&w_xdrs, &fsize)){
				err_msg("(%s) - error in transmission", prog_name);
				break;
			}
			fflush(w_stream);
			
			// send file
			while(1){
				// initialize variables
				memset(fbuf, 0, FBUFSZ * sizeof(char));
				memset(&w_msg, 0, sizeof(message));
				
				// read data until end or error
				n = read(fd, fbuf, FBUFSZ);
				if(n <= 0){
					break;
				}
				
				// send data
				w_msg.tag = OK;
				w_msg.message_u.fdata.contents.contents_len = (u_int)n;
				w_msg.message_u.fdata.contents.contents_val = (char *)malloc(n * sizeof(char));
				memcpy(w_msg.message_u.fdata.contents.contents_val, fbuf, n);
				if(!xdr_message(&w_xdrs, &w_msg)){
					err_msg("(%s) - error in transmission", prog_name);
					free(w_msg.message_u.fdata.contents.contents_val);
					break;
				}
				fflush(w_stream);
				free(w_msg.message_u.fdata.contents.contents_val);
				sent += n;
			}
			close(fd);
			if(sent != fsize){
				err_msg("(%s) - error in read or transmission", prog_name);
				break;
			}
			printf("(%s) - file sent\n", prog_name);
		}
		
		// QUIT request
		else if(r_msg.tag == QUIT){
			printf("(%s) - client requested to end communication\n", prog_name);
			break;
		}
		
		// undefined request
		else {
			printf("(%s) - communication error, incorrect message format\n", prog_name);
			break;
		}
	}
	
	// close streams and file descriptors
	xdr_destroy(&w_xdrs);
	xdr_destroy(&r_xdrs);
	fclose(r_stream);
	fclose(w_stream);
	close(conn_fd);
	printf("(%s) - client socket closed\n", prog_name);
	return;
}

void prot_a(int conn_fd){
	
	// data vars
	char fname[MAXFNAME];
	char r_buf[BUFSZ];
	char w_buf[BUFSZ];
	char fbuf[FBUFSZ];
	ssize_t n,sent;
	
	// messages vars
	int get_sz = strlen(GET_M);
	int quit_sz = strlen(QUIT_M);
	int error_sz = strlen(ERROR_M);
	int ok_sz = strlen(OK_M);
	
	//file vars
	int fd;
	struct stat statbuf;
	uint32_t fsize_n, lastmod_n;

	while(1){
			
		// initialize vars
		memset(fname, 0, MAXFNAME * sizeof(char));
		memset(r_buf, 0, BUFSZ * sizeof(char));
		memset(w_buf, 0, BUFSZ * sizeof(char));
		sent = 0;
			
		printf("(%s) - Waiting for file request...\n", prog_name);
			
		// leave last byte of r_buf for \0
		if((n = recv(conn_fd, r_buf, BUFSZ-1, 0)) == -1){
			err_msg("(%s) - error in recv", prog_name);
			continue;
		}
		// if no data is received client has closed connection, break client loop
		if(n == 0){
			err_msg("(%s) - connection closed by client", prog_name);
			break;
		}
		// terminate received string
		r_buf[n] = '\0';
		
		// quit request received
		if(strncmp(r_buf, QUIT_M, quit_sz) == 0){
			printf("(%s) - client requested end of communication\n", prog_name);
			break;
		}
		// undefined request received
		if(strncmp(r_buf, GET_M, get_sz) != 0){
			err_msg("(%s) - incorrect format in request", prog_name);
			if(sendn(conn_fd, ERROR_M, error_sz, MSG_NOSIGNAL) != error_sz){
				err_msg("(%s) - could not send back error message", prog_name);
				break;
			}
			continue;
		}
		// get request received
		else{
			// cut trailer from recv_buf
			n = strcspn(r_buf, END_M);
			r_buf[n] = '\0';
			strncpy(fname, &r_buf[get_sz], MAXFNAME);
			// terminate copied string in case source was longer than destination
			fname[MAXFNAME-1] = '\0';
			printf("(%s) - filename requested is: %s\n", prog_name, fname);
			
			// try to open file for reading
			if((fd = open(fname, O_RDONLY)) == -1){
				err_msg("(%s) - error opening file for reading..", prog_name);
				if(sendn(conn_fd, ERROR_M, error_sz, MSG_NOSIGNAL) != error_sz){
					err_msg("(%s) - could not send back error message", prog_name);
					break;
				}
				continue;
			}
			
			// get file info
			if(stat(fname, &statbuf) == -1){
				err_msg("(%s) - file not found or error in retrieving file infos", prog_name);
				if(sendn(conn_fd, ERROR_M, error_sz, MSG_NOSIGNAL) != error_sz){
					err_msg("(%s) - could not send back error message", prog_name);
					break;
				}
				continue;
			}
			fsize_n = htonl(statbuf.st_size);
			lastmod_n = htonl(statbuf.st_mtim.tv_sec);
			
			// build info message
			memcpy(w_buf, OK_M, ok_sz);
			memcpy(&w_buf[ok_sz], &fsize_n, sizeof(fsize_n));
			memcpy(&w_buf[ok_sz + sizeof(fsize_n)], &lastmod_n, sizeof(lastmod_n));
			
			// send info message
			n = ok_sz + sizeof(fsize_n) + sizeof(lastmod_n);
			if(sendn(conn_fd, w_buf, n, MSG_NOSIGNAL) != n){
				err_msg("(%s) - error sending file infos..", prog_name);
				break;
			}
			
			// send file
			while(1){
				
				//initialize vars
				memset(fbuf, 0, FBUFSZ * sizeof(char));
				
				// read until end of file or error
				n = read(fd, fbuf, FBUFSZ);
				if(n <= 0){
					break;
				}
				
				// send data
				if(sendn(conn_fd, fbuf, n, MSG_NOSIGNAL) != n){
					err_msg("(%s) - error in transmission", prog_name);
					break;
				}
				sent += n;
			}
			close(fd);
			if(sent != statbuf.st_size){
				err_msg("(%s) - error in read or transmission", prog_name);
				break;
			}
			printf("(%s) - file sent\n", prog_name);
		}
	}
		
	// out of client loop close client socket
	close(conn_fd);
	printf("(%s) - client socket closed\n", prog_name);
	return;
}	
