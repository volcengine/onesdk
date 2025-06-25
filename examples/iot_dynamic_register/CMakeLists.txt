cmake_minimum_required(VERSION 3.10)


project(iot_dynamic_register C)

# add_subdirectory(../../ build)
include_directories(${PROJECT_BINARY_DIR}/build/include)
include_directories(${PROJECT_BINARY_DIR}/build/external_libs/libwebsockets/include)
include_directories("/opt/homebrew/include") # openssl

add_executable(iot_dynamic_register main.c)

target_link_libraries(iot_dynamic_register PRIVATE onesdk_shared)
