# Specification of SOME/IP Message Format (Message Header)

---

## 🧩 Overview

The **SOME/IP Message Header** defines how data is **serialized** (encoded) inside **Protocol Data Units (PDUs)** that are transmitted over **UDP** or **TCP**.  
It provides a consistent structure for communication between ECUs (Electronic Control Units) using **Service-Oriented Architecture (SOA)** principles.

---

## 🧠 SOME/IP Header Structure

|Field|Size (bits)|Description|
|---|---|---|
|Message ID (Service ID / Method ID)|32|Identifies the service and operation (method/event).|
|Length|32|Number of bytes following this field (from Request ID onward).|
|Request ID (Client ID / Session ID)|32|Distinguishes between concurrent requests.|
|Protocol Version|8|Specifies the SOME/IP header format version.|
|Interface Version|8|Specifies the version of the service definition (API).|
|Message Type|8|Specifies whether it’s a request, response, or event.|
|Return Code|8|Indicates processing result or error type.|

### 💡 Representation

```
|<------------------------- 32 bits ------------------------>|
+------------------------------------------------------------+
| Message ID (Service ID + Method/Event ID)                  |
+------------------------------------------------------------+
| Length (in bytes, from Request ID to end of message)       |
+------------------------------------------------------------+
| Request ID (Client ID + Session ID)                        |
+------------------------------------------------------------+
| Protocol | Interface | Message | Return | → 4 bytes total  |
| Version  | Version   | Type    | Code   |                  |
+------------------------------------------------------------+
| Payload (variable size, depending on UDP/TCP)              |
```

---

## 🧰 End-to-End (E2E) Communication Protection

### Purpose

To detect **data corruption**, **message loss**, **duplication**, or **out-of-order delivery** between ECUs.  
For example, when exchanging cyclic data (speed, torque, temperature, OTA progress), E2E ensures reliability.

### Placement

If used, the E2E header is inserted **after the Return Code** and before the Payload:

```
| SOME/IP Header (16 bytes) |
| E2E Header (depends on profile) |
| Payload |
```

### Common E2E Fields

|Field|Purpose|
|---|---|
|Counter|Detects missing or repeated messages.|
|CRC/Checksum|Detects data corruption.|
|Data ID|Identifies protected data type.|
|Length / Reserved|Implementation-specific padding.|

Example (AUTOSAR E2E Profile 2):

```
| CRC | Counter | DataID | Length |  →  total 4–8 bytes
```

---

## 1️⃣ Message ID (32-bit)

The **Message ID** uniquely identifies which **service** and **operation** the message belongs to.

### Bit Layout

```text
|<---------------------- 32 bits ---------------------->|
| Service ID (16) | 0 (1 bit) | Method ID (15) |
```

- Bits 31–16 → Service ID
    
- Bit 15 → Always `0` (separator)
    
- Bits 14–0 → Method ID
    

**Supports:**

- 65,536 services
    
- Each with up to 32,768 methods/events
    

### Event ID Layout

```text
|<---------------------- 32 bits ---------------------->|
| Service ID (16) | 1 (1 bit) | Event ID (15) |
```

**Difference:**

- Bit 15 = `0` → Method (RPC call)
    
- Bit 15 = `1` → Event (Notification)
    

|Case|Direction|Type|Purpose|Example|
|---|---|---|---|---|
|**Method**|Client → Server|Request/Response|Ask ECU to perform an action|Raspberry Pi → QNX: `startUpdate()`|
|**Event**|Server → Client|Publish/Subscribe|Notify other ECUs about a change|QNX → Pi: `ProgressUpdate`|

> **Every event or field notifier must belong to an event group**, since subscriptions in SOME/IP are done per event group.

---

## 2️⃣ Length (32-bit)

Specifies the **number of bytes** following this field, **starting from Request ID** until the end of the message.

```
Length = sizeof(Request ID + remaining header + payload)
```

---

## 3️⃣ Request ID (32-bit)

Used to handle **parallel** or **asynchronous** requests.

### Structure

```text
|<------------------- 32 bits ------------------->|
| Client ID (16) | Session ID (16) |
```

- **Client ID:** Identifies the sender ECU.
    
- **Session ID:** Identifies a specific request instance.
    

**Example:**

|Request|Client ID|Session ID|Combined Request ID|Meaning|
|---|---|---|---|---|
|1|0x1201|0x000A|0x1201000A|First request|
|2|0x1201|0x000B|0x1201000B|Second parallel request|

