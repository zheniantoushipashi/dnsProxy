
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_log.h>
#include <lxl_hash.h>
#include <lxl_string.h>
#include <lxl_palloc.h>
#include <lxl_process.h>
#include <lxl_dns.h>
#include <lxl_dns_data.h>


extern lxl_uint_t  lxl_current_sec;
extern lxl_hash_t  lxl_dns_hash;
extern lxl_pool_t *lxl_dns_pool;


#define lxl_dns_ipv4cmp(s, c)					\
	lxl_str4cmp(s, c[0], c[1], c[2], c[3])

#define lxl_dns_ipv6cmp(s, c)					\
	lxl_str16cmp(s, c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8], c[9], c[10], c[11], c[12], c[13], c[14], c[15])


static lxl_int_t lxl_dns_data_dump_rr(char *buffer, lxl_dns_rr_t *rr);


void 
lxl_dns_data_dump(void)
{
	int fd;
	char file[32];
	char buffer[4096];

	snprintf(file, sizeof(file), "dump_zone_data.%u", lxl_pid);
	lxl_log_error(LXL_LOG_INFO, 0, "dns dump data to file %s", file);
	fd = open(file, O_CREAT|O_WRONLY|O_TRUNC, 0666);
	if (fd == -1) {
		lxl_log_error(LXL_LOG_ALERT, errno, "open(%s) failed", file);
		return;
	}

	ssize_t n;
	char zname[256];
	lxl_uint_t i, j, count, nelts;
	lxl_list_t *list1, *list2;
    lxl_hash_elt_t *tmp, *next, **buckets;
	lxl_dns_rr_t rr;
	lxl_dns_zone_t *zone;
	lxl_dns_rrset_t *rrset;
	lxl_dns_rdata_t *rdata;

    nelts = lxl_dns_hash.nelts;
	for (i = 0; i < nelts; ++i) {
		j = 0;
		buckets = &lxl_dns_hash.buckets[i];
		tmp = *buckets;
		while (tmp){
			next = tmp->next;
			zone = (lxl_dns_zone_t *) tmp->value;
			lxl_dns_label_to_dot(tmp->name, zname);
			n = snprintf(buffer, 4096, "\nzone:[%s]\texpires:[%lu]\tbuckets:[%lu,%lu]\n", zname, lxl_current_sec - zone->update_sec, i, j);
			write(fd, buffer, n);
			for (list1 = lxl_list_head(&zone->rrset_list); list1 != lxl_list_sentinel(&zone->rrset_list); list1 = lxl_list_next(list1)) {
				rrset = lxl_list_data(list1, lxl_dns_rrset_t, list);
				rr.type = rrset->type;
				rr.ttl = rrset->ttl;
				rr.nlen = rrset->nlen;
				rr.name = rrset->name;
				for (list2 = lxl_list_head(&rrset->rdata_list); list2 != lxl_list_sentinel(&rrset->rdata_list); list2 = lxl_list_next(list2)) {
					rdata = lxl_list_data(list2, lxl_dns_rdata_t, list);
					rr.rdlength = rdata->rdlength;
					rr.rdata = rdata->rdata;
					count = lxl_dns_data_dump_rr(buffer, &rr);
					if (count > 0) {
						n = write(fd, buffer, count);
						if (n == -1) {
							lxl_log_error(LXL_LOG_ALERT, errno, "write(%d) failed", fd);
						}
					} else {
						lxl_log_error(LXL_LOG_ALERT, 0, "lxl_dns_data_dump_rr(name:%s type:0x%04x) failed", rr.name, rr.type);
					}
				}
			}

			tmp = next;
			++j;
		}   
	} 

	lxl_log_error(LXL_LOG_INFO, 0, "dns data dump suceed");
}

