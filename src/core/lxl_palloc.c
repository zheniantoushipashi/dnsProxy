
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_alloc.h>
#include <lxl_palloc.h>


static void *lxl_palloc_block(lxl_pool_t *pool, size_t size);
static void *lxl_palloc_large(lxl_pool_t *pool, size_t size);


lxl_pool_t *
lxl_create_pool(size_t size)
{
	lxl_pool_t *p;

	p = lxl_memalign(LXL_POOL_ALIGNMENT, size);
	if (p == NULL) {
		return NULL;
	}

	p->d.last = (u_char *) p + sizeof(lxl_pool_t);
	p->d.end = (u_char *) p + size;
	p->d.next = NULL;
	p->d.failed = 0;
	size = size - sizeof(lxl_pool_t);
	p->max = (size < LXL_MAX_ALLOC_FROM_POOL) ? size : LXL_MAX_ALLOC_FROM_POOL;
	p->current = p;
	p->large = NULL;
	p->cleanup = NULL;

	return p;
}

void 
lxl_destroy_pool(lxl_pool_t *pool)
{
	lxl_pool_t *p, *n;
	lxl_pool_large_t *l;
	lxl_pool_cleanup_t *c;

	for (c = pool->cleanup; c; c = c->next) {
		lxl_log_debug(LXL_LOG_DEBUG_ALLOC, 0, "cleanup %p", c);
		c->handler(c->data);
	}

	for (l = pool->large; l; l = l->next) {
		lxl_log_debug(LXL_LOG_DEBUG_ALLOC, 0, "free: %p", l);
		if (l->alloc) {
			lxl_free(l->alloc);
		}
	}

	for (p = pool, n = pool->d.next; ; p = n, n = n->d.next) {
		lxl_log_debug(LXL_LOG_DEBUG_ALLOC, 0, "free: %p, unused: %lu", p, p->d.end - p->d.last);
		if (n == NULL) {
			break;
		}
	}

	for (p = pool, n = pool->d.next; ; p = n, n = n->d.next) {
		lxl_free(p);
		if (n == NULL) {
			break;
		}
	}
}

void 
lxl_reset_pool(lxl_pool_t *pool)
{
	lxl_pool_t *p;
	lxl_pool_large_t *l;

	for (l = pool->large; l; l = l->next) {
		if (l->alloc) {
			lxl_free(l->alloc);
		}
	}
	pool->large = NULL;

	for (p = pool; p; p = p->d.next) {
		p->d.last = (u_char *) p + sizeof(lxl_pool_t);
	}
}

void *
lxl_palloc(lxl_pool_t *pool, size_t size)
{
	u_char *m;
	lxl_pool_t *p;

	if (size <= pool->max) {
		p = pool->current;
		do {
			m = lxl_align_ptr(p->d.last, LXL_POOL_ALIGNMENT);
			if (size <= (size_t) (p->d.end - m)) {
				p->d.last = m + size;
				return m;
			}

			p = p->d.next;
		} while (p);

		return lxl_palloc_block(pool, size);
	}

	return lxl_palloc_large(pool, size);
}

void *
lxl_pnalloc(lxl_pool_t *pool, size_t size)
{
	u_char *m;
	lxl_pool_t *p;

	if (size <= pool->max) {
		p = pool->current;
		do {
			m = p->d.last;
			if (size <= (size_t) (p->d.end - m)) {
				p->d.last = m + size;

				return m;
			}

			p = p->d.next;
		} while (p);

		return lxl_palloc_block(pool, size);
	}

	return lxl_palloc_large(pool, size);
}

static void *
lxl_palloc_block(lxl_pool_t *pool, size_t size)
{
	u_char *m;
	size_t psize;
	lxl_pool_t *p, *new, *current;

	psize = (size_t) (pool->d.end - (u_char *) pool);
	m = lxl_memalign(LXL_POOL_ALIGNMENT, psize);
	if (m == NULL) {
		return NULL;
	}

	new = (lxl_pool_t *) m;
	new->d.end = m + psize;
	new->d.next = NULL;
	new->d.failed = 0;
	m += sizeof(lxl_pool_data_t);
	m = lxl_align_ptr(m, LXL_POOL_ALIGNMENT);
	new->d.last = m + size;
	
	current = pool->current;
	for (p = current; p->d.next; p = p->d.next) {
		if (++p->d.failed > 5) {
			current = p->d.next;
		}
	}

	p->d.next = new;
	pool->current = current ? current : new;
	
	return m;
}

static void *
lxl_palloc_large(lxl_pool_t *pool, size_t size)
{
	void *p;
	unsigned n;
	lxl_pool_large_t *large;

	p = lxl_alloc(size);
	if (p == NULL) {
		return NULL;
	}

	n = 0;
	for (large = pool->large; large; large = large->next) {
		if (large->alloc == NULL) {
			large->alloc = p;
			return p;
		}

		if (++n > 4) {
			break;
		}
	}

	large = lxl_palloc(pool, sizeof(lxl_pool_large_t));
	if (large == NULL) {
		lxl_free(p);
		return NULL;
	}

	large->alloc = p;
	large->next = pool->large;
	pool->large = large;

	return p;
}

void *
lxl_pmemalign(lxl_pool_t *pool, size_t size, size_t alignment)
{
	void *p;
	lxl_pool_large_t *large;

	p = lxl_memalign(alignment, size);
	if (p == NULL) {
		return NULL;
	}

	large = lxl_palloc(pool, sizeof(lxl_pool_large_t));
	if (large == NULL) {
		lxl_free(p);
		return NULL;
	}

	large->alloc = p;
	large->next = NULL;
	pool->large = large;

	return p;
}

int
lxl_pfree(lxl_pool_t *pool, void *p)
{
	lxl_pool_large_t *l;

	for (l = pool->large; l; l = l->next) {
		if (p == l->alloc) {
			lxl_log_debug(LXL_LOG_DEBUG_ALLOC, 0, "free: ", l->alloc);
			lxl_free(l->alloc);
			l->alloc = NULL;

			return 0;
		}
	}

	return -1;
}

void *
lxl_pcalloc(lxl_pool_t *pool, size_t size)
{
	void *p;

	p = lxl_palloc(pool, size);
	if (p) {
		memset(p, 0x00, size);
	}

	return p;
}

lxl_pool_cleanup_t *
lxl_pool_cleanup_add(lxl_pool_t *p, size_t size)
{
	lxl_pool_cleanup_t *c;

	c = lxl_palloc(p, sizeof(lxl_pool_cleanup_t));
	if (c == NULL) {
		return NULL;
	}	

	if (size) {
		c->data = lxl_palloc(p, size);
		if (c->data == NULL) {
			return NULL;
		} 
	} else {
		c->data = NULL;
	}

	c->handler = NULL;
	c->next = p->cleanup;
	p->cleanup = c;

	lxl_log_debug(LXL_LOG_DEBUG_ALLOC, 0, "add cleanup: %p", c);
	
	return c;
}
