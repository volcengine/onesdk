# 如果存在ESP_IDF 变量，则将realtime_audio_example.c添加到编译列表中
#define ESP_ADF
set(ONESDK_ESP32_SRCS "station_example_main.c" "sntp_example_main.c")


if(ONESDK_ENABLE_AI_REALTIME)
    list(APPEND ONESDK_ESP32_SRCS "realtime_main.c")
else()
    list(APPEND ONESDK_ESP32_SRCS "chat.c")
endif()

message(STATUS "onesdk_esp32 srcs: ${ONESDK_ESP32_SRCS}")
set(ONESDK_ESP32_DEPS onesdk)
message(STATUS "onesdk_esp32 deps: ${ONESDK_ESP32_DEPS}")
idf_component_register(SRCS  "${ONESDK_ESP32_SRCS}"
                    PRIV_REQUIRES ${ONESDK_ESP32_DEPS}
                    # INCLUDE_DIRS "../libwebsockets/include;${IDF_PATH}/components/spi_flash/include;${IDF_PATH}/components/nvs_flash/include")
                    INCLUDE_DIRS ".;${IDF_PATH}/components/spi_flash/include;${IDF_PATH}/components/nvs_flash/include")

# target_link_libraries(${COMPONENT_LIB} PRIVATE onesdk)
set(ONESDK_ESP32_INCLUDES
    ${PROJECT_BINARY_DIR}/../libwebsockets/include
    ${PROJECT_BINARY_DIR}/onesdk/include
    ../onesdk/src/
    ../onesdk/external_libs/cJson)
include_directories(${ONESDK_ESP32_INCLUDES})
message(STATUS "onesdk_esp32 includes: ${ONESDK_ESP32_INCLUDES}")