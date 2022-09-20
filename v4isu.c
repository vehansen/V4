/*	V4ISU.C - V4 Index Sequential Utilities

	Created 30-Mar-92 by Victor E. Hansen		*/

#define NEED_SOCKET_LIBS 1
#ifndef NULLID
#include "v4defs.c"
#include <setjmp.h>
#include <time.h>

#ifdef VMSOS
#include <file.h>
#include rms
   struct vms__dsc {
     short int length ;		/* Length of value */
     short int desc ;		/* Descriptive info */
     char *pointer ;		/* Pointer to value */
    } ;
#endif

#ifdef UNIX
/*#include <sys/mode.h> */
#include <fcntl.h>
#endif

#endif		/* NULLID */

//extern struct V4MM__MemMgmtMaster *V4_GLOBAL_MMM_PTR ;		/* Point to master area structure */
extern struct V4C__ProcessInfo *gpi ;	/* Global process information */

/*	Set up some abbreviations to make code more readable/managable	*/

#define UPD V4MM_BktPtr_Update
#define IDX V4MM_LockIdType_Index
#define DAT V4MM_LockIdType_Data
#define MEM V4MM_BktPtr_MemLock
#define IGNORELOCK V4MM_BktPtr_IgnoreLock
#define LOCKROOT lrx = v4mm_LockSomething(aid,V4MM_LockId_Root,V4MM_LockMode_Write,V4MM_LockIdType_Root,10,NULL,16)
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

/*	H I G H   L E V E L   C A L L S			*/

/*	v4is_Open - Opens up a new area & links to a file */
/*	Call: v4is_Open( pcb , ocb , errmsg )
	  where pcb is pointer to IS control block- struct V4IS__ParControlBlk,
	  ocb, if not NULL is pointer to option control block,
	  errmsg if not NULL is updated with any error messages		*/

