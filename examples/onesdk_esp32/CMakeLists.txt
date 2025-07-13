# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.10)
set(ONESDK_ENABLE_AI ON)
option(ONESDK_ENABLE_AI "Enable AI" ON)
set(ONESDK_ENABLE_AI_REALTIME OFF)
option(ONESDK_ENABLE_AI_REALTIME "Enable AI Realtime" OFF)
set(ONESDK_ENABLE_IOT OFF)
option(ONESDK_ENABLE_IOT "Enable IOT" OFF)
if (ESP_PLATFORM)



	if(NOT DEFINED ENV{ADF_PATH})
		message(WARNING "ADF_PATH not set. realtime audio will not be avaiable.")
	else()
		include($ENV{ADF_PATH}/CMakeLists.txt)
		message(STATUS "ADF_PATH: $ENV{ADF_PATH} found. IDF_PATH: $ENV{IDF_PATH} found.")
	endif()
	include($ENV{IDF_PATH}/tools/cmake/project.cmake)
	project(onesdk_esp32)
	
# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(BUILD_PLATFORM "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(BUILD_PLATFORM "macos")
else()
    set(BUILD_PLATFORM "linux")
endif()

enable_testing()

# Platform-specific libraries
if(BUILD_PLATFORM STREQUAL "windows")
    set(PLATFORM_LIBS ws2_32 crypt32 iphlpapi)
else()
    set(PLATFORM_LIBS "")
endif()


	# target_link_libraries(onesdk PRIVATE libwebsockets)
            ${PLATFORM_LIBS})
	target_link_libraries(onesdk_esp32.elf PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/esp-idf/onesdk/libonesdk.a")
            ${PLATFORM_LIBS})
	target_link_libraries(onesdk_esp32.elf PUBLIC websockets)
            ${PLATFORM_LIBS})
	# option(LWS_WITH_DRIVERS "With generic drivers for gpio, i2c, display etc" ON)
	set(LWS_WITH_MBEDTLS ON)
	option(LWS_WITH_MBEDTLS "With mbedtls" ON)
	set(LWS_WITH_DRIVERS OFF)
	option(LWS_WITH_SECURE_STREAMS "With secure streams" OFF)
	set(LWS_WITH_SECURE_STREAMS OFF)
	option(LWS_WITH_SECURE_STREAMS_STATIC_POLICY_ONLY "static ssp" OFF)
	set(LWS_WITH_SECURE_STREAMS_STATIC_POLICY_ONLY OFF)
	# option(LWS_WITH_LWSAC "With lwsac" ON)
	# set(LWS_WITH_LWSAC ON)
	# option(LWS_WITH_STRUCT_JSON "With lws_struct JSON" ON)
	# set(LWS_WITH_STRUCT_JSON ON)
	# option(LWS_WITH_SYS_NTPCLIENT "With ntpclient" ON)
    #     set(LWS_WITH_SYS_NTPCLIENT ON)
	option(LWS_WITH_HTTP2 "With HTTP/2" OFF)
	set(LWS_WITH_HTTP2 OFF)
	
	# if(NOT ONESDK_ENABLE_AI)
	# 	option(LWS_WITH_SSL "disable TLS/SSL" OFF)
	# 	set(LWS_WITH_SSL OFF)
	# 	set(LWS_WITH_MBEDTLS OFF)
	# 	option(LWS_WITH_MBEDTLS "With mbedtls" OFF)
	# endif()
	if(ONESDK_ENABLE_IOT)
		option(LWS_ROLE_MQTT "Build with support for MQTT client" ON)
		set(LWS_ROLE_MQTT ON)
	endif()
	add_subdirectory(libwebsockets)
	# add_subdirectory(onesdk)

	# add_test(NAME flashing COMMAND idf.py flash)
	# set_tests_properties(flashing PROPERTIES
	# 		     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	# 		     TIMEOUT 120)

	# add_test(NAME boot COMMAND /usr/local/bin/sai-expect)
	# set_tests_properties(boot PROPERTIES
	# 		     DEPENDS flashing
	# 		     TIMEOUT 20)

endif()