// DirTreeCtrl.cpp: 
// wrapped CTreeCtrl to select and or display folders and files (optional )
// 
// Aleksey Kochetov:
//  11.06.2002 minimal error handling has been added
//  13.06.2002 by performance reasons I don't want to check folder is empty or not
//             SUBDIR_ALWAYS has been added
//  13.06.2002 bug fix, open/close folder does not change image
//  06.08.2002 bug fix, make sure that selected item is visible
//  17.11.2002 bug fix, system image list, Win98 compatibility

#include "stdafx.h"
#include <WinException/WinException.h>
#include "DirTreeCtrl.h"
#include "SortStringArray.h"
#include <set>

#define SUBDIR_ALWAYS 1

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CDirTreeCtrl, CTreeCtrl)

    static 
    void GetFileImage (LPCTSTR path, TVITEM& item, UINT flags = SHGFI_ICON|SHGFI_SMALLICON/*|SHGFI_OPENICON*/)
    {
        SHFILEINFO shFinfo;
        if (SHGetFileInfo(path, 0, &shFinfo, sizeof(shFinfo), flags))
        {
            item.mask |= TVIF_IMAGE|TVIF_SELECTEDIMAGE;
            item.iImage = item.iSelectedImage = shFinfo.iIcon;
            DestroyIcon(shFinfo.hIcon); // we only need the index of the system image list
        }
    }

    static
    bool IsFolder (LPCTSTR path)
    {
        ASSERT_EXCEPTION_FRAME;

        DWORD dwFileAttributes = GetFileAttributes(path);

        if (dwFileAttributes != INVALID_FILE_ATTRIBUTES)
            return (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;

        return false;
    }

    static
    void CollectAllItems (CDirTreeCtrl& tree, HTREEITEM item, std::vector<HTREEITEM>& items)
    {
        if (!tree.ItemHasChildren(item))
        {
            // this is either a file or an empty folder
            items.push_back(item);
        }
        else
        {
            HTREEITEM subitem = tree.GetChildItem(item);
            CString text = tree.GetItemText(subitem);

            // if it contains an empty item then it is not open folder so ignore it
            if (text.IsEmpty()) return;
            
            items.push_back(item);
            while(subitem != NULL)
            {
                CollectAllItems(tree, subitem, items);
                subitem = tree.GetNextItem(subitem, TVGN_NEXT);
            }
        }
    }

/////////////////////////////////////////////////////////////////////////////
// CDirTreeCtrl

CDirTreeCtrl::CDirTreeCtrl()
{
    m_strFilter.Add(_T("*.*"));
}

CDirTreeCtrl::~CDirTreeCtrl()
{
    try {
        m_imgList.Detach();
    } _DESTRUCTOR_HANDLER_;
}

BEGIN_MESSAGE_MAP(CDirTreeCtrl, CTreeCtrl)
    //{{AFX_MSG_MAP(CDirTreeCtrl)
    ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemexpanded)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen f�r Nachrichten CDirTreeCtrl 

BOOL CDirTreeCtrl::DisplayTree(LPCTSTR strRoot, BOOL bFiles)
{
    //DWORD dwStyle = GetStyle();   // read the windowstyle
    //if ( dwStyle & TVS_EDITLABELS ) 
    //{
    //	// Don't allow the user to edit ItemLabels
    //	ModifyStyle( TVS_EDITLABELS , 0 );
    //}
    
    // Display the DirTree with the Rootname e.g. C:\
    // if Rootname == NULL then Display all Drives on this PC
    // First, we need the system-ImageList
    
    DeleteAllItems();

    if ( !GetSysImgList() )
        return FALSE;
    m_bFiles = bFiles;  // if TRUE, Display Path- and Filenames 
    if ( strRoot == NULL || strRoot[0] == '\0' )
    {
        if ( !DisplayDrives() )
            return FALSE;
        m_strRoot = _T("");
    }
    else
    {
        m_strRoot = strRoot;
        if ( m_strRoot.Right(1) != '\\' )
            m_strRoot += "\\";
        HTREEITEM hParent = AddItem( TVI_ROOT, m_strRoot );

        if (!hParent) // 11.06.2002   minimal error handling has been added
        {
            MessageBeep((UINT)-1);
            AfxMessageBox(CString(_T("Cannot show \"")) + m_strRoot + _T("\"."), MB_OK|MB_ICONSTOP);
            return FALSE;	
        }
        DisplayPath( hParent, strRoot );
    }
    return TRUE;	
}

