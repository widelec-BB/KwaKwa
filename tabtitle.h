/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
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

/* attrs */
#define TTA_Unread    0x8EDB0000   /* [.s.] BOOL */

#endif /* __TABTITLE_H__ */
