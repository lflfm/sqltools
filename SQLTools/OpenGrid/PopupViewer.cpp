/*
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2017 Aleksey Kochetov

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

/*
2017-12-10 Popuip Viewer is based on the code from sqltools++ by Randolf Geist
*/

#include "stdafx.h"
#include "resource.h"
#include "PopupViewer.h"
#include "COMMON\VisualAttributes.h"
#include "COMMON/GUICommandDictionary.h"
#include "GridView.h"

#include <MultiMon.h>
extern "C" {
    extern BOOL InitMultipleMonitorStubs(void);
    extern HMONITOR (WINAPI* g_pfnMonitorFromWindow)(HWND, DWORD);
    extern BOOL     (WINAPI* g_pfnGetMonitorInfo)(HMONITOR, LPMONITORINFO);
    extern HMONITOR (WINAPI* g_pfnMonitorFromRect)(LPCRECT, DWORD);
};

using namespace Common;

namespace OG2 /* OG = OpenGrid V2 */
{
///////////////////////////////////////////////////////////////////////////////
// GridPopupEdit
GridPopupEdit::GridPopupEdit()
{
	m_accelTable = 0;
}

GridPopupEdit::~GridPopupEdit()
{
	if (m_accelTable)
		DestroyAcceleratorTable(m_accelTable);
}

BEGIN_MESSAGE_MAP(GridPopupEdit, CEdit)
	ON_WM_CREATE()
	ON_WM_CONTEXTMENU()
    ON_WM_INITMENUPOPUP()
    ON_WM_KEYDOWN()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
    ON_COMMAND(ID_GRIDPOPUP_CLOSE, OnGridPopupClose)
	ON_COMMAND(ID_GRIDPOPUP_WORDWRAP, OnGridPopupWordWrap)
END_MESSAGE_MAP()

// GridPopupEdit message handlers

void GridPopupEdit::OnGridPopupWordWrap()
{
    /* Stupid windows, changing these styles just does not work, you need to recreate the control...*/
    ((PopupFrameWnd *)GetParent())->OnGridPopupWordWrap();
}

void GridPopupEdit::OnEditCopy()
{
	int nStart, nEnd;
	CEdit::GetSel(nStart, nEnd);

	if (nStart >= nEnd)
		OnEditSelectAll();

	CEdit::Copy();
}

void GridPopupEdit::OnEditSelectAll()
{
	CEdit::SetSel(0, -1, TRUE);
}

int GridPopupEdit::OnCreate (LPCREATESTRUCT lpCreateStruct)
{
    int retval = CEdit::OnCreate(lpCreateStruct);
		
    VisualAttribute attr;

    /*const VisualAttributesSet& set = ((COEditorView *) ((CMDIMainFrame *)AfxGetMainWnd())->GetActiveView())->GetEditorSettings().GetVisualAttributesSet();
    const VisualAttribute& textAttr = set.FindByName("Text");

    SetFont(textAttr.NewFont());
    */

    // TODO: add font to settings
    if (!m_Font.m_hObject)
    {
        m_Font.CreateFont(
              -attr.PointToPixel(9), 0, 0, 0,
              FW_NORMAL,
              0,
              0,
              0, ANSI_CHARSET,//DEFAULT_CHARSET,
              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
              "Courier New"
            );
    }

	SetFont(&m_Font);

    CMenu menu;
    VERIFY(menu.LoadMenu(IDR_GRIDPOPUP_OPTIONS));
    CMenu* pPopup = menu.GetSubMenu(0);
    ASSERT(pPopup != NULL);
    Common::GUICommandDictionary::AddAccelDescriptionToMenu(pPopup->m_hMenu);

	m_accelTable = Common::GUICommandDictionary::GetMenuAccelTable(pPopup->m_hMenu);

    return retval;
}

void GridPopupEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    switch (nChar)
    {
    case VK_ESCAPE:
        OnGridPopupClose();
        return;
        break;
    }

    CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL GridPopupEdit::PreTranslateMessage(MSG* pMsg)
{
	if (m_accelTable)
		if (TranslateAccelerator(m_hWnd, m_accelTable, pMsg) == 0)
			return CEdit::PreTranslateMessage(pMsg);
		else
			return true;
	else
		return CEdit::PreTranslateMessage(pMsg);
}

void GridPopupEdit::OnContextMenu (CWnd* , CPoint pos)
{
    CMenu menu;
    VERIFY(menu.LoadMenu(IDR_GRIDPOPUP_OPTIONS));
    CMenu* pPopup = menu.GetSubMenu(0);
    ASSERT(pPopup != NULL);
    Common::GUICommandDictionary::AddAccelDescriptionToMenu(pPopup->m_hMenu);

    const int MENU_ITEM_TEXT_SIZE   = 80;
    const int MENUITEMINFO_OLD_SIZE = 44;

    MENUITEMINFO mii;
    memset(&mii, 0, sizeof mii);
    mii.cbSize = MENUITEMINFO_OLD_SIZE;  // 07.04.2003 bug fix, no menu shortcut labels on Win95,... because of SDK incompatibility
    mii.fMask  = MIIM_SUBMENU | MIIM_DATA | MIIM_ID | MIIM_TYPE;

    char buffer[MENU_ITEM_TEXT_SIZE];
    mii.dwTypeData = buffer;
    mii.cch = sizeof buffer;

    if (pPopup->GetMenuItemInfo(ID_GRIDPOPUP_CLOSE, &mii))
    {
        string menuText = buffer;

        if (menuText.find('\t') != string::npos)
            menuText += "; ";
        else
            menuText += "\t";

        menuText += "ESC";

        strcpy(buffer, menuText.c_str());

        mii.cch = menuText.size() + 1;

        pPopup->SetMenuItemInfo(ID_GRIDPOPUP_CLOSE, &mii);
    }

    pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pos.x, pos.y, this);
}

void GridPopupEdit::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    CEdit::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    pPopupMenu->CheckMenuItem(ID_GRIDPOPUP_WORDWRAP, MF_BYCOMMAND|((((PopupFrameWnd *)GetParent())->GetEditorWordWrap()) ? MF_CHECKED : MF_UNCHECKED));
}

void GridPopupEdit::OnGridPopupClose()
{
    GetParent()->SendMessage(WM_CLOSE);
}

///////////////////////////////////////////////////////////////////////////////
// PopupFrameWnd

void HtmlView::LoadHTMLfromString (const string& html)
{
    HRESULT hr;
    void* pHTML = NULL;
    IDispatch* pHTMLDoc = NULL;
    IStream* pHTMLStream = NULL;
    IPersistStreamInit* pPersistStreamInit = NULL;

    HGLOBAL hHTML = GlobalAlloc(GPTR, html.size());
    pHTML = GlobalLock(hHTML);
    memcpy(pHTML, html.c_str(), html.size());
    GlobalUnlock(hHTML);

    hr = CreateStreamOnHGlobal(hHTML, TRUE, &pHTMLStream);
    if (SUCCEEDED(hr))
    {
        // Get the underlying document.
        m_pBrowserApp->get_Document(&pHTMLDoc);
        if (!pHTMLDoc)
        {
            // Ensure we have a document.
            Navigate2(_T("about:blank"), NULL, NULL, NULL, NULL);
            m_pBrowserApp->get_Document(&pHTMLDoc);
        }

        if (pHTMLDoc)
        {
            hr = pHTMLDoc->QueryInterface( IID_IPersistStreamInit, (void**)&pPersistStreamInit );
            if (SUCCEEDED(hr))
            {
                hr = pPersistStreamInit->InitNew();
                if (SUCCEEDED(hr))
                {
                    // Load the HTML from the stream.
                    hr = pPersistStreamInit->Load(pHTMLStream);
                }

                pPersistStreamInit->Release();
            }

            pHTMLDoc->Release();
        }

        pHTMLStream->Release();
    }

    return;
}

BEGIN_MESSAGE_MAP(HtmlView, CHtmlView)
    ON_WM_KEYDOWN()
END_MESSAGE_MAP()

// GridPopupEdit message handlers

void HtmlView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    switch (nChar)
    {
    case VK_ESCAPE:
        GetParent()->SendMessage(WM_CLOSE);
        return;
    }

    __super::OnKeyDown(nChar, nRepCnt, nFlags);
}

///////////////////////////////////////////////////////////////////////////////
// PopupFrameWnd

const char* cszSection = "GridPopup";
const char* cszWP = "WP";
const char* cszTab = "Tab";
const char* cszWordWrap = "WordWrap";

PopupFrameWnd*  PopupFrameWnd::m_theOne = 0;
CWnd*           PopupFrameWnd::m_lastClient = 0;
WINDOWPLACEMENT PopupFrameWnd::m_lastWindowPlacement = { 0 };
int             PopupFrameWnd::m_currentTab = 0;
bool            PopupFrameWnd::m_bWordWrap = false;

