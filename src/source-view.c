
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "source-view.h"
#include "builder.h"

#define CONTENTS_FALLBACK_BG_COLOR "ivory"
#define CONTENTS_FALLBACK_FG_COLOR "black"

/* TYPE SYSTEM */

static GtkFrameClass *parent_class = NULL;
G_DEFINE_TYPE(I7SourceView, i7_source_view, GTK_TYPE_FRAME);

static void
i7_source_view_init(I7SourceView *self)
{
	/* Build the interface */
	GtkBuilder *builder = create_new_builder("source.builder.xml", self);

	/* Make our base-class frame invisible */
	gtk_frame_set_label(GTK_FRAME(self), NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(self), GTK_SHADOW_NONE);
	
	/* Reparent the widget into our new frame */
	self->notebook = GTK_WIDGET(load_object(builder, "source_notebook"));
	gtk_container_add(GTK_CONTAINER(self), self->notebook);

	/* Save public pointers to specific widgets */
	self->source = GTK_WIDGET(load_object(builder, "source"));
	self->headings = GTK_WIDGET(load_object(builder, "headings"));
	self->heading_depth = GTK_WIDGET(load_object(builder, "heading_depth"));
	self->message = GTK_WIDGET(load_object(builder, "message"));
	self->previous = GTK_WIDGET(load_object(builder, "previous"));
	self->next = GTK_WIDGET(load_object(builder, "next"));
	
	/* Change the color of the Contents page */
	GdkColor bg, fg;
	/* Look up the colors in the theme; if they aren't specified, use defaults */
	GtkStyle *style = gtk_rc_get_style(GTK_WIDGET(self->headings));
	if(!gtk_style_lookup_color(style, "info_fg_color", &fg))
		gdk_color_parse(CONTENTS_FALLBACK_FG_COLOR, &fg);
	if(!gtk_style_lookup_color(style, "info_bg_color", &bg))
		gdk_color_parse(CONTENTS_FALLBACK_BG_COLOR, &bg);
	gtk_widget_modify_text(self->headings, GTK_STATE_NORMAL, &fg);
	gtk_widget_modify_text(self->message, GTK_STATE_NORMAL, &fg);
	gtk_widget_modify_base(self->headings, GTK_STATE_NORMAL, &bg);
	gtk_widget_modify_base(self->message, GTK_STATE_NORMAL, &bg);

	gtk_range_set_value(GTK_RANGE(self->heading_depth), I7_DEPTH_PARTS_AND_HIGHER);
	
	/* Turn to the default page */
	gtk_notebook_set_current_page(GTK_NOTEBOOK(self->notebook), I7_SOURCE_VIEW_TAB_SOURCE);

	/* Builder object not needed anymore */
	g_object_unref(builder);
}

static void
i7_source_view_finalize(GObject *self)
{
	G_OBJECT_CLASS(parent_class)->finalize(self);
}

static void
i7_source_view_class_init(I7SourceViewClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = i7_source_view_finalize;
	
	parent_class = g_type_class_peek_parent(klass);
}

/* SIGNAL HANDLERS */

gchar *
on_heading_depth_format_value(GtkScale *scale, gdouble value)
{
	if(value < I7_DEPTH_VOLUMES_AND_BOOKS)
		return g_strdup(_("Volumes only"));
	if(value < I7_DEPTH_PARTS_AND_HIGHER)
		return g_strdup(_("Volumes and Books"));
	if(value < I7_DEPTH_CHAPTERS_AND_HIGHER)
		return g_strdup(_("Parts and higher"));
	if(value < I7_DEPTH_ALL_HEADINGS)
		return g_strdup(_("Chapters and higher"));
	return g_strdup(_("All headings"));
}

/* PUBLIC FUNCTIONS */

GtkWidget *
i7_source_view_new()
{
	return GTK_WIDGET(g_object_new(I7_TYPE_SOURCE_VIEW, NULL));
}

void
i7_source_view_set_contents_display(I7SourceView *self, I7ContentsDisplay display)
{
	GtkWidget *headingswin = gtk_widget_get_parent(self->headings);
	GtkWidget *messagewin = gtk_widget_get_parent(self->message);
	switch(display) {
		case I7_CONTENTS_NORMAL:
			gtk_widget_show(headingswin);
			gtk_widget_hide(messagewin);
			break;
		case I7_CONTENTS_TOO_SHALLOW:
			gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(self->message)),
			    _("No headings are visible at this level. Drag the slider below"
			    " to the right to make the headings in the source text visible."), -1);
			gtk_widget_hide(headingswin);
			gtk_widget_show(messagewin);
			break;
		case I7_CONTENTS_NO_HEADINGS:
			gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(self->message)),
			    _("Larger Inform projects are usually divided up with headings "
			    "like 'Chapter 2 - Into the Forest'. This page automatically "
			    "displays those headings as a Table of Contents, but since "
			    "there are no headings in this project yet, there is nothing to"
			    " see."), -1);
			gtk_widget_hide(headingswin);
			gtk_widget_show(messagewin);
	}
}

void 
i7_source_view_jump_to_line(I7SourceView *self, guint line)
{
	GtkTextView *view = GTK_TEXT_VIEW(self->source);
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);
	GtkTextIter cursor, line_end;

	gtk_text_buffer_get_iter_at_line(buffer, &cursor, line - 1); 
	/* line is counted from 0 */
	line_end = cursor;
	if(!gtk_text_iter_ends_line(&line_end))
		/* if already at end, this will push it to the end of the NEXT line */
		gtk_text_iter_forward_to_line_end(&line_end);
	gtk_text_buffer_select_range(buffer, &cursor, &line_end);
	gtk_text_view_scroll_to_mark(view, gtk_text_buffer_get_insert(buffer), 0.25, FALSE, 0.0, 0.0);
	gtk_widget_grab_focus(GTK_WIDGET(view));
}
