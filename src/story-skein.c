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

#include <gnome.h>
#include <math.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include "gtkterp/gtkterp.h"

#include "interface.h"
#include "support.h"

#include "appmenu.h"
#include "appwindow.h"
#include "colorscheme.h"
#include "configfile.h"
#include "datafile.h"
#include "error.h"
#include "prefs.h"
#include "skein.h"
#include "story.h"
#include "tabgame.h"
#include "tabskein.h"

#define max(a, b) (((a) > (b))? (a) : (b))
#define VERTICAL_NODE_FILL_FACTOR 2.0
#define LABEL_MULTIPLIER 0.7

typedef struct {
    Story *story;
    GNode *node;
    GnomeCanvasGroup *nodegroup;
} ClickedNode;

static void
clicked_node_free(gpointer data, GClosure *closure)
{
    g_free(data);
}

static void
skein_popup_play_to_here(GtkMenuItem *menuitem, ClickedNode *clickednode)
{
    play_to_node(clickednode->story->theskein, clickednode->node, 
                 clickednode->story);
}

static void
finished_editing_node(GtkEntry *entry, ClickedNode *clicked)
{
    clicked->story->editingskein = FALSE;
	skein_set_line(clicked->story->theskein, clicked->node, 
				   gtk_entry_get_text(entry));
	/* The Entry is destroyed in the redraw */
}

static void
finished_editing_label(GtkEntry *entry, ClickedNode *clicked)
{
	clicked->story->editingskein = FALSE;
	skein_set_label(clicked->story->theskein, clicked->node,
					gtk_entry_get_text(entry));
	/* Lock the thread containing the affected node */
	skein_lock(clicked->story->theskein, 
			   skein_get_thread_bottom(clicked->story->theskein,
									   clicked->node));
	/* The Entry is destroyed in the redraw */
}

static gboolean
editing_lose_focus(GtkWidget *entry, GdkEventFocus *event, Story *thestory)
{
    thestory->editingskein = FALSE;
    skein_schedule_redraw(thestory->theskein, thestory);
    return FALSE; /* do  not interrupt event */
}

static gboolean
cancel_editing(GtkWidget *entry, GdkEventKey *event, Story *thestory)
{
    if(event->keyval != GDK_Escape)
        return FALSE; /* propagate further */
    thestory->editingskein = FALSE;
    skein_schedule_redraw(thestory->theskein, thestory);
    return TRUE;
}

static void
edit_node(Skein *skein, gboolean label, Story *thestory, GNode *thenode,
		  int whichcanvas)
{
    double vertical_spacing = (double)config_file_get_int("SkeinSettings",
                                                          "VerticalSpacing");
	show_node(skein, GOT_USER_ACTION, thenode, thestory);
    thestory->editingskein = TRUE;
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), 
                       label? 
                       (node_has_label(thenode)? node_get_label(thenode) : "")
					   : node_get_line(thenode));
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
    
	double x = node_get_x(thenode);
	double y = vertical_spacing * (double)(g_node_depth(thenode) - 1);
	GtkRequisition req = { 0, 0 };
	gtk_widget_size_request(entry, &req);
    GnomeCanvasItem *editbox = 
        gnome_canvas_item_new(thestory->skeingroup[whichcanvas], 
							  gnome_canvas_widget_get_type(),
                              "anchor", GTK_ANCHOR_CENTER,
							  "x", x, "y", y,
							  "width", (double)(req.width),
							  "height", (double)(req.height),
                              "widget", entry,
                              NULL);
	if(label) {
		double height = 0.0;
		g_object_get(G_OBJECT(editbox), "height", &height, NULL);
    	gnome_canvas_item_set(editbox, 
							  "y", y - height * VERTICAL_NODE_FILL_FACTOR * 
							  	   LABEL_MULTIPLIER,
							  NULL);
	}
    gtk_widget_show(entry);
    gtk_widget_grab_focus(entry);
	
	/* This is so ugly. But the old clickednode structure doesn't persist
    into this function call */
    ClickedNode *nodedata = g_new0(ClickedNode, 1);
    nodedata->story = thestory;
    nodedata->node = thenode;
    nodedata->nodegroup = NULL;
    g_signal_connect_data(G_OBJECT(entry), "activate", 
                          G_CALLBACK(label? finished_editing_label : 
									 finished_editing_node),
						  nodedata, clicked_node_free, 0);
    g_signal_connect(G_OBJECT(entry), "focus-out-event",
                     G_CALLBACK(editing_lose_focus), thestory);
    g_signal_connect(G_OBJECT(entry), "key-press-event",
                     G_CALLBACK(cancel_editing), thestory);
}

