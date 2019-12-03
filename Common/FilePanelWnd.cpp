/* 
    Copyright (C) 2002 Aleksey Kochetov

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
    16.12.2002 bug fix, file manager context menu fails
    09.03.2003 bug fix, desktop flickers when the programm is starting
    31.03.2003 bug fix, Attempt to open File dialog for a specified folder in File explorer fails 
               if any file alredy open from another location
    27.03.2005 bug fix, wrong message if file or folder does not exist
    04.02.2014 improvement, "File Panel/Open Files" can be a drag & drop source so any file can be dragged 
               and dropped on another application such as SQLTools.
*/

/*
TODO#2: add drag from for history list
TODO#2: remove shared code from OnExplorerTree_NotifyDisplayTooltip  / OnRecentFilesListCtrl_NotifyDisplayTooltip
*/

#include "stdafx.h"
#include <Shlwapi.h>
#include <string>
#include "COMMON/AppUtilities.h"
#include <COMMON/ExceptionHelper.h>
#include <WinException/WinException.h>
#include <COMMON/FilePanelWnd.h>
#include <COMMON/WorkbookMDIFrame.h>
#include "COMMON/GUICommandDictionary.h"
#include "COMMON/AppUtilities.h"
#include "CustomShellContextMenu.h"

#if defined(UNICODE) || defined(_UNICODE)
#error("no unicode support")
#endif

// FilePanelWnd
// TODO:
//      destroy icon test on Win95/Win98
//      history and favorites tabs
//      exception handling
//      add folder to driver list - folder favorites list
//      show/hide command

#define FPW_OPEN_FILES_TAB  0
#define FPW_EXPLORER_TAB    1
#define FPW_HISTORY_TAB     2
#define FPW_NUM_OF_TAB      3

static int ICONS[FPW_NUM_OF_TAB] = { CSIDL_DESKTOP, CSIDL_PERSONAL, CSIDL_RECENT };

#define ID_FPW_OPEN_FILES   1000
#define ID_FPW_DRIVES       1001
#define ID_FPW_EXPLORER     1002
#define ID_FPW_FILTER       1003
#define ID_FPW_HISTORY      1004

#define ACTIVATE_FILE_TIMER 777
#define ACTIVATE_FILE_DELAY 500

static const char* cszSection = "FileManager";
static const char* cszTab     = "ActiveTabV2";
static const char* cszFilter  = "ActiveFilter";
static const char* cszDrive   = "CurrentDrive";

    static BOOL GetVolumeName (LPCSTR rootDirectory, CString& volumeName)
    {
        const int STRSIZE = 80;
        DWORD volumeSerialNumber;           // volume serial number
        DWORD maximumComponentLength;       // maximum file name length
        DWORD fileSystemFlags;              // file system options
        char  fileSystemNameBuffer[STRSIZE];// file system name buffer

        BOOL volInfo = GetVolumeInformation(
            rootDirectory,                  // root directory
            volumeName.GetBuffer(STRSIZE),  // volume name buffer
            STRSIZE,                        // length of name buffer
            &volumeSerialNumber,            // volume serial number
            &maximumComponentLength,        // maximum file name length
            &fileSystemFlags,               // file system options
            fileSystemNameBuffer,           // file system name buffer
            sizeof(fileSystemNameBuffer)    // length of file system name buffer
        );

        volumeName.ReleaseBuffer();

        return volInfo;
    }

    static LPCSTR MakeVolumeLabel (LPCSTR rootDirectory, CString& volumeLabel)
    {
        volumeLabel = rootDirectory;
        volumeLabel.TrimRight('\\');
        
        CString volumeName;
        if (GetVolumeName(rootDirectory, volumeName))
        {
            volumeLabel += "   ";
            volumeLabel += volumeName;
        }

        return volumeLabel;
    }

    inline
    static bool IsFolder (const CString& path)
    {
        return Common::AppIsFolder(path, false/*nothrow*/);
    }

///////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNAMIC(CFilePanelWnd, CTabCtrl)

CFilePanelWnd::CFilePanelWnd()
{
    m_isExplorerInitialized = FALSE;
    m_isDrivesInitialized   = FALSE;
    m_isLabelEditing        = FALSE;

    m_Tooltips           = TRUE;
    m_PreviewInTooltips  = TRUE;
    m_PreviewLines       = 10;
    m_ShellContexMenu    = TRUE;
    m_ShellContexMenuFilter = "(^tsvn.*$)|(^.*cvs.*$)|(^properties$)";

    m_filterNames.Add("All files (*.*)");
    m_filterData .Add("*.*");
    m_selectedFilter = -1;
}

CFilePanelWnd::~CFilePanelWnd()
{
}

void CFilePanelWnd::OnShowControl (BOOL show)
{
    if (show)
        ChangeTab(GetCurSel(), TRUE);
}

void CFilePanelWnd::SelectDrive (const CString& path, BOOL force)
{
    if (!m_hWnd)
    {
        m_curDrivePath = path;
    }
    else
    {
        CWaitCursor wait;
        for (int i(0), count(m_driverPaths.GetSize()); i < count; i++)
        {
            if (!m_driverPaths.ElementAt(i).CompareNoCase(path)
            && (force || m_driverPaths.ElementAt(i).CompareNoCase(m_curDrivePath)))
            {
                CString volumeName;
                // check driver availability
                if (!force && !GetVolumeName(path, volumeName))
                {
                    CString message;
                    message.Format("Cannot select drive \"%s\". Drive/device is not available.", (LPCSTR)path);
                    AfxMessageBox(message, MB_OK|MB_ICONSTOP);
                    SelectDrive(m_curDrivePath, TRUE);
                }
                else
                {
                    try
                    {
                        m_drivesCBox.SetCurSel(i);
                        m_curDrivePath = path;
                        m_explorerTree.DisplayTree(m_curDrivePath, TRUE);
                        m_explorerTree.SetSelPath(m_curDrivePath);
                    } catch (CWinException* x) {   
                        CString message;
                        message.Format("Cannot select drive \"%s\". %s", (LPCSTR)path, (LPCSTR)*x);
                        AfxMessageBox(message, MB_OK|MB_ICONSTOP);
                    }
                }
            }
        }
    }
}


