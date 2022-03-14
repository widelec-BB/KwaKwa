/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __DESCWINDOW_H__
#define __DESCWINDOW_H__


#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *DescWindowClass;

struct MUI_CustomClass *CreateDescWindowClass(void);
void DeleteDescWindowClass(void);

/* methods */
#define DWM_ChangeDesc              0xCEDC0000
#define DWM_ChangeListActive        0xCEDC0001
#define DWM_AddDescToList           0xCEDC0002
#define DWM_LoadActualDescription   0xCEDC0003
#define DWM_SearchNext              0xCEDC0004


/* objects */
#define USD_DESC_WINDOW                0xCEDC1000
#define USD_DESC_CYCLE                 0xCEDC1001
#define USD_DESC_STRING                0xCEDC1002
#define USD_DESC_ADDTOLIST_CHECK       0xCEDC1003
#define USD_DESC_SHOWLIST_CHECK        0xCEDC1004
#define USD_DESC_LIST                  0xCEDC1005
#define USD_DESC_SEARCH_NEXT_BUTTON    0xCEDC1006
#define USD_DESC_SEARCH_STRING         0xCEDC1007

#endif /* __DESCWINDOW_H__ */
