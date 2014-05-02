/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_SOCKET_H_INCLUDE
#define LXL_SOCKET_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>


#define LXL_BLOCKING	0
#define LXL_NONBLOCKING	1
#define LXL_LISTENQ		10240	/* unix domain socket stream you guanxi */


#define lxl_nonblocking_n	"ioctl(FIOBIO)"

lxl_int_t lxl_nonblocking(int fd);
lxl_int_t lxl_blocking(int fd);

/*int lxl_tcp_socket(void);
int lxl_tcp_connect(char *ip, uint16_t port, lxl_int_t nb);
int lxl_tcp_listen(char *ip, uint16_t port, lxl_int_t nb);
int lxl_udp_socket(void);
int lxl_udp_listen(char *ip, uint16_t port, lxl_int_t nb);
int lxl_unix_connect(char *path, int nb);
int lxl_unix_listen(char *path);*/

ssize_t lxl_readn(int fd, char *buf, size_t size);
ssize_t lxl_writen(int fd, char *buf, size_t size);
ssize_t lxl_recv(lxl_connection_t *c, char *buf, size_t size);
ssize_t lxl_send(lxl_connection_t *c, char *buf, size_t size);
ssize_t lxl_recvfrom(lxl_connection_t *c, char *buf, size_t size);
ssize_t lxl_recvfrom_peer(lxl_connection_t *c, char *buf, size_t size);
ssize_t lxl_sendto(lxl_connection_t *c, char *buf, size_t size);


#endif	/* LXL_SOCKET_H_INCLUDE */
