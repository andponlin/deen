/*
 * Copyright 2019, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#include "ggtkgeneral.h"

#include <assert.h>
#include <gtk/gtk.h>
#include <stdlib.h>

#include "ggtkconstants.h"
#include "ggtkinstall.h"
#include "core/search.h"

// ------------------------------------------------
// STATE
// ------------------------------------------------

deen_ggtk_state *deen_ggtk_state_global;

deen_ggtk_state *deen_ggtk_state_create() {
	char *root_dir = deen_root_dir();

	deen_ggtk_state *result = (deen_ggtk_state *) deen_emalloc(sizeof(deen_ggtk_state));
	bzero(result, sizeof(deen_ggtk_state));

	result->install = (deen_ggtk_install *) deen_emalloc(sizeof(deen_ggtk_install));
	bzero(result->install, sizeof(deen_ggtk_install));

	result->widgets = (deen_ggtk_widgets *) deen_emalloc(sizeof(deen_ggtk_widgets));
	bzero(result->widgets, sizeof(deen_ggtk_widgets));

	result->search = (deen_ggtk_search *) deen_emalloc(sizeof(deen_ggtk_search));
	bzero(result->search, sizeof(deen_ggtk_search));

	result->install->state = DEEN_INSTALL_STATE_IDLE;
	result->install->progress = 0;
	pthread_mutex_init(&(result->lock), NULL);

	if (deen_is_installed(root_dir)) {
		result->install->state = DEEN_INSTALL_STATE_COMPLETED;
	}

	result->search->tab_array = pango_tab_array_new_with_positions(
		1, // count of tab-stops
		TRUE,
		PANGO_TAB_LEFT,
		DEEN_GGTK_ENTRY_TAB);

	result->search->text_buffer = gtk_text_buffer_new(NULL);

	result->search->tag_foreground_grammar = gtk_text_buffer_create_tag(
		result->search->text_buffer, "foreground_grammar",
			"foreground", "royal blue",
			"style", PANGO_STYLE_ITALIC,
			NULL);
	result->search->tag_foreground_context = gtk_text_buffer_create_tag(
		result->search->text_buffer, "foreground_context",
			"foreground", "orchid",
			"style", PANGO_STYLE_ITALIC,
			NULL);
	result->search->tag_foreground_layout = gtk_text_buffer_create_tag(
		result->search->text_buffer, "foreground_layout", "foreground", "grey", NULL);
	result->search->tag_background_keyword_hit = gtk_text_buffer_create_tag(
		result->search->text_buffer, "background_keyword_hit", "background", "orange", NULL);
	result->search->tag_hanging_indent_and_tab_stops = gtk_text_buffer_create_tag(
		result->search->text_buffer, "hanging_indent_and_tab_stops",
		"indent", -DEEN_GGTK_ENTRY_TAB,
		"tabs", result->search->tab_array,
		NULL);

	free(root_dir);

	return result;
}

void deen_ggtk_state_free(deen_ggtk_state *value) {
	if (NULL != value->install->file_raw_input) {
		g_clear_object(&(value->install->file_raw_input));
	}

	g_clear_object(&value->search->text_buffer);

	if (NULL != value->search->text_buffer) {
		g_object_unref(value->search->text_buffer);
	}

	if (NULL != value->search->tab_array) {
		pango_tab_array_free(value->search->tab_array);
	}

	if (NULL != value->search->context) {
		deen_search_free(value->search->context);
	}

	free(value->install);
	free(value->widgets);
	free(value->search);
	free(value);
}

// ------------------------------------------------
// GUI
// ------------------------------------------------

/*
Loads up the icon from the resources and sets that as the icon for the window.
*/

void deen_ggtk_setup_icon() {
	GdkPixbuf *icon = gdk_pixbuf_new_from_resource(DEEN_GGTK_RESOURCE_ICON, NULL);
	assert(NULL != icon);
	gtk_window_set_default_icon(icon);
}

/*
This will get invoked on the main thread when the main thread's event loop is
idle.
*/

gboolean deen_ggtk_main_stack_update_visible_ui_on_idle(void *context) {
	deen_ggtk_main_stack_update_visible_ui();
	return FALSE; // don't run again.
}

/*
The application has a main 'stack' which contains a number of different views;
choosing a file, indexing and then searching.  Based on the state of the
application, this function will choose the most appropriate view to display in
the stack.
*/

void deen_ggtk_main_stack_update_visible_ui() {
	assert(NULL != deen_ggtk_state_global->widgets->stack_main);

	char *child_name;
	switch (deen_ggtk_state_global->install->state) {
		case DEEN_INSTALL_STATE_IDLE:
			child_name = STACK_CHILD_NAME_INSTALL_FILE_CHOICE;
			break;
		case DEEN_INSTALL_STATE_COMPLETED:
			child_name = STACK_CHILD_NAME_SEARCH;
			break;
		default:
			child_name = STACK_CHILD_NAME_INSTALL_PROGRESS;
			break;
	}

	gtk_stack_set_visible_child_name(
		GTK_STACK(deen_ggtk_state_global->widgets->stack_main),
		child_name);
}

void deen_ggtk_update_ui() {
	deen_ggtk_main_stack_update_visible_ui();
	deen_ggtk_install_file_choice_update_ui();
	deen_ggtk_install_progress_update_ui();
}

#define DEEN_GGTK_GET_WIDGET(N) GTK_WIDGET(gtk_builder_get_object(builder, N));

void deen_ggtk_setup_global_from_builder(GtkBuilder *builder) {
	deen_ggtk_state_global->widgets->window_main = GTK_WINDOW(gtk_builder_get_object(builder, "window_main"));
	deen_ggtk_state_global->widgets->stack_main = DEEN_GGTK_GET_WIDGET("stack_main");
	deen_ggtk_state_global->widgets->file_chooser_start_install_raw_input = DEEN_GGTK_GET_WIDGET("file_chooser_start_install_raw_input");
	deen_ggtk_state_global->widgets->button_start_install_go = DEEN_GGTK_GET_WIDGET("button_start_install_go");
	deen_ggtk_state_global->widgets->progress_bar_install = DEEN_GGTK_GET_WIDGET("progress_bar_install");
	deen_ggtk_state_global->widgets->button_install_cancel = DEEN_GGTK_GET_WIDGET("button_install_cancel");
	deen_ggtk_state_global->widgets->entry_search_keywords = DEEN_GGTK_GET_WIDGET("entry_search_keywords");
	deen_ggtk_state_global->widgets->text_view_results = DEEN_GGTK_GET_WIDGET("text_view_results");
	deen_ggtk_state_global->widgets->label_results_notes = DEEN_GGTK_GET_WIDGET("label_results_notes");
	deen_ggtk_state_global->widgets->button_results_show_all = DEEN_GGTK_GET_WIDGET("button_results_show_all");

	gtk_text_view_set_buffer(
		GTK_TEXT_VIEW(deen_ggtk_state_global->widgets->text_view_results),
		deen_ggtk_state_global->search->text_buffer);

	gtk_label_set_text(
		GTK_LABEL(deen_ggtk_state_global->widgets->label_results_notes),
		DEEN_VERSION);
}

// ------------------------------------------------
// GTK TYPE HANDLING
// ------------------------------------------------

gboolean deen_ggtk_gboolean_from_deen_bool(deen_bool b) {
	if (DEEN_TRUE == b) {
		return TRUE;
	}
	return FALSE;
}

// ------------------------------------------------
