
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_core.h>
#include <lxl_config.h>
#include <lxl_log.h>
#include <lxl_hash.h>
#include <lxl_times.h>
#include <lxl_conf_file.h>
#include <lxl_alloc.h>
#include <lxl_dns.h>
#include <lxl_dns_data.h>


static void *	lxl_dns_core_create_main_conf(lxl_conf_t *cf);
static char *	lxl_dns_core_init_main_conf(lxl_conf_t *cf, void *conf);
static void *	lxl_dns_core_create_srv_conf(lxl_conf_t *cf);
static char *	lxl_dns_core_merge_srv_conf(lxl_conf_t *cf, void *parent, void *child);
static char *	lxl_dns_core_server(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
static char *	lxl_dns_core_listen(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);

static int		lxl_dns_core_init(lxl_cycle_t *cycle);
static int 		lxl_dns_load_named_root(char *file);
static int 		lxl_dns_parse_root_rr(lxl_int_t argc, char (*argv)[64], lxl_uint_t line, void *args);


lxl_pool_t 	   			   *lxl_dns_pool;
lxl_dns_zone_t 			   *lxl_dns_root_zone;


static lxl_command_t lxl_dns_core_commands[] = {
	
	{ lxl_string("server"),
	  LXL_DNS_MAIN_CONF|LXL_CONF_BLOCK|LXL_CONF_NOARGS,
	  lxl_dns_core_server,
      0,
      0,
      NULL },

	{ lxl_string("listen"),
	  LXL_DNS_SRV_CONF|LXL_CONF_TAKE12,
	  lxl_dns_core_listen,
      0,
      0,
      NULL },

	{ lxl_string("tcp"),
      LXL_DNS_MAIN_CONF|LXL_CONF_FLAG,
      lxl_conf_set_flag_slot,
	  LXL_DNS_MAIN_CONF_OFFSET,
      offsetof(lxl_dns_core_main_conf_t, tcp),
      NULL },

	{ lxl_string("zone_hash_bucket_size"),
	  LXL_DNS_MAIN_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_size_slot, 
	  LXL_DNS_MAIN_CONF_OFFSET,
	  offsetof(lxl_dns_core_main_conf_t, zone_hash_bucket_size),
	  NULL },

	{ lxl_string("named_root_file"),
      LXL_DNS_MAIN_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_str_slot, 
	  LXL_DNS_MAIN_CONF_OFFSET,
	  offsetof(lxl_dns_core_main_conf_t, named_root_file),
      NULL },

	{ lxl_string("client_timeout"),
      LXL_DNS_MAIN_CONF|LXL_DNS_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_num_slot, 
	  LXL_DNS_SRV_CONF_OFFSET, 
	  offsetof(lxl_dns_core_srv_conf_t, client_timeout),
	  NULL, },

	/*{ lxl_string("upstream_timeout"),
	  LXL_DNS_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_num_slot, 
	  0, 
	  offsetof(lxl_dns_core_srv_conf_t, upstream_timeout),
	  NULL }, */

	{ lxl_string("connection_pool_size"),
	  LXL_DNS_MAIN_CONF|LXL_DNS_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_size_slot,
	  LXL_DNS_SRV_CONF_OFFSET,
	  offsetof(lxl_dns_core_srv_conf_t, connection_pool_size),
	  NULL },

	{ lxl_string("client_buffer_size"),
	  LXL_DNS_MAIN_CONF|LXL_DNS_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_size_slot,
	  LXL_DNS_SRV_CONF_OFFSET,
	  offsetof(lxl_dns_core_srv_conf_t, client_buffer_size),
	  NULL },

	lxl_null_command
};

static lxl_dns_module_t lxl_dns_core_module_ctx = {
	lxl_string("dns"),
	lxl_dns_core_create_main_conf,	/* create main configuration */
	lxl_dns_core_init_main_conf,	/* init main configuration */
	lxl_dns_core_create_srv_conf,	/* create server configuration */
	lxl_dns_core_merge_srv_conf		/* merge server configuration */
};

lxl_module_t lxl_dns_core_module = {
	0,
	0,
	(void *) &lxl_dns_core_module_ctx,
	lxl_dns_core_commands,
	LXL_DNS_MODULE,
	NULL,
	lxl_dns_core_init,
};


static void *	
lxl_dns_core_create_main_conf(lxl_conf_t *cf)
{
	lxl_dns_core_main_conf_t *cmcf;

	cmcf = lxl_pcalloc(cf->pool, sizeof(lxl_dns_core_main_conf_t));
	if (cmcf == NULL) {
		return NULL;
	}

	if (lxl_array_init(&cmcf->servers, cf->pool, 4, sizeof(lxl_dns_core_srv_conf_t)) != 0) {
		return NULL;
	}

	if (lxl_array_init(&cmcf->listens, cf->pool, 4, sizeof(lxl_dns_listen_t)) != 0) {
		return NULL;
	}

	cmcf->tcp = LXL_CONF_UNSET_UINT;
	cmcf->zone_hash_bucket_size = LXL_CONF_UNSET_UINT;
	lxl_str_null(&cmcf->named_root_file);

	return cmcf;
}

static char *	
lxl_dns_core_init_main_conf(lxl_conf_t *cf, void *conf)
{
	lxl_dns_core_main_conf_t *cmcf = conf;

	lxl_conf_init_value(cmcf->tcp, 0);
	lxl_conf_init_uint_value(cmcf->zone_hash_bucket_size, 1000000);		/* 1m */
	/* named root file */
	
	return LXL_CONF_OK;
}

static void *
lxl_dns_core_create_srv_conf(lxl_conf_t *cf)
{
	lxl_dns_core_srv_conf_t *cscf;

	cscf = lxl_pcalloc(cf->pool, sizeof(lxl_dns_core_srv_conf_t));
	if (cscf == NULL) {
		return NULL;
	}

	/* set by lxl_pcalloc 
 	 * cscf->ctx = NULL
     */

	cscf->client_timeout = LXL_CONF_UNSET_UINT;
	cscf->connection_pool_size = LXL_CONF_UNSET_SIZE;
	cscf->client_buffer_size = LXL_CONF_UNSET_SIZE;

	return cscf;
}

static char *
lxl_dns_core_merge_srv_conf(lxl_conf_t *cf, void *parent, void *child)
{
	lxl_dns_core_srv_conf_t *prev = parent;
	lxl_dns_core_srv_conf_t *conf = child;

	lxl_conf_merge_uint_value(conf->client_timeout, prev->client_timeout, 3000);
	lxl_conf_merge_size_value(conf->connection_pool_size, prev->connection_pool_size, 1024);
	lxl_conf_merge_size_value(conf->client_buffer_size, prev->client_buffer_size, 512);

	return LXL_CONF_OK;
}

static char *
lxl_dns_core_server(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	char 					   *rv;
	void 					   *mconf;
	lxl_uint_t 					i;
	lxl_conf_t 					pcf;
	lxl_dns_module_t		   *module;
	lxl_dns_conf_ctx_t 		   *ctx, *main_ctx;
	lxl_dns_core_srv_conf_t    *cscf, **cscfp;
	lxl_dns_core_main_conf_t   *cmcf;

	ctx = lxl_pcalloc(cf->pool, sizeof(lxl_dns_conf_ctx_t));
	if (ctx == NULL) {
		return LXL_CONF_ERROR;
	}

	main_ctx = cf->ctx;
	ctx->main_conf = main_ctx->main_conf;

	ctx->srv_conf = lxl_pcalloc(cf->pool, lxl_dns_max_module * sizeof(void *));
	if (ctx->srv_conf == NULL) {
		return LXL_CONF_ERROR;
	}

	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_DNS_MODULE) {
			continue;
		}

		module = lxl_modules[i]->ctx;
		if (module->create_srv_conf) {
			mconf = module->create_srv_conf(cf);
			if (mconf == NULL) {
				return LXL_CONF_ERROR;
			}

			ctx->srv_conf[lxl_modules[i]->ctx_index] = mconf;
		}
	}
	
	cscf = ctx->srv_conf[lxl_dns_core_module.ctx_index];
	cscf->ctx = ctx;
	cmcf = ctx->main_conf[lxl_dns_core_module.ctx_index];
	cscfp = lxl_array_push(&cmcf->servers);
	if (cscfp == NULL) {
		return LXL_CONF_ERROR;
	}

	*cscfp = cscf;

	pcf = *cf;
	cf->ctx = ctx;
	cf->cmd_type = LXL_DNS_SRV_CONF;
	rv = lxl_conf_parse(cf, NULL);
	*cf = pcf;

	return rv;
}

