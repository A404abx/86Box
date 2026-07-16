# Aztech Sound Galaxy Washington 16 (Modem Combo) Implementation

The Aztech Sound Galaxy Washington 16 is a multi-function ISA card that combines:
1.  **Sound**: Aztech Sound Galaxy 16 (Pro 16 AB compatible, based on AZT2316 chip).
2.  **Modem**: Rockwell R6674-16 (14.4kbps controller).

## Implementation in 86Box:

### 1. Sound Part
- Utilizes the existing `azt2316a` (Washington) emulation in `src/sound/snd_azt2316a.c`.
- This provides Sound Blaster Pro, Windows Sound System, OPL3, and MPU-401 compatibility.

### 2. Modem Part
- Leverages the newly refactored shared modem logic (`src/network/modem.c`).
- Spawns a dedicated NS16550A UART instance via `device_add_inst(&ns16550_device, port + 1)`.
- The UART is attached to the virtual modem logic which handles AT commands and TCP/IP line emulation.

## Changes:
- **`src/include/86box/snd_sb_dsp.h`**: Defined `SB_SUBTYPE_CLONE_AZT_WASH_MODEM` (Type 2).
- **`src/sound/snd_azt2316a.c`**:
    - Added `azt_wash_modem_device`.
    - Implemented `azt_wash_modem_init` and `azt_wash_modem_close`.
    - Integrated modem configuration (COM port, IRQ, Address, etc.) into the sound card's config UI.
- **`src/sound/sound.c`**: Registered the new combo card.

## Configuration Options:
When selecting "Aztech Sound Galaxy Washington 16 (Modem)", the user can configure:
- Sound parameters (Base, IRQ, DMA, WSS IRQ/DMA).
- Modem parameters:
    - **COM Port Index**: COM1 through COM8.
    - **Modem Base Address**: Standard choices like 0x2f8 (COM2), 0x3e8 (COM3), etc.
    - **Modem IRQ**: Standard ISA IRQs.
    - **Baud Rate**, **TCP/IP Port**, **Telnet Mode**, and **Phonebook File**.

## How to test:
1. Select the "Aztech Sound Galaxy Washington 16 (Modem)" in the Sound configuration.
2. In the device settings, configure the Modem COM port (e.g., COM2 at 0x2f8 IRQ 3).
3. Boot the guest OS.
4. The OS should detect a sound card AND a new serial port.
5. In the guest, use a terminal program to talk to the modem at the assigned COM port.
