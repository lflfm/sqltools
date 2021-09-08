/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2004 Aleksey Kochetov

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

// 16.12.2004 (Ken Clubok) Add CSV prefix option
// 2017-11-28 implemented range selection
// 2017-11-28 added 3 new export formats

#pragma once
#ifndef __GridSourceBase_h__
#define __GridSourceBase_h__

/***************************************************************************/
/*      Purpose: Text data source implementation for grid component        */
/*      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~           */
/*      (c) 1996-1999,2003 Aleksey Kochetov                                */
/***************************************************************************/

#include <set>
#include <Common/NVector.h>
#include "AGridSource.h"
#include "StringTable.h"


#define BASE_SOURCE_PROPERTY(type, name) \
    private: type m_##name; \
    public:  type Get##name () const { return m_##name; } \
    public:  void Set##name (type val) { m_##name = val; } 

#define BASE_SOURCE_STATIC_PROPERTY(type, name) \
    private: static type m_##name; \
    public:  static type Get##name () { return m_##name; } \
    public:  static void Set##name (type val) { m_##name = val; } 

#define BASE_SOURCE_COL_PROPERTY(type, name) \
    public: type GetCol##name (int col) const { return m_ColumnInfo.at(col).m_##name; } \
    public: void SetCol##name (int col, type val) { m_ColumnInfo.at(col).m_##name = val; } 


namespace OG2 /* OG = OpenGrid */
{
    enum EColumnAlignment  
    { 
        ecaLeft, 
        ecaRight 
    };
    
    enum ETextFormat       
    { 
        etfDefault          = -1,
        etfPlainText        = 0, 
        etfQuotaDelimited   = 1, 
        etfTabDelimitedText = 2, 
        etfXmlElem          = 3, 
        etfXmlAttr          = 4,
        etfHtml             = 5,
        etfPLSQL1           = 6,
        etfPLSQL2           = 7,
        etfPLSQL3           = 8,
        etfPLSQL4           = 9,
    };

    enum EOrientation
    {
        eoVert = 0,
        eoHorz = 1
    };

class GridSourceBase : public AGridSource
{
public:
    GridSourceBase ();
    ~GridSourceBase ();

    // AGridSource interface implementation
    virtual bool CanModify () const                                     { return false; }
    virtual void Clear ()                                               { m_ColumnInfo.clear(); }

    // Cell management
    virtual int GetFirstFixCount (eDirection) const;
    virtual int GetLastFixCount  (eDirection) const;

    virtual int GetFirstFixSize  (int pos, eDirection) const;
    virtual int GetLastFixSize   (int pos, eDirection) const;
    virtual int GetSize          (int pos, eDirection) const;

    virtual void PaintCell (DrawCellContexts&) const;
    virtual void CalculateCell (DrawCellContexts&, size_t maxTextLength) const;

    virtual CWnd* BeginEdit  (int row, int col, CWnd*, const CRect&); // get editor for grid window
    virtual void  PostEdit   (); // event for commit edit result &
    virtual void  CancelEdit (); // end edit session

    // clipboard operation support
    virtual void DoEditCopy  (int, int, int, int, ECopyWhat, int);
    virtual void DoEditPaste ();
    virtual void DoEditCut   ();

    // drag & drop support
    virtual BOOL       DoDragDrop (int, int, int, int, ECopyWhat);
    virtual DROPEFFECT DoDragOver (COleDataObject*, DWORD dwKeyState);
    virtual BOOL       DoDrop     (COleDataObject*, DROPEFFECT);

    // tooltips support
    //virtual const char* GetTooltipString (TooltipCellContexts&) const;

    // GridSourceBase extended data source interfaces
    virtual void SetCols (int);

    BASE_SOURCE_PROPERTY(bool, ShowFixCol);
    BASE_SOURCE_PROPERTY(bool, ShowFixRow);
    BASE_SOURCE_PROPERTY(bool, ShowRowEnumeration);
    BASE_SOURCE_PROPERTY(bool, ShowTransparentFixCol);
    BASE_SOURCE_PROPERTY(int,  MaxShowLines);

    BASE_SOURCE_PROPERTY(EOrientation, Orientation);
    bool IsTableOrientation () const                                    { return m_Orientation == eoVert; }
    bool IsFormOrientation () const                                     { return m_Orientation == eoHorz; }
    
