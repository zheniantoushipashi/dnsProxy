
#include <lxl_config.h>
#include <lxl_log.h>
#include <lxl_palloc.h>
#include <lxl_hash.h>


lxl_int_t lxl_hash_sample(lxl_uint_t nelts);


int
main(int argc, char *argv[])
{
	lxl_log_init(LXL_LOG_DEBUG, LXL_LOG_DEBUG_ALL, LXL_LOG_FLUSH);
	//lxl_hash_sample(10 * 10000);
	lxl_hash_sample(10);
		

	return 0;
}

lxl_int_t
lxl_hash_sample(lxl_uint_t nelts)
{
	char name[64];
	lxl_uint_t i, len, *value;
	lxl_pool_t *pool;
	lxl_hash_t hash;

	pool = lxl_create_pool(1024);
	lxl_hash_init(&hash, pool, nelts, lxl_hash_key);
	for (i = 0; i < 40; ++i) {
		value = lxl_alloc(sizeof(lxl_uint_t));
		*value = i;
		len = snprintf((char *) name, sizeof(name), "www.%lutaobao%lu.com", i, 40 - i);
		if (lxl_hash_add(&hash, name, len, value) == -1) {
			lxl_log_error(LXL_LOG_DEBUG, 0, "add exist value %lu, len %lu", *value, len);
		} else {
			lxl_log_error(LXL_LOG_DEBUG, 0, "add value %lu, len %lu", *value, len);
		}
	}

	for (i = 8; i < 18; ++i) {
		len = snprintf((char *) name, sizeof(name), "www.%lutaobao%lu.com", i, 40 - i);
		value = lxl_hash_del(&hash, name, len);
		if (value) {
			lxl_log_error(LXL_LOG_DEBUG,0, "hash delete name %s, key %lu", name, *value);
			//lxl_free(value);
		} else {
			lxl_log_error(LXL_LOG_DEBUG,0, "hash not find name %s", name);
		}
	}

	for (i = 0; i < 40; ++i) {
		len = snprintf((char *) name, sizeof(name), "www.%lutaobao%lu.com", i, 40 - i);
		value = lxl_hash_find(&hash, name, len);
		if (value) {
			lxl_log_error(LXL_LOG_DEBUG, 0, "hash find value %lu", *value);
		} else {
			lxl_log_error(LXL_LOG_DEBUG,0, "hash not find name %s", name);
		}
	}
	
	return 0;
}
