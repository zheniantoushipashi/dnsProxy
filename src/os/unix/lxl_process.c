
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_log.h>
#include <lxl_times.h>
#include <lxl_cycle.h>
#include <lxl_event.h>
#include <lxl_process.h>


static void lxl_start_worker_process(lxl_cycle_t *cycle, lxl_int_t n);
static void lxl_worker_process_cycle(lxl_cycle_t *cycle, lxl_int_t worker);
static void lxl_worker_process_init(lxl_cycle_t *cycle, lxl_int_t worker);


void 
lxl_master_process_cycle(lxl_cycle_t *cycle)
{
	lxl_core_conf_t *ccf;

	ccf = (lxl_core_conf_t *) lxl_get_conf(cycle->conf_ctx, lxl_core_module);
	lxl_start_worker_process(cycle, ccf->worker_process);
	lxl_msleep(100);
	
	for (; ;) {
		lxl_sleep(1);
	}
}

static void
lxl_start_worker_process(lxl_cycle_t *cycle, lxl_int_t n)
{
	pid_t pid;
	lxl_int_t i;
	
	lxl_log_error(LXL_LOG_INFO, 0, "start worker process");
	for (i = 0; i < n; ++i) {
		pid = fork();
		switch (pid) {
			case -1:
				lxl_log_error(LXL_LOG_ALERT, errno, "fork() failed");
				lxl_log_flush();
				return;

			case 0:
				lxl_pid = getpid();
				lxl_worker_process_cycle(cycle, i);

			default:
				break;
		}
	}
}

static void 
lxl_worker_process_cycle(lxl_cycle_t *cycle, lxl_int_t worker)
{
	lxl_worker_process_init(cycle, worker);
	
	for (; ;) {
		lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "worker cycle");
		lxl_process_events_and_timers(cycle);
	}
}


static void 
lxl_worker_process_init(lxl_cycle_t *cycle, lxl_int_t worker)
{
	lxl_uint_t i;

	lxl_log_error(LXL_LOG_INFO, 0, "worker process init");

	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->init_process) {
			if (lxl_modules[i]->init_process(cycle) == -1) {
				/* fatal */
				lxl_log_flush();
				exit(2);
			}
		}
	}
}

