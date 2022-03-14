/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <libvstring.h>
#include "support.h"
#include "minmaxslider.h"

struct MUI_CustomClass *MinMaxSliderClass;


static IPTR MinMaxSliderDispatcher(VOID);
const struct EmulLibEntry MinMaxSliderGate = {TRAP_LIB, 0, (VOID(*)(VOID))MinMaxSliderDispatcher};

struct MinMaxSliderData
{
	STRPTR mmsd_MinText;
	STRPTR mmsd_MaxText;
	STRPTR mmsd_Unit;
};

struct MUI_CustomClass *CreateMinMaxSliderClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Slider, NULL, sizeof(struct MinMaxSliderData), (APTR)&MinMaxSliderGate);
	MinMaxSliderClass = cl;
	return cl;
}

VOID DeleteMinMaxSliderClass(VOID)
{
	if (MinMaxSliderClass) MUI_DeleteCustomClass(MinMaxSliderClass);
}

static IPTR MinMaxSliderNew(Class *cl, Object *obj, struct opSet *msg)
{
	if((obj = (Object*)DoSuperMethodA(cl, obj, msg)))
	{
		struct MinMaxSliderData *d = INST_DATA(cl, obj);

		d->mmsd_MaxText = StrNew((STRPTR)GetTagData(MMSA_MaxText, (IPTR)NULL, msg->ops_AttrList));
		d->mmsd_MinText = StrNew((STRPTR)GetTagData(MMSA_MinText, (IPTR)NULL, msg->ops_AttrList));
		d->mmsd_Unit = StrNew((STRPTR)GetTagData(MMSA_Unit, (IPTR)NULL, msg->ops_AttrList));

		return (IPTR)obj;
	}

	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}

static IPTR MinMaxSliderStringify(Class *cl, Object *obj, struct MUIP_Numeric_Stringify *msg)
{
	struct MinMaxSliderData *d = INST_DATA(cl, obj);
	static BYTE buffer[50];

	if(msg->value == xget(obj, MUIA_Slider_Max) && d->mmsd_MaxText)
		return (IPTR)d->mmsd_MaxText;
	else if(msg->value == xget(obj, MUIA_Slider_Min) && d->mmsd_MinText)
		return (IPTR)d->mmsd_MinText;

	if(d->mmsd_Unit)
		FmtNPut((STRPTR)buffer, "%ld %ls", sizeof(buffer), msg->value, d->mmsd_Unit);
	else
		FmtNPut((STRPTR)buffer, "%ld", sizeof(buffer), msg->value);

	return (IPTR) buffer;
}

static IPTR MinMaxSliderDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW:  return (MinMaxSliderNew(cl, obj, (struct opSet*) msg));
		case MUIM_Numeric_Stringify: return(MinMaxSliderStringify(cl, obj, (struct MUIP_Numeric_Stringify*) msg));
		default:  return (DoSuperMethodA(cl, obj, msg));
	}
}

