# A few common Makefile items

CC = gcc
CXX = g++

UNAME = $(shell uname)

LIB_NAME = model
LIB_SO = lib$(LIB_NAME).so

BASE = ../..
INCLUDE = -I$(BASE)/include -I../include
INCLUDE += -I/usr/local/src/apache/httpd-2.4.58 -I/usr/local/src/apache/httpd-2.4.58/os/unix -I/usr/local/src/apache/httpd-2.4.58/include -I/usr/local/apache/apr/include/apr-1 -I/usr/local/apache/apr-util/include/apr-1 -I/usr/local/src/apache/httpd-2.4.58/modules/aaa -I/usr/local/src/apache/httpd-2.4.58/modules/cache -I/usr/local/src/apache/httpd-2.4.58/modules/core -I/usr/local/src/apache/httpd-2.4.58/modules/database -I/usr/local/src/apache/httpd-2.4.58/modules/filters -I/usr/local/src/apache/httpd-2.4.58/modules/ldap -I/usr/local/src/apache/httpd-2.4.58/server -I/usr/local/src/apache/httpd-2.4.58/modules/loggers -I/usr/local/src/apache/httpd-2.4.58/modules/lua -I/usr/local/src/apache/httpd-2.4.58/modules/proxy -I/usr/local/src/apache/httpd-2.4.58/modules/http2 -I/usr/local/src/apache/httpd-2.4.58/modules/session -I/usr/local/src/apache/httpd-2.4.58/modules/ssl -I/usr/local/src/apache/httpd-2.4.58/modules/test -I/usr/local/src/apache/httpd-2.4.58/server -I/usr/local/src/apache/httpd-2.4.58/modules/md -I/usr/local/src/apache/httpd-2.4.58/modules/arch/unix -I/usr/local/src/apache/httpd-2.4.58/modules/dav/main -I/usr/local/src/apache/httpd-2.4.58/modules/generators -I/usr/local/src/apache/httpd-2.4.58/modules/mappers -I/usr/local/src/apache/httpd-2.4.58/server/mpm/worker -I/usr/local/src/apache/apr-1.7.4/include -I/usr/local/src/apache/apr-1.7.4/include/arch/unix

# C preprocessor flags
CPPFLAGS += $(INCLUDE) -g

# C++ compiler flags
CXXFLAGS += $(CPPFLAGS)

# C compiler flags
CFLAGS += $(CPPFLAGS)

# Linker flags
# BINDIR="${0%/*}"
# export LD_LIBRARY_PATH=${BINDIR}/..
# LDFLAGS += -L/usr/local/src/apache/httpd-2.4.58/.libs -lhttpd -Wl,-rpath /usr/local/src/apache/httpd-2.4.58/.libs
LDFLAGS += -L$(BASE) -l$(LIB_NAME) -rdynamic -Wl,-rpath $(BASE)
# LDFLAGS += -L/usr/local/apache/apr-util/lib -laprutil-1 -L/usr/local/apache/apr/lib -lapr-1 -lpcre
LDFLAGS += -lpcre -lexpat -lcrypt

# Mac OSX options
ifeq ($(UNAME), Darwin)
MACFLAGS = -D_XOPEN_SOURCE -DMAC
CPPFLAGS += $(MACFLAGS)
CXXFLAGS += $(MACFLAGS)
CFLAGS += $(MACFLAGS)
LDFLAGS += $(MACFLAGS)
endif
