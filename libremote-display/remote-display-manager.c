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
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-glib/glib-malloc.h>
#include <avahi-glib/glib-watch.h>
#include <avahi-common/simple-watch.h>

#include <libremote-display/remote-display-manager.h>
#include <libremote-display/remote-display-device.h>
#include <libremote-display/remote-display-device-private.h>
#include <libremote-display/remote-display-device-airplay.h>

struct _RemoteDisplayManagerPrivate {
	GHashTable *known_devices; /* key = name, value = RemoteDisplayDevice */

	/* AIRPLAY support */

	/* Avahi <-> GLib adaptors */
	AvahiGLibPoll *poll;
	/* Avahi client */
	AvahiClient *client;
	/* Service browser */
	AvahiServiceBrowser *browser;
	/* Pending resolvers */
	GHashTable *resolvers;

	/* DLNA support */
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), REMOTE_DISPLAY_TYPE_MANAGER, RemoteDisplayManagerPrivate))

G_DEFINE_TYPE (RemoteDisplayManager, remote_display_manager, G_TYPE_OBJECT);

enum {
	DEVICE_APPEARED,
	DEVICE_DISAPPEARED,
	NUM_SIGS
};

static guint signals[NUM_SIGS] = {0,};

static void
on_resolve_callback (AvahiServiceResolver *r,
		     AvahiIfIndex interface, AvahiProtocol protocol,
		     AvahiResolverEvent event,
		     const char *name, const char *type,
		     const char *domain, const char *host_name,
		     const AvahiAddress *address,
		     uint16_t port,
		     AvahiStringList *txt,
		     AvahiLookupResultFlags flags,
		     void *userdata)
{
	RemoteDisplayManager *self = REMOTE_DISPLAY_MANAGER (userdata);
	RemoteDisplayManagerPrivate *priv = self->priv;

	switch (event) {
	case AVAHI_RESOLVER_FOUND: {
			RemoteDisplayDevice *device;

			device = remote_display_device_airplay_new (name, txt, host_name, port);
			if (device) {
				g_hash_table_insert (priv->known_devices, g_strdup (name), device);
				g_signal_emit (self, signals[DEVICE_APPEARED], 0, device);
			}
			g_hash_table_remove (priv->resolvers, r);
		}
		break;
	default:
		break;
	}
}

static void
on_browse_callback (AvahiServiceBrowser *b,
		    AvahiIfIndex interface, AvahiProtocol protocol,
		    AvahiBrowserEvent event,
		    const char *name,
		    const char *type,
		    const char *domain,
		    AvahiLookupResultFlags flags,
		    void *userdata)
{
	RemoteDisplayManager *self = REMOTE_DISPLAY_MANAGER (userdata);
	RemoteDisplayManagerPrivate *priv = self->priv;

	switch (event) {
	case AVAHI_BROWSER_NEW: {
			AvahiServiceResolver *resolver;

			resolver = avahi_service_resolver_new (priv->client,
							       interface, protocol, name, type, domain,
							       AVAHI_PROTO_UNSPEC, 0, on_resolve_callback, self);
			/* TODO: error handling */

			g_hash_table_insert (priv->resolvers, g_strdup (name), resolver);
		}
		break;
	case AVAHI_BROWSER_REMOVE: {
			RemoteDisplayDevice *device;

			device = g_hash_table_lookup (priv->known_devices, name);
			if (device) {
				g_signal_emit (self, signals[DEVICE_DISAPPEARED], 0, device);
				g_hash_table_remove (priv->known_devices, name);
			}
		}
		break;
	default:
		/* Nothing */
		;
	}
}

static void
on_client_state_changed (AvahiClient *client, AvahiClientState state, void *user_data)
{
	RemoteDisplayManager *self = REMOTE_DISPLAY_MANAGER (user_data);
	RemoteDisplayManagerPrivate *priv = self->priv;

	switch (state) {
	case AVAHI_CLIENT_S_RUNNING:
		priv->browser = avahi_service_browser_new (client,
							   AVAHI_IF_UNSPEC,
							   AVAHI_PROTO_UNSPEC,
							   "_airplay._tcp",
							   NULL, 0,
							   on_browse_callback, self);
		break;
	case AVAHI_CLIENT_S_REGISTERING:
	case AVAHI_CLIENT_CONNECTING:
		/* Silently do nothing */
		break;
	case AVAHI_CLIENT_S_COLLISION:
	case AVAHI_CLIENT_FAILURE:
	default:
		g_warning ("Cannot connect to Avahi: state %d", state);
		break;
	}
}

static void
remote_display_manager_finalize (GObject *object)
{
	RemoteDisplayManagerPrivate *priv = REMOTE_DISPLAY_MANAGER(object)->priv;

	g_clear_pointer (&priv->resolvers, g_hash_table_destroy);
	g_clear_pointer (&priv->browser, avahi_service_browser_free);
	g_clear_pointer (&priv->client, avahi_client_free);
	g_clear_pointer (&priv->poll, avahi_glib_poll_free);
}

static void
remote_display_manager_class_init (RemoteDisplayManagerClass *klass)
{
	GObjectClass *o_class = (GObjectClass *)klass;

	g_type_class_add_private (klass, sizeof (RemoteDisplayManagerPrivate));

	o_class->finalize = remote_display_manager_finalize;

	signals[DEVICE_APPEARED] = g_signal_new ("device-appeared",
						 REMOTE_DISPLAY_TYPE_MANAGER,
						 G_SIGNAL_RUN_FIRST,
						 0, NULL, NULL,
						 g_cclosure_marshal_generic,
						 G_TYPE_NONE,
						 1, G_TYPE_OBJECT);

	signals[DEVICE_DISAPPEARED] = g_signal_new ("device-disappeared",
						    REMOTE_DISPLAY_TYPE_MANAGER,
						    G_SIGNAL_RUN_FIRST,
						    0, NULL, NULL,
						    g_cclosure_marshal_generic,
						    G_TYPE_NONE,
						    1, G_TYPE_OBJECT);
}

static void
remote_display_manager_init (RemoteDisplayManager *self)
{
	RemoteDisplayManagerPrivate *priv;
	int error;

	priv = self->priv = GET_PRIVATE (self);

	priv->known_devices = g_hash_table_new_full (g_str_hash, g_str_equal,
						     g_free, g_object_unref);

	/* AirPlay */
	priv->resolvers = g_hash_table_new_full (g_str_hash, g_str_equal,
						 g_free, (GDestroyNotify) avahi_service_resolver_free);

	priv->poll = avahi_glib_poll_new (NULL, G_PRIORITY_DEFAULT);
	priv->client = avahi_client_new (avahi_glib_poll_get (priv->poll),
					 AVAHI_CLIENT_NO_FAIL,
					 on_client_state_changed,
					 self,
					 &error);
}

RemoteDisplayManager *
remote_display_manager_new (void)
{
	return g_object_new (REMOTE_DISPLAY_TYPE_MANAGER, NULL);
}
