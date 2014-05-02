
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_buf.h>


lxl_buf_t *
lxl_create_temp_buf(lxl_pool_t *pool, size_t size)
{
	lxl_buf_t *b;

	b = lxl_alloc_buf(pool);
	if (b == NULL) {
		return NULL;
	}

	b->data = lxl_palloc(pool, size);
	if (b->data == NULL) {
		return NULL;
	}

	b->len = 0;
	b->nalloc = size;

	return b;
}
