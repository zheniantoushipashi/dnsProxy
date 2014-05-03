
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_INET_INCLUDE
#define LXL_INET_INCLUDE


#include <lxl_core.h>
#include <lxl_config.h>
#include <lxl_string.h>


/*	255.255.255.255 15+1 */
#define LXL_INET_ADDRSTRLEN		16
/* 	ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255	*/
#define LXL_INET6_ADDRSTRLEN	46
/* LXL_INET6_ADDRSTRLEN + sizeof("[]:65535")(9) */
#define LXL_SOCKADDR_STRLEN		54		
#define LXL_SOCKADDRLEN			sizeof(struct sockaddr_in6)


typedef struct {
	lxl_str_t 	url;
	lxl_str_t 	host;
	
	in_port_t	port;
	in_port_t	default_port;	
	int 		family;
	
	unsigned 	listen:1;
	unsigned 	no_resolve:1;
	unsigned 	no_port:1;
	unsigned 	wildcard:1;
	
	socklen_t	socklen;
	char		sockaddr[LXL_SOCKADDRLEN];
	char 		*err;
} lxl_url_t;


int	lxl_parse_url(lxl_pool_t *pool, lxl_url_t *u);
int lxl_inet_resolve_host(lxl_pool_t *pool, lxl_url_t *u);


#endif	/* LXL_INET_INCLUDE	*/
