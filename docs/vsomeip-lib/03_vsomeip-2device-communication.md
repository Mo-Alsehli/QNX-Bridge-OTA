# 1. The Example Applications – What We’re Running

We have two applications, you can get them from [vsomeip-example-local](https://github.com/Mo-Alsehli/QNX-Bridge-OTA/tree/master/vsomeip_example_local):

* **Service** (`service-example.cpp`)

  * `create_application("World")`
  * Offers service:

    * `SERVICE_ID = 0x1234`
    * `INSTANCE_ID = 0x5678`
    * `METHOD_ID = 0x0421`
  * Registers a message handler, prints the received string, and sends a response.

* **Client** (`client-example.cpp`)

  * `create_application("Hello")`
  * Registers **availability callback** for `(0x1234, 0x5678)`
  * When service is available, it:

    * Creates a request message with method `0x0421`
    * Sends a payload string like `"NOK"`
    * Waits for a response and prints it.

Both are built by your `CMakeLists.txt`:

```cmake
cmake_minimum_required (VERSION 3.13)

set (CMAKE_CXX_FLAGS "-g -std=c++0x")

find_package(vsomeip3)
find_package(Boost 1.55 COMPONENTS system thread log REQUIRED)

include_directories(
    ${Boost_INCLUDE_DIR}
    ${VSOMEIP_INCLUDE_DIRS}   # (typo; should be VSOMEIP_INCLUDE_DIRS)
)

add_executable(service-example server/src/service-example.cpp)
target_link_libraries(service-example vsomeip3 ${Boost_LIBRARIES})

add_executable(client-example client/src/client-example.cpp)
target_link_libraries(client-example vsomeip3 ${Boost_LIBRARIES})
```

---

# 2. Running Both Apps on the Same Ubuntu Machine (IPC)

### 2.1. Why you *don’t* need custom JSON here

After you installed `vsomeip` on Ubuntu using:

```bash
cmake -Bbuild -DCMAKE_INSTALL_PREFIX=../install_folder -DENABLE_SIGNAL_HANDLING=1 .
cmake --build build --target install
```

it installed default configs from `config/` into:

```text
../install_folder/etc/vsomeip/
```

Typically these include:

* `vsomeip.json`
* `vsomeip-local.json`
* `vsomeip-udp-client.json`
* `vsomeip-udp-service.json`
* etc.

Those default configs already define generic applications and basic routing suitable for **local testing**. For IPC on the same machine you can:

* Use `unicast = 127.0.0.1`
* Use auto-generated client IDs (no uniqueness problem, because there’s only one device)
* Use `vsomeip-local.json` as a generic local configuration.

So for **same-machine testing**, you can simply re-use the **default config file** instead of writing your own.

---

### 2.2. Build the examples on Ubuntu

Let’s assume the structure:

```text
vsomeip-examples/
├── CMakeLists.txt
├── server/src/service-example.cpp
├── client/src/client-example.cpp
```

Build:

```bash
cd vsomeip-examples
mkdir -p build
cd build

# Important: tell CMake where vsomeip is installed, if needed
cmake -DCMAKE_PREFIX_PATH=/full/path/to/install_folder ..

make -j$(nproc)
```

You should now have:

```text
build/service-example
build/client-example
```

---

### 2.3. Run the service using the default local config

From `vsomeip-examples/build`:

```bash
# Use the default vsomeip local configuration (from your earlier install)
export VSOMEIP_CONFIGURATION=/full/path/to/install_folder/etc/vsomeip/vsomeip-local.json

# Set the application name so vsomeip matches it in the JSON
export VSOMEIP_APPLICATION_NAME=World

./service-example
```

Notes:

* `VSOMEIP_CONFIGURATION` points to **an existing default JSON** installed with vsomeip.
* The `applications` section inside that JSON must contain `"name": "World"` or you add it later when you customize configs. For pure local testing, vsomeip’s auto-configuration can still assign IDs when not set – on **one** host that’s okay.

You should see logs like:

* Routing manager creation
* Service offered: `0x1234.0x5678`

---

### 2.4. Run the client on the same machine

In another terminal:

```bash
cd vsomeip-examples/build

export VSOMEIP_CONFIGURATION=/full/path/to/install_folder/etc/vsomeip/vsomeip-local.json
export VSOMEIP_APPLICATION_NAME=Hello

./client-example
```

Expected behaviour:

* Client logs *“service available”* callback for `(0x1234, 0x5678)`
* Sends request with method `0x0421`
* Service prints something like:
  `SERVER: Received: Hello from client`
* Client prints the response string.

At this point you’ve verified:

* vsomeip is correctly installed
* IPC via SOME/IP works
* No custom JSON was needed – you used the **default configs**.

---

# 3. Running Between Ubuntu (Service) and Raspberry Pi (Client)

Now we move to **two devices**. Here, **auto-config is not enough anymore**:

> If you don’t set explicit client IDs, both devices will auto-assign `client_id = 1`, and the communication may not work. That’s why the guide insists on setting IDs in `applications` when you span multiple devices.

You already have these JSON files (simplified):

### 3.1. `service.json` for Ubuntu

```json
{
    "unicast" : "192.168.10.1",

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

### 3.2. `client.json` for Raspberry Pi

```json
{
    "unicast" : "192.168.10.2",

    "logging" : {
        "level" : "debug",
        "console" : "true"
    },

    "applications" : [
        {
            "name" : "Hello",
            "id" : "0x1313"
        }
    ],

    "routing" : "Hello",

    "service-discovery" : {
        "enable" : "true",
        "multicast" : "224.224.224.245",
        "port" : "30490",
        "protocol" : "udp"
    }
}
```

Key design points (straight from the SOME/IP + vsomeip docs you quoted):

* `unicast` = each device’s actual IP.
* `id` in `"applications"`:

  * `0x1212` for `"World"` (service)
  * `0x1313` for `"Hello"` (client)
    Must be **unique across the network**.
* `services` section on the **service** defines how to reach it:

  * `unreliable: 30509` → UDP port 30509 (SOME/IP over UDP).
* `routing` says which application hosts the routing manager on that ECU.
* `service-discovery` is enabled and identical on both devices:

  * same multicast group `224.224.224.245`
  * same port `30490`
  * protocol `udp`

---

### 3.3. Network and Multicast Setup

Assume:

* Ubuntu (service): `192.168.10.1`
* Raspberry Pi (client): `192.168.10.2`
* They are directly connected or on the same LAN.

On **each device**, make sure multicast route exists (example with `eth0`):

```bash
sudo ip route add 224.224.224.245 dev eth0
```

(Use your actual interface name: `ip a` to check, could be `enp3s0`, `eth0`, etc.)

---

### 3.4. Run the service on Ubuntu with `service.json`

1. Put `service.json` somewhere, e.g.:

   ```bash
   mkdir -p ~/vsomeip-cfg
   cp service.json ~/vsomeip-cfg/
   ```

2. Run:

   ```bash
   cd vsomeip-examples/build

   export VSOMEIP_CONFIGURATION=$HOME/vsomeip-cfg/service.json
   export VSOMEIP_APPLICATION_NAME=World

   ./service-example
   ```

You should see:

* vsomeip routing manager attaching to `World`
* SD offers over multicast for `0x1234/0x5678`
* A log that UDP port 30509 is being used.

---

### 3.5. Build and run the client on Raspberry Pi (Yocto)

You already have:

* **vsomeip library** integrated into your Yocto image via `vsomeip_git.bb`:

  * It installs libs into `/usr/lib`
  * Headers into `/usr/include/vsomeip`
  * JSON configs into `/usr/etc/vsomeip` (because `${prefix}= /usr` in Yocto)

Now we add a **Yocto recipe for the example applications**.

## 4. Building the Example Client/Service with Yocto

### 4.1. Import the example project with devtool (recommended workflow)

From your Yocto build environment:

```bash
source oe-init-build-env

devtool add vsomeip-examples git://<your-git-host>/vsomeip-examples.git
```

This creates:

* `workspace/sources/vsomeip-examples/` – your app source
* `workspace/recipes/vsomeip-examples/vsomeip-examples_git.bb` – initial recipe

**OR you can get the client code from `meta-ota`**
> meta-ota/recipes-core/client-example/files

You then edit that recipe to:

* Depend on `vsomeip` and `boost`
* Use CMake
* Install the binaries and JSON configs properly

### 4.2. A suitable application recipe (aligned with vsomeip_git.bb style)

Here’s a detailed `vsomeip-examples_git.bb` that matches the pattern you used for the library recipe:

```bitbake
DESCRIPTION = "vsomeip request/response example (service + client)"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=<your-md5>"

SRC_URI = "git://<your-git-host>/vsomeip-examples.git;protocol=https;branch=main"

PV = "1.0+git${SRCPV}"
SRCREV = "<your-commit-id>"

S = "${WORKDIR}/git"

DEPENDS = "vsomeip boost"

inherit cmake pkgconfig

# If vsomeip is not in a standard CMake prefix, you can help CMake:
# EXTRA_OECMAKE = "-DCMAKE_PREFIX_PATH=${STAGING_DIR_HOST}${prefix}"

do_install:append() {
    # Install example binaries (CMake usually already installs them to ${bindir},
    # but you can be explicit if needed).
    install -d ${D}${bindir}
    install -m 0755 ${B}/service-example ${D}${bindir}/
    install -m 0755 ${B}/client-example ${D}${bindir}/

    # Install your dedicated runtime configs for the two-device setup
    install -d ${D}${prefix}/etc/vsomeip
    install -m 0644 ${S}/cfg/service.json ${D}${prefix}/etc/vsomeip/service.json
    install -m 0644 ${S}/cfg/client.json  ${D}${prefix}/etc/vsomeip/client.json
}

FILES:${PN} += " \
    ${bindir}/service-example \
    ${bindir}/client-example \
    ${prefix}/etc/vsomeip/service.json \
    ${prefix}/etc/vsomeip/client.json \
"
```

Why this fits your journey:

* It mirrors the **vsomeip_git.bb** pattern you already used:

  * `DEPENDS = "systemd boost"` there → here `DEPENDS = "vsomeip boost"`.
  * You used `do_install:append()` to copy JSON configs to `${prefix}/etc/vsomeip` and fixed QA by adding them to `FILES:${PN}`. We do the **same** for the application configs.
* It lets your Yocto image end up with:

  * Binaries: `/usr/bin/service-example`, `/usr/bin/client-example`
  * Configs: `/usr/etc/vsomeip/service.json`, `/usr/etc/vsomeip/client.json`

### 4.3. Add the examples to your image

In your image recipe or `local.conf`:

```bitbake
IMAGE_INSTALL:append = " vsomeip-examples"
```

Rebuild:

```bash
bitbake <your-image-name>
```

Flash and boot the Raspberry Pi.

---

### 4.4. Running the client on the Raspberry Pi with Yocto

Your target filesystem now contains:

* `/usr/bin/client-example`
* `/usr/etc/vsomeip/client.json`
* `/usr/lib/libvsomeip3.so ...` (from the vsomeip recipe)

You have two options:

#### Option 1 – Use the installed client.json directly

```bash
export VSOMEIP_CONFIGURATION=/usr/etc/vsomeip/client.json
export VSOMEIP_APPLICATION_NAME=Hello

client-example
```

#### Option 2 – Copy it somewhere else and point to it

```bash
mkdir -p /home/root/vsomeip-cfg
cp /usr/etc/vsomeip/client.json /home/root/vsomeip-cfg/

export VSOMEIP_CONFIGURATION=/home/root/vsomeip-cfg/client.json
export VSOMEIP_APPLICATION_NAME=Hello

client-example
```

On Ubuntu (service side) you’re still running:

```bash
export VSOMEIP_CONFIGURATION=$HOME/vsomeip-cfg/service.json
export VSOMEIP_APPLICATION_NAME=World

./service-example
```

As long as:

* IPs in `unicast` are correct
* SD multicast is configured on both sides
* `service` / `instance` / `method` IDs match your C++ constants
# **vsomeip Request/Response – Full Practical Guide**

**(Local IPC on Ubuntu → Cross-Device Ubuntu ↔ Raspberry Pi → Yocto Build Instructions)**

---

# **1. Example Applications – What Exactly We Are Running**

Your project contains two C++ applications demonstrating a **SOME/IP Request/Response** workflow.

Both applications come from:
`vsomeip_example_local/`

---

## **1.1 Service Application (`service-example.cpp`)**

**Application Name:** `"World"`

**Exports / Offers:**

* `SERVICE_ID  = 0x1234`
* `INSTANCE_ID = 0x5678`
* `METHOD_ID   = 0x0421`

**Behaviour:**

1. Creates a vsomeip app named `"World"`.
2. Offers service `0x1234/0x5678`.
3. Registers a callback for method `0x0421`.
4. Prints the received message.
5. Sends back a response message to the client.

---

## **1.2 Client Application (`client-example.cpp`)**

**Application Name:** `"Hello"`

**Behaviour:**

1. Creates a vsomeip app named `"Hello"`.
2. Registers availability callback for service `0x1234/0x5678`.
3. Once service becomes available:

   * Creates a request message using `METHOD_ID = 0x0421`.
   * Sends payload (e.g., `"NOK"`).
   * Waits for the response and prints it.

---

## **1.3 Build System (`CMakeLists.txt`)**

```cmake
cmake_minimum_required (VERSION 3.13)

set (CMAKE_CXX_FLAGS "-g -std=c++0x")

find_package(vsomeip3)
find_package(Boost 1.55 COMPONENTS system thread log REQUIRED)

include_directories(
    ${Boost_INCLUDE_DIR}
    ${VSOMEIP_INCLUDE_DIRS}
)

add_executable(service-example server/src/service-example.cpp)
target_link_libraries(service-example vsomeip3 ${Boost_LIBRARIES})

add_executable(client-example client/src/client-example.cpp)
target_link_libraries(client-example vsomeip3 ${Boost_LIBRARIES})
```

---

# **2. Running Both Applications on the Same Ubuntu Machine (Local IPC)**

This part focuses on running the two applications **on the same host**, without writing any JSON configuration.

---

## **2.1 Why You *Don’t* Need JSON Config for Local Testing**

When you installed vsomeip using:

```bash
cmake -Bbuild -DCMAKE_INSTALL_PREFIX=../install_folder -DENABLE_SIGNAL_HANDLING=1 .
cmake --build build --target install
```

vsomeip automatically installed **default configuration files** (inside `config/`) to:

```
../install_folder/etc/vsomeip/
```

These include:

* `vsomeip.json`
* `vsomeip-local.json`
* `vsomeip-udp-service.json`
* `vsomeip-udp-client.json`

These defaults handle:

* Local routing manager
* Local unicast = `127.0.0.1`
* Auto-generated client IDs
* Working IPC without needing any custom configuration

So **for local-only IPC**, you simply load the default config and run.

---

## **2.2 Building the Examples on Ubuntu**

Project structure:

```
vsomeip-examples/
├── CMakeLists.txt
├── server/src/service-example.cpp
└── client/src/client-example.cpp
```

Build:

```bash
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH=/path/to/install_folder ..
make -j$(nproc)
```

Output:

```
build/service-example  
build/client-example
```

---

## **2.3 Running the Service Locally**

```bash
export VSOMEIP_CONFIGURATION=/path/to/install_folder/etc/vsomeip/vsomeip-local.json
export VSOMEIP_APPLICATION_NAME=World

./service-example
```

Expected output includes:

* Routing manager creation
* Offer service `0x1234/0x5678`
* Ready to handle requests

---

## **2.4 Running the Client Locally**

```bash
export VSOMEIP_CONFIGURATION=/path/to/install_folder/etc/vsomeip/vsomeip-local.json
export VSOMEIP_APPLICATION_NAME=Hello

./client-example
```

Expected behaviour:

* Client discovers service
* Sends request (`METHOD_ID = 0x0421`)
* Service prints received message
* Client prints response

**At this point: IPC is verified and everything works without any JSON editing.**

---

# **3. Running Between Ubuntu (Service) and Raspberry Pi (Client)**

Now we move to **cross-device** communication.
Here, JSON configuration becomes **mandatory**.

---

## **3.1 Key Concept: Why JSON Is Required Across Devices**

When multiple ECUs are on the network:

* Each ECU must have a **unique `client_id`**
* You must specify the **unicast IP**
* You must define **routing** and **service-discovery multicast**

Without this:

* Both devices will use `client_id = 1` (default)
* SD announcements will not match
* Requests will not be routed correctly

---

## **3.2 JSON Configuration for Ubuntu (Service)**

`service.json`:

```json
{
    "unicast": "192.168.10.1",

    "logging": {
        "level": "debug",
        "console": "true"
    },

    "applications": [
        { "name": "World", "id": "0x1212" }
    ],

    "services": [
        {
            "service": "0x1234",
            "instance": "0x5678",
            "unreliable": "30509"
        }
    ],

    "routing": "World",

    "service-discovery": {
        "enable": "true",
        "multicast": "224.224.224.245",
        "port": "30490",
        "protocol": "udp"
    }
}
```

---

## **3.3 JSON Configuration for Raspberry Pi (Client)**

`client.json`:

```json
{
    "unicast": "192.168.10.2",

    "logging": {
        "level": "debug",
        "console": "true"
    },

    "applications": [
        { "name": "Hello", "id": "0x1313" }
    ],

    "routing": "Hello",

    "service-discovery": {
        "enable": "true",
        "multicast": "224.224.224.245",
        "port": "30490",
        "protocol": "udp"
    }
}
```

---

## **3.4 Multicast Route Configuration**

On **each** device:

```bash
sudo ip route add 224.224.224.245 dev eth0
```

Replace `eth0` with your actual interface.

---

## **3.5 Running the Service on Ubuntu**

```bash
export VSOMEIP_CONFIGURATION=/home/user/vsomeip-cfg/service.json
export VSOMEIP_APPLICATION_NAME=World
./service-example
```

You should see:

* SD multicast offers
* UDP socket port opened (30509)
* Routing manager active

---

## **3.6 Running the Client on Raspberry Pi**

After installing the client with Yocto (next section):

```bash
export VSOMEIP_CONFIGURATION=/usr/etc/vsomeip/client.json
export VSOMEIP_APPLICATION_NAME=Hello
client-example
```

Expected:

* Client receives SD offer from Ubuntu
* Sends request
* Receives response

---

# **4. Building the Example Client with Yocto (Raspberry Pi)**

This step integrates your example source into Yocto.

---

## **4.1 Adding the Project with devtool**

```bash
source oe-init-build-env
devtool add vsomeip-examples git://<your_repo>/vsomeip-examples.git
```

This creates:

* `workspace/sources/vsomeip-examples`
* `workspace/recipes/vsomeip-examples/vsomeip-examples_git.bb`

---

## **4.2 A Proper Yocto Recipe for the Examples**

```bitbake
DESCRIPTION = "vsomeip request/response example"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=<md5>"

SRC_URI = "git://<your_repo>/vsomeip-examples.git;protocol=https;branch=main"

PV = "1.0+git${SRCPV}"
SRCREV = "<commit-id>"

S = "${WORKDIR}/git"

DEPENDS = "vsomeip boost"

inherit cmake pkgconfig

do_install:append() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/service-example ${D}${bindir}/
    install -m 0755 ${B}/client-example ${D}${bindir}/

    install -d ${D}${prefix}/etc/vsomeip
    install -m 0644 ${S}/cfg/service.json ${D}${prefix}/etc/vsomeip/
    install -m 0644 ${S}/cfg/client.json  ${D}${prefix}/etc/vsomeip/
}

FILES:${PN} += " \
    ${bindir}/service-example \
    ${bindir}/client-example \
    ${prefix}/etc/vsomeip/service.json \
    ${prefix}/etc/vsomeip/client.json \
"
```

This recipe matches your previous vsomeip recipe style (installing to `usr/etc`).

---

## **4.3 Add Package to Image**

In your image recipe or local.conf:

```
IMAGE_INSTALL:append = " vsomeip-examples"
```

---

## **4.4 Running on Raspberry Pi**

```bash
export VSOMEIP_CONFIGURATION=/usr/etc/vsomeip/client.json
export VSOMEIP_APPLICATION_NAME=Hello

client-example
```

---

# **5. Final Communication Flow Summary**

| Step  | Description                                                     |
| ----- | --------------------------------------------------------------- |
| **1** | Build both apps locally on Ubuntu                               |
| **2** | Run both using default local config (no JSON needed)            |
| **3** | Create `service.json` and `client.json`                         |
| **4** | Configure multicast route                                       |
| **5** | Run service on Ubuntu with its JSON                             |
| **6** | Build client with Yocto, install JSON inside `/usr/etc/vsomeip` |
| **7** | Run client on Raspberry Pi                                      |
| **8** | Client discovers Ubuntu → sends request → receives response     |
