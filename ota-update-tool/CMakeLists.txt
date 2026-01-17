cmake_minimum_required(VERSION 3.16)

project(ota-apply
  VERSION 1.0.0
  LANGUAGES CXX
)

# ---- Build option: device naming ----
option(QEMU_ENV "Build for QEMU target device naming (vda2/vda3)" ON)

# ---- Executable ----
add_executable(ota-apply
  src/ota-apply.cpp
)

# ---- C++ standard ----
target_compile_features(ota-apply PRIVATE cxx_std_17)

# ---- Warnings (reasonable for system utility) ----
target_compile_options(ota-apply PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

# ---- Define QEMU_ENV exactly as used in the code ----
if(QEMU_ENV)
  target_compile_definitions(ota-apply PRIVATE QEMU_ENV=1)
else()
  target_compile_definitions(ota-apply PRIVATE QEMU_ENV=0)
endif()

# ---- Install to /usr/sbin (privileged system tool) ----
include(GNUInstallDirs)
install(TARGETS ota-apply
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
)
