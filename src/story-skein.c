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
#include "configfile.h"
#include "node.h"
#include "skein.h"
#include "skein-view.h"
#include "story.h"
#include "story-private.h"

void
on_node_activate(I7Skein *skein, I7Node *node, I7Story *story)
{
}

void
on_node_popup(I7Skein *skein, I7Node *node, I7Story *story)
{
}

void
on_differs_badge_activate(I7Skein *skein, I7Node *node, I7Story *story)
{
}

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
