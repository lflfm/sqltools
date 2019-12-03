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
*/

#include "stdafx.h"
#include <map>
#include <boost/array.hpp>
//#include <boost/pool/object_pool.hpp>
#include <COMMON/AppGlobal.h>
#include <COMMON/ExceptionHelper.h>
#include "OpenEditor/OEPlsSqlSyntaxImpl.h"
#include "OpenEditor/OEPlsSqlSyntaxImpl2.h"
#include "OpenEditor/OEPlsSqlSyntaxImpl.inl"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
    factory matrix
                    toplevel dml-sql ddl-sql     pls     expr  
    BlockNode              1       0       0       1        0
    PackageNode            1       0       0       0        0
    ProcedureNode          1       0       0       1        0
    SelectionNode          0       0       0       1        0
    CaseNode               0       0       0       1        0
    SqlCaseNode            0       1       0       1        1
    IterationNode          0       0       0       1        0
    ExpressionNode         1       1       1       1        1
    OpenCursorNode         0       0       0       1        0
    SelectNode             1       1*      1*      1        1
    InsertNode             1       0       0       1        0
    UpdateNode             1       0       0       1        0
    DeleteNode             1       0       0       1        0
    SqlPlusCmdNode         1       0       0       0        0
    SqlSttmNode            1       0       0       1        0

    the toplevel factoty has to recognize a syntax by several 
    tokents (create procedure, create or replace ...)
    the pls/sql factory is simpler because only single token 
    recognition is required
    dml-sql and ddl-sql are equal and require no so much work
*/

namespace OpenEditor
{
    typedef SyntaxNode* (*SimpleFactory)();
    //typedef FixedArray<Token, 7> RecognitionPattern;
    typedef boost::array<EToken,7> RecognitionPatternImpl;

    //template <class T> SyntaxNode* SimpleFactoryImpl () { return new T; }
    template <class T> 
    SyntaxNode* SimpleFactoryImpl () 
    { 
        //static boost::object_pool<T> pool(100);
        //return pool.construct(); 
        //TRACE2("%s sizeof = %d\n", typeid(Token).name(), sizeof(Token));
        //TRACE2("%s sizeof = %d\n", typeid(T).name(), sizeof(T));
        //TRACE2("%s sizeof = %d\n", typeid(FixedArray<Token, 10>).name(), sizeof(FixedArray<Token, 10>));
        //TRACE2("%s sizeof = %d\n", typeid(SyntaxNodeImpl<FixedArray<Token, 2> >).name(), sizeof(SyntaxNodeImpl<FixedArray<Token, 2> >));
        return new T;
    }

    static struct RecognitionSeq
    {
        SimpleFactory m_factory;
        RecognitionPatternImpl m_pattern;
        