static void
skein_popup_edit(GtkMenuItem *menuitem, ClickedNode *clickednode)
{
	int which = find_out_what_canvas_this_group_is_on(clickednode->story, 
													  clickednode->nodegroup);
    edit_node(clickednode->story->theskein, FALSE, clickednode->story,
			  clickednode->node, which);
}

static void
skein_popup_add_label(GtkMenuItem *menuitem, ClickedNode *clickednode)
{
	int which = find_out_what_canvas_this_group_is_on(clickednode->story, 
													  clickednode->nodegroup);
    edit_node(clickednode->story->theskein, TRUE, clickednode->story,
			  clickednode->node, which);
}

static void
skein_popup_lock(GtkMenuItem *menuitem, ClickedNode *clickednode)
{
    if(node_get_temporary(clickednode->node))
        skein_lock(clickednode->story->theskein, clickednode->node);
    else
        skein_unlock(clickednode->story->theskein, clickednode->node, TRUE);
}

static void
skein_popup_lock_thread(GtkMenuItem *menuitem, ClickedNode *clickednode)
{
    if(node_get_temporary(clickednode->node))
        skein_lock(clickednode->story->theskein, 
                   skein_get_thread_bottom(clickednode->story->theskein, 
                                           clickednode->node));
    else
        skein_unlock(clickednode->story->theskein, 
                     skein_get_thread_top(clickednode->story->theskein,
                                          clickednode->node), TRUE);
}

static void
skein_popup_new_thread(GtkMenuItem *menuitem, ClickedNode *clickednode)
{
	/* JEEZ I am so sick of GnomeCanvas, it SUCKS ASS */
	/* Have to copy everything out of clickednode, because it gets invalidated
	in the redraw */
    double horizontal_spacing = (double)config_file_get_int("SkeinSettings", 
                                                           "HorizontalSpacing");
	Skein *theskein = clickednode->story->theskein;
	Story *thestory = clickednode->story;
	GNode *thenode = clickednode->node;
	int which = find_out_what_canvas_this_group_is_on(thestory, 
													  clickednode->nodegroup);
	GNode *newnode = skein_add_new(theskein, thenode);
	skein_layout(theskein, horizontal_spacing);
	skein_redraw(thestory);
	
	edit_node(theskein, FALSE, thestory, newnode, which);
}

static void
skein_popup_insert_knot(GtkMenuItem *menuitem, ClickedNode *clickednode)
{
	/* Have to copy everything out of clickednode, because it gets invalidated
	in the redraw */
    double horizontal_spacing = (double)config_file_get_int("SkeinSettings", 
                                                            "HorizontalSpacing");
	Skein *theskein = clickednode->story->theskein;
	Story *thestory = clickednode->story;
	GNode *thenode = clickednode->node;
	int which = find_out_what_canvas_this_group_is_on(thestory, 
													  clickednode->nodegroup);
	GNode *newnode = skein_add_new_parent(theskein, thenode);
	skein_layout(theskein, horizontal_spacing);
	skein_redraw(thestory);
	
	edit_node(theskein, FALSE, thestory, newnode, which);
}

