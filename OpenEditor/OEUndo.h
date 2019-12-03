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

// 23.09.2002 improvement/performance tuning, undo has been reimplemented
// 06.12.2002 bug fix, in find dialog count and mark change "modified" status

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __OEUndo_h__
#define __OEUndo_h__

#include <string>
#include <vector>
#include <map>
#include <arg_shared.h>
#include <OpenEditor/OEHelpers.h>

#pragma warning(push)
#pragma warning(disable : 4244)

namespace OpenEditor
{
    using Common::FixedString;
    using std::vector;
    using std::pair;
    using std::map;
    using arg::counted_ptr;

    class Storage;

    enum EFileFormat;

    //enum EEditOperation
    //{
    //    eeoInsert,
    //    eeoOverwrite,
    //    eeoDelete,
    //    eeoInsertLine,
    //    eeoDeleteLine,
    //    eeoSplitLine,
    //    eeoMergeLine,
    //    eeoInsertLines,
    //    eeoDeleteLines,
    //    eeoCursorPosition,
    //    eeoSelection,
    //    eeoNotification,
    //    eeoGroup,
    //};

    struct UndoContext
    {
        UndoContext (Storage&);
        Storage&   m_storage;
        EBlockMode m_selMode;
        bool       m_altColumnarMode;
        Position   m_position;
        Square     m_selection;
        void*      m_lastSaved;
        bool       m_onlyStorageAffected;
    };

        class UndoBase;

    class UndoStack
    {
    public:
        UndoStack ();
        ~UndoStack ();

        unsigned GetCountLimit () const;
        void SetCountLimit (unsigned);
        unsigned GetMemLimit () const;
        void SetMemLimit (unsigned);

        void SetSavedPos ();
        void SetUnconditionallyModified ();
        bool IsSavedPos () const;
        bool IsDataModified () const;

        unsigned Size () const;
        bool IsEmpty () const;
        void Clear ();

        void NextUndoGroup ();
        void Push (UndoBase*);
        void RemoveExceed ();

        bool CanUndo () const;
        bool CanRedo () const;
        bool Undo (UndoContext&);
        bool Redo (UndoContext&);

        unsigned GetMemUsage() const;

        bool AppendOnInsert (int, int, const char*, int, const DelimitersMap&);

    private:
        void removeAllAbove (UndoBase*);
        void debug_checking () const;

        UndoBase* m_top, *m_bottom, *m_current;
        void* m_saved;

        unsigned m_countLimit;
        unsigned m_memUsage, m_memLimit;

        Sequence m_actionSeq, m_groupSeq;

        mutable SequenceId m_cachedActionId;
        mutable bool m_cachedDataModified;

        bool m_savedIsGone, // saved action has been removed, but it might be cursor movement or selection
             m_completeUndoImpossible; // modifing operation has been removed and complete undo is impossible
    };

#pragma pack(push, 1)
    class UndoBase
    {
    public:
        UndoBase ();
        virtual ~UndoBase ();
        virtual void Undo (UndoContext&) const = 0;
        virtual void Redo (UndoContext&) const = 0;
        //virtual EEditOperation Type () const = 0;
        virtual bool Modifying () const { return true; };
        const SequenceId& GetGroupId () const { return m_groupId; }
        void SetGroupId (const SequenceId& groupId) { m_groupId = groupId; }
        virtual unsigned GetMemUsage() const = 0;

        virtual bool IsAppendable () const { return false; }
        virtual bool Append (int, int, const char*, int, const DelimitersMap&) { return false; }
        bool AboveSaved (void* saved) const;
    protected:
        SequenceId m_groupId;
        friend class UndoStack;
        UndoBase* m_below, *m_above;
    };

    class UndoInsert : public UndoBase
    {
    public:
        UndoInsert (int, int, const char*, int, ELineStatus);
        virtual void Undo (UndoContext&) const;
        virtual void Redo (UndoContext&) const;
        //virtual EEditOperation Type () const { return eeoInsert; };
        virtual unsigned GetMemUsage() const { return sizeof(*this) + m_str.length(); };

        virtual bool IsAppendable () const { return true; }
        virtual bool Append (int, int, const char*, int, const DelimitersMap&);
    private:
        Position m_position;
        FixedString m_str;
        ELineStatus m_status;
    };

    class UndoOverwrite : public UndoBase
    {
    public:
        UndoOverwrite (int, int, const char*, int, const char*, int, ELineStatus);
        virtual void Undo (UndoContext&) const;
        virtual void Redo (UndoContext&) const;
        //virtual EEditOperation Type () const { return eeoOverwrite; };
        virtual unsigned GetMemUsage() const { return sizeof(*this) + m_orgStr.length() + m_newStr.length(); };