    BASE_SOURCE_COL_PROPERTY(string,           Header);      // Set/GetColHeader
    BASE_SOURCE_COL_PROPERTY(int,              CharWidth);   // Set/GetColCharWidth
    BASE_SOURCE_COL_PROPERTY(int,              CharHeight);  // Set/GetColCharHeight
    BASE_SOURCE_COL_PROPERTY(EColumnAlignment, Alignment);   // Set/GetColAlignment

    // First fix column image support
    virtual int GetImageIndex  (int row) const = 0;
    virtual void SetImageIndex (int row, int image) = 0;
    void SetImageList (CImageList* imageList, BOOL deleteImageList);

    int  GetFixSize (EOrientation rot, eDirection dir) const            { return m_FixSize[rot][dir]; }
    void SetFixSize (EOrientation rot, eDirection dir, int size)        { m_FixSize[rot][dir] = size; }
    int  GetColumnCount () const                                        { return m_ColumnInfo.size(); }
    virtual bool IsTextField (int /*col*/) const                        { return false;}

protected:
    void DrawDecFrame (DrawCellContexts&) const;
    virtual void CopyColumnNames (std::string&, int, int) const;
    virtual int CopyToString (std::string&, int, int, int, int, ECopyWhat, int); // return actual format
    virtual void GetCellStr (std::string&, int row, int col) const = 0;
    void GetCellStr (std::wstring& buff, int row, int col) const
    {
        std::string utf8buff;
        GetCellStr(utf8buff, row, col);
        buff = Common::wstr(utf8buff);
    }

    struct ColumnInfo
    {
        string           m_Header;
        int              m_CharWidth;
        int              m_CharHeight;
        EColumnAlignment m_Alignment;

        ColumnInfo () : m_CharHeight(1), m_CharWidth(10), m_Alignment(ecaLeft) {}
    };

    Common::nvector<ColumnInfo> m_ColumnInfo;

    CEdit* m_Editor;
    CImageList* m_ImageList;
    BOOL m_bDeleteImageList;
    
    int m_FixSize[2][2]; // the first index is Orientation, the second is Dir

    friend class GridStringOleSource;
};

class GridStringSource : public GridSourceBase
{
public:
    GridStringSource ()                                                 { Clear(); }
    // AGridSource interface implementation
    virtual void Clear ();
    virtual int  Append ();
    virtual int  Insert (int row);
    virtual void Delete (int row);
    virtual void DeleteAll ();
    
    virtual int GetCount (eDirection dir) const;
    
    virtual int GetSize (int pos, eDirection) const;

    virtual int  ExportText (std::ostream&, int row = -1, int nrows = -1, int col = -1, int ncols = -1, int format = -1) const;
    virtual void GetCellStrForExport (std::string&, int line, int col, int subformat = 0) const;

    // GridStringSource extended data source interfaces
    // Data manipulation
    void ReserveMore (int nrows)                                        { m_table.ReserveMore(nrows); }

    const string& GetString (int row, int col) const                    { return m_table.GetString(row, col); }
    string&       GetString (int row, int col)                          { return m_table.GetString(row, col); }
    void          SetString (int row, int col, const string& str)       { m_table.SetString(row, col, str); }

    // GridSourceBase interface implementation
    virtual void SetImageIndex (int row, int image)                     { m_table.SetImageIndex(row, image); }
    virtual int  GetImageIndex (int row) const                          { return m_table.GetImageIndex(row); }
    virtual void SetCols (int cols);
    virtual void GetCellStr (std::string& str, int row, int col) const  { if (IsTableOrientation()) str = GetString(row, col); else str = GetString(col, row); }

    // Data saving settings
    BASE_SOURCE_STATIC_PROPERTY(ETextFormat, OutputFormat);
    BASE_SOURCE_STATIC_PROPERTY(char, FieldDelimiterChar);
    BASE_SOURCE_STATIC_PROPERTY(char, QuoteChar);
    BASE_SOURCE_STATIC_PROPERTY(char, QuoteEscapeChar);
    BASE_SOURCE_STATIC_PROPERTY(char, PrefixChar);
    BASE_SOURCE_STATIC_PROPERTY(bool, OutputWithHeader);
    BASE_SOURCE_STATIC_PROPERTY(bool, ColumnNameAsAttr);
    BASE_SOURCE_STATIC_PROPERTY(string, XmlRootElement   );
    BASE_SOURCE_STATIC_PROPERTY(string, XmlRecordElement );
    BASE_SOURCE_STATIC_PROPERTY(int,    XmlFieldElementCase);


    void GetMaxColumnWidthByFirstLine (Common::nvector<size_t>&, int from, int to) const;

private:
    StringTable m_table;
};

}//namespace OG2

#endif//__GridSourceBase_h__
