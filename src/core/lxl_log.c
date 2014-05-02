
/*
 * Copyright (c) xianliang.li
 */


#include <lxl_times.h>
#include <lxl_cycle.h>
#include <lxl_files.h>
#include <lxl_process.h>
#include <lxl_log.h>


#define LXL_ERROR_LOG_PATH		"logs/error.log"

#define LXL_LOG_EMERG_STR		"emerg"
#define LXL_LOG_ALERT_STR		"alert"
#define LXL_LOG_ERROR_STR		"error"
#define LXL_LOG_WARN_STR		"warn"
#define	LXL_LOG_INFO_STR		"info"
#define LXL_LOG_DEBUG_STR		"debug"

#define LXL_LOG_DEBUG_CORE_STR 	"debug_core"
#define LXL_LOG_DEBUG_ALLOC_STR	"debug_alloc"
#define LXL_LOG_DEBUG_EVENT_STR	"debug_event"
#define LXL_LOG_DEBUG_DNS_STR	"debug_dns"
#define LXL_LOG_DEBUG_MAIL_STR	"debug_mail"
#define LXL_LOG_DEBUG_DFS_STR	"debug_dfs"
#define LXL_LOG_DEBUG_ALL_STR	"debug_all"

#define LXL_LOG_FLUSH_STR		"flush"
#define LXL_LOG_BUFFER_STR		"buffer"


#define lxl_memcpy(d, s, n)						\
	{											\
		size_t lxl_memcpy_n = (n);				\
		char *lxl_memcpy_d = (char *) (d);		\
		char *lxl_memcpy_s = (char *) (s);		\
		while (lxl_memcpy_n--) {				\
			*lxl_memcpy_d++ = *lxl_memcpy_s++;	\
		}										\
	}				


static inline void	lxl_log_write(size_t size);
static char *		lxl_error_log(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);

