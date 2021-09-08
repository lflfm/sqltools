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
    17/05/2002 bug fix, cannot search for $,[,],^
    22/05/2002 bug fix, search fails it it started after EOF
    28/07/2002 bug fix, RegExp replace fails on \1...
    09.09.2002 improvement, find/replace batch performance
    16.12.2002 bug fix, Search all windows does not find occurance above the current
    09.01.2001 bug fix, changed for compatibility with boost regexp (it was required for VC6 only)
    16.03.2003 bug fix, infinite cycle is possible on "Count" or "Mark all" in search all windows mode
    24.05.2004 bug fix, search for empty string "^$" stops on the first occurence and does not follow forward/backward
    24.05.2004 bug fix, exception on attempt to replace "^" with any string (insert at start of line)
    01.06.2004 bug fix, invalid string length for search starting not at BOL
    01.06.2004 bug fix, while loop has been changed because of empty line/start line replacement
    27.06.2004 bug fix, undo for "invalid string length for search starting not at BOL"
    30.06.2004 improvement/bug fix, text search/replace interface has been changed
    23.09.2004 bug fix, replace all fails on 1 line selection
    27.10.2004 bug fix, Infinite cycle on Count/Mark/Replace All if "Search All Windows" is on
    17.01.2005 (ak) changed exception handling for suppressing unnecessary bug report
*/

#include "stdafx.h"
#include <string.h>
#include <algorithm>
#include "arg_shared.h"
#include <COMMON/ExceptionHelper.h>
#include "OpenEditor/OEHelpers.h"
#include "OpenEditor/OEStorage.h"

