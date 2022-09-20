/*	V4IS.C - V4 Index Sequential Modules

	Created 30-Mar-92 by Victor E. Hansen		*/

#define NEED_SOCKET_LIBS 1
#ifndef NULLID
#include "v4defs.c"
#endif
#include <time.h>

#ifdef SIGNALS
#include <signal.h>
sigset_t oldsigset ;						/* List of block signals (see v4is_Put) */
int resetsigset = FALSE ;					/* Flag to determine if above has been changed */
#endif

#ifdef WINNT
#include <windows.h>
#endif

//extern struct V4MM__MemMgmtMaster *V4_GLOBAL_MMM_PTR ;		/* Point to master area structure */
extern struct V4C__ProcessInfo *gpi ;	/* Global process information */

/*	Set up some abbreviations to make code more readable/managable	*/

#define UPD V4MM_BktPtr_Update
#define IDX V4MM_LockIdType_Index
#define DAT V4MM_LockIdType_Data
#define MEM V4MM_BktPtr_MemLock
#define REREAD V4MM_BktPtr_ReRead
#define IGNORELOCK V4MM_BktPtr_IgnoreLock
#define LOCKROOT lrx = v4mm_LockSomething(aid,V4MM_LockId_Root,V4MM_LockMode_Write,V4MM_LockIdType_Root,10,NULL,8)
#define RELROOT(WHERE) v4mm_LockRelease(aid,lrx,mmm->MyPid,aid,WHERE) ; v4mm_TempLock(aid,0,FALSE)
#define RELIDX \
  if (mmm->Areas[ax].LockIDX >= 0) { v4mm_LockRelease(aid,mmm->Areas[ax].LockIDX,mmm->MyPid,aid,"RELIDX") ; mmm->Areas[ax].LockIDX=UNUSED ; } ;
#define RELDAT \
  if (mmm->Areas[ax].LockDAT >= 0) { v4mm_LockRelease(aid,mmm->Areas[ax].LockDAT,mmm->MyPid,aid,"RELDAT") ; mmm->Areas[ax].LockDAT=UNUSED ; } ;
#define RELTREE \
  if (mmm->Areas[ax].LockTree>=0) { v4mm_LockRelease(aid,mmm->Areas[ax].LockTree,mmm->MyPid,aid,"RELTREE") ; mmm->Areas[ax].LockTree=UNUSED ; } ;
typedef struct V4FFI__KeyInfoDtl KID ;
typedef union V4IS__KeyPrefix KP ;
typedef struct V4IS__IndexBktKeyList IBKL ;
typedef struct V4IS__IndexKeyDataEntry IKDE ;
typedef struct V4IS__Key KEY ;
typedef struct V4IS__IndexBktHdr IBH ;
typedef struct V4__BktHdr BH ;
typedef struct V4IS__DataBktHdr DBH ;

/*	v4is_Put - Writes out a record via current PCB		*/
/*	Call: v4is_Put( pcb , errmsg )
	  where pcb is point to IS control block - struct V4IS__ParControlBlk,
	  errmsg if not null is updated with error message & return is FALSE	*/

LOGICAL v4is_Put(pcb,errmsg)
  struct V4IS__ParControlBlk *pcb ;
  UCCHAR *errmsg ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__GlobalBuf *glb ;
#ifdef SIGNALS
  sigset_t putset ;
#endif
  struct V4IS__AreaCB *acb ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__Bkt *bkt ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct {
    union V4IS__KeyPrefix kp ;
    char KeyVal[V4IS_KeyBytes_Max] ;
   } lclkey ;
  struct V4IS__Key *keyptr = NULL ;
  static char *cmpbuf = 0 ;			/* Buffer for data compression */
#ifdef V4_BUILD_CS_FUNCTIONALITY
  static struct V4CS__Msg_Put *mput = NULL ;	/* Put record (various modes) */
  struct V4CS__Msg_Error err ;
#endif
  union V4MM__DataIdMask dim ;
  char *bufptr,*origbufptr ; int kx,buflen,nbuflen,mode,cmpmode,datamode,fileref,keynum ;
  int i,aid,ax,res,dataid,pid ;

	pcb->PutCount ++ ;
/*	Decode put buffer & length */
	if (pcb->PutBufPtr != NULL) { bufptr = pcb->PutBufPtr ; pcb->PutBufPtr = NULL ; } else { bufptr = pcb->DfltPutBufPtr ; } ;
	if (pcb->PutBufLen != 0) { buflen = pcb->PutBufLen ; pcb->PutBufLen = 0 ; } else { buflen = pcb->DfltPutBufLen ; } ;
	if (pcb->AreaId == V4IS_AreaId_SeqOnly)			/* Handle sequential only */
	 {
#ifdef SIGNALS
	   sigemptyset(&putset) ; sigaddset(&putset,SIGINT) ;	/* Create signal set to block SIGINT */
	   sigaddset(&putset,SIGTERM) ;				/*   and SIGTERM */
	   sigprocmask(SIG_BLOCK,&putset,&oldsigset) ;		/* Block signal for duration of v4is_Put */
	   resetsigset = TRUE ;					/* Remember that we did this */
#endif
#ifdef WINNT
	   WriteFile(pcb->hfile,&buflen,sizeof buflen,&i,NULL) ;
	   WriteFile(pcb->hfile,&bufptr,buflen,&i,NULL) ;
#else
	   fwrite(&buflen,sizeof buflen,1,pcb->FilePtr) ;	/* First write out number of bytes in record */
	   fwrite(bufptr,buflen,1,pcb->FilePtr) ;		/*  then the actual data */
#endif
#ifdef SIGNALS
	   if (resetsigset)					/* Do we have to reset set of blocked signals ? */
	    { resetsigset = FALSE ; sigprocmask(SIG_SETMASK,&oldsigset,NULL) ; } ;
#endif
	   return(TRUE) ;
	 } ;
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
	aid = pcb->AreaId ;
	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return(FALSE) ;			/* Could not find area? */
//	for(ax=0;ax<mmm->NumAreas;ax++) { if (mmm->Areas[ax].AreaId == pcb->AreaId) break ; } ;
//	if (ax >= mmm->NumAreas) return(FALSE) ;		/* Could not find area? */
	acb = v4mm_ACBPtr(aid) ;
	if (mmm->Areas[ax].SegId != 0)				/* Global area ? */
	 { if ( GETGLB(ax) == NULL)
	    v4_error(V4E_AREANOTGLOBAL,0,"V4MM","Put","AREANOTGLOBAL","Area (%d %s) not global",ax,UCretASC(mmm->Areas[ax].UCFileName)) ;
	   for(;glb->PanicLockout > 0;)				/* Another process in panic mode ? */
	    { /* time(&timer) ; printf("%.19s -  Process %d in lockout mode from v4is_Put (%d) by pid=%d\n",
				ctime(&timer),mmm->MyPid,glb->PanicLockout,glb->PanicPid) ; */
	      if (glb->PanicLockout == V4IS_Lockout_Wait) { HANGLOOSE(250) ; continue ; } ;
	      if (glb->PanicLockout == V4IS_Lockout_WaitReadOK) { HANGLOOSE(250) ; continue ; } ;
	      if (glb->PanicLockout == V4IS_Lockout_Error)
	 	v4_error(V4E_LOCKOUTERR,0,"V4IS","Put","LOCKOUTERR","Cannot PUT to area (%d %s) - Area has been locked-out",
			ax,UCretASC(mmm->Areas[ax].UCFileName)) ;
	      HANGLOOSE(250) ; glb->PanicLockout-- ;
	    } ;
	 } ;
	if (acb->RootInfo->NumLevels >= V4IS_AreaCB_LvlMax - 2)
	 v4_error(V4E_PUTTOOMANYLVLS,0,"V4IS","Put","PUTTOOMANYLVLS","Cannot PUT to area (%d %s) - Must reformat index",
			ax,UCretASC(mmm->Areas[ax].UCFileName)) ;
	if (pcb->KeyNum != 0) { keynum = pcb->KeyNum-1 ; pcb->KeyNum = 0 ; }
	 else { keynum = (pcb->DfltKeyNum > 0 ? pcb->DfltKeyNum-1 : pcb->DfltKeyNum) ; } ;
/*	Figure out how we are to PUT this record */
	if (pcb->PutMode != 0) { mode = pcb->PutMode ; pcb->PutMode = 0 ; } else { mode = pcb->DfltPutMode ; } ;
	if (pcb->OpenMode == V4IS_PCB_OM_NewAppend)
	 { if (mode != V4IS_PCB_GP_Append)
	    v4_error(V4E_PUTAPND,0,"V4IS","Put","PUTAPND","Append is only Put mode allowed for this area (%s)",UCretASC(pcb->UCFileName)) ;
	 } ;
	switch (0xFFFF & mode)
	 { default:
/*		Maybe do some data compression ? */
		origbufptr = bufptr ;					/* Save original for uncompressed key */
		if ((mode & V4IS_PCB_CmpOnOverload) ? buflen > acb->RootInfo->MaxRecordLen
				: ((!(mode & V4IS_PCB_NoDataCmp)) && buflen >= acb->MinCmpBytes) )
		 { if (cmpbuf == NULL) cmpbuf = (char *)v4mm_AllocChunk(V4IS_BktSize_Max+12000,FALSE) ;
		   if (buflen < V4IS_DataCmp_Mth2MinSize)
		    { nbuflen = data_compress(cmpbuf,bufptr,buflen,0,V4IS_BktSize_Max+12000) ;
		      if (nbuflen < buflen)				/* Make sure compressed is actually smaller! */
		       { buflen = nbuflen ; bufptr = (char *)cmpbuf ; cmpmode = V4IS_DataCmp_Mth1 ; }
		       else cmpmode = V4IS_DataCmp_None ;
		    } else
		    { lzrw1_compress(bufptr,buflen,cmpbuf,&nbuflen) ;
		      if (nbuflen < buflen)				/* Make sure compressed is actually smaller! */
		       { double pc ;
		         pc = (double)nbuflen * 100.0 / (double)acb->RootInfo->MaxRecordLen ;
		         if (pcb->MaxBlockFillPC < pc) pcb->MaxBlockFillPC = pc ;
			 buflen = nbuflen ; bufptr = (char *)cmpbuf ; cmpmode = V4IS_DataCmp_Mth2 ; }
		       else cmpmode = V4IS_DataCmp_None ;
		    } ;
		 } else { cmpmode = V4IS_DataCmp_None ; } ;
		if (buflen > acb->RootInfo->MaxRecordLen)
		 { if (errmsg == NULL)
		    v4_error(V4E_MAXAREALEN,0,"V4IS","Put","MAXAREALEN","Record length exceeds maximum allowed for this area (%d > %d)",
			buflen,acb->RootInfo->MaxRecordLen) ;
		   v_Msg(NULL,errmsg,"V4ISRecSize",buflen,pcb->UCFileName,acb->RootInfo->MaxRecordLen) ;
		   return(FALSE) ;
		 } ;
		break ;
	   case V4IS_PCB_GP_Delete:
	   case V4IS_PCB_GP_Unlock:	/* Don't need record for these modes */
		cmpmode = V4IS_DataCmp_None ;
		break ;
	 } ;
	if (pcb->FileRef != 0) { fileref = pcb->FileRef ; pcb->FileRef = 0 ; } else { fileref = pcb->DfltFileRef ; } ;
	if (pcb->DataMode != 0) { datamode = pcb->DataMode ; pcb->DataMode = 0 ; } else { datamode = pcb->DfltDataMode ; } ;

#ifdef SIGNALS
	sigemptyset(&putset) ; sigaddset(&putset,SIGINT) ;	/* Create signal set to block SIGINT */
	   sigaddset(&putset,SIGTERM) ;				/*   and SIGTERM */
	sigprocmask(SIG_BLOCK,&putset,&oldsigset) ;		/* Block signal for duration of v4is_Put */
	resetsigset = TRUE ;					/* Remember that we did this */
#endif

