#include "stdafx.h"
#include "OEOverwriteFileDlg.h"


// OEOverwriteFileDlg dialog

OEOverwriteFileDlg::OEOverwriteFileDlg (LPCSTR title)
: CDialog(OEOverwriteFileDlg::IDD, NULL),
m_Title(title)
{
}

void OEOverwriteFileDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL OEOverwriteFileDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

    SetWindowText(m_Title);

    ::SendMessage(::GetDlgItem(m_hWnd, IDC_OEOF_EXCLAMATION), 
        STM_SETICON, (WPARAM)::LoadIcon(NULL, IDI_EXCLAMATION), 0L);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

BEGIN_MESSAGE_MAP(OEOverwriteFileDlg, CDialog)
	ON_BN_CLICKED(IDYES, OnYes)
	ON_BN_CLICKED(IDNO,  OnNo)
END_MESSAGE_MAP()

void OEOverwriteFileDlg::OnYes ()
{
    EndDialog(IDYES);
}

void OEOverwriteFileDlg::OnNo ()
{
    EndDialog(IDNO);
}