static lxl_int_t 
lxl_dns_data_dump_rr(char *buffer, lxl_dns_rr_t *rr)
{
	uint16_t weight;
	int ret;
	char name[256], data[256];
	char *str;
	
	lxl_dns_label_to_dot(rr->name, name);
	str = lxl_dns_get_rr_strtype(rr->type);
	switch (rr->type) {
		case LXL_DNS_A:
			inet_ntop(AF_INET, rr->rdata, data, 256);
			ret = snprintf(buffer, 4096, "%-40s\t%-8s\t%-8u\t%-40s\n", name, str, rr->ttl, data);
			break;
		
		case LXL_DNS_AAAA:
			inet_ntop(AF_INET6, rr->rdata, data, 256);
			ret = snprintf(buffer, 4096, "%-40s\t%-8s\t%-8u\t%-40s\n", name, str, rr->ttl, data);
			break;

		case LXL_DNS_MX:
			weight = ntohs(*((uint16_t *) data));
			lxl_dns_label_to_dot(rr->rdata + 2, data);
			ret = snprintf(buffer, 4096, "%-40s\t%-8s\t%-8u\t%-8hu\t%-40s\n", name, str, rr->ttl, weight, data);
			break;

		case LXL_DNS_SOA:
		case LXL_DNS_TXT:
			memcpy(data, rr->rdata, rr->rdlength);
			data[rr->rdlength + 1] = '\0'; 
			ret = snprintf(buffer, 4096, "%-40s\t%-8s\t%-8u\t%-40s\n", name, str, rr->ttl, data);
			break;

		default:
			lxl_dns_label_to_dot(rr->rdata, data);
			ret = snprintf(buffer, 4096, "%-40s\t%-8s\t%-8u\t%-40s\n", name, str, rr->ttl, data);
			break;
	}

	return (lxl_int_t) ret;
}

void 
lxl_dns_data_rebuild(void)
{
	lxl_pool_t *new_pool;
	lxl_list_t *list1, *list2;
	lxl_dns_rr_t rr;
	lxl_dns_zone_t *old_zone, *new_zone;
	lxl_dns_rrset_t *rrset;
	lxl_dns_rdata_t *rdata;
	
	old_zone = lxl_dns_zone_find(LXL_DNS_ROOT_LABEL, LXL_DNS_ROOT_LEN);
	if (old_zone == NULL) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dns data rebuild root not find");
		return;
	}

	lxl_log_error(LXL_LOG_INFO, 0, "dns data rebuild create new pool");
	new_pool = lxl_create_pool(LXL_DEFAULT_POOL_SIZE);
	if (new_pool == NULL) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dns data rebuild create pool failed");
		return;
	}

	lxl_log_error(LXL_LOG_INFO, 0, "dns data rebuild init hash");
	if (lxl_hash_init(&lxl_dns_hash, new_pool, lxl_dns_hash.nelts, lxl_dns_hash.key) == -1) {
        lxl_log_error(LXL_LOG_ERROR, 0, "dns init hash failed");
        return;
    }

	new_zone = lxl_palloc(new_pool, sizeof(lxl_dns_zone_t));
	if (new_zone == NULL) {
		return;
	}

	lxl_list_init(&new_zone->rrset_list);
	new_zone->update_sec = lxl_current_sec;
	for (list1 = lxl_list_head(&old_zone->rrset_list); list1 != lxl_list_sentinel(&old_zone->rrset_list); list1 = lxl_list_next(list1)) {
		rrset = lxl_list_data(list1, lxl_dns_rrset_t, list);
		rr.type = rrset->type;
		rr.ttl = rrset->ttl;
		rr.nlen = rrset->nlen;
		rr.name = rrset->name;
		rr.soa_nlen = 0;
		rr.soa_flags = LXL_DNS_RRSET_NORMAL_TYPE;
		rr.expires_sec = lxl_current_sec + rrset->ttl;
		rr.update_flags = LXL_DNS_RR_NORMAL_TYPE;
		for (list2 = lxl_list_head(&rrset->rdata_list); list2 != lxl_list_sentinel(&rrset->rdata_list); list2 = lxl_list_next(list2)) {
			rdata = lxl_list_data(list2, lxl_dns_rdata_t, list);
			rr.rdlength = rdata->rdlength;
			rr.rdata = rdata->rdata;
			if (lxl_dns_rr_add(new_pool, new_zone, &rr) == -1) {
				lxl_log_error(LXL_LOG_ERROR, 0, "dns data rebuild add rr failed");
				return;
			}
		}
	}

	lxl_log_error(LXL_LOG_INFO, 0, "dns data rebuild add root zone");
	if (lxl_dns_zone_add(LXL_DNS_ROOT_LABEL, LXL_DNS_ROOT_LEN, new_zone) == -1) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dns data rebuild add root zone failed");
		//return;
	}

	lxl_log_error(LXL_LOG_INFO, 0, "dns data rebuild destory pool");
	lxl_destroy_pool(lxl_dns_pool);
	lxl_dns_pool = new_pool;
	lxl_log_error(LXL_LOG_INFO, 0, "dns data rebuild succeed");
}