try_again:							/* Here to re-enter Update/Insert from Write */
	switch((0xFFFF & mode))
	 { default: v4_error(V4E_INVPUTMODE,0,"V4IS","Put","INVPUTMODE","Invalid PUT mode on %s (%d)",UCretASC(pcb->UCFileName),mode) ;
	   case V4IS_PCB_GP_Obsolete:
		if (!(acb->AccessMode & V4IS_PCB_AM_Delete))
		 v4_error(V4E_AREANODELETE,0,"V4IS","Put","AREANODELETE","Delete operation not permitted for this area (%s)",UCretASC(pcb->UCFileName)) ;
#ifdef V4_BUILD_CS_FUNCTIONALITY
		if (acb->CSaid != UNUSED)			/* Going off to a remote server ? */
		 { if (mput == NULL) mput = (struct V4CS_MsgPut *)v4mm_AllocChunk(sizeof *mput,FALSE) ;
		   mput->mh.MsgType = V4CS_Msg_Put ; mput->mh.Flags = 0 ;
		   mput->PutMode = V4CS_PutMode_Obsolete ; mput->CmpMode = cmpmode ; mput->PutBytes = buflen ;
		   memcpy(&mput->DataBuffer,bufptr,buflen) ; mput->AreaId = acb->CSaid ;
		   mput->mh.Bytes = (char *)( (char *)&mput->DataBuffer[buflen] ) - (char *)mput ;
		   if ((i = send(acb->CSSocket,mput,mput->mh.Bytes,0)) < mput->mh.Bytes)
		    v4_error(V4E_SENDERR,0,"V4IS","GET","SENDERR",
				"Error (%s) (len = %d/%d) in send on socket (%d)",v_OSErrString(NETERROR),mput->mh.Bytes,i,UCretASC(pcb->UCFileName)) ;
		   if ((i = recv(acb->CSSocket,&err.mh,sizeof err.mh,0)) < 0)
		    v4_error(V4E_RECVERR,err.VStdErrNum,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
		   switch (err.mh.MsgType)
		    { default:
			v4_error(V4E_UNEXPSRVMSG,0,"V4IS","GET","UNEXPSRVMSG","Unexpected server message (%d) on Get to file (%s)",
				err.mh.MsgType,UCretASC(pcb->UCFileName)) ;
		      case V4CS_Msg_Error:		/* Got an error, read remainder of message */
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","PUT","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			v4_error(V4E_SRVERR,err.VStdErrNum,"V4IS","PUT","SRVERR",err.Message) ;
		      case V4CS_Msg_PutOK:
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","PUT","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			break ;
		    } ;
		   return(TRUE) ;
		 } ;
#endif
		if (mmm->Areas[ax].LockRec == UNUSED && mmm->Areas[ax].SegId != 0)
		 v4_error(V4E_RECNOTLOCKED,0,"V4IS","Put","RECNOTLOCKED","Area (%d %s) - cannot update without prior lock", ax,UCretASC(pcb->UCFileName)) ;
		if (pcb->DataId == -1)
		 v4_error(V4E_NORECTODEL,0,"V4IS","Obsolete","NORECTODEL","No current record to delete") ;
		v4is_ObsoleteData(pcb->AreaId,pcb->DataId) ;
		break ;
	   case V4IS_PCB_GP_Reset:
		if (acb->HashInfo != NULL)				/* Got a hash file? */
		 { dataid = v4h_PutHash(pcb,acb,keyptr,origbufptr,V4H_PutHash_Reset) ;
		   pcb->DataId = dataid ;
		   break ;
		 } ;
#ifdef V4_BUILD_CS_FUNCTIONALITY
		if (acb->CSaid != UNUSED)			/* Going off to a remote server ? */
		 { if (mput == NULL) mput = (struct V4CS_MsgPut *)v4mm_AllocChunk(sizeof *mput,FALSE) ;
		   mput->mh.MsgType = V4CS_Msg_Put ; mput->mh.Flags = 0 ;
		   mput->PutMode = V4CS_PutMode_Reset ; mput->CmpMode = cmpmode ; mput->PutBytes = buflen ;
		   memcpy(&mput->DataBuffer,bufptr,buflen) ; mput->AreaId = acb->CSaid ;
		   mput->mh.Bytes = (char *)( (char *)&mput->DataBuffer[buflen] ) - (char *)mput ;
		   if ((i = send(acb->CSSocket,mput,mput->mh.Bytes,0)) < mput->mh.Bytes)
		    v4_error(V4E_SENDERR,0,"V4IS","GET","SENDERR",
				"Error (%s) (len = %d/%d) in send on socket (%d)",v_OSErrString(NETERROR),mput->mh.Bytes,i,UCretASC(pcb->UCFileName)) ;
		   if ((i = recv(acb->CSSocket,&err.mh,sizeof err.mh,0)) < 0)
		    v4_error(V4E_RECVERR,err.VStdErrNum,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
		   switch (err.mh.MsgType)
		    { default:
			v4_error(V4E_UNEXPSRVMSG,0,"V4IS","GET","UNEXPSRVMSG","Unexpected server message (%d) on Get to file (%s)",
				err.mh.MsgType,UCretASC(pcb->UCFileName)) ;
		      case V4CS_Msg_Error:		/* Got an error, read remainder of message */
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","PUT","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			v4_error(V4E_SRVERR,err.VStdErrNum,"V4IS","PUT","SRVERR",err.Message) ;
		      case V4CS_Msg_PutOK:
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","PUT","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			break ;
		    } ;
		   return(TRUE) ;
		 } ;
#endif
		v4_error(V4E_AREANOTHASH1,0,"V4IS","Put","AREANOTHASH1","Area (%s) not hashed, RESET put mode not supported",UCretASC(pcb->UCFileName)) ;
	   case V4IS_PCB_GP_Cache:
		if (acb->HashInfo != NULL)				/* Got a hash file? */
		 { dataid = v4h_PutHash(pcb,acb,keyptr,origbufptr,V4H_PutHash_Cache) ;
		   pcb->DataId = dataid ;
		   break ;
		 } ;
#ifdef V4_BUILD_CS_FUNCTIONALITY
		if (acb->CSaid != UNUSED)			/* Going off to a remote server ? */
		 { if (mput == NULL) mput = (struct V4CS_MsgPut *)v4mm_AllocChunk(sizeof *mput,FALSE) ;
		   mput->mh.MsgType = V4CS_Msg_Put ; mput->mh.Flags = 0 ;
		   mput->PutMode = V4CS_PutMode_Cache ; mput->CmpMode = cmpmode ; mput->PutBytes = buflen ;
		   memcpy(&mput->DataBuffer,bufptr,buflen) ; mput->AreaId = acb->CSaid ;
		   mput->mh.Bytes = (char *)( (char *)&mput->DataBuffer[buflen] ) - (char *)mput ;
		   if ((i = send(acb->CSSocket,mput,mput->mh.Bytes,0)) < mput->mh.Bytes)
		    v4_error(V4E_SENDERR,0,"V4IS","GET","SENDERR",
				"Error (%s) (len = %d/%d) in send on socket (%d)",v_OSErrString(NETERROR),mput->mh.Bytes,i,UCretASC(pcb->UCFileName)) ;
		   if ((i = recv(acb->CSSocket,&err.mh,sizeof err.mh,0)) < 0)
		    v4_error(V4E_RECVERR,err.VStdErrNum,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
		   switch (err.mh.MsgType)
		    { default:
			v4_error(V4E_UNEXPSRVMSG,0,"V4IS","GET","UNEXPSRVMSG","Unexpected server message (%d) on Get to file (%s)",
				err.mh.MsgType,UCretASC(pcb->UCFileName)) ;
		      case V4CS_Msg_Error:		/* Got an error, read remainder of message */
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","PUT","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			v4_error(V4E_SRVERR,err.VStdErrNum,"V4IS","PUT","SRVERR",err.Message) ;
		      case V4CS_Msg_PutOK:
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","PUT","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			break ;
		    } ;
		   return(TRUE) ;
		 } ;
#endif
		v4_error(V4E_AREANOTHASH2,0,"V4IS","Put","AREANOTHASH2","Area (%s) not hashed, CACHE put mode not supported",UCretASC(pcb->UCFileName)) ;
	   case V4IS_PCB_GP_Append:
		keyptr = (KEY *)origbufptr ;	/* First assume key is at begin of record */
		if (fileref != 0) keyptr = v4is_MakeFFKey(acb,fileref,&lclkey,0,NULL,origbufptr,TRUE) ;
		if (v4is_FFKeyCount(acb,fileref) > 1)
		 { if (datamode == V4IS_PCB_DataMode_Index)
		    v4_error(V4E_PUTIDXMLTKEY,0,"V4IS","Put","PUTIDXMLTKEY","Cannot PUT data in index bucket with multiple keys (%s)",
				UCretASC(pcb->UCFileName)) ;
		    datamode = V4IS_PCB_DataMode_Data ;			/* Force data into data bucket */
		 } ;
		v4is_Append(aid,keyptr,bufptr,buflen,datamode,cmpmode,
				(fileref == 0 ? FALSE : (v4is_FFKeyCount(acb,fileref) > 1 ? TRUE : FALSE)),0) ;
		acb = v4mm_ACBPtr(aid) ; acb->CurPosKeyIndex++ ;	/* Increment Current Key Position */
		break ;
	   case V4IS_PCB_GP_Unlock:
#ifdef V4_BUILD_CS_FUNCTIONALITY
		if (acb->CSaid != UNUSED)			/* Going off to a remote server ? */
		 { if (mput == NULL) mput = (struct V4CS_MsgPut *)v4mm_AllocChunk(sizeof *mput,FALSE) ;
		   mput->mh.MsgType = V4CS_Msg_Put ; mput->mh.Flags = 0 ;
		   mput->PutMode = V4CS_PutMode_Unlock ; mput->CmpMode = cmpmode ; mput->PutBytes = buflen ;
		   memcpy(&mput->DataBuffer,bufptr,buflen) ; mput->AreaId = acb->CSaid ;
		   mput->mh.Bytes = (char *)( (char *)&mput->DataBuffer[buflen] ) - (char *)mput ;
		   if ((i = send(acb->CSSocket,mput,mput->mh.Bytes,0)) < mput->mh.Bytes)
		    v4_error(V4E_SENDERR,0,"V4IS","GET","SENDERR",
				"Error (%s) (len = %d/%d) in send on socket (%d)",v_OSErrString(NETERROR),mput->mh.Bytes,i,UCretASC(pcb->UCFileName)) ;
		   if ((i = recv(acb->CSSocket,&err.mh,sizeof err.mh,0)) < 0)
		    v4_error(V4E_RECVERR,err.VStdErrNum,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
		   switch (err.mh.MsgType)
		    { default:
			v4_error(V4E_UNEXPSRVMSG,0,"V4IS","GET","UNEXPSRVMSG","Unexpected server message (%d) on Get to file (%s)",
				err.mh.MsgType,UCretASC(pcb->UCFileName)) ;
		      case V4CS_Msg_Error:		/* Got an error, read remainder of message */
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","PUT","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			v4_error(V4E_SRVERR,err.VStdErrNum,"V4IS","PUT","SRVERR",err.Message) ;
		      case V4CS_Msg_PutOK:
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","PUT","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			break ;
		    } ;
		   return(TRUE) ;
		 } ;
#endif
		if (mmm->Areas[ax].LockRec >= 0)
		 { i = mmm->Areas[ax].LockRec ; mmm->Areas[ax].LockRec = UNUSED ;
		   v4mm_LockRelease(pcb->AreaId,i,mmm->MyPid,pcb->AreaId,"V3Unlock") ;
		 } ;
		break ;
	   case V4IS_PCB_GP_Delete:
		if (!(acb->AccessMode & V4IS_PCB_AM_Delete))
		 v4_error(V4E_AREANODELETE,0,"V4IS","Put","AREANODELETE","Delete operation not permitted for this area (%s)",UCretASC(pcb->UCFileName)) ;
#ifdef V4_BUILD_CS_FUNCTIONALITY
		if (acb->CSaid != UNUSED)			/* Going off to a remote server ? */
		 { if (mput == NULL) mput = (struct V4CS_MsgPut *)v4mm_AllocChunk(sizeof *mput,FALSE) ;
		   mput->mh.MsgType = V4CS_Msg_Put ; mput->mh.Flags = 0 ;
		   mput->PutMode = V4CS_PutMode_Delete ; mput->CmpMode = cmpmode ; mput->PutBytes = buflen ;
		   memcpy(&mput->DataBuffer,bufptr,buflen) ; mput->AreaId = acb->CSaid ;
		   mput->mh.Bytes = (char *)( (char *)&mput->DataBuffer[buflen] ) - (char *)mput ;
		   if ((i = send(acb->CSSocket,mput,mput->mh.Bytes,0)) < mput->mh.Bytes)
		    v4_error(V4E_SENDERR,0,"V4IS","GET","SENDERR",
				"Error (%s) (len = %d/%d) in send on socket (%d)",v_OSErrString(NETERROR),mput->mh.Bytes,i,UCretASC(pcb->UCFileName)) ;
		   if ((i = recv(acb->CSSocket,&err.mh,sizeof err.mh,0)) < 0)
		    v4_error(V4E_RECVERR,err.VStdErrNum,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
		   switch (err.mh.MsgType)
		    { default:
			v4_error(V4E_UNEXPSRVMSG,0,"V4IS","GET","UNEXPSRVMSG","Unexpected server message (%d) on Get to file (%s)",
				err.mh.MsgType,UCretASC(pcb->UCFileName)) ;
		      case V4CS_Msg_Error:		/* Got an error, read remainder of message */
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","PUT","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			v4_error(V4E_SRVERR,err.VStdErrNum,"V4IS","PUT","SRVERR",err.Message) ;
		      case V4CS_Msg_PutOK:
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","PUT","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			break ;
		    } ;
		   return(TRUE) ;
		 } ;
#endif
		if (mmm->Areas[ax].LockRec == UNUSED && mmm->Areas[ax].SegId != 0)
		 v4_error(V4E_RECNOTLOCKED,0,"V4IS","Put","RECNOTLOCKED","Area (%d %s) - cannot update without prior lock", ax,UCretASC(pcb->UCFileName)) ;
		if (acb->HashInfo != NULL)				/* Got a hash file? */
		 { dataid = v4h_PutHash(pcb,acb,keyptr,origbufptr,V4H_PutHash_Delete) ;
		   pcb->DataId = dataid ;
		   break ;
		 } ;
		if (mmm->Areas[ax].IndexOnlyCount > 0)
		 { dim.dataid = pcb->DataId ;				/* Decode the DataId */
		   if (dim.fld.BktNum >= mmm->Areas[ax].IndexOnly[0].BktOffset)
		    v4_error(V4E_UPDATELINK,0,"V4IS","Put","UPDATELINK","Area (%d %s) - cannot update record in linked area",ax,UCretASC(pcb->UCFileName)) ;
		 } ;
		v4is_Delete(aid,pcb,fileref) ;
		acb->CurPosKeyIndex = pcb->CurPosKeyIndex-1 ; acb->CurLvl = pcb->CurLvl ; /* Restore to current index position */
		for(i=0;i<=acb->CurLvl;i++) { acb->Lvl[i].BktNum = pcb->BktNums[i] ; } ;
		acb->KeyPrefix.all = pcb->KeyPrefix.all ;
		break ;
	   case V4IS_PCB_GP_Update:
		if (!(acb->AccessMode & V4IS_PCB_AM_Update))
		 v4_error(V4E_AREANOUPD,0,"V4IS","Put","AREANOUPD","Update operation not permitted for this area (%s)",UCretASC(pcb->UCFileName)) ;
#ifdef NOTWANTED
		if (mmm->Areas[ax].LockRec == UNUSED && mmm->Areas[ax].SegId != 0)
		 v4_error(V4E_RECNOTLOCKED,0,"V4IS","Put","RECNOTLOCKED","Area (%d %s) - cannot update without prior lock", ax,UCretASC(pcb->UCFileName)) ;
#endif
		if (mmm->Areas[ax].IndexOnlyCount > 0)
		 { dim.dataid = pcb->DataId ;				/* Decode the DataId */
		   if (dim.fld.BktNum >= mmm->Areas[ax].IndexOnly[0].BktOffset)
		    v4_error(V4E_UPDATELINK,0,"V4IS","Put","UPDATELINK","Area (%d %s) - cannot update record in linked area",ax,UCretASC(pcb->UCFileName)) ;
		 } ;
#ifdef V4_BUILD_CS_FUNCTIONALITY
		if (acb->CSaid != UNUSED)			/* Going off to a remote server ? */
		 { if (mput == NULL) mput = (struct V4CS_MsgPut *)v4mm_AllocChunk(sizeof *mput,FALSE) ;
		   mput->mh.MsgType = V4CS_Msg_Put ; mput->mh.Flags = 0 ;
		   mput->PutMode = V4CS_PutMode_Update ; mput->CmpMode = cmpmode ; mput->PutBytes = buflen ;
		   memcpy(&mput->DataBuffer,bufptr,buflen) ; mput->AreaId = acb->CSaid ;
		   mput->mh.Bytes = (char *)( (char *)&mput->DataBuffer[buflen] ) - (char *)mput ;
		   if ((i = send(acb->CSSocket,mput,mput->mh.Bytes,0)) < mput->mh.Bytes)
		    v4_error(V4E_SENDERR,0,"V4IS","GET","SENDERR",
				"Error (%s) (len = %d/%d) in send on socket (%d)",v_OSErrString(NETERROR),mput->mh.Bytes,i,UCretASC(pcb->UCFileName)) ;
		   if ((i = recv(acb->CSSocket,&err.mh,sizeof err.mh,0)) < 0)
		    v4_error(V4E_RECVERR,err.VStdErrNum,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
		   switch (err.mh.MsgType)
		    { default:
			v4_error(V4E_UNEXPSRVMSG,0,"V4IS","GET","UNEXPSRVMSG","Unexpected server message (%d) on Get to file (%s)",
				err.mh.MsgType,UCretASC(pcb->UCFileName)) ;
		      case V4CS_Msg_Error:		/* Got an error, read remainder of message */
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","PUT","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			v4_error(V4E_SRVERR,err.VStdErrNum,"V4IS","PUT","SRVERR",err.Message) ;
		      case V4CS_Msg_PutOK:
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","PUT","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			break ;
		    } ;
		   return(TRUE) ;
		 } ;
#endif
		if (keyptr == NULL) keyptr = (KEY *)origbufptr ;	/* If no key assume begin of buffer is key */
		keynum = 0 ;						/* Primary key */
		if (fileref != 0) keyptr = v4is_MakeFFKey(acb,fileref,&lclkey,0,NULL,origbufptr,TRUE) ;
		if (v4is_FFKeyCount(acb,fileref) > 1)
		 { if (datamode == V4IS_PCB_DataMode_Index)
		    v4_error(V4E_PUTIDXMLTKEY,0,"V4IS","Put","PUTIDXMLTKEY","Cannot PUT data in index bucket with multiple keys (%s)",
				UCretASC(pcb->UCFileName)) ;
		    datamode = V4IS_PCB_DataMode_Data ;			/* Force data into data bucket */
		 } ;
		if (acb->HashInfo != NULL)				/* Got a hash file? */
		 { dataid = v4h_PutHash(pcb,acb,keyptr,origbufptr,V4H_PutHash_Update) ;
		   pcb->DataId = dataid ;
		   break ;
		 } ;
		v4is_Replace(aid,keyptr,origbufptr,bufptr,buflen,datamode,cmpmode,pcb->DataId,fileref) ;
		acb->CurPosKeyIndex = pcb->CurPosKeyIndex ; acb->CurLvl = pcb->CurLvl ;	/* Restore to current index position */
		for(i=0;i<=acb->CurLvl;i++) { acb->Lvl[i].BktNum = pcb->BktNums[i] ; } ;
		acb->KeyPrefix.all = pcb->KeyPrefix.all ;
		break ;
	   case V4IS_PCB_GP_DataOnly:
		if (!(acb->AccessMode & V4IS_PCB_AM_Insert))
		 v4_error(V4E_AREANOPUT,0,"V4IS","Put","AREANOPUT","Insert operation not permitted for this area (%s)",UCretASC(pcb->UCFileName)) ;
		if (pcb->LockMode != 0)	
		 v4_error(V4E_AREANOPUT,0,"V4IS","Put","AREANOPUT","DataOnly operation not permitted (locking enabled) for this area (%s)",UCretASC(pcb->UCFileName)) ;
		pcb->DataId = v4is_StashData(aid,bufptr,buflen,datamode,cmpmode,0,fileref,acb->RootInfo->FillPercent) ;
		break ;
	   case V4IS_PCB_GP_Insert:
		if (!(acb->AccessMode & V4IS_PCB_AM_Insert))
		 v4_error(V4E_AREANOPUT,0,"V4IS","Put","AREANOPUT","Insert operation not permitted for this area (%s)",UCretASC(pcb->UCFileName)) ;
#ifdef V4_BUILD_CS_FUNCTIONALITY
		if (acb->CSaid != UNUSED)			/* Going off to a remote server ? */
		 { if (mput == NULL) mput = (struct V4CS_MsgPut *)v4mm_AllocChunk(sizeof *mput,FALSE) ;
		   mput->mh.MsgType = V4CS_Msg_Put ; mput->mh.Flags = 0 ;
		   mput->PutMode = V4CS_PutMode_Insert ; mput->CmpMode = cmpmode ; mput->PutBytes = buflen ;
		   memcpy(&mput->DataBuffer,bufptr,buflen) ; mput->AreaId = acb->CSaid ;
		   mput->mh.Bytes = (char *)( (char *)&mput->DataBuffer[buflen] ) - (char *)mput ;
		   if ((i = send(acb->CSSocket,mput,mput->mh.Bytes,0)) < mput->mh.Bytes)
		    v4_error(V4E_SENDERR,0,"V4IS","GET","SENDERR",
				"Error (%s) (len = %d/%d) in send on socket (%d)",v_OSErrString(NETERROR),mput->mh.Bytes,i,UCretASC(pcb->UCFileName)) ;
		   if ((i = recv(acb->CSSocket,&err.mh,sizeof err.mh,0)) < 0)
		    v4_error(V4E_RECVERR,err.VStdErrNum,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
		   switch (err.mh.MsgType)
		    { default:
			v4_error(V4E_UNEXPSRVMSG,0,"V4IS","GET","UNEXPSRVMSG","Unexpected server message (%d) on Get to file (%s)",
				err.mh.MsgType,UCretASC(pcb->UCFileName)) ;
		      case V4CS_Msg_Error:		/* Got an error, read remainder of message */
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","PUT","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			v4_error(V4E_SRVERR,err.VStdErrNum,"V4IS","PUT","SRVERR",err.Message) ;
		      case V4CS_Msg_PutOK:
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","PUT","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			break ;
		    } ;
		   return(TRUE) ;
		 } ;
#endif
		keyptr = (KEY *)origbufptr ;		/* First assume key is at begin of record */
		if (fileref != 0) keyptr = v4is_MakeFFKey(acb,fileref,&lclkey,0,NULL,origbufptr,TRUE) ;
		if (v4is_FFKeyCount(acb,fileref) > 1)
		 { if (datamode == V4IS_PCB_DataMode_Index)
		    v4_error(V4E_PUTIDXMLTKEY,0,"V4IS","Put","PUTIDXMLTKEY","Cannot PUT data in index bucket with multiple keys (%s)",
				UCretASC(pcb->UCFileName)) ;
		    datamode = V4IS_PCB_DataMode_Data ;			/* Force data into data bucket */
		 } ;
		if (acb->HashInfo != NULL)				/* Got a hash file? */
		 { dataid = v4h_PutHash(pcb,acb,keyptr,origbufptr,V4H_PutHash_Insert) ;
		   pcb->DataId = dataid ;
		   break ;
		 } ;
		memcpy(&acb->LastKey,keyptr,keyptr->Bytes) ;
		dataid = v4is_Insert(aid,keyptr,bufptr,buflen,datamode,cmpmode,0,
			(fileref == 0 ? FALSE : (v4is_FFKeyCount(acb,fileref)>1 ? TRUE : FALSE)),v4is_FFKeyDups(acb,fileref,0),0,errmsg) ;
		if (dataid == -1) return(FALSE) ;			/* Insert failed */
		pcb->DataId = dataid ;
		if (fileref != 0)
		 { for(kx=1;kx<v4is_FFKeyCount(acb,fileref);kx++)				/* Loop thru all secondary keys */
		    { keyptr = v4is_MakeFFKey(acb,fileref,&lclkey,kx,NULL,origbufptr,TRUE) ;
		      acb->DataId = dataid ;				/* Restore proper ID (positionkey may have trashed) */
		      v4is_Insert(aid,keyptr,NULL,0,datamode,cmpmode,0,FALSE,v4is_FFKeyDups(acb,fileref,kx),acb->DataId,NULL) ;
		    } ;
		   if (kx > 1)				/* If alternate keys then reposition to primary */
		    v4is_PositionKey(aid,&acb->LastKey,NULL,dataid,V4IS_PosBoKL) ;
		 } ;
		break ;
	   case V4IS_PCB_GP_Write:
		if (!((acb->AccessMode & V4IS_PCB_AM_Update) || (acb->AccessMode & V4IS_PCB_AM_Insert)))
		 v4_error(V4E_AREANOWRT,0,"V4IS","Put","AREANOWRT","Write (Update+Insert) not permitted for this area (%s)",UCretASC(pcb->UCFileName)) ;
#ifdef V4_BUILD_CS_FUNCTIONALITY
		if (acb->CSaid != UNUSED)			/* Going off to a remote server ? */
		 { if (mput == NULL) mput = (struct V4CS_MsgPut *)v4mm_AllocChunk(sizeof *mput,FALSE) ;
		   mput->mh.MsgType = V4CS_Msg_Put ; mput->mh.Flags = 0 ;
		   mput->PutMode = V4CS_PutMode_Write ; mput->CmpMode = cmpmode ; mput->PutBytes = buflen ;
		   memcpy(&mput->DataBuffer,bufptr,buflen) ; mput->AreaId = acb->CSaid ;
		   mput->mh.Bytes = (char *)( (char *)&mput->DataBuffer[buflen] ) - (char *)mput ;
		   if ((i = send(acb->CSSocket,mput,mput->mh.Bytes,0)) < mput->mh.Bytes)
		    v4_error(V4E_SENDERR,0,"V4IS","GET","SENDERR",
				"Error (%s) (len = %d/%d) in send on socket (%d)",v_OSErrString(NETERROR),mput->mh.Bytes,i,UCretASC(pcb->UCFileName)) ;
		   if ((i = recv(acb->CSSocket,&err.mh,sizeof err.mh,0)) < 0)
		    v4_error(V4E_RECVERR,err.VStdErrNum,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
		   switch (err.mh.MsgType)
		    { default:
			v4_error(V4E_UNEXPSRVMSG,0,"V4IS","GET","UNEXPSRVMSG","Unexpected server message (%d) on Get to file (%s)",
				err.mh.MsgType,UCretASC(pcb->UCFileName)) ;
		      case V4CS_Msg_Error:		/* Got an error, read remainder of message */
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","PUT","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			v4_error(V4E_SRVERR,err.VStdErrNum,"V4IS","PUT","SRVERR",err.Message) ;
		      case V4CS_Msg_PutOK:
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","PUT","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			break ;
		    } ;
		   return(TRUE) ;
		 } ;
#endif
/*		For "WRITE" have to first see if record exists - access via primary key */
		if (keyptr == NULL) keyptr = (KEY *)origbufptr ;	/* If no key assume begin of buffer is key */
		keynum = 0 ;						/* Primary key */
		if (fileref != 0)
		 { if (v4is_FFKeyDups(acb,fileref,keynum))		/* If duplicates on primary, assume Insert */
		    { mode = (mode & 0xffff0000) + V4IS_PCB_GP_Insert ; goto try_again ; } ;
		   keyptr = v4is_MakeFFKey(acb,fileref,&lclkey,keynum,NULL,origbufptr,TRUE) ;
		 } ;
		if (acb->HashInfo != NULL)				/* Got a hash file? */
		 { dataid = v4h_PutHash(pcb,acb,keyptr,origbufptr,V4H_PutHash_Write) ;
		   pcb->DataId = dataid ;
		   break ;
		 } ;
		RELDAT RELIDX
		v4mm_LockSomething(aid,V4MM_LockId_Tree,V4MM_LockMode_Read,V4MM_LockIdType_Tree,20,&mmm->Areas[ax].LockTree,8) ;
		res = v4is_PositionKey(aid,keyptr,NULL,0,V4IS_PosBoKL) ;
		if (res != V4RES_PosKey)				/* If no key then do "Insert" */
		 { RELTREE mode = (mode & 0xffff0000) + V4IS_PCB_GP_Insert ;
		   goto try_again ;
		 } ;
/*		Record exists- have to do an "Update", but first lets get DataId */
		bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,aid,IDX) ;	/* Get bucket pointer */
		ibh = (IBH *)bkt ;				/* Bucket header @ start of bucket */
		ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;	/* Get pointer to key index list below header */
		ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[acb->CurPosKeyIndex]] ;	/* then use to get begin of key entry @ bottom */
		switch (ikde->EntryType)
		 { default: v4_error(V4E_INVIKDE,0,"V4IS","GetRec","INVIKDE","Invalid IKDE Entry Type (%d) in Bucket %d, AreaID %d",
					ikde->EntryType,ibh->sbh.BktNum,aid) ;
		   case V4IS_EntryType_IndexLink: v4_error(V4E_SNBINDEXLINK,0,"V4IS","Get","SNBINDEXLINK","Positioned at IndexLink key- should not be here") ;
		   case V4IS_EntryType_DataLink:
		   case V4IS_EntryType_DataLen:
			pcb->DataId = ikde->DataId ; IBHINUSE ; RELTREE
			mode = (mode & 0xffff0000) + V4IS_PCB_GP_Update ; goto try_again ;
	 	 } ;
	 } ;
/*	Make sure nothing is locked which should not be */
	RELIDX RELDAT RELTREE
/*	If we have a record locked then unlock it */
	if (mmm->Areas[ax].LockRec != UNUSED)
	 { i = mmm->Areas[ax].LockRec ; mmm->Areas[ax].LockRec = UNUSED ; v4mm_LockRelease(aid,i,mmm->MyPid,aid,"PutLockRec") ;  } ;
	if (mmm->Areas[ax].GBPtr != NULL && (mode & V4IS_PCB_ReLock))	/* Relock record ? */
	 { if ( (pid=v4mm_LockSomething(aid,pcb->DataId,V4MM_LockMode_Write,V4MM_LockIdType_Record,0,&mmm->Areas[ax].LockRec,9))
			< 0) v4_error(V4E_RECCURLOCK,0,"V4IS","Put","RECCURLOCK","Record locked (v4is_Put) by Pid=%d",-pid) ;
	 } ;
	if (mode & V4IS_PCB_SafePut) v4mm_AreaCheckPoint(aid) ;		/* Maybe flush out any updated buckets */
#ifdef NOTCURRENTLYINUSE
	if (mode & V4IS_PCB_CheckTree)
	 { if (!(pcb->GetCount = v4is_CheckTree(pcb))) v4_error(V4E_DIRTREEBAD,0,"V4IS","Put","DIRTREEBAD","Directory Tree out of whack!") ;
	 } ;
#endif
#ifdef SIGNALS
	if (resetsigset)				/* Do we have to reset set of blocked signals ? */
	 { resetsigset = FALSE ; sigprocmask(SIG_SETMASK,&oldsigset,NULL) ; } ;
#endif
	return(TRUE) ;
}

/*	K E Y   P O S I T I O N I N G				*/

/*	v4is_PositionKey - Positions area to a particular key	*/
/*	Call: V4IS_Posxxx = v4is_PositionKey( aid , key , keyptr , dataid , posind )
	  where aid is area id,
		key is pointer to the key,
		keyptr if not NULL is updated with pointer to key(/data) entry in index
		dataid if non-ZERO specifies exact key in multi-key environment,
		posind determines where in chain of duplicates to position (V4IS_PosxxKL) */

int v4is_PositionKey(aid,key,keyptr,dataid,posind)
  int aid ;
  struct V4IS__Key *key ;
  struct V4IS__Key *(*keyptr) ;
  int dataid,posind ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__GlobalBuf *glb ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__Bkt *bkt ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  union V4IS__KeyPrefix *kp ;
  struct V4IS__Key skey ;
  struct V4IS__Key *dkey ;
  int keypos,ax,kx,totaltreelocks ;

/*	Start a top level, nesting down as deep as necessary to get to data */
	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return(UNUSED) ;			/* Could not find area? */
//	mmm = V4_GLOBAL_MMM_PTR ; ax = aid ;
	acb = v4mm_ACBPtr(aid) ;
	kp = (KP *)key ; acb->KeyPrefix.all = kp->all ;
	if (mmm->Areas[ax].SegId == 0) { glb = NULL ; } else { GETGLB(ax) ; } ;
	if (glb != NULL) totaltreelocks = glb->TotalTreeLocks ;
	bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl=0].BktNum,aid,IDX) ;	/* Link to top level bucket */
	for(;acb->CurLvl<V4IS_AreaCB_LvlMax;)
	 {
top_of_loop:
	   if (glb != NULL)
	    { if (glb->TotalTreeLocks != totaltreelocks)		/* Something happen to index tree ? */
	       { totaltreelocks = glb->TotalTreeLocks ;			/* Not really sure we want this?? VEH 032194 */
		 bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl=0].BktNum,aid,IDX) ;	/* Start all over again */
		 continue ;
	       } ;
	    } ;
	   ibh = (IBH *)bkt ;
	   ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;	/* Pointer to B header & Key indexes */
	   keypos = v4is_SearchBucket(bkt,key) ;
	   if (keypos >= 0)	/* Got an exact match ? */
	    { switch (posind)
	       { default: v4_error(V4E_INVPOSIND,0,"V4IS","PositionKey","INVPOSIND","Invalid position indicator (%d) in call",posind) ;
		 case V4IS_PosDCKLx:				/* Same as below except maybe return(via keyptr) ikde, not kp */
		 case V4IS_PosDCKL:	break ;			/* Don't care or key s/b unique */
		 case V4IS_PosBoKL:				/* Position to begin of list of duplicates */
			for(kx=keypos-1;kx>=0;kx--)		/* Look backwards to begin of chain */
			 { ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[kx]] ; dkey = (KEY *)((char *)ikde + sizeof *ikde) ;
			   if (memcmp(key,dkey,key->Bytes) != 0) break ; /* Want to position to first in chain of duplicates! */
			 } ; keypos = kx+1 ; break ;
		 case V4IS_PosEoKL:				/* Position to end of list of duplicates */
			for(kx=keypos+1;kx<ibh->KeyEntryNum;kx++)	/* Look forwards to end of chain */
			 { ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[kx]] ; dkey = (KEY *)((char *)ikde + sizeof *ikde) ;
			   if (memcmp(key,dkey,key->Bytes) != 0) break ; /* Want to position to last in chain of duplicates! */
			 } ; keypos = kx-1 ; break ;
	       } ;
	      ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[keypos]] ;
	      switch(ikde->EntryType)
	       { default: v4_error(V4E_INVIKDE,0,"V4IS","GetRec","INVIKDE","Invalid IKDE Entry Type (%d) in Bucket %d, AreaID %d",
					ikde->EntryType,ibh->sbh.BktNum,aid) ;
		 case V4IS_EntryType_IndexLink:
			dkey = (KEY *)((char *)ikde + sizeof *ikde) ; memcpy(&skey,dkey,dkey->Bytes) ;
			IBHINUSE ;
			bkt = v4mm_BktPtr(ikde->AuxVal,aid,IDX) ;		/* Link to child bucket */
			ibh = (IBH *)bkt ; acb->Lvl[++acb->CurLvl].BktNum = ibh->sbh.BktNum ;
			if (ibh->sbh.BktType != V4_BktHdrType_Index)
			 { ax = aid ;
			   v4_error(V4E_LINKNOTINDEX,0,"V4IS","PositionKey","LINKNOTINDEX",
				"Area (%d %s) Link from bucket (%d) to nonIndex bucket (%d)",
				aid,UCretASC(mmm->Areas[ax].UCFileName),acb->Lvl[acb->CurLvl-1].BktNum,acb->Lvl[acb->CurLvl].BktNum) ;
			 } ;
	   		ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;	/* Pointer to B header & Key indexes */
			ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[0]] ; dkey = (KEY *)((char *)ikde + sizeof *ikde) ;
			if (memcmp(&skey,dkey,dkey->Bytes) != 0)
			 { ax = aid ;
			   v4_error(V4E_FIRSTNOTSAMEASPARENT,0,"V4IS","PositionKey","FIRSTNOTSAMEASPARENT",
				"Area (%d %s) Link from bucket (%d) to bucket (%d)- First key in child not same as parent",
				aid,UCretASC(mmm->Areas[ax].UCFileName),acb->Lvl[acb->CurLvl-1].BktNum,acb->Lvl[acb->CurLvl].BktNum) ;
			 } ;
			continue ;						/*  and continue looping down */
		 case V4IS_EntryType_DataLink:
		 case V4IS_EntryType_DataLen:
/*		      Record current position & return OK */
		      acb->CurPosKeyIndex = keypos ;
		      kp = (KP *)((char *)ikde + sizeof *ikde) ;
		      if (dataid != 0)
		       { for(;;)
			  { if (ikde->DataId == dataid) break ;			/* Got the one we want */
			    IBHINUSE ;
			    v4is_PositionRel(aid,V4IS_PosNext,acb->KeyPrefix.all,NULL) ;	/* BAD *************** */
		 	    bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,aid,IDX) ;
			    ibh = (IBH *)bkt ;	/* Get the bucket */
			    ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ; /* Ptr to B hdr & Key indexes */
			    ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[acb->CurPosKeyIndex]] ;
	 		    if (glb != NULL)
	    		     { if (glb->TotalTreeLocks != totaltreelocks)		/* Something happen to index tree ? */
				{ totaltreelocks = glb->TotalTreeLocks ;
				  bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl=0].BktNum,aid,IDX) ;	/* Start all over again */
				  goto top_of_loop ;
				} ;
			     } ;
			  } ;
		       } ;
		      if (keyptr != NULL)					/* Return either kp or ikde */
		       { *keyptr = (posind == V4IS_PosDCKLx ? (KEY *)ikde : (KEY *)kp) ; } ;
		      acb->DataId = ikde->DataId ;				/* Save for possible replacement */
		      IBHINUSE ;
		      return(V4RES_PosKey) ;
	       } ;
	    } ;
/*	   Not an exact match- maybe we can continue looking down? */
	   keypos = (-keypos)-1 ;
	   if (keypos <= 0)						/* If keypos <= 0 then at begin of bucket */
	    { IBHINUSE ; acb->CurPosKeyIndex = 0 ; return(V4RES_PosNext) ; } ;
	   ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[keypos-1]] ;		/* Pointer to key at bottom of bucket */
/*	   Look at prior key, see if a link downward */
	   switch(ikde->EntryType)
	    { default: v4_error(V4E_INVIKDE,0,"V4IS","GetRec","INVIKDE","Invalid IKDE Entry Type (%d) in Bucket %d, AreaID %d",
					ikde->EntryType,ibh->sbh.BktNum,aid) ;
	      case V4IS_EntryType_IndexLink:
		   dkey = (KEY *)((char *)ikde + sizeof *ikde) ; memcpy(&skey,dkey,dkey->Bytes) ;
		   IBHINUSE ;
		   bkt = v4mm_BktPtr(ikde->AuxVal,aid,IDX) ;		/* Link to child bucket */
		   ibh = (IBH *)bkt ; acb->Lvl[++acb->CurLvl].BktNum = ibh->sbh.BktNum ;
	   	   ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;	/* Pointer to B header & Key indexes */
		   ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[0]] ; dkey = (KEY *)((char *)ikde + sizeof *ikde) ;
		   if (memcmp(&skey,dkey,dkey->Bytes) != 0)
		    { ax = aid ;
		      v4_error(V4E_FIRSTNOTSAMEASPARENT1,0,"V4IS","PositionKey","FIRSTNOTSAMEASPARENT1",
				"Area (%d %s) Link from bucket (%d) to bucket (%d)- First key in child not same as parent",
				aid,UCretASC(mmm->Areas[ax].UCFileName),acb->Lvl[acb->CurLvl-1].BktNum,acb->Lvl[acb->CurLvl].BktNum) ;
		    } ;
		   continue ;							/*  and continue looping down */
	      case V4IS_EntryType_DataLink:
	      case V4IS_EntryType_DataLen:
		   IBHINUSE ; acb->CurPosKeyIndex = keypos ; return(V4RES_PosNext) ;
	    } ;
	 } ;
	ax = aid ;
	v4_error(V4E_EXCEEDLVLMAX,0,"V4IS","PositionKey","EXCEEDLVLMAX",
		"Area (%d %s) Exceeded maximum (%d) number of nested index nodes on keyed lookup",
		aid,UCretASC(mmm->Areas[ax].UCFileName),V4IS_AreaCB_LvlMax) ;
	return(V4RES_xxx) ;
}

/*	v4is_PositionRel - Positions Relative to Current Key			*/
/*	Call: ok = v4is_PositionRel( aid , posflag , keyprefix , keyptr)
	  where ok = TRUE if done, FALSE if error,
		aid is area ID
		posflag is positioning flag- V4IS_Posxxx,
		keyprefix is key prefix of last key (so "next" in multi-key indexes makes some sense!),
		keyptr (if not NULL) is updated with pointer to key in buckets	*/

