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

void EditContext::GetScanLine (int& line, int& start, int& end) const
{
    line  = m_nScanCurrentLine;
    start = m_nScanStartInx;
    end   = m_nScanEndInx;
}

void EditContext::GetScanLine (const char*& ptr, int& len) const
{
    int line, start, end;
    GetScanLine(line, start, end);

    GetLine(line, ptr, len);

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
}

void EditContext::PutLine (const char* ptr, int len)
{
    _CHECK_ALL_PTR_

    int line, start, end;
    GetScanLine(line, start, end);

    m_pStorage->ReplaceLinePart(line, start, end, ptr, len);
}

void EditContext::StopScan ()
{
    memset(&m_ScanSquare, 0, sizeof m_ScanSquare);
    m_nScanCurrentLine = 0;
}


void EditContext::ScanAndReplaceText (bool (*pmfnDo)(const EditContext&, string&), bool curentWord)
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
            string str;
            int len = 0;
            const char* ptr = 0;

            GetScanLine(ptr, len);

            if (len > 0)
            {
                str.assign(ptr, len);

                if (!(*pmfnDo)(*this, str))
                    break;

                if (static_cast<int>(str.length()) != len || strncmp(str.c_str(), ptr, len))
                    PutLine(str.c_str(), str.length());
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

bool EditContext::LowerText (const EditContext&, string& str)
{
    for (string::iterator it = str.begin(); it != str.end(); ++it)
        *it = tolower(*it);

    return true;
}

bool EditContext::UpperText (const EditContext&, string& str)
{
    for (string::iterator it = str.begin(); it != str.end(); ++it)
        *it = toupper(*it);

    return true;
}

bool EditContext::CapitalizeText  (const EditContext&, string& str)
{
    for (string::iterator it = str.begin(); it != str.end(); ++it)
        if (it == str.begin() || !isalpha(*(it-1)))
            *it = toupper(*it);
        else
            *it = tolower(*it);

    return true;
}

bool EditContext::InvertCaseText  (const EditContext&, string& str)
{
    for (string::iterator it = str.begin(); it != str.end(); ++it)
        if (islower(*it))
            *it = toupper(*it);
        else if (isupper(*it))
            *it = tolower(*it);

    return true;
}

bool EditContext::tabify (string& str, int startPos, int tabSpacing, bool leading)
{
    string buff;

    for (unsigned i(0); i < str.size() && str[i]; )
    {
        if (str[i] == ' ')
        {
            // If all characters are space untill next tab position
            // then we replace them with a tab character
            unsigned pos = ((i + startPos)/ tabSpacing + 1) * tabSpacing;

            for (unsigned k(0); i + k + startPos < pos && str[i + k] == ' '; k++)
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
            buff += str.c_str() + i;
            break;
        }

        buff += str[i++];
    }

    str = buff;

    return true;
}

bool EditContext::untabify (string& str, int startPos, int tabSpacing, bool leading)
{
    string buff;

    for (unsigned i(0), j(0); i < str.size() && str[i]; i++)
    {
        if (str[i] == '\t')
        {
            unsigned pos = ((j + startPos) / tabSpacing + 1) * tabSpacing;

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
                buff += str.c_str() + i;
                break;
            }
            else
            {
                buff += str[i];
                j++;
            }
        }
    }

    str = buff;

    return true;
}

bool EditContext::TabifyText (const EditContext& edt, string& str)
{
    UntabifyText(edt, str);

    int tabSpacing = edt.GetTabSpacing();
    int startPos = edt.inx2pos(edt.m_nScanCurrentLine, edt.m_nScanStartInx);
    return tabify(str, startPos, tabSpacing, false);
}

bool EditContext::TabifyLeadingSpaces (const EditContext& edt, string& str)
{
    UntabifyLeadingSpaces(edt, str); 

    int tabSpacing = edt.GetTabSpacing();
    int startPos = edt.inx2pos(edt.m_nScanCurrentLine, edt.m_nScanStartInx);
    return tabify(str, startPos, tabSpacing, true);
}

