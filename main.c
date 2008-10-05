/*
 * Copyright (C) 2008 Felipe Contreras.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <gst/gst.h>

static GstElement *pipeline;
static GstElement *src;
static GstElement *demuxer;
static GstElement *muxer;
static GstElement *sink;

GMainLoop *loop;

static gboolean
bus_cb (GstBus *bus,
        GstMessage *msg,
        gpointer data)
{
    switch (GST_MESSAGE_TYPE (msg))
    {
        case GST_MESSAGE_EOS:
            {
                g_debug ("end-of-stream");
                g_main_loop_quit (loop);
                break;
            }
        case GST_MESSAGE_ERROR:
            {
                gchar *debug;
                GError *err;

                gst_message_parse_error (msg, &err, &debug);
                g_free (debug);

                g_warning ("error: %s", err->message);
                g_error_free (err);

                g_main_loop_quit (loop);
                break;
            }
        default:
            break;
    }

    return TRUE;
}

static void
pad_added_cb (GstElement *element,
              GstPad *pad,
              gpointer data)
{
    GstCaps *caps;
    GstStructure *str;

    caps = gst_pad_get_caps (pad);
    str = gst_caps_get_structure (caps, 0);

    {
        gchar *tmp;
        tmp = gst_caps_to_string (caps);
        g_debug ("caps [%s]", tmp);
        g_free (tmp);
    }

    if (g_strrstr (gst_structure_get_name (str), "video"))
    {
        GstPad *sinkpad;
        {
            gint width, height;
            g_print ("name: %s\n", gst_structure_get_name (str));
            if (gst_structure_get_int (str, "width", &width))
                g_print ("width: %i\n", width);
            if (gst_structure_get_int (str, "height", &height))
                g_print ("width: %i\n", height);
        }
        sinkpad = gst_element_get_request_pad (muxer, "video_%d");
        gst_pad_link (pad, sinkpad);
        gst_object_unref (sinkpad);
    }

    if (g_strrstr (gst_structure_get_name (str), "audio"))
    {
        g_debug ("audio pad");
        {
            gint channels, rate;
            g_print ("name: %s\n", gst_structure_get_name (str));
            if (gst_structure_get_int (str, "channels", &channels))
                g_print ("channels: %i\n", channels);
            if (gst_structure_get_int (str, "rate", &rate))
                g_print ("rate: %i\n", rate);
        }
    }

    gst_caps_unref (caps);
}

static void
test (const char *location)
{
    pipeline = gst_pipeline_new ("gst-transcode");

    {
        GstBus *bus;
        bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
        gst_bus_add_watch (bus, bus_cb, NULL);
        gst_object_unref (bus);
    }

    src = gst_element_factory_make ("filesrc", "src");
    g_object_set (G_OBJECT (src), "location", location, NULL);
    gst_bin_add (GST_BIN (pipeline), src);

    demuxer = gst_element_factory_make ("matroskademux", "demuxer");
    gst_bin_add (GST_BIN (pipeline), demuxer);

    gst_element_link_pads (src, "src", demuxer, "sink");

    g_signal_connect (demuxer, "pad-added", G_CALLBACK (pad_added_cb), NULL);

    muxer = gst_element_factory_make ("matroskamux", "muxer");
    gst_bin_add (GST_BIN (pipeline), muxer);

    sink = gst_element_factory_make ("filesink", "sink");
    g_object_set (G_OBJECT (sink), "location", "foobar", NULL);
    gst_bin_add (GST_BIN (pipeline), sink);

    gst_element_link_pads (muxer, "src", sink, "sink");

    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    g_main_loop_run (loop);
}

int
main (int argc,
      char *argv[])
{
    gst_init (&argc, &argv);
    loop = g_main_loop_new (NULL, FALSE);

    test ("/data/public/videos/cc/big_buck_bunny_720p_h264.mkv");
    
    return 0;
}
