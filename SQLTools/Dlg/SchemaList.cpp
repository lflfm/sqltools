/*
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2004 Aleksey Kochetov

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
#include "SQLTools.h"
#include "COMMON\ExceptionHelper.h"
#include "Dlg/SchemaList.h"
#include <OCI8/BCursor.h>
#include "ServerBackgroundThread\TaskQueue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;
using namespace ServerBackgroundThread;

/////////////////////////////////////////////////////////////////////////////
// CSchemaList

CSchemaList::CSchemaList (BOOL allSchemas)
: m_allSchemas(allSchemas),
m_busy(FALSE)
{
}

BEGIN_MESSAGE_MAP(CSchemaList, CComboBox)
	//{{AFX_MSG_MAP(CSchemaList)
	ON_CONTROL_REFLECT(CBN_SETFOCUS, OnSetFocus)
	//}}AFX_MSG_MAP
    ON_WM_SETCURSOR()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSchemaList message handlers

    LPSCSTR cszSchemasSttm =
        "select username from sys.all_users a_u"
		" where username <> :schema"
        " and exists (select null from all_objects a_s where a_s.owner = a_u.username)";

    LPSCSTR cszAllSchemasSttm =
        "select username from sys.all_users a_u"
		" where username <> :schema";

    // merge this one with BackgroundTask_PopulateSchemaList
	struct PopulateSchemaListTask : Task
    {
        CSchemaList& m_wndSchemaList;
        string m_defSchema;
        bool m_allSchemas;
        vector<string> m_schemas;

        PopulateSchemaListTask (CSchemaList& wndSchemaList, const string& defSchema, bool allSchemas)
            : Task("Populate SchemaList", 0),
            m_wndSchemaList(wndSchemaList),
            m_defSchema(defSchema),
            m_allSchemas(allSchemas)
        {
            m_wndSchemaList.m_busy = TRUE;
            SetCursor(LoadCursor(NULL, IDC_APPSTARTING/*IDC_WAIT*/));
        }

        void DoInBackground (OciConnect& connect)
        {
            SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
            OciCursor cursor(connect);
			cursor.Prepare(m_allSchemas ? cszAllSchemasSttm : cszSchemasSttm);
			cursor.Bind(":schema", m_defSchema.c_str());
            cursor.Execute();
            while (cursor.Fetch())
                m_schemas.push_back(cursor.ToString(0));
        }

        void ReturnInForeground ()
        {
            BOOL dropped = m_wndSchemaList.GetDroppedState();
            if (dropped) 
                m_wndSchemaList.ShowDropDown(FALSE);

            vector<string>::const_iterator it = m_schemas.begin();
            
            for (; it != m_schemas.end(); ++it)
                m_wndSchemaList.AddString(Common::wstr(*it).c_str());

            // TODO: reimplement PUBLIC support! Forgot how :(
            //m_wndSchemaList.AddString("PUBLIC");

            if (dropped) 
                m_wndSchemaList.ShowDropDown(TRUE);

            m_wndSchemaList.m_busy = FALSE;
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }

        void ReturnErrorInForeground () 
        {
            m_wndSchemaList.m_busy = FALSE;
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
    };


void CSchemaList::OnSetFocus()
{
    if (GetCount() <= 1) 
    {
        try { EXCEPTION_FRAME;

            BkgdRequestQueue::Get().Push(TaskPtr(new PopulateSchemaListTask(*this, Common::str(m_defSchema).c_str(), m_allSchemas ? true : false)));
        }
        _DEFAULT_HANDLER_
    }
}

    // BackgroundTask_GetDefSchema
	struct GetDefSchemaTask : Task
    {
        CSchemaList& m_wndSchemaList;
        string m_defSchema;

        GetDefSchemaTask (CSchemaList& wndSchemaList)
            : Task("Populate SchemaList", 0),
            m_wndSchemaList(wndSchemaList)
        {
            m_wndSchemaList.m_busy = TRUE;
            SetCursor(LoadCursor(NULL, IDC_APPSTARTING/*IDC_WAIT*/));
        }

        void DoInBackground (OciConnect& connect)
        {
            string user;
            connect.GetCurrentUserAndSchema(user, m_defSchema);
        }

        void ReturnInForeground ()
        {
            m_wndSchemaList.m_defSchema = m_defSchema.c_str();
            m_wndSchemaList.SetCurSel(m_wndSchemaList.AddString(Common::wstr(m_defSchema).c_str()));
            
            NMHDR nmh;
            nmh.code = CSchemaList::NM_LIST_LOADED;    // Message type defined by control.
            nmh.idFrom = GetDlgCtrlID(m_wndSchemaList.m_hWnd);
            nmh.hwndFrom = m_wndSchemaList.m_hWnd;

            m_wndSchemaList.GetParent()->SendMessage(WM_NOTIFY,  nmh.idFrom, (LPARAM)&nmh);

            m_wndSchemaList.m_busy = FALSE;
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }

        void ReturnErrorInForeground () 
        {
            m_wndSchemaList.m_busy = FALSE;
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
    };


void CSchemaList::UpdateSchemaList ()
{
    ResetContent();

    try { EXCEPTION_FRAME;

        BkgdRequestQueue::Get().Push(TaskPtr(new GetDefSchemaTask(*this)));

    }
    _DEFAULT_HANDLER_
}

void CSchemaList::PreSubclassWindow()
{
	CComboBox::PreSubclassWindow();
    UpdateSchemaList();
}



BOOL CSchemaList::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (m_busy)
    {
        SetCursor(LoadCursor(NULL, IDC_APPSTARTING/*IDC_WAIT*/));
        return TRUE;
    }

    return CComboBox::OnSetCursor(pWnd, nHitTest, message);
}
