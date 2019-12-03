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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __OEHelpers_h__
#define __OEHelpers_h__

#include <string>
#include <vector>
#include <map>
#include <arg_shared.h>
#include <COMMON/QuickArray.h>
#include <COMMON/FixedString.h>
#include <COMMON/Fastmap.h>
#include <COMMON/NVector.h>


namespace OpenEditor
{
    using std::string;
    using std::vector;
    using std::pair;
    using std::map;
    using arg::counted_ptr;
    using Common::Fastmap;

    class Storage;

    enum EBlockMode { ebtStream, /*ebtLine,*/ ebtColumn, ebtUndefined };

    enum EIndent { eiNone = 0, eiAuto = 1, eiSmart = 2 };

    enum ELineStatus { elsNothing = 0, elsUpdated = 1, elsUpdatedSaved = 2, elsRevertedBevoreSaved = 3 };

    const int STRING_ID_SIZE         = 24;
    const int STRING_STATUS_SIZE     =  2;
    const int BOOKMARK_GROUPS_SIZE   =  2;
    const int RANDOM_BOOKMARK_SIZE   =  4;
    const unsigned RANDOM_BOOKMARK_NUMBER = 10U;

    enum EBookmarkGroup { eBmkGroup1 = 0, /*eBmkGroup2, eBmkGroup3,*/ eRandomBmkGroup };

    class EditContextBase
    {
    public:
        virtual void OnChangedLines (int, int) = 0;
        virtual void OnChangedLinesQuantity (int, int) = 0;
        virtual void OnChangedLinesStatus (int, int) = 0;
        virtual void OnSettingsChanged () = 0;
    };


    union Position
    {
        struct
        {
            int column;
            int line;
        };
        int coordinate[2];

        bool operator == (const Position other) const;
        bool operator != (const Position other) const;
        bool operator < (const Position other) const;
    };



    union Square
    {
        struct
        {
            Position start, end;
        };
        int coordinate[4];

        void clear ()                       { memset(this, 0, sizeof(*this)); }
        bool is_empty () const              { return !(start != end); }
        bool normalize ()                   { if (!(start < end)) { std::swap(start, end); return true; } return false; }
        void swap ()                        { std::swap(start, end); }
        void normalize_columns ()           { if (!(start.column < end.column)) std::swap(start.column, end.column); }
        bool line_in_rect (int line) const  { return start.line <= line && line <= end.line; }
        bool column_in_rect (int col) const { return start.column <= col && col <= end.column; }
        bool operator != (const Square& other) const;
    };



#pragma pack(push, 1)
    class String : public Common::FixedString
    {
    public:
        // overflow of 24-bit id is theoretically possible 
        // - we can either expand it to 32-bit integer 
        // - or assign id only on request "GetLineId"
        struct Tag
        {
            unsigned status   : STRING_STATUS_SIZE;
            unsigned id       : STRING_ID_SIZE;
            unsigned bookmark : RANDOM_BOOKMARK_SIZE;
            unsigned bmkmask  : BOOKMARK_GROUPS_SIZE;
        } tag;

        String ();
        String (const String& other);
        void operator = (const String& other);

        ELineStatus get_status () const      { return (ELineStatus)tag.status; }
        void set_status (ELineStatus status) { tag.status = status; }
    };
#pragma pack(pop)


typedef Common::QuickArray<String> StringArray;


    struct DelimitersMap : Fastmap<bool>
    {
        DelimitersMap ();
        DelimitersMap (const char*);

        void Set (const char*);
        void Get (string&);

        static const char* m_cszDefDelimiters;
    private:
        Fastmap<bool> m_Data;
    };

    class LineId
    {
    public:
        LineId ()                                               : m_id(UINT_MAX) {}
        explicit LineId (unsigned id)                           : m_id(id) {}

