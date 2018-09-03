AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -DLIBDIR="\"$(libdir)\""

bin_PROGRAMS = withhost
withhost_SOURCES = withhost.c

lib_LTLIBRARIES = libwithhost.la
libwithhost_la_SOURCES = withhost_ld.c
libwithhost_la_LDFLAGS = -shared
