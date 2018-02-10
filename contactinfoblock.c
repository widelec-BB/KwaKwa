/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/cybergraphics.h>
#include <libvstring.h>

#include "globaldefines.h"
#include "talkwindow.h"
#include "contactslist.h"
#include "contactinfoblock.h"

#include "kwakwa_api/defs.h"
#include "kwakwa_api/pictures.h"

#define SPACE_BETWEEN_NAME_AND_DESCRIPTION 5

struct MUI_CustomClass *ContactInfoBlockClass;

static IPTR ContactInfoBlockDispatcher(VOID);
const struct EmulLibEntry ContactInfoBlockGate = {TRAP_LIB, 0, (VOID(*)(VOID))ContactInfoBlockDispatcher};

/* DIRTY HACK */
struct CustomRenderInfo
{
	char dummy[256];
	struct TextFont *mri_Font[MUIV_Font_Count];
};

static IPTR ChangeFont(Object *obj, LONG font)
{
	struct CustomRenderInfo *cri = (struct CustomRenderInfo *)muiRenderInfo(obj);
	IPTR result = (IPTR)_font(obj);

	_font(obj) = cri->mri_Font[-font];

	return result;
}
/* DIRTY HACK END */

struct ContactInfoBlockData
{
	struct ContactEntry *contact; /* contact to draw, may be null! */
};


struct MUI_CustomClass *CreateContactInfoBlockClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Area, NULL, sizeof(struct ContactInfoBlockData), (APTR)&ContactInfoBlockGate);
	ContactInfoBlockClass = cl;
	return cl;
}

VOID DeleteContactInfoBlockClass(VOID)
{
	if (ContactInfoBlockClass) MUI_DeleteCustomClass(ContactInfoBlockClass);
}


static IPTR ContactInfoBlockNew(Class *cl, Object *obj, struct opSet *msg)
{
	struct TagItem *tag;

	tag = FindTagItem(CIBA_ContactEntry, msg->ops_AttrList);

	obj = DoSuperNew(cl, obj,
		MUIA_Frame, MUIV_Frame_Group,
		MUIA_Background, MUII_GroupBack,
	TAG_MORE, msg->ops_AttrList);

	if(obj)
	{
		struct ContactInfoBlockData *d = INST_DATA(cl, obj);

		if(tag != NULL)
		{
			d->contact = (struct ContactEntry*) tag->ti_Data;
		}

		return (IPTR)obj;
	}

	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}


static IPTR ContactInfoBlockSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct ContactInfoBlockData *d = INST_DATA(cl, obj);
	int tagcount = 0;
	struct TagItem *tag, *tagptr = msg->ops_AttrList;

	while((tag = NextTagItem(&tagptr)))
	{
		switch(tag->ti_Tag)
		{
			case CIBA_ContactEntry:
				d->contact = (struct ContactEntry*) tag->ti_Data;
				tagcount++;
			break;
		}
	}

	tagcount += DoSuperMethodA(cl, obj, (Msg)msg);
	return tagcount;
}

static IPTR ContactInfoBlockGet(Class *cl, Object *obj, struct opGet *msg)
{
	struct ContactInfoBlockData *d = INST_DATA(cl, obj);
	int rv = FALSE;

	switch(msg->opg_AttrID)
	{
		case CIBA_ContactEntry:
			*msg->opg_Storage = (ULONG)d->contact;
		return TRUE;

		default: rv = (DoSuperMethodA(cl, obj, (Msg)msg));
	}

	return rv;
}

static IPTR ContactInfoBlockAskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
	struct ContactInfoBlockData *d = INST_DATA(cl, obj);
	IPTR result = DoSuperMethodA(cl, obj, msg);

	/* we don't care about Def and Min Width */
	msg->MinMaxInfo->MaxWidth += MUI_MAXMAX;

	if(d->contact && d->contact->avatar)
	{
		msg->MinMaxInfo->MinHeight += d->contact->avatar->p_Height;
		msg->MinMaxInfo->DefHeight += d->contact->avatar->p_Height;
		msg->MinMaxInfo->MaxHeight += d->contact->avatar->p_Height;
	}
	else
	{
		msg->MinMaxInfo->MinHeight += 50;
		msg->MinMaxInfo->DefHeight += 50;
		msg->MinMaxInfo->MaxHeight += 50;
	}

	return result;
}

