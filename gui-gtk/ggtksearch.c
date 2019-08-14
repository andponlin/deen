/*
 * Copyright 2019, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#include "ggtksearch.h"

#include <string.h>

#include "core/keyword.h"
#include "core/search.h"
#include "core/types.h"
#include "ggtkgeneral.h"
#include "ggtktypes.h"
#include "ggtkrendertextbuffer.h"


deen_search_context *deen_ggtk_ensure_search_context() {
	if (NULL == deen_ggtk_state_global->search->context) {
		char *root_dir = deen_root_dir();

		deen_ggtk_state_global->search->context = deen_search_init(root_dir);

		if (NULL == deen_ggtk_state_global->search->context) {
			deen_log_error_and_exit("unable to create a search context");
		}

		free(root_dir);
	}

	return deen_ggtk_state_global->search->context;
}

void deen_ggtk_set_results_notes(deen_search_result *result) {
	char notes_assembly_buffer[1024];

	snprintf(
		notes_assembly_buffer, 1024,
		"Showing %d of %d", result->entry_count, result->total_count);

	gtk_label_set_text(
		GTK_LABEL(deen_ggtk_state_global->widgets->label_results_notes),
		notes_assembly_buffer);
}

void deen_ggtk_update_button_results_show_all(deen_search_result *result) {
	if (NULL == result || (result->entry_count >= result->total_count)) {
		gtk_widget_set_sensitive(
			deen_ggtk_state_global->widgets->button_results_show_all, FALSE);
	} else {
		gtk_widget_set_sensitive(
			deen_ggtk_state_global->widgets->button_results_show_all, TRUE);
	}
}


void deen_ggtk_search_by_provided_keywords(size_t max_result_count) {
	deen_search_context *context = deen_ggtk_ensure_search_context();
	const gchar *search_expression = gtk_entry_get_text(
		GTK_ENTRY(deen_ggtk_state_global->widgets->entry_search_keywords));
	deen_search_result *result;
	deen_keywords *keywords = deen_keywords_create();
	size_t search_expression_len = strlen((char *) search_expression);
	uint8_t *search_expression_upper = (uint8_t *) deen_emalloc(
		sizeof(uint8_t) * (search_expression_len + 1));

	search_expression_upper[search_expression_len] = 0;
	memcpy(
		search_expression_upper,
		search_expression,
		search_expression_len);

	deen_to_upper(search_expression_upper);

	DEEN_LOG_TRACE2("keywords; [%s] --> [%s]", search_expression, search_expression_upper);
	deen_keywords_add_from_string(keywords, search_expression_upper);

	// dump out the keywords for now
	deen_trace_log_keywords(keywords);

	result = deen_search(context, keywords, max_result_count);

	if(0 == result->total_count) {
		if(deen_keywords_adjust(keywords)) {
			DEEN_LOG_INFO0("no results found -> did adjust keywords");
			deen_trace_log_keywords(keywords);
			deen_search_result_free(result);
			result = deen_search(context, keywords, max_result_count);
		}
	}

	gtk_text_buffer_set_text(
		deen_ggtk_state_global->search->text_buffer, "", 0);

	deen_ggtk_render_textbuffer(
		deen_ggtk_state_global->search->text_buffer,
		result,
		keywords);
	deen_ggtk_set_results_notes(result);
	deen_ggtk_update_button_results_show_all(result);

	deen_search_result_free(result);
	deen_keywords_free(keywords);

	free((void *) search_expression_upper);
}

/*
This gets hit when the user presses the return key while editing the keywords.
*/

void on_entry_search_keywords_activate(GtkEntry *entry) {
	deen_ggtk_search_by_provided_keywords(DEEN_RESULT_SIZE_DEFAULT);
}

void on_button_results_show_all_clicked() {
	deen_ggtk_search_by_provided_keywords(DEEN_RESULT_SIZE_MAX);
}