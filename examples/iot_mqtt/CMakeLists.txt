project(iot_mqtt C)

# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(BUILD_PLATFORM "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(BUILD_PLATFORM "macos")
else()
    set(BUILD_PLATFORM "linux")
endif()

cmake_minimum_required(VERSION 3.10)

include_directories(
    ${PROJECT_BINARY_DIR}/build/include
    ${PROJECT_BINARY_DIR}/build/external_libs/libwebsockets/include
    /opt/homebrew/include
)

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

add_executable(${PROJECT_NAME} iot_mqtt_client.c)
# Platform-specific libraries
if(BUILD_PLATFORM STREQUAL "windows")
    set(PLATFORM_LIBS ws2_32 crypt32 iphlpapi)
else()
    set(PLATFORM_LIBS "")
endif()


target_link_libraries(${PROJECT_NAME} PRIVATE 
    onesdk_shared
    websockets_shared
)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PLATFORM_LIBS})