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
 
#ifndef COLOR_SCHEME_H
#define COLOR_SCHEME_H

#include <glib.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcestylescheme.h>

GSList *get_style_schemes_sorted(void);
gboolean is_user_scheme(const gchar *scheme_id);
const gchar *install_scheme(const gchar *fname);
gboolean uninstall_scheme(const gchar *id);
GtkSourceStyleScheme *get_style_scheme(void);
void set_highlight_styles(GtkSourceBuffer *buffer);

#endif
