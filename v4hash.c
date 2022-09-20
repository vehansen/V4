/*	V4HASH.C - Hash Modules for V4

	Created 21-Nov-94 by Victor E. Hansen		*/

#ifndef NULLID
#include "v4defs.c"
#endif
#include <time.h>

#ifdef WINNT
#include <windows.h>
#endif

#define IFTRACEAREA if (acb->RootInfo->NextAvailUserNum == 5)
extern struct V4C__ProcessInfo *gpi ;	/* Global process information */

//extern struct V4MM__MemMgmtMaster *V4_GLOBAL_MMM_PTR ;		/* Point to master area structure */

/*	Set up some abbreviations to make code more readable/managable	*/

#define LOCKROOT lrx = v4mm_LockSomething(pcb->AreaId,V4MM_LockId_Root,V4MM_LockMode_Write,V4MM_LockIdType_Root,10,NULL,7)
#define RELROOT(WHERE) v4mm_LockRelease(pcb->AreaId,lrx,mmm->MyPid,pcb->AreaId,WHERE) ; v4mm_TempLock(pcb->AreaId,0,FALSE)

/*	v4h_PutHash - Writes out a Hash Entry			*/
/*	Call: dataid = v4h_PutHash( pcb , acb , keyptr , bufptr , hashmode )
	  where dataid is dataid of resulting record,
	  	pcb is pointer to parameter control block,
	  	pointer to area control block,
	  	keyptr is pointer to user key,
	  	bufptr is buffer to be written (length from hashinfo),
	  	hashmode is how to write - V4H_PutHash_xxx	*/