    private:
        Position m_position;
        FixedString m_orgStr, m_newStr;
        ELineStatus m_status;
    };

    class UndoDelete : public UndoBase
    {
    public:
        UndoDelete (int, int, const char*, int, ELineStatus);
        virtual void Undo (UndoContext&) const;
        virtual void Redo (UndoContext&) const;
        //virtual EEditOperation Type () const { return eeoDelete; };
        virtual unsigned GetMemUsage() const { return sizeof(*this) + m_str.length(); };
    private:
        Position m_position;
        FixedString m_str;
        ELineStatus m_status;
    };

    class UndoInsertLine : public UndoBase
    {
    public:
        UndoInsertLine (int, const char*, int, ELineStatus);
        virtual void Undo (UndoContext&) const;
        virtual void Redo (UndoContext&) const;
        //virtual EEditOperation Type () const { return eeoInsertLine; };
        virtual unsigned GetMemUsage() const { return sizeof(*this) + m_str.length(); };
    private:
        int m_line;
        FixedString m_str;
        ELineStatus m_status;
    };

    class UndoDeleteLine : public UndoBase
    {
    public:
        UndoDeleteLine (int, const char*, int, ELineStatus[2]);
        virtual void Undo (UndoContext&) const;
        virtual void Redo (UndoContext&) const;
        //virtual EEditOperation Type () const { return eeoDeleteLine; };
        virtual unsigned GetMemUsage() const { return sizeof(*this) + m_str.length(); };
    private:
        int m_line;
        FixedString m_str;
        ELineStatus m_status[2];
    };

    class UndoSplitLine : public UndoBase
    {
    public:
        UndoSplitLine (int, int, ELineStatus);
        virtual void Undo (UndoContext&) const;
        virtual void Redo (UndoContext&) const;
        //virtual EEditOperation Type () const { return eeoSplitLine; };
        virtual unsigned GetMemUsage() const { return sizeof(*this); };
    private:
        Position m_position;
        ELineStatus m_status;
    };

    class UndoMergeLine : public UndoBase
    {
    public:
        UndoMergeLine (int, int, ELineStatus[2]);
        virtual void Undo (UndoContext&) const;
        virtual void Redo (UndoContext&) const;
        //virtual EEditOperation Type () const { return eeoMergeLine; };
        virtual unsigned GetMemUsage() const { return sizeof(*this); };
    private:
        Position m_position;
        ELineStatus m_status[2];
    };

    class UndoInsertLines : public UndoBase
    {
    public:
        UndoInsertLines (unsigned line, const StringArray& lines, ELineStatus status);
        virtual void Undo (UndoContext&) const;
        virtual void Redo (UndoContext&) const;
        //virtual EEditOperation Type () const { return eeoInsertLines; };
        virtual unsigned GetMemUsage() const;
    private:
        unsigned m_line;
        StringArray m_lines;
        ELineStatus m_status; // insert line status
    };

    class UndoDeleteLines : public UndoBase
    {
    public:
        UndoDeleteLines (unsigned line, const StringArray& lines, ELineStatus status);
        virtual void Undo (UndoContext&) const;
        virtual void Redo (UndoContext&) const;
        //virtual EEditOperation Type () const { return eeoDeleteLines; };
        virtual unsigned GetMemUsage() const;
    private:
        unsigned m_line;
        StringArray m_lines;
        ELineStatus m_status; // next line status
    };

    class UndoReorder : public UndoBase
    {
    public:
        UndoReorder (int top, const vector<int>& order, const vector<ELineStatus>&);
        virtual void Undo (UndoContext&) const;
        virtual void Redo (UndoContext&) const;
        //virtual EEditOperation Type () const { return eeoReorder; };
        virtual unsigned GetMemUsage() const;
    private:
        int m_top;
        const vector<int> m_order;
        const vector<ELineStatus> m_statuses;
    };

    class UndoSetFileFormat : public UndoBase
    {
    public:
        UndoSetFileFormat (EFileFormat oldFormat, EFileFormat newFormat);
        virtual void Undo (UndoContext&) const;
        virtual void Redo (UndoContext&) const;
        virtual unsigned GetMemUsage() const;
    private:
        EFileFormat m_oldFormat, m_newFormat;
    };

    class UndoCursorPosition : public UndoBase
    {
    public:
        UndoCursorPosition (int, int);
        virtual void Undo (UndoContext&) const;
        virtual void Redo (UndoContext&) const;
        //virtual EEditOperation Type () const { return eeoCursorPosition; };
        virtual bool Modifying () const { return false; };
        virtual unsigned GetMemUsage() const { return sizeof(*this); };
    private:
        Position m_position;
    };

