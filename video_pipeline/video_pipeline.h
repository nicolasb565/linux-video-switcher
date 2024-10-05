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
    bool setPipelineState(GstState state, GstClockTime timeout_ns);
    
protected:
    bool createPipelineStaticParts();
    bool destroyPipeline();
    
    GstElement* pipeline;
private:
    VideoPipeline() {
        createPipelineStaticParts();
    }
};
