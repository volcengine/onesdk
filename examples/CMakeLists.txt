#
# Copyright 2022-2024 Beijing Volcano Engine Technology Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.10)
project(onesdkSamples C)

set(CMAKE_C_STANDARD 11)

# set(LOG_DIR "./log")
# configure_file("iot/test_prams.c.in" "${CMAKE_CURRENT_BINARY_DIR}/test_prams.c" @ONLY)

include_directories("include")
link_directories(lib)

IF (APPLE)
    IF (BUILD_SHARED_LIBS)
        set(IOT_LINKS
                libaws-c-common.1.0.0.dylib
                libaws-c-common.1.dylib
                libaws-c-common.dylib
                libonesdk_shared.dylib
                libwebsockets.dylib
                libcjson.dylib
        )
    ELSE ()
        set(IOT_LINKS
                libonesdk.a
        )
    ENDIF ()
ELSEIF (UNIX AND NOT APPLE)
    IF (BUILD_SHARED_LIBS)
        set(IOT_LINKS
                libaws-c-common.so.1.0.0
                libaws-c-common.so.1
                libaws-c-common.so
                libiot_sdk_shared.so
                libonesdk_shared.so
                libwebsockets.so
                libcjson.so
        )
    ELSE ()
        set(IOT_LINKS
                libonesdk.a
                "-lpthread -ldl -lm"
        )
    ENDIF ()
ENDIF ()
# set(TEST_COMMON_SOURCE
#         ${CMAKE_CURRENT_BINARY_DIR}/test_prams.c
#         iot/test_params.h
# )

#add_subdirectory(${PROJECT_SOURCE_DIR}/ws-client)
#add_subdirectory(${PROJECT_SOURCE_DIR}/onesdk_realtime)