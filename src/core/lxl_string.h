
/*
 * Copyright (c) xianliang.li
 */


#ifndef LXL_STRING_H_INCLUDE
#define LXL_STRING_H_INCLUDE


#include <lxl_config.h>


typedef struct {
	size_t len;
	char *data;
} lxl_str_t;


#define lxl_string(text)			{ sizeof(text) - 1, (char *) text }		
#define lxl_null_string				{ 0, NULL }
#define lxl_str_set(str, text)		(str)->len = sizeof(text) -1; (str)->data = (char *) text
#define lxl_str_null(str)			(str)->len = 0; (str)->data = NULL
#define	lxl_tolower(c)				((c >= 'A' && c <= 'Z') ? (c | 0x20) : c)
#define lxl_toupper(c)				((c >= 'a' && c <= 'z') ? (c & ~0x20) : c)

#define lxl_strlower(s, n)									\
	{														\
		size_t lxl_strlower_n = (n);						\
		char *lxl_strlower_s = (char *) (s);				\
		while (lxl_strlower_n--) {							\
			lxl_strlower_s[lxl_strlower_n] 					\
			= lxl_tolower(lxl_strlower_s[lxl_strlower_n]);	\
		}													\
	}

#define lxl_strupper(s, n)									\
	{														\
		size_t lxl_strupper_n = (n);						\
		char *lxl_strupper_s = (char *) (s);				\
		while (lxl_strupper_n--) {							\
			lxl_strupper_s[lxl_strupper_n] 					\
			= lxl_toupper(lxl_strupper_s[lxl_strupper_n]);	\
		}													\
	}

#define lxl_str4cmp(s, c0, c1, c2, c3)						\
	s[0] == c0 && s[1] == c1 && s[2] == c2 && s[3] == c3

#define lxl_str16cmp(s, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15)	\
	s[0] == c0 && s[1] == c1 && s[2] == c2 && s[3] == c3 										\
		&& s[4] == c4 && s[5] == c5	&& s[6] == c6 && s[7] == c7									\
		&& s[8] == c8 && s[9] == c9 && s[10] == c10 && s[11] == c11								\
		&& s[12] == c12 && s[13] == c13 && s[14] == c14 && s[15] == c15	

/* must n xiang deng */
static inline lxl_int_t 
lxl_strncmp(char *s1, char *s2, size_t n)
{
	lxl_uint_t c1, c2;

    while (n) {
        c1 = (lxl_uint_t) *s1++;
        c2 = (lxl_uint_t) *s2++;
        if (c1 == c2) {
			--n;
			continue;
		}

		return c1 - c2;
	}

	return 0;
}

static inline lxl_int_t 
lxl_strncasecmp(char *s1, char *s2, size_t n)
{
	lxl_uint_t  c1, c2;

    while (n) {
        c1 = (lxl_uint_t) *s1++;
        c2 = (lxl_uint_t) *s2++;
        c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;
        c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;
        if (c1 == c2) {
			--n;
			continue;
		}

		return c1 - c2;
	}

	return 0;
}

/*
	{}  or {}; 
*/
#define lxl_memcpy(d, s, n)						\
	{											\
		size_t lxl_memcpy_n = (n);				\
		char *lxl_memcpy_d = (char *) (d);		\
		char *lxl_memcpy_s = (char *) (s);		\
		while (lxl_memcpy_n--) {				\
			*lxl_memcpy_d++ = *lxl_memcpy_s++;	\
		}										\
	}				


/*char *		strstrall(str_t *parent, str_t *child);*/
void lxl_strtrim(char *buf, char *cset);


#endif	/* LXL_STRING_H_INCLUDE */