        unsigned GetId () const                                 { return m_id; }
        bool Valid() const                                      { return m_id != UINT_MAX; }
        int  operator != (const LineId& other) const            { return m_id != other.m_id; }
        int  operator == (const LineId& other) const            { return m_id == other.m_id; }
    private:
        unsigned m_id;
    };


    
    class SequenceId
    {
    public:
        SequenceId ()                                           : m_id(NONE_VAL) {}
        int  operator != (const SequenceId& other) const        { return m_id != other.m_id; }
        int  operator == (const SequenceId& other) const        { return m_id == other.m_id; }
        unsigned operator - (const SequenceId& other) const     { return m_id - other.m_id; }
        unsigned ToUInt () const                                { return m_id; } 
        static const unsigned NONE_VAL  = UINT_MAX;
        static const unsigned START_VAL = 1000U;
    protected:
        SequenceId (unsigned seq)                               : m_id(seq) {}
        unsigned GetNextVal ()                                  { _ASSERTE(m_id >= START_VAL); return ++m_id; }
        void operator ++ (int)                                  { _ASSERTE(m_id >= START_VAL); m_id++; }
        unsigned m_id;
    };


    class Sequence : public SequenceId
    {
    public:
        Sequence ()                                             : SequenceId(START_VAL) {}
        void Reset ()                                           { m_id = START_VAL; }
        using SequenceId::GetNextVal;
        using SequenceId::operator ++;
    private:
        Sequence (const Sequence&);
        Sequence& operator = (const Sequence&);
    };


    class RandomBookmark
    {
    public:
        enum Range { FIRST = 0, LAST = RANDOM_BOOKMARK_NUMBER - 1 };

        RandomBookmark ()                                       {}
        explicit RandomBookmark (unsigned char val)             : m_id(val) { _ASSERTE(Valid()); }
        explicit RandomBookmark (Range val)                     : m_id(static_cast<unsigned char>(val)) {}

        unsigned char GetId () const                            { return m_id; }

        bool Valid() const                                      { return m_id <= LAST; }
        void operator ++ (int)                                  { m_id++; }
    private:
        unsigned char m_id;
    };



    class RandomBookmarkArray
    {
    public:
        RandomBookmarkArray ()                                  { Reset(); }

        int  GetLine (RandomBookmark bookmark) const            { return bookmark.Valid() ? m_lines[bookmark.GetId()] : -1; }
        void SetLine (RandomBookmark bookmark, int line)        { if (bookmark.Valid()) m_lines[bookmark.GetId()] = line; }

        const SequenceId& GetActionId () const                  { return m_actionId; }
        void  SetActionNumber (const SequenceId& val)           { m_actionId = val; }

        void Reset ()                                           { for (size_t i = 0; i < sizeof(m_lines)/sizeof(m_lines[0]); i++) m_lines[i] = -1; }

    private:
        SequenceId m_actionId;
        int m_lines[RANDOM_BOOKMARK_NUMBER];

        RandomBookmarkArray (const RandomBookmarkArray&);
        RandomBookmarkArray& operator = (const RandomBookmarkArray&);
    };



    class LineTokenizer
    {
    public:
        static const unsigned char cbSpaceChar;
        static const unsigned char cbVirtSpaceChar;
        static const unsigned char cbTabChar;

        LineTokenizer (bool showWhiteSpace, int tabSpacing, const DelimitersMap&);

        void StartScan (const char* str, int len);
        bool Next ();
        bool IsSpace () const   { return isspace(m_Buffer[m_Offset]) ? true : false; }
        bool Eol () const       { return m_Offset == m_Length; }

        void GetCurentWord (const char*& str, int& pos, int& len) const;

        void EnableProcessSpaces (bool);

    private:
        const DelimitersMap m_delimiters;
        const char* m_Buffer;
        int   m_Length,
              m_Offset,
              m_Position,
              m_TabSpacing;
        bool  m_showWhiteSpace, m_processSpaces;
        unsigned char m_spaceChar;
        unsigned char m_virtSpaceChar;
        unsigned char m_tabChar;
        mutable string m_Whitespaces;

        friend class EditContext;
    };


    class MultilineQuotesScanner
    {
    public:
        MultilineQuotesScanner (Storage&);

