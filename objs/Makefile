
CC = gcc
#CFLAGS = -g3 -Wall -DLXL_DEBUG
CFLAGS = -g -Wall 

ALL_INCS = -I src/core -I src/event -I src/os/unix -I src/dns

CORE_INCS = -I src/core -I src/event -I src/os/unix

CORE_DEPS = src/core/lxl_log.h \
		src/core/lxl_palloc.h \
		src/core/lxl_buf.h \
		src/core/lxl_file.h \
		src/core/lxl_inet.h \
		src/core/lxl_hash.h \
		src/core/lxl_list.h \
		src/core/lxl_slist.h \
		src/core/lxl_array.h \
		src/core/lxl_stack.h \
		src/core/lxl_rbtree.h \
		src/core/lxl_string.h \
		src/core/lxl_times.h \
		src/core/lxl_cycle.h \
		src/core/lxl_conf_file.h \
		src/core/lxl_connection.h \
		src/event/lxl_event.h \
		src/event/lxl_event_timer.h \
		src/os/unix/lxl_alloc.h \
		src/os/unix/lxl_files.h \
		src/os/unix/lxl_socket.h \
		src/os/unix/lxl_process.h \

DNS_INCS = -I src/dns

DNS_DEPS = src/dns/lxl_dns.h \
		src/dns/lxl_dns_data.h \
		src/dns/lxl_dns_request.h \
		src/dns/lxl_dns_upstream.h 	


SPEED_SRC = src/core/speed.c \
			src/core/lxl_log.c \
			src/core/lxl_palloc.c \
			src/core/lxl_buf.c \
			src/core/lxl_file.c \
			src/core/lxl_inet.c \
			src/core/lxl_hash.c \
			src/core/lxl_array.c \
			src/core/lxl_rbtree.c \
			src/core/lxl_times.c \
			src/core/lxl_string.c \
			src/core/lxl_cycle.c \
			src/core/lxl_conf_file.c \
			src/core/lxl_connection.c \
			src/event/lxl_event.c \
			src/event/lxl_event_timer.c \
			src/event/lxl_epoll_module.c \
			src/os/unix/lxl_files.c	\
			src/os/unix/lxl_socket.c \
			src/os/unix/lxl_process.c \
			src/dns/lxl_dns.c \
			src/dns/lxl_dns_core_module.c \
			src/dns/lxl_dns_data.c \
			src/dns/lxl_dns_request.c \
			src/dns/lxl_dns_parse.c	\
			src/dns/lxl_dns_upstream.c \
			src/dns/lxl_dns_package.c

all: objs/speed

objs/speed: objs/src/core/speed.o \
	objs/src/core/lxl_log.o \
	objs/src/core/lxl_palloc.o \
	objs/src/core/lxl_buf.o \
	objs/src/core/lxl_file.o \
	objs/src/core/lxl_inet.o \
	objs/src/core/lxl_times.o \
	objs/src/core/lxl_string.o \
	objs/src/core/lxl_hash.o \
	objs/src/core/lxl_array.o \
	objs/src/core/lxl_rbtree.o \
	objs/src/core/lxl_cycle.o \
	objs/src/core/lxl_conf_file.o \
	objs/src/core/lxl_connection.o \
	objs/src/event/lxl_event.o \
	objs/src/event/lxl_event_timer.o \
	objs/src/event/lxl_epoll_module.o \
	objs/src/os/unix/lxl_files.o \
	objs/src/os/unix/lxl_socket.o \
	objs/src/os/unix/lxl_process.o \
	objs/src/dns/lxl_dns.o \
	objs/src/dns/lxl_dns_core_module.o \
	objs/src/dns/lxl_dns_data.o \
	objs/src/dns/lxl_dns_request.o \
	objs/src/dns/lxl_dns_parse.o \
	objs/src/dns/lxl_dns_upstream.o \
	objs/src/dns/lxl_dns_package.o	

	$(CC) -o objs/speed \
	objs/src/core/speed.o \
	objs/src/core/lxl_log.o \
	objs/src/core/lxl_palloc.o \
	objs/src/core/lxl_buf.o \
	objs/src/core/lxl_file.o \
	objs/src/core/lxl_inet.o \
	objs/src/core/lxl_times.o \
	objs/src/core/lxl_string.o \
	objs/src/core/lxl_hash.o \
	objs/src/core/lxl_array.o \
	objs/src/core/lxl_rbtree.o \
	objs/src/core/lxl_cycle.o \
	objs/src/core/lxl_conf_file.o \
	objs/src/core/lxl_connection.o \
	objs/src/event/lxl_event.o \
	objs/src/event/lxl_event_timer.o \
	objs/src/event/lxl_epoll_module.o \
	objs/src/os/unix/lxl_files.o \
	objs/src/os/unix/lxl_socket.o \
	objs/src/os/unix/lxl_process.o \
	objs/src/dns/lxl_dns.o \
	objs/src/dns/lxl_dns_core_module.o \
	objs/src/dns/lxl_dns_data.o \
	objs/src/dns/lxl_dns_request.o \
	objs/src/dns/lxl_dns_parse.o \
	objs/src/dns/lxl_dns_upstream.o \
	objs/src/dns/lxl_dns_package.o