LOGICAL v4is_PositionRel(aid,posflag,keyprefix,keyptr)
  int aid ;
  int posflag ;
  int keyprefix ;
  struct V4IS__Key *(*keyptr) ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__Bkt *bkt ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4IS__Key *key,skey ;
  union V4IS__KeyPrefix *kp ;
  int ditch=0 ;
  int i,bktnum,dataid,check,ax ;

	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return(UNUSED) ;			/* Could not find area? */
	acb = v4mm_ACBPtr(aid) ;		/* Grab area control block */
	switch (posflag & 0xffff)
	 { default: v4_error(V4E_INVPOSFLAG,0,"V4IS","PositionRel","INVPOSFLAG","Invalid Position Flag (%x)",posflag) ;
	   case V4IS_PosEOF:
	   case V4IS_PosBOF:
		acb->CurLvl = 0 ;
		for(;;)
		 { bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,aid,IDX) ; ibh = (IBH *)bkt ;	/* Get the bucket */
		   if (acb->CurLvl == 0 && ibh->KeyEntryNum == 0)		/* Check for empty file */
		    { return(FALSE) ;
//		      if (keyprefix == -1) { return(FALSE) ; }
//		       else v4_error(V4E_HITEOF,0,"V4IS","PositionRel","HITEOF","EOF hit") ;
		    } ;
		   acb->CurPosKeyIndex = ((posflag & 0xffff) == V4IS_PosBOF ? 0 : ibh->KeyEntryNum - 1) ;
		   ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;	/* Get pointer to key index list below header */
		   ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[acb->CurPosKeyIndex]] ;	/* then use to get begin of key entry @ bottom */
		   switch (ikde->EntryType)
	    	    { default: v4_error(V4E_INVIKDE,0,"V4IS","GetRec","INVIKDE","Invalid IKDE Entry Type (%d) in Bucket %d, AreaID %d",
					ikde->EntryType,ibh->sbh.BktNum,aid) ;
			case V4IS_EntryType_IndexLink:				/* Got index link - nest down */
				acb->Lvl[++acb->CurLvl].BktNum = ikde->AuxVal ; IBHINUSE ;
				continue ;
			case V4IS_EntryType_DataLen:
			case V4IS_EntryType_DataLink:
				kp = (KP *)((char *)ikde + sizeof *ikde) ;
				acb->KeyPrefix.all = kp->all ;			/* Save key prefix for "next" operation */
				IBHINUSE ;
				if (keyptr != NULL) *keyptr = (struct V4IS__Key *)kp ;
				return(TRUE) ;
		    } ;
		 } ;
	   case V4IS_PosNext:
		if (acb->CurLvl < 0)
		 v4_error(V4E_NOCURPOS,0,"V4IS","PositionRel","NOCURPOS","Cannot position NEXT/PRIOR- No current position") ;
		for(check=FALSE;;)
		 {
		   bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,aid,IDX) ; ibh = (IBH *)bkt ;
		   if (ibh->sbh.BktType != V4_BktHdrType_Index && ibh->sbh.BktType != V4_BktHdrType_Root)
		    { 
//		      ax = aid ; mmm = V4_GLOBAL_MMM_PTR ;
		      v4_error(V4E_LINKNOTINDEX,0,"V4IS","PositionKey","LINKNOTINDEX",
				"Area (%d %s) Link from bucket (%d) to nonIndex bucket (%d)",
				aid,UCretASC(mmm->Areas[ax].UCFileName),acb->Lvl[acb->CurLvl-1].BktNum,acb->Lvl[acb->CurLvl].BktNum) ;
		    } ;
		   if (acb->CurPosKeyIndex+1 >= ibh->KeyEntryNum)	/* At end of this bucket ? */
		    { if (ibh->sbh.BktType == V4_BktHdrType_Root)
		       { IBHINUSE ;
			 if (keyprefix == -1 || (posflag & V4IS_PCB_NoError)!=0) { return(FALSE) ; }
			  else v4_error(V4E_HITEOF,0,"V4IS","PositionRel","HITEOF","EOF hit") ;	/* Hit EOF */
		       } ;
		      ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;		/* Get pointer to key index list below header */
		      ikde = (IKDE *)(char *)&bkt->Buffer[ibkl->DataIndex[0]] ;
		      key = (KEY *)((char *)ikde + sizeof *ikde) ;
		      memcpy(&skey,key,key->Bytes) ;			/* Copy key into save buffer */
		      bktnum = ibh->sbh.BktNum ;			/* Save bucket number */
		      dataid = ikde->DataId ;				/* Save data id */
		      IBHINUSE ;
#ifdef FLUSH
		      v4mm_BktFlush(ibh->sbh.BktNum,aid) ;		/* All done with this bucket - flush it */
#endif
		      if (ibh->ParentBktNum != acb->Lvl[acb->CurLvl-1].BktNum)
		       printf("? Bkt %d level (%d), parent bkt (%d) not same as in acb->Lvl (%d)\n",
				ibh->sbh.BktNum,acb->CurLvl,ibh->ParentBktNum,acb->Lvl[acb->CurLvl-1].BktNum) ;
		      i = v4is_PositionToParentNode(aid,&skey,bktnum,dataid,ibh->ParentBktNum) ;
		      acb->CurPosKeyIndex = i ; IBHINUSE ;
		      continue ;					/* Continue along */
		    } ;
		   acb->CurPosKeyIndex++ ;				/* Position to next key in index */
		   ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;		/* Get pointer to key index list below header */
		   ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[acb->CurPosKeyIndex]] ;	/* get begin of key entry @ bottom */
		   if (check)						/* Make sure first key matches that of parent link */
		    { key = (KEY *)((char *)ikde + sizeof *ikde) ;
		      if (memcmp(&skey,key,key->Bytes) != 0)
		       { if (ditch++ > 3)
			  { 
//			    mmm = V4_GLOBAL_MMM_PTR ; ax = aid ;
			    v4_error(V4E_BADKEYINLINK,0,"V4IS","PositionRel","BADKEYINLINK",
				"Area (%d %s) First key in bucket (%d) does not match that of parent (%d)",
				aid,UCretASC(mmm->Areas[ax].UCFileName),acb->Lvl[acb->CurLvl].BktNum,acb->Lvl[acb->CurLvl-1].BktNum) ;
			  } ;
			 v4is_PositionKey(aid,&acb->LastKey,NULL,acb->DataId,V4IS_PosBoKL) ;	/* Reposition to last key */
			 printf("?? Last ditch keyed repositioning in PositionRel\n") ;
			 check = FALSE ; continue ;
		       } ;
		    } ;
		   switch (ikde->EntryType)
	    	    { default: v4_error(V4E_INVIKDE,0,"V4IS","GetRec","INVIKDE","Invalid IKDE Entry Type (%d) in Bucket %d, AreaID %d",
					ikde->EntryType,ibh->sbh.BktNum,aid) ;
			case V4IS_EntryType_IndexLink:				/* Got index link - nest down */
				acb->Lvl[++acb->CurLvl].BktNum = ikde->AuxVal ;
				IBHINUSE ;
				key = (KEY *)((char *)ikde + sizeof *ikde) ;
				memcpy(&skey,key,key->Bytes) ; check = TRUE ;	/* Copy key into save buffer */
				acb->CurPosKeyIndex = -1 ; break ;
			case V4IS_EntryType_DataLen:
			case V4IS_EntryType_DataLink:
				kp = (KP *)((char *)ikde + sizeof *ikde) ;	/* Pointer to begin of key */
				if (keyprefix == -1) acb->KeyPrefix.all = kp->all ;
				if (kp->all != keyprefix && keyprefix != -1)
				 { IBHINUSE ; 
				   v_Msg(NULL,acb->pcb->errMsgBuf,"V4ISHitEOF") ;
				   return(FALSE) ;
//				   v4_error(V4E_HITEOF,0,"V4IS","PositionRel","HITEOF",UCretASC(acb->pcb->errMsgBuf)) ;
				 } ;
				IBHINUSE ;
				key = (KEY *)((char *)ikde + sizeof *ikde) ; acb->DataId = ikde->DataId ;
				memcpy(&acb->LastKey,key,key->Bytes) ;		/* Save just in case */
				if (keyptr != NULL) *keyptr = key ;
				return(TRUE) ;				/* Got something - return */
		    } ;
		 } ;
	 } ;
}

/*	v4is_PositionToParentNode - Positions index (acb...) to key in parent node corresponding to this key */
/*	Call: bx = v4is_PositionToParentNode( aid, key , bktnum , dataid , parentbkt )
	  where bx is index in current node (acb->CurLvl),
		aid is file's area id,
		key is key to position to,
		bktnum is current bucket (parent should point to it),
		dataid is data id of current key in case we need to do a keyed access to readjust tree,
		parentbkt is bucket number of parent					*/

int v4is_PositionToParentNode(aid,key,bktnum,dataid,parentbkt)
  struct V4IS__Key *key ;
  int bktnum,dataid,parentbkt ;
{ struct V4IS__Key *tkey ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__Bkt *bkt ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  int i,savei,tries ; char ebuf[250] ;

	acb = v4mm_ACBPtr(aid) ;		/* Grab area control block */
	acb->CurLvl-- ;
/*
	if (acb->Lvl[acb->CurLvl].BktNum != parentbkt)
	 printf("?? Parent in bucket (%d) is bucket %d, acb->Lvl[acb->CurLvl-1(%d)].BktNum is %d\n",
		   bktnum,parentbkt,acb->CurLvl-1,acb->Lvl[acb->CurLvl].BktNum) ;
*/
	bkt = v4mm_BktPtr(parentbkt,aid,IDX) ; ibh = (IBH *)bkt ;
	for(tries=0;tries<3;tries++)
	 {
	   i = v4is_SearchBucket(bkt,key) ;		/* Search for key in parent bucket */
	   if (i < 0)
	    { v4is_PositionKey(aid,key,NULL,dataid,V4IS_PosBoKL) ;	/* Positioning failed - go keyed to reform tree */
	      bktnum = acb->Lvl[acb->CurLvl].BktNum ;
	      bkt = v4mm_BktPtr(acb->Lvl[--acb->CurLvl].BktNum,aid,IDX) ; ibh = (IBH *)bkt ;
	      continue ;
	    } ;
	   ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
	   ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[i]] ;		/* Get ikde to this entry */
	   if (ikde->EntryType == V4IS_EntryType_IndexLink && ikde->AuxVal == bktnum)
	    { acb->CurPosKeyIndex = i ; return(i) ; } ;
	   for(savei=i,i--;i>=0;i--)			/* Not correct link - search backwards */
	    { ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[i]] ;
	      if (ikde->EntryType != V4IS_EntryType_IndexLink || ikde->AuxVal != bktnum) continue ;
	      tkey = (KEY *)((char *)ikde + sizeof *ikde) ;
	      if (memcmp(key,tkey,key->Bytes) == 0) { acb->CurPosKeyIndex = i ; return(i) ; } ;
	    } ;
	   for(i=savei+1;i<ibh->KeyEntryNum;i++)		/* Still can't find - search forwards */
	    { ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[i]] ;
	      if (ikde->EntryType != V4IS_EntryType_IndexLink || ikde->AuxVal != bktnum) continue ;
	      tkey = (KEY *)((char *)ikde + sizeof *ikde) ;
	      if (memcmp(key,tkey,key->Bytes) == 0) { acb->CurPosKeyIndex = i ; return(i) ; } ;
	    } ;
	   if (dataid == 0) break ;					/* Should not be here! */
	   sprintf(ebuf,"PosToPar failed: bkt=%d, dataid=%x, pbkt=%d key=%d %d %d",
			bktnum,dataid,parentbkt,key->KeyType,key->AuxVal,key->KeyVal.IntVal) ;
	   v4is_BugChk(aid,ebuf) ;
	   v4is_PositionKey(aid,key,NULL,dataid,V4IS_PosBoKL) ;	/* Positioning failed - go keyed to reform tree */
	   bktnum = acb->Lvl[acb->CurLvl].BktNum ;
	   bkt = v4mm_BktPtr(acb->Lvl[--acb->CurLvl].BktNum,aid,IDX) ; ibh = (IBH *)bkt ;
	 } ;
	v4_error(V4E_NOPOSPAR,0,"V4IS","PositionRel","NOPOSPAR",
		"Area (%d) Could not find position in parent index bucket (%d) of child bucket (%d)",
		aid,acb->Lvl[acb->CurLvl].BktNum,acb->Lvl[acb->CurLvl+1].BktNum) ;
	return(UNUSED) ;
}

/*	v4is_SearchBucket - Searches bucket for a key		*/
/*	Call: index - v4is_SearchBucket( bktptr , key )
	  where index is +n for exact match on n'th key, -n if no match & n = correct spot
		bktptr is pointer to the bucket,
		key is pointer to the key			*/

int v4is_SearchBucket(bktptr,key)
  struct V4IS__Bkt *bktptr ;
  struct V4IS__Key *key ;
{ struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct lki { int int1,int2,int3 ; } *ik1, *ik2 ;
  B64INT b641,b642 ;
  int i,j,k,c ;

/*	Let's link up everything */
	ibh = (IBH *)bktptr ; ibkl = (IBKL *)&bktptr->Buffer[ibh->KeyEntryTop] ;
	i = 0 ; j = ibh->KeyEntryNum-1 ; k = 1 ;
	switch (key->KeyMode)
	 { 
	   default: v4_error(V4E_INVKEYMODE,0,"V4IS","SearchBucket","INVKEYMODE","Invalid KeyMode (%d)",key->KeyMode) ;
	   case V4IS_KeyMode_Int:
		ik1 = (struct lki *)key ;
		for(;;)
		 { if (i > j) return(-(i+1)) ;	/* No match */
	   	   k = (i+j) / 2 ;
/*	  	   Find key for this entry */
		   ikde = (IKDE *)&bktptr->Buffer[ibkl->DataIndex[k]] ;
		   ik2 = (struct lki *)((char *)ikde + sizeof *ikde) ;
		   if ( ik1->int1 == ik2->int1)
		    { if (ik1->int2 == ik2->int2) { c = 0 ; }
		       else { c = (ik1->int2 > ik2->int2 ? 1 : -1) ; } ;
		    } else { c = (ik1->int1 > ik2->int1 ? 1 : -1) ; } ;
		   if (c == 0) return(k) ;		/* Got a match */
		   if (c < 0) { j = k-1 ; } else { i = k+1 ; } ;
		 } ; break ;
	   case V4IS_KeyMode_Fixed:
		ik1 = (struct lki *)key ;
		for(;;)
		 { if (i > j) return(-(i+1)) ;	/* No match */
	   	   k = (i+j) / 2 ;
/*	  	   Find key for this entry */
		   ikde = (IKDE *)&bktptr->Buffer[ibkl->DataIndex[k]] ;
		   ik2 = (struct lki *)((char *)ikde + sizeof *ikde) ;
		   if ( ik1->int1 == ik2->int1)
		    { memcpy(&b641,&ik1->int2,sizeof b641) ; memcpy(&b642,&ik2->int2,sizeof b641) ;
		      if (b641 == b642) { c = 0 ; }
		       else { c = (b641 > b642 ? 1 : -1) ; } ;
		    } else { c = (ik1->int1 > ik2->int1 ? 1 : -1) ; } ;
		   if (c == 0) return(k) ;		/* Got a match */
		   if (c < 0) { j = k-1 ; } else { i = k+1 ; } ;
		 } ; break ;
	   case V4IS_KeyMode_Int2:
		ik1 = (struct lki *)key ;
		for(;;)
		 { if (i > j) return(-(i+1)) ;	/* No match */
	   	   k = (i+j) / 2 ;
/*	  	   Find key for this entry */
		   ikde = (IKDE *)&bktptr->Buffer[ibkl->DataIndex[k]] ;
		   ik2 = (struct lki *)((char *)ikde + sizeof *ikde) ;
		   if ( ik1->int1 == ik2->int1)
		    { if (ik1->int2 == ik2->int2)
		       { if (ik1->int3 == ik2->int3) { c = 0 ; }
			  else { c = (ik1->int3 > ik2->int3 ? 1 : -1) ; } ;
		       } else { c = (ik1->int2 > ik2->int2 ? 1 : -1) ; } ;
		    } else { c = (ik1->int1 > ik2->int1 ? 1 : -1) ; } ;
		   if (c == 0) return(k) ;		/* Got a match */
		   if (c < 0) { j = k-1 ; } else { i = k+1 ; } ;
		 } ; break ;
	    case V4IS_KeyMode_RevInt:
		ik1 = (struct lki *)key ;
		for(;;)
		 { if (i > j) return(-(i+1)) ;	/* No match */
	   	   k = (i+j) / 2 ;
/*	  	   Find key for this entry */
		   ikde = (IKDE *)&bktptr->Buffer[ibkl->DataIndex[k]] ;
		   ik2 = (struct lki *)((char *)ikde + sizeof *ikde) ;
		   if ( ik1->int1 == ik2->int1)
		    { if (ik1->int2 == ik2->int2) { c = 0 ; }
		       else { c = (ik2->int2 > ik1->int2 ? 1 : -1) ; } ;
		    } else { c = (ik1->int1 > ik2->int1 ? 1 : -1) ; } ;
		   if (c == 0) return(k) ;		/* Got a match */
		   if (c > 0) { j = k-1 ; } else { i = k+1 ; } ;
		 } ; break ;
	    case V4IS_KeyMode_Alpha:
		ik1 = (struct lki *)key ;
		for(;;)
		 { if (i > j) return(-(i+1)) ;	/* No match */
	   	   k = (i+j) / 2 ;
/*	  	   Find key for this entry */
		   ikde = (IKDE *)&bktptr->Buffer[ibkl->DataIndex[k]] ;
		   ik2 = (struct lki *)((char *)ikde + sizeof *ikde) ;
		   if ( ik1->int1 == ik2->int1) { c = memcmp(&ik1->int2,&ik2->int2,key->Bytes-(sizeof ik1->int1)) ; }
		    else { c = (ik1->int1 > ik2->int1 ? 1 : -1) ; } ;
		   if (c == 0) return(k) ;		/* Got a match */
		   if (c < 0) { j = k-1 ; } else { i = k+1 ; } ;
		 } ; break ;

	 } ;
}

/*	v4is_Get - Retrieves a record and copies into buffer	*/
/*	Call: V4IS_Getxxx = v4is_Get( pcb )
	  where pcb is pointer to get/put instruction structure- V4IS__ParControlBlk	*/

LOGICAL v4is_Get(pcb)
  struct V4IS__ParControlBlk *pcb ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__GlobalBuf *glb ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__Bkt *bkt,*dbkt ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4IS__DataBktHdr *dbh ;
  struct V4IS__DataBktEntryHdr *dbeh ;
  struct V4IS__Key *key,*keyptr ;
#ifdef V4_BUILD_CS_FUNCTIONALITY
  struct V4CS__Msg_GetKey gkey ;
  struct V4CS__Msg_GetNext gnext ;
  struct V4CS__Msg_GetDataId gdid ;
  struct V4CS__Msg_DataRec *dr = NULL ;
  struct V4CS__Msg_Error err ;
#endif
  HANGLOOSEDCL int dataid_tries ;
  union V4MM__DataIdMask dim ; int tdataid ;
  union V4IS__KeyPrefix *kp1,*kp2 ;
  struct {
    union V4IS__KeyPrefix kp ;
    char KeyVal[V4IS_KeyBytes_Max] ;
   } lclkey ;
  char *bufptr ; int *intptr ; int buflen,seqbuflen,fileref,keynum ;
  int i,index,mode,res,aid,ax,pid,ok,bktnum ; char ebuf[150] ;
  int lrx ;

	pcb->GetCount++ ;
	if(pcb->GetMode != 0) { mode = pcb->GetMode ; pcb->GetMode = 0 ; } else { mode = pcb->DfltGetMode ; } ;
	if (pcb->GetBufPtr != NULL) { bufptr = pcb->GetBufPtr ; pcb->GetBufPtr = NULL ; } else { bufptr = pcb->DfltGetBufPtr ; } ;
	if (pcb->GetBufLen != 0) { buflen = pcb->GetBufLen ; pcb->GetBufLen = 0 ; } else { buflen = pcb->DfltGetBufLen ; } ;
	if (pcb->AreaId == V4IS_AreaId_SeqOnly)			/* Handle sequential only */
	 {
#ifdef WINNT
	   if (!ReadFile(pcb->hfile,&seqbuflen,sizeof seqbuflen,&ok,NULL)) ok = 0 ;
	   if (ok < sizeof seqbuflen) seqbuflen = V4IS_EOFMark_SeqOnly ;
#else
	   ok = fread(&seqbuflen,1,sizeof seqbuflen,pcb->FilePtr) ; /* First read in number of bytes in record */
	   if (ok == EOF || ok < sizeof seqbuflen) seqbuflen = V4IS_EOFMark_SeqOnly ;	/* If EOF then force V4 eof */
#endif
	   if (seqbuflen == V4IS_EOFMark_SeqOnly)		/* Hit EOF ? */
	    { v_Msg(NULL,pcb->errMsgBuf,"V4ISHitEOF") ;
	      if (mode & V4IS_PCB_NoError) { pcb->GetLen = UNUSED ; return(FALSE) ; } ;
	      v4_error(V4E_HITEOF,0,"V4IS","Get","HITEOF",UCretASC(pcb->errMsgBuf)) ;
	    }
#ifdef WINNT
	   if (!ReadFile(pcb->hfile,bufptr,(seqbuflen > buflen ? buflen : seqbuflen),&ok,NULL)) ok = 0 ;
	   if (ok < (seqbuflen > buflen ? buflen : seqbuflen))
	    v4_error(V4E_SEQIOERR,0,"V4IS","Get","SEQIOERR","Area (xxx %s) Error (%s) reading sequential record",UCretASC(pcb->UCFileName),v_OSErrString(GetLastError())) ;
#else
	   ok = fread(bufptr,1,(seqbuflen > buflen ? buflen : seqbuflen),pcb->FilePtr) ; /* Read in all (part) of data */
	   if (ok == EOF || ok < (seqbuflen > buflen ? buflen : seqbuflen))
	    v4_error(V4E_SEQIOERR,0,"V4IS","Get","SEQIOERR","Area (xxx %s) Error (%s) reading sequential record",UCretASC(pcb->UCFileName),v_OSErrString(errno)) ;
#endif
	   if (seqbuflen > buflen)
	    { pcb->DataLen = seqbuflen ; pcb->GetLen = buflen ;
	      for(ok=seqbuflen-buflen;ok>0;ok--)		/* Flush until end of record */
	       {
#ifdef WINNT
		 if (!ReadFile(pcb->hfile,ebuf,1,&ok,NULL)) ok = 0 ; if (ok != 1) break ;
#else
		 if (fread(ebuf,1,1,pcb->FilePtr) != 1) break ;
#endif
	       } ;
	      v4_error(V4E_SEQRECTOOBIG,0,"V4IS","Get","SEQRECTOOBIG","File (%s) next record (#%d) length (%d) too big for user buffer (%d)",
			UCretASC(pcb->UCFileName),pcb->GetCount,seqbuflen,buflen) ;
	    } ;
	   pcb->DataLen = (pcb->GetLen = seqbuflen) ;
	   return(TRUE) ;
	 } ;
	if (pcb->AreaId < 0) v4_error(V4E_INVAREAID,0,"V4IS","GET","INVAREAID","Invalid AreaId on this PCB (%s)",UCretASC(pcb->UCFileName)) ;
	aid = pcb->AreaId ;
	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED)
	 { v_Msg(NULL,pcb->errMsgBuf,"V4ISAreaId",aid,pcb->UCFileName) ;
	   if (mode & V4IS_PCB_NoError) { pcb->GetLen = UNUSED ; return(FALSE) ; } ;
	   v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
	 } ;
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++) { if (mmm->Areas[ax].AreaId == pcb->AreaId) break ; } ;
//	if (ax >= mmm->NumAreas) return(FALSE) ;			/* Could not find area? */
	acb = v4mm_ACBPtr(aid) ;
	if (mmm->Areas[ax].SegId != 0)				/* Global area ? */
	 { if ( GETGLB(ax) == NULL)
	    v4_error(V4E_AREANOTGLOBAL,0,"V4MM","Get","AREANOTGLOBAL","Area (%d %s) not global",ax,UCretASC(mmm->Areas[ax].UCFileName)) ;
	   for(;glb->PanicLockout > 0;)				/* Another process in panic mode ? */
	    { /* time(&timer) ; printf("%.19s -  Process %d in lockout mode from v4is_Get (%d) by pid=%d\n",
				ctime(&timer),mmm->MyPid,glb->PanicLockout,glb->PanicPid) ; */
	      if (glb->PanicLockout == V4IS_Lockout_WaitReadOK) { break ; } ;
	      if (glb->PanicLockout == V4IS_Lockout_Wait) { HANGLOOSE(250) ; continue ; } ;
	      if (glb->PanicLockout == V4IS_Lockout_Error)
	 	v4_error(V4E_LOCKOUTERR,0,"V4IS","Put","LOCKOUTERR","Cannot GET to area (%d %s) - Area has been locked-out",
			ax,UCretASC(mmm->Areas[ax].UCFileName)) ;
	      HANGLOOSE(250) ; glb->PanicLockout-- ;
	    } ;
	 } ;
	if (!(acb->AccessMode & V4IS_PCB_AM_Get))
	 v4_error(V4E_AREANOGET,0,"V4IS","Get","AREANOGET","Get operation not allowed on this area (%s)",UCretASC(pcb->UCFileName)) ;
	if (pcb->KeyPtr != NULL) { keyptr = pcb->KeyPtr ; pcb->KeyPtr = NULL ; } else { keyptr = pcb->DfltKeyPtr ; } ;
	if (pcb->KeyNum != 0) { keynum = pcb->KeyNum-1 ; pcb->KeyNum = 0 ; }
	 else { keynum = (pcb->DfltKeyNum > 0 ? pcb->DfltKeyNum-1 : pcb->DfltKeyNum) ; } ;
	if (pcb->FileRef != 0) { fileref = pcb->FileRef ; pcb->FileRef = 0 ; } else { fileref = pcb->DfltFileRef ; } ;
/*	How are we to position ? */
	if ((0xFFFF & mode) != V4IS_PCB_GP_DataId) pcb->DataId = -1 ;		/* No index positioning */
