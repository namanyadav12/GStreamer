/* GStreamer Editing Services
 * Copyright (C) 2010 Brandon Lewis <brandon.lewis@collabora.co.uk>
 *               2010 Nokia Corporation
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
 * SECTION:ges-formatter
 * @short_description: Timeline saving and loading.
 *
 **/

#include <gst/gst.h>
#include <gio/gio.h>
#include <stdlib.h>

#include "ges-formatter.h"
#include "ges-internal.h"
#include "ges.h"

/* TODO Add a GCancellable somewhere in the API */
static void ges_extractable_interface_init (GESExtractableInterface * iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (GESFormatter, ges_formatter,
    G_TYPE_INITIALLY_UNOWNED, G_IMPLEMENT_INTERFACE (GES_TYPE_EXTRACTABLE,
        ges_extractable_interface_init));

struct _GESFormatterPrivate
{
  gpointer nothing;
};

static void ges_formatter_dispose (GObject * object);
static gboolean default_can_load_uri (GESFormatterClass * class,
    const gchar * uri, GError ** error);
static gboolean default_can_save_uri (GESFormatterClass * class,
    const gchar * uri, GError ** error);

/* GESExtractable implementation */
static gchar *
extractable_check_id (GType type, const gchar * id)
{
  GESFormatterClass *class;

  if (id)
    return g_strdup (id);

  class = g_type_class_peek (type);
  return g_strdup (class->name);
}

static gchar *
extractable_get_id (GESExtractable * self)
{
  GESAsset *asset;

  if (!(asset = ges_extractable_get_asset (self)))
    return NULL;

  return g_strdup (ges_asset_get_id (asset));
}

static gboolean
_register_metas (GESExtractableInterface * iface, GObjectClass * class,
    GESAsset * asset)
{
  GESFormatterClass *fclass = GES_FORMATTER_CLASS (class);
  GESMetaContainer *container = GES_META_CONTAINER (asset);

  ges_meta_container_register_meta_string (container, GES_META_READABLE,
      GES_META_FORMATTER_NAME, fclass->name);
  ges_meta_container_register_meta_string (container, GES_META_READABLE,
      GES_META_DESCRIPTION, fclass->description);
  ges_meta_container_register_meta_string (container, GES_META_READABLE,
      GES_META_FORMATTER_MIMETYPE, fclass->mimetype);
  ges_meta_container_register_meta_string (container, GES_META_READABLE,
      GES_META_FORMATTER_EXTENSION, fclass->extension);
  ges_meta_container_register_meta_double (container, GES_META_READABLE,
      GES_META_FORMATTER_VERSION, fclass->version);
  ges_meta_container_register_meta_uint (container, GES_META_READABLE,
      GES_META_FORMATTER_RANK, fclass->rank);

  return TRUE;
}

static void
ges_extractable_interface_init (GESExtractableInterface * iface)
{
  iface->check_id = (GESExtractableCheckId) extractable_check_id;
  iface->get_id = extractable_get_id;
  iface->asset_type = GES_TYPE_ASSET;
  iface->register_metas = _register_metas;
}

static void
ges_formatter_class_init (GESFormatterClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESFormatterPrivate));

  object_class->dispose = ges_formatter_dispose;

  klass->can_load_uri = default_can_load_uri;
  klass->can_save_uri = default_can_save_uri;
  klass->load_from_uri = NULL;
  klass->save_to_uri = NULL;

  /* We set dummy  metas */
  klass->name = "base-formatter";
  klass->extension = "noextension";
  klass->description = "Formatter base class, you should give"
      " a name to your formatter";
  klass->mimetype = "No mimetype";
  klass->version = 0.0;
  klass->rank = GST_RANK_NONE;
}

static void
ges_formatter_init (GESFormatter * object)
{
  object->priv = G_TYPE_INSTANCE_GET_PRIVATE (object,
      GES_TYPE_FORMATTER, GESFormatterPrivate);
  object->project = NULL;
}

static void
ges_formatter_dispose (GObject * object)
{
  ges_formatter_set_project (GES_FORMATTER (object), NULL);
}

static gboolean
default_can_load_uri (GESFormatterClass * class, const gchar * uri,
    GError ** error)
{
  GST_DEBUG ("%s: no 'can_load_uri' vmethod implementation",
      g_type_name (G_OBJECT_CLASS_TYPE (class)));
  return FALSE;
}

