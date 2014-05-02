
/*
 * Coryright (C) xianliang.li
 */


#ifndef LXL_SLIST_H_INCLUDE
#define LXL_SLIST_H_INCLUDE


#include <lxl_config.h>


typedef struct lxl_slist_s lxl_slist_t;

struct lxl_slist_s {
	lxl_slist_t *next;
};


#define lxl_slist_init(h)		(h)->next = NULL

#define lxl_slist_empty(h)		((h)->next == NULL)

#define lxl_slist_sentinel()	NULL

#define lxl_slist_head(h)		(h)->next

#define lxl_slist_next(n)		(n)->next

#define lxl_slist_add(prev, n)						\
	(n)->next = (prev)->next;						\
	(prev)->next = n						

#define lxl_slist_add_head(h, n)					\
	(n)->next = (h)->next;							\
	(h)->next = n					

#define lxl_slist_del(prev, n)						\
	(prev)->next = (n)->next

#define lxl_slist_data(slist, type, link)			\
	(type *) ((u_char *) slist - offsetof(type, link))


#endif	/* LXL_STLIST_H_INCLUDE */
