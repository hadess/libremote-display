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

#ifndef __REMOTE_DISPLAY_MANAGER_H__
#define __REMOTE_DISPLAY_MANAGER_H__

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define REMOTE_DISPLAY_TYPE_MANAGER (remote_display_manager_get_type())
#define REMOTE_DISPLAY_MANAGER(obj)                                                 \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
				     REMOTE_DISPLAY_TYPE_MANAGER,                        \
				     RemoteDisplayManager))
#define REMOTE_DISPLAY_MANAGER_CLASS(klass)                                         \
	(G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
				  REMOTE_DISPLAY_TYPE_MANAGER,                           \
				  RemoteDisplayManagerClass))
#define IS_REMOTE_DISPLAY_MANAGER(obj)                                              \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
				     REMOTE_DISPLAY_TYPE_MANAGER))
#define IS_REMOTE_DISPLAY_MANAGER_CLASS(klass)                                      \
	(G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
				  REMOTE_DISPLAY_TYPE_MANAGER))
#define REMOTE_DISPLAY_MANAGER_GET_CLASS(obj)                                       \
	(G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
				    REMOTE_DISPLAY_TYPE_MANAGER,                         \
				    RemoteDisplayManagerClass))

typedef struct _RemoteDisplayManagerPrivate RemoteDisplayManagerPrivate;
typedef struct _RemoteDisplayManager      RemoteDisplayManager;
typedef struct _RemoteDisplayManagerClass RemoteDisplayManagerClass;

struct _RemoteDisplayManager {
	GObject parent;

	RemoteDisplayManagerPrivate *priv;
};

struct _RemoteDisplayManagerClass {
	GObjectClass parent_class;
};

GType remote_display_manager_get_type (void) G_GNUC_CONST;

RemoteDisplayManager *remote_display_manager_new (void);

G_END_DECLS

#endif /* __REMOTE_DISPLAY_MANAGER_H__ */
