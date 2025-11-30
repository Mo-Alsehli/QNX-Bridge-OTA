cmake_minimum_required(VERSION 3.13)
project(Simple-FileTransfer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -O0")

set(CMAKE_PREFIX_PATH
    "/opt/commonapi/runtime"
    "/opt/commonapi/vsomeip"
)

find_package(CommonAPI REQUIRED CONFIG)
find_package(CommonAPI-SomeIP REQUIRED CONFIG)
find_package(vsomeip3 REQUIRED)

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
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
)

add_executable(FileTransferClient
    src/FileTransferClient.cpp
    ${CORE_GEN}
    ${SOMEIP_GEN}
)

target_link_libraries(FileTransferClient
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
)
