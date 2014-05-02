
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_array.h>


lxl_array_t *
lxl_array_create(lxl_pool_t *p, lxl_uint_t n, size_t size)
{
	lxl_array_t *a = lxl_palloc(p, sizeof(lxl_array_t));
	if (a == NULL) {
		return NULL;
	}

	if (lxl_array_init(a, p, n, size) != 0) {
		return NULL;
	}

	return a;
}
