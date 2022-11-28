SHELL := /bin/bash
#
# Build targets
#
BINARIES	:=	batttest dsmetool dsmetest bootstate waitfordsme dsmereboot

#
# Target composition and overrides
#
batttest_C_OBJS		:=	batttest.o
batttest: C_EXTRA_GENFLAGS := $$(pkg-config --cflags glib-2.0)
batttest: LD_EXTRA_GENFLAGS := -ldsme $$(pkg-config --libs glib-2.0)
dsmetool_C_OBJS 	:=	dsmetool.o
dsmetool: C_EXTRA_GENFLAGS := $$(pkg-config --cflags glib-2.0)
dsmetool: LD_EXTRA_GENFLAGS := -ldsme $$(pkg-config --libs glib-2.0)
dsmetest_C_OBJS 	:=	dsmetest.o
dsmetest: C_EXTRA_GENFLAGS := $$(pkg-config --cflags glib-2.0)
dsmetest: LD_EXTRA_GENFLAGS := -ldsme $$(pkg-config --libs glib-2.0)
bootstate_C_OBJS 	:=	bootstate.o
bootstate: C_EXTRA_GENFLAGS := $$(pkg-config --cflags glib-2.0)
bootstate: LD_EXTRA_GENFLAGS := -ldsme $$(pkg-config --libs glib-2.0)
waitfordsme_C_OBJS 	:=	waitfordsme.o
waitfordsme: C_EXTRA_GENFLAGS := $$(pkg-config --cflags glib-2.0)
waitfordsme: LD_EXTRA_GENFLAGS := -ldsme $$(pkg-config --libs glib-2.0)
kicker_C_OBJS 	:=	kicker.o
kicker: C_EXTRA_GENFLAGS := $$(pkg-config --cflags glib-2.0)
kicker: LD_EXTRA_GENFLAGS := -ldsme $$(pkg-config --libs glib-2.0)
dsmereboot_C_OBJS := dsmereboot.o
dsmereboot: C_EXTRA_GENFLAGS := $$(pkg-config --cflags glib-2.0)
dsmereboot: LD_EXTRA_GENFLAGS := -ldsme $$(pkg-config --libs glib-2.0)


#
# Install files in this directory
#
INSTALL_PERM    :=      644
INSTALL_OWNER   :=      $(shell id -u)
INSTALL_GROUP   :=      $(shell id -g)


INSTALL_BINARIES                        :=      batttest dsmetool bootstate waitfordsme dsmetest
$(INSTALL_BINARIES): INSTALL_PERM       :=      755
$(INSTALL_BINARIES): INSTALL_DIR        :=      $(DESTDIR)/usr/sbin

include $(TOPDIR)/Rules.make
