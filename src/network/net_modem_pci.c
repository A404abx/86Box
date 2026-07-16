/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          This file is part of the 86Box distribution.
 *
 *          Standard PCI Modem emulation.
 *
 * Authors: Arena AI
 *
 *          Copyright 2024 Arena AI.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
#include <stdbool.h>
#define HAVE_STDARG_H
#include <86box/86box.h>
#include <86box/device.h>
#include <86box/serial.h>
#include <86box/network.h>
#include <86box/pci.h>
#include <86box/modem.h>

typedef struct pci_modem_s {
    modem_t *modem;
    serial_t *serial;
    uint8_t pci_slot;
    uint8_t regs[256];
    uint8_t irq_state;
    int port_index;
} pci_modem_t;

extern void modem_read_phonebook_file(modem_t *modem, const char *path);

static void
pci_modem_irq_handler(struct serial_s *serial, void *priv, int set)
{
    pci_modem_t *dev = (pci_modem_t *) priv;
    pci_irq(dev->pci_slot, PCI_INTA, 1, set, &dev->irq_state);
}

static void
pci_modem_write(int func, int addr, int len, uint8_t val, void *priv)
{
    pci_modem_t *dev = (pci_modem_t *) priv;

    if (func > 0)
        return;

    switch (addr) {
        case PCI_REG_COMMAND:
            dev->regs[addr] = val & 0x47;
            break;
        case 0x10: /* BAR0 Low */
            dev->regs[addr] = (val & 0xf8) | 0x01;
            serial_setup(dev->serial, (dev->regs[0x11] << 8) | dev->regs[0x10], 0xff);
            break;
        case 0x11: /* BAR0 High */
            dev->regs[addr] = val;
            serial_setup(dev->serial, (dev->regs[0x11] << 8) | dev->regs[0x10], 0xff);
            break;
        case 0x3c: /* Interrupt Line */
            dev->regs[addr] = val;
            break;
        default:
            break;
    }
}

static uint8_t
pci_modem_read(int func, int addr, int len, void *priv)
{
    pci_modem_t *dev = (pci_modem_t *) priv;

    if (func > 0)
        return 0xff;

    return dev->regs[addr];
}

