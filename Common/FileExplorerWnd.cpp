/* 
    Copyright (C) 2002, 2018 Aleksey Kochetov

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
TODO: return focus the the main window on <Escape>
TODO#2: add drag from for history list
TODO#2: remove shared code from NotifyDisplayTooltip  / OnRecentFilesListCtrl_NotifyDisplayTooltip
*/

#include "stdafx.h"
#include "resource.h"
#include <Shlwapi.h>
#include <string>
#include "AppUtilities.h"
#include "ExceptionHelper.h"
#include <WinException/WinException.h>
#include "FileExplorerWnd.h"
#include "WorkbookMDIFrame.h"
#include "GUICommandDictionary.h"
#include "AppUtilities.h"
#include "CustomShellContextMenu.h"

// FilePanelWnd
// TODO:
//      exception handling
//      add folder to driver list - folder favorites list

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

static LPCWSTR cszSection = L"FileManager";
static LPCWSTR cszTab     = L"ActiveTabV2";
static LPCWSTR cszFilter  = L"ActiveFilter";
static LPCWSTR cszDrive   = L"CurrentDrive";

    static BOOL GetVolumeName (LPCWSTR rootDirectory, CString& volumeName)
    {
        const int STRSIZE = 80;
        DWORD volumeSerialNumber;           // volume serial number
        DWORD maximumComponentLength;       // maximum file name length
        DWORD fileSystemFlags;              // file system options
        WCHAR fileSystemNameBuffer[STRSIZE];// file system name buffer

        BOOL volInfo = GetVolumeInformation(
            rootDirectory,                  // root directory
            volumeName.GetBuffer(STRSIZE),  // volume name buffer
            STRSIZE,                        // length of name buffer
            &volumeSerialNumber,            // volume serial number
            &maximumComponentLength,        // maximum file name length
            &fileSystemFlags,               // file system options
            fileSystemNameBuffer,           // file system name buffer
            sizeof(fileSystemNameBuffer)/sizeof(WCHAR) // length of file system name buffer
        );

        volumeName.ReleaseBuffer();

        return volInfo;
    }

    static LPCWSTR MakeVolumeLabel (LPCWSTR rootDirectory, CString& volumeLabel)
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
IMPLEMENT_DYNAMIC(FileExplorerWnd, CDockablePane)

FileExplorerWnd::FileExplorerWnd()
{
    m_startup               = TRUE;
    m_isExplorerInitialized = FALSE;
    m_isDrivesInitialized   = FALSE;
    m_isLabelEditing        = FALSE;

    m_Tooltips           = TRUE;
    m_PreviewInTooltips  = TRUE;
    m_PreviewLines       = 10;

    m_ctxmenuFileProperties =
    m_ctxmenuTortoiseGit = 
    m_ctxmenuTortoiseSvn = true;

    m_filterNames.Add(L"All files (*.*)");
    m_filterData .Add(L"*.*");
    m_selectedFilter = -1;
}

FileExplorerWnd::~FileExplorerWnd()
{
}

