# CommonAPI C++ Conceptual Guide (Architecture & Core Concepts)

## Introduction

### What is CommonAPI?

CommonAPI C++ is a **middleware-independent IPC framework** that allows applications to communicate seamlessly without depending on a single middleware. It separates the business logic from communication logic using generated C++ code from Franca IDL definitions.

**Key Benefits**

* Middleware-agnostic (SOME/IP, D-Bus…)
* Type-safe IPC
* Code generation from Franca IDL
* Supports synchronous & asynchronous calls
* Built-in timeout and error handling

### Architecture Overview

```
┌──────────────────────┐
│  Application Logic   │
└──────────────────────┘
           ↓
┌──────────────────────┐
│   Generated Code     │
└──────────────────────┘
           ↓
┌──────────────────────┐
│  CommonAPI Runtime   │
└──────────────────────┘
           ↓
┌──────────────────────┐
│ Middleware Binding   │
└──────────────────────┘
           ↓
┌──────────────────────┐
│     Middleware       │
└──────────────────────┘
```

### Key Components

| Component             | Description                        |
| --------------------- | ---------------------------------- |
| **Franca IDL**        | Defines interfaces, types, methods |
| **CommonAPI Core**    | Runtime + abstraction layer        |
| **CommonAPI Binding** | SOME/IP or D-Bus support           |
| **Code Generator**    | Converts FIDL → C++                |
| **Proxy**             | Client-side representation         |
| **Stub**              | Service-side implementation        |

---

## Franca IDL (Interface Definition Language)

### Basic Interface

```fidl
package com.example.myservice

interface Calculator {
    version { major 1 minor 0 }
}
```

### Methods

```fidl
method add {
    in  { Int32 a; Int32 b; }
    out { Int32 sum; }
}
```

### Fire-and-Forget

```fidl
method logMessage fireAndForget {
    in { String message }
}
```

### Attributes

```fidl
attribute Int32 counter
attribute String status readonly
```

### Broadcasts

```fidl
broadcast statusChanged {
    out { String newStatus; Int32 code; }
}
```

### Enumerations, Structs, Arrays, Maps, Unions, Polymorphic Types

Franca supports:

* `enum`
* `struct`
* `array`
* `map`
* `union`
* polymorphic structures (`extends`, `polymorphic`)
* typeCollection for reusable types

---

## Proxies (Client Side)

### Creating a Proxy

```cpp
auto runtime = CommonAPI::Runtime::get();
auto proxy = runtime->buildProxy<CalculatorProxy>("local", "calculator.instance1");
```

### Method Calls

**Synchronous**

```cpp
CommonAPI::CallStatus status;
int32_t result;
proxy->add(5, 3, status, result);
```

**Asynchronous**

```cpp
proxy->addAsync(5, 3, [](auto status, int32_t r){});
```

### Attributes

Get:

```cpp
proxy->getCounterAttribute().getValue(status, value);
```

Set:

```cpp
proxy->getCounterAttribute().setValue(42, status);
```

Subscribe:

```cpp
proxy->getCounterAttribute().getChangedEvent().subscribe(callback);
```

### Broadcasts

```cpp
proxy->getStatusChangedEvent().subscribe(
    [](auto s, auto code){}
);
```

---

## Stubs (Service Side)

### Stub Implementation

```cpp
class CalculatorImpl : public CalculatorStubDefault {
public:
    void add(..., addReply_t reply) override {
        reply(a + b);
    }
};
```

### Registering the Service

```cpp
runtime->registerService("local", "calculator.instance1", service);
```

### Attributes in Stubs

```cpp
setCounterAttribute(42);
auto v = getCounterAttribute();
```

### Broadcasts

```cpp
fireStatusChangedEvent("Running", 200);
```

---

## CommonAPI Address Format

```
domain:interface:instance
```

Example:

```
local:com.example.Calculator:instance1
```

---

## CallStatus Reference

| Status              | Meaning               |
| ------------------- | --------------------- |
| SUCCESS             | OK                    |
| NOT_AVAILABLE       | Service not reachable |
| REMOTE_ERROR        | Timeout / crash       |
| INVALID_VALUE       | Bad input             |
| SERIALIZATION_ERROR | Marshalling failed    |

---

## Recommended Project Structure

```
project/
├── fidl/
├── src/
├── src-gen/
├── build/
├── commonapi.ini
└── CMakeLists.txt
```

---

## Development Workflow

### Step 1 — Write the FIDL

```fidl
package com.example

interface Calculator {
    method add { in {Int32 a; Int32 b;} out {Int32 result;} }
    attribute Int32 lastResult readonly
    broadcast resultCalculated { out {Int32 result} }
}
```

### Step 2 — Generate Code

```bash
commonapi_generator -dest src-gen Calculator.fidl
```

### Step 3 — Implement Service

```cpp
class CalculatorImpl : public CalculatorStubDefault {
public:
    void add(..., addReply_t reply) override {
        int r = a + b;
        setLastResultAttribute(r);
        fireResultCalculatedEvent(r);
        reply(r);
    }
};
```

### Step 4 — Implement Client

```cpp
auto proxy = runtime->buildProxy<CalculatorProxy>("local", "calculator.instance1");
proxy->add(10, 5, status, result);
```

### Step 5 — CMake Configuration

```cmake
find_package(CommonAPI REQUIRED)
include_directories(src-gen ${COMMONAPI_INCLUDE_DIRS})
add_executable(service CalculatorService.cpp src-gen/.../CalcStubDefault.cpp)
```

### Step 6 — Run

```bash
export COMMONAPI_CONFIG=../commonapi.ini
./calculator-service
./calculator-client
```

---

## Code Generator Usage

```
commonapi_generator [options] <fidl>
```

Options include:

* `-d <dir>`
* `-ns` (no stub)
* `-np` (no proxy)
* `-sk` (generate skeleton)
* `-ll verbose`

---

## Practical Examples

### Example 1 — Full Calculator Service/Client

Includes:

* basic methods
* broadcasts
* attributes
* async calls

### Example 2 — Configuration Service

Demonstrates:

* maps
* structs
* enumerations
* attribute maps

### Example 3 — Managed Interfaces

Dynamic device registration:

* manager interface
* device interface
* availability events

---

