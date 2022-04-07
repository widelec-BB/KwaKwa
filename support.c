/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <dos/dos.h>
#include <libvstring.h>
#include <classes/multimedia/video.h>
#include <classes/multimedia/streams.h>
#define USE_INLINE_STDARG
#include <proto/charsets.h>
#undef USE_INLINE_STDARG

#include "support.h"
#include "globaldefines.h"
#include "locale.h"

#include "kwakwa_api/defs.h"
#include "kwakwa_api/pictures.h"

extern struct Library *MultimediaBase;

Object* StringLabel(STRPTR label, STRPTR preparse)
{
	Object *obj = MUI_NewObject(MUIC_Text,
		MUIA_Unicode, TRUE,
		MUIA_FramePhantomHoriz, TRUE,
		MUIA_Frame, MUIV_Frame_String,
		MUIA_Text_PreParse, preparse,
		MUIA_Text_Contents, label,
		MUIA_HorizWeight, 0,
	TAG_END);

	return obj;
}

Object* StringGadget(ULONG id)
{
	Object *obj = MUI_NewObject(MUIC_String,
		MUIA_Unicode, TRUE,
		MUIA_UserData, id,
		MUIA_ObjectID, id,
		MUIA_Frame, MUIV_Frame_String,
		MUIA_Background, MUII_StringBack,
		MUIA_CycleChain, TRUE,
		MUIA_String_AdvanceOnCR, TRUE,
	TAG_END);

	return obj;
}

Object* GfxButton(ULONG id, STRPTR pic, UBYTE control)
{
	Object *obj = MUI_NewObject(MUIC_Group,
		MUIA_InputMode, MUIV_InputMode_RelVerify,
		MUIA_UserData, id,
		MUIA_ObjectID, id,
		MUIA_ControlChar, control,
		MUIA_Group_Child, MUI_NewObject(MUIC_Dtpic,
			MUIA_Dtpic_Name, pic,
		TAG_END),
	TAG_END);

	return obj;
}

Object* CheckBox(ULONG id, BOOL selected, STRPTR help)
{
	return MUI_NewObject(MUIC_Image,
		MUIA_Unicode, TRUE,
		MUIA_ObjectID, id,
		MUIA_UserData, id,
		MUIA_Image_Spec, "6:15",
		MUIA_ShowSelState, FALSE,
		MUIA_Selected, selected,
		MUIA_InputMode, MUIV_InputMode_Toggle,
		MUIA_CycleChain, TRUE,
		MUIA_ShortHelp, help,
	TAG_END);
}

Object* NormalButton(STRPTR label, UBYTE control, LONG objid, ULONG weight)
{
	Object *obj;

	obj = MUI_NewObject(MUIC_Text,
		MUIA_Unicode, TRUE,
		MUIA_Text_Contents, (ULONG)label,
		MUIA_Text_PreParse, "\33c",
		MUIA_Frame, MUIV_Frame_Button,
		MUIA_Background, MUII_ButtonBack,
		MUIA_Font, MUIV_Font_Button,
		MUIA_InputMode, MUIV_InputMode_RelVerify,
		MUIA_Text_HiChar, control,
		MUIA_ControlChar, control,
		MUIA_CycleChain, TRUE,
		MUIA_HorizWeight, weight,
		MUIA_UserData, objid,
	TAG_END);

	return obj;
}

VOID Beep(VOID)
{
	struct Screen *screen = LockPubScreen(NULL);

	if(screen)
	{
		DisplayBeep(screen);
		UnlockPubScreen(NULL, screen);
	}
}

VOID ActLocalTimeToClockData(struct ClockData *cd)
{
	struct DateStamp ds;
	ULONG secs;

	DateStamp(&ds);

	secs = DateStamp2Amiga(&ds);

	Amiga2Date(secs, cd);
}

VOID ActUTCTimeToClockData(struct ClockData *cd)
{
	struct DateStamp ds;
	ULONG secs;

	DateStamp(&ds);

	secs = DateStamp2Amiga(&ds);

	LocalToUTC(secs, NULL);

	Amiga2Date(secs, cd);
}

