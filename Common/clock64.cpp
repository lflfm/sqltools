/*
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2016 Aleksey Kochetov

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
#include "clock64.h"

clock64_t clock64 ()
{
    clock64_t current_tics;

    FILETIME ct;
    GetSystemTimeAsFileTime(&ct);

    /* calculate the elapsed number of 100 nanosecond units */
    current_tics = (clock64_t)ct.dwLowDateTime +
                    (((clock64_t)ct.dwHighDateTime) << 32);

    /* return number of elapsed milliseconds */
    return (current_tics / 10000);
}
