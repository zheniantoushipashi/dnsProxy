
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_cycle.h>
#include <lxl_string.h>
#include <lxl_times.h>
#include <lxl_connection.h>
#include <lxl_alloc.h>
#include <lxl_event.h>


typedef struct {
	lxl_uint_t events;
} lxl_epoll_conf_t;


static int 		lxl_epoll_init(lxl_cycle_t *cycle);
static void 	lxl_epoll_done(void);
static int		lxl_epoll_add_event(lxl_event_t *ev, uint32_t event, lxl_uint_t flags);
static int 		lxl_epoll_del_event(lxl_event_t *ev, uint32_t event);
static int		lxl_epoll_add_connection(lxl_connection_t *c);
static int 		lxl_epoll_del_connection(lxl_connection_t *c, lxl_uint_t flags);
static int		lxl_epoll_process_events(lxl_uint_t timer);

static void *	lxl_epoll_create_conf(lxl_cycle_t *cycle);
static char *	lxl_epoll_init_conf(lxl_cycle_t *cycle, void *conf);


static int 			epfd = -1;
static lxl_uint_t 	nevents;
static struct epoll_event *event_list;
static lxl_str_t 	epoll_name = lxl_string("epoll");

static lxl_command_t lxl_epoll_commands[] = {
	
	{ lxl_string("epoll_events"),
	  LXL_EVENT_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_num_slot,
	  0,
      offsetof(lxl_epoll_conf_t, events),
      NULL },

	lxl_null_command
};

lxl_event_module_t lxl_epoll_module_ctx = {
	&epoll_name, 
	lxl_epoll_create_conf,
	lxl_epoll_init_conf,

	{
		lxl_epoll_add_event,
		lxl_epoll_del_event,
		lxl_epoll_add_connection,
		lxl_epoll_del_connection,
		lxl_epoll_process_events,
		lxl_epoll_init,
		lxl_epoll_done
	}
};

lxl_module_t lxl_epoll_module = {
	0,
	0,
	(void *) &lxl_epoll_module_ctx,
	lxl_epoll_commands,
	LXL_EVENT_MODULE,
	NULL,
	NULL
};


static int
lxl_epoll_init(lxl_cycle_t *cycle)
{
	lxl_epoll_conf_t *epcf;

	epcf = lxl_event_get_conf(cycle->conf_ctx, lxl_epoll_module);
	lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "epoll conf events %lu, cycle->connection_n %lu", epcf->events, cycle->connection_n);
	epfd = epoll_create(cycle->connection_n / 2);
	if (epfd == -1) {
		lxl_log_error(LXL_LOG_EMERG, errno, "epoll_create(%d) failed", cycle->connection_n / 2);
		return -1;
	}

	event_list = (struct epoll_event *) lxl_alloc(epcf->events * sizeof(struct epoll_event));
	if (event_list == NULL) {
		return -1;
	}

	nevents = epcf->events;
	lxl_event_actions = lxl_epoll_module_ctx.actions;
	
	return 0;
}

static void 
lxl_epoll_done()
{
	if (close(epfd) == -1) {
		lxl_log_error(LXL_LOG_ALERT, errno, "epoll close(%d) failed", epfd);
	}

	epfd = -1;
	lxl_free(event_list);

	event_list = NULL;
	nevents = 0;
}

static int
lxl_epoll_add_event(lxl_event_t *ev, uint32_t event, lxl_uint_t flags)
{
	int op;
	uint32_t events, prev; 
	lxl_event_t *e;
	lxl_connection_t *c;
	struct epoll_event ee;

	c = ev->data;
	events = event;
	if (event == LXL_READ_EVENT) {
		e = c->write;
		prev = EPOLLOUT; 
	} else {
		e = c->read;
		//prev = EPOLLIN|EPOLLRDHUP;
		prev = EPOLLIN;
	}

	if (e->active) {
		op = EPOLL_CTL_MOD;
		events |= prev;
	} else {
		op = EPOLL_CTL_ADD;
	}

	/* events = EPOLLIN; EPOLLET;	default EPOLLLT */
	ee.events = events | flags;
	ee.data.ptr = (void *) c;

	lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "epoll add event: fd:%d, op:%d, ev:%08x", c->fd, op, ee.events);
	if (epoll_ctl(epfd, op, c->fd, &ee) == -1) {
		lxl_log_error(LXL_LOG_ERROR, 0, "epoll_ctl(%d, %d) failed", op, c->fd);
		return -1;
	}

	ev->active = 1;

	return 0;
}

