/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * new-i7
 * Copyright (C) P. F. Chimento 2010 <philip.chimento@gmail.com>
 * 
 * new-i7 is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * new-i7 is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <goocanvas.h>
#include "skein-view.h"
#include "skein.h"
#include "node.h"

typedef struct _I7SkeinViewPrivate
{
	I7Skein *skein;
	gulong layout_handler;
} I7SkeinViewPrivate;

#define I7_SKEIN_VIEW_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE((o), I7_TYPE_SKEIN_VIEW, I7SkeinViewPrivate))
#define I7_SKEIN_VIEW_USE_PRIVATE(o,n) I7SkeinViewPrivate *n = I7_SKEIN_VIEW_PRIVATE(o)

G_DEFINE_TYPE(I7SkeinView, i7_skein_view, GOO_TYPE_CANVAS);

static void
on_item_created(I7SkeinView *view, GooCanvasItem *item, GooCanvasItemModel *model, I7Skein **skeinptr)
{
	if(I7_IS_NODE(model)) {
		i7_node_calculate_size(I7_NODE(model), GOO_CANVAS_ITEM_MODEL(*skeinptr), GOO_CANVAS(view));
		/* TODO connect button press signals */
	}
}

static void
i7_skein_view_init(I7SkeinView *self)
{
	I7_SKEIN_VIEW_USE_PRIVATE(self, priv);
	priv->skein = NULL;
	priv->layout_handler = 0;

	g_signal_connect_after(self, "item-created", G_CALLBACK(on_item_created), &priv->skein);
}

static void
i7_skein_view_finalize(GObject *self)
{
	I7_SKEIN_VIEW_USE_PRIVATE(self, priv);

	if(priv->skein) {
		g_signal_handler_disconnect(priv->skein, priv->layout_handler);
		g_object_unref(priv->skein);
	}

	G_OBJECT_CLASS(i7_skein_view_parent_class)->finalize(self);
}

static void
i7_skein_view_class_init(I7SkeinViewClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = i7_skein_view_finalize;

	/* Add private data */
	g_type_class_add_private(klass, sizeof(I7SkeinViewPrivate));
}

/* PUBLIC FUNCTIONS */

GtkWidget *
i7_skein_view_new(void)
{
	return g_object_new(I7_TYPE_SKEIN_VIEW, NULL);
}

void 
i7_skein_view_set_skein(I7SkeinView *self, I7Skein *skein)
{
	g_return_if_fail(self || I7_IS_SKEIN_VIEW(self));
	g_return_if_fail(skein || I7_IS_SKEIN(skein));
	I7_SKEIN_VIEW_USE_PRIVATE(self, priv);

	if(priv->skein == skein)
		return;

	if(priv->skein) {
		g_signal_handler_disconnect(priv->skein, priv->layout_handler);
		g_object_unref(priv->skein);
	}
	priv->skein = skein;

	if(skein == NULL) {
		goo_canvas_set_root_item_model(GOO_CANVAS(self), NULL);
		return;
	}
	
	goo_canvas_set_root_item_model(GOO_CANVAS(self), GOO_CANVAS_ITEM_MODEL(skein));
	g_object_ref(skein);
	priv->layout_handler = g_signal_connect(skein, "needs-layout", G_CALLBACK(i7_skein_draw), self);
	i7_skein_draw(skein, GOO_CANVAS(self));
}

I7Skein *
i7_skein_view_get_skein(I7SkeinView *self)
{
	g_return_val_if_fail(self || I7_IS_SKEIN_VIEW(self), NULL);
	I7_SKEIN_VIEW_USE_PRIVATE(self, priv);
	return priv->skein;
}
