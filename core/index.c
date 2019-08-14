/*
 * Copyright 2016-2017, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#include "index.h"

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "common.h"

// transaction
#define SQL_TRANSACTION_BEGIN "BEGIN"
#define SQL_TRANSACTION_COMMIT "COMMIT"

// init
#define SQL_TABLE_PREFIX_CREATE "CREATE TABLE deen_prefix(id INTEGER PRIMARY KEY, prefix VARCHAR(4) UNIQUE NOT NULL)"
#define SQL_TABLE_PREFIX_INDEX_CREATE "CREATE UNIQUE INDEX deen_prefix_idx01 ON deen_prefix(prefix)"
#define SQL_TABLE_REF_CREATE "CREATE TABLE deen_ref(id INTEGER PRIMARY KEY, deen_prefix_id INTEGER NOT NULL, ref NUMBER NOT NULL, FOREIGN KEY (deen_prefix_id) REFERENCES deen_prefix(id))"
#define SQL_TABLE_REF_INDEX_CREATE "CREATE UNIQUE INDEX deen_ref_idx01 ON deen_ref(deen_prefix_id, ref)"

// adding
#define SQL_PREFIX_BULK_FETCH "SELECT id, prefix FROM deen_prefix WHERE prefix IN "
#define SQL_PREFIX_INSERT "INSERT INTO deen_prefix(prefix) VALUES (?)"
#define SQL_PREFIX_REF_INSERT "INSERT INTO deen_ref (deen_prefix_id, ref) VALUES "

// searching
#define SQL_REF_LOOKUP "SELECT r.ref FROM deen_ref r JOIN deen_prefix p ON p.id = r.deen_prefix_id WHERE p.prefix = ?"


static void deen_index_run_sql(sqlite3 *db, char *sql) {
	sqlite3_stmt *stmt = NULL;

	if (SQLITE_OK != sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL)) {
		deen_log_error_and_exit("sqllite error preparing statement for [%s]; %s", sql, sqlite3_errmsg(db));
	}

	if (SQLITE_DONE != sqlite3_step(stmt)) {
		deen_log_error_and_exit("unable to execute statement for [%s]; %s", sql, sqlite3_errmsg(db));
	}

	if (SQLITE_OK != sqlite3_finalize(stmt)) {
		deen_log_error_and_exit("sqllite error finalizing statement for [%s]; %s", sql, sqlite3_errmsg(db));
	}
}


void deen_index_init(sqlite3 *db) {
	deen_index_run_sql(db, SQL_TABLE_PREFIX_CREATE);
	deen_index_run_sql(db, SQL_TABLE_PREFIX_INDEX_CREATE);
	deen_index_run_sql(db, SQL_TABLE_REF_CREATE);
	deen_index_run_sql(db, SQL_TABLE_REF_INDEX_CREATE);
}


void deen_transaction_begin(sqlite3 *db) {
	sqlite3_exec(db, SQL_TRANSACTION_BEGIN, NULL, NULL, NULL);
}


void deen_transaction_commit(sqlite3 *db) {
	sqlite3_exec(db, SQL_TRANSACTION_COMMIT, NULL, NULL, NULL);
}


deen_index_add_context *deen_index_add_context_create(sqlite3 *db) {
	deen_index_add_context *result = (deen_index_add_context *) deen_emalloc(sizeof(deen_index_add_context));
	memset(result, 0, sizeof(deen_index_add_context));
	result->db = db;
	return result;
}


void deen_index_add_context_free(deen_index_add_context *context) {
	if (NULL!=context) {

		if (NULL!= context->prefix_insert_stmt) {
			if (SQLITE_OK != sqlite3_finalize(context->prefix_insert_stmt)) {
				deen_log_error_and_exit("sqllite error finalizing prefix insert stmt; %s", sqlite3_errmsg(context->db));
			}
		}

		if (0 != context->find_existing_prefixes_stmts_count) {
			int i;

			for (i=0; i<context->find_existing_prefixes_stmts_count; i++) {
				if (NULL != context->find_existing_prefixes_stmts[i]) {
					if (SQLITE_OK != sqlite3_finalize(context->find_existing_prefixes_stmts[i])) {
						deen_log_error_and_exit("sqllite error finalizing find existing prefixes stmt; %s", sqlite3_errmsg(context->db));
					}
				}
			}

			free((void *) context->find_existing_prefixes_stmts);
		}

		if (0 != context->ref_insert_stmts_count) {
			int i;

			for (i=0; i<context->ref_insert_stmts_count; i++) {
				if (NULL != context->ref_insert_stmts[i]) {
					if (SQLITE_OK != sqlite3_finalize(context->ref_insert_stmts[i])) {
						deen_log_error_and_exit("sqllite error finalizing find existing prefixes stmt; %s", sqlite3_errmsg(context->db));
					}
				}
			}

			free((void *) context->ref_insert_stmts);
		}
	}
}


static sqlite3_stmt *deen_index_get_or_create_find_existing_prefixes_stmt(
	deen_index_add_context *index_add_context,
	size_t prefix_count
) {
	if (prefix_count > 1000) {
		deen_log_error_and_exit("proposterous quantity of prefixes to search for; %u", prefix_count);
	}

	if (prefix_count >= index_add_context->find_existing_prefixes_stmts_count) {
		size_t i;

		index_add_context->find_existing_prefixes_stmts = deen_erealloc(
			index_add_context->find_existing_prefixes_stmts,
			sizeof(sqlite3_stmt *) * prefix_count
		);

		for (
			i=index_add_context->find_existing_prefixes_stmts_count;
			i < prefix_count;
			i++) {
			index_add_context->find_existing_prefixes_stmts[i] = NULL;
		}
	}

	if(NULL == index_add_context->find_existing_prefixes_stmts[prefix_count - 1]) {
		uint32_t i;
		size_t len = strlen(SQL_PREFIX_BULK_FETCH) + (size_t) (3 + ((prefix_count-1) * 2));
		char *sql = deen_emalloc(len + 1); // +1 for the NULL at the end
		strcpy(sql, SQL_PREFIX_BULK_FETCH);
		strcat(sql, "(");

		for (i = 0; i < prefix_count; i++) {
			if (0 != i) {
				strcat(sql, ",");
			}

			strcat(sql, "?");
		}

		strcat(sql, ")");

		if (SQLITE_OK != sqlite3_prepare_v2(
			index_add_context->db,
			sql, len,
			&(index_add_context->find_existing_prefixes_stmts[prefix_count - 1]),
			NULL)) {
			deen_log_error_and_exit("sqllite error preparing statement for [%s]; %s", sql, sqlite3_errmsg(index_add_context->db));
		}

		free((void *) sql);
	}

	return index_add_context->find_existing_prefixes_stmts[prefix_count - 1];
}


static sqlite3_stmt *deen_index_get_or_create_ref_insert_stmt(
	deen_index_add_context *index_add_context,
	size_t tuple_count) {

	if (tuple_count > 1000) {
		deen_log_error_and_exit("proposterous quantity of tupls to insert; %u", tuple_count);
	}

	if (tuple_count >= index_add_context->ref_insert_stmts_count) {
		size_t i;

		index_add_context->ref_insert_stmts = deen_erealloc(
			index_add_context->ref_insert_stmts,
			sizeof(sqlite3_stmt *) * tuple_count
		);

		for (
			i=index_add_context->ref_insert_stmts_count;
			i < tuple_count;
			i++) {
			index_add_context->ref_insert_stmts[i] = NULL;
		}
	}

	if(NULL == index_add_context->ref_insert_stmts[tuple_count - 1]) {
		uint32_t i;
		size_t len = strlen(SQL_PREFIX_REF_INSERT) + (size_t) (tuple_count * 6);
		char *sql = deen_emalloc(len + 1); // +1 for the NULL at the end
		strcpy(sql, SQL_PREFIX_REF_INSERT);

		for (i = 0; i < tuple_count; i++) {
			if (0 != i) {
				strcat(sql, ",");
			}

			strcat(sql, "(?,?)");
		}

		if (SQLITE_OK != sqlite3_prepare_v2(
			index_add_context->db,
			sql, len,
			&(index_add_context->ref_insert_stmts[tuple_count - 1]),
			NULL)) {
			deen_log_error_and_exit("sqllite error preparing statement for [%s]; %s", sql, sqlite3_errmsg(index_add_context->db));
		}

		free((void *) sql);
	}

	return index_add_context->ref_insert_stmts[tuple_count - 1];
}


/*
Goes through the 'prefix_and_refs' and finds the corresponding primary keys.
The primary keys are copied into the 'prefix_ids' at the same locations.
*/