LOGICAL v4is_Open(pcb,ocb,errmsg)
  struct V4IS__ParControlBlk *pcb ;
  struct V4IS__OptControlBlk *ocb ;
  UCCHAR *errmsg ;
{ struct V4IS__AreaCB *acb ;
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__RootInfo *ri ;
  struct V4IS__Bkt *bkt ;
  struct V4IS__OptControlBlk *cocb ;
  struct V4H__FixHash *vfh ;
  struct V4IS__IndexOnly *io,*riio ;
  struct V4__BktHdr *sbh,sosbh ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4C__AreaHInfo *ahi ;
  struct V4IS__FreeDataInfo *fdi ;
  struct V4FFI__KeyInfo *ki ;
  struct V4FFI__KeyInfoDtl *kid ;
#ifdef WINNT
  int bytes ;
#endif
#ifdef V4_BUILD_CS_FUNCTIONALITY
  struct V4CS__Msg_OpenFileRef ofr ;
  struct V4CS__Msg_FileRefAID fra ;
  struct V4CS__Msg_Error err ;
#endif
  time_t createtime ;
  int aid,ax,bs,bs1,gbc,lm,ct ;
  int i ; UCCHAR ebuf[250] ; UCCHAR *ubp ;

	pcb->AreaId = -1 ;					/* Flag until all OK */
	mmm = CURRENTMMM ;
	for(cocb=ocb;cocb!=NULL;cocb=cocb->Nextocb)	/* Make quick scan thru optional args - look for mmm override */
	 { if(cocb->ocbType != V4IS_OCBType_mmm) continue ;
	   mmm = (struct V4MM__MemMgmtMaster *)cocb->ocbPtr ; break ;
	 } ;
	acb = (struct V4IS__AreaCB *)v4mm_AllocChunk(sizeof *acb,TRUE) ;
	acb->pcb = pcb ;			/* Link acb to pcb VEH060928 */
	if (UCstrcmp(pcb->UCFileName,UClit("nil")) == 0) { pcb->OpenMode = V4IS_PCB_OM_Nil ; ZUS(pcb->UCFileName) ; } ;
	if (pcb->AccessMode == -1)		/* If access = -1 then init properly */
	 pcb->AccessMode = V4IS_PCB_AM_Get | V4IS_PCB_AM_Delete | V4IS_PCB_AM_Insert | V4IS_PCB_AM_Update | V4IS_PCB_AMF_Indexed ;
/*	If this is a new file then set up new root bucket & link up everything */
reopen:
	switch(pcb->OpenMode)
	 { default: v4_error(V4E_INVOPNMODE,0,"V4IS","Open","INVOPNMODE","Open Mode not yet supported (%d)",pcb->OpenMode) ;
	   case V4IS_PCB_OM_MustBeNew:
		ubp = v_UCLogicalDecoder(pcb->UCFileName,VLOGDECODE_Exists,0,errmsg) ;
		if (ubp == NULL) { if (pcb->AreaFlags & V4IS_PCB_OF_NoError) return(FALSE) ; ubp = pcb->UCFileName ; } ;
#ifdef WINNT
		pcb->hfile = UCCreateFile(ubp,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL) ;
		if (ubp != pcb->UCFileName) UCstrcpy(pcb->UCFileName,ubp) ;
		if (pcb->hfile == INVALID_HANDLE_VALUE) goto new_entry ;
		if (pcb->hfile == INVALID_HANDLE_VALUE) goto new_entry ;
		CloseHandle(pcb->hfile) ;
#else
		pcb->FilePtr = UCfopen(ubp,"rb+") ;	/* Try & open file */
		if (pcb->FilePtr == NULL) goto new_entry ;	/* if we got it assume we opened Update */
		fclose(pcb->FilePtr) ;
#endif
		v_Msg(NULL,ebuf,"V4ISExists",ubp) ;
		if (pcb->AreaFlags & V4IS_PCB_OF_NoError) { if (errmsg != NULL) UCstrcpy(errmsg,ebuf) ; return(FALSE) ; } ;
		v4_UCerror(V4E_MUSTBENEW,0,"V4IS","Open","MUSTBENEW",ebuf) ;
	   case V4IS_PCB_OM_NewIf:
		ubp = v_UCLogicalDecoder(pcb->UCFileName,VLOGDECODE_Exists,0,errmsg) ;
		if (ubp == NULL) { if (pcb->AreaFlags & V4IS_PCB_OF_NoError) return(FALSE) ; ubp = pcb->UCFileName ; } ;
#ifdef WINNT
		pcb->hfile = UCCreateFile(ubp,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL) ;
		if (pcb->hfile != INVALID_HANDLE_VALUE) { CloseHandle(pcb->hfile) ; pcb->OpenMode = V4IS_PCB_OM_Update ; goto reopen ; } ;
#else
		pcb->FilePtr = UCfopen(ubp,"rb+") ;	/* Try & open file */
		if (pcb->FilePtr != NULL)			/* if we got it assume we opened Update */
		 { /* setvbuf(pcb->FilePtr,NULL,_IONBF,0) ; */ fclose(pcb->FilePtr) ; pcb->OpenMode = V4IS_PCB_OM_Update ; goto reopen ; } ;
#endif
		pcb->OpenMode = V4IS_PCB_OM_New ; goto reopen ;
	   case V4IS_PCB_OM_Nil:
	   case V4IS_PCB_OM_NewAppend:
	   case V4IS_PCB_OM_NewTemp:
	   case V4IS_PCB_OM_New:
new_entry:
/*		Figure out a good bucket size */
		bs = (pcb->BktSize == 0 ? V4IS_BktSize_Dflt : pcb->BktSize) ;
		if (pcb->MaxRecordLen > 10)			/* Make sure bucket size can accomodate record length */
		 { bs1 = ((pcb->MaxRecordLen + sizeof(struct V4IS__DataBktHdr) + sizeof(struct V4IS__DataBktEntryHdr)) + (V4IS_BktIncrement-1)) & ~(V4IS_BktIncrement-1) ;
		   if (bs1 > bs) bs = bs1 ;			/* Take larger */
		 } ;
		bs = (bs + V4IS_BktIncrement-1) & ~(V4IS_BktIncrement-1) ;
		if (bs > V4IS_BktSize_Max)
		 { if (errmsg != NULL) v_Msg(NULL,errmsg,"V4ISRecSize",pcb->MaxRecordLen,pcb->UCFileName,V4IS_BktSize_Max) ;
		   if (pcb->AreaFlags & V4IS_PCB_OF_NoError) return(FALSE) ;
		   v4_error(V4E_INVRECBYTES,0,"V4IS","Open","INVRECBYTES",
			"Invalid record size (%d) for area (%s) - would exceed bucket size max (%d)",pcb->MaxRecordLen,UCretASC(pcb->UCFileName),V4IS_BktSize_Max) ;
		 } ;
		vfh = NULL ;
		for(cocb=ocb;cocb!=NULL;cocb=cocb->Nextocb)	/* Make quick scan thru optional args - may change bs! */
		 { switch (cocb->ocbType)
		    { default:	break ;
		      case V4IS_OCBType_Hash:
			vfh = (struct V4H__FixHash *)cocb->ocbPtr ;
			if (vfh->HashType == 0) vfh->HashType = V4H_HashType_Fixed ;
			if (vfh->RecordBytes == 0) vfh->RecordBytes = pcb->MaxRecordLen ;
			if (vfh->RecordBytes <= 0 || vfh->RecordBytes > V4H_EntryBuf_Max)
			 { if (errmsg != NULL) v_Msg(NULL,errmsg,"V4ISRecSizeH",pcb->MaxRecordLen,pcb->UCFileName,1,V4H_EntryBuf_Max) ;
			   if (pcb->AreaFlags & V4IS_PCB_OF_NoError) return(FALSE) ;
			   v4_error(V4E_INVRECBYTES,0,"V4IS","Open","INVRECBYTES",
					"Invalid record size (%d) for hash area (%s)",vfh->RecordBytes,UCretASC(pcb->UCFileName)) ;
			 } ;
			if (vfh->MaxRecords <= 0)
			 { if (errmsg != NULL) v_Msg(NULL,errmsg,"V4ISHashRecMax",vfh->MaxRecords,pcb->UCFileName) ;
			   if (pcb->AreaFlags & V4IS_PCB_OF_NoError) return(FALSE) ;
			   v4_error(V4E_INVMAXRECS,0,"V4IS","Open","INVMAXRECS",
					"Invalid max number of records (%d) for hash area (%s)",vfh->MaxRecords,UCretASC(pcb->UCFileName)) ;
			 } ;
			vfh->EntryBytes = ALIGN(sizeof(struct V4H__HashEntry)-V4H_EntryBuf_Max+vfh->RecordBytes) ;
			bs = vfh->MaxRecords*vfh->EntryBytes ;		/* Force proper bucket size */
			bs += (sizeof(struct V4IS__RootInfo) + sizeof(struct V4H__FixHash)
				+ (pcb->KeyInfo != NULL ? pcb->KeyInfo->Bytes : 0)  ) ;
		    } ;
		 } ;
		ubp = v_UCLogicalDecoder(pcb->UCFileName,VLOGDECODE_NewFile,0,errmsg) ;
//printf("ubp = %s\n",UCretASC(ubp)) ;
		if (ubp == NULL) { if (pcb->AreaFlags & V4IS_PCB_OF_NoError) return(FALSE) ; ubp = pcb->UCFileName ; } ;
		if (ubp != pcb->UCFileName) UCstrcpy(pcb->UCFileName,ubp) ;
#ifdef WINNT
		if (pcb->OpenMode == V4IS_PCB_OM_NewTemp)
		 { pcb->hfile = UCCreateFile(pcb->UCFileName,GENERIC_READ+GENERIC_WRITE,0,
		 				NULL,CREATE_ALWAYS,FILE_FLAG_DELETE_ON_CLOSE,NULL) ;
		 } else if (pcb->OpenMode != V4IS_PCB_OM_Nil)
		 { pcb->hfile = UCCreateFile(pcb->UCFileName,GENERIC_READ+GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL) ; } ;
		if (pcb->hfile == INVALID_HANDLE_VALUE)
		 { if (errmsg != NULL) v_Msg(NULL,errmsg,"FileError2a",pcb->UCFileName,GetLastError()) ;
//v_Msg(NULL,ebuf,"@Err open(%1U): %2O\n",pcb->UCFileName,GetLastError()) ; vout_UCText(VOUT_Err,0,ebuf) ;
		   if (pcb->AreaFlags & V4IS_PCB_OF_NoError) return(FALSE) ;
		   v4_error(V4E_NOFILECREATE,0,"V4IS","Open","NOFILECREATE","Could not create file: %s (%s)",UCretASC(pcb->UCFileName),v_OSErrString(GetLastError())) ;
		 } ;
#else
#ifdef VMSOS
		if (pcb->OpenMode != V4IS_PCB_OM_Nil)
		 pcb->FilePtr = UCfopen(pcb->UCFileName,"wb+","alq=128") ;
#else
		if (pcb->OpenMode != V4IS_PCB_OM_Nil)
		 pcb->FilePtr = UCfopen(pcb->UCFileName,"wb+") ;
#endif
		if (pcb->FilePtr == NULL && pcb->OpenMode != V4IS_PCB_OM_Nil)
		 { if (errmsg != NULL) v_Msg(NULL,errmsg,"FileError2a",pcb->UCFileName,errno) ;
		   if (pcb->AreaFlags & V4IS_PCB_OF_NoError) return(FALSE) ;
		   v4_error(V4E_NOFILECREATE,0,"V4IS","Open","NOFILECREATE","Could not create file: %s (%d)",UCretASC(pcb->UCFileName),errno) ;
		 } ;
#endif
		if (pcb->AccessMode & V4IS_PCB_AMF_Sequential)	/* Opening for Sequential Output only ? */
		 { pcb->AreaId = V4IS_AreaId_SeqOnly ;		/*  yes - give special aid */
		   free(acb) ;
		   sosbh.BktType = V4_BktHdrType_SeqOnly ; sosbh.BktNum = 0 ;
#ifdef WINNT
		   WriteFile(pcb->hfile,&sosbh,sizeof sosbh,&bytes,NULL) ;
#else
		   fwrite(&sosbh,sizeof sosbh,1,pcb->FilePtr) ;
#endif
		   return(TRUE) ;
		 } ;
		createtime = time(NULL) ;			/* Get create time for area */
#ifdef WINNT
		aid = v4mm_MakeNewAId(acb,bs,pcb->hfile,pcb->UCFileName) ;
#else
		aid = v4mm_MakeNewAId(acb,bs,pcb->FilePtr,pcb->UCFileName) ;
#endif
		acb->BktSize = bs ;
#ifdef segshare
		if (pcb->LockMode != 0)				/* Enable locking/file-sharing ? */
		 v4mm_ShareSegInit(aid,pcb->UCFileName,15,bs,50,createtime,FALSE) ;
#endif
		bkt = v4is_MakeBkt(aid,V4_BktHdrType_Root) ; sbh = (BH *)bkt ;
/*		If we have free data bucket list then get it now */
		if (vfh == NULL)				/* Do it unless we are making hash file */
		 { ibh = (IBH *)bkt ;
		   fdi = (struct V4IS__FreeDataInfo *)&bkt->Buffer[ibh->KeyEntryTop] ;
		   memset(fdi,0,sizeof *fdi) ;
		   acb->RootInfo->FreeDataInfoByte = ibh->KeyEntryTop ;
		   ibh->KeyEntryTop += ALIGN(sizeof *fdi) ; ibh->FreeBytes -= ALIGN(sizeof *fdi) ;
		   acb->FreeData = fdi ;
		 } else acb->FreeData = NULL ;
/*		If we have foreign file key info then get it into root index bucket now */
		if (pcb->KeyInfo != NULL)
		 { ibh = (IBH *)bkt ;
		   ki = (struct V4FFI__KeyInfo *)&bkt->Buffer[ibh->KeyEntryTop] ;	/* Link up to correct spot in root */
		   memcpy(ki,pcb->KeyInfo,pcb->KeyInfo->Bytes) ;			/* Copy info into Root */
		   acb->RootInfo->FFIKeyInfoByte = ibh->KeyEntryTop ;			/* Tweak indexes */
		   ibh->KeyEntryTop += ALIGN(ki->Bytes) ; ibh->FreeBytes -= ALIGN(ki->Bytes) ;
		   acb->KeyInfo = ki ;
		 } else acb->KeyInfo = NULL ;
/*		Check for any optional open info */
		acb->io = NULL ;
		for(cocb=ocb;cocb!=NULL;cocb=cocb->Nextocb)
		 { switch (cocb->ocbType)
		    { default:
			v4_error(V4E_INVOCBTYPE,0,"V4IS","Open","INVOCBTYPE","Invalid ocbType (%d) for area (%s)",cocb->ocbType,UCretASC(pcb->UCFileName)) ;
		      case V4IS_OCBType_Hash:			/* vfh has already been set above */
			if (vfh->FillPercent < 10 || vfh->FillPercent > 95) vfh->FillPercent = V4H_FillPercent_Default ;
			if (vfh->HashKeyBytes == 0)		/* No key defined ? */
			 { if (acb->KeyInfo == NULL)
			    v4_error(V4E_NOKEYINFO,0,"V4IS","Open","NOKEYINFO","Area (%s) - no key info specified",UCretASC(pcb->UCFileName)) ;
			   ki = acb->KeyInfo ;
		   	   if (ki->Count >= 1)
		    	    { kid = (KID *)&ki->Buffer ;
			      vfh->HashKeyOffset = kid->Key[0].Offset ; vfh->HashKeyBytes = kid->Key[0].KeyPrefix.fld.Bytes ;
			      switch (vfh->HashFunction == 0 ? kid->Key[0].KeyPrefix.fld.KeyMode : 99)
			       { default:	vfh->HashFunction = V4H_HashFunc_Alpha1 ; break ;
				 case V4IS_KeyMode_Int:	vfh->HashFunction = V4H_HashFunc_Int1 ; break ;
				 case 99:	break ; /* hash defined, don't update */
			       } ;
			    } ;
			 } ;
			break ;
		      case V4IS_OCBType_IndexOnly:
			io = (struct V4IS__IndexOnly *)cocb->ocbPtr ;		/* Link to structure */
			ibh = (IBH *)bkt ;
			riio = (struct V4IS__IndexOnly *)&bkt->Buffer[ibh->KeyEntryTop] ; /* Link up to correct spot in root */
			memcpy(riio,io,io->Bytes) ;				/* Copy info into Root */
			acb->RootInfo->IndexOnlyInfoByte = ibh->KeyEntryTop ;	/* Tweak indexes */
			ibh->KeyEntryTop += ALIGN(io->Bytes) ; ibh->FreeBytes -= ALIGN(io->Bytes) ;
			acb->io = io ;
		   } ;
		 } ;
		if (vfh != NULL)				/* Creating a hash file ? */
		 { ibh = (IBH *)bkt ;
		   acb->HashInfo = (int *)&bkt->Buffer[ibh->KeyEntryTop] ;
		   acb->RootInfo->AreaHInfoByte = ibh->KeyEntryTop ;
		   ibh->KeyEntryTop += ALIGN(sizeof *ahi) ; ibh->FreeBytes -= ALIGN(sizeof *ahi) ; /* Make room for it */
		   memcpy(acb->HashInfo,vfh,sizeof(*vfh)) ;
		   vfh = (struct V4H__FixHash *)acb->HashInfo ;	/* Link up vfh to area in RootInfo */
		   pcb->RelHNum = 0 ;				/* Force this to zip so we don't overwrite below */
		 } ;
/*		If this is going to handle V4 info then set up some special stuff */
		if (pcb->RelHNum > 0)
		 { ibh = (IBH *)bkt ;
		   ahi = (struct V4C__AreaHInfo *)&bkt->Buffer[ibh->KeyEntryTop] ;	/* Position for Heirarchy info */
		   acb->RootInfo->AreaHInfoByte = ibh->KeyEntryTop ;
		   ibh->KeyEntryTop += ALIGN(sizeof *ahi) ; ibh->FreeBytes -= ALIGN(sizeof *ahi) ; /* Make room for it */
		   ahi->RelHNum = pcb->RelHNum ;
		   acb->RootInfo->NextAvailIntDictNum = MAXDIMNUM + 1 ;
		   acb->RootInfo->NextAvailDimDictNum = ADDHNUMDIM(ahi->RelHNum,Dim_StartDimIndex) ;
		   acb->RootInfo->NextAvailExtDictNum = ADDHNUMDICT(ahi->RelHNum,1) ;
//		   acb->RootInfo->NextAvailValueNum = 1 ;
		   acb->RootInfo->NextAvailValueNum = ADDHNUMDICT(ahi->RelHNum,1) ; /* VEH040313 - ensure that values are unique by RelHNum */
		   acb->ahi = ahi ;
		 } ;
		acb->Lvl[acb->CurLvl=0].BktNum = sbh->BktNum ;	/* Set up index levels */
/*		Now back into max record length if we have'nt got one yet */
		if (pcb->MaxRecordLen <= 10)
		 pcb->MaxRecordLen = bs - (sizeof(struct V4IS__DataBktHdr) + sizeof(struct V4IS__DataBktEntryHdr)) ;
		acb->RootInfo->MaxRecordLen = pcb->MaxRecordLen ;	/* Stash max record length */
		acb->RootInfo->BktSize = bs ;				/* Stash bucket size */
		acb->RootInfo->Version = V4IS_MajorVersion*1000 + V4IS_MinorVersion ;
		acb->RootInfo->MinCmpBytes =
		  ( pcb->MinCmpBytes == 0 ? 50 : pcb->MinCmpBytes) ;
		acb->MinCmpBytes = acb->RootInfo->MinCmpBytes ;
		acb->RootInfo->CreateTime = createtime ;
		for(ax=0;ax<mmm->NumAreas;ax++)
		 { if (aid != mmm->Areas[ax].AreaId) continue ;
		   mmm->Areas[ax].MinBkts = 5 ;
		   mmm->Areas[ax].MaxBkts = 10 ;
		   break ;
		 } ;
		if (vfh != NULL)					/* If a hash file then set up RootInfo */
		 { acb->RootInfo->BktSize = vfh->RecordBytes*vfh->MaxRecords ; bs = acb->RootInfo->BktSize ;
		   acb->RootInfo->GlobalBufCount = 1 ;
		   acb->RootInfo->NumLevels = -1 ;
		 } ;
		SBHINUSE ;
		break ;
	   case V4IS_PCB_OM_Update:
	   case V4IS_PCB_OM_Read:
	   case V4IS_PCB_OM_Partial:
		ubp = v_UCLogicalDecoder(pcb->UCFileName,VLOGDECODE_Exists,0,errmsg) ;
		if (ubp == NULL) { if (pcb->AreaFlags & V4IS_PCB_OF_NoError) return(FALSE) ; ubp = pcb->UCFileName ; } ;
		if (ubp != pcb->UCFileName) UCstrcpy(pcb->UCFileName,ubp) ;
#ifdef WINNT
//VEH141020 - Added FILE_SHARE_DELETE so that an open area can be renamed (and updated)
		pcb->hfile = UCCreateFile(pcb->UCFileName,(pcb->OpenMode == V4IS_PCB_OM_Update ? GENERIC_READ|GENERIC_WRITE : GENERIC_READ),
			(pcb->LockMode != 0 ? FILE_SHARE_READ|FILE_SHARE_WRITE :(pcb->OpenMode == V4IS_PCB_OM_Update ? 0 : FILE_SHARE_READ|FILE_SHARE_DELETE)),
			NULL,OPEN_EXISTING,
			((pcb->AreaFlags & V4IS_PCB_OF_SeqScan) != 0 ? FILE_FLAG_SEQUENTIAL_SCAN : 0),
			NULL) ;
		if (pcb->hfile == INVALID_HANDLE_VALUE)
		 { if (errmsg != NULL) v_Msg(NULL,errmsg,"FileError1",pcb->UCFileName,GetLastError()) ;
		   if (pcb->AreaFlags & V4IS_PCB_OF_NoError) return(FALSE) ;
		   v4_error(V4E_NOFILEACCESS,0,"V4IS","Open","NOFILEACCESS","Could not access file: %s (%s)",UCretASC(pcb->UCFileName),v_OSErrString(GetLastError())) ;
		 } ;
//		i = GetFullPathName(pcb->FileName,sizeof pcb->FileName,pcb->FileName,NULL) ;
#else
		pcb->FilePtr = fopen(pcb->UCFileName,(pcb->OpenMode == V4IS_PCB_OM_Update ? "rb+" : "rb")) ;
		if (pcb->FilePtr == NULL)
		 { if (errmsg != NULL) v_Msg(NULL,errmsg,"FileError1",pcb->UCFileName,errno) ;
		   if (pcb->AreaFlags & V4IS_PCB_OF_NoError) return(FALSE) ;
		   v4_error(V4E_NOFILEACCESS,0,"V4IS","Open","NOFILEACCESS","Could not access file: %s (%s)",UCretASC(pcb->UCFileName),v_OSErrString(errno)) ;
		 } ;
#endif
#ifdef WINNT
		ReadFile(pcb->hfile,&sosbh,sizeof sosbh,&bytes,NULL) ;
#else
		fread(&sosbh,sizeof sosbh,1,pcb->FilePtr) ;	/* Suck up bucket header for root */
#endif
		if (sosbh.BktType == V4_BktHdrType_SeqOnly)
		 { if (sosbh.BktNum != 0)
		    { if (errmsg != NULL) v_Msg(NULL,errmsg,"V4ISSeqHdr",pcb->UCFileName) ;
		      if (pcb->AreaFlags & V4IS_PCB_OF_NoError) return(FALSE) ;
		      v4_error(V4E_INVSEQHDR,0,"V4IS","Open","INVSEQHDR","Invalid SeqOnly Header- BktNum <> 0") ;
		    } ;
		   pcb->AreaId = V4IS_AreaId_SeqOnly ;
		   free(acb) ;
		   return(TRUE) ;
		 } ;
#ifdef WINNT
		aid = v4mm_MakeNewAId(acb,sizeof *ri,pcb->hfile,pcb->UCFileName) ;
#else
		aid = v4mm_MakeNewAId(acb,sizeof *ri,pcb->FilePtr,pcb->UCFileName) ;
#endif
		bkt = v4mm_BktPtr(0,aid,IDX+IGNORELOCK) ; sbh = (BH *)bkt ;
		acb->Lvl[acb->CurLvl=0].BktNum = sbh->BktNum ;	/* Set up index levels */
		acb->CurPosKeyIndex = -1 ;				/* Position to BOF */
		acb->KeyPrefix.all = -1 ;				/* No positioning (more) */
		ri = (acb->RootInfo = (struct V4IS__RootInfo *)((char *)&bkt->Buffer + sizeof *ibh)) ;
		acb->MinCmpBytes = (pcb->MinCmpBytes == 0 ? ri->MinCmpBytes : pcb->MinCmpBytes) ;
		acb->MaxRecordLen = (pcb->MaxRecordLen < ri->MaxRecordLen ? pcb->MaxRecordLen : ri->MaxRecordLen) ;
/*		Got all info we need - flush the area & reopen with proper bucket size */
		bs = ri->BktSize ;
		i = ri->NumLevels ;
		gbc = ri->GlobalBufCount ; lm = ri->LockMax ; ct = ri->CreateTime ;
		SBHINUSE ; v4mm_AreaFlush(aid,FALSE) ;
		if (i >= V4IS_AreaCB_LvlMax - 2 && pcb->OpenMode != V4IS_PCB_OM_Read && (pcb->AreaFlags & V4IS_PCB_OF_ForceOpen) == 0)
		 {
#ifdef WINNT
		   CloseHandle(pcb->hfile) ;
#else
		   fclose(pcb->FilePtr) ;
#endif
		   if (pcb->AreaFlags & V4IS_PCB_OF_NoError) { if (errmsg) v_Msg(NULL,errmsg,"V4ISReformat",pcb->UCFileName) ; return(FALSE) ; } ;
		   v4_error(V4E_OPENTOOMANYLVLS,0,"V4IS","Open","OPENTOOMANYLVLS","Area (%s) cannot be opened for update- must reformat index first!",
				UCretASC(pcb->UCFileName)) ;
		 } ;
#ifdef WINNT
		aid = v4mm_MakeNewAId(acb,bs,pcb->hfile,pcb->UCFileName) ;
#else
		aid = v4mm_MakeNewAId(acb,bs,pcb->FilePtr,pcb->UCFileName) ;
#endif
#ifdef segshare
		if (pcb->LockMode != 0)				/* Enable locking/file-sharing ? */
		 { v4mm_ShareSegInit(aid,pcb->UCFileName,(gbc == 0 ? 15 : gbc),bs,(lm == 0 ? 50 : lm),ct,
					((pcb->AreaFlags & V4IS_PCB_OF_ForceClear) != 0)) ;
		   pcb->DataId = -1 ; pcb->AreaId = aid ; acb->CSaid = -1 ;
		   if (pcb->OpenMode == V4IS_PCB_OM_Partial) return(TRUE) ;
		 } ;
#endif
		bkt = v4mm_BktPtr(0,aid,MEM+IDX+IGNORELOCK) ; sbh = (BH *)bkt ;
		acb->Lvl[acb->CurLvl=0].BktNum = sbh->BktNum ;	/* Set up index levels */
		ri = (acb->RootInfo = (struct V4IS__RootInfo *)((char *)&bkt->Buffer + sizeof *ibh)) ;
		for(ax=0;ax<mmm->NumAreas;ax++)
		 { if (aid != mmm->Areas[ax].AreaId) continue ;
		   mmm->Areas[ax].MinBkts = ri->NumLevels + 1 ;
		   mmm->Areas[ax].MaxBkts = ri->NumLevels + 5 ;
		   break ;
		 } ;
		if (ri->AreaHInfoByte != 0)				/* Do we have V4 info ? */
		 { if (ri->AreaHInfoByte < 0 || ri->AreaHInfoByte > ri->BktSize)
		    { if (pcb->AreaFlags & V4IS_PCB_OF_NoError) { if (errmsg) v_Msg(NULL,errmsg,"V4ISInvFormat",pcb->UCFileName) ; return(FALSE) ; } ;
		      v4_error(V4E_BADV4AREA,0,"V4IS","Open","BADV4AREA","Area (%s) - not a valid V4 Area",UCretASC(pcb->UCFileName)) ;
		    } ;
		   if (ri->NumLevels == -1)				/*  but is this a hash file? */
		    { acb->HashInfo = (int *)&bkt->Buffer[ri->AreaHInfoByte] ; }
		    else { acb->ahi = (struct V4C__AreaHInfo *)&bkt->Buffer[ri->AreaHInfoByte] ;
			   if (acb->ahi->RelHNum < 0 || acb->ahi->RelHNum > V4DPI_WorkRelHNum)
			    { if (pcb->AreaFlags & V4IS_PCB_OF_NoError) { if (errmsg) v_Msg(NULL,errmsg,"V4ISInvFormat",pcb->UCFileName) ; return(FALSE) ; } ;
			      v4_error(V4E_BADV4AREA,0,"V4IS","Open","BADV4AREA","Area (%s) - not a valid V4 Area",UCretASC(pcb->UCFileName)) ;
			    } ;
			 } ;
		 } ;
		if (ri ->NumLevels == -1 && acb->HashInfo == NULL)
		 { if (pcb->AreaFlags & V4IS_PCB_OF_NoError) { if (errmsg) v_Msg(NULL,errmsg,"V4ISHashMissing",pcb->UCFileName) ; return(FALSE) ; } ;
		   v4_error(V4E_NOHASHINFO,0,"V4IS","Open","NOHASHINFO","Area (%s) - missing HashInfo",UCretASC(pcb->UCFileName)) ;
		 } ;
		if (ri->FreeDataInfoByte > 0)
		 acb->FreeData = (struct V4IS__FreeDataInfo *)&bkt->Buffer[ri->FreeDataInfoByte] ;
		if (ri->FFIKeyInfoByte > 0)
		 { acb->KeyInfo = (struct V4FFI__KeyInfo *)&bkt->Buffer[ri->FFIKeyInfoByte] ; pcb->KeyInfo = acb->KeyInfo ;
		   ki = acb->KeyInfo ;				/* If foreign keys then update pcb with first fileref */
		   if (ki->Count >= 1)				/*   as a convenience to V3 (V3 will move to dflt_fileref!) */
		    { kid = (KID *)&ki->Buffer ; pcb->FileRef = kid->FileRef ; } ;
		 } ;
		if (ri->IndexOnlyInfoByte > 0)
		 { acb->io = (struct V4IS__IndexOnly *)&bkt->Buffer[ri->IndexOnlyInfoByte] ; }
		 else { acb->io = NULL ; } ;
                if (ri->NextUnusedBkt >= ri->NextAvailBkt)
                 { UCsprintf(ebuf,250,UClit("Open: NextUnusedBkt (%d) >= NextAvailBkt(%d) - will clear"),ri->NextUnusedBkt,ri->NextAvailBkt) ;
                   v4is_BugChk(aid,UCretASC(ebuf)) ;
                   ri->NextUnusedBkt = 0 ;
                 } ;
		SBHINUSE ;
		break ;
	 } ;
/*	Make sure we have valid access modes in acb */
	mmm->Areas[ax].WriteAccess = FALSE ;			/* Assume no write access */
	acb->AccessMode = 0 ;
	if ( (pcb->AccessMode & V4IS_PCB_AM_Get) ) acb->AccessMode |= V4IS_PCB_AM_Get ;
	switch (pcb->OpenMode)
	 { case V4IS_PCB_OM_Update:
	   case V4IS_PCB_OM_Nil:
	   case V4IS_PCB_OM_New:
	   case V4IS_PCB_OM_NewIf:
	   case V4IS_PCB_OM_NewAppend:
	   case V4IS_PCB_OM_NewTemp:			/* Only allow other modes if area open for update */
	 	if (pcb->AccessMode & V4IS_PCB_AM_Delete) acb->AccessMode |= V4IS_PCB_AM_Delete ;
	 	if (pcb->AccessMode & V4IS_PCB_AM_Insert) acb->AccessMode |= V4IS_PCB_AM_Insert ;
	 	if (pcb->AccessMode & V4IS_PCB_AM_Update) acb->AccessMode |= V4IS_PCB_AM_Update ;
		mmm->Areas[ax].WriteAccess = TRUE ;		/* Write access to file/area */
	 } ;
	pcb->AccessMode = acb->AccessMode ;
/*	Check to see if this fileref is really on a remote server */
	acb->CSaid = -1 ;					/* Assume no server */
#ifdef V4_BUILD_CS_FUNCTIONALITY
	if (mmm->rfr != NULL && pcb->OpenMode != V4IS_PCB_OM_New)
	 { acb->CSFlags = 0 ;			/* No flags/option for now */
	   if ((acb->CSSocket = v4is_ConnectFileRef(pcb->FileRef,acb)) >= 0)
/*	      Send message to server requesting aid of file */
	    { ofr.mh.MsgType = V4CS_Msg_OpenFileRef ;	/* Set up message */
	      ofr.mh.Flags = 0 ; ofr.mh.Bytes = sizeof ofr ;
	      ofr.FileRef = pcb->FileRef ; ofr.OpenMode = pcb->OpenMode ;
	      ofr.AccessMode = pcb->AccessMode ; ofr.ShareMode = pcb->LockMode ;
	      if (acb->CSFlags & V4CS_Flag_FlipEndian)
	       { FLIPLONG(ofr.FileRef,ofr.FileRef) ; FLIPLONG(ofr.OpenMode,ofr.OpenMode) ;
	         FLIPLONG(ofr.AccessMode,ofr.AccessMode) ; FLIPLONG(ofr.ShareMode,ofr.ShareMode) ;
		 FLIPSHORT(ofr.mh.Bytes,ofr.mh.Bytes) ;
	       } ;
	      if ((i = send(acb->CSSocket,&ofr,sizeof ofr,0)) < sizeof ofr)
	       v4_error(V4E_SENDERR,0,"V4IS","OPEN","SENDERR",
			"Error (%s) (len = %d/%d) in send on socket (%d)",v_OSErrString(NETERROR),ofr.mh.Bytes,i,UCretASC(pcb->UCFileName)) ;
	      if ((i = recv(acb->CSSocket,&err.mh,sizeof err.mh,0)) < 0)
	       v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
	      switch (err.mh.MsgType)
	       { default:
			v4_error(V4E_UNEXPSRVMSG,0,"V4IS","GET","UNEXPSRVMSG","Unexpected server message (%d) on Get to file (%s)",
				err.mh.MsgType,UCretASC(pcb->UCFileName)) ;
		 case V4CS_Msg_Error:		/* Got an error, read remainder of message */
			if ((i = recv(acb->CSSocket,(char *)&err+sizeof err.mh,err.mh.Bytes-sizeof err.mh,0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","PUT","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			v4_error(V4E_SRVERR,0,"V4IS","PUT","SRVERR",err.Message) ;
		 case V4CS_Msg_FileRefAID:
			if ((i = recv(acb->CSSocket,(char *)&fra+sizeof err.mh,sizeof(fra)-sizeof(err.mh),0)) < 0)
			 v4_error(V4E_RECVERR,0,"V4IS","OPEN","RECVERR","Error (%s) in recv on socket (%s)",v_OSErrString(NETERROR),UCretASC(pcb->UCFileName)) ;
			acb->CSaid = fra.AreaId ;
			if (acb->CSFlags & V4CS_Flag_FlipEndian) FLIPLONG(acb->CSaid,fra.AreaId) ;
			break ;
	       } ;
	    } ;
	 } ;
#endif
/*	Store results back in pcb */
	pcb->DataId = -1 ;					/* NO current record */
	pcb->AreaId = aid ;
	pcb->GetLen = acb->RootInfo->MaxRecordLen ;		/* Save max record length */
#ifdef VMSOS
	fgetname(pcb->FilePtr,UCretASC(pcb->UCFileName),0) ;
#endif
/*	Make sure nothing is locked which should not be */
	RELIDX RELDAT RELTREE
	mmm->Areas[ax].IndexOnlyCount = 0 ;
	if (acb->io != NULL)
	 { if (acb->io->count < 0 || acb->io->count > V4IS_IndexOnly_Max)
	    v4_error(V4E_BADV4AREA,0,"V4IS","Open","BADV4AREA","Area (%s) - not a valid V4 Area",UCretASC(pcb->UCFileName)) ;
	   for(i=0;i<acb->io->count;i++)
	    { mmm->Areas[ax].IndexOnly[i].BktOffset = acb->io->Entry[i].BktOffset ;
#ifdef WINNT
#else
	      mmm->Areas[ax].IndexOnly[i].ioFilePtr = fopen(UCretASC(acb->io->Entry[i].AreaName),"rb") ;
#endif
	      mmm->Areas[ax].IndexOnlyCount++ ;
	    } ;
	 } ;
	return(TRUE) ;
}

/*	v4is_Close - Closes up an area */
/*	Call: v4is_Close( pcb )
	  where pcb is pointer to IS control block- struct V4IS__ParControlBlk	*/

void v4is_Close(pcb)
  struct V4IS__ParControlBlk *pcb ;
{ struct V4IS__AreaCB *acb ;
  struct V4MM__MemMgmtMaster *mmm ;
#ifdef V4_BUILD_CS_FUNCTIONALITY
  struct V4CS__Msg_CloseAID caid ;
#endif
  int i,ax,buflen ;


	if (pcb->AreaId < 0) return ;				/* Not properly opened! */
//	mmm = v4mm_MemMgmtInit(NULL) ;		/* Initialize if necessary */
	if (pcb->OpenMode == V4IS_PCB_OM_Nil) return ;
#ifdef WINNT
	if (pcb->AreaId == V4IS_AreaId_SeqOnly)			/* Handle sequential-only */
	 { if (pcb->OpenMode == V4IS_PCB_OM_New)
	    { buflen = V4IS_EOFMark_SeqOnly ; WriteFile(pcb->hfile,&buflen,sizeof buflen,&i,NULL) ; } ; /* Write EOF Mark */
	   CloseHandle(pcb->hfile) ; pcb->AreaId = -1 ;
	   return ;
	 } ;
#else
	if (pcb->AreaId == V4IS_AreaId_SeqOnly)			/* Handle sequential-only */
	 { if (pcb->OpenMode == V4IS_PCB_OM_New)
	    { buflen = V4IS_EOFMark_SeqOnly ; fwrite(&buflen,sizeof buflen,1,pcb->FilePtr) ; } ; /* Write EOF Mark */
	   fclose(pcb->FilePtr) ; pcb->AreaId = -1 ;
	   return ;
	 } ;
#endif
	FINDAREAINPROCESS(ax,pcb->AreaId)
	if (ax == UNUSED) return ;			/* Could not find area? */
	acb = v4mm_ACBPtr(pcb->AreaId) ;
	v4mm_AreaFlush(pcb->AreaId,TRUE) ;			/* Flush out any/all buckets */
	if (acb->AuxDataBufPtr != NULL) { v4mm_FreeChunk(acb->AuxDataBufPtr) ; acb->AuxDataBufPtr = NULL ; } ;
#ifdef WINNT
	CloseHandle(pcb->hfile) ;
#else
	fclose(pcb->FilePtr) ;					/* Close the file associated with this area */
#endif
	if (acb->io != NULL)
	 { 
//	   for(ax=0;ax<mmm->NumAreas;ax++) { if (mmm->Areas[ax].AreaId == pcb->AreaId) break ; } ;
//	   if (ax < mmm->NumAreas)
	   for(i=0;i<acb->io->count;i++)			/* Close any index-only linked areas */
#ifdef WINNT
	       { continue ; } ;	/* NEEDS  WORK */
#else
	       { if (mmm->Areas[ax].IndexOnly[i].ioFilePtr != NULL) fclose(mmm->Areas[ax].IndexOnly[i].ioFilePtr) ; } ;
#endif
	 } ;
#ifdef V4_BUILD_CS_FUNCTIONALITY
	if (acb->CSaid >= 0)					/* Close corresponding file on remote server ? */
	 {
/*	   Send message to server requesting aid of file */
	   caid.mh.MsgType = V4CS_Msg_CloseAID ;	/* Set up message */
	   caid.mh.Flags = 0 ; caid.mh.Bytes = sizeof caid ;
	   caid.AreaId = acb->CSaid ;
	   if ((i = send(acb->CSSocket,&caid,caid.mh.Bytes,0)) < caid.mh.Bytes)
	    v4_error(V4E_SENDERR,0,"V4IS","CLOSE","SENDERR",
			"Error (%s) (len = %d/%d) in send on socket (%d)",v_OSErrString(NETERROR),caid.mh.Bytes,i,UCretASC(pcb->UCFileName)) ;
	   v4is_DisconnectAreaId(pcb->AreaId,acb->CSSocket) ;
	   if (pcb->AreaFlags & V4IS_PCB_OF_DisconnectOnClose)	/* If disconnecting from server then shutdown socket */
	    SOCKETCLOSE(acb->CSSocket) ;
	 } ;
#endif
	for(i=0;i<pcb->AuxLinkCnt;i++)				/* If any linked areas then zip-zop thru and close also */
	 { v4is_Close(pcb->Link[i].PCBPtr) ; } ;
	if (pcb->OpenMode == V4IS_PCB_OM_NewTemp)		/* Delete this baby? */
	 UCremove(pcb->UCFileName) ;				/*  then try to delete */
	if (acb->dil != NULL) v4mm_FreeChunk(acb->dil) ;	/* Free up list if data-only reads */
	if (acb->AuxDataBufPtr != NULL) v4mm_FreeChunk(acb->AuxDataBufPtr) ;
	v4mm_FreeChunk(acb) ;					/* Free up chunk allocated to ACB */
}

/*	v4is_Delete - Deletes the current record		*/
/*	Call: xxx = v4is_Delete( aid , pcb , fileref )
	  where aid is area id,
		pcb is parameter control block,
		fileref is FileRef under consideration (0 for V4 internal)		*/

void v4is_Delete(aid,pcb,fileref)
  int aid ;
  struct V4IS__ParControlBlk *pcb ;
  int fileref ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__Bkt *bkt ;
  struct V4__BktHdr *sbh ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__DataBktHdr *dbh ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__DataBktEntryHdr *dbeh ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4IS__IndexBktKeyList *ibkl ;
  union V4MM__DataIdMask dim ;
  struct V4IS__Key *key ;
  union V4IS__KeyPrefix *kp ;
  struct V4IS__KeyCmpList kcl ;
  struct {
    union V4IS__KeyPrefix kp ;
    char KeyVal[V4IS_KeyBytes_Max] ;
   } lclkey ;
  int index,ax,kx ; char *dataptr ; char ebuf[200] ;

	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return ;			/* Could not find area? */
//	ax = aid ;
//	mmm = V4_GLOBAL_MMM_PTR ;
	acb = v4mm_ACBPtr(aid) ;
	if (pcb->DataId == -1)
	 v4_error(V4E_NORECTODEL,0,"V4IS","Delete","NORECTODEL","No current record to delete") ;
	dim.dataid = pcb->DataId ;				/* Decode the DataId */
	bkt = v4mm_BktPtr(dim.fld.BktNum,aid,DAT) ; dbh = (DBH *)bkt ; /* Link to data bucket */
/*	Before we get too carried away, let's make sure we actually have a data bucket */
	switch (dbh->sbh.BktType)
	 { default: v4_error(V4E_INVBKTTYPE,0,"V4IS","Get","INVBKTTYPE","Unknown bucket type-5 (%d)",dbh->sbh.BktType) ;
	   case V4_BktHdrType_Data:
/*		Scan thru data bucket for wanted data entry */
		for(index=dbh->FirstEntryIndex;index<dbh->FreeEntryIndex;index+=ALIGN(dbeh->Bytes))
		 { dbeh = (struct V4IS__DataBktEntryHdr *)&bkt->Buffer[index] ;
		   if (dbeh->Bytes == 0)
		    v4_error(V4E_INVDBEHLEN,0,"V4IS","Get","INVDBEHLEN","Area (%d %s) Bucket (%d) invalid dbeh->Bytes",
				aid,UCretASC(mmm->Areas[ax].UCFileName),dbh->sbh.BktNum) ;
		   if (dbeh->DataId != pcb->DataId) continue ;
		   v4is_CopyData(aid,&dataptr,0,(char*)dbeh + sizeof *dbeh,dbeh->Bytes-sizeof *dbeh,
					dbeh->CmpMode,pcb,V4IS_PCB_RetPtr) ;
		   DBHINUSE ;
		   goto delete_it ;
		 } ;
		v4_error(V4E_BADDATAID,0,"V4IS","Delete1","BADDATAID","Data record associated with dataid (%x) cannot be found",pcb->DataId) ;
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
			   if (ikde->DataId != pcb->DataId) continue ;	/* Is this the one we want? */
			   if (ikde->KeyNotInRec)
			    { key = (KEY *)((char *)ikde + sizeof *ikde) ;
			      v4is_CopyData(aid,&dataptr,0,(char *)ikde+sizeof *ikde+key->Bytes,
					 ikde->AuxVal-sizeof *ikde-key->Bytes,ikde->CmpMode,pcb,V4IS_PCB_RetPtr) ;
			    } else v4is_CopyData(aid,&dataptr,0,(char *)ikde+sizeof *ikde,ikde->AuxVal-sizeof *ikde,
						 ikde->CmpMode,pcb,V4IS_PCB_RetPtr) ;
			   IBHINUSE ;
			   goto delete_it ;
       		    } ;
		  } ;
		v4_error(V4E_BADDATAID,0,"V4IS","Delete2","BADDATAID","Data record associated with dataid (%x) cannot be found",pcb->DataId) ;
	 } ;
delete_it:
/*	First build up list of keys to this record so we can be sure & delete them all */
	kcl.count = 0 ;
	if (fileref != 0)
	 { for(kx=0;kx<v4is_FFKeyCount(acb,fileref);kx++)
	    { v4is_MakeFFKey(acb,fileref,&lclkey,kx,NULL,dataptr,TRUE) ;
	      kp = (KP *)&lclkey ; memcpy(&kcl.Entry[kcl.count].OKey,kp,kp->fld.Bytes) ; kcl.count ++ ;
	    } ;
	 } else			 /* Got internal key- begin of record buffer is key */
	 { kp = (KP *)dataptr ; memcpy(&kcl.Entry[kcl.count].OKey,dataptr,kp->fld.Bytes) ;
	   kcl.count ++ ;
	 } ;
	RELDAT RELIDX
	v4mm_LockSomething(aid,V4MM_LockId_Tree,V4MM_LockMode_Write,V4MM_LockIdType_Tree,33,&mmm->Areas[ax].LockTree,17) ;
	for(kx=0;kx<kcl.count;kx++)					/* All keys built- go thru & delete */
	 { if (v4is_PositionKey(aid,&kcl.Entry[kx].OKey,NULL,pcb->DataId,V4IS_PosBoKL) != V4RES_PosKey)
	    { sprintf(ebuf,"PositionKey failure for key (%d), DataId (%x) in v4is_Delete",kx+1,pcb->DataId) ;
	      v4is_BugChk(aid,ebuf) ;
	      continue ;						/* Key may be gone due to prior attempts (?!) */
	      v4_error(V4E_NOKEYPOSTODEL,0,"V4IS","Delete","NOKEYPOSTODEL","Could not position to key in index to delete") ;
	    } ;
	   IFTRACEAREA
	    { sprintf(ebuf,"Delete: RemoveIndexEntry(bkt=%d pos=%d key=%s) dataid=%x kx=%d",
			acb->Lvl[acb->CurLvl].BktNum,acb->CurPosKeyIndex,v4is_FormatKey(&kcl.Entry[kx].OKey),pcb->DataId,kx) ;
	    } ;
	   v4is_RemoveIndexEntry(pcb,aid,acb->CurLvl,acb->CurPosKeyIndex,NULL) ;
	 } ;
	bkt = v4mm_BktPtr(dim.fld.BktNum,aid,DAT+UPD) ; sbh = (BH *)bkt ;/* Get bucket with data */
	if (sbh->BktType == V4_BktHdrType_Data)
	 { v4is_RemoveData(aid,bkt,pcb->DataId,TRUE) ;			/* If data in other bucket then trash it */
	   v4is_DataBktFree(aid,acb,(struct V4IS__DataBktHdr *)bkt,FALSE) ;	/* Maybe save number of free bytes */
	 } ;
	SBHINUSE ;
	acb->RootInfo->RecordCount-- ;					/* Decrement record count */
	acb->CurPosKeyIndex-- ;						/* Adjust so "next-record" will work */
	pcb->DataId = -1 ;						/* No more "current record" */
/*	Make sure nothing is locked which should not be */
	RELIDX RELDAT RELTREE
}

