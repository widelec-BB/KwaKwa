/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __PICTUREVIEW_H__
#define __PICTUREVIEW_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *PictureViewClass;

struct MUI_CustomClass *CreatePictureViewClass(VOID);
VOID DeletePictureViewClass(VOID);

#define MCC_PV_TAGBASE        (0x6EDE0000)
#define MCC_PV_ID(x)          (MCC_PV_TAGBASE + (x))

/* attrs */
#define PVA_PictureData       MCC_PV_ID(0x0000)    /* [i..] APTR  */
#define PVA_PictureDataSize   MCC_PV_ID(0x0001)    /* [i..] ULONG */
#define PVA_PictureFilePath   MCC_PV_ID(0x0002)    /* [i..] STRPTR */
#define PVA_GlobalAlpha       MCC_PV_ID(0x0003)    /* [isg] ULONG */

/* methods */
#define PVM_SavePicture       MCC_PV_ID(0x1000)
#define PVM_OpenPicture       MCC_PV_ID(0x1001)

#endif /* __PICTUREVIEW_H__ */
