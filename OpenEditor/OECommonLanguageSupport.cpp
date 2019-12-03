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
    07.03.2003 improvement, match searcher mor C++, Jave and ... skips strings, charater and comments
    2011.10.02 improvement, FindMatch does not work for TXT files because no lenguage defined. Now C++ is used as default.
    2011.12.25 bug fix, Find Brace (Ctrl+[) causes an exception under some conditions
*/

#include "stdafx.h"
#include <stack>
#include <algorithm>
#include <COMMON/Fastmap.h>
#include "COMMON/StrHelpers.h"
#include <COMMON/ExceptionHelper.h>
#include "OpenEditor/OECommonLanguageSupport.h"
#include "OpenEditor/OELanguage.h"
#include "OpenEditor/OEStorage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace OpenEditor
{
    using namespace std;
    using namespace Common;

    static
    bool exist_any (set<int> s1, set<int> s2)
    {
        for (set<int>::const_iterator it1 = s1.begin(); it1 != s1.end(); ++it1)
        {
            set<int>::const_iterator it2 = s2.find(*it1);
            if (it2 != s2.end())
                return true;
        }
        
        return false;
    }

    struct MatchToken 
    {
        string keyword;
        int orientation;
        set<int> lexemes;

        MatchToken () { orientation = 0; }
        MatchToken (const string& str, int ornt) : keyword(str), orientation(ornt) {}
    };

    struct MatchTokenPos : MatchToken
    {
        int position;

        MatchTokenPos () { position = 0; }
        MatchTokenPos (const MatchToken& tkn, int pos) : MatchToken(tkn), position(pos) {}
    };

    struct MatchEntry
    {
        int balance;
        set<int> lexemes;

        MatchEntry (int bal) : balance(bal) {}
        MatchEntry (const MatchToken& tkn) : lexemes(tkn.lexemes), balance(tkn.orientation) {}

        bool IsOrientationSame (const MatchToken tkn) const { return (balance > 0 && tkn.orientation > 0) || (balance < 0 && tkn.orientation < 0); }
    };

    struct CommonLanguageSupport::Context
    {
        bool backward;
        bool casesensitive;
        Fastmap<bool> fastmap;
        map<string, MatchToken> fullmap;

        stack<MatchEntry> matchStack;
        vector<MatchTokenPos> tokens;

        void init (const LanguagePtr&);
    };

    void CommonLanguageSupport::Context::init (const LanguagePtr& lang)
    {
        backward = false;
        casesensitive = lang->IsCaseSensitive();

        const vector<vector<string> >& braces = lang->GetMatchBraces();

        for (unsigned i = 0; i < braces.size(); i++)
        {
            const vector<string>& braceSet = braces[i];

            for (unsigned j = 0; j < braceSet.size(); j++)
            {
                string brace;
                
                if (casesensitive)
                {
                    brace = braceSet[j];
                    fastmap[brace[0]] = 1;
                }
                else
                {
                    to_upper_str(braceSet[j].c_str(), brace);
                    fastmap[toupper(brace[0])] = true;
                    fastmap[tolower(brace[0])] = true;
                }

                std::map<string, MatchToken>::iterator it = fullmap.find(brace);
                if (it == fullmap.end())
                {
                    int orientation = (j == 0) ? 1 : ((j == braceSet.size()-1) ? -1 : 0);
                    fullmap.insert(
                        std::map<string, MatchToken>::value_type(
                                brace, MatchToken(brace, orientation) 
                            )
                        );
                    it = fullmap.find(brace);
                }
                it->second.lexemes.insert(i);
            }
        }
    }

CommonLanguageSupport::CommonLanguageSupport (Storage* pstorage) 
    : LanguageSupport(pstorage), 
    m_cxt(new Context)
{
    string language = m_pStorage->GetSettings().GetLanguage();
    
    if (language == "NONE")
        language = "C++"; // we need some language and why not C++ 

    const LanguagePtr langPtr = LanguagesCollection::Find(language);

    m_cxt->init(langPtr);
}

CommonLanguageSupport::~CommonLanguageSupport () 
{ 
    delete m_cxt; 
}

bool CommonLanguageSupport::FindMatch (int _line, int _offset, Match& match, bool quickExactMatch /*= false*/)
{
    _CHECK_AND_THROW_(m_pStorage, "CommonLanguageSupport has not been initialized!");

    Context& cxt = *m_cxt;
    cxt.backward = false;
    cxt.tokens.clear();
    cxt.matchStack = stack<MatchEntry>();

    cxt.matchStack.push(MatchEntry(1)); // search for any closing brace

    match.found    = false;
    match.partial  = true;
    match.backward = false;
    match.broken   = false;
    match.line[0]   = _line;
    match.offset[0] = _offset;
    match.length[0] = 0;

    {
        readTokens(_line);

        vector<MatchTokenPos>::const_iterator it = cxt.tokens.begin();
        for (; it != cxt.tokens.end(); ++it)
        {
            if (it->position <= _offset 
            && _offset < it->position + static_cast<int>(it->keyword.length()))
            {
                _offset = it->position;
                cxt.matchStack.pop(); // empty stack - search for the current lexeme
                cxt.backward = it->orientation == -1 ? true : false;

                match.backward  = cxt.backward;
                match.partial   = false;
                match.offset[0] = it->position;
                match.length[0] = it->keyword.length();
                break;
            }
        }
    }

    if (quickExactMatch && match.partial)
        return false;

    int offset = _offset;
    int nlines = m_pStorage->GetLineCount();
    for (int line = _line; !match.backward ? line < nlines : line >= 0; !match.backward ? line++ : line--)
    {
        readTokens(line);

        vector<MatchTokenPos>::const_iterator it = cxt.tokens.begin();
        for (; it != cxt.tokens.end(); ++it)
        {
            if (!match.backward ? (it->position >= offset) : (it->position <= offset))
            {
                if (cxt.matchStack.empty()
                || cxt.matchStack.top().IsOrientationSame(*it))
                {
                    cxt.matchStack.push(*it);
                    
                    if (cxt.matchStack.size() == 1         // it's a top 
                    && cxt.matchStack.top().balance == 0)  // if it's optional/middle brace/keyword
                        cxt.matchStack.top().balance = 1;  // then assume that it's a start point
                }
                else
                {
                    MatchEntry& top = cxt.matchStack.top();

                    if (top.lexemes.empty() // search for any
                    || exist_any(top.lexemes, it->lexemes))
                    {
                        if (!match.partial              // known search
                        && !match.backward              // do it if it's forward search
                        && it->orientation == 0         // it's an optional/middle brace/keyword
                        && cxt.matchStack.size() == 1)  // it's a top 
                        {
                            top.balance = 0;
                        }
                        else
                            top.balance += it->orientation;
                        
                        if (!top.balance)
                            cxt.matchStack.pop();
                    }
                    else
                        match.broken = true;

                    if (match.broken || cxt.matchStack.empty()) 
                    {
                        match.found = true;
                        match.line  [1] = line;
                        match.length[1] = it->keyword.length();
                        match.offset[1] = it->position;
                        break;
                    }
                }
            }
        }

        if (match.found) 
            break;

        offset = !match.backward ? 0 : INT_MAX;
    }

    if (!match.partial && !match.found)
        match.broken = true;

    return match.found;
}

void CommonLanguageSupport::readTokens (int line)
{
    Context& cxt = *m_cxt;
    MultilineQuotesScanner& scanner = m_pStorage->GetMultilineQuotesScanner();
    LineTokenizer tokenizer(false, 1, scanner.GetDelimitersMap());

    int lineLength;
    const char* lineStr;
    m_pStorage->GetLine(line, lineStr, lineLength);
    tokenizer.StartScan(lineStr, lineLength);
    cxt.tokens.clear();

    while (!tokenizer.Eol())
    {
        int pos, len;
        const char* str;
        tokenizer.GetCurentWord(str, pos, len);
        
        if (len > 0 && cxt.fastmap[*str])
        {
            string brace(str, len);
            
            if (!cxt.casesensitive)
                to_upper_str(brace.c_str(), brace);

            map<string, MatchToken>::const_iterator it = cxt.fullmap.find(brace);
            
            if (it != cxt.fullmap.end())
            {
                // probe the position status
                bool parsing = false; // ignore it
                int state = 0, quoteId = 0, parsingLength;
                scanner.Scan(line, state, quoteId, parsing);
                scanner.ScanLine(lineStr, pos, state, quoteId, parsing, &parsingLength);
                // if it's code then change balance
                if (!state && pos <= parsingLength)
                    cxt.tokens.push_back(MatchTokenPos(it->second, pos));
            }
        }
        tokenizer.Next();
    }
    if (cxt.backward)
        reverse(cxt.tokens.begin(), cxt.tokens.end());
}

};