IMPLEMENT_DYNCREATE(PopupFrameWnd, CFrameWnd)

PopupFrameWnd::PopupFrameWnd()
{
}

PopupFrameWnd::~PopupFrameWnd()
{
    m_theOne = 0;
}

void PopupFrameWnd::ShowPopup (CWnd* client, const string& text)
{
    m_lastClient = client;

    if (!m_theOne)
    {
        if (!m_lastWindowPlacement.length) // if it is 0 then this is the first time for the instance
            m_bWordWrap = AfxGetApp()->GetProfileInt(cszSection, cszWordWrap, (int)m_bWordWrap) ? true: false;

        m_theOne = new PopupFrameWnd;
        
        CRect rect;
        {
            CRect rc;
            // lets make sure that the viewer is visiable on any monitor
            if (g_pfnMonitorFromWindow)
            {
                HMONITOR hmonitor = g_pfnMonitorFromWindow(*client, MONITOR_DEFAULTTONEAREST);
                MONITORINFO monitorInfo;
                memset(&monitorInfo, 0, sizeof(monitorInfo));
                monitorInfo.cbSize = sizeof(monitorInfo);

                if (g_pfnGetMonitorInfo(hmonitor, &monitorInfo))
                {
                    rc = monitorInfo.rcMonitor;
                }
                else
                    GetDesktopWindow()->GetWindowRect(rc);
            }
            else
                GetDesktopWindow()->GetWindowRect(rc);

            rect.left += rc.left + (rc.right - rc.left)/ 2;
            rect.top  += rc.top + 100;
            rect.right = rc.right - 100;
            rect.bottom = rect.top + (rc.bottom - rc.top) / 2;
        }

        DWORD style = WS_POPUPWINDOW|WS_CAPTION|WS_THICKFRAME|WS_MAXIMIZEBOX|WS_MINIMIZEBOX;
        
        LPCSTR pszClassName = AfxRegisterWndClass(CS_VREDRAW | CS_HREDRAW,
          ::LoadCursor(NULL, IDC_ARROW),
          (HBRUSH) ::GetStockObject(WHITE_BRUSH),
          AfxGetApp()->LoadIcon(IDR_MAINFRAME)
        );

        if (!m_theOne->CreateEx(0, pszClassName, "Grid Popup Viewer", style, rect, AfxGetMainWnd(), 0)) 
        {
            delete m_theOne;
            m_theOne = 0;
            AfxMessageBox("Failed to create \"Grid Popup Viewer\"!", MB_OK|MB_ICONERROR);
            AfxThrowUserException();
	    }

        if (!m_lastWindowPlacement.length) // if it is 0 then this is the first time for the instance
        {
            m_currentTab = AfxGetApp()->GetProfileInt(cszSection, cszTab, m_currentTab);
            m_currentTab = min(m_currentTab, 1);
            m_currentTab = max(0, m_currentTab);

            // let' try to restore the previous state from the profile
            // if we can check visibility of the old postion
            if (g_pfnMonitorFromRect)
            {
                BYTE *pWP;
                UINT size;
                if (AfxGetApp()->GetProfileBinary(cszSection, cszWP, &pWP, &size)
                && size == sizeof(WINDOWPLACEMENT))
                {
                    if (g_pfnMonitorFromRect(&((WINDOWPLACEMENT*)pWP)->rcNormalPosition, MONITOR_DEFAULTTONULL) != NULL)
                    {
                        m_lastWindowPlacement = *(WINDOWPLACEMENT*)pWP;
	                    delete []pWP;
                    }
                }
            }
        }

        if (!m_currentTab)
            m_theOne->OnGridPopupText();
        else
            m_theOne->OnGridPopupHtml();

        m_theOne->ResoreLastWindowPlacement();
    }

    m_theOne->SetPopupText(text);
    m_theOne->ShowWindow(SW_SHOW);
    m_theOne->SetFocus();
}

void PopupFrameWnd::DestroyPopup (CWnd* client)
{
    if (m_theOne && m_lastClient == client)
    {
        if (m_theOne->m_hWnd)
            m_theOne->OnClose();
        delete m_theOne;
        m_theOne = 0;
    }
}

