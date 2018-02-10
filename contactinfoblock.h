/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __CONTACTINFOBLOCK_H__
#define __CONTACTINFOBLOCK_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *ContactInfoBlockClass;

struct MUI_CustomClass *CreateContactInfoBlockClass(void);
void DeleteContactInfoBlockClass(void);

/* tags */
#define CIBA_ContactEntry       0xCEDE0000

#endif /* __CONTACTINFOBLOCK_H__ */
