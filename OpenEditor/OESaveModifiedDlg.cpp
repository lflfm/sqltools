#include "stdafx.h"
#include "resource.h"
#include "OpenEditor\OESaveModifiedDlg.h"


// CSaveModifiedDlg dialog

COESaveModifiedDlg::COESaveModifiedDlg (DocSavingList& list, BOOL& quickSaveInWorkspace, BOOL thereIsActiveWorkspace)
	: CDialog(COESaveModifiedDlg::IDD),
    m_list(list),
    m_quickSaveInWorkspace(quickSaveInWorkspace),
    m_thereIsActiveWorkspace(thereIsActiveWorkspace)
{
}

BEGIN_MESSAGE_MAP(COESaveModifiedDlg, CDialog)
    ON_BN_CLICKED(IDNO, OnNO)
END_MESSAGE_MAP()

BOOL COESaveModifiedDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    CString title;
    title.LoadString(IDR_MAINFRAME);
    SetWindowText(title);

    ::SetWindowText(::GetDlgItem(m_hWnd, IDC_OESA_USE_WORKSPACE), 
        m_thereIsActiveWorkspace ? L"Save in &Active Workspace" : L"Create Quick Sn&apshot");

    HWND hList = ::GetDlgItem(m_hWnd, IDC_OESA_LIST);
    
    _ASSERTE(hList);

    if (hList)
    {
        int maxWidth = 0;
        CClientDC dc(this);
        CFont* pOrgFont = dc.SelectObject(GetFont());

        for (DocSavingListConst_Iterator it = m_list.begin(); it != m_list.end(); ++it)
        {
            title = it->first->GetPathName();
            if (title.IsEmpty()) title = it->first->GetTitle();
            int index = ::SendMessage(hList, LB_ADDSTRING, NULL, (LPARAM)(LPCWSTR)title);
            ::SendMessage(hList, LB_SETSEL, it->second ? TRUE : FALSE, index);

            CSize size = dc.GetTextExtent(title);
            if (maxWidth < size.cx) maxWidth = size.cx;
        }

        ::SendMessage(hList, LB_SETHORIZONTALEXTENT, maxWidth + 10, 0);
        dc.SelectObject(pOrgFont);
   }

    return TRUE;
}

void COESaveModifiedDlg::OnOK()
{
    HWND hList = ::GetDlgItem(m_hWnd, IDC_OESA_LIST);
    
    DocSavingList_Iterator it = m_list.begin();

    for (int index = 0; it != m_list.end(); ++it, ++index)
    {
        it->second = ::SendMessage(hList, LB_GETSEL, index, 0L) ? true : false;
    }

    CDialog::OnOK();
}


void COESaveModifiedDlg::OnNO()
{
    EndDialog(IDNO);
}

LRESULT COESaveModifiedDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDialog::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_

    return 0;
}

void COESaveModifiedDlg::DoDataExchange (CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_OESA_USE_WORKSPACE, m_quickSaveInWorkspace);
}
