/*
 * filter_boxblur.c -- blur filter
 * Copyright (C) ?-2007 Leny Grisel <leny.grisel@laposte.net>
 * Copyright (C) 2007 Jean-Baptiste Mardelle <jb@ader.ch>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


static void PreCompute(uint8_t *image, int32_t *rgb, int width, int height)
{
	register int x, y, z;
	int32_t pts[3];
	
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pts[0] = image[0];
			pts[1] = image[1];
			pts[2] = image[2];
			for (z = 0; z < 3; z++) 
			{
				if (x > 0) pts[z] += rgb[-3];
				if (y > 0) pts[z] += rgb[width * -3];
				if (x>0 && y>0) pts[z] -= rgb[(width + 1) * -3];
				*rgb++ = pts[z];
            }
			image += 3;
		}
	}
}

static int32_t GetRGB(int32_t *rgb, unsigned int w, unsigned int h, unsigned int x, int offsetx, unsigned int y, int offsety, unsigned int z)
{
	int xtheo = x * 1 + offsetx;
	int ytheo = y + offsety;
	if (xtheo < 0) xtheo = 0; else if (xtheo >= w) xtheo = w - 1;
	if (ytheo < 0) ytheo = 0; else if (ytheo >= h) ytheo = h - 1;
	return rgb[3*(xtheo+ytheo*w)+z];
}

static void DoBoxBlur(uint8_t *image, int32_t *rgb, unsigned int width, unsigned int height, unsigned int boxw, unsigned int boxh)
{
	register int x, y;
	float mul = 1.f / ((boxw*2) * (boxh*2));

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			*image++ = (GetRGB(rgb, width, height, x, +boxw, y, +boxh, 0)
			          + GetRGB(rgb, width, height, x, -boxw, y, -boxh, 0)
			          - GetRGB(rgb, width, height, x, -boxw, y, +boxh, 0)
			          - GetRGB(rgb, width, height, x, +boxw, y, -boxh, 0)) * mul;
			*image++ = (GetRGB(rgb, width, height, x, +boxw, y, +boxh, 1)
			          + GetRGB(rgb, width, height, x, -boxw, y, -boxh, 1)
			          - GetRGB(rgb, width, height, x, -boxw, y, +boxh, 1)
			          - GetRGB(rgb, width, height, x, +boxw, y, -boxh, 1)) * mul;
			*image++ = (GetRGB(rgb, width, height, x, +boxw, y, +boxh, 2)
			          + GetRGB(rgb, width, height, x, -boxw, y, -boxh, 2)
			          - GetRGB(rgb, width, height, x, -boxw, y, +boxh, 2)
			          - GetRGB(rgb, width, height, x, +boxw, y, -boxh, 2)) * mul;
		}
	}
}

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the image
	*format = mlt_image_rgb24;
	int error = mlt_frame_get_image( this, image, format, width, height, 1 );
	short hori = mlt_properties_get_int( MLT_FRAME_PROPERTIES( this ), "hori" );
	short vert = mlt_properties_get_int( MLT_FRAME_PROPERTIES( this ), "vert" );

	// Only process if we have no error and a valid colour space
	if ( error == 0 )
	{
		double factor = mlt_properties_get_double( MLT_FRAME_PROPERTIES( this ), "boxblur" );
		if ( factor != 0)
		{
			int h = *height + 1;
			int32_t *rgb = mlt_pool_alloc( 3 * *width * h * sizeof(int32_t) );
			PreCompute( *image, rgb, *width, h );
			DoBoxBlur( *image, rgb, *width, h, (int) factor * hori, (int) factor * vert );
			mlt_pool_release( rgb );
		}
	}
	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	// Get the starting blur level
	double blur = (double) mlt_properties_get_int( MLT_FILTER_PROPERTIES( this ), "start" );
	short hori = mlt_properties_get_int(MLT_FILTER_PROPERTIES( this ), "hori" );
	short vert = mlt_properties_get_int(MLT_FILTER_PROPERTIES( this ), "vert" );

	// If there is an end adjust gain to the range
	if ( mlt_properties_get( MLT_FILTER_PROPERTIES( this ), "end" ) != NULL )
	{
		// Determine the time position of this frame in the transition duration
		double end = (double) mlt_properties_get_int( MLT_FILTER_PROPERTIES( this ), "end" );
		blur += ( end - blur ) * mlt_filter_get_progress( this, frame );
	}

	// Push the frame filter
	mlt_properties_set_double( MLT_FRAME_PROPERTIES( frame ), "boxblur", blur );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "hori", hori );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "vert", vert );
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_boxblur_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "start", arg == NULL ? "2" : arg);
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "hori", arg == NULL ? "1" : arg);
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "vert", arg == NULL ? "1" : arg);
	}
	return this;
}