/*	v4is_ObsoleteData - Marks data as obsolete			*/
/*	Call: v4is_ObsoleteData( aid , dataid )
	  where aid is area id,
		dataid is ID of record to be marked as obsolete		*/

void v4is_ObsoleteData(aid,dataid)
  int aid ;
  int dataid ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__DataBktHdr *dbh ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__DataBktEntryHdr *dbeh ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__Bkt *bkt ;
  union V4MM__DataIdMask dim ;
  int ax,index ;

	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return ;			/* Could not find area? */
//	ax = aid ;
//	mmm = V4_GLOBAL_MMM_PTR ;
	acb = v4mm_ACBPtr(aid) ;
	dim.dataid = dataid ;				/* Decode the DataId */
	bkt = v4mm_BktPtr(dim.fld.BktNum,aid,DAT+UPD) ; dbh = (DBH *)bkt ; /* Link to data bucket */
/*	Before we get too carried away, let's make sure we actually have a data bucket */
	switch (dbh->sbh.BktType)
	 { default: v4_error(V4E_INVBKTTYPE,0,"V4IS","Get","INVBKTTYPE","Unknown bucket type-6 (%d)",dbh->sbh.BktType) ;
	   case V4_BktHdrType_Data:
/*		Scan thru data bucket for wanted data entry */
		for(index=dbh->FirstEntryIndex;index<dbh->FreeEntryIndex;index+=ALIGN(dbeh->Bytes))
		 { dbeh = (struct V4IS__DataBktEntryHdr *)&bkt->Buffer[index] ;
		   if (dbeh->Bytes == 0)
		    v4_error(V4E_INVDBEHLEN,0,"V4IS","Get","INVDBEHLEN","Area (%d %s) Bucket (%d) invalid dbeh->Bytes",
				aid,UCretASC(mmm->Areas[ax].UCFileName),dbh->sbh.BktNum) ;
		   if (dbeh->DataId != dataid) continue ;
		   dbeh->FileRef = V4IS_FileRef_Obsolete ;
		   DBHINUSE ;
		   return ;
		 } ;
		v4_error(V4E_BADDATAID,0,"V4IS","Obsolete","BADDATAID","Data record associated with dataid (%x) cannot be found",dataid) ;
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
			   IBHINUSE ;
			   return ;
       		    } ;
		  } ;
		v4_error(V4E_BADDATAID,0,"V4IS","Obsolete","BADDATAID","Data record associated with dataid (%x) cannot be found",dataid) ;
	 } ;
}
/*	v4is_Append - Appends record to end of area, used for rebuilding area index */
/*	Call: v4is_Append( aid , primekey , bufptr , datamode , cmpmode , multkeys , linkdataid )
	  where aid is area,
		primekey is pointer to primary key,
		bufptr is pointer to buffer being written
		pcb is pointer to instruction structure, see V4IS__ParControlBlk structure,
		cmpmode is data compression mode- V4IS_DataCmp_xxx,
		multkeys is TRUE if this is primary and more to follow,
		linkdataid is dataid to use if appending non-primary key (bufptr s/b NULL!)	*/

void v4is_Append(aid,primekey,bufptr,bufbytes,datamode,cmpmode,multkeys,linkdataid)
  int aid ;
  struct V4IS__Key *primekey ;
  void *bufptr ;
  int bufbytes ;
  int datamode ;
  int cmpmode ;
  LOGICAL multkeys ;
  int linkdataid ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__Bkt *bkt ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__Key *key ;
  struct V4IS__Key skey ;
  struct V4IS__IndexBktHdr *ibh ;
  int bktnum,oldbktnum,childbktnum,stashdata,ax,lx,fileref,newparent,newbktnum ;

	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return ;			/* Could not find area? */
//	ax = aid ;
//	mmm = V4_GLOBAL_MMM_PTR ;
	acb = v4mm_ACBPtr(aid) ;
	if (bufptr == NULL)						/* Are we just stashing key or do we have data record ? */
	 { if (v4is_StashIndexKey(aid,primekey,multkeys,linkdataid)) return ;	/* Just try to stash key */
	 } else								/* Try to store data in data bucket */
	 { switch (primekey->KeyType)
	    { default:v4_error(V4E_BADPRIMEKEYTYPE,0,"V4IS","Insert","BADPRIMEKEYTYPE","Invalid primary key type (%d)",primekey->KeyType) ;
	      case V4IS_KeyType_FrgnKey1: case V4IS_KeyType_FrgnKey2: case V4IS_KeyType_FrgnKey3: case V4IS_KeyType_FrgnKey4:
	      case V4IS_KeyType_FrgnKey5: case V4IS_KeyType_FrgnKey6: case V4IS_KeyType_FrgnKey7:	fileref = primekey->AuxVal ; break ;
//	      case V4IS_KeyType_FrgnKey5:	fileref = primekey->AuxVal ; break ;
	      case V4IS_KeyType_V4Segments: case V4IS_KeyType_V4: case V4IS_KeyType_AggShell: case V4IS_KeyType_xdbRow:
	      case V4IS_KeyType_Binding:	fileref = 0 ; break ;
	    } ;
	   if (stashdata=v4is_StashData(aid,bufptr,bufbytes,datamode,cmpmode,0,primekey->AuxVal,acb->RootInfo->FillPercent))
	    { if (v4is_StashIndexKey(aid,primekey,multkeys,acb->DataId)) return ;	/* That was easy */
	    } else { if (v4is_StashIndexData(aid,primekey,bufptr,bufbytes,cmpmode,multkeys,0)) return ; } ;
	 } ;
/*	No room in current index bucket - have to create new bucket, link to parent, etc. */
/*	Better lock down entire index tree - have no idea what is gonna happen */
	RELIDX RELDAT
	v4mm_LockSomething(aid,V4MM_LockId_Tree,V4MM_LockMode_Write,V4MM_LockIdType_Tree,35,&mmm->Areas[ax].LockTree,18) ;
	bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,aid,IDX+UPD) ;	/* Get current index */
	ibh = (IBH *)bkt ;
	if (ibh->sbh.BktType == V4_BktHdrType_Root)			/* Is this the root (i.e. no parent?) */
	 { childbktnum = v4is_IndexSplit(aid,bkt,0,NULL,-1) ;		/* Copy all entries to new bucket */
	   IBHINUSE ;
	   for(lx=acb->CurLvl;lx>0;lx--)
	    { acb->Lvl[lx+1].BktNum = acb->Lvl[lx].BktNum ; } ;		/* Drop everything down a level */
	   acb->CurLvl++ ; acb->RootInfo->NumLevels ++ ;
	   acb->Lvl[1].BktNum = childbktnum ;				/* Make new index directly below root */
	   bkt = v4mm_BktPtr(childbktnum,aid,IDX+UPD) ;			/* Link up to child */
	   ibh = (IBH *)bkt ;			/* Inform child of parent */
	   ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
	   key = (KEY *)((char *)&bkt->Buffer[ibkl->DataIndex[0]] + sizeof(struct V4IS__IndexKeyDataEntry)) ; /* Get pointer to first key */
	   memcpy(&skey,key,key->Bytes) ;
	   v4is_IndexStashAppendLink(aid,0,&skey) ;			/* Insert first key of child as link from parent */
	   IBHINUSE ;
/*	   Try to stash again, may have freed up some space */
	   if (bufptr == NULL)
	    { if (v4is_StashIndexKey(aid,primekey,multkeys,linkdataid)) return ;
	    } else
	    { if (stashdata)
	       { if (v4is_StashIndexKey(aid,primekey,multkeys,acb->DataId)) return ;
	       } else { if (v4is_StashIndexData(aid,primekey,bufptr,bufbytes,cmpmode,multkeys,0)) return ; } ;
	    } ;
	 } else IBHINUSE ;
	bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,aid,IDX+UPD) ;	/* Get current index */
	ibh = (IBH *)bkt ; oldbktnum = ibh->sbh.BktNum ;
	ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
	key = (KEY *)((char *)&bkt->Buffer[ibkl->DataIndex[ibh->KeyEntryNum-1]] + sizeof(struct V4IS__IndexKeyDataEntry)) ; /* Get pointer to last key */
	if (memcmp(key,primekey,primekey->Bytes) == 0)			/* Does current key match that of last key? */
	 { childbktnum = v4is_IndexSplit(aid,bkt,ibh->KeyEntryNum-1,NULL,-1) ;	/*  Yes - split at begin of range */
	   IBHINUSE ;
	   bkt = v4mm_BktPtr(childbktnum,aid,IDX+UPD) ;			/* Link up to child */
	   ibh = (IBH *)bkt ; acb->CurPosKeyIndex = ibh->KeyEntryNum ;
	 } else
	 { IBHINUSE ;
	   bkt = v4is_MakeBkt(aid,V4_BktHdrType_Index) ;			/* Make new index bucket */
	   ibh = (IBH *)bkt ; ibh->ParentBktNum = acb->Lvl[acb->CurLvl-1].BktNum ;
	   acb->CurPosKeyIndex = 0 ;
	 } ;
	acb->Lvl[acb->CurLvl].BktNum = ibh->sbh.BktNum ;		/* Define new current at lowest level */
	newbktnum = ibh->sbh.BktNum ; newparent = ibh->ParentBktNum ;
	IBHINUSE ;
	v4mm_BktFlush(oldbktnum,aid) ;				/* Flush the full index bucket */
/*	Now try to stash key/data in new bucket */
	if (bufptr == NULL)
	 { if (!v4is_StashIndexKey(aid,primekey,multkeys,linkdataid))
	    v4_error(V4E_STASHFAILKEY,0,"V4IS","Append","STASHFAILKEY","Cannot stash data (after multiple attempts)") ;
	 } else
	 { if (stashdata)
	    { if (!v4is_StashIndexKey(aid,primekey,multkeys,acb->DataId))
	       v4_error(V4E_STASHFAILKEY,0,"V4IS","Append","STASHFAILKEY","Cannot stash data (after multiple attempts)") ;
	    } else { if (!v4is_StashIndexData(aid,primekey,bufptr,bufbytes,cmpmode,multkeys,0))
		      v4_error(V4E_STASHFAILDATA,0,"V4IS","Append","STASHFAILDATA","Cannot stash data (after multiple attempts)") ;
	           } ;
	 } ;
	bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,aid,IDX+UPD) ;	/* Link up to new index */
	ibh = (IBH *)bkt ;
	ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
	key = (KEY *)((char *)&bkt->Buffer[ibkl->DataIndex[0]] + sizeof(struct V4IS__IndexKeyDataEntry)) ;	/* Get pointer to first key */
	memcpy(&skey,key,key->Bytes) ;
	bktnum = v4is_IndexStashAppendLink(aid,acb->CurLvl-1,&skey) ;	/* Insert first key of child as link from parent */
	if (newparent != bktnum)
	 { bkt = v4mm_BktPtr(newbktnum,aid,IDX+UPD) ; ibh = (IBH *)bkt ; ibh->ParentBktNum = bktnum ; } ;
	IBHINUSE ;
}

/*	v4is_IndexStashAppendLink - Adds key & link to child into parent bucket (in APPEND mode)	*/
/*	NOTE: This may be a recursive call if parent is full!			*/
/*	Call: bktnum = v4is_IndexStashAppendLink( aid , parentlvl , key )
	  where bktnum is bucket where key was stashed,
		aid is area id,
		parentlvl is acb->Lvl of parent,
		key is point to key to be inserted into parent			*/

int v4is_IndexStashAppendLink(aid,parentlvl,key)
  int aid ;
  int parentlvl ;
  struct V4IS__Key *key ;
{ struct V4IS__Bkt *bkt ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4IS__Key *key2 ;
  struct V4IS__Key skey ;
  int ix,lx,bktnum,oldbktnum,newparent,curlvl ;

/*	Link up to everything */
	acb = v4mm_ACBPtr(aid) ;
	bkt = v4mm_BktPtr(acb->Lvl[parentlvl].BktNum,aid,IDX+UPD) ;	/* Get parent node */
	ibh = (IBH *)bkt ; oldbktnum = ibh->sbh.BktNum ;
	if (ibh->FreeBytes < (sizeof *ikde + 4 + key->Bytes))
	 { if (ibh->sbh.BktType != V4_BktHdrType_Root)			/* No room in parent - this may get messy */
	    { ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
	      key2 = (KEY *)((char *)&bkt->Buffer[ibkl->DataIndex[ibh->KeyEntryNum-1]] + sizeof *ikde) ;
	      if (memcmp(key,key2,key2->Bytes) == 0)			/* Does current key match that of last key? */
	       { bktnum = v4is_IndexSplit(aid,bkt,ibh->KeyEntryNum-1,NULL,-1) ;	/*  Yes - split at begin of range */
	         IBHINUSE ;
	         bkt = v4mm_BktPtr(bktnum,aid,IDX+UPD) ; ibh = (IBH *)bkt ;
	       } else
	       { IBHINUSE ;
	         bkt = v4is_MakeBkt(aid,V4_BktHdrType_Index) ; ibh = (IBH *)bkt ; bktnum = ibh->sbh.BktNum ;
		 ibh->ParentBktNum = acb->Lvl[parentlvl].BktNum ;
	       } ; IBHINUSE ;
	      acb->Lvl[parentlvl].BktNum = bktnum ;			/* Define this as the index for this level */
#ifdef FLUSH
	      v4mm_BktFlush(oldbktnum,aid) ;				/* Don't need this directory bucket any more */
#endif
	      memcpy(&skey,key,key->Bytes) ;
	      curlvl = acb->CurLvl ;
	      newparent = v4is_IndexStashAppendLink(aid,parentlvl-1,&skey) ;	/* Link new index to its parent */
	      if (curlvl != acb->CurLvl) parentlvl++ ;			/* If root split then adjust levels */
	      bkt = v4mm_BktPtr(bktnum,aid,IDX+UPD) ; ibh = (IBH *)bkt ;
	      ibh->ParentBktNum = newparent ;
	    } else
	    { bktnum = v4is_IndexSplit(aid,bkt,0,NULL,-1) ;		/* This is root bucket - copy all keys to new bucket */
	      IBHINUSE ;
	      bkt = v4mm_BktPtr(bktnum,aid,IDX+UPD) ; ibh = (IBH *)bkt ; ibh->ParentBktNum = 0 ;
	      for(lx=acb->CurLvl;lx>0;lx--)
	       { acb->Lvl[lx+1].BktNum = acb->Lvl[lx].BktNum ; } ;	/* Drop everything down a level */
	      acb->CurLvl++ ;
	      if (acb->RootInfo->NumLevels < acb->CurLvl+1)
	       { acb->RootInfo->NumLevels = (acb->CurLvl+1) ;		/* Save number of levels in index */
	       } ;
	      acb->Lvl[1].BktNum = bktnum ;				/* Make new index directly below root */
	      IBHINUSE ;
	      memcpy(&skey,key,key->Bytes) ;
	      bktnum = v4is_IndexStashAppendLink(aid,parentlvl+1,&skey) ;	/* Link this new index to parent */
	      bkt = v4mm_BktPtr(bktnum,aid,IDX+UPD) ; ibh = (IBH *)bkt ;
	      ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
	      key2 = (KEY *)((char *)&bkt->Buffer[ibkl->DataIndex[0]] + sizeof *ikde) ;	/* Get pointer to first key */
	      memcpy(&skey,key2,key2->Bytes) ;
	      v4is_IndexStashAppendLink(aid,0,&skey) ;		/* Now link child to the empty (new) root */
	      IBHINUSE ;
	      return(bktnum) ;
	    } ;
	 } ;
/*	At this point should be enough room in bkt for key link - append it */
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
	ibkl->DataIndex[ibh->KeyEntryNum++] = ix ;			/* Add index to this entry at bottom of list (next key) */
	IBHINUSE ;
	return(ibh->sbh.BktNum) ;					/* Return bucket where key landed */
}
/*	B U C K E T   U T I L I T I E S			*/

/*	v4is_MakeBkt - Allocates a new bucket to current area */
/*	Call: bktptr = v4is_MakeBkt ( aid , bkttype )
	  where bktptr is new bucket ID
		aid is current aread ID,
		bkttype is bucket type- V4_BktHdrType_xxx	*/

struct V4IS__Bkt *v4is_MakeBkt(aid,bkttype)
  int aid ;
  int bkttype ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__RootInfo *ri ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__Bkt *bkt ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__DataBktHdr *dbh ;
  struct V4IS__UnusedBktHdr *ubh ;
  int ax,bnum,lrx ; char ebuf[250] ;

	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return(NULL) ;			/* Could not find area? */
//	mmm = V4_GLOBAL_MMM_PTR ; ax = aid ;
	acb = v4mm_ACBPtr(aid) ;
	switch (bkttype)
	 { default: v4_error(V4E_MAKEINVBKTTYPE,0,"V4IS","MakeBkt","MAKEINVBKTTYPE","Invalid bucket type (%d) for area %d",bkttype,aid) ;
	   case V4_BktHdrType_Root:
		LOCKROOT ;
		bkt = (struct V4IS__Bkt *)v4mm_NewBktPtr(0,aid,UPD+MEM+IDX,V4MM_LockIdType_Index) ;	/* Make new bucket 0 (root) */
		ibh = (IBH *)bkt ;					/* Bucket header begins bucket */
		ibh->sbh.BktNum = 0 ; ibh->sbh.BktType = V4_BktHdrType_Root ; ibh->sbh.AvailBktSeq = 1 ;
		ibh->ParentBktNum = 0 ;
		ibh->KeyEntryTop = sizeof *ibh + sizeof *ri ;
		ibh->KeyEntryNum = 0 ;
		ibh->FreeBytes = acb->BktSize - (sizeof *ibh + sizeof *ri) ;
		ibh->DataTop = acb->BktSize ;
		ri = (struct V4IS__RootInfo *)((char *)bkt + sizeof *ibh) ;		/* Root info immediately follows */
		memset(ri,0,sizeof *ri) ;			/* Zap everything in root info */
		ri->BktSize = acb->BktSize ;
		ri->NextAvailBkt = 1 ;
		ri->NumLevels = 1 ;				/* Start out with single level index */
		acb->RootInfo = ri ;				/* Link new Root to ACB */
		RELROOT("MakeRoot") ; break ;
	   case V4_BktHdrType_Index:
		LOCKROOT ; ri = acb->RootInfo ;
Index_Again:
                if (ri->NextUnusedBkt >= ri->NextAvailBkt)
                 { sprintf(ebuf,"MakeBkt(Index): NextUnusedBkt (%d) >= NextAvailBkt(%d) - will clear",ri->NextUnusedBkt,ri->NextAvailBkt) ;
                   v4is_BugChk(aid,ebuf) ;
                   ri->NextUnusedBkt = 0 ;
                 } ;
		if (ri->NextUnusedBkt == 0)			/* Anything to use ? */
		 { bnum = ri->NextAvailBkt++ ;			/* No- Allocate bucket */
		   bkt = (struct V4IS__Bkt *)v4mm_NewBktPtr(bnum,aid,UPD,V4MM_LockIdType_Index) ;		/* Make new bucket */
		 } else
		 { bkt = v4mm_BktPtr(ri->NextUnusedBkt,aid,UPD+IDX) ; /* Get next unused bucket */
		   ubh = (struct V4IS__UnusedBktHdr *)bkt ; ri->NextUnusedBkt = ubh->NextUnusedBkt ; bnum = ubh->sbh.BktNum ;
		   if  (ubh->UBHFlag != UBHFlagVal) goto Index_Again ;
/* printf("Reusing bucket %d as index\n",bnum) ; */
		 } ;
		ibh = (IBH *)bkt ; ibh->sbh.InUse = 1 ;		/* Bucket header begins bucket */
		ibh->sbh.BktNum = bnum ; ibh->sbh.BktType = V4_BktHdrType_Index ; ibh->sbh.AvailBktSeq = 1 ;
		ibh->ParentBktNum = UNUSED ;			/* Don't know yet */
		ibh->KeyEntryTop = sizeof *ibh ;
		ibh->KeyEntryNum = 0 ;
		ibh->FreeBytes = ri->BktSize - (sizeof *ibh) ;
		ibh->DataTop = ri->BktSize ;
		RELROOT("MakeIndex") ; break ;
	   case V4_BktHdrType_Data:
		LOCKROOT ; ri = acb->RootInfo ;
Data_Again:
                if (ri->NextUnusedBkt >= ri->NextAvailBkt)
                 { sprintf(ebuf,"MakeBkt(Data): NextUnusedBkt (%d) >= NextAvailBkt(%d) - will clear",ri->NextUnusedBkt,ri->NextAvailBkt) ;
                   v4is_BugChk(aid,ebuf) ;
                   ri->NextUnusedBkt = 0 ;
                 } ;
		if (ri->NextUnusedBkt == 0)			/* Anything to use ? */
		 { bnum = ri->NextAvailBkt++ ;
		   bkt = (struct V4IS__Bkt *)v4mm_NewBktPtr(bnum,aid,UPD,V4MM_LockIdType_Data) ;		/* Make new bucket */
		 } else
		 { bkt = v4mm_BktPtr(ri->NextUnusedBkt,aid,UPD+DAT) ; /* Get next unused bucket */
		   ubh = (struct V4IS__UnusedBktHdr *)bkt ;
		   ri->NextUnusedBkt = ubh->NextUnusedBkt ; bnum = ubh->sbh.BktNum ;
		   if  (ubh->UBHFlag != UBHFlagVal) goto Data_Again ;
/* printf("Reusing bucket %d as data\n",bnum) ; */
		 } ;
		dbh = (DBH *)bkt ; dbh->sbh.InUse = 1 ;		/* Bucket header begins bucket */
		dbh->sbh.BktNum = bnum ; dbh->sbh.BktType = V4_BktHdrType_Data ; dbh->sbh.AvailBktSeq = 1 ;
		dbh->FreeBytes = ri->BktSize - sizeof *dbh ;
		dbh->FirstEntryIndex = 0 ;			/* Empty bucket */
		dbh->FreeEntryIndex = sizeof *dbh ;
/* 		v4is_DataBktFree(aid,acb,dbh,TRUE) ;*/		/* Record what is now available for use */
		RELROOT("MakeData") ; break ;
	 } ;
	v4mm_BktUpdated(aid,0) ;				/* Mark root bucket as updated */
	return(bkt) ;						/* Return pointer to new bucket */
}

