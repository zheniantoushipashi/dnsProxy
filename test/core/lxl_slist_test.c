
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_log.h>
#include <lxl_slist.h>
#include <lxl_alloc.h>


typedef struct {
	lxl_slist_t slist;
	lxl_int_t x;
} lxl_slist_test_t;


lxl_int_t lxl_slist_sample(lxl_uint_t n);


int 
main(int argc, char *argv[])
{
	lxl_log_t *log;

	log = lxl_log_init(LXL_LOG_DEBUG, LXL_LOG_FLUSH);
	lxl_slist_sample(10);

	return 0;
}

lxl_int_t
lxl_slist_sample(lxl_uint_t n)
{
	lxl_int_t i;
	lxl_slist_t head, *l, *next;
	lxl_slist_test_t *test;

	lxl_slist_init(&head);
	for (i = 0; i < n; ++i) {
		test = lxl_alloc(sizeof(lxl_slist_test_t));
		test->x = i;
		lxl_slist_add_head(&head, &(test->slist));
	}

	for (l = lxl_slist_next(&head); l != lxl_slist_sentinel(); l = next) {
		next = lxl_slist_next(l);
		test = lxl_slist_data(l, lxl_slist_test_t, slist);
		lxl_log(LXL_LOG_DEBUG, 0, "%lu", test->x);
	}

	return 0;
}
