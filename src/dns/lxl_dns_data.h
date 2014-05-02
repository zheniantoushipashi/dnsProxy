
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_DNS_DATA_H_INCLUDE
#define LXL_DNS_DATA_H_INCLUDE


#include <lxl_config.h>
#include <lxl_list.h>


#define LXL_DNS_A				0x0100
#define LXL_DNS_NS				0x0200
#define LXL_DNS_CNAME			0x0500
#define LXL_DNS_SOA				0x0600
#define LXL_DNS_PTR				0x0c00		/* 12 */
#define LXL_DNS_MX				0x0f00		/* 15 */
#define LXL_DNS_TXT				0x1000		/* 16 */
#define LXL_DNS_AAAA			0x1c00		/* 28 */
#define LXL_DNS_ANY				0xff00		
#define LXL_DNS_INVALID_TYPE	0x0000

#define LXL_DNS_STR_A			"A"
#define LXL_DNS_STR_NS			"NS"
#define LXL_DNS_STR_CNAME		"CNAME"
#define LXL_DNS_STR_SOA			"SOA"
#define LXL_DNS_STR_PTR			"PTR"
#define LXL_DNS_STR_MX			"MX"
#define LXL_DNS_STR_TXT			"TXT"
#define LXL_DNS_STR_AAAA		"AAAA"
#define LXL_DNS_STR_INVALID_TYPE "INVALID TYPE"	

#define LXL_DNS_STR_A_LEN		1
#define LXL_DNS_STR_NS_LEN		2
#define LXL_DNS_STR_CNAME_LEN	5
#define LXL_DNS_STR_SOA_LEN		3
#define LXL_DNS_STR_PTR_LEN		3
#define LXL_DNS_STR_MX_LEN		2
#define LXL_DNS_STR_TXT_LEN		3
#define LXL_DNS_STR_AAAA_LEN	4

#define LXL_DNS_MIN_TTL			60
#define LXL_DNS_MAX_TTL			604800	/* 7 day */
//#define LXL_DNS_MAX_TTL			120

#define LXL_DNS_ROOT_DOT		"."
#define LXL_DNS_ROOT_LABEL		"\0"
#define LXL_DNS_ROOT_LEN		1

#define LXL_DNS_RR_NORMAL_TYPE		0x0000
#define LXL_DNS_RR_UPDATE_TYPE		0x0001

#define LXL_DNS_RRSET_NORMAL_FIND	0x0000
#define LXL_DNS_RRSET_EXPIRES_FIND	0x0001

#define LXL_DNS_RRSET_NORMAL_TYPE	0x0001
#define LXL_DNS_RRSET_SOA_TYPE		0x0002
#define LXL_DNS_RRSET_NONE_TYPE		0x0004


#define LXL_DNS_CHECK_TTL(ttl)										\
	ttl = (ttl) < (LXL_DNS_MIN_TTL) ? (LXL_DNS_MIN_TTL) : (ttl);	\
	ttl = (ttl) > (LXL_DNS_MAX_TTL) ? (LXL_DNS_MAX_TTL) : (ttl) 
	


typedef struct {
	lxl_list_t 	list;

	uint16_t 		rdlength;
	uint16_t 		update_flags;
	char 	 		rdata[0];
} lxl_dns_rdata_t;

typedef struct {
	lxl_uint_t 		expires_sec;
	uint16_t 		update_flags;
	uint16_t		soa_flags;
	uint16_t 		soa_nlen;

	uint16_t 		nlen;
	uint16_t 		rdlength;
	uint16_t 		type;
	uint32_t 		ttl;
	char	 		*name;
	char 	 		*rdata;
} lxl_dns_rr_t;

typedef struct {
	uint16_t 		mlen;
	uint16_t 		rlen;
	uint16_t 		nlen;
	uint32_t 		ttl;
	/*uint32_t 		serial;
	uint32_t 		refresh;
	uint32_t 		retry;
	uint32_t 		expire;
	uint32_t 		minmum;*/
	char 			*mname;
	char 			*rname;
	char 			soa_part[20];
	char			name[0];
} lxl_dns_soa_t;

typedef struct {
	lxl_list_t 		list;
	lxl_list_t 		rdata_list;

	lxl_uint_t 		expires_sec;
	uint16_t 		update_flags;
	uint16_t		soa_flags;
	uint16_t 		soa_nlen;

	uint16_t 		nlen;
	uint16_t 		type;			/* network byte order */
	uint32_t 		ttl;			/* host byte order */
	char 			name[0];
} lxl_dns_rrset_t;

typedef struct {
//	lxl_dns_soa_t 	*soa;
	lxl_uint_t 		update_sec;
	lxl_list_t 		rrset_list;
	//uint16_t zlen;
	//char zname[1];
} lxl_dns_zone_t;

//extern lxl_hash_t  lxl_dns_hash;


void 	lxl_dns_data_dump(void);
void 	lxl_dns_data_rebuild(void);

#define lxl_dns_zone_find(zname, zlen)				\
	lxl_hash_find(&lxl_dns_hash, zname, zlen)

#define lxl_dns_zone_add(zname, zlen, zone)			\
	lxl_hash_add(&lxl_dns_hash, zname, zlen, zone)

/* use pool, not free zone */
#define lxl_dns_zone_del(zname, zlen, zone)			\
	lxl_hash_del(&lxl_dns_hash, zname, zlen)

lxl_dns_zone_t *lxl_dns_zone_addfind(char *zname, uint16_t zlen);
void			lxl_dns_zone_update(lxl_dns_zone_t *zone);

lxl_int_t 		lxl_dns_rr_add(lxl_pool_t *pool, lxl_dns_zone_t *zone, lxl_dns_rr_t *rr);

lxl_dns_rrset_t *lxl_dns_rrset_create(char *name, uint16_t len, uint32_t ttl, lxl_dns_rdata_t *rata);
lxl_dns_rrset_t *lxl_dns_rrset_find(lxl_list_t *head, char *name, uint16_t len, uint16_t type, uint16_t flags);
lxl_dns_rrset_t *lxl_dns_rrset_dnsfind(lxl_list_t *head, char *name, uint16_t len, uint16_t type);
lxl_dns_rrset_t *lxl_dns_rrset_typefind(lxl_list_t *head, uint16_t type);
lxl_dns_rrset_t *lxl_dns_rrset_soafind(lxl_list_t *head);

lxl_dns_rdata_t *lxl_dns_rdata_create(char *data, size_t len, uint16_t type);
/* not support MX typefind */
lxl_dns_rdata_t *lxl_dns_rdata_find(lxl_list_t *head, char *rdata, uint16_t rdlength);
lxl_dns_rdata_t *lxl_dns_rdata_typefind(lxl_list_t *head, char *rdata, uint16_t rdlenght, uint16_t type);

lxl_int_t 		lxl_dns_label_to_dot(char *domain_label, char *domain_dot);
uint16_t 		lxl_dns_get_rr_type(char *type, size_t len);
char 		   *lxl_dns_get_rr_strtype(uint16_t type);


#endif	/* LXL_DNS_DATA_H_INCLUDE */
