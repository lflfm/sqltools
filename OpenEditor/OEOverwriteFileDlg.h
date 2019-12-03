#pragma once

#include "OpenEditorInc.rh"


// OEOverwriteFileDlg dialog
class OEOverwriteFileDlg : public CDialog
{
public:
	OEOverwriteFileDlg (LPCSTR title);

protected:
// Dialog Data
    CString m_Title;
	enum { IDD = IDD_OE_OVERWRITE_FILE };

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

    afx_msg void OnYes ();
    afx_msg void OnNo ();

	DECLARE_MESSAGE_MAP()
};
