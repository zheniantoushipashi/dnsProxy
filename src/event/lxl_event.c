
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_log.h>
#include <lxl_buf.h>
#include <lxl_list.h>
#include <lxl_string.h>
#include <lxl_alloc.h>
#include <lxl_palloc.h>
#include <lxl_cycle.h>
#include <lxl_connection.h>
#include <lxl_conf_file.h>
#include <lxl_socket.h>
#include <lxl_event_timer.h>
#include <lxl_event.h>


#define DEFAULT_CONNECTIONS		512


extern lxl_module_t lxl_epoll_module;


static void	lxl_enable_accept_events(lxl_cycle_t *cycle);
static void lxl_disable_accept_events(lxl_cycle_t *cycle);
static void lxl_close_accepted_connection(lxl_connection_t *c, lxl_uint_t flags);

static char *lxl_event_init_conf(lxl_cycle_t *cycle, void *conf);
static int	 lxl_event_module_init(lxl_cycle_t *cycle);
static int	 lxl_event_process_init(lxl_cycle_t *cycle);
static char *lxl_events_block(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);

static char *lxl_event_connections(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
static char *lxl_event_use(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
static void *lxl_event_core_create_conf(lxl_cycle_t *cycle);
static char *lxl_event_core_init_conf(lxl_cycle_t *cycle, void *conf);


lxl_uint_t			lxl_event_max_module;

lxl_uint_t			lxl_use_accept_mutex;
lxl_int_t			lxl_accept_disabled;

lxl_uint_t 			lxl_event_flags = LXL_USE_CLEAR_EVENT;
lxl_event_actions_t lxl_event_actions;

lxl_list_t			lxl_posted_event_list;


static lxl_command_t lxl_events_commands[] = {

	{ lxl_string("events"),
	  LXL_MAIN_CONF|LXL_CONF_BLOCK|LXL_CONF_NOARGS,
	  lxl_events_block,
	  0,
	  0,
 	  NULL },

	lxl_null_command
};

static lxl_core_module_t lxl_events_module_ctx = {
	lxl_string("events"),
	NULL,
	lxl_event_init_conf
};

lxl_module_t lxl_events_module = {
	0,
	0,
	(void *) &lxl_events_module_ctx,
	lxl_events_commands,
	LXL_CORE_MODULE,
	NULL,
	NULL
};

static lxl_str_t event_core_name = lxl_string("event_core");

static lxl_command_t lxl_event_core_commands[] = {

	{ lxl_string("worker_connections"),
	  LXL_EVENT_CONF|LXL_CONF_TAKE1,
	  lxl_event_connections,
	  0,
	  0,
	  NULL },
	
	{ lxl_string("use"),
      LXL_EVENT_CONF|LXL_CONF_TAKE1,
	  lxl_event_use,
      0,
	  0,
	  NULL },

	{ lxl_string("accept_mutex"),
	  LXL_EVENT_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_flag_slot,
	  0,
	  offsetof(lxl_event_conf_t, accept_mutex),
	  NULL },

	lxl_null_command
};

lxl_event_module_t lxl_event_core_module_ctx = {
	&event_core_name,
	lxl_event_core_create_conf,
	lxl_event_core_init_conf,

	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

lxl_module_t lxl_event_core_module = {
	0,
	0,
	(void *) &lxl_event_core_module_ctx,
	lxl_event_core_commands,
	LXL_EVENT_MODULE,
	lxl_event_module_init,
	lxl_event_process_init,
};


void 
lxl_process_events_and_timers(lxl_cycle_t *cycle)
{
	lxl_uint_t timer, delta;
	
#if LXL_DEBUG
	lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "process events and timer");
#endif 

	timer = lxl_event_find_timer();
	if (lxl_use_accept_mutex) {
		if (lxl_accept_disabled > 0) {
			lxl_accept_disabled = cycle->connection_n / 8 - cycle->free_connection_n;
			if (lxl_accept_disabled <= 0) {
				lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "enable accept events");
				lxl_enable_accept_events(cycle);
			}
		} else {
			lxl_accept_disabled = cycle->connection_n / 8 - cycle->free_connection_n;
			if (lxl_accept_disabled > 0) {
				lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "disable accept events");
				lxl_disable_accept_events(cycle);
			}
		}
	}

	delta = lxl_current_msec;
	lxl_process_events(timer);
	delta = lxl_current_msec - delta;
	lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "timer delta: %lu", delta);

	if (delta) {
		/* timer not exact */
		lxl_event_expire_timers();
	}
}

