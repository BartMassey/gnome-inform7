#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <goocanvas.h>

#include "node.h"
#include "skein.h"

#define MIN_TEXT_WIDTH 25.0
#define LABEL_MULTIPLIER 0.7

enum {
    PROP_0,
    PROP_LINE,
    PROP_LABEL,
    PROP_TEXT_TRANSCRIPT,
    PROP_TEXT_EXPECTED,
    PROP_CHANGED,
    PROP_TEMP,
    PROP_SCORE
};

typedef struct _I7NodePrivate {
	gchar *line;
    gchar *label;
    gchar *id;
    
    gchar *text_transcript;
    gchar *text_expected;
    
    gboolean played;
    gboolean changed;
    gboolean temp;
    gint score;
    
    gdouble width;
    gdouble linewidth;
    gdouble labelwidth;
    gdouble x;
    
    /* Graphical goodness */
    GooCanvasItemModel *node_group;
    GooCanvasItemModel *line_item;
    GooCanvasItemModel *label_item;
    GooCanvasItemModel *badge_item;
    GooCanvasItemModel *line_shape_item;
    GooCanvasItemModel *label_shape_item;
    GooCanvasItemModel *tree_item;
} I7NodePrivate;

#define I7_NODE_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o), I7_TYPE_NODE, I7NodePrivate))
#define I7_NODE_USE_PRIVATE(o,n) I7NodePrivate *n = I7_NODE_PRIVATE(o)

G_DEFINE_TYPE(I7Node, i7_node, G_TYPE_OBJECT);

/* SIGNAL HANDLERS */
static void
on_text_notify(GObject *object, GParamSpec *pspec)
{
	I7_NODE_USE_PRIVATE(object, priv);

	gboolean old_changed_status = priv->changed;
	priv->changed = (strcmp(priv->text_transcript, priv->text_expected) != 0);
	if(priv->changed != old_changed_status)
		g_object_notify(object, "changed");
    
    /* TODO clear diffs */
    
    if(priv->changed && priv->text_expected && strlen(priv->text_expected))
        /* TODO new diffs */;
}

/* TYPE SYSTEM */

static void
i7_node_init(I7Node *self)
{
	I7_NODE_USE_PRIVATE(self, priv);
	priv->id = g_strdup_printf("node-%p", self);
    priv->width = -1.0;
    priv->linewidth = -1.0;
    priv->labelwidth = 0.0;
    priv->x = 0.0;
	self->gnode = NULL;
    
    /* TODO diffs */
    
    /* Create the canvas items, though some of them can't be drawn yet */
    priv->node_group = goo_canvas_group_model_new(NULL, NULL);
    priv->line_item = goo_canvas_text_model_new(priv->node_group, "", 0.0, 0.0, -1, GTK_ANCHOR_CENTER, NULL);
	priv->label_item = goo_canvas_text_model_new(priv->node_group, "", 0.0, 0.0, -1, GTK_ANCHOR_CENTER, NULL);
	priv->badge_item = goo_canvas_image_model_new(priv->node_group, NULL, 0.0, 0.0, NULL);
	priv->line_shape_item = goo_canvas_path_model_new(priv->node_group, "", NULL);
    priv->label_shape_item = goo_canvas_path_model_new(priv->node_group, "", NULL);
    goo_canvas_item_model_raise(priv->badge_item, NULL);
    
    /* Connect signals */
	g_signal_connect(self, "notify::text-expected", G_CALLBACK(on_text_notify), NULL);
	g_signal_connect(self, "notify::text-transcript", G_CALLBACK(on_text_notify), NULL);
}

