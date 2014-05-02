
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_inet.h>
#include <lxl_socket.h>
#include <lxl_event.h>
#include <lxl_event_timer.h>
#include <lxl_connection.h>


lxl_listening_t *
lxl_create_listening(lxl_conf_t *cf, void *sockaddr, socklen_t socklen, int type)
{
	size_t len;
	lxl_listening_t *ls;
	struct sockaddr *sa;
	struct sockaddr_in *sin;
	char text[LXL_SOCKADDR_STRLEN] = { 0 };

	ls = lxl_array_push(&cf->cycle->listening);
	if (ls == NULL) {
		return NULL;
	}

	memset(ls, 0x00, sizeof(lxl_listening_t));

	sa = lxl_palloc(cf->pool, socklen);
	if (sa == NULL) {
		return NULL;
	}

	memcpy(sa, sockaddr, socklen);
	sin = (struct sockaddr_in *) sa;
	if (inet_ntop(sa->sa_family, &sin->sin_addr, text, LXL_SOCKADDR_STRLEN) == NULL) {
		lxl_log_error(LXL_LOG_ERROR, errno, "inet_ntop() failed");
	}

	len = strlen(text);
	ls->addr_text = lxl_pnalloc(cf->pool, len + 1);
	if (ls->addr_text == NULL) {
		return NULL;
	}

	memcpy(ls->addr_text, text, len + 1);
	
	ls->sockaddr = sa;
	ls->socklen = socklen;
	ls->fd = -1;
	ls->type = type;
	ls->backlog = LXL_LISTEN_BACKLOG;
	ls->rcvbuf = -1;
	ls->sndbuf = -1;

	return ls;
}

int 
lxl_open_listening_sockets(lxl_cycle_t *cycle)
{
	int fd, reuseaddr;
	lxl_uint_t i, tries, nelts, failed;
	lxl_listening_t *ls;

	// tcp or udp
	reuseaddr = 1;
	for (tries = 5; tries; --tries) {
		failed = 0;
		nelts = lxl_array_nelts(&cycle->listening);
		ls = lxl_array_elts(&cycle->listening);
		for (i = 0; i < nelts; ++i) {
			if (ls[i].fd != -1) {
				continue;
			}

			fd = socket(ls[i].sockaddr->sa_family, ls[i].type, 0);
			if (fd == -1) {
				lxl_log_error(LXL_LOG_EMERG, errno, "socket() %s failed", ls[i].addr_text);
				return -1;
			}
			
			if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *) &reuseaddr, sizeof(int)) == -1) {
				lxl_log_error(LXL_LOG_EMERG, errno, "setsockopt(SO_REUEADDR) %s failed", ls[i].addr_text);
				goto failed;
			}

			if (lxl_nonblocking(fd) == -1) {
				lxl_log_error(LXL_LOG_EMERG, errno, lxl_nonblocking_n " %s failed", ls[i].addr_text);
				goto failed;
			}

			lxl_log_debug(LXL_LOG_DEBUG_CORE, 0, "bind() to %s#%d", ls[i].addr_text);
			if (bind(fd, ls[i].sockaddr, ls[i].socklen) == -1) {
				lxl_log_error(LXL_LOG_EMERG, errno, "bind() to %s failed", ls[i].addr_text);

				if (close(fd) == -1) {
					lxl_log_error(LXL_LOG_EMERG, errno, "close() %s failed", ls[i].addr_text);
				}

				if (errno != EADDRINUSE) {
					return -1;
				}

				failed = 1;
				
				continue;
			}

			if (ls[i].type == SOCK_STREAM) {
				if(listen(fd, ls[i].backlog) == -1) {
					lxl_log_error(LXL_LOG_EMERG, errno, "listen() to %s, backlog %d failed", ls[i].addr_text, ls[i].backlog);
					goto failed;
				}
			}

			ls[i].listen = 1;
			ls[i].fd = fd;
		}

		if (!failed) {
			break;
		}

		lxl_log_error(LXL_LOG_INFO, 0, "try again to bind() after 500ms");
		lxl_msleep(500);
	}

	if (failed) {
		lxl_log_error(LXL_LOG_EMERG, 0, "still could not bind()");
		return -1;
	}

	return 0;

failed:

	if (close(fd) == -1) {
		lxl_log_error(LXL_LOG_EMERG, errno, "close() %s failed", ls[i].addr_text);
	}

	return -1;
}

