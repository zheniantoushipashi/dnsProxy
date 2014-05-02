
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_buf.h>
#include <lxl_list.h>
#include <lxl_hash.h>
#include <lxl_array.h>
#include <lxl_times.h>
#include <lxl_event.h>
#include <lxl_event_timer.h>
#include <lxl_dns_data.h>
#include <lxl_dns_request.h>
//#include <lxl_dns_resolver.h>
#include <lxl_dns_upstream.h>


#define 	LXL_DNS_UPSTREAM_PORT			0x3500

/* 3000 ===> 1500 */
#define 	LXL_DNS_AUTHORITY_READ_TIMEOUT	1500


/* rr |NAME|TYPE(2)|CLASS(2)|TTL(4)|RDLENGTH(2)|RDATA| */
#define		LXL_DNS_IS_OFFSET_PTR(offset)	((offset & 0x00c0) == 0x00c0)
//#define 	LXL_DNS_IS_PTR(offset)			(0xc00 <= offset && offset <= 0xcfff)		// 16c0 11two bit		
#define   	LXL_DNS_GET_OFFSET(offset)		(offset &= 0xff3f)
//#define 	LXL_DNS_GET_OFFSET(offset)		(offset &= 0x3fff)


extern lxl_hash_t 	lxl_dns_hash;
extern lxl_pool_t  *lxl_dns_pool;


static void 	 lxl_dns_upstream_handler(lxl_event_t *rev);
static void 	 lxl_dns_upstream_process_response(lxl_dns_request_t *r, lxl_dns_upstream_t *u);
static void 	 lxl_dns_upstream_finalize_request(lxl_dns_request_t *r, lxl_dns_upstream_t *u, lxl_uint_t rcode);
static size_t 	 lxl_dns_upstream_parse_name(lxl_dns_upstream_t *u, char *data, char *buffer, size_t size, size_t *len);
static size_t 	 lxl_dns_upstream_parse_rdata(lxl_dns_upstream_t *u, char *data, char *buffer, size_t size, size_t *len);

static lxl_int_t lxl_dns_upstream_resolver_rrset(lxl_dns_request_t *r, lxl_dns_upstream_t *u, lxl_dns_zone_t *zone);
static lxl_int_t lxl_dns_upstream_resolver_author2(lxl_dns_request_t *r, lxl_dns_upstream_t *u, lxl_dns_zone_t *zone);
static lxl_int_t lxl_dns_upstream_resolver_author_ip(lxl_dns_upstream_t *u, lxl_dns_zone_t *zone, lxl_dns_rrset_t *rrset, uint16_t flags);
static lxl_int_t lxl_dns_upstream_resolver_author_ip2(lxl_dns_upstream_t *u, lxl_dns_zone_t *zone);


lxl_int_t 
lxl_dns_upstream_init(lxl_dns_request_t *r, char *qname, lxl_uint_t qlen) 
{
	lxl_connection_t *c;
	lxl_dns_upstream_t *u;

	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "%s dns upstream init, qname:%s qlen:%lu", r->rid, qname, qlen);
	u = lxl_palloc(r->pool, sizeof(lxl_dns_upstream_t));
	if (u == NULL) {
		return -1;
	}

	if (lxl_event_connect_peer_udp(&u->connection) == -1) {
		return -1;
	}

	
	c = u->connection;
	c->buffer = lxl_create_temp_buf(r->pool, LXL_DNS_CLIENT_BUFFER_SIZE);
	if (c->buffer == NULL) {
		return -1;
	}

	lxl_array_init(&u->author_ip_array, r->pool, 8, sizeof(uint32_t));
	lxl_stack_init(&u->question_stack);

	c->data = r;
	c->read->handler = lxl_dns_upstream_handler;
	u->read_event_handler = lxl_dns_upstream_process_response;

	u->header = *(r->header);
	u->question_part = *(r->question_part);
	u->qname = qname;
	u->qlen = qlen;  
	u->qname_flags = LXL_DNS_UPSTREAM_NORMAL_QNAME;
	//r->author_rrset = NULL;	/* yao */
	r->upstream = u;

	return 0;
}

void
lxl_dns_upstream_next(lxl_dns_request_t *r, lxl_dns_upstream_t *u)
{
	lxl_uint_t nelts;
	uint32_t *ipv4;
	struct sockaddr_in sockaddr;
	lxl_connection_t *c;

	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "%s dns upstream next: qname:%s type:0x%04x flags:0x%04x", 
					r->rid, u->qname, u->question_part.qtype, u->qname_flags);
	nelts = lxl_array_nelts(&u->author_ip_array);
	if (u->author_ip_curr_index == nelts) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dns upstream qname %s author ip used %ld", u->qname, nelts);
		// finize request or wang shangzhao zhao mei bi yao  
		lxl_dns_upstream_finalize_request(r, u, LXL_DNS_RCODE_SERVFAIL);
		return;
	}

	++(u->header.id);
	ipv4 = lxl_array_data(&u->author_ip_array, uint32_t, u->author_ip_curr_index);
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = LXL_DNS_UPSTREAM_PORT;
	sockaddr.sin_addr.s_addr = *ipv4;
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns upstream next netls:%lu index:%lu ip:%s", 
					nelts, u->author_ip_curr_index, inet_ntoa(sockaddr.sin_addr));
	
	++u->author_ip_curr_index;
	c = u->connection;
	c->sockaddr = *((struct sockaddr *) &sockaddr);
	c->socklen = sizeof(struct sockaddr_in);
	/* lxl_dns_upstream_reinit_request() */
	lxl_dns_upstream_init_request(r);
}

