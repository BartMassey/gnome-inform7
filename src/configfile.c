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
 
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <gtksourceview/gtksourceview.h>
#include "configfile.h"
#include "app.h"
#include "builder.h"
#include "error.h"
#include "story.h"

/* Enum-to-string lookup tables */
GConfEnumStringPair font_set_lookup_table[] = {
    { FONT_STANDARD, "Standard" },
    { FONT_MONOSPACE, "Monospace" },
    { FONT_CUSTOM, "Custom" }
};

GConfEnumStringPair font_size_lookup_table[] = {
    { FONT_SIZE_STANDARD, "Standard" },
    { FONT_SIZE_MEDIUM, "Medium" },
    { FONT_SIZE_LARGE, "Large" },
    { FONT_SIZE_HUGE, "Huge" }
};

/* Don't use these functions if setting more than one key at the same time */
void 
config_file_set_string(const gchar *key, const gchar *value)
{
	GConfClient *client = gconf_client_get_default();
	gconf_client_set_string(client, key, value, NULL);
	g_object_unref(client);
}

gchar *
config_file_get_string(const gchar *key)
{
	GConfClient *client = gconf_client_get_default();
	gchar *retval = gconf_client_get_string(client, key, NULL);
	g_object_unref(client);
	return retval;
}

void 
config_file_set_int(const gchar *key, const gint value)
{
	GConfClient *client = gconf_client_get_default();
	gconf_client_set_int(client, key, value, NULL);
	g_object_unref(client);
}

gint 
config_file_get_int(const gchar *key)
{
	GConfClient *client = gconf_client_get_default();
	gint retval = gconf_client_get_int(client, key, NULL);
	g_object_unref(client);
	return retval;
}

void 
config_file_set_bool(const gchar *key, const gboolean value)
{
	GConfClient *client = gconf_client_get_default();
	gconf_client_set_bool(client, key, value, NULL);
	g_object_unref(client);
}

gboolean 
config_file_get_bool(const gchar *key)
{
	GConfClient *client = gconf_client_get_default();
	gboolean retval = gconf_client_get_bool(client, key, NULL);
	g_object_unref(client);
	return retval;
}

void 
config_file_set_enum(const gchar *key, const gint value, GConfEnumStringPair lookup_table[])
{
	GConfClient *client = gconf_client_get_default();
	gconf_client_set_string(client, key, gconf_enum_to_string(lookup_table, value), NULL);
	g_object_unref(client);
}

gint 
config_file_get_enum(const gchar *key, GConfEnumStringPair lookup_table[])
{
    gchar *string = config_file_get_string(key);
    gint retval = -1;
    gconf_string_to_enum(lookup_table, string, &retval);
    g_free(string);
    return retval;
}

/* returns -1 if unset or invalid */
static int
get_enum_from_entry(GConfEntry *entry, GConfEnumStringPair lookup_table[])
{
	GConfValue *value = gconf_entry_get_value(entry);
	if(!value || value->type != GCONF_VALUE_STRING)
		return -1;
	int enumvalue = -1;
	if(!gconf_string_to_enum(lookup_table, gconf_value_get_string(value), &enumvalue))
		return -1;
	return enumvalue;
}

static void
set_key_to_default(GConfClient *client, GConfEntry *entry)
{
	gconf_client_set(client, gconf_entry_get_key(entry), gconf_client_get_default_from_schema(client, gconf_entry_get_key(entry), NULL), NULL);
}

static void
on_config_font_set_changed(GConfClient *client, guint id, GConfEntry *entry, GtkWidget *combobox)
{
	int newvalue = get_enum_from_entry(entry, font_set_lookup_table);
	/* validate new value */
	if(newvalue == -1) {
		set_key_to_default(client, entry);
		return;
	}
	/* update application to reflect new value */
	I7App *theapp = i7_app_get();
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), newvalue);
	update_font(GTK_WIDGET(theapp->prefs->source_example));
	update_font(GTK_WIDGET(theapp->prefs->tab_example));
	i7_app_foreach_document(theapp, i7_document_update_fonts);
}

static void
on_config_custom_font_changed(GConfClient *client, guint id, GConfEntry *entry, GtkWidget *fontbutton)
{
	const gchar *newvalue = gconf_value_get_string(gconf_entry_get_value(entry));
	/* TODO: validate new value? */
	
	/* update application to reflect new value */
	I7App *theapp = i7_app_get();
	gtk_font_button_set_font_name(GTK_FONT_BUTTON(fontbutton), newvalue);
	if(config_file_get_enum(PREFS_FONT_SET, font_set_lookup_table) == FONT_CUSTOM) {
		update_font(GTK_WIDGET(theapp->prefs->source_example));
		update_font(GTK_WIDGET(theapp->prefs->tab_example));
		i7_app_foreach_document(theapp, i7_document_update_fonts);
	}
}

