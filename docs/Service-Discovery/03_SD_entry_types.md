# 📘 SOME/IP-SD – Section 7.4: Service Discovery Entry Types

---

## 1. Overview

**Service Discovery Entries** define *what action* the message represents within the SOME/IP-SD protocol.

They are the **core commands (verbs)** used by ECUs to:

* Offer or request services
* Subscribe or unsubscribe to event updates

There are **two main categories:**

| Category                        | Purpose                            | Entry Types                                                                 |
| ------------------------------- | ---------------------------------- | --------------------------------------------------------------------------- |
| **Type 1 – Service Entries**    | Service availability and discovery | `OfferService`, `StopOfferService`, `FindService`                           |
| **Type 2 – Eventgroup Entries** | Event subscription and management  | `SubscribeEventgroup`, `StopSubscribeEventgroup`, `NackSubscribeEventgroup` |

---

## 2. Type 1 – Service Entries

### 🟢 **OfferService**

* **Sent by:** Server ECU
* **Purpose:** Announce that a service is available.
* **Contains:**

  * `Service ID`, `Instance ID`, `Major/Minor Version`, `TTL`, and associated endpoint information.
* **Behavior:**

  * Clients store this information temporarily.
  * The service remains valid only for the defined **TTL** period unless refreshed.

💡 **Example:**
QNX server sends `OfferService` for `OTA_Update_Service` with TTL = 3 s.
→ Raspberry Pi knows service is available and connects.

---

### 🔴 **StopOfferService**

* **Sent by:** Server ECU
* **Purpose:** Indicate that a previously offered service is no longer available.
* **Key Field:** `TTL = 0`
* **Behavior:**

  * Instructs all clients to remove the service from their discovery table.

💡 **Example:**
QNX reboots → sends `StopOfferService` to notify clients to clear old connections.

---

### 🔍 **FindService**

* **Sent by:** Client ECU
* **Purpose:** Actively search for an available service.
* **Contains:** `Service ID` + `Instance ID`.
* **Behavior:**

  * If a server matches, it replies with an `OfferService` entry.
  * If no match, the client continues broadcasting FindService periodically.

💡 **Example:**
Raspberry Pi sends `FindService (OTA_Update_Service)` → QNX responds with `OfferService`.

---

## 3. Type 2 – Eventgroup Entries

### 🟣 **SubscribeEventgroup**

* **Sent by:** Client ECU
* **Purpose:** Subscribe to a specific eventgroup to receive data or notifications.
* **Contains:** `Service ID`, `Instance ID`, `Eventgroup ID`, and `TTL`.
* **Behavior:**

  * Server adds this client to its internal subscriber list.
  * The subscription remains active until TTL expires or StopSubscribe is sent.

💡 **Example:**
Pi subscribes to `UpdateProgress` eventgroup of OTA service → QNX starts sending progress updates.

---

### 🟠 **StopSubscribeEventgroup**

* **Sent by:** Client ECU
* **Purpose:** Cancel a previous subscription.
* **Key Field:** `TTL = 0`
* **Behavior:**

  * Server removes the client from its subscriber list.

💡 **Example:**
When OTA update finishes, Pi sends `StopSubscribeEventgroup` to stop receiving further progress events.

---

### 🚫 **NackSubscribeEventgroup**

* **Sent by:** Server ECU
* **Purpose:** Reject a subscription request.
* **Used When:**

  * Client requests a non-existent eventgroup.
  * Subscription request is invalid or unauthorized.
* **Behavior:**

  * Server refuses to add the client to the subscriber list.
  * Client must handle or retry appropriately.

💡 **Example:**
If Pi tries subscribing to a restricted eventgroup, QNX replies with `NackSubscribeEventgroup`.

---

## 4. TTL (Time To Live)

| TTL Value | Meaning                                | Action                                 |
| --------- | -------------------------------------- | -------------------------------------- |
| **> 0**   | Entry valid for that many seconds      | Remains active until refreshed         |
| **0**     | Entry expired or intentionally stopped | Stops offering or cancels subscription |

* TTL ensures **dynamic** and **fault-tolerant** communication:

  * Services automatically expire if a server disconnects.
  * Clients must periodically **refresh** their FindService or Subscribe requests.

💡 **Example:**
If QNX stops sending `OfferService` updates within TTL, Pi automatically removes the OTA service from its list.

---

## 5. OTA Example Flow (End-to-End)

| Step  | ECU          | Entry Type                    | Description                                        |
| ----- | ------------ | ----------------------------- | -------------------------------------------------- |
| **1** | Raspberry Pi | `FindService`                 | Requests `OTA_Update_Service`                      |
| **2** | QNX          | `OfferService`                | Announces OTA service endpoint with TTL = 3 s      |
| **3** | Raspberry Pi | `SubscribeEventgroup`         | Subscribes to `UpdateProgress` events              |
| **4** | QNX          | *(accepts)*                   | Adds Pi to subscriber list and sends event updates |
| **5** | Either ECU   | `StopSubscribe` / `StopOffer` | Ends communication or disconnects                  |

---

## 6. Summary

| Category                        | Handles                            | Entry Types                               | Direction       |
| ------------------------------- | ---------------------------------- | ----------------------------------------- | --------------- |
| **Type 1 – Service Entries**    | Service discovery and availability | Offer / StopOffer / Find                  | Server ↔ Client |
| **Type 2 – Eventgroup Entries** | Event subscription and control     | Subscribe / StopSubscribe / NackSubscribe | Client ↔ Server |

🧭 **Final Insight:**

> Entry Types are the **verbs** of SOME/IP Service Discovery — defining what action each ECU takes in the service lifecycle.
> Type 1 manages *who offers what*, while Type 2 manages *who listens to what*.
> TTL ensures that all connections stay fresh and automatically recover from reboots or network loss.