static void deen_index_find_existing_prefixes(
	deen_index_add_context *index_add_context,
	uint8_t **prefixes,
	uint32_t prefix_count,
	uint32_t *prefix_ids) {

	uint32_t i;
	sqlite3_stmt *stmt = deen_index_get_or_create_find_existing_prefixes_stmt(index_add_context, prefix_count);
	deen_bool processed_all_rows;

	for (i = 0; i < prefix_count; i++) {
		if (SQLITE_OK != sqlite3_bind_text(
			stmt,
			i + 1, // first parameter is at index 1
			(const char *) prefixes[i],
			-1,
			SQLITE_TRANSIENT // means that sql-lite won't try to free the string.
		)) {
			deen_log_error_and_exit("sqllite error setting parameter %d; %s", i + 1, sqlite3_errmsg(index_add_context->db));
		}
	}

	processed_all_rows = DEEN_FALSE;

	while (!processed_all_rows) {
		switch (sqlite3_step(stmt)) {

			case SQLITE_ROW:
			{
				uint32_t j;
				off_t row_id = (off_t) sqlite3_column_int64(stmt, 0);
				const unsigned char *row_prefix = sqlite3_column_text(stmt, 1);

				for (j=0;j<prefix_count;j++) {
					if (0 == strcmp((const char *) prefixes[j], (const char *) row_prefix)) {
						prefix_ids[j] = row_id;
					}
				}
			}
			break;

			case SQLITE_DONE:
				processed_all_rows = DEEN_TRUE;
				break;

			default:
				deen_log_error_and_exit("sqllite error getting row from; %s", sqlite3_errmsg(index_add_context->db));
				break;

		}
	}

	if (SQLITE_OK != sqlite3_reset(stmt)) {
		deen_log_error_and_exit("sqllite error resetting stmt; %s", sqlite3_errmsg(index_add_context->db));
	}

}


