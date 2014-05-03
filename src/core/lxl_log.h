
/*
 * Copyright (c) xianliang.li
 */


#ifndef LXL_LOG_H_INCLUDE
#define LXL_LOG_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_conf_file.h>


#define LXL_LOG_EMERG		0
#define LXL_LOG_ALERT		1
#define LXL_LOG_ERROR		2
#define LXL_LOG_WARN		3
#define LXL_LOG_INFO		4
#define LXL_LOG_DEBUG		5

#define LXL_LOG_DEBUG_CORE	0x0001
#define LXL_LOG_DEBUG_ALLOC 0x0002
#define LXL_LOG_DEBUG_EVENT 0x0004
#define LXL_LOG_DEBUG_DNS	0x0008
#define LXL_LOG_DEBUG_MAIL	0x0010
#define LXL_LOG_DEBUG_DFS	0x0020
#define LXL_LOG_DEBUG_ALL	0xffff

#define LXL_LOG_BUFFER		0
#define LXL_LOG_FLUSH		1

#define LXL_MAX_TIME_STR	24
#define LXL_MAX_ERR_STR		64
#define LXL_MAX_FTIME_STR	64
#define LXL_MAX_FMT_STR		64
#define LXL_MAX_MSG_STR		2048
#define LXL_MAX_BUF_STR		4096

//#define LXL_MAX_PATH		256


struct lxl_log_s {
	int			fd;

	lxl_uint_t	use_flush;
	lxl_uint_t  use_stderr;
	lxl_uint_t 	error_level;
	lxl_uint_t  debug_level;

	size_t 		buf_len;
	char		buf[LXL_MAX_BUF_STR];

	char		file[LXL_MAX_CONF_PATH];
};

typedef struct {
	lxl_uint_t  use_flush;
	lxl_uint_t  use_stderr;
	lxl_uint_t	error_level;
	lxl_uint_t  debug_level;

	lxl_str_t	file;
} lxl_log_conf_t;


int 	lxl_strerror_init(void);
char *	lxl_errlog_module_init_conf(lxl_cycle_t *cycle, void *conf);

lxl_log_t *	lxl_log_init(lxl_uint_t error_level, lxl_uint_t debug_level, lxl_uint_t flush, char *prefix);
#define 	lxl_log_destroy(log)	close((log)->fd)

#define	lxl_log_error(loglevel, err, args...)	\
	if (lxl_log.error_level >= loglevel)	lxl_log_core(loglevel, err, args) 

#define	lxl_log_error1(loglevel, err, args...)	\
	if (lxl_log.error_level >= loglevel)	lxl_log_core_flush(loglevel, err, args) 

#define	lxl_log_debug(loglevel, err, args...)	\
	if (lxl_log.debug_level & loglevel)		lxl_log_core(LXL_LOG_DEBUG, err, args) 

void 	lxl_log_core(lxl_uint_t loglevel, int err, const char *format, ...);
void 	lxl_log_core_flush(lxl_uint_t loglevel, int err, const char *format, ...);
void	lxl_log_stderr(int err, const char *fmt, ...);
void	lxl_log_flush(void);


extern lxl_log_t  lxl_log;


#endif	/* LXL_LOG_H_INCLUDE */
