#include "my_sockwrap.h"
#include "errlib.h"

#define BUFSZ 8192
#define MAXFNAME 256
#define GET "GET "
#define END "\r\n"
#define OK "+OK\r\n"
#define ERROR "-ERR\r\n"
#define COPY "_new"
#define QUIT "QUIT\r\n"

typedef int SOCKET;
char *prog_name;

int buildRequest(char *buf, char *fname);
int readFileinfo(SOCKET s, uint32_t *fsize, uint32_t *fdate);
int copyFile(SOCKET s, char *buf, uint32_t fsize, char *fname);

int main(int argc, char *argv[]){
	
	// sock vars
	struct sockaddr_storage saddr;
	socklen_t saddr_len;
	SOCKET s;
	ssize_t rcv;
	
	// buffers
	char msg_buf[BUFSZ];
	char data_buf[BUFSZ];
	char fname[MAXFNAME];
	
	// messages sizes
	int ok_sz = strlen(OK);
	int error_sz = strlen(ERROR);
	int quit_sz = strlen(QUIT);
	
	// flags
	int break_flag = 0;
	int status_flag = 0;
	
	// select vars
	fd_set rfds;
	
	// file vars
	uint32_t fsize, fdate, r_data;
	ssize_t n;
	int fd;
	char *new_name;
	
	prog_name = argv[0];
	
	// clean saddr and set saddrlen
	memset(&saddr, 0, sizeof(saddr));
	saddr_len = sizeof(saddr);
	
	if(argc != 3){
		err_quit("Usage: %s <address> <port>", prog_name);
	}
	
	// connect to server
	s = Tcp_connect(argv[1], argv[2]);
	Getpeername(s, (SA *)&saddr, &saddr_len);
	printf("(%s) - Connected to: %s\n", prog_name, Sock_ntop((SA *)&saddr, saddr_len));
	
	while(1){
		
		FD_ZERO(&rfds);
		FD_SET(s, &rfds);
		FD_SET(fileno(stdin), &rfds);
		
		Select(FD_SETSIZE, &rfds, 0, 0, NULL);
		
		// read data from console if present
		if(FD_ISSET(fileno(stdin), &rfds)){
			
			// clean msg buffer
			memset(msg_buf, 0, BUFSZ * sizeof(char));
			//memset(fname, 0, MAXFNAME * sizeof(char));
			
			switch(buildRequest(msg_buf, fname)){
				// correct file request
				case 0:
					Sendn(s, msg_buf, strlen(msg_buf), 0);
					break;
				// clean quit request (wait active file transfers than ask server to close connection)
				case 1:
					printf("(%s) - Sending QUIT message\n", prog_name);
					Sendn(s, QUIT, quit_sz, 0);
					break;
				// force quit request (do not wait for pending file transfers)
				case -1:
					printf("(%s) - Forcing quit...\n", prog_name);
					Close(s);
					break_flag = 1;
					break;
				// wrong command received
				case -2:
					printf("%s - Wrong command try again...\n", prog_name);
					break;
			}
		}
		
		// exit if force quit requested
		if(break_flag)
			break;
		
		
		// read data from socket if present
		if(FD_ISSET(s, &rfds)){
			
			// clean data buffer
			memset(data_buf, 0, BUFSZ * sizeof(char));
			//memset(fname, 0, MAXFNAME * sizeof(char));
			
			switch(status_flag){
				
				// there is not a file transfer pending
				case 0:
					// receive response
					rcv = Recv(s, data_buf, ok_sz, MSG_WAITALL);
					
					if(rcv == 0){
						printf("(%s) - connection closed by server\n", prog_name);
						Close(s);
						return 0;
					}
					
					else if(rcv != ok_sz){
						err_msg("(%s) - error recv() red less bytes than expected", prog_name);
						Close(s);
						return -1;
					}
					
					else{
						// terminate received string
						data_buf[ok_sz] = '\0';
						
						//file not found or not recognised response..
						if(strcmp(data_buf, OK) != 0){
							// receive rest of message
							Recv(s, &data_buf[ok_sz], (error_sz - ok_sz), MSG_WAITALL);
							data_buf[error_sz] = '\0';
							if(strcmp(data_buf, ERROR) != 0){
								err_msg("(%s) - undefined server response", prog_name);
								Close(s);
								return -1;
							}
							err_msg("(%s) - illegal command or not existing file", prog_name);
						}
						// file found...
						else{
							// get file infos
							if(readFileinfo(s, &fsize, &fdate) == -1){
								err_msg("(%s) - could not get file infos", prog_name);
								Close(s);
								return -1;
							}
							
							// open a file for writing
							new_name = malloc(sizeof(char) * (MAXFNAME + strlen(COPY)));
							strcpy(new_name,fname);
							strcat(new_name, COPY);
							
							if((fd = open(new_name, O_CREAT | O_WRONLY, 0775)) == -1){
								free(new_name);
								return -1;
							}
							// set quantity of data to transfer
							r_data = fsize;
							// set flag so that at next iteration data is read
							status_flag = 1;
						}
					}
					break;
				
				// there is a file transfer active...
				case 1:
					// read data
					n = Recv(s, data_buf, BUFSZ, 0);
					// write data to file
					if(write(fd, data_buf, n) != n){
						free(new_name);
						Close(fd);
						return -1;
					}
					r_data-=n;
					// if no more data to read close file
					if(r_data <= 0){
						Close(fd);
						printf("(%s) - file received: %s, Size: %d, Time last-mod: %d\n", prog_name, new_name, fsize, fdate);
						free(new_name);
						status_flag = 0;
					}
					break;
			}
		}
	}
	return 0;
}

int buildRequest(char *msg_buf, char *fname){
	
	int n;
	size_t limit;
	char cmd[BUFSZ];
	
	// read command from keyboard
	Fgets(cmd, BUFSZ, stdin);
		
	if((strncmp(cmd, "GET ", 4) == 0) || (strncmp(cmd, "get ", 4) == 0)){
		memcpy(fname, &(cmd[4]), MAXFNAME);
		limit = strcspn(fname, "\r\n");
		fname[limit] = '\0';
		
		// build message for server inside msg_buf
		strcpy(msg_buf, GET);
		n = strlen(msg_buf);
		strncpy(&msg_buf[n], fname, MAXFNAME);
		n = strlen(msg_buf);
		strcpy(&msg_buf[n], END);
		return 0;
	}
	else if(cmd[0] == 'Q' || cmd[0] == 'q'){
		return 1;
	}
	else if(cmd[0] == 'A' || cmd[0] == 'a'){
		return -1;
	}
	return -2;
}
	

int readFileinfo(SOCKET s, uint32_t *fsize, uint32_t *fdate){
	
	uint32_t fdate_n, fsize_n;
	
	if(Recv(s, &fsize_n, sizeof(fsize_n), MSG_WAITALL) != sizeof(fsize_n)){
		return -1;
	}
	
	if(Recv(s, &fdate_n, sizeof(fdate_n), MSG_WAITALL) != sizeof(fdate_n)){
		return -1;
	}
	
	*fsize = ntohl(fsize_n);
	*fdate = ntohl(fdate_n);
	
	return 0;
}
	
	
	
	
	
	
	
	