objs/src/core/speed.o: $(CORE_DEPS) src/core/speed.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/core/speed.o src/core/speed.c

objs/src/core/lxl_log.o: $(CORE_DEPS) src/core/lxl_log.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/core/lxl_log.o src/core/lxl_log.c

objs/src/core/lxl_palloc.o: $(CORE_DEPS) src/core/lxl_palloc.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/core/lxl_palloc.o src/core/lxl_palloc.c

objs/src/core/lxl_buf.o: $(CORE_DEPS) src/core/lxl_buf.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/core/lxl_buf.o src/core/lxl_buf.c

objs/src/core/lxl_file.o: $(CORE_DEPS) src/core/lxl_file.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/core/lxl_file.o src/core/lxl_file.c

objs/src/core/lxl_inet.o: $(CORE_DEPS) src/core/lxl_inet.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/core/lxl_inet.o src/core/lxl_inet.c

objs/src/core/lxl_times.o: $(CORE_DEPS) src/core/lxl_times.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/core/lxl_times.o src/core/lxl_times.c

objs/src/core/lxl_string.o: $(CORE_DEPS) src/core/lxl_string.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/core/lxl_string.o src/core/lxl_string.c

objs/src/core/lxl_hash.o: $(CORE_DEPS) src/core/lxl_hash.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/core/lxl_hash.o src/core/lxl_hash.c

objs/src/core/lxl_array.o: $(CORE_DEPS) src/core/lxl_array.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/core/lxl_array.o src/core/lxl_array.c

objs/src/core/lxl_rbtree.o: $(CORE_DEPS) src/core/lxl_rbtree.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/core/lxl_rbtree.o src/core/lxl_rbtree.c

objs/src/core/lxl_cycle.o: $(CORE_DEPS) src/core/lxl_cycle.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/core/lxl_cycle.o src/core/lxl_cycle.c

objs/src/core/lxl_conf_file.o: $(CORE_DEPS) src/core/lxl_conf_file.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/core/lxl_conf_file.o src/core/lxl_conf_file.c

objs/src/core/lxl_connection.o: $(CORE_DEPS) src/core/lxl_connection.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/core/lxl_connection.o src/core/lxl_connection.c

objs/src/event/lxl_event.o: $(CORE_DEPS) src/event/lxl_event.c 
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/event/lxl_event.o src/event/lxl_event.c

objs/src/event/lxl_event_timer.o: $(CORE_DEPS) src/event/lxl_event_timer.c 
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/event/lxl_event_timer.o src/event/lxl_event_timer.c

objs/src/event/lxl_epoll_module.o: $(CORE_DEPS) src/event/lxl_epoll_module.c 
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/event/lxl_epoll_module.o src/event/lxl_epoll_module.c

objs/src/os/unix/lxl_files.o: $(CORE_DEPS) src/os/unix/lxl_files.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/os/unix/lxl_files.o src/os/unix/lxl_files.c

objs/src/os/unix/lxl_socket.o: $(CORE_DEPS) src/os/unix/lxl_socket.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/os/unix/lxl_socket.o src/os/unix/lxl_socket.c

objs/src/os/unix/lxl_process.o: $(CORE_DEPS) src/os/unix/lxl_process.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) -o objs/src/os/unix/lxl_process.o src/os/unix/lxl_process.c

objs/src/dns/lxl_dns.o: $(CORE_DEPS) $(DNS_DEPS) src/dns/lxl_dns.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(DNS_INCS) -o objs/src/dns/lxl_dns.o src/dns/lxl_dns.c 

objs/src/dns/lxl_dns_core_module.o: $(CORE_DEPS) $(DNS_DEPS) src/dns/lxl_dns_core_module.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(DNS_INCS) -o objs/src/dns/lxl_dns_core_module.o src/dns/lxl_dns_core_module.c 

