
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_palloc.h>
#include <lxl_files.h>
#include <lxl_file.h>


int 
lxl_get_full_name(lxl_pool_t *pool, lxl_str_t *prefix, lxl_str_t *name)
{
	size_t len;
	char *p;

	if (lxl_file_separator(name->data[0])) {
		return 0;
	}
	
	len = prefix->len + name->len;
	p = lxl_pnalloc(pool, len + 1);
	if (p == NULL) {
		return -1;
	}

	memcpy(p, prefix->data, prefix->len);
	memcpy(p + prefix->len, name->data, name->len);
	p[len] = '\0';
	name->len = len;
	name->data = p;

	return 0;
}