When the provider replies, it **copies the same Request ID** into the response header → enabling the client to match responses.

---

## 4️⃣ Protocol Version (8-bit)

Identifies which **SOME/IP header structure** is used.

- **Default (AUTOSAR-compliant):** `0x01`
    

|Field|Size|Typical Value|Meaning|
|---|---|---|---|
|Protocol Version|8 bits|0x01|Current SOME/IP header version|

**Rules:**

- Increment this number **only** if header structure changes.
    
- Payload/interface updates don’t affect it.
    

---

## 5️⃣ Interface Version (8-bit)

Specifies the **Major Version** of the service interface (API definition), not the SOME/IP protocol itself.

- Increment this number when **incompatible changes** are made to the service interface.
    
- Typical value: `0x01`
    

---

## 6️⃣ Message Type (8-bit)

Differentiates between message kinds:

|Hex|Name|Direction|Description|
|---|---|---|---|
|0x00|REQUEST|Client → Server|Request expecting response|
|0x01|REQUEST_NO_RETURN|Client → Server|Fire-and-forget request|
|0x02|NOTIFICATION|Server → Client|Event or field notification|
|0x80|RESPONSE|Server → Client|Normal response|
|0x81|ERROR|Server → Client|Error response|

### Transport Protocol (TP) Variants

Used when the payload is **too large** to fit in a single frame.

|Hex|Type|Description|
|---|---|---|
|0x20|TP_REQUEST|Segmented request|
|0x21|TP_REQUEST_NO_RETURN|TP fire-and-forget|
|0x22|TP_NOTIFICATION|TP event|
|0xA0|TP_RESPONSE|TP segmented response|
|0xA1|TP_ERROR|TP segmented error|

#### SOME/IP-TP

Used for **large data transfers** (e.g., OTA firmware).

|Type|Description|Use Case|Example|
|---|---|---|---|
|**Non-TP**|Fits in one UDP/TCP frame|Small control data|`startUpdate()` call|
|**TP**|Multi-frame segmented|Large payloads|50 KB OTA image chunk|

---

## 7️⃣ Return Code (8-bit)

Indicates if a request was processed successfully.

|Message Type|Allowed Return Codes|
|---|---|
|REQUEST|0x00 (E_OK)|
|REQUEST_NO_RETURN|0x00 (E_OK)|
|NOTIFICATION|0x00 (E_OK)|
|RESPONSE|Defined in [PRS_SOMEIP_00191]|
|ERROR|Defined in [PRS_SOMEIP_00191], ≠ 0x00|

---

## 8️⃣ Payload (Variable Size)

Contains serialized **parameters** or **data elements** for the service (method or event).

- Size depends on the **transport protocol**:
    
    - **UDP:** 0–1400 bytes
        
    - **TCP:** No strict limit (segmentation supported)
        

**Example Uses:**

- Method → includes input/output parameters
    
- Event → includes sensor readings or state updates
    

---

## 🌍 Endianness

All SOME/IP headers are encoded in **network byte order (Big Endian)**.  
This ensures interoperability across heterogeneous hardware platforms.

---

## ✅ Summary Table

|Field|Size|Purpose|Key Notes|
|---|---|---|---|
|Message ID|32 bits|Identifies service/method/event|Combines Service ID + Method/Event ID|
|Length|32 bits|Size of remaining message|From Request ID to end|
|Request ID|32 bits|Tracks request instances|Client ID + Session ID|
|Protocol Version|8 bits|Header structure version|Typically `1`|
|Interface Version|8 bits|Service API version|Typically `1`|
|Message Type|8 bits|Defines message behavior|Request / Response / Event|
|Return Code|8 bits|Indicates success or error|0x00 = OK|
|Payload|Variable|Actual data|Serialized parameters|
|Endianness|—|Big Endian|Network byte order|

---

### 🧩 Concept Recap

> SOME/IP transforms communication from **message-level** to **service-level**,  
> ensuring scalable, structured, and reliable data exchange between ECUs.

It enables:

- Clear separation of service logic (method/event definitions)
    
- Parallel requests via unique Request IDs
    
- Version control via Protocol & Interface Versions
    
- Robust communication via E2E protection and TP segmentation
    

**Result:** A standardized, efficient, and scalable foundation for automotive SOA communication.