lxl_int_t
lxl_dns_upstream_init_request(lxl_dns_request_t *r)
{
	ssize_t n;
	char *data;
	lxl_buf_t *b;
	lxl_connection_t *c;
	lxl_dns_upstream_t *u;

	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "%s dns upstream init request", r->rid);
	u = r->upstream;
	c = u->connection;
	b = c->buffer;

	memcpy(b->data, &u->header, 12);
	data = b->data + 12;
	memcpy(data, u->qname, u->qlen);
	data += u->qlen;
	memcpy(data, &u->question_part, 4);
	data += 4;
	u->request_n = b->len = data - b->data;

	n = c->send(c, b->data, b->len);
	if (n < 0) {
		lxl_log_error(LXL_LOG_ALERT, errno, "dns upstream request send failed");
		goto failed;
	}

	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns upstream request: qname:%s type:%04x send:%ld", u->qname, u->question_part.qtype, n);
	if (!c->read->active) {
		if (lxl_add_event(c->read, LXL_READ_EVENT, LXL_CLEAR_EVENT) == -1) {
		//if (lxl_add_event(c->read, LXL_READ_EVENT, LXL_LEVEL_EVENT) == -1) {
			goto failed;
		}
	}

	// nginx 3s, dig 5s duigui dayu 3s
	c->read->timedout = 0;
	lxl_add_timer(c->read, LXL_DNS_AUTHORITY_READ_TIMEOUT);

	return 0;

failed:
	lxl_dns_upstream_finalize_request(r, u, LXL_DNS_RCODE_SERVFAIL);
	return -1;
}

static void
lxl_dns_upstream_handler(lxl_event_t *rev)
{
	lxl_connection_t *c;
	lxl_dns_request_t *r;
	lxl_dns_upstream_t *u;

	c = rev->data;
	r = c->data;
	u = r->upstream;
	c = u->connection;
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "%s dns upstream handler request %s", r->rid, u->qname);

	/*if (rev->write) {
		lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns upstream write......");
	} else {
	u->read_event_handler(r, u);
	}*/
	u->read_event_handler(r, u);
}

