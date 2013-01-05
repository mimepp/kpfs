## Process this file with automake to produce Makefile.in
SUBDIRS = src #docs

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = kpfs.pc

ACLOCAL_AMFLAGS = -I m4

confdir = $(datadir)/kpfs/
conf_DATA = kpfs.conf

EXTRA_DIST = autogen.sh kpfs.pc.in gtk-doc.make

kpfsdir = $(includedir)/kpfs
kpfs_HEADERS = config.h

indent:
	indent -npro -kr -i8 -ts8 -sob -l160 -ss -ncs -cp1 -il0 src/*.c src/*.h src/tests/*.c
