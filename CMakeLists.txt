# although cmake is de facto the best build systems generator tool for cpp, F**K CMAKE

### the cmake version we accepted. Don't modify this, we've been pretty
# restrained about the version of cmake we use.
# Update the version of cmake on your system to use modern cmake features.
cmake_minimum_required(VERSION 3.15...3.24)

### optional: C and CXX compiler
# Note that these two lines must go before enable language, or conan's check
# will fail
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

### the project statement
# Note that after we call this line in the top level project, we can use
# variables like PROJECT_VERSION_MAJOR,
# <PROJECT-NAME>_VERSION_{MAJOR, MINOR, PATCH, TWEAK} and CMAKE_PROJECT_VERSION
project(
  mergebot
  VERSION 0.1.0
  LANGUAGES C CXX)

### options go here
option(MB_OUT_TO_BIN "Output all target files to build/bin" ON)
option(MB_BUILD_SHARED "Build shared library for mergebot" OFF)
if(PROJECT_NAME STREQUAL CMAKE_PROJECT_NAME)
  option(MB_BUILD_TESTs "Build mergebot's unit tests" ON)
endif(PROJECT_NAME STREQUAL CMAKE_PROJECT_NAME)
option(MB_USE_CCACHE "Use ccache to accelerate compilation if ccache found in path" ON)
option(MB_INSTALL_TARGET "Enable installation targets" OFF)

### if you set any CMAKE_ variables, that can go here.
# (But usually don't do this, except maybe for C++ standard)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH} ")
set(CMAKE_LINK_WHAT_YOU_USE ON)

# C standard can be overridden when this is used as a sub-project
if(NOT CMAKE_C_STANDARD)
  # This project can use C11, but will gracefully decay down to C89
  set(CMAKE_C_STANDARD 11 CACHE STRING "The C standard we recommend to use")
  set(CMAKE_C_STANDARD_REQUIRED OFF)
  set(CMAKE_C_EXTENSIONS OFF)
endif(NOT CMAKE_C_STANDARD)

# C++ standard can also be overridden when this is used as a sub-project
if(NOT CMAKE_CXX_STANDARD)
  # we can use all C++17 features in this project
  set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard to use")
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif(NOT CMAKE_CXX_STANDARD)

# enable dynamic lib to link static lib
if(NOT DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
    # Otherwise we cannot link .so libs with .a libs
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif(NOT DEFINED CMAKE_POSITION_INDEPENDENT_CODE)

# the default build target is set to Release
if (NOT CMAKE_BUILD_TYPE)
    # Default build target
    # TODO(hwa): in release phase, change default build target to Release
    set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_BUILD_TYPE)
message(STATUS "Build directory: ${PROJECT_BINARY_DIR}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

### conventions we need to follow
if (PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)
    message(FATAL_ERROR "The binary directory of CMake cannot be the same as source directory!"
        "Please consider use 'cmake -B build' to specify a different binary directory."
        "Otherwise mergebot may fail to build, or make Git hard to exclude binary files.")
endif(PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)

### solve compatibility problems
if(WIN32)
  set(MB_PLATFORM_NAME MB_PLATFORM_WINDOWS)
  # get rid of 'min', 'max' macros; get back 'M_PI', 'M_E'
  add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
  add_definitions(-D_UNICODE -DUNICODE)
else(WIN32)
  set(MB_PLATFORM_NAME MB_PLATFORM_POSIX)
endif(WIN32)

if (MB_OUT_TO_BIN)
    # Otherwise Windows (which doesn't support RPATH) cannot find the DLLs at runtime
    set(MB_OUTPUT_DIR ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${MB_OUTPUT_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${MB_OUTPUT_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${MB_OUTPUT_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${MB_OUTPUT_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${MB_OUTPUT_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${MB_OUTPUT_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${MB_OUTPUT_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${MB_OUTPUT_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${MB_OUTPUT_DIR})
    set(CMAKE_DEBUG_POSTFIX "")  # No more annoying MB_d.dll
endif()

### our preferences
# make compiler be a grudge in debug mode
# speed up compilation by cache(https://ccache.dev)
if(MB_USE_CCACHE AND NOT MSVC)
  find_program(CCACHE_PROGRAM ccache)
  if(CCACHE_PROGRAM)
    message(STATUS "Found CCache: ${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  endif(CCACHE_PROGRAM)
endif(MB_USE_CCACHE AND NOT MSVC)

if(CMAKE_BUILD_TYPE STREQUAL "DEBUG")
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC") # MSVC
    # non standard warning,member function not override any super class members,
    # non-virtual-dtor
    set(CMAKE_cxx_FLAGS "${CMAKE_CXX_FLAGS} /permissive /w14263 /w14265")
  else(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")  # clang and gcc
    set(CMAKE_cxx_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wnon-virtual-dtor\
                         -Wold-style-cast -Woverloaded-virtual -Wnull-dereference")
    set(CMAKE_cxx_FLAGS "${CMAKE_cxx_FLAGS} -O0 -g")
  endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
endif(CMAKE_BUILD_TYPE STREQUAL "DEBUG")

if(BUILD_SHARED_LIBS)
  # Only export MB_EXPORT symbols from the shared library.
  add_compile_options(-fvisibility=hidden)
endif(BUILD_SHARED_LIBS)

if(BUILD_SHARED_LIBS)
  target_compile_definitions(mergebot
    PUBLIC
      # Used by include/export.h.
      MB_SHARED_LIBRARY
  )
endif(BUILD_SHARED_LIBS)


## configure conan
include(cmake/conaninstall.cmake)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

### find packages go here.
## use conan to manage all the packages it contains
find_package(ZLIB)
if (ZLIB_FOUND)
  set(ZLIB_LIBRARY ${ZLIB_LIBRARIES})
#  add_definitions(-DMB_HAVE_ZLIB)
  # TODO(hwa): fix the error of undefined reference to deflate
else()
  message("ZLIB can't be found. MergeBot configured without zlib support")
endif ()

## check linter tools goes here
include(cmake/checklinter.cmake)

## use old school methods to manage 3rd party dependency here.
# when we talk about old school methods, we refer to find_package,
# fetch_content, shell script
include(cmake/googletest.cmake)

### split compiled libraries and executables to sub folders,
### don't add those targets here
# add_subdirectory starts here
# add compiled library here
add_subdirectory(src)



### test target goes here
# TODO(hwa): enable google testing
if(MB_BUILD_TESTS)
  enable_testing()

  # prevent overriding the parent project's compiler/linker settings on Windows.
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(install_gtest OFF)
  set(install_gmock OFF)
  set(build_gmock ON)

  # This project is tested using GoogleTest.
  add_subdirectory("third_party/googletest")
endif(MB_BUILD_TESTS)


### install target goes here
# TODO(hwa): add install targets
if(MB_INSTALL_TARGET)

endif(MB_INSTALL_TARGET)
