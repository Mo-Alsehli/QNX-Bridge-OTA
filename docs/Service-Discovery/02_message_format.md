# 📘 Service Discovery Message Format (SOME/IP-SD)

---

## 1. General Layout

The **Service Discovery (SD)** message in SOME/IP is a **specialized SOME/IP message** used for advertising and finding services between ECUs.

It contains:

1. **SOME/IP Header** – identifies this as an SD message.
2. **SOME/IP-SD Payload** – holds *Entries* (service/event info) and *Options* (connection details).

| Layer          | Description                                                                                 |
| -------------- | ------------------------------------------------------------------------------------------- |
| SOME/IP Header | Identifies this as a Service Discovery message (`ServiceID = 0xFFFF`, `MethodID = 0x8100`). |
| SD Payload     | Contains `Entries` + `Options` arrays.                                                      |

---

## 2. SOME/IP Header Fields

| Field                 | Value / Description                                          |
| --------------------- | ------------------------------------------------------------ |
| **Message ID**        | `0xFFFF8100` → identifies SD message                         |
| **Length**            | Size of the message (excluding header)                       |
| **Request ID**        | Combines Client ID + Session ID (Session starts at `0x0001`) |
| **Protocol Version**  | `0x01`                                                       |
| **Interface Version** | `0x01`                                                       |
| **Message Type**      | `0x02` → Event                                               |
| **Return Code**       | `0x00`                                                       |
| **Byte Order**        | All fields are **Big Endian (Network Byte Order)**           |

🧠 **Purpose:**
This header helps distinguish SD traffic from normal SOME/IP service messages.

---

## 3. SD Header (Inside the Payload)

| Field                       | Description                                                                                  |
| --------------------------- | -------------------------------------------------------------------------------------------- |
| **Flags**                   | Used to indicate ECU reboot or unicast capability. (Reboot flag clears previous state info.) |
| **Reserved (24-bit)**       | Reserved for future use.                                                                     |
| **Length of Entries Array** | Specifies how many bytes the Entries take.                                                   |
| **Entries Array**           | Contains the actual Offer, Find, Subscribe, etc. entries.                                    |
| **Length of Options Array** | Specifies how many bytes the Options take.                                                   |
| **Options Array**           | Holds endpoint and configuration data.                                                       |

🧩 **Purpose:**
The SD header separates and organizes *what* is being announced (Entries) and *how* to connect (Options).

---

## 4. Entries Array (Core Component)

Entries describe the actions related to services and events.

### Type 1 – Service Entries

Used for advertising or requesting services.

| Field                   | Description                                          |
| ----------------------- | ---------------------------------------------------- |
| **Type**                | `OfferService`, `FindService`, or `StopOfferService` |
| **Service ID**          | Identifies the specific service                      |
| **Instance ID**         | Identifies the service instance                      |
| **Major/Minor Version** | For compatibility control                            |
| **TTL**                 | Time-to-live in seconds (`0 = stop offering`)        |

---

### Type 2 – Eventgroup Entries

Used for managing event subscriptions and notifications.

| Field                                        | Description                                    |
| -------------------------------------------- | ---------------------------------------------- |
| **Service ID / Instance ID / Major Version** | Identifies the parent service                  |
| **TTL**                                      | Lifetime of the subscription (0 = unsubscribe) |
| **Eventgroup ID**                            | Identifies the specific eventgroup             |
| **Counter**                                  | Incremental value to track event updates       |

🧠 **Summary:**

* **Type 1** → Offers or finds *services*.
* **Type 2** → Subscribes or manages *eventgroups*.

---

## 5. Options Array

Each entry can reference one or more **Options**, which contain connection or configuration information.

| Type                     | Description                                           |
| ------------------------ | ----------------------------------------------------- |
| **Endpoint Option**      | Defines IP address + Port (unicast or multicast).     |
| **Configuration Option** | Carries string parameters (e.g., `hostname=QNX_ECU`). |

**Configuration Option Example:**

```
09 hostname=QNX_ECU
06 role=Server
00
```

* Each key-value pair is prefixed by its length and terminated by `0x00`.

🧩 **Purpose:**
Options describe **where** and **how** to access a service or event.

---

