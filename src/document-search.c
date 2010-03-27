#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksourceiter.h>
#include <webkit/webkit.h>
#include "document.h"
#include "document-private.h"

#define FINDBAR_NOT_FOUND_FALLBACK_BG_COLOR "tomato"
#define FINDBAR_NOT_FOUND_FALLBACK_FG_COLOR "white"

/* THE "SEARCH ENGINE" */

static gboolean
find_no_wrap(const GtkTextIter *startpos, const gchar *text, gboolean forward, GtkSourceSearchFlags flags, I7SearchType search_type, GtkTextIter *match_start, GtkTextIter *match_end)
{
	if(search_type == I7_SEARCH_CONTAINS)
		return forward? 
			gtk_source_iter_forward_search(startpos, text, flags, match_start, match_end, NULL)
			: gtk_source_iter_backward_search(startpos, text, flags, match_start, match_end, NULL);

	GtkTextIter start, end, searchfrom = *startpos;
	while(forward? 
		gtk_source_iter_forward_search(&searchfrom, text, flags, &start, &end, NULL)
		: gtk_source_iter_backward_search(&searchfrom, text, flags, &start, &end, NULL))
	{
		if(search_type == I7_SEARCH_FULL_WORD && gtk_text_iter_starts_word(&start) && gtk_text_iter_ends_word(&end)) {
			*match_start = start;
			*match_end = end;
			return TRUE;
		} else if(search_type == I7_SEARCH_STARTS_WORD && gtk_text_iter_starts_word(&start)) {
			*match_start = start;
			*match_end = end;
			return TRUE;
		}
		searchfrom = forward? end : start;
	}
	return FALSE;
}

static gboolean
find(GtkTextBuffer *buffer, const gchar *text, gboolean forward, gboolean ignore_case, gboolean restrict_search, I7SearchType search_type, GtkTextIter *match_start, GtkTextIter *match_end)
{
	GtkTextIter iter;
	GtkSourceSearchFlags flags = GTK_SOURCE_SEARCH_TEXT_ONLY
		| (ignore_case? GTK_SOURCE_SEARCH_CASE_INSENSITIVE : 0)
		| (restrict_search? GTK_SOURCE_SEARCH_VISIBLE_ONLY : 0);

	/* Start the search at the end or beginning of the selection */
	if(forward)
		gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_selection_bound(buffer));
	else 
		gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));
	
	if(!find_no_wrap(&iter, text, forward, flags, search_type, match_start, match_end)) 
	{
		/* Wrap around to the beginning or end */
		if(forward)
			gtk_text_buffer_get_start_iter(buffer, &iter);
		else
			gtk_text_buffer_get_end_iter(buffer, &iter);
		if(!find_no_wrap(&iter, text, forward, flags, search_type, match_start, match_end))
			return FALSE;
	}
	return TRUE;
}

/* CALLBACKS */

void
on_findbar_entry_changed(GtkEditable *editable, I7Document *document)
{
	i7_document_unhighlight_quicksearch(document);
	gchar *search_text = gtk_editable_get_chars(editable, 0, -1);
	i7_document_set_quicksearch_not_found(document, !i7_document_highlight_quicksearch(document, search_text, TRUE));
	g_free(search_text);
}

void
on_find_entry_changed(GtkEditable *editable, I7Document *document)
{
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(editable));
    gboolean text_not_empty = !(text == NULL || strlen(text) == 0);
    gtk_widget_set_sensitive(document->find_button, text_not_empty);
    gtk_widget_set_sensitive(document->replace_button, text_not_empty);
    gtk_widget_set_sensitive(document->replace_all_button, text_not_empty);
}

void
on_find_button_clicked(GtkButton *button, I7Document *document)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(document->find_entry));
	gboolean ignore_case = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(document->ignore_case));
	gboolean forward = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(document->reverse));
	gboolean restrict_search = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(document->restrict_search));
	I7SearchType search_type = gtk_combo_box_get_active(GTK_COMBO_BOX(document->search_type));
	i7_document_find(document, text, forward, ignore_case, restrict_search, search_type);
}

void
on_replace_button_clicked(GtkButton *button, I7Document *document)
{
	I7_DOCUMENT_USE_PRIVATE(document, priv);
	const gchar *search_text = gtk_entry_get_text(GTK_ENTRY(document->find_entry));
	const gchar *replace_text = gtk_entry_get_text(GTK_ENTRY(document->replace_entry));
	gboolean ignore_case = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(document->ignore_case));
	gboolean forward = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(document->reverse));
	gboolean restrict_search = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(document->restrict_search));
	I7SearchType search_type = gtk_combo_box_get_active(GTK_COMBO_BOX(document->search_type));
    GtkTextIter start, end;
	GtkTextBuffer *buffer = GTK_TEXT_BUFFER(priv->buffer);

    gtk_text_buffer_get_selection_bounds(buffer, &start, &end);
    gchar *selected = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

	/* if the text is already selected, then replace it, otherwise "find" again 
	 to select the text */
    if(!(ignore_case? strcasecmp(selected, search_text) : strcmp(selected, search_text))) {
		/* Replacing counts as one action for Undo */
		gtk_text_buffer_begin_user_action(buffer);
		gtk_text_buffer_delete(buffer, &start, &end);
		gtk_text_buffer_insert(buffer, &start, replace_text, -1);
		gtk_text_buffer_end_user_action(buffer);
	}
	g_free(selected);
	
    /* Find the next occurrence of the text */
    i7_document_find(document, search_text, forward, ignore_case, restrict_search, search_type);
}