BOOL CDirTreeCtrl::GetSysImgList()
{
    // 17.11.02   Win98 support
    SHFILEINFO shFinfo;
    HIMAGELIST hImgList = (HIMAGELIST)SHGetFileInfo(_T(""), 0, &shFinfo, sizeof(shFinfo), SHGFI_SMALLICON|SHGFI_SYSICONINDEX);

    if ( !hImgList )
    {
        m_strError = "Cannot retrieve the Handle of SystemImageList!";
        return FALSE;
    }

    if (m_imgList.m_hImageList) 
        m_imgList.Detach();

    m_imgList.Attach(hImgList);

    ::SendMessage(m_hWnd, TVM_SETIMAGELIST, (UINT)TVSIL_NORMAL, (LPARAM)hImgList);

    IMAGEINFO imageInfo;
    memset(&imageInfo, 0, sizeof(imageInfo));
    if (m_imgList.GetImageInfo(0, &imageInfo))
    {
        int itemHeight = ::SendMessage(m_hWnd, TVM_GETITEMHEIGHT, 0, 0);
        itemHeight = max<int>(itemHeight, (imageInfo.rcImage.bottom - imageInfo.rcImage.top) + 2);
        ::SendMessage(m_hWnd, TVM_SETITEMHEIGHT, itemHeight, 0);
    }

    return TRUE;   // OK
}

BOOL CDirTreeCtrl::DisplayDrives()
{
    //
    // Displaying the Availible Drives on this PC
    // This are the First Items in the TreeCtrl
    //
    DeleteAllItems();
    TCHAR  szDrives[128*2];
    TCHAR* pDrive;

    if ( !GetLogicalDriveStrings( sizeof(szDrives)/sizeof(szDrives[0]), szDrives ) )
    {
        m_strError = "Error Getting Logical DriveStrings!";
        return FALSE;
    }

    pDrive = szDrives;
    while( *pDrive )
    {
        HTREEITEM hParent = AddItem( TVI_ROOT, pDrive );
        if (hParent)  // 11.06.2002   minimal error handling has been added
        {
            if ( FindSubDir( pDrive ) )
                InsertItem( _T(""), 0, 0, hParent );
            pDrive += _tcslen( pDrive ) + 1;
        }
        else
        {
            MessageBeep((UINT)-1);
            AfxMessageBox(CString(_T("Cannot show \"")) + pDrive + _T("\"."), MB_OK|MB_ICONSTOP);
            return FALSE;
        }
    }


    return TRUE;

}
    