lxl_dns_zone_t *
lxl_dns_zone_addfind(char *zname, uint16_t zlen)
{
	void **value;
	lxl_dns_zone_t *zone;

	value = lxl_hash_addfind(&lxl_dns_hash, zname, zlen);
	if (value == NULL) {
		return NULL;
	}

	if (*value == NULL) {
		zone = lxl_palloc(lxl_dns_pool, sizeof(lxl_dns_zone_t));
		if (zone == NULL) {
			return NULL;
		}

		lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns data add zone zname:%s zlen:%hu", zname, zlen);
		lxl_list_init(&zone->rrset_list);
		*value = zone;
	}

	return *value;
}

void
lxl_dns_zone_update(lxl_dns_zone_t *zone)
{
	lxl_list_t *list1, *list2;
	lxl_dns_rrset_t *rrset;
	lxl_dns_rdata_t *rdata;

	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns zone update");
	zone->update_sec = lxl_current_sec;
	for (list1 = lxl_list_head(&zone->rrset_list); list1 != lxl_list_sentinel(&zone->rrset_list); list1 = lxl_list_next(list1)) {
		rrset = lxl_list_data(list1, lxl_dns_rrset_t, list);
		if (rrset->update_flags) {
			rrset->update_flags = 0;
			for (list2 = lxl_list_head(&rrset->rdata_list); list2 != lxl_list_sentinel(&rrset->rdata_list); list2 = lxl_list_next(list2)) {
				rdata = lxl_list_data(list2, lxl_dns_rdata_t, list);
				if (rdata->update_flags) {
					rdata->update_flags = 0;
				} else {
					lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "delete rrset: name:%s nlen:%hu type:0x%04x rdata:%s rdlength:%hu", 
						rrset->name, rrset->nlen, rrset->type, rdata->rdata, rdata->rdlength);
					lxl_list_del(list2);
					//lxl_free(rdata);
				}
			}
		}
	}
}

lxl_int_t 
lxl_dns_rr_add(lxl_pool_t *pool, lxl_dns_zone_t *zone, lxl_dns_rr_t *rr)
{
	lxl_dns_rrset_t *rrset;
	lxl_dns_rdata_t *rdata;

#if 1
	char *data, buffer[40];	/* 40 ipv6 16 ipv4 */
	if (rr->soa_flags & LXL_DNS_RRSET_NORMAL_TYPE) {
		switch(rr->type) {
			case LXL_DNS_A:
				inet_ntop(AF_INET, rr->rdata, buffer, 40);
				data = buffer;
				break;

			case LXL_DNS_AAAA:
				inet_ntop(AF_INET6, rr->rdata, buffer, 40);
				data = buffer;
				break;

			case LXL_DNS_MX:
				data = rr->rdata + 2;

			default:
				data = rr->rdata;
				break;
		}
	} else {
		data = rr->rdata;
	}

	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns add rr: name:%s type:%04x ttl:%u rdata:%s", rr->name, rr->type, rr->ttl, data);
#endif
	rrset = lxl_dns_rrset_find(&zone->rrset_list, rr->name, rr->nlen, rr->type, LXL_DNS_RRSET_NORMAL_FIND);
	if (rrset == NULL) {
		lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dna add rrset");
		rrset = lxl_palloc(pool, sizeof(lxl_dns_rrset_t) + rr->nlen);
		if (rrset == NULL) {
			return -1;
		}

		rdata = lxl_palloc(pool, sizeof(lxl_dns_rdata_t) + rr->rdlength);
		if (rdata == NULL) {
			//lxl_free(rrset);
			return -1;
		}

		
		rrset->soa_nlen = rr->soa_nlen;
		rrset->soa_flags = rr->soa_flags;
		rrset->expires_sec = rr->expires_sec;
		rrset->update_flags = rr->update_flags;

		rrset->type = rr->type;
		rrset->ttl = rr->ttl;
		rrset->nlen = rr->nlen;
		memcpy(rrset->name, rr->name, rr->nlen);

		rdata->update_flags = rr->update_flags;
		rdata->rdlength = rr->rdlength;
		memcpy(rdata->rdata, rr->rdata, rr->rdlength);

		lxl_list_init(&rrset->rdata_list);
		lxl_list_add_tail(&rrset->rdata_list, &rdata->list);
		lxl_list_add_tail(&zone->rrset_list, &rrset->list);

		return 0;
	}

	rrset->update_flags = rr->update_flags;
	rrset->ttl = rr->ttl;
	rrset->expires_sec = rr->expires_sec;
	rdata = lxl_dns_rdata_find(&rrset->rdata_list, rr->rdata, rr->rdlength);
	if (rdata == NULL) {
		lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns add rdata");
		rdata = lxl_palloc(pool, sizeof(lxl_dns_rdata_t) + rr->rdlength);
		if (rdata == NULL) {
			return -1;
		}

		rdata->update_flags = rr->update_flags;
		rdata->rdlength = rr->rdlength;
		memcpy(rdata->rdata, rr->rdata, rr->rdlength);

		lxl_list_add_tail(&rrset->rdata_list, &rdata->list);
	} else {
		rdata->update_flags = rr->update_flags;
	}
	
	return 0;
}

