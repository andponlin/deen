/*
 * Copyright 2016, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */
 
#ifndef __INSTALL_H
#define __INSTALL_H

#include "common.h"

#include <pthread.h>

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

/*
 This is a function pointer type for a function that gets called when some
 non-trivial progress has been made in the indexing process.
 */

typedef deen_bool (*deen_install_progress_cb)(enum deen_install_state state, float progress);

deen_bool deen_install_from_path(
	pthread_mutex_t *cancel_mutex,
	uint8_t *cancel, // boolean
	const char *deen_root_dir,
	const char *filename,
	deen_install_progress_cb progress_cb);

#endif /* __INSTALL_H */
