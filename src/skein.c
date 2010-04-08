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
#include <libgnomecanvas/libgnomecanvas.h>

#include "skein.h"
#include "prefs.h"

#define MIN_TEXT_WIDTH 25.0
#define DATA(node) ((NodeData *)(node)->data)

typedef struct _SkeinPrivate
{
	GNode *root;
	GNode *current;
	GNode *played;
	gboolean layout;
    gboolean modified;
} I7SkeinPrivate;

#define I7_SKEIN_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), I7_TYPE_SKEIN, I7SkeinPrivate))
#define I7_SKEIN_USE_PRIVATE(o,n) I7SkeinPrivate *n = I7_SKEIN_PRIVATE(o)

enum 
{
	TREE_CHANGED,
	THREAD_CHANGED,
	NODE_TEXT_CHANGED,
	NODE_COLOR_CHANGED,
	LOCK_CHANGED,
	TRANSCRIPT_THREAD_CHANGED,
	SHOW_NODE,

	LAST_SIGNAL
};

static guint i7_skein_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(I7Skein, i7_skein, G_TYPE_OBJECT);

static void
i7_skein_init(I7Skein *self)
{
	I7_SKEIN_USE_PRIVATE(self, priv);
    priv->root = node_create(_("- start -"), "", "", "", FALSE, FALSE, FALSE, 0);
    priv->current = priv->root;
    priv->played = priv->root;
    priv->layout = FALSE;
    priv->modified = TRUE;
}

static void
i7_skein_finalize(GObject *object)
{
	node_destroy(I7_SKEIN_PRIVATE(object)->root);
	G_OBJECT_CLASS(i7_skein_parent_class)->finalize(object);
}

