
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_CONNECTION_H_INCLUDE
#define LXL_CONNECTION_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_list.h>
#include <lxl_buf.h>
#include <lxl_cycle.h>


#define	LXL_CONNECTION_NOT_CLOSE_SOCKET	0
#define LXL_CONNECTION_CLOSE_SOCKET		1


typedef void (*lxl_connection_handler_pt) (lxl_connection_t *c);
typedef ssize_t (*lxl_recv_pt)	(lxl_connection_t *c, char *buf, size_t len);
typedef ssize_t (*lxl_send_pt)	(lxl_connection_t *c, char *buf, size_t len);


typedef struct lxl_listening_s {
	struct sockaddr    *sockaddr;
	socklen_t			socklen;
	//lxl_str_t			addr_text;
	char			   *addr_text;

	int 				backlog;
	int 				fd;
	int 				type;
	//int					proto;

	int 				rcvbuf;
	int 				sndbuf;
	size_t 				pool_size;	/* 1024 dns */
	/*char 			   *client_buffer;	for udp
	size_t				client_buffer_size; */

	lxl_connection_handler_pt handler;
	void 			   *servers;

	lxl_connection_t   *connection;

	unsigned 			bound:1;
	unsigned 			listen:1;
	unsigned 			nonblocking:1;
} lxl_listening_t;

struct lxl_connection_s {
	void 			   *data;						/* next to c */	
	lxl_event_t 	   *read;
    lxl_event_t 	   *write;
	lxl_recv_pt 		recv;
	lxl_send_pt 		send;

	lxl_listening_t    *listening;
	socklen_t 			socklen;
	struct sockaddr 	sockaddr;
	// lxl_str_t   addr_text ip string
	lxl_pool_t 		   *pool;
	lxl_buf_t 		   *buffer;		/* replace lxl_buf_t buffer */
	lxl_list_t 			list;
//	lxl_uint_t 			request;

    int 				fd;
	unsigned			udp:1;
	unsigned 			closefd:1;
	unsigned 			timedout:1;
};


lxl_listening_t *lxl_create_listening(lxl_conf_t *cf, void *sockaddr, socklen_t socklen, int type);
int 			 lxl_open_listening_sockets(lxl_cycle_t *cycle);
void			 lxl_configure_listening_sockets(lxl_cycle_t *cycle);
lxl_connection_t *lxl_get_connection(int fd);

#define lxl_free_connection(c)						\
	(c)->data = lxl_cycle->free_connections;			\
	lxl_cycle->free_connections = c;					\
	++lxl_cycle->free_connection_n

void lxl_close_connection(lxl_connection_t *c);


#endif	/* LXL_CONNECTION_H_INCLUDE */
