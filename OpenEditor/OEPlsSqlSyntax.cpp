/* 
    Copyright (C) 2002-2015 Aleksey Kochetov

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
    06.03.2003 bug fix, stack overflow on closing a big pl/sql file with syntax errors
    03.06.2003 bug fix, sql find match fails on select/insert/... if there is no ending ';'
    02.05.2008 bug fix, some syntax constructins are not recognized as valid because of comments 
                        solution is to skip COMMENT token
    2011.09.21 bug fix, PL/SQLAnalyzer fails on packages with startup/shuldtown/run procedures 
                        (those are script keywords)
*/

#include "stdafx.h"
#include "OpenEditor/OEPlsSqlSyntax.h"
#include "OpenEditor/OEPlsSqlSyntaxImpl.h"
#include "OpenEditor/OEPlsSqlSyntaxImpl.inl"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace OpenEditor
{

bool SyntaxNode::StopSign (EToken token) const
{
    switch (token)
    {
    case etDECLARE:				return true;
    case etFUNCTION:			return true;
    case etRETURN:			    return false;
    case etPROCEDURE:			return true;
    case etPACKAGE:				return true;
    case etBODY:				return false;
    case etBEGIN:				return true;
    case etEXCEPTION:			return true;
    case etEND:					return false;
    case etIF:					return false;
    case etTHEN:				return false;
    case etELSE:				return true;
    case etELSIF:				return true;
    case etWHEN:				return true;
    case etFOR:					return true;
    case etWHILE:				return true;
    case etLOOP:				return false;
    case etEXIT:				return true;
    case etIS:					return false;
    case etAS:					return false;
    case etSEMICOLON:			return true;
    case etQUOTE:				return true;
    case etDOUBLE_QUOTE:		return false;
    case etLEFT_ROUND_BRACKET:	return true;
    case etRIGHT_ROUND_BRACKET:	return true;
    case etMINUS:   			return false;
    case etSLASH:				return true;
    case etSTAR:				return false;
    case etSELECT:				return true;
    case etINSERT:				return true;
    case etUPDATE:				return true;
    case etDELETE:				return true;
    case etALTER:				return true;
    case etANALYZE:				return true;
    case etCREATE:				return true;
    case etDROP:				return true;
    case etFROM:				return true;
    case etWHERE:				return true;
    case etSET:					return true;
    case etOPEN:                return true;
    case etUNION:               return true;
    case etINTERSECT:           return true;
    case etMINUS_SQL:           return true;   
    case etTRIGGER:             return true;   
    }
    return false;
}

SyntaxNode::SyntaxNode ()                               
{ 
    m_failure = false;
    m_failureLine = -1;
    m_sibling = m_parent = m_child = 0; 
    m_lastChild = 0;
}

void SyntaxNode::clear ()
{
    m_failure = false;
    m_failureLine = -1;
    m_sibling = m_parent = m_child = 0; 
    m_lastChild = 0;
}

void SyntaxNode::AttachSibling (SyntaxNode* sibling)    
{ 
    if (!m_sibling)
    {
        m_sibling = sibling; 
        sibling->m_parent = m_parent; 
    }
    else
        m_sibling->AttachSibling(sibling);
}

void SyntaxNode::AttachChild (SyntaxNode* child)       
{ 
    if (!m_child)
    {
        m_lastChild =
        m_child = child; 
        child->m_parent = this; 
    }
    else
    {
        m_lastChild->AttachSibling(child);
        m_lastChild = child;
    }
}

int SyntaxNode::GetLevel () const
{
    const SyntaxNode* node = this;
    for (int level = 0; node && node->m_parent; node = node->m_parent, level++);
    return level;
}

///////////////////////////////////////////////////////////////////////////////

PlSqlAnalyzer::PlSqlAnalyzer ()
: m_root(new ScriptNode)
{
    m_top   = m_root.get();
    m_error = false;
}

PlSqlAnalyzer::~PlSqlAnalyzer ()
{
    try { EXCEPTION_FRAME;

        Clear();
    }
    _DESTRUCTOR_HANDLER_;
}

void PlSqlAnalyzer::Clear ()
{ 
    m_error = false; 

    m_top = m_root.get(); 

	std::vector<SyntaxNode*>::const_iterator it = m_pool.begin();
    for (; it != m_pool.end(); ++it)
        delete *it;

    m_pool.clear();

    m_root->Clear(); 
}

void PlSqlAnalyzer::Attach (SyntaxNode* node)
{
    if (m_top)
    {
        //TRACE("Open %s, level = %d, line = %d, col = %d\n", 
        //    node->GetName(), m_top->GetLevel()+1,
        //    node->m_tokens.front().line+1, node->m_tokens.front().offset+1);
        m_top->AttachChild(node);
        m_top = node;
        m_pool.push_back(node);
    }
}

void PlSqlAnalyzer::CloseTop (const Token& token)
{
    ASSERT(m_top);
    
    if (m_top)
    {
        //TRACE("Close %s, level = %d, line = %d, col = %d\n",
        //    m_top->GetName(), m_top->GetLevel(),
        //    m_top->GetTokens().front().line+1, m_top->GetTokens().front().offset+1);
        
        ASSERT(m_top->IsCompleted() || m_top->IsFailed());

        SyntaxNode* parent = m_top->GetParent();
        
        if (parent) 
            parent->OnChildCompletion(m_top, token);

        m_top = parent;
    }
}

void PlSqlAnalyzer::CloseAll (const Token& token) 
{
    ASSERT(m_top);
    ASSERT(token == etEOS || token == etFAILURE);

    if (m_top)
    {
        //TRACE("CloseAll, level = %d, line = %d, col = %d\n", 
        //    m_top->GetLevel(),token.line+1, token.offset+1);

        if (!m_error
        && token == etEOS
        && m_top->GetLevel() == 1
        && m_top->IsCompleted())
        {
            CloseTop(token);
        }
        else
        {
            while (m_top->GetParent())
            {
                if (token == etFAILURE
                && m_top->GetLevel() == 1)
                {
                    m_top->Failure(token.line); // set failed but don't colapse it
                    break;
                }
                m_top->Colapse(token);
                CloseTop(token);
            }
        }
    }
}

void PlSqlAnalyzer::PutToken (const Token& token)
{
	if (token == etEOF 
    || (token == etSLASH && token.offset == 0))
	{
        Token eos;
        eos.token = etEOS;
        //eos.line = max(0, token.line-1);
        eos.line = token.line;
        //eos.length = 1; // TODO: decide what's better - either 0 or 1
        std::auto_ptr<SyntaxNode> dummy;
		m_top->PutToken(eos, dummy);
//        ASSERT(!dummy.get());
        CloseAll(eos);
		m_error = false;
		return;
	}

	if (!m_error)
	{
        if (token == etCOMMENT
        || (token == etEOL && !m_top->WantsEOL()))
            return;

        std::auto_ptr<SyntaxNode> child;

        // 2011.09.21 bug fix, PL/SQLAnalyzer fails on packages with startup/shuldtown/run procedures (those are script keywords)
        if (token.token != etUNKNOWN
        && !m_top->IsScriptNode() // so it is NOT ScriptNode
        && !PlSqlParser::IsPlSqlToken(token.token) // and it is NOT a PL/SQL token
        && PlSqlParser::IsScriptToken(token.token) // and it is a script token 
        ) 
        {
            // then convert it to etUNKNOWN
            Token _token = token;
            _token.token = etUNKNOWN;
		    m_top->PutToken(_token, child);
        }
        else
		    m_top->PutToken(token, child);

        if (child.get())
        {
            Attach(child.release());
        }
        else if (m_top->IsFailed())  // DOTO: throw an exception on Failed!
		{
            //TODO: make it more tolerant

            TRACE("FALURE: token = %s, line = %d, col = %d\n", PlSqlParser::GetStringToken(token), token.line+1, token.offset+1);

			m_error = true;
            Token failure  = token;
            failure.token  = etFAILURE;
            failure.length = 0;
            CloseAll(failure);
			return;
		}
        else
        {
		    while (m_top->IsCompleted())
			    CloseTop(token);
        }
	}
}

bool PlSqlAnalyzer::FindToken (int line, int offset, const SyntaxNode*& node, int& index) const
{ 
    return m_root->FindToken(line, offset, node, index); 
}

void PlSqlAnalyzer::GetLineStatus (LineStatusMap& statusMap) const
{ 
    int recursion = 0;
    m_root->GetLineStatus(statusMap, recursion); 
}

};