
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_TIMES_H_INCLUDE
#define LXL_TIMES_H_INCLUDE


#include <lxl_config.h>


#define 	lxl_sleep(s)	sleep(s)
#define 	lxl_msleep(ms)	usleep(ms * 1000)


void lxl_time_init(void);
void lxl_time_update(void);


extern volatile lxl_uint_t lxl_current_sec;
extern volatile lxl_uint_t lxl_current_msec;


#endif	/* LXL_TIMES_H_INCLUDE */
