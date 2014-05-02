
/* 
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_log.h>
#include <lxl_stack.h>
#include <lxl_times.h>
#include <lxl_string.h>
#include <lxl_alloc.h>
#include <lxl_socket.h>
#include <lxl_event_timer.h>
#include <lxl_event.h>
#include <lxl_dns.h>


static char *lxl_dns_block(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
static int	 lxl_dns_optimize_servers(lxl_conf_t *cf, lxl_array_t *listens, lxl_int_t tcp);

static void  lxl_dns_data_rebuild_handler(lxl_event_t *ev);


lxl_hash_t 					lxl_dns_hash;
lxl_dns_zone_t             *lxl_dns_root_zone;
lxl_pool_t                 *lxl_dns_pool;
lxl_event_t 				lxl_dns_event;
lxl_uint_t					lxl_dns_max_module;


static lxl_command_t lxl_dns_commands[] = {
	
	{ lxl_string("dns"),
	  LXL_MAIN_CONF|LXL_CONF_BLOCK|LXL_CONF_NOARGS,
	  lxl_dns_block,
	  0,
      0,
	  NULL },

	lxl_null_command
};

static lxl_core_module_t lxl_dns_module_ctx = {
	lxl_string("dns"),
	NULL,
	NULL
};

lxl_module_t lxl_dns_module = {
	0,
	0,
	(void *) &lxl_dns_module_ctx,
	lxl_dns_commands,
	LXL_CORE_MODULE,
	NULL,
	NULL
};


static char *
lxl_dns_block(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	/* l hour
	memset(&lxl_dns_event, 0x00, sizeof(lxl_event_t));
	lxl_dns_event.data = c;
	lxl_dns_event.handler = lxl_dns_data_rebuild_handler;
	lxl_add_timer(&lxl_dns_event, 86400*1000);
	//lxl_add_timer(&lxl_dns_event, 120*1000);

	lxl_dns_root_zone = NULL;
	lxl_log_error(LXL_LOG_INFO, 0, "dns start succeed");

	return 0;*/

	char *rv;
	lxl_uint_t i, j, mi, nelts;
	lxl_conf_t pcf;
	lxl_dns_module_t   	*module;
	lxl_dns_conf_ctx_t 	*ctx;
	lxl_dns_core_srv_conf_t **cscfp;
	lxl_dns_core_main_conf_t *cmcf;

	if (*(void **) conf) {
		return "is duplicate";
	}

	ctx = lxl_pcalloc(cf->pool, sizeof(lxl_dns_conf_ctx_t));
	if (ctx == NULL) {
		return LXL_CONF_ERROR;
	}

	*(lxl_dns_conf_ctx_t **) conf = ctx;

	lxl_dns_max_module = 0;
	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_DNS_MODULE) {
			continue;
		}

		lxl_modules[i]->ctx_index = lxl_dns_max_module++;
	}

	/* the dns main_conf context */
	ctx->main_conf = lxl_pcalloc(cf->pool, lxl_dns_max_module * sizeof(void *));
	if (ctx->main_conf == NULL) {
		return LXL_CONF_ERROR;
	}

	/* the dns srv_conf context */
	ctx->srv_conf = lxl_pcalloc(cf->pool, lxl_dns_max_module * sizeof(void *));
	if (ctx->srv_conf == NULL) {
		return LXL_CONF_ERROR;
	}

	/* create the main_conf's, the null srv_conf's of the dns module */
	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_DNS_MODULE) {
			continue;
		}

		module = lxl_modules[i]->ctx;
		mi = lxl_modules[i]->ctx_index;

		if (module->create_main_conf) {
			ctx->main_conf[mi] = module->create_main_conf(cf);
			if (ctx->main_conf[mi] == NULL) {
				return LXL_CONF_ERROR;
			}
			ctx->main_conf[lxl_modules[i]->ctx_index] = module->create_main_conf(cf);
		}

		if (module->create_srv_conf) {
			ctx->srv_conf[mi] = module->create_srv_conf(cf);
			if (ctx->srv_conf[mi] == NULL) {
				return LXL_CONF_ERROR;
			}
		}
	}

	/* parse inside the dns{} block */
	pcf = *cf;
	cf->ctx = ctx;
	cf->module_type = LXL_DNS_MODULE;
	cf->cmd_type = LXL_DNS_MAIN_CONF;
	rv = lxl_conf_parse(cf, NULL);
	if (rv != LXL_CONF_OK) {
		*cf = pcf;
		return rv;
	}

	cmcf = ctx->main_conf[lxl_dns_core_module.ctx_index];
	cscfp = lxl_array_elts(&cmcf->servers);

	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_DNS_MODULE) {
			continue;
		}

		module = lxl_modules[i]->ctx;
		mi = lxl_modules[i]->ctx_index;

		/* init dns{} mail_conf's */
		if (module->init_main_conf) {
			rv = module->init_main_conf(cf, ctx->main_conf[lxl_modules[i]->ctx_index]);
			if (rv != LXL_CONF_OK) {
				*cf = pcf;
				return rv;
			}
		}

		/* merge servers {} srv_conf */
		nelts = lxl_array_nelts(&cmcf->servers);
		for (j = 0; j < nelts; ++j) {
			cf->ctx = cscfp[j]->ctx;
			if (module->merge_srv_conf) {
				rv = module->merge_srv_conf(cf, ctx->srv_conf[mi], cscfp[j]->ctx->srv_conf[mi]);
				if (rv != LXL_CONF_OK) {
					*cf = pcf;
					return rv;
				}
			}
		}
	}

	*cf = pcf;

	if (lxl_dns_optimize_servers(cf, &cmcf->listens, cmcf->tcp) != 0) {
		return LXL_CONF_ERROR;
	}

	return LXL_CONF_OK;
}

static int 
lxl_dns_optimize_servers(lxl_conf_t *cf, lxl_array_t *listens, lxl_int_t tcp)
{
	lxl_uint_t	i, nelts;
	lxl_listening_t *ls;
	lxl_dns_listen_t *listen;

	nelts = lxl_array_nelts(listens);
	listen = lxl_array_elts(listens);
	for (i = 0; i < nelts; ++i) {
		ls = lxl_create_listening(cf, listen[i].sockaddr, listen[i].socklen, SOCK_DGRAM);
		if (ls == NULL) {
			return -1;
		}

		ls->handler = lxl_dns_init_connection;
		/* cscf->connection_pool_size 1024 default */
		ls->pool_size = 1024;

		if (tcp) {
			ls = lxl_create_listening(cf, listen[i].sockaddr, listen[i].socklen, SOCK_STREAM);
			if (ls == NULL) {
				return -1;
			}

			ls->handler = lxl_dns_init_connection;
			ls->pool_size = 1024;	/* not tcp and tcp limit 1024 */
		} 
	}
	
	return 0;
}

static void 	 
lxl_dns_data_rebuild_handler(lxl_event_t *ev)
{
	lxl_dns_data_dump();
	lxl_dns_data_rebuild();
	ev->timedout = 0;
	//lxl_add_timer(ev, 10800*1000);
	lxl_add_timer(ev, 86400*1000);
}
