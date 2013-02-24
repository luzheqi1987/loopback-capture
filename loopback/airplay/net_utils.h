#ifndef _NET_UTILS_H
#define _NET_UTILS_H

#include <string.h>
#include <windows.h>
#include <errno.h>

#include <sys/types.h>
#include <fcntl.h>

char *get_local_addr(int fd);
int set_sock_nonblock(int sockfd);
int tcp_open(void); 
int tcp_connect(int sock_fd, const char *host, unsigned int port);
int tcp_write(int fd, const char *buf, int n);
int tcp_read(int fd, char *buf, int n);

#endif /* _NET_UTILS_H */
