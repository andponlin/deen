/*
 * Copyright 2019, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#include <gtk/gtk.h>
#include <libgen.h>
#include <unistd.h>

#include "ggtkconstants.h"
#include "ggtkgeneral.h"
#include "ggtktypes.h"
#include "ggtkresources.h"

typedef struct deen_ggtk_args deen_ggtk_args;
struct deen_ggtk_args {
	deen_bool version;
	deen_bool trace_enabled;
};

static void deen_ggtk_syntax(char *binary_name) {
	char *binary_name_basename = basename(binary_name);
	printf("version %s\n",DEEN_VERSION);
	printf("%s [-h]\n", binary_name_basename);
	printf("%s [-v] [-t]\n", binary_name_basename);
	exit(1);
}

static void deen_ggtk_init_args(deen_ggtk_args *args) {
	args->version = DEEN_FALSE;
	args->trace_enabled = DEEN_FALSE;
}

static void deen_ggtk_capture_args(deen_ggtk_args *args, int argc, char** argv) {
	int i;

	for (i = 1; i < argc; i++) {
		if ('-' == argv[i][0]) {

			if (2 != strlen(argv[i])) {
				deen_log_error_and_exit("unrecognized switch [%s]", argv[i]);
			}

			switch (argv[i][1]) {
				case 'h':
					deen_ggtk_syntax(argv[0]);
					break;

				case 'v':
					args->version = DEEN_TRUE;
					break;
				case 't':
					args->trace_enabled = DEEN_TRUE;
					break;
				default:
					deen_log_error_and_exit("unrecognized switch [%s]", argv[i]);
					break;
			}
		} else {
			deen_log_error_and_exit("unexpected command line argument [%s]", argv[i]);
		}
	}
}

int main(int argc, char *argv[]) {

	deen_ggtk_args args;
	deen_ggtk_init_args(&args);
	deen_ggtk_capture_args(&args, argc, argv);

	if (DEEN_TRUE == args.version) {
		printf("%s\n", DEEN_VERSION);
	}

	if (DEEN_TRUE == args.trace_enabled) {
		deen_set_trace_enabled(DEEN_TRUE);
	}

	GtkBuilder *builder;
	gtk_init(&argc, &argv);
	deen_ggtk_register_resource();
	deen_ggtk_setup_icon();

	deen_ggtk_state_global = deen_ggtk_state_create();

	builder = gtk_builder_new();
	if (0 == gtk_builder_add_from_resource(builder, DEEN_GGTK_RESOURCE_GLADE_MAIN, NULL)) {
		deen_log_error_and_exit("unable to load the glade resource [%s]", DEEN_GGTK_RESOURCE_GLADE_MAIN);
	}

	deen_ggtk_setup_global_from_builder(builder);
	gtk_builder_connect_signals(builder, NULL);

	g_object_unref(builder);

	deen_ggtk_update_ui();

	gtk_widget_show(GTK_WIDGET(deen_ggtk_state_global->widgets->window_main));
	gtk_main();

	deen_ggtk_state_free(deen_ggtk_state_global);
	deen_ggtk_unregister_resource();

	return 0;
}

deen_bool deen_ggtk_query_cancel_indexing_on_quit() {
	deen_bool result = DEEN_FALSE;
	GtkWidget *dialog;
	GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
    dialog = gtk_message_dialog_new(
        deen_ggtk_state_global->widgets->window_main,
		flags,
		GTK_MESSAGE_ERROR,
		GTK_BUTTONS_YES_NO,
		"The application is currently indexing data.  Would you like to cancel the indexing and quit?");

	if (GTK_RESPONSE_YES == gtk_dialog_run(GTK_DIALOG(dialog))) {
		result = DEEN_TRUE;
	}

	gtk_widget_destroy (dialog);

	return result;
}

deen_bool deen_ggtk_is_indexing() {
	switch (deen_ggtk_state_global->install->state) {
		case DEEN_INSTALL_STATE_STARTING:
		case DEEN_INSTALL_STATE_INDEXING:
			return DEEN_TRUE;
		default:
			return DEEN_FALSE;
	}
}

/*
Checks to see if the system is currently indexing.  It will perform the
necessary locking to ensure that the check is thread-safe.
*/

deen_bool deen_ggtk_thread_safe_is_indexing() {
	deen_bool result;
	pthread_mutex_lock(&deen_ggtk_state_global->lock);
	result = deen_ggtk_is_indexing();
	pthread_mutex_unlock(&deen_ggtk_state_global->lock);
	return result;
}

/*
 Returns true if the quit should proceed.  This may check to see if the
 installation process is in progress; if so then it should warn the user that
 this is the case and ask them if they really want to quit or not.
*/

deen_bool deen_ggtk_quit_requested_should_proceed() {
	deen_bool should_quit = DEEN_TRUE;

	pthread_mutex_lock(&deen_ggtk_state_global->lock);

	if (deen_ggtk_is_indexing()) {
		if (!deen_ggtk_state_global->install->cancel) {
			if (deen_ggtk_query_cancel_indexing_on_quit()) {
				deen_ggtk_state_global->install->cancel = DEEN_TRUE;
			} else {
				should_quit = DEEN_FALSE;
			}
		}
	}

	pthread_mutex_unlock(&deen_ggtk_state_global->lock);

    return should_quit;
}

/*
This signal is called when the window WILL close.
*/

gboolean on_window_main_delete_event(
	GtkWidget *widget, GdkEvent *event, gpointer data) {
	// when false then expect the regular window deletion process to run
    // its course.
	return !deen_ggtk_gboolean_from_deen_bool(
		deen_ggtk_quit_requested_should_proceed());
}

/*
This signal is called when the window DID close.
*/

void on_window_main_destroy() {
	while (deen_ggtk_thread_safe_is_indexing()) {
		usleep(1);
	}

	gtk_main_quit();
}

/*
Pressing CTRL-Q in the main window should initiate a quit of the application.
*/

gboolean on_window_main_key_release_event(GtkWidget *widget, GdkEventKey *event) {
	GdkModifierType modifiers = gtk_accelerator_get_default_mod_mask ();
	if (event->keyval == GDK_KEY_q && GDK_CONTROL_MASK == (event->state & modifiers)) {
		if (DEEN_TRUE == deen_ggtk_quit_requested_should_proceed()) {
			// should close the window and end up quitting.
			gtk_widget_destroy(widget);
		}

		// means; event was handled and GTK+ need not take further responsibility
		return TRUE;
	}

	// means; event was not handled and GTK+ should carry on handling it.
	return FALSE;
}