
#include <lxl_stack.h>


void lxl_stack_sample_ptr(void);

int main(int argc, char *argv[])
{
	lxl_log_t *log;
	lxl_pool_t *pool;
	lxl_stack_t stack;

	log = lxl_log_init(LXL_LOG_DEBUG, LXL_LOG_FLUSH);
	pool = lxl_pool_create(4096);
	lxl_stack_init(&stack, pool, 20, sizeof(int));
	
	int i;
	int *elt;
	for (i = 0; i < 24; ++i) {
		elt = lxl_stack_push(&stack);
		*elt = i + 50;
	}
	elt = lxl_stack_top(&stack);
	lxl_log(LXL_LOG_DEBUG, 0, "%d", *(int *) elt);
	for (i = 0; i < 30; ++i) {
		elt = lxl_stack_pop(&stack);
		if (elt != NULL) {
			lxl_log(LXL_LOG_DEBUG, 0, "%d", *(int *) elt);
		} else {
			lxl_log(LXL_LOG_DEBUG, 0, "nil");
		}
	}

	lxl_log(LXL_LOG_DEBUG, 0, "stack sample pointer");
	lxl_stack_sample_ptr();

	return 0;
}

void
lxl_stack_sample_ptr()
{
	int i, **elt, *ptr;
	lxl_stack_t intptr_stack;

	lxl_stack_init(&intptr_stack, NULL, 10, sizeof(int *));

	for (i = 0; i < 10; ++i) {
		ptr = lxl_alloc(sizeof(int *));
		*ptr = i + 4;
		elt = lxl_stack_push(&intptr_stack);
		*elt = ptr;
	}

	for (i = 0; i < 10; ++i) {
		elt = lxl_stack_pop(&intptr_stack);
		lxl_log(LXL_LOG_DEBUG, 0, "%p %d", *elt, **elt);
	}
}
