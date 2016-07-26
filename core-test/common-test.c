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
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

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

// ---------------------------------------------------------------
// FOR EACH WORD FROM FILE
// ---------------------------------------------------------------

/*
This call-back method can be employed to produce the sample output that is then
subsequently read-in for processing.
*/

/*
static deen_bool test_for_each_word_from_file_reference_callback(
	const uint8_t *s,
	size_t len,
	off_t ref, // offset after last newline.
	float progress,
	void *context) {

	printf("%llu,", ref);

	for(size_t i = 0;i < len; i++) {
		putc(s[i], stdout);
	}

	printf("\n");

	return DEEN_TRUE; // keep processing.
}
*/


/*
This callback is used to check the words found are as expected based on a
reference output that was generated previously.
*/

static deen_bool test_for_each_word_from_file_check_callback(
	const uint8_t *s,
	size_t len,
	off_t ref, // offset after last newline.
	float progress,
	void *context) {

	FILE *reference_file = (FILE *) context;
	int reference_ref = -1;
	uint8_t reference_word[32];

	if (2 != fscanf(
		reference_file, "%d,%s\n",
		&reference_ref, (char *) reference_word)) {
		deen_log_error_and_exit("failed test 'test_for_each_word_from_file' -- unable to read the required reference data");
	}

	if (ref != (off_t) reference_ref) {
		deen_log_error_and_exit(
			"failed test 'test_for_each_word_from_file' -- ref mismatch (expected; %d, actual; %llu)",
			reference_ref,
			ref);
	}

	if (strlen((char *) reference_word) != len
		|| 0 != memcmp(s, reference_word, len)) {
			deen_log_error_and_exit(
				"failed test 'test_for_each_word_from_file' -- word mismatch (expected; %s)",
				(char *) reference_word);
	}

	return DEEN_TRUE; // keep processing.
}


/*
This test works by reading in the input file and processing it to produce a list
of words.  The words are then compared to another file which is read in order to
get a "known correct output".  If the comparison goes OK then the test has been
successful.
*/

static void test_for_each_word_from_file() {
	int fd = open("core-test/input_for_each_word_from_file_a.txt", O_RDONLY);

	if (-1 == fd) {
		deen_log_error_and_exit("failed test 'test_for_each_word_from_file' -- unable to open test data");
	}

	FILE *reference_file = fopen("core-test/output_for_each_word_from_file_a.txt", "r");

	if (NULL == reference_file) {
		deen_log_error_and_exit("failed test 'test_for_each_word_from_file' -- unable to open reference data");
	}

	deen_for_each_word_from_file(
		4, // crazy small to better check the buffer handling
		fd,
		// &test_for_each_word_from_file_reference_callback,
		&test_for_each_word_from_file_check_callback,
		(void *) reference_file);

	close(fd);
	fclose(reference_file);

	DEEN_LOG_INFO0("passed test 'test_for_each_word_from_file'");
}


// ---------------------------------------------------------------
// FOR EACH WORD FROM MEMORY
// ---------------------------------------------------------------


static uint8_t *test_for_each_word_expected(size_t offset) {
	switch(offset) {
		case 0: return (uint8_t *) "Es";
		case 1: return (uint8_t *) "ist";
		case 2: return (uint8_t *) "mir";
		case 3: return (uint8_t *) "k\xc3\x84se";
		default:
			deen_log_error_and_exit("failed test 'test_for_each_word' -- word out of range");
	}

	return NULL;
}


static deen_bool test_for_each_word_check_callback(
	const uint8_t *s,
	size_t offset,
	size_t len,
	void *context) {

	size_t *word_offset = (size_t *) context;
	uint8_t *word = test_for_each_word_expected(*word_offset);
	size_t word_len = strlen((char *) word);

	if (
		word_len != len
		|| 0 != memcmp(&s[offset],word,sizeof(uint8_t) * len)) {
		deen_log_error_and_exit("failed test 'test_for_each_word' -- at %s", word);
	}

	word_offset[0]++;

	return DEEN_TRUE;
}


