/* GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) 2008 Jan Schmidt <jan.schmidt@sun.com>
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gst/gst.h>
#include "visualization.h"
#include <unistd.h>

#define sampling_freq 8000
#define spect_bands 500
static GMainLoop *loop;

static void
quit_app ()
{
  g_main_loop_quit (loop);
}

/* receive spectral data from element message */
static gboolean
message_handler (GstBus * bus, GstMessage * message, gpointer data)
{
  if (message->type == GST_MESSAGE_ELEMENT) {
    const GstStructure *s = gst_message_get_structure (message);
    const gchar *name = gst_structure_get_name (s);
    GstClockTime endtime;

    if (strcmp (name, "spectrum") == 0) {
      const GValue *magnitudes;
      const GValue *mag;
      guint i;

      if (!gst_structure_get_clock_time (s, "endtime", &endtime))
        endtime = GST_CLOCK_TIME_NONE;

      magnitudes = gst_structure_get_value (s, "magnitude");
      static double mags[spect_bands];

      for (i = 0; i < spect_bands; ++i) {
        mag = gst_value_list_get_value (magnitudes, i);
        mags[i] = (double) g_value_get_float(mag);
      }

      visualization_feed_spectrum(mags, -80, -20);
    }
  } else if (message->type == GST_MESSAGE_ERROR) {
    gchar  *debug;
    GError *error;

    gst_message_parse_error (message, &error, &debug);

    g_printerr ("Error: %s\n%s\n", error->message, debug);
    g_error_free (error);
    g_free (debug);

    g_main_loop_quit (loop);
  }
  return TRUE;
}

int
main (int argc, char *argv[])
{
  GstElement *bin;
  GstElement *src, *audioconvert, *spectrum, *sink;
  GstBus *bus;
  GstCaps *caps;

  gst_init (&argc, &argv);
  visualization_launch(&argc, &argv, spect_bands, quit_app);

  bin = gst_pipeline_new ("bin");

  src = gst_element_factory_make ("autoaudiosrc", "src");
  g_assert (src);

  audioconvert = gst_element_factory_make ("audioconvert", NULL);
  g_assert (audioconvert);

  spectrum = gst_element_factory_make ("spectrum", "spectrum");
  g_object_set (G_OBJECT (spectrum),
                "bands", spect_bands,
                "threshold", -80,
                "post-messages", TRUE,
                "message-phase", TRUE,
                "interval", 10000000,
                NULL);

  sink = gst_element_factory_make ("fakesink", "sink");
  g_object_set (G_OBJECT (sink), "sync", TRUE, NULL);

  gst_bin_add_many (GST_BIN (bin), src, audioconvert, spectrum, sink, NULL);

  caps = gst_caps_new_simple ("audio/x-raw",
                              "rate", G_TYPE_INT, sampling_freq,
                              NULL);

  int ret;
  ret = gst_element_link(src, audioconvert);
  g_assert(ret);
  ret = gst_element_link_filtered (audioconvert, spectrum, caps);
  g_assert(ret);
  ret = gst_element_link (spectrum, sink);
  g_assert(ret);

  gst_caps_unref (caps);

  bus = gst_element_get_bus (bin);
  gst_bus_add_watch (bus, message_handler, NULL);
  gst_object_unref (bus);

  gst_element_set_state (bin, GST_STATE_PLAYING);

  /* we need to run a GLib main loop to get the messages */
  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);

  gst_element_set_state (bin, GST_STATE_NULL);

  gst_object_unref (bin);

  return 0;
}
