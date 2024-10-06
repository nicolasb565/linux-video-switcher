#pragma once

#include <string>

#include <gst/gst.h>
#include <glib.h>

#include "v4l2-udev.h"

class VideoPipeline {
public:
    virtual ~VideoPipeline() {
        destroyPipeline();
    }
    
    static VideoPipeline& getInstance() {
        static VideoPipeline instance;
        return instance;
    }

    bool addInput(VideoPipelineInput input);
    bool removeInput(VideoPipelineInput input);
    bool selectInput(VideoPipelineInput input);
    void setPipelineState(GstState state);
    void createPipeline();
    void destroyPipeline();
    
protected:
    GstElement* pipeline = NULL;
private:
    VideoPipeline() {}
    guint bus_watch_id;
};
