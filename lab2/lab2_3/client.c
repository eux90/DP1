#include "my_sockwrap.h"
#include "errlib.h"

#define BUFSZ 512
#define MAXFNAME 256
#define GET "GET "
#define END "\r\n"
#define OK "+OK\r\n"
#define ERROR "-ERR\r\n"
#define COPY "_new"

typedef int SOCKET;
char *prog_name;

void buildRequest(char *buf, char *fname);
int readFileinfo(SOCKET s, uint32_t *fsize, uint32_t *fdate);
int copyFile(SOCKET s, char *buf, uint32_t fsize, char *fname);

int main(int argc, char *argv[]){
	
	struct sockaddr_storage saddr;
	socklen_t saddr_len;
	SOCKET s;
	char buf[BUFSZ];
	char fname[MAXFNAME];
	int ok_sz = strlen(OK);
	int error_sz = strlen(ERROR);
	uint32_t fsize, fdate;
	
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
		printf("(%s) - enter file name to request (max %d chars will be sent):\n",prog_name, MAXFNAME);
		
		// clean buffer
		memset(buf, 0, BUFSZ * sizeof(char));
		buildRequest(buf, fname);
		//printf("Message: %s\n", buf);
		
		// send file request
		Sendn(s, buf, strlen(buf), 0);
		
		// receive response
		if(Recv(s, buf, ok_sz, MSG_WAITALL) != ok_sz){
			err_msg("(%s) - error recv() red less bytes than expected", prog_name);
			Close(s);
			return -1;
		}
		
		// terminate received string
		buf[ok_sz] = '\0';
		
		//printf("Received: %s", buf);
		
		// check if file has been found
		if(strcmp(buf, OK) != 0){
			// receive rest of message
			Recv(s, &buf[ok_sz], (error_sz - ok_sz), MSG_WAITALL);
			buf[error_sz] = '\0';
			if(strcmp(buf, ERROR) != 0){
				err_msg("(%s) - undefined server response", prog_name);
				Close(s);
				return -1;
			}
			err_msg("(%s) - illegal command or not existing file", prog_name);
			continue;
		}
		
		// get file infos
		if(readFileinfo(s, &fsize, &fdate) == -1){
			err_msg("(%s) - could not get file infos", prog_name);
			Close(s);
			return -1;
		}
		
		if(copyFile(s,buf,fsize,fname) == -1){
			err_msg("(%s) - error in file transfer", prog_name);
			Close(s);
			return -1;
		}
		
		printf("(%s) - file received\n", prog_name);
	}

	return 0;
}

int copyFile(SOCKET s, char *buf, uint32_t fsize, char *fname){
	
	ssize_t n;
	int fd;
	char *new_name = malloc(sizeof(char) * (MAXFNAME + strlen(COPY)));
	strcpy(new_name,fname);
	strcat(new_name, COPY);
	
	if((fd = open(new_name, O_CREAT | O_WRONLY, 0775)) == -1){
		free(new_name);
		return -1;
	}
	
	do{
		n = Recv(s, buf, BUFSZ, 0);
		if(write(fd, buf, n) != n){
			free(new_name);
			Close(fd);
			return -1;
		}
		fsize-=n;
		printf("(%s) - Read: %ld bytes, remaining %d bytes\n",prog_name, n, fsize);
	}while(fsize > 0);
	free(new_name);
	Close(fd);
	return 0;
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



void buildRequest(char *buf, char *fname){
	
	int n;
	size_t limit;
	
	Fgets(fname, MAXFNAME, stdin);
	// remove \r or \n or \r\n
	limit = strcspn(fname, "\r\n");
	fname[limit] = '\0';
	
	// build message inside buf
	strcpy(buf, GET);
	n = strlen(buf);
	strncpy(&buf[n], fname, MAXFNAME);
	n = strlen(buf);
	strcpy(&buf[n], END);
	//printf("message %s", buf);
	return;
}
	
	
	
	
	
	
	
	
