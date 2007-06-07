/*
 * runtime.cpp: Core surface and canvas definitions.
 *
 * Author:
 *   Miguel de Icaza (miguel@novell.com)
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */
#include <config.h>
#include <string.h>
#include <gtk/gtk.h>
#include <malloc.h>
#include <glib.h>
#include <stdlib.h>
#include <gdk/gdkx.h>
#if AGG
#    include <agg_rendering_buffer.h>
#    include "Agg2D.h"
#else
#    define CAIRO 1
#endif

#include <cairo-xlib.h>
#include "runtime.h"
#include "transform.h"
#include "animation.h"

#if AGG
struct _SurfacePrivate {
	agg::rendering_buffer rbuf;
	Agg2D *graphics;
};
#endif

NameScope *global_NameScope;

void 
base_ref (Base *base)
{
	if (base->refcount & BASE_FLOATS)
		base->refcount = 1;
	else
		base->refcount++;
}

void
base_unref (Base *base)
{
	if (base->refcount == BASE_FLOATS || base->refcount == 1){
		delete base;
	} else
		base->refcount--;
}

void 
collection_add (Collection *collection, void *data)
{
	if (collection->add_fn)
		collection->add_fn (collection, data);
	collection->list = g_slist_append (collection->list, data);
}

void 
collection_remove (Collection *collection, void *data)
{
	if (collection->remove_fn)
		collection->remove_fn (collection, data);

	collection->list = g_slist_remove (collection->list, data);
}


Point
point_from_str (const char *s)
{
	// FIXME - not robust enough for production
	char *next = NULL;
	double x = strtod (s, &next);
	double y = 0.0;
	if (next)
		y = strtod (++next, NULL);
	return Point (x, y);
}

Rect
rect_from_str (const char *s)
{
	// FIXME - not robust enough for production
	char *next = NULL;
	double x = strtod (s, &next);
	double y = 0.0;
	if (next)
		y = strtod (++next, &next);
	double w = 0.0;
	if (next)
		w = strtod (++next, &next);
	double h = 0.0;
	if (next)
		h = strtod (++next, &next);
	return Rect (x, y, w, h);
}

/**
 * Value implementation
 */

void
Value::Init ()
{
	memset (&u, 0, sizeof (u));
}

Value::Value()
  : k (INVALID)
{
}

Value::Value(bool z)
{
	Init ();
	k = BOOL;
	u.z = z;
}

Value::Value (double d)
{
	Init ();
	k = DOUBLE;
	u.d = d;
}

Value::Value (guint64 i)
{
	Init ();
	k = UINT64;
	u.ui64 = i;
}

Value::Value (gint64 i)
{
	Init ();
	k = INT64;
	u.i64 = i;
}

Value::Value (gint32 i)
{
	Init ();
	k = INT32;
	u.i32 = i;
}

Value::Value (Color c)
{
	Init ();
	k = COLOR;
	u.color = new Color (c);
}

Value::Value (DependencyObject *obj)
{
	g_assert (obj != NULL);
		
	Init ();
	k = DEPENDENCY_OBJECT;
	u.dependency_object = obj;
}

Value::Value (Point pt)
{
	Init ();
	k = POINT;
	u.point = new Point (pt);
}

Value::Value (Rect rect)
{
	Init ();
	k = RECT;
	u.rect = new Rect (rect);
}

Value::Value (RepeatBehavior repeat)
{
	Init();
	k = REPEATBEHAVIOR;
	u.repeat = new RepeatBehavior (repeat);
}

Value::Value (Duration duration)
{
	Init();
	k = DURATION;
	u.duration = new Duration (duration);
}

Value::Value (const char* s)
{
	Init ();
	k = STRING;
	u.s= g_strdup (s);
}


Value::~Value ()
{
	if (k == STRING)
		g_free (u.s);
}



/**
 * item_getbounds:
 * @item: the item to update the bounds of
 *
 * Does this by requesting bounds update to all of its parents. 
 */
void
item_update_bounds (UIElement *item)
{
	double cx1 = item->x1;
	double cy1 = item->y1;
	double cx2 = item->x2;
	double cy2 = item->y2;
	
	item->getbounds ();

	//
	// If we changed, notify the parent to recompute its bounds
	//
	if (item->x1 != cx1 || item->y1 != cy1 || item->y2 != cy2 || item->x2 != cx2){
		if (item->parent != NULL)
			item_update_bounds (item->parent);
	}
}

