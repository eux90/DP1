#include "my_sockwrap.h"
#include "errlib.h"

#define BUFSZ 300
#define FBUFSZ 8192
#define MAXFNAME 256
#define GET_M "GET "
#define END_M "\r\n"
#define ERROR_M "-ERR\r\n"
#define OK_M "+OK\r\n"
#define QUIT_M "QUIT\r\n"
#define CHMAX 3

typedef int SOCKET;

char *prog_name;
int nchild;

void prot_a(int conn_fd);

// handle SIGCHLD and decrement child counter
void handle_sigchld(int sig);

int main(int argc, char *argv[]){
	
	// socket vars
	SOCKET listen_fd,conn_fd;
	struct sockaddr_storage saddr,caddr;
	socklen_t saddr_len,caddr_len;
	
	// mutliprocess management
	int	childpid;		/* pid of child process */
	struct sigaction sa;
	
	prog_name = argv[0];
	nchild = 0;
	
	if(argc != 2){
		err_quit("Usage: %s <port>", prog_name);
	}
	
	memset(&saddr, 0, sizeof(saddr));
	memset(&caddr, 0, sizeof(caddr));
	saddr_len = sizeof(saddr);
	caddr_len = sizeof(caddr);
	
	// register signal handler
	sa.sa_handler = &handle_sigchld;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	if (sigaction(SIGCHLD, &sa, 0) == -1) {
		perror(0);
		exit(1);
	}
	
	// listen for connections
	listen_fd = Tcp_listen(INADDR_ANY, argv[1], &saddr_len);
	Getsockname(listen_fd, (SA *)&saddr, &saddr_len);
	printf("(%s) - Server started at %s\n", prog_name, sock_ntop((SA *) &saddr, saddr_len));
	conn_fd = listen_fd;
	
	/* main loop */
	for(;;){
		// allow only CHMAX client at a time
		while(nchild < CHMAX){
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
			printf("(%s) - Client connected: %s\n", prog_name, sock_ntop((SA *) &caddr, caddr_len));
			
			
			/* fork a new process to serve the client on the new connection */
			if((childpid=fork())<0) 
			{ 
				err_msg("fork() failed");
				close(conn_fd);
			}
			else if (childpid > 0)
			{ 
				/* parent process */
				close(conn_fd);	/* close connected socket */
				// increment counter of child processes
				nchild++;
			}
			else
			{
				/* child process */
				close(listen_fd);			/* close passive socket */
				prot_a(conn_fd);			/* serve client */
				exit(0);
			}	
		}
	}
	
	// out of server loop
	return 0;
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
	//int fd;
	FILE *fPtr = NULL;
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
			
			// try to open file for reading
			//if((fd = open(fname, O_RDONLY)) == -1){
			if((fPtr = fopen(fname, "r")) == NULL){
				err_msg("(%s) - error opening file for reading..", prog_name);
				if(sendn(conn_fd, ERROR_M, error_sz, MSG_NOSIGNAL) != error_sz){
					err_msg("(%s) - could not send back error message", prog_name);
					break;
				}
				continue;
			}
			
			// build info message
			memcpy(w_buf, OK_M, ok_sz);
			memcpy(&w_buf[ok_sz], &fsize_n, sizeof(fsize_n));
			memcpy(&w_buf[ok_sz + sizeof(fsize_n)], &lastmod_n, sizeof(lastmod_n));
			
			// send info message
			n = ok_sz + sizeof(fsize_n) + sizeof(lastmod_n);
			if(sendn(conn_fd, w_buf, n, MSG_NOSIGNAL) != n){
				err_msg("(%s) - error sending file infos..", prog_name);
				fclose(fPtr);
				break;
			}
			
			// send file
			while(1){
				
				//initialize vars
				memset(fbuf, 0, FBUFSZ * sizeof(char));
				
				// read until end of file or error
				//n = read(fd, fbuf, FBUFSZ);
				if(((n = fread(fbuf, 1, FBUFSZ, fPtr)) == 0) || (ferror(fPtr) != 0)){
					clearerr(fPtr);
					break;
				}
				
				// send data			
				if(sendn(conn_fd, fbuf, n, MSG_NOSIGNAL) != n){
					err_msg("(%s) - error in transmission", prog_name);
					break;
				}
				sent += n;
			}
			//close(fd);
			fclose(fPtr);
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

void handle_sigchld(int sig) {
  int saved_errno = errno;
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
  errno = saved_errno;
  // decrese child counter
  nchild--;
}
	
		
		
