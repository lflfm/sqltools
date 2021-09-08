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
    17.04.2002 bug fix, strange vertical cursor movement on leading tabs and spaces
    26.07.2002 bug fix, program hangs on template expanding
    08.09.2002 bug fix, WordFromPoint has been updated
    09.01.2003 bug fix, unkown exception on word left on empty line
    29.06.2003 improvement, "Restrict cursor" has been replaced with "Cursor beyond EOL" and "Cursor beyond EOF"
    09.01.2003 bug fix, template cannot be expanded properly for a word at the end of line
*/

#include "stdafx.h"
#include <algorithm>
#include <COMMON/ExceptionHelper.h>
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

    class LineWatcher
    {
        EditContext& m_context;
        int m_line;
    public:
        LineWatcher (EditContext& context)
        : m_context(context)
        {
            m_line = m_context.GetPosition().line;
        }

        ~LineWatcher ()
        {
            if (m_context.IsCurrentLineHighlighted()
            && m_line != m_context.GetPosition().line)
            {
                m_context.InvalidateLines(m_line, m_line);
                m_context.InvalidateLines(m_context.GetPosition().line, m_context.GetPosition().line);
            }
        }
    };

bool EditContext::GoToUp (bool force)
{
    _CHECK_ALL_PTR_;

    if (!force && m_bDirtyPosition && adjustCursorPosition())
        return false;

    LineWatcher watcher(*this);

    if (m_curPos.line > 0)
    {
        m_curPos.line--;

        m_curPos.column = m_orgHrzPos;

        if (!force && !GetCursorBeyondEOL())
        {
            m_curPos.column
                = min(m_orgHrzPos, GetLineLength(m_curPos.line));
        }
        m_curPos.column = adjustPosByTab(m_curPos.line, m_curPos.column);

        return true;
    }
    return false;
}

bool EditContext::GoToDown (bool force)
{
    _CHECK_ALL_PTR_;

    if (!force && m_bDirtyPosition && adjustCursorPosition())
        return false;

    LineWatcher watcher(*this);

    int lines = GetLineCount();

    if (force || GetCursorBeyondEOF() || m_curPos.line < lines - 1)
    {
        m_curPos.line++;

        m_curPos.column = m_orgHrzPos;

        if (!force && !GetCursorBeyondEOL())
        {
            m_curPos.column
                = min(m_orgHrzPos, GetLineLength(m_curPos.line));
        }
        m_curPos.column = adjustPosByTab(m_curPos.line, m_curPos.column);

        return true;
    }

    return false;
}

bool EditContext::GoToLeft (bool force)
{
    _CHECK_ALL_PTR_;

    if (!force && m_bDirtyPosition && adjustCursorPosition())
        return false;

    if (m_curPos.column > 0)
    {
        m_curPos.column--;
        // 17/04/2002 bug fix, strange vertical cursor movement on leading tabs and spaces
        m_orgHrzPos = /*m_curPos.column;*/
        m_curPos.column = adjustPosByTab(m_curPos.line, m_curPos.column);

        return true;
    }
    else if (m_curPos.line > 0)
    {
        LineWatcher watcher(*this);

        bool retVal = false;
        if (GoToUp())
        {
            GoToEnd();
            retVal = true;
        }
        return retVal;
    }
    return false;
}

bool EditContext::GoToRight (bool force)
{
    _CHECK_ALL_PTR_;

    if (!force && m_bDirtyPosition && adjustCursorPosition())
        return false;

    int count = GetLineCount();
    int len = (m_curPos.line < count) ? GetLineLength(m_curPos.line) : 0;

    if (force || GetCursorBeyondEOL() || m_curPos.column < len)
    {
        m_curPos.column++;
        // 17/04/2002 bug fix, strange vertical cursor movement on leading tabs and spaces
        m_orgHrzPos = /*m_curPos.column;*/
        m_curPos.column = adjustPosByTab(m_curPos.line, m_curPos.column, true);
        return true;
    }
    else if (m_curPos.line < count - 1)
    {
        LineWatcher watcher(*this);

        bool retVal = false;
        if (GoToDown())
        {
            GoToStart();
            retVal = true;
        }
        return retVal;
    }
    return false;
}

bool EditContext::GoToTop ()
{
    _CHECK_ALL_PTR_;

    LineWatcher watcher(*this);

    if (m_curPos.line > 0 || m_curPos.column > 0)
    {
        m_curPos.line   =
        m_orgHrzPos     =
        m_curPos.column = 0;
        return true;
    }
    return false;
}

