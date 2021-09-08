#pragma once
#include <string>

class CConnectSettingsDlg : public CDialog
{
// Dialog Data
	enum { IDD = IDD_CONNECT_SETTINGS };
public:
	CConnectSettingsDlg(CWnd* pParent = NULL);
	virtual ~CConnectSettingsDlg();

protected:
	virtual void DoDataExchange(CDataExchange* pDX); 
	DECLARE_MESSAGE_MAP()

public:
    std::wstring m_Passwords[2];
    bool m_SavePasswords;
    std::string m_ScriptOnLogin;
protected:
    virtual void OnOK();
};