lxl_int_t
lxl_handler_read_event(lxl_event_t *rev)
{
	if (lxl_event_flags & LXL_USE_CLEAR_EVENT) {
		if (!rev->active && !rev->ready) {
			return lxl_add_event(rev, LXL_READ_EVENT, LXL_CLEAR_EVENT);
		}
		
		return 0;
	} else if (lxl_event_flags & LXL_USE_LEVEL_EVENT) {
		if (!rev->active && !rev->ready) {
			return lxl_add_event(rev, LXL_READ_EVENT, LXL_LEVEL_EVENT);
		}

		return 0;
	}

	return 0;
}

void
lxl_event_accept(lxl_event_t *ev)
{
	int fd;
	char ip[INET6_ADDRSTRLEN];
	socklen_t socklen;
	struct sockaddr sockaddr;
	struct sockaddr_in *sin;
	lxl_listening_t *ls;
	lxl_connection_t *c, *lc;
	
	lc = ev->data;
	ls = lc->listening;
	ev->ready = 0;	// ?
	//ls = lc->listening;
	for (; ;) {
		// accept4();
		//socklen = sizeof(struct sockaddr);
		fd = accept(lc->fd, &sockaddr, &socklen);
		if (fd != -1) {
			c = lxl_get_connection(fd);
			if (c == NULL) {
				if (close(fd) == -1) {
					lxl_log_error(LXL_LOG_ALERT, errno, "close() socket failed");
				}
				return;
			}

			lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "accept pool size %ld", ls->pool_size);
			c->pool = lxl_create_pool(ls->pool_size);
			if (c->pool == NULL) {
				lxl_close_accepted_connection(c, LXL_CONNECTION_CLOSE_SOCKET);
				return;
			}

			if (lxl_nonblocking(fd) == -1) {
				lxl_log_error(LXL_LOG_ALERT, errno, "nonblocking failed");
				lxl_close_accepted_connection(c, LXL_CONNECTION_CLOSE_SOCKET);
				return;
			}
	
			c->udp = 0;
			c->closefd = 1;
			c->recv = lxl_recv;
			c->send = lxl_send;
			c->write->ready = 1;
			
			sin = (struct sockaddr_in *) &sockaddr;
			lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "accept %s fd:%d", 
					inet_ntop(AF_INET, &sin->sin_addr, ip, INET6_ADDRSTRLEN), fd);

			ls->handler(c); 
		} else {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				lxl_log_debug(LXL_LOG_DEBUG_EVENT, errno, "accept() not ready");
				return;
			} else if (errno == EINTR || errno == ECONNABORTED) {
				lxl_log_error(LXL_LOG_WARN, errno, "accept() failed");
				continue;
			} else {
				lxl_log_error(LXL_LOG_ERROR, errno, "accept() failed");
			}
			
			return;
		}
	}
}

void 
lxl_event_accept_udp(lxl_event_t *ev)
{
#define LXL_UDP_SIZE	512
	ssize_t n;
	char buffer[LXL_UDP_SIZE], ip[INET6_ADDRSTRLEN];
	socklen_t socklen;
	struct sockaddr sockaddr;
	struct sockaddr_in *sin;
	lxl_listening_t *ls;
	lxl_connection_t *c, *lc;

	lc = ev->data;
	ls = lc->listening;
	for (; ;) {
		socklen = sizeof(struct sockaddr);	
		n = recvfrom(lc->fd, buffer, LXL_UDP_SIZE, 0, &sockaddr, &socklen);
		if (n != -1) {
			c = lxl_get_connection(lc->fd);
			if (c == NULL) {
				return;
			}

			lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "accept udp pool size %ld", ls->pool_size);
			c->pool = lxl_create_pool(ls->pool_size);
			if (c->pool == NULL) {
				lxl_close_accepted_connection(c, LXL_CONNECTION_NOT_CLOSE_SOCKET);
				return;
			}

			c->buffer = lxl_create_temp_buf(c->pool, LXL_UDP_SIZE);
			if (c->buffer == NULL) {
				lxl_close_accepted_connection(c, LXL_CONNECTION_NOT_CLOSE_SOCKET);
				return;
			}

			c->buffer->len = n;
			memcpy(c->buffer->data, buffer, n);
			c->udp = 1;
			c->closefd = 0;
			c->sockaddr = sockaddr;
			c->socklen = socklen;
			//c->recv = lxl_recvfrom;	
			c->send = lxl_sendto;	/* udp only send */

			sin = (struct sockaddr_in *) &sockaddr;
			lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "recvfrom() %s fd:%d nbyte:%ld", 
						inet_ntop(AF_INET, &sin->sin_addr, ip, INET6_ADDRSTRLEN), lc->fd, n);

			ls->handler(c);
		} else {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				lxl_log_debug(LXL_LOG_DEBUG_EVENT, errno, "recvfrom() not ready");
				return;
			} else if (errno == EINTR) {
				lxl_log_debug(LXL_LOG_DEBUG_EVENT, errno, "recvfrom() failed");
				continue;
			} else {
				lxl_log_error(LXL_LOG_ALERT, errno, "recvfrom() failed");
				return;
			}
		}
	}
}

