project(ws-client C)
cmake_minimum_required(VERSION 3.10)
# add_subdirectory(../../ build)
message(PROJECT_BINARY_DIR ${PROJECT_BINARY_DIR})
include_directories(${PROJECT_BINARY_DIR}/build/include)
include_directories(${PROJECT_BINARY_DIR}/build/external_libs/libwebsockets/include)
include_directories("/opt/homebrew/include") # openssl

set(SAMP ws-client)

add_executable(${SAMP} ws_client.c)
target_link_libraries(${PROJECT_NAME} PRIVATE onesdk_shared)