static void *		lxl_errlog_module_create_conf(lxl_cycle_t *cycle);
static char *		lxl_log_set_log(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
static int 			lxl_log_get_error_level(char *error_level);
static int 			lxl_log_get_debug_level(char *debug_level);
static int 			lxl_log_get_flush_flag(char *flush);


extern volatile 	lxl_uint_t   lxl_current_sec;

static 	lxl_uint_t  lxl_log_cache_sec;
static struct tm	lxl_log_tm;
static struct tm	lxl_log_cache_tm;
static char         lxl_log_time[LXL_MAX_TIME_STR];
static lxl_uint_t  	lxl_sys_nerr;
static char 	  **lxl_sys_errlist;
static char 	   *lxl_unknown_error = "Unknown error";
const char 			lxl_log_err_levels[][6] = {"emerg", "alert", "error", "warn ", "info ", "debug"};
//static char 	   *lxl_log_file = NULL;

lxl_log_t 			lxl_log;	


static lxl_command_t lxl_errlog_commands[] = {

	{lxl_string("error_log"),
	 LXL_MAIN_CONF|LXL_DIRECT_CONF|LXL_CONF_1MORE,
	 lxl_error_log,
     0,
	 0,
	 NULL},

	lxl_null_command
};

static lxl_core_module_t lxl_errlog_module_ctx = {
	lxl_string("errlog"),
	lxl_errlog_module_create_conf,
	NULL
};

lxl_module_t lxl_errlog_module = {
	0,
	0,
	(void *) &lxl_errlog_module_ctx,
	lxl_errlog_commands,
	LXL_CORE_MODULE,
	NULL,
	NULL
};


int 
lxl_strerror_init(void)
{
	lxl_uint_t err;
	size_t len;
	char *p, *msg;
	
	//lxl_sys_nerr = sys_nerr;
	lxl_sys_nerr = 134;
	len = lxl_sys_nerr * sizeof(char *);
	lxl_sys_errlist = malloc(len);
	if (lxl_sys_errlist == NULL) {
		goto failed;
	}

	for (err = 0; err < lxl_sys_nerr; ++err) {
		msg = strerror(err);
		len = strlen(msg);
		p = malloc(len + 1);
		if (p == NULL) {
			goto failed;
		}

		memcpy(p, msg, len);
		p[len] = '\0';
		lxl_sys_errlist[err] = p;
	}

	return 0;

failed:
	lxl_log_stderr(0, "malloc(%lu) failed (%d: %s)", len, errno, strerror(errno));

	return -1;
}

lxl_log_t *
lxl_log_init(lxl_uint_t error_level, lxl_uint_t debug_level, lxl_uint_t flush, char *prefix)
{
	char *name;
	size_t nlen, plen;
	
	lxl_log.use_flush = 1;
	lxl_log.use_stderr = 0;
	lxl_log.error_level = LXL_LOG_DEBUG;
	lxl_log.debug_level = LXL_LOG_DEBUG_ALL;
	lxl_log.file[0] = '\0';
	lxl_log.buf_len = 0;

	time_t t;
	lxl_log_cache_sec = t = time(NULL);
	localtime_r(&t, &lxl_log_tm);
	lxl_log_cache_tm = lxl_log_tm;
	snprintf(lxl_log_time, LXL_MAX_TIME_STR, "%4d/%02d/%02d %02d:%02d:%02d", lxl_log_tm.tm_year + 1900, 
			lxl_log_tm.tm_mon + 1, lxl_log_tm.tm_mday, lxl_log_tm.tm_hour, lxl_log_tm.tm_min, lxl_log_tm.tm_sec);

	name = LXL_ERROR_LOG_PATH;
	nlen = strlen(name);
	if (nlen == 0) {
		lxl_log.fd = STDERR_FILENO;
		lxl_log.use_stderr = 1;
		return &lxl_log;
	}

	if (!lxl_file_separator(name[0])) {
		if (prefix) {
			plen = strlen(prefix);
		} else {
			plen = strlen(LXL_PREFIX);
		}

		if (plen) {	
			if (!lxl_file_separator(prefix[plen - 1])) {
				snprintf(lxl_log.file, LXL_MAX_CONF_PATH, "%s/%s", prefix, name);
			} else {
				snprintf(lxl_log.file, LXL_MAX_CONF_PATH, "%s%s", prefix, name);
			}
		} else {
			snprintf(lxl_log.file, LXL_MAX_CONF_PATH, "%s", name);
		}
	}

	//fd = open(lxl_log_file, O_CREAT|O_WRONLY|O_TRUNC, 0666);	
	lxl_log.fd = open(lxl_log.file, O_CREAT|O_WRONLY|O_APPEND, 0666);	
	if (lxl_log.fd == -1) {
		lxl_log_stderr(errno, "open() failed", lxl_log.file);
		lxl_log.fd = STDERR_FILENO;
	}
	
	return &lxl_log;
}

void lxl_log_core(lxl_uint_t loglevel, int err, const char *format, ...)
{
	int n;
	va_list ap;
	size_t  fmt_len, msg_len;
	char *s, *p, errstr[LXL_MAX_ERR_STR], fmt[LXL_MAX_FMT_STR], msg[LXL_MAX_MSG_STR];

	if (lxl_log_cache_sec != lxl_current_sec) {
		time_t t = lxl_log_cache_sec = lxl_current_sec;
		//lxl_log_cache_tm = lxl_log_tm;
		localtime_r(&t, &lxl_log_tm);
		snprintf(lxl_log_time, LXL_MAX_TIME_STR, "%4d/%02d/%02d %02d:%02d:%02d", 
				lxl_log_tm.tm_year + 1900, lxl_log_tm.tm_mon + 1, lxl_log_tm.tm_mday, lxl_log_tm.tm_hour, lxl_log_tm.tm_min, lxl_log_tm.tm_sec);
	}

	n = snprintf(fmt, LXL_MAX_FMT_STR, "%s [%s] %d: ", lxl_log_time, lxl_log_err_levels[loglevel], lxl_pid);
	if (n > 0) {
		fmt_len = n;
	} else {
		fmt_len = 0;
	}

	memcpy(msg, fmt, fmt_len);
	msg_len = fmt_len;

	va_start(ap, format);
	n = vsnprintf(msg + msg_len, LXL_MAX_MSG_STR - msg_len, format, ap);
	va_end(ap);
	if (n > 0) {
		msg_len += n;
	}

	if (err) {
		if (msg_len > LXL_MAX_MSG_STR - LXL_MAX_ERR_STR) {
			p = &msg[LXL_MAX_MSG_STR - LXL_MAX_ERR_STR];
			*p++ = '.';
			*p++ = '.';
			*p++ = '.';
			msg_len = LXL_MAX_MSG_STR - LXL_MAX_ERR_STR + 3;
		}

		s = ((lxl_uint_t) err < lxl_sys_nerr) ? lxl_sys_errlist[err] : lxl_unknown_error;
		n = snprintf(errstr, LXL_MAX_ERR_STR, " (%d: %s)", err, s);
		if (n > 0) {
			memcpy(msg + msg_len, errstr, n);
			msg_len += n;
		}
	}

	msg[msg_len] = '\n';
	msg_len += 1;

	if (lxl_log.use_flush) {
		memcpy(lxl_log.buf, msg, msg_len);

		lxl_log_write((size_t) msg_len);
	} else {
		if (msg_len > (LXL_MAX_BUF_STR - lxl_log.buf_len)) {
			lxl_log_write((size_t) lxl_log.buf_len);

			memcpy(lxl_log.buf, msg, msg_len);
			lxl_log.buf_len = msg_len;
		} else {
			memcpy(lxl_log.buf + lxl_log.buf_len, msg, msg_len);
			lxl_log.buf_len += msg_len;
		}
	}
}

void lxl_log_core_flush(lxl_uint_t loglevel, int err, const char *format, ...)
{
	int n;
	va_list ap;
	size_t  fmt_len, msg_len;
	char *s, *p, errstr[LXL_MAX_ERR_STR], fmt[LXL_MAX_FMT_STR], msg[LXL_MAX_MSG_STR];

	if (lxl_log_cache_sec != lxl_current_sec) {
		time_t t = lxl_log_cache_sec = lxl_current_sec;
		localtime_r(&t, &lxl_log_tm);
		snprintf(lxl_log_time, LXL_MAX_TIME_STR, "%4d/%02d/%02d %02d:%02d:%02d", 
				lxl_log_tm.tm_year + 1900, lxl_log_tm.tm_mon + 1, lxl_log_tm.tm_mday, lxl_log_tm.tm_hour, lxl_log_tm.tm_min, lxl_log_tm.tm_sec);
	}

	n = snprintf(fmt, LXL_MAX_FMT_STR, "%s [%s] %d: ", lxl_log_time, lxl_log_err_levels[loglevel], lxl_pid);
	if (n > 0) {
		fmt_len = n;
	} else {
		fmt_len = 0;
	}

	memcpy(msg, fmt, fmt_len);
	msg_len = fmt_len;

	va_start(ap, format);
	n = vsnprintf(msg + msg_len, LXL_MAX_MSG_STR - msg_len, format, ap);
	va_end(ap);
	if (n > 0) {
		msg_len += n;
	}

	if (err) {
		if (msg_len > LXL_MAX_MSG_STR - LXL_MAX_ERR_STR) {
			p = &msg[LXL_MAX_MSG_STR - LXL_MAX_ERR_STR];
			*p++ = '.';
			*p++ = '.';
			*p++ = '.';
			msg_len = LXL_MAX_MSG_STR - LXL_MAX_ERR_STR + 3;
		}

		s = ((lxl_uint_t) err < lxl_sys_nerr) ? lxl_sys_errlist[err] : lxl_unknown_error;
		n = snprintf(errstr, LXL_MAX_ERR_STR, " (%d: %s)", err, s);
		if (n > 0) {
			memcpy(msg + msg_len, errstr, n);
			msg_len += n;
		}
	}

	msg[msg_len] = '\n';
	msg_len += 1;

	memcpy(lxl_log.buf + lxl_log.buf_len, msg, msg_len);
	lxl_log.buf_len += msg_len;

	lxl_log_write(lxl_log.buf_len);

	lxl_log.buf_len = 0;
}

static inline void 
lxl_log_write(size_t size)
{
	write(lxl_log.fd, lxl_log.buf, size);
	if (!lxl_log.use_stderr &&
		(lxl_log_tm.tm_mday != lxl_log_cache_tm.tm_mday
		|| lxl_log_tm.tm_mon != lxl_log_cache_tm.tm_mon
		|| lxl_log_tm.tm_year != lxl_log_cache_tm.tm_year)
	/*	|| lxl_log_tm.tm_sec != lxl_log.cache_tm_tm_sec */) {
		/*fprintf(stderr, "cache time %d%02d%02d, current time %d%02d%02d", lxl_log_cache_tm.tm_year, lxl_log_cache_tm.tm_mon, 
				lxl_log_cache_tm.tm_mday, lxl_log_cache_tm.tm_year, lxl_log_cache_tm.tm_mon, lxl_log_cache_tm.tm_mday);*/
		int fd;
		char newpath[256];

		snprintf(newpath, sizeof(newpath), "%s-%d%02d%02d", 
				lxl_log.file, lxl_log_cache_tm.tm_year + 1900, lxl_log_cache_tm.tm_mon + 1, lxl_log_cache_tm.tm_mday);

		if (access(newpath, F_OK) != 0) {	/* file not exist */
			close(lxl_log.fd);
			rename(lxl_log.file, newpath);	/* other process write to newpath */
		} else {
			close(lxl_log.fd);
		}

		fd = open(lxl_log.file, O_CREAT|O_WRONLY|O_APPEND, 0666);
		if (fd != -1) {
			lxl_log.fd = fd;
		} 
	}

	lxl_log_cache_tm = lxl_log_tm;
}

void 
lxl_log_stderr(int err, const char *fmt, ...)
{
	int n;
	size_t msg_len;
	va_list args;
	char *p, *s, errstr[LXL_MAX_ERR_STR], msg[LXL_MAX_MSG_STR];

	memcpy(msg, "speed: ", 7);
	msg_len = 7;

	va_start(args, fmt);
	n = vsnprintf(msg + msg_len, LXL_MAX_MSG_STR - msg_len, fmt, args);
	va_end(args);
	if (n > 0) {
		msg_len += n;
	} 

	if (err) {
		if (msg_len > LXL_MAX_MSG_STR - LXL_MAX_ERR_STR) {
			p = &msg[LXL_MAX_MSG_STR - LXL_MAX_ERR_STR];
			*p++ = '.';
			*p++ = '.';
			*p++ = '.';
			msg_len = LXL_MAX_MSG_STR - LXL_MAX_ERR_STR + 3;
		}

		s = ((lxl_uint_t) err < lxl_sys_nerr) ? lxl_sys_errlist[err] : lxl_unknown_error;
		n = snprintf(errstr, LXL_MAX_ERR_STR, " (%d: %s)", err, s);
		if (n > 0) {
			memcpy(msg + msg_len, errstr, n);
			msg_len += n;
		}
	}

	msg[msg_len] = '\n';
	msg_len += 1;

	write(STDERR_FILENO, msg, msg_len);
}

static char *		
lxl_error_log(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	return lxl_log_set_log(cf, cmd, conf);
}

static void *		
lxl_errlog_module_create_conf(lxl_cycle_t *cycle)
{
	lxl_log_conf_t *lcf;

	lcf = lxl_pcalloc(cycle->pool, sizeof(lxl_core_conf_t));
	if (lcf == NULL) {
		return NULL;
	}

	lcf->use_flush = LXL_CONF_UNSET;
	lcf->use_stderr = LXL_CONF_UNSET;
	lcf->error_level = LXL_CONF_UNSET;
	lcf->debug_level = LXL_CONF_UNSET;
	lxl_str_null(&lcf->file);

	return lcf;
}

char *		
lxl_errlog_module_init_conf(lxl_cycle_t *cycle, void *conf)
{	
	lxl_log_conf_t *lcf = (lxl_log_conf_t *) conf;

	int fd;
	
	fd = cycle->log->fd;
	if (!lcf->use_stderr) {
		lxl_log_debug(LXL_LOG_DEBUG_CORE, 0, "open log file: %s", lcf->file.data);
		cycle->log->fd = open(lcf->file.data, O_CREAT|O_WRONLY|O_APPEND, 0666);
		if (cycle->log->fd == -1) {
			lxl_log_error(LXL_LOG_EMERG, errno, "open(%s) failed", lcf->file.data);
			return LXL_CONF_ERROR;
		}
	} else {
		cycle->log->fd = STDERR_FILENO;
	}

	if (close(fd) == -1) {
		lxl_log_error(LXL_LOG_ALERT, errno, "close() build-in log failed");
	}

	cycle->log->use_flush = lcf->use_flush;
	cycle->log->use_stderr = lcf->use_stderr;
	cycle->log->error_level = lcf->error_level;
	cycle->log->debug_level = lcf->debug_level;
	memcpy(cycle->log->file, lcf->file.data, lcf->file.len + 1);

	return LXL_CONF_OK;
}

static char *
lxl_log_set_log(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	lxl_log_conf_t *lcf = (lxl_log_conf_t *) conf;

	int n, use_flush, set_error_level, set_flush_flag;
	lxl_uint_t i, nelts, error_level, debug_level;
	lxl_str_t *value, name;
	lxl_log_t *log;
	lxl_cycle_t *cycle;

	cycle = cf->cycle;
	log = cycle->log;
	if (lcf->use_flush != LXL_CONF_UNSET) {
		return "is duplicate";
	}

	nelts = lxl_array_nelts(cf->args);
	value = lxl_array_elts(cf->args);
	if (value[1].len == 6 && memcmp(value[1].data, "stderr", 6) == 0) {
		lcf->use_stderr = 1;
	} else {
		name = value[1];
		if (lxl_get_full_name(cycle->pool, &cycle->prefix, &name) == -1) {
			return LXL_CONF_ERROR;
		}

		if (name.len > LXL_MAX_CONF_PATH) {
			lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "error_log path %s is to long", name.data);
			return LXL_CONF_ERROR;
		}

		lcf->file.len = name.len;
		lcf->file.data = name.data;
		lcf->use_stderr = 0;
	}

	if (nelts == 2) {
		log->use_flush = 0;
		log->error_level = LXL_LOG_ERROR;
		log->debug_level = 0;
		return LXL_CONF_OK;
	}
	
	set_error_level = 0;
	set_flush_flag = 0;
	use_flush = 0;
	error_level = LXL_LOG_ERROR;
	debug_level = 0;
	for (i = 2; i < nelts; ++i) {
		n = lxl_log_get_error_level(value[i].data);
		if (n != -1) {
			if (set_error_level) {
				lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "duplicat log level \"%s\"", value[i].data);
				return LXL_CONF_ERROR;
			}

			set_error_level = 1;
			error_level = (lxl_uint_t) n;
			continue;
		}

		n = lxl_log_get_debug_level(value[i].data);
		if (n != -1) {
			debug_level |= (lxl_uint_t) n;
			continue;
		}

		n = lxl_log_get_flush_flag(value[i].data);
		if (n != -1) {
			if (set_flush_flag) {
				lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "duplicat log flush flag \"%s\"", value[i].data);
				return LXL_CONF_ERROR;
			}

			set_flush_flag = 1;
			use_flush = n;
			continue;
		}

		lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "invalid log arguments \"%s\"", value[i].data);
		return LXL_CONF_ERROR;
	}

	lcf->use_flush = use_flush;
	lcf->error_level = error_level;
	lcf->debug_level = debug_level;

	return LXL_CONF_OK;
}

