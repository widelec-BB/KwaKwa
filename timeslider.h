/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __TIMESLIDER_H__
#define __TIMESLIDER_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *TimeSliderClass;

struct MUI_CustomClass *CreateTimeSliderClass(VOID);
VOID DeleteTimeSliderClass(VOID);

/* 0x6EDD XXXX TimeSliderClass */

#define TSA_OnlyCompleteValues     0x6EDD1000
#define TSA_MinText                0x6EDD1001

#endif /* __TIMESLIDER_H__ */
