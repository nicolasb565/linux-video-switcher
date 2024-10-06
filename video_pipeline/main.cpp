#include <csignal>

#include <atomic>
#include <iostream>

#include "v4l2-udev.h"
#include "video_pipeline.h"

static std::atomic_bool stop_process = false;
GMainLoop* mainloop = NULL;

static void sighandler(int signum) {
    stop_process = true;
}

static gboolean handleDbusMessages(gpointer user_data) {
    //TODO
    //handle setting changes or provide infos about pipeline
    return G_SOURCE_CONTINUE;
}

static gboolean updateV4l2DeviceList(gpointer user_data) {
    V4L2UdevMonitor& v4l2DevMonitor = V4L2UdevMonitor::getInstance();
    if(v4l2DevMonitor.updateV4L2DeviceList()) {
        std::cout << "New v4l2 device list:" << std::endl;
        for(auto dev: v4l2DevMonitor.getV4l2DeviceList()) {
            std::cout << "    " << dev.name << " (" << dev.path << ")" << std::endl;
        }
    }
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
    g_timeout_add(500, updateV4l2DeviceList, NULL);
    gst_init (&argc, &argv);
    while(!stop_process) {
        updateV4l2DeviceList(NULL);
        restartVideoPipeline();
        g_main_loop_run(mainloop);
    }
    stopVideoPipeline();
    gst_deinit();
    g_main_loop_unref(mainloop);
}

