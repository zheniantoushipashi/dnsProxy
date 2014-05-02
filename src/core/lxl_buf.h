
/*
 * Copyright (c) xianliang.li
 */


#ifndef LXL_BUF_H_INCLUDE
#define LXL_BUF_H_INCLUDE


#include <lxl_config.h>
#include <lxl_palloc.h>


#define lxl_alloc_buf(p)	lxl_palloc(p, sizeof(lxl_buf_t))
#define lxl_calloc_buf(p)	lxl_pcalloc(p, sizeof(lxl_buf_t))


typedef struct {
	size_t len;
	size_t nalloc;
	char *data;

	char *pos;
	char *last;
	
	char *start;
	char *end;
} lxl_buf_t;

lxl_buf_t *	lxl_create_temp_buf(lxl_pool_t *pool, size_t size);

static inline lxl_int_t
lxl_buf_palloc(lxl_buf_t *buf, lxl_pool_t *p, size_t size)
{
	buf->data = lxl_palloc(p, size);
	if (buf->data == NULL) {
		return -1;
	}
		
	buf->len = 0;
	buf->nalloc = size;

	return 0;
}


#endif	/* LXL_BUF_H_INCLUDE */
