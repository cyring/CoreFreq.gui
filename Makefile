# CoreFreq/GUI
# Copyright (C) 2015-2020 CYRIL INGENIERIE
# Licenses: GPL2

CC ?= cc
CFLAGS = -Wall
FREETYPE_INC ?= $(shell pkg-config --cflags freetype2 2>/dev/null)
COREFREQ_INC ?= -I ../CoreFreq
PREFIX ?= /usr

ifneq ($(FREETYPE_INC),)
	CONFIG_XFT = -D HAVE_XFT=1
	LIBRARY_XFT = -lXft
endif

.PHONY: all
all: corefreq-gui

.PHONY: install
install: module-install
	install -Dm 0755 corefreq-gui -t $(PREFIX)/bin

.PHONY: clean
clean:
	rm -f corefreq-gui corefreq-gui.o corefreq-gui-lib.o

corefreq-gui-lib.o: corefreq-gui-lib.c
	$(CC)	$(CFLAGS) -c corefreq-gui-lib.c \
		$(CONFIG_XFT) $(FREETYPE_INC) $(COREFREQ_INC) \
		-o corefreq-gui-lib.o

corefreq-gui.o: corefreq-gui.c
	$(CC)	$(CFLAGS) -c corefreq-gui.c \
		$(CONFIG_XFT) $(FREETYPE_INC) $(COREFREQ_INC) \
		-o corefreq-gui.o

corefreq-gui: corefreq-gui.o corefreq-gui-lib.o
	$(CC)	$(CFLAGS) corefreq-gui.c corefreq-gui-lib.c \
		$(CONFIG_XFT) $(FREETYPE_INC) $(COREFREQ_INC) \
		-o corefreq-gui -lX11 $(LIBRARY_XFT) -lpthread -lrt

.PHONY: info
info:
	$(info CC [$(shell whereis -b $(CC))])
	$(info CFLAGS [$(CFLAGS)])
	$(info PWD [$(PWD)])
	$(info FREETYPE_INC [$(FREETYPE_INC)])
	$(info PREFIX [$(PREFIX)])

.PHONY: help
help:
	@echo -e \
	"o---------------------------------------------------------------o\n"\
	"|  make [all] [clean] [info] [help] [install]                   |\n"\
	"|                                                               |\n"\
	"|  CC=<COMPILER>                                                |\n"\
	"|    where <COMPILER> is cc, gcc, clang                         |\n"\
	"|                                                               |\n"\
	"|  CFLAGS=<ARG>                                                 |\n"\
	"|    where default argument is -Wall                            |\n"\
	"|                                                               |\n"\
	"|  FREETYPE_INC=-I <PATH>                                       |\n"\
	"|    where <PATH> is the FreeType headers directory             |\n"\
	"|                                                               |\n"\
	"|  COREFREQ_INC=-I <PATH>                                       |\n"\
	"|    where <PATH> is the CoreFreq source directory              |\n"\
	"o---------------------------------------------------------------o"

