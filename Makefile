DIRS = $(dir $(wildcard */Makefile))

default :
	for dir in $(DIRS) ; do $(MAKE) $(MAKECMDGOALS) -C $$dir ; done

clean debug clear remake: default