static gboolean
can_remove(GNode *node, Story *thestory)
{
	if(game_is_running(thestory) && in_current_thread(thestory->theskein, node)) 
	{
		GtkWidget *dialog = gtk_message_dialog_new_with_markup
			(NULL, 0, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
			 _("<b>Unable to delete the active branch in the skein</b>"));
		gtk_message_dialog_format_secondary_text
			(GTK_MESSAGE_DIALOG(dialog), _("It is not possible to delete the "
			 "branch of the skein that leads to the current position in the "
			 "game. To delete this branch, either stop or restart the game."));
		gtk_dialog_run(GTK_DIALOG(dialog));
    	gtk_widget_destroy(dialog);
		return FALSE;
	}
	
    if(node_get_temporary(node))
        return TRUE;
    GtkWidget *dialog = gtk_message_dialog_new
		(NULL, 0, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
         _("This knot has been locked to preserve it. Do you really want to "
		 "delete it? (This cannot be undone.)"));
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    if(response == GTK_RESPONSE_YES)
        return TRUE;
    return FALSE;
}        

static void
skein_popup_delete(GtkMenuItem *menuitem, ClickedNode *clickednode)
{
	Skein *theskein = clickednode->story->theskein;
	GNode *thenode = clickednode->node;
    if(can_remove(thenode, clickednode->story))
        skein_remove_single(theskein, thenode);
}

static void
skein_popup_delete_below(GtkMenuItem *menuitem, ClickedNode *clickednode)
{
	Skein *theskein = clickednode->story->theskein;
	GNode *thenode = clickednode->node;
    if(can_remove(thenode, clickednode->story))
        skein_remove_all(theskein, thenode, TRUE);
}

static void
skein_popup_delete_thread(GtkMenuItem *menuitem, ClickedNode *clickednode)
{
	Skein *theskein = clickednode->story->theskein;
	GNode *thenode = clickednode->node;
    GNode *topnode = skein_get_thread_top(theskein, thenode);
    if(can_remove(topnode, clickednode->story))
        skein_remove_all(theskein, topnode, TRUE);
}

