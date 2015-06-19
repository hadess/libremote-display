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

#ifndef __REMOTE_DISPLAY_DEVICE_H__
#define __REMOTE_DISPLAY_DEVICE_H__

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define REMOTE_DISPLAY_TYPE_DEVICE remote_display_device_get_type ()
G_DECLARE_DERIVABLE_TYPE (RemoteDisplayDevice, remote_display_device, REMOTE_DISPLAY, DEVICE, GObject)

struct _RemoteDisplayDeviceClass
{
	GObjectClass parent_class;
#if 0
	void (* handle_frob)  (GtkFrobber *frobber,
			       guint       n_frobs);

	gpointer padding[12];
#endif
};

typedef struct _RemoteDisplayDevicePrivate RemoteDisplayDevicePrivate;
typedef struct _RemoteDisplayDevice      RemoteDisplayDevice;
typedef struct _RemoteDisplayDeviceClass RemoteDisplayDeviceClass;

typedef enum {
	REMOTE_DISPLAY_DEVICE_CAPABILITIES_NONE   = 0,
	REMOTE_DISPLAY_DEVICE_CAPABILITIES_VIDEO  = 1 << 1,
	REMOTE_DISPLAY_DEVICE_CAPABILITIES_PHOTO  = 1 << 2,
	REMOTE_DISPLAY_DEVICE_CAPABILITIES_SCREEN = 1 << 3
} RemoteDisplayDeviceCapabilities;

typedef enum {
	REMOTE_DISPLAY_DEVICE_STATE_STOPPED,
	REMOTE_DISPLAY_DEVICE_STATE_LOADING,
	REMOTE_DISPLAY_DEVICE_STATE_PLAYING,
	REMOTE_DISPLAY_DEVICE_STATE_PAUSED
} RemoteDisplayDeviceState;

char *remote_display_device_to_string (RemoteDisplayDevice *device);
const char *remote_display_device_get_name (RemoteDisplayDevice *device);
RemoteDisplayDeviceCapabilities remote_display_device_get_capabilities (RemoteDisplayDevice *device);
void remote_display_device_set_password (RemoteDisplayDevice *device, const char *password);
gboolean remote_display_device_get_password_protected (RemoteDisplayDevice *device);
void remote_display_device_open_and_play (RemoteDisplayDevice *device,
					  const char          *uri,
					  guint64              position_ms);
void remote_display_device_play (RemoteDisplayDevice *device);
void remote_display_device_pause (RemoteDisplayDevice *device);
void remote_display_device_stop (RemoteDisplayDevice *device);
void remote_display_device_seek (RemoteDisplayDevice *device,
				 gdouble              position_ms);

G_END_DECLS

#endif /* __REMOTE_DISPLAY_DEVICE_H__ */
