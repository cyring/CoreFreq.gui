# CoreFreq/GUI
# Copyright (C) 2015-2025 CYRIL COURTIAT
# Licenses: GPL2

COREFREQ_MAJOR = 2
COREFREQ_MINOR = 0
COREFREQ_REV = 4
CC ?= cc
CFLAGS = -Wall
FREETYPE_INC ?= $(shell pkg-config --cflags freetype2 2>/dev/null)
COREFREQ_INC ?= -I ../CoreFreq/x86_64
PREFIX ?= /usr
UBENCH = 0
CORE_COUNT ?= 256
TASK_ORDER = 5
MAX_FREQ_HZ ?= 7125000000
ARCH_PMC ?=

DEFINITIONS =	-D COREFREQ_MAJOR=$(COREFREQ_MAJOR) \
		-D COREFREQ_MINOR=$(COREFREQ_MINOR) \
		-D COREFREQ_REV=$(COREFREQ_REV) \
		-D CORE_COUNT=$(CORE_COUNT) -D TASK_ORDER=$(TASK_ORDER) \
		-D MAX_FREQ_HZ=$(MAX_FREQ_HZ) -D UBENCH=$(UBENCH)

ifneq ($(FEAT_DBG),)
DEFINITIONS += -D FEAT_DBG=$(FEAT_DBG)
endif

ifneq ($(LEGACY),)
DEFINITIONS += -D LEGACY=$(LEGACY)
endif

ifneq ($(ARCH_PMC),)
DEFINITIONS += -D ARCH_PMC=$(ARCH_PMC)
endif

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
		$(DEFINITIONS) \
		-o corefreq-gui-lib.o

corefreq-gui.o: corefreq-gui.c
	$(CC)	$(CFLAGS) -c corefreq-gui.c \
		$(CONFIG_XFT) $(FREETYPE_INC) $(COREFREQ_INC) \
		$(DEFINITIONS) \
		-o corefreq-gui.o

corefreq-gui: corefreq-gui.o corefreq-gui-lib.o
	$(CC)	$(CFLAGS) corefreq-gui.c corefreq-gui-lib.c \
		$(CONFIG_XFT) $(FREETYPE_INC) $(COREFREQ_INC) \
		$(DEFINITIONS) \
		-o corefreq-gui -lX11 $(LIBRARY_XFT) -lpthread -lrt

.PHONY: info
info:
	$(info CC [$(shell whereis -b $(CC))])
	$(info CFLAGS [$(CFLAGS)])
	$(info LEGACY [$(LEGACY)])
	$(info UBENCH [$(UBENCH)])
	$(info FEAT_DBG [$(FEAT_DBG)])
	$(info CORE_COUNT [$(CORE_COUNT)])
	$(info TASK_ORDER [$(TASK_ORDER)])
	$(info MAX_FREQ_HZ [$(MAX_FREQ_HZ)])
	$(info PWD [$(PWD)])
	$(info ARCH_PMC [$(ARCH_PMC)])
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
	"|  CORE_COUNT=<N>                                               |\n"\
	"|    where <N> is 64, 128, 256, 512 or 1024 builtin CPU         |\n"\
	"|                                                               |\n"\
	"|  LEGACY=<L>                                                   |\n"\
	"|    where level <L>                                            |\n"\
	"|    1: assembly level restriction such as CMPXCHG16            |\n"\
	"|                                                               |\n"\
	"|  UBENCH=<N>                                                   |\n"\
	"|    where <N> is 0 to disable or 1 to enable micro-benchmark   |\n"\
	"|                                                               |\n"\
	"|  TASK_ORDER=<N>                                               |\n"\
	"|    where <N> is the memory page unit of kernel allocation     |\n"\
	"|                                                               |\n"\
	"|  FEAT_DBG=<N>                                                 |\n"\
	"|    where <N> is 0 or N for FEATURE DEBUG level                |\n"\
	"|    3: XMM assembly in RING operations                         |\n"\
	"|                                                               |\n"\
	"|  MAX_FREQ_HZ=<freq>                                           |\n"\
	"|    where <freq> is at least 4850000000 Hz                     |\n"\
	"|                                                               |\n"\
	"|  Architectural Counters:                                      |\n"\
	"|    -------------------------------------------------------    |\n"\
	"|   |           Intel           |            AMD            |   |\n"\
	"|   |----------- REG -----------|----------- REG -----------|   |\n"\
	"|   |       ARCH_PMC=PCU        |      ARCH_PMC=L3          |   |\n"\
	"|   |                           |      ARCH_PMC=PERF        |   |\n"\
	"|   |                           |      ARCH_PMC=UMC         |   |\n"\
	"|    -------------------------------------------------------    |\n"\
	"|                                                               |\n"\
	"|  FREETYPE_INC=-I <PATH>                                       |\n"\
	"|    where <PATH> is the FreeType headers directory             |\n"\
	"|                                                               |\n"\
	"|  COREFREQ_INC=-I <PATH>                                       |\n"\
	"|    where <PATH> is the CoreFreq source directory              |\n"\
	"o---------------------------------------------------------------o"
