/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 2005 Aleksey Kochetov

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
#include "ConnectData.h"
#include "Cryptography\fileenc.h"
#include "Cryptography\prng.h"
#include "base64.h"

    using namespace std;

    const char* ConnectData::PROBE = "probe";

    /* simple entropy collection function that uses the fast timer      */
    /* since we are not using the random pool for generating secret     */
    /* keys we don't need to be too worried about the entropy quality   */

    int entropy_fun(unsigned char buf[], unsigned int len)
    {   unsigned __int64    pentium_tsc[1];
        unsigned int        i;

        QueryPerformanceCounter((LARGE_INTEGER *)pentium_tsc);
        for(i = 0; i < 8 && i < len; ++i)
            buf[i] = ((unsigned char*)pentium_tsc)[i];
        return i;
    }

void ConnectData::checkAndThrowEncryptionCode (int err)
{
    switch (err)
    {
    case GOOD_RETURN:       break;
    case PASSWORD_TOO_LONG: throw PasswordTooLongException();
    case BAD_MODE:          throw InvalidEncryptionModeException();
    default:                throw UnknownEncryptionException();
    }
}

void ConnectData::Encrypt (const string& password, const string& in, string& out) 
{
    vector<unsigned char> buffer;

    int mode = 3 ; // see fileenc.h
    unsigned char salt[16];
    unsigned char pwd_ver[PWD_VER_LENGTH];
    fcrypt_ctx  zcx[1];
    prng_ctx rng[1];    /* the context for the random number pool   */
    prng_init(entropy_fun, rng);                /* initialise RNG   */
    prng_rand(salt, SALT_LENGTH(mode), rng);    /* and the salt     */
	
    for (int i = 0; i < SALT_LENGTH(mode); ++i)
        buffer.push_back(salt[i]);

    int err = fcrypt_init(mode, (const unsigned char*)password.c_str(), password.length(), salt, pwd_ver, zcx);
    checkAndThrowEncryptionCode(err);

    for (int i = 0; i < sizeof(pwd_ver); ++i)
        buffer.push_back(pwd_ver[i]);

    int offset = buffer.size();
    for (string::const_iterator it = in.begin(); it != in.end(); ++it)
        buffer.push_back(*it);

    fcrypt_encrypt(&buffer[0] + offset, in.length(), zcx);

    CBase64Codec::encode(buffer, out);
}

void ConnectData::Decrypt (const string& password, const string& in, string& out) 
{
    if (in.empty())
    {
        out.clear();
        return;
    }

    unsigned long offset = 0;
    vector<unsigned char> buffer;
    CBase64Codec::decode(in, buffer);

    int mode = 3 ; // see fileenc.h
    unsigned char salt[16];
    if (offset + SALT_LENGTH(mode) > buffer.size())
        throw EncryptedDataCorrupt();
    memcpy(salt, &buffer.at(offset), SALT_LENGTH(mode));
    offset += SALT_LENGTH(mode);

    unsigned char org_pwd_ver[PWD_VER_LENGTH];
    if (offset + sizeof(org_pwd_ver) > buffer.size())
        throw EncryptedDataCorrupt();
    memcpy(org_pwd_ver, &buffer.at(offset), sizeof(org_pwd_ver));
    offset += sizeof(org_pwd_ver);

    fcrypt_ctx zcx[1];
    unsigned char pwd_ver[PWD_VER_LENGTH];
    int err = fcrypt_init(mode, (const unsigned char*)password.c_str(), password.length(), salt, pwd_ver, zcx);
    checkAndThrowEncryptionCode(err);

    if (memcmp(org_pwd_ver, pwd_ver, PWD_VER_LENGTH))
        throw InvalidMasterPasswordException();

    if (unsigned int size = buffer.size() - offset)
    {
        fcrypt_decrypt(&buffer.at(offset), size, zcx);
        out.assign((const char*)&buffer.at(offset), buffer.size() - offset);
    }
    else
        out.clear();
}

