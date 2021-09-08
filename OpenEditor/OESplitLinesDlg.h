/* 
    Copyright (C) 2011 Aleksey Kochetov

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

#pragma once

// OESplitLinesDlg dialog
class OESplitLinesDlg : public CDialog
{
public:
    OESplitLinesDlg (CWnd* pParent = NULL);

    enum { IDD = IDD_OE_SPLIT_LINES };

    int m_nLeftMargin;
    int m_nRightMargin;
    CString m_sInsertNewLineAfter;
    CString m_sIgnorePreviousBetween;
    CString m_sDontChangeBetween;
    bool m_bAdvancedOptions;

    void GetIgnoreForceNewLine (std::vector<std::pair<wchar_t,wchar_t> >& ) const;
    void GetDontChaneBeetwen (std::vector<std::pair<wchar_t,wchar_t> >& ) const;

protected:
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual void OnOK();

public:
    DECLARE_MESSAGE_MAP()
    afx_msg void OnBnClicked_AdvancedOptions();
};
