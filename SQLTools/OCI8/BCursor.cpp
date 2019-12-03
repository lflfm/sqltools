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

#include "stdafx.h"
#include "OCI8/BCursor.h"

namespace OCI8
{
///////////////////////////////////////////////////////////////////////////////
// BuffCursor - buffered cursor
///////////////////////////////////////////////////////////////////////////////
    string BuffCursor::m_defaultDateFormat = "%Y.%m.%d %H:%M:%S";
    string BuffCursor::m_defaultNull;
    int    BuffCursor::m_defaultIntNull = 0;
    double BuffCursor::m_defaultDoubleNull = 0.0;

BuffCursor::BuffCursor (Connect& connect, const char* sttm, int prefetch, int strLimit)
    : Statement(connect, sttm, prefetch),
    m_dateFormat(m_defaultDateFormat),
    m_null(m_defaultNull),
    m_intNull(m_defaultIntNull),
    m_doubleNull(m_defaultDoubleNull)
{
    m_strLimit = strLimit;
    m_curRecord = static_cast<ub4>(-1);
    m_retRecords = m_buffRecords = 0;
    m_endOfFetch = false;
}

BuffCursor::BuffCursor (Connect& connect, int prefetch, int strLimit)
    : Statement(connect, prefetch),
    m_dateFormat(m_defaultDateFormat),
    m_null(m_defaultNull),
    m_intNull(m_defaultIntNull),
    m_doubleNull(m_defaultDoubleNull)
{
    m_strLimit = strLimit,
    m_curRecord = static_cast<ub4>(-1);
    m_retRecords = m_buffRecords = 0;
    m_endOfFetch = false;
}

BuffCursor::~BuffCursor ()
{
}

void BuffCursor::Execute ()
{
    Statement::Execute(0);

    Define();

    m_curRecord = static_cast<ub4>(-1);
    m_retRecords = m_buffRecords = 0;
    m_endOfFetch = false;
}

bool BuffCursor::Fetch ()
{
    bool retVal = false;

    if (m_curRecord < m_buffRecords - 1) 
    {
        m_curRecord++;
        retVal = true;
    } 
    else if (!m_endOfFetch)
    {
        //SHOW_WAIT;
        //OciBeginProcess(GetConnect());

        ub4 retRecords = 0;
        m_endOfFetch = Statement::Fetch((ub4)m_prefetch, retRecords) ? false : true;
        m_buffRecords = retRecords - m_retRecords;
        m_retRecords = retRecords;
        m_curRecord = 0;

        retVal =  m_buffRecords > 0 ? true : false;

        vector<counted_ptr<HostArray> >::iterator it = m_fields.begin();
        for (; it != m_fields.end(); ++it)
            (*it)->m_cur_count = m_retRecords;

        //OciEndProcess(GetConnect());
    }

    return retVal;
}

void BuffCursor::Define ()
{
    ASSERT_EXCEPTION_FRAME;

    m_fields.clear();
    m_fieldNames.clear();

    ub4 rowSize = 0;
	sb4 cols = 0;
	CHECK(OCIAttrGet(m_sttmp, OCI_HTYPE_STMT,(dvoid *)&cols, (ub4*)0, OCI_ATTR_PARAM_COUNT, GetOCIError()));
	
    for (int col = 0; col < cols; col++) 
    {
	    ub2 type, size;
	    OCIParam *paramd;
        counted_ptr<HostArray> fld;
        char* col_name = 0;
        ub4   col_name_len = 0;

	    CHECK(OCIParamGet(m_sttmp, OCI_HTYPE_STMT, GetOCIError(), (dvoid**)&paramd, col + 1));
	    CHECK(OCIAttrGet((dvoid*)paramd, OCI_DTYPE_PARAM, (dvoid*)&type, 0, OCI_ATTR_DATA_TYPE, GetOCIError()));
	    CHECK(OCIAttrGet((dvoid*)paramd, OCI_DTYPE_PARAM, (dvoid*)&size, 0, OCI_ATTR_DATA_SIZE, GetOCIError()));
        CHECK(OCIAttrGet((dvoid*)paramd, OCI_DTYPE_PARAM, (dvoid*)&col_name, (ub4*)&col_name_len, OCI_ATTR_NAME, GetOCIError()));

        rowSize += ((size && size < m_strLimit) ? size : m_strLimit);

        switch (type) 
        {
		case SQLT_CHR:
		case SQLT_LNG:
		case SQLT_CLOB:
		case SQLT_AFC:
            fld = counted_ptr<HostArray>(new StringArray((size && size < m_strLimit) ? size : m_strLimit, m_prefetch));
			break;
        case SQLT_INT:
        case SQLT_FLT:
		case SQLT_NUM:
            fld = counted_ptr<HostArray>(new NumberArray(m_prefetch));
			break;
		case SQLT_DAT:
            fld = counted_ptr<HostArray>(new DateArray(m_prefetch, m_dateFormat));
			break;
		default:
            _RAISE(Exception(0, "OCI8::BuffCursor::Define(): Unsupported data type!"));
			break;
        }

        m_fields.push_back(fld);
        m_fieldNames.push_back(string(col_name, col_name_len));

        Statement::Define(col + 1, *fld);
	}

    TRACE("OCI8::BuffCursor::Define: total host arrays' size = %u\n", rowSize * m_prefetch);
}

void BuffCursor::Bind (const char* name, const char* value, int len)
{
    ASSERT_EXCEPTION_FRAME;

    if (!(name && value)) _RAISE(Exception(0, "OCI8::BuffCursor::Bind(): Invalid arguments!"));
    
    if (len == -1) len = strlen(value);
    counted_ptr<Variable>& fld = m_boundFields[name] = counted_ptr<Variable>(new StringVar(value, len));

    Statement::Bind(name, *fld.get());
}

void BuffCursor::Bind (const char* name, int value)
{
    ASSERT_EXCEPTION_FRAME;

    if (!(name)) _RAISE(Exception(0, "OCI8::BuffCursor::Bind(): Invalid arguments!"));
    
    counted_ptr<Variable>& fld = m_boundFields[name] = counted_ptr<Variable>(new NumberVar(m_connect, value));

    Statement::Bind(name, *fld.get());
}

void BuffCursor::Bind (const char* name, double value)
{
    ASSERT_EXCEPTION_FRAME;

    if (!(name)) _RAISE(Exception(0, "OCI8::BuffCursor::Bind(): Invalid arguments!"));
    
    counted_ptr<Variable>& fld = m_boundFields[name] = counted_ptr<Variable>(new NumberVar(m_connect, value));

    Statement::Bind(name, *fld.get());
}

bool BuffCursor::IsRecordGood () const
{
    vector<counted_ptr<HostArray> >::const_iterator it = m_fields.begin();

    for (; it != m_fields.end(); ++it)
        if (!(*it)->IsGood(m_curRecord))
            return false;

    return true;
}

bool BuffCursor::IsRecordTruncated () const
{
    vector<counted_ptr<HostArray> >::const_iterator it = m_fields.begin();

    for (; it != m_fields.end(); ++it)
        if ((*it)->IsTruncated(m_curRecord))
            return true;

    return false;
}

};