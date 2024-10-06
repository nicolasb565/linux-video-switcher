#include <iostream>

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
        const char* devname = udev_device_get_sysname(dev);
        const char* devpath = udev_device_get_devpath(dev);
        VideoPipelineInput newInput;
        newInput.name = devname;
        newInput.path = devpath;
        newInput.type = VideoPipelineInputType::V4L2_CAMERA;
        deviceList.push_back(newInput);
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
        << " DEVPATH=" << udev_device_get_devpath(dev)
        << std::endl;
        
        udev_device_unref(dev);
        dev = nullptr;
    }
    
    if(hasChanges) {
        v4l2DeviceList = probeDevices();
    }
    
    return hasChanges;
}
