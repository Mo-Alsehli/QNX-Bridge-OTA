# 📘 SOME/IP-SD – Sections 7.5, 7.6, and 7.7

## Message Transmission, Reception, and Timing Behavior

---

## 🔹 Section 7.5 – Sending and Receiving of Messages

### **Overview**

This section explains how SOME/IP Service Discovery (SD) messages are **sent and received** through the **Socket Adaptor (SoAd)**, which handles UDP communication between ECUs.

---

### **7.5.1 – Message Transmission**

| Feature                  | Description                                                                                                                                                  |
| ------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Mechanism**            | SD constructs messages (Entries + Options) and transmits via `SoAd_Transmit()`.                                                                              |
| **Protocol**             | Always uses **UDP** for communication.                                                                                                                       |
| **Modes**                | - **Multicast:** for broadcasting `OfferService` or `FindService`. <br> - **Unicast:** for specific replies or confirmations (e.g., subscription responses). |
| **Typical Entries Sent** | `OfferService`, `StopOfferService`, `FindService`, `SubscribeEventgroup`, `StopSubscribeEventgroup`.                                                         |
| **Routing**              | Managed by Socket Adaptor via preconfigured **PDU IDs** and IP endpoints.                                                                                    |

🧠 **Key Point:**
SD does not handle raw sockets directly — it delegates this to SoAd, which ensures the right routing and IP configuration.

---

### **7.5.2 – Message Reception**

| Process Step | Description                                                         |
| ------------ | ------------------------------------------------------------------- |
| 1️⃣          | `SoAd_RxIndication()` is triggered when a UDP packet arrives.       |
| 2️⃣          | SD parses the SOME/IP-SD header to identify Entry and Option types. |
| 3️⃣          | Updates internal **service state table** based on message content.  |
| 4️⃣          | Optionally replies (e.g., OfferService in response to FindService). |

| Entry Type                | ECU Reaction                                      |
| ------------------------- | ------------------------------------------------- |
| `FindService`             | Server replies with `OfferService`.               |
| `OfferService`            | Client records service availability and connects. |
| `StopOfferService`        | Client removes service from local table.          |
| `SubscribeEventgroup`     | Server adds client to subscriber list.            |
| `StopSubscribeEventgroup` | Server removes client from subscriber list.       |

---

### **7.5.3 – Answering Behavior (Multicast)**

* **Purpose:** Avoid network flooding when many ECUs are active.
* ECUs receiving multicast SD messages:

  1. Compare received entry with their own service configuration.
  2. If matched, reply **via unicast** to the requesting ECU instead of another multicast.
  3. Avoids redundant broadcast replies.

---

### **Implementation Notes**

| Parameter            | Description                                          |
| -------------------- | ---------------------------------------------------- |
| **UDP Port**         | Default SOME/IP-SD port: **30490**                   |
| **Interfaces**       | Each network interface can host its own SD instance. |
| **Transport**        | Always UDP-based (for both unicast and multicast).   |
| **Repetition Rules** | Governed by Sections 7.6 (server) and 7.7 (client).  |

---

### **OTA Example**

| ECU                       | Role                                              | Behavior |
| ------------------------- | ------------------------------------------------- | -------- |
| **QNX (Server)**          | Sends `OfferService` for `OTA_Update_Service`.    |          |
| **Raspberry Pi (Client)** | Sends `FindService` request to locate OTA server. |          |
| **QNX (Server)**          | Responds via unicast `OfferService`.              |          |
| **Pi (Client)**           | Subscribes to OTA `UpdateProgress` Eventgroup.    |          |
| **QNX (Server)**          | Publishes event updates via UDP multicast.        |          |

---

**Summary:**
Section 7.5 defines the runtime message exchange — how ECUs use UDP to announce, discover, and subscribe to services dynamically through the Socket Adaptor.

---

## 🔹 Section 7.6 – Timings and Repetitions for Server Services & Event Handlers

