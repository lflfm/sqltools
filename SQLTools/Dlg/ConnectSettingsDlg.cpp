#include "stdafx.h"
#include "SQLTools.h"
#include "Dlg\ConnectSettingsDlg.h"
#include <COMMON\DlgDataExt.h>


CConnectSettingsDlg::CConnectSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConnectSettingsDlg::IDD, pParent)
    , m_SavePasswords(false)
    , m_ScriptOnLogin("NOT IMPLEMENTED YET")
{
}

CConnectSettingsDlg::~CConnectSettingsDlg()
{
}

void CConnectSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_CS_PASSWORD1, m_Passwords[0]);
    DDX_Text(pDX, IDC_CS_PASSWORD2, m_Passwords[1]);
    DDX_Check(pDX, IDC_CS_SAVE_PASSWORD, m_SavePasswords);
    DDX_Text(pDX, IDC_CS_SCRIPT_ON_LOGON, m_ScriptOnLogin);
}

BEGIN_MESSAGE_MAP(CConnectSettingsDlg, CDialog)
END_MESSAGE_MAP()

void CConnectSettingsDlg::OnOK()
{
    UpdateData();

    if (m_Passwords[0] != m_Passwords[1])
    {
        MessageBeep((UINT)-1);
        AfxMessageBox("Master password and confirmation are not identical!", MB_OK|MB_ICONSTOP);
        ::SetFocus(::GetDlgItem(m_hWnd, IDC_CS_PASSWORD1));
        return;
    }
    
    CDialog::OnOK();
}
