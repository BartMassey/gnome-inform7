/*
 * Compile me with:
gcc -o skeintest skeintest.c skein.c node.c `pkg-config --cflags --libs gtk+-2.0 gdk-pixbuf-2.0 libxml-2.0 cairo goocanvas`
 */
 
#include <gtk/gtk.h>
#include <goocanvas.h>
#include "skein.h"
#include "skein-view.h"

typedef struct {
	GtkWidget *view;
	I7Skein *skein;
} Widgets;

static void
hspacing_changed(GtkRange *hspacing, Widgets *w)
{
	g_object_set(w->skein, "horizontal-spacing", gtk_range_get_value(hspacing), NULL);
}

static void
vspacing_changed(GtkRange *vspacing, Widgets *w)
{
	g_object_set(w->skein, "vertical-spacing", gtk_range_get_value(vspacing), NULL);
}

int
main(int argc, char **argv)
{
	GError *error = NULL;
	
	gtk_init(&argc, &argv);
	
	/* Create widgets */
	Widgets *w = g_slice_new0(Widgets);
	w->skein = i7_skein_new();
	w->view = i7_skein_view_new();
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
	GtkWidget *hspacing = gtk_hscale_new_with_range(20.0, 100.0, 1.0);
	GtkWidget *vspacing = gtk_hscale_new_with_range(20.0, 100.0, 1.0);
	
	/* Configure widgets */
	gtk_window_set_default_size(GTK_WINDOW(window), 400, 600);
	gtk_range_set_value(GTK_RANGE(hspacing), 40.0);
	gtk_range_set_value(GTK_RANGE(vspacing), 75.0);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(hspacing, "value-changed", G_CALLBACK(hspacing_changed), w);
	g_signal_connect(vspacing, "value-changed", G_CALLBACK(vspacing_changed), w);
	g_object_set(w->skein, 
		"vertical-spacing", 75.0, 
		"unlocked-color", "#6865FF",
		NULL);
	
	/* Assemble widgets */
	gtk_box_pack_start(GTK_BOX(hbox), hspacing, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vspacing, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(scroll), w->view);
	gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	
	/* Read skein */
	if(!i7_skein_load(w->skein, "/home/fliep/Documents/if/Blood.inform/Skein.skein", &error)) {
		g_printerr("Error: %s\n", error->message);
		g_error_free(error);
	}

	i7_skein_view_set_skein(I7_SKEIN_VIEW(w->view), w->skein);
	
	/* Display widgets */
	gtk_widget_show_all(window);
	gtk_main();
	
	g_object_unref(w->skein);
	g_slice_free(Widgets, w);

	return 0;
}
