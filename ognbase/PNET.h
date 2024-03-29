/*
 * PNET.h
 * Copyright (C) 2020 Manuel Rösel
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include "SoC.h"


#ifndef PNETHELPER_H
#define PNETHELPER_H

void aes_init();

void PNETencrypt(unsigned char msg[],size_t msgLen, char **arr, size_t *arr_len);
void PNETdecrypt(unsigned char msg[],size_t msgLen, char **arr, size_t *arr_len);


#endif /* PNETHELPER_H */
