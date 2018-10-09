AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4

cmusvisdatadir = $(datadir)/cmusvis

bin_PROGRAMS = cmusvis
cmusvisdata_DATA = new_config
cmusvis_SOURCES = cmusvis.c
cmusvis_LDFLAGS = -L/usr/local/lib -Wl,-rpath /usr/local/lib
cmusvis_CPPFLAGS = -DPACKAGE=\"$(PACKAGE)\" -DVERSION=\"$(VERSION)\" \
           -D_POSIX_SOURCE -D _POSIX_C_SOURCE=200809L -DDATADIR='"$(cmusvisdatadir)"'
cmusvis_CFLAGS = -std=c99 -Wall -Wextra -Wno-unused-result -Wno-maybe-uninitialized 