/*
Tests that words can be found in a string.
*/


static void test_for_each_word() {
	uint8_t *sample = (uint8_t *) "  Es ist mir k\xc3\x84se";
	size_t upto = 0;

	deen_for_each_word(
		sample,
		0,
		&test_for_each_word_check_callback,
		&upto);

	DEEN_LOG_INFO0("passed test 'test_for_each_word'");
}

// ---------------------------------------------------------------

static void test_to_upper() {
	uint8_t *sample = (uint8_t *) "pL\xc3\xb6tzLich";
	uint8_t *expected = (uint8_t *) "PL\xc3\x96TZLICH";
	size_t len = strlen((char *) sample);
	uint8_t *buffer[128];

	memcpy(buffer, sample, sizeof(uint8_t) * (len + 1));

	// - - - - - - - - - -
	deen_to_upper((uint8_t *) buffer);
	// - - - - - - - - - -

	if (0 != memcmp(buffer, expected, sizeof(uint8_t) * len)) {
		deen_log_error_and_exit("failed test 'test_to_upper'");
	}

	DEEN_LOG_INFO0("passed test 'test_to_upper'");
}


static void test_imatches_at__positive() {
	uint8_t *sample = (uint8_t *) "pL\xc3\xb6tzLich";
	uint8_t *part = (uint8_t *) "L\xc3\xb6t";

	// - - - - - - - - - -
	if(DEEN_TRUE != deen_imatches_at(sample, part, 1)) {
		deen_log_error_and_exit("failed test 'test_imatches_at__positive'");
	}
	// - - - - - - - - - -

	DEEN_LOG_INFO0("passed test 'test_imatches_at__positive'");
}

static void test_imatches_at__negative() {
	uint8_t *sample = (uint8_t *) "pL\xc3\xb6tzLich";
	uint8_t *part = (uint8_t *) "L\xc3\xb6t";

	// - - - - - - - - - -
	if(DEEN_FALSE != deen_imatches_at(sample, part, 2)) {
		deen_log_error_and_exit("failed test 'test_imatches_at__negative'");
	}
	// - - - - - - - - - -

	DEEN_LOG_INFO0("passed test 'test_imatches_at__negative'");
}


static void test_ifind_first__positive() {
	uint8_t *sample = (uint8_t *) "pL\xc3\xb6tzLich";
	uint8_t *part = (uint8_t *) "\xc3\xb6";

	// - - - - - - - - - -
	if(2 != deen_ifind_first(sample, part, 1,7)) {
		deen_log_error_and_exit("failed test 'test_ifind_first__positive'");
	}
	// - - - - - - - - - -

	DEEN_LOG_INFO0("passed test 'test_ifind_first__positive'");
}

static void test_ifind_first__negative() {
	uint8_t *sample = (uint8_t *) "pL\xc3\xb6tzLich";
	uint8_t *part = (uint8_t *) "\xc3\xb6";

	// - - - - - - - - - -
	if(DEEN_NOT_FOUND != deen_ifind_first(sample, part, 3,7)) {
		deen_log_error_and_exit("failed test 'test_ifind_first__negative'");
	}
	// - - - - - - - - - -

	DEEN_LOG_INFO0("passed test 'test_ifind_first__negative'");
}

// ---------------------------------------------------------------
// DRIVING THE TESTS
// ---------------------------------------------------------------


int main(int argc, char** argv) {

	test_utf8_usascii_equivalent();
	test_is_common_upper_word();
	test_utf8_is_usascii_clean();
	test_utf8_crop_to_unicode_len();
	test_utf8_sequences_count__ok_accented();
	test_utf8_sequences_count__incomplete_sequence();
	test_utf8_sequence_len__accented();
	test_utf8_sequence_len__non_accented();
	test_for_each_word_from_file();
	test_for_each_word();
	test_to_upper();
	test_imatches_at__positive();
	test_imatches_at__negative();
	test_ifind_first__positive();
	test_ifind_first__negative();

	return 0;
}