static void 
lxl_dns_upstream_process_response(lxl_dns_request_t *r, lxl_dns_upstream_t *u) 
{
	uint16_t type, first_type, rdlength, flags, rcode, aa;	/*c0 0c pan duan diyige zijie */
	uint32_t ttl;
	lxl_int_t ret;
	lxl_uint_t nelts, type_flags;
	size_t n, zlen, nlen, rdlen, mnlen, rnlen;
	char *zname, *data, *data_end;
	char buffer_name[256], buffer_rdata[258];	
	static char buffer_mname[256], buffer_rname[256], buffer_soa[512];
	lxl_buf_t *b;
	lxl_list_t	*list;
	lxl_stack_t *stack;
	lxl_connection_t *c;
	lxl_dns_rr_t rr;
	lxl_dns_header_t *header;
	lxl_dns_zone_t  *zone;
	lxl_dns_rrset_t **p, *rrset;
	lxl_dns_rdata_t *rdata;
	lxl_dns_upstream_question_t *question;

	c = u->connection;
	b = c->buffer;
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "%s dns upstream process response: qname:%s type:0x%04x flags:0x%04x", 
					r->rid, u->qname, u->question_part.qtype, u->qname_flags);
	if (c->read->timedout) {
		lxl_log_error(LXL_LOG_INFO, 0, "upstream timed out");
		lxl_dns_upstream_next(r, u);		
		return;
	}

	n = c->recv(c, b->data, b->nalloc);
	if (n < 0) {
		lxl_log_error(LXL_LOG_ALERT, errno, "recv failed");
		goto failed;
	}

	b->len = n;
	//lxl_list_init(&list);
	data = b->data;
	data_end = b->data + b->len;

	header = (lxl_dns_header_t *) data;
	flags = ntohs(header->flags);
	aa = LXL_DNS_GET_AA(flags);
	rcode = LXL_DNS_GET_ROCDE(flags);
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "%s upstream response: an:0x%04x ns:0x%04x ad:0x%04x aa:0x%02x rcode:0x%02x",
					r->rid, header->ancount, header->nscount, header->arcount, aa, rcode);
	if (u->header.id != header->id) {
		lxl_log_error(LXL_LOG_INFO, 0, "upstream header id arre not same 0x%04x:0x%04x", u->header.id, header->id);
		return;
	}

	if (header->ancount == 0 && header->nscount == 0 && header->arcount == 0) {
		if (rcode == LXL_DNS_RCODE_OK /*&& r->question_part->qtype != LXL_DNS_SOA*/) {
			// dig www.so.com mx two case
			zone = lxl_dns_zone_find(r->qname, r->qlen);
			if (zone) {
				lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns add none rrset");
				rr.type = u->question_part.qtype;
				rr.ttl = 600;
				rr.nlen = u->qlen;
				rr.name = u->qname;
				rr.rdlength = 0;
				rr.rdata = NULL;
				rr.soa_nlen = 0;
				rr.soa_flags = LXL_DNS_RRSET_NONE_TYPE;
				rr.expires_sec = lxl_current_sec + 600;
				rr.update_flags = LXL_DNS_RR_NORMAL_TYPE;
				if (lxl_dns_rr_add(lxl_dns_pool, zone, &rr) == -1) {
					lxl_log_error(LXL_LOG_ERROR, 0, "dns add rr failed");
					goto failed;
				}
			}

			lxl_dns_upstream_finalize_request(r, u, LXL_DNS_RCODE_OK);
			return;
		} else if (rcode == LXL_DNS_RCODE_REFUSED) {
			lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns rcode refuse");
			goto upstream;
		} else {
			goto failed;
		}
	}

	data =  b->data + u->request_n;
	zlen = u->qlen;
	zname = u->qname;
	if (header->ancount == 0) {
		zlen = u->qlen;
		zname = u->qname;
		n = lxl_dns_upstream_parse_name(u, data, buffer_name, 256, &nlen);
		if (n == 0) {
			goto failed;
		}

		first_type = *((uint16_t *) (data + n));
		if (first_type != LXL_DNS_SOA) {
			zlen = nlen;
			zname = buffer_name;
		}
	}

	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "find zone zname:%s zlen:%lu", zname, zlen);
	zone = lxl_dns_zone_addfind(zname, zlen);
	if (zone == NULL) {
		goto failed;
	}

	do {
		type_flags = 1;
		n = lxl_dns_upstream_parse_name(u, data, buffer_name, 256, &nlen);
		if (n == 0) {	
			goto failed2;
		}

		data += n;
		type = *((uint16_t *) (data));
		ttl = ntohl(*((uint32_t *) (data + 4)));
		LXL_DNS_CHECK_TTL(ttl);
		rdlength = ntohs(*((uint16_t *) (data + 8)));
		data += 10;

		switch (type) {
			case LXL_DNS_TXT:
			case LXL_DNS_AAAA:
			case LXL_DNS_A:
				rdlen = rdlength;
				memcpy(buffer_rdata, data, rdlen);
				data += rdlength;

				break;

			case LXL_DNS_MX:
				memcpy(buffer_rdata, data, 2);
				if (lxl_dns_upstream_parse_rdata(u, data + 2, buffer_rdata + 2, 256, &rdlen) == 0) {
					goto failed2;
				}
		
				rdlen += 2;
				data += rdlength;

				break;

			case LXL_DNS_PTR:
			case LXL_DNS_NS:
			case LXL_DNS_CNAME:
				if (lxl_dns_upstream_parse_rdata(u, data, buffer_rdata, 256, &rdlen) == 0) {
					goto failed2;
				}
				
				data += rdlength;

				break;

			case LXL_DNS_SOA:
				n = lxl_dns_upstream_parse_name(u, data, buffer_mname, 256, &mnlen);	
				if (n == 0) {
					goto failed2;
				}

				data += n;
				n = lxl_dns_upstream_parse_name(u, data, buffer_rname, 256, &rnlen);	
				if (n == 0) {
					goto failed2;
				}
				
				if (mnlen + rnlen + 20 > 256) {	/* buffer_rdata -2 */
					lxl_log_error(LXL_LOG_INFO, 0, "soa rdata to long, mnlen %ld, rnlen %ld", mnlen, rnlen);
					goto failed2;
				}

				data += n;
				memcpy(buffer_rdata, buffer_mname, mnlen);
				rdlen = mnlen;
				memcpy(buffer_rdata + rdlen, buffer_rname, rnlen);
				rdlen += rnlen;
				memcpy(buffer_rdata + rdlen, data, 20);	/* soa define */
				rdlen += 20;
				data += 20;

				break;

			default:
				flags = 0;
				lxl_log_error(LXL_LOG_INFO, 0, "not spourt type:%04x, rdlength:%hu", type, rdlength);
				//rdlen = rdlength;
				//memcpy(buffer_rdata, data, rdlen);
				data += rdlength;
				lxl_dns_parse_package_to_hex(b->data, b->len);
				goto failed2;

				break;
		}
			
		if (type_flags) {
			rr.expires_sec = lxl_current_sec + ttl;
			rr.update_flags = LXL_DNS_RR_UPDATE_TYPE;
			if (type == LXL_DNS_SOA && header->ancount == 0) {
				lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "add soa_flgas rrset");
				memcpy(buffer_soa, buffer_name, nlen);
				memcpy(buffer_soa + nlen, buffer_rdata, rdlen);
				rr.type = u->question_part.qtype;
				rr.ttl = ttl;
				rr.nlen = zlen;
				rr.name = zname;
				rr.rdlength = nlen + rdlen;
				rr.rdata = buffer_soa;
				rr.soa_nlen = nlen;
				rr.soa_flags = LXL_DNS_RRSET_SOA_TYPE;
			} else {
				rr.type = type;
				rr.ttl = ttl;
				rr.nlen = nlen;
				rr.rdlength = rdlen;
				rr.name = buffer_name;
				rr.rdata = buffer_rdata;
				rr.soa_nlen = 0;
				rr.soa_flags = LXL_DNS_RRSET_NORMAL_TYPE;
			}
			if (lxl_dns_rr_add(lxl_dns_pool, zone, &rr) == -1) {
				lxl_log_error(LXL_LOG_ERROR, 0, "dns add rr failed");
				goto failed2;
			}
		}

		if (data >= data_end) {
#if 1
			lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "upstream package has %d not parse", data_end - data);
