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

#ifndef REMOTE_DISPLAY_ERROR_H
#define REMOTE_DISPLAY_ERROR_H

#include <glib.h>

G_BEGIN_DECLS

/**
 * REMOTE_DISPLAY_ERROR:
 *
 * Error domain for libremote-display. Errors from this domain will be from
 * the #RemoteDisplayError enumeration.
 * See #GError for more information on error domains.
 **/
#define REMOTE_DISPLAY_ERROR (remote_display_error_quark ())

/**
 * RemoteDisplayError:
 * @REMOTE_DISPLAY_ERROR_NOT_SUPPORTED: The request made was not supported.
 * @REMOTE_DISPLAY_ERROR_INVALID_ARGUMENTS: The request made contained invalid arguments.
 * @REMOTE_DISPLAY_ERROR_INTERNAL_SERVER: The server encountered an (possibly unrecoverable) internal error.
 *
 * Error codes returned by remote-display functions.
 **/
typedef enum {
	REMOTE_DISPLAY_ERROR_PARSE,
	REMOTE_DISPLAY_ERROR_NOT_SUPPORTED,
	REMOTE_DISPLAY_ERROR_INVALID_ARGUMENTS,
	REMOTE_DISPLAY_ERROR_INTERNAL_SERVER
} RemoteDisplayError;

GQuark remote_display_error_quark (void);

G_END_DECLS

#endif /* REMOTE_DISPLAY_ERROR_H */
