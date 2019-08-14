/*
 * Copyright 2019, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#include <assert.h>
#include <errno.h>
#include <gtk/gtk.h>

#include "core/common.h"
#include "core/install.h"
#include "ggtkgeneral.h"
#include "ggtktypes.h"

// ------------------------------------------------
// DECLARATIONS
// ------------------------------------------------

gboolean deen_ggtk_install_error_report_on_idle(void *context);
gboolean deen_ggtk_install_progress_update_ui_on_idle(void *context);

// ------------------------------------------------
// CANCEL THE INSTALLATION
// ------------------------------------------------

/*
This function will perform the cancellation of the installation.
*/

void deen_ggtk_install_cancel() {
	pthread_mutex_lock(&deen_ggtk_state_global->lock);
	deen_ggtk_state_global->install->cancel = DEEN_TRUE;
	pthread_mutex_unlock(&deen_ggtk_state_global->lock);
}

deen_bool deen_ggtk_install_is_cancelled() {
	deen_bool result;
	pthread_mutex_lock(&deen_ggtk_state_global->lock);
	result = deen_ggtk_state_global->install->cancel;
	pthread_mutex_unlock(&deen_ggtk_state_global->lock);
	return result;
}

/*
This callback function is invoked from the indexing process to check to see if
the indexing process should be cancelled or not.
*/

deen_bool deen_ggtk_is_cancelled_cb(void *context) {
	return deen_ggtk_install_is_cancelled();
}

// ------------------------------------------------
// UPDATING THE UI
// ------------------------------------------------

/*
 In the GTK Switch widget are a number of different 'pages' to choose from.  One
 of those is the one for file-choice.  This function will update these UI
 elements based on the state stored in the global data.
*/

void deen_ggtk_install_file_choice_update_ui() {
	if (NULL == deen_ggtk_state_global->install->file_raw_input) {
		gtk_widget_set_sensitive(
			deen_ggtk_state_global->widgets->button_start_install_go, FALSE);
	} else {
		gtk_widget_set_sensitive(
			deen_ggtk_state_global->widgets->button_start_install_go, TRUE);
	}
}

/*
After the progress update is stored in the global state, the UI needs to be
updated to reflect this change in state.  This function will take the data from
the state and will show it in the widgets of the UI.
*/

void deen_ggtk_install_progress_update_ui() {
	gtk_progress_bar_set_fraction(
		GTK_PROGRESS_BAR(deen_ggtk_state_global->widgets->progress_bar_install),
		deen_ggtk_state_global->install->progress);

	pthread_mutex_lock(&deen_ggtk_state_global->lock);
    gboolean cancel_is_sensitive = !deen_ggtk_gboolean_from_deen_bool(
        deen_ggtk_state_global->install->cancel);
    pthread_mutex_unlock(&deen_ggtk_state_global->lock);

    gtk_widget_set_sensitive(
        deen_ggtk_state_global->widgets->button_install_cancel, cancel_is_sensitive);
}

// ------------------------------------------------
// INSTALLATION PROCESSING
// ------------------------------------------------

/*
This function will check to ensure that the selected file looks like a DING file
and if not, it will warn the user that the file is unlikely to be correct.
*/

deen_bool deen_ggtk_check_input_file_ding_format() {
	assert(NULL != deen_ggtk_state_global->install->file_raw_input);
	char *path = g_file_get_path(deen_ggtk_state_global->install->file_raw_input);
	char *msg;

	switch (deen_install_check_for_ding_format(path)) {
		case DEEN_INSTALL_CHECK_OK:
			return DEEN_TRUE;
		case DEEN_INSTALL_CHECK_IS_COMPRESSED:
			msg = "The input file appears to be compressed.  Decompress the "
				"input file before using it.";
			break;
		case DEEN_INSTALL_CHECK_IO_PROBLEM:
			msg = "The input file was unable to be read.";
			break;
		case DEEN_INSTALL_CHECK_TOO_SMALL:
			msg = "The input file is too small to be used.";
			break;
		case DEEN_INSTALL_CHECK_BAD_FORMAT:
			msg = "The input file appears to be in the wrong format.  Check "
				"that the file is a DING data file.";
			break;
		default:
			msg = "An unexpected issue has arisen with the input file";
			break;
	}

	GtkWidget *dialog;
	GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
    dialog = gtk_message_dialog_new(
        deen_ggtk_state_global->widgets->window_main,
		flags,
		GTK_MESSAGE_ERROR,
		GTK_BUTTONS_CLOSE,
		"%s", msg);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy (dialog);
	g_free(path);

	return DEEN_FALSE;
}

/*
This function is called from the indexing process to indicate how progress is
going with the indexing.
*/