#endif
			break;
		}
	} while (data < data_end);


	lxl_dns_zone_update(zone);
	if (header->ancount != 0) {
		if (u->qname_flags & LXL_DNS_UPSTREAM_ROOT_QNAME) {
			rrset = lxl_dns_rrset_typefind(&zone->rrset_list, LXL_DNS_NS);
			if (rrset == NULL) {
				lxl_log_error(LXL_LOG_ERROR, 0, "root ns rrset is empty");
				goto failed;
			}

			// ? root ns ==> a  yiban douhui dai add ip
			if (lxl_dns_upstream_resolver_author_ip2(u, zone) == -1) {
				lxl_log_error(LXL_LOG_ERROR, 0, "root ns glue ip is empty");
				goto failed;
			}

			stack = lxl_stack_pop(&u->question_stack);
			if (stack == NULL) {
				lxl_log_error(LXL_LOG_ERROR, 0, "question stack pop empty");
				goto failed;
			}

			question = lxl_stack_data(stack, lxl_dns_upstream_question_t, stack);
			u->qlen = question->qlen;
			u->qname = question->qname;
			u->qname_flags = question->qname_flags;
			u->question_part = question->question_part;
#if 1
			lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "question stack pop: qname%s type:0x%04x flags:0x%04x", 
							u->qname, u->question_part.qtype, u->qname_flags);
#endif 

			goto upstream;
		}

		if ((u->qname_flags & LXL_DNS_UPSTREAM_NS_QNAME) && lxl_dns_upstream_resolver_author_ip2(u, zone) > 0) {
			stack = lxl_stack_pop(&u->question_stack);
			if (stack == NULL) {
				lxl_log_error(LXL_LOG_ERROR, 0, "question stack pop empty");
				goto failed;
			}

			question = lxl_stack_data(stack, lxl_dns_upstream_question_t, stack);
			u->qlen = question->qlen;
			u->qname = question->qname;
			u->qname_flags = question->qname_flags;
			u->question_part = question->question_part;
#if 1
			lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "question stack pop: qname%s type:0x%04x flags:0x%04x", 
							u->qname, u->question_part.qtype, u->qname_flags);
#endif 

			goto upstream;
		}

		//ret = lxl_dns_upstream_resolver_rrset(r, u, &new_zone);
		ret = lxl_dns_upstream_resolver_rrset(r, u, zone);
		if (ret == 1) {
			lxl_dns_package_answer(r);
			lxl_dns_package_header(r);
			b = r->connection->buffer;
			lxl_connection_t *c = r->connection;
			n = c->send(c, b->data, b->len);
			if (n < 0) {
				lxl_log_error(LXL_LOG_ALERT, errno, "sendto failed");
				return;
			}

			lxl_log_error(LXL_LOG_INFO, 0, "%s sendto success %ld", r->rid, n);
			lxl_close_connection(u->connection);
			lxl_dns_close_request(r);
			return;
		} else if (ret == 0) {
			if (lxl_array_empty(&r->answer_rrset_array)) {
				lxl_log_error(LXL_LOG_ERROR, 0, "answer rrset is empty");
				goto failed;
			}

			nelts = lxl_array_nelts(&r->answer_rrset_array);
			p = lxl_array_data(&r->answer_rrset_array, lxl_dns_rrset_t *, nelts - 1);
			rrset = *p;
			lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "answer rrset nelts:%lu last name:%s type:0x%04x", nelts, rrset->name, rrset->type);

			list = lxl_list_head(&rrset->rdata_list);
			rdata = lxl_list_data(list, lxl_dns_rdata_t, list);
			u->qname = rdata->rdata;
			u->qlen = rdata->rdlength;
			lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "qname chanage: qname:%s type:0x%04x flags:0x%04x", 
							u->qname, u->question_part.qtype, u->qname_flags);
			if (lxl_dns_upstream_resolver_author(r, u) == -1) {
				goto failed;
			}

			//lxl_dns_upstream_init_request(r);
			goto upstream;
		} else {
			lxl_log_error(LXL_LOG_ERROR, 0, "resolver rrset failed");
			goto failed;
		}
	} else {
        if (first_type == LXL_DNS_SOA) {
            lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "resolver soa in author answer");
            rrset = lxl_dns_rrset_find(&zone->rrset_list, zname, zlen, u->question_part.qtype, LXL_DNS_RRSET_EXPIRES_FIND);
            if (rrset) {
            	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "find soa");
				r->author_rrset = rrset;				

				goto succeed;
            } else {
				goto failed;
			}
        }

		if(lxl_dns_upstream_resolver_author2(r, u, zone) == -1) {
			goto failed;
		}

		goto upstream;
	}

