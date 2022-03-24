/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/locale.h>
#include <devices/rawkeycodes.h>
#include <mui/Hyperlink_mcc.h>
#include <libvstring.h>

#include "kwakwa_api/pictures.h"
#include "globaldefines.h"
#include "locale.h"
#include "virtualtext.h"
#include "prefswindow.h"
#include "application.h"
#include "talktab.h"
#include "emoticon.h"
#include "pictureview.h"
#include "fileview.h"
#include "lexer.h"
#include "emoticonstab.h"

#include "support.h"

struct MUI_CustomClass *VirtualTextClass;
static IPTR VirtualTextDispatcher(void);
const struct EmulLibEntry VirtualTextGate = {TRAP_LIB, 0, (void(*)(void))VirtualTextDispatcher};

static IPTR Layout(VOID);

struct EmulLibEntry LayoutGate = {TRAP_LIB, 0, (VOID(*)(VOID))Layout};
struct Hook LayoutHook = {{0, 0}, (HOOKFUNC)&LayoutGate, 0, 0};

/* and once again we will hack MUI internals... beacuse "M" in MUI stands for "Magic"! ;-) */
struct MUI_RGBcolor
{
	ULONG red;
	ULONG green;
	ULONG blue;
};


/* bit mask for MUIA_UserData for objects inserted to VitualText */
/* objects attrs */
#define VTG_FULL_WIDTH              (1 << 10) /* will automatically go to next line after insert that */
#define VTG_EMOTICON_MESSAGE_OLD    (1 << 11) /* emoticon from old message, for refreshing alpha value after prefs change */

/* objects types */
#define VTG_NORMAL_OBJECT        (0)
#define VTG_TITLE_OBJECT         (1 << 0)
#define VTG_SYSTEM_MSG_OBJECT    (1 << 1)
#define VTG_EMOTICON_OBJECT      (1 << 2)
#define VTG_PICTUREVIEW_OBJECT   (1 << 3)


/* macros for creating objects */
#define normal_text(str, pre) MUI_NewObject(MUIC_Text, MUIA_UserData, VTG_NORMAL_OBJECT, MUIA_Frame, MUIV_Frame_None, MUIA_Weight, 0, \
MUIA_Text_PreParse, pre, MUIA_Text_Contents, str, TAG_END)

struct VTP_AddMessageText {ULONG MethodID; STRPTR message; ULONG type; ULONG flags;};
struct VTP_AddMessage {ULONG MethodID; STRPTR message; ULONG flags;};
struct VTP_AddPlainText {ULONG MethodID; STRPTR text; ULONG type;};
struct VTP_AddSystemMessage {ULONG MethodID; STRPTR message; ULONG timestamp; ULONG flags;};
struct VTP_SelectStart {ULONG MethodID; ULONG mouse_x; ULONG mouse_y;};
struct VTP_SelectStop {ULONG MethodID; LONG mouse_x; LONG mouse_y;};
struct VTP_AddMessageHeadLine {ULONG MethodID; STRPTR sender; ULONG timestamp; ULONG flags;};
struct VTP_AddPicture {ULONG MethodID; APTR picture; ULONG picture_size; ULONG flags;};
struct VTP_AddFileView {ULONG MethodID; APTR data; ULONG data_size; STRPTR txt_contents; ULONG flags;};
struct VTP_CreateEmoticon {ULONG MethodID; STRPTR word; STRPTR path;};
struct VTP_FormatTime {ULONG MethodID; ULONG timestamp;};
struct VTP_Find {ULONG MethodID; STRPTR text;};

struct VirtualTextData
{
	UBYTE myPreparse[25], friendPreparse[25], preparse_header_l[25], preparse_header_r[25], preparse_system[25];
	UBYTE selectBackground[30], myPreparseOld[30], friendPreparseOld[30];
	UBYTE preparse_header_l_old[30], preparse_header_r_old[30], preparse_system_old[30];
	UBYTE timeBuffer[255];
	BOOL preparsesDone;
	ULONG height, width; /* object size */
	ULONG history_alpha;
	BOOL show_end;
	BOOL scroll_on_resize;
	BOOL setup_done;

	struct MUI_EventHandlerNode ehn;
	Object *mark_start, *mark_stop;

	struct Locale *locale;
};

struct MUI_CustomClass *CreateVirtualTextClass(void)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Virtgroup, NULL, sizeof(struct VirtualTextData), (APTR)&VirtualTextGate);
	VirtualTextClass = cl;
	return cl;
}

void DeleteVirtualTextClass(void)
{
	if (VirtualTextClass) MUI_DeleteCustomClass(VirtualTextClass);
}

static IPTR VirtualTextNew(Class *cl, Object *obj, struct opSet *msg)
{
	obj = DoSuperNew(cl, obj,
		MUIA_Group_LayoutHook, &LayoutHook,
		MUIA_DoubleBuffer, TRUE,
	TAG_MORE, msg->ops_AttrList);

	if(obj)
	{
		struct VirtualTextData *d = INST_DATA(cl, obj);

		if((d->locale = OpenLocale(NULL)))
		{
			d->preparsesDone = FALSE;
			d->setup_done = FALSE;
			d->scroll_on_resize = (BOOL)GetTagData(VTA_ScrollOnResize, (ULONG)TRUE, msg->ops_AttrList);

			DoMethod(obj, VTM_ApplyPrefs);
			prefs_changed(obj, VTM_ApplyPrefs);

			return (IPTR)obj;
		}
	}
	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}

static IPTR VirtualTextDispose(Class *cl, Object *obj, Msg msg)
{
	struct VirtualTextData *d = INST_DATA(cl, obj);

	if(d->locale)
		CloseLocale(d->locale);

	return DoSuperMethodA(cl, obj, msg);
}

static IPTR VirtualTextGet(Class *cl, Object *obj, struct opGet *msg)
{
	struct VirtualTextData *d = INST_DATA(cl, obj);
	int rv = FALSE;

	switch(msg->opg_AttrID)
	{
		case VTA_Height:
			*msg->opg_Storage = d->height;
		return TRUE;

		case VTA_Width:
			*msg->opg_Storage = d->width;
		return TRUE;

		default: rv = (DoSuperMethodA(cl, obj, (Msg)msg));
	}

	return rv;
}

static IPTR VirtualTextSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct VirtualTextData *d = INST_DATA(cl, obj);
	int tagcount = 0;
	struct TagItem *tag = 0, *tagptr = msg->ops_AttrList;

	while((tag = NextTagItem(&tagptr)))
	{
		switch(tag->ti_Tag)
		{
			case VTA_Height:
				d->height = tag->ti_Data;
				tagcount++;
			break;

			case VTA_Width:
				d->width = tag->ti_Data;
				tagcount++;
			break;
		}
	}

	tagcount += DoSuperMethodA(cl, obj, (Msg)msg);
	return tagcount;
}

