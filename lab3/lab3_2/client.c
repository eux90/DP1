#include "my_sockwrap.h"
#include "errlib.h"
#include <time.h>

#define BUFSZ 8192
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

void prot_a_multiplex(SOCKET s);
char *print_time(uint32_t fdate);

int main(int argc, char *argv[]){

	// sock vars
	struct sockaddr_storage saddr;
	socklen_t saddr_len;
	SOCKET s;

	// initialize variables
	memset(&saddr, 0, sizeof(saddr));
	saddr_len = sizeof(saddr);
	prog_name = argv[0];

	if(argc != 3){
		err_quit("Usage: %s <address> <port>", prog_name);
	}

	// connect to server
	s = Tcp_connect(argv[1], argv[2]);
	Getpeername(s, (SA *)&saddr, &saddr_len);
	printf("(%s) - Connected to: %s\n", prog_name, Sock_ntop((SA *)&saddr, saddr_len));

	prot_a_multiplex(s);

	return 0;
}

void prot_a_multiplex(SOCKET s){

	// data vars
	char rk_buf[BUFSZ];
	char w_buf[BUFSZ];
	char r_buf[BUFSZ];
	char fname[MAXFNAME];
	ssize_t n;

	// messages sizes
	int ok_sz = strlen(OK_M);
	int error_sz = strlen(ERROR_M);
	int end_sz = strlen(END_M);
	int quit_sz = strlen(QUIT_M);
	int get_sz = strlen(GET_M);

	// file vars
	int fd;
	uint32_t fsize, fdate, fsize_n, fdate_n, r_data;

	// select vars
	fd_set rfds;

	//flag vars
	int status_flag = 0;
	
	printf("\n*******************************************************\n");
	printf("(%s) - RETRIEVE FILE:\t\t get <filename>\n", prog_name);
	printf("(%s) - QUIT GRACEFULLY:\t\t q\n", prog_name);
	printf("(%s) - FORCE QUIT:\t\t a\n", prog_name);
	printf("*******************************************************\n\n");

	while(1){

		// initialize select vars
		FD_ZERO(&rfds);
		FD_SET(s, &rfds);
		FD_SET(fileno(stdin), &rfds);

		Select(FD_SETSIZE, &rfds, 0, 0, NULL);

		// read data from console if present
		if(FD_ISSET(fileno(stdin), &rfds)){

			memset(&rk_buf, 0, BUFSZ);
			Fgets(rk_buf, BUFSZ, stdin);

			// QUIT request
			if(rk_buf[0] == 'Q' || rk_buf[0] == 'q'){
				// build QUIT message
				memset(w_buf, 0, BUFSZ * sizeof(char));
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
				memset(fname, 0, MAXFNAME * sizeof(char));
				memcpy(fname, &rk_buf[4], MAXFNAME);
				n = strcspn(fname, "\r\n");
				fname[n] = '\0';
				fname[MAXFNAME-1] = '\0';
				// build GET message
				memset(w_buf, 0, BUFSZ * sizeof(char));
				strncpy(w_buf, GET_M, get_sz);
				n = strlen(w_buf);
				strncpy(&w_buf[n], fname, MAXFNAME);
				n = strlen(w_buf);
				strncpy(&w_buf[n], END_M, end_sz);
				// send message
				Sendn(s, w_buf, strlen(w_buf), 0);
				printf("(%s) - GET message sent\n", prog_name);
			}
		}

		// read data from socket if present
		if(FD_ISSET(s, &rfds)){
			// there is not a file transfer pending
			if(status_flag == 0){
				// receive response
				memset(r_buf, 0, BUFSZ * sizeof(char));
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

				printf("(%s) - file %s found, Size: %u, Last mod: %s\n", prog_name, fname, fsize, print_time(fdate));

				// open a file for writing
				if((fd = open(fname, O_CREAT | O_WRONLY, 0775)) == -1){
					err_msg("(%s) - could not create a file for writing...", prog_name);
					break;
				}
				// set flag so that at next iteration data is read
				r_data = fsize;
				status_flag = 1;
			}

			// there is a file transfer active...
			else{
				memset(r_buf, 0, BUFSZ * sizeof(char));
				n = Recv(s, r_buf, BUFSZ, 0);
				// write data to file
				if(write(fd, r_buf, n) != n){
					err_msg("(%s) - write failed...", prog_name);
					break;
				}
				r_data-=n;
				// if no more data to read close file
				if(r_data <= 0){
					Close(fd);
					printf("(%s) - file correctly received\n", prog_name);
					status_flag = 0;
				}
			}
		}
	}
	close(s);
	return;
}

char *print_time(uint32_t fdate){

	time_t time = fdate;
	return ctime(&time);
}
