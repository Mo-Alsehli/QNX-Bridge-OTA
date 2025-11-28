# CommonAPI C++ Deep-Dive Guide (Advanced Topics & Troubleshooting)

## Configuration File (commonapi.ini)

### Search Order

1. Executable directory
2. `COMMONAPI_CONFIG` env variable
3. `/etc/commonapi.ini`

### Sections

```ini
[logging]
console=true
level=info

[default]
binding=dbus

[proxy]
local:com.example.Calculator:instance1=libCalc-DBus.so
```

---

## Deployment Files (.fdepl)

### Example

```fdepl
define org.genivi.commonapi.core.deployment for interface com.example.Calculator {
    DefaultEnumBackingType = UInt8
    method add { Timeout = 3000 }
}
```

---

## Namespaces & Versioning

Franca:

```fidl
version { major 1 minor 0 }
```

C++:

```
namespace v1_0::com::example
```

---

## Managed Interfaces

Supports:

* dynamic service registration
* runtime discovery
* availability callbacks

---

## Advanced Data Types

### Unions

```cpp
if (value.isType<int32_t>())
    value.get<int32_t>();
```

### Polymorphic Structs

```cpp
auto car = std::make_shared<Car>();
proxy->addVehicle(car);
```

---

## Mainloop Integration

### GLib Example

```cpp
g_timeout_add(10, dispatchCommonAPI, context.get());
```

---

## Logging Options

Levels:

* fatal
* error
* warning
* info
* debug
* verbose

---

## Threading

* each **connection ID** → separate thread
* callbacks run in receiver threads

---

## Troubleshooting

### 1. Service Not Available

* wrong domain/instance
* wrong library path
* missing binding

### 2. REMOTE_ERROR

* timeout
* service crash
* incorrect binding

Increase timeout:

```cpp
CommonAPI::CallInfo info(10000);
```

### 3. Libraries Not Found

```bash
export LD_LIBRARY_PATH=...
```

### 4. Code Generation Errors

* invalid fidl syntax
* missing imports

Use verbose mode:

```bash
commonapi_generator -ll verbose file.fidl
```

### 5. Callback Not Triggered

* program exits too early
* missing event loop

---

