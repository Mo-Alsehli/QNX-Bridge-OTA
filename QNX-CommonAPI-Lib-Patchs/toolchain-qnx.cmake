# Tell CMake we are cross-compiling for QNX
set(CMAKE_SYSTEM_NAME QNX)
set(CMAKE_SYSTEM_VERSION 8.0)

# Use the environment already set by qnxsdp-env.sh
set(QNX_HOST   $ENV{QNX_HOST})
set(QNX_TARGET $ENV{QNX_TARGET})

# Compilers (x86_64)
set(CMAKE_C_COMPILER   ${QNX_HOST}/usr/bin/qcc)
set(CMAKE_CXX_COMPILER ${QNX_HOST}/usr/bin/qcc)

# Architecture: x86_64 Neutrino
set(CMAKE_C_FLAGS   "-Vgcc_ntox86_64")
set(CMAKE_CXX_FLAGS "-Vgcc_ntox86_64")

# Sysroot
set(CMAKE_SYSROOT ${QNX_TARGET})

# Prevent CMake from testing programs on the host
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
