
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_CYCLE_H_INCLUDE
#define LXL_CYCLE_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_array.h>
#include <lxl_string.h>


#define LXL_UDP		1


struct lxl_cycle_s {
	void			****conf_ctx;
	lxl_pool_t 		   *pool;

	lxl_log_t		   *log;

	lxl_int_t			worker_process;

	lxl_array_t 		listening;
	size_t 				buffer_size;
	void			   *buffers;
	lxl_uint_t 			connection_n;
	lxl_uint_t 			free_connection_n;
	lxl_connection_t   *connections;
	lxl_connection_t   *free_connections;
	lxl_event_t 	   *read_events;
	lxl_event_t 	   *write_events;

	lxl_str_t			prefix;
	lxl_str_t 			conf_file;
	char 			   *hostname;
	/*lxl_str_t			conf_file;
	lxl_str_t 			hostname;*/
};

typedef struct {
	//lxl_flag_t	daemon;
	lxl_int_t	daemon;
	lxl_int_t 	worker_process;

	lxl_int_t	rlimit_nofile;

	lxl_int_t 	priority;

	lxl_uint_t  cpu_affinity_n;
	lxl_uint_t *cpu_affinity;

	char 	   *username;
	/*lxl_uid_t	uid;
	lxl_gid_t	gid;*/
	lxl_str_t	working_directory;
} lxl_core_conf_t;


extern lxl_cycle_t *lxl_cycle;
extern lxl_module_t lxl_core_module;
extern lxl_module_t lxl_errlog_module;


lxl_cycle_t *lxl_init_cycle(lxl_cycle_t *cycle);


#endif	/* LXL_CYCLE_H_INCLUDE */
