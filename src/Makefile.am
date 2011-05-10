INCLUDES = -I$(VARNISHSRC)/include -I$(VARNISHSRC)

vmoddir = $(VMODDIR)
vmod_LTLIBRARIES = libvmod_header.la

libvmod_header_la_LDFLAGS = -version-info 1:0:0

libvmod_header_la_SOURCES = \
	vcc_if.c \
	vcc_if.h \
	vmod_header.c

vcc_if.c vcc_if.h: $(VARNISHSRC)/lib/libvmod_std/vmod.py $(top_srcdir)/src/vmod_header.vcc
	@PYTHON@ $(VARNISHSRC)/lib/libvmod_std/vmod.py $(top_srcdir)/src/vmod_header.vcc

EXTRA_DIST = vmod_header.vcc

CLEANFILES = $(builddir)/vcc_if.c $(builddir)/vcc_if.h
