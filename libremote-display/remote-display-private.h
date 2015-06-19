/*
   Copyright (C) 2015 Bastien Nocera

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301  USA.

   Authors: Bastien Nocera <hadess@hadess.net>

 */

#ifndef REMOTE_DISPLAY_PRIVATE_H
#define REMOTE_DISPLAY_PRIVATE_H

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	AIRPLAY_VIDEO_SUPPORT        = 1 << 0,
	AIRPLAY_PHOTO_SUPPORT        = 1 << 1,
	AIRPLAY_VIDEO_HLS_SUPPORT    = 1 << 4,
	AIRPLAY_VIDEO_SCREEN_SUPPORT = 1 << 7,
	AIRPLAY_VIDEO_AUDIO          = 1 << 9,
	AIRPLAY_VIDEO_PHOTO_CACHING  = 1 << 13
} AirplayFeaturesMask;

G_END_DECLS

#endif /* REMOTE_DISPLAY_PRIVATE_H */