failed:

	lxl_log_error(LXL_LOG_ERROR, 0, "%s dns upstream process response failed", r->rid);
	lxl_dns_upstream_finalize_request(r, u, LXL_DNS_RCODE_SERVFAIL);

	return;

failed2:
	
	lxl_log_error(LXL_LOG_ERROR, 0, "%s dns upstream process response failed2", r->rid);
	if (!lxl_list_empty(&zone->rrset_list)) {	
		lxl_dns_zone_update(zone);
	}

	lxl_dns_upstream_finalize_request(r, u, LXL_DNS_RCODE_SERVFAIL);

	return;

succeed:
	lxl_dns_upstream_finalize_request(r, u, LXL_DNS_RCODE_OK);

	return;

upstream:
	lxl_dns_upstream_next(r, u);

	return;
}

static void 
lxl_dns_upstream_finalize_request(lxl_dns_request_t *r, lxl_dns_upstream_t *u, lxl_uint_t rcode)
{
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "%s dns upstream finalize request qname %s", r->rid, u->qname);
	lxl_close_connection(u->connection);
	lxl_dns_finalize_request(r, rcode);
}

static size_t 
lxl_dns_upstream_parse_name(lxl_dns_upstream_t *u, char *data, char *buffer, size_t size, size_t *len)
{
	size_t n, nlen, response_n;
	uint16_t n_offset, h_offset;
	char *data_ptr;
	u_char label;
	lxl_buf_t *b;
	lxl_uint_t flags;
	
	n = nlen = 0;
	flags = 1;
	b = u->connection->buffer;
	response_n = b->len;
	data_ptr = data;
	do {
		n_offset = *((uint16_t *) data_ptr);
		while (LXL_DNS_IS_OFFSET_PTR(n_offset)) { 
	//		lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "offset 0x%04x", n_offset);
			LXL_DNS_GET_OFFSET(n_offset);
	//		lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "offset 0x%04x", n_offset);
			h_offset = ntohs(n_offset);
			if (h_offset > response_n) {
				lxl_log_error(LXL_LOG_ERROR, 0, "offset %hu > response length %lu", h_offset, response_n);
				return 0;
			}

			data_ptr = b->data + h_offset;
			n_offset = *((uint16_t *) data_ptr);
			if(flags) {
				n += 2;
				flags = 0;
			}
		} 

		// label xianzhi 64
		label = *((u_char *) data_ptr);
		if (label == '\0') {
			buffer[nlen] = '\0';
			*len = nlen + 1;
			if(flags) {
				n += 1;
			}

			lxl_strlower(buffer, *len);
			return n;
		}

		if (nlen + label >= size) {
			lxl_log_error(LXL_LOG_ERROR, 0, "name max is %lu, nlen %lu, label %lu", size, nlen, label);
			return 0;
		}

		memcpy(buffer + nlen, data_ptr, label + 1);
		nlen += (label + 1);
		data_ptr += (label + 1);
		if (flags) {
			n += (label + 1);
		}
	} while (label);

	return 0;
}

static size_t 
lxl_dns_upstream_parse_rdata(lxl_dns_upstream_t *u, char *data, char *buffer, size_t size, size_t *len)
{
	size_t nlen, response_n;	/* ji suan len ptr, len min is 2 (offset) */
	uint16_t n_offset, h_offset;
	char *data_ptr;
	u_char label;
	lxl_buf_t *b;
	
	nlen = 0;
	b = u->connection->buffer;
	response_n = b->len;
	data_ptr = data;
	do {
		n_offset = *((uint16_t *) data_ptr);
		while (LXL_DNS_IS_OFFSET_PTR(n_offset)) { 
	//		lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "offset 0x%04x", n_offset);
			LXL_DNS_GET_OFFSET(n_offset);
	//		lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "offset 0x%04x", n_offset);
			h_offset = ntohs(n_offset);
			if (h_offset > response_n) {
				lxl_log_error(LXL_LOG_ERROR, 0, "offset %hu > response length %lu", h_offset, response_n);

				return 0;
			}

			data_ptr = b->data + h_offset;
			n_offset = *((uint16_t *) data_ptr);
		} 

		// label xianzhi 64
		label = *((u_char *) data_ptr);
		if (label == '\0') {
			buffer[nlen] = '\0';
			*len = nlen + 1;
			lxl_strlower(buffer, *len);

			return 1;
		}

		if (nlen + label >= size) {
			lxl_log_error(LXL_LOG_ERROR, 0, "name max is %lu, nlen %lu, label %lu", size, nlen, label);

			return 0;
		}

		memcpy(buffer + nlen, data_ptr, label + 1);
		nlen += (label + 1);
		data_ptr += (label + 1);
	} while (label);

	return 0;
}

