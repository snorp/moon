/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * context-cairo.cpp
 *
 * Copyright 2010 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 *
 */

#include <config.h>

#include "context-cairo.h"

namespace Moonlight {

CairoContext::CairoContext (CairoSurface *surface) : Context (surface)
{
}

void
CairoContext::Push (Group extents)
{
	cairo_surface_t *parent = Top ()->GetTarget ()->Cairo ();
	Rect            r = extents.r.RoundOut ();
        cairo_surface_t *data =
		cairo_surface_create_similar (parent,
					      CAIRO_CONTENT_COLOR_ALPHA,
					      r.width,
					      r.height);
        MoonSurface     *surface = new CairoSurface (data, r.width, r.height);
        Target          *target = new Target (surface, extents.r);
        cairo_matrix_t  matrix;

	Top ()->GetMatrix (&matrix);

	Stack::Push (new Context::Node (target, &matrix, &extents.r));

	target->unref ();
	surface->unref ();

	cairo_surface_destroy (data);
}

void
CairoContext::Clear (Color *color)
{
	cairo_t *cr = Context::Push (Cairo ());

	cairo_save (cr);
	cairo_set_operator (cr , CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba (cr, color->r, color->g, color->b, color->a);
	cairo_paint (cr);
	cairo_restore (cr);

	Pop ();
}

void
CairoContext::Blend (MoonSurface *src,
		     double      alpha,
		     double      x,
		     double      y)
{
	cairo_surface_t *surface = src->Cairo ();
	cairo_t         *cr = Context::Push (Cairo ());

	cairo_set_source_surface (cr, surface, x, y);
	cairo_paint_with_alpha (cr, alpha);
	cairo_surface_destroy (surface);

	Pop ();
}

void
CairoContext::Flush ()
{
	cairo_surface_flush (cairo_get_target (Top ()->Cairo ()));
}

};
