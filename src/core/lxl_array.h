
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_ARRAY_H_INCLUDE
#define LXL_ARRAY_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
//#include <lxl_alloc.h>
#include <lxl_palloc.h>


typedef struct {
	void 		*elts;
	lxl_uint_t 	nelts;
	lxl_uint_t 	nalloc;
	size_t 		size;
	lxl_pool_t 	*pool;
} lxl_array_t;


lxl_array_t *lxl_array_create(lxl_pool_t *p, lxl_uint_t n, size_t size);

static inline lxl_int_t 
lxl_array_init(lxl_array_t *a, lxl_pool_t *p, lxl_uint_t n, size_t size)
{
	a->elts = lxl_palloc(p, size * n);
	if (a->elts == NULL) {
		return -1;
	}

	a->nelts = 0;
	a->nalloc = n;
	a->size = size;
	a->pool = p;

	return 0;
}

#define lxl_array_empty(a)								\
	((a)->nelts == 0)

#define lxl_array_full(a)								\
	((a)->nelts == (a)->nalloc)

#define lxl_array_nelts(a)								\
	(a)->nelts

#define lxl_array_clear(a)								\
	(a)->nelts = 0

#define lxl_array_elts(a)								\
	(a)->elts

#define lxl_array_data(a, type, index)					\
	(type *) ((u_char *) (a)->elts + (index) * (a)->size) 

static inline void *
lxl_array_push(lxl_array_t *a)
{
	void *elt, *new;
	size_t size;
	lxl_pool_t *p;

	if (lxl_array_full(a)) {
		size = a->nalloc * a->size;
		p = a->pool;
		if ((u_char *) a->elts + size == p->d.last && p->d.last + a->size <= p->d.end) {
			p->d.last += a->size;
			++a->nalloc;
		} else {
			new = lxl_palloc(p, 2 * size);
			if (new == NULL) {
				return NULL;
			}

			memcpy(new, a->elts, size);
			a->elts = new;
			a->nalloc *= 2;
		}
	}

	elt = (u_char *) a->elts + a->nelts * a->size;
	++a->nelts;

	return elt;
}

static inline lxl_int_t
lxl_array_del(lxl_array_t *a, void *elt, lxl_cmp_pt cmp)
{
	void *e;
	lxl_uint_t i;
		
	for (i = 0; i < a->nelts; ++i) {
		e = (u_char *) a->elts + i * a->size;
		if (cmp(elt, e) == 0) {
			memcpy(e, e + a->size, (a->nelts - i - 1) * a->size);
			--a->nelts;
			
			return 0;
		}
	}

	return -1;
}

static inline void  *
lxl_array_find(lxl_array_t *a, void *elt, lxl_cmp_pt cmp)
{
	void *e;
	lxl_uint_t i;

	for(i = 0; i < a->nelts; ++i) {
		e = (u_char *) a->elts + i * a->size;
		if (cmp(elt, e) == 0) {
			return e; 
		}
	}

	return NULL;
}
/*
static void 
lxl_array_destroy(lxl_array_t *a)
{
	a->nelts = 0;
	a->nalloc = 0;
	lxl_free(a->elts);
}
*/
#if 0
static inline lxl_int_t 
lxl_array_find_index(lxl_array_t *a, void *elt, lxl_cmp_pt cmp)
{
	lxl_uint_t i;

	for(i = 0; i < a->nelts; ++i) {
		if (cmp(elt, a->elts[i]) == 0) {
			return (lxl_int_t) i;   /* ignore */
		}
	}

	return -1;
}
#endif


#endif	/* LXL_ARRAY_H_INCLUDE */
