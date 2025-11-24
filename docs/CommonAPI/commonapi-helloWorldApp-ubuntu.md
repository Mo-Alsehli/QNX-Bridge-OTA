# CommonAPI C++ SOME/IP — Hello World Application Guide (Ubuntu 24.04)

This guide explains **exactly how to create**, **generate**, **build**, and **run** a full **CommonAPI + SOME/IP Hello World** application on Ubuntu 24.04 using:

- CommonAPI Core Runtime 3.2.4  
- CommonAPI SOME/IP Runtime 3.2.4  
- CommonAPI Generators 3.2.15  
- vsomeip 3.5.10  
- Installation paths under `/opt/commonapi`  
- A clean project structure  

All steps are fully tested.

---

# 📌 1. Create Project Structure

```bash
mkdir -p HelloSomeIP/{fidl,src,src-gen}
cd HelloSomeIP
```

Expected tree:

```
HelloSomeIP/
├── fidl/
├── src/
└── src-gen/
```

---

# 📌 2. Create FIDL Interface

Create `fidl/HelloWorld.fidl`:

```fidl
package commonapi.examples

interface HelloWorld {
    version { major 0 minor 1 }

    method sayHello {
        in { String name }
        out { String message }
    }
}
```

---

# 📌 3. Create SOME/IP Deployment File (FDEPL)

Create `fidl/HelloWorld.fdepl`:

```fidl
import "platform:/plugin/org.genivi.commonapi.someip/deployment/CommonAPI-4-SOMEIP_deployment_spec.fdepl"
import "HelloWorld.fidl"

define org.genivi.commonapi.someip.deployment for interface commonapi.examples.HelloWorld {
    SomeIpServiceID = 0x1234

    method sayHello {
        SomeIpMethodID = 0x0001
        SomeIpReliable = true
    }
}

define org.genivi.commonapi.someip.deployment for provider as Service {
    instance commonapi.examples.HelloWorld {
        InstanceId = "commonapi.examples.HelloWorld"
        
        SomeIpInstanceID = 0x5678
        SomeIpUnicastAddress = "127.0.0.1"
        SomeIpReliableUnicastPort = 30509
        SomeIpUnreliableUnicastPort = 30509
    }
}
```

---

# 📌 4. Generate Code (Core + SOME/IP)

```bash
/opt/commonapi/generators/core/commonapi-core-generator-linux-x86_64 \
    -d src-gen/core -sk fidl/HelloWorld.fidl

/opt/commonapi/generators/someip/commonapi-someip-generator-linux-x86_64 \
    -d src-gen/someip fidl/HelloWorld.fdepl
```

Generated structure:

```
src-gen/
├── core/v0/commonapi/examples/*.cpp|hpp
└── someip/v0/commonapi/examples/*.cpp|hpp
```

---

# 📌 5. Implement Service Stub

Create `src/HelloWorldStubImpl.hpp`:

```cpp
#pragma once
#include <v0/commonapi/examples/HelloWorldStubDefault.hpp>
#include <CommonAPI/CommonAPI.hpp>

class HelloWorldStubImpl
    : public v0::commonapi::examples::HelloWorldStubDefault {

public:
    HelloWorldStubImpl() = default;
    virtual ~HelloWorldStubImpl() = default;

    virtual void sayHello(
        const std::shared_ptr<CommonAPI::ClientId> _client,
        std::string name,
        sayHelloReply_t reply) override;
};
```

Create `src/HelloWorldStubImpl.cpp`:

```cpp
#include "HelloWorldStubImpl.hpp"
#include <iostream>
#include <sstream>

void HelloWorldStubImpl::sayHello(
        const std::shared_ptr<CommonAPI::ClientId>,
        std::string name,
        sayHelloReply_t reply) {

    std::stringstream ss;
    ss << "Hello " << name << "!";

    std::cout << "[SERVICE] sayHello('" << name << "') → '" 
              << ss.str() << "'" << std::endl;

    reply(ss.str());
}
```

