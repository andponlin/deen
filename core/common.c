/*
 * Copyright 2016, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#include "constants.h"

#include "common.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

// 10k
#define BUFFER_SIZE_EACH_WORD_FROM_FILE (1024 * 10)

#define ISWORDCHAR(C) (!isspace(C) && !ispunct(C))

// ---------------------------------------------------------------
// UTILITY
// ---------------------------------------------------------------

time_t deen_seconds_since_epoc() {
	struct timeval tp;

	if (0 != gettimeofday(&tp, NULL)) {
		deen_log_error_and_exit("it was not possible to get the current time");
	}

	return tp.tv_sec;
}

uint32_t deen_max(uint32_t x, uint32_t y) {
	if (x > y) {
		return x;
	}

	return y;
}

// ------------------------------------------------
// STRINGS
// ------------------------------------------------

static const uint8_t COMMON_FUER[] = {
    0x46, 0xc3, 0x9c, 0x52
};

static const char * const COMMON_3[] = {
    // ENGLISH
    "AND", "BUT", "THE", "ARE", "WAS",
    // GERMAN
    "VON",
    "DER", "DIE", "DAS", "UND",
    "ICH", "SIE", "VOM", "WIR",
    "WAR", "IHR", "IHM", "IHN",
	"HAT", "DES", "MIR",
    // END
    NULL
};

static const char * const COMMON_4[] = {
    // ENGLISH
    "THEN", "THEM", "ALSO",
    // GERMAN
    "ABER", "DENN", "ALSO", "ZWAR",
	"SEIN", "SIND", "BIST", "SEID",
	"ODER", "MEIN", "IHRE", "EURE",
    // END
    NULL
};

static const char * const COMMON_5[] = {
    // ENGLISH
    "WHICH",
    // GERMAN
    "HABEN", "MEINE", "IHNEN", "IHREN",
	"IHREM",
    // END
    NULL
};

/*
Returns 1 if the word is a common word out of the supplied list of words.
*/

static deen_bool deen_is_common_word_upper_from_list(
	const uint8_t *s,
	size_t len,
	const char * const words[]) {

	int i = 0;

	while (NULL != words[i]) {

		if (0 == memcmp(s, words[i], len)) {
	    	return DEEN_TRUE;
		}

		i++;
	}

	return DEEN_FALSE;
}


/*
Returns 1 if the word supplied is a common word.
*/

deen_bool deen_is_common_upper_word(const uint8_t *s, size_t len) {
	switch (len) {
		case 2:
		case 1:
		case 0:
		    return DEEN_TRUE;

		case 3:
			if (0 == memcmp(s, COMMON_FUER, 4)) { // the german fuer is a special case with accented characters.
				return DEEN_TRUE;
			}

			if (deen_is_common_word_upper_from_list(s,len,COMMON_3)) {
				return DEEN_TRUE;
			}
			break;

		case 4:
			if (deen_is_common_word_upper_from_list(s,len,COMMON_4)) {
				return DEEN_TRUE;
			}
			break;

		case 5:
			if (deen_is_common_word_upper_from_list(s,len,COMMON_5)) {
				return DEEN_TRUE;
			}
			break;
	}

	return DEEN_FALSE;
}

deen_bool deen_imatches_at(const uint8_t *s, const uint8_t *f, size_t at) {
	// assume that the first char does match.
	size_t o = 0;
	size_t f_len = strlen((const char *)f);

	while (o<f_len) {

		uint8_t c_s = s[at+o];
		uint8_t c_f = f[o];

		if (0==(c_f&0x80)) {
			if (c_s!=c_f && toupper(c_s)!=c_f) {
				return DEEN_FALSE;
			}
		}
		else {
			if((0xc3==c_f)&&(0xc3==c_s)) {

				// don't bother checking to see if we're at the end
				// of the string or we would have a broken utf-8
				// sequence.

				o++;
				c_s = s[at+o];
				c_f = f[o];

				switch (c_f) {
					// first check for the upper case and then the lower case.
					case 0x8b: if(c_s!=0x8b&&c_s!=0xab) return DEEN_FALSE; break; // Ee
					case 0x9c: if(c_s!=0x9c&&c_s!=0xbc) return DEEN_FALSE; break; // Ue
					case 0x96: if(c_s!=0x96&&c_s!=0xb6) return DEEN_FALSE; break; // Oe
					case 0x84: if(c_s!=0x84&&c_s!=0xa4) return DEEN_FALSE; break; // Ae
					case 0x8f: if(c_s!=0x8f&&c_s!=0xaf) return DEEN_FALSE; break; // Ie
					default:
						if(c_f != c_s) {
							return DEEN_FALSE;
						}
						break;
				}
			}
			else {
				if (c_f != c_s) {
					return DEEN_FALSE;
				}
			}
		}

		o++;
	}

	return DEEN_TRUE;
}