static void
i7_node_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    I7Node *node = I7_NODE(object);
    
    switch(prop_id) {
		case PROP_LINE:
			i7_node_set_line(node, g_value_get_string(value));
			break;
		case PROP_LABEL:
			i7_node_set_label(node, g_value_get_string(value));
			break;
		case PROP_TEXT_TRANSCRIPT:
			i7_node_set_transcript_text(node, g_value_get_string(value));
			break;
		case PROP_TEXT_EXPECTED:
			i7_node_set_expected_text(node, g_value_get_string(value));
			break;
		case PROP_CHANGED: /* Construct only */
			I7_NODE_PRIVATE(node)->changed = g_value_get_boolean(value);
			g_object_notify(object, "changed");
			break;
		case PROP_TEMP:
			i7_node_set_temporary(node, g_value_get_boolean(value));
			break;
		case PROP_SCORE: /* Construct only */
			I7_NODE_PRIVATE(node)->score = g_value_get_int(value);
			g_object_notify(object, "changed");
			break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
i7_node_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    I7_NODE_USE_PRIVATE(object, priv);
    
    switch(prop_id) {
		case PROP_LINE:
			g_value_set_string(value, priv->line);
			break;
		case PROP_LABEL:
			g_value_set_string(value, priv->label);
			break;
		case PROP_TEXT_TRANSCRIPT:
			g_value_set_string(value, priv->text_transcript);
			break;
		case PROP_TEXT_EXPECTED:
			g_value_set_string(value, priv->text_expected);
			break;
		case PROP_CHANGED:
			g_value_set_boolean(value, priv->changed);
			break;
		case PROP_TEMP:
			g_value_set_boolean(value, priv->temp);
			break;
		case PROP_SCORE:
			g_value_set_int(value, priv->score);
			break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
unref_node(GNode *gnode)
{
	g_object_unref(gnode->data);
}

static void
i7_node_finalize(GObject *object)
{
	I7_NODE_USE_PRIVATE(object, priv);
	g_free(priv->line);
    g_free(priv->label);
    g_free(priv->text_transcript);
    g_free(priv->text_expected);
	g_free(priv->id);
    g_object_unref(priv->node_group);
    
    /* recurse */
	GNode *gnode = I7_NODE(object)->gnode;
	g_node_children_foreach(gnode, G_TRAVERSE_ALL, (GNodeForeachFunc)unref_node, NULL);
    /* free the node itself */
    g_node_destroy(gnode);
	
	G_OBJECT_CLASS(i7_node_parent_class)->finalize(object);
}

static void
i7_node_class_init(I7NodeClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->set_property = i7_node_set_property;
	object_class->get_property = i7_node_get_property;
	object_class->finalize = i7_node_finalize;

	/* Install properties */
	GParamFlags flags = G_PARAM_LAX_VALIDATION | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB;
	/* SUCKY DEBIAN replace with G_PARAM_STATIC_STRINGS */
	g_object_class_install_property(object_class, PROP_LINE,
		g_param_spec_string("line", _("Line text"),
		    _("The command entered in the game"),
		    "", flags | G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property(object_class, PROP_LABEL,
		g_param_spec_string("label", _("Label"),
		    _("The text this node has been labelled with"),
		    "", flags | G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property(object_class, PROP_TEXT_TRANSCRIPT,
	    g_param_spec_string("text-transcript", _("Transcript text"),
		    _("The text produced by the command on the last play-through"),
		    "", flags | G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property(object_class, PROP_TEXT_EXPECTED,
	    g_param_spec_string("text-expected", _("Expected text"),
		    _("The text this command should produce"),
		    "", flags | G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property(object_class, PROP_CHANGED,
	    g_param_spec_boolean("changed", _("Changed"),
		    _("Whether the transcript text and expected text differ in this node"),
		    FALSE, flags | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property(object_class, PROP_TEMP,
	    g_param_spec_boolean("temp", _("Unlocked"),
		    _("Whether this node is unlocked"),
		    TRUE, flags | G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property(object_class, PROP_SCORE,
	    g_param_spec_int("score", _("Score"),
		    _("This node's score, used for cleaning up the skein"),
		    G_MININT16, G_MAXINT16, 0, flags | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	
    /* Private data */
    g_type_class_add_private(klass, sizeof(I7NodePrivate));
}

I7Node *
i7_node_new(const gchar *line, const gchar *label, const gchar *transcript, 
    const gchar *expected, gboolean played, gboolean changed, gboolean temp, 
    int score, GooCanvasItemModel *skein_group)
{
	I7Node *retval = g_object_new(I7_TYPE_NODE, 
	    "line", line, 
	    "label", label, 
	    "text-transcript", transcript,
	    "text-expected", expected,
	    "changed", changed,
	    "temp", temp,
	    "score", score,
	    NULL);
	I7_NODE_USE_PRIVATE(retval, priv);
	priv->played = played;
	g_object_set(priv->node_group, "parent", skein_group, NULL);
	retval->gnode = g_node_new(retval);
	return retval;
}

gchar *
i7_node_get_line(I7Node *node)
{
    return g_strdup(I7_NODE_PRIVATE(node)->line);
}

void
i7_node_set_line(I7Node *node, const gchar *line)
{
    I7_NODE_USE_PRIVATE(node, priv);
	g_free(priv->line);
	priv->line = g_strdup(line? line : ""); /* silently accept NULL */

	/* Update the graphics */
	g_object_set(priv->line_item, "text", priv->line, NULL);
	
	/* Update the layout */
    priv->width = -1.0;
    priv->linewidth = -1.0;
    priv->labelwidth = -1.0;
	
	g_object_notify(G_OBJECT(node), "line");
}

gchar *
i7_node_get_label(I7Node *node)
{
    return g_strdup(I7_NODE_PRIVATE(node)->label);
}

void
i7_node_set_label(I7Node *node, const gchar *label)
{
    I7_NODE_USE_PRIVATE(node, priv);
    g_free(priv->label);
	priv->label = g_strdup(label? label : ""); /* silently accept NULL */

	/* Update the graphics */
	g_object_set(priv->label_item, "text", priv->label, NULL);

	/* Update the layout */
    priv->width = -1.0;
    priv->linewidth = -1.0;
    priv->labelwidth = -1.0;
	
	g_object_notify(G_OBJECT(node), "label");
}

gboolean
i7_node_has_label(I7Node *node)
{
	I7_NODE_USE_PRIVATE(node, priv);
    return (node->gnode->parent != NULL) && priv->label && (strlen(priv->label) > 0);
}

gchar *
i7_node_get_expected_text(I7Node *node)
{
    return g_strdup(I7_NODE_PRIVATE(node)->text_expected);
}

void
i7_node_set_expected_text(I7Node *node, const gchar *text)
{
    I7_NODE_USE_PRIVATE(node, priv);

    g_free(priv->text_expected);
	priv->text_expected = g_strdup(text? text : ""); /* silently accept NULL */
	
	/* Change newline separators to \n */
	if(strstr(priv->text_expected, "\r\n")) {
		gchar **lines = g_strsplit(priv->text_expected, "\r\n", 0);
		g_free(priv->text_expected);
		priv->text_expected = g_strjoinv("\n", lines);
		g_strfreev(lines);
	}
	priv->text_expected = g_strdelimit(priv->text_expected, "\r", '\n');
	
	g_object_notify(G_OBJECT(node), "text-expected");
}

gboolean
i7_node_get_changed(I7Node *node)
{
    return I7_NODE_PRIVATE(node)->changed;
}

gboolean
i7_node_get_temporary(I7Node *node)
{
    return I7_NODE_PRIVATE(node)->temp;
}

void
i7_node_set_temporary(I7Node *node, gboolean temp)
{
    I7_NODE_PRIVATE(node)->temp = temp;
	g_object_notify(G_OBJECT(node), "temp");
}

void
i7_node_set_played(I7Node *node)
{
    I7_NODE_PRIVATE(node)->played = TRUE;
}

gchar *
i7_node_get_transcript_text(I7Node *node)
{
    return g_strdup(I7_NODE_PRIVATE(node)->text_transcript);
}

void
i7_node_set_transcript_text(I7Node *node, const gchar *transcript)
{
    I7_NODE_USE_PRIVATE(node, priv);
    g_free(priv->text_transcript);
    priv->text_transcript = g_strdup(transcript? transcript : ""); /* silently accept NULL */

	/* Change newline separators to \n */
	if(strstr(priv->text_transcript, "\r\n")) {
		gchar **lines = g_strsplit(priv->text_transcript, "\r\n", 0);
		g_free(priv->text_transcript);
		priv->text_transcript = g_strjoinv("\n", lines);
		g_strfreev(lines);
	}
	priv->text_transcript = g_strdelimit(priv->text_transcript, "\r", '\n');
	
	g_object_notify(G_OBJECT(node), "text-transcript");
}

void
i7_node_bless(I7Node *node)
{
    I7_NODE_USE_PRIVATE(node, priv);
    g_free(priv->text_expected);
    priv->text_expected = g_strdup(priv->text_transcript);
	g_object_notify(G_OBJECT(node), "text-expected");
}

double
i7_node_get_line_width(I7Node *node, GooCanvas *canvas)
{
    I7_NODE_USE_PRIVATE(node, priv);
    GooCanvasBounds size;
    GooCanvasItem *item;
    
    if(priv->width < 0.0) {
		item = goo_canvas_get_item(canvas, priv->line_item);
		goo_canvas_item_get_bounds(item, &size);
	    priv->width = size.x2 - size.x1;
	    priv->linewidth = priv->width;
	    
	    if(priv->label && strlen(priv->label) > 0) {
	    	item = goo_canvas_get_item(canvas, priv->label_item);
	    	goo_canvas_item_get_bounds(item, &size);
	        priv->labelwidth = size.x2 - size.x1;
	    } else
	        priv->labelwidth = 0.0;

        if(priv->width < MIN_TEXT_WIDTH)
            priv->width = MIN_TEXT_WIDTH;
        if(priv->linewidth < MIN_TEXT_WIDTH)
            priv->linewidth = MIN_TEXT_WIDTH;
        if(priv->labelwidth < MIN_TEXT_WIDTH)
            priv->labelwidth = MIN_TEXT_WIDTH;
    }
    return priv->width;
}

gdouble
i7_node_get_line_text_width(I7Node *node)
{
    return I7_NODE_PRIVATE(node)->linewidth;
}

gdouble
i7_node_get_label_text_width(I7Node *node)
{
    return I7_NODE_PRIVATE(node)->labelwidth;
}

gdouble
i7_node_get_tree_width(I7Node *node, GooCanvas *canvas, gdouble spacing)
{
    /* Get the tree width of all children */
    int i;
    gdouble total = 0.0;
    for(i = 0; i < g_node_n_children(node->gnode); i++) {
        total += i7_node_get_tree_width(g_node_nth_child(node->gnode, i)->data, canvas, spacing);
        if(i > 0)
            total += spacing;
    }
    /* Return the largest of the above, the width of this node's line and the
    width of this node's label */
    gdouble linewidth = i7_node_get_line_width(node, canvas);
    gdouble labelwidth = i7_node_get_label_text_width(node);
    if(total > linewidth && total > labelwidth)
        return total;
    return MAX(linewidth, labelwidth);
}    

const gchar *
i7_node_get_unique_id(I7Node *node)
{
    return I7_NODE_PRIVATE(node)->id;
}

gdouble
i7_node_get_x(I7Node *node)
{
    return I7_NODE_PRIVATE(node)->x;
}

gboolean
i7_node_in_thread(I7Node *node, I7Node *endnode)
{
    return (endnode == node) || g_node_is_ancestor(node->gnode, endnode->gnode);
}

static void
write_child_pointer(GNode *gnode, GString *string)
{
	g_string_append_printf(string, "      <child nodeId=\"%s\"/>\n", I7_NODE_PRIVATE(gnode->data)->id);
}

gchar *
i7_node_get_xml(I7Node *node)
{
	I7_NODE_USE_PRIVATE(node, priv);
	
    /* Escape the following strings if necessary */
    gchar *line = g_markup_escape_text(priv->line, -1);
    gchar *text_transcript = g_markup_escape_text(priv->text_transcript, -1);
    gchar *text_expected = g_markup_escape_text(priv->text_expected, -1);
    gchar *label = g_markup_escape_text(priv->label, -1);

	GString *string = g_string_new("");
	g_string_append_printf(string, "  <item nodeId=\"%s\">\n", priv->id);
	g_string_append_printf(string, "    <command xml:space=\"preserve\">%s</command>\n", line);
	g_string_append_printf(string, "    <result xml:space=\"preserve\">%s</result>\n", text_transcript);
	g_string_append_printf(string, "    <commentary xml:space=\"preserve\">%s</commentary>\n", text_expected);
    g_string_append_printf(string, "    <played>%s</played>\n", priv->played? "YES" : "NO");
	g_string_append_printf(string, "    <changed>%s</changed>\n", priv->changed? "YES" : "NO");
	g_string_append_printf(string, "    <temporary score=\"%d\">%s</temporary>\n", priv->score, priv->temp? "YES" : "NO");
	
    if(label)
        g_string_append_printf(string, "    <annotation xml:space=\"preserve\">%s</annotation>\n", label);
    if(node->gnode->children) {
        g_string_append(string, "    <children>\n");
        g_node_children_foreach(node->gnode, G_TRAVERSE_ALL, (GNodeForeachFunc)write_child_pointer, string);
        g_string_append(string, "    </children>\n");
    }
    g_string_append(string, "  </item>\n");
    /* Free strings if necessary */
    g_free(line);
	g_free(text_transcript);
    g_free(text_expected);
    g_free(label);
	
	return g_string_free(string, FALSE); /* return cstr */
}

void
i7_node_layout(I7Node *node, gpointer skeinptr, GooCanvas *canvas, gdouble x)
{
	I7_NODE_USE_PRIVATE(node, priv);
	I7Skein *skein = I7_SKEIN(skeinptr);
	GooCanvasBounds size;
    GooCanvasItem *item;
    gdouble width, height, hspacing, vspacing;
	gchar *path;
	
	g_object_get(skein, 
		"horizontal-spacing", &hspacing, 
		"vertical-spacing", &vspacing,
		NULL);
	
    /* Store the centre x coordinate for this node */
    priv->x = x;
    
    /* Move this node's item models to their proper places, now that we can use
    the canvas to calculate them */
    item = goo_canvas_get_item(canvas, priv->line_item);
    goo_canvas_item_get_bounds(item, &size);
    width = size.x2 - size.x1;
    height = size.y2 - size.y1;
    /* Move the label, its background, and the differs badge */
    goo_canvas_item_model_translate(priv->label_item, 0, -height);
    goo_canvas_item_model_translate(priv->label_shape_item, 0, -height);
    goo_canvas_item_model_translate(priv->badge_item, width / 2, -height / 2);
    
    /* Draw the text background */
    path = g_strdup_printf("M %.1f -%.1f "
    	"a %.1f,%.1f 0 0,1 0,%.1f "
    	"h -%.1f "
    	"a %.1f,%.1f 0 0,1 0,-%.1f "
    	"Z",
    	width / 2, height / 2, height / 2, height / 2, height, width,
    	height / 2, height / 2, height);
    g_object_set(priv->line_shape_item, "data", path, NULL);
    g_free(path);
    
    /* Draw the label background */
    if(priv->label && *priv->label) {
    	item = goo_canvas_get_item(canvas, priv->label_item);
		goo_canvas_item_get_bounds(item, &size);
		width = size.x2 - size.x1;
		height = size.y2 - size.y1;
		path = g_strdup_printf("M %.1f,%.1f "
			"a %.1f,%.1f 0 0,0 -%.1f,-%.1f "
			"h -%.1f "
			"a %.1f,%.1f 0 0,0 -%.1f,%.1f "
			"Z",
			width / 2 + height, height / 2, height, height, height, height,
			width, height, height, height, height);
		g_object_set(priv->label_shape_item, 
			"data", path, 
			"visibility", GOO_CANVAS_ITEM_VISIBLE,
			NULL);
		g_free(path);
    } else {
    	g_object_set(priv->label_shape_item, 
    		"data", "", 
    		"visibility", GOO_CANVAS_ITEM_HIDDEN,
    		NULL);
    }
    
    /* Show or hide the differs badge */
    if(priv->changed && priv->text_expected && *priv->text_expected)
    	g_object_set(priv->badge_item, 
    		"pixbuf", skein->differs_badge,
    		"visibility", GOO_CANVAS_ITEM_VISIBLE, 
    		NULL);
    else
    	g_object_set(priv->badge_item, 
    		"pixbuf", skein->differs_badge,
    		"visibility", GOO_CANVAS_ITEM_HIDDEN, 
    		NULL);

    
    /* Find the total width of all descendant nodes */
    int i;
    gdouble total = i7_node_get_tree_width(node, canvas, hspacing);
    /* Lay out each child node */
    gdouble child_x = 0.0;
    for(i = 0; i < g_node_n_children(node->gnode); i++) {
        I7Node *child = g_node_nth_child(node->gnode, i)->data;
        gdouble treewidth = i7_node_get_tree_width(child, canvas, hspacing);
        i7_node_layout(child, skein, canvas, x - total * 0.5 + child_x + treewidth * 0.5);
        child_x += treewidth + hspacing;
    }

	/* Move the node's group to its proper place */
	gdouble y = (gdouble)(g_node_depth(node->gnode) - 1) * vspacing;
	goo_canvas_item_model_translate(priv->node_group, priv->x, y);
}  

/* DEBUG */
void
i7_node_dump(I7Node *node)
{
	g_printerr("(%s)", I7_NODE_PRIVATE(node)->line);
}
