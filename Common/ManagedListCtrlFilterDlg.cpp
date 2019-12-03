#include "stdafx.h"
#include "ManagedListCtrlFilterDlg.h"

namespace Common
{

CManagedListCtrlFilterDlg::CManagedListCtrlFilterDlg(CWnd* pParent, const CRect& colHeaderRect, 
    ListCtrlManager& listCtrlManager, int filterColumn)
: CDialog(CManagedListCtrlFilterDlg::IDD, pParent),
m_colHeaderRect(colHeaderRect),
m_listCtrlManager(listCtrlManager),
m_filterColumn(filterColumn),
m_operation(0)
{
    listCtrlManager.GetFilter(m_filter);
    m_value = m_filter.at(m_filterColumn).value.c_str();
}

CManagedListCtrlFilterDlg::~CManagedListCtrlFilterDlg()
{
}

void CManagedListCtrlFilterDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_MLF_LIST, m_valueList);
    DDX_Control(pDX, IDC_MLF_OPERATION, m_operationList);
    DDX_Control(pDX, IDC_MLF_COL_LIST, m_columnList);
    DDX_Text(pDX, IDC_MLF_LIST, m_value);
    DDX_CBIndex(pDX, IDC_MLF_OPERATION, m_operation);
}

BEGIN_MESSAGE_MAP(CManagedListCtrlFilterDlg, CDialog)
    ON_CBN_DBLCLK(IDC_MLF_LIST, OnCbnDblclk_List)
    ON_CBN_EDITCHANGE(IDC_MLF_LIST, OnCbnEditChange_List)
    ON_CBN_SELCHANGE(IDC_MLF_LIST, OnCbnSelChange_List)
    ON_BN_CLICKED(IDC_MLF_CLEAR, OnBnClicked_Clear)
    ON_BN_CLICKED(IDC_MLF_CLEAR_ALL, OnBnClicked_ClearAll)
    ON_CBN_SELCHANGE(IDC_MLF_COL_LIST, OnCbnSelChange_ColList)
    ON_BN_CLICKED(IDC_MLF_APPLY, OnBnClicked_Apply)
END_MESSAGE_MAP()

BOOL CManagedListCtrlFilterDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    try { EXCEPTION_FRAME;

        m_filterImage.Create(IDB_MLIST_FILTER, 12, 20, RGB(0,255,0));
        m_columnList.SetImageList(&m_filterImage);

        m_operationList.AddString("Contains");
        m_operationList.AddString("Starts with");
        m_operationList.AddString("Exactly matches");
        m_operationList.SetCurSel(0);

        std::vector<std::string> headers; 
        m_listCtrlManager.GetColumnHeaders(headers);

        COMBOBOXEXITEM item;
        memset(&item, 0, sizeof(item));
        item.mask = CBEIF_TEXT;
        item.mask |= CBEIF_IMAGE|CBEIF_SELECTEDIMAGE;

        std::vector<std::string>::const_iterator it = headers.begin();
        for (; it != headers.end(); ++it)
        {
            item.iSelectedImage = item.iImage = !item.iItem ? 0 : 1;
            item.pszText = (LPSTR)it->c_str();
            m_columnList.InsertItem(&item);
            ++item.iItem;
        }

        m_columnList.SetCurSel(m_filterColumn);
        OnCbnSelChange_ColList();

        CRect rc;
        m_listCtrlManager.GetListCtrl().GetWindowRect(rc);
        SetWindowPos(0, rc.left + 40, rc.top + 80, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
    }
    _COMMON_DEFAULT_HANDLER_

    return TRUE;
}

void CManagedListCtrlFilterDlg::OnCbnDblclk_List()
{
    OnOK();
}

void CManagedListCtrlFilterDlg::OnCbnEditChange_List()
{
    try { EXCEPTION_FRAME;

        CString buff;
        m_valueList.GetWindowText(buff);
        
        COMBOBOXEXITEM item;
        memset(&item, 0, sizeof(item));
        item.mask |= CBEIF_IMAGE|CBEIF_SELECTEDIMAGE;
        item.iSelectedImage = item.iImage = !buff.IsEmpty() ? 0 : 1;
        item.iItem = m_filterColumn;
        m_columnList.SetItem(&item);
        m_columnList.Invalidate();
    }
    _COMMON_DEFAULT_HANDLER_
}