get_again:
	ok = TRUE ;
	switch((0xFFFF & mode))
	 { default: v4_error(V4E_INVGETMODE,0,"V4IS","GetRec","INVGETMODE","Invalid GetMode (%d)",mode) ;
	   case V4IS_PCB_GP_NextNum1:
		if (!(acb->AccessMode & V4IS_PCB_AM_Update))
		 v4_error(V4E_AREANOUPD,0,"V4IS","Put","AREANOUPD","Update operation not permitted for this area (%s)",UCretASC(pcb->UCFileName)) ;
		intptr = (int *)bufptr ;
		LOCKROOT ;
		*intptr = acb->RootInfo->NextAvailUserNum ; acb->RootInfo->NextAvailUserNum++ ;
		RELROOT("UserNum") ;
		v4mm_BktUpdated(pcb->AreaId,0) ;
		return(V4RES_xxx) ;
	   case V4IS_PCB_GP_DataOnly:
		if (pcb->LockMode != 0)	
		 v4_error(V4E_AREANOGET,0,"V4IS","Get","AREANOGET","DataOnly operation not permitted (locking enabled) for this area (%s)",UCretASC(pcb->UCFileName)) ;
#ifdef V4_BUILD_CS_FUNCTIONALITY
		if (acb->CSaid != UNUSED)			/* Going off to a remote server ? */
		 { gnext.mh.MsgType = V4CS_Msg_GetNext ; gnext.mh.Flags = 0 ;
		   gnext.LockFlag = (((mode & (V4IS_PCB_NoLock | V4IS_PCB_IgnoreLock)) == 0) ? TRUE : FALSE) ;
		   gnext.GetMode = V4CS_GetMode_DataOnly ;
		   gnext.AreaId = acb->CSaid ; gnext.mh.Bytes = sizeof gnext ;
		   if ((i = send(acb->CSSocket,&gnext,gnext.mh.Bytes,0)) < gnext.mh.Bytes)
		    v4_error(V4E_SENDERR,0,"V4IS","GET","SENDERR",
				"Area (%s), Error (%s) (len = %d/%d) in send on socket (%d)",UCretASC(pcb->UCFileName),v_OSErrString(NETERROR),gnext.mh.Bytes,i) ;
		   if ((i = recv(acb->CSSocket,&err.mh,sizeof err.mh,0)) < 0)
		    v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
		   switch (err.mh.MsgType)
		    { default:
			v4_error(V4E_UNEXPSRVMSG,0,"V4IS","GET","UNEXPSRVMSG","Unexpected server message (%d) on Get to file (%s)",
				err.mh.MsgType,UCretASC(pcb->UCFileName)) ;
		      case V4CS_Msg_Error:		/* Got an error, read remainder of message */
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			v4_error(V4E_SRVERR,err.VStdErrNum,"V4IS","GET","SRVERR",err.Message) ;
		      case V4CS_Msg_DataRec:
			dr = (struct V4CS__Msg_DataRec *)v4mm_AllocChunk(err.mh.Bytes) ;	/* Allocate what we need */
			blen= err.mh.Bytes - sizeof err.mh ;		/* Number of bytes to grab */
			bp = (char *)dr + sizeof err.mh ;
			for(i=0;i<blen;)
			 { if ((ok = recv(acb->CSSocket,bp,blen-i,0)) < 0)
			    v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			   i += ok ; bp += ok ;			/* May not receive entire buffer in one recv packet */
			 } ;
		    } ;
		   pcb->DataId = dr->DataId ; pcb->DataId2 = dr->DataId2 ;
		   ok = v4is_CopyData(aid,bufptr,buflen,&dr->Buffer,dr->DataLen,dr->CmpMode,pcb,mode) ;
		   if (dr != NULL) { v4mm_FreeChunk(dr) ; dr = NULL ; } ;
		   if (!ok) v4_error(V4E_RECTOOBIG,0,"V4IS","CopyData","RECTOOBIG","Area (%d %s) Record too big for user buffer (%d > %d)",
					ax,UCretASC(mmm->Areas[ax].UCFileName),pcb->DataLen,pcb->GetLen) ;
		   return ;
		 } ;
#endif
		if (acb->dil == NULL) acb->dil = (struct V4IS__DataIdList *)v4mm_AllocChunk(sizeof *acb->dil,TRUE) ;
		for(;;)
		 { for(;acb->dil->NextX >= acb->dil->Count;)
		    { if (!v4is_FillDataIdList(aid,acb,acb->dil))
		       { v_Msg(NULL,pcb->errMsgBuf,"V4ISHitEOF") ;
		         if (mode & V4IS_PCB_NoError) { pcb->GetLen = UNUSED ; return(FALSE) ; } ;
		         v4_error(V4E_HITEOF,0,"V4IS","Get","HITEOF",UCretASC(pcb->errMsgBuf)) ;
		       };
		    } ;
		   dbeh = (struct V4IS__DataBktEntryHdr *)v4is_Getdbeh(aid,ax,acb->dil->DataIds[acb->dil->NextX++]) ;
		   if (dbeh->FileRef == V4IS_FileRef_Obsolete) continue ;	/* Data marked as gonner? */
		   if (dbeh != NULL) break ;			/* If null then dataid missing/moved/gone- get next */
		 } ;
		acb->DataId = dbeh->DataId ; pcb->DataId = acb->DataId ;
		ok = v4is_CopyData(aid,bufptr,buflen,(char*)dbeh + sizeof *dbeh,
						dbeh->Bytes-sizeof *dbeh,dbeh->CmpMode,pcb,mode) ;
		goto end_get ;
	   case V4IS_PCB_GP_KeyedPrior:
	   case V4IS_PCB_GP_KeyedNext:
		if (acb->HashInfo != NULL)
		 v4_error(V4E_INVHASHGETMODE,0,"V4IS","Get","INVHASHGETMODE","Function (%x) not allowed for hashed area(%s)",mode,UCretASC(pcb->UCFileName)) ;
		if (fileref != 0)
		 { keyptr = v4is_MakeFFKey(acb,fileref,&lclkey,keynum,keyptr,bufptr,TRUE) ; } ;
		if (keyptr == NULL) keyptr = (KEY *)bufptr ;	/* If no key assume begin of buffer is key */
#ifdef V4_BUILD_CS_FUNCTIONALITY
		if (acb->CSaid != UNUSED)			/* Going off to a remote server ? */
		 { gkey.mh.MsgType = V4CS_Msg_GetKey ; gkey.mh.Flags = 0 ;
		   gkey.LockFlag = (((mode & (V4IS_PCB_NoLock | V4IS_PCB_IgnoreLock)) == 0) ? TRUE : FALSE) ;
		   gkey.GetMode = V4CS_GetMode_GE ;
		   gkey.ActualBytes = v4is_BytesInFFKey(acb,fileref,keynum) ;
		   memcpy(&gkey.key,keyptr,keyptr->Bytes) ; gkey.AreaId = acb->CSaid ;
		   gkey.mh.Bytes = (char *)( (char *)&gkey.key + gkey.key.Bytes ) - (char *)&gkey ;
		   if ((i = send(acb->CSSocket,&gkey,gkey.mh.Bytes,0)) < gkey.mh.Bytes)
		    v4_error(V4E_SENDERR,0,"V4IS","GET","SENDERR",
				"Error (%s) (len = %d/%d) in send on socket (%d)",v_OSErrString(NETERROR),gkey.mh.Bytes,i,UCretASC(pcb->UCFileName)) ;
		   if ((i = recv(acb->CSSocket,&err.mh,sizeof err.mh,0)) < 0)
		    v4_error(V4E_RECVERR,err.VStdErrNum,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
		   switch (err.mh.MsgType)
		    { default:
			v4_error(V4E_UNEXPSRVMSG,0,"V4IS","GET","UNEXPSRVMSG","Unexpected server message (%d) on Get to file (%s)",
				err.mh.MsgType,UCretASC(pcb->UCFileName)) ;
		      case V4CS_Msg_Error:		/* Got an error, read remainder of message */
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			v4_error(V4E_SRVERR,err.VStdErrNum,"V4IS","GET","SRVERR",err.Message) ;
		      case V4CS_Msg_DataRec:
			dr = (struct V4CS__Msg_DataRec *)v4mm_AllocChunk(err.mh.Bytes) ;	/* Allocate what we need */
			blen= err.mh.Bytes - sizeof err.mh ;		/* Number of bytes to grab */
			bp = (char *)dr + sizeof err.mh ;
			for(i=0;i<blen;)
			 { if ((ok = recv(acb->CSSocket,bp,blen-i,0)) < 0)
			    v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			   i += ok ; bp += ok ;			/* May not receive entire buffer in one recv packet */
			 } ;
		    } ;
		   pcb->DataId = dr->DataId ; pcb->DataId2 = dr->DataId2 ;
		   ok = v4is_CopyData(aid,bufptr,buflen,&dr->Buffer,dr->DataLen,dr->CmpMode,pcb,mode) ;
		   if (dr != NULL) { v4mm_FreeChunk(dr) ; dr = NULL ; } ;
		   if (!ok) v4_error(V4E_RECTOOBIG,0,"V4IS","CopyData","RECTOOBIG","Area (%d %s) Record too big for user buffer (%d > %d)",
					ax,UCretASC(mmm->Areas[ax].UCFileName),pcb->DataLen,pcb->GetLen) ;
		   return ;
		 } ;
#endif
		if (acb->HashInfo != NULL)				/* Got a hash file? */
		 v4_error(V4E_INVFORHASH,0,"V4IS","Get","INVFORHASH","Invalid v4is_Get mode (0x%x) for hashed area (%s)",mode,UCretASC(pcb->UCFileName)) ;
		RELDAT RELIDX
		v4mm_LockSomething(aid,V4MM_LockId_Tree,V4MM_LockMode_Read,V4MM_LockIdType_Tree,30,&mmm->Areas[ax].LockTree,10) ;
		res = v4is_PositionKey(aid,keyptr,NULL,0,V4IS_PosBoKL) ;
		bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,aid,IDX) ;	/* Get bucket pointer */
		ibh = (IBH *)bkt ; ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
		ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[acb->CurPosKeyIndex]] ;
		kp1 = (KP *)keyptr ;			/* If at "next" then make sure key prefix's match */
		if (acb->CurPosKeyIndex >= ibh->KeyEntryNum)
		 { IBHINUSE ;
		   if (!v4is_PositionRel(aid,(0xffff0000&mode)+V4IS_PosNext,kp1->all,NULL))
		    { RELIDX RELDAT RELTREE			/* Can't find record - don't generate error maybe */
		      if (mode & V4IS_PCB_NoError) { pcb->GetLen = UNUSED ; return(FALSE) ; } ;
		      v4_error(V4E_POSPASTEOF,0,"V4IS","Get","POSPASTEOF","No such key & positioned past end of file") ;
		    } ;
		   bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,aid,IDX) ;	/* Get bucket pointer */
		   ibh = (IBH *)bkt ; ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
		   ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[acb->CurPosKeyIndex]] ;
		 } else if (ikde->EntryType == V4IS_EntryType_IndexLink)
		 { IBHINUSE ;
		   acb->CurPosKeyIndex-- ;				/*  go back one so next-rec will position corectly */
		   if (!v4is_PositionRel(aid,(0xffff0000&mode)+V4IS_PosNext,kp1->all,NULL))
		    { RELIDX RELDAT RELTREE			/* Can't find record - don't generate error maybe */
		      if (mode & V4IS_PCB_NoError) { pcb->GetLen = UNUSED ; return(FALSE) ; } ;
		      v4_error(V4E_POSPASTEOF,0,"V4IS","Get","POSPASTEOF","No such key & positioned past end of file") ;
		    } ;
		   bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,aid,IDX) ;	/* Get bucket pointer */
		   ibh = (IBH *)bkt ; ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
		   ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[acb->CurPosKeyIndex]] ;
		 } ;
		kp2 = (KP *)((char *)ikde + sizeof *ikde) ;
		if (kp1->all != kp2->all)
		 { IBHINUSE ; RELIDX RELDAT RELTREE
		   v_Msg(NULL,pcb->errMsgBuf,"V4ISHitEOF") ;
		   if (mode & V4IS_PCB_NoError) { RELTREE pcb->GetLen = UNUSED ; return(FALSE) ; } ;
		   v4_error(V4E_HITEOF,0,"V4IS","Get","HITEOF",UCretASC(pcb->errMsgBuf)) ;
		 } ;
		IBHINUSE ;
		acb->KeyPrefix.all = kp1->all ;		/* Save for possible "Next" records */
		RELTREE break ;
	   case V4IS_PCB_GP_Keyed:
		if (fileref != 0)
		 { keyptr = v4is_MakeFFKey(acb,fileref,&lclkey,keynum,keyptr,bufptr,TRUE) ; } ;
		if (keyptr == NULL) keyptr = (KEY *)bufptr ;	/* If no key assume begin of buffer is key */
#ifdef V4_BUILD_CS_FUNCTIONALITY
		if (acb->CSaid != UNUSED)			/* Going off to a remote server ? */
		 { gkey.mh.MsgType = V4CS_Msg_GetKey ; gkey.mh.Flags = 0 ;
		   gkey.LockFlag = (((mode & (V4IS_PCB_NoLock | V4IS_PCB_IgnoreLock)) == 0) ? TRUE : FALSE) ;
		   gkey.GetMode = V4CS_GetMode_EQ ;
		   gkey.ActualBytes = v4is_BytesInFFKey(acb,fileref,keynum) ;
		   memcpy(&gkey.key,keyptr,keyptr->Bytes) ; gkey.AreaId = acb->CSaid ;
		   gkey.mh.Bytes = (char *)( (char *)&gkey.key + gkey.key.Bytes ) - (char *)&gkey ;
		   if ((i = send(acb->CSSocket,&gkey,gkey.mh.Bytes,0)) < gkey.mh.Bytes)
		    v4_error(V4E_SENDERR,0,"V4IS","GET","SENDERR",
				"Error (%s) (len = %d/%d) in send on socket (%d)",v_OSErrString(NETERROR),gkey.mh.Bytes,i,UCretASC(pcb->UCFileName)) ;
		   if ((i = recv(acb->CSSocket,&err.mh,sizeof err.mh,0)) < 0)
		    v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
		   switch (err.mh.MsgType)
		    { default:
			v4_error(V4E_UNEXPSRVMSG,0,"V4IS","GET","UNEXPSRVMSG","Unexpected server message (%d) on Get to file (%s)",
				err.mh.MsgType,UCretASC(pcb->UCFileName)) ;
		      case V4CS_Msg_Error:		/* Got an error, read remainder of message */
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			v4_error(V4E_SRVERR,err.VStdErrNum,"V4IS","GET","SRVERR",err.Message) ;
		      case V4CS_Msg_DataRec:
			dr = (struct V4CS__Msg_DataRec *)v4mm_AllocChunk(err.mh.Bytes) ;	/* Allocate what we need */
			blen= err.mh.Bytes - sizeof err.mh ;		/* Number of bytes to grab */
			bp = (char *)dr + sizeof err.mh ;
			for(i=0;i<blen;)
			 { if ((ok = recv(acb->CSSocket,bp,blen-i,0)) < 0)
			    v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			   i += ok ; bp += ok ;			/* May not receive entire buffer in one recv packet */
			 } ;
		    } ;
		   pcb->DataId = dr->DataId ; pcb->DataId2 = dr->DataId2 ;
		   ok = v4is_CopyData(aid,bufptr,buflen,&dr->Buffer,dr->DataLen,dr->CmpMode,pcb,mode) ;
		   if (dr != NULL) { v4mm_FreeChunk(dr) ; dr = NULL ; } ;
		   if (!ok) v4_error(V4E_RECTOOBIG,0,"V4IS","CopyData","RECTOOBIG","Area (%d %s) Record too big for user buffer (%d > %d)",
					ax,UCretASC(mmm->Areas[ax].UCFileName),pcb->DataLen,pcb->GetLen) ;
		   return ;
		 } ;
#endif
		RELDAT RELIDX
		v4mm_LockSomething(aid,V4MM_LockId_Tree,V4MM_LockMode_Read,V4MM_LockIdType_Tree,31,&mmm->Areas[ax].LockTree,11) ;
		if (acb->HashInfo != NULL)
		 { ok = v4h_GetHashKey(pcb,acb,bufptr,buflen,keyptr,mode) ;
		   goto end_get ;
		 } ;
		res = v4is_PositionKey(aid,keyptr,NULL,0,V4IS_PosBoKL) ; RELTREE
		if (res != V4RES_PosKey)
		 { RELIDX RELDAT RELTREE				/* Can't find record - don't generate error maybe */
		   if (mode & V4IS_PCB_NoError) { pcb->GetLen = UNUSED ; return(FALSE) ; } ;
		   v4_error(V4E_AREANOKEY,0,"V4IS","Get","AREANOKEY","No such key in area") ;
		 } ;
		break ;
	   case V4IS_PCB_GP_Next:
#ifdef V4_BUILD_CS_FUNCTIONALITY
		if (acb->CSaid != UNUSED)			/* Going off to a remote server ? */
		 { gnext.mh.MsgType = V4CS_Msg_GetNext ; gnext.mh.Flags = 0 ;
		   gnext.LockFlag = (((mode & (V4IS_PCB_NoLock | V4IS_PCB_IgnoreLock)) == 0) ? TRUE : FALSE) ;
		   gnext.GetMode = V4CS_GetMode_Next ;
		   gnext.AreaId = acb->CSaid ; gnext.mh.Bytes = sizeof gnext ;
		   if (acb->CSFlags & V4CS_Flag_FlipEndian)
		    { FLIPSHORT(gnext.mh.Bytes,gnext.mh.Bytes) ; FLIPLONG(gnext.AreaId,gnext.AreaId) ;
		      gnext.mh.Flags = V4CS_Flag_NoCompress ;	/* Don't want returned record to be compressed */
		    } ;
		   if ((i = send(acb->CSSocket,&gnext,sizeof gnext,0)) < sizeof gnext)
		    v4_error(V4E_SENDERR,0,"V4IS","GET","SENDERR",
				"Area (%s), Error (%s) (len = %d/%d) in send on socket (%d)",UCretASC(pcb->UCFileName),v_OSErrString(NETERROR),gnext.mh.Bytes,i) ;
		   if ((i = recv(acb->CSSocket,&err.mh,sizeof err.mh,0)) < 0)
		    v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
		   switch (err.mh.MsgType)
		    { default:
			v4_error(V4E_UNEXPSRVMSG,0,"V4IS","GET","UNEXPSRVMSG","Unexpected server message (%d) on Get to file (%s)",
				err.mh.MsgType,UCretASC(pcb->UCFileName)) ;
		      case V4CS_Msg_Error:		/* Got an error, read remainder of message */
			if (acb->CSFlags & V4CS_Flag_FlipEndian) { FLIPSHORT(err.mh.Bytes,err.mh.Bytes) ; } ;
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			*((char *)&err + err.mh.Bytes) = '\0' ;
			v4_error(V4E_SRVERR,err.VStdErrNum,"V4IS","GET","SRVERR",err.Message) ;
		      case V4CS_Msg_DataRec:
			if (acb->CSFlags & V4CS_Flag_FlipEndian) FLIPSHORT(err.mh.Bytes,err.mh.Bytes) ;
			dr = (struct V4CS__Msg_DataRec *)v4mm_AllocChunk(err.mh.Bytes) ;	/* Allocate what we need */
			blen= err.mh.Bytes - sizeof err.mh ;		/* Number of bytes to grab */
			bp = (char *)dr + sizeof err.mh ;
			for(i=0;i<blen;)
			 { if ((ok = recv(acb->CSSocket,bp,blen-i,0)) < 0)
			    v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			   i += ok ; bp += ok ;			/* May not receive entire buffer in one recv packet */
			 } ;
		    } ;
		   if (acb->CSFlags & V4CS_Flag_FlipEndian)
		    { FLIPLONG(dr->DataId,dr->DataId) ; FLIPLONG(dr->DataId2,dr->DataId2) ; FLIPSHORT(dr->DataLen,dr->DataLen) ;
		    } ;
		   pcb->DataId = dr->DataId ; pcb->DataId2 = dr->DataId2 ;
		   ok = v4is_CopyData(aid,bufptr,buflen,&dr->Buffer,dr->DataLen,dr->CmpMode,pcb,mode) ;
		   if (dr != NULL) { v4mm_FreeChunk(dr) ; dr = NULL ; } ;
		   if (!ok) v4_error(V4E_RECTOOBIG,0,"V4IS","CopyData","RECTOOBIG","Area (%d %s) Record too big for user buffer (%d > %d)",
					ax,UCretASC(mmm->Areas[ax].UCFileName),pcb->DataLen,pcb->GetLen) ;
		   return ;
		 } ;
#endif
		if (acb->HashInfo != NULL)				/* Got a hash file? */
		 v4_error(V4E_INVFORHASH,0,"V4IS","Get","INVFORHASH","Invalid v4is_Get mode (0x%x) for hashed area (%s)",mode,UCretASC(pcb->UCFileName)) ;
		RELDAT RELIDX
		v4mm_LockSomething(aid,V4MM_LockId_Tree,V4MM_LockMode_Read,V4MM_LockIdType_Tree,32,&mmm->Areas[ax].LockTree,12) ;
		res = v4is_PositionRel(aid,(0xffff0000&mode)+V4IS_PosNext,acb->KeyPrefix.all,NULL) ; RELTREE
		if (!res && (mode & V4IS_PCB_NoError)!=0) { pcb->GetLen = UNUSED ; return(FALSE) ; } ;
		if (!res || acb->KeyPrefix.all == -1)			/* Empty file */
		 v4_error(V4E_HITEOF,0,"V4IS","PositionRel","HITEOF","EOF hit") ;
		break ;
	   case V4IS_PCB_GP_Prior:
		res = v4is_PositionRel(aid,V4IS_PosPrior,acb->KeyPrefix.all,NULL) ; break ;
	   case V4IS_PCB_GP_BOF:
#ifdef V4_BUILD_CS_FUNCTIONALITY
		if (acb->CSaid != UNUSED)			/* Going off to a remote server ? */
		 { gnext.mh.MsgType = V4CS_Msg_GetNext ; gnext.mh.Flags = 0 ;
		   gnext.LockFlag = (((mode & (V4IS_PCB_NoLock | V4IS_PCB_IgnoreLock)) == 0) ? TRUE : FALSE) ;
		   gnext.GetMode = V4CS_GetMode_BOF ;
		   gnext.AreaId = acb->CSaid ; gnext.mh.Bytes = sizeof gnext ;
		   if ((i = send(acb->CSSocket,&gnext,gnext.mh.Bytes,0)) < gnext.mh.Bytes)
		    v4_error(V4E_SENDERR,0,"V4IS","GET","SENDERR",
				"Area (%s), Error (%s) (len = %d/%d) in send on socket (%d)",UCretASC(pcb->UCFileName),v_OSErrString(NETERROR),gnext.mh.Bytes,i) ;
		   if ((i = recv(acb->CSSocket,&err.mh,sizeof err.mh,0)) < 0)
		    v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%d)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
		   switch (err.mh.MsgType)
		    { default:
			v4_error(V4E_UNEXPSRVMSG,0,"V4IS","GET","UNEXPSRVMSG","Unexpected server message (%d) on Get to file (%s)",
				err.mh.MsgType,UCretASC(pcb->UCFileName)) ;
		      case V4CS_Msg_Error:		/* Got an error, read remainder of message */
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			v4_error(V4E_SRVERR,err.VStdErrNum,"V4IS","GET","SRVERR",err.Message) ;
		      case V4CS_Msg_DataRec:
			dr = (struct V4CS__Msg_DataRec *)v4mm_AllocChunk(err.mh.Bytes) ;	/* Allocate what we need */
			blen= err.mh.Bytes - sizeof err.mh ;		/* Number of bytes to grab */
			bp = (char *)dr + sizeof err.mh ;
			for(i=0;i<blen;)
			 { if ((ok = recv(acb->CSSocket,bp,blen-i,0)) < 0)
			    v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			   i += ok ; bp += ok ;			/* May not receive entire buffer in one recv packet */
			 } ;
		    } ;
		   pcb->DataId = dr->DataId ; pcb->DataId2 = dr->DataId2 ;
		   ok = v4is_CopyData(aid,bufptr,buflen,&dr->Buffer,dr->DataLen,dr->CmpMode,pcb,mode) ;
		   if (dr != NULL) { v4mm_FreeChunk(dr) ; dr = NULL ; } ;
		   if (!ok) v4_error(V4E_RECTOOBIG,0,"V4IS","CopyData","RECTOOBIG","Area (%d %s) Record too big for user buffer (%d > %d)",
					ax,UCretASC(mmm->Areas[ax].UCFileName),pcb->DataLen,pcb->GetLen) ;
		   return ;
		 } ;
#endif
		if (acb->HashInfo != NULL)				/* Got a hash file? */
		 v4_error(V4E_INVFORHASH,0,"V4IS","Get","INVFORHASH","Invalid v4is_Get mode (0x%x) for hashed area (%s)",mode,UCretASC(pcb->UCFileName)) ;
		res = v4is_PositionRel(aid,V4IS_PosBOF,0,NULL) ;
		if (!res)
		 { if (mode & V4IS_PCB_NoError) { pcb->GetLen = UNUSED ; return(FALSE) ; } ;
		   v4_error(V4E_INVFORHASH,0,"V4IS","Get","POSBOF","Could not position to BOF (%d %s)",mode,UCretASC(pcb->UCFileName)) ;
		 } ;
		break ;
	   case V4IS_PCB_GP_EOF:
		if (acb->HashInfo != NULL)				/* Got a hash file? */
		 v4_error(V4E_INVFORHASH,0,"V4IS","Get","INVFORHASH","Invalid v4is_Get mode (0x%x) for hashed area (%s)",mode,UCretASC(pcb->UCFileName)) ;
		res = v4is_PositionRel(aid,V4IS_PosEOF,0,NULL) ;
		if (!res)
		 { if (mode & V4IS_PCB_NoError) { pcb->GetLen = UNUSED ; return(FALSE) ; } ;
		   v4_error(V4E_INVFORHASH,0,"V4IS","Get","POSEOF","Could not position to EOF (%d %s)",mode,UCretASC(pcb->UCFileName)) ;
		 } ;
		break ;
	   case V4IS_PCB_GP_DataId:
		if (pcb->DataId == -1)
		 v4_error(V4E_DATAIDBAD,0,"V4IS","Get","DATAIDBAD","Not valid Id in DataId") ;
#ifdef V4_BUILD_CS_FUNCTIONALITY
		if (acb->CSaid != UNUSED)			/* Going off to a remote server ? */
		 { gdid.mh.MsgType = V4CS_Msg_GetDataId ; gdid.mh.Flags = 0 ;
		   gdid.LockFlag = (((mode & (V4IS_PCB_NoLock | V4IS_PCB_IgnoreLock)) == 0) ? TRUE : FALSE) ;
		   gdid.DataId = pcb->DataId ; gdid.DataId2 = pcb->DataId2 ; gdid.AreaId = acb->CSaid ;
		   gdid.mh.Bytes = sizeof gdid ;
		   if ((i = send(acb->CSSocket,&gdid,gdid.mh.Bytes,0)) < gdid.mh.Bytes)
		    v4_error(V4E_SENDERR,0,"V4IS","GET","SENDERR",
				"Error (%d) (len = %d/%d) in send on socket (%d)",NETERROR,gdid.mh.Bytes,i,UCretASC(pcb->UCFileName)) ;
		   if ((i = recv(acb->CSSocket,&err.mh,sizeof err.mh,0)) < 0)
		    v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
		   switch (err.mh.MsgType)
		    { default:
			v4_error(V4E_UNEXPSRVMSG,0,"V4IS","GET","UNEXPSRVMSG","Unexpected server message (%d) on Get to file (%s)",
				err.mh.MsgType,UCretASC(pcb->UCFileName)) ;
		      case V4CS_Msg_Error:		/* Got an error, read remainder of message */
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			v4_error(V4E_SRVERR,err.VStdErrNum,"V4IS","GET","SRVERR",err.Message) ;
		      case V4CS_Msg_DataRec:
			dr = (struct V4CS__Msg_DataRec *)v4mm_AllocChunk(err.mh.Bytes) ;	/* Allocate what we need */
			blen= err.mh.Bytes - sizeof err.mh ;		/* Number of bytes to grab */
			bp = (char *)dr + sizeof err.mh ;
			for(i=0;i<blen;)
			 { if ((ok = recv(acb->CSSocket,bp,blen-i,0)) < 0)
			    v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			   i += ok ; bp += ok ;			/* May not receive entire buffer in one recv packet */
			 } ;
		    } ;
		   ok = v4is_CopyData(aid,bufptr,buflen,&dr->Buffer,dr->DataLen,dr->CmpMode,pcb,mode) ;
		   if (dr != NULL) { v4mm_FreeChunk(dr) ; dr = NULL ; } ;
		   if (!ok) v4_error(V4E_RECTOOBIG,0,"V4IS","CopyData","RECTOOBIG","Area (%d %s) Record too big for user buffer (%d > %d)",
					ax,UCretASC(mmm->Areas[ax].UCFileName),pcb->DataLen,pcb->GetLen) ;
		   return ;
		 } ;
#endif
		if (acb->HashInfo != NULL)
		 { ok = v4h_GetHashDataId(pcb,acb,bufptr,buflen,keyptr,mode) ;
		   goto end_get ;
		 } ;
		acb->CurLvl = -1 ;					/* Prevent user from NEXT/PRIOR */
		acb->KeyPrefix.all = 0 ;				/* Prevent user doing a "next record" */
		dim.dataid = pcb->DataId ;				/* Decode the DataId */
		dbkt = v4mm_BktPtr(dim.fld.BktNum,aid,DAT) ; dbh = (DBH *)dbkt ; /* Link to data bucket */
