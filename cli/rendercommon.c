/*
 * Copyright 2016-2017, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#include "rendercommon.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/common.h"
#include "core/constants.h"


#ifndef __MINGW32__
static deen_bool deen_term_is_utf8_langenv(char *lang_value) {
	if (NULL!=lang_value) {
		int len = strlen(lang_value);
		return len > 6 && (
			0 == strcmp(&lang_value[len-5], ".utf8") ||
			0 == strcmp(&lang_value[len-6], ".UTF-8")
		);
	}

	return DEEN_FALSE;
}
#endif

deen_bool deen_term_is_utf8() {
#ifdef __MINGW32__
	return DEEN_TRUE; // this is forced by console setup.
#else
	return
		deen_term_is_utf8_langenv(getenv("LANG")) ||
		deen_term_is_utf8_langenv(getenv("LC_CTYPE"));
#endif
}


void deen_term_print_str(uint8_t *str) {
	deen_term_print_str_range(str, 0, strlen((char *) str));
}


static void deen_term_print_ascii_str(uint8_t *str, size_t from, size_t to) {
	size_t i;

	for (i = from; i<to; i++) {
		fputc((int) str[i], stdout);
	}
}


void deen_term_print_str_range(uint8_t *str, size_t from, size_t to) {
	if (deen_term_is_utf8()) {
		deen_term_print_ascii_str(str, from, to);
	}
	else {
		if (deen_utf8_is_usascii_clean(&str[from], to-from)) {
			deen_term_print_ascii_str(str, from, to);
		}
		else {
			size_t i;
			size_t len = to-from;

			for (i=from;i<to;) {
				size_t sequence_length;

				switch (deen_utf8_sequence_len(&str[i], len - i, &sequence_length)) {

					case DEEN_SEQUENCE_OK:
						if (1 == sequence_length) {
							putchar((int) str[i]);
						}
						else {
							uint8_t *equivalent = deen_utf8_usascii_equivalent(&str[i], len - i);

							if (NULL!=equivalent) {
								fputs((char *) equivalent, stdout);
							}
							else {
								putchar((int) '?');
							}
						}

						i += sequence_length;

						break;

					case DEEN_BAD_SEQUENCE:
						DEEN_LOG_ERROR0("bad utf-8 sequence");
						return;

					case DEEN_INCOMPLETE_SEQUENCE:
						DEEN_LOG_ERROR0("incomplete utf-8 sequence");
						return;

				}
			}
		}
	}
}
