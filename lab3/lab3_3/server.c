#include "my_sockwrap.h"
#include "errlib.h"

#define BUFSZ 8192
#define MAXFNAME 256
#define GET_M "GET "
#define END_M "\r\n"
#define ERROR_M "-ERR\r\n"
#define OK_M "+OK\r\n"
#define QUIT_M "QUIT\r\n"
#define MAXPROCS 10

typedef int SOCKET;

char *prog_name;

void prot_a(int conn_fd);

int main(int argc, char *argv[]){
	
	// socket vars
	SOCKET listen_fd,conn_fd;
	struct sockaddr_storage saddr,caddr;
	socklen_t saddr_len,caddr_len;
	
	// mutliprocess management
	int	childpid;		/* pid of child process */
	int i = 0;
	int nprocs;
	
	// initialize variables
	memset(&saddr, 0, sizeof(saddr));
	memset(&caddr, 0, sizeof(caddr));
	saddr_len = sizeof(saddr);
	caddr_len = sizeof(caddr);
	prog_name = argv[0];
	
	if(argc != 3){
		err_quit("Usage: %s <port> <#processes>", prog_name);
	}
	
	nprocs = atoi(argv[2]);
	if(nprocs > MAXPROCS){
		err_quit("%s - max allowed is %d children", prog_name, MAXPROCS);
	}
	
	memset(&saddr, 0, sizeof(saddr));
	saddr_len = sizeof(saddr);
	
	// listen for connections
	listen_fd = Tcp_listen(INADDR_ANY, argv[1], &saddr_len);
	Getsockname(listen_fd, (SA *)&saddr, &saddr_len);
	printf("(%s) - Server started at %s\n", prog_name, sock_ntop((SA *) &saddr, saddr_len));
	
	/* create processes */
    for (i=0; i<nprocs; i++){
		if((childpid=fork())<0)
			err_sys("fork() failed");
		else if (childpid == 0){
	    /* child process */
	    
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
				prot_a(conn_fd);
			}
			
		}
	}
    printf("(%s) - Process pool created\n", prog_name);
    // keep main process open until childs are alive
    for(i=0; i<nprocs; i++){
		waitpid((pid_t)(-1), 0, 0);
	}
	return 0;
}


void prot_a(int conn_fd){
	
	// data vars
	char fname[MAXFNAME];
	char r_buf[BUFSZ];
	char w_buf[BUFSZ];
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
				memset(w_buf, 0, BUFSZ * sizeof(char));
				
				// read until end of file or error
				n = read(fd, w_buf, BUFSZ);
				if(n <= 0){
					break;
				}
				
				// send data
				if(sendn(conn_fd, w_buf, n, MSG_NOSIGNAL) != n){
					err_msg("(%s) - error in transmission", prog_name);
					break;
				}
				sent += n;
				memset(w_buf, 0, BUFSZ * sizeof(char));
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