static void
i7_skein_class_init(I7SkeinClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = i7_skein_finalize;

	/* Signals */
	/* tree-changed */
	i7_skein_signals[TREE_CHANGED] = g_signal_new("tree-changed",
	    G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_NO_RECURSE,
		G_STRUCT_OFFSET(I7SkeinClass, tree_changed), NULL, NULL,
	    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	/* thread-changed */
	i7_skein_signals[THREAD_CHANGED] = g_signal_new("thread-changed",
	    G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_NO_RECURSE,
	    G_STRUCT_OFFSET(I7SkeinClass, thread_changed), NULL, NULL,
	    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	/* node-text-changed */
	i7_skein_signals[NODE_TEXT_CHANGED] = g_signal_new("node-text-changed",
	    G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_NO_RECURSE,
	    G_STRUCT_OFFSET(I7SkeinClass, node_text_changed), NULL, NULL,
	    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	/* node-color-changed - a node was blessed or unblessed */
	i7_skein_signals[NODE_COLOR_CHANGED] = g_signal_new("node-color-changed",
	    G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_NO_RECURSE,
	    G_STRUCT_OFFSET(I7SkeinClass, node_color_changed), NULL, NULL,
	    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	/* lock-changed */
	i7_skein_signals[LOCK_CHANGED] = g_signal_new ("lock-changed",
	    G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_NO_RECURSE,
	    G_STRUCT_OFFSET(I7SkeinClass, lock_changed), NULL, NULL,
	    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	/* transcript-thread-changed */
	i7_skein_signals[TRANSCRIPT_THREAD_CHANGED] = g_signal_new("transcript-thread-changed",
	    G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_NO_RECURSE,
	    G_STRUCT_OFFSET(I7SkeinClass, transcript_thread_changed), NULL, NULL,
	    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	/* show-node - skein requests its view to display a certain node */
	i7_skein_signals[SHOW_NODE] = g_signal_new("show-node",
	    G_OBJECT_CLASS_TYPE(klass), 0,
	    G_STRUCT_OFFSET(I7SkeinClass, show_node), NULL, NULL,
	    g_cclosure_marshal_VOID__UINT_POINTER, G_TYPE_NONE, 2, 
	    G_TYPE_UINT, G_TYPE_POINTER);

	/* Add private data */
	g_type_class_add_private(klass, sizeof(I7SkeinPrivate));
}

I7Skein *
i7_skein_new(void)
{
	return g_object_new(I7_TYPE_SKEIN, NULL);
}

GNode *
i7_skein_get_root_node(I7Skein *skein)
{
    return I7_SKEIN_PRIVATE(skein)->root;
}

GNode *
i7_skein_get_current_node(I7Skein *skein)
{
    return I7_SKEIN_PRIVATE(skein)->current;
}

void
i7_skein_set_current_node(I7Skein *skein, GNode *node)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
    priv->current = node;
    g_signal_emit_by_name(skein, "thread-changed");
    priv->modified = TRUE;
}

gboolean
in_current_thread(I7Skein *skein, GNode *node)
{
    return node_in_thread(node, I7_SKEIN_PRIVATE(skein)->current);
}

GNode *
i7_skein_get_played_node(I7Skein *skein)
{
    return I7_SKEIN_PRIVATE(skein)->played;
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

/* Read the ObjectiveC "YES" and "NO" into a boolean */
static gboolean
get_boolean_from_content(xmlNode *node, gboolean default_val)
{
    if(xmlStrEqual(node->children->content, "YES"))
        return TRUE;
    else if(xmlStrEqual(node->children->content, "NO"))
        return FALSE;
    else
        return default_val;
}

static gchar *
get_text_content_from_node(xmlNode *node)
{
    if(node->children)
        if(xmlStrEqual(node->children->name, "text"))
            return g_strdup((gchar *)node->children->content);
    return NULL;
}

void
i7_skein_load(I7Skein *skein, const gchar *filename)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
	
    xmlDoc *xmldoc = xmlParseFile(filename);
    if(!xmldoc) {
        error_dialog(NULL, NULL, 
		  _("This project's Skein was not found, or it was unreadable."));
        return;
    }
    
    /* Get the top XML node */
    xmlNode *top = xmlDocGetRootElement(xmldoc);
    if(!xmlStrEqual(top->name, "Skein")) {
        error_dialog(NULL, NULL, _("This project's Skein was unreadable."));
        goto finally;
    }
    
    /* Get the ID of the root node */
    gchar *root_id = get_property_from_node(top, "rootNode");
    if(!root_id) {
        error_dialog(NULL, NULL, _("This project's Skein was unreadable."));
        goto finally;
    }

    /* Get all the item nodes and the ID of the active node */
    GHashTable *nodetable = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    gchar *active_id = NULL;
    xmlNode *item;
    /* Create a node object for each of the XML item nodes */
    for(item = top->children; item; item = item->next) {
        if(xmlStrEqual(item->name, "activeNode"))
            active_id = get_property_from_node(item, "nodeId");
        else if(xmlStrEqual(item->name, "item")) {
            gchar *id, *command = NULL, *label = NULL;
            gchar *result = NULL, *commentary = NULL;
            gboolean played = FALSE, changed = FALSE, temp = TRUE;
            int score = 0;
            
            xmlNode *child;
            for(child = item->children; child; child = child->next) {
                if(xmlStrEqual(child->name, "command"))
                    command = get_text_content_from_node(child);
                else if(xmlStrEqual(child->name, "annotation"))
                    label = get_text_content_from_node(child);
                else if(xmlStrEqual(child->name, "result"))
                    result = get_text_content_from_node(child);
                else if(xmlStrEqual(child->name, "commentary"))
                    commentary = get_text_content_from_node(child);
                else if(xmlStrEqual(child->name, "played"))
                    played = get_boolean_from_content(child, FALSE);
                else if(xmlStrEqual(child->name, "changed"))
                    changed = get_boolean_from_content(child, FALSE);
                else if(xmlStrEqual(child->name, "temporary")) {
                    temp = get_boolean_from_content(child, TRUE);
                    gchar *trash = get_property_from_node(child, "score");
                    sscanf(trash, "%d", &score);
                    g_free(trash);
                }
            }
            id = get_property_from_node(item, "nodeId"); /* freed by table */
                
            GNode *skein_node = node_create(command, label, result, commentary, played, changed, temp, score);
            g_hash_table_insert(nodetable, id, skein_node);
            g_free(command);
            g_free(label);
            g_free(result);
            g_free(commentary);
        }
    }
    
    /* Loop through the item nodes again, adding the children to each parent */
    for(item = top->children; item; item = item->next) {
        if(!xmlStrEqual(item->name, "item"))
            continue;
        gchar *id = get_property_from_node(item, "nodeId");
        GNode *parent_node = g_hash_table_lookup(nodetable, id);
        xmlNode *child;
        for(child = item->children; child; child = child->next) {
            if(!xmlStrEqual(child->name, "children"))
                continue;
            xmlNode *list;
            for(list = child->children; list; list = list->next) {
                if(!xmlStrEqual(list->name, "child"))
                    continue;
                gchar *child_id = get_property_from_node(list, "nodeId");
                g_node_append(parent_node, (GNode *)g_hash_table_lookup(nodetable, child_id));
                g_free(child_id);
            }
        }
        g_free(id);
    }

    /* Discard the current skein and replace with the new */
    node_destroy(priv->root);
    priv->root = (GNode *)g_hash_table_lookup(nodetable, root_id);
    priv->current = (GNode *)g_hash_table_lookup(nodetable, active_id);
    priv->played = priv->root;
    priv->layout = FALSE;
    g_signal_emit_by_name(skein, "tree-changed");
    priv->modified = FALSE;
    
    g_free(root_id);
    g_free(active_id);    
    g_hash_table_destroy(nodetable);
finally:
    xmlFreeDoc(xmldoc);
}

static gboolean
node_write_xml(GNode *node, FILE *fp)
{
    NodeData *data = DATA(node);
    /* Escape the following strings if necessary */
    gchar *line = g_markup_escape_text(data->line, -1);
    gchar *text_transcript = g_markup_escape_text(data->text_transcript, -1);
    gchar *text_expected = g_markup_escape_text(data->text_expected, -1);
    gchar *label = g_markup_escape_text(data->label, -1);
    
    fprintf(fp,
            "  <item nodeId=\"%s\">\n"
            "    <command xml:space=\"preserve\">%s</command>\n"
            "    <result xml:space=\"preserve\">%s</result>\n"
            "    <commentary xml:space=\"preserve\">%s</commentary>\n"
            "    <played>%s</played>\n"
            "    <changed>%s</changed>\n"
            "    <temporary score=\"%d\">%s</temporary>\n",
            data->id? data->id : "",
            line? line : "",
            text_transcript? text_transcript : "",
            text_expected? text_expected : "",
            data->played? "YES" : "NO",
            data->changed? "YES" : "NO",
            data->score,
            data->temp? "YES" : "NO");
    if(label && strlen(label))
        fprintf(fp, "    <annotation xml:space=\"preserve\">%s</annotation>\n", data->label);
    if(node->children) {
        fprintf(fp, "    <children>\n");
        GNode *child;
        for(child = node->children; child; child = child->next)
            fprintf(fp, "      <child nodeId=\"%s\"/>\n", DATA(child)->id);
        fprintf(fp, "    </children>\n");
    }
    fprintf(fp, "  </item>\n");
    /* Free strings if necessary */
    g_free(line);
	g_free(text_transcript);
    g_free(text_expected);
    g_free(label);
    return FALSE; /* Do not stop the traversal */
}
    
void
i7_skein_save(I7Skein *skein, const gchar *filename)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
    GError *err = NULL;
	
    FILE *skeinfile = fopen(filename, "w");
    if(!skeinfile) {
        error_dialog(NULL, NULL, _("Error saving file '%s': %s"), filename, g_strerror(errno));
        return;
    }
    
    fprintf(skeinfile,
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<Skein rootNode=\"%s\" "
            "xmlns=\"http://www.logicalshift.org.uk/IF/Skein\">\n"
            "  <generator>GNOME Inform 7</generator>\n"
            "  <activeNode nodeId=\"%s\"/>\n",
            node_get_unique_id(priv->root),
            node_get_unique_id(priv->current));
    g_node_traverse(priv->root, G_PRE_ORDER, G_TRAVERSE_ALL, -1, (GNodeTraverseFunc)node_write_xml, skeinfile);
    fprintf(skeinfile, "</Skein>\n");
    fclose(skeinfile);
    priv->modified = FALSE;
}

void
i7_skein_reset(I7Skein *skein, gboolean current)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
    if(current)
        priv->current = priv->root;
    priv->played = priv->root;
    g_signal_emit_by_name(skein, "thread-changed");
    priv->modified = TRUE;
}

static void
node_layout(GNode *node, PangoLayout *layout, double x, double spacing)
{
    /* Store the centre x coordinate for this node */
    DATA(node)->x = x;
    
    /* Find the total width of all descendant nodes */
    int i;
    double total = node_get_tree_width(node, layout, spacing);
    /* Lay out each child node */
    double child_x = 0.0;
    for(i = 0; i < g_node_n_children(node); i++) {
        GNode *child = g_node_nth_child(node, i);
        double treewidth = node_get_tree_width(child, layout, spacing);
        node_layout(child, layout, x - total * 0.5 + child_x + treewidth * 0.5, spacing);
        child_x += treewidth + spacing;
    }
}  
            
void
i7_skein_layout(I7Skein *skein, double spacing)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
    if(!priv->layout) {
        PangoFontDescription *font = get_font_description();
        PangoContext *context = gdk_pango_context_get();
        PangoLayout *layout = pango_layout_new(context);
        pango_layout_set_font_description(layout, font);
        node_layout(priv->root, layout, 0.0, spacing);
        g_object_unref(G_OBJECT(layout));
        g_object_unref(G_OBJECT(context));
        pango_font_description_free(font);
    }
    priv->layout = TRUE;
}

void
i7_skein_invalidate_layout(I7Skein *skein)
{
	I7_SKEIN_PRIVATE(skein)->layout = FALSE;
    g_signal_emit_by_name(skein, "tree-changed");
}

void
i7_skein_new_line(I7Skein *skein, const gchar *line)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
	
    gboolean node_added = FALSE;
    gchar *nodeline = g_strescape(line, "\"");

    /* Is there a child node with the same line? */
    GNode *node = priv->current->children;
    while(node != NULL) {
        /* Special case: NULL is treated as "" */
        if(DATA(node)->line == NULL) {
            if(line == NULL || strlen(line) == 0)
                break;
        } else if(strcmp(DATA(node)->line, line) == 0)
            break;
        node = node->next;
    }
    if(node == NULL) {
        node = node_create(nodeline, "", "", "", TRUE, FALSE, TRUE, 0);
        g_node_append(priv->current, node);
        node_added = TRUE;
    }
    g_free(nodeline);
    
    /* Make this the new current node */
    priv->current = node;
    priv->played = node;

    /* Send signals */
    if(node_added) {
        priv->layout = FALSE;
        g_signal_emit_by_name(skein, "tree-changed");
    } else
        g_signal_emit_by_name(skein, "thread-changed");
    g_signal_emit_by_name(skein, "show-node", GOT_LINE, node);
    priv->modified = TRUE;
}

/* Find the next node to use. Return TRUE if found, and if so, store a
newly-allocated copy of its command text in line.*/
gboolean
i7_skein_next_line(I7Skein *skein, gchar **line)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
	
    if(!g_node_is_ancestor(priv->played, priv->current))
        return FALSE;

    GNode *next = priv->current;
    while(next->parent != priv->played)
        next = next->parent;
    priv->played = next;
    *line = g_strcompress(node_get_line(next));
    g_signal_emit_by_name(skein, "thread-changed");
    g_signal_emit_by_name(skein, "show-node", GOT_LINE, next);
    return TRUE;
}

