/*
 * Copyright 2016, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#include "core/constants.h"
#include "core/keyword.h"

#include "renderplain.h"
#include "rendercommon.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/*
These character sequences code for various colors in the ANSI terminal.
*/

#define TTYMAGENTA "\x1b[35m"
#define TTYORANGE "\x1b[36m"
#define TTYBLUE "\x1b[33m"
#define TTYRED "\x1b[31m"
#define TTYFADED "\x1b[2m"
#define TTYSEQRESET "\x1b[0m"

/*
This is a UTF-8 sequence that represents the unicode character that is a
horizontal line suitable for use in constructing a horizontal rule.
*/

static uint8_t UTF8_RULE[4] = { 0xe2, 0x94, 0x80, 0x00 };


static void deen_render_tty_or_nontty(
	deen_bool is_tty,
	const char *tty_version,
	const char *non_tty_version) {

	if (DEEN_TRUE == is_tty) {
		if (NULL != tty_version) {
			fputs(tty_version, stdout);
		}
	}
	else {
		if (NULL != non_tty_version) {
			fputs(non_tty_version, stdout);
		}
	}
}



/*
This will go through the text and look for keywords.  It will highlight any
keywords as bold and print the rest.
*/

static void deen_render_plain_text_highlights(
	uint8_t *text,
	deen_keywords *keywords,
	deen_bool tty) {

	if (deen_term_is_utf8() && tty) {

		size_t len = strlen((char *) text);
		size_t upto = 0;

		while (upto < len) {

			deen_first_keyword first_keyword = deen_ifind_first_keyword(
				text, keywords, upto, len);

			if (DEEN_NOT_FOUND == first_keyword.offset) {
				deen_term_print_str(&text[upto]);
				upto = len;
			}
			else {
				deen_bool is_valid_keyword_found;
				size_t keyword_len = strlen((char *) first_keyword.keyword);
				deen_term_print_str_range(text, upto, first_keyword.offset);

				// need to make sure that highlighting is only happening at
				// the prefix of a word and is not finding text 'randomly'
				// within the line.

				is_valid_keyword_found =
					0 == first_keyword.offset ||
					isspace(text[first_keyword.offset-1]) ||
					ispunct(text[first_keyword.offset-1]);

				if(is_valid_keyword_found) {
					fputs(TTYRED, stdout);
				}

				deen_term_print_str_range(
					text,
					first_keyword.offset,
					first_keyword.offset + keyword_len);

				if(is_valid_keyword_found) {
					fputs(TTYSEQRESET, stdout);
				}

				upto = first_keyword.offset + keyword_len;
			}
		}
	}
	else {
		deen_term_print_str(text);
	}
}


static void deen_render_plain_entry_atom(deen_entry_atom *atom, deen_keywords *keywords, deen_bool tty) {
		switch (atom->type) {
			case ATOM_TEXT:
				deen_render_plain_text_highlights(atom->text, keywords, tty);
				break;

			case ATOM_CONTEXT:
				deen_render_tty_or_nontty(tty, TTYBLUE, "[");
				deen_term_print_str(atom->text);
				deen_render_tty_or_nontty(tty, TTYSEQRESET, "]");
				break;

			case ATOM_GRAMMAR:
				deen_render_tty_or_nontty(tty, TTYORANGE, "{");
				deen_term_print_str(atom->text);
				deen_render_tty_or_nontty(tty, TTYSEQRESET, "}");
				break;

			default:
				deen_log_error_and_exit("unknown atom type");
				break;
		}
}


void deen_render_plain_entry_sub_sub(
	deen_entry_sub_sub *sub_sub,
	deen_keywords *keywords,
	deen_bool tty) {

	if (NULL!=sub_sub) {
		uint32_t i;

		for (i=0;i<sub_sub->atom_count;i++) {
			if (0!=i) {
				fputs(" ", stdout);
			}

			deen_render_plain_entry_atom(&(sub_sub->atoms[i]), keywords, tty);
		}
	}
}


void deen_render_plain_entry_sub(
	deen_entry_sub *sub,
	deen_keywords *keywords,
	deen_bool tty) {

    if (NULL!=sub) {
	uint32_t i;

	for (i=0;i<sub->sub_sub_count;i++) {
		if (0!=i) {
			fputs("; ", stdout);
		}

		deen_render_plain_entry_sub_sub(&(sub->sub_subs[i]), keywords, tty);
	}
    }
}

void deen_render_plain_entry(
	deen_entry *entry,
	deen_keywords *keywords,
	deen_bool tty) {

	uint32_t i;
	uint32_t max_count = entry->german_sub_count;

	if (entry->english_sub_count > max_count) {
		max_count = entry->english_sub_count;
	}

	for (i=0;i<max_count;i++) {
		if (1==max_count) {
			fputs("    ", stdout);
		}
		else {
			deen_render_tty_or_nontty(tty, TTYFADED, NULL);
			printf("%2d) ", i+1);
			deen_render_tty_or_nontty(tty, TTYSEQRESET, NULL);
		}

		if (i < entry->german_sub_count) {
			deen_render_plain_entry_sub(&(entry->german_subs[i]), keywords, tty);
		}
		else {
			fputs("???", stdout);
		}

		if (deen_term_is_utf8() && tty) {
			fputs(TTYMAGENTA, stdout);
			fputs(" ", stdout);
			fputs((char *) UTF8_RULE, stdout);
			fputs((char *) UTF8_RULE, stdout);
			fputs(" ", stdout);
			fputs(TTYSEQRESET,stdout);
		}
		else {
			fputs(" :: ", stdout);
		}

		if (i < entry->english_sub_count) {
			deen_render_plain_entry_sub(&(entry->english_subs[i]), keywords, tty);
		}
		else {
			fputs("???", stdout);
		}

		fputs("\n", stdout);
	}
}


void deen_render_rule(deen_bool tty) {
	if (deen_term_is_utf8() && tty) {

		int i;
		deen_render_tty_or_nontty(tty, TTYFADED, NULL);

		for (i=0;i<32;i++) {
			fputs((char *) UTF8_RULE, stdout);
		}

		deen_render_tty_or_nontty(tty, TTYSEQRESET, NULL);
	}
	else {
		fputs("- - - - - - - - - - - - - -", stdout);
	}

	fputs("\n", stdout);
}


void deen_render_plain(deen_search_result *result, deen_keywords *keywords) {
    if (NULL!=result) {
		if (0 == result->entry_count) {
			puts("not found\n");
		}
		else {
			uint32_t i;
			deen_bool tty = isatty(fileno(stdout));

			deen_render_tty_or_nontty(tty, TTYFADED, NULL);
			printf("showing %d of %d - best match last\n", result->entry_count, result->total_count);
			deen_render_tty_or_nontty(tty, TTYSEQRESET, NULL);

			i = result->entry_count;

			do {
				i--;
				deen_render_rule(tty);
				deen_render_plain_entry(&result->entries[i], keywords, tty);
			}
			while(i > 0);
		}
	}
}