/*	v4is_StashIndexData - Stashes Key & Data into Index Bucket	*/
/*	Call: logical = v4is_StashIndexData( aid , key , dataptr , databytes , cmpmode , multkeys, dataid )
	  where logical is TRUE if ok, FALSE if not (i.e. not enough room),
		aid is current area,
		key is pointer to primary key,
		dataptr/databytes is pointer & bytes in data to stash,
		cmpmode is data compression mode,
		multkeys is TRUE if this is primary with more to follow,
		dataid (if nonzero) is dataid to re-use			*/

LOGICAL v4is_StashIndexData(aid,key,dataptr,databytes,cmpmode,multkeys,dataid)
  int aid ;
  struct V4IS__Key *key ;
  void *dataptr ; int databytes ;
  int cmpmode, dataid ;
  LOGICAL multkeys ;
{ struct V4IS__AreaCB *acb ;
  struct V4IS__Bkt *bkt ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  union V4MM__DataIdMask dim ;
  int i,ix,notinrec,totalbytes ;

/*	Link up to everything */
	acb = v4mm_ACBPtr(aid) ;
	bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,aid,IDX+UPD) ;	/* Get current index bucket */
	ibh = (IBH *)bkt ;
	/* FIXME - add code to insert key in front of record */
	notinrec = ((char *)key != (char *)dataptr) ;			/* If key pointer not same as data then have to insert */
	if (notinrec)
	 { if (ibh->FreeBytes < ALIGN(sizeof *ikde + databytes + key->Bytes + 4))
	    { IBHINUSE ; return(FALSE) ; } ;
	   totalbytes = sizeof *ikde + databytes + key->Bytes ;
	 } else
	 { if (ibh->FreeBytes < ALIGN(sizeof *ikde + databytes + 4))
	    { IBHINUSE ; return(FALSE) ; } ;			/* No room - return FALSE */
	   totalbytes = sizeof *ikde + databytes ;
	 } ;
	ix = DATATOPVAL(ibh->DataTop) ;					/* ix = index to top of data section at bottom of bucket */
	ix -= ALIGN(totalbytes) ;					/* Decrement for bytes needed for header+data */
//	ibh->DataTop -= ALIGN(totalbytes) ;				/* Adjust header info */
	ibh->DataTop = ix ;
	ibh->FreeBytes -= ALIGN(totalbytes) + 2 ;			/* Adjust header info */
	ikde = (IKDE *)&bkt->Buffer[ix] ;				/* Set pointer to header */
	ikde->EntryType = V4IS_EntryType_DataLen ;			/* Making a immediate data entry */
	ikde->AuxVal = totalbytes ;					/* Save bucket number */
	ikde->HasMultKeys = multkeys ;
	dim.dataid = dataid ;
	if (dim.fld.BktNum != ibh->sbh.BktNum) dataid = 0 ;		/* If in new/different bucket then can't reuse dataid */
	if (dataid == 0) dataid = v4mm_MakeDataId(bkt) ;		/* Grab new dataid if necessary */
	acb->DataId = (ikde->DataId = dataid) ;				/* Save Data ID */
	ikde->CmpMode = cmpmode ;					/* Save compression mode */
	if (ikde->KeyNotInRec = notinrec)
	 { memcpy(((char *)ikde + sizeof *ikde),key,key->Bytes) ;	/* Copy key */
	   memcpy(((char *)ikde + sizeof *ikde + key->Bytes),dataptr,databytes) ;	/*  and then data */
	 } else memcpy(((char *)ikde + sizeof *ikde),dataptr,databytes) ;	/* Copy data buffer */
	ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;			/* Figure out where key list is */
/*	Figure out how to insert into DataIndex (maybe have to make a hole) */
	if (acb->CurPosKeyIndex >= ibh->KeyEntryNum)
	 { ibkl->DataIndex[ibh->KeyEntryNum++] = ix ;			/* Add index to this entry at bottom of list (next key) */
	 } else
	 { for(i=ibh->KeyEntryNum;i>acb->CurPosKeyIndex;i--)
	    { ibkl->DataIndex[i] = ibkl->DataIndex[i-1] ; } ;		/* Make hole for key */
	   ibkl->DataIndex[acb->CurPosKeyIndex] = ix ;			/* Insert index into hole */
	   ibh->KeyEntryNum++ ;						/* Increment number of keys */
	 } ;
	IBHINUSE ;
	acb->RootInfo->RecordCount++ ;					/* If added data then up record count for area */
	return(TRUE) ;
}

/*	R E F O R M A T T I N G    M O D U L E S		*/

/*	v4is_Reformat - Copies one area into another via Append	*/
/*	Call: numrec = v4is_Reformat( srcpcb , dstpcb , refres , verifyforeign , refopts )
	  where numrec is number records converted, UNUSED if error,
		srcpcb/dstpcb are source and destination parameter control blocks,
		refres if not NULL is pointer to results block,
		verifyforeign is TRUE if verifying a foreign-key only file,
		refopts are special reformat options (see V4IS_Reformat_xxx)		*/

int v4is_Reformat(srcpcb,dstpcb,refres,verifyforeign,refopts)
  struct V4IS__ParControlBlk *srcpcb ;
  struct V4IS__ParControlBlk *dstpcb ;
  struct V4IS__RefResults *refres ;
  LOGICAL verifyforeign ;
  int refopts ;
{ struct V4IS__Bkt *bkt,*dbkt ;
  struct V4IS__AreaCB *acb,*dacb ;
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__Key *primekey ;
  struct V4IS__Key okey ; int chkdups ;
  struct V4IS__DataBktHdr *dbh ;
  struct V4IS__DataBktEntryHdr *dbeh ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4DPI__XDictEntry *xde ;
  union V4IS__KeyPrefix *kp,oldkp ; int oldcnt ;
  struct V4IS__RefHash *rf = NULL ; int hx ;
  int posflag,obscnt,datacnt,reccnt,index,keyx,firstbo,k,ax ; char *data ; int datalen ;
  int errcnt = 0 ; int groupkeycnt = -1 ;
int hit=0 ;

#ifndef V4_BUILD_V4IS_MAINT
	printf("This build of V4 does not include v4is_Reformat functionality!\n") ; return ;
#endif
	if (refres != NULL) memset(refres,0,sizeof *refres) ;
	FINDAREAINPROCESS(ax,srcpcb->AreaId)		/* Call this to get mmm */
	if (ax == UNUSED) return(UNUSED) ;		/* Could not find area? */
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
	acb = v4mm_ACBPtr(srcpcb->AreaId) ; dacb = v4mm_ACBPtr(dstpcb->AreaId) ;
/*	Copy some stuff from the RootInfo */
#define RI(EL) dacb->RootInfo->EL = acb->RootInfo->EL ;
	RI(NextAvailDimDictNum) RI(NextAvailExtDictNum) RI(NextAvailIntDictNum) RI(NextAvailValueNum) RI(lastSegNum)
	RI(NextAvailUserNum)
	if (acb->RootInfo->FreeDataInfoByte != 0 && dacb->RootInfo->FreeDataInfoByte == 0)	/* Got free data info ? */
	 { bkt = v4mm_BktPtr(0,dstpcb->AreaId,IDX) ; ibh = (IBH *)bkt ;	/* Get root bucket */
	   dacb->FreeData = (struct V4IS__FreeDataInfo *)&bkt->Buffer[ibh->KeyEntryTop] ;
	   dacb->RootInfo->FreeDataInfoByte = ibh->KeyEntryTop ;
	   ibh->KeyEntryTop += sizeof *(dacb->FreeData) ; ibh->FreeBytes -= sizeof *(dacb->FreeData) ;
	   memset(dacb->FreeData,0,sizeof *(dacb->FreeData)) ;		/* Don't copy - reset and will set up during reformat */
	   IBHINUSE ;
	 } ;
	if (acb->RootInfo->AreaHInfoByte != 0)				/* Got V4 Area Info ? */
	 { bkt = v4mm_BktPtr(0,dstpcb->AreaId,IDX) ; ibh = (IBH *)bkt ;	/* Get root bucket */
	   dacb->ahi = (struct V4C__AreaHInfo *)&bkt->Buffer[ibh->KeyEntryTop] ; dacb->RootInfo->AreaHInfoByte = ibh->KeyEntryTop ;
	   ibh->KeyEntryTop += sizeof *(dacb->ahi) ; ibh->FreeBytes -= sizeof *(dacb->ahi) ;
	   *(dacb->ahi) = *(acb->ahi) ; IBHINUSE ;
	 } ;
	if (acb->KeyInfo != NULL)				/* Got Key info to copy ? */
	 { bkt = v4mm_BktPtr(0,dstpcb->AreaId,IDX) ; ibh = (IBH *)bkt ;
	   dacb->KeyInfo = (struct V4FFI__KeyInfo *)&bkt->Buffer[ibh->KeyEntryTop] ;
	   dacb->RootInfo->FFIKeyInfoByte = ibh->KeyEntryTop ;
	   ibh->KeyEntryTop += ALIGN(acb->KeyInfo->Bytes) ; ibh->FreeBytes -= ALIGN(acb->KeyInfo->Bytes) ;
	   memcpy(dacb->KeyInfo,acb->KeyInfo,acb->KeyInfo->Bytes) ; IBHINUSE ;
	 } ;
	if (acb->RootInfo->IndexOnlyInfoByte != 0)		/* Got Index-Only links? */
	 { bkt = v4mm_BktPtr(0,dstpcb->AreaId,IDX) ; ibh = (IBH *)bkt ;
	   dacb->io = (struct V4IS__IndexOnly *)&bkt->Buffer[ibh->KeyEntryTop] ;
	   dacb->RootInfo->IndexOnlyInfoByte = ibh->KeyEntryTop ;
	   ibh->KeyEntryTop += ALIGN(acb->io->Bytes) ; ibh->FreeBytes -= ALIGN(acb->io->Bytes) ;
	   memcpy(dacb->io,acb->io,acb->io->Bytes) ;
	   firstbo = dacb->io->Entry[0].BktOffset ;		/* Save first bucket offset */
	 } else { dacb->io = NULL ; firstbo = -1 ; } ;

	obscnt = 0 ; oldcnt = 0 ; oldkp.all = 0x80000000 ;
	for((datacnt=0,reccnt=0,posflag=V4IS_PosBOF);;(reccnt++,posflag=V4IS_PosNext))
	 { if ((refopts & V4IS_Reformat_Silent) != 0)
	    { if( setjmp(((struct V4C__ProcessInfo *)v_GetProcessInfo())->environment) != 0)
	       { if (refopts & V4IS_Reformat_NoContPrompt) return(UNUSED) ;	/* Just return error if this option set */
		 exit(EXITABORT) ;	/* VEH050807 - The code below seems to hang on UNIX when within "batch file" - screw it - just exit */
//	         v4lex_GetStdInput("  ... Enter \'C\' to continue  ?  ",reply,1) ;
//	         printf("  ... Enter \'C\' to continue  ?  ") ; gets(reply) ;
//		 if (reply[0] != 'c' && reply[0] != 'C') exit(EXITABORT) ;
	       } ;
	    } ;
	   if (! v4is_PositionRel(srcpcb->AreaId,posflag,-1,NULL)) break ;
/*	   if ((reccnt % 2500) == 0) printf("[%d => %d datarecs]\n",reccnt,datacnt) ; */
	   if (refres != NULL) { if (acb->CurLvl > refres->MaxLevels) refres->MaxLevels = acb->CurLvl ; } ;
	   bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,srcpcb->AreaId,IDX) ;
	   ibh = (IBH *)bkt ;		/* Get the bucket */
	   ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;	/* Pointer to B header & Key indexes */
	   ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[acb->CurPosKeyIndex]] ;
	   primekey = (KEY *)((char *)ikde + sizeof *ikde) ;	/* Pointer to primary key */
	   kp = (KP *)primekey ;
	   if (kp->all != oldkp.all)
	    { if (oldkp.all != 0x80000000)
	       { if (((refopts & V4IS_Reformat_Silent) == 0) && reccnt > 0)
		  printf("Key Group- Bytes:%d Mode:%d Type:%d AuxVal:%d  %d records => %d total\n",
			 oldkp.fld.Bytes,oldkp.fld.KeyMode,oldkp.fld.KeyType,oldkp.fld.AuxVal,reccnt-oldcnt,reccnt) ;
	         if (verifyforeign)
	          { if (groupkeycnt >= 0)
		     { if (groupkeycnt != reccnt-oldcnt)
		        { errcnt++ ;
		          printf("? Key count (%d) does not match key count (%d) of prior group\n",reccnt-oldcnt,groupkeycnt) ;
		        } ;
		     } else { groupkeycnt = reccnt-oldcnt ; } ;
	          } ;
	       } ;
	      if (kp->all < oldkp.all)
	       v4_error(V4E_KEYPREFIXSEQERR,0,"V4IS","Reformat","KEYPREFIXSEQERR","Key Prefix Sequence Error: %x s/b greater than %x",
			kp->all,oldkp.all) ;
	      oldkp.all = kp->all ; oldcnt = reccnt ;
	      switch (kp->fld.KeyType)
	       { default:	chkdups = -1 ; break ;
		 case V4IS_KeyType_FrgnKey1:
				chkdups = v4is_FFKeyDups(acb,kp->fld.AuxVal,0) ; break ;
	       } ;
	    } ;
	   switch (chkdups)
	    { default:	break ;			/* Don't check for dups */
	      case 0:				/* If dup keys not allowed then check for them */
		if (memcmp(&okey,primekey,primekey->Bytes) == 0)
		 { if (verifyforeign) { errcnt++ ; printf("? Key (%s) - duplicate detected\n",v4is_FormatKey(&okey)) ; } ; } ;
		memcpy(&okey,primekey,primekey->Bytes) ;
	    } ;
	   switch (ikde->EntryType)
	    { default: v4_error(V4E_INVIKDETYPE,0,"V4IS","Reformat","INVIKDETYPE","Unknown EntryType") ;
	      case V4IS_EntryType_DataLink:
		switch (primekey->KeyType)
		 { default: v4_error(V4E_BADKEYTYPE,0,"V4IS","Reformat","BADKEYTYPE","Unknown key type (%d)",primekey->KeyType) ;
		   case V4IS_KeyType_FrgnKey1:
//		   case V4IS_KeyType_FrgnFile:
		   case V4IS_KeyType_V4Segments:
		   case V4IS_KeyType_AggShell:
		   case V4IS_KeyType_Binding:
		   case V4IS_KeyType_V4:
			if (firstbo > 0 ? ikde->AuxVal >= firstbo : FALSE)	/* Is data in a linked area? */
			 { if (dstpcb->OpenMode != V4IS_PCB_OM_Nil)		/* Then maybe just append the key */
			    v4is_Append(dstpcb->AreaId,primekey,NULL,0,0,0,0,ikde->DataId) ;
			   break ;
			 } ;
/*			For all the above types- grab data in data bucket & rewrite in new file */
			dbkt = v4mm_BktPtr(ikde->AuxVal,srcpcb->AreaId,DAT) ; dbh = (DBH *)dbkt ;
			if (dbh->sbh.BktType != V4_BktHdrType_Data)
			 v4_error(V4E_BKTNOTDATA,0,"V4IS","Reformat","BKTNOTDATA","Bucket (%d) not type data (%d)",
			 		dbh->sbh.BktNum,dbh->sbh.BktType) ;
/*			Scan thru data bucket for wanted data entry */
			for(index=dbh->FirstEntryIndex;index<dbh->FreeEntryIndex;index+=ALIGN(dbeh->Bytes))
			 { dbeh = (struct V4IS__DataBktEntryHdr *)&dbkt->Buffer[index] ;
			   if (dbeh->Bytes == 0)
			    v4_error(V4E_DBEHLENZERO,0,"V4IS","Reformat","DBEHLENZERO","Data Entry Header cannot be 0 in Bkt (%d)",dbh->sbh.BktNum) ;
			   if (dbeh->DataId != ikde->DataId) { continue ; } else break ;
			 } ;
			if (index>=dbh->FreeEntryIndex)
			 { errcnt++ ;
			   printf("? Data record (%x) cannot be found for key %s\n",ikde->DataId,v4is_FormatKey(primekey)) ;
			   continue ;
			 } ;
			if (dstpcb->OpenMode != V4IS_PCB_OM_Nil && dbeh->FileRef != V4IS_FileRef_Obsolete)
			v4is_Append(dstpcb->AreaId,primekey,(char *)dbeh+sizeof *dbeh,dbeh->Bytes-sizeof *dbeh,
					V4IS_PCB_DataMode_Data,dbeh->CmpMode,ikde->HasMultKeys,0) ;
			if (ikde->HasMultKeys)
			 { if (rf == NULL)
			    { if (acb->RootInfo->RecordCount*1.5 < V4IS_RefHash_Max)	/* Figure out how much to allocate */
			       { k = V4IS_RefHash_Max ; }
			       else { k = v_CvtToPrime((int)((acb->RootInfo->RecordCount) * 1.5)) ;/* Base off of record count */
				      printf("[Increased HashMax from %d to %d]\n",V4IS_RefHash_Max,k) ;
				    } ;
			      rf = (struct V4IS__RefHash *)v4mm_AllocChunk(128+k*(sizeof *rf->entry),TRUE) ;
			      rf->HashMax = k ; rf->HashLimit = (rf->HashMax * 9) / 10 ;
			    } ;
			   if (rf->Count >= rf->HashLimit)
			    v4_error(V4E_REFHASHLIM,0,"V4IS","Reformat","REFHASHLIM","Exceeded limit (%d) on DataId hash table (record %d)",
					rf->HashLimit,reccnt) ;
			   hx = ((dbeh->DataId*dbeh->DataId) % rf->HashMax) ; if (hx < 0) hx = -hx ;
			   for(;;hx=(hx+1)%rf->HashMax)
			    { if (rf->entry[hx].odataid == 0) break ;
			      hit++ ;
			      if ((hit % 100000) == 0) printf("  Hash hits = %d\n",hit) ;
			      if (rf->entry[hx].odataid == dbeh->DataId)
			       { errcnt++ ;
				 printf("? DataId (%x) referenced more than once- %s\n",dbeh->DataId,v4is_FormatKey(primekey)) ;
				 break ;
			       } ;
			    } ;
			   rf->entry[hx].odataid = dbeh->DataId ;
			   if (dstpcb->OpenMode == V4IS_PCB_OM_Nil) { rf->entry[hx].ndataid = 1 ; } /* Track number of references */
			    else { rf->entry[hx].ndataid = (dbeh->FileRef == V4IS_FileRef_Obsolete ? -1 : dacb->DataId) ; } ;
			   rf->Count++ ;
			 } ;
			if (dbeh->FileRef == V4IS_FileRef_Obsolete) { obscnt++ ; } else { datacnt ++ ; } ;
			break ;
		   case V4IS_KeyType_FrgnKey2:	keyx = 2 ; goto tranid ;
		   case V4IS_KeyType_FrgnKey3:	keyx = 3 ; goto tranid ;
		   case V4IS_KeyType_FrgnKey4:	keyx = 4 ; goto tranid ;
		   case V4IS_KeyType_FrgnKey5:	keyx = 5 ; goto tranid ;
		   case V4IS_KeyType_FrgnKey6:	keyx = 6 ; goto tranid ;
		   case V4IS_KeyType_FrgnKey7:	keyx = 7 ; goto tranid ;
/*			For all of the above types- translate DataId & rewrite link only */
tranid:
			if (rf == NULL)
			 { errcnt++ ; printf("? Hit non-primary key without first encountering any primary keys\n") ; continue ; } ;
			if (firstbo > 0 ? ikde->AuxVal >= firstbo : FALSE)	/* Is data in a linked area? */
			 { if (dstpcb->OpenMode != V4IS_PCB_OM_Nil)		/* Then maybe just append the key */
			    v4is_Append(dstpcb->AreaId,primekey,NULL,0,0,0,0,ikde->DataId) ;
			   break ;
			 } ;
			hx = ((ikde->DataId*ikde->DataId) % rf->HashMax) ; if (hx < 0) hx = -hx ;
			for(;;hx=(hx+1)%rf->HashMax)
			 { if (rf->entry[hx].odataid == 0) break ;
			   if (rf->entry[hx].odataid == ikde->DataId) break ;
			   hit++ ;
			   if ((hit % 100000) == 0) printf("  Hash hits = %d\n",hit) ;
			 } ;
			if (rf->entry[hx].odataid != ikde->DataId)
			 { errcnt++ ; printf("? Could not locate old dataid (%x) on record %d\n",ikde->DataId,reccnt) ;
			 } ;
			dacb->DataId = rf->entry[hx].ndataid ;
			if (dstpcb->OpenMode != V4IS_PCB_OM_Nil)
			 { if (dacb->DataId != -1)		/* If -1 then original data flagged as obsolete! */
			    v4is_Append(dstpcb->AreaId,primekey,NULL,0,0,0,0,dacb->DataId) ; }
			 else { rf->entry[hx].ndataid ++ ;
				if (keyx != 0 ? rf->entry[hx].ndataid != keyx : FALSE)
				 { errcnt++ ; printf("? DataId (%x) reference count (%d) not equal to %d- %s\n",
							rf->entry[hx].odataid,rf->entry[hx].ndataid,keyx,v4is_FormatKey(primekey)) ;
				 } ;
			      } ;
			break ;
		 } ;
		IBHINUSE ;
		break ;
	      case V4IS_EntryType_DataLen:
		if (ikde->KeyNotInRec)
		 { data = (char *)primekey + primekey->Bytes ; datalen = ikde->AuxVal - (sizeof *ikde + primekey->Bytes) ; }
		 else { data = (char *)primekey ; datalen = ikde->AuxVal - sizeof *ikde ; } ;
		if ((refopts & V4IS_Reformat_XDict) != 0)		/* If option set then only copy External Dict Dimension entries */
		 { xde = (struct V4DPI__XDictEntry *)data ;
		   if (!(xde->DimXId == UNUSED || xde->DimXId == 1)) continue ;
		   if (xde->kp.fld.KeyType != V4IS_KeyType_V4) continue ;
		   if (!(xde->kp.fld.AuxVal == V4IS_SubType_IntXDict || xde->kp.fld.AuxVal == V4IS_SubType_RevXDict || xde->kp.fld.AuxVal == V4IS_SubType_XDictDimPoints)) continue ;
		 } ;

		if (dstpcb->OpenMode != V4IS_PCB_OM_Nil)
		 v4is_Append(dstpcb->AreaId,primekey,data,datalen,V4IS_PCB_DataMode_Index,ikde->CmpMode,ikde->HasMultKeys,0) ;
		if (ikde->HasMultKeys)
		 { if (rf == NULL)
		    { if (acb->RootInfo->RecordCount*1.5 < V4IS_RefHash_Max)	/* Figure out how much to allocate */
		       { k = V4IS_RefHash_Max ; }
		       else { k = v_CvtToPrime((int)((acb->RootInfo->RecordCount) * 1.5)) ;/* Base off of record count */
			      printf("[Increased HashMax from %d to %d]\n",V4IS_RefHash_Max,k) ;
			    } ;
		      rf = (struct V4IS__RefHash *)v4mm_AllocChunk(128+k*(sizeof *rf->entry),TRUE) ;
		      rf->HashMax = k ; rf->HashLimit = (rf->HashMax * 9) / 10 ;
		    } ;
		   if (rf->Count >= rf->HashLimit)
		    v4_error(V4E_REFHASHLIM,0,"V4IS","Reformat","REFHASHLIM","Exceeded limit (%d) on DataId hash table (record %d)",
				rf->HashLimit,reccnt) ;
		   hx = ((dbeh->DataId*dbeh->DataId) % rf->HashMax) ; if (hx < 0) hx = -hx ;
		   for(;;hx=(hx+1)%rf->HashMax) { if (rf->entry[hx].odataid == 0) break ; } ;
		   rf->entry[hx].odataid = dbeh->DataId ; rf->entry[hx].ndataid = dacb->DataId ; rf->Count++ ;
		 } ;
		datacnt ++ ; IBHINUSE ;
		break ;
	    } ; dacb->CurPosKeyIndex++ ;		/* Increment destination key position to keep at "EOF" */
	 } ;
	if (refres != NULL)
	 { refres->DataIdCnt = datacnt ; refres->KeyCnt = reccnt ; refres->ObsoleteCnt = obscnt ;
	   refres->BktReads = mmm->Areas[acb->AreaId].BktReads + mmm->Areas[dacb->AreaId].BktReads ;
	   refres->BktWrites = mmm->Areas[acb->AreaId].BktWrites + mmm->Areas[dacb->AreaId].BktWrites ;
	 } ;
	if ((refopts & V4IS_Reformat_Silent) == 0 && reccnt > 0)
	 printf("Key Group- Bytes:%d Mode:%d Type:%d AuxVal:%d  %d records => %d total\n",
		oldkp.fld.Bytes,oldkp.fld.KeyMode,oldkp.fld.KeyType,oldkp.fld.AuxVal,reccnt-oldcnt,reccnt) ;
	if (verifyforeign)
	 { if (groupkeycnt >= 0)
	    { if (groupkeycnt != reccnt-oldcnt)
	       { errcnt++ ;
		 printf("? Key count (%d) does not match key count (%d) of prior group\n",reccnt-oldcnt,groupkeycnt) ;
	       } ;
	    } else { groupkeycnt = reccnt-oldcnt ; } ;
	 } ;
	if (errcnt > 0) v4_error(V4E_ERRORS,0,"V4IS","Reformat","ERRORS","Detected %d errors reformating/verifying area (%s)",errcnt,UCretASC(srcpcb->UCFileName)) ;
	return(reccnt) ;				/* Return number of records copied */
}

