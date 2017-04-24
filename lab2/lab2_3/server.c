#include "my_sockwrap.h"
#include "errlib.h"

#define BUFSZ 8192
#define MAXFNAME 256
#define INFOSZ 13
#define GET "GET "
#define END "\r\n"
#define ERROR "-ERR\r\n"
#define OK "+OK\r\n"
#define QUIT "QUIT\r\n"

typedef int SOCKET;

char *prog_name;

int sendFile(SOCKET c, char *fname);
int getFileName(SOCKET c, char *fname);
int getFileInfo(char *fname, char *finfo);

int main(int argc, char *argv[]){
	
	SOCKET listen_fd,conn_fd;
	struct sockaddr_storage saddr,caddr;
	socklen_t saddr_len,caddr_len;
	
	char fname[MAXFNAME];
	char finfo[INFOSZ];
	int break_flag=0;
	
	prog_name = argv[0];
	
	memset(&saddr, 0, sizeof(saddr));
	memset(&caddr, 0, sizeof(caddr));
	saddr_len = sizeof(saddr);
	caddr_len = sizeof(caddr);
	
	if(argc != 2){
		err_quit("Usage: %s <port>", prog_name);
	}
	
	// listen for connections
	listen_fd = Tcp_listen(INADDR_ANY, argv[1], &saddr_len);
	Getsockname(listen_fd, (SA *)&saddr, &saddr_len);
	printf("(%s) - Server started at %s\n", prog_name, sock_ntop((SA *) &saddr, saddr_len));
	
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
			
			// reset break flag important
			break_flag = 0;
			
			printf("(%s) - Waiting for file request...\n", prog_name);
			switch(getFileName(conn_fd, fname)){
				// error in server
				case -1:
					err_msg("(%s) - error in recv", prog_name);
					break_flag = 1;
					break;
				// client closes connection
				case -2:
					err_msg("(%s) - connection closed by client", prog_name);
					break_flag = 1;
					break;
				// wrong command
				case -3:
					err_msg("(%s) - incorrect format in request", prog_name);
					if(sendn(conn_fd, ERROR, strlen(ERROR), MSG_NOSIGNAL) != strlen(ERROR)){
						err_msg("(%s) - could not send back error message", prog_name);
						break_flag = 1;
					}
					break;
				// ok filename in fname
				case 1:
					printf("(%s) - client requested end of communication\n", prog_name);
					break_flag = 1;
					break;
				case 0:
					break;
			}
			if(break_flag){
				break;
			}
					
			//printf("File name: %s\n", fname);
			
			// get file info (size, lastmod)
			if(getFileInfo(fname, finfo) == -1){
				err_msg("(%s) - file not found or error in retrieving file info", prog_name);
				if(sendn(conn_fd, ERROR, strlen(ERROR), 0) != strlen(ERROR)){
					err_msg("(%s) - could not send back error message", prog_name);
					break;
				}
				continue;
			}
			
			// send file info
			if(sendn(conn_fd, finfo, INFOSZ, MSG_NOSIGNAL) != INFOSZ){
				err_msg("(%s) - error while transfering file infos...", prog_name);
				break;
			}
			
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
					break;
			}
			if(break_flag){
				break;
			}
			printf("(%s) - file sent to client\n", prog_name);
		}
		
		// out of client loop
		if(close(conn_fd) != 0){
			err_msg ("(%s) error - close() failed", prog_name);
		}
		else{
			printf("(%s) - Client socket closed.\n", prog_name);
		}
	}
	
	// out of server loop
	return 0;
}

int sendFile(SOCKET c, char *fname){
	
	int fd;
	ssize_t nbytes;
	char *buf = malloc(BUFSZ * sizeof(char));
	
	if((fd = open(fname, O_RDONLY)) == -1){
		return -1;
	}
	
	do{
		// error in reading file
		if((nbytes = read(fd, buf, BUFSZ)) == -1){
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
		
	

int getFileName(SOCKET c, char *fname){
	
	ssize_t n = 0;
	char *buf = malloc(BUFSZ * sizeof(char));
	char *name = malloc(BUFSZ * sizeof(char));

	// get file request
	if((n = recv(c, buf, BUFSZ-1, 0)) == -1){
		return -1;
	}
	// terminate string
	buf[n] = '\0';
	
	// detect closing by client
	if(n==0){
		return -2;
	}
	
	if(strcmp(buf, QUIT) == 0){
		return 1;
	}
	
	// extract file name from buffer if request is correct
	if(sscanf(buf, GET"%s"END, name) != 1){
		return -3;
	}
	
	//printf("File name received: %s\n", name);
	
	strncpy(fname, name, MAXFNAME);
	// terminate string if source exceed max fname size
	fname[MAXFNAME - 1] = '\0';
	free(buf);
	free(name);
	return 0;
}
	
		
		