static void *
pci_modem_init(const device_t *info)
{
    pci_modem_t *dev = (pci_modem_t *) calloc(1, sizeof(pci_modem_t));
    const char *phonebook_file = NULL;

    int port = device_get_config_int("port"); // 0-7 for COM1-COM8
    dev->port_index = port;

    /* Ensure the port is enabled in the serial core. */
    com_ports[port].enabled = 1;

    /* Initialize PCI configuration space. */
    memset(dev->regs, 0, 256);
    if (info->local == 1) {
        /* CIS WS-5614HMMG (Hardware バリアント: Conexant RH56D/SP-PCI) */
        dev->regs[0x00] = 0xf1; dev->regs[0x01] = 0x14; /* Vendor: Conexant (14F1) */
        dev->regs[0x02] = 0x35; dev->regs[0x03] = 0x10; /* Device: 1035 */
        dev->regs[0x08] = 0x01; /* Revision */
        dev->regs[0x09] = 0x00; dev->regs[0x0a] = 0x00; dev->regs[0x0b] = 0x07; /* Class: Communication Controller */
        dev->regs[0x2c] = 0x36; dev->regs[0x2d] = 0x14; /* Subsystem Vendor: Wisecom (1436) */
        dev->regs[0x2e] = 0x35; dev->regs[0x2f] = 0x10; /* Subsystem ID */
    } else if (info->local == 2) {
        /* Wisecom WS-5614HMMG (HSF Data/Fax) */
        dev->regs[0x00] = 0x7a; dev->regs[0x01] = 0x12; /* Vendor: Rockwell (127A) */
        dev->regs[0x02] = 0x13; dev->regs[0x03] = 0x20; /* Device: 2013 */
        dev->regs[0x08] = 0x01; /* Revision */
        dev->regs[0x09] = 0x00; dev->regs[0x0a] = 0x00; dev->regs[0x0b] = 0x07; /* Class: Communication Controller */
        dev->regs[0x2c] = 0x36; dev->regs[0x2d] = 0x14; /* Subsystem Vendor: Wisecom (1436) */
        dev->regs[0x2e] = 0x13; dev->regs[0x2f] = 0x21; /* Subsystem ID: 2113 */
    } else if (info->local == 3) {
        /* Wisecom WS-5614HMMG (HSF Voice/Speakerphone) */
        dev->regs[0x00] = 0x7a; dev->regs[0x01] = 0x12; /* Vendor: Rockwell (127A) */
        dev->regs[0x02] = 0x15; dev->regs[0x03] = 0x20; /* Device: 2015 */
        dev->regs[0x08] = 0x01; /* Revision */
        dev->regs[0x09] = 0x00; dev->regs[0x0a] = 0x00; dev->regs[0x0b] = 0x07; /* Class: Communication Controller */
        dev->regs[0x2c] = 0x36; dev->regs[0x2d] = 0x14; /* Subsystem Vendor: Wisecom (1436) */
        dev->regs[0x2e] = 0x15; dev->regs[0x2f] = 0x21; /* Subsystem ID: 2115 */
    } else if (info->local == 4) {
        /* Wisecom WS-5614PM3 (HCF) */
        dev->regs[0x00] = 0x7a; dev->regs[0x01] = 0x12; /* Vendor: Rockwell (127A) */
        dev->regs[0x02] = 0x03; dev->regs[0x03] = 0x10; /* Device: 1003 */
        dev->regs[0x08] = 0x01; /* Revision */
        dev->regs[0x09] = 0x00; dev->regs[0x0a] = 0x00; dev->regs[0x0b] = 0x07; /* Class: Communication Controller */
        dev->regs[0x2c] = 0x36; dev->regs[0x2d] = 0x14; /* Subsystem Vendor: Wisecom (1436) */
        dev->regs[0x2e] = 0x03; dev->regs[0x2f] = 0x10; /* Subsystem ID */
    } else if (info->local == 5) {
        /* Wisecom WS-5614PM3D (HCF) */
        dev->regs[0x00] = 0xf1; dev->regs[0x01] = 0x14; /* Vendor: Conexant (14F1) */
        dev->regs[0x02] = 0x34; dev->regs[0x03] = 0x10; /* Device: 1034 */
        dev->regs[0x08] = 0x01; /* Revision */
        dev->regs[0x09] = 0x00; dev->regs[0x0a] = 0x00; dev->regs[0x0b] = 0x07; /* Class: Communication Controller */
        dev->regs[0x2c] = 0x36; dev->regs[0x2d] = 0x14; /* Subsystem Vendor: Wisecom (1436) */
        dev->regs[0x2e] = 0x34; dev->regs[0x2f] = 0x10; /* Subsystem ID */
    } else if (info->local == 6) {
        /* Generic Conexant HSF Modem */
        dev->regs[0x00] = 0x7a; dev->regs[0x01] = 0x12; /* Vendor: Rockwell (127A) */
        dev->regs[0x02] = 0x13; dev->regs[0x03] = 0x20; /* Device: 2013 */
        dev->regs[0x08] = 0x01; /* Revision */
        dev->regs[0x09] = 0x00; dev->regs[0x0a] = 0x00; dev->regs[0x0b] = 0x07; /* Class: Communication Controller */
        dev->regs[0x2c] = 0x7a; dev->regs[0x2d] = 0x12; /* Subsystem Vendor */
        dev->regs[0x2e] = 0x13; dev->regs[0x2f] = 0x20; /* Subsystem ID */
    } else {
        /* Standard PCI Modem (NetMos 9835) */
        dev->regs[0x00] = 0x10; dev->regs[0x01] = 0x97; /* Vendor: NetMos */
        dev->regs[0x02] = 0x35; dev->regs[0x03] = 0x98; /* Device: 9835 */
        dev->regs[0x08] = 0x01; /* Revision */
        dev->regs[0x09] = 0x02; dev->regs[0x0a] = 0x00; dev->regs[0x0b] = 0x07; /* Class: Serial Controller */
        dev->regs[0x2c] = 0x00; dev->regs[0x2d] = 0x10; /* Subsystem Vendor */
        dev->regs[0x2e] = 0x01; dev->regs[0x2f] = 0x00; /* Subsystem ID */
    }
    dev->regs[0x0e] = 0x00; /* Header Type: Normal */
    dev->regs[0x10] = 0x01; /* BAR0: IO */
    dev->regs[0x3d] = 0x01; /* Interrupt Pin: INTA */

    /* Add a UART instance. */
    dev->serial = device_add_inst(&ns16550_device, port + 1);
    /* We don't call serial_setup yet, wait for BAR0 write. */

    /* Initialize modem logic. */
    dev->modem = (modem_t *) calloc(1, sizeof(modem_t));
    memset(dev->modem->mac, 0xfc, 6);
    dev->modem->baudrate = device_get_config_int("baudrate");
    dev->modem->listen_port = device_get_config_int("listen_port");
    dev->modem->telnet_mode = device_get_config_int("telnet_mode");

    modem_init_common(dev->modem);

    /* Attach modem logic and IRQ handler to our internal UART. */
    dev->modem->serial = serial_attach_ex_3(port, modem_rcr_cb, modem_write, 
                                            modem_dtr_callback, pci_modem_irq_handler, dev->modem);

    modem_reset(dev->modem);
    dev->modem->card = network_attach(dev->modem, dev->modem->mac, modem_rx, NULL);

    phonebook_file = device_get_config_string("phonebook_file");
    if (phonebook_file && phonebook_file[0] != 0) {
        modem_read_phonebook_file(dev->modem, phonebook_file);
    }

    /* Register as a PCI device. */
    pci_add_card(PCI_ADD_NORMAL, pci_modem_read, pci_modem_write, dev, &dev->pci_slot);

    return dev;
}

