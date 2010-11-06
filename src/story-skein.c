/* This file is part of GNOME Inform 7.
 * Copyright (c) 2006-2009 P. F. Chimento <philip.chimento@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "configfile.h"
#include "node.h"
#include "skein.h"
#include "skein-view.h"
#include "story.h"
#include "story-private.h"

#if 0
void
play_to_node(Skein *skein, GNode *newnode, Story *thestory)
{
    /* Change the current node */
    GNode *current = skein_get_current_node(skein);
    skein_set_current_node(skein, newnode);
    /* Check if the new node is reachable from the old current node */
    gboolean reachable = g_node_is_ancestor(current, newnode);
    /* Build and run if the game isn't running, or the new node is unreachable*/
    if(!game_is_running(thestory) || !reachable)
        on_replay_activate(GTK_MENU_ITEM(lookup_widget(thestory->window, 
          "replay")), NULL);
    else {
        GtkTerp *terp = GTK_TERP(lookup_widget(thestory->window, "game_r"));
        if(!gtk_terp_get_running(terp))
            terp = GTK_TERP(lookup_widget(thestory->window, "game_l"));
        gtk_terp_set_interactive(terp, FALSE);
        /* Get a list of the commands that need to be fed in */
        GSList *commands = skein_get_commands(skein);
        /* Set the current node back to what it was */
        skein_set_current_node(skein, current);
        while(commands != NULL) {
            gtk_terp_feed_command(terp, (gchar *)(commands->data));
            g_free(commands->data);
            commands = g_slist_delete_link(commands, commands);
        }
        gtk_terp_set_interactive(terp, TRUE);
    }
}
#endif

void
on_node_activate(I7Skein *skein, I7Node *node, I7Story *story)
{
}

void
on_differs_badge_activate(I7Skein *skein, I7Node *node, I7Story *story)
{
}

typedef struct {
	I7Node *node;
	I7Skein *skein;
	I7SkeinView *view;
} I7PopupMenuCallbackData;

static void
on_popup_menu_play_to_here(GtkMenuItem *menuitem, I7PopupMenuCallbackData *data)
{
	g_slice_free(I7PopupMenuCallbackData, data);
}

static void
on_popup_menu_edit(GtkMenuItem *menuitem, I7PopupMenuCallbackData *data)
{
	i7_skein_view_edit_node(data->view, data->node);
	g_slice_free(I7PopupMenuCallbackData, data);
}

static void
on_popup_menu_edit_label(GtkMenuItem *menuitem, I7PopupMenuCallbackData *data)
{
	i7_skein_view_edit_label(data->view, data->node);
	g_slice_free(I7PopupMenuCallbackData, data);
}

static void
on_popup_menu_show_in_transcript(GtkMenuItem *menuitem, I7PopupMenuCallbackData *data)
{
	g_slice_free(I7PopupMenuCallbackData, data);
}

static void
on_popup_menu_lock(GtkMenuItem *menuitem, I7PopupMenuCallbackData *data)
{
	if(i7_node_get_locked(data->node))
		i7_skein_unlock(data->skein, data->node);
	else
		i7_skein_lock(data->skein, data->node);
	g_slice_free(I7PopupMenuCallbackData, data);
}

static void
on_popup_menu_lock_thread(GtkMenuItem *menuitem, I7PopupMenuCallbackData *data)
{
	if(i7_node_get_locked(data->node))
		i7_skein_unlock(data->skein, i7_skein_get_thread_top(data->skein, data->node));
	else
		i7_skein_lock(data->skein, i7_skein_get_thread_bottom(data->skein, data->node));
	g_slice_free(I7PopupMenuCallbackData, data);
}

static void
on_popup_menu_new_thread(GtkMenuItem *menuitem, I7PopupMenuCallbackData *data)
{
	I7Node *newnode = i7_skein_add_new(data->skein, data->node);
	i7_skein_view_edit_node(data->view, newnode);
	g_slice_free(I7PopupMenuCallbackData, data);
}

static void
on_popup_menu_insert_knot(GtkMenuItem *menuitem, I7PopupMenuCallbackData *data)
{
	I7Node *newnode = i7_skein_add_new_parent(data->skein, data->node);
	i7_skein_view_edit_node(data->view, newnode);
	g_slice_free(I7PopupMenuCallbackData, data);
}

