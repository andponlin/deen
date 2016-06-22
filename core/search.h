/*
 * Copyright 2016, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#ifndef __SEARCH_H
#define __SEARCH_H

#include "common.h"

/**
 * This function will return the search context.  If the context was not able
 * to be created then it will return NULL and the log will have displayed what
 * the problem was.
 */

deen_search_context *deen_search_init(char *deen_root_dir);


void deen_search_free(deen_search_context *context);


deen_search_result *deen_search(
	deen_search_context *context,
	deen_keywords *keywords,
	size_t max_result_count);


void deen_search_result_free(deen_search_result *result);


#endif /* __SEARCH_H */
