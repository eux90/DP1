#include "my_sockwrap.h"
#include "errlib.h"

#define SENDSZ 8192
#define MSGSZ 2048
#define MAXFNAME 256
#define INFOSZ 13
#define MAXPROCS 10
#define WAIT_SEC 120
#define GET "GET "
#define END "\r\n"
#define ERROR "-ERR\r\n"
#define OK "+OK\r\n"
#define QUIT "QUIT\r\n"

typedef int SOCKET;

char *prog_name;

int sendFile(SOCKET c, char *fname);
int parseMessage(char *recv_buf, char *fname);
int getFileInfo(char *fname, char *finfo);
void service(int conn_fd);

int main(int argc, char *argv[]){
	
	// socket vars
	SOCKET listen_fd;
	struct sockaddr_storage saddr;
	socklen_t saddr_len;
	
	// mutliprocess management
	int	childpid;		/* pid of child process */
	int i = 0;
	int nprocs;
	
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
	    
			service(listen_fd);

		}
    }
    printf("(%s) - Process pool created\n", prog_name);
    // keep main process open until childs are alive
    for(i=0; i<nprocs; i++){
		waitpid((pid_t)(-1), 0, 0);
	}
}


void service(int listen_fd){
	
	// socket vars
	SOCKET conn_fd;
	struct sockaddr_storage caddr;
	socklen_t caddr_len;
	
	// data vars
	char fname[MAXFNAME];
	char finfo[INFOSZ];
	char recv_buf[MSGSZ];
	ssize_t n;
	
	//flags
	int break_flag = 0;
	
	//select vars
	struct timeval tv;
	fd_set rfds;
	int retval;
	
	memset(&caddr, 0, sizeof(caddr));
	caddr_len = sizeof(caddr);
	
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

		printf("(%s) - Client connected: %s\n", prog_name, sock_ntop((SA *) &caddr, caddr_len));
		while(1){
			
			// clean fname and buffer for recv and finfo for send
			memset(fname, 0, MAXFNAME);
			memset(recv_buf, 0, MSGSZ);
			memset(finfo, 0, INFOSZ);
			
			// set select parameters
			FD_ZERO(&rfds);
			FD_SET(conn_fd, &rfds);
			tv.tv_sec = WAIT_SEC;
			tv.tv_usec = 0;
			
			printf("(%s) - Waiting for file request...\n", prog_name);
			
			// wait for input
			retval = select(FD_SETSIZE, &rfds, 0, 0, &tv);
			if(retval == -1){
				err_msg("(%s) - select failed", prog_name);
				break;
			}
			if(retval == 0){
				printf("(%s) - Timeout on client connection\n", prog_name);
				break;
			}
			
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
		// reset break_flag (IMPORTANT)
		break_flag = 0;
	}
	
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
	
		
		