static void	
lxl_enable_accept_events(lxl_cycle_t *cycle)
{
	lxl_uint_t i, nelts;
	lxl_listening_t *ls;
	lxl_connection_t *c;

	nelts = lxl_array_nelts(&cycle->listening);
	for (i = 0; i < nelts; ++i) {
		ls = lxl_array_data(&cycle->listening, lxl_listening_t, i);
		c = ls->connection;
		if (c->read->active) {
			continue;
		}

		lxl_add_event(c->read, LXL_READ_EVENT, 0);
	}
}

static void 
lxl_disable_accept_events(lxl_cycle_t *cycle) 
{
	
	lxl_uint_t i, nelts;
	lxl_listening_t *ls;
	lxl_connection_t *c;

	nelts = lxl_array_nelts(&cycle->listening);
	for (i = 0; i < nelts; ++i) {
		ls = lxl_array_data(&cycle->listening, lxl_listening_t, i);
		c = ls->connection;
		if (!c->read->active) {
			continue;
		}

		lxl_del_event(c->read, LXL_READ_EVENT/*, LXL_DISABLE_EVENT*/);
	}
}

static void 
lxl_close_accepted_connection(lxl_connection_t *c, lxl_uint_t flags)
{
	lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "close accepted connection socket %d", c->fd);

	lxl_free_connection(c);

	if (flags == LXL_CONNECTION_CLOSE_SOCKET) {
		if (close(c->fd) == -1) {
			lxl_log_error(LXL_LOG_ALERT, errno, "close socket %d failed", c->fd);
		}
	}
	c->fd = -1;

	if (c->pool) {
		lxl_destroy_pool(c->pool);
	}
}

lxl_int_t 
lxl_event_connect_peer(lxl_connection_t **c, struct sockaddr *sa, socklen_t len)
{
	lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "event connect peer");

	return 0;
}

lxl_int_t
lxl_event_connect_peer_udp(lxl_connection_t **c)
{
	int fd;
	lxl_connection_t *connection;

	lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "event udp connect peer");
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1) {
		lxl_log_error(LXL_LOG_ERROR, errno, "socket() failed");
		return -1;
	}

	lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "socket %d", fd);
	connection = lxl_get_connection(fd);
	if (connection == NULL) {
		lxl_log_error(LXL_LOG_WARN, 0, "get connection() failed");
		close(fd);
		return -1;
	}
	
	// nonblock
	connection->udp = 1;
	connection->closefd = 1;
	connection->fd = fd;
	//connection->sockaddr = *sa;
	//connection->socklen = len;
	connection->recv = lxl_recvfrom;
	connection->send = lxl_sendto;
	*c = connection;

	return 0;
}

static char *
lxl_event_init_conf(lxl_cycle_t *cycle, void *conf)
{
	if (lxl_get_conf(cycle->conf_ctx, lxl_events_module) == NULL) {
		lxl_log_error(LXL_LOG_EMERG, 0, "no \"events\" section in configueation");
		return LXL_CONF_ERROR;
	}

	return LXL_CONF_OK;
}

static int	 
lxl_event_module_init(lxl_cycle_t *cycle)
{
	void 			  ***cf;
	struct rlimit		 rlmt;
	lxl_int_t			 limit;
	lxl_core_conf_t		*ccf;
	lxl_event_conf_t	*ecf;
	
	ccf = (lxl_core_conf_t *) lxl_get_conf(cycle->conf_ctx, lxl_core_module);
	cf = lxl_get_conf(cycle->conf_ctx, lxl_events_module);
	ecf = (*cf)[lxl_event_core_module.ctx_index];
	lxl_log_error(LXL_LOG_INFO, 0, "using the \"%s\" event method", ecf->name);

	if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
		lxl_log_error(LXL_LOG_ALERT, errno, "getrlimit(RLIMIT_NOFILE) failed, ignored");
	} else {
		if (ecf->connections > (lxl_uint_t) rlmt.rlim_cur
			&& (ccf->rlimit_nofile == LXL_CONF_UNSET || ecf->connections > (lxl_uint_t) ccf->rlimit_nofile)) {
			limit = (ccf->rlimit_nofile == LXL_CONF_UNSET) ? (lxl_int_t) rlmt.rlim_cur : ccf->rlimit_nofile;
			lxl_log_error(LXL_LOG_WARN, 0, "%lu worker_connections exceed open file resouse limit: %ld", 
							ecf->connections, limit);
		}
	}

	return 0;
}

