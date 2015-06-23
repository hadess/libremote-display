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
#include <arpa/inet.h>
#include <net/if.h>

#define _GNU_SOURCE     /* To get defns of NI_MAXSERV and NI_MAXHOST */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>

#include <gio/gio.h>
#include <libsoup/soup.h>
#include <plist/plist.h>

#include <avahi-glib/glib-malloc.h>
#include <avahi-common/strlst.h>

#include <libremote-display/remote-display-device.h>
#include <libremote-display/remote-display-device-private.h>
#include <libremote-display/remote-display-private.h>
#include <libremote-display/remote-display-device-airplay.h>

struct _RemoteDisplayDeviceAirplay {
	GObject parent_instance;

	GCancellable *cancellable;

	char *hostname;
	guint port;
	guint features;
	char *password;

	gboolean connected;
	char *session_id;
	SoupServer *server;
	SoupSession *session;
	GQueue *actions;
};

/* Note, those names match AirPlay commands, not
 * the public API */
typedef enum {
	REMOTE_DISPLAY_DEVICE_ACTION_PLAY,
	REMOTE_DISPLAY_DEVICE_ACTION_SCRUB,
	REMOTE_DISPLAY_DEVICE_ACTION_RATE,
	REMOTE_DISPLAY_DEVICE_ACTION_STOP
} RemoteDisplayDeviceActionType;

typedef struct {
	RemoteDisplayDeviceActionType type;
	char *uri;
	gfloat value;
} RemoteDisplayDeviceAirplayAction;

G_DEFINE_TYPE (RemoteDisplayDeviceAirplay, remote_display_device_airplay, REMOTE_DISPLAY_TYPE_DEVICE);

static void remote_display_airplay_clear_session (RemoteDisplayDeviceAirplay *device);

static void
action_free (RemoteDisplayDeviceAirplayAction *action)
{
	g_free (action->uri);
	g_free (action);
}

static void
remote_display_device_airplay_finalize (GObject *object)
{
	RemoteDisplayDeviceAirplay *device = REMOTE_DISPLAY_DEVICE_AIRPLAY (object);

	if (device->cancellable) {
		g_cancellable_cancel (device->cancellable);
		g_object_unref (device->cancellable);
	}
	if (device->actions)
		g_queue_free_full (device->actions, (GDestroyNotify) action_free);

	g_free (device->hostname);
	g_free (device->password);
	remote_display_airplay_clear_session (device);

	G_OBJECT_CLASS (remote_display_device_airplay_parent_class)->finalize (object);
}

static void
remote_display_device_airplay_class_init (RemoteDisplayDeviceAirplayClass *klass)
{
	GObjectClass *o_class = (GObjectClass *)klass;

	o_class->finalize = remote_display_device_airplay_finalize;
}

static void
remote_display_device_airplay_init (RemoteDisplayDeviceAirplay *device)
{
	device->cancellable = g_cancellable_new ();
	device->actions = g_queue_new ();
}

static void
remote_display_airplay_clear_session (RemoteDisplayDeviceAirplay *device)
{
	g_clear_pointer (&device->session_id, g_free);
	g_clear_object (&device->server);
	g_clear_object (&device->session);
}

static SoupMessage *
remote_display_airplay_create_message (RemoteDisplayDeviceAirplay *device,
				       const char          *method,
				       const char          *path)
{
	SoupMessage *msg;
	char *uri;
	GTimeVal date;
	char *date_str;

	g_return_val_if_fail (*path == '/', NULL);

	uri = g_strdup_printf ("http://%s:%d%s", device->hostname, device->port, path);
	msg = soup_message_new (method, uri);
	g_free (uri);
	soup_message_headers_append (msg->request_headers, "X-Apple-Session-ID", device->session_id);
	g_get_current_time (&date);
	date_str = g_time_val_to_iso8601 (&date);
	soup_message_headers_append (msg->request_headers, "X-Transmit-Date", date_str);
	g_free (date_str);

	return msg;
}

