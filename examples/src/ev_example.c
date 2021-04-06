#include <stdio.h>
#include <errno.h>

#include <ev.h>

#include "pf9802.h"

void print_result(pf9802_result *r) {
    printf("voltage : %.08f\n", r->voltage);
    printf("current : %.08f\n", r->current);
    printf("pf      : %.08f\n", r->pf);
    printf("freq    : %.08f\n", r->freq);
    printf("power   : %.08f\n", r->power);
    printf("\n");
    fflush(stdout);
}

void pf_cb(EV_P_ pf9802_t *p, pf9802_result *r, int error, void *udata) {
    if (error != PF_EOK) {
        printf("pf9802 get failed: %d\n", error);
        if (error == PF_ESYS) {
            printf("errno = %d\n", errno);
        }
        pf9802_async_stop(EV_A_ p);
        return;
    }
    print_result(r);
}

int main(int argc, char *argv[]) {
    pf9802_t *p;

    if (argc != 2) {
        printf("%s <dev>\n", argv[0]);
        return -1;
    }

    p = pf9802_open(argv[1]);
    if (!p) {
        return -1;
    }

    pf9802_async_init(p, pf_cb, NULL);
    pf9802_async_start(EV_DEFAULT_ p);

    ev_run(EV_DEFAULT_ 0);

    pf9802_close(p);

    return 0;
}

