/* 
    Copyright (C) 2018 Aleksey Kochetov

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

#include <string>
#include <vector>

namespace OpenEditor
{
    struct WindowsCodepage
    {
        int id;
        std::wstring group;
        std::wstring name;
        std::wstring longName;

        WindowsCodepage (int, const wchar_t*, const wchar_t*, const wchar_t*);
        WindowsCodepage ();

        static const std::vector<WindowsCodepage>& GetAll ();
        static const WindowsCodepage& GetCodepageIndex (int);
        static const WindowsCodepage& GetCodepageId (int);
        static int GetIndexId (int);
    };

};

