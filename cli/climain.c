/*
 * Copyright 2016, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#include "renderplain.h"
#include "core/constants.h"
#include "core/common.h"
#include "core/install.h"
#include "core/keyword.h"
#include "core/search.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>

#include <sys/types.h>

typedef struct deen_cli_args deen_cli_args;

struct deen_cli_args {
	deen_bool index;
	deen_bool trace_enabled;
	uint32_t result_count;
	uint8_t *search_expression;
	char *ding_filename;
};

static void deen_cli_init_args(deen_cli_args *args) {
	args->index = DEEN_FALSE;
	args->trace_enabled = DEEN_FALSE;
	args->result_count = DEEN_RESULT_SIZE_DEFAULT;
	args->search_expression = NULL;
	args->ding_filename = NULL;
}

static void deen_cli_syntax() {
	printf("version %s\n",DEEN_VERSION);
	printf("deen [-t] [-i] <ding-file>\n");
	printf("deen [-t] [-c <result-count>] <search-term>\n");
	exit(1);
}

static void deen_cli_capture_args(deen_cli_args *args, int argc, char** argv) {
	for (int i = 1; i < argc; i++) {
		if ('-' == argv[i][0]) {

			if (2 != strlen(argv[i])) {
				deen_log_error_and_exit("unrecognized switch; %s", argv[i]);
			}

			switch (argv[i][1]) {

				case 'i':
					args->index = DEEN_TRUE;

					if (i == argc - 1) {
						deen_log_error_and_exit("expected a ding file to be specified");
					}

					args->ding_filename = argv[i + 1];
					i++;
					break;

				case 't':
					args->trace_enabled = DEEN_TRUE;
					break;

				case 'c':
					if (i == argc - 1) {
						deen_log_error_and_exit("expected a count to be specified");
					}

					args->result_count = (uint32_t) atoi(argv[i + 1]);

					if (0 == args->result_count) {
						deen_log_error_and_exit("bad result count value; %s", argv[i + 1]);
					}

					i++;
					break;

				default:
					deen_log_error_and_exit("unrecognized switch; %s", argv[i]);
					break;

			}

		} else {
			args->search_expression = (uint8_t *) argv[i];

			if (i != argc - 1) {
				deen_log_error_and_exit("the search-term must be the last argument");
			}
		}
	}
}

static void deen_cli_validate_args(deen_cli_args *args) {
	if (args->index) {
		if (NULL != args->search_expression) {
			deen_log_error_and_exit("when indexing, search arguments are not allowed");
		}
	}
	else {
		if (NULL == args->search_expression || 0 == args->search_expression[0]) {
			deen_log_error_and_exit("a search expression was expected");
		}
	}
}


static const char *deen_cli_state_to_string(enum deen_install_state state) {
	switch(state) {
		case DEEN_INSTALL_STATE_IDLE: return "idle";
		case DEEN_INSTALL_STATE_STARTING: return "starting";
		case DEEN_INSTALL_STATE_INDEXING: return "indexing";
		case DEEN_INSTALL_STATE_COMPLETED: return "completed";
		case DEEN_INSTALL_STATE_ERROR: return "error";
		default: return "???";
	}
}


static deen_bool deen_cli_install_progress_cb(enum deen_install_state state, float progress) {
	switch(state) {
		case DEEN_INSTALL_STATE_INDEXING:
		case DEEN_INSTALL_STATE_COMPLETED:
			{
				int percentage = (int) (100.0f * progress);
				DEEN_LOG_INFO2("%12s %3d%%", deen_cli_state_to_string(state), percentage);
			}
			break;

		default:
			DEEN_LOG_INFO1("%s", deen_cli_state_to_string(state));
	}

	return DEEN_TRUE; // keep going
}


/**
 * Caller is responsible for freeing this.
 */

static char *deen_cli_root_dir() {
	struct passwd *pw = getpwuid(getuid());
	char *deen_root_dir = (char *) deen_emalloc(strlen(pw->pw_dir) + 2 + strlen(DIR_DEEN));
	sprintf(deen_root_dir, "%s/%s", pw->pw_dir, DIR_DEEN);
	return deen_root_dir;
}