/*
Goes through the list of 'prefix_and_refs' and finds any that do not have a
prefix_id.  If they don't have a prefix_id then it creates one and adds it in.
*/

static void deen_index_add_missing_prefixes(
	deen_index_add_context *index_add_context,
	uint8_t **prefixes,
	uint32_t prefix_count,
	uint32_t *prefix_ids) {

	uint32_t i;

	for (i = 0;i < prefix_count; i++) {
		if (0==prefix_ids[i]) {
			
			uint32_t j;

			if (NULL == index_add_context->prefix_insert_stmt) {
				if (SQLITE_OK != sqlite3_prepare_v2(
					index_add_context->db,
					SQL_PREFIX_INSERT,
					-1,
					&(index_add_context->prefix_insert_stmt),
					NULL)
				) {
					deen_log_error_and_exit("sqllite error preparing statement for [%s]; %s", SQL_PREFIX_INSERT, sqlite3_errmsg(index_add_context->db));
				}
			}

			if (SQLITE_OK != sqlite3_bind_text(index_add_context->prefix_insert_stmt, 1, (const char *) prefixes[i], -1, SQLITE_TRANSIENT)) {
				deen_log_error_and_exit("sqllite error setting parameter in [%s]; %s", SQL_PREFIX_INSERT, sqlite3_errmsg(index_add_context->db));
			}

			if (SQLITE_DONE != sqlite3_step(index_add_context->prefix_insert_stmt)) {
				deen_log_error_and_exit("sqllite error executing insert for \"%s\" [%s]; %s", prefixes[i], SQL_PREFIX_INSERT, sqlite3_errmsg(index_add_context->db));
			}

			prefix_ids[i] = (uint32_t) sqlite3_last_insert_rowid(index_add_context->db);

			if (0 == prefix_ids[i]) {
				deen_log_error_and_exit("last insert failed to add a row from [%s]", SQL_PREFIX_INSERT);
			}

			if (SQLITE_OK != sqlite3_reset(index_add_context->prefix_insert_stmt)) {
				deen_log_error_and_exit("sqllite error resetting stmt [%s]; %s", SQL_PREFIX_INSERT, sqlite3_errmsg(index_add_context->db));
			}

			// maybe this prefix is used again further down the list?  If so,
			// then swap those in as well in order to avoid trying another
			// insert which will fail because the column must be unique.

			for (j = i;j < prefix_count; j++) {
				if (0 == strcmp((const char *) prefixes[i], (const char *) prefixes[j])) {
					prefix_ids[j] = prefix_ids[i];
				}
			}

		}
	}
}


static void deen_index_add_refs(
	deen_index_add_context *index_add_context,
	off_t ref,
	uint32_t *prefix_ids,
	uint32_t prefix_count) {

	uint32_t i;

	sqlite3_stmt *stmt = deen_index_get_or_create_ref_insert_stmt(
		index_add_context,
		prefix_count);

	for (i = 0;i<prefix_count;i++) {

		if (SQLITE_OK != sqlite3_bind_int(stmt, 1 + (2 * i), prefix_ids[i])) {
			deen_log_error_and_exit("sqllite error binding into statement for add indexes; %s", sqlite3_errmsg(index_add_context->db));
		}

		if (SQLITE_OK != sqlite3_bind_int(stmt, 2 + (2 * i), (int) ref)) {
			deen_log_error_and_exit("sqllite error binding into statement for add indexes; %s", sqlite3_errmsg(index_add_context->db));
		}

	}

	if (SQLITE_DONE != sqlite3_step(stmt)) {
		deen_log_error_and_exit("sqllite error executing add indexes; %s", sqlite3_errmsg(index_add_context->db));
	}

	if (SQLITE_OK != sqlite3_reset(stmt)) {
		deen_log_error_and_exit("sqllite error resetting stmt for add indexes; %s", sqlite3_errmsg(index_add_context->db));
	}
}


