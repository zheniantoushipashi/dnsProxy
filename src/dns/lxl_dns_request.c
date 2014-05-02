
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_palloc.h>
#include <lxl_list.h>
#include <lxl_hash.h>
#include <lxl_times.h>
#include <lxl_connection.h>
#include <lxl_event.h>
#include <lxl_event_timer.h>
#include <lxl_dns.h>
#include <lxl_dns_data.h>
#include <lxl_dns_upstream.h>
#include <lxl_dns_request.h>


extern lxl_hash_t lxl_dns_hash;


static void lxl_dns_empty_handler(lxl_event_t *wev);
static void lxl_dns_wait_request_handler(lxl_event_t *rev);
static void lxl_dns_process_request_line(lxl_event_t *rev);

static lxl_dns_request_t * lxl_dns_create_request(lxl_connection_t *c);
static void lxl_dns_process_request(lxl_dns_request_t *r);
static void lxl_dns_writer_udp(lxl_dns_request_t *r);


void 
lxl_dns_init_connection(lxl_connection_t *c)
{
	lxl_event_t *rev;

	rev = c->read;
	rev->handler = lxl_dns_wait_request_handler;
	c->write->handler = lxl_dns_empty_handler;
	
	/* udp or deffered accept */
	if (c->udp /*|| rev->ready*/) {
		rev->handler(rev);
		return;
	}

	/* add timer dns 3s */
	lxl_add_timer(rev, LXL_DNS_CLIENT_TIMEOUT);
	// reuseable connection
	if (lxl_handler_read_event(c->read) != 0) {
		lxl_dns_close_connection(c);
		return;
	}
}

static void 
lxl_dns_empty_handler(lxl_event_t *wev)
{
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns empty handler");

	return;
}

static void 
lxl_dns_wait_request_handler(lxl_event_t *rev)
{
	ssize_t n;
	lxl_buf_t *b;
	lxl_connection_t *c;
	lxl_dns_request_t *r;
	
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns wait request handler");
	c = rev->data;

	if (c->udp) {
		r = lxl_dns_create_request(c);
		if (r == NULL) {
			lxl_log_error(LXL_LOG_ERROR, 0, "dns create request failed");
			lxl_dns_close_connection(c);
			return;
		}

		c->data = r;
		if (lxl_dns_parse_request(r, r->connection->buffer->data) == -1) {
			lxl_dns_finalize_request(r, r->rcode);
			return;
		}

		lxl_dns_process_request(r);
		return;
	}
	
	if (rev->timedout) {
		lxl_log_error(LXL_LOG_INFO, 0, "client timed out");
		lxl_dns_close_connection(c);
		return;
	}
	// c->close

	b = c->buffer;
	if (b == NULL) {
		b = lxl_create_temp_buf(c->pool, LXL_DNS_CLIENT_BUFFER_SIZE);
		if (b == NULL) {
			lxl_dns_close_connection(c);
			return;
		}

		c->buffer = b;
	}

	n = c->recv(c, b->data, b->nalloc);
	switch (n) {
		case LXL_EAGAIN:
			if (!rev->timer_set) {
				lxl_add_timer(rev, LXL_DNS_CLIENT_TIMEOUT);
			}

			if (lxl_handler_read_event(rev) != 0) {
				lxl_dns_close_connection(c);
				return;
			}
			
			return;
			break;
		
		case 0:
			lxl_log_error(LXL_LOG_INFO, 0, "client closed connection");
		case LXL_ERROR:
			lxl_dns_close_connection(c);

			return;
			break;

		case 1:
			lxl_log_error(LXL_LOG_INFO, 0, "recv client 1 byte");
			lxl_dns_close_connection(c);
			
			return;
			break;

		default:
			b->len += n;
			break;
	}

	// reuseable
	c->data = lxl_dns_create_request(c);
	if (c->data == NULL) {
		lxl_dns_close_connection(c);
		return;
	} 

	rev->handler = lxl_dns_process_request_line;
	lxl_dns_process_request_line(rev);
}