static char *	
lxl_dns_core_listen(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	size_t off;
	lxl_uint_t i, nelts;
	lxl_url_t u;
	lxl_str_t *value;
	in_port_t port;
	struct sockaddr *sa;
	struct sockaddr_in *sin;
	lxl_dns_listen_t *ls;
	lxl_dns_core_main_conf_t *cmcf;

	value = lxl_array_elts(cf->args);
	u.url = value[1];
	u.listen = 1;

	if (lxl_parse_url(cf->pool, &u) != 0) {
		if (u.err) {
			lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "%s in \"%s\" of the \"listen\" directive", u.err, u.url.data);
		}

		return LXL_CONF_ERROR;
	}

	cmcf = lxl_dns_conf_get_module_main_conf(cf, lxl_dns_core_module);
	ls = lxl_array_elts(&cmcf->listens);
	nelts = lxl_array_nelts(&cmcf->listens);
	for (i = 0; i < nelts; ++i) {
		sa = (struct sockaddr *) ls[i].sockaddr;
		off = offsetof(struct sockaddr_in, sin_addr);
		sin = (struct sockaddr_in *) sa;
		port = sin->sin_port;
		
		if (memcmp(ls[i].sockaddr + off, u.sockaddr + off, 4) != 0) {
			continue;
		}

		if (port != u.port) {
			continue;
		}

		lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "duplicate \"%s\" address and port again", &u.url);
		
		return LXL_CONF_ERROR;
	}
	
	ls = lxl_array_push(&cmcf->listens);
	if (ls == NULL) {
		return LXL_CONF_ERROR;
	}

	memset(ls, 0x00, sizeof(lxl_dns_listen_t));
	memcpy(ls->sockaddr, u.sockaddr, u.socklen);
	ls->socklen = u.socklen;
	ls->wildcard = u.wildcard;
	ls->ctx = cf->ctx;
	
	return LXL_CONF_OK;
}

