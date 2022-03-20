/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/****h* KwaKwaAPI/defs.h
 *
 *  NAME
 *    defs.h -- Constants definitions
 *
 *  AUTHOR
 *    Filip Maryjañski
 *
 *  DESCRIPTION
 *    This file contains definitions of constants used in KwaKwa.
 *
 ********/

#ifndef __KWAKWA_DEFS_H__
#define __KWAKWA_DEFS_H__

/****d* defs.h/KWA_STATUS_#?
 *
 *  NAME
 *    KWA_STATUS_#?
 *
 *  FUNCTION
 *    Constans used to set user status. Statuses are bit masks.
 *    For each of statuses is available macro to check status.
 *    First 8 bits stands for status type, next are for additional
 *    flags (like status description, only for friends, etc.).
 *
 *  SOURCE
 */

#define KWA_STATUS_FRESH             (0)
#define KWA_STATUS_NOT_AVAIL         (1 << 1)
#define KWA_STATUS_AVAIL             (1 << 2)
#define KWA_STATUS_BUSY              (1 << 3)
#define KWA_STATUS_INVISIBLE         (1 << 4)
#define KWA_STATUS_FFC               (1 << 5)
#define KWA_STATUS_DND               (1 << 6)
#define KWA_STATUS_BLOCKED           (1 << 7)
#define KWA_STATUS_HIDE              (1 << 10)


#define KWA_S(x)            ((x) & 0x000000FFUL)
#define KWA_S_FRESH(x)      ((x) == KWA_STATUS_FRESH)
#define KWA_S_NAVAIL(x)     ((x) & KWA_STATUS_NOT_AVAIL)
#define KWA_S_AVAIL(x)      ((x) & KWA_STATUS_AVAIL)
#define KWA_S_BUSY(x)       ((x) & KWA_STATUS_BUSY)
#define KWA_S_INVISIBLE(x)  ((x) & KWA_STATUS_INVISIBLE)
#define KWA_S_FFC(x)        ((x) & KWA_STATUS_FFC)
#define KWA_S_DND(x)        ((x) & KWA_STATUS_DND)
#define KWA_S_BLOCKED(x)    ((x) & KWA_STATUS_BLOCKED)

#define KWA_S_HIDE(x)       ((x) & KWA_STATUS_HIDE)

#define KWA_S_NORMAL(x)     (!KWA_S_HIDE(x) && !KWA_S_BLOCKED(x))

/******KWA_STATUS_#?******/

#define KWA_STATUS_DESC_MAX_SIZE 1024

/****d* defs.h/MSG_FLAG_#?
 *
 *  NAME
 *    MSG_FLAG_#?
 *
 *  FUNCTION
 *    Flags for incoming messages:
 *     MSG_FLAG_NORMAL -- normal incoming message;
 *     MSG_FLAG_MULTILOGON -- message was send from another client (parallel loged).
 *
 *  SOURCE
 */

#define MSG_FLAG_NORMAL       0x00000000
#define MSG_FLAG_MULTILOGON   0x00000001

/******MSG_FLAG_#?******/

/****d* defs.h/GENDER_#?
 *
 *  NAME
 *    MSG_FLAG_#?
 *
 *  FUNCTION
 *    Constants for contact's gender:
 *     GENDER_UNKNOWN -- gender is unknown;
 *     GENDER_MALE -- gender is male;
 *     GENDER_FEMALE -- gender is female.
 *
 *  SOURCE
 */

#define GENDER_UNKNOWN 0
#define GENDER_MALE 1
#define GENDER_FEMALE 2

/******GENDER_#?******/

#endif /* __KWAKWA_DEFS_H__ */
