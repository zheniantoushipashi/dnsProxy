
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_queue.h>


lxl_queue_t *
lxl_queue_create(lxl_pool_t *p, lxl_uint_t n, size_t size)
{
	lxl_queue_t *q;
	
	q = lxl_palloc(p, sizeof(lxl_queue_t));
	if (q == NULL) {
		return NULL;
	}

	if (lxl_queue_init(q, p, n, size) != 0) {
		return NULL;
	}

	return q;
}

void *
lxl_queue_in(lxl_queue_t *q)
{
	lxl_uint_t i;
	u_char *ptr;
	void *elt, *new;
	size_t size;
	lxl_pool_t *p;

	if (lxl_queue_full(q)) {
		size  = q->nalloc * q->size;
		p = q->pool;
		if (p) {
			if ((u_char *) q->elts + size == p->d.last && p->d.last + q->size <= p->d.end) {
				if (q->front > q->rear) {
					i = (q->nalloc - q->front) * q->size;
					ptr = (u_char *) q->elts + q->front * q->size;
					do {
						--i;
						*(ptr + i + q->size) = *(ptr + i);
					} while (i);
				}
				p->d.last += q->size;
				++q->nalloc;
			} else {
				new = lxl_palloc(p, 2 * size);
				if (new == NULL) {
					return NULL;
				}

				if (q->front > q->rear) {
					i = (q->nalloc - q->front) * q->size;
					memcpy(new, q->elts + q->front, i);
					memcpy((u_char *) new + i, q->elts, q->rear * q->size);
					q->front = 0;
					q->rear = q->nalloc - 1;
				} else {
					memcpy(new, q->elts, q->rear * q->size);
				}
				q->nalloc *= 2;
				q->elts = new;
			}
		} else {
			new = lxl_alloc(2 * size);
			if (new == NULL) {
				return NULL;
			}

			if (q->front > q->rear) {
				i = (q->nalloc - q->front) * q->size;
				memcpy(new, q->elts + q->front, i);
				memcpy((u_char *) new + i, q->elts, q->rear * q->size);
				q->front = 0;
				q->rear = q->nalloc - 1;
			} else {
				memcpy(new, q->elts, q->rear * q->size);
			}
			q->nalloc *= 2;
			q->elts = new;
		}
	}

	elt = (u_char *) q->elts + q->rear * q->size;
	++q->rear;
	/* q->rear %= q->nalloc; */

	return elt;
}
