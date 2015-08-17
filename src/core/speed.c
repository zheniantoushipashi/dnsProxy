
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_log.h>
#include <lxl_core.h>
#include <lxl_times.h>
#include <lxl_palloc.h>
#include <lxl_cycle.h>
#include <lxl_conf_file.h>
#include <lxl_files.h>
#include <lxl_process.h>


static int	 lxl_get_options(int argc, char *argv[]);
static int 	 lxl_process_options(lxl_cycle_t *cycle);
static void *lxl_core_module_create_conf(lxl_cycle_t *cycle);
static char *lxl_core_module_init_conf(lxl_cycle_t *cycle, void *conf);
static char *lxl_set_worker_process(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
static int	 lxl_daemon(void);


static lxl_command_t lxl_core_commands[] = {

	{ lxl_string("daemon"),
	  LXL_MAIN_CONF|LXL_DIRECT_CONF|LXL_CONF_FLAG,
	  lxl_conf_set_flag_slot,
	  0,
	  offsetof(lxl_core_conf_t, daemon),
	  NULL },
	
	{ lxl_string("worker_process"),
	  LXL_MAIN_CONF|LXL_DIRECT_CONF|LXL_CONF_TAKE1,
	  lxl_set_worker_process,
	  0, 
	  0,
	  NULL },

	{ lxl_string("worker_rlimit_nofile"),
	  LXL_MAIN_CONF|LXL_DIRECT_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_num_slot,
      0,
	  offsetof(lxl_core_conf_t, rlimit_nofile),
	  NULL },
	
	{ lxl_string("working_directory"),
	  LXL_MAIN_CONF|LXL_DIRECT_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_str_slot,
	  0,
	  offsetof(lxl_core_conf_t, working_directory),
	  NULL },

	/*{ lxl_string("error_log"),
	},
	
	{ lxl_string("debug_log_level"),
	},*/

	lxl_null_command
};

static lxl_core_module_t lxl_core_module_ctx = {
	lxl_string("core"),
	lxl_core_module_create_conf,
	lxl_core_module_init_conf
};

lxl_module_t lxl_core_module = {
	0,
	0,
	(void *) &lxl_core_module_ctx,
	lxl_core_commands,
	LXL_CORE_MODULE,
	NULL,
	NULL
};

extern lxl_module_t lxl_errlog_module;
extern lxl_module_t lxl_events_module;
extern lxl_module_t lxl_event_core_module;
extern lxl_module_t lxl_epoll_module;
extern lxl_module_t lxl_dns_module;
extern lxl_module_t lxl_dns_core_module;

lxl_module_t *lxl_modules[] = {
	&lxl_core_module,
	&lxl_errlog_module,
	&lxl_events_module,
	&lxl_event_core_module,
	&lxl_epoll_module,
	&lxl_dns_module,
	&lxl_dns_core_module,
	NULL
};

lxl_uint_t lxl_max_module;

static lxl_uint_t 	lxl_show_help;
static lxl_uint_t 	lxl_show_version;
static char    	   *lxl_prefix;
static char        *lxl_conf_file;


int main(int argc, char *argv[])
{

	fprintf(stderr, "%s\n", "sdfsfsdfsdf");
	lxl_uint_t i;
	lxl_log_t *log;
	lxl_cycle_t *cycle, init_cycle;
	lxl_core_conf_t *ccf;

	if (lxl_strerror_init() == -1) {
		return 1;
	}

	if (lxl_get_options(argc, argv) != 0) {
		return 1;
	}
	
	if (lxl_show_version) {
		fprintf(stderr, "speed version: speed/0.1.0\n");

		if (lxl_show_help) {
			fprintf(stderr, 
				"Usage: speed [-?hv] [-c filename]\n\n"
				"Options: \n"
				" -?,-h         : this help and exit\n"
				" -v            : show version and exit\n"
				" -c filename   : set configuration file (default " LXL_CONF_PATH ")\n"
				);
		}

		return 0;
	}

	lxl_time_init();
	lxl_pid = getpid();

	log = lxl_log_init(LXL_LOG_DEBUG, LXL_LOG_DEBUG_ALL, LXL_LOG_FLUSH, NULL);
	if (log == NULL) {
		return 1;
	}
	
	memset(&init_cycle, 0x00, sizeof(lxl_cycle_t));
	init_cycle.log = log;
	lxl_cycle = &init_cycle;

	init_cycle.pool = lxl_create_pool(1024);
	if (init_cycle.pool == NULL) {
		return 1;
	}

	if (lxl_process_options(lxl_cycle) == -1) {
		return 1;
	}

	lxl_max_module = 0;
	for (i = 0; lxl_modules[i]; ++i) {
		lxl_modules[i]->index = i;
		++lxl_max_module;
	}

	cycle = lxl_init_cycle(&init_cycle);
	if (cycle == NULL) {
		return 1;
	}

	lxl_cycle = cycle;

	ccf = (lxl_core_conf_t *) lxl_get_conf(cycle->conf_ctx, lxl_core_module);

	if (ccf->daemon) {
		if (lxl_daemon() != 0) {
			return 1;
		}
	}

	lxl_master_process_cycle(lxl_cycle);

	return 0;
}

static int	 
lxl_get_options(int argc, char *argv[])
{
	char *p;
	lxl_int_t i;

	for (i = 1; i < argc; ++i) {
		p = argv[i];
		if (*p++ != '-') {
			lxl_log_stderr(0, "invalid option: %s", argv[i]);
			return -1;
		}

		while (*p) {
			switch (*p++) {

			case '?':
			case 'h':
				lxl_show_version = 1;
				lxl_show_help = 1;
				break;

			case 'v':
				lxl_show_version = 1;
				break;
			
			case 'c':
				if (*p) {
					lxl_conf_file = p;
					goto next;
				}

				if (argv[++i]) {
					lxl_conf_file = argv[i];
					goto next;
				}

				lxl_log_stderr(0, "option -c request file name");
				return -1;

			default:
				lxl_log_stderr(0, "invalid option: %c", *(p - 1));
				return -1;
			}
		}

	next:

		continue;
	}	

	return 0;
}

static int 	 
lxl_process_options(lxl_cycle_t *cycle)
{
	char path[LXL_MAX_CONF_PATH];
	char *p;
	size_t len;

	if (lxl_prefix) {
		len = strlen(lxl_prefix);
		if (!lxl_file_separator(lxl_prefix[len - 1])) {
			p = lxl_pnalloc(cycle->pool, len + 2);
			if (p == NULL) {
				return -1;
			}

			memcpy(p, lxl_prefix, len + 1);
			p[len] = '/';
			++len;
			p[len] = '\0';
		} else {
			p = lxl_prefix;
		}
	} else {
		if (getcwd(path, LXL_MAX_CONF_PATH) == NULL) {
			lxl_log_stderr(errno, "[emerg]: getcwd failed");
			return -1;
		}

		len = strlen(path);
		if (!lxl_file_separator(path[len - 1])) {
			if (len + 1 >= LXL_MAX_CONF_PATH) {
				lxl_log_stderr(0, "[emerg]: prefix path %lu to long", len);
				return -1;
			}

			p = lxl_pnalloc(cycle->pool, len + 2);
			if (p == NULL) {
				return -1;
			}

			memcpy(p, path, len);
			p[len] = '/';
			++len;
			p[len] = '\0';
		} else {
			p = lxl_pnalloc(cycle->pool, len + 1);
			if (p == NULL) {
				return -1;
			}

			memcpy(p, path, len + 1);
		}
	}
	cycle->prefix.len = len;
	cycle->prefix.data = p;

	if (lxl_conf_file) {
		len = strlen(lxl_conf_file);
		p =  lxl_pnalloc(cycle->pool, len + 1);
		if (p == NULL) {
			return -1;
		}
		
		memcpy(p, lxl_conf_file, len + 1);
	} else {
		len = strlen(LXL_CONF_PATH);
		p =  lxl_pnalloc(cycle->pool, len + 1);
		if (p == NULL) {
			return -1;
		}
		
		memcpy(p, LXL_CONF_PATH, len + 1);
	}
	cycle->conf_file.len = len;
	cycle->conf_file.data = p;

	return 0;
}

static void *
lxl_core_module_create_conf(lxl_cycle_t *cycle)
{
	lxl_core_conf_t *ccf;

	ccf = lxl_pcalloc(cycle->pool, sizeof(lxl_core_conf_t));
	if (ccf == NULL) {
		return NULL;
	}

	ccf->daemon = LXL_CONF_UNSET;
	ccf->worker_process = LXL_CONF_UNSET;

	ccf->rlimit_nofile = LXL_CONF_UNSET;

	ccf->priority = 0;
	ccf->cpu_affinity_n = 0;
	ccf->cpu_affinity = NULL;
	ccf->username = NULL;
	lxl_str_null(&ccf->working_directory);

	return ccf;
}

static char *
lxl_core_module_init_conf(lxl_cycle_t *cycle, void *conf)
{
	lxl_core_conf_t *ccf = conf;

	lxl_conf_init_value(ccf->daemon, 1);
	lxl_conf_init_value(ccf->worker_process, 1);

	return NULL;
}

static char *
lxl_set_worker_process(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	lxl_str_t		*value;
	lxl_core_conf_t *ccf;
	
	ccf = (lxl_core_conf_t *) conf;
	if (ccf->worker_process != LXL_CONF_UNSET) {
		return "is duplicate";
	}

	value = (lxl_str_t *) lxl_array_elts(cf->args);
	if (value[1].len == 4 && memcmp(value[1].data, "auto", 4) == 0) {
		//ccf->worker_porcess = lxl_ncpu;
		ccf->worker_process = 4;
	}

	ccf->worker_process = atoi(value[1].data);
	if (ccf->worker_process <= 0) {
		return "invalid value";
	}

	return LXL_CONF_OK;
}

static int	 
lxl_daemon(void)
{
	int fd;

	switch (fork()) {
	case -1:
		lxl_log_error(LXL_LOG_EMERG, errno, "fork failed");
		return -1;

	case 0:
		break;

	default:
		exit(0);
	}

	lxl_pid = getpid();

	if (setsid() == -1) {
		lxl_log_error(LXL_LOG_EMERG, errno, "setsid() failed");
		return -1;
	}

	umask(0);
	
	fd = open("/dev/null", O_RDWR);
	if (fd == -1) {
		lxl_log_error(LXL_LOG_EMERG, errno, "open(/dev/null) failed");
		return -1;
	}

	if (dup2(fd, STDIN_FILENO) == -1) {
		lxl_log_error(LXL_LOG_EMERG, errno, "dup2(STDIN_FILENO) failed");
		return -1;
	}

	if (dup2(fd, STDOUT_FILENO) == -1) {
		lxl_log_error(LXL_LOG_EMERG, errno, "dup2(STDOUT_FILENO) failed");
		return -1;
	}

#if 0
	if (dup2(fd, STDERROR_FILENO) == -1) {
		lxl_log_error(LXL_LOG_EMERG, errno, "dup2(STDERROR_FILENO) failed");
		return -1;
	}
#endif 

	if (fd > STDERR_FILENO) {
		if (close(fd) == -1) {
			lxl_log_error(LXL_LOG_EMERG, errno, "close failed");
			return -1;
		}
	}
	
	return 0;
}
