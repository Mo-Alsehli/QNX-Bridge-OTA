# SOME/IP-SD – Section 7.4: Service Discovery Entry Types

### 1. Overview
- Entries define **what the SD message is doing**.
- Two categories:
  - **Type 1 (Service Entries)** → Offer / StopOffer / Find
  - **Type 2 (Eventgroup Entries)** → Subscribe / StopSubscribe / NackSubscribe

---

### 2. Type 1 – Service Entries

#### 🟢 OfferService
- Sent by **Server** → announces a service is available.
- Includes Service ID, Instance ID, TTL, endpoint.
- Client stores this info and can connect.
- TTL defines how long it remains valid.

#### 🔴 StopOfferService
- Sent by **Server** → withdraws previously offered service.
- TTL = 0.

#### 🔍 FindService
- Sent by **Client** → asks if a service is available.
- If a matching server exists, it replies with OfferService.

---

### 3. Type 2 – Eventgroup Entries

#### 🟣 SubscribeEventgroup
- Sent by **Client** → subscribes to receive event data from a service.
- Includes Service ID, Instance ID, Eventgroup ID, TTL.
- Server adds client to its subscriber list.

#### 🟠 StopSubscribeEventgroup
- Sent by **Client** → cancels subscription.
- TTL = 0.

#### 🚫 NackSubscribeEventgroup
- Sent by **Server** → rejects invalid or unauthorized subscriptions.

---

### 4. TTL (Time To Live)
- Defines how long the entry is valid.
- TTL = 0 → stop or cancel.
- Server and client refresh TTLs periodically.

---

### 5. OTA Example Flow

| Step | ECU | Entry Type | Description |
|------|-----|-------------|-------------|
| 1 | Pi | FindService | Looks for OTA_Update_Service |
| 2 | QNX | OfferService | Announces OTA service endpoint |
| 3 | Pi | SubscribeEventgroup | Subscribes to UpdateProgress events |
| 4 | QNX | (accepts) | Sends events to Pi |
| 5 | (either) | StopSubscribe/StopOffer | Ends connection |

---

**Summary:**  
- Entry Types = “verbs” of Service Discovery.  
- Type 1 handles service discovery; Type 2 handles event subscriptions.  
- TTL ensures dynamic and reliable communication in distributed systems.
