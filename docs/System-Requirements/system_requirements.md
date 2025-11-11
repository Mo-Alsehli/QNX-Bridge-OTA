# QNX-Bridge-OTA

**Over-the-Air Software Update System using SOME/IP between Linux, QNX, and Raspberry Pi**

---

## 1. Introduction

**QNX-Bridge-OTA** is a cross-platform embedded framework that enables reliable software updates to be transferred over Ethernet between heterogeneous systems.
The design uses **SOME/IP** for structured communication and **FTP** for the initial image delivery.

Main roles:

* **Host PC (Linux)** – provides the update image.
* **QNX VM** – verifies integrity and forwards the update.
* **Raspberry Pi (Linux)** – receives and installs the image.

This system demonstrates how a QNX-based gateway can coordinate and bridge Over-the-Air (OTA) updates between different platforms using automotive-grade communication standards.

---

## 2. System Architecture

```
+---------------------+      +---------------------+                    +----------------------+
| Host PC (Linux)     |      | QNX Virtual Machine |                    | Raspberry Pi (Linux) |
|---------------------|      |---------------------|                    |----------------------|
| - Holds update img  | FTP  | - Receives image    | SOME/IP (TCP/UDP)  | - Receives data      |
| - Initiates update  | ---> | - Performs CRC check| -----------------> | - Writes to flash    |
| - Sends commands    |      | - Forwards via TP   | <----------------- | - Sends feedback     |
+---------------------+      +---------------------+                    +----------------------+
```

**Communication flow**

1. The Host sends the image to QNX through **FTP**.
2. QNX performs a **CRC integrity check** on the received file.
3. QNX forwards the verified image to the Raspberry Pi using **SOME/IP** (TCP for commands, UDP-TP for data).
4. The Raspberry Pi reassembles the image, writes it to storage, and reports progress/events back to QNX.

---

## 6. Data Flow

```
[1] Host transfers image to QNX via FTP
[2] QNX validates image integrity (CRC)
[3] QNX sends StartUpdate() to Raspberry Pi
[4] QNX transmits image using SOME/IP-TP segments
[5] Raspberry Pi reassembles and verifies chunks
[6] Raspberry Pi emits events:
      - ProgressUpdate
      - BlockCRC_OK
      - UpdateComplete
[7] QNX logs status and informs Host of completion
```

---

## 10. UML Diagram

### 1. System Architecture (Component Diagram)

```
+---------------------+
|  Host PC (Linux)    |
|  - FTP Client       |
|  - SOME/IP Client   |
+----------+----------+
           |
           | FTP
           v
+----------+----------+
|  QNX VM (Bridge)    |
|  - FTP Server       |
|  - CRC Checker      |
|  - SOME/IP Server   |
|  - SOME/IP Client   |
+----------+----------+
           |
           | SOME/IP (TCP/UDP)
           v
+----------+----------+
|  Raspberry Pi        |
|  - SOME/IP Server    |
|  - Flash Manager     |
|  - Event Emitter     |
+----------------------+
```

### 2. Sequence Diagram (Update Workflow)

```
Host PC      QNX VM        Raspberry Pi
  |             |                |
  |---FTP Img-->|                |
  |             |                |
  |             |---StartUpdate->|
  |             |<--ACK----------|
  |             |---TransferChunk()--->|
  |             |<--BlockCRC_OK--------|
  |             |---FinishUpdate()---->|
  |             |<--UpdateComplete-----|
  |<----Status Report------------------|
```

