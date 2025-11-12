# 🧭 Guide for Installing QNX OS & Development Tools on Ubuntu

This guide provides **solid steps and explanations** for installing and using **QNX OS**, **Momentics IDE**, and **QNX Development Tools** on Ubuntu, along with how to configure and run QNX in **VMware**.

---

## 🪪 1. Licensing

### 🔹 Purpose

QNX products are **commercial** and require a valid license before downloading or installation.

### 🔹 Steps

1. Go to the official **[myQNX website](https://www.qnx.com/)** and create an account.
2. Request or activate a **developer license**.
3. You can follow this step-by-step guide video for assistance:
   ▶️ [QNX Licensing Tutorial](https://www.youtube.com/watch?v=DtWA5E-cFCo&t=2s)

> **Tip:** Use the same email address across your QNX account and QNX Software Center to avoid license mismatch.

---

## 🧰 2. QNX Software Center Installation

### 🔹 Purpose

The **QNX Software Center** is the central tool used to install QNX products such as **SDP (Software Development Platform)** and **Momentics IDE**.

### 🔹 Steps

1. After licensing, log in to your [MyQNX Developer Portal](https://www.qnx.com/).
2. Download the **QNX Software Center Installer** (`.run` file for Linux).
3. Make it executable and run it:

   ```bash
   chmod +x qnxsoftwarecenter.run
   ./qnxsoftwarecenter.run
   ```
4. Log in using your **licensed credentials**.
5. Inside the Software Center:

   * Click **Add Installation** → choose **SDP 8.0**.
   * Follow the prompts to complete the setup.

### 🔹 Result

* The installer will create a folder named **`/home/<user>/qnx800`**.
* This folder contains all SDK tools and environment scripts.

### 🔹 Activate the Environment

To enable QNX commands in your terminal:

```bash
source ~/qnx800/qnxsdp-env.sh
```

> **Explanation:**
> This script sets all required environment variables (PATH, QNX_HOST, QNX_TARGET) so your Ubuntu shell can compile and run QNX applications from the command line.

---

## 💡 3. Installing QNX Momentics IDE

### 🔹 Purpose

**Momentics IDE** provides a full-featured graphical environment for developing, debugging, and profiling QNX applications.

### 🔹 Steps

1. Open **QNX Software Center** again.
2. Click **Add Installation** → choose **QNX Momentics IDE**.
3. Proceed through the installation process (similar to SDP).

> **Result:**
> You’ll now have the Eclipse-based IDE ready for QNX development with SDK integration.

---

## 🧩 4. Setting Up QNX Development in VS Code

### 🔹 Purpose

Use Visual Studio Code for QNX development with modern editor capabilities.

### 🔹 Steps

1. Open **VS Code**.
2. Go to the **Extensions** tab and install **“QNX Extension”**.
3. Configure it to detect your installed **SDP** and **Momentics IDE**.
   - You can configure the SDP path which we have already installed in `/home/<user>/qnx800

You can watch this reference tutorial for the setup:
▶️ [QNX Development Tools Setup Video](https://www.youtube.com/watch?v=s8_rvkSfj10)

---

## 💻 5. Running QNX OS on VMware

### 🔹 Purpose

Run a **virtual QNX target** on your PC using **VMware Workstation**, allowing you to test your QNX applications in a full OS environment.

### 🔹 Steps

#### Step 1: Install VMware Workstation

* Go to the official Broadcom (VMware) page:
  [Download VMware Workstation Pro](https://support.broadcom.com/group/ecx/productdownloads?subfamily=VMware%20Workstation%20Pro&freeDownloads=true)
* Choose the **Linux installer** (usually `.bundle` file).
* Accept the **terms and conditions** to enable the download button.
* After download:

  ```bash
  chmod +x VMware-Workstation.bundle
  sudo ./VMware-Workstation.bundle
  ```
* Follow the on-screen instructions to complete the installation.

---

#### Step 2: Configure QNX Virtual Target in VS Code

1. Open VS Code and go to the **QNX Toolkit** (from the extensions).
2. Click on **QNX Targets → Create QNX Virtual Target**.
3. Choose:

   * **Target Type:** `vmware`
   * **Architecture:** `x86_64`
4. When prompted for `mkqnximage`, either:

   * Provide the path if available, or
   * Simply press **Enter** to continue with defaults.

> **Result:**
> VMware Workstation will automatically launch, and your **QNX virtual machine** will boot up.

You can find detailed documentation here:
📘 [QNX Quickstart Guide – Create Target System](https://www.qnx.com/developers/docs/8.0/com.qnx.doc.qnxsdp.quickstart/topic/create_target_system.html)

---

## ✅ Summary

| Component              | Purpose                     | Installation Tool   | Main Path            |
| ---------------------- | --------------------------- | ------------------- | -------------------- |
| **SDP 8.0**            | Core SDK and compilers      | QNX Software Center | `/home/user/qnx800`  |
| **Momentics IDE**      | GUI development environment | QNX Software Center | Integrated with SDP  |
| **VS Code Toolkit**    | Lightweight editor for QNX  | VS Code Extensions  | Uses SDK environment |
| **VMware Workstation** | Runs QNX Virtual Machine    | Broadcom site       | `/usr/bin/vmware`    |

---

## ⚙️ Example Command Workflow

```bash
# 1. Load the QNX SDK environment
source ~/qnx800/qnxsdp-env.sh

# 2. Verify QNX tools
qcc -V

# 3. Launch Momentics IDE (if installed)
momentics &
```

---

## 📘 References

* [QNX Official Documentation](https://www.qnx.com/developers/docs/8.0/)
* [QNX Quickstart Guide – Virtual Targets](https://www.qnx.com/developers/docs/8.0/com.qnx.doc.qnxsdp.quickstart/topic/create_target_system.html)
* [QNX Licensing & Setup Video](https://www.youtube.com/watch?v=DtWA5E-cFCo&t=2s)
* [VS Code QNX Toolkit Setup](https://www.youtube.com/watch?v=s8_rvkSfj10)

---

**End of Guide — You now have a complete setup for QNX OS development on Ubuntu using VMware, Momentics, and VS Code.**
