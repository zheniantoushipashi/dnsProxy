
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_log.h>
#include <lxl_alloc.h>
#include <lxl_files.h>
#include <lxl_conf_file.h>


#define LXL_CONF_BUFFER	4096


static int 			lxl_conf_handler(lxl_conf_t *cf, lxl_int_t last);
static int 			lxl_conf_read_token(lxl_conf_t *cf);

static ssize_t 		lxl_conf_parse_size(lxl_str_t *line);
static lxl_int_t 	lxl_conf_parse_time(lxl_str_t *line, lxl_uint_t is_sec);


extern lxl_module_t *lxl_modules[];

static lxl_uint_t argument_number[] = {
	LXL_CONF_NOARGS,
	LXL_CONF_TAKE1,
	LXL_CONF_TAKE2,
	LXL_CONF_TAKE3,
	LXL_CONF_TAKE4,
	LXL_CONF_TAKE5,
	LXL_CONF_TAKE6,
	LXL_CONF_TAKE7
};


char *
lxl_conf_parse(lxl_conf_t *cf, lxl_str_t *filename)
{
	int fd;
	lxl_int_t rc;
	lxl_buf_t buf;
	lxl_conf_file_t conf_file;
	enum {
		parse_file = 0,
		parse_block
	} type;

	if (filename) {
		fd = open(filename->data, O_RDONLY);
		if (fd == -1) {
			lxl_conf_log_error(LXL_LOG_EMERG, cf, errno, "open() \"%s\" failed", filename->data);
			return LXL_CONF_ERROR;
		}

		cf->conf_file = &conf_file;
		if (lxl_fd_info(fd, &cf->conf_file->file.info) == -1) {
			lxl_log_error(LXL_LOG_EMERG, errno, lxl_fd_info_n " \"%s\" failed", filename->data);
		}

		cf->conf_file->buffer = &buf;
		buf.start = lxl_alloc(LXL_CONF_BUFFER);
		if (buf.start == NULL) {
			goto failed;
		}

		buf.pos =  buf.start;
		buf.last = buf.start;
		buf.end = buf.last + LXL_CONF_BUFFER;
		cf->conf_file->line = 1;

		cf->conf_file->file.fd = fd;
		cf->conf_file->file.name.len = filename->len;
		cf->conf_file->file.name.data = filename->data;
		cf->conf_file->file.offset = 0;

		type = parse_file;
	} else {
		type = parse_block;
	}

	for (; ;) {
		rc = lxl_conf_read_token(cf);

#if 1
		lxl_uint_t i, nelts;
		lxl_str_t *value;
		nelts = lxl_array_nelts(cf->args);
		value = lxl_array_elts(cf->args);
		for (i = 0; i < nelts; ++i) {
			lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "args: index:%lu value:%s", i, value[i].data);
		}
#endif
		
		if (rc == LXL_ERROR) {
			goto done;
		}

		if (rc == LXL_CONF_BLOCK_DONE) {
			if (type != parse_block) {
				lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "unexpected end of file, unexpected \"}\"");
			}

			goto done;
		}

		if (rc == LXL_CONF_FILE_DONE) {
			if (type == parse_block) {
				lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "unexpected end of file, expecting \"}\"");
				goto failed;
			}

			goto done;
		}

		/*if (cf->handler) {
			
		}*/

		rc = lxl_conf_handler(cf, rc);
		
		if (rc == -1) {
			goto failed;
		}
	}

failed:
	
	rc = LXL_ERROR;

done:

	if (filename) {
		if (cf->conf_file->buffer->start) {
			lxl_free(cf->conf_file->buffer->start);
		} 

		if (close(fd) == -1) {
			lxl_log_error(LXL_LOG_ALERT, errno, "close() %s failed", filename->data);
			return LXL_CONF_ERROR;

		}
	}

	if (rc == LXL_ERROR) {
		return LXL_CONF_ERROR;
	}
	
	return LXL_CONF_OK;
	
}

