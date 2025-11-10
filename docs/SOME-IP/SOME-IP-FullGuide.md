# 🚗 SOME/IP — Scalable Service-Oriented Middleware over IP

_(Practical Guide for Automotive & OTA Systems)_

---

## 🧭 What Is SOME/IP?

**SOME/IP (Scalable Service-Oriented Middleware over IP)** is the **standard communication middleware** used in automotive ECUs.  
It enables distributed components to talk in terms of **services** (not raw messages), supporting:

- ⚙️ **Remote Procedure Calls (RPCs)**
    
- 🔔 **Event Notifications**
    
- 💾 **Structured Data Serialization**
    

It’s cross-platform — runs on **Linux**, **QNX**, and **AUTOSAR ECUs**, making it perfect for OTA (Over-the-Air) frameworks.

---

## 🧩 SOME/IP Architecture (Big Picture)

```text
┌──────────────────────────────────────────┐
│               Application Layer           │
│   (Services, Methods, Events, Fields)     │
├──────────────────────────────────────────┤
│            SOME/IP Middleware             │
│  ├─ Serialization                         │
│  ├─ Transport (TCP / UDP / TP)            │
│  └─ Service Discovery (SOME/IP-SD)        │
├──────────────────────────────────────────┤
│           Transport Layer (IP)            │
│    TCP – Reliable | UDP – Fast/Multicast  │
└──────────────────────────────────────────┘
```

---

## ⚙️ 1. Service Concepts

|Component|Purpose|Communication Type|
|---|---|---|
|**Method**|Remote function call|Request / Response|
|**Event**|Async notification|Publish / Subscribe|
|**Field**|Shared variable (Getter / Setter / Notifier)|Bidirectional|

💡 _Think of a service like a class:_  
methods = functions, events = signals, fields = shared state.

---

## 📦 2. SOME/IP Message Format

Each message follows the **SOME/IP Header (16 bytes)** → Payload.

```
+------------------------------------------------------------+
| Message ID (Service + Method/Event ID)   [32 b]            |
+------------------------------------------------------------+
| Length (bytes after this field)          [32 b]            |
+------------------------------------------------------------+
| Request ID (Client + Session ID)         [32 b]            |
+------------------------------------------------------------+
| Protocol | Interface | MsgType | RetCode [4×8 b = 32 b]    |
+------------------------------------------------------------+
| Payload (serialized data)                                 |
+------------------------------------------------------------+
```

|Field|Purpose|
|---|---|
|**Message ID**|Identifies which service + method/event|
|**Length**|Size of everything that follows|
|**Request ID**|Tracks concurrent requests|
|**Protocol / Interface Version**|Ensure compatibility|
|**Message Type**|Request / Response / Event|
|**Return Code**|Execution result|

🧠 **Endian:** Always _Big-Endian_ (Network order).

---

## 🧮 3. Data Serialization Rules

|Type|Description|Example Use|
|---|---|---|
|**Basic** (`uint8`, `float32`, …)|Fixed-size primitives|sensor data|
|**Struct**|Grouped types, serialized sequentially|composite message|
|**Array (Fixed / Dynamic)**|Multiple elements, optional length field|file blocks, lists|
|**String**|`[Length][Text][\0]`|names, paths|
|**Enum / Bitfield**|Compact flags or modes|status bits|
|**Union / Variant**|Runtime-selectable data type|mixed payloads|

> All elements are **aligned** (2, 4, 8 bytes) and padded if needed.  
> Serialization ensures different ECUs parse identical binary layouts.

---

## 🌐 4. Transport Protocols

### 🔸 UDP Binding

- Lightweight and connectionless.
    
- Ideal for **events or multicast updates**.
    
- One socket per service instance (unicast + multicast).
    
- Fragmentation handled by IP (or SOME/IP-TP).
    

### 🔹 TCP Binding

- Reliable, ordered stream.
    
- Used for **RPCs / control commands**.
    
- Client manages connection (open → use → close).
    
- Disable Nagle’s algorithm (`TCP_NODELAY`) for low latency.
    

### 🔸 SOME/IP-TP (Transport Protocol over UDP)

Used when message > UDP limit (~1400 B).  
Splits payload into segments with a small **TP-Header**:

```
| Offset (28 b) | Reserved (3 b) | MoreSegments (1 b) |
```

📦 Example (OTA image transfer):

```
Original firmware chunk (32 KB)
   ↓  split into 23 UDP segments (~1392 B each)
   ↓  each has same MessageID + SessionID
Receiver reassembles → full image block
```

|Feature|UDP|TCP|SOME/IP-TP|
|---|---|---|---|
|Reliability|✗|✓|Partial|
|Ordering|✗|✓|Maintained by offset|
|Use Case|Events|RPCs|Large UDP transfers|

---

## 🔁 5. Communication Types

### 1️⃣ Request / Response (RPC)

```
Client ──Request──▶ Server
Client ◀─Response── Server
```

