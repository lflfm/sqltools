#pragma once

#include <fstream>
using namespace std;

namespace CrimsonEditor
{

// ENCODING TYPE
#define ENCODING_TYPE_UNKNOWN			0x00
#define ENCODING_TYPE_ASCII				0x01
#define ENCODING_TYPE_UNICODE_LE		0x02
#define ENCODING_TYPE_UNICODE_BE		0x03
#define ENCODING_TYPE_UTF8_WBOM			0x04
#define ENCODING_TYPE_UTF8_XBOM			0x05

extern CString ENCODING_TYPE_DESCRIPTION_FULL[];
extern CString ENCODING_TYPE_DESCRIPTION_SHORT[];

// FILE FORMAT
#define FILE_FORMAT_UNKNOWN				0x00
#define FILE_FORMAT_DOS					0x01
#define FILE_FORMAT_UNIX				0x02
#define FILE_FORMAT_MAC					0x03

extern CString FILE_FORMAT_DESCRIPTION_FULL[];
extern CString FILE_FORMAT_DESCRIPTION_SHORT[];

// GLOBAL FUNCTIONS
BOOL DetectEncodingTypeAndFileFormat(LPCTSTR lpszPathName, INT & nEncodingType, INT & nFileFormat);
BOOL DetectEncodingType(LPVOID lpContents, INT nLength, INT & nEncodingType);
BOOL DetectFileFormat(LPVOID lpContents, INT nLength, INT & nFileFormat);

};//namespace CrimsonEditor