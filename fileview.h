/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __FILEVIEW_H__
#define __FILEVIEW_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *FileViewClass;

struct MUI_CustomClass *CreateFileViewClass(VOID);
VOID DeleteFileViewClass(VOID);

#define MCC_FV_TAGBASE        (0x7EDB0000)
#define MCC_FV_ID(x)          (MCC_FV_TAGBASE + (x))

/* attrs */
#define FVA_FileData          MCC_FV_ID(0x0000)    /* [i..] APTR   */
#define FVA_FileDataSize      MCC_FV_ID(0x0001)    /* [i..] ULONG  */
#define FVA_TextContents      MCC_FV_ID(0x0002)    /* [i..] STRPTR */
#define FVA_TextPreparse      MCC_FV_ID(0x0003)    /* [i..] STRPTR */
#define FVA_FileName          MCC_FV_ID(0x0004)    /* [i..] STRPTR */

/* methods */
#define FVM_SaveFile          MCC_FV_ID(0x1000)
#define FVM_OpenFile          MCC_FV_ID(0x1001)

#endif /* __FILEVIEW_H__ */
