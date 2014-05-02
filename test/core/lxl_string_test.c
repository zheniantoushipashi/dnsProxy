
#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_palloc.h>
#include <lxl_string.h>


int main(int argc, char *argv[])
{
	lxl_log_t *log;
	lxl_pool_t *pool;

	log = lxl_log_init(LXL_LOG_DEBUG, LXL_LOG_FLUSH);
	pool = lxl_pool_create(4096, log);

	lxl_str_t aa = lxl_null_string;
	char *a = "123456";
	lxl_str_t b = lxl_string("123456");
	lxl_str_t c = lxl_null_string, d = lxl_null_string;
/*	lxl_str_newlen(&c, 8);
	lxl_str_newdata(&d, "abcdefg");
	lxl_str_setdata(&c, a);
	lxl_str_free(&c);
	lxl_str_free(&d);*/

	lxl_str_t x = lxl_string("uuuuuuuuuu");
	lxl_str_t xx = lxl_string("iiiiiiiiiiiiii");
	x = xx;

	lxl_str_t s1 = lxl_null_string, s2 = lxl_null_string;
	s1.nalloc = 10;
	s1.data = (char *) lxl_alloc(s1.nalloc, log);
	s2.nalloc = 10;
	s2.data = (char *) lxl_palloc(pool, 10);
	//lxl_str_free(&s1);

	return 0;
}