static IPTR VirtualTextCreatePreparses(Class *cl, Object *obj)
{
	struct VirtualTextData *d = INST_DATA(cl, obj);
	struct MUI_PenSpec *pen_spec;
	LONG myPen, friendPen, headerPen, systemPen;
	LONG myColor, friendColor, headerColor, systemColor;
	struct MUI_RGBcolor *mark_rgb, *my_rgb_old, *friend_rgb_old, *systemmsg_old, *headline_old;
	UBYTE alpha;

	pen_spec = (struct MUI_PenSpec*)xget(prefs_object(USD_PREFS_ML_MYCOLOR_POPPEN), MUIA_Pendisplay_Spec);
	myPen = MUI_ObtainPen(muiRenderInfo(obj), pen_spec, 0);
	myColor = MUIPEN(myPen);

	pen_spec = (struct MUI_PenSpec*)xget(prefs_object(USD_PREFS_ML_FRIENDCOLOR_POPPEN), MUIA_Pendisplay_Spec);
	friendPen = MUI_ObtainPen(muiRenderInfo(obj), pen_spec, 0);
	friendColor = MUIPEN(friendPen);

	pen_spec = (struct MUI_PenSpec*)xget(prefs_object(USD_PREFS_TW_HEADLINE_COLOR), MUIA_Pendisplay_Spec);
	headerPen = MUI_ObtainPen(muiRenderInfo(obj), pen_spec, 0);
	headerColor = MUIPEN(headerPen);

	pen_spec = (struct MUI_PenSpec*)xget(prefs_object(USD_PREFS_TW_SYSTEMMSG_COLOR), MUIA_Pendisplay_Spec);
	systemPen = MUI_ObtainPen(muiRenderInfo(obj), pen_spec, 0);
	systemColor = MUIPEN(systemPen);

	MUI_ReleasePen(muiRenderInfo(obj), myPen);
	MUI_ReleasePen(muiRenderInfo(obj), friendPen);
	MUI_ReleasePen(muiRenderInfo(obj), headerPen);
	MUI_ReleasePen(muiRenderInfo(obj), systemPen);

	mark_rgb = (struct MUI_RGBcolor*)xget(prefs_object(USD_PREFS_TW_SELECTION_COLOR), MUIA_Pendisplay_RGBcolor);
	my_rgb_old = (struct MUI_RGBcolor*)xget(prefs_object(USD_PREFS_ML_MYCOLOR_OLD_POPPEN), MUIA_Pendisplay_RGBcolor);
	friend_rgb_old = (struct MUI_RGBcolor*)xget(prefs_object(USD_PREFS_ML_FRIENDCOLOR_OLD_POPPEN), MUIA_Pendisplay_RGBcolor);
	systemmsg_old = (struct MUI_RGBcolor*)xget(prefs_object(USD_PREFS_TW_SYSTEMMSG_OLD_COLOR), MUIA_Pendisplay_RGBcolor);
	headline_old = (struct MUI_RGBcolor*)xget(prefs_object(USD_PREFS_TW_HEADLINE_OLD_COLOR), MUIA_Pendisplay_RGBcolor);

	FmtNPut((STRPTR)d->myPreparse, "\033P[%ld]", sizeof(d->myPreparse), myColor);
	FmtNPut((STRPTR)d->friendPreparse, "\033P[%ld]", sizeof(d->friendPreparse), friendColor);

	alpha = (((DOUBLE)(100 - xget(prefs_object(USD_PREFS_OLD_MESSAGES_TRANSPARENCY), MUIA_Slider_Level))) / 100.) * 0xFF;

	FmtNPut((STRPTR)d->myPreparseOld, "\033P[%02X%02X%02X%02X]", sizeof(d->myPreparseOld), alpha, (UBYTE)(my_rgb_old->red >> 24), (UBYTE)(my_rgb_old->green >> 24), (UBYTE)(my_rgb_old->blue >> 24));
	FmtNPut((STRPTR)d->friendPreparseOld, "\033P[%02X%02X%02X%02X]", sizeof(d->friendPreparseOld), alpha, (UBYTE)(friend_rgb_old->red >> 24), (UBYTE)(friend_rgb_old->green >> 24), (UBYTE)(friend_rgb_old->blue >> 24));
	FmtNPut((STRPTR)d->preparse_header_l_old, "\033P[%02X%02X%02X%02X]", sizeof(d->preparse_header_l_old), alpha, (UBYTE)(headline_old->red >> 24), (UBYTE)(headline_old->green >> 24), (UBYTE)(headline_old->blue >> 24));
	FmtNPut((STRPTR)d->preparse_header_r_old, "\033P[%02X%02X%02X%02X]", sizeof(d->preparse_header_r_old), alpha, (UBYTE)(headline_old->red >> 24), (UBYTE)(headline_old->green >> 24), (UBYTE)(headline_old->blue >> 24));
	FmtNPut((STRPTR)d->preparse_system_old, "\033P[%02X%02X%02X%02X]", sizeof(d->preparse_system_old), alpha, (UBYTE)(systemmsg_old->red >> 24), (UBYTE)(systemmsg_old->green >> 24), (UBYTE)(systemmsg_old->blue >> 24));

	FmtNPut((STRPTR)d->preparse_header_l, "\033P[%ld]", sizeof(d->preparse_header_l), headerColor);
	FmtNPut((STRPTR)d->preparse_header_r, "\033P[%ld]", sizeof(d->preparse_header_r), headerColor);
	FmtNPut((STRPTR)d->preparse_system, "\033P[%ld]", sizeof(d->preparse_system), systemColor);
	FmtNPut((STRPTR)d->selectBackground, "2:%08X,%08X,%08X", sizeof(d->selectBackground), mark_rgb->red, mark_rgb->green, mark_rgb->blue);

	StrCat("\33l", (STRPTR)d->preparse_header_l_old);
	StrCat("\33r", (STRPTR)d->preparse_header_r_old);

	StrCat("\33l", (STRPTR)d->preparse_header_l);
	StrCat("\33r", (STRPTR)d->preparse_header_r);

	if((BOOL)xget(prefs_object(USD_PREFS_TW_HEADLINE_BOLD), MUIA_Selected))
	{
		StrCat("\33b", (STRPTR)d->preparse_header_l_old);
		StrCat("\33b", (STRPTR)d->preparse_header_r_old);

		StrCat("\33b", (STRPTR)d->preparse_header_l);
		StrCat("\33b", (STRPTR)d->preparse_header_r);
	}

	if((BOOL)xget(prefs_object(USD_PREFS_TW_HEADLINE_ITALIC), MUIA_Selected))
	{
		StrCat("\33i", (STRPTR)d->preparse_header_l_old);
		StrCat("\33i", (STRPTR)d->preparse_header_r_old);

		StrCat("\33i", (STRPTR)d->preparse_header_l);
		StrCat("\33i", (STRPTR)d->preparse_header_r);
	}

	if((BOOL)xget(prefs_object(USD_PREFS_TW_SYSTEMMSG_BOLD), MUIA_Selected))
	{
		StrCat("\33b", (STRPTR)d->preparse_system_old);
		StrCat("\33b", (STRPTR)d->preparse_system);
	}

	if((BOOL)xget(prefs_object(USD_PREFS_TW_SYSTEMMSG_ITALIC), MUIA_Selected))
	{
		StrCat("\33i", (STRPTR)d->preparse_system_old);
		StrCat("\33i", (STRPTR)d->preparse_system);
	}

	d->preparsesDone = TRUE;

	return 1;
}

