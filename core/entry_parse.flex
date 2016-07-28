/*
 * Copyright 2016, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

%{
#include "common.h"

typedef struct deen_entry_yy_extra deen_entry_yy_extra;
struct deen_entry_yy_extra {
    deen_entry_sub *subs;
	uint32_t sub_count;
};


void deen_entry_yy_push_sub(deen_entry_yy_extra *extra) {
    DEEN_LOG_TRACE0("+sub");

	if(0==extra->sub_count) {
		extra->subs = (deen_entry_sub *) deen_emalloc(sizeof(deen_entry_sub));
	}
	else {
		extra->subs = (deen_entry_sub *) deen_erealloc(
			extra->subs,
			(extra->sub_count + 1) * sizeof(deen_entry_sub)
		);
	}

	extra->sub_count++;
	extra->subs[extra->sub_count-1].sub_subs = NULL;
	extra->subs[extra->sub_count-1].sub_sub_count = 0;
}


void deen_entry_yy_push_sub_sub(deen_entry_yy_extra *extra) {
	deen_entry_sub *sub;

	DEEN_LOG_TRACE0(" +sub_sub");

	sub = &(extra->subs[extra->sub_count-1]);

	if(0==sub->sub_sub_count) {
		sub->sub_subs = (deen_entry_sub_sub *) deen_emalloc(sizeof(deen_entry_sub_sub));
	}
	else {
		sub->sub_subs = (deen_entry_sub_sub *) deen_erealloc(
			sub->sub_subs,
			(sub->sub_sub_count + 1) * sizeof(deen_entry_sub_sub)
		);
	}

	sub->sub_sub_count++;
	sub->sub_subs[sub->sub_sub_count-1].atom_count = 0;
	sub->sub_subs[sub->sub_sub_count-1].atoms = NULL;
}


void deen_entry_yy_push_atom(deen_entry_yy_extra *extra, enum deen_entry_atom_type type, char *text, size_t len) {
	deen_entry_sub *sub;
	deen_entry_sub_sub *sub_sub;
	deen_entry_atom *new_atom;

	DEEN_LOG_TRACE2("  +atom; %d : >%s<", type, text);

	sub = &(extra->subs[extra->sub_count-1]);
	sub_sub = &(sub->sub_subs[sub->sub_sub_count-1]);

	if(0==sub_sub->atom_count) {
		sub_sub->atoms = (deen_entry_atom *) deen_emalloc(sizeof(deen_entry_atom));
	}
	else {
		sub_sub->atoms = (deen_entry_atom *) deen_erealloc(
			sub_sub->atoms,
			(sub_sub->atom_count + 1) * sizeof(deen_entry_atom)
		);
	}

	sub_sub->atom_count++;
	new_atom = &(sub_sub->atoms[sub_sub->atom_count-1]);

	new_atom->type = type;
	new_atom->text = NULL;

	if(NULL!=text && len > 0) {
		new_atom->text = (unsigned char *) deen_emalloc(len+1);
		memcpy(new_atom->text, text, len);
		new_atom->text[len] = 0;
	}
	else {
		new_atom->text = NULL;
	}
}

%}

textcontent         [^ \{\[\|;][^\{\[\|]+[^ \{\[\|;]
charcontent        [^ \{\[\|;]

%pointer
%option reentrant noyywrap stack
%option extra-type="deen_entry_yy_extra *"
%x GRAMMAR CONTEXT

%%

<INITIAL>[ \t\n\r]      {}
<INITIAL>[ ]*\{[ ]*     yy_push_state(GRAMMAR, yyscanner);
<INITIAL>[ ]*\[[ ]*     yy_push_state(CONTEXT, yyscanner);
<INITIAL>[ ]*\|[ ]*     { deen_entry_yy_push_sub(yyextra); deen_entry_yy_push_sub_sub(yyextra); }
<INITIAL>[ ]*;[ ]*      deen_entry_yy_push_sub_sub(yyextra);
<INITIAL>{textcontent}  deen_entry_yy_push_atom(yyextra, ATOM_TEXT, yytext, yyleng);
<INITIAL>{charcontent} deen_entry_yy_push_atom(yyextra, ATOM_TEXT, yytext, yyleng);
<GRAMMAR>[^\}]+         deen_entry_yy_push_atom(yyextra, ATOM_GRAMMAR, yytext, yyleng);
<CONTEXT>[^\]]+         deen_entry_yy_push_atom(yyextra, ATOM_CONTEXT, yytext, yyleng);
<GRAMMAR>[ ]*\}[ ]*     yy_pop_state(yyscanner);
<CONTEXT>[ ]*\][ ]*     yy_pop_state(yyscanner);

%%

void deen_entry_create_yy(
	const uint8_t *data,
	deen_entry_sub **subs,
	uint32_t *sub_count) {

	yyscan_t scanner;
	deen_entry_yy_extra extra;

	extra.subs = NULL;
	extra.sub_count = 0;

	deen_entry_yy_push_sub(&extra);
	deen_entry_yy_push_sub_sub(&extra);

	yylex_init_extra(&extra, &scanner);
	yy_scan_string((char *) data, scanner);

	yylex(scanner);

	yylex_destroy(scanner);

	// copy the data back
	*subs = extra.subs;
	*sub_count = extra.sub_count;
}
