/*
 * Copyright 2019, Andrew Lindesay. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Lindesay, apl@lindesay.co.nz
 */

#include "ggtkrendertextbuffer.h"

#include "ggtkgeneral.h"
#include "core/keyword.h"

#define NUMBER_PREFIX_BUFFER_LEN 32

static uint8_t UTF8_LANGUAGE_SEPARATOR[8] = {
	0x20,
	0xe2, 0x94, 0x80, // horizontal line
	0xe2, 0x94, 0x80, // horizontal line
	0x20
};

static uint8_t UTF8_ENTRY_SEPARATOR[13] = {
	0xe2, 0x94, 0x84,
	0xe2, 0x94, 0x84,
	0xe2, 0x94, 0x84,
	0xe2, 0x94, 0x84,
	0x0a
};

void deen_ggtk_append_to_textbuffer_with_tag(
	GtkTextBuffer *target, const gchar *text, gint len, GtkTextTag *tag) {
	if (0 != len) {
		GtkTextIter end_iter;
		gtk_text_buffer_get_end_iter(target, &end_iter);
		gtk_text_buffer_insert_with_tags(target, &end_iter, text, len, tag, NULL);
	}
}

void deen_ggtk_append_to_textbuffer(GtkTextBuffer *target, const gchar *text, gint len) {
	if (0 != len) {
		GtkTextIter end_iter;
		gtk_text_buffer_get_end_iter(target, &end_iter);
		gtk_text_buffer_insert(target, &end_iter, text, len);
	}
}

void deen_ggtk_append_space_to_textbuffer(GtkTextBuffer *target) {
	deen_ggtk_append_to_textbuffer(target, " ", 1);
}

/*
This will go through the text and look for keywords.  It will highlight any
keywords as bold and print the rest.
*/

static void deen_ggtk_render_plain_text_highlights(
	GtkTextBuffer *target,
	uint8_t *text,
	deen_keywords *keywords) {

	size_t len = strlen((char *) text);
	size_t upto = 0;

	while (upto < len) {
		deen_first_keyword first_keyword = deen_ifind_first_keyword(
			text, keywords, upto, len);

		if (DEEN_NOT_FOUND == first_keyword.offset) {
			deen_ggtk_append_to_textbuffer(
				target,
				(gchar *) &text[upto],
				strlen((char *) &text[upto]));
			upto = len;
		}
		else {
			deen_bool is_valid_keyword_found;
			size_t keyword_len = strlen((char *) first_keyword.keyword);
			deen_ggtk_append_to_textbuffer(
				target,
				(gchar *) &text[upto],
				first_keyword.offset - upto);

			// need to make sure that highlighting is only happening at
			// the prefix of a word and is not finding text 'randomly'
			// within the line.

			is_valid_keyword_found =
				0 == first_keyword.offset ||
				isspace(text[first_keyword.offset-1]) ||
				ispunct(text[first_keyword.offset-1]);

			if(is_valid_keyword_found) {
				deen_ggtk_append_to_textbuffer_with_tag(
					target,
					(gchar *) &text[first_keyword.offset],
					keyword_len,
					deen_ggtk_state_global->search->tag_background_keyword_hit);
			} else {
				deen_ggtk_append_to_textbuffer(
					target,
					(gchar *) &text[first_keyword.offset],
					keyword_len);
			}

			upto = first_keyword.offset + keyword_len;
		}
	}
}


static void deen_ggtk_render_plain_entry_atom(
	GtkTextBuffer *target,
	deen_entry_atom *atom,
	deen_keywords *keywords) {

		switch (atom->type) {
			case ATOM_TEXT:
				deen_ggtk_render_plain_text_highlights(target, atom->text, keywords);
				break;

			case ATOM_CONTEXT:
				deen_ggtk_append_space_to_textbuffer(target);
				deen_ggtk_append_to_textbuffer_with_tag(
					target, (gchar *) atom->text, strlen((char *) atom->text),
					deen_ggtk_state_global->search->tag_foreground_context);
				deen_ggtk_append_space_to_textbuffer(target);
				break;

			case ATOM_GRAMMAR:
				deen_ggtk_append_space_to_textbuffer(target);
				deen_ggtk_append_to_textbuffer_with_tag(
					target, (gchar *) atom->text, strlen((char *) atom->text),
					deen_ggtk_state_global->search->tag_foreground_grammar);
				deen_ggtk_append_space_to_textbuffer(target);
				break;

			default:
				deen_log_error_and_exit("unknown atom type");
				break;
		}
}


