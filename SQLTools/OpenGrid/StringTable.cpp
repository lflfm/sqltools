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

// 22.09.2007 bug fix, compatibility issue with MSVS 2007
// 2017-11-28 implemented range selection

#include "stdafx.h"
#include "StringTable.h"

using Common::nvector;

StringTable::StringTable (int ncols, int nrows) 
: m_table("StringTable::m_table"), 
m_ColumnCount(ncols)                                  
{ 
#if (STRING_TABLE_CONTAINER == ndeque)
    UNUSED_ALWAYS(nrows);
#else
    if (nrows != -1) 
        m_table.reserve(nrows); 
#endif
}

void StringTable::Format (int ncols, int nrows)                     
{ 
    m_ColumnCount = ncols; 
    m_table.clear(); 

#if (STRING_TABLE_CONTAINER == ndeque)
    UNUSED_ALWAYS(nrows);
#else
    if (nrows != -1) 
        m_table.reserve(nrows); 
#endif
}

void StringTable::GetMaxColumnWidthByFirstLine (nvector<size_t>& result, int from, int to) const
{
    result.clear();
    result.resize(m_ColumnCount, 0);
    Container::const_iterator rIt = m_table.begin();
    for (int row = 0; rIt != m_table.end(); ++rIt, ++row)
    {
        if (from <= row && row <= to)
        {
            for (int col = 0; col < m_ColumnCount; col++)
            {
                const string& val = rIt->GetString(col);
            
                size_t len = val.size();
                size_t first_line_len = val.find_first_of("\n\r");
            
                if (first_line_len != string::npos && first_line_len < len)
                  len = first_line_len;

                if (result[col] < len) 
				    result[col] = len;
            }
        }
    }
}

void StringTable::GetMaxRowWidthByFirstLine (nvector<size_t>& result, int from, int to) const
{
    result.clear();
    result.resize(m_table.size(), 0);
    for (int col = 0; col < m_ColumnCount; col++)
    {
        if (from <= col && col <= to)
        {
		    Container::const_iterator rIt = m_table.begin();
		    for (int row = 0; rIt != m_table.end(); ++rIt, ++row)
		    {
			    size_t len = rIt->GetString(col).size();
			    if (result[row] < len) 
				    result[row] = len;
            }
        }
    }
}

    // 22.09.2007 bug fix, compatibility issue with MSVS 2007
    int calc_lines (const string& str, int lim)
    {
        int nlines = 1;
        if (nlines < lim)
        {
            char lastchar = 0;
            string::const_iterator it = str.begin();

            for (; it != str.end(); ++it)
            {
                lastchar = *it;
                if (lastchar == '\n' && ++nlines == lim) 
                    break;
            }

            // ignore <CR> after last line
            if (nlines > 1 && it == str.end() 
            && (lastchar == '\n' || lastchar == '\r'))
                nlines--;
        }
        return nlines;
    }

int StringTable::GetMaxLinesInColumn (int col, int lim) const
{
    int nlines = 1;
    Container::const_iterator rIt = m_table.begin();
    for (; rIt != m_table.end(); ++rIt)
    {
		int nl = calc_lines(rIt->GetString(col), lim);
        if (nlines < nl)
            nlines = nl;
    }
    return nlines;
}

int StringTable::GetMaxLinesInRow (int row, int lim) const
{
    int nlines = 1;
    const StringRow& roW = m_table.at(row);
    for (int col = 0; col < m_ColumnCount; col++)
    {
	    int nl = calc_lines(roW.GetString(col), lim);
        if (nlines < nl)
            nlines = nl;
    }
    return nlines;
}
