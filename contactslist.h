/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __CONTACTSLIST_H__
#define __CONTACTSLIST_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include "kwakwa_api/contact.h"

extern struct MUI_CustomClass *ContactsListClass;

struct MUI_CustomClass *CreateContactsListClass(VOID);
VOID DeleteContactsListClass(VOID);

/* objects */
#define USD_CONTACTS_LIST  0x8EDA0000

/* methods */
#define CLSM_InsertSingle  0x8EDA1000
#define CLSM_Remove        0x8EDA1001
#define CLSM_GetEntry      0x8EDA1002
#define CLSM_Clear         0x8EDA1003
#define CLSM_Sort          0x8EDA1004
#define CLSM_Compare       0x8EDA1005

#define CLSM_DoubleClick         0x8EDA1100
#define CLSM_EditContact         0x8EDA1101
#define CLSM_SetAllUnavail       0x8EDA1102
#define CLSM_CheckUnread         0x8EDA1103
#define CLSM_ExportToFile        0x8EDA1104
#define CLSM_ImportFromFile      0x8EDA1105
#define CLSM_NewAvatar           CLSM_RedrawAll /* TODO: it suxx */
#define CLSM_RemoveClones        0x8EDA1107
#define CLSM_SaveList            0x8EDA1108
#define CLSM_ReadList            0x8EDA1109
#define CLSM_RemoveEntry         0x8EDA110A
#define CLSM_OpenLogFile         0x8EDA110B
#define CLSM_FindEntry           0x8EDA110C

/* privates */
#define AVATAR_SIZE              50

#define CLSM_Scroll              0x8EDA3000
#define CLSM_DrawActive          0x8EDA3001
#define CLSM_DrawContextMenu     0x8EDA3002
#define CLSM_SelectEntryByName   0x8EDA3003
#define CLSM_RedrawAll           0x8EDA3004

/* attrs */
#define CLSA_Prop_Gadget     0x8EDA2000
#define CLSA_Search_String   0x8EDA2001
#define CLSA_List_Entries    0x8EDA2100
#define CLSA_Active          0x8EDA2102 /* little difference between CL and MUIC_List: in CL we store here pointer to entry, not index */

/* consts */
#define CLSV_Insert_Top        0
#define CLSV_Insert_Sorted    -2
#define CLSV_Insert_Bottom    -3

#define CLSV_Remove_First      0
#define CLSV_Remove_Active    -1
#define CLSV_Remove_Last      -2

#define CLSV_GetEntry_Active  -1

#define CLSV_FindEntry_Mode_ID    0
#define CLSV_FindEntry_Mode_Name  1

#endif
