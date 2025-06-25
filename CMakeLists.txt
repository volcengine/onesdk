# CMake 最低版本号要求
cmake_minimum_required(VERSION 3.6)

project(onesdk VERSION 0.1)

set(CMAKE_C_STANDARD 11)

# ### ONESDK OPTIONS  ###
# 是否使能AI功能
option(ONESDK_ENABLE_AI "Enable AI functionality" ON)
# 是否使能实时AI功能
option(ONESDK_ENABLE_AI_REALTIME "Enable real-time AI functionality" ON)
# 是否使能物联网功能
option(ONESDK_ENABLE_IOT "Enable IoT functionality" ON)

# 定义编译选项
option(ONESDK_WITH_EXAMPLE "Build onesdk examples" ON)
option(ONESDK_WITH_TEST "Build onesdk unit tests" ON)
option(ONESDK_WITH_SHARED "Build shared lib for onesdk" ON)
option(ONESDK_WITH_STATIC "Build static lib for onesdk" ON)
option(ONESDK_WITH_STRICT_MODE "Build onesdk with ADDRESS SANTITIZE mode" OFF)
option(ONESDK_WITH_EXAMPLE_EXTRA "Build extra examples, for internal use only" OFF)
# set lws 相关配置

# Generate the onesdk_config.h that includes all the public compilation settings.
configure_file(
       "${PROJECT_SOURCE_DIR}/cmake/onesdk_config.h.in"
       "${PROJECT_BINARY_DIR}/include/onesdk_config.h")


option(LWS_WITH_TLS "Build with TLS support" ON)
# 添加一个选项来控制是否抽取源文件
option(ONESDK_EXTRACT_SRC "Extract source files to output/onesdk" OFF)

if(ONESDK_EXTRACT_SRC)
    # 定义输出目录 作为components输出
    set(EXTRACT_OUTPUT_COMPONENTS_DIR "${PROJECT_BINARY_DIR}/output/components/onesdk")

    # 复制 include 目录
    file(COPY "${PROJECT_SOURCE_DIR}/include" DESTINATION "${EXTRACT_OUTPUT_COMPONENTS_DIR}")

    # 复制 platform 目录（如果存在）
    if(EXISTS "${PROJECT_SOURCE_DIR}/platform")
        file(COPY "${PROJECT_SOURCE_DIR}/platform" DESTINATION "${EXTRACT_OUTPUT_COMPONENTS_DIR}")
    endif()

    # 复制 src 目录
    file(COPY "${PROJECT_SOURCE_DIR}/src" DESTINATION "${EXTRACT_OUTPUT_COMPONENTS_DIR}")

    # 复制 external_libs 目录 only cJson
    file(COPY "${PROJECT_SOURCE_DIR}/external_libs/cJson" DESTINATION "${EXTRACT_OUTPUT_COMPONENTS_DIR}/external_libs")

	# 复制cmake目录
    file(COPY "${PROJECT_SOURCE_DIR}/cmake" DESTINATION "${EXTRACT_OUTPUT_COMPONENTS_DIR}")
    # 定义输出目录 作为onesdk依赖的libwebsockets，先于onesdk构建
    set(EXTRACT_OUTPUT_LIBWEBSOCKETS_DIR "${PROJECT_BINARY_DIR}/output/")
    # 复制 lbwebsockets 目录
    file(COPY "${PROJECT_SOURCE_DIR}/external_libs/libwebsockets" DESTINATION "${EXTRACT_OUTPUT_LIBWEBSOCKETS_DIR}/")

endif()
# ### ONESDK OPTIONS end ###

# 编译工具链
set(COMPILE_TOOLS "gcc")
set(BUILD_PLATFORM "linux")
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(BUILD_PLATFORM "macos")
endif ()


set(EXTERNAL_LIB_DIR "${PROJECT_SOURCE_DIR}/external_libs" CACHE INTERNAL "external libraries root.")