int v4h_PutHash(pcb,acb,keyptr,bufptr,hashmode)
  struct V4IS__ParControlBlk *pcb ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__Key *keyptr ;
  char *bufptr ;
  int hashmode ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__GlobalBuf *glb ;
  struct V4H__FixHash *vfh ;
  struct V4H__HashEntry vhe,*vhe1 ;
  int *intptr,ax,dataid,i,lrx ; int hash,ht ;
  char *bucketptr ;

	FINDAREAINPROCESS(ax,pcb->AreaId)
	if (ax == UNUSED) return(UNUSED) ;			/* Could not find area? */
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++)
//	 { if (mmm->Areas[ax].AreaId == pcb->AreaId) break ; } ;
//	if (ax >= mmm->NumAreas) return(UNUSED) ;			/* Could not find area? */

	if (mmm->Areas[ax].SegId != 0)				/* Global area ? */
	 { if ( GETGLB(ax) == NULL)
	    v4_error(V4E_AREANOTGLOBAL,0,"V4MM","AreaCheckPoint","AREANOTGLOBAL","Area (%d %s) not global",ax,UCretASC(mmm->Areas[ax].UCFileName)) ;
	   bucketptr = (char *)&glb->Buffer[glb->BktOffset] ;
	 } else
	 { bucketptr = (char *)mmm->Bkts[0].BktPtr ;
	 } ;
	vfh = (struct V4H__FixHash *)acb->HashInfo ;
	switch (vfh->HashType)
	 { default: v4_error(V4E_INVHASHMODE,0,"V4H","PutHash","INVHASHMODE","Invalid hash mode (%d)",vfh->HashType) ;
	   case V4H_HashType_Fixed:		/* Fixed record, simple hash file */
		switch (vfh->HashFunction)
		 { default: v4_error(V4E_INVHASHFUNC,0,"V4H","PutHash","INVHASHFUNC","Invalid hash function (%d)",vfh->HashFunction) ;
		   case V4H_HashFunc_Int1:
			hash = 0 ; ht = 0 ; intptr = (int *)bufptr + vfh->HashKeyOffset ;
			for(i=0;i<vfh->HashKeyBytes;i+=(sizeof (int)))
			 { hash += (*intptr + ht) ; ht = (ht ^ hash) ;
			   intptr++ ;
			 } ;
		   case V4H_HashFunc_Alpha1:
		   	break ;
		 } ;
		vhe.Hash = hash & 0x7fffffff ;		/* Build up hash entry */
		if (vhe.Hash == 0) vhe.Hash = 1 ;	/* Can't have 0 */
		memcpy(vhe.EntryBuf,bufptr,vfh->RecordBytes) ;
		dataid = (vhe.Hash % vfh->MaxRecords) ;	/* Dataid = position in hash table */
		vhe1 = (struct V4H__HashEntry *)((char *)bucketptr + (vfh->EntryBytes * dataid)) ;
		LOCKROOT ;
	   	switch (hashmode)
	   	 { case V4H_PutHash_Insert:
	   	   case V4H_PutHash_Update:
	   	   case V4H_PutHash_Write:
			for(;;dataid=((++dataid) % vfh->MaxRecords))
			 { vhe1 = (struct V4H__HashEntry *)((char *)bucketptr + (vfh->EntryBytes * dataid)) ;
			   if (vhe1->Hash == 0)
			    { if (hashmode == V4H_PutHash_Update)
			       v4_error(V4E_NORECORDUPD,0,"V4H","HashPut","NORECORDUPD","No record found in area (%s) to update",UCretASC(pcb->UCFileName)) ;
			      break ;
			    } ;
			   if (vhe.Hash != vhe1->Hash) continue ;
			   if (memcmp(vhe1->EntryBuf+vfh->HashKeyOffset,bufptr+vfh->HashKeyOffset,vfh->HashKeyBytes) == 0) break ;
			 } ;
			if (vhe1->Hash != 0 && hashmode == V4H_PutHash_Insert)
			 v4_error(V4E_INSRECEXISTS,0,"V4H","HashPut","INSRECEXISTS","Record already exists with specified key") ;
	   	   case V4H_PutHash_Cache:
	   	   	if (vhe1->Hash == 0)
	   	   	 { if (acb->RootInfo->RecordCount >= (vfh->MaxRecords*100)/(100-vfh->FillPercent))
	   	   	    v4_error(V4E_MAXRECORD,0,"V4H","HashPut","MAXRECORD","Area (%s) at record max (%d), Records=%d at %d fill percent",
	   	   	    		UCretASC(pcb->UCFileName),(vfh->MaxRecords*100)/(100-vfh->FillPercent),vfh->MaxRecords,vfh->FillPercent) ;
	   	   	   acb->RootInfo->RecordCount++ ;
	   	   	 } ;
			memcpy(vhe1,&vhe,vfh->EntryBytes) ;	/* Just copy (over) */
			break ;
		   case V4H_PutHash_Delete:
			for(;;dataid=((++dataid) % vfh->MaxRecords))
			 { vhe1 = (struct V4H__HashEntry *)((char *)bucketptr + (vfh->EntryBytes * dataid)) ;
			   if (vhe1->Hash == 0)
			    v4_error(V4E_NORECORDDEL,0,"V4H","HashPut","NORECORDDEL","No record found in area (%s) to delete",UCretASC(pcb->UCFileName)) ;
			   if (vhe.Hash != vhe1->Hash) continue ;
			   if (memcmp(vhe1->EntryBuf+vfh->HashKeyOffset,bufptr+vfh->HashKeyOffset,vfh->HashKeyBytes) == 0) break ;
			 } ;
	   	   	if (vhe1->Hash != 0) acb->RootInfo->RecordCount-- ;
			vhe1->Hash = 0 ;			/* Zapping hash code deletes the entry */
			break ;
		   case V4H_PutHash_Reset:
			memset(bucketptr,0,acb->RootInfo->BktSize-glb->BktOffset) ;
			acb->RootInfo->RecordCount = 0 ;
		   	break ;
	   	 } ;
		v4mm_BktUpdated(pcb->AreaId,0) ;		/* Mark root bucket as updated */
	   	RELROOT("PutHash") ;
		return(dataid) ;
	 } ;
	return(UNUSED) ;	/* Should not get here */
}

/*	v4h_GetHashKey - Gets a record based on hash key	*/
/*	Call: ok = v4h_GetHashKey( pcb , acb , bufptr , buflen , keyptr , mode )
	  where ok is TRUE if bufptr updated with data, FALSE if too small,
	  	pcb is parameter control block,
	  	acb is area control block,
	  	bufptr/buflen is user space to be updated,
	  	keyptr is pointer to key,
	  	mode is get mode				*/

