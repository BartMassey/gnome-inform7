/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * new-i7
 * Copyright (C) P. F. Chimento 2010 <philip.chimento@gmail.com>
 * 
 * new-i7 is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * new-i7 is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gdk/gdkkeysyms.h>
#include <goocanvas.h>
#include "skein-view.h"
#include "skein.h"
#include "node.h"

typedef struct _I7SkeinViewPrivate
{
	I7Skein *skein;
	gulong layout_handler;
} I7SkeinViewPrivate;

#define I7_SKEIN_VIEW_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE((o), I7_TYPE_SKEIN_VIEW, I7SkeinViewPrivate))
#define I7_SKEIN_VIEW_USE_PRIVATE(o,n) I7SkeinViewPrivate *n = I7_SKEIN_VIEW_PRIVATE(o)

G_DEFINE_TYPE(I7SkeinView, i7_skein_view, GOO_TYPE_CANVAS);

static void
on_item_created(I7SkeinView *view, GooCanvasItem *item, GooCanvasItemModel *model, I7Skein **skeinptr)
{
	if(I7_IS_NODE(model)) {
		i7_node_calculate_size(I7_NODE(model), GOO_CANVAS_ITEM_MODEL(*skeinptr), GOO_CANVAS(view));
		g_signal_connect(item, "button-press-event", G_CALLBACK(on_node_button_press), model);
	}
}

static void
i7_skein_view_init(I7SkeinView *self)
{
	I7_SKEIN_VIEW_USE_PRIVATE(self, priv);
	priv->skein = NULL;
	priv->layout_handler = 0;

	g_signal_connect_after(self, "item-created", G_CALLBACK(on_item_created), &priv->skein);
}

static void
i7_skein_view_finalize(GObject *self)
{
	I7_SKEIN_VIEW_USE_PRIVATE(self, priv);

	if(priv->skein) {
		g_signal_handler_disconnect(priv->skein, priv->layout_handler);
		g_object_unref(priv->skein);
	}

	G_OBJECT_CLASS(i7_skein_view_parent_class)->finalize(self);
}

static void
i7_skein_view_class_init(I7SkeinViewClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = i7_skein_view_finalize;

	/* Add private data */
	g_type_class_add_private(klass, sizeof(I7SkeinViewPrivate));
}

/* PUBLIC FUNCTIONS */

GtkWidget *
i7_skein_view_new(void)
{
	return g_object_new(I7_TYPE_SKEIN_VIEW, NULL);
}

void 
i7_skein_view_set_skein(I7SkeinView *self, I7Skein *skein)
{
	g_return_if_fail(self || I7_IS_SKEIN_VIEW(self));
	g_return_if_fail(skein || I7_IS_SKEIN(skein));
	I7_SKEIN_VIEW_USE_PRIVATE(self, priv);

	if(priv->skein == skein)
		return;

	if(priv->skein) {
		g_signal_handler_disconnect(priv->skein, priv->layout_handler);
		g_object_unref(priv->skein);
	}
	priv->skein = skein;

	if(skein == NULL) {
		goo_canvas_set_root_item_model(GOO_CANVAS(self), NULL);
		return;
	}
	
	goo_canvas_set_root_item_model(GOO_CANVAS(self), GOO_CANVAS_ITEM_MODEL(skein));
	g_object_ref(skein);
	priv->layout_handler = g_signal_connect(skein, "needs-layout", G_CALLBACK(i7_skein_draw), self);
	i7_skein_draw(skein, GOO_CANVAS(self));
}

I7Skein *
i7_skein_view_get_skein(I7SkeinView *self)
{
	g_return_val_if_fail(self || I7_IS_SKEIN_VIEW(self), NULL);
	I7_SKEIN_VIEW_USE_PRIVATE(self, priv);
	return priv->skein;
}

static gboolean
on_edit_popup_key_press(GtkWidget *entry, GdkEventKey *event, GtkWidget *edit_popup)
{
	switch(event->keyval) {
	case GDK_Escape:
		gtk_widget_destroy(edit_popup);
		return TRUE;
	case GDK_Return:
	case GDK_KP_Enter:
		{
			I7Node *node = I7_NODE(g_object_get_data(G_OBJECT(edit_popup), "node"));
			i7_node_set_command(node, gtk_entry_get_text(GTK_ENTRY(entry)));
			gtk_widget_destroy(edit_popup);
			return TRUE;
		}
	}
	return FALSE; /* event not handled */
}

void
i7_skein_view_edit_node(I7SkeinView *self, I7Node *node)
{
	GtkWidget *parent = gtk_widget_get_toplevel(GTK_WIDGET(self));
	
	GtkWidget *edit_popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	/* stackoverflow.com/questions/1925568/how-to-give-keyboard-focus-to-a-pop-up-gtk-window */
	GTK_WIDGET_SET_FLAGS(edit_popup, GTK_CAN_FOCUS);
	gtk_window_set_decorated(GTK_WINDOW(edit_popup), FALSE);
	gtk_window_set_has_frame(GTK_WINDOW(edit_popup), FALSE);
	gtk_window_set_type_hint(GTK_WINDOW(edit_popup), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
	gtk_window_set_transient_for(GTK_WINDOW(edit_popup), GTK_WINDOW(parent));
	gtk_window_set_gravity(GTK_WINDOW(edit_popup), GDK_GRAVITY_CENTER);
	/* Associate this window with the node we are editing */
	g_object_set_data(G_OBJECT(edit_popup), "node", node);

	GtkWidget *entry = gtk_entry_new();
	gtk_widget_add_events(entry, GDK_FOCUS_CHANGE_MASK);
	gchar *command = i7_node_get_command(node);
	gtk_entry_set_text(GTK_ENTRY(entry), command);
	g_free(command);
	gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
	gtk_container_add(GTK_CONTAINER(edit_popup), entry);
	g_signal_connect_swapped(entry, "focus-out-event", G_CALLBACK(gtk_widget_destroy), edit_popup);
	g_signal_connect(entry, "key-press-event", G_CALLBACK(on_edit_popup_key_press), edit_popup);
	
	gtk_widget_show_all(edit_popup);

	gint x, y, px, py;
	if(!i7_node_get_command_coordinates(node, &x, &y, GOO_CANVAS(self)))
		return;
	gdk_window_get_origin(gtk_widget_get_window(parent), &px, &py);

	gtk_window_move(GTK_WINDOW(edit_popup), x + px, y + py);
	/* GDK_GRAVITY_CENTER seems to have no effect? */
	gtk_widget_grab_focus(entry);
	gtk_window_present(GTK_WINDOW(edit_popup));
}	
