cmake_minimum_required(VERSION 2.8)

set(PROJECT_NAME "AICB")

# source files
file(GLOB_RECURSE src_files "*.cpp")

# link libraries
include(${CMAKE_CURRENT_SOURCE_DIR}/../common-link-libraries.cmake)