static gboolean
can_remove(I7Skein *skein, I7Node *node/*, I7Story *story */)
{
	if(/*game_is_running(story) &&*/ i7_skein_is_node_in_current_thread(skein, node)) {
		GtkWidget *dialog = gtk_message_dialog_new_with_markup(NULL, 0, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
			_("<b>Unable to delete the active branch in the skein</b>"));
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), 
		    _("It is not possible to delete the branch of the skein that leads "
			"to the current position in the game. To delete this branch, either"
			" stop or restart the game."));
		/* GTK bug #632511 */
		gtk_widget_show(dialog);
		gtk_window_present(GTK_WINDOW(dialog));
		gtk_dialog_run(GTK_DIALOG(dialog));
    	gtk_widget_destroy(dialog);
		return FALSE;
	}
	
    if(!i7_node_get_locked(node))
        return TRUE;
	
    GtkWidget *dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
         _("This knot has been locked to preserve it. Do you really want to "
		 "delete it? (This cannot be undone.)"));
	gtk_widget_show(dialog);
	gtk_window_present(GTK_WINDOW(dialog));
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return (response == GTK_RESPONSE_YES);
}        

static void
on_popup_menu_delete(GtkMenuItem *menuitem, I7PopupMenuCallbackData *data)
{
    if(can_remove(data->skein, data->node))
		i7_skein_remove_single(data->skein, data->node);
	g_slice_free(I7PopupMenuCallbackData, data);
}

static void
on_popup_menu_delete_below(GtkMenuItem *menuitem, I7PopupMenuCallbackData *data)
{
    if(can_remove(data->skein, data->node))
        i7_skein_remove_all(data->skein, data->node);
	g_slice_free(I7PopupMenuCallbackData, data);
}

static void
on_popup_menu_delete_thread(GtkMenuItem *menuitem, I7PopupMenuCallbackData *data)
{
    I7Node *topnode = i7_skein_get_thread_top(data->skein, data->node);
    if(can_remove(data->skein, topnode))
        i7_skein_remove_all(data->skein, topnode);
	g_slice_free(I7PopupMenuCallbackData, data);
}

static void
on_popup_menu_save_transcript(GtkMenuItem *menuitem, I7PopupMenuCallbackData *data)
{
	g_slice_free(I7PopupMenuCallbackData, data);
}

#define ADD_MENU_ITEM(name, callback) \
	menuitem = gtk_menu_item_new_with_mnemonic(name); \
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem); \
	g_signal_connect(menuitem, "activate", G_CALLBACK(callback), data);
#define ADD_SEPARATOR \
	menuitem = gtk_separator_menu_item_new(); \
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
#define ADD_IMAGE_MENU_ITEM(name, stock, callback) \
	menuitem = gtk_image_menu_item_new_with_mnemonic(name); \
	image = gtk_image_new_from_stock(stock, GTK_ICON_SIZE_MENU); \
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image); \
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem); \
	g_signal_connect(menuitem, "activate", G_CALLBACK(callback), data);

