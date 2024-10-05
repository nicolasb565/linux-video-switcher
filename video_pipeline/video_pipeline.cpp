#include <iostream>

#include "video_pipeline.h"

gboolean handleDbusMessages(gpointer user_data);

static gboolean bus_call(GstBus* bus, GstMessage* msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *) data;

    switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
        g_print ("End of stream\n");
        g_main_loop_quit (loop);
        break;

    case GST_MESSAGE_ERROR: {
        gchar  *debug;
        GError *error;

        gst_message_parse_error (msg, &error, &debug);
        g_free (debug);

        g_printerr ("Error: %s\n", error->message);
        g_error_free (error);

        g_main_loop_quit (loop);
        break;
    }
    default:
        break;
    }

    return TRUE;
}

bool VideoPipeline::addInput(VideoPipelineInput input) {
    return true;
}

bool VideoPipeline::removeInput(VideoPipelineInput input) {
    return true;
}

bool VideoPipeline::selectInput(VideoPipelineInput input) {
    return true;
}

bool VideoPipeline::setPipelineState(GstState state, GstClockTime timeout_ns) {
    return true;
}

void VideoPipeline::runMainloop() {
    g_main_loop_run (mainloop);
}

bool VideoPipeline::createPipelineStaticParts() {
    mainloop = g_main_loop_new(NULL, FALSE);
    pipeline = gst_pipeline_new("video_pipeline");
    
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline));
    guint bus_watch_id = gst_bus_add_watch(bus, bus_call, mainloop);
    gst_object_unref (bus);
    
    g_timeout_add(100, handleDbusMessages, NULL);
    
    GstElement* source = gst_element_factory_make("videotestsrc", "src");
    GstCaps* caps = gst_caps_new_simple("video/x-raw",
     "format", G_TYPE_STRING, "I420",
     "framerate", GST_TYPE_FRACTION, 30, 1,
     "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
     "width", G_TYPE_INT, 1280,
     "height", G_TYPE_INT, 720,
     NULL);
    GstElement* caps_filter = gst_element_factory_make("capsfilter", "caps_src");
    g_object_set(caps_filter, "caps", caps, NULL);
    GstElement* queue_src = gst_element_factory_make("queue", "queue_src");
    GstElement* videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    g_object_set(videoconvert, "n-threads", 4, NULL);
    GstElement* input_selector = gst_element_factory_make("input-selector", "input-selector");
    GstElement* sink = gst_element_factory_make("ximagesink", "sink");
    
    gst_bin_add_many(GST_BIN(pipeline), source, caps_filter, queue_src, videoconvert, input_selector, sink, NULL);
    gst_element_link_many(source, caps_filter, queue_src, videoconvert, input_selector, sink, NULL);
    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    
    return true;
}

bool VideoPipeline::destroyPipeline() {
    return true;
}
