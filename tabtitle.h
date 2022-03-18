/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __TABTITLE_H__
#define __TABTITLE_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *TabTitleClass;

struct MUI_CustomClass *CreateTabTitleClass(VOID);
VOID DeleteTabTitleClass(VOID);

/* 0x8EDB XXXX TabTitle */
#define MCC_TT_TAGBASE            (0x8EDB0000)
#define MCC_TT_ID(x)              (MCC_TT_TAGBASE + (x))

/* attrs */
#define TTA_ContactName           MCC_TT_ID(0x0000)   /* [is.] STRPTR */
#define TTA_Unread                MCC_TT_ID(0x0001)   /* [is.] BOOL */
#define TTA_Status                MCC_TT_ID(0x0002)   /* [is.] ULONG */
#define TTA_ShowStatusImage       MCC_TT_ID(0x0003)   /* [is.] BOOL */

/* methods */
#define TTM_RefreshContents       MCC_PV_ID(0x1000)

#endif /* __TABTITLE_H__ */
