/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __HISTORYCONVERSATIONSLIST_H__
#define __HISTORYCONVERSATIONSLIST_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *HistoryConversationsListClass;

struct MUI_CustomClass *CreateHistoryConversationsListClass(VOID);
VOID DeleteHistoryConversationsListClass(VOID);

struct HConversationsListEntry
{
	QUAD id;
	STRPTR contact_id;
	STRPTR contact_name;
	STRPTR user_id;
	ULONG timestamp_start;
	ULONG timestamp_end;
	ULONG flags;
};

/* methods */
#define HCVLM_Remove       0x8EDD0000
#define HCVLM_TitleClick   0x8EDD0001

/* attrs */
#define HCVLA_SortOrder       0x8EDD8000
#define HCVLA_ClickedColumn   0x8EDD8001

#define HCVLV_SortOrder_None       0
#define HCVLV_ClickedColumn_None  -1

#endif /* __HISTORYCONVERSATIONSLIST_H__ */
