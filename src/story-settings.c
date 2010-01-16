/*  Copyright 2006 P.F. Chimento
 *  This file is part of GNOME Inform 7.
 * 
 *  GNOME Inform 7 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  GNOME Inform 7 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNOME Inform 7; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <glib.h>
#include <gtk/gtk.h>
#include "story.h"
#include "story-private.h"
#include "panel.h"

void
on_z5_button_toggled(GtkToggleButton *button, I7Story *story)
{
	gboolean value = gtk_toggle_button_get_active(button);
	if(value)
		I7_STORY_PRIVATE(story)->story_format = I7_STORY_FORMAT_Z5;
	
	/* When the one changes, change the other */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(story->panel[LEFT]->z5), value);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(story->panel[RIGHT]->z5), value);
}

void
on_z6_button_toggled(GtkToggleButton *button, I7Story *story)
{
	gboolean value = gtk_toggle_button_get_active(button);
	if(value)
		I7_STORY_PRIVATE(story)->story_format = I7_STORY_FORMAT_Z6;
	
	/* When the one changes, change the other */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(story->panel[LEFT]->z6), value);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(story->panel[RIGHT]->z6), value);
}

void
on_z8_button_toggled(GtkToggleButton *button, I7Story *story)
{
	gboolean value = gtk_toggle_button_get_active(button);
	if(value)
		I7_STORY_PRIVATE(story)->story_format = I7_STORY_FORMAT_Z8;
	
	/* When the one changes, change the other */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(story->panel[LEFT]->z8), value);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(story->panel[RIGHT]->z8), value);
}

void
on_glulx_button_toggled(GtkToggleButton *button, I7Story *story)
{
	gboolean value = gtk_toggle_button_get_active(button);
	if(value)
		I7_STORY_PRIVATE(story)->story_format = I7_STORY_FORMAT_GLULX;
	
	/* When the one changes, change the other */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(story->panel[LEFT]->glulx), value);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(story->panel[RIGHT]->glulx), value);
}

void
on_blorb_button_toggled(GtkToggleButton *button, I7Story *story)
{
	gboolean value = gtk_toggle_button_get_active(button);
	I7_STORY_PRIVATE(story)->make_blorb = value;
	
	/* When the one changes, change the other */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(story->panel[LEFT]->blorb), value);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(story->panel[RIGHT]->blorb), value);
}

/* Select all the right buttons according to the story settings */
void i7_story_update_settings(I7Story *story) {
	switch(I7_STORY_PRIVATE(story)->story_format) {
		case I7_STORY_FORMAT_Z6:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(story->panel[LEFT]->z6), TRUE);
			break;
		case I7_STORY_FORMAT_Z8:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(story->panel[LEFT]->z8), TRUE);
			break;
		case I7_STORY_FORMAT_GLULX:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(story->panel[LEFT]->glulx), TRUE);
			break;
		case I7_STORY_FORMAT_Z5:
		default:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(story->panel[LEFT]->z5), TRUE);
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(story->panel[LEFT]->blorb), I7_STORY_PRIVATE(story)->make_blorb);
	/* The callbacks ensure that we don't have to manually set the other ones */
}
