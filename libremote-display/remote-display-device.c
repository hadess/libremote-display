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

#include <string.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <libsoup/soup.h>

#include <avahi-glib/glib-malloc.h>
#include <avahi-common/strlst.h>

#include <libremote-display/remote-display.h>
#include <libremote-display/remote-display-device-private.h>
#include <libremote-display/remote-display-device-airplay.h>
#include <libremote-display/remote-display-private.h>

struct _RemoteDisplayDevicePrivate {
	char *name;                            /* User-visible name */
	char *id;                              /* UUID for DLNA, device ID for AirPlay */
	GIcon *icon;                           /* URL or bytes data for DLNA, named icon for AirPlay */
	gboolean password_protected;           /* Always FALSE for DLNA */
	RemoteDisplayDeviceCapabilities caps;
	RemoteDisplayDeviceState last_state;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), REMOTE_DISPLAY_TYPE_DEVICE, RemoteDisplayDevicePrivate))

G_DEFINE_TYPE_WITH_PRIVATE (RemoteDisplayDevice, remote_display_device, G_TYPE_OBJECT);

enum {
	STATE_CHANGED,
	NUM_SIGS
};

enum {
	PROP_0 = 0,
	PROP_NAME,
	PROP_ID,
	PROP_ICON,
	PROP_PASSWORD_PROTECTED,
	PROP_CAPS
};

static guint signals[NUM_SIGS] = {0,};

static void
remote_display_device_finalize (GObject *object)
{
	RemoteDisplayDevicePrivate *priv = GET_PRIVATE (object);

	g_free (priv->name);
	g_free (priv->id);
	g_clear_object (&priv->icon);

	G_OBJECT_CLASS (remote_display_device_parent_class)->finalize (object);
}

