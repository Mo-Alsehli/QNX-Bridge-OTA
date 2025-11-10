# SOME/IP – Communication Types

---

## 🧠 Overview

SOME/IP provides multiple **communication paradigms** tailored for different use cases in automotive systems.  
It supports both **synchronous (Request/Response)** and **asynchronous (Event/Fire & Forget)** exchanges between ECUs, allowing flexibility for **real-time**, **diagnostic**, and **OTA** communication.

---

## ⚙️ 4.2.2 Request/Response Communication

### 🔹 Concept

This is the **core Remote Procedure Call (RPC)** mechanism in SOME/IP.

**Flow:**

```
Client  ── Request ──▶  Server
Client  ◀─ Response ──  Server
```

### 🔹 Message Types

|Type|Hex|Direction|Description|
|---|---|---|---|
|**Request**|`0x00`|Client → Server|Standard request expecting a response.|
|**RequestNoReturn**|`0x01`|Client → Server|“Fire & forget” — no response expected.|
|**Response**|`0x80`|Server → Client|Normal RPC response.|
|**Error**|`0x81`|Server → Client|Error during request processing.|

### 🔹 Identification

Each Request/Response pair is matched using:

```
Request ID = Client ID (16 bits) + Session ID (16 bits)
```

This allows multiple concurrent or asynchronous requests to be tracked correctly.

### 🔹 In OTA Context

Used for all **command–response** operations between devices:

|Function|Description|
|---|---|
|`StartUpdate()`|Initiates update session.|
|`CheckCRC()`|Verifies data integrity.|
|`TransferChunk()`|Sends firmware data blocks.|
|`FinishUpdate()`|Completes update and validates.|

**Transport:** Typically TCP (reliable and ordered).  
**Message flow:** Deterministic request-response, ideal for transactional control.

---

## 🔔 4.2.3 Fire & Forget

### 🔹 Concept

A **one-way request** — the client sends a message **without expecting a reply**.

**Flow:**

```
Client ──▶ Server
(no response)
```

Used when:

- The action is **non-critical**.
    
- Response confirmation is unnecessary.
    
- Network load should be minimized.
    

**Examples:**

- Heartbeat signals (`Alive` messages).
    
- Simple progress indicators (`ChunkSent`, `ProcessingStarted`).
    

**In OTA:**  
Could be used for quick status notifications or diagnostic heartbeats during large transfers.

---

## 📡 4.2.4 Notification Events

### 🔹 Concept

A **publish/subscribe** mechanism where a **provider (server)** sends updates to **subscribers (clients)** automatically.

**Flow:**

```
Server (Provider) ──▶ Multiple Clients (Subscribers)
```

- Based on **Events** defined within a service.
    
- Can use **UDP multicast** or **TCP unicast**, depending on configuration.
    
- Events are grouped into **Event Groups** for efficient subscription and management.
    

### 🔹 Characteristics

|Property|Description|
|---|---|
|**Provider → Subscriber**|One-way communication.|
|**Cyclic or On-change**|Data sent periodically or when updated.|
|**Transport**|UDP multicast (preferred) or TCP unicast.|
|**Event Groups**|Logical bundles of related signals (e.g., `UpdateStatus`, `CRCStatus`).|

### 🔹 In OTA Context

Used for asynchronous feedback from devices:

|Event|Direction|Description|
|---|---|---|
|`BlockCRC_OK`|Pi → QNX|Confirms data block integrity.|
|`UpdateComplete`|Pi → QNX|Signals successful update finish.|
|`ProgressUpdate`|QNX → Pi|Optional event during flashing.|

**Ideal for:** Non-blocking updates, progress signals, and multi-client notifications.

---

## 🧾 4.2.5 SOME/IP Header Fields

|Field|Size (bits)|Description|
|---|---|---|
|**Message ID**|32|Service ID + Method/Event ID|
|**Length**|32|Total size from Request ID onward|
|**Request ID**|32|Client ID + Session ID|
|**Protocol Version**|8|Format version (usually `0x01`)|
|**Interface Version**|8|Service API version|
|**Message Type**|8|Indicates message purpose (Request, Response, Event)|
|**Return Code**|8|Processing result or error|

