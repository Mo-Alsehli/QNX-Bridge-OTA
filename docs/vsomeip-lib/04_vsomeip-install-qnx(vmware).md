# Building vsomeip for QNX SDP 8.0 (x86_64 VMware)

This guide documents the complete process of building and installing vsomeip 3.4.10 for QNX running on VMware x86_64 architecture.

## Table of Contents
1. [Prerequisites](#prerequisites)
2. [Workspace Setup](#workspace-setup)
3. [Building Boost](#building-boost)
4. [Building vsomeip](#building-vsomeip)
5. [Installing to QNX Target](#installing-to-qnx-target)
6. [Verification](#verification)

---

## Prerequisites

### Required Software
- Ubuntu host system
- QNX SDP 8.0 installed at `~/qnx800/`
- VMware with QNX x86_64 VM running
- Network connectivity between Ubuntu and QNX VM

### Verify QNX Installation
```bash
ls ~/qnx800/qnxsdp-env.sh
```

This file must exist before proceeding.

---

## Workspace Setup

### 1. Create Workspace Directory
```bash
mkdir -p ~/qnxprojects
cd ~/qnxprojects
export WORKSPACE=${PWD}
```

### 2. Clone Required Repositories
```bash
# Clone QNX build files
git clone https://github.com/qnx-ports/build-files.git

# Clone vsomeip 3.4.10 for QNX
git clone https://github.com/qnx-ports/vsomeip.git -b qnx_3.4.10

# Optional: Clone googletest for running tests
git clone https://github.com/qnx-ports/googletest.git -b qnx_v1.13.0
export GTEST_ROOT=$WORKSPACE/googletest
```

### 3. Source QNX Environment
```bash
source ~/qnx800/qnxsdp-env.sh
```

**Verify the environment:**
```bash
echo $QNX_HOST
echo $QNX_TARGET
which qcc
which q++
```

Expected output:
- `QNX_HOST`: `/home/<user>/qnx800/host/linux/x86_64`
- `QNX_TARGET`: `/home/<user>/qnx800/target/qnx`
- `qcc` and `q++` should point to QNX compiler locations

---

## Building Boost

vsomeip requires Boost 1.78.0 with QNX-specific patches.

### 1. Download Boost Source
**Option A: Using tarball (Recommended - Faster)**
```bash
cd ~/qnxprojects
wget https://archives.boost.io/release/1.78.0/source/boost_1_78_0.tar.gz
tar -xvf boost_1_78_0.tar.gz
mv boost_1_78_0 boost
```

**Option B: Using Git (Slower)**
```bash
cd ~/qnxprojects
git clone https://github.com/boostorg/boost.git
cd boost
git checkout boost-1.78.0
git submodule update --init --recursive --jobs 8
```

### 2. Apply QNX Patches

**Interprocess patch:**
```bash
cd ~/qnxprojects/boost/libs/interprocess
git apply $WORKSPACE/build-files/ports/boost/interprocess_1.78.0_qnx_7.1.patch
cd -
```

**Tools patch (mandatory):**
```bash
cd ~/qnxprojects/boost/tools/build
git apply $WORKSPACE/build-files/ports/boost/tools_qnx.patch
cd $WORKSPACE
```

### 3. Build and Install Boost for x86_64

```bash
cd ~/qnxprojects
export CPU=x86_64
#  consider `export CPUVAR=x86_64`

BOOST_CPP_VERSION_FLAG="-std=c++17" \
QNX_PROJECT_ROOT="$(pwd)/boost" \
make -C build-files/ports/boost install -j4 CPU=x86_64
```

**Note:** Use `-j$(nproc)` instead of `-j4` to use all CPU cores (requires 32GB RAM).

### 4. Verify Boost Installation

```bash
ls $QNX_TARGET/x86_64/usr/local/lib | grep boost
```

You should see:
- `libboost_system.so`, `libboost_thread.so`, `libboost_log.so`, etc.

---

## Building vsomeip

### 1. Set Environment Variables

```bash
export CPU=x86_64
export TEST_IP_MASTER="192.168.10.3"    # Your QNX VM IP
export TEST_IP_SLAVE="192.168.10.1"     # Your Ubuntu IP
export GTEST_ROOT=$WORKSPACE/googletest
```

### 2. Build vsomeip

```bash
cd ~/qnxprojects

TEST_IP_MASTER=$TEST_IP_MASTER \
TEST_IP_SLAVE=$TEST_IP_SLAVE \
QNX_PROJECT_ROOT="$(pwd)/vsomeip" \
make -C build-files/ports/vsomeip install -j4 CPU=x86_64
```

### 3. Verify Build Success

Check that libraries were created:
```bash
ls $QNX_TARGET/x86_64/usr/local/lib | grep vsomeip
```

Expected output:
```
libvsomeip3.so
libvsomeip3.so.3
libvsomeip3.so.3.4.10
libvsomeip3-sd.so
libvsomeip3-sd.so.3
libvsomeip3-sd.so.3.4.10
libvsomeip3-e2e.so
libvsomeip3-e2e.so.3
libvsomeip3-e2e.so.3.4.10
libvsomeip3-cfg.so
libvsomeip3-cfg.so.3
libvsomeip3-cfg.so.3.4.10
```

Check CMake files:
```bash
ls $QNX_TARGET/x86_64/usr/local/lib/cmake/vsomeip3
```

---

## Installing to QNX Target

### 1. Prepare Directories on QNX

SSH into your QNX VM:
```bash
ssh qnxuser@192.168.10.3
```

Create necessary directories:
```bash
mkdir -p /data/home/qnxuser/lib
mkdir -p /data/home/qnxuser/bin
mkdir -p /data/home/qnxuser/config
exit
```

### 2. Copy Libraries from Ubuntu to QNX

**Copy Boost libraries:**
```bash
scp $QNX_TARGET/x86_64/usr/local/lib/libboost* \
    qnxuser@192.168.10.3:/data/home/qnxuser/lib/
```

**Copy vsomeip libraries:**
```bash
scp $QNX_TARGET/x86_64/usr/local/lib/libvsomeip3* \
    qnxuser@192.168.10.3:/data/home/qnxuser/lib/
```

**Copy vsomeip SD, E2E, and CFG modules:**
```bash
scp $QNX_TARGET/x86_64/usr/local/lib/libvsomeip3-sd* \
    qnxuser@192.168.10.3:/data/home/qnxuser/lib/

scp $QNX_TARGET/x86_64/usr/local/lib/libvsomeip3-e2e* \
    qnxuser@192.168.10.3:/data/home/qnxuser/lib/

scp $QNX_TARGET/x86_64/usr/local/lib/libvsomeip3-cfg* \
    qnxuser@192.168.10.3:/data/home/qnxuser/lib/
```

### 3. Set Environment on QNX

SSH back into QNX:
```bash
ssh qnxuser@192.168.10.3
```

Export library path:
```bash
export LD_LIBRARY_PATH=/data/home/qnxuser/lib:$LD_LIBRARY_PATH
```

Make it persistent:
```bash
echo 'export LD_LIBRARY_PATH=/data/home/qnxuser/lib:$LD_LIBRARY_PATH' >> ~/.profile
```

---

## Verification

### 1. Check Library Architecture

Verify the libraries are x86_64:
```bash
file /data/home/qnxuser/lib/libvsomeip3.so
```

Expected output:
```
ELF 64-bit LSB shared object, x86-64, QNX...
```

**Important:** If you see "AArch64" or "ARM", you built for the wrong architecture.

### 2. List Installed Libraries

```bash
ls -lh /data/home/qnxuser/lib | grep vsomeip
ls -lh /data/home/qnxuser/lib | grep boost
```

### 3. Test Library Loading

```bash
export LD_LIBRARY_PATH=/data/home/qnxuser/lib
ldd /data/home/qnxuser/lib/libvsomeip3.so.3.4.10
```

All dependencies should resolve correctly.

---

## Troubleshooting

### Issue: "file in wrong format" error

**Cause:** You built libraries for the wrong CPU architecture (aarch64 instead of x86_64).

**Solution:**
1. Clean the old build:
   ```bash
   rm -rf $QNX_TARGET/aarch64le/usr/local/lib/libvsomeip*
   rm -rf $QNX_TARGET/aarch64le/usr/local/lib/libboost*
   ```

2. Rebuild with `CPU=x86_64` explicitly set (see Build steps above)

### Issue: Boost submodule clone takes forever

**Cause:** Boost has 150+ submodules and slow I/O inside VM.

**Solution:** Use the tarball method instead of git clone (Option A in Boost section).

### Issue: CMake cannot find vsomeip

**Cause:** CMake is looking in the wrong sysroot path.

**Solution:** When building applications, use:
```bash
-DCMAKE_PREFIX_PATH="$QNX_TARGET/x86_64/usr/local;$QNX_TARGET/x86_64/usr"
```

---

## Summary

After following this guide, you will have:

✅ Boost 1.78.0 installed in `$QNX_TARGET/x86_64/usr/local`  
✅ vsomeip 3.4.10 installed in `$QNX_TARGET/x86_64/usr/local`  
✅ Runtime libraries copied to QNX VM at `/data/home/qnxuser/lib`  
✅ Environment configured with `LD_LIBRARY_PATH`  

You are now ready to build and run SOME/IP applications on QNX.

---

## Next Steps

- [Building a SOME/IP Application for QNX](./vsomeip_app_qnx.md)
- [Configuring QNX Network with Static IP](./QNX_NETWORK_CONFIG.md)