cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

# # Detect if we're being included as a subproject.  If so, disable the tests.
# if(DEFINED PROJECT_NAME)
#   set(IS_SUBPROJECT ON)
# else()
#   set(IS_SUBPROJECT OFF)
# endif()

#
# Define our project name and top-level settings.
#
project(cpp-tagged-union CXX)

# Require the C++14 standard (constexpr constructor assignment expression).
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#
# Configure the project
#
include_directories(
  ${PROJECT_SOURCE_DIR}/include
)