---

# 📌 6. Write the Service Application

`src/HelloWorldService.cpp`:

```cpp
#include <CommonAPI/CommonAPI.hpp>
#include <iostream>
#include "HelloWorldStubImpl.hpp"

int main() {
    CommonAPI::Runtime::setProperty("LibraryBase", "HelloWorld");

    auto runtime = CommonAPI::Runtime::get();
    auto service = std::make_shared<HelloWorldStubImpl>();

    bool registered =
        runtime->registerService("local",
                                 "commonapi.examples.HelloWorld",
                                 service,
                                 "service-sample");

    if (!registered) {
        std::cerr << "Failed to register service!" << std::endl;
        return 1;
    }

    std::cout << "[SERVICE] HelloWorld Service started." << std::endl;

    while (true)
        std::this_thread::sleep_for(std::chrono::seconds(1));
}
```

---

# 📌 7. Write the Client Application

`src/HelloWorldClient.cpp`:

```cpp
#include <CommonAPI/CommonAPI.hpp>
#include <iostream>
#include <thread>
#include <v0/commonapi/examples/HelloWorldProxy.hpp>

int main() {
    CommonAPI::Runtime::setProperty("LibraryBase", "HelloWorld");

    auto runtime = CommonAPI::Runtime::get();
    auto proxy = runtime->buildProxy<v0::commonapi::examples::HelloWorldProxy>(
        "local", "commonapi.examples.HelloWorld", "client-sample");

    std::cout << "[CLIENT] Waiting for service..." << std::endl;

    while (!proxy->isAvailable())
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::cout << "[CLIENT] Service available." << std::endl;

    CommonAPI::CallStatus status;
    std::string message;

    for (;;) {
        proxy->sayHello("World", status, message);
        std::cout << "[CLIENT] Response: " << message << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
```

---

# 📌 8. Create SOME/IP Binding Config

`commonapi4someip.ini`:

```ini
[default]
binding=someip

[logging]
console=true
file=./mylog.log
dlt=false
level=verbose
```

---

# 📌 9. Write CMakeLists.txt (Modern, SOME/IP only)

```cmake
cmake_minimum_required(VERSION 3.13)
project(HelloWorld)

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

file(GLOB CORE_GEN src-gen/core/v0/commonapi/examples/*.cpp)
file(GLOB SOMEIP_GEN src-gen/someip/v0/commonapi/examples/*.cpp)

add_executable(HelloWorldService
    src/HelloWorldService.cpp
    src/HelloWorldStubImpl.cpp
    ${CORE_GEN}
    ${SOMEIP_GEN}
)

target_link_libraries(HelloWorldService
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
)

add_executable(HelloWorldClient
    src/HelloWorldClient.cpp
    ${CORE_GEN}
    ${SOMEIP_GEN}
)

target_link_libraries(HelloWorldClient
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
)
```

---

# 📌 10. Build the Application

```bash
cmake -Bbuild .
cmake --build build -j$(nproc)
```

Executables created:

```
build/HelloWorldService
build/HelloWorldClient
```

---

# 📌 11. Run the Service

```bash
COMMONAPI_CONFIG=commonapi4someip.ini \
LD_LIBRARY_PATH=/opt/commonapi/runtime/lib:/opt/commonapi/vsomeip/lib:./build \
./build/HelloWorldService
```

---

# 📌 12. Run the Client

```bash
COMMONAPI_CONFIG=commonapi4someip.ini \
LD_LIBRARY_PATH=/opt/commonapi/runtime/lib:/opt/commonapi/vsomeip/lib:./build \
./build/HelloWorldClient
```

---

# 📌 13. Expected Output

### Service:

```
[SERVICE] sayHello('World') → 'Hello World!'
```

### Client:

```
[CLIENT] Response: Hello World!
```