static void
server_cb (SoupServer *server,
	   SoupMessage *msg,
	   const char *path,
	   GHashTable *query,
	   SoupClientContext *client,
	   gpointer user_data)
{
	RemoteDisplayDeviceAirplay *device = user_data;
	SoupMessageBody *body;
	RemoteDisplayDeviceState state;
	plist_t plist, p_state;
	char *str;

	g_object_get (G_OBJECT (msg), SOUP_MESSAGE_REQUEST_BODY, &body, NULL);
	plist = NULL;
	plist_from_xml (body->data, body->length, &plist);
	if (!plist) {
		plist = NULL;
		plist_from_bin (body->data, body->length, &plist);
		if (!plist) {
			g_warning ("Failed to parse event from the server");
			soup_message_set_status (msg, SOUP_STATUS_MALFORMED);
			return;
		}
	}
	soup_message_body_free (body);

	p_state = plist_dict_get_item (plist, "state");
	if (!p_state) {
		g_warning ("Failed to get 'state' from plist");
		soup_message_set_status (msg, SOUP_STATUS_MALFORMED);
		return;
	}
	plist_get_string_val (p_state, &str);

	if (g_strcmp0 (str, "loading") == 0)
		state = REMOTE_DISPLAY_DEVICE_STATE_LOADING;
	else if (g_strcmp0 (str, "playing") == 0)
		state = REMOTE_DISPLAY_DEVICE_STATE_PLAYING;
	else if (g_strcmp0 (str, "paused") == 0)
		state = REMOTE_DISPLAY_DEVICE_STATE_PAUSED;
	else if (g_strcmp0 (str, "stopped") == 0)
		state = REMOTE_DISPLAY_DEVICE_STATE_STOPPED;
	else {
		g_warning ("Unhandled state '%s' in plist", str);
		free (str);
		soup_message_set_status (msg, SOUP_STATUS_MALFORMED);
		return;
	}

	free (str);
	plist_free (plist);

	g_signal_emit_by_name (G_OBJECT (device), "state-changed", state);
	soup_message_set_status (msg, 200);
}

static void pop_action_queue (RemoteDisplayDeviceAirplay *device);

static void
action_cb (SoupSession *session,
	   SoupMessage *msg,
	   gpointer user_data)
{
	RemoteDisplayDeviceAirplay *device = user_data;
	guint status;

	g_object_get (G_OBJECT (msg), SOUP_MESSAGE_STATUS_CODE, &status, NULL);
	if (status != 200) {
		g_warning ("Call failed: %d", status);
		return;
	}

	pop_action_queue (device);
}

static void
pop_action_queue (RemoteDisplayDeviceAirplay *device)
{
	RemoteDisplayDeviceAirplayAction *action;
	SoupMessage *msg;

	action = g_queue_pop_head (device->actions);
	if (!action)
		return;

	if (action->type == REMOTE_DISPLAY_DEVICE_ACTION_PLAY) {
		char *params;
		msg = remote_display_airplay_create_message (device, "POST", "/play");
		params = g_strdup_printf ("Content-Location: %s\nStart-Position: %lf\n", action->uri, action->value);
		soup_message_set_request (msg, "text/parameters", SOUP_MEMORY_TAKE, params, strlen(params));
	} else if (action->type == REMOTE_DISPLAY_DEVICE_ACTION_SCRUB) {
		char *path;
		path = g_strdup_printf ("/scrub?position=%lf", action->value);
		msg = remote_display_airplay_create_message (device, "POST", path);
		g_free (path);
	} else if (action->type == REMOTE_DISPLAY_DEVICE_ACTION_RATE) {
		char *path;
		path = g_strdup_printf ("/rate?value=%lf", action->value);
		msg = remote_display_airplay_create_message (device, "POST", path);
		g_free (path);
	} else if (action->type == REMOTE_DISPLAY_DEVICE_ACTION_STOP) {
		msg = remote_display_airplay_create_message (device, "POST", "/stop");
	} else {
		g_assert_not_reached ();
	}

	soup_session_queue_message (device->session, msg, action_cb, device);
}