ULONG ActLocalTime2Amiga(VOID)
{
	struct DateStamp ds;

	DateStamp(&ds);

	return DateStamp2Amiga(&ds);
}

VOID Amiga2DateStamp(ULONG secs, struct DateStamp *ds)
{
	if(ds)
	{
		ds->ds_Days = secs / (24 * 60 * 60);
		secs -= ds->ds_Days * (24 * 60 * 60);

		ds->ds_Minute = secs / 60;
		secs -= ds->ds_Minute * 60;

		ds->ds_Tick = secs * TICKS_PER_SECOND;
	}
}

ULONG DateStamp2Amiga(struct DateStamp *ds)
{
	ULONG secs = 0;

	if(ds)
	{
		secs  = ds->ds_Days * 24 * 60 * 60;
		secs += ds->ds_Minute * 60;
		secs += ds->ds_Tick / TICKS_PER_SECOND;
	}

	return secs;
}

static ULONG gTimeFix = 0;

VOID SetTimeFixValue(LONG fix_value)
{
	gTimeFix = fix_value;
}

ULONG UTCToLocal(ULONG secs, struct Locale *locale)
{
	LONG diff = 0;

	if(locale)
	{
		diff = 60 * locale->loc_GMTOffset;
	}
	else if((locale = OpenLocale(NULL)))
	{
		diff = 60 * locale->loc_GMTOffset;
		CloseLocale(locale);
	}

	diff += gTimeFix;

	return secs - diff;
}

ULONG LocalToUTC(ULONG secs, struct Locale *locale)
{
	LONG diff = 0;

	if(locale)
	{
		diff = 60 * locale->loc_GMTOffset;
	}
	else if((locale = OpenLocale(NULL)))
	{
		diff = 60 * locale->loc_GMTOffset;
		CloseLocale(locale);
	}

	diff += gTimeFix;

	return secs + diff;
}

BOOL IsPictureFile(STRPTR path)
{
	Object *pic;

	pic = MediaNewObjectTags(
		MMA_StreamType, (IPTR)"file.stream",
		MMA_StreamName, (IPTR)path,
		MMA_MediaType, MMT_PICTURE,
		MMA_Decode, FALSE,
	TAG_END);

	if(pic)
	{
		DisposeObject(pic);
		return TRUE;
	}
	return FALSE;
}

struct Picture *LoadPictureFile(STRPTR path)
{
	struct Picture *result = NULL;
	Object *pic = NULL;

	pic = MediaNewObjectTags(
		MMA_StreamType, (ULONG)"file.stream",
		MMA_StreamName, (ULONG)path,
		MMA_MediaType, MMT_PICTURE,
	TAG_END);

	if(pic)
	{
		if((result = AllocMem(sizeof(struct Picture), MEMF_ANY)))
		{
			result->p_Height = MediaGetPort(pic, 0, MMA_Video_Height);
			result->p_Width = MediaGetPort(pic, 0, MMA_Video_Width);

			if((result->p_Data = (BYTE*)MediaAllocVec((result->p_Height * result->p_Width) << 2)))
			{
				DoMethod(pic, MMM_Pull, 0, (ULONG) result->p_Data, (result->p_Height * result->p_Width) << 2);
			}
		}
		DisposeObject(pic);
	}

	return result;
}

struct Picture *LoadPictureMemory(APTR data, QUAD *length)
{
	struct Picture *result = NULL;
	Object *pic = NULL;

	pic = MediaNewObjectTags(
		MMA_StreamType, (IPTR)"memory.stream",
		MMA_StreamHandle, (IPTR)data,
		MMA_StreamLength, (IPTR)length,
		MMA_MediaType, MMT_PICTURE,
	TAG_END);