int 
lxl_dns_core_init(lxl_cycle_t *cycle)
{
	lxl_dns_pool = lxl_create_pool(LXL_DEFAULT_POOL_SIZE);
	if (lxl_dns_pool == NULL) {
		return -1;
	}

	lxl_dns_root_zone = lxl_palloc(lxl_dns_pool, sizeof(lxl_dns_zone_t));
	if (lxl_dns_root_zone == NULL) {
		return -1;
	}

	lxl_log_error(LXL_LOG_INFO, 0, "dns init zone hash");
	if (lxl_hash_init(&lxl_dns_hash, lxl_dns_pool, 1024000, lxl_hash_key) == -1) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dns init hash failed");
		return -1;
	}

	lxl_list_init(&lxl_dns_root_zone->rrset_list);
	lxl_dns_root_zone->update_sec = lxl_current_sec;

	lxl_log_error(LXL_LOG_INFO, 0, "dns load named root path conf/named.root");
	lxl_dns_load_named_root("conf/named.root");

	lxl_log_error(LXL_LOG_INFO, 0, "dns add root zone");
	if (lxl_dns_zone_add(LXL_DNS_ROOT_LABEL, LXL_DNS_ROOT_LEN, lxl_dns_root_zone) == -1) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dns add root zone failed");
		return -1;
	}

	return 0;
}

