# Modem Implementation Summary

The ISA and PCI modem implementation has been completed following the multi-stage plan.

## Changes Made:

### Core refactoring:
- **`src/include/86box/modem.h`**: Created new header to share modem logic between COM-attached modems and internal modems.
- **`src/network/modem.c`**: Extracted core Hayes AT command and virtual line logic from `net_modem.c` into this shared file.
- **`src/network/net_modem.c`**: Refactored to act as a wrapper around the shared modem logic for external COM port modems.

### Serial core enhancements:
- **`src/include/86box/serial.h`**: Added an `irq_callback` to `serial_device_t` to allow PCI devices to handle interrupts via the PCI bus instead of direct PIC interrupts.
- **`src/device/serial.c`**: 
    - Updated `serial_do_irq` to call the new callback if present.
    - Updated `serial_init` to support forcing a specific COM port instance via `device_add_inst`, allowing internal cards to target specific ports (COM1-COM8).

### New devices:
- **`src/network/net_modem_isa.c`**: Implemented the "Internal ISA Modem" device. It allows users to configure the target COM port, Base IO address, and IRQ.
- **`src/network/net_modem_pci.c`**: Implemented the "Standard PCI Modem" device. It appears as a PCI serial controller (NetMos 9835 compatible) and handles interrupts via the PCI bus.
- **`src/network/network.c`**: Registered the new devices so they appear in the "Network" configuration section (consistent with the existing modem).
- **`src/include/86box/network.h`**: Added extern declarations for the new devices.

### Build system:
- **`src/network/CMakeLists.txt`**: Added new source files to the build.

## How to test:
1. Rebuild 86Box.
2. Open the configuration.
3. Go to the "Network" section.
4. You should now see "Internal ISA Modem" and "Standard PCI Modem" in addition to the standard modem.
5. For ISA modem, configure the Base IO and IRQ to avoid conflicts with existing ports.
6. For PCI modem, the OS should automatically detect it as a serial port.
7. Use any terminal software (like Terminal.exe in Windows 3.1 or HyperTerminal in 9x) to talk to the modem using AT commands.
