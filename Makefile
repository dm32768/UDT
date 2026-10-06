# libudt for Linux. Builds the shared library (soname libudt.so.0) and the
# static one; `make install` honours DESTDIR, prefix and libdir.
#
# -std=gnu++11 is deliberate: the sources use dynamic exception
# specifications, which C++17 removed.

VERSION  = 4.13
SOVERSION = 0
SONAME   = libudt.so.$(SOVERSION)

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

all: libudt.so.$(VERSION) libudt.a

libudt.so.$(VERSION): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared -Wl,-soname,$(SONAME) -o $@ $^ $(LDLIBS)

libudt.a: $(OBJS)
	$(AR) rcs $@ $^

src/%.o: src/%.cpp $(wildcard src/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

install: all
	install -d $(DESTDIR)$(libdir) $(DESTDIR)$(includedir)/udt
	install -m 0755 libudt.so.$(VERSION) $(DESTDIR)$(libdir)/
	ln -sf libudt.so.$(VERSION) $(DESTDIR)$(libdir)/$(SONAME)
	ln -sf libudt.so.$(VERSION) $(DESTDIR)$(libdir)/libudt.so
	install -m 0644 libudt.a $(DESTDIR)$(libdir)/
	install -m 0644 src/*.h $(DESTDIR)$(includedir)/udt/

check: libudt.so.$(VERSION) tests/loopback.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -Isrc -o tests/loopback tests/loopback.cpp ./libudt.so.$(VERSION) $(LDFLAGS) $(LDLIBS)
	LD_LIBRARY_PATH=. ./tests/loopback

clean:
	rm -f $(OBJS) libudt.so.$(VERSION) libudt.a tests/loopback

.PHONY: all install check clean