static gboolean
default_can_save_uri (GESFormatterClass * class,
    const gchar * uri, GError ** error)
{
  GST_DEBUG ("%s: no 'can_save_uri' vmethod implementation",
      g_type_name (G_OBJECT_CLASS_TYPE (class)));
  return FALSE;
}

/**
 * ges_formatter_can_load_uri:
 * @uri: a #gchar * pointing to the URI
 * @error: A #GError that will be set in case of error
 *
 * Checks if there is a #GESFormatter available which can load a #GESTimeline
 * from the given URI.
 *
 * Returns: TRUE if there is a #GESFormatter that can support the given uri
 * or FALSE if not.
 */

gboolean
ges_formatter_can_load_uri (const gchar * uri, GError ** error)
{
  gboolean ret = FALSE;
  GList *formatter_assets, *tmp;
  GESFormatterClass *class = NULL;

  if (!(gst_uri_is_valid (uri))) {
    GST_ERROR ("Invalid uri!");
    return FALSE;
  }

  if (!(gst_uri_has_protocol (uri, "file"))) {
    gchar *proto = gst_uri_get_protocol (uri);
    GST_ERROR ("Unspported protocol '%s'", proto);
    g_free (proto);
    return FALSE;
  }

  formatter_assets = ges_list_assets (GES_TYPE_FORMATTER);
  for (tmp = formatter_assets; tmp; tmp = tmp->next) {
    GESAsset *asset = GES_ASSET (tmp->data);

    class = g_type_class_ref (ges_asset_get_extractable_type (asset));
    if (class->can_load_uri (class, uri, error)) {
      g_type_class_unref (class);
      ret = TRUE;
      break;
    }
    g_type_class_unref (class);
  }

  g_list_free (formatter_assets);
  return ret;
}

/**
 * ges_formatter_can_save_uri:
 * @uri: a #gchar * pointing to a URI
 * @error: A #GError that will be set in case of error
 *
 * Returns TRUE if there is a #GESFormatter available which can save a
 * #GESTimeline to the given URI.
 *
 * Returns: TRUE if the given @uri is supported, else FALSE.
 */

gboolean
ges_formatter_can_save_uri (const gchar * uri, GError ** error)
{
  if (!(gst_uri_is_valid (uri))) {
    GST_ERROR ("%s invalid uri!", uri);
    return FALSE;
  }

  if (!(gst_uri_has_protocol (uri, "file"))) {
    gchar *proto = gst_uri_get_protocol (uri);
    GST_ERROR ("Unspported protocol '%s'", proto);
    g_free (proto);
    return FALSE;
  }

  /* TODO: implement file format registry */
  /* TODO: search through the registry and chose a GESFormatter class that can
   * handle the URI.*/

  return TRUE;
}

/**
 * ges_formatter_load_from_uri:
 * @formatter: a #GESFormatter
 * @timeline: a #GESTimeline
 * @uri: a #gchar * pointing to a URI
 * @error: A #GError that will be set in case of error
 *
 * Load data from the given URI into timeline.
 *
 * Returns: TRUE if the timeline data was successfully loaded from the URI,
 * else FALSE.
 */

gboolean
ges_formatter_load_from_uri (GESFormatter * formatter, GESTimeline * timeline,
    const gchar * uri, GError ** error)
{
  gboolean ret = FALSE;
  GESFormatterClass *klass = GES_FORMATTER_GET_CLASS (formatter);

  g_return_val_if_fail (GES_IS_FORMATTER (formatter), FALSE);
  g_return_val_if_fail (GES_IS_TIMELINE (timeline), FALSE);

  if (klass->load_from_uri) {
    ges_timeline_enable_update (timeline, FALSE);
    formatter->timeline = timeline;
    ret = klass->load_from_uri (formatter, timeline, uri, error);
    ges_timeline_enable_update (timeline, TRUE);
  }

  return ret;
}

/**
 * ges_formatter_save_to_uri:
 * @formatter: a #GESFormatter
 * @timeline: a #GESTimeline
 * @uri: a #gchar * pointing to a URI
 * @overwrite: %TRUE to overwrite file if it exists
 * @error: A #GError that will be set in case of error
 *
 * Save data from timeline to the given URI.
 *
 * Returns: TRUE if the timeline data was successfully saved to the URI
 * else FALSE.
 */