static IPTR VirtualTextSetup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
	IPTR result = (IPTR)DoSuperMethodA(cl, obj, msg);
	struct VirtualTextData *d = INST_DATA(cl, obj);

	if(result)
	{
		d->setup_done = TRUE;

		d->ehn.ehn_Class = cl;
		d->ehn.ehn_Object = obj;
		d->ehn.ehn_Events = IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_RAWKEY;
		d->ehn.ehn_Flags = MUI_EHF_PRIORITY; /* not for public use? bite me. needed to override sending event first to active/default gadget */
		d->ehn.ehn_Priority = 2;

		DoMethod(_win(obj), MUIM_Window_AddEventHandler, &d->ehn);

		VirtualTextCreatePreparses(cl, obj);
	}

	return result;
}

static IPTR VirtualTextCleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
	struct VirtualTextData *d = INST_DATA(cl, obj);

	DoMethod(_win(obj), MUIM_Window_RemEventHandler, &d->ehn);

	d->setup_done = FALSE;

	return DoSuperMethodA(cl, obj, msg);
}

static IPTR VirtualTextHandleEvent(Class *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
	struct VirtualTextData *d = INST_DATA(cl, obj);
	struct IntuiMessage *imsg = msg->imsg;

	if(imsg)
	{
		if(_isinobject(imsg->MouseX, imsg->MouseY))
		{
			if(imsg->Class == IDCMP_RAWKEY)
			{
				ULONG current_top = xget(obj, MUIA_Virtgroup_Top);
				if(imsg->Code == RAWKEY_NM_WHEEL_DOWN)
				{
					set(obj, MUIA_Virtgroup_Top, current_top + 20);
					return MUI_EventHandlerRC_Eat;
				}

				if(imsg->Code == RAWKEY_NM_WHEEL_UP)
				{
					set(obj, MUIA_Virtgroup_Top, current_top - 20);
					return MUI_EventHandlerRC_Eat;
				}
			}

			if(imsg->Class == IDCMP_MOUSEBUTTONS)
			{
				if(imsg->Code == IECODE_LBUTTON)
				{
					d->mark_stop = d->mark_start = NULL;
					DoMethod(obj, VTM_SelectStart, imsg->MouseX, imsg->MouseY);
					MUI_Redraw(obj, MADF_DRAWOBJECT);
				}
				else
				{
					DoMethod(obj, VTM_CopyText);
					d->mark_stop = d->mark_start = NULL;
					MUI_Redraw(obj, MADF_DRAWOBJECT);
				}
			}

			if(imsg->Class == IDCMP_MOUSEMOVE && d->mark_start)
			{
				DoMethod(obj, VTM_SelectStop, imsg->MouseX, imsg->MouseY);
				MUI_Redraw(obj, MADF_DRAWOBJECT);
			}
		}
	}
	return 0;
}

static IPTR VirtualTextShowEnd(Class *cl, Object *obj)
{
	ULONG visible_height = xget(obj, MUIA_Height);
	ULONG virtual_height = xget(obj, MUIA_Virtgroup_Height);

	set(obj, MUIA_Virtgroup_Top, (virtual_height - visible_height) + 100);

	return (IPTR)(virtual_height - visible_height);
}

static IPTR Layout(VOID)
{
	Object *parent = (Object*) REG_A2;
	struct MUI_LayoutMsg *msg = (struct MUI_LayoutMsg*) REG_A1;
	Object *virtual_text = findobj(USD_TALKTAB_VIRTUALTEXT, parent);

	if(msg->lm_Type == MUILM_MINMAX)
	{
		struct MUI_MinMax *minmax = &msg->lm_MinMax;

		minmax->MinHeight += 100;
		minmax->MinWidth += 100;
		minmax->DefHeight += 100;
		minmax->DefWidth = 100;
		minmax->MaxHeight += MUI_MAXMAX;
		minmax->MaxWidth += MUI_MAXMAX;

		return 0;
	}

	if(msg->lm_Type == MUILM_LAYOUT)
	{
		LONG act_width = 0, act_height = 0;
		Object *cstate = (Object *)msg->lm_Children->mlh_Head;
		Object *obj;
		ULONG space_length;
		ULONG max_height_in_line = 0;

		if(!virtual_text)
			return FALSE;

		space_length = DoMethod(virtual_text, MUIM_TextDim, " ", 1, NULL, 0) & 0xFFFF;

		while((obj = (Object*)NextObject(&cstate), obj))
		{
			STRPTR txt = (STRPTR)xget(obj, MUIA_Text_Contents);
			ULONG obj_type = xget(obj, MUIA_UserData);
			ULONG defheight = _defheight(obj);

			if(max_height_in_line < defheight && !(txt && *txt == '\n'))
				max_height_in_line = defheight;

			if(obj_type & VTG_PICTUREVIEW_OBJECT)
			{
				if(!MUI_Layout(obj, act_width, act_height, _minwidth(obj), _minheight(obj), 0))
					return FALSE;

				act_width = 0;
				act_height += _minheight(obj);
				max_height_in_line = 0;
				continue;
			}

			if(txt && txt[0] == '\n')
			{
				act_width = 0;
				act_height += max_height_in_line;
				max_height_in_line = 0;

				if(!MUI_Layout(obj, act_width, act_height, _minwidth(obj), _minheight(obj), 0))
					return FALSE;
				continue;
			}

			if(txt && txt[0] == ' ')
			{
				act_width += space_length;
				if(!MUI_Layout(obj, act_width, act_height, _minwidth(obj), _minheight(obj), 0))
					return FALSE;
				continue;
			}

			if(act_width != 0 && (act_width + _minwidth(obj) >= msg->lm_Layout.Width-1))
			{
				act_width = 0;
				act_height += max_height_in_line;
				max_height_in_line = defheight;
			}

			if((obj_type & VTG_FULL_WIDTH) && msg->lm_Layout.Width >= _minwidth(obj))
			{
				if(!MUI_Layout(obj, act_width, act_height, msg->lm_Layout.Width, defheight, 0))
					return FALSE;
				act_height += max_height_in_line;
				max_height_in_line = 0;
			}
			else
			{
				if(!MUI_Layout(obj, act_width, act_height, _minwidth(obj), defheight, 0))
					return FALSE;

				act_width += _minwidth(obj);

				if(obj_type & VTG_TITLE_OBJECT)
				{
					if(xget(prefs_object(USD_PREFS_TW_HEADLINE_INSERT_NEWLINE), MUIA_Selected))
					{
						act_width = 0;
						act_height += max_height_in_line;
						max_height_in_line = 0;
					}
					else
					{
						act_width += space_length;
					}
				}
				else if(obj_type & VTG_SYSTEM_MSG_OBJECT)
				{
					act_width = 0;
					act_height += max_height_in_line;
					max_height_in_line = 0;
				}
			}
		}

		msg->lm_Layout.Height = act_height;

		return TRUE;
	}

	return MUILM_UNKNOWN;
}


