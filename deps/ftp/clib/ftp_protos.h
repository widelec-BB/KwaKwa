

#ifndef __FTPPROTOS_H__
#define __FTPPROTOS_H__

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef  UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif
#ifndef  __LIBRARIES_FTP_H__
#include <libraries/ftp.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

//functions

//struct FTPHandler *FTPInit( struct LibBase  *FTPBase );
STRPTR FTPGetLastMessage( struct FTPHandler *nControl );
struct FTPHandler *FTPConnect( CONST STRPTR host, int port, struct TagItem *taglist );
LONG FTPLogin( struct FTPHandler *nControl, CONST STRPTR user, CONST STRPTR pass );
LONG FTPAccess( struct FTPHandler *nControl, struct FTPHandler **nData, STRPTR path, LONG typ, LONG mode, UQUAD startPos );
int FTPRead( struct FTPHandler *nData, APTR buf, LONG max );
int FTPWrite( struct FTPHandler *nData, APTR buf, LONG len );
LONG FTPClose( struct FTPHandler *nData);
LONG FTPSite( struct FTPHandler *nControl, CONST STRPTR cmd );
STRPTR FTPSysType( struct FTPHandler *nControl, STRPTR buf, LONG max );
LONG FTPMakeDir( struct FTPHandler *nControl, CONST STRPTR path );
LONG FTPChdir( struct FTPHandler *nControl, CONST STRPTR path );
LONG FTPCDUp( struct FTPHandler *nControl );
LONG FTPDeleteDir( struct FTPHandler *nControl, CONST STRPTR path );
STRPTR FTPGetPath( struct FTPHandler *nControl, STRPTR path, LONG max );
struct RemoteFile *FTPList( struct FTPHandler *nControl, CONST STRPTR path );
struct RemoteFile *FTPDir( struct FTPHandler *nControl, CONST STRPTR path, BOOL showHidden );
LONG FTPSize( struct FTPHandler *nControl, CONST STRPTR fName, UQUAD *size );
LONG FTPModDate( struct FTPHandler *nControl, CONST STRPTR path, STRPTR dt, LONG max );
LONG FTPReceive( struct FTPHandler *nControl, CONST STRPTR dstFile, STRPTR srcFile, UBYTE mode, UQUAD startPos );
LONG FTPSend( struct FTPHandler *nControl, CONST STRPTR srcFile, CONST STRPTR dstFile, UBYTE mode, UQUAD startPos );
LONG FTPRename( struct FTPHandler *nControl, CONST STRPTR src, CONST STRPTR dst );
LONG FTPDeleteFile( struct FTPHandler *nControl, CONST STRPTR fnm );
LONG FTPDeleteDir( struct FTPHandler *nControl, CONST STRPTR fnm );
LONG FTPQuit( struct FTPHandler *nControl);
LONG FTPNoop( struct FTPHandler *ftpCtrl );
STRPTR FTPHelp( struct FTPHandler *ftpCtrl , STRPTR ret );
STRPTR FTPFeat( struct FTPHandler *ftpCtrl );
LONG FTPSet( struct FTPHandler *ftpCtrl, struct TagItem *taglist );
LONG FTPGet( struct FTPHandler *ftpCtrl, struct TagItem *taglist );

void FTPDeleteFileList( struct RemoteFile *f );


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FTPPROTOS_H */

