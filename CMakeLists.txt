cmake_minimum_required(VERSION 3.20.0)
project(symbolic)

set(EXECUTABLE_NAME symbolic)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "-O0 -g3")
elseif(CMAKE_BUILD_TYPE STREQUAL "Test")
    set(CMAKE_CXX_FLAGS "-O0 -g3 -DTEST_BUILD")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "-O3 -g0 -DNDEBUG")
endif()

# Configuration
# =============

include(GNUInstallDirs)

# We generate the kernel.json file, given the installation prefix and the executable name
configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/symbolic/kernel.json.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/symbolic/kernel.json"
)

option(XEUS_STATIC_DEPENDENCIES "link statically with xeus dependencies" OFF)
if (XEUS_STATIC_DEPENDENCIES)
    set(xeus-zmq_target "xeus-zmq-static")
else ()
    set(xeus-zmq_target "xeus-zmq")
endif ()

# Dependencies
# ============

# Be sure to use recent versions
set(xeus-zmq_REQUIRED_VERSION 1.0.2)

find_package(xeus-zmq ${xeus-zmq_REQUIRED_VERSION} REQUIRED)
find_package(Threads)
find_package(Boost REQUIRED COMPONENTS regex)
include_directories(${BOOST_INCLUDEDIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Target and link
# ===============

if(CMAKE_BUILD_TYPE STREQUAL "Test")
    file(GLOB_RECURSE SOURCES src/**.cpp tests/**.cpp)
else()
    file(GLOB_RECURSE SOURCES src/**.cpp)
endif()

# My kernel executable
add_executable(${EXECUTABLE_NAME} ${SOURCES} )
target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${xeus-zmq_target} Threads::Threads ${BOOST_REGEX_LIBRARY})

set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

# Installation
# ============

# Install my_kernel
install(TARGETS ${EXECUTABLE_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Configuration and data directories for jupyter and my_kernel
set(XJUPYTER_DATA_DIR "share/jupyter" CACHE STRING "Jupyter data directory")

# Install Jupyter kernelspecs
set(MY_KERNELSPEC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels)
install(DIRECTORY ${MY_KERNELSPEC_DIR}
        DESTINATION ${XJUPYTER_DATA_DIR}
        PATTERN "*.in" EXCLUDE)