	if(pic)
	{
		if((result = AllocMem(sizeof(struct Picture), MEMF_ANY)))
		{
			result->p_Height = MediaGetPort(pic, 0, MMA_Video_Height);
			result->p_Width = MediaGetPort(pic, 0, MMA_Video_Width);

			if((result->p_Data = (BYTE*)MediaAllocVec((result->p_Height * result->p_Width) << 2)))
				DoMethod(pic, MMM_Pull, 0, (ULONG) result->p_Data, (result->p_Height * result->p_Width) << 2);
		}
		DisposeObject(pic);
	}

	return result;
}

struct Picture *CopyPicture(struct Picture *src)
{
	struct Picture *result = NULL;

	if(src == NULL || src->p_Data == NULL)
		return NULL;

	if((result = AllocMem(sizeof(struct Picture), MEMF_ANY)))
	{
		result->p_Height = src->p_Height;
		result->p_Width = src->p_Width;

		if((result->p_Data = (BYTE*)MediaAllocVec((result->p_Height * result->p_Width) << 2)))
		{
			CopyMemQuick(src->p_Data, result->p_Data, (result->p_Height * result->p_Width) << 2);
		}
		else
		{
			FreeMem(result, sizeof(struct Picture));
			result = NULL; /* not longer valid */
		}
	}
	return result;
}

VOID FreePicture(struct Picture *pic)
{
	if(pic)
	{
		if(pic->p_Data)
			MediaFreeVec(pic->p_Data);

		FreeMem(pic, sizeof(struct Picture));
	}
}


STRPTR StrIStr(STRPTR txt, STRPTR tmp) /* brute force case insensitive strstr() */
{
	STRPTR temp = tmp;
	BYTE a, b;

	while(*txt != 0x00 && *temp != 0x00)
	{
		if(_between('A', *txt, 'Z'))
			a = *txt | 0x20;
		else
			a = *txt;
		if(_between('A', *temp, 'Z'))
			b = *temp | 0x20;
		else
			b = *temp;

		if(a != b)
		{
			txt -= temp - tmp;
			temp = tmp - 1 ;
		}
		txt++;
		temp++;
	}

	if(*temp == 0x00)
		return txt - (temp - tmp);
	else
		return NULL;
}

UBYTE* LoadFile(STRPTR path, ULONG *size)
{
	UBYTE *result = NULL;
	BPTR fh;

	if(size)
		*size = 0;

	if((fh = Open(path, MODE_OLDFILE)))
	{
		struct FileInfoBlock fib;

		if(ExamineFH(fh, &fib))
		{
			if((result = AllocVec(fib.fib_Size + 1, MEMF_ANY)))
			{
				if(size)
					*size = fib.fib_Size;

				FRead(fh, result, fib.fib_Size, 1);

				result[fib.fib_Size] = 0x00; /* always data will be ended with '\0' */
			}
		}
		Close(fh);
	}

	return result;
}

BOOL SaveFile(STRPTR path, APTR data, ULONG data_size)
{
	BOOL result = FALSE;
	BPTR fh;

	if((fh = Open(path, MODE_NEWFILE)))
	{
		if((FWrite(fh, data, data_size, 1)) == 1)
		{
			result = TRUE;
		}

		Close(fh);
	}

	return result;
}


STRPTR Utf8ToSystem(STRPTR src)
{
	LONG size;

	if(src)
	{
		if((size = GetByteSize(src, -1, MIBENUM_UTF_8, MIBENUM_SYSTEM)) != -1)
		{
			STRPTR result;

			if((result = AllocVec(size * sizeof(UBYTE), MEMF_ANY)))
			{
				if(ConvertTags(src, -1, result, size, MIBENUM_UTF_8, MIBENUM_SYSTEM, TAG_END) != -1)
					return result;

				StrFree(result);
				result = NULL;
			}
		}
	}

	return NULL;
}