UIElement::~UIElement ()
{
	printf ("FIXME: We should go through all of the attached properties and unref them\n");
}

void
UIElement::get_xform_for (UIElement *item, cairo_matrix_t *result)
{
	printf ("get_xform_for called on a non-container, you must implement this in your container\n");
	exit (1);
}

void 
item_invalidate (UIElement *item)
{
	double res [6];
	Surface *s = item_get_surface (item);

	if (s == NULL)
		return;

//#define DEBUG_INVALIDATE
#ifdef DEBUG_INVALIDATE
	printf ("Requesting invalidate for %d %d %d %d\n", 
				    (int) item->x1, (int)item->y1, 
				    (int)(item->x2-item->x1+1), (int)(item->y2-item->y1+1));
#endif
	// 
	// Note: this is buggy: why do we need to queue the redraw on the toplevel
	// widget (s->data) and does not work with the drawing area?
	//
	gtk_widget_queue_draw_area ((GtkWidget *)s->drawing_area, 
				    (int) item->x1, (int)item->y1, 
				    (int)(item->x2-item->x1+2), (int)(item->y2-item->y1+2));
}

void 
item_set_transform_origin (UIElement *item, Point p)
{
	item_invalidate (item);
	
	item->user_xform_origin = p;

	item->update_xform ();
	item->getbounds ();

	item_invalidate (item);
}

void
item_get_render_affine (UIElement *item, cairo_matrix_t *result)
{
	Value* v = item->GetValue (UIElement::RenderTransformProperty);
	if (v == NULL)
		cairo_matrix_init_identity (result);
	else {
		Transform *t = (Transform*)v->u.dependency_object;
		t->GetTransform (result);
	}
}

void
UIElement::update_xform ()
{
	cairo_matrix_t user_transform;

	//
	// What is more important, the space used by 6 doubles,
	// or the time spent calling the parent (that will call
	// DependencyObject->GetProperty to get the positions?
	//
	// Currently we go for thiner, but if we decide to go
	// for reduced computation, we should introduce the 
	// base transform in UIElement that will be updated by the
	// container on demand
	//
	if (parent != NULL)
		parent->get_xform_for (this, &absolute_xform);
	else
		cairo_matrix_init_identity (&absolute_xform);

	item_get_render_affine (this, &user_transform);

	Point p = getxformorigin ();
	cairo_matrix_translate (&absolute_xform, p.x, p.y);
	cairo_matrix_multiply (&absolute_xform, &user_transform, &absolute_xform);
	cairo_matrix_translate (&absolute_xform, -p.x, -p.y);
}

void
UIElement::OnSubPropertyChanged (DependencyProperty *prop, DependencyProperty *subprop)
{
	if (prop == UIElement::RenderTransformProperty) {
		item_invalidate (this);
		update_xform ();
		getbounds ();
		item_invalidate (this);
	}
}

void
item_set_render_transform (UIElement *item, Transform *transform)
{
	item->SetValue (UIElement::RenderTransformProperty, transform);
}

double
framework_element_get_height (FrameworkElement *framework_element)
{
	return framework_element->GetValue (FrameworkElement::HeightProperty)->u.d;
}

void
framework_element_set_height (FrameworkElement *framework_element, double height)
{
	framework_element->SetValue (FrameworkElement::HeightProperty, Value (height));
}

double
framework_element_get_width (FrameworkElement *framework_element)
{
	return framework_element->GetValue (FrameworkElement::WidthProperty)->u.d;
}

void
framework_element_set_width (FrameworkElement *framework_element, double width)
{
	framework_element->SetValue (FrameworkElement::WidthProperty, Value (width));
}

Surface *
item_get_surface (UIElement *item)
{
	if (item->flags & Video::IS_CANVAS){
		Canvas *canvas = (Canvas *) item;
		if (canvas->surface)
			return canvas->surface;
	}

	if (item->parent != NULL)
		return item_get_surface (item->parent);

	return NULL;
}

static void
panel_child_add (Collection *col, void *datum)
{
	Panel *panel = (Panel *) col->closure;
	UIElement *item = (UIElement *) datum;

	item->parent = panel;
	item->update_xform ();
	panel->getbounds ();
	item_invalidate (panel);
}

static void
panel_child_remove (Collection *col, void *datum)
{
	Panel *panel = (Panel *) col->closure;
	
	item_invalidate (panel);
	panel->getbounds ();
}

void 
panel_child_add (Panel *panel, UIElement *item)
{
	collection_add (&panel->children, item);
}

