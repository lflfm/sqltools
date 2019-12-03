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

#include "stdafx.h"
#include <assert.h>
#include "COMMON/ExceptionHelper.h"
#include "OCI8/Conversions.h"

char g_bin_to_hex[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };

unsigned char g_hex_to_bin[256];

static struct g_hex_to_bin_Initializer
{
    g_hex_to_bin_Initializer ()
    {
        memset(g_hex_to_bin, 0, sizeof g_hex_to_bin);

        g_hex_to_bin['0'] = 0;
        g_hex_to_bin['1'] = 1;
        g_hex_to_bin['2'] = 2;
        g_hex_to_bin['3'] = 3;
        g_hex_to_bin['4'] = 4;
        g_hex_to_bin['5'] = 5;
        g_hex_to_bin['6'] = 6;
        g_hex_to_bin['7'] = 7;
        g_hex_to_bin['8'] = 8;
        g_hex_to_bin['9'] = 9;
        g_hex_to_bin['A'] = 0xA;
        g_hex_to_bin['B'] = 0xB;
        g_hex_to_bin['C'] = 0xC;
        g_hex_to_bin['D'] = 0xD;
        g_hex_to_bin['E'] = 0xE;
        g_hex_to_bin['F'] = 0xF;
        g_hex_to_bin['a'] = 0xA;
        g_hex_to_bin['b'] = 0xB;
        g_hex_to_bin['c'] = 0xC;
        g_hex_to_bin['d'] = 0xD;
        g_hex_to_bin['e'] = 0xE;
        g_hex_to_bin['f'] = 0xF;
    }
}
g_hex_to_bin_initializer;


int raw_to_str (const unsigned char* data, int data_size, char* str, int str_size)
{
    _CHECK_AND_THROW_(2 * data_size <= str_size,  "raw_to_str: String buffer owerflow.");

    char* ptr = str;

    for (int i = 0; i < data_size; i++, ptr++, ptr++)
    {
        assert(ptr < str + str_size);
        byte_to_str(data[i], ptr);
    }

    return 2 * data_size;
}


int str_to_raw (const char* str, int str_size, unsigned char* data, int data_size)
{
    _CHECK_AND_THROW_(str_size <= 2 * data_size,  "raw_to_str: Raw buffer owerflow.");
    _CHECK_AND_THROW_(str_size % 2 == 0,  "raw_to_str: String length must be even.");

    const char* ptr = str;

    for (int i = 0; *ptr && i < str_size; i++, ptr++, ptr++)
        data[i] = str_to_byte(ptr);

    return i;
}

