/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __MINMAX_SLIDER_H__
#define __MINMAX_SLIDER_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *MinMaxSliderClass;

struct MUI_CustomClass *CreateMinMaxSliderClass(VOID);
VOID DeleteMinMaxSliderClass(VOID);

#define MCC_MMS_TAGBASE     (0x6EDF0000)
#define MCC_MMS_ID(x)       (MCC_MMS_TAGBASE + (x))

/* attrs */
#define MMSA_MinText        MCC_MMS_ID(0x0001)   /* [i..] STRPTR */
#define MMSA_MaxText        MCC_MMS_ID(0x0002)   /* [i..] STRPTR */
#define MMSA_Unit           MCC_MMS_ID(0x0003)   /* [i..] STRPTR */

#endif /* __MINMAX_SLIDER_H__ */