/*		Before we get too carried away, let's make sure we actually have a data bucket */
		switch (dbh->sbh.BktType)
		 { default: v4_error(V4E_INVBKTTYPE,0,"V4IS","Get","INVBKTTYPE","Unknown bucket type-2 (%d)",dbh->sbh.BktType) ;
		   case V4_BktHdrType_Data:
/*			Scan thru data bucket for wanted data entry */
			tdataid = pcb->DataId ;
			for(i=0;i<mmm->Areas[ax].IndexOnlyCount;i++)	/* See if we should link to another area */
			 { if (dim.fld.BktNum < mmm->Areas[ax].IndexOnly[i].BktOffset) break ; } ;
			if (i > 0)					/* Is bucket number up in range for index-only link ? */
			 { dim.fld.BktNum -= mmm->Areas[ax].IndexOnly[i-1].BktOffset ; tdataid = dim.dataid ; } ;
			for(index=dbh->FirstEntryIndex;index<dbh->FreeEntryIndex;index+=ALIGN(dbeh->Bytes))
			 { dbeh = (struct V4IS__DataBktEntryHdr *)&dbkt->Buffer[index] ;
			   if (dbeh->Bytes == 0)
			    v4_error(V4E_INVDBEHLENIS0,0,"V4IS","Get","INVDBEHLENIS0","Area (%d %s) Bucket (%d) invalid dbeh->Bytes",
					aid,UCretASC(mmm->Areas[ax].UCFileName),dbh->sbh.BktNum) ;
			   if (dbeh->DataId != tdataid) continue ;
			   ok = v4is_CopyData(aid,bufptr,buflen,(char*)dbeh + sizeof *dbeh,
						dbeh->Bytes-sizeof *dbeh,dbeh->CmpMode,pcb,mode) ;
			   DBHINUSE ;
			   goto got_dataid ;
			 } ;
			v4_error(V4E_BADDDATAID,0,"V4IS","GetRec","BADDDATAID","Area (%d %s) data dataid (%x) cannot be found",
					aid,UCretASC(mmm->Areas[ax].UCFileName),pcb->DataId) ;
		   case V4_BktHdrType_Root:
		   case V4_BktHdrType_Index:
	   		ibh = (IBH *)dbkt ;
			ibkl = (IBKL *)&dbkt->Buffer[ibh->KeyEntryTop] ;/* Ptr to B header & Key indexes */
			for(index=0;index<ibh->KeyEntryNum;index++)
			 { ikde = (IKDE *)&dbkt->Buffer[ibkl->DataIndex[index]] ;
	      		   switch(ikde->EntryType)
	    	    	    { default: v4_error(V4E_INVIKDE,0,"V4IS","GetRec","INVIKDE","Invalid IKDE Entry Type (%d) in Bucket %d, AreaID %d",
					ikde->EntryType,ibh->sbh.BktNum,aid) ;
			      case V4IS_EntryType_IndexLink:	continue ;
			      case V4IS_EntryType_DataLink:	continue ;
			      case V4IS_EntryType_DataLen:
				   if (ikde->DataId != pcb->DataId) continue ;	/* Is this the one we want? */
				   /* FIXME */
				   if (ikde->KeyNotInRec)
				    { key = (KEY *)((char *)ikde + sizeof *ikde) ;
				      ok = v4is_CopyData(aid,bufptr,buflen,(char *)ikde+sizeof *ikde+key->Bytes,
						 ikde->AuxVal-sizeof *ikde-key->Bytes,
						 ikde->CmpMode,pcb,mode) ;/* Copy data to user space */
				    } else ok = v4is_CopyData(aid,bufptr,buflen,(char *)ikde+sizeof *ikde,ikde->AuxVal-sizeof *ikde,
						 ikde->CmpMode,pcb,mode) ;/* Copy data to user space */
				   IBHINUSE ;
				   goto got_dataid ;
	       		    } ;
			  } ;
			v4_error(V4E_BADIDATAID,0,"V4IS","GetRec","BADIDATAID","Area (%d %s) index dataid (%x) cannot be found",
					aid,UCretASC(mmm->Areas[ax].UCFileName),ikde->DataId) ;
		 } ;
got_dataid:
		if (fileref != 0)						/* Position to primary key */
		 { keyptr = v4is_MakeFFKey(acb,fileref,&lclkey,0,keyptr,bufptr,FALSE) ;
		   v4is_PositionKey(aid,keyptr,NULL,pcb->DataId,V4IS_PosBoKL) ;
		 } ;
		goto end_get ;
	 } ;
/*	If we drop thru to here then acb is positioned to key & maybe data */
	bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,aid,IDX) ;	/* Get bucket pointer */
	ibh = (IBH *)bkt ;				/* Bucket header @ start of bucket */
	ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;	/* Get pointer to key index list below header */
	ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[acb->CurPosKeyIndex]] ;	/* then use to get begin of key entry @ bottom */
	switch (ikde->EntryType)
	 { default: v4_error(V4E_INVIKDE,0,"V4IS","GetRec","INVIKDE","Invalid IKDE Entry Type (%d) in Bucket %d, AreaID %d",
					ikde->EntryType,ibh->sbh.BktNum,aid) ;
	   case V4IS_EntryType_IndexLink:
		v4_error(V4E_SNBINDEXLINK,0,"V4IS","Get","SNBINDEXLINK","Positioned at IndexLink key- should not be here") ;
	   case V4IS_EntryType_DataLink:
/*		Data in another bucket - link & grab */
#ifdef FLUSH
		if ((0xFFFF & mode) == V4IS_PCB_GP_Next)		/* Have we gotten prior data buckets? */
		 { if (acb->SeqDataBktNum != 0 && ikde->AuxVal != acb->SeqDataBktNum)
		    { v4mm_BktFlush(acb->SeqDataBktNum,aid) ; } ;
		   acb->SeqDataBktNum = ikde->AuxVal ;
		 } ;
#endif
		pcb->DataId = ikde->DataId ;				/* Save this for use in future operations (e.g. Delete) */
		bktnum = ikde->AuxVal ;					/* Bucket we want */
		dbkt = v4mm_BktPtr(bktnum,aid,DAT) ;			/* Link to data bucket */
		IBHINUSE ;
		dbh = (DBH *)dbkt ;			/* Data bucket header = begin of data bkt */
/*		Before we get too carried away, let's make sure we actually have a data bucket */
/*		May not be in case we have found alternate key which links back to main key+data ! */
		switch (dbh->sbh.BktType)
		 { default: v4_error(V4E_INVBKTTYPE,0,"V4IS","Get","INVBKTTYPE","Unknown bucket type-3 (%d)",dbh->sbh.BktType) ;
		   case V4_BktHdrType_Data:
			for(dataid_tries=0;dataid_tries<5;dataid_tries++)
			 {
/*			   Scan thru data bucket for wanted data entry */
			   tdataid = pcb->DataId ; dim.dataid = pcb->DataId ;
			   for(i=0;i<mmm->Areas[ax].IndexOnlyCount;i++)	/* See if we should link to another area */
			    { if (dim.fld.BktNum < mmm->Areas[ax].IndexOnly[i].BktOffset) break ; } ;
			   if (i > 0)					/* Is bucket number up in range for index-only link ? */
			    { dim.fld.BktNum -= mmm->Areas[ax].IndexOnly[i-1].BktOffset ; tdataid = dim.dataid ; } ;
			   for(index=dbh->FirstEntryIndex;index<dbh->FreeEntryIndex;index+=ALIGN(dbeh->Bytes))
			    { dbeh = (struct V4IS__DataBktEntryHdr *)&dbkt->Buffer[index] ;
			      if (dbeh->Bytes == 0)
			       v4_error(V4E_INVDBEHLEN,0,"V4IS","Get","INVDBEHLEN","Area (%d %s) Bucket (%d) invalid dbeh->Bytes",
					aid,UCretASC(mmm->Areas[ax].UCFileName),dbh->sbh.BktNum) ;
			      if (dbeh->DataId != tdataid) continue ;
			      ok = v4is_CopyData(aid,bufptr,buflen,(char*)dbeh + sizeof *dbeh,
						dbeh->Bytes-sizeof *dbeh,dbeh->CmpMode,pcb,mode) ;
			      DBHINUSE ;
			      goto end_get ;
			    } ;
/*			   Could not find dataid- wait & see if it is "in transit" by another process */
			   if (mmm->Areas[ax].SegId != 0)				/* Global area ? */
			    { GETGLB(ax) ;
			      for(i=0;i<V4MM_GlobalBufUpd_Max;i++)
			       { if (glb->DataUpd[i].DataIdRmv != pcb->DataId) continue ;
				 if (glb->DataUpd[i].DataIdNew == UNUSED)
				  {
				    goto get_again ;				/* Record gone- get next one */
				  } ;
				 if (glb->DataUpd[i].DataIdNew != 0) pcb->DataId = glb->DataUpd[i].DataIdNew ;
				 dim.dataid = pcb->DataId ; bktnum = dim.fld.BktNum ;
				 RELDAT ; HANGLOOSE(50) ;
				 dbkt = v4mm_BktPtr(bktnum,aid,DAT) ; dbh = (DBH *)dbkt ;
				 goto retry_data_lookup ;
			       } ;
			      dbkt = v4mm_BktPtr(bktnum,aid,DAT+REREAD) ;	/* Problems - try rereading bucket */
			      dbh = (DBH *)dbkt ;
			      continue ;
			    } ;
			   break ;			/* If here then no retry- drop thru to error below */
retry_data_lookup:	   continue ;
			 } ;
			sprintf(ebuf,"Data record (%x) not in bucket (%d)",pcb->DataId,bktnum) ;
			v4is_BugChk(aid,ebuf) ;
			v4_error(V4E_BADDATAID,0,"V4IS","GetRec1","BADDATAID","Data record associated with dataid (%x) cannot be found",
					pcb->DataId) ;
		   case V4_BktHdrType_Root:
		   case V4_BktHdrType_Index:
	   		ibh = (IBH *)dbkt ;
			ibkl = (IBKL *)&dbkt->Buffer[ibh->KeyEntryTop] ;/* Ptr to B header & Key indexes */
			for(index=0;index<ibh->KeyEntryNum;index++)
			 { ikde = (IKDE *)&dbkt->Buffer[ibkl->DataIndex[index]] ;
	      		   switch(ikde->EntryType)
	 		    { default: v4_error(V4E_INVIKDE,0,"V4IS","GetRec","INVIKDE","Invalid IKDE Entry Type (%d) in Bucket %d, AreaID %d",
					ikde->EntryType,ibh->sbh.BktNum,aid) ;
			      case V4IS_EntryType_IndexLink:	continue ;
			      case V4IS_EntryType_DataLink:	continue ;
			      case V4IS_EntryType_DataLen:
				   if (ikde->DataId != pcb->DataId) continue ;	/* Is this the one we want? */
				   /* FIXME */
				   if (ikde->KeyNotInRec)
				    { key = (KEY *) ((char *)ikde + sizeof *ikde) ;
				      ok = v4is_CopyData(aid,bufptr,buflen,(char *)ikde+sizeof *ikde+key->Bytes,
						 ikde->AuxVal-sizeof *ikde-key->Bytes,
						 ikde->CmpMode,pcb,mode) ;/* Copy data to user space */
				    } else ok = v4is_CopyData(aid,bufptr,buflen,(char *)ikde+sizeof *ikde,ikde->AuxVal-sizeof *ikde,
						 ikde->CmpMode,pcb,mode) ;/* Copy data to user space */
				   IBHINUSE ;
				   goto end_get ;
	       		    } ;
			  } ;
			sprintf(ebuf,"Data (GetRec2) record (%x) not in bucket (%d)",pcb->DataId,bktnum) ;
			v4is_BugChk(aid,ebuf) ;
			v4_error(V4E_BADDATAID,0,"V4IS","GetRec2","BADDATAID","Data record associated with dataid (%x) cannot be found",
					pcb->DataId) ;
		 } ;
	   case V4IS_EntryType_DataLen:
/*		Data immediately follows, pull it & return */
		/* FIXME */
		if (ikde->KeyNotInRec)
		 { key = (KEY *)((char *)ikde + sizeof *ikde) ;
		   ok = v4is_CopyData(aid,bufptr,buflen,(char *)ikde+sizeof *ikde+key->Bytes,
				  ikde->AuxVal-sizeof *ikde-key->Bytes,ikde->CmpMode,pcb,mode) ;
		 } else ok = v4is_CopyData(aid,bufptr,buflen,(char *)ikde+sizeof *ikde,
						ikde->AuxVal-sizeof *ikde,ikde->CmpMode,pcb,mode) ;
		pcb->DataId = ikde->DataId ;				/* Save this for use in future operations (e.g. Delete) */
		IBHINUSE ;
		break ;
	 } ;
end_get:
	pcb->CurPosKeyIndex = acb->CurPosKeyIndex ; pcb->CurLvl = acb->CurLvl ;	/* Save current index so REPLACE is cool */
	pcb->KeyPrefix.all = acb->KeyPrefix.all ;
	for(i=0;i<=pcb->CurLvl;i++) { pcb->BktNums[i] = acb->Lvl[i].BktNum ; } ;
	if (mmm->Areas[ax].GBPtr != NULL)
	 {
/*	   Make sure nothing is locked which should not be */
	   RELIDX RELDAT
	   if ((mode & (V4IS_PCB_NoLock | V4IS_PCB_IgnoreLock)) == 0)	/* Maybe lock the datarecord */
	    { if ( (pid=v4mm_LockSomething(aid,pcb->DataId,V4MM_LockMode_Write,V4MM_LockIdType_Record,0,&mmm->Areas[ax].LockRec,12))
			< 0)
	       { if ((mode & V4IS_PCB_LockWait) == 0)			/* Do we have lock-wait ? */
	          v4_error(V4E_RECCURLOCK,0,"V4IS","Get","RECCURLOCK","Record locked (v4is_Get) by Pid=%d",-pid) ;
		 HANGLOOSE(250) ; goto get_again ;
	       } ;
	    } ;
	 } ;
	if (!ok) v4_error(V4E_RECTOOBIG,0,"V4IS","CopyData","RECTOOBIG","Area (%d %s) Record too big for user buffer (%d > %d)",
			ax,UCretASC(mmm->Areas[ax].UCFileName),pcb->DataLen,pcb->GetLen) ;
	return(V4RES_xxx) ;
}

/*	v4is_FillDataIdList - Updates DataIdList with dataids in "next" data bucket	*/
/*	Call: ok = v4is_FillDataIdList( aid , acb , dil )
	  where aid is area id,
	  	acb is area's control block,
	  	dil is pointer to V4IS__DataIdList to be updated			*/

LOGICAL v4is_FillDataIdList(aid,acb,dil)
  int aid ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__DataIdList *dil ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__Bkt *dbkt ;
  struct V4IS__DataBktHdr *dbh ;
  struct V4IS__DataBktEntryHdr *dbeh ;
  int ax,index ;

	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return(FALSE) ;			/* Could not find area? */
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++) { if (mmm->Areas[ax].AreaId == aid) break ; } ;
	for(++dil->Bucket;dil->Bucket<acb->RootInfo->NextAvailBkt;dil->Bucket++)	/* Look for next data bucket */
	 { dbkt = v4mm_BktPtr(dil->Bucket,aid,DAT) ; dbh = (DBH *)dbkt ;
	   if (dbh->sbh.BktType == V4_BktHdrType_Data) break ;
	 } ;
	if (dil->Bucket >= acb->RootInfo->NextAvailBkt)
	 return(FALSE) ;							/* End of file */

	dil->Count = 0 ; dil->NextX = 0 ;
	for(index=dbh->FirstEntryIndex;index<dbh->FreeEntryIndex;index+=ALIGN(dbeh->Bytes))
	 { dbeh = (struct V4IS__DataBktEntryHdr *)&dbkt->Buffer[index] ;
	   if (dbeh->Bytes == 0)
	    v4_error(V4E_INVDBEHLENIS0,0,"V4IS","Get","INVDBEHLENIS0","Area (%d %s) Bucket (%d) invalid dbeh->Bytes",
			aid,UCretASC(mmm->Areas[ax].UCFileName),dbh->sbh.BktNum) ;
	   if (dil->Count >= V4IS_DataIdListMax)
	    v4_error(V4E_DILMAX,0,"V4IS","Get","DILMAX","Area (%d %s) Bucket (%d) - max entries in dil->DataIds",
			aid,UCretASC(mmm->Areas[ax].UCFileName),dbh->sbh.BktNum) ;
	   dil->DataIds[dil->Count++] = dbeh->DataId ;
	 } ;
	return(TRUE) ;
}

/*	v4is_Getdbeh - Gets dbeh (maybe updates buffer) from dataid				*/
/*	Call: dbeh = v4is_Getdbeh( aid , ax , dataid , dbehptr , bufptr , buflen )
	  where dbeh is pointer to dbeh corresponding to that dataid, NULL if dataid not found (possible if updated!),
	  	aid, ax are area ids,
	  	dataid is data id to grab							*/

struct V4IS__DataBktEntryHdr *v4is_Getdbeh(aid,ax,dataid)
  int aid,ax,dataid ;
{
  struct V4IS__Bkt *dbkt ;
  struct V4IS__DataBktHdr *dbh ;
  struct V4IS__DataBktEntryHdr *dbeh ;
  struct V4MM__MemMgmtMaster *mmm ;
  union V4MM__DataIdMask dim ; int tdataid ;
  int i,index ;

	FINDAREAINPROCESS(ax,aid) ;
	dim.dataid = dataid ;				/* Decode the DataId */
	dbkt = v4mm_BktPtr(dim.fld.BktNum,aid,DAT) ; dbh = (DBH *)dbkt ; /* Link to data bucket */
	tdataid = dataid ;
	for(i=0;i<mmm->Areas[ax].IndexOnlyCount;i++)	/* See if we should link to another area */
	 { if (dim.fld.BktNum < mmm->Areas[ax].IndexOnly[i].BktOffset) break ; } ;
	if (i > 0)					/* Is bucket number up in range for index-only link ? */
	 { dim.fld.BktNum -= mmm->Areas[ax].IndexOnly[i-1].BktOffset ; tdataid = dim.dataid ; } ;
	for(index=dbh->FirstEntryIndex;index<dbh->FreeEntryIndex;index+=ALIGN(dbeh->Bytes))
	 { dbeh = (struct V4IS__DataBktEntryHdr *)&dbkt->Buffer[index] ;
	   if (dbeh->Bytes == 0)
	    v4_error(V4E_INVDBEHLENIS0,0,"V4IS","Get","INVDBEHLENIS0","Area (%d %s) Bucket (%d) invalid dbeh->Bytes",
			aid,UCretASC(mmm->Areas[ax].UCFileName),dbh->sbh.BktNum) ;
	   if (dbeh->DataId == dataid) return(dbeh) ;
	 } ;
	return(NULL) ;
}

/*	v4is_ErrorCleanup - Called on certain errors to clean up locks and whatever	*/
/*	Call: v4is_ErrorCleanup( pcb )
	  where pcb is parameter control block						*/

void v4is_ErrorCleanup(pcb)
  struct V4IS__ParControlBlk *pcb ;
{ struct V4MM__MemMgmtMaster *mmm ;
  int aid,ax ;

#ifdef SIGNALS
	if (resetsigset)				/* Do we have to reset set of blocked signals ? */
	 { resetsigset = FALSE ; sigprocmask(SIG_SETMASK,&oldsigset,NULL) ; } ;
#endif
	if (pcb == NULL) return ;
//	mmm = V4_GLOBAL_MMM_PTR ;
//	if (pcb->AreaId < 0) return ;			/* Area not yet open? */
	if (pcb->AreaId == V4IS_AreaId_SeqOnly) return ; /* Area sequential? */
	aid = pcb->AreaId ;
	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return ;			/* Could not find area? */
//	for(ax=0;ax<mmm->NumAreas;ax++) { if (mmm->Areas[ax].AreaId == aid) break ; } ;
//	if (ax >= mmm->NumAreas) return ;
	if (mmm->Areas[ax].GBPtr != NULL)
	 { RELIDX RELDAT RELTREE			 /* Make sure nothing is locked which should not be */
	    if (mmm->Areas[ax].LockExtra >= 0)
	     { v4mm_LockRelease(aid,mmm->Areas[ax].LockExtra,mmm->MyPid,aid,"ErrClnExtra") ; mmm->Areas[ax].LockExtra=UNUSED ; } ;
	 } ;
}

/*	v4is_GetData - Copies "current" record into user's buffer		*/
/*	Call: v4is_GetData( aid , bufptr , buflen , mode )
	  where aid is area id,
		bufptr is pointer to user buffer (see mode below),
		buflen is buffer length,
		mode is combo of V4IS_PCB_xxx (if V4IS_PCB_AllocBuf then bufptr is pointer to pointer & buffer is allocated) */

int v4is_GetData(aid,bufptr,buflen,mode)
  int aid ;
  void *bufptr ;
  int buflen,mode ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__Bkt *bkt,*dbkt ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4IS__DataBktHdr *dbh ;
  struct V4IS__DataBktEntryHdr *dbeh ;
  union V4MM__DataIdMask dim ; int tdataid ;
  int ax,i,index ;

	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return(UNUSED) ;			/* Could not find area? */
//	mmm = V4_GLOBAL_MMM_PTR ; ax = aid ;
	acb = v4mm_ACBPtr(aid) ;
	bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,aid,IDX) ;	/* Get bucket pointer */
	ibh = (IBH *)bkt ;				/* Bucket header @ start of bucket */
	ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;	/* Get pointer to key index list below header */
	ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[acb->CurPosKeyIndex]] ;	/* then use to get begin of key entry @ bottom */
	switch (ikde->EntryType)
	 { default: v4_error(V4E_INVIKDE,0,"V4IS","GetRec","INVIKDE","Invalid IKDE Entry Type (%d) in Bucket %d, AreaID %d",
					ikde->EntryType,ibh->sbh.BktNum,aid) ;
	   case V4IS_EntryType_IndexLink:
		v4_error(V4E_SNBINDEXLINK,0,"V4IS","Get","SNBINDEXLINK","Positioned at IndexLink key- should not be here") ;
	   case V4IS_EntryType_DataLink:
/*		Data in another bucket - link & grab */
		dbkt = v4mm_BktPtr(ikde->AuxVal,aid,DAT) ;			/* Link to data bucket */
		IBHINUSE ;
		dbh = (DBH *)dbkt ;			/* Data bucket header = begin of data bkt */
/*			Scan thru data bucket for wanted data entry */
			tdataid = ikde->DataId ; dim.dataid = ikde->DataId ;
			for(i=0;i<mmm->Areas[ax].IndexOnlyCount;i++)	/* See if we should link to another area */
			 { if (dim.fld.BktNum < mmm->Areas[ax].IndexOnly[i].BktOffset) break ; } ;
			if (i > 0)					/* Is bucket number up in range for index-only link ? */
			 { dim.fld.BktNum -= mmm->Areas[ax].IndexOnly[i-1].BktOffset ; tdataid = dim.dataid ; } ;
			for(index=dbh->FirstEntryIndex;index<dbh->FreeEntryIndex;index+=ALIGN(dbeh->Bytes))
			 { dbeh = (struct V4IS__DataBktEntryHdr *)&dbkt->Buffer[index] ;
			   if (dbeh->Bytes == 0)
			    v4_error(V4E_INVDBEHLEN,0,"V4IS","Get","INVDBEHLEN","Area (%d %s) Bucket (%d) invalid dbeh->Bytes",
					aid,UCretASC(mmm->Areas[ax].UCFileName),dbh->sbh.BktNum) ;
			   if (dbeh->DataId != tdataid)continue ;
			   v4is_CopyData(aid,bufptr,buflen,(char*)dbeh + sizeof *dbeh,dbeh->Bytes-sizeof *dbeh,
						dbeh->CmpMode,NULL,mode) ;
			   DBHINUSE ;
			   goto end_get ;
			 } ;
			v4_error(V4E_BADDATAID,0,"V4IS","GetData","BADDATAID","Data record associated with dataid (%x) cannot be found",
					ikde->DataId) ;
	 } ;
end_get:
	return(V4RES_xxx) ;
}

#define V4IS_DATAMODE_AUTO 1		/* Have V4 determine where to place data */
#define V4IS_DATAMODE_LCL 2		/* Place data after key in directory bucket */
#define V4IS_DATAMODE_RMT 4		/* Place data in remote bucket (allows more keys in directory bucket) */
#define V4IS_DATAMODE_CMP1 8		/* Compress data before writing to bucket (method 1) */

/*	v4is_Insert - Inserts record into specified area		*/
/*	Call: ndataid = v4is_Insert( aid , primekey , bufptr , bufbytes , datamode , cmpmode , dataid , multkeys , dupok , linkdataid , errmsg)
	  where ndataid is is DataId of new data record (only if bufptr/bufbytes set) (0 to stash key), (-1 if error),
		aid is area id,
		primekey is pointer to primary key,
		bufptr is pointer to buffer being written (if NULL then just stash key in index with link to acb->DataID),
		bufbytes is number of bytes,
		datamode determines where data is to be placed (index or data buckets)
		cmpmode is data compression mode,
		dataid if nonzero is dataid of data that is actually being replaced - try to reuse as much as possible,
		multkeys is TRUE if this is primary key and others to follow,
		dupok is TRUE if duplicates allowed on key,
		linkdataid is dataid to use for alternate key link to data (bufptr,bufbytes s/b null/0 if this is set!),
		errmsg if not NULL then updated with error message & v4is_Insert returns dataid of -1			 */

int v4is_Insert(aid,primekey,bufptr,bufbytes,datamode,cmpmode,dataid,multkeys,dupok,linkdataid,errmsg)
  int aid ;
  struct V4IS__Key *primekey ;
  void *bufptr ;
  UCCHAR *errmsg ;
  int bufbytes,datamode,cmpmode,dataid,multkeys,dupok,linkdataid ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__Bkt *bkt ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__Key *key ;
  struct V4IS__Key skey ;
  struct V4IS__IndexBktHdr *ibh ;
  char ebuf[250] ;
  int oldbktnum,childbktnum,ax,lx,res,firsttime,dataok,fileref,parentbkt,actparentbkt,ndataid ;

//if (primekey->KeyMode == 11)
// printf("mooooo.................\n") ;

	firsttime = TRUE ; dataok = FALSE ;
	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return(UNUSED) ;			/* Could not find area? */
//	ax = aid ;
//	mmm = V4_GLOBAL_MMM_PTR ;
	acb = v4mm_ACBPtr(aid) ;
	if (bufbytes > acb->RootInfo->MaxRecordLen)
	 { if (errmsg == NULL)
	    { v4_error(V4E_RECEXCEEDBKT,0,"V4IS","Insert","RECEXCEEDBKT","Record size (%d) exceeds that allowed for given bucket size (%d) in area (%d)",
			bufbytes,acb->RootInfo->BktSize,aid) ;
	    } else { v_Msg(NULL,errmsg,"V4ISRecSize",bufbytes,mmm->Areas[ax].UCFileName,acb->RootInfo->BktSize) ; return(-1) ; } ;
	 } ;
	RELIDX RELDAT
	v4mm_LockSomething(aid,V4MM_LockId_Tree,V4MM_LockMode_Write,V4MM_LockIdType_Tree,34,&mmm->Areas[ax].LockTree,13) ;
try_again:							/* Be able to start all over again if index is split */
	if (dataid != 0 && bufptr != NULL && firsttime)		/* Replacing existing record ? */
	 { if ((res = v4is_PositionKey(aid,primekey,NULL,dataid,V4IS_PosBoKL)) != V4RES_PosKey)	/* Look for existing */
	    { if (errmsg == NULL)
	       { v4_error(V4E_NOKEYPOSTODEL,0,"V4IS","Insert","NOKEYPOSTODEL","Could not position to key in index to delete") ;
	       } else { RELIDX RELDAT RELTREE v_Msg(NULL,errmsg,"@Could not position to key in index to delete") ; return(-1) ; } ;
	    } ;
	 } else { res = v4is_PositionKey(aid,primekey,NULL,0,V4IS_PosEoKL) ; } ;	/* Position index */
	firsttime = FALSE ;
	switch (res)
	 { default: v4_error(V4E_BADPOSKEYRES,0,"V4IS","Insert","BADPOSKEYRES","Unknown result code from PositionKey (%d)",res) ;
	   case V4RES_PosKey:
		if (dupok)					/* Are duplicates allowed ? */
		 { acb->CurPosKeyIndex ++ ; break ; } ;		/* Increment so that new duplicate always at end of chain */
		if (dataid == 0)
		 { if (errmsg == NULL)
		    { v4_error(V4E_INSRECEXISTS,0,"V4IS","Insert","INSRECEXISTS","Area(%d) - Record already exists with specified key (%s)",
				acb->AreaId,v4is_FormatKey(primekey)) ;
		    } else
		    { v_Msg(NULL,errmsg,"@Area(%1d) - Record already exists with specified key (%2s)",acb->AreaId,v4is_FormatKey(primekey)) ; return(-1) ;
		    } ;
		 } ;
/*		v4is_PositionKey(aid,primekey,NULL,dataid,V4IS_PosBoKL) ; Why do we need this??? */
	   IFTRACEAREA
	    { sprintf(ebuf,"Insert: RemoveIndexEntry(bkt=%d pos=%d key=%s) dataid=%x",
			acb->Lvl[acb->CurLvl].BktNum,acb->CurPosKeyIndex,v4is_FormatKey(primekey),dataid) ;
	    } ;
	   	v4is_RemoveIndexEntry(NULL,aid,acb->CurLvl,acb->CurPosKeyIndex,NULL) ;
		dupok = TRUE ;					/* Set OK so next positionkey will put at end of dup chain */
		goto try_again ;
	   case V4RES_PosNext: break ;
	 } ;
	if (bufptr == NULL)						/* Are we just stashing key or do we have data record ? */
	 { if (v4is_StashIndexKey(aid,primekey,multkeys,linkdataid)) return(0) ;	/* Just try to stash key */
	 } else								/* Try to store data in data bucket */
	 { switch (primekey->KeyType)
	    { default:v4_error(V4E_BADPRIMEKEYTYPE,0,"V4IS","Insert","BADPRIMEKEYTYPE","Invalid primary key type (%d)",primekey->KeyType) ;
	      case V4IS_KeyType_FrgnKey1: case V4IS_KeyType_FrgnKey2: case V4IS_KeyType_FrgnKey3: case V4IS_KeyType_FrgnKey4:
	      case V4IS_KeyType_FrgnKey5: case V4IS_KeyType_FrgnKey6: case V4IS_KeyType_FrgnKey7:	fileref = primekey->AuxVal ; break ;
//	      case V4IS_KeyType_FrgnKey5:	fileref = primekey->AuxVal ; break ;
	      case V4IS_KeyType_V4Segments: case V4IS_KeyType_V4: case V4IS_KeyType_AggShell: case V4IS_KeyType_xdbRow:
	      case V4IS_KeyType_Binding:	fileref = 0 ; break ;
	    } ;
	   if (!dataok)						/* If data not stashed, then try in data bucket */
	    { dataok = v4is_StashData(aid,bufptr,bufbytes,datamode,cmpmode,dataid,fileref,0) ; ndataid = acb->DataId ; } ;
	   if (dataok)						/* If data stashed then try for keys else key+data */
	    { if (v4is_StashIndexKey(aid,primekey,multkeys,ndataid)) goto insert_done ;
	    } else { if (v4is_StashIndexData(aid,primekey,bufptr,bufbytes,cmpmode,multkeys,dataid)) goto insert_done ; } ;
	 } ;
