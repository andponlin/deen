# =============================================================================
#
# Copyright 2016, Andrew Lindesay. All Rights Reserved.
# Distributed under the terms of the MIT License.
#
# Authors:
#		Andrew Lindesay, apl@lindesay.co.nz
#
# =============================================================================

VERSIONMAJOR=0
VERSIONMIDDLE=1
VERSIONMINOR=3
VERSION=$(VERSIONMAJOR).$(VERSIONMIDDLE).$(VERSIONMINOR)

CC=gcc
RM=rm -f
FLEX=flex
ECHO=@echo
ZIP=zip

CFLAGSOTHER=-Wall -c -I . -DDEEN_VERSION=\"$(VERSION)\"

# Different flags are required for the compilation of the flex output file
# because some warnings can be tolerated from that.

CFLAGSFLEXOTHER=-c -I .

# -fstack-protector; checks for operations happening on the stack.  Requires
# also use of -lssp

ifdef DEBUG
	CFLAGS=-g -gdwarf-2 -ggdb -DDEBUG
	LDFLAGS=-g -gdwarf-2 -ggdb
else
	CFLAGS=-O
	LDFLAGS=-dead_strip
endif

COREOBJS=core/common.o core/entry.o core/entry_parse.o core/install.o \
	core/keyword.o core/search.o core/index.o
CLIOBJS=cli/climain.o cli/renderplain.o cli/rendercommon.o
LDFLAGSOTHER=-lsqlite3

TESTKEYWORDOBJS=core-test/keyword-test.o
TESTCOMMONOBJS=core-test/common-test.o

all: deen

deen: $(CLILIBS) $(COREOBJS) $(CLIOBJS)
	$(CC) $(CLIOBJS) $(COREOBJS) -o deen $(LDFLAGS) $(LDFLAGSOTHER)

# ----------------------------------
# TESTS

tests: deen-keyword-test deen-common-test
	./deen-keyword-test
	./deen-common-test

deen-keyword-test: $(COREOBJS) $(TESTKEYWORDOBJS)
	$(CC) $(TESTKEYWORDOBJS) $(COREOBJS) -o deen-keyword-test $(LDFLAGS) $(LDFLAGSOTHER)

deen-common-test: $(COREOBJS) $(TESTCOMMONOBJS)
	$(CC) $(TESTCOMMONOBJS) $(COREOBJS) -o deen-common-test $(LDFLAGS) $(LDFLAGSOTHER)

# ----------------------------------

core/entry_parse.c: core/entry_parse.flex
	$(FLEX) -o core/entry_parse.c core/entry_parse.flex

core/entry_parse.o: core/entry_parse.c
	$(CC) $(CFLAGS) $(CFLAGSFLEXOTHER) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(CFLAGSOTHER) -o $@ $^

clean:
	$(RM) core/*.o
	$(RM) core/entry_parse.c
	$(RM) cli/*.o
	$(RM) deen
	$(RM) deen-*-test

# ----------------------------------
