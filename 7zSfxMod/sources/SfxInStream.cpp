/*---------------------------------------------------------------------------*/
/* File:        SfxInStream.cpp                                              */
/* File:        Volumes.cpp                                                  */
/* Created:     Sat, 20 Nov 2010 11:58:35 GMT                                */
/*              by Oleg N. Scherbakov, mailto:oleg@7zsfx.info                */
/* Last update: Tue, 29 Dec 2015 05:31:38 GMT                                */
/*              by Oleg N. Scherbakov, mailto:oleg@7zsfx.info                */
/* Revision:    1865                                                         */
/*---------------------------------------------------------------------------*/
#include "stdafx.h"

#if defined(SFX_VOLUMES) || defined(SFX_PROTECT)
#include "7zSfxModInt.h"

#ifdef SFX_VOLUMES
	#include "sfx_multistream.cpp"

HRESULT CSfxInStream::OpenSubStream( LPCTSTR fileName, CSubStreamInfo * subStream )
{
	CInFileStream * streamSpec = new CInFileStream;
	CMyComPtr<IInStream> stream( streamSpec );
	if( !streamSpec->Open(fileName) )
		return S_FALSE;
	// size comes from the opened handle: no extra FindFirstFile and no
	// dangling pointer if the lookup fails (see audit: use-after-free)
	UInt64 size = 0;
	if( !streamSpec->GetLength( size ) )
		return S_FALSE;
	subStream->Size = size;
	subStream->Stream = stream;
	return S_OK;
}

bool CSfxInStream::Open(LPCTSTR fileName)
{
	Streams.Clear();

	CSubStreamInfo stream;
	if( OpenSubStream( fileName, &stream ) != S_OK )
		return false;
	Streams.Add( stream );
	InitVolumes( fileName );
	CMultiStream::Init();
	return true;
}

static int ReverseFindOneOf( CSfxStringU& src, LPCWSTR chars )
{
	while( *chars != L'\0' )
	{
		int nPos = src.ReverseFind( *chars );
		if( nPos >= 0 )
			return nPos;
		chars++;
	}
	return -1;
}

// true if str[nPos..] is a 3..4 digit volume index ("001", "010", "1000")
static bool IsVolumeIndex( const CSfxStringU& str, int nPos )
{
	int nLen = str.Len() - nPos;
	if( nLen != 3 && nLen != 4 )
		return false;
	for( int i = nPos; i < str.Len(); i++ )
		if( str[i] < L'0' || str[i] > L'9' )
			return false;
	return true;
}

// style 0: (name.7z.001.exe, name.7z.002, ...) || (name.001.exe, name.002, ...)
// style 1: name.exe, name.001, ...
// loader:  name.7z.001 / name.001 (the opened file is already the first volume)
static int GetVolumesName( CSfxStringU& baseName, LPCTSTR fileName )
{
	baseName = fileName;

	// External archive: the name itself ends with a volume index
	int nExtPos = ReverseFindOneOf( baseName, L".\\/" );
	if( nExtPos > 0 && baseName[nExtPos] == L'.' && IsVolumeIndex( baseName, nExtPos + 1 ) )
	{
		CSfxStringU strIndex = ((const wchar_t *)baseName) + nExtPos + 1;
		int nStart = (int)StringToLong( (const wchar_t *)strIndex ) + 1;
		baseName.ReleaseBuffer(nExtPos);
		return nStart;
	}

	if( nExtPos <= 0 || baseName[nExtPos] != L'.' )
		return -1;
	baseName.ReleaseBuffer(nExtPos);
#ifdef _SFX_USE_VOLUME_NAME_STYLE
	switch( nVolumeNameStyle )
	{
	case 1:
		{
			int nExtPos1 = ReverseFindOneOf( baseName, L".\\/" );
			if( nExtPos1 > 0 && baseName[nExtPos1] == L'.' && IsVolumeIndex( baseName, nExtPos1 + 1 ) )
			{
				CSfxStringU strIndex = ((const wchar_t *)baseName) + nExtPos1 + 1;
				int nStart = (int)StringToLong( (const wchar_t *)strIndex ) + 1;
				baseName.ReleaseBuffer(nExtPos1);
				return nStart;
			}
			return 1;
		}
	default:
#endif // _SFX_USE_VOLUME_NAME_STYLE
		int nExtPos = ReverseFindOneOf( baseName, L".\\/" );
		if( nExtPos <= 0 || baseName[nExtPos] != L'.' )
			return -1;
		if( IsVolumeIndex( baseName, nExtPos + 1 ) )
		{
			CSfxStringU strIndex = ((const wchar_t *)baseName) + nExtPos + 1;
			int nStart = (int)StringToLong( (const wchar_t *)strIndex ) + 1;
			baseName.ReleaseBuffer(nExtPos);
			return nStart;
		}
#ifdef _SFX_USE_VOLUME_NAME_STYLE
	}
#endif // _SFX_USE_VOLUME_NAME_STYLE
		return -1;
}

bool CSfxInStream::InitVolumes( LPCTSTR fileName )
{
	CSubStreamInfo	subStream;
	CSfxStringU baseName;
	m_strVolumeError.Empty();
	int nStartVolume = GetVolumesName( baseName, fileName );

	if( nStartVolume == -1 )
		return false;

	for( int nVolume=nStartVolume; ; nVolume++ )
	{
		CSfxStringU strVolume;
		WCHAR		ccIndex[32];
		wsprintf( ccIndex, L".%03u", nVolume );
		strVolume = baseName + ccIndex;
		if( OpenSubStream( strVolume, &subStream ) == S_OK )
			Streams.Add( subStream );
		else
		{
			// keep the first missing name: it is shown when the archive
			// still fails to open (incomplete volume set)
			m_strVolumeError = strVolume;
			break;
		}
	}
	return true;
}
#endif // SFX_VOLUMES
#endif // SFX_VOLUMES || SFX_PROTECT
