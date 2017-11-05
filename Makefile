# =============================================================================
#
# Copyright 2016-2017, Andrew Lindesay. All Rights Reserved.
# Distributed under the terms of the MIT License.
#
# Authors:
#		Andrew Lindesay, apl@lindesay.co.nz
#
# =============================================================================

VERSIONMAJOR=0
VERSIONMIDDLE=1
VERSIONMINOR=4
VERSION=$(VERSIONMAJOR).$(VERSIONMIDDLE).$(VERSIONMINOR)

SQLITEVERSION=3210000
SQLITEZIPURL=http://sqlite.org/2017/sqlite-amalgamation-$(SQLITEVERSION).zip
SQLITETMP=sqlite-amalgamation-$(SQLITEVERSION).zip
SQLITEDIR=sqlite-amalgamation-$(SQLITEVERSION)
SQLITEHEADER=$(SQLITEDIR)/sqlite3.h

CC=gcc
RM=rm -f
FLEX=flex
ECHO=@echo
ZIP=zip
WGET=wget

CFLAGSOTHER=-Wall -c -I . -I $(SQLITEDIR) -DDEEN_VERSION=\"$(VERSION)\"

# Different flags are required for the compilation of the flex output file
# because some warnings can be tolerated from that.

CFLAGSFLEXOTHER=-c -I . -I $(SQLITEDIR)

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
	core/keyword.o core/search.o core/index.o \
	$(SQLITEDIR)/sqlite3.o
CLIOBJS=cli/climain.o cli/renderplain.o cli/rendercommon.o
LDFLAGSOTHER=

TESTKEYWORDOBJS=core-test/keyword-test.o
TESTCOMMONOBJS=core-test/common-test.o
TESTINDEXOBJS=core-test/index-test.o
TESTENTRYOBJS=core-test/entry-test.o

all: deen

deen: $(SQLITEHEADER) $(CLILIBS) $(COREOBJS) $(CLIOBJS)
	$(CC) $(CLIOBJS) $(COREOBJS) -o deen $(LDFLAGS) $(LDFLAGSOTHER)

# ----------------------------------
# TESTS

tests: deen-keyword-test deen-common-test deen-index-test deen-entry-test
	./deen-keyword-test
	./deen-common-test
	./deen-index-test
	./deen-entry-test

deen-keyword-test: $(SQLITEHEADER) $(COREOBJS) $(TESTKEYWORDOBJS)
	$(CC) $(TESTKEYWORDOBJS) $(COREOBJS) -o deen-keyword-test $(LDFLAGS) $(LDFLAGSOTHER)

deen-common-test: $(SQLITEHEADER) $(COREOBJS) $(TESTCOMMONOBJS)
	$(CC) $(TESTCOMMONOBJS) $(COREOBJS) -o deen-common-test $(LDFLAGS) $(LDFLAGSOTHER)

deen-index-test: $(SQLITEHEADER) $(COREOBJS) $(TESTINDEXOBJS)
	$(CC) $(TESTINDEXOBJS) $(COREOBJS) -o deen-index-test $(LDFLAGS) $(LDFLAGSOTHER)

deen-entry-test: $(SQLITEHEADER) $(COREOBJS) $(TESTENTRYOBJS)
	$(CC) $(TESTENTRYOBJS) $(COREOBJS) -o deen-entry-test $(LDFLAGS) $(LDFLAGSOTHER)

# ----------------------------------

$(SQLITETMP):
	$(WGET) $(SQLITEZIPURL)

$(SQLITEDIR)/sqlite3.c $(SQLITEDIR)/sqlite3.h: $(SQLITETMP)
	unzip $(SQLITETMP)
	touch $(SQLITEDIR)/sqlite3.c
	touch $(SQLITEDIR)/sqlite3.h

core/entry_parse.c: core/entry_parse.flex
	$(FLEX) -o core/entry_parse.c core/entry_parse.flex

core/entry_parse.o: core/entry_parse.c
	$(CC) $(CFLAGS) $(CFLAGSFLEXOTHER) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(CFLAGSOTHER) -o $@ $^

clean:
	$(RM) $(SQLITETMP)
	$(RM) $(SQLITEDIR)/*.o
	$(RM) $(SQLITEDIR)/*.c
	$(RM) $(SQLITEDIR)/*.h
	$(RM) core/*.o
	$(RM) core/entry_parse.c
	$(RM) cli/*.o
	$(RM) deen
	$(RM) deen-*-test
	$(RM) deen.exe
	$(RM) deen-*-test.exe
	$(RM) tmp_index_e2e.sqlite

# ----------------------------------
