/*
 * This file is part of libpf9802.

 * libpf9802 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * libpf9802 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with libpf9802.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#include "pf9802.h"
#include "utils.h"
#include "port_endian.h"

#define PF_REQ  ((uint8_t)0x05)
#define PF_RESP ((uint8_t)0xfa)

typedef union {
    uint8_t buf[20];
    union {
        float f;
        uint32_t u;
    } data[5];
} result_t;

enum {
    S_IDLE,
    S_WRITE_REQ,
    S_READ_RESP,
    S_READ_DATA,
};

struct pf9802_s {
    int fd;
    int state;
    struct ev_io io;
    result_t data_buffer;
    int data_idx;
    pf9802_data_cb cb;
    void *udata;
};

static void __post_result(pf9802_result *r, result_t *result) {
    int i;
    for (i = 0; i < 5; ++i) {
        result->data[i].u = be32toh(result->data[i].u);
    }
    r->voltage = result->data[0].f;
    r->current = result->data[1].f;
    r->pf      = result->data[2].f;
    r->freq    = result->data[3].f;
    r->power   = result->data[4].f;
}

static int __open_serial_tty(const char *dev) {
    int fd;
    struct termios options;

    fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        perror("pf9802_open open failed - ");
        goto __out;
    }
    fcntl(fd, F_SETFL, 0);

    tcgetattr(fd, &options);
    cfsetispeed(&options, B2400);
    cfsetospeed(&options, B2400);

    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
#ifdef CNEW_RTSCTS
    options.c_cflag &= ~CNEW_RTSCTS;
#else
    options.c_cflag &= ~CRTSCTS;
#endif

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    options.c_iflag &= ~(IXON | IXOFF | IXANY);

    options.c_oflag &= ~OPOST;

    tcsetattr(fd, TCSANOW, &options);

__out:
    return fd;
}

pf9802_t *pf9802_open(const char *dev) {
    int fd;
    pf9802_t *p = NULL;

    fd = __open_serial_tty(dev);
    if (fd == -1) {
        goto __out;
    }

    p = (pf9802_t *)malloc(sizeof *p);
    if (!p) {
        perror("pf9802_open malloc failed - ");
        close(fd);
        goto __out;
    }
    memset(p, 0, sizeof *p);
    p->fd       = fd;
    p->state    = S_IDLE;
    p->data_idx = 0;

__out:
    return p;
}

void pf9802_close(pf9802_t *p) {
    close(p->fd);
    free(p);
}

int pf9802_get(pf9802_t *p, pf9802_result *r) {
    unsigned char sig;
    int ret;
    result_t result;
    int fd = p->fd;

    assert(p->state == S_IDLE);
    sig = PF_REQ;
    ret = writen(fd, &sig, sizeof sig);
    if (ret == -1) {
        perror("pf9802_get write failed - ");
        goto __err_out;
    }

    ret = readn(fd, &sig, sizeof sig);
    if (ret == -1 || ret == 0) {
        perror("pf9802_get read resp failed - ");
        goto __err_out;
    }

    if (sig != PF_RESP) {
        fprintf(stderr, "unexpected response: %#hhx\n", sig);
        goto __err_out;
    }

    {
        int sum = 0;
        int remain = sizeof result;
        uint8_t *buf = result.buf;

        while (remain && ((ret = readn(fd, buf + sum, remain)) > 0)) {
            sum += ret;
            remain -= ret;
        }
        if (ret == -1 || ret == 0) {
            perror("pf9802_get read results failed - ");
            goto __err_out;
        }
        if (sum != sizeof result) {
            fprintf(stderr, "unexpected data size: %d\n", sum);
            goto __err_out;
        }
    }
    __post_result(r, &result);

    return 1;
__err_out:
    return 0;
}

static void __ev_io_cb(EV_P_ struct ev_io *w, int revents);

void pf9802_async_init(pf9802_t *p, pf9802_data_cb cb, void *udata) {
    p->cb    = cb;
    p->udata = udata;
    p->state = S_IDLE;

    ev_io_init(&p->io, __ev_io_cb, p->fd, EV_WRITE);
    p->io.data = p;
}

void pf9802_async_start(EV_P_ pf9802_t *p) {
    tcflush(p->fd, TCIOFLUSH);
    p->state    = S_WRITE_REQ;
    p->data_idx = 0;
    ev_io_start(EV_A_ &p->io);
}

void pf9802_async_stop(EV_P_ pf9802_t *p) {
    ev_io_stop(EV_A_ &p->io);
    p->io.events = EV_WRITE;
    p->state = S_IDLE;
    tcflush(p->fd, TCIOFLUSH);
}

static void __ev_io_cb(EV_P_ struct ev_io *w, int revents) {
    pf9802_t *p = (pf9802_t *)w->data;
    int ret;
    int data_left;
    uint8_t byte;
    pf9802_result r;

    switch (p->state) {
    case S_WRITE_REQ:
        byte = PF_REQ;
        assert(revents & EV_WRITE);
        ret = writen(p->fd, &byte, sizeof byte);
        if (ret != 1) {
            p->cb(EV_A_ p, NULL, PF_ESYS, p->udata);
            goto __err_out;
        }
        ev_io_stop(EV_A_ &p->io);
        p->io.events = EV_READ;
        ev_io_start(EV_A_ &p->io);
        p->state = S_READ_RESP;
        break;

    case S_READ_RESP:
        assert(revents & EV_READ);
        ret = readn(p->fd, &byte, sizeof byte);
        if (ret <= 0) {
            p->cb(EV_A_ p, NULL, (ret ? PF_ESYS : PF_ETRUNC), p->udata);
            goto __err_out;
        }
        if (byte != PF_RESP) {
            p->cb(EV_A_ p, NULL, PF_ERESP, p->udata);
            goto __err_out;
        }
        p->state = S_READ_DATA;
        /* fall through */

    case S_READ_DATA:
        assert(revents & EV_READ);
        data_left = sizeof(result_t) - p->data_idx;
        ret = readn(p->fd, p->data_buffer.buf + p->data_idx, data_left);
        if (ret < 0) {
            p->cb(EV_A_ p, NULL, PF_ESYS, p->udata);
            goto __err_out;
        }
        if (ret == 0 && data_left) {
            p->cb(EV_A_ p, NULL, PF_ETRUNC, p->udata);
            goto __err_out;
        }
        data_left   -= ret;
        p->data_idx += ret;
        if (data_left > 0) { /* need more */
            break;
        }
        __post_result(&r, &p->data_buffer);
        p->cb(EV_A_ p, &r, PF_EOK, p->udata);
        p->data_idx = 0;

        if (p->state != S_IDLE) {
            ev_io_stop(EV_A_ &p->io);
            p->io.events = EV_WRITE;
            ev_io_start(EV_A_ &p->io);
            p->state = S_WRITE_REQ;
        }
        break;

    default:
        assert(0);
        break;
    }

__err_out:
    return;
}

