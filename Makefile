include common.mk

SCFENCE_DIR := scfence

OBJECTS := libthreads.o schedule.o model.o threads.o librace.o action.o \
	   nodestack.o clockvector.o main.o snapshot-interface.o cyclegraph.o \
	   datarace.o impatomic.o cmodelint.o \
	   snapshot.o malloc.o mymemory.o common.o mutex.o promise.o conditionvariable.o \
	   context.o scanalysis.o execution.o plugins.o libannotate.o \
	   mockfs.o request.o

CPPFLAGS += -Iinclude -I. -I$(SCFENCE_DIR)
CPPFLAGS += -I/usr/local/src/apache/httpd-2.4.58 -I/usr/local/src/apache/httpd-2.4.58/os/unix -I/usr/local/src/apache/httpd-2.4.58/include -I/usr/local/apache/apr/include/apr-1 -I/usr/local/apache/apr-util/include/apr-1 -I/usr/local/src/apache/httpd-2.4.58/modules/aaa -I/usr/local/src/apache/httpd-2.4.58/modules/cache -I/usr/local/src/apache/httpd-2.4.58/modules/core -I/usr/local/src/apache/httpd-2.4.58/modules/database -I/usr/local/src/apache/httpd-2.4.58/modules/filters -I/usr/local/src/apache/httpd-2.4.58/modules/ldap -I/usr/local/src/apache/httpd-2.4.58/server -I/usr/local/src/apache/httpd-2.4.58/modules/loggers -I/usr/local/src/apache/httpd-2.4.58/modules/lua -I/usr/local/src/apache/httpd-2.4.58/modules/proxy -I/usr/local/src/apache/httpd-2.4.58/modules/http2 -I/usr/local/src/apache/httpd-2.4.58/modules/session -I/usr/local/src/apache/httpd-2.4.58/modules/ssl -I/usr/local/src/apache/httpd-2.4.58/modules/test -I/usr/local/src/apache/httpd-2.4.58/server -I/usr/local/src/apache/httpd-2.4.58/modules/md -I/usr/local/src/apache/httpd-2.4.58/modules/arch/unix -I/usr/local/src/apache/httpd-2.4.58/modules/dav/main -I/usr/local/src/apache/httpd-2.4.58/modules/generators -I/usr/local/src/apache/httpd-2.4.58/modules/mappers -I/usr/local/src/apache/httpd-2.4.58/server/mpm/worker -I/usr/local/src/apache/apr-1.7.4/include -I/usr/local/src/apache/apr-1.7.4/include/arch/unix
LDFLAGS := -ldl -lrt -rdynamic
SHARED := -shared

# Mac OSX options
ifeq ($(UNAME), Darwin)
LDFLAGS := -ldl
SHARED := -Wl,-undefined,dynamic_lookup -dynamiclib
endif

TESTS_DIR := test

MARKDOWN := doc/Markdown/Markdown.pl

all: $(LIB_SO) README.html

debug: CPPFLAGS += -DCONFIG_DEBUG
debug: all

PHONY += docs
docs: *.c *.cc *.h README.html
	doxygen

README.html: README.md
	$(MARKDOWN) $< > $@


malloc.o: malloc.c
	$(CC) -fPIC -c malloc.c -DMSPACES -DONLY_MSPACES -DHAVE_MMAP=0 $(CPPFLAGS) -Wno-unused-variable

%.o : %.cc
	$(CXX) -MMD -MF .$@.d -fPIC -c $< $(CPPFLAGS)

include $(SCFENCE_DIR)/Makefile

-include $(wildcard $(SCFENCE_DIR)/.*.d)

$(LIB_SO): $(OBJECTS)
	$(CXX) $(SHARED) -o $(LIB_SO) $+ $(LDFLAGS) \
	-Wl,--whole-archive /usr/local/src/apache/httpd-2.4.58/.libs/libhttpd.a -Wl,--no-whole-archive \
	-Wl,--whole-archive /usr/local/src/apache/apr-1.7.4/.libs/libapr-1.a -Wl,--no-whole-archive \
	-Wl,--whole-archive /usr/local/src/apache/apr-util-1.6.3/.libs/libaprutil-1.a -Wl,--no-whole-archive

%.pdf: %.dot
	dot -Tpdf $< -o $@

-include $(OBJECTS:%=.%.d)

PHONY += clean
clean:
	rm -f *.o *.so .*.d *.pdf *.dot $(SCFENCE_DIR)/.*.d $(SCFENCE_DIR)/*.o
	$(MAKE) -C $(TESTS_DIR) clean

PHONY += mrclean
mrclean: clean
	rm -rf docs

PHONY += tags
tags:
	ctags -R

PHONY += tests
tests: $(LIB_SO)
	$(MAKE) -C $(TESTS_DIR)

BENCH_DIR := benchmarks

PHONY += benchmarks
benchmarks: $(LIB_SO)
	@if ! test -d $(BENCH_DIR); then \
		echo "Directory $(BENCH_DIR) does not exist" && \
		echo "Please clone the benchmarks repository" && \
		echo && \
		exit 1; \
	fi
	$(MAKE) -C $(BENCH_DIR)

PHONY += pdfs
pdfs: $(patsubst %.dot,%.pdf,$(wildcard *.dot))

.PHONY: $(PHONY)

# A 1-inch margin PDF generated by 'pandoc'
%.pdf: %.md
	pandoc -o $@ $< -V header-includes='\usepackage[margin=1in]{geometry}'