objs/src/dns/lxl_dns_data.o: $(CORE_DEPS) $(DNS_DEPS) src/dns/lxl_dns_data.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(DNS_INCS) -o objs/src/dns/lxl_dns_data.o src/dns/lxl_dns_data.c 
objs/src/dns/lxl_dns_parse.o: $(CORE_DEPS) $(DNS_DEPS) src/dns/lxl_dns_parse.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(DNS_INCS) -o objs/src/dns/lxl_dns_parse.o src/dns/lxl_dns_parse.c 

objs/src/dns/lxl_dns_request.o: $(CORE_DEPS) $(DNS_DEPS) src/dns/lxl_dns_request.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(DNS_INCS) -o objs/src/dns/lxl_dns_request.o src/dns/lxl_dns_request.c 

objs/src/dns/lxl_dns_upstream.o: $(CORE_DEPS) $(DNS_DEPS) src/dns/lxl_dns_upstream.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(DNS_INCS) -o objs/src/dns/lxl_dns_upstream.o src/dns/lxl_dns_upstream.c 

objs/src/dns/lxl_dns_package.o: $(CORE_DEPS) $(DNS_DEPS) src/dns/lxl_dns_package.c
	$(CC) -c $(CFLAGS) $(CORE_INCS) $(DNS_INCS) -o objs/src/dns/lxl_dns_package.o src/dns/lxl_dns_package.c 


#speed: $(SPEED_SRC) $(CORE_DEPS)
#	$(CC) $(CFLAGS) $(ALL_INCS) $(SPEED_SRC) -o $@
#	cp $@ ~/bin;
	

test: lxl_log_test 

LOG_SRC 	= src/core/lxl_log.c  src/core/lxl_times.c
ARRAY_SRC 	= src/core/lxl_array.c src/core/lxl_log.c	src/core/lxl_palloc.c
LIST_SRC	= src/core/lxl_log.c 
SLIST_SRC	= src/core/lxl_log.c 
QUEUE_SRC 	= src/core/lxl_queue.c src/core/lxl_log.c 	src/core/lxl_palloc.c
STACK_SRC 	= src/core/lxl_log.c src/core/lxl_palloc.c	src/core/lxl_stack.c
STRING_SRC 	= src/core/lxl_log.c src/core/lxl_string.c
HASH_SRC 	= src/core/lxl_log.c src/core/lxl_palloc.c 	src/core/lxl_hash.c

lxl_log_test: test/core/lxl_log_test.c $(LOG_SRC)
	$(CC) $(CFLAGS) $(ALL_INCS) $(LOG_SRC) $< -o objs/test/core/$@
	
lxl_log_test1: $(SPEED_SRC)  test/core/lxl_log_test.c
		$(CC) $(ALL_INCS)  $^ -o logtest	
lxl_pool_test: $(SPEED_SRC) test/core/lxl_pool_test.c
        $(CC) $(ALL_INCS)  $^ -o objs/test/core/$@
lxl_array_test: test/core/lxl_array_test.c $(ARRAY_SRC)
	 $(CC) $(CFLAGS) $(ALL_INCS) $(ARRAY_SRC) $< -o objs/test/core/$@

lxl_list_test: test/core/lxl_list_test.c $(LIST_ARC)
	$(CC) $(CFLAGS) $(ALL_INCS) $(LIST_SRC) $< -o objs/test/core/$@

lxl_slist_test: test/core/lxl_slist_test.c $(SLIST_ARC)
	$(CC) $(CFLAGS) $(ALL_INCS) $(SLIST_SRC) $< -o objs/test/core/$@

lxl_queue_test: test/core/lxl_queue_test.c $(QUEUE_SRC)
	 $(CC) $(CFLAGS) $(ALL_INCS) $(QUEUE_SRC) $< -o objs/test/core/$@

lxl_stack_test: test/core/lxl_stack_test.c $(STACK_SRC)
	 $(CC) $(CFLAGS) $(ALL_INCS) $(STACK_SRC) $< -o objs/test/core/$@

lxl_string_test: test/core/lxl_string_test.c $(STRING_SRC)
	 $(CC) $(CFLAGS) $(ALL_INCS) $(STRING_SRC) $< -o objs/test/core/$@

lxl_hash_test: test/core/lxl_hash_test.c $(HASH_SRC)
	$(CC) $(CFLAGS) $(ALL_INCS) $(HASH_SRC) $< -o  objs/test/core/$@

clean: 
	rm -rf speed objs/src objs/test;
