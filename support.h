/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __SUPPORT_H__
#define __SUPPORT_H__

#include <exec/types.h>
#include <proto/ezxml.h>
#include <proto/locale.h>
#include <proto/intuition.h>
#include <intuition/classusr.h>
#include <utility/date.h>
#define USE_INLINE_STDARG
#include <proto/multimedia.h>
#undef USE_INLINE_STDARG

#define findobj(id, parent) (Object*)DoMethod(parent, MUIM_FindUData, id)
#define EmptyRectangle(weight) MUI_NewObject(MUIC_Rectangle, MUIA_Weight, weight, TAG_END)
#define _between(a,x,b) ((x)>=(a) && (x)<=(b))
#define _isinobject(x,y) (_between(_mleft(obj),(x),_mright(obj)) && _between(_mtop(obj),(y),_mbottom(obj)))

Object* StringGadget(ULONG id);
Object* StringLabel(STRPTR label, STRPTR preparse);
Object* GfxButton(ULONG id, STRPTR pic, UBYTE control);
Object* NormalButton(STRPTR label, UBYTE control, LONG objid, ULONG weight);
VOID Beep(VOID);
VOID ActLocalTimeToClockData(struct ClockData *cd);
VOID ActUTCTimeToClockData(struct ClockData *cd);
ULONG ActLocalTime2Amiga(VOID);
VOID Amiga2DateStamp(ULONG secs, struct DateStamp *ds);
ULONG DateStamp2Amiga(struct DateStamp *ds);
ULONG UTCToLocal(ULONG secs, struct Locale *locale);
ULONG LocalToUTC(ULONG secs, struct Locale *locale);
BOOL IsPictureFile(STRPTR path);
struct Picture *LoadPictureFile(STRPTR path);
struct Picture *LoadPictureMemory(APTR data, QUAD *length);
struct Picture *CopyPicture(struct Picture *src);
VOID FreePicture(struct Picture *pic);
STRPTR StrIStr(STRPTR txt, STRPTR tmp); /* brute force case insensitive strstr() */
UBYTE *LoadFile(STRPTR path, ULONG *size);
BOOL SaveFile(STRPTR path, APTR data, ULONG data_size);
STRPTR SystemToUtf8(STRPTR src);
STRPTR Utf8ToSystem(STRPTR src);
VOID *MemSet(VOID* ptr, LONG word, LONG size);
STRPTR StrNewPublic(STRPTR str);
STRPTR StrNewLen(STRPTR s, LONG len);
STRPTR URLEncode(STRPTR s);
VOID WriteEzxmlToFile(ezxml_t xml, BPTR fh);
VOID SetTimeFixValue(LONG fix_value);
BOOL CheckForOtherCopy(STRPTR portName, BOOL wake);
STRPTR *ExplodeString(STRPTR str, UBYTE delimiter, ULONG *entries);
STRPTR *ExplodeConstString(CONST_STRPTR str, UBYTE delimiter, ULONG *entries);

static inline ULONG xget(Object *obj, ULONG att)
{
	ULONG result;

	GetAttr(att, obj, &result);
	return result;
}

#endif /* __SUPPORT_H__ */