static int 
lxl_conf_handler(lxl_conf_t *cf, lxl_int_t last)
{
	char *rv;
	void *conf, **confp;
	lxl_uint_t i, nelts, found;
	lxl_str_t *name;
	lxl_command_t *cmd;

	found = 0;
	name = (lxl_str_t *) lxl_array_elts(cf->args);

	for (i = 0; lxl_modules[i]; ++i) {
		cmd = lxl_modules[i]->commands;
		if (cmd == NULL) {
			continue;
		}

		for ( /* void */; cmd->name.len; ++cmd) {
			if (name->len == cmd->name.len && memcmp(name->data, cmd->name.data, name->len) == 0) {
				found = 1;
				if (lxl_modules[i]->type != cf->module_type) {
					continue;
				}

				/* is directive's location right ? */
				if (!(cmd->type & cf->cmd_type)) {
					continue;
				}

				if (!(cmd->type & LXL_CONF_BLOCK) && last != LXL_OK) {
					lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "directive \"%s\" is not terminated by \";\"", name->data);
					return -1;
				}

				if ((cmd->type & LXL_CONF_BLOCK) && last != LXL_CONF_BLOCK_START) {
					lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "directive \"%s\" has not opening \"{\"", name->data);
					return -1;
				}

				nelts = lxl_array_nelts(cf->args);
				if (cmd->type & LXL_CONF_FLAG) {
					if (nelts != 2) {
						goto invalid;
					}
				} else if (cmd->type & LXL_CONF_1MORE) {
					if (nelts < 2) {
						goto invalid;
					}
				} else if (cmd->type & LXL_CONF_2MORE) {
					if (nelts < 3) {
						goto invalid;
					}
				} else if (nelts > LXL_CONF_MAX_ARGS) {
					goto invalid;
				} else if (!(cmd->type & argument_number[nelts - 1])) {
					goto invalid;
				}

				conf = NULL;
				if (cmd->type & LXL_DIRECT_CONF) {
					conf = ((void **) cf->ctx)[lxl_modules[i]->index];
				} else if (cmd->type & LXL_MAIN_CONF) {
					conf = &(((void **) cf->ctx)[lxl_modules[i]->index]);
				} else if (cf->ctx) {
					confp = *(void **) ((char *) cf->ctx + cmd->conf);
					if (confp) {
						conf = confp[lxl_modules[i]->ctx_index];
					}
				}

				rv = cmd->set(cf, cmd, conf);

				if (rv == LXL_CONF_OK) {
					return 0;
				}

				if (rv == LXL_CONF_ERROR) {
					return -1;
				}

				lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "\"%s\" directive %s", name->data, rv);
				return -1;
			}
		}
	}

	if (found) {
		lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "\"%s\" directive not allowed here", name->data);
		return -1;
	}

	lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "unknow directive \"%s\"", name->data);

	return -1;

invalid:

	lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "invalid number of arguments in \"%s\"", name->data);

	return -1;
}

static int 
lxl_conf_read_token(lxl_conf_t *cf)
{
	char ch, *dst, *src, *start;
	off_t file_size;
	size_t len;
	ssize_t n, size;
	lxl_str_t *word;
	lxl_buf_t *b;
	lxl_uint_t found, start_line, last_space, sharp_comment;

	found = 0;
	last_space = 1;
	sharp_comment = 0;

	lxl_array_clear(cf->args);
	b = cf->conf_file->buffer;
	start = b->pos;
	start_line = cf->conf_file->line;

	file_size = lxl_file_size(&cf->conf_file->file.info);

	for (; ;) {
		if (b->pos >= b->last) {
			if (cf->conf_file->file.offset >= file_size) {
				if (cf->args->nelts > 0 || !last_space) {
					lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "unexpected end file, \";\" or \"}\"");
					return LXL_ERROR;
				}

				return LXL_CONF_FILE_DONE;
			}		

			len = b->pos - start;
			if (len == LXL_CONF_BUFFER) {
				cf->conf_file->line = start_line;
				lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "too long paraments \"%*s...\" started", start);
				return LXL_ERROR;
			}

			if (len) {
				memmove(b->start, start, len);
			}

			size = (ssize_t) (file_size - cf->conf_file->file.offset);
			if (size > b->end - (b->start + len)) {
				size = b->end - (b->start + len);
			}

			n = lxl_read_file(&cf->conf_file->file, b->start + len, size, cf->conf_file->file.offset);
			if (n == -1) {
				return -1;
			}

			if (n != size) {
				lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, lxl_read_file_n " returned only %ld bytes instead of %ld", n, size);
				return -1;
			}

			b->pos = b->start + len;
			b->last = b->pos + n;
			start = b->start;
		}
	
		ch = *b->pos++;
		if (ch == '\n') {
			cf->conf_file->line++;
			if (sharp_comment) {
				sharp_comment = 0;
			}
		}

		if (sharp_comment) {
			continue;
		}

		if (last_space) {
			if (ch == ' ' || ch == '\t' || ch == CR || ch == LF) {
				continue;
			}

			start = b->pos - 1;
			start_line = cf->conf_file->line;

			switch (ch) {
				case ';':
					if (lxl_array_nelts(cf->args) == 0) {
						lxl_conf_log_error(LXL_LOG_EMERG, cf, 0,  "unexpected \";\"");
						return LXL_ERROR;
					}

					return LXL_OK;
				
				case '{':
					if (lxl_array_nelts(cf->args) == 0) {
						lxl_conf_log_error(LXL_LOG_EMERG, cf, 0,  "unexpected \"{\"");
						return LXL_ERROR;
					}

					return LXL_CONF_BLOCK_START;

				case '}':
					if (lxl_array_nelts(cf->args) != 0) {
						lxl_conf_log_error(LXL_LOG_EMERG, cf, 0,  "unexpected \"}\"");
						return LXL_ERROR;
					}

					return LXL_CONF_BLOCK_DONE;
					
				case '#':
					sharp_comment = 1;
					continue;

				default:
					last_space = 0;
			}
		} else {
			if (ch == ' ' || ch == '\t' || ch == CR || ch == LF || ch == ';' || ch == '{') {
				last_space = 1;
				found = 1;
			}

			if (found) {
				word = lxl_array_push(cf->args);
				if (word == NULL) {
					return LXL_ERROR;
				}

				word->data = lxl_pnalloc(cf->pool, b->pos - start + 1);
				if (word == NULL) {
					return LXL_ERROR;
				}

				for (dst = word->data, src = start, len = 0; src < b->pos - 1; ++len) {
					*dst++ = *src++;
				}
				*dst = '\0';
				word->len = len;

				if (ch == ';') {
					return LXL_OK;
				}

				if (ch == '{') {
					return LXL_CONF_BLOCK_START;
				}

				found = 0;
			}
		}
	}
}