static void deen_cli_index(const char *filename) {
	deen_bool cancel = DEEN_FALSE;
	char *deen_root_dir = deen_cli_root_dir();

	deen_install_from_path(
		NULL, // mutex lock for cancelling
		&cancel,
		deen_root_dir,
		filename,
		deen_cli_install_progress_cb);

	free((void *) deen_root_dir);
}


static void deen_cli_check_and_index(const char *filename) {
	switch (deen_install_check_for_ding_format(filename)) {

		case DEEN_INSTALL_CHECK_OK:
			DEEN_LOG_INFO0("the ding input file looks like valid data");
			deen_cli_index(filename);
			break;

		case DEEN_INSTALL_CHECK_IS_COMPRESSED:
			DEEN_LOG_ERROR0("the ding input file looks compressed; decompress it first");
			break;

		case DEEN_INSTALL_CHECK_IO_PROBLEM:
			DEEN_LOG_ERROR0("a problem has arisen processing the ding input file - io problem");
			break;

		case DEEN_INSTALL_CHECK_TOO_SMALL:
			DEEN_LOG_ERROR0("a problem has arisen processing the ding input file - too small");
			break;

		case DEEN_INSTALL_CHECK_BAD_FORMAT:
			DEEN_LOG_ERROR0("the input ding file looks malformed");
			break;

		default:
			deen_log_error_and_exit("illegal state; unknown check result");
			break;

	}
}


static void deen_cli_trace_keywords(deen_keywords *keywords) {
	uint32_t i;

	for(i=0;i<keywords->count;i++) {
		DEEN_LOG_TRACE2("keyword %u; [%s]", i, keywords->keywords[i]);
	}
}


static void deen_cli_query(deen_cli_args *args) {

	char *deen_root_dir = deen_cli_root_dir();
	deen_keywords *keywords = deen_keywords_create();
	size_t search_expression_len = strlen((char *) args->search_expression);
	uint8_t *search_expression_upper = (uint8_t *) deen_emalloc(
		sizeof(uint8_t) * (search_expression_len + 1));

	search_expression_upper[search_expression_len] = 0;
	memcpy(
		search_expression_upper,
		args->search_expression,
		search_expression_len);

	deen_to_upper(search_expression_upper);

	DEEN_LOG_TRACE2("keywords; [%s] --> [%s]", args->search_expression, search_expression_upper);
	deen_keywords_add_from_string(keywords, search_expression_upper);

	// dump out the keywords for now
	deen_cli_trace_keywords(keywords);

	// run the search

	deen_search_context *context = deen_search_init(deen_root_dir);

	if(NULL==context) {
		deen_log_error_and_exit("unable to create a search context");
	}

	deen_search_result *result = deen_search(context, keywords, args->result_count);

	if(0 == result->total_count) {
		if(deen_keywords_adjust(keywords)) {
			DEEN_LOG_INFO0("no results found -> did adjust keywords");
			deen_cli_trace_keywords(keywords);
			deen_search_result_free(result);
			result = deen_search(context, keywords, args->result_count);
		}
	}

    deen_render_plain(result, keywords);

    deen_search_result_free(result);
    deen_search_free(context);
    deen_keywords_free(keywords);

    free((void *) deen_root_dir);
	free((void *) search_expression_upper);
}


int main(int argc, char** argv) {

	if (1 == argc) {
		deen_cli_syntax();
	}

	// capture the argument inputs from the user.

	deen_cli_args args;
	deen_cli_init_args(&args);
	deen_cli_capture_args(&args, argc, argv);
	deen_cli_validate_args(&args);

	if (args.trace_enabled) {
		deen_set_trace_enabled(DEEN_TRUE);
	}

	// now action the indexing.

	if (args.index) {
		deen_cli_check_and_index(args.ding_filename);
	} else {
		deen_cli_query(&args);
	}

	return EXIT_SUCCESS;
}
