#include "my_sockwrap.h"
#include "errlib.h"

#define SENDSZ 8192
#define MSGSZ 2048
#define MAXFNAME 256
#define INFOSZ 13
#define GET "GET "
#define END "\r\n"
#define ERROR "-ERR\r\n"
#define OK "+OK\r\n"
#define QUIT "QUIT\r\n"
#define CHMAX 3

typedef int SOCKET;

char *prog_name;
int nchild;

int sendFile(SOCKET c, char *fname);
int parseMessage(char *recv_buf, char *fname);
int getFileInfo(char *fname, char *finfo);
void service(int conn_fd);

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
				service(conn_fd);			/* serve client */
				exit(0);
			}	
		}
	}
	
	// out of server loop
	return 0;
}


void service(int conn_fd){
	
	char fname[MAXFNAME];
	char finfo[INFOSZ];
	char recv_buf[MSGSZ];
	ssize_t n;
	int break_flag = 0;
	
	while(1){
			
		// clean fname and buffer for recv and finfo for send
		memset(fname, 0, MAXFNAME);
		memset(recv_buf, 0, MSGSZ);
		memset(finfo, 0, INFOSZ);
			
		printf("(%s) - Waiting for file request...\n", prog_name);
			
		// receve max MSGSZ-1 bytes because last one in buffer will be set to \0
		if((n = recv(conn_fd, recv_buf, MSGSZ-1, 0)) == -1){
			err_msg("(%s) - error in recv", prog_name);
			continue;
		}
		// if no data is received client has closed connection, si break client loop
		if(n == 0){
			err_msg("(%s) - connection closed by client", prog_name);
			break;
		}
		// terminate received string
		recv_buf[n] = '\0';
			
		switch(parseMessage(recv_buf, fname)){
			case -1:
				err_msg("(%s) - incorrect format in request", prog_name);
				if(sendn(conn_fd, ERROR, strlen(ERROR), MSG_NOSIGNAL) != strlen(ERROR)){
					err_msg("(%s) - could not send back error message", prog_name);
					break_flag = 1;
				}
				break;
			case 1:
				printf("(%s) - client requested end of communication\n", prog_name);
				break_flag = 1;
				break;
			case 0:
				printf("File name received: %s\n", fname);
				// try to get file info
				if(getFileInfo(fname, finfo) == -1){
					err_msg("(%s) - file not found or error in retrieving file info", prog_name);
					if(sendn(conn_fd, ERROR, strlen(ERROR), 0) != strlen(ERROR)){
						err_msg("(%s) - could not send back error message", prog_name);
						break_flag = 1;
					}
				}
				else{
					// send file info
					if(sendn(conn_fd, finfo, INFOSZ, MSG_NOSIGNAL) != INFOSZ){
						err_msg("(%s) - error while transfering file infos...", prog_name);
						break_flag = 1;
					}
					else{
						switch(sendFile(conn_fd, fname)){
							case -1:
								err_msg("(%s) - error in opening file to transfer...", prog_name);
								break_flag = 1;
								break;
							case -2:
								err_msg("(%s) - error in reading of file...", prog_name);
								break_flag = 1;
								break;
							case -3:
								err_msg("(%s) - error while sending data to client...", prog_name);
								break_flag = 1;
								break;
							case 0:
								printf("(%s) - file sent to client\n", prog_name);
								break;
						}
					}
				}
				break;
		}
			
		// if some error occurred or client requested connection closing break client loop
		if(break_flag)
			break;	
	}
		
	// out of client loop close client socket
	close(conn_fd);
	printf("(%s) - Client socket closed.\n", prog_name);
	return;
}

int parseMessage(char *recv_buf, char *fname){
	
	size_t n = 0;
	int quitsz = strlen(QUIT);
	int getsz = strlen(GET);
	
	// check if client requested end of communication
	if(strncmp(recv_buf, QUIT, quitsz) == 0){
		return 1;
	}
	
	// check if message format is correct
	if(strncmp(recv_buf, GET, getsz) != 0){
		return -1;
	}
	
	// cut trailer from recv_buf
	n = strcspn(recv_buf, END);
	recv_buf[n] = '\0';
	strncpy(fname, &recv_buf[getsz], MAXFNAME);
	// terminate copied string in case source was longer than destination
	fname[MAXFNAME-1] = '\0';
	return 0;
}
	
int sendFile(SOCKET c, char *fname){
	
	int fd;
	ssize_t nbytes;
	char *buf = malloc(SENDSZ * sizeof(char));
	
	if((fd = open(fname, O_RDONLY)) == -1){
		return -1;
	}
	
	do{
		// error in reading file
		if((nbytes = read(fd, buf, SENDSZ)) == -1){
			return -2;
		}
		// there is data to send
		if(nbytes > 0){
			// error while sending data to socket
			if(sendn(c, buf, nbytes, MSG_NOSIGNAL) != nbytes){
				return -3;
			}
		}
	}while(nbytes > 0);
	free(buf);
	
	return 0;
}
	
int getFileInfo(char *fname, char *finfo){
	
	struct stat statbuf;
	uint32_t fsize_n, lastmod_n;
	
	if(stat(fname, &statbuf) == -1){
		return -1;
	}
	
	
	fsize_n = htonl(statbuf.st_size);
	lastmod_n = htonl(statbuf.st_mtim.tv_sec);
	
	// build info message
	memcpy(finfo, OK, strlen(OK));
	memcpy(&finfo[strlen(OK)], &fsize_n, sizeof(fsize_n));
	memcpy(&finfo[strlen(OK) + sizeof(fsize_n)], &lastmod_n, sizeof(lastmod_n));
	return 0;
}

void handle_sigchld(int sig) {
  int saved_errno = errno;
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
  errno = saved_errno;
  // decrese child counter
  nchild--;
}
	
		
		
