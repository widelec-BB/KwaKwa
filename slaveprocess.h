/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __SLAVEPROCESS_H__
#define __SLAVEPROCESS_H__

#include <clib/alib_protos.h>
#include <intuition/classes.h>

extern Class *SlaveProcessClass;

Class *CreateSlaveProcessClass(void);
void DeleteSlaveProcessClass(void);

/* 0x6EDC XXXX SlaveProcessClass */

/* methods */
#define SPM_StartProcess      0x6EDC0000
#define SPM_KillProcess       0x6EDC0001
#define SPM_SignalMethod      0x6EDC0002
#define SPM_SendNotification  0x6EDC0003
#define SPM_FreeNotification  0x6EDC0004
#define SPM_SendHttpGet       0x6EDC0005
#define SPM_FreeHttpGet       0x6EDC0006
#define SPM_SendHttpPost      0x6EDC0007
#define SPM_FreeHttpPost      0x6EDC0008
#define SPM_SendFtpPut        0x6EDC0009
#define SPM_FreeFtpPut        0x6EDC000A

/* attrs */
#define SPA_SigBit            0x6EDC1000

#endif /* __SLAVEPROCESS_H__ */
