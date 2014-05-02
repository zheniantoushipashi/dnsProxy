
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_DNS_UPSTREAM_H_INCLUDE
#define LXL_DNS_UPSTREAM_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_array.h>
#include <lxl_stack.h>
#include <lxl_dns.h>
#include <lxl_dns_request.h>


//#define 	LXL_DNS_UPSTREAM_FT_OK			0x00000001
//#define 	LXL_DNS_UPSTREAM_FT_TIMEOUT		0x00000002

//#define 	LXL_DNS_UPSTREAM_RAW_QNAME 		0x00000001
//#define 	LXL_DNS_UPSTREAM_CNAME_QNAME 	0x00000002
#define 	LXL_DNS_UPSTREAM_NORMAL_QNAME	0x00000000
#define 	LXL_DNS_UPSTREAM_NS_QNAME 		0x00000004
#define 	LXL_DNS_UPSTREAM_ROOT_QNAME		0x00000008


typedef struct {
	lxl_stack_t 				stack;
	lxl_uint_t 					qname_flags;
	lxl_dns_question_part_t 	question_part;
	lxl_uint_t 					qlen;
	char 						*qname;
} lxl_dns_upstream_question_t;

typedef void (*lxl_dns_upstream_handler_pt) (lxl_dns_request_t *r, lxl_dns_upstream_t *up);

struct lxl_dns_upstream_s {
	lxl_dns_upstream_handler_pt	read_event_handler;
	lxl_dns_upstream_handler_pt write_event_handler;

	lxl_connection_t 		*connection;

	//char data[256];	// 255+1
	size_t 					request_n;
	size_t  				response_n;
	lxl_dns_header_t 		header;
	lxl_dns_question_part_t question_part;

	lxl_uint_t 				qname_flags;	/* cname ns */
	char 					*qname;
	char 					*qcname;
	lxl_uint_t 				qlen;
	lxl_uint_t 				qclen;
	lxl_stack_t 			question_stack;
	
	lxl_dns_rrset_t 		*ns_rrset;
	/*char 					*author_name;
	lxl_uint_t				author_name_len;
	uint16_t 				alen;
	char 			 		*aname_curr;
	lxl_array_t 	 		author_rrset_array;
	lxl_uint_t				aname_curr_index;
	lxl_array_t 			aname_array;*/
	lxl_uint_t				author_ip_curr_index;
	lxl_array_t 	 		author_ip_array;
};


lxl_int_t lxl_dns_upstream_init(lxl_dns_request_t *r, char *qname, lxl_uint_t qlen);
lxl_int_t lxl_dns_upstream_init_request(lxl_dns_request_t *r);
lxl_int_t lxl_dns_upstream_resolver_author(lxl_dns_request_t *r, lxl_dns_upstream_t *u);
void 	  lxl_dns_upstream_next(lxl_dns_request_t *r, lxl_dns_upstream_t *u);


#endif	/* LXL_DNS_UPSTREAM_H_INCLUDE */
