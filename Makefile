# libudt for Linux. Builds the shared library (soname libudt.so.0) and the
# static one; `make install` honours DESTDIR, prefix and libdir.
#
# -std=gnu++11 is deliberate: the sources use dynamic exception
# specifications, which C++17 removed.

VERSION  = 4.13
SOVERSION = 0
SONAME   = libudt.so.$(SOVERSION)
LIB      = $(SONAME).$(VERSION)

CXX      ?= g++
AR       ?= ar
CXXFLAGS ?= -O2 -g
CPPFLAGS += -DLINUX
CXXFLAGS += -std=gnu++11 -fPIC -Wall -fno-strict-aliasing -fvisibility=hidden
LDLIBS   += -lpthread

prefix   ?= /usr
libdir   ?= $(prefix)/lib
includedir ?= $(prefix)/include

SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:.cpp=.o)

all: $(LIB) libudt.a

$(LIB): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared -Wl,-soname,$(SONAME) -o $@ $^ $(LDLIBS)

libudt.a: $(OBJS)
	$(AR) rcs $@ $^

src/%.o: src/%.cpp $(wildcard src/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

install: all
	install -d $(DESTDIR)$(libdir) $(DESTDIR)$(includedir)/udt
	install -m 0755 $(LIB) $(DESTDIR)$(libdir)/
	ln -sf $(LIB) $(DESTDIR)$(libdir)/$(SONAME)
	ln -sf $(SONAME) $(DESTDIR)$(libdir)/libudt.so
	install -m 0644 libudt.a $(DESTDIR)$(libdir)/
	install -m 0644 src/*.h $(DESTDIR)$(includedir)/udt/

check: $(LIB) tests/loopback.cpp
	ln -sf $(LIB) $(SONAME)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -Isrc -o tests/loopback tests/loopback.cpp ./$(SONAME) $(LDFLAGS) $(LDLIBS)
	LD_LIBRARY_PATH=. ./tests/loopback

clean:
	rm -f $(OBJS) $(LIB) $(SONAME) libudt.a tests/loopback

.PHONY: all install check clean
