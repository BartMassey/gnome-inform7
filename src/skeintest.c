/*
 * Compile me with:
gcc -o skeintest skeintest.c skein.c node.c `pkg-config --cflags --libs gtk+-2.0 gdk-pixbuf-2.0 libxml-2.0 cairo goocanvas`
 */
 
#include <gtk/gtk.h>
#include <goocanvas.h>
#include "skein.h"

int
main(int argc, char **argv)
{
	GError *error = NULL;
	
	gtk_init(&argc, &argv);
	
	/* Create widgets */
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget *canvas = goo_canvas_new();
	
	/* Configure widgets */
	gtk_window_set_default_size(GTK_WINDOW(window), 400, 600);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	
	/* Assemble widgets */
	gtk_container_add(GTK_CONTAINER(scroll), canvas);
	gtk_container_add(GTK_CONTAINER(window), scroll);
	
	/* Read skein */
	I7Skein *skein = i7_skein_new();
	if(!i7_skein_load(skein, "/home/fliep/Documents/writing/if/Poop.inform/Skein.skein", &error)) {
		g_printerr("Error: %s\n", error->message);
		g_error_free(error);
	}
	
	g_object_set(skein, 
		"vertical-spacing", 75.0, 
		"unlocked-color", "#6865FF",
		NULL);
	
	goo_canvas_set_root_item_model(GOO_CANVAS(canvas), i7_skein_get_root_group(skein));
	i7_skein_layout(skein, GOO_CANVAS(canvas));
	
	/* Display widgets */
	gtk_widget_show_all(window);
	gtk_main();
	
	g_object_unref(skein);
}
