#include "net_utils.h"
#include <io.h>
#include <Windows.h>
#include <winsock.h>
#include <fcntl.h>
#include "glib.h"
typedef int socklen_t;

char *
get_local_addr (int fd)
{
    struct sockaddr_in addr;
    int ret;
    socklen_t len = sizeof (addr);

    ret = getsockname (fd, (struct sockaddr *) &addr, &len);

    //return inet_ntoa (addr.sin_addr);
    return "172.16.172.102";
}

int
set_sock_nonblock (int sockfd)
{
    u_long mode = 1;
    ioctlsocket(sockfd, FIONBIO, &mode);
    return 0;
}

int
tcp_open ()
{
    int sock_fd;
    sock_fd = socket (AF_INET, SOCK_STREAM, 0);
    return sock_fd;
}

int
tcp_connect (int sock_fd, const char *host, unsigned int port)
{
    struct sockaddr_in addr;
    struct hostent *h;
    int ret;

    h = gethostbyname (host);
    if (h) {
        addr.sin_family = h->h_addrtype;
        memcpy ((char *) &addr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    }else{
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr (host);
        if (addr.sin_addr.s_addr == 0xFFFFFFFF) {
            return -1;
        }
    }
    addr.sin_port = htons (port);
    ret = connect (sock_fd, (struct sockaddr *)&addr, sizeof (struct sockaddr));
    if (ret == -1) {
        int err = WSAGetLastError();
        if (err == WSAEINPROGRESS || err == WSAEWOULDBLOCK) {
            return 0;
        }
    }
    return ret;
}

int
tcp_write (int fd, const char *buf, int n)
{
    size_t nleft;
    ssize_t nwritten = 0;
    const char *ptr;
    int ret;
    int err;

    ptr = buf;
    nleft = n;
    while (nleft > 0) {
        ret = send (fd, ptr, nleft, 0);
        if (ret <= 0) {
            err = WSAGetLastError();
            if (ret < 0 && err == EWOULDBLOCK) {
                break;
            } else if (ret < 0 && err == EINTR) {
                continue;
            } else {
                return -1;
            }
        }
        nwritten += ret;
        nleft -= ret;
        ptr   += ret;
    }
    return nwritten;
}

int tcp_listener(int fd, int port) {
    struct sockaddr_in addr;
    bool val = TRUE;
    int vallen = sizeof(val);
    int ret;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&val, 1);
    if (ret < 0) {
        printf("setsockopt failed: %d\n", WSAGetLastError());
        return ret;
    }
    ret = bind(fd, (struct sockaddr *)&addr, sizeof (struct sockaddr));
    if (ret < 0) {
        printf("bind failed: %d\n", WSAGetLastError());
        return ret;
    }
    ret = listen(fd, 10);
    if (ret < 0) {
        printf("listen failed: %d\n", WSAGetLastError());
        return ret;
    }
    return 0;
}

int
udp_open ()
{
    int sock_fd;
    sock_fd = socket (AF_INET, SOCK_DGRAM, 0);
    return sock_fd;
}

int
udp_write (int fd, const char *buf, int n, const char *host, unsigned int port)
{
    size_t nleft;
    ssize_t nwritten = 0;
    const char *ptr;
    int ret;
    int err;

    struct sockaddr_in addr;
    struct hostent *h;

    h = gethostbyname (host);
    if (h) {
        addr.sin_family = h->h_addrtype;
        memcpy ((char *) &addr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    }else{
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr (host);
        if (addr.sin_addr.s_addr == 0xFFFFFFFF) {
            return -1;
        }
    }
    addr.sin_port = htons (port);

    ptr = buf;
    nleft = n;
    while (nleft > 0) {
        ret = sendto (fd, ptr, nleft, 0, (struct sockaddr *)&addr, sizeof (struct sockaddr));
        if (ret <= 0) {
            err = WSAGetLastError();
            if (ret < 0 && err == EWOULDBLOCK) {
                break;
            } else if (ret < 0 && err == EINTR) {
                continue;
            } else {
                return -1;
            }
        }
        nwritten += ret;
        nleft -= ret;
        ptr   += ret;
    }
    return nwritten;
}
