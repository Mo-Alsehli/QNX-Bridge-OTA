# CommonAPI C++ (Core + SOME/IP) — Installation Guide for Ubuntu 24.04  
**Based on a fully working, tested setup**

This guide explains how to install:

- **CommonAPI Core Runtime (3.2.4)**
- **CommonAPI SOME/IP Runtime (3.2.4)**
- **CommonAPI Core & SOME/IP Generators (3.2.15)**
- **vsomeip 3.5.10**

Everything will be installed into a clean, isolated directory:

```

/opt/commonapi/
├── runtime/      ← CommonAPI Core + SOME/IP runtime
├── vsomeip/      ← vsomeip 3.5.10 installation
└── generators/   ← core + someip code generators

```

All steps are validated on **Ubuntu 24.04**, including required patches.

---

# 📌 1. Prerequisites

Install required packages:

```bash
sudo apt update
sudo apt install -y git cmake g++ default-jre libexpat-dev libboost-all-dev
```

Verify Java 8+ exists:

```bash
java -version
```

---

# 📌 2. Prepare folders

```bash
sudo mkdir -p /opt/commonapi/{runtime,vsomeip,generators}
sudo chown -R $USER:$USER /opt/commonapi
```

---

# 📌 3. Install vsomeip 3.5.10

Assume you already cloned and built it.

Move the working installation:

```bash
mv ~/workspace/OJTBrightskies_Embedded_Linux/vsomeip_3.5.10 /opt/commonapi/vsomeip
```

Ensure structure:

```
/opt/commonapi/vsomeip/
    ├── bin
    ├── etc
    ├── include
    └── lib
```

---

# 📌 4. Build & Install CommonAPI Core Runtime (3.2.4)

Clone source:

```bash
git clone https://github.com/GENIVI/capicxx-core-runtime.git
cd capicxx-core-runtime
git checkout 3.2.4
```

❗ **Fix Ubuntu 24 build error**

Edit:

```
include/CommonAPI/Types.hpp
```

Add at the top:

```cpp
#include <string>
```

Now build:

```bash
cmake -Bbuild -DCMAKE_INSTALL_PREFIX=/opt/commonapi/runtime .
cmake --build build --target install -j$(nproc)
```

---

# 📌 5. Build & Install CommonAPI SOME/IP Runtime (3.2.4)

Clone:

```bash
cd ..
git clone https://github.com/GENIVI/capicxx-someip-runtime.git
cd capicxx-someip-runtime
git checkout 3.2.4
```

Build:

```bash
cmake -Bbuild \
    -DCMAKE_INSTALL_PREFIX=/opt/commonapi/runtime \
    -DCMAKE_PREFIX_PATH="/opt/commonapi/runtime;/opt/commonapi/vsomeip" \
    -DUSE_INSTALLED_COMMONAPI=OFF .

cmake --build build --target install -j$(nproc)
```

This installs:

```
libCommonAPI-SomeIP.so
CommonAPI-SomeIP includes + cmake files
```

---

# 📌 6. Install Code Generators (Core + SOME/IP)

Download latest generators (3.2.15):

```bash
cd /opt/commonapi/generators

# Core
wget https://github.com/GENIVI/capicxx-core-tools/releases/download/3.2.15/commonapi_core_generator.zip
unzip commonapi_core_generator.zip -d core

# SOME/IP
wget https://github.com/GENIVI/capicxx-someip-tools/releases/download/3.2.15/commonapi_someip_generator.zip
unzip commonapi_someip_generator.zip -d someip
```
> OR: you can simply download the binaries from the repo directly from the releases section

Make generators executable:

```bash
chmod +x core/commonapi-core-generator-linux-x86_64
chmod +x someip/commonapi-someip-generator-linux-x86_64
```

Generators directory now contains:

```
/opt/commonapi/generators/core/commonapi-core-generator-linux-x86_64
/opt/commonapi/generators/someip/commonapi-someip-generator-linux-x86_64
```

This is correct.

---

# 📌 7. Environment Variables (optional)

You can export these to avoid long commands:

```bash
export COMMONAPI_PREFIX=/opt/commonapi
export LD_LIBRARY_PATH=$COMMONAPI_PREFIX/runtime/lib:$COMMONAPI_PREFIX/vsomeip/lib:$LD_LIBRARY_PATH
export PATH=$COMMONAPI_PREFIX/generators/core:$COMMONAPI_PREFIX/generators/someip:$PATH
```

---

# 📌 8. Verify Installation

### Check CommonAPI:

```bash
ls /opt/commonapi/runtime/lib | grep CommonAPI
```

You should see:

```
libCommonAPI.so
libCommonAPI-SomeIP.so
```

### Check vsomeip:

```bash
ls /opt/commonapi/vsomeip/lib | grep vsomeip
```

### Check generators:

```bash
/opt/commonapi/generators/core/commonapi-core-generator-linux-x86_64 -h
/opt/commonapi/generators/someip/commonapi-someip-generator-linux-x86_64 -h
```

---

# 📌 9. Final Folder Layout (Exactly as in this working setup)

```
/opt/commonapi
├── runtime
│   ├── lib
│   │   ├── libCommonAPI.so
│   │   └── libCommonAPI-SomeIP.so
│   ├── include
│   └── cmake
│
├── vsomeip
│   ├── bin
│   ├── etc
│   ├── lib
│   └── include
│
└── generators
    ├── core
    │   ├── commonapi-core-generator-linux-x86_64
    │   └── plugins/featured files...
    └── someip
        ├── commonapi-someip-generator-linux-x86_64
        └── plugins/featured files...
```

---

# 📌 10. Your Installation Is Now Fully Complete

You now have everything required to:

* Write FIDL interfaces
* Write SOME/IP deployment files
* Generate proxy + stub code
* Build a SOME/IP service/client
* Run on Ubuntu 24.04 with vsomeip 3.5.10