static void 
lxl_dns_process_request_line(lxl_event_t *rev)
{
	ssize_t n;
	lxl_buf_t *b;
	lxl_connection_t *c;
	lxl_dns_request_t *r;

	c = rev->data;
	r = c->data;
	
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns process request line");
	if (rev->timedout) {	
		lxl_log_error(LXL_LOG_INFO, 0, "client timed out");
		c->timedout = 1;
		lxl_dns_close_request(r);
		return;
	}

	if (r->request_n == 0) {	
		r->request_n = ntohs(*((uint16_t *) c->buffer->data));
		lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "request_n %hu", r->request_n);
		if (c->buffer->len == r->request_n + 2) {
			goto done;
		}
	} else {
		b = c->buffer;
		n = c->recv(c, b->data + b->len, b->nalloc - b->len);
		switch (n) {
			case LXL_EAGAIN:
				/*if (!rev->timer_set) {
					lxl_add_timer(rev, 3000);
				}

				if (lxl_handler_read_event(rev) != 0) {
					lxl_dns_close_connection(c);
					return;
				}*/

				return;
				break;

			case 0:
				lxl_log_error(LXL_LOG_INFO, 0, "client closed connection");
			case LXL_ERROR:
				lxl_dns_close_connection(c);

				return;
				break;

			default:
				b->len += n;
				break;
		}

		if (b->len == r->request_n + 2) {
			goto done;
		}
	}

done:

	if (lxl_dns_parse_request(r, c->buffer->data + 2) == -1) {
		lxl_dns_finalize_request(r, r->rcode);
	}

	lxl_dns_process_request(r);
	return;
}

static lxl_dns_request_t *
lxl_dns_create_request(lxl_connection_t *c)
{
	lxl_dns_request_t *r;

	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns create request");
	r = lxl_palloc(c->pool, sizeof(lxl_dns_request_t));
	if (r == NULL) {
		return NULL;
	}


	r->pool = c->pool;
	r->connection = c;
	r->tc = 0;
	r->rcode = LXL_DNS_RCODE_OK;
	r->ancount = r->nscount = r->arcount = 0;
	r->request_n = 0;
	r->author_rrset = NULL;
	//lxl_array_init(&r->addition_rrset_array, r->pool,  4, sizeof(lxl_dns_rrset_t *));
	lxl_array_init(&r->answer_rrset_array, r->pool, 8, sizeof(lxl_dns_rrset_t *));

	return r;
}

static void 
lxl_dns_process_request(lxl_dns_request_t *r)
{
	uint16_t type, len;
	char *name, *qname;
	lxl_uint_t nelts, flags, qlen;
	lxl_list_t *list;
	lxl_dns_zone_t *zone;
	lxl_dns_rrset_t **p, *rrset;
	lxl_dns_rdata_t *rdata;

	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "%s dns request qname:%s qtype:0x%04x", r->rid, r->qname, r->question_part->qtype);
	name = r->qname;
	len = r->qlen;
	type = r->question_part->qtype;
	if (type == LXL_DNS_CNAME) {
		zone = lxl_dns_zone_find(name, len);
		if (zone == NULL) {
			goto upstream;
		}

		lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns find zone:%s len:%hu", name, len);
		rrset = lxl_dns_rrset_find(&zone->rrset_list, name, len, type, LXL_DNS_RRSET_EXPIRES_FIND);
		if (rrset != NULL) {
			p = lxl_array_push(&r->answer_rrset_array);
			if (p == NULL) {
				goto failed;
			}

			*p = rrset;

			goto succeed;
		}

		goto upstream;
	} else {
		do {
			flags = 0;
			zone = lxl_dns_zone_find(name, len);
			if (zone == NULL) {
				lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns not find zone:%s len:%hu", name, len);
				break;
			}

			lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns find zone:%s len:%hu", name, len);
			do {
				rrset = lxl_dns_rrset_dnsfind(&zone->rrset_list, name, len, type);
				if (rrset == NULL) {
					/* soa speical process */
					if (type == LXL_DNS_SOA) {
						rrset = lxl_dns_rrset_soafind(&zone->rrset_list);
						if (rrset) {
							r->author_rrset = rrset;
							lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns find soa rrset");

							goto succeed;
						}					
					}

					break;
				}

				if (rrset->type == type) {
					p = lxl_array_push(&r->answer_rrset_array);
					if (p == NULL) {
						goto failed;
					}

					*p = rrset;
					goto succeed;
				}

				list = lxl_list_head(&rrset->rdata_list);
				rdata = lxl_list_data(list, lxl_dns_rdata_t, list);
				if (len == rdata->rdlength && memcmp(name, rdata->rdata, len) == 0) {
					lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns find rrset same: name:%s rdata:%s type:%04x", name, rdata->rdata, rrset->type);
					break;
				}

				p = lxl_array_push(&r->answer_rrset_array);
				if (p == NULL) {
					goto failed;
				}

				*p = rrset;
				name = rdata->rdata;
				len = rdata->rdlength;
				flags = 1;
			} while (1);
		} while (flags);
	}

	// aditonal cha  ns or mx 
	nelts = lxl_array_nelts(&r->answer_rrset_array);
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns request find rrset nelts %lu", nelts);
	if (nelts == 0) {

		// not dns  root(.)
		goto upstream;
	} else {
		p = lxl_array_data(&r->answer_rrset_array, lxl_dns_rrset_t *, nelts - 1);
		rrset = *p;
		/*if (rrset->type == r->question_part->qtype) {

			goto succeed;
		}*/
		list = lxl_list_head(&rrset->rdata_list);
		rdata = lxl_list_data(list, lxl_dns_rdata_t, list);
		qname = rdata->rdata;
		qlen = (lxl_uint_t) rdata->rdlength;

		goto upstream2;
	}

	return;

