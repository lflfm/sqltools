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

/*
    13/05/2002 bug fix, undo operation completes with garbage on empty lines or following by them
*/

#include "stdafx.h"
#include "COMMON/StrHelpers.h"
#include "OpenEditor/OEHelpers.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace OpenEditor
{
    using namespace std;

    const char* DelimitersMap::m_cszDefDelimiters = " \t\'\\()[]{}+-*/.,!?;:=><%|@&^";

    const unsigned char LineTokenizer::cbSpaceChar     =  32;
    const unsigned char LineTokenizer::cbVirtSpaceChar = 183;
    const unsigned char LineTokenizer::cbTabChar       = 187;


/////////////////////////////////////////////////////////////////////////////

DelimitersMap::DelimitersMap ()
{
}

DelimitersMap::DelimitersMap (const char* delimiters)
{
    Set(delimiters);
}

void DelimitersMap::Set (const char* str)
{
    string delim;
    erase();

    for (; str && *str; str++)
        operator[](*str) = true;

    // they'r required
    operator[](' ') = true;
    operator[]('\t') = true;
    operator[]('\n') = true;
    operator[]('\r') = true;
}

void DelimitersMap::Get (string& buff)
{
    buff.erase();
    for (unsigned int i = 0; i < _size(); i++)
        if ((*this)[i])
            buff += static_cast<char>(i);
}

/////////////////////////////////////////////////////////////////////////////

LineTokenizer::LineTokenizer (bool showWhiteSpace, int tabSpacing, const DelimitersMap& delimiters)
:   m_delimiters(delimiters)
{
    m_processSpaces  =  false;
    m_showWhiteSpace = showWhiteSpace;

    if (m_showWhiteSpace)
    {
        m_spaceChar     = cbSpaceChar;
        m_virtSpaceChar = cbVirtSpaceChar;
        m_tabChar       = cbTabChar;
    }
    else
    {
        m_spaceChar     =
        m_virtSpaceChar =
        m_tabChar       = cbSpaceChar;
    }
   
    m_TabSpacing = tabSpacing;
    m_Buffer = 0;
    m_Length = m_Offset = m_Position = m_Position = 0;
}

void LineTokenizer::StartScan (const char* str, int len)
{
    m_Buffer = str;
    m_Length = len;
    m_Offset = m_Position = 0;

    if (!(m_showWhiteSpace || m_processSpaces))
    {
        // skip white space & expand tabs
        for (; m_Offset < m_Length && isspace(m_Buffer[m_Offset]); m_Offset++)
        {
            if (m_Buffer[m_Offset] == '\t')
                m_Position = (m_Position / m_TabSpacing + 1) * m_TabSpacing;
            else
                m_Position++;
        }
    }
}

bool LineTokenizer::Next ()
{
    bool isSpace = isspace(m_Buffer[m_Offset]) ? true : false;

    if (!isSpace)                       // it's world
    {
        if (m_Offset < m_Length && m_delimiters[m_Buffer[m_Offset]])
        {
            m_Offset++;
            m_Position++;
        }
        else
            for (; m_Offset < m_Length && !m_delimiters[m_Buffer[m_Offset]]; m_Offset++)
                m_Position++;
    }
    if (isSpace || !(m_showWhiteSpace || m_processSpaces))  // it's space
    {
        for (; m_Offset < m_Length && isspace(m_Buffer[m_Offset]); m_Offset++)
        {
            if (m_Buffer[m_Offset] == '\t')
                m_Position = (m_Position / m_TabSpacing + 1) * m_TabSpacing;
            else
                m_Position++;
        }
    }
    return !Eol();
}

void LineTokenizer::GetCurentWord (const char*& str, int& pos, int& len) const
{
    if (!isspace(m_Buffer[m_Offset]))   // it's world
    {
        if (m_delimiters[m_Buffer[m_Offset]])
            len = 1;
        else
            // skip word
            for (len = 0; m_Offset + len < m_Length
                          && !m_delimiters[m_Buffer[m_Offset + len]]; len++)
                ;

        str = m_Buffer + m_Offset;
        pos = m_Position;
    }
    else                                // it's space
    {
        m_Whitespaces.erase();

        for (int offset(m_Offset), position(m_Position); offset < m_Length && isspace(m_Buffer[offset]); offset++)
        {
            if (m_Buffer[offset] == '\t')
            {
                position = (position / m_TabSpacing + 1) * m_TabSpacing;
                m_Whitespaces += m_tabChar;
                m_Whitespaces.resize(position - m_Position, m_spaceChar);
            }
            else
            {
                position++;
                m_Whitespaces += m_virtSpaceChar;
            }
        }
        str = m_Whitespaces.c_str();
        len = m_Whitespaces.size();
        pos = m_Position;
    }
}

}
