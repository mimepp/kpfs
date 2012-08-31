## Process this file with automake to produce Makefile.in
include $(abs_top_builddir)/src/rules.mk

SUBDIRS = . tests

DIST_SUBDIRS = $(SUBDIRS)

lib_LTLIBRARIES = libkpfs.la

libkpfs_la_SOURCES = \
	kpfs_api.c	\
	kpfs_util.c	\
	kpfs_conf.c	\
	kpfs_curl.c	\
	kpfs_node.c	\
	kpfs_oauth.c
	
libkpfs_la_LIBADD = $(KPFS_LIB_LDFLAGS)

AM_CPPFLAGS = -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64

bin_PROGRAMS = kpfs

kpfs_SOURCES = \
	kpfs.h		\
	kpfs.c

kpfs_LDADD = $(KPFS_BIN_LDFLAGS)

EXTRA_DIST = \
	rules.mk	\
	kpfs.h		\
	kpfs_api.h	\
	kpfs_util.h	\
	kpfs_conf.h	\
	kpfs_curl.h	\
	kpfs_node.h	\
	kpfs_oauth.h

kpfsdir=$(includedir)/kpfs
kpfs_HEADERS = \
	kpfs.h		\
	kpfs_api.h	\
	kpfs_util.h	\
	kpfs_conf.h	\
	kpfs_curl.h	\
	kpfs_node.h	\
	kpfs_oauth.h

CLEANFILES =

if WITH_GOBJECT_INTROSPECTION
kpfs-1.0.gir: $(lib_LTLIBRARIES) $(G_IR_SCANNER) Makefile.am
	$(G_IR_SCANNER) \
	--warn-error \
	-v \
	--namespace kpfs \
	--nsversion 1.0 \
	--add-include-path=$(srcdir) \
	--add-include-path=$(builddir) \
	--library=$(lib_LTLIBRARIES) \
	--output $@ \
	--verbose \
	--pkg=glib-2.0 \
	$(kpfs_HEADERS) \
	$(libkpfs_la_SOURCES)

girdir = $(datadir)/gir-1.0
gir_DATA = kpfs-1.0.gir

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = kpfs-1.0.typelib

$(typelibs_DATA): $(gir_DATA)
	g-ir-compiler \
	--includedir=$(srcdir) \
	--includedir=$(builddir) \
	-o $@ $<

CLEANFILES += kpfs-1.0.gir $(typelibs_DATA)

endif # WITH_GOBJECT_INTROSPECTION