/*	R E C O V E R Y   M O D U L E S				*/

/*	v4is_DumpArea - Dumps all data records in an area to a standard sequential file	*/
/*	Call: outcnt = v4is_DumpArea( ifp , ofp , buckets , bucketsize , maxrecs , silent , dumphdr , dumpdata )
	  where outcnt is number of data records output,
		ifp is FILE pointer to area to be dumped,
		ofp is FILE pointer to sequential file,
		buckets is number of buckets of area to be dumped (in event that RootInfo is destroyed),
		bucketsize is the bucket size of the area,
		maxrecs is max number of records to copy (0 for all of them),
		   (if listing == 3 then maxrecs is megabytes, if unused greater-than then generate error),
		silent is TRUE to run silent, FALSE for messages & error recovery,
		dumphdr is TRUE to dump out root header info,
		dumpdata is TRUE to dump out all data records,
		listing = 1 for index listing, 2 for bucket utilization, 3 for summary utilization		*/

int v4is_DumpArea(ifp,ofp,buckets,bucketsize,maxrecs,silent,dumphdr,dumpdata,listing)
  FILE *ifp ;
  FILE *ofp ;
  int buckets,bucketsize,maxrecs,listing ;
  LOGICAL silent,dumphdr,dumpdata ;
{ struct V4IS__RootInfo *ri ;
  struct V4C__AreaHInfo *ahi ;
  struct V4FFI__KeyInfo *ki ;
  struct V4FFI__KeyInfoDtl *kid ;
  struct V4IS__Bkt *bkt ;
  struct V4__BktHdr *sbh ;
  struct V4IS__SeqDumpHdr sdh ;
  struct V4IS__SeqDumpRI sdri ;
  struct V4IS__SeqDumpAreaHI sdahi ;
  struct V4IS__SeqDumpFFIK sdfk ;
  struct V4IS__SeqDumpFFIKDetail *sdfkd ;
  struct V4IS__Key *key ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4IS__DataBktHdr *dbh ;
  struct V4IS__DataBktEntryHdr *dbeh ;
  union V4MM__DataIdMask dim ;
  int obscnt,outcnt,index,kx,kc,bktnum,megabytes,wasted,bktsize ; char *data ; double pc ; int ipc ;
  int unusedbkts=0,indexdat=0,datadat=0,indexcnt=0,datacnt=0,keybytes=0,keycnt=0,indexbkt=0,databkt=0,reccnt ;

#ifndef V4_BUILD_V4IS_MAINT
	printf("This build of V4 does not include v4is_DumpArea functionality!\n") ; return ;
#endif
	bkt = (struct V4IS__Bkt *)v4mm_AllocChunk(bucketsize,FALSE) ;	/* Allocate a bucket */
	if (listing == 3) { megabytes = maxrecs ; maxrecs = 0 ; } ;
	maxrecs = (maxrecs <= 0 ? V4LIM_BiggestPositiveInt : maxrecs) ;
	obscnt = 0 ;
	for(bktnum=0,outcnt=0;bktnum<buckets && datacnt<maxrecs;bktnum++)	/* ZipZop thru all buckets */
	 { if (silent >= 0)
	    { if( setjmp(((struct V4C__ProcessInfo *)v_GetProcessInfo())->environment) != 0)
	       { 
		 exit(EXITABORT) ;	/* VEH050807 - The code below seems to hang on UNIX when within "batch file" - screw it - just exit */
//	         if (v_IsBatchJob()) exit(EXITABORT) ;			/* If running batch/command file then don't try to prompt */
//	         v4lex_GetStdInput("  ... Enter \'C\' to continue  ?  ",reply,1) ;
//	         printf("  ... Enter \'C\' to continue  ?  ") ; gets(reply) ;
//		 if (reply[0] != 'c' && reply[0] != 'C') exit(EXITABORT) ;
	       } ;
	    } ;
	   if (fseek(ifp,(FSEEKINT)((FSEEKINT)bktnum*(FSEEKINT)bucketsize),SEEK_SET) != 0)
	    { printf("fseek error (%s) on bucket %d, ...continuing\n",v_OSErrString(errno),bktnum) ; continue ; } ;
	   if (fread(bkt,bucketsize,1,ifp) <= 0)
	    { printf("fread error (%s) on bucket %d, ...continuing\n",v_OSErrString(errno),bktnum) ; continue ; } ;
	   sbh = (BH *)bkt ;
	   switch (sbh->BktType)
	    { default: printf("  Unknown Bucket Type-7 (%d)\n",sbh->BktType) ; break ;
	      case V4_BktHdrType_Unused:
		unusedbkts++ ; continue ;
	      case V4_BktHdrType_Root:
		ri = (struct V4IS__RootInfo *)((char *)&bkt->Buffer + sizeof *ibh) ;	 /* Capture RootInfo Stuff */
#define C(FIELD) sdri.FIELD = ri->FIELD ;
		C(BktSize) C(Version) C(NextAvailDimDictNum) C(NextAvailExtDictNum) C(NextAvailIntDictNum)
		C(NextAvailValueNum) C(MinCmpBytes) C(MaxRecordLen) C(DataMode) C(FillPercent) C(AllocationIncrement)
		C(RecordCount) C(GlobalBufCount) C(LockMax) C(NextAvailUserNum) C(lastSegNum)
		bktsize = ri->BktSize ;
		sdh.CmpMode = 0 ; sdh.SeqDumpType = V4IS_SeqDumpType_RootInfo ;
		sdh.Placement = V4IS_SeqDumpPlace_Index ; sdh.Bytes = sizeof sdri ;
		if (dumphdr)
		 { if (fwrite(&sdh,sizeof sdh,1,ofp) <= 0)
		    { printf("fwrite error (%s) in bucket %d kx=%d, ...continuing\n",v_OSErrString(errno),bktnum,kx) ; continue ; } ;
		   if (fwrite((char *)&sdri,sdh.Bytes,1,ofp) <= 0)
		    { printf("fwrite error (%s) in root info %d kx=%d, ...continuing\n",v_OSErrString(errno),bktnum,kx) ; } ;
		 } ;
		if (ri->AreaHInfoByte != 0 && dumphdr)
		 { ahi = (struct V4C__AreaHInfo *)&bkt->Buffer[ri->AreaHInfoByte] ;
		   sdh.CmpMode = 0 ; sdh.SeqDumpType = V4IS_SeqDumpType_AreaHI ;
		   sdh.Placement = V4IS_SeqDumpPlace_Index ; sdh.Bytes = sizeof sdahi ;
#define CA(FIELD) sdahi.FIELD = ahi->FIELD ;
		   CA(RelHNum) CA(ExtDictUpd) CA(IntDictUpd)
		   CA(BindingsUpd)
		   if (fwrite(&sdh,sizeof sdh,1,ofp) <= 0)
		    { printf("fwrite error (%s) in bucket %d kx=%d, ...continuing\n",v_OSErrString(errno),bktnum,kx) ; continue ; } ;
		   if (fwrite((char *)&sdahi,sdh.Bytes,1,ofp) <= 0)
		    { printf("fwrite error (%s) in AreaHI info %d kx=%d, ...continuing\n",v_OSErrString(errno),bktnum,kx) ; } ;
		 } ;
		if (ri->FFIKeyInfoByte != 0 && dumphdr)
		 { ki = (struct V4FFI__KeyInfo *)&bkt->Buffer[ri->FFIKeyInfoByte] ;
		   sdfk.Bytes = 0 ; sdfk.Count = 0 ;
		   for((kid=(KID *)&ki->Buffer,kx=0);kx<ki->Count;(kx++,kid=(KID *)((char *)kid+kid->Bytes)))
		    { sdfkd = (struct V4IS__SeqDumpFFIKDetail *)&sdfk.Buffer[sdfk.Bytes] ; sdfk.Count++ ;
		      sdfkd->FileRef = kid->FileRef ; sdfkd->KeyCount = kid->KeyCount ;
		      for(kc=0;kc<kid->KeyCount;kc++)
		       { sdfkd->Key[kc].KeyPrefix = kid->Key[kc].KeyPrefix ; sdfkd->Key[kc].Offset = kid->Key[kc].Offset ;
			 sdfkd->Key[kc].DupsOK = kid->Key[kc].DupsOK ; sdfkd->Key[kc].UpdateOK = kid->Key[kc].UpdateOK ;
		       } ;
		      sdfkd->Bytes = (char *)&sdfkd->Key[kc] - (char *)sdfkd ;
		      sdfk.Bytes += sdfkd->Bytes ;
		    } ;
		   sdfk.Bytes = (char *)&sdfk.Buffer[sdfk.Bytes] - (char *)&sdfk ;
		   sdh.CmpMode = 0 ; sdh.SeqDumpType = V4IS_SeqDumpType_FFIKeys ;
		   sdh.Placement = V4IS_SeqDumpPlace_Index ; sdh.Bytes = sdfk.Bytes ;
		   if (fwrite(&sdh,sizeof sdh,1,ofp) <= 0)
		    { printf("fwrite error (%s) in bucket %d kx=%d, ...continuing\n",v_OSErrString(errno),bktnum,kx) ; continue ; } ;
		   if (fwrite((char *)&sdfk,sdh.Bytes,1,ofp) <= 0)
		    { printf("fwrite error (%s) in FFIKeys info %d kx=%d, ...continuing\n",v_OSErrString(errno),bktnum,kx) ; } ;
		 } ;
		if (!dumpdata && listing == 0) goto dump_done ;
	      case V4_BktHdrType_Index:
		ibh = (IBH *)bkt ; ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
		switch (listing)
		 { case 1:
			printf("Bkt %d - Index - %d Keys  - Parent %d\n",ibh->sbh.BktNum,ibh->KeyEntryNum,ibh->ParentBktNum) ;
			break ;
		   case 2:	/* Detail listing */
			printf("Index %5d Free %5d Keys %4d\n",ibh->sbh.BktNum,ibh->FreeBytes,ibh->KeyEntryNum) ;
		   case 3:	/* Summary only listing */
			keycnt += ibh->KeyEntryNum ; indexbkt++ ; keybytes += (bucketsize-ibh->FreeBytes) ;
			break ;
		 } ;
		for(kx=0;kx<ibh->KeyEntryNum;kx++)
		 { ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[kx]] ;
		   switch (ikde->EntryType)
		    { default: printf("    Key %d - UNKNOWN ENTRY TYPE (%d)\n",kx,ikde->EntryType) ; break ;
		      case V4IS_EntryType_IndexLink:
			if (listing == 1) printf("  %d->%d,",kx,ikde->AuxVal) ;
			break ;
		      case V4IS_EntryType_DataLink:
			break ;
		      case V4IS_EntryType_DataLen:
			if (!dumpdata) break ;
			sdh.CmpMode = ikde->CmpMode ; sdh.SeqDumpType = V4IS_SeqDumpType_DataRec ;
			sdh.Placement = V4IS_SeqDumpPlace_Index ; sdh.Bytes = ikde->AuxVal - sizeof *ikde ;
			key = (KEY *)((char *)ikde+ sizeof *ikde) ; data = (char *)key ;
			if (ikde->KeyNotInRec)
			 { sdh.Bytes -= key->Bytes ; sdh.FileRef = key->AuxVal ; data = (char *)key + key->Bytes ;
			 } else { sdh.FileRef = 0 ; } ;
			if (sdh.FileRef == V4IS_FileRef_Obsolete) { obscnt++ ; break ; } ;
			if (fwrite(&sdh,sizeof sdh,1,ofp) <= 0)
			 { printf("fwrite error (%s) in bucket %d kx=%d, ...continuing\n",v_OSErrString(errno),bktnum,kx) ; continue ; } ;
			if (fwrite(data,sdh.Bytes,1,ofp) <= 0)
			 { printf("fwrite error (%s) in index bucket %d kx=%d, ...continuing\n",v_OSErrString(errno),bktnum,kx) ; continue ; } ;
			indexdat += sdh.Bytes ; indexcnt += 1 ;
			outcnt ++ ; break ;
		    } ;
		 } ;
		if (FALSE) printf("\n") ;
		break ;
	      case V4_BktHdrType_Data:
		dbh = (DBH *)bkt ;
		for(reccnt=0,index=dbh->FirstEntryIndex;index<dbh->FreeEntryIndex;index+=ALIGN(dbeh->Bytes))
		 { dbeh = (struct V4IS__DataBktEntryHdr *)&bkt->Buffer[index] ;
		   if (dbeh->FileRef == V4IS_FileRef_Obsolete) { obscnt++ ; continue ; } ;	/* Don't bother if obsolete */
		   if (dbeh->Bytes <= sizeof *dbeh || dbeh->Bytes > bktsize)
		    { printf("? Bucket (%d) dbeh->Bytes=%d, min is %d, max is %d, ...continuing\n",bktnum,(int)dbeh->Bytes,(int)sizeof *dbeh,bktsize) ;
		      continue ;
		    } ;
		   dim.dataid = dbeh->DataId ;
		   if (dim.fld.BktNum != bktnum)
		    { printf("? Bucket (%d) dbeh->DataId=%x links to bucket %d, ...continuing\n",bktnum,dbeh->DataId,dim.fld.BktNum) ;
		      continue ;
		    } ;
		   sdh.CmpMode = dbeh->CmpMode ; sdh.Bytes = dbeh->Bytes - sizeof *dbeh ;
		   sdh.SeqDumpType = V4IS_SeqDumpType_DataRec ; sdh.Placement = V4IS_SeqDumpPlace_Data ;
		   sdh.FileRef = dbeh->FileRef ;
		   datadat += sdh.Bytes ; datacnt += 1 ;
		   reccnt++ ; if (!dumpdata) continue ;
		   if (fwrite(&sdh,sizeof sdh,1,ofp) <= 0)
		    { printf("fwrite error (%s) in data bucket %d index=%d, ...continuing\n",v_OSErrString(errno),bktnum,index) ; continue ; } ;
		   if (fwrite((char *)dbeh+sizeof *dbeh,sdh.Bytes,1,ofp) <= 0)
		    { printf("fwrite error (%s) in data bucket %d index=%d, ...continuing\n",v_OSErrString(errno),bktnum,index) ; continue ; } ;
		   outcnt++ ;
		 } ;
		switch (listing)
		 {
		   case 2:
			printf("Data  %5d Free %5d Records %4d\n",dbh->sbh.BktNum,dbh->FreeBytes,reccnt) ;
		   case 3:	/* Summary only listing */
			databkt++ ; break ;
		 } ;
		break ;
	    } ;
	 } ;
dump_done:
	sdh.CmpMode = 0 ; sdh.Bytes = 0 ; sdh.SeqDumpType = V4IS_SeqDumpType_EOF ; sdh.Placement = 0 ;
	if (dumpdata || dumphdr)
	 { if (fwrite(&sdh,sizeof sdh,1,ofp) <= 0)
	    { printf("fwrite error (%s) in EOF marker bucket %d index=%d, ...continuing\n",v_OSErrString(errno),bktnum,index) ; } ;
	 } ;
	switch (listing)
	 { default:
		if (silent != TRUE) printf("Total index data %d in %d records\n",indexdat,indexcnt) ;
		if (silent != TRUE) printf("Total data data %d in %d records\n",datadat,datacnt) ;
		break ;
	   case 2: case 3:
		if (datadat == 0 || databkt == 0) { pc = 0.0 ; ipc = 0 ; }
		 else { pc = (double)datadat * 100.0 ; pc = pc / ((double)(databkt*bucketsize)) ; ipc = DtoI(pc) ; } ;
		wasted = (databkt * bucketsize) - datadat ;
		if (unusedbkts > 0)
		 { printf("  Recap Unused %d buckets, %d bytes\n",unusedbkts,unusedbkts*bktsize) ;
		   wasted += (unusedbkts*bktsize) ;
		 } ;
		printf("  Recap   Data %5d buckets, %7d records, %9d bytes, %5d bytes/record, %2d %% bucket utilization\n",
			databkt,datacnt,datadat,(datacnt == 0 ? 0 : datadat/datacnt),ipc) ;
		if (keybytes == 0 || indexbkt == 0) { pc = 0.0 ; ipc = 0 ; }
		 else { pc = (double)keybytes * 100.0 ; pc = pc / ((double)(indexbkt*bucketsize)) ; ipc = DtoI(pc) ; } ;
		wasted += ( (indexbkt*bucketsize) - keybytes ) ;
		printf("  Recap  Index %5d buckets, %7d keys, %12d bytes, %5d bytes/record, %2d %% bucket utilization\n",
			indexbkt,keycnt,keybytes,(keycnt == 0 ? 0 : keybytes/keycnt),ipc) ;
		if (megabytes > 0 && wasted > (megabytes*1000000))
		 v4_error(V4E_MAXWASTE,0,"V4ISU","DumpData","MAXWASTE","Wasted space (%d) exceeds allowed threshold (%d mb)",wasted,megabytes) ;
	 } ;
	return(outcnt) ;
}

/*	v4is_RestoreArea - Restores V4 Area from standard sequential file	*/
/*	Call: count = v4is_RestoreArea( sfl , ofile , newmode , silent , bucketsize, fillpercent , fastrebuild , recordsize )
	  where count is number of records restored,
		sfl is pointer to file name list (struct V4IS_SeqFileList),
		ofile is file name string of new/existing file to be updated,
		newmode is TRUE to create a new file, FALSE to add/insert records to existing area,
		fileref is -1 to use filerefs in dump, 0 to use in existing area, +n to use n,
		silent is TRUE to run silent, FALSE for messages & error recovery,
		bucketsize is bucket size (bytes) of new file (0 for default),
		fillpercent if nonzero is index/data fill percentage to use,
		fastrebuild is TRUE to use vsort for fast file rebuild,
		recordsize if nonzero specifies max record size of area		*/

