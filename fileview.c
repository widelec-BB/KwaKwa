/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/cybergraphics.h>
#include <proto/asl.h>
#include <proto/wb.h>
#include <libvstring.h>

#include "globaldefines.h"
#include "locale.h"
#include "prefswindow.h"
#include "kwakwa_api/pictures.h"
#include "support.h"
#include "fileview.h"

struct MUI_CustomClass *FileViewClass;

static IPTR FileViewDispatcher(VOID);
const struct EmulLibEntry FileViewGate = {TRAP_LIB, 0, (VOID(*)(VOID))FileViewDispatcher};

struct FileViewData
{
	APTR   fvd_FileData;
	ULONG  fvd_FileDataSize;
	STRPTR fvd_FileName;
};


struct MUI_CustomClass *CreateFileViewClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct FileViewData), (APTR)&FileViewGate);
	FileViewClass = cl;
	return cl;
}


VOID DeleteFileViewClass(VOID)
{
	if(FileViewClass) MUI_DeleteCustomClass(FileViewClass);
}

static IPTR FileViewNew(Class *cl, Object *obj, struct opSet *msg)
{
	Object *txt, *save, *open;

	obj = (Object*)DoSuperNew(cl, obj,
		MUIA_Group_Horiz, TRUE,
		MUIA_Group_Child, (txt = MUI_NewObject(MUIC_Text,
		TAG_END)),
		MUIA_Group_Child, (save = MUI_NewObject(MUIC_Text,
			MUIA_Unicode, TRUE,
			MUIA_Text_Contents, (IPTR)GetString(MSG_FILEVIEW_BUTTON_SAVE),
			MUIA_Text_PreParse, (IPTR)"\33c",
			MUIA_Frame, MUIV_Frame_Button,
			MUIA_Background, MUII_ButtonBack,
			MUIA_Font, MUIV_Font_Button,
			MUIA_InputMode, MUIV_InputMode_RelVerify,
			MUIA_CycleChain, TRUE,
			MUIA_Weight, 0,
		TAG_END)),
		MUIA_Group_Child, (open = MUI_NewObject(MUIC_Text,
			MUIA_Unicode, TRUE,
			MUIA_Text_Contents, (IPTR)GetString(MSG_FILEVIEW_BUTTON_OPEN),
			MUIA_Text_PreParse, (IPTR)"\33c",
			MUIA_Frame, MUIV_Frame_Button,
			MUIA_Background, MUII_ButtonBack,
			MUIA_Font, MUIV_Font_Button,
			MUIA_InputMode, MUIV_InputMode_RelVerify,
			MUIA_CycleChain, TRUE,
			MUIA_Weight, 0,
		TAG_END)),
	TAG_MORE, (IPTR)msg->ops_AttrList);

	if(obj)
	{
		struct FileViewData *d = INST_DATA(cl, obj);
		struct TagItem *act_tag, *itr_tag = msg->ops_AttrList;
		APTR data = NULL;

		while((act_tag = NextTagItem(&itr_tag)))
		{
			switch(act_tag->ti_Tag)
			{
				case FVA_FileData:
					data = (APTR)act_tag->ti_Data; /* temporary store pointer to be sure we will know size before allocating memory */
				break;

				case FVA_FileDataSize:
					d->fvd_FileDataSize = act_tag->ti_Data;
				break;

				case FVA_TextContents:
					set(txt, MUIA_Text_Contents, act_tag->ti_Data);
				break;

				case FVA_TextPreparse:
					set(txt, MUIA_Text_PreParse, act_tag->ti_Data);
				break;

				case FVA_FileName:
					d->fvd_FileName = StrNew((STRPTR)act_tag->ti_Data);
				break;
			}
		}

		if(data && d->fvd_FileDataSize)
		{
			if((d->fvd_FileData = AllocMem(d->fvd_FileDataSize, MEMF_ANY)))
			{
				CopyMem(data, d->fvd_FileData, d->fvd_FileDataSize);

				DoMethod(save, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
				 FVM_SaveFile);
				DoMethod(open, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
				 FVM_OpenFile);

				return (IPTR)obj;
			}
		}
	}
	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}

static IPTR FileViewDispose(Class *cl, Object *obj, Msg msg)
{
	struct FileViewData *d = INST_DATA(cl, obj);

	if(d->fvd_FileData)
		FreeMem(d->fvd_FileData, d->fvd_FileDataSize);

	return DoSuperMethodA(cl, obj, msg);
}

static IPTR FileViewSaveFile(Class *cl, Object *obj)
{
	struct FileViewData *d = INST_DATA(cl, obj);
	BOOL result = FALSE;
	struct FileRequester *freq;

	if((freq = MUI_AllocAslRequestTags(ASL_FileRequest, TAG_END)))
	{
		STRPTR last_path = LoadFile(CACHE_DIR GUI_DIR "file_save_asl.cfg", NULL);

		if(MUI_AslRequestTags(freq,
			ASLFR_Window, _window(obj),
			ASLFR_TitleText, GetString(MSG_FILEVIEW_ASL_TITLE),
			ASLFR_PositiveText, GetString(MSG_FILEVIEW_ASL_SAVEAS_POSITIVE),
			ASLFR_InitialPattern, "#?",
			ASLFR_DoPatterns, TRUE,
			ASLFR_RejectIcons, TRUE,
			ASLFR_PrivateIDCMP, TRUE,
			ASLFR_DoSaveMode, TRUE,
			ASLFR_InitialDrawer, last_path ? last_path : (STRPTR)"",
		TAG_END))
		{
			UBYTE location[500];

			FmtNPut(location, CACHE_DIR GUI_DIR "file_save_asl.cfg", sizeof(location));
			SaveFile(location, freq->fr_Drawer, StrLen(freq->fr_Drawer));

			StrNCopy(freq->fr_Drawer, (STRPTR)location, sizeof(location));
			AddPart((STRPTR)location, freq->fr_File, sizeof(location));

			if(SaveFile(location, d->fvd_FileData, d->fvd_FileDataSize))
				result = TRUE;

			if(!result)
				MUI_Request_Unicode(_app(obj), obj, APP_NAME, GetString(MSG_SEND_PICTURE_FAIL_BUTTONS), GetString(MSG_SEND_PICTURE_FAIL_MSG));
		}
		MUI_FreeAslRequest(freq);
	}

	return(IPTR)result;
}

static IPTR FileViewOpenFile(Class *cl, Object *obj)
{
	struct FileViewData *d = INST_DATA(cl, obj);
	BOOL result = FALSE;
	ULONG id = GetUniqueID();

	if(d->fvd_FileName)
	{
		STRPTR path;

		if((path = FmtNew("T:%ls_%x", d->fvd_FileName, id)))
		{
			if(SaveFile(path, d->fvd_FileData, d->fvd_FileDataSize))
				result = OpenWorkbenchObjectA(path, TAG_END);

			StrFree(path);
		}
	}

	if(!result)
	{
		UBYTE buffer[32];

		FmtNPut(buffer, "T:.kwakwa_%x", sizeof(buffer), id);

		if(SaveFile(buffer, d->fvd_FileData, d->fvd_FileDataSize))
			result = OpenWorkbenchObjectA(buffer, TAG_END);
	}

	return result;
}

static IPTR FileViewDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW: return(FileViewNew(cl, obj, (struct opSet*)msg));
		case OM_DISPOSE: return(FileViewDispose(cl, obj, msg));
		case FVM_SaveFile: return(FileViewSaveFile(cl, obj));
		case FVM_OpenFile: return(FileViewOpenFile(cl, obj));
		default: return(DoSuperMethodA(cl, obj, msg));
	}
}
