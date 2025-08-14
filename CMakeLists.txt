# CMake minimum version requirement
# Using 3.10 for cJSON compatibility - avoid CMake 4.x which may cause issues
cmake_minimum_required(VERSION 3.10)

project(onesdk VERSION 0.1)

set(CMAKE_C_STANDARD 11)

# Add UTF-8 support for MSVC to handle libwebsockets encoding issues
if(MSVC)
    add_compile_options(/utf-8)
    add_compile_options(/wd4819)  # Suppress encoding warnings
endif()

# ### ONESDK OPTIONS  ###
# Enable AI functionality
option(ONESDK_ENABLE_AI "Enable AI functionality" ON)
# Enable real-time AI functionality
option(ONESDK_ENABLE_AI_REALTIME "Enable real-time AI functionality" ON)
# Enable IoT functionality
option(ONESDK_ENABLE_IOT "Enable IoT functionality" ON)

# Define compilation options
option(ONESDK_WITH_EXAMPLE "Build onesdk examples" ON)
option(ONESDK_WITH_TEST "Build onesdk unit tests" OFF)
option(ONESDK_WITH_SHARED "Build shared lib for onesdk" ON)
option(ONESDK_WITH_STATIC "Build static lib for onesdk" ON)
option(ONESDK_WITH_STRICT_MODE "Build onesdk with ADDRESS SANTITIZE mode" OFF)
option(ONESDK_WITH_EXAMPLE_EXTRA "Build extra examples, for internal use only" OFF)

# Generate the onesdk_config.h that includes all the public compilation settings.
configure_file(
       "${PROJECT_SOURCE_DIR}/cmake/onesdk_config.h.in"
       "${PROJECT_BINARY_DIR}/include/onesdk_config.h")

# Set libwebsockets related configuration

option(LWS_WITH_TLS "Build with TLS support" ON)
option(LWS_WITH_MINIMAL_EXAMPLES "Build with examples" OFF)
if(ONESDK_ENABLE_IOT)
	option(LWS_ROLE_MQTT "Build with support for MQTT client" ON)
	set(LWS_ROLE_MQTT ON)
endif()
# Add an option to control whether to extract source files
option(ONESDK_EXTRACT_SRC "Extract source files to output/onesdk" OFF)

if(ONESDK_EXTRACT_SRC)
    # Define output directory as components output
    set(EXTRACT_OUTPUT_COMPONENTS_DIR "${PROJECT_BINARY_DIR}/output/components/onesdk")

    # Copy include directory
    file(COPY "${PROJECT_SOURCE_DIR}/include" DESTINATION "${EXTRACT_OUTPUT_COMPONENTS_DIR}")

    # Copy platform directory (if exists)
    if(EXISTS "${PROJECT_SOURCE_DIR}/platform")
        file(COPY "${PROJECT_SOURCE_DIR}/platform" DESTINATION "${EXTRACT_OUTPUT_COMPONENTS_DIR}")
    endif()

    # Copy src directory
    file(COPY "${PROJECT_SOURCE_DIR}/src" DESTINATION "${EXTRACT_OUTPUT_COMPONENTS_DIR}")

    # Copy external_libs directory only cJson
    file(COPY "${PROJECT_SOURCE_DIR}/external_libs/cJson" DESTINATION "${EXTRACT_OUTPUT_COMPONENTS_DIR}/external_libs")

	# Copy cmake directory
    file(COPY "${PROJECT_SOURCE_DIR}/cmake" DESTINATION "${EXTRACT_OUTPUT_COMPONENTS_DIR}")
    # Define output directory as onesdk dependent libwebsockets, build before onesdk
    set(EXTRACT_OUTPUT_LIBWEBSOCKETS_DIR "${PROJECT_BINARY_DIR}/output/")
    # Copy libwebsockets directory
    # file(COPY "${PROJECT_SOURCE_DIR}/external_libs/libwebsockets" DESTINATION "${EXTRACT_OUTPUT_LIBWEBSOCKETS_DIR}/")

    # Apply libwebsockets patch
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E echo "Patching libwebsockets..."
    )
    execute_process(
        COMMAND patch -d ${PROJECT_SOURCE_DIR}/external_libs/libwebsockets -p1 -N -i ${PROJECT_SOURCE_DIR}/libwebsockets-v4.3.patch
        RESULT_VARIABLE PATCH_RESULT
    )
    if(NOT PATCH_RESULT EQUAL 0)
        message(WARNING "libwebsockets patch may have already been applied or failed to apply.")
    endif()
endif()
# ### ONESDK OPTIONS end ###

# Compilation toolchain
set(COMPILE_TOOLS "gcc")
set(BUILD_PLATFORM "linux")

# Platform detection and configuration
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(BUILD_PLATFORM "macos")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(BUILD_PLATFORM "windows")
    # Windows specific settings
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    # Use MSVC compiler flags
    if(MSVC)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
    endif()
endif()

set(EXTERNAL_LIB_DIR "${PROJECT_SOURCE_DIR}/external_libs" CACHE INTERNAL "external libraries root.")

message(STATUS "PROJECT SRC DIR: ${PROJECT_SOURCE_DIR}")
message(STATUS "PROJECT BIN DIR: ${PROJECT_BINARY_DIR}")
message(STATUS "BUILD PLATFORM: ${BUILD_PLATFORM}")

# Add include and external_libs/include to header file search path
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

# Platform specific include directories
if(BUILD_PLATFORM STREQUAL "windows")
    # Add Windows SDK paths if available
    if(DEFINED ENV{WINDOWS_SDK_PATH})
        include_directories("$ENV{WINDOWS_SDK_PATH}/Include")
    endif()
    # Add OpenSSL paths for Windows
	message(STATUS "OPENSSL_ROOT_DIR for onesdk: $ENV{OPENSSL_ROOT_DIR}")
	# include_directories("C:/Program Files/OpenSSL-Win64/include")
    if(DEFINED ENV{OPENSSL_ROOT_DIR})
        include_directories("$ENV{OPENSSL_ROOT_DIR}/include")
    endif()
