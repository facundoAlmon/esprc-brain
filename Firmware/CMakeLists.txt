# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.13)

execute_process(
    COMMAND git -C ${CMAKE_CURRENT_LIST_DIR} rev-parse --short HEAD
    OUTPUT_VARIABLE GIT_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
string(TIMESTAMP BUILD_DATE "%Y%m%d")
if(GIT_SHA)
    set(PROJECT_VER "${BUILD_DATE}-${GIT_SHA}")
else()
    set(PROJECT_VER "${BUILD_DATE}")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(esprc_brain)
