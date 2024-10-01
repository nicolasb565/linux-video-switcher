cmake_minimum_required(VERSION 3.24.0)

project(linux-video-switcher LANGUAGES CXX)

set(CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSYSTEMD REQUIRED libsystemd)

add_subdirectory(supervisor)
add_subdirectory(video_pipeline)
