cmake_minimum_required(VERSION 3.5)


project(aigw_auth_client C)

# add_subdirectory(../../ build)
include_directories(${PROJECT_BINARY_DIR}/build/include)
include_directories(${PROJECT_BINARY_DIR}/build/external_libs/libwebsockets/include)
include_directories("/opt/homebrew/include") # openssl

add_executable(aigw_auth_client main.c)


target_link_libraries(aigw_auth_client PRIVATE onesdk_shared)