static int
lxl_epoll_del_event(lxl_event_t *ev, uint32_t event)
{
	int op;
	uint32_t prev;
	lxl_event_t *e;
	lxl_connection_t *c;
	struct epoll_event ee;

	c = ev->data;
	if (event == LXL_READ_EVENT) {
		e = c->write;
		prev = EPOLLOUT;
	} else {
		e = c->read;
		prev = EPOLLIN;
	}

	if (e->active) {
		op = EPOLL_CTL_MOD;
		ee.events = prev; 
		ee.data.ptr = (void *) c;
	} else {
		op = EPOLL_CTL_DEL;
		ee.events = 0;
		ee.data.ptr = NULL;
	}

	lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "epoll del event: fd:%d, op:%d, ev:%08x", c->fd, op, ee.events);
	if (epoll_ctl(epfd, op, c->fd, &ee) == -1) {
		lxl_log_error(LXL_LOG_ERROR, 0, "epoll_ctl(%d, %d) failed", op, c->fd);
		return -1;
	}

	ev->active = 0;

	return 0;
}

static int 
lxl_epoll_add_connection(lxl_connection_t *c)
{
	struct  epoll_event ee;

	ee.events = EPOLLIN | EPOLLOUT | EPOLLET; /* fei bianliang */
	ee.data.ptr = (void *) c;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, c->fd, &ee) == -1) {
		return -1;
	}
	c->read->active = 1;
	c->write->active = 1;

	return 0;
}

static int
lxl_epoll_del_connection(lxl_connection_t *c, lxl_uint_t flags)
{
	int op;
	struct epoll_event ee;

	/* close(fd) not to epoll_ctl */
	if (flags & LXL_CLOSE_EVENT) { 
		c->read->active = 0;
		c->write->active = 0;
		return 0;
	}

	op = EPOLL_CTL_DEL;
	ee.events = 0;
	ee.data.ptr = NULL; /* del ? */
	if (epoll_ctl(epfd, op, c->fd, &ee) == -1) {
		return -1;
	}
	c->read->active = 0;
	c->write->active = 0;

	return 0;
}

static int
lxl_epoll_process_events(lxl_uint_t timer)
{
	int events, i;
	uint32_t revents;
	lxl_event_t *rev, *wev;
	lxl_connection_t *c;

	lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "epoll timer: %d", (int) timer);
	events = epoll_wait(epfd, event_list, nevents, (int) timer);

	lxl_time_update();

	if (events == -1) {
		/* EINTR alarm */
		return -1;
	} else if (events == 0) {
		if (timer == -1) {
			return -1;
		}
		
		lxl_log_error(LXL_LOG_ERROR, 0, "epoll_wait() return no events without timeout");
	} else {
		for (i = 0; i < events; ++i) {
			c = (lxl_connection_t *) event_list[i].data.ptr;
			if (c->fd == -1) {
				continue;
			}

			revents = event_list[i].events;
			rev = c->read;
			wev = c->write;
			if (revents & (EPOLLERR|EPOLLHUP)) {
				continue;
				/* handle error */
			} else if ((revents & EPOLLIN) && rev->active) {
				/* rev->ready = 1;*/
				rev->handler(rev);
			} else if ((revents & EPOLLOUT) && wev->active) {
				wev->handler(wev);
			} else {
				lxl_log_error(LXL_LOG_ERROR, errno, "epoll_wait() Unknow events");
			}
		}
	}

	return 0;
}

static void *	
lxl_epoll_create_conf(lxl_cycle_t *cycle)
{
	lxl_epoll_conf_t *epcf;
	
	epcf = lxl_palloc(cycle->pool, sizeof(lxl_epoll_conf_t));
	if (epcf == NULL) {
		return NULL;
	}

	epcf->events = LXL_CONF_UNSET_UINT;

	return epcf;
}

static char *	
lxl_epoll_init_conf(lxl_cycle_t *cycle, void *conf)
{
	lxl_epoll_conf_t *epcf = (lxl_epoll_conf_t *) conf;

	lxl_conf_init_uint_value(epcf->events, 512);

	return LXL_CONF_OK;
}
