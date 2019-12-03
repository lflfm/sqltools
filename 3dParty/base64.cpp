/*
  KeePass Password Safe - The Open-Source Password Manager
  Copyright (C) 2003-2005 Dominik Reichl <dominik.reichl@t-online.de>

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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "StdAfx.h"
#include "base64.h"

static const char *g_pCodes =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char g_pMap[256] =
{
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,
	 52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255,
	255, 254, 255, 255, 255,   0,   1,   2,   3,   4,   5,   6,
	  7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,
	 19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,
	255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
	 37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
	 49,  50,  51, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255
};

CBase64Codec::CBase64Codec()
{
}

CBase64Codec::~CBase64Codec()
{
}

bool CBase64Codec::Encode(const BYTE *pIn, DWORD uInLen, BYTE *pOut, DWORD *uOutLen)
{
	DWORD i, len2, leven;
	BYTE *p;

	ASSERT((pIn != NULL) && (uInLen != 0) && (pOut != NULL) && (uOutLen != NULL));

	len2 = ((uInLen + 2) / 3) << 2;
	if((*uOutLen) < (len2 + 1)) return false;

	p = pOut;
	leven = 3 * (uInLen / 3);
	for(i = 0; i < leven; i += 3)
	{
		*p++ = g_pCodes[pIn[0] >> 2];
		*p++ = g_pCodes[((pIn[0] & 3) << 4) + (pIn[1] >> 4)];
		*p++ = g_pCodes[((pIn[1] & 0xf) << 2) + (pIn[2] >> 6)];
		*p++ = g_pCodes[pIn[2] & 0x3f];
		pIn += 3;
	}

	if (i < uInLen)
	{
		DWORD a = pIn[0];
		DWORD b = ((i + 1) < uInLen) ? pIn[1] : 0;
		DWORD c = 0;

		*p++ = g_pCodes[a >> 2];
		*p++ = g_pCodes[((a & 3) << 4) + (b >> 4)];
		*p++ = ((i + 1) < uInLen) ? g_pCodes[((b & 0xf) << 2) + (c >> 6)] : '=';
		*p++ = '=';
	}

	*p = 0; // Append NULL byte
	*uOutLen = p - pOut;
	return true;
}

bool CBase64Codec::Decode(const BYTE *pIn, DWORD uInLen, BYTE *pOut, DWORD *uOutLen)
{
	DWORD t, x, y, z;
	BYTE c;
	DWORD g = 3;

	ASSERT((pIn != NULL) && (uInLen != 0) && (pOut != NULL) && (uOutLen != NULL));

	for(x = y = z = t = 0; x < uInLen; x++)
	{
		c = g_pMap[pIn[x]];
		if(c == 255) continue;
		if(c == 254) { c = 0; g--; }

		t = (t << 6) | c;

		if(++y == 4)
		{
			if((z + g) > *uOutLen) { return false; } // Buffer overflow
			pOut[z++] = (BYTE)((t>>16)&255);
			if(g > 1) pOut[z++] = (BYTE)((t>>8)&255);
			if(g > 2) pOut[z++] = (BYTE)(t&255);
			y = t = 0;
		}
	}

	*uOutLen = z;
	return true;
}

void CBase64Codec::encode (const std::vector<unsigned char>& in, std::string& out)
{
    unsigned long len = in.size();
	unsigned long leven = 3 * (len / 3);
    const unsigned char* ptr = &in[0];

    out.clear();
    out.reserve((((len + 2) / 3) << 2) + 1);

    unsigned long i = 0;
	for (; i < leven; i += 3)
	{
		out += g_pCodes[ptr[0] >> 2];
		out += g_pCodes[((ptr[0] & 3) << 4) + (ptr[1] >> 4)];
		out += g_pCodes[((ptr[1] & 0xf) << 2) + (ptr[2] >> 6)];
		out += g_pCodes[ptr[2] & 0x3f];
		ptr += 3;
	}

	if (i < len)
	{
		unsigned int a = ptr[0];
		unsigned int b = ((i + 1) < len) ? ptr[1] : 0;
		unsigned int c = 0;

		out += g_pCodes[a >> 2];
		out += g_pCodes[((a & 3) << 4) + (b >> 4)];
		out += ((i + 1) < len) ? g_pCodes[((b & 0xf) << 2) + (c >> 6)] : '=';
		out += '=';
	}
}

void CBase64Codec::decode (const std::string& in, std::vector<unsigned char>& out)
{
    out.clear();
    out.reserve(in.length()); // reserve more

    std::string::const_iterator it = in.begin();
	for(unsigned long t = 0, y = 0, g = 3; it != in.end(); ++it)
	{
		unsigned char c = g_pMap[*it];
		if(c == 255) continue;
		if(c == 254) { c = 0; g--; }

		t = (t << 6) | c;

		if(++y == 4)
		{
            out.push_back((unsigned char)((t>>16)&255));
			if(g > 1) out.push_back((unsigned char)((t>>8)&255));
			if(g > 2) out.push_back((unsigned char)(t&255));
			y = t = 0;
		}
	}
}
