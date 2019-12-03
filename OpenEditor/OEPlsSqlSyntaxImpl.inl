/* 
    Copyright (C) 2003,2005 Aleksey Kochetov

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

#include "OpenEditor/OEPlsSqlSyntax.h"

namespace OpenEditor
{ 
    template <class TokenArray>
    bool SyntaxNodeImpl<TokenArray>::FindToken (int line, int offset, const SyntaxNode*& node, int& index) const
    {
        if (m_tokens.size())
        {
            const Token& front = m_tokens.front();  
            const Token& back  = m_tokens.back();  
            TRACE("SyntaxNodeImpl::FindToken, Node=%s, line=%d, offset=%d\n", typeid(*this).name(), line, offset);
            TRACE("\tm_tokens.front().line=%d, m_tokens.front().offset=%d\n", front.line, front.offset);
            TRACE("\tm_tokens.back().line=%d, m_tokens.back().offset=%d\n", back.line, back.offset);

            if ((front.line < line 
                || (front.line == line && front.offset <= offset))
            && (back.line > line
                || (back.line == line && (back.offset + back.length) > offset)))
            {
                for (TokenArray::const_iterator it = m_tokens.begin();
                    it != m_tokens.end();
                    ++it)
                {
                    if (it->line == line && (it->offset <= offset && offset < it->offset + it->length))
                    {
                        node = this;
                        index = it - m_tokens.begin();
                        return true;
                    }
                }

                if (const SyntaxNode* child = GetChild())
                {
                    while (child && child->GetHitCode(line, 1) != ABOVE) // inverted to GetLineStatus
                    {
                        if (child->FindToken(line, offset, node, index))
                            return true;
                        child = child->GetSibling();
                    }
                }
            }
            // zero length closing token - EOS/EOF
            else if (!offset && !back.length 
            && line == back.line && offset == back.offset)
            {
                node = this;
                index = m_tokens.size()-1;
                return true;
            }
        }
        return false;
    }

    //TODO: close all parent construction where a child fails
    template <class TokenArray>
    void SyntaxNodeImpl<TokenArray>::GetLineStatus (LineStatusMap& statusMap, int& recursion) const
    {
//#ifdef _DEBUG
//    DbgPrintNode();
//#endif
        if (++recursion > RECURSION_LIMIT)
        {
            --recursion; // TODO: add a recursion controller class
            return;      // return silently because this method is called in background
        }

        int line = statusMap.GetBaseLine();
        int count = statusMap.GetLineCount();
        EHitCode hitCode = SyntaxNodeImpl<TokenArray>::GetHitCode(line, count);

        if (hitCode == HIT)
        {
            int begin = m_tokens.front().line;
            int end   = m_tokens.back().line;
            //TRACE("Name=%s, Line=%d, first=%d, last=%d\n",  typeid(*this).name(), line, begin,  end);

            if (typeid(*this) != typeid(ExpressionNode))
            {
                if (line <= begin && begin < line + count) 
                    statusMap[begin].beginCounter++;

                if (line <= end && end < line + count) 
                    statusMap[end].endCounter++;
                
                for (int i = max(begin, line), limit = min(end + 1, line + count); i < limit; i++)
                {
                    statusMap[i].level++;
                    //TRACE("i=%d, statusMap[i].level=%d\n",  i,  statusMap[i].level);
                    if (IsFailed() && GeFailureLine() <= i) 
                        statusMap[i].syntaxError = true;
                }
            }

            if (const SyntaxNode* child = GetChild())
            {
                while (child && child->GetHitCode(line, count) != ABOVE)//BELOW)
                {
                    child->GetLineStatus(statusMap, recursion);
                    child = child->GetSibling();
                }
            }
        }

        --recursion;
    }

    template <class TokenArray>
    void SyntaxNodeImpl<TokenArray>::GetToken (int index, int& line, int& offset, int& length) const
    {
        line   = m_tokens.at(index).line;
        offset = m_tokens.at(index).offset;
        length = m_tokens.at(index).length;
    }

    // always stop on the first and last token
    //   or on token with "stop sign"
    template <class TokenArray>
    void SyntaxNodeImpl<TokenArray>::GetNextToken (int index, int& line, int& offset, int& length, bool& backward) const
    {
        if (++index >= static_cast<int>(m_tokens.size()))
        {
            backward = true;
            index = 0;
        }
        else
        {
            backward = false;

            for (; index < static_cast<int>(m_tokens.size()); index++)
            {
                if (StopSign(m_tokens[index]))
                    break;
            }

            if (static_cast<int>(m_tokens.size()) == index)
                --index;
        }

        line   = m_tokens.at(index).line;
        offset = m_tokens.at(index).offset;
        length = m_tokens.at(index).length;
    }

    template <class TokenArray>
    void SyntaxNodeImpl<TokenArray>::Colapse (const Token& token)
    {
        if (!IsCompleted())
        {
            m_tokens.push_back(token);
            Failure(token.line);
        }
        //else
        //    ASSERT(0);
    }

    template <class TokenArray>
    SyntaxNode::EHitCode SyntaxNodeImpl<TokenArray>::GetHitCode (int line, int count) const
    {
        EHitCode retVal = UNKNOWN; //, ABOVE, HIT, BELOW

        if (!m_tokens.empty())
        {
            int begin = m_tokens.front().line;
            int end   = m_tokens.back().line;

            if ( ((line <= begin) && (begin <= (line + count - 1)))
            || (begin <= line) && (line <= (end)))
                retVal = HIT;
            else if ((line + count - 1) < begin) // a window is above this statement
                retVal = ABOVE;
            else if (line > end) // a window is below this statement
                retVal = BELOW;
        }

        return retVal;
    }

    template <class TokenArray>
    void SyntaxNodeImpl<TokenArray>::DbgPrintNode () const
    {
	    std::string indent;
	    indent.append(GetLevel(), '>');

	    if (m_tokens.size())
            TRACE("%4d%s%s%s\n", m_tokens.front().line + 1,  indent.c_str(), typeid(*this).name(), IsFailed() ? "failure" : "");
	    else
		    TRACE("%4d%s%s%s\n", -1, indent.c_str(), typeid(*this).name(), IsFailed() ? "failure" : "");

	    if (const SyntaxNode* node = GetChild())
            node->DbgPrintNode();

	    indent.clear();
	    indent.append(GetLevel(), '<');

	    if (m_tokens.size())
		    TRACE("%4d%s%s\n", m_tokens.back().line + 1,  indent.c_str(), typeid(*this).name());
	    else
		    TRACE("%4d%s%s\n", -1, indent.c_str(), typeid(*this).name());

	    if (const SyntaxNode* node = GetSibling())
            node->DbgPrintNode();
    }

};
