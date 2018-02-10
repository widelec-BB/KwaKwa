#ifndef __LIBRARIES_FTP_H__
#define __LIBRARIES_FTP_H__

#ifndef  EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef DEVICES_TIMER
#include <devices/timer.h> 
#endif

# include <sys/types.h>
# include <time.h>

#define	MIN_FTPVERSION	(51UL)

#define FTPLIB_FILE_FILE		1
#define FTPLIB_FILE_DIR			2
#define FTPLIB_FILE_LINK		3

#define FTPLIB_MODE_LIST		1
#define FTPLIB_MODE_DIR			2

#define FTPLIB_BUFSIZ           16284
//65536
#define FTPLIB_CONTROL			0
#define FTPLIB_READ				1
#define FTPLIB_WRITE			2
#define FTPLIB_STRBUFSIZ        16284

//
#define FTPLIB_LIST				0x000000a1
#define FTPLIB_DIR				0x000000a2
#define FTPLIB_FILE_READ		0x000000a3
#define FTPLIB_FILE_WRITE		0x000000a4

// codes
#define FTPLIB_ASCII			'A'
#define FTPLIB_BINARY			'I'

//
#define FTPLIB_PASSIVE			1
#define FTPLIB_PORT				2


// errors

#define FTPLIB_ERROR_CANNOTOPENLIB			1
#define FTPLIB_ERROR_WRONGPORT				2
#define FTPLIB_ERROR_HOSTNAME				3
#define FTPLIB_ERROR_CANNOTCREATESOCKET		4
#define FTPLIB_ERROR_SETSOCKOPTION			5
#define FTPLIB_ERROR_CONNECTTOHOST			6
#define FTPLIB_ERROR_ALLOCINTMEMORY			7
#define FTPLIB_ERROR_FTPCONNECTION			8


// ftp connection

#define FTP_TAG_LOGMSG						0x10000001
#define	FTP_TAG_TIMEOUT						0x10000002
#define FTP_TAG_PROGRESSMSG					0x10000003
#define FTP_TAG_CONNMODE					0x100f0004
#define FTP_TAG_TRANSFER_INFO_HOOK			0x100f0005
#define FTP_TAG_CALLBACK					0x100f0006
#define FTP_TAG_IDLETIME					0x100f0007
#define FTP_TAG_CALLBACKARG					0x100f0008
#define FTP_TAG_CALLBACKBYTES				0x100f0009
#define FTP_TAG_REFRESHEVERY				0x100f0010
#define FTP_TAG_GROUP_CLASS					0x100f0011
#define FTP_TAG_LOGPORT						0x100f0012
#define FTP_TAG_ABORT						0x100f0013

// progress message

#ifndef __PROGRESS_LOG_MESSAGE__
#define __PROGRESS_LOG_MESSAGE__

struct ProgressLogMsg
{ 
	struct Message msg; 
	UQUAD tr_Bytes;
};

#endif

// log message
#ifndef __REMOTE_LOG_MESSAGE__
#define __REMOTE_LOG_MESSAGE__

struct RemoteLogMsg
{
	struct Message  msg;	// MUST be the first thing in the message
	ULONG Code;
	BYTE LogMessage[100];
};
#endif

typedef int (*FtpCallback)( void *nControl, int xfered, void *arg);


struct FTPHandler
{
	UBYTE				Response[ FTPLIB_STRBUFSIZ ];
	struct Library		*socketBase;
	struct timeval		Idletime;
	struct Hook			*Callback;
    APTR				Idlearg;
    FtpCallback			Idlecb;

	UQUAD				Cavail;
	UQUAD				Cleft;
	int					Handle;				// Socket
	int					AcceptTimeout;
	int					Dir;
	int					FTPMode;
	int					StopTransfer;
	int					Port;
	unsigned int		Saddr;
	STRPTR				Rvs;
	STRPTR				Cput;
	STRPTR				Cget;
	STRPTR				Buf;
	struct FTPHandler		*Ctrl;
	struct FTPHandler		*Data;
	STRPTR				Path;			// pointer to host path

	LONG				Transfered;
	LONG				Transfered1;
	LONG				Cbbytes;

	LONG				RefreshTime;

	STRPTR				ReadBuf;

	UQUAD				tr_BytesReaded;
	ULONG				tr_FilesReaded;
	ULONG				tr_AllFiles;
	UQUAD				tr_AllBytes;
	UQUAD				tr_ActBytes;
	UQUAD				tr_ReadedFilesBytes;
	ULONG				tr_ReadedFiles;
	struct timeval		tr_DateTimeTransferStart;			 // date when Transfert Start

	//struct infoMessage	im;
	ULONG				GroupClass;
	struct MsgPort		*LogReplyPort;
	struct MsgPort		*LogSendPort;
	struct RemoteLogMsg	LogMsg;
	
	struct MsgPort		*ProgressReplyPort;
	struct MsgPort		*ProgressSendPort;
	struct ProgressLogMsg	ProgressMsg;
	
	int Error;											// if values > -1 then error occure
};

// file structure

#ifndef __REMOTE_FILE__
#define __REMOTE_FILE__

struct RemoteFile
{
	STRPTR		Name;
	UBYTE		Access[ 10 ];
	UBYTE		Date[ 20 ];
	time_t		DateTime;
	UQUAD		Size;
	LONG		Type;
	STRPTR		Link;

	struct	RemoteFile *NextFile;
};
#endif


#endif /* LIBRARIES_FTP_H */

