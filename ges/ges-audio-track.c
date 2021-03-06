/* GStreamer Editing Services
 * Copyright (C) <2013> Thibault Saunier <thibault.saunier@collabora.com>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * SECTION: gesaudiotrack
 * @short_description: A standard GESTrack for raw audio
 *
 * Sane default properties to specify and fixate the output stream are
 * set as restriction-caps.
 * It is advised, to modify these properties, to use
 * #ges_track_update_restriction_caps, setting them directly is
 * possible through #ges_track_set_restriction_caps, but not specifying
 * one of them can lead to negotiation issues, only use that function
 * if you actually know what you're doing :)
 *
 * The default properties are:
 * - format: S32LE
 * - channels: 2
 * - rate: 44100
 * - layout: interleaved
 */

#include "ges-internal.h"
#include "ges-smart-adder.h"
#include "ges-audio-track.h"

#define DEFAULT_CAPS "audio/x-raw"

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define DEFAULT_RESTRICTION_CAPS "audio/x-raw, format=S32LE, channels=2, "\
  "rate=44100, layout=interleaved"
#else
#define DEFAULT_RESTRICTION_CAPS "audio/x-raw, format=S32BE, channels=2, "\
  "rate=44100, layout=interleaved"
#endif

struct _GESAudioTrackPrivate
{
  gpointer nothing;
};

G_DEFINE_TYPE (GESAudioTrack, ges_audio_track, GES_TYPE_TRACK);

/****************************************************
 *              Private methods and utils           *
 ****************************************************/
static GstElement *
create_element_for_raw_audio_gap (GESTrack * track)
{
  GstElement *elem;

  elem = gst_element_factory_make ("audiotestsrc", NULL);
  g_object_set (elem, "wave", 4, NULL);

  return elem;
}


/****************************************************
 *              GObject vmethods implementations    *
 ****************************************************/

static void
ges_audio_track_init (GESAudioTrack * self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), GES_TYPE_AUDIO_TRACK,
      GESAudioTrackPrivate);
}

static void
ges_audio_track_finalize (GObject * object)
{
  /* TODO: Add deinitalization code here */

  G_OBJECT_CLASS (ges_audio_track_parent_class)->finalize (object);
}

static void
ges_audio_track_class_init (GESAudioTrackClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
/*   GESTrackClass *parent_class = GES_TRACK_CLASS (klass);
 */

  g_type_class_add_private (klass, sizeof (GESAudioTrackPrivate));

  object_class->finalize = ges_audio_track_finalize;

  GES_TRACK_CLASS (klass)->get_mixing_element = ges_smart_adder_new;
}

/****************************************************
 *              API implementation                  *
 ****************************************************/
/**
 * ges_audio_track_new:
 *
 * Creates a new #GESAudioTrack of type #GES_TRACK_TYPE_AUDIO and with generic
 * raw audio caps ("audio/x-raw");
 *
 * Returns: (transfer floating): A new #GESTrack
 */
GESAudioTrack *
ges_audio_track_new (void)
{
  GESAudioTrack *ret;
  GstCaps *caps = gst_caps_from_string (DEFAULT_CAPS);
  GstCaps *restriction_caps = gst_caps_from_string (DEFAULT_RESTRICTION_CAPS);

  ret = g_object_new (GES_TYPE_AUDIO_TRACK, "caps", caps,
      "track-type", GES_TRACK_TYPE_AUDIO, NULL);

  ges_track_set_create_element_for_gap_func (GES_TRACK (ret),
      create_element_for_raw_audio_gap);

  ges_track_set_restriction_caps (GES_TRACK (ret), restriction_caps);

  gst_caps_unref (caps);
  gst_caps_unref (restriction_caps);

  return ret;
}
