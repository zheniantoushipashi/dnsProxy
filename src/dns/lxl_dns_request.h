
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_DNS_REQUEST_H_INCLUDE
#define LXL_DNS_REQUEST_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dns.h>
#include <lxl_dns_data.h>


#define LXL_DNS_IN			0x0100

//#define LXL_DNS_MIN_TTL 	60
//#define LXL_DNS_MAX_TTL 	604800		

#define LXL_DNS_TC			0x0001

#define LXL_DNS_RCODE_OK		0
#define LXL_DNS_RCODE_FORMERR	1
#define LXL_DNS_RCODE_SERVFAIL	2
#define LXL_DNS_RCODE_NAMEERR	3
#define LXL_DNS_RCODE_NOTIMP	4 
#define LXL_DNS_RCODE_REFUSED	5

#define LXL_DNS_GET_AA(flags)			(flags & 0x0400) >> 10
#define LXL_DNS_GET_RD(flags)			(flags & 0x0100) >> 8
#define LXL_DNS_GET_ROCDE(flags)		(flags & 0x000f)

#define LXL_DNS_SET_QR_R(flags)     	(flags |= 0x8000)
#define LXL_DNS_SET_TC(flags)			(flags |= 0x0200)
#define LXL_DNS_SET_RA(flags)			(flags |= 0x0080)
#define LXL_DNS_SET_RD(flags)			(flags |= 0x0100)
#define LXL_DNS_SET_RCODE(flags, err)   (flags = ((flags & 0xfff0) + err))	


typedef void (*lxl_dns_event_handler_pt) (lxl_dns_request_t *r);

#pragma pack (2)	// (1)
typedef struct {
	uint16_t id;
	uint16_t flags;
	uint16_t qdcount;
	uint16_t ancount;
	uint16_t nscount;
	uint16_t arcount;
} lxl_dns_header_t;
#pragma pack ()

#pragma pack (2) 
typedef struct {
	uint16_t qtype;
	uint16_t qclass;
} lxl_dns_question_part_t ;
#pragma pack ()

#pragma pack (2) 
typedef struct {
	uint16_t type;
	uint16_t class;
	uint32_t ttl;
	uint16_t rdlength;
} lxl_dns_rr_part_t;
#pragma pack ()

struct lxl_dns_request_s {
	lxl_connection_t 			*connection;
	lxl_pool_t 					*pool;
	lxl_dns_event_handler_pt 	read_event_handler;
	lxl_dns_event_handler_pt 	write_event_handler;
	lxl_dns_upstream_t			*upstream;

	/* dns for tcp */
	uint16_t					request_n;
	uint16_t 					qlen;
	uint16_t 					rcode;
	uint16_t 					tc;
	uint16_t 					ancount;
	uint16_t 					nscount;
	uint16_t 					arcount;
	char 						rid[16];
	char 						*qname;
	lxl_dns_header_t 			*header;
	lxl_dns_question_part_t  	*question_part;

	char						*pos;
	char 						*end;

	//lxl_dns_zone_t 				*zone;
	lxl_dns_rrset_t 			*author_rrset;
	lxl_array_t 				answer_rrset_array;
	lxl_array_t 				addition_rrset_array;
};

void lxl_dns_finalize_request(lxl_dns_request_t *r, lxl_uint_t rcode);
void lxl_dns_close_request(lxl_dns_request_t *r);
void lxl_dns_free_request(lxl_dns_request_t *r);
void lxl_dns_close_connection(lxl_connection_t *c);

//lxl_int_t lxl_dns_loop_zone(lxl_dns_request_t *r, lxl_dns_zone_t *zone, char *name, uint16_t nlen, uint16_t type);

#endif	/* LXL_DNS_REQUEST_H_INCLUDE */
