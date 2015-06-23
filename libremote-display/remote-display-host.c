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
#include <libremote-display/remote-display-host.h>

typedef struct {
	GMappedFile *mapped_file;
	char *uri;
	char *mime_type;
} RemoteDisplayHostFile;

struct _RemoteDisplayHostPrivate {
	GInetAddress *remote_address;
	GInetAddress *local_address;
	SoupServer *server;
	gboolean server_started;
	GHashTable *files;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), REMOTE_DISPLAY_TYPE_HOST, RemoteDisplayHostPrivate))

G_DEFINE_TYPE_WITH_PRIVATE (RemoteDisplayHost, remote_display_host, G_TYPE_OBJECT);

enum {
	PROP_0 = 0,
	PROP_REMOTE_ADDRESS,
	PROP_LOCAL_ADDRESS
};

static void
remote_display_host_finalize (GObject *object)
{
	RemoteDisplayHostPrivate *priv = GET_PRIVATE (object);

	g_clear_object (&priv->remote_address);
	g_clear_object (&priv->local_address);
	g_clear_pointer (&priv->files, g_hash_table_unref);
	g_clear_object (&priv->server);

	G_OBJECT_CLASS (remote_display_host_parent_class)->finalize (object);
}

static void
remote_display_host_get_property (GObject    *object,
				  guint       prop_id,
				  GValue     *value,
				  GParamSpec *pspec)
{
	RemoteDisplayHostPrivate *priv;

	priv = GET_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_REMOTE_ADDRESS:
		g_value_set_object (value, priv->remote_address);
		break;
	case PROP_LOCAL_ADDRESS:
		g_value_set_object (value, priv->local_address);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
remote_display_host_set_property (GObject      *object,
				  guint         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
	RemoteDisplayHostPrivate *priv;

	priv = GET_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_LOCAL_ADDRESS:
		g_clear_object (&priv->local_address);
		priv->local_address = g_value_dup_object (value);
		break;
	case PROP_REMOTE_ADDRESS:
		g_clear_object (&priv->remote_address);
		priv->remote_address = g_value_dup_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
remote_display_host_class_init (RemoteDisplayHostClass *klass)
{
	GObjectClass *o_class = (GObjectClass *)klass;

	o_class->set_property = remote_display_host_set_property;
	o_class->get_property = remote_display_host_get_property;
	o_class->finalize = remote_display_host_finalize;

	g_object_class_install_property (o_class,
					 PROP_REMOTE_ADDRESS,
					 g_param_spec_object ("remote-address",
							      "Remote address",
							      "The address of the client",
							      G_TYPE_INET_ADDRESS,
							      G_PARAM_READWRITE));
	g_object_class_install_property (o_class,
					 PROP_LOCAL_ADDRESS,
					 g_param_spec_object ("local-address",
							      "Local address",
							      "The address of the server",
							      G_TYPE_INET_ADDRESS,
							      G_PARAM_READWRITE));
}

static void
file_free (RemoteDisplayHostFile *file)
{
	g_free (file->uri);
	g_free (file->mime_type);
	g_mapped_file_unref (file->mapped_file);
	g_free (file);
}

static gboolean
client_allowed (RemoteDisplayHost *host,
		SoupClientContext *client)
{
	RemoteDisplayHostPrivate *priv = GET_PRIVATE (host);
	GSocketAddress *remote_sock;
	GInetAddress *remote_addr;

	remote_sock = soup_client_context_get_remote_address (client);
	remote_addr = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (remote_sock));

	return g_inet_address_equal (remote_addr, priv->remote_address);
}

static void
server_callback (SoupServer        *server,
		 SoupMessage       *msg,
		 const char        *path,
		 GHashTable        *query,
		 SoupClientContext *client,
		 gpointer           user_data)
{
	RemoteDisplayHost *host = user_data;
	RemoteDisplayHostPrivate *priv = GET_PRIVATE (host);
	RemoteDisplayHostFile *file;

	if (!client_allowed (host, client)) {
		g_debug ("Client %s not allowed", soup_client_context_get_host (client));
		soup_message_set_status (msg, SOUP_STATUS_FORBIDDEN);
		return;
	}

	if (msg->method != SOUP_METHOD_GET &&
	    msg->method != SOUP_METHOD_HEAD) {
		g_debug ("Method is not GET or HEAD");
		soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);
		return;
	}

	if (path == NULL || *path != '/') {
		g_debug ("Invalid path '%s'requested", path);
		soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
		return;
	}

	file = g_hash_table_lookup (priv->files, path + 1);
	if (!file) {
		soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
		return;
	}

	if (!file->mapped_file) {
		char *path;

		path = g_filename_from_uri (file->uri, NULL, NULL);
		//FIXME uri
		file->mapped_file = g_mapped_file_new (path, FALSE, NULL);
		g_free (path);

		if (!file->mapped_file) {
			soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
			return;
		}
	}

	if (msg->method == SOUP_METHOD_GET) {
		soup_message_set_response (msg, file->mime_type,
					   SOUP_MEMORY_STATIC,
					   g_mapped_file_get_contents (file->mapped_file),
					   g_mapped_file_get_length (file->mapped_file));
	} else {
		soup_message_headers_set_content_type (msg->response_headers,
						       file->mime_type, NULL);

		soup_message_headers_set_content_length (msg->response_headers,
							 g_mapped_file_get_length (file->mapped_file));
	}

	soup_message_set_status(msg, SOUP_STATUS_OK);
}

