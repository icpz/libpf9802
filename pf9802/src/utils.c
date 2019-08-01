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

#include <unistd.h>
#include <errno.h>

#define _WOULDBLOCK(err) (((err) == EWOULDBLOCK) || ((err) == EAGAIN))

ssize_t readn(int fd, void *vptr, size_t n) {
    size_t nleft;
    ssize_t nread;
    char *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR) {
                nread = 0;
            } else if (_WOULDBLOCK(errno)) {
                nread = 0;
                break;
            } else {
                goto __err_out;
            }
        } else if (nread == 0) {
            break;
        }
        nleft -= nread;
        ptr   += nread;
    }

    return (n - nleft);
__err_out:
    return -1;
}

ssize_t writen(int fd, const void *vptr, size_t n) {
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = vptr;
    nleft = n;

    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR) {
                nwritten = 0;
            } else {
                goto __err_out;
            }
        }
        nleft -= nwritten;
        ptr   += nwritten;
    }

    return n;
__err_out:
    return -1;
}

