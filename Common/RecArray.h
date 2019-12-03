/*
    Copyright (C) 2004 Aleksey Kochetov

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
#ifndef __RECARRAY_H__
#define __RECARRAY_H__

namespace Common
{

class RecArray
{
    struct DataContainer
    {
        int   m_nDataSize;
        char  m_bIndicator;

        void* GetData () const        { return (((BYTE*)this) + sizeof DataContainer); }
        operator int   () const       { return *(int*)GetData(); }
        operator char  () const       { return *(char*)GetData(); }
        operator char* () const       { return (char*)GetData(); }
        operator const char* () const { return (const char*)GetData(); }
        operator const BYTE* () const { return (const BYTE*)GetData(); }

        bool IsNull () const       { return m_bIndicator == -1; }
        bool IsGood () const       { return ((m_bIndicator == 0) || (m_bIndicator == -1)); }

        void SetNull (bool null)   { m_bIndicator = (null) ? -1 : 0; }
    };

    void Set (DataContainer&, int);
    void Set (DataContainer&, char);
    void Set (DataContainer&, char*, int=-1);
    void Set (DataContainer&, BYTE*, int);

    struct RecContainer
    {
        void* GetData () const
            {
              return (((BYTE*)this) /*+ sizeof RecContainer*/);
            }

        DataContainer*& operator [] (int col)
            {
              return ((DataContainer**)GetData())[col];
            }

        BYTE* GetRecData (int cols)
            {
              return ((BYTE*)GetData()) + cols * sizeof(DataContainer*);
            }
    };

    DataContainer* CreateDataContainer (int size);
    void DestroyDataContainer (DataContainer*);
    RecContainer* CreateRecContainer ();
    void DestroyRecContainer (RecContainer*);


    HANDLE m_hHeap;
    int    m_nColumns, m_nRows;
    RecContainer** m_pArray;
    int    m_nArraySize,
           m_nRecDataSize;

public:
    RecArray ();
    ~RecArray ();

    int GetColumns () const { return m_nColumns; }
    int GetRows () const    { return m_nRows; }

    void Format (int col, int recDataSize);
    void Clear  ();

    int  AddRow ();
    void Free   ();

    int  InsertRow (int);
    void DeleteRow (int);

    void SetCell (int row, int col, const char*, int=-1);
    void SetCell (int row, int col, int);
    void SetCell (int row, int col, char);
    void SetCell (int row, int col, const BYTE*, int);

    const int   GetCellSize (int row, int col) const;

    const char* GetCellStr (int row, int col) const;
    const int   GetCellInt (int row, int col) const;
    const char  GetCellChar (int row, int col) const;
    const BYTE* GetCellByte (int row, int col) const;

    void SetRowData (int row, const BYTE*, int);
    const BYTE* GetRowData (int row) const;
    BYTE* GetRowData (int row);

};

}//namespace Common

#endif//__RECARRAY_H__