/*	No room in current index bucket - have to create new bucket, link to parent, etc. */
	bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,aid,IDX+UPD) ;	/* Get current index */
	ibh = (IBH *)bkt ; oldbktnum = ibh->sbh.BktNum ;
	if (ibh->sbh.BktType == V4_BktHdrType_Root)			/* Is this the root (i.e. no parent?) */
	 { childbktnum = v4is_IndexSplit(aid,bkt,ibh->KeyEntryNum/2,NULL,-1) ;	/* Copy 1/2 of the entries into new index */
	   IBHINUSE ;
	   for(lx=acb->CurLvl;lx>0;lx--)
	    { acb->Lvl[lx+1].BktNum = acb->Lvl[lx].BktNum ; } ;		/* Drop everything down a level */
	   acb->CurLvl++ ; acb->RootInfo->NumLevels ++ ;		/* Remember # of levels!! */
	   acb->Lvl[1].BktNum = childbktnum ;				/* Make new index directly below root */
	   bkt = v4mm_BktPtr(childbktnum,aid,IDX+UPD) ;			/* Link up to child */
	   ibh = (IBH *)bkt ; parentbkt = ibh->ParentBktNum ;
	   ibh->ParentBktNum = 0 ;					/* Parent is root */
	   ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
	   key = (KEY *)((char *)&bkt->Buffer[ibkl->DataIndex[0]] + sizeof(struct V4IS__IndexKeyDataEntry)) ; /* Get pointer to first key */
	   memcpy(&skey,key,key->Bytes) ;
	   v4is_IndexStashLink(aid,0,&skey,V4IS_StashAt_End,"Insert",&actparentbkt) ;	/* Insert first as link from parent */
	   IBHINUSE ;
	   if (actparentbkt != parentbkt)
	    { bkt = v4mm_BktPtr(childbktnum,aid,IDX+UPD) ;			/* Link up to child */
	      ibh = (IBH *)bkt ; ibh->ParentBktNum = 0 ;			/* Parent is root */
	    } ;
	   goto try_again ;
	 } ;
	if (ibh->KeyEntryNum <= 1)
	 v4_error(V4E_BKTTOOSMALL,0,"V4IS","Insert","BKTTOOSMALL","Bucket size is too small for data-in-index Insert") ;
	childbktnum = v4is_IndexSplit(aid,bkt,ibh->KeyEntryNum/2,NULL,-1) ;/* Copy 1/2 of the entries into new index */
	IBHINUSE ;
	bkt = v4mm_BktPtr(childbktnum,aid,IDX+UPD) ;			/* Link up to child */
	ibh = (IBH *)bkt ; parentbkt = ibh->ParentBktNum ;
	acb->Lvl[acb->CurLvl].BktNum = childbktnum ;			/* Link in new child bucket */
	ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
	if (ibh->KeyEntryNum > 0)					/* Get pointer to key- if bucket empty use new */
	 { key = (KEY *)((char *)&bkt->Buffer[ibkl->DataIndex[0]] + sizeof(struct V4IS__IndexKeyDataEntry)) ; }
	 else { key = (KEY *)primekey ; } ;
	memcpy(&skey,key,key->Bytes) ;
	v4is_IndexStashLink(aid,acb->CurLvl-1,&skey,oldbktnum,"Insert1",&actparentbkt) ; /* Insert first as link from parent */
	if (parentbkt != actparentbkt)
	 { bkt = v4mm_BktPtr(childbktnum,aid,IDX+UPD) ; ibh = (IBH *)bkt ; ibh->ParentBktNum = actparentbkt ; } ;
	IBHINUSE ;
	goto try_again ;						/* Plugging */
insert_done:
	RELIDX RELDAT RELTREE
	return(ndataid) ;						/* Return DataId of new data record */
}

/*	v4is_Replace - Replaces existing record with new version	*/
/*	Call: v4is_Replace( aid , keyptr, origptr, bufptr , buflen , datamode , cmpmode , dataid , fileref )
	  where aid is current area,
		keyptr is pointer to key,
		origptr is pointer to original (uncompacted) data (for comparing keys),
		bufptr/buflen is pointer to key-record,
		datamode is where data is to reside (index or data),
		cmpmode is data compression mode,
		dataid is 0 for current record, old dataid to replace old record,
		fileref is 0 for internal records, +n for fileref (foreign records)		*/

void v4is_Replace(aid,keyptr,origptr,bufptr,buflen,datamode,cmpmode,dataid,fileref)
  int aid ;
  struct V4IS__Key *keyptr ;
  void *origptr ; void *bufptr ;
  int buflen ; int datamode,cmpmode ; int dataid ; int fileref ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__Bkt *bkt ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__DataBktHdr *dbh ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__DataBktEntryHdr *dbeh ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__Key *key ;
  union V4IS__KeyPrefix *kp1,*kp2 ;
  union V4MM__DataIdMask dim ;
  struct V4IS__KeyCmpList kcl ;
  struct V4FFI__KeyInfo *ki ;
  struct V4FFI__KeyInfoDtl *kid ;
  struct {
    union V4IS__KeyPrefix kp ;
    char KeyVal[V4IS_KeyBytes_Max] ;
   } lclkey1,lclkey2 ;
  char ebuf[250] ;
  int ax,kx,index,ndataid,curindata ; char *dataptr ;

	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return ;			/* Could not find area? */
//	ax = aid ; mmm = V4_GLOBAL_MMM_PTR ;
	acb = v4mm_ACBPtr(aid) ;
	if (dataid == 0) dataid = acb->DataId ;
	if (dataid == -1)
	 v4_error(V4E_NOCURRECREP,0,"V4IS","Replace","NOCURRECREP","No current record to replace") ;
	kcl.count = 0 ;						/* Reset key-compare-list */
	kcl.DataId = dataid ; curindata = FALSE ;
	dim.dataid = dataid ;					/* Decode the DataId */
	bkt = v4mm_BktPtr(dim.fld.BktNum,aid,DAT) ; dbh = (DBH *)bkt ; /* Link to data bucket */
/*	Before we get too carried away, let's make sure we actually have a data bucket */
	switch (dbh->sbh.BktType)
	 { default: v4_error(V4E_INVBKTTYPE,0,"V4IS","Get","INVBKTTYPE","Unknown bucket type-4 (%d)",dbh->sbh.BktType) ;
	   case V4_BktHdrType_Data:
/*		Scan thru data bucket for wanted data entry */
		for(index=dbh->FirstEntryIndex;index<dbh->FreeEntryIndex;index+=ALIGN(dbeh->Bytes))
		 { dbeh = (struct V4IS__DataBktEntryHdr *)&bkt->Buffer[index] ;
		   if (dbeh->Bytes == 0)
		    v4_error(V4E_INVDBEHLEN,0,"V4IS","Get","INVDBEHLEN","Area (%d %s) Bucket (%d) invalid dbeh->Bytes",
				aid,UCretASC(mmm->Areas[ax].UCFileName),dbh->sbh.BktNum) ;
		   if (dbeh->DataId != dataid) continue ;
		   v4is_CopyData(aid,&dataptr,0,(char*)dbeh+sizeof *dbeh,dbeh->Bytes-sizeof *dbeh,
					dbeh->CmpMode,NULL,V4IS_PCB_RetPtr) ;
		   curindata = TRUE ; goto got_dataptr ;
		 } ;
		v4_error(V4E_BADDATAID,0,"V4IS","Replace","BADDATAID","Data record associated with dataid (%x) cannot be found",dataid) ;
	   case V4_BktHdrType_Root:
	   case V4_BktHdrType_Index:
   		ibh = (IBH *)bkt ;
		ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;	/* Pointer to B header & Key indexes */
		for(index=0;index<ibh->KeyEntryNum;index++)
		 { ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[index]] ;
      		   switch(ikde->EntryType)
	 	    { default: v4_error(V4E_INVIKDE,0,"V4IS","GetRec","INVIKDE","Invalid IKDE Entry Type (%d) in Bucket %d, AreaID %d",
					ikde->EntryType,ibh->sbh.BktNum,aid) ;
		      case V4IS_EntryType_IndexLink:	continue ;
		      case V4IS_EntryType_DataLink:	continue ;
		      case V4IS_EntryType_DataLen:
			   if (ikde->DataId != dataid) continue ;	/* Is this the one we want? */
			   if (ikde->KeyNotInRec)
			    { key = (KEY *)((char *)ikde + sizeof *ikde) ;
			      v4is_CopyData(aid,&dataptr,0,(char *)ikde+sizeof *ikde+key->Bytes,
					 ikde->AuxVal-sizeof *ikde-key->Bytes,ikde->CmpMode,NULL,V4IS_PCB_RetPtr) ;
			    } else v4is_CopyData(aid,&dataptr,0,(char *)ikde+sizeof *ikde,ikde->AuxVal-sizeof *ikde,
						 ikde->CmpMode,NULL,V4IS_PCB_RetPtr) ;
			   goto got_dataptr ;
       		    } ;
		  } ;
		v4_error(V4E_DATAIDNOTIDX,0,"V4IS","GetRec","DATAIDNOTIDX","Data record with dataid (%x) cannot be found in index",dataid) ;
	 } ;
got_dataptr:
	if (fileref != 0)
	 { if ((ki = acb->KeyInfo) == NULL)
	    v4_error(V4E_AREANOFFI,0,"V4IS","Get","AREANOFFI","No foreign file key information in area") ;
	   for((kid=(KID *)&ki->Buffer,kx=0);kx<ki->Count;(kx++,kid=(KID *)(kid+kid->Bytes)))	/* Look for fileref */
	    { if (kid->FileRef != fileref) continue ;
	      for(kx=0;kx<kid->KeyCount;kx++)				/* Loop thru all secondary keys */
	       { if (kid->Key[kx].KeyPrefix.all == 0)			/* Have to construct V4 key from key in buffer ? */
	          { kp1 = (KP *)((char *)origptr + kid->Key[kx].Offset) ;
		    kp2 = (KP *)((char *)dataptr + kid->Key[kx].Offset) ;/* Get pts to keys in rec (V4 keys) */
	          } else
	          { lclkey1.kp.all = kid->Key[kx].KeyPrefix.all ; lclkey2.kp.all = lclkey1.kp.all ;
		    memcpy(lclkey1.KeyVal,(char *)origptr+kid->Key[kx].Offset,lclkey1.kp.fld.Bytes) ;
		    memcpy(lclkey2.KeyVal,(char *)dataptr+kid->Key[kx].Offset,lclkey2.kp.fld.Bytes) ;
		    if (lclkey1.kp.fld.Bytes & 0x3)				/* If key not aligned then pad and align */
		     { memset(&lclkey1.KeyVal[lclkey1.kp.fld.Bytes],0,4) ; memset(&lclkey2.KeyVal[lclkey2.kp.fld.Bytes],0,4) ; } ;
		    lclkey1.kp.fld.Bytes = ALIGN(sizeof kid->Key[0].KeyPrefix + lclkey1.kp.fld.Bytes) ;
		    lclkey2.kp.fld.Bytes = ALIGN(sizeof kid->Key[0].KeyPrefix + lclkey2.kp.fld.Bytes) ;
		    kp1 = (KP *)&lclkey1 ; kp2 = (KP *)&lclkey2 ;
	          } ;
		 memcpy(&kcl.Entry[kcl.count].OKey,kp2,kp2->fld.Bytes) ;
		 memcpy(&kcl.Entry[kcl.count].NKey,kp1,kp1->fld.Bytes) ;
		 kcl.Entry[kcl.count].Primary = (kcl.count == 0 ? TRUE : FALSE) ;
		 kcl.Entry[kcl.count].Differ = (memcmp(kp1,kp2,kp1->fld.Bytes) != 0) ;
		 kcl.Entry[kcl.count].DupsOK = kid->Key[kx].DupsOK ;
		 kcl.count ++ ;
	       } ;
	      if (kid->KeyCount > 1)
	       { if (datamode == V4IS_PCB_DataMode_Index)
		  v4_error(V4E_PUTIDXMLTKEY,0,"V4IS","Put","PUTIDXMLTKEY","Cannot PUT data in index bucket with multiple keys (%s)",
				UCretASC(mmm->Areas[ax].UCFileName)) ;
		 datamode = V4IS_PCB_DataMode_Data ;			/* Force data into data bucket */
	       } ;
	      break ;
	    } ;
	 } else			 /* Got internal key- begin of record buffer is key */
	 { kp2 = (KP *)dataptr ; memcpy(&kcl.Entry[kcl.count].OKey,dataptr,kp2->fld.Bytes) ;
	   kp1 = (KP *)keyptr ; memcpy(&kcl.Entry[kcl.count].NKey,keyptr,kp1->fld.Bytes) ;
	   kcl.Entry[kcl.count].StartIndex = 0 ; kcl.Entry[kcl.count].Primary = TRUE ;
	   kcl.Entry[kcl.count].Differ = (memcmp(kp2,kp1,kp1->fld.Bytes) != 0) ;
	   kcl.count ++ ;
	 } ;
	DBHINUSE ; RELDAT RELIDX
	v4mm_LockSomething(aid,V4MM_LockId_Tree,V4MM_LockMode_Write,V4MM_LockIdType_Tree,36,&mmm->Areas[ax].LockTree,14) ;
	for(kx=0;kx<kcl.count;kx++)
	 { if (kcl.Entry[kx].Primary && kcl.Entry[kx].Differ)
	    { printf("OKey = %s\n",v4is_FormatKey(&kcl.Entry[kx].OKey)) ;
	      printf("  NewKey = %s, len=%d, dataid=%x, fileref=%d\n",
		v4is_FormatKey(&kcl.Entry[kx].NKey),buflen,dataid,fileref) ;
	      v4is_ExamineBkt(dim.fld.BktNum,aid) ;
	      v4_error(V4E_CHNGPRIMONREP,0,"V4IS","Replace","CHNGPRIMONREP","Cannot change primary key on REPLACE") ;
            } ;
	   if (!kcl.Entry[kx].Differ) continue ;			/* Key is the same */
	   if (v4is_PositionKey(aid,&kcl.Entry[kx].OKey,NULL,dataid,V4IS_PosBoKL) != V4RES_PosKey)
	    v4_error(V4E_NOKEYPOSTODEL,0,"V4IS","Replace","NOKEYPOSTODEL","Could not position to key in index to delete") ;
	   IFTRACEAREA
	    { sprintf(ebuf,"Replace: RemoveIndexEntry(bkt=%d pos=%d key=%s) dataid=%x kx=%d",
			acb->Lvl[acb->CurLvl].BktNum,acb->CurPosKeyIndex,v4is_FormatKey(&kcl.Entry[kx].OKey),dataid,kx) ;
	    } ;
	   v4is_RemoveIndexEntry(NULL,aid,acb->CurLvl,acb->CurPosKeyIndex,NULL) ;
	 } ;
	RELTREE
	if ( (datamode == V4IS_PCB_DataMode_Auto || datamode == V4IS_PCB_DataMode_Data) && curindata)
	 { ndataid = v4is_StashData(aid,bufptr,buflen,datamode,cmpmode,kcl.DataId,fileref,0) ;
	   acb->DataId = ndataid ;				/* If data in data bucket (most cases!) then just try to replace */
	 } else							/* Data not in data bucket - have to re-insert */
	 { curindata = FALSE ;
	   ndataid = v4is_Insert(aid,keyptr,bufptr,buflen,datamode,cmpmode,kcl.DataId,(kcl.count > 1 ? TRUE : FALSE),FALSE,0,NULL) ;
	 } ;
	for(kx=0;kx<kcl.count;kx++)
	 { if (kcl.Entry[kx].Primary && !curindata) continue ;		/* Primary already take care of */
	   acb->DataId = ndataid ;					/* Make sure up-to-date */
	   if (kcl.Entry[kx].Differ)
	    { v4is_Insert(aid,&kcl.Entry[kx].NKey,NULL,0,0,0,0,FALSE,kcl.Entry[kx].DupsOK,ndataid,NULL) ; /* Insert other key */
	    } else
	    { if (ndataid != kcl.DataId)
	       v4is_ReplaceDataId(aid,&kcl.Entry[kx].OKey,ndataid,kcl.DataId) ;/* Replace data id link with new link */
	    } ;
	 } ;
}

/*	D A T A   B U C K E T   M O D U L E S				*/

/*	v4is_StashData - Maybe stores data record in current/next data buffer */
/*	Call: newdataid = v4is_StashData( aid , dataptr , databytes , datamode , cmpmode , dataid , fileref , fillpercent )
	  where newdataid = data id where data stashed, if dataid != 0 then newdataid == dataid if fit in same bucket
		aid is area id,
		dataptr/databytes is pointer & bytes in data record,
		datamode is where data to be placed,
		cmpmode is data compression mode,
		dataid- if non zero then attempt to rewrite this record with same dataid,
		fileref is fileref associated with this record,
		fillpercent is 0-100 indicating allowed bucket fill (0 = 100%) */

int v4is_StashData(aid,dataptr,databytes,datamode,cmpmode,dataid,fileref,fillpercent)
  int aid ;
  void *dataptr ;
  int databytes ; int datamode ; int cmpmode ; int dataid ; int fileref ; int fillpercent ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__GlobalBuf *glb ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__RootInfo *ri ;
  struct V4IS__Bkt *dbkt ;
  struct V4IS__DataBktHdr *dbh ;
  struct V4IS__DataBktEntryHdr *dbeh ;
  union V4MM__DataIdMask dim ;
  int ax,indata,newdata,num,dux,fillfactor ;


	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return(FALSE) ;			/* Could not find area? */
//	ax = aid ; mmm = V4_GLOBAL_MMM_PTR ;
	acb = v4mm_ACBPtr(aid) ;
	ri = acb->RootInfo ; newdata = TRUE ; dim.dataid = dataid ;
	if (mmm->Areas[ax].SegId != 0)				/* Global area ? */
	 { GETGLB(ax) ;
	   if (glb != NULL && dataid != 0)
	    { dux = (glb->LastDataIdUpdCnt++) % V4MM_GlobalBufUpd_Max ;
	      glb->DataUpd[dux].DataIdRmv = dataid ; glb->DataUpd[dux].DataIdNew = 0 ;
	    } ;
	 } else glb = NULL ;
/*	If dataid <> 0 then try to delete existing record with that ID */
	if (dataid != 0)
	 { newdata = FALSE ;
	   dbkt = v4mm_BktPtr(dim.fld.BktNum,aid,DAT+UPD) ;
	   dbh = (DBH *)dbkt ;			/* Grab bucket associated with this ID */
	   if (dbh->sbh.BktType == V4_BktHdrType_Data)
	    { v4is_RemoveData(aid,dbkt,dataid,FALSE) ; v4is_DataBktFree(aid,acb,(struct V4IS__DataBktHdr *)dbkt,FALSE) ; DBHINUSE ; RELDAT
	      acb->RootInfo->RecordCount-- ;			/* Decrement record count for area */
	    } else { DBHINUSE ; return(FALSE) ; } ;		/* Data originally in index - keep it that way */
	 } ;
/*	Are we storing data in index bucket or in separate data bucket ? */
	switch (datamode)
	 { default:
		indata = (databytes > datamode ? TRUE : FALSE) ; 	/* Do data if buffer larger than user cutoff */
		break ;
	   case V4IS_PCB_DataMode_Auto:
		indata = TRUE ; break ;					/* By default, put data in data bucket */
	   case V4IS_PCB_DataMode_Index:
		return(FALSE) ;
	   case V4IS_PCB_DataMode_Data:
		indata = TRUE ; break ;
	 } ;
	if (!indata && dataid == 0) return(FALSE) ;			/* Data does not go here */

	fillfactor = 100 - (fillpercent < 25 || fillpercent > 100 ? 100 : fillpercent) ; /* Figure out free bytes for expansion */
	fillfactor = (ri->BktSize * fillfactor) / 100 ;			/* fillfactor = bytes to keep free */
/*	If first time then may not have a data bucket - make one! */
	if (dataid != 0)
	 { dim.dataid = dataid ; dbkt = v4mm_BktPtr(dim.fld.BktNum,aid,DAT+UPD) ;
	   dbh = (struct V4IS__DataBktHdr *)dbkt ;			/* If replacing existing, try same bucket */
	   if (dbh->FreeBytes < ALIGN(databytes) + sizeof *dbeh)
	    { num = v4is_DataBktAvail(aid,acb,ALIGN(databytes)+sizeof *dbeh,fillfactor) ;
	      newdata = TRUE ;
	      if (num >= 0)						/* Get something ? */
	       { dbkt = v4mm_BktPtr(num,aid,DAT+UPD) ; dbh = (DBH *)dbkt ; } ;
	    } ;
	 } else
	 { num = v4is_DataBktAvail(aid,acb,ALIGN(databytes)+sizeof *dbeh,fillfactor) ; /* See if we can scrounge space somewhere */
	   if (num < 0)								/* Get something ? */
	    { dbkt = v4is_MakeBkt(aid,V4_BktHdrType_Data) ;			/*  NO- allocate new bucket */
	      dbh = (DBH *)dbkt ; DBHINUSE ; num = dbh->sbh.BktNum ;
/*	      num = v4is_DataBktAvail(aid,acb,ALIGN(databytes)+sizeof *dbeh,fillfactor) ; */
	    } ;
	   dbkt = v4mm_BktPtr(num,aid,DAT+UPD) ;			/*   Yes! */
	   dbh = (DBH *)dbkt ;
	 } ;
/*	Is there room in this bucket, or have we max'ed out on DataId sequence numbers ? */
	if (dbh->FreeBytes < ALIGN(databytes) + sizeof *dbeh || dbh->sbh.AvailBktSeq >= V4MM_MaxDataIdSeq)
	 { if (dbh->sbh.BktType == V4_BktHdrType_Data)
	    { DBHINUSE ;
#ifdef FLUSH
	      v4mm_BktFlush(dbh->sbh.BktNum,aid) ;	/* Flush data bucket which is full */
#endif
	    } else DBHINUSE ;
	   dbkt = v4is_MakeBkt(aid,V4_BktHdrType_Data) ;			/* Create a new bucket */
	   dbh = (DBH *)dbkt ; newdata = TRUE ;
	 } ;
/*	Store data in data bucket, & remember bucket number & data id */
	if (dbh->FirstEntryIndex == 0) dbh->FirstEntryIndex = dbh->FreeEntryIndex ;
	dbeh = (struct V4IS__DataBktEntryHdr *)&dbkt->Buffer[dbh->FreeEntryIndex] ;
	dbeh->DataId = (newdata ? v4mm_MakeDataId(dbkt) : dataid) ;	/* Assign new data id */
	acb->DataId = dbeh->DataId ;
	dbeh->CmpMode = cmpmode ;					/* Save data compression mode */
	dbeh->Bytes = databytes+sizeof *dbeh ;
	dbeh->FileRef = fileref ;					/* Save FileRef for possible disaster recovery */
	memcpy(((char *)dbeh)+sizeof *dbeh,dataptr,databytes) ;		/* Copy data into data bucket */
	dbh->FreeBytes -= ALIGN(dbeh->Bytes) ;
/*	Increment FreeEntryIndex - need to check to see if = V4IS_BktSize_Max, if so set to V4IS_BktSize_Max-1 */
/*	  otherwise FreeEntryIndex becomes 0 because we lose the high order bit! (VEH050620) */
	if (dbh->FreeEntryIndex + ALIGN(dbeh->Bytes) >= V4IS_BktSize_Max)
	 { dbh->FreeEntryIndex = V4IS_BktSize_Max - 1 ; }
	 else { dbh->FreeEntryIndex += ALIGN(dbeh->Bytes) ; } ;
	if (glb != NULL && dataid != 0) glb->DataUpd[dux].DataIdNew = dbeh->DataId ;	/* Record (new) dataid */
/* if (glb != NULL)
 printf("%x => %x @ %d %d\n",glb->DataUpd[dux].DataIdRmv,glb->DataUpd[dux].DataIdNew,dux,glb->LastDataIdUpdCnt) ; */
	v4is_DataBktFree(aid,acb,dbh,FALSE) ; DBHINUSE ; RELDAT
	acb->RootInfo->RecordCount++ ;			/* Up record count for area */
	return(acb->DataId) ;						/* Return new(old) dataid */
}

/*	v4is_DataBktFree - Record number of bytes remaining in this data bucket for possible use later */
/*	Call: slot = v4is_DataBktFree( aid , acb , dbh , lockflag )
	  where slot is slot position used (-1 if not used),
		aid is current area id,
		acb is current area control block pointer,
		dbh is pointer to current data bucket,
		lockflag is TRUE if root already locked (as from MakeBkt routine)	*/

int v4is_DataBktFree(aid,acb,dbh,lockflag)
  int aid ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__DataBktHdr *dbh ;
  LOGICAL lockflag ;
{ struct V4MM__MemMgmtMaster *mmm ;
  int i,mini,min,ax,lrx ;

	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return(-1) ;			/* Could not find area? */
//	ax = aid ; mmm = V4_GLOBAL_MMM_PTR ;
	v4mm_BktUpdated(aid,0) ;
	if (!lockflag) { LOCKROOT ; } ;
	for(i=0;i<acb->FreeData->count;i++)			/* First see if we already have this bucket */
	 { if (dbh->sbh.BktNum == acb->FreeData->Entry[i].DataBktNum)
	    { acb->FreeData->Entry[i].FreeBytes = dbh->FreeBytes ;
	      if (!lockflag) { RELROOT("DataFree1") ; } ; return(i) ;
	    } ;
	 } ;
	if (acb->FreeData->count <V4IS_FreeDataInfo_Max)
	 { i = (acb->FreeData->count ++) ;
	   acb->FreeData->Entry[i].FreeBytes = dbh->FreeBytes ; acb->FreeData->Entry[i].DataBktNum = dbh->sbh.BktNum ;
	   if (!lockflag ) { RELROOT("DataFree2") ; } ; return(i) ;
	 } ;
	for(i=0,min=V4LIM_BiggestPositiveInt;i<acb->FreeData->count;i++)		/* Now see if this is greater than what we got */
	 { if (acb->FreeData->Entry[i].FreeBytes > min) continue ;
	   min = acb->FreeData->Entry[i].FreeBytes ; mini = i ;
	 } ;
	if (dbh->FreeBytes < min)
	 { if (!lockflag) { RELROOT("DataFree3") ; } ; return(-1) ; } ;		/* Min on record bigger than what we got now */
	acb->FreeData->Entry[mini].FreeBytes = dbh->FreeBytes ; acb->FreeData->Entry[mini].DataBktNum = dbh->sbh.BktNum ;
	if (!lockflag ) { RELROOT("DataFree4") ; } ; return(mini) ;
}