static lxl_int_t 
lxl_dns_upstream_resolver_rrset(lxl_dns_request_t *r, lxl_dns_upstream_t *u, lxl_dns_zone_t *zone)
{
	uint16_t nlen, type;
	char *name;
	lxl_list_t *list;
	lxl_dns_rdata_t *rdata;
	lxl_dns_rrset_t **p, *rrset;

	name = u->qname;
	nlen = u->qlen;
	type = r->question_part->qtype;
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "%s dns upstream resolver: name:%s type:0x%04x", r->rid, name, type);
	switch (type) {
		case LXL_DNS_CNAME: 
			rrset = lxl_dns_rrset_find(&zone->rrset_list, (char *) name, nlen, type, LXL_DNS_RRSET_EXPIRES_FIND);
			if (rrset == NULL)  {
				return 0;
			}

			p = lxl_array_push(&r->answer_rrset_array);
			if (p == NULL) {
				return -1;
			}

			*p = rrset;

			return 1;
			break;

		case LXL_DNS_SOA:
			do {
				rrset = lxl_dns_rrset_dnsfind(&zone->rrset_list, (char *) name, nlen, type);
				if (rrset == NULL) {
					break;
				}

				if (rrset->type == type) {
					p = lxl_array_push(&r->answer_rrset_array);
					if (p == NULL) {
						return -1;
					}

					*p = rrset;

					return 1;
				}

				list = lxl_list_head(&rrset->rdata_list);
				rdata = lxl_list_data(list, lxl_dns_rdata_t, list);
				if (nlen == rdata->rdlength && memcmp(name, rdata->rdata, nlen) == 0) {
					lxl_log_debug(LXL_LOG_DEBUG, 0, "answer rrset same: name:%s rdata:%s type:%04x", name, rdata->rdata, rrset->type);
					break;
				}

				p = lxl_array_push(&r->answer_rrset_array);
				if (p == NULL) {
					return -1;
				}

				*p = rrset;
				name = rdata->rdata;
				nlen = rdata->rdlength;
			} while (1);

			/* only author has cname and soa */
			rrset = lxl_dns_rrset_typefind(&zone->rrset_list, type);
			if (rrset) {
				r->author_rrset = rrset;
				lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "upstream find soa");

				return 1;
			}

			return 0;
			break;

		default:
			do {
				rrset = lxl_dns_rrset_dnsfind(&zone->rrset_list, (char *) name, nlen, type);
				if (rrset == NULL) {
					break;
				}
			
				if (rrset->type == type) {
					p = lxl_array_push(&r->answer_rrset_array);
					if (p == NULL) {
						return -1;
					}

					*p = rrset;

					return 1;
				}

				list = lxl_list_head(&rrset->rdata_list);
				rdata = lxl_list_data(list, lxl_dns_rdata_t, list);
				if (nlen == rdata->rdlength && memcmp(name, rdata->rdata, nlen) == 0) {
					lxl_log_debug(LXL_LOG_DEBUG, 0, "answer rrset same: name:%s rdata:%s type:%04x", name, rdata->rdata, rrset->type);
					break;
				}

				p = lxl_array_push(&r->answer_rrset_array);
				if (p == NULL) {
					return -1;
				}

				*p = rrset;
				name = rdata->rdata;
				nlen = rdata->rdlength;
			} while (1);

			return 0;
			break;
	}

	return 0;
}