static void
pci_modem_close(void *priv)
{
    pci_modem_t *dev = (pci_modem_t *) priv;
    modem_close_common(dev->modem);
    netcard_close(dev->modem->card);
    free(dev->modem);
    free(dev);
}

static const device_config_t pci_modem_config[] = {
    {
        .name           = "port",
        .description    = "COM Port Index",
        .type           = CONFIG_SELECTION,
        .default_string = NULL,
        .default_int    = 4, /* Default to COM5 */
        .file_filter    = NULL,
        .spinner        = { 0 },
        .selection      = {
            { .description = "COM1", .value = 0 },
            { .description = "COM2", .value = 1 },
            { .description = "COM3", .value = 2 },
            { .description = "COM4", .value = 3 },
            { .description = "COM5", .value = 4 },
            { .description = "COM6", .value = 5 },
            { .description = "COM7", .value = 6 },
            { .description = "COM8", .value = 7 },
            { .description = ""                 }
        },
        .bios           = { { 0 } }
    },
    {
        .name           = "baudrate",
        .description    = "Baud Rate",
        .type           = CONFIG_SELECTION,
        .default_string = NULL,
        .default_int    = 115200,
        .file_filter    = NULL,
        .spinner        = { 0 },
        .selection      = {
            { .description = "115200", .value = 115200 },
            { .description =  "57600", .value =  57600 },
            { .description =  "56000", .value =  56000 },
            { .description =  "38400", .value =  38400 },
            { .description =  "33600", .value =  33600 },
            { .description =  "28800", .value =  28800 },
            { .description =  "19200", .value =  19200 },
            { .description =  "14400", .value =  14400 },
            { .description =   "9600", .value =   9600 },
            { .description =   "7200", .value =   7200 },
            { .description =   "4800", .value =   4800 },
            { .description =   "2400", .value =   2400 },
            { .description =   "1800", .value =   1800 },
            { .description =   "1200", .value =   1200 },
            { .description =    "600", .value =    600 },
            { .description =    "300", .value =    300 },
            { .description = ""                        }
        },
        .bios           = { { 0 } }
    },
    {
        .name           = "listen_port",
        .description    = "TCP/IP listening port",
        .type           = CONFIG_SPINNER,
        .default_string = NULL,
        .default_int    = 0,
        .file_filter    = NULL,
        .spinner        = {
            .min =     0,
            .max = 32767
        },
        .selection      = { { 0 } },
        .bios           = { { 0 } }
    },
    {
        .name           = "phonebook_file",
        .description    = "Phonebook File",
        .type           = CONFIG_FNAME,
        .default_string = NULL,
        .file_filter    = "Text files (*.txt)|*.txt",
        .spinner        = { 0 },
        .selection      = { { 0 } },
        .bios           = { { 0 } }
    },
    {
        .name           = "telnet_mode",
        .description    = "Telnet emulation",
        .type           = CONFIG_BINARY,
        .default_string = NULL,
        .default_int    = 0,
        .file_filter    = NULL,
        .spinner        = { 0 },
        .selection      = { { 0 } },
        .bios           = { { 0 } }
    },
    { .name = "", .description = "", .type = CONFIG_END }
};

