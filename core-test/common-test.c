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

static void test_utf8_usascii_equivalent() {
	if(0 != strcmp((char *) deen_utf8_usascii_equivalent((uint8_t *) "\xc3\x9c", 2), "UE")) {
		deen_log_error_and_exit("failed test 'test_utf8_usascii_equivalent'");
	}

	if(NULL != deen_utf8_usascii_equivalent((uint8_t *) "Z", 1)) {
		deen_log_error_and_exit("failed test 'test_utf8_usascii_equivalent'");
	}

	DEEN_LOG_INFO0("passed test 'test_utf8_usascii_equivalent'");
}


static void test_is_common_upper_word() {
	if(DEEN_TRUE == deen_is_common_upper_word((uint8_t *) "FLIP", 4)) {
		deen_log_error_and_exit("failed test 'test_is_common_upper_word' - FLIP");
	}

	if(DEEN_FALSE == deen_is_common_upper_word((uint8_t *) "THE", 3)) {
		deen_log_error_and_exit("failed test 'test_is_common_upper_word' - THE");
	}

	if(DEEN_FALSE == deen_is_common_upper_word((uint8_t *) "F\xc3\x9cR", 4)) {
		deen_log_error_and_exit("failed test 'test_is_common_upper_word' - FUER");
	}

	DEEN_LOG_INFO0("passed test 'test_is_common_upper_word'");
}


static void test_utf8_is_usascii_clean() {
	if(DEEN_FALSE == deen_utf8_is_usascii_clean((uint8_t *) "FLIP", 4)) {
		deen_log_error_and_exit("failed test 'test_utf8_is_usascii_clean' - FLIP");
	}

	if(DEEN_TRUE == deen_utf8_is_usascii_clean((uint8_t *) "F\xc3\x9cR", 4)) {
		deen_log_error_and_exit("failed test 'test_utf8_is_usascii_clean' - FUER");
	}

	DEEN_LOG_INFO0("passed test 'test_utf8_is_usascii_clean'");
}



int main(int argc, char** argv) {
	test_utf8_usascii_equivalent();
	test_is_common_upper_word();
	test_utf8_is_usascii_clean();
	return 0;
}
