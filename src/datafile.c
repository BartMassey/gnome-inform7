/*  Copyright 2007 P.F. Chimento
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
 
#include <gnome.h>
#include <stdarg.h>

#include "datafile.h"
#include "error.h"

#define EXTENSIONS_BASE_PATH "Inform", "Extensions"

/* Returns the directory for installed extensions, with the author and name path
components tacked on if they are not NULL. Returns an allocated string which
must be freed. */
gchar *get_extension_path(const gchar *author, const gchar *extname) {
    if(!author)
        return g_build_filename(g_get_home_dir(), EXTENSIONS_BASE_PATH, NULL);
    if(!extname)
        return g_build_filename(g_get_home_dir(), EXTENSIONS_BASE_PATH, author,
          NULL);
    return g_build_filename(g_get_home_dir(), EXTENSIONS_BASE_PATH, author,
      extname, NULL);
}


/* Check to see if all the external binaries are in the path and the wine
directory is set up correctly */
gboolean check_external_binaries() {
    if(!g_find_program_in_path("frotz")) {
        error_dialog(NULL, NULL, "The 'frotz' binary was not found in your "
          "path. You must have the Frotz interpreter installed in order to use "
          "GnomeInform 7.");
        return FALSE;
    }
    return TRUE;
}

/* Returns the path to filename in the application data directory. */
gchar *get_datafile_path(const gchar *filename) {
    gchar *real_filename = g_build_filename("gnome-inform7", filename, NULL);
    gchar *path = gnome_program_locate_file(gnome_program_get(), 
      GNOME_FILE_DOMAIN_APP_DATADIR, real_filename, TRUE, NULL);
    g_free(real_filename);
    if(path)
        return path;
    error_dialog(NULL, NULL, "An application file, %s, was not found. Please "
      "reinstall GNOME Inform 7.", filename);
    return NULL;
}

/* Concatenates the path elements and returns the path to the filename in the
application data directory. Must end with NULL. */
gchar *get_datafile_path_va(const gchar *path1, ...) {
    va_list ap;
    
    int num_args = 0;
    va_start(ap, path1);
    do
        num_args++;
    while(va_arg(ap, gchar *) != NULL);
    va_end(ap);
    
    gchar **args = g_new(gchar *, num_args + 2);
    args[0] = g_strdup("gnome-inform7");
    args[1] = g_strdup(path1);
    int i;
    va_start(ap, path1);
    for(i = 2; i < num_args + 2; i++)
        args[i] = g_strdup(va_arg(ap, gchar *));
    va_end(ap);
    
    gchar *real_filename = g_build_filenamev(args);
    gchar *path = gnome_program_locate_file(gnome_program_get(), 
      GNOME_FILE_DOMAIN_APP_DATADIR, real_filename, TRUE, NULL);
    g_free(real_filename);
    
    if(path) {
        g_strfreev(args);
        return path;
    }
    error_dialog(NULL, NULL, "An application file, %s, was not found. Please "
      "reinstall GNOME Inform 7.", args[num_args]); /* argument before NULL */
    g_strfreev(args);
    return NULL;
}

/* Returns TRUE if filename exists in the data directory, otherwise FALSE.
Used when we do not necessarily want to return an error if it does not. */
gboolean check_datafile(const gchar *filename) {
    gchar *real_filename = g_build_filename("gnome-inform7", filename, NULL);
    gchar *path = gnome_program_locate_file(gnome_program_get(), 
      GNOME_FILE_DOMAIN_APP_DATADIR, real_filename, TRUE, NULL);
    g_free(real_filename);
    if(path)
        return TRUE;
    return FALSE;
}
