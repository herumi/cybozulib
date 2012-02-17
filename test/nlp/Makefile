include ../../common.mk

TARGET=$(TEST_FILE)
#LIBS=$(TOPDIR)lib/libcybozu_base$(OBJSUF).a
LIBS=

SRC=$(wildcard *.cpp)

all: $(TARGET)

test: $(TARGET)
	@$(UNIT_TEST)

$(OBJDIR):
	@$(MKDIR) $(OBJDIR)

clean:
	$(CLEAN)

$(LIBS):
	make -C ../../src

-include $(DEPEND_FILE)

