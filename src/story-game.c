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

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libchimara/chimara-if.h>
#include "story.h"
#include "story-private.h"
#include "error.h"

void
i7_story_run_compiler_output(I7Story *story)
{
	I7_STORY_USE_PRIVATE(story, priv);
	GError *err = NULL;

    I7StoryPanel side = i7_story_choose_panel(story, I7_PANE_GAME);
	ChimaraIF *glk = CHIMARA_IF(story->panel[side]->tabs[I7_PANE_GAME]);
	    
    /* Load and start the interpreter */
	if(!chimara_if_run_game(glk, priv->compiler_output, &err)) {
		error_dialog(GTK_WINDOW(story), err, _("Could not load interpreter: "));
    }
	
	/* Display and set the focus to the interpreter */
	i7_story_show_pane(story, I7_PANE_GAME);
    gtk_widget_grab_focus(GTK_WIDGET(glk));
}

void
i7_story_run_compiler_output_and_replay(I7Story *story)
{
}

void
i7_story_run_compiler_output_and_entire_skein(I7Story *story)
{
}

void
i7_story_stop_running_game(I7Story *story)
{
	ChimaraGlk *glk;

	int side;
	/* Execute for both panels */
	for(side = LEFT; side < I7_STORY_NUM_PANELS; side++) {
		glk = CHIMARA_GLK(story->panel[side]->tabs[I7_PANE_GAME]);
		if(chimara_glk_get_running(glk)) {
			chimara_glk_stop(glk);
			chimara_glk_wait(glk); /* Seems to be necessary? */
		}
	}
}

gboolean 
i7_story_get_game_running(I7Story *story)
{
	return chimara_glk_get_running(CHIMARA_GLK(story->panel[LEFT]->tabs[I7_PANE_GAME]))
		|| chimara_glk_get_running(CHIMARA_GLK(story->panel[RIGHT]->tabs[I7_PANE_GAME]));
}

#if 0
/* Callback for when the interpreter is finished */
static void 
on_interpreter_exit(GtkTerp *terp, gint exit_code, Story *thestory)
{
    gtk_terp_unload_game(terp);
    
    if(thestory->handler_finished)
        g_signal_handler_disconnect((gpointer)terp, thestory->handler_finished);
    if(thestory->handler_input)
        g_signal_handler_disconnect((gpointer)terp, thestory->handler_input);
    
    thestory->handler_finished = 0;
    thestory->handler_input = 0;
    
    gtk_widget_set_sensitive(lookup_widget(thestory->window, "stop"), FALSE);
    gtk_widget_set_sensitive(lookup_widget(thestory->window, "stop_toolbutton"),
      FALSE);
}

/* Grab commands entered by the user and store them in the skein */
static void 
catch_input(GtkTerp *terp, const gchar *command, Story *thestory)
{
    skein_new_line(thestory->theskein, command);
}

/* Run the story in the GtkTerp widget */
void 
run_project(Story *thestory) 
{
    int right = choose_notebook(thestory->window, TAB_GAME);
    GtkTerp *terp = GTK_TERP(lookup_widget(thestory->window, right?
      "game_r" : "game_l"));
    
    /* Load and start the interpreter */
    gchar *file = g_strconcat("output.", get_story_extension(thestory), NULL);
    gchar *path = g_build_filename(thestory->filename, "Build", file, NULL);
    g_free(file);
    GError *err = NULL;
    if(!gtk_terp_load_game(terp, path, &err)) {
        error_dialog(GTK_WINDOW(thestory->window), err,
          _("Could not load interpreter: "));
        g_free(path);
        return;
    }
    g_free(path);
    
    /* Get a list of the commands that need to be fed in */
    GSList *commands = skein_get_commands(thestory->theskein);
    
    /* Set non-interactive if there are commands, because if we don't, the first
    screen might freeze on a "-- more --" prompt and ignore the first automatic
    input */
    if(commands)
        gtk_terp_set_interactive(terp, FALSE);
    
    if(!gtk_terp_start_game(terp, (thestory->story_format == FORMAT_GLULX)?
       (config_file_get_bool("IDESettings", "UseGit")? 
	   GTK_TERP_GIT : GTK_TERP_GLULXE) : GTK_TERP_FROTZ, &err)) {
        error_dialog(GTK_WINDOW(thestory->window), err,
          _("Could not start interpreter: "));
        gtk_terp_unload_game(terp);
        return;
    }

    /* Connect signals and save the signal handlers to disconnect later */
    thestory->handler_finished = g_signal_connect((gpointer)terp,
      "stopped", G_CALLBACK(on_interpreter_exit), (gpointer)thestory);
    thestory->handler_input = g_signal_connect((gpointer)terp,
      "command-received", G_CALLBACK(catch_input), (gpointer)thestory);
    
    /* Now the "Stop" option works */
    gtk_widget_set_sensitive(lookup_widget(thestory->window, "stop"), TRUE);
    gtk_widget_set_sensitive(lookup_widget(thestory->window, "stop_toolbutton"),
      TRUE);
    /* Display and set the focus to the terminal widget */
    gtk_notebook_set_current_page(get_notebook(thestory->window, right),
      TAB_GAME);
    gtk_widget_grab_focus(GTK_WIDGET(terp));
    
    /* Wait for the interpreter to get ready */
    while(!gtk_terp_get_running(terp))
        gtk_main_iteration();
    
    /* Feed the commands up to the current pointer in the skein into the
    terminal */
    skein_reset(thestory->theskein, TRUE);      
    while(commands != NULL) {
        gtk_terp_feed_command(terp, (gchar *)(commands->data));
        g_free(commands->data);
        commands = g_slist_delete_link(commands, commands);
    }
    gtk_terp_set_interactive(terp, TRUE);
}
#endif

/* SIGNAL HANDLERS */

/* Set the "stop" action to be sensitive when the game starts */
void
on_game_started(ChimaraGlk *game, I7Story *story)
{
	I7_STORY_USE_PRIVATE(story, priv);
	GtkAction *stop = gtk_action_group_get_action(priv->story_action_group, "stop");
	gtk_action_set_sensitive(stop, TRUE);
}

/* Set the "stop" action to be insensitive when the game finishes */
void
on_game_stopped(ChimaraGlk *game, I7Story *story)
{
	I7_STORY_USE_PRIVATE(story, priv);
	GtkAction *stop = gtk_action_group_get_action(priv->story_action_group, "stop");
	gtk_action_set_sensitive(stop, FALSE);
}