succeed:

	lxl_dns_finalize_request(r, LXL_DNS_RCODE_OK);

	return;

upstream:

	if (lxl_dns_upstream_init(r, r->qname, r->qlen) == 0) {
		if (lxl_dns_upstream_resolver_author(r, r->upstream) == 0) {
			//lxl_dns_upstream_init_request(r);
			lxl_dns_upstream_next(r, r->upstream);
		} else {
			lxl_dns_finalize_request(r, LXL_DNS_RCODE_NAMEERR);
		}
	} else {
		lxl_dns_finalize_request(r, LXL_DNS_RCODE_SERVFAIL);
	}

	return;

upstream2:

	if (lxl_dns_upstream_init(r, qname, qlen) == 0) {
		if (lxl_dns_upstream_resolver_author(r, r->upstream) == 0) {
			lxl_dns_upstream_next(r, r->upstream);
		} else {
			lxl_dns_finalize_request(r, LXL_DNS_RCODE_NAMEERR);
		}
	} else {
		lxl_dns_finalize_request(r, LXL_DNS_RCODE_SERVFAIL);
	}

	return;

failed:

	lxl_dns_finalize_request(r, LXL_DNS_RCODE_SERVFAIL);

	return;
}

static void 
lxl_dns_writer_udp(lxl_dns_request_t *r)
{
	ssize_t n;
	lxl_connection_t *c;

	c = r->connection;
	n = c->send(c, c->buffer->data, c->buffer->len);
	if (n < 0) {
		lxl_log_error(LXL_LOG_ERROR, errno, "%s send(%d) failed", r->rid, c->fd);
	} else {
		lxl_log_error(LXL_LOG_INFO, 0, "%s send %ld success", r->rid, n);
	}
}

void 
lxl_dns_finalize_request(lxl_dns_request_t *r, lxl_uint_t rcode)
{
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "%s dns finalize request qname:%s qtype:0x%04x rcode:%lu", r->rid, r->qname, r->question_part->qtype, rcode);

	r->rcode =  rcode;
	if (rcode == LXL_DNS_RCODE_OK) {
		lxl_dns_package_answer(r);
	}
	lxl_dns_package_header(r);

	lxl_dns_writer_udp(r);
	lxl_dns_close_request(r);
}

void 
lxl_dns_close_request(lxl_dns_request_t *r)
{
	lxl_log_debug(LXL_LOG_DEBUG, 0, "%s dns close request", r->rid);
	lxl_dns_free_request(r);
	lxl_dns_close_connection(r->connection);
}

void 
lxl_dns_free_request(lxl_dns_request_t *r)
{
	//lxl_destroy_pool(r->connection->pool);
}

void
lxl_dns_close_connection(lxl_connection_t *c)
{
	// upstream 里面的connection为NULL
	lxl_close_connection(c);
	lxl_destroy_pool(c->pool);
}
