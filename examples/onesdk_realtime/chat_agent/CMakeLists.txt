cmake_minimum_required(VERSION 3.5)

project(onesdk_realtime_chat_agent_example C)
# add_subdirectory(../../ build)

# if (requirements)
message("building ${PROJECT_NAME} with include dir: ${PROJECT_BINARY_DIR}/build/include")
include_directories(${PROJECT_BINARY_DIR}/build/include)
include_directories(${PROJECT_BINARY_DIR}/build/external_libs/libwebsockets/include)
include_directories(${PROJECT_SOURCE_DIR}/../../external_libs/cJson)
include_directories("/opt/homebrew/include") # openssl

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

add_executable(${PROJECT_NAME}
        main.c
)
message("linking ${PROJECT_NAME} with websockets_shared = ${websockets_shared} libs  ${LIBWEBSOCKETS_DEP_LIBS}")

if (websockets_static)
    target_link_libraries(${PROJECT_NAME}
            onesdk cjson pthread
            ${LIBWEBSOCKETS_DEP_LIBS})
else()
    target_link_libraries(${PROJECT_NAME}
            onesdk_shared cjson pthread
            ${LIBWEBSOCKETS_DEP_LIBS})
    add_dependencies(${PROJECT_NAME}
            websockets_shared)
endif()



