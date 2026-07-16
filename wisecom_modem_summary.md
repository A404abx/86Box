# Wisecom WS-5614HMMG Implementation

The Wisecom WS-5614HMMG is a PCI modem based on the Conexant (formerly Rockwell) chipset. 

## Implementation in 86Box:

### 1. Chipset Identification:
- **Vendor ID**: `14F1` (Conexant)
- **Device ID**: `1035` (RH56D/SP-PCI Hardware Modem)
- **Subsystem Vendor**: `1436` (Wisecom)
- **Class Code**: `070000` (Serial Controller)

### 2. Emulation Strategy:
- This specific model belongs to the "Hardware Modem" family of Conexant PCI modems, which means it integrates a hardware UART.
- In 86Box, it is implemented as a standard 16550A UART mapped to PCI IO space (BAR0).
- It leverages the shared virtual modem logic for AT commands and TCP/IP connectivity.

## Changes:
- **`src/network/net_modem_pci.c`**:
    - Updated to support multiple PCI modem models.
    - Added the `cis_ws5614_device` with the specific Conexant/Wisecom IDs.
- **`src/include/86box/network.h`**: Added extern declaration.
- **`src/network/network.c`**: Registered the device in the network adapter list.

## Configuration:
The device appears in the "Network" settings as "Wisecom WS-5614HMMG (PCI Modem)".
Users can configure:
- **COM Port Index**: Internal COM port mapping.
- **Baud Rate**, **TCP/IP Port**, **Telnet Mode**, and **Phonebook File**.

## How to test:
1. Select the "Wisecom WS-5614HMMG (PCI Modem)" in the Network configuration.
2. Boot the guest OS (e.g., Windows 98).
3. The OS should detect a new PCI device.
4. Install the appropriate Conexant/Wisecom drivers or a generic "Hardware Modem on PCI Bus" driver.
5. Verify communication via HyperTerminal.
