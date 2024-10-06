#include <iostream>
#include <algorithm>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "v4l2-udev.h"

bool pixelformatIsCompressed(uint32_t pixelformat) {
    switch(pixelformat) {
    case V4L2_PIX_FMT_JPEG:
    case V4L2_PIX_FMT_MPEG:
    case V4L2_PIX_FMT_H264:
    case V4L2_PIX_FMT_H264_NO_SC:
    case V4L2_PIX_FMT_H264_MVC:
    case V4L2_PIX_FMT_H263:
    case V4L2_PIX_FMT_MPEG1:
    case V4L2_PIX_FMT_MPEG2:
    case V4L2_PIX_FMT_MPEG4:
    case V4L2_PIX_FMT_XVID:
    case V4L2_PIX_FMT_VC1_ANNEX_G:
    case V4L2_PIX_FMT_VC1_ANNEX_L:
    case V4L2_PIX_FMT_VP8:
    case V4L2_PIX_FMT_VP9:
        return true;
    default:
        break;
    }
    return false;
}

bool videoFormatsSortFunction(const VideoFormat& a, const VideoFormat& b) {
    int scoreA = 0;
    int scoreB = 0;
    uint64_t nbrPixelsA = a.width * a.height;
    uint64_t nbrPixelsB = b.width * b.height;
    
    if(nbrPixelsA > nbrPixelsB) {
        scoreA++;
    }
    else if(nbrPixelsB > nbrPixelsA) {
        scoreB++;
    }
    
    if(pixelformatIsCompressed(a.pixelformat)) {
        scoreA--;
    }
    if(pixelformatIsCompressed(b.pixelformat)) {
        scoreB--;
    }
    
    return scoreA > scoreB;
}

bool pixelFormatIsSupported(uint32_t pixelformat) {
    //we assume all uncompressed formats are supported
    //videoconvert should handle any issues
    
    //we do not support compressed formats other than jpeg/mjpg for now
    switch(pixelformat) {
    case V4L2_PIX_FMT_H264:
    case V4L2_PIX_FMT_H264_NO_SC:
    case V4L2_PIX_FMT_H264_MVC:
    case V4L2_PIX_FMT_H263:
    case V4L2_PIX_FMT_MPEG1:
    case V4L2_PIX_FMT_MPEG2:
    case V4L2_PIX_FMT_MPEG4:
    case V4L2_PIX_FMT_XVID:
    case V4L2_PIX_FMT_VC1_ANNEX_G:
    case V4L2_PIX_FMT_VC1_ANNEX_L:
    case V4L2_PIX_FMT_VP8:
    case V4L2_PIX_FMT_VP9:
        return false;
    default:
        break;
    }
    return true;
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
                if(!pixelFormatIsSupported(fmt.pixelformat)) {
                    continue;
                }
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
    
    std::sort(input.videoFormats.begin(), input.videoFormats.end(), videoFormatsSortFunction);
    
    return input.videoFormats.size() > 0;
}

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
