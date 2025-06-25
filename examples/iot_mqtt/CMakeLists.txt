project(iot_mqtt C)
cmake_minimum_required(VERSION 3.10)

include_directories(
    ${PROJECT_BINARY_DIR}/build/include
    ${PROJECT_BINARY_DIR}/build/external_libs/libwebsockets/include
    /opt/homebrew/include
)

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

add_executable(${PROJECT_NAME} iot_mqtt_client.c)
target_link_libraries(${PROJECT_NAME} PRIVATE 
    onesdk_shared
    websockets_shared
)