void FileExplorerWnd::SelectDrive (const CString& path, BOOL force)
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
                    message.Format(L"Cannot select drive \"%s\". Drive/device is not available.", (LPCWSTR)path);
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
                        message.Format(L"Cannot select drive \"%s\". %s", (LPCWSTR)path, (LPCWSTR)*x);
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
void FileExplorerWnd::DisplayDrivers (BOOL force, BOOL curOnly)
{
    if (force || !m_isDrivesInitialized)
    {
        CWaitCursor wait;

        if (!curOnly) m_drivesCBox.LockWindowUpdate(); // 09.03.2003 bug fix, desktop flickers when the programm is starting
        m_drivesCBox.SetRedraw(FALSE);
        m_drivesCBox.ResetContent();
        WCHAR  szDrives[2*1024];
        WCHAR* pDrive;

        if (curOnly)
        {
            _CHECK_AND_THROW_(m_curDrivePath.GetLength() < sizeof(szDrives) - 2, 
                "Current driver path is too long.");
            memset(szDrives, 0, sizeof(szDrives));
            wcsncpy(szDrives, (LPCWSTR)m_curDrivePath, m_curDrivePath.GetLength());
        }
        else
        {
            _CHECK_AND_THROW_(GetLogicalDriveStrings(sizeof(szDrives)/sizeof(WCHAR), szDrives) > 0, 
                "Cannot get logical driver strings.");
        }

        m_driverPaths.RemoveAll();

        COMBOBOXEXITEM item;
        memset(&item, 0, sizeof(item));

        for (pDrive = szDrives; *pDrive; pDrive += wcslen(pDrive) + 1, item.iItem++)
        {
            m_driverPaths.Add(pDrive);

            item.mask = CBEIF_TEXT;
            CString volumeLabel;
            item.pszText = (LPWSTR)MakeVolumeLabel(pDrive, volumeLabel);

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

BEGIN_MESSAGE_MAP(FileExplorerWnd, CDockablePane)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_CBN_SETFOCUS(ID_FPW_DRIVES, OnDrive_SetFocus)
    ON_CBN_SELCHANGE(ID_FPW_DRIVES, OnDrive_SelChange)

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

    ON_WM_SHOWWINDOW()
    ON_WM_PAINT()
END_MESSAGE_MAP()


// FilePanelWnd message handlers


int FileExplorerWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDockablePane::OnCreate(lpCreateStruct) == -1)
        return -1;

    // the order is significant for navagation
    if (!m_explorerTree.Create(
        WS_CHILD|WS_VISIBLE|WS_TABSTOP
        /*|TVS_LINESATROOT*/|TVS_HASBUTTONS|TVS_HASLINES|TVS_SHOWSELALWAYS|TVS_EDITLABELS, 
        CRect(0, 0, 0, 0), this, ID_FPW_EXPLORER))
    {
        TRACE0("Failed to create explorer tree\n");
        return -1;
    }
    if (!m_filterCBox.Create(
        WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST,
        CRect(0, 0, 0, GetSystemMetrics(SM_CYFULLSCREEN)/3), this, ID_FPW_FILTER))
    {
            TRACE0("Failed to create drives combo box\n");
            return -1;
    }
    if (!m_drivesCBox.Create(
        WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST,
        CRect(0, 0, 0, GetSystemMetrics(SM_CYFULLSCREEN)/3), this, ID_FPW_DRIVES))
    {
            TRACE0("Failed to create drives combo box\n");
            return -1;
    }

    for (int i = 0; i < m_filterNames.GetSize() && i < m_filterData.GetSize(); ++i)
    {
        COMBOBOXEXITEM item;
        memset(&item, 0, sizeof(item));
        item.iItem = i;
        item.mask = CBEIF_TEXT;
        item.pszText = m_filterNames[i].LockBuffer();

        int pos = 0;
        SHFILEINFO shFinfo;
        CString buff = m_filterData.GetAt(i).Tokenize(L";", pos);
        buff.Replace(L"*", L"$$$dummy$$$");
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
    ModifyStyleEx(0, WS_EX_CONTROLPARENT, 0);
    m_explorerTree.ModifyStyleEx(0, WS_EX_STATICEDGE/*WS_EX_CLIENTEDGE*/, 0);

    m_drivesCBox.SetImageList(&m_explorerTree.GetSysImageList());
    m_filterCBox.SetImageList(&m_explorerTree.GetSysImageList());

    m_explorerStateImageList.Create(IDB_OE_EXPLORER_STATE_LIST, 16, 64, RGB(0,0,255));
    m_explorerTree.SetImageList(&m_explorerStateImageList, TVSIL_STATE);

    // load saved drve
    CString drivePath = AfxGetApp()->GetProfileString(cszSection, cszDrive);
    if (drivePath.IsEmpty())
    {
        CString path;
        Common::AppGetPath(path);
        PathBuildRoot(drivePath.GetBuffer(MAX_PATH), PathGetDriveNumber(path));
        drivePath.ReleaseBuffer();
    }
    m_curDrivePath = drivePath;

    m_drivesRClick.SubclassWindow(::GetWindow(m_drivesCBox.m_hWnd, GW_CHILD));
    m_filterRClick.SubclassWindow(::GetWindow(m_filterCBox.m_hWnd, GW_CHILD));

    m_tooltip.Create(this, FALSE);
    m_tooltip.SetBehaviour(PPTOOLTIP_DISABLE_AUTOPOP|PPTOOLTIP_MULTIPLE_SHOW);
    m_tooltip.SetNotify(m_hWnd);
    m_tooltip.AddTool(&m_explorerTree);
    m_tooltip.SetDirection(PPTOOLTIP_TOPEDGE_LEFT);

    SetTooltips(m_Tooltips);

    return 0;
}

void FileExplorerWnd::OnDestroy ()
{
    // for Win95 compatibility
    m_explorerTree.DestroyWindow();
    m_drivesCBox.DestroyWindow(); 
    m_filterCBox.DestroyWindow();
    m_drivesRClick.DestroyWindow();
    m_filterRClick.DestroyWindow();

    CDockablePane::OnDestroy();
}

void FileExplorerWnd::OnSize(UINT nType, int cx, int cy)
{
    CDockablePane::OnSize(nType, cx, cy);

    if (cx > 0 && cy > 0
    && nType != SIZE_MAXHIDE && nType != SIZE_MINIMIZED) 
    {
        CRect rect;
        GetClientRect(&rect);
        //rect.InflateRect(3, 0);
        
        CRect comboRect;
        m_drivesCBox.GetWindowRect(&comboRect);
        int comboH = comboRect.Height() /*+ 2*/;

        HDWP hdwp = ::BeginDeferWindowPos(10);
        ::DeferWindowPos(hdwp, m_drivesCBox, 0, rect.left, rect.top, 
            rect.Width(), rect.Height()/2, SWP_NOZORDER);
        ::DeferWindowPos(hdwp, m_explorerTree, 0, rect.left + 1, rect.top + comboH + 1, 
            rect.Width() - 2, rect.Height() - 2 * comboH - 2, SWP_NOZORDER);
        ::DeferWindowPos(hdwp, m_filterCBox, 0, rect.left, 
            rect.top + rect.Height() - comboH /*+ 2*/, rect.Width(), rect.Height()/2, SWP_NOZORDER);
        ::EndDeferWindowPos(hdwp);
    }
}

void FileExplorerWnd::OnPaint()
{
    CPaintDC dc(this);

    CRect rectTree;
    m_explorerTree.GetWindowRect(rectTree);
    ScreenToClient(rectTree);

    rectTree.InflateRect(1, 1);
    dc.Draw3dRect(rectTree, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DSHADOW));
}

// move the following code to refresh command handler
void FileExplorerWnd::OnDrive_SetFocus ()
{
    DisplayDrivers();
}

void FileExplorerWnd::OnDrive_SelChange ()
{
    int inx = m_drivesCBox.GetCurSel();
    _ASSERTE(inx < m_driverPaths.GetSize());

    if (inx < m_driverPaths.GetSize())
        SelectDrive(m_driverPaths.ElementAt(inx), FALSE);

    AfxGetApp()->WriteProfileString(cszSection, cszDrive, GetCurDrivePath());
}

void FileExplorerWnd::OnExplorerTree_DblClick (NMHDR*, LRESULT* pResult)
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

void FileExplorerWnd::OnExplorerTree_RClick (NMHDR*, LRESULT* pResult)
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
        pPopup->ModifyMenu(ID_FPW_OPEN, MF_BYCOMMAND, ID_FPW_OPEN, !isFolder ? L"Open File\tDblClick" : L"Open File Dalog...\tCtrl+DblClick");
        pPopup->EnableMenuItem(ID_FPW_REFRESH, isFolder ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);

        ASSERT(pPopup != NULL);
        m_explorerTree.ClientToScreen(&point);

        if ((m_ctxmenuFileProperties || m_ctxmenuTortoiseGit || m_ctxmenuTortoiseSvn) && ::PathFileExists(path))
        {
            if (UINT idCommand = CustomShellContextMenu(this, point, path, pPopup, m_ctxmenuFileProperties, m_ctxmenuTortoiseGit, m_ctxmenuTortoiseSvn).ShowContextMenu())
                SendMessage(WM_COMMAND, idCommand, 0);
        }
        else
        {
            pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
        }
    }
    else
        *pResult = 0;
}

