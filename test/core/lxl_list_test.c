
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_log.h>
#include <lxl_list.h>
#include <lxl_alloc.h>


typedef struct {
	lxl_list_t list;
	lxl_int_t x;
} lxl_list_test_t;


lxl_int_t lxl_list_sample(lxl_uint_t n);
lxl_int_t lxl_list_sample_merge(void);


int 
main(int argc, char *argv[])
{
	lxl_log_t *log;

	log = lxl_log_init(LXL_LOG_DEBUG, LXL_LOG_FLUSH);
	lxl_list_sample(10);
	lxl_list_sample_merge();

	return 0;
}

lxl_int_t
lxl_list_sample(lxl_uint_t n)
{
	lxl_int_t i;
	lxl_list_t head, *l;
	lxl_list_test_t *test;

	lxl_log(LXL_LOG_DEBUG, 0, "lxl list sample");
	lxl_list_init(&head);
	for (i = 0; i < n; ++i) {
		test = lxl_alloc(sizeof(lxl_list_test_t));
		test->x = i;
		//lxl_list_add_tail(&head, &(test->list));
		lxl_list_add_head(&head, &(test->list));
	}

	for (l = lxl_list_next(&head); l != lxl_list_sentinel(&head); l = lxl_list_next(l)) {
		test = lxl_list_data(l, lxl_list_test_t, list);
		lxl_log(LXL_LOG_DEBUG, 0, "%lu", test->x);
	}

	return 0;
}

lxl_int_t 
lxl_list_sample_merge(void)
{
	lxl_int_t i;
	lxl_list_t head1, head2, *list;
	lxl_list_test_t *test;

	lxl_log(LXL_LOG_DEBUG, 0, "lxl list sample merge");
	lxl_list_init(&head1);
	for (i = 0; i < 10; ++i) {
		test = lxl_alloc(sizeof(lxl_list_test_t));
		test->x = i;
		lxl_list_add_tail(&head1, &(test->list));
	}

	lxl_list_init(&head2);
	for (i = 10; i < 20; ++i) {
		test = lxl_alloc(sizeof(lxl_list_test_t));
		test->x = i;
		lxl_list_add_tail(&head2, &(test->list));
	}
	
	lxl_list_merge(&head1, &head2);

	for (list = lxl_list_next(&head1); list != lxl_list_sentinel(&head1); list = lxl_list_next(list)) {
		test = lxl_list_data(list, lxl_list_test_t, list);
		lxl_log(LXL_LOG_DEBUG, 0, "%lu", test->x);
	}

	return 0;
}
