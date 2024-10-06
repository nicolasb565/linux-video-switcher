#pragma once

#include <string>
#include <vector>

#include <libudev.h>

enum class VideoPipelineInputType {
    TESTPATTERN, V4L2_CAMERA
};

struct VideoPipelineInput {
    std::string name;
    std::string path;
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
