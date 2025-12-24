# QNX-Bridge-OTA

**Over-The-Air (OTA) Software Update System using CommonAPI & SOME/IP**

---

## 1. Project Overview

This repository contains an **end-to-end Over-The-Air (OTA) software update system** designed for **heterogeneous embedded platforms**, specifically:

* **Host PC (Ubuntu Linux)** – development, image preparation, and testing
* **QNX (Virtual Machine)** – OTA gateway and integrity validation
* **Raspberry Pi (Embedded Linux / Yocto)** – update target
* **Qt6 GUI** – monitoring and control interface (non-critical)

The project demonstrates how **CommonAPI over SOME/IP** can be used to implement a **service-oriented OTA update pipeline**, where a QNX-based system acts as a **bridge** between a Linux host and an embedded Linux target.

The focus of this project is **technical correctness, architecture clarity, and protocol-level understanding**, not productization or marketing.

---

## 3. Repository Structure (What Lives Where)

This repository intentionally contains **multiple sub-projects**, reflecting the real development process of embedded OTA systems.

### 3.1 Core OTA Implementations

#### `CommonAPI-QNX-OTA/`

**Primary OTA implementation (QNX ↔ Linux)**

* File transfer service and client using **CommonAPI + SOME/IP**
* OTA logic:

  * Version comparison
  * Chunked file transfer
  * Update metadata exchange
* Used as the **main reference implementation**

Key contents:

* `src/` – FileTransfer client & server logic
* `fidl/` – Interface definitions
* `src-gen/` – Generated CommonAPI/SOME-IP code
* `vsomeip.json` – SOME/IP configuration

---

#### `QNX-Ota-Server/`

**Minimal QNX-side OTA server**

* Isolated QNX service implementation
* Useful for:

  * Deployment testing
  * Performance validation
  * Debugging without full client stack

---

### 3.2 Ubuntu & Raspberry Pi Experiments

#### `CommonAPI-FileTF-ubuntu/`

* Ubuntu-based build of the file transfer service/client
* Used for:

  * Rapid iteration
  * Debugging without QNX constraints

#### `vsomeip_rpi_ubuntu/`

* SOME/IP examples compiled for Raspberry Pi (Ubuntu-based)
* Used to validate:

  * Network behavior
  * Service discovery
  * Message transport

---

### 3.3 GUI (Monitoring & Control)

#### `GUI/`

**Qt6-based OTA monitoring interface**

* Card-based dashboard UI
* Displays:

  * Update availability
  * Download progress
  * System status
* Communicates with backend via CommonAPI/SOME-IP
* **Not required for OTA functionality** (non-critical component)

The GUI is intentionally **decoupled from OTA logic**.

---

### 3.4 Documentation

#### `docs/`

This folder contains **structured technical documentation**, including:

* **CommonAPI**

  * Installation
  * Code generation
  * Ubuntu & QNX usage
* **SOME/IP**

  * Protocol overview
  * Message format
  * Transport bindings
* **Service Discovery**

  * AUTOSAR SD concepts
  * Message types
* **System Architecture**

  * Component diagrams
  * Deployment diagrams
  * Sequence diagrams
* **System Requirements**

  * Functional requirements
  * Architectural constraints

This documentation is meant to support **learning, review, and examination**, not just usage.

---

### 3.5 Supporting & Experimental Code

* `HelloCommonAPI-ubuntu/`, `commonapi-server-qnx/`

  * Early CommonAPI experiments
  * Used to validate toolchains and runtimes
* `vsomeip_example_local/`, `vsomeip-server-qnx/`

  * SOME/IP reference examples
* `yocto-meta-layers/`

  * Yocto layers for Raspberry Pi images
  * Kernel modules, services, and OTA-related recipes

These directories reflect **incremental development and experimentation**.

---

## 4. What Is *Not* Version-Controlled

The following are intentionally excluded via `.gitignore`:

* OTA images (`*.wic`, `*.iso`, `*.img`)
* Runtime `data/` directories
* Build artifacts
* Logs

These are **generated artifacts**, transferred dynamically during OTA execution.
