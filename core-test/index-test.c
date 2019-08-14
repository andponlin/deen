/*
 * Copyright 2016, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

/*
This test is really an end-to-end test for just the index part.  It will go
through the process of creating an index database, putting some data into the
index and then making a basic query of it to make sure that the expected
results come out.
*/

#include <stdio.h>
#include <sqlite3.h>

#include "core/index.h"
#include "core/common.h"
#include "core/types.h"

#define OUTPUT_DATABASE_FILE "tmp_index_e2e.sqlite"

static void test_index_e2e_setup(sqlite3 *db) {
	DEEN_LOG_TRACE0("will init database...");
	deen_index_init(db);

	DEEN_LOG_TRACE0("will create add context...");
	deen_index_add_context *add_context = deen_index_add_context_create(db);

	DEEN_LOG_TRACE0("add to index...");
	{
		uint8_t *prefixes[3] = {
			(uint8_t *) "ERT",
			(uint8_t *) "RAT",
			(uint8_t *) "DAT"
		};

		deen_index_add(add_context, 123, prefixes, 3);
	}

	{
		uint8_t *prefixes[3] = {
			(uint8_t *) "ZEE",
			(uint8_t *) "RAT",
			(uint8_t *) "PIN"
		};

		deen_index_add(add_context, 456, prefixes, 3);
	}

	{
		uint8_t *prefixes[3] = {
			(uint8_t *) "PIG",
			(uint8_t *) "ZIG",
			(uint8_t *) "DIG"
		};

		deen_index_add(add_context, 789, prefixes, 3);
	}

	DEEN_LOG_TRACE0("close add context...");
	deen_index_add_context_free(add_context);
}

static deen_bool test_index_e2e_find_ref(deen_index_lookup_result *result, off_t expected) {
	for(int i = 0; i < result->refs_count; i++) {
		if(result->refs[i] == expected) {
			return DEEN_TRUE;
		}
	}

	return DEEN_FALSE;
}

static deen_bool test_index_e2e_lookup(sqlite3 *db) {

	DEEN_LOG_TRACE0("perform lookup...");
	deen_index_lookup_result *lookup_result = deen_index_lookup(db, (uint8_t *) "RAT");
	deen_bool result = DEEN_TRUE;

	if (2 != lookup_result->refs_count) {
		DEEN_LOG_ERROR0("not able to find the expected references");
	}

	if (DEEN_TRUE != test_index_e2e_find_ref(lookup_result, 123)) {
		DEEN_LOG_ERROR0("not able to find the expected ref 123");
		result = DEEN_FALSE;
	}

	if (DEEN_TRUE != test_index_e2e_find_ref(lookup_result, 456)) {
		DEEN_LOG_ERROR0("not able to find the expected ref 456");
		result = DEEN_FALSE;
	}

	DEEN_LOG_TRACE0("free results...");
	deen_index_lookup_result_free(lookup_result);

	return result;
}

 /*
 This is an end-to-end test of the indexing.  So it will create an index data
 set, it will load some index data and it will then query that data to make
 sure that it generates sensible, expected results.
 */

 static void test_index_e2e() {

	 sqlite3 *db = NULL;
	deen_bool result = DEEN_TRUE;

	 DEEN_LOG_TRACE0("running test 'test_index_e2e'");

	 DEEN_LOG_TRACE0("will open database...");
	 if (SQLITE_OK != sqlite3_open_v2(
		 OUTPUT_DATABASE_FILE, &db,
		 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
		 NULL)) {
			 result = DEEN_FALSE;
			 DEEN_LOG_ERROR0("not able to create database");
	 }

	 if (DEEN_TRUE == result) {
		 test_index_e2e_setup(db);
	}

	 result = result && test_index_e2e_lookup(db);

	 if(NULL != db) {
		DEEN_LOG_TRACE0("will close database...");
		sqlite3_close_v2(db);

		DEEN_LOG_TRACE0("will remove database file...");
		if (0 != remove(OUTPUT_DATABASE_FILE)) {
			result = DEEN_FALSE;
			DEEN_LOG_ERROR0("unable to delete the temporary database file.");
		}
 	}

	if (DEEN_TRUE == result) {
		DEEN_LOG_INFO0("passed test 'test_index_e2e'");
	} else {
		deen_log_error_and_exit("failed test 'test_index_e2e'");
	}

 }

 // ---------------------------------------------------------------
 // DRIVING THE TEST
 // ---------------------------------------------------------------


 int main(int argc, char** argv) {

 	test_index_e2e();

 	return 0;
 }
