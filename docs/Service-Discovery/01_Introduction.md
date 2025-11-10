# Introduction to Service Discovery.

## SOME/IP Service Discovery – Services and Eventgroups
### 1. Concept
- A **Service** is a logical functionality provided by an ECU.
- Each Service can contain **0–many Eventgroups** (collections of notifications).
- When implemented:
  - **ServerService** = ECU offering the Service.
  - **ClientService** = ECU using the Service.
  - **EventHandler** (on server) ↔ **ConsumedEventgroup** (on client).

---

### 2. ECU Roles

#### 🖥 Server Service
- Offers the service when available (`OfferService`).
- Withdraws it when unavailable (`StopOfferService`).
- Responds to `FindService` requests from clients.

#### 📡 Client Service
- Listens for `OfferService` / `StopOfferService` messages.
- Stores discovery info in volatile memory.
- Sends `FindService` to locate available servers.

---

### 3. Publish/Subscribe Communication
- Used for event-based data transfer.
- **Server (Publisher)** → provides data/events.
- **Client (Subscriber)** → subscribes via `SubscribeEventgroup`.
- Publish = `OfferService`; Subscribe = `SubscribeEventgroup`.

Example:  
QNX (Server) → offers `OTA_Update_Service`.  
Raspberry Pi (Client) → subscribes to `UpdateProgress` events.

---

### 4. Multicast Optimization
- Reduces traffic when many clients subscribe to the same eventgroup.
- **Eventhandler Multicast Endpoint (Server side)**:  
  Server switches to multicast when many clients subscribe.
- **Consumed Eventgroup Multicast Endpoint (Client side)**:  
  Client requests multicast IP/port for receiving events.
---

# SOME/IP Service Discovery in Practice

### 1. Behavior
- Acts like a **background daemon**.
- Periodically sends **OfferService** or **FindService** messages.
- Uses **TTL** to track which services are alive.
- Clears entries when ECUs reboot or stop offering.

### 2. OTA Setup Roles
| ECU | Role | Function |
|------|------|-----------|
| QNX | Server | Offers OTA service (OfferService). |
| Raspberry Pi | Client | Discovers OTA service (FindService). |

### 3. Implementation
- **Do NOT re-implement SD** manually.
- Use an existing SOME/IP library (e.g. **vsomeip**, **CommonAPI**).
- These libraries provide:
  - Built-in SD daemon (`vsomeipd`).
  - Configurable IDs and endpoints.
  - Automatic TTL and state handling.

### 4. Developer Task
- Only configure your service (ID, instance, port).
- Start the SD daemon and your app.
- React to callbacks when services appear/disappear.

### 5. Custom Implementation
- Only needed if building a full custom SOME/IP stack from scratch.

---

**In short:**  
For your QNX ↔ RPi OTA system —  
> You just need to **configure** and **run** the SD component from the SOME/IP library,  
> not write the message format or protocol logic yourself.