//
// Displaying the Path in the TreeCtrl
//
void CDirTreeCtrl::DisplayPath (HTREEITEM hParent, LPCTSTR strPath, BOOL skipFolders /*= FALSE*/)
{
    CWaitCursor wait;
    CFileFind find;
    CString strPathFiles = strPath;
    CSortStringArray strDirArray, strFileArray;
    
    if ( strPathFiles.Right(1) != "\\" )
        strPathFiles += "\\";
    strPathFiles += "*.*";

    BOOL bFind = find.FindFile( strPathFiles );
    while ( bFind )
    {
        bFind = find.FindNextFile();
        if ( find.IsDirectory() && !find.IsDots() )
            strDirArray.Add( find.GetFilePath() );
        if ( !find.IsDirectory() && m_bFiles )
            strFileArray.Add( find.GetFilePath() );
    }
    strDirArray.Sort();
    SetRedraw( FALSE );
    
    if (!skipFolders)
    {
        for ( int i = 0; i < strDirArray.GetSize(); i++ )
        {
            if (HTREEITEM hItem = AddItem(hParent, strDirArray.GetAt(i))) 
            {
                if ( FindSubDir( strDirArray.GetAt(i) ) )
                    InsertItem( _T(""), 0, 0, hItem );
            }
            else // 11.06.2002   minimal error handling has been added
            {
                MessageBeep((UINT)-1);
                if (AfxMessageBox(CString(_T("Cannot show \"")) + strDirArray.GetAt(i) + _T("\"."), 
                        MB_OKCANCEL|MB_ICONSTOP) == IDCANCEL)
                {
                    SetRedraw(TRUE);
                    return;
                }
            }
        }
    }

    if ( m_bFiles )
    {
        if (m_strFilter.IsEmpty())
        {
            strFileArray.Sort();
            for (int i = 0; i < strFileArray.GetSize(); i++ )
            {
                HTREEITEM hItem = AddItem( hParent, strFileArray.GetAt(i) );
                if (!hItem) // 11.06.2002 minimal error handling has been added
                {
                    MessageBeep((UINT)-1);
                    if (AfxMessageBox(CString(_T("Cannot show \"")) + strFileArray.GetAt(i) + _T("\"."), 
                            MB_OKCANCEL|MB_ICONSTOP) == IDCANCEL)
                    {
                        SetRedraw(TRUE);
                        return;
                    }
                }
            }
        }
        else
        {
            CStringList files;
            
            for (int i = 0; i < m_strFilter.GetCount(); ++i)
            {
                CString strPathFiles = strPath;
                if ( strPathFiles.Right(1) != _T("\\") )
                    strPathFiles += _T("\\");
                strPathFiles += m_strFilter.GetAt(i);

                BOOL bFind = find.FindFile(strPathFiles);
                while ( bFind )
                {
                    bFind = find.FindNextFile();
                    if (!find.IsDirectory())
                        files.AddTail( (LPCTSTR)find.GetFilePath() );
                }
            }

            for (POSITION pos = files.GetHeadPosition(); pos != NULL;)
            {
                CString file = files.GetNext(pos);
                HTREEITEM hItem = AddItem( hParent, file);
                if (!hItem) // 11.06.2002   minimal error handling has been added
                {
                    MessageBeep((UINT)-1);
                    if (AfxMessageBox(CString(_T("Cannot show \"")) + file + _T("\"."), MB_OKCANCEL|MB_ICONSTOP) == IDCANCEL)
                    {
                        SetRedraw(TRUE);
                        return;
                    }
                }
            }
        }
    }

    //
    // change item image to an open folder
    //
    TVITEM item;
    memset(&item, 0, sizeof(item));
    item.hItem = hParent;
    GetFileImage (strPath, item, SHGFI_ICON|SHGFI_SMALLICON|SHGFI_OPENICON);
    SetItem(&item);
    
    SetRedraw( TRUE );
}

HTREEITEM CDirTreeCtrl::AddItem (HTREEITEM hParent, LPCTSTR strPath)
{
    // Adding the Item to the TreeCtrl with the current Icons
    CString strTemp = strPath;
    
    if ( strTemp.Right(1) != '\\' )
         strTemp += "\\";
    
    TVINSERTSTRUCT item;
    memset(&item, 0, sizeof(item));
    item.hParent = hParent;
    item.hInsertAfter = TVI_LAST;
    
    item.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
    GetFileImage(strTemp, item.item, SHGFI_ICON|SHGFI_SMALLICON);
    
    if (strTemp.Right(1) == "\\")
        strTemp.SetAt(strTemp.GetLength() - 1, '\0');
    
    if (hParent == TVI_ROOT)
        item.item.pszText = (LPTSTR)(LPCTSTR)strTemp;
    else
        item.item.pszText = (LPTSTR)(LPCTSTR)GetSubPath(strTemp);

    return this->InsertItem(&item);
}