message(STATUS "PROJECT SRC DIR: ${PROJECT_SOURCE_DIR}")
message(STATUS "PROJECT BIN DIR: ${PROJECT_BINARY_DIR}")
# 将 include 和 external_libs/include 添加到头文件搜索路径
include_directories(
		${PROJECT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/include/iot
		${PROJECT_SOURCE_DIR}/src
		${PROJECT_SOURCE_DIR}/platform
        ${PROJECT_SOURCE_DIR}/external_libs/libwebsockets/include
        ${PROJECT_SOURCE_DIR}/external_libs/cJson
        ${PROJECT_BINARY_DIR}/external_libs/libwebsockets
		${PROJECT_BINARY_DIR}/include
)
if (ONESDK_ENABLE_IOT)
	option(LWS_ROLE_MQTT "Build with support for MQTT client" ON)
	set(LWS_ROLE_MQTT ON)
endif()
if (ONESDK_WITH_EXAMPLE)
	set(BUILD_TYPE "debug")
endif()
add_subdirectory(external_libs)

# 在 macOS 平台添加 CoreFoundation 框架
if(BUILD_PLATFORM STREQUAL "macos")
	set(COREFOUNDATION_FRAMEWORK "-framework CoreFoundation")
else()
	set(COREFOUNDATION_FRAMEWORK "")
endif()

# 编译类型: release/debug
set(BUILD_TYPE "debug")

file(GLOB SOURCE "${PROJECT_SOURCE_DIR}/src/*.c")

# source 目录
# aux_source_directory(src ONESDK_SRC_DIRS)

# file(GLOB_RECURSE ONESDK_SRCS "src/*.c")

file(GLOB_RECURSE AWS_SRCS "src/aws/*.c")
file(GLOB_RECURSE PLATFORM_SRCS "platform/plat/*.c")
file(GLOB_RECURSE IOT_SRCS "src/iot/*/*.c")
file(GLOB_RECURSE UTIL_SRCS "src/util/*.c")

set(ONESDK_SRCS
        src/protocols/http_libs.c
        src/protocols/http.c
        src/protocols/lws_http_client.c
		src/protocols/lws_http_download.c
		src/protocols/lws_http_upload.c
		src/iot/dynreg.c
		src/infer_realtime_ws.c
		src/mcp_client.c
		src/aigw/auth.c
		src/aigw/llm.c
		src/infer_inner_chat.c
		src/onesdk.c
		src/iot_basic.c
		src/onesdk_chat.c
		src/onesdk_rt.c
		${AWS_SRCS}
		${PLATFORM_SRCS}
		${UTIL_SRCS}
)

if (ONESDK_ENABLE_IOT)
	list(APPEND ONESDK_SRCS
			src/iot/iot_mqtt.c
			src/iot/iot_utils.c
			src/iot/iot_kv.c
			src/iot/iot_popen.c
			src/onesdk_iot.c
			${IOT_SRCS}
	)
endif()

# 链接 libwebsockets 库
if (ONESDK_WITH_SHARED)
    add_library(onesdk_shared SHARED ${ONESDK_SRCS})
    target_link_libraries(onesdk_shared PRIVATE
		websockets_shared
		cjson
		${COREFOUNDATION_FRAMEWORK}
	)
endif()
if (ONESDK_WITH_STATIC)
    add_library(onesdk STATIC ${ONESDK_SRCS})
    target_link_libraries(onesdk PRIVATE
        websockets
        ${LIBWEBSOCKETS_DEP_LIBS}
        ${COREFOUNDATION_FRAMEWORK}
    )
endif()

## compiler options ##
include(CheckCCompilerFlag)

# 检查编译器是否支持 -Wincompatible-pointer-types 选项
check_c_compiler_flag(-Wincompatible-pointer-types HAS_INCOMPATIBLE_POINTER_TYPES)
if (HAS_INCOMPATIBLE_POINTER_TYPES)
    # 如果支持，将其作为错误处理
    if (ONESDK_WITH_STATIC)
		target_compile_options(${PROJECT_NAME} PRIVATE -Werror=incompatible-pointer-types)
	endif()
	if (ONESDK_WITH_SHARED)
		target_compile_options(${PROJECT_NAME}_shared PRIVATE -Werror=incompatible-pointer-types)
	endif()
endif()

check_c_compiler_flag(-Wimplicit-function-declaration HAS_IMPLICIT_FUNCTION_DECLARATION)
if (HAS_IMPLICIT_FUNCTION_DECLARATION)
	if (ONESDK_WITH_STATIC)
    	target_compile_options(${PROJECT_NAME} PRIVATE -Wimplicit-function-declaration)
	endif()
	if (ONESDK_WITH_SHARED)
		target_compile_options(${PROJECT_NAME}_shared PRIVATE -Wimplicit-function-declaration)
	endif()
endif()
## compiler options ##

if (APPLE)
	# 根据intel还是arm架构判断
	message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
	if (CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
		include_directories("/opt/homebrew/include")
	else()
		include_directories("/usr/local/include")
	endif()
	find_package(Threads REQUIRED)
	# Don't add the exact path to CoreFoundation as this would hardcode the SDK version
	list(APPEND PLATFORM_LIBS dl Threads::Threads "-framework CoreFoundation")
	list(APPEND PLATFORM_LIBS ${CMAKE_DL_LIBS})

	target_link_libraries(${PROJECT_NAME} PUBLIC ${PLATFORM_LIBS})
	target_link_libraries(${PROJECT_NAME}_shared PUBLIC ${PLATFORM_LIBS})

	# list (APPEND AWS_COMMON_OS_SRC "source/darwin/*.c") # OS specific includes
	# list (APPEND AWS_COMMON_OS_SRC "source/platform_fallback_stubs/system_info.c")
endif()

# 输出include所有文件，供依赖项目引用
file(COPY ${PROJECT_SOURCE_DIR}/include DESTINATION ${PROJECT_BINARY_DIR})


if (ONESDK_WITH_EXAMPLE)
	if (ONESDK_WITH_STRICT_MODE)
		message(STATUS "BUILD WITH ADDRESS SANTITIZE")
		set(BUILD_TYPE "debug")
		set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
		set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
		set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	endif()
	if (ONESDK_ENABLE_AI)
		add_subdirectory("examples/chat/chatbot")
		add_subdirectory("examples/chat/text_image")
		add_subdirectory("examples/chat/function_call")
		add_subdirectory("examples/chat/mcp")
	endif() # ONESDK_ENABLE_AI
	if (ONESDK_ENABLE_AI_REALTIME)
		# add_subdirectory("examples/ws-client")
		add_subdirectory("examples/onesdk_realtime")
	endif() # ONESDK_ENABLE_AI_REALTIME
	if (ONESDK_ENABLE_IOT)
		add_subdirectory("examples/onesdk_iot")
	endif() # ONESDK_ENABLE_IOT
endif() # ONESDK_WITH_EXAMPLE

if (ONESDK_WITH_EXAMPLE_EXTRA)
	# add_subdirectory("examples/iot_dynamic_register")
	add_subdirectory("examples/aigw_auth_client")
	add_subdirectory("examples/raw_http_chat")
	add_subdirectory("examples/http")
if(ONESDK_ENABLE_IOT)
	add_subdirectory("examples/iot_mqtt")
endif()

endif()

if (ONESDK_WITH_TEST AND NOT APPLE)
    enable_language(CXX)
    set(BUILD_TYPE "debug")

	# build CppUTest
	include(FetchContent)
	FetchContent_Declare(
		CppUTest
		GIT_REPOSITORY https://github.com/cpptest/cpptest.git
		GIT_TAG        master # or use release tag, eg. v4.0
		# FIND_PACKAGE_ARGS # Recent CMake versions can integrate FetchContent with find_package
		# https://cmake.org/cmake/help/latest/module/FetchContent.html#integrating-with-find-package
	)
	FetchContent_MakeAvailable(CppUTest)
	# 添加头文件包含路径
	include_directories(${CppUTest_SOURCE_DIR}/include)

	# 添加编译选项
	add_compile_options(
    	"$<$<COMPILE_LANGUAGE:CXX>:-include${CppUTest_SOURCE_DIR}/include/CppUTest/MemoryLeakDetectorNewMacros.h>"
    	"$<$<COMPILE_LANGUAGE:C>:-include${CppUTest_SOURCE_DIR}/include/CppUTest/MemoryLeakDetectorMallocMacros.h>"
	)

	set(CPPUTEST_LDFLAGS
		CppUTest::CppUTest
		CppUTest::CppUTestExt  # 添加Mock支持
	)

	add_subdirectory(tests)
    if (ONESDK_WITH_SHARED)
	    # 覆盖率
	    # 添加覆盖率编译选项（在 add_executable 后添加）
	    if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
		    target_compile_options(onesdk_shared PUBLIC
			    -fprofile-arcs -ftest-coverage
		    )
		    set(COVERATE_LDFLAGS --coverage)
	    endif()
        target_link_libraries(onesdk_shared PUBLIC
			"${CPPUTEST_LDFLAGS}"
			"${COVERATE_LDFLAGS}"
			)
	    set(COVERATE_LDFLAGS "--coverage")
	endif()
	if (ONESDK_WITH_STATIC)
	    # 覆盖率
	    # 添加覆盖率编译选项（在 add_executable 后添加）
	    if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
		    target_compile_options(onesdk PUBLIC
			    -fprofile-arcs -ftest-coverage
		    )
		    set(COVERATE_LDFLAGS "--coverage")
	    endif()
        target_link_libraries(onesdk PUBLIC
			"${CPPUTEST_LDFLAGS}"
			"${COVERATE_LDFLAGS}"
			)
	endif()
endif()