## cmake flags
cmake_minimum_required (VERSION 3.0)
project(qoo)

# CMake modules should be included in ${CMAKE_SOURCE_DIR}/cmake
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# --- Build options ---
option(BUILD_SHARED "Build shared library" ON)
option(BUILD_STATIC "Build static library" ON)
option(BUILD_TESTS "Build tests" ON)
OPTION(ENABLE_CODECOVERAGE "Enable code coverage testing support" OFF)
OPTION(ENABLE_STATIC_ANALYSIS "Enable static analysis support" OFF)
OPTION(ENABLE_PROFILE "Enable code profiling support" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)

# --- Build properties ---

# Set a default build type if none was specified
set(default_build_type "Release")

IF(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
            STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
ENDIF()


if(ENABLE_SANITIZERS)
  message(STATUS "Forcing build type to Debug to run coverage.")
      set(CMAKE_BUILD_TYPE "Debug" CACHE
              STRING "Choose the type of build." FORCE)
      set (CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wshadow -Wpointer-arith -Wcast-qual -Wunused -Wstrict-prototypes -Wmissing-prototypes -Wwrite-strings -Werror -fno-omit-frame-pointer -fsanitize=address")
      set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wshadow -Wpointer-arith -Wcast-qual -Wunused -Wstrict-prototypes -Wmissing-prototypes -Wwrite-strings -Werror  -fno-omit-frame-pointer -fsanitize=address")
      set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
ENDIF()

if(ENABLE_CODECOVERAGE)
  message(STATUS "Forcing build type to Debug to run coverage.")
      set(CMAKE_BUILD_TYPE "Debug" CACHE
              STRING "Choose the type of build." FORCE)
  # --- System Libraries ---
  include(GNUInstallDirs)
  include(UseCodeCoverage)
ENDIF()

IF(ENABLE_STATIC_ANALYSIS)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  add_compile_options(-fanalyzer)
ENDIF()


# Generate position-independent code (-fPIC on UNIX)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O1 -std=gnu99 -fno-pie")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O1")

if(ENABLE_PROFILE)
  message(STATUS "Enabling profile flags.")
  string (REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
  string (REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g -pg -ggdb -fno-pie -fno-omit-frame-pointer")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -pg -ggdb -fno-pie -fno-omit-frame-pointer")
  # enable vectorization report flags
  # using Clang
  if (CMAKE_C_COMPILER_ID MATCHES "Clang")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Rpass-analysis=loop-vectorize -Rpass=loop-vectorize -Rpass-missed=loop-vectorize")

  # using GCC
  elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -ftree-vectorize -fopt-info-vec-all")

  # using Intel C++
  elseif (CMAKE_C_COMPILER_ID STREQUAL "Intel")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -qopt-report=5 -qopt-report-phase=vec")

  # using Visual Studio C++
  elseif (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  # TBD
  endif()
endif(ENABLE_PROFILE)

# --- Build directories ---
add_subdirectory("src")
add_subdirectory("t-digest-c/src")
include_directories("t-digest-c/src")

# --- Documentation ---
# TODO

# --- Unit Tests ---
ENABLE_TESTING()

if(BUILD_TESTS)
  add_subdirectory("tests")
endif()