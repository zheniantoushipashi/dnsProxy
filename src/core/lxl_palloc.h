
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_PALLOC_H_INCLUDE
#define LXL_PALLOC_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>


#define LXL_POOL_ALIGNMENT		16
#define LXL_MAX_ALLOC_FROM_POOL 4095 	/* pagesize - 1 */
#define LXL_DEFAULT_POOL_SIZE	16384
#define LXL_MIN_POLL_SIZE		lxl_align((sizeof(lxl_pool_t) + 2 * sizeof(lxl_pool_large_t)), LXL_POOL_ALIGNMENT)

#define lxl_align(d, a)						\
	(((d) + (a - 1)) & ~(a -1))

#define lxl_align_ptr(p, a)					\
	(u_char *) (((unsigned long) (p) + ((unsigned long) a - 1)) & ~((unsigned long) a - 1)) 
	//(u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))



typedef struct {
	unsigned failed;
	u_char *last;
	u_char *end;
	lxl_pool_t *next;
} lxl_pool_data_t;

typedef struct lxl_pool_large_s lxl_pool_large_t;

struct lxl_pool_large_s {
	lxl_pool_large_t *next;
	void *alloc;
};

typedef void (*lxl_pool_cleanup_pt) (void *data);

typedef struct lxl_pool_cleanup_s lxl_pool_cleanup_t;

struct lxl_pool_cleanup_s {
	lxl_pool_cleanup_pt handler;
	void *data;
	lxl_pool_cleanup_t *next;
};

struct lxl_pool_s {
	lxl_pool_data_t d;
	unsigned max;
	lxl_pool_t *current;
	lxl_pool_large_t *large;
	lxl_pool_cleanup_t *cleanup;
};


lxl_pool_t * lxl_create_pool(size_t size);
void lxl_destroy_pool(lxl_pool_t *pool);
void lxl_reset_pool(lxl_pool_t *pool);

void *lxl_palloc(lxl_pool_t *pool, size_t size);
void *lxl_pnalloc(lxl_pool_t *pool, size_t size);
void *lxl_pcalloc(lxl_pool_t *pool, size_t size);
void *lxl_pmemalign(lxl_pool_t *pool, size_t size, size_t alignment);
int lxl_pfree(lxl_pool_t *pool, void *p);
lxl_pool_cleanup_t * lxl_pool_cleanup_add(lxl_pool_t *pool, size_t size);


#endif	/* LXL_PALLOC_H_INCLUDE */
