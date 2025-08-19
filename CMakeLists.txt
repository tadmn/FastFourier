# Copyright (c) 2020 - Fielding DSP GmbH
cmake_minimum_required(VERSION 3.10)

project(FastFourier)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS_RELEASE -O3)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# When building the public API, don't include any symbols from static libraries that we depend on
if ((ANDROID) OR (CMAKE_SYSTEM_NAME MATCHES Linux))
  set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--exclude-libs,ALL ${CMAKE_SHARED_LINKER_FLAGS}")
endif()

# Set a default build type if none was specified
set(default_build_type "Release")
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  set(default_build_type "Debug")
endif()

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions("-DDEBUG=1" "-D_DEBUG=1")
elseif(CMAKE_BUILD_TYPE MATCHES "Release")
  add_definitions("-DNDEBUG=1")
else()
  message(FATAL_ERROR "Unknown build configuration \"${CMAKE_BUILD_TYPE}\": must be Debug or Release")
endif()

# Increase warning level
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-missing-field-initializers -Wshadow-all -Wshorten-64-to-32 -Wstrict-aliasing -Wuninitialized -Wunused-parameter -Wsign-compare -Wint-conversion -Wconditional-uninitialized -Wreorder -Wconstant-conversion -Wunused-private-field -Wbool-conversion -Wextra-semi -Wno-ignored-qualifiers -Wunreachable-code")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--no-undefined")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-missing-field-initializers -Wshadow -Wstrict-aliasing -Wuninitialized -Wunused-parameter -Wsign-compare -Wreorder -Wno-ignored-qualifiers -Wunreachable-code")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--no-undefined")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

########################################################
##### Set Library paths and find common libraries ######
########################################################
if(APPLE)
 find_library(FFT_ENGINE_LIBRARY Accelerate)
elseif(CMAKE_SYSTEM_NAME MATCHES Windows)
  if (CMAKE_SIZEOF_VOID_P MATCHES "8")
    set(IPP_DIR "C:/Program Files (x86)/Intel/oneAPI/ipp/latest/lib/cmake/ipp")
    find_package(IPP)
    if (IPP_FOUND)
      set(FFT_ENGINE_LIBRARY IPP::ipps IPP::ippvm IPP::ippcore)
      message("IPP libraries found")
    else ()
      message(FATAL_ERROR "IPP libraries not found")
    endif ()
  else()
   message(FATAL_ERROR "32 bit Windows is not supported")
  endif()
elseif(CMAKE_SYSTEM_NAME MATCHES Linux)
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86" OR CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    if (CMAKE_SIZEOF_VOID_P MATCHES "8")
      set(IPP_DIR "/opt/intel/oneapi/ipp/latest/lib/cmake/ipp")
      find_package(IPP)
      if (IPP_FOUND)
        set(FFT_ENGINE_LIBRARY IPP::ipps IPP::ippvm IPP::ippcore)
        message("IPP libraries found")
      else ()
        message(FATAL_ERROR "IPP libraries not found")
      endif ()
    else()
      message(FATAL_ERROR "32 bit Linux is not supported")
    endif()
  else()
    message(FATAL_ERROR "Unsupported system processor \"${CMAKE_SYSTEM_PROCESSOR}\"")
  endif()
else()
  message(FATAL_ERROR "Unsupported system \"${CMAKE_SYSTEM_NAME}\"")
endif()

if(NOT DEFINED FFT_ENGINE_LIBRARY)
  message(FATAL_ERROR "FFT engine library not found")
endif()

########################################################
################### The main target ####################
########################################################
add_library(FastFourier STATIC src/FastFourier.cpp include/FastFourier.h)
target_link_libraries(FastFourier ${FFT_ENGINE_LIBRARY} Threads::Threads)
target_include_directories(FastFourier PUBLIC include)