int v4is_RestoreArea(sfl,ofile,newmode,fileref,silent,bucketsize,fillpercent,fastrebuild,recordsize)
  struct V4IS__SeqFileList *sfl ;
  UCCHAR *ofile ;
  LOGICAL newmode,silent,fastrebuild ;
  int fileref,bucketsize,fillpercent,recordsize ;
{ struct V4IS__RootInfo *ri ;
  struct V4IS__AreaCB *acb ;
  static struct V4IS__ParControlBlk pcb ;
  struct V4IS__OptControlBlk ocb ;
  struct V4IS__IndexOnly io ;
  struct V4FFI__KeyInfo ki ;
  struct V4FFI__KeyInfoDtl *kid ;
  struct V4IS__Bkt *bkt ;
  struct V4__BktHdr *sbh ;
  struct V4IS__SeqDumpHdr sdh ;
  struct V4IS__SeqDumpRI sdri ;
  struct V4IS__SeqDumpAreaHI sdahi ;
  struct V4IS__SeqDumpFFIK sdfk ;
  struct V4IS__SeqDumpFFIKDetail *sdfkd ;
  struct V4IS__Key *key ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4IS__DataBktHdr *dbh ;
  struct V4IS__DataBktEntryHdr *dbeh ;
  union V4MM__DataIdMask dim ;
#ifdef WINNT
   HANDLE kfd ;
#else
   int kfd ;
#endif
#ifdef VMSOS
  struct vms__dsc strdsc ;
  int res,resx ;
#endif
  FILE *ifp ;
  UCCHAR tnam1[100],tnam2[100] ;
  int bytes,maxkeyBytes,maxkeynum,keycnt,sortrecs ; UCCHAR sbuf[200] ; char *bp1 ; UCCHAR tmpstr[200] ;
  struct {
    struct V4IS__DataIdKey dkey ;
    char AlphaValCont[300] ;
   } vkey ;
  char *data,*cdata ;
  int obscnt,outcnt,index,kx,kc,sflx,sflinit,dosetup,needbkt,dataid ;

#ifndef V4_BUILD_V4IS_MAINT
	printf("This build of V4 does not include v4is_RestoreArea functionality!\n") ; return ;
#endif
	data = NULL ; cdata = NULL ; bkt = NULL ; ri = NULL ;
	obscnt = 0 ; outcnt = 0 ; dosetup = TRUE ;
	memset(&pcb,0,sizeof pcb) ; memset(&ki,0,sizeof ki) ; memset(&sdahi,0,sizeof sdahi) ;
	if (fastrebuild)
	 { maxkeyBytes = 0 ;			/* Have to scan to determine largest key */
	   maxkeynum = 0 ;			/* Have to track largest number of keys */
	   sortrecs = 0 ;			/* Number of records to sort */
	   v_MakeOpenTmpFile(UClit("v4rst1"),tnam1,UCsizeof(tnam1),NULL,NULL) ;
	   v_MakeOpenTmpFile(UClit("v4rst2"),tnam2,UCsizeof(tnam2),NULL,NULL) ;
//	   TEMPFILE(tnam1,"v4is") ;		/* Create a temp file name */
//	   TEMPFILE(tnam2,"v4is") ;		/*   and another one */
#ifdef WINNT
	   kfd = UCCreateFile(tnam1,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL) ;
	   if (kfd == INVALID_HANDLE_VALUE)
	    { printf("? Error (%s) creating temp sort file (%s)\n",v_OSErrString(GetLastError()),UCretASC(tnam1)) ; exit(EXITABORT) ; } ;
#else
	   kfd = creat(UCretASC(tnam1),0660) ;		/* Create temp file for keys */
	   if (kfd == -1)
	    { printf("? Error (%s) creating temp sort file (%s)\n",v_OSErrString(errno),UCretASC(tnam1)) ; exit(EXITABORT) ; } ;
#endif
	 } ;
	io.count = 0 ;					/* Assume no index-only links */
	for(sflx=0;sflx<sfl->Count;sflx++)
	 { if (sfl->File[sflx].Type != V4IS_SeqFileType_Index) continue ;
	   UCstrcpy(io.Entry[io.count].AreaName,sfl->File[sflx].Name) ;
	   if (io.count == 0) { io.Entry[io.count].BktOffset = sfl->BktOffset ; } ;
	   io.Entry[io.count+1].BktOffset = io.Entry[io.count].BktOffset + sfl->File[sflx].BktCount ;
	   io.count++ ;
	 } ;
	for(ifp=NULL,sflinit=TRUE,sflx=0;sflx<sfl->Count;)
	 { if (silent != -1)
	    { if( setjmp(((struct V4C__ProcessInfo *)v_GetProcessInfo())->environment) != 0)
	       { 
		 exit(EXITABORT) ;	/* VEH050807 - The code below seems to hang on UNIX when within "batch file" - screw it - just exit */
//	         if (v_IsBatchJob()) exit(EXITABORT) ;			/* If running batch/command file then don't try to prompt */
//	         v4lex_GetStdInput("  ... Enter \'C\' to continue  ?  ",reply,1) ;
//	         printf("  ... Enter \'C\' to continue  ?  ") ; gets(reply) ;
//		 if (reply[0] != 'c' && reply[0] != 'C') exit(EXITABORT) ;
	       } ;
	    } ;
top_of_loop:
	   if (sflinit)
	    { if (ifp != NULL) fclose(ifp) ;
	      if (bkt != NULL) { v4mm_FreeChunk(bkt) ; bkt = NULL ; } ;
	      if (sflx >= sfl->Count) break ;			/* All done ? */
	      ifp = fopen(UCretASC(v_UCLogicalDecoder(sfl->File[sflx].Name,VLOGDECODE_Exists,0,tmpstr)),"rb") ;
//	      ifp = fopen(v3_logical_decoder(UCretASC(sfl->File[sflx].Name),TRUE),"rb") ;
	      if (ifp == NULL)
	       { printf("? Error (%s) accessing input file %s\n",v_OSErrString(errno),UCretASC(sfl->File[sflx].Name)) ; exit(EXITABORT) ; } ;
	      sflinit = FALSE ; needbkt = TRUE ;
	    } ;
	   switch (sfl->File[sflx].Type)
	    { default:
		printf("? Unknown file type (%d) for file (%s)\n",sfl->File[sflx].Type,UCretASC(sfl->File[sflx].Name)) ; exit(EXITABORT) ;
	      case V4IS_SeqFileType_Index:
	      case V4IS_SeqFileType_V4IS:
		if (needbkt)
		 { if (bkt == NULL) { bkt = (struct V4IS__Bkt *)v4mm_AllocChunk(sfl->File[sflx].BktSize,FALSE) ; } ;
		   if (data == NULL) { data = (char *)v4mm_AllocChunk(sfl->File[sflx].BktSize,FALSE) ; } ;
		   if (fread(bkt,sfl->File[sflx].BktSize,1,ifp) <= 0)
		    { sflx++ ; sflinit = TRUE ; goto top_of_loop ; } ;
		   sbh = (BH *)bkt ; kx = 0 ; index = -1 ;
		   needbkt = FALSE ;
		 } ;
		switch (sbh->BktType)
		 { default:
			printf("  Unknown Bucket Type-8 (%d)\n",sbh->BktType) ;
			needbkt = TRUE ;		/* Want to skip over bad(?) bucket */
			continue ;
		   case V4_BktHdrType_Unused:
			needbkt = TRUE ; continue ;
		   case V4_BktHdrType_Root:
		   case V4_BktHdrType_Index:
			ibh = (IBH *)bkt ; ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
			if (kx >= ibh->KeyEntryNum)  { needbkt = TRUE ; goto top_of_loop ; } ;
			ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[kx]] ;
			switch (ikde->EntryType)
			 { default:
				kx++ ; printf("    Key %d - UNKNOWN ENTRY TYPE (%d)\n",kx,ikde->EntryType) ; goto top_of_loop ;
			   case V4IS_EntryType_IndexLink:
			   case V4IS_EntryType_DataLink:
				kx++ ; goto top_of_loop ;
			   case V4IS_EntryType_DataLen:
				sdh.CmpMode = ikde->CmpMode ; sdh.SeqDumpType = V4IS_SeqDumpType_DataRec ;
				sdh.Placement = V4IS_SeqDumpPlace_Index ; sdh.Bytes = ikde->AuxVal - sizeof *ikde ;
				key = (KEY *)((char *)ikde+ sizeof *ikde) ; bp1 = (char *)key ;
				if (ikde->KeyNotInRec)
				 { sdh.Bytes -= key->Bytes ; sdh.FileRef = key->AuxVal ; bp1 = (char *)key + key->Bytes ; }
				 else { sdh.FileRef = 0 ; } ;
				memcpy(data,bp1,sdh.Bytes) ;
				dataid = ikde->DataId ;
				kx++ ; break ;
		   	 } ;
			break ;
		   case V4_BktHdrType_Data:
			dbh = (DBH *)bkt ;
			if (index == -1) { index = dbh->FirstEntryIndex ; } else { index += ALIGN(dbeh->Bytes) ; } ;
			if (index >= dbh->FreeEntryIndex) { needbkt = TRUE ; goto top_of_loop ; } ;
			dbeh = (struct V4IS__DataBktEntryHdr *)&bkt->Buffer[index] ;
			sdh.CmpMode = dbeh->CmpMode ; sdh.Bytes = dbeh->Bytes - sizeof *dbeh ;
			sdh.SeqDumpType = V4IS_SeqDumpType_DataRec ; sdh.Placement = V4IS_SeqDumpPlace_Data ;
			sdh.FileRef = dbeh->FileRef ;
			if (ri == NULL) { printf("? No output file has been created\n") ; return(0) ; } ;
			if (sdh.Bytes > ri->BktSize)
			 { printf("? DataId (%d) raw length (%d) exceeds bucket size (%d), ignoring...\n",
				  dbeh->DataId,sdh.Bytes,ri->BktSize) ;
			   needbkt = TRUE ; goto top_of_loop ;
			 } ;
			memcpy(data,(char *)dbeh+sizeof *dbeh,sdh.Bytes) ;
			dataid = dbeh->DataId ;
			break ;
		 } ;
		break ;
	      case V4IS_SeqFileType_Seq:
		if (fread(&sdh,sizeof sdh,1,ifp) <= 0)
		 { printf("fread error (%s) on sequential input file, ... quitting!\n",v_OSErrString(errno)) ; sflx=9999 ; break ; } ;
		dataid = 0 ;
		break ;
	    } ;
	   if (sdh.FileRef == V4IS_FileRef_Obsolete) { obscnt++ ; continue ; } ;
	   switch (sdh.SeqDumpType)
	    { default:
		printf("Invalid header type on sequential file... quitting\n") ; sflx=9999 ; break ;
	      case V4IS_SeqDumpType_RootInfo:
		if (sdh.Bytes != sizeof sdri)
		 { printf("Size in RootInfo header does not match that of program... quitting\n") ; sflx=9999 ; break ; } ;
		if (fread(&sdri,sizeof sdri,1,ifp) <= 0)
		 { printf("fread error (%s) on RootInfo detail, ... quitting!\n",v_OSErrString(errno)) ; sflx=9999 ; break ; } ;
		break ;
	      case V4IS_SeqDumpType_EOF:			/* Fall thru (in case no data- want to create file!) */
	      case V4IS_SeqDumpType_DataRec:
		if (dosetup)
		 { dosetup = FALSE ;				/* Only create/open output file once */
		   strcpy(pcb.V3name,"restore") ; UCstrcpy(pcb.UCFileName,ofile) ;
		   if (newmode)
		    { pcb.BktSize = sdri.BktSize ; pcb.MinCmpBytes = sdri.MinCmpBytes ; pcb.MaxRecordLen = sdri.MaxRecordLen ;
		      pcb.DfltDataMode = sdri.DataMode ; pcb.OpenMode = V4IS_PCB_OM_New ;
		      if (bucketsize > 0) pcb.BktSize = bucketsize ;
		      if (recordsize > 0) pcb.MaxRecordLen = recordsize ;
		      if (ki.Count > 0) { pcb.KeyInfo = &ki ; } ;
		      if (sdahi.RelHNum != 0) pcb.RelHNum = sdahi.RelHNum ;
		    } else
		    { pcb.OpenMode = V4IS_PCB_OM_Update ;
		    } ;
		   pcb.AccessMode = -1 ;
		   pcb.DfltPutMode = (fastrebuild ? V4IS_PCB_GP_Append : V4IS_PCB_GP_Insert) ;
		   if (io.count > 0)				/* Have extra info for open? */
		    { io.Bytes = (char *)&io.Entry[io.count] - (char *)&io ;
		      ocb.Nextocb = NULL ; ocb.ocbType = V4IS_OCBType_IndexOnly ; ocb.ocbPtr = (char *)&io ;
		    } ;
		   v4is_Open(&pcb,(io.count > 0 ? &ocb : NULL),NULL) ;
		   if (pcb.AreaId < 0) { sflx=9999 ; break ; } ;
		   if (!newmode && fileref == 0) fileref = pcb.FileRef ;	/* If fileref == 0 then take from file */
		   acb = v4mm_ACBPtr(pcb.AreaId) ; ri = acb->RootInfo ;
		   ri->FillPercent = sdri.FillPercent ; ri->AllocationIncrement = sdri.AllocationIncrement ;
		   ri->GlobalBufCount = sdri.GlobalBufCount ; ri->LockMax = sdri.LockMax ;
		   ri->NextAvailUserNum = sdri.NextAvailUserNum ;
		   if (fillpercent != 0) ri->FillPercent = fillpercent ;
		   if (data == NULL) data = (char *)v4mm_AllocChunk(ri->BktSize,FALSE) ;
		 } ;
		if (sdh.SeqDumpType == V4IS_SeqDumpType_EOF) { sflx++ ; sflinit=TRUE ; break ; } ;
		if (sdh.Bytes > ri->BktSize)
		 { printf("? Data record exceeds bucket maximum... ignoring!\n") ; continue ; } ;
		if (sfl->File[sflx].Type == V4IS_SeqFileType_Seq)
		 { if (fread(data,sdh.Bytes,1,ifp) <= 0)
		    { printf("? Error reading data record... quitting\n") ; sflx=9999 ; break ; } ;
		 } ;
		switch (sdh.CmpMode)
		 { default: printf("? Invalid compression mode... ignoring record...\n") ; continue ;
		   case V4IS_DataCmp_None:
			pcb.PutBufPtr = data ; pcb.PutBufLen = sdh.Bytes ; break ;
		   case V4IS_DataCmp_Mth1:
			if (cdata == NULL) cdata = (char *)v4mm_AllocChunk(ri->BktSize,FALSE) ;
			pcb.PutBufLen = data_expand(cdata,data,sdh.Bytes) ; pcb.PutBufPtr = cdata ; break ;
		   case V4IS_DataCmp_Mth2:
			if (cdata == NULL) cdata = (char *)v4mm_AllocChunk(ri->BktSize*2,FALSE) ;
			lzrw1_decompress(data,sdh.Bytes,cdata,&pcb.PutBufLen,ri->BktSize*2) ;
			pcb.PutBufPtr = cdata ; break ;
		 } ;
	        if (sfl->File[sflx].Type != V4IS_SeqFileType_Index)
		 { pcb.FileRef = (fileref != -1 ? fileref : sdh.FileRef) ;
		   if (pcb.FileRef == 0)					/* Ensure proper placement of V4 records */
		    { switch (sdh.Placement)
		       { default: printf("? Invalid data mode in header, defaulting to 0\n") ; pcb.DataMode = 0 ; break ;
		         case V4IS_SeqDumpPlace_Index:	pcb.DataMode = V4IS_PCB_DataMode_Index ; break ;
		         case V4IS_SeqDumpPlace_Data:	pcb.DataMode = V4IS_PCB_DataMode_Data ; break ;
		       } ;
		      if (sdh.CmpMode == V4IS_DataCmp_None) pcb.PutMode = V4IS_PCB_NoDataCmp | pcb.DfltPutMode ;
		    } ;
		   outcnt ++ ;
		   if (!fastrebuild) { v4is_Put(&pcb,NULL) ; break ; } ;
		   vkey.dkey.DataId =
		    v4is_StashData(pcb.AreaId,data,sdh.Bytes,pcb.DataMode,sdh.CmpMode,0,pcb.FileRef,fillpercent) ;
		 } else
		 { dim.dataid = dataid ; dim.fld.BktNum += sfl->BktOffset ;	/* Add in offset */
		   vkey.dkey.DataId = dim.dataid ;				/* Pretend we did a StashData */
		 } ;
		for(kc=v4is_FFKeyCount(acb,pcb.FileRef);kc>=1;kc--)
		 { v4is_MakeFFKey(acb,pcb.FileRef,&vkey.dkey.KeyP,kc-1,NULL,pcb.PutBufPtr,FALSE) ;	/* Make next key */
#ifdef WINNT
		   WriteFile(kfd,&vkey,vkey.dkey.KeyP.fld.Bytes+sizeof vkey.dkey.DataId,&bytes,NULL) ;
#else
		   bytes = write(kfd,&vkey,vkey.dkey.KeyP.fld.Bytes+sizeof vkey.dkey.DataId) ;
#endif
		   if (bytes != vkey.dkey.KeyP.fld.Bytes+sizeof vkey.dkey.DataId)
		    { printf("? Error (%s) writing to temp sort file\n",v_OSErrString(errno)) ; exit(EXITABORT) ; } ;
		   sortrecs++ ;
		   if (vkey.dkey.KeyP.fld.Bytes > maxkeyBytes) maxkeyBytes = vkey.dkey.KeyP.fld.Bytes ;
		   if (kc > maxkeynum) maxkeynum = kc ;
		 } ;
		break ;
	      case V4IS_SeqDumpType_AreaHI:
		if (sdh.Bytes != sizeof sdahi)
		 { printf("Size in AreaHI header does not match that of program... quitting\n") ; sflx=9999 ; break ; } ;
		if (fread(&sdahi,sizeof sdahi,1,ifp) <= 0)
		 { printf("fread error (%s) on AreaHI detail, ... quitting!\n",v_OSErrString(errno)) ; sflx=9999 ; break ; } ;
		break ;
	      case V4IS_SeqDumpType_FFIKeys:
		if (sdh.Bytes > sizeof sdfk)
		 { printf("Size in Foreign Key header does not match that of program...quitting\n") ; sflx=9999 ; break ; } ;
		if (fread(&sdfk,sdh.Bytes,1,ifp) <= 0)
		 { printf("fread error (%s) on Foreign Key detail, ... quitting!\n",v_OSErrString(errno)) ; sflx=9999 ; break ; } ;
		sdfkd=(struct V4IS__SeqDumpFFIKDetail *)&sdfk.Buffer ;
		for((kx=0);kx<sdfk.Count;(kx++,sdfkd=(struct V4IS__SeqDumpFFIKDetail *)((char *)sdfkd+sdfkd->Bytes)))
		 { kid = (KID *)&ki.Buffer[ki.Bytes] ; ki.Count++ ;
		   kid->KeyCount = sdfkd->KeyCount ;
		   for(kc=0;kc<sdfkd->KeyCount;kc++)
		    { kid->Key[kc].KeyPrefix = sdfkd->Key[kc].KeyPrefix ; kid->Key[kc].Offset = sdfkd->Key[kc].Offset ;
		      if (kid->Key[kc].KeyPrefix.fld.AuxVal == sdfkd->FileRef && fileref != -1)
		       kid->Key[kc].KeyPrefix.fld.AuxVal = fileref ;		/* Update with new fileref */
		      kid->Key[kc].DupsOK = sdfkd->Key[kc].DupsOK ; kid->Key[kc].UpdateOK = sdfkd->Key[kc].UpdateOK ;
		    } ;
		   kid->FileRef = (fileref != -1 ? fileref : sdfkd->FileRef) ;
		   kid->Bytes = (char *)&kid->Key[kc] - (char *)kid ;
		   ki.Bytes += kid->Bytes ;
		 } ;
		ki.Bytes = (char *)&ki.Buffer[ki.Bytes] - (char *)&ki ;		/* Get proper length */
		break ;
	    } ;
	 } ;
	if (newmode)
	 {
#define CR(FIELD) ri.FIELD = sdri->FIELD ;
	   C(NextAvailDimDictNum) C(NextAvailExtDictNum) C(NextAvailIntDictNum) C(NextAvailValueNum) C(NextAvailUserNum) C(lastSegNum)
#define CAR(FIELD) acb->ahi->FIELD = sdahi.FIELD ;
	   if (acb->ahi != NULL)
	    { CAR(RelHNum) CAR(ExtDictUpd) CAR(IntDictUpd)
	      CAR(BindingsUpd)
	    } ;
	 } ;
	if (fastrebuild)
	 {
#ifdef WINNT
	   CloseHandle(kfd) ;
#else
	   close(kfd) ;			/* Close off the key file */
#endif
	   v_Msg(NULL,sbuf,"@-n %1d -kk0.%2d %3U %4U",sortrecs,maxkeyBytes+sizeof vkey.dkey.DataId,tnam1,tnam2) ;
	   if (!v_SpawnSort(sbuf,gpi->sortWaitFlag,tmpstr)) { vout_UCText(VOUT_Err,0,tmpstr) ; exit(EXITABORT) ; } ;
/*	   Now loop thru keys & just append to new area */
#ifdef WINNT
	   kfd = UCCreateFile(tnam2,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL) ;
	   if (kfd == INVALID_HANDLE_VALUE) v4_error(V4E_SORTFAIL,0,"V4ISU","RestoreArea","SORTFAIL","Sort (%s) appears to have failed",UCretASC(sbuf)) ;
	   DeleteFile(UCretASC(tnam1)) ;
#else
	   kfd = open(UCretASC(tnam2),O_RDONLY|O_BINARY) ;
	   if (kfd == -1) v4_error(V4E_SORTFAIL,0,"V4ISU","RestoreArea","SORTFAIL","Sort (%s) appears to have failed",sbuf) ;
	   UCremove(tnam1) ;				/* Don't need the first file anymore */
#endif

	   for(keycnt=0;;keycnt++)
	    {
#ifdef WINNT
	      if (!ReadFile(kfd,&vkey,sizeof vkey.dkey,&bytes,NULL)) break ;
	      if (bytes < sizeof vkey.dkey) break ;
#else
	      if (read(kfd,&vkey,sizeof vkey.dkey) < sizeof vkey.dkey) break ;
#endif
	      if (vkey.dkey.KeyP.fld.Bytes + sizeof vkey.dkey.DataId > sizeof vkey.dkey)
#ifdef WINNT
	       ReadFile(kfd,&vkey.AlphaValCont,vkey.dkey.KeyP.fld.Bytes+sizeof vkey.dkey.DataId - sizeof vkey.dkey,&bytes,NULL) ;
#else
	       read(kfd,&vkey.AlphaValCont,vkey.dkey.KeyP.fld.Bytes+sizeof vkey.dkey.DataId - sizeof vkey.dkey) ;
#endif
	      v4is_Append(pcb.AreaId,(struct V4IS__Key *)&vkey.dkey.KeyP,NULL,0,0,0,
			   (maxkeynum>1 && vkey.dkey.KeyP.fld.KeyType==V4IS_KeyType_FrgnKey1),vkey.dkey.DataId) ;
	      acb->CurPosKeyIndex++ ;		/* Make sure always at EOF */
	    } ;
	   printf("Appended %d keys to area\n",keycnt) ;
	   if (obscnt > 0) printf("Ignored %d obsolete records\n",obscnt) ;
#ifdef WINNT
	   CloseHandle(kfd) ; DeleteFile(UCretASC(tnam2)) ;
#else
	   close(kfd) ; UCremove(tnam2) ;		/* Close and trash */
#endif
	 } ;
	v4is_Close(&pcb) ;
	v4mm_FreeChunk(data) ; if (cdata != NULL) v4mm_FreeChunk(cdata) ;
	return(outcnt) ;
}


/*	v4is_RebuildAreaIndex - Rebuilds entire index for an area		*/
/*	Call: v4is_RebuildAreaIndex( pcb )
	  where pcb is pointer to pcb open for updates				*/

int v4is_RebuildAreaIndex(pcb)
  struct V4IS__ParControlBlk *pcb ;
{ struct V4IS__RootInfo *ri ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__Bkt *bkt ;
  struct V4__BktHdr *sbh ;
  struct V4IS__Key *key ;
  struct V4IS__IndexBktHdr *ibh ;
#ifdef WINNT
   HANDLE kfd ;
#else
   int kfd ;
#endif
#ifdef VMSOS
  struct vms__dsc strdsc ;
  int res,resx ;
#endif
  struct V4IS__DataBktHdr *dbh ;
  struct V4IS__DataBktEntryHdr *dbeh ;
  struct V4IS__UnusedBktHdr *ubh ;
  union V4MM__DataIdMask dim ;
  UCCHAR tnam1[100],tnam2[100] ;
  int bytes,maxkeyBytes,maxkeynum,keycnt,bktsize ; UCCHAR sbuf[200] ; UCCHAR tmpstr[100] ;
  struct {
    struct V4IS__DataIdKey dkey ;
    char AlphaValCont[300] ;
   } vkey ;
  char *data = NULL ; char *cdata = NULL ;
  int outcnt,index,kx,bktnum,sortrecs,goodbktnum ;

#ifndef V4_BUILD_V4IS_MAINT
	printf("This build of V4 does not include v4is_RebuildAreaIndex functionality!\n") ; return ;
#endif
	maxkeyBytes = 0 ;			/* Have to scan to determine largest key */
	maxkeynum = 0 ;				/* Have to track largest number of keys */
	sortrecs = 0 ;
	v_MakeOpenTmpFile(UClit("v4rai1"),tnam1,UCsizeof(tnam1),NULL,NULL) ;
	v_MakeOpenTmpFile(UClit("v4rai2"),tnam2,UCsizeof(tnam2),NULL,NULL) ;
#ifdef WINNT
	kfd = UCCreateFile(tnam1,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL) ;
	if (kfd == INVALID_HANDLE_VALUE)
	 { printf("? Error (%s) creating temp sort file (%s)\n",v_OSErrString(GetLastError()),UCretASC(tnam1)) ; exit(EXITABORT) ; } ;
#else
	kfd = creat(UCretASC(tnam1),0660) ;		/* Create temp file for keys */
	if (kfd == -1)
	 { printf("? Error (%s) creating temp sort file (%s)\n",v_OSErrString(errno),UCretASC(tnam1)) ; exit(EXITABORT) ; } ;
#endif

