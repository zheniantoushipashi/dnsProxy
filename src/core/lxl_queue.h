
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_QUEUE_H_INCLUDE
#define LXL_QUEUE_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_alloc.h>
#include <lxl_palloc.h>


typedef struct {
	lxl_uint_t front;
	lxl_uint_t rear;
	lxl_uint_t nalloc;
	size_t	   size;
	void 	   *elts;
	lxl_pool_t *pool;
} lxl_queue_t;


lxl_queue_t * lxl_queue_create(lxl_pool_t *p, lxl_uint_t n, size_t size);
void lxl_queue_destroy(lxl_queue_t *q);

/*#define lxl_queue_free(q)						\
		free((q)->elts);						\
		free(q);								\
		q = NULL*/

static inline lxl_int_t
lxl_queue_init(lxl_queue_t *q, lxl_pool_t *p, lxl_uint_t n, size_t size)
{
	/* one is empty, add */
	if (p) {
		q->elts = lxl_palloc(p, (n + 1) * size);	
	} else {
		q->elts = lxl_alloc((n + 1) * size);
	}
	if (q->elts == NULL) {
		return -1;
	}

	q->front = 0;
	q->rear = 0;
	q->nalloc = n + 1;
	q->size = size;
	q->pool = p;

	return 0;
}

#define lxl_queue_empty(q)	((q)->front == (q)->rear)
#define	lxl_queue_full(q)	((q)->front == ((q)->rear + 1) % (q)->nalloc)

void *lxl_queue_in(lxl_queue_t *q);

int lxl_queue_realloc(lxl_queue_t *q);

/*#define lxl_queue_in(q, elt)					\
	if (lxl_queue_full(q)) {					\
		lxl_queue_realloc(q);					\
	}											\
	q->elts[q->rear] = elt;						\
	++q->rear;									\
	q->rear %= q->nalloc*/

static inline void *	
lxl_queue_out(lxl_queue_t *q)
{
	void *elt;

	if (lxl_queue_empty(q)) {
		return NULL;
	} else {
		elt = (u_char *) q->elts + q->front * q->size;
		++q->front;
		q->front %= q->nalloc;
		return elt;
	}
}


#endif	/* LXL_QUEUE_H_INCLUDE */