Each SOME/IP message — whether request, response, or event — **must include this full header** for decoding and routing.

---

## ⚠️ 4.2.6 Error Handling

|Code|Meaning|Typical Cause|
|---|---|---|
|`0x00`|**OK**|Successful execution|
|`0x01`|**General Error**|Internal server failure|
|`0x02`|**Unknown Service**|Invalid Service ID|
|`0x03`|**Unknown Method**|Invalid Method/Event ID|
|`0x04`|**Not Ready**|Service not yet initialized|
|`0x05`|**Not Reachable**|Target unavailable|
|`0x06`|**Timeout**|Response not received in time|
|`0x07`|**Wrong Interface Version**|Version mismatch between sender and receiver|
|`0x08`|**Malformed Message**|Serialization or length error|
|`0x09`|**Wrong Message Type**|Unexpected message type in context|

**Error messages** always use message type `0x81 (ERROR)`.

**In OTA:**

- Timeout → lost chunk or stalled ECU.
    
- Wrong version → incompatible firmware protocol.
    
- Malformed message → serialization mismatch.
    

---

## 🔄 4.3 Compatibility Rules

Before processing, the receiver must:

1. Compare **Interface Version** in header against its own definition.
    
2. If mismatch → return **E_WRONG_INTERFACE_VERSION (0x07)**.
    

Ensures that both ends use the **same API structure and expectations**, avoiding misinterpretation of payloads.

---

## ⚙️ 5 Configuration Parameters

Defined in the **AUTOSAR ARXML** or service configuration files:

|Parameter|Purpose|
|---|---|
|**Service ID**|Uniquely identifies the service.|
|**Method/Event IDs**|Identify specific RPCs or events.|
|**Client/Server Ports**|Assign TCP/UDP ports per instance.|
|**Transport Protocol**|TCP for reliability, UDP for multicast.|

These parameters are typically loaded at startup by the SOME/IP stack (e.g., **vsomeipd**).

---

## 📘 6 Protocol Usage & Guidelines

|Section|Guideline|Explanation|
|---|---|---|
|**6.1 Choosing Transport**|TCP for large or reliable data; UDP for fast or multicast updates.|OTA chunks → TCP; progress events → UDP.|
|**6.2 CAN/FlexRay Wrapping**|Optional for legacy systems.|SOME/IP can be encapsulated in non-Ethernet networks if needed.|
|**6.3 Padding**|Align payloads to 4-byte boundaries.|Prevents misalignment errors on 32-bit/64-bit ECUs.|

---

## 📚 7 References

Standardized under **AUTOSAR Foundation Specifications**, which define:

- SOME/IP protocol
    
- SOME/IP-SD (Service Discovery)
    
- E2E protection profiles
    
- ARXML service description format
    

(Implementation tools typically integrate these automatically.)

---

## ✅ Key Takeaways for OTA Systems

1. **Use Request/Response** for critical command exchange:
    
    - `StartUpdate()`, `TransferChunk()`, `CheckCRC()`, `FinishUpdate()`  
        → Ensures confirmation and error reporting.
        
2. **Use Notification Events** for asynchronous feedback:
    
    - `BlockCRC_OK`, `UpdateComplete`, progress reports.
        
3. **Implement robust error handling:**
    
    - Handle all return codes gracefully (timeouts, version mismatches, malformed data).
        
4. **Select the right transport:**
    
    - **TCP** → reliable, ordered transfer (control/data).
        
    - **UDP** → fast, multicast notifications.
        
5. **Ensure Interface Version consistency:**
    
    - QNX ↔ Raspberry Pi must use the same service definitions and ARXML schema.
        
6. **Maintain 4-byte alignment** and standardized serialization to avoid cross-platform parsing issues.
    

---

### 🧩 In Summary

SOME/IP communication is flexible and modular:

- **Request/Response** → transactional RPC calls.
    
- **Fire & Forget** → lightweight one-way commands.
    
- **Notification Events** → scalable publish/subscribe updates.
    

When combined with proper **error handling** and **version control**, these mechanisms form the backbone of reliable distributed automotive systems — essential for OTA frameworks, diagnostics, and multi-ECU communication.