## 6. Example – OTA Service Discovery Flow

1. **QNX (Server)** sends an `OfferService` (Type 1 Entry) with TTL = 3 s and includes its IP Option.
2. **Raspberry Pi (Client)** receives the message → extracts `ServiceID` and endpoint → configures its socket.
3. For continuous updates, QNX sends a **Type 2 Entry** (Eventgroup), and the Pi subscribes using `SubscribeEventgroup`.

🧠 **Result:**
Both ECUs are dynamically connected using automatically discovered IPs and ports.

---

## 7. Endpoint Options (Section 7.3.9.2–7.3.9.5)

Endpoint options specify the **network location** and **protocol type** for communication.

| Type                      | Contents                                 | Protocol Support |
| ------------------------- | ---------------------------------------- | ---------------- |
| **IPv4 Endpoint Option**  | IPv4 address + Port + Protocol (UDP/TCP) | UDP / TCP        |
| **IPv6 Endpoint Option**  | IPv6 address + Port + Protocol           | UDP / TCP        |
| **IPv4 Multicast Option** | IPv4 multicast address + Port            | UDP only         |
| **IPv6 Multicast Option** | IPv6 multicast address + Port            | UDP only         |

---

### 7.1 IPv4 Endpoint Option

Used for defining **unicast** communication between client and server.

* Carries **IPv4 address**, **Layer 4 protocol**, and **Port number**.
* Allows clients to configure sockets dynamically.

| Protocol        | Behavior                                                        |
| --------------- | --------------------------------------------------------------- |
| **UDP**         | Server uses the announced port as its source port for events.   |
| **TCP**         | Client ensures socket is connected (`ONLINE` state) before use. |
| **Secure Port** | Requires TLS/DTLS session first.                                |

💡 **Example:**
QNX → `OfferService` for OTA
→ Announces endpoint `192.168.1.10:30509 (UDP)`.

---

### 7.2 IPv6 Endpoint Option

Same as IPv4 Endpoint Option, but with IPv6 addressing.

🧩 **Note:**
Only required in IPv6 networks (not mandatory for IPv4-based systems).

---

### 7.3 IPv4 Multicast Option

Used when a service wants to broadcast data to **multiple subscribers**.

| Role                      | Function                                                           |
| ------------------------- | ------------------------------------------------------------------ |
| **Server (Eventhandler)** | Announces multicast IP/Port to send data (e.g., `239.0.0.1:4000`). |
| **Client (Subscriber)**   | Joins the multicast group to receive events.                       |

⚙️ Only **UDP** is supported for multicast transmission.

💡 **Example:**
QNX multicasts “UpdateProgress” → `239.0.0.1:4000`
All subscribed clients receive updates simultaneously.

---

### 7.4 IPv6 Multicast Option

Identical to IPv4 multicast, but with IPv6 addressing.
Supports UDP multicast only.

---

## 8. OTA Project Mapping (QNX ↔ Raspberry Pi)

| Option                    | ECU Role                | Function                                     |
| ------------------------- | ----------------------- | -------------------------------------------- |
| **IPv4 Endpoint Option**  | QNX → Offer OTA Service | Advertises server’s IP/Port for connection   |
| **IPv4 Multicast Option** | QNX & RPi               | Used for event updates like `UpdateProgress` |
| **IPv6 Options**          | Optional                | Only for IPv6-based networks                 |

---

## 9. Summary

| Element         | Function                                              |
| --------------- | ----------------------------------------------------- |
| **SD Message**  | Special SOME/IP message (`0xFFFF8100`) for discovery. |
| **Entries**     | Define *what* is offered (service or event).          |
| **Options**     | Define *where/how* to reach it (IP, Port, Config).    |
| **TTL**         | Controls how long a service is considered active.     |
| **Reboot Flag** | Clears old state info after ECU reset.                |
| **Multicast**   | Efficient event distribution for many clients.        |

---

### ✅ Key Takeaway

> **SOME/IP-SD** defines a flexible and automated mechanism for service discovery using standard message structures (Entries + Options).
> Endpoint options describe **where** a service is reachable and **which protocol** to use — all managed automatically by the SOME/IP middleware (e.g., `vsomeipd`).
