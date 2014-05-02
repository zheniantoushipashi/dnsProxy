
/*
 * Copyright (c) xianliang.li
 */


#include <lxl_string.h>


#if 0
int 
lxl_str_newlen(lxl_str_t *str, size_t len)
{
	/* str->data = (char *) malloc(len * sizeof(char));	*/
	str->data = (char *) malloc(len);	
	if (str->data == NULL) {
		return -1;
	}

	str->len = 0;
	str->nalloc = len;

	return 0;
}

int
lxl_str_newdata(lxl_str_t *str, char *data)
{
	if (!data) {
		return -1;
	}

	size_t len = strlen(data);	/* data not null */
	//if (str->nalloc == 0) {
		str->data = (char *) malloc(len);
		if (!str->data) {
			return -1;
		}

		str->len = str->nalloc = len;
		lxl_memcpy(str->data, data, len);
	/*} else {
		if (str->nalloc < len) {
			str->data = (char *) realloc(str->data, len);
			if (!str->data) {
				// If realloc() fails the original block is left untouched; it is not freed or moved. 
				return -1;
			}

			str->len = str->nalloc = len;
			lxl_memcpy(str->data, data, len);
		} else {
			str->len = len;
			lxl_memcpy(str->data, data, len);
		}
	}*/
	
	return 0;
}

int 
lxl_str_renewdata(lxl_str_t *str, char *data)
{
	lxl_str_free(str);
	return lxl_str_newdata(str, data);
}
#endif
/*void 
lxl_strlower(char *data, size_t len)
{
	while (len--) {
		data[len] = lxl_tolower(data[len]);
	}
}

void 
lxl_strupper(char *data, size_t len)
{
	while (len--) {
		data[len] = lxl_toupper(data[len]);
	}
}*/

/*char *
strstrall(str_t *parent, str_t *child)
{
	int pos = parent->len - child->len;
	if (pos < 0) {
		return NULL;
	}
	
	if (strncasecmp(parent->data+pos, child->data, child->len) == 0) {
		return parent->data+pos;
	}

	return NULL;
}*/

void 
lxl_strtrim(char *buf, char *cset)
{
	lxl_int_t  len;
	char *start, *end, *sp, *ep;

	start = sp = buf;
	end = ep = buf + strlen(buf) - 1;
	while (sp <= end && strchr(cset, *sp)) {
		++sp;
	}

	while (ep > start && strchr(cset, *ep)) {
		--ep;
	}

	len = ep - sp;
	len += 1;	// yao 
	if (len > 0) {
		memmove(buf, sp, len);
		buf[len] = '\0';
	} else {
		buf[0] = '\0';
	}
}