static IPTR VirtualTextDraw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
	struct VirtualTextData *d = INST_DATA(cl, obj);
	IPTR result;
	ULONG height = xget(obj, MUIA_Height), width = xget(obj, MUIA_Width);

	/* prepare preparses */
	if(!(d->preparsesDone))
		VirtualTextCreatePreparses(cl, obj);

	result = (IPTR) DoSuperMethodA(cl, obj, msg);

	if(msg->flags & MADF_DRAWOBJECT)
	{
		if(d->scroll_on_resize && (d->height != height || d->width != width))
		{
			SetAttrs(obj, VTA_Height, height, VTA_Width, width, TAG_END);
			DoMethod(obj, VTM_ShowEnd);
		}
	}

	return result;
}

static IPTR VirtualTextClear(Class *cl, Object *obj)
{
	ENTER();
	if(DoSuperMethod(cl, obj, MUIM_Group_InitChange))
	{
		struct List *l = (struct List*) xget(obj, MUIA_Group_ChildList);
		Object *cstate = (Object *)l->lh_Head;
		Object *child;

		while(child = (Object*)NextObject(&cstate), child)
		{
			DoMethod(obj, OM_REMMEMBER, child);
			MUI_DisposeObject(child);
		}
		DoSuperMethod(cl, obj, MUIM_Group_ExitChange);
	}
	LEAVE();
	return (IPTR)1;
}

static inline BOOL StrIShortEqu(STRPTR s, STRPTR d) /* case insensitive, if shorter string is begining of longer returns TRUE */
{
	BYTE a, b;

	while(*s != 0x00 && *d != 0x00)
	{
		if(_between('A', *s, 'Z'))
			a = *s | 0x20;
		else
			a = *s;
		if(_between('A', *d, 'Z'))
			b = *d | 0x20;
		else
			b = *d;

		if(a != b)
			return FALSE;

		s++;
		d++;
	}

	if(*s == 0x00 && *d != 0x00)
		return FALSE;

	return TRUE;
}

static inline Object *CreateEmoticon(STRPTR path, STRPTR word, STRPTR preparse, ULONG flags, ULONG history_alpha)
{
	Object *result = NULL;
	ULONG type = VTG_EMOTICON_OBJECT;
	ULONG alpha = 0xFFFFFFFF;

	if(flags & VTV_FromHistory)
	{
		type |= VTG_EMOTICON_MESSAGE_OLD;
		alpha = history_alpha;
	}

	result = NewObject(EmoticonClass->mcc_Class, NULL,
		EMOA_PicturePath, path,
		EMOA_GlobalAlpha, alpha,
		EMOA_TextPreparse, preparse,
		MUIA_UserData, type,
		MUIA_ShortHelp, word,
	TAG_END);

	return result;
}

static inline Object* ParseSpecialWord(STRPTR word, ULONG type, STRPTR preparse, ULONG flags, ULONG history_alpha)
{
	STRPTR buffer = word; /* typically URL and word is this same */
	Object *result = NULL;

	if(IS_URL(type))
	{
		STRPTR encoded;

		if(type & URL_MAIL) /* we need to add "mailto:" in this type for proper handling by MUIC_Hyperlink */
			buffer = FmtNew("mailto:%s", word);
		else if((type & URL_PLAIN) || (type & URL_DOMAIN)) /* assume "http://" in this type */
			buffer = FmtNew("http://%s", word);

		result = MUI_NewObject(MUIC_Hyperlink,
			MUIA_UserData, VTG_NORMAL_OBJECT,
			MUIA_Weight, 0,
			MUIA_Text_Contents, word,
			MUIA_Hyperlink_URI, buffer,
		TAG_END);

		if((encoded = SystemToUtf8(buffer)))
		{
			set(result, MUIA_Hyperlink_URI, encoded);
			FreeVec(encoded);
		}
	}
	else if(IS_EMOTICON(type) && ((BOOL)xget(prefs_object(USD_PREFS_EMOTICONS_ONOFF), MUIA_Selected)))
	{
		if(IS_SIMILE(type))
		{
			LONG i;

			for(i = EmoticonsTabSize - 1; i >= 0 && !result; i--) /* search backwards, so longest go first */
			{
				if(StrIShortEqu(word, EmoticonsTab[i].emo))
					result = CreateEmoticon((STRPTR)xget(prefs_object(USD_PREFS_EMOTICONS_PATH(i)), MUIA_String_Contents), word, preparse, flags, history_alpha);
			}
		}
		else if(IS_BIG_EMOTICON(type) && ((BOOL)xget(prefs_object(USD_PREFS_EMOTICONS_LONG_ONOFF), MUIA_Selected)))
		{
			STRPTR temp;

			if((temp = StrNew(word)))
			{
				temp[StrLen(temp)-1] = 0x00; /* omit '>' */

				if((buffer = FmtNew(EMOT_DIR "%s.gif", temp + 1))) /* +1 ==> omit '<' */
					result = CreateEmoticon(buffer, word, preparse, flags, history_alpha);

				StrFree(temp);
			}
		}
		else if(IS_PUNCTUATION(type))
		{
			if(word[0] == '?')
				result = CreateEmoticon((STRPTR)xget(prefs_object(USD_PREFS_EMOTICONS_PATH_QUESTION), MUIA_String_Contents), word, preparse, flags, history_alpha);
			else if(word[0] == '!')
				result = CreateEmoticon((STRPTR)xget(prefs_object(USD_PREFS_EMOTICONS_PATH_EXCLAMATION), MUIA_String_Contents), word, preparse, flags, history_alpha);
		}
	}

	/* this special word doesn't fit to our cases ^, so we make from it normal text */
	if(result == NULL)
		result = normal_text(word, preparse);

	/* free if allocated */
	if(buffer != word) FmtFree(buffer);

	return result;
}

