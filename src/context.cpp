/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * context.cpp
 *
 * Copyright 2010 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 *
 */

#include <config.h>

#include "context.h"
#include "projection.h"

namespace Moonlight {

#define MIN_X -32768
#define MIN_Y MIN_X
#define MAX_W 65536
#define MAX_H MAX_W

Context::Cairo::Cairo ()
{
	r = Rect (MIN_X, MIN_Y, MAX_W, MAX_H);
}

Context::Target::Target ()
{
	native = NULL;
	box    = Rect ();
	cairo  = NULL;
	init   = NULL;
}

Context::Target::Target (MoonSurface *moon,
			 Rect        extents)
{
	native = moon->ref ();
	box    = extents;
	cairo  = NULL;
	init   = moon->ref ();
}

Context::Target::~Target ()
{
	native->unref ();

	if (cairo)
		cairo->unref ();

	if (init)
		init->unref ();
}

Rect
Context::Target::GetData (MoonSurface **ref)
{
	if (ref)
		*ref = native->ref ();

	return box;
}

cairo_surface_t *
Context::Target::Cairo ()
{
	cairo_surface_t *surface;

	if (cairo)
		return cairo->Cairo ();

	surface = native->Cairo ();

	/* set device offset */
	if (!box.IsEmpty ())
		cairo_surface_set_device_offset (surface, -box.x, -box.y);

	return surface;
}

void
Context::Target::SetCairoTarget (Target *target)
{
	Target *old = cairo;

	if (target)
		cairo = (Target *) target->ref ();
	else
		cairo = NULL;

	if (old)
		old->unref ();
}

Context::Target *
Context::Target::GetCairoTarget ()
{
	return cairo;
}

void
Context::Target::SetInit (MoonSurface *src)
{
	MoonSurface *old = init;

	if (src)
		init = src->ref ();
	else
		init = NULL;

	if (old)
		old->unref ();
}

MoonSurface *
Context::Target::GetInit ()
{
	return init;
}

Context::Node::Node (Target         *surface,
		     cairo_matrix_t *matrix,
		     const Rect     *clip)
{
	target    = (Target *) surface->ref ();
	box       = clip ? *clip : Rect (MIN_X, MIN_Y, MAX_W, MAX_H);
	transform = *matrix;
	context   = NULL;
}

Context::Node::~Node ()
{
	if (context) {
		cairo_surface_flush (cairo_get_target (context));
		cairo_destroy (context);
	}

	if (target)
		target->unref ();
}

cairo_t *
Context::Node::Cairo ()
{
	Target *surface = GetTarget ();

	if (!context) {
		cairo_surface_t *dst;

		if (box.IsEmpty ())
			dst = cairo_image_surface_create (CAIRO_FORMAT_A1, 1, 1);
		else
			dst = surface->Cairo ();

		context = cairo_create (dst);
		box.Draw (context);
		cairo_clip (context);
		cairo_set_matrix (context, &transform);

		cairo_surface_destroy (dst);
	}

	return context;
}

void
Context::Node::GetMatrix (cairo_matrix_t *matrix)
{
	*matrix = transform;
}

void
Context::Node::GetClip (Rect *clip)
{
	*clip = box;
}

Context::Target *
Context::Node::GetTarget ()
{
	return target;
}

Context::Context (MoonSurface *surface)
{
	AbsoluteTransform transform = AbsoluteTransform ();
	Rect              r = Rect (0, 0, 32768, 32768);
	Target            *target;

	target = new Target (surface, r);
	Stack::Push (new Context::Node (target, &transform.m, NULL));
	target->unref ();
}

void
Context::Push (Transform transform)
{
	cairo_matrix_t matrix;
	Rect           box;

	Top ()->GetMatrix (&matrix);
	Top ()->GetClip (&box);

	cairo_matrix_multiply (&matrix, &transform.m, &matrix);

	Stack::Push (new Context::Node (Top ()->GetTarget (), &matrix, &box));
}

void
Context::Push (AbsoluteTransform transform)
{
	cairo_matrix_t matrix = transform.m;
	Rect           box;

	Top ()->GetClip (&box);

	Stack::Push (new Context::Node (Top ()->GetTarget (), &matrix, &box));
}

void
Context::Push (Clip clip)
{
	cairo_matrix_t matrix;
	Rect           box;

	Top ()->GetMatrix (&matrix);
	Top ()->GetClip (&box);

	box = box.Intersection (clip.r);

	Stack::Push (new Context::Node (Top ()->GetTarget (), &matrix, &box));
}

void
Context::Push (Group extents)
{
	g_warning ("Context::Push has been called. The derived class should have overridden it.");
	Push (Clip ());
}

void
Context::Push (Group extents, MoonSurface *surface)
{
	cairo_matrix_t matrix;
	Target         *target = new Target (surface, extents.r);

	Top ()->GetMatrix (&matrix);

	Stack::Push (new Context::Node (target, &matrix, &extents.r));
	target->unref ();
}

cairo_t *
Context::Push (Cairo extents)
{
	Push (Clip (extents.r));
	return Top ()->Cairo ();
}

Context::Node *
Context::Top ()
{
	return (Node *) Stack::Top ();
}

void
Context::Pop ()
{
	delete Stack::Pop ();
}

Rect
Context::Pop (MoonSurface **ref)
{
	Node *node = (Node *) Stack::Pop ();
	Rect r = Rect ();

	if (Top ()->GetTarget () != node->GetTarget ())
		r = node->GetTarget ()->GetData (ref);

	delete node;

	return r;
}

bool
Context::IsImmutable ()
{
	Rect box;

	Top ()->GetClip (&box);

	return box.IsEmpty ();
}

void
Context::GetMatrix (double *out)
{
	Context::Target *target = Top ()->GetTarget ();
	Rect            r = target->GetData (NULL);
	cairo_matrix_t  ctm;

	Top ()->GetMatrix (&ctm);

	Matrix3D::Affine (out,
			  ctm.xx, ctm.xy,
			  ctm.yx, ctm.yy,
			  ctm.x0, ctm.y0);
}

void
Context::Clear (Color *color)
{
	g_warning ("Context::Clear has been called. The derived class should have overridden it.");
}

void
Context::Blend (MoonSurface *src,
		double      alpha,
		double      x,
		double      y)
{
	g_warning ("Context::Blend has been called. The derived class should have overridden it.");
}

void
Context::Project (MoonSurface  *src,
		  const double *matrix,
		  double       alpha,
		  double       x,
		  double       y)
{
	g_warning ("Context::Project has been called. The derived class should have overridden it.");
}

void
Context::Blur (MoonSurface  *src,
	       double       radius,
	       double       x,
	       double       y)
{
	g_warning ("Context::Blur has been called. The derived class should have overridden it.");
}

void
Context::DropShadow (MoonSurface *src,
		     double      dx,
		     double      dy,
		     double      radius,
		     Color       *color,
		     double      x,
		     double      y)
{
	g_warning ("Context::DropShadow has been called. The derived class should have overridden it.");
}

void
Context::ShaderEffect (MoonSurface *src,
		       PixelShader *shader,
		       Brush       **sampler,
		       int         *sampler_mode,
		       int         n_sampler,
		       Color       *constant,
		       int         n_constant,
		       int         *ddxUvDdyUvPtr,
		       double      x,
		       double      y)
{
	g_warning ("Context::ShaderEffect has been called. The derived class should have overridden it.");
}

void
Context::Flush ()
{
	g_warning ("Context::Flush has been called. The derived class should have overridden it.");
}

};
