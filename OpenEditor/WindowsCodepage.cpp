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

#include "stdafx.h"
#include "WindowsCodepage.h"
#include <iterator>


namespace OpenEditor
{
    WindowsCodepage::WindowsCodepage (int cp, const wchar_t* gr, const wchar_t* nm, const wchar_t* ln)
    : id(cp), group(gr), name(nm), longName(ln) 
    {
    }
    
    WindowsCodepage::WindowsCodepage ()
    : id(0)
    {
    }

    const std::vector<WindowsCodepage>& WindowsCodepage::GetAll ()
    {
        static std::vector<WindowsCodepage> _all  
        {
            WindowsCodepage(1250,    L"Windows", L"Win-1250",     L"Windows 1250 - Central European"        ),
            WindowsCodepage(1251,    L"Windows", L"Win-1251",     L"Windows 1251 - Cyrillic"                ),
            WindowsCodepage(1252,    L"Windows", L"Win-1252",     L"Windows 1252 - Western European"        ),
            WindowsCodepage(1253,    L"Windows", L"Win-1253",     L"Windows 1253 - Greek"                   ),
            WindowsCodepage(1254,    L"Windows", L"Win-1254",     L"Windows 1254 - Turkish"                 ),
            WindowsCodepage(1257,    L"Windows", L"Win-1257",     L"Windows 1257 - Baltic"                  ),
            WindowsCodepage(1258,    L"Windows", L"Win-1258",     L"Windows 1258 - Vietnam"                 ),
            WindowsCodepage(874,     L"Windows", L"Win-874",      L"Windows 874 - Thai"                     ),
            WindowsCodepage(932,     L"Windows", L"Win-932",      L"Windows 932 - Japanese Shift-JIS"       ),
            WindowsCodepage(949,     L"Windows", L"Win-949",      L"Windows 949 - Korean"                   ),
            WindowsCodepage(936,     L"Windows", L"Win-936",      L"Windows 936 - Simplified Chinese GBK"   ),
            WindowsCodepage(950,     L"Windows", L"Win-950",      L"Windows 950 - Traditional Chinese Big5" ),
            WindowsCodepage(28591,   L"ISO",     L"ISO-8859-1",   L"ISO-8859-1 - Latin-1 Western European"  ),
            WindowsCodepage(28592,   L"ISO",     L"ISO-8859-2",   L"ISO-8859-2 - Latin-2 Central European"  ),
            WindowsCodepage(28593,   L"ISO",     L"ISO-8859-3",   L"ISO-8859-3 - Latin-3 South European"    ),
            WindowsCodepage(28594,   L"ISO",     L"ISO-8859-4",   L"ISO-8859-4 - Latin-4 North European"    ),
            WindowsCodepage(28595,   L"ISO",     L"ISO-8859-5",   L"ISO-8859-5 - Cyrillic"                  ),
            WindowsCodepage(28597,   L"ISO",     L"ISO-8859-7",   L"ISO-8859-7 - Greek"                     ),
            WindowsCodepage(28599,   L"ISO",     L"ISO-8859-9",   L"ISO-8859-9 - Latin-5 Turkish"           ),
            WindowsCodepage(28603,   L"ISO",     L"ISO-8859-13",  L"ISO-8859-13 - Latin-7 Baltic Rim"       ),
            WindowsCodepage(28605,   L"ISO",     L"ISO-8859-15",  L"ISO-8859-15 - Latin-9"                  ),
            WindowsCodepage(51932,	 L"EUC",     L"EUC Japanese"           , L"EUC Japanese"                ),
            WindowsCodepage(51949,	 L"EUC",     L"EUC Korean"             , L"EUC Korean"                  ),
            WindowsCodepage(51936,	 L"EUC",     L"EUC Simplified Chinese" , L"EUC Simplified Chinese"      ),
            WindowsCodepage(51950,	 L"EUC",     L"EUC Traditional Chinese", L"EUC Traditional Chinese"     ),
            WindowsCodepage(437,     L"OEM",     L"IBM-437",      L"IBM-437 - OEM United States"            ),
            WindowsCodepage(737,     L"OEM",     L"IBM-737",      L"IBM-737 - OEM Greek"                    ),
            WindowsCodepage(775,     L"OEM",     L"IBM-775",      L"IBM-775 - OEM Baltic"                   ),
            WindowsCodepage(850,     L"OEM",     L"IBM-850",      L"IBM-850 - OEM Western European"         ),
            WindowsCodepage(852,     L"OEM",     L"IBM-852",      L"IBM-852 - OEM Central European"         ),
            WindowsCodepage(855,     L"OEM",     L"IBM-855",      L"IBM-855 - OEM Cyrillic"                 ),
            WindowsCodepage(857,     L"OEM",     L"IBM-857",      L"IBM-857 - OEM Turkish"                  ),
            WindowsCodepage(858,     L"OEM",     L"IBM-858",      L"IBM-858 - OEM OEM Western European + Euro symbol"),
            WindowsCodepage(860,     L"OEM",     L"IBM-860",      L"IBM-860 - OEM Portuguese"               ),
            WindowsCodepage(861,     L"OEM",     L"IBM-861",      L"IBM-861 - OEM Icelandic"                ),
            WindowsCodepage(863,     L"OEM",     L"IBM-863",      L"IBM-863 - OEM French Canadian"          ),
            WindowsCodepage(865,     L"OEM",     L"IBM-865",      L"IBM-865 - OEM Nordic"                   ),
            WindowsCodepage(866,     L"OEM",     L"CP-866" ,      L"CP-866 - OEM Russian"                   ),
            WindowsCodepage(869,     L"OEM",     L"IBM-869",      L"IBM-869 - OEM Modern Greek"             ),
            WindowsCodepage(20866,   L"Other",   L"KOI8-R",       L"KOI8-R - Russian"                       ),
            WindowsCodepage(21866,   L"Other",   L"KOI8-U",       L"KOI8-U - Ukrainian"                     ),
            WindowsCodepage(20127,	 L"Other",   L"US-ASCII" ,    L"US-ASCII (7-bit)"                      ),
        };


        static std::vector<WindowsCodepage> _supported;  

        if (_supported.empty())
        {
            CPINFO cpInfo;
            std::copy_if(_all.begin(), _all.end(), std::back_inserter(_supported), 
                        [&cpInfo] (const auto& cp) { return GetCPInfo(cp.id, &cpInfo) == TRUE; });
        }

        return _supported;
    }

    const WindowsCodepage& WindowsCodepage::GetCodepageIndex (int inx)
    {
        const std::vector<WindowsCodepage>& codepages = WindowsCodepage::GetAll();

        if (inx < (int)codepages.size())
            return codepages.at(inx);

        static WindowsCodepage empty;
        return empty;
    }

    const WindowsCodepage& WindowsCodepage::GetCodepageId (int id)
    {
        const std::vector<WindowsCodepage>& codepages = WindowsCodepage::GetAll();

        auto it = std::find_if(codepages.begin(), codepages.end(), [&id](const WindowsCodepage& cp) { return cp.id == id; });

        if (it != codepages.end())
            return *it;

        static WindowsCodepage empty;
        return empty;
    }

    int WindowsCodepage::GetIndexId (int id)
    {
        const std::vector<WindowsCodepage>& codepages = WindowsCodepage::GetAll();

        auto it = std::find_if(codepages.begin(), codepages.end(), [&id](const WindowsCodepage& cp) { return cp.id == id; });

        if (it != codepages.end())
            return it - codepages.begin();

        return -1;
    }

};
