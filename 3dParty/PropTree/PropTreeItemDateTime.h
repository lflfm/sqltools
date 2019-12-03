#pragma once
#include "PropTreeItem.h"


class CPropTreeItemDateTime : public CDateTimeCtrl, public CPropTreeItem
{
	DECLARE_DYNAMIC(CPropTreeItemDateTime)
    SYSTEMTIME m_datetime;

public:
	CPropTreeItemDateTime();
	virtual ~CPropTreeItemDateTime();

public:
	BOOL CreateDateTimeCtrl (DWORD dwStyle = WS_CHILD|DTS_LONGDATEFORMAT);

	// Retrieve the item's attribute value
    SYSTEMTIME GetItemValue () const                    { return m_datetime; }

	// Set the item's attribute value
    void SetItemValue (const SYSTEMTIME& datetime)      { m_datetime = datetime; }

	// The attribute area needs drawing
	virtual void DrawAttribute(CDC* pDC, const RECT& rc);

	// Called when attribute area has changed size
	virtual void OnMove();

	// Called when the item needs to refresh its data
	virtual void OnRefresh();

	// Called when the item needs to commit its changes
	virtual void OnCommit();

	// Called to activate the item
	virtual void OnActivate();

    // ak
    virtual void OnCancel();


protected:
	DECLARE_MESSAGE_MAP()
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
};


