/*---------------------------------------------------------------------------*/
/* File:        main.cpp                                                     */
/* Created:     Fri, 29 Jul 2005 03:23:00 GMT                                */
/*              by Oleg N. Scherbakov, mailto:oleg@7zsfx.info                */
/* Last update: Sat, 02 Apr 2016 06:31:33 GMT                                */
/*              by Oleg N. Scherbakov, mailto:oleg@7zsfx.info                */
/* Revision:    3901                                                         */
/*---------------------------------------------------------------------------*/
/* Revision:    1798                                                         */
/* Updated:		Wed, 30 Jun 2010 09:24:36 GMT                                */
/*              by Oleg N. Scherbakov, mailto:oleg@7zsfx.info                */
/* Description: Fix: extraction folder was not set as default				 */
/*				for external programs										 */
/*---------------------------------------------------------------------------*/
/* Revision:    1794                                                         */
/* Updated:     Sat, 26 Jun 2010 04:23:10 GMT                                */
/*              by Oleg N. Scherbakov, mailto:oleg@7zsfx.info                */
/* Description: Execution of external programs was rewritten to support		 */
/*				'wait all childs' of the executed process					 */
/*				new execution prefix 'waitall'								 */
/*---------------------------------------------------------------------------*/
/* Revision:    1774                                                         */
/* Updated:     Sun, 06 Jun 2010 08:51:48 GMT                                */
/*              by Oleg N. Scherbakov, mailto:oleg@7zsfx.info                */
/* Description: Add warning stuff											 */
/*				(WarningTitle, MiscFlags, -mf command line switch)			 */
/*---------------------------------------------------------------------------*/
/* Revision:    1698                                                         */
/* Updated:     Mon, 22 Mar 2010 11:26:58 GMT                                */
/*              by Oleg N. Scherbakov, mailto:oleg@7zsfx.info                */
/* Description: New file stamp.                                              */
/*---------------------------------------------------------------------------*/
#include "stdafx.h"
#include "7zSfxModInt.h"
#include "ExtractEngine.h"
#include "archive.h"

void	DeleteSFX( CSfxStringU moduleName );

CSfxStringU SfxMultiByteToUnicodeString( const CSfxStringA &srcString, UINT codePage );
CSfxStringA SfxUnicodeStringToMultiByte( const CSfxStringU &srcString, UINT codePage );

const UInt64 kMaxCheckStartPosition = 1 << 21;  /* 2 MiB: larger icons/config shift the 7z signature deeper */

char kSignatureConfigStart[] = ",!@Install@!UTF-8!";
char kSignatureConfigEnd[] = ",!@InstallEnd@!";


LPCWSTR	lpwszErrorTitle;
#ifdef _SFX_USE_WARNINGS
	LPCWSTR		lpwszWarningTitle;
	CSfxStringU	strWarningTitle;
#endif // _SFX_USE_WARNINGS
LPCWSTR	lpwszTitle;
LPCWSTR lpwszExtractTitle;
LPCWSTR lpwszExtractPathTitle;
LPCWSTR lpwszExtractPathText;
LPCWSTR lpwszCancelPrompt;
LPCWSTR	lpwszCancelText;
LPCWSTR	lpwszExtractDialogText;
#ifdef SFX_CRYPTO
	LPCWSTR	lpwszPasswordTitle = NULL;
	LPCWSTR	lpwszPasswordText = NULL;
#endif // SFX_CRYPTO
#ifdef _SFX_USE_VOLUME_NAME_STYLE
	int nVolumeNameStyle=0;
#endif // _SFX_USE_VOLUME_NAME_STYLE

int		GUIMode = 0;
int		GUIFlags = -1;
int		MiscFlags = 0;
int		ExtractDialogWidth = 300;
int		ExtractPathWidth = 300;
int		OverwriteMode = OVERWRITE_MODE_ALL;
int		OverwriteFlags = 0;
int		FinishMessage = -1;
#ifdef _SFX_USE_BEGINPROMPTTIMEOUT
	int		BeginPromptTimeout = 0;
#endif // _SFX_USE_BEGINPROMPTTIMEOUT
CSfxStringU	extractPath;
CSfxStringU strSfxFolder;
CSfxStringU strSfxName;
CSfxStringU	strErrorTitle;
CSfxStringU	strTitle;
#ifdef _SFX_USE_PREFIX_PLATFORM
	CSfxStringU	strOSPlatform;
#endif // _SFX_USE_PREFIX_PLATFORM
#ifdef _SFX_USE_SFXAPI
	CSfxAPI	gSfxApi;
#endif // _SFX_USE_SFXAPI
CSfxArchive	gSfxArchive;

bool	fUseInstallPath;
int		nSingleInstanceMode;

CSfxStringU	strModulePathName;
CSfxStringU g_strCmdLineArchive;
CSfxStringU strLogFile;
int nRetryAttempts = 0;
unsigned long ulPayloadCRC = 0;
int nCleanMode = 0;
CObjectVector<CTextConfigPair> gConfig;

HMODULE	hKernel32;

CObjectVector<CTextConfigPair> Variables;
bool fUseBackward = false;

typedef struct tagENVALIAS {
	int		nFolder;
	LPCWSTR	lpwszName;
} ENVALIAS, * PENVALIAS;

#ifndef CSIDL_COMMON_DOCUMENTS
	#define CSIDL_COMMON_DOCUMENTS          0x002e        // All Users\Documents
#endif

ENVALIAS EnvAliases [] = {
	{ CSIDL_COMMON_DESKTOPDIRECTORY,	L"CommonDesktop" },
	{ CSIDL_COMMON_DOCUMENTS,			L"CommonDocuments" },
	{ CSIDL_DESKTOPDIRECTORY,			L"UserDesktop" },
	{ CSIDL_PERSONAL,					L"MyDocuments" },
	{ CSIDL_PERSONAL,					L"MyDocs" }
};

void SfxInit()
{
	hKernel32 = ::LoadLibraryA("kernel32");
	InitCommonControls();
	CrcGenerateTable();

#ifdef _SFX_USE_LANG
	GetUILanguage();
#endif // _SFX_USE_LANG

	lpwszErrorTitle = GetLanguageString( STR_ERROR_TITLE );
#ifdef _SFX_USE_WARNINGS
	lpwszWarningTitle = GetLanguageString( STR_WARNING_TITLE );
#endif // _SFX_USE_WARNINGS
	lpwszTitle = GetLanguageString( STR_TITLE );
	lpwszExtractTitle = GetLanguageString( STR_EXTRACT_TITLE );
	lpwszExtractPathTitle = GetLanguageString( STR_EXTRACT_PATH_TITLE );
	lpwszExtractPathText = GetLanguageString( STR_EXTRACT_PATH_TEXT );
	lpwszCancelPrompt = GetLanguageString( STR_CANCEL_PROMPT );
	lpwszCancelText = lpwszExtractDialogText = NULL;

	WCHAR	wszPath[MAX_PATH+1];
	WCHAR	wszName[sizeof(SPECIAL_FOLDER_FORMAT)/sizeof(WCHAR)+32];
	for( int i = 0; i < 0x40; i++ )
	{
		if( SHGetSpecialFolderPath(NULL,wszPath,i,FALSE) != FALSE )
		{
			wsprintf( wszName, SPECIAL_FOLDER_FORMAT, i );
			SfxAddEnvironmentVarWithAlias( wszName, wszPath );
			for( int j = 0; j < (sizeof(EnvAliases)/sizeof(EnvAliases[0])); j++ )
			{
				if( EnvAliases[j].nFolder == i )
					SfxAddEnvironmentVar( EnvAliases[j].lpwszName, wszPath );
			}
		}
	}
}

void ReplaceWithProcessId( CSfxStringU& str )
{
	if( str.Find( L"SfxVarPID" ) < 0 && str.Find( L"%%PID%%" ) < 0 )
		return;
	WCHAR wszPid[16];
	wsprintf( wszPid, L"%u", (unsigned)::GetCurrentProcessId() );
	str.Replace( L"%SfxVarPID%", wszPid );
	str.Replace( L"%%PID%%", wszPid );
}