Panel::Panel ()
{
	children.Setup (panel_child_add, panel_child_remove, this);
}

Canvas::Canvas ()
{
	flags |= IS_CANVAS;
}

void
Canvas::get_xform_for (UIElement *item, cairo_matrix_t *result)
{
	*result = absolute_xform;

	// Compute left/top if its attached to the item
	Value *val_top = item->GetValue (Canvas::TopProperty);
	double top = val_top == NULL ? 0.0 : val_top->u.d;

	Value *val_left = item->GetValue (Canvas::LeftProperty);
	double left = val_left == NULL ? 0.0 : val_left->u.d;
		
	cairo_matrix_translate (result, left, top);
}

void
Canvas::update_xform ()
{
	UIElement::update_xform ();
	GSList *il;

	for (il = children.list; il != NULL; il = il->next){
		UIElement *item = (UIElement *) il->data;

		item->update_xform ();
	}
}

void
Canvas::getbounds ()
{
	bool first = TRUE;
	GSList *il;

	for (il = children.list; il != NULL; il = il->next){
		UIElement *item = (UIElement *) il->data;

		item->getbounds ();
		if (first){
			x1 = item->x1;
			x2 = item->x2;
			y1 = item->y1;
			y2 = item->y2;
			first = FALSE;
			continue;
		} 

		if (item->x1 < x1)
			x1 = item->x1;
		if (item->x2 > x2)
			x2 = item->x2;
		if (item->y1 < y1)
			y1 = item->y1;
		if (item->y2 > y2)
			y2 = item->y2;
	}

	// If we found nothing.
	if (first){
		x1 = y1 = x2 = y2 = 0;
	}
}

void 
Canvas::OnSubPropertyChanged (DependencyProperty *prop, DependencyProperty *subprop)
{
	printf ("Prop %s changed in %s\n", prop->name, subprop->name);
}

void 
surface_clear (Surface *s, int x, int y, int width, int height)
{
	static unsigned char n;
	cairo_matrix_t identity;

	cairo_matrix_init_identity (&identity);

	cairo_set_matrix (s->cairo, &identity);

	cairo_set_source_rgba (s->cairo, 0.7, 0.7, 0.7, 1.0);
	cairo_rectangle (s->cairo, x, y, width, height);
	cairo_fill (s->cairo);
}
	
void
surface_clear_all (Surface *s)
{
	memset (s->buffer, 0, s->width * s->height * 4);
}

static void
surface_realloc (Surface *s)
{
	if (s->buffer)
		free (s->buffer);

	int size = s->width * s->height * 4;
	s->buffer = (unsigned char *) malloc (size);
	surface_clear_all (s);
       
	s->cairo_buffer_surface = cairo_image_surface_create_for_data (
		s->buffer, CAIRO_FORMAT_ARGB32, s->width, s->height, s->width * 4);

	s->cairo_buffer = cairo_create (s->cairo_buffer_surface);
	s->cairo = s->cairo_buffer;
}

void 
surface_destroy (Surface *s)
{
	base_unref (s->toplevel);

	cairo_destroy (s->cairo_buffer);
	if (s->cairo_xlib)
		cairo_destroy (s->cairo_xlib);

	if (s->pixmap != NULL)
		gdk_pixmap_unref (s->pixmap);
	
	cairo_surface_destroy (s->cairo_buffer_surface);
	if (s->xlib_surface)
		cairo_surface_destroy (s->xlib_surface);

	gtk_widget_destroy (s->drawing_area);

	// TODO: add everything
	delete s;
}

void
create_xlib (Surface *s, GtkWidget *widget)
{
	s->pixmap = gdk_pixmap_new (GDK_DRAWABLE (widget->window), s->width, s->height, -1);

	s->xlib_surface = cairo_xlib_surface_create (
		GDK_WINDOW_XDISPLAY(widget->window),
		GDK_WINDOW_XWINDOW(GDK_DRAWABLE (s->pixmap)),
		GDK_VISUAL_XVISUAL (gdk_window_get_visual(widget->window)),
		s->width, s->height);

	s->cairo_xlib = cairo_create (s->xlib_surface);
}

gboolean
realized_callback (GtkWidget *widget, gpointer data)
{
	Surface *s = (Surface *) data;
	cairo_surface_t *xlib;

	create_xlib (s, widget);

	s->cairo = s->cairo_xlib;
}