/*
Returns the index to the first instance of the string f in
the string s within the bounds (from,to) where from is
inclusive and to is exclusive.  The search is expecting f
to be upper case and s can be mixed case.  This is only
expecting to have to deal with UTF8 strings.  The point of
this is that the searched-in string does not need to be
case-switched and it uses it's limited scope to optimize
the search.
*/

size_t deen_ifind_first(const uint8_t *s, const uint8_t *f, size_t from, size_t to) {
	size_t i = from;
	size_t f_len = strlen((const char *) f);

	if (to < from) {
		deen_log_error_and_exit("the from and to are the wrong way around");
	}

// check to make sure that the keyword could fit into the portion of the string
// being searched within.

	if (from != to && to >= f_len && to-from >= f_len) {
		size_t to_minus_f_len = (to-f_len)+1;

		while (i<to_minus_f_len) {

			uint32_t c_s = (uint32_t) s[i];

			switch (c_s) {
				case 0x20:
				case 0x7c: break;

				default:
					if (deen_imatches_at(s,f,i)) {
						return i;
					}
					break;
			}

			i++;
		}
	}

	return DEEN_NOT_FOUND;
}

/*
 * This will ensure that not only english latin characters are upper-cased, but
 * also german accented characters.
 */

void deen_to_upper(uint8_t *s) {
	size_t len = strlen((const char *)s);

	for (int i=0;i<len;i++) {
		if (0xc3 == s[i]) {
			i++;

			switch(s[i]) {
				case 0xab: s[i] = 0x8b; break;
				case 0xbc: s[i] = 0x9c; break;
				case 0xb6: s[i] = 0x96; break;
				case 0xa4: s[i] = 0x84; break;
				case 0xaf: s[i] = 0x8f; break;
			}
		}
		else {
			s[i] = toupper(s[i]);
		}
	}
}

uint8_t *deen_strnchr(uint8_t *a, uint8_t b, size_t len) {
	size_t i;

	for (i=0;i<len;i++) {
		if (0==a[i]) {
			return NULL;
		}

		if (b==a[i]) {
			return &a[i];
		}
	}

	return NULL;
}


uint8_t *deen_utf8_usascii_equivalent(uint8_t *c, size_t c_length) {
	if (c_length > 2 && c[0] == 0xc3) {
		switch (c[1]) {
			case 0xab: return (uint8_t *) "ee";
			case 0xb6: return (uint8_t *) "oe";
			case 0xbc: return (uint8_t *) "ue";
			case 0xa4: return (uint8_t *) "ae";
			case 0xaf: return (uint8_t *) "ie";

			case 0x8b: return (uint8_t *) "EE";
			case 0x9c: return (uint8_t *) "UE";
			case 0x96: return (uint8_t *) "OE";
			case 0x84: return (uint8_t *) "AE";
			case 0x8f: return (uint8_t *) "IE";

			case 0x9f: return (uint8_t *) "ss";

		}
	}

	return NULL;
}


deen_bool deen_utf8_is_usascii_clean(
	uint8_t *c,
	size_t c_length) {

	uint32_t *c_asint = (uint32_t *) c;
	size_t asints_len = (c_length / 4);
	size_t i;

	for (i = 0; i < asints_len; i++) {
		if (0 != (c_asint[i] & 0x80808080)) {
			return DEEN_FALSE;
		}
	}

	for (i = (asints_len * 4); i < c_length;i++) {
		if (0 != (c[i] & 0x80)) {
			return DEEN_FALSE;
		}
	}

	return DEEN_TRUE;

}


