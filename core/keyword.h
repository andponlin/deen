/*
 * Copyright 2016, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */
 
#ifndef __KEYWORD_H
#define __KEYWORD_H

#include "common.h"

// ---------------------------------------------------------------

deen_keywords *deen_keywords_create();

void deen_keywords_free(deen_keywords *keywords);

void deen_keywords_add_from_string(deen_keywords *keywords, uint8_t *input);

size_t deen_keywords_longest_keyword(deen_keywords *keywords);

deen_bool deen_keywords_all_present(deen_keywords *keywords, const uint8_t *input);

/**
 * In some cases, the keywords can contain abbreviations and so on that make
 * searches difficult in the data.  For example, the string "oe" can be an
 * abbreviation for o-umlaut and so if nothing is found, the keywords can be
 * tweaked so that searches work.
 */

deen_bool deen_keywords_adjust(deen_keywords *keywords);

// ---------------------------------------------------------------


#endif /* __KEYWORD_H */
