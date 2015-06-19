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

#ifndef __REMOTE_DISPLAY_DEVICE_PRIVATE_H__
#define __REMOTE_DISPLAY_DEVICE_PRIVATE_H__

#include <glib-object.h>
#include <libremote-display/remote-display-device.h>
#include <avahi-common/strlst.h>

G_BEGIN_DECLS

//#define REMOTE_DISPLAY_TYPE_DEVICE_DLNA remote_display_device_dlna_get_type ()
//G_DECLARE_FINAL_TYPE (RemoteDisplayDeviceDlna, remote_display_device_dlna, REMOTE_DISPLAY, DEVICE_DLNA, RemoteDisplayDevice)

void remote_display_device_set_name (RemoteDisplayDevice *device,
				     const char          *name);
void remote_display_device_set_id (RemoteDisplayDevice *device,
				   const char          *id);
void remote_display_device_set_icon (RemoteDisplayDevice *device,
				     GIcon               *icon);
void remote_display_device_set_password_protected (RemoteDisplayDevice *device,
						   gboolean             password_protected);
void remote_display_device_set_capabilities (RemoteDisplayDevice             *device,
					     RemoteDisplayDeviceCapabilities  caps);

//RemoteDisplayDevice *remote_display_device_dlna_new    (const char      *name);

G_END_DECLS

#endif /* __REMOTE_DISPLAY_DEVICE_PRIVATE_H__ */
