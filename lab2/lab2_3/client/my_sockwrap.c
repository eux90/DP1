/*
 
 module: sockwrap.c
 
 purpose: library of wrapper and utility socket functions
          wrapper functions include error management
 
 reference: Stevens, Unix network programming (3ed)
 
 */


#include <stdlib.h> // getenv()
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h> // timeval
#include <sys/select.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h> // inet_aton()
#include <sys/un.h> // unix sockets
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "errlib.h"
#include "my_sockwrap.h"

extern char *prog_name;

int Socket (int family, int type, int protocol)
{
	int n;
	if ( (n = socket(family,type,protocol)) < 0)
		err_sys ("(%s) error - socket() failed", prog_name);
	return n;
}

void Bind (int sockfd, const SA *myaddr,  socklen_t myaddrlen)
{
	if ( bind(sockfd, myaddr, myaddrlen) != 0)
		err_sys ("(%s) error - bind() failed", prog_name);
}

void Listen (int sockfd, int backlog)
{
	char *ptr;
	if ( (ptr = getenv("LISTENQ")) != NULL)
		backlog = atoi(ptr);
	if ( listen(sockfd,backlog) < 0 )
		err_sys ("(%s) error - listen() failed", prog_name);
}


int Accept (int listen_sockfd, SA *cliaddr, socklen_t *addrlenp)
{
	int n;
again:
	if ( (n = accept(listen_sockfd, cliaddr, addrlenp)) < 0)
	{
		if (INTERRUPTED_BY_SIGNAL ||
			errno == EPROTO || errno == ECONNABORTED ||
			errno == EMFILE || errno == ENFILE ||
			errno == ENOBUFS || errno == ENOMEM			
		    )
			goto again;
		else
			err_sys ("(%s) error - accept() failed", prog_name);
	}
	return n;
}


void Connect (int sockfd, const SA *srvaddr, socklen_t addrlen)
{
	if ( connect(sockfd, srvaddr, addrlen) != 0)
		err_sys ("(%s) error - connect() failed", prog_name);
}


void Close (int fd)
{
	if (close(fd) != 0)
		err_sys ("(%s) error - close() failed", prog_name);
}


void Shutdown (int fd, int howto)
{
	if (shutdown(fd,howto) != 0)
		err_sys ("(%s) error - shutdown() failed", prog_name);
}


ssize_t Read (int fd, void *bufptr, size_t nbytes)
{
	ssize_t n;
again:
	if ( (n = read(fd,bufptr,nbytes)) < 0)
	{
		if (INTERRUPTED_BY_SIGNAL)
			goto again;
		else
			err_sys ("(%s) error - read() failed", prog_name);
	}
	return n;
}


void Write (int fd, void *bufptr, size_t nbytes)
{
	if (write(fd,bufptr,nbytes) != nbytes)
		err_sys ("(%s) error - write() failed", prog_name);
}

ssize_t Recv(int fd, void *bufptr, size_t nbytes, int flags)
{
	ssize_t n;

	if ( (n = recv(fd,bufptr,nbytes,flags)) < 0)
		err_sys ("(%s) error - recv() failed", prog_name);
	return n;
}

ssize_t Recvfrom (int fd, void *bufptr, size_t nbytes, int flags, SA *sa, socklen_t *salenptr)
{
	ssize_t n;

	if ( (n = recvfrom(fd,bufptr,nbytes,flags,sa,salenptr)) < 0)
		err_sys ("(%s) error - recvfrom() failed", prog_name);
	return n;
}

void Sendto (int fd, void *bufptr, size_t nbytes, int flags, const SA *sa, socklen_t salen)
{
	if (sendto(fd,bufptr,nbytes,flags,sa,salen) != (ssize_t)nbytes)
		err_sys ("(%s) error - sendto() failed", prog_name);
}

void Send (int fd, void *bufptr, size_t nbytes, int flags)
{
	if (send(fd,bufptr,nbytes,flags) != (ssize_t)nbytes)
		err_sys ("(%s) error - send() failed", prog_name);
}