lxl_dns_rrset_t *
lxl_dns_rrset_find(lxl_list_t *head, char *name, uint16_t nlen, uint16_t type, uint16_t flags)
{
	lxl_list_t *l;
	lxl_dns_rrset_t *rrset;

	for (l = lxl_list_next(head); l != lxl_list_sentinel(head); l = lxl_list_next(l)) {
		rrset = lxl_list_data(l, lxl_dns_rrset_t, list);
		if (rrset->nlen == nlen && (rrset->type == type) && memcmp(rrset->name, name, nlen) == 0) {
			if (flags) {
				if (rrset->expires_sec > lxl_current_sec) {
#if LXL_DEBUG
					lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns expires find rrset: name:%s nlen:%hu type:%04x", name, nlen, type);
#endif
					return rrset;
				} 
#if LXL_DEBUG
				else {
					lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns expires find expires %lds rrset: name:%s nlen:%hu type:%04x", 
									lxl_current_sec - rrset->expires_sec, name, nlen, type);
				}
#endif
			} else {

#if LXL_DEBUG
				lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns normal find rrset: name:%s nlen:%hu type:%04x", name, nlen, type);
#endif
				return rrset;
			}
		}
	}

#if LXL_DEBUG
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns not find rrset: name:%s nlen:%hu type:%04x", name, nlen, type);
#endif
	return NULL;
}

lxl_dns_rrset_t *
lxl_dns_rrset_dnsfind(lxl_list_t *head, char *name, uint16_t nlen, uint16_t type)
{
	lxl_list_t *list;
	lxl_dns_rrset_t *rrset;

	for (list = lxl_list_next(head); list != lxl_list_sentinel(head); list = lxl_list_next(list)) {
		rrset = lxl_list_data(list, lxl_dns_rrset_t, list);
		if (rrset->nlen == nlen && 
			(rrset->type == type || (rrset->type == LXL_DNS_CNAME && (rrset->soa_flags & LXL_DNS_RRSET_NORMAL_TYPE)))
			 && memcmp(rrset->name, name, nlen) == 0) {
			if (rrset->expires_sec > lxl_current_sec) {
#if LXL_DEBUG
				lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns dnsfind rrset: name:%s nlen:%hu type:%04x", name, nlen, type);
#endif
				return rrset;
			}
#if DEBUG
			else {
				lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns find expires %lds rrset: name:%s nlen:%hu type:%04x", 
								lxl_current_sec - rrset->expires_sec, name, nlen, type);
			}
#endif
		}
	}

#if LXL_DEBUG
			lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns not dnsfind rrset: name:%s nlen:%hu type:0x%04x", name, nlen, type);
#endif
	return NULL;
}

