
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_log.h>
#include <lxl_palloc.h>
#include <lxl_inet.h>



static int	lxl_parse_inet_url(lxl_pool_t *pool, lxl_url_t *u);


in_addr_t
lxl_inet_addr(char *text, size_t len) 
{
	char *p, c;
	in_addr_t addr;
	lxl_uint_t octet, n;

	addr = 0;
	octet = 0;
	n = 0;
	for (p = text; p < text + len; ++p) {
		c = *p;
		if (c >= '0' && c <= '9') {
			octet = octet * 10 + (c - '0');
			continue;
		}

		if (c == '.' && octet < 256) {
			addr = (addr << 8) + octet;
			octet = 0;
			++n;
			continue;
		}

		return INADDR_NONE;
	}

	if (n == 3 && octet < 256) {
		addr = (addr << 8) + octet;
		return htonl(addr);
	}

	return INADDR_NONE;
}

int
lxl_parse_url(lxl_pool_t *pool, lxl_url_t *u)
{
	return lxl_parse_inet_url(pool, u);
}

static int	
lxl_parse_inet_url(lxl_pool_t *pool, lxl_url_t *u)
{
	int n;
	size_t len;
	char *host, *last, *port;
	struct sockaddr_in *sin;

	u->socklen = sizeof(struct sockaddr_in);
	sin = (struct sockaddr_in *) (u->sockaddr);
	sin->sin_family = AF_INET;
	u->family = AF_INET;
	
	host = u->url.data;
	port = strchr(host, ':');
	if (!port) {
		u->err = "example, ip:port";
		return -1;
	}

	++port;
	n = atoi(port);
	if (n < 1 || n > 65535) {
		u->err = "invalid port";
		return -1;
	}

	u->port = (in_port_t) n;
	sin->sin_port = htons((in_port_t) n);
	last = port - 1;

	/*if (port) {
		++port;
		n = atoi(port);
		if (n < 1 || n > 65535) {
			u->err = "invalid port";
			return -1;
		}

		u->port = (in_port_t) n;
		sin->sin_port = htons((in_port_t) n);
		last = port - 1;
	} else {
		if (u->listen) {
			n = atoi(host);
			if (n < 1 || n > 65535) {
				u->err = "invalid port";
				return -1;
			}

			u->port = (in_port_t) n;
			sin->sin_port = htons(in_port_t) n;
			u->wildcard = 1;
			return -1;
		}

		u->no_port = 1;
		u->port = u->default_port;
		sin->sin_port = htons(u->default_port);
	}*/

	len =  last - host;
	if (len == 0) {
		u->err = "no host";
		return -1;
	}

	u->host.len = len;
	u->host.data = host;
	if (u->listen && len == 1 && *host == '*') {
		u->wildcard = 1;
		return 0;
	}

	//sin->sin_addr.s_addr = lxl_inet_addr(host, len);
	*last = '\0';
	sin->sin_addr.s_addr = inet_addr(host);
	if (sin->sin_addr.s_addr != INADDR_NONE) {
		if (sin->sin_addr.s_addr == INADDR_ANY) {
			u->wildcard = 1;
		}
	} else {
		u->err = "invalid host";
		return -1;
	}
	*last = ':';

	/*if (lxl_inet_resolve_host(pool, u) != 0) {
		return -1;
	}*/
	
	return 0;
}
