/*
 * Copyright 2016, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */
 
#ifndef __CONSTANTS_H
#define __CONSTANTS_H

#include <stdint.h>

/*
This set of constants define the prefix used to log at different levels.
*/

#define DEEN_PREFIX_TRACE "~"
#define DEEN_PREFIX_INFO "-"
#define DEEN_PREFIX_ERROR "!"
#define DEEN_PREFIX_ERROREXIT "!x"

/*
This is used for testing purposes to be able to
see what happens when there is a problem in the
data installation process.
*/

#define DEEN_CAUSE_ERROR_IN_INSTALL DEEN_FALSE

/*
The indexing uses a "prefix tree" approach so that
the start of words are indexed.  This value sets
the number of characters into the word that are
pushed into the tree.
*/

#define DEEN_INDEXING_DEPTH 4

/*
A word must have at least this many characters to be
worth indexing.
*/

#define DEEN_INDEXING_MIN 3

// This constant controls how many results to show by default.

#define DEEN_RESULT_SIZE_DEFAULT 10

/**
 * This constant is used when establishing the distance that a word is
 * from the keywords.  This value really means that the entry does not
 * contain the keywords.
 */

#define DEEN_MAX_SORT_DISTANCE_FROM_KEYWORDS UINT32_MAX

#define DEEN_NOT_FOUND SIZE_MAX

#define DEEN_LEAF_INDEX "deen.idx.sqllite3"
#define DEEN_LEAF_DING_DATA "de-en.txt"

#define DIR_DEEN ".deen"

/*
 This constant is a leafname for a temporary file that gets produced first when
 an index is made.  The index is then optimized into the right place.
 */

#define DEEN_LEAF_TMPINDEX "deen_idx_tmp.XXXXXX"

#define DEEN_TRUE 1
#define DEEN_FALSE 0

#endif /* __CONSTANTS_H */
