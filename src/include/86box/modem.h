/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          This file is part of the 86Box distribution.
 *
 *          Hayes AT-compliant modem emulation definitions.
 *
 * Authors: Cacodemon345
 *          The DOSBox Team
 *          (Refactored for internal modems by Arena AI)
 */
#ifndef EMU_MODEM_H
#define EMU_MODEM_H

#include <86box/fifo8.h>
#include <86box/timer.h>
#include <86box/serial.h>
#include <86box/network.h>
#include <86box/plat_netsocket.h>

#define COMMAND_BUFFER_SIZE 512
#define NUMBER_BUFFER_SIZE  128
#define PHONEBOOK_SIZE      200

typedef enum ResTypes {
    ResNONE,
    ResOK,
    ResERROR,
    ResCONNECT,
    ResRING,
    ResBUSY,
    ResNODIALTONE,
    ResNOCARRIER,
    ResNOANSWER
} ResTypes;

typedef enum modem_mode_t {
    MODEM_MODE_COMMAND = 0,
    MODEM_MODE_DATA    = 1
} modem_mode_t;

typedef struct modem_phonebook_entry_t {
    char phone[NUMBER_BUFFER_SIZE];
    char address[NUMBER_BUFFER_SIZE];
} modem_phonebook_entry_t;

typedef struct modem_s {
    uint8_t   mac[6];
    serial_t *serial;
    uint32_t  baudrate;

    modem_mode_t mode;

    uint8_t    esc_character_expected;
    pc_timer_t host_to_serial_timer;
    pc_timer_t dtr_timer;
    pc_timer_t cmdpause_timer;

    uint8_t  tx_pkt_ser_line[0x10000]; /* SLIP-encoded. */
    uint32_t tx_count;

    Fifo8   rx_data; /* Data received from the network. */
    uint8_t reg[100];

    Fifo8 data_pending; /* Data yet to be sent to the host. */

    char     cmdbuf[COMMAND_BUFFER_SIZE];
    char     prevcmdbuf[COMMAND_BUFFER_SIZE];
    char     numberinprogress[NUMBER_BUFFER_SIZE];
    char     lastnumber[NUMBER_BUFFER_SIZE];
    uint32_t cmdpos;
    uint32_t port;
    int      plusinc, flowcontrol;
    int      in_warmup, dtrmode;
    int      dcdmode;

    bool     connected, ringing;
    bool     echo, numericresponse;
    bool     tcpIpMode, tcpIpConnInProgress;
    bool     cooldown;
    bool     telnet_mode;
    bool     dtrstate;
    uint32_t tcpIpConnCounter;

    int doresponse;
    int cmdpause;
    int listen_port;
    int ringtimer;

    SOCKET serversocket;
    SOCKET clientsocket;
    SOCKET waitingclientsocket;

    struct {
        bool    binary[2];
        bool    echo[2];
        bool    supressGA[2];
        bool    timingMark[2];
        bool    inIAC;
        bool    recCommand;
        uint8_t command;
    } telClient;

    modem_phonebook_entry_t entries[PHONEBOOK_SIZE];
    uint32_t                entries_num;

    netcard_t *card;
} modem_t;

extern void modem_reset(modem_t *modem);
extern void modem_write(serial_t *s, void *priv, uint8_t txval);
extern void modem_rcr_cb(struct serial_s *serial, void *priv);
extern void modem_dtr_callback(struct serial_s *serial, int status, void *priv);
extern void modem_speed_changed(void *priv);
extern int  modem_rx(void *priv, uint8_t *buf, int io_len);
extern void modem_init_common(modem_t *modem);
extern void modem_close_common(modem_t *modem);

#endif /*EMU_MODEM_H*/
