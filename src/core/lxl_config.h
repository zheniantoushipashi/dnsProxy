
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_CONFIG_H_INCLUDE
#define LXL_CONFIG_H_INCLUDE


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <errno.h>
#include <time.h>
#include <malloc.h>			/* memalign */
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/resource.h>


#ifdef __linux__
#define LXL_HAVE_EPOLL	1
#endif


typedef char				lxl_flag_t;
typedef long				lxl_int_t;
typedef unsigned long		lxl_uint_t;
typedef lxl_uint_t			lxl_msec_t;

typedef lxl_int_t (*lxl_cmp_pt) (void *elt1, void *elt2);


#define LXL_MAX_UINT32_VALUE	(uint32_t) 0xffffffff
#define LXL_MAX_INT32_VALUE		(uint32_t) 0x7fffffff

#ifdef	MAXHOSTNAMELEN
#define LXL_MAXHOSTNAMELEN		MAXHOSTNAMELEN
#else	
#define LXL_MAXHOSTNAMELEN		256
#endif

#define LXL_LISTEN_BACKLOG		511


#endif	/* LXL_CONFIG_H_INCLUDE */