static inline STRPTR GetPreparse(struct VirtualTextData *d, ULONG type, ULONG flags)
{
	STRPTR preparse = NULL;

	switch(type)
	{
		case VTV_NormalMessage:
			if(flags & VTV_Outgoing)
			{
				if(flags & VTV_FromHistory)
					preparse = d->myPreparseOld;
				else
					preparse = d->myPreparse;
			}
			else
			{
				if(flags & VTV_FromHistory)
					preparse = d->friendPreparseOld;
				else
					preparse = d->friendPreparse;
			}
		break;

		case VTV_SystemMessage:
			if(flags & VTV_FromHistory)
				preparse = d->preparse_system_old;
			else
				preparse = d->preparse_system;
		break;
	}

	return preparse;
}

static IPTR VirtualTextAddMessageText(Class *cl, Object *obj, struct VTP_AddMessageText *msg)
{
	struct VirtualTextData *d = INST_DATA(cl, obj);
	BYTE normal[1024], special[1024], white[2] = {0};
	STRPTR parse = msg->message, preparse = GetPreparse(d, msg->type, msg->flags);
	ULONG type;

	if(parse == NULL || preparse == NULL) return (IPTR)0; /* if passed NULL -> do nothing */

	do
	{
		type = MessageParse(&parse, (STRPTR)normal, (STRPTR)special);

		if(type & NORMALWORD)
			DoMethod(obj, OM_ADDMEMBER, normal_text(normal, preparse));

		if(type & SPECIALWORD)
			DoMethod(obj, OM_ADDMEMBER, ParseSpecialWord((STRPTR)special, type, preparse, msg->flags, d->history_alpha));

		if(type & WHITECHAR)
		{
			Object *o;

			white[0] = *parse;

			if((o = normal_text(white, preparse)))
			{
				set(o, MUIA_ShowMe, FALSE);
				DoMethod(obj, OM_ADDMEMBER, o);
			}
			parse++;
		}

		normal[0] = special[0] = 0x00;

	}while(type);

	DoMethod(obj, OM_ADDMEMBER, normal_text("\n", NULL)); /* go to next line */

	return (IPTR)1;
}

static IPTR VirtualTextAddMessage(Class *cl, Object *obj, struct VTP_AddMessage *msg)
{
	return DoMethod(obj, VTM_AddMessageText, msg->message, VTV_NormalMessage, msg->flags);
}

static IPTR VirtualTextApplyPrefs(Class *cl, Object *obj)
{
	struct VirtualTextData *d = INST_DATA(cl, obj);
	struct List *list = (struct List*) xget(obj, MUIA_Group_ChildList);
	Object *cstate = (Object *)list->lh_Head;
	Object *child;
	ULONG title_bg = ((BOOL)xget(prefs_object(USD_PREFS_TW_HEADLINE_BACKGROUND), MUIA_Selected)) ? MUII_ButtonBack : (ULONG)"";
	ULONG title_mask = ((BOOL)xget(prefs_object(USD_PREFS_TW_HEADLINE_MAXWIDTH), MUIA_Selected)) ? (VTG_TITLE_OBJECT | VTG_FULL_WIDTH) : VTG_TITLE_OBJECT;
	ULONG system_bg = ((BOOL)xget(prefs_object(USD_PREFS_TW_SYSTEMMSG_BACKGROUND), MUIA_Selected)) ? MUII_ButtonBack : (ULONG)"";
	ULONG system_mask = ((BOOL)xget(prefs_object(USD_PREFS_TW_SYSTEMMSG_MAXWIDTH), MUIA_Selected)) ? (VTG_SYSTEM_MSG_OBJECT | VTG_FULL_WIDTH) : VTG_SYSTEM_MSG_OBJECT;

	d->history_alpha = (((DOUBLE)(100 - xget(prefs_object(USD_PREFS_OLD_MESSAGES_TRANSPARENCY), MUIA_Slider_Level))) / 100.) * 0xFFFFFFFFUL;
	d->preparsesDone = FALSE;

	if(d->setup_done)
		VirtualTextCreatePreparses(cl, obj);

	if(DoSuperMethod(cl, obj, MUIM_Group_InitChange))
	{
		while(child = (Object*)NextObject(&cstate), child)
		{
			ULONG child_type = xget(child, MUIA_UserData);

			if(child_type & VTG_TITLE_OBJECT)
			{
				set(child, MUIA_UserData, title_mask);
				set(child, MUIA_Background, title_bg);
			}

			if(child_type & VTG_SYSTEM_MSG_OBJECT)
			{
				set(child, MUIA_UserData, system_mask);
				set(child, MUIA_Background, system_bg);
			}

			if(child_type & VTG_EMOTICON_MESSAGE_OLD)
			{
				set(child, EMOA_GlobalAlpha, d->history_alpha);
			}
		}
		DoSuperMethod(cl, obj, MUIM_Group_ExitChange);
	}

	return (IPTR)1;
}

