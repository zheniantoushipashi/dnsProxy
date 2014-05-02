/*
 * Copyright (c) xianliang.li
 * Copyright (c) alibaba, inc.
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

//#include "lxl_log.h"
#include "lxl_util.h"
//#include "conf.h"


int 
lxl_daemon()
{
	int fd;

	switch (fork()) {
	case -1:
		return -1;
	case 0:
		break;
	default:
		exit(0);
	}
	
	if (setsid() == -1) {
		//lxl_log_error(LXL_LOG_EMERG, log, "setsid() failed: %s", strerror(errno));
		return -1;
	}

	umask(0);	/* umask&~0 ===> 777 */
	fd = open("/dev/null", O_RDWR);
	if (fd == -1) {
		//lxl_log_error(LXL_LOG_EMERG, log, "open(\"/dev/null\") failed: %s", strerror(errno));
		return -1;
	}

	if (dup2(fd, STDIN_FILENO) == -1) {
		//lxl_log_error(LXL_LOG_EMERG, log, "dup2(STDIN_FILENO) failed: %s", strerror(errno));
		return -1;
	}
	
	if (dup2(fd, STDOUT_FILENO) == -1) {
		//lxl_log_error(LXL_LOG_EMERG, log, "dup2(STDOUT_FILENO) failed: %s", strerror(errno));
		return -1;
	}
	/*tempfd = dup2(fd, STDERR_FILENO);*/

	if (fd > STDERR_FILENO) {
		close(fd);
	}	

	return 0;
}

int
lxl_setr_nofile(size_t rlim)
{
	struct rlimit r;	
	
	r.rlim_cur = rlim;
	r.rlim_max = rlim;		/* rlimit <= kernel max */
	if (setrlimit(RLIMIT_NOFILE, &r) == -1) {
		return -1;
	}

	return 0;
}

int 
lxl_setr_core(void)
{
	struct rlimit r;	

	r.rlim_cur = RLIM_INFINITY;
	r.rlim_max = RLIM_INFINITY;		
	if (setrlimit(RLIMIT_CORE, &r) == -1) {
		return -1;
	}

	return 0;
}

int 
lxl_is_obs_pathname(char *pathname)
{
	if (pathname[0] != '/') {
		return -1;
	}

	return 0;
}

int 
lxl_is_filename(char *filename)
{
	size_t len = strlen(filename);

	if (filename[len -1] == '/') {
		return -1;
	}

	return 0;
}

int 
lxl_mkdir(char *dir)
{
	char *pos;
	struct stat st;
	char d[256];

	snprintf(d, sizeof(d), "%s", dir);
	
	pos = d;
	while (pos) {
		pos = strchr(pos + 1, '/');
		if (pos) {
			*pos = '\0';
			if (stat(d, &st) == -1) {
				if (mkdir(d, 0744) == -1) {	/* dir */
					*pos = '/';
					return -1;
				}
			}
			*pos = '/';
		}
	}

	if (stat(d, &st) == -1) {
		if (mkdir(d, 0744) == -1) {
			return -1;
		}
	}

	return 0;
}

/*void
lxl_worker_process_init(lxl_log_t *log, conf_core_t *core)
{
	struct rlimit r;
	struct passwd *pwd;
	struct group *grp;

	// set env
	if (core->worker_priority != CONF_UNSET_INT && setpriority(PRIO_PROCESS, 0, core->worker_priority) == -1) {
		lxl_log_error(LXL_LOG_ERROR, log, "setpriority(PROI_PROCESS, %d) failed: %s", core->worker_priority, strerror(errno));
	}

	if (core->worker_core != CONF_UNSET_INT) {	
		r.rlim_cur = RLIM_INFINITY;
		r.rlim_max = RLIM_INFINITY;
		if (setrlimit(RLIMIT_CORE, &r) == -1) {
			lxl_log_error(LXL_LOG_ERROR, log, "setrlimit(RLIM_INFINITY) failed: %s", strerror(errno));
		}
	} 
	
	if (core->worker_nofile != CONF_UNSET_INT) {
		r.rlim_cur = (size_t) core->worker_nofile;
		r.rlim_max = (size_t) core->worker_nofile;
		if (setrlimit(RLIMIT_NOFILE, &r) == -1) {
			lxl_log_error(LXL_LOG_ERROR, log, "setrlimit(%d) failed: %s", core->worker_nofile, strerror(errno));
		}
	}

	/* root pid gid uid %d */
	/*if (geteuid() == 0) {
		pwd = getpwnam(core->username->data);
		if (pwd == NULL) {
			lxl_log_error(LXL_LOG_EMERG, log, "getpwname(%s) failed: %s", core->username->data, strerror(errno));
			exit(1);
		}	

		grp = getgrnam(core->groupname->data);
		if (grp == NULL) {
			lxl_log_error(LXL_LOG_EMERG, log, "getgrnam(%s) failed: %s", core->groupname->data, strerror(errno));
			exit(1);
		}
		
		if (setgid(grp->gr_gid) == -1) {
			lxl_log_error(LXL_LOG_EMERG, log, "setgid(%d) failed: %s", grp->gr_gid, strerror(errno));
			exit(1);
		}

		if (initgroups(core->username->data, grp->gr_gid) == -1) {
			lxl_log_error(LXL_LOG_EMERG, log, "initgroups(%s, %d) failed: %s", core->username->data, grp->gr_gid, strerror(errno));
			exit(1);
		}

		if (setuid(pwd->pw_uid) == -1) {
			lxl_log_error(LXL_LOG_EMERG, log, "setuid(%d) failed: %s", pwd->pw_uid, strerror(errno));
			exit(1);
		}
	}

	// cpu fa
	if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) == -1) {
		lxl_log_error(LXL_LOG_WARN, log, "prctl(PR_SET_DUMPABLE) failed: %s", strerror(errno));
	}

	if (core->worker_dir != CONF_UNSET_PTR) {
		if (chdir(core->worker_dir->data) == -1) {
			lxl_log_error(LXL_LOG_EMERG, log, "worker_dir chdir(%s) failed: %s", core->worker_dir->data, strerror(errno));
			exit(1);
		}
	}
}*/

#if 0
int main()
{
	/*lxl_mkdir("ll2");
	printf("ll2 %s\n", strerror(errno));
	lxl_mkdir("/home/lixianliang/ll1");
	printf("llll1 %s\n", strerror(errno));
	lxl_mkdir("./ll3");
	printf("ll3 %s\n", strerror(errno));
	lxl_mkdir("./ll4/d");
	printf("ll4 %s\n", strerror(errno));*/

	char str1[] = "ABCDEFGHIJKLMNOPQRAST";
	lxl_strlower(str1, strlen(str1));
	printf("str1 %s\n", str1);
	lxl_strupper(str1, strlen(str1));
	printf("st1 %s\n", str1);
	
	return 0;
}
#endif
