/* Automatically generated header! Do not edit! */

#ifndef _PPCINLINE_FTP_H
#define _PPCINLINE_FTP_H

#ifndef __PPCINLINE_MACROS_H
#include <ppcinline/macros.h>
#endif /* !__PPCINLINE_MACROS_H */

#ifndef FTP_BASE_NAME
#define FTP_BASE_NAME FTPBase
#endif /* !FTP_BASE_NAME */

#define FTPLogin(__p0, __p1, __p2) \
	(((LONG (*)(void *, struct FTPHandler *, CONST STRPTR , CONST STRPTR ))*(void**)((long)(FTP_BASE_NAME) - 52))((void*)(FTP_BASE_NAME), __p0, __p1, __p2))

#define FTPSend(__p0, __p1, __p2, __p3, __p4) \
	(((LONG (*)(void *, struct FTPHandler *, CONST STRPTR , CONST STRPTR , UBYTE , UQUAD ))*(void**)((long)(FTP_BASE_NAME) - 154))((void*)(FTP_BASE_NAME), __p0, __p1, __p2, __p3, __p4))

#define FTPRead(__p0, __p1, __p2) \
	(((int (*)(void *, struct FTPHandler *, APTR , LONG ))*(void**)((long)(FTP_BASE_NAME) - 64))((void*)(FTP_BASE_NAME), __p0, __p1, __p2))

#define FTPFeat(__p0) \
	(((STRPTR (*)(void *, struct FTPHandler *))*(void**)((long)(FTP_BASE_NAME) - 196))((void*)(FTP_BASE_NAME), __p0))

#define FTPConnect(__p0, __p1, __p2) \
	(((struct FTPHandler *(*)(void *, CONST STRPTR , int , struct TagItem *))*(void**)((long)(FTP_BASE_NAME) - 40))((void*)(FTP_BASE_NAME), __p0, __p1, __p2))

#define FTPSize(__p0, __p1, __p2) \
	(((LONG (*)(void *, struct FTPHandler *, CONST STRPTR , UQUAD *))*(void**)((long)(FTP_BASE_NAME) - 136))((void*)(FTP_BASE_NAME), __p0, __p1, __p2))

#define FTPAccess(__p0, __p1, __p2, __p3, __p4, __p5) \
	(((LONG (*)(void *, struct FTPHandler *, struct FTPHandler **, CONST STRPTR , LONG , LONG , UQUAD ))*(void**)((long)(FTP_BASE_NAME) - 58))((void*)(FTP_BASE_NAME), __p0, __p1, __p2, __p3, __p4, __p5))

#define FTPSysType(__p0, __p1, __p2) \
	(((STRPTR (*)(void *, struct FTPHandler *, STRPTR , LONG ))*(void**)((long)(FTP_BASE_NAME) - 88))((void*)(FTP_BASE_NAME), __p0, __p1, __p2))

#define FTPMakeDir(__p0, __p1) \
	(((LONG (*)(void *, struct FTPHandler *, CONST STRPTR ))*(void**)((long)(FTP_BASE_NAME) - 94))((void*)(FTP_BASE_NAME), __p0, __p1))

#define FTPDir(__p0, __p1) \
	(((struct RemoteFile *(*)(void *, struct FTPHandler *, CONST STRPTR ))*(void**)((long)(FTP_BASE_NAME) - 130))((void*)(FTP_BASE_NAME), __p0, __p1))

#define FTPClose(__p0) \
	(((LONG (*)(void *, struct FTPHandler *))*(void**)((long)(FTP_BASE_NAME) - 76))((void*)(FTP_BASE_NAME), __p0))

#define FTPList(__p0, __p1) \
	(((struct RemoteFile *(*)(void *, struct FTPHandler *, CONST STRPTR ))*(void**)((long)(FTP_BASE_NAME) - 124))((void*)(FTP_BASE_NAME), __p0, __p1))

#define FTPGet(__p0, __p1) \
	(((LONG (*)(void *, struct FTPHandler *, struct TagItem *))*(void**)((long)(FTP_BASE_NAME) - 202))((void*)(FTP_BASE_NAME), __p0, __p1))

