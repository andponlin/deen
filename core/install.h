/*
 * Copyright 2016-2019, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */
 
#ifndef __INSTALL_H
#define __INSTALL_H

#ifndef __MINGW32__
#include <pthread.h>
#endif

#include "common.h"

enum deen_install_check_ding_format_check_result {
    DEEN_INSTALL_CHECK_OK,
    DEEN_INSTALL_CHECK_IS_COMPRESSED,
    DEEN_INSTALL_CHECK_IO_PROBLEM,
    DEEN_INSTALL_CHECK_TOO_SMALL,
    DEEN_INSTALL_CHECK_BAD_FORMAT
};

/*
This object is accessed as a singleton.  It is able to
control the state of the installation of the data
file into an index.
*/

enum deen_install_state {
	DEEN_INSTALL_STATE_IDLE,
	DEEN_INSTALL_STATE_STARTING,
	DEEN_INSTALL_STATE_INDEXING,
	DEEN_INSTALL_STATE_COMPLETED,
	DEEN_INSTALL_STATE_ERROR,
};

enum deen_install_check_ding_format_check_result deen_install_check_for_ding_format(const char *filename);

void deen_log_install_progress(enum deen_install_state state, float progress);

/*
 This is a function pointer type that returns a boolean.  It is called during
 indexing and if it returns false then the indexing process should stop.
*/

typedef deen_bool (*deen_is_cancelled_cb)(void *context);

/*
 This is a function pointer type for a function that gets called when some
 non-trivial progress has been made in the indexing process.
 */

typedef deen_bool (*deen_install_progress_cb)(
	void *context, enum deen_install_state state, float progress);

deen_bool deen_install_from_path(
	const char *deen_root_dir,
	const char *filename,
	void *process_cb_context,
	deen_install_progress_cb progress_cb,
	deen_is_cancelled_cb is_cancelled_cb);

/*
 Returns true if the data files for Deen are already installed in the root
 directory.
 */

deen_bool deen_is_installed(const char *deen_root_dir);

#endif /* __INSTALL_H */