static gboolean
on_canvas_item_event(GnomeCanvasItem *item, GdkEvent *event, 
                     ClickedNode *clickednode)
{
    Story *thestory = clickednode->story;
    GNode *node = clickednode->node;
    Skein *theskein = thestory->theskein;
    gboolean doubleclick = FALSE;
    
    if(event->type == GDK_2BUTTON_PRESS && event->button.button == 1)
        /* trap double-clicking the left button */
        doubleclick = TRUE;
    else if(event->type != GDK_BUTTON_PRESS || event->button.button != 3) 
        /* otherwise only trap the right button */
        return FALSE;
    /* Do not do a menu if editing skein */
    if(thestory->editingskein)
        return FALSE;
    
    /* Do "Play to here" if it was a double click */
    if(doubleclick) {
        play_to_node(theskein, node, thestory);
        return TRUE;
    }
    /* TODO: trap clicking on the "differs badge" and show the transcript */
    
    /* Pop up the pop-up menu */
    GtkWidget *skeinmenu = gtk_menu_new();
    GtkWidget *menuitem = gtk_menu_item_new_with_label(_("Play to here"));
    gtk_menu_shell_append(GTK_MENU_SHELL(skeinmenu), menuitem);
    g_signal_connect(menuitem, "activate",
      G_CALLBACK(skein_popup_play_to_here), clickednode);
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(skeinmenu), menuitem);
    if(node->parent) { /* if not root */
        menuitem = gtk_menu_item_new_with_label(_("Edit"));
        gtk_menu_shell_append(GTK_MENU_SHELL(skeinmenu), menuitem);
        g_signal_connect(menuitem, "activate",
          G_CALLBACK(skein_popup_edit), clickednode);
        menuitem = gtk_menu_item_new_with_label((node_get_label(node) &&
          strlen(node_get_label(node)))? _("Edit Label") : _("Add Label"));
        gtk_menu_shell_append(GTK_MENU_SHELL(skeinmenu), menuitem);
        g_signal_connect(menuitem, "activate",
          G_CALLBACK(skein_popup_add_label), clickednode);
    }
    menuitem = gtk_menu_item_new_with_label(_("Show in Transcript"));
    gtk_widget_set_sensitive(menuitem, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(skeinmenu), menuitem);
    if(node->parent) {
        menuitem = gtk_menu_item_new_with_label(node_get_temporary(node)?
          _("Lock") : _("Unlock"));
        gtk_menu_shell_append(GTK_MENU_SHELL(skeinmenu), menuitem);
        g_signal_connect(menuitem, "activate",
          G_CALLBACK(skein_popup_lock), clickednode);
        menuitem = gtk_menu_item_new_with_label(node_get_temporary(node)?
          _("Lock This Thread") : _("Unlock This Thread"));
        gtk_menu_shell_append(GTK_MENU_SHELL(skeinmenu), menuitem);
        g_signal_connect(menuitem, "activate",
          G_CALLBACK(skein_popup_lock_thread), clickednode);
    }
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(skeinmenu), menuitem);
    menuitem = gtk_menu_item_new_with_label(_("New Thread"));
    gtk_menu_shell_append(GTK_MENU_SHELL(skeinmenu), menuitem);
    g_signal_connect(menuitem, "activate",
      G_CALLBACK(skein_popup_new_thread), clickednode);
    if(node->parent) {
        menuitem = gtk_menu_item_new_with_label(_("Insert Knot"));
        gtk_menu_shell_append(GTK_MENU_SHELL(skeinmenu), menuitem);
        g_signal_connect(menuitem, "activate",
          G_CALLBACK(skein_popup_insert_knot), clickednode);
        menuitem = gtk_menu_item_new_with_label(_("Delete"));
        gtk_menu_shell_append(GTK_MENU_SHELL(skeinmenu), menuitem);
        g_signal_connect(menuitem, "activate",
          G_CALLBACK(skein_popup_delete), clickednode);
        menuitem = gtk_menu_item_new_with_label(_("Delete all Below"));
        gtk_menu_shell_append(GTK_MENU_SHELL(skeinmenu), menuitem);
        g_signal_connect(menuitem, "activate",
          G_CALLBACK(skein_popup_delete_below), clickednode);
        menuitem = gtk_menu_item_new_with_label(_("Delete all in Thread"));
        gtk_menu_shell_append(GTK_MENU_SHELL(skeinmenu), menuitem);
        g_signal_connect(menuitem, "activate",
          G_CALLBACK(skein_popup_delete_thread), clickednode);
    }
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(skeinmenu), menuitem);
    menuitem = gtk_menu_item_new_with_label(_("Save Transcript to here..."));
    gtk_widget_set_sensitive(menuitem, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(skeinmenu), menuitem);
    gtk_widget_show_all(skeinmenu);
    gtk_menu_popup(GTK_MENU(skeinmenu), NULL, NULL, NULL, NULL, 
      3 /* right button */, event->button.time);
    return TRUE;
}

void
skein_layout_and_redraw(Skein *skein, Story *thestory)
{
    double horizontal_spacing = (double)config_file_get_int("SkeinSettings", 
                                                           "HorizontalSpacing");
	skein_layout(skein, horizontal_spacing);
    skein_schedule_redraw(skein, thestory);
	
	/* Make the labels button sensitive or insensitive, since this function is
	called on every "node-text-changed" signal */
	gboolean labels = skein_has_labels(skein);
	gtk_widget_set_sensitive(lookup_widget(thestory->window, "skein_labels_l"),
							 labels);
	gtk_widget_set_sensitive(lookup_widget(thestory->window, "skein_labels_r"),
							 labels);
}

void
skein_schedule_redraw(Skein *skein, Story *thestory)
{
    if(thestory->redrawingskein == FALSE) {
        thestory->redrawingskein = TRUE;
        g_idle_add((GSourceFunc)skein_redraw, thestory);
    }
}

