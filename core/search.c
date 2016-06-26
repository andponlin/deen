/*
 * Copyright 2016, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#include "common.h"
#include "constants.h"
#include "keyword.h"
#include "entry.h"
#include "search.h"
#include "index.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define SIZE_BUFFER_LINE_DEFAULT 196

void deen_search_free(deen_search_context *context) {
    if (-1 != context->fd_data) {
		close(context->fd_data);
	}

	if (NULL != context->db) {
		sqlite3_close_v2(context->db);
	}

	free((void *) context);
}


deen_search_context *deen_search_init(char *deen_root_dir) {

	deen_search_context *context = (deen_search_context *) deen_emalloc(sizeof(deen_search_context));

	deen_bool is_error = DEEN_FALSE;
	char *data_path = (char *) deen_emalloc(strlen(deen_root_dir) + strlen(DEEN_LEAF_DING_DATA) + 2);
	char *index_path = (char *) deen_emalloc(strlen(deen_root_dir) + strlen(DEEN_LEAF_INDEX) + 2);

	sprintf(data_path, "%s/%s", deen_root_dir, DEEN_LEAF_DING_DATA);
	sprintf(index_path, "%s/%s", deen_root_dir, DEEN_LEAF_INDEX);

	context->fd_data = open(data_path, O_RDONLY);

	if (-1 == context->fd_data) {
		is_error = DEEN_TRUE;
		DEEN_LOG_ERROR1("unable to open data file; %s", data_path);
	}

	if (SQLITE_OK != sqlite3_open_v2(index_path, &(context->db), SQLITE_OPEN_READONLY, NULL)) {
		DEEN_LOG_ERROR1("unable to open the sqllite3 database; %s", index_path);
	}

	free((void *) data_path);
	free((void *) index_path);

	if (is_error) {
		deen_search_free(context);
		return NULL;
	}

	return context;
}


/*
This function is used with quick sort to order the
references into the data.
*/

static int deen_compare_refs(const void *item1, const void *item2) {
	off_t int1 = ((off_t *)item1)[0];
	off_t int2 = ((off_t *)item2)[0];
	if (int1 == int2) return 0;
	if (int1 < int2) return -1;
	return 1;
}


/**
 * This function will find the intersection of the "refs_combined" and the
 * "refs".  It will return the new length of the "refs_combined".  The length
 * will be the same or smaller than the "refs_length" value.
 */

static int deen_search_intersect_refs(
    off_t *refs_combined,
	off_t *refs,
	size_t refs_combined_length,
	size_t refs_length) {

	size_t i;

	for (i=0;i<refs_combined_length;) {
	    if (NULL == bsearch(
		    &refs_combined[i], // what to find
		    refs,
		    refs_length,
		    sizeof(off_t),
		    &deen_compare_refs)) {
			if (i<refs_combined_length-1)
				memmove(
					&refs_combined[i],
					&refs_combined[i+1],
					sizeof(size_t) * ((refs_combined_length-i)-1)
				);
			refs_combined_length--;
		}
		else {
			i++;
		}
	}

	return refs_combined_length;
}

/**
 * This function will take the refs and will return the results, unsorted.
 */


