/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <libvstring.h>
#include <proto/cybergraphics.h>

#include "libnsgif.h"
#include "kwakwa_api/pictures.h"

#include "globaldefines.h"
#include "support.h"
#include "virtualtext.h"
#include "emoticon.h"


/********************** libnsgif callbacks ******************************/
static void *bitmap_create_default_callback(int width, int height)
{
	return AllocVec((width * height) << 2, MEMF_ANY | MEMF_CLEAR);
}

static void bitmap_set_opaque_default_callback(void *bitmap, bool opaque)
{
	(void)opaque;  /* unused */
	(void)bitmap;  /* unused */
}


static bool bitmap_test_opaque_default_callback(void *bitmap)
{
	(void)bitmap;	/* unused */
	return false;	/* unused */
}


static unsigned char *bitmap_get_buffer_default_callback(void *bitmap)
{
	(void)bitmap;	/* unused */
	return bitmap;	/* unused */
}


static void bitmap_destroy_default_callback(void *bitmap)
{
	if(bitmap)
		FreeVec(bitmap);
}

static void bitmap_modified_default_callback(void *bitmap)
{
	if(bitmap)
		return;
}

static gif_bitmap_callback_vt Bitmap_default_callbacks =
{
	bitmap_create_default_callback,
	bitmap_destroy_default_callback,
	bitmap_get_buffer_default_callback,
	bitmap_set_opaque_default_callback,
	bitmap_test_opaque_default_callback,
	bitmap_modified_default_callback
};

#define GIF_DEFAULT_CALLBACKS &Bitmap_default_callbacks

/*************************************************************************/


struct MUI_CustomClass *EmoticonClass;

static IPTR EmoticonDispatcher(VOID);
const struct EmulLibEntry EmoticonGate = {TRAP_LIB, 0, (VOID(*)(VOID))EmoticonDispatcher};

#define EMOTICON_MODE_GRAPHIC		0x00
#define EMOTICON_MODE_TEXT			0x01

struct EmoticonData
{
	UBYTE emod_PicturePath[255];				/* path to picture file */
	struct gif_animation *emod_Animation; 	/* animation structure for GIF files */
	ULONG emod_ActAnimFrame;					/* next frame to play */
	STRPTR emod_ShortHelp;						/* MUI don't copy short help to private buffer, so we do this here */
	ULONG emod_GlobalAlpha;						/* global alpha value for WritePixelArrayAlpha() */
	UBYTE emod_Mode;								/* actual mode (graphic or text) */
	STRPTR emod_Preparse;						/* MUI preparse for text mode */
	ULONG emod_PrevClickSec;					/* for DoubleClick() */
	ULONG emod_PrevClickMic;					/* for DoubleClick() */
	BOOL emod_isGif;								/* TRUE if emoticon is in GIF */
	struct Picture *emod_Picture;				/* for formats other than GIF */

	struct MUI_InputHandlerNode hnode;		/* input handler for time stuff */
	struct MUI_EventHandlerNode handler;	/* mouse input handler */
};


struct MUI_CustomClass *CreateEmoticonClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Area, NULL, sizeof(struct EmoticonData), (APTR)&EmoticonGate);
	EmoticonClass = cl;
	return cl;
}


VOID DeleteEmoticonClass(VOID)
{
	if (EmoticonClass) MUI_DeleteCustomClass(EmoticonClass);
}

static BOOL isGifFile(STRPTR path)
{
	BOOL result = FALSE;
	Object *pic = NULL;

	pic = MediaNewObjectTags(
		MMA_StreamType, (IPTR)"file.stream",
		MMA_StreamName, (IPTR)path,
		MMA_MediaType, MMT_PICTURE,
		MMA_Decode, FALSE,
	TAG_END);

	if(pic)
	{
		if(StrIStr((STRPTR)xget(pic, MMA_DataFormat), "gif"))
			result = TRUE;

		DisposeObject(pic);
	}

	return result;
}

static struct gif_animation* LoadGif(STRPTR path)
{
	struct gif_animation *result = NULL;
	ULONG size;
	UBYTE *file_data;

