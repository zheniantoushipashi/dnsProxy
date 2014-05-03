
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_log.h>
#include <lxl_times.h>
#include <lxl_connection.h>
#include <lxl_conf_file.h>
#include <lxl_socket.h>
//#include <lxl_dns.h>
#include <lxl_cycle.h>


#define LXL_DNS_PORT	1053
//#define LXL_DNS_PORT	53


static void lxl_destroy_cycle_pools(lxl_conf_t *conf);


lxl_cycle_t 		*lxl_cycle;


lxl_cycle_t *
lxl_init_cycle(lxl_cycle_t *old_cycle)
{
	size_t			len;
	void 	   	   *rv;
	lxl_uint_t 		i;
	lxl_conf_t 		conf;
	lxl_pool_t 	   *pool;
	lxl_cycle_t    *cycle;
	lxl_core_module_t  *module;
	char 			hostname[LXL_MAXHOSTNAMELEN];
	
	lxl_log_error(LXL_LOG_INFO, 0, "cycle init");
	lxl_time_update();

	pool = lxl_create_pool(LXL_DEFAULT_POOL_SIZE);
	if (pool == NULL) {
		return NULL;
	}

	cycle = lxl_palloc(pool, sizeof(lxl_cycle_t));
	if (cycle == NULL) {
		lxl_destroy_pool(pool);
		return NULL;
	}

	cycle->pool = pool;
	cycle->log = old_cycle->log;
	cycle->prefix.data = lxl_pnalloc(pool, old_cycle->prefix.len + 1);
	if (cycle->prefix.data == NULL) {
		lxl_destroy_pool(pool);
		return NULL;
	}

	cycle->prefix.len = old_cycle->prefix.len;
	memcpy(cycle->prefix.data, old_cycle->prefix.data, old_cycle->prefix.len + 1);

	cycle->conf_file.data = lxl_pnalloc(pool, old_cycle->conf_file.len + 1);
	if (cycle->conf_file.data == NULL) {
		lxl_destroy_pool(pool);
		return NULL;
	}

	cycle->conf_file.len = old_cycle->conf_file.len;
	memcpy(cycle->conf_file.data, old_cycle->conf_file.data, old_cycle->conf_file.len + 1);

	cycle->listening.elts = lxl_pcalloc(pool, 4 * sizeof(lxl_listening_t));
	if (cycle->listening.elts == NULL) {
		lxl_destroy_pool(pool);
		return NULL;
	}

	cycle->listening.nelts = 0;
	cycle->listening.size = sizeof(lxl_listening_t);
	cycle->listening.nalloc = 4;
	cycle->listening.pool = pool;
	
	cycle->conf_ctx = lxl_pcalloc(pool, lxl_max_module * sizeof(void *));
	if (cycle->conf_ctx == NULL) {
		lxl_destroy_pool(pool);
		return NULL;
	}
	
	if (gethostname(hostname,  LXL_MAXHOSTNAMELEN) == -1) {
		lxl_log_error(LXL_LOG_EMERG, errno, "gethostname() failed");
		lxl_destroy_pool(pool);
		return NULL;
	}

	hostname[LXL_MAXHOSTNAMELEN - 1] = '\0';
	len = strlen(hostname);
	cycle->hostname = lxl_pnalloc(pool, len + 1);
	if (cycle->hostname == NULL) {
		lxl_destroy_pool(pool);
		return NULL;
	}

	memcpy(cycle->hostname, hostname, len + 1);
	lxl_strlower(cycle->hostname, len);

	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_CORE_MODULE) {		
			continue;
		}

		module = lxl_modules[i]->ctx;
		if (module->create_conf) {
			rv = module->create_conf(cycle);
			if (rv == NULL) {	
				lxl_destroy_pool(cycle->pool);
				return NULL;
			}

			cycle->conf_ctx[lxl_modules[i]->index] = rv;
		}
	}

	memset(&conf, 0x00, sizeof(lxl_conf_t));
	conf.args = lxl_array_create(pool, 10, sizeof(lxl_str_t));
	if (conf.args == NULL) {
		lxl_destroy_pool(pool);
		return NULL;
	}

	conf.temp_pool = lxl_create_pool(LXL_DEFAULT_POOL_SIZE);
	if (conf.temp_pool == NULL) {
		lxl_destroy_pool(pool);
		return NULL;
	}
		
	conf.ctx = cycle->conf_ctx;
	conf.cycle = cycle;
	conf.pool = pool;
	conf.module_type = LXL_CORE_MODULE;
	conf.cmd_type = LXL_MAIN_CONF;

	if (lxl_conf_parse(&conf, &cycle->conf_file) != LXL_CONF_OK) {
		lxl_destroy_cycle_pools(&conf);
		return NULL;
	}
	
	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_CORE_MODULE) {		
			continue;
		}

		module = lxl_modules[i]->ctx;
		if (module->init_conf) {
			if (module->init_conf(cycle, cycle->conf_ctx[lxl_modules[i]->index]) == LXL_CONF_ERROR) {
				lxl_destroy_cycle_pools(&conf);
				return NULL;
			}
		}
	}

	/* create paths */

	if (lxl_open_listening_sockets(cycle) != 0) {
		lxl_destroy_cycle_pools(&conf);
		return NULL;
	}

	lxl_configure_listening_sockets(cycle);
	
	/* init module */
	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->init_module) {
			if (lxl_modules[i]->init_module(cycle) != 0) {
				/* fatal */
				exit(1);
			}
		}
	}

	/* errlog init conf */
	if (lxl_errlog_module_init_conf(cycle, lxl_get_conf(cycle->conf_ctx, lxl_errlog_module)) != LXL_CONF_OK) {
		lxl_destroy_cycle_pools(&conf);
		return NULL;
	}

	return cycle;
}

static void
lxl_destroy_cycle_pools(lxl_conf_t *conf)
{
	lxl_destroy_pool(conf->temp_pool);
	lxl_destroy_pool(conf->pool);
}
