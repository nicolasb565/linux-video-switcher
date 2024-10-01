pkg_check_modules(GSTLIBS REQUIRED
    glib-2.0
    gobject-2.0
    gstreamer-1.0)

add_executable (video_pipeline main.cpp video_pipeline.cpp)

target_compile_options(video_pipeline PRIVATE ${GSTLIBS_CFLAGS})
target_link_libraries(video_pipeline ${GSTLIBS_LIBRARIES} ${LIBSYSTEMD_LIBRARIES})
