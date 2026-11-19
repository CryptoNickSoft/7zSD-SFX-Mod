/*---------------------------------------------------------------------------*/
/* File:        strings.h                                                    */
/* Created:     Sun, 07 Feb 2016 08:08:09 GMT                                */
/*              by Oleg N. Scherbakov, mailto:oleg@7zsfx.info                */
/* Last update: Fri, 01 Apr 2016 06:27:03 GMT                                */
/*              by Oleg N. Scherbakov, mailto:oleg@7zsfx.info                */
/* Revision:    54                                                           */
/*---------------------------------------------------------------------------*/
#ifndef _STRINGS_H_INCLUDED_
#define _STRINGS_H_INCLUDED_

#define SKIP_WHITESPACES_W(str) 	while( *str != L'\0' && ((unsigned)*str) <= L' ' ) str++;

class CSfxStringA : public AString
{
public:
	CSfxStringA() : AString() {};
	CSfxStringA(char c) : AString(c) {};
	operator const char *() const { return Ptr(); }
	CSfxStringA &operator=(const char *s) { *((AString *)this)=s; return *this; };
	int Length() const { return (int)(AString::Len()); }
	char * GetBuffer(unsigned minLen)
	{
		CSfxStringA tmp = *this;
		strncpy( GetBuf(minLen), tmp.Ptr(), tmp.Len() );
		return (char *)Ptr();
	};

	void ReleaseBuffer() { ReleaseBuffer( MyStringLen(Ptr()) ); };
	void ReleaseBuffer(unsigned newLen) { ReleaseBuf_SetEnd(newLen); };
};

class CSfxStringU : public UString
{
public:
	CSfxStringU::CSfxStringU() : UString() {};
	CSfxStringU::CSfxStringU(const wchar_t *s) : UString(s){};
	CSfxStringU::CSfxStringU(const char *s) : UString(){ SetFromAscii(s); };

	void AddAscii(const char *s)
	{
		if (s == NULL || *s == 0) return;
		int len = ::MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
		if (len > 1)
		{
			UString tmp;
			wchar_t *buf = tmp.GetBuf((unsigned)len);
			::MultiByteToWideChar(CP_ACP, 0, s, -1, buf, len);
			tmp.ReleaseBuf_SetEnd((unsigned)(len - 1));
			*((UString *)this) += tmp;
		}
	}
	void SetFromAscii(const char *s)
	{
		UString tmp;
		if (s != NULL && *s != 0)
		{
			int len = ::MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
			if (len > 1)
			{
				wchar_t *buf = tmp.GetBuf((unsigned)len);
				::MultiByteToWideChar(CP_ACP, 0, s, -1, buf, len);
				tmp.ReleaseBuf_SetEnd((unsigned)(len - 1));
			}
		}
		*((UString *)this) = tmp;
	};
	int Length() const { return (int)(UString::Len()); }
	CSfxStringU &operator=(const wchar_t *s) { *((UString *)this)=s; return *this; };
	CSfxStringU &operator=(const char *s) { SetFromAscii(s); return *this; };
	void ReleaseBuffer() { ReleaseBuffer( MyStringLen(Ptr()) ); };
	void ReleaseBuffer(unsigned newLen) { ReleaseBuf_SetEnd(newLen); };
	wchar_t * GetBuffer(unsigned minLen)
	{
		CSfxStringU tmp = *this;
		wcsncpy( GetBuf(minLen), tmp.Ptr(), tmp.Len() );
		return (wchar_t *)Ptr();
	};
};

CSfxStringU	 GetWindowStringU( HWND hwnd );

#endif // _STRINGS_H_INCLUDED_
