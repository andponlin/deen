/*
 * Copyright 2016-2017, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#include "common.h"
#include "constants.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>

// ---------------------------------------------------------------
// CREATION
// ---------------------------------------------------------------


extern void deen_entry_create_yy(
	const uint8_t *data,
	deen_entry_sub **subs,
	uint32_t *sub_count);


deen_entry deen_entry_create(
	const uint8_t *german,
	const uint8_t *english) {

	deen_entry result;

	deen_entry_create_yy(
		german,
		&(result.german_subs),
		&(result.german_sub_count));

	deen_entry_create_yy(
		english,
		&(result.english_subs),
		&(result.english_sub_count));

	return result;
}


// ---------------------------------------------------------------
// FREEING DATA
// ---------------------------------------------------------------


static void deen_entry_atom_free(deen_entry_atom *atom) {
	if (NULL!=atom) {
		if (NULL!=atom->text) {
			free((void *) atom->text);
		}
	}
}


static void deen_entry_sub_sub_free(deen_entry_sub_sub *sub_sub) {
	if (NULL != sub_sub) {
		uint32_t i;

		for (i=0;i<sub_sub->atom_count;i++) {
			deen_entry_atom_free(&(sub_sub->atoms[i]));
		}

		free((void *) sub_sub->atoms);
	}
}


static void deen_entry_sub_free(deen_entry_sub *sub) {
	if (NULL!=sub) {
		uint32_t i;

		for (i=0;i<sub->sub_sub_count;i++) {
			deen_entry_sub_sub_free(&(sub->sub_subs[i]));
		}

		free((void *) sub->sub_subs);
	}
}


void deen_entry_free(deen_entry *entry) {
	if (NULL!=entry) {
		uint32_t i;

		for (i=0;i<entry->english_sub_count;i++) {
			deen_entry_sub_free(&(entry->english_subs[i]));
		}

		for (i=0;i<entry->german_sub_count;i++) {
			deen_entry_sub_free(&(entry->german_subs[i]));
		}
	}
}


// ---------------------------------------------------------------
// SCORING
// ---------------------------------------------------------------


typedef struct
	deen_entry_calculate_distance_from_keywords_foreachword_callback_state
	deen_entry_calculate_distance_from_keywords_foreachword_callback_state;
struct deen_entry_calculate_distance_from_keywords_foreachword_callback_state
{
	uint32_t accumulated_distance_from_keyword;
	deen_keywords *keywords;
	deen_bool *keyword_use_map; // boolean
};


size_t deen_entry_find_first_keyword(
	const uint8_t *s,
	deen_keywords *keywords,
	uint32_t offset) {
	uint32_t i;

	for (i=0;i<keywords->count;i++) {
		if (deen_imatches_at(s, keywords->keywords[i], offset)) {
			return i;
		}
	}

	return DEEN_NOT_FOUND;
}


deen_bool deen_entry_calculate_distance_from_keywords_foreachword_callback(
	const uint8_t *s, size_t offset, size_t len, void *context) {

	deen_entry_calculate_distance_from_keywords_foreachword_callback_state *state =
		(deen_entry_calculate_distance_from_keywords_foreachword_callback_state *) context;

		// find the first keyword at the start of this string.

	size_t keyword_offset = deen_entry_find_first_keyword(s, state->keywords, offset);

	if (DEEN_NOT_FOUND == keyword_offset) {
		state->accumulated_distance_from_keyword += (uint32_t) len;
	}
	else {
		size_t keyword_len;
		size_t sequence_count;

		state->keyword_use_map[keyword_offset] = DEEN_TRUE;

		// how many letters (decoded from UTF-8) remain in the rest of the word?
		keyword_len = strlen((char *) state->keywords->keywords[keyword_offset]);

		switch (deen_utf8_sequences_count(&s[offset + keyword_len], len-keyword_len, &sequence_count)) {

			case DEEN_SEQUENCE_OK:
				state->accumulated_distance_from_keyword += sequence_count;
				break;

			case DEEN_BAD_SEQUENCE:
				deen_log_error_and_exit("encountered bad utf-8 sequence");
				break;

			case DEEN_INCOMPLETE_SEQUENCE:
				deen_log_error_and_exit("encountered incomplete utf-8 sequence");
				break;

		}
	}

	return DEEN_TRUE;
}


/*
If any keyword is not present at all then return the maximum value.  Otherwise,
for each word, find the longest characters that are not part of the keyword and
add all of those up.  Keywords are expected to be in order with largest first.
*/

static uint32_t deen_entry_sub_sub_calculate_distance_from_keywords(
	deen_entry_sub_sub *sub_sub,
	deen_keywords *keywords,
	deen_bool *keyword_use_map) {
	
	deen_entry_calculate_distance_from_keywords_foreachword_callback_state state;
	uint32_t i;

	memset(keyword_use_map, 0, (sizeof(deen_bool) * keywords->count));

	state.keywords = keywords;
	state.keyword_use_map = keyword_use_map;
	state.accumulated_distance_from_keyword = 0;

	for (i=0;i<sub_sub->atom_count;i++) {

		if (ATOM_TEXT == sub_sub->atoms[i].type) {

			deen_for_each_word(
				sub_sub->atoms[i].text, 0,
				&deen_entry_calculate_distance_from_keywords_foreachword_callback,
				&state);
		}
	}

	for (i=0;i<keywords->count;i++) {
		if (!keyword_use_map[i]) {
			return DEEN_MAX_SORT_DISTANCE_FROM_KEYWORDS;
		}
	}

	return state.accumulated_distance_from_keyword;
}


static uint32_t deen_entry_sub_calculate_distance_from_keywords(
	deen_entry_sub *sub,
	deen_keywords *keywords,
	deen_bool *keyword_use_map) {

	uint32_t i;
	int result = DEEN_MAX_SORT_DISTANCE_FROM_KEYWORDS;

	for (i=0;i<sub->sub_sub_count;i++) {
		uint32_t sub_sub_result = deen_entry_sub_sub_calculate_distance_from_keywords(
			&sub->sub_subs[i], keywords, keyword_use_map);

		if (sub_sub_result < result) {
			result = sub_sub_result;
		}
	}

	return result;
}


static uint32_t deen_entry_subs_calculate_distance_from_keywords(
	deen_entry_sub *subs,
	uint32_t sub_count,
	deen_keywords *keywords,
	deen_bool *keyword_use_map) {

	uint32_t result = DEEN_MAX_SORT_DISTANCE_FROM_KEYWORDS;
	uint32_t i;

	for (i=0;i < sub_count;i++) {

		uint32_t sub_result = deen_entry_sub_calculate_distance_from_keywords(
			&subs[i],
			keywords,
			keyword_use_map);

		DEEN_LOG_TRACE2(" %u) distance from keyword; %u", i, sub_result);

		if (sub_result < result) {
			result = sub_result;
		}
	}

	return result;
}


uint32_t deen_entry_calculate_distance_from_keywords(
	deen_entry *entry,
	deen_keywords *keywords,
	deen_bool *keyword_use_map) {

	uint32_t german_result = deen_entry_subs_calculate_distance_from_keywords(
		entry->german_subs, entry->german_sub_count, keywords, keyword_use_map);

	uint32_t english_result = deen_entry_subs_calculate_distance_from_keywords(
		entry->english_subs, entry->english_sub_count, keywords, keyword_use_map);

	if (german_result < english_result) {
		return german_result;
	}

	return english_result;
}
