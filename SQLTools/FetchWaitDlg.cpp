// FetchWaitDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SQLTools.h"
#include "FetchWaitDlg.h"


// FetchWaitDlg dialog
FetchWaitDlg::FetchWaitDlg (CWnd* pParent, volatile bool& allRowsFetched)
	: CDialog(FetchWaitDlg::IDD, pParent)
    , m_allRowsFetched(allRowsFetched)
    , m_text(_T(""))
{

}

FetchWaitDlg::~FetchWaitDlg()
{
}

void FetchWaitDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_WAIT_FOR_TEXT, m_text);
}


BEGIN_MESSAGE_MAP(FetchWaitDlg, CDialog)
    ON_WM_TIMER()
END_MESSAGE_MAP()


// FetchWaitDlg message handlers


BOOL FetchWaitDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    SetTimer(1, 250, NULL);

    return TRUE;
}


void FetchWaitDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1)
    {
        if (m_allRowsFetched)
        {
            KillTimer(nIDEvent);
            EndDialog(IDOK);
        }
    }

    CDialog::OnTimer(nIDEvent);
}
