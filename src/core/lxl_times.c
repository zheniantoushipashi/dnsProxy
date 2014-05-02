
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
//#include <lxl_log.h>
#include <lxl_times.h>


#define  LXL_CACHE_LOG_TIME_LEN	24

//volatile lxl_uint_t 	lxl_cache_sec;
volatile lxl_uint_t 	lxl_current_sec;
volatile lxl_uint_t 	lxl_current_msec;
//volatile struct timeval lxl_current_tv;
//volatile char			lxl_log_time[LXL_CACHE_LOG_TIME_LEN];


void 
lxl_time_init(void)
{
	struct timeval tv;
	
	gettimeofday(&tv, NULL);

	//lxl_cache_sec = 0;
	lxl_current_sec = tv.tv_sec;
	lxl_current_msec = tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void
lxl_time_update()
{
	struct timeval tv;
#if 0
	lxl_log_debug(LXL_LOG_DEBUG_CORE, 0, "time update old:%lu %lu", lxl_current_sec, lxl_current_msec);
#endif 
	//lxl_cache_sec = lxl_current_sec;
	gettimeofday(&tv, NULL);
	lxl_current_sec = tv.tv_sec;
	lxl_current_msec = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#if 0
	lxl_log_debug(LXL_LOG_DEBUG_CORE, 0, "time update new:%lu %lu", lxl_current_sec, lxl_current_msec);
#endif
}
