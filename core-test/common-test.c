/*
 * Copyright 2016, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#import "core/types.h"
#import "core/common.h"

#include <string.h>
#include <stdlib.h>

static void test_utf8_usascii_equivalent() {
	if (0 != strcmp((char *) deen_utf8_usascii_equivalent((uint8_t *) "\xc3\x9c", 2), "UE")) {
		deen_log_error_and_exit("failed test 'test_utf8_usascii_equivalent'");
	}

	if (NULL != deen_utf8_usascii_equivalent((uint8_t *) "Z", 1)) {
		deen_log_error_and_exit("failed test 'test_utf8_usascii_equivalent'");
	}

	DEEN_LOG_INFO0("passed test 'test_utf8_usascii_equivalent'");
}


static void test_is_common_upper_word() {
	if (DEEN_TRUE == deen_is_common_upper_word((uint8_t *) "FLIP", 4)) {
		deen_log_error_and_exit("failed test 'test_is_common_upper_word' - FLIP");
	}

	if (DEEN_FALSE == deen_is_common_upper_word((uint8_t *) "THE", 3)) {
		deen_log_error_and_exit("failed test 'test_is_common_upper_word' - THE");
	}

	if (DEEN_FALSE == deen_is_common_upper_word((uint8_t *) "F\xc3\x9cR", 4)) {
		deen_log_error_and_exit("failed test 'test_is_common_upper_word' - FUER");
	}

	DEEN_LOG_INFO0("passed test 'test_is_common_upper_word'");
}


static void test_utf8_is_usascii_clean() {
	if (DEEN_FALSE == deen_utf8_is_usascii_clean((uint8_t *) "FLIP", 4)) {
		deen_log_error_and_exit("failed test 'test_utf8_is_usascii_clean' - FLIP");
	}

	if (DEEN_TRUE == deen_utf8_is_usascii_clean((uint8_t *) "F\xc3\x9cR", 4)) {
		deen_log_error_and_exit("failed test 'test_utf8_is_usascii_clean' - FUER");
	}

	DEEN_LOG_INFO0("passed test 'test_utf8_is_usascii_clean'");
}


static void test_utf8_crop_to_unicode_len() {
	uint8_t *buffer = deen_emalloc(sizeof(uint8_t) * 5);
	strcpy((char *) buffer, "F\xc3\x9cR");

	// - - - - - - - - - -
	deen_utf8_crop_to_unicode_len(buffer, 4, 2);
	// - - - - - - - - - -

	if (0 != buffer[3]) {
		deen_log_error_and_exit("failed test 'test_utf8_crop_to_unicode_len'");
	}

	free((void *) buffer);
	DEEN_LOG_INFO0("passed test 'test_utf8_crop_to_unicode_len'");
}


static void test_utf8_sequences_count__ok_accented() {
	size_t sequence_count;
	deen_utf8_sequence_result sequence_result;

	// - - - - - - - - - -
	sequence_result = deen_utf8_sequences_count((uint8_t *) "F\xc3\x9cR", 4, &sequence_count);
	// - - - - - - - - - -

	if (DEEN_SEQUENCE_OK != sequence_result) {
		deen_log_error_and_exit("failed test 'test_utf8_sequences_count__ok_accented' -- sequence result");
	}

	if (3 != sequence_count) {
		deen_log_error_and_exit("failed test 'test_utf8_sequences_count__ok_accented' -- wrong length");
	}

	DEEN_LOG_INFO0("passed test 'test_utf8_sequences_count__ok_accented'");
}


static void test_utf8_sequences_count__incomplete_sequence() {
	size_t sequence_count;
	deen_utf8_sequence_result sequence_result;

	// - - - - - - - - - -
	sequence_result = deen_utf8_sequences_count((uint8_t *) "F\xc3", 2, &sequence_count);
	// - - - - - - - - - -

	if (DEEN_INCOMPLETE_SEQUENCE != sequence_result) {
		deen_log_error_and_exit("failed test 'test_utf8_sequences_count__incomplete_sequence' -- sequence result");
	}

	DEEN_LOG_INFO0("passed test 'test_utf8_sequences_count__incomplete_sequence'");
}


static void test_utf8_sequence_len__accented() {
	uint8_t *s = (uint8_t *) "\xc3\x9c";
	size_t sequence_length;
	deen_utf8_sequence_result sequence_result;

	sequence_result = deen_utf8_sequence_len(s, 2, &sequence_length);

	if (DEEN_SEQUENCE_OK != sequence_result) {
		deen_log_error_and_exit("failed test 'test_utf8_sequence_len__accented' -- sequence result");
	}

	if (2 != sequence_length) {
		deen_log_error_and_exit("failed test 'test_utf8_sequence_len__accented' -- wrong length");
	}

	DEEN_LOG_INFO0("passed test 'test_utf8_sequence_len__accented'");
}


static void test_utf8_sequence_len__non_accented() {
	uint8_t *s = (uint8_t *) "F";
	size_t sequence_length;
	deen_utf8_sequence_result sequence_result;

	sequence_result = deen_utf8_sequence_len(s, 1, &sequence_length);

	if (DEEN_SEQUENCE_OK != sequence_result) {
		deen_log_error_and_exit("failed test 'test_utf8_sequence_len__non_accented' -- sequence result");
	}

	if (1 != sequence_length) {
		deen_log_error_and_exit("failed test 'test_utf8_sequence_len__non_accented' -- wrong length");
	}

	DEEN_LOG_INFO0("passed test 'test_utf8_sequence_len__non_accented'");
}


int main(int argc, char** argv) {

	test_utf8_usascii_equivalent();
	test_is_common_upper_word();
	test_utf8_is_usascii_clean();
	test_utf8_crop_to_unicode_len();
	test_utf8_sequences_count__ok_accented();
	test_utf8_sequences_count__incomplete_sequence();
	test_utf8_sequence_len__accented();
	test_utf8_sequence_len__non_accented();

	return 0;
}