LPCTSTR CDirTreeCtrl::GetSubPath(LPCTSTR strPath)
{
    //
    // getting the last SubPath from a PathString
    // e.g. C:\temp\readme.txt
    // the result = readme.txt
    static CString strTemp;
    int     iPos;

    strTemp = strPath;
    if ( strTemp.Right(1) == '\\' )
         strTemp.SetAt( strTemp.GetLength() - 1, '\0' );
    iPos = strTemp.ReverseFind( '\\' );
    if ( iPos != -1 )
        strTemp = strTemp.Mid( iPos + 1);

    return (LPCTSTR)strTemp;
}

#if (SUBDIR_ALWAYS==1)

BOOL CDirTreeCtrl::FindSubDir (LPCTSTR)
{
    return TRUE;
}

#else // SUBDIR_ALWAYS

BOOL CDirTreeCtrl::FindSubDir (LPCTSTR strPath)
{
    CFileFind find;
    CString   strTemp = strPath;
    BOOL      bFind;

    if ( strTemp[strTemp.GetLength()-1] == '\\' )
        strTemp += "*.*";
    else
        strTemp += "\\*.*";
        
    bFind = find.FindFile( strTemp );
    
    
    while ( bFind )
    {
        bFind = find.FindNextFile();

        if ( find.IsDirectory() && !find.IsDots() )
        {
            return TRUE;
        }
        if ( !find.IsDirectory() && m_bFiles && !find.IsHidden() )
            return TRUE;
        
    }

    return FALSE;
}

#endif//SUBDIR_ALWAYS

void CDirTreeCtrl::OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    CString strPath;
     
    if ( pNMTreeView->itemNew.state & TVIS_EXPANDED )
    {
        ExpandItem( pNMTreeView->itemNew.hItem, TVE_EXPAND );
    }
    else
    {
        //
        // Delete the Items, but leave one there, for 
        // expanding the item next time
        //
        HTREEITEM hChild = GetChildItem( pNMTreeView->itemNew.hItem );
                
        while ( hChild ) 
        {
            DeleteItem( hChild );
            hChild = GetChildItem( pNMTreeView->itemNew.hItem );
        }
        InsertItem(_T(""), pNMTreeView->itemNew.hItem );
    
        TVITEM item;
        memset(&item, 0, sizeof(item));
        item.hItem = pNMTreeView->itemNew.hItem;
        GetFileImage(GetFullPath(item.hItem), item, SHGFI_ICON|SHGFI_SMALLICON);
        SetItem(&item);
    }

    *pResult = 0;
}

CString CDirTreeCtrl::GetFullPath(HTREEITEM hItem)
{
    // get the Full Path of the item
    CString strReturn;
    CString strTemp;
    HTREEITEM hParent = hItem;

    strReturn = _T("");

    while ( hParent )
    {
        
        strTemp  = GetItemText( hParent );
        strTemp += "\\";
        strReturn = strTemp + strReturn;
        hParent = GetParentItem( hParent );
    }
    
    strReturn.TrimRight( '\\' );

    return strReturn;

}

// 2018-11-12 reimplemeted
BOOL CDirTreeCtrl::SetSelPath (LPCTSTR szPath)
{
    BOOL bRet = FALSE;

    HTREEITEM hItem  = TVI_ROOT;

    int curPos = 0;
    CString path(szPath);
    CString part = path.Tokenize(_T("\\"), curPos);

    SetRedraw( FALSE );

    while (!part.IsEmpty())
    {
        hItem = SearchSiblingItem( hItem, part.MakeUpper());
        
        if (!hItem)
            break;

        // Info:
        // the notification OnItemExpanded 
        // will not called every time 
        // after the call Expand. 
        // You must call Expand with TVE_COLLAPSE | TVE_COLLAPSERESET
        // to Reset the TVIS_EXPANDEDONCE Flag
        UINT uState;
        uState = GetItemState( hItem, TVIS_EXPANDEDONCE );
        if ( uState )
        {
            Expand( hItem, TVE_EXPAND );
            Expand( hItem, TVE_COLLAPSE | TVE_COLLAPSERESET );
            InsertItem(_T(""), hItem ); // insert a blank child-item
            Expand( hItem, TVE_EXPAND ); // now, expand send a notification
        }
        else
            Expand( hItem, TVE_EXPAND );

        part = path.Tokenize(_T("\\"), curPos);
    }
    
    if ( hItem ) // Ok the last subpath was found
    {
        EnsureVisible( hItem ); // 06.08.2002 bug fix, make sure that selected item is visible
        SelectItem( hItem );    // select the last expanded item
        bRet = TRUE;
    }
    
    SetRedraw( TRUE );

    return bRet;
}