static IPTR VirtualTextAddSystemMessage(Class *cl, Object *obj, struct VTP_AddSystemMessage *msg)
{
	struct VirtualTextData *d = INST_DATA(cl, obj);
	Object *insert = NULL;

	if(xget(prefs_object(USD_PREFS_TW_SYSTEMMSG_HEADLINE), MUIA_Selected))
	{
		DoMethod(obj, VTM_FormatTime, msg->timestamp);

		/* create object */
		insert = MUI_NewObject(MUIC_Group,
			MUIA_UserData, ((BOOL)xget(prefs_object(USD_PREFS_TW_SYSTEMMSG_MAXWIDTH), MUIA_Selected)) ? (VTG_SYSTEM_MSG_OBJECT | VTG_FULL_WIDTH) : VTG_SYSTEM_MSG_OBJECT,
			MUIA_Background, ((BOOL)xget(prefs_object(USD_PREFS_TW_SYSTEMMSG_BACKGROUND), MUIA_Selected)) ? MUII_ButtonBack : (ULONG)"",
			MUIA_Group_Horiz, TRUE,
			MUIA_Group_Child, MUI_NewObject(MUIC_Text,
				MUIA_Text_PreParse, (msg->flags & VTV_FromHistory) ? d->preparse_header_l_old : d->preparse_header_l,
				MUIA_Text_Contents, ((BOOL)xget(prefs_object(USD_PREFS_TW_SYSTEMMSG_HEADLINE_REVERSE), MUIA_Selected)) ? (STRPTR)d->timeBuffer : (STRPTR)GetString(MSG_SYSTEMMSG_HEADLINE_TITLE),
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Text,
				MUIA_Text_PreParse, (msg->flags & VTV_FromHistory) ? d->preparse_header_r_old : d->preparse_header_r,
				MUIA_Text_Contents, ((BOOL)xget(prefs_object(USD_PREFS_TW_SYSTEMMSG_HEADLINE_REVERSE), MUIA_Selected)) ? (STRPTR)GetString(MSG_SYSTEMMSG_HEADLINE_TITLE) : (STRPTR)d->timeBuffer,
			TAG_END),
		TAG_END);
	}

	DoMethod(obj, OM_ADDMEMBER, normal_text("\n", NULL)); /* go to next line */

	if(insert)
		DoMethod(obj, OM_ADDMEMBER, insert);

	DoMethod(obj, VTM_AddMessageText, msg->message, VTV_SystemMessage, msg->flags);
	DoMethod(obj, OM_ADDMEMBER, normal_text("\n", NULL)); /* go to next line */

	return (IPTR)TRUE;
}

static IPTR VirtualTextAskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
	IPTR result = (IPTR) DoSuperMethodA(cl, obj, msg);

	msg->MinMaxInfo->MinWidth += 100;
	msg->MinMaxInfo->DefWidth += 100;
	msg->MinMaxInfo->MaxWidth += MUI_MAXMAX;

	msg->MinMaxInfo->MinHeight += 100;
	msg->MinMaxInfo->DefHeight += 100;
	msg->MinMaxInfo->MaxHeight += MUI_MAXMAX;

	return result;
}

#define _isinobj(x, y, obj) (_between(xget(obj, MUIA_LeftEdge),(x),xget(obj, MUIA_RightEdge)) && _between(xget(obj, MUIA_TopEdge),(y),xget(obj, MUIA_BottomEdge)))

static IPTR VirtualTextSelectStart(Class *cl, Object *obj, struct VTP_SelectStart *msg)
{
	struct VirtualTextData *d = INST_DATA(cl, obj);
	struct List *l = (struct List*) xget(obj, MUIA_Group_ChildList);
	Object *cstate = (Object *)l->lh_Head;
	Object *child;

	while((child = (Object*)NextObject(&cstate)))
	{
		LONG child_top = (LONG)xget(child, MUIA_TopEdge);

		if(child_top < 0)
			continue;

		if(child_top > msg->mouse_y)
			break;

		if(_isinobj(msg->mouse_x, msg->mouse_y, child))
			d->mark_start = child;
	}

	return (IPTR)0;
}

static IPTR VirtualTextSelectStop(Class *cl, Object *obj, struct VTP_SelectStop *msg)
{
	struct VirtualTextData *d = INST_DATA(cl, obj);
	struct List *l = (struct List*) xget(obj, MUIA_Group_ChildList);
	Object *cstate = (Object *)l->lh_Head;
	Object *child;

	while((child = (Object*)NextObject(&cstate)))
	{
		LONG child_top = (LONG)xget(child, MUIA_TopEdge);

		if(child_top < 0)
			continue;

		if(child_top > msg->mouse_y)
			break;

		if(_isinobj(msg->mouse_x, msg->mouse_y, child))
		{
			d->mark_stop = child;
		}
	}

	if(d->mark_start && d->mark_stop)
	{
		cstate = (Object *)l->lh_Head;
		BOOL between = FALSE;
		BOOL change = FALSE;

		if(d->mark_start != d->mark_stop)
		{
			while((child = (Object*)NextObject(&cstate)))
			{
				if(between == FALSE && (child == d->mark_start || child == d->mark_stop))
					change = between = TRUE;

				if(between)
					set(child, MUIA_Background, d->selectBackground);
				else
				{
					ULONG child_type = xget(child, MUIA_UserData);

					if(child_type & VTG_TITLE_OBJECT && xget(prefs_object(USD_PREFS_TW_HEADLINE_BACKGROUND), MUIA_Selected))
						set(child, MUIA_Background, MUII_ButtonBack);
					else if(child_type & VTG_SYSTEM_MSG_OBJECT && xget(prefs_object(USD_PREFS_TW_SYSTEMMSG_BACKGROUND), MUIA_Selected))
						set(child, MUIA_Background, MUII_ButtonBack);
					else
						set(child, MUIA_Background, "");
				}

				if(change == FALSE && between == TRUE && (child == d->mark_start || child == d->mark_stop))
					between = FALSE;
				change = FALSE;
			}
		}
		else
			set(d->mark_start, MUIA_Background, d->selectBackground);
	}

	return (IPTR)0;
}