static void
remote_display_device_get_property (GObject    *object,
				    guint       prop_id,
				    GValue     *value,
				    GParamSpec *pspec)
{
	RemoteDisplayDevicePrivate *priv;

	priv = GET_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_NAME:
		g_value_set_string (value, priv->name);
		break;
	case PROP_ID:
		g_value_set_string (value, priv->id);
		break;
	case PROP_ICON:
		g_value_set_object (value, priv->icon);
		break;
	case PROP_PASSWORD_PROTECTED:
		g_value_set_boolean (value, priv->password_protected);
		break;
	case PROP_CAPS:
		g_value_set_uint (value, priv->caps);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
remote_display_device_class_init (RemoteDisplayDeviceClass *klass)
{
	GObjectClass *o_class = (GObjectClass *)klass;

	o_class->get_property = remote_display_device_get_property;
	o_class->finalize = remote_display_device_finalize;

	g_object_class_install_property (o_class,
					 PROP_NAME,
					 g_param_spec_string ("name",
							      "Name",
							      "The user-visible name for the device",
							      NULL,
							      G_PARAM_READABLE));
	g_object_class_install_property (o_class,
					 PROP_ID,
					 g_param_spec_string ("id",
							      "ID",
							      "The unique ID for the device",
							      NULL,
							      G_PARAM_READABLE));
	g_object_class_install_property (o_class,
					 PROP_ICON,
					 g_param_spec_object ("icon",
							      "Icon",
							      "The icon representing the device",
							      G_TYPE_ICON,
							      G_PARAM_READABLE));
	g_object_class_install_property (o_class,
					 PROP_PASSWORD_PROTECTED,
					 g_param_spec_boolean ("password-protected",
							      "Password protected",
							      "Whether the device is password protected",
							      FALSE,
							      G_PARAM_READABLE));
	g_object_class_install_property (o_class,
					 PROP_CAPS,
					 g_param_spec_uint ("capabilities",
							    "Capabilities",
							    "The capabilities of the device",
							    REMOTE_DISPLAY_DEVICE_CAPABILITIES_NONE, G_MAXUINT,
							    REMOTE_DISPLAY_DEVICE_CAPABILITIES_NONE,
							    G_PARAM_READABLE));

	signals[STATE_CHANGED] = g_signal_new ("state-changed",
					       REMOTE_DISPLAY_TYPE_DEVICE,
					       G_SIGNAL_RUN_FIRST,
					       0, NULL, NULL,
					       g_cclosure_marshal_generic,
					       G_TYPE_NONE,
					       1, REMOTE_DISPLAY_TYPE_DISPLAY_DEVICE_STATE);
}

static void
remote_display_device_init (RemoteDisplayDevice *device)
{
}

void
remote_display_device_open_and_play (RemoteDisplayDevice *device,
				     const char          *uri,
				     guint64              position_ms)
{
	g_return_if_fail (REMOTE_DISPLAY_IS_DEVICE (device));
	g_return_if_fail (remote_display_device_get_capabilities (device) & REMOTE_DISPLAY_DEVICE_CAPABILITIES_VIDEO);

	if (REMOTE_DISPLAY_IS_DEVICE_AIRPLAY (device)) {
		remote_display_device_airplay_open_and_play (REMOTE_DISPLAY_DEVICE_AIRPLAY (device), uri, position_ms);
	} else {
		g_assert_not_reached ();
	}
}

void
remote_display_device_play (RemoteDisplayDevice *device)
{
	g_return_if_fail (REMOTE_DISPLAY_IS_DEVICE (device));
	g_return_if_fail (remote_display_device_get_capabilities (device) & REMOTE_DISPLAY_DEVICE_CAPABILITIES_VIDEO);

	if (REMOTE_DISPLAY_IS_DEVICE_AIRPLAY (device)) {
		remote_display_device_airplay_play (REMOTE_DISPLAY_DEVICE_AIRPLAY (device));
	} else {
		g_assert_not_reached ();
	}
}

void
remote_display_device_pause (RemoteDisplayDevice *device)
{
	g_return_if_fail (REMOTE_DISPLAY_IS_DEVICE (device));
	g_return_if_fail (remote_display_device_get_capabilities (device) & REMOTE_DISPLAY_DEVICE_CAPABILITIES_VIDEO);

	if (REMOTE_DISPLAY_IS_DEVICE_AIRPLAY (device)) {
		remote_display_device_airplay_pause (REMOTE_DISPLAY_DEVICE_AIRPLAY (device));
	} else {
		g_assert_not_reached ();
	}
}

void
remote_display_device_stop (RemoteDisplayDevice *device)
{
	g_return_if_fail (REMOTE_DISPLAY_IS_DEVICE (device));
	g_return_if_fail (remote_display_device_get_capabilities (device) & REMOTE_DISPLAY_DEVICE_CAPABILITIES_VIDEO);

	if (REMOTE_DISPLAY_IS_DEVICE_AIRPLAY (device)) {
		remote_display_device_airplay_stop (REMOTE_DISPLAY_DEVICE_AIRPLAY (device));
	} else {
		g_assert_not_reached ();
	}
}

void
remote_display_device_seek (RemoteDisplayDevice *device,
			    gdouble              position_ms)
{
	g_return_if_fail (REMOTE_DISPLAY_IS_DEVICE (device));
	g_return_if_fail (remote_display_device_get_capabilities (device) & REMOTE_DISPLAY_DEVICE_CAPABILITIES_VIDEO);

	if (REMOTE_DISPLAY_IS_DEVICE_AIRPLAY (device)) {
		remote_display_device_airplay_seek (REMOTE_DISPLAY_DEVICE_AIRPLAY (device), position_ms);
	} else {
		g_assert_not_reached ();
	}
}

RemoteDisplayDeviceCapabilities
remote_display_device_get_capabilities (RemoteDisplayDevice *device)
{
	RemoteDisplayDevicePrivate *priv;

	g_return_val_if_fail (REMOTE_DISPLAY_IS_DEVICE (device), REMOTE_DISPLAY_DEVICE_CAPABILITIES_NONE);

	priv = GET_PRIVATE (device);
	return priv->caps;
}

void
remote_display_device_set_password (RemoteDisplayDevice *device,
				    const char          *password)
{
#if 0
	g_return_if_fail (REMOTE_DISPLAY_IS_DEVICE (device));
	g_return_if_fail (device->priv->password_protected);

	g_clear_pointer (&device->priv->password, g_free);
	device->priv->password = g_strdup (password);
#endif
}

char *
remote_display_device_to_string (RemoteDisplayDevice *device)
{
	GString *s;
	RemoteDisplayDevicePrivate *priv;

	g_return_val_if_fail (REMOTE_DISPLAY_IS_DEVICE (device), NULL);

	priv = GET_PRIVATE (device);

	s = g_string_new (NULL);
	g_string_append_printf (s, "\tName: %s\n", priv->name);
	g_string_append_printf (s, "\tID: %s\n", priv->id);
	if (priv->icon) {
		char *icon;
		icon = g_icon_to_string (priv->icon);
		g_string_append_printf (s, "\tIcon: %s\n", icon);
		g_free (icon);
	}
	g_string_append (s, "\tCapabilities: ");
	if (priv->caps == REMOTE_DISPLAY_DEVICE_CAPABILITIES_NONE) {
		g_string_append (s, "None\n");
	} else {
		if (priv->caps & REMOTE_DISPLAY_DEVICE_CAPABILITIES_VIDEO)
			g_string_append (s, "Video ");
		if (priv->caps & REMOTE_DISPLAY_DEVICE_CAPABILITIES_PHOTO)
			g_string_append (s, "Photo ");
		if (priv->caps & REMOTE_DISPLAY_DEVICE_CAPABILITIES_SCREEN)
			g_string_append (s, "Screen");
		g_string_append (s, "\n");
	}
	g_string_append_printf (s, "\tPassword protected: %s\n", priv->password_protected ? "true" : "false");

	if (REMOTE_DISPLAY_IS_DEVICE_AIRPLAY (device))
		return remote_display_device_airplay_add_to_string (REMOTE_DISPLAY_DEVICE_AIRPLAY (device), s);

	return g_string_free (s, FALSE);
}

void
remote_display_device_set_name (RemoteDisplayDevice *device,
				const char          *name)
{
	RemoteDisplayDevicePrivate *priv;

	g_return_val_if_fail (REMOTE_DISPLAY_IS_DEVICE (device), NULL);

	priv = GET_PRIVATE (device);
	g_clear_pointer (&priv->name, g_free);
	priv->name = g_strdup (name);
}

void
remote_display_device_set_id (RemoteDisplayDevice *device,
			      const char          *id)
{
	RemoteDisplayDevicePrivate *priv;

	g_return_val_if_fail (REMOTE_DISPLAY_IS_DEVICE (device), NULL);

	priv = GET_PRIVATE (device);
	g_clear_pointer (&priv->id, g_free);
	priv->id = g_strdup (id);
}

void
remote_display_device_set_icon (RemoteDisplayDevice *device,
				GIcon               *icon)
{
	RemoteDisplayDevicePrivate *priv;

	g_return_val_if_fail (REMOTE_DISPLAY_IS_DEVICE (device), NULL);

	priv = GET_PRIVATE (device);
	g_clear_object (&priv->icon);
	priv->icon = g_object_ref (icon);
}

void
remote_display_device_set_password_protected (RemoteDisplayDevice *device,
					      gboolean             password_protected)
{
	RemoteDisplayDevicePrivate *priv;

	g_return_val_if_fail (REMOTE_DISPLAY_IS_DEVICE (device), FALSE);

	priv = GET_PRIVATE (device);
	priv->password_protected = password_protected;
}

void
remote_display_device_set_capabilities (RemoteDisplayDevice             *device,
					RemoteDisplayDeviceCapabilities  caps)
{
	RemoteDisplayDevicePrivate *priv;

	g_return_val_if_fail (REMOTE_DISPLAY_IS_DEVICE (device), FALSE);

	priv = GET_PRIVATE (device);
	priv->caps = caps;
}