void 
lxl_conf_log_error(lxl_uint_t level, lxl_conf_t *cf, int err, const char *fmt, ...)
{
#define LXL_MAX_CONF_ERRSTR	1024

	va_list ap;
	char errstr[LXL_MAX_CONF_ERRSTR];

	va_start(ap, fmt);
	vsnprintf(errstr, LXL_MAX_CONF_ERRSTR, fmt, ap);
	va_end(ap);


	lxl_log_error1(level, err, "%s in %s:%lu", errstr, cf->conf_file->file.name.data, cf->conf_file->line);
}

char *
lxl_conf_set_flag_slot(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	char *p = conf;

	lxl_int_t *fp;
	lxl_str_t *value;
	
	fp = (lxl_int_t *) (p + cmd->offset);
	if (*fp != LXL_CONF_UNSET) {
		return "is duplicate";
	}

	value = lxl_array_elts(cf->args);
	if (strcmp(value[1].data, "on") == 0) {
		*fp = 1;
	} else if (strcmp(value[1].data, "off") == 0) {
		*fp = 0;
	} else {
		// lxl_conf_log_error(LXL_LOG_EMERG, cf);
		return LXL_CONF_ERROR;
	}
	
	return LXL_CONF_OK;
}

char *
lxl_conf_set_str_slot(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	char *p = conf;
	
	lxl_str_t *field, *value;

	field = (lxl_str_t *) (p + cmd->offset);
	if (field->data) {
		return "is duplicate";
	}

	value = lxl_array_elts(cf->args);
	*field = value[1];
	/*if (cmd->post) {
		post = cmd->post;
		return post->post_handle(cf, post, field);
	}*/

	return LXL_CONF_OK;
}

char *
lxl_conf_set_num_slot(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	char *p = conf;
	
	lxl_int_t *np;
	lxl_str_t *value;

	np = (lxl_int_t *) (p + cmd->offset);
	if (*np != LXL_CONF_UNSET) {
		return "is duplicate";
	}

	value = lxl_array_elts(cf->args);
	*np = atoi(value[1].data);
	if (*np <= 0) {
		return "invalid number";
	}
	/*if (cmd->post) {
		post = cmd->post;
		return post->post_handle(cf, post, field);
	}*/

	return LXL_CONF_OK;
}

char *
lxl_conf_set_size_slot(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	char *p = conf;

	size_t *sp;
	lxl_str_t *value;
	
	sp = (size_t *) (p + cmd->offset);
	if (*sp != LXL_CONF_UNSET_SIZE) {
		return "is duplicate";
	}

	value = lxl_array_elts(cf->args);
	*sp = lxl_conf_parse_size(&value[1]);
	if (*sp == (size_t) -1) {
		return "invalied value";
	}

	return LXL_CONF_OK;
}

