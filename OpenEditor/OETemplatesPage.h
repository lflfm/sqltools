/* 
    Copyright (C) 2002, 2017 Aleksey Kochetov

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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __OETemplatesPage_h__
#define __OETemplatesPage_h__

#include "resource.h"
#include "OpenEditor/OESettings.h"
#include "COMMON\ManagedListCtrl.h"


    class TemplateAdapter : public Common::ListCtrlDataProvider, Common::ListCtrlDataProviderHelper
    {
        typedef OpenEditor::Template::Entry Entry;
        typedef OpenEditor::TemplatePtr TemplatePtr;

        TemplatePtr m_entries;

        enum
        {
            e_name     = 0, 
            e_keyword     , 
            e_min_length  ,
            e_count
        };

    public:
        TemplateAdapter () : m_entries(0) {}
        TemplateAdapter (TemplatePtr entries) : m_entries(entries) {}

        void SetData (TemplatePtr entries) { m_entries = entries; }

        const Entry& data (int row) const { return m_entries->GetEntry(row); }

        virtual int getRowCount () const { return (int)m_entries->GetCount(); }

        virtual int getColCount () const { return e_count; }

        virtual Type getColType (int col) const {
            switch (col) {
            case e_name       : return String;   
            case e_keyword    : return String;      
            case e_min_length : return Number;     
            }
            return String;
        }

        virtual const wchar_t* getColHeader (int col) const {
            switch (col) {
            case e_name       : return L"Name";   
            case e_keyword    : return L"Keyword";      
            case e_min_length : return L"MinLen";     
            }
            return L"Unknown";
        }

        virtual const wchar_t* getString (int row, int col) const {
            switch (col) {
            case e_name       : return getStr(data(row).name     );
            case e_keyword    : return getStr(data(row).keyword  );
            case e_min_length : return getStr(data(row).minLength);
            }
            return L"Unknown";
        }

        bool IsVisibleRow (int row) const {
            return !data(row).deleted;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case e_name       : return comp(data(row1).name     , data(row2).name     );
            case e_keyword    : return comp(data(row1).keyword  , data(row2).keyword  );
            case e_min_length : return comp(data(row1).minLength, data(row2).minLength);
            }
            return 0;
        }

        virtual int getImageIndex (int) const { return -1; }
    };

class COETemplatesPage : public CPropertyPage
{
    OpenEditor::SettingsManager& m_manager;
    OpenEditor::TemplatePtr m_currTemplate;
    TemplateAdapter m_adapter;
    bool m_templateListInitialized;

    Common::VisualAttribute m_textAttr;
    enum { IDD = IDD_OE_TEMPLATES };
    //CListCtrl m_templateList;
    Common::CManagedListCtrl m_templateList;
    CString  m_templateName;
    int m_startEntry;
    string m_newTemplateText;

    int findItem (int entry) const;
    int getCurrentSelection () const;

public:
    COETemplatesPage (OpenEditor::SettingsManager&, LPCSTR name, int startEntry = -1);
    ~COETemplatesPage ();

    void SetNewTemplateText (const string& newTemplateText) { m_newTemplateText = newTemplateText; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClicked_AddTempl();
    afx_msg void OnBnClicked_EditTempl();
    afx_msg void OnBnClicked_DelTempl();
    afx_msg void OnNMDblclk_TemplList(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnCbnSelchange_Category();
    afx_msg void OnBnClicked_ListIfAlternative();
};

//{{AFX_INSERT_LOCATION}}

#endif//__OETemplatesPage_h__
