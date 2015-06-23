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

#ifndef __REMOTE_DISPLAY_DEVICE_AIRPLAY_H__
#define __REMOTE_DISPLAY_DEVICE_AIRPLAY_H__

#include <glib-object.h>
#include <libremote-display/remote-display-device.h>
#include <avahi-common/strlst.h>
#include <avahi-common/address.h>

G_BEGIN_DECLS

#define REMOTE_DISPLAY_TYPE_DEVICE_AIRPLAY remote_display_device_airplay_get_type ()
G_DECLARE_FINAL_TYPE (RemoteDisplayDeviceAirplay, remote_display_device_airplay, REMOTE_DISPLAY, DEVICE_AIRPLAY, RemoteDisplayDevice)

RemoteDisplayDevice *remote_display_device_airplay_new           (AvahiIfIndex                interface,
								  AvahiProtocol               protocol,
								  const char                 *name,
							          AvahiStringList            *txt,
								  const char                 *host_name,
								  const AvahiAddress         *address,
								  guint16                     port);
char                *remote_display_device_airplay_add_to_string (RemoteDisplayDeviceAirplay *device,
								  GString                    *s);
void                 remote_display_device_airplay_open_and_play (RemoteDisplayDeviceAirplay *device,
								  const char                 *uri,
								  gdouble                     orig_position);
void                 remote_display_device_airplay_play          (RemoteDisplayDeviceAirplay *device);
void                 remote_display_device_airplay_pause         (RemoteDisplayDeviceAirplay *device);
void                 remote_display_device_airplay_stop          (RemoteDisplayDeviceAirplay *device);
void                 remote_display_device_airplay_seek          (RemoteDisplayDeviceAirplay *device,
								  gdouble                     position_ms);
void                 remote_display_device_airplay_set_password  (RemoteDisplayDeviceAirplay *device,
								  const char                 *password);

G_END_DECLS

#endif /* __REMOTE_DISPLAY_DEVICE_AIRPLAY_H__ */
