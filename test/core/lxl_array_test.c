
#include <lxl_array.h>


lxl_int_t lxl_intcmp(void *elt1, void *elt2);
lxl_int_t lxl_array_sample(void);
lxl_int_t lxl_array_sample_ptr(void);

int main(int argc, char *argv[])
{
	lxl_log_t *log = lxl_log_init(LXL_LOG_DEBUG, LXL_LOG_FLUSH);
	//lxl_array_sample();
	lxl_array_sample_ptr();
	
	return 0;
}


lxl_int_t 
lxl_intcmp(void *elt1, void *elt2)
{
	return (*(int *) elt1 - *(int *) elt2);
}

lxl_int_t 
lxl_array_sample()
{
	lxl_int_t i, *value;
	void *k;

	lxl_pool_t *p = lxl_pool_create(LXL_DEFAULT_POOL_SIZE);
	lxl_array_t *a = lxl_array_create(p, 8, sizeof(lxl_int_t));
	for (i = 0; i < 8; ++i) {
		value = lxl_array_push(a);
		if (value != NULL) {
			*value = i + 10;
		}
	}

	//int o = 11;
	//lxl_array_del(a, (void *) &o, &lxl_intcmp);

	for (i = 0; i < a->nelts; ++i) {
		value = lxl_array_data(a, lxl_int_t, i);
		//k = (u_char *) a->elts + i * a->size;
		//lxl_log(LXL_LOG_DEBUG, 0, "%ld\n", *(lxl_int_t *) k);
		lxl_log(LXL_LOG_DEBUG, 0, "%ld\n", *value);
	}

	return 0;
}

lxl_int_t 
lxl_array_sample_ptr()
{
	lxl_int_t i, *value, **elt;

	lxl_pool_t *p = lxl_pool_create(LXL_DEFAULT_POOL_SIZE);
	lxl_array_t *a = lxl_array_create(p, 8, sizeof(lxl_int_t));
	for (i = 0; i < 8; ++i) {
		value = lxl_alloc(sizeof(lxl_int_t *));
		*value = i + 20;
		elt = lxl_array_push(a);
		if (elt != NULL) {
			*elt = value;
		}
	}


	for (i = 0; i < a->nelts; ++i) {
		elt = lxl_array_data(a, lxl_int_t *, i);
		lxl_log(LXL_LOG_DEBUG, 0, "%ld\n", **elt);
	}

	return 0;
}
