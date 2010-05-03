#ifndef __NODE_H__
#define __NODE_H__

#include <glib-object.h>
#include <goocanvas.h>

G_BEGIN_DECLS

#define I7_TYPE_NODE         (i7_node_get_type())
#define I7_NODE(o)           (G_TYPE_CHECK_INSTANCE_CAST((o), I7_TYPE_NODE, I7Node))
#define I7_NODE_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST((c), I7_TYPE_NODE, I7NodeClass))
#define I7_IS_NODE(o)        (G_TYPE_CHECK_INSTANCE_TYPE((o), I7_TYPE_NODE))
#define I7_IS_NODE_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE((c), I7_TYPE_NODE))
#define I7_NODE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), I7_TYPE_NODE, I7NodeClass))

typedef struct _I7NodeClass I7NodeClass;
typedef struct _I7Node I7Node;

struct _I7NodeClass {
	GObjectClass parent_class;
};

struct _I7Node {
	GObject parent_instance;
	GNode *gnode;
	GooCanvasItemModel *tree_item; /* The tree line associated with the node */
};

GType i7_node_get_type(void) G_GNUC_CONST;
I7Node *i7_node_new(const gchar *line, const gchar *label, const gchar *transcript, 
    const gchar *expected, gboolean played, gboolean changed, gboolean temp, int score,
    GooCanvasItemModel *skein_group);
gchar *i7_node_get_line(I7Node *node);
void i7_node_set_line(I7Node *node, const gchar *line);
gchar *i7_node_get_label(I7Node *node);
void i7_node_set_label(I7Node *node, const gchar *label);
gboolean i7_node_has_label(I7Node *node);
gchar *i7_node_get_expected_text(I7Node *node);
void i7_node_set_expected_text(I7Node *node, const gchar *text);
gchar *i7_node_get_transcript_text(I7Node *node);
void i7_node_set_transcript_text(I7Node *node, const gchar *transcript);
gboolean i7_node_get_changed(I7Node *node);
gboolean i7_node_get_temporary(I7Node *node);
void i7_node_set_temporary(I7Node *node, gboolean temp);
void i7_node_set_played(I7Node *node);
void i7_node_bless(I7Node *node);
gdouble i7_node_get_line_width(I7Node *node, GooCanvas *canvas);
gdouble i7_node_get_line_text_width(I7Node *node);
gdouble i7_node_get_label_text_width(I7Node *node);
gdouble i7_node_get_tree_width(I7Node *node, GooCanvas *canvas, gdouble spacing);
const gchar *i7_node_get_unique_id(I7Node *node);
gdouble i7_node_get_x(I7Node *node);
gboolean i7_node_in_thread(I7Node *node, I7Node *endnode);
gchar *i7_node_get_xml(I7Node *node);
void i7_node_layout(I7Node *node, gpointer skeinptr, GooCanvas *canvas, gdouble x);

#endif /* __NODE_H__ */
