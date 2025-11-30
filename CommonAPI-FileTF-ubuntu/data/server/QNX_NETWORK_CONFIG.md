# Configuring QNX Network with Static IP for SOME/IP Applications

This guide explains how to configure static IP networking on QNX SDP 8.0 running in VMware, specifically for SOME/IP communication.

## Table of Contents
1. [Understanding QNX Network Architecture](#understanding-qnx-network-architecture)
2. [VMware Network Configuration](#vmware-network-configuration)
3. [Setting Static IP on QNX](#setting-static-ip-on-qnx)
4. [Configuring Multicast Routing](#configuring-multicast-routing)
5. [Running SOME/IP Applications](#running-someip-applications)
6. [Troubleshooting](#troubleshooting)

---

## Understanding QNX Network Architecture

### QNX VM Network Characteristics

When running QNX in VMware, the filesystem behaves differently than a traditional installation:

- **Root filesystem** (`/`) is mounted from an IFS (Image File System) in RAM
- **`/etc`** directory is recreated on every boot
- Traditional configuration files like `/etc/system/config/interfaces` do not exist or are ignored
- Network configuration must be done at boot time or manually after boot

### Network Interface

QNX in VMware uses:
- **Interface name**: `vmx0` (VMware virtual NIC)
- **Driver**: VMware VMXNET driver
- **Not**: `eth0` (this is a Linux convention)

---

## VMware Network Configuration

### 1. Change from NAT to Bridged Mode

For SOME/IP to work correctly, your QNX VM **must** use Bridged networking (not NAT).

**Why Bridged Mode?**
- NAT mode hides the VM behind the host, breaking multicast
- SOME/IP Service Discovery requires multicast (224.224.224.245)
- Clients on other devices (like Raspberry Pi) cannot reach the VM in NAT mode

### Steps to Enable Bridged Mode:

1. **Power off** the QNX VM completely (not suspend)

2. **Open VM Settings**:
   - Right-click on your QNX VM → **Settings**
   - Click **Network Adapter**

3. **Select Bridged Mode**:
   - Choose: **Bridged: Connected directly to the physical network**
   - Check: **Replicate physical network connection state**

4. **Select Physical Adapter**:
   - Click **Configure Adapters**
   - If using **WiFi**: Enable only your WiFi adapter
   - If using **Ethernet**: Enable only your Ethernet adapter
   - Disable all other adapters (Bluetooth, VirtualBox, Docker, etc.)

5. **Apply and Start VM**

### 2. Configure Ubuntu Host Network (if needed)

On your Ubuntu host, you need to bind VMware's bridge to the correct adapter.

Open VMware Virtual Network Editor:
```bash
sudo vmware-netcfg
```

Find **VMnet0** (Bridged):
- Change from: `Bridged to: Automatic`
- Change to: `Bridged to: wlo1` (your WiFi adapter)

Or if using Ethernet:
- Change to: `Bridged to: enp3s0` (your Ethernet adapter)

Save and restart VMware.

---

## Setting Static IP on QNX

### Option 1: Temporary Static IP (Lost on Reboot)

This method is quick for testing but does not persist across reboots.

**SSH into QNX:**
```bash
ssh qnxuser@<current-qnx-ip>
```

**Set static IP:**
```bash
ifconfig vmx0 192.168.10.3 netmask 255.255.255.0 up
```

**Add default gateway(Optional):**
```bash
route add default 192.168.10.1
```

**Verify:**
```bash
ifconfig vmx0
```

Expected output:
```
vmx0: flags=8863<UP,BROADCAST,RUNNING,SIMPLEX,MULTICAST> mtu 1500
    inet 192.168.10.3 netmask 0xffffff00 broadcast 192.168.10.255
    status: active
```

---

### Option 2: Persistent Static IP via Startup Script

Since QNX SDP 8.0 VM boots from an IFS (RAM filesystem), traditional methods don't work. The correct approach is to create a startup script.

#### Method A: Create `rc.sysinit` Script

**1. Create the script:**

```bash
vi /etc/rc.d/rc.sysinit
```

**2. Add network configuration:**

```bash
#!/bin/sh

# Disable DHCP on vmx0 if running
ifconfig vmx0 destroy

# Configure static IP
ifconfig vmx0 192.168.10.3 netmask 255.255.255.0 up

# Add default gateway
route add default 192.168.10.1

# Add SOME/IP multicast route
route add -host 224.224.224.245 192.168.10.3 -ifp vmx0

echo "Static IP configured on vmx0: 192.168.10.3"
```

**3. Make it executable:**

```bash
chmod +x /etc/rc.d/rc.sysinit
```

**4. Ensure it's called during boot:**

Check `/etc/system/config/sysinit`:
```bash
cat /etc/system/config/sysinit
```

If the file exists and doesn't include `rc.sysinit`, add:
```bash
. /etc/rc.d/rc.sysinit
```

**Note:** In some QNX VM configurations, `/etc/system` may not exist or may be read-only. In this case, see Method B below.

---

#### Method B: Modify Boot Image (Advanced)

If your QNX VM does not persist `/etc`, you must modify the boot IFS image.

**1. Locate the boot image:**
```bash
ls /.boot
ls /boot
```

**2. Copy to Ubuntu host:**
```bash
scp /.boot user@192.168.10.1:/home/user/qnx_boot_backup.ifs
```

**3. Extract the IFS on Ubuntu:**
```bash
dumpifs qnx_boot_backup.ifs > extracted_boot
```

**4. Edit the sysinit script inside extracted boot:**

Navigate to the extracted filesystem and modify the startup script to include:
```bash
ifconfig vmx0 192.168.10.3 netmask 255.255.255.0 up
route add default 192.168.10.1
route add -host 224.224.224.245 192.168.10.3 -ifp vmx0
```

**5. Rebuild the IFS:**
```bash
mkifs buildfile new_boot.ifs
```

**6. Replace boot image in VM:**
```bash
scp new_boot.ifs qnxuser@192.168.10.3:/tmp/
ssh qnxuser@192.168.10.3 'cp /tmp/new_boot.ifs /.boot'
```

**7. Reboot QNX VM**

**Note:** This method is complex and requires familiarity with `mkifs` and QNX boot images. Use Method A (rc.sysinit) if possible.

---

### Option 3: Manual Configuration (Quick Test)

For immediate testing without persistence:

```bash
ifconfig vmx0 192.168.10.3 netmask 255.255.255.0
route add default 192.168.10.1
```

---

## Configuring Multicast Routing

SOME/IP Service Discovery uses multicast address `224.224.224.245`. QNX requires an explicit route for this.

### 1. Add Multicast Route

**Correct syntax for QNX:**
```bash
route add -host 224.224.224.245 192.168.10.3 -ifp vmx0
```

**Explanation:**
- `-host`: Single IP route (not a network)
- `224.224.224.245`: SOME/IP-SD multicast address (AUTOSAR standard)
- `192.168.10.3`: Your QNX IP (used as gateway)
- `-ifp vmx0`: Force route through vmx0 interface

### 2. Verify the Route

```bash
route show
```

Expected output should include:
```
224.224.224.245  192.168.10.3  UH  vmx0
```

### 3. Test Multicast Connectivity

From another device (Ubuntu or Raspberry Pi), send a multicast packet:
```bash
ping 224.224.224.245
```

Monitor on QNX:
```bash
tcpdump -i vmx0 -n multicast
```

---

## Complete Network Configuration Script

Create a single script to configure everything:

**File:** `/data/home/qnxuser/scripts/network-setup.sh`

```bash
#!/bin/sh

# QNX Network Configuration for SOME/IP
# Run this script after boot if static IP is not persistent

echo "Configuring QNX network for SOME/IP..."

# Set static IP on vmx0
ifconfig vmx0 192.168.10.3 netmask 255.255.255.0 up

# Add default gateway
route add default 192.168.10.1

# Add SOME/IP multicast route
route add -host 224.224.224.245 192.168.10.3 -ifp vmx0

# Verify configuration
echo "Network interface vmx0:"
ifconfig vmx0

echo ""
echo "Routing table:"
route show

echo ""
echo "Network configuration complete."
echo "QNX IP: 192.168.10.3"
echo "SOME/IP multicast route: 224.224.224.245 via vmx0"
```

**Make it executable:**
```bash
chmod +x /data/home/qnxuser/scripts/network-setup.sh
```

**Run after every boot:**
```bash
/data/home/qnxuser/scripts/network-setup.sh
```

---

## Running SOME/IP Applications

Once networking is configured, you can run SOME/IP services.

### 1. Set Environment Variables

```bash
export LD_LIBRARY_PATH=/data/home/qnxuser/lib
export VSOMEIP_CONFIGURATION=/data/home/qnxuser/config/service.json
```

Make persistent:
```bash
echo 'export LD_LIBRARY_PATH=/data/home/qnxuser/lib' >> ~/.profile
echo 'export VSOMEIP_CONFIGURATION=/data/home/qnxuser/config/service.json' >> ~/.profile
```

### 2. Verify vsomeip Configuration

Your `service.json` must have:
- **unicast**: `"192.168.10.3"` (your QNX static IP)
- **multicast**: `"224.224.224.245"` (correct SOME/IP-SD address)
- **port**: `30490` (as integer, not string)

Example minimal configuration:
```json
{
    "unicast": "192.168.10.3",
    "applications": [
        { "name": "World", "id": "0x1212" }
    ],
    "routing": "World",
    "service-discovery": {
        "enable": true,
        "multicast": "224.224.224.245",
        "port": 30490,
        "protocol": "udp"
    }
}
```

### 3. Run Your SOME/IP Service

```bash
/data/home/qnxuser/bin/service-example
```

Expected output:
```
[info] Application "World" is registered.
[info] Routing manager initialized.
[info] OFFERING service [1234.5678]
[info] Service Discovery started.
```

### 4. Test from Client

On Raspberry Pi or Ubuntu:
```bash
export VSOMEIP_CONFIGURATION=/path/to/client.json
./client-example
```

Expected:
```
[info] Service [1234.5678] discovered at 192.168.10.3
```

---

## Troubleshooting

### Issue: QNX IP changes after reboot

**Cause:** Static IP configuration was not persistent.

**Solution:**
- Use `rc.sysinit` method (Option 2, Method A)
- Or run the network setup script manually after each boot
- Or modify the boot IFS image (Option 2, Method B - advanced)

---

### Issue: Cannot reach QNX from Ubuntu/Raspberry Pi

**Symptoms:**
```bash
ping 192.168.10.3
# No response
```

**Checklist:**
1. Verify QNX has the IP:
   ```bash
   ifconfig vmx0
   ```
2. Verify VMware is in Bridged mode (not NAT)
3. Verify Ubuntu host and QNX are on the same network segment
4. Test from QNX:
   ```bash
   ping 192.168.10.1  # Ubuntu
   ping 192.168.10.2  # Raspberry Pi
   ```

---

### Issue: Service Discovery not working

**Symptoms:** Client cannot discover the service.

**Checklist:**
 **Check firewall rules:**
   - Ensure UDP port 30490 is open
   - Ensure multicast traffic is allowed

---

### Issue: "status: no carrier" on vmx0

**Cause:** Ethernet cable not connected or VMware bridge not configured correctly.

**Solution:**
1. Verify Ethernet cable is connected (if using physical connection)
2. Verify VMware bridging is set to the correct physical adapter (WiFi or Ethernet)
3. Restart VMware networking:
   ```bash
   # On Ubuntu host
   sudo systemctl restart vmware
   ```

---

### Issue: "route: bad address" error

**Cause:** Incorrect route syntax for QNX.

**Wrong (Linux syntax):**
```bash
route add 224.224.224.245 dev vmx0
```

**Correct (QNX syntax):**
```bash
route add -host 224.224.224.245 192.168.10.3 -ifp vmx0
```

---

## Network Topology Reference

### Example Network Setup

```
┌──────────────────┐
│  Ubuntu Host     │
│  192.168.10.1    │
│  (Development)   │
└────────┬─────────┘
         │
    Ethernet Cable
         │
┌────────┴─────────┐
│  Raspberry Pi    │
│  192.168.10.2    │
│  (SOME/IP Client)│
└────────┬─────────┘
         │
    Ethernet via VMware Bridge
         │
┌────────┴─────────┐
│  QNX VM          │
│  192.168.10.3    │
│  (SOME/IP Server)│
└──────────────────┘

All devices communicate via 192.168.10.0/24 network
Service Discovery: 224.224.224.245:30490 (multicast)
```

---

## Summary

After completing this guide, you have:

✅ Configured VMware Bridged networking  
✅ Set a static IP on QNX (192.168.10.3)  
✅ Added default gateway routing  
✅ Configured SOME/IP multicast routing (224.224.224.245)  
✅ Verified network connectivity between devices  
✅ Successfully run SOME/IP applications with service discovery  

Your QNX system is now ready for automotive SOME/IP communication!

---

## Quick Reference Commands

```bash
# Set static IP
ifconfig vmx0 192.168.10.3 netmask 255.255.255.0 up

# Add default gateway
route add default 192.168.10.1

# Add SOME/IP multicast route
route add -host 224.224.224.245 192.168.10.3 -ifp vmx0

# Verify configuration
ifconfig vmx0
route show

# Set environment for SOME/IP
export LD_LIBRARY_PATH=/data/home/qnxuser/lib
export VSOMEIP_CONFIGURATION=/data/home/qnxuser/config/service.json

# Run service
/data/home/qnxuser/bin/service-example
```