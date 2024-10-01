#include <iostream>

#include "video_pipeline.h"

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

bool VideoPipeline::runMainloop() {
    return true;
}
 
bool VideoPipeline::createPipelineStaticParts() {
    return true;
}

bool VideoPipeline::destroyPipeline() {
    return true;
}