char *
lxl_conf_set_msec_slot(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	char *p = conf;

	lxl_msec_t 	*msp;
	lxl_str_t *value;

	msp = (lxl_msec_t *) (p + cmd->offset);
	if (*msp != LXL_CONF_UNSET_MSEC) {
		return "is duplicate";
	}

	value = lxl_array_elts(cf->args);
	*msp = lxl_conf_parse_time(&value[1], 0);
	if (*msp == (lxl_msec_t) -1) {
		return "invalid value";
	}

	return LXL_CONF_OK;
}

char *
lxl_conf_set_sec_slot(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	char *p = conf;

	time_t *sp;
	lxl_str_t *value;
	
	sp = (time_t *) (p + cmd->offset);
	if (*sp != LXL_CONF_UNSET) {
		return "is duplicate";
	}

	value = lxl_array_elts(cf->args);
	*sp = lxl_conf_parse_time(&value[1], 1);
	if (*sp == (time_t) -1) {
		return "is invalid";
	}

	return LXL_CONF_OK;
}

static ssize_t 
lxl_conf_parse_size(lxl_str_t *line)
{
	char uint;
	size_t len, size;
	lxl_int_t scale;

	len = line->len;
	uint = line->data[len - 1];
	switch (uint) {
		case 'K':
		case 'k':
			--len;
			line->data[len] = '\0';
			scale = 1024;
			break;

		case 'M':
		case 'm':
			--len;
			line->data[len] = '\0';
			scale = 1024 * 1024;
			break;
	
		default:
			scale = 1;
	}

	size = atol(line->data);
	if (size <= 0) {
		return -1;
	}

	size *= scale;

	return  size;
}

static lxl_int_t 	
lxl_conf_parse_time(lxl_str_t *line, lxl_uint_t is_sec)
{
	char *p, *last;
	lxl_int_t value, total, scale;
	lxl_uint_t max, valid;
	enum {
		st_start = 0,
		st_year,
		st_month,
		st_week,
		st_day,
		st_hour,
		st_min,
		st_sec,
		st_msec,
		st_last
	} step;

	valid = 0;
	value = 0;
	total = 0;
	step = is_sec ? st_start : st_week;
	scale = is_sec ? 1 : 1000;
	p = line->data;
	last = p + line->len;
	while (p < last) {
		if (*p >= '0' && *p <= '9') {
			value = value * 10 + (*p - '0');
			++p;
			valid = 1;
			continue;
		}
	
		switch (*p++) {
			case 'y':
				if (step > st_start) {
					return -1;
				}
				step = st_year;
				max = LXL_MAX_INT32_VALUE / (60 * 60 * 24 * 365);
				scale = 60 * 60 * 24 * 365;
				break;

			case 'M':
				if (step > st_month) {
					return -1;
				}
				step = st_month;
				max = LXL_MAX_INT32_VALUE / (60 * 60 * 24 * 30);
				scale = 60 * 60 * 24 * 30;
				break;

			case 'w':
				step = st_week;
				max = LXL_MAX_INT32_VALUE / (60 * 60 * 24 * 7);
				scale = 60 * 60 * 24 * 7;
				break;
				
			case 'd':
				step = st_day;
				max = LXL_MAX_INT32_VALUE / (60 * 60 * 24);
				scale = 60 * 60 * 24;
				break;

			case 'h':
				step = st_hour;
				max = LXL_MAX_INT32_VALUE / (60 * 60);
				scale = 60 * 60;
				break;

			case 'm':
				if (*p == 's') {
					if (is_sec) {
						return -1;
					}
					p++;
					step = st_msec;
					max = LXL_MAX_INT32_VALUE;
					scale = 1;
					break;
				}

				step = st_min;
				max = LXL_MAX_INT32_VALUE / 60;
				scale = 60;
				break;

			case 's':
				step = st_sec;
				max = LXL_MAX_INT32_VALUE;
				scale = 1;

			case ' ':
				step = st_last;
				max = LXL_MAX_INT32_VALUE;
				scale = 1;
				break;
			
			default:
				return -1;
		}

		if (step != st_msec && !is_sec) {
			scale *= 1000;
			max /= 1000;
		}

		if ((lxl_uint_t) value > max) {
			return -1;
		}

		total += value * scale;
		if ((lxl_uint_t) total > LXL_MAX_INT32_VALUE) {
			return -1;
		}

		break;
	}
	
	if (valid) {
        return total;
    }

	return -1;
}
