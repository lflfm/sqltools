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
    15.12.2002 bug fix, Pl/Sql highliter fails on start line comments
    10.03.2003 bug fix, start-line comment (promt & remark) should be separeted by delimiter from text
    30.03.2003 improvement, SQR support has been added
*/

#include "stdafx.h"
#include "OpenEditor/OEHighlighter.h"
#include "COMMON/VisualAttributes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace OpenEditor
{

HighlighterBasePtr HighlighterFactory::CreateHighlighter (const char* name)
{
    if (!strcmp(name, "NONE"))
        return HighlighterBasePtr();
    if (!strcmp(name, "PL/SQL"))
        return HighlighterBasePtr(new PlSqlHighlighter);
    if (!strcmp(name, "C++"))
        return HighlighterBasePtr(new CPlusPlusHighlighter);
    if (!strcmp(name, "Shell"))
        return HighlighterBasePtr(new ShellHighlighter);
    if (!strcmp(name, "XML"))
        return HighlighterBasePtr(new XmlHighlighter);
    if (!strcmp(name, "SQR"))
        return HighlighterBasePtr(new SqrHighlighter);
    if (!strcmp(name, "PERL"))
        return HighlighterBasePtr(new PerlHighlighter);

    return HighlighterBasePtr(new CommonHighlighter(name));
}


HighlighterBase::HighlighterBase ()
{
    m_current.color = RGB(0,0,0);
    m_current.font  = efmNormal;
    m_printableSpaces = false;
}


///////////////////////////////////////////////////////////////////////////////
// CommonHighlighter
//  TODO:
//      PL/SQL  - '.' as separator, sys package hook, a new list of sys pakages
//      Shell   - one string
//      XML/XSL -
//
    inline
    pair<std::wstring, std::wstring> wpair (pair<std::string, std::string> p)
    {
        return std::pair<std::wstring, std::wstring>(Common::wstr(p.first), Common::wstr(p.second));
    }

    inline
    pair<std::string, std::string> spair (pair<std::wstring, std::wstring> p)
    {
        return std::pair<std::string, std::string>(Common::str(p.first), Common::str(p.second));
    }

    inline
    set<std::wstring> wset (set<std::string> nset)
    {
        set<std::wstring> wset;
        for (auto it = nset.begin(); it != nset.end(); ++it)
            wset.insert(Common::wstr(*it));
        return wset;
    }

CommonHighlighter::CommonHighlighter (const char* langName)
    :m_symbolFastMap(false), m_lineCommentFastMap(false)
{
    m_currentLine = 0;
    m_currentLineLength = 0;

    m_seqOf = ePlainText;
    m_isStartLine = false;

    m_textLabel        = "Text";
    m_numberLabel      = "Number";
    m_characterLabel   = "Character";
    m_stringLabel      = "String";
    m_commentLabel     = "Comment";
    m_lineCommentLabel = "Line Comment";

    const LanguagePtr langPtr = LanguagesCollection::Find(langName);

    m_LanguageKeywordMap = langPtr->GetLanguageKeywordMap();
    m_keywordGroups      = langPtr->GetKeywordGroups();
    m_caseSensiteve      = langPtr->IsCaseSensitive();
    m_commentPair        = wpair(langPtr->GetCommentPair());
    m_endLineComment     = Common::wstr(langPtr->GetEndLineComment());
    m_endLineCommentCol  = langPtr->GetEndLineCommentCol();
    m_startLineComment   = wset(langPtr->GetStartLineComment());
    m_escapeChar         = Common::wstr(langPtr->GetEscapeChar());
    m_stringPair         = wpair(langPtr->GetStringPair());
    m_charPair           = wpair(langPtr->GetCharPair());

    m_delimiters.Set(langPtr->GetDelimiters().c_str());

    m_symbolFastMap.erase();
    m_lineCommentFastMap.erase();

    if (!m_commentPair.first.empty())  m_symbolFastMap.assign(*m_commentPair.first.begin()  , true);
    if (!m_commentPair.second.empty()) m_symbolFastMap.assign(*m_commentPair.second.rbegin(), true);
    if (!m_stringPair.first.empty())   m_symbolFastMap.assign(*m_stringPair.first.begin()   , true);
    if (!m_stringPair.second.empty())  m_symbolFastMap.assign(*m_stringPair.second.rbegin() , true);
    if (!m_charPair.first.empty())     m_symbolFastMap.assign(*m_charPair.first.begin()     , true);
    if (!m_charPair.second.empty())    m_symbolFastMap.assign(*m_charPair.second.rbegin()   , true);

    if (!m_endLineComment.empty())
    {
        m_symbolFastMap.assign(*m_endLineComment.begin(), true);
        m_lineCommentFastMap.assign(*m_endLineComment.begin(), true);
    }

    if (!m_endLineComment.empty())
    {
        wchar_t ch = *m_endLineComment.begin();
        if (m_caseSensiteve)
        {
            m_symbolFastMap.assign(ch, true);
            m_lineCommentFastMap.assign(ch, true);
        }
        else
        {
            m_symbolFastMap.assign(towupper(ch), true);
            m_symbolFastMap.assign(towlower(ch), true);
            m_lineCommentFastMap.assign(towupper(ch), true);
            m_lineCommentFastMap.assign(towlower(ch), true);
        }
    }

    auto it = m_startLineComment.begin();
    for (; it != m_startLineComment.end(); ++it)
        if (it->size())
        {
            auto ch = it->at(0);
            if (m_caseSensiteve)
            {
                m_symbolFastMap.assign(ch, true);
                m_lineCommentFastMap.assign(ch, true);
            }
            else
            {
                m_symbolFastMap.assign(towupper(ch), true);
                m_symbolFastMap.assign(towlower(ch), true);
                m_lineCommentFastMap.assign(towupper(ch), true);
                m_lineCommentFastMap.assign(towlower(ch), true);
            }
        }
}

    inline
    void HighlighterBase::Attrib::operator = (const VisualAttribute& attr)
    {
        color = attr.m_Foreground;
        font  = (attr.m_FontBold ? efmBold : efmNormal)
            | (attr.m_FontItalic ? efmItalic : efmNormal)
            | (attr.m_FontUnderline ? efmUnderline : efmNormal);
    }

void CommonHighlighter::Init (const VisualAttributesSet& set_)
{
    // required hardcoded categories
    m_textAttr   = set_.FindByName(m_textLabel);
    m_numberAttr = set_.FindByName(m_numberLabel);

    if (!m_charPair.first.empty())    m_characterAttr   = set_.FindByName(m_characterLabel);
    if (!m_stringPair.first.empty())  m_stringAttr      = set_.FindByName(m_stringLabel);
    if (!m_commentPair.first.empty()) m_commentAttr     = set_.FindByName(m_commentLabel);
    if (!m_endLineComment.empty())    m_lineCommentAttr = set_.FindByName(m_lineCommentLabel);

    // language depended categories
    m_keyAttrs.resize(m_keywordGroups.size());
    std::vector<string>::const_iterator it = m_keywordGroups.begin();
    for (int i(0); it != m_keywordGroups.end(); ++it, i++)
        m_keyAttrs.at(i) = set_.FindByName(it->c_str());
}

void CommonHighlighter::NextLine (const wchar_t* currentLine, int currentLineLength)
{
    m_currentLine = currentLine;
    m_currentLineLength = currentLineLength;

    m_isStartLine = true;

    if (!(m_seqOf & eMultilineGroup))
    {
        m_current = m_textAttr;
        m_seqOf = ePlainText;
        m_printableSpaces = false;
    }
}

bool CommonHighlighter::closingOfSeq (const wchar_t* str, int /*len*/, int /*pos*/)
{
    switch (m_seqOf)
    {
    case eComment:
        if ((str - m_commentPair.second.size() + 1) >= m_currentLine
            && !wcsncmp(
                    m_commentPair.second.c_str(),
                    str - m_commentPair.second.size() + 1,
                    m_commentPair.second.size())
        )
        {
            m_seqOf = ePlainText;
            m_printableSpaces = false;
            return true;
        }
        break;
    case eCharacter:
        if ((str - m_charPair.second.size() + 1) >= m_currentLine
            && !wcsncmp(
                    m_charPair.second.c_str(),
                    str - m_charPair.second.size() + 1,
                    m_charPair.second.size())
            && (
                // escape is not defined or the previos fragment is not escape or it is not double escape
                m_escapeChar.empty()
                // the current position < escape size
                || str - m_currentLine < static_cast<int>(m_escapeChar.size())
                // the previous fragment is not escape
                || wcsncmp(m_escapeChar.c_str(), str - m_escapeChar.size(), m_escapeChar.size())
                // it is double escape
                || (
                    // the current position >= 2 * escape size
                    str - m_currentLine >= 2 * static_cast<int>(m_escapeChar.size())
                    // the previous previous fragment is escape
                    && !wcsncmp(m_escapeChar.c_str(), str - 2 * m_escapeChar.size(), m_escapeChar.size())
                )
            )
        )
        {
            m_seqOf = ePlainText;
            m_printableSpaces = false;
            return true;
        }
        break;
    case eString:
        if ((str - m_stringPair.second.size() + 1) >= m_currentLine
            && !wcsncmp(
                    m_stringPair.second.c_str(),
                    str - m_stringPair.second.size() + 1,
                    m_stringPair.second.size())
            && (
                // escape is not defined or the previos fragment is not escape or it is not double escape
                m_escapeChar.empty()
                // the current position < escape size
                || str - m_currentLine < static_cast<int>(m_escapeChar.size())
                // the previous fragment is not escape
                || wcsncmp(m_escapeChar.c_str(), str - m_escapeChar.size(), m_escapeChar.size())
                // it is double escape
                || (
                    // the current position >= 2 * escape size
                    str - m_currentLine >= 2 * static_cast<int>(m_escapeChar.size())
                    // the previous previous fragment is escape
                    && !wcsncmp(m_escapeChar.c_str(), str - 2 * m_escapeChar.size(), m_escapeChar.size())
                )
            )
        )
        {
            m_seqOf = ePlainText;
            m_printableSpaces = false;
            return true;
        }
        break;
    }
    return false;
}

bool CommonHighlighter::openingOfSeq (const wchar_t* str, int len, int pos)
{
    bool comment = false;

    if (m_lineCommentFastMap[*str])
    {
        if (!m_endLineComment.empty()
        && (m_endLineCommentCol == -1 || m_endLineCommentCol == pos)
        && !wcsncmp(str, m_endLineComment.c_str(), m_endLineComment.size())
        )
            comment = true;

        if (m_isStartLine)
        {
            auto it = m_startLineComment.begin();
            for (; it != m_startLineComment.end(); ++it)
                if (len == static_cast<int>(it->length()) // 10.03.2003 bug fix, start-line comment (promt & remark) should be separeted by delimiter from text
                && (m_caseSensiteve && !wcsncmp(str, it->c_str(), it->length())
                    || !m_caseSensiteve && !wcsnicmp(str, it->c_str(), it->length()))
                )
                {
                    comment = true;
                    break;
                }
        }
    }

    if (comment)
    {
        m_seqOf = eLineComment;
        m_current = m_lineCommentAttr;
        m_printableSpaces = true;
        return true;
    }
    else if (!m_charPair.first.empty()
        && (m_currentLine + m_currentLineLength) >= (str + m_charPair.first.size())
        && !wcsncmp(str, m_charPair.first.c_str(), m_charPair.first.size()))
    {
        m_seqOf = eCharacter;
        m_current = m_characterAttr;
        m_printableSpaces = true;
        return true;
    }
    else if (!m_stringPair.first.empty()
        && (m_currentLine + m_currentLineLength) >= (str + m_stringPair.first.size())
        && !wcsncmp(str, m_stringPair.first.c_str(), m_stringPair.first.size()))
    {
        m_seqOf = eString;
        m_current = m_stringAttr;
        m_printableSpaces = true;
        return true;
    }
    else if (!m_commentPair.first.empty()
        && (m_currentLine + m_currentLineLength) >= (str + m_commentPair.first.size())
        && !wcsncmp(str, m_commentPair.first.c_str(), m_commentPair.first.size()))
    {
        m_seqOf = eComment;
        m_current = m_commentAttr;
        m_printableSpaces = true;
        return true;
    }

    return false;
}

// export some additional functionality
bool CommonHighlighter::IsKeyword (const wchar_t* str, int len, std::wstring& keyword)
{
    std::wstring key(str, len);

    if (!m_caseSensiteve)
        for (auto it = key.begin(); it != key.end(); ++it)
            *it = towupper(*it);

    LanguageKeywordMapConstIterator
        it = m_LanguageKeywordMap->find(key);

    if (it != m_LanguageKeywordMap->end())
    {
        keyword = it->second.keyword;
        return true;
    }

    return false;
}

bool CommonHighlighter::isKeyword (const wchar_t* str, int len)
{
    std::wstring key(str, len);

    if (!m_caseSensiteve)
        for (auto it = key.begin(); it != key.end(); ++it)
            *it = towupper(*it);

    LanguageKeywordMapConstIterator
        it = m_LanguageKeywordMap->find(key);

    if (it != m_LanguageKeywordMap->end())
    {
        m_seqOf = eKeyword;
        m_current = m_keyAttrs.at(it->second.groupIndex);
        return true;
    }
    else
    {
        m_seqOf = ePlainText;
        m_current = m_textAttr;
        return false;
    }
}

// it's the simplest implementeation of isDigit
// we check only the first character
bool CommonHighlighter::isDigit (const wchar_t* str, int /*len*/)
{
    if (iswdigit(*str))
    {
        m_current = m_numberAttr;
        return true;
    }

    return false;
}

void CommonHighlighter::NextWord (const wchar_t* str, int len, int pos)
{
    _ASSERTE(str && len > 0);

    if (!(m_symbolFastMap[*str] && closingOfSeq(str, len, pos))
        && (m_seqOf & ePlainGroup)
        && !(m_symbolFastMap[*str] && openingOfSeq(str, len, pos))
        && !isDigit(str, len)
    )
        isKeyword(str, len);

    m_isStartLine = false;
}

void CommonHighlighter::InitStorageScanner (MultilineQuotesScanner& scanner) const
{
    scanner.ClearSettings ();
    scanner.SetCaseSensitive(m_caseSensiteve);

    auto it = m_startLineComment.begin();
    for (; it != m_startLineComment.end(); ++it)
        scanner.AddStartLineQuotes(Common::str(*it));

    if (!m_endLineComment.empty())    scanner.AddSingleLineQuotes(Common::str(m_endLineComment));
    if (!m_commentPair.first.empty()) scanner.AddMultilineQuotes (spair(m_commentPair));
    if (!m_stringPair.first.empty())  scanner.AddMultilineQuotes (spair(m_stringPair));
    if (!m_charPair.first.empty())    scanner.AddMultilineQuotes (spair(m_charPair));
    if (!m_escapeChar.empty())        scanner.AddEscapeChar      (Common::str(m_escapeChar));

    scanner.SetDelimitersMap(m_delimiters);
}

void CommonHighlighter::SetMultilineQuotesState (int state, int quoteId, bool)
{
    m_current = m_textAttr;
    m_seqOf = ePlainText;

    if (state > 0)
    {
        if (m_commentPair.first.empty()) quoteId++;
        if (m_stringPair.first.empty()) quoteId++;
        //if (m_charPair.first.empty())   quoteId++;

        switch (quoteId)
        {
        case 0:
            m_seqOf = eComment;
            m_current = m_commentAttr;
            break;
        case 1:
            m_seqOf = eString;
            m_current = m_stringAttr;
            break;
        case 2:
            m_seqOf = eCharacter;
            m_current = m_characterAttr;
        }

        m_printableSpaces = true;
    }
}

};
