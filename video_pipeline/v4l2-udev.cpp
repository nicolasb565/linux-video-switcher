#include <iostream>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "v4l2-udev.h"

V4L2UdevMonitor::V4L2UdevMonitor() {
    udev = udev_new();
    udevMonitor = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(udevMonitor, "video4linux", NULL);
    udev_monitor_enable_receiving(udevMonitor);
    if(!updateV4L2DeviceList()) {
        v4l2DeviceList = probeDevices();
    }
}
    
V4L2UdevMonitor::~V4L2UdevMonitor() {
    udev_monitor_unref(udevMonitor);
    udev_unref(udev);
}

std::vector<VideoPipelineInput> V4L2UdevMonitor::getV4l2DeviceList() {
    return v4l2DeviceList;
}

bool populateV4l2DevInfos(VideoPipelineInput& input, const char* v4l2Name) {
    std::string devPath = "/dev/";
    devPath.append(v4l2Name);
    
    int fd = open(devPath.c_str(), O_RDWR);
    if(fd != -1) {
        if(ioctl(fd, VIDIOC_QUERYCAP, &input.devcap) == 0) {
            struct v4l2_fmtdesc fmt;
            struct v4l2_frmsizeenum frmsize;
            struct v4l2_frmivalenum frmival;
            fmt.index = 0;
            fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            while(ioctl(fd, VIDIOC_ENUM_FMT, &fmt) == 0) {
                frmsize.pixel_format = fmt.pixelformat;
                frmsize.index = 0;
                while(ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0) {
                    if(frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                        frmival.index = 0;
                        frmival.pixel_format = fmt.pixelformat;
                        frmival.width = frmsize.discrete.width;
                        frmival.height = frmsize.discrete.height;
                        struct VideoFormat format;
                        format.width = frmival.width;
                        format.height = frmival.height;
                        format.pixelformat = frmival.pixel_format;
                        while (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0) {
                            if(frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
                                FrameInterval interval;
                                interval.numerator = frmival.discrete.numerator;
                                interval.denominator = frmival.discrete.denominator;
                                format.frameIntervals.push_back(interval);
                            }
                            frmival.index++;
                        }
                        input.videoFormats.push_back(format);
                    }
                    frmsize.index++;
                }
                fmt.index++;
            }
        }
        close(fd);
    }
    
    return input.videoFormats.size() > 0;
}

std::vector<VideoPipelineInput> V4L2UdevMonitor::probeDevices() {
    std::vector<VideoPipelineInput> deviceList;
    struct udev_list_entry* dev_list_entry = nullptr;
    struct udev_device* dev = nullptr;
    
    struct udev_enumerate* enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "video4linux");
    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    
    udev_list_entry_foreach(dev_list_entry, devices) {
        const char* path = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(udev, path);
        const char* v4l2name = udev_device_get_sysname(dev);
        const char* devpath = udev_device_get_devpath(dev);
        VideoPipelineInput newInput;
        newInput.devpath = "/sys";
        newInput.devpath.append(devpath);
        newInput.devname = v4l2name;
        newInput.type = VideoPipelineInputType::V4L2_CAMERA;
        if(populateV4l2DevInfos(newInput, v4l2name)) {
            deviceList.push_back(newInput);
        }
        udev_device_unref(dev);
    }
    udev_enumerate_unref(enumerate);
    
    return deviceList;
}

bool V4L2UdevMonitor::updateV4L2DeviceList() {
    bool hasChanges = false;
    struct udev_device* dev = nullptr;
    
    while((dev = udev_monitor_receive_device(udevMonitor))) {
        hasChanges = true;
        std::cout << "V4L2 UDEV event"
        << " ACTION=" << udev_device_get_action(dev)
        << " DEVNAME=" << udev_device_get_sysname(dev)
        << " DEVPATH=" << "/sys" << udev_device_get_devpath(dev)
        << std::endl;
        
        udev_device_unref(dev);
        dev = nullptr;
    }
    
    if(hasChanges) {
        v4l2DeviceList = probeDevices();
    }
    
    return hasChanges;
}