lxl_dns_rrset_t *
lxl_dns_rrset_typefind(lxl_list_t *head, uint16_t type)
{
	lxl_list_t *list;
	lxl_dns_rrset_t *rrset;

	for (list = lxl_list_next(head); list != lxl_list_sentinel(head); list = lxl_list_next(list)) {
		rrset = lxl_list_data(list, lxl_dns_rrset_t, list);
		if (rrset->type == type && (rrset->soa_flags & LXL_DNS_RRSET_NORMAL_TYPE)) {
			if (rrset->expires_sec > lxl_current_sec) {
#if LXL_DEBUG
				lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns typefind rrset type:%04x", type);
#endif
				return rrset;
			}
#if LXL_DEBUG
			else {
				lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns find expires %lds rrset: type:%04x", lxl_current_sec - rrset->expires_sec, type);
			}
#endif
		}
	}

#if LXL_DEBUG
			lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns not typefind rrset:%04x", type);
#endif
	return NULL;
}

lxl_dns_rrset_t *
lxl_dns_rrset_soafind(lxl_list_t *head)
{
	lxl_list_t *list;
	lxl_dns_rrset_t *rrset;

	for (list = lxl_list_next(head); list != lxl_list_sentinel(head); list = lxl_list_next(list)) {
		rrset = lxl_list_data(list, lxl_dns_rrset_t, list);
		if (rrset->type == LXL_DNS_SOA) {
			if (rrset->expires_sec > lxl_current_sec) {
#if LXL_DEBUG
				lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns soafind rrset");
#endif
				return rrset;
			}
#if LXL_DEBUG
			else {
				lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns soafind expires %lds", lxl_current_sec - rrset->expires_sec);
			}
#endif
		}
	}

#if LXL_DEBUG
			lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns not soafind rrset");
#endif
	return NULL;
}

#include <lxl_alloc.h>

lxl_dns_rdata_t *
lxl_dns_rdata_create(char *data, size_t len, uint16_t type)
{
	lxl_dns_rdata_t *rdata;

	// switch mx  soa
	if (type == LXL_DNS_A) {
		/* uint32_t */
		rdata = lxl_alloc(sizeof(lxl_dns_rdata_t) + 4);
		if (rdata == NULL) {
			return NULL;
		}
	
		if (inet_pton(AF_INET, data, (void *) &rdata->rdata) <= 0) {
			goto failed;
		}

		rdata->rdlength = 4;
	} else if (type == LXL_DNS_AAAA) {
		/* 8 * uint16_t */
		rdata = lxl_alloc(sizeof(lxl_dns_rdata_t) + 16);
		if (rdata == NULL) {
			return NULL;
		}
	
		if (inet_pton(AF_INET6, data, (void *) &rdata->rdata) <= 0) {
			goto failed;
		}
			
		rdata->rdlength = 16;
//	} else {
	} else if (type == LXL_DNS_NS) {
		lxl_dns_domain_dot_to_label(data, len);
		rdata = lxl_alloc(sizeof(lxl_dns_rdata_t) + len + 1);
		if (rdata == NULL) {
			return NULL;
		}

		memcpy(&rdata->rdata, data, len + 1);
		rdata->rdlength = (uint16_t) len + 1;
	} else {
		return NULL;
	}

	return rdata;

failed:
	//lxl_free(rdata);
	lxl_log_debug(LXL_LOG_ERROR, errno, "inet_pton(type:%04x) failed", type);
	return NULL;
}

lxl_dns_rdata_t *
lxl_dns_rdata_find(lxl_list_t *head, char *rdata, uint16_t rdlength)
{
	lxl_list_t *list;
	lxl_dns_rdata_t *dns_rdata;

	for (list = lxl_list_next(head); list != lxl_list_sentinel(head); list = lxl_list_next(list)) {
		dns_rdata = lxl_list_data(list, lxl_dns_rdata_t, list);
		if (dns_rdata->rdlength == rdlength && memcmp(dns_rdata->rdata, rdata, rdlength) == 0) {
#if LXL_DEBUG
			lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns typefind rdata %s, rdlength %hu", rdata, rdlength);
#endif
			return dns_rdata;
		}
	}

#if LXL_DEBUG
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns not typefind rrdata %s, rdlength %hu", rdata, rdlength);
#endif
	return NULL;
}

