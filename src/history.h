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
 
#ifndef HISTORY_H
#define HISTORY_H

#include <glib.h>
#include "panel.h"

void history_free_queue(I7Panel *panel);
void history_goto_current(I7Panel *panel);
void history_push_pane(I7Panel *panel, I7PanelPane pane);
void history_push_tab(I7Panel *panel, I7PanelPane pane, guint tab);
void history_push_docpage(I7Panel *panel, const gchar *uri);

#endif /* HISTORY_H */
