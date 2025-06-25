cmake_minimum_required(VERSION 3.5)
project (onesdk_tests)

enable_language(C)
enable_language(CXX)

# include project source
include_directories(${PROJECT_SOURCE_DIR}/../include)
include_directories(${PROJECT_SOURCE_DIR}/../src)

# include mocks only for test code
include_directories(${PROJECT_SOURCE_DIR}/mocks)

include(FetchContent)
FetchContent_Declare(
	CppUTest
	GIT_REPOSITORY git@code.byted.org:byteedge-iec/cpputest.git
	GIT_TAG        master # or use release tag, eg. v4.0
	# FIND_PACKAGE_ARGS # Recent CMake versions can integrate FetchContent with find_package
	# https://cmake.org/cmake/help/latest/module/FetchContent.html#integrating-with-find-package
)
FetchContent_MakeAvailable(CppUTest)

include_directories(${CppUTest_SOURCE_DIR}/include)

# 添加编译选项
add_compile_options(
    "$<$<COMPILE_LANGUAGE:CXX>:-include${CppUTest_SOURCE_DIR}/include/CppUTest/MemoryLeakDetectorNewMacros.h>"
    "$<$<COMPILE_LANGUAGE:C>:-include${CppUTest_SOURCE_DIR}/include/CppUTest/MemoryLeakDetectorMallocMacros.h>"
)

# build test library for auth
add_library(auth_test auth/auth_test.cpp)
add_library(util_test util/util_test.cpp)
add_library(iot_basic_test iot_basic/iot_basic_test.cpp)
add_library(llm_config_test llm_config/llm_config_test.cpp)
add_library(dynreg_test dynreg/dynreg_test.cpp)
add_library(onesdk_rt_test onesdk_rt/onesdk_rt_test.cpp)
add_library(plat_test plat/plat_hardware_id_test.cpp)

add_executable(run_all_tests run_all_tests.cpp)

target_link_libraries(run_all_tests PRIVATE
    CppUTest::CppUTest      # 主测试框架
    CppUTest::CppUTestExt   # 添加Mock扩展支持
    auth_test
    util_test
    iot_basic_test
    llm_config_test
    dynreg_test
    plat_test
	onesdk_rt_test
    onesdk_shared
    websockets_shared
	cjson
    --coverage
)

# 添加覆盖率编译选项（在 add_executable 后添加）
if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
    target_compile_options(run_all_tests PRIVATE
        -fprofile-arcs -ftest-coverage
    )
endif()

# 在文件末尾添加覆盖率目标
find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)

if(LCOV_PATH AND GENHTML_PATH)
    add_custom_target(coverage
        # 清理计数器时增加递归
        COMMAND ${LCOV_PATH} --directory ${CMAKE_BINARY_DIR} --zerocounters

        # 添加测试执行错误处理
        COMMAND $<TARGET_FILE:run_all_tests> -o junit

        # 修正捕获路径并添加基准目录
        COMMAND ${LCOV_PATH} --directory ${CMAKE_BINARY_DIR} --capture
            --output-file coverage.info
            --base-directory ${CMAKE_SOURCE_DIR}/..

        # 调整过滤规则（确保包含项目源码路径）
        COMMAND ${LCOV_PATH} --remove coverage.info
            '*/tests/*'
            '*/mocks/*'
            '*/usr/include/*'
            '*/third_party/*'
            '*/external_libs/*'
            '*/examples/*'
            --output-file coverage_filtered.info

        # 添加分支覆盖率分析
        COMMAND ${GENHTML_PATH} coverage_filtered.info
            --output-directory coverage_report
            --branch-coverage

        # coverage.out
        COMMAND cp coverage_filtered.info coverage.info

        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating code coverage report"
        DEPENDS run_all_tests  # 添加依赖确保构建完成
    )
endif()