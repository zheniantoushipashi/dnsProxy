
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_log.h>
#include <lxl_event.h>
#include <lxl_connection.h>
#include <lxl_socket.h>


/*
 * nginx use ioctl, also two system call fcntl 
 * 
 * int
lxl_nonblocking(int fd)
{
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);	 
	
	return 0;
}*/
lxl_int_t
lxl_nonblocking(int fd){
	int nb = 1;

	return ioctl(fd, FIONBIO, &nb);
}

lxl_int_t
lxl_blocking(int fd)
{
	int nb = 0;
	
	return ioctl(fd, FIONBIO, &nb);
}

int
lxl_tcp_socket(void)
{
	return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

/*
 * int nb 0 block 1 nonblock
 */
int 
lxl_tcp_connect(char *ip, uint16_t port, lxl_int_t nb)
{
	int fd, reuse = 1;
	struct sockaddr_in addr;
	
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == -1) {
		return -1;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1) {
		goto failed;
	}
	if (nb == 1 /* && lxl_nonblocking(fd) redis code style */) {
		if (lxl_nonblocking(fd) == -1) {
			goto failed;
		}
	}
	if (connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
		if (nb == 1 && errno == EINPROGRESS) {	/* lxl_nonblocking */
			return fd;
		}
		goto failed;
	}

	return fd;

failed:
	close(fd);
	return -1;
}

int 
lxl_tcp_listen(char *ip, uint16_t port, lxl_int_t nb)
{
	int fd, reuse = 1;
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	if (ip == NULL) { 
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	} else {
		addr.sin_addr.s_addr = inet_addr(ip);
	}
	addr.sin_port = htons(port);
	fd = socket(addr.sin_family, SOCK_STREAM, IPPROTO_TCP);
	if (fd == -1) {
		return -1;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1) {
		goto failed;
	}
	if (bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
		goto failed;
	}
	if (nb == 1 && lxl_nonblocking(fd) == -1) {
		goto failed;
	}
	if (listen(fd, LXL_LISTENQ) == -1) {
		goto failed;
	}

	return fd;

failed:
	close(fd);
	return -1; 
}

int 
lxl_udp_socket(void)
{
	return socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
}

int 
lxl_udp_listen(char *ip, uint16_t port, lxl_int_t nb)
{
	int fd, reuse = 1;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd == -1) {
		return -1;
	}
	addr.sin_family = AF_INET;
	if (ip == NULL) {
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	} else { 
		// addr.sin_addr.s_addr = inet_addr(ip);
		inet_aton(ip, &addr.sin_addr);
	}
	addr.sin_port = htons(port);
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1) {
		goto failed;
	}
	if (bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
		goto failed;
	}
	if (nb == LXL_NONBLOCKING && lxl_nonblocking(fd) == -1) {
		goto failed;
	}

	return fd;
	
failed:
	close(fd);
	return -1;
}

int 
lxl_unix_connect(char *path, int nb)
{
	int fd;
	struct sockaddr_un addr;
	
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1) {
		return -1;
	}
	if (nb == LXL_NONBLOCKING /* && lxl_nonblocking(fd) redis code style */) {
		if (lxl_nonblocking(fd) == -1) {
			goto failed;
		}
	}
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, sizeof(addr.sun_path)-1);
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		if (nb == LXL_NONBLOCKING && errno == EINPROGRESS) {	/* lxl_nonblocking */
			return fd;
		}
		goto failed;
	}

	return fd;

failed:
	close(fd);
	return -1;
}

int 
lxl_unix_listen(char *path)
{
	int fd;
	struct sockaddr_un addr;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1) {
		return -1;
	}
	addr.sun_family = AF_UNIX;
	if (path == NULL) { 
		unlink("/tmp/unix.sock");
		strncpy(addr.sun_path, "/tmp/unix.sock", sizeof(addr.sun_path)-1);
	} else {
		unlink(path);
		strncpy(addr.sun_path, path, sizeof(addr.sun_path)-1);
	}
	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		goto failed;
	}
	if (listen(fd, LXL_LISTENQ) == -1) {
		goto failed;
	}

	return fd;

failed:
	close(fd);
	return -1; 
}

ssize_t 
lxl_readn(int fd, char *buf, size_t size)
{
	ssize_t n, nread;

	nread = 0;
	while (size > 0) {
		n = read(fd, buf+nread, size);
		if (n == -1) {
			if (errno == EINTR) {
				continue;
			} else if (errno == EAGAIN || errno == EWOULDBLOCK) {	
				return nread;
			} else {
				return -1;
			}
		} else if (n == 0) {
			return nread;	/* EOF */
		} else {
			nread += n;
			size -= n;
		}
	}

	return nread;
}

