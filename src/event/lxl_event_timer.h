
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_EVENT_TIMER_H_INCLUDE
#define LXL_EVENT_TIMER_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_log.h>
#include <lxl_times.h>
#include <lxl_rbtree.h>
#include <lxl_event.h>


#define	LXL_TIMER_LAZY_DELAY	300


extern lxl_rbtree_t 		lxl_event_timer_rbtree;
extern lxl_rbtree_node_t 	lxl_event_timer_sentinel;


#define lxl_event_timer_init()			\
	lxl_rbtree_init(&lxl_event_timer_rbtree, &lxl_event_timer_sentinel, lxl_rbtree_insert_value)

lxl_uint_t lxl_event_find_timer(void);
void lxl_event_expire_timers(void);

static inline void 
lxl_event_del_timer(lxl_event_t *ev)
{
	lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "event timer del: %d:%lu", lxl_event_ident(ev->data), ev->timer.key);
	lxl_rbtree_delete(&lxl_event_timer_rbtree, &ev->timer);
	ev->timer_set = 0;
}

static inline void 
lxl_event_add_timer(lxl_event_t *ev, lxl_uint_t timer)
{
	lxl_uint_t key;
	lxl_int_t diff;
	
	key = lxl_current_msec + timer;
	if (ev->timer_set) {
		diff = (lxl_int_t) (key - ev->timer.key);
		if (lxl_abs(diff) < LXL_TIMER_LAZY_DELAY) {
			lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "event timer: %d, old %lu, new %lu", lxl_event_ident(ev->data), ev->timer.key, key);
			return;
		}

		lxl_event_del_timer(ev);
	}

	ev->timer.key = key;
	lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "event timer add: %d: %lu:%lu", lxl_event_ident(ev->data), timer, key);
	lxl_rbtree_insert(&lxl_event_timer_rbtree, &ev->timer);
	ev->timer_set = 1;
}


#endif /* LXL_EVENT_TIMER_H_INCLUDE */
