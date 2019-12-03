/* 
    Copyright (C) 2003-2015 Aleksey Kochetov

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

#include "Common/FixedArray.h"
#include "OpenEditor/OEPlsSqlSyntax.h"

namespace OpenEditor
{ 
    using Common::FixedArray;
    
    template <class TokenArray = FixedArray<Token, 10> >
    class SyntaxNodeImpl : public SyntaxNode
    {
    protected:
        SyntaxNodeImpl () : m_state(0) {}

    public:
        virtual bool WantsEOL () const { return false; }
        virtual bool FindToken (int line, int offset, const SyntaxNode*& node, int& index) const;
        virtual void GetToken (int index, int& line, int& offset, int& length) const;
        virtual void GetNextToken (int index, int& line, int& offset, int& length, bool& backward) const;
        virtual void GetLineStatus (LineStatusMap&, int& recursion) const;
        virtual void Colapse (const Token&);

    protected:
        void append (int state, const Token& token) { m_state = state; m_tokens.push_back(token); }
        virtual void clear () { SyntaxNode::clear(); m_tokens.clear(); }
        virtual EHitCode GetHitCode (int line, int count) const;
        virtual void DbgPrintNode () const;

        int m_state;
    //private:
        TokenArray m_tokens;
    };

    // TODO: make sure that we really need ScriptNode
    class ScriptNode : public SyntaxNodeImpl<FixedArray<Token, 7> >
    {
    public:
        ~ScriptNode ();
        virtual bool IsScriptNode () const { return true; };

        void Clear ();

        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return false; }
        virtual bool FindToken (int line, int offset, const SyntaxNode*& node, int& index) const;
        virtual void GetLineStatus (LineStatusMap&, int& recursion) const;
    };

    class BlockNode : public SyntaxNodeImpl<FixedArray<Token, 5> >
    {
        enum State { eUndef = 0, eDeclare, eBegin, eException, eEnd, eComplete };

    public:
        virtual bool IsScriptNode () const { return false; };
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };

    class PackageNode : public SyntaxNodeImpl<FixedArray<Token, 12> >
    {
        enum State { eUndef = 0, eCreate, eOr, eReplace, 
            ePackage, eWrapped, eAuthid1, eAuthid2, eBody, eName1, eNameSeparator, eName2, eIs, 
            eBegin, eException, eEnd, eComplete 
        };

    public:
        virtual bool IsScriptNode () const { return false; };
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };

    class TypeBodyNode : public SyntaxNodeImpl<FixedArray<Token, 12> >
    {
        enum State { eUndef = 0, eCreate, eOr, eReplace, 
            eType, eBody, eName1, eNameSeparator, eName2, eIs, 
            eBegin, eException, eEnd, eComplete 
        };

    public:
        virtual bool IsScriptNode () const { return false; };
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };

    class ProcedureNode : public SyntaxNodeImpl<FixedArray<Token, 10> >
    {
        enum State { eUndef = 0, eCreate, eOr, eReplace, 
            eProcedure, eName1, eNameSeparator, eName2, eArguments, eIs, 
            eBegin, eException, eEnd, eComplete, eLanguage 
        };

    public:
        virtual bool IsScriptNode () const { return false; };
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };

    class FunctionNode : public SyntaxNodeImpl<FixedArray<Token, 10> >
    {
        enum State { eUndef = 0, eCreate, eOr, eReplace, 
            eFunction, eName1, eNameSeparator, eName2, eArguments, eReturn, eIs, 
            eBegin, eException, eEnd, eComplete, eLanguage 
        };

    public:
        virtual bool IsScriptNode () const { return false; };
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };

    class JavaNode : public SyntaxNodeImpl<FixedArray<Token, 7> >
    {
        enum State { eUndef = 0, eCreate, eOr, eReplace, eAnd, eCompile, eJava, eComplete };

    public:
        virtual bool IsScriptNode () const { return false; };
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };

    class TriggerNode : public SyntaxNodeImpl<FixedArray<Token, 10> >
    {
        enum State { eUndef = 0, eCreate, eOr, eReplace, 
            eTrigger, eName1, eNameSeparator, eName2, eHeader, eDeclare, 
            eBegin, eException, eEnd, eComplete
        };

    public:
        virtual bool IsScriptNode () const { return false; };
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };

    class SelectionNode : public SyntaxNodeImpl<std::vector<Token> >
    {
        enum State { eUndef = 0, eIf, eElsIf, eElse, eEnd, eEndIf, eComplete };

    public:
        virtual bool IsScriptNode () const { return false; };
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };

    class CaseNode : public SyntaxNodeImpl<std::vector<Token> >
    {
        enum State { eUndef = 0, eCase, eWhen, eElse, eEnd, eEndCase, eComplete };

    public:
        virtual bool IsScriptNode () const { return false; };
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };

    class SqlCaseNode : public SyntaxNodeImpl<std::vector<Token> >
    {
        enum State { eUndef = 0, eCase, eWhen, eElse, eEnd, eComplete };

    public:
        virtual bool IsScriptNode () const { return false; };
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };

    class IterationNode : public SyntaxNodeImpl<FixedArray<Token, 5> >
    {
        enum State { eUndef = 0, eFor, eWhile, eLoop, eEnd, eEndLoop, eComplete };

    public:
        virtual bool IsScriptNode () const { return false; };
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };


    class ExpressionNode : public SyntaxNodeImpl<FixedArray<Token, 5> >
    {
        enum State { eUndef = 0, eOpen, eComplete };

    public:
        virtual bool IsScriptNode () const { return false; };
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };

    class SelectNode : public SyntaxNodeImpl<std::vector<Token> >
    {
        enum State { eUndef = 0, eWith, eSelect, eFrom, eWhere, eUnion, eIntersect, eMinus, eNextPosibleSet, eComplete };

    public:
        virtual bool IsScriptNode () const { return false; };
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
        virtual void OnChildCompletion (SyntaxNode* /*child*/, const Token&);
    };

    class InsertNode : public SyntaxNodeImpl<FixedArray<Token, 5> >
    {
        enum State { eUndef = 0, eInsert, eComplete };

    public:
        virtual bool IsScriptNode () const { return false; };
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };

    class UpdateNode : public SyntaxNodeImpl<FixedArray<Token, 5> >
    {
        enum State { eUndef = 0, eUpdate, eSet, eWhere, eComplete };

    public:
        virtual bool IsScriptNode () const { return false; };
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };

    class DeleteNode : public SyntaxNodeImpl<FixedArray<Token, 5> >
    {
        enum State { eUndef = 0, eDelete, eWhere, eComplete };

    public:
        virtual bool IsScriptNode () const { return false; };
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };

    class OpenCursorNode : public SyntaxNodeImpl<FixedArray<Token, 5> >
    {
        enum State { eUndef = 0, eOpen, eFor, eComplete };

    public:
        virtual bool IsScriptNode () const { return false; };
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };

    class SqlPlusCmdNode : public SyntaxNodeImpl<FixedArray<Token, 2> >
    {
        enum State { eUndef = 0, ePending, eComplete };

    public:
        virtual bool IsScriptNode () const { return true; };
        virtual bool WantsEOL () const { return true; }
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };

    class GenericSqlNode : public SyntaxNodeImpl<FixedArray<Token, 2> >
    {
        enum State { eUndef = 0, ePending, eGrant, eRevoke, eComplete };

    public:
        virtual bool IsScriptNode () const { return false; };
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };

    // there are a couple of sql statement and a bunch of sql*plus commands starting with "set"
    // but I'm afraid that oracle might inroduce a new sql statement
    // and prefer unknown syntax to be recognized rather as sql than command
    class GenericSetNode : public SyntaxNodeImpl<FixedArray<Token, 3> >
    {
        enum State { eUndef = 0, eSet, ePendingSql, ePendingSqlPlusCmd, eComplete };

    public:
        virtual bool IsScriptNode () const { return true; };
        virtual bool WantsEOL () const { return true; }
        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&);
        virtual bool IsCompleted () const { return m_state == eComplete; }
    };
};
