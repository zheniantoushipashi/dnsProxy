
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_log.h>
#include <lxl_times.h>
#include <lxl_string.h>
#include <lxl_dns.h>
#include <lxl_dns_request.h>


#define LXL_DNS_LABLE_MAXSIZE		63
#define LXL_DNS_NAME_MAXSIZE		255
#define LXL_DNS_UDP_MSG_MINSIZE		17
#define LXL_DNS_UDP_MSG_MAXSIZE		512


extern volatile lxl_uint_t lxl_current_msec;
extern volatile lxl_uint_t lxl_current_sec;


lxl_int_t
lxl_dns_domain_dot_to_label(char *domain, size_t len)
{
	/* last must . and tolower */
	size_t i, j, start, label_len; 
	
	if (domain[len - 1] != '.') {
		lxl_log_error(LXL_LOG_WARN, 0, "domain last char not (.)");
		return -1;
	}

	start = 0;
	for (i = 0; i < len; ++i) {
		if (domain[i] == '.') {
			for(j = i; j > start; --j) {
				domain[j] = domain[j - 1];
			}

			label_len = i - start;
			if (label_len > LXL_DNS_LABLE_MAXSIZE) {
				lxl_log_error(LXL_LOG_WARN, 0, "domain label not more than %lu", LXL_DNS_LABLE_MAXSIZE);
				return -1;
			}

			domain[j] = (unsigned char) label_len;
			start = i + 1;
		} else {
			domain[i] = lxl_tolower(domain[i]);
		}
	}

	return 0;
}

lxl_int_t
lxl_dns_parse_request(lxl_dns_request_t *r, char *data)
{
	lxl_uint_t qlen, label;

	r->header = (lxl_dns_header_t *) data;
	r->qname =  data + sizeof(lxl_dns_header_t);
	snprintf(r->rid, sizeof(r->rid), "%lX%04hX", lxl_current_msec, r->header->id);
	qlen = 0;
	label = (lxl_uint_t) *(r->qname);
	while (label) {
		if (label > LXL_DNS_LABLE_MAXSIZE) {
			lxl_log_error(LXL_LOG_WARN, 0, "%s dns label max size %u", r->rid, LXL_DNS_LABLE_MAXSIZE);
			r->rcode = LXL_DNS_RCODE_FORMERR;
			return -1;
		}

		if (qlen + label > LXL_DNS_NAME_MAXSIZE) {
			lxl_log_error(LXL_LOG_WARN, 0, "%s dns name max size %u", r->rid, LXL_DNS_NAME_MAXSIZE);
			r->rcode = LXL_DNS_RCODE_FORMERR;
			return -1;
		}

		qlen += (label + 1);
		label = (lxl_uint_t) *(r->qname + qlen);
	}

	r->qlen = qlen + 1;
	lxl_strlower(r->qname, r->qlen);
	r->question_part = (lxl_dns_question_part_t *) (r->qname + r->qlen);
	if (r->question_part->qtype != LXL_DNS_A && r->question_part->qtype != LXL_DNS_NS
			//&& r->question_part->qtype != LXL_DNS_AAAA 
			&& r->question_part->qtype != LXL_DNS_PTR && r->question_part->qtype != LXL_DNS_MX && r->question_part->qtype != LXL_DNS_TXT
			&& r->question_part->qtype != LXL_DNS_CNAME && r->question_part->qtype != LXL_DNS_SOA) {
		r->rcode = LXL_DNS_RCODE_NOTIMP;
		lxl_log_error(LXL_LOG_WARN, 0, "%s dns only support A NS PTR MX TXT CNAME SOA now 0x%04x", r->rid, r->question_part->qtype);
		return -1;
	}

	if (ntohs(r->header->arcount) > 0) {
		lxl_log_error(LXL_LOG_WARN, 0, "%s dns not implemented edns", r->rid);
		r->rcode = LXL_DNS_RCODE_NOTIMP;
		return -1;
	}

	if (strstr(r->qname, "localdomain")) {
		r->rcode = LXL_DNS_RCODE_NAMEERR;
		lxl_log_error(LXL_LOG_WARN, 0, "%s ======dns not ====== not localdomain", r->rid);
		return -1;
	}
	
	return 0;
}

void      
lxl_dns_parse_package_to_hex(char *data, size_t size)
{
	int n;
	size_t i, len;
    char buffer[128];
    uint8_t oct;
    uint16_t hex0, hex1, hex2, hex3, hex4, hex5, hex6, hex7, hex8, hex9, hex10, hex11;
    
	lxl_log_error(LXL_LOG_INFO, 0, "parse packge size %lu to hex", size);
	i = size / 24;
	while (i--) {
		hex0 = ntohs(*((uint16_t *) data));
		hex1 = ntohs(*((uint16_t *) data + 2));
		hex2 = ntohs(*((uint16_t *) data + 4));
		hex3 = ntohs(*((uint16_t *) data + 6));
		hex4 = ntohs(*((uint16_t *) data + 8));
		hex5 = ntohs(*((uint16_t *) data + 10));
		hex6 = ntohs(*((uint16_t *) data + 12));
		hex7 = ntohs(*((uint16_t *) data + 14));
		hex8 = ntohs(*((uint16_t *) data + 16));
		hex9 = ntohs(*((uint16_t *) data + 18));
		hex10 = ntohs(*((uint16_t *) data + 20));
		hex11 = ntohs(*((uint16_t *) data + 22));
		data += 24;
		lxl_log_error(LXL_LOG_INFO, 0, "%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x", 
					hex0, hex1, hex2, hex3, hex4, hex5, hex6, hex7, hex8, hex9, hex10, hex11);
    }
	
	len = 0;
	i = size % 24;
	i = i / 2;
	while (i--) {
		hex0 = ntohs(*((uint16_t *) data));
		data += 2;
		n = snprintf(buffer + len, 128 - len, "%04x ", hex0);
		len += n;
	}

	i = size % 2;
	if (i) {
		oct = (*(uint8_t *) data);
		snprintf(buffer + len, 128 - len, "%02x", oct);
	}

	lxl_log_error(LXL_LOG_INFO, 0, "%s", buffer);
}