void deen_index_add(
	deen_index_add_context *index_add_context,
	off_t ref,
	uint8_t **prefixes,
	uint32_t prefix_count) {

	uint32_t *prefix_ids;

	if (0==prefix_count) {
		DEEN_LOG_INFO0("requested zero indexes added");
		return;
	}

	prefix_ids = deen_emalloc(prefix_count * sizeof(uint32_t));
	memset(prefix_ids, 0, sizeof(uint32_t) * prefix_count);

#ifdef DEBUG
	deen_millis start_ms = deen_millis_since_epoc();
#endif

	deen_index_find_existing_prefixes(index_add_context, prefixes, prefix_count, prefix_ids);

#ifdef DEBUG
	deen_millis after_find_existing_prefixes_ms = deen_millis_since_epoc();
	index_add_context->find_existing_prefixes_millis += (after_find_existing_prefixes_ms - start_ms);
#endif

	deen_index_add_missing_prefixes(index_add_context, prefixes, prefix_count, prefix_ids);

#ifdef DEBUG
	deen_millis after_add_missing_prefixes_ms = deen_millis_since_epoc();
	index_add_context->add_missing_prefixes_millis += (after_add_missing_prefixes_ms - after_find_existing_prefixes_ms);
#endif

	deen_index_add_refs(index_add_context, ref, prefix_ids, prefix_count);

#ifdef DEBUG
	deen_millis after_add_refs_ms = deen_millis_since_epoc();
	index_add_context->add_refs_millis += (after_add_refs_ms - after_add_missing_prefixes_ms);
#endif

	free(prefix_ids);

}


deen_index_lookup_result *deen_index_lookup(
	sqlite3 *db,
	uint8_t *prefix) {

	deen_bool processed_all_rows;
	sqlite3_stmt *stmt;
	uint32_t allocted_refs_count = 10;
	deen_index_lookup_result *result = (deen_index_lookup_result *) deen_emalloc(sizeof(deen_index_lookup_result));

	result->refs = (off_t *) deen_emalloc(sizeof(off_t) * allocted_refs_count);
	result->refs_count = 0;

	stmt = NULL;

	if (SQLITE_OK != sqlite3_prepare_v2(db, SQL_REF_LOOKUP, -1, &stmt, NULL)) {
		deen_log_error_and_exit("sqllite error preparing statement for [%s]; %s", SQL_REF_LOOKUP, sqlite3_errmsg(db));
	}

	if (SQLITE_OK != sqlite3_bind_text(stmt, 1, (const char *) prefix, -1, SQLITE_TRANSIENT)) {
		deen_log_error_and_exit("sqllite error setting parameter in [%s]; %s", SQL_REF_LOOKUP, sqlite3_errmsg(db));
	}

	processed_all_rows = DEEN_FALSE;

	while (!processed_all_rows) {
		switch (sqlite3_step(stmt)) {

			case SQLITE_ROW:

				if (result->refs_count >= allocted_refs_count) {
					allocted_refs_count += 10;
					result->refs = (off_t *) deen_erealloc(result->refs, sizeof(off_t) * allocted_refs_count);
				}

				result->refs[result->refs_count] = (off_t) sqlite3_column_int(stmt, 0);
				result->refs_count++;

				break;

			case SQLITE_DONE:
				processed_all_rows = DEEN_TRUE;
				break;

			default:
				deen_log_error_and_exit("sqllite error getting row from [%s]; %s", SQL_REF_LOOKUP, sqlite3_errmsg(db));
				break;

		}
	}

	if (SQLITE_OK != sqlite3_reset(stmt)) {
		deen_log_error_and_exit("sqllite error resetting stmt [%s]; %s", SQL_REF_LOOKUP, sqlite3_errmsg(db));
	}

	if (SQLITE_OK != sqlite3_finalize(stmt)) {
		deen_log_error_and_exit("sqllite error finalizing statement for [%s]; %s", SQL_REF_LOOKUP, sqlite3_errmsg(db));
	}

	return result;
}


void deen_index_lookup_result_free(deen_index_lookup_result *result) {
	if (NULL != result) {
		free((void *) result->refs);
		free((void *) result);
	}
}
