/*
 * Copyright 2016, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */
 
#ifndef RENDERCOMMON_H
#define RENDERCOMMON_H

#include "core/types.h"

/*
Figures out if the current environment is UTF8 or not.
*/

deen_bool deen_term_is_utf8();

/*
Takes into account the encoding of the terminal and will adjust accordingly.
For example, if it is not UTF-8 then it will not print UTF-8 chars at all.
*/

void deen_term_print_str(uint8_t *str);

void deen_term_print_str_range(uint8_t *str, size_t from, size_t to);

#endif /* RENDERCOMMON_H */