static deen_search_result *deen_search_refs_to_result(
	deen_search_context *context,
	deen_keywords *keywords,
	off_t *refs,
	size_t refs_length) {

	deen_bool is_error = DEEN_FALSE;
	uint8_t *buffer = (uint8_t *) deen_emalloc(sizeof(uint8_t) * SIZE_BUFFER_LINE_DEFAULT);
	size_t buffer_size = SIZE_BUFFER_LINE_DEFAULT;

	deen_search_result *result = (deen_search_result *) deen_emalloc(sizeof(deen_search_result));
	result->entries = NULL;
	result->total_count = 0;
	result->entry_count = 0;

	for (size_t i=0;!is_error && i<refs_length;i++) {

	// move to the point in the file where the line starts.

		if (-1 == lseek(context->fd_data, (off_t) refs[i], SEEK_SET)) {
			DEEN_LOG_ERROR1("unable to seek in data to; %d", (int) refs[i]);
			is_error = DEEN_TRUE;
		}

	// now read the line containing the data.

		ssize_t bufferread_size = 0;
		uint8_t *newline_c = NULL;

	// read in a line of data; this should fairly quickly right-size the
	// buffer and therefore will be fairly optimal.

		do {

	// if the buffer is too small then resize it to make it
	// larger.

			if (bufferread_size == buffer_size) {
				buffer_size *= 2;
				buffer = (uint8_t *) deen_erealloc(buffer, buffer_size);
			}

			ssize_t actuallyread = read(context->fd_data,&buffer[bufferread_size],(buffer_size-bufferread_size));

			switch (actuallyread) {
				case 0:
					buffer[bufferread_size] = '\n';
					bufferread_size++;
					break;

				case -1:
					DEEN_LOG_ERROR1("an error has arisen accessing the data at; %u", refs[i]);
					is_error = DEEN_TRUE;
					break;

				default:
					bufferread_size += actuallyread;
					break;
			}
		}
		while (!is_error && NULL == (newline_c = deen_strnchr(buffer,'\n',bufferread_size)));

	// if the line starts with '#' then it is a comment and we do not
	// wish to process comments.

		if (!is_error && 0 != buffer[0] && '#' != buffer[0]) {

			newline_c[0] = 0;

	// need to make sure that all of the keywords are able to be found
	// in the line.

		    uint8_t *separator_c = (uint8_t *) strstr((const char *)buffer,"::");

		    if (NULL==separator_c) {
			    DEEN_LOG_ERROR1("corrupted line missing '::' separator at offset %d",(int) refs[i]);
		    }
		    else {

				uint8_t *german_c = buffer;
				uint8_t *english_c = &separator_c[2];

				// now remove whitespace from the end of the german data.

				{
					uint8_t *german_end_c = separator_c;

					do {
						german_end_c[0] = 0;
						german_end_c--;
					}
					while(german_end_c > german_c && isspace(german_end_c[0]));
				}

// now remove whitespace from the start of the english data.

				while(0 != english_c[0] && isspace(english_c[0])) {
					english_c++;
				}

// check that all of the keywords appear in either the english
// or the german text.

				if (deen_keywords_all_present(keywords, german_c) ||
					deen_keywords_all_present(keywords, english_c)) {

// this entry looks like a viable one so build it.

					deen_entry entry = deen_entry_create(german_c, english_c);

					if (
						(0 != entry.english_sub_count) &&
						(0 != entry.german_sub_count) ) {

						if (0==result->entry_count) {
							result->entries = (deen_entry *) deen_emalloc(sizeof(deen_entry));
						}
						else {
							result->entries = (deen_entry *) deen_erealloc(
								result->entries,
								(result->entry_count + 1) * sizeof(deen_entry)
							);
						}

						memcpy(
							&(result->entries[result->entry_count]),
							&entry,
							sizeof(deen_entry));

						DEEN_LOG_TRACE1("added entry; total now at %d",result->entry_count);

						result->total_count++;
						result->entry_count++;
					}

				}
				else {
					DEEN_LOG_TRACE2("keywords not found in; %s :: %s", german_c, english_c);
				}
			}
		}
	}

    if (is_error) {
		deen_search_result_free(result);
		return NULL;
    }

    return result;
}


static int deen_search_sort_callback(const void *a, const void *b) {
	deen_entry *entry_a = (deen_entry *) a;
	deen_entry *entry_b = (deen_entry *) b;

	// if they are the same distance from the keywords, perhaps choose the
	// less complex one first.

	if (entry_a->distance_from_keywords == entry_b->distance_from_keywords) {
		int sub_count_a = entry_a->german_sub_count;
		int sub_count_b = entry_b->german_sub_count;
		return sub_count_a - sub_count_b;

		// TODO; some more complex comparisons?
	}

	return (int) (entry_a->distance_from_keywords - entry_b->distance_from_keywords);
}


