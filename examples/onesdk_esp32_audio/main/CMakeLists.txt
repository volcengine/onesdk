# set(COMPONENT_SRCS "sntp_example_main.c" "onesdk_main.c" "main.c" "volc_rtc.c" "byte_rtc_request.c")
set(COMPONENT_SRCS "onesdk_main.c" "main.c" "sntp_example_main.c" "audio_utils.c")
set(COMPONENT_ADD_INCLUDEDIRS .)

register_component()

spiffs_create_partition_image(spiffs_data ../spiffs FLASH_IN_PROJECT)
# Platform-specific libraries
if(BUILD_PLATFORM STREQUAL "windows")
    set(PLATFORM_LIBS ws2_32 crypt32 iphlpapi)
else()
    set(PLATFORM_LIBS "")
endif()


# target_link_libraries(${COMPONENT_LIB} PRIVATE onesdk)
            ${PLATFORM_LIBS})
set(ONESDK_ESP32_INCLUDES
    ${PROJECT_BINARY_DIR}/../libwebsockets/include
    ${PROJECT_BINARY_DIR}/onesdk/include
    # ../onesdk/src/
    # ../onesdk/external_libs/cJson
    )
include_directories(${ONESDK_ESP32_INCLUDES})
message(STATUS "onesdk_esp32 includes: ${ONESDK_ESP32_INCLUDES}")