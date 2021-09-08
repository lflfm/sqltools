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

#include "stdafx.h"
#include "FilenameTemplate.h"
#include "StrHelpers.h"

namespace Common
{

    FilenameTemplate FilenameTemplate::m_theOne;

FilenameTemplate::FilenameTemplate()
{
    m_formatFields.push_back(FilenameTemplateField(0,    "&n"     , "&&n\tDocument counter"));
    m_formatFields.push_back(FilenameTemplateField("%d", "&dd"    , "&&dd\tDay of month as decimal number"));
    m_formatFields.push_back(FilenameTemplateField("%m", "&mm"    , "&&mm\tMonth as decimal number"));
    m_formatFields.push_back(FilenameTemplateField("%B", "&month" , "&&month\tFull month name"));
    m_formatFields.push_back(FilenameTemplateField("%b", "&mon"   , "&&mon\tAbbreviated month name"));
    m_formatFields.push_back(FilenameTemplateField("%Y", "&yyyy"  , "&&yyyy\tYear with century"));
    m_formatFields.push_back(FilenameTemplateField("%y", "&yy"    , "&&yy\tYear without century"));
    m_formatFields.push_back(FilenameTemplateField("%H", "&hh24"  , "&&hh24\tHour in 24-hour format"));
    m_formatFields.push_back(FilenameTemplateField("%I", "&hh12"  , "&&hh12\tHour in 12-hour format"));
    m_formatFields.push_back(FilenameTemplateField("%p", "&am"    , "&&am\tCurrent locale's A.M./P.M. indicator"));
    m_formatFields.push_back(FilenameTemplateField("%M", "&mi"    , "&&mi\tMinute as decimal number"));
    m_formatFields.push_back(FilenameTemplateField("%S", "&ss"    , "&&ss\tSecond as decimal number"));
}

void FilenameTemplate::Format (const char* format, std::string& title, int counter)
{
    Common::Substitutor subst;

    CTime tm = CTime::GetCurrentTime();

    FilenameTemplateFields::const_iterator it = m_theOne.m_formatFields.begin();
    for (; it != m_theOne.m_formatFields.end(); ++it)
        if (it->timeFormat)
        {
            CString buff = tm.Format(it->timeFormat);
            subst.AddPair(it->displayFormat, Common::str(buff));
        }

    CString buff;
    buff.Format(L"%d", counter + 1);
    subst.AddPair("&n", Common::str(buff));

    subst << format;

    title = subst.GetResult();
}

};//Common