bool EditContext::GoToBottom ()
{
    _CHECK_ALL_PTR_;

    LineWatcher watcher(*this);

    int lines  = GetLineCount();

    if (lines > 0)
    {
        int column = GetLineLength(lines - 1);

        if (m_curPos.line != lines - 1 || m_curPos.column != column)
        {
            m_curPos.line   = lines - 1;
            m_orgHrzPos     =
            m_curPos.column = column;
            return true;
        }
    }
    else
    {
        if (m_curPos.line || m_curPos.column)
        {
            m_curPos.line   =
            m_orgHrzPos     =
            m_curPos.column = 0;
            return true;
        }
    }

    return false;
}

bool EditContext::GoToStart (bool force)
{
    if (!force && m_bDirtyPosition && adjustCursorPosition())
        return false;

    if (m_curPos.column > 0 || m_orgHrzPos > 0)
    {
        m_curPos.column = 0;
        m_orgHrzPos = m_curPos.column;
        return true;
    }
    return false;
}

bool EditContext::GoToEnd (bool force)
{
    _CHECK_ALL_PTR_;

    if (!force && m_bDirtyPosition && adjustCursorPosition())
        return false;

    if (m_curPos.line < GetLineCount())
    {
        int len = GetLineLength(m_curPos.line);

        if (m_curPos.column != len)
        {
            m_curPos.column = len;
            m_orgHrzPos = m_curPos.column;
            return true;
        }
    }
    else
        return GoToStart();

    return false;
}

bool EditContext::WordRight ()
{
    if (m_bDirtyPosition && adjustCursorPosition())
        return false;

    Position pos = wordRight(true);
    bool ret_val = (m_curPos != pos);
    //m_curPos = pos;
    MoveTo(pos);
    return ret_val;
}

bool EditContext::WordLeft ()
{
    if (m_bDirtyPosition && adjustCursorPosition())
        return false;

    Position pos = wordLeft(true);
    bool ret_val = (m_curPos != pos);
    //m_curPos = pos;
    MoveTo(pos);
    return ret_val;
}

bool EditContext::MoveTo (Position pos, bool force)
{
    _CHECK_ALL_PTR_;

    LineWatcher watcher(*this);

    pos.line   = max(0, pos.line);
    pos.column = max(0, pos.column);

    if (force || (GetCursorBeyondEOL() && GetCursorBeyondEOF()))
    {
        m_curPos.line   = pos.line;
        m_orgHrzPos     =
        m_curPos.column = pos.column;

        if (force)
            m_bDirtyPosition = force;
    }
    else
    {
        int lines = GetLineCount();

        if (lines > 0)
        {
            m_curPos.line   = GetCursorBeyondEOF() ? pos.line : min(pos.line, lines - 1);
            m_orgHrzPos     =
            m_curPos.column = GetCursorBeyondEOL() ? pos.column : min(pos.column, GetLineLength(pos.line));
        }
        else
        {
            m_curPos.line   =
            m_orgHrzPos     =
            m_curPos.column = 0;
        }
    }

    //if (!force)
        m_curPos.column = adjustPosByTab(m_curPos.line, m_curPos.column);

    AdjustCaretPosition();

    return true;
}

void EditContext::AdjustPosition (Position& pos, bool force) const
{
    _CHECK_ALL_PTR_;

    pos.line   = max(0, pos.line);
    pos.column = max(0, pos.column);

    if (!force
    && !(GetCursorBeyondEOL() && GetCursorBeyondEOF()))
    {
        int lines = GetLineCount();

        if (lines > 0)
        {
            pos.line   = GetCursorBeyondEOF() ? pos.line : min(pos.line, lines - 1);
            pos.column = GetCursorBeyondEOL() ? pos.column : min(pos.column, GetLineLength(pos.line));
        }
        else
        {
            pos.line   =
            pos.column = 0;
        }
    }

    if (!force)
        pos.column = adjustPosByTab(pos.line, pos.column);
}

bool EditContext::SmartGoToStart ()
{
    _CHECK_ALL_PTR_;

    // if before the end and 'Home key goes to the beggining of text' in preferences 
    if (m_curPos.line < GetLineCount() && GetSettings().GetUseSmartHome())
    {
        OEStringW lineBuff;
        GetLineW(m_curPos.line, lineBuff);
        const wchar_t* str = lineBuff.data(); 
        int len = lineBuff.length();

        for (int true_pos(0); true_pos < len && iswspace(str[true_pos]); true_pos++)
            ;
        true_pos = inx2pos(str, len, true_pos);
        m_orgHrzPos = (m_curPos.column == true_pos) ? 0 : true_pos;
        bool ret_val = (m_curPos.column != m_orgHrzPos) ? true : false;
        m_curPos.column = m_orgHrzPos;
        return ret_val;
    }
    else
        return GoToStart();
}