static void
remote_display_host_init (RemoteDisplayHost *host)
{
	RemoteDisplayHostPrivate *priv;

	priv = GET_PRIVATE (host);
	priv->files = g_hash_table_new_full (g_str_hash, g_str_equal,
					     g_free, (GDestroyNotify) file_free);
	priv->server = soup_server_new (NULL, NULL);
	soup_server_add_handler (priv->server, NULL,
				 server_callback, host, NULL);

}

RemoteDisplayHost *
remote_display_host_new (GInetAddress *remote_address,
			 GInetAddress *local_address)
{
	g_return_val_if_fail (G_INET_ADDRESS (remote_address), NULL);
	g_return_val_if_fail (G_INET_ADDRESS (local_address), NULL);

	return g_object_new (REMOTE_DISPLAY_TYPE_HOST,
			     "remote-address", remote_address,
			     "local-address", local_address,
			     NULL);
}

static char *
get_server_uri (SoupServer *server,
		const char *filename)
{
	GSList *uris;
	SoupURI *uri;
	char *ret;

	uris = soup_server_get_uris (server);
	g_assert (uris);

	uri = uris->data;
	ret = g_strdup_printf ("%s://%s:%d/%s",
			       soup_uri_get_scheme (uri),
			       soup_uri_get_host (uri),
			       soup_uri_get_port (uri),
			       filename);
	g_slist_free_full (uris, (GDestroyNotify) soup_uri_free);
	return ret;
}

char *
remote_display_host_file (RemoteDisplayHost *host,
			  const char        *uri,
			  GError           **error)
{
	RemoteDisplayHostPrivate *priv;
	RemoteDisplayHostFile *file;
	GChecksum *checksum;
	const char *str;
	char *ret;

	g_return_val_if_fail (REMOTE_DISPLAY_IS_HOST (host), FALSE);
	g_return_val_if_fail (uri != NULL, FALSE);

	priv = GET_PRIVATE (host);

	//FIXME if http or https, just pass the URL

	if (!priv->server_started) {
		GSocketAddress *addr;

		addr = g_inet_socket_address_new (priv->local_address, 0);
		if (!soup_server_listen (priv->server, addr, 0, error)) {
			g_clear_object (&addr);
			return FALSE;
		}
		g_object_unref (addr);
		priv->server_started = TRUE;
	}

	checksum = g_checksum_new (G_CHECKSUM_SHA256);
	g_checksum_update (checksum, (const guchar *) uri, strlen (uri));
	str = g_checksum_get_string (checksum);

	//FIXME add suffix and get mime type

	file = g_new0 (RemoteDisplayHostFile, 1);
	file->uri = g_strdup (uri);
	file->mime_type = g_strdup ("video/mp4");

	g_hash_table_insert (priv->files, g_strdup (str), file);

	ret = get_server_uri (priv->server, str);
	g_checksum_free (checksum);

	return ret;
}