void PopupFrameWnd::SetEditorWordWrap (bool bWordWrap)
{
    m_bWordWrap = bWordWrap;

    CString theText;
    CRect theRect = CFrameWnd::rectDefault;

    m_EditBox.GetWindowText(theText);
    m_EditBox.GetWindowRect(theRect);
    ScreenToClient(theRect);
    m_EditBox.DestroyWindow();

    DWORD nStyle = WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_AUTOVSCROLL|ES_MULTILINE|ES_READONLY|ES_NOHIDESEL;

    if (!m_bWordWrap)
        nStyle |= WS_HSCROLL|ES_AUTOHSCROLL;

    if (!m_EditBox.Create(nStyle, theRect, this, (UINT)IDC_STATIC)) 
    {
        MessageBeep((UINT)-1);
        AfxMessageBox("Failed to re-create the edit control!", MB_OK|MB_ICONSTOP);
	}

    m_EditBox.SetWindowText(theText);
    m_EditBox.SetFocus();
}

void PopupFrameWnd::SetPopupText (const std::string& text)
{ 
    if (m_EditBox.m_hWnd) 
        m_EditBox.SetWindowText(text.c_str()); 

    if (m_hmlViewer.m_hWnd) 
        m_hmlViewer.LoadHTMLfromString(text.c_str());
}

void PopupFrameWnd::ResoreLastWindowPlacement ()
{
    if (m_hWnd && m_lastWindowPlacement.length > 0)
	    SetWindowPlacement((WINDOWPLACEMENT*)&m_lastWindowPlacement);
}

BEGIN_MESSAGE_MAP(PopupFrameWnd, CFrameWnd)
	ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_CLOSE()
    ON_WM_DESTROY()
    ON_WM_SETFOCUS()
	ON_COMMAND(ID_GRIDPOPUP_TEXT,     OnGridPopupText)
	ON_COMMAND(ID_GRIDPOPUP_HTML,     OnGridPopupHtml)
	ON_COMMAND(ID_GRIDPOPUP_WORDWRAP, OnGridPopupWordWrap)
	ON_UPDATE_COMMAND_UI(ID_GRIDPOPUP_WORDWRAP, OnUpdate_GridPopupWordWrap)
END_MESSAGE_MAP()


// PopupFrameWnd message handlers

void PopupFrameWnd::OnSetFocus (CWnd *)
{
    if (m_EditBox.m_hWnd)    
        m_EditBox.SetFocus();
}

    static struct 
    {
        int   iBitmap;   // zero-based index of button image
        int   idCommand; // command to be sent when button pressed
        BYTE  fsState;   // button state--see below
        BYTE  fsStyle;   // button style--see below
        const char* szString;  // label string
    } 
    g_buttons[] =
    {
        {  0, ID_GRIDPOPUP_TEXT,     TBSTATE_ENABLED|TBSTATE_CHECKED, BTNS_GROUP/*|BTNS_SHOWTEXT*/, "Text"  },
        {  1, ID_GRIDPOPUP_HTML,     TBSTATE_ENABLED, BTNS_GROUP/*|BTNS_SHOWTEXT*/, "Html"   },
        { -1,  0, 0, BTNS_SEP, 0 },
        {  2, ID_GRIDPOPUP_WORDWRAP, TBSTATE_ENABLED|TBSTATE_CHECKED, BTNS_CHECK/*|BTNS_SHOWTEXT*/, "Wordwrap"   },
        //{ -1,  0, 0, BTNS_SEP, 0 },
    };

int PopupFrameWnd::OnCreate (LPCREATESTRUCT lpCreateStruct)
{
	int retval = CFrameWnd::OnCreate(lpCreateStruct);

    if (!m_toolbar.CreateEx(this, 0,
                                  WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_SIZE_DYNAMIC | CBRS_FLYBY,
                                  CRect(1, 1, 1, 1),
                                  AFX_IDW_CONTROLBAR_FIRST - 1)
    || !m_toolbar.LoadToolBar(IDT_GRIDPOPUP))
    {
		TRACE0("Failed to create tool bar\n");
        return -1;
    }
    m_toolbar.GetToolBarCtrl().ModifyStyle(0, TBSTYLE_FLAT|TBSTYLE_LIST);

    for (int i = 0; i < sizeof(g_buttons)/sizeof(g_buttons[0]); i++)
    {
        if (g_buttons[i].idCommand)
        {
            m_toolbar.SetButtonInfo(i, g_buttons[i].idCommand, g_buttons[i].fsStyle, g_buttons[i].iBitmap);
            m_toolbar.SetButtonText(i, g_buttons[i].szString);
        }
    }

	// Resize the buttons so that the text will fit.
	CRect rc(0, 0, 0, 0);
	CSize sizeMax(0, 0);
	for (int nIndex = m_toolbar.GetToolBarCtrl().GetButtonCount() - 1; nIndex >= 0; nIndex--)
	{
		m_toolbar.GetToolBarCtrl().GetItemRect(nIndex, rc);
		rc.NormalizeRect();
		sizeMax.cx = __max(rc.Size().cx, sizeMax.cx);
		sizeMax.cy = __max(rc.Size().cy, sizeMax.cy);
	}
	m_toolbar.SetSizes(sizeMax, CSize(16,16));

    EnableDocking(CBRS_ALIGN_TOP);
    m_toolbar.EnableDocking(CBRS_ALIGN_TOP);
    DockControlBar(&m_toolbar);

    DWORD nStyle = WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_AUTOVSCROLL|ES_MULTILINE|ES_READONLY|ES_NOHIDESEL;

    if (!m_bWordWrap)
        nStyle |= WS_HSCROLL|ES_AUTOHSCROLL;

    if (!m_EditBox.Create(nStyle, CFrameWnd::rectDefault, this, (UINT)IDC_STATIC)) 
	    return -1;

    LPCSTR pszClassName = AfxRegisterWndClass(CS_VREDRAW | CS_HREDRAW);
    if (!m_hmlViewer.Create(pszClassName, NULL, WS_CHILD|WS_VISIBLE, CFrameWnd::rectDefault, this, (UINT)IDC_STATIC)) 
	    return -1;

    return retval;
}