void ReplaceVariables( CSfxStringU& str )
{
	ReplaceWithProcessId( str );
	ExpandEnvironmentStrings( str );
	ReplaceWithArchivePath( str, strSfxFolder );
	ReplaceWithArchiveName( str, strSfxName );
#ifdef _SFX_USE_PREFIX_PLATFORM
	ReplaceWithPlatform( str );
#endif // _SFX_USE_PREFIX_PLATFORM
	ExpandEnvironmentStrings( str );
}

void ReplaceVariablesEx( CSfxStringU& str )
{
	ExpandEnvironmentStrings( str );
	ReplaceWithExtractPath( str, extractPath );
	ReplaceVariables( str );
}

void SetEnvironment()
{
	for( int i = 0; i < (int)Variables.Size(); i++ )
	{
		CSfxStringU val = Variables[i].String;
		ReplaceVariablesEx( val );
		::SetEnvironmentVariable( Variables[i].ID, val );
	}
}

LPCWSTR UpdateFlagsCommon( LPCWSTR lpwszText, int * pnValue )
{
	while( (*lpwszText >= L'0' && *lpwszText <= L'9') || *lpwszText == L'+' || *lpwszText == L'-' )
	{
		int nValue = StringToLong( lpwszText );
		// skip to next
		while( *lpwszText == L'-' || *lpwszText == L'+' ) lpwszText++;
		while( *lpwszText >= L'0' && *lpwszText <= L'9' ) lpwszText++;
		if( nValue > 0 )
			pnValue[0] |= nValue;
		else
			pnValue[0] &= (~(0-nValue));
	}

	return lpwszText;
}

LPCWSTR UpdateGUIFlags( LPCWSTR lpwszText )
{
	if( GUIFlags == -1 || (*lpwszText != L'+' && *lpwszText != L'-') )
		GUIFlags = 0;

	return UpdateFlagsCommon( lpwszText, &GUIFlags );
}

LPCWSTR UpdateMiscFlags( LPCWSTR lpwszText )
{
	if( *lpwszText != L'+' && *lpwszText != L'-' )
		MiscFlags = 0;

	return UpdateFlagsCommon( lpwszText, &MiscFlags );
}

LPCWSTR UpdateOverwriteMode( LPCWSTR lpwszText )
{
	int nValue = OverwriteMode | OverwriteFlags;
	if( *lpwszText != L'+' && *lpwszText != L'-' )
		nValue = OVERWRITE_MODE_ALL;
	LPCWSTR lpwszRet = UpdateFlagsCommon( lpwszText, &nValue );
	if( (nValue&OVERWRITE_MODE_MASK) <= OVERWRITE_MODE_MAX )
		OverwriteMode = nValue & OVERWRITE_MODE_MASK;
	OverwriteFlags = nValue & (~OVERWRITE_MODE_MASK);
	return lpwszRet;
}

bool SaveConfiguration( LPCWSTR lpwszFileName, CSfxStringA & CfgData )
{
	COutFileStream	cfg;
	UInt32 writed;
	static BYTE utf8prefix[4] = { 0xEF, 0xBB, 0xBF, 0 };
	if( cfg.Create_ALWAYS( lpwszFileName ) == false )
	{
		CSfxStringU filePath = lpwszFileName;
		int nPos  = GetDirectorySeparatorPos( filePath );
		if( nPos < 0 )
			return false;
		filePath.ReleaseBuffer( nPos );
		if( CreateFolderTree( (LPCWSTR)filePath ) == FALSE || cfg.Create_ALWAYS( lpwszFileName ) == false )
			return false;
	}
	CSfxStringA fullCfg;
	fullCfg = (char *)utf8prefix;
	fullCfg += kSignatureConfigStart;
	fullCfg += (LPCSTR)CfgData;
	fullCfg += kSignatureConfigEnd;
	if( ((ISequentialOutStream *)&cfg)->Write(fullCfg, fullCfg.Len(), &writed) != S_OK || writed != fullCfg.Len() )
		return false;
	return true;
}

void ReplaceVariableInShortcut( CSfxStringU& strShortcut, CSfxStringU& strVarName, CSfxStringU& strVarValue )
{
	int nVarNameLength = strVarName.Len();
	for( int i = 0; i < (int)strShortcut.Len(); i++ )
	{
		if( strShortcut[i] == L'%' &&
				MyStrincmp( ((const wchar_t *)strShortcut)+i+1, strVarName, nVarNameLength ) == 0 &&
					strShortcut[i+nVarNameLength+1] == L'%' )
		{
			// var found
			strShortcut.Delete( i, nVarNameLength+2 );
			strShortcut.Insert( i, strVarValue );
		}
	}
}

LPCWSTR IsSfxSwitch( LPCWSTR lpwszCommandLine, LPCWSTR lpwszSwitch )
{
	SKIP_WHITESPACES_W(lpwszCommandLine);
	// 1. check old notation
	if( lpwszCommandLine[0] == L'-' && lpwszCommandLine[1] == '-' )
	{
		lpwszCommandLine++;
	}
	// 2. New (common) notation
	if( lpwszCommandLine[0] == L'-' || lpwszCommandLine[0] == '/')
	{
		int nSwitchLength = lstrlen(lpwszSwitch);
		if( MyStrincmp( lpwszCommandLine+1, lpwszSwitch, lstrlen(lpwszSwitch) ) == 0 &&
				(((unsigned)lpwszCommandLine[nSwitchLength+1]) <= L' ' || lpwszCommandLine[nSwitchLength+1] == L':') )
		{
			return lpwszCommandLine+nSwitchLength+1;
		}
	}
	
	return NULL;
}

void PostExecute_Shortcut( LPCWSTR lpwszValue )
{
	CSfxStringU strShortcut = lpwszValue;
	ReplaceWithExtractPath( strShortcut, extractPath );
	ReplaceWithArchivePath( strShortcut, strSfxFolder );
	ReplaceWithArchiveName( strShortcut, strSfxName );
#ifdef _SFX_USE_PREFIX_PLATFORM
	ReplaceWithPlatform( strShortcut );
#endif // _SFX_USE_PREFIX_PLATFORM
	for( int i = 0; i < (int)Variables.Size(); i++ )
	{
		CSfxStringU varValue = MyGetEnvironmentVariable( Variables[i].ID );
		ExpandEnvironmentStrings( varValue );
		ReplaceWithExtractPath( varValue, extractPath );
		ReplaceWithArchivePath( varValue, strSfxFolder );
		ReplaceWithArchiveName( varValue, strSfxName );
#ifdef _SFX_USE_PREFIX_PLATFORM
		ReplaceWithPlatform( varValue );
#endif // _SFX_USE_PREFIX_PLATFORM

		ReplaceVariableInShortcut( strShortcut, Variables[i].ID, varValue );
	}
	CreateShortcut( strShortcut );
}

void PostExecute_Delete( LPCWSTR lpwszValue )
{
	CSfxStringU tmp = lpwszValue;
	ReplaceVariablesEx( tmp );
	DeleteFileOrDirectoryAlways( tmp );
}

typedef void (* POST_EXECUTE_PROC)( LPCWSTR lpwszParamName );

