#include <csignal>

#include <atomic>
#include <iostream>

#include "video_pipeline.h"

std::atomic_bool stop_process = false;

void sighandler(int signum) {
    stop_process = true;
}

void handleDbusMessages(VideoPipeline& pipeline) {
    //TODO
    //handle setting changes or provide infos about pipeline
}

int main(int argc, char* argv[]) {
    gst_init (&argc, &argv);
    VideoPipeline pipeline;
    while(!stop_process) {
        handleDbusMessages(pipeline);
        if(!pipeline.runMainloop()) {
            break;
        }
    }
    gst_deinit();
}

