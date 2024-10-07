#include <iostream>

#include <gst/video/video-format.h>

#include "video_pipeline.h"

extern GMainLoop* mainloop;

std::string getMessageSourceDescription(GstMessage* msg) {
    std::string sourceDescription = " from ";
    if(msg->src && msg->src->name) {
        sourceDescription.append(msg->src->name);
    }
    sourceDescription.append(" of type ");
    if(msg->src) {
        sourceDescription.append(G_OBJECT_TYPE_NAME(msg->src));
    }
    else {
        sourceDescription.append("unknown");
    }
    
    return sourceDescription;
}

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

        std::cout << "Error " << error->code << getMessageSourceDescription(msg) << ": " << error->message << std::endl;
        g_error_free(error);
        
        g_main_loop_quit(mainloop);

        break;
    }
    
    case GST_MESSAGE_WARNING: {
        gchar  *debug;
        GError *error;

        gst_message_parse_warning(msg, &error, &debug);
        g_free(debug);

        std::cout << "Warning " << error->code << getMessageSourceDescription(msg) << ": " << error->message << std::endl;
        g_error_free(error);

        break;
    }
    
    default:
        break;
    }

    return TRUE;
}

const char* getInputSrcElementType(VideoPipelineInput input) {
    switch(input.type) {
    case VideoPipelineInputType::TESTPATTERN:
        return "videotestsrc";
    case VideoPipelineInputType::V4L2_CAMERA:
        return "v4l2src";
    }
    std::cout << "invalid input type" << std::endl;
    return "";
}

const char* getMimeTypeFromPixelFormat(uint32_t pixelformat) {
    if(pixelformatIsCompressed(pixelformat)) {
        switch(pixelformat) {
        case V4L2_PIX_FMT_JPEG:
            return "image/jpeg";
        case V4L2_PIX_FMT_H264:
            return "video/x-h264";
        case V4L2_PIX_FMT_HEVC:
            return "video/x-h265";
        }
    }
    return "video/x-raw";
}

const char* getVideoRawFormatFromPixelFormat(uint32_t pixelformat) {
    switch(pixelformat) {
    case V4L2_PIX_FMT_YUYV:
        return "YUY2";
    case V4L2_PIX_FMT_NV12:
        return "NV12";
    }
    return "";
}

std::vector<GstElement*> getCompressedFormatRequiredElements(VideoFormat format) {
    std::vector<GstElement*> elementsList;
    switch(format.pixelformat) {
    case V4L2_PIX_FMT_JPEG:
        elementsList.push_back(gst_element_factory_make("jpegdec", "jpegdec"));
        elementsList.push_back(gst_element_factory_make("queue", "queue_jpegdec"));
        break;
    default:
        break;
    }
    
    for(int i = 1; i < elementsList.size(); i++) {
        gst_element_link(elementsList[i - 1], elementsList[i]);
    }
    
    return elementsList;
}

bool VideoPipeline::addInput(VideoPipelineInput input) {
    //gst_video_format_from_fourcc pour convertir de pixelformat a un format gstreamer
    //ensuite utilser gst_video_format_to_string
    GstElement* source = gst_element_factory_make(getInputSrcElementType(input), "src");
    
    VideoFormat selectedFormat = input.videoFormats[1];
    
    GstCaps* caps = nullptr;
    std::vector<GstElement*> compressedFormatRequiredElements = getCompressedFormatRequiredElements(selectedFormat);
    if(pixelformatIsCompressed(selectedFormat.pixelformat)) {
        //for now handle only jpeg
        caps = gst_caps_new_simple(getMimeTypeFromPixelFormat(selectedFormat.pixelformat),
        "framerate", GST_TYPE_FRACTION, selectedFormat.frameIntervals[0].denominator, selectedFormat.frameIntervals[0].numerator,
        "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
        "width", G_TYPE_INT, selectedFormat.width,
        "height", G_TYPE_INT, selectedFormat.height,
        NULL);
    }
    else {
        caps = gst_caps_new_simple(getMimeTypeFromPixelFormat(selectedFormat.pixelformat),
        "format", G_TYPE_STRING, getVideoRawFormatFromPixelFormat(selectedFormat.pixelformat),
        "framerate", GST_TYPE_FRACTION, selectedFormat.frameIntervals[0].denominator, selectedFormat.frameIntervals[0].numerator,
        "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
        "width", G_TYPE_INT, selectedFormat.width,
        "height", G_TYPE_INT, selectedFormat.height,
        NULL);
    }
    
    GstElement* caps_filter = gst_element_factory_make("capsfilter", "caps_src");
    g_object_set(caps_filter, "caps", caps, NULL);
    GstElement* queue_src = gst_element_factory_make("queue", "queue_src");
    GstElement* videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    g_object_set(videoconvert, "n-threads", 4, NULL);
    
    gst_bin_add_many(GST_BIN(pipeline), source, caps_filter, queue_src, videoconvert, NULL);
    for(auto element: compressedFormatRequiredElements) {
        gst_bin_add(GST_BIN(pipeline), element);
    }
    
    gst_element_link_many(source, caps_filter, queue_src, NULL);
    if(!compressedFormatRequiredElements.empty()) {
        gst_element_link(queue_src, compressedFormatRequiredElements[0]);
        gst_element_link(compressedFormatRequiredElements.back(), videoconvert);
    }
    else {
        gst_element_link(queue_src, videoconvert);
    }
    gst_element_link(videoconvert, input_selector);
    
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

void VideoPipeline::createPipeline(std::vector<VideoPipelineInput> inputs) {
    pipeline = gst_pipeline_new("video_pipeline");
    
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_call, mainloop);
    gst_object_unref (bus);
    
    input_selector = gst_element_factory_make("input-selector", "input-selector");
    GstElement* sink = gst_element_factory_make("ximagesink", "sink");
    
    gst_bin_add_many(GST_BIN(pipeline), input_selector, sink, NULL);
    gst_element_link_many(input_selector, sink, NULL);
    
    for(auto input: inputs) {
        addInput(input);
    }
    
    gst_debug_bin_to_dot_file(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "video-pipeline");
}

void VideoPipeline::destroyPipeline() {
    if(pipeline) {
        setPipelineState(GST_STATE_NULL);
        gst_object_unref (pipeline);
        g_source_remove (bus_watch_id);
        pipeline = NULL;
        input_selector = NULL;
    }
}
