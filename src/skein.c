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

#include <errno.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <goocanvas.h>

#include "skein.h"
#include "node.h"

typedef struct _I7SkeinPrivate
{
	I7Node *root;
	I7Node *current;
	I7Node *played;
    gboolean modified;
    
    gdouble hspacing;
    gdouble vspacing;
    GdkColor locked;
    GdkColor unlocked;

	GooCanvasLineDash *unlocked_dash;
} I7SkeinPrivate;

#define I7_SKEIN_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), I7_TYPE_SKEIN, I7SkeinPrivate))
#define I7_SKEIN_USE_PRIVATE I7SkeinPrivate *priv = I7_SKEIN_PRIVATE(self)

enum 
{
	NEEDS_LAYOUT,
	NODE_ACTIVATE,
	NODE_MENU_POPUP,
	TRANSCRIPT_THREAD_CHANGED,
	SHOW_NODE,
	LAST_SIGNAL
};

enum
{
	PROP_0,
	PROP_HORIZONTAL_SPACING,
	PROP_VERTICAL_SPACING,
	PROP_LOCKED_COLOR,
	PROP_UNLOCKED_COLOR
};

static guint i7_skein_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(I7Skein, i7_skein, GOO_TYPE_CANVAS_GROUP_MODEL);

/* SIGNAL HANDLERS */

static void
on_node_text_notify(I7Node *node, GParamSpec *pspec, I7Skein *self)
{
	I7_SKEIN_USE_PRIVATE;
	g_signal_emit_by_name(self, "needs-layout");
	priv->modified = TRUE;
}

static void
on_node_label_notify(I7Node *node, GParamSpec *pspec, I7Skein *self)
{
	if(i7_node_has_label(node))
		i7_skein_lock(self, i7_skein_get_thread_bottom(self, node));
	on_node_text_notify(node, pspec, self);
}

static void
on_node_color_notify(I7Node *node, GParamSpec *pspec, I7Skein *self)
{
	I7_SKEIN_USE_PRIVATE;
	priv->modified = TRUE;
}

static void
node_listen(I7Skein *self, I7Node *node)
{
	g_signal_connect(node, "notify::command", G_CALLBACK(on_node_text_notify), self);
	g_signal_connect(node, "notify::label", G_CALLBACK(on_node_label_notify), self);
	g_signal_connect(node, "notify::text-expected", G_CALLBACK(on_node_color_notify), self);
}

/* TYPE SYSTEM */

static void
i7_skein_init(I7Skein *self)
{
	I7_SKEIN_USE_PRIVATE;
    priv->root = i7_node_new(_("- start -"), "", "", "", FALSE, FALSE, 0, GOO_CANVAS_ITEM_MODEL(self));
	node_listen(self, priv->root);
    priv->current = priv->root;
    priv->played = priv->root;
    priv->modified = TRUE;
    priv->unlocked_dash = goo_canvas_line_dash_new(2, 5.0, 5.0);
    
    priv->hspacing = 40.0;
    priv->vspacing = 40.0;
    gdk_color_parse("black", &priv->locked);
    gdk_color_parse("black", &priv->unlocked);
    
    /* Load the "differs badge" */
    GError *err = NULL;
    GtkIconTheme *icon_theme = gtk_icon_theme_get_default();
    self->differs_badge = gtk_icon_theme_load_icon(icon_theme, "inform7-skein-differs-badge", 16, 0, &err);
    if(!self->differs_badge) {
    	g_warning("Could not load differs badge: %s", err->message);
    	g_error_free(err);
    }
}

static void
i7_skein_set_property(GObject *self, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    I7_SKEIN_USE_PRIVATE;
    
    switch(prop_id) {
		case PROP_HORIZONTAL_SPACING:
			priv->hspacing = g_value_get_double(value);
			g_object_notify(self, "horizontal-spacing");
			g_signal_emit_by_name(self, "needs-layout");
			break;
		case PROP_VERTICAL_SPACING:
			priv->vspacing = g_value_get_double(value);
			g_object_notify(self, "vertical-spacing");
			g_signal_emit_by_name(self, "needs-layout");
			break;
		case PROP_LOCKED_COLOR:
			gdk_color_parse(g_value_get_string(value), &priv->locked);
			g_object_notify(self, "locked-color");
			/* TODO: Change the color of all the locked threads */
			break;
		case PROP_UNLOCKED_COLOR:
			gdk_color_parse(g_value_get_string(value), &priv->unlocked);
			g_object_notify(self, "unlocked-color");
			/* TODO: Change the color of all the unlocked threads */
			break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, prop_id, pspec);
    }
}

