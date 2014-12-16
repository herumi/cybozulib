GCC_VER=$(shell $(PRE)$(CC) -dumpversion)
UNAME_S=$(shell uname -s)
ifeq ($(UNAME_S),Linux)
  OS=Linux
endif
ifeq ($(UNAME_S),Darwin)
  CFLAGS+=-std=c++11 # clang
else
  LDFLAGS += -lrt
endif
CP = cp -f
AR = ar r
MKDIR=mkdir -p
RM=rm -fr
CFLAGS_OPT+=-fomit-frame-pointer -DNDEBUG
ifeq ($(CXX),clang++)
  CFLAGS_OPT+=-O3
else
  ifeq ($(shell expr $(GCC_VER) \> 4.6.0),1)
    CFLAGS_OPT+=-Ofast
  else
    CFLAGS_OPT+=-O3
  endif
endif
CFLAGS_WARN=-Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wfloat-equal -Wpointer-arith
CFLAGS+= -g -D_FILE_OFFSET_BITS=64
CFLAGS+=$(CFLAGS_WARN)
BIT?=64
ifeq ($(BIT),0)
	BIT_OPT=
else
	BIT_OPT=-m$(BIT)
endif
CFLAGS+= $(BIT_OPT)
LDFLAGS+= $(BIT_OPT)
ifeq ($(MARCH),)
ifeq ($(shell expr $(GCC_VER) \> 4.2.1),1)
	CFLAGS+=-march=native
endif
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
  ifeq ($(OS),Linux)
    LDFLAGS+=-rdynamic
  endif
  OBJDIR=debug
  OBJSUF=d
endif

TOPDIR:=$(realpath $(dir $(lastword $(MAKEFILE_LIST))))/
EXTDIR:=$(TOPDIR)../cybozulib_ext/

####################################################

CFLAGS+= -I$(TOPDIR)include
LDFLAGS += -lz -lpthread -lssl -lcrypto
HAS_BOOST=$(shell echo "\#include <boost/version.hpp>" | ($(CXX) -E - 2>/dev/null) | grep "boost/version.hpp" >/dev/null && echo "1")
HAS_MECAB=$(shell echo "\#include <mecab.h>" | ($(CXX) -E - 2>/dev/null) | grep "mecab.h" >/dev/null && echo "1")
HAS_EIGEN=$(shell echo "\#include <eigen3/Eigen/Sparse>" | ($(CXX) -E - 2>/dev/null) | grep "eigen3/Eigen/Sparse" >/dev/null && echo "1")
ifeq ($(HAS_BOOST),1)
  LDFLAGS += -lboost_regex
endif
ifeq ($(HAS_MECAB),1)
  LDFLAGS += -lmecab
endif

####################################################

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