static void
on_config_font_size_changed(GConfClient *client, guint id, GConfEntry *entry, GtkWidget *combobox)
{
	int newvalue = get_enum_from_entry(entry, font_size_lookup_table);
	/* validate new value */
	if(newvalue == -1) {
		set_key_to_default(client, entry);
		return;
	}
	/* update application to reflect new value */
	I7App *theapp = i7_app_get();
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), newvalue);
	update_font(GTK_WIDGET(theapp->prefs->source_example));
	update_font(GTK_WIDGET(theapp->prefs->tab_example));
	i7_app_foreach_document(theapp, i7_document_update_fonts);
	i7_app_foreach_document(theapp, i7_document_update_font_sizes);
}

static void
on_config_style_scheme_changed(GConfClient *client, guint id, GConfEntry *entry, GtkWidget *list)
{
	I7App *theapp = i7_app_get();
	
	const gchar *newvalue = gconf_value_get_string(gconf_entry_get_value(entry));
	/* TODO: validate new value? */
	
	/* update application to reflect new value */
	select_style_scheme(GTK_TREE_VIEW(list), newvalue);
	update_style(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(theapp->prefs->source_example))));
	i7_app_foreach_document(theapp, i7_document_update_fonts);
	i7_app_foreach_document(theapp, i7_document_update_font_styles);
}

static void
on_config_tab_width_changed(GConfClient *client, guint id, GConfEntry *entry, GtkWidget *range)
{
	int newvalue = gconf_value_get_int(gconf_entry_get_value(entry));
	/* validate new value */
	if(newvalue < 0) {
		set_key_to_default(client, entry);
		return;
	}
	/* update application to reflect new value */
	I7App *theapp = i7_app_get();
	gtk_range_set_value(GTK_RANGE(range), (gdouble)newvalue);
	update_tabs(theapp->prefs->tab_example);
	update_tabs(theapp->prefs->source_example);
	i7_app_foreach_document(theapp, i7_document_update_tabs);
}

static void
on_config_syntax_highlighting_changed(GConfClient *client, guint id, GConfEntry *entry, GtkWidget *toggle)
{
	gboolean newvalue = gconf_value_get_bool(gconf_entry_get_value(entry));
	/* update application to reflect new value */
	I7App *theapp = i7_app_get();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), newvalue);
	i7_app_foreach_document(theapp, i7_document_update_source_highlight);
	i7_app_foreach_document(theapp, i7_document_update_fonts);
}

static void
on_config_intelligence_changed(GConfClient *client, guint id, GConfEntry *entry, GtkWidget *toggle)
{
	gboolean newvalue = gconf_value_get_bool(gconf_entry_get_value(entry));
	/* update application to reflect new value */
	I7App *theapp = i7_app_get();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), newvalue);
	/* make the other checkboxes dependent on this checkbox active or inactive*/
	gtk_widget_set_sensitive(theapp->prefs->auto_number, newvalue);
}

static void
on_config_author_name_changed(GConfClient *client, guint id, GConfEntry *entry, GtkWidget *editable)
{
	const gchar *newvalue = gconf_value_get_string(gconf_entry_get_value(entry));
	/* update application to reflect new value */
	gtk_entry_set_text(GTK_ENTRY(editable), newvalue);
}

static void
on_config_clean_build_files_changed(GConfClient *client, guint id, GConfEntry *entry, GtkWidget *toggle)
{
	gboolean newvalue = gconf_value_get_bool(gconf_entry_get_value(entry));
	/* update application to reflect new value */
	I7App *theapp = i7_app_get();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), newvalue);
	/* make the other checkboxes dependent on this checkbox active or inactive*/
	gtk_widget_set_sensitive(theapp->prefs->clean_index_files, newvalue);
}

static void
on_config_debug_log_visible_changed(GConfClient *client, guint id, GConfEntry *entry, GtkWidget *toggle)
{
	gboolean newvalue = gconf_value_get_bool(gconf_entry_get_value(entry));
	/* update application to reflect new value */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), newvalue);
	i7_app_foreach_document(i7_app_get(), newvalue? i7_story_add_debug_tabs : i7_story_remove_debug_tabs);
}

static void
on_config_use_git_changed(GConfClient *client, guint id, GConfEntry *entry, GtkComboBox *box)
{
	gboolean newvalue = gconf_value_get_bool(gconf_entry_get_value(entry));
	/* update application to reflect new value */
	gtk_combo_box_set_active(box, newvalue? 1 : 0);
}