const device_t pci_modem_device = {
    .name          = "Standard PCI Modem",
    .internal_name = "pci_modem",
    .flags         = DEVICE_PCI,
    .local         = 0,
    .init          = pci_modem_init,
    .close         = pci_modem_close,
    .reset         = NULL,
    .available     = NULL,
    .speed_changed = modem_speed_changed,
    .force_redraw  = NULL,
    .config        = pci_modem_config
};

const device_t cis_ws5614_device = {
    .name          = "Wisecom WS-5614HMMG (Hardware PCI Modem)",
    .internal_name = "cis_ws5614",
    .flags         = DEVICE_PCI,
    .local         = 1,
    .init          = pci_modem_init,
    .close         = pci_modem_close,
    .reset         = NULL,
    .available     = NULL,
    .speed_changed = modem_speed_changed,
    .force_redraw  = NULL,
    .config        = pci_modem_config
};

const device_t cis_ws5614_hsf_device = {
    .name          = "Wisecom WS-5614HMMG (HSF Data/Fax PCI)",
    .internal_name = "cis_ws5614_hsf",
    .flags         = DEVICE_PCI,
    .local         = 2,
    .init          = pci_modem_init,
    .close         = pci_modem_close,
    .reset         = NULL,
    .available     = NULL,
    .speed_changed = modem_speed_changed,
    .force_redraw  = NULL,
    .config        = pci_modem_config
};

const device_t cis_ws5614_voice_device = {
    .name          = "Wisecom WS-5614HMMG (HSF Voice PCI)",
    .internal_name = "cis_ws5614_voice",
    .flags         = DEVICE_PCI,
    .local         = 3,
    .init          = pci_modem_init,
    .close         = pci_modem_close,
    .reset         = NULL,
    .available     = NULL,
    .speed_changed = modem_speed_changed,
    .force_redraw  = NULL,
    .config        = pci_modem_config
};

const device_t wisecom_pm3_device = {
    .name          = "Wisecom WS-5614PM3 (HCF PCI)",
    .internal_name = "wisecom_pm3",
    .flags         = DEVICE_PCI,
    .local         = 4,
    .init          = pci_modem_init,
    .close         = pci_modem_close,
    .reset         = NULL,
    .available     = NULL,
    .speed_changed = modem_speed_changed,
    .force_redraw  = NULL,
    .config        = pci_modem_config
};

const device_t wisecom_pm3d_device = {
    .name          = "Wisecom WS-5614PM3D (HCF PCI)",
    .internal_name = "wisecom_pm3d",
    .flags         = DEVICE_PCI,
    .local         = 5,
    .init          = pci_modem_init,
    .close         = pci_modem_close,
    .reset         = NULL,
    .available     = NULL,
    .speed_changed = modem_speed_changed,
    .force_redraw  = NULL,
    .config        = pci_modem_config
};

const device_t conexant_hsf_device = {
    .name          = "Conexant HSF PCI Modem (Generic)",
    .internal_name = "conexant_hsf",
    .flags         = DEVICE_PCI,
    .local         = 6,
    .init          = pci_modem_init,
    .close         = pci_modem_close,
    .reset         = NULL,
    .available     = NULL,
    .speed_changed = modem_speed_changed,
    .force_redraw  = NULL,
    .config        = pci_modem_config
};