void FileExplorerWnd::OnDrivers_RClick (NMHDR*, LRESULT* pResult)
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

void FileExplorerWnd::OnFpwOpen ()
{
    HTREEITEM hCurSel = m_explorerTree.GetNextItem(TVI_ROOT, TVGN_CARET);
    if (hCurSel)
    {
        CString path = m_explorerTree.GetFullPath(hCurSel);
        AfxGetApp()->OpenDocumentFile(path);
    }
}

void FileExplorerWnd::OnFpwShellOpen ()
{
    HTREEITEM hCurSel = m_explorerTree.GetNextItem(TVI_ROOT, TVGN_CARET);
    if (hCurSel)
    {
        CString path = m_explorerTree.GetFullPath(hCurSel);

        HINSTANCE hResult = ::ShellExecute(NULL, L"open", path, NULL, NULL, SW_SHOWNORMAL);
    
        if ((UINT)hResult <= HINSTANCE_ERROR)
        {
            MessageBeep((UINT)-1);
            AfxMessageBox(L"Shell Open failed.", MB_OK|MB_ICONSTOP);
        }
    }
}

void FileExplorerWnd::OnExplorerTree_BeginLabelEdit (NMHDR* pNMHDR, LRESULT* pResult)
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
    BOOL DoRename (LPCWSTR path, LPCWSTR oldName, LPCWSTR newName)
    {
        if (!::PathFileExists(path)) 
            return FALSE;

        WCHAR src[MAX_PATH+1]; 
        memset(src, 0, sizeof(src));
        ::PathCombine(src, path, oldName);
        
        WCHAR dest[MAX_PATH+1]; 
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

void FileExplorerWnd::OnExplorerTree_EndLabelEdit (NMHDR* pNMHDR, LRESULT* pResult)
{
    m_isLabelEditing = FALSE;

    TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;
    HTREEITEM hItem = pTVDispInfo->item.hItem;

    CString oldName = m_explorerTree.GetItemText(hItem);

    CString newName; 

    if (pNMHDR->code == TVN_ENDLABELEDITW)
        newName = pTVDispInfo->item.pszText;
    else
        newName = Common::wstr(string((char*)pTVDispInfo->item.pszText)).c_str();

    
    if (oldName.CompareNoCase(newName)) 
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

void FileExplorerWnd::OnFpwRename ()
{
    HTREEITEM hCurSel = m_explorerTree.GetNextItem(TVI_ROOT, TVGN_CARET);
    if (hCurSel && m_explorerTree.GetParentItem(hCurSel)) // don't rename the root
    {
        m_explorerTree.SetFocus();
        m_explorerTree.EditLabel(hCurSel);
    }
}

void FileExplorerWnd::OnFpwDelete ()
{
    HTREEITEM hCurSel = m_explorerTree.GetNextItem(TVI_ROOT, TVGN_CARET);
    if (hCurSel)
    {
        CString path = m_explorerTree.GetFullPath(hCurSel);

        SHFILEOPSTRUCT fo; 
        memset(&fo, 0, sizeof(fo));
        fo.hwnd   = AfxGetMainWnd()->m_hWnd;
        fo.wFunc  = FO_DELETE;
        fo.pFrom = path; // double 0 terminated
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

void FileExplorerWnd::OnFpwRefresh ()
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

BOOL FileExplorerWnd::SetCurPath (const CString& path)
{
    if (wcsnicmp(path, m_curDrivePath, m_curDrivePath.GetLength()))
        DisplayDrivers();

    for (int i(0), count(m_driverPaths.GetSize()); i < count; i++)
    {
        if (!wcsnicmp(path, m_driverPaths[i], m_driverPaths[i].GetLength()))
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
    msg.Format(L"Cannot find the file \n\n%s", (LPCWSTR)path);
    
    if (!m_selectedFilter)
        AfxMessageBox(msg, MB_OK|MB_ICONSTOP);
    else
    {
        msg += L"\n\nDo you want to reset the file filter and try again?    ";
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

void FileExplorerWnd::OnFpwRefreshDrivers()
{
    DisplayDrivers(TRUE);
}

LRESULT FileExplorerWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDockablePane::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_

    return 0;
}

BOOL FileExplorerWnd::PreTranslateMessage(MSG* pMsg)
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
    
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE) // FRM
    {
        if (CWnd* pWnd = AfxGetMainWnd())
            if (CMDIFrameWnd* pFrame = DYNAMIC_DOWNCAST(CMDIFrameWnd, pWnd))
                if (CMDIChildWnd* pMDIChild = pFrame->MDIGetActive())
                    pMDIChild->SetFocus();
        return TRUE;
    }

    return CDockablePane::PreTranslateMessage(pMsg);
}

void FileExplorerWnd::OnNotifyDisplayTooltip(NMHDR * pNMHDR, LRESULT * result)
{
    *result = 0;
    NM_PPTOOLTIP_DISPLAY * pNotify = (NM_PPTOOLTIP_DISPLAY*)pNMHDR;

    if (m_explorerTree.m_hWnd == pNotify->hwndTool)
        NotifyDisplayTooltip(pNMHDR);
}

void FileExplorerWnd::SetFileCategoryFilter (const CStringArray& names, const CStringArray& filters, int select)
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
                CString buff = m_filterData.GetAt(i).Tokenize(L";", pos);
                buff.Replace(L"*", L"$$$dummy$$$");
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

void FileExplorerWnd::OnFilter_SelChange ()
{
    m_selectedFilter = m_filterCBox.GetCurSel();
    _ASSERTE(m_selectedFilter < m_filterData.GetSize());

    if (m_selectedFilter < m_filterData.GetSize())
        m_explorerTree.SetFilter(m_filterData.GetAt(m_selectedFilter));
    else
        m_selectedFilter = -1;

    AfxGetApp()->WriteProfileInt(cszSection, cszFilter, m_selectedFilter);
}

void FileExplorerWnd::SetTooltips (BOOL tooltips) 
{ 
    m_Tooltips = tooltips;              
    
    if (m_Tooltips) 
    {
        if (m_explorerTree.m_hWnd) 
            m_explorerTree.ModifyStyle(0, TVS_NOTOOLTIPS);
    }
    else            
    {
        if (m_explorerTree.m_hWnd) 
            m_explorerTree.ModifyStyle(TVS_NOTOOLTIPS, 0);
    }
}

void FileExplorerWnd::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CDockablePane::OnShowWindow(bShow, nStatus);

    if (!m_startup && bShow && !m_isExplorerInitialized)
    {
        CWaitCursor wait;
        DisplayDrivers(FALSE, TRUE);
        SelectDrive(m_curDrivePath, TRUE);
        m_isExplorerInitialized = TRUE;
    }
}


void FileExplorerWnd::CompleteStartup  ()
{ 
    m_startup = FALSE; 

    if (IsVisible() && IsWindowVisible())
    {
        CWaitCursor wait;
        DisplayDrivers(FALSE, TRUE);
        SelectDrive(m_curDrivePath, TRUE);
        m_isExplorerInitialized = TRUE;
    }
}
