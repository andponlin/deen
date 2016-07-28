/*
 * Copyright 2016, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#include "core/types.h"
#include "core/common.h"
#include "core/keyword.h"

#include <string.h>

static void test_keywords_all_present() {
	deen_keywords *keywords = deen_keywords_create();

	deen_keywords_add_from_string(keywords, (uint8_t *) "YERT");

	// - - - - - - - - - -
	if(DEEN_TRUE != deen_keywords_all_present(keywords, (uint8_t *) "zing {adj} | Zing Zong Ting | Yert")) {
		deen_log_error_and_exit("failed test 'test_keywords_all_present'");
	}
	// - - - - - - - - - -

	deen_keywords_free(keywords);

	DEEN_LOG_INFO0("passed test 'test_keywords_all_present'");
}


static void test_keywords_longest_keyword() {
	deen_keywords *keywords = deen_keywords_create();

	deen_keywords_add_from_string(keywords, (uint8_t *) "De");
	deen_keywords_add_from_string(keywords, (uint8_t *) "Bethels");
	deen_keywords_add_from_string(keywords, (uint8_t *) "Piha");

	// - - - - - - - - - -
	if(7 != deen_keywords_longest_keyword(keywords)) {
		deen_log_error_and_exit("failed test 'test_keywords_longest_keyword'");
	}
	// - - - - - - - - - -

	deen_keywords_free(keywords);

	DEEN_LOG_INFO0("passed test 'test_keywords_longest_keyword'");
}


static void test_keywords_adjust() {
	uint8_t *keyword;
	deen_keywords *keywords = deen_keywords_create();
	deen_keywords_add_from_string(keywords, (uint8_t *) "KOENIG");

	// - - - - - - - - - -
	deen_keywords_adjust(keywords);
	// - - - - - - - - - -

	keyword = keywords->keywords[0];

	if(0 != memcmp(keyword, "K\xC3\x96NIG", 6)) {
		deen_log_error_and_exit("failed test 'test_keywords_adjust'");
	}

	deen_keywords_free(keywords);

	DEEN_LOG_INFO0("passed test 'test_keywords_adjust'");
}


int main(int argc, char** argv) {
	test_keywords_all_present();
	test_keywords_longest_keyword();
	test_keywords_adjust();
	return 0;
}
