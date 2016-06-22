/*
 * Copyright 2016, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */
 
#ifndef __TYPES_H
#define __TYPES_H

#include "constants.h"

#include <sys/types.h>

#include <sqlite3.h>

/*
This provides a type for a boolean (actually a byte).
*/

typedef uint8_t deen_bool;

/*
When seeing how long a UTF-8 sequence is, the function will return
this type in order to indicate the result.
*/

typedef enum deen_utf8_sequence_result deen_utf8_sequence_result;
enum deen_utf8_sequence_result {
	DEEN_SEQUENCE_OK, // sequence was OK
	DEEN_BAD_SEQUENCE, // corrupt UTF-8 sequence
	DEEN_INCOMPLETE_SEQUENCE // UTF-8 sequence ran out of characters to consume
};

/*
This is used to identify the first found keyword from a list of
keywords within a string.  It is returned as the result of a
function call.
*/

typedef struct deen_first_keyword deen_first_keyword;
struct deen_first_keyword
{
	uint8_t *keyword;
	size_t offset;
};


enum deen_entry_atom_type {
    ATOM_TEXT,
    ATOM_GRAMMAR,
    ATOM_CONTEXT
};


typedef struct deen_entry_atom deen_entry_atom;
struct deen_entry_atom {
    enum deen_entry_atom_type type;
    uint8_t *text;
};


typedef struct deen_entry_sub_sub deen_entry_sub_sub;
struct deen_entry_sub_sub {
    deen_entry_atom *atoms;
    uint32_t atom_count;
};


typedef struct deen_entry_sub deen_entry_sub;
struct deen_entry_sub {
    deen_entry_sub_sub *sub_subs;
    uint32_t sub_sub_count;
};


typedef struct deen_entry deen_entry;
struct deen_entry {
    deen_entry_sub *german_subs;
    deen_entry_sub *english_subs;
    uint32_t english_sub_count;
    uint32_t german_sub_count;
	uint32_t distance_from_keywords;
};


typedef struct deen_search_result deen_search_result;
struct deen_search_result {
    uint32_t total_count;
    uint32_t entry_count;
    deen_entry *entries;
};


typedef struct deen_search_context deen_search_context;
struct deen_search_context {
    sqlite3 *db;
    int fd_data;
};


/*
This struct maintains state around the database connection as well as any
statements that can be re-used as part of the indexing process.  This
avoids repeated parsing the SQL statements.
*/

typedef struct deen_index_add_context deen_index_add_context;
struct deen_index_add_context {

	sqlite3 *db;

	sqlite3_stmt *prefix_insert_stmt;
	sqlite3_stmt *ref_insert_stmt;

	sqlite3_stmt **find_existing_prefixes_stmts;
	size_t find_existing_prefixes_stmts_count;

};


typedef struct deen_keywords deen_keywords;
struct deen_keywords
{
	uint32_t count;
	uint8_t **keywords;
};


typedef struct deen_index_lookup_result deen_index_lookup_result;
struct deen_index_lookup_result {
	off_t *refs;
	uint32_t refs_count;
};


#endif /* __TYPES_H */