static IPTR VirtualTextCopyText(Class *cl, Object *obj)
{
	struct VirtualTextData *d = INST_DATA(cl, obj);
	struct List *l = (struct List*) xget(obj, MUIA_Group_ChildList);
	Object *cstate = (Object *)l->lh_Head, *child;
	BOOL between = FALSE, change = FALSE;
	ULONG txt_len = 0;

	if(d->mark_start && d->mark_stop)
	{
		if(d->mark_start == d->mark_stop && xget(d->mark_start, MUIA_Text_Contents))
		{
			ULONG child_type = xget(d->mark_start, MUIA_UserData);

			if(child_type & VTG_TITLE_OBJECT || child_type & VTG_SYSTEM_MSG_OBJECT)
			{
				Object *timeobj;

				txt_len += 1; /* for " " */

				if((timeobj = (Object*)DoMethod(d->mark_start, MUIM_Family_GetChild, 1, d->mark_start)))
					txt_len += StrLen((STRPTR)xget(timeobj, MUIA_Text_Contents));
				else
					txt_len += StrLen((STRPTR)"[??:??:??]");

				txt_len += 1; /* for "\n" */
			}
			txt_len += StrLen((STRPTR)xget(d->mark_start, MUIA_Text_Contents));

			if(txt_len > 0)
			{
				if(DoMethod(_app(obj), APPM_ClipboardStart, txt_len))
				{
					DoMethod(_app(obj), APPM_ClipboardWrite, (STRPTR)xget(d->mark_start, MUIA_Text_Contents), -1);

					if(child_type & VTG_TITLE_OBJECT || child_type & VTG_SYSTEM_MSG_OBJECT)
					{
						Object *timeobj;

						DoMethod(_app(obj), APPM_ClipboardWrite, " ", 1);

						if((timeobj = (Object*)DoMethod(d->mark_start, MUIM_Family_GetChild, 1, d->mark_start)))
							DoMethod(_app(obj), APPM_ClipboardWrite, xget(timeobj, MUIA_Text_Contents), -1);
						else
							DoMethod(_app(obj), APPM_ClipboardWrite, "[??:??:??]", 10);

						DoMethod(_app(obj), APPM_ClipboardWrite, "\n", 1);
					}
					DoMethod(_app(obj), APPM_ClipboardEnd);
				}
			}

			if(child_type & VTG_TITLE_OBJECT && xget(prefs_object(USD_PREFS_TW_HEADLINE_BACKGROUND), MUIA_Selected))
				set(d->mark_start, MUIA_Background, MUII_ButtonBack);
			else if(child_type & VTG_SYSTEM_MSG_OBJECT && xget(prefs_object(USD_PREFS_TW_SYSTEMMSG_BACKGROUND), MUIA_Selected))
				set(d->mark_start, MUIA_Background, MUII_ButtonBack);
			else
				set(d->mark_start, MUIA_Background, "");

			return (IPTR)0;
		}

		while((child = (Object*)NextObject(&cstate)))
		{
			ULONG child_type = xget(child, MUIA_UserData);

			if(between == FALSE && (child == d->mark_start || child == d->mark_stop))
			{
				between = TRUE;
				change = TRUE;
			}

			if(between)
			{
				if(child_type & VTG_TITLE_OBJECT || child_type & VTG_SYSTEM_MSG_OBJECT)
				{
					Object *timeobj;

					txt_len += 1; /* for " " */

					if((timeobj = (Object*)DoMethod(child, MUIM_Family_GetChild, 1, child)))
						txt_len += StrLen((STRPTR)xget(timeobj, MUIA_Text_Contents));
					else
						txt_len += StrLen((STRPTR)"[??:??:??]");

					txt_len += 1; /* for "\n" */
				}
				txt_len += StrLen((STRPTR)xget(child, MUIA_Text_Contents));
			}
			if(change == FALSE && between == TRUE && (child == d->mark_start || child == d->mark_stop))
				between = FALSE;

			change = FALSE;

			if(child_type & VTG_TITLE_OBJECT && xget(prefs_object(USD_PREFS_TW_HEADLINE_BACKGROUND), MUIA_Selected))
				set(child, MUIA_Background, MUII_ButtonBack);
			else if(child_type & VTG_SYSTEM_MSG_OBJECT && xget(prefs_object(USD_PREFS_TW_SYSTEMMSG_BACKGROUND), MUIA_Selected))
				set(child, MUIA_Background, MUII_ButtonBack);
			else
				set(child, MUIA_Background, "");
		}


		if(DoMethod(_app(obj), APPM_ClipboardStart, txt_len))
		{
			cstate = (Object *)l->lh_Head;
			between = change = FALSE;

			while((child = (Object*)NextObject(&cstate)))
			{
				ULONG child_type = xget(child, MUIA_UserData);

				if(between == FALSE && (child == d->mark_start || child == d->mark_stop))
					between = change = TRUE;

				if(between)
				{
					DoMethod(_app(obj), APPM_ClipboardWrite, (STRPTR)xget(child, MUIA_Text_Contents), -1);

					if(child_type & VTG_TITLE_OBJECT || child_type & VTG_SYSTEM_MSG_OBJECT)
					{
						Object *timeobj;

						DoMethod(_app(obj), APPM_ClipboardWrite, " ", 1);

						if((timeobj = (Object*)DoMethod(child, MUIM_Family_GetChild, 1, child)))
							DoMethod(_app(obj), APPM_ClipboardWrite, xget(timeobj, MUIA_Text_Contents), -1);
						else
							DoMethod(_app(obj), APPM_ClipboardWrite, "[??:??:??]", 10);

						DoMethod(_app(obj), APPM_ClipboardWrite, "\n", 1);
					}
				}
				if(change == FALSE && between == TRUE && (child == d->mark_start || child == d->mark_stop))
					between = FALSE;

				change = FALSE;

				if(child_type & VTG_TITLE_OBJECT && xget(prefs_object(USD_PREFS_TW_HEADLINE_BACKGROUND), MUIA_Selected))
					set(child, MUIA_Background, MUII_ButtonBack);
				else if(child_type & VTG_SYSTEM_MSG_OBJECT && xget(prefs_object(USD_PREFS_TW_SYSTEMMSG_BACKGROUND), MUIA_Selected))
					set(child, MUIA_Background, MUII_ButtonBack);
				else
					set(child, MUIA_Background, "");
			}
			DoMethod(_app(obj), APPM_ClipboardEnd);
		}
	}

	return (IPTR)0;
}

static IPTR VirtualTextAddMessageHeadLine(Class *cl, Object *obj, struct VTP_AddMessageHeadLine *msg)
{
	struct VirtualTextData *d = INST_DATA(cl, obj);
	BOOL result = FALSE;
	Object *child;

	DoMethod(obj, VTM_FormatTime, msg->timestamp);

	child = MUI_NewObject(MUIC_Group,
		MUIA_UserData, ((BOOL)xget(prefs_object(USD_PREFS_TW_HEADLINE_MAXWIDTH), MUIA_Selected)) ? (VTG_TITLE_OBJECT | VTG_FULL_WIDTH) : VTG_TITLE_OBJECT,
		MUIA_Background, ((BOOL)xget(prefs_object(USD_PREFS_TW_HEADLINE_BACKGROUND), MUIA_Selected)) ? MUII_ButtonBack : (ULONG)"",
		MUIA_Group_Horiz, TRUE,
		MUIA_Group_Child, MUI_NewObject(MUIC_Text,
			MUIA_Text_PreParse, (msg->flags & VTV_FromHistory) ? d->preparse_header_l_old : d->preparse_header_l,
			MUIA_Text_Contents, ((BOOL)xget(prefs_object(USD_PREFS_TW_HEADLINE_REVERSE), MUIA_Selected)) ? (STRPTR)d->timeBuffer : (STRPTR)msg->sender,
		TAG_END),
		MUIA_Group_Child, MUI_NewObject(MUIC_Text,
			MUIA_Text_PreParse, (msg->flags & VTV_FromHistory) ? d->preparse_header_r_old : d->preparse_header_r,
			MUIA_Text_Contents, ((BOOL)xget(prefs_object(USD_PREFS_TW_HEADLINE_REVERSE), MUIA_Selected)) ? (STRPTR)msg->sender : (STRPTR)d->timeBuffer,
		TAG_END),
	TAG_END);

	if(child)
	{
		DoMethod(obj, OM_ADDMEMBER, child);
		result = TRUE;
	}

	return (IPTR)result;
}

