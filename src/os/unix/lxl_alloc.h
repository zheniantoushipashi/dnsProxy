
/*
 * Copyright (C) xianliang.li
 */



#ifndef LXL_ALLOC_H_INCLUDE
#define LXL_ALLOC_H_INCLUDE


#include <lxl_config.h>
#include <lxl_log.h>


static inline void *
lxl_alloc(size_t size) 
{
	void *p;
	
	p = malloc(size);
	if (p == NULL) {
		lxl_log_error(LXL_LOG_EMERG, errno, "malloc(%lu) failed", size);
		p = NULL;
	}

	lxl_log_debug(LXL_LOG_DEBUG_ALLOC, 0, "malloc(%lu) %p", size, p); 

	return p;
}

static inline void *
lxl_calloc(size_t n, size_t size) 
{
	void *p;

	p = calloc(n, size);
	if (p == NULL) {
		lxl_log_error(LXL_LOG_EMERG, errno, "calloc(%lu, %lu) failed", n, size);
		return NULL;
	}

	lxl_log_debug(LXL_LOG_DEBUG_ALLOC, 0, "calloc(%lu, %lu) %p", n, size, p); 

	return p;
}

static inline void *
lxl_memalign(size_t alignment, size_t size)
{
	void *p;

	p = memalign(alignment, size);
	if (p == NULL) {
		lxl_log_error(LXL_LOG_EMERG, errno, "memalign(%lu, %lu) failed", alignment, size);
	}

	lxl_log_debug(LXL_LOG_DEBUG_ALLOC, 0, "memalign(%lu, %lu) %p", alignment, size, p); 

	return p;
}

/*static inline void *
lxl_memalign(size_t alignment, size_t size)
{
	void *p;
	int err;

	err = posix_memalign(&p, alignment, size);
	if (err) {
		lxl_log_error(LXL_LOG_EMERG, errno, "posix_memalign(%lu, %lu) failed", alignment, size);
		p = NULL;
	}

	lxl_log_debug(LXL_LOG_DEBUG_ALLOC, 0, "posix_memalign(%lu, %lu) %p", alignment, size, p); 

	return p;
}*/

#define lxl_free		free


#endif	/* LXL_ALLOC_H_INCLUDE */
