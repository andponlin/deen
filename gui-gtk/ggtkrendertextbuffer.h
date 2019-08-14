/*
 * Copyright 2019, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#ifndef __DEEN_GGTK_RENDERTEXTBUFFER_H
#define __DEEN_GGTK_RENDERTEXTBUFFER_H

#include <gtk/gtk.h>

#include "core/types.h"

void deen_ggtk_render_textbuffer(
	GtkTextBuffer *target,
	deen_search_result *result,
	deen_keywords *keywords);

#endif // __DEEN_GGTK_RENDERTEXTBUFFER_H