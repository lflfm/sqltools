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

#include <string>
#include <COMMON/NVector.h>

    using std::string;
    using std::vector;

#define DECLARE_STRTAB_PROPERTY(type, name) \
    protected: type m_##name; \
    public: type Get##name () const   { return m_##name; } \
    public: void Set##name (type val) { m_##name = val; }

#define DECLARE_STRTAB_PROPERTY_ADAPTER(type, name) \
    public: type Get##name (int row) const      { return m_table.at(row).Get##name(); } \
    public: void Set##name (int row, type val)  { m_table.at(row).Set##name(val); }

class StringRow
{
    Common::nvector<string> m_row;
public:
    DECLARE_STRTAB_PROPERTY(int, ImageIndex);

    StringRow (int ncol)                                        : m_row("StringRow::m_row", ncol), m_ImageIndex(-1) {}
    StringRow ()                                                : m_row("StringRow::m_row"), m_ImageIndex(-1) {}

    void Resize (int ncols)                                     { m_row.resize(ncols); }

    const string& GetString (int col) const                     { return m_row.at(col); }
    string& GetString (int col)                                 { return m_row.at(col); }
    void SetString (int col, const string& str)                 { m_row.at(col) = str; }
};

#define STRING_TABLE_CONTAINER ndeque

class StringTable
{
    typedef Common::STRING_TABLE_CONTAINER<StringRow> Container;
    Container m_table;
public:
    DECLARE_STRTAB_PROPERTY(int, ColumnCount);

    StringTable ()                                              : m_table("StringTable::m_table"), m_ColumnCount(0) {}
    StringTable (int ncols, int nrows = -1);

    void Clear ()                                               { m_table.clear(); }
    void Format (int ncols, int nrows = -1);

#if (STRING_TABLE_CONTAINER == ndeque)
    void ReserveMore (int)                                      {}
#else
    void ReserveMore (int nrows)                                { m_table.reserve(m_table.size() + nrows); }
#endif

    int  Append ();
    void Insert (int row)                                       { m_table.insert(m_table.begin() + row, StringRow(GetColumnCount())); }
    void Delete (int row)                                       { if (&m_table.at(row)) m_table.erase(m_table.begin() + row); }
    void DeleteAll ()                                           { m_table.clear(); }

    bool IsEmpty () const                                       { return !GetColumnCount() && !GetRowCount(); }
    int GetRowCount () const                                    { return m_table.size(); }

    const string& GetString (int row, int col) const            { return m_table.at(row).GetString(col); }
    string& GetString (int row, int col)                        { return m_table.at(row).GetString(col); }
    void SetString (int row, int col, const string& str)        { m_table.at(row).SetString(col, str); }

    DECLARE_STRTAB_PROPERTY_ADAPTER(int, ImageIndex);  // Get/SetImageIndex(row[, val])

    void GetMaxColumnWidthByFirstLine (Common::nvector<size_t>& result, int from, int to) const;
    void GetMaxRowWidthByFirstLine (Common::nvector<size_t>& result, int from, int to) const;

    int GetMaxLinesInColumn (int col, int lim) const;
    int GetMaxLinesInRow (int row, int lim) const;
};

inline
int StringTable::Append ()                                              
    { 
        m_table.resize(m_table.size() + 1);        
        m_table.rbegin()->Resize(GetColumnCount());  
        return m_table.size() - 1;
    }