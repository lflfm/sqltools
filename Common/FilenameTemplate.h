/*
    Copyright (C) 2017 Aleksey Kochetov

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
#include <Common/FixedArray.h>

namespace Common
{

    struct FilenameTemplateField
    { 
        const char* timeFormat; 
        const char* displayFormat;
        const char* menuLabel;
        FilenameTemplateField (const char* tf, const char* df, const char* ml)
            : timeFormat(tf), displayFormat(df), menuLabel(ml) {}
    };
    
    typedef FixedArray<FilenameTemplateField, 12> FilenameTemplateFields;

class FilenameTemplate
{
    static FilenameTemplate m_theOne;
    FilenameTemplateFields m_formatFields;
    FilenameTemplate();

public:

    static const FilenameTemplateFields& GetFormatFields () { return m_theOne.m_formatFields; }

    static void Format (const char* format, std::string& title, int counter); 
};

};//Common