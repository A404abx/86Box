/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          This file is part of the 86Box distribution.
 *
 *          Internal ISA Modem emulation.
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
#include <86box/modem.h>

typedef struct isa_modem_s {
    modem_t *modem;
    serial_t *serial;
    int port_index;
} isa_modem_t;

extern void modem_read_phonebook_file(modem_t *modem, const char *path);

static void *
isa_modem_init(const device_t *info)
{
    isa_modem_t *dev = (isa_modem_t *) calloc(1, sizeof(isa_modem_t));
    const char *phonebook_file = NULL;

    int port = device_get_config_int("port"); // 0-7 for COM1-COM8
    uint16_t base = device_get_config_hex16("base");
    uint8_t irq = device_get_config_int("irq");

    dev->port_index = port;

    /* Ensure the port is enabled in the serial core. */
    com_ports[port].enabled = 1;

    /* Add a UART instance. */
    dev->serial = device_add_inst(&ns16550_device, port + 1);
    serial_setup(dev->serial, base, irq);

    /* Initialize modem logic. */
    dev->modem = (modem_t *) calloc(1, sizeof(modem_t));
    memset(dev->modem->mac, 0xfc, 6);
    dev->modem->baudrate = device_get_config_int("baudrate");
    dev->modem->listen_port = device_get_config_int("listen_port");
    dev->modem->telnet_mode = device_get_config_int("telnet_mode");

    modem_init_common(dev->modem);

    /* Attach modem logic to our internal UART. */
    dev->modem->serial = serial_attach_ex_2(port, modem_rcr_cb, modem_write, modem_dtr_callback, dev->modem);

    modem_reset(dev->modem);
    dev->modem->card = network_attach(dev->modem, dev->modem->mac, modem_rx, NULL);

    phonebook_file = device_get_config_string("phonebook_file");
    if (phonebook_file && phonebook_file[0] != 0) {
        modem_read_phonebook_file(dev->modem, phonebook_file);
    }

    return dev;
}

static void
isa_modem_close(void *priv)
{
    isa_modem_t *dev = (isa_modem_t *) priv;
    modem_close_common(dev->modem);
    netcard_close(dev->modem->card);
    free(dev->modem);
    free(dev);
}

static const device_config_t isa_modem_config[] = {
    {
        .name           = "port",
        .description    = "COM Port Index",
        .type           = CONFIG_SELECTION,
        .default_string = NULL,
        .default_int    = 2, /* Default to COM3 */
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
        .name           = "base",
        .description    = "Base Address",
        .type           = CONFIG_HEX16,
        .default_string = NULL,
        .default_int    = 0x3e8,
        .file_filter    = NULL,
        .spinner        = { 0 },
        .selection      = {
            { .description = "0x3f8", .value = 0x3f8 },
            { .description = "0x2f8", .value = 0x2f8 },
            { .description = "0x3e8", .value = 0x3e8 },
            { .description = "0x2e8", .value = 0x2e8 },
            { .description = "0x3f0", .value = 0x3f0 },
            { .description = "0x2f0", .value = 0x2f0 },
            { .description = "0x3e0", .value = 0x3e0 },
            { .description = "0x2e0", .value = 0x2e0 },
            { .description = ""                      }
        },
        .bios           = { { 0 } }
    },
    {
        .name           = "irq",
        .description    = "IRQ",
        .type           = CONFIG_SELECTION,
        .default_string = NULL,
        .default_int    = 4,
        .file_filter    = NULL,
        .spinner        = { 0 },
        .selection      = {
            { .description = "IRQ 3", .value = 3 },
            { .description = "IRQ 4", .value = 4 },
            { .description = "IRQ 5", .value = 5 },
            { .description = "IRQ 7", .value = 7 },
            { .description = "IRQ 9", .value = 9 },
            { .description = "IRQ 10", .value = 10 },
            { .description = "IRQ 11", .value = 11 },
            { .description = "IRQ 12", .value = 12 },
            { .description = "IRQ 15", .value = 15 },
            { .description = ""                  }
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

const device_t isa_modem_device = {
    .name          = "Internal ISA Modem",
    .internal_name = "isa_modem",
    .flags         = DEVICE_ISA,
    .local         = 0,
    .init          = isa_modem_init,
    .close         = isa_modem_close,
    .reset         = NULL,
    .available     = NULL,
    .speed_changed = modem_speed_changed,
    .force_redraw  = NULL,
    .config        = isa_modem_config
};
