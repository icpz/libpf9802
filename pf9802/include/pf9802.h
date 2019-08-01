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

#ifndef __PF9802_H__
#define __PF9802_H__

#include <ev.h>

struct pf9802_s;
typedef struct pf9802_s pf9802_t;

typedef struct {
    float voltage;
    float current;
    float pf;
    float freq;
    float power;
} pf9802_result;

typedef void (*pf9802_data_cb)(EV_P_ pf9802_result *r, int e, void *udata);

pf9802_t *pf9802_open(const char *dev);
void pf9802_close(pf9802_t *p);
int pf9802_get(pf9802_t *p, pf9802_result *r);

enum {
    PF_EOK    = 0,
    PF_ERESP  = 1,
    PF_ETRUNC = 2,
    PF_ESYS   = 3,
};

void pf9802_async_init(pf9802_t *p, pf9802_data_cb cb, void *udata);
void pf9802_async_start(EV_P_ pf9802_t *p);
void pf9802_async_stop(EV_P_ pf9802_t *p);

#endif /* __PF9802_H__ */