/* Get a list of the commands from the play pointer to the current pointer,
without resetting either of them */
GSList *
i7_skein_get_commands(I7Skein *skein)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
	
    GSList *commands = NULL;
    gchar *skeinline = NULL;
    GNode *pointer = priv->played;
    while(g_node_is_ancestor(pointer, priv->current)) {
        GNode *next = priv->current;
        while(next->parent != pointer)
            next = next->parent;
        pointer = next;
        skeinline = g_strcompress(node_get_line(next));
        commands = g_slist_prepend(commands, g_strdup(skeinline));
        g_free(skeinline);
        g_signal_emit_by_name(skein, "show-node", GOT_LINE, next);
    }
    commands = g_slist_reverse(commands);
    return commands;
}

/* Update the status of the last played node */
void
i7_skein_update_after_playing(I7Skein *skein, const gchar *transcript)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
	
    node_set_played(priv->played);
    g_signal_emit_by_name(skein, "node-color-changed");
    if(strlen(transcript)) {
        node_new_transcript_text(priv->played, transcript);
        g_signal_emit_by_name(skein, "show-node", GOT_TRANSCRIPT, priv->played);
    }
}

gboolean
i7_skein_get_line_from_history(I7Skein *skein, gchar **line, int history)
{
    /* Find the node to return the line from */
    GNode *node = I7_SKEIN_PRIVATE(skein)->current;
    while(--history > 0) {
        node = node->parent;
        if(node == NULL)
            return FALSE;
    }
    /* Don't return the root node */
    if(G_NODE_IS_ROOT(node))
        return FALSE;
    
    *line = g_strdelimit(g_strdup(node_get_line(node)), "\b\f\n\r\t", ' ');
    return TRUE;
}

