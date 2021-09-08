/* 
    Copyright (C) 2018 Aleksey Kochetov

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

#include "stdafx.h"
#include "OpenEditor/OEView.h"
#include <COMMON/AppGlobal.h>
#include "OpenEditor/OEPlsSqlParser.h"
#include "SQLWorksheet/PlsSqlParser.h"
#include <stack>
//#include <regex>
//std::regex pattern("[[:space:]]+as[[:space:]]+(sysdba|sysoper)[[:space:]]*$", std::regex_constants::icase|std::regex_constants::extended);



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using namespace std;
    using namespace OpenEditor;

    // convert to inline!
#define RETURN_IF_LOCKED { if (!m_bAttached) return; if (IsLocked()) { Global::SetStatusText("The content is locked and cannot be modified!"); MessageBeep((UINT)-1); return; } } 

/*
    !!! use special collection of tokens as intermediate result
    when 1st pass is done do column alignment for column and table aliases

    +add a stack of indents (block and special)
    +    ( starts block indent
    +    "SELECT/WHERE" - special indent
    +    it is going to be rather indent context

    *when SELECT is found look for the next known keyword FROM if distance is small
    *then format in line if long then in column
    Same for case
    if list of table after FROM or list of columns after BY are shor, don't add OEL and make single line

    ?add rules to control breaks (non-breakable pairs of tokens like ":=", "anything.anything", "(+)"...

    *pre-scan for table list and column list in order to choose short/long mode, 
    *  use existing formatting (more like if it is already multiline then long)

    +CASE, nested CASE
    +multiline comments
    +align aliases with accounting length of non-aliased lines as well
    always multiline comments (n > 1) with new line
    start multiline comments with new line, if length is longer than (n)
    end line comment on new line if it is longer than (n)
    add option to jon end line comment to the previous line (can be implemented as in post-processing)
    BETWEEN AND
    alignList does not hanle -1 properly (I guess any number or expression might cause the same issue)

    Some posible ideas:
    introduce popSpecialIndent( to FROM or to WHERE);
    add void* to Token and use it to save IndentCtx*
    incorparate COEditorView::PreNormalizeOnChar
*/

    enum IndentType {
        eitRoot,
        eitBlock,
        eitSpecial,
    };

    struct IndentCtx
    {
        IndentCtx (IndentType type, int size =  2) 
            : type(type),
            size(size), 
            bookmark(-1),
            spacer(size, ' ') 
        {}

        IndentCtx (IndentType type, const IndentCtx& prev, int size =  2) 
            : type(type),
            size(prev.size + size), 
            bookmark(-1),
            spacer(prev.spacer.size() + size, ' ')
        {}

        IndentType type;
        int size;
        int bookmark;
        string spacer;
    };

    struct IndentCtxStack : stack<IndentCtx>
    {
        IndentCtxStack (int size)
            : indentSize(size),
            currentLineTokenCounter(0)
        {
            push(IndentCtx (eitRoot, 0));
        }

        void pushSpecialIndent (bool resetCounter = true)
        {
            push(IndentCtx(eitSpecial, top(), indentSize));

            if (resetCounter)
                resetCurrentLineTokenCounter();
        }

        void popSpecialIndent (bool resetCounter = true)
        {
            if (top().type == eitSpecial)
              pop();

            if (resetCounter)
                resetCurrentLineTokenCounter();
        }

        void pushBlockIndent ()
        {
            push(IndentCtx(eitBlock, top(), indentSize));
            resetCurrentLineTokenCounter();
        }

        void popBlockIndent ()
        {
            while (top().type == eitSpecial)
              pop();

            if (top().type == eitBlock)
              pop();

            resetCurrentLineTokenCounter();
        }

        int  getCurrentLineTokenCounter   () const { return currentLineTokenCounter; }
        void incrementCurrentLineTokenCounter ()   { ++currentLineTokenCounter;      }
        void resetCurrentLineTokenCounter ()       { currentLineTokenCounter = 0;    }

        const string& getSpacer () const { return top().spacer; }

        int indentSize;
        int currentLineTokenCounter;
    };

    static int block_distance (std::vector<OpenEditor::Token>::const_iterator begin, std::vector<OpenEditor::Token>::const_iterator end)
    {
        int distance = 0, level = 0;
        std::vector<OpenEditor::Token>::const_iterator it = begin + 1;
        for (; it != end; ++it)
        {
            if (it->token == Common::PlsSql::etRIGHT_ROUND_BRACKET)
                --level;
            else if (it->token == Common::PlsSql::etLEFT_ROUND_BRACKET)
                ++level;

            if (level < 0)
                return distance;

            distance++;
        }
        return distance;
    }

    //static int from_distance (std::vector<OpenEditor::Token>::const_iterator begin, std::vector<OpenEditor::Token>::const_iterator end)
    //{
    //    int ncommas = 0, blocks = 0, level = 0;
    //    std::vector<OpenEditor::Token>::const_iterator it = begin + 1;
    //    for (; it != end; ++it)
    //    {
    //        switch (it->token)
    //        {
    //        case Common::PlsSql::etCOMMA: 
    //            if (!level) ++ncommas;
    //            break;
    //        case Common::PlsSql::etLEFT_ROUND_BRACKET:
    //            ++level, ++blocks;
    //            break;
    //        case Common::PlsSql::etRIGHT_ROUND_BRACKET:
    //            --level;
    //            break;
    //        case Common::PlsSql::etSEMICOLON:
    //        case Common::PlsSql::etWHERE:
    //        case Common::PlsSql::etGROUP:
    //        case Common::PlsSql::etORDER:
    //            if (!level)
    //                level = -1;
    //        }

    //        if (level < 0)
    //           break;
    //    }
    //    return !blocks ? ncommas : INT_MAX;
    //}

    struct OutputTokenStream
    {
        std::vector<OpenEditor::Token> tokens;
        
        void  putSpace (const string& val = string(1, ' '))
        {
            OpenEditor::Token tk;
            tk.token = etWHITESPACE;
            tk.value = val;
            tk.length = static_cast<OpenEditor::Token::size_pos>(tk.value.length());
            tokens.push_back(tk);
        }
        
        void  putEol ()
        {
            OpenEditor::Token tk;
            tk.token = Common::PlsSql::etEOL;
            tk.value = '\n';
            tk.length = static_cast<OpenEditor::Token::size_pos>(tk.value.length());
            tokens.push_back(tk);
        }
        
        void  putToken (const OpenEditor::Token& tk, const char* str, int len)
        {
            tokens.push_back(tk);
            tokens.back().value = string(str, len);
        }

        void flush (std::ostream& out)
        {
            for (auto it = tokens.begin(); it != tokens.end(); ++it)
                out << it->value;
        }

        int position () const { return tokens.size(); }

        /*
        select
           col,
           col col_alias,
           col "col alias",
           col as col_alias,
           col as "col alias",
           tab.col,
           func(col),
           tab.col col_alias,
           func(col) col_alias,
           tab.col as col_alias,
           func(col) as col_alias,
           (select 1 from dual) col_alias
           (select 1, 2, 3, 4, 5, 6, 7, 8, 9 from dual) col_alias
        from dual
        */
        void  alignList (unsigned int from, unsigned int indent)
        {
            if (from >= 0 && from < tokens.size())
            {
                // 1st pass - looking for aliases
                std::vector<int> aliasTokenPos;
                bool haveAs = false;
                {
                    auto it = tokens.begin() + from;

                    for (; it != tokens.end(); ++it)
                    {
                        string buffer;
                        std::vector<OpenEditor::Token*> lineTokens;

                        for (; it != tokens.end() && it->token != Common::PlsSql::etEOL; ++it)
                        {
                            buffer += it->value;
                            lineTokens.push_back(&*it);
                        }

                        auto rAsIt = lineTokens.rend();

                        if (!lineTokens.empty()
                        &&  (
                                (
                                    ((*lineTokens.begin())->token == etWHITESPACE) 
                                    && ((*lineTokens.begin())->value.size() == indent)
                                    && (it == tokens.end() || (*lineTokens.rbegin())->token == Common::PlsSql::etCOMMA)
                                )
                                || !aliasTokenPos.size()
                            )
                        )
                        {
                            // looking for (indent)(anything)(space)[(as)(space)](identifier)[,] in backward
                            auto rIt = lineTokens.rbegin();

                            if (rIt != lineTokens.rend() && **rIt == Common::PlsSql::etCOMMENT) // skip comment
                                rIt++;
                            if (rIt != lineTokens.rend() && **rIt == Common::PlsSql::etCOMMA) // skip comma
                                rIt++;

                            if (rIt != lineTokens.rend() 
                            && **rIt != Common::PlsSql::etWHITESPACE
                            && **rIt != Common::PlsSql::etDOT
                            ) 
                            {
                                rIt++;
                                if (rIt != lineTokens.rend() 
                                && **rIt == Common::PlsSql::etWHITESPACE)
                                {
                                    rAsIt = rIt; // possibly found
                                    rIt++;

                                    if (rIt == lineTokens.rend()  // end of line so no match
                                    || **rIt == Common::PlsSql::etSELECT  // in case of the first line
                                    )
                                    {
                                        rAsIt = lineTokens.rend();
                                    }
                                    else if (**rIt == Common::PlsSql::etAS)
                                    {
                                        rIt++;
                                        if (rIt != lineTokens.rend() 
                                        || **rIt == Common::PlsSql::etWHITESPACE)
                                        {
                                            rAsIt = rIt;
                                            haveAs = true;
                                        }
                                    }
                                    else
                                        ; // do nothing because it is already found
                                }
                            }
                        }

                        if (!lineTokens.empty() && rAsIt != lineTokens.rend())
                        {
                            int inx = lineTokens.size() - 1 - (rAsIt - lineTokens.rbegin());
                            aliasTokenPos.push_back(inx);
                        }
                        else
                            aliasTokenPos.push_back(-1);

                        if (it == tokens.end())
                            break;
                    }
                }
                // 2d pass - calculating max distance to alias
                unsigned int maxDistance = 0;
                {
                    auto it = tokens.begin() + from;
                    for (int line = 0; it != tokens.end(); ++it, ++line)
                    {
                        string buffer;
                        bool hasAlias = false;
                        for (int pos = 0; it != tokens.end() && it->token != Common::PlsSql::etEOL; ++it, ++pos)
                        {
                            if (it->token != Common::PlsSql::etCOMMENT) // assuming it can be at EOL only!
                                buffer += it->value;

                            if (aliasTokenPos[line] == pos)
                            {
                                hasAlias = true;
                                maxDistance = std::max(maxDistance, buffer.length());
                            }
                        }
                        if (!hasAlias)
                            maxDistance = std::max(maxDistance, buffer.length());

                        if (it == tokens.end())
                            break;
                    }
                }
                // 3d pass - adjusting witespace to align aliases
                {
                    auto it = tokens.begin() + from;
                    for (int line = 0; it != tokens.end(); ++it, ++line)
                    {
                        string buffer;
                        for (int pos = 0; it != tokens.end() && it->token != Common::PlsSql::etEOL; ++it, ++pos)
                        {
                            buffer += it->value;

                            if (aliasTokenPos[line] == pos 
                            && maxDistance > buffer.length())
                            {
                                auto next = it;
                                if (!haveAs
                                || (++next != tokens.end() && next->token == Common::PlsSql::etAS)
                                )
                                    it->value += string(maxDistance - buffer.length(), ' ');
                                else
                                    it->value += string(maxDistance - buffer.length() + 3/*"AS "*/, ' ');
                            }
                        }

                        if (it == tokens.end())
                            break;
                    }
                }
            }
        }
    };

    static
    bool is_join_qualifier (Common::PlsSql::Token tk)
    {
        switch (tk.token)
        {
        case Common::PlsSql::etINNER:
        case Common::PlsSql::etLEFT:
        case Common::PlsSql::etRIGHT:
        case Common::PlsSql::etFULL:
        case Common::PlsSql::etNATURAL:
            return true;
        }
        return false;
    }

    static
    bool new_line_after (Common::PlsSql::Token tk)
    {
        //return true;

        if (tk.token == Common::PlsSql::etSELECT)
            return false;

        if (tk.token == Common::PlsSql::etFROM)
            return false;

        if (tk.token == Common::PlsSql::etWHERE)
            return false;

        if (tk.token == etJOIN)
            return false;

        if (tk.token == etBY)
            return false;

        return true;
    }

    static
    int find_eol_backward (OutputTokenStream str)
    {
        auto it = str.tokens.rbegin();
        for (; it != str.tokens.rend(); ++it)
            if (it->token == Common::PlsSql::etEOL)
                break;
        
        return it - str.tokens.rbegin();
    }

    static
    string align_comment (Common::PlsSql::Token tk, const string& spacer)
    {
        string buffer;

        if (tk.token == Common::PlsSql::etCOMMENT)
        {
            if (tk.value.find('\n') != string::npos)
            {
                string line;
                istringstream in(tk.value);

                while (getline(in, line))
                {
                    auto it = line.begin();

                    if (!buffer.empty()) // take the first line as is
                    {
                        buffer += '\n';
                        buffer += spacer;
                        for (int i = 0; it != line.end() && i < tk.offset && isspace(*it); ++it, ++i)
                            ;
                    }

                    buffer.append(it, line.end());
                }

            }
            else
                buffer = tk.value;
        }

        return buffer;
    }
    