gboolean
unrealized_callback (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	Surface *s = (Surface *) data;

	cairo_surface_destroy(s->xlib_surface);
	s->xlib_surface = NULL;

	s->cairo = s->cairo_buffer;
}

gboolean
expose_event_callback (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	Surface *s = (Surface *) data;

	s->frames++;

	if (event->area.x > s->width || event->area.y > s->height)
		return TRUE;

	//
	// BIG DEBUG BLOB
	// 
	if (cairo_status (s->cairo) != CAIRO_STATUS_SUCCESS){
		printf ("expose event: the cairo context has an error condition and refuses to paint: %s\n", 
			cairo_status_to_string (cairo_status (s->cairo)));
	}

//#define DEBUG_INVALIDATE
#ifdef DEBUG_INVALIDATE
	printf ("Got a request to repaint at %d %d %d %d\n", event->area.x, event->area.y, event->area.width, event->area.height);
#endif

	surface_repaint (s, event->area.x, event->area.y, event->area.width, event->area.height);
	gdk_draw_drawable (
		widget->window, gtk_widget_get_style (widget)->white_gc, s->pixmap, 
		event->area.x, event->area.y, // gint src_x, gint src_y,
		event->area.x, event->area.y, // gint dest_x, gint dest_y,
		MIN (event->area.width, s->width),
		MIN (event->area.height, s->height));

	return TRUE;
}

void
Canvas::render (Surface *s, int x, int y, int width, int height)
{
	GSList *il;
	double actual [6];
	
	for (il = children.list; il != NULL; il = il->next){
		UIElement *item = (UIElement *) il->data;

		item->render (s, x, y, width, height);
	}
}

Canvas *
canvas_new ()
{
	return new Canvas ();
}

Surface *
surface_new (int width, int height)
{
	Surface *s = new Surface ();

	s->drawing_area = gtk_drawing_area_new ();
	gtk_widget_set_double_buffered (s->drawing_area, FALSE);

	gtk_widget_show (s->drawing_area);

	gtk_widget_set_usize (s->drawing_area, width, height);
	s->buffer = NULL;
	s->width = width;
	s->height = height;
	s->toplevel = NULL;

	surface_realloc (s);

	gtk_signal_connect (GTK_OBJECT (s->drawing_area), "expose_event",
			    G_CALLBACK (expose_event_callback), s);

	gtk_signal_connect (GTK_OBJECT (s->drawing_area), "realize",
			    G_CALLBACK (realized_callback), s);

	gtk_signal_connect (GTK_OBJECT (s->drawing_area), "unrealize",
			    G_CALLBACK (unrealized_callback), s);

	return s;
}

void
surface_attach (Surface *surface, UIElement *toplevel)
{
	if (!(toplevel->flags & UIElement::IS_CANVAS)){
		printf ("Unsupported toplevel\n");
		return;
	}
	if (surface->toplevel)
		base_unref (surface->toplevel);

	Canvas *canvas = (Canvas *) toplevel;
	base_ref (canvas);
	canvas->surface = surface;
	surface->toplevel = canvas;
}

void
surface_repaint (Surface *s, int x, int y, int width, int height)
{
	surface_clear (s, x, y, width, height);
	s->toplevel->render (s, x, y, width, height);
}

void *
surface_get_drawing_area (Surface *s)
{
	return s->drawing_area;
}

/*
	DependencyObject
*/

GHashTable *DependencyObject::properties = NULL;

typedef struct {
	DependencyObject *dob;
	DependencyProperty *prop;
} Attachee;

void
DependencyObject::NotifyAttacheesOfPropertyChange (DependencyProperty *subproperty)
{
	for (GSList *l = attached_list; l != NULL; l = l->next){
		Attachee *att = (Attachee*)l->data;

		att->dob->OnSubPropertyChanged (att->prop, subproperty);
	}
}

