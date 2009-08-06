/* GStreamer Editing Services
 * Copyright (C) 2009 Edward Hervey <bilboed@bilboed.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _GES_TIMELINE_LAYER
#define _GES_TIMELINE_LAYER

#include <glib-object.h>
#include <ges/ges-types.h>

G_BEGIN_DECLS

#define GES_TYPE_TIMELINE_LAYER ges_timeline_layer_get_type()

#define GES_TIMELINE_LAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_TIMELINE_LAYER, GESTimelineLayer))

#define GES_TIMELINE_LAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_TIMELINE_LAYER, GESTimelineLayerClass))

#define GES_IS_TIMELINE_LAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_TIMELINE_LAYER))

#define GES_IS_TIMELINE_LAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_TIMELINE_LAYER))

#define GES_TIMELINE_LAYER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_TIMELINE_LAYER, GESTimelineLayerClass))

struct _GESTimelineLayer {
  GObject parent;

  GESTimeline *timeline;	/* The timeline where this layer is being used */

  GSList * objects_start;	/* The TimelineObjects sorted by start and priority */
};

struct _GESTimelineLayerClass {
  GObjectClass parent_class;

  void	(*object_added)		(GESTimelineLayer * layer, GESTimelineObject * object);
  void	(*object_removed)	(GESTimelineLayer * layer, GESTimelineObject * object);
};

GType ges_timeline_layer_get_type (void);

GESTimelineLayer* ges_timeline_layer_new (void);

void ges_timeline_layer_set_timeline (GESTimelineLayer * layer, GESTimeline * timeline);
gboolean ges_timeline_layer_add_object (GESTimelineLayer * layer, GESTimelineObject * object);

G_END_DECLS

#endif /* _GES_TIMELINE_LAYER */