/* Add an empty node as the child of node */
GNode *
i7_skein_add_new(I7Skein *skein, GNode *node)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
	
    GNode *newnode = node_create("", "", "", "", FALSE, FALSE, TRUE, 0);
    g_node_append(node, newnode);
    
    priv->layout = FALSE;
    g_signal_emit_by_name(skein, "tree-changed");
    priv->modified = TRUE;
    
    return newnode;
}

GNode *
i7_skein_add_new_parent(I7Skein *skein, GNode *node)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
	
    GNode *newnode = node_create("", "", "", "", FALSE, FALSE, TRUE, 0);
    g_node_insert(node->parent, g_node_child_position(node->parent, node), newnode);
    g_node_unlink(node);
    g_node_append(newnode, node);
    
    priv->layout = FALSE;
    g_signal_emit_by_name(skein, "tree-changed");
    priv->modified = TRUE;
    
    return newnode;
}

gboolean
i7_skein_remove_all(I7Skein *skein, GNode *node, gboolean notify)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
	
    if(!G_NODE_IS_ROOT(node)) {
        gboolean in_current = in_current_thread(skein, node);
        node_destroy(node);
        
        if(in_current) {
            priv->current = priv->root;
            priv->played = priv->root;
        }
        
        priv->layout = FALSE;
        if(notify)
            g_signal_emit_by_name(skein, "tree-changed");
        priv->modified = TRUE;
        
        return TRUE;
    }
    return FALSE;
}

