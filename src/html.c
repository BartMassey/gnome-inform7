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
#include <glib/gi18n.h>
#include <webkit/webkit.h>
#include "html.h"
#include "error.h"

/* Have the html widget display the HTML file in filename */
void 
html_load_file(WebKitWebView *html, const gchar *filename) 
{
    g_return_if_fail(html);
    g_return_if_fail(filename || strlen(filename));
    
    GError *error = NULL;
	gchar *uri = g_filename_to_uri(filename, NULL, &error);
	if(!uri) {
		WARN_S(_("Could not convert filename to URI"), filename, error);
		return;
	}
	webkit_web_view_open(html, uri); /* Deprecated since 1.1.1 */
	g_free(uri);
}

/* Blank the html widget */
void 
html_load_blank(WebKitWebView *html) 
{
    g_return_if_fail(html);
	webkit_web_view_open(html, "about:blank"); /* Deprecated since 1.1.1 */
}

/* Reload the html widget */
void
html_refresh(WebKitWebView *html)
{
	g_return_if_fail(html);
	webkit_web_view_reload(html);
}

#if 0
struct literal {
	const gchar *name;
	int len;
	gchar replace;
};

static struct literal literals[] = {
	{"quot;", 5, '\"'},
	{"nbsp;", 5, ' '},
	{"lt;",   3, '<'},
	{"gt;",   3, '>'}
};

#define NUM_LITERALS (sizeof(literals) / sizeof(literals[0]))

/* Convert HTML text to plain text by removing tags and substituting literals */
gchar *
html_to_plain_text(const gchar *htmltext)
{
    /* Reserve space for the main text */
    int len = strlen(htmltext);
    GString *text = g_string_sized_new(len);

    /* Scan the text, removing markup */
    gboolean white = FALSE;
    const gchar *p1 = htmltext;
    const gchar *p2 = p1 + len;
    while(p1 < p2) {
        /* Look for an HTML tag */
        if(*p1 == '<') {
            if(*(p1 + 1) == '!') {
                p1 = strstr(p1, "-->");
                if(p1 != NULL)
                    p1 += 2;
                else
                    p1 = p2;
            } else {
                while(p1 < p2 && *p1 != '>')
                    p1++;
            }
            g_assert(*p1 == '>');
            
            /* Add a carriage return for appropriate markup */
            if(p1 >= htmltext + 2 && (strncmp(p1, "<p", 2) == 0
               || strncmp(p1, "<P", 2) == 0)) {
                g_string_append_c(text, '\n');
                white = TRUE;
            }
        } else if(*p1 == '&') {
            /* Scan for a known literal */
            gboolean found = FALSE;
            int i = 0;
            while(!found && i < NUM_LITERALS) {
                if(strncmp(p1 + 1, literals[i].name, literals[i].len) == 0)
                    found = TRUE;
                if(!found)
                    i++;
            }
			/* Replace the literal */
            if(found) {
                g_string_append_c(text, literals[i].replace);
                p1 += literals[i].len;
            } else
                g_string_append_c(text, *p1);
            white = FALSE;
        } else if(isspace(*p1)) {
            if(!white)
                g_string_append_c(text, ' ');
            white = TRUE;
        } else {
            g_string_append_c(text, *p1);
            white = FALSE;
        }
        p1++;
    }

    gchar *retval = g_strdup(text->str);
    g_string_free(text, TRUE);
    return retval;
}
#endif