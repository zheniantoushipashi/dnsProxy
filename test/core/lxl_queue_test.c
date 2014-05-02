
#include <lxl_queue.h>

int main(int argc, char *argv[])
{
	lxl_log_t *log;
	lxl_pool_t *pool;
	lxl_queue_t *queue;

	log = lxl_log_init(LXL_LOG_DEBUG, LXL_LOG_FLUSH);
	pool = lxl_pool_create(LXL_DEFAULT_POOL_SIZE, log);
	queue = lxl_queue_create(pool, 20, sizeof(int));

	int i;
	int *elt;
	for (i = 0; i < 30; ++i) {
		elt = lxl_queue_in(queue);
		*elt =  i + 50;
	}

	for (i = 0; i < 40; ++i) {
		elt = lxl_queue_out(queue);
		if (elt != NULL) {
			fprintf(stderr, "%d\n", *(int *)elt);
		} else {
			fprintf(stderr, "nil\n");
		}
	}

	return 0;
}
