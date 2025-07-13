cmake_minimum_required(VERSION 3.10)

project(onesdk_realtime_translation_example C)

# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(BUILD_PLATFORM "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(BUILD_PLATFORM "macos")
else()
    set(BUILD_PLATFORM "linux")
endif()

# add_subdirectory(../../ build)

# if (requirements)
message("building ${PROJECT_NAME} with include dir: ${PROJECT_BINARY_DIR}/build/include")
include_directories(${PROJECT_BINARY_DIR}/build/include)
include_directories(${PROJECT_BINARY_DIR}/build/external_libs/libwebsockets/include)
include_directories(${PROJECT_SOURCE_DIR}/../../external_libs/cJson)
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

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

add_executable(${PROJECT_NAME}
        main.c
)
message("linking ${PROJECT_NAME} with websockets_shared = ${websockets_shared} libs  ${LIBWEBSOCKETS_DEP_LIBS}")

if (websockets_static)
# Platform-specific libraries
if(BUILD_PLATFORM STREQUAL "windows")
    set(PLATFORM_LIBS ws2_32 crypt32 iphlpapi)
else()
    set(PLATFORM_LIBS "")
endif()

    target_link_libraries(${PROJECT_NAME}
            onesdk cjson pthread
            ${LIBWEBSOCKETS_DEP_LIBS}
            ${PLATFORM_LIBS})
else()
    target_link_libraries(${PROJECT_NAME}
            onesdk_shared cjson pthread
            ${LIBWEBSOCKETS_DEP_LIBS}
            ${PLATFORM_LIBS})
    add_dependencies(${PROJECT_NAME}
            websockets_shared)
endif()



