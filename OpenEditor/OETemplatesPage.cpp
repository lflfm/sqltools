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

// 2017-12-25 UI update
// 2017-12-25 improvement, added quick access to template dialog using context menu

#include "stdafx.h"
#include <COMMON/ExceptionHelper.h>
#include "OpenEditor/OETemplates.h"
#include "OpenEditor/OETemplatesPage.h"
#include "OpenEditor/OETemplatesDlg.h"
#include "OpenEditor/OELanguageManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using namespace std;
    using namespace OpenEditor;

    static struct 
    {
        const char* title;
        int width;
        int format;
    }
    g_columns[] =
    {
        { "Name",   170, LVCFMT_LEFT  },
        { "Key",    125, LVCFMT_LEFT  },
        { "MinLen",  60, LVCFMT_RIGHT },
    };

/////////////////////////////////////////////////////////////////////////////
// COETemplatesPage property page

COETemplatesPage::COETemplatesPage (SettingsManager& manager, LPCSTR name, int startEntry) 
: CPropertyPage(COETemplatesPage::IDD),
m_manager(manager),
m_templateName(name),
m_startEntry(startEntry),
m_templateList(m_adapter),
m_templateListInitialized(false)
{
}

COETemplatesPage::~COETemplatesPage()
{
}

int COETemplatesPage::findItem (int entry) const
{
    LVFINDINFO fi;
    memset(&fi, 0, sizeof(fi));
    fi.flags = LVFI_PARAM;
    fi.lParam = entry;
    return m_templateList.FindItem(&fi);
}

int COETemplatesPage::getCurrentSelection () const
{
    ASSERT_EXCEPTION_FRAME;

    if (POSITION pos = m_templateList.GetFirstSelectedItemPosition())
    {
        LVITEM item;
        memset(&item, 0, sizeof(item));
        item.mask = LVIF_PARAM;
        item.iItem = m_templateList.GetNextSelectedItem(pos);

        VERIFY(m_templateList.GetItem(&item));
        return item.lParam;
    }

    THROW_APP_EXCEPTION("No current selection");
}

void COETemplatesPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_OET_TEMPL_LIST, m_templateList);
    DDX_Text(pDX, IDC_OET_CATEGORY, m_templateName);
}


BEGIN_MESSAGE_MAP(COETemplatesPage, CPropertyPage)
    ON_BN_CLICKED(IDC_OET_ADD_TEMPL, OnBnClicked_AddTempl)
    ON_BN_CLICKED(IDC_OET_EDIT_TEMPL, OnBnClicked_EditTempl)
    ON_BN_CLICKED(IDC_OET_DEL_TEMPL, OnBnClicked_DelTempl)
    ON_NOTIFY(NM_DBLCLK, IDC_OET_TEMPL_LIST, OnNMDblclk_TemplList)
    ON_CBN_SELCHANGE(IDC_OET_CATEGORY, OnCbnSelchange_Category)
    ON_BN_CLICKED(IDC_OET_LIST_IF_ALTERNATIVE, OnBnClicked_ListIfAlternative)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COETemplatesPage message handlers

BOOL COETemplatesPage::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();

    try { EXCEPTION_FRAME;

        m_templateList.SetExtendedStyle(LVS_EX_FULLROWSELECT);

        SendDlgItemMessage(IDC_OET_CATEGORY, CB_RESETCONTENT);
        
        // lest add missing templates
        {
            vector<string> langs;
            int nlangs = m_manager.GetClassCount();
            for (int i = 0; i < nlangs; ++i)
                langs.push_back(m_manager.GetClassByPos(i)->GetName());

            vector<string>::const_iterator it = langs.begin();
            for (; it != langs.end(); ++it)
            {
                TemplatePtr tmpl = m_manager.GetTemplateByName(*it);
                if (!tmpl.get())
                    m_manager.Add(*it);
            }
        }

        const OpenEditor::TemplateCollection& templColl 
            = m_manager.GetTemplateCollection();

        OpenEditor::TemplateCollection::ConstIterator 
            it = templColl.Begin(),
            end = templColl.End();

        int startIndex = 0;
        for (int index = 0; it != end; ++it, ++index)
        {
            SendDlgItemMessage(IDC_OET_CATEGORY, CB_ADDSTRING, 0, 
                (LPARAM)it->first.c_str());
            if (it->first == (LPCSTR)m_templateName)
                startIndex = index;
        }
        
        SendDlgItemMessage(IDC_OET_CATEGORY, CB_SETCURSEL, startIndex);
        OnCbnSelchange_Category();

        if (m_startEntry >= 0)
        {
            int item = findItem(m_startEntry);
            m_templateList.SetItemState(item, LVIS_SELECTED, LVIS_SELECTED);
            m_templateList.EnsureVisible(item, FALSE);
            PostMessage(WM_COMMAND, IDC_OET_EDIT_TEMPL);
        }
        else if (m_startEntry == -2)
        {
            PostMessage(WM_COMMAND, IDC_OET_ADD_TEMPL);
        }
    }
    _OE_DEFAULT_HANDLER_;
    
    return TRUE;
}

