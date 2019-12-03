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

// 26.12.2004 (ak) request 1082797 added support for BINARY_FLOAT and BINARY_DOUBLE
// 02.01.2004 (kc) request 1086239 added m_types vector to support CSV export.  May also be useful for future requests in which the datatype of a column must be known.
// 03.01.2003 (ak) removed m_types because we already have type information in m_fields (it's private now but it can be changed)
// 07.02.2005 (Ken Clubok) R1105003: Bind variables

#include "stdafx.h"
#include "OCI8/ACursor.h"

namespace OCI8
{
///////////////////////////////////////////////////////////////////////////////
// AutoCursor - buffered cursor
///////////////////////////////////////////////////////////////////////////////
AutoCursor::AutoCursor (Connect& connect, const char* sttm, int prefetch, int strLimit, int blobHexRowLength)
    : Statement(connect, sttm, prefetch),
    m_IntNull(0),
    m_DoubleNull(0.0),
    m_ExecutionTime(0.0)
{
    m_StringLimit = strLimit;
    m_BlobHexRowLength = blobHexRowLength;
    m_endOfFetch = false;
    m_SkipLobs = false;
}

AutoCursor::AutoCursor (Connect& connect, int prefetch, int strLimit, int blobHexRowLength)
    : Statement(connect, prefetch),
    m_IntNull(0),
    m_DoubleNull(0.0),
    m_ExecutionTime(0.0)
{
    m_StringLimit = strLimit,
    m_BlobHexRowLength = blobHexRowLength;
    m_endOfFetch = false;
    m_SkipLobs = false;
}

AutoCursor::~AutoCursor ()
{
}

void AutoCursor::Attach (RefCursorVariable& refCur)
{
    if (IsOpen()) Close();

    RefCursorVariable tmpCur(m_connect, m_prefetch);
    swap(tmpCur.m_sttmp, refCur.m_sttmp);
    swap(m_sttmp, tmpCur.m_sttmp);
    Define();
}

void AutoCursor::Execute ()
{
    clock64_t startTime = clock64();

    if (GetType() == StmtSelect)
    {
        Statement::Execute(0);
        Define();
    }
    else
    {
        Statement::Execute(1);
    }
    
    SetExecutionTime(double(clock64() - startTime)/CLOCKS_PER_SEC);

    m_endOfFetch = false;
}

bool AutoCursor::Fetch ()
{
    if (!m_endOfFetch)
    {
        vector<counted_ptr<Variable> >::const_iterator it = m_fields.begin();

        for (; it != m_fields.end(); ++it)
            (*it)->BeforeFetch();

        ub4 retRecords = 0;
        m_endOfFetch = Statement::Fetch(1, retRecords) ? false : true;
    }

    return !m_endOfFetch;
}

/** @brief Binds variables to statement.
 *
 * @arg var: Variable to bind.
 * @arg sql: Name of bind variable.
 */
void AutoCursor::Bind(Variable* var, const string& name)
{
	var->Bind(*this, name.c_str());
}


/** @brief Sets up vectors, following a select statement.
 *
 * Each element of m_fields will hold a counted pointer, of the appropriate type to hold the data.
 * Each element of m_fieldNames will hold a string, with the column name.
 *
 */
void AutoCursor::Define ()
{
    m_fields.clear();
    m_fieldNames.clear();

	sb4 cols = 0;
	CHECK(OCIAttrGet(m_sttmp, OCI_HTYPE_STMT,(dvoid *)&cols, (ub4*)0, OCI_ATTR_PARAM_COUNT, GetOCIError()));
	
    int defined = 0;
    for (int col = 0; col < cols; col++) 
    {
        ub1 char_form;
	    ub2 type, size;
	    OCIParam *paramd;
        counted_ptr<Variable> fld;
        char* col_name = 0;
        ub4   col_name_len = 0;

	    CHECK(OCIParamGet(m_sttmp, OCI_HTYPE_STMT, GetOCIError(), (dvoid**)&paramd, col + 1));
	    CHECK(OCIAttrGet((dvoid*)paramd, OCI_DTYPE_PARAM, (dvoid*)&type, 0, OCI_ATTR_DATA_TYPE, GetOCIError()));
	    CHECK(OCIAttrGet((dvoid*)paramd, OCI_DTYPE_PARAM, (dvoid*)&size, 0, OCI_ATTR_DATA_SIZE, GetOCIError()));
        CHECK(OCIAttrGet((dvoid*)paramd, OCI_DTYPE_PARAM, (dvoid*)&col_name, (ub4*)&col_name_len, OCI_ATTR_NAME, GetOCIError()));
        CHECK(OCIAttrGet((dvoid*)paramd, OCI_DTYPE_PARAM, (dvoid*)&char_form, 0, OCI_ATTR_CHARSET_FORM, GetOCIError()));

        switch (type) 
        {
		case SQLT_CLOB:
            if (m_SkipLobs)
                fld = counted_ptr<Variable>(new DummyVar(true));
            else
                fld = counted_ptr<Variable>(new CLobVar(GetConnect(), m_StringLimit, (ECharForm)char_form));
			break;
		case SQLT_BLOB:
            if (m_SkipLobs)
                fld = counted_ptr<Variable>(new DummyVar(true));
            else
                fld = counted_ptr<Variable>(new BLobVar(GetConnect(), m_StringLimit, m_BlobHexRowLength/2));
			break;
		case SQLT_BFILE:
            if (m_SkipLobs)
                fld = counted_ptr<Variable>(new DummyVar(true));
            else
                fld = counted_ptr<Variable>(new BFileVar(GetConnect(), m_StringLimit, m_BlobHexRowLength/2));
			break;
        case SQLT_RID:
        case SQLT_RDD:
            fld = counted_ptr<Variable>(new StringVar(20+1));
			break;
        case SQLT_BIN:
            size *= 2;
        case SQLT_LBI:
            fld = counted_ptr<Variable>(new StringVar((size && size < m_StringLimit) ? size : m_StringLimit));
            break;
		case SQLT_CHR:
		case SQLT_LNG:
		case SQLT_AFC:
            fld = counted_ptr<Variable>(new StringVar((size && size < m_StringLimit) ? size : m_StringLimit));
			break;
        case SQLT_INT:
        case SQLT_FLT:
		case SQLT_NUM:
        case SQLT_IBFLOAT:
        case SQLT_IBDOUBLE:
            if (m_NumberFormat.empty())
                fld = counted_ptr<Variable>(new NumStrVar);
            else
                fld = counted_ptr<Variable>(new NumberVar(GetConnect(), m_NumberFormat));
			break;

		case SQLT_DAT:
        case SQLT_DATE:
            fld = counted_ptr<Variable>(new DateVar(GetConnect(), m_DateFormat));
            break;
        case SQLT_TIME:
            if (Connect::IsTimestampSupported())
                fld = counted_ptr<Variable>(new TimestampVar(GetConnect(), TimestampVar::TIME, m_TimeFormat));
		    break;
        case SQLT_TIMESTAMP:
            if (Connect::IsTimestampSupported())
                fld = counted_ptr<Variable>(new TimestampVar(GetConnect(), (TimestampVar::ESubtype)type, m_TimestampFormat));
			break;
        case SQLT_TIMESTAMP_TZ:
        case SQLT_TIMESTAMP_LTZ:
            if (Connect::IsTimestampSupported())
                fld = counted_ptr<Variable>(new TimestampVar(GetConnect(), (TimestampVar::ESubtype)type, m_TimestampTzFormat));
			break;
        case SQLT_INTERVAL_YM:
        case SQLT_INTERVAL_DS:
            if (Connect::IsIntervalSupported())
                fld = counted_ptr<Variable>(new IntervalVar(GetConnect(), (IntervalVar::ESubtype)type));
			break;

        // Unfortunatelly the following method works only if value is samaller than 4K
        //case SQLT_NTY:
        //    {
        //        const char* type_schema, *type_name;
        //        CHECK(OCIAttrGet((dvoid*)paramd, OCI_DTYPE_PARAM, &type_name, 0, OCI_ATTR_TYPE_NAME, GetOCIError()));
        //        CHECK(OCIAttrGet((dvoid*)paramd, OCI_DTYPE_PARAM, &type_schema, 0, OCI_ATTR_SCHEMA_NAME, GetOCIError()));
        //        if (!strcmp(type_schema, "SYS") && !strcmp(type_name, "XMLTYPE"))
        //        {
        //            fld = counted_ptr<Variable>(new StringVar(m_StringLimit));
        //        }
        //        break;
        //    }
#ifdef XMLTYPE_SUPPORT
        case SQLT_NTY:
            if (Connect::IsXMLTypeSupported())
            {
                const char* type_schema, *type_name;
                CHECK(OCIAttrGet((dvoid*)paramd, OCI_DTYPE_PARAM, &type_name, 0, OCI_ATTR_TYPE_NAME, GetOCIError()));
                CHECK(OCIAttrGet((dvoid*)paramd, OCI_DTYPE_PARAM, &type_schema, 0, OCI_ATTR_SCHEMA_NAME, GetOCIError()));
                if (!strcmp(type_schema, "SYS") && !strcmp(type_name, "XMLTYPE"))
                    fld = counted_ptr<Variable>(new XmlTypeVariable(GetConnect(), m_StringLimit));
			    break;
            }
#endif//XMLTYPE_SUPPORT
        }

        if (!fld.get())
            fld = counted_ptr<Variable>(new DummyVar);

        m_fields.push_back(fld);
        m_fieldNames.push_back(string(col_name, col_name_len));

        if (!dynamic_cast<DummyVar*>(fld.get()))
        {
            Statement::Define(col + 1, *fld);
            defined++;
        }
	}

    if (!defined) 
        THROW_APP_EXCEPTION(Exception(-1, "The query fails because all columns types are currently not supported."));
}

};