ssize_t
lxl_writen(int fd, char *buf, size_t size)
{/* void * ==> char * */
	ssize_t n, nwrite;

	nwrite = 0;
	while (size > 0) {
		n = write(fd, buf+nwrite, size);
		if (n == -1) {
			if (errno == EINTR) {
				continue;
			} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return nwrite;
			} else {
				return -1;
			}
		} else if(n == 0) {
			return nwrite;
		} else {
			nwrite += n;
			size -= n;
		}
	}

	return nwrite;
}

ssize_t 
lxl_recv(lxl_connection_t *c, char *buf, size_t size)
{
	ssize_t n;
	lxl_event_t *rev;

	rev = c->read;
	for (; ;) {
		n = recv(c->fd, buf, size, 0);
		lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "recv: fd:%d %ld of %lu", c->fd, n, size);

		if (n > 0) {
			return n;
		} else if (n == 0) {
			rev->ready = 0;
			rev->eof = 1;
			return n;
		} else {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				rev->ready = 0;
				lxl_log_debug(LXL_LOG_DEBUG_EVENT, errno, "recv() not ready");
				return LXL_EAGAIN;
			} else if (errno == EINTR) {
				lxl_log_debug(LXL_LOG_DEBUG_EVENT, errno, "recv() not ready");
				continue;
			} else {
				rev->error = 1;
				lxl_log_error(LXL_LOG_ALERT, errno, "recv() failed");
				return LXL_ERROR;
			}
		}
	}
}

ssize_t 
lxl_send(lxl_connection_t *c, char *buf, size_t size) 
{
    	ssize_t n;
	lxl_event_t *wev;

	wev = c->write;
	for (; ;) {
		n = send(c->fd, buf, size, 0);
		lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "send: fd:%d %ld of %lu", c->fd, n, size);

		if (n != -1) {
			if (n < (ssize_t) size) {
				wev->ready = 0;
			}

			return n;
		} else {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				wev->ready = 0;
				lxl_log_debug(LXL_LOG_DEBUG_EVENT, errno, "send() not ready");
				return LXL_EAGAIN;
			} else if (errno == EINTR) {
				lxl_log_debug(LXL_LOG_DEBUG_EVENT, errno, "send() not ready");
				continue;
			} else {
				wev->error = 1;
				lxl_log_error(LXL_LOG_ALERT, errno, "send() failed");
				return LXL_ERROR;
			}
		}
	}
}

ssize_t 
lxl_recvfrom(lxl_connection_t *c, char *buf, size_t size)
{
	ssize_t n;

	for (; ;) {
		//n = recvfrom(c->fd, buf, size, 0, NULL, NULL);
		n = recv(c->fd, buf, size, 0);
		lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "recv: fd:%d %ld of %lu", c->fd, n, size);

		if (n != -1) {
			return n;
		} else {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				lxl_log_debug(LXL_LOG_DEBUG_EVENT, errno, "recv() not ready");
				return LXL_EAGAIN;
			} else if (errno == EINTR) {
				lxl_log_debug(LXL_LOG_DEBUG_EVENT, errno, "recv() not ready");
				continue;
			} else {
				lxl_log_error(LXL_LOG_ALERT, errno, "recv() failed");
				return LXL_ERROR;
			}
		}
	}
}

ssize_t 
lxl_recvfrom_peer(lxl_connection_t *c, char *buf, size_t size)
{
	ssize_t n;

	for (; ;) {
		n = recvfrom(c->fd, buf, size, 0, &c->sockaddr, &c->socklen);
		lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "recvfrom: fd:%d %ld of %lu", c->fd, n, size);

		if (n != -1) {
			return n;
		} else {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				lxl_log_debug(LXL_LOG_DEBUG_EVENT, errno, "recvfrom() not ready");
				return LXL_EAGAIN;
			} else if (errno == EINTR) {
				lxl_log_debug(LXL_LOG_DEBUG_EVENT, errno, "recvfrom() not ready");
				continue;
			} else {
				lxl_log_error(LXL_LOG_ALERT, errno, "recvfrom() failed");
				return LXL_ERROR;
			}
		}
	}
}

ssize_t 
lxl_sendto(lxl_connection_t *c, char *buf, size_t size) 
{
	ssize_t n;

	for (; ;) {
		n = sendto(c->fd, buf, size, 0, &c->sockaddr, c->socklen);
		lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "sendto: fd:%d %ld of %lu", c->fd, n, size);

		if (n == size) {
			return n;
		} else {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				lxl_log_debug(LXL_LOG_DEBUG_EVENT, errno, "sendto() not ready");
				return LXL_EAGAIN;
			} else if (errno == EINTR) {
				lxl_log_debug(LXL_LOG_DEBUG_EVENT, errno, "sendto() not ready");
				continue;
			} else {
				lxl_log_error(LXL_LOG_ALERT, errno, "sendto() failed");
				return LXL_ERROR;
			}
		}
	}
}
