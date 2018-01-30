SHELL := /bin/bash

#
# Build targets
#
BINARIES     := dsme dsme-server dsme-exec-helper
SUBDIRS      := util modules

VERSION := 0.60.48

#
# Install files in this directory
#
INSTALL_PERM  := 644
INSTALL_OWNER := $(shell id -u)
INSTALL_GROUP := $(shell id -g)

INSTALL_BINARIES                      := dsme dsme-server dsme-exec-helper
$(INSTALL_BINARIES)    : INSTALL_PERM := 755
$(INSTALL_BINARIES)    : INSTALL_DIR  := $(DESTDIR)/sbin

INSTALL_INCLUDES                      := include/dsme/dsme-cal.h     \
                                         include/dsme/dsmesock.h     \
                                         include/dsme/logging.h      \
                                         include/dsme/mainloop.h     \
                                         include/dsme/modulebase.h   \
                                         include/dsme/modules.h      \
                                         include/dsme/oom.h          \
                                         include/dsme/timers.h       \
                                         dsme-wdd-wd.h              

$(INSTALL_INCLUDES)    : INSTALL_DIR  := $(DESTDIR)/usr/include/dsme

#
# Compiler and tool flags
# C_OPTFLAGS are not used for debug builds (ifdef DEBUG)
# C_DBGFLAGS are not used for normal builds
#
C_GENFLAGS     := -DPRG_VERSION=$(VERSION) -pthread -g -std=c99 \
                  -Wall -Wwrite-strings -Wmissing-prototypes -Werror# -pedantic
C_OPTFLAGS     := -O2 -s
C_DBGFLAGS     := -g -DDEBUG -DDSME_LOG_ENABLE
C_DEFINES      := DSME_POSIX_TIMER DSME_WD_SYNC DSME_BMEIPC
C_INCDIRS      := $(TOPDIR)/include $(TOPDIR)/modules $(TOPDIR) 
MKDEP_INCFLAGS := $$(pkg-config --cflags-only-I glib-2.0)


LD_GENFLAGS := -pthread

# If OSSO_DEBUG is defined, compile in the logging
#ifdef OSSO_LOG
C_OPTFLAGS += -DDSME_LOG_ENABLE
#endif

ifneq (,$(findstring DSME_BMEIPC,$(C_DEFINES)))
export DSME_BMEIPC = yes
endif

ifneq (,$(findstring DSME_MEMORY_THERMAL_MGMT,$(C_DEFINES)))
export DSME_MEMORY_THERMAL_MGMT = yes
endif

#
# Target composition and overrides
#

# dsme
dsme_C_OBJS       := dsme-wdd.o dsme-wdd-wd.o oom.o
dsme: C_OPTFLAGS  := -O2 -s
dsme: C_GENFLAGS  := -DPRG_VERSION=$(VERSION) -g -std=c99 \
                     -Wall -Wwrite-strings -Wmissing-prototypes -Werror -Wno-unused-but-set-variable
dsme_LIBS         := cal
dsme: LD_GENFLAGS :=


# dsme-server
dsme-server_C_OBJS             := dsme-server.o modulebase.o timers.o \
                                  logging.o oom.o mainloop.o          \
                                  dsme-cal.o dsmesock.o
dsme-server_LIBS               := dsme dl cal
dsme-server: LD_EXTRA_GENFLAGS := -rdynamic $$(pkg-config --libs gthread-2.0)

#logging.o: C_EXTRA_DEFINES :=  USE_STDERR
dsme-server.o : C_EXTRA_GENFLAGS := $$(pkg-config --cflags glib-2.0)
mainloop.o    : C_EXTRA_GENFLAGS := $$(pkg-config --cflags glib-2.0)
modulebase.o  : C_EXTRA_GENFLAGS := $$(pkg-config --cflags glib-2.0)
timers.o      : C_EXTRA_GENFLAGS := $$(pkg-config --cflags glib-2.0)
dsmesock.o    : C_EXTRA_GENFLAGS := $$(pkg-config --cflags glib-2.0)

# TODO: move dsme-exec-helper to modules/
# dsme-exec-helper
dsme-exec-helper_C_OBJS := dsme-exec-helper.o oom.o
dsme-exec-helper.o : C_EXTRA_GENFLAGS := $$(pkg-config --cflags glib-2.0)


#
# This is the topdir for build
#
TOPDIR := $(shell /bin/pwd)

#
# Non-target files/directories to be deleted by distclean
#
DISTCLEAN_DIRS	:=	doc tags

#DISTCLN_SUBDIRS := _distclean_tests

#
# Actual rules
#
include $(TOPDIR)/Rules.make

.PHONY: tags
tags:
	find . -name '*.[hc]'  |xargs ctags

.PHONY: doc
doc:
	doxygen

local_install:
	mkdir -p $(DESTDIR)/etc/dsme
	install -m 600 -o $(INSTALL_OWNER) -g $(INSTALL_GROUP) lifeguard.uids $(DESTDIR)/etc/dsme


.PHONY: test
test: all
	make -C test depend
	make -C test
	make -C test run