void ProcessPostExecuteSub( POST_EXECUTE_PROC pfnPostExecute,
						    LPCWSTR lpwszParamPrefix,
							LPCWSTR lpwszBatchIndexes,
							int nUseDefault )
{
		CSfxStringU	ustrParamName;
		LPCWSTR	lpwszValue;
		LPCWSTR p1 = lpwszBatchIndexes;
		bool	fBatchExecuted = false;
		int nIndex;
		while( (*p1 >= L'0' && *p1 <= L'9') ||
			   (*p1 >= L'a' && *p1 <= L'z') ||
			   (*p1 >= L'A' && *p1 <= L'Z') ||
			   (lpwszBatchIndexes == p1 && nUseDefault == -1) )
		{
			ustrParamName = lpwszParamPrefix;
			ustrParamName += *p1;
			nIndex = 0;
			p1++;

			while( (lpwszValue = GetTextConfigValue( ustrParamName, &nIndex)) != NULL )
			{
				pfnPostExecute( lpwszValue );
				nIndex++;
				fBatchExecuted = true;
			}
		}
		switch( nUseDefault )
		{
		case 0:
			break;
		case 1:
			ProcessPostExecuteSub( pfnPostExecute, lpwszParamPrefix, L"\0", -1 );
			break;
		default:
			if( *lpwszBatchIndexes == L'\0' && nUseDefault == -1 )
				break;
			if( fBatchExecuted == false && *lpwszBatchIndexes != L'\0' )
				ProcessPostExecuteSub( pfnPostExecute, lpwszParamPrefix, L"\0",  -1 );
		}
}

void SetConfigVariables()
{
	LPCWSTR	lpwszValue;
	int from;

	// Language= : force UI language (decimal or hex LangID, e.g. 1049 or 0x419)
	// Must be applied before any GetLanguageString() caches idSfxLang.
#ifdef _SFX_USE_LANG
	if( (lpwszValue = GetTextConfigValue( CFG_LANGUAGE )) != NULL && lpwszValue[0] != L'\0' )
	{
		long idLang = (lpwszValue[0] == L'0' && (lpwszValue[1] == L'x' || lpwszValue[1] == L'X'))
			? StringToLong( lpwszValue + 2 ) /* crude hex fallback */
			: StringToLong( lpwszValue );
		if( idLang > 0 && idLang <= 0xffff )
			idSfxLang = (LANGID)idLang;
	}
#endif // _SFX_USE_LANG

	// PayloadCRC= : expected CRC32 of the 7z data block (hex, e.g. 0x12345678).
	// 0 / absent disables the self-check. Reset so repeated SetConfigVariables()
	// passes (config reload + command-line reparse) never keep a stale value.
	ulPayloadCRC = 0;
	if( (lpwszValue = GetTextConfigValue( CFG_PAYLOADCRC )) != NULL && lpwszValue[0] != L'\0' )
	{
		if( lpwszValue[0] == L'0' && (lpwszValue[1] == L'x' || lpwszValue[1] == L'X') )
		{
			LPCWSTR p = lpwszValue + 2;
			unsigned long v = 0;
			while( *p != L'\0' )
			{
				int d = (*p >= L'0' && *p <= L'9') ? (*p - L'0')
					: (*p >= L'a' && *p <= L'f') ? (*p - L'a' + 10)
					: (*p >= L'A' && *p <= L'F') ? (*p - L'A' + 10) : -1;
				if( d < 0 ) break;
				v = (v << 4) | (unsigned long)d;
				p++;
			}
			ulPayloadCRC = v;
		}
		else
		{
			ulPayloadCRC = (unsigned long)StringToLong( lpwszValue );
		}
		WCHAR wszDbg[128];
		wsprintfW( wszDbg, L"[crc] cfg PayloadCRC=%s -> 0x%08X", lpwszValue, (unsigned)ulPayloadCRC );
		::SfxLogAppend( wszDbg );
	}
	else
	{
		::SfxLogAppend( L"[crc] cfg PayloadCRC absent -> disabled" );
	}

	// CleanMode= : low-footprint cleanup bitmask
	//   1 = silent self-delete of the exe (no visible cmd window)
	//   2 = deep temp-folder cleanup (fallback + empty wrapper dirs)
	//   4 = overwrite file contents before deleting (anti-recovery)
	//   8 = leave no auxiliary artifacts (suppress install log)
	nCleanMode = 0;
	if( (lpwszValue = GetTextConfigValue( CFG_CLEANMODE )) != NULL && lpwszValue[0] != L'\0' )
		nCleanMode = StringToLong( lpwszValue );
	if( nCleanMode < 0 ) nCleanMode = 0;

	// Main title
	if( (lpwszValue = GetTextConfigValue( CFG_TITLE )) != NULL )
	{
		strErrorTitle = lpwszValue; strErrorTitle += GetLanguageString( STR_ERROR_SUFFIX );
		lpwszErrorTitle = strErrorTitle;
#ifdef _SFX_USE_WARNINGS
		strWarningTitle = lpwszValue; strWarningTitle += GetLanguageString( STR_WARNING_SUFFIX );
		lpwszWarningTitle = strWarningTitle;
#endif // _SFX_USE_WARNINGS
		lpwszTitle = lpwszValue;
	}
	// Update error title
	if( (lpwszValue = GetTextConfigValue( CFG_ERRORTITLE )) != NULL )
		lpwszErrorTitle = lpwszValue;
#ifdef _SFX_USE_WARNINGS
	// Update warnings title
	if( (lpwszValue = GetTextConfigValue( CFG_WARNINGTITLE )) != NULL )
		lpwszWarningTitle = lpwszValue;
#endif // _SFX_USE_WARNINGS
	// Extract title
	if( (lpwszValue = GetTextConfigValue( CFG_EXTRACT_TITLE )) != NULL )
		lpwszExtractTitle = lpwszValue;
	// Load GUIMode & 7-Zip > 4.42 'Progress'
	if( (lpwszValue = GetTextConfigValue( CFG_PROGRESS)) != NULL && lstrcmpi( lpwszValue, L"no" ) == 0 )
		GUIMode = GUIMODE_HIDDEN;
	if( (lpwszValue = GetTextConfigValue( CFG_GUIMODE )) != NULL )
	{
		if( lpwszValue[0] >= L'0' && lpwszValue[0] <= L'0'+GUIMODE_MAX )
			GUIMode = lpwszValue[0] - L'0';
	}
	// Load OverwriteMode
	if( (lpwszValue = GetTextConfigValue( CFG_OVERWRITE_MODE)) != NULL )
	{
		UpdateOverwriteMode( lpwszValue );
	}
	// Load GUIFlags
	from = 0;
	while( (lpwszValue = GetTextConfigValue( CFG_GUIFLAGS, &from )) != NULL )
	{
		from++;
		UpdateGUIFlags( lpwszValue );
	}
	// Load MiscFlags
	from = 0;
	while( (lpwszValue = GetTextConfigValue( CFG_MISCFLAGS, &from )) != NULL )
	{
		from++;
		UpdateMiscFlags( lpwszValue );
	}
	
	lpwszCancelText = GetTextConfigValue( CFG_EXTRACT_CANCELTEXT );
	lpwszExtractDialogText = GetTextConfigValue( CFG_EXTRACT_DIALOGTEXT );
	if( (lpwszValue = GetTextConfigValue( CFG_EXTRACT_DIALOGWIDTH)) != NULL )
		ExtractDialogWidth = StringToLong( lpwszValue );
	if( (lpwszValue = GetTextConfigValue( CFG_EXTRACTPATH_WIDTH)) != NULL )
		ExtractPathWidth = StringToLong( lpwszValue );
	
	// Extract path strings
	if( (lpwszValue = GetTextConfigValue(CFG_EXTRACT_PATH_TITLE)) != NULL )
		lpwszExtractPathTitle = lpwszValue;
	if( (lpwszValue = GetTextConfigValue(CFG_EXTRACT_PATH_TEXT)) != NULL )
		lpwszExtractPathText = lpwszValue;
	
	// Cancel prompt text
	if( (lpwszValue = GetTextConfigValue(CFG_CANCEL_PROMPT)) != NULL )
		lpwszCancelPrompt = lpwszValue;

#ifdef SFX_CRYPTO
	// password from config (command line '-p' has priority)
	if( CSfxArchive::CPassword::IsDefined() == false )
	{
		if( (lpwszValue = GetTextConfigValue(CFG_PASSWORD)) != NULL && lpwszValue[0] != L'\0' )
			CSfxArchive::CPassword::Set( lpwszValue );
	}
	// author code as archive password: SfxAuthor="..."
	if( (lpwszValue = GetTextConfigValue(CFG_SFXAUTHOR)) != NULL && lpwszValue[0] != L'\0' )
	{
		WCHAR wszAuthorCode[64];
		GetSfxAuthorCode( lpwszValue, wszAuthorCode, 64 );
		CSfxArchive::CPassword::SetAuthorCode( wszAuthorCode );
	}
	else
	{
		CSfxArchive::CPassword::SetAuthorCode( L"" );
	}
	if( (lpwszValue = GetTextConfigValue(CFG_PASSWORD_TITLE)) != NULL )
		lpwszPasswordTitle = lpwszValue;
	else
		lpwszPasswordTitle = lpwszTitle;
	if( (lpwszValue = GetTextConfigValue(CFG_PASSWORD_TEXT)) != NULL )
		lpwszPasswordText = lpwszValue;
	else
		lpwszPasswordText = GetLanguageString( STR_PASSWORD_TEXT );
#endif // SFX_CRYPTO
#ifdef _SFX_USE_VOLUME_NAME_STYLE
	if( (lpwszValue = GetTextConfigValue( CFG_VOLUME_NAME_STYLE )) != NULL )
	{
		if( lpwszValue[0] >= L'0' && lpwszValue[0] <= L'9' )
			nVolumeNameStyle = lpwszValue[0] - L'0';
	}
#endif // _SFX_USE_VOLUME_NAME_STYLE
}