static IPTR VirtualTextAddPicture(Class *cl, Object *obj, struct VTP_AddPicture *msg)
{
	BOOL result = FALSE;
	Object *pic = NewObject(PictureViewClass->mcc_Class, NULL,
		MUIA_UserData, VTG_PICTUREVIEW_OBJECT,
		PVA_PictureData, msg->picture,
		PVA_PictureDataSize, msg->picture_size,
	TAG_END);

	if(pic)
	{
		DoMethod(obj, OM_ADDMEMBER, pic);
		DoMethod(obj, OM_ADDMEMBER, normal_text("\n", NULL));
		result = TRUE;
	}

	return (IPTR)result;
}

static IPTR VirtualTextAddFileView(Class *cl, Object *obj, struct VTP_AddFileView *msg)
{
	struct VirtualTextData *d = INST_DATA(cl, obj);
	BOOL result = FALSE;
	Object *fv = NewObject(FileViewClass->mcc_Class, NULL,
		MUIA_UserData, VTG_FULL_WIDTH,
		FVA_FileData, msg->data,
		FVA_FileDataSize, msg->data_size,
		FVA_TextContents, msg->txt_contents,
		FVA_TextPreparse, d->preparse_system,
	TAG_END);

	if(fv)
	{
		DoMethod(obj, OM_ADDMEMBER, fv);
		DoMethod(obj, OM_ADDMEMBER, normal_text("\n", NULL));
		result = TRUE;
	}

	return result;
}

static IPTR VirtualTextFormatTime(Class *cl, Object *obj, struct VTP_FormatTime *msg)
{
	struct VirtualTextData *d = INST_DATA(cl, obj);
	struct DateTime dt;
	UBYTE day_buffer[128];

	if(msg->timestamp == 0)
		msg->timestamp = ActLocalTime2Amiga();
	else
		msg->timestamp = UTCToLocal(msg->timestamp, d->locale);

	Amiga2DateStamp(msg->timestamp, &dt.dat_Stamp);
	dt.dat_Format = FORMAT_DEF;
	dt.dat_Flags = DTF_SUBST;
	dt.dat_StrDate = day_buffer;
	dt.dat_StrDay = NULL;
	dt.dat_StrTime = NULL;
	DateToStr(&dt);

	FmtNPut((STRPTR)d->timeBuffer, "[%ls %02d:%02d:%02d]", sizeof(d->timeBuffer), dt.dat_StrDate,
	 dt.dat_Stamp.ds_Minute / 60, dt.dat_Stamp.ds_Minute % 60, dt.dat_Stamp.ds_Tick / TICKS_PER_SECOND);

	return (IPTR)d->timeBuffer;
}

static IPTR VirtualTextRelayout(Class *cl, Object *obj)
{
	DoSuperMethod(cl, obj, MUIM_Group_InitChange);
	DoSuperMethod(cl, obj, MUIM_Group_ExitChange);

	return (IPTR)0;
}

static IPTR VirtualTextInitChange(Class *cl, Object *obj)
{
	struct VirtualTextData *d = INST_DATA(cl, obj);
	ULONG height = xget(obj, MUIA_Height);

	d->show_end = _between(height - 30, xget(obj, MUIA_Virtgroup_Height) - xget(obj, MUIA_Virtgroup_Top), height + 30);

	return DoSuperMethod(cl, obj, MUIM_Group_InitChange);
}

static IPTR VirtualTextExitChange(Class *cl, Object *obj)
{
	struct VirtualTextData *d = INST_DATA(cl, obj);
	IPTR result = DoSuperMethod(cl, obj, MUIM_Group_ExitChange);

	if(d->show_end)
		DoMethod(obj, VTM_ShowEnd);

	return result;
}

static IPTR VirtualTextDispatcher(void)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch(msg->MethodID)
	{
		case OM_NEW: return(VirtualTextNew(cl, obj, (struct opSet*)msg));
		case OM_DISPOSE: return(VirtualTextDispose(cl, obj, msg));
		case OM_GET: return(VirtualTextGet(cl, obj, (struct opGet*)msg));
		case OM_SET: return(VirtualTextSet(cl, obj, (struct opSet*)msg));
		case MUIM_Setup: return(VirtualTextSetup(cl, obj, (struct MUIP_Setup*)msg));
		case MUIM_Cleanup: return(VirtualTextCleanup(cl, obj, (struct MUIP_Cleanup*)msg));
		case MUIM_HandleEvent: return(VirtualTextHandleEvent(cl, obj, (struct MUIP_HandleEvent*)msg));
		case VTM_AddMessage: return(VirtualTextAddMessage(cl, obj, (struct VTP_AddMessage*)msg));
		case VTM_AddMessageText: return(VirtualTextAddMessageText(cl, obj, (struct VTP_AddMessageText*)msg));
		case VTM_ShowEnd: return(VirtualTextShowEnd(cl, obj));
		case VTM_Clear: return(VirtualTextClear(cl, obj));
		case MUIM_Draw: return(VirtualTextDraw(cl, obj, (struct MUIP_Draw*)msg));
		case VTM_ApplyPrefs: return(VirtualTextApplyPrefs(cl, obj));
		case VTM_AddSystemMessage: return(VirtualTextAddSystemMessage(cl, obj, (struct VTP_AddSystemMessage*)msg));
		case MUIM_AskMinMax: return(VirtualTextAskMinMax(cl, obj, (struct MUIP_AskMinMax*) msg));
		case VTM_SelectStart: return(VirtualTextSelectStart(cl, obj, (struct VTP_SelectStart*)msg));
		case VTM_SelectStop: return(VirtualTextSelectStop(cl, obj, (struct VTP_SelectStop*)msg));
		case VTM_CopyText: return(VirtualTextCopyText(cl, obj));
		case VTM_AddMessageHeadLine: return(VirtualTextAddMessageHeadLine(cl, obj, (struct VTP_AddMessageHeadLine*)msg));
		case VTM_AddPicture: return(VirtualTextAddPicture(cl, obj, (struct VTP_AddPicture*)msg));
		case VTM_AddFileView: return(VirtualTextAddFileView(cl, obj, (struct VTP_AddFileView*)msg));
		case VTM_FormatTime: return(VirtualTextFormatTime(cl, obj, (struct VTP_FormatTime*)msg));
		case VTM_Relayout: return(VirtualTextRelayout(cl, obj));
		case VTM_InitChange: return(VirtualTextInitChange(cl, obj));
		case VTM_ExitChange: return(VirtualTextExitChange(cl, obj));
		default: return(DoSuperMethodA(cl, obj, msg));
	}
}
