# 📘 **Installing CommonAPI Core Runtime (capicxx-core-runtime) on QNX x86_64 VM**

*Validated on Ubuntu 24.04 Host + QNX SDP 8.0 + QNX Neutrino x86_64 VM*

---

## 📌 **Overview**

This guide explains how to:

1. Prepare the QNX cross-compilation environment
2. Patch the CommonAPI Core CMake system to support QNX
3. Build the CommonAPI Core Runtime using **QCC (QNX compiler)**
4. Install the resulting library into the QNX sysroot (`$QNX_TARGET`)
5. Verify the installation

This process targets:

* **QNX Neutrino 8.0**
* **x86_64 architecture**
* **CommonAPI Core Runtime 3.2.4**

---

# 🧩 **1. Prerequisites**

### ✔ QNX SDP 8.0 installed on Ubuntu

Usually located at:

```
~/home/<user>/qnx800/
```

### ✔ QNX VMWare VM (x86_64)

### ✔ CommonAPI Core Runtime source

Version 3.2.4 (tag checked out):

```
capicxx-core-runtime/
capicxx-core-runtime.orig/        # backup used for patch generation
```

### ✔ Required tools

```bash
sudo apt install cmake build-essential pkg-config
```

### ✔ Install the `capicxx-core-runtime` source

```bash
git clone https://github.com/GENIVI/capicxx-core-runtime.git
cd capicxx-core-runtime
git checkout 3.2.4
```

---

# 🏁 **2. Load the QNX Cross-Compilation Environment**

Before running CMake or building anything:

```bash
source /opt/qnx800/qnxsdp-env.sh
```

Verify that QNX compilers are available:

```bash
which qcc
echo $QNX_HOST
echo $QNX_TARGET
```

Expected:

```
/opt/qnx800/host/linux/x86_64/usr/bin/qcc
```

---

# 🧰 **3. Create the QNX Toolchain File**

Inside your workspace (example: `~/qnxprojects`):

Create:

```
toolchain-qnx.cmake
```

Contents:

```cmake
set(CMAKE_SYSTEM_NAME QNX)
set(CMAKE_SYSTEM_VERSION 8.0)

set(QNX_HOST   $ENV{QNX_HOST})
set(QNX_TARGET $ENV{QNX_TARGET})

# QNX compilers (x86_64)
set(CMAKE_C_COMPILER   ${QNX_HOST}/usr/bin/qcc)
set(CMAKE_CXX_COMPILER ${QNX_HOST}/usr/bin/qcc)

# Architecture flags
set(CMAKE_C_FLAGS   "-Vgcc_ntox86_64")
set(CMAKE_CXX_FLAGS "-Vgcc_ntox86_64")

# QNX sysroot
set(CMAKE_SYSROOT ${QNX_TARGET})

# Do not attempt running binaries on host
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
```

---

# 🛠️ **4. Apply the CMake Patch for QNX Support**

The repository already contains the required patch to enable QNX support in the CommonAPI Core Runtime build system.
This patch updates `CMakeLists.txt` to:

* detect and handle QNX correctly
* add QNX-specific compiler flags
* remove Linux-only behavior
* avoid linking against `libdl`
* disable eventfd/epoll for future SOME/IP porting

The patch is stored inside the repo at:

```
CommonAPI-Patchs/0001-qnx-core-runtime-cmake.patch
```

Apply it from the root of the CommonAPI source directory:

```bash
patch -p1 < 0001-qnx-core-runtime-cmake.patch
```

---

# 🏗️ **5. Configure the Build for QNX x86_64**

Inside your CommonAPI source:

```bash
cd capicxx-core-runtime
mkdir build
cd build
```

Run CMake:

```bash
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../toolchain-qnx.cmake \
  -DBUILD_TEST=OFF \
  -DBUILD_EXAMPLES=OFF
```

Expected:

```
-- The C compiler identification is QCC 12.x
-- The CXX compiler identification is QCC 12.x
-- Configuring CommonAPI for QNX...
```

---

# 🔨 **6. Build the CommonAPI Core Runtime**

```bash
cmake --build . -- -j$(nproc)
```

Expected output:

```
[100%] Linking C shared library libCommonAPI.so
[100%] Built target CommonAPI
```

---

# 📦 **7. Install the Library into the QNX Sysroot**

Install to:

```
$QNX_TARGET/x86_64/usr/local
```

Run:

```bash
cmake --install . --prefix $QNX_TARGET/x86_64/usr/local
```

This installs:

### Libraries:

```
$QNX_TARGET/x86_64/usr/local/lib/libCommonAPI.so
$QNX_TARGET/x86_64/usr/local/lib/libCommonAPI.so.3.2.4
```

### Headers:

```
$QNX_TARGET/x86_64/usr/local/include/CommonAPI-3.2/CommonAPI/*.hpp
```

---

# 🧪 **8. Verify the Installation**

Check library:

```bash
ls $QNX_TARGET/x86_64/usr/local/lib | grep Common
```

Expected:

```
libCommonAPI.so
libCommonAPI.so.3.2.4
```

Check ELF format:

```bash
file $QNX_TARGET/x86_64/usr/local/lib/libCommonAPI.so.3.2.4
```

---

# 🧱 **9. What’s Next?**

Now that the **CommonAPI Core Runtime** builds successfully for QNX:

✔ QNX cross toolchain validated
✔ CMake patch applied correctly
✔ Libraries installed into `$QNX_TARGET`
✔ Ready for application builds

Next step:

### **Build / Port CommonAPI-SomeIP Runtime for QNX**

(where we patch Linux-only syscalls like epoll, eventfd, timerfd)

---
