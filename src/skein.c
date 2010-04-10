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

#include "skein.h"
#include "node.h"

typedef struct _SkeinPrivate
{
	I7Node *root;
	I7Node *current;
	I7Node *played;
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

/* SIGNAL HANDLERS */

static void
on_node_text_notify(I7Node *node, GParamSpec *pspec, I7Skein *skein)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
	priv->layout = FALSE;
	g_signal_emit_by_name(skein, "node-text-changed");
	priv->modified = TRUE;
}

static void
on_node_color_notify(I7Node *node, GParamSpec *pspec, I7Skein *skein)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
	g_signal_emit_by_name(skein, "node-color-changed");
	priv->modified = TRUE;
}

static void
node_listen(I7Skein *skein, I7Node *node)
{
	g_signal_connect(node, "notify::line", G_CALLBACK(on_node_text_notify), skein);
	g_signal_connect(node, "notify::label", G_CALLBACK(on_node_text_notify), skein);
	g_signal_connect(node, "notify::text-expected", G_CALLBACK(on_node_color_notify), skein);
}

/* TYPE SYSTEM */

static void
i7_skein_init(I7Skein *self)
{
	I7_SKEIN_USE_PRIVATE(self, priv);
    priv->root = i7_node_new(_("- start -"), "", "", "", FALSE, FALSE, FALSE, 0);
	node_listen(self, priv->root);
    priv->current = priv->root;
    priv->played = priv->root;
    priv->layout = FALSE;
    priv->modified = TRUE;
}

static void
i7_skein_finalize(GObject *object)
{
	g_object_unref(I7_SKEIN_PRIVATE(object)->root);
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
	    G_TYPE_UINT, I7_TYPE_NODE);

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
i7_skein_get_root_node(I7Skein *skein)
{
    return I7_SKEIN_PRIVATE(skein)->root;
}

I7Node *
i7_skein_get_current_node(I7Skein *skein)
{
    return I7_SKEIN_PRIVATE(skein)->current;
}

void
i7_skein_set_current_node(I7Skein *skein, I7Node *node)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
    priv->current = node;
    g_signal_emit_by_name(skein, "thread-changed");
    priv->modified = TRUE;
}

gboolean
i7_skein_is_node_in_current_thread(I7Skein *skein, I7Node *node)
{
    return i7_node_in_thread(node, I7_SKEIN_PRIVATE(skein)->current);
}

I7Node *
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