deen_bool deen_ggtk_install_progress_cb(
	void *context,
	enum deen_install_state state,
	float progress)
{
	deen_log_install_progress(state, progress);

	enum deen_install_state effective_state = state;

	deen_bool should_update_widget_stack = DEEN_FALSE;

	pthread_mutex_lock(&deen_ggtk_state_global->lock);

	// if there was an error then the error should be reported and then the
	// system should switch to being in idle state.

	if (DEEN_INSTALL_STATE_ERROR == effective_state) {
		if (!deen_ggtk_state_global->install->cancel) {
			g_idle_add(deen_ggtk_install_error_report_on_idle, NULL);
		}
		effective_state = DEEN_INSTALL_STATE_IDLE;
		progress = 0;
	}

	// if there is a difference between the install state and what is currently
	// recorded then it is necessary to check that the right view is being
	// displayed in the UI.

	if (effective_state != deen_ggtk_state_global->install->state) {
		should_update_widget_stack = DEEN_TRUE;
	}

	deen_ggtk_state_global->install->state = effective_state;
	deen_ggtk_state_global->install->progress = progress;

	if (should_update_widget_stack) {
		g_idle_add(deen_ggtk_main_stack_update_visible_ui_on_idle, NULL);
	}

	g_idle_add(deen_ggtk_install_progress_update_ui_on_idle, NULL);

	pthread_mutex_unlock(&deen_ggtk_state_global->lock);

	return DEEN_TRUE;
}

/*
This is the pthreads starter for the process of performing the installation.
*/

void *deen_ggtk_run_install_thread(void *data) {
	assert(NULL != deen_ggtk_state_global->install->file_raw_input);

	char *root_dir = deen_root_dir();
	char *filename = g_file_get_path(deen_ggtk_state_global->install->file_raw_input);

	deen_install_from_path(
		root_dir,
		filename,
		NULL,
		deen_ggtk_install_progress_cb,
		deen_ggtk_is_cancelled_cb);

	free(root_dir);
	g_free(filename);

	return NULL;
}

/*
This method is invoked from the main thread in order to get the process of
installing started.
*/

gboolean deen_ggtk_initiate_install_thread() {

	// it could be that the process was previously cancelled.  For a fresh run,
	// it is necessary to reset the cancel to false or else the installation
	// will be pre-cancelled.

	pthread_mutex_lock(&deen_ggtk_state_global->lock);
	deen_ggtk_state_global->install->cancel = DEEN_FALSE;
	deen_ggtk_state_global->install->progress = 0;
	pthread_mutex_unlock(&deen_ggtk_state_global->lock);

	pthread_t thread;

	if (0 == pthread_create(&thread, NULL, deen_ggtk_run_install_thread, NULL)) {
		DEEN_LOG_INFO0("started thread for installing");
		return TRUE;
	}

	DEEN_LOG_ERROR1("unable to start the thread for installing: %s",
		strerror(errno));

	return FALSE;
}

// ------------------------------------------------
// HANDLING UI INTERACTION
// ------------------------------------------------

/*
This is triggered when the user has, using the UI, chosen a new file.  The file
should be obtained from the UI and stored in the application's state.
*/

void deen_ggtk_set_raw_input_file() {
	if (NULL != deen_ggtk_state_global->install->file_raw_input) {
		g_clear_object(&(deen_ggtk_state_global->install->file_raw_input));
	}

	deen_ggtk_state_global->install->file_raw_input = gtk_file_chooser_get_file(
		GTK_FILE_CHOOSER(deen_ggtk_state_global->widgets->file_chooser_start_install_raw_input));

	if (NULL == deen_ggtk_state_global->install->file_raw_input) {
		DEEN_LOG_INFO0("unset the raw input file");
	} else {
		DEEN_LOG_INFO0("set the raw input file");
	}

	deen_ggtk_install_file_choice_update_ui();
}

// gtk signal function
void on_button_start_install_go_clicked() {
	if (DEEN_TRUE == deen_ggtk_check_input_file_ding_format()) {
		if (TRUE != deen_ggtk_initiate_install_thread()) {
			DEEN_LOG_ERROR0("! a problem has arisen initiating the indexing");
		}
	}
}

// gtk signal function
void on_file_chooser_start_install_raw_input_file_set() {
	deen_ggtk_set_raw_input_file();
}

// gtk signal function
void on_button_install_cancel_clicked() {
	deen_ggtk_install_cancel();
}

gboolean deen_ggtk_install_error_report_on_idle(void *context) {
	GtkWidget *dialog;
	GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
	dialog = gtk_message_dialog_new(
		deen_ggtk_state_global->widgets->window_main,
		flags,
		GTK_MESSAGE_ERROR,
		GTK_BUTTONS_CLOSE,
		"An unexpected error has occurred during indexing.  Check the logs for details.");

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy (dialog);

	return FALSE; // don't run again
}

/*
This is invoked from the background thread onto the main thread.
*/

gboolean deen_ggtk_install_progress_update_ui_on_idle(void *context) {
	deen_ggtk_install_progress_update_ui();
	return FALSE; // don't run again
}

// ---------------------------------------------------------------

