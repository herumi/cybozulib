CXX = g++ -x c++ -c
LD = g++
#CXX = clang++ -x c++ -c
#LD = clang++
CC = gcc -c
CP = cp -f
AR = ar r
MKDIR=mkdir -p
RM=rm -fr
CFLAGS_OPT = -O3 -fomit-frame-pointer -DNDEBUG 
#CFLAGS_WARN=-Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith #-Wswitch-enum -Wstrict-aliasing=2
CFLAGS_WARN=-Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wfloat-equal -Wpointer-arith #-Wswitch-enum -Wstrict-aliasing=2
CFLAGS = -g -D_FILE_OFFSET_BITS=64 -msse2 -march=native
CFLAGS+=$(CFLAGS_WARN)
LDFLAGS = 

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

TOPDIR=$(shell 'pwd' | sed "s@cybozulib/.*@cybozulib/@")
EXTDIR=$(TOPDIR)../cybozulib_ext/
#CFLAGS+= -I$(TOPDIR)include -I$(EXTDIR)icu4c/icu/include -I$(EXTDIR)openssl/openssl/include
#LDFLAGS+= -L$(TOPDIR)lib -lssl -lcrypto -ldl -licuuc -licudata -licui18n -lz -Wl,-rpath,'$$ORIGIN/../lib'
CFLAGS+= -I$(TOPDIR)include 
LDFLAGS+= -L$(TOPDIR)lib -lz -lrt -lpthread -lssl -lmecab -Wl,-rpath,'$$ORIGIN/../lib'

MKDEP = sh -ec '$(CC) -MM $(CFLAGS) $< | sed "s@\($*\)\.o[ :]*@$(OBJDIR)/\1.o $@ : @g" > $@; [ -s $@ ] || rm -f $@' 

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
	$(CXX) $< -o $@ $(CFLAGS)

$(OBJDIR)/%.d: %.cpp $(OBJDIR)
	@$(MKDEP)

$(TOPDIR)bin/%$(OBJSUF).exe: $(OBJDIR)/%.o $(LIBS)
	$(LD) $< -o $@ $(LIBS) $(LDFLAGS)

OBJS=$(addprefix $(OBJDIR)/,$(SRC:.cpp=.o))

DEPEND_FILE=$(addprefix $(OBJDIR)/, $(SRC:.cpp=.d))
TEST_FILE=$(addprefix $(TOPDIR)bin/, $(SRC:.cpp=$(OBJSUF).exe))

.PHONY: test

