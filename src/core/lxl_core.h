
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_CORE_H_INCLUDE
#define LXL_CORE_H_INCLUDE


#define LXL_PREFIX   		""
#define LXL_CONF_PATH		"conf/speed.conf"

#define LF					10
#define CR					13			
#define CRLF				"\x0d\x0a"

#define LXL_OK 				0
#define LXL_ERROR 			-1
#define LXL_EAGAIN 			-2

#define lxl_abs(value)       (((value) >= 0) ? (value) : - (value))
#define lxl_max(val1, val2)  ((val1 < val2) ? (val2) : (val1))
#define lxl_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))


typedef struct lxl_module_s			lxl_module_t;
typedef struct lxl_command_s		lxl_command_t;
typedef struct lxl_conf_s			lxl_conf_t;
typedef struct lxl_log_s 			lxl_log_t;
typedef struct lxl_pool_s   		lxl_pool_t;
typedef struct lxl_file_s			lxl_file_t;
typedef struct lxl_event_s 			lxl_event_t;
typedef struct lxl_connection_s 	lxl_connection_t;
typedef struct lxl_cycle_s			lxl_cycle_t;


#endif	/* LXL_CORE_H_INCLUDE */
