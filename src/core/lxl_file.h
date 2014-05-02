
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_FILE_H_INCLUDE
#define LXL_FILE_H_INCLUDE


#include <lxl_core.h>
#include <lxl_config.h>
#include <lxl_string.h>


struct lxl_file_s {
	int 		fd;
	lxl_str_t 	name;
	struct stat info;

	off_t 		offset;
	off_t 		sys_offset;
	//unsigned 
};


int lxl_get_full_name(lxl_pool_t *pool, lxl_str_t *prefix, lxl_str_t *name);


#endif	/*	LXL_FILE_H_INCLUDE	*/