        bool match (const RecognitionPattern& tokens, bool& partial)
        {
            assert(!tokens.empty());

            RecognitionPattern::const_iterator tIt = tokens.begin(); 
            RecognitionPatternImpl::const_iterator pIt = m_pattern.begin();

            for (; tIt != tokens.end() && pIt != m_pattern.end(); ++tIt, ++pIt)
                if (*tIt != *pIt)
                    return false;

            // match or partial match 
            if (tIt == tokens.end())
            {
                if (pIt != m_pattern.end() && *pIt == etNONE)
                    return true;

                partial = true;
            }

            // (pIt == m_pattern.end()) is never going to happen because the pattern is always closing by etNONE

            return false;
        }
    }
    g_recognitionSeq[] =
    {
        { SimpleFactoryImpl<BlockNode>,     etLESS,    etLESS, etNONE },
        { SimpleFactoryImpl<BlockNode>,     etDECLARE, etNONE },
        { SimpleFactoryImpl<BlockNode>,     etBEGIN,   etNONE },
        { SimpleFactoryImpl<ProcedureNode>, etCREATE, etPROCEDURE, etNONE },
        { SimpleFactoryImpl<FunctionNode>,  etCREATE, etFUNCTION,  etNONE },
        { SimpleFactoryImpl<PackageNode>,   etCREATE, etPACKAGE,   etNONE },
        { SimpleFactoryImpl<TriggerNode>,   etCREATE, etTRIGGER,   etNONE },
        { SimpleFactoryImpl<ProcedureNode>, etCREATE, etOR, etREPLACE, etPROCEDURE, etNONE },
        { SimpleFactoryImpl<FunctionNode>,  etCREATE, etOR, etREPLACE, etFUNCTION,  etNONE },
        { SimpleFactoryImpl<PackageNode>,   etCREATE, etOR, etREPLACE, etPACKAGE,   etNONE },
        { SimpleFactoryImpl<TriggerNode>,   etCREATE, etOR, etREPLACE, etTRIGGER,   etNONE },

        { SimpleFactoryImpl<TypeBodyNode>,  etCREATE, etTYPE, etBODY, etNONE },
        { SimpleFactoryImpl<TypeBodyNode>,  etCREATE, etOR, etREPLACE, etTYPE, etBODY, etNONE },
        
        { SimpleFactoryImpl<JavaNode>, etCREATE, etJAVA, etNONE                                       },
        { SimpleFactoryImpl<JavaNode>, etCREATE, etOR,   etREPLACE, etJAVA, etNONE                    },
        { SimpleFactoryImpl<JavaNode>, etCREATE, etAND,  etCOMPILE, etJAVA, etNONE                    },
        { SimpleFactoryImpl<JavaNode>, etCREATE, etOR,   etREPLACE, etAND,  etCOMPILE, etJAVA, etNONE },
        
        { SimpleFactoryImpl<SelectNode>,    etSELECT,   etNONE },
        { SimpleFactoryImpl<SelectNode>,    etWITH,     etNONE },
        { SimpleFactoryImpl<InsertNode>,    etINSERT,   etNONE },
        { SimpleFactoryImpl<UpdateNode>,    etUPDATE,   etNONE },
        { SimpleFactoryImpl<DeleteNode>,    etDELETE,   etNONE },

        // it mignt be either sql statement or sql*plus command
        { SimpleFactoryImpl<GenericSetNode>, etSET,  etNONE },

        { SimpleFactoryImpl<SqlPlusCmdNode>, etAT,        etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etACCEPT,    etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etARCHIVE,   etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etAPPEND,    etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etATTRIBUTE, etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etBREAK,     etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etBTITLE,    etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etCLEAR,     etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etCOLUMN,    etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etCOMPUTE,   etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etCONNECT,   etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etCOPY,      etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etDEFINE,    etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etDESCRIBE,  etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etDISCONNECT,etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etEXECUTE,   etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etEXIT,      etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etHELP,      etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etHOST,      etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etPAUSE,     etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etPRINT,     etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etPROMPT,    etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etRECOVER,   etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etREMARK,    etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etREPFOOTER, etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etREPHEADER, etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etRUN,       etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etSHOW,      etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etSHUTDOWN,  etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etSPOOL,     etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etSTART,     etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etSTARTUP,   etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etTIMING,    etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etTTITLE,    etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etUNDEFINE,  etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etVARIABLE,  etNONE },
        { SimpleFactoryImpl<SqlPlusCmdNode>, etWHENEVER,  etNONE },
    };

void ScriptFactory::RecognizeAndCreate (const RecognitionPattern& tokens, std::auto_ptr<SyntaxNode>& child)
{
    bool partial = false;

    for (int i = 0; i < sizeof(g_recognitionSeq)/sizeof(g_recognitionSeq[0]); i++)
    {
        if (g_recognitionSeq[i].match(tokens, partial))
        {
            child.reset(g_recognitionSeq[i].m_factory());
            break;
        }
    }

    if (!child.get() && !partial)
        child.reset(new GenericSqlNode);

    if (child.get())
        feed(tokens, *child);
}

void ScriptFactory::Create (const RecognitionPattern& tokens, std::auto_ptr<SyntaxNode>& child)
{
    child.reset(new GenericSqlNode);
    feed(tokens, *child);
}

void ScriptFactory::feed (const RecognitionPattern& tokens, SyntaxNode& child)
{
    RecognitionPattern::const_iterator it = tokens.begin();

    std::auto_ptr<SyntaxNode> dummy;

    for (; it != tokens.end() ; ++it)
    {
        child.PutToken(*it, dummy);
       // ASSERT(!dummy.get());
    }
}


void DmlSqlFactory::RecognizeAndCreate (const Token& token, std::auto_ptr<SyntaxNode>& child)
{
	switch (token)
	{
		case etCASE:
			child.reset(new SqlCaseNode);
			break;
		case etLEFT_ROUND_BRACKET:
			child.reset(new ExpressionNode);
			break;
		case etSELECT:
			child.reset(new SelectNode);
			break;
	}
    
    if (child.get())
    {
        std::auto_ptr<SyntaxNode> dummy;
        child->PutToken(token, dummy);
        ASSERT(!dummy.get());
    }
}

void PlSqlFactory::RecognizeAndCreate (const Token& token, std::auto_ptr<SyntaxNode>& child)
{
    STACK_OVERFLOW_GUARD(3);

	switch (token)
	{
		case etDECLARE:
		case etBEGIN:
			child.reset(new BlockNode);
			break;
		case etFUNCTION:
			child.reset(new FunctionNode);
			break;
		case etPROCEDURE:
			child.reset(new ProcedureNode);
			break;
		case etIF:
			child.reset(new SelectionNode);
			break;
		case etCASE:
			child.reset(new CaseNode);
			break;
		case etFOR:
		case etWHILE:
		case etLOOP:
			child.reset(new IterationNode);
			break;
		case etLEFT_ROUND_BRACKET:
			child.reset(new ExpressionNode);
			break;
		case etOPEN:
			child.reset(new OpenCursorNode);
			break;
		case etWITH:
		case etSELECT:
			child.reset(new SelectNode);
			break;
		case etINSERT:
			child.reset(new InsertNode);
			break;
		case etUPDATE:
			child.reset(new UpdateNode);
			break;
		case etDELETE:
			child.reset(new DeleteNode);
			break;
            //new SqlSttmNode;    
			//break;
	}

    if (child.get())
    {
        std::auto_ptr<SyntaxNode> dummy;
        child->PutToken(token, dummy);
        ASSERT(!dummy.get());
    }
}

}; // namespace OpenEditor