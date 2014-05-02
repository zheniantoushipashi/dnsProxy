/*
 * Copyright (c) xianliang.li
 * Copyright (c) alibaba, inc.
 */


#ifndef LXL_UTIL_H_INCLUDE
#define LXL_UTIL_H_INCLUDE


#include <errno.h>
#include <sys/types.h>

#include "lxl_log.h"
//#include "conf.h"


int		lxl_daemon(void);
int 	lxl_setr_nofile(size_t rlim);
int 	lxl_setr_core(void);
int 	lxl_is_obs_pathname(char *pathname);
int 	lxl_is_filename(char *filename);
int 	lxl_mkdir(char *dir);
//void	lxl_worker_process_init(lxl_log_t *log, conf_core_t *core);


#endif	/* LXL_UTIL_H_INCLUDE */
