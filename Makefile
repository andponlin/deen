# =============================================================================
#
# Copyright 2016-2019, Andrew Lindesay. All Rights Reserved.
# Distributed under the terms of the MIT License.
#
# Authors:
#		Andrew Lindesay, apl@lindesay.co.nz
#
# =============================================================================

VERSIONMAJOR=0
VERSIONMIDDLE=2
VERSIONMINOR=1
VERSION=$(VERSIONMAJOR).$(VERSIONMIDDLE).$(VERSIONMINOR)

SQLITEVERSION=3490100
SQLITEZIPURL=http://sqlite.org/2025/sqlite-amalgamation-$(SQLITEVERSION).zip
SQLITETMP=sqlite-amalgamation-$(SQLITEVERSION).zip
SQLITEDIR=sqlite-amalgamation-$(SQLITEVERSION)
SQLITEHEADER=$(SQLITEDIR)/sqlite3.h
SQLITECOMPILEOPTS=-DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION

GLIBCOMPILERESOURCES=glib-compile-resources
CC=gcc
RM=rm -f
FLEX=flex
ECHO=@echo
ZIP=zip
WGET=wget

CFLAGSOTHER=-Wall -c -I . -I $(SQLITEDIR) -DDEEN_VERSION=\"$(VERSION)\" $(SQLITECOMPILEOPTS)

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
GTKOBJS=gui-gtk/ggtkmain.o gui-gtk/ggtkinstall.o gui-gtk/ggtkgeneral.o \
	gui-gtk/ggtkresources.o gui-gtk/ggtksearch.o gui-gtk/ggtkrendertextbuffer.o
GTKRSRCS=gui-gtk/ggtkresources.xml gui-gtk/ggtkmain.glade
LDFLAGSOTHER=
GTKLDFLAGS=-lpthread

TESTKEYWORDOBJS=core-test/keyword-test.o
TESTCOMMONOBJS=core-test/common-test.o
TESTINDEXOBJS=core-test/index-test.o
TESTENTRYOBJS=core-test/entry-test.o

all: deen

deen: $(SQLITEHEADER) $(CLILIBS) $(COREOBJS) $(CLIOBJS)
	$(CC) $(CLIOBJS) $(COREOBJS) -o deen $(LDFLAGS) $(LDFLAGSOTHER)

deen-ggtk: $(SQLITEHEADER) $(GTKOBJS) $(COREOBJS)
	$(CC) -rdynamic $(GTKOBJS) $(COREOBJS) -o deen-ggtk $(LDFLAGS) $(LDFLAGSOTHER) $(GTKLDFLAGS) $(shell pkg-config gtk+-3.0 --libs)

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

gui-gtk/ggtkresources.h: $(GTKRSRCS)
	$(GLIBCOMPILERESOURCES) gui-gtk/ggtkresources.xml --sourcedir=gui-gtk \
	--c-name deen_ggtk --manual-register --generate-header --target=gui-gtk/ggtkresources.h

gui-gtk/ggtkresources.c: $(GTKRSRCS)
	$(GLIBCOMPILERESOURCES) gui-gtk/ggtkresources.xml --sourcedir=gui-gtk \
	--c-name deen_ggtk --manual-register --generate-source --target=gui-gtk/ggtkresources.c

gui-gtk/ggtkmain.o: gui-gtk/ggtkmain.c gui-gtk/ggtkresources.h
	$(CC) $(CFLAGS) $(CFLAGSOTHER) $(shell pkg-config gtk+-3.0 --cflags) \
	-o gui-gtk/ggtkmain.o gui-gtk/ggtkmain.c

gui-gtk/%.o: gui-gtk/%.c
	$(CC) $(CFLAGS) $(CFLAGSOTHER) $(shell pkg-config gtk+-3.0 --cflags) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(CFLAGSOTHER) -o $@ $^

clean-sqlite:
	$(RM) $(SQLITETMP)
	$(RM) $(SQLITEDIR)/*.o
	$(RM) $(SQLITEDIR)/*.c
	$(RM) $(SQLITEDIR)/*.h

clean-own:
	$(RM) core/*.o
	$(RM) core/entry_parse.c
	$(RM) cli/*.o
	$(RM) deen
	$(RM) deen-*-test
	$(RM) deen.exe
	$(RM) deen-*-test.exe
	$(RM) tmp_index_e2e.sqlite

clean-gui:
	$(RM) deen-gui
	$(RM) gui-gtk/ggtkresources.c
	$(RM) gui-gtk/ggtkresources.h
	$(RM) gui-gtk/*.o
	$(RM) gui-gtk/*.o

clean: clean-sqlite clean-own clean-gui

# ----------------------------------
