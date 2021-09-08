/*
    Copyright (C) 2002-2016 Aleksey Kochetov

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
    09.04.2002 bug fix, debug exception on undo & redo
    13.05.2002 bug fix, undo operation completes with garbage on empty lines or following by them
    17.09.2002 performance tuning, insert/delete block operation
    23.09.2002 improvement/performance tuning, undo has been reimplemented
    24.09.2002 bug fix, go to previos bookmark does not work properly
    06.12.2002 bug fix, in find dialog count and mark change "modified" status
    06.01.2002 performance tuning, file reading became 5-10 faster as result of lines' array preallocation.
               Actually it's a fix for performance degradation since 097 build 27.
    13.10.2003 small improvement, if pos == 0 then the second line inherits bookmarks
    20.11.2003 bug fix, handling unregular eol, e.c. single '\n' for MSDOS files
    01.06.2004 bug fix, ReplaceLinePart can replace 0-length line part now
    05.07.2004 bug fix, exception "FixedString is too long (>=64K)" on undo after save
    2011.09.22 bug fix, deleting selection extended beyond EOF causes EXCEPTION_ACCESS_VIOLATION
    2016.06.12 improvement, Highlighting for changed lines
    2016.06.28 improvement, added SetFileFormat and Undo for this operation
*/


#include "stdafx.h"
#include <COMMON/ExceptionHelper.h>
#include "OpenEditor/OESettings.h"
#include "OpenEditor/OEStorage.h"
#include "OpenEditor/OELanguageManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace OpenEditor
{
#define PUSH_IN_UNDO_STACK(undo) \
    if (m_bUndoEnabled && !m_bUndoRedoProcess) pushInUndoStack(undo);

#define THROW_X_IS_LOCKED THROW_APP_EXCEPTION("The content is locked and cannot be modified!");

    using namespace std;
    using namespace Common;

    const char Storage::m_cszDosLineDelim  [3] = "\r\n";
    const char Storage::m_cszMacLineDelim  [3] = "\n\r";
    const char Storage::m_cszUnixLineDelim [3] = "\n";

    Searcher Storage::m_defSearcher(true /*persistent*/);

    Storage::
    UndoGroup::UndoGroup (Storage& storage, const Position* pos)
    : m_Storage(storage)
    {
        if (!m_Storage.m_pUndoGroup)
        {
            m_Storage.m_pUndoGroup = this;

            if (pos)
                m_Storage.pushInUndoStack(new UndoCursorPosition(pos->line, pos->column));
        }
    }

    Storage::
    UndoGroup::~UndoGroup ()
    {
        try {
            if (m_Storage.m_pUndoGroup == this)
            {
                m_Storage.m_pUndoGroup = 0;
                m_Storage.m_UndoStack.NextUndoGroup();
            }
        }
        _DESTRUCTOR_HANDLER_;
    }

    Storage::NotificationDisabler::NotificationDisabler (Storage* owner, bool noNotificationUndo)
    : m_pOwner(owner),
    m_noNotificationUndo(noNotificationUndo)
    {
        _ASSERTE(!m_pOwner->m_bDisableNotifications); // recursion is not supported
        m_pOwner->m_bDisableNotifications = true;
        if (!m_noNotificationUndo)
            m_pOwner->pushInUndoStack(new UndoNotification(true));

    }

    void Storage::NotificationDisabler::Enable ()
    {
        m_pOwner->m_bDisableNotifications = false;
        m_pOwner->Notify_ChangedLines(0, m_pOwner->GetLineCount());
        if (!m_noNotificationUndo)
            m_pOwner->pushInUndoStack(new UndoNotification(false));
    }

    /// private helper class //////////////////////////////////////////////////
    class QuoteBalanceChecker
    {
        int state[3], quoteId[3];
        bool parsing[3];
    public:
        void ScanBeforeUpdate (const Storage* pStorage, int line, const OEStringW& lineBuff);
        void NotifyAboutChanged (Storage* pStorage, int line, const OEStringW& lineBuff);
    };

    void QuoteBalanceChecker::ScanBeforeUpdate (const Storage* pStorage, int line, const OEStringW& lineBuff)
    {
        if (!pStorage->m_bDisableNotifications)
        {
            state[0]   = 0;
            quoteId[0] = 0;
            parsing[0] = false;
            pStorage->m_Scanner.Scan(line, state[0], quoteId[0], parsing[0]);

            state[1]   = state[0];
            quoteId[1] = quoteId[0];
            parsing[1] = parsing[0];

            pStorage->m_Scanner.ScanLine(lineBuff.data(), lineBuff.length(), state[1], quoteId[1], parsing[1]);
        }
    }

    void QuoteBalanceChecker::NotifyAboutChanged (Storage* pStorage, int line, const OEStringW& lineBuff)
    {
        if (!pStorage->m_bDisableNotifications)
        {
            state[2]   = state[0];
            quoteId[2] = quoteId[0];
            parsing[2] = parsing[0];

            pStorage->m_Scanner.ScanLine(lineBuff.data(), lineBuff.length(), state[2], quoteId[2], parsing[2]);

            if (state[1] != state[2]
            || quoteId[1] != quoteId[2] // ?????
            || parsing[1] != parsing[2])
            {
                pStorage->Notify_ChangedLines(line, pStorage->m_Lines.size());
            }
            else
            {
                pStorage->Notify_ChangedLines(line, line);
            }
        }
    }
    /// private helper class //////////////////////////////////////////////////

#pragma warning ( disable : 4355 )
Storage::Storage ()
    : m_Scanner(*this),
    m_Lines(2 * 1024,  16 * 1024),
    m_locked(false),
    m_fileFotmat(effDefault),
    m_codepage(CP_UTF8)
{
    //int sz1 = sizeof(String);
    //int sz2 = sizeof(Common::FixedString);

    m_bDisableNotifications = false;
    setQuotesValidLimit(0);
    m_Lines.append();

    m_bUndoEnabled = true;
    m_bUndoRedoProcess = false;
    m_pUndoGroup = 0;

    m_pSettings = 0;
    m_pSearcher = 0;
    SetSearcher(&m_defSearcher);

    memset(m_BookmarkCountersByGroup, 0, sizeof m_BookmarkCountersByGroup);
    memset(m_szLineDelim, 0, sizeof m_szLineDelim);
}
#pragma warning ( default : 4355 )

Storage::~Storage ()
{
    try
    {
        if (m_pSettings)
            m_pSettings->RemoveSubscriber(this);

        if (m_pSearcher)
            m_pSearcher->RemoveRef(this);
    }
    _DESTRUCTOR_HANDLER_;
}

void Storage::Lock (bool locked)
{
    if (m_locked != locked)
    {
        m_locked = locked;
    
        vector<EditContextBase*>::iterator
            it(m_refContextList.begin()), end(m_refContextList.end());

        for (; it != end; ++it)
            (*it)->OnSettingsChanged();
    }
}

void Storage::Clear (bool destroy)
{
    if (!destroy && IsLocked())
        THROW_X_IS_LOCKED;

    memset(m_BookmarkCountersByGroup, 0, sizeof m_BookmarkCountersByGroup);
    partialClear();

    int orgLines = m_Lines.size();
    m_Lines.clear();
    Notify_ChangedLinesQuantity(0, -orgLines);
}

void Storage::partialClear ()
{
    m_actionSeq++;

    setQuotesValidLimit(0);

    m_UndoStack.Clear();
}

void Storage::LinkContext (EditContextBase* context)
{
    if (context)
    {
        vector<EditContextBase*>::iterator
            it(m_refContextList.begin()), end(m_refContextList.end());

        for (; it != end && *it != context; ++it);

        if (it == end)
           m_refContextList.push_back(context);
    }
}

void Storage::UnlinkContext (EditContextBase* context)
{
    if (context)
    {
        vector<EditContextBase*>::iterator
            it(m_refContextList.begin()), end(m_refContextList.end());

        for (; it != end && *it != context; ++it);

        if (*it == context)
           m_refContextList.erase(it, it + 1);
    }
}

void Storage::Notify_ChangedLines (int startLine, int endLine)
{
    if (!m_bDisableNotifications)
    {
        vector<EditContextBase*>::iterator
            it(m_refContextList.begin()), end(m_refContextList.end());

        for (; it != end; ++it)
            (*it)->OnChangedLines(startLine, endLine);

        if (startLine != endLine)
            setQuotesValidLimit(startLine);
    }
}

void Storage::Notify_ChangedLinesQuantity (int line, int quantity)
{
    if (!m_bDisableNotifications)
    {
        vector<EditContextBase*>::iterator
            it(m_refContextList.begin()), end(m_refContextList.end());

        for (; it != end; ++it)
            (*it)->OnChangedLinesQuantity(line, quantity);

        setQuotesValidLimit(line);
    }
}

void Storage::Notify_ChangedLinesStatus (int startLine, int endLine)
{
    if (!m_bDisableNotifications)
    {
        vector<EditContextBase*>::iterator
            it(m_refContextList.begin()), end(m_refContextList.end());

        for (; it != end; ++it)
            (*it)->OnChangedLinesStatus(startLine, endLine);
    }
}

void Storage::OnSettingsChanged ()
{
    m_languageSupport
        = LanguageManager::CreateLanguageSupport(GetSettings().GetLanguage(), this);

    m_UndoStack.SetCountLimit(GetSettings().GetUndoLimit());
    m_UndoStack.SetMemLimit(GetSettings().GetUndoMemLimit());

    m_delimitersMap.Set(GetSettings().GetDelimiters().c_str());

    vector<EditContextBase*>::iterator
        it(m_refContextList.begin()), end(m_refContextList.end());

    for (; it != end; ++it)
        (*it)->OnSettingsChanged();

    if (m_fileFotmat == effDefault)
        m_fileFotmat = (EFileFormat)GetSettings().GetFileCreateAs();
}

void Storage::SetText (
    const char* text, unsigned long length, // text pointer end length
    bool use_buffer,                        // use external persistent buffer
    bool refresh,                           // rescan for reallocated buffer
    bool external,                          // external changes, it requires additional synchronization
    bool on_error                           // called in case of unrecoverable error
)
{
    if (!on_error && !(refresh && !external) && IsLocked())
        THROW_X_IS_LOCKED;

    m_actionSeq++;

    size_t orgLines = m_Lines.size();
    size_t orgIncrement = m_Lines.increment();

    if (!refresh)
    {
        Clear(on_error);
        m_Lines.reserve(length / 80);
        m_Lines.increment() = length / 160;
    }

    // default
    memcpy(m_szLineDelim, m_cszDosLineDelim, sizeof(m_szLineDelim));

    {
        // check text type (Unix (\n), Dos (\r\n), Mac (\n\r))
        for (const char* ptr = text; ptr && *ptr != '\n' && *ptr != '\r' && ptr < text + length; ptr++)
            ;
        if (ptr < text + length)
        {
            if (ptr[0] == '\n')
            {
                if (ptr + 1 < text + length && ptr[1] == '\r')
                {
                    memcpy(m_szLineDelim, m_cszMacLineDelim, sizeof(m_szLineDelim));
                    m_fileFotmat = effMac;
                }
                else
                {
                    memcpy(m_szLineDelim, m_cszUnixLineDelim, sizeof(m_szLineDelim));
                    m_fileFotmat = effUnix;
                }
            }
            else if (ptr[0] == '\r')
            {
                if (ptr + 1 < text + length && ptr[1] == '\n')
                {
                    memcpy(m_szLineDelim, m_cszDosLineDelim, sizeof(m_szLineDelim));
                    m_fileFotmat = effDos;
                }
                else
                    ; // show warning and use default
            }
        }
    }

    StorageStringA buff;
    int pos = 0;
    const char* ptr = text;
    bool trunc = GetSettings().GetTruncateSpaces();

    unsigned line(0), size(m_Lines.size());
    while (ptr + pos < text + length)
    {
        if (ptr[pos] == m_szLineDelim[0]
        // 20.11.2003 bug fix, handling unregular eol, e.c. single '\n' for MSDOS files
        || ptr[pos] == m_szLineDelim[1])
        {
            if (use_buffer)
                buff.assign_static(ptr, pos);
            else
                buff.assign(ptr, pos);

            if (trunc) buff.truncate();

            if (!refresh || line >= size)
            {
                buff.tag.id = m_lineSeq.GetNextVal();
                m_Lines.push_back(buff);
                line++;
            }
            else
            {
                m_Lines[line++].assign(buff);
            }

            if (m_szLineDelim[1] && ((ptr + pos + 1) < text + length))
            {
                if (ptr[pos + 1] == m_szLineDelim[1])
                    pos++;
                else
                    ; // do nothing, ignore unregular eol
            }

            ptr += pos + 1;
            pos = 0;
        }
        else
            pos++;
    }

    if (pos) // ptr is not empty
    {
        if (use_buffer)
            buff.assign_static(ptr, pos);
        else
            buff.assign(ptr, pos);

        if (trunc) buff.truncate();

        if (!refresh || line >= size)
        {
            buff.tag.id = m_lineSeq.GetNextVal();
            m_Lines.push_back(buff);
            line++;
        }
        else
        {
            m_Lines[line++].assign(buff);
        }
    }
    // the previos line exists and has <CR>
    else if (m_Lines.size() > 1)
    {
        buff.cleanup();

        if (!refresh || line >= size)
        {
            buff.tag.id = m_lineSeq.GetNextVal();
            m_Lines.push_back(buff);
            line++;
        }
        else
        {
            m_Lines[line++].assign(buff);
        }
    }
    // it's a reload and a new file has less lines than previous one
    if (!refresh || line < size)
    {
        // skip if the new file is empty
        // and the original file had only one empty line
        if (!(refresh && !line && size == 1 && !length && !m_Lines[0].size()))
            m_Lines.reduce(line);
    }


    if (m_Lines.size() != orgLines)
        Notify_ChangedLinesQuantity(
                max(0, static_cast<int>(min(m_Lines.size(), orgLines)) - 1),
                m_Lines.size() - orgLines
            );

    // invalidate all views
    if (external)
        Notify_ChangedLines(0, m_Lines.size() - 1);

    // reset undo and scaner
    if (external || m_Lines.size() != orgLines)
        partialClear();

    m_Lines.increment() = orgIncrement;
}


const char* Storage::getLineDelim () const
{
    if (m_fileFotmat == effDefault 
    && m_szLineDelim[0]) 
        return m_szLineDelim;

    switch (m_fileFotmat)
    {
    case effMac:    return m_cszMacLineDelim;
    case effUnix:   return m_cszUnixLineDelim;
    case effDos:    return m_cszDosLineDelim;
    default:        _ASSERTE(0);
    }

    return m_cszDosLineDelim;
}

const wchar_t* Storage::GetFileFormatName () const
{
    const char* delim = getLineDelim();
    if (delim[0] == '\r')       return L" Dos ";
    else if (delim[1] == '\r')  return L" Mac ";
    else                        return L" Unix ";
}

const wchar_t* Storage::GetCodepageName () const
{
    switch (m_codepage)
    {
    case CP_ACP:  return L" ANSI ";
    case CP_UTF8: return L" UTF8 ";
    }
    return L"Unknown";
}

unsigned long Storage::GetTextLengthA () const
{
    // initialize a line delimiter
    int delim_length = getLineDelim()[1] ? 2 : 1;

    // calculate text size
    unsigned long usage = 0;
    bool the_first = true;

    for (int line(0), nlines = m_Lines.size(); line < nlines; line++)
    {
        usage += m_Lines[line].length() + (!the_first ? delim_length : 0);
        the_first = false;
    }

    return usage;
}

unsigned long Storage::GetTextLengthW () const
{
    // initialize a line delimiter
    int delim_length = getLineDelim()[1] ? 2 : 1;

    // calculate text size
    unsigned long usage = 0;
    bool the_first = true;

    for (int line(0), nlines = m_Lines.size(); line < nlines; line++)
    {
        usage += GetLineLengthW(line) + (!the_first ? delim_length : 0);
        the_first = false;
    }

    return usage;
}

unsigned long Storage::GetTextA (char* buffer, unsigned long size) const
{
    // initialize a line delimiter
    const char* ln_delim = getLineDelim();
    int ln_delim_len = ln_delim[1] ? 2 : 1;

    // copy text
    char* ptr = buffer;
    bool the_first = true;

    for (int line(0), nlines = m_Lines.size(); line < nlines; line++)
    {
        if (the_first)
            the_first = false;
        else
        {
            _CHECK_AND_THROW_((ptr + ln_delim_len) <= (buffer + size), "Text buffer size is not enough.");
            memcpy(ptr, ln_delim, ln_delim_len);
            ptr += ln_delim_len;
        }

        const OEStringA& str = m_Lines[line];
        _CHECK_AND_THROW_((ptr + str.length()) <= (buffer + size), "Text buffer size is not enough.");
        memcpy(ptr, str.data(), str.length());
        ptr += str.length();
    }

    return (ptr - buffer);
}

unsigned long Storage::GetTextW (wchar_t* buffer, unsigned long size) const
{
    // initialize a line delimiter
    std::wstring _ln_delim = Common::wstr(getLineDelim());
    const void* ln_delim = _ln_delim.c_str();
    int ln_delim_len = _ln_delim.size();

    // copy text
    wchar_t* ptr = buffer;
    bool the_first = true;

    for (int line(0), nlines = m_Lines.size(); line < nlines; line++)
    {
        if (the_first)
            the_first = false;
        else
        {
            _CHECK_AND_THROW_((ptr + ln_delim_len) <= (buffer + size), "Text buffer size is not enough.");
            memcpy(ptr, ln_delim, ln_delim_len * sizeof(wchar_t));
            ptr += ln_delim_len;
        }

        OEStringW str;
        GetLineW(line, str);
        _CHECK_AND_THROW_((ptr + str.length()) <= (buffer + size), "Text buffer size is not enough.");
        memcpy(ptr, str.data(), str.length() * sizeof(wchar_t));
        ptr += str.length();
    }

    return (ptr - buffer);
}

void Storage::TruncateSpaces (bool force)
{
    if (IsLocked())
        THROW_X_IS_LOCKED;

    if (force || GetSettings().GetTruncateSpaces())
    {
        m_actionSeq++;

        UndoGroup(*this);

        for (int line(0), nlines = m_Lines.size(); line < nlines; line++)
        {
            OEStringW  orgStr;
            GetLineW(line, orgStr);
            OEStringW  newStr = orgStr;
            newStr.truncate();

            if (newStr.size() != orgStr.size())
            {
                ELineStatus orgStatus = m_Lines[line].get_status();

                PUSH_IN_UNDO_STACK(new UndoDelete(line, newStr.size(), 
                    orgStr.data() + newStr.size(), orgStr.size() - newStr.size(), orgStatus));
                toMultibyte(newStr, m_Lines[line]);
                m_Lines[line].set_status(elsUpdated);
            }
        }
    }
}

int Storage::GetLineLengthW (int line) const
{
    if (int length = m_Lines.at(line).length())
        return ::MultiByteToWideChar(m_codepage, 0, m_Lines[line].data(), m_Lines[line].length(), 0, 0);
    return 0;
}

std::string Storage::GetChar (int line, int pos) const
{
    wchar_t ch = GetCharW(line, pos);
    return Common::str(&ch, 1);
}

wchar_t Storage::GetCharW (int line, int pos) const
{
    OEStringW str;
    GetLineW(line, str);
    return str.at(pos);
}

void Storage::GetLineA (int line, const char*& ptr, int& len) const
{
    ptr = m_Lines.at(line).data();
    len = m_Lines[line].length();
}

void Storage::GetLineW (int line, OEStringW& str) const
{
    int length = m_Lines.at(line).length();
    LPWSTR buff = str.reserve(length + 1);
    length = ::MultiByteToWideChar(m_codepage, 0, m_Lines[line].data(), length, buff, length);
    str.set_length((length > 0) ? length : 0);
}

void Storage::GetLineW (int line, OEStringW& str, int codepage) const
{
    int length = m_Lines.at(line).length();
    LPWSTR buff = str.reserve(length + 1);
    length = ::MultiByteToWideChar(codepage, 0, m_Lines[line].data(), length, buff, length);
    str.set_length((length > 0) ? length : 0);
}

void Storage::Insert (wchar_t ch, int line, int pos)
{
    if (IsLocked())
        THROW_X_IS_LOCKED;

    m_actionSeq++;

    if (ch != '\r')
    {
        InsertLinePart(line, pos, &ch, 1);
    }
    else
    {
        if (!m_Lines.size()) m_Lines.append();
        SplitLine(line, pos);
    }
}

void Storage::Overwrite (wchar_t ch, int line, int pos)
{
    if (IsLocked())
        THROW_X_IS_LOCKED;

    _ASSERTE(ch != '\r');

    if (ch != '\r')
    {
        m_actionSeq++;

        QuoteBalanceChecker chk;

        OEStringW lineBuff;
        GetLineW(line, lineBuff);
        chk.ScanBeforeUpdate(this, line, lineBuff);

        wchar_t orgStr = lineBuff.at(pos);

        lineBuff.replace(pos, ch);
        toMultibyte(lineBuff, m_Lines[line]);

        wchar_t newStr =  ch;

        ELineStatus orgStatus = m_Lines[line].get_status();
        m_Lines[line].set_status(elsUpdated);

        PUSH_IN_UNDO_STACK(new UndoOverwrite(line, pos, &orgStr, 1, &newStr, 1, orgStatus));

        chk.NotifyAboutChanged(this, line, lineBuff);
    }
}

void Storage::Delete (int line, int pos)
{
    if (IsLocked())
        THROW_X_IS_LOCKED;

    _ASSERTE(line < static_cast<int>(m_Lines.size()));

    if (line < static_cast<int>(m_Lines.size()))
    {
        m_actionSeq++;

        if (pos == static_cast<int>(m_Lines.at(line).length()))
            MergeLines(line);
        else
            DeleteLinePart(line, pos, pos + 1);
    }
}

// string mustn't have '\r' or '\n'
void Storage::InsertLine (int line, const wchar_t* str, int len, ELineStatus status[2])
{
    if (IsLocked())
        THROW_X_IS_LOCKED;

    m_actionSeq++;

    StorageStringA buff;
    buff.tag.id = m_lineSeq.GetNextVal();
    buff.set_status(status[0]);

    if (line < (int)m_Lines.size()) // 2018-12-15 bug fix
        m_Lines.at(line).set_status(status[1]);

    if (str) 
        toMultibyte(str, len, buff);

    m_Lines.insert(buff, line);
    PUSH_IN_UNDO_STACK(new UndoInsertLine(line, str, len, elsNothing));

    Notify_ChangedLinesQuantity(line, 1);
    Notify_ChangedLines(line, m_Lines.size());
}

void Storage::InsertLines (int line, const StringArrayW& lines, ELineStatus lineStatus, const vector<ELineStatus>* statuses)
{
    _ASSERTE(!statuses || statuses->size() == lines.size());

    if (IsLocked())
        THROW_X_IS_LOCKED;

    m_actionSeq++;

    ELineStatus orgStatus = elsNothing;
    if (line < static_cast<int>(m_Lines.size()))
    {
        orgStatus = m_Lines[line].get_status();
        m_Lines[line].set_status(lineStatus);
    }

    StringArrayA linesA(lines.size());
    toMultibyte(lines, linesA);
    m_Lines.insert(linesA, line);

    for (unsigned i = line; i < line + lines.size(); i++)
    {
        m_Lines[i].tag.id = m_lineSeq.GetNextVal();
        
        if (statuses && (i-line) < statuses->size())
            m_Lines[i].set_status((*statuses)[(i-line)]);
        else
            m_Lines[i].set_status(elsUpdated);
    }

    PUSH_IN_UNDO_STACK(new UndoInsertLines(line, lines, orgStatus));

    Notify_ChangedLinesQuantity(line, lines.size());
    Notify_ChangedLines(line, m_Lines.size());
}


// string mustn't have '\r' or '\n'
void Storage::InsertLinePart (int line, int pos, const wchar_t* str, int len, ELineStatus status)
{
    if (IsLocked())
        THROW_X_IS_LOCKED;

    m_actionSeq++;

    QuoteBalanceChecker chk;

    OEStringW lineBuff;
    GetLineW(line, lineBuff);
    chk.ScanBeforeUpdate(this, line, lineBuff);

    //m_Lines.at(line).insert(pos, str, len);
    lineBuff.insert(pos, str, len);
    toMultibyte(lineBuff, m_Lines.at(line));

    ELineStatus orgStatus = m_Lines[line].get_status();
    m_Lines[line].set_status(status);

    if (m_bUndoEnabled && !m_bUndoRedoProcess)
    {
        if (!m_UndoStack.AppendOnInsert(line, pos, str, len, m_delimitersMap))
            PUSH_IN_UNDO_STACK(new UndoInsert(line, pos, str, len, orgStatus));
    }

    chk.NotifyAboutChanged(this, line, lineBuff);
}

void Storage::ReplaceLinePart (int line, int from_pos, int to_pos, const wchar_t* str, int len, ELineStatus status)
{
    if (IsLocked())
        THROW_X_IS_LOCKED;

    _ASSERTE((to_pos - from_pos) >= 0);

    if (line < static_cast<int>(m_Lines.size())
    && (from_pos < static_cast<int>(m_Lines[line].length()) || (!from_pos && !m_Lines[line].length())))
    // 01.06.2004 bug fix, ReplaceLinePart can replace 0-length line part now
    //&& (to_pos - from_pos) >= 0)
    {
        _ASSERTE(to_pos >= from_pos || !to_pos && !from_pos);

        m_actionSeq++;

        QuoteBalanceChecker chk;

        OEStringW orgStr;
        GetLineW(line, orgStr);
        chk.ScanBeforeUpdate(this, line, orgStr);

        OEStringW newStr = orgStr;

        if (to_pos > static_cast<int>(orgStr.length()))
            to_pos = orgStr.length();

        newStr.erase(from_pos, to_pos - from_pos);
        newStr.insert(from_pos, str, len);

        ELineStatus orgStatus = m_Lines[line].get_status();
        m_Lines[line].set_status(status);
        toMultibyte(newStr, m_Lines[line]);

        PUSH_IN_UNDO_STACK(new UndoOverwrite(line, from_pos, orgStr.data() + from_pos, to_pos - from_pos, str, len, orgStatus));

        chk.NotifyAboutChanged(this, line, newStr);
    }
}

void Storage::DeleteLine (int line, ELineStatus nextLineStatus)
{
    if (IsLocked())
        THROW_X_IS_LOCKED;

    if (line < static_cast<int>(m_Lines.size()))
    {
        m_actionSeq++;

        const StorageStringA& str = m_Lines.at(line);
        if (str.tag.bmkmask)
            for (int i(0); i < BOOKMARK_GROUPS_SIZE; i++)
                if (str.tag.bmkmask & (1 << i))
                    m_BookmarkCountersByGroup[i]--;

        ELineStatus orgStatus[2];
        orgStatus[0] = str.get_status();

        if ((line+1 < static_cast<int>(m_Lines.size())))
        {
            orgStatus[1] = m_Lines[line+1].get_status();
            m_Lines[line+1].set_status(nextLineStatus);
        }
        else
            orgStatus[1] = elsNothing;
        
        OEStringW orgStr;
        GetLineW(line, orgStr);
        m_Lines.erase(line);
        
        PUSH_IN_UNDO_STACK(new UndoDeleteLine(line, orgStr.data(), orgStr.length(), orgStatus));

        Notify_ChangedLinesQuantity(max(0, line - 1), -1);
        Notify_ChangedLines(line, m_Lines.size());
    }
}

// 2011.09.22 bug fix, deleting selection extended beyond EOF causes EXCEPTION_ACCESS_VIOLATION
void Storage::DeleteLines (int line, int count, ELineStatus status)
{
    if (IsLocked())
        THROW_X_IS_LOCKED;

    if (line < static_cast<int>(m_Lines.size()))
    {
        m_actionSeq++;

        count = min<int>(count, m_Lines.size() - line);

        StringArrayW lines(count);
        for (int i = 0; i < count; i++)
        {
            const StorageStringA& str = m_Lines.at(line + i);
            
            StorageStringW& undoStr = lines.append();
            GetLineW(line + i, undoStr);
            undoStr.set_status(str.get_status());

            if (str.tag.bmkmask)
                for (int j(0); j < BOOKMARK_GROUPS_SIZE; j++)
                    if (str.tag.bmkmask & (1 << j))
                        m_BookmarkCountersByGroup[j]--;
        }

        ELineStatus orgStatus = elsNothing;
        if ((line + count) < static_cast<int>(m_Lines.size()))
        {
            orgStatus = m_Lines[line + count].get_status();
            m_Lines[line + count].set_status(status);
        }

        m_Lines.erase(line, count);

        PUSH_IN_UNDO_STACK(new UndoDeleteLines(line, lines, orgStatus));
        
        Notify_ChangedLinesQuantity(max(0, line - 1), -count);
        Notify_ChangedLines(line, m_Lines.size());
    }
}

void Storage::Reorder (int top, const vector<int>& order, const vector<ELineStatus>* statuses)
{
    if (IsLocked())
        THROW_X_IS_LOCKED;

    int nlines = static_cast<int>(m_Lines.size());

    ASSERT(top < nlines && top + static_cast<int>(order.size()) <= nlines);

    if (top < nlines && top + static_cast<int>(order.size()) <= nlines)
    {
        m_actionSeq++;

        StringArrayA lines(order.size()/*capacity*/);
        vector<ELineStatus> orgStatuses;

        vector<int>::const_iterator it = order.begin();
        for (int line = top; it != order.end(); ++it, line++)
        {
            lines.append(m_Lines.at(*it));
            orgStatuses.push_back(m_Lines[line].get_status());
        }

        for (unsigned int i = 0; i < lines.size(); i++)
        {
            m_Lines.at(i + top) = lines[i];
        }

        if (statuses)
            for (unsigned int i = 0; i < lines.size() && i < statuses->size(); i++)
                m_Lines[i + top].set_status((*statuses)[i]);
        else
            for (unsigned int i = 0; i < lines.size(); i++)
                m_Lines[i + top].set_status(elsUpdated);

        PUSH_IN_UNDO_STACK(new UndoReorder(top, order, orgStatuses));

        Notify_ChangedLines(top, top + order.size() - 1);
    }
}

void Storage::DeleteLinePart (int line, int from_pos, int to_pos, ELineStatus status)
{
    if (IsLocked())
        THROW_X_IS_LOCKED;

    _ASSERTE((to_pos - from_pos) >= 0);

    if (line < static_cast<int>(m_Lines.size())
    && from_pos < static_cast<int>(m_Lines[line].length())
    && (to_pos - from_pos) >= 0)
    {
        m_actionSeq++;

        QuoteBalanceChecker chk;

        OEStringW orgStr;
        GetLineW(line, orgStr);
        chk.ScanBeforeUpdate(this, line, orgStr);

        OEStringW newStr = orgStr;

        if (to_pos > static_cast<int>(orgStr.length()))
            to_pos = orgStr.length();

        ELineStatus orgStatus = m_Lines[line].get_status();
        newStr.erase(from_pos, to_pos - from_pos);
        toMultibyte(newStr, m_Lines[line]);
        m_Lines[line].set_status(status);

        PUSH_IN_UNDO_STACK(new UndoDelete(line, from_pos, orgStr.data() + from_pos, to_pos - from_pos, orgStatus));

        chk.NotifyAboutChanged(this, line, newStr);
    }
}

void Storage::SplitLine (int line, int pos, ELineStatus status1, ELineStatus status2)
{
    if (IsLocked())
        THROW_X_IS_LOCKED;

    _ASSERTE(line < static_cast<int>(m_Lines.size()));

    if (line < static_cast<int>(m_Lines.size()))
    {
        m_actionSeq++;

        OEStringW orgStr;
        GetLineW(line, orgStr);
        ELineStatus orgStatus = m_Lines[line].get_status();

        if (static_cast<int>(orgStr.length()) < pos)
            pos = orgStr.length();

        StorageStringA newStr;
        newStr.tag.id = m_lineSeq.GetNextVal();

        // 13.10.2003 small improvement, if pos == 0 then the second line inherits bookmarks
        if (!pos)
        {
            m_Lines[line].set_status(status2);
            newStr.set_status(status1);
            m_Lines.insert(newStr, line);
        }
        else
        {
            if (pos < static_cast<int>(orgStr.length()))
                toMultibyte(orgStr.data() + pos, orgStr.length() - pos, newStr);

            newStr.set_status(status2);
            m_Lines.insert(newStr, line + 1);

            if (pos < static_cast<int>(orgStr.length()))  
            {
                orgStr.erase(pos);
                toMultibyte(orgStr, m_Lines.at(line));
            }
            m_Lines[line].set_status(status1);
        }

        PUSH_IN_UNDO_STACK(new UndoSplitLine(line, pos, orgStatus));

        Notify_ChangedLinesQuantity(line, 1);
        Notify_ChangedLines(line, m_Lines.size());
    }
}

// ALTERNATIVE BOOKMARKS HANDLING:
// the result line has to have bookmark if at least one of lines has it
// the first line has higher priority if both lines have random bookmarks
// the result line should get the second line id if the first line empty and
// its id less then second line id - OR - the second line has ERROR MARK!
void Storage::MergeLines (int line, ELineStatus status)
{
    if (IsLocked())
        THROW_X_IS_LOCKED;

    if (line + 1 < static_cast<int>(m_Lines.size()))
    {
        m_actionSeq++;

        ELineStatus orgStatus[2] = { m_Lines[line].get_status(), m_Lines[line+1].get_status() };
        int size = m_Lines[line].length();
        m_Lines[line].append(m_Lines[line + 1]); // can be done in utf8
        m_Lines[line].set_status(status);
        Tag tag2 = m_Lines[line + 1].tag;
        m_Lines.erase(line + 1);
        PUSH_IN_UNDO_STACK(new UndoMergeLine(line, size, orgStatus));

        if (tag2.bmkmask)
        {
            if (!size && !m_Lines[line].tag.bmkmask)
                m_Lines[line].tag = tag2;
            else
                for (int i(0); i < BOOKMARK_GROUPS_SIZE; i++)
                    if (tag2.bmkmask & (1 << i))
                        m_BookmarkCountersByGroup[i]--;
        }

        Notify_ChangedLinesQuantity(line, -1);
        Notify_ChangedLines(line, m_Lines.size());
   }
}

void Storage::ExpandLinesTo (int line)
{
    if (IsLocked())
        THROW_X_IS_LOCKED;

    if (line >= static_cast<int>(m_Lines.size()))
    {
        m_actionSeq++;

        int nlines = GetLineCount();
        Notify_ChangedLines(nlines, nlines);
        m_Lines.expand(line + 1);

        StringArrayW lines(line + 1 - nlines);
        lines.expand(line + 1 - nlines);

        for (int i = 0; i + nlines < line + 1; ++i)
        {
            lines.at(i).tag.id = m_Lines.at(i + nlines).tag.id = m_lineSeq.GetNextVal();
            m_Lines.at(i + nlines).set_status(elsUpdated);
            lines.at(i).set_status(elsUpdated);
        }

        PUSH_IN_UNDO_STACK(new UndoInsertLines(nlines, lines, elsNothing));

        Notify_ChangedLines(nlines, GetLineCount());
    }
}

void Storage::SetSettings (const Settings* settings)
{
    _CHECK_AND_THROW_(settings, "Invalid initialization!");

    if (m_pSettings)
        m_pSettings->RemoveSubscriber(this);

    if (settings)
        settings->AddSubscriber(this);

    m_pSettings = settings;

    OnSettingsChanged();
}

int Storage::GetCodepage () const
{
    return m_codepage;
}

void Storage::SetCodepage (int codepage, bool undo)
{
    if (IsLocked())
        THROW_X_IS_LOCKED;

    if (m_codepage != codepage)
    {
        if (undo)
        {
            m_actionSeq++;

            PUSH_IN_UNDO_STACK(new UndoSetCodepage(m_codepage, codepage));
        }

        m_codepage = codepage;

        Notify_ChangedLines(0, m_Lines.size() - 1);
    }
}

    int get_actual_codepage (int codepage)
    {
        switch (codepage)
        {
        case CP_ACP:
            return GetACP();
        case CP_OEMCP:
            return GetOEMCP();
        }
        return codepage;
    }

void Storage::ConvertToCodepage (int codepage)
{
    if (IsLocked())
        THROW_X_IS_LOCKED;

    UndoGroup(*this);

    int orgCodepage = m_codepage;
    PUSH_IN_UNDO_STACK(new UndoSetCodepage(m_codepage, codepage));

    if (get_actual_codepage(orgCodepage) != get_actual_codepage(codepage))
    {
        m_actionSeq++;

        for (int line(0), nlines = m_Lines.size(); line < nlines; line++)
        {
            const OEStringA&  orgStr = m_Lines[line];

            OEStringW buffStr;
            GetLineW(line, buffStr, orgCodepage);  // using old codepage

            OEStringA newStr;
            toMultibyte(buffStr, newStr, codepage);          

            if (newStr.size() != orgStr.size() || memcmp(newStr.data(), orgStr.data(), orgStr.size()))
            {
                ELineStatus orgStatus = m_Lines[line].get_status();

                PUSH_IN_UNDO_STACK(new UndoConvertLine(line, buffStr, orgStatus));

                m_Lines[line].assign(newStr);
                m_Lines[line].set_status(elsUpdated);
            }
        }
    }

    SetCodepage(codepage, true);
}

void Storage::SetFileFormat (EFileFormat format, bool undo /*= true*/)
{
    if (m_fileFotmat != format)
    {
        if (undo)
        {
            PUSH_IN_UNDO_STACK(new UndoSetFileFormat(m_fileFotmat, format));
        }

        m_fileFotmat = format;
    }
}

void Storage::SetSearcher (Searcher* searcher)
{
    _CHECK_AND_THROW_(searcher, "Invalid initialization!");

    if (m_pSearcher)
        m_pSearcher->RemoveRef(this);

    if (searcher)
        searcher->AddRef(this);

    m_pSearcher = searcher;
}

void Storage::SetBookmark (int line, EBookmarkGroup group, bool on)
{
    _CHECK_AND_THROW_(group < BOOKMARK_GROUPS_SIZE, "Invalid value for bookmark group!");

    m_actionSeq++;

    unsigned int oldState = m_Lines.at(line).tag.bmkmask;

    if (on)
      m_Lines[line].tag.bmkmask |= 1 << group;
    else
      m_Lines[line].tag.bmkmask &= ~(1 << group);

    if (oldState != m_Lines[line].tag.bmkmask)
    {
        m_BookmarkCountersByGroup[group] += on ? 1 : -1;
        _ASSERTE(m_BookmarkCountersByGroup[group] >= 0);
        Notify_ChangedLines(line, line);
    }
}

void Storage::RemoveAllBookmark (EBookmarkGroup group)
{
    _CHECK_AND_THROW_(group < BOOKMARK_GROUPS_SIZE, "Invalid value for bookmark group!");

    m_actionSeq++;

    unsigned mask = 1 << group;

    NotificationDisabler disabler(this, true);

    for (int line(0), nlines(m_Lines.size()), first(-1), last(-1); line < nlines; line++)
        if (m_Lines[line].tag.bmkmask & mask)
        {
            m_Lines[line].tag.bmkmask &= ~mask;
            if (first == -1) first = line;
            last = line;
        }

   m_BookmarkCountersByGroup[group] = 0;

   disabler.Enable();
   Notify_ChangedLines(first, last); // actually we need invalidate only line state :(
}

int Storage::GetBookmarkNumber (EBookmarkGroup group)
{
    _CHECK_AND_THROW_(group < BOOKMARK_GROUPS_SIZE, "Invalid value for bookmark group!");

    return m_BookmarkCountersByGroup[group];
}

bool Storage::NextBookmark (int& _line, EBookmarkGroup group) const
{
    _CHECK_AND_THROW_(group < BOOKMARK_GROUPS_SIZE, "Invalid value for bookmark group!");

    unsigned mask = 1 << group;

    if (_line + 1 < static_cast<int>(m_Lines.size()))
    {
        for (int line = _line + 1, nlines = m_Lines.size(); line < nlines; line++)
            if (m_Lines[line].tag.bmkmask & mask)
            {
                _line = line;
                return true;
            }
    }

    for (int line = 0, nlines = _line; line < nlines; line++)
        if (m_Lines[line].tag.bmkmask & mask)
        {
            _line = line;
            return true;
        }

    return false;
}

bool Storage::PreviousBookmark (int& _line, EBookmarkGroup group) const
{
    _CHECK_AND_THROW_(group < BOOKMARK_GROUPS_SIZE, "Invalid value for bookmark group!");

    unsigned mask = 1 << group;

    int line = min((unsigned)_line, m_Lines.size())-1;

    for (; line > 0; line--)
        if (m_Lines[line].tag.bmkmask & mask)
        {
            _line = line;
            return true;
        }

    line = m_Lines.size() - 1;
    int end = _line;

    for (; line > end; line--)
        if (m_Lines[line].tag.bmkmask & mask)
        {
            _line = line;
            return true;
        }

    return false;
}

void Storage::GetBookmarkedLines (std::vector<int>& lines, EBookmarkGroup group) const
{
    _CHECK_AND_THROW_(group < BOOKMARK_GROUPS_SIZE, "Invalid value for bookmark group!");

    lines.empty();

    unsigned mask = 1 << group;

    for (int line = 0, nlines = m_Lines.size(); line < nlines; line++)
        if (m_Lines[line].tag.bmkmask & mask)
            lines.push_back(line);
}

void Storage::SetRandomBookmark (RandomBookmark bookmark, int line, bool on)
{
    _CHECK_AND_THROW_(bookmark.Valid(), "Invalid id for random bookmark!");
    SetBookmark(line, eRandomBmkGroup, on);
    m_Lines[line].tag.bookmark = bookmark.GetId();
}

bool Storage::GetRandomBookmark (RandomBookmark bookmark, int& _line) const
{
    _CHECK_AND_THROW_(bookmark.Valid(), "Invalid id for random bookmark!");

    // update cache if it's necessary
    GetRandomBookmarks();

    int line = m_RandomBookmarks.GetLine(bookmark);

    if (line != -1)
    {
        _line = line;
        return true;
    }

    return false;
}

// the vector will be valid untill actionNumber is the same
const RandomBookmarkArray& Storage::GetRandomBookmarks () const
{
    if (m_RandomBookmarks.GetActionId() != m_actionSeq)
    {
        m_RandomBookmarks.Reset();

        for (int line = 0, nlines = m_Lines.size(); line < nlines; line++)
            if (m_Lines[line].tag.bmkmask & 1 << eRandomBmkGroup)
                m_RandomBookmarks.SetLine(RandomBookmark(m_Lines[line].tag.bookmark), line);

        m_RandomBookmarks.SetActionNumber(m_actionSeq);
    }
    return m_RandomBookmarks;
}

bool Storage::FindById (LineId id, int& _line) const
{
    for (int line = 0, nlines = m_Lines.size(); line < nlines; line++)
        if (m_Lines[line].tag.id == id.GetId())
        {
            _line = line;
            return true;
        }

    return false;
}

void Storage::ResetLineStatusOnSave ()
{
    for (int line = 0, nlines = m_Lines.size(); line < nlines; line++)
        if (m_Lines[line].get_status() == elsUpdated)
            m_Lines[line].set_status(elsUpdatedSaved);
        else if (m_Lines[line].get_status() == elsRevertedBevoreSaved)
            m_Lines[line].set_status(elsNothing);

    Notify_ChangedLinesStatus(0, nlines);
}

// Memory usage reporting
void Storage::GetMemoryUsage (unsigned& usage, unsigned& allocated, unsigned& undo) const
{
    usage     = 0;
    allocated = 0;
    undo      = 0;

    // calculate text buffer memory usage
    {
        for (int line = 0, nlines = m_Lines.size(); line < nlines; line++)
        {
            usage     += m_Lines[line].length();
            allocated += (m_Lines[line].capacity() ? m_Lines[line].capacity() : m_Lines[line].length()) + sizeof(StorageStringA);
        }
    }
    // calculate undo memory usage
    undo = m_UndoStack.GetMemUsage();
}

// for background processing
bool Storage::OnIdle ()
{
    return m_languageSupport.get() ? m_languageSupport->OnIdle() : false;
}

void Storage::toMultibyte (const OEStringW& src, OEStringA& dst, int codepage)
{
    if (src.length() > 0)
    {
        int size = src.length() * 4;
        char* buffer = new char[size];

        int len = ::WideCharToMultiByte(codepage, 0, src.data(), src.length(), buffer, size, 0, 0);
        if (len > 0)
            dst.assign(buffer, len);
    
        delete[] buffer;
    }
    else
        dst.cleanup();
}

void Storage::toMultibyte (const OEStringW& src, OEStringA& dst)
{
    toMultibyte(src, dst, m_codepage);
}

void Storage::toMultibyte (const wchar_t* src, int length, OEStringA& dst)
{
    if (length > 0)
    {
        int size = length * 4;
        char* buffer = new char[size];

        int len = ::WideCharToMultiByte(m_codepage, 0, src, length, buffer, size, 0, 0);
        if (len > 0)
            dst.assign(buffer, len);
    
        delete[] buffer;
    }
    else
        dst.cleanup();
}

void Storage::toMultibyte (const StringArrayW& src, StringArrayA& dst)
{
    dst.clear();
    for (size_t i = 0; i < src.size(); ++i)
    {
        dst.append();
        dst.last().tag = src[i].tag;
        toMultibyte(src[i], dst.last());
    }
}

};