    class UndoSelection : public UndoBase
    {
    public:
        UndoSelection (EBlockMode selMode, bool altColumnarMode, const Square& selection);
        virtual void Undo (UndoContext&) const;
        virtual void Redo (UndoContext&) const;
        //virtual EEditOperation Type () const { return eeoSelection; };
        virtual bool Modifying () const { return false; };
        virtual unsigned GetMemUsage() const { return sizeof(*this); };
    private:
        Square m_selection;
        EBlockMode m_selMode;
        bool m_AltColumnarMode;
    };

    class UndoNotification : public UndoBase
    {
    public:
        UndoNotification (bool);
        virtual void Undo (UndoContext&) const;
        virtual void Redo (UndoContext&) const;
        //virtual EEditOperation Type () const { return eeoNotification; }
        virtual bool Modifying () const { return false; };
        virtual unsigned GetMemUsage() const { return sizeof(*this); };
    private:
        bool m_disabled;
    };

#pragma pack(pop)

    // inline methods //
    inline
    unsigned UndoStack::GetCountLimit () const
        { return m_countLimit; }
    inline
    void UndoStack::SetCountLimit (unsigned limit)
        { m_countLimit = limit; }
    inline
    unsigned UndoStack::GetMemLimit () const
        { return m_memLimit; }
    inline
    void UndoStack::SetMemLimit (unsigned limit)
        { m_memLimit = limit; }
    inline
    void UndoStack::NextUndoGroup ()
        { m_groupSeq++; }
    inline
    void UndoStack::SetSavedPos ()
        {
            m_saved = m_current;
            m_savedIsGone =
            m_completeUndoImpossible = false;
        }
    inline
    void UndoStack::SetUnconditionallyModified ()
    {
        m_saved = 0;
        m_savedIsGone =
        m_completeUndoImpossible = true;
    }
    inline
    bool UndoStack::IsSavedPos () const
        { return m_saved == m_current ? true : false; }
    inline
    bool UndoStack::IsEmpty () const
        { return m_top ? false : true; }
    inline
    bool UndoStack::CanUndo () const
        { return m_current ? true : false; }
    inline
    bool UndoStack::CanRedo () const
        { return m_current != m_top ? true : false; }
    inline
    unsigned UndoStack::GetMemUsage() const
        { return m_memUsage + sizeof(*this); }

    inline
    UndoBase::UndoBase ()
        { memset(this, 0, sizeof(*this)); };
    inline
    UndoBase::~UndoBase ()
        { _ASSERTE(!m_above && !m_below); }
    inline
    UndoInsert::UndoInsert (int line, int col, const char* str, int len, ELineStatus status)
        : m_str(str, len) { m_position.line = line; m_position.column = col; m_status = status; }
    inline
    UndoOverwrite::UndoOverwrite (int line, int col, const char* orgStr, int orgLen, const char* newStr, int newLen, ELineStatus status)
        : m_orgStr(orgStr, orgLen), m_newStr(newStr, newLen) { m_position.line = line; m_position.column = col; m_status = status; }
    inline
    UndoDelete::UndoDelete (int line, int col, const char* str, int len, ELineStatus status)
        : m_str(str, len){ m_position.line = line; m_position.column = col; m_status = status; }
    inline
    UndoInsertLine::UndoInsertLine (int line, const char* str, int len, ELineStatus status)
        : m_str(str, len){ m_line = line; m_status = status; }
    inline
    UndoDeleteLine::UndoDeleteLine (int line, const char* str, int len, ELineStatus status[2])
        : m_str(str, len){ m_line = line; ; m_status[0] = status[0]; m_status[1] = status[1]; }
    inline
    UndoSplitLine::UndoSplitLine (int line, int col, ELineStatus status)
        { m_position.line = line; m_position.column = col; m_status = status; }
    inline
    UndoMergeLine::UndoMergeLine (int line, int col, ELineStatus status[2])
        { m_position.line = line; m_position.column = col; m_status[0] = status[0]; m_status[1] = status[1]; }
    inline
    UndoInsertLines::UndoInsertLines (unsigned line, const StringArray& lines, ELineStatus status)
        : m_lines(lines) { m_line = line; m_status = status; }
    inline
    UndoDeleteLines::UndoDeleteLines (unsigned line, const StringArray& lines, ELineStatus status)
        : m_lines(lines) { m_line = line; m_status = status; }
    inline
    UndoCursorPosition::UndoCursorPosition (int line, int col)
        { m_position.line = line; m_position.column = col; }
    inline
    UndoSelection::UndoSelection (EBlockMode selMode, bool altColumnarMode, const Square& selection)
        { m_selMode = selMode; m_AltColumnarMode = altColumnarMode; m_selection = selection; }
    inline
    UndoNotification::UndoNotification (bool disabled) : m_disabled(disabled) {}

}

#pragma warning(pop)

#endif//__OEUndo_h__
