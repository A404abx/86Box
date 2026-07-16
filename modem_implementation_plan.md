# ISA and PCI Modem Implementation Plan

## Overview
The goal is to implement internal ISA and PCI modems for 86Box. These modems will combine a 16550A UART emulation with the existing Hayes-compliant AT command and TCP/IP-based virtual line logic.

## 1. Refactoring existing Modem Logic
Currently, `net_modem.c` implements a modem that attaches to an existing COM port. To support ISA and PCI modems, we need to make this logic reusable.

### Proposed Changes:
- Create `src/include/86box/modem.h` to define the modem structure and functions.
- Refactor `src/network/net_modem.c` to separate the core modem logic from the COM port attachment.
- The core logic should handle:
    - AT command parsing.
    - S-registers.
    - TCP/IP socket connection (acting as the phone line).
    - SLIP/PPP (if applicable).
    - Interfacing with a `serial_t` structure.

## 2. ISA Modem Implementation
An internal ISA modem is essentially a UART at a configurable IO base and IRQ, with the modem logic attached.

### Implementation Details:
- New file: `src/network/net_modem_isa.c`.
- Device name: "Internal ISA Modem".
- Configuration:
    - Base IO (0x3f8, 0x2f8, 0x3e8, 0x2e8, etc.).
    - IRQ (3, 4, 5, 7, etc.).
- The device will instantiate a `serial_t` and a `modem_t`.
- It will use `io_sethandler` to register the UART registers.

## 3. PCI Modem Implementation
A PCI modem is a PCI device that provides a UART. Many hardware PCI modems follow the 16550 interface.

### Implementation Details:
- New file: `src/network/net_modem_pci.c`.
- Device name: "Standard PCI Modem".
- PCI Configuration:
    - Vendor/Device ID (e.g., a common generic one or a specific chip like NetMos 9835).
    - BAR0: IO space for UART registers (8 bytes).
    - Interrupt Pin: INTA.
- The device will instantiate a `serial_t` and a `modem_t`.
- It will handle PCI configuration space reads/writes.

## 4. Work Stages
1. **Stage 1**: Define `modem_t` and common functions in a header and shared source.
2. **Stage 2**: Update `net_modem.c` to use the shared logic.
3. **Stage 3**: Implement the ISA modem device.
4. **Stage 4**: Implement the PCI modem device.
5. **Stage 5**: Testing and verification (by user).

## 5. Register Maps (Internal)
The modems will use standard NS16550A register maps for the serial interface.
The AT command set will remain compatible with the existing implementation in `net_modem.c`.
