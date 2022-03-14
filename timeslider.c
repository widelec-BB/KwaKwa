/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <libvstring.h>
#include "locale.h"
#include "globaldefines.h"
#include "support.h"
#include "timeslider.h"

struct MUI_CustomClass *TimeSliderClass;


static IPTR TimeSliderDispatcher(VOID);
const struct EmulLibEntry TimeSliderGate = {TRAP_LIB, 0, (VOID(*)(VOID))TimeSliderDispatcher};


struct TimeSliderData
{
	BOOL tsd_OnlyCompleteValues;
	STRPTR tsd_MinText;
};

struct MUI_CustomClass *CreateTimeSliderClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Slider, NULL, sizeof(struct TimeSliderData), (APTR)&TimeSliderGate);
	TimeSliderClass = cl;
	return cl;
}

VOID DeleteTimeSliderClass(VOID)
{
	if (TimeSliderClass) MUI_DeleteCustomClass(TimeSliderClass);
}

static IPTR TimeSliderNew(Class *cl, Object *obj, struct opSet *msg)
{
	obj = (Object*)DoSuperNew(cl, obj,
		MUIA_Slider_Min, 0,
		MUIA_Slider_Max, 86400,
	TAG_MORE, (ULONG)msg->ops_AttrList);

	if(obj)
	{
		struct TimeSliderData *d = INST_DATA(cl, obj);

		d->tsd_OnlyCompleteValues = (BOOL)GetTagData(TSA_OnlyCompleteValues, (ULONG)FALSE, msg->ops_AttrList);
		d->tsd_MinText = (STRPTR)GetTagData(TSA_MinText, (ULONG)GetString(MSG_TIMESLIDER_CLASS_NEVER), msg->ops_AttrList);

		return (IPTR)obj;
	}

	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}

static IPTR TimeSliderStringify(Class *cl, Object *obj, struct MUIP_Numeric_Stringify *msg)
{
	struct TimeSliderData *d = INST_DATA(cl, obj);
	static BYTE buffer[25];

	if(msg->value == 0 && d->tsd_MinText)
	{
		FmtNPut((STRPTR)buffer, d->tsd_MinText, sizeof(buffer));
	}
	else if(msg->value < 60)
	{
		FmtNPut((STRPTR)buffer, "%ld %ls", sizeof(buffer), msg->value, GetString(MSG_TIMESLIDER_CLASS_SEC));
	}
	else if(msg->value < 3600)
	{
		if(msg->value % 60)
		{
			FmtNPut((STRPTR)buffer, "%ld %ls %ld %ls", sizeof(buffer), msg->value / 60, GetString(MSG_TIMESLIDER_CLASS_MIN),
			 msg->value % 60, GetString(MSG_TIMESLIDER_CLASS_SEC));
		}
		else
		{
			FmtNPut((STRPTR)buffer, "%ld %ls", sizeof(buffer), msg->value / 60, GetString(MSG_TIMESLIDER_CLASS_MIN));
		}
	}
	else
	{
		if(!(msg->value % 3600))
		{
			FmtNPut((STRPTR)buffer, "%ld %ls", sizeof(buffer), msg->value / 3600, GetString(MSG_TIMESLIDER_CLASS_H));
		}
		else if(msg->value % 60)
		{
			FmtNPut((STRPTR)buffer, "%ld %ls %ld %ls %ld %ls", sizeof(buffer), msg->value / 3600, GetString(MSG_TIMESLIDER_CLASS_H),
			 (msg->value % 3600) / 60, GetString(MSG_TIMESLIDER_CLASS_MIN), msg->value % 60, GetString(MSG_TIMESLIDER_CLASS_SEC));
		}
		else
		{
			FmtNPut((STRPTR)buffer, "%ld %ls %ld %ls", sizeof(buffer), msg->value / 3600, GetString(MSG_TIMESLIDER_CLASS_H),
			 (msg->value % 3600) / 60, GetString(MSG_TIMESLIDER_CLASS_MIN));
		}
	}

	return (IPTR) buffer;
}

static IPTR TimeSliderIncrease(Class *cl, Object *obj, struct MUIP_Numeric_Increase *msg)
{
	struct TimeSliderData *d = INST_DATA(cl, obj);

	if(d->tsd_OnlyCompleteValues)
	{
		ULONG v = xget(obj, MUIA_Slider_Level);

		if(v >= 60 && v < 3600)
			msg->amount *= 60;
		else if(v >= 3600)
			msg->amount *= 3600;
	}

	return (IPTR)DoSuperMethodA(cl, obj, msg);
}

static IPTR TimeSliderDecrease(Class *cl, Object *obj, struct MUIP_Numeric_Decrease *msg)
{
	struct TimeSliderData *d = INST_DATA(cl, obj);

	if(d->tsd_OnlyCompleteValues)
	{
		ULONG v = xget(obj, MUIA_Slider_Level);

		if(v > 60 && v <= 3600)
			msg->amount *= 60;
		else if(v > 3600)
			msg->amount *= 3600;
	}

	return (IPTR)DoSuperMethodA(cl, obj, msg);
}

static IPTR TimeSliderScaleToValue(Class *cl, Object *obj, struct MUIP_Numeric_ScaleToValue *msg)
{
	struct TimeSliderData *d = INST_DATA(cl, obj);
	LONG result = DoSuperMethodA(cl, obj, msg);

	if(d->tsd_OnlyCompleteValues)
	{
		if(result >= 60 && result < 3600)
			result -= result % 60;
		else if(result >= 3600)
			result -= result % 3600;
	}

	return (IPTR)result;
}

static IPTR TimeSliderDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW:  return (TimeSliderNew(cl, obj, (struct opSet*) msg));
		case MUIM_Numeric_Stringify: return(TimeSliderStringify(cl, obj, (struct MUIP_Numeric_Stringify*) msg));
		case MUIM_Numeric_Increase: return(TimeSliderIncrease(cl, obj, (struct MUIP_Numeric_Increase*)msg));
		case MUIM_Numeric_Decrease: return(TimeSliderDecrease(cl, obj, (struct MUIP_Numeric_Decrease*)msg));
		case MUIM_Numeric_ScaleToValue: return(TimeSliderScaleToValue(cl, obj, (struct MUIP_Numeric_ScaleToValue*)msg));
		default:  return (DoSuperMethodA(cl, obj, msg));
	}
}

