# Building a SOME/IP Application for QNX x86_64

This guide explains how to build a SOME/IP service application for QNX using vsomeip, following the exact workflow used in the project.

## Table of Contents
1. [Prerequisites](#prerequisites)
2. [Project Structure](#project-structure)
3. [CMakeLists Configuration](#cmakelists-configuration)
4. [Building the Application](#building-the-application)
5. [Creating vsomeip Configuration](#creating-vsomeip-configuration)
6. [Deploying to QNX](#deploying-to-qnx)

---

## Prerequisites

Before starting, ensure you have:

✅ vsomeip installed for QNX x86_64 (see previous guide)  
✅ QNX SDP 8.0 environment sourced  
✅ Your service source code (e.g., `service-example.cpp`)  
✅ Network connectivity to QNX VM  

---

## Project Structure

Create your project directory:

```bash
cd ~/qnxprojects
mkdir vsomeip-server-qnx
cd vsomeip-server-qnx
```

Recommended structure:
```
vsomeip-server-qnx/
├── CMakeLists.txt
├── src/
│   └── service-example.cpp
└── config/
    └── service.json
```

---

## CMakeLists Configuration

Create `CMakeLists.txt` in your project root:

```cmake
cmake_minimum_required(VERSION 3.13)
project(vsomeip_service)

# Use modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QNX-specific: Position independent code
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# Tell CMake where to find QNX vsomeip (x86_64)
set(VSOMEIP_DIR "$ENV{QNX_TARGET}/x86_64/usr/local/lib/cmake/vsomeip3")
set(BOOST_ROOT "$ENV{QNX_TARGET}/x86_64/usr/local")

# Find vsomeip
find_package(vsomeip3 REQUIRED HINTS ${VSOMEIP_DIR})

# Find Boost
find_package(Boost REQUIRED COMPONENTS system thread log)

# Include directories
include_directories(
    ${Boost_INCLUDE_DIRS}
    ${VSOMEIP_INCLUDE_DIRS}
)

# Create executable
add_executable(service-example src/service-example.cpp)

# Link libraries
target_link_libraries(service-example
    vsomeip3
    vsomeip3-sd
    ${Boost_LIBRARIES}
)
```

### Key Points:

- **VSOMEIP_DIR**: Points to x86_64 CMake module (not aarch64)
- **BOOST_ROOT**: Points to x86_64 Boost installation
- **vsomeip3-sd**: Service Discovery module (required for SD functionality)
- **Boost components**: system, thread, log (commonly needed)

---

## Building the Application

### 1. Source QNX Environment

```bash
source ~/qnx800/qnxsdp-env.sh
```

### 2. Clear Any Conflicting Environment Variables

**Critical:** Remove any Linux vsomeip paths from environment:

```bash
unset LD_LIBRARY_PATH
unset CPLUS_INCLUDE_PATH
unset PKG_CONFIG_PATH
unset CMAKE_PREFIX_PATH
```

**Why?** If you previously built vsomeip for Linux/Raspberry Pi, these variables will cause CMake to link against the wrong libraries.

### 3. Create Build Directory

```bash
cd ~/qnxprojects/vsomeip-server-qnx
mkdir build
cd build
```

### 4. Run CMake with QNX Cross-Compiler

```bash
cmake .. \
  -DCMAKE_C_COMPILER=qcc \
  -DCMAKE_CXX_COMPILER=q++ \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSROOT=$QNX_TARGET \
  -DCMAKE_PREFIX_PATH="$QNX_TARGET/x86_64/usr/local;$QNX_TARGET/x86_64/usr"
```

**Expected CMake Output:**
```
-- Found vsomeip3: /home/user/qnx800/target/qnx/x86_64/usr/local/lib/libvsomeip3.so
-- Found Boost: /home/user/qnx800/target/qnx/x86_64/usr/local (found version "1.78.0")
-- Configuring done
-- Generating done
```

### 5. Build the Executable

```bash
make -j4
```

**Expected Output:**
```
[ 50%] Building CXX object CMakeFiles/service-example.dir/src/service-example.cpp.o
[100%] Linking CXX executable service-example
[100%] Built target service-example
```

### 6. Verify the Binary

```bash
file service-example
```

**Expected:**
```
service-example: ELF 64-bit LSB executable, x86-64, ... QNX ...
```

---

## Creating vsomeip Configuration

### 1. Create Configuration Directory

```bash
mkdir -p ~/qnxprojects/vsomeip-server-qnx/config
```

### 2. Create `service.json`

Create `config/service.json` with the following content:

```json
{
    "unicast": "192.168.10.3",
    "logging": {
        "level": "debug",
        "console": "true"
    },
    "applications": [
        {
            "name": "World",
            "id": "0x1212"
        }
    ],
    "services": [
        {
            "service": "0x1234",
            "instance": "0x5678",
            "reliable": "tcp",
            "unreliable": "udp"
        }
    ],
    "routing": "World",
    "service-discovery": {
        "enable": true,
        "multicast": "224.224.224.245",
        "port": 30490,
        "protocol": "udp"
    }
}
```

### Configuration Explanation:

- **unicast**: QNX VM IP address (must match your network)
- **applications.name**: Application identifier (matches routing manager)
- **applications.id**: Unique application ID in hex
- **services.service**: SOME/IP service ID (0x1234 in this example)
- **services.instance**: SOME/IP instance ID (0x5678)
- **reliable**: TCP transport for reliable messages
- **unreliable**: UDP transport for unreliable messages
- **routing**: Name of the routing manager application
- **service-discovery**:
  - **enable**: Must be `true` for service discovery
  - **multicast**: Standard SOME/IP-SD multicast address (224.224.224.245)
  - **port**: Standard SOME/IP-SD port (30490)
  - **protocol**: Always "udp" for service discovery

---

## Deploying to QNX

### 1. Copy the Binary

```bash
scp build/service-example qnxuser@192.168.10.3:/data/home/qnxuser/bin/
```

### 2. Copy the Configuration

```bash
scp config/service.json qnxuser@192.168.10.3:/data/home/qnxuser/config/
```

### 3. SSH into QNX and Set Permissions

```bash
ssh qnxuser@192.168.10.3

chmod +x /data/home/qnxuser/bin/service-example
```

---

## Running the Application on QNX

### 1. Set Environment Variables

```bash
export LD_LIBRARY_PATH=/data/home/qnxuser/lib:$LD_LIBRARY_PATH
export VSOMEIP_CONFIGURATION=/data/home/qnxuser/config/service.json
```

### 2. Configure Multicast Routing

QNX requires an explicit route for multicast traffic:

```bash
route add -host 224.224.224.245 192.168.10.3 -ifp vmx0
```

Verify the route:
```bash
route show
```

Expected output should include:
```
224.224.224.245  192.168.10.3  UH  vmx0
```

### 3. Run the Service

```bash
/data/home/qnxuser/bin/service-example
```

### Expected Output:

```
[info] Application "World" is registered.
[info] Routing manager initialized.
[info] OFFERING service [1234.5678]
[info] Service Discovery started.
```

---

## Troubleshooting

### Issue: "Cannot load shared library"

**Symptoms:**
```
error while loading shared libraries: libvsomeip3.so.3: cannot open shared object file
```

**Solution:**
```bash
export LD_LIBRARY_PATH=/data/home/qnxuser/lib
ldd /data/home/qnxuser/bin/service-example
```

Ensure all libraries resolve correctly.

---

### Issue: "file in wrong format" during CMake

**Cause:** Linking against aarch64 libraries instead of x86_64.

**Solution:**
1. Verify CMAKE_PREFIX_PATH points to x86_64:
   ```bash
   echo $QNX_TARGET/x86_64/usr/local
   ```

2. Clean build and rebuild:
   ```bash
   cd build
   rm -rf *
   cmake .. [... with correct paths]
   ```

---

### Issue: Service Discovery not working

**Symptoms:** Client cannot discover the service.

**Checklist:**
1. Verify multicast address is `224.224.224.245`
2. Verify multicast route is added:
   ```bash
   route show | grep 224.224.224.245
   ```
3. Verify port 30490 (not a string)
4. Verify firewall allows UDP multicast
5. Verify both service and client use the same SD configuration

---

### Issue: CMake cannot find Boost or vsomeip

**Solution:**

Check that paths exist:
```bash
ls $QNX_TARGET/x86_64/usr/local/lib/cmake/vsomeip3
ls $QNX_TARGET/x86_64/usr/local/lib/cmake/Boost-1.78.0
```

If missing, rebuild Boost and vsomeip (see installation guide).

---

## Testing with a Client

Once your service is running on QNX, you can test it with a client running on:
- Raspberry Pi
- Ubuntu host
- Another QNX instance

### Example Client Configuration (Raspberry Pi)

Create a `client.json` on the Raspberry Pi:

```json
{
    "unicast": "192.168.10.2",
    "logging": {
        "level": "debug",
        "console": "true"
    },
    "applications": [
        {
            "name": "HelloClient",
            "id": "0x1313"
        }
    ],
    "routing": "HelloClient",
    "service-discovery": {
        "enable": true,
        "multicast": "224.224.224.245",
        "port": 30490,
        "protocol": "udp"
    }
}
```

Run the client:
```bash
export VSOMEIP_CONFIGURATION=/path/to/client.json
./client-example
```

Expected interaction:
```
[Client] Discovered service [1234.5678] at 192.168.10.3
[Client] Sending request to service...
[Service] Received request, sending response...
[Client] Received response from service.
```

---

## Summary

After completing this guide, you have:

✅ Created a QNX-compatible CMakeLists.txt  
✅ Built a SOME/IP service for x86_64 QNX  
✅ Created a correct vsomeip.json configuration  
✅ Deployed the application to QNX VM  
✅ Configured multicast routing  
✅ Successfully run a SOME/IP service  

---

## Next Steps

- [Configure QNX Network with Static IP](./QNX_NETWORK_CONFIG.md)
- Add request/response handlers to your service
- Implement SOME/IP events and notifications
- Add SOME/IP-TP for large payload transfers