gboolean
i7_skein_remove_single(I7Skein *skein, GNode *node)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
	
    if(!G_NODE_IS_ROOT(node)) {
        gboolean in_current = in_current_thread(skein, node);

		if(!G_NODE_IS_LEAF(node)) {
			int i;
        	for(i = g_node_n_children(node) - 1; i >= 0; i--) {
				GNode *iter = g_node_nth_child(node, i);
				g_node_unlink(iter);
				g_node_insert_after(node->parent, node, iter);
			}
		}	
        node_destroy(node);
		
        if(in_current) {
            priv->current = priv->root;
            priv->played = priv->root;
        }
        
        priv->layout = FALSE;
        g_signal_emit_by_name(skein, "tree-changed");
        priv->modified = TRUE;
        return TRUE;
    }
    return FALSE;
}

void
i7_skein_set_line(I7Skein *skein, GNode *node, const gchar *line)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
    node_set_line(node, line);
    priv->layout = FALSE;
    g_signal_emit(skein, i7_skein_signals[NODE_TEXT_CHANGED], 0);
    priv->modified = TRUE;
}

void
i7_skein_set_label(I7Skein *skein, GNode *node, const gchar *label)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
    node_set_label(node, label);
    priv->layout = FALSE;
    g_signal_emit(skein, i7_skein_signals[NODE_TEXT_CHANGED], 0);
    priv->modified = TRUE;
}