        bool IsCaseSensitive () const;
        void SetCaseSensitive (bool);

        void ClearSettings ();
        void SetParsingQuotes    (const pair<string, string>&);
        int  AddStartLineQuotes  (const string&);
        int  AddSingleLineQuotes (const string&);
        int  AddMultilineQuotes  (const pair<string, string>&);
        void AddEscapeChar       (const string&);
        void SetDelimitersMap    (const DelimitersMap&);
        const DelimitersMap& GetDelimitersMap () const;

        void Invalidate (int from_line);
        void Scan (int to_line, int& state, int& quoteId, bool& parsing);
        bool ScanLine (const char* str, int length, int& state, int& quoteId, bool& parsing, int* parsingLength = 0) const;

    private:
        void buildMap (int to_line);
        void buildOpeningFastMap () const;
        bool is_equal (const char* str, int length, int offset, const string& shape) const;

        Storage& m_Storage;
        DelimitersMap m_delimiters;

        bool m_bCaseSensitive;
        vector<string> m_StartLineQuotes;  // actual from line start to end
        vector<string> m_SingleLineQuotes; // actual any line position to end
        vector<pair<string, string> > m_MultilineQuotes; // opening & closing quotes pair
        string m_escapeChar;

        enum eQuoteType
        {
            eqtStartLine = 0x01,
            eqtSingleLine = 0x04,
            eqtOpeningMultiline = 0x08,
            eqtOpeningParse = 0x10,
            eqtClosingParse = 0x40,
        };
        mutable Fastmap<char> m_OpeningFastMap;
        mutable bool m_OpeningFastMapDirty;

        struct Entry
        {
            int state;
            int quoteId;
            bool parsing;
            Entry (int _state, int _quoteId, bool _parsing)
                : state(_state), quoteId(_quoteId), parsing(_parsing) {}
        };
        map<int, Entry> m_mapQuotes;
        int m_nQuotesValidLimit;

        bool m_bParsingAlways;
        pair<string, string> m_ParsingQuotes; // <> for XML
    };

    enum ESearchBatch { esbCount, esbMark, esbReplace };

    struct FindCtx
    {
        // in-out
        Storage* m_storage;
        int m_line;
        int m_start; 
        int m_end;
        // in
        bool m_thruEof;
        bool m_skipFirstNull;
        // out
        bool m_eofPassed;
        // in
        string m_text;

        FindCtx () 
        { 
            memset(this, 0, sizeof(*this)); 
            m_thruEof = true;
            m_skipFirstNull = true;
        }
    };

    class Searcher
    {
    public:
        Searcher (bool persistent = false);
        virtual ~Searcher ();

        void AddRef    (Storage*);
        void RemoveRef (Storage*);

        bool IsTextEmpty () const;
        const char* GetText () const;
        void SetText (const char* str);
        void GetOption (bool& backward, bool& wholeWords, bool& matchCase, bool& regExpr, bool& searchAll) const;
        void SetOption (bool backward, bool wholeWords, bool matchCase, bool regExpr, bool searchAll);
        bool IsBackwardSearch () const;
        void SetBackwardSearch (bool backward);

        bool Find (FindCtx&) const;
        bool Replace (FindCtx&) const; // m_end is in/out
        int  DoBatch (Storage*, const char* text, ESearchBatch mode, Square& last);

    private:
        void compileExpression () const;
        bool isSelectionMatched (const FindCtx&) const;
        bool isMatched (const char* str, int offset, int len, const DelimitersMap&, int& start, int& end) const;
        bool isMatchedBackward (const char* str, int offset, int len, const DelimitersMap&, int& start, int& end) const;

        Searcher (const Searcher&);   // not supported
        void operator = (const Searcher&); // not supported

        string m_strText;

        bool m_bBackward;
        bool m_bWholeWords;
        bool m_bMatchCase;
        bool m_bRegExpr;
        bool m_bSearchAll;

