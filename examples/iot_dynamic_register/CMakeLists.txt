cmake_minimum_required(VERSION 3.10)


project(iot_dynamic_register C)


# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(BUILD_PLATFORM "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(BUILD_PLATFORM "macos")
else()
    set(BUILD_PLATFORM "linux")
endif()

# add_subdirectory(../../ build)
include_directories(${PROJECT_BINARY_DIR}/build/include)
include_directories(${PROJECT_BINARY_DIR}/build/external_libs/libwebsockets/include)
# Platform-specific include directories
if(BUILD_PLATFORM STREQUAL "macos")
    include_directories("/opt/homebrew/include") # openssl
elseif(BUILD_PLATFORM STREQUAL "windows")
    # Windows specific include directories
    if(DEFINED ENV{OPENSSL_ROOT_DIR})
        include_directories("$ENV{OPENSSL_ROOT_DIR}/include")
    endif()
    if(DEFINED ENV{WINDOWS_SDK_PATH})
        include_directories("$ENV{WINDOWS_SDK_PATH}/Include")
    endif()
endif()

add_executable(iot_dynamic_register main.c)

# Platform-specific libraries
if(BUILD_PLATFORM STREQUAL "windows")
    set(PLATFORM_LIBS ws2_32 crypt32 iphlpapi)
else()
    set(PLATFORM_LIBS "")
endif()


target_link_libraries(iot_dynamic_register PRIVATE onesdk_shared)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PLATFORM_LIBS})