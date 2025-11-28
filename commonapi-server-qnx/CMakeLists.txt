cmake_minimum_required(VERSION 3.13)
project(HelloWorldService)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------------------
# Paths inside QNX sysroot
# ------------------------------------------------------------------------------
set(QNX_SYSROOT "$ENV{QNX_TARGET}/x86_64/usr/local")

set(COMMONAPI_DIR        "${QNX_SYSROOT}/lib/cmake/CommonAPI-3.2.4")
set(COMMONAPI_SOMEIP_DIR "${QNX_SYSROOT}/lib/cmake/CommonAPI-SomeIP-3.2.4")
set(VSOMEIP_DIR          "${QNX_SYSROOT}/lib/cmake/vsomeip3")

# Add library search path
link_directories(${QNX_SYSROOT}/lib)

# ------------------------------------------------------------------------------
# Find packages
# ------------------------------------------------------------------------------
find_package(CommonAPI 3.2 REQUIRED HINTS ${COMMONAPI_DIR})
find_package(CommonAPI-SomeIP 3.2 REQUIRED HINTS ${COMMONAPI_SOMEIP_DIR})
find_package(vsomeip3 REQUIRED HINTS ${VSOMEIP_DIR})

# Boost (inside QNX sysroot)
set(Boost_ROOT "${QNX_SYSROOT}")
set(Boost_NO_SYSTEM_PATHS ON)
find_package(Boost 1.78.0 REQUIRED COMPONENTS system thread log)

# ------------------------------------------------------------------------------
# Include directories
# ------------------------------------------------------------------------------
include_directories(
    src-gen/core
    src-gen/someip
    ${COMMONAPI_INCLUDE_DIRS}
    ${COMMONAPI_SOMEIP_INCLUDE_DIRS}
    ${VSOMEIP_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
)

# ------------------------------------------------------------------------------
# Generated sources
# ------------------------------------------------------------------------------
file(GLOB CORE_GEN   src-gen/core/v0/commonapi/examples/*.cpp)
file(GLOB SOMEIP_GEN src-gen/someip/v0/commonapi/examples/*.cpp)

# ------------------------------------------------------------------------------
# Executable
# ------------------------------------------------------------------------------
add_executable(HelloWorldService
    src/HelloWorldService.cpp
    src/HelloWorldStubImpl.cpp
    ${CORE_GEN}
    ${SOMEIP_GEN}
)

# ------------------------------------------------------------------------------
# Linking with explicit library paths
# ------------------------------------------------------------------------------
target_link_libraries(HelloWorldService
    ${QNX_SYSROOT}/lib/libCommonAPI-SomeIP.so.3.2.4
    ${QNX_SYSROOT}/lib/libCommonAPI.so.3.2.4
    vsomeip3
    vsomeip3-sd
    ${Boost_LIBRARIES}
    socket
)

# Set RPATH for runtime
set_target_properties(HelloWorldService PROPERTIES
    INSTALL_RPATH "${QNX_SYSROOT}/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
    LINK_FLAGS "-Wl,-rpath-link,${QNX_SYSROOT}/lib"
)