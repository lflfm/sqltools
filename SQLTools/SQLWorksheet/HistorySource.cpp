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

//	16.12.2004 (Ken Clubok) Add IsStringField method, to support CSV prefix option

#include "stdafx.h"
#include "SQLTools.h"
#include "HistorySource.h"
#include "HistoryFileManager.h"
#include "COMMON/StrHelpers.h"

    using namespace std;
    using namespace OG2;
    using namespace Common;

HistorySource::HistorySource ()
    : m_modified(false),
    m_memoryAllocated(0),
    m_fileTime(0)
{
    SetShowFixCol(true);
    SetCols(4);

    SetColCharWidth(TIME_COLUMN, 15);
    SetColCharWidth(DURATION_COLUMN, 10);
    SetColCharWidth(CONNECT_COLUMN, 20);
    SetColCharWidth(STATEMENT_COLUMN, 32);

    SetColHeader(TIME_COLUMN, "Time");
    SetColHeader(DURATION_COLUMN, "Duration");
    SetColHeader(CONNECT_COLUMN, "Connection");
    SetColHeader(STATEMENT_COLUMN, "Statement");

    SetMaxShowLines(3);
}

HistorySource::~HistorySource ()
{
}

void HistorySource::OnSettingsChanged ()
{
    if (!GetSQLToolsSettings().GetHistoryEnabled()) {
        m_memoryAllocated = 0;
        DeleteAll();
    }
}

void HistorySource::AddStatement (time_t startTime, const string& duration, const string& connectDesc, const string& sqlSttm)
{
    string sttm = sqlSttm;
    std::string::size_type end = sttm.find_last_not_of(" \t\n\r");
    if (end != sttm.size() && end != std::string::npos)
        sttm.resize(end+1);

    const SQLToolsSettings& settings = GetSQLToolsSettings();

    if (!sttm.empty() && settings.GetHistoryEnabled())
    {
        m_modified = true;

        if (settings.GetHistoryDistinct()) 
        {
            int rows = GetRowCount();
            for (int row = 0; row < rows; row++) 
            {
                string str;
                GetCellStr(str, row, STATEMENT_COLUMN);
                if (str == sttm)
                {
                    m_memoryAllocated -= str.size();
                    Delete(row);
                    break;
                }
            }
        }

        int rowLim = settings.GetHistoryMaxCout();
        int memLim = 1024 * settings.GetHistoryMaxSize() - sttm.size();

        while (int rows = GetRowCount())
        {
            if (m_memoryAllocated < memLim && rows < rowLim) break;

            string text;
            GetCellStr(text, rows-1, STATEMENT_COLUMN);
            m_memoryAllocated -= text.size();
            Delete(rows-1);
        }

        char time_buff[80];
#pragma message("improvement: add option for choosing date/time format")
        //GetTimeFormat(0/*LOCALE_USER_DEFAULT*/, 0, 0, "HH':'mm':'ss", time, sizeof(time));

        tm* tp = localtime(&startTime);
        strftime(time_buff, sizeof(time_buff), "%Y.%m.%d %H:%M:%S", tp);
        time_buff[sizeof(time_buff)-1] = 0;

        NotificationDisabler disabler(this);
        int row = Insert(0);
        m_memoryAllocated += sttm.size();
        SetString(row, TIME_COLUMN, time_buff);
        SetString(row, DURATION_COLUMN, duration);
        SetString(row, CONNECT_COLUMN, connectDesc);
        SetString(row, STATEMENT_COLUMN, sttm);
        disabler.Enable();
        Notify_ChangedRowsQuantity(row, 1);
    }
}

void HistorySource::LoadStatement (const string& startTime, const string& duration, const string& connectDesc, const string& sqlSttm, bool atTop)
{
    int row = atTop ? Insert(0) : Append();
    SetString(row, TIME_COLUMN, startTime);
    SetString(row, DURATION_COLUMN, duration);
    SetString(row, CONNECT_COLUMN, connectDesc);
    SetString(row, STATEMENT_COLUMN, sqlSttm);
    m_memoryAllocated += sqlSttm.length();
    m_modified = true;
}

bool HistorySource::IsTextField (int col) const
{
	return (col != 0);
}