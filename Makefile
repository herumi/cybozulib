include common.mk

all:
	$(MKDIR) lib bin
	$(MAKE) -C src
	$(MAKE) -C test
	$(MAKE) -C sample

test:
	$(MAKE) -C test test

clean:
	$(MAKE) -C src clean
	$(MAKE) -C test clean
	$(MAKE) -C sample clean

