/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <libvstring.h>

#include "percentageslider.h"

struct MUI_CustomClass *PercentageSliderClass;


static IPTR PercentageSliderDispatcher(VOID);
const struct EmulLibEntry PercentageSliderGate = {TRAP_LIB, 0, (VOID(*)(VOID))PercentageSliderDispatcher};


struct MUI_CustomClass *CreatePercentageSliderClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Slider, NULL, 0, (APTR)&PercentageSliderGate);
	PercentageSliderClass = cl;
	return cl;
}

VOID DeletePercentageSliderClass(VOID)
{
	if (PercentageSliderClass) MUI_DeleteCustomClass(PercentageSliderClass);
}

static IPTR PercentageSliderNew(Class *cl, Object *obj, struct opSet *msg)
{
	obj = DoSuperNew(cl, obj,
		MUIA_Slider_Min, 1,
		MUIA_Slider_Max, 100,
		MUIA_Slider_Level, 75,
	TAG_MORE, msg->ops_AttrList);

	if(obj)
	{
		return (IPTR)obj;
	}

	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}

static IPTR PercentageSliderStringify(Class *cl, Object *obj, struct MUIP_Numeric_Stringify *msg)
{
	static BYTE buffer[5];

	FmtNPut((STRPTR)buffer, "%ld%%", sizeof(buffer), msg->value);

	return (IPTR) buffer;
}

static IPTR PercentageSliderDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW: return (PercentageSliderNew(cl, obj, (struct opSet*) msg));
		case MUIM_Numeric_Stringify: return(PercentageSliderStringify(cl, obj, (struct MUIP_Numeric_Stringify*) msg));
		default:  return (DoSuperMethodA(cl, obj, msg));
	}
}