bool EditContext::SmartGoToEnd ()
{
    _CHECK_ALL_PTR_;

    // if before the end and 'Home key goes to the beggining of text' in preferences 
    if (m_curPos.line < GetLineCount() && GetSettings().GetUseSmartEnd())
    {
        OEStringW lineBuff;
        GetLineW(m_curPos.line, lineBuff);
        const wchar_t* str = lineBuff.data(); 
        int len = lineBuff.length();

        for (int true_pos(len); true_pos > 0 && iswspace(str[true_pos-1]); true_pos--)
            ;
        true_pos = inx2pos(str, len, true_pos);
        m_orgHrzPos = (m_curPos.column == true_pos) ? inx2pos(str, len, len) : true_pos;
        bool ret_val = (m_curPos.column != m_orgHrzPos) ? true : false;
        m_curPos.column = m_orgHrzPos;
        return ret_val;
    }
    else
        return GoToEnd();
}

    static
    bool _skip_spaces_right (const wchar_t* str, int len, int& start)
    {
        for (int i(start); i < len && iswspace(str[i]); i++);
        bool ret_val = (i != start) ? true : false;
        start = i;
        return ret_val;
    }

    static
    bool _skip_delimiters_right (const wchar_t* str, int len, int& start, const DelimitersMap& delim, bool hurry)
    {
        for (int i(start); i < len && !iswspace(str[i]) && delim[str[i]]; i++)
            if (!hurry && i != start) break;
        bool ret_val = (i != start) ? true : false;
        start = i;
        return ret_val;
    }

    static
    bool _skip_word_right (const wchar_t* str, int len, int& start, const DelimitersMap& delim)
    {
        for (int i(start); i < len && !delim[str[i]]; i++);
        bool ret_val = (i != start) ? true : false;
        start = i;
        return ret_val;
    }

Position
EditContext::wordRight (bool hurry)
{
    _CHECK_ALL_PTR_;

    bool skip_space_only = false;
    Position pos = m_curPos;

    if (pos.line >= GetLineCount()
    || pos.column >= GetLineLength(pos.line))
    {
        pos.column = 0;
        pos.line++;

        if (pos.line >= GetLineCount())
            return pos;

        skip_space_only = true;
    }

    OEStringW lineBuff;
    GetLineW(pos.line, lineBuff);
    const wchar_t* str = lineBuff.data();
    int len = lineBuff.length();

    int first = pos2inx(str, len, pos.column);

    // sD sW dW dsW
    int i = first;
    if (!_skip_spaces_right(str, len, i) && !skip_space_only)
        if (!_skip_delimiters_right(str, len, i, GetDelimiters(), hurry))
        {
            if (!_skip_spaces_right(str, len, i))
                if (_skip_word_right(str, len, i, GetDelimiters()))
                    _skip_spaces_right(str, len, i);
        }
        else
            _skip_spaces_right(str, len, i);

    pos.column = inx2pos(str, len, i);
    return pos;
}

    static
    bool _skip_spaces_left (const wchar_t* str, int& start)
    {
        for (int i(start); i >= 0 && str && iswspace(str[i]); i--);
        bool ret_val = (i != start) ? true : false;
        start = i;
        return ret_val;
    }

    static
    bool _skip_delimiters_left (const wchar_t* str, int& start, const DelimitersMap& delim, bool hurry)
    {
        for (int i(start); i >= 0 && str && !iswspace(str[i]) && delim[str[i]]; i--)
            if (!hurry && i != start) break;
        bool ret_val = (i != start) ? true : false;
        start = i;
        return ret_val;
    }

    static
    bool _skip_word_left (const wchar_t* str, int& start, const DelimitersMap& delim)
    {
        for (int i(start); i >= 0 && str && !delim[str[i]]; i--);
        bool ret_val = (i != start) ? true : false;
        start = str ? i : -1;
        return ret_val;
    }

Position
EditContext::wordLeft (bool hurry)
{
    _CHECK_ALL_PTR_;

    Position pos = m_curPos;

    if (pos.line >= GetLineCount()) // on virtual space
    {
        if (pos.column || --pos.line >= GetLineCount())
        {
            pos.column = 0;
            return pos;
        }
        else
            pos.column = GetLineLength(pos.line);
    }
    if (!pos.column)  // on line begin
    {
        if (pos.line == 0) return pos;
        pos.column = GetLineLength(--pos.line);
    }

    OEStringW lineBuff;
    GetLineW(pos.line, lineBuff);
    const wchar_t* str = lineBuff.data();
    int len = lineBuff.length();

    int last = max(0, pos2inx(str, len, pos.column)
                           - ((m_curPos.line == pos.line) ? 1 : 0));

    int i = last;
    _skip_spaces_left(str, i);
    if (!_skip_delimiters_left(str, i, GetDelimiters(), hurry))
        _skip_word_left(str, i, GetDelimiters());

    pos.column = inx2pos(str, len, i + 1);
    return pos;
}