        struct RegexpCxt; // for isolation from "boost"
        mutable RegexpCxt* m_cxt;
        mutable bool m_bPatternCompiled;

        bool m_bPersistent;
        vector<Storage*> m_refList;
    };

    
    struct LineStatus
    {
        bool syntaxError;
        int  beginCounter;
        int  endCounter;
        int  level;

        LineStatus ()
            : syntaxError(false), 
            beginCounter(0), 
            endCounter(0),
            level(0)
            {}
    };

    class LineStatusMap
    {
    public:
        LineStatusMap (int baseLine, int count)
            : m_data("LineStatusMap", count),
            m_baseLine(baseLine), m_provider(0) {}

        LineStatusMap () 
            : m_data("LineStatusMap"),
            m_baseLine(-1), m_provider(0) {}

        void Reset (int baseLine, int count)             
        { 
            m_baseLine = baseLine;
            m_data.clear();
            m_data.resize(count); 
        }

        LineStatus& operator [] (int inx)               { return m_data.at(inx - m_baseLine); }
        const LineStatus& operator [] (int inx) const   { return m_data.at(inx - m_baseLine); }
        int GetBaseLine () const                        { return m_baseLine; }
        int GetLineCount () const                       { return m_data.size(); }

        void SetProviderAction (const void* provider, SequenceId providerAction)          { m_provider = provider; m_providerAction = providerAction; }
        bool IsSameProviderAction (const void* provider, SequenceId providerAction) const { return (m_provider == provider) && (m_providerAction == providerAction); }

    private:
        int m_baseLine;
        Common::nvector<LineStatus> m_data;
        const void* m_provider;
        SequenceId m_providerAction;

        // prohibited
        LineStatusMap (const LineStatusMap&);
        void operator = (const LineStatusMap&);
    };

    ///////////////////////////////////////////////////////////////////////////////
    // Square inline methods //
    inline
    bool intersect_not_null (const Square& r1, const Square& r2)
    {
        return (r1.line_in_rect(r2.start.line) || r2.line_in_rect(r1.start.line))
           && (r1.column_in_rect(r2.start.column) || r2.column_in_rect(r1.start.column));
    }

    inline
    bool Square::operator != (const Square& other) const
    {
        return (start != other.start || end != other.end);
    }


    // Position inline methods //
    inline
    bool Position::operator != (Position other) const
    {
        return column != other.column || line != other.line;
    }

    inline
    bool Position::operator == (Position other) const
    {
        return column == other.column && line == other.line;
    }

    inline
    bool Position::operator < (Position other) const
    {
        return (line < other.line) || (line == other.line && column < other.column);
    }

    // String inline methods //
    inline
    String::String ()
    : FixedString ()
    {
        memset(&tag, 0, sizeof(tag));
    }

    inline
    String::String (const String& other)
    : FixedString (other)
    {
        *this = other;
    }

    inline
    void String::operator = (const String& other)
    {
        FixedString::assign(other);
        tag = other.tag;
    }

    // LineTokenizer inline methods //
    inline
    void LineTokenizer::EnableProcessSpaces (bool enable)
    {
        m_processSpaces = enable;
    }

    // MultilineQuotesScanner inline methods //
    inline
    void MultilineQuotesScanner::Invalidate (int from_line)
    {
        _ASSERTE(from_line >= 0);
        if (from_line < m_nQuotesValidLimit)
            m_nQuotesValidLimit = max(0, from_line);
    }

    inline
    const DelimitersMap& MultilineQuotesScanner::GetDelimitersMap () const
    {
        return m_delimiters;
    }

    // Searcher inline methods //
    inline
    bool Searcher::IsTextEmpty () const
    {
        return m_strText.empty();
    }

    inline
    const char* Searcher::GetText () const
    {
        return m_strText.c_str();
    }

    inline
    bool Searcher::IsBackwardSearch () const
    {
        return m_bBackward;
    }

    inline
    void Searcher::SetBackwardSearch (bool backward)
    {
        m_bBackward = backward;
    }
}

#endif//__OEHelpers_h__