lxl_int_t 
lxl_dns_upstream_resolver_author(lxl_dns_request_t *r, lxl_dns_upstream_t *u)
{
	char label, *aname;
	lxl_uint_t alen;
	lxl_stack_t *stack;
	lxl_dns_zone_t *zone;
	lxl_dns_rrset_t *rrset;
	lxl_dns_rdata_t *rdata;
	lxl_dns_upstream_question_t *question;


	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "%s dns upstream resolver author: qname:%s type:%04x flags:%04x", 
					r->rid, u->qname, u->question_part.qtype, u->qname_flags);
	aname = u->qname;
	alen = u->qlen - (aname - u->qname);
	do {
		zone = lxl_hash_find(&lxl_dns_hash, aname, alen);
		if (zone) {
			rrset = lxl_dns_rrset_find(&zone->rrset_list, aname, alen, LXL_DNS_NS, LXL_DNS_RRSET_NORMAL_FIND);
			if (rrset) {
				lxl_log_debug(LXL_LOG_DEBUG, 0, "rrset expires %lds", lxl_current_sec - rrset->expires_sec);
				if (rrset->expires_sec > lxl_current_sec) {
					if (lxl_dns_upstream_resolver_author_ip(u, zone, rrset, LXL_DNS_RRSET_EXPIRES_FIND) > 0) {
						lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "resolver author: name:%s ip nelts:%lu", 
										aname, lxl_array_nelts(&u->author_ip_array));
						if (u->qname_flags & LXL_DNS_UPSTREAM_NS_QNAME) {
							rdata = lxl_dns_rdata_find(&rrset->rdata_list, u->qname, u->qlen);
							if (rdata) {
								stack = lxl_stack_top(&u->question_stack);
								if (stack == NULL) {
									lxl_log_error(LXL_LOG_ERROR, 0, "question stack top empty");
									return -1;
								}

								question = lxl_stack_data(stack, lxl_dns_upstream_question_t, stack);
								u->qname = question->qname;
								u->qlen = question->qlen;
								u->qname_flags = question->qname_flags;
								u->question_part = question->question_part;
								lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "question chanage: qname:%s teyp:%04x flags:%04x", 
												u->qname, u->question_part.qtype, u->qname_flags);
							}
						}

						return 0;
					}
				} else {
					if (alen == LXL_DNS_ROOT_LEN && memcmp(aname, LXL_DNS_ROOT_LABEL, alen) == 0) {
						lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns upstream root ns expires");
						if (lxl_dns_upstream_resolver_author_ip(u, zone, rrset, LXL_DNS_RRSET_NORMAL_FIND) > 0) {
							lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns upstream resolver author name:%s auhtor ip nelts:%lu", 
								r->rid, aname, lxl_array_nelts(&u->author_ip_array));

							question = lxl_palloc(r->pool, sizeof(lxl_dns_upstream_question_t));
							if (question == NULL) {
								return -1;
							}

#if 1
							lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "question stack push: qname:%s type:%04x flags:%04x",
											u->qname, u->question_part.qtype, u->qname_flags);
#endif
							question->qname = u->qname;
							question->qlen = u->qlen;
							question->qname_flags = u->qname_flags;
							question->question_part = u->question_part;
							lxl_stack_push(&u->question_stack, &question->stack);
							u->qname = aname;
							u->qlen = alen;
							u->question_part.qtype = LXL_DNS_NS;
							u->question_part.qclass = LXL_DNS_IN;
							u->qname_flags |= LXL_DNS_UPSTREAM_ROOT_QNAME;
							lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "question change: qname:. type:%04x flags:%04x", 
											u->question_part.qtype, u->qname_flags);

							return 0;
						}
					}
				} 
			}
		}

		label = *(aname);
		aname += (label + 1);
		alen -= (label + 1);
	} while (label);
	
	lxl_log_error(LXL_LOG_WARN, 0, "resolver author failed", r->rid);

	return -1;
}

static lxl_int_t 
lxl_dns_upstream_resolver_author2(lxl_dns_request_t *r, lxl_dns_upstream_t *u, lxl_dns_zone_t *zone)
{
	lxl_stack_t *stack;
	lxl_list_t *list, *head;
	lxl_dns_rrset_t *rrset;
	lxl_dns_rdata_t *rdata, *ns_rdata;
	lxl_dns_upstream_question_t *question;

	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "%s dns upstream resolver author2: qname:%s type:%04x, flags:%04x", 
					r->rid, u->qname, u->question_part.qtype, u->qname_flags);
	rrset = lxl_dns_rrset_typefind(&zone->rrset_list, LXL_DNS_NS);
	if (rrset) {
		lxl_log_debug(LXL_LOG_DEBUG, 0, "resolver2 find ns rrset in answer zone");
		if (u->qname_flags & LXL_DNS_UPSTREAM_NS_QNAME) {
			head = &(u->ns_rrset->rdata_list);
			for (list = lxl_list_head(head); list != lxl_list_sentinel(head); list = lxl_list_next(list)) {
				ns_rdata = lxl_list_data(list, lxl_dns_rdata_t, list);
				rdata = lxl_dns_rdata_find(&rrset->rdata_list, (char *) &ns_rdata->rdata, ns_rdata->rdlength);
				lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns upstream  find ns rdata %s", ns_rdata->rdata);
				if (rdata) {
					if (lxl_dns_upstream_resolver_author_ip(u, zone, rrset, LXL_DNS_RRSET_EXPIRES_FIND) > 0) {
						stack = lxl_stack_pop(&u->question_stack);
						if (stack == NULL) {
							lxl_log_error(LXL_LOG_ERROR, 0, "question stack pop empty");
							return -1;
						}

						question = lxl_stack_data(stack, lxl_dns_upstream_question_t, stack);
						u->qname = question->qname;
						u->qlen = question->qlen;
						u->qname_flags = question->qname_flags;
						u->question_part = question->question_part;
						lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "question chanage: qname:%s type:%04x flags:%04x", 
										u->qname, u->question_part.qtype, u->qname_flags);
						return 0;
					}
				}
			}
		}

		if (lxl_dns_upstream_resolver_author_ip(u, zone, rrset, LXL_DNS_RRSET_EXPIRES_FIND) > 0) {
			return 0;
		}
		
		list = lxl_list_head(&rrset->rdata_list);
		rdata = lxl_list_data(list, lxl_dns_rdata_t, list);
	/*	if (u->qname_flags & LXL_DNS_UPSTREAM_NS_QNAME) {
			//lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "LXL_DNS_UPSTREAM_NS_QNAME flags 0x%04x", u->qname_flags);
			u->qname = rdata->rdata;
			u->qlen = rdata->rdlength;
		} else { */
			question = lxl_palloc(r->pool, sizeof(lxl_dns_upstream_question_t));
			if (question == NULL) {
				lxl_log_error(LXL_LOG_ERROR, 0, "palloc question failed");
				return -1;
			}

