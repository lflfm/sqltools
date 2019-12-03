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

#ifndef __AFX_STDAFX_H__
#define __AFX_STDAFX_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef WINVER
#define WINVER 0x0501
#endif

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#include <afxcmn.h>			// MFC support for Windows Common Controls
#include <afxmt.h>

#pragma warning ( push )
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4788 )
#pragma warning ( disable : 4238 )
#pragma warning ( disable : 4239 )

#include <xstddef>
#include <COMMON/ExceptionHelper.h>
#pragma warning (disable : 4005)
#pragma message ("ATTENTION: stl macros _RAISE & _THROW are redefined for this project")
#define _RAISE(x)	 do { Common::TraceStackOnThrow(); throw (x); } while(false)
#define _THROW(x, y) do { Common::TraceStackOnThrow(); throw (x(y)); } while(false)
#pragma warning (default : 4005)


#include <algorithm>
#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <new>
#include <string>
#include <sstream>
#include <vector>
#include <use_ansi.h>

#define LPSCSTR static const char*

using std::string;
using std::istringstream;
using std::ostringstream;
using std::ends;
using std::endl;

namespace OCI8
{
    class Connect;
    class Statement;
    class AutoCursor;
    class BuffCursor;
    class StringVar;
    class NumberVar; 
    class DateVar;
    class HostArray;
    class StringArray;
    class NumberArray;
    class DateArray;
    class Exception;
    class UserCancel;
};

typedef OCI8::Connect       OciConnect;
typedef OCI8::Statement     OciStatement;
typedef OCI8::AutoCursor    OciAutoCursor;
typedef OCI8::BuffCursor    OciCursor;
typedef OCI8::StringVar     OciStringVar;
typedef OCI8::NumberVar     OciNumberVar; 
typedef OCI8::DateVar       OciDateVar;
typedef OCI8::Exception     OciException;
typedef OCI8::UserCancel    OciUserCancel;
typedef OCI8::HostArray     OciHostArray;
typedef OCI8::StringArray   OciStringArray;
typedef OCI8::NumberArray   OciNumberArray;
typedef OCI8::DateArray     OciDateArray;

void default_oci_handler (const OciException&, const char*, int);

#define DEFAULT_OCI_HANDLER(x) \
    default_oci_handler(x, __FILE__, __LINE__);

#define _DEFAULT_HANDLER_ \
    catch (const OciException& x) { DEFAULT_OCI_HANDLER(x); } \
    _COMMON_DEFAULT_HANDLER_

#define _OE_DEFAULT_HANDLER_ _COMMON_DEFAULT_HANDLER_

#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4788 )
#pragma warning ( disable : 4238 )
#pragma warning ( disable : 4239 )

#undef isalpha
#undef isupper
#undef islower
#undef isdigit
#undef isxdigit
#undef isspace
#undef ispunct
#undef isalnum
#undef isprint
#undef isgraph
#undef iscntrl
#undef toupper
#undef tolower

inline int isalpha (char ch) { return isalpha((int)(unsigned char)ch); }
inline int isupper (char ch) { return isupper((int)(unsigned char)ch); }
inline int islower (char ch) { return islower((int)(unsigned char)ch); }
inline int isdigit (char ch) { return isdigit((int)(unsigned char)ch); }
inline int isxdigit (char ch) { return isxdigit((int)(unsigned char)ch); }
inline int isspace (char ch) { return isspace((int)(unsigned char)ch); }
inline int ispunct (char ch) { return ispunct((int)(unsigned char)ch); }
inline int isalnum (char ch) { return isalnum((int)(unsigned char)ch); }
inline int isprint (char ch) { return isprint((int)(unsigned char)ch); }
inline int isgraph (char ch) { return isgraph((int)(unsigned char)ch); }
inline int iscntrl (char ch) { return iscntrl((int)(unsigned char)ch); }
inline char toupper (char ch) { return (char)toupper((int)(unsigned char)ch); }
inline char tolower (char ch) { return (char)tolower((int)(unsigned char)ch); }

#ifdef GetRValue
#undef GetRValue
#endif
#ifdef GetGValue
#undef GetGValue
#endif
#ifdef GetBValue
#undef GetBValue
#endif
inline BYTE GetRValue (COLORREF rgb) { return static_cast<BYTE>(0xFF & rgb); }
inline BYTE GetGValue (COLORREF rgb) { return static_cast<BYTE>((0xFF00 & rgb) >> 8); }
inline BYTE GetBValue (COLORREF rgb) { return static_cast<BYTE>((0xFF0000 & rgb) >> 16); }

#include <exception>
#include <memory>
#include <string>
#include <map>
#include <list>
#include <vector>

#define _SCB_REPLACE_MINIFRAME
//#define _SCB_MINIFRAME_CAPTION
#define baseCMyBar CSizingControlBarCF
//#define baseCMyBar CSizingControlBarG
#include "SizeCBar/sizecbar.h"
#include "SizeCBar/scbarg.h"
#include "SizeCBar/scbarcf.h"
#include <afxdlgs.h>
#include <afxcview.h>

#if _MFC_VER <= 0x0600

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES 0xFFFFFFFF
#define VK_OEM_PLUS       0xBB   // '+' any country
#define VK_OEM_COMMA      0xBC   // ',' any country
#define VK_OEM_MINUS      0xBD   // '-' any country
#define VK_OEM_PERIOD     0xBE   // '.' any country
#define CB_GETCOMBOBOXINFO 0x0164
#endif

#else//_MFC_VER > 0x0600

#include <afxdhtml.h>

#undef max
#undef min
using std::max;
using std::min;

#include <boost/noncopyable.hpp>
using boost::noncopyable;

#endif

#if _MFC_VER >= 0x0800
// disable "depricated" warnings
#pragma warning(disable : 4996) 
#endif

#endif//__AFX_STDAFX_H__
