#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include <libudev.h>
#include <linux/videodev2.h>

enum class VideoPipelineInputType {
    TESTPATTERN, V4L2_CAMERA
};

struct FrameInterval {
    uint32_t numerator;
    uint32_t denominator;
};

struct VideoFormat {
    uint32_t pixelformat;
    uint32_t width;
    uint32_t height;
    std::vector<FrameInterval> frameIntervals;
};

struct VideoPipelineInput {
    struct v4l2_capability devcap;
    std::vector<VideoFormat> videoFormats;
    std::string devpath;
    std::string devname;
    VideoPipelineInputType type;
};

class V4L2UdevMonitor {
public:
    virtual ~V4L2UdevMonitor();
    static V4L2UdevMonitor& getInstance() {
        static V4L2UdevMonitor instance;
        return instance;
    }

    std::vector<VideoPipelineInput> getV4l2DeviceList();
    bool updateV4L2DeviceList();
private:
    V4L2UdevMonitor();
    std::vector<VideoPipelineInput> probeDevices();

    std::vector<VideoPipelineInput> v4l2DeviceList;
    struct udev* udev = nullptr;
    struct udev_monitor* udevMonitor = nullptr;
};

bool pixelformatIsCompressed(uint32_t pixelformat);