/*
    Network path sample: "\\\\Akocheto-2\\TEMP"
*/
void CFilePanelWnd::DisplayDrivers (BOOL force, BOOL curOnly)
{
    if (force || !m_isDrivesInitialized)
    {
        CWaitCursor wait;

        if (!curOnly) m_drivesCBox.LockWindowUpdate(); // 09.03.2003 bug fix, desktop flickers when the programm is starting
        m_drivesCBox.SetRedraw(FALSE);
        m_drivesCBox.ResetContent();
        char  szDrives[2*1024];
        char* pDrive;

        if (curOnly)
        {
            _CHECK_AND_THROW_(m_curDrivePath.GetLength() < sizeof(szDrives) - 2, 
                "Current driver path is too long.");
            memset(szDrives, 0, sizeof(szDrives));
            memcpy(szDrives, (LPCSTR)m_curDrivePath, m_curDrivePath.GetLength());
        }
        else
        {
            _CHECK_AND_THROW_(GetLogicalDriveStrings(sizeof(szDrives), szDrives), 
                "Cannot get logical driver strings.");
        }

        m_driverPaths.RemoveAll();

        COMBOBOXEXITEM item;
        memset(&item, 0, sizeof(item));

        for (pDrive = szDrives; *pDrive; pDrive += strlen(pDrive) + 1, item.iItem++)
        {
            m_driverPaths.Add(pDrive);

            item.mask = CBEIF_TEXT;
            CString volumeLabel;
            item.pszText = (LPSTR)MakeVolumeLabel(pDrive, volumeLabel);

            SHFILEINFO shFinfo;
            if (SHGetFileInfo(pDrive, 0, &shFinfo, sizeof(shFinfo), SHGFI_ICON|SHGFI_SMALLICON))
            {
                item.mask |= CBEIF_IMAGE|CBEIF_SELECTEDIMAGE;
                item.iSelectedImage = item.iImage = shFinfo.iIcon;
                DestroyIcon(shFinfo.hIcon); // we only need the index from the system image list
            }

            m_drivesCBox.InsertItem(&item);
            
            if (m_curDrivePath == pDrive)
                m_drivesCBox.SetCurSel(item.iItem);
        }

        m_drivesCBox.SetRedraw(TRUE);
        if (!curOnly) m_drivesCBox.UnlockWindowUpdate();

        m_isDrivesInitialized = !curOnly;
    }
}

void CFilePanelWnd::ChangeTab (int nTab, BOOL bForce)
{
    _CHECK_AND_THROW_(nTab < FPW_NUM_OF_TAB, "Invalid index of a file manager tab.");

    if (nTab != GetCurSel())
        SetCurSel(nTab);

    // the window does not receive WM_CLOSE so have to write it here
    AfxGetApp()->WriteProfileInt(cszSection, cszTab, GetCurSel());

    m_openFilesList.ShowWindow(nTab == FPW_OPEN_FILES_TAB ? SW_SHOW : SW_HIDE);
    m_drivesCBox.   ShowWindow(nTab == FPW_EXPLORER_TAB   ? SW_SHOW : SW_HIDE);
    m_explorerTree. ShowWindow(nTab == FPW_EXPLORER_TAB   ? SW_SHOW : SW_HIDE);
    m_filterCBox.   ShowWindow(nTab == FPW_EXPLORER_TAB   ? SW_SHOW : SW_HIDE);
    m_recentFilesListCtrl.ShowWindow(nTab == FPW_HISTORY_TAB ? SW_SHOW : SW_HIDE);

    if (bForce || IsWindowVisible())
    {
        switch (nTab)
        {
        case FPW_EXPLORER_TAB:
            if (!m_isExplorerInitialized)
            {
                CWaitCursor wait;
                UpdateWindow();

                DisplayDrivers(FALSE, TRUE);
                SelectDrive(m_curDrivePath, TRUE);
                m_isExplorerInitialized = TRUE;
            }
            break;
        case FPW_HISTORY_TAB:
            if (!m_recentFilesListCtrl.IsInitialized())
                m_recentFilesListCtrl.Initialize();
            break;
        }
    }
}

void CFilePanelWnd::ActivateOpenFile ()
{
    POSITION pos = m_openFilesList.GetFirstSelectedItemPosition();

    if (pos)
    {
        LVITEM item;
        memset(&item, 0, sizeof(item));
        item.mask = LVIF_PARAM;
        item.iItem = m_openFilesList.GetNextSelectedItem(pos);

        VERIFY(m_openFilesList.GetItem(&item));
        _ASSERT(AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CWorkbookMDIFrame)));
        _ASSERT(((CMDIChildWnd*)item.lParam)->IsKindOf(RUNTIME_CLASS(CMDIChildWnd)));

        ((CWorkbookMDIFrame*)AfxGetMainWnd())->ActivateChild((CMDIChildWnd*)item.lParam);
    }
}

BEGIN_MESSAGE_MAP(CFilePanelWnd, CTabCtrl)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_NOTIFY_REFLECT(TCN_SELCHANGE, OnTab_SelChange)
    ON_CBN_SETFOCUS(ID_FPW_DRIVES, OnDrive_SetFocus)
    ON_CBN_SELCHANGE(ID_FPW_DRIVES, OnDrive_SelChange)

    ON_NOTIFY(NM_CLICK, ID_FPW_OPEN_FILES, OnOpenFiles_Click)
    ON_NOTIFY(NM_RCLICK, ID_FPW_OPEN_FILES, OnOpenFiles_RClick)
    ON_NOTIFY(LVN_KEYDOWN, ID_FPW_OPEN_FILES, OnOpenFiles_KeyDown)
    ON_NOTIFY(LVN_BEGINDRAG, ID_FPW_OPEN_FILES, OnOpenFiles_OnBeginDrag)

    ON_NOTIFY(NM_DBLCLK, ID_FPW_EXPLORER, OnExplorerTree_DblClick)
    ON_NOTIFY(NM_RCLICK, ID_FPW_EXPLORER, OnExplorerTree_RClick)
    ON_NOTIFY(TVN_BEGINLABELEDITA, ID_FPW_EXPLORER, OnExplorerTree_BeginLabelEdit)
    ON_NOTIFY(TVN_BEGINLABELEDITW, ID_FPW_EXPLORER, OnExplorerTree_BeginLabelEdit)
    ON_NOTIFY(TVN_ENDLABELEDITA, ID_FPW_EXPLORER, OnExplorerTree_EndLabelEdit)
    ON_NOTIFY(TVN_ENDLABELEDITW, ID_FPW_EXPLORER, OnExplorerTree_EndLabelEdit)

    ON_NOTIFY(NM_RCLICK, ID_FPW_DRIVES, OnDrivers_RClick)
    ON_WM_TIMER()
    ON_COMMAND(ID_FPW_OPEN, OnFpwOpen)
    ON_COMMAND(ID_FPW_SHELL_OPEN, OnFpwShellOpen)
    ON_COMMAND(ID_FPW_RENAME, OnFpwRename)
    ON_COMMAND(ID_FPW_DELETE, OnFpwDelete)
    ON_COMMAND(ID_FPW_REFRESH, OnFpwRefresh)
    ON_COMMAND(ID_FPW_REFRESH_DRIVERS, OnFpwRefreshDrivers)
	ON_NOTIFY(UDM_TOOLTIP_DISPLAY, NULL, OnNotifyDisplayTooltip)
    ON_CBN_SELCHANGE(ID_FPW_FILTER, OnFilter_SelChange)

    ON_NOTIFY(NM_DBLCLK, ID_FPW_HISTORY, OnRecentFilesListCtrl_DblClick)
    ON_NOTIFY(NM_RCLICK, ID_FPW_HISTORY, OnRecentFilesListCtrl_RClick)
    ON_NOTIFY(LVN_BEGINDRAG, ID_FPW_HISTORY, OnRecentFilesListCtrl_OnBeginDrag)
