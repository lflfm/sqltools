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
// 16.12.2004 (Ken Clubok) Add IsStringField method, to support CSV prefix option

#pragma once
#include <OpenGrid/GridSourceBase.h>

class HistorySource : public OG2::GridStringSource
{
public:
    HistorySource  ();
    ~HistorySource ();

    bool IsModified () const                    { return m_modified; }
    void SetModified (bool flag)                { m_modified = flag; }

    __int64 GetHistoryFileTime () const         { return m_fileTime; }
    void SetHistoryFileTime (__int64 fileTime)  { m_fileTime = fileTime; }

    int  GetRowCount () const                   { return GetCount(OG2::edVert); }

    const std::string& GetTime         (int line) const { return GetString(line, TIME_COLUMN); }
    const std::string& GetDuration     (int line) const { return GetString(line, DURATION_COLUMN); }
    const std::string& GetConnectDesc  (int line) const { return GetString(line, CONNECT_COLUMN); }
    const std::string& GetSqlStatement (int line) const { return GetString(line, STATEMENT_COLUMN); }

    bool IsTextField (int col) const; 

    void OnSettingsChanged ();

    // method for SQLWorksheetDoc
    void AddStatement (time_t startTime, const string& duration, const string& connectDesc, const string& sqlSttm);
    // method for HistoryFileManager
    void LoadStatement (const string& startTime, const string& duration, const string& connectDesc, const string& sqlSttm, bool atTop = false);

protected:
    static const int TIME_COLUMN = 0;
    static const int DURATION_COLUMN = 1;
    static const int CONNECT_COLUMN = 2;
    static const int STATEMENT_COLUMN = 3;

    bool m_modified;
    int  m_memoryAllocated;
    __int64 m_fileTime;
};