static void
revhttp_cb (GObject *object,
	    GAsyncResult *result,
	    gpointer user_data)
{
	RemoteDisplayDeviceAirplay *device = user_data;
	SoupSession *session = SOUP_SESSION (object);
	GError *error = NULL;
	SoupServer *server;

	server = soup_session_reverse_http_connect_finish (session, result, &error);
	if (server == NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			g_error_free (error);
			return;
		}
		g_warning ("Reverse HTTP failed: %s", error->message);
		g_error_free (error);
		remote_display_airplay_clear_session (device);
		return;
	}
	g_debug ("Connected AirPlay reverse HTTP");
	device->server = server;
	soup_server_add_handler (device->server, NULL, server_cb, device, NULL);

	device->session = soup_session_new_with_options (SOUP_SESSION_MAX_CONNS_PER_HOST, 1,
							 SOUP_SESSION_MAX_CONNS, 1,
							 NULL);

	pop_action_queue (device);
	//FIXME user-agent
	//g_object_set (G_OBJECT (session), SOUP_SESSION_USER_AGENT, "Quicktime/7.2.0", NULL);
}

void
remote_display_device_airplay_set_password (RemoteDisplayDeviceAirplay *device,
					    const char          *password)
{
	g_return_if_fail (REMOTE_DISPLAY_IS_DEVICE_AIRPLAY (device));

	g_clear_pointer (&device->password, g_free);
	device->password = g_strdup (password);
}

static GSocketAddress *
avahi_address_to_gsocket_address (const AvahiAddress *address,
				  AvahiIfIndex        interface)
{
	/* Note that this zeroes out the port, as we don't know which
	 * port the requests are going to be coming from */
	switch (address->proto) {
	case AVAHI_PROTO_INET: {
		struct sockaddr_in sockaddr4;
		sockaddr4.sin_family = AF_INET;
		sockaddr4.sin_port = htons (0);
		/* ->address is already in network byte order */
		sockaddr4.sin_addr.s_addr = address->data.ipv4.address;
		return g_socket_address_new_from_native (&sockaddr4, sizeof(struct sockaddr_in));
		}
		break;
	case AVAHI_PROTO_INET6: {
		struct sockaddr_in6 sockaddr6;
		sockaddr6.sin6_family = AF_INET6;
		sockaddr6.sin6_port = htons (0);
		memcpy (sockaddr6.sin6_addr.s6_addr, address->data.ipv6.address, 16);
		sockaddr6.sin6_flowinfo = 0;
		sockaddr6.sin6_scope_id = interface;
		return g_socket_address_new_from_native (&sockaddr6, sizeof(struct sockaddr_in6));
		}
		break;
	default:
		g_assert_not_reached ();
	}
}

static GInetAddress *
avahi_address_to_address (const AvahiAddress *address,
			  AvahiIfIndex        interface)
{
	GSocketAddress *sock_addr;
	GInetAddress *addr;

	sock_addr = avahi_address_to_gsocket_address (address, interface);
	if (!sock_addr)
		return NULL;

	addr = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (sock_addr));
	g_object_ref (addr);
	g_object_unref (sock_addr);

	return addr;
}

static int
avahi_protocol_to_family (AvahiProtocol protocol)
{
	switch (protocol) {
	case AVAHI_PROTO_INET:
		return AF_INET;
		break;
	case AVAHI_PROTO_INET6:
		return AF_INET6;
		break;
	default:
		g_assert_not_reached ();
	}
}

static GInetAddress *
get_local_address (AvahiIfIndex  interface,
		   AvahiProtocol protocol)
{
	GInetAddress *address = NULL;
	struct ifaddrs *ifaddr, *ifa;
	int n;
	char ifname[IF_NAMESIZE];

	if (!if_indextoname (interface, (char *) &ifname)) {
		g_warning ("if_indextoname failed for %d", interface);
		return NULL;
	}

	if (getifaddrs (&ifaddr) == -1) {
		g_warning ("getifaddrs failed");
		return NULL;
	}

	for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
		char host[NI_MAXHOST];
		int family, s;

		if (ifa->ifa_addr == NULL)
			continue;
		if (g_strcmp0 (ifa->ifa_name, ifname) != 0)
			continue;
		family = ifa->ifa_addr->sa_family;
		if (family != avahi_protocol_to_family (protocol)) {
			g_message ("%d != %d", ifa->ifa_addr->sa_family, protocol);
			continue;
		}

		s = getnameinfo (ifa->ifa_addr,
				 (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
				 host, NI_MAXHOST,
				 NULL, 0, NI_NUMERICHOST);
		if (s != 0) {
			g_warning ("Failed to get address from interface details");
			continue;
		}

		address = g_inet_address_new_from_string (host);
		break;
	}

	freeifaddrs (ifaddr);

	return address;
}