void 
lxl_configure_listening_sockets(lxl_cycle_t *cycle)
{
	lxl_uint_t 	i, nelts;
	lxl_listening_t *ls;

	nelts = lxl_array_nelts(&cycle->listening);
	ls = lxl_array_elts(&cycle->listening);
	for (i = 0; i < nelts; ++i) {
		if (ls[i].rcvbuf != -1) {
			if (setsockopt(ls[i].fd, SOL_SOCKET, SO_RCVBUF, (const void *) &ls[i].rcvbuf, sizeof(int)) == -1) {
				lxl_log_error(LXL_LOG_ALERT, errno, "setsockopt(SO_RCVBUF, %d) %s failed, ignored", ls[i].rcvbuf, ls[i].addr_text);
			}
		}

		if (ls[i].sndbuf != -1) {
			if (setsockopt(ls[i].fd, SOL_SOCKET, SO_SNDBUF, (const void *) &ls[i].sndbuf, sizeof(int)) == -1) {
				lxl_log_error(LXL_LOG_ALERT, errno, "setsockopt(SO_SNDBUF, %d) %s failed, ignored", ls[i].sndbuf, ls[i].addr_text);
			}
		}
		/* keepalive */
		/* setfib route */
		if (ls[i].listen && ls[i].type == SOCK_STREAM) {
			/* chanage */
			if (listen(ls[i].fd, ls[i].backlog) == -1) {
				lxl_log_error(LXL_LOG_ALERT, errno, "listen() to %s, blacklog %d failed, ignored", ls[i].addr_text, ls[i].backlog);
			}	
		}
		/* accept filter */
		/* tcp deffer accept */
	}
}

lxl_connection_t *
lxl_get_connection(int fd)
{
	lxl_event_t *rev, *wev;
	lxl_connection_t *c;

	lxl_log_debug(LXL_LOG_DEBUG_CORE, 0, "get connection fd:%d", fd);
	c = lxl_cycle->free_connections;
	if (c == NULL) {
		lxl_log_error(LXL_LOG_ERROR, 0, "%lu worker_connections not enough", lxl_cycle->connection_n);
		return NULL;
	}

	lxl_cycle->free_connections = c->data;
	--lxl_cycle->free_connection_n;

	rev = c->read;
	wev = c->write;
	memset(c, 0, sizeof(lxl_connection_t));
	c->read = rev;
	c->write = wev;
	c->fd = fd;
	//c->log = log;
	memset(rev, 0, sizeof(lxl_event_t));
	memset(wev, 0, sizeof(lxl_event_t));
	rev->data = c;
	wev->data = c;
	wev->write = 1;

	return c;
}

void
lxl_close_connection(lxl_connection_t *c)
{
	//int fd;
	lxl_log_debug(LXL_LOG_DEBUG_CORE, 0, "close connection socket %d", c->fd);
	if (c->fd == -1) {
		lxl_log_error(LXL_LOG_ERROR, 0, "connection already closed");
		return;
	}

	/* only udp upstream on read */
	if (c->read->timer_set) {	
		lxl_del_timer(c->read);
	}

	if (c->write->timer_set) {
		lxl_del_timer(c->write);
	}

	//lxl_del_conn(c, LXL_CLOSE_EVENT);
	// LXL_CLOSE_EVENT close
	/*if (c->read->active) {
		lxl_del_event(c->read, LXL_READ_EVENT, LXL_CLOSE_EVENT);
	}

	if (c->write->active) {
		lxl_del_event(c->write, LXL_WRITE_EVENT, LXL_CLOSE_EVENT);
	}*/
	c->read->closed = 1;
	c->write->closed = 1;
	
	lxl_free_connection(c);
	
	if (c->closefd) {
//		fd = c->fd;
//		c->fd = -1;
		if (close(c->fd) ==-1) {
			lxl_log_error(LXL_LOG_ALERT, errno, "close socket %d failed", c->fd);
		}
	}

	c->fd = -1;
}

/*void
lxl_close_connection_udp(lxl_connection_t *c)
{
	lxl_log_debug(LXL_LOG_DEBUG_CORE, 0, "close udp connection fd:%d", c->fd);
	if (c->fd == -1) {
		lxl_log_error(LXL_LOG_ERROR, 0, "connection already closed");
		return -1;
	}

	if (c->read->timer_set) {
		lxl_del_timer(c->read);
	}

	if (c->write->timer_set) {
		lxl_del_timer(c->write);
	}

	//lxl_del_conn(c, LXL_CLOSE_EVENT);
	// LXL_CLOSE_EVENT close
	c->read->closed = 1;
	c->write->closed = 1;
	
	lxl_free_connection(c);
}*/
