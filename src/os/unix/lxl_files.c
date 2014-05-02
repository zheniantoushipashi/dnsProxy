

/*
 * Copyright (C) xianliang.li
 */


#include <lxl_log.h>
#include <lxl_file.h>
#include <lxl_files.h>


ssize_t 
lxl_read_file(lxl_file_t *file, char *buf, size_t size, off_t offset)
{
	ssize_t n;
	
	if (file->sys_offset != offset) {
		if (lseek(file->fd, offset, SEEK_CUR) == -1) {
			lxl_log_error(LXL_LOG_ALERT, errno, "lseek() \"%s\" failed", file->name.data);
			return -1;
		}

		file->sys_offset = offset;
	}

	n = read(file->fd, buf, size);
	if (n == -1) {
		lxl_log_error(LXL_LOG_ALERT, errno, "read() \"%s\" failed", file->name.data);
		return -1;
	}

	file->sys_offset += n;

	file->offset += n;

	return n;
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
				if (mkdir(d, 0744) == -1) {	/* access dir */
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
