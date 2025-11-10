# SOME/IP Transport Protocol Bindings

---

## 🌐 Overview (Section 4.2.1)

**SOME/IP** supports both **UDP** and **TCP** transport bindings to transmit **Remote Procedure Calls (RPCs)**, **events**, and **notifications** between ECUs.

|**Aspect**|**Explanation**|
|---|---|
|**Supported Transports**|UDP and TCP|
|**Purpose**|Defines how SOME/IP messages are encapsulated and transferred via network layers|
|**Port-Based Mapping**|Each **Service Instance** maps to a unique port number on the server side|
|**Multiple Messages per Packet**|A single UDP/TCP packet may contain multiple SOME/IP messages|
|**Message Header Rule**|Every SOME/IP message must include its own header and length field|
|**Message Boundary Detection**|Receivers detect message boundaries using the `Length` field in the SOME/IP header|
|**Unaligned Message Handling**|Receivers must accept messages even if not aligned to 32-bit boundaries|
|**Typical Setup per Service Instance**|- One TCP connection- One UDP unicast connection- One UDP multicast connection|
|**TCP Usage**|Reliable communication for RPCs (Request/Response)|
|**UDP Usage**|Fast, low-latency communication for events or multicast messages|

---

## ⚙️ Key Takeaways

- **TCP** → reliability, ordering, and connection-based (used for method calls).
    
- **UDP** → speed, low overhead, multicast-friendly (used for events).
    
- Each SOME/IP message is **self-contained** with its own header and `Length` field.
    
- **Ports** differentiate multiple service instances.
    
- Each service instance may maintain up to **three parallel links**:
    
    - One TCP
        
    - One UDP unicast
        
    - One UDP multicast
        

---

## 🚀 4.2.1.1 – UDP Binding

### 🧩 Overview

Defines how SOME/IP messages are carried over **UDP datagrams**.

|**Requirement ID**|**Description**|**Notes**|
|---|---|---|
|[PRS_SOMEIP_00139]|SOME/IP messages are encapsulated directly in UDP packets.|The entire SOME/IP message forms the UDP payload.|
|[PRS_SOMEIP_00137]|SOME/IP does not restrict UDP fragmentation.|Fragmentation/reassembly handled by the IP layer.|
|[PRS_SOMEIP_00943]|One UDP **unicast** connection per service instance.|Handles all unicast methods/events for that instance.|
|[PRS_SOMEIP_00942]|One UDP **multicast** connection per service instance.|Used for all events configured for multicast.|

### 🔹 UDP Key Principles

- Each **SOME/IP message = one UDP datagram payload**.
    
- Fragmentation (if needed) occurs automatically in the IP stack.
    
- **No need for multiple sockets** per event or method; one socket pair per instance is enough.
    
- **Unicast UDP:** targeted messages between client and server.
    
- **Multicast UDP:** shared messages (events/notifications) for multiple subscribers.
    

### ✅ Advantages

- Lightweight, low overhead, suitable for **real-time broadcasts**.
    
- Simplified socket management.
    
- Ideal for event-driven or sensor-based data (e.g., speed updates, sensor values).
    

---

## 🔒 4.2.1.2 – TCP Binding

### 🧩 Overview

Defines SOME/IP operation over **TCP**, adding **reliability**, **ordering**, and **connection management**.

|**Requirement ID**|**Description**|**Notes**|
|---|---|---|
|—|TCP binding follows the same message structure as UDP but adds reliability.|Enables transmission of large or ordered data streams.|
|—|Nagle’s algorithm should be **disabled** (`TCP_NODELAY`).|Reduces latency for real-time requests.|
|[PRS_SOMEIP_00706]|Lost TCP connections → outstanding requests treated as **timeouts**.|Prevents infinite waiting for responses.|
|[PRS_SOMEIP_00707]|One TCP connection per service instance for all communication.|All methods/events/notifications share one connection.|
|[PRS_SOMEIP_00708]|**Client** opens the TCP connection when needed.|Usually on first request or notification.|
|[PRS_SOMEIP_00709]|**Client** closes connection when no longer needed.|Graceful shutdown.|
|[PRS_SOMEIP_00710]|Client closes when services stop or timeout.|Avoids resource waste.|
|[PRS_SOMEIP_00711]|**Server must not close TCP immediately** when services stop.|Gives client time to finish transactions.|