LOGICAL v4h_GetHashKey(pcb,acb,bufptr,buflen,keyptr,mode)
  struct V4IS__ParControlBlk *pcb ;
  struct V4IS__AreaCB *acb ;
  char *bufptr ; int buflen ;
  struct V4IS__Key *keyptr ;
  int mode ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__GlobalBuf *glb ;
  struct V4H__FixHash *vfh ;
  struct V4H__HashEntry *vhe1 ;
  int *intptr,ax,dataid,i ; int hash,ht ;
  char *bucketptr ;

	FINDAREAINPROCESS(ax,pcb->AreaId)
	if (ax == UNUSED) return(FALSE) ;			/* Could not find area? */
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++)
//	 { if (mmm->Areas[ax].AreaId == pcb->AreaId) break ; } ;
//	if (ax >= mmm->NumAreas) return(FALSE) ;		/* Could not find area? */

	if (mmm->Areas[ax].SegId != 0)				/* Global area ? */
	 { if ( GETGLB(ax) == NULL)
	    v4_error(V4E_AREANOTGLOBAL,0,"V4MM","AreaCheckPoint","AREANOTGLOBAL","Area (%d %s) not global",ax,UCretASC(mmm->Areas[ax].UCFileName)) ;
	   bucketptr = (char *)&glb->Buffer[glb->BktOffset] ;
	 } else
	 { bucketptr = (char *)mmm->Bkts[0].BktPtr ;
	 } ;
	vfh = (struct V4H__FixHash *)acb->HashInfo ;
	switch (vfh->HashType)
	 { default: v4_error(V4E_INVHASHMODE,0,"V4H","PutHash","INVHASHMODE","Invalid hash mode (%d)",vfh->HashType) ;
	   case V4H_HashType_Fixed:		/* Fixed record, simple hash file */
		switch (vfh->HashFunction)
		 { default: v4_error(V4E_INVHASHFUNC,0,"V4H","GetHash","INVHASHFUNC","Invalid hash function (%d)",vfh->HashFunction) ;
		   case V4H_HashFunc_Int1:
			hash = 0 ; ht = 0 ; intptr = (int *)bufptr + vfh->HashKeyOffset ;
			for(i=0;i<vfh->HashKeyBytes;i+=(sizeof (int)))
			 { hash += (*intptr + ht) ; ht = (ht ^ hash) ;
			   intptr++ ;
			 } ;
		   case V4H_HashFunc_Alpha1:
		   	break ;
		 } ;
		hash = hash & 0x7fffffff ;		/* Build up hash entry */
		dataid = (hash % vfh->MaxRecords) ;	/* Dataid = position in hash table */
		for(;;dataid=((++dataid) % vfh->MaxRecords))
		 { vhe1 = (struct V4H__HashEntry *)((char *)bucketptr + (vfh->EntryBytes * dataid)) ;
		   if (vhe1->Hash == 0)
		    v4_error(V4E_NORECORDGET,0,"V4H","GET","NORECORDGET","Record not found in area (%s)",UCretASC(pcb->UCFileName)) ;
		   if (hash != vhe1->Hash) continue ;
		   if (memcmp(vhe1->EntryBuf+vfh->HashKeyOffset,bufptr+vfh->HashKeyOffset,vfh->HashKeyBytes) == 0) break ;
		 } ;
/*		Return to v4is_Get() - final cleanup & locking will take place there */
		return(v4is_CopyData(pcb->AreaId,bufptr,buflen,vhe1->EntryBuf,vfh->RecordBytes,V4IS_DataCmp_None,pcb,mode)) ;
	 } ;
	return(FALSE) ;
}

/*	v4h_GetHashDataId - Get hash record based on hash code	*/
/*	Call: ok = v4h_GetHashDataId( pcb , acb , bufptr , buflen , keyptr , mode )
	  where ok is TRUE if bufptr updated with data, FALSE if too small,
	  	pcb is parameter control block,
	  	acb is area control block,
	  	bufptr/buflen is user space to be updated,
	  	keyptr is pointer to key,
	  	mode is get mode				*/

