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
 
#ifndef EXT_WINDOW_H
#define EXT_WINDOW_H

#include <gnome.h>

#if !GTK_CHECK_VERSION(2,10,0)
# define SUCKY_GNOME 1
#endif

void
after_ext_window_realize               (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_xclose_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_xsave_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_xsave_as_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_xrevert_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_xundo_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_xredo_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_xcut_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_xcopy_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_xpaste_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_xselect_all_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_xfind_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_xcheck_spelling_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_xautocheck_spelling_activate        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_xshift_selection_right_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_xshift_selection_left_activate      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_ext_window_delete_event             (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_ext_window_destroy                  (GtkObject       *object,
                                        gpointer         user_data);

void jump_to_line_ext(GtkWidget *widget, gint line);
#endif