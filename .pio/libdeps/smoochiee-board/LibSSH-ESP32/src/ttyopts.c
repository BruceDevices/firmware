/*
 * ttyopts.c - encoding of TTY modes.
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2023    by Utimaco TS GmbH <oss_committee@utimaco.com>
 * Author: Daniel Evers <daniel.evers@utimaco.com>
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "libssh_esp32_config.h"
#ifdef HAVE_TERMIOS_H
#undef HAVE_TERMIOS_H
#endif

#include <stdint.h>
#include <stdio.h>

#include <libssh/priv.h>
#include <string.h>

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

/** Terminal mode opcodes */
enum {
    TTY_OP_END = 0,
    TTY_OP_VINTR = 1,
    TTY_OP_VQUIT = 2,
    TTY_OP_VERASE = 3,
    TTY_OP_VKILL = 4,
    TTY_OP_VEOF = 5,
    TTY_OP_VEOL = 6,
    TTY_OP_VEOL2 = 7,
    TTY_OP_VSTART = 8,
    TTY_OP_VSTOP = 9,
    TTY_OP_VSUSP = 10,
    TTY_OP_VDSUSP = 11,
    TTY_OP_VREPRINT = 12,
    TTY_OP_VWERASE = 13,
    TTY_OP_VLNEXT = 14,
    TTY_OP_VFLUSH = 15,
    TTY_OP_VSWTC = 16,
    TTY_OP_VSTATUS = 17,
    TTY_OP_VDISCARD = 18,
    TTY_OP_IGNPAR = 30,
    TTY_OP_PARMRK = 31,
    TTY_OP_INPCK = 32,
    TTY_OP_ISTRIP = 33,
    TTY_OP_INLCR = 34,
    TTY_OP_IGNCR = 35,
    TTY_OP_ICRNL = 36,
    TTY_OP_IUCLC = 37,
    TTY_OP_IXON = 38,
    TTY_OP_IXANY = 39,
    TTY_OP_IXOFF = 40,
    TTY_OP_IMAXBEL = 41,
    TTY_OP_IUTF8 = 42,
    TTY_OP_ISIG = 50,
    TTY_OP_ICANON = 51,
    TTY_OP_XCASE = 52,
    TTY_OP_ECHO = 53,
    TTY_OP_ECHOE = 54,
    TTY_OP_ECHOK = 55,
    TTY_OP_ECHONL = 56,
    TTY_OP_NOFLSH = 57,
    TTY_OP_TOSTOP = 58,
    TTY_OP_IEXTEN = 59,
    TTY_OP_ECHOCTL = 60,
    TTY_OP_ECHOKE = 61,
    TTY_OP_PENDIN = 62,
    TTY_OP_OPOST = 70,
    TTY_OP_OLCUC = 71,
    TTY_OP_ONLCR = 72,
    TTY_OP_OCRNL = 73,
    TTY_OP_ONOCR = 74,
    TTY_OP_ONLRET = 75,
    TTY_OP_CS7 = 90,
    TTY_OP_CS8 = 91,
    TTY_OP_PARENB = 92,
    TTY_OP_PARODD = 93,
    TTY_OP_ISPEED = 128,
    TTY_OP_OSPEED = 129,
};

/**
 * Encodes a single SSH terminal mode option into the buffer.
 *
 * @param[in]  attr     The mode's opcode value.
 *
 * @param[in]  value    The mode's value.
 *
 * @param[out] buf      Destination buffer to encode into.
 *
 * @param[in]  buflen   The length of the buffer.
 *
 * @return              number of bytes written to the buffer on success, -1 on
 * error.
 */
static int
encode_termios_opt(unsigned char opcode,
                   uint32_t value,
                   unsigned char *buf,
                   size_t buflen)
{
    int offset = 0;

    /* always need 5 bytes */
    if (buflen < 5) {
        return -1;
    }

    /* 1 byte opcode */
    buf[offset++] = opcode;

    /* 4 bytes value (big endian) */
    value = htonl(value);
    memcpy(buf + offset, &value, sizeof(value));
    offset += sizeof(value);

    return offset;
}

#ifdef HAVE_TERMIOS_H
/** Converts a baudrate constant (Bxxxx) to a numeric value. */
static int
baud2speed(int baudrate)
{
    switch (baudrate) {
    default:
    case B0:
        return 0;
    case B50:
        return 50;
    case B75:
        return 75;
    case B110:
        return 110;
    case B134:
        return 134;
    case B150:
        return 150;
    case B200:
        return 200;
    case B300:
        return 300;
    case B600:
        return 600;
    case B1200:
        return 1200;
    case B1800:
        return 1800;
    case B2400:
        return 2400;
    case B4800:
        return 4800;
    case B9600:
        return 9600;
    case B19200:
        return 19200;
    case B38400:
        return 38400;
    case B57600:
        return 57600;
    case B115200:
        return 115200;
    case B230400:
        return 230400;
    }
}