RemoteDisplayDevice *
remote_display_device_airplay_new (AvahiIfIndex        interface,
				   AvahiProtocol       protocol,
				   const char         *name,
				   AvahiStringList    *txt,
				   const char         *host_name,
				   const AvahiAddress *address,
				   guint16             port)
{
	RemoteDisplayDeviceAirplay *device;
	AvahiStringList *l;
	guint features = 0x0;
	char *device_id = NULL;
	gboolean password_protected = FALSE;
	RemoteDisplayDeviceCapabilities caps;
	GInetAddress *remote_address, *local_address;

	remote_address = avahi_address_to_address (address, interface);
	if (!remote_address) {
		g_warning ("Couldn't get remote address");
		return NULL;
	}
	local_address = get_local_address (interface, protocol);
	if (!local_address) {
		g_clear_object (&remote_address);
		g_warning ("Couldn't get local address");
		return NULL;
	}

	/* Collect the features */
	for (l = txt; l != NULL; l = avahi_string_list_get_next(l)) {
		char *key, *value;

		avahi_string_list_get_pair (l, &key, &value, NULL);
		if (!key || !value) {
			g_clear_pointer (&key, avahi_free);
			g_clear_pointer (&value, avahi_free);
			continue;
		}

		if (g_strcmp0 (key, "features") == 0)
			features = strtol (value, NULL, 16);
		else if (g_strcmp0 (key, "deviceid") == 0)
			device_id = g_strdup (value);
		else if (g_strcmp0 (key, "pw") == 0)
			password_protected = (*value == '1');

		avahi_free (key);
		avahi_free (value);
	}

	if (!device_id || !features) {
		g_debug ("Device '%s' is missing metadata, not adding", name);
		return NULL;
	}

	caps = REMOTE_DISPLAY_DEVICE_CAPABILITIES_NONE;
	if (features & AIRPLAY_VIDEO_SUPPORT)
		caps |= REMOTE_DISPLAY_DEVICE_CAPABILITIES_VIDEO;
	if (features & AIRPLAY_PHOTO_SUPPORT)
		caps |= REMOTE_DISPLAY_DEVICE_CAPABILITIES_PHOTO;
	if (features & AIRPLAY_VIDEO_SCREEN_SUPPORT)
		caps |= REMOTE_DISPLAY_DEVICE_CAPABILITIES_SCREEN;

	device = g_object_new (REMOTE_DISPLAY_TYPE_DEVICE_AIRPLAY, NULL);
	remote_display_device_set_name (REMOTE_DISPLAY_DEVICE (device), name);
	remote_display_device_set_id (REMOTE_DISPLAY_DEVICE (device), device_id);
	g_free (device_id);
	//FIXME remote_display_device_set_icon
	remote_display_device_set_password_protected (REMOTE_DISPLAY_DEVICE (device), password_protected);
	remote_display_device_set_capabilities (REMOTE_DISPLAY_DEVICE (device), caps);

	device->hostname = g_strdup (host_name);
	device->port = port;
	device->features = features;
	g_clear_object (&remote_address);
	g_clear_object (&local_address);

	return REMOTE_DISPLAY_DEVICE (device);
}