static int 
lxl_dns_load_named_root(char *file)
{
	FILE *fp;
	char buf[512];
	char argv[5][64];
	lxl_int_t  argc;
	lxl_uint_t line;

	fp = fopen(file, "r");
	if (fp == NULL) {
		lxl_log_error(LXL_LOG_ALERT, errno, "fopen(%s) failed", file);
		return -1;
	}

	line = 0;
	while ((fgets(buf, sizeof(buf), fp)) != NULL) {
		++line;
#if LXL_DEBUG
		//lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "source buf %s, line %lu", buf, line);
#endif
		lxl_strtrim(buf, " \t\r\n");
		if (buf[0] == ';' || buf[0] == '#' || buf[0] == '\0') {
			continue;
		} else {
#if LXL_DEBUG
			//lxl_log_debug(LXL_LOG_DEBUG_DNS, 0, "line %lu trim buf: %s", line, buf);
#endif
			// lxl_strtrim(buf, "#");
		//	lxl_dns_parse(buf, argv);
			argc = sscanf(buf, "%s %s %s %s %s", argv[0], argv[1], argv[2], argv[3], argv[4]);
			if (argc == -1) {
				lxl_log_error(LXL_LOG_ALERT, errno, "ssconf failed");
				continue;
			}
			
			lxl_dns_parse_root_rr(argc, argv, line, NULL);
		}
	}

	fclose(fp);

	return 0;
}

static int 
lxl_dns_parse_root_rr(lxl_int_t argc, char (*argv)[64], lxl_uint_t line, void *args)
{
	/* . ttl class type data */
	uint16_t type;
	size_t len;
	char *dns_str_type, *data;
	lxl_dns_rr_t rr;
	lxl_dns_rdata_t *rdata;

	if (argc == 4) {
		dns_str_type = argv[2];
		data = argv[3];
	} else if (argc == 5) {
		dns_str_type = argv[3];
		data = argv[4];
	} else {
		lxl_log_error(LXL_LOG_EMERG, 0, "need 4 | 5 argv, line %lu argc is %ld", line, argc);
		return -1;
	}

	type = lxl_dns_get_rr_type(dns_str_type, strlen(dns_str_type));
	if (type == LXL_DNS_INVALID_TYPE) {
		lxl_log_error(LXL_LOG_EMERG, 0, "unknow dns type %s, line %lu", dns_str_type, line);
		return -1;
	}

	if(type != LXL_DNS_A && type != LXL_DNS_AAAA && type != LXL_DNS_NS) {
		lxl_log_error(LXL_LOG_EMERG, 0, "root domain(.) suport A AAAA NS, not suport type %s", dns_str_type);
		return -1;
	}

	len = strlen(data);
	rdata = lxl_dns_rdata_create(data, len, type);
	if (rdata == NULL) {
		return -1;
	}

	len = strlen(argv[0]);
	lxl_dns_domain_dot_to_label(argv[0], len);
	if (len == 1) {
		rr.nlen = 1;
	} else {
		rr.nlen = len + 1;
	}
	rr.name = argv[0];
	rr.type = type;
	rr.ttl = atoi(argv[1]);
	LXL_DNS_CHECK_TTL(rr.ttl);
	rr.expires_sec = lxl_current_sec + rr.ttl;
	rr.update_flags = LXL_DNS_RR_NORMAL_TYPE;
	rr.rdlength = rdata->rdlength;
	rr.rdata = rdata->rdata;
	rr.soa_nlen = 0;
	rr.soa_flags = LXL_DNS_RRSET_NORMAL_TYPE;

	if (lxl_dns_rr_add(lxl_dns_pool, lxl_dns_root_zone, &rr) == 1) {
		lxl_free(rdata);
		return -1;
	}

	lxl_free(rdata);
	return 1; 
}
