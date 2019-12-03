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
    16.03.2003 bug fix, plsql match sometimes fails after some edit operations
    19.10.2006 improvement, use common language support if plsql support fails to find a match
*/

#include "stdafx.h"
#include "OpenEditor/OEContext.h"
#include "OpenEditor/OEPlsSqlSupport.h"
#include "OpenEditor/OECommonLanguageSupport.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace OpenEditor
{

PlSqlSupport::PlSqlSupport (Storage* pstorage)
    : CommonLanguageSupport(pstorage),
    m_parser(m_analyzer)
{
    m_done = false;
    m_currLine  = 0;
    m_batchLimit = 1000;
}

PlSqlSupport::~PlSqlSupport ()
{
}

bool PlSqlSupport::FindMatch (int line, int offset, Match& match, bool quickExactMatch /*= false*/)
{
    _CHECK_AND_THROW_(m_pStorage, "PlSqlSupport has not been initialized!")

    if (!quickExactMatch)
    {
        processText(INT_MAX);

	    int index;
	    const SyntaxNode* node = 0;

	    if (m_analyzer.FindToken(line, offset, node, index))
	    {
            match.found   = true;
            match.broken  = node->IsFailed();
            match.partial = false;
            node->GetToken(index, match.line[0], match.offset[0], match.length[0]);
		    node->GetNextToken(index, match.line[1], match.offset[1], match.length[1], match.backward);
            return true;
	    }
        /*
        {
            LineStatusMap statusMap;
            GetLineStatus(statusMap, line, 1);

            // if there is a syntax error or the analyzer fails
            // we use CommonLanguageSupport
            if (statusMap[line].syntaxError)
                return CommonLanguageSupport::FindMatch(line, offset, match, quickExactMatch);
        }

        match.found = false;

        return false;
        */
    }

    return CommonLanguageSupport(m_pStorage).FindMatch(line, offset, match, true/*quickExactMatch*/);
}

void PlSqlSupport::GetLineStatus (LineStatusMap& statusMap, int baseLine, int count) const
{
    if (statusMap.IsSameProviderAction(this, m_actionId)
    && baseLine == statusMap.GetBaseLine()
    && count == statusMap.GetLineCount()
    )
        return;

    statusMap.Reset(baseLine, count);
    m_analyzer.GetLineStatus(statusMap);

    if (m_done)
        statusMap.SetProviderAction(this, m_actionId);
}

bool PlSqlSupport::OnIdle ()
{
    return processText(m_batchLimit);
}

bool PlSqlSupport::processText (int batchLimit)
{
    // TODO: wait 0.5 - 1 sec after any user changes
    // add limit on the number lines and memory consumption
    if (m_pStorage->GetActionId() != m_storageActionId)
    {
        m_done = false;
        m_currLine  = 0;
        m_parser.Clear();
        m_analyzer.Clear(); // 16.03.2003 bug fix, plsql match sometimes fails after some edit operations
        m_storageActionId = m_pStorage->GetActionId();
    }

    if (!m_done)
    {
        m_actionId++;

        //int prevCurrLine = m_currLine;
        int nlines = m_pStorage->GetLineCount();

        for (int i = 0; i < batchLimit && m_currLine < nlines; i++, m_currLine++)
        {
	        int length;
	        const char* str;
            m_pStorage->GetLine(m_currLine, str, length);
	        m_parser.PutLine(m_currLine, str, length);
        }

        if (m_currLine == nlines)
        {
            m_done = true;
            m_parser.PutEOF(m_currLine);
        }

        // 2011.09.19 bug fix, editor gutter might not show syntax nesting correctly during incremantal parsing
        // ideally we should limit refresh by top syntax block that includes prevCurrLine
        //m_pStorage->Notify_ChangedLinesStatus(prevCurrLine, m_currLine);
        m_pStorage->Notify_ChangedLinesStatus(0, m_currLine);

        TRACE("PlSqlSupport::processText: curr line = %d, lines = %d\n", m_currLine, nlines);
    }

    return  !m_done;
}

};