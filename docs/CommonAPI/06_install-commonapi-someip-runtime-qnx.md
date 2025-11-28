# QNX Port of `capicxx-someip-runtime` (CommonAPI-SomeIP Runtime)

## 0. Purpose of This Document

This document explains **from scratch** how to:

1. Set up the **QNX cross-compilation environment** on Ubuntu
2. Clone the upstream `capicxx-someip-runtime`
3. Understand **why QNX requires porting**
4. Apply the **three QNX patches**

   * `0001-qnx-someip-runtime-cmake.patch`
   * `0002-someip-qnx-header-compat.patch`
   * `0003-someip-qnx-src-compat.patch`
5. Build and install the QNX version of **libCommonAPI-SomeIP.so**
6. Regenerate SOME/IP bindings in a **QNX-safe configuration**

All steps are complete, explicit, and based fully on the working procedure you validated.

---

## 1. Background – Why a QNX Port Is Needed

The upstream **CommonAPI-SomeIP Runtime** is designed for **Linux**, and it assumes the presence of:

* Linux-specific system calls like:

  * `epoll_*`
  * `eventfd()`
  * `timerfd_*`
  * `inotify_*`
* Linux libraries (`dl`, `rt`)
* GCC + glibc behavior
* Linux header availability
* Linux runtime behavior for threads, sockets, timers, etc.

However, QNX:

* **Does not implement** these syscalls
* Uses **QCC/QCC** compilers and its own sysroot
* Requires CMake to be explicitly told how to cross-compile
* Requires replacing or disabling Linux-specific features

Thus, the runtime must be **patched** to:

1. Disable Linux-only syscalls
2. Add QNX-safe replacements (select/poll/POSIX timers)
3. Modify headers to avoid pulling Linux-only includes
4. Adapt the build system to use QNX correctly

---

## 2. QNX Environment Setup on Ubuntu

### 2.1 Install QNX SDP

Assume QNX SDP 8.0 is installed at:

```
/opt/qnx800/
```

### 2.2 Load the QNX Build Environment

You must run this in **every shell** used for building:

```bash
source /opt/qnx800/qnxsdp-env.sh
```

This sets:

* `QNX_HOST`
* `QNX_TARGET`
* `PATH` → contains QNX tools and QCC compiler

Verify:

```bash
which qcc
which QCC
echo $QNX_TARGET
echo $QNX_HOST
```

All must point inside `/opt/qnx800/…`.

### 2.3 Create a QNX CMake Toolchain File

Create:

```bash
mkdir -p ~/qnxprojects
nano ~/qnxprojects/toolchain-qnx.cmake
```

Add:

```cmake
set(CMAKE_SYSTEM_NAME QNX)
set(CMAKE_SYSTEM_VERSION 8.0)

set(QNX_HOST   $ENV{QNX_HOST})
set(QNX_TARGET $ENV{QNX_TARGET})

set(CMAKE_C_COMPILER   ${QNX_HOST}/usr/bin/qcc)
set(CMAKE_CXX_COMPILER ${QNX_HOST}/usr/bin/qcc)

set(CMAKE_C_FLAGS   "-Vgcc_ntox86_64")
set(CMAKE_CXX_FLAGS "-Vgcc_ntox86_64")

set(CMAKE_SYSROOT ${QNX_TARGET})

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
```

**Purpose:**
This prevents CMake from using Linux GCC and forces the correct QNX sysroot.

---

## 3. Preparing the Source and Patch Layout

Clone the upstream runtime:

```bash
cd ~/qnxprojects
git clone https://github.com/GENIVI/capicxx-someip-runtime.git
cp -r capicxx-someip-runtime capicxx-someip-runtime.orig
```

Patch directory:

```
QNX-Bridge-OTA/
  CommonAPI-Patchs/
    0001-qnx-someip-runtime-cmake.patch
    0002-someip-qnx-header-compat.patch
    0003-someip-qnx-src-compat.patch
```

Use:

* `capicxx-someip-runtime/` → **patched tree**
* `capicxx-someip-runtime.orig/` → **clean reference**

---

## 4. Overview of the QNX Porting Patches

### 4.1 Patch 1 – QNX CMake Adaptation

**Purpose:** Adjust build system for QNX.

Main changes:

1. QNX detection and log message:

   ```cmake
   message(STATUS "Configuring CommonAPI-SomeIP for QNX.")
   ```

2. Use QNX platform libs:

   ```cmake
   if(CMAKE_SYSTEM_NAME STREQUAL "QNX")
     set(PLATFORM_LIBS pthread)
   else()
     set(PLATFORM_LIBS dl pthread rt)
   endif()
   ```

