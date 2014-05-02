
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_CONF_FILE_INCLUDE
#define LXL_CONF_FILE_INCLUDE


#include <lxl_core.h>
#include <lxl_config.h>
#include <lxl_buf.h>
#include <lxl_file.h>
#include <lxl_array.h>
#include <lxl_string.h>


#define LXL_CONF_NOARGS			0x00000001
#define LXL_CONF_TAKE1			0x00000002
#define LXL_CONF_TAKE2			0x00000004
#define LXL_CONF_TAKE3			0x00000008
#define LXL_CONF_TAKE4			0x00000010
#define LXL_CONF_TAKE5			0x00000020
#define LXL_CONF_TAKE6			0x00000040
#define LXL_CONF_TAKE7			0x00000080

#define LXL_CONF_TAKE12			(LXL_CONF_TAKE1|LXL_CONF_TAKE2)
#define LXL_CONF_TAKE123		(LXL_CONF_TAKE1|LXL_CONF_TAKE2|LXL_CONF_TAKE3)

#define LXL_CONF_BLOCK			0x00000100
#define LXL_CONF_FLAG			0x00000200
#define LXL_CONF_ANY			0x00000400
#define LXL_CONF_1MORE			0x00000800
#define LXL_CONF_2MORE			0x00001000

#define LXL_CONF_MAX_ARGS		8

#define LXL_DIRECT_CONF			0x00010000

#define LXL_MAIN_CONF			0x01000000

#define LXL_CONF_UNSET			-1
#define LXL_CONF_UNSET_PTR		(void *) -1
#define LXL_CONF_UNSET_UINT		(lxl_uint_t) -1
#define LXL_CONF_UNSET_SIZE		(size_t) -1
#define LXL_CONF_UNSET_MSEC		(lxl_msec_t) -1

#define LXL_CONF_OK				NULL
#define LXL_CONF_ERROR			(void *) -1

#define LXL_CONF_BLOCK_START	1
#define LXL_CONF_BLOCK_DONE		2
#define LXL_CONF_FILE_DONE		3

#define LXL_CORE_MODULE      	0x45524F43  /* "CORE" */

#define LXL_MAX_CONF_PATH		256


struct lxl_command_s {
	lxl_str_t 	name;
	lxl_uint_t	type;
	char 	   *(*set)(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
	lxl_uint_t	conf;
	lxl_uint_t 	offset;
	void 	   *post;
};


#define lxl_null_command	{ lxl_null_string, 0, NULL, 0, 0, NULL }


struct lxl_module_s {
	lxl_uint_t		index;
	lxl_uint_t 		ctx_index;
	void		   *ctx;
	lxl_command_t  *commands;
	lxl_uint_t		type;
	int 			(*init_module)(lxl_cycle_t *cycle);
	int				(*init_process)(lxl_cycle_t *cycle);
};

typedef struct {
	lxl_str_t 	name;
	void 	   *(*create_conf) (lxl_cycle_t *cycle);
	char       *(*init_conf) (lxl_cycle_t *cycle, void *conf);
} lxl_core_module_t;

typedef struct {
	lxl_file_t file;
	lxl_buf_t *buffer;
	lxl_uint_t line;
} lxl_conf_file_t;

struct lxl_conf_s {
	char 	   	       *name;
	lxl_array_t    	   *args;

	lxl_cycle_t		   *cycle;
	lxl_pool_t 	       *pool;
	lxl_pool_t         *temp_pool;
	lxl_conf_file_t    *conf_file;

	void 	   	   	   *ctx;
	lxl_uint_t 		 	module_type;
	lxl_uint_t 			cmd_type;
};


#define lxl_get_conf(conf_ctx, module)	conf_ctx[module.index]

#define lxl_conf_init_value(conf, default)						\
	if (conf == LXL_CONF_UNSET) {								\
		conf = default;											\
	}

#define lxl_conf_init_ptr_value(conf, default)					\
	if (conf == LXL_CONF_UNSET_PTR) {							\
		conf = default;											\
	}

#define lxl_conf_init_uint_value(conf, default)					\
	if (conf == LXL_CONF_UNSET_UINT) {							\
		conf = default;											\
	}

#define lxl_conf_init_size_value(conf, default)					\
	if (conf == LXL_CONF_UNSET_SIZE) {							\
		conf = default;											\
	}

#define lxl_conf_merge_value(conf, prev, default)				\
	if (conf == LXL_CONF_UNSET) {								\
		conf = (prev == LXL_CONF_UNSET) ? default : prev;		\
	}

#define lxl_conf_merge_ptr_value(conf, prev, default)			\
	if (conf == LXL_CONF_UNSET_PTR) {							\
		conf = (prev == LXL_CONF_UNSET_PTR) ? default : prev;	\
	}

#define lxl_conf_merge_uint_value(conf, prev, default)			\
	if (conf == LXL_CONF_UNSET_UINT) {							\
		conf = (prev == LXL_CONF_UNSET_UINT) ? default : prev;	\
	}

#define lxl_conf_merge_size_value(conf, prev, default)			\
	if (conf == LXL_CONF_UNSET_SIZE) {							\
		conf = (prev == LXL_CONF_UNSET_SIZE) ? default : prev;	\
	}


char *lxl_conf_parse(lxl_conf_t *cf, lxl_str_t *filename);

void lxl_conf_log_error(lxl_uint_t level, lxl_conf_t *cf, int err, const char *fmt, ...);

char *lxl_conf_set_flag_slot(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
char *lxl_conf_set_str_slot(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
char *lxl_conf_set_num_slot(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
char *lxl_conf_set_size_slot(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
char *lxl_conf_set_msec_slot(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
char *lxl_conf_set_sec_slot(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);


extern lxl_uint_t		lxl_max_module;
extern lxl_module_t 	*lxl_modules[];


#endif	/* LXL_CONF_FILE_INCLUDE */
