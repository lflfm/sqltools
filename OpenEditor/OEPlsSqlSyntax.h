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

#pragma once

#include <set>
#include "OpenEditor/OEHelpers.h"
#include "OpenEditor/OEPlsSqlParser.h"

namespace OpenEditor
{ 
    using std::string;
    using std::set;
    using namespace Common::PlsSql;

    class EditContext;
    class ScriptNode; 

    //
    // Attention: a tree builder is responsinle for destroing all nodes!
    //
    class SyntaxNode : private noncopyable
    {
    public:
        SyntaxNode ();
        virtual ~SyntaxNode () {};

        virtual bool IsScriptNode () const = 0;

        virtual void PutToken (const Token&, std::auto_ptr<SyntaxNode>&) = 0;
        virtual void OnChildCompletion (SyntaxNode* /*child*/, const Token&) {}

        virtual bool WantsEOL    () const = 0;
        virtual bool IsCompleted () const = 0;

        int GetLevel () const;
        virtual bool StopSign (EToken token) const;

        virtual bool FindToken (int line, int offset, const SyntaxNode*& node, int& index) const = 0;
        virtual void GetToken (int index, int& line, int& offset, int& length) const = 0;
        virtual void GetNextToken (int index, int& line, int& offset, int& length, bool& backward) const = 0;
        virtual void GetLineStatus (LineStatusMap&, int& recursion) const = 0;
        virtual void DbgPrintNode () const = 0;

        void AttachSibling (SyntaxNode*);   
        void AttachChild   (SyntaxNode*);  

        SyntaxNode* GetSibling ()                   { return m_sibling; }
        SyntaxNode* GetParent ()                    { return m_parent; }
        SyntaxNode* GetChild ()                     { return m_child; }

        const SyntaxNode* GetSibling () const       { return m_sibling; }
        const SyntaxNode* GetParent () const        { return m_parent; }
        const SyntaxNode* GetChild () const         { return m_child; }

        void Failure (int line)                     { if (!m_failure) m_failureLine = line, m_failure = true; }
        bool IsFailed () const                      { return m_failure; }
        int  GeFailureLine () const                 { return m_failureLine; }
        virtual void Colapse (const Token&) = 0;

        enum EHitCode { UNKNOWN, ABOVE, HIT, BELOW };
        virtual EHitCode GetHitCode (int line, int count) const = 0;

    protected:
        void clear ();
        static const int RECURSION_LIMIT = 1000;

    private:
        bool m_failure;
        int m_failureLine;
        SyntaxNode *m_sibling, *m_parent, *m_child;
        SyntaxNode *m_lastChild;
    };

    class PlSqlAnalyzer : public SyntaxAnalyser, private noncopyable
    {
    public:
        PlSqlAnalyzer ();
        ~PlSqlAnalyzer ();

        void PutToken (const Token&);

        bool FindToken (int line, int offset, const SyntaxNode*& node, int& index) const;
        void GetLineStatus (LineStatusMap&) const;
        void Clear ();

    private:
        std::auto_ptr<ScriptNode> m_root;
        SyntaxNode* m_top;
        vector<SyntaxNode*> m_pool;

        bool m_error;

        void Attach   (SyntaxNode*);
        void CloseTop (const Token&);
        void CloseAll (const Token&) ;
    };

}; // namespace OpenEditor
