# 📘 Installing vsomeip in Yocto

---

## 1. Introduction

This guide explains how to integrate **vsomeip** (SOME/IP middleware) into a Yocto-based embedded Linux image.
The process includes:

1. Creating the recipe using **devtool**
2. Building vsomeip inside Yocto
3. Fixing packaging & `/usr/etc` QA errors
4. Installing default JSON configs properly
5. Adding the library to your Yocto image
6. Verifying the installation on the target (e.g., Raspberry Pi)

This method works for any Yocto-based system.

---

## 2. Required Yocto Setup

Set up the Yocto build environment:

```bash
source oe-init-build-env
```

Make sure you have a custom layer (e.g. `meta-custom`) to host new recipes.

---

## 3. Step 1 — Create the Recipe Using devtool

Yocto’s recommended workflow for new recipes is:

```bash
devtool add vsomeip https://github.com/COVESA/vsomeip.git
```

This action:

* Clones the source into:

  ```
  workspace/sources/vsomeip
  ```
* Creates a starter recipe at:

  ```
  workspace/recipes/vsomeip/vsomeip_git.bb
  ```

Now you have a working base.

---

## 4. Step 2 — Build the Recipe Using devtool

```bash
devtool build vsomeip
```

If successful, you can deploy it (optional):

```bash
devtool deploy-target vsomeip root@<TARGET_IP>
```

---

## 5. Step 3 — Understanding the Packaging Error

### ❌ The Common Error

```
ERROR: vsomeip: Files/directories were installed but not shipped in any package:
  /usr/etc
  /usr/etc/vsomeip
  /usr/etc/vsomeip/vsomeip.json
```

### ✔️ Why This Happens

vsomeip installs default configs into:

```
${prefix}/etc/vsomeip → /usr/etc/vsomeip
```

But unless you explicitly declare these paths in the recipe, Yocto raises a packaging QA error.

---

## 6. Step 4 — Fixing Packaging and Installing JSON Files

We fix this by:

1. Creating the directory inside `${D}`
2. Copying JSON files into `/usr/etc/vsomeip`
3. Declaring these paths inside `FILES:${PN}`

---

## 7. Final Working Recipe

### **vsomeip_git.bb**

```
# Recipe created by recipetool

LICENSE = "Unknown"
LIC_FILES_CHKSUM = "file://LICENSE;md5=9741c346eef56131163e13b9db1241b3"

SRC_URI = "git://github.com/COVESA/vsomeip.git;protocol=https;branch=master"

PV = "1.0+git${SRCPV}"
SRCREV = "c4e0db329da9b63f511f3c2456c040582daf9305"

S = "${WORKDIR}/git"

DEPENDS = "systemd boost"

inherit cmake pkgconfig

EXTRA_OECMAKE = ""

do_install:append() {
        install -d ${D}${prefix}/etc/vsomeip
        cp -r ${S}/config/*.json ${D}${prefix}/etc/vsomeip/
}

FILES:${PN} += " \
    ${bindir} \
    ${prefix}/etc/vsomeip \
    ${prefix}/etc/*.json \
"
```

---

## 8. Explanation of Key Fixes

### ✔️ Why do_install:append() is required?

vsomeip installs its JSON configs into `${prefix}/etc/vsomeip`.
We manually ensure this directory exists and copy the configs:

```bash
install -d ${D}${prefix}/etc/vsomeip
cp -r ${S}/config/*.json ${D}${prefix}/etc/vsomeip/
```

### ✔️ Why FILES:${PN} is required?

Yocto must be told to package these installed files:

```
${prefix}/etc/vsomeip
${prefix}/etc/*.json
${bindir}
```

This resolves the `installed-vs-shipped` QA error.

### ✔️ Why `${prefix}/etc` becomes `/usr/etc`?

Because:

* `${prefix}` = `/usr`
* So:
  `/usr/etc` is valid and expected

Yocto keeps system files at `/etc` for the system, `/usr/etc` for applications.

---

## 9. Step 5 — Add vsomeip to Your Image

Inside `local.conf` or your custom image recipe:

```
IMAGE_INSTALL:append = " vsomeip"
```

Rebuild:

```bash
bitbake core-image-minimal
```

---

## 10. Step 6 — Verify Installation on the Target

After flashing the image, verify vsomeip:

### ✔️ Libraries:

```bash
ls /usr/lib | grep someip
```

### ✔️ Header files:

```bash
ls /usr/include/vsomeip
```

### ✔️ JSON configuration files:

```bash
ls /usr/etc/vsomeip
```

Expected files:

```
vsomeip.json
vsomeip-local.json
vsomeip-tcp-service.json
...
```

### ✔️ Example binaries (if enabled):

```bash
ls /usr/bin | grep someip
```

---

## 11. Step 7 — Running Test Applications

Start service:

```bash
simple-someip-service
```

Start client:

```bash
simple-someip-client
```

If the client discovers the service and exchanges messages, the installation is confirmed working.

---

# ✔️ Final Summary

### Devtool workflow:

```bash
devtool add vsomeip https://github.com/COVESA/vsomeip.git
devtool build vsomeip
```

### Packaging fix added to recipe:

* Install JSON files into `${prefix}/etc/vsomeip`
* Add directory to `FILES:${PN}`

### Add package to image:

```
IMAGE_INSTALL:append = " vsomeip"
```

### Verify:

Check `/usr/lib`, `/usr/include`, `/usr/etc/vsomeip`, and someip example binaries.

The vsomeip library is now fully integrated into your Yocto image.
