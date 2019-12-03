#pragma once


// FetchWaitDlg dialog

class FetchWaitDlg : public CDialog
{
public:
	FetchWaitDlg (CWnd* pParent, volatile bool& allRowsFetched);
	virtual ~FetchWaitDlg();

// Dialog Data
	enum { IDD = IDD_WAIT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    CString m_text;
    volatile bool& m_allRowsFetched;
    virtual BOOL OnInitDialog();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
};
