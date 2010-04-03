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

#ifndef _STORY_H_
#define _STORY_H_

#include <stdarg.h>
#include <glib-object.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksourcebuffer.h>
#include "osxcart/plist.h"
#include "app.h"
#include "document.h"
#include "panel.h"

#define I7_TYPE_STORY             	(i7_story_get_type())
#define I7_STORY(obj)             	(G_TYPE_CHECK_INSTANCE_CAST((obj), I7_TYPE_STORY, I7Story))
#define I7_STORY_CLASS(klass)     	(G_TYPE_CHECK_CLASS_CAST((klass), I7_TYPE_STORY, I7StoryClass))
#define I7_IS_STORY(obj)          	(G_TYPE_CHECK_INSTANCE_TYPE((obj), I7_TYPE_STORY))
#define I7_IS_STORY_CLASS(klass)  	(G_TYPE_CHECK_CLASS_TYPE((klass), I7_TYPE_STORY))
#define I7_STORY_GET_CLASS(obj)   	(G_TYPE_INSTANCE_GET_CLASS((obj), I7_TYPE_STORY, I7StoryClass))

typedef enum {
	LEFT = 0,
	RIGHT,
	I7_STORY_NUM_PANELS
} I7StoryPanel;

typedef enum {
    I7_STORY_FORMAT_Z5 = 5,
    I7_STORY_FORMAT_Z6 = 6,
    I7_STORY_FORMAT_Z8 = 8,
    I7_STORY_FORMAT_GLULX = 256
} I7StoryFormat;

typedef struct {
	I7DocumentClass parent_class;
} I7StoryClass;

typedef struct {
	I7Document parent_instance;

	GtkWidget *facing_pages;
	GtkWidget *notes_window;
	GtkWidget *notes_view;
	I7Panel *panel[I7_STORY_NUM_PANELS];
} I7Story;

typedef void (*CompileActionFunc)(I7Story *, gpointer);

GType i7_story_get_type(void) G_GNUC_CONST;
I7Story *i7_story_new(I7App *app, const gchar *filename, const gchar *title, const gchar *author);
I7Story *i7_story_new_from_file(I7App *app, const gchar *filename);
I7Story *i7_story_new_from_dialog(I7App *app);
I7Story *i7_story_new_from_uri(I7App *app, const gchar *uri);
gboolean i7_story_open(I7Story *story, const gchar *directory);
I7StoryPanel i7_story_choose_panel(I7Story *story, I7PanelPane newtab);
void i7_story_show_pane(I7Story *story, I7PanelPane pane);
void i7_story_show_tab(I7Story *story, I7PanelPane pane, gint tab);
void i7_story_show_docpage(I7Story *story, gchar *file);
gchar *i7_story_get_materials_path(I7Story *story);
const gchar *i7_story_get_extension(I7Story *story);

/* Source pane, story-source.c */
void on_panel_paste_code(I7Panel *panel, gchar *code, I7Story *story);
void on_panel_jump_to_line(I7Panel *panel, guint line, I7Story *story);
gboolean reindex_headings(GtkTextBuffer *buffer, I7Document *document);

/* Errors pane, story-errors.c */
void i7_story_add_debug_tabs(I7Document *document);
void i7_story_remove_debug_tabs(I7Document *document);
GtkSourceBuffer *create_inform6_source_buffer(void);

/* Index pane, story-index.c */
void i7_story_reload_index_tabs(I7Story *story, gboolean wait);

/* Settings pane, story-settings.c */
PlistObject *create_default_settings(void);
void on_z5_button_toggled(GtkToggleButton *togglebutton, I7Story *story);
void on_z6_button_toggled(GtkToggleButton *togglebutton, I7Story *story);
void on_z8_button_toggled(GtkToggleButton *togglebutton, I7Story *story);
void on_glulx_button_toggled(GtkToggleButton *togglebutton, I7Story *story);
void on_blorb_button_toggled(GtkToggleButton *togglebutton, I7Story *story);
void on_nobble_rng_button_toggled(GtkToggleButton *togglebutton, I7Story *story);
void on_notify_story_format(I7Story *story);
void on_notify_create_blorb(I7Story *story);
void on_notify_nobble_rng(I7Story *story);
I7StoryFormat i7_story_get_story_format(I7Story *story);
void i7_story_set_story_format(I7Story *story, I7StoryFormat format);
gboolean i7_story_get_create_blorb(I7Story *story);
void i7_story_set_create_blorb(I7Story *story, gboolean create_blorb);
gboolean i7_story_get_nobble_rng(I7Story *story);
void i7_story_set_nobble_rng(I7Story *story, gboolean nobble_rng);
gboolean i7_story_get_elastic_tabs(I7Story *story);
void i7_story_set_elastic_tabs(I7Story *story, gboolean elastic_tabs);

/* Compiling, story-compile.c */
void i7_story_set_compile_finished_action(I7Story *story, CompileActionFunc callback, gpointer data);
void i7_story_compile(I7Story *story, gboolean release, gboolean refresh);
void i7_story_save_compiler_output(I7Story *story, const gchar *dialog_title);
void i7_story_save_ifiction(I7Story *story);

/* Game pane, story-game.c */
void i7_story_run_compiler_output(I7Story *story);
void i7_story_run_compiler_output_and_replay(I7Story *story);
void i7_story_run_compiler_output_and_entire_skein(I7Story *story);
void i7_story_stop_running_game(I7Story *story);

#endif /* _STORY_H_ */