LOGICAL v4h_GetHashDataId(pcb,acb,bufptr,buflen,keyptr,mode)
  struct V4IS__ParControlBlk *pcb ;
  struct V4IS__AreaCB *acb ;
  char *bufptr ; int buflen ;
  struct V4IS__Key *keyptr ;
  int mode ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__GlobalBuf *glb ;
  struct V4H__FixHash *vfh ;
  struct V4H__HashEntry *vhe1 ;
  int ax ;
  char *bucketptr ;

	FINDAREAINPROCESS(ax,pcb->AreaId)
	if (ax == UNUSED) return(FALSE) ;			/* Could not find area? */
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++)
//	 { if (mmm->Areas[ax].AreaId == pcb->AreaId) break ; } ;
//	if (ax >= mmm->NumAreas) return(FALSE) ;		/* Could not find area? */

	if (mmm->Areas[ax].SegId != 0)				/* Global area ? */
	 { if ( GETGLB(ax) == NULL)
	    v4_error(V4E_AREANOTGLOBAL,0,"V4MM","AreaCheckPoint","AREANOTGLOBAL","Area (%d %s) not global",ax,UCretASC(mmm->Areas[ax].UCFileName)) ;
	   bucketptr = (char *)&glb->Buffer[glb->BktOffset] ;
	 } else
	 { bucketptr = (char *)mmm->Bkts[0].BktPtr ;
	 } ;
	vfh = (struct V4H__FixHash *)acb->HashInfo ;
	switch (vfh->HashType)
	 { default: v4_error(V4E_INVHASHMODE,0,"V4H","PutHash","INVHASHMODE","Invalid hash mode (%d)",vfh->HashType) ;
	   case V4H_HashType_Fixed:		/* Fixed record, simple hash file */
		vhe1 = (struct V4H__HashEntry *)((char *)bucketptr + (vfh->EntryBytes * pcb->DataId)) ;
		if (vhe1->Hash != 0)
		 return(v4is_CopyData(pcb->AreaId,bufptr,buflen,vhe1->EntryBuf,vfh->RecordBytes,V4IS_DataCmp_None,pcb,mode)) ;
		v4_error(V4E_NODATAID,0,"V4H","Get","NODATAID","No such hash entry in area (%s) with dataid (%x)",UCretASC(pcb->UCFileName),pcb->DataId) ;
	 } ;
	return(FALSE) ;
}

/*	v4h_CheckPoint - Force write of entry just updated	*/
/*	Call: v4h_CheckPoint( pcb , acb )
	  where pcb is parameter control block,
	  	acb is area control block			*/

void v4h_CheckPoint(pcb,acb)
  struct V4IS__ParControlBlk *pcb ;
  struct V4IS__AreaCB *acb ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__GlobalBuf *glb ;
  struct V4H__FixHash *vfh ;
  struct V4H__HashEntry *vhe1 ;
  int ax ;
  char *bucketptr ;

	FINDAREAINPROCESS(ax,pcb->AreaId)
	if (ax == UNUSED) return ;			/* Could not find area? */
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++)
//	 { if (mmm->Areas[ax].AreaId == pcb->AreaId) break ; } ;
//	if (ax >= mmm->NumAreas) return ;			/* Could not find area? */

	if (mmm->Areas[ax].SegId != 0)				/* Global area ? */
	 { if ( GETGLB(ax) == NULL)
	    v4_error(V4E_AREANOTGLOBAL,0,"V4MM","AreaCheckPoint","AREANOTGLOBAL","Area (%d %s) not global",ax,UCretASC(mmm->Areas[ax].UCFileName)) ;
	   bucketptr = (char *)&glb->Buffer[glb->BktOffset] ;
	 } else
	 { bucketptr = (char *)mmm->Bkts[0].BktPtr ;
	 } ;
	vfh = (struct V4H__FixHash *)acb->HashInfo ;
	switch (vfh->HashType)
	 { default: v4_error(V4E_INVHASHMODE,0,"V4H","PutHash","INVHASHMODE","Invalid hash mode (%d)",vfh->HashType) ;
	   case V4H_HashType_Fixed:		/* Fixed record, simple hash file */
		vhe1 = (struct V4H__HashEntry *)((char *)bucketptr + (vfh->EntryBytes * pcb->DataId)) ;
	 } ;
}
