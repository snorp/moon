/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * surface-glx.cpp
 *
 * Copyright 2010 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 *
 */

#include <config.h>

#include <string.h>

#define __MOON_GLX__

#include "surface-glx.h"

namespace Moonlight {

GLXSurface::GLXSurface (Display *dpy, XID win) : GLSurface ()
{
	XWindowAttributes attr;

	XGetWindowAttributes (dpy, win, &attr);

	display = dpy;
	window  = win;
	size[0] = attr.width;
	size[1] = attr.height;
	vid     = XVisualIDFromVisual (attr.visual);
}

GLXSurface::GLXSurface (GLsizei w, GLsizei h) : GLSurface (w, h)
{
	display = NULL;
	window  = 0;
	vid     = 0;
}

VisualID
GLXSurface::GetVisualID ()
{
	return vid;
}

void
GLXSurface::SwapBuffers ()
{
	g_assert (window);
	glXSwapBuffers (display, window);
}

void
GLXSurface::Reshape (int width, int height)
{
	size[0] = width;
	size[1] = height;

	if (texture) {
		glDeleteTextures (1, &texture);
		texture = 0;
	}

	if (data) {
		g_free (data);
		data = NULL;
	}
}

cairo_surface_t *
GLXSurface::Cairo ()
{
	g_assert (window == 0);

	return GLSurface::Cairo ();
}

bool
GLXSurface::HasTexture ()
{
	return texture != 0;
}

};