HTREEITEM CDirTreeCtrl::SearchSiblingItem( HTREEITEM hItem, LPCTSTR strText)
{
    HTREEITEM hFound = GetChildItem( hItem );
    CString   strTemp;
    while ( hFound )
    {
        strTemp = GetItemText( hFound );
        strTemp.MakeUpper();
        if ( strTemp == strText )
            return hFound;
        hFound = GetNextItem( hFound, TVGN_NEXT );
    }

    return NULL;
}


void CDirTreeCtrl::ExpandItem(HTREEITEM hItem, UINT nCode)
{	
    CString strPath;
    
    if ( nCode == TVE_EXPAND )
    {
        HTREEITEM hChild = GetChildItem( hItem );
        while ( hChild )
        {
            DeleteItem( hChild );
            hChild = GetChildItem( hItem );
        }
        
        strPath = GetFullPath( hItem );
        DisplayPath( hItem, strPath );
    }
}

BOOL CDirTreeCtrl::IsValidPath(LPCTSTR strPath)
{
    // 06.08.2002 bug fix, Win95 compatibility
    return (GetFileAttributes(strPath) != 0xFFFFFFFF) ? TRUE : FALSE;
}

void CDirTreeCtrl::RefreshFolder (HTREEITEM hItem)
{
    UINT uState = GetItemState(hItem, TVIS_EXPANDEDONCE|TVIS_EXPANDED);

    if (uState & TVIS_EXPANDED)
    {
        SetRedraw(FALSE);

        if (uState & TVIS_EXPANDEDONCE)
        {
            Expand( hItem, TVE_EXPAND );
            Expand( hItem, TVE_COLLAPSE | TVE_COLLAPSERESET );
            InsertItem(_T(""), hItem ); // insert a blank child-item
            Expand( hItem, TVE_EXPAND ); // now, expand send a notification
        }

        SetRedraw(TRUE);
    }
}

BOOL CDirTreeCtrl::PreCreateWindow (CREATESTRUCT& cs)
{
    //cs.lpszClass = "SysTreeView32";
    cs.style |= WS_CHILD|WS_TABSTOP|/*TVS_LINESATROOT|*/TVS_HASBUTTONS|TVS_HASLINES|TVS_SHOWSELALWAYS;

    if (!CTreeCtrl::PreCreateWindow(cs))
        return FALSE;

    return TRUE;
}

void CDirTreeCtrl::SetFilter (const CString& filter)
{
    CStringArray arrFilter;

    int curPos = 0;
    CString token = filter.Tokenize(_T(";"), curPos);
    while (!token.IsEmpty())
    {
        arrFilter.Add(token);
        token = filter.Tokenize(_T(";"), curPos);
    };

    SetFilter(arrFilter);
}

void CDirTreeCtrl::SetFilter (const CStringArray& filter)
{
    m_strFilter.RemoveAll();
    m_strFilter.Copy(filter);

    std::vector<HTREEITEM> items;

    CollectAllItems(*this, GetRootItem(), items);

    std::vector<HTREEITEM>::iterator it = items.begin();
    for (; it != items.end(); ++it)
    {
        CString path = GetFullPath(*it);
        if (!IsFolder(path))
        {
            DeleteItem(*it);
            *it = NULL;
        }
    }

    it = items.begin();
    for (; it != items.end(); ++it)
    {
        if (*it)
        {
            CString path = GetFullPath(*it);
            DisplayPath(*it, path, TRUE);
        }
    }
}

void CDirTreeCtrl::ClearFilter ()
{
    m_strFilter.RemoveAll();
    m_strFilter.Add(_T("*.*"));
    SetFilter(m_strFilter);
}