void
show_node(Skein *skein, guint why, GNode *node, Story *thestory)
{
    double horizontal_spacing = (double)config_file_get_int("SkeinSettings", 
                                                           "HorizontalSpacing");
    double vertical_spacing = (double)config_file_get_int("SkeinSettings",
                                                          "VerticalSpacing");
    /* Why, oh why, can't GnomeCanvas have a SANE scroll mechanism?
    And a model-view-controller interface? */
    double x, y, width, x1 = 0.0, y1 = 0.0, x2 = 0.0, y2 = 0.0;
    double scroll_x, scroll_y;
    int foo;
    extern GtkWidget *inspector_window;
    gchar *skein_widgets[3] = {
        "skein_l", "skein_r", "skein_inspector_canvas"
    };
    gchar *scroll_widgets[3] = {
        "skein_l_scroll", "skein_r_scroll", "skein_inspector_scroll"
    };
    
    switch(why) {
        case GOT_LINE:
        case GOT_USER_ACTION:
            /* Work out the position of the node */
            x = node_get_x(node);
            y = (double)(g_node_depth(node) - 1) * vertical_spacing;
            width = node_get_line_text_width(node) * 0.5;
            for(foo = 0; foo < 3; foo++) {
                GnomeCanvas *canvas = GNOME_CANVAS(lookup_widget(
                    foo == 2? inspector_window : thestory->window,
                    skein_widgets[foo]));
                GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(lookup_widget(
                    foo == 2? inspector_window : thestory->window,
                    scroll_widgets[foo]));
                gnome_canvas_get_scroll_region(canvas, &x1, &y1, &x2, &y2);
                GtkAdjustment *h = gtk_scrolled_window_get_hadjustment(scroll);
                GtkAdjustment *v = gtk_scrolled_window_get_vadjustment(scroll);
                scroll_x = ((x-x1) / (x2-x1)) * (h->upper-h->lower) + h->lower;
                scroll_y = ((y-y1) / (y2-y1)) * (v->upper-v->lower) + v->lower;
                gtk_adjustment_clamp_page(h, scroll_x-width-horizontal_spacing,
                                          scroll_x+width+horizontal_spacing);
                gtk_adjustment_clamp_page(v, scroll_y - 1.5*vertical_spacing,
                                          scroll_y + 1.5*vertical_spacing);
            }
            break;
        case GOT_TRANSCRIPT:
            break;
        default:
            g_assert_not_reached();
    }
}

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

void
on_skein_layout_clicked(GtkToolButton *toolbutton, gpointer user_data)
{
	GtkWidget *dialog = create_skein_spacing_dialog();
	Story *thestory = get_story(GTK_WIDGET(toolbutton));
	thestory->old_horizontal_spacing = config_file_get_int("SkeinSettings", 
														   "HorizontalSpacing");
	thestory->old_vertical_spacing = config_file_get_int("SkeinSettings", 
														 "VerticalSpacing");
	GtkWidget *horiz = lookup_widget(dialog, "skein_horizontal_spacing");
	GtkWidget *vert = lookup_widget(dialog, "skein_vertical_spacing");
	gtk_range_set_value(GTK_RANGE(horiz), 
						(gdouble)thestory->old_horizontal_spacing);
	gtk_range_set_value(GTK_RANGE(vert), 
						(gdouble)thestory->old_vertical_spacing);
	g_signal_connect(lookup_widget(dialog, "skein_spacing_use_defaults"),
					 "clicked",
					 G_CALLBACK(on_skein_spacing_use_defaults_clicked),
					 thestory);
	g_signal_connect(lookup_widget(dialog, "skein_spacing_cancel"), "clicked",
					 G_CALLBACK(on_skein_spacing_cancel_clicked), thestory);
	g_signal_connect(horiz, "value-changed", 
					 G_CALLBACK(on_skein_horizontal_spacing_value_changed),
					 thestory);
	g_signal_connect(vert, "value-changed", 
					 G_CALLBACK(on_skein_vertical_spacing_value_changed),
					 thestory);
	gtk_dialog_run(GTK_DIALOG(dialog));
}

