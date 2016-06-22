/*
 * Copyright 2016, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */
 
#ifndef __INDEX_H
#define __INDEX_H

#include "types.h"

#include <stdint.h>
#include <sqlite3.h>

/*
This function will populate the table structures into the database.
*/

void deen_index_init(sqlite3 *db);

/*
Creates a context that can then be used with indexing functions.  The context
is used to cache statements such that they can be re-used instead of being
continuously parsed from SQL.
*/

deen_index_add_context *deen_index_add_context_create(sqlite3 *db);

/*
Frees a context that was created earlier.  Note that the database that was
supplied on creation is *NOT* released.  The caller is expected to handle
this aspect themselves.
*/

void deen_index_add_context_free(deen_index_add_context *context);

/*
This function will load the reference into the prefixes specified.  This
assumes that no prior call was made with the same reference.
*/

void deen_index_add(
	deen_index_add_context *index_add_context,
	off_t ref,
	uint8_t **prefixes,
	uint32_t prefix_count);

/*
This function will lookup the prefix to resolve it into some references.
The result is dynamically allocated and must be freed by the caller.
*/

deen_index_lookup_result *deen_index_lookup(
	sqlite3 *db,
	uint8_t *prefix);

void deen_index_lookup_result_free(deen_index_lookup_result *result);

#endif /* __INDEX_H */
