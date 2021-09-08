#include "stdafx.h"
#include "PropTree.h"
#include "PropTreeItemDateTime.h"


IMPLEMENT_DYNAMIC(CPropTreeItemDateTime, CDateTimeCtrl)
CPropTreeItemDateTime::CPropTreeItemDateTime()
{
    memset(&m_datetime, 0, sizeof(m_datetime));
}

CPropTreeItemDateTime::~CPropTreeItemDateTime()
{
}


BOOL CPropTreeItemDateTime::CreateDateTimeCtrl (DWORD dwStyle)
{
	ASSERT(m_pProp!=NULL);

	if (IsWindow(m_hWnd))
		DestroyWindow();

	// force as not visible child window
	dwStyle = (WS_CHILD|dwStyle) & ~WS_VISIBLE;

	if (!Create(dwStyle, CRect(0,0,0,0), m_pProp->GetCtrlParent(), GetCtrlID()))
	{
		TRACE0("CPropTreeItemCombo::CCreateDateTimeCtrl() - failed to create combo box\n");
		return FALSE;
	}

	SendMessage(WM_SETFONT, (WPARAM)m_pProp->GetNormalFont()->m_hObject);
    SendMessage(DTM_SETSYSTEMTIME, GDT_NONE, 0);

	return TRUE;
}

BEGIN_MESSAGE_MAP(CPropTreeItemDateTime, CDateTimeCtrl)
    ON_WM_CHAR()
    ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

void CPropTreeItemDateTime::DrawAttribute(CDC* pDC, const RECT& rc)
{
	ASSERT(m_pProp!=NULL);

	// verify the window has been created
	if (!IsWindow(m_hWnd))
	{
		TRACE0("CPropTreeItemDateTime::DrawAttribute() - The window has not been created\n");
		return;
	}

    if (m_datetime.wYear)
    {
        pDC->SelectObject(m_pProp->GetNormalFont());
        pDC->SetTextColor(GetSysColor(!IsReadOnly() ? COLOR_WINDOWTEXT : COLOR_GRAYTEXT));
	    pDC->SetTextColor(RGB(0,0,0));
	    pDC->SetBkMode(TRANSPARENT);

        TCHAR buffer[100];
        GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &m_datetime, 0,  buffer, sizeof(buffer)/sizeof(buffer[0]));
	    CRect r = rc;
        r.InflateRect(-2,-2);
	    pDC->DrawText(buffer, &r, DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
    }
}


void CPropTreeItemDateTime::OnMove()
{
	if (IsWindow(m_hWnd) && IsWindowVisible())
		SetWindowPos(NULL, m_rc.left-5, m_rc.top, m_rc.Width()+5, m_rc.Height(), SWP_NOZORDER|SWP_SHOWWINDOW);
}


void CPropTreeItemDateTime::OnRefresh()
{
    COleDateTime d;
    d.SetDate(m_datetime.wYear, m_datetime.wMonth, m_datetime.wDay);
    SetTime(d);
}


void CPropTreeItemDateTime::OnCommit()
{
    if (GetTime(&m_datetime) == GDT_NONE)
        memset(&m_datetime, 0, sizeof(m_datetime));

	ShowWindow(SW_HIDE);
}


void CPropTreeItemDateTime::OnActivate()
{
    if (m_datetime.wYear)
    {
        COleDateTime d;
        d.SetDate(m_datetime.wYear, m_datetime.wMonth, m_datetime.wDay);
        SetTime(d);
    }
    else
    {
        SYSTEMTIME systemtime;
        ::GetSystemTime(&systemtime);
        SendMessage(DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&systemtime);
        SendMessage(DTM_SETSYSTEMTIME, GDT_NONE, 0);
    }

	SetWindowPos(NULL, m_rc.left-5, m_rc.top, m_rc.Width()+5, m_rc.Height(), SWP_NOZORDER|SWP_SHOWWINDOW);
	SetFocus();
}

// ak
void CPropTreeItemDateTime::OnCancel()
{
	// hide edit control
	if (m_hWnd) ShowWindow(SW_HIDE);
}

// CPropTreeItemDateTime message handlers
void CPropTreeItemDateTime::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar==VK_RETURN)
		CommitChanges();
	
    CDateTimeCtrl::OnChar(nChar, nRepCnt, nFlags);
}

void CPropTreeItemDateTime::OnKillFocus(CWnd* pNewWnd)
{
    CDateTimeCtrl::OnKillFocus(pNewWnd);

    if (pNewWnd != GetMonthCalCtrl())
        CommitChanges();
}