END_MESSAGE_MAP()


// FilePanelWnd message handlers


int CFilePanelWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CTabCtrl::OnCreate(lpCreateStruct) == -1)
        return -1;

    InsertItem(FPW_OPEN_FILES_TAB, "Open Files");
    InsertItem(FPW_EXPLORER_TAB,   "Explorer");
    InsertItem(FPW_HISTORY_TAB,    "History");

    if (!m_openFilesList.Create(
        WS_CHILD|WS_TABSTOP
        |LVS_REPORT|LVS_SORTASCENDING|LVS_NOCOLUMNHEADER
        |LVS_SHOWSELALWAYS|LVS_SINGLESEL, 
        CRect(0, 0, 0, 0), this, ID_FPW_OPEN_FILES))
    {
            TRACE0("Failed to create open files list\n");
            return -1;
    }

    if (!m_drivesCBox.Create(
        WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST,
        CRect(0, 0, 0, GetSystemMetrics(SM_CYFULLSCREEN)/2), this, ID_FPW_DRIVES))
    {
            TRACE0("Failed to create drives combo box\n");
            return -1;
    }

    if (!m_explorerTree.Create(
        WS_CHILD|WS_TABSTOP
        /*|TVS_LINESATROOT*/|TVS_HASBUTTONS|TVS_HASLINES|TVS_SHOWSELALWAYS|TVS_EDITLABELS, 
        CRect(0, 0, 0, 0), this, ID_FPW_EXPLORER))
    {
        TRACE0("Failed to create explorer tree\n");
        return -1;
    }

    if (!m_filterCBox.Create(
        WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST,
        CRect(0, 0, 0, GetSystemMetrics(SM_CYFULLSCREEN)/2), this, ID_FPW_FILTER))
    {
            TRACE0("Failed to create drives combo box\n");
            return -1;
    }

    if (!m_recentFilesListCtrl.Create(
        WS_CHILD|WS_TABSTOP
        |LVS_REPORT|WS_CHILD|WS_BORDER|WS_GROUP|WS_TABSTOP|LVS_SHOWSELALWAYS,
        CRect(0, 0, 0, 0), this, ID_FPW_HISTORY))
    {
            TRACE0("Failed to create open history list\n");
            return -1;
    }
    
    COMBOBOXEXITEM item;
    memset(&item, 0, sizeof(item));
    for (int i = 0; i < m_filterNames.GetSize() && i < m_filterData.GetSize(); ++i)
    {
        item.iItem = i;
        item.mask = CBEIF_TEXT;
        item.pszText = m_filterNames[i].LockBuffer();

        int pos = 0;
        SHFILEINFO shFinfo;
        CString buff = m_filterData.GetAt(i).Tokenize(";", pos);
        buff.Replace("*", "$$$dummy$$$");
        if (SHGetFileInfo(buff, 0, &shFinfo, sizeof(shFinfo), SHGFI_ICON|SHGFI_SMALLICON|SHGFI_USEFILEATTRIBUTES))
        {
            item.mask |= CBEIF_IMAGE|CBEIF_SELECTEDIMAGE;
            item.iSelectedImage = item.iImage = shFinfo.iIcon;
            DestroyIcon(shFinfo.hIcon); // we only need the index from the system image list
        }

        m_filterCBox.InsertItem(&item);
        m_filterNames[i].ReleaseBuffer();
    }
    
    m_selectedFilter = AfxGetApp()->GetProfileInt(cszSection, cszFilter, 0);
    if (m_selectedFilter != -1 
    && m_selectedFilter < m_filterCBox.GetCount())
    {
        m_filterCBox.SetCurSel(m_selectedFilter);
        m_explorerTree.SetFilter(m_filterData.GetAt(m_selectedFilter));
    }
    else
        m_selectedFilter = -1;

    // insert a column
    m_openFilesList.InsertColumn(0, (LPCSTR)NULL);
    m_openFilesList.SetExtendedStyle(m_openFilesList.GetExtendedStyle()|LVS_EX_FULLROWSELECT);
    m_recentFilesListCtrl.SetExtendedStyle(m_recentFilesListCtrl.GetExtendedStyle()|LVS_EX_FULLROWSELECT);

    ModifyStyleEx(0, WS_EX_CONTROLPARENT, 0);
    m_openFilesList.ModifyStyleEx(0, WS_EX_STATICEDGE/*WS_EX_CLIENTEDGE*/, 0);
    m_explorerTree.ModifyStyleEx(0, WS_EX_STATICEDGE/*WS_EX_CLIENTEDGE*/, 0);
    m_recentFilesListCtrl.ModifyStyleEx(0, WS_EX_STATICEDGE/*WS_EX_CLIENTEDGE*/, 0);

    m_drivesCBox.SetImageList(&m_explorerTree.GetSysImageList());
    m_filterCBox.SetImageList(&m_explorerTree.GetSysImageList());
    m_openFilesList.SetImageList(&m_explorerTree.GetSysImageList(), LVSIL_SMALL);
    m_recentFilesListCtrl.SetImageList(&m_explorerTree.GetSysImageList(), LVSIL_SMALL);

    IMalloc * pShellMalloc = NULL;
    HRESULT hres = SHGetMalloc(&pShellMalloc);
    if (SUCCEEDED(hres))
    {
        SetImageList(&m_explorerTree.GetSysImageList());

        for (int i = 0; i < FPW_NUM_OF_TAB; ++i)
        {
            LPITEMIDLIST ppidl = NULL;
            HRESULT hres = SHGetSpecialFolderLocation(NULL, ICONS[i], &ppidl);
            if (SUCCEEDED(hres))
            {
                SHFILEINFO shFinfo;
                if (SHGetFileInfo((LPCSTR)ppidl, 0, &shFinfo, sizeof(shFinfo), SHGFI_PIDL|SHGFI_ICON|SHGFI_SMALLICON))
                {
                    TCITEM item;
                    item.mask = TCIF_IMAGE;
                    item.iImage = shFinfo.iIcon;
                    SetItem(i, &item);
                    DestroyIcon(shFinfo.hIcon); // we only need the index from the system image list
                }
                pShellMalloc->Free(ppidl);
            }
        }
        pShellMalloc->Release();
    }

    m_explorerStateImageList.Create(IDB_OE_EXPLORER_STATE_LIST, 16, 64, RGB(0,0,255));
    m_explorerTree.SetImageList(&m_explorerStateImageList, TVSIL_STATE);

    // load saved drve
    CString drivePath = AfxGetApp()->GetProfileString(cszSection, cszDrive);
    if (drivePath.IsEmpty())
    {
        std::string path;
        Common::AppGetPath(path);
        PathBuildRoot(drivePath.GetBuffer(MAX_PATH), PathGetDriveNumber(path.c_str()));
        drivePath.ReleaseBuffer();
    }
    m_curDrivePath = drivePath;
    // load saved tab
    int nTab = AfxGetApp()->GetProfileInt(cszSection, cszTab, FPW_EXPLORER_TAB);
    ChangeTab(nTab < FPW_NUM_OF_TAB ? nTab : 0);
    
    m_drivesRClick.SubclassWindow(::GetWindow(m_drivesCBox.m_hWnd, GW_CHILD));
    m_filterRClick.SubclassWindow(::GetWindow(m_filterCBox.m_hWnd, GW_CHILD));

	m_tooltip.Create(this, FALSE);
	m_tooltip.SetBehaviour(PPTOOLTIP_DISABLE_AUTOPOP|PPTOOLTIP_MULTIPLE_SHOW);
	m_tooltip.SetNotify(m_hWnd);
	m_tooltip.AddTool(&m_explorerTree);
    m_tooltip.AddTool(&m_recentFilesListCtrl);
    m_tooltip.SetDirection(PPTOOLTIP_TOPEDGE_LEFT);

    SetTooltips(m_Tooltips);

    return 0;
}