size_t deen_utf8_crop_to_unicode_len(
	uint8_t *c,
	size_t c_length,
	size_t unicode_length) {

	size_t upto = 0;
	size_t unicode_count = 0;

	while (upto < c_length && unicode_count < unicode_length) {

		size_t sequence_length;

		switch (deen_utf8_sequence_len(&c[upto], c_length - upto, &sequence_length)) {

			case DEEN_SEQUENCE_OK:
				upto += sequence_length;
				unicode_count++;
				break;

			case DEEN_BAD_SEQUENCE:
				deen_log_error_and_exit("bad utf8 sequence");
				break;

			case DEEN_INCOMPLETE_SEQUENCE:
				deen_log_error_and_exit("incomplete utf8 sequence");
				break;
		}

	}

	if (unicode_count == unicode_length) {
		c[upto] = 0;
	}

	return unicode_count;
}


deen_utf8_sequence_result deen_utf8_sequences_count(
	const uint8_t *c,
	size_t c_length,
	size_t *sequence_count) {

	sequence_count[0] = 0;

	for (size_t i = 0; i < c_length;) {

		size_t sequence_length;
		deen_utf8_sequence_result sequence_result = deen_utf8_sequence_len(
			&c[i],
			c_length - i,
			&sequence_length);

		if (DEEN_SEQUENCE_OK != sequence_result) {
			sequence_count[0] = 0;
			return sequence_result;
		}

		sequence_count[0]++;
		i += sequence_length;

	}

	return DEEN_SEQUENCE_OK;
}


deen_utf8_sequence_result deen_utf8_sequence_len(
	const uint8_t *c,
	size_t c_length,
	size_t *sequence_length) {

	sequence_length[0] = 0;

	if (0==c_length) {
		return DEEN_INCOMPLETE_SEQUENCE;
	}

	if (0 == (c[0] & 0x80)) {
		sequence_length[0] = 1;
	}
	else {
		if (0xc0 == (c[0] & 0xe0)) {
			sequence_length[0] = 2;
		}
		else {
			if (0xe0 == (c[0] & 0xf0)) {
				sequence_length[0] = 3;
			}
			else {
				if (0xf0 == (c[0] & 0xf8)) {
					sequence_length[0] = 4;
				}
				else {
					return DEEN_BAD_SEQUENCE;
				}
			}
		}
	}

	if (c_length < sequence_length[0]) {
		return DEEN_INCOMPLETE_SEQUENCE;
	}

	for (uint32_t i = 1; i < sequence_length[0]; i++) {
		if (0x80 != (c[i] & 0xc0)) {
			return DEEN_BAD_SEQUENCE;
		}
	}

	return DEEN_SEQUENCE_OK;
}