gboolean
ges_formatter_save_to_uri (GESFormatter * formatter, GESTimeline *
    timeline, const gchar * uri, gboolean overwrite, GError ** error)
{
  GError *lerr = NULL;
  gboolean ret = FALSE;
  GESFormatterClass *klass = GES_FORMATTER_GET_CLASS (formatter);

  GST_DEBUG_OBJECT (formatter, "Saving %" GST_PTR_FORMAT " to %s",
      timeline, uri);
  if (klass->save_to_uri)
    ret = klass->save_to_uri (formatter, timeline, uri, overwrite, &lerr);
  else
    GST_ERROR_OBJECT (formatter, "save_to_uri not implemented!");

  if (lerr) {
    GST_WARNING_OBJECT (formatter, "%" GST_PTR_FORMAT
        " not saved to %s error: %s", timeline, uri, lerr->message);
    g_propagate_error (error, lerr);
  } else
    GST_INFO_OBJECT (formatter, "%" GST_PTR_FORMAT
        " saved to %s", timeline, uri);

  return ret;
}

/**
 * ges_formatter_get_default:
 *
 * Get the default #GESAsset to use as formatter. It will return
 * the asset for the #GESFormatter that has the highest @rank
 *
 * Returns: (transfer none): The #GESAsset for the formatter with highest @rank
 */
GESAsset *
ges_formatter_get_default (void)
{
  GList *assets, *tmp;
  GESAsset *ret = NULL;
  GstRank tmprank, rank = GST_RANK_NONE;

  assets = ges_list_assets (GES_TYPE_FORMATTER);
  for (tmp = assets; tmp; tmp = tmp->next) {
    tmprank = GST_RANK_NONE;
    ges_meta_container_get_uint (GES_META_CONTAINER (tmp->data),
        GES_META_FORMATTER_RANK, &tmprank);

    if (tmprank > rank) {
      rank = tmprank;
      ret = GES_ASSET (tmp->data);
    }
  }
  g_list_free (assets);

  return ret;
}

void
ges_formatter_class_register_metas (GESFormatterClass * class,
    const gchar * name, const gchar * description, const gchar * extension,
    const gchar * mimetype, gdouble version, GstRank rank)
{
  class->name = name;
  class->description = description;
  class->extension = extension;
  class->mimetype = mimetype;
  class->version = version;
  class->rank = rank;
}

/* Main Formatter methods */

/*< protected >*/
void
ges_formatter_set_project (GESFormatter * formatter, GESProject * project)
{
  formatter->project = project;
}

GESProject *
ges_formatter_get_project (GESFormatter * formatter)
{
  return formatter->project;
}

static void
_list_formatters (GType * formatters, guint n_formatters)
{
  GType *tmptypes, type;
  guint tmp_n_types, i;

  for (i = 0; i < n_formatters; i++) {
    type = formatters[i];
    tmptypes = g_type_children (type, &tmp_n_types);
    if (tmp_n_types) {
      /* Recurse as g_type_children does not */
      _list_formatters (tmptypes, tmp_n_types);
      g_free (tmptypes);
    }

    if (G_TYPE_IS_ABSTRACT (type)) {
      GST_DEBUG ("%s is abstract, not using", g_type_name (type));
    } else {
      gst_object_unref (ges_asset_request (type, NULL, NULL));
    }
  }
}

void
_init_formatter_assets (void)
{
  GType *formatters;
  guint n_formatters;

  formatters = g_type_children (GES_TYPE_FORMATTER, &n_formatters);
  _list_formatters (formatters, n_formatters);
  g_free (formatters);
}

GESAsset *
_find_formatter_asset_for_uri (const gchar * uri)
{
  GESFormatterClass *class = NULL;
  GList *formatter_assets, *tmp;
  GESAsset *asset = NULL;

  formatter_assets = ges_list_assets (GES_TYPE_FORMATTER);
  for (tmp = formatter_assets; tmp; tmp = tmp->next) {
    asset = GES_ASSET (tmp->data);
    class = g_type_class_ref (ges_asset_get_extractable_type (asset));
    if (class->can_load_uri (class, uri, NULL)) {
      g_type_class_unref (class);
      asset = gst_object_ref (asset);
      break;
    }

    asset = NULL;
    g_type_class_unref (class);
  }

  g_list_free (formatter_assets);

  return asset;
}