void
DependencyObject::SetValue (DependencyProperty *property, Value value)
{
	g_return_if_fail (property != NULL);

	if (property->value_type < Value::DEPENDENCY_OBJECT)
		g_return_if_fail (property->value_type == value.k);

	Value *current_value = (Value*)g_hash_table_lookup (current_values, property->name);

	if (current_value == NULL || *current_value != value.k) {
		Value *copy = g_new (Value, 1);
		*copy = value;

		if (value.k == Value::STRING)
			copy->u.s = g_strdup (value.u.s);

		if (current_value != NULL && current_value->k >= Value::DEPENDENCY_OBJECT){
			DependencyObject *dob = current_value->u.dependency_object;

			for (GSList *l = dob->attached_list; l; l = l->next) {
				Attachee *att = (Attachee*)l->data;
				if (att->dob == this && att->prop == property) {
					dob->attached_list = g_slist_remove_link (dob->attached_list, l);
					delete att;
					break;
				}
			}
		}

		if (value.k >= Value::DEPENDENCY_OBJECT){
			DependencyObject *dob = value.u.dependency_object;
			Attachee *att = new Attachee ();
			att->dob = this;
			att->prop = property;
			dob->attached_list = g_slist_append (dob->attached_list, att);
		}
		g_hash_table_insert (current_values, property->name, copy);

		// 
		//NotifyAttacheesOfPropertyChange (property);
		OnPropertyChanged (property);
	}
}

Value *
DependencyObject::GetValue (DependencyProperty *property)
{
	Value *value = NULL;

	value = (Value *) g_hash_table_lookup (current_values, property->name);

	if (value != NULL)
		return value;

	return property->default_value;
}

DependencyObject::DependencyObject ()
{
	this->objectType = Value::INVALID;
	current_values = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_free);
	events = new EventObject ();
	this->attached_list = NULL;
}

DependencyObject::~DependencyObject ()
{
	g_hash_table_destroy (current_values);
	delete events;
}

void
DependencyObject::SetObjectType (Value::Kind objectType)
{
	this->objectType = objectType;
}

DependencyProperty *
DependencyObject::GetDependencyProperty (char *name)
{
	return DependencyObject::GetDependencyProperty (objectType, name);
}

DependencyProperty *
DependencyObject::GetDependencyProperty (Value::Kind type, char *name)
{
	GHashTable *table;
	DependencyProperty *property;

	if (properties == NULL)
		return NULL;

	table = (GHashTable*) g_hash_table_lookup (properties, &type);

	if (table == NULL)
		return NULL;

	property = (DependencyProperty*) g_hash_table_lookup (table, name);

	return property;	
}

DependencyObject*
DependencyObject::FindName (char *name)
{
	NameScope *scope = NameScope::GetNameScope (this);
	if (!scope)
		scope = global_NameScope;

	return scope->FindName (name);
}

//
// Use this for values that can be null
//
DependencyProperty *
DependencyObject::Register (Value::Kind type, char *name, Value::Kind vtype)
{
	g_return_val_if_fail (name != NULL, NULL);

	return RegisterFull (type, name, NULL, vtype);
}