### 🔹 TCP Key Principles

- TCP ensures **loss-free, ordered** message delivery.
    
- **No additional reliability mechanisms** needed in SOME/IP.
    
- **Client controls connection lifecycle**:
    
    - Opens TCP session on demand.
        
    - Handles reconnection if broken.
        
    - Closes when finished.
        
- Server waits before closing, avoiding reconnection loops.
    

### 🧠 Why Disable Nagle’s Algorithm?

- Nagle’s algorithm buffers small packets → adds delay.
    
- Disabling it (`TCP_NODELAY`) ensures **low latency** communication (crucial for control signals).
    

### ✅ Advantages

- Guaranteed message delivery.
    
- Maintains sequence integrity.
    
- Supports larger messages natively (no need for TP segmentation).
    

### ⚠️ Caution

If the server closes the TCP socket first, clients may interpret it as an error and reconnect repeatedly.

---

## 📦 4.2.1.4 – Transporting Large SOME/IP Messages over UDP (SOME/IP-TP)

### 🧩 Overview

**SOME/IP-TP (Transport Protocol)** is a **fragmentation and reassembly layer** used when a SOME/IP message exceeds the UDP size limit.

|Term|Description|
|---|---|
|**Original Message**|Complete SOME/IP message before fragmentation (e.g., 32 KB firmware block).|
|**Segment**|A smaller portion of that message (≤ 1392 bytes).|
|**TP Header**|Added to each segment to identify its order and continuation.|
|**Session ID**|Same across all segments belonging to one message.|
|**More Segments Flag**|1 = more segments follow, 0 = last segment.|

---

### 🧱 TP Header Format

|Field|Bits|Description|
|---|---|---|
|**Offset**|28|Position of segment in message (in 16-byte multiples).|
|**Reserved Flags**|3|Always 0.|
|**More Segments Flag**|1|Indicates whether more segments follow (1 = yes, 0 = no).|

**Each segment:**

```
| SOME/IP Header | TP Header | Segment Payload |
```

---

### 📨 Sender Rules

- Fragment only when message size > UDP payload limit.
    
- All segments of a message share the same **Session ID**.
    
- Send segments **in ascending offset order**.
    
- Each segment ≤ 1392 B payload (Ethernet MTU = 1500).
    
- Last segment → `More Segments Flag = 0`.
    
- Avoid duplicate or overlapping offsets.
    

---

### 📥 Receiver Rules

- Identify fragments by `(Message ID + Session ID)`.
    
- Reassemble segments in correct order using the `Offset`.
    
- Abort if any segment is missing, misordered, or corrupt.
    
- **Return Code** from the last segment applies to the full message.
    
- No interleaving of two segmented messages within one buffer.
    

---

### 📊 Practical Limits

|Parameter|Value|Description|
|---|---|---|
|**Max Payload per Segment**|1392 B|Fits within Ethernet MTU 1500.|
|**Offset Step**|16 B|Offset expressed in multiples of 16.|
|**Encoding**|Big-Endian|Network byte order.|

---

### 🔧 Example (OTA Scenario)

In an **Over-The-Air (OTA)** update:

- QNX (server) sends a 32 KB firmware chunk via UDP using SOME/IP-TP.
    
- It divides the data into ~23 segments (1392 B each).
    
- Each segment carries the same Message ID and Session ID.
    
- Raspberry Pi (client) reassembles the segments before writing to storage.
    
- If TCP were used instead → no TP layer needed.
    

---

### ✅ Summary

|Feature|UDP|TCP|SOME/IP-TP|
|---|---|---|---|
|**Reliability**|No|Yes|Partial (reassembly only)|
|**Ordering**|No|Yes|Maintained via Offset|
|**Fragmentation**|IP-level only|Automatic|TP-layer|
|**Use Case**|Events / Multicast|RPCs / Critical Data|Large UDP transfers|
|**Example**|Sensor broadcast|ECU control call|OTA image chunks|

---

### 🔎 In Essence

- **UDP Binding** → lightweight, used for fast event/multicast updates.
    
- **TCP Binding** → reliable, used for request/response communication.
    
- **SOME/IP-TP** → adds fragmentation/reassembly over UDP for large data.
    

These transport bindings make SOME/IP scalable — from **simple sensor ECUs** using UDP events to **complex domain controllers** using TCP RPCs and large UDP transfers with TP support.