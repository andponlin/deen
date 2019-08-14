/*
 * Copyright 2019, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#ifndef __DEEN_GGTK_GENERAL_H
#define __DEEN_GGTK_GENERAL_H

#include <gtk/gtk.h>

#include "ggtktypes.h"

// ------------------------------------------------
// STATE
// ------------------------------------------------

/*
This value stores the state for the application; the various widgets, pointers
to data etc...
*/

extern deen_ggtk_state *deen_ggtk_state_global;

deen_ggtk_state *deen_ggtk_state_create();
void deen_ggtk_state_free(deen_ggtk_state *value);

// ------------------------------------------------
// GUI
// ------------------------------------------------

gboolean deen_ggtk_main_stack_update_visible_ui_on_idle(void *context);
void deen_ggtk_main_stack_update_visible_ui();
void deen_ggtk_update_ui();
void deen_ggtk_setup_global_from_builder(GtkBuilder *builder);
void deen_ggtk_setup_icon();

// ------------------------------------------------
// GTK TYPE HANDLING
// ------------------------------------------------

/*
Deen has it's own type for a boolean in the C language as does GTK.  This
function wil convert from the Deen one to the GTK one.
*/

gboolean deen_ggtk_gboolean_from_deen_bool(deen_bool b);

// ------------------------------------------------

#endif // __DEEN_GGTK_GENERAL_H