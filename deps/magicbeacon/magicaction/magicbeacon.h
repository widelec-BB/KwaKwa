#ifndef MAGICACTION_MAGICBEACON_H
#define MAGICACTION_MAGICBEACON_H

/*
** magicbeacon.h
**
** (c)2011 Guido Mersmann
**
** Application interface port description for MagicBeacon.
**
** MagicBeacon is a port driven notification mechanism using case
** insensitive English spelled names as signal types. The rule
** behind this is easy:
**
** "Application.Notification"
**
** The goal is, to be specific and flexible at the same time.
**
** Example: Two mail applications named MailA and MailB.
**
** Both support new mail notification. So MagicBeacon can be set to
** "MAILA.NEWMAIL" and "MAILB.NEWMAIL". Both tools are running at the
** same time cause different actions. If the user wants to handle a
** single notification for both applications to create the same action,
** for both events, he simply specifies "NEWMAIL".
**
** To avoid dealing with tons of wrong and crappy names, I deceided
** to predefine them here, so developers can take a look to get an
** idea which to pick and users get a little database of posibilities.
**
** If you have a cool idea let me know so the notification name can
** be registered.
**
*/
/*************************************************************************/

#include <exec/types.h>
#include <exec/ports.h>

/*************************************************************************/

#define MBNP_NAME "MagicBeaconNotification" /* this is the port name */

# pragma pack(2)

struct MagicBeaconNotificationMessage
{
	struct Message  mbnm_Message;
	ULONG           mbnm_NotificationMode;    /* must be set to NOTIFICATIONMODE_#? */

	char           *mbnm_NotificationName;    /* name of the sender. Same
											  ** name must be used in
											  ** MagicBeacon to catch the
											  ** beacon
											  */

	char           *mbnm_NotificationMessage; /* this is a textual message
											  ** which can contain
											  ** information for the
											  ** beaconing process (%m
											  ** usage)
											  */
	char           *mbnm_NotificationImage;   /* name of image to show in
											  ** bubble. Full path or just
											  ** a name like "WARNING" to
											  ** get a build in one
											  ** like "MEDIAINSERT"
											  */
	APTR            mbnm_UserData;            /* this field is free to use
											  ** for the sending application.
											  ** useful to track multiple
											  ** messages by an ID or store
											  ** other information along with
											  ** it. */
};

#define NOTIFICATIONMODE_QUICK     0 /* MagicBeacon will just take
										the notification and reply */
#define NOTIFICATIONMODE_GETRESULT 1 /* MagicBeacon will wait for user
										clicking on bubble and reply
										message with mbnm_Result set */

#define NOTIFICATIONRESULT_IGNORED   0 /* user ignored notification */
#define NOTIFICATIONRESULT_CONFIRMED 1 /* user confirmed notification */
#define NOTIFICATIONRESULT_CANCELED  2 /* user rejected notification */

# pragma pack()

/*************************************************************************/

/*
** macros making the live easier.
*/

#define MBNM_MESSAGESIZE sizeof( struct MagicBeaconNotificationMessage )

#define mbnm_Length    mbnm_Message.mn_Length     /* must be set to MBPM_MESSAGESIZE to stay
													 compatible with future extensions */
#define mbnm_ReplyPort mbnm_Message.mn_ReplyPort
#define mbnm_Result    mbnm_NotificationMode

/*************************************************************************/

/*
** standardized notification names
**
** The list will grow along with the ideas
*/

#define MBN_NEWMAIL           "NEWMAIL"           /* message contains sender of the mail */
#define MBN_NEWRSS            "NEWRSS"            /* message contains topic of the feed */
#define MBN_TRACKNEXT         "TRACKNEXT"         /* message contains title of track */
#define MBN_TRACKLAST         "TRACKLAST"         /* application specific message */
#define MBN_NEWPRIVATEMSG     "NEWPRIVATEMSG"     /* message contains chat line  */
#define MBN_NEWMSG            "NEWMSG"            /* message contains chat line */
#define MBN_CONNECTED         "CONNECTED"         /* message contains server name */
#define MBN_DISCONNECTED      "DISCONNECTED"      /* message contains server name */
#define MBN_CONNECTIONFAILED  "CONNECTIONFAILED"  /* message contains server name */
#define MBN_TRANSFERCOMPLETE  "TRANSFERCOMPLETE"  /* use this for completition of multiple transfers - no message ("") */
#define MBN_TRANSFERDONE      "TRANSFERDONE"      /* message contains file name or "" */
#define MBN_TRANSFERFAILED    "TRANSFERFAILED"    /* message contains file name or "" */
#define MBN_TRANSFERCANCELLED "TRANSFERCANCELLED" /* message contains file name or "" */
#define MBN_OPERATIONDONE     "OPERATIONDONE"     /* name of finished operation */
#define MBN_UPDATEAVAILABLE   "UPDATEAVAILABLE"   /* name of the available update */

/*************************************************************************/

#endif /* MAGICACTION_MAGICBEACON_H */
