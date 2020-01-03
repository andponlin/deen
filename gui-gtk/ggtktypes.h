/*
 * Copyright 2019-2020, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#ifndef __DEEN_GGTK_COMMON_H
#define __DEEN_GGTK_COMMON_H

#include <pthread.h>

#include "core/install.h"

#define STACK_CHILD_NAME_INSTALL_FILE_CHOICE "stack_install_file_choice"
#define STACK_CHILD_NAME_INSTALL_PROGRESS "stack_install_progress"
#define STACK_CHILD_NAME_SEARCH "stack_search"


typedef struct deen_ggtk_widgets deen_ggtk_widgets;
struct deen_ggtk_widgets {
	GtkWindow *window_main;
	GtkWidget *stack_main;

	GtkWidget *file_chooser_start_install_raw_input;
	GtkWidget *button_start_install_go;

	GtkWidget *progress_bar_install;
	GtkWidget *button_install_cancel;

	GtkWidget *entry_search_keywords;
	GtkWidget *button_search;
	GtkWidget *text_view_results;
	GtkWidget *label_results_notes;
	GtkWidget *button_results_show_all;
};

typedef struct deen_ggtk_search deen_ggtk_search;
struct deen_ggtk_search {
	GtkTextBuffer *text_buffer;
	deen_search_context *context;
	PangoTabArray *tab_array;
	GtkTextTag *tag_foreground_grammar;
	GtkTextTag *tag_foreground_context;
	GtkTextTag *tag_foreground_layout;
	GtkTextTag *tag_hanging_indent_and_tab_stops;
	GtkTextTag *tag_background_keyword_hit;
};

typedef struct deen_ggtk_install deen_ggtk_install;
struct deen_ggtk_install {
	GFile *file_raw_input;
	enum deen_install_state state;
	float progress;
	uint8_t cancel; // boolean
};

typedef struct deen_ggtk_state deen_ggtk_state;
struct deen_ggtk_state {
	deen_ggtk_widgets *widgets;
	deen_ggtk_install *install;
	deen_ggtk_search *search;
	pthread_mutex_t lock;
};

#endif // __DEEN_GGTK_COMMON_H