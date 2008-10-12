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
static GstElement *decodebin;

static GMainLoop *loop;

static gboolean
show_field (GQuark field_id,
            const GValue *value,
            gpointer user_data)
{
    const gchar *field;
    gchar *tmp;
    const gchar *type;

    field = g_quark_to_string (field_id);

    switch (G_VALUE_TYPE (value))
    {
        case G_TYPE_BOOLEAN:
            tmp = g_strdup_printf ("%s", g_value_get_boolean (value) ? "true" : "false");
            type = "bool";
            break;
        case G_TYPE_INT:
            tmp = g_strdup_printf ("%i", g_value_get_int (value));
            type = "int";
            break;
        case G_TYPE_STRING:
            tmp = g_strdup_printf ("%s", g_value_get_string (value));
            type = "string";
            break;
        default:
            if (GST_VALUE_HOLDS_FRACTION (value))
            {
                tmp = g_strdup_printf ("%i/%i",
                                       gst_value_get_fraction_numerator (value),
                                       gst_value_get_fraction_denominator (value));
                type = "fraction";
                break;
            }
#if 0
            tmp = g_strdup_printf ("(%s) unknown", G_VALUE_TYPE_NAME (value));
            type = "unknown";
#else
            goto leave;
#endif
            break;
    }

    g_print ("%s: %s: %s\n", field, type, tmp);

    g_free (tmp);

leave:
    return TRUE;
}

static void
show_caps (GstCaps *caps)
{
    guint i;

#if 0
    {
        gchar *tmp;
        tmp = gst_caps_to_string (caps);
        g_debug ("caps [%s]", tmp);
        g_free (tmp);
    }
#endif

    for (i = 0; i < gst_caps_get_size (caps); i++)
    {
        GstStructure *struc;
        struc = gst_caps_get_structure (caps, i);
        g_print ("%s\n", gst_structure_get_name (struc));
        gst_structure_foreach (struc, show_field, NULL);
        g_print ("\n");
    }
}

static void
show_info (void)
{
    GstIterator *it;
    gboolean done = FALSE;
    gpointer item;

    it = gst_element_iterate_src_pads (decodebin);

    while (!done)
    {
        switch (gst_iterator_next (it, &item))
        {
            case GST_ITERATOR_OK:
                {
                    GstPad *pad;
                    GstCaps *caps;
                    pad = GST_PAD (item);
                    caps = gst_pad_get_caps (pad);
                    show_caps (caps);
                    gst_caps_unref (caps);
                }
                break;
            case GST_ITERATOR_DONE:
                done = TRUE;
                break;
            default:
                break;
        }
    }

    gst_iterator_free (it);
}

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
        case GST_MESSAGE_STATE_CHANGED:
            {
                if (msg->src == GST_OBJECT (decodebin))
                {
                    GstState old, new, pending;
                    gst_message_parse_state_changed (msg, &old, &new, &pending);
#if 0
                    g_debug ("state changed: %s: %u, %u, %u", GST_OBJECT_NAME (msg->src), old, new, pending);
#endif
                    if (new == GST_STATE_PAUSED)
                    {
                        show_info ();
                        g_main_loop_quit (loop);
                    }
                }
                break;
            }
        default:
            break;
    }

    return TRUE;
}

static gboolean
continue_cb (GstElement *bin,
             GstPad *pad,
             GstCaps *caps,
             gpointer user_data)
{
    static guint count = 0;

    if (count++ == 0)
        return TRUE;

    return FALSE;
}

static void
identify (const char *location)
{
    pipeline = gst_pipeline_new ("gst-identify");

    {
        GstBus *bus;
        bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
        gst_bus_add_watch (bus, bus_cb, NULL);
        gst_object_unref (bus);
    }

    src = gst_element_factory_make ("filesrc", "src");
    g_object_set (G_OBJECT (src), "location", location, NULL);
    gst_bin_add (GST_BIN (pipeline), src);

    decodebin = gst_element_factory_make ("decodebin2", "decodebin");
    gst_bin_add (GST_BIN (pipeline), decodebin);

    gst_element_link_pads (src, "src", decodebin, "sink");

    g_signal_connect (decodebin, "autoplug-continue", G_CALLBACK (continue_cb), NULL);

    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    g_main_loop_run (loop);
}

int
main (int argc,
      char *argv[])
{
    gst_init (&argc, &argv);
    loop = g_main_loop_new (NULL, FALSE);

    identify (argv[1]);
    
    return 0;
}