static void
on_config_elastic_tabs_padding_changed(GConfClient *client, guint id, GConfEntry *entry)
{
	int newvalue = gconf_value_get_int(gconf_entry_get_value(entry));
	/* validate new value */
	if(newvalue < 0) {
		set_key_to_default(client, entry);
		return;
	}
	/* update application to reflect new value */
	i7_app_foreach_document(i7_app_get(), i7_document_refresh_elastic_tabs);
}

#if 0
static gboolean
update_skein_spacing(GtkWidget *window)
{
    Story *thestory = get_story(window);
    skein_invalidate_layout(thestory->theskein);
    skein_layout_and_redraw(thestory->theskein, thestory);
    return FALSE; /* one-shot idle function */
}

static void
on_config_skein_spacing_changed(GConfClient *client, guint id, GConfEntry *entry, GtkWidget *window)
{
    for_each_story_window_idle((GSourceFunc)update_skein_spacing);
}
#endif
static void
on_config_generic_bool_changed(GConfClient *client, guint id, GConfEntry *entry, GtkWidget *toggle)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), gconf_value_get_bool(gconf_entry_get_value(entry)));
}

struct KeyToMonitor {
	const gchar *name, *widgetname;
	void (*callback)();
};

static struct KeyToMonitor keys_to_monitor[] = {
	{ PREFS_FONT_SET, "font_set", on_config_font_set_changed },
	{ PREFS_CUSTOM_FONT, "custom_font", on_config_custom_font_changed },
	{ PREFS_FONT_SIZE, "font_size",	on_config_font_size_changed },
	{ PREFS_STYLE_SCHEME, "schemes_view", on_config_style_scheme_changed },
	{ PREFS_TAB_WIDTH, "tab_ruler", on_config_tab_width_changed },
	{ PREFS_SYNTAX_HIGHLIGHTING, "enable_highlighting", on_config_syntax_highlighting_changed },
	{ PREFS_AUTO_INDENT, "auto_indent", on_config_generic_bool_changed },
	{ PREFS_INTELLIGENCE, "follow_symbols", on_config_intelligence_changed },
	{ PREFS_AUTO_NUMBER_SECTIONS, "auto_number", on_config_generic_bool_changed },
	{ PREFS_AUTHOR_NAME, "author_name", on_config_author_name_changed },
	{ PREFS_CLEAN_BUILD_FILES, "clean_build_files", on_config_clean_build_files_changed },
	{ PREFS_CLEAN_INDEX_FILES, "clean_index_files", on_config_generic_bool_changed },
	{ PREFS_DEBUG_LOG_VISIBLE, "show_debug_tabs", on_config_debug_log_visible_changed },
	{ PREFS_USE_GIT, "glulx_combo", on_config_use_git_changed },
	{ PREFS_ELASTIC_TABS_PADDING, NULL, on_config_elastic_tabs_padding_changed }/*,
	{ PREFS_HORIZONTAL_SPACING, NULL, on_config_skein_spacing_changed },
	{ PREFS_VERTICAL_SPACING, NULL, on_config_skein_spacing_changed }*/
};

/* Check if the config keys exist and if not, set them to defaults. */
void
init_config_file(GtkBuilder *builder)
{
	/* Initialize the GConf client */
	GConfClient *client = gconf_client_get_default();
	gconf_client_set_error_handling(client, GCONF_CLIENT_HANDLE_ALL);

	/* Start monitoring the directories */
	gconf_client_add_dir(client, PREFS_BASE_PATH, GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);

	/* Add listeners to specific keys and pass them their associated widgets as data */
	int i;
	for(i = 0; i < G_N_ELEMENTS(keys_to_monitor); i++)
		gconf_client_notify_add(client, keys_to_monitor[i].name, 
			(GConfClientNotifyFunc)keys_to_monitor[i].callback,
			(keys_to_monitor[i].widgetname)? GTK_WIDGET(load_object(builder, keys_to_monitor[i].widgetname)) : NULL,
			NULL, NULL);

	g_object_unref(client);
}

/* Notify every config key so that the preferences dialog picks it up */
void
trigger_config_file(void)
{
	/* Initialize the GConf client */
	GConfClient *client = gconf_client_get_default();
	/* Trigger the keys */
	int i;
	for(i = 0; i < G_N_ELEMENTS(keys_to_monitor); i++)
		gconf_client_notify(client, keys_to_monitor[i].name);

	g_object_unref(client);
}