void
i7_skein_lock(I7Skein *skein, GNode *node)
{
    while(node) {
        node_set_temporary(node, FALSE);
        node = node->parent;
    }
    g_signal_emit(skein, i7_skein_signals[LOCK_CHANGED], 0);
    I7_SKEIN_PRIVATE(skein)->modified = TRUE;
}

void
i7_skein_unlock(I7Skein *skein, GNode *node, gboolean notify)
{
    node_set_temporary(node, TRUE);
    for(node = node->children; node != NULL; node = node->next)
        i7_skein_unlock(skein, node, FALSE);
    
    if(notify)
        g_signal_emit(skein, i7_skein_signals[LOCK_CHANGED], 0);
    I7_SKEIN_PRIVATE(skein)->modified = TRUE;
}

void
i7_skein_trim(I7Skein *skein, GNode *node, int minScore, gboolean notify)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
	
    int i = 0;
    while(i < g_node_n_children(node)) {
        GNode *child = g_node_nth_child(node, i);
        if(node_get_temporary(child)) {
            if(in_current_thread(skein, child)) {
                priv->current = priv->root;
                priv->played = priv->root;
            }
            
            if(i7_skein_remove_all(skein, child, FALSE) == FALSE)
                i++;
        } else {
            i7_skein_trim(skein, child, -1, FALSE);
            i++;
        }
    }
    
    if(notify)
        g_signal_emit_by_name(skein, "tree-changed");
    priv->modified = TRUE;
}



static void
get_labels(GNode *node, GSList **labels)
{
	NodeData *data = DATA(node);
	if(data->label && strlen(data->label)) {
		NodeLabel *nodelabel = g_new0(NodeLabel, 1);
		nodelabel->label = g_strdup(data->label);
		nodelabel->node = node;
		*labels = g_slist_prepend(*labels, (gpointer)nodelabel);
	}
	for(node = node->children; node != NULL; node = node->next)
		get_labels(node, labels);
}

GSList *
i7_skein_get_labels(I7Skein *skein)
{
	GSList *labels = NULL;
	get_labels(I7_SKEIN_PRIVATE(skein)->root, &labels);
	labels = g_slist_sort_with_data(labels, (GCompareDataFunc)strcmp, NULL);
	return labels;
}

static gboolean
has_labels(GNode *node)
{
    NodeData *data = DATA(node);
    if(data->label && strlen(data->label))
        return TRUE;
    for(node = node->children; node != NULL; node = node->next)
        if(has_labels(node))
            return TRUE;
    return FALSE;
}

gboolean
i7_skein_has_labels(I7Skein *skein)
{
    return has_labels(I7_SKEIN_PRIVATE(skein)->root);
}

void
i7_skein_bless(I7Skein *skein, GNode *node, gboolean all)
{
    while(node != NULL) {
        node_bless(node);
        node = all? node->parent : NULL;
    }
    g_signal_emit(skein, i7_skein_signals[NODE_COLOR_CHANGED], 0);
    I7_SKEIN_PRIVATE(skein)->modified = TRUE;
}

gboolean
i7_skein_can_bless(I7Skein *skein, GNode *node, gboolean all)
{
    gboolean can_bless = FALSE;
    while(node != NULL) {
        can_bless |= DATA(node)->changed;
        node = all? node->parent : NULL;
    }
    return can_bless;
}

