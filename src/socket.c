#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h> // inet_pton, inet_ntop
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "socket.h"
#include "log.h"
#include "utils.h"

int
get_ip_address_info(struct tracker_prot *tp, int *ip, uint16 *port)
{
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if(tp->prot_type == TRACKER_PROT_UDP) {
        hints.ai_socktype = SOCK_DGRAM;
    }

    LOG_INFO("looking up %s:%s ...\n", tp->host, tp->port);

	struct addrinfo *res = NULL;
    int ret = getaddrinfo(tp->host, tp->port, &hints, &res);
    if(ret) {
        LOG_INFO("getaddrinfo (%s:%s):%s\n", tp->host, tp->port, gai_strerror(ret));
        *ip = 0;
        *port = 0;
        return -1;
    }
	
	memcpy(port, res->ai_addr->sa_data, 2);
	memcpy(ip, res->ai_addr->sa_data+2, 4);

	freeaddrinfo(res);
    return 0; 
}

int
set_socket_unblock(int sfd)
{    
    int flags;

    if((flags = fcntl(sfd, F_GETFL, 0)) < 0) {
        LOG_ERROR("fcntl %d get failed!\n", sfd);
        return -1;
    }

    flags |= O_NONBLOCK;

    if(fcntl(sfd, F_SETFL, flags) < 0) {
        LOG_ERROR("fcntl %d set failed!\n", sfd);
        return -1;
    }

    return 0;
}

int
set_socket_opt(int sfd)
{
    int flags = 1;
    return setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags));
}

int
get_socket_opt(int sfd, int opname, int *flags)
{
    socklen_t slen = sizeof(int);
    return getsockopt(sfd, SOL_SOCKET, opname, flags, &slen);
}

int
get_socket_error(int sfd, int *error)
{
    socklen_t slen = sizeof(int);
    return getsockopt(sfd, SOL_SOCKET, SO_ERROR, error, &slen);
}

int
socket_tcp_send(int sfd, char *buf, int buflen, int flags)
{
    return send(sfd, buf, buflen, flags);
}

int
socket_tcp_recv(int sfd, char *buf, int buflen, int flags)
{
    return recv(sfd, buf, buflen, flags);
}

int
socket_udp_send(int sfd, char *buf, int buflen, int flags)
{
    return send(sfd, buf, buflen, flags);
}

int
socket_udp_recv(int sfd, char *buf, int buflen, int flags)
{
    return recv(sfd, buf, buflen, flags);
}

int
socket_udp_recvfrom(int sfd, char *buf, int buflen, int flags, struct sockaddr *sa, socklen_t  *sl)
{
    return recvfrom(sfd, buf, buflen ,flags, sa, sl);
}

int
socket_tcp_create(void)
{
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd < 0) {
        LOG_ERROR("socket:%s\n", strerror(errno));
        return -1;
    }

    return sfd;
}

int
socket_udp_create(void)
{
    int sfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sfd < 0) {
        LOG_ERROR("socket:%s\n", strerror(errno));
        return -1;
    }

    return sfd;
}

int
socket_tcp_connect(int sock, int ip, uint16 port)
{
    struct sockaddr_in sa;

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = port;
    sa.sin_addr.s_addr = ip;

    return connect(sock, (struct sockaddr *)&sa, sizeof(sa));
}

int
socket_udp_connect(int sock, int ip, uint16 port)
{
    return socket_tcp_connect(sock, ip, port);
}

int
socket_udp_bind(int sock, int ip, uint16 port)
{
    return socket_tcp_bind(sock, ip, port);
}

int
socket_tcp_bind(int sock, int ip, uint16 port)
{
    struct sockaddr_in sa;

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = !ip ? socket_htonl(INADDR_ANY) : ip;
    sa.sin_port = port;

    return bind(sock, (struct sockaddr *)&sa, sizeof(sa));
}

int
socket_tcp_listen(int sock, int backlog)
{
    return listen(sock, backlog);
}

