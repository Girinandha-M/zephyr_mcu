# MSPM0 Secure Boot with Zephyr - Step by Step Guide

This guide shows you how to set up secure boot on TI MSPM0 using Zephyr RTOS.

## What You'll Build
- **Secure Bootloader** that verifies applications before running them
- **BCR Security** that protects your code from tampering
- **Signed Applications** that only boot if they're authentic

## Hardware You Need
- **MSPM0G3507** OR **MSPM0L2228**

## Before You Start
# Step 1: New to Zephyr? Set up your workspace first:
- **📋 [Official Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)** - Complete installation instructions
---
# Step 2: Get source code:
## For BCR config application
```bash
cd ~/zephyrproject/zephyr

git remote add mcuboot git@gitlab.com:linumiz/linumiz/zephyr.git

git fetch mcuboot
```

## For Bootloader
```bash
cd ~/zephyrproject/bootloader/mcuboot

git remote add mcuboot git@gitlab.com:linumiz/linumiz/zephyr.git

git fetch mcuboot
```
---

## For MSPM0G3507
Flash Bootloader first, then BCR config.

## 1. Build and Flash Bootloader
```bash
cd ~/zephyrproject/bootloader/mcuboot

git checkout mcuboot/mcuboot_G3507

rm -rf build && west build -b lp_mspm0g3507 -s boot/zephyr
```

## 2. Flashing using openocd runner
```bash
west flash --openocd $OPENOCD_DIR/src/openocd --openocd-search $OPENOCD_DIR/tcl
```

## 3. BCR config for static write protection
```bash
cd ~/zephyrproject/zephyr

git checkout mcuboot/secure_boot_G3507

rm -rf build && west build -b lp_mspm0g3507 -s samples/hello_world

west flash --openocd $OPENOCD_DIR/src/openocd --openocd-search $OPENOCD_DIR/tcl
```

---

# For MSPM0L2228
Flash BCR config first, then Bootloader (To tell the BCR that CSC exits first)

## 1. Build and Flash BCR config
```bash
cd ~/zephyrproject/zephyr

git checkout mcuboot/secure_boot_L2228

rm -rf build && west build -b lp_mspm0l2228 -s samples/hello_world

west flash --openocd $OPENOCD_DIR/src/openocd --openocd-search $OPENOCD_DIR/tcl
```

Now the BCR will not pass initdone.

## 2. Building and Flashing Bootloader
```bash
cd ~/zephyrproject/bootloader/mcuboot

git checkout mcuboot/mcuboot_L2228

rm -rf build && west build -b lp_mspm0l2228 -s boot/zephyr

west flash --openocd $OPENOCD_DIR/src/openocd --openocd-search $OPENOCD_DIR/tcl
```

---