elseif(BUILD_PLATFORM STREQUAL "macos")
    # Add OpenSSL paths for macOS
    if(DEFINED ENV{OPENSSL_ROOT_DIR})
        include_directories("$ENV{OPENSSL_ROOT_DIR}/include")
    endif()
endif()

if (ONESDK_ENABLE_IOT)
	option(LWS_ROLE_MQTT "Build with support for MQTT client" ON)
	set(LWS_ROLE_MQTT ON)
endif()
if (ONESDK_WITH_EXAMPLE)
	set(BUILD_TYPE "debug")
endif()
add_subdirectory(external_libs)

# Add CoreFoundation framework on macOS platform
if(BUILD_PLATFORM STREQUAL "macos")
	set(COREFOUNDATION_FRAMEWORK "-framework CoreFoundation")
else()
	set(COREFOUNDATION_FRAMEWORK "")
endif()

# Windows specific libraries
if(BUILD_PLATFORM STREQUAL "windows")
    set(WINDOWS_LIBS ws2_32 crypt32 iphlpapi)
else()
    set(WINDOWS_LIBS "")
endif()

# Build type: release/debug
set(BUILD_TYPE "debug")

file(GLOB SOURCE "${PROJECT_SOURCE_DIR}/src/*.c")

# source directory
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

# Link libwebsockets library
if (ONESDK_WITH_SHARED)
    add_library(onesdk_shared SHARED ${ONESDK_SRCS})
    target_link_libraries(onesdk_shared PRIVATE
		websockets_shared
		cjson
		${COREFOUNDATION_FRAMEWORK}
		${WINDOWS_LIBS}
	)
endif()
if (ONESDK_WITH_STATIC)
    add_library(onesdk STATIC ${ONESDK_SRCS})
    target_link_libraries(onesdk PRIVATE
        websockets
        ${LIBWEBSOCKETS_DEP_LIBS}
        ${COREFOUNDATION_FRAMEWORK}
        ${WINDOWS_LIBS}
    )
endif()

## compiler options ##
include(CheckCCompilerFlag)

# Platform-specific compiler options
if(BUILD_PLATFORM STREQUAL "windows")
    # Windows specific compiler options
    if(MSVC)
        # MSVC specific warnings and options
        target_compile_options(onesdk_shared PRIVATE /W3 /D_CRT_SECURE_NO_WARNINGS)
        target_compile_options(onesdk PRIVATE /W3 /D_CRT_SECURE_NO_WARNINGS)
    else()
        # MinGW specific options
        target_compile_options(onesdk_shared PRIVATE -Wall -Wextra)
        target_compile_options(onesdk PRIVATE -Wall -Wextra)
    endif()
else()
    # Unix-like platforms (Linux/macOS)
    # Check if compiler supports -Wincompatible-pointer-types option
    check_c_compiler_flag(-Wincompatible-pointer-types HAS_INCOMPATIBLE_POINTER_TYPES)
    if (HAS_INCOMPATIBLE_POINTER_TYPES)
        # If supported, treat it as an error
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
endif()
## compiler options ##

if (APPLE)
	# Determine based on intel or arm architecture
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
elseif(BUILD_PLATFORM STREQUAL "windows")
    # Windows platform specific libraries
    find_package(Threads REQUIRED)
    list(APPEND PLATFORM_LIBS Threads::Threads)
    
    target_link_libraries(${PROJECT_NAME} PUBLIC ${PLATFORM_LIBS})
    target_link_libraries(${PROJECT_NAME}_shared PUBLIC ${PLATFORM_LIBS})
else()
    # Linux platform
    find_package(Threads REQUIRED)
    list(APPEND PLATFORM_LIBS dl Threads::Threads)
    list(APPEND PLATFORM_LIBS ${CMAKE_DL_LIBS})
    
    target_link_libraries(${PROJECT_NAME} PUBLIC ${PLATFORM_LIBS})
    target_link_libraries(${PROJECT_NAME}_shared PUBLIC ${PLATFORM_LIBS})
endif()

# Output include files for dependent projects to reference
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
	add_subdirectory("examples/iot_dynamic_register")
	# add_subdirectory("examples/aigw_auth_client")
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
		GIT_REPOSITORY https://github.com/cpputest/cpputest.git
		GIT_TAG        master # or use release tag, eg. v4.0
		# FIND_PACKAGE_ARGS # Recent CMake versions can integrate FetchContent with find_package
		# https://cmake.org/cmake/help/latest/module/FetchContent.html#integrating-with-find_package
	)
	FetchContent_MakeAvailable(CppUTest)
	# Add header file include path
	include_directories(${CppUTest_SOURCE_DIR}/include)

	# Add compilation options
	add_compile_options(
    	"$<$<COMPILE_LANGUAGE:CXX>:-include${CppUTest_SOURCE_DIR}/include/CppUTest/MemoryLeakDetectorNewMacros.h>"
    	"$<$<COMPILE_LANGUAGE:C>:-include${CppUTest_SOURCE_DIR}/include/CppUTest/MemoryLeakDetectorMallocMacros.h>"
	)

	set(CPPUTEST_LDFLAGS
		CppUTest::CppUTest
		CppUTest::CppUTestExt  # Add Mock support
	)

	add_subdirectory(tests)
    if (ONESDK_WITH_SHARED)
	    # Coverage
	    # Add coverage compilation options (add after add_executable)
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
	    # Coverage
	    # Add coverage compilation options (add after add_executable)
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