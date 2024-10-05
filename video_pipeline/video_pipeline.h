#pragma once

#include <string>

#include <gst/gst.h>
#include <glib.h>

enum class VideoPipelineInputType {
    TESTPATTERN, V4L2_CAMERA
};

struct VideoPipelineInput {
    std::string name;
    VideoPipelineInputType type;
    union {
        int pattern;
        int videodevNo;
    } id;
};

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