void Inet_aton (const char *strptr, struct in_addr *addrptr) {

	if (inet_aton(strptr, addrptr) == 0)
		err_quit ("(%s) error - inet_aton() failed for '%s'", prog_name, strptr);
}

void Inet_pton (int af, const char *strptr, void *addrptr)
{
	int status = inet_pton(af, strptr, addrptr);
	if (status == 0)
		err_quit ("(%s) error - inet_pton() failed for '%s': invalid address", prog_name, strptr);
	if (status < 0)
		err_sys ("(%s) error - inet_pton() failed for '%s'", prog_name, strptr);
}


void Inet_ntop (int af, const void *addrptr, char *strptr, size_t length)
{
	if ( inet_ntop(af, addrptr, strptr, length) == NULL)
		err_quit ("(%s) error - inet_ntop() failed: invalid address", prog_name);
}

/* Added by Enrico Masala <masala@polito.it> Apr 2011 */
#define MAXSTR 1024
void Print_getaddrinfo_list(struct addrinfo *list_head) {
	struct addrinfo *p = list_head;
	char info[MAXSTR];
	char tmpstr[MAXSTR];
	err_msg ("(%s) listing all results of getaddrinfo", prog_name);	
	while (p != NULL) {
		info[0]='\0';

		int ai_family = p->ai_family;
		if (ai_family == AF_INET)
			strcat(info, "AF_INET  ");
		else if (ai_family == AF_INET6)
			strcat(info, "AF_INET6 ");
		else
			{ sprintf(tmpstr, "(fam=%d?) ",ai_family); strcat(info, tmpstr); }

		int ai_socktype = p->ai_socktype;
		if (ai_socktype == SOCK_STREAM)
			strcat(info, "SOCK_STREAM ");
		else if (ai_socktype == SOCK_DGRAM)
			strcat(info, "SOCK_DGRAM  ");
		else if (ai_socktype == SOCK_RAW)
			strcat(info, "SOCK_RAW    ");
		else
			{ sprintf(tmpstr, "(sock=%d?) ",ai_socktype); strcat(info, tmpstr); }

		int ai_protocol = p->ai_protocol;
		if (ai_protocol == IPPROTO_UDP)
			strcat(info, "IPPROTO_UDP ");
		else if (ai_protocol == IPPROTO_TCP)
			strcat(info, "IPPROTO_TCP ");
		else if (ai_protocol == IPPROTO_IP)
			strcat(info, "IPPROTO_IP  ");
		else
			{ sprintf(tmpstr, "(proto=%d?) ",ai_protocol); strcat(info, tmpstr); }

		char text_addr[INET6_ADDRSTRLEN];
		if (ai_family == AF_INET)
			Inet_ntop(ai_family, &(((struct sockaddr_in *)p->ai_addr)->sin_addr), text_addr, sizeof(text_addr));
		else if (ai_family == AF_INET6)
			Inet_ntop(ai_family, &(((struct sockaddr_in6 *)p->ai_addr)->sin6_addr), text_addr, sizeof(text_addr));
		else
			strcpy(text_addr, "(addr=?)");
		//Inet_ntop(AF_INET6, p->ai_addr, text_addr, sizeof(text_addr));
		strcat(info, text_addr);
		strcat(info, " ");

		if (p->ai_canonname != NULL) {
			strcat(info, p->ai_canonname);
			strcat(info, " ");
		}
			
		err_msg ("(%s) %s", prog_name, info);
		p = p->ai_next;
	}
}

#ifndef MAXLINE
#define MAXLINE 1024
#endif

/* reads exactly "n" bytes from a descriptor */
ssize_t readn (int fd, void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nread;
	char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0)
	{
		if ( (nread = read(fd, ptr, nleft)) < 0)
		{
			if (INTERRUPTED_BY_SIGNAL)
			{
				nread = 0;
				continue; /* and call read() again */
			}
			else
				return -1;
		}
		else
			if (nread == 0)
				break; /* EOF */

		nleft -= nread;
		ptr   += nread;
	}
	return n - nleft;
}