static IPTR ContactInfoBlockDraw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
	struct ContactInfoBlockData *d = INST_DATA(cl, obj);

	DoSuperMethodA(cl, obj, msg);

	if(msg->flags & MADF_DRAWOBJECT)
	{
		if(d->contact)
		{
			BYTE buffer[512];
			ULONG name_height;
			APTR clip;

			if(KWA_S_AVAIL(d->contact->status))
				FmtNPut((STRPTR)buffer, "%s %s", sizeof(buffer), "\33I[4:PROGDIR:gfx/available.mbr]", ContactName(d->contact));
			else if(KWA_S_BUSY(d->contact->status))
				FmtNPut((STRPTR)buffer, "%s %s", sizeof(buffer), "\33I[4:PROGDIR:gfx/away.mbr]", ContactName(d->contact));
			else if(KWA_S_FFC(d->contact->status))
				FmtNPut((STRPTR)buffer, "%s %s", sizeof(buffer), "\33I[4:PROGDIR:gfx/ffc.mbr]", ContactName(d->contact));
			else if(KWA_S_DND(d->contact->status))
				FmtNPut((STRPTR)buffer, "%s %s", sizeof(buffer), "\33I[4:PROGDIR:gfx/dnd.mbr]", ContactName(d->contact));
			else if(KWA_S_BLOCKED(d->contact->status))
				FmtNPut((STRPTR)buffer, "%s %s", sizeof(buffer), "\33I[4:PROGDIR:gfx/blocked.mbr]", ContactName(d->contact));
			else if(KWA_S_INVISIBLE(d->contact->status))
				FmtNPut((STRPTR)buffer, "%s %s", sizeof(buffer), "\33I[4:PROGDIR:gfx/invisible.mbr]", ContactName(d->contact));
			else
				FmtNPut((STRPTR)buffer, "%s %s", sizeof(buffer), "\33I[4:PROGDIR:gfx/unavailable.mbr]", ContactName(d->contact));

			clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj));

			name_height = DoMethod(obj, MUIM_TextDim, (STRPTR)buffer, StrLen((STRPTR)buffer), NULL, 0x00) >> 16;

			DoMethod(obj, MUIM_Text, _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj), (STRPTR)buffer, StrLen((STRPTR)buffer), NULL, 0x00);

			if(d->contact->statusdesc != NULL)
			{
				IPTR old_font = ChangeFont(obj, MUIV_Font_Tiny);
				LONG act_desc_height = name_height + SPACE_BETWEEN_NAME_AND_DESCRIPTION, words_in_line = 0, width = d->contact->avatar ? _mwidth(obj) - d->contact->avatar->p_Width : _mwidth(obj);
				STRPTR line_end = d->contact->statusdesc, line_start = d->contact->statusdesc, prev_word_end = NULL, last_space = NULL;
				ULONG line_size;

				while(*line_end != 0x00)
				{
					if(*line_end == ' ')
					{
						line_size = DoMethod(obj, MUIM_TextDim, line_start, line_end - line_start, NULL, 0x00);

						if(words_in_line != 0 && (line_size & 0xFFFF) >= width)
						{
							ULONG prev_line_size = DoMethod(obj, MUIM_TextDim, line_start, prev_word_end - line_start, NULL, 0x00);

							DoMethod(obj, MUIM_Text, _mleft(obj), _mtop(obj) + act_desc_height, _mwidth(obj), _mheight(obj), line_start, prev_word_end - line_start, NULL, 0x00);

							act_desc_height += prev_line_size >> 16;
							words_in_line = 0;
							line_start = prev_word_end + 1;
						}
						prev_word_end = line_end;
						words_in_line++;
					}
					line_end++;
				}

				line_size = DoMethod(obj, MUIM_TextDim, line_start, line_end - line_start, NULL, 0x00);

				if((line_size & 0xFFFF) >= width)
				{
					last_space = line_end;
					while(last_space >= line_start && *last_space != ' ') /* search for last ' ' in line */
						last_space--;
				}

				if(last_space != NULL && last_space > line_start && *last_space == ' ')
				{
					DoMethod(obj, MUIM_Text, _mleft(obj), _mtop(obj) + act_desc_height, _mwidth(obj), _mheight(obj), line_start, last_space - line_start, NULL, 0x00);

					act_desc_height += (line_size = DoMethod(obj, MUIM_TextDim, line_start, last_space - line_start, NULL, 0x00) >> 16);

					DoMethod(obj, MUIM_Text, _mleft(obj), _mtop(obj) + act_desc_height, _mwidth(obj), _mheight(obj), last_space + 1, line_end - last_space - 1, NULL, 0x00);

					act_desc_height += (line_size = DoMethod(obj, MUIM_TextDim, last_space + 1, line_end - last_space - 1, NULL, 0x00) >> 16);
				}
				else
				{
					DoMethod(obj, MUIM_Text, _mleft(obj), _mtop(obj) + act_desc_height, _mwidth(obj), _mheight(obj), line_start, line_end - line_start, NULL, 0x00);
					act_desc_height +=  line_size >> 16;
				}

				_font(obj) = (struct TextFont *) old_font;
			}

			if(d->contact->avatar != NULL)
			{
				struct Picture *avatar = d->contact->avatar;

				WritePixelArrayAlpha(avatar->p_Data, 0, 0, avatar->p_Width << 2, _rp(obj), _mright(obj) - avatar->p_Width,
				 _mtop(obj), avatar->p_Width, avatar->p_Height, 0xFFFFFFFF);
			}
			MUI_RemoveClipping(muiRenderInfo(obj), clip);
		}
	}

	return (IPTR)0;
}

static IPTR ContactInfoBlockDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW:  return (ContactInfoBlockNew(cl, obj, (struct opSet*)msg));
		case OM_SET:  return (ContactInfoBlockSet(cl, obj, (struct opSet*)msg));
		case OM_GET:  return (ContactInfoBlockGet(cl, obj, (struct opGet*)msg));
		case MUIM_AskMinMax: return(ContactInfoBlockAskMinMax(cl, obj, (struct MUIP_AskMinMax*) msg));
		case MUIM_Draw: return(ContactInfoBlockDraw(cl, obj, (struct MUIP_Draw*) msg));
		default:  return (DoSuperMethodA(cl, obj, msg));
	}
}