void CFilePanelWnd::OnDestroy ()
{
    // for Win95 compatibility
    m_openFilesList.DestroyWindow();
    m_explorerTree.DestroyWindow();
    m_drivesCBox.DestroyWindow(); 
    m_filterCBox.DestroyWindow();
    m_drivesRClick.DestroyWindow();
    m_filterRClick.DestroyWindow();
    m_recentFilesListCtrl.DestroyWindow();

    CTabCtrl::OnDestroy();
}

void CFilePanelWnd::OnSize(UINT nType, int cx, int cy)
{
    CTabCtrl::OnSize(nType, cx, cy);

    if (cx > 0 && cy > 0
    && nType != SIZE_MAXHIDE && nType != SIZE_MINIMIZED) 
    {
        CRect rect;
        GetClientRect(&rect);
        AdjustRect(FALSE, &rect);
        rect.InflateRect(3, 0);
        
        CRect comboRect;
        m_drivesCBox.GetWindowRect(&comboRect);
        int comboH = comboRect.Height() + 2;

        HDWP hdwp = ::BeginDeferWindowPos(10);
        ::DeferWindowPos(hdwp, m_openFilesList, 0, rect.left, rect.top, 
            rect.Width(), rect.Height(), SWP_NOZORDER);
        ::DeferWindowPos(hdwp, m_drivesCBox, 0, rect.left, rect.top, 
            rect.Width(), rect.Height()/2, SWP_NOZORDER);
        ::DeferWindowPos(hdwp, m_explorerTree, 0, rect.left, 
            rect.top + comboH, rect.Width(), rect.Height() - 2 * comboH, SWP_NOZORDER);
        ::DeferWindowPos(hdwp, m_filterCBox, 0, rect.left, 
            rect.top + rect.Height() - comboH + 2, rect.Width(), rect.Height()/2, SWP_NOZORDER);
        ::DeferWindowPos(hdwp, m_recentFilesListCtrl, 0, rect.left, rect.top, 
            rect.Width(), rect.Height(), SWP_NOZORDER);
        ::EndDeferWindowPos(hdwp);

        m_openFilesList.GetClientRect(rect);
        m_openFilesList.SetColumnWidth(0, rect.Width());
    }
}

void CFilePanelWnd::OnTab_SelChange (NMHDR*, LRESULT *pResult)
{
    ChangeTab(GetCurSel());
    *pResult = 0;
}

// move the following code to refresh command handler
void CFilePanelWnd::OnDrive_SetFocus ()
{
    DisplayDrivers();
}

void CFilePanelWnd::OnDrive_SelChange ()
{
    int inx = m_drivesCBox.GetCurSel();
    _ASSERTE(inx < m_driverPaths.GetSize());

    if (inx < m_driverPaths.GetSize())
        SelectDrive(m_driverPaths.ElementAt(inx), FALSE);

    AfxGetApp()->WriteProfileString(cszSection, cszDrive, GetCurDrivePath());
}

void CFilePanelWnd::DoOpenFile (const string& path, unsigned int item)
{
    if (!path.empty())
    {
        if (::PathFileExists(path.c_str()))
            AfxGetApp()->OpenDocumentFile(path.c_str());
        else if (AfxMessageBox(("The file \"" + path + "\" does not exist.\n\nWould you like to remove it from the history?").c_str()
            , MB_ICONEXCLAMATION|MB_YESNO) == IDYES)
        {
            if (item != -1)
                m_recentFilesListCtrl.RemoveEntry(item);
            else
                m_recentFilesListCtrl.RemoveEntry(path);
        }
    }
}

void CFilePanelWnd::OpenFiles_Append (LVITEM& item)
{
    item.iItem = m_openFilesList.GetItemCount();
    VERIFY(m_openFilesList.InsertItem(&item) != -1);
}


void CFilePanelWnd::OpenFiles_UpdateByParam (LPARAM param, LVITEM& item)
{
    int nItem = OpenFiles_FindByParam(param);

    ASSERT(nItem != -1);

    if (nItem != -1)
    {
        // sorting does not work for update so ...
        VERIFY(m_openFilesList.DeleteItem(nItem));
        VERIFY(m_openFilesList.InsertItem(&item) != -1);
    }
}


