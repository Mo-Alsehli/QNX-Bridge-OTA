# 📘 Complete Guide to SOME/IP Service Discovery (SOME/IP-SD)

---

## 1. Introduction to Service Discovery

**Service Discovery (SD)** in SOME/IP is the mechanism enabling ECUs (Electronic Control Units) to dynamically **find**, **offer**, and **manage** communication with each other.
Each ECU can act as:

| Role                   | Description                                                                                   |
| ---------------------- | --------------------------------------------------------------------------------------------- |
| **ServerService**      | Offers a service to others (e.g., QNX offering an OTA update service).                        |
| **ClientService**      | Requests or consumes offered services (e.g., Raspberry Pi requesting the OTA update service). |
| **EventHandler**       | Generates events or notifications on the server side.                                         |
| **ConsumedEventgroup** | Subscribes to events or notifications on the client side.                                     |

### Communication Mechanism

**Publish/Subscribe Pattern**

* **Server (Publisher)** → Sends data/events (`OfferService`)
* **Client (Subscriber)** → Requests updates (`SubscribeEventgroup`)

When many clients need the same data, multicast communication optimizes bandwidth usage.

---

## 2. SOME/IP-SD Message Format

SOME/IP-SD messages are **special SOME/IP packets** used for service advertisement and discovery.

### General Layout

1. **SOME/IP Header**
2. **SOME/IP-SD Payload** – contains:

   * **Entries Array** (what is being offered/subscribed)
   * **Options Array** (where/how to reach it)

### SOME/IP Header Fields

| Field                            | Description                          |
| -------------------------------- | ------------------------------------ |
| **Message ID**                   | `0xFFFF8100` (identifies SD message) |
| **Protocol / Interface Version** | `0x01` / `0x01`                      |
| **Message Type**                 | `0x02` (Event)                       |
| **Return Code**                  | `0x00`                               |
| **Byte Order**                   | Big Endian (Network Order)           |

---

### SD Header (Inside Payload)

| Field                        | Description                          |
| ---------------------------- | ------------------------------------ |
| **Flags**                    | Indicates ECU reboot/unicast support |
| **Entries Array**            | List of Offers/Finds/Subscribes      |
| **Options Array**            | IP addresses, ports, configs         |
| **Reserved / Length fields** | Manage memory layout                 |

---

### Entries Array

| Type                            | Purpose                     | Key Fields                     |
| ------------------------------- | --------------------------- | ------------------------------ |
| **Type 1 – Service Entries**    | Advertise/discover services | Service ID, Instance ID, TTL   |
| **Type 2 – Eventgroup Entries** | Manage event subscriptions  | Service ID, Eventgroup ID, TTL |

---

### Options Array

| Option                   | Description                                  |
| ------------------------ | -------------------------------------------- |
| **IPv4/IPv6 Endpoint**   | IP address, port, protocol (UDP/TCP)         |
| **Multicast Endpoint**   | Used for event broadcast to multiple clients |
| **Configuration Option** | Key-value pairs like `hostname=QNX_ECU`      |

💡 **Example:**

```text
09 hostname=QNX_ECU
06 role=Server
00
```

---

## 3. Service Discovery Entry Types (Section 7.4)

Entries define *what the SD message is doing*. They act as **verbs** in communication.

### Type 1 – Service Entries

| Entry                | Sent By | Purpose                                |
| -------------------- | ------- | -------------------------------------- |
| **OfferService**     | Server  | Announces a service availability       |
| **StopOfferService** | Server  | Withdraws an offered service (`TTL=0`) |
| **FindService**      | Client  | Searches for available services        |

🧠 **Example:**
Raspberry Pi → `FindService(OTA_Update_Service)`
QNX → `OfferService` with service info and TTL.

---

### Type 2 – Eventgroup Entries

| Entry                       | Sent By | Purpose                         |
| --------------------------- | ------- | ------------------------------- |
| **SubscribeEventgroup**     | Client  | Subscribe to server events      |
| **StopSubscribeEventgroup** | Client  | Cancel a subscription (`TTL=0`) |
| **NackSubscribeEventgroup** | Server  | Reject invalid subscriptions    |

🧠 **Example:**
Pi subscribes to OTA “UpdateProgress” → QNX starts sending progress updates.

---

### TTL (Time To Live)

| TTL     | Meaning                             |
| ------- | ----------------------------------- |
| **> 0** | Entry valid for that time (seconds) |
| **0**   | Stop or cancel action               |

TTL ensures ECUs auto-refresh or remove services when messages stop arriving — making the system **self-healing** and **dynamic**.

---

### OTA Example Flow

| Step | ECU    | Action                                             |
| ---- | ------ | -------------------------------------------------- |
| 1    | Pi     | `FindService` – looks for OTA service              |
| 2    | QNX    | `OfferService` – announces availability            |
| 3    | Pi     | `SubscribeEventgroup` – subscribes to updates      |
| 4    | QNX    | Sends `UpdateProgress` events                      |
| 5    | Either | `StopOffer` / `StopSubscribe` – ends communication |

---

## 4. Message Transmission and Reception (Section 7.5)

SOME/IP-SD relies on the **Socket Adaptor (SoAd)** to send and receive UDP messages.

### Transmission (7.5.1)

* Built by SD and sent via `SoAd_Transmit()`.
* Uses:

  * **Multicast** for `OfferService` / `FindService`.
  * **Unicast** for specific replies or confirmations.
* Common entries: OfferService, FindService, SubscribeEventgroup.

### Reception (7.5.2)

* `SoAd_RxIndication()` notifies SD when UDP packet received.
* SD:

  1. Parses header.
  2. Updates local service table.
  3. May respond (e.g., Offer → Find).

