cmake_minimum_required(VERSION 3.13)
project(Simple-FileTransfer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -O0")

# set(CMAKE_PREFIX_PATH
#     "/opt/commonapi/runtime"
#     "/opt/commonapi/vsomeip"
# )

# Path to CommonAPI and CommonAPI-SomeIP installation
set(QNX_SYSROOT "$ENV{QNX_TARGET}/x86_64/usr/local")

set(COMMONAPI_DIR "${QNX_SYSROOT}/lib/cmake/CommonAPI-3.2.4")
set(COMMONAPI_SOMEIP_DIR "${QNX_SYSROOT}/lib/cmake/CommonAPI-SomeIP-3.2.4")
set(VSOMEIP3_DIR "${QNX_SYSROOT}/lib/cmake/vsomeip3")
link_directories(${QNX_SYSROOT}/lib)



find_package(CommonAPI 3.2 REQUIRED HITNS ${COMMONAPI_DIR})
find_package(CommonAPI-SomeIP 3.2 REQUIRED HITNS ${COMMONAPI_SOMEIP_DIR})
find_package(vsomeip3 REQUIRED HINTS ${VSOMEIP3_DIR})

# Boost (inside QNX sysroot)
set(Boost_ROOT "${QNX_SYSROOT}")
set(Boost_NO_SYSTEM_PATHS ON)
find_package(Boost 1.78.0 REQUIRED COMPONENTS system thread log)

include_directories(
    src
    src-gen/core
    src-gen/someip
    ${COMMONAPI_INCLUDE_DIRS}
    ${COMMONAPI_SOMEIP_INCLUDE_DIRS}
    ${VSOMEIP_INCLUDE_DIRS}
)

file(GLOB CORE_GEN src-gen/core/v0/filetransfer/example/*.cpp)
file(GLOB SOMEIP_GEN src-gen/someip/v0/filetransfer/example/*.cpp)

add_executable(FileTransferServer
    src/FileTransferServer.cpp
    ${CORE_GEN}
    ${SOMEIP_GEN}
)

target_link_libraries(FileTransferServer
    ${QNX_SYSROOT}/lib/libCommonAPI-SomeIP.so.3.2.4
    ${QNX_SYSROOT}/lib/libCommonAPI.so.3.2.4
    vsomeip3
    vsomeip3-sd
    ${Boost_LIBRARIES}
    socket
)

# Set RPATH for runtime
set_target_properties(FileTransferServer PROPERTIES
    INSTALL_RPATH "${QNX_SYSROOT}/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
    LINK_FLAGS "-Wl,-rpath-link,${QNX_SYSROOT}/lib"
)