ssize_t Readn (int fd, void *ptr, size_t nbytes)
{
	ssize_t n;

	if ( (n = readn(fd, ptr, nbytes)) < 0)
		err_sys ("(%s) error - readn() failed", prog_name);
	return n;
}


/* read a whole buffer, for performance, and then return one char at a time */
static ssize_t my_read (int fd, char *ptr)
{
	static int read_cnt = 0;
	static char *read_ptr;
	static char read_buf[MAXLINE];

	if (read_cnt <= 0)
	{
again:
		if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0)
		{
			if (INTERRUPTED_BY_SIGNAL)
				goto again;
			return -1;
		}
		else
			if (read_cnt == 0)
				return 0;
		read_ptr = read_buf;
	}
	read_cnt--;
	*ptr = *read_ptr++;
	return 1;
}

/* NB: Use my_read (buffered recv from stream socket) to get data. Subsequent readn() calls will not behave as expected */
ssize_t readline (int fd, void *vptr, size_t maxlen)
{
	int n, rc;
	char c, *ptr;

	ptr = vptr;
	for (n=1; n<maxlen; n++)
	{
		if ( (rc = my_read(fd,&c)) == 1)
		{
			*ptr++ = c;
			if (c == '\n')
				break;	/* newline is stored, like fgets() */
		}
		else if (rc == 0)
		{
			if (n == 1)
				return 0; /* EOF, no data read */
			else
				break; /* EOF, some data was read */
		}
		else
			return -1; /* error, errno set by read() */
	}
	*ptr = 0; /* null terminate like fgets() */
	return n;
}


ssize_t Readline (int fd, void *ptr, size_t maxlen)
{
	ssize_t n;

	if ( (n = readline(fd, ptr, maxlen)) < 0)
		err_sys ("(%s) error - readline() failed", prog_name);
	return n;
}


ssize_t readline_unbuffered (int fd, void *vptr, size_t maxlen)
{
	int n, rc;
	char c, *ptr;

	ptr = vptr;
	for (n=1; n<maxlen; n++)
	{
		if ( (rc = recv(fd,&c,1,0)) == 1)
		{
			*ptr++ = c;
			if (c == '\n')
				break;	/* newline is stored, like fgets() */
		}
		else if (rc == 0)
		{
			if (n == 1)
				return 0; /* EOF, no data read */
			else
				break; /* EOF, some data was read */
		}
		else
			return -1; /* error, errno set by read() */
	}
	*ptr = 0; /* null terminate like fgets() */
	return n;
}


ssize_t Readline_unbuffered (int fd, void *ptr, size_t maxlen)
{
	ssize_t n;

	if ( (n = readline_unbuffered(fd, ptr, maxlen)) < 0)
		err_sys ("(%s) error - readline_unbuffered() failed", prog_name);
	return n;
}


ssize_t writen (int fd, const void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0)
	{
		if ( (nwritten = write(fd, ptr, nleft)) <= 0)
		{
			if (INTERRUPTED_BY_SIGNAL)
			{
				nwritten = 0;
				continue; /* and call write() again */
			}
			else
				return -1;
		}
		nleft -= nwritten;
		ptr   += nwritten;
	}
	return n;
}


void Writen (int fd, void *ptr, size_t nbytes)
{
	if (writen(fd, ptr, nbytes) != nbytes)
		err_sys ("(%s) error - writen() failed", prog_name);
}

ssize_t sendn (int fd, const void *vptr, size_t n, int flags)
{
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0)
	{
		if ( (nwritten = send(fd, ptr, nleft, flags)) <= 0)
		{
			if (INTERRUPTED_BY_SIGNAL)
			{
				nwritten = 0;
				continue; /* and call send() again */
			}
			else
				return -1;
		}
		nleft -= nwritten;
		ptr   += nwritten;
	}
	return n;
}


void Sendn (int fd, void *ptr, size_t nbytes, int flags)
{
	if (sendn(fd, ptr, nbytes, flags) != nbytes)
		err_sys ("(%s) error - writen() failed", prog_name);
}