	acb = v4mm_ACBPtr(pcb->AreaId) ; ri = acb->RootInfo ; bktsize = ri->BktSize ;
	bkt = (struct V4IS__Bkt *)v4mm_AllocChunk(ri->BktSize,FALSE) ;	/* Allocate a bucket */
	for(bktnum=0;bktnum<ri->NextAvailBkt;bktnum++)			/* ZipZop thru all buckets */
	 { if( setjmp(((struct V4C__ProcessInfo *)v_GetProcessInfo())->environment) != 0)
	    { printf("Error accessing bucket (%d), ignoring...\n",bktnum-1) ; continue ; } ;
	   bkt = v4mm_BktPtr(bktnum,pcb->AreaId,IDX) ;
	   sbh = (BH *)bkt ;
	   switch (sbh->BktType)
	    { default: printf("  Unknown Bucket Type-9 (%d)\n",sbh->BktType) ; break ;
	      case V4_BktHdrType_Root:
		goodbktnum = bktnum ;					/* Save last OK bucket */
		continue ;
	      case V4_BktHdrType_Unused:
		goodbktnum = bktnum ;					/* Save last OK bucket */
		continue ;
	      case V4_BktHdrType_Index:
		goodbktnum = bktnum ;					/* Save last OK bucket */
		bkt = v4mm_BktPtr(bktnum,pcb->AreaId,IDX+UPD) ;
		ubh = (struct V4IS__UnusedBktHdr *)bkt ;	/* Bucket is empty - put on list */
		ubh->NextUnusedBkt = acb->RootInfo->NextUnusedBkt ; ubh->UBHFlag = UBHFlagVal ;
		ubh->sbh.BktType = V4_BktHdrType_Unused ;
		acb->RootInfo->NextUnusedBkt = ubh->sbh.BktNum ; v4mm_BktUpdated(pcb->AreaId,0) ;
		continue ;
	      case V4_BktHdrType_Data:
		goodbktnum = bktnum ;					/* Save last OK bucket */
		dbh = (DBH *)bkt ;
		for(index=dbh->FirstEntryIndex;index<dbh->FreeEntryIndex;index+=ALIGN(dbeh->Bytes))
		 { dbeh = (struct V4IS__DataBktEntryHdr *)&bkt->Buffer[index] ;
		   if (dbeh->Bytes <= sizeof *dbeh || dbeh->Bytes > bktsize)
		    { printf("? Bucket (%d) dbeh->Bytes=%d, min is %d, max is %d, ...continuing\n",bktnum,(int)dbeh->Bytes,(int)sizeof *dbeh,bktsize) ;
		      continue ;
		    } ;
		   dim.dataid = dbeh->DataId ;
		   if (dim.fld.BktNum != bktnum)
		    { printf("? Bucket (%d) dbeh->DataId=%x links to bucket %d, ...continuing\n",bktnum,dbeh->DataId,dim.fld.BktNum) ;
		      continue ;
		    } ;
		   data = (char *)dbeh+sizeof *dbeh ;
		   switch (dbeh->CmpMode)
		    { default: printf("? Invalid compression mode... ignoring record...\n") ; continue ;
		      case V4IS_DataCmp_None:
			break ;
		      case V4IS_DataCmp_Mth1:
			if (cdata == NULL) cdata = (char *)v4mm_AllocChunk(ri->BktSize,FALSE) ;
			data_expand(cdata,data,dbeh->Bytes-sizeof *dbeh) ; data = cdata ; break ;
		      case V4IS_DataCmp_Mth2:
			if (cdata == NULL) cdata = (char *)v4mm_AllocChunk(ri->BktSize*2,FALSE) ;
			lzrw1_decompress(data,dbeh->Bytes-sizeof *dbeh,cdata,&bytes,ri->BktSize*2) ;
			data = cdata ; break ;
		    } ;
		   vkey.dkey.DataId = dbeh->DataId ;
		   if (acb->KeyInfo == NULL)				/* No foreign file key info? */
  		    { key = (struct V4IS__Key *)data ;			/* Key best be begin of record */
		      memcpy(&vkey.dkey.KeyP,key,key->Bytes) ;
if (vkey.dkey.KeyP.fld.KeyMode > 3)
 { printf("? Invalid KeyMode (%d) for DataID (%x)... ignoring record\n",vkey.dkey.KeyP.fld.KeyMode,vkey.dkey.DataId) ; continue ; } ;
#ifdef WINNT
		      WriteFile(kfd,&vkey,vkey.dkey.KeyP.fld.Bytes+sizeof vkey.dkey.DataId,&bytes,NULL) ;
#else
		      bytes = write(kfd,&vkey,vkey.dkey.KeyP.fld.Bytes+sizeof vkey.dkey.DataId) ;
#endif
		      if (bytes != vkey.dkey.KeyP.fld.Bytes+sizeof vkey.dkey.DataId)
		       { printf("? Error (%s) writing to temp sort file\n",v_OSErrString(errno)) ; exit(EXITABORT) ; } ;
		      sortrecs ++ ;
		      if (vkey.dkey.KeyP.fld.Bytes > maxkeyBytes) maxkeyBytes = vkey.dkey.KeyP.fld.Bytes ;
		      continue ;
		    } ;
		   for(kx=v4is_FFKeyCount(acb,pcb->FileRef);kx>=1;kx--)
		    { v4is_MakeFFKey(acb,pcb->FileRef,&vkey.dkey.KeyP,kx-1,NULL,data,FALSE) ;	/* Make next key */
#ifdef WINNT
		      WriteFile(kfd,&vkey,vkey.dkey.KeyP.fld.Bytes+sizeof vkey.dkey.DataId,&bytes,NULL) ;
#else
		      bytes = write(kfd,&vkey,vkey.dkey.KeyP.fld.Bytes+sizeof vkey.dkey.DataId) ;
#endif
		      if (bytes != vkey.dkey.KeyP.fld.Bytes+sizeof vkey.dkey.DataId)
		       { printf("? Error (%s) writing to temp sort file\n",v_OSErrString(errno)) ; exit(EXITABORT) ; } ;
		      sortrecs ++ ;
		      if (vkey.dkey.KeyP.fld.Bytes > maxkeyBytes) maxkeyBytes = vkey.dkey.KeyP.fld.Bytes ;
		      if (kx > maxkeynum) maxkeynum = kx ;
		    } ;
		 } ;
		break ;
	    } ;
	 } ;
	if (ri->NextAvailBkt != goodbktnum+1)
	 { printf("Last valid bucket number is %d, changing ri->NextAvailBkt from %d to %d\n",
			goodbktnum,ri->NextAvailBkt,goodbktnum+1) ;
	   ri->NextAvailBkt = goodbktnum+1 ;
	 } ;
#ifdef WINNT
	CloseHandle(kfd) ;
#else
	close(kfd) ;			/* Close off the key file */
#endif
	v_Msg(NULL,sbuf,"@-n %1d -kk0.%2d %3U %4U",sortrecs,maxkeyBytes+sizeof vkey.dkey.DataId,tnam1,tnam2) ;
//printf("Sort: %s\n",sbuf) ;
	if (!v_SpawnSort(sbuf,gpi->sortWaitFlag,tmpstr)) { vout_UCText(VOUT_Err,0,tmpstr) ; exit(EXITABORT) ; } ;
	bkt = v4mm_BktPtr(0,pcb->AreaId,IDX+UPD) ;		/* Link up to root bucket again */
	ibh = (IBH *)bkt ; ibh->KeyEntryNum = 0 ;
	ibh->KeyEntryTop = sizeof *ibh + sizeof *ri ;
	ibh->KeyEntryNum = 0 ; ibh->DataTop = ri->BktSize ;
	ibh->FreeBytes = ri->BktSize - (sizeof *ibh + sizeof *ri) ;
	ri->NumLevels = 1 ;
	acb->Lvl[acb->CurLvl=0].BktNum = 0 ; acb->CurPosKeyIndex = 0 ; acb->KeyPrefix.all = -1 ;
	if (ri->FreeDataInfoByte != 0 && ri->FreeDataInfoByte != ibh->KeyEntryTop)
	 { ibh->KeyEntryTop += ALIGN(sizeof(struct V4IS__FreeDataInfo)) ; ibh->FreeBytes -= ALIGN(sizeof(struct V4IS__FreeDataInfo)) ;
	   printf("[WARNING: RootInfo block not correct, use -or/-of to rebuild file ASAP]\n") ;
	   printf(" continuing with -k ... area should be OK for now!\n") ;
	 } ;
	if (ri->FreeDataInfoByte != 0)
	 { ibh->KeyEntryTop += ALIGN(sizeof(struct V4IS__FreeDataInfo)) ; ibh->FreeBytes -= ALIGN(sizeof(struct V4IS__FreeDataInfo)) ; } ;
	if (ri->FFIKeyInfoByte != 0)
	 { ibh->KeyEntryTop += ALIGN(acb->KeyInfo->Bytes) ; ibh->FreeBytes -= ALIGN(acb->KeyInfo->Bytes) ; } ;
	if (ri->AreaHInfoByte)
	 { ibh->KeyEntryTop += ALIGN(sizeof(struct V4C__AreaHInfo)) ; ibh->FreeBytes -= ALIGN(sizeof(struct V4C__AreaHInfo)) ; } ;
/*	Now loop thru keys & just append to new area */
#ifdef WINNT
	kfd = UCCreateFile(tnam2,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL) ;
	if (kfd == INVALID_HANDLE_VALUE)
	 { printf("? Error (%s) opening temp sort file (%s)\n",v_OSErrString(GetLastError()),UCretASC(tnam2)) ; exit(EXITABORT) ; } ;
	DeleteFile(UCretASC(tnam1)) ;
#else
	kfd = open(UCretASC(tnam2),O_RDONLY|O_BINARY) ;
	if (kfd == -1)
	 { printf("? Error (%s) opening temp sort file (%s)\n",v_OSErrString(errno),UCretASC(tnam2)) ; exit(EXITABORT) ; } ;
	UCremove(tnam1) ;				/* Don't need the first file anymore */
#endif
	for(keycnt=0;;keycnt++)
	 { if( setjmp(((struct V4C__ProcessInfo *)v_GetProcessInfo())->environment) != 0)
	    { printf("Error appending key %d to index, ABORTING - AREA PROBABLY CORRUPT!\n",keycnt) ;
	      printf("  Use -os/-oo combination or -of to completely rebuild area\n") ;
	      break ;
	    } ;
#ifdef WINNT
	   if (!ReadFile(kfd,&vkey,sizeof vkey.dkey,&bytes,NULL)) break ;
	   if (bytes < sizeof vkey.dkey) break ;
#else
	   if (read(kfd,&vkey,sizeof vkey.dkey) < sizeof vkey.dkey) break ;
#endif
	   if (vkey.dkey.KeyP.fld.Bytes + sizeof vkey.dkey.DataId > sizeof vkey.dkey)
#ifdef WINNT
	    ReadFile(kfd,&vkey.AlphaValCont,vkey.dkey.KeyP.fld.Bytes+sizeof vkey.dkey.DataId - sizeof vkey.dkey,&bytes,NULL) ;
#else
	    read(kfd,&vkey.AlphaValCont,vkey.dkey.KeyP.fld.Bytes+sizeof vkey.dkey.DataId - sizeof vkey.dkey) ;
#endif
	   v4is_Append(pcb->AreaId,(struct V4IS__Key *)&vkey.dkey.KeyP,NULL,0,0,0,
			   (maxkeynum>1 && vkey.dkey.KeyP.fld.KeyType==V4IS_KeyType_FrgnKey1),vkey.dkey.DataId) ;
	   acb->CurPosKeyIndex++ ;		/* Make sure always at EOF */
	 } ;
	printf("Appended %d keys to area\n",keycnt) ;
#ifdef WINNT
	CloseHandle(kfd) ; DeleteFile(UCretASC(tnam2)) ;
#else
	close(kfd) ; UCremove(tnam2) ;		/* Close and trash */
#endif
	v4is_Close(pcb) ;
	if (cdata != NULL) v4mm_FreeChunk(cdata) ;
	return(outcnt) ;
}

/*	D E B U G G I N G   M O D U L E S			*/

/*	v4is_FormatKey - Formats key for printing				*/
/*	Call: strptr = v4is_FormatKey( keyptr )
	  where strptr is pointer to formatted string,
		keyptr is pointer to a key					*/

char *v4is_FormatKey(keyptr)
  struct V4IS__Key *keyptr ;
{ static char buf[100] ;
  char tbuf[100] ; double dnum ; B64INT b64 ;

	switch (keyptr->KeyMode)
	 { default:			strcpy(tbuf,"?mode?") ; break ;
	   case V4IS_KeyMode_Int:
	   case V4IS_KeyMode_RevInt:	sprintf(tbuf,"%d",keyptr->KeyVal.IntVal) ; break ;
	   case V4IS_KeyMode_Int2:	sprintf(tbuf,"%d+%d",keyptr->KeyVal.Int2Val[0],keyptr->KeyVal.Int2Val[1]) ; break ;
	   case V4IS_KeyMode_Alpha:	strncpy(tbuf,keyptr->KeyVal.Alpha,keyptr->Bytes - sizeof (union V4IS__KeyPrefix)) ;
					tbuf[keyptr->Bytes - sizeof (union V4IS__KeyPrefix)] = 0 ; break ;
	   case V4IS_KeyMode_DblFlt:	memcpy(&dnum,keyptr->KeyVal.B64,sizeof dnum) ; sprintf(tbuf,"%g",dnum) ; break ;
	   case V4IS_KeyMode_Fixed:	memcpy(&b64,keyptr->KeyVal.B64,sizeof b64) ; sprintf(tbuf,"%lld",b64) ; break ;
	 } ;
	sprintf(buf,"(M:%d T:%d Aux:%d B:%d)=%s",keyptr->KeyMode,keyptr->KeyType,keyptr->AuxVal,keyptr->Bytes,tbuf) ;
	return(buf) ;
}

/*	v4is_BugChk - Dumps as much as possible to a file for later study	 */
/*	Call: v4is_BugChk( areaid , message )
	  where areaid is area id to be checked,
		message is descriptive message of problem			*/

void v4is_BugChk(areaid,message)
  int areaid ;
  char *message ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__LockTable *lt ;
  struct V4MM__GlobalBuf *glb ;
  time_t timer ;
  FILE *fp ;
  int ax,bx,i ; char file[V_FileName_Max] ;

#ifndef V4_BUILD_V4IS_MAINT
	printf("This build of V4 does not include v4is_BugChk functionality!\n") ; return ;
#endif
	FINDAREAINPROCESS(ax,areaid)
	if (ax == UNUSED) return ;			/* Could not find area? */
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++) { if (mmm->Areas[ax].AreaId == areaid) break ; } ;
//	if (ax >= mmm->NumAreas) return ;			/* Could not find area? */
	time(&timer) ;
	sprintf(file,"%lld.bugchk",time(NULL)) ;			/* Create file-name */
	if ((fp = fopen(file,"a")) == NULL) return ;
	fprintf(fp,"%.19s - V4IS BugChk on file %s, pid %d, job %d - %s\n",
		ctime(&timer),UCretASC(mmm->Areas[ax].UCFileName),mmm->MyPid,mmm->UserJobId,message) ;
	if ((mmm->Areas[ax].SegId == 0) || (GETGLB(ax) == NULL))
	 { fclose(fp) ; return ; } ;
	fprintf(fp,"Bkt Number Pending PendPid   CallCnt Lock Upd\n") ;
	for(bx=0;bx<glb->BktCnt;bx++)
	 { fprintf(fp,"%3d%7d%8d%8d%10d%5d%4d\n",
			 bx,glb->Bkt[bx].BktNum,glb->Bkt[bx].PendingBktNum,glb->Bkt[bx].PendingPid,
			 glb->Bkt[bx].CallCnt,glb->Bkt[bx].MemLock,glb->Bkt[bx].Updated) ;
	 } ;
	lt = (struct V4MM__LockTable *)&glb->Buffer[glb->LockTableOffset] ;
	fprintf(fp,"LockDat=%d, LockIDX=%d, LockTree=%d\n",mmm->Areas[ax].LockDAT,mmm->Areas[ax].LockIDX,mmm->Areas[ax].LockTree) ;
	fprintf(fp,"LastUniqueLockId=%d, PidCnt=%d (%d %d %d)\n",lt->LastUniqueLockId,lt->PidCnt,
		lt->LogPids[0],lt->LogPids[1],lt->LogPids[2]) ;
	fprintf(fp,"lt->LocksTree=%d, lt->LocksIDX=%d\n",lt->LocksTree,lt->LocksIDX) ;
	fprintf(fp,"Lock Src Type Modes        Id     UID Pids Last 3----------- Wait Last 6\n") ;
	for(i=0;i<lt->Count;i++)
	 { fprintf(fp,"%4d%4d%5d%6d%10d%8d%5d%6d%6d%6d%5d%6d%6d%6d%6d%6d%6d\n",
			i,lt->Entry[i].Source,lt->Entry[i].LockIdType,lt->Entry[i].LockMode,lt->Entry[i].LockId,lt->Entry[i].UniqueLockId,
			lt->Entry[i].PidCount,lt->Entry[i].Pids[0],lt->Entry[i].Pids[1],lt->Entry[i].Pids[2],lt->Entry[i].WaitCount,
			lt->Entry[i].WaitPids[0],lt->Entry[i].WaitPids[1],lt->Entry[i].WaitPids[2],lt->Entry[i].WaitPids[3],
			lt->Entry[i].WaitPids[4],lt->Entry[i].WaitPids[5]) ;
	 } ;
	fclose(fp) ;
}

void v4is_SearchForLink(link,aid)
  int link ;
  int aid ;
{ struct V4IS__AreaCB *acb ;
  struct V4IS__Bkt *bkt ;
  struct V4__BktHdr *sbh ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  int bktnum,kx ;
  
	acb = v4mm_ACBPtr(aid) ;
	for(bktnum=0;bktnum<acb->RootInfo->NextAvailBkt;bktnum++)
	 { bkt = v4mm_BktPtr(bktnum,aid,IDX) ; sbh = (BH *)bkt ;
	   switch (sbh->BktType)
	    { case V4_BktHdrType_Root:
	      case V4_BktHdrType_Index:
		ibh = (IBH *)bkt ; ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
		for(kx=0;kx<ibh->KeyEntryNum;kx++)
		 { 
		   ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[kx]] ;
		   switch (ikde->EntryType)
		    { default: printf("    Key %d - UNKNOWN ENTRY TYPE (%d)\n",kx,ikde->EntryType) ; break ;
		      case V4IS_EntryType_IndexLink:
			if (ikde->AuxVal != link) continue ;
			printf("Index bucket (%d), parent (%d), key index (%d of %d) links to index bucket (%d)\n",
				sbh->BktNum, ibh->ParentBktNum,kx+1,ibh->KeyEntryNum,ikde->AuxVal) ;
			break ;
		      case V4IS_EntryType_DataLink:
			if (ikde->AuxVal != link) continue ;
			printf("Index bucket (%d), parent (%d), key index (%d of %d) links to data bucket (%d)\n",
				sbh->BktNum, ibh->ParentBktNum,kx+1,ibh->KeyEntryNum,ikde->AuxVal) ;
			break ;
		      case V4IS_EntryType_DataLen:
			break ;
		    } ;
		 } ;
		break ;
	      case V4_BktHdrType_Data:		continue ;
	    } ;
	 } ;
}

void v4is_ExamineBkts(type,aid)
  int type ;
  int aid ;
{ struct V4IS__AreaCB *acb ;
  struct V4IS__Bkt *bkt ;
  int bktnum ;
  struct V4__BktHdr *sbh ;
  
	acb = v4mm_ACBPtr(aid) ;
	for(bktnum=0;bktnum<acb->RootInfo->NextAvailBkt;bktnum++)
	 { bkt = v4mm_BktPtr(bktnum,aid,IDX) ; sbh = (BH *)bkt ;
	   switch (sbh->BktType)
	    { case V4_BktHdrType_Root:		if (type == V4_BktHdrType_Index || type == 0) v4is_ExamineBkt(bktnum,aid) ; break ;
	      case V4_BktHdrType_Index:		if (type == V4_BktHdrType_Index || type == 0) v4is_ExamineBkt(bktnum,aid) ; break ;
	      case V4_BktHdrType_Data:		if (type == V4_BktHdrType_Data || type == 0) v4is_ExamineBkt(bktnum,aid) ; break ;
	    } ;
//	   if (sbh->BktType != type) continue ;
//	   v4is_ExamineBkt(bktnum,aid) ;
	 } ;
}

void v4is_ExamineBkt(bktnum,aid)
  int bktnum ;
{ 
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__Bkt *bkt ;
  struct V4__BktHdr *sbh ;
  struct V4IS__Key *key ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4IS__DataBktHdr *dbh ;
  struct V4IS__DataBktEntryHdr *dbeh ;
  union V4IS__KeyPrefix *kp ;
  int index,kx,ax ;

#ifndef V4_BUILD_V4IS_MAINT
	printf("This build of V4 does not include v4is_ExamineBkt functionality!\n") ; return ;
#endif
	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) return ;			/* Could not find area? */
//	mmm = V4_GLOBAL_MMM_PTR ;
	bkt = v4mm_BktPtr(bktnum,aid,IDX) ; sbh = (BH *)bkt ;
	printf("Bucket #%d (Next Data ID is %d)\n",sbh->BktNum,sbh->AvailBktSeq) ;
	switch (sbh->BktType)
	 { default: printf("  Unknown Bucket Type-10 (%d)\n",sbh->BktType) ; break ;
	   case V4_BktHdrType_Unused:
		printf("  Bucket currently not in use\n") ; break ;
	   case V4_BktHdrType_Root:
		printf("  Type ROOT\n") ;
	   case V4_BktHdrType_Index:
		if (sbh->BktType == V4_BktHdrType_Index) printf("  Type INDEX\n") ;
		ibh = (IBH *)bkt ; ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
		printf("	Parent bucket is %d, Freebytes=%d\n",ibh->ParentBktNum,ibh->FreeBytes) ;
		for(kx=0;kx<ibh->KeyEntryNum;kx++)
		 { if (ibkl->DataIndex[kx] < sizeof *ibh || ibkl->DataIndex[kx] > mmm->Areas[aid].BktSize)
		    printf("? Next key- DataIndex[%d] = %d not between %d .. %d\n",
				kx,(int)ibkl->DataIndex[kx],(int)sizeof *ibh,mmm->Areas[aid].BktSize) ;
		   ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[kx]] ;
		   kp = (KP *)(key = (KEY *)((char *)&bkt->Buffer[ibkl->DataIndex[kx]] + sizeof *ikde)) ;
		   switch (ikde->EntryType)
		    { default: printf("    Key %d - UNKNOWN ENTRY TYPE (%d)\n",kx,ikde->EntryType) ; break ;
		      case V4IS_EntryType_IndexLink:
				printf("    Key %d: Prefix(%x.%d %s) link to bucket %d\n",kx,kp->all,key->KeyVal.IntVal,
					v4is_FormatKey((struct V4IS__Key *)kp),ikde->AuxVal) ;
				break ;
		      case V4IS_EntryType_DataLink:
				printf("    Key %d: Prefix(%x.%d %s) Data ID (%x) in bucket %d\n",kx,kp->all,key->KeyVal.IntVal,
					v4is_FormatKey((struct V4IS__Key *)kp),ikde->DataId,ikde->AuxVal) ;
				break ;
		      case V4IS_EntryType_DataLen:
				printf("    Key %d: Prefix(%x.%d) Local data of %d bytes (DataID %x)\n",
						kx,kp->all,key->KeyVal.IntVal,ikde->AuxVal,ikde->DataId) ;
				break ;
		    } ;
		 } ;
		break ;
	   case V4_BktHdrType_Data:
		dbh = (DBH *)bkt ;
		printf("  DATA Bucket #%d, Free %d, FirstEntry %d, FreeEntry %d\n",
			dbh->sbh.BktNum,dbh->FreeBytes,dbh->FirstEntryIndex,dbh->FreeEntryIndex) ;
		for(index=dbh->FirstEntryIndex;index<dbh->FreeEntryIndex;index+=ALIGN(dbeh->Bytes))
		 { dbeh = (struct V4IS__DataBktEntryHdr *)&bkt->Buffer[index] ;
		   kp = (KP *)(key = (KEY *)((char *)&bkt->Buffer[index] + sizeof *dbeh)) ;
		   printf("    Data at %d: ID=%x, Bytes=%d, Prefix(%x.%d)\n",index,dbeh->DataId,dbeh->Bytes,kp->all,
		  	 key->KeyVal.IntVal) ;
		   if (dbeh->Bytes == 0) { printf("? Invalid dbhe->Bytes (0) ... quitting\n") ; break ; } ;
		 } ;
		break ;
	 } ;
	SBHINUSE ;
}

