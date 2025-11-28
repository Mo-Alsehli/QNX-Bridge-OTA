# SOME/IP Service Application on QNX

*(vsomeip3-based “World” service)*

## 1. Purpose

This document explains how to build and run a **SOME/IP service** on **QNX** using **vsomeip3**.

The service:

* Runs on your **QNX VM** at IP `192.168.10.3`.
* Offers a SOME/IP service `(0x1234, 0x5678)`.
* Listens for method `0x0421`.
* Echoes a string payload and appends: `" -> who is there?"`.

This is the **QNX-side SOME/IP server** that will eventually be part of your OTA bridge (QNX ↔ Raspberry Pi).

---

## 2. Prerequisites

### 2.1 QNX SDP Environment (on Ubuntu host)

Load the environment on your Ubuntu host:

```bash
source /opt/qnx800/qnxsdp-env.sh
```

This must be done in **every shell** before building so that:

* `QNX_HOST` and `QNX_TARGET` are set correctly.
* `qcc` is in `PATH`.
* CMake finds the QNX sysroot.

Check:

```bash
which qcc
echo $QNX_TARGET
```

### 2.2 vsomeip3 for QNX

You must have vsomeip3 built for QNX and installed into:

* `$QNX_TARGET/x86_64/usr/local/lib/`
* `$QNX_TARGET/x86_64/usr/local/lib/cmake/vsomeip3/`
* `$QNX_TARGET/x86_64/usr/local/include/vsomeip/`

### 2.3 Boost for QNX

Boost must also be built for QNX and installed into:

```
$QNX_TARGET/x86_64/usr/local
```

Your CMake should contain:

```cmake
set(BOOST_ROOT "$ENV{QNX_TARGET}/x86_64/usr/local")
find_package(Boost REQUIRED COMPONENTS system thread log)
```

### 2.4 QNX VM Network Configuration

Your QNX VM must use:

* Static IP: `192.168.10.3`
* Netmask: `255.255.255.0`

Example QNX command:

```bash
ifconfig wm0 192.168.10.3 netmask 255.255.255.0 up
```

Multicast routing may be required:

```bash
route add -net 224.224.224.245 wm0
```

---

## 3. Project Layout

```
vsomeip_service_qnx/
 ├── CMakeLists.txt
 ├── vsomeip.json
 └── src/
     └── service-example.cpp
```

---

## 4. vsomeip.json – Configuration

```json
{
    "unicast" : "192.168.10.3",

    "logging" : {
        "level" : "debug",
        "console" : "true"
    },

    "applications" : [
        {
            "name" : "World",
            "id" : "0x1212"
        }
    ],

    "services" : [
        {
            "service" : "0x1234",
            "instance" : "0x5678",
            "unreliable" : "30509"
        }
    ],

    "routing" : "World",

    "service-discovery" : {
        "enable" : "true",
        "multicast" : "224.224.224.245",
        "port" : "30490",
        "protocol" : "udp"
    }
}
```

### Meaning of key fields

* `"unicast": "192.168.10.3"` → Local IP of the QNX device.
* `"applications"` → `name` **must match** the name used in your C++ code (`create_application("World")`).
* `"services"` → Defines the SOME/IP service `(0x1234, 0x5678)`.
* `"routing": "World"` → This application acts as the routing manager.
* `"service-discovery"` → Multicast-based discovery using standard SOME/IP SD settings.

---

## 5. Service Implementation – Explained

`src/service-example.cpp`:

```cpp
#include <iostream>
#include <vsomeip/vsomeip.hpp>

#define SAMPLE_SERVICE_ID   0x1234
#define SAMPLE_INSTANCE_ID  0x5678
#define SAMPLE_METHOD_ID    0x0421   

std::shared_ptr<vsomeip::application> app;
```

Defines service, instance, and method IDs.

### Request Handler

```cpp
void on_message(const std::shared_ptr<vsomeip::message> &req) {

    auto payload = req->get_payload();
    std::string received(reinterpret_cast<const char*>(payload->get_data()),
                         payload->get_length());

    std::cout << "SERVER: Received: " << received << std::endl;
```

Extracts string payload.

```cpp
    auto resp = vsomeip::runtime::get()->create_response(req);

    std::string answer = received + " -> who is there?";
    std::vector<vsomeip::byte_t> data(answer.begin(), answer.end());

    auto pl = vsomeip::runtime::get()->create_payload();
    pl->set_data(data);
    resp->set_payload(pl);

    app->send(resp);
}
```

Constructs and sends back the response.

### Main Application

```cpp
int main() {
    app = vsomeip::runtime::get()->create_application("World");
    app->init();
```

Loads config and vsomeip runtime.

```cpp
    app->register_message_handler(
        SAMPLE_SERVICE_ID,
        SAMPLE_INSTANCE_ID,
        SAMPLE_METHOD_ID,
        on_message);
```

Registers handler for method `0x0421`.

```cpp
    app->offer_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
    app->start();
}
```

Publishes the service and starts vsomeip threads.

---

## 6. CMake Configuration for QNX

`CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.13)
project(service-example)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VSOMEIP_DIR "$ENV{QNX_TARGET}/x86_64/usr/local/lib/cmake/vsomeip3")
find_package(vsomeip3 REQUIRED HINTS ${VSOMEIP_DIR})

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
```

The QNX **toolchain file** is passed when calling CMake (not inside `CMakeLists.txt`).

---

## 7. Building (on Ubuntu Host)

```bash
source /opt/qnx800/qnxsdp-env.sh

cd ~/workspace/OJTBrightskies_Embedded_Linux/QNX-Bridge-OTA/vsomeip_service_qnx
mkdir -p build
cd build

cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=~/qnxprojects/toolchain-qnx.cmake

cmake --build . -- -j$(nproc)
```

Output:

```
service-example
```

Check binary type:

```bash
file service-example
```

---

## 8. Deploy to QNX

From Ubuntu:

```bash
scp build/service-example root@192.168.10.3:/root/
scp ../vsomeip.json root@192.168.10.3:/root/
```

On QNX:

```bash
ls /root/service-example
ls /root/vsomeip.json
```

---

## 9. Running on QNX

### 1. Set environment variables:

```bash
export VSOMEIP_CONFIGURATION=/root/vsomeip.json
export VSOMEIP_APPLICATION_NAME=World
```

### 2. Run the service:

```bash
./service-example
```

Expected logs:

* App “World” initialized
* Service `(0x1234, 0x5678)` offered
* SD multicast traffic created

When a client sends a request:

```
SERVER: Received: hello
```

Client receives:

```
hello -> who is there?
```
