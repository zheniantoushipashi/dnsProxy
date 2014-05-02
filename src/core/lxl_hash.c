
/* 
 * Copyright (C) xianliang.li
 */


#include <lxl_hash.h>


int
lxl_hash_init(lxl_hash_t *hash, lxl_pool_t *pool, lxl_uint_t nelts, lxl_hash_key_pt key)
{	
	hash->buckets = lxl_pcalloc(pool, nelts * sizeof(lxl_hash_elt_t *));
	if (hash->buckets == NULL) {
		return -1;
	}

	hash->nelts = nelts;
	hash->key =  key;
	hash->pool = pool;

	return 0;
}
/*
void 
lxl_hash_clear(lxl_hash_t *hash)
{
	lxl_uint_t i, nelts;
    lxl_hash_elt_t *tmp, *next, **buckets;

    nelts = hash->nelts;
    for (i = 0; i < nelts; ++i) {
        buckets = &hash->buckets[i];
		tmp = *buckets;
		if (tmp) {
			*buckets = NULL;
		}

		while (tmp){
			next = tmp->next;
			lxl_free(tmp);
			tmp = next;
		} 
    }
}

void 
lxl_hash_destroy(lxl_hash_t *hash) 
{
	lxl_uint_t i, nelts;
    lxl_hash_elt_t *tmp, *next, **buckets;

    nelts = hash->nelts;
    for (i = 0; i < nelts; ++i) {
        buckets = &hash->buckets[i];
		tmp = *buckets;
		if (tmp) {
			*buckets = NULL;
		}

		while (tmp){
			next = tmp->next;
			lxl_free(tmp);
			tmp = next;
		} 
    }

	lxl_free(hash->buckets);
	hash->nelts = 0;
	hash->buckets = NULL;
	hash->key = NULL;
}*/

int
lxl_hash_add(lxl_hash_t *hash, char *name, size_t len, void *value)
{
	lxl_uint_t hash_key;
	lxl_hash_elt_t *elt, *tmp, **curr, **buckets;

	hash_key = lxl_hash_key((u_char *) name, len);
	buckets = &hash->buckets[hash_key % hash->nelts];
	curr = buckets;
	while (*curr) {
		tmp = *curr;
		if (tmp->len == len && memcmp(tmp->name, name, len) == 0) {
			return -1;
		}
		
		curr = &tmp->next;
	}

	elt = (lxl_hash_elt_t *) lxl_palloc(hash->pool, sizeof(lxl_hash_elt_t) + len);
	if (elt == NULL) {
		return -1;
	}

	elt->value = value;
	elt->next = NULL;
	elt->len = len;
	memcpy(elt->name, name, len); 
	*curr = elt;
	
	return 0;
}

void *
lxl_hash_del(lxl_hash_t *hash, char *name, size_t len)
{
	lxl_uint_t hash_key;
	lxl_hash_elt_t *tmp, **curr, **buckets;

	hash_key = lxl_hash_key((u_char *) name, len);
	buckets = &hash->buckets[hash_key % hash->nelts];
	curr = buckets;
	while (*curr) {
		tmp = *curr;
		if (tmp->len == len && memcmp(tmp->name, name, len) == 0) {
			*curr = tmp->next;

			return tmp->value;
		}

		curr = &tmp->next;
	}

	return NULL;
}

void *
lxl_hash_find(lxl_hash_t *hash, char *name, size_t len)
{
	lxl_uint_t hash_key;
	lxl_hash_elt_t *tmp, **curr, **buckets;
	
	hash_key = lxl_hash_key((u_char *) name, len);
	buckets = &hash->buckets[hash_key % hash->nelts];
	curr = buckets;
	while (*curr) {
		tmp = *curr;
		if (tmp->len == len && memcmp(tmp->name, name, len) == 0) {
			return tmp->value;
		}

		curr = &tmp->next;
	}

	return NULL;
}

void **
lxl_hash_addfind(lxl_hash_t *hash, char *name, size_t len)
{
	lxl_uint_t hash_key;
	lxl_hash_elt_t *elt, *tmp, **curr, **buckets;
	
	hash_key = lxl_hash_key((u_char *) name, len);
	buckets = &hash->buckets[hash_key % hash->nelts];
	curr = buckets;
	while (*curr) {
		tmp = *curr;
		if (tmp->len == len && memcmp(tmp->name, name, len) == 0) {
			return &tmp->value;
		}

		curr = &tmp->next;
	}

	elt = (lxl_hash_elt_t *) lxl_palloc(hash->pool, sizeof(lxl_hash_elt_t) + len);
	if (elt == NULL) {
		return NULL;
	}

	elt->value = NULL;
	elt->next = NULL;
	elt->len = len;
	memcpy(elt->name, name, len);
	*curr = elt;
	
	return &elt->value;
}
