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

#ifndef __PF_UTILS_H__
#define __PF_UTILS_H__

#include <unistd.h>

ssize_t readn(int fd, void *buff, size_t n);
ssize_t writen(int fd, const void *vptr, size_t n);

#endif /* __PF_UTILS_H__ */