int Select (int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout)
{
	int n;
again:
	if ( (n = select (maxfdp1, readset, writeset, exceptset, timeout)) < 0)
	{
		if (INTERRUPTED_BY_SIGNAL)
			goto again;
		else
			err_sys ("(%s) error - select() failed", prog_name);
	}
	return n;
}


pid_t Fork (void)
{
	pid_t pid;
	if ((pid=fork()) < 0)
		err_sys ("(%s) error - fork() failed", prog_name);
	return pid;
}

#ifdef SOLARIS
const char * hstrerror (int err)
{
	if (err == NETDB_INTERNAL)
		return "internal error - see errno";
	if (err == NETDB_SUCCESS)
		return "no error";
	if (err == HOST_NOT_FOUND)
		return "unknown host";
	if (err == TRY_AGAIN)
		return "hostname lookup failure";
	if (err == NO_RECOVERY)
		return "unknown server error";
	if (err == NO_DATA)
		return "no address associated with name";
	return "unknown error";
}
#endif

struct hostent *Gethostbyname (const char *hostname)
{
	struct hostent *hp;
	if ((hp = gethostbyname(hostname)) == NULL)
		err_quit ("(%s) error - gethostbyname() failed for '%s': %s",
				prog_name, hostname, hstrerror(h_errno));
	return hp;
}

void Getsockname (int sockfd, struct sockaddr *localaddr, socklen_t *addrp)
{
	if ((getsockname(sockfd, localaddr, addrp)) != 0)
		err_quit ("(%s) error - getsockname() failed", prog_name);
}

void Getaddrinfo ( const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
	int err_code;
	err_code = getaddrinfo(node, service, hints, res);
	if (err_code!=0) {
		err_quit ("(%s) error - getaddrinfo() failed  %s %s : (code %d) %s", prog_name, node, service, err_code, gai_strerror(err_code));
	}
}

void
Getpeername(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
	if (getpeername(fd, sa, salenptr) < 0)
		err_sys("getpeername error");
}

void
Getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlenptr)
{
	if (getsockopt(fd, level, optname, optval, optlenptr) < 0)
		err_sys("getsockopt error");
}

void
Setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
	if (setsockopt(fd, level, optname, optval, optlen) < 0)
		err_sys("setsockopt error");
}

char *
sock_ntop(const struct sockaddr *sa, socklen_t salen)
{
    char		portstr[8];
    static char str[128];		/* Unix domain is largest */

	switch (sa->sa_family) {
	case AF_INET: {
		struct sockaddr_in	*sin = (struct sockaddr_in *) sa;

		if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
			return(NULL);
		if (ntohs(sin->sin_port) != 0) {
			snprintf(portstr, sizeof(portstr), ":%d", ntohs(sin->sin_port));
			strcat(str, portstr);
		}
		return(str);
	}
/* end sock_ntop */


	case AF_INET6: {
		struct sockaddr_in6	*sin6 = (struct sockaddr_in6 *) sa;

		str[0] = '[';
		if (inet_ntop(AF_INET6, &sin6->sin6_addr, str + 1, sizeof(str) - 1) == NULL)
			return(NULL);
		if (ntohs(sin6->sin6_port) != 0) {
			snprintf(portstr, sizeof(portstr), "]:%d", ntohs(sin6->sin6_port));
			strcat(str, portstr);
			return(str);
		}
		return (str + 1);
	}



	case AF_UNIX: {
		struct sockaddr_un	*unp = (struct sockaddr_un *) sa;

			/* OK to have no pathname bound to the socket: happens on
			   every connect() unless client calls bind() first. */
		if (unp->sun_path[0] == 0)
			strcpy(str, "(no pathname bound)");
		else
			snprintf(str, sizeof(str), "%s", unp->sun_path);
		return(str);
	}


	default:
		snprintf(str, sizeof(str), "sock_ntop: unknown AF_xxx: %d, len %d",
				 sa->sa_family, salen);
		return(str);
	}
    return (NULL);
}

char *
Sock_ntop(const struct sockaddr *sa, socklen_t salen)
{
	char	*ptr;

	if ( (ptr = sock_ntop(sa, salen)) == NULL)
		err_sys("sock_ntop error");	/* inet_ntop() sets errno */
	return(ptr);
}