/**
 * Encodes all terminal options from the given \c termios structure
 * into the buffer.
 *
 * @param[in]  attr     The terminal options to encode.
 *
 * @param[out] buf      Modes will be encoded into this buffer.
 *
 * @param[in]  buflen   The length of the buffer.
 *
 * @return              number of bytes in the buffer on success, -1 on error.
 */
static int
encode_termios_opts(struct termios *attr, unsigned char *buf, size_t buflen)
{
    unsigned int offset = 0;
    int rc;

#define SSH_ENCODE_OPT(code, value)                                      \
    rc = encode_termios_opt(code, value, buf + offset, buflen - offset); \
    if (rc < 0) {                                                        \
        return rc;                                                       \
    } else {                                                             \
        offset += rc;                                                    \
    }

#define SSH_ENCODE_INPUT_OPT(opt) \
    SSH_ENCODE_OPT(TTY_OP_##opt, (attr->c_iflag & opt) ? 1 : 0)
    SSH_ENCODE_INPUT_OPT(IGNPAR)
    SSH_ENCODE_INPUT_OPT(PARMRK)
    SSH_ENCODE_INPUT_OPT(INPCK)
    SSH_ENCODE_INPUT_OPT(ISTRIP)
    SSH_ENCODE_INPUT_OPT(INLCR)
    SSH_ENCODE_INPUT_OPT(IGNCR)
    SSH_ENCODE_INPUT_OPT(ICRNL)
#ifdef IUCLC
    SSH_ENCODE_INPUT_OPT(IUCLC)
#endif
    SSH_ENCODE_INPUT_OPT(IXON)
    SSH_ENCODE_INPUT_OPT(IXANY)
    SSH_ENCODE_INPUT_OPT(IXOFF)
    SSH_ENCODE_INPUT_OPT(IMAXBEL)
#ifdef IUTF8
    SSH_ENCODE_INPUT_OPT(IUTF8)
#endif
#undef SSH_ENCODE_INPUT_OPT

#define SSH_ENCODE_OUTPUT_OPT(opt) \
    SSH_ENCODE_OPT(TTY_OP_##opt, (attr->c_oflag & opt) ? 1 : 0)
    SSH_ENCODE_OUTPUT_OPT(OPOST)
#ifdef OLCUC
    SSH_ENCODE_OUTPUT_OPT(OLCUC)
#endif
    SSH_ENCODE_OUTPUT_OPT(ONLCR)
    SSH_ENCODE_OUTPUT_OPT(OCRNL)
    SSH_ENCODE_OUTPUT_OPT(ONOCR)
    SSH_ENCODE_OUTPUT_OPT(ONLRET)
#undef SSH_ENCODE_OUTPUT_OPT

#define SSH_ENCODE_CONTROL_OPT(opt) \
    SSH_ENCODE_OPT(TTY_OP_##opt, (attr->c_cflag & opt) ? 1 : 0)
    SSH_ENCODE_CONTROL_OPT(CS7)
    SSH_ENCODE_CONTROL_OPT(CS8)
    SSH_ENCODE_CONTROL_OPT(PARENB)
    SSH_ENCODE_CONTROL_OPT(PARODD)
#undef SSH_ENCODE_CONTROL_OPT

#define SSH_ENCODE_LOCAL_OPT(opt) \
    SSH_ENCODE_OPT(TTY_OP_##opt, (attr->c_lflag & opt) ? 1 : 0)
    SSH_ENCODE_LOCAL_OPT(ISIG)
    SSH_ENCODE_LOCAL_OPT(ICANON)
#ifdef XCASE
    SSH_ENCODE_LOCAL_OPT(XCASE)
#endif
    SSH_ENCODE_LOCAL_OPT(ECHO)
    SSH_ENCODE_LOCAL_OPT(ECHOE)
    SSH_ENCODE_LOCAL_OPT(ECHOK)
    SSH_ENCODE_LOCAL_OPT(ECHONL)
    SSH_ENCODE_LOCAL_OPT(NOFLSH)
    SSH_ENCODE_LOCAL_OPT(TOSTOP)
    SSH_ENCODE_LOCAL_OPT(IEXTEN)
    SSH_ENCODE_LOCAL_OPT(ECHOCTL)
    SSH_ENCODE_LOCAL_OPT(ECHOKE)
#ifdef PENDIN
    SSH_ENCODE_LOCAL_OPT(PENDIN)
#endif
#undef SSH_ENCODE_LOCAL_OPT

#define SSH_ENCODE_CC_OPT(opt) SSH_ENCODE_OPT(TTY_OP_##opt, attr->c_cc[opt])
    SSH_ENCODE_CC_OPT(VINTR)
    SSH_ENCODE_CC_OPT(VQUIT)
    SSH_ENCODE_CC_OPT(VERASE)
    SSH_ENCODE_CC_OPT(VKILL)
    SSH_ENCODE_CC_OPT(VEOF)
    SSH_ENCODE_CC_OPT(VEOL)
    SSH_ENCODE_CC_OPT(VEOL2)
    SSH_ENCODE_CC_OPT(VSTART)
    SSH_ENCODE_CC_OPT(VSTOP)
    SSH_ENCODE_CC_OPT(VSUSP)
#ifdef VDSUSP
    SSH_ENCODE_CC_OPT(VDSUSP)
#endif
    SSH_ENCODE_CC_OPT(VREPRINT)
    SSH_ENCODE_CC_OPT(VWERASE)
    SSH_ENCODE_CC_OPT(VLNEXT)
#ifdef VFLUSH
    SSH_ENCODE_CC_OPT(VFLUSH)
#endif
#ifdef VSWTC
    SSH_ENCODE_CC_OPT(VSWTC)
#endif
#ifdef VSTATUS
    SSH_ENCODE_CC_OPT(VSTATUS)
#endif
    SSH_ENCODE_CC_OPT(VDISCARD)
#undef SSH_ENCODE_CC_OPT

    SSH_ENCODE_OPT(TTY_OP_ISPEED, baud2speed(cfgetispeed(attr)))
    SSH_ENCODE_OPT(TTY_OP_OSPEED, baud2speed(cfgetospeed(attr)))
#undef SSH_ENCODE_OPT

    /* end of options */
    if (buflen > offset) {
        buf[offset++] = TTY_OP_END;
    } else {
        return -1;
    }

    return (int)offset;
}
#endif

/**
 * Encodes a set of default options to ensure "sane" PTY behavior.
 * This function intentionally doesn't use the \c termios structure
 * to allow it to work on Windows as well.
 *
 * The "sane" default set is derived from the `stty sane`, but iutf8 support is
 * added on top of that.
 *
 * @param[out] buf      Modes will be encoded into this buffer.
 *
 * @param[in]  buflen   The length of the buffer.
 *
 * @return              number of bytes in the buffer on success, -1 on error.
 */
static int
encode_default_opts(unsigned char *buf, size_t buflen)
{
    unsigned int offset = 0;
    int rc;

#define SSH_ENCODE_OPT(code, value)                                      \
    rc = encode_termios_opt(code, value, buf + offset, buflen - offset); \
    if (rc < 0) {                                                        \
        return rc;                                                       \
    } else {                                                             \
        offset += rc;                                                    \
    }

    SSH_ENCODE_OPT(TTY_OP_VINTR, 003)
    SSH_ENCODE_OPT(TTY_OP_VQUIT, 034)
    SSH_ENCODE_OPT(TTY_OP_VERASE, 0177)
    SSH_ENCODE_OPT(TTY_OP_VKILL, 025)
    SSH_ENCODE_OPT(TTY_OP_VEOF, 004)
    SSH_ENCODE_OPT(TTY_OP_VEOL, 0)
    SSH_ENCODE_OPT(TTY_OP_VEOL2, 0)
    SSH_ENCODE_OPT(TTY_OP_VSTART, 021)
    SSH_ENCODE_OPT(TTY_OP_VSTOP, 023)
    SSH_ENCODE_OPT(TTY_OP_VSUSP, 032)
    SSH_ENCODE_OPT(TTY_OP_VDSUSP, 031)
    SSH_ENCODE_OPT(TTY_OP_VREPRINT, 022)
    SSH_ENCODE_OPT(TTY_OP_VWERASE, 027)
    SSH_ENCODE_OPT(TTY_OP_VLNEXT, 026)
    SSH_ENCODE_OPT(TTY_OP_VDISCARD, 017)
    SSH_ENCODE_OPT(TTY_OP_IGNPAR, 0)
    SSH_ENCODE_OPT(TTY_OP_PARMRK, 0)
    SSH_ENCODE_OPT(TTY_OP_INPCK, 0)
    SSH_ENCODE_OPT(TTY_OP_ISTRIP, 0)
    SSH_ENCODE_OPT(TTY_OP_INLCR, 0)
    SSH_ENCODE_OPT(TTY_OP_IGNCR, 0)
    SSH_ENCODE_OPT(TTY_OP_ICRNL, 1)
    SSH_ENCODE_OPT(TTY_OP_IUCLC, 0)
    SSH_ENCODE_OPT(TTY_OP_IXON, 1)
    SSH_ENCODE_OPT(TTY_OP_IXANY, 0)
    SSH_ENCODE_OPT(TTY_OP_IXOFF, 0)
    SSH_ENCODE_OPT(TTY_OP_IMAXBEL, 0)
    SSH_ENCODE_OPT(TTY_OP_IUTF8, 1)
    SSH_ENCODE_OPT(TTY_OP_ISIG, 1)
    SSH_ENCODE_OPT(TTY_OP_ICANON, 1)
    SSH_ENCODE_OPT(TTY_OP_XCASE, 0)
    SSH_ENCODE_OPT(TTY_OP_ECHO, 1)
    SSH_ENCODE_OPT(TTY_OP_ECHOE, 1)
    SSH_ENCODE_OPT(TTY_OP_ECHOK, 1)
    SSH_ENCODE_OPT(TTY_OP_ECHONL, 0)
    SSH_ENCODE_OPT(TTY_OP_NOFLSH, 0)
    SSH_ENCODE_OPT(TTY_OP_TOSTOP, 0)
    SSH_ENCODE_OPT(TTY_OP_IEXTEN, 1)
    SSH_ENCODE_OPT(TTY_OP_ECHOCTL, 1)
    SSH_ENCODE_OPT(TTY_OP_ECHOKE, 1)
    SSH_ENCODE_OPT(TTY_OP_PENDIN, 0)
    SSH_ENCODE_OPT(TTY_OP_OPOST, 1)
    SSH_ENCODE_OPT(TTY_OP_OLCUC, 0)
    SSH_ENCODE_OPT(TTY_OP_ONLCR, 1)
    SSH_ENCODE_OPT(TTY_OP_OCRNL, 0)
    SSH_ENCODE_OPT(TTY_OP_ONOCR, 0)
    SSH_ENCODE_OPT(TTY_OP_ONLRET, 0)
    SSH_ENCODE_OPT(TTY_OP_CS7, 1)
    SSH_ENCODE_OPT(TTY_OP_CS8, 1)
    SSH_ENCODE_OPT(TTY_OP_PARENB, 0)
    SSH_ENCODE_OPT(TTY_OP_PARODD, 0)
    SSH_ENCODE_OPT(TTY_OP_ISPEED, 38400);
    SSH_ENCODE_OPT(TTY_OP_OSPEED, 38400);

#undef SSH_ENCODE_OPT

    /* end of options */
    if (buflen > offset) {
        buf[offset++] = TTY_OP_END;
    } else {
        return -1;
    }

    return (int)offset;
}

/**
 * @ingroup libssh_misc
 *
 * @brief Encode the current TTY options as SSH modes.
 *
 * Call this function to determine the settings of the process' TTY and
 * encode them as SSH Terminal Modes according to RFC 4254 section 8.
 *
 * If STDIN isn't connected to a TTY, this function fills the buffer with
 * "sane" default modes.
 *
 * The encoded modes can be passed to \c ssh_channel_request_pty_size_modes .
 *
 * @code
 *   unsigned char modes_buf[SSH_TTY_MODES_MAX_BUFSIZE];
 *   encode_current_tty_opts(modes_buf, sizeof(modes_buf));
 * @endcode
 *
 *
 * @param[out] buf      Modes will be encoded into this buffer.
 *
 * @param[in]  buflen   The length of the buffer.
 *
 * @return              number of bytes in the buffer on success, -1 on error.
 */
int
encode_current_tty_opts(unsigned char *buf, size_t buflen)
{
#ifdef HAVE_TERMIOS_H
    struct termios attr;
    ZERO_STRUCT(attr);

    if (isatty(STDIN_FILENO)) {
        /* get local terminal attributes */
        if (tcgetattr(STDIN_FILENO, &attr) < 0) {
            perror("tcgetattr");
            return -1;
        }
        return encode_termios_opts(&attr, buf, buflen);
    }
#endif

    /* use "sane" default attributes */
    return encode_default_opts(buf, buflen);
}