//
// DependencyObject takes ownership of the Value * for default_value
//
DependencyProperty *
DependencyObject::Register (Value::Kind type, char *name, Value *default_value)
{
	g_return_val_if_fail (default_value != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	return RegisterFull (type, name, default_value, default_value->k);
}

DependencyProperty *
DependencyObject::RegisterFull (Value::Kind type, char *name, Value *default_value, Value::Kind vtype)
{
	GHashTable *table;

	DependencyProperty *property = new DependencyProperty (name, default_value, vtype);
	property->type = type;
	
	/* first add the property to the global 2 level property hash */
	if (NULL == properties)
		properties = g_hash_table_new (g_int_hash, g_int_equal);

	table = (GHashTable*) g_hash_table_lookup (properties, &property->type);

	if (table == NULL) {
		table = g_hash_table_new (g_str_hash, g_str_equal);
		g_hash_table_insert (properties, &property->type, table);
	}

	g_hash_table_insert (table, property->name, property);

	return property;
}

Value::Kind
DependencyObject::GetObjectType ()
{
	return objectType;
}

/*
	DependencyProperty
*/
DependencyProperty::DependencyProperty (char *name, Value *default_value, Value::Kind kind)
{
	this->name = g_strdup (name);
	this->default_value = default_value;
	this->value_type = kind;
}

DependencyProperty::~DependencyProperty ()
{
	g_free (name);
	if (default_value != NULL)
		g_free (default_value);
}

// event handlers for c++
typedef struct {
	EventHandler func;
	gpointer data;
} EventClosure;

EventObject::EventObject ()
{
	event_hash = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
free_closure_list (gpointer key, gpointer data, gpointer userdata)
{
	g_free (key);
	g_list_foreach ((GList*)data, (GFunc)g_free, NULL);
}

EventObject::~EventObject ()
{
	g_hash_table_foreach (event_hash, free_closure_list, NULL);
}

void
EventObject::AddHandler (char *event_name, EventHandler handler, gpointer data)
{
	GList *events = (GList*)g_hash_table_lookup (event_hash, event_name);

	EventClosure *closure = new EventClosure ();
	closure->func = handler;
	closure->data = data;

	if (events == NULL) {
		g_hash_table_insert (event_hash, g_strdup (event_name), g_list_prepend (NULL, closure));
	}
	else {
		events = g_list_append (events, closure); // not prepending means we don't need to g_hash_table_replace
	}
}

void
EventObject::RemoveHandler (char *event_name, EventHandler handler, gpointer data)
{
	GList *events = (GList*)g_hash_table_lookup (event_hash, event_name);

	if (events == NULL)
		return;

	GList *l;
	for (l = events; l; l = l->next) {
		EventClosure *closure = (EventClosure*)l->data;
		if (closure->func == handler && closure->data == data)
			break;
	}

	if (l == NULL) /* we didn't find it */
		return;

	g_free (l->data);
	events = g_list_delete_link (events, l);

	if (events == NULL) {
		/* delete the event */
		gpointer key, value;
		g_hash_table_lookup_extended (event_hash, event_name, &key, &value);
		g_free (key);
	}
	else {
		g_hash_table_replace (event_hash, event_name, events);
	}
}

void
EventObject::Emit (char *event_name)
{
	GList *events = (GList*)g_hash_table_lookup (event_hash, event_name);

	if (events == NULL)
		return;
	
	for (GList *l = events; l; l = l->next) {
		EventClosure *closure = (EventClosure*)l->data;
		closure->func (closure->data);
	}
}

NameScope::NameScope ()
{
	names = g_hash_table_new (g_str_hash, g_str_equal);
}

NameScope::~NameScope ()
{
  //	g_hash_table_foreach (/* XXX */);
}

void
NameScope::RegisterName (const char *name, DependencyObject *object)
{
	g_hash_table_insert (names, g_strdup (name) ,object);
}

void
NameScope::UnregisterName (const char *name)
{
}

DependencyObject*
NameScope::FindName (const char *name)
{
	return (DependencyObject*)g_hash_table_lookup (names, name);
}

NameScope*
NameScope::GetNameScope (DependencyObject *obj)
{
	Value *v = obj->GetValue (NameScope::NameScopeProperty);
	return v == NULL ? NULL : (NameScope*)v->u.dependency_object;
}

void
SetNameScope (DependencyObject *obj, NameScope *scope)
{
	obj->SetValue (NameScope::NameScopeProperty, scope);
}

void
EventTrigger::AddAction (TriggerAction *action)
{
	g_return_if_fail (action);

	actions = g_slist_append (actions, action);
}

EventTrigger *
event_trigger_new ()
{
	return new EventTrigger ();
}

void
event_trigger_action_add (EventTrigger *trigger, TriggerAction *action)
{
	g_return_if_fail (trigger);
	g_return_if_fail (action);

	trigger->AddAction (action);
}

DependencyProperty *NameScope::NameScopeProperty;

void
namescope_init ()
{
	global_NameScope = new NameScope ();
	NameScope::NameScopeProperty = DependencyObject::Register (Value::NAMESCOPE, "NameScope", Value::NAMESCOPE);
}

DependencyProperty* FrameworkElement::HeightProperty;
DependencyProperty* FrameworkElement::WidthProperty;

void
framework_element_init ()
{
	FrameworkElement::HeightProperty = DependencyObject::Register (Value::FRAMEWORKELEMENT, "Height", new Value (0.0));
	FrameworkElement::WidthProperty = DependencyObject::Register (Value::FRAMEWORKELEMENT, "Width", new Value (0.0));
}

DependencyProperty* Canvas::TopProperty;
DependencyProperty* Canvas::LeftProperty;

void 
canvas_init ()
{
	Canvas::TopProperty = DependencyObject::Register (Value::CANVAS, "Top", new Value (0.0));
	Canvas::LeftProperty = DependencyObject::Register (Value::CANVAS, "Left", new Value (0.0));
}

DependencyProperty* UIElement::RenderTransformProperty;

void
item_init ()
{
	UIElement::RenderTransformProperty = DependencyObject::Register (Value::UIELEMENT, "RenderTransform", Value::TRANSFORM);
}

void
runtime_init ()
{
	namescope_init ();
	item_init ();
	framework_element_init ();
	canvas_init ();
	transform_init ();
	animation_init ();
	brush_init ();
	shape_init ();
	geometry_init ();
	xaml_init ();
}