lxl_dns_rdata_t *
lxl_dns_rdata_typefind(lxl_list_t *head, char *rdata, uint16_t rdlength, uint16_t type)
{
	lxl_list_t *list;
	lxl_dns_rdata_t *dns_rdata;

	for (list = lxl_list_next(head); list != lxl_list_sentinel(head); list = lxl_list_next(list)) {
		dns_rdata = lxl_list_data(list, lxl_dns_rdata_t, list);
		if (dns_rdata->rdlength == rdlength) {
			if (type == LXL_DNS_MX) {
				if (memcmp(dns_rdata->rdata + 2, rdata + 2, rdlength - 2) == 0) {
#if LXL_DEBUG
					lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns find: rdata:%s rdlength:%hu type:MX", rdata + 2, rdlength);
#endif
					return dns_rdata;
				}
			} else {
				if (memcmp(dns_rdata->rdata, rdata, rdlength) == 0) {
#if LXL_DEBUG
					lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns find: rdata:%s rdlength:%hu type:%04x", rdata, rdlength, type);
#endif
					return dns_rdata;
				}
			}
		}
	}

#if LXL_DEBUG
	lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "dns not find: rrdata rdlength:%hu type:%04x", rdlength, type);
#endif
	return NULL;
}

lxl_int_t       
lxl_dns_label_to_dot(char *domain_label, char *domain_dot)
{
	u_char label;
	char *p; 
	
    p = domain_label;
    label = *((u_char *) p); 
	if (label == 0) {
		domain_dot[0] = '.';
		domain_dot[1] = '\0';
		
		return 0;
	}

    do {
        /*if (lable > DNS_MAX_LABLE || end > (unsigned int) len) {
            return -1; 
        } */  

		++p;
		do {
			*domain_dot++ = *p++;
			--label;
		} while (label);


        *domain_dot++ = '.'; 
        label = *((u_char *) p);
    } while (label);
    *domain_dot = '\0';

	return 0;
}

uint16_t 	
lxl_dns_get_rr_type(char *type, size_t len) 
{
	uint16_t t;

	lxl_strupper(type, len);
	// switch PTR
	if (len == LXL_DNS_STR_A_LEN && memcmp(type, LXL_DNS_STR_A, len) == 0) {
		t = LXL_DNS_A;
	} else if (len == LXL_DNS_STR_NS_LEN && memcmp(type, LXL_DNS_STR_NS, len) == 0) {
		t = LXL_DNS_NS;
	} else if (len == LXL_DNS_STR_AAAA_LEN && memcmp(type, LXL_DNS_STR_AAAA, len) == 0) {
		t = LXL_DNS_AAAA;
	} else if (len == LXL_DNS_STR_CNAME_LEN && memcmp(type, LXL_DNS_STR_CNAME, len) == 0) {
		t = LXL_DNS_CNAME;
	} else if (len == LXL_DNS_STR_SOA_LEN && memcmp(type, LXL_DNS_STR_SOA, len) == 0) {
		t = LXL_DNS_SOA;
	} else if (len == LXL_DNS_STR_MX_LEN && memcmp(type, LXL_DNS_STR_MX, len) == 0) {
		t = LXL_DNS_MX;
	} else if (len == LXL_DNS_STR_TXT_LEN && memcmp(type, LXL_DNS_STR_TXT, len) == 0) {
		t = LXL_DNS_TXT;
	} else {
		t = LXL_DNS_INVALID_TYPE;
	}

	return t;
}

char *
lxl_dns_get_rr_strtype(uint16_t type)
{
	char *s;

	switch (type) {
		case LXL_DNS_A:
			s = LXL_DNS_STR_A;
			break;

		case LXL_DNS_NS:
			s = LXL_DNS_STR_NS;
			break;
		
		case LXL_DNS_CNAME:
			s = LXL_DNS_STR_CNAME;
			break;

		case LXL_DNS_SOA:
			s = LXL_DNS_STR_SOA;
			break;
		
		case LXL_DNS_PTR:
			s = LXL_DNS_STR_PTR;
			break;

		case LXL_DNS_MX:
			s = LXL_DNS_STR_MX;
			break;

		case LXL_DNS_TXT:
			s = LXL_DNS_STR_TXT;
			break;

		case LXL_DNS_AAAA:
			s = LXL_DNS_STR_AAAA;
			break;

		default:
			s = LXL_DNS_STR_INVALID_TYPE;
			break;
	}

	return s;
}