static void deen_search_sort(deen_search_result *search_result, deen_keywords *keywords) {
	if (search_result->entry_count > 0) {

		// allocated once to avoid continuously allocating memory.
		deen_bool *keyword_use_map = (deen_bool *) deen_emalloc(sizeof(deen_bool) * keywords->count);

		for (uint32_t i=0;i<search_result->entry_count;i++) {
			search_result->entries[i].distance_from_keywords = deen_entry_calculate_distance_from_keywords(
				&(search_result->entries[i]), keywords, keyword_use_map);
		}

		qsort(
			search_result->entries, search_result->entry_count,
			sizeof(deen_entry), deen_search_sort_callback);

	}
}


static void deen_search_crop(deen_search_result *search_result, size_t max_result_count) {
	if (max_result_count < search_result->entry_count) {

		for (uint32_t i=max_result_count;i<search_result->entry_count;i++) {
			deen_entry_free(&search_result->entries[i]);
		}

		search_result->entries = (deen_entry *) deen_erealloc(
			search_result->entries,
			sizeof(deen_entry) * max_result_count);

		search_result->entry_count = max_result_count;
	}
}


deen_search_result *deen_search(
	deen_search_context *context,
	deen_keywords *keywords,
	size_t max_result_count) {

	size_t keywords_longest_len = deen_keywords_longest_keyword(keywords);
	uint8_t *keyword_prefix_buffer = (uint8_t *) deen_emalloc(sizeof(uint8_t) * keywords_longest_len);

	off_t *refs_combined = NULL;
	size_t refs_combined_length = 0;

	for (uint32_t i=0;i<keywords->count;i++) {

		// copy the keyword into a buffer and then cut it off
		// to make the prefix to search on.

		size_t keyword_len = strlen((char *) keywords->keywords[i]);
		memcpy(keyword_prefix_buffer, keywords->keywords[i], keyword_len);
		deen_utf8_crop_to_unicode_len(keyword_prefix_buffer, keyword_len, DEEN_INDEXING_DEPTH);

// now actually find all of the references for the keyword and
// then we can put the references into a set.  For the first
// keyword, all of the references are added to the set and for
// the remaining keywords, references which were in the set are
// removed if they are not found for the keyword; essentially an
// intersection of all of the refs for all of the keywords
// supplied.

		deen_index_lookup_result *lookup_result = deen_index_lookup(
			context->db,
			keyword_prefix_buffer);

		qsort(
			lookup_result->refs,
			lookup_result->refs_count,
			sizeof(off_t),deen_compare_refs);

		if (NULL==refs_combined) {
			refs_combined = (off_t *) deen_emalloc(sizeof(off_t) * lookup_result->refs_count);
			refs_combined_length = lookup_result->refs_count;
			memcpy(refs_combined,lookup_result->refs,sizeof(off_t) * lookup_result->refs_count);
		}
		else {
			refs_combined_length = deen_search_intersect_refs(
				refs_combined,
				lookup_result->refs,
				refs_combined_length,
				lookup_result->refs_count);
		}

		deen_index_lookup_result_free(lookup_result);
	}

	free((void *) keyword_prefix_buffer);

	// now take the references and load-up those lines that are
	// at those references.  Then check that, for each line that
	// is loaded, all of the supplied keywords can be found on
	// that line.

	for (int i=0;i<refs_combined_length;i++) {
		DEEN_LOG_TRACE1("ref; %d", (int) refs_combined[i]);
	}

	deen_search_result *search_result = deen_search_refs_to_result(
		context, keywords,
		refs_combined,
		refs_combined_length);

	deen_search_sort(search_result, keywords);
	deen_search_crop(search_result, max_result_count);

	return search_result;
}

void deen_search_result_free(deen_search_result *result) {
	if (NULL != result) {
		for (uint32_t i=0;i<result->entry_count;i++) {
			deen_entry_free(&(result->entries[i]));
		}

		free((void *) result);
    }
}