char *
sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
    static char str[128];		/* Unix domain is largest */

	switch (sa->sa_family) {
	case AF_INET: {
		struct sockaddr_in	*sin = (struct sockaddr_in *) sa;

		if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
			return(NULL);
		return(str);
	}

#ifdef	IPV6
	case AF_INET6: {
		struct sockaddr_in6	*sin6 = (struct sockaddr_in6 *) sa;

		if (inet_ntop(AF_INET6, &sin6->sin6_addr, str, sizeof(str)) == NULL)
			return(NULL);
		return(str);
	}
#endif

#ifdef	AF_UNIX
	case AF_UNIX: {
		struct sockaddr_un	*unp = (struct sockaddr_un *) sa;

			/* OK to have no pathname bound to the socket: happens on
			   every connect() unless client calls bind() first. */
		if (unp->sun_path[0] == 0)
			strcpy(str, "(no pathname bound)");
		else
			snprintf(str, sizeof(str), "%s", unp->sun_path);
		return(str);
	}
#endif

	default:
		snprintf(str, sizeof(str), "sock_ntop_host: unknown AF_xxx: %d, len %d",
				 sa->sa_family, salen);
		return(str);
	}
    return (NULL);
}

char *
Sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
	char	*ptr;

	if ( (ptr = sock_ntop_host(sa, salen)) == NULL)
		err_sys("sock_ntop_host error");	/* inet_ntop() sets errno */
	return(ptr);
}

void
Fclose(FILE *fp)
{
	if (fclose(fp) != 0)
		err_sys("fclose error");
}

char *
Fgets(char *ptr, int n, FILE *stream)
{
	char	*rptr;

	if ( (rptr = fgets(ptr, n, stream)) == NULL && ferror(stream))
		err_sys("fgets error");

	return (rptr);
}

FILE *
Fopen(const char *filename, const char *mode)
{
	FILE	*fp;

	if ( (fp = fopen(filename, mode)) == NULL)
		err_sys("fopen error");

	return(fp);
}

void
Fputs(const char *ptr, FILE *stream)
{
	if (fputs(ptr, stream) == EOF)
		err_sys("fputs error");
}

Sigfunc *
signal(int signo, Sigfunc *func)
{
	struct sigaction	act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (signo == SIGALRM) {
#ifdef	SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;	/* SunOS 4.x */
#endif
	} else {
#ifdef	SA_RESTART
		act.sa_flags |= SA_RESTART;		/* SVR4, 44BSD */
#endif
	}
	if (sigaction(signo, &act, &oact) < 0)
		return(SIG_ERR);
	return(oact.sa_handler);
}
/* end signal */

Sigfunc *
Signal(int signo, Sigfunc *func)	/* for our signal() function */
{
	Sigfunc	*sigfunc;

	if ( (sigfunc = signal(signo, func)) == SIG_ERR)
		err_sys("signal error");
	return(sigfunc);
}

int
tcp_connect(const char *host, const char *serv)
{
	int				sockfd, n;
	struct addrinfo	hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
		err_quit("tcp_connect error for %s, %s: %s",
				 host, serv, gai_strerror(n));
	ressave = res;

	do {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0)
			continue;	/* ignore this one */

		if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
			break;		/* success */

		Close(sockfd);	/* ignore this one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL)	/* errno set from final connect() */
		err_sys("tcp_connect error for %s, %s", host, serv);

	freeaddrinfo(ressave);

	return(sockfd);
}
/* end tcp_connect */

/*
 * We place the wrapper function here, not in wraplib.c, because some
 * XTI programs need to include wraplib.c, and it also defines
 * a Tcp_connect() function.
 */

int
Tcp_connect(const char *host, const char *serv)
{
	return(tcp_connect(host, serv));
}