### **Purpose**

This section defines how a **server ECU** (e.g., QNX) manages **OfferService** and **Eventgroup** announcements over time — controlling *when*, *how often*, and *for how long* messages are repeated.

---

### **1. OfferService Behavior**

| Step | Action                | Description                                                         |
| ---- | --------------------- | ------------------------------------------------------------------- |
| 1️⃣  | **Initial Delay**     | Wait random delay before first Offer (avoids broadcast collisions). |
| 2️⃣  | **Initial Offers**    | Send first OfferService message immediately after delay.            |
| 3️⃣  | **Quick Repetitions** | Repeat Offer several times at short intervals.                      |
| 4️⃣  | **Cyclic Offers**     | Continue sending offers periodically at longer intervals.           |

| Parameter                                       | Description                                             |
| ----------------------------------------------- | ------------------------------------------------------- |
| `SdServerTimerInitialOfferDelayMin/Max`         | Random delay range before first Offer.                  |
| `SdServerTimerInitialOfferRepetitionsBaseDelay` | Time gap between repeated offers.                       |
| `SdServerTimerInitialOfferRepetitionsMax`       | Maximum number of repetitions.                          |
| `SdServerTimerOfferCyclicDelay`                 | Period between periodic (cyclic) Offers.                |
| `SdServerTimerTTL`                              | Time-to-live value clients use to consider Offer valid. |

💡 **Goal:**
Ensure that all clients eventually receive the Offer even if initial packets are lost.

---

### **2. StopOfferService**

* Sent when service becomes unavailable.
* Always includes `TTL = 0`.
* Can be repeated multiple times for reliability.

---

### **3. Eventhandler Timings**

* Eventgroups (notifications) follow a similar pattern to services:

  * Initial delay
  * Repetition phase
  * Cyclic refresh phase
* Parameter: `SdEventHandlerMulticastThreshold`
  → If many clients subscribe, the server switches to **multicast** instead of multiple unicasts.

---

### **4. TTL Interaction**

| Behavior        | Description                                                                         |
| --------------- | ----------------------------------------------------------------------------------- |
| Server → Client | Sends Offers more frequently than TTL to ensure validity.                           |
| Client → Server | Removes expired services if TTL elapsed without renewal.                            |
| Result          | Dynamic and self-healing system, automatically adapting to reboots or network loss. |

---

### **5. OTA Example**

| ECU                   | Action             | Timing                                                      |
| --------------------- | ------------------ | ----------------------------------------------------------- |
| QNX (Server)          | OfferService       | 200 ms delay → 3 repeats every 2 s → then cyclic every 10 s |
| Raspberry Pi (Client) | Receive Offer      | Updates TTL = 30 s                                          |
| QNX (Server)          | StopOfferService   | Sent before shutdown (TTL = 0)                              |
| QNX (Server)          | EventHandler Offer | Re-announced every 10 s for UpdateProgress multicast        |

---

**Summary:**
Section 7.6 defines the **server’s heartbeat** — how services continually announce their presence and keep event streams alive for clients.

---

## 🔹 Section 7.7 – Timings for Client Services & Consumed Eventgroups

### **Purpose**

This section defines how a **client ECU** (e.g., Raspberry Pi) periodically searches for services (`FindService`) and maintains active subscriptions (`SubscribeEventgroup`).

---

### **1. Client Service Timing (FindService)**

| Step | Behavior                                                     |
| ---- | ------------------------------------------------------------ |
| 1️⃣  | Random delay before first `FindService`.                     |
| 2️⃣  | Sends first `FindService`.                                   |
| 3️⃣  | Repeats several times quickly (to ensure discovery).         |
| 4️⃣  | Stops repeating after receiving matching `OfferService`.     |
| 5️⃣  | Restarts Find cycle if no Offer received before TTL expires. |