Used for **transactional commands**:  
`StartUpdate()`, `CheckCRC()`, `TransferChunk()`, `FinishUpdate()`  
→ Typically over **TCP**.

---

### 2️⃣ Fire & Forget

```
Client ──▶ Server   (no response)
```

Used for **non-critical** actions (e.g., heartbeat).

---

### 3️⃣ Notification Events

```
Server (Provider) ──▶ Multiple Clients (Subscribers)
```

Used for **asynchronous updates** via **UDP multicast**:  
`ProgressUpdate`, `BlockCRC_OK`, `UpdateComplete`.

---

### 🧩 Visual Summary

```text
        ┌──────────────┐
        │   Client ECU │
        └──────┬───────┘
               │ Request / Fire&Forget
               ▼
        ┌──────────────┐
        │  Server ECU  │
        └──────┬───────┘
               │ Event Notifications
               ▼
         Other Subscribers
```

---

## ⚠️ 6. Error Handling

|Code|Meaning|Typical Cause|
|---|---|---|
|`0x00`|OK|Successful|
|`0x01`|General Error|Internal failure|
|`0x02`|Unknown Service|Wrong Service ID|
|`0x03`|Unknown Method|Wrong Method/Event|
|`0x04`|Not Ready|Init incomplete|
|`0x05`|Not Reachable|Target offline|
|`0x06`|Timeout|Response missing|
|`0x07`|Wrong Interface Ver.|API mismatch|
|`0x08`|Malformed Message|Serialization error|

🧠 **Best Practice:**  
Always match responses via `Request ID` and check `Return Code`.

---

## 🔄 7. Version & Compatibility

|Field|Purpose|
|---|---|
|**Protocol Version**|Format of SOME/IP header (usually `1`)|
|**Interface Version**|Version of the defined service API|
|**Compatibility Rule**|If Interface versions differ → return `E_WRONG_INTERFACE_VERSION (0x07)`|

---

## ⚙️ 8. Configuration Parameters (ARXML / Manual)

|Parameter|Description|
|---|---|
|Service ID|Unique per service|
|Method / Event IDs|Operation identifiers|
|Client / Server Ports|Define TCP/UDP bindings|
|Transport Protocol|TCP (RPCs) / UDP (Events)|
|Instance ID|Differentiates multiple service instances|

---

## 🚀 9. OTA-Focused Example Flow

```text
[PC] ─FTP─▶ [QNX VM]
   │
   │  (CRC check done)
   ▼
[QNX] ─SOME/IP/TCP─▶ [Raspberry Pi]
   │   startUpdate()
   │   transferChunk()
   │   checkCRC()
   │   finishUpdate()
   │
   ▼
[Pi] ─SOME/IP/UDP─▶ [QNX]
   Event: BlockCRC_OK / UpdateComplete
```

---

## ✅ 10. Quick Reference Summary

|Layer|Key Concept|OTA Role|
|---|---|---|
|**Service**|Logical group of methods/events|“UpdateService”|
|**Method**|RPC call (Req/Resp)|Control commands|
|**Event**|Pub/Sub async update|Progress, CRC status|
|**Transport**|TCP / UDP / SOME/IP-TP|Data exchange|
|**Header**|Unified message frame|Parsing + routing|
|**Serialization**|On-wire layout|Ensures compatibility|
|**Error Handling**|Return codes|Reliability|
|**Versioning**|Protocol & Interface IDs|Prevent mismatch|

---

## 🧩 Visual Cheat-Sheet

```text
┌──────────────────────────────────────────────────────────────┐
│              SOME/IP Communication Cheat-Sheet               │
├────────────┬──────────────────────┬───────────────────────────┤
│  Message   │  Direction           │  Example (OTA)            │
├────────────┼──────────────────────┼───────────────────────────┤
│ Request    │ Client → Server      │ StartUpdate(), CheckCRC() │
│ Response   │ Server → Client      │ OK / Error code           │
│ Event      │ Server → Subscribers │ BlockCRC_OK, Progress     │
│ Fire&Forget│ Client → Server      │ Heartbeat                 │
├────────────┴──────────────────────┴───────────────────────────┤
│ TCP = Reliable / Ordered / For Commands                      │
│ UDP = Fast / Multicast / For Events                          │
│ SOME/IP-TP = Fragmented Large UDP (OTA data blocks)          │
└──────────────────────────────────────────────────────────────┘
```

---

## 🧠 Final Takeaway

> **SOME/IP = Language of Modern ECUs.**  
> It unifies all communication — control, feedback, and data transfer — over IP networks using a **service-oriented** model.  
> In your OTA system, it ensures smooth, scalable, and standard-compliant interaction between **Linux ↔ QNX ↔ Raspberry Pi**.

---

**Author:** Embedded Linux / OTA Project — QNX ↔ Raspberry Pi  
**References:** AUTOSAR PRS SOME/IP R20-11, vsomeip Documentation

---