static void
i7_skein_get_property(GObject *self, guint prop_id, GValue *value, GParamSpec *pspec)
{
    I7_SKEIN_USE_PRIVATE;
    
    switch(prop_id) {
		case PROP_HORIZONTAL_SPACING:
			g_value_set_double(value, priv->hspacing);
			break;
		case PROP_VERTICAL_SPACING:
			g_value_set_double(value, priv->vspacing);
			break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, prop_id, pspec);
    }
}

static void
i7_skein_finalize(GObject *self)
{
	I7_SKEIN_USE_PRIVATE;
	
	g_object_unref(priv->root);
	goo_canvas_line_dash_unref(priv->unlocked_dash);
	g_object_unref(I7_SKEIN(self)->differs_badge);
	
	G_OBJECT_CLASS(i7_skein_parent_class)->finalize(self);
}

static void
i7_skein_class_init(I7SkeinClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS(klass);
	object_class->set_property = i7_skein_set_property;
	object_class->get_property = i7_skein_get_property;
	object_class->finalize = i7_skein_finalize;

	/* Signals */
	/* needs-layout - skein requests view to calculate new sizes for its nodes */
	i7_skein_signals[NEEDS_LAYOUT] = g_signal_new("needs-layout",
		G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_NO_RECURSE,
		G_STRUCT_OFFSET(I7SkeinClass, needs_layout), NULL, NULL,
		g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	/* node-activate - user double-clicked on a node */
	i7_skein_signals[NODE_ACTIVATE] = g_signal_new("node-activate",
		G_OBJECT_CLASS_TYPE(klass), 0,
		G_STRUCT_OFFSET(I7SkeinClass, node_activate), NULL, NULL,
		g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, I7_TYPE_NODE);
	/* node-popup-menu - user right-clicked on a node */
	i7_skein_signals[NODE_MENU_POPUP] = g_signal_new("node-menu-popup",
		G_OBJECT_CLASS_TYPE(klass), 0,
		G_STRUCT_OFFSET(I7SkeinClass, node_menu_popup), NULL, NULL,
		g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, I7_TYPE_NODE);
	/* transcript-thread-changed */
	i7_skein_signals[TRANSCRIPT_THREAD_CHANGED] = g_signal_new("transcript-thread-changed",
	    G_OBJECT_CLASS_TYPE(klass), 0,
	    G_STRUCT_OFFSET(I7SkeinClass, transcript_thread_changed), NULL, NULL,
	    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	/* show-node - skein requests its view to display a certain node */
	i7_skein_signals[SHOW_NODE] = g_signal_new("show-node",
	    G_OBJECT_CLASS_TYPE(klass), 0,
	    G_STRUCT_OFFSET(I7SkeinClass, show_node), NULL, NULL,
	    g_cclosure_marshal_VOID__UINT_POINTER, G_TYPE_NONE, 2, 
	    G_TYPE_UINT, I7_TYPE_NODE);
	    
	/* Install properties */
	GParamFlags flags = G_PARAM_CONSTRUCT | G_PARAM_LAX_VALIDATION | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB;
	/* SUCKY DEBIAN replace with G_PARAM_STATIC_STRINGS */
	g_object_class_install_property(object_class, PROP_HORIZONTAL_SPACING,
		g_param_spec_double("horizontal-spacing", _("Horizontal spacing"),
		    _("Pixels of horizontal space between skein branches"),
		    20.0, 100.0, 40.0, G_PARAM_READWRITE | flags));
	g_object_class_install_property(object_class, PROP_VERTICAL_SPACING,
		g_param_spec_double("vertical-spacing", _("Vertical spacing"),
			_("Pixels of vertical space between skein items"),
			20.0, 100.0, 40.0, G_PARAM_READWRITE | flags));
	g_object_class_install_property(object_class, PROP_LOCKED_COLOR,
		g_param_spec_string("locked-color", _("Locked color"),
			_("Color of locked threads"),
			"black", G_PARAM_WRITABLE | flags));
	g_object_class_install_property(object_class, PROP_UNLOCKED_COLOR,
		g_param_spec_string("unlocked-color", _("Unlocked color"),
			_("Color of unlocked threads"),
			"black", G_PARAM_WRITABLE | flags));

	/* Add private data */
	g_type_class_add_private(klass, sizeof(I7SkeinPrivate));
}

I7Skein *
i7_skein_new(void)
{
	return g_object_new(I7_TYPE_SKEIN, NULL);
}

GQuark
i7_skein_error_quark(void)
{
	return g_quark_from_static_string("i7-skein-error-quark");
}

I7Node *
i7_skein_get_root_node(I7Skein *self)
{
	I7_SKEIN_USE_PRIVATE;
    return priv->root;
}

I7Node *
i7_skein_get_current_node(I7Skein *self)
{
	I7_SKEIN_USE_PRIVATE;
    return priv->current;
}

void
i7_skein_set_current_node(I7Skein *self, I7Node *node)
{
	I7_SKEIN_USE_PRIVATE;
    priv->current = node;
    priv->modified = TRUE;
}

gboolean
i7_skein_is_node_in_current_thread(I7Skein *self, I7Node *node)
{
	I7_SKEIN_USE_PRIVATE;
    return i7_node_in_thread(node, priv->current);
}

I7Node *
i7_skein_get_played_node(I7Skein *self)
{
	I7_SKEIN_USE_PRIVATE;
    return priv->played;
}

/* Search an xmlNode's properties for a certain one and return its content.
String must be freed. Return NULL if not found */
static gchar *
get_property_from_node(xmlNode *node, const gchar *propertyname)
{
    xmlAttr *props;
    for(props = node->properties; props; props = props->next)
        if(xmlStrEqual(props->name, (const xmlChar *)propertyname))
            return g_strdup((gchar *)props->children->content);
    return NULL;
}

/* Read the ObjectiveC "YES" and "NO" into a boolean, or return default_val if
 content is malformed */
static gboolean
get_boolean_from_content(xmlNode *node, gboolean default_val)
{
    if(xmlStrEqual(node->children->content, (xmlChar *)"YES"))
        return TRUE;
    else if(xmlStrEqual(node->children->content, (xmlChar *)"NO"))
        return FALSE;
    else
        return default_val;
}

static gchar *
get_text_content_from_node(xmlNode *node)
{
    if(node->children)
        if(xmlStrEqual(node->children->name, (xmlChar *)"text"))
            return g_strdup((gchar *)node->children->content);
    return NULL;
}

gboolean
i7_skein_load(I7Skein *self, const gchar *filename, GError **error)
{
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
	
	I7_SKEIN_USE_PRIVATE;
	
    xmlDoc *xmldoc = xmlParseFile(filename);
    if(!xmldoc) {
		xmlErrorPtr xml_error = xmlGetLastError();
		if(error)
			*error = g_error_new_literal(I7_SKEIN_ERROR, I7_SKEIN_ERROR_XML, g_strdup(xml_error->message));
        return FALSE;
    }
    
    /* Get the top XML node */
    xmlNode *top = xmlDocGetRootElement(xmldoc);
    if(!xmlStrEqual(top->name, (xmlChar *)"Skein")) {
		if(error)
			*error = g_error_new(I7_SKEIN_ERROR, I7_SKEIN_ERROR_BAD_FORMAT, _("<Skein> element not found."));
        goto fail;
    }
    
    /* Get the ID of the root node */
    gchar *root_id = get_property_from_node(top, "rootNode");
    if(!root_id) {
		if(error)
			*error = g_error_new(I7_SKEIN_ERROR, I7_SKEIN_ERROR_BAD_FORMAT, _("rootNode attribute not found."));
        goto fail;
    }

    /* Get all the item nodes and the ID of the active node */
    GHashTable *nodetable = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    gchar *active_id = NULL;
    xmlNode *item;
    /* Create a node object for each of the XML item nodes */
    for(item = top->children; item; item = item->next) {
        if(xmlStrEqual(item->name, (xmlChar *)"activeNode"))
            active_id = get_property_from_node(item, "nodeId");
        else if(xmlStrEqual(item->name, (xmlChar *)"item")) {
            gchar *id, *command = NULL, *label = NULL;
            gchar *transcript = NULL, *expected = NULL;
            gboolean unlocked = TRUE;
            int score = 0;
            
            xmlNode *child;
            for(child = item->children; child; child = child->next) {
				/* Ignore "played" and "changed"; these are calculated */
                if(xmlStrEqual(child->name, (xmlChar *)"command"))
                    command = get_text_content_from_node(child);
                else if(xmlStrEqual(child->name, (xmlChar *)"annotation"))
                    label = get_text_content_from_node(child);
                else if(xmlStrEqual(child->name, (xmlChar *)"result"))
                    transcript = get_text_content_from_node(child);
                else if(xmlStrEqual(child->name, (xmlChar *)"commentary"))
                    expected = get_text_content_from_node(child);
                else if(xmlStrEqual(child->name, (xmlChar *)"temporary")) {
                    unlocked = get_boolean_from_content(child, TRUE);
                    gchar *trash = get_property_from_node(child, "score");
                    sscanf(trash, "%d", &score);
                    g_free(trash);
                }
            }
            id = get_property_from_node(item, "nodeId"); /* freed by table */
            
            I7Node *skein_node = i7_node_new(command, label, transcript, expected, FALSE, !unlocked, score, GOO_CANVAS_ITEM_MODEL(self));
			node_listen(self, skein_node);
            g_hash_table_insert(nodetable, id, skein_node);
            g_free(command);
            g_free(label);
            g_free(transcript);
            g_free(expected);
        }
    }
    
    /* Loop through the item nodes again, adding the children to each parent */
    for(item = top->children; item; item = item->next) {
        if(!xmlStrEqual(item->name, (xmlChar *)"item"))
            continue;
        gchar *id = get_property_from_node(item, "nodeId");
        I7Node *parent_node = g_hash_table_lookup(nodetable, id);
        xmlNode *child;
        for(child = item->children; child; child = child->next) {
            if(!xmlStrEqual(child->name, (xmlChar *)"children"))
                continue;
            xmlNode *list;
            for(list = child->children; list; list = list->next) {
                if(!xmlStrEqual(list->name, (xmlChar *)"child"))
                    continue;
                gchar *child_id = get_property_from_node(list, "nodeId");
                g_node_append(parent_node->gnode, I7_NODE(g_hash_table_lookup(nodetable, child_id))->gnode);
                g_free(child_id);
            }
        }
        g_free(id);
    }

    /* Discard the current skein and replace with the new */
    g_object_unref(priv->root);
    priv->root = I7_NODE(g_hash_table_lookup(nodetable, root_id));
    priv->played = I7_NODE(g_hash_table_lookup(nodetable, active_id));
    priv->current = priv->root;

	/* Calculate which nodes should be "played" */
	GNode *gnode = priv->root->gnode;
	do {
		i7_node_set_played(I7_NODE(gnode->data), TRUE);
		for(gnode = gnode->children; gnode; gnode = gnode->next) {
			if(i7_node_in_thread(I7_NODE(gnode->data), priv->played))
			    break;
		}
	} while(gnode);
	
    g_signal_emit_by_name(self, "needs-layout");
    priv->modified = FALSE;
    
    g_free(root_id);
    g_free(active_id);    
    g_hash_table_destroy(nodetable);

	return TRUE;
fail:
    xmlFreeDoc(xmldoc);
	return FALSE;
}

gboolean
node_write_xml(I7Node *node, FILE *fp)
{
	gchar *xml = i7_node_get_xml(node);
	fprintf(fp, xml);
	g_free(xml);
    return FALSE; /* Do not stop the traversal */
}
    
gboolean
i7_skein_save(I7Skein *self, const gchar *filename, GError **error)
{
	g_return_val_if_fail(FALSE, error == NULL || *error == NULL);
	
	I7_SKEIN_USE_PRIVATE;
	
    FILE *skeinfile = fopen(filename, "w");
    if(!skeinfile) {
		if(error)
			*error = g_error_new(G_FILE_ERROR, g_file_error_from_errno(errno),
			    _("Error saving file '%s': %s"), filename, g_strerror(errno));
        return FALSE;
    }
    
    fprintf(skeinfile,
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<Skein rootNode=\"%s\" "
            "xmlns=\"http://www.logicalshift.org.uk/IF/Skein\">\n"
            "  <generator>GNOME Inform 7</generator>\n"
            "  <activeNode nodeId=\"%s\"/>\n",
            i7_node_get_unique_id(priv->root),
            i7_node_get_unique_id(priv->current));
    g_node_traverse(priv->root->gnode, G_PRE_ORDER, G_TRAVERSE_ALL, -1, (GNodeTraverseFunc)node_write_xml, skeinfile);
    fprintf(skeinfile, "</Skein>\n");
    fclose(skeinfile);
    priv->modified = FALSE;

	return TRUE;
}

void
i7_skein_reset(I7Skein *self, gboolean current)
{
	I7_SKEIN_USE_PRIVATE;
    if(current)
        priv->current = priv->root;
    priv->played = priv->root;
    /* TODO modify the node colors */
    priv->modified = TRUE;
}

static guint
rgba_from_gdk_color(GdkColor *color)
{
	return (color->red >> 8) << 24
		| (color->green >> 8) << 16
		| (color->blue >> 8) << 8
		| 0xFF;
}

static void
draw_tree(I7Skein *self, I7Node *node, GooCanvas *canvas)
{
	I7_SKEIN_USE_PRIVATE;
	
	/* Draw a line from the node to its parent */
	if(node->gnode->parent) {
		/* Calculate the coordinates */
		gdouble nodex = i7_node_get_x(node);
		gdouble destx = i7_node_get_x(I7_NODE(node->gnode->parent->data));
		gdouble nodey = (gdouble)(g_node_depth(node->gnode) - 1.0) * priv->vspacing;
		gdouble desty = nodey - priv->vspacing;
		
		if(node->tree_item)
			goo_canvas_item_model_remove_child(GOO_CANVAS_ITEM_MODEL(self), goo_canvas_item_model_find_child(GOO_CANVAS_ITEM_MODEL(self), node->tree_item));
		
		if(nodex == destx) {
			node->tree_item = goo_canvas_polyline_model_new_line(GOO_CANVAS_ITEM_MODEL(self),
				destx, desty, nodex, nodey,
				NULL);
		} else {
			node->tree_item = goo_canvas_polyline_model_new(GOO_CANVAS_ITEM_MODEL(self), FALSE, 4,
				destx, desty,
				destx, desty + 0.2 * priv->vspacing,
				nodex, nodey - 0.2 * priv->vspacing,
				nodex, nodey,
				NULL);
		}
		
		if(i7_node_get_locked(node))
			g_object_set(node->tree_item,
				"stroke-color-rgba", rgba_from_gdk_color(&priv->locked),
				"line-width", i7_node_in_thread(node, priv->current)? 4.0 : 1.5,
				NULL);
		else
			g_object_set(node->tree_item,
				"stroke-color-rgba", rgba_from_gdk_color(&priv->unlocked),
				"line-dash", priv->unlocked_dash,
				"line-width", i7_node_in_thread(node, priv->current)? 4.0 : 1.5,
				NULL);
		
		goo_canvas_item_model_lower(node->tree_item, NULL); /* put at bottom */
	}
	
	/* Draw the children's lines to this node */
	int i;
	for(i = 0; i < g_node_n_children(node->gnode); i++) {
        I7Node *child = g_node_nth_child(node->gnode, i)->data;
        draw_tree(self, child, canvas);
    }
}
   
void
i7_skein_draw(I7Skein *self, GooCanvas *canvas)
{
	I7_SKEIN_USE_PRIVATE;
	
	i7_node_layout(priv->root, GOO_CANVAS_ITEM_MODEL(self), canvas, 0.0);
	
	gdouble treewidth = i7_node_get_tree_width(priv->root, GOO_CANVAS_ITEM_MODEL(self), canvas);
    draw_tree(self, priv->root, canvas);

    goo_canvas_set_bounds(canvas, 
    	-treewidth * 0.5 - priv->hspacing, -(priv->vspacing) * 0.5,
    	treewidth * 0.5 + priv->hspacing, g_node_max_height(priv->root->gnode) * priv->vspacing);
}

void
i7_skein_new_command(I7Skein *self, const gchar *command)
{
	I7_SKEIN_USE_PRIVATE;
	
    gboolean node_added = FALSE;
    gchar *node_command = g_strescape(command, "\"");

    /* Is there a child node with the same line? */
	I7Node *node = NULL;
    GNode *gnode = priv->current->gnode->children;
    while(gnode != NULL) {
		gchar *cmp_command = i7_node_get_command(I7_NODE(gnode->data));
        /* Special case: NULL is treated as "" */
        if((strlen(cmp_command) == 0 && (command == NULL || strlen(command) == 0)) || (strcmp(cmp_command, command) == 0)) {
			g_free(cmp_command);
			node = gnode->data;
            break;
		}
        gnode = gnode->next;
    }
    if(node == NULL) { 
		/* Wasn't found, create new node */
        node = i7_node_new(node_command, "", "", "", TRUE, FALSE, 0, GOO_CANVAS_ITEM_MODEL(self));
		node_listen(self, node);
        g_node_append(priv->current->gnode, node->gnode);
        node_added = TRUE;
    }
    g_free(node_command);
    
    /* Make this the new current node */
    priv->current = node;
    priv->played = node;

    /* Send signals */
    if(node_added) {
        g_signal_emit_by_name(self, "needs-layout");
    } else {
    	/* TODO: change node colors */
		;
    }
    g_signal_emit_by_name(self, "show-node", GOT_COMMAND, node);
    priv->modified = TRUE;
}

/* Find the next node to use. Return TRUE if found, and if so, store a
newly-allocated copy of its command text in line.*/
gboolean
i7_skein_next_command(I7Skein *self, gchar **line)
{
	I7_SKEIN_USE_PRIVATE;
	
    if(!g_node_is_ancestor(priv->played->gnode, priv->current->gnode))
        return FALSE;

    I7Node *next = priv->current;
    while(next->gnode->parent != priv->played->gnode)
        next = next->gnode->parent->data;
    priv->played = next;
	gchar *temp = i7_node_get_command(next);
    *line = g_strcompress(temp);
	g_free(temp);
    g_signal_emit_by_name(self, "show-node", GOT_COMMAND, next);
    return TRUE;
}

/* Get a list of the commands from the play pointer to the current pointer,
without resetting either of them */
GSList *
i7_skein_get_commands(I7Skein *self)
{
	I7_SKEIN_USE_PRIVATE;
	
    GSList *commands = NULL;
    GNode *pointer = priv->played->gnode;
    while(g_node_is_ancestor(pointer, priv->current->gnode)) {
        I7Node *next = priv->current;
        while(next->gnode->parent != pointer)
            next = next->gnode->parent->data;
        pointer = next->gnode;
		gchar *skein_command = i7_node_get_command(next);
        commands = g_slist_prepend(commands, g_strcompress(skein_command));
		g_free(skein_command);
        g_signal_emit_by_name(self, "show-node", GOT_COMMAND, next);
    }
    commands = g_slist_reverse(commands);
    return commands;
}

/* Update the status of the last played node */
void
i7_skein_update_after_playing(I7Skein *self, const gchar *transcript)
{
	I7_SKEIN_USE_PRIVATE;
	
    i7_node_set_played(priv->played, TRUE);
    if(strlen(transcript)) {
        i7_node_set_transcript_text(priv->played, transcript);
        g_signal_emit_by_name(self, "show-node", GOT_TRANSCRIPT, priv->played);
    }
}

gboolean
i7_skein_get_command_from_history(I7Skein *self, gchar **command, int history)
{
	I7_SKEIN_USE_PRIVATE;
	
    /* Find the node to return the command from */
    GNode *gnode = priv->current->gnode;
    while(--history > 0) {
        gnode = gnode->parent;
        if(gnode == NULL)
            return FALSE;
    }
    /* Don't return the root node */
    if(G_NODE_IS_ROOT(gnode))
        return FALSE;
    
    *command = g_strdelimit(i7_node_get_command(I7_NODE(gnode->data)), "\b\f\n\r\t", ' ');
    return TRUE;
}

/* Add an empty node as the child of node */
I7Node *
i7_skein_add_new(I7Skein *self, I7Node *node)
{
	I7_SKEIN_USE_PRIVATE;
	
    I7Node *newnode = i7_node_new("", "", "", "", FALSE, FALSE, 0, GOO_CANVAS_ITEM_MODEL(self));
	node_listen(self, newnode);
    g_node_append(node->gnode, newnode->gnode);
    
    g_signal_emit_by_name(self, "needs-layout");
    priv->modified = TRUE;
    
    return newnode;
}

I7Node *
i7_skein_add_new_parent(I7Skein *self, I7Node *node)
{
	I7_SKEIN_USE_PRIVATE;
	
    I7Node *newnode = i7_node_new("", "", "", "", FALSE, FALSE, 0, GOO_CANVAS_ITEM_MODEL(self));
	node_listen(self, newnode);
    g_node_insert(node->gnode->parent, g_node_child_position(node->gnode->parent, node->gnode), newnode->gnode);
    g_node_unlink(node->gnode);
    g_node_append(newnode->gnode, node->gnode);
    
    g_signal_emit_by_name(self, "needs-layout");
    priv->modified = TRUE;
    
    return newnode;
}

gboolean
i7_skein_remove_all(I7Skein *self, I7Node *node)
{
	I7_SKEIN_USE_PRIVATE;
	
    if(G_NODE_IS_ROOT(node->gnode))
		return FALSE;
	
	/* FIXME something's wrong here */
    gboolean in_current = i7_skein_is_node_in_current_thread(self, node);
    g_object_unref(node);
    
    if(in_current) {
        priv->current = priv->root;
        priv->played = priv->root;
    }
        
    g_signal_emit_by_name(self, "needs-layout");
    priv->modified = TRUE;
    return TRUE;
}

gboolean
i7_skein_remove_single(I7Skein *self, I7Node *node)
{
	I7_SKEIN_USE_PRIVATE;
	
    if(G_NODE_IS_ROOT(node->gnode))
		return FALSE;

    gboolean in_current = i7_skein_is_node_in_current_thread(self, node);

	if(!G_NODE_IS_LEAF(node->gnode)) {
		int i;
    	for(i = g_node_n_children(node->gnode) - 1; i >= 0; i--) {
			GNode *iter = g_node_nth_child(node->gnode, i);
			g_node_unlink(iter);
			g_node_insert_after(node->gnode->parent, node->gnode, iter);
		}
	}	
    g_object_unref(node);
	
    if(in_current) {
        priv->current = priv->root;
        priv->played = priv->root;
    }
    
    g_signal_emit_by_name(self, "needs-layout");
    priv->modified = TRUE;
    return TRUE;
}

void
i7_skein_lock(I7Skein *self, I7Node *node)
{
	GNode *iter;
	for(iter = node->gnode; iter; iter = iter->parent)
        i7_node_set_locked(I7_NODE(iter->data), TRUE);
    
    /* TODO change thread colors */
    I7_SKEIN_PRIVATE(self)->modified = TRUE;
}

static void
i7_skein_unlock_recurse(I7Skein *self, I7Node *node)
{
    i7_node_set_locked(node, FALSE);
	GNode *iter;
    for(iter = node->gnode->children; iter; iter = iter->next)
        i7_skein_unlock(self, I7_NODE(iter->data));
    
    /* TODO change thread colors */
}

void
i7_skein_unlock(I7Skein *self, I7Node *node)
{
	I7_SKEIN_USE_PRIVATE;
	i7_skein_unlock_recurse(self, node);
    priv->modified = TRUE;
}

static void
i7_skein_trim_recurse(I7Skein *self, I7Node *node, int min_score)
{
	I7_SKEIN_USE_PRIVATE;
	
    int i = 0;
    while(i < g_node_n_children(node->gnode)) {
        I7Node *child = g_node_nth_child(node->gnode, i)->data;
        if(i7_node_get_locked(child)) {
            i7_skein_trim_recurse(self, child, -1);
            i++;
        } else {
			if(i7_skein_is_node_in_current_thread(self, child)) {
                priv->current = priv->root;
                priv->played = priv->root;
            }
            
            if(i7_skein_remove_all(self, child) == FALSE)
                i++;
        }
    }
}

void
i7_skein_trim(I7Skein *self, I7Node *node, int min_score)
{
	I7_SKEIN_USE_PRIVATE;
	i7_skein_trim_recurse(self, node, min_score);
    g_signal_emit_by_name(self, "needs-layout");
    priv->modified = TRUE;
}

static void
get_labels(GNode *gnode, GSList **labels)
{
	I7Node *node = gnode->data;
	if(i7_node_has_label(node)) {
		NodeLabel *nodelabel = g_slice_new0(NodeLabel);
		nodelabel->label = i7_node_get_label(node);
		nodelabel->node = node;
		*labels = g_slist_prepend(*labels, nodelabel);
	}
	g_node_children_foreach(gnode, G_TRAVERSE_ALL, (GNodeForeachFunc)get_labels, labels);
}

GSList *
i7_skein_get_labels(I7Skein *self)
{
	I7_SKEIN_USE_PRIVATE;
	GSList *labels = NULL;
	get_labels(priv->root->gnode, &labels);
	labels = g_slist_sort_with_data(labels, (GCompareDataFunc)strcmp, NULL);
	return labels;
}

static gboolean
has_labels(I7Node *node)
{
    if(i7_node_has_label(node))
        return TRUE;

	GNode *iter;
    for(iter = node->gnode->children; iter; iter = iter->next)
        if(has_labels(iter->data))
            return TRUE;
    return FALSE;
}

gboolean
i7_skein_has_labels(I7Skein *self)
{
	I7_SKEIN_USE_PRIVATE;
    return has_labels(priv->root);
}

void
i7_skein_bless(I7Skein *self, I7Node *node, gboolean all)
{
	I7_SKEIN_USE_PRIVATE;
	GNode *iter;
    for(iter = node->gnode; iter; iter = all? iter->parent : NULL)
        i7_node_bless(I7_NODE(iter->data));

    priv->modified = TRUE;
}

gboolean
i7_skein_can_bless(I7Skein *self, I7Node *node, gboolean all)
{
    gboolean can_bless = FALSE;
	GNode *iter;

	for(iter = node->gnode; iter; iter = all? iter->parent : NULL)
        can_bless |= i7_node_get_changed(I7_NODE(iter->data));

	return can_bless;
}

I7Node *
i7_skein_get_thread_top(I7Skein *self, I7Node *node)
{
    while(TRUE) {
        if(G_NODE_IS_ROOT(node->gnode)) {
            g_assert_not_reached();
            return node;
        }
        if(g_node_n_children(node->gnode->parent) != 1)
            return node;
        if(G_NODE_IS_ROOT(node->gnode->parent))
            return node;
        node = node->gnode->parent->data;
    }
}

I7Node *
i7_skein_get_thread_bottom(I7Skein *self, I7Node *node)
{
    while(TRUE) {
        if(g_node_n_children(node->gnode) != 1)
            return node;
        node = node->gnode->children->data;
    }
}

/*
i7_skein_get_blessed_thread_ends(I7Skein *skein, ...

void save_transcript(I7Skein *skein, GNode *node, const gchar *path)
*/

/* Returns whether the skein was modified since last save or load */
gboolean
i7_skein_get_modified(I7Skein *self)
{
	I7_SKEIN_USE_PRIVATE;
    return priv->modified;
}

/* DEBUG */
static void
i7_skein_node_dump(I7Node *node)
{
	gchar *command = i7_node_get_command(node);
	g_printerr("(%s)", command);
	g_free(command);
}

static void 
dump_node_data(GNode *node, gpointer foo) 
{
    i7_skein_node_dump(I7_NODE(node->data));
    if(g_node_n_children(node)) {
        g_printerr("->(");
        g_node_children_foreach(node, G_TRAVERSE_ALL, dump_node_data, foo);
        g_printerr(")");
    }
}

void 
i7_skein_dump(I7Skein *self) 
{
	I7_SKEIN_USE_PRIVATE;
    dump_node_data(priv->root->gnode, NULL);
    g_printerr("\n");
}
