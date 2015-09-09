#include <stdio.h>
#include "lxl_core.h"
#include "lxl_string.h"
#include "lxl_config.h"
#include "lxl_conf_file.h"

void  dump_pool(lxl_pool_t* pool) {
	while(pool) {
		printf("pool = 0x%x\n", pool);  
        printf("  .d\n");  
        printf("    .last = 0x%x\n", pool->d.last);  
        printf("    .end = 0x%x\n", pool->d.end);  
        printf("    .next = 0x%x\n", pool->d.next);  
        printf("    .failed = %d\n", pool->d.failed);  
        printf("  .max = %d\n", pool->max);  
        printf("  .current = 0x%x\n", pool->current);  
       // printf("  .chain = 0x%x\n", pool->chain);  
        printf("  .large = 0x%x\n", pool->large);  
        printf("  .cleanup = 0x%x\n", pool->cleanup);  
     //   printf("  .log = 0x%x\n", pool->log);  
        printf("available pool memory = %d\n\n", pool->d.end - pool->d.last);  
        pool = pool->d.next;  
	}
}


int main() {
	lxl_pool_t  * pool;
	printf("---------------------------------\n");
	printf("create a new pool\n");
	printf("---------------------------------\n");
	pool = ngx_create_pool(1024);
    dump_pool(pool); 
}