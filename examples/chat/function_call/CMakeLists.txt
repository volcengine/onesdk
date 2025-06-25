cmake_minimum_required(VERSION 3.5)

project(onesdk_function_call_example C)


# if (requirements)
message("building ${PROJECT_NAME} with include dir: ${PROJECT_BINARY_DIR}/build/include")
include_directories(${PROJECT_BINARY_DIR}/build/include)
include_directories(${PROJECT_BINARY_DIR}/build/external_libs/libwebsockets/include)
include_directories(${PROJECT_SOURCE_DIR}/../../external_libs/cJson)

include_directories("/opt/homebrew/include") # openssl
add_executable(${PROJECT_NAME}
        main_function_call.c
)

message("linking ${PROJECT_NAME} with websockets_shared = ${websockets_shared} libs  ${LIBWEBSOCKETS_DEP_LIBS}")

if (websockets)
    target_link_libraries(${PROJECT_NAME}
            onesdk cjson
            ${LIBWEBSOCKETS_DEP_LIBS})
else()
    target_link_libraries(${PROJECT_NAME}
            onesdk_shared cjson
            ${LIBWEBSOCKETS_DEP_LIBS})
    add_dependencies(${PROJECT_NAME}
            websockets_shared)
endif()