bool EditContext::WordFromPoint (Position pos, Square& sqr, const char* delims) const
{
    _CHECK_ALL_PTR_;

    if (pos.line < GetLineCount()
    && pos.column <= GetLineLength(pos.line))
    {
        OEStringW lineBuff;
        GetLineW(pos.line, lineBuff);
        const wchar_t* str = lineBuff.data();
        int len = lineBuff.length();

        if (len > 0)
        {
            int inx = pos2inx(str, len, pos.column); // covert screen position to string index

            if (!(
                  // empty line
                  !len
                  // OR it is the first space
                  || !inx && iswspace(str[inx])
                  // OR it's EOL and last char is space
                  || inx == len && iswspace(str[inx-1])
                  // OR it's space in the line and previous char is space again
                  || inx != -1 && iswspace(str[inx]) && (inx > 0 && iswspace(str[inx-1]))
               ))
            {
                if (inx == len 
                || iswspace(str[inx])) --inx;

                DelimitersMap delimsMap;

                if (delims)
                    delimsMap.Set(delims);
                else
                    delimsMap = GetDelimiters();

                if (delimsMap[str[inx]])
                {
                    sqr.start.line = sqr.end.line = pos.line;
                    sqr.start.column = inx2pos(str, len, inx);
                    sqr.end.column   = inx2pos(str, len, inx + 1);
                }
                else
                {
                    sqr.start.line = sqr.end.line = pos.line;
                    sqr.start.column = inx;
                    _skip_word_left(str, sqr.start.column, delimsMap);
                    sqr.start.column = inx2pos(str, len, sqr.start.column + 1);
                    sqr.end.column   = inx;
                    _skip_word_right(str, len, sqr.end.column, delimsMap);
                    sqr.end.column = inx2pos(str, len, sqr.end.column);
                }
                return true;
            }
        }
    }
    sqr.clear();
    return false;
}

bool EditContext::WordOrSpaceFromPoint (Position pos, Square& sqr, const char* delims) const
{
    _CHECK_ALL_PTR_;

    sqr.clear();

    if (pos.line < GetLineCount())
    {
        OEStringW lineBuff;
        GetLineW(pos.line, lineBuff);
        const wchar_t* str = lineBuff.data();
        int len = lineBuff.length();

        if (len > 0)
        {
            DelimitersMap delimsMap;

            if (delims)
                delimsMap.Set(delims);
            else
                delimsMap = GetDelimiters();

            int inx = pos2inx(str, len, pos.column, true); // covert screen position to string index

            // ignore click belong the line except lines with trailing spaces
            if (inx >= len && iswspace(str[len - 1])) inx = len - 1;

            if (inx < len
            && delimsMap[str[inx]]
            && !(inx > 0 && iswspace(str[inx]) && !delimsMap[str[inx - 1]])) // if it's a space after word - try get a word later
            {
                delimsMap.Set(" \t");
                sqr.start.line = sqr.end.line = pos.line;

                if (iswspace(str[inx]))
                {

                    for (sqr.start.column = inx; sqr.start.column >=0 && delimsMap[str[sqr.start.column]]; sqr.start.column--)
                        ;

                    for (sqr.end.column = inx; sqr.end.column < len && delimsMap[str[sqr.end.column]]; sqr.end.column++)
                        ;
                }
                else
                {
                    for (sqr.start.column = inx; sqr.start.column >=0 && !delimsMap[str[sqr.start.column]]; sqr.start.column--)
                        ;

                    for (sqr.end.column = inx; sqr.end.column < len && !delimsMap[str[sqr.end.column]]; sqr.end.column++)
                        ;
                }

                sqr.start.column = inx2pos(str, len, sqr.start.column + 1);
                sqr.end.column   = inx2pos(str, len, sqr.end.column);

                return true;
            }
            else
                return WordFromPoint(pos, sqr, delims);
        }
    }

    return false;
}

bool EditContext::GetBlockOrWordUnderCursor (std::wstring& buff, Square& sqr, bool onlyOneLine, const char* delims)
{
    _CHECK_ALL_PTR_;

    sqr.clear(); // 26/07/2002 bug fix, program hangs on template expanding

    if (!IsSelectionEmpty())
    {
        GetSelection(sqr);
        sqr.normalize();

        if (onlyOneLine && sqr.end.line != sqr.start.line)
            return false;
    }
    else
    {
        WordFromPoint(GetPosition(), sqr, delims);
    }

    if (!sqr.is_empty())
    {
        GetBlock(buff, &sqr);
        return buff.size() ? true : false;
    }

    return false;
}

};