bool EditContext::UntabifyText (const EditContext& edt, string& str)
{
    int tabSpacing = edt.GetTabSpacing();
    int startPos = edt.inx2pos(edt.m_nScanCurrentLine, edt.m_nScanStartInx);
    return untabify(str, startPos, tabSpacing, false);
}

bool EditContext::UntabifyLeadingSpaces (const EditContext& edt, string& str)
{
    int tabSpacing = edt.GetTabSpacing();
    int startPos = edt.inx2pos(edt.m_nScanCurrentLine, edt.m_nScanStartInx);
    return untabify(str, startPos, tabSpacing, true);
}

bool EditContext::ColumnLeftJustify (const EditContext& edt, string& str)
{
    UntabifyText(edt, str);

    string buff;

    string::size_type beg = str.find_first_not_of(" \t");
    if (beg != string::npos)
    {
        buff = str.substr(beg);
        buff.resize(str.size(), ' ');
        str = buff;
    }

    return true;
}

bool EditContext::ColumnCenterJustify (const EditContext& edt, string& str)
{
    // Untabify done beforoe this call
    // UntabifyText(edt, str);

    string buff = str;
    Common::trim_symmetric(buff);
    // calculate block width because a line can be shorter than block
    string::size_type width = abs(edt.m_ScanSquare.end.column - edt.m_ScanSquare.start.column);
    string::size_type leftPadding = (width - buff.size())/2;
    buff.insert(0, leftPadding, ' ');
    buff.resize(width, ' ');
    str = buff;

    return true;
}

bool EditContext::ColumnRightJustify (const EditContext& edt, string& str)
{
    // Untabify done beforoe this call
    // UntabifyText(edt, str);

    string buff = str;

    string::size_type end = buff.find_last_not_of(" \t");
    if (end != string::npos) 
    {
        buff.erase(end + 1);
        buff.insert(0, str.size() - buff.size(), ' ');
        str = buff;
    }

    // for shorter lines
    string::size_type width = abs(edt.m_ScanSquare.end.column - edt.m_ScanSquare.start.column);
    if (buff.size() < width)
    {
        buff.insert(0, width - buff.size(), ' ');
        str = buff;
    }

    return true;
}

bool EditContext::TrimLeadingSpaces (const EditContext&, string& str)
{
    string buff;

    string::size_type beg = str.find_first_not_of(" \t");
    if (beg != string::npos)
    {
        buff = str.substr(beg);
        str = buff;
    }

    return true;
}

bool EditContext::TrimExcessiveSpaces (const EditContext& edt, string& str)
{
    //UntabifyText(edt, str);
    //TrimLeadingSpaces(edt, str);
    //TrimTrailingSpaces(edt, str);

    //string buff;
    //bool is_prev_space = false;

    //for (string::const_iterator it = str.begin(); it != str.end(); ++it)
    //{
    //    bool is_space = isspace(*it);

    //    if (!is_space || !is_prev_space)
    //        buff.push_back(*it);

    //    is_prev_space = is_space;
    //}

    //str = buff;

    //return true;
    
    TrimTrailingSpaces(edt, str);

    string buff;
    string::size_type beg = str.find_first_not_of(" \t");

    if (beg != string::npos)
    {
        bool is_space, is_prev_space = false;

        // skip leading spaces
        string::iterator it = str.begin();
        for (; isspace(*it); ++it)
            buff += *it;

        for (; it != str.end(); ++it)
        {
            is_space = isspace(*it);
        
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

bool EditContext::TrimTrailingSpaces (const EditContext&, string& str)
{
    string buff = str;

    string::size_type end = buff.find_last_not_of(" \t");
    if (end != string::npos) 
    {
        buff.erase(end + 1);
        str = buff;
    }

    return true;
}

};