/*	v4is_DataBktAvail - Returns bucket number of data bucket capable of holding specified number of bytes	*/
/*	Call: bktnum = v4is_DataBktAvail( aid , acb , bytes , freebytes )
	  where bktnum is data bucket to use (-1 if none available),
		aid is area id,
		acb is pointer to area's control block,
		bytes is number of bytes we want,
		freebytes = number of free bytes to keep in bucket */

int v4is_DataBktAvail(aid,acb,bytes,freebytes)
  int aid ;
  struct V4IS__AreaCB *acb ;
  int bytes ; int freebytes ;
{ struct V4MM__MemMgmtMaster *mmm ;
  int i,mini,min,ax,lrx ;

	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return(UNUSED) ;			/* Could not find area? */
//	ax = aid ; mmm = V4_GLOBAL_MMM_PTR ;
	v4mm_BktUpdated(aid,0) ;
	for(i=0,mini=(-1),min=V4LIM_BiggestPositiveInt;i<acb->FreeData->count;i++)	/* Look for smallest which is > bytes */
	 { if (acb->FreeData->Entry[i].FreeBytes < (bytes+freebytes)) continue ; /* Too small */
	   if (acb->FreeData->Entry[i].FreeBytes >= min) continue ;	/* Bigger than current "best" */
	   min = acb->FreeData->Entry[i].FreeBytes ; mini = i ;
	 } ;
	if (mini == -1) return(-1) ;					/* Nothing fits - have to allocate at "EOF" */
	LOCKROOT ; acb->FreeData->Entry[mini].FreeBytes -= bytes ; RELROOT("DataBktAvail") ;
	return(acb->FreeData->Entry[mini].DataBktNum) ;
}

/*	v4is_CopyData - Copies data from bucket back to user buffer	*/
/*	Call: logical = v4is_CopyData( aid , bufptr , buflen , dataptr , datalen , cmpmode , pcb , mode)
	  where	logical is TRUE if all data copied, FALSE if only some copied (user buffer too small),
		aid is area id,
		bufptr is pointer to user buffer (or ptr to ptr if mode = V4IS_PCB_RetPtr),
		buflen is buffer length (max),
		dataptr/datalen is pointer/len of data in bucket,
		cmpmode is compression mode,
		pcb is parameter control block,
		mode is (Dflt)GetMode (for optional flags)		*/

LOGICAL v4is_CopyData(aid,bufptr,buflen,dataptr,datalen,cmpmode,pcb,mode)
  int aid ;
  void *bufptr ;
  int buflen ;
  void *dataptr ; int datalen ;
  int cmpmode ;
  struct V4IS__ParControlBlk *pcb ;
  int mode ;
{ struct V4IS__AreaCB *acb ;
  char *(*charptr) ;
  int copylen=0,actlen ;

	acb = v4mm_ACBPtr(aid) ;					/* Get area control block */
	if (pcb != NULL) pcb->GetLen = 0 ;
	if (datalen < 0) v4_error(V4E_INVDATALEN,0,"V4IS","CopyData","INVDATALEN","Invalid data length (%d) to copy to user buffer",datalen) ;
#ifdef V4_BUILD_CS_FUNCTIONALITY
	if (mode & V4IS_PCB_CSDataRec)			/* Return in V4 Server format ? */
	 { dr = (struct V4CS__Msg_DataRec *)bufptr ;
	   dr->AreaId = pcb->AreaId ; dr->DataLen = datalen ; dr->CmpMode = cmpmode ;
	   if ((mode & V4IS_PCB_CSExpDataRec) == 0)
	    { memcpy(&dr->Buffer,dataptr,datalen) ; return(TRUE) ;
	    } else
	    { switch (cmpmode)				/* Don't want to return compressed record - decompress now */
	       { case V4IS_DataCmp_Mth1:
		   actlen = data_expand(&dr->Buffer,dataptr,datalen) ;
		   dr->DataLen = actlen ; dr->CmpMode = V4IS_DataCmp_None ;
		   return(TRUE) ;
		 case V4IS_DataCmp_Mth2:
		   lzrw1_decompress(dataptr,datalen,&dr->Buffer,&actlen,buflen) ;
		   dr->DataLen = actlen ; dr->CmpMode = V4IS_DataCmp_None ;
		   return(TRUE) ;
		 case V4IS_DataCmp_None:
		   memcpy(&dr->Buffer,dataptr,datalen) ; return(TRUE) ;
	       } ;
	    }
	 } ;
#endif
/*	Do we have data compression */
	switch (cmpmode)
	 { default: v4_error(V4E_INVCOMPMODE,0,"V4IS","CopyData","INVCOMPMODE","Invalid compression mode (%d)",cmpmode) ;
	   case V4IS_DataCmp_Mth1:
/*		If not allocated then allocate * 2 (same area may also have Mth2 comppression with forced overflows) */
		if (acb->AuxDataBufPtr == NULL) acb->AuxDataBufPtr = (BYTE *)v4mm_AllocChunk(acb->RootInfo->BktSize*2,FALSE) ;
		actlen = data_expand(acb->AuxDataBufPtr,dataptr,datalen) ;
		if (mode & V4IS_PCB_AllocBuf)
		 { charptr = (char *(*))bufptr ;			/* Actually giver pointer to pointer ! */
		   *charptr = (char *)v4mm_AllocChunk(actlen,FALSE) ;	/* Allocate buffer */
		   memcpy(*charptr,acb->AuxDataBufPtr,actlen) ; break ;
		 } ;
		if (!(mode & (V4IS_PCB_LengthOnly | V4IS_PCB_DataPtr | V4IS_PCB_RetPtr)))
		 { copylen = (actlen > buflen ? buflen : actlen) ;	/* Figure out bytes to copy */
		   memcpy(bufptr,acb->AuxDataBufPtr,copylen) ;
		 } ;
		if (mode & V4IS_PCB_RetPtr)
		 { charptr = (char **)bufptr ; *charptr = (char *)acb->AuxDataBufPtr ; } ;
		if (mode & V4IS_PCB_DataPtr)
		 { pcb->DataLen =actlen ;				/* Update with pointer to data in expand buffer */
		   pcb->DataPtr = (char *)acb->AuxDataBufPtr ;
		   pcb->GetLen = -1 ;					/* Flag so user know's s/he has got a pointer */
		 } ;
		break ;
	   case V4IS_DataCmp_Mth2:
		if ((mode & (V4IS_PCB_LengthOnly | V4IS_PCB_DataPtr | V4IS_PCB_RetPtr | V4IS_PCB_AllocBuf)) == 0)
		 { lzrw1_decompress(dataptr,datalen,bufptr,&actlen,buflen) ;
		   copylen = (actlen > buflen ? buflen : actlen) ; break ;
		 } ;
		if (acb->AuxDataBufPtr == NULL) acb->AuxDataBufPtr = (BYTE *)v4mm_AllocChunk(acb->RootInfo->BktSize*2,FALSE) ;
		lzrw1_decompress(dataptr,datalen,acb->AuxDataBufPtr,&actlen,acb->RootInfo->BktSize*2) ;
		copylen = (actlen > buflen ? buflen : actlen) ;
		if (mode & V4IS_PCB_AllocBuf)
		 { charptr = (char *(*))bufptr ;			/* Actually given pointer to pointer ! */
		   *charptr = (char *)v4mm_AllocChunk(copylen,FALSE) ;	/* Allocate buffer */
		   memcpy(*charptr,acb->AuxDataBufPtr,copylen) ; break ;
		 } ;
		if (mode & V4IS_PCB_RetPtr)
		 { charptr = (char **)bufptr ; *charptr = (char *)acb->AuxDataBufPtr ; } ;
		if (mode & V4IS_PCB_DataPtr)
		 { pcb->DataLen = copylen ;				/* Update with pointer to data in expand buffer */
		   pcb->DataPtr = (char *)acb->AuxDataBufPtr ;
		   pcb->GetLen = -1 ;					/* Flag so user know's s/he has got a pointer */
		 } ;
		break ;
	   case V4IS_DataCmp_None:
		if (mode & V4IS_PCB_AllocBuf)
		 { charptr = (char *(*))bufptr ;			/* Actually giver pointer to pointer ! */
		   *charptr = (char *)v4mm_AllocChunk(datalen,FALSE) ;	/* Allocate buffer */
		   memcpy(*charptr,dataptr,datalen) ; break ;
		 } ;
/*		Copy data back into user buffer */
		if (!(mode & (V4IS_PCB_LengthOnly | V4IS_PCB_DataPtr | V4IS_PCB_RetPtr)))
		 { actlen = datalen ;
		   copylen = (actlen > buflen ? buflen : actlen) ;
		   memcpy(bufptr,dataptr,copylen) ;
		 } ;
		if (mode & V4IS_PCB_RetPtr)
		 { charptr = (char **)bufptr ; *charptr = (char *)dataptr ; } ;
		if (mode & V4IS_PCB_DataPtr)
		 { pcb->DataLen = datalen ;				/* Update with pointer to data in this bucket */
		   pcb->DataPtr = dataptr ;
		   pcb->GetLen = -1 ;					/* Flag so user know's s/he has got a pointer */
		 } ;
		break ;
	 } ;
/*	Return number of bytes in record- maybe generate error if too big */
	if (pcb != NULL) { pcb->DataLen = actlen ; pcb->GetLen = copylen ; } ;
	if (!(mode & (V4IS_PCB_LengthOnly | V4IS_PCB_DataPtr | V4IS_PCB_RetPtr | V4IS_PCB_AllocBuf)))
	 { if (copylen < actlen) return(FALSE) ;			/* Record too big for buffer */
	 } ;
	return(TRUE) ;
}

/*	v4is_RemoveData - Removes a data record from a data bucket	*/
/*	Call: num = v4is_RemoveData( aid , bkt , dataid , markflag )
	  where num is number of bytes removed (TRUE) or 0 (FALSE) if no bytes removed,
		aid is area id,
		bkt is pointer to the data bucket,
		dataid is ID of record to be removed,
		markflag is TRUE to record in glb->DataIdRmv buffer	*/

int v4is_RemoveData(aid,bkt,dataid,markflag)
  int aid ;
  struct V4IS__Bkt *bkt ;
  int dataid,markflag ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__GlobalBuf *glb ;
  struct V4IS__DataBktHdr *dbh ;
  struct V4IS__DataBktEntryHdr *dbeh,*ndbeh ;
  int ax,len,index,dux,fei ;

	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return(UNUSED) ;			/* Could not find area? */
//	ax = aid ; mmm = V4_GLOBAL_MMM_PTR ;
	if (mmm->Areas[ax].SegId != 0)				/* Global area ? */
	 { GETGLB(ax) ;
	   if (glb != NULL && markflag)
	    { dux = (glb->LastDataIdUpdCnt++) % V4MM_GlobalBufUpd_Max ;
	      glb->DataUpd[dux].DataIdRmv = dataid ; glb->DataUpd[dux].DataIdNew = UNUSED ;
	    } ;
	 } else glb = NULL ;
	dbh = (DBH *)bkt ;				/* Grab bucket associated with this ID */
	for(index=dbh->FirstEntryIndex;index<dbh->FreeEntryIndex;)
	 { dbeh = (struct V4IS__DataBktEntryHdr *)&bkt->Buffer[index] ;
	   if (dbeh->Bytes == 0)
	    v4_error(V4E_INVDBEHLEN,0,"V4IS","Get","INVDBEHLEN","Area (%d %s) Bucket (%d) invalid dbeh->Bytes",
			aid,UCretASC(mmm->Areas[ax].UCFileName),dbh->sbh.BktNum) ;
	   index += ALIGN(dbeh->Bytes) ;				/* Advance index to next data entry */
	   if (dbeh->DataId != dataid) continue ;			/* Found the one we want ? */
	   ndbeh = (struct V4IS__DataBktEntryHdr *)&bkt->Buffer[index] ; /* Pointer to next entry */
	   len = ALIGN(dbeh->Bytes) ;					/* Number of bytes we are removing */
	   fei = dbh->FreeEntryIndex ;
	   if (fei == V4IS_BktSize_Max - 1 && dbh->FreeBytes == 0)
	    fei = V4IS_BktSize_Max ;					/* VEH050620 - Check for bucket size high-order bit loss */
	   memcpy(dbeh,ndbeh,(1 + fei - index)) ;
	   dbh->FreeBytes += len ; dbh->FreeEntryIndex = fei - len ;
	   return(len) ;
	 } ;
	return(UNUSED) ;
}

/*	I N D E X   B U C K E T   M O D U L E S			*/

/*	v4is_StashIndexKey - Stores key in current directory slot (in acb) 				*/
/*	Call: logical = v4is_StashIndexKey( aid , keyptr , multkeys , linkdataid )
	  where logical is TRUE if ok, FALSE if not (i.e. not enough room),
		aid is current area,
		keyptr is pointer to key to stash,
		multkeys is TRUE if this is primary key with more keys to follow,
		linkdataid is dataid this key links to							*/

LOGICAL v4is_StashIndexKey(aid,keyptr,multkeys,linkdataid)
  int aid ;
  void *keyptr ;
  LOGICAL multkeys ;
  int linkdataid ;
{ struct V4IS__AreaCB *acb ;
  struct V4IS__Bkt *bkt ;
  struct V4IS__Key *key ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  union V4MM__DataIdMask dim ;
  int i,ix ;
  char ebuf[250] ;

/*	Link up to everything */
	key = (struct V4IS__Key *)keyptr ; acb = v4mm_ACBPtr(aid) ;
	bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,aid,IDX+UPD) ;	/* Get current index bucket */
	ibh = (IBH *)bkt ;
	if (ibh->FreeBytes < (sizeof *ikde + key->Bytes + 4))
	 { IBHINUSE ; return(FALSE) ; } ;			/* No room - return FALSE */
	ix = DATATOPVAL(ibh->DataTop) ;					/* ix = index to top of data section at bottom of bucket */
	ix -= (sizeof *ikde + key->Bytes) ;				/* Decrement for bytes needed for header+key */
//	ibh->DataTop -= (sizeof *ikde + key->Bytes) ;			/* Adjust header info */
	ibh->DataTop = ix ;
	ibh->FreeBytes -= (sizeof *ikde + key->Bytes + 2) ;		/* Adjust free bytes remaining */
	ikde = (IKDE *)&bkt->Buffer[ix] ;				/* Set pointer to header */
	ikde->EntryType = V4IS_EntryType_DataLink ;			/* Making a data link */
	ikde->HasMultKeys = multkeys ;					/* Is this primary with more to come ? */
	dim.dataid = linkdataid ;
	ikde->AuxVal = dim.fld.BktNum ;					/* Save bucket number */
	ikde->DataId = linkdataid ;					/* Save Data ID */
	memcpy(((char *)ikde + sizeof *ikde),key,key->Bytes) ;		/* Copy primary key */
	ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;			/* Figure out where key list is */
/*	Figure out how to insert into DataIndex (maybe have to make a hole) */
	   IFTRACEAREA
	    { sprintf(ebuf,"StashIndexKey(bkt=%d pos=%d/%d key=%s) dataid=%x",
			acb->Lvl[acb->CurLvl].BktNum,acb->CurPosKeyIndex,ibh->KeyEntryNum,v4is_FormatKey(key),linkdataid) ;
	    } ;
	if (acb->CurPosKeyIndex >= ibh->KeyEntryNum)
	 { ibkl->DataIndex[ibh->KeyEntryNum++] = ix ;			/* Add index to this entry at bottom of list (next key) */
	 } else
	 { for(i=ibh->KeyEntryNum;i>acb->CurPosKeyIndex;i--)
	    { ibkl->DataIndex[i] = ibkl->DataIndex[i-1] ; } ;		/* Make hole for key */
	   ibkl->DataIndex[acb->CurPosKeyIndex] = ix ;			/* Insert index into hole */
	   ibh->KeyEntryNum++ ;						/* Increment number of keys */
	 } ;
	IBHINUSE ;
	return(TRUE) ;
}

/*	v4is_IndexSplit - Splits part of index bucket into new bucket	*/
/*	Call: bktnum = v4is_IndexSplit( aid , bktptr , startkey , keyposptr , badkx )
	  where bktnum is bucket number of new bucket,
		aid is area id,
		bktptr is current bucket (bucket is "consolidated" upon return),
		startkey is starting key index to split out,
		keyposptr if not NULL is updated with actual key position used for split,
		  (i.e. updated with index in original bucket used to start new bucket),
		badkx if not -1 is forbidden key index to split on			*/

int v4is_IndexSplit(aid,bktptr,startkey,keyposptr,badkx)
  int aid ;
  void *bktptr ;
  int startkey ;
  int *keyposptr ;
  int badkx ;
{ struct V4IS__Bkt *cbkt,*nbkt ;
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__IndexBktHdr *cibh,*nibh ;
  struct V4IS__IndexBktKeyList *cibkl,*nibkl ;
  struct V4IS__IndexKeyDataEntry *cikde ;
  struct V4IS__Key *key,*skey ;
  int ax,nx,cx,nbytes ;
  char ebuf[250] ;

	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return(UNUSED) ;			/* Could not find area? */
//	mmm = V4_GLOBAL_MMM_PTR ; ax = aid ;
	cbkt = (struct V4IS__Bkt *)bktptr ; cibh = (IBH *)cbkt ; cibkl = (IBKL *)&cbkt->Buffer[cibh->KeyEntryTop] ;
	v4mm_TempLock(aid,cibh->sbh.BktNum,TRUE) ;			/* Lock bucket into memory */
	mmm->Areas[ax].LockExtra = mmm->Areas[ax].LockIDX ;		/* Save LockID for current index */
	mmm->Areas[ax].LockIDX = UNUSED ;				/* Make believe we don't have so we can grab another */
	nbkt = v4is_MakeBkt(aid,V4_BktHdrType_Index) ; nibh = (IBH *)nbkt ; nibkl = (IBKL *)&nbkt->Buffer[nibh->KeyEntryTop] ;
	nibh->ParentBktNum = cibh->ParentBktNum ;
	acb = v4mm_ACBPtr(aid) ;	/* This (plus ebuf) is only needed for IFTRACEAREA */
	IFTRACEAREA
	 { sprintf(ebuf,"IndexSplit(skey=%d badkx=%d) Cur(bkt=%d Num=%d) New(bkt=%d)",
			startkey,badkx,cibh->sbh.BktNum,cibh->KeyEntryNum,nibh->sbh.BktNum) ;
	 } ;
	if (cibh->KeyEntryNum <= 1)					/* If only 1 key then not much to split ! */
	 { v4mm_TempLock(aid,cibh->sbh.BktNum,FALSE) ; NIBHINUSE ;
	   if (mmm->Areas[ax].LockExtra >= 0)
	    { v4mm_LockRelease(aid,mmm->Areas[ax].LockExtra,mmm->MyPid,aid,"SplitExtra") ; mmm->Areas[ax].LockExtra=UNUSED ; } ;
	   if (keyposptr != NULL) *keyposptr = startkey ;
	   return(nibh->sbh.BktNum) ;
	 } ;
	for(;;)
	 { cikde = (IKDE *)&cbkt->Buffer[cibkl->DataIndex[startkey]] ;
	   skey = (KEY *)((char *)cikde + sizeof *cikde) ;		/* skey points to requested start key */
	   for(cx=startkey-1;cx>=0;cx--)				/* Scan back to make sure we don't split chain of dups */
	    { cikde = (IKDE *)&cbkt->Buffer[cibkl->DataIndex[cx]] ;
	      key = (KEY *)((char *)cikde + sizeof *cikde) ; if (memcmp(key,skey,skey->Bytes) != 0) break ;
	    } ;
	   if (cx < 0) { cx = startkey ; } else { cx++ ; } ;		/* If hit begin of bkt, then allow split where requested */
	   startkey = cx ;						/* Maybe new startkey */
	   if (startkey != badkx) break ;				/* If not the forbidden index then quit */
	   startkey-- ;							/* Keep plugging with new start key */
	 } ;
	if (keyposptr != NULL) *keyposptr = startkey ;			/* May return actual starting key */
	for(;cx<cibh->KeyEntryNum;cx++)
	 { cikde = (IKDE *)&cbkt->Buffer[cibkl->DataIndex[cx]] ;	/* Link to entry in current bucket */
	   switch (cikde->EntryType)
	    { default: v4_error(V4E_INVIKDE,0,"V4IS","GetRec","INVIKDE","Invalid IKDE Entry Type (%d) in Bucket %d, AreaID %d",
					cikde->EntryType,nibh->sbh.BktNum,aid) ;
	      case V4IS_EntryType_DataLen:
			nbytes = ALIGN(cikde->AuxVal) ; break ;
	      case V4IS_EntryType_DataLink:
	      case V4IS_EntryType_IndexLink:
			key = (KEY *)((char *)cikde + sizeof *cikde) ; nbytes = sizeof *cikde + key->Bytes ;
			break ;
	    } ;
//	   nx = (nibh->DataTop -= nbytes) ;
	   nx = DATATOPVAL(nibh->DataTop) - nbytes ;
	   nibh->DataTop = nx ;
	   nibh->FreeBytes -= (nbytes + 2) ;				/* Adjust new bucket for number of bytes */
	   nibkl->DataIndex[nibh->KeyEntryNum++] = nx ;			/* Add index to list at top of bucket */
	   memcpy(&nbkt->Buffer[nx],cikde,nbytes) ;			/* Copy from current to new */
	   if (cikde->EntryType == V4IS_EntryType_DataLen)		/* If we moved local data then ... */
	    { cikde = (IKDE *)&nbkt->Buffer[nx] ;			/*  have to create new DataId */
	      cikde->DataId = v4mm_MakeDataId(nbkt) ;
	    } ;
	 } ;
	cibh->KeyEntryNum = startkey ;					/* Adjust number of keys in current */
	v4is_IndexConsolidate(cbkt,aid) ;				/* Consolidate all info in current bucket */
/* VEH960328 - Reversed order of next two lines */
	v4mm_BktUpdated(aid,cibh->sbh.BktNum) ;				/* Make sure old bucket marked as updated */
	v4mm_TempLock(aid,cibh->sbh.BktNum,FALSE) ;			/* Turn off temporary lock */
	IFTRACEAREA
	 { sprintf(ebuf,"IndexSplit after consolidate(startkey=%d)",startkey) ;
	 } ;
	if (mmm->Areas[ax].LockExtra >= 0)
	 { v4mm_LockRelease(aid,mmm->Areas[ax].LockExtra,mmm->MyPid,aid,"SplitExtra") ; mmm->Areas[ax].LockExtra=UNUSED ; } ;
	NIBHINUSE ;
	v4is_IndexNewParent(nbkt,aid) ;					/* Inform children of new parent */
	return(nibh->sbh.BktNum) ;					/* Return bucket number of new bucket */
}

/*	v4is_IndexNewParent - Flags all children index buckets that they have new parent	*/
/*	Call: v4is_IndexNewParent( bktptr , aid )
	  where bktptr is pointer to new parent bucket,
		aid is area id									*/

void v4is_IndexNewParent(bktptr,aid)
  struct V4IS__Bkt *bktptr ;
  int aid ;
{ struct V4IS__Bkt *pbkt,*nbkt ;
  struct V4IS__IndexBktHdr *pibh,*nibh ;
  struct V4IS__IndexBktKeyList *pibkl ;
  struct V4IS__IndexKeyDataEntry *pikde ;
  int px ;

	pbkt = bktptr ; pibh = (IBH *)pbkt ; pibkl = (IBKL *)&pbkt->Buffer[pibh->KeyEntryTop] ;
	v4mm_TempLock(aid,pibh->sbh.BktNum,TRUE) ;			/* Lock parent into memory */
	for(px=0;px<pibh->KeyEntryNum;px++)
	 { pikde = (IKDE *)&pbkt->Buffer[pibkl->DataIndex[px]] ;	/* Link to entry in parent bucket */
	   switch (pikde->EntryType)
	    { default: v4_error(V4E_INVIKDE,0,"V4IS","GetRec","INVIKDE","Invalid IKDE Entry Type (%d) in Bucket %d, AreaID %d",
					pikde->EntryType,pibh->sbh.BktNum,aid) ;
	      case V4IS_EntryType_DataLen:		break ;
	      case V4IS_EntryType_DataLink:		break ;
	      case V4IS_EntryType_IndexLink:
			nbkt = v4mm_BktPtr(pikde->AuxVal,aid,IDX+UPD) ; nibh = (IBH *)nbkt ;
			nibh->ParentBktNum = pibh->sbh.BktNum ;		/* Just link to parent */
			break ;
	    } ;
	 } ;
	v4mm_TempLock(aid,pibh->sbh.BktNum,FALSE) ;			/* Turn off temporary lock */
}


/*	v4is_IndexConsolidate - Consolidates data at bottom of bucket (hopefully freeing up space) */
/*	Call: v4is_IndexConsolidate( bktptr , aid )
	  where bktptr is point to bucket,
		aid is area id for bucket							*/

void v4is_IndexConsolidate(bktptr,aid)
  struct V4IS__Bkt *bktptr ;
{ struct V4IS__Bkt *bkt,nbkt ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4IS__Key *key ;
  int nx,num,kx ;

	acb = v4mm_ACBPtr(aid) ;
	bkt = bktptr ; ibh = (IBH *)bkt ; ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
	nx = acb->RootInfo->BktSize ;					/* nx = Bottom of bucket */
	for(kx=0;kx<ibh->KeyEntryNum;kx++)
	 { if (ibkl->DataIndex[kx] == V4IS_DataIndex_Null) continue ;	/* Skip over empty place-holder */
	   ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[kx]] ;		/* Point to key entry in old bucket */
	   switch (ikde->EntryType)
	    { default: v4_error(V4E_INVIKDE,0,"V4IS","GetRec","INVIKDE","Invalid IKDE Entry Type (%d) in Bucket %d, AreaID %d",
					ikde->EntryType,ibh->sbh.BktNum,aid) ;
	      case V4IS_EntryType_IndexLink:
	      case V4IS_EntryType_DataLink:
		   key = (KEY *)((char *)ikde + sizeof *ikde) ; num = sizeof *ikde + key->Bytes ; break ;
	      case V4IS_EntryType_DataLen:
		   num = ALIGN(ikde->AuxVal) ; break ;
	    } ;
	    nx -= num ; memcpy(&nbkt.Buffer[nx],ikde,num) ;		/* Copy bytes to new bucket */
	    ibkl->DataIndex[kx] = nx ;					/* Update old bucket with new info */
	 } ;
/*	Now copy recompacted data/index junk back into old bucket */
	num = acb->RootInfo->BktSize - nx ;				/* num = Total bytes recompacted */
	memcpy(&bkt->Buffer[nx],&nbkt.Buffer[nx],num) ;			/* Copy data */
	ibh->DataTop = nx ;						/* Set new top of data in bucket */
	ibh->FreeBytes = (char *)&bkt->Buffer[ibh->DataTop]
		- (char *)&ibkl->DataIndex[ibh->KeyEntryNum] ;		/* Determine new free bytes */
}