void
on_node_popup(I7SkeinView *view, I7Node *node)
{
	/* Create a one-off callback data structure */
	I7PopupMenuCallbackData *data = g_slice_new0(I7PopupMenuCallbackData);
	data->node = node;
	data->skein = i7_skein_view_get_skein(view);
	data->view = view;

	GtkWidget *menuitem, *image;
	GtkWidget *menu = gtk_menu_new();

	/* Construct menu by hand, since there are too many customizations each time
	 to use a GtkBuilder file */
	ADD_MENU_ITEM(_("_Play to Here"), on_popup_menu_play_to_here);
    ADD_SEPARATOR;
	if(!i7_node_is_root(node)) {
		ADD_MENU_ITEM(_("_Edit"), on_popup_menu_edit);
		ADD_IMAGE_MENU_ITEM(i7_node_has_label(node)? _("Edit _Label") : _("Add _Label"), GTK_STOCK_EDIT, on_popup_menu_edit_label);
    }
	ADD_IMAGE_MENU_ITEM(_("Show in _Transcript"), GTK_STOCK_GO_FORWARD, on_popup_menu_show_in_transcript);
    gtk_widget_set_sensitive(menuitem, FALSE);
    if(!i7_node_is_root(node)) {
		ADD_MENU_ITEM(i7_node_get_locked(node)? _("Un_lock") : _("_Lock"), on_popup_menu_lock);
        ADD_MENU_ITEM(i7_node_get_locked(node)? _("Unloc_k This Thread") : _("Loc_k This Thread"), on_popup_menu_lock_thread);
    }
	ADD_SEPARATOR;
	ADD_MENU_ITEM(_("_New Thread"), on_popup_menu_new_thread);
    if(!i7_node_is_root(node)) {
		ADD_IMAGE_MENU_ITEM(_("_Insert Knot"), GTK_STOCK_ADD, on_popup_menu_insert_knot);
        ADD_IMAGE_MENU_ITEM(_("_Delete"), GTK_STOCK_DELETE, on_popup_menu_delete);
        ADD_MENU_ITEM(_("Delete all _Below"), on_popup_menu_delete_below);
		ADD_MENU_ITEM(_("Delete _all in Thread"), on_popup_menu_delete_thread);
    }
	ADD_SEPARATOR;
	ADD_MENU_ITEM(_("_Save Transcript to here..."), on_popup_menu_save_transcript);
	gtk_widget_set_sensitive(menuitem, FALSE);

	gtk_widget_show_all(menu);
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time());
}

#undef ADD_MENU_ITEM
#undef ADD_SEPARATOR
#undef ADD_IMAGE_MENU_ITEM

typedef struct {
	I7SkeinView *skein_view;
	I7Node *node;
} I7LabelsMenuCallbackData;

static void
jump_to_node(GtkMenuItem *menuitem, I7LabelsMenuCallbackData *data)
{
	i7_skein_view_show_node(data->skein_view, data->node, I7_REASON_USER_ACTION);
}

static void
create_labels_menu(I7SkeinNodeLabel *nodelabel, I7Panel *panel)
{
	GtkWidget *item = gtk_menu_item_new_with_label(nodelabel->label);
	gtk_widget_show(item);

	/* Create a one-off callback data structure */
	I7LabelsMenuCallbackData *data = g_new0(I7LabelsMenuCallbackData, 1);
	data->skein_view = I7_SKEIN_VIEW(panel->tabs[I7_PANE_SKEIN]);
	data->node = nodelabel->node;
	
	g_signal_connect_data(item, "activate", G_CALLBACK(jump_to_node), data, (GClosureNotify)g_free, 0);
	gtk_menu_shell_append(GTK_MENU_SHELL(panel->labels_menu), item);
}

void
on_labels_changed(I7Skein *skein, I7Panel *panel)
{
	GSList *labels = i7_skein_get_labels(skein);

	/* Create a new menu */
	if(panel->labels_menu)
		gtk_widget_destroy(panel->labels_menu);
	panel->labels_menu = gtk_menu_new();
	gtk_widget_show(panel->labels_menu);
	g_slist_foreach(labels, (GFunc)create_labels_menu, panel);
	i7_skein_free_node_label_list(labels);
	
	/* Set the menu as the drop-down menu of the button */
    gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(panel->labels), panel->labels_menu);
}

void
on_show_node(I7Skein *skein, I7SkeinShowNodeReason why, I7Node *node, I7Panel *panel)
{
	i7_skein_view_show_node(I7_SKEIN_VIEW(panel->tabs[I7_PANE_SKEIN]), node, why);
}

void
on_skein_spacing_use_defaults_clicked(GtkButton *button, I7Story *story)
{
	config_file_set_to_default(PREFS_HORIZONTAL_SPACING);
	config_file_set_to_default(PREFS_VERTICAL_SPACING);
}

void
on_skein_spacing_vertical_value_changed(GtkRange *range)
{
	config_file_set_int(PREFS_VERTICAL_SPACING, (gint)gtk_range_get_value(range));
}

void
on_skein_spacing_horizontal_value_changed(GtkRange *range)
{
	config_file_set_int(PREFS_HORIZONTAL_SPACING, (gint)gtk_range_get_value(range));
}

I7Skein *
i7_story_get_skein(I7Story *story)
{
	I7_STORY_USE_PRIVATE(story, priv);
	return priv->skein;
}
