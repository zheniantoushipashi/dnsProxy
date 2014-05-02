
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_EVENT_H_INCLUDE
#define LXL_EVENT_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_log.h>
#include <lxl_string.h>
#include <lxl_rbtree.h>
#include <lxl_connection.h>


#define LXL_USE_LEVEL_EVENT		0x00000001
#define LXL_USE_CLEAR_EVENT		0x00000002
//#define LXL_USE_EPOLL_EVENT		

#define LXL_READ_EVENT			EPOLLIN
#define LXL_WRITE_EVENT			EPOLLOUT
#define LXL_LEVEL_EVENT			0
#define LXL_CLEAR_EVENT			EPOLLET

#define LXL_CLOSE_EVENT 		1
#define LXL_DISABLE_EVENT		2

#define lxl_add_event		lxl_event_actions.add
#define lxl_del_event		lxl_event_actions.del
#define lxl_add_conn		lxl_event_actions.add_conn
#define lxl_del_conn		lxl_event_actions.del_conn
#define lxl_process_events	lxl_event_actions.process_events
#define lxl_init_events		lxl_event_actions.init
#define lxl_done_events		lxl_event_actions.done

#define lxl_add_timer		lxl_event_add_timer
#define lxl_del_timer		lxl_event_del_timer

#define lxl_event_ident(p)	((lxl_connection_t *) (p))->fd

#define LXL_EVENT_MODULE	0x544E5645  /* "EVNT" */
#define LXL_EVENT_CONF		0x02000000 


typedef void (*lxl_event_handler_pt) (lxl_event_t *ev);

struct lxl_event_s {
	void 			*data;				/* point to c */

	unsigned 		write:1;
	unsigned 		accept:1;
	/*unsigned    instance:1;*/
	unsigned 		active:1;
	unsigned 		ready:1;
	unsigned		eof:1;
	unsigned 		error:1;
	unsigned 		closed:1;
	unsigned 		timedout:1;
	unsigned 		timer_set:1;

	lxl_list_t			list;
	lxl_rbtree_node_t 	timer;
	lxl_event_handler_pt handler;
};

/*typedef struct {
	lxl_connection_t *connection;
	struct sockadd_in sockaddr;
	socklen_t socklen;
} lxl_connection_peer_t;*/

typedef struct {
	int 	(*add) (lxl_event_t *ev, uint32_t event, lxl_uint_t flags);
	int 	(*del) (lxl_event_t *ev, uint32_t event);
	int 	(*add_conn) (lxl_connection_t *c);
	int 	(*del_conn) (lxl_connection_t *c, lxl_uint_t flags);
	int 	(*process_events) (lxl_uint_t timer);
	int 	(*init) (lxl_cycle_t *cycle);
	void	(*done) (void);
} lxl_event_actions_t;

extern lxl_event_actions_t lxl_event_actions;

typedef struct {
	lxl_uint_t connections; 
	lxl_uint_t use;
	lxl_int_t  accept_mutex;
	char 	  *name;
} lxl_event_conf_t;

typedef struct {
	lxl_str_t 		*name;
	void 			*(*create_conf) (lxl_cycle_t *cycle);
	char 			*(*init_conf)	(lxl_cycle_t *cycle, void *conf);
	lxl_event_actions_t actions;
} lxl_event_module_t;


extern lxl_uint_t 	lxl_use_accept_mutex;
extern lxl_list_t 	lxl_posted_event_list;
extern lxl_module_t lxl_events_module;


#define lxl_event_get_conf(conf_ctx, module)			\
	(*(lxl_get_conf(conf_ctx, lxl_events_module))) [module.ctx_index]


//lxl_int_t 	lxl_event_process_init(lxl_cycle_t *cycle);
void		lxl_process_events_and_timers(lxl_cycle_t *cycle);
lxl_int_t	lxl_handler_read_event(lxl_event_t *rev);
lxl_int_t   lxl_handler_write_event(lxl_event_t *wev);

void 		lxl_event_accept(lxl_event_t *ev);
void 		lxl_event_accept_udp(lxl_event_t *ev);

lxl_int_t  	lxl_event_connect_peer(lxl_connection_t **c, struct sockaddr *sa, socklen_t len);
lxl_int_t 	lxl_event_connect_peer_udp(lxl_connection_t **c);


#endif /* LXL_EVENT_H_INCLUDE */
