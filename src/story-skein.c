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



#if 0
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
#endif