void CFilePanelWnd::OpenFiles_RemoveByParam (LPARAM param)
{
    int nItem = OpenFiles_FindByParam(param);

    ASSERT(nItem != -1);

    if (nItem != -1)
        VERIFY(m_openFilesList.DeleteItem(nItem));
}


void CFilePanelWnd::OpenFiles_ActivateByParam (LPARAM param)
{
    int nItem = OpenFiles_FindByParam(param);

    ASSERT(nItem != -1);

    if (nItem != -1)
        VERIFY(m_openFilesList.SetItemState(nItem, LVIS_SELECTED, LVIS_SELECTED));
}


LPARAM CFilePanelWnd::OpenFiles_GetCurSelParam ()
{
    int nItem = m_openFilesList.GetNextItem(-1, LVNI_SELECTED);

    if (nItem != -1)
    {
        LVITEM item;
        item.mask = LVIF_PARAM;
        item.iItem = nItem;
        VERIFY(m_openFilesList.GetItem(&item));
        return item.lParam;
    }

    return 0;
}


int CFilePanelWnd::OpenFiles_FindByParam (LPARAM param)
{
    LVITEM item;
    memset(&item, 0, sizeof(item));
    item.mask = LVIF_PARAM|LVIF_TEXT;
    int nItems = m_openFilesList.GetItemCount();

    for (item.iItem = 0; item.iItem < nItems; item.iItem++) 
    {
        VERIFY(m_openFilesList.GetItem(&item));

        if (item.lParam == param)
            return item.iItem;
    }

    return -1;
}

void CFilePanelWnd::OnOpenFiles_Click (NMHDR*, LRESULT* pResult)
{
    ActivateOpenFile();
    *pResult = 0;
}

void CFilePanelWnd::OnOpenFiles_KeyDown (NMHDR*, LRESULT* pResult)
{
    SetTimer(ACTIVATE_FILE_TIMER, ACTIVATE_FILE_DELAY, 0);
    *pResult = 0;
}

BOOL CFilePanelWnd::GetActiveDocumentPath (NMITEMACTIVATE* pItem, CString& path)
{
    LVITEM item;
    memset(&item, 0, sizeof(item));
    item.mask = LVIF_PARAM;
    item.iItem = pItem->iItem;
    VERIFY(m_openFilesList.GetItem(&item));
    ASSERT(AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CWorkbookMDIFrame)));
    ASSERT(((CMDIChildWnd*)item.lParam)->IsKindOf(RUNTIME_CLASS(CMDIChildWnd)));
    path = ((CMDIChildWnd*)item.lParam)->GetActiveDocument()->GetPathName();

    return !path.IsEmpty();
}

#include <afxole.h>

void CFilePanelWnd::OnOpenFiles_OnBeginDrag (NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pItem = (NMITEMACTIVATE*)pNMHDR;
    
    if (pItem && pItem->iItem != -1)
    {
        CString path;

        if (GetActiveDocumentPath(pItem, path))
        {
            //MessageBox(path);
            UINT uBuffSize = sizeof(DROPFILES) + path.GetLength() + 1 + 1;
            if (HGLOBAL hgDrop = GlobalAlloc(GHND|GMEM_SHARE, uBuffSize))
            {
                if (DROPFILES*  pDrop = (DROPFILES*)GlobalLock(hgDrop))
                {
                    memset(pDrop, 0, uBuffSize);
                    pDrop->pFiles = sizeof(DROPFILES);
                    memcpy((LPBYTE(pDrop) + sizeof(DROPFILES)), (LPCTSTR)path, path.GetLength());

                    GlobalUnlock(hgDrop);

                    COleDataSource data;
                    FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
                    data.CacheGlobalData ( CF_HDROP, hgDrop, &etc );

                    /*DROPEFFECT dwEffect  =*/ data.DoDragDrop( DROPEFFECT_COPY );
                }
            }
        }
    }

    *pResult = 0;
}

void CFilePanelWnd::OnOpenFiles_RClick (NMHDR* pNMHDR, LRESULT* pResult)
{
    ActivateOpenFile();

    NMITEMACTIVATE* pItem = (NMITEMACTIVATE*)pNMHDR;
    if (pItem && pItem->iItem != -1)
    {
        *pResult = 1;
        CPoint point(pItem->ptAction);
        m_openFilesList.ClientToScreen(&point);

        CMenu menu;
        VERIFY(menu.LoadMenu(IDR_OE_WORKBOOK_POPUP));
        CMenu* pPopup = menu.GetSubMenu(0);

        ASSERT(pPopup != NULL);
        ASSERT_KINDOF(CFrameWnd, AfxGetMainWnd());
        Common::GUICommandDictionary::AddAccelDescriptionToMenu(pPopup->m_hMenu);

        CString path;
        if (!m_ShellContexMenu || !GetActiveDocumentPath(pItem, path) || !PathFileExists(path))
        {
            pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, AfxGetMainWnd());
        }
        else
        {
            AfxGetMainWnd()->SendMessage(WM_INITMENUPOPUP, (WPARAM)pPopup->m_hMenu, 0);
            if (UINT idCommand = CustomShellContextMenu(this, point, path, pPopup, m_ShellContexMenuFilter).ShowContextMenu())
		        AfxGetMainWnd()->SendMessage(WM_COMMAND, idCommand, 0);
        }
    }
    else
        *pResult = 0;
}

void CFilePanelWnd::OnTimer (UINT nIDEvent)
{
    if (nIDEvent == ACTIVATE_FILE_TIMER)
    {
        BOOL active = GetFocus() == &m_openFilesList ? TRUE : FALSE;
        ActivateOpenFile();
        if (active) 
            m_openFilesList.SetFocus();
    }

    CTabCtrl::OnTimer(nIDEvent);
}

void CFilePanelWnd::OnExplorerTree_DblClick (NMHDR*, LRESULT* pResult)
{
    HTREEITEM hCurSel = m_explorerTree.GetNextItem(TVI_ROOT, TVGN_CARET);

    if (hCurSel)
    {
        CPoint point;
        ::GetCursorPos(&point);
        m_explorerTree.ScreenToClient(&point);
        if (m_explorerTree.HitTest(point, 0) == hCurSel)
        {
            CString path = m_explorerTree.GetFullPath(hCurSel);
            if (!IsFolder(path) || (0xFF00 & GetKeyState(VK_CONTROL)))
            {
                PostMessage(WM_COMMAND, ID_FPW_OPEN);
                *pResult = 1;
                return;
            }
        }
    }

    *pResult = 0;
}