void
remote_display_device_airplay_open_and_play (RemoteDisplayDeviceAirplay *device,
					     const char          *uri,
					     gdouble              orig_position)
{
	RemoteDisplayDeviceCapabilities caps;
	RemoteDisplayDeviceAirplayAction *action;

	g_return_if_fail (REMOTE_DISPLAY_IS_DEVICE_AIRPLAY (device));

	g_object_get (G_OBJECT (device), "capabilities", &caps, NULL);
	g_return_if_fail (caps & REMOTE_DISPLAY_DEVICE_CAPABILITIES_VIDEO);

	action = g_new0 (RemoteDisplayDeviceAirplayAction, 1);
	action->type = REMOTE_DISPLAY_DEVICE_ACTION_PLAY;
	action->uri = g_strdup (uri);
	action->value = orig_position;

	g_queue_push_tail (device->actions, action);

	if (!device->session_id) {
		SoupSession *session;
		SoupMessage *msg;

		device->session_id = g_uuid_random ();

		session = soup_session_new ();
		//FIXME set user-agent
		msg = remote_display_airplay_create_message (device, "POST", "/reverse");
		soup_message_headers_append (msg->request_headers, "X-Apple-Purpose", "event");

		soup_session_reverse_http_connect_async (session, msg, device->cancellable, revhttp_cb, device);
		//g_object_unref (session);
		//g_object_unref (msg);
	} else {
		pop_action_queue (device);
	}
}

static void
remote_display_device_airplay_rate (RemoteDisplayDeviceAirplay *device,
				    gfloat                      rate)
{
	RemoteDisplayDeviceCapabilities caps;
	RemoteDisplayDeviceAirplayAction *action;

	g_return_if_fail (REMOTE_DISPLAY_IS_DEVICE_AIRPLAY (device));

	g_object_get (G_OBJECT (device), "capabilities", &caps, NULL);
	g_return_if_fail (caps & REMOTE_DISPLAY_DEVICE_CAPABILITIES_VIDEO);

	action = g_new0 (RemoteDisplayDeviceAirplayAction, 1);
	action->type = REMOTE_DISPLAY_DEVICE_ACTION_RATE;
	action->value = rate;

	g_queue_push_tail (device->actions, action);
	pop_action_queue (device);
}

void
remote_display_device_airplay_play (RemoteDisplayDeviceAirplay *device)
{
	remote_display_device_airplay_rate (device, 1.0);
}

void
remote_display_device_airplay_pause (RemoteDisplayDeviceAirplay *device)
{
	remote_display_device_airplay_rate (device, 0.0);
}

void
remote_display_device_airplay_stop (RemoteDisplayDeviceAirplay *device)
{
	RemoteDisplayDeviceCapabilities caps;
	RemoteDisplayDeviceAirplayAction *action;

	g_return_if_fail (REMOTE_DISPLAY_IS_DEVICE_AIRPLAY (device));

	g_object_get (G_OBJECT (device), "capabilities", &caps, NULL);
	g_return_if_fail (caps & REMOTE_DISPLAY_DEVICE_CAPABILITIES_VIDEO);

	action = g_new0 (RemoteDisplayDeviceAirplayAction, 1);
	action->type = REMOTE_DISPLAY_DEVICE_ACTION_STOP;

	g_queue_push_tail (device->actions, action);
	pop_action_queue (device);
}

void
remote_display_device_airplay_seek (RemoteDisplayDeviceAirplay *device,
				    gdouble                     position_ms)
{
	RemoteDisplayDeviceCapabilities caps;
	RemoteDisplayDeviceAirplayAction *action;

	g_return_if_fail (REMOTE_DISPLAY_IS_DEVICE_AIRPLAY (device));

	g_object_get (G_OBJECT (device), "capabilities", &caps, NULL);
	g_return_if_fail (caps & REMOTE_DISPLAY_DEVICE_CAPABILITIES_VIDEO);

	action = g_new0 (RemoteDisplayDeviceAirplayAction, 1);
	action->type = REMOTE_DISPLAY_DEVICE_ACTION_SCRUB;
	action->value = position_ms;

	g_queue_push_tail (device->actions, action);
	pop_action_queue (device);
}

char *
remote_display_device_airplay_add_to_string (RemoteDisplayDeviceAirplay *device,
					     GString                    *s)
{
	g_return_val_if_fail (REMOTE_DISPLAY_IS_DEVICE_AIRPLAY (device), NULL);

	g_string_append_printf (s, "\tHostname: %s\n", device->hostname);
	g_string_append_printf (s, "\tPort: %d\n", device->port);
	g_string_append_printf (s, "\tFeatures: 0x%x\n", device->features);

	return g_string_free (s, FALSE);
}
