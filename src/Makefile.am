INCLUDES = -I$(VARNISHSRC)/include -I$(VARNISHSRC)

vmoddir = $(VMODDIR)
vmod_LTLIBRARIES = libvmod_header.la

libvmod_header_la_LDFLAGS = -module -export-dynamic -avoid-version

libvmod_header_la_SOURCES = \
	vcc_if.c \
	vcc_if.h \
	vmod_header.c

vcc_if.c vcc_if.h: $(VARNISHSRC)/lib/libvcl/vmodtool.py $(top_srcdir)/src/vmod_header.vcc
	@PYTHON@ $(VARNISHSRC)/lib/libvcl/vmodtool.py $(top_srcdir)/src/vmod_header.vcc

VMOD_TESTS = tests/*.vtc
.PHONY: $(VMOD_TESTS)

tests/*.vtc:
	$(VARNISHSRC)/bin/varnishtest/varnishtest -Dvarnishd=$(VARNISHSRC)/bin/varnishd/varnishd -Dvmod_topbuild=$(abs_top_builddir) $@

check: $(VMOD_TESTS)

EXTRA_DIST = \
	vmod_header.vcc \
	$(VMOD_TESTS)

CLEANFILES = $(builddir)/vcc_if.c $(builddir)/vcc_if.h
