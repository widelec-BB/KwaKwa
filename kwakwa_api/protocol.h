/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __KWAKWA_PROTOCOL_H__
#define __KWAKWA_PROTOCOL_H__

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif /* EXEC_TYPES_H */

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif /* EXEC_NODES_H */

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif /* EXEC_LISTS_H */

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif /* UTILITY_TAGITEM_H */

#ifndef UTILITY_DATE_H
#include <utility/date.h>
#endif /* UTILITY_DATE_H */

#ifndef INTUITION_CLASSUSR_H
#include <intuition/classusr.h>
#endif /* INTUITION_CLASSUSR_H */

#include "defs.h"
#include "pictures.h"
#include "contact.h"
#include "gui.h"

#define KWAKWA_GLOBAL_NUMBER  0x0EAB /* never use this for your modules if you wish to have own serial number send e-mail to widelec@morphos.pl */
#define KWAKWA_MODULE_ID(x)   (TAG_USER | (KWAKWA_GLOBAL_NUMBER << 16) | (x))

struct NotifyListEntry
{
	STRPTR nle_EntryID;
	ULONG  nle_Status;
};

/* methods */
#define KWAM_Connect          KWAKWA_MODULE_ID(0x0000)
#define KWAM_Disconnect       KWAKWA_MODULE_ID(0x0001)
#define KWAM_ChangeStatus     KWAKWA_MODULE_ID(0x0002)
#define KWAM_NotifyList       KWAKWA_MODULE_ID(0x0003)
#define KWAM_SendMessage      KWAKWA_MODULE_ID(0x0004)
#define KWAM_TypingNotify     KWAKWA_MODULE_ID(0x0005)
#define KWAM_AddNotify        KWAKWA_MODULE_ID(0x0006)
#define KWAM_RemoveNotify     KWAKWA_MODULE_ID(0x0007)
#define KWAM_WatchEvents      KWAKWA_MODULE_ID(0x0008)
#define KWAM_FreeEvents       KWAKWA_MODULE_ID(0x0009)
#define KWAM_TimedMethod      KWAKWA_MODULE_ID(0x000A)
#define KWAM_ImportList       KWAKWA_MODULE_ID(0x000B)
#define KWAM_ExportList       KWAKWA_MODULE_ID(0x000C)
#define KWAM_SendPicture      KWAKWA_MODULE_ID(0x000D)
#define KWAM_AnswerInvite     KWAKWA_MODULE_ID(0x000E)
#define KWAM_FetchContactInfo KWAKWA_MODULE_ID(0x000F)

/* attributes */
#define KWAA_Socket           KWAKWA_MODULE_ID(0x1000)   /* [..G] LONG v1.0 */
#define KWAA_ModuleID         KWAKWA_MODULE_ID(0x1001)   /* [..G] ULONG v1.0 */
#define KWAA_ProtocolName     KWAKWA_MODULE_ID(0x1002)   /* [..G] STRPTR (read only!) v1.0 */
#define KWAA_GuiTagList       KWAKWA_MODULE_ID(0x1003)   /* [..G] struct TagList* v1.0 */
#define KWAA_WantRead         KWAKWA_MODULE_ID(0x1004)   /* [..G] BOOL v1.0 */
#define KWAA_WantWrite        KWAKWA_MODULE_ID(0x1005)   /* [..G] BOOL v1.0 */
#define KWAA_AppObject        KWAKWA_MODULE_ID(0x1006)   /* [I..] Object* v1.0*/
#define KWAA_SocketSigMask    KWAKWA_MODULE_ID(0x1007)   /* [I..] ULONG v2.0 */
#define KWAA_UserID           KWAKWA_MODULE_ID(0x1008)   /* [..G] STRPTR v2.1 */

/* parameters */
struct KWAP_Connect           {ULONG MethodID; ULONG Status; STRPTR Description;};
struct KWAP_Disconnect        {ULONG MethodID; STRPTR Description;};
struct KWAP_ChangeStatus      {ULONG MethodID; ULONG Status; STRPTR Description;};
struct KWAP_NotifyList        {ULONG MethodID; ULONG EntriesNo; struct NotifyListEntry *Entries;};
struct KWAP_SendMessage       {ULONG MethodID; STRPTR ContactID; STRPTR Txt;};
struct KWAP_TypingNotify      {ULONG MethodID; STRPTR ContactID; LONG TxtLength;};
struct KWAP_AddNotify         {ULONG MethodID; struct NotifyListEntry *Entry;};
struct KWAP_RemoveNotify      {ULONG MethodID; struct NotifyListEntry *Entry;};
struct KWAP_WatchEvents       {ULONG MethodID; ULONG CanRead; ULONG CanWrite;};
struct KWAP_FreeEvents        {ULONG MethodID; struct MinList *Events;};
struct KWAP_HttpCallBack      {ULONG MethodID; ULONG DataLength; UBYTE *Data; APTR UserData;};
struct KWAP_ExportList        {ULONG MethodID; ULONG ContactsNo; struct ContactEntry *Contacts;};
struct KWAP_SendPicture       {ULONG MethodID; STRPTR ContactID; STRPTR Path;};
struct KWAP_AnswerInvite      {ULONG MethodID; STRPTR ContactID; ULONG Answer;};
struct KWAP_FetchContactInfo  {ULONG MethodID; STRPTR ContactID; Object *Req; ULONG ReqMethod;};

/* events */

