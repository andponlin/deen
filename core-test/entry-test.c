/*
 * Copyright 2016, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#include "core/common.h"
#include "core/entry.h"
#include "core/keyword.h"

#include <string.h>
#include <stdlib.h>

static deen_entry deen_create_example_1() {
	return deen_entry_create(
		(const uint8_t *) "W\xc3\xbcsst {m} [zool.] | Regensburg [geol.]; Donnau {f} {pl} [geol.]",
		(const uint8_t *) "Chop [sport]; Peanutbutter Sauce | Toe [Br.]");
}

/*
 This test will take a relatively complex entry, will parse it and then check
 that the resultant output is correct.
 */

static void test_create() {
	// - - - - - - - - - -
	deen_entry entry = deen_create_example_1();
	// - - - - - - - - - -

	deen_bool result = DEEN_TRUE;

	if (2 != entry.english_sub_count) {
		result = DEEN_FALSE;
		DEEN_LOG_ERROR0("wrong number of sub count for english entry");
	}

	if (2 != entry.german_sub_count) {
		result = DEEN_FALSE;
		DEEN_LOG_ERROR0("wrong number of sub count for german entry");
	}

	if (2 != entry.german_subs[1].sub_sub_count) {
		result = DEEN_FALSE;
		DEEN_LOG_ERROR0("wrong number of sub sub count for german entry 1");
	}

	if (2 != entry.german_subs[1].sub_sub_count) {
		result = DEEN_FALSE;
		DEEN_LOG_ERROR0("wrong number of sub sub count for german entry 1");
	}

	if (4 != entry.german_subs[1].sub_subs[1].atom_count) {
		result = DEEN_FALSE;
		DEEN_LOG_ERROR0("wrong number of sub sub atom count for german entry 1, sub sub 1");
	}

	deen_entry_atom *atoms = entry.german_subs[1].sub_subs[1].atoms;

	if (ATOM_TEXT != atoms[0].type || 0 != strcmp((char *) atoms[0].text, "Donnau")) {
		result = DEEN_FALSE;
		DEEN_LOG_ERROR0("bad atom 0");
	}

	if (ATOM_GRAMMAR != atoms[1].type || 0 != strcmp((char *) atoms[1].text, "f")) {
		result = DEEN_FALSE;
		DEEN_LOG_ERROR0("bad atom 1");
	}

	if (ATOM_GRAMMAR != atoms[2].type || 0 != strcmp((char *) atoms[2].text, "pl")) {
		result = DEEN_FALSE;
		DEEN_LOG_ERROR0("bad atom 2");
	}

	if (ATOM_CONTEXT != atoms[3].type || 0 != strcmp((char *) atoms[3].text, "geol.")) {
		result = DEEN_FALSE;
		DEEN_LOG_ERROR0("bad atom 3");
	}

	deen_entry_free(&entry);

	if(DEEN_TRUE != result) {
		deen_log_error_and_exit("failed test 'test_create' -- bad parse");
	}

	DEEN_LOG_INFO0("passed test 'test_create'");
}

/*
 The expected found bitmap is a 32bit number each bit of which indicates
 if the keyword should have been found or not.
 */

static void test_entry_calculate_distance_from_keywords__generic(
	char *test_name,
	uint8_t *keywords_str,
	uint32_t expected_distance
) {
	deen_keywords *keywords = deen_keywords_create();
	deen_keywords_add_from_string(keywords, (uint8_t *) keywords_str);
	deen_entry entry = deen_create_example_1();
	deen_bool *keyword_use_map = (deen_bool *) deen_emalloc(
		sizeof(deen_bool) * keywords->count);

	// - - - - - - - - - -
	uint32_t actual_distance = deen_entry_calculate_distance_from_keywords(
		&entry, keywords, keyword_use_map);
	// - - - - - - - - - -


	deen_keywords_free(keywords);
	free(keyword_use_map);

	if (expected_distance != actual_distance) {
		deen_log_error_and_exit(
			"failed test '%s' -- expected %d, was %d",
			test_name, expected_distance, actual_distance);
	}

	DEEN_LOG_INFO1("passed test '%s'", test_name);
}

static void test_entry_calculate_distance_from_keywords__ok() {
	test_entry_calculate_distance_from_keywords__generic(
		"test_entry_calculate_distance_from_keywords__ok",
		(uint8_t *) "REGENS", 4);
}

static void test_entry_calculate_distance_from_keywords__full_match() {
	test_entry_calculate_distance_from_keywords__generic(
		"test_entry_calculate_distance_from_keywords__full_match",
		(uint8_t *) "REGENSBURG", 0);
}

static void test_entry_calculate_distance_from_keywords__not_found() {
	test_entry_calculate_distance_from_keywords__generic(
		"test_entry_calculate_distance_from_keywords__not_found",
		(uint8_t *) "ISLANDS", DEEN_MAX_SORT_DISTANCE_FROM_KEYWORDS);
}

static void test_entry_calculate_distance_from_keywords__two_keywords() {
	test_entry_calculate_distance_from_keywords__generic(
		"test_entry_calculate_distance_from_keywords__two_keywords",
		(uint8_t *) "PEANUT SAU", 6 + 2);
}

int main(int argc, char** argv) {
	test_create();
	test_entry_calculate_distance_from_keywords__ok();
	test_entry_calculate_distance_from_keywords__full_match();
	test_entry_calculate_distance_from_keywords__not_found();
	test_entry_calculate_distance_from_keywords__two_keywords();
	return 0;
}
