/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2004 Aleksey Kochetov

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
*/

#pragma once

int raw_to_str (const unsigned char* data, int data_size, char* str, int str_size);
int str_to_raw (const char* str, int str_size, unsigned char* data, int data_size);


    extern char g_bin_to_hex[16];
    extern unsigned char g_hex_to_bin[256];

inline
const char* byte_to_str (unsigned char ch, char* buff)
{
    buff[2] = 0;
    buff[1] = g_bin_to_hex[0x0F & (unsigned char)ch];
    buff[0] = g_bin_to_hex[0x0F & ((unsigned char)ch) >> 4];

    return buff;
}


inline
unsigned char str_to_byte (const char* buff)
{
    unsigned val = 0;

    val |= g_hex_to_bin[buff[1]];
    val |= g_hex_to_bin[buff[0]] << 4;

    return (unsigned char) val;
}