void
on_replace_all_button_clicked(GtkButton *button, I7Document *document)
{
	I7_DOCUMENT_USE_PRIVATE(document, priv);
	const gchar *search_text = gtk_entry_get_text(GTK_ENTRY(document->find_entry));
	const gchar *replace_text = gtk_entry_get_text(GTK_ENTRY(document->replace_entry));
	gboolean ignore_case = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(document->ignore_case));
	gboolean restrict_search = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(document->restrict_search));
	I7SearchType search_type = gtk_combo_box_get_active(GTK_COMBO_BOX(document->search_type));
    GtkTextIter cursor, start, end;
	GtkTextBuffer *buffer = GTK_TEXT_BUFFER(priv->buffer);
	GtkSourceSearchFlags flags = GTK_SOURCE_SEARCH_TEXT_ONLY
		| (ignore_case? GTK_SOURCE_SEARCH_CASE_INSENSITIVE : 0)
		| (restrict_search? GTK_SOURCE_SEARCH_VISIBLE_ONLY : 0);
	
    /* Replace All counts as one action for Undo */
    gtk_text_buffer_begin_user_action(buffer);

    gtk_text_buffer_get_start_iter(buffer, &cursor);
    int replace_count = 0;

    while(find_no_wrap(&cursor, search_text, TRUE, flags, search_type, &start, &end)) {
		/* delete preserves start and end iterators */
        gtk_text_buffer_delete(buffer, &start, &end);
		/* Save end position */
		GtkTextMark *tempmark = gtk_text_buffer_create_mark(buffer, NULL, &end, FALSE);
        gtk_text_buffer_insert(buffer, &start, replace_text, -1);
		/* Continue from end position, so as to avoid a loop if replace text
		 contains search text */
        gtk_text_buffer_get_iter_at_mark(buffer, &cursor, tempmark);
		gtk_text_buffer_delete_mark(buffer, tempmark);
        replace_count++;
    }

    gtk_text_buffer_end_user_action(buffer);

	gchar *message = g_strdup_printf(_("%d occurrences replaced"), replace_count);
    i7_document_flash_status_message(document, message, SEARCH_OPERATIONS);
	g_free(message);

	/* Close the dialog */
	gtk_widget_hide(document->find_dialog);
}

/* PUBLIC FUNCTIONS */

gboolean 
i7_document_highlight_quicksearch(I7Document *document, const gchar *text, gboolean forward)
{
	return I7_DOCUMENT_GET_CLASS(document)->highlight_search(document, text, forward);
}

void 
i7_document_unhighlight_quicksearch(I7Document *document)
{
	GtkWidget *focus = i7_document_get_highlighted_view(document);
	if(!focus)
		return;
	
	if(GTK_IS_TEXT_VIEW(focus)) {
		/* Remove selection */
		GtkTextIter start;
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(focus));
		if(gtk_text_buffer_get_selection_bounds(buffer, &start, NULL))
			gtk_text_buffer_place_cursor(buffer, &start);
	} else if(WEBKIT_IS_WEB_VIEW(focus)) {
		webkit_web_view_set_highlight_text_matches(WEBKIT_WEB_VIEW(focus), FALSE);
		webkit_web_view_unmark_text_matches(WEBKIT_WEB_VIEW(focus));
	}
	
	i7_document_set_highlighted_view(document, NULL);
}

void 
i7_document_set_highlighted_view(I7Document *document, GtkWidget *view)
{
	I7_DOCUMENT_PRIVATE(document)->highlighted_view = view;
}

GtkWidget *
i7_document_get_highlighted_view(I7Document *document)
{
	return I7_DOCUMENT_PRIVATE(document)->highlighted_view;
}

void 
i7_document_set_quicksearch_not_found(I7Document *document, gboolean not_found)
{
	if(not_found) {
		GdkColor bg, fg;
		/* Look up the colors in the theme, otherwise use fallback colors */
		GtkStyle *style = gtk_rc_get_style(document->findbar_entry);
		if(!gtk_style_lookup_color(style, "error_fg_color", &fg))
			gdk_color_parse(FINDBAR_NOT_FOUND_FALLBACK_FG_COLOR, &fg);
		if(!gtk_style_lookup_color(style, "error_bg_color", &bg))
			gdk_color_parse(FINDBAR_NOT_FOUND_FALLBACK_BG_COLOR, &bg);
		gtk_widget_modify_base(document->findbar_entry, GTK_STATE_NORMAL, &bg);
		gtk_widget_modify_text(document->findbar_entry, GTK_STATE_NORMAL, &fg);
		i7_document_flash_status_message(document, _("Phrase not found"), SEARCH_OPERATIONS);
	} else {
		gtk_widget_modify_base(document->findbar_entry, GTK_STATE_NORMAL, NULL);
		gtk_widget_modify_text(document->findbar_entry, GTK_STATE_NORMAL, NULL);
	}
}

void
i7_document_find(I7Document *document, const gchar *text, gboolean forward, gboolean ignore_case, gboolean restrict_search, I7SearchType search_type)
{
	I7_DOCUMENT_USE_PRIVATE(document, priv);
	GtkTextIter start, end;

	if(!find(GTK_TEXT_BUFFER(priv->buffer), text, forward, ignore_case, restrict_search, search_type, &start, &end)) 
	{
		i7_document_flash_status_message(document, _("Phrase not found"), SEARCH_OPERATIONS);
		return;
	}

	/* We may have searched the invisible regions, so if the found text is 
	 invisible, go back to showing the entire source. */
	if(gtk_text_iter_has_tag(&start, priv->invisible_tag) || gtk_text_iter_has_tag(&end, priv->invisible_tag))
		i7_document_show_entire_source(document);
	
	gtk_text_buffer_select_range(GTK_TEXT_BUFFER(priv->buffer), &start, &end);
	i7_document_scroll_to_selection(document);
}
