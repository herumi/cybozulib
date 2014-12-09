GCC_VER=$(shell $(PRE)$(CC) -dumpversion)
ifeq ($(shell expr $(GCC_VER) \> 4.2.1),1)
  CFLAGS_OPT +=-march=native
endif
ifeq ($(shell uname -s),Linux)
  LDFLAGS += -lrt
endif
HAS_BOOST=$(shell echo "\#include <boost/version.hpp>" | (gcc -E - 2>/dev/null) | grep "boost/version.hpp" >/dev/null && echo "1")
HAS_MECAB=$(shell echo "\#include <mecab.hpp>" | (gcc -E - 2>/dev/null) | grep "mecab.hpp" >/dev/null && echo "1")
CP = cp -f
AR = ar r
MKDIR=mkdir -p
RM=rm -fr
CFLAGS_OPT += -O3 -fomit-frame-pointer -DNDEBUG
CFLAGS_WARN=-Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wfloat-equal -Wpointer-arith #-Wswitch-enum -Wstrict-aliasing=2
CFLAGS+= -g -D_FILE_OFFSET_BITS=64
CFLAGS+=$(CFLAGS_WARN)
LDFLAGS += -lz -lpthread -lssl -lcrypto
BIT?=64
ifeq ($(BIT),0)
	BIT_OPT=
else
	BIT_OPT=-m$(BIT)
endif
ifeq ($(MARCH),)
	CFLAGS+=-march=native
else
	CFLAGS+=$(MARCH)
endif

DEBUG=1
ifeq ($(RELEASE),1)
	DEBUG=0
endif

ifeq ($(DEBUG),0)
	CFLAGS+=$(CFLAGS_OPT)
	OBJDIR=release
	OBJSUF=
else
	LDFLAGS+=-rdynamic
	OBJDIR=debug
	OBJSUF=d
endif
ifeq ($(HAS_BOOST),1)
	LDFLAGS += -lboost_regex
endif
ifeq ($(HAS_MECAB),1)
	LDFLAGS += -lmecab
endif

TOPDIR:=$(realpath $(dir $(lastword $(MAKEFILE_LIST))))/
EXTDIR:=$(TOPDIR)../cybozulib_ext/
CFLAGS+= -I$(TOPDIR)include $(BIT_OPT)
LDFLAGS+= -L$(TOPDIR)lib $(BIT_OPT) -Wl,-rpath,'$$ORIGIN/../lib'

MKDEP = sh -ec '$(PRE)$(CC) -MM $(CFLAGS) $< | sed "s@\($*\)\.o[ :]*@$(OBJDIR)/\1.o $@ : @g" > $@; [ -s $@ ] || rm -f $@; touch $@'

CLEAN=$(RM) $(TARGET) $(OBJDIR)

define UNIT_TEST
sh -ec 'for i in $(TARGET); do $$i|grep "ctest:name"; done' > result.txt
grep -v "ng=0, exception=0" result.txt || echo "all unit tests are ok"
endef

define SAMPLE_TEST
sh -ec 'for i in $(TARGET); do $$i; done'
endef

.SUFFIXES: .cpp .d .exe

$(OBJDIR)/%.o: %.cpp
	$(PRE)$(CXX) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/%.d: %.cpp $(OBJDIR)
	@$(MKDEP)

$(TOPDIR)bin/%$(OBJSUF).exe: $(OBJDIR)/%.o $(LIBS)
	$(PRE)$(CXX) $< -o $@ $(LIBS) $(LDFLAGS)

OBJS=$(addprefix $(OBJDIR)/,$(SRC:.cpp=.o))

DEPEND_FILE=$(addprefix $(OBJDIR)/, $(SRC:.cpp=.d))
TEST_FILE=$(addprefix $(TOPDIR)bin/, $(SRC:.cpp=$(OBJSUF).exe))

.PHONY: test