int
socket_tcp_accept(int sock, int *ip, uint16 *port)
{
    struct sockaddr_in sa;
    socklen_t slen = sizeof(sa);

    memset(&sa, 0, sizeof(sa));

    int clisock = accept(sock, (struct sockaddr *)&sa, &slen);
    if(clisock < 0) {
        return -1;
    }

    if(ip) {
        *ip = sa.sin_addr.s_addr;
    }

    if(port) {
        *port = sa.sin_port;
    }

    return clisock;
}

uint64
socket_hton64(uint64 host)
{
    uint64 net;
    
    char *h = (char *)&host;
    char *n = (char *)&net;

    n[7] = h[0];
    n[6] = h[1];
    n[5] = h[2];
    n[4] = h[3];
    n[3] = h[4];
    n[2] = h[5];
    n[1] = h[6];
    n[0] = h[7];

    return net;
}

uint64
socket_ntoh64(uint64 net)
{
    uint64 host;
    
    char *n = (char *)&net;
    char *h = (char *)&host;

    h[7] = n[0];
    h[6] = n[1];
    h[5] = n[2];
    h[4] = n[3];
    h[3] = n[4];
    h[2] = n[5];
    h[1] = n[6];
    h[0] = n[7];

    return host;
}

unsigned int
socket_ntohl(unsigned int net)
{
    return ntohl(net);
}

unsigned int
socket_htonl(unsigned int host)
{
    return htonl(host);
}

unsigned short 
socket_ntohs(unsigned short net)
{
    return ntohs(net);
}

unsigned short 
socket_htons(unsigned short host)
{
    return htons(host);
}

#define ENLARGE_STEP (1024)

int
socket_tcp_recv_until_block(int fd, char **dst, int *dstlen)
{
    if(fd < 0 || !dst || !dstlen) {
        LOG_ERROR("invalid param!\n");
        return -1;
    }

    char *rspbuf = NULL;
    int mallocsz = 0, rcvtotalsz = 0;

    while(1) {

        if(mallocsz <= rcvtotalsz && utils_enlarge_space(&rspbuf, &mallocsz, ENLARGE_STEP)) {
            break;
        }

        int rcvlen = socket_tcp_recv(fd, rspbuf+rcvtotalsz, mallocsz - rcvtotalsz, 0);

        if(rcvlen > 0) {
            rcvtotalsz += rcvlen;
        } else if(!rcvlen) {
            /* peer close */
            break;
        } else if(rcvlen < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        } else {
            LOG_ERROR("socket recv failed:%s\n", strerror(errno));
            break;
        }
    }

    if(rcvtotalsz) {
        *dst = rspbuf;
        *dstlen = rcvtotalsz;
    } else {
        free(rspbuf);
    }

#if 0
    LOG_DEBUG("socket recv totalsz = %d\n", rcvtotalsz);
#endif

    return rcvtotalsz;
}

int
socket_tcp_send_until_block(int fd, char *sndbuf, int buflen)
{
    if(fd < 0 || !sndbuf || buflen <= 0) {
        return -1;
    }

    int totalsnd = 0;
    char *s = sndbuf;
    while(buflen > 0) {
        int wlen = socket_tcp_send(fd, s, buflen, 0);
        if(wlen > 0) {
            buflen -= wlen;
            s += wlen;
            totalsnd += wlen;
        } else if(wlen < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        } else {
            LOG_ERROR("socket send:%s\n", strerror(errno));
            return -1;
        }
    }

    return totalsnd;
}

int
socket_tcp_send_all(int fd, char *sndbuf, int buflen)
{
    if(fd < 0 || !sndbuf || buflen <= 0) {
        return -1;
    }

    char *s = sndbuf;
    while(buflen > 0) {
        int wlen = socket_tcp_send(fd, s, buflen, 0);
        if(wlen > 0) {
            buflen -= wlen;
            s += wlen;
        } else if(wlen < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            continue;
        } else {
            LOG_ERROR("socket send:%s\n", strerror(errno));
            return -1;
        }
    }

    return 0;
}

int
socket_tcp_send_iovs(int fd, const struct iovec *iov, int iovcnt)
{
    if(fd < 0 || !iov || iovcnt <= 0) {
        return -1;
    }
    return writev(fd, iov, iovcnt);
}

