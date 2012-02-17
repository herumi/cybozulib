include common.mk

all:
	$(MKDIR) lib bin
	make -C src
	make -C test
	make -C sample

test:
	make -C test test

clean:
	make -C src clean
	make -C test clean
	make -C sample clean

