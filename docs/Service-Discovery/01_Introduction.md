# 📘 Introduction to Service Discovery (SOME/IP SD)

---

## 1. Concept Overview

**Service Discovery (SD)** in SOME/IP is the mechanism that allows ECUs (Electronic Control Units) to find, offer, and manage communication with each other dynamically.

* A **Service** is a logical functionality (e.g., OTA update, diagnostics) provided by an ECU.
* Each Service may contain **Eventgroups**, which are sets of notifications or data signals broadcast by the Service.

### ECU Roles

| Role                   | Description                                                           |
| ---------------------- | --------------------------------------------------------------------- |
| **ServerService**      | ECU that *offers* the service to others.                              |
| **ClientService**      | ECU that *uses* or *consumes* the offered service.                    |
| **EventHandler**       | Server-side logic that generates events or notifications.             |
| **ConsumedEventgroup** | Client-side component that subscribes to receive those notifications. |

---

## 2. ECU Roles Explained

### 🖥 Server Service (Provider)

* Announces availability by sending `OfferService`.
* Withdraws availability using `StopOfferService`.
* Replies to client `FindService` requests.
* Example: **QNX** offering the **OTA Update Service**.

### 📡 Client Service (Consumer)

* Continuously listens for `OfferService` and `StopOfferService` messages.
* Keeps discovered services in volatile memory (cleared on restart).
* Sends `FindService` to locate available servers.
* Example: **Raspberry Pi** searching for the **OTA Update Service**.

---

## 3. Publish/Subscribe Mechanism

**Purpose:** Efficient event-driven data exchange between ECUs.

| Role                    | Function              | Description                                        |
| ----------------------- | --------------------- | -------------------------------------------------- |
| **Server (Publisher)**  | `OfferService`        | Sends out event notifications or periodic updates. |
| **Client (Subscriber)** | `SubscribeEventgroup` | Requests to receive updates or data streams.       |

🔹 **Example Use Case (OTA System):**

* QNX (Server) → Offers `OTA_Update_Service`.
* Raspberry Pi (Client) → Subscribes to `UpdateProgress` events to monitor update status.

This mechanism ensures that clients only receive relevant notifications they have subscribed to.

---

## 4. Multicast Optimization

When multiple clients subscribe to the same eventgroup, **multicast communication** is used to reduce network traffic.

| Component                                                | Function                                                                          |
| -------------------------------------------------------- | --------------------------------------------------------------------------------- |
| **Eventhandler Multicast Endpoint (Server side)**        | Sends one multicast message for all subscribers instead of multiple unicast ones. |
| **Consumed Eventgroup Multicast Endpoint (Client side)** | Configures a multicast IP/port to receive shared event notifications.             |

🧩 **Example:**
Instead of sending 10 separate messages to 10 clients, the server multicasts one message that all clients receive simultaneously.

---

## 5. SOME/IP SD in Practice

### Behavior

* Operates as a **background daemon** managing service advertisement and discovery.
* Periodically sends:

  * `OfferService` (server side)
  * `FindService` (client side)
* Uses **TTL (Time To Live)** to determine service validity.
* Automatically removes inactive or rebooted ECUs.

---

### OTA Setup Roles

| ECU              | Role   | Function                                               |
| ---------------- | ------ | ------------------------------------------------------ |
| **QNX**          | Server | Offers the OTA service (`OfferService`).               |
| **Raspberry Pi** | Client | Finds and connects to the OTA service (`FindService`). |

---

### Implementation Guidelines

🔧 **Use Existing Libraries – Don’t Reinvent SD**

* Recommended libraries: **vsomeip**, **CommonAPI**.
* They provide:

  * Built-in **SD Daemon** (`vsomeipd`).
  * Configurable **Service IDs**, **Instance IDs**, and **Endpoints**.
  * Automatic management of **TTL** and **state changes**.

---

### Developer Responsibilities

1. **Configure your service parameters:**

   * Service ID
   * Instance ID
   * Port / Protocol (UDP or TCP)

2. **Start the SD daemon (`vsomeipd`).**

3. **Launch your application.**

   * React to callback events like:

     * “Service Available”
     * “Service Lost”

4. **Handle communication logic.**

   * Subscribe, send, and process event data as needed.

---

### Custom Implementation (Optional)

Only necessary if developing your **own SOME/IP stack** from scratch (e.g., for research or specialized systems).
In standard automotive applications, using ready-made SD modules is strongly preferred.

---

## 🧭 Summary

| Concept               | Key Point                                               |
| --------------------- | ------------------------------------------------------- |
| **Purpose**           | Enables ECUs to automatically find and offer services.  |
| **Mechanism**         | Uses Offer/Find messages and Publish/Subscribe pattern. |
| **Optimization**      | Multicast reduces redundant transmissions.              |
| **Developer Role**    | Configure, not reimplement, the SD logic.               |
| **Practical Example** | QNX ↔ RPi OTA Service using vsomeipd daemon.            |

---

### ✅ In Short

For your **QNX ↔ Raspberry Pi OTA system**:

> Simply **configure and launch** the Service Discovery daemon from the SOME/IP library —
> no need to manually handle message encoding or protocol logic.