#define BOOST_REGEX_NO_LIB
#define BOOST_REGEX_STATIC_LINK
#include <boost/cregex.hpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace OpenEditor
{
    using namespace std;
    using namespace boost;
    using arg::counted_ptr;

    const int cnRegMatchSize = 100;

    // for isolation from "boost"
    struct Searcher::RegexpCxt
    {
        regex_t     pattern;
        regmatch_t  match[cnRegMatchSize];

        RegexpCxt ()  { ::memset(this, 0, sizeof(*this)); }
        ~RegexpCxt () { try { regfree(&pattern); } _DESTRUCTOR_HANDLER_; }
    };


Searcher::Searcher (bool persistent)
: m_cxt(new RegexpCxt)
{
    m_bPersistent = persistent;

    m_bBackward   = false;
    m_bWholeWords = false;
    m_bMatchCase  = false;
    m_bRegExpr    = false;
    m_bSearchAll  = false;

    m_bPatternCompiled = false;
}

Searcher::~Searcher ()
{
    try { EXCEPTION_FRAME;

        delete m_cxt;
    }
    _DESTRUCTOR_HANDLER_;
}

void Searcher::AddRef (Storage* context)
{
    if (context)
    {
        vector<Storage*>::iterator
            it(m_refList.begin()), end(m_refList.end());

        for (; it != end && *it != context; ++it);

        if (it == end)
           m_refList.push_back(context);
    }
}

void Searcher::RemoveRef (Storage* context)
{
    if (context)
    {
        vector<Storage*>::iterator
            it(m_refList.begin()), end(m_refList.end());

        for (; it != end && *it != context; ++it);

        if (*it == context)
           m_refList.erase(it, it + 1);
    }

    if (!m_bPersistent && !m_refList.size())
        delete this;
}

void Searcher::SetText (const wchar_t* str)
{
    m_strText = str;
    m_bPatternCompiled = false;
}

void Searcher::SetOption (bool backward, bool wholeWords, bool matchCase, bool regExpr, bool searchAll)
{
    m_bBackward   = backward;
    m_bWholeWords = wholeWords;
    m_bMatchCase  = matchCase;
    m_bRegExpr    = regExpr;
    m_bSearchAll  = searchAll;
    m_bPatternCompiled = false;
}

void Searcher::GetOption (bool& backward, bool& wholeWords, bool& matchCase, bool& regExpr, bool& searchAll) const
{
    backward   = m_bBackward;
    wholeWords = m_bWholeWords;
    matchCase  = m_bMatchCase;
    regExpr    = m_bRegExpr;
    searchAll  = m_bSearchAll;
}

bool Searcher::isMatched (const wchar_t* str, int offset, int len, const DelimitersMap& delim, int& start, int& end) const
{
    if (!str || offset > len) // workaround for rx bug
    {
        if (offset > 0)
            return false;
        str = L"";
        len = 0;
    }

    if (!m_bPatternCompiled)
        compileExpression();

    m_cxt->match[0].rm_so = offset; // from
    m_cxt->match[0].rm_eo = len;    // to

    // 24.05.2004 bug fix, search for empty string "^$" stops on the first occurence and does not follow forward/backward
    // we can search for empty string (^$) so we enforce one cycle to check it
    bool empty_cycle = !len ? true : false;

    while ((m_cxt->match[0].rm_so < len || empty_cycle)
    // 24.05.2004 bug fix, exception on attempt to replace "^" with any string (insert at start of line)
    && !regexec(&m_cxt->pattern, str, cnRegMatchSize, m_cxt->match, !offset ? REG_STARTEND : REG_NOTBOL|REG_STARTEND))
    {
        empty_cycle = false;

        if (m_bWholeWords)
        {
            if ((m_cxt->match[0].rm_so > 0 && !delim[str[m_cxt->match[0].rm_so - 1]])
            || (m_cxt->match[0].rm_eo < len && !delim[str[m_cxt->match[0].rm_eo]]))
            {
                m_cxt->match[0].rm_so++;
                m_cxt->match[0].rm_eo = len;
                continue;
            }
        }

        start = m_cxt->match[0].rm_so;
        end   = m_cxt->match[0].rm_eo;

        return true;
    }

    return false;
}

bool Searcher::isMatchedBackward (const wchar_t* str, int rlimit, int len, const DelimitersMap& delim, int& start, int& end) const
{
    if (!str) // workaround for rx bug
    {
        str = L"";
        len = 0;
    }

    if (!m_bPatternCompiled)
        compileExpression();

    bool retVal = false;

    m_cxt->match[0].rm_so = 0;      // from
    m_cxt->match[0].rm_eo = len;    // to

    // 24.05.2004 bug fix, search for empty string "^$" stops on the first occurence and does not follow forward/backward
    // we can search for empty string (^$) so we enforce one cycle to check it
    bool empty_cycle = !len ? true : false;

    while ((m_cxt->match[0].rm_so < len || empty_cycle)
    && !regexec(&m_cxt->pattern, str, cnRegMatchSize, m_cxt->match, REG_STARTEND))
    {
        empty_cycle = false;

        if (m_bWholeWords)
        {
            if ((m_cxt->match[0].rm_so > 0 && !delim[str[m_cxt->match[0].rm_so - 1]])
            || (m_cxt->match[0].rm_eo < len && !delim[str[m_cxt->match[0].rm_eo]]))
            {
                m_cxt->match[0].rm_so++;
                m_cxt->match[0].rm_eo = len;
                continue;
            }
        }

        if (m_cxt->match[0].rm_eo <= rlimit                     // ok,
        || (!m_cxt->match[0].rm_so && !m_cxt->match[0].rm_eo))  // we found emty string
        {
            retVal = true;
            start = m_cxt->match[0].rm_so;
            end   = m_cxt->match[0].rm_eo;

            m_cxt->match[0].rm_so++;
            m_cxt->match[0].rm_eo = len;
        }
        else
            break;
    }

    return retVal;
}


void Searcher::compileExpression () const
{
    if (!m_bPatternCompiled)
    {
        wstring pattern;

        if (m_bRegExpr)
        {
            pattern = m_strText;
        }
        else
        {
            auto it = m_strText.begin();
            for (; it != m_strText.end(); ++it)
            {
                switch (*it) // 17/05/2002 bug fix, cannot search for $,[,],^
                {
                case '.': case '*': case '\\':
                case '[': case ']':
                case '^': case '$':
                    pattern += '\\';
                }
                pattern += *it;
            }
        }

        int error = regcomp(&m_cxt->pattern, pattern.c_str(),
                            (m_bRegExpr ? REG_EXTENDED : 0) | (m_bMatchCase ? 0 :REG_ICASE));

        m_bPatternCompiled = true;

        if (error)
        {
            wchar_t buff[256];
            buff[0] = 0;
            regerror(error, &m_cxt->pattern, buff, sizeof buff);

            THROW_APP_EXCEPTION(string("Regular expression error:\n") + Common::str(buff));
        }
    }
}


bool Searcher::isSelectionMatched (const FindCtx& ctx) const
{
    ASSERT(ctx.m_storage);

    if (!m_bPatternCompiled)
        compileExpression();

    //if (start != end)
    if (ctx.m_line < ctx.m_storage->GetLineCount())
    {
        OEStringW lineBuff;
        ctx.m_storage->GetLineW(ctx.m_line, lineBuff);
        int len = lineBuff.length();
        const wchar_t* str = lineBuff.data();

        if (!str) str = L""; // workaround for rx bug

        m_cxt->match[0].rm_so = ctx.m_start;
        m_cxt->match[0].rm_eo = ctx.m_end;

        if(len >= ctx.m_end
        && !regexec(&m_cxt->pattern, str, cnRegMatchSize, m_cxt->match, REG_STARTEND))
            return true;
    }
    return false;
}

// all parameters are in-out
// const Storage*& pStorage, int& line, int& _start, int& _end, bool thruEof, bool& eofPassed, bool skipFirstNull
bool Searcher::Find (FindCtx& ctx) const
{
    ctx.m_eofPassed = false;

    if (!m_bPatternCompiled)
        compileExpression();

    vector<Storage*> storages;
    vector<Storage*>::const_iterator it;

    if (m_bSearchAll)
    {
        it = find(m_refList.begin(), m_refList.end(), ctx.m_storage);

        if (!m_bBackward)
        {
            storages.insert(storages.end(), it, m_refList.end());
            storages.insert(storages.end(), m_refList.begin(), it);
        }
        else
        {
            storages.insert(storages.end(), ++it, m_refList.end());
            storages.insert(storages.end(), m_refList.begin(), it);
            reverse(storages.begin(), storages.end());
        }
    }
    //else
    // if we find nothing in other windows then we try again for the current from top
    // 16.12.2002 bug fix, Search all windows does not find occurance above the current
        storages.push_back(ctx.m_storage);

    int llimit = ctx.m_start; // 23.09.2004 bug fix, replace all fails on 1 line selection
    int rlimit = ctx.m_end;

    // 24.05.2004 bug fix, search for empty string "^$" stops on the first occurence and does not follow forward/backward
    if (ctx.m_skipFirstNull
    && isSelectionMatched(ctx))
    {
        if (!m_bBackward)
        {
            if (!ctx.m_start && !ctx.m_end) // found empty string
            {
                if (++ctx.m_line == ctx.m_storage->GetLineCount())
                {
                    ctx.m_line = 0;
                    ctx.m_eofPassed = true;
                }
            }
        }
        else
        {
            // if something is selected and it is equal the expression, skip it!
            if (ctx.m_start || ctx.m_end)
            {
                rlimit = ctx.m_start;
            }
            else // found empty string
            {
                if (--ctx.m_line < 0)
                {
                    ctx.m_line = ctx.m_storage->GetLineCount()-1;
                    ctx.m_eofPassed = true;
                }
                rlimit = -1;
            }
        }

        if (ctx.m_eofPassed && storages.size() > 1)
            storages.erase(storages.begin());
    }

    // the loop works only for all windows searching
    //      if we find nothing in other windows then we try again for the current from top
    for (int attempt(0); attempt < (((!ctx.m_thruEof || storages.size()) == 1) ? 1 : 2); attempt++)
    {
        // all window loop
        for (it = storages.begin(); it != storages.end(); ++it)
        {
            int nlines = (*it)->GetLineCount();
            const DelimitersMap& delim = (*it)->GetDelimiters();

            // lines serching, double loop,
            //      in the first serching below current line
            //      in the second serching from top to current line
            int first_line = 0;
            for (int loop(0); loop < ((!ctx.m_thruEof || m_bSearchAll) ? 1 : 2); loop++)
            {
                if (!m_bBackward)
                {
                    for (int i(ctx.m_line); i < nlines; i++)
                    {
                        OEStringW lineBuff;
                        (*it)->GetLineW(i, lineBuff);
                        int len = lineBuff.length();
                        const wchar_t* str = lineBuff.data();

                        if (isMatched(str, llimit, len, delim, ctx.m_start, ctx.m_end))
                        {
                            ctx.m_storage = *it;
                            ctx.m_line    = i;
                            return true;
                        }
                        // reset for next line (after first)
                        llimit = 0;
                    }
                    // reset for upper text part searching
                    nlines = min(nlines, ctx.m_line + 1); // 22/05/2002 bug fix, search fails it it started after EOF
                    ctx.m_line = 0;
                    llimit = 0;
                }
                else
                {
                    if (ctx.m_line == -1 || ctx.m_line > nlines - 1)
                    {
                        ctx.m_line = nlines - 1;
                        rlimit = -1;
                    }

                    for (int i(ctx.m_line); i >= first_line; i--)
                    {
                        OEStringW lineBuff;
                        (*it)->GetLineW(i, lineBuff);
                        int len = lineBuff.length();
                        const wchar_t* str = lineBuff.data();

                        if (rlimit == -1)
                            rlimit = len;

                        if (isMatchedBackward(str, rlimit, len, delim, ctx.m_start, ctx.m_end))
                        {
                            ctx.m_storage = *it;
                            ctx.m_line    = i;
                            return true;
                        }
                        // reset for next line (after first)
                        rlimit = -1;
                    }
                    // reset for upper text part searching
                    first_line = ctx.m_line;
                    ctx.m_line = -1;
                    rlimit     = -1;
                }
                ctx.m_eofPassed = true;
            }
        }
    }

    return false;
}


bool Searcher::Replace (FindCtx& ctx) const
{
    if (wcspbrk(ctx.m_text.c_str(), L"\n\r") != NULL)
        THROW_APP_EXCEPTION("Replace error: replace string with <Carriage return> not supporded!");

    if (ctx.m_line < ctx.m_storage->GetLineCount())
    {
        OEStringW lineBuff;
        ctx.m_storage->GetLineW(ctx.m_line, lineBuff);
        int len = lineBuff.length();
        const wchar_t* str = lineBuff.data();

        int _start, _end;
        // check the current selection
        // !!! it's possible to supress double checking !!!
        if (isMatched(str, ctx.m_start, len, ctx.m_storage->GetDelimiters(), _start, _end)
        && _start == ctx.m_start && _end == ctx.m_end)
        {
            wstring buff;

            if (m_bRegExpr)
            {
                OEStringW lineBuff2;
                ctx.m_storage->GetLineW(ctx.m_line, lineBuff2);
                int len2 = lineBuff2.length();
                const wchar_t* str2 = lineBuff2.data();

                for (const wchar_t* ptr = ctx.m_text.c_str(); *ptr; ptr++)
                {
                    if (*ptr == '\\' && ::iswdigit(*(ptr+1)))
                    {
                        wchar_t _group[2] = { *(++ptr), 0 }; // 28/07/2002 bug fix, RegExp replace fails on \1...
                        int group = _wtoi(_group);

                        if (group < cnRegMatchSize
                        && m_cxt->match[group].rm_so != -1 && m_cxt->match[group].rm_eo != -1)
                        {
                            _CHECK_AND_THROW_(m_cxt->match[group].rm_so <= len2 && m_cxt->match[group].rm_eo <= len2,
                                              "Replace error: regular expresion match group failured!");
                            buff.append(str2 + m_cxt->match[group].rm_so, m_cxt->match[group].rm_eo - m_cxt->match[group].rm_so);
                        }
                    }
                    else
                    {
                        buff += *ptr;
                    }
                }
            }
            else
            {
                buff = ctx.m_text;
            }

            Storage::UndoGroup undoGroup(*ctx.m_storage);
            ctx.m_storage->ReplaceLinePart(ctx.m_line, m_cxt->match[0].rm_so, m_cxt->match[0].rm_eo, buff.c_str(), buff.length());

            ctx.m_end = ctx.m_start + buff.size();

            return true;
        }
    }
    return false;
}

// 09.09.2002 improvement, find/replace batch performance
int  Searcher::DoBatch (Storage* _storage, const wchar_t* text, ESearchBatch mode, Square& last)
{
    ASSERT(_storage);

    int counter = -1;

    bool _bBackward = m_bBackward;
    m_bBackward = false;

    counted_ptr<Storage::UndoGroup> pUndoGroup;
    counted_ptr<Storage::NotificationDisabler> pDisabler;

    try
    {
        FindCtx ctx;
        ctx.m_text = text;
        ctx.m_storage = _storage;
        const Storage* storage2 = 0;

        // 01.06.2004 bug fix, while loop has been changed because of empty line/start line replacement
        do
        {
            // 16.03.2003 bug fix, infinite cycle is possible on "Count" or "Mark all" in search all windows mode
            if (ctx.m_storage == _storage         // it's a start storage
            && ctx.m_eofPassed)
                break;

            // a next storage
            if (ctx.m_storage != storage2)
            {
                // it's again the first storage!
                if (storage2 && ctx.m_storage == _storage)
                    break;

                if (mode == esbReplace)
                    pUndoGroup = counted_ptr<Storage::UndoGroup>(new Storage::UndoGroup(*ctx.m_storage));

                if (mode != esbCount)
                    pDisabler  = counted_ptr<Storage::NotificationDisabler>(
                        new Storage::NotificationDisabler(ctx.m_storage, mode == esbReplace ? false : true)
                        );

                storage2 = ctx.m_storage;
            }

            if (++counter > 0)
            {
                switch (mode)
                {
                case esbReplace:
                    Replace(ctx);
                    if (_storage == ctx.m_storage)
                    {
                        last.start.line = last.end.line = ctx.m_line;
                        last.start.column = ctx.m_start;
                        last.end.column = ctx.m_end;
                    }
                    ctx.m_start = ctx.m_end;
                    break;
                case esbMark:
                    // 27.10.2004 bug fix, Infinite cycle on Count/Mark/Replace All if "Search All Windows" is on
                    ctx.m_storage->SetBookmark(ctx.m_line, eBmkGroup1, true);
                    ctx.m_start = ctx.m_end = 0;
                    ctx.m_line++;
                    break;
                case esbCount:
                    ctx.m_start = ctx.m_end;
                    break;
                }
            }

#pragma message("\tTODO: reimplement infinite cycle protection!\n")
            _CHECK_AND_THROW_(counter < 10000000,
                "Infinite cycle or the number of occurences is too much (> 10000000)! "
                "You can try to repeat it again.");

            ctx.m_skipFirstNull = counter ? true : false;
        }
        while (Find(ctx));
    }
    catch (...)
    {
        m_bBackward = _bBackward;
        throw;
    }
    m_bBackward = _bBackward;

    ASSERT(counter >= 0);

    return counter;
}

};