void CFilePanelWnd::OnExplorerTree_RClick (NMHDR*, LRESULT* pResult)
{
    UINT uFlags;
    CPoint point;
    ::GetCursorPos(&point);
    m_explorerTree.ScreenToClient(&point);
    HTREEITEM hItem = m_explorerTree.HitTest(point, &uFlags);
    
    if (hItem && TVHT_ONITEM & uFlags)
    {
        *pResult = 1;

        if (m_explorerTree.GetNextItem(TVI_ROOT, TVGN_CARET) != hItem)
            m_explorerTree.SelectItem(hItem);

        CMenu menu;
        VERIFY(menu.LoadMenu(IDR_OE_EXPLORER_POPUP));
        CMenu* pPopup = menu.GetSubMenu(0);

        CString path = m_explorerTree.GetFullPath(hItem);
        if (!path.IsEmpty() && path.Right(1) == ":") path += '\\';
        BOOL isFolder = IsFolder(path);
        if (!isFolder) pPopup->SetDefaultItem(ID_FPW_OPEN);
        pPopup->ModifyMenu(ID_FPW_OPEN, MF_BYCOMMAND, ID_FPW_OPEN, !isFolder ? "Open File\tDblClick" : "Open File Dalog...\tCtrl+DblClick");
        pPopup->EnableMenuItem(ID_FPW_REFRESH, isFolder ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);

        ASSERT(pPopup != NULL);
        m_explorerTree.ClientToScreen(&point);

        if (!m_ShellContexMenu || !::PathFileExists(path))
        {
            pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
        }
        else
        {
            if (UINT idCommand = CustomShellContextMenu(this, point, path, pPopup, m_ShellContexMenuFilter).ShowContextMenu())
		        SendMessage(WM_COMMAND, idCommand, 0);
        }
    }
    else
        *pResult = 0;
}

void CFilePanelWnd::OnDrivers_RClick (NMHDR*, LRESULT* pResult)
{
    CPoint point;
    ::GetCursorPos(&point);

    CMenu menu;
    VERIFY(menu.LoadMenu(IDR_OE_DRIVERS_POPUP));
    CMenu* pPopup = menu.GetSubMenu(0);
    ASSERT(pPopup != NULL);
    pPopup->SetDefaultItem(ID_FPW_REFRESH_DRIVERS);
    pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);

    *pResult = 1;
}

void CFilePanelWnd::OnFpwOpen ()
{
    HTREEITEM hCurSel = m_explorerTree.GetNextItem(TVI_ROOT, TVGN_CARET);
    if (hCurSel)
    {
        CString path = m_explorerTree.GetFullPath(hCurSel);
        AfxGetApp()->OpenDocumentFile(path);
    }
}

void CFilePanelWnd::OnFpwShellOpen ()
{
    HTREEITEM hCurSel = m_explorerTree.GetNextItem(TVI_ROOT, TVGN_CARET);
    if (hCurSel)
    {
        CString path = m_explorerTree.GetFullPath(hCurSel);

		HINSTANCE hResult = ::ShellExecute(NULL, "open", path, NULL, NULL, SW_SHOWNORMAL);
    
        if ((UINT)hResult <= HINSTANCE_ERROR)
        {
            MessageBeep((UINT)-1);
            AfxMessageBox("Shell Open failed.", MB_OK|MB_ICONSTOP);
        }
    }
}

void CFilePanelWnd::OnExplorerTree_BeginLabelEdit (NMHDR* pNMHDR, LRESULT* pResult)
{
    TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;

    HTREEITEM hCurSel = pTVDispInfo->item.hItem;
    if (hCurSel && m_explorerTree.GetParentItem(hCurSel))
    {
        m_isLabelEditing = TRUE;
        *pResult = 0;
    }
    else
        *pResult = 1;
}

    static
    BOOL DoRename (LPCTSTR path, LPCTSTR oldName, LPCTSTR newName)
    {
        if (!::PathFileExists(path)) 
            return FALSE;

	    char src[MAX_PATH+1]; 
        memset(src, 0, sizeof(src));
        ::PathCombine(src, path, oldName);
        
	    char dest[MAX_PATH+1]; 
        memset(dest, 0, sizeof(dest));
        ::PathCombine(dest, path, newName);

        if (!::PathFileExists(src)) 
            return FALSE;

	    SHFILEOPSTRUCT fo; 
        memset(&fo, 0, sizeof(fo));
	    fo.hwnd   = AfxGetMainWnd()->m_hWnd;		
        fo.wFunc  = FO_RENAME;
	    fo.pFrom  = src;			
        fo.pTo    = dest;
	    fo.fFlags = FOF_ALLOWUNDO;

        if (!SHFileOperation(&fo) 
        && !fo.fAnyOperationsAborted 
        && ::PathFileExists(dest)) 
		    return TRUE;
        else 
            return FALSE;
    }

void CFilePanelWnd::OnExplorerTree_EndLabelEdit (NMHDR* pNMHDR, LRESULT* pResult)
{
    m_isLabelEditing = FALSE;

    TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;
    HTREEITEM hItem = pTVDispInfo->item.hItem;

    CString oldName = m_explorerTree.GetItemText(hItem);

    char newName[MAX_PATH]; 
    newName[0] = 0;

	if (pNMHDR->code == TVN_ENDLABELEDITA)
		lstrcpyn(newName, pTVDispInfo->item.pszText, sizeof(newName));
	else
		_wcstombsz(newName, (const wchar_t*)pTVDispInfo->item.pszText, sizeof(newName));

    
    if (strlen(newName) && oldName.CompareNoCase(newName)) 
    {
        HTREEITEM hParent = m_explorerTree.GetParentItem(hItem);

        if (hParent)
        {
            CString path = m_explorerTree.GetFullPath(hParent);

    	    if (DoRename(path, oldName, newName))
                m_explorerTree.SetItemText(hItem, newName);
        }
    }

    *pResult = 0;
}

void CFilePanelWnd::OnFpwRename ()
{
    HTREEITEM hCurSel = m_explorerTree.GetNextItem(TVI_ROOT, TVGN_CARET);
    if (hCurSel && m_explorerTree.GetParentItem(hCurSel)) // don't rename the root
    {
        m_explorerTree.SetFocus();
        m_explorerTree.EditLabel(hCurSel);
    }
}

