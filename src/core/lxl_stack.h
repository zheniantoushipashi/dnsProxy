
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_STACK_H_INCLUDE
#define LXL_STACK_H_INCLUDE


#include <lxl_config.h>


typedef struct lxl_stack_s lxl_stack_t;

struct lxl_stack_s {
	lxl_stack_t *next;
};


#define lxl_stack_init(h)		(h)->next = NULL

#define lxl_stack_empty(h)		((h)->next == NULL)

#define lxl_stack_push(h, n)						\
	(n)->next = (h)->next;							\
	(h)->next = n

#define lxl_stack_data(stack, type, link)			\
	(type *) ((u_char *) stack - offsetof(type, link))


static inline lxl_stack_t *
lxl_stack_pop(lxl_stack_t *h)
{
	lxl_stack_t *n;

	if (lxl_stack_empty(h)) {
		return NULL;
	}

	n = h->next;
	h->next = n->next;

	return n;
}

static inline lxl_stack_t *
lxl_stack_top(lxl_stack_t *h)
{
	if (lxl_stack_empty(h)) {
			return NULL;
	}

	return h->next;
}


#endif	/* LXL_STACK_H_INCLUDE */
