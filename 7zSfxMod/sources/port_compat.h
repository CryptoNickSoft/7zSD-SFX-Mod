#ifndef _PORT_COMPAT_H_INCLUDED_
#define _PORT_COMPAT_H_INCLUDED_

/* Compatibility layer: 7zSFX (2016 API) -> 7-Zip 26.06 */

/* The modern Z7 macros end with "private:", while the old MY_* macros
   left the following members public. Restore the old behaviour. */

#ifndef MY_UNKNOWN_IMP1
#define MY_UNKNOWN_IMP1(i)        Z7_COM_UNKNOWN_IMP_1(i) public:
#endif
#ifndef MY_UNKNOWN_IMP2
#define MY_UNKNOWN_IMP2(i1,i2)    Z7_COM_UNKNOWN_IMP_2(i1,i2) public:
#endif
#ifndef MY_UNKNOWN_IMP3
#define MY_UNKNOWN_IMP3(i1,i2,i3) Z7_COM_UNKNOWN_IMP_3(i1,i2,i3) public:
#endif

#define SFX_PORT_IFACE_DECL(m) STDMETHOD(m);

#ifndef INTERFACE_IArchiveOpenCallback
#define INTERFACE_IArchiveOpenCallback(x) Z7_IFACEM_IArchiveOpenCallback(SFX_PORT_IFACE_DECL)
#endif

#endif // _PORT_COMPAT_H_INCLUDED_