#if 1
			lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "question stack push: qname:%s type:%04x flags:%04x", 
					u->qname, u->question_part.qtype, u->qname_flags);
#endif
			question->qname = u->qname;
			question->qlen = u->qlen;
			question->qname_flags = u->qname_flags;
			question->question_part = u->question_part;
			lxl_stack_push(&u->question_stack, &question->stack);
			u->qname = rdata->rdata;
			u->qlen = rdata->rdlength;
			u->question_part.qtype = LXL_DNS_A;
			u->qname_flags = LXL_DNS_UPSTREAM_NS_QNAME;
		//}
		lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "question chanage: qname:%s type:0x%04x flags 0x%04x", u->qname, u->question_part.qtype, u->qname_flags);
		u->ns_rrset = rrset;
	}

	return lxl_dns_upstream_resolver_author(r, u);
}

static lxl_int_t 
lxl_dns_upstream_resolver_author_ip(lxl_dns_upstream_t *u, lxl_dns_zone_t *zone, lxl_dns_rrset_t *ns_rrset, uint16_t flags) 
{
	uint32_t *ipv4;
	lxl_uint_t nelts;
	lxl_list_t *a_rdata_list, *ns_rdata_list;
	lxl_dns_rrset_t *a_rrset;
	lxl_dns_rdata_t *a_rdata, *ns_rdata;

	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns upstream resolver ip");
	u->author_ip_curr_index = 0;
	lxl_array_clear(&u->author_ip_array);
	for (ns_rdata_list = lxl_list_head(&ns_rrset->rdata_list); 
			ns_rdata_list != lxl_list_sentinel(&ns_rrset->rdata_list); ns_rdata_list = lxl_list_next(ns_rdata_list)) {
		ns_rdata = lxl_list_data(ns_rdata_list, lxl_dns_rdata_t, list);
		a_rrset = lxl_dns_rrset_find(&zone->rrset_list, ns_rdata->rdata, ns_rdata->rdlength, LXL_DNS_A, flags);
		if (a_rrset) {
			for (a_rdata_list = lxl_list_head(&a_rrset->rdata_list); 
					a_rdata_list != lxl_list_sentinel(&a_rrset->rdata_list); a_rdata_list = lxl_list_next(a_rdata_list)) {

				a_rdata = lxl_list_data(a_rdata_list, lxl_dns_rdata_t, list);
				ipv4 = lxl_array_push(&u->author_ip_array);
				if (ipv4 == NULL) {
					lxl_log_error(LXL_LOG_ERROR, 0, "array push author ip failed");
					return -1;
				}

				memcpy(ipv4, &a_rdata->rdata, sizeof(uint32_t));
			}
		}
	}

	nelts = lxl_array_nelts(&u->author_ip_array);
#if 1
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "resolver upstream author ip nelts %lu", nelts);
#endif 

	return (lxl_int_t) nelts;
}

static lxl_int_t 
lxl_dns_upstream_resolver_author_ip2(lxl_dns_upstream_t *u, lxl_dns_zone_t *zone)
{
	uint32_t *ipv4;
	lxl_uint_t nelts;
	lxl_list_t *list;
	lxl_dns_rrset_t *rrset;
	lxl_dns_rdata_t *rdata;

	u->author_ip_curr_index = 0;
	lxl_array_clear(&u->author_ip_array);
	rrset = lxl_dns_rrset_find(&zone->rrset_list, u->qname, u->qlen, LXL_DNS_A, LXL_DNS_RRSET_EXPIRES_FIND);
	if (rrset) {
		for (list = lxl_list_head(&rrset->rdata_list); list != lxl_list_sentinel(&rrset->rdata_list); list = lxl_list_next(list)) {
			rdata = lxl_list_data(list, lxl_dns_rdata_t, list);
			ipv4 = lxl_array_push(&u->author_ip_array);
			if (ipv4 == NULL) {
				return -1;
			}

			memcpy(ipv4, &rdata->rdata, sizeof(uint32_t));
		}
	}

	nelts = lxl_array_nelts(&u->author_ip_array);
#if 1
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "resolver upstream author ip2 nelts %lu", nelts);
#endif 

	return (lxl_int_t) nelts;
}
