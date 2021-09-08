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
    15.12.2002 bug fix, undo/redo of lower/upper don't save position/selection
    05.03.2003 bug fix, lower/upper/... operaration causes an exception on blank space
    2011.11.28 bug fix, insert and delete of selection do not handle tabified lines properly in the column mode
    2016.06.14 improvement, Alternative Columnar selection mode 
    2018.12.06 bug fix, when a line is having only spaces, 'Trim Trailing Space / Ctrl + K, R" is not trimming any space.
               same issue for "Trim Leading Spaces / Ctrl+K, O"
*/

#include "stdafx.h"
#include <algorithm>
#include <sstream>
#include <COMMON/ExceptionHelper.h>
#include "COMMON\StrHelpers.h"
#include "OpenEditor/OEContext.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define _CHECK_ALL_PTR_ _CHECK_AND_THROW_(m_pStorage, "Editor has not been initialized properly!")

namespace OpenEditor
{
    using namespace std;

void EditContext::StartScan (EBlockMode blkMode, const Square* pSqr)
{
    if (pSqr)
    {
        m_ScanBlockMode = blkMode;
        m_ScanSquare = *pSqr;
        m_ScanSquare.normalize();

        if (m_ScanBlockMode == ebtColumn)
            m_ScanSquare.normalize_columns();

        int lines = GetLineCount();
        if (!(m_ScanSquare.end.line < lines))
        {
            m_ScanSquare.end.line = lines - 1;
            
            if (m_ScanBlockMode != ebtColumn)
                m_ScanSquare.end.column = INT_MAX;
        }

        m_nScanCurrentLine = pSqr->start.line;
    }
    else
    {
        memset(&m_ScanSquare, 0, sizeof m_ScanSquare);
        m_ScanBlockMode = blkMode;
        m_nScanCurrentLine = 0;
    }

    calculateScanLine();
}

bool EditContext::Next ()
{
    m_nScanCurrentLine++;

    bool eof = Eof();

    if (!eof) 
        calculateScanLine();

    return !eof;
}

bool EditContext::Eof () const
{
    if (m_ScanSquare.is_empty())
        return m_nScanCurrentLine < GetLineCount() ? false : true;
    else
        return m_nScanCurrentLine < m_ScanSquare.end.line + 1 ? false : true;
}

void EditContext::calculateScanLine ()
{
    m_nScanStartInx = 0;
    m_nScanEndInx   = GetLineLength(m_nScanCurrentLine);

    switch (m_ScanBlockMode)
    {
    case ebtStream:
        if (m_nScanCurrentLine == m_ScanSquare.start.line)
        {
            m_nScanStartInx = pos2inx(m_nScanCurrentLine, m_ScanSquare.start.column);
        }
        if (m_nScanCurrentLine == m_ScanSquare.end.line)
        {
            m_nScanEndInx = pos2inx(m_nScanCurrentLine, m_ScanSquare.end.column);
        }
        break;
    case ebtColumn:
        m_nScanStartInx = pos2inx(m_nScanCurrentLine, m_ScanSquare.start.column);
        m_nScanEndInx   = pos2inx(m_nScanCurrentLine, m_ScanSquare.end.column);
        break;
    }
}

void EditContext::GetScanPosition (int& line, int& start, int& end) const
{
    line  = m_nScanCurrentLine;
    start = m_nScanStartInx;
    end   = m_nScanEndInx;
}

void EditContext::GetScanLine (std::wstring& str) const
{
    int line, start, end;
    GetScanPosition(line, start, end);

    OEStringW lineBuff;
    GetLineW(line, lineBuff);
    const wchar_t* ptr = lineBuff.data();
    int len = lineBuff.length();

    if (start < len)
    {
        ptr += start;
        len = max(0, min(len, end) - start);
    }
    else
    {
        ptr = 0;
        len = 0;
    }

    if (len > 0)
        str.assign(ptr, len);
    else
        str.clear();
}

void EditContext::PutLine (const std::wstring& str)
{
    _CHECK_ALL_PTR_

    int line, start, end;
    GetScanPosition(line, start, end);

    m_pStorage->ReplaceLinePart(line, start, end, str.c_str(), str.length());
}

void EditContext::StopScan ()
{
    memset(&m_ScanSquare, 0, sizeof m_ScanSquare);
    m_nScanCurrentLine = 0;
}


void EditContext::ScanAndReplaceText (bool (*pmfnDo)(const EditContext&, std::wstring&), bool curentWord)
{
    Square blkPos;

    // 05.03.2003 bug fix, lower/upper/... operaration causes an exception on blank space
    if (!curentWord || !WordFromPoint(GetPosition(), blkPos))
        GetSelection(blkPos);

    blkPos.normalize();

    if (!blkPos.is_empty())
    {
        // start undo transaction
        UndoGroup undoGroup(*this);
        PushInUndoStack(m_curPos);
        Square sel;
        GetSelection(sel);
        PushInUndoStack(GetBlockMode(), IsAltColumnarMode(), sel);

        Storage::NotificationDisabler disabler(m_pStorage);

        StartScan(GetBlockMode(), &blkPos);

        while (!Eof())
        {
            wstring orgStr;
            GetScanLine(orgStr);

            if (orgStr.length() > 0)
            {
                wstring newStr = orgStr;

                if (!(*pmfnDo)(*this, newStr))
                    break;

                if (newStr != orgStr)
                    PutLine(newStr);
            }

            Next();
        }

        disabler.Enable();

        StopScan();

        GetSelection(sel);
        PushInUndoStack(GetBlockMode(), IsAltColumnarMode(), sel);
        PushInUndoStack(m_curPos);
    }
}

bool EditContext::LowerText (const EditContext&, wstring& str)
{
    for (auto it = str.begin(); it != str.end(); ++it)
        *it = towlower(*it);

    return true;
}

bool EditContext::UpperText (const EditContext&, wstring& str)
{
    for (auto it = str.begin(); it != str.end(); ++it)
        *it = towupper(*it);

    return true;
}

bool EditContext::CapitalizeText  (const EditContext&, wstring& str)
{
    for (auto it = str.begin(); it != str.end(); ++it)
        if (it == str.begin() || !iswalpha(*(it-1)))
            *it = towupper(*it);
        else
            *it = towlower(*it);

    return true;
}

bool EditContext::InvertCaseText  (const EditContext&, wstring& str)
{
    for (auto it = str.begin(); it != str.end(); ++it)
        if (iswlower(*it))
            *it = towupper(*it);
        else if (iswupper(*it))
            *it = towlower(*it);

    return true;
}

bool EditContext::tabify (const wchar_t* str, int len, std::wstring& result, int startPos, int tabSpacing, bool leading)
{
    std::wstring buff;

    for (int i = 0; i < len && str[i]; )
    {
        if (str[i] == ' ')
        {
            // If all characters are space untill next tab position
            // then we replace them with a tab character
            int pos = ((i + startPos)/ tabSpacing + 1) * tabSpacing;

            for (int k = 0; i + k + startPos < pos && str[i + k] == ' '; k++)
                ;

            if (i + k + startPos == pos)
            {
                buff +=  '\t';
                i += k;
                continue;
            }
        }
        else if (leading) // exit when it's not space
        {
            buff.append(str + i, len - i);
            break;
        }

        buff += str[i++];
    }

    result = buff;

    return true;
}

bool EditContext::untabify (const wchar_t* str, int len, std::wstring& result, int startPos, int tabSpacing, bool leading)
{
    std::wstring buff;

    for (int i = 0, j = 0; i < len && str[i]; i++)
    {
        if (str[i] == '\t')
        {
            int pos = ((j + startPos) / tabSpacing + 1) * tabSpacing;

            while ((j + startPos) < pos)
            {
                buff += ' ';
                j++;
            }
        }
        else
        {
            if (leading)  // exit when it's not space
            {
                buff.append(str + i, len - i);
                break;
            }
            else
            {
                buff += str[i];
                j++;
            }
        }
    }

    result = buff;

    return true;
}

bool EditContext::TabifyText (const EditContext& edt, std::wstring& str)
{
    UntabifyText(edt, str);

    int tabSpacing = edt.GetTabSpacing();
    int startPos = edt.inx2pos(edt.m_nScanCurrentLine, edt.m_nScanStartInx);
    return tabify(str.c_str(), str.length(), str, startPos, tabSpacing, false);
}

bool EditContext::TabifyLeadingSpaces (const EditContext& edt, std::wstring& str)
{
    UntabifyLeadingSpaces(edt, str); 

    int tabSpacing = edt.GetTabSpacing();
    int startPos = edt.inx2pos(edt.m_nScanCurrentLine, edt.m_nScanStartInx);
    return tabify(str.c_str(), str.length(), str, startPos, tabSpacing, true);
}

bool EditContext::UntabifyText (const EditContext& edt, std::wstring& str)
{
    int tabSpacing = edt.GetTabSpacing();
    int startPos = edt.inx2pos(edt.m_nScanCurrentLine, edt.m_nScanStartInx);
    return untabify(str.c_str(), str.length(), str, startPos, tabSpacing, false);
}

bool EditContext::UntabifyLeadingSpaces (const EditContext& edt, std::wstring& str)
{
    int tabSpacing = edt.GetTabSpacing();
    int startPos = edt.inx2pos(edt.m_nScanCurrentLine, edt.m_nScanStartInx);
    return untabify(str.c_str(), str.length(), str, startPos, tabSpacing, true);
}

bool EditContext::ColumnLeftJustify (const EditContext& edt, std::wstring& str)
{
    UntabifyText(edt, str);

    wstring buff;

    wstring::size_type beg = str.find_first_not_of(L" \t");
    if (beg != wstring::npos)
    {
        buff = str.substr(beg);
        buff.resize(str.size(), ' ');
        str = buff;
    }

    return true;
}

bool EditContext::ColumnCenterJustify (const EditContext& edt, std::wstring& str)
{
    // Untabify done beforoe this call
    // UntabifyText(edt, str);

    wstring buff = str;
    Common::trim_symmetric(buff);
    // calculate block width because a line can be shorter than block
    wstring::size_type width = abs(edt.m_ScanSquare.end.column - edt.m_ScanSquare.start.column);
    wstring::size_type leftPadding = (width - buff.size())/2;
    buff.insert(0, leftPadding, ' ');
    buff.resize(width, ' ');
    str = buff;

    return true;
}

bool EditContext::ColumnRightJustify (const EditContext& edt, std::wstring& str)
{
    // Untabify done beforoe this call
    // UntabifyText(edt, str);

    wstring buff = str;

    wstring::size_type end = buff.find_last_not_of(L" \t");
    if (end != wstring::npos) 
    {
        buff.erase(end + 1);
        buff.insert(0, str.size() - buff.size(), ' ');
        str = buff;
    }

    // for shorter lines
    wstring::size_type width = abs(edt.m_ScanSquare.end.column - edt.m_ScanSquare.start.column);
    if (buff.size() < width)
    {
        buff.insert(0, width - buff.size(), ' ');
        str = buff;
    }

    return true;
}

bool EditContext::TrimLeadingSpaces (const EditContext&, wstring& str)
{
    wstring buff;

    wstring::size_type beg = str.find_first_not_of(L" \t");
    if (beg != wstring::npos)
    {
        buff = str.substr(beg);
        str = buff;
    }
    else
        str.clear();

    return true;
}

bool EditContext::TrimExcessiveSpaces (const EditContext& edt, std::wstring& str)
{
    TrimTrailingSpaces(edt, str);

    wstring buff;
    wstring::size_type beg = str.find_first_not_of(L" \t");

    if (beg != string::npos)
    {
        bool is_space, is_prev_space = false;

        // skip leading spaces
        auto it = str.begin();
        for (; iswspace(*it); ++it)
            buff += *it;

        for (; it != str.end(); ++it)
        {
            is_space = iswspace(*it);
        
            if (!is_space || !is_prev_space)
                buff += *it;

            is_prev_space = is_space;
        }
        str = buff;
    }
    else
        str.clear();

    return true;
}

bool EditContext::TrimTrailingSpaces (const EditContext&, std::wstring& str)
{
    wstring buff = str;

    wstring::size_type end = buff.find_last_not_of(L" \t");
    if (end != wstring::npos) 
    {
        buff.erase(end + 1);
        str = buff;
    }
    else
        str.clear();

    return true;
}

};