#define FTPDeleteDir(__p0, __p1) \
	(((LONG (*)(void *, struct FTPHandler *, CONST STRPTR ))*(void**)((long)(FTP_BASE_NAME) - 112))((void*)(FTP_BASE_NAME), __p0, __p1))

#define FTPDeleteFile(__p0, __p1) \
	(((LONG (*)(void *, struct FTPHandler *, CONST STRPTR ))*(void**)((long)(FTP_BASE_NAME) - 166))((void*)(FTP_BASE_NAME), __p0, __p1))

#define FTPGetLastMessage(__p0) \
	(((STRPTR (*)(void *, struct FTPHandler *))*(void**)((long)(FTP_BASE_NAME) - 34))((void*)(FTP_BASE_NAME), __p0))

#define FTPQuit(__p0) \
	(((LONG (*)(void *, struct FTPHandler *))*(void**)((long)(FTP_BASE_NAME) - 172))((void*)(FTP_BASE_NAME), __p0))

#define FTPSet(__p0, __p1) \
	(((LONG (*)(void *, struct FTPHandler *, struct TagItem *))*(void**)((long)(FTP_BASE_NAME) - 46))((void*)(FTP_BASE_NAME), __p0, __p1))

#define FTPChdir(__p0, __p1) \
	(((LONG (*)(void *, struct FTPHandler *, CONST STRPTR ))*(void**)((long)(FTP_BASE_NAME) - 100))((void*)(FTP_BASE_NAME), __p0, __p1))

#define FTPDeleteFileList(__p0) \
	(((void (*)(void *, struct RemoteFile *))*(void**)((long)(FTP_BASE_NAME) - 178))((void*)(FTP_BASE_NAME), __p0))

#define FTPGetPath(__p0, __p1, __p2) \
	(((STRPTR (*)(void *, struct FTPHandler *, STRPTR , LONG ))*(void**)((long)(FTP_BASE_NAME) - 118))((void*)(FTP_BASE_NAME), __p0, __p1, __p2))

#define FTPModDate(__p0, __p1, __p2, __p3) \
	(((LONG (*)(void *, struct FTPHandler *, CONST STRPTR , STRPTR , LONG ))*(void**)((long)(FTP_BASE_NAME) - 142))((void*)(FTP_BASE_NAME), __p0, __p1, __p2, __p3))

#define FTPCDUp(__p0) \
	(((LONG (*)(void *, struct FTPHandler *))*(void**)((long)(FTP_BASE_NAME) - 106))((void*)(FTP_BASE_NAME), __p0))

#define FTPWrite(__p0, __p1, __p2) \
	(((int (*)(void *, struct FTPHandler *, APTR , LONG ))*(void**)((long)(FTP_BASE_NAME) - 70))((void*)(FTP_BASE_NAME), __p0, __p1, __p2))

#define FTPReceive(__p0, __p1, __p2, __p3, __p4) \
	(((LONG (*)(void *, struct FTPHandler *, CONST STRPTR , STRPTR , UBYTE , UQUAD ))*(void**)((long)(FTP_BASE_NAME) - 148))((void*)(FTP_BASE_NAME), __p0, __p1, __p2, __p3, __p4))

#define FTPSite(__p0, __p1) \
	(((LONG (*)(void *, struct FTPHandler *, CONST STRPTR ))*(void**)((long)(FTP_BASE_NAME) - 82))((void*)(FTP_BASE_NAME), __p0, __p1))

#define FTPNoop(__p0) \
	(((LONG (*)(void *, struct FTPHandler *))*(void**)((long)(FTP_BASE_NAME) - 184))((void*)(FTP_BASE_NAME), __p0))

#define FTPHelp(__p0, __p1) \
	(((STRPTR (*)(void *, struct FTPHandler *, STRPTR ))*(void**)((long)(FTP_BASE_NAME) - 190))((void*)(FTP_BASE_NAME), __p0, __p1))

#define FTPRename(__p0, __p1, __p2) \
	(((LONG (*)(void *, struct FTPHandler *, CONST STRPTR , CONST STRPTR ))*(void**)((long)(FTP_BASE_NAME) - 160))((void*)(FTP_BASE_NAME), __p0, __p1, __p2))

#endif /* !_PPCINLINE_FTP_H */
