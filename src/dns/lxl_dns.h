
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_DNS_H_INCLUDE
#define LXL_DNS_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_inet.h>
#include <lxl_hash.h>
#include <lxl_dns_data.h>
#include <lxl_connection.h>


#define LXL_DNS_CLIENT_BUFFER_SIZE	512
#define LXL_DNS_CLIENT_TIMEOUT		3000	/* tcp */
#define LXL_DNS_AUTHORITY_TIMEOUT	1500

#define LXL_DNS_MODULE				0x00444e53	/* "DNS" */
#define LXL_DNS_MAIN_CONF			0x02000000
#define LXL_DNS_SRV_CONF			0x04000000

#define LXL_DNS_MAIN_CONF_OFFSET	offsetof(lxl_dns_conf_ctx_t, main_conf)
#define LXL_DNS_SRV_CONF_OFFSET		offsetof(lxl_dns_conf_ctx_t, srv_conf)


typedef struct lxl_dns_request_s	lxl_dns_request_t;
typedef struct lxl_dns_upstream_s	lxl_dns_upstream_t;
typedef lxl_int_t (*lxl_dns_block_pt) (lxl_uint_t n);

typedef struct {
	void 		**main_conf;
	void 		**srv_conf;	
} lxl_dns_conf_ctx_t;

typedef struct {
	char 				sockaddr[LXL_SOCKADDRLEN];
	socklen_t			socklen;
	lxl_dns_conf_ctx_t  *ctx;
	unsigned 			wildcard:1;
	
	/* bind */
} lxl_dns_listen_t;

typedef struct {
	lxl_int_t	tcp;
	lxl_uint_t	zone_hash_bucket_size;
	lxl_str_t	named_root_file;
	lxl_array_t	servers;		/* lxl_dns_core_srv_conf_t */
	lxl_array_t listens;		/* lxl_dns_listen_t	*/
} lxl_dns_core_main_conf_t;

typedef struct {
//	lxl_int_t	tcp;
	lxl_uint_t	client_timeout;
	//lxl_uint_t	authority_timeout;
	size_t		connection_pool_size;
	size_t		client_buffer_size;
	
	lxl_dns_conf_ctx_t	*ctx;	/* server_ctx */
} lxl_dns_core_srv_conf_t;

typedef struct {
	lxl_str_t name;
	void 	*(*create_main_conf)	(lxl_conf_t *cf);
	char	*(*init_main_conf)		(lxl_conf_t *cf, void *conf);
	void	*(*create_srv_conf)		(lxl_conf_t *cf);
	char	*(*merge_srv_conf)		(lxl_conf_t *cf, void *parent, void *child);
} lxl_dns_module_t;


#define lxl_dns_conf_get_module_main_conf(cf, module)				\
	((lxl_dns_conf_ctx_t *) (cf)->ctx)->main_conf[module.ctx_index]


void lxl_dns_init_connection(lxl_connection_t *c);
//void lxl_dns_close_connection(lxl_connection_t *c, lxl_uint_t flags);
void lxl_dns_init_udp_connection(lxl_connection_t *c);
void lxl_dns_process_udp_request(lxl_connection_t *lc);


lxl_int_t lxl_dns_parse_request(lxl_dns_request_t *r, char *data);
lxl_int_t lxl_dns_domain_dot_to_label(char *domain, size_t len);
void	  lxl_dns_parse_package_to_hex(char *data, size_t len);
lxl_int_t lxl_dns_package_request(lxl_dns_request_t *r);
lxl_int_t lxl_dns_package_answer(lxl_dns_request_t *r);
void	  lxl_dns_package_header(lxl_dns_request_t *r);


extern lxl_uint_t 			lxl_dns_max_module;
extern lxl_module_t			lxl_dns_core_module;

extern lxl_hash_t 			lxl_dns_hash;
extern lxl_dns_zone_t      *lxl_dns_root_zone;
extern lxl_pool_t          *lxl_dns_pool;


#endif	/* LXL_DNS_H_INCLUDE */