void
on_skein_trim_clicked(GtkToolButton *toolbutton, gpointer user_data)
{
	GtkWidget *dialog = create_skein_trim_dialog();
	Story *thestory = get_story(GTK_WIDGET(toolbutton));
	g_signal_connect(lookup_widget(dialog, "skein_trim_ok"), "clicked",
					 G_CALLBACK(on_skein_trim_ok_clicked), thestory);
	gtk_dialog_run(GTK_DIALOG(dialog));
}

void
on_skein_play_all_clicked(GtkToolButton *toolbutton, gpointer user_data)
{
	
}

static void
free_node_labels(GSList *item, gpointer data)
{
	NodeLabel *nodelabel = (NodeLabel *)item->data;
	if(nodelabel) {
		if(nodelabel->label) g_free(nodelabel->label);
		g_free(nodelabel);
	}
}

static void
jump_to_node(GtkMenuItem *menuitem, GNode *node)
{
	Story *thestory = get_story(GTK_WIDGET(menuitem));
	show_node(thestory->theskein, GOT_USER_ACTION, node, thestory);
}

void
on_skein_labels_show_menu(GtkMenuToolButton *menutoolbutton, gpointer user_data)
{
	Story *thestory = get_story(GTK_WIDGET(menutoolbutton));
	
	/* Destroy the previous menu */
    gtk_menu_tool_button_set_menu(menutoolbutton, NULL);
	/* Create a new menu */
	GtkWidget *menu = gtk_menu_new();
	gtk_widget_show(menu);
	GSList *labels = skein_get_labels(thestory->theskein);
	for( ; labels != NULL; labels = g_slist_next(labels)) {
		NodeLabel *nodelabel = (NodeLabel *)labels->data;
		GtkWidget *item = gtk_menu_item_new_with_label
			(g_strdup(nodelabel->label));
		gtk_widget_show(item);
		g_signal_connect(item, "activate", G_CALLBACK(jump_to_node), 
						 nodelabel->node);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	}
	g_slist_foreach(labels, (GFunc)free_node_labels, NULL);
	g_slist_free(labels);
	
	/* Set the menu as the drop-down menu of the button */
    gtk_menu_tool_button_set_menu(menutoolbutton, menu);
}

void
on_skein_spacing_use_defaults_clicked(GtkButton *button, Story *thestory)
{
	gtk_range_set_value(GTK_RANGE(lookup_widget(GTK_WIDGET(button), 
												"skein_horizontal_spacing")), 
						40.0);
	gtk_range_set_value(GTK_RANGE(lookup_widget(GTK_WIDGET(button), 
												"skein_vertical_spacing")), 
						75.0);
	/* Config file settings now triggered by "value-changed" signal */
	skein_layout_and_redraw(thestory->theskein, thestory);
}

void
on_skein_spacing_cancel_clicked(GtkButton *button, Story *thestory)
{
	/* Close the dialog */
	config_file_set_int("SkeinSettings", "HorizontalSpacing", 
						thestory->old_horizontal_spacing);
    config_file_set_int("SkeinSettings", "VerticalSpacing", 
						thestory->old_vertical_spacing);
    gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(button)));
}

void
on_skein_vertical_spacing_value_changed(GtkRange *range, Story *thestory)
{
	config_file_set_int("SkeinSettings", "VerticalSpacing", 
						(gint)gtk_range_get_value(range));
}

void
on_skein_horizontal_spacing_value_changed(GtkRange *range, Story *thestory)
{
	config_file_set_int("SkeinSettings", "HorizontalSpacing", 
						(gint)gtk_range_get_value(range));
}

void
on_skein_trim_ok_clicked(GtkButton *button, Story *thestory)
{
    int pruning = 31 - (int)gtk_range_get_value
		(GTK_RANGE(lookup_widget(thestory->window, "skein_trim_scale")));
	if(pruning < 1)
		pruning = 1;
	skein_trim(thestory->theskein, skein_get_root_node(thestory->theskein),
			   -1, TRUE);
    gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(button)));
}