void CManagedListCtrlFilterDlg::OnCbnSelChange_List()
{
    try { EXCEPTION_FRAME;

        CString buff;

        int inx = m_valueList.GetCurSel();
        if (inx != -1)
            m_valueList.GetLBText(inx, buff);
        
        COMBOBOXEXITEM item;
        memset(&item, 0, sizeof(item));
        item.mask |= CBEIF_IMAGE|CBEIF_SELECTEDIMAGE;
        item.iSelectedImage = item.iImage = !buff.IsEmpty() ? 0 : 1;
        item.iItem = m_filterColumn;
        m_columnList.SetItem(&item);
        m_columnList.Invalidate();
    }
    _COMMON_DEFAULT_HANDLER_
}

void CManagedListCtrlFilterDlg::OnBnClicked_Clear()
{
    try { EXCEPTION_FRAME;

        m_filter.at(m_filterColumn).value.clear();
        m_value.Empty();
        UpdateData(FALSE);

        updateColumnListIcons();
    }
    _COMMON_DEFAULT_HANDLER_
}

void CManagedListCtrlFilterDlg::OnBnClicked_ClearAll()
{
    try { EXCEPTION_FRAME;

        ListCtrlManager::FilterCollection::iterator it = m_filter.begin();
        for (; it != m_filter.end(); ++it)
            it->value.clear();
        
        m_value.Empty();
        UpdateData(FALSE);

        updateColumnListIcons();
    }
    _COMMON_DEFAULT_HANDLER_
}

void CManagedListCtrlFilterDlg::OnCbnSelChange_ColList()
{
    try { EXCEPTION_FRAME;

        UpdateData(TRUE);
        m_filter.at(m_filterColumn).value = m_value;
        m_filter.at(m_filterColumn).operation 
            = static_cast <Common::ListCtrlManager::EFilterOperation>(m_operation);
        
        int col = m_columnList.GetCurSel();
        m_valueList.ResetContent();

        std::set<std::string> values; 
        m_listCtrlManager.GetColumnValues(col, values);

        std::set<std::string>::const_iterator it = values.begin();
        for (; it != values.end(); ++it)
            m_valueList.AddString(it->c_str());

        m_value = m_filter.at(col).value.c_str();
        m_operation = m_filter.at(col).operation;
        UpdateData(FALSE);

        int inx = m_valueList.FindStringExact(0, m_filter.at(col).value.c_str());
        if (inx != -1) m_valueList.SetCurSel(inx);

        m_filterColumn = col;

        updateColumnListIcons();
    }
    _COMMON_DEFAULT_HANDLER_
}

void CManagedListCtrlFilterDlg::updateColumnListIcons ()
{
    COMBOBOXEXITEM item;
    memset(&item, 0, sizeof(item));
    item.mask |= CBEIF_IMAGE|CBEIF_SELECTEDIMAGE;
    ListCtrlManager::FilterCollection::iterator it = m_filter.begin();
    for (; it != m_filter.end(); ++it)
    {
        item.iSelectedImage = item.iImage = !it->value.empty() ? 0 : 1;
        m_columnList.SetItem(&item);
        ++item.iItem;
    }
    m_columnList.Invalidate();
}

void CManagedListCtrlFilterDlg::OnBnClicked_Apply ()
{
    try { EXCEPTION_FRAME;

        UpdateData(TRUE);
        m_filter.at(m_filterColumn).value = m_value;
        m_filter.at(m_filterColumn).operation 
            = static_cast <Common::ListCtrlManager::EFilterOperation>(m_operation);
        m_listCtrlManager.SetFilter(m_filter);
    }
    _COMMON_DEFAULT_HANDLER_
}

void CManagedListCtrlFilterDlg::OnOK()
{
    try { EXCEPTION_FRAME;

        CDialog::OnOK();
        m_filter.at(m_filterColumn).value = m_value;
        m_filter.at(m_filterColumn).operation 
            = static_cast <Common::ListCtrlManager::EFilterOperation>(m_operation);
        m_listCtrlManager.SetFilter(m_filter);
    }
    _COMMON_DEFAULT_HANDLER_
}

}//namespace Common


