cmake_minimum_required(VERSION 3.13)
project(service-example)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VSOMEIP_DIR "$ENV{QNX_TARGET}/x86_64/usr/local/lib/cmake/vsomeip3")


# Find vsomeip
find_package(vsomeip3 REQUIRED HINTS ${VSOMEIP_DIR})

# Find Boost inside QNX sysroot
set(BOOST_ROOT "$ENV{QNX_TARGET}/x86_64/usr/local")
find_package(Boost REQUIRED COMPONENTS system thread log)

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${VSOMEIP_INCLUDE_DIRS}
)

add_executable(service-example src/service-example.cpp)

target_link_libraries(service-example
    vsomeip3
    vsomeip3-sd
    ${Boost_LIBRARIES}
)