3. Disable Linux syscalls:

   ```cmake
   add_definitions(
     -DCAPI_NO_EPOLL
     -DCAPI_NO_EVENTFD
     -DCAPI_NO_TIMERFD
     -DCAPI_NO_INOTIFY
   )
   ```

4. Disable tests/examples for QNX:

   ```cmake
   if(CMAKE_SYSTEM_NAME STREQUAL "QNX")
     set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
     set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
   endif()
   ```

5. Link target with QNX libs only:

   ```cmake
   target_link_libraries(CommonAPI-SomeIP ${PLATFORM_LIBS})
   ```

---

### 4.2 Patch 2 – Header Compatibility

**Purpose:** Remove Linux-only includes and add QNX-safe ones.

Examples:

```cpp
#ifndef CAPI_NO_EPOLL
#include <sys/epoll.h>
#endif
```

Removed:

```cpp
// #include <sys/inotify.h>
```

Added:

```cpp
#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
```

Also includes fixes for:

* `__unused` macros
* GCC/glibc attribute compatibility

---

### 4.3 Patch 3 – Source Compatibility

**Purpose:** Replace Linux functionality in `.cpp` files.

Examples:

#### Epoll

```cpp
#ifndef CAPI_NO_EPOLL
    // original epoll implementation
#else
    // QNX select()/poll() replacement
#endif
```

#### Timerfd / eventfd

* Removed or replaced with:

  * POSIX timers
  * Pipes
  * Select loops

#### Thread naming differences

```cpp
#ifndef __QNXNTO__
pthread_setname_np(...);
#endif
```

This patch adapts the runtime to work under QNX without Linux syscall failures.

---

## 5. Applying All Patches to a Fresh Clone

From inside the runtime directory:

```bash
cd ~/qnxprojects/capicxx-someip-runtime
patch -p1 < ../QNX-Bridge-OTA/CommonAPI-Patchs/0001-qnx-someip-runtime-cmake.patch
patch -p1 < ../QNX-Bridge-OTA/CommonAPI-Patchs/0002-someip-qnx-header-compat.patch
patch -p1 < ../QNX-Bridge-OTA/CommonAPI-Patchs/0003-someip-qnx-src-compat.patch
```

If all patches apply cleanly, you are ready to build.

---

## 6. Building `capicxx-someip-runtime` for QNX

### 6.1 Load the QNX Environment

```bash
source /opt/qnx800/qnxsdp-env.sh
```

### 6.2 Configure Using CMake

```bash
cd ~/qnxprojects/capicxx-someip-runtime
mkdir -p build
cd build

cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../toolchain-qnx.cmake \
  -DBUILD_TESTS=OFF \
  -DBUILD_EXAMPLES=OFF
```

Expected important lines:

```
-- The C compiler identification is QCC
-- The CXX compiler identification is QCC
-- Configuring CommonAPI-SomeIP for QNX.
```

If you see GCC, something is wrong.

### 6.3 Build

```bash
cmake --build . -- -j$(nproc)
```

This produces:

```
libCommonAPI-SomeIP.so
```

---

## 7. Installing the Runtime to the QNX Sysroot

Run:

```bash
cmake --install .
```

This installs:

* Library → `$QNX_TARGET/usr/local/lib`
* Headers → `$QNX_TARGET/usr/local/include/CommonAPI-3.2/CommonAPI/SomeIP/`
* CMake config files → `$QNX_TARGET/usr/local/lib/cmake/CommonAPI-SomeIP/`

Verify:

```bash
ls $QNX_TARGET/x86_64/usr/local/lib | grep CommonAPI-SomeIP
```

You now have a working, QNX-compatible **CommonAPI-SomeIP Runtime**.

---

## 8. Regenerating SOME/IP Bindings for QNX

When generating bindings using the codegen tools, you must ensure that they inherit the same flags used by the runtime.

Set:

```bash
export COMMONAPI_CPP_FLAGS="-DCAPI_NO_EPOLL -DCAPI_NO_EVENTFD -DCAPI_NO_TIMERFD -DCAPI_NO_INOTIFY"
```

Then invoke the generator:

```bash
commonapi-generator \
  --dest ./src-gen \
  --flag $COMMONAPI_CPP_FLAGS \
  your_interface.fidl
```

Likewise for the SOME/IP generator:

```bash
commonapi-someip-generator \
  --generate-proxy \
  --generate-stub \
  --dest ./src-gen \
  --flag $COMMONAPI_CPP_FLAGS \
  your_interface.fdepl
```

This ensures the generated code **matches the patched runtime** and does not call Linux syscalls.

---

## 9. Completion

You now have:

* A fully patched, QNX-ready `capicxx-someip-runtime`
* A working QNX build environment
* Installed runtime into QNX sysroot
* Proper code generation setup for QNX bindings

This completes the full end-to-end porting workflow.
