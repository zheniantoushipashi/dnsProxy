
default: build

clean: 
	rm -f objs/speed
	rm -rf objs/src objs/test

build:
	sh auto/dir;
	$(MAKE)	-f objs/Makefile