int
lxl_event_process_init(lxl_cycle_t *cycle)
{
	lxl_uint_t 			i, nelts;
	lxl_connection_t 	*c, *next;
	lxl_listening_t 	*ls;
	lxl_core_conf_t 	*ccf;
	lxl_event_conf_t 	*ecf;
	lxl_event_module_t  *m;

	ccf = (lxl_core_conf_t *) lxl_get_conf(cycle->conf_ctx, lxl_core_module);
	ecf = (*(lxl_get_conf(cycle->conf_ctx, lxl_events_module))) [lxl_event_core_module.ctx_index];

	if (ccf->worker_process > 1 && ecf->accept_mutex) {
		lxl_use_accept_mutex = 1;
	} else {
		lxl_use_accept_mutex = 0;
	}
	lxl_accept_disabled = 1;

	//lxl_list_init(&lxl_posted_event_list);
	lxl_event_timer_init();

	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_EVENT_MODULE) {
			continue;
		}

		if (lxl_modules[i]->ctx_index != ecf->use) {
			continue;
		}

		m = lxl_modules[i]->ctx;
		if (m->actions.init(cycle) != 0) {
			/* fatal */
			exit(2);
		}

		break;
	}
	
	cycle->connections = lxl_alloc(cycle->connection_n * sizeof(lxl_connection_t));
	if (cycle->connections == NULL) {
		return -1;
	}

	cycle->read_events = lxl_alloc(cycle->connection_n * sizeof(lxl_event_t));
	if (cycle->read_events == NULL) {
		return -1;
	}

	for (i = 0; i < cycle->connection_n; ++i) {
		cycle->read_events[i].closed = 1;
	}

	cycle->write_events = lxl_alloc(cycle->connection_n * sizeof(lxl_event_t));
	if (cycle->write_events == NULL) {
		return -1;
	}

	for (i = 0; i < cycle->connection_n; ++i) {
		cycle->write_events[i].closed = 1;
	}

	next = NULL;
	i = cycle->connection_n;
	c = cycle->connections;
	do {
		--i;
		c[i].data = next;
		c[i].fd = -1;
		c[i].read = &cycle->read_events[i];
		c[i].write = &cycle->write_events[i];
		next = &c[i];
	} while (i);
	cycle->free_connection_n = cycle->connection_n;
	cycle->free_connections = next;

	ls = lxl_array_elts(&cycle->listening);
	nelts = lxl_array_nelts(&cycle->listening);
	for (i = 0; i < nelts; ++i) {
		c = lxl_get_connection(ls[i].fd);
		if (c == NULL) {
			return -1;
		}

		c->listening = &ls[i];
		ls[i].connection = c;
		//rev = c->read;
		c->read->accept = 1;
		if (ls[i].type == SOCK_STREAM) {
			c->udp = 0;
			c->closefd = 1;
			c->read->handler = lxl_event_accept;	/* tcp or udp */
		} else {
			c->udp = 1;
			c->closefd = 1;
			c->read->handler = lxl_event_accept_udp;
			/*c->recv = lxl_recvfrom;
			c->send = lxl_sendto;*/
		}

		if (lxl_use_accept_mutex) {
			continue;
		}

		if (lxl_add_event(c->read, LXL_READ_EVENT, LXL_LEVEL_EVENT) == -1) {
			return -1;
		}
	}

	return 0;	
}