void PopupFrameWnd::OnSize(UINT nType, int cx, int cy)
{
    __super::OnSize(nType, cx, cy);

    if (cx > 0 && cy > 0)
    {
        CRect rc;
        m_toolbar.GetWindowRect(rc);

        if (m_EditBox.m_hWnd)
            m_EditBox.SetWindowPos(0, 0, rc.Height(), cx, cy - rc.Height(), SWP_NOZORDER);

        if (m_hmlViewer.m_hWnd)
            m_hmlViewer.SetWindowPos(0, 0, rc.Height(), cx, cy - rc.Height(), SWP_NOZORDER);
    }
}

void PopupFrameWnd::OnClose()
{
    WINDOWPLACEMENT wp;
	wp.length = sizeof wp;

    if (GetWindowPlacement(&wp))
    {
        if (wp.showCmd == SW_MINIMIZE)
        {
            wp.showCmd = SW_SHOWNORMAL;
            wp.flags   = 0;
        }
        AfxGetApp()->WriteProfileBinary(cszSection, cszWP, (LPBYTE)&wp, sizeof(wp));
        m_lastWindowPlacement = wp;
    }

    AfxGetApp()->WriteProfileInt(cszSection, cszTab, m_currentTab);
    AfxGetApp()->WriteProfileInt(cszSection, cszWordWrap, (int)m_bWordWrap);

    __super::OnClose();
}

void PopupFrameWnd::OnDestroy()
{
    __super::OnDestroy();
}

void PopupFrameWnd::OnGridPopupText ()
{
    m_currentTab = 0;
    m_hmlViewer.ShowWindow(SW_HIDE);
    m_EditBox.ShowWindow(SW_SHOW);
    m_toolbar.GetToolBarCtrl().CheckButton(ID_GRIDPOPUP_TEXT);
    m_toolbar.GetToolBarCtrl().EnableButton(ID_GRIDPOPUP_WORDWRAP, TRUE);
    m_toolbar.GetToolBarCtrl().PressButton(ID_GRIDPOPUP_WORDWRAP, m_bWordWrap);
    m_EditBox.SetFocus();
}

void PopupFrameWnd::OnGridPopupHtml ()
{
    m_currentTab = 1;
    m_hmlViewer.ShowWindow(SW_SHOW);
    m_EditBox.ShowWindow(SW_HIDE);
    m_toolbar.GetToolBarCtrl().CheckButton(ID_GRIDPOPUP_HTML);
    m_toolbar.GetToolBarCtrl().EnableButton(ID_GRIDPOPUP_WORDWRAP, FALSE);
    m_toolbar.GetToolBarCtrl().PressButton(ID_GRIDPOPUP_WORDWRAP, m_bWordWrap);
    m_hmlViewer.SetFocus();
}

void PopupFrameWnd::OnGridPopupWordWrap ()
{
    SetEditorWordWrap(!m_bWordWrap);
    //m_toolbar.PressButton(ID_GRIDPOPUP_WORDWRAP, m_EditBox.GetWordWrap());
}

void PopupFrameWnd::OnUpdate_GridPopupWordWrap (CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!m_currentTab);
    pCmdUI->SetCheck(m_bWordWrap);
}

}//namespace OG2
