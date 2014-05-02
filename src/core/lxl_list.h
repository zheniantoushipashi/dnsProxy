
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_LIST_H_INCLUDE
#define LXL_LIST_H_INCLUDE


#include <lxl_config.h>


typedef struct lxl_list_s lxl_list_t;

struct lxl_list_s {
	lxl_list_t *prev;
	lxl_list_t *next;
};


#define lxl_list_init(h)		(h)->prev = h; (h)->next = h

#define lxl_list_empty(h)  		(h == (h)->next)

#define lxl_list_sentinel(h) 	(h)

#define lxl_list_head(h)		(h)->next

#define lxl_list_last(h)		(h)->prev

#define lxl_list_next(l)		(l)->next

#define lxl_list_prev(l)		(l)->prev


/*#define lxl_list_add_head(h, n)				\
	(n)->next = (h)->next;					\
	(h)->next->prev = n;					\
	(h)->next = n;							\
	(n)->prev = h							*/

/*#define lxl_list_add_tail(h, n)				\
	(n)->prev = (h)->prev;					\
	(h)->prev->next = n;					\
	(h)->prev = n;							\
	(n)->next = h*/
#define lxl_list_add_head(h, n)				\
	(n)->next = (h)->next;					\
	(n)->next->prev = n;					\
	(n)->prev = h;							\
	(h)->next = n			

#define lxl_list_add_tail(h, n)				\
	(n)->prev = (h)->prev;					\
	(n)->prev->next = n;					\
	(n)->next = h;							\
	(h)->prev = n

#define lxl_list_del(n)						\
	(n)->next->prev = (n)->prev;			\
	(n)->prev->next = (n)->next

/* h n is may empty */
#define lxl_list_merge(h, n)				\
	(h)->prev->next = (n)->next;			\
	(n)->next->prev = (h)->prev;			\
	(h)->prev = (n)->prev;					\
	(n)->prev->next = h

#define lxl_list_headmerge(h, n)			\
	(h)->next->prev = (n)->prev;			\
	(n)->prev->next = (h)->next;			\
	(h)->next = (n)->next;					\
	(n)->next->prev = h

#define lxl_list_data(list, type, link)		\
	(type *) ((u_char *)list - offsetof(type, link)) 


#endif	/* LXL_LIST_H_INCLUDE */
