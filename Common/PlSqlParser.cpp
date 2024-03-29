/* 
    SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2015 Aleksey Kochetov

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
    16.03.2003 bug fix, plsql match sometimes fails after some edit operations
*/

#include "stdafx.h"
#include <map>
#include "OpenEditor/OEHelpers.h"
#include "ExceptionHelper.h"
#include "PlSqlParser.h"
#include "StrHelpers.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace Common
{
namespace PlsSql
{
    using std::map;
    OpenEditor::DelimitersMap m_Delimiters(" \t\'\"\\()[]{}+-*/.,!?;:=><%|@&^");

PlSqlParser::PlSqlParser (SyntaxAnalyser* analyzer, TokenMapPtr tokenMap, ReservedPtr reserved)
    : m_analyzer(analyzer),
    m_tokenMap(tokenMap),
    m_reserved(reserved),
    m_sequenceOfLength(0)
{
    m_sequenceOf = eNone;
    SetTokenMap(m_tokenMap);
}

// 16.03.2003 bug fix, plsql match sometimes fails after some edit operations
void PlSqlParser::Clear ()
{
    m_sequenceOf = eNone;
}

void PlSqlParser::SetTokenMap (TokenMapPtr tokenMap)
{
    m_tokenMap = tokenMap;
}

void PlSqlParser::PutEOF (int line)
{
    Token token;
    token = etEOF;
    token.line = line;
    token.offset = 0;
    token.length = 0;
    m_analyzer->PutToken(token);
}

bool PlSqlParser::PutLine (int line, const wchar_t* str, int length)
{
    //TRACE("PutLine: %4d:%s\n", line+1, string(str, length).c_str());

    Token token;

    for (int offset = 0; offset < length; )
    {
        token = etUNKNOWN;
        token.line = line;
        token.offset = static_cast<Token::size_pos>(offset);
        token.length = 0;
        token.reserved = 0;
        token.value.clear();

        wstring buffer;

        if (m_sequenceOf == eNone)
        {
            // skip white space
            for (; offset < length && iswspace(str[offset]); offset++)
                {}

            // check EOL
            if (!(offset < length))
                break;

            token.offset = static_cast<Token::size_pos>(offset);
            // read string token
            if (m_Delimiters[str[offset]])
                buffer += str[offset++];
            else
                for (; offset < length && !m_Delimiters[str[offset]]; offset++)
                    buffer += towupper(str[offset]);

            token.length = static_cast<Token::size_pos>(buffer.length());
            token.value = buffer;
        }
        else
        {
            switch (m_sequenceOf)
            {
            case eEndLineComment:
                m_sequenceOfLength += length - offset;
                m_sequenceToken.value = wstring(str + m_sequenceToken.offset, m_sequenceOfLength);
                offset = length;
                m_sequenceOf = eNone;
                token = m_sequenceToken;
                break;
            case eQuotedString:
                for (; offset < length; offset++)
                {
                    m_sequenceOfLength++;
                    if (str[offset] == '\'')
                    {
                        offset++;
                        m_sequenceOf = eNone;
                        token = m_sequenceToken;
                        break;
                    }
                }
                break;
            case eDblQuotedString:
                for (; offset < length; offset++)
                {
                    m_sequenceOfLength++;
                    if (str[offset] == '\"')
                    {
                        offset++;
                        m_sequenceOf = eNone;
                        token = m_sequenceToken;
                        break;
                    }
                }
                break;
            case eComment:
                for (; offset < length; offset++)
                {
                    m_sequenceOfLength++;
                    m_sequenceToken.value += str[offset];

                    if (offset && str[offset-1] == '*' && str[offset] == '/')
                    {
                        offset++;
                        m_sequenceOf = eNone;
                        token = m_sequenceToken;
                        break;
                    }
                }
                break;
            }

            // sequence end
            if (m_sequenceOf == eNone)
            {
                token.length = static_cast<Token::size_pos>(m_sequenceOfLength);
                m_sequenceOfLength = 0;
            }
        }


        if (token == etUNKNOWN && !buffer.empty())
        {
            Reserved::const_iterator resIt = m_reserved->find(buffer);
            token.reserved = (resIt != m_reserved->end()) ? resIt->second : 0;
            std::map<wstring, int>::const_iterator it = m_tokenMap->find(buffer);

            if (it != m_tokenMap->end())
            {
                token = (EToken)it->second;

                switch (token)
                {
                case etQUOTE:
                    m_sequenceOf = eQuotedString; // start string accumulation
                    m_sequenceToken = etQUOTED_STRING;
                    m_sequenceToken.line = token.line;
                    m_sequenceToken.offset = token.offset;
                    m_sequenceToken.length = token.length;
                    m_sequenceToken.value.clear();
                    m_sequenceOfLength = 1;
                    break;
                case etDOUBLE_QUOTE:
                    m_sequenceOf = eDblQuotedString; // start string accumulation
                    m_sequenceToken = etDOUBLE_QUOTED_STRING;
                    m_sequenceToken.line = token.line;
                    m_sequenceToken.offset = token.offset;
                    m_sequenceToken.length = token.length;
                    m_sequenceToken.value.clear();
                    m_sequenceOfLength = 1;
                    break;
                case etMINUS:
                    if (offset < length && str[offset] == '-')
                    {
                        m_sequenceOf = eEndLineComment; // skip line remainer
                        m_sequenceToken = etCOMMENT;
                        m_sequenceToken.line = token.line;
                        m_sequenceToken.offset = token.offset;
                        m_sequenceToken.length = token.length;
                        m_sequenceToken.value.clear();
                        m_sequenceOfLength = 2;
                        offset++;
                    }
                    break;
                case etSLASH:
                    if (offset < length && str[offset] == '*')
                    {
                        m_sequenceOf = eComment; // skip comment
                        m_sequenceToken = etCOMMENT;
                        m_sequenceToken.line = token.line;
                        m_sequenceToken.offset = token.offset;
                        m_sequenceToken.length = token.length;
                        m_sequenceToken.value = L"/*";
                        m_sequenceOfLength = 2;
                        offset++;
                    }
                    break;
                }
            }
        }

        if (m_sequenceOf == eNone)
        {
            /*
            1234/67    /
            length=7   1
            offset=4   0
            seg(0,4)   0,0
            seg(5,2)   1,0
            */
            if (token == etSLASH 
            && is_blank_line(str,  token.offset)
            && is_blank_line(str + token.offset + token.length, length - token.offset - token.length))
                token = etEOS;
            m_analyzer->PutToken(token);
        }
    }

    if (m_sequenceOf == eEndLineComment)
        m_sequenceOf = eNone;
    else if (m_sequenceOf == eComment)
        m_sequenceToken.value += '\n';
    
    if (m_sequenceOf == eNone)
    {
        token = etEOL;
        token.line = line;
        token.offset = static_cast<Token::size_pos>(length);
        m_sequenceToken.length = 0;
        m_analyzer->PutToken(token);
    }

    return true;
}

}; // namespace PlsSql
}; // namespace Common