	if((file_data = LoadFile(path, &size)))
	{
		if((result = AllocMem(sizeof(struct gif_animation), MEMF_ANY | MEMF_CLEAR)))
		{
			gif_result code;
			gif_create(result, GIF_DEFAULT_CALLBACKS);

			do
			{
				code = gif_initialise(result, size, file_data);
				if(code != GIF_OK && code != GIF_WORKING)
				{
					FreeMem(result, sizeof(struct gif_animation));
					result = NULL;
					break;
				}
			}while(code != GIF_OK);
		}
	}

	return result;
}

static VOID emoPathCopy(STRPTR src, STRPTR desc, ULONG desc_len)
{
	ULONG i, j;
	static BYTE map[][2] =
	{
		{'ê', 'e'},
		{'Ê', 'e'},
		{'ó', 'o'},
		{'Ó', 'O'},
		{'±', 'a'},
		{'¡', 'A'},
		{'¶', 's'},
		{'¦', 'S'},
		{'³', 'l'},
		{'£', 'L'},
		{'¿', 'z'},
		{'¯', 'Z'},
		{'¼', 'z'},
		{'¬', 'Z'},
		{'æ', 'c'},
		{'Æ', 'C'},
		{'ñ', 'n'},
		{'Ñ', 'N'},
	};

	for(i = 0; i < desc_len - 1 && src[i]; i++)
	{
		desc[i] = src[i];

		for(j = 0; j < sizeof(map) / sizeof(*map); j++)
		{
			if((map[j][0] & 0xFF) == (src[i] & 0xFF))
			{
				desc[i] = map[j][1];
				break;
			}
		}
	}

	desc[i] = '\0';
}

static IPTR EmoticonNew(Class *cl, Object *obj, struct opSet *msg)
{
	obj = (Object*)DoSuperNew((ULONG)cl, (ULONG)obj,
		MUIA_DoubleBuffer, FALSE, /* virtual text object has own double buffer, so we can't use it here (MUI bug?) */
	TAG_MORE, (ULONG)msg->ops_AttrList);

	if(obj)
	{
		struct EmoticonData *d = INST_DATA(cl, obj);
		struct TagItem *act_tag, *itr_tag = msg->ops_AttrList;
		ULONG alpha = 0xFFFFFFFF;

		while((act_tag = NextTagItem(&itr_tag)))
		{
			switch(act_tag->ti_Tag)
			{
				case EMOA_PicturePath:
					emoPathCopy((STRPTR)act_tag->ti_Data, d->emod_PicturePath, sizeof(d->emod_PicturePath));
				break;

				case EMOA_GlobalAlpha:
					alpha = act_tag->ti_Data;
				break;

				case EMOA_TextPreparse:
					d->emod_Preparse = (STRPTR)act_tag->ti_Data;
				break;

				case MUIA_ShortHelp:
					d->emod_ShortHelp = StrNew((STRPTR)act_tag->ti_Data); /* MUI don't copy short help to private buffer, so we do this here */
				break;
			}
		}

		d->emod_GlobalAlpha = alpha;
		d->emod_Mode = EMOTICON_MODE_GRAPHIC;

		set(obj, MUIA_ShortHelp, d->emod_ShortHelp);

		if(DoMethod(obj, EMOM_ReloadPicture))
			return (IPTR)obj;
	}
	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}

static IPTR EmoticonDispose(Class *cl, Object *obj, Msg msg)
{
	struct EmoticonData *d = INST_DATA(cl, obj);

	if(d->emod_Picture)
		FreePicture(d->emod_Picture);

	if(d->emod_Animation)
	{
		FreeVec(d->emod_Animation->gif_data);
		gif_finalise(d->emod_Animation);
		FreeMem(d->emod_Animation, sizeof(struct gif_animation));
	}

	if(d->emod_ShortHelp)
		StrFree(d->emod_ShortHelp);

	return DoSuperMethodA(cl, obj, msg);
}

static IPTR EmoticonGet(Class *cl, Object *obj, struct opGet *msg)
{
	struct EmoticonData *d = INST_DATA(cl, obj);
	int rv = FALSE;

	switch (msg->opg_AttrID)
	{
		case EMOA_PicturePath:
			*msg->opg_Storage = (ULONG) d->emod_PicturePath;
		return TRUE;

		case MUIA_Text_Contents:
			*msg->opg_Storage = (ULONG) d->emod_ShortHelp;
		return TRUE;

		case EMOA_GlobalAlpha:
			*msg->opg_Storage = (ULONG) d->emod_GlobalAlpha;
		return TRUE;

		default: rv = (DoSuperMethodA(cl, obj, (Msg)msg));
	}

	return rv;
}

