/*
    Copyright (C) 2002,2016 Aleksey Kochetov

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
    23.09.2002 improvement/performance tuning, undo has been reimplemented
    06.12.2002 bug fix, in find dialog count and mark change "modified" status
    13.01.2003 bug fix, losing modification status if undo stack overflows
    2016.06.12 improvement, Highlighting for changed lines
    2016.06.14 improvement, Alternative Columnar selection mode 
    2016.06.28 improvement, added Undo for SetFileFormat
*/

#include "stdafx.h"
#include "COMMON/StrHelpers.h"
#include "OpenEditor/OEUndo.h"
#include "OpenEditor/OEStorage.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace OpenEditor
{
#ifdef _DEBUG
#define CHECK_UNDOSTACK   //debug_checking()
#else
#define CHECK_UNDOSTACK
#endif


///////////////////////////////////////////////////////////////////////////////

UndoContext::UndoContext (Storage& storage)
: m_storage(storage)
{
    m_selMode = ebtUndefined;
    m_altColumnarMode = false;
    m_position.column = m_position.line = 0;
    m_selection.clear();
    m_onlyStorageAffected = false;
};

///////////////////////////////////////////////////////////////////////////////

UndoStack::UndoStack ()
{
    m_top = m_bottom = m_current = 0;
    m_saved = 0;

    m_countLimit = 1000;
    m_memLimit   = 1024 * 1024;
    m_memUsage   = 0;

    m_cachedDataModified = false;
    m_savedIsGone = false;
    m_completeUndoImpossible = false;
}

UndoStack::~UndoStack ()
{
    try { EXCEPTION_FRAME;

        Clear();
    }
    _DESTRUCTOR_HANDLER_;
}

void UndoStack::Clear ()
{
    CHECK_UNDOSTACK;
    removeAllAbove(m_bottom);

    m_top = m_bottom = m_current = 0;
    m_saved = 0;

    // dont reset m_countLimit, m_memLimit;
    m_memUsage = 0;

    m_cachedDataModified = false;
    m_savedIsGone = false;
    m_completeUndoImpossible = false;

    m_actionSeq.Reset();
    m_cachedActionId = m_actionSeq;
}

#ifdef _DEBUG
void UndoStack::debug_checking () const
{
    _ASSERTE((m_top && m_bottom && m_memUsage)
        || (!m_top && !m_bottom && !m_current && !m_memUsage));

    if (m_top && m_bottom && m_current)
    {
        UndoBase* item = m_top;
        while (item && item->m_below)
            item = item->m_below;
        _ASSERTE(item == m_bottom);

        if (m_current)
        {
            item = m_current;
            while (item && item->m_above)
                item = item->m_above;
            _ASSERTE(item == m_top);
        }

        item = m_bottom;
        while (item && item->m_above)
            item = item->m_above;
        _ASSERTE(item == m_top);
    }
}
#endif

void UndoStack::removeAllAbove (UndoBase* item)
{
    m_actionSeq++;

    // in order to handle properly a save above item
    // we have to traverse from the top
    if (!m_savedIsGone && !m_completeUndoImpossible)
    {
        UndoBase* ptr = m_top;
        while (ptr && ptr != item)
        {
            if (ptr == m_saved)
                m_savedIsGone = true;
            if (m_savedIsGone && !m_completeUndoImpossible)
                m_completeUndoImpossible |= ptr->Modifying();
            ptr = ptr->m_below;
        }
        if (ptr != item)
        {
            // something is deeply wrong if this is happenning
            _ASSERTE(ptr == item);
            m_savedIsGone = true;
            m_completeUndoImpossible = true;
        }
    }

    while (item)
    {
        UndoBase* next = item->m_above;

#ifdef _DEBUG
        // just for testing
        item->m_below = 0;
        item->m_above = 0;
#endif
        m_memUsage -= item->GetMemUsage();
        delete item;
        item = next;
    }
    m_top = 0;
}

void UndoStack::Push (UndoBase* pUndo)
{
    CHECK_UNDOSTACK;
    m_actionSeq++;

    if (m_current != m_top)
    {
        removeAllAbove(m_current ? m_current->m_above : m_bottom);
        m_top = m_current;
    }

    pUndo->SetGroupId(m_groupSeq);

    if (!m_top)
    {
        pUndo->m_below = pUndo->m_above = 0;
        m_current = m_top = m_bottom = pUndo;
    }
    else
    {
        pUndo->m_above = 0;
        pUndo->m_below = m_top;
        m_top->m_above = pUndo;
        m_current = m_top = pUndo;
    }

    m_memUsage += pUndo->GetMemUsage();


    if (!IsEmpty()
    && m_bottom->GetGroupId() != m_current->GetGroupId()
    && (m_memUsage > m_memLimit
        || ((m_top->GetGroupId() - m_bottom->GetGroupId()) > m_countLimit)))
        RemoveExceed();

    CHECK_UNDOSTACK;
}

void UndoStack::RemoveExceed ()
{
    CHECK_UNDOSTACK;

    SequenceId curGroup;

    while (m_bottom
    && (
        // allocated more then limit
        m_memUsage > m_memLimit
        // the number of stored undo groups more then limit
        || ((m_top->GetGroupId() - m_bottom->GetGroupId()) > m_countLimit)
        // group has to be removed whole!
        || m_bottom->GetGroupId() == curGroup
    ))
    {
        if (!m_saved || m_bottom == m_saved)
            m_savedIsGone = true;
        if (m_savedIsGone && !m_completeUndoImpossible)
            m_completeUndoImpossible |= m_bottom->Modifying();

        UndoBase* next = m_bottom->m_above;
#ifdef _DEBUG
        // just for testing
        m_bottom->m_above = 0;
        m_bottom->m_below = 0;
#endif
        m_memUsage -= m_bottom->GetMemUsage();
        curGroup = m_bottom->GetGroupId();
        delete m_bottom;
        m_bottom = next;
        if (m_bottom)
            m_bottom->m_below = 0;
    }
    if (!m_bottom)
        m_current = m_top = 0;

    CHECK_UNDOSTACK;
}

bool UndoStack::Undo (UndoContext& cxt)
{
    CHECK_UNDOSTACK;
    m_actionSeq++;
    cxt.m_lastSaved = m_saved;

    if (CanUndo())
    {
        try
        {
            cxt.m_storage.m_bUndoRedoProcess = true;
            SequenceId groupId = m_current->GetGroupId();

            while (m_current && m_current->GetGroupId() == groupId)
            {
                m_current->Undo(cxt);
                m_current = m_current->m_below;
            }

            cxt.m_storage.m_bUndoRedoProcess = false;
        }
        catch (...)
        {
            cxt.m_storage.m_bUndoRedoProcess = false;
            throw;
        }

        CHECK_UNDOSTACK;
        return true;
    }
    return false;
}

bool UndoStack::Redo (UndoContext& cxt)
{
    CHECK_UNDOSTACK;
    m_actionSeq++;
    cxt.m_lastSaved = m_saved;

    if (CanRedo())
    {
        try
        {
            cxt.m_storage.m_bUndoRedoProcess = true;

            UndoBase* item = m_current ? m_current->m_above : m_bottom;
            SequenceId groupId = item->GetGroupId();

            while (item && item->GetGroupId() == groupId)
            {
                item->Redo(cxt);
                m_current = item;
                item = item->m_above;
            }

            cxt.m_storage.m_bUndoRedoProcess = false;
        }
        catch (...)
        {
            cxt.m_storage.m_bUndoRedoProcess = false;
            throw;
        }

        CHECK_UNDOSTACK;
        return true;
    }
    return false;
}

bool UndoStack::AppendOnInsert (int line, int col, const char* str, int len, const DelimitersMap& delim)
{
    if (len == 1
    && !IsEmpty()
    && m_top == m_current
    && m_top->IsAppendable())
    {
        unsigned memUsage = m_top->GetMemUsage();

        if (m_top->Append(line, col, str, len, delim))
        {
            m_actionSeq++;
            m_memUsage += m_top->GetMemUsage() - memUsage;
            return true;
        }
    }
    return false;
}

/*
all this complexity because undo stack contains non-modifying items
such as cusor position or selection. Otherwise simple IsSavedPos would be sufficient
*/
bool UndoStack::IsDataModified () const
{
    if (m_completeUndoImpossible)
        return true;
    if (IsSavedPos())
        return false;
    if (m_cachedActionId == m_actionSeq)
        return m_cachedDataModified;

    bool modified = false, currFound = false, savedFound = false;

    for (UndoBase* item = m_top; !modified && item; item = item->m_below)
    {
        if (item == m_current)
            currFound = true;
        if (item == m_saved)
            savedFound = true;
        if (currFound && savedFound)
            break;
        if (currFound || savedFound)
            modified |= item->Modifying();
    }

    m_cachedActionId = m_actionSeq;
    m_cachedDataModified = modified;

    return modified;
}

///////////////////////////////////////////////////////////////////////////////
bool UndoBase::AboveSaved (void* saved) const
{
    if (!saved)
        return true;

    const UndoBase* ptr = m_below;
    
    while (ptr)
    {
        if (ptr == saved)
            return true;

        ptr = ptr->m_below;
    }

    return false;
}

bool UndoInsert::Append (int line, int col, const char* str, int len, const DelimitersMap& delim)
{
     if (str && len
     && m_position.line == line
     && m_position.column + static_cast<int>(m_str.length()) == col
     && delim[m_str.last()] == delim[*str])
    {
        m_str.append(str, len);
        return true;
    }
    return false;
}

void UndoInsert::Undo (UndoContext& cxt) const
{
    ELineStatus status = AboveSaved(cxt.m_lastSaved) ? m_status : elsRevertedBevoreSaved;
    cxt.m_position = m_position;
    cxt.m_storage.DeleteLinePart(cxt.m_position.line, cxt.m_position.column, cxt.m_position.column + m_str.length(), status);
}

void UndoInsert::Redo (UndoContext& cxt) const
{
    ELineStatus status = AboveSaved(cxt.m_lastSaved) ? elsUpdated : elsUpdatedSaved;
    cxt.m_position = m_position;
    cxt.m_storage.InsertLinePart(cxt.m_position.line, cxt.m_position.column, m_str.data(), m_str.length(), status);
    cxt.m_position.column += m_str.length();
}

void UndoOverwrite::Undo (UndoContext& cxt) const
{
    ELineStatus status = AboveSaved(cxt.m_lastSaved) ? m_status : elsRevertedBevoreSaved;
    cxt.m_position = m_position;
    cxt.m_storage.ReplaceLinePart(cxt.m_position.line, cxt.m_position.column, cxt.m_position.column + m_newStr.length(),
                            m_orgStr.data(), m_orgStr.length(), status);
}

void UndoOverwrite::Redo (UndoContext& cxt) const
{
    ELineStatus status = AboveSaved(cxt.m_lastSaved) ? elsUpdated : elsUpdatedSaved;
    cxt.m_position = m_position;
    cxt.m_storage.ReplaceLinePart(cxt.m_position.line, cxt.m_position.column, cxt.m_position.column + m_orgStr.length(),
                            m_newStr.data(), m_newStr.length(), status);
    cxt.m_position.column += m_newStr.length();
}

void UndoDelete::Undo (UndoContext& cxt) const
{
    ELineStatus status = AboveSaved(cxt.m_lastSaved) ? m_status : elsRevertedBevoreSaved;
    cxt.m_position = m_position;
    cxt.m_storage.InsertLinePart(cxt.m_position.line, cxt.m_position.column, m_str.data(), m_str.length(), status);
    cxt.m_position.column += m_str.length();
}

void UndoDelete::Redo (UndoContext& cxt) const
{
    ELineStatus status = AboveSaved(cxt.m_lastSaved) ? elsUpdated : elsUpdatedSaved;
    cxt.m_position = m_position;
    cxt.m_storage.DeleteLinePart(cxt.m_position.line, cxt.m_position.column, cxt.m_position.column + m_str.length(), status);
}

/*
it seems it is not used so it is not tested!
*/
void UndoInsertLine::Undo (UndoContext& cxt) const
{
    //ELineStatus status = AboveSaved(cxt.m_lastSaved) ? m_status : elsRevertedBevoreSaved;
    cxt.m_position.column = 0;
    cxt.m_position.line = m_line;
    cxt.m_storage.DeleteLine(cxt.m_position.line);
}

void UndoInsertLine::Redo (UndoContext& cxt) const
{
    //ELineStatus status = AboveSaved(cxt.m_lastSaved) ? elsUpdated : elsUpdatedSaved;
    
    ELineStatus status[2] = {
        AboveSaved(cxt.m_lastSaved) ? elsUpdated : elsUpdatedSaved,
        AboveSaved(cxt.m_lastSaved) ? elsUpdated : elsUpdatedSaved
    };

    cxt.m_position.column = 0;
    cxt.m_position.line = m_line;
    cxt.m_storage.InsertLine(cxt.m_position.line, m_str.data(), m_str.length(), status);
}

void UndoDeleteLine::Undo (UndoContext& cxt) const
{
    ELineStatus status[2] = {
        AboveSaved(cxt.m_lastSaved) ? m_status[0] : elsRevertedBevoreSaved,
        AboveSaved(cxt.m_lastSaved) ? m_status[1] : elsRevertedBevoreSaved
    };

    cxt.m_position.column = 0;
    cxt.m_position.line = m_line;
    cxt.m_storage.InsertLine(cxt.m_position.line, m_str.data(), m_str.length(), status);
}

void UndoDeleteLine::Redo (UndoContext& cxt) const
{
    ELineStatus status = AboveSaved(cxt.m_lastSaved) ? elsUpdated : elsUpdatedSaved;
    cxt.m_position.column = 0;
    cxt.m_position.line = m_line;
    cxt.m_storage.DeleteLine(cxt.m_position.line, status);
}

void UndoSplitLine::Undo (UndoContext& cxt) const
{
    ELineStatus status = AboveSaved(cxt.m_lastSaved) ? m_status : elsRevertedBevoreSaved;
    cxt.m_position = m_position;
    cxt.m_storage.MergeLines(cxt.m_position.line, status);
}

void UndoSplitLine::Redo (UndoContext& cxt) const
{
    ELineStatus status = AboveSaved(cxt.m_lastSaved) ? elsUpdated : elsUpdatedSaved;
    cxt.m_position = m_position;
    cxt.m_storage.SplitLine(cxt.m_position.line, cxt.m_position.column, status, status);
}

void UndoMergeLine::Undo (UndoContext& cxt) const
{
    ELineStatus status[2] = { 
        AboveSaved(cxt.m_lastSaved) ? m_status[0] : elsRevertedBevoreSaved,
        AboveSaved(cxt.m_lastSaved) ? m_status[1] : elsRevertedBevoreSaved
    };
    cxt.m_position = m_position;
    cxt.m_storage.SplitLine(cxt.m_position.line, cxt.m_position.column, status[0], status[1]);
}

void UndoMergeLine::Redo (UndoContext& cxt) const
{
    ELineStatus status = AboveSaved(cxt.m_lastSaved) ? elsUpdated : elsUpdatedSaved;
    cxt.m_position = m_position;
    cxt.m_storage.MergeLines(cxt.m_position.line, status);
}

void UndoInsertLines::Undo (UndoContext& cxt) const
{
    ELineStatus status = AboveSaved(cxt.m_lastSaved) ? m_status : elsRevertedBevoreSaved;
    cxt.m_position.column = 0;
    cxt.m_position.line = m_line;
    cxt.m_storage.DeleteLines(m_line, m_lines.size(), status);
}

void UndoInsertLines::Redo (UndoContext& cxt) const
{
    ELineStatus status = AboveSaved(cxt.m_lastSaved) ? elsUpdated : elsUpdatedSaved;
    vector<ELineStatus> statuses(m_lines.size(), status);
    cxt.m_position.column = 0;
    cxt.m_position.line = m_line;
    cxt.m_storage.InsertLines(cxt.m_position.line, m_lines, status, &statuses);
}

unsigned UndoInsertLines::GetMemUsage() const
{
    unsigned usage = sizeof(*this);

    for (unsigned i = 0; i < m_lines.size(); i++)
        usage += m_lines[i].length();

    return usage;
};

void UndoDeleteLines::Undo (UndoContext& cxt) const
{
    bool above = AboveSaved(cxt.m_lastSaved);
    ELineStatus status = above ? m_status : elsRevertedBevoreSaved;

    vector<ELineStatus> statuses;
    for (unsigned int i = 0; i < m_lines.size(); ++i)
        statuses.push_back(above ? m_lines[i].get_status() : elsRevertedBevoreSaved);

    cxt.m_position.column = 0;
    cxt.m_position.line = m_line;
    cxt.m_storage.InsertLines(cxt.m_position.line, m_lines, status, &statuses);
}

void UndoDeleteLines::Redo (UndoContext& cxt) const
{
    ELineStatus status = AboveSaved(cxt.m_lastSaved) ? elsUpdated : elsUpdatedSaved;
    cxt.m_position.column = 0;
    cxt.m_position.line = m_line;
    cxt.m_storage.DeleteLines(cxt.m_position.line, m_lines.size(), status);
}

unsigned UndoDeleteLines::GetMemUsage() const
{
    unsigned usage = sizeof(*this);

    for (unsigned i = 0; i < m_lines.size(); i++)
        usage += m_lines[i].length();

    return usage;
};

// not tested yet
UndoReorder::UndoReorder (int top, const vector<int>& order, const vector<ELineStatus>& statuses)
: m_top(top), m_order(order), m_statuses(statuses)
{
}

void UndoReorder::Undo (UndoContext& cxt) const
{
    bool above = AboveSaved(cxt.m_lastSaved);

    vector<ELineStatus> statuses;
    for (unsigned int i = 0; i < m_statuses.size(); ++i)
        statuses.push_back(above ? m_statuses[i] : elsRevertedBevoreSaved);

    vector<int> reversed(m_order.size());

    vector<int>::const_iterator it = m_order.begin();

    for (int i = 0; it != m_order.end(); ++it, i++)
        reversed.at(*it - m_top) = i + m_top;

    cxt.m_position.column = 0;
    cxt.m_position.line = m_top;
    cxt.m_storage.Reorder(m_top, reversed, &statuses);
}

void UndoReorder::Redo (UndoContext& cxt) const
{
    ELineStatus status = AboveSaved(cxt.m_lastSaved) ? elsUpdated : elsUpdatedSaved;
    vector<ELineStatus> statuses(m_order.size(), status);

    cxt.m_position.column = 0;
    cxt.m_position.line = m_top;
    cxt.m_storage.Reorder(m_top, m_order, &statuses);
}

unsigned UndoReorder::GetMemUsage() const
{
    return sizeof(*this) + sizeof(int) * m_order.capacity();
}

UndoSetFileFormat::UndoSetFileFormat (EFileFormat oldFormat, EFileFormat newFormat)
    :m_oldFormat(oldFormat), m_newFormat(newFormat)
{
}

void UndoSetFileFormat::Undo (UndoContext& cxt) const
{
    cxt.m_onlyStorageAffected = true;
    cxt.m_storage.SetFileFormat(m_oldFormat);
}

void UndoSetFileFormat::Redo (UndoContext& cxt) const
{
    cxt.m_onlyStorageAffected = true;
    cxt.m_storage.SetFileFormat(m_newFormat);
}

unsigned UndoSetFileFormat::GetMemUsage() const
{
    return sizeof(*this);
}

void UndoCursorPosition::Undo (UndoContext& cxt) const
{
    cxt.m_position = m_position;
}

void UndoCursorPosition::Redo (UndoContext& cxt) const
{
    cxt.m_position = m_position;
}

void UndoSelection::Undo (UndoContext& cxt) const
{
    cxt.m_selMode = m_selMode;
    cxt.m_altColumnarMode = m_AltColumnarMode;
    cxt.m_selection = m_selection;
}

void UndoSelection::Redo (UndoContext& cxt) const
{
    cxt.m_selMode = m_selMode;
    cxt.m_altColumnarMode = m_AltColumnarMode;
    cxt.m_selection = m_selection;
}

void UndoNotification::Undo (UndoContext& cxt) const
{
    if (!cxt.m_storage.m_bDisableNotifications)
    {
        cxt.m_storage.Notify_ChangedLines(0, cxt.m_storage.GetLineCount()+1);
        cxt.m_storage.Notify_ChangedLinesQuantity(0, 0);
    }

    cxt.m_storage.m_bDisableNotifications = !m_disabled;

    if (!cxt.m_storage.m_bDisableNotifications)
    {
        cxt.m_storage.Notify_ChangedLines(0, cxt.m_storage.GetLineCount()+1);
        cxt.m_storage.Notify_ChangedLinesQuantity(0, 0);
    }
}

void UndoNotification::Redo (UndoContext& cxt) const
{
    if (!cxt.m_storage.m_bDisableNotifications)
    {
        cxt.m_storage.Notify_ChangedLines(0, cxt.m_storage.GetLineCount()+1);
        cxt.m_storage.Notify_ChangedLinesQuantity(0, 0);
    }

    cxt.m_storage.m_bDisableNotifications = m_disabled;

    if (!cxt.m_storage.m_bDisableNotifications)
    {
        cxt.m_storage.Notify_ChangedLines(0, cxt.m_storage.GetLineCount()+1);
        cxt.m_storage.Notify_ChangedLinesQuantity(0, 0);
    }
}

}
