#include <iostream>

#include "video_pipeline.h"

extern GMainLoop* mainloop;
gboolean handleDbusMessages(gpointer user_data);

static gboolean bus_call(GstBus* bus, GstMessage* msg, gpointer data) {
    GMainLoop* mainloop = (GMainLoop *) data;

    switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
        std::cout << "End of stream" << std::endl;
        break;

    case GST_MESSAGE_ERROR: {
        gchar  *debug;
        GError *error;

        gst_message_parse_error(msg, &error, &debug);
        g_free(debug);

        std::cout << "Error: " << error->message << std::endl;
        g_error_free(error);
        
        g_main_loop_quit(mainloop);

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

void VideoPipeline::setPipelineState(GstState state) {
    gst_element_set_state (pipeline, state);
}

void VideoPipeline::createPipeline() {
    pipeline = gst_pipeline_new("video_pipeline");
    
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_call, mainloop);
    gst_object_unref (bus);
    
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
}

void VideoPipeline::destroyPipeline() {
    if(pipeline) {
        setPipelineState(GST_STATE_NULL);
        gst_object_unref (pipeline);
        g_source_remove (bus_watch_id);
        pipeline = NULL;
    }
}
