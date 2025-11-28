cmake_minimum_required(VERSION 3.10)
project(HelloWorldClient)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall")

find_package(CommonAPI REQUIRED CONFIG)
find_package(CommonAPI-SomeIP REQUIRED CONFIG)
find_package(vsomeip3 REQUIRED)
find_package(Boost COMPONENTS system thread REQUIRED)

include_directories(
    src-gen/core
    src-gen/someip
    ${Boost_INCLUDE_DIRS}
    ${COMMONAPI_INCLUDE_DIRS}
    ${COMMONAPI_SOMEIP_INCLUDE_DIRS}
    ${VSOMEIP_INCLUDE_DIRS}
)

file(GLOB CORE_GEN src-gen/core/v1/commonapi/examples/*.cpp)
file(GLOB SOMEIP_GEN src-gen/someip/v1/commonapi/examples/*.cpp)

add_executable(HelloWorldClient
    HelloWorldClient.cpp
    ${CORE_GEN}
    ${SOMEIP_GEN}
)

target_link_libraries(HelloWorldClient
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
    ${Boost_LIBRARIES}
)
