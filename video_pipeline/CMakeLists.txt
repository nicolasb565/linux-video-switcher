pkg_check_modules(GSTLIBS REQUIRED
    glib-2.0
    gobject-2.0
    gstreamer-1.0)
pkg_check_modules(LIBUDEV REQUIRED libudev)

add_executable(video_pipeline main.cpp video_pipeline.cpp v4l2-udev.cpp)

target_compile_options(video_pipeline PRIVATE ${GSTLIBS_CFLAGS} ${LIBUDEV_CFLAGS})
target_link_libraries(video_pipeline ${GSTLIBS_LIBRARIES} ${LIBSYSTEMD_LIBRARIES} ${LIBUDEV_LIBRARIES})