static int 
lxl_log_get_error_level(char *error_level)
{
	if (strcmp(LXL_LOG_EMERG_STR, error_level) == 0) {
		return LXL_LOG_EMERG;	
	} else if (strcmp(LXL_LOG_ERROR_STR, error_level) == 0) {
		return LXL_LOG_ERROR;
	} else if (strcmp(LXL_LOG_WARN_STR, error_level) == 0) {
		return LXL_LOG_WARN;
	} else if (strcmp(LXL_LOG_INFO_STR, error_level) == 0) {
		return LXL_LOG_INFO;
	} else if (strcmp(LXL_LOG_DEBUG_STR, error_level) == 0) {
		return LXL_LOG_DEBUG;
	}  else {
		return -1;
	}
}

static int 
lxl_log_get_debug_level(char *debug_level)
{
	if (strcmp(LXL_LOG_DEBUG_CORE_STR, debug_level) == 0) {
		return LXL_LOG_DEBUG_CORE;	
	} else if (strcmp(LXL_LOG_DEBUG_ALLOC_STR, debug_level) == 0) {
		return LXL_LOG_DEBUG_ALLOC;
	} else if (strcmp(LXL_LOG_DEBUG_EVENT_STR, debug_level) == 0) {
		return LXL_LOG_DEBUG_EVENT;
	} else if (strcmp(LXL_LOG_DEBUG_DNS_STR, debug_level) == 0) {
		return LXL_LOG_DEBUG_DNS;
	} else if (strcmp(LXL_LOG_DEBUG_MAIL_STR, debug_level) == 0) {
		return LXL_LOG_DEBUG_MAIL;
	} else if (strcmp(LXL_LOG_DEBUG_DFS_STR, debug_level) == 0) {
		return LXL_LOG_DEBUG_DFS;
	} else if (strcmp(LXL_LOG_DEBUG_ALL_STR, debug_level) == 0) {
		return LXL_LOG_DEBUG_ALL;
	}  else {
		return -1;
	}
}

static int 			
lxl_log_get_flush_flag(char *flush)
{
	if (strcmp(LXL_LOG_FLUSH_STR, flush) == 0) {
		return 1;
	} else if (strcmp(LXL_LOG_BUFFER_STR, flush) == 0) {
		return 0;
	} else {
		return 1;
	}
}