void CFilePanelWnd::OnFpwDelete ()
{
    HTREEITEM hCurSel = m_explorerTree.GetNextItem(TVI_ROOT, TVGN_CARET);
    if (hCurSel)
    {
        CString path = m_explorerTree.GetFullPath(hCurSel);
	    char src[MAX_PATH+1]; 
        memset(src, 0, sizeof(src));
        strncpy(src, path, sizeof(src)-2);

	    SHFILEOPSTRUCT fo; 
        memset(&fo, 0, sizeof(fo));
	    fo.hwnd   = AfxGetMainWnd()->m_hWnd;		
        fo.wFunc  = FO_DELETE;
	    fo.pFrom  = src;			
        fo.fFlags = FOF_ALLOWUNDO;

        DWORD dummy;

	    if (!SHFileOperation(&fo) 
        && !fo.fAnyOperationsAborted 
        && !Common::AppGetFileAttrs(fo.pFrom, &dummy)) 
        {
		    HTREEITEM hParent = m_explorerTree.GetParentItem(hCurSel);
		    m_explorerTree.SelectItem(hParent);
		    m_explorerTree.DeleteItem(hCurSel);
	    }
    }
}

void CFilePanelWnd::OnFpwRefresh ()
{
    HTREEITEM hCurSel = m_explorerTree.GetNextItem(TVI_ROOT, TVGN_CARET);
    if (hCurSel)
    {
        CString path = m_explorerTree.GetFullPath(hCurSel);
        if (IsFolder(path))
            m_explorerTree.RefreshFolder(hCurSel);
        else
            MessageBeep((UINT)-1);
    }
}

BOOL CFilePanelWnd::SetCurPath (const CString& path)
{
    ChangeTab(FPW_EXPLORER_TAB, TRUE);

    if (strnicmp(path, m_curDrivePath, m_curDrivePath.GetLength()))
        DisplayDrivers();

    for (int i(0), count(m_driverPaths.GetSize()); i < count; i++)
    {
        if (!strnicmp(path, m_driverPaths[i], m_driverPaths[i].GetLength()))
        {
            SelectDrive(m_driverPaths[i]);
            if (m_explorerTree.SetSelPath(path))
            {
	            m_explorerTree.PostMessage(TVM_ENSUREVISIBLE, 0, (LPARAM)m_explorerTree.GetSelectedItem()); 
                return TRUE;
            }
        }
    }

    MessageBeep((UINT)-1);
    CString msg;
    msg.Format("Cannot find the file \n\n%s", (LPCSTR)path);
    
    if (!m_selectedFilter)
        AfxMessageBox(msg, MB_OK|MB_ICONSTOP);
    else
    {
        msg += "\n\nDo you want to reset the file filter and try again?    ";
        if (AfxMessageBox(msg, MB_YESNO|MB_ICONEXCLAMATION) == IDYES)
        {
            m_selectedFilter = 0;
            m_filterCBox.SetCurSel(m_selectedFilter);
            m_explorerTree.SetFilter(m_filterData.GetAt(m_selectedFilter));
            SetCurPath(path);
        }
    }

    return FALSE;
}

void CFilePanelWnd::OnFpwRefreshDrivers()
{
    DisplayDrivers(TRUE);
}

LRESULT CFilePanelWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CTabCtrl::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_

    return 0;
}

BOOL CFilePanelWnd::PreTranslateMessage(MSG* pMsg)
{
    if (!m_isLabelEditing && m_Tooltips)
	    m_tooltip.RelayEvent(pMsg);

    if (m_isLabelEditing) 
    {
	    switch( pMsg->message ) 
        {
	    case WM_KEYDOWN:
		    switch( pMsg->wParam ) 
            {
		    case VK_RETURN:
                m_explorerTree.SendMessage(TVM_ENDEDITLABELNOW, FALSE);
                return TRUE;
		    case VK_ESCAPE:
                m_explorerTree.SendMessage(TVM_ENDEDITLABELNOW, TRUE);
                return TRUE;
            }
        }
        return FALSE;
    }
    else if (m_explorerTree.m_hWnd == ::GetFocus())
    {
	    switch( pMsg->message ) 
        {
	    case WM_KEYDOWN:
		    switch( pMsg->wParam ) 
            {
		    case VK_RETURN:
                PostMessage(WM_COMMAND, ID_FPW_OPEN, 0L);
                return TRUE;
		    case VK_DELETE:
                PostMessage(WM_COMMAND, ID_FPW_DELETE, 0L);
                return TRUE;
		    case VK_F2:
                SendMessage(WM_COMMAND, ID_FPW_RENAME, 0L);
                return TRUE;
		    case VK_F5:
                PostMessage(WM_COMMAND, ID_FPW_REFRESH, 0L);
                return TRUE;
            }
        }
    }

    return CTabCtrl::PreTranslateMessage(pMsg);
}

void CFilePanelWnd::OnNotifyDisplayTooltip(NMHDR * pNMHDR, LRESULT * result)
{
	*result = 0;
	NM_PPTOOLTIP_DISPLAY * pNotify = (NM_PPTOOLTIP_DISPLAY*)pNMHDR;

	if (NULL == pNotify->hwndTool)
	{
		//Order to update a tooltip for a current Tooltip Help
		//He has not a handle of the window
		//If you want change tooltip's parameter than make it's here
	}
	else if (m_explorerTree.m_hWnd == pNotify->hwndTool)
        OnExplorerTree_NotifyDisplayTooltip(pNMHDR);
    else if (m_recentFilesListCtrl.m_hWnd == pNotify->hwndTool)
        OnRecentFilesListCtrl_NotifyDisplayTooltip(pNMHDR);
}

void CFilePanelWnd::SetFileCategoryFilter (const CStringArray& names, const CStringArray& filters, int select)
{
    ASSERT(!names.IsEmpty() && !filters.IsEmpty());
    ASSERT(names.GetSize() == filters.GetSize());

    CString filterCache;
    for (int i = 0; i < filters.GetSize(); ++i)
    {
        if (i) filterCache += '\n';
        filterCache += filters.GetAt(i);
    }

    BOOL filterChanged = filterCache != m_filterCache ? TRUE : FALSE;

    if (filterChanged
    && !names.IsEmpty() && names.GetSize() == filters.GetSize())
    {
        m_filterNames.RemoveAll();
        m_filterData.RemoveAll();
        
        m_filterNames.Copy(names);
        m_filterData.Copy(filters);

        if (m_filterCBox.m_hWnd)
        {
            m_filterCBox.ResetContent();

            COMBOBOXEXITEM item;
            memset(&item, 0, sizeof(item));
            for (int i = 0; i < m_filterNames.GetSize(); ++i)
            {
                item.iItem = i;
                item.mask = CBEIF_TEXT;
                item.pszText = m_filterNames[i].LockBuffer();

                int pos = 0;
                SHFILEINFO shFinfo;
                CString buff = m_filterData.GetAt(i).Tokenize(";", pos);
                buff.Replace("*", "$$$dummy$$$");
                if (SHGetFileInfo(buff, 0, &shFinfo, sizeof(shFinfo), SHGFI_ICON|SHGFI_SMALLICON|SHGFI_USEFILEATTRIBUTES))
                {
                    item.mask |= CBEIF_IMAGE|CBEIF_SELECTEDIMAGE;
                    item.iSelectedImage = item.iImage = shFinfo.iIcon;
                    DestroyIcon(shFinfo.hIcon); // we only need the index from the system image list
                }

                m_filterCBox.InsertItem(&item);
                m_filterNames[i].ReleaseBuffer();

            }
        }

        m_filterCache = filterCache;
    }

    if (filterChanged
    || m_selectedFilter != select)
    {
        if (select < m_filterData.GetSize())
            m_selectedFilter = select;
        else
            m_selectedFilter = -1;

        if (m_selectedFilter != -1)
        {
            m_filterCBox.SetCurSel(m_selectedFilter);
            m_explorerTree.SetFilter(m_filterData.GetAt(m_selectedFilter));
        }
    }
}

