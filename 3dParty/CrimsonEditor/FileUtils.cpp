#include "stdafx.h"
#include "FileUtils.h"

namespace CrimsonEditor
{

#define _SWAP_UCHAR(A, B)		{ _uchar_temp = (A); (A) = (B); (B) = _uchar_temp; }
static UCHAR _uchar_temp;


CString ENCODING_TYPE_DESCRIPTION_FULL[] = {
	/* ENCODING_TYPE_UNKNOWN */			_T("Unknown Encoding"),
	/* ENCODING_TYPE_ASCII */			_T("ASCII Encoding"),
	/* ENCODING_TYPE_UNICODE_LE */		_T("Unicode Encoding (Little Endian)"),
	/* ENCODING_TYPE_UNICODE_BE */		_T("Unicode Encoding (Big Endian)"),
	/* ENCODING_TYPE_UTF8_WBOM */		_T("UTF-8 Encoding (with BOM)"),
	/* ENDOCING_TYPE_UTF8_XBOM */		_T("UTF-8 Encoding (w/o BOM)"),
};

CString ENCODING_TYPE_DESCRIPTION_SHORT[] = {
	/* ENCODING_TYPE_UNKNOWN */			_T("N.A."),
	/* ENCODING_TYPE_ASCII */			_T("ASCII"),
	/* ENCODING_TYPE_UNICODE_LE */		_T("Unicode"),
	/* ENCODING_TYPE_UNICODE_BE */		_T("Unicode"),
	/* ENCODING_TYPE_UTF8_WBOM */		_T("UTF-8"),
	/* ENCODING_TYPE_UTF8_XBOM */		_T("UTF-8"),
};

CString FILE_FORMAT_DESCRIPTION_FULL[] = {
	/* FILE_FORMAT_UNKNOWN */			_T("Unknown Format"),
	/* FILE_FORMAT_DOS     */			_T("DOS Format"),
	/* FILE_FORMAT_UNIX    */			_T("UNIX Format"),
	/* FILE_FORMAT_MAC     */			_T("MAC Format"),
};

CString FILE_FORMAT_DESCRIPTION_SHORT[] = {
	/* FILE_FORMAT_UNKNOWN */			_T("N.A."),
	/* FILE_FORMAT_DOS     */			_T("DOS"),
	/* FILE_FORMAT_UNIX    */			_T("UNIX"),
	/* FILE_FORMAT_MAC     */			_T("MAC"),
};


BOOL DetectEncodingTypeAndFileFormat(LPCTSTR lpszPathName, INT & nEncodingType, INT & nFileFormat)
{
	try {
		CFile file(lpszPathName, CFile::modeRead | CFile::typeBinary);
		UCHAR szBuffer[4096]; INT nCount = file.Read( szBuffer, 4096 );
		file.Close();

		DetectEncodingType(szBuffer, nCount, nEncodingType);
		DetectFileFormat(szBuffer, nCount, nFileFormat);
		TRACE2("EncodingType: %d, FileFormat: %d\n", nEncodingType, nFileFormat);

		return TRUE;

	} catch( CException * ex ) {
		nEncodingType = ENCODING_TYPE_ASCII;
		nFileFormat = FILE_FORMAT_DOS;

		ex->Delete(); 
		return FALSE;
	}
}

BOOL DetectEncodingType(LPVOID lpContents, INT nLength, INT & nEncodingType)
{
	LPBYTE lpBuffer = (LPBYTE)lpContents;

	if( nLength >= 2 && lpBuffer[0] == 0xFF && lpBuffer[1] == 0xFE ) { nEncodingType = ENCODING_TYPE_UNICODE_LE; return TRUE; }
	if( nLength >= 2 && lpBuffer[0] == 0xFE && lpBuffer[1] == 0xFF ) { nEncodingType = ENCODING_TYPE_UNICODE_BE; return TRUE; }
	if( nLength >= 3 && lpBuffer[0] == 0xEF && lpBuffer[1] == 0xBB && lpBuffer[2] == 0xBF ) { nEncodingType = ENCODING_TYPE_UTF8_WBOM; return TRUE; }

	BOOL lookLikeUtf8 = FALSE;
	for (INT i=0,n=0; i<nLength ; i++)
	{
		if (( lpBuffer[i] & 0xE0 ) == 0xC0 ) { n=1; lookLikeUtf8 = TRUE; }		// 110bbbbb
		else if (( lpBuffer[i] & 0xF0 ) == 0xE0 ) { n=2; lookLikeUtf8 = TRUE; } // 1110bbbb
		else if (( lpBuffer[i] & 0xF8 ) == 0xF0 ) { n=3; lookLikeUtf8 = TRUE; } // 11110bbb
		else n=0;
		while(n--)
			if (( ++i == nLength ) || (( lpBuffer[i] & 0xC0 ) !=0x80 )) // n bytes matching 10bbbbbb follow ?
			{
				nEncodingType = ENCODING_TYPE_ASCII;
				return TRUE;
			}
	}
	if ( lookLikeUtf8 )
	{
		nEncodingType = ENCODING_TYPE_UTF8_XBOM;
		return TRUE;
	}else{
		nEncodingType = ENCODING_TYPE_ASCII;
		return TRUE;
	}
}

BOOL DetectFileFormat(LPVOID lpContents, INT nLength, INT & nFileFormat)
{
	LPBYTE lpBuffer = (LPBYTE)lpContents;
	BOOL bHasCR = FALSE, bHasLF = FALSE;

	for( INT i = 0; i < nLength; i++ ) {
		if( lpBuffer[i] == '\r' ) bHasCR = TRUE;
		if( lpBuffer[i] == '\n' ) bHasLF = TRUE;
		if( bHasCR && bHasLF ) { nFileFormat = FILE_FORMAT_DOS; return TRUE; }
	}

	if( ! bHasCR && bHasLF ) nFileFormat = FILE_FORMAT_UNIX;
	else if( bHasCR && ! bHasLF ) nFileFormat = FILE_FORMAT_MAC;
	else nFileFormat = FILE_FORMAT_DOS; // default file format

	return TRUE;
}
 
}; // namespace CrimsonEditor