static IPTR EmoticonSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct EmoticonData *d = INST_DATA(cl, obj);
	int tagcount = 0;
	struct TagItem *tag = 0, *tagptr = msg->ops_AttrList;

	while((tag = NextTagItem(&tagptr)))
	{
		switch(tag->ti_Tag)
		{
			case EMOA_GlobalAlpha:
				d->emod_GlobalAlpha = tag->ti_Data;
			break;
		}
	}

	tagcount += DoSuperMethodA(cl, obj, (Msg)msg);
	return tagcount;
}

static IPTR EmoticonAskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
	struct EmoticonData *d = INST_DATA(cl, obj);
	IPTR result = DoSuperMethodA(cl, obj, msg);
	ULONG w = d->emod_Animation->width;
	ULONG h = d->emod_Animation->height;

	if(d->emod_Mode == EMOTICON_MODE_GRAPHIC)
	{
		if(d->emod_isGif)
		{
			w = d->emod_Animation->width;
			h = d->emod_Animation->height;
		}
		else
		{
			w = d->emod_Picture->p_Width;
			h = d->emod_Picture->p_Height;
		}
	}
	else if(d->emod_Mode == EMOTICON_MODE_TEXT)
	{
		ULONG res = DoMethod(obj, MUIM_TextDim, (IPTR)d->emod_ShortHelp, StrLen(d->emod_ShortHelp), (IPTR)d->emod_Preparse, 0);
		w = res & 0xffff;
		h = res >> 16;
	}

	msg->MinMaxInfo->MinWidth 	+= w;
	msg->MinMaxInfo->MinHeight += h;
	msg->MinMaxInfo->DefWidth 	+= w;
	msg->MinMaxInfo->DefHeight += h;
	msg->MinMaxInfo->MaxWidth 	+= w;
	msg->MinMaxInfo->MaxHeight += h;

	return result;
}

static IPTR EmoticonDraw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
	struct EmoticonData *d = INST_DATA(cl, obj);
	IPTR result = DoSuperMethodA(cl, obj, msg);

	if(msg->flags & MADF_DRAWOBJECT)
	{
		if(d->emod_Mode == EMOTICON_MODE_GRAPHIC)
		{
			if(d->emod_isGif)
			{
				gif_result code;

				if((code = gif_decode_frame(d->emod_Animation, d->emod_ActAnimFrame)) == GIF_OK)
				{
					WritePixelArrayAlpha(d->emod_Animation->frame_image, 0, 0, d->emod_Animation->width << 2, _rp(obj), _mleft(obj),
					 _mtop(obj), _mwidth(obj), _mheight(obj), d->emod_GlobalAlpha);
				}
			}
			else
			{
				WritePixelArrayAlpha(d->emod_Picture->p_Data, 0, 0, d->emod_Picture->p_Width << 2, _rp(obj), _left(obj),
				 _top(obj), _width(obj), _height(obj), d->emod_GlobalAlpha);
			}
		}
		else if(d->emod_Mode == EMOTICON_MODE_TEXT)
		{
			DoMethod(obj, MUIM_Text, _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj),
			 (IPTR)d->emod_ShortHelp, StrLen(d->emod_ShortHelp), (IPTR)d->emod_Preparse, 0x00);
		}
	}

	return result;
}

static IPTR EmoticonSetup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
	struct EmoticonData *d = INST_DATA(cl, obj);
	IPTR result = DoSuperMethodA(cl, obj, msg);

	if(d->emod_isGif && d->emod_Animation->frame_count > 1)
	{
		d->hnode.ihn_Object = obj;
		d->hnode.ihn_Millis = 800 / ((d->emod_Animation->frame_count) > 6 ? 6 : (d->emod_Animation->frame_count));
		d->hnode.ihn_Flags =  MUIIHNF_TIMER;
		d->hnode.ihn_Method = EMOM_DrawNextFrame;

		DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR)&d->hnode);
	}

	d->handler.ehn_Class = cl;
	d->handler.ehn_Object = obj;
	d->handler.ehn_Events = IDCMP_MOUSEBUTTONS;

	DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&d->handler);

	return result;
}