// PayloadCRC= self-check: compute CRC32 of the 7z data block (from the '7z'
// signature to the end of file) and compare with the expected value.
static UInt32 ReadFullStream( IInStream * stream, Byte * buf, UInt32 size )
{
	UInt32 done = 0;
	while( done < size )
	{
		UInt32 read = 0;
		if( stream->Read( buf + done, size - done, &read ) != S_OK )
			break;
		if( read == 0 )
			break;
		done += read;
	}
	return done;
}

// PayloadCRC= self-check: CRC32 of the 7z data block (from the LAST '7z'
// signature to the end of the stream) must match ulPayloadCRC. The stream is
// the whole multivolume set, so all volumes are covered.
static bool CheckPayloadCRC()
{
	if( ulPayloadCRC == 0 )
		return true; // disabled

	IInStream * stream = gSfxArchive.GetStream();
	if( stream == NULL )
		return false;

	static const Byte kSig[6] = { '7', 'z', 0xBC, 0xAF, 0x27, 0x1C };
	const UInt32 kBufSize = (1 << 16);
	Byte *buf = new Byte[kBufSize];
	if( buf == NULL )
		return false;

	if( stream->Seek( 0, STREAM_SEEK_SET, NULL ) != S_OK )
	{
		delete[] buf;
		return false;
	}

	// The stub may embed the same signature, so keep the LAST occurrence.
	UInt64 basePos = 0;
	UInt64 lastSigPos = (UInt64)(Int64)-1;
	UInt32 tailLen = 0;
	for( ;; )
	{
		UInt32 read = ReadFullStream( stream, buf + tailLen, kBufSize - tailLen );
		UInt32 dataLen = tailLen + read;
		for( UInt32 i = 0; i + 6 <= dataLen; i++ )
		{
			if( memcmp( buf + i, kSig, 6 ) == 0 )
				lastSigPos = basePos + i;
		}
		if( read == 0 )
			break;
		tailLen = (dataLen >= 5) ? 5 : dataLen;
		memcpy( buf, buf + dataLen - tailLen, tailLen );
		basePos += dataLen - tailLen;
	}

	if( lastSigPos == (UInt64)(Int64)-1 )
	{
		delete[] buf;
		::SfxLogAppend( L"[crc] signature not found" );
		return false;
	}


	if( stream->Seek( (Int64)lastSigPos, STREAM_SEEK_SET, NULL ) != S_OK )
	{
		delete[] buf;
		return false;
	}

	UInt32 crc = CRC_INIT_VAL;
	for( ;; )
	{
		UInt32 read = ReadFullStream( stream, buf, kBufSize );
		if( read == 0 )
			break;
		crc = CrcUpdate( crc, buf, read );
	}
	crc = CRC_GET_DIGEST( crc );
	WCHAR wszCrc[48];
	wsprintfW( wszCrc, L"[crc] ok computed=0x%08X", (unsigned)crc );
	::SfxLogAppend( wszCrc );
	delete[] buf;
	return ( crc == ulPayloadCRC );
}
void ShowSfxVersion()
{
	GUIFlags = GUIFLAGS_XPSTYLE;
	CSfxDialog_Version dlg;
	extern unsigned int g_NumCodecs;
	extern const CCodecInfo *g_Codecs[]; 
	CSfxStringU ustrVersion = GetLanguageString( STR_SFXVERSION );
	unsigned int ki;
	for( ki = 0; ki < g_NumCodecs; ki++ )
	{
		if( ki != 0 )
			ustrVersion += L", ";
		ustrVersion.AddAscii( g_Codecs[ki]->Name );
	}
#ifdef SFX_VOLUMES
	if( ki != 0 )
		ustrVersion += L", ";
	ustrVersion += L"Volumes";
#endif // SFX_VOLUMES
	WCHAR tmp[128];
	wsprintf( tmp, L" \n\t%X - %03X - %03X - %03X - %03X",
				SFXBUILD_OPTIONS1, SFXBUILD_OPTIONS2, SFXBUILD_OPTIONS3, SFXBUILD_OPTIONS4, SFXBUILD_OPTIONS5 );
	ustrVersion += tmp;
	ustrVersion += L"\n";
	dlg.Show( SD_OK|SD_ICONINFORMATION, lpwszTitle, ustrVersion );
}

void SfxCleanup()
{
	if( fUseInstallPath == false )
	{
		DeleteFileOrDirectoryAlways( extractPath );
		SfxCleanupFallbackTemp();
	}
	if( nCleanMode & 2 )
		SfxCleanupFallbackTemp(); // deep temp cleanup (fallback base) even if install-path used
}

#include <new.h>
int __cdecl sfx_new_handler( size_t size )
{
	MessageBoxA( NULL, "Could not allocate memory", "7-Zip SFX", MB_OK|MB_ICONSTOP|MB_SYSTEMMODAL );
	return 0;
}

static bool IsVolumeNameSuffix( LPCWSTR lpwszToken )
{
    int nLen = (int)wcslen( lpwszToken );
    int nPos = nLen - 1;
    int nDigits = 0;
    while( nPos >= 0 && lpwszToken[nPos] >= L'0' && lpwszToken[nPos] <= L'9' )
    {
        nDigits++;
        nPos--;
    }
    return ((nDigits == 3 || nDigits == 4) && nPos > 0 && lpwszToken[nPos] == L'.');
}

static bool IsCmdLineArchiveName( LPCWSTR lpwszToken )
{
    if( lpwszToken == NULL || lpwszToken[0] == L'\0' )
        return false;
    if( lpwszToken[0] == L'-' || lpwszToken[0] == L'/' )
        return false;
    for( LPCWSTR p = lpwszToken; *p != L'\0'; p++ )
    {
        if( *p == L'.' && (p[1]|0x20) == L'7' && (p[2]|0x20) == L'z' )
            return true;
    }
    // multivolume sets: "payload.001", "setup.001" (no ".7z" in the name)
    return IsVolumeNameSuffix( lpwszToken );
}
#ifdef _SFX_USE_CUSTOM_EXCEPTIONS
int APIENTRY WinMain2( HINSTANCE hInstance,
					   HINSTANCE hPrevInstance,
					   LPSTR lpCmdLine, int nCmdShow );

