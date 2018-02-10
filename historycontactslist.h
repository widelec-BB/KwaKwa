/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __HISTORYCONTACTSLIST_H__
#define __HISTORYCONTACTSLIST_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *HistoryContactsListClass;

struct MUI_CustomClass *CreateHistoryContactsListClass(VOID);
VOID DeleteHistoryContactsListClass(VOID);

struct HContactsListEntry
{
	ULONG plugin_id;
	STRPTR id;
	STRPTR name;
};

/* methods */
#define HCLM_Remove         0x8EDC0000
#define HCLM_TitleClick     0x8EDC0001

/* attrs */
#define HCLA_SortOrder      0x8EDC8000

/* consts */
#define HCLV_SortOrder_None 0

#endif /* __HISTORYCONTACTSLIST_H__ */