| Parameter                                      | Description                                              |
| ---------------------------------------------- | -------------------------------------------------------- |
| `SdClientTimerInitialFindDelayMin/Max`         | Random startup delay range.                              |
| `SdClientTimerInitialFindRepetitionsBaseDelay` | Gap between repeated FindService messages.               |
| `SdClientTimerInitialFindRepetitionsMax`       | Number of quick repetitions.                             |
| `SdClientTimerFindCyclicDelay`                 | Interval for periodic Find messages after initial phase. |

---

### **2. TTL Handling**

| Situation       | Client Reaction                                    |
| --------------- | -------------------------------------------------- |
| Offer received  | Stores service as active and starts TTL countdown. |
| TTL expires     | Marks service as unavailable, restarts Find cycle. |
| Offer refreshed | Resets TTL countdown.                              |

---

### **3. Consumed Eventgroup Timing (Subscriptions)**

| Step | Behavior                                              |
| ---- | ----------------------------------------------------- |
| 1️⃣  | Wait random delay before first `SubscribeEventgroup`. |
| 2️⃣  | Send initial Subscribe message.                       |
| 3️⃣  | Repeat several times quickly.                         |
| 4️⃣  | Continue with cyclic refresh before TTL expires.      |

| Parameter                                                       | Description                               |
| --------------------------------------------------------------- | ----------------------------------------- |
| `SdConsumedEventGroupTimerInitialSubscribeDelayMin/Max`         | Random delay before first Subscribe.      |
| `SdConsumedEventGroupTimerInitialSubscribeRepetitionsBaseDelay` | Gap between quick repetitions.            |
| `SdConsumedEventGroupTimerInitialSubscribeRepetitionsMax`       | Max repetitions.                          |
| `SdConsumedEventGroupTimerSubscribeCyclicDelay`                 | Interval for cyclic subscription renewal. |
| `SdConsumedEventGroupTimerTTL`                                  | Subscription validity period.             |

---

### **4. Combined Logic Flow**

1. **Find Cycle:** Client repeatedly searches until it finds an Offer.
2. **Offer Received:** Starts subscription cycle.
3. **Subscription Maintenance:** Refreshes SubscribeEventgroup before TTL expires.
4. **Loss Recovery:** If Offer not renewed → unsubscribe and restart Find cycle.

---

### **5. OTA Example**

| ECU                   | Action                                | Timing                                                      |
| --------------------- | ------------------------------------- | ----------------------------------------------------------- |
| Raspberry Pi (Client) | `FindService(OTA_Update_Service)`     | 100 ms delay → 3 repeats every 2 s → then cyclic every 10 s |
| QNX (Server)          | `OfferService`                        | TTL = 30 s                                                  |
| Pi (Client)           | `SubscribeEventgroup(UpdateProgress)` | 200 ms delay → 3 repeats → then every 10 s                  |
| If Offer stops        | TTL expires → restart FindService     | After ≈ 30 s                                                |

---

**Summary:**
Section 7.7 defines the **client’s heartbeat** — how it persistently looks for services and maintains subscriptions through controlled timing, repetition, and TTL logic.
Together with Section 7.6, it ensures continuous, loss-tolerant communication between ECUs.

---

### 🧭 Final Takeaway

| Section | Focus          | Role                    | Description                              |
| ------- | -------------- | ----------------------- | ---------------------------------------- |
| **7.5** | Message Flow   | Communication Layer     | Defines UDP send/receive via SoAd.       |
| **7.6** | Server Timings | QNX (Provider)          | Manages Offer and Eventgroup repetition. |
| **7.7** | Client Timings | Raspberry Pi (Consumer) | Manages Find and Subscribe repetition.   |

> **In short:**
> SOME/IP-SD works as a synchronized heartbeat between ECUs — servers continually **offer**, and clients continually **search and subscribe** — all managed through carefully defined timing, repetition, and TTL rules to keep the distributed system alive and consistent.
