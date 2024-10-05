#include <csignal>

#include <atomic>
#include <iostream>

#include "video_pipeline.h"

std::atomic_bool stop_process = false;
GMainLoop* mainloop = NULL;

void sighandler(int signum) {
    stop_process = true;
}

gboolean handleDbusMessages(gpointer user_data) {
    //TODO
    //handle setting changes or provide infos about pipeline
    return G_SOURCE_CONTINUE;
}

static void restartVideoPipeline() {
    VideoPipeline& pipeline = VideoPipeline::getInstance();
    pipeline.destroyPipeline();
    pipeline.createPipeline();
    pipeline.setPipelineState(GST_STATE_PLAYING);
}

static void stopVideoPipeline() {
    VideoPipeline& pipeline = VideoPipeline::getInstance();
    pipeline.destroyPipeline();
}

int main(int argc, char* argv[]) {
    mainloop = g_main_loop_new(NULL, FALSE);
    g_timeout_add(100, handleDbusMessages, NULL);
    gst_init (&argc, &argv);
    while(!stop_process) {
        restartVideoPipeline();
        g_main_loop_run(mainloop);
    }
    stopVideoPipeline();
    gst_deinit();
    g_main_loop_unref(mainloop);
}