| Entry                   | Receiver Action       |
| ----------------------- | --------------------- |
| FindService             | Server → OfferService |
| OfferService            | Client → Connect      |
| StopOfferService        | Client → Remove       |
| SubscribeEventgroup     | Server → Register     |
| StopSubscribeEventgroup | Server → Remove       |

### Answering Behavior (7.5.3.1)

* Multicast is used for discovery.
* Servers reply via **unicast** when possible to reduce traffic.

---

### OTA Example (Runtime)

| ECU | Role   | Action                                              |
| --- | ------ | --------------------------------------------------- |
| QNX | Server | Sends `OfferService`                                |
| RPi | Client | Sends `FindService`, subscribes to `UpdateProgress` |
| QNX | Server | Sends events (UDP multicast)                        |

**Default UDP Port:** `30490`

---

## 5. Server Timing & Repetition Rules (Section 7.6)

### OfferService Sequence

1. Random **initial delay** to prevent collisions.
2. Send initial **OfferService**.
3. Repeat quickly a few times.
4. Continue **cyclic offers** at longer intervals.

| Parameter                                       | Description                    |
| ----------------------------------------------- | ------------------------------ |
| `SdServerTimerInitialOfferDelayMin/Max`         | Random initial delay           |
| `SdServerTimerInitialOfferRepetitionsBaseDelay` | Delay between repeated offers  |
| `SdServerTimerOfferCyclicDelay`                 | Period between periodic offers |
| `SdServerTimerTTL`                              | Client validity duration       |

💡 **EventHandlers** use same logic for eventgroup updates.
If many subscribers → switch to multicast.

---

### TTL Interaction

* Servers refresh Offers before TTL expires.
* Clients delete services when TTL runs out.
* Ensures live status reflection of all ECUs.

### OTA Example (Server Side)

| ECU | Action           | Timing                                                 |
| --- | ---------------- | ------------------------------------------------------ |
| QNX | OfferService     | 200 ms delay → 3 repeats every 2 s → cyclic every 10 s |
| RPi | Receive Offer    | Updates TTL = 30 s                                     |
| QNX | StopOfferService | TTL = 0 before shutdown                                |

---

## 6. Client Timing & Repetition Rules (Section 7.7)

### FindService Cycle

| Step | Action                                      |
| ---- | ------------------------------------------- |
| 1️⃣  | Wait random delay                           |
| 2️⃣  | Send `FindService`                          |
| 3️⃣  | Repeat several times quickly                |
| 4️⃣  | Stop repeating once `OfferService` received |
| 5️⃣  | Restart if TTL expires                      |

| Parameter                              | Description                           |
| -------------------------------------- | ------------------------------------- |
| `SdClientTimerInitialFindDelayMin/Max` | Random delay before first Find        |
| `SdClientTimerFindCyclicDelay`         | Periodic interval after initial phase |

---

### SubscribeEventgroup Cycle

| Step | Action                              |
| ---- | ----------------------------------- |
| 1️⃣  | Wait random delay                   |
| 2️⃣  | Send `SubscribeEventgroup`          |
| 3️⃣  | Repeat multiple times               |
| 4️⃣  | Renew cyclically before TTL expires |

| Parameter                                               | Description                         |
| ------------------------------------------------------- | ----------------------------------- |
| `SdConsumedEventGroupTimerInitialSubscribeDelayMin/Max` | Random delay before first Subscribe |
| `SdConsumedEventGroupTimerSubscribeCyclicDelay`         | Periodic renewal interval           |
| `SdConsumedEventGroupTimerTTL`                          | Subscription lifetime               |

---

### Combined Logic

1. Client repeatedly sends **FindService** until Offer received.
2. Upon Offer → starts subscription cycle.
3. Refreshes SubscribeEventgroup before TTL expires.
4. If Offer lost → restart FindService cycle.

---

### OTA Example (Client Side)

| ECU            | Action                                | Timing                                               |
| -------------- | ------------------------------------- | ---------------------------------------------------- |
| Raspberry Pi   | `FindService(OTA_Update_Service)`     | 100 ms delay → 3 repeats every 2 s → then every 10 s |
| QNX            | `OfferService`                        | TTL = 30 s                                           |
| RPi            | `SubscribeEventgroup(UpdateProgress)` | 200 ms delay → 3 repeats → then every 10 s           |
| If Offer stops | TTL expires → restart FindService     | ~30 s later                                          |

---

## 7. Overall Summary

| Section                  | Focus        | Description                                              |
| ------------------------ | ------------ | -------------------------------------------------------- |
| **1 – Introduction**     | Concept      | ECUs offer, find, and subscribe to services dynamically. |
| **2 – Message Format**   | Structure    | SOME/IP Header + SD Payload (Entries + Options).         |
| **3 – Entry Types**      | Actions      | Type 1 (Offer/Find) and Type 2 (Subscribe/Stop).         |
| **4 – Message Handling** | Transport    | UDP via SoAd, multicast/unicast delivery.                |
| **5 – Server Timings**   | Heartbeat    | Defines Offer repetition & TTL refresh.                  |
| **6 – Client Timings**   | Search Cycle | Defines Find/Subscribe repetition and TTL sync.          |

---

## 🧭 Final Takeaway

> SOME/IP Service Discovery forms the **communication backbone** of modern automotive networks.
>
> * **Servers** continuously announce services (`OfferService`).
> * **Clients** dynamically search and subscribe (`FindService`, `SubscribeEventgroup`).
> * **TTL**, **multicast**, and **timing repetition** rules keep systems synchronized, fault-tolerant, and self-updating — essential for distributed systems like the **QNX ↔ Raspberry Pi OTA update architecture**.
