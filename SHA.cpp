/*
 ---------------------------------------------------------------------------
 Copyright (c) 2002, Dr Brian Gladman <brg@gladman.me.uk>, Worcester, UK.
 All rights reserved.

 LICENSE TERMS

 The free distribution and use of this software in both source and binary
 form is allowed (with or without changes) provided that:

   1. distributions of this source code include the above copyright
      notice, this list of conditions and the following disclaimer;

   2. distributions in binary form include the above copyright
      notice, this list of conditions and the following disclaimer
      in the documentation and/or other associated materials;

   3. the copyright holder's name is not used to endorse products
      built using this software without specific written permission.

 ALTERNATIVELY, provided that this notice is retained in full, this product
 may be distributed under the terms of the GNU General Public License (GPL),
 in which case the provisions of the GPL apply INSTEAD OF those given above.

 DISCLAIMER

 This software is provided 'as is' with no explicit or implied warranties
 in respect of its properties, including, but not limited to, correctness
 and/or fitness for purpose.
 ---------------------------------------------------------------------------
 Issue Date: 30/11/2002

 This is a byte oriented version of SHA1 that operates on arrays of bytes
 stored in memory. It runs at 22 cycles per byte on a Pentium P4 processor
*/

/* Modified by Camper using extern methods     6.7.2004 */

#include "StdAfx.h"
#include "SHA.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
// CSHA
//

CSHA::CSHA()
{
	CSHA::Reset();
}

void CSHA::Reset()
{
	m_hash = SHA1();
	m_sha.Restart();
}

void CSHA::GetHash(SHA1* pHash) const
{
	*pHash = m_hash;
}

void CSHA::Add(LPCVOID pData, DWORD nLength)
{
	m_sha.Update((const byte *)pData, nLength);
}

void CSHA::Finish()
{
	m_sha.Final(m_hash.b);
}

void CSHA::GetHash(CAICHHash& rHash)
{
	ASSERT(rHash.GetHashSize() == sizeof(SHA1));
	GetHash((SHA1*)rHash.GetRawHash());
}

void CSHA::Finish(CAICHHash& rHash)
{
	Finish();
	GetHash(rHash);
}

//////////////////////////////////////////////////////////////////////
// CSHA get hash string (Base64)

CString CSHA::GetHashString(BOOL bURN) const
{
	SHA1 pHash;
	GetHash( &pHash );
	return HashToString( &pHash, bURN );
}

//////////////////////////////////////////////////////////////////////
// CSHA convert hash to string (Base64)

CString CSHA::HashToString(const SHA1* pHashIn, BOOL bURN)
{
	static LPCTSTR pszBase64 = _T("ABCDEFGHIJKLMNOPQRSTUVWXYZ234567");

	CString strHash;
	LPTSTR pszHash = strHash.GetBuffer( bURN ? 9 + 32 : 32 );

	if ( bURN )
	{
		*pszHash++ = 'u'; *pszHash++ = 'r'; *pszHash++ = 'n'; *pszHash++ = ':';
		*pszHash++ = 's'; *pszHash++ = 'h'; *pszHash++ = 'a'; *pszHash++ = '1'; *pszHash++ = ':';
	}

	LPBYTE pHash = (LPBYTE)pHashIn;
	int nShift = 7;

	for ( int nChar = 32 ; nChar ; nChar-- )
	{
		BYTE nBits = 0;

		for ( int nBit = 0 ; nBit < 5 ; nBit++ )
		{
			if ( nBit ) nBits <<= 1;
			nBits |= ( *pHash >> nShift ) & 1;

			if ( ! nShift-- )
			{
				nShift = 7;
				pHash++;
			}
		}

		*pszHash++ = pszBase64[ nBits ];
	}

	strHash.ReleaseBuffer( bURN ? 9 + 32 : 32 );

	return strHash;
}

//////////////////////////////////////////////////////////////////////
// CSHA convert hash to string (hex)

CString CSHA::HashToHexString(const SHA1* pHashIn, BOOL bURN)
{
	static LPCTSTR pszHex = _T("0123456789ABCDEF");

	LPBYTE pHash = (LPBYTE)pHashIn;
	CString strHash;
	LPTSTR pszHash = strHash.GetBuffer(SHA1_DIGEST_SIZE*sizeof(TCHAR));

	for ( int nByte = 0 ; nByte < SHA1_DIGEST_SIZE ; ++nByte, ++pHash )
	{
		*pszHash++ = pszHex[ *pHash >> 4 ];
		*pszHash++ = pszHex[ *pHash & 15 ];
	}

	strHash.ReleaseBuffer(SHA1_DIGEST_SIZE*sizeof(TCHAR));

	if ( bURN )
		strHash = _T("urn:sha1:") + strHash;

	return strHash;
}

//////////////////////////////////////////////////////////////////////
// CSHA parse hash from string (Base64)

BOOL CSHA::HashFromString(LPCTSTR pszHash, SHA1* pHashIn)
{
	if ( ! pszHash || _tcslen( pszHash ) < 32 ) return FALSE;  //Invalid hash

	if ( _tcsnicmp(pszHash, _T("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"), 32 ) == 0 ) return FALSE; //Bad hash

	SHA1 Hash;
	LPBYTE pHash = (LPBYTE)&Hash;
	DWORD nBits	= 0;
	int nCount	= 0;

	for ( int nChars = 32 ; nChars-- ; pszHash++ )
	{
		if ( *pszHash >= 'A' && *pszHash <= 'Z' )
			nBits |= ( *pszHash - 'A' );
		else if ( *pszHash >= 'a' && *pszHash <= 'z' )
			nBits |= ( *pszHash - 'a' );
		else if ( *pszHash >= '2' && *pszHash <= '7' )
			nBits |= ( *pszHash - '2' + 26 );
		else
			return FALSE;

		nCount += 5;

		if ( nCount >= 8 )
		{
			*pHash++ = (BYTE)( nBits >> ( nCount - 8 ) );
			nCount -= 8;
		}

		nBits <<= 5;
	}

	*pHashIn = Hash;

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// CSHA parse hash from URN

BOOL CSHA::HashFromURN(LPCTSTR pszHash, SHA1* pHashIn)
{
	if (pszHash == NULL)
		return FALSE;
	size_t nLen = _tcslen( pszHash );

	if ( nLen >= 41 && _tcsnicmp( pszHash, _T("urn:sha1:"), 9 ) == 0 )
		return HashFromString( pszHash + 9, pHashIn );

	if ( nLen >= 37 && _tcsnicmp( pszHash, _T("sha1:"), 5 ) == 0 )
		return HashFromString( pszHash + 5, pHashIn );

	if ( nLen >= 85 && _tcsnicmp( pszHash, _T("urn:bitprint:"), 13 ) == 0 )
		// 13 + 32 + 1 + 39
		return HashFromString( pszHash + 13, pHashIn );

	if ( nLen >= 81 && _tcsnicmp( pszHash, _T("bitprint:"), 9 ) == 0 )
		return HashFromString( pszHash + 9, pHashIn );

	return FALSE;
}

BOOL CSHA::IsNull(SHA1* pHash)
{
	return (*pHash == SHA1()) ? TRUE : FALSE;
}