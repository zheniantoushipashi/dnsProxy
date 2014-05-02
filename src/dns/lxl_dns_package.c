
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_log.h>
#include <lxl_array.h>
#include <lxl_dns.h>
#include <lxl_dns_data.h>
#include <lxl_dns_request.h>


extern lxl_uint_t lxl_current_sec;


static lxl_int_t lxl_dns_package_rrset(lxl_dns_request_t *r, lxl_dns_rrset_t *rrset, lxl_uint_t *count);
static lxl_int_t lxl_dns_package_name(lxl_dns_request_t *r, char *name, uint16_t nlen);
static void 	 lxl_dns_package_soa(lxl_dns_request_t *r, lxl_dns_rrset_t *rrset);


lxl_int_t
lxl_dns_package_answer(lxl_dns_request_t *r)
{
	uint16_t flags;
	lxl_buf_t *b;
	lxl_uint_t i, count, nelts;
	lxl_dns_rrset_t **p;
	
	nelts = lxl_array_nelts(&r->answer_rrset_array);
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns package answer rrset %lu", nelts);
	b = r->connection->buffer;
	r->pos = (char *) r->question_part + sizeof(lxl_dns_question_part_t);
	r->end = (char *) b->data + b->nalloc;
	for (i = 0; i < nelts; ++i) {
		p = lxl_array_data(&r->answer_rrset_array, lxl_dns_rrset_t *, i);
		flags = (*p)->soa_flags;
		switch (flags) {
			case LXL_DNS_RRSET_NORMAL_TYPE:
				lxl_dns_package_rrset(r, *p, &count);
				r->ancount += count;
				if (r->tc & LXL_DNS_TC) {
					goto succeed;
				}

				break;

			case LXL_DNS_RRSET_SOA_TYPE:
				lxl_dns_package_soa(r, *p);
				if (r->tc & LXL_DNS_TC) {
					goto succeed;
				}

				r->nscount = 1;
				goto succeed;

				break;

			default:		/* LXL_DNS_RRSET_NONE_TYPE */
				lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns package none rrset");
				goto succeed;

				break;
		}
	}

	if (r->author_rrset != NULL) {
		lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns package author rrset");
		lxl_dns_package_soa(r, r->author_rrset);
		if (r->tc & LXL_DNS_TC) {
			goto succeed;
		}

		r->nscount = 1;
	}

	goto succeed;

succeed:

	//lxl_dns_package_header(r);
	b->len = r->pos - b->data;
	/* tcp */
	if (r->request_n) {
		*((uint16_t *) b->data) = htons(b->len - 2);
	}

	return 0;

}

void 
lxl_dns_package_header(lxl_dns_request_t *r)
{
	uint16_t flags;

	/* recurse dns  Z is 2 
	flags = ntohs(r->header->flags); */
	flags = 0;
	LXL_DNS_SET_QR_R(flags);
	LXL_DNS_SET_RD(flags);
	LXL_DNS_SET_RA(flags);
	LXL_DNS_SET_RCODE(flags, r->rcode);
	/* has not tcp */
	/*if (r->tc & LXL_DNS_TC) {
		LXL_DNS_SET_TC(flags); 
	}*/
	r->header->flags = htons(flags);
	r->header->ancount = htons(r->ancount);
	r->header->nscount = htons(r->nscount);
	r->header->arcount = htons(r->arcount);
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns packet response header: flags:0x%04x ancount:%hu nscount:%hu arcount:%hu", flags, r->ancount, r->nscount,r->arcount);
}

lxl_int_t
lxl_dns_package_request(lxl_dns_request_t *r)
{
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns package request");

	return 0;
}

static lxl_int_t 
lxl_dns_package_rrset(lxl_dns_request_t *r, lxl_dns_rrset_t *rrset, lxl_uint_t *count)
{
	size_t size;
	lxl_list_t *list;
	lxl_dns_rdata_t *rdata;
	lxl_dns_rr_part_t part;
	
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns package rrset");
	size = sizeof(lxl_dns_rr_part_t);
	*count = 0;
	part.type = rrset->type;
	part.class = LXL_DNS_IN;
	part.ttl = htonl(rrset->expires_sec - lxl_current_sec);
	for (list = lxl_list_head(&rrset->rdata_list); list != lxl_list_sentinel(&rrset->rdata_list); list = lxl_list_next(list)) {
		rdata = lxl_list_data(list, lxl_dns_rdata_t, list);
		if ((r->end - r->pos) < (rrset->nlen + size + rdata->rdlength)) {
			r->tc = LXL_DNS_TC;
			lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns package tc");

			return 0;
		}

		lxl_dns_package_name(r, rrset->name, rrset->nlen);
		part.rdlength =  htons(rdata->rdlength);
		memcpy(r->pos, &part, size);
		r->pos += size;
		memcpy(r->pos, &rdata->rdata, rdata->rdlength);
		r->pos +=  rdata->rdlength;

		++(*count);
	}

	return 0;
}

static lxl_int_t
lxl_dns_package_name(lxl_dns_request_t *r, char *name, uint16_t nlen)
{
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns package name %s", name);
	if (r->end - r->pos < nlen) {
		r->tc = LXL_DNS_TC;
		return 0;
	}

	memcpy(r->pos, name, nlen);
	r->pos += nlen;

	return 0;
}

static void
lxl_dns_package_soa(lxl_dns_request_t *r, lxl_dns_rrset_t *rrset)
{
	size_t size;
	uint16_t rdlength;
	lxl_list_t *list;
	lxl_dns_rdata_t *rdata;
	lxl_dns_rr_part_t part;

	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns package soa");
	size = sizeof(lxl_dns_rr_part_t);
	part.type = LXL_DNS_SOA;
	part.class = LXL_DNS_IN;
	part.ttl = htonl(rrset->expires_sec - lxl_current_sec);
	list = lxl_list_head(&rrset->rdata_list);
	rdata = lxl_list_data(list, lxl_dns_rdata_t, list);
	if (rrset->soa_flags) {
		if ((r->end - r->pos) < (rdata->rdlength + size)) {
			r->tc = LXL_DNS_TC;
			lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns package tc");
			return;
		}

		rdlength = rdata->rdlength - rrset->soa_nlen;
		part.rdlength = htons(rdlength);
		memcpy(r->pos, rdata->rdata, rrset->soa_nlen);
		r->pos += rrset->soa_nlen;
		memcpy(r->pos, &part, size);
		r->pos += size;
		memcpy(r->pos, rdata->rdata + rrset->soa_nlen, rdlength);
		r->pos += rdlength;
	} else {
		if ((r->end - r->pos) < (rrset->nlen + rdata->rdlength + size)) {
			r->tc = LXL_DNS_TC;
			lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns package tc");
			return;
		}
		
		part.rdlength = htons(rdata->rdlength);
		memcpy(r->pos, rrset->name, rrset->nlen);
		r->pos += rrset->nlen;
		memcpy(r->pos, &part, size);
		r->pos += size;
		memcpy(r->pos, rdata->rdata, rdata->rdlength);
		r->pos += rdata->rdlength;
	}
}
