
#include "config.h"
#include <locale.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <libremote-display/remote-display.h>
#include <libremote-display/remote-display-private.h>

static GMainLoop *loop = NULL;
static GList *files = NULL;
static char *target_device = NULL;

static const gchar *
get_type_name (GType class_type, int type)
{
	GEnumClass *eclass;
	GEnumValue *value;

	eclass = G_ENUM_CLASS (g_type_class_peek (class_type));
	value = g_enum_get_value (eclass, type);

	if (value == NULL)
		return "unknown";

	return value->value_nick;
}

static void
device_state_changed_cb (RemoteDisplayDevice      *device,
			 RemoteDisplayDeviceState  state,
			 gpointer                  user_data)
{
	const char *state_s;

	state_s = get_type_name (REMOTE_DISPLAY_TYPE_DISPLAY_DEVICE_STATE, state);
	g_message ("state changed to %s (%d)", state_s, state);
}

static void
device_appeared_cb (RemoteDisplayManager *manager,
		    RemoteDisplayDevice  *device,
		    gpointer              user_data)
{
	if (target_device != NULL) {
		char *name;

		g_object_get (G_OBJECT (device), "name", &name, NULL);
		if (g_strcmp0 (name, target_device) == 0) {
			g_print ("Device '%s' appeared, will start playing", name);
			g_signal_connect (G_OBJECT (device), "state-changed",
					  G_CALLBACK (device_state_changed_cb), NULL);
			remote_display_device_open_and_play (device, files->data, 0.0);
		}
	} else {
		char *str;

		g_print ("Detected new device:\n");
		str = remote_display_device_to_string (device);
		g_print ("%s", str);
		g_free (str);
	}
}

static void
device_disappeared_cb (RemoteDisplayManager *manager,
		       RemoteDisplayDevice  *device,
		       gpointer              user_data)
{
	char *name;

	g_object_get (G_OBJECT (device), "name", &name, NULL);
	g_print ("Device disappeared: %s\n", name);
	g_free (name);
}

static void
show_help (GOptionContext *context)
{
	char *help;

	help = g_option_context_get_help (context, FALSE, NULL);
	g_printerr ("%s", help);
	g_free (help);
}

static gboolean
stop_scanning_cb (gpointer user_data)
{
	g_main_loop_quit (loop);
	return G_SOURCE_REMOVE;
}

int main (int argc, char **argv)
{
	GError *error = NULL;
	GOptionContext *context;
	gboolean list_devices = FALSE;
	gboolean monitor_devices = FALSE;
	char **params = NULL;
	const GOptionEntry entries[] = {
		{ "list-devices", 'l', 0, G_OPTION_ARG_NONE, &list_devices, "List devices on the network", NULL },
		{ "monitor-devices", 'm', 0, G_OPTION_ARG_NONE, &monitor_devices, "Monitor devices on the network", NULL },
		{ "device", 'd', 0, G_OPTION_ARG_STRING, &target_device, NULL },
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &params, NULL, "[FILENAMES...]" },
		{ NULL }
	};
	RemoteDisplayManager *manager;

	setlocale (LC_ALL, "");

	/* Parse our own command-line options */
	context = g_option_context_new ("- test remote display");
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);

	if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
		g_print ("Option parsing failed: %s\n", error->message);
		return 1;
	}

	//FIXME Do a better job at verifying options
	if (!list_devices &&
	    !monitor_devices &&
	    (!target_device || !params)) {
		show_help (context);
		return 1;
	}

	manager = remote_display_manager_new ();
	g_signal_connect (G_OBJECT (manager), "device-appeared",
			  G_CALLBACK (device_appeared_cb), NULL);
	g_signal_connect (G_OBJECT (manager), "device-disappeared",
			  G_CALLBACK (device_disappeared_cb), NULL);

	if (list_devices)
		g_timeout_add_seconds (1, stop_scanning_cb, NULL);
	else if (monitor_devices)
		;
	else if (target_device && params) {
		guint i;
		for (i = 0; params[i]; i++)
			files = g_list_prepend (files, params[i]);
		files = g_list_reverse (files);
		g_print ("Waiting for device to appear\n");
	}

	loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (loop);

	g_object_unref (manager);

	return 0;
}
