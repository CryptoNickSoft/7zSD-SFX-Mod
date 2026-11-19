#include "stdafx.h"
#include "7zSfxModInt.h"

/* Resolve a relative path against baseDir. Absolute paths are kept as is. */

void SfxResolveRelativePath( CSfxStringU &path, LPCWSTR baseDir )
{
	if( path.IsEmpty() || baseDir == NULL || *baseDir == 0 )
		return;

	bool fAbsolute = ( path.Len() >= 3 && path[1] == L':' && path[2] == L'\\' ) ||
		( path.Len() >= 2 && path[0] == L'\\' && path[1] == L'\\' );
	if( fAbsolute )
		return;

	unsigned lb = (unsigned)wcslen( baseDir );
	unsigned lp = path.Len();
	bool addSep = ( lb > 0 && baseDir[lb - 1] != L'\\' );

	CSfxStringU combined;
	wchar_t *buf = combined.GetBuffer( lb + (addSep ? 1 : 0) + lp + 1 );
	if( buf == NULL )
		return;
	wcscpy( buf, baseDir );
	if( addSep )
		wcscat( buf, L"\\" );
	wcscat( buf, (LPCWSTR)path );
	combined.ReleaseBuffer( (unsigned)wcslen( buf ) );
	path = (LPCWSTR)combined;
}

/* Convert an absolute path longer than MAX_PATH to the \\?\ form.
   Relative paths are resolved against baseDir first. */

void SfxMakeLongPath( CSfxStringU &path, LPCWSTR baseDir )
{
	if( path.IsEmpty() )
		return;

	SfxResolveRelativePath( path, baseDir );

	LPCWSTR src = path;
	if( wcsncmp( src, L"\\\\?\\", 4 ) == 0 )
		return;

	if( path.Len() < 248 )
		return;

	bool fAbsolute = ( path.Len() >= 3 && path[1] == L':' && path[2] == L'\\' ) ||
		( path.Len() >= 2 && path[0] == L'\\' && path[1] == L'\\' );
	if( fAbsolute == false )
		return;

	DWORD need = ::GetFullPathNameW( src, 0, NULL, NULL );
	if( need == 0 )
		return;

	CSfxStringU full;
	wchar_t *buf = full.GetBuffer( need + 8 );
	if( buf == NULL )
		return;
	DWORD n = ::GetFullPathNameW( src, need + 8, buf, NULL );
	if( n == 0 || n >= need + 8 )
		return;
	full.ReleaseBuffer( n );

	bool fUnc = ( n >= 2 && full[0] == L'\\' && full[1] == L'\\' );
	LPCWSTR prefix = fUnc ? L"\\\\?\\UNC\\" : L"\\\\?\\";
	LPCWSTR tail = fUnc ? full.Ptr() + 2 : full.Ptr();
	unsigned tailLen = fUnc ? n - 2 : n;
	unsigned prefixLen = fUnc ? 8 : 4;

	CSfxStringU res;
	wchar_t *dst = res.GetBuffer( prefixLen + tailLen + 1 );
	if( dst == NULL )
		return;
	wcscpy( dst, prefix );
	wcsncat( dst, tail, tailLen );
	res.ReleaseBuffer( prefixLen + tailLen );

	path = (LPCWSTR)res;
}

void SfxMakeLongPath( CSfxStringU &path )
{
	SfxMakeLongPath( path, NULL );
}