deen_bool deen_for_each_word_from_file(
	int fd,
	deen_bool (*process_callback)(
		const uint8_t *s,
		size_t len,
		off_t ref, // index in file to after last newline
		float progress,
		void *context),
	void *context) {

	deen_bool result = DEEN_TRUE;

	uint8_t *c_buffer = (uint8_t *) deen_emalloc(sizeof(unsigned char) * BUFFER_SIZE_EACH_WORD_FROM_FILE);
	size_t c_buffer_len = BUFFER_SIZE_EACH_WORD_FROM_FILE;
	size_t c_buffer_loadedlen = 0;

	// find out the length of the file.

	off_t file_last_line_offset = 0;
	off_t file_lastread = 0;
	off_t file_read = 0;
	off_t file_len = lseek(fd,0,SEEK_END);

	if (-1==file_len) {
		DEEN_LOG_ERROR0("unable to obtain the length of the file to be processed");
		result = DEEN_FALSE;
	}

	if (result) {

		if (-1 == lseek(fd,0,SEEK_SET)) {
			DEEN_LOG_ERROR0("unable to return the file pointer back to the start of the file to be processed");
			result = DEEN_FALSE;
		}

		// feed in more data

		while (
			result &&
			((file_lastread = read(fd, &c_buffer[c_buffer_loadedlen], c_buffer_len-c_buffer_loadedlen)) > 0) )
		{
			DEEN_LOG_TRACE1("did read %u additional bytes", file_lastread);

			file_read += file_lastread;
			float progress = (float) file_read / (float) file_len;
			c_buffer_loadedlen += (size_t) file_lastread;

			// find the next non-whitespace.

			deen_bool need_more_data = DEEN_FALSE;
			uint32_t c_buffer_word_start = 0;
			uint32_t c_buffer_word_end = 0;

			while (!need_more_data && result) {

				while (c_buffer_word_start < c_buffer_loadedlen && !ISWORDCHAR(c_buffer[c_buffer_word_start])) {
					if ('\n' == c_buffer[c_buffer_word_start]) {
						// want the index to the next line not the newline character itself.
						file_last_line_offset = (file_read - (c_buffer_loadedlen - c_buffer_word_start)) + 1;
					}

					c_buffer_word_start++;
				}

				if (c_buffer_word_start < c_buffer_loadedlen) {

					c_buffer_word_end = c_buffer_word_start;

					while (
						result &&
						!need_more_data &&
						c_buffer_word_end < c_buffer_loadedlen &&
						ISWORDCHAR(c_buffer[c_buffer_word_end])) {

						size_t utf8_sequence_len;

						switch (deen_utf8_sequence_len(
							&c_buffer[c_buffer_word_end],
							c_buffer_loadedlen - c_buffer_word_end,
							&utf8_sequence_len)) {

								case DEEN_SEQUENCE_OK:
									c_buffer_word_end += utf8_sequence_len;
									break;

								case DEEN_BAD_SEQUENCE:
									DEEN_LOG_ERROR1("bad utf8 sequence at %u", file_read - (c_buffer_loadedlen - c_buffer_word_end));
									result = DEEN_FALSE;
									break;

								case DEEN_INCOMPLETE_SEQUENCE:
									need_more_data = DEEN_TRUE;
									break;

						}
					}

					// if the end of the buffer was reached trying to find the
					// end of a word then more data needs to be loaded in.

					if (c_buffer_word_end == c_buffer_loadedlen) {
						need_more_data = DEEN_TRUE;
					}

					// if the end of a word was found then it should be reported
					// back to the client using the callback function.

					if (result && !need_more_data && c_buffer_word_end < c_buffer_loadedlen) {

						if (!process_callback(
							&c_buffer[c_buffer_word_start],
							c_buffer_word_end - c_buffer_word_start,
							file_last_line_offset,
							progress,
							context)) {

							DEEN_LOG_INFO0("user initiated cancel of word extraction from file");
							result = DEEN_FALSE;
						}

						// move onto the next word.  Not +1 because it might be a
						// newline which needs to be processed.

						c_buffer_word_start = c_buffer_word_end;
					}

				}
				else {
					need_more_data = DEEN_TRUE;
				}
			}

			// if a single word filled the entire buffer then it is
			// necessary that a larger buffer is sought.

			if (result) {
				if (0 == c_buffer_word_end) {
					c_buffer_len += sizeof(unsigned char) * BUFFER_SIZE_EACH_WORD_FROM_FILE;
					c_buffer = (uint8_t *) deen_erealloc(c_buffer, c_buffer_len);
					DEEN_LOG_ERROR1("requiring a larger buffer for reading words from file; %u bytes", c_buffer_len);
				}
				else {

					// now need to move any unprocessed data back to the start of
					// the buffer and read in some more material to complete the
					// current word.
					size_t c_buffer_loadedlen_remaining = c_buffer_loadedlen - c_buffer_word_start;
					memmove(c_buffer, &c_buffer[c_buffer_word_start], c_buffer_loadedlen_remaining);
					c_buffer_loadedlen = c_buffer_loadedlen_remaining;
				}
			}
		}
	}

	if (NULL!=c_buffer) {
		free((void *) c_buffer);
	}

	return result;
}