gboolean
i7_skein_load(I7Skein *skein, const gchar *filename, GError **error)
{
	g_return_if_fail(error == NULL || *error == NULL);
	
	I7_SKEIN_USE_PRIVATE(skein, priv);
	
    xmlDoc *xmldoc = xmlParseFile(filename);
    if(!xmldoc) {
		xmlErrorPtr xml_error = xmlGetLastError();
		if(error)
			*error = g_error_new_literal(I7_SKEIN_ERROR, I7_SKEIN_ERROR_XML, g_strdup(xml_error->message));
        return FALSE;
    }
    
    /* Get the top XML node */
    xmlNode *top = xmlDocGetRootElement(xmldoc);
    if(!xmlStrEqual(top->name, "Skein")) {
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
            
            I7Node *skein_node = i7_node_new(command, label, result, commentary, played, changed, temp, score);
			node_listen(skein, skein_node);
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
        I7Node *parent_node = g_hash_table_lookup(nodetable, id);
        xmlNode *child;
        for(child = item->children; child; child = child->next) {
            if(!xmlStrEqual(child->name, "children"))
                continue;
            xmlNode *list;
            for(list = child->children; list; list = list->next) {
                if(!xmlStrEqual(list->name, "child"))
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
    priv->current = I7_NODE(g_hash_table_lookup(nodetable, active_id));
    priv->played = priv->root;
    priv->layout = FALSE;
    g_signal_emit_by_name(skein, "tree-changed");
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
i7_skein_save(I7Skein *skein, const gchar *filename, GError **error)
{
	g_return_if_fail(error == NULL || *error == NULL);
	
	I7_SKEIN_USE_PRIVATE(skein, priv);
	
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
i7_skein_reset(I7Skein *skein, gboolean current)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
    if(current)
        priv->current = priv->root;
    priv->played = priv->root;
    g_signal_emit_by_name(skein, "thread-changed");
    priv->modified = TRUE;
}
            
void
i7_skein_layout(I7Skein *skein, double spacing)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
    if(!priv->layout) {
        i7_node_layout(priv->root, 0.0, spacing);
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
	I7Node *node = NULL;
    GNode *gnode = priv->current->gnode->children;
    while(gnode != NULL) {
		gchar *cmpline = i7_node_get_line(I7_NODE(gnode->data));
        /* Special case: NULL is treated as "" */
        if((strlen(cmpline) == 0 && (line == NULL || strlen(line) == 0)) || (strcmp(cmpline, line) == 0)) {
			g_free(cmpline);
			node = gnode->data;
            break;
		}
        gnode = gnode->next;
    }
    if(node == NULL) { 
		/* Wasn't found, create new node */
        node = i7_node_new(nodeline, "", "", "", TRUE, FALSE, TRUE, 0);
		node_listen(skein, node);
        g_node_append(priv->current->gnode, node->gnode);
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
	
    if(!g_node_is_ancestor(priv->played->gnode, priv->current->gnode))
        return FALSE;

    I7Node *next = priv->current;
    while(next->gnode->parent != priv->played->gnode)
        next = next->gnode->parent->data;
    priv->played = next;
	gchar *temp = i7_node_get_line(next);
    *line = g_strcompress(temp);
	g_free(temp);
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
    GNode *pointer = priv->played->gnode;
    while(g_node_is_ancestor(pointer, priv->current->gnode)) {
        I7Node *next = priv->current;
        while(next->gnode->parent != pointer)
            next = next->gnode->parent->data;
        pointer = next->gnode;
		gchar *skeinline = i7_node_get_line(next);
        commands = g_slist_prepend(commands, g_strcompress(skeinline));
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
	
    i7_node_set_played(priv->played);
    g_signal_emit_by_name(skein, "node-color-changed");
    if(strlen(transcript)) {
        i7_node_set_transcript_text(priv->played, transcript);
        g_signal_emit_by_name(skein, "show-node", GOT_TRANSCRIPT, priv->played);
    }
}

gboolean
i7_skein_get_line_from_history(I7Skein *skein, gchar **line, int history)
{
    /* Find the node to return the line from */
    GNode *node = I7_SKEIN_PRIVATE(skein)->current->gnode;
    while(--history > 0) {
        node = node->parent;
        if(node == NULL)
            return FALSE;
    }
    /* Don't return the root node */
    if(G_NODE_IS_ROOT(node))
        return FALSE;
    
    *line = g_strdelimit(i7_node_get_line(I7_NODE(node->data)), "\b\f\n\r\t", ' ');
    return TRUE;
}

/* Add an empty node as the child of node */
I7Node *
i7_skein_add_new(I7Skein *skein, I7Node *node)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
	
    I7Node *newnode = i7_node_new("", "", "", "", FALSE, FALSE, TRUE, 0);
	node_listen(skein, newnode);
    g_node_append(node->gnode, newnode->gnode);
    
    priv->layout = FALSE;
    g_signal_emit_by_name(skein, "tree-changed");
    priv->modified = TRUE;
    
    return newnode;
}

I7Node *
i7_skein_add_new_parent(I7Skein *skein, I7Node *node)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
	
    I7Node *newnode = i7_node_new("", "", "", "", FALSE, FALSE, TRUE, 0);
	node_listen(skein, newnode);
    g_node_insert(node->gnode->parent, g_node_child_position(node->gnode->parent, node->gnode), newnode->gnode);
    g_node_unlink(node->gnode);
    g_node_append(newnode->gnode, node->gnode);
    
    priv->layout = FALSE;
    g_signal_emit_by_name(skein, "tree-changed");
    priv->modified = TRUE;
    
    return newnode;
}

gboolean
i7_skein_remove_all(I7Skein *skein, I7Node *node, gboolean notify)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
	
    if(G_NODE_IS_ROOT(node->gnode))
		return FALSE;
	
    gboolean in_current = i7_skein_is_node_in_current_thread(skein, node);
    g_object_unref(node);
    
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

gboolean
i7_skein_remove_single(I7Skein *skein, I7Node *node)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
	
    if(G_NODE_IS_ROOT(node->gnode))
		return FALSE;

    gboolean in_current = i7_skein_is_node_in_current_thread(skein, node);

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
    
    priv->layout = FALSE;
    g_signal_emit_by_name(skein, "tree-changed");
    priv->modified = TRUE;
    return TRUE;
}

void
i7_skein_lock(I7Skein *skein, I7Node *node)
{
	GNode *iter;
	for(iter = node->gnode; iter; iter = iter->parent)
        i7_node_set_temporary(I7_NODE(iter->data), FALSE);
    
    g_signal_emit_by_name(skein, "lock-changed");
    I7_SKEIN_PRIVATE(skein)->modified = TRUE;
}

void
i7_skein_unlock(I7Skein *skein, I7Node *node, gboolean notify)
{
    i7_node_set_temporary(node, TRUE);
	GNode *iter;
    for(iter = node->gnode->children; iter; iter = iter->next)
        i7_skein_unlock(skein, I7_NODE(iter->data), FALSE);
    
    if(notify)
        g_signal_emit_by_name(skein, "lock-changed");
    I7_SKEIN_PRIVATE(skein)->modified = TRUE;
}

void
i7_skein_trim(I7Skein *skein, I7Node *node, int minScore, gboolean notify)
{
	I7_SKEIN_USE_PRIVATE(skein, priv);
	
    int i = 0;
    while(i < g_node_n_children(node->gnode)) {
        I7Node *child = g_node_nth_child(node->gnode, i)->data;
        if(i7_node_get_temporary(child)) {
            if(i7_skein_is_node_in_current_thread(skein, child)) {
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
i7_skein_get_labels(I7Skein *skein)
{
	GSList *labels = NULL;
	get_labels(I7_SKEIN_PRIVATE(skein)->root->gnode, &labels);
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
i7_skein_has_labels(I7Skein *skein)
{
    return has_labels(I7_SKEIN_PRIVATE(skein)->root);
}

void
i7_skein_bless(I7Skein *skein, I7Node *node, gboolean all)
{
	GNode *iter;
    for(iter = node->gnode; iter; iter = all? iter->parent : NULL)
        i7_node_bless(I7_NODE(iter->data));

    g_signal_emit_by_name(skein, "node-color-changed");
    I7_SKEIN_PRIVATE(skein)->modified = TRUE;
}

gboolean
i7_skein_can_bless(I7Skein *skein, I7Node *node, gboolean all)
{
    gboolean can_bless = FALSE;
	GNode *iter;

	for(iter = node->gnode; iter; iter = all? iter->parent : NULL)
        can_bless |= i7_node_get_changed(I7_NODE(iter->data));

	return can_bless;
}

I7Node *
i7_skein_get_thread_top(I7Skein *skein, I7Node *node)
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
i7_skein_get_thread_bottom(I7Skein *skein, I7Node *node)
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
i7_skein_get_modified(I7Skein *skein)
{
    return I7_SKEIN_PRIVATE(skein)->modified;
}

/* DEBUG */
static void 
dump_node_data(GNode *node, gpointer foo) 
{
    i7_node_dump(I7_NODE(node->data));
    if(g_node_n_children(node)) {
        g_printerr("->(");
        g_node_children_foreach(node, G_TRAVERSE_ALL, dump_node_data, foo);
        g_printerr(")");
    }
}

void 
i7_skein_dump(I7Skein *skein) 
{
    dump_node_data(I7_SKEIN_PRIVATE(skein)->root->gnode, NULL);
    g_printerr("\n");
}
