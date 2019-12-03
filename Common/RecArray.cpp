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

#include "stdafx.h"
#include <COMMON\RecArray.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace Common
{

    const UINT cnArraySizeInc = 1024*4; // 1024 * 4 (ptr size)

RecArray::RecArray ()
{
    m_hHeap = 0;
    m_nColumns = m_nRows = 0;
    m_pArray = 0;
    m_nArraySize = m_nRecDataSize = 0;
}

RecArray::~RecArray ()
{
    try {
        if(m_hHeap) Free();
    }
    _DESTRUCTOR_HANDLER_;
}

void RecArray::Format (int col, int recDataSize)
{
  _ASSERTE(!m_hHeap);

      m_nColumns = col;
      m_nRecDataSize = recDataSize;
      m_nRows    = 0;
      m_hHeap    = HeapCreate(0, 16*1024, 0);

    if (!m_hHeap)
        AfxThrowMemoryException();

    m_pArray  = (RecContainer**)HeapAlloc(m_hHeap, HEAP_ZERO_MEMORY, cnArraySizeInc);

    if (!m_pArray)
        AfxThrowMemoryException();

    m_nArraySize = cnArraySizeInc;
}

void RecArray::Free ()
{
    HeapDestroy(m_hHeap);
    m_hHeap    = 0;
    m_pArray   = 0;
    m_nRows    = 0;

    if (m_nColumns || m_nRecDataSize)
        Format(m_nColumns, m_nRecDataSize);
}

void RecArray::Clear ()
{
    m_nColumns =
    m_nRecDataSize = 0;
    Free();
}

RecArray::DataContainer* RecArray::CreateDataContainer (int size)
{
    _ASSERTE(m_hHeap);

    void* ptr = HeapAlloc(m_hHeap, HEAP_ZERO_MEMORY, size + sizeof DataContainer);
    if (!ptr)
        AfxThrowMemoryException();

    return (DataContainer*)ptr;
}

void RecArray::DestroyDataContainer (DataContainer* data)
{
    _ASSERTE(m_hHeap);
    HeapFree(m_hHeap, 0, data);
}

RecArray::RecContainer* RecArray::CreateRecContainer ()
{
    _ASSERTE(m_hHeap);

    void* ptr = HeapAlloc(m_hHeap, HEAP_ZERO_MEMORY, m_nColumns * sizeof(DataContainer*) + m_nRecDataSize);
    if (!ptr)
        AfxThrowMemoryException();

    _ASSERTE(ptr == ((RecContainer*)ptr)->GetData());
    return (RecContainer*)ptr;
}

void RecArray::DestroyRecContainer (RecContainer* rec)
{
    _ASSERTE(m_hHeap);
    HeapFree(m_hHeap, 0, rec);
}

int RecArray::AddRow ()
{
    if (((m_nRows + 2) * sizeof(RecContainer*)) > (UINT)m_nArraySize)
    {
        LPVOID lp = HeapReAlloc(m_hHeap, HEAP_ZERO_MEMORY, m_pArray,
                                m_nArraySize + cnArraySizeInc);
        if (!lp)
            AfxThrowMemoryException();

        m_pArray = (RecContainer**)lp;
        m_nArraySize += cnArraySizeInc;
    }
    int row = m_nRows++;
    m_pArray[row] = CreateRecContainer();
    return row;
}

void RecArray::SetCell (int row, int col, const char* str, int len)
{
    _ASSERTE(str);
    _ASSERTE(row < m_nRows && col < m_nColumns);

    DataContainer*& data = (*(m_pArray[row]))[col];

    if (len == -1)
        len = strlen(str);

    if (data)
        DestroyDataContainer(data);

    data = CreateDataContainer(len+1);

    if (!data)
        AfxThrowMemoryException();

    strcpy((char*)*data, str);

    data->m_nDataSize = len+1;
    data->m_bIndicator = 0;
}

const char* RecArray::GetCellStr (int row, int col) const
{
    _ASSERTE(row < m_nRows && col < m_nColumns);
    return *(*(m_pArray[row]))[col];
}

void RecArray::SetRowData (int row, const BYTE* data, int len)
{
    _ASSERTE(m_nRecDataSize == len);
    memcpy(m_pArray[row]->GetRecData(m_nColumns), data, m_nRecDataSize);
}

const BYTE* RecArray::GetRowData (int row) const
{
    return m_pArray[row]->GetRecData(m_nColumns);
}

BYTE* RecArray::GetRowData (int row)
{
    return m_pArray[row]->GetRecData(m_nColumns);
}

}//namespace Common
