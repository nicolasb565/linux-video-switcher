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

std::string fcc2s(unsigned int val) {
    std::string s;

    s += val & 0xff;
    s += (val >> 8) & 0xff;
    s += (val >> 16) & 0xff;
    s += (val >> 24) & 0xff;
    return s;
}

static gboolean updateV4l2DeviceList(gpointer user_data) {
    V4L2UdevMonitor& v4l2DevMonitor = V4L2UdevMonitor::getInstance();
    if(v4l2DevMonitor.updateV4L2DeviceList()) {
        std::cout << "New v4l2 device list:" << std::endl;
        for(auto& dev: v4l2DevMonitor.getV4l2DeviceList()) {
            std::cout << "    " << dev.devcap.card << " (" << dev.devname << " " << dev.devcap.bus_info << ")" << std::endl;
            for(auto& format: dev.videoFormats) {
                std::cout << "        " << format.width << "x" << format.height << " " << fcc2s(format.pixelformat) << std::endl;
                for(auto& interval: format.frameIntervals) {
                    std::cout << "            " << (double)interval.denominator / (double)interval.numerator << " fps" << std::endl;
                }
            }
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

