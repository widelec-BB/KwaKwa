/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __PERCENTAGE_SLIDER_H__
#define __PERCENTAGE_SLIDER_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *PercentageSliderClass;

struct MUI_CustomClass *CreatePercentageSliderClass(VOID);
VOID DeletePercentageSliderClass(VOID);

#endif /* __PERCENTAGE_SLIDER_H__ */