void deen_ggtk_render_plain_entry_sub_sub(
	GtkTextBuffer *target,
	deen_entry_sub_sub *sub_sub,
	deen_keywords *keywords) {

	if (NULL != sub_sub) {
		uint32_t i;

		for (i = 0; i < sub_sub->atom_count; i++) {
			if (0 != i) {
				deen_ggtk_append_to_textbuffer(target, " ", 1);
			}

			deen_ggtk_render_plain_entry_atom(target, &(sub_sub->atoms[i]), keywords);
		}
	}
}


void deen_ggtk_render_plain_entry_sub(
	GtkTextBuffer *target,
	deen_entry_sub *sub,
	deen_keywords *keywords) {

	if (NULL != sub) {
		uint32_t i;

		for (i = 0; i < sub->sub_sub_count; i++) {
			if (0 != i) {
				deen_ggtk_append_to_textbuffer(target, "; ", 2);
			}

			deen_ggtk_render_plain_entry_sub_sub(target, &(sub->sub_subs[i]), keywords);
		}
	}
}

void deen_ggtk_render_plain_entry(
	GtkTextBuffer *target,
	deen_entry *entry,
	deen_keywords *keywords) {

	char number_buffer[NUMBER_PREFIX_BUFFER_LEN];
	uint32_t i;
	uint32_t max_sub_count = entry->german_sub_count;

	if (entry->english_sub_count > max_sub_count) {
		max_sub_count = entry->english_sub_count;
	}

	for (i = 0; i < max_sub_count; i++) {

		if (1 == max_sub_count) {
			deen_ggtk_append_to_textbuffer(target, "\t", 1);
		}
		else {
			snprintf(number_buffer, NUMBER_PREFIX_BUFFER_LEN, "%2d)\t", i + 1);
			deen_ggtk_append_to_textbuffer_with_tag(
				target, number_buffer, strlen(number_buffer),
				deen_ggtk_state_global->search->tag_foreground_layout);
		}

		if (i < entry->german_sub_count) {
			deen_ggtk_render_plain_entry_sub(target, &(entry->german_subs[i]), keywords);
		}
		else {
			deen_ggtk_append_to_textbuffer(target, "???", 3);
		}

		deen_ggtk_append_to_textbuffer_with_tag(
			target, (gchar *) UTF8_LANGUAGE_SEPARATOR, 8,
			deen_ggtk_state_global->search->tag_foreground_layout);

		if (i < entry->english_sub_count) {
			deen_ggtk_render_plain_entry_sub(target, &(entry->english_subs[i]), keywords);
		}
		else {
			deen_ggtk_append_to_textbuffer(target, "???", 3);
		}

		deen_ggtk_append_to_textbuffer(target, "\n", 1);
	}
}

void deen_ggtk_render_entry_separator(GtkTextBuffer *target) {
		deen_ggtk_append_to_textbuffer_with_tag(
			target, (gchar *) UTF8_ENTRY_SEPARATOR, 13,
			deen_ggtk_state_global->search->tag_foreground_layout);
}

void deen_ggtk_render_textbuffer(
	GtkTextBuffer *target,
	deen_search_result *result,
	deen_keywords *keywords) {

	if (NULL != result && result->entry_count > 0) {
		uint32_t i = result->entry_count;

		do {
			if (i != result->entry_count) {
				deen_ggtk_render_entry_separator(target);
			}
			i--;
			deen_ggtk_render_plain_entry(target, &result->entries[i], keywords);
		}
		while(i > 0);
	}

	GtkTextIter start_iter, end_iter;

	gtk_text_buffer_get_start_iter(target, &start_iter);
	gtk_text_buffer_get_end_iter(target, &end_iter);

	gtk_text_buffer_apply_tag(
		target,
		deen_ggtk_state_global->search->tag_hanging_indent_and_tab_stops,
		&start_iter,
		&end_iter);
}