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

    return inet_ntoa (addr.sin_addr);
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

    ptr = buf;
    nleft = n;
    while (nleft > 0) {
        ret = send (fd, ptr, nleft, 0);
        if (ret <= 0) {
            if (ret < 0 && errno == EWOULDBLOCK) {
                break;
            } else if (ret < 0 && errno == EINTR) {
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