void
i7_skein_set_expected_text(I7Skein *skein, GNode *node, const gchar *text)
{
    node_set_expected_text(node, text);
    g_signal_emit(skein, i7_skein_signals[NODE_COLOR_CHANGED], 0);
    I7_SKEIN_PRIVATE(skein)->modified = TRUE;
}

GNode *
i7_skein_get_thread_top(I7Skein *skein, GNode *node)
{
    while(TRUE) {
        if(G_NODE_IS_ROOT(node)) {
            g_assert_not_reached();
            return node;
        }
        if(g_node_n_children(node->parent) != 1)
            return node;
        if(G_NODE_IS_ROOT(node->parent))
            return node;
        node = node->parent;
    }
}

GNode *
i7_skein_get_thread_bottom(I7Skein *skein, GNode *node)
{
    while(TRUE) {
        if(g_node_n_children(node) != 1)
            return node;
        node = node->children;
    }
}

/*
i7_skein_get_blessed_thread_ends(I7Skein *skein, ...

void save_transcript(I7Skein *skein, GNode *node, const gchar *path)
*/

/* Returns whether the skein was modified since last save or load */
gboolean
i7_skein_get_modified(I7Skein *skein)
{
    return I7_SKEIN_PRIVATE(skein)->modified;
}

GNode *
node_create(const gchar *line, const gchar *label, const gchar *transcript,
            const gchar *expected, gboolean played, gboolean changed,
            gboolean temp, int score)
{
    NodeData *data = g_slice_new0(NodeData);
    data->line = g_strdup(line);
    data->label = g_strdup(label);
    data->text_transcript = g_strdup(transcript);
    data->text_expected = g_strdup(expected);
    data->played = played;
    data->changed = changed;
    data->temp = temp;
    data->score = score;
    data->width = -1.0;
    data->linewidth = -1.0;
    data->labelwidth = 0.0;
    data->x = 0.0;
    
    data->id = g_strdup_printf("node-%p", data);
    /* diffs */
    
    return g_node_new(data);
}

static void
free_node_data(GNode *node)
{
    NodeData *data = DATA(node);
    g_free(data->line);
    g_free(data->label);
    g_free(data->text_transcript);
    g_free(data->text_expected);
    g_free(data->id);
    g_slice_free(NodeData, data);
    
    /* recurse */
    g_node_children_foreach(node, G_TRAVERSE_ALL, (GNodeForeachFunc)free_node_data, NULL);
    /* free the node itself */
    g_node_destroy(node);
}

void
node_destroy(GNode *node)
{
    free_node_data(node);
}

const gchar *
node_get_line(GNode *node)
{
    return DATA(node)->line;
}

void
node_set_line(GNode *node, const gchar *line)
{
    NodeData *data = DATA(node);
    if(data->line)
        g_free(data->line);
    data->line = g_strdup(line);
    data->width = -1.0;
    data->linewidth = -1.0;
    data->labelwidth = -1.0;
}

const gchar *
node_get_label(GNode *node)
{
    return DATA(node)->label;
}

void
node_set_label(GNode *node, const gchar *label)
{
    NodeData *data = DATA(node);
    if(data->label)
        g_free(data->label);
    data->label = g_strdup(label);
    data->width = -1.0;
    data->linewidth = -1.0;
    data->labelwidth = -1.0;
}

gboolean
node_has_label(GNode *node)
{
    NodeData *data = DATA(node);
    return (node->parent != NULL) && data->label && (strlen(data->label) > 0);
}

const gchar *
node_get_expected_text(GNode *node)
{
    return DATA(node)->text_expected;
}

gboolean
node_get_changed(GNode *node)
{
    return DATA(node)->changed;
}

gboolean
node_get_temporary(GNode *node)
{
    return DATA(node)->temp;
}

void
node_set_played(GNode *node)
{
    DATA(node)->played = TRUE;
}

void
node_set_temporary(GNode *node, gboolean temp)
{
    DATA(node)->temp = temp;
}