/* types */
#define KE_TYPE_MODULE_MESSAGE      0x00000000
#define KE_TYPE_CONNECT             0x00000001
#define KE_TYPE_DISCONNECT          0x00000002
#define KE_TYPE_STATUS_CHANGE       0x00000003
#define KE_TYPE_LIST_CHANGE         0x00000004
#define KE_TYPE_NEW_MESSAGE         0x00000005
#define KE_TYPE_TYPING_NOTIFY       0x00000006
#define KE_TYPE_SEND_HTTP_GET       0x00000007
#define KE_TYPE_SEND_HTTP_POST      0x00000008
#define KE_TYPE_NEW_AVATAR          0x00000009
#define KE_TYPE_IMPORT_LIST         0x0000000A
#define KE_TYPE_EXPORT_LIST         0x0000000B
#define KE_TYPE_NEW_PICTURE         0x0000000C
#define KE_TYPE_NEW_INVITE          0x0000000D
#define KE_TYPE_NOTIFY_BEACON       0x0000000E

/* event message */

struct StatusChange
{
	ULONG  ke_NewStatus;
	STRPTR ke_Description;
};

struct ListChange
{
	STRPTR ke_ContactID;
	ULONG  ke_NewStatus;
	STRPTR ke_Description;
};

struct NewMessage
{
	STRPTR ke_ContactID;
	ULONG  ke_TimeStamp;
	STRPTR ke_Txt;
	ULONG  ke_Flags;
};

struct TypingNotify
{
	STRPTR ke_ContactID;
	ULONG  ke_TxtLen;
};

struct ModuleMessage
{
	ULONG  ke_Errno;
	STRPTR ke_MsgTxt;
};

struct HttpGet
{
	STRPTR ke_Url;
	STRPTR ke_UserAgent;
	ULONG  ke_MethodID;
	APTR   ke_UserData;
};

struct HttpPost
{
	STRPTR          ke_Url;
	STRPTR          ke_UserAgent;
	struct TagItem *ke_Data;
	ULONG           ke_DataItems;
	ULONG           ke_MethodID;
	APTR            ke_UserData;
};

struct NewAvatar
{
	STRPTR          ke_ContactID;
	struct Picture *ke_Picture;
};

struct ImportList
{
	ULONG                ke_ContactsNo;
	struct ContactEntry *ke_Contacts;
};

struct ExportList
{
	BOOL ke_Accepted;
};

struct NewPicture
{
	STRPTR ke_ContactID;
	ULONG  ke_TimeStamp;
	ULONG  ke_Flags;
	ULONG  ke_DataSize;
	APTR   ke_Data;
};

struct NewInvite
{
	STRPTR ke_ContactID;
	ULONG  ke_TimeStamp;
};

struct NotifyBeacon
{
	STRPTR ke_NotificationName;
	STRPTR ke_Message;
	ULONG  ke_WaitForResult;
	ULONG  ke_MethodID;
	APTR   ke_UserData;
};

struct KwaEvent
{
	struct MinNode ke_Node;
	ULONG ke_Type;
	ULONG ke_ModuleID;
	union
	{
		struct StatusChange  kec_StatusChange;
		struct ListChange    kec_ListChange;
		struct NewMessage    kec_NewMessage;
		struct TypingNotify  kec_TypingNotify;
		struct ModuleMessage kec_ModuleMessage;
		struct HttpGet       kec_HttpGet;
		struct HttpPost      kec_HttpPost;
		struct NewAvatar     kec_NewAvatar;
		struct ImportList    kec_ImportList;
		struct ExportList    kec_ExportList;
		struct NewPicture    kec_NewPicture;
		struct NewInvite     kec_NewInvite;
		struct NotifyBeacon  kec_NotifyBeacon;
	} ke_Content;
};

#define ke_StatusChange  ke_Content.kec_StatusChange
#define ke_ListChange    ke_Content.kec_ListChange
#define ke_NewMessage    ke_Content.kec_NewMessage
#define ke_TypingNotify  ke_Content.kec_TypingNotify
#define ke_ModuleMessage ke_Content.kec_ModuleMessage
#define ke_HttpGet       ke_Content.kec_HttpGet
#define ke_HttpPost      ke_Content.kec_HttpPost
#define ke_NewAvatar     ke_Content.kec_NewAvatar
#define ke_ImportList    ke_Content.kec_ImportList
#define ke_ExportList    ke_Content.kec_ExportList
#define ke_NewPicture    ke_Content.kec_NewPicture
#define ke_NewInvite     ke_Content.kec_NewInvite
#define ke_NotifyBeacon  ke_Content.kec_NotifyBeacon

#define AllocKwaEvent()    AllocMem(sizeof(struct KwaEvent), MEMF_PUBLIC | MEMF_CLEAR)
#define FreeKwaEvent(ptr)  FreeMem((ptr), sizeof(struct KwaEvent))

/*
 * NOTE: Please note that every field with timestamp should contain
 * an Amiga timestamp (number of seconds since Jan 1, 1978)
 * NOT a Unix timestamp (number of seconds since Jan 1, 1970). Also,
 * it should be an UTC time. Conversion to local time will be performed
 * by application. For conversion from Unix timestamp to Amiga timestamp
 * you can use UnixToAmigaTimestamp() macro (defined below).
 */
#define UnixToAmigaTimestamp(nix_timestamp) ((nix_timestamp) - 252460800UL)

/* errno codes */
#define ERRNO_ONLY_MESSAGE       0x00000000
#define ERRNO_OUT_OF_MEMORY      0x00000001
#define ERRNO_CONNECTION_FAILED  0x00000002
#define ERRNO_LOGIN_FAILED       0x00000003
#define ERRNO_NOT_SUPPORTED      0x00000004

#endif /* __KWAKWA_PROTOCOL_H__ */
