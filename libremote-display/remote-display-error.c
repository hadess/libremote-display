/*
   Copyright (C) 2015 Bastien Nocera

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301  USA.

   Authors: Bastien Nocera <hadess@hadess.net>

 */

#include <libremote-display/remote-display-error.h>

/**
 * SECTION:remote-display-error
 * @short_description: Error helper functions
 * @include: remote-display/remote-display.h
 *
 * Contains helper functions for reporting errors to the user.
 **/

/**
 * remote_display_error_quark:
 *
 * Gets the remote-display error quark.
 *
 * Return value: a #GQuark.
 **/
GQuark
remote_display_error_quark (void)
{
	static GQuark quark;
	if (!quark)
		quark = g_quark_from_static_string ("remote_display_error");

	return quark;
}

