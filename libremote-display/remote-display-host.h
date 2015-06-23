/*
 * Copyright (C) 2015 Bastien Nocera <hadess@hadess.net>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option) any
 * later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this package; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __REMOTE_DISPLAY_HOST_H__
#define __REMOTE_DISPLAY_HOST_H__

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define REMOTE_DISPLAY_TYPE_HOST remote_display_host_get_type ()
G_DECLARE_DERIVABLE_TYPE (RemoteDisplayHost, remote_display_host, REMOTE_DISPLAY, HOST, GObject)

struct _RemoteDisplayHostClass
{
	GObjectClass parent_class;
};

typedef struct _RemoteDisplayHostPrivate RemoteDisplayHostPrivate;
typedef struct _RemoteDisplayHost      RemoteDisplayHost;
typedef struct _RemoteDisplayHostClass RemoteDisplayHostClass;

RemoteDisplayHost *remote_display_host_new (GInetAddress *remote_address,
					    GInetAddress *local_address);
char *remote_display_host_file (RemoteDisplayHost  *host,
				const char         *uri,
				GError            **error);

G_END_DECLS

#endif /* __REMOTE_DISPLAY_HOST_H__ */