LRESULT COETemplatesPage::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CPropertyPage::WindowProc(message, wParam, lParam);
    }
    _OE_DEFAULT_HANDLER_;

    return 0;
}

void COETemplatesPage::OnBnClicked_AddTempl()
{
    if (m_templateList.GetItemCount())
    {
        int index = getCurrentSelection();
        m_templateList.EnsureVisible(findItem(index), FALSE);
    }

    Template::Entry entry;
    Template::GenUUID(entry);
    if (!m_newTemplateText.empty())
        swap(entry.text, m_newTemplateText);

    if (COETemplatesDlg(this, COETemplatesDlg::emInsert, *m_currTemplate, entry, m_textAttr).DoModal() == IDOK)
    {
        m_currTemplate->AppendEntry(entry);
        m_templateList.OnAppendEntry();
        m_templateList.Resort();
    }
}

void COETemplatesPage::OnBnClicked_EditTempl()
{
    int index = getCurrentSelection();
    m_templateList.EnsureVisible(findItem(index), FALSE);
    Template::Entry entry = m_currTemplate->GetEntry(index);
    if (COETemplatesDlg(this, COETemplatesDlg::emEdit, *m_currTemplate, entry, m_textAttr).DoModal() == IDOK)
    {
        m_currTemplate->UpdateEntry(index, entry);
        m_templateList.OnUpdateEntry(index);
        m_templateList.Resort();
    }
}

void COETemplatesPage::OnBnClicked_DelTempl()
{
    int index = getCurrentSelection();
    m_templateList.EnsureVisible(findItem(index), FALSE);
    Template::Entry entry = m_currTemplate->GetEntry(index);
    if (COETemplatesDlg(this, COETemplatesDlg::emDelete, *m_currTemplate, entry, m_textAttr).DoModal() == IDOK)
    {
        m_currTemplate->DeleteEntry(index);
        m_templateList.OnDeleteEntry(index);
    }
}

void COETemplatesPage::OnNMDblclk_TemplList (NMHDR*, LRESULT *pResult)
{
    OnBnClicked_EditTempl();
    *pResult = 0;
}

void COETemplatesPage::OnCbnSelchange_Category()
{
    try { EXCEPTION_FRAME;

        UpdateData();

        m_templateList.DeleteAllItems();
        m_currTemplate = m_manager.GetTemplateCollection()
            .Find(string(m_templateName));

        m_adapter.SetData(m_currTemplate);
        if (!m_templateListInitialized)
        {
            m_templateList.Init();
            m_templateListInitialized = true;
        }
        m_templateList.Refresh();
        
        m_textAttr = m_manager.FindByName((LPCSTR)m_templateName)
            ->GetVisualAttributesSet().FindByName("Text");

        CheckDlgButton(IDC_OET_LIST_IF_ALTERNATIVE, 
            m_currTemplate->GetAlwaysListIfAlternative() ? BST_CHECKED : BST_UNCHECKED);
    }
    _OE_DEFAULT_HANDLER_;
}

void COETemplatesPage::OnBnClicked_ListIfAlternative()
{
    m_currTemplate->SetAlwaysListIfAlternative(
        IsDlgButtonChecked(IDC_OET_LIST_IF_ALTERNATIVE) ? true : false);
}
