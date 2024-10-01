add_executable (supervisor supervisor.cpp)

target_link_libraries(supervisor ${LIBSYSTEMD_LIBRARIES})