static char *
lxl_events_block(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	char 	  		   *rv;
	lxl_uint_t 			i;
	void 			 ***ctx;
	lxl_conf_t 			pcf;
	lxl_event_module_t *m;

	if (*(void **) conf) {
		return "is duplicate";
	}

	lxl_event_max_module = 0;
	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_EVENT_MODULE) {
			continue;
		}

		lxl_modules[i]->ctx_index = lxl_event_max_module++;
	}
	
	ctx = lxl_pcalloc(cf->pool, sizeof(void *));
	if (ctx == NULL) {
		return LXL_CONF_ERROR;
	}
	
	*ctx = lxl_pcalloc(cf->pool, lxl_event_max_module * sizeof(void *));
	if (*ctx == NULL) {
		return LXL_CONF_ERROR;
	}

	*(void **) conf = ctx;

	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_EVENT_MODULE) {
			continue;
		}

		m = lxl_modules[i]->ctx;
		if (m->create_conf) {
			(*ctx)[lxl_modules[i]->ctx_index] = m->create_conf(cf->cycle);
			if ((*ctx)[lxl_modules[i]->ctx_index] == NULL) {
				return LXL_CONF_ERROR;
			}
		}
	}

	pcf = *cf;	
	cf->ctx = ctx;
	cf->module_type = LXL_EVENT_MODULE;
	cf->cmd_type = LXL_EVENT_CONF;
	rv = lxl_conf_parse(cf, NULL);
	*cf = pcf;
	if (rv != LXL_CONF_OK) {
		return rv;
	}

	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_EVENT_MODULE) {
			continue;
		}

		m = lxl_modules[i]->ctx;
		if (m->init_conf) {
			rv = m->init_conf(cf->cycle, (*ctx)[lxl_modules[i]->ctx_index]);
			if (rv != LXL_CONF_OK) {
				return rv;
			}
		}
	}

	return LXL_CONF_OK;
}

static char *
lxl_event_connections(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	lxl_event_conf_t *ecf = (lxl_event_conf_t *) conf;
	
	lxl_str_t *value;

	if (ecf->connections != LXL_CONF_UNSET_UINT) {
		return "is duplicate";
	}

	value = lxl_array_elts(cf->args);
	ecf->connections = atoi(value[1].data);
	if (ecf->connections <= 0) {
		lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "invalid number \"%s\"", value[1].data);
		return LXL_CONF_ERROR;
	}

	cf->cycle->connection_n = ecf->connections;

	return LXL_CONF_OK;
}

static char *
lxl_event_use(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	lxl_event_conf_t *ecf = (lxl_event_conf_t *) conf;

	lxl_int_t i;
	lxl_str_t *value;
	lxl_event_module_t *module;

	if (ecf->use != LXL_CONF_UNSET_UINT) {
		return "is duplicate";
	}

	value = lxl_array_elts(cf->args);
	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_EVENT_MODULE) {
			continue;
		}

		module = (lxl_event_module_t *) lxl_modules[i]->ctx;
		if (module->name->len == value[1].len 
			&& memcmp(module->name->data, value[1].data, value[1].len) == 0) {
			ecf->use = lxl_modules[i]->ctx_index;
			ecf->name = module->name->data;	
			return LXL_CONF_OK;
		}
	}

	lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "invalid event type \"%s\"", value[1].data);
	return LXL_CONF_ERROR;
}

static void *
lxl_event_core_create_conf(lxl_cycle_t *cycle)
{
	lxl_event_conf_t *ecf;

	ecf = lxl_palloc(cycle->pool, sizeof(lxl_event_conf_t));
	if (ecf == NULL) {
		return NULL;
	}

	ecf->connections = LXL_CONF_UNSET_UINT;
	ecf->use = LXL_CONF_UNSET_UINT;	
	ecf->accept_mutex = LXL_CONF_UNSET;
	ecf->name = (void *) LXL_CONF_UNSET;

	return ecf;
}

static char *
lxl_event_core_init_conf(lxl_cycle_t *cycle, void *conf)
{
	lxl_event_conf_t *ecf = (lxl_event_conf_t *) conf;

	lxl_uint_t i;
	lxl_module_t *module;
	lxl_event_module_t *event_module;

	module = NULL;

#if LXL_HAVE_EPOLL

		module = &lxl_epoll_module;

#endif 

#if LXL_HAVE_KQUEUE

		/* module = &lxl_kqueue_module */

#endif

	if (module == NULL) {
		for (i = 0; lxl_modules[i]; ++i) {
			if (lxl_modules[i]->type != LXL_EVENT_MODULE) {
				continue;
			}

			event_module = lxl_modules[i]->ctx;
			if (event_module->name->len == event_core_name.len 
					&& memcmp(event_module->name->data, event_core_name.data, event_core_name.len) == 0) {
				continue;
			}

			module = lxl_modules[i];
			break;
		}
	}

	if (module == NULL) {
		lxl_log_error(LXL_LOG_EMERG, 0, "no events module found");
		return LXL_CONF_ERROR;
	}

	lxl_conf_init_uint_value(ecf->connections, DEFAULT_CONNECTIONS);
	cycle->connection_n = ecf->connections;
	lxl_conf_init_uint_value(ecf->use, module->ctx_index);
	lxl_conf_init_value(ecf->accept_mutex, 0);
	
	return LXL_CONF_OK;
}