void deen_for_each_word(
	const uint8_t *s, size_t offset,
	deen_bool (*eachword_callback)(const uint8_t *s, size_t offset, size_t len, void *context),
	void *context
) {

	size_t len = strlen((char *) &s[offset]);

	while (offset < len) {

		// skip whitespace
		while (offset < len && (isspace(s[offset]) || ispunct(s[offset]))) {
			offset++;
		}

		int end = offset;

		while (end < len && !isspace(s[end]) && !ispunct(s[end])) {
			end++;
		}

		if (end != offset) {
			if (DEEN_TRUE != eachword_callback(s, offset, end-offset, context)) {
				offset = len;
			}
			else {
				offset = end;
			}
		}
	}
}


deen_first_keyword deen_ifind_first_keyword(
	const uint8_t *s,
	deen_keywords *keywords,
	size_t from, size_t to) {

	deen_first_keyword result;
	result.keyword = NULL;
	result.offset = DEEN_NOT_FOUND;

	for (int i=0;i<keywords->count;i++) {
		size_t keyword_i = deen_ifind_first(s, keywords->keywords[i], from, to);

		if (DEEN_NOT_FOUND != keyword_i) {
			if (DEEN_NOT_FOUND == result.offset || keyword_i < result.offset) {
				result.offset = keyword_i;
				result.keyword = keywords->keywords[i];
			}
		}
	}

	return result;
}


// ---------------------------------------------------------------
// ENSURED MEMORY ALLOCATION
// ---------------------------------------------------------------

void *deen_emalloc(size_t size) {
	void *r = (void *) malloc(size);
	if (NULL==r) {
		deen_log_error_and_exit("memory exhaustion on malloc");
	}
	return r;
}

// ---------------------------------------------------------------

void *deen_erealloc(void *ptr, size_t size) {
	void *r = (void *) realloc(ptr,size);
	if (NULL==r) {
		deen_log_error_and_exit("memory exhaustion on realloc");
	}
	return r;
}

// ---------------------------------------------------------------
// LOGGING
// ---------------------------------------------------------------

// private
void deen_log(
	const char *prefix,
	const char *file,
	uint32_t line,
	const char *fmt,
	...) {
	va_list argptr;
	va_start(argptr,fmt);
	deen_vlog(prefix,file,line,fmt,argptr);
	va_end(argptr);
}

// ---------------------------------------------------------------

// private
void deen_vlog(
	const char *prefix,
	const char *file,
	uint32_t line,
	const char *fmt,
	va_list argptr) {

	fputs(prefix,stdout);
	fputc(' ',stdout);

#ifdef DEBUG
	if(NULL!=file) {
		fprintf(stdout,"(%s:%u) ",file,line);
	}
#endif

	vfprintf(stdout,fmt,argptr);
	fputc('\n',stdout);
}


static deen_bool deen_global_trace_enabled = DEEN_FALSE;


void deen_set_trace_enabled(deen_bool flag) {
	deen_global_trace_enabled = flag;
}


deen_bool deen_is_trace_enabled() {
	return deen_global_trace_enabled;
}


// ---------------------------------------------------------------

void deen_log_info(const char *fmt, ...) {
	va_list argptr;
	va_start(argptr,fmt);
	deen_vlog(DEEN_PREFIX_INFO,NULL,0,fmt,argptr);
	va_end(argptr);
}

// ---------------------------------------------------------------

void deen_log_error(const char *fmt, ...) {
	va_list argptr;
	va_start(argptr,fmt);
	deen_vlog(DEEN_PREFIX_ERROR,NULL,0,fmt,argptr);
	va_end(argptr);
}

// ---------------------------------------------------------------

void deen_log_error_and_exit(const char *fmt, ...) {
	va_list argptr;
	va_start(argptr,fmt);
	deen_vlog(DEEN_PREFIX_ERROREXIT,NULL,0,fmt,argptr);
	va_end(argptr);
	exit(EXIT_FAILURE);
}

// ---------------------------------------------------------------
