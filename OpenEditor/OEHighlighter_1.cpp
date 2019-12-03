/*
    Copyright (C) 2002,2003,2015 Aleksey Kochetov

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
    07.03.2003 bug fix, some shell highlighter problem have been fixed
    07.03.2003 bug fix, PlSqlHighlighter does not process '&&'
    30.03.2003 improvement, SQR support has been added
    2015.10.07 bug fix, incorrect highlithing in lines stated with 's'
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
///////////////////////////////////////////////////////////////////////////////

void CPlusPlusHighlighter::Init (const VisualAttributesSet& set_)
{
    CommonHighlighter::Init(set_);
    // required hardcoded categories
    m_preprocessorAttrs = set_.FindByName("Preprocessor");
}

void CPlusPlusHighlighter::NextLine (const char* currentLine, int currentLineLength)
{
    m_includeDirective = false;
    m_openBrace = 0;
    CommonHighlighter::NextLine(currentLine, currentLineLength);
}

void CPlusPlusHighlighter::NextWord (const char* str, int len, int pos)
{
    if (m_isStartLine && len == (sizeof("#include") - 1)
    && !strncmp(str, "#include", len))
        m_includeDirective = true;

    if (m_openBrace)
    {
        if (m_openBrace == '<' && *str == '>'
            || m_openBrace == '\"' && *str == '\"')
        {
            m_openBrace = 0;
            m_includeDirective = false;
        }
        m_current = m_preprocessorAttrs;
        return;
    }

    if (m_includeDirective && m_seqOf & ePlainText && (*str == '<' || *str == '\"'))
    {
        m_openBrace = *str;
        m_current = m_preprocessorAttrs;
        return;
    }

    CommonHighlighter::NextWord(str, len, pos);
}

///////////////////////////////////////////////////////////////////////////////

void PlSqlHighlighter::Init (const VisualAttributesSet& set_)
{
    CommonHighlighter::Init(set_);
    // required hardcoded categories
    m_bindVarAttrs = set_.FindByName("Bind Variable");
    m_substitutionAttrs = set_.FindByName("Substitution");
    m_fileNameAttrs = set_.FindByName("File name (@ & @@)");

    vector<string>::const_iterator it = m_keywordGroups.begin();
    for (unsigned int i = 0; i < m_keywordGroups.size(); ++i)
        if (m_keywordGroups[i] == "User Objects")
        {
            m_userObjectsGroup = i;
            break;
        }
}

void PlSqlHighlighter::NextLine (const char* currentLine, int currentLineLength)
{
    if (m_openBrace)
    {
        m_openBrace = 0;
        if (m_seqOf == eString)
            m_current = m_stringAttr;
    }

    CommonHighlighter::NextLine(currentLine, currentLineLength);

    if (m_isStartLine && !(m_seqOf & eStringGroup))
	{
		for (int i = 0; i < currentLineLength; i++)
			if (currentLine[i] == '@')
			{
				m_openBrace = '@';
				m_current = m_fileNameAttrs;
				break;
			}
			else if (!(currentLine[i] == ' ' || currentLine[i] == '\t'))
				break;
	}
}

bool PlSqlHighlighter::IsPlainText()
{
    if (m_openBrace) 
        return false; 
    
    return CommonHighlighter::IsPlainText(); 
}

void PlSqlHighlighter::NextWord (const char* str, int len, int pos)
{
    switch (m_openBrace)
    {
    case '@':
        //if (*str == '@') break;
        m_current = m_fileNameAttrs;
        return;
    case ':':
        if (*str != '\'')
        {
            m_current = m_bindVarAttrs;
            m_openBrace = '*';
            return;
        }
        m_openBrace = 0;
        if (m_seqOf == eString) m_current = m_stringAttr;
        break;
    case '&':
        if (*str != '\'' && *str != '&') // bug fix, PlSqlHighlighter does not process '&&'
        {
            m_current = m_substitutionAttrs;
            m_openBrace = '*';
            return;
        }
        m_openBrace = 0;
        if (m_seqOf == eString) m_current = m_stringAttr;
        break;
    case '*':
        m_openBrace = 0;
        if (m_seqOf == eString) m_current = m_stringAttr;
        break;
    case 's':
        m_current = m_fileNameAttrs;
        return;
    }

    if (!(m_seqOf & eCommentGroup))
    {
        switch (*str)
        {
        case ':':
            if ((str + 1) == (m_currentLine + m_currentLineLength) || !isalnum(str[1]))
                break;
            m_current = m_bindVarAttrs;
            m_openBrace = *str;
            return;
        case '&':
            if (str > m_currentLine && str[-1] == '\\')
                break;
            m_current = m_substitutionAttrs;
            m_openBrace = *str;
            return;
        //case '@':
        //    if (m_isStartLine && !(m_seqOf & eStringGroup))
        //        m_openBrace = *str;
        //    break;
        case 'S':
        case 's':
            if (m_isStartLine 
                && len >= 3 && len <= 5 // 2015.10.07 bug fix, incorrect highlithing ins stated with 's'
                && !strnicmp(str, "spool", len)
            )
                m_openBrace = 's';
        }
    }

    CommonHighlighter::NextWord(str, len, pos);
}


bool PlSqlHighlighter::IsKeyword (const char* str, int len, string& keyword)
{
    string key(str, len);

    if (!m_caseSensiteve)
        for (string::iterator it = key.begin(); it != key.end(); ++it)
            *it = toupper(*it);

    LanguageKeywordMapConstIterator
        it = m_LanguageKeywordMap->find(key);

	if (it != m_LanguageKeywordMap->end()
    && it->second.groupIndex != m_userObjectsGroup ) // do not want to NORMALIZE user objects!
    {
        keyword = it->second.keyword;
        return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

void SqrHighlighter::Init (const VisualAttributesSet& set_)
{
    CommonHighlighter::Init(set_);
    // required hardcoded categories
    m_variablesAttrs = set_.FindByName("Variables");
    m_preprocessorAttrs = set_.FindByName("Preprocessor");
}

void SqrHighlighter::NextWord (const char* str, int len, int pos)
{
    CommonHighlighter::NextWord(str, len, pos);

    if (m_seqOf == ePlainText)
    {
        switch (*str)
        {
        case '#':
            if (!strnicmp(str, "#debug", sizeof("#debug")-1))
            {
                m_current = m_preprocessorAttrs;
                break;
            }
        case '$':
        case '%':
        case '&':
        case '@':
            m_current = m_variablesAttrs;
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void ShellHighlighter::Init (const VisualAttributesSet& set_)
{
    // owerride required hardcoded categories
    m_characterLabel = m_stringLabel;

    CommonHighlighter::Init(set_);

    // required hardcoded categories
    m_substAttrs = set_.FindByName("Substitution");
}

void ShellHighlighter::NextLine (const char* currentLine, int currentLineLength)
{
    m_isBraceToken = m_isSubstitution = false;
    CommonHighlighter::NextLine(currentLine, currentLineLength);
}

void ShellHighlighter::NextWord (const char* str, int len, int pos)
{
    if (m_isSubstitution)
    {
        m_current = m_substAttrs;

        if (*str == '}') m_isBraceToken = false;
        else if (*str == '{') m_isBraceToken = true;

        if (!m_isBraceToken)
            m_isSubstitution = false;

        return;
    }

    if (m_seqOf & ePlainGroup)
    {
        if (*str == '$')
        {
            m_current = m_substAttrs;
            m_isSubstitution = true;
            m_isBraceToken = false;
            return;
        }
    }

    CommonHighlighter::NextWord(str, len, pos);
}

///////////////////////////////////////////////////////////////////////////////

void PerlHighlighter::Init (const VisualAttributesSet& set_)
{
    // owerride required hardcoded categories
    m_characterLabel = m_stringLabel;

    CommonHighlighter::Init(set_);

    // required hardcoded categories
    m_vartAttrs = set_.FindByName("Variables");
}

void PerlHighlighter::NextWord (const char* str, int len, int pos)
{
    if (m_variable) 
    {
        m_current = m_orgAttrs;
        m_variable = false;
    }

    CommonHighlighter::NextWord(str, len, pos);

    if (len > 0 
    && (m_seqOf == ePlainText || m_seqOf == eCharacter) )
    {
        switch (str[0])
        {
        case '%':   //Hash
        case '&':   //Anonymous subroutine
        case '*':   //Typeglob 
            if (!(len > 1 && isalpha(str[1]))) break;

        case '$':   //Scalar
        case '@':   //List

            m_orgAttrs = m_current;
            m_current = m_vartAttrs;
            m_variable = true;
            return;
        }
    }
}

};