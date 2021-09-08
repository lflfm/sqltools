/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2020 Aleksey Kochetov

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
#include <time.h>
#include <iomanip>
#include "OCI8/Datatypes.h"
#include "OCI8/Statement.h"
#include "OCI8/Conversions.h"
#include "COMMON/ExceptionHelper.h"

#if !(OCI_DTYPE_LAST < SHRT_MAX)
#error("\tERROR: assumption OCI_DTYPE_LAST < SHORT_MAX is wrong!\n") 
#endif

#pragma message("\tBUG: check all memcpy!\n") 

// 04.09.2003 bug fix, wrong date for year less then 2000 (introduced in 141)
// 26.10.2003 workaround for oracle 8.1.6, removed trailing '0' for long columns and trigger text
// 08.07.2004 bug fix, Memory corruption on a query with blob columns
// 07.02.2005 (Ken Clubok) R1105003: Bind variables
// 13.03.2005 (ak) R1105003: Bind variables

    using namespace std;

namespace OCI8
{
///////////////////////////////////////////////////////////////////////////////
// OCI8::Variable
///////////////////////////////////////////////////////////////////////////////
const string Variable::m_null;

Variable::Variable (ub2 type, sb4 size)
: m_threadId(GetCurrentThreadId())
{
    m_type = type;
    m_indicator = OCI_IND_NULL;
    m_buffer_size = m_size = size;
    m_buffer = new char[m_size];
    memset(m_buffer, 0, m_size);
    m_owner = true;
}

Variable::Variable (dvoid* buffer, ub2 type, sb4 size)
: m_threadId(GetCurrentThreadId())
{
    m_type = type;
    m_indicator = OCI_IND_NULL;
    m_buffer_size = m_size = size;
    m_buffer = buffer;
    m_owner = false;
}

Variable::~Variable ()
{
    try { EXCEPTION_FRAME;

        if (m_owner)
            delete[] (char*)m_buffer;

        _ASSERTE(m_threadId == GetCurrentThreadId());
    }
    _DESTRUCTOR_HANDLER_;
}

void Variable::GetString (string&, const string& /*null*/) const
{
    ASSERT_EXCEPTION_FRAME;
    _RAISE(Exception(0, "OCI8::Variable::GetString(): method is not implemented."));
}

void Variable::Define (Statement& sttm, int pos)
{
    OCIDefine* defhp;
    sttm.GetConnect().CHECK(
        OCIDefineByPos(sttm.GetOCIStmt(), &defhp, sttm.GetOCIError(), pos,
                        m_buffer, m_buffer_size, m_type, &m_indicator, &m_ret_size, 0, OCI_DEFAULT));
}

/** @brief Bind variable by position.
 *
 *  @arg sttm: Statement to bind - must be prepared.
 *  @arg pos: Position of bind variable within statement.
 */
void Variable::Bind (Statement& sttm, int pos)
{
    OCIBind* bndhp;

    switch (m_type)
    {
    case SQLT_CLOB:
    case SQLT_BLOB:
    case SQLT_BFILEE:
    case SQLT_CFILEE:
        sttm.GetConnect().CHECK(
            OCIBindByPos(sttm.GetOCIStmt(), &bndhp, sttm.GetOCIError(), pos,
                        &m_buffer, m_size, m_type, &m_indicator, 0, 0, 0, 0, OCI_DEFAULT));
        break;
    default:
        sttm.GetConnect().CHECK(
            OCIBindByPos(sttm.GetOCIStmt(), &bndhp, sttm.GetOCIError(), pos,
                        m_buffer, m_size, m_type, &m_indicator, 0, 0, 0, 0, OCI_DEFAULT));
        break;
    }
}

/** @brief Bind variable by name.
 *
 *  @arg sttm: Statement to bind - must be prepared.
 *  @arg name: Name of bind variable.
 */
void Variable::Bind (Statement& sttm, const char* name)
{
    OCIBind* bndhp;

    wstring wname = Common::wstr(name);

    switch (m_type)
    {
    case SQLT_CLOB:
    case SQLT_BLOB:
    case SQLT_BFILEE:
    case SQLT_CFILEE:
        sttm.GetConnect().CHECK(
            OCIBindByName(sttm.GetOCIStmt(), &bndhp, sttm.GetOCIError(), (OraText*)wname.c_str(), wname.length()*sizeof(wchar_t),
                        &m_buffer, m_size, m_type, &m_indicator, 0, 0, 0, 0, OCI_DEFAULT));
        break;
    default:
        sttm.GetConnect().CHECK(
            OCIBindByName(sttm.GetOCIStmt(), &bndhp, sttm.GetOCIError(), (OraText*)wname.c_str(), wname.length()*sizeof(wchar_t),
                        m_buffer, m_size, m_type, &m_indicator, 0, 0, 0, 0, OCI_DEFAULT));
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// OCI8::XmlTypeVariable
///////////////////////////////////////////////////////////////////////////////
#ifdef XMLTYPE_SUPPORT
XmlTypeVariable::XmlTypeVariable (Connect& connect, int limit)
    : Variable(0, static_cast<ub2>(SQLT_NTY), 0),
    m_connect(connect),
    m_docnode(0),
    m_indicator(OCI_IND_NULL),
    m_limit(limit)
{
}

XmlTypeVariable::~XmlTypeVariable ()
{
    if (m_docnode)
        m_connect.CHECK(OCIObjectFree(m_connect.GetOCIEnv(), m_connect.GetOCIError(), m_docnode, OCI_OBJECTFREE_FORCE));
}

//
// NOTE: OCIObjectFree has to move in BeforeFetch method
//
void XmlTypeVariable::GetString (string& strbuff, const string& null) const
{
    if (m_docnode)
    {
        if (m_indicator && *m_indicator == OCI_IND_NULL /*IsNull()*/)
            strbuff = null;
        else
        {
            std::auto_ptr<char> ptr(new char[m_limit]);
            size_t length = m_connect.XmlSaveDom((XMLNode*)m_docnode, ptr.get(), m_limit);
            strbuff.assign(ptr.get(), length);
        }
    }
}

void XmlTypeVariable::Define (Statement& sttm, int pos)
{
    OCIDefine* defhp;
    sttm.GetConnect().CHECK(
        OCIDefineByPos(sttm.GetOCIStmt(), &defhp, sttm.GetOCIError(), pos,
                        &m_buffer, 0, m_type, &m_indicator, 0, 0, OCI_DEFAULT));

    ub4 obj_size = sizeof(void*);
    ub4 ind_size = 0;
    sttm.GetConnect().CHECK(
        OCIDefineObject(defhp, sttm.GetOCIError(), sttm.GetConnect().GetXMLType(), 
                        (dvoid**)&m_docnode, &obj_size, (dvoid**)&m_indicator, &ind_size));
}

void XmlTypeVariable::BeforeFetch ()
{
    if (m_docnode)
    {
        m_connect.CHECK(OCIObjectFree(m_connect.GetOCIEnv(), m_connect.GetOCIError(), m_docnode, OCI_OBJECTFREE_FORCE));
        const_cast<XmlTypeVariable*>(this)->m_docnode = 0;
    }
}
#endif//XMLTYPE_SUPPORT

///////////////////////////////////////////////////////////////////////////////
// OCI8::RefCursorVariable
//      limited functionality, can be bound
///////////////////////////////////////////////////////////////////////////////

RefCursorVariable::RefCursorVariable (Connect& connect, int prefetch)
    : Variable(0, static_cast<ub2>(SQLT_RSET), 0),
    m_connect(connect),
    m_sttmp(0),
    m_prefetch(prefetch)
{
    CHECK_ALLOC(OCIHandleAlloc(GetOCIEnv(), (dvoid**)&m_sttmp, OCI_HTYPE_STMT, 0, 0));
}

RefCursorVariable::~RefCursorVariable ()
{
    try { EXCEPTION_FRAME;

        if (m_sttmp)
            OCIHandleFree(m_sttmp, OCI_HTYPE_STMT);

        m_sttmp = 0;
    }
    _DESTRUCTOR_HANDLER_;
}

void RefCursorVariable::GetString (string& str, const string& /*null = m_null*/) const
{
    str = "Use PRINT varible to see the result set";
}

void RefCursorVariable::Bind (Statement& sttm, int pos)
{
    //8171  1315603 when fetching NULL data.
    //              ie: indp / rcodep may be incorrect.
    //              Workaround: Set OCI_ATTR_PREFETCH_ROWS to 0
    //              This problem was introduced in 8.1.6
    if (Connect::GetClientVersion() >= ecvClient9X)
        CHECK(OCIAttrSet(m_sttmp, OCI_HTYPE_STMT, &m_prefetch, 0, OCI_ATTR_PREFETCH_ROWS, GetOCIError()));

    OCIBind* bndhp;

    sttm.GetConnect().CHECK(
        OCIBindByPos(sttm.GetOCIStmt(), &bndhp, sttm.GetOCIError(), pos,
                    &m_sttmp, 0, SQLT_RSET, 0, 0, 0, 0, 0, OCI_DEFAULT));
}

void RefCursorVariable::Bind (Statement& sttm, const char* name)
{
    OCIBind* bndhp;

    wstring wname = Common::wstr(name);

    sttm.GetConnect().CHECK(
        OCIBindByName(sttm.GetOCIStmt(), &bndhp, sttm.GetOCIError(), (OraText*)wname.c_str(), wname.length()*sizeof(wchar_t),
                    &m_sttmp, 0, SQLT_RSET, 0, 0, 0, 0, 0, OCI_DEFAULT));
}

bool RefCursorVariable::IsStateInitialized ()
{
    ub4 state;

    CHECK(OCIAttrGet(m_sttmp, OCI_HTYPE_STMT, &state, (ub4 *)0, OCI_ATTR_STMT_STATE, GetOCIError()));

    return state == OCI_STMT_STATE_INITIALIZED;
}

bool RefCursorVariable::IsStateExecuted ()
{
    ub4 state;

    CHECK(OCIAttrGet(m_sttmp, OCI_HTYPE_STMT, &state, (ub4 *)0, OCI_ATTR_STMT_STATE, GetOCIError()));

    return state == OCI_STMT_STATE_EXECUTED;
}

bool RefCursorVariable::IsStateEndOfFetch ()
{
    ub4 state;

    CHECK(OCIAttrGet(m_sttmp, OCI_HTYPE_STMT, &state, (ub4 *)0, OCI_ATTR_STMT_STATE, GetOCIError()));

    return state == OCI_STMT_STATE_END_OF_FETCH;
}

///////////////////////////////////////////////////////////////////////////////
// OCI8::NativeOciVariable
///////////////////////////////////////////////////////////////////////////////

NativeOciVariable::NativeOciVariable (Connect& connect, ub2 sqlt_type, ub2 oci_handle_type)
    : Variable(0, static_cast<ub2>(sqlt_type), 0),
    m_connect(connect),
    m_oci_handle_type(oci_handle_type)
{
    m_connect.CHECK(OCIDescriptorAlloc(m_connect.GetOCIEnv(), &m_buffer, m_oci_handle_type, 0, 0));
}

NativeOciVariable::~NativeOciVariable ()
{
    try { EXCEPTION_FRAME;
	    m_connect.CHECK(OCIDescriptorFree(m_buffer, m_oci_handle_type));
        m_buffer = 0;
    }
    _DESTRUCTOR_HANDLER_;
}

void NativeOciVariable::Define (Statement& sttm, int pos)
{
    OCIDefine* defhp;
    sttm.GetConnect().CHECK(
        OCIDefineByPos(sttm.GetOCIStmt(), &defhp, sttm.GetOCIError(), pos,
                        &m_buffer, 0, m_type, &m_indicator, 0, 0, OCI_DEFAULT));
}

///////////////////////////////////////////////////////////////////////////////
// OCI8::StringVar
///////////////////////////////////////////////////////////////////////////////

StringVar::StringVar (int size)
: Variable(SQLT_LVC, size * sizeof(wchar_t) + sizeof(int))
{
}

StringVar::StringVar (const string& str)
: Variable(SQLT_LVC, str.size() * sizeof(wchar_t) + sizeof(int))
{
    Assign(str.c_str(), str.size());
}

StringVar::StringVar (const char* str, int size)
: Variable(SQLT_LVC, (size = (size != -1 ? size : (str ? strlen(str) : 0))) * sizeof(wchar_t) + sizeof(int))
{
    Assign(str, size);
}

void StringVar::Assign (const char* _str, int _size)
{
    ASSERT_EXCEPTION_FRAME;

    if (_size == -1)
        _size = strlen(_str);

    wstring str = Common::wstr(_str, _size);

    int size = str.length() * sizeof(wchar_t);

    if (size)
    {
        if ((size + static_cast<int>(sizeof(int))) > m_buffer_size)
            _RAISE(Exception(0, "OCI8::StringVar::Assign(): String buffer overflow!"));

        m_indicator = OCI_IND_NOTNULL;

        *(int*)m_buffer = str.length();

        m_size = size + sizeof(int);

        memcpy((char*)m_buffer + sizeof(int), str.c_str(), size);
    }
    else
    {
        m_indicator = OCI_IND_NULL;
    }
}

void StringVar::Assign (const string& str)
{
    Assign(str.data(), str.size());
}

void StringVar::GetString (std::string& strbuff, const std::string& null) const
{
    if (IsNull())
        strbuff = null;
    else
    {
        // 26.10.2003 workaround for oracle 8.1.6, removed trailing '0' for long columns and trigger text
        int length = *(int*)m_buffer;
        //int length2 = length;

        for (; length > 0; length--)
            if ( ((const wchar_t*)((char*)m_buffer + sizeof(int)))[length-1] != 0 ) 
                break;

        //strbuff = Common::str((wchar_t*)((char*)m_buffer + sizeof(int)), length2);
        //_ASSERT(length == length2);
        strbuff = Common::str((wchar_t*)((char*)m_buffer + sizeof(int)), length);
    }
}

///////////////////////////////////////////////////////////////////////////////
// OCI8::NumberVar
///////////////////////////////////////////////////////////////////////////////
const char overflow[] = "###"; 

NumberVar::NumberVar (Connect& connect)
: Variable(&m_value, SQLT_VNU, sizeof(m_value)),
m_connect(connect)
{
    OCINumberSetZero(m_connect.GetOCIError(), &m_value);
}

NumberVar::NumberVar (Connect& connect, const string& numberFormat)
: Variable(&m_value, SQLT_VNU, sizeof(m_value)),
m_connect(connect),
m_numberFormat(Common::wstr(numberFormat))
{
    OCINumberSetZero(m_connect.GetOCIError(), &m_value);
}

NumberVar::NumberVar (Connect& connect, int value)
: Variable(&m_value, SQLT_VNU, sizeof(m_value)),
m_connect(connect)
{
    Assign(value);
}

NumberVar::NumberVar (Connect& connect, double value)
: Variable(&m_value, SQLT_VNU, sizeof(m_value)),
m_connect(connect)
{
    Assign(value);
}

void NumberVar::Assign (int val)
{
    m_connect.CHECK(OCINumberFromInt(m_connect.GetOCIError(), &val, sizeof(val), OCI_NUMBER_SIGNED, &m_value));
    m_indicator = OCI_IND_NOTNULL;
}

void NumberVar::Assign (double val)
{
    m_connect.CHECK(OCINumberFromReal(m_connect.GetOCIError(), &val, sizeof(val), &m_value));
    m_indicator = OCI_IND_NOTNULL;
}

void NumberVar::GetString (std::string& strbuff, const std::string& null) const
{
    if (IsNull())
        strbuff = null;
    else
    {
        OraText buffer[256];
        ub4 buffer_size = sizeof(buffer);
        ub1 fmt_length = (ub1)m_numberFormat.length() * sizeof(wchar_t);
        const OraText* fmt = fmt_length ? (const OraText*)m_numberFormat.c_str() : 0;

        try
        {
            //TODO: improve it - cache it or something else
            //string nls_param;
            //if (!m_connect.GetNlsNumericCharacters().empty())
            //{
            //    nls_param = "NLS_NUMERIC_CHARACTERS='";
            //    nls_param += m_connect.GetNlsNumericCharacters();
            //    nls_param += '\'';
            //}

            m_connect.CHECK(OCINumberToText(m_connect.GetOCIError(), &m_value, 
                fmt, fmt_length, 
                //(const OraText*)nls_param.c_str(), nls_param.length(),
                0/*nls_params*/, 0/*nls_p_length*/,
                &buffer_size, buffer));

            strbuff = Common::str(reinterpret_cast<wchar_t*>(buffer), buffer_size / sizeof(wchar_t));
        }
        catch (const Exception& x)
        {
            if (x != 22065) throw;
            strbuff = overflow;
        }
    }
}

int NumberVar::ToInt (int null) const
{
    if (IsNull()) return null;

    int val;
    m_connect.CHECK(OCINumberToInt(m_connect.GetOCIError(), &m_value, sizeof(val), OCI_NUMBER_SIGNED, &val));
    return val;
}

double NumberVar::ToDouble (double null) const
{
    if (IsNull()) return null;

    double val;
    m_connect.CHECK(OCINumberToReal(m_connect.GetOCIError(), &m_value, sizeof(val), &val));
    return val;
}

///////////////////////////////////////////////////////////////////////////////
// OCI8::DateVar
///////////////////////////////////////////////////////////////////////////////
DateVar::DateVar (Connect& connect)
: Variable(&m_value, SQLT_ODT, sizeof(m_value)),
m_connect(connect)
{
}

DateVar::DateVar (Connect& connect, const string& dateFormat)
: Variable(&m_value, SQLT_ODT, sizeof(m_value)),
m_connect(connect),
m_dateFormat(Common::wstr(dateFormat))
{
    if (m_dateFormat.empty())
        m_dateFormat = Common::wstr(m_connect.GetNlsDateFormat());
}

void DateVar::GetString (std::string& strbuff, const std::string& null) const
{
    if (IsNull())
        strbuff = null;
    else
    {
        wchar_t buffer[256];
        size_t buffer_size = sizeof(buffer);

        //wstring nlsLanguage = Common::wstr(m_connect.GetNlsLanguage());

        m_connect.CHECK(OCIDateToText(m_connect.GetOCIError(), &m_value, 
            (const OraText*)m_dateFormat.c_str(), static_cast<ub1>(m_dateFormat.length() * sizeof(wchar_t)), 
            //0/*const oratext *fmt*/, 0/*ub1 fmt_length*/, 
            //(const OraText*)nlsLanguage.c_str(), nlsLanguage.length() * sizeof(wchar_t),
            0/*nls_params*/, 0/*nls_p_length*/,
            &buffer_size, (OraText*)buffer));

        strbuff = Common::str(reinterpret_cast<wchar_t*>(buffer), buffer_size / sizeof(wchar_t));
    }
}

///////////////////////////////////////////////////////////////////////////////
// OCI8::TimestampVar
///////////////////////////////////////////////////////////////////////////////
    ub2 TimestampVar::sqlt_dtype_map (ub2 dtype)
    {
        switch (dtype)
        {
        case SQLT_DATE:          return OCI_DTYPE_DATE;
        case SQLT_TIME:          return OCI_DTYPE_TIME;
        case SQLT_TIMESTAMP:     return OCI_DTYPE_TIMESTAMP;
        case SQLT_TIMESTAMP_TZ:  return OCI_DTYPE_TIMESTAMP_TZ;
        case SQLT_TIMESTAMP_LTZ: return OCI_DTYPE_TIMESTAMP_LTZ;
        }
        _RAISE(Exception(0, "TimestampVar::TimestampVar(): unsupported SQLT_."));
    }

TimestampVar::TimestampVar (Connect& connect, ESubtype subtype)
: NativeOciVariable(connect, static_cast<ub2>(subtype), sqlt_dtype_map(static_cast<ub2>(subtype)))
{
}

TimestampVar::TimestampVar (Connect& connect, ESubtype subtype, const string& dateFormat)
: NativeOciVariable(connect, static_cast<ub2>(subtype), sqlt_dtype_map(static_cast<ub2>(subtype))),
m_dateFormat(Common::wstr(dateFormat))
{
    if (m_dateFormat.empty())
    {
        switch (subtype)
        {
        case SQLT_DATE:          m_dateFormat = Common::wstr(m_connect.GetNlsDateFormat()); break;
        case SQLT_TIME:          m_dateFormat = Common::wstr(m_connect.GetNlsTimeFormat()); break;
        case SQLT_TIMESTAMP:     m_dateFormat = Common::wstr(m_connect.GetNlsTimestampFormat()); break;
        case SQLT_TIMESTAMP_TZ:  m_dateFormat = Common::wstr(m_connect.GetNlsTimestampTzFormat()); break;
        case SQLT_TIMESTAMP_LTZ: m_dateFormat = Common::wstr(m_connect.GetNlsTimestampTzFormat()); break;
        }
    }
}

void TimestampVar::GetString (std::string& strbuff, const std::string& null) const
{
    if (IsNull())
        strbuff = null;
    else
    {
        wchar_t buffer[256];
        size_t buffer_size = sizeof(buffer);
        size_t fmt_length = m_dateFormat.length();
        const wchar_t* fmt = fmt_length ? m_dateFormat.c_str() : 0;

        m_connect.DateTimeToText((const OCIDateTime*)m_buffer, 
            fmt, fmt_length * sizeof(wchar_t), 0/*fsprec*/, 
            //m_connect.GetNlsLanguage().c_str(), m_connect.GetNlsLanguage().length(),
            0/*lang_name*/, 0/*lang_length*/,
            buffer, &buffer_size);

        strbuff = Common::str(buffer, buffer_size /  sizeof(wchar_t));
    }
}

///////////////////////////////////////////////////////////////////////////////
// OCI8::IntervalVar
///////////////////////////////////////////////////////////////////////////////
    ub2 IntervalVar::sqlt_dtype_map (ub2 dtype)
    {
        switch (dtype)
        {
        case SQLT_INTERVAL_YM:  return OCI_DTYPE_INTERVAL_YM;
        case SQLT_INTERVAL_DS:  return OCI_DTYPE_INTERVAL_DS;
        }
        _RAISE(Exception(0, "TimestampVar::TimestampVar(): unsupported SQLT_."));
    }

IntervalVar::IntervalVar (Connect& connect, ESubtype subtype)
: NativeOciVariable(connect, static_cast<ub2>(subtype), sqlt_dtype_map(static_cast<ub2>(subtype)))
{
}

void IntervalVar::GetString (std::string& strbuff, const std::string& null) const
{
    if (IsNull())
        strbuff = null;
    else
    {
        wchar_t buffer[256];
        size_t result_byte_size;

        m_connect.IntervalToText((const OCIInterval*)m_buffer, 0/*lfprec*/, 2/*fsprec*/, buffer, sizeof(buffer), &result_byte_size);

        strbuff = Common::str(buffer, result_byte_size / sizeof(wchar_t));
    }
}

///////////////////////////////////////////////////////////////////////////////
// OCI8::DummyVar
///////////////////////////////////////////////////////////////////////////////
const std::string DummyVar::m_unsupportedStr = "<Unsupported>";
const std::string DummyVar::m_skippedStr = "<Skipped>";

DummyVar::DummyVar (bool skipped)
: Variable(0, 0),
m_skipped(skipped)
{
}

void DummyVar::GetString (std::string& strbuff, const std::string& /*null*/) const
{
    strbuff = m_skipped ? m_skippedStr : m_unsupportedStr;
}

///////////////////////////////////////////////////////////////////////////////
// OCI8::LobVar
///////////////////////////////////////////////////////////////////////////////

LobVar::LobVar (Connect& connect, ELobSubtype subtype, int limit, ECharForm charForm /*ecfImplicit*/)
: NativeOciVariable(connect, static_cast<ub2>(subtype), OCI_DTYPE_LOB),
  m_limit(limit),
  m_charForm(charForm)
  //m_temporary(false)
{
}

int LobVar::getLobLength () const
{
    if (IsNull()) return 0;

    ub4 length = 0;
	m_connect.CHECK(OCILobGetLength(m_connect.GetOCISvcCtx(), m_connect.GetOCIError(), (OCILobLocator*)m_buffer, &length));

    if (length > static_cast<ub4>(m_limit))
        length = m_limit;

    return length;
}

void LobVar::getString (char* strbuff, int buffsize, ub2 csid) const
{
    ASSERT_EXCEPTION_FRAME;

    int len = getLobLength();
    int char_size = (csid == OCI_UTF16ID) ? sizeof(wchar_t) : sizeof(char);

    if (char_size * len >= buffsize)
        _RAISE(Exception(0, "OCI8::LobVar::GetString(): String buffer overflow!"));

    if (!IsNull())
    {
	    ub4 offset = 1;
	    ub4 readsize = len;

        m_connect.CHECK(
            OCILobRead(m_connect.GetOCISvcCtx(), m_connect.GetOCIError(), (OCILobLocator*)m_buffer,
                      &readsize, offset, strbuff, (ub4)buffsize, 0, 0,  csid, (ub1)m_charForm)
            );
    }

    //strbuff[char_size * len] = 0;
}

//void LobVar::makeTemporary (ub1 lobtype)
//{
//    m_connect.CHECK(
//        OCILobCreateTemporary(m_connect.GetOCISvcCtx(), m_connect.GetOCIError(), (OCILobLocator*)m_buffer,
//            OCI_DEFAULT, OCI_DEFAULT, lobtype, TRUE, OCI_DURATION_SESSION)
//        );
//    m_temporary = true;
//}

void LobVar::GetString (std::string& strbuff, const std::string& null) const
{
    if (IsNull())
        strbuff = null;
    else
    {
        std::wstring wbuff;
        wbuff.resize(getLobLength()+1);
        getString((char*)(wbuff.c_str()), sizeof(wchar_t) * wbuff.size(), OCI_UTF16ID);
        strbuff = Common::str(wbuff);
        if (strbuff.size() > 0 && *strbuff.rbegin() == 0)
            strbuff.erase(strbuff.size()-1);
    }
}

///////////////////////////////////////////////////////////////////////////////
// OCI8::CLobVar
///////////////////////////////////////////////////////////////////////////////
//void CLobVar::MakeTemporary ()
//{
//    makeTemporary(OCI_TEMP_CLOB);
//}

///////////////////////////////////////////////////////////////////////////////
// OCI8::BLobVar
///////////////////////////////////////////////////////////////////////////////

BLobVar::BLobVar (Connect& connect, ELobSubtype subtype, int limit, int hexFormatLength)
: LobVar(connect, subtype, limit),
m_hexFormatLength(hexFormatLength)
{
    _CHECK_AND_THROW_(m_hexFormatLength % 2 == 0,  "OCI8::BLobVar::BLobVar: Hex line length must be even.");
}

BLobVar::BLobVar (Connect& connect, int limit, int hexFormatLength)
: LobVar(connect, elsBLob, limit),
m_hexFormatLength(hexFormatLength)
{
    _CHECK_AND_THROW_(m_hexFormatLength % 2 == 0,  "OCI8::BLobVar::BLobVar: Hex line length must be even.");
}

void BLobVar::GetString (std::string& strbuff, const std::string& null) const
{
    if (IsNull())
        strbuff = null;
    else
    {
        std::string bin_data;
        bin_data.resize(getLobLength()+1);
        getString((char*)(bin_data.c_str()), bin_data.size(), 0);
        
        ostringstream out;
        out << std::hex << std::setfill('0');

        string::const_iterator it = bin_data.begin();
        for (int i = 0; it != bin_data.end(); ++it)
        {
            out  << std::setw(2) << (int)(unsigned char)*it;
            if (!(++i % m_hexFormatLength)) out << endl;
        }

        strbuff = out.str();
    }
}

///////////////////////////////////////////////////////////////////////////////
// OCI8::BFileVar
///////////////////////////////////////////////////////////////////////////////

BFileVar::BFileVar (Connect& connect, int limit, int hexFormatLength)
: BLobVar(connect, elsBFile, limit, hexFormatLength)
{
}

void BFileVar::GetString (string& buff, const string& null) const
{
    m_connect.CHECK(
        OCILobFileOpen(m_connect.GetOCISvcCtx(),m_connect.GetOCIError(), (OCILobLocator*)m_buffer, OCI_FILE_READONLY)
        );
    try
    {
        BLobVar::GetString(buff, null);
    }
    catch (const Exception&)
    {
        m_connect.CHECK(
            OCILobFileClose(m_connect.GetOCISvcCtx(),m_connect.GetOCIError(), (OCILobLocator*)m_buffer)
            );
    }
    m_connect.CHECK(
        OCILobFileClose(m_connect.GetOCISvcCtx(),m_connect.GetOCIError(), (OCILobLocator*)m_buffer)
        );
}

///////////////////////////////////////////////////////////////////////////////
// OCI8::HostArray
///////////////////////////////////////////////////////////////////////////////
const string HostArray::m_null;

HostArray::HostArray (ub2 type, sb4 size, sb4 count)
: m_threadId(GetCurrentThreadId())
{
    m_type = type;
    m_elm_buff_size = size;
    m_count = count;
    m_cur_count = 0;
    m_buffer = new char[m_elm_buff_size * m_count];
    memset(m_buffer, 0, m_elm_buff_size * m_count);
    m_indicators = new sb2[m_count];
    memset(m_indicators, 0, sizeof (sb2) * m_count);
    m_sizes = new ub2[m_count];
    memset(m_sizes, 0, sizeof (ub2) * m_count);
    //m_ret_codes = new ub2[m_count];
}

HostArray::~HostArray ()
{
    try { EXCEPTION_FRAME;

        //delete[] m_ret_codes;
        delete[] m_sizes;
        delete[] m_indicators;
        delete[] (char*)m_buffer;

        _ASSERTE(m_threadId == GetCurrentThreadId());
    }
    _DESTRUCTOR_HANDLER_;
}

void HostArray::Assign (int inx, const dvoid* buffer, ub2 size)
{
    ASSERT_EXCEPTION_FRAME;

    if (inx < m_count)
    {
        if (size <= m_elm_buff_size)
	    {
		    memcpy(((char*)m_buffer) + m_elm_buff_size * inx, buffer, size);
            m_sizes[inx] = size;
	    }
	    else
            _RAISE(Exception(0, "OCI8::HostArray::Assign(): Buffer overflow!"));
    }
    else
        _RAISE(Exception(0, "OCI8::HostArray::Assign(): Out of range!"));
}

void HostArray::Assign (int inx, int val)
{
    Assign(inx, &val, sizeof(val));
}

void HostArray::Assign (int inx, long val)
{
    Assign(inx, &val, sizeof(val));
}

const void* HostArray::At (int inx) const
{
    ASSERT_EXCEPTION_FRAME;

    if (inx >= static_cast<int>(m_cur_count))
        _RAISE(Exception(0, "OCI8::HostArray::At(): Out of range!"));

    return ((const char*)m_buffer) + m_elm_buff_size * inx;
}


void HostArray::Define (Statement& sttm, int pos)
{
    OCIDefine* defhp;
    sttm.GetConnect().CHECK(
        OCIDefineByPos(sttm.GetOCIStmt(), &defhp, sttm.GetOCIError(), pos,
                         m_buffer, m_elm_buff_size, m_type, m_indicators,
                         m_sizes, 0, OCI_DEFAULT));
}

void HostArray::Bind (Statement& sttm, int pos)
{
    OCIBind* bndhp;
    sttm.GetConnect().CHECK(
        OCIBindByPos(sttm.GetOCIStmt(), &bndhp, sttm.GetOCIError(), pos,
                       m_buffer, m_elm_buff_size, m_type, m_indicators,
                       m_sizes, 0, m_count, 0, OCI_DEFAULT));
}

void HostArray::Bind (Statement& sttm, const char* name)
{
    OCIBind* bndhp;
    wstring wname = Common::wstr(name);
    sttm.GetConnect().CHECK(
        OCIBindByName(sttm.GetOCIStmt(), &bndhp, sttm.GetOCIError(), (OraText*)wname.c_str(), wname.length()*sizeof(wchar_t),
                       m_buffer, m_elm_buff_size, m_type, m_indicators,
                       m_sizes, 0, m_count, &m_cur_count, OCI_DEFAULT));
}

///////////////////////////////////////////////////////////////////////////////
// OCI8::StringArray
///////////////////////////////////////////////////////////////////////////////

StringArray::StringArray (int size, int count)
: HostArray(SQLT_LVC, sizeof(wchar_t) * size + sizeof(int), count)
{
}

void StringArray::GetString (int inx, std::string& strbuff, const std::string& null) const
{
    if (IsNull(inx))
        strbuff = null;
    else
    {
        // 26.10.2003 workaround for oracle 8.1.6, removed trailing '0' for long columns and trigger text
        int length = *(int*)At(inx);
        //int length2 = length;

        for (; length > 0; length--)
            if ( ((const wchar_t*)((const char*)at(inx) + sizeof(int)))[length-1] != 0 ) 
                break;

        //strbuff = Common::str((wchar_t*)(at(inx) + sizeof(int)), length2);
        //_ASSERT(length == length2);
        strbuff = Common::str((wchar_t*)((const char*)at(inx) + sizeof(int)), length);
    }
}

void StringArray::GetTime (int, struct tm&, struct tm*) const
{
    ASSERT_EXCEPTION_FRAME;
    _RAISE(Exception(0, "OCI8::StringArray::GetTime(): Cannot cast String to \"tm\"!"));
}

int StringArray::ToInt (int, int) const
{
    ASSERT_EXCEPTION_FRAME;
    _RAISE(Exception(0, "OCI8::StringArray::ToInt(): Cannot cast String to Int!"));
}

__int64 StringArray::ToInt64 (int, __int64) const
{
    ASSERT_EXCEPTION_FRAME;
    _RAISE(Exception(0, "OCI8::StringArray::ToInt64(): Cannot cast Date to Int!"));
}

double StringArray::ToDouble (int, double) const
{
    ASSERT_EXCEPTION_FRAME;
    _RAISE(Exception(0, "OCI8::StringArray::ToDouble(): Cannot cast String to Double!"));
}

///////////////////////////////////////////////////////////////////////////////
// OCI8::NumberArray
///////////////////////////////////////////////////////////////////////////////

NumberArray::NumberArray (int count)
: HostArray(SQLT_STR, sizeof(wchar_t) * 41, count)
{
}

void NumberArray::GetString (int inx, std::string& strbuff, const std::string& null) const
{
    if (IsNull(inx))
        strbuff = null;
    else
        strbuff = Common::str((const wchar_t*)At(inx));
}

void NumberArray::GetTime (int, struct tm&, struct tm*) const
{
    ASSERT_EXCEPTION_FRAME;
    _RAISE(Exception(0, "OCI8::NumberArray::GetTime(): Cannot cast Number to \"tm\"!"));
}

int NumberArray::ToInt (int inx, int null) const
{
    if (IsNull(inx)) return null;

    return _wtoi((const wchar_t*)At(inx));
}

__int64 NumberArray::ToInt64 (int inx, __int64 null) const
{
    if (IsNull(inx)) return null;

    return _wtoi64((const wchar_t*)At(inx));
}

double NumberArray::ToDouble (int inx, double null) const
{
    if (IsNull(inx)) return null;

    return _wtof((const wchar_t*)At(inx));
}

///////////////////////////////////////////////////////////////////////////////
// OCI8::DateArray
///////////////////////////////////////////////////////////////////////////////
const std::string DateArray::m_defaultDateFormat = "%Y.%m.%d %H:%M:%S";

DateArray::DateArray (int count, const std::string& defaultDateFormat)
: HostArray(SQLT_DAT, 7, count),
  m_dateFormat(defaultDateFormat),
  m_dataStrLength(0)
{
    time_t t1;
    time(&t1);
    tm *t2 = gmtime(&t1);
    char buff[80];
    strftime(buff, sizeof(buff), m_dateFormat.c_str(), t2);
    m_dataStrLength = strlen(buff);
}

void DateArray::GetString (int inx, std::string& strbuff, const std::string& null) const
{
    if (IsNull(inx))
        strbuff = null;
    else
    {
        const char* data = (const char*)At(inx);
        tm time;
        time.tm_isdst = time.tm_wday = time.tm_yday = -1;
        time.tm_year = (data[0] - 100) * 100 + (data[1] - 100) - 1900;
        time.tm_mon  = data[2] - 1;
        time.tm_mday = data[3];
        time.tm_hour = data[4] - 1;
        time.tm_min  = data[5] - 1;
        time.tm_sec  = data[6] - 1;
        char buff[80];
        strftime(buff, sizeof buff, m_dateFormat.c_str(), &time);
        strbuff = buff;
    }
}

void DateArray::GetTime (int inx, struct tm& time, struct tm* null /*= 0*/) const
{
    if (!IsNull(inx))
    {
        const char* data = (const char*)At(inx);
        time.tm_isdst = time.tm_wday = time.tm_yday = -1;
        time.tm_year = (data[0] - 100) * 100 + (data[1] - 100) - 1900;
        time.tm_mon  = data[2] - 1;
        time.tm_mday = data[3];
        time.tm_hour = data[4] - 1;
        time.tm_min  = data[5] - 1;
        time.tm_sec  = data[6] - 1;
    }
    else
    {
        if (null)
            time = *null;
        else
            memset(&time, 0, sizeof(time));
    }
}

int DateArray::ToInt (int, int) const
{
    ASSERT_EXCEPTION_FRAME;
    _RAISE(Exception(0, "OCI8::DateArray::ToInt(): Cannot cast Date to Int!"));
}

__int64 DateArray::ToInt64 (int, __int64) const
{
    ASSERT_EXCEPTION_FRAME;
    _RAISE(Exception(0, "OCI8::DateArray::ToInt64(): Cannot cast Date to Int!"));
}

double DateArray::ToDouble (int, double) const
{
    ASSERT_EXCEPTION_FRAME;
    _RAISE(Exception(0, "OCI8::DateArray::ToDouble(): Cannot cast Date to Double!"));
}

///////////////////////////////////////////////////////////////////////////////
};