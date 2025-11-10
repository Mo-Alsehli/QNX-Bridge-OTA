# Service Discovery Message Format


## SOME/IP-SD Message Format (Section 7.3)

### 1. General Layout
- SD message = Special SOME/IP message (`ServiceID=0xFFFF`, `MethodID=0x8100`).
- Consists of:
  1. SOME/IP Header
  2. SOME/IP-SD Payload (Entries + Options)

---

### 2. SOME/IP Header Fields
| Field | Value / Meaning |
|-------|------------------|
| **Message ID** | `0xFFFF8100` → identifies SD message |
| **Length** | Size of message after this field |
| **Request ID** | Client ID + Session ID (`SessionID` starts at `0x0001`) |
| **Protocol Version** | `0x01` |
| **Interface Version** | `0x01` |
| **Message Type** | `0x02` (Event) |
| **Return Code** | `0x00` |

- All fields are **Big-Endian** (Network Byte Order).

---

### 3. SD Header (inside Payload)
| Field | Description |
|--------|-------------|
| **Flags** | Signals reboot/unicast capability. Reboot flag clears partner state. |
| **Reserved** | Not used (24 bit). |
| **Length of Entries Array** | Bytes occupied by Entries. |
| **Entries Array** | List of Offer/Find/Subscribe items. |
| **Length of Options Array** | Bytes occupied by Options. |
| **Options Array** | Additional info (IP endpoints, config strings). |

---

### 4. Entries Array

#### Type 1 – Service Entries
| Field | Description |
|--------|-------------|
| Type | Offer / Find / Stop |
| Service ID | Identifies the service |
| Instance ID | Service instance |
| Major / Minor Version | Versioning info |
| TTL | Lifetime (0 = Stop) |

#### Type 2 – Eventgroup Entries
| Field | Description |
|--------|-------------|
| Service ID / Instance ID / Major Version | Linked service info |
| TTL | Lifetime (0 = unsubscribe) |
| Eventgroup ID | Eventgroup identifier |
| Counter | Incremental count |

---

### 5. Options Array
- Contains extra data (endpoint or config info).
- Each Entry references options by index.
- Common types:
  - **Endpoint Option:** IP + Port (unicast/multicast).
  - **Configuration Option:** key=value strings.

**Config Option format:**
[length][key=value][length][key=value][0x00 terminator]
```
Example:
09 hostname=QNX_ECU
06 role=Server
00
```
---

### 6. OTA Example
1. **QNX Server** builds OfferService (Type 1 Entry) → TTL = 3 s, adds IP Option.  
2. **Raspberry Pi Client** receives Offer → extracts ServiceID & endpoint → ready to communicate.  
3. For event updates, QNX uses Type 2 Entry (Eventgroup) and Pi subscribes.

---

**Key Points**
- SD messages are UDP multicast.
- TTL defines service availability duration.
- Session ID + Reboot Flag detect ECU restarts.
- Entries define *what* (service/event); Options define *where/how* (endpoint/config).

# SOME/IP-SD – Endpoint Options (7.3.9.2–7.3.9.5)

### 1. Purpose
- Endpoint Options define **where** and **how** to reach a service.
- They carry **IP Address**, **Protocol (UDP/TCP)**, and **Port Number**.
- Used in SD messages to dynamically configure socket connections.

---

### 2. IPv4 Endpoint Option
- Transports **IPv4 address**, **Layer 4 protocol**, and **Port number**.
- Lets clients configure a socket to connect to the announced service.

**Behavior:**
- **UDP:** Server uses the announced port as its source port for events.
- **TCP:** Client must ensure socket is in ONLINE state before use.
- **Secure port (optional):** Security association (TLS/DTLS) must be established first.

💡 Example:  
QNX offers `OTA_Update_Service` → announces `192.168.1.10:30509 (UDP)`.

---

### 3. IPv6 Endpoint Option
- Same structure as IPv4 Endpoint Option but for IPv6 addresses.
- Follows the same UDP/TCP/secure rules.

---

### 4. IPv4 Multicast Option
- Used when events should be sent to **multiple clients** efficiently.
- Two cases:
  1. **Server (Eventhandler multicast endpoint):**  
     Announces multicast IP + port to send events (e.g., `239.0.0.1:4000`).
  2. **Client (Consumed Eventgroup multicast endpoint):**  
     Announces where it expects to receive multicast events.

- **Only UDP** is supported for multicast.

💡 Example:  
QNX multicasts “UpdateProgress” events to `239.0.0.1:4000`.

---

### 5. IPv6 Multicast Option
- Same as IPv4 Multicast Option but uses IPv6 addressing.
- Only UDP supported.

---

### 🔧 OTA Project Mapping
| Option | Role | Description |
|---------|------|-------------|
| IPv4 Endpoint | QNX → Offer OTA Service | Provides IP/port for direct communication |
| IPv4 Multicast | QNX & Pi | Used for event updates (progress/status) |
| IPv6 Options | Optional | Only needed if IPv6 network is used |

---

**Key Takeaway:**  
Endpoint Options tell ECUs *where* a service lives and *how* to connect (IP, Port, Protocol).  
In your setup, `vsomeip` handles creating these automatically during Offer/Find/Subscribe operations.