#ifdef NOTCURRENTLYINUSE

v4is_ValidateBkt(bktnum,aid)
  int bktnum ;
{ struct V4IS__AreaCB *acb ;
  struct V4IS__Bkt *bkt ;
  struct V4__BktHdr *sbh ;
  struct V4IS__Key *key ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4IS__DataBktHdr *dbh ;
  struct V4IS__DataBktEntryHdr *dbeh ;
  union V4IS__KeyPrefix *kp ;
  int index,kx ;

	bkt = v4mm_BktPtr(bktnum,aid,IDX) ; sbh = (BH *)bkt ;
	switch (sbh->BktType)
	 { default: printf("  Unknown Bucket Type (%d)\n",sbh->BktType) ; break ;
	   case V4_BktHdrType_Root:
	   case V4_BktHdrType_Index:
		ibh = (IBH *)bkt ; ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
		printf("	Parent bucket is %d\n",ibh->ParentBktNum) ;
		for(kx=0;kx<ibh->KeyEntryNum;kx++)
		 { ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[kx]] ;
		   kp = (KP *)(key = (KEY *)((char *)&bkt->Buffer[ibkl->DataIndex[kx]] + sizeof *ikde)) ;
		   switch (ikde->EntryType)
		    { default:
			printf("    Key %d - UNKNOWN ENTRY TYPE (%d)\n",kx,ikde->EntryType) ; break ;
		      case V4IS_EntryType_IndexLink:
				break ;
		      case V4IS_EntryType_DataLink:
				break ;
		      case V4IS_EntryType_DataLen:
				break ;
		    } ;
		 } ;
		break ;
	   case V4_BktHdrType_Data:
		dbh = (DBH *)bkt ;
		for(index=dbh->FirstEntryIndex;index<dbh->FreeEntryIndex;index+=ALIGN(dbeh->Bytes))
		 { dbeh = (struct V4IS__DataBktEntryHdr *)&bkt->Buffer[index] ;
		   kp = (KP *)(key = (KEY *)((char *)&bkt->Buffer[index] + sizeof *dbeh)) ;
		 } ;
		break ;
	 } ;
}

/*	Checks bucket's keys for proper order	*/

v4is_CheckBkt(bkt,locmsg)
  struct V4IS__Bkt *bkt ;
  char *locmsg ;
{ struct V4IS__AreaCB *acb ;
  struct V4__BktHdr *sbh ;
  union V4IS__KeyPrefix *kp ;
  struct V4IS__Key *key ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  int oldkey,oldkp,kx ; char buf[250] ; char oldstr[1000] ;

/* return ;	NOTE: this does not work if area has alpha keys! */
	ibh = (IBH *)bkt ; ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;
	for(kx=0;kx<ibh->KeyEntryNum;kx++)
	 { ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[kx]] ;
	   kp = (KP *)(key = (KEY *)((char *)&bkt->Buffer[ibkl->DataIndex[kx]] + sizeof *ikde)) ;
	   if (kx > 0 && oldkp == kp->all &&
	         (kp->fld.KeyMode == V4IS_KeyMode_Alpha ?
		   memcmp(oldstr,&key->KeyVal,kp->fld.Bytes) == 0 : key->KeyVal.IntVal == oldkey)
	      )
	    { sprintf(buf,"Key #%d (%d > %d) in bkt #%d",kx,oldkey,key->KeyVal.IntVal,ibh->sbh.BktNum) ;
	      v4_error(V4E_CHKBKTFAIL,0,"V4IS",locmsg,"CHKBKTFAIL",buf) ;
	    } ;
	   oldkey = key->KeyVal.IntVal ; oldkp = kp->all ;
	   if (kp->fld.KeyMode == V4IS_KeyMode_Alpha) memcpy(oldstr,&key->KeyVal,kp->fld.Bytes) ;
	 } ;
/*	printf("Bkt: %d OK! at %s\n",ibh->sbh.BktNum,locmsg) ; */
}

/*	v4is_CheckTree - Checks out directory tree for an area (gets slow on large areas!) */
/*	Call: logical = v4is_CheckTree( pcb )
	  where logical is TRUE if tree OK, FALSE if not, acb set to directory status,
		pcb is control block to area to be checked				*/

v4is_CheckTree(pcb)
  struct V4IS__ParControlBlk *pcb ;
{ struct V4IS__Bkt *bkt ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__Key *primekey ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  union V4IS__KeyPrefix *kp,oldkp ;
  int posflag,count ;

	acb = v4mm_ACBPtr(pcb->AreaId) ;
	oldkp.all = 0x80000000 ;
	for(posflag=V4IS_PosBOF,count=0;;posflag=V4IS_PosNext,count++)
	 {
	   if (! v4is_PositionRel(pcb->AreaId,posflag,-1,NULL)) break ;
	   bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,pcb->AreaId,IDX) ;
	   ibh = (IBH *)bkt ;		/* Get the bucket */
	   ibkl = (IBKL *)&bkt->Buffer[ibh->KeyEntryTop] ;	/* Pointer to B header & Key indexes */
	   ikde = (IKDE *)&bkt->Buffer[ibkl->DataIndex[acb->CurPosKeyIndex]] ;
	   primekey = (KEY *)((char *)ikde + sizeof *ikde) ;	/* Pointer to primary key */
	   kp = (KP *)primekey ; IBHINUSE ;
	   if (kp->all != oldkp.all)
	    { if (kp->all < oldkp.all) return(-count) ;
	      oldkp.all = kp->all ;
	    } ;
	 } ;
	return(count) ;				/* Tree looks OK */
}
#endif		/* NOTCURRENTLYINUSE */


/*	C L I E N T  -  S E R V E R   M O D U L E S			*/


#ifdef V4_BUILD_CS_FUNCTIONALITY

/*	v4is_RemoteFileRef - Logs a remote file via file-ref (must be done before file is opened) */
/*	Call: index = v4is_RemoteFileRef( fileref , "host" , port , options )
	  where index is index in table (-1) for error,
		fileref is fileref to be recorded,
		host is null-terminated ascii string of host,
		port is port number (if 0 then host assumed to be Unix style socket filename),
		options are- V4CS_RFROpt_xxx					*/

int v4is_RemoteFileRef(fileref,host,port,options)
  int fileref ;
  char *host ;
  int port,options ;
{
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4CS__RemoteFileRef *rfr ;

#ifdef NEWMMM
needwork;
#else
	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
#endif
	if (mmm->rfr == NULL)					/* Maybe have to allocate ? */
	 mmm->rfr = (struct V4CS__RemoteFileRef *)v4mm_AllocChunk(sizeof *rfr,TRUE) ;
	rfr = mmm->rfr ;
	if (rfr->FileRefCount >= V4CS_RFRFileRef_Max) return(-1) ;	/* Table is full */
	rfr->FileRefs[rfr->FileRefCount].FileRef = fileref ;		/* Copy args into new FileRefs */
	if (strlen(host) >= sizeof rfr->FileRefs[0].HostName) return(-1) ;
	strcpy(rfr->FileRefs[rfr->FileRefCount].HostName,host) ;
	rfr->FileRefs[rfr->FileRefCount].PortId = port ;
	rfr->FileRefs[rfr->FileRefCount].Options = options ;
	rfr->FileRefs[rfr->FileRefCount].Socket = -1 ;
	return(rfr->FileRefCount++) ;
}

/*	v4is_ConnectFileRef - Connects a FileRef with its remote server		*/
/*	Call: socket = v4is_ConnectFileRef( fileref , acb )
	  where socket is socket number to server (-n if error),
		fileref is fileref to be connected,
		acb is Area control block					*/

int v4is_ConnectFileRef(fileref,acb)
  int fileref ;
  struct V4IS__AreaCB *acb ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4CS__RemoteFileRef *rfr ;
#ifdef USOCKETS
   struct sockaddr_un saun ;
#define SOCKETS 1
#endif
#ifdef ISOCKETS
   char *ptr ;
   int bytes,clen,got,msgtype ;
   struct sockaddr_in sin ;
   struct V4CS__MsgHeader req_tport ;
   struct V4CS__Msg_TPort  reply ;
#define SOCKETS 1
#endif
#ifdef SOCKETS
   struct hostent *hp ;
   int tsocket ;
#endif
#ifdef WINNT
  OFSTRUCT ofs ;
  WSADATA wsad ;
  int i ;
#endif
int fx,sx ;

#ifdef SOCKETS
#ifdef NEWMMM
needwork;
#else
	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
#endif
	if (mmm->rfr == NULL)					/* Maybe have to allocate ? */
	 mmm->rfr = (struct V4CS__RemoteFileRef *)v4mm_AllocChunk(sizeof *rfr,TRUE) ;
	rfr = mmm->rfr ;
	for(fx=0;fx<rfr->FileRefCount;fx++)			/* Look for fileref */
	 { if (rfr->FileRefs[fx].FileRef == fileref) break ; } ;
	if (fx >= rfr->FileRefCount) return(-1) ;		/* FileRef not recorded (see above routine) */
	if (rfr->FileRefs[fx].Socket != -1) return(rfr->FileRefs[fx].Socket) ;
	for(sx=0;sx<rfr->ServerCount;sx++)			/* Have to look for connection to server */
	 { if (rfr->FileRefs[fx].PortId != rfr->Servers[sx].PortId) continue ;
	   if (strcmp(rfr->FileRefs[fx].HostName,rfr->Servers[sx].HostName) != 0) continue ;
	   rfr->FileRefs[fx].Socket = rfr->Servers[sx].Socket ;	/* Already have a connection! */
	   return(rfr->FileRefs[fx].Socket) ;
	 } ;
/*	When here then have to create a new connection to a remote server */
	if (rfr->ServerCount >= V4CS_RFRServer_Max) return(-2) ;
	SOCKETINIT
	if (rfr->FileRefs[fx].PortId <= 0)			/* If no port then assume unix socket */
	 {
#ifdef USOCKETS
	   if ((tsocket = socket(AF_UNIX, SOCK_STREAM,0)) < 0)
	    v4_error(V4E_SOCKETMAKE,0,"V4ISU","REMOTEFILEREF","SOCKETMAKE","Error (%s) creating Unix Socket (%s)",
			v_OSErrString(NETERROR),rfr->FileRefs[fx].HostName) ;
	   memset(&saun,0,sizeof saun) ;
	   saun.sun_family = AF_UNIX ; strcpy(saun.sun_path,rfr->FileRefs[fx].HostName) ;
	   if (connect(tsocket,&saun,sizeof saun.sun_family + strlen(saun.sun_path)) < 0)
	    v4_error(V4E_SOCKETCONNECT,0,"V4IS","REMOTEFILEREF","SOCKETCONNECT","Error (%s) connect\'ing Unix Socket to name (%s)",
				v_OSErrString(NETERROR),rfr->FileRefs[fx].HostName) ;
#else
	   v4_error(V4E_USOCKETNOTSUP,0,"V4IS","REMOTEFILEREF","USOCKETNOTSUP","Unix Sockets not supported in this version of V4") ;
#endif
	 } else /*	Here to open Internet Socket */
	 {
#ifdef ISOCKETS
	   if ((tsocket = socket(AF_INET, SOCK_STREAM,0)) < 0)
	     v4_error(V4E_SOCKETMAKE,0,"V4IS","REMOTEFILEREF","SOCKETMAKE","Error (%s) creating INET Socket (%s)",v_OSErrString(NETERROR),rfr->FileRefs[fx].HostName) ;
	   memset(&sin,0,sizeof sin) ;
	   sin.sin_family = AF_INET ;
/*	   Check file_name for host/port number, if no host then use current host */
	   sin.sin_port = htons(rfr->FileRefs[fx].PortId) ;
	   if ((hp = gethostbyname(rfr->FileRefs[fx].HostName)) == NULL)
	     v4_error(V4E_GETHOSTADDR,0,"V4IS","REMOTEFILEREF","GETHOSTADDR","Error (%s) obtaining network address of host (gethostbyname(%s))",
			v_OSErrString(NETERROR),rfr->FileRefs[fx].HostName) ;
	   memcpy(&sin.sin_addr,hp->h_addr,hp->h_length) ;
	   if (connect(tsocket,&sin,sizeof sin) < 0)
	     v4_error(V4E_SOCKETCONNECT,0,"V4IS","REMOTEFILEREF","SOCKETCONNECT","Error (%s) connect'ing INET Socket to name (%s)",
			v_OSErrString(NETERROR),rfr->FileRefs[fx].HostName) ;
           req_tport.MsgType = V4CS_Msg_ReqTPort ;
           req_tport.Flags = 0 ;
           req_tport.Bytes = sizeof(req_tport) ;
           send(tsocket,&req_tport,sizeof(req_tport),0) ;
	   recv(tsocket,&reply.mh,sizeof(struct V4CS__MsgHeader),0) ;	/* Get reply header */
           msgtype = reply.mh.MsgType ;
	   bytes = reply.mh.Bytes ;
	   if (bytes != sizeof (struct V4CS__Msg_TPort))		/* If size not correct then have to flip */
	    acb->CSFlags |= V4CS_Flag_FlipEndian ;
	   if (acb->CSFlags & V4CS_Flag_FlipEndian)
	    FLIPSHORT(bytes,reply.mh.Bytes) ;
	   ptr = &reply.TPort ;
	   for(got=sizeof(struct V4CS__MsgHeader);got<bytes;)
	   { if ((clen = recv(tsocket,ptr,bytes-got,0)) < 0)
	       v4_error(V4E_RECVERR,0,"V4IS","REMOTEFILEREF","SOCKETRECV","Error (%s) in recv on socket (%d)",
			   v_OSErrString(NETERROR),tsocket) ;
	     got += clen ;
	     ptr += clen ;			/* May not receive entire buffer in one recv packet */
	   } ;
	   SOCKETCLOSE(tsocket) ; ;
	   if ((tsocket = socket(AF_INET, SOCK_STREAM,0)) < 0)
	    v4_error(V4E_SOCKETMAKE,0,"V4IS","REMOTEFILEREF","SOCKETMAKE","Error (%s) creating INET Socket (%s)",v_OSErrString(NETERROR),rfr->FileRefs[fx].HostName) ;
	   memset(&sin,0,sizeof sin) ;
	   sin.sin_family = AF_INET ;
/*	   Check file_name for host/port number, if no host then use current host */
	   if (acb->CSFlags & V4CS_Flag_FlipEndian) FLIPLONG(reply.TPort,reply.TPort) ;
	   sin.sin_port = htons(reply.TPort) ;
	   memcpy(&sin.sin_addr,hp->h_addr,hp->h_length) ;
	   if (connect(tsocket,&sin,sizeof sin) < 0)
	     v4_error(V4E_SOCKETCONNECT,0,"V4IS","REMOTEFILEREF","SOCKETCONNECT","Error (%s) connect'ing INET Socket to name (%s)",
			v_OSErrString(NETERROR),rfr->FileRefs[fx].HostName) ;
#else
	   v4_error(V4E_ISOCKETNOTSUP,0,"V4IS","REMOTEFILEREF","ISOCKETNOTSUP","INET Sockets not supported in this version of V4") ;
#endif
	 } ;
	sx = rfr->ServerCount ;
	rfr->Servers[sx].Socket = tsocket ; rfr->Servers[sx].PortId = rfr->FileRefs[fx].PortId;
	strcpy(rfr->Servers[sx].HostName,rfr->FileRefs[fx].HostName) ;
	rfr->ServerCount ++ ;
	return(tsocket) ;
#else
	return(-1) ;
#endif
}

/*	v4is_DisconnectAreaId - Disconnects (maybe) a socket from a server	*/
/*	Call: ok = v4is_DisconnectFileRef( areaid , cssocket )
	  where ok is TRUE if all is well, -n if not,
		areaid is the area id to be disconnected,
		cssocket is the socket number					*/

LOGICAL v4is_DisconnectAreaId(areaid,cssocket)
  int areaid ;
  int cssocket ;
{
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__AreaCB *acb ;
  struct V4CS__RemoteFileRef *rfr ;
  int cnt,i,fx,sx ;

#ifdef SOCKETS
#ifdef NEWMMM
needwork;
#else
	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
#endif
	if (mmm->rfr == NULL)					/* Maybe have to allocate ? */
	 mmm->rfr = (struct V4CS__RemoteFileRef *)v4mm_AllocChunk(sizeof *rfr,TRUE) ;
	rfr = mmm->rfr ;
	for(cnt=0,i=0;i<mmm->NumAreas;i++)			/* See how many areas on this socket */
	 { if (mmm->Areas[i].AreaId == UNUSED) continue ;
	   acb = (struct V4IS__AreaCB *)mmm->Areas[i].AreaCBPtr ;
	   if (acb->CSaid >= 0 && acb->CSSocket == cssocket) cnt++ ; /* Got another area on socket */
	 } ;
	if (cnt > 0) return(TRUE) ;				/* Areas still open on socket - leave alone */
	for(fx=0;fx<rfr->FileRefCount;fx++)			/* Reset all filerefs on this socket */
	 { if (rfr->FileRefs[fx].Socket != cssocket) continue ;
	   rfr->FileRefs[fx].Socket = UNUSED ;
	 } ;
	for(sx=0;sx<rfr->ServerCount;sx++)			/* Find in Server list & close socket */
	 { if (rfr->Servers[sx].Socket != cssocket) continue ;
	   SOCKETCLOSE(cssocket) ;
	   rfr->Servers[sx] = rfr->Servers[--rfr->ServerCount] ;
	   break ;
	 } ;
	return(TRUE) ;
#else
	return(FALSE) ;
#endif
}
#endif	/* End of #ifdef V4_BUILD_CS_FUNCTIONALITY */

/*	V A X  /   V M S   I N T E R F A C E   R O U T I N E S		*/

#ifdef VAXVMSxx
/*	v4vms_Open - Opens file for block mode access			*/
/*	Call: rabp = v4vms_Open( filename , update , create )
	  where rabp is pointer to file's rab,
		filename is the file to open,
		update is TRUE to open for update, FALSE for read-only,
		create is TRUE to create area				*/

v4vms_Open(filename,update,create)
  char *filename ;
  int update,create ;
{ struct FAB *fabp ; struct RAB *rabp ; struct NAM *namp ;
  int res ;
  char xnam[V_FileName_Max] ; char errbuf[250] ;

	fabp = v4mm_AllocChunk(sizeof(*fabp)) ; *fabp = cc$rms_fab ;
	rabp = v4mm_AllocChunk(sizeof(*rabp)) ; *rabp = cc$rms_rab ;
	namp = v4mm_AllocChunk(sizeof(*namp)) ; *namp = cc$rms_nam ;
	namp->nam$l_rsa = &xnam ; namp->nam$b_rss = sizeof(xnam) ;
	rabp->rab$l_fab = fabp ; fabp->fab$l_nam = namp ;

	fabp->fab$b_fac = FAB$M_GET | FAB$M_BIO ;
	if (update) fabp->fab$b_fac |= (FAB$M_UPD | FAB$M_PUT | FAB$M_DEL) ;

	fabp->fab$l_fna = filename ; fabp->fab$b_fns = strlen(filename) ;

	fabp->fab$w_mrs = 512 ;
	fabp->fab$b_org = FAB$C_SEQ ;
	fabp->fab$l_fop = 0 ;
	fabp->fab$b_rfm = FAB$C_FIX ;

	fabp->fab$b_shr = FAB$M_UPI ;

	if (create) { res = sys$create(fabp) ; }
	 else { res = sys$open(fabp) ; } ;
	if ((res & 1) == 0) v4_error(V4E_RMSOPEN,0,"V4VMS","Open","RMSOPNERR","RMS Open Error: %s",mscu_vms_errmsg(res,errbuf)) ;
	res = sys$connect(rabp) ;
	if ((res & 1) == 0) v4_error(V4E_RMSCONNECT,0,"V4VMS","Open","RMSCONERR","RMS Connect Error: %s",mscu_vms_errmsg(res,errbuf)) ;

	xnam[namp->nam$b_rsl] = 0 ; strcpy(filename,xnam) ;

	return(rabp) ;
}

/*	v4vms_Close - Close an area					*/

v4vms_Close(rabp)
  struct RAB *rabp ;
{ int res ; char errbuf[250] ;

	res = sys$close(rabp->rab$l_fab) ;
	if ((res & 1) == 0) v4_error(V4E_RMSCLOSE,0,"V4VMS","Close","RMSCLOSERR","RMS Close Error: %s",mscu_vms_errmsg(res,errbuf)) ;
	v4mm_FreeChunk(rabp->rab$l_fab->fab$l_nam) ; v4mm_FreeChunk(rabp->rab$l_fab) ;
	v4mm_FreeChunk(rabp) ;
	return ;
}

/*	v4vms_Put - Put a bucket				*/

v4vms_Put(rabp,bktptr,bktnum,bktsize)
  struct RAB *rabp ;
  char *bktptr ;
  int bktnum,bktsize ;
{ int vbn,res ; char errbuf[250] ;

	vbn = (bktnum * bktsize) / 512 ;		/* Convert from V4 bucket to RMS 512 bucket */
	vbn++ ;						/* 1 is first bucket */
	rabp->rab$l_bkt = vbn ;
	rabp->rab$l_rbf = bktptr ;
	rabp->rab$w_rsz = bktsize ;

	res = sys$write(rabp) ;
	if ((res & 1) == 0) v4_error(V4E_RMSPUT,0,"V4VMS","Put","RMSPUTERR","RMS Write Error: %s",mscu_vms_errmsg(res,errbuf)) ;
}

/*	v4vms_Get - Get a bucket				*/

v4vms_Get(rabp,bktptr,bktnum,bktsize)
  struct RAB *rabp ;
  char *bktptr ;
  int bktnum,bktsize ;
{ int vbn,res ; char errbuf[250] ;

	vbn = (bktnum * bktsize) / 512 ;		/* Convert from V4 bucket to RMS 512 bucket */
	vbn++ ;						/* 1 is first bucket */
	rabp->rab$l_bkt = vbn ;
	rabp->rab$l_ubf = bktptr ;
	rabp->rab$w_usz = bktsize ;

	res = sys$read(rabp) ;
	if ((res & 1) == 0) v4_error(V4E_RMSGET,0,"V4VMS","Get","RMSGETERR","RMS Read Error: %s",mscu_vms_errmsg(res,errbuf)) ;
}
#endif