int
tcp_listen(const char *host, const char *serv, socklen_t *addrlenp)
{
	int				listenfd, n;
	const int		on = 1;
	struct addrinfo	hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
		err_quit("tcp_listen error for %s, %s: %s",
				 host, serv, gai_strerror(n));
	ressave = res;

	do {
		listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (listenfd < 0)
			continue;		/* error, try next one */

		Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
			break;			/* success */

		Close(listenfd);	/* bind error, close and try next one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL)	/* errno from final socket() or bind() */
		err_sys("tcp_listen error for %s, %s", host, serv);

	Listen(listenfd, LISTENQ);

	if (addrlenp)
		*addrlenp = res->ai_addrlen;	/* return size of protocol address */

	freeaddrinfo(ressave);

	return(listenfd);
}
/* end tcp_listen */

/*
 * We place the wrapper function here, not in wraplib.c, because some
 * XTI programs need to include wraplib.c, and it also defines
 * a Tcp_listen() function.
 */

int
Tcp_listen(const char *host, const char *serv, socklen_t *addrlenp)
{
	return(tcp_listen(host, serv, addrlenp));
}

int
udp_client(const char *host, const char *serv, SA **saptr, socklen_t *lenp)
{
	int				sockfd, n;
	struct addrinfo	hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
		err_quit("udp_client error for %s, %s: %s",
				 host, serv, gai_strerror(n));
	ressave = res;

	do {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd >= 0)
			break;		/* success */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL)	/* errno set from final socket() */
		err_sys("udp_client error for %s, %s", host, serv);

	*saptr = Malloc(res->ai_addrlen);
	memcpy(*saptr, res->ai_addr, res->ai_addrlen);
	*lenp = res->ai_addrlen;

	freeaddrinfo(ressave);

	return(sockfd);
}
/* end udp_client */

int
Udp_client(const char *host, const char *serv, SA **saptr, socklen_t *lenptr)
{
	return(udp_client(host, serv, saptr, lenptr));
}

int
udp_server(const char *host, const char *serv, socklen_t *addrlenp)
{
	int				sockfd, n;
	struct addrinfo	hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
		err_quit("udp_server error for %s, %s: %s",
				 host, serv, gai_strerror(n));
	ressave = res;

	do {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0)
			continue;		/* error - try next one */

		if (bind(sockfd, res->ai_addr, res->ai_addrlen) == 0)
			break;			/* success */

		Close(sockfd);		/* bind error - close and try next one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL)	/* errno from final socket() or bind() */
		err_sys("udp_server error for %s, %s", host, serv);

	if (addrlenp)
		*addrlenp = res->ai_addrlen;	/* return size of protocol address */

	freeaddrinfo(ressave);

	return(sockfd);
}
/* end udp_server */

int
Udp_server(const char *host, const char *serv, socklen_t *addrlenp)
{
	return(udp_server(host, serv, addrlenp));
}

int
sock_cmp_addr(const struct sockaddr *sa1, const struct sockaddr *sa2,
			 socklen_t salen)
{
	if (sa1->sa_family != sa2->sa_family)
		return(-1);

	switch (sa1->sa_family) {
		case AF_INET: {
			return(memcmp( &((struct sockaddr_in *) sa1)->sin_addr,
					   &((struct sockaddr_in *) sa2)->sin_addr,
					   sizeof(struct in_addr)));
		}

		case AF_INET6: {
			return(memcmp( &((struct sockaddr_in6 *) sa1)->sin6_addr,
					   &((struct sockaddr_in6 *) sa2)->sin6_addr,
					   sizeof(struct in6_addr)));
		}
	}

	return (-1);
}

int
sock_cmp_port(const struct sockaddr *sa1, const struct sockaddr *sa2,
			 socklen_t salen)
{
	if (sa1->sa_family != sa2->sa_family)
		return(-1);

	switch (sa1->sa_family) {
	case AF_INET: {
		return( ((struct sockaddr_in *) sa1)->sin_port ==
				((struct sockaddr_in *) sa2)->sin_port);
	}

	case AF_INET6: {
		return( ((struct sockaddr_in6 *) sa1)->sin6_port ==
				((struct sockaddr_in6 *) sa2)->sin6_port);
	}

	}
    return (-1);
}

void *
Malloc(size_t size)
{
	void	*ptr;

	if ( (ptr = malloc(size)) == NULL)
		err_sys("malloc error");
	return(ptr);
}