void
node_new_transcript_text(GNode *node, const gchar *transcript)
{
    NodeData *data = DATA(node);
    if(data->text_transcript)
        g_free(data->text_transcript);
    data->text_transcript = g_strdup(transcript);
    g_strdelimit(data->text_transcript, "\r", '\n');
    data->changed = (strcmp(data->text_transcript, data->text_expected) != 0);
    
    /* clear diffs */
    if(data->changed && data->text_expected && strlen(data->text_expected))
        /* new diffs */;
}

void
node_bless(GNode *node)
{
    NodeData *data = DATA(node);
    if(data->text_expected)
        g_free(data->text_expected);
    data->text_expected = g_strdup(data->text_transcript);
    data->changed = FALSE;
    
    /* clear diffs */
}

void
node_set_expected_text(GNode *node, const gchar *text)
{
    NodeData *data = DATA(node);
    if(data->text_expected)
        g_free(data->text_expected);
    data->text_expected = g_strdup(text);
    g_strdelimit(data->text_transcript, "\r", '\n');
    data->changed = (strcmp(data->text_transcript, data->text_expected) != 0);
    
    /* clear diffs */
    
    if(data->changed && data->text_expected && strlen(data->text_expected))
        /* new diffs */;
}

static double
text_pixel_width(PangoLayout *layout, const gchar *text)
{
    int textwidth = 0;
	if(text && strlen(text)) {
    	pango_layout_set_text(layout, text, -1);
    	pango_layout_get_pixel_size(layout, &textwidth, NULL);
	}
    return (double)textwidth;
}

double
node_get_line_width(GNode *node, PangoLayout *layout)
{
    NodeData *data = DATA(node);
    if(data->width < 0.0) {
    	if(layout) {
		    double size = text_pixel_width(layout, data->line);
		    data->width = size;
		    data->linewidth = size;
		    
		    if(data->label && strlen(data->label) > 0)
		        data->labelwidth = text_pixel_width(layout, data->label);
		    else
		        data->labelwidth = 0.0;
        }
        if(data->width < MIN_TEXT_WIDTH)
            data->width = MIN_TEXT_WIDTH;
        if(data->linewidth < MIN_TEXT_WIDTH)
            data->linewidth = MIN_TEXT_WIDTH;
        if(data->labelwidth < MIN_TEXT_WIDTH)
            data->labelwidth = MIN_TEXT_WIDTH;
    }
    return data->width;
}

double
node_get_line_text_width(GNode *node)
{
    return DATA(node)->linewidth;
}

double
node_get_label_text_width(GNode *node)
{
    return DATA(node)->labelwidth;
}

double
node_get_tree_width(GNode *node, PangoLayout *layout, double spacing)
{
    /* Get the tree width of all children */
    int i;
    double total = 0.0;
    for(i = 0; i < g_node_n_children(node); i++) {
        total += node_get_tree_width(g_node_nth_child(node,i), layout, spacing);
        if(i > 0)
            total += spacing;
    }
    /* Return the largest of the above, the width of this node's line and the
    width of this node's label */
    double linewidth = node_get_line_width(node, layout);
    double labelwidth = node_get_label_text_width(node);
    if(total > linewidth && total > labelwidth)
        return total;
    return max(linewidth, labelwidth);
}    

const gchar *
node_get_unique_id(GNode *node)
{
    return DATA(node)->id;
}

double
node_get_x(GNode *node)
{
    return DATA(node)->x;
}

gboolean
node_in_thread(GNode *node, GNode *endnode)
{
    return (endnode == node) || g_node_is_ancestor(node, endnode);
}

/* DEBUG */
static void 
dump_node_data(GNode *node, gpointer foo) 
{
    g_printerr("(%s)", DATA(node)->line);
    if(g_node_n_children(node)) {
        g_printerr("->(");
        g_node_children_foreach(node, G_TRAVERSE_ALL, dump_node_data, foo);
        g_printerr(")");
    }
}

void 
i7_skein_dump(I7Skein *skein) 
{
    dump_node_data(I7_SKEIN_PRIVATE(skein)->root, NULL);
    g_printerr("\n");
}
