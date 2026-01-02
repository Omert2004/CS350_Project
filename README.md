# CS350 Project – Secure Boot Framework for STM32

![Platform](https://img.shields.io/badge/platform-STM32-blue.svg)
![Language](https://img.shields.io/badge/language-C-green.svg)
![Framework](https://img.shields.io/badge/framework-STM32CubeIDE-orange.svg)
![Course](https://img.shields.io/badge/course-CS350-lightgrey.svg)

**Platform:** STM32F746G / STM32F411  
**Language:** C  
**Framework:** STM32CubeIDE (HAL)  
**Cryptography:** TinyCrypt (software-based)  
**Course:** CS350 – Operating Systems  
**University:** Özyeğin University  

---

## 📘 Project Overview

**CS350_Project** is a **software-based Secure Boot Framework** designed for STM32 microcontrollers that lack advanced hardware security features such as TrustZone.

The framework establishes a **Root of Trust (RoT)** entirely in software and provides:

- Firmware authenticity using **ECDSA-P256**
- Firmware integrity using **SHA-256**
- Optional firmware confidentiality using **AES-128**
- Runtime memory isolation via the **ARM Cortex-M MPU**
- Fail-safe firmware updates using a **dual-slot architecture**

The system has been tested on **STM32F746G (Cortex-M7)** and successfully ported to **STM32F411 (Cortex-M4)**.

---

## ⚙️ Key Features

- Secure boot with **cryptographic verification before execution**
- Software-based **Root of Trust**
- **ECDSA-P256** signature verification
- **SHA-256** hash validation
- **AES-128-CBC** encrypted firmware updates
- Dual-slot firmware architecture (Active / Download)
- **Automatic recovery** if active firmware is invalid
- **Manual rollback** via user button
- **MPU-enforced memory protection**
- Transport-layer independent update mechanism
- Lightweight bootloader (~32 KB after optimization)

---

## 🧠 System Architecture

### 📦 Memory Layout (STM32F746G Example)

Flash memory is partitioned into logical regions aligned with STM32 flash sectors:

| Sector(s) | Purpose |
|----------|---------|
| 0–1 | Secure Bootloader (Root of Trust) |
| 2 | Configuration Sector (persistent boot state) |
| 3–4 | User Reserved Area |
| 5 | Active Application Slot (Slot A) |
| 6 | Download Slot (Slot B) |
| 7 | Scratchpad (temporary buffer during updates) |

> The layout is configurable via `mem_layout.h` to support different STM32 families.

---

## 🔐 Security Mechanisms

### Cryptography

- **SHA-256** for firmware integrity
- **ECDSA-P256** for authenticity verification
- **AES-128-CBC** for encrypted firmware transmission
- **AES-128-ECB** for backup encryption during swap

All cryptographic operations are implemented using **TinyCrypt** in software, ensuring portability across STM32 devices without hardware crypto accelerators.

---

### Memory Protection (MPU)

- Bootloader flash marked as **Privileged Read-Only**
- Prevents application code from modifying the bootloader
- Configuration sector remains **Read-Write** to store FSM state

---

## 🔁 Boot Flow & State Machine

On every reset, the bootloader executes a finite state machine:

1. System initialization (clock, HAL, interrupts)
2. Read boot configuration from Sector 2
3. State handling:
   - `STATE_NORMAL` → Verify and jump to Active Application
   - `STATE_UPDATE_REQ` → Perform secure firmware update
   - `STATE_ROLLBACK` → Restore previous firmware
4. Verification steps:
   - ECDSA signature verification
   - SHA-256 hash validation
   - Reset vector sanity check
5. Automatic recovery if Active Slot is invalid
6. System halt with LED indication if no valid firmware exists

---

## 🔄 Firmware Update Logic

The **application**, not the bootloader, is responsible for firmware transport.

### Update Procedure

1. Application receives encrypted firmware (UART used in demo)
2. Firmware is written to Download Slot (Sector 6)
3. Application sets `STATE_UPDATE_REQ` in Configuration Sector
4. System reset triggers the bootloader update flow

---

### 🔐 Atomic Swap Process

To guarantee power-failure safety:

1. Decrypt new firmware into Scratchpad
2. Backup active firmware into Download Slot
3. Copy new firmware to Active Slot
4. Reset state to `STATE_NORMAL`

---

## 📌 Application Requirements

Applications compatible with this bootloader must:

- Include shared `mem_layout.h`
- Include `bootloader_interface.h`
- Be linked to start at Active Slot address (e.g. `0x08040000`)
- Relocate vector table to the new flash offset
- Re-enable interrupts at startup using `__enable_irq()`

---

## 🛠 Firmware Packaging

A Python-based packaging toolchain is used:

- Computes SHA-256 hash
- Signs firmware using ECDSA private key
- Encrypts firmware using AES-128-CBC
- Appends metadata footer (signature, version, hash)

The output is a **secure firmware update binary** ready for transmission.

---

## 🧪 Testing & Validation

The framework was validated using:

- MPU violation tests (intentional bootloader write attempts)
- Cryptographic self-tests using known test vectors
- End-to-end update scenarios:
  - Empty system halt
  - Auto-provisioning from Download Slot
  - Manual rollback
  - Firmware swapping
- Cross-family portability test on STM32F411

---

## 🧰 Development Environment

| Component | Tool |
|---------|------|
| MCU | STM32F746G / STM32F411 |
| IDE | STM32CubeIDE |
| Language | C |
| Compiler | ARM GCC |
| Crypto Library | TinyCrypt |
| Debugger | ST-Link |
| OS | Windows / Linux |

---

## 📂 Repository Structure

```text
CS350_Project/
│
├── Bootloader/
│   ├── Core/
│   ├── Drivers/
│   ├── Cryptology/
│   ├── BL_Functions/
│   └── main.c
│
├── Application/
│   ├── Core/
│   ├── Drivers/
│   └── main.c
│
├── Tools/
│   ├── keygen.py
│   ├── extract_pubkey.py
│   └── generate_update.py
│
├── Documentation/
│   └── CS350_Final_Report.pdf
│
└── README.md
```
---

## 👥 Authors

- **Oğuz Mert Coşkun**  
  🔗 GitHub: https://github.com/Omert2004

- **Mehmet Arda Güler**  
  🔗 GitHub: https://github.com/mardaguler

- **Mert Kırgın**  
  🔗 GitHub: https://github.com/mertkirgin

**Electrical & Electronics Engineering**  
**Özyeğin University**

---

## 🧩 Keywords

`STM32` `Secure Boot` `Bootloader` `Embedded Security`  
`ECDSA` `SHA-256` `AES-128` `MPU`  
`Firmware Update` `Dual Slot` `Root of Trust`