STRPTR SystemToUtf8(STRPTR src)
{
	LONG size;

	if(src)
	{
		if((size = GetByteSize(src, -1, MIBENUM_SYSTEM, MIBENUM_UTF_8)) != -1)
		{
			STRPTR result;

			if((result = AllocVec(size * sizeof(UBYTE), MEMF_PUBLIC)))
			{
				if(ConvertTags(src, -1, result, size, MIBENUM_SYSTEM, MIBENUM_UTF_8, TAG_END) != -1)
					return result;

				StrFree(result);
				result = NULL;
			}
		}
	}

	return NULL;
}

VOID *MemSet(VOID* ptr, LONG word, LONG size)
{
	LONG i;

	for(i = 0; i < size; i++)
		((UBYTE*)ptr)[i] = (UBYTE)word;

	return ptr;
}

STRPTR StrNewPublic(STRPTR str)
{
	STRPTR n = NULL;

	if(str != NULL)
	{
		ULONG len = StrLen(str);

		if(len == 0) return NULL;

		if((n = AllocVec(len + sizeof(UBYTE), MEMF_PUBLIC)))
			StrCopy(str, n);
	}

	return n;
}

STRPTR StrNewLen(STRPTR s, LONG len)
{
	STRPTR result = NULL;

	if(s != NULL && len > 0)
	{
		if((result = AllocVec(len + 1, MEMF_PUBLIC)))
		{
			LONG i;

			for(i=0; i < len; i++)
				result[i] = s[i];

			result[len] = 0x00;
		}
	}

	return result;
}

static inline BOOL IsURLSafe(UBYTE ch)
{
	if('A' <= ch && ch <= 'Z')
		return TRUE;

	if('a' <= ch && ch <= 'z')
		return TRUE;

	if('0' <= ch && ch <= '9')
		return TRUE;

	if(ch == '-' || ch == '_' || ch == '.' || ch == '~' || ch == '/')
		return TRUE;

	return FALSE;
}

STRPTR URLEncode(STRPTR s)
{
	STRPTR result = NULL;
	ENTER();

	if(s)
	{
		ULONG len = 0;
		STRPTR t = s;

		while(*t)
		{
			if(IsURLSafe(*t))
				len++;
			else
				len += 3;

			t++;
		}

		len++; /* for '\0' */

		if((result = AllocVec(len, MEMF_ANY)))
		{
			STRPTR d = result;

			t = s;

			while(*t)
			{
				if(IsURLSafe(*t))
					*d++ = *t;
				else
				{
					FmtNPut(d, "%%%lX", 4, (ULONG)*t);
					d += 3;
				}
				t++;
			}
			*d = '\0';
		}
	}

	LEAVE();
	return result;
}

VOID WriteEzxmlToFile(ezxml_t xml, BPTR fh)
{
	if(xml && fh)
	{
		STRPTR xmltxt;

		/* ezxml_toxml() result HAVE TO be freed! */
		if((xmltxt = ezxml_toxml(xml)))
		{
			STRPTR x = xmltxt;
			ULONG tabs = 0;

			FPrintf(fh, "<?xml version=\"1.0\"?>\n");

			while(*x)
			{
				if(*x == '<' && *(x + 1) != '/')
					tabs++;
				else if(*x == '<' && *(x + 1) == '/')
					tabs--;

				if(*x == '>' && *(x + 1) == '<')
				{
					ULONG t;

					if(*(x + 2) == '/')
						tabs--;

					FPutC(fh, '>');
					FPutC(fh, '\n');

					for(t = 0; t < tabs; t++)
						FPutC(fh, '\t');
				}
				else
				{
					FPutC(fh, *x);
				}
				x++;
			}
			FPutC(fh, '\n');

			FreeVec(xmltxt);
		}
	}
}

