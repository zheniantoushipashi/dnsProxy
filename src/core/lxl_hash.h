
/*
 * Copyright (c) xianliang.li
 */


#ifndef LXL_HASH_H_INCLUDE
#define	LXL_HASH_H_INCLUDE


#include <lxl_config.h>
#include <lxl_string.h>
#include <lxl_palloc.h>


typedef struct lxl_hash_elt_s lxl_hash_elt_t;

struct lxl_hash_elt_s {
	lxl_hash_elt_t *next;  
	void *value;		/* sizeof(lxl_*_t) <= 8 */
	lxl_uint_t len;	
	char name[0];
};

typedef lxl_uint_t (*lxl_hash_key_pt) (u_char *data, size_t len); 

typedef struct {
	lxl_uint_t 		nelts;	
	lxl_hash_elt_t **buckets;  	/* bucke == NULL */
	lxl_hash_key_pt key;
	lxl_pool_t     *pool;
} lxl_hash_t;

typedef struct {
	char *name;
	void *value;
} lxl_hash_keyvalue_t;

int		lxl_hash_init(lxl_hash_t *hash, lxl_pool_t *pool, lxl_uint_t nelts, lxl_hash_key_pt key);
/*void 	lxl_hash_clear(lxl_hash_t *hash);
void 	lxl_hash_destroy(lxl_hash_t *hash);*/
int 	lxl_hash_add(lxl_hash_t *hash, char *name, size_t len, void *value);
void   *lxl_hash_del(lxl_hash_t *hash, char *name, size_t len);
void   *lxl_hash_find(lxl_hash_t *hash, char *name, size_t len);
void  **lxl_hash_addfind(lxl_hash_t *hash, char *name, size_t len);

#define lxl_hash(key, c)	((lxl_uint_t) key * 31 + c)	

static inline lxl_uint_t 
lxl_hash_key(u_char *data, size_t len) 
{ 
	lxl_uint_t i, key;

	key = 0;
	for (i = 0; i < len; ++i) {
		key = lxl_hash(key, data[i]);
	}

	return key;
}

static inline lxl_uint_t
lxl_hash_key_lc(u_char *data, size_t len) 
{
	lxl_uint_t i, key;

	key = 0;
	for (i = 0; i < len; ++i) {
		key = lxl_hash(key, lxl_tolower(data[i]));
	}

	return key;
}


#endif	/* LXL_HASH_H_INCLUDE */
