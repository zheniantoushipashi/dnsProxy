
#include <lxl_times.h>
#include <lxl_log.h>


int main(int argc, char *argv[])
{
	if (lxl_strerror_init() == -1) {
		return 1;
	}

	lxl_time_init();

	//lxl_log_t *log = lxl_log_init(LXL_LOG_DEBUG, 0, LXL_LOG_FLUSH);
	lxl_log_t *log = lxl_log_init(LXL_LOG_DEBUG, 0, LXL_LOG_BUFFER,"/logs");
	int i;
    int m = 5
	for (i = 0; i < 180; ++i) {
        lxl_log_error(LXL_LOG_DEBUG, i,  "this i is %d , m is  %d",i, m);
		//lxl_log_error(LXL_LOG_DEBUG, i, "lxl log test %d", i);
	}

	return 0;
}