void COEditorView::OnEditFormatSql ()
{
    RETURN_IF_LOCKED

    if (!IsSelectionEmpty())
    {
        OpenEditor::SimpleSyntaxAnalyser analyser;
        OpenEditor::PlSqlParser parser(analyser);

        OpenEditor::Square selection;
        GetSelection(selection);
        selection.normalize();

        // convert positions to indexes (because of tabs)
        selection.start.column = PosToInx(selection.start.line, selection.start.column);
        selection.end.column   = PosToInx(selection.end.line, selection.end.column);


        int line   = selection.start.line;
        int offset = selection.start.column;
        int nlines = GetLineCount();

        for (; line < nlines && line <= selection.end.line; line++)
        {
            int length;
            const char* str;
            GetLine(line, str, length);

            if (line == selection.end.line) 
                length = min(length, selection.end.column);
            
            parser.PutLine(line, str + offset, length - offset);
            offset = 0;
        }

        auto tokens = analyser.GetTokens();
        
        OutputTokenStream out;
        IndentCtxStack stack(2); // TODO: take from editor settings

        bool singleLine = false;
        OpenEditor::Token prevTk; 
        auto lastSingleLineToken = tokens.end();

        for (auto it = tokens.begin(); it != tokens.end(); ++it)
        {
            OpenEditor::Token tk = *it;

            int length;
            const char* str;
            GetLine(tk.line, str, length);

            if (tk.token != Common::PlsSql::etEOL)
            {
                if (!singleLine)
                {
                    if (tk.token == Common::PlsSql::etCOMMENT)
                    {
                        auto eof = tk.value.find('\n');
                        tk.token = Common::PlsSql::etCOMMENT;
                    }

                    /*
                    adding EOL and decreasing INDENT here
                    */
                    if (tk.token == Common::PlsSql::etRIGHT_ROUND_BRACKET)
                    {
                        stack.popBlockIndent();
                        out.putEol();
                    }
                    else if (tk.token == Common::PlsSql::etEND)
                    {
                        stack.popSpecialIndent(); // for WHEN/ELSE
                        stack.popSpecialIndent(); // for CASE itself
                        out.putEol();
                    }
                    else if (tk.token == Common::PlsSql::etFROM
                    || tk.token == Common::PlsSql::etWHERE
                    || (tk.token == etGROUP)
                    || (tk.token == etORDER)
                    || (tk.token == Common::PlsSql::etEND)
                    )
                    {
                        if (tk.token == Common::PlsSql::etFROM && stack.top().bookmark > -1)
                            out.alignList(stack.top().bookmark, stack.top().spacer.size());

                        stack.popSpecialIndent();
                        out.putEol();
                    }
                    else if (tk.token == Common::PlsSql::etJOIN 
                    || is_join_qualifier(tk)
                    )
                    {
                        if (!is_join_qualifier(prevTk))
                            stack.popSpecialIndent();
                        
                        if (!is_join_qualifier(prevTk) && tk.token == Common::PlsSql::etJOIN
                        || is_join_qualifier(tk))
                        {
                            stack.resetCurrentLineTokenCounter();
                            out.putEol();
                        }
                    }
                    else if (tk.token == Common::PlsSql::etWHEN
                    || tk.token == Common::PlsSql::etELSE
                    )
                    {
                        if (prevTk.token != Common::PlsSql::etCASE)
                            stack.popSpecialIndent(false);

                        stack.resetCurrentLineTokenCounter();
                        out.putEol();
                    }
                    else if (tk.token == Common::PlsSql::etAND
                    || tk.token == Common::PlsSql::etON
                    || tk.token == Common::PlsSql::etUSING
                    || prevTk.token == Common::PlsSql::etCOMMA
                    || prevTk.token == Common::PlsSql::etCOMMENT
                    || prevTk.token == Common::PlsSql::etSEMICOLON
                    || (prevTk.token == Common::PlsSql::etTHEN && tk.token == Common::PlsSql::etCASE)
                    || (prevTk.token == Common::PlsSql::etELSE && tk.token == Common::PlsSql::etCASE)
                    || (prevTk.token == Common::PlsSql::etSELECT && tk.token == Common::PlsSql::etCASE) // review when short case formatting will be implemented
                    || (tk.token == Common::PlsSql::etCOMMENT && tk.value.find('\n') != string::npos) // multiline comments
                    //comma in front || tk.token == Common::PlsSql::etCOMMA
                    )
                    {
                        stack.resetCurrentLineTokenCounter();
                        out.putEol();
                    }
                }

                if (stack.getCurrentLineTokenCounter() == 0)
                {
                    out.putSpace(stack.getSpacer());
                }
                else
                {
                    // non-breackable
                    if (prevTk.token != Common::PlsSql::etNONE
                    && prevTk.token != Common::PlsSql::etDOT 
                    && tk.token != Common::PlsSql::etDOT
                    && prevTk.token != Common::PlsSql::etLEFT_ROUND_BRACKET
                    && tk.token != Common::PlsSql::etLEFT_ROUND_BRACKET
                    && tk.token != Common::PlsSql::etRIGHT_ROUND_BRACKET
                    && prevTk.token != Common::PlsSql::etCOLON // for SQL only
                    && tk.token != Common::PlsSql::etCOMMA
                    && tk.token != Common::PlsSql::etSEMICOLON
                    )
                        out.putSpace();
                    else if (prevTk.reserved != 0 && tk.token == Common::PlsSql::etLEFT_ROUND_BRACKET)
                        out.putSpace();
                }

                if (tk.token == Common::PlsSql::etCOMMENT)
                {
                    string comment = align_comment(tk, stack.getSpacer());
                    out.putToken(tk, comment.c_str(), comment.size());
                }
                else
                    out.putToken(tk, str + tk.offset + (tk.line == selection.start.line ? selection.start.column : 0), tk.length);
                
                stack.incrementCurrentLineTokenCounter();

                if (!singleLine)
                {
                    /*
                    increasing INDENT here
                    */
                    if (tk.token == Common::PlsSql::etLEFT_ROUND_BRACKET)
                    {
                        // calculate distance and if it short then format this block in single line
                        // that means all following indents will be supressed - can it be done right here?
                        int distance = block_distance(it, tokens.end());
                        if (distance < 16)
                        {
                            lastSingleLineToken = it + distance + 1;
                            singleLine = true;
                        }
                        else
                        {
                            stack.pushBlockIndent();
                            out.putEol();
                        }
                    }
                    else if (tk.token == Common::PlsSql::etSELECT)
                    {
                        stack.pushSpecialIndent();

                        if (new_line_after(tk))
                        {
                            out.putEol();
                            stack.top().bookmark = out.position();
                        }
                        else
                        {
                            stack.incrementCurrentLineTokenCounter(); // to supress indent whitespaces
                            auto step_back = find_eol_backward(out);
                            stack.top().bookmark = out.position() - step_back;
                        }
                    }
                    else if (tk.token == Common::PlsSql::etFROM
                    || tk.token == Common::PlsSql::etJOIN
                    || tk.token == Common::PlsSql::etWHERE
                    || (tk.token == Common::PlsSql::etBY && prevTk.token == etGROUP)
                    || (tk.token == Common::PlsSql::etBY && prevTk.token == etORDER)
                    )
                    {
                        stack.pushSpecialIndent();

                        if (new_line_after(tk))
                            out.putEol();
                        else
                            stack.incrementCurrentLineTokenCounter(); // to supress indent whitespaces
                    }
                    else if (tk.token == Common::PlsSql::etCASE
                    || tk.token == Common::PlsSql::etTHEN
                    || tk.token == Common::PlsSql::etELSE
                    )
                    {
                        stack.pushSpecialIndent(tk.token == Common::PlsSql::etCASE);
                    }
                }

                if (singleLine && lastSingleLineToken == it)
                {
                    lastSingleLineToken = tokens.end();
                    singleLine = false;
                }

                prevTk = tk;
            }
        }

        UndoGroup undoGroup(*this);
        PushInUndoStack(GetPosition());
        DeleteBlock(true);

        std::ostringstream ostr;
        out.flush(ostr);
        string buffer = ostr.str();

        AlignCodeFragment(buffer.c_str(), buffer);

        if (!selection.end.column && selection.end.line < nlines)
            buffer += '\n';

        InsertBlock(buffer.c_str(), false, true);
    }
}
