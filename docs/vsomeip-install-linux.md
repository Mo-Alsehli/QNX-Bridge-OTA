# 📘 Installing vsomeip on Ubuntu

---

## 1. Introduction

`vsomeip` is an open-source implementation of the **SOME/IP automotive middleware** used for communication between ECUs.
This guide walks through installing `vsomeip` on Ubuntu — from prerequisites, building, installing, and verifying the environment.

---

## 2. Preparation & Prerequisites

Before building vsomeip, ensure your system has:

### ✔️ Required Packages

* CMake (≥ 3.5)
* GCC / G++ compiler
* Boost library (**≥ 1.55**)
* UUID development headers
* Git
* Make

Install all prerequisites:

```bash
sudo apt update
sudo apt install -y \
    git cmake g++ make \
    libboost-all-dev \
    uuid-dev
```

### ✔️ Why is Boost Mandatory?

vsomeip relies heavily on **Boost.Asio**, concurrency utilities, and Boost logging.
If not installed, CMake will fail during configuration.

---

## 3. Clone the vsomeip Repository

```bash
git clone https://github.com/COVESA/vsomeip.git
cd vsomeip
```

This downloads the full vsomeip source code.

---

## 4. Configure the Build

Use CMake to prepare the build:

```bash
cmake -Bbuild \
    -DCMAKE_INSTALL_PREFIX=../install_folder \
    -DENABLE_SIGNAL_HANDLING=1 \
    .
```

### Explanation of Options

| Option                                     | Meaning                                                                    |
| ------------------------------------------ | -------------------------------------------------------------------------- |
| `-Bbuild`                                  | Creates a clean build directory separate from source.                      |
| `-DCMAKE_INSTALL_PREFIX=../install_folder` | Where to install libraries, headers, binaries, and config files.           |
| `-DENABLE_SIGNAL_HANDLING=1`               | Ensures Ctrl-C cleans vsomeip shared memory segments (`/dev/shm/vsomeip`). |
| `.`                                        | Tells CMake to use the current folder as source.                           |

### Why ENABLE_SIGNAL_HANDLING?

When disabled, stopping a vsomeip application may leave stale shared memory and cause errors like:

```
vsomeip: shared memory segment already exists
```

Enabling this prevents the issue.

---

## 5. Build & Install

Run:

```bash
cmake --build build --target install
```

After installation, you will have:

```
install_folder/
 ├── bin/
 ├── lib/
 ├── include/vsomeip/
 └── etc/vsomeip/
```

Where:

* **lib/** → all vsomeip shared libraries
* **include/** → header files
* **etc/vsomeip/** → default SOME/IP JSON configuration files

---

## 6. Add vsomeip to System Library Path (Optional)

If you want the library to be globally accessible:

```bash
echo "<absolute-path-to-install_folder>/lib" | sudo tee /etc/ld.so.conf.d/vsomeip.conf
sudo ldconfig
```

Replace the path with your actual installation directory.

---

## 7. Verify the Installation

### ✔️ Check header files

```bash
ls install_folder/include/vsomeip
```

You should see files like:

```
application.hpp
runtime.hpp
message.hpp
plugin.hpp
...
```

### ✔️ Check library files

```bash
ls install_folder/lib | grep someip
```

Expected output:

```
libvsomeip3.so
libvsomeip3-cfg.so
```

### ✔️ Check config files

```bash
ls install_folder/etc/vsomeip
```

You should see:

```
vsomeip.json
vsomeip-local.json
vsomeip-tcp-service.json
...
```

---

## 8. Running Example Applications (Optional)

vsomeip provides sample client and service examples.

### Start the service:

```bash
./build/examples/simple-someip-service
```

### Start the client:

```bash
./build/examples/simple-someip-client
```

Expected behaviour:

* Service offers a SOME/IP service
* Client discovers it
* Client sends request and receives response

Successful exchange confirms that vsomeip is installed correctly.

---

## 9. Troubleshooting

### 🛑 CMake cannot find Boost

```bash
sudo apt install libboost-all-dev
```

### 🛑 Shared memory leftover errors

```bash
sudo rm -f /dev/shm/vsomeip*
```

### 🛑 Service discovery not working

Enable multicast:

```bash
sudo sysctl -w net.ipv4.ip_forward=1
sudo sysctl -w net.ipv4.conf.all.multicast=1
```

---

## 10. Summary

To install vsomeip on Ubuntu:

```bash
sudo apt install libboost-all-dev git cmake g++ make uuid-dev

git clone https://github.com/COVESA/vsomeip.git
cd vsomeip

cmake -Bbuild -DCMAKE_INSTALL_PREFIX=../install_folder -DENABLE_SIGNAL_HANDLING=1 .
cmake --build build --target install
```

vsomeip is now installed and ready for development.
