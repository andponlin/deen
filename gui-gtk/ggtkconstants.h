/*
 * Copyright 2019, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#ifndef __DEEN_GGTK_CONSTANTS_H
#define __DEEN_GGTK_CONSTANTS_H

/*
The GTK system supports resources that can be bundled into the binary.  See the
Makefile for commands that do this.  These are the names of the resources that
are being bundled.  The names can be used in code to reference the resources.
*/

#define DEEN_GGTK_RESOURCE_GLADE_MAIN "/nz/co/lindesay/deen/ggtkmain.glade"
#define DEEN_GGTK_RESOURCE_ICON "/nz/co/lindesay/deen/icon.svg"

/*
The text in the display of the results is indented; either with a numerical
prefix or without.  This value represents how far in the tab is.
*/

#define DEEN_GGTK_ENTRY_TAB 25

#endif // __DEEN_GGTK_CONSTANTS_H