BOOL CheckForOtherCopy(STRPTR portName, BOOL wake)
{
	struct MsgPort *port;

	Forbid();

	port = FindPort(portName);

	Permit();

	if(port && wake)
	{
		struct Library *RexxSysBase;

		if((RexxSysBase = OpenLibrary("rexxsyslib.library", 44)))
		{
			struct MsgPort *replyPort;

			if((replyPort = CreateMsgPort()))
			{
				struct RexxMsg *rexxMsg;

				if((rexxMsg = CreateRexxMsg(replyPort, NULL, NULL)))
				{
					rexxMsg->rm_Action = RXCOMM;
					rexxMsg->rm_Args[0] = CreateArgstring("WindowControl main show", 23); /* 23 == StrLen("WindowControl main show") */

					if(rexxMsg->rm_Args[0])
					{
						Forbid();

						port = FindPort(portName);

						if(port)
							PutMsg(port, (struct Message*)rexxMsg);

						Permit();

						if(port)
							WaitPort(replyPort);

						while(GetMsg(replyPort));

						DeleteArgstring(rexxMsg->rm_Args[0]);
					}

					DeleteRexxMsg(rexxMsg);
				}

				DeleteMsgPort(replyPort);
			}

			CloseLibrary(RexxSysBase);
		}
	}

	return port ? TRUE : FALSE;
}

STRPTR *ExplodeString(STRPTR str, UBYTE delimiter, ULONG *entries)
{
	STRPTR *res = NULL;
	ULONG len = 0, act = 0;

	if(str)
	{
		STRPTR t = str;

		while(*t == delimiter)
			t++;

		while(*t++)
		{
			if(*t == delimiter && t[1] != 0x00 && t[1] != delimiter)
				len++;
		}

		len++;

		res = AllocVec(sizeof(STRPTR) * len + 1, MEMF_ANY | MEMF_CLEAR);

		if(res)
		{
			t = str;

			while(*t++)
			{
				if(*t == delimiter && t[1] != delimiter)
				{
					STRPTR end = t;

					while(end[-1] == delimiter)
						end--;

					*end = 0x00;

					while(*str == delimiter)
						str++;

					if(*str)
						res[act++] = StrNew(str);

					str = ++t;
					t++;
				}
			}
			res[act] = StrNew(str);

			if(entries)
				*entries = len;
		}
	}

	return res;
}

STRPTR *ExplodeConstString(CONST_STRPTR str, UBYTE delimiter, ULONG *entries)
{
	STRPTR temp = StrNew((STRPTR)str);
	STRPTR *res = ExplodeString(temp, delimiter, entries);

	StrFree(temp);

	return res;
}

STRPTR GetStatusName(ULONG status)
{
	if(KWA_S_AVAIL(status))
		return GetString(MSG_GG_STATUS_AVAIL);
	if(KWA_S_BUSY(status))
		return GetString(MSG_GG_STATUS_AWAY);
	if(KWA_S_FFC(status))
		return GetString(MSG_GG_STATUS_FFC);
	if(KWA_S_DND(status))
		return GetString(MSG_GG_STATUS_DND);
	if(KWA_S_BLOCKED(status))
		return GetString(MSG_GG_STATUS_BLOCKED);
	if(KWA_S_INVISIBLE(status))
		return GetString(MSG_GG_STATUS_INVISIBLE);

	return GetString(MSG_GG_STATUS_UNAVAIL);
}

LONG MUI_Request_Unicode(Object *app, Object *win, STRPTR title, STRPTR gadgets, STRPTR format, ...)
{
	ULONG res = 0;
	STRPTR sys_title;
	va_list args;

	va_start(args, format);

	if((sys_title = Utf8ToSystem(title)))
	{
		STRPTR sys_gadgets;

		if((sys_gadgets = Utf8ToSystem(gadgets)))
		{
			STRPTR unicode_txt = VFmtNew(format, args);
			if(unicode_txt)
			{
				STRPTR sys_txt = Utf8ToSystem(unicode_txt);
				if(sys_txt)
				{
					res = MUI_RequestA(app, win, 0, sys_title, sys_gadgets, sys_txt, NULL);
					StrFree(sys_txt);
				}
				StrFree(unicode_txt);
			}
			StrFree(sys_gadgets);
		}
		StrFree(sys_title);
	}

	va_end(args);

	return res;
}