static IPTR EmoticonCleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
	struct EmoticonData *d = INST_DATA(cl, obj);

	if(d->emod_isGif && d->emod_Animation->frame_count > 1)
		DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR)&d->hnode);

	DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&d->handler);

	return (DoSuperMethodA(cl, obj, msg));
}

static IPTR EmoticonDrawNextFrame(Class *cl, Object *obj)
{
	struct EmoticonData *d = INST_DATA(cl, obj);

	if(!d->emod_isGif)
		return(IPTR)0;

	if(d->emod_ActAnimFrame == d->emod_Animation->frame_count)
		d->emod_ActAnimFrame = 0;

	MUI_Redraw(obj, MADF_DRAWOBJECT);

	d->emod_ActAnimFrame++;

	return (IPTR)1;
}

static IPTR EmoticonHandleEvent(Class *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
	struct EmoticonData *d = INST_DATA(cl, obj);
	struct IntuiMessage *imsg = msg->imsg;
	IPTR result = 0;

	if(imsg)
	{
		if(imsg->Class == IDCMP_MOUSEBUTTONS)
		{
			if(_isinobject(imsg->MouseX, imsg->MouseY))
			{
				if(imsg->Code == IECODE_LBUTTON)
				{
					ULONG cur_click_sec, cur_click_mic;

					CurrentTime(&cur_click_sec, &cur_click_mic);

					if(DoubleClick(d->emod_PrevClickSec, d->emod_PrevClickMic, cur_click_sec, cur_click_mic))
					{
						DoMethod(obj, EMOM_DoubleClick);
						result |= MUI_EventHandlerRC_Eat;
					}
					else
					{
						d->emod_PrevClickSec = cur_click_sec;
						d->emod_PrevClickMic = cur_click_mic;
					}
				}
			}
		}
	}

	return result;
}

static IPTR EmoticonDoubleClick(Class *cl, Object *obj)
{
	struct EmoticonData *d = INST_DATA(cl, obj);

	if(d->emod_Mode == EMOTICON_MODE_TEXT)
		d->emod_Mode = EMOTICON_MODE_GRAPHIC;
	else
		d->emod_Mode = EMOTICON_MODE_TEXT;

	DoMethod((Object*)xget(obj, MUIA_Parent), VTM_Relayout);

	return 0;
}

static IPTR EmoticonReloadPicture(Class *cl, Object *obj)
{
	struct EmoticonData *d = INST_DATA(cl, obj);

	if(d->emod_PicturePath && *d->emod_PicturePath)
	{
		if(isGifFile(d->emod_PicturePath))
		{
			d->emod_isGif = TRUE;

			if((d->emod_Animation = LoadGif(d->emod_PicturePath)))
				return (IPTR)TRUE;
		}
		else
		{
			d->emod_isGif = FALSE;

			if((d->emod_Picture = LoadPictureFile(d->emod_PicturePath)))
				return (IPTR)TRUE;
		}
	}

	return (IPTR)FALSE;
}

static IPTR EmoticonDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch(msg->MethodID)
	{
		case OM_NEW: return (EmoticonNew(cl, obj, (struct opSet*)msg));
		case OM_DISPOSE: return (EmoticonDispose(cl, obj, msg));
		case OM_GET: return (EmoticonGet(cl, obj, (struct opGet*)msg));
		case OM_SET: return (EmoticonSet(cl, obj, (struct opSet*)msg));

		case MUIM_AskMinMax: return(EmoticonAskMinMax(cl, obj, (struct MUIP_AskMinMax*)msg));
		case MUIM_Draw: return(EmoticonDraw(cl, obj, (struct MUIP_Draw*)msg));
		case MUIM_Setup: return(EmoticonSetup(cl, obj, (struct MUIP_Setup*)msg));
		case MUIM_Cleanup: return(EmoticonCleanup(cl, obj, (struct MUIP_Cleanup*)msg));
		case MUIM_HandleEvent: return(EmoticonHandleEvent(cl, obj, (struct MUIP_HandleEvent *)msg));

		case EMOM_DrawNextFrame: return(EmoticonDrawNextFrame(cl, obj));
		case EMOM_DoubleClick: return(EmoticonDoubleClick(cl, obj));
		case EMOM_ReloadPicture: return(EmoticonReloadPicture(cl, obj));

		default: return (DoSuperMethodA(cl, obj, msg));
	}
}