/*	v4is_IndexStashLink - Adds key & link to child into parent bucket 	*/
/*	NOTE: This may be a recursive call if parent is full!			*/
/*	Call: logical = v4is_IndexStashLink( aid , parentlvl , key , oldbktnum , fromptr , bktnumptr )
	  where logical is TRUE if root node was split (used only for recursive calls),
		aid is area id,
		parentlvl is acb->Lvl of parent,
		key is point to key to be inserted into parent,
		oldbktnum is bucket number that was split (used to correctly position in parent node),
		fromptr is text string where this is being called from,
		bktnumptr is not NULL is updated with bucket used to stash link	*/

LOGICAL v4is_IndexStashLink(aid,parentlvl,key,oldbktnum,fromptr,bktnumptr)
  int aid ;
  int parentlvl ;
  struct V4IS__Key *key ;
  int oldbktnum ;
  char *fromptr ;			/* Source module */
  int *bktnumptr ;
{ struct V4IS__Bkt *bkt ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4IS__Key *key2,*dkey ;
  struct V4IS__Key skey ;
  int ix,lx,kx,skx,i,sx,bktnum,oldnum,kxinc,splitroot,actparentbkt,parentbkt ;
  char ebuf[250] ;

/*	Link up to everything */
	acb = v4mm_ACBPtr(aid) ; splitroot = FALSE ;
	IFTRACEAREA
	 { sprintf(ebuf,"IndexStashLink(plvl=%d oldbkt=%d from=%s)",parentlvl,oldbktnum,fromptr) ;
	 } ;
try_again:
	bkt = v4mm_BktPtr(acb->Lvl[parentlvl].BktNum,aid,IDX+UPD) ;	/* Get parent node */
	ibh = (IBH *)bkt ;
	kx = v4is_SearchBucket(bkt,key) ; kxinc = FALSE ;
	if (kx >= 0 && oldbktnum == V4IS_StashAt_End)
	 { ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
	   for(i=kx+1;i<ibh->KeyEntryNum;i++)	/* Look forwards to end of chain */
	    { ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[i]] ; dkey = (KEY *)((char *)ikde + sizeof *ikde) ;
	      if (memcmp(key,dkey,key->Bytes) != 0) break ; /* Want to position to last in chain of duplicates! */
	    } ; kx = i ;					/* Position to next so dups always go at end */
	 } else if (kx >= 0)					/* Got exact match- check for dups */
	 { ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
	   ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[kx]] ;		/* Get ikde to this entry */
	   if (ikde->EntryType != V4IS_EntryType_IndexLink || ikde->AuxVal != oldbktnum)
	    { for(skx=kx,kx--;kx>=0;kx--)			/* Not correct link - search backwards */
	       { ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[kx]] ;
	         if (ikde->EntryType != V4IS_EntryType_IndexLink || ikde->AuxVal != oldbktnum) continue ;
	         dkey = (KEY *)((char *)ikde + sizeof *ikde) ;
	         if (memcmp(key,dkey,dkey->Bytes) == 0) { goto got_link ; } else { break ; } ;
	       } ;
	      for(kx=skx+1;kx<ibh->KeyEntryNum;kx++)		/* Still can't find - search forwards */
	       { ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[kx]] ;
	         if (ikde->EntryType != V4IS_EntryType_IndexLink || ikde->AuxVal != oldbktnum) continue ;
	         dkey = (KEY *)((char *)ikde + sizeof *ikde) ;
	         if (memcmp(key,dkey,dkey->Bytes) == 0) { goto got_link ; } else { break ; } ;
	       } ;
	      v4_error(V4E_NOPOSPAR,0,"V4IS","IndexStashLink","NOPOSPAR",
		"Area (%d) Could not find position in parent index bucket (%d) of child bucket (%d)",
		aid,acb->Lvl[parentlvl].BktNum,acb->Lvl[parentlvl+1].BktNum) ;
	    } ;
got_link:  kx++ ; kxinc = TRUE ;				/* Increment so duplicate will appear after current */
	 } ;
	if (kx < 0) { kx = (-kx) - 1 ; } ;
	if (ibh->FreeBytes < (sizeof *ikde + 4 + key->Bytes))
	 { if (ibh->sbh.BktType != V4_BktHdrType_Root)			/* No room in parent - this may get messy */
	    { oldnum = ibh->sbh.BktNum ;				/* Save old bucket number */
	      sx = ibh->KeyEntryNum / 2 ;
	      if (kx == sx) sx-- ;					/* Don't want to split right where key should go! */
	      bktnum = v4is_IndexSplit(aid,bkt,sx,&sx,kx) ;		/* Copy 1/2 of records to new bucket */
	      IBHINUSE ;
	      bkt = v4mm_BktPtr(bktnum,aid,IDX+UPD) ; ibh = (IBH *)bkt ;
	      bktnum = ibh->sbh.BktNum ; parentbkt = ibh->ParentBktNum ;
	      acb->Lvl[parentlvl].BktNum = bktnum ;			/* Define this as the index for this level */
	      ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
	      key2 = (KEY *)((char *)&bkt->Buffer[ibkl->DataIndex[0]] + sizeof *ikde) ;	/* Get pointer to first key */
	      IBHINUSE ;
	      memcpy(&skey,key2,key2->Bytes) ;
	      splitroot = v4is_IndexStashLink(aid,parentlvl-1,&skey,oldnum,"Stash1",&actparentbkt) ; /* Link new to its parent */
	      if (parentbkt != actparentbkt)
	       { bkt = v4mm_BktPtr(bktnum,aid,IDX+UPD) ; ibh = (IBH *)bkt ; ibh->ParentBktNum = actparentbkt ; } ;
	      if (splitroot) parentlvl++ ;				/* If split root then everything bumped down 1 */
/*	      Now decide where key is to go- old bucket or new? */
	      if (kxinc) kx-- ;						/* Adjust kx to get proper bucket */
	      acb->Lvl[parentlvl].BktNum = (kx >= sx ? bktnum : oldnum) ;
	      goto try_again ;						/* Keep plugging */
	    } else
	    { sx = ibh->KeyEntryNum / 2 ;
	      if (kx == sx) sx-- ;					/* Don't want to split right where key should go! */
	      bktnum = v4is_IndexSplit(aid,bkt,sx,&sx,kx) ;		/* This is root bucket - copy 1/2 keys to new bucket */
	      IBHINUSE ;
	      for(lx=acb->CurLvl;lx>0;lx--)
	       { acb->Lvl[lx+1].BktNum = acb->Lvl[lx].BktNum ; } ;	/* Drop everything down a level */
	      acb->CurLvl++ ;
	      if (acb->RootInfo->NumLevels < acb->CurLvl+1)
	       { acb->RootInfo->NumLevels = (acb->CurLvl+1) ;			/* Save number of levels in index */
	       } ;
	      acb->Lvl[1].BktNum = bktnum ;				/* Make new index directly below root */
/*	      Now link key to either root or new bucket below based on where key belongs */
	      if (kxinc) kx-- ;						/* Maybe decrement kx so we don't get wrong bucket */
	      memcpy(&skey,key,key->Bytes) ;
	      if (kx >= sx)						/* Have to play bucket games if key goes in root */
	       { v4is_IndexStashLink(aid,parentlvl+1,&skey,oldbktnum,"Stash2",bktnumptr) ;
	       } else
	       { acb->Lvl[1].BktNum = acb->Lvl[2].BktNum ;		/* "Roll back" temporarily */
	         v4is_IndexStashLink(aid,(kx >= sx ? parentlvl+1 : parentlvl),&skey,oldbktnum,"Stash2",bktnumptr) ;
		 acb->Lvl[1].BktNum = bktnum ;
	       } ;
	      bkt = v4mm_BktPtr(bktnum,aid,IDX+UPD) ; ibh = (IBH *)bkt ;
	      ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
	      key2 = (KEY *)((char *)&bkt->Buffer[ibkl->DataIndex[0]] + sizeof *ikde) ;	/* Get pointer to first key */
	      memcpy(&skey,key2,key2->Bytes) ;
	      v4is_IndexStashLink(aid,0,&skey,V4IS_StashAt_End,"Stash3",NULL) ;/* Now link child to the empty (new) root */
	      IBHINUSE ;
	      return(TRUE) ;						/* Return TRUE because root split! */
	    } ;
	 } ;
/*	At this point should be enough room in bkt for key link */
	ix = DATATOPVAL(ibh->DataTop) ;					/* ix = index to top of data section at bottom of bucket */
	ix -= (sizeof *ikde + key->Bytes) ;				/* Decrement for bytes needed for header+data */
//	ibh->DataTop -= (sizeof *ikde + key->Bytes) ;			/* Adjust header info */
	ibh->DataTop = ix ;
	ibh->FreeBytes -= (sizeof *ikde + key->Bytes + 2) ;
	ikde = (IKDE *)&bkt->Buffer[ix] ;				/* Set pointer to header */
	ikde->EntryType = V4IS_EntryType_IndexLink ;			/* Making a immediate data entry */
	ikde->AuxVal = acb->Lvl[parentlvl+1].BktNum ;			/* Save bucket number of child bucket */
	memcpy(&bkt->Buffer[ix+sizeof *ikde],key,key->Bytes) ;		/* Copy actual key */
	ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;			/* Figure out where key list is */
/*	Figure out where in index this is to go */
	if (kx >= ibh->KeyEntryNum)
	 { ibkl->DataIndex[ibh->KeyEntryNum++] = ix ;			/* Insert at "end" of bucket */
	 } else
	 { for(i=ibh->KeyEntryNum;i>kx;i--)
	    { ibkl->DataIndex[i] = ibkl->DataIndex[i-1] ; } ;
	   ibkl->DataIndex[kx] = ix ; ibh->KeyEntryNum ++ ;
	 } ;
	if (bktnumptr != NULL) *bktnumptr = ibh->sbh.BktNum ;
	IBHINUSE ;
/* Temp check to verify we did it right */
memcpy(&skey,key,key->Bytes) ;
bkt = v4mm_BktPtr(ikde->AuxVal,aid,IDX) ; ibh = (IBH *)bkt ;
ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;			/* Figure out where key list is */
key = (KEY *)((char *)&bkt->Buffer[ibkl->DataIndex[0]] + sizeof *ikde) ;	/* Get pointer to first key */
if (memcmp(&skey,key,key->Bytes) != 0)
 { printf("?? StashLink (from %s) from index (%d) to child (%d) bad - first key in child does not match parent\n",
	fromptr,acb->Lvl[parentlvl].BktNum,acb->Lvl[parentlvl+1].BktNum) ;
   printf("   kx = %d, kxinc = %d, sx = %d\n",kx,kxinc,sx) ;
   v4is_ExamineBkt(acb->Lvl[parentlvl].BktNum,aid) ; v4is_ExamineBkt(acb->Lvl[parentlvl+1].BktNum,aid) ;
   v4_error(V4E_INVLINK,0,"V4IS","StashLink","INVLINK","Invalid link from parent to child") ;
 } ;
	return(splitroot) ;
}

/*	v4is_ReplaceDataId - Replaces data id for an index entry with new one */
/*	Call: v4is_ReplaceDataId( aid , keyptr , new , old )
	  where aid is area id,
		keyptr is point to the wanted key,
		new is the new dataid,
		old is the old one						*/

LOGICAL v4is_ReplaceDataId(aid,keyptr,new,old)
  int aid ;
  struct V4IS__Key *keyptr ;
  int new,old ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__Bkt *bkt ;
  struct V4IS__Key *key ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  union V4MM__DataIdMask dim ;
  HANGLOOSEDCL
  int ax,tries ; char ebuf[250] ;

	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return(FALSE) ;			/* Could not find area? */
//	mmm = V4_GLOBAL_MMM_PTR ; ax = aid ;
	acb = v4mm_ACBPtr(aid) ;
	RELDAT RELIDX
	v4mm_LockSomething(aid,V4MM_LockId_Tree,V4MM_LockMode_Read,V4MM_LockIdType_Tree,37,&mmm->Areas[ax].LockTree,15) ;
	for(tries=0;tries<9;tries++)
	 { if (v4is_PositionKey(aid,keyptr,&key,old,V4IS_PosBoKL) != V4RES_PosKey)
	    { sprintf(ebuf,"Could not locate key-dataid (%x) to update to %x (PositionKey), try (%d)",old,new,tries) ;
	      v4is_BugChk(aid,ebuf) ;
	      if (tries < 5) { HANGLOOSE(250) ; continue ; } ;
	      v4_error(V4E_NOFINDKEY,0,"V4IS","ReplaceDataId","NOFINDKEY","Area (%d) Could not locate specified key to replace DataId",aid) ;
	    } ;
	   bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,aid,IDX+UPD) ;/* Get bucket pointer */
	   ibh = (IBH *)bkt ;		/* Bucket header @ start of bucket */
	   ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;	/* Get pointer to key index list below header */
	   ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[acb->CurPosKeyIndex]] ;/* then use to get begin of key entry @ bottom */
	   key = (KEY *)((char *)ikde + sizeof *ikde) ;		/* Pointer to actual key */
	   if (memcmp(key,keyptr,key->Bytes) != 0)		/* Keys don't match ? */
	    { HANGLOOSE(250) ; continue ; } ;			/*  wait for possible update by other process and try again */
	   if (ikde->DataId != old)				/* Not the one we want */
	    { HANGLOOSE(250) ; continue ; } ;			/*  wait for possible update by other process and try again */
	   ikde->DataId = new ;					/* Replace dataid */
	   dim.dataid = new ; ikde->AuxVal = dim.fld.BktNum ;	/*  and link to (probably new) bucket */
	   IBHINUSE ; RELTREE
	   return(TRUE) ;
	 } ;
	sprintf(ebuf,"Could not locate key-dataid (%x) to update to %x",old,new) ;
	v4is_BugChk(aid,ebuf) ;
	v4_error(V4E_NOKEYWITHDATAID,0,"V4IS","ReplaceDataId","NOKEYWITHDATAID","Could not locate key with specified DataId") ;
	return(FALSE) ;
}

/*	v4is_RemoveIndexEntry - Removes a key (and maybe data) from an index bucket */
/*	Call: v4is_RemoveIndexEntry( pcb , aid , level, keyindex , replacekey )
	  where pcb is pointer to pcb or NULL,
		aid is area id,
		level is index level of index(bucket) to be updated,
		keyindex is key index to be removed (replaced),
		replacekey if not null is key to replace key to be removed (bucket @ level+1 MUST BE SET!!)	*/

void v4is_RemoveIndexEntry(pcb,aid,level,keyindex,replacekey)
  struct V4IS__ParControlBlk *pcb ;
  int aid ;
  int level ;
  int keyindex ;
  struct V4IS__Key *replacekey ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__Bkt *bkt ;
  struct V4IS__Key *key,okey,skey ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__UnusedBktHdr *ubh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  int i,ix,kcnt,lrx,ax,bktnum,dataid,savelvl ;
  char ebuf[250] ;

	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return ;			/* Could not find area? */
//	mmm = V4_GLOBAL_MMM_PTR ; ax = aid ;
	acb = v4mm_ACBPtr(aid) ;
	savelvl = acb->CurLvl ; acb->CurLvl = level ;
	bkt = v4mm_BktPtr(acb->Lvl[level].BktNum,aid,IDX+UPD) ;	/* Get bucket pointer */
	ibh = (IBH *)bkt ; bktnum = ibh->sbh.BktNum ;		/* Bucket header @ start of bucket */
	ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;		/* Get pointer to key index list below header */
	ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[keyindex]] ; /* then use to get begin of key entry @ bottom */
	key = (KEY *)((char *)ikde + sizeof *ikde) ;		/* Pointer to actual key */
	memcpy(&okey,key,key->Bytes) ;				/* Copy old key into save buffer */
	   IFTRACEAREA
	    { sprintf(ebuf,"RemoveIndexEntry(bkt=%d pos=%d key=%s cnt=%d) dataid=%x",
			bktnum,keyindex,v4is_FormatKey(&okey),ibh->KeyEntryNum,ikde->DataId) ;
	    } ;
	if (replacekey != NULL)
	 { ibkl->DataIndex[keyindex] = V4IS_DataIndex_Null ;	/* Mark index position as empty */
	   v4is_IndexConsolidate(bkt,aid) ;			/* Consolidate space */
	   ix = DATATOPVAL(ibh->DataTop) ;			/* ix = index to top of data section at bottom of bucket */
	   ix -= (sizeof *ikde + replacekey->Bytes) ;		/* Decrement for bytes needed for header+data */
//	   ibh->DataTop -= (sizeof *ikde + replacekey->Bytes) ;	/* Adjust header info */
	   ibh->DataTop = ix ;
	   ibh->FreeBytes -= (sizeof *ikde + replacekey->Bytes + 2) ;
	   ikde = (IKDE *)&bkt->Buffer[ix] ;			/* Set pointer to header */
	   ikde->EntryType = V4IS_EntryType_IndexLink ;		/* Making a immediate data entry */
	   ikde->AuxVal = acb->Lvl[level+1].BktNum ;		/* Save bucket number of child bucket */
	   memcpy(&bkt->Buffer[ix+sizeof *ikde],replacekey,replacekey->Bytes) ;	/* Copy actual key */
	   ibkl->DataIndex[keyindex] = ix ;			/* Add index to this entry at bottom of list (next key) */
/*	   If this is the first key of the bucket then have to delete from parent & link parent to new first key */
	   if (keyindex > 0 || level == 0) { IBHINUSE ; return ; } ;
	   ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[keyindex]] ;/* Link up to first key in bucket */
	   key = (KEY *)((char *)ikde + sizeof *ikde) ; memcpy(&skey,key,key->Bytes) ;	/* Copy new start key into save buffer */
	   dataid = ikde->DataId ;
	   IBHINUSE ;
	   i = v4is_PositionToParentNode(aid,&okey,bktnum,dataid,ibh->ParentBktNum) ;
/*
if (i <= 0) printf("? RmvIdxEntry: i=%d, level=%d, skey=%d %d %d okey=%d\n",
  i,level,skey.AuxVal,skey.KeyType,skey.KeyVal.IntVal,okey.KeyVal.IntVal) ;
*/
	   v4is_RemoveIndexEntry(pcb,aid,level-1,i,&skey) ;	/* Replace old link in parent with new link */
	   acb->CurLvl = savelvl ;
	   return ;
	 } ;
/*	First bump up the list of indexes */
	for(i=keyindex;i<ibh->KeyEntryNum;i++) { ibkl->DataIndex[i] = ibkl->DataIndex[i+1] ; } ;
	kcnt = --ibh->KeyEntryNum ;				/* Decrement number of keys */
	v4is_IndexConsolidate(bkt,aid) ;			/* Now consolidate info in bucket */
/*	If this is the first key of the bucket then have to delete from parent & link parent to new first key */
	if (keyindex > 0 || level == 0) { IBHINUSE ; return ; } ;
	if (kcnt > 0)
	 { ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[keyindex]] ;/* Link up to first key in bucket */
	   key = (KEY *)((char *)ikde + sizeof *ikde) ; memcpy(&skey,key,key->Bytes) ;	/* Copy new start key into save buffer */
	   dataid = ikde->DataId ;
	 } else dataid = 0 ;
	IBHINUSE ;
	i = v4is_PositionToParentNode(aid,&okey,bktnum,dataid,ibh->ParentBktNum) ;
	if (kcnt <= 0)						/* Bucket empty ? */
	 { bkt = v4mm_BktPtr(bktnum,aid,IDX+UPD) ; ibh = (IBH *)bkt ;
	   if (pcb == NULL ? FALSE : pcb->BktNums[pcb->CurLvl] == bktnum) /* Are we trashing our "current position bucket" ? */
	    { pcb->CurPosKeyIndex = i ;					   /*  Yes - then save new current position in parent */
	      pcb->BktNums[ pcb->CurLvl = level-1 ] = ibh->ParentBktNum ;
	    } ;
	   ubh = (struct V4IS__UnusedBktHdr *)ibh ;	/* Bucket is empty - put on list */
	   LOCKROOT ; ubh->NextUnusedBkt = acb->RootInfo->NextUnusedBkt ;
	   ubh->UBHFlag = UBHFlagVal ; ubh->sbh.BktType = V4_BktHdrType_Unused ;
	   acb->RootInfo->NextUnusedBkt = ubh->sbh.BktNum ;
	   RELROOT("RemoveIndexEntry") ; v4mm_BktUpdated(aid,0) ;
	   v4is_RemoveIndexEntry(pcb,aid,level-1,i,NULL) ;		/* Remove key from parent bucket */
	   acb->CurLvl = savelvl ;
	   return ;
	 } ;
/*
if (i <= 0) printf("? RmvIdxEntry2: i=%d, level=%d, skey=%d %d %d okey=%d\n",
  i,level,skey.AuxVal,skey.KeyType,skey.KeyVal.IntVal,okey.KeyVal.IntVal) ;
*/
	v4is_RemoveIndexEntry(pcb,aid,level-1,i,&skey) ;	/* Replace old link in parent with new link */
	acb->CurLvl = savelvl ;
}

/*	F O R E I G N   F I L E   M O D U L E S			*/

/*	v4is_FFKeyCount - Returns number of keys for a fileref			*/
/*	Call: number = v4is_FFKeyCount( acb , fileref )
	  where number is number of keys in fileref,
		acb is current acb,
		fileref is file ref to use					*/

int v4is_FFKeyCount(acb,fileref)
  struct V4IS__AreaCB *acb ;
  int fileref ;
{ struct V4FFI__KeyInfo *ki ;
  struct V4FFI__KeyInfoDtl *kid ;
  int kx ;

	if (fileref <= 0) return(0) ;
	if ((ki = acb->KeyInfo) == NULL)
	 v4_error(V4E_AREANOFFI,0,"V4IS","Get","AREANOFFI","No foreign file key information in area") ;
	for((kid=(KID *)&ki->Buffer,kx=0);kx<ki->Count;(kx++,kid=(KID *)(kid+kid->Bytes)))
	 { if (kid->FileRef == fileref) return(kid->KeyCount) ; } ;
	return(0) ;						/* Could not find ? */
}
/*	v4is_FFKeyDups - Returns TRUE if duplicates allowed on key */
/*	Call: logical = v4is_FFKeyDups( acb , fileref, keynum )
	  where logical is TRUE if dups allowed,
		acb is current acb,
		fileref is file ref to use,
		keynum is key number (0 is first)			*/

LOGICAL v4is_FFKeyDups(acb,fileref,keynum)
  struct V4IS__AreaCB *acb ;
  int fileref,keynum ;
{ struct V4FFI__KeyInfo *ki ;
  struct V4FFI__KeyInfoDtl *kid ;
  int kx ;

	if (fileref <= 0) return(0) ;
	if ((ki = acb->KeyInfo) == NULL)
	 v4_error(V4E_AREANOFFI,0,"V4IS","Get","AREANOFFI","No foreign file key information in area") ;
	for((kid=(KID *)&ki->Buffer,kx=0);kx<ki->Count;(kx++,kid=(KID *)(kid+kid->Bytes)))
	 { if (kid->FileRef != fileref) continue ;
	   return(kid->Key[keynum].DupsOK) ;
	 } ;
	return(FALSE) ;						/* Could not find ? */
}

/*	v4is_MakeFFKEY - Updates *keyptr to proper key based on arguments	*/
/*	Call: keyptr = v4is_MakeFFKey( acb , fileref , keyptr , keynum , keybufptr , databufptr , inrecflag )
	  where acb is file's acb,
		fileref is file ref to used,
		keyptr is pointer to local key buffer to be updated,
		keynum is key number for fileref,
		keybufptr is pointer to key buffer (usually NULL),
		databufptr is pointer to record buffer (key data pulled from here),
		inrecflag is TRUE to enforce keybuf is within get-buffer, FALSE if don't care	*/

KEY *v4is_MakeFFKey(acb,fileref,keyptr,keynum,keybufptr,databufptr,inrecflag)
  struct V4IS__AreaCB *acb ;
  int fileref ;
  void *keyptr ;
  int keynum ; LOGICAL inrecflag ;
  void *keybufptr,*databufptr ;
{ struct V4FFI__KeyInfo *ki ;
  struct V4FFI__KeyInfoDtl *kid ;
  struct lcl__FFKey {
    union V4IS__KeyPrefix kp ;
    char KeyVal[V4IS_KeyBytes_Max] ;
   } *ffkey ;
  int kx ;

	ffkey = (struct lcl__FFKey *)keyptr ;
	if ((ki = acb->KeyInfo) == NULL)
	 v4_error(V4E_AREANOFFI,0,"V4IS","MakeFFKey","AREANOFFI","No foreign file key information in area") ;
	for((kid=(KID *)&ki->Buffer,kx=0);kx<ki->Count;(kx++,kid=(KID *)(kid+kid->Bytes)))	/* Look for fileref */
	 { if (kid->FileRef != fileref) continue ;
	   if (keynum >= kid->KeyCount)
	    v4_error(V4E_KEYNUMTOOBIG,0,"V4IS","MakeFFKey","KEYNUMTOOBIG","KeyNum too big for specified foreign file FileRef") ;
	   if (kid->Key[keynum].KeyPrefix.all == 0)			/* Have to construct V4 key from key in buffer ? */
	    { return((KEY *)((char *)databufptr + kid->Key[keynum].Offset)) ;
	    } else
	    { ffkey->kp.all = kid->Key[keynum].KeyPrefix.all ;
	      if (inrecflag && (keybufptr == NULL ? FALSE : keybufptr != (char *)databufptr+kid->Key[keynum].Offset))
	       v4_error(V4E_KEYNOTINBUF,0,"V4IS","MakeFFKey","KEYNOTINBUF","Key buffer not the same as key position within get buffer") ;
	      memcpy(ffkey->KeyVal,(char *)databufptr+kid->Key[keynum].Offset,ffkey->kp.fld.Bytes) ;
	      if (ffkey->kp.fld.Bytes & 0x3)				/* If key not aligned then pad and align */
	       memset(&ffkey->KeyVal[ffkey->kp.fld.Bytes],0,4) ;
	      ffkey->kp.fld.Bytes = ALIGN(sizeof kid->Key[0].KeyPrefix + ffkey->kp.fld.Bytes) ;
	    } ;
	   return((KEY *)ffkey) ;
	 } ;
	v4_error(V4E_FILEREFNOTINAREA,0,"V4IS","MakeFFKey","FILEREFNOTINAREA","Could not locate fileref (%d) in area",fileref) ;
	return(NULL) ;
}

/*	v4is_BytesInFFKEY - Returns actual number (not padded to align) of bytes in FF Key	*/
/*	Call: bytes = v4is_BytesInFFKey( acb , fileref , keynum )
	  where acb is file's acb,
		fileref is file ref to used,
		keynum is key number for fileref						*/

int v4is_BytesInFFKey(acb,fileref,keynum)
  struct V4IS__AreaCB *acb ;
  int fileref ;
  int keynum ;
{ struct V4FFI__KeyInfo *ki ;
  struct V4FFI__KeyInfoDtl *kid ;
  int kx ;

	if ((ki = acb->KeyInfo) == NULL)
	 v4_error(V4E_AREANOFFI,0,"V4IS","BytesInFFKey","AREANOFFI","No foreign file key information in area") ;
	for((kid=(KID *)&ki->Buffer,kx=0);kx<ki->Count;(kx++,kid=(KID *)(kid+kid->Bytes)))	/* Look for fileref */
	 { if (kid->FileRef != fileref) continue ;
	   if (keynum >= kid->KeyCount)
	    v4_error(V4E_KEYNUMTOOBIG,0,"V4IS","BytesInFFKey","KEYNUMTOOBIG","KeyNum too big for specified foreign file FileRef") ;
	   if (kid->Key[keynum].KeyPrefix.all == 0)			/* Have to construct V4 key from key in buffer ? */
	    { return(-1) ; }						/* Not a FF Key */
	    else { return(kid->Key[keynum].KeyPrefix.fld.Bytes) ; } ;
	 } ;
	v4_error(V4E_FILEREFNOTINAREA,0,"V4IS","BytesInFFKey","FILEREFNOTINAREA","Could not locate fileref (%d) in area",fileref) ;
	return(-1) ;
}
