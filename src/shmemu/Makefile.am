# For license: see LICENSE file at top-level

OTHER_CPPFLAGS            = -I../../include \
				-I$(srcdir)/.. \
				-I$(srcdir)/../shmemc \
				-I$(srcdir)/uthash

LIBSHMEMU_SOURCES         = shmemu-init.c \
				hostname.c fatal.c timer.c unitparse.c \
				miscops.c round.c

if ENABLE_LOGGING
LIBSHMEMU_SOURCES        += deprecate.c logger.c
endif

lib_LTLIBRARIES           = libshmemu.la
libshmemu_la_SOURCES      = $(LIBSHMEMU_SOURCES)
libshmemu_la_CFLAGS       = $(OTHER_CPPFLAGS)

lib_LIBRARIES             = libshmemu.a
libshmemu_a_SOURCES       = $(LIBSHMEMU_SOURCES)
libshmemu_a_CFLAGS        = $(OTHER_CPPFLAGS)