int APIENTRY WinMain( HINSTANCE hInstance,
					  HINSTANCE hPrevInstance,
					  LPSTR lpCmdLine, int nCmdShow )
{
	try {
		return WinMain2( hInstance, hPrevInstance, lpCmdLine, nCmdShow );
	}
	catch( ... ) {
		return ERRC_EXCEPTION;
	}
}
int APIENTRY WinMain2( HINSTANCE hInstance,
#else
int APIENTRY WinMain( HINSTANCE hInstance,
#endif // _SFX_USE_CUSTOM_EXCEPTIONS
					 HINSTANCE hPrevInstance,
					 LPSTR lpCmdLine, int nCmdShow )
{
#ifdef _DEBUG
	DWORD dwStartTime = ::GetTickCount();
#endif // -DEBUG
	_set_new_handler( sfx_new_handler );
	CreateDummyWindow();

	OSVERSIONINFO versionInfo;
	versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
	if( ::GetVersionEx(&versionInfo) == FALSE ||
			versionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT ||
				versionInfo.dwMajorVersion < 5 )
	{
		::MessageBoxA( NULL, "Sorry, this program requires Microsoft Windows 2000 or later.", "7-Zip SFX", MB_OK|MB_ICONSTOP|MB_SYSTEMMODAL );
		return ERRC_PLATFORM;
	}

	kSignatureConfigStart[0] = kSignatureConfigEnd[0] = ';';

	int		ShortcutDefault = -1;
	int		DeleteDefault = -1;
	int		SelfDelete = -1;
	LPCWSTR	lpwszValue;
	// Command line
	CSfxStringU tmpstr;
	CSfxStringU	SfxConfigFilename;
	CSfxStringU strCmdLine;

	SfxInit();

	strCmdLine = LoadQuotedString( ::GetCommandLine(), tmpstr );
	    ReplaceHexChars( strCmdLine );

    // External archive: path from the command line (argument or drag & drop)
    {
        LPWSTR pszCmd = strCmdLine.GetBuffer(0);
        LPWSTR p = pszCmd;
        while( *p != L'\0' )
        {
            while( *p != L'\0' && ((unsigned)*p) <= L' ' )
                p++;
            if( *p == L'\0' )
                break;
            LPWSTR pTokenStart = p;
            LPWSTR pTokenValue = p;
            bool fQuoted = (*p == L'"');
            if( fQuoted != false )
            {
                pTokenValue = ++p;
                while( *p != L'\0' && *p != L'"' )
                    p++;
            }
            else
            {
                while( *p != L'\0' && ((unsigned)*p) > L' ' )
                    p++;
            }
            WCHAR cSave = *p;
            *p = L'\0';
            if( IsCmdLineArchiveName( pTokenValue ) != false )
            {
                g_strCmdLineArchive = pTokenValue;
                LPWSTR pBlankEnd = p;
                *p = cSave;
                if( fQuoted != false && cSave == L'"' )
                    pBlankEnd = p + 1;
                for( LPWSTR q = pTokenStart; q < pBlankEnd; q++ )
                    *q = L' ';
            }
            else
            {
                *p = cSave;
            }
            if( fQuoted != false && cSave == L'"' )
                p++;
        }
        strCmdLine.ReleaseBuffer();
    }
	LPCWSTR str = strCmdLine;
#ifdef _SFX_USE_ENVIRONMENT_VARS
	SfxAddEnvironmentVarWithAlias( SFX_ENV_VAR_MODULE_PLATFORM, PLATFORM_NAME_W );
	SfxAddEnvironmentVarWithAlias( SFX_ENV_VAR_SYSTEM_PLATFORM, GetPlatformName() );
	SfxAddEnvironmentVarWithAlias( SFX_ENV_VAR_CMDLINE_0, ::GetCommandLine() );
#endif // _SFX_USE_ENVIRONMENT_VARS

#ifdef _SFX_USE_LANG
	#ifdef _SFX_USE_ENVIRONMENT_VARS
		wsprintf( tmpstr.GetBuffer(32), L"%d", idSfxLang );
		tmpstr.ReleaseBuffer();
		SfxAddEnvironmentVarWithAlias( SFX_ENV_VAR_SYSTEM_LANGUAGE, (LPCWSTR)tmpstr );
	#endif // _SFX_USE_ENVIRONMENT_VARS

	if( (lpwszValue = IsSfxSwitch( str, CMDLINE_SFXLANG )) != NULL && *lpwszValue == L':' )
	{
		long idLang=StringToLong( lpwszValue+1 );
		if( idLang > 0 && idLang <= 0xffff )
			idSfxLang = (LANGID)idLang;
		str++;
		while( ((unsigned)*str) > L' ' ) str++;
	}
#endif // _SFX_USE_LANG

	if( IsSfxSwitch( str, CMDLINE_SFXVERSION ) != NULL )
	{
		ShowSfxVersion();
		return ERRC_NONE;
	}

#ifdef _SFX_USE_PREFIX_WAITALL
	if( (lpwszValue = IsSfxSwitch(str,CMDLINE_SFXWAITALL )) != NULL )
	{
		return Child_ExecuteSfxWaitAll( lpwszValue );
	}
#endif // _SFX_USE_PREFIX_WAITALL

#ifdef _SFX_USE_ELEVATION
	bool fInElevation = false;
	if( (lpwszValue = IsSfxSwitch(str,CMDLINE_SFXELEVATION)) != NULL )
	{
		fInElevation = true;
		str = lpwszValue;
	}
#endif // _SFX_USE_ELEVATION
#ifdef _DEBUG
	strModulePathName = L"S:\\tmp\\setup_button_x86_x64.exe";
	strModulePathName = L"S:\\tmp\\Aida64AE.exe";
	strModulePathName = L"C:\\tmp\\vars.exe";
#else
	if( ::GetModuleFileName( NULL, strModulePathName.GetBuffer(MAX_PATH*2), MAX_PATH*2 ) == 0 )
	{
		SfxErrorDialog( TRUE, ERR_MODULEPATHNAME );
		return ERRC_GET_PATHNAME;
	}
#endif
	strModulePathName.ReleaseBuffer();

	if( (lpwszValue = IsSfxSwitch( str, CMDLINE_SFXTEST )) != NULL )
#ifdef _SFX_USE_TEST
	{
		if( lpwszValue[0] != L':' )
			return ERRC_SFXTEST;
		switch( lpwszValue[1]|L' ' )
		{
		case L'd':
			// dialogs test
			if( lpwszValue[1] == L'D' )
			{
				if( lpwszValue[2] != L':' )
					return ERRC_SFXTEST;
				lpwszValue += 3;
				while( ((unsigned)*lpwszValue) > L' ' && *lpwszValue != L':' )
				{
					TSD_Flags += *lpwszValue++;
				}
				if( TSD_Flags.IsEmpty() != false )
					return ERRC_SFXTEST;
			}
			else
				lpwszValue += 2;
			TSD_ExtractTimeout = 0;
			if( *lpwszValue == L':' )
			{
				TSD_ExtractTimeout=StringToLong( lpwszValue+1 );
			}
			if( TSD_ExtractTimeout > 3600 || TSD_ExtractTimeout == 0 )
					TSD_ExtractTimeout = 10;
			nTestModeType = TMT_DIALOGS;
			break;
		case L'a':
			nTestModeType = TMT_ARCHIVE;
			break;
		case L'c':
			if( lpwszValue[2] == L'c' )
				nTestModeType = TMT_CHECK_CONFIG;
			else
				nTestModeType = TMT_DIALOGS_TO_STDOUT;
			break;
		case L'o':
			if( lpwszValue[2] != L':' )
				return 0;
			switch( lpwszValue[3] )
			{
			case '1':
				return SFXBUILD_OPTIONS1;
			case '2':
				return SFXBUILD_OPTIONS2;
			case '3':
				return SFXBUILD_OPTIONS3;
			case '4':
				return SFXBUILD_OPTIONS4;
			case '5':
				return SFXBUILD_OPTIONS5;
			default:
				return 0;
			}
		case L'v':
			return 0x4000 | VERSION_REVISION;
		default:
			return ERRC_SFXTEST;
		}
	
		while( ((unsigned)*lpwszValue) > L' ' ) lpwszValue++;
		str = lpwszValue;
		
		if( (lpwszValue = IsSfxSwitch( str,CMDLINE_SFXCONFIG )) != NULL )
		{
			if( *lpwszValue == L':' ) lpwszValue++;
			SKIP_WHITESPACES_W( lpwszValue );
			tmpstr.Empty();
			str = LoadQuotedString( lpwszValue, tmpstr );
			if( nTestModeType != TMT_ARCHIVE )
				strModulePathName = tmpstr;
		}
	}
#else
	{
		return ERRC_SFXTEST;
	}
#endif // _SFX_USE_TEST

	strSfxFolder = strModulePathName;
	strSfxName = strModulePathName;
	int nPos = GetDirectorySeparatorPos( strModulePathName );
	if( nPos >= 0 )
	{
		strSfxFolder.ReleaseBuffer( nPos );
		strSfxName = ((const wchar_t *)strModulePathName) + nPos + 1;
		strTitle = ((const wchar_t *)strModulePathName)+nPos+1;
		if( (nPos = strTitle.ReverseFind( L'.' )) > 0 )
			strTitle.ReleaseBuffer( nPos );
		strErrorTitle = strTitle;
		strErrorTitle += GetLanguageString( STR_ERROR_SUFFIX );
		lpwszTitle = strTitle;
		lpwszErrorTitle = strErrorTitle;
#ifdef SFX_CRYPTO
		lpwszPasswordTitle = lpwszTitle;
#endif // SFX_CRYPTO
	}
#ifdef _SFX_USE_PREFIX_PLATFORM
	strOSPlatform = GetPlatformName();
#endif // _SFX_USE_PREFIX_PLATFORM
	
	if( gSfxArchive.Init(strModulePathName) == false )
	{
		SfxErrorDialog( TRUE, ERR_OPEN_ARCHIVE, (LPCWSTR)strModulePathName );
		return ERRC_OPEN_ARCHIVE;
	}

	// Read SFX config
	CSfxStringA	textConfig;
	int	result;

	gConfig.Clear();
	if( (result=LoadAndParseConfig(gSfxArchive.GetStream(),FALSE,&textConfig)) != 0 )
		return result;

#ifdef _SFX_USE_TEST
	if( nTestModeType == 0 && (lpwszValue = IsSfxSwitch( str,CMDLINE_SFXCONFIG )) != NULL )
#else
	if( (lpwszValue = IsSfxSwitch( str,CMDLINE_SFXCONFIG )) != NULL )
#endif // _SFX_USE_TEST
	{
		if( *lpwszValue == L':' ) lpwszValue++;
		SKIP_WHITESPACES_W( lpwszValue );
		if( *lpwszValue != L'\0' && SaveConfiguration( lpwszValue, textConfig ) == false )
		{
			SfxErrorDialog( FALSE, ERR_WRITE_CONFIG );
			return ERRC_CONFIG_DATA;
		}
		return ERRC_NONE;
	}
	
	// parse command line
	gSfxArchive.SetCommandLine(str);
	if( (str=ParseCommandLineParameters()) == NULL )
		return ERR_CONFIG_CMDLINE;
SetConfigVariables();
#if defined(SFX_VOLUMES) && defined(_SFX_USE_VOLUME_NAME_STYLE)
	// Volumes in style 1 ("name.exe" + "name.001"...): the style is known only
	// after the embedded config is parsed, so attach the volumes now and re-open.
	if( nVolumeNameStyle != 0 && gSfxArchive.GetStreamsCount() <= 1 )
		gSfxArchive.Reopen( strModulePathName );
#endif // SFX_VOLUMES && _SFX_USE_VOLUME_NAME_STYLE
SfxLogInit();

    // External archive mode:
    //   loader.exe archive.7z   (argument or drag & drop)
    //   Archive="archive.7z"    (optional fallback in loader config)
    bool fExternalArchive = false;
    CSfxStringU archivePathSrc;
    bool fArchiveFromCmdLine = false;
    if( g_strCmdLineArchive.IsEmpty() == false )
    {
        archivePathSrc = g_strCmdLineArchive;
        fArchiveFromCmdLine = true;
    }
    else if( (lpwszValue = GetTextConfigValue(CFG_ARCHIVE)) != NULL && lpwszValue[0] != L'\0' )
    {
        archivePathSrc = lpwszValue;
        ReplaceVariables( archivePathSrc );
    }
    if( archivePathSrc.IsEmpty() == false )
    {
        CSfxStringU archivePath = archivePathSrc;
        bool fAbsolute = (archivePath.Len() > 1 && archivePath[1] == L':')
            || (archivePath.Len() > 1 && archivePath[0] == L'\\' && archivePath[1] == L'\\');
        if( fAbsolute == false && fArchiveFromCmdLine == false )
        {
            CSfxStringU strPath = strSfxFolder;
            if( strPath.Len() > 0 && strPath[strPath.Len()-1] != L'\\' )
                strPath += L'\\';
            strPath += (LPCWSTR)archivePath;
            archivePath = strPath;
        }
        if( gSfxArchive.Reopen( archivePath ) == false )
        {
            SfxErrorDialog( TRUE, ERR_OPEN_ARCHIVE, (LPCWSTR)archivePath );
            return ERRC_OPEN_ARCHIVE;
        }
        fExternalArchive = true;
    }

    if( gSfxArchive.Open() == false )
    {
        // report the missing volume instead of failing silently at extraction
        LPCWSTR lpwszErrorName = gSfxArchive.GetVolumeError();
        if( lpwszErrorName == NULL || lpwszErrorName[0] == L'\0' )
            lpwszErrorName = (LPCWSTR)strModulePathName;
        if( GUIMode != GUIMODE_HIDDEN )
            SfxErrorDialog( FALSE, ERR_OPEN_ARCHIVE, lpwszErrorName );
        return ERRC_OPEN_ARCHIVE;
    }
    // Reinit config due possible changes in compressed version
    if( fExternalArchive == false )
    {
        gConfig.Clear();
        LoadAndParseConfig(gSfxArchive.GetStream(),NULL);
        SetConfigVariables();
    }
	// compressed config
	gSfxArchive.LoadAndParseCompressedConfig();
	ParseCommandLineParameters();
	SetConfigVariables();
#if defined(SFX_VOLUMES) && defined(_SFX_USE_VOLUME_NAME_STYLE)
	// VolumeNameStyle may come from the compressed config (stored inside the
	// archive): attach the volumes now and re-open the handler once.
	if( fExternalArchive == false && nVolumeNameStyle != 0 && gSfxArchive.GetStreamsCount() <= 1 )
	{
		gSfxArchive.Close();
		if( gSfxArchive.Reopen( strModulePathName ) )
			gSfxArchive.Open();
	}
#endif // SFX_VOLUMES && _SFX_USE_VOLUME_NAME_STYLE

#ifdef _SFX_USE_ELEVATION
// Check elevation
	if( fInElevation == false && (MiscFlags&MISCFLAGS_ELEVATE) != 0 && IsRunAsAdmin() == FALSE )
	{
		CSfxStringU sfxPath;
		CSfxStringU executeString;
		CSfxStringU strCmdLine = LoadQuotedString( ::GetCommandLine(), sfxPath );
		executeString = L"\"" + sfxPath + L"\" -" + CMDLINE_SFXELEVATION + L' ' + strCmdLine;
		::SetProcessWorkingSetSize( ::GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1 );
		if( SfxExecute( executeString, SFXEXEC_RUNAS, NULL ) == FALSE )
			return ERRC_ELEVATE;
		return ERRC_NONE;
	}
#endif // _SFX_USE_ELEVATION

	// extra environment variables
	int		nIndex = 0;
	while( (lpwszValue = GetTextConfigValue( CFG_SETENVIRONMENT, &nIndex)) != NULL )
	{
		CSfxStringU strEnvName = lpwszValue;
		nPos = strEnvName.Find( L'=');
		if( nPos <= 0 )
			break;
		strEnvName.ReleaseBuffer( nPos );
		SfxAddEnvironmentVar( (LPCWSTR)strEnvName, lpwszValue+nPos+1 );
		nIndex++;
	}

	SetEnvironment();

	if( GUIFlags == -1 )
		GUIFlags = 0;

	if( gSfxArchive.GetShowHelp() != false )
	{
		if( (lpwszValue = GetTextConfigValue(CFG_HELP_TEXT)) == NULL )
				lpwszValue = GetLanguageString( STR_DEFAULT_HELP_TEXT );
		CSfxDialog_HelpText	dlg;
		dlg.Show( SD_OK|SD_ICONINFORMATION, lpwszTitle, lpwszValue );
		return ERRC_NONE;
	}

	if( gSfxArchive.GetAssumeYes() != false )
		GUIFlags &= (~GUIFLAGS_CONFIRM_CANCEL);

	(void)CoInitialize( NULL );

	// Path to extract
	if( (lpwszValue = GetTextConfigValue(CFG_INSTALLPATH)) != NULL )
	{
		extractPath = lpwszValue;
		fUseInstallPath = true;
	}

	ReplaceVariables( extractPath );

#ifdef _SFX_USE_SINGLEINSTANCE
	// Single-instance guard: only for a fixed (shared) extraction path.
	// 0 = off (default), 1 = silent exit, 2 = activate the running copy.
	if( (lpwszValue = GetTextConfigValue(CFG_SINGLEINSTANCE)) != NULL )
	{
		long nMode = StringToLong( lpwszValue );
		if( nMode > 2 ) nMode = 2;
		nSingleInstanceMode = (int)nMode;
		if( nMode > 0 && fUseInstallPath != false && extractPath.IsEmpty() == false )
		{
			if( SfxTryAcquireSingleInstance( extractPath ) == false )
			{
				if( nMode == 2 )
					SfxActivateRunningInstance( extractPath );
				return ERRC_NONE;
			}
			if( nMode == 2 )
				SfxStartInstanceWatcher( extractPath );
		}
	}
#endif // _SFX_USE_SINGLEINSTANCE

#ifdef _SFX_USE_BEGINPROMPTTIMEOUT
	if( (lpwszValue = GetTextConfigValue( CFG_BEGINPROMPTTIMEOUT)) != NULL )
		BeginPromptTimeout = StringToLong( lpwszValue );
#endif // _SFX_USE_BEGINPROMPTTIMEOUT

#ifdef _SFX_USE_TEST
	if( TSD_Flags.IsEmpty() == false )
	{
		return TestSfxDialogs();
	}
	if( nTestModeType == TMT_DIALOGS_TO_STDOUT )
	{
		return TestSfxDialogsToStdout();
	}
#endif // _SFX_USE_TEST

#ifdef _SFX_USE_CONFIG_EARLY_EXECUTE
	#if defined(_SFX_USE_ENVIRONMENT_VARS) && defined(_SFX_USE_SFXAPI)
		SfxAddEnvironmentVarInternal( SFX_ENV_VAR_APIPATH, L"", false );
		SetEnvironment();
	#endif // defined(_SFX_USE_ENVIRONMENT_VARS) && defined(_SFX_USE_SFXAPI)
	if( GetTextConfigValue(CFG_EXECUTE_ON_LOAD) != NULL && gSfxArchive.GetNoRun() == false )
	{
		#if defined(_SFX_USE_ENVIRONMENT_VARS) && defined(_SFX_USE_SFXAPI)
			SfxAddEnvironmentVarInternal( SFX_ENV_VAR_APIPATH, gSfxApi.GetDirectory(), false );
			SetEnvironment();
		#endif // defined(_SFX_USE_ENVIRONMENT_VARS) && defined(_SFX_USE_SFXAPI)
		ExecuteBatch( CFG_EXECUTE_ON_LOAD, SFX_ON_LOAD_DIR, L"", CSfxStringU(L""), L"" );
	}
#endif // _SFX_USE_CONFIG_EARLY_EXECUTE

#ifdef _SFX_USE_SFXAPI
	gSfxArchive.HidePrepareDialog();
#endif // _SFX_USE_SFXAPI
#ifdef _DEBUG
	DWORD dwTime = ::GetTickCount()-dwStartTime;
	CHAR msg[128];
	wsprintfA( msg, "Time: %u msec", dwTime );
	::MessageBoxA( NULL, msg, "Time", MB_OK );
#endif // -DEBUG
	// Begin prompt
Loc_BeginPrompt:
	if( (lpwszValue = GetTextConfigValue(CFG_BEGINPROMPT)) != NULL )
	{
		if( gSfxArchive.GetAssumeYes() == false )
		{
			if( SfxBeginPrompt( lpwszTitle, lpwszValue ) == FALSE )
			{
				return ERRC_CANCELED;
			}
			if( GetKeyState(VK_SHIFT)&0x8000 )
			{
				gSfxArchive.SetAutoInstall(true);
				gSfxArchive.SetAssumeYes(true);
			}
#ifdef _SFX_USE_BEGINPROMPTTIMEOUT
			if( CSfxDialog::IsTimedOut() != FALSE )
				GUIFlags &= ~GUIFLAGS_EXTRACT_PATH2;
#endif // _SFX_USE_BEGINPROMPTTIMEOUT
		}
	}

	// check autoinstall
	CSfxStringU	executeName;
	if( gSfxArchive.GetAutoInstall() != false )
	{
		LPCWSTR p1 = gSfxArchive.GetBatchInstall();
		do {
			executeName = CFG_AUTOINSTALL;
			executeName += *p1;
			if( GetTextConfigValue( executeName ) == NULL )
			{
				// auto install command not found
				SfxErrorDialog( FALSE, ERR_AUTOINSTALL_NOTFOUND, (LPCWSTR)executeName );
				return ERRC_BAD_AUTOINSTALL;
			}
			p1++;
		} while( (*p1 >= L'0' && *p1 <= L'9') ||
				 (*p1 >= L'a' && *p1 <= L'z') ||
				 (*p1 >= L'A' && *p1 <= L'Z') );
		executeName = CFG_AUTOINSTALL;
	}

	bool fUseExecuteFile = false;
	// check for 'ExecuteFile' (only if no 'autoinstall' commands)
	if( executeName.IsEmpty() != false && GetTextConfigValue( CFG_EXECUTEFILE ) != NULL )
	{
		executeName = CFG_EXECUTEFILE;
		fUseExecuteFile = true;
	}
	// if no 'ExecuteFile' - try 'RunProgram'
	if( executeName.IsEmpty() != false && GetTextConfigValue( CFG_RUNPROGRAM ) != NULL )
		executeName = CFG_RUNPROGRAM;

	// Path to extract
	if( gSfxArchive.GetAutoInstall() == false && gSfxArchive.GetAssumeYes() == false &&
			(GUIFlags&(GUIFLAGS_EXTRACT_PATH2|GUIFLAGS_EXTRACT_PATH1)) == GUIFLAGS_EXTRACT_PATH2 &&
				SfxExtractPathDialog( lpwszExtractPathTitle, lpwszExtractPathText ) == FALSE )
	{
		if( fUseBackward != false )
			goto Loc_BeginPrompt;
		return ERRC_CANCELED;
	}

	// 
	if( extractPath.IsEmpty() != false )
	{
		// create temp path
		extractPath = CreateTempName( L"" );
		fUseInstallPath = false;
	}
	else
	{
		CSfxStringU tmp = extractPath;
		ReplaceVariables( tmp );
		if( tmp.IsEmpty() == false )
			extractPath = tmp;
		fUseInstallPath = true;
	}

	if( extractPath[extractPath.Len()-1] == L'\\' || extractPath[extractPath.Len()-1] == L'/' )
	{
		extractPath.ReleaseBuffer( extractPath.Len()-1 );
	}

	// relative InstallPath is resolved against the module folder
	SfxMakeLongPath( extractPath, strSfxFolder );

	if( gSfxArchive.GetAssumeYes() != false )
		MiscFlags |= MISCFLAGS_NO_CHECK_DISK_FREE | MISCFLAGS_NO_CHECK_RAM;
#ifdef _SFX_USE_CONFIG_EARLY_EXECUTE
	CSfxStringU strPreExtract = CFG_PREEXTRACT;
	strPreExtract += *gSfxArchive.GetBatchInstall();
	if( GetTextConfigValue(strPreExtract) != NULL && gSfxArchive.GetNoRun() == false )
	{
		#if defined(_SFX_USE_ENVIRONMENT_VARS) && defined(_SFX_USE_SFXAPI)
			SfxAddEnvironmentVarInternal( SFX_ENV_VAR_APIPATH, gSfxApi.GetDirectory(), false );
			SetEnvironment();
		#endif // defined(_SFX_USE_ENVIRONMENT_VARS) && defined(_SFX_USE_SFXAPI)
		ExecuteBatch( CFG_PREEXTRACT, SFX_ON_LOAD_DIR, gSfxArchive.GetBatchInstall(), CSfxStringU(L""), L"" );
	}
#endif // _SFX_USE_CONFIG_EARLY_EXECUTE

	// PayloadCRC= self-check: abort before extracting if the 7z data block is corrupted
	if( CheckPayloadCRC() == false )
	{
		::SfxLogAppend( L"[crc] MISMATCH" );
		if( GUIMode != GUIMODE_HIDDEN )
			SfxErrorDialog( FALSE, ERR_7Z_CRC_ERROR );
		return ERRC_EXTRACT;
	}

#ifdef _SFX_USE_TEST
	HRESULT hrStatus;
	if( TSD_ExtractTimeout == 0 )
		hrStatus = ExtractArchive( extractPath );
	else
	{
		if( ExtractDialog() != FALSE )
			hrStatus = S_OK;
		else
			hrStatus = E_FAIL;
	}
#else
	// RetryAttempts= : allow N retries when extraction fails (transient lock/AV)
	LPCWSTR lpwszRetry = GetTextConfigValue( CFG_RETRYATTEMPTS );
	nRetryAttempts = (lpwszRetry != NULL) ? _wtoi( lpwszRetry ) : 0;
	if( nRetryAttempts < 0 ) nRetryAttempts = 0;
	HRESULT hrStatus = ExtractArchive( extractPath );
	int nTry = 0;
	while( hrStatus != S_OK && nTry < nRetryAttempts )
	{
		SfxLogAppend( L"[extract] retry" );
		::Sleep( 300 );
		SfxCleanup();
		hrStatus = ExtractArchive( extractPath );
		nTry++;
	}
#endif // _SFX_USE_TEST

	// delete temporary folder
	if( hrStatus != S_OK )
	{
		SfxLogAppend( L"[extract] FAILED" );
		SfxCleanup();
		return ERRC_EXTRACT;
	}
	SfxLogAppend( L"[extract] OK" );

	//
	SetEnvironment();

	if( gSfxArchive.GetNoRun() == false || fUseInstallPath == false )
	{
		// execute program
		CSfxStringU	ustrDirPrefix;
		CSfxStringU	ustrRunProgram;
		if( gSfxArchive.GetAutoInstall() == false )
			GetConfigDirectory( ustrDirPrefix );

		if( executeName.IsEmpty() != false )
		{
			if( fUseInstallPath == false )
			{
				lpwszValue = L"setup.exe";
				ustrRunProgram = extractPath + L"\\" + lpwszValue;
				if( ::GetFileAttributes( ustrRunProgram ) == (DWORD)-1 )
				{
					// nothing to execute
					SfxLogAppend( L"[run] setup.exe not found" );
					SfxCleanup();
					SfxErrorDialog( FALSE, ERR_NO_SETUP_EXE );
					return ERRC_NOTHING_TO_EXECUTE;
				}
				SfxLogAppend( L"[run] executing setup.exe" );
				ExecuteConfigProgram( CSfxStringU(lpwszValue), extractPath, fUseExecuteFile, ustrDirPrefix, str );
			}
		}
		else
		{
			SfxLogAppend( L"[run] executing batch" );
			ExecuteBatch( executeName, extractPath, gSfxArchive.GetBatchInstall(), ustrDirPrefix, str );
		}
	
#ifdef _SFX_USE_TEST
		if( nTestModeType == 0 )
		{
#endif // _SFX_USE_TEST
		// create shortcuts
#define SH_TEST
		SetEnvironment();
		ProcessPostExecuteSub( PostExecute_Shortcut, CFG_SHORTCUT, gSfxArchive.GetBatchInstall(), ShortcutDefault );

		::SetCurrentDirectory( strSfxFolder );
		ProcessPostExecuteSub( PostExecute_Delete, CFG_DELETE, gSfxArchive.GetBatchInstall(), DeleteDefault );

		SfxCleanup();
#ifdef _SFX_USE_TEST
		}
#endif // _SFX_USE_TEST
	}
	if( FinishMessage == -1 && gSfxArchive.GetAssumeYes() == false )
	{
			FinishMessage = 1;
	}
	if( FinishMessage > 0 && (lpwszValue = GetTextConfigValue( CFG_FINISHMESSAGE )) != NULL )
	{
		if( FinishMessage > FINISHMESSAGE_MAX_TIMEOUT )
			FinishMessage = FINISHMESSAGE_MAX_TIMEOUT;
		CSfxDialog_FinishMessage	dlg;
		dlg.Show( SD_OK|SD_ICONINFORMATION, lpwszTitle, lpwszValue );
	}

	// self delete
#ifdef _SFX_USE_TEST
	if( gSfxArchive.GetNoRun() == false && nTestModeType == 0 )
#else
	if( gSfxArchive.GetNoRun() == false )
#endif // _SFX_USE_TEST
	{
		switch( SelfDelete )
		{
		case 0:
			break;
		case 1:
			DeleteSFX( strModulePathName );
			break;
		default:
			if( (lpwszValue = GetTextConfigValue(CFG_SELFDELETE)) != NULL && lpwszValue[0] == L'1' )
				DeleteSFX( strModulePathName );
			break;
		}
	}
	
	SfxLogAppend( L"[done] OK" );
	return ERRC_NONE;
}

void DeleteSFX( CSfxStringU moduleName )
{
	WCHAR	szRoot[4];
	if( moduleName[1] == L':' && (moduleName[2] == L'\\' || moduleName[2] == L'/') )
	{
		szRoot[0] = moduleName[0];
		szRoot[1] = L':'; szRoot[2] = L'\\'; szRoot[3] = L'\0';
		if( ::GetDriveType(szRoot) != DRIVE_FIXED )
			return;
	}
	else
	{
		return;
	}

	// CleanMode bit 1: silent self-delete. A direct DeleteFileW of our own
	// still-loaded image always fails, so schedule a reboot-delete as a
	// catch-all fallback and then fall through to the hidden self-cleaning
	// .cmd helper below, which retries the delete after this process exits
	// and removes itself (no visible window, transient trace).
	if( nCleanMode & 1 )
	{
		ClearFileAttributes( moduleName );
		::MoveFileExW( moduleName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT );
	}

	CSfxStringU tmpPath = CreateTempName( L".cmd" );

	HANDLE hfCmd = ::CreateFile( tmpPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hfCmd != INVALID_HANDLE_VALUE )
	{
		CSfxStringU cmdBody;
		cmdBody = L":Repeat\r\n";
		cmdBody += L"del \""; cmdBody += moduleName; cmdBody += L"\"\r\n";
		cmdBody += L"if exist \""; cmdBody += moduleName; cmdBody += L"\" goto Repeat\r\n";
		cmdBody += L"del \""; cmdBody += tmpPath; cmdBody += L"\"\r\n";
		DWORD dwWrited;
		BOOL fWrited = WriteFile( hfCmd, SfxUnicodeStringToMultiByte(cmdBody, CP_OEMCP), cmdBody.Len(), &dwWrited, NULL );
		::CloseHandle( hfCmd );
		if( fWrited == FALSE || dwWrited != (DWORD)cmdBody.Len() )
		{
			DeleteFileOrDirectoryAlways( tmpPath );
			return;
		}
		ClearFileAttributes( moduleName );
		// execute cmd
		ShellExecute( NULL, L"open", tmpPath, NULL, NULL, SW_HIDE );
	}
}
