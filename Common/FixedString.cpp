/* 
    Copyright (C) 2002 Aleksey Kochetov

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
#define FIXEDSTRING_MODULE
#include <COMMON/FixedString.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace Common
{
    static const FixedString::size_type BYTES_PER_PARA = 16;

    void FixedString::reallocate (size_type length1, size_type length2, bool exact)
    {
        _reallocate(max(length1, length2), exact);
    }

    void FixedString::_reallocate (size_type len, bool exact)
    {
        if (len < length())
        {
            ASSERT(len >= length());
            return;
        }

        if (!len) 
            cleanup();

        size_type cap = exact ? len : (len + BYTES_PER_PARA - 1) & ~(BYTES_PER_PARA - 1);

        if (cap + 1 > max_size()) _Xlen();

        char* buffer = new char[cap + 1];
        ref_counter(buffer + 1) = 1;
        memcpy(buffer + 1, _buffer, length());

        if (capacity() && !(--ref_counter(_buffer)))
        {
            delete[] (_buffer-1);
        }

        _buffer = buffer + 1;
        set_capacity(cap);
    }

	void FixedString::_Xlen() // report a length_error
	{
        THROW_STD_EXCEPTION("FixedString: data is too long.");	
    }

	void FixedString::_Xran(const char* where) // report an out_of_range error
	{
        _THROW(std::out_of_range, std::string(where) + ": invalid subscript.");
	}
}