void CFilePanelWnd::OnFilter_SelChange ()
{
    m_selectedFilter = m_filterCBox.GetCurSel();
    _ASSERTE(m_selectedFilter < m_filterData.GetSize());

    if (m_selectedFilter < m_filterData.GetSize())
        m_explorerTree.SetFilter(m_filterData.GetAt(m_selectedFilter));
    else
        m_selectedFilter = -1;

    AfxGetApp()->WriteProfileInt(cszSection, cszFilter, m_selectedFilter);
}

void CFilePanelWnd::SetTooltips (BOOL tooltips) 
{ 
    m_Tooltips = tooltips;              
    
    if (m_Tooltips) 
    {
        if (m_explorerTree.m_hWnd) 
            m_explorerTree.ModifyStyle(0, TVS_NOTOOLTIPS);
        if (m_recentFilesListCtrl.m_hWnd) 
            m_recentFilesListCtrl.ModifyStyle(0, TVS_NOTOOLTIPS);
    }
    else            
    {
        if (m_explorerTree.m_hWnd) 
            m_explorerTree.ModifyStyle(TVS_NOTOOLTIPS, 0);
        if (m_recentFilesListCtrl.m_hWnd) 
            m_recentFilesListCtrl.ModifyStyle(TVS_NOTOOLTIPS, 0);
    }
}

void CFilePanelWnd::OnRecentFilesListCtrl_DblClick (NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
    if (POSITION pos = m_recentFilesListCtrl.GetFirstSelectedItemPosition())
    {
        LVITEM item;
        memset(&item, 0, sizeof(item));
        item.mask = LVIF_PARAM;
        item.iItem = m_recentFilesListCtrl.GetNextSelectedItem(pos);

        VERIFY(m_recentFilesListCtrl.GetItem(&item));
        string path = m_recentFilesListCtrl.GetPath(item.lParam);

        DoOpenFile(path, item.lParam);
    }
}

void CFilePanelWnd::OnRecentFilesListCtrl_RClick (NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pItem = (NMITEMACTIVATE*)pNMHDR;
    if (pItem && pItem->iItem != -1)
    {
        *pResult = 1;
        CPoint point(pItem->ptAction);
        m_recentFilesListCtrl.ClientToScreen(&point);

        LVITEM item;
        memset(&item, 0, sizeof(item));
        item.mask = LVIF_PARAM;
        item.iItem = pItem->iItem;

        VERIFY(m_recentFilesListCtrl.GetItem(&item));
        string path = m_recentFilesListCtrl.GetPath(item.lParam);

        CMenu menu;
        menu.CreatePopupMenu();

        menu.AppendMenu(MF_STRING, ID_FPW_HISTORY_OPEN, "&Open File\tDblClick");
        menu.SetDefaultItem(ID_FPW_HISTORY_OPEN);
        menu.AppendMenu(MF_SEPARATOR, (UINT_PTR)-1);
        menu.AppendMenu(MF_STRING, ID_FPW_HISTORY_REMOVE, "&Remove");
        menu.AppendMenu(MF_SEPARATOR, (UINT_PTR)-1);
        menu.AppendMenu(MF_STRING, ID_FILE_SYNC_LOCATION, "Sho&w File Location");
        menu.AppendMenu(MF_STRING, ID_FILE_COPY_LOCATION, "Copy File Locat&ion");

        int choice = (int)menu.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
        switch (choice)
        {
        case ID_FPW_HISTORY_OPEN:
            DoOpenFile(path, item.lParam);
            break;
        case ID_FPW_HISTORY_REMOVE:
            m_recentFilesListCtrl.RemoveEntry(item.lParam);
            break;
        case ID_FILE_SYNC_LOCATION:
            SetCurPath(path.c_str());
            break;
        case ID_FILE_COPY_LOCATION:
            Common::AppCopyTextToClipboard(path);
            break;
        default:
            ;
        }
    }
}

void CFilePanelWnd::OnRecentFilesListCtrl_OnBeginDrag (NMHDR* pNMHDR, LRESULT* pResult)
{
    NMITEMACTIVATE* pItem = (NMITEMACTIVATE*)pNMHDR;
    
    if (pItem && pItem->iItem != -1)
    {
        LVITEM item;
        memset(&item, 0, sizeof(item));
        item.mask = LVIF_PARAM;
        item.iItem = pItem->iItem;

        VERIFY(m_recentFilesListCtrl.GetItem(&item));
        CString path = m_recentFilesListCtrl.GetPath(item.lParam).c_str();

        //if (GetActiveDocumentPath(pItem, path))
        if (!path.IsEmpty())
        // TODO#2: move the following block {} in procedure
        {
            //MessageBox(path);
            UINT uBuffSize = sizeof(DROPFILES) + path.GetLength() + 1 + 1;
            if (HGLOBAL hgDrop = GlobalAlloc(GHND|GMEM_SHARE, uBuffSize))
            {
                if (DROPFILES*  pDrop = (DROPFILES*)GlobalLock(hgDrop))
                {
                    memset(pDrop, 0, uBuffSize);
                    pDrop->pFiles = sizeof(DROPFILES);
                    memcpy((LPBYTE(pDrop) + sizeof(DROPFILES)), (LPCTSTR)path, path.GetLength());

                    GlobalUnlock(hgDrop);

                    COleDataSource data;
                    FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
                    data.CacheGlobalData ( CF_HDROP, hgDrop, &etc );

                    /*DROPEFFECT dwEffect  =*/ data.DoDragDrop( DROPEFFECT_COPY );
                }
            }
        }
    }

    *pResult = 0;
}
