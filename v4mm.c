/*	V4MM.C - Memory Management Modules for V4

	Created 1-Apr-92 by Victor E. Hansen		*/

#ifndef NULLID
#include "v4defs.c"
#endif
#ifdef POSIX
#include <signal.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#endif
#include <time.h>
#ifdef WINNT
CRITICAL_SECTION wincs ;	/* Used to provide lockout for multi-threaded processes (e.g. V4SERVER) */
#endif
#ifdef VMSOS
#include rms
#endif

//struct V4MM__MemMgmtMaster *V4_GLOBAL_MMM_PTR ;
int v4mm_GlobalBufGrab( /* int , int */ ) ;
struct V4MM__GlobalBuf *v4mm_GetGLB(/* ax */) ;
extern struct V4C__ProcessInfo *gpi ;	/* Global process information */

typedef struct V4IS__IndexBktKeyList IBKL ;
typedef struct V4IS__IndexKeyDataEntry IKDE ;
typedef struct V4IS__IndexBktHdr IBH ;
typedef struct V4IS__DataBktHdr DBH ;

#define xLOCKALLOC(src) lkax = v4mm_LockSomething(areaid,V4MM_LockId_Alloc,V4MM_LockMode_Write,V4MM_LockIdType_Alloc,01,NULL,src)
#define xRELALLOC if (lkax >= 0) { v4mm_LockRelease(areaid,lkax,mmm->MyPid,areaid,"xRELALLOC") ; lkax = UNUSED ; }

#define LOCKALLOC(src) lkax = v4mm_LockSomething(areaid,V4MM_LockId_Alloc,V4MM_LockMode_Write,V4MM_LockIdType_Alloc,01,NULL,src) ;\
	if (lkax < 1) printf("?? LOCKALLOC returns -1 from %d\n",src)
#define RELALLOC(where) if (lkax >= 0) { v4mm_LockRelease(areaid,lkax,mmm->MyPid,areaid,where) ; lkax = UNUSED ; } \
	else { printf("?? RELALLOC at -1 from %s\n",where) ; }

#define LOCKLT(locnum) \
if (lt->ActivePid == mmm->MyPid) { lt->NestLockCount++ ; } \
 else { int upidcnt ; \
	for(tx=0,upidcnt=0;;tx++) \
	 { if (GETSPINLOCK(&lt->IsActive)) break ; \
	   if (lt->ActivePid == UNUSEDPID) \
	    { upidcnt++ ;  if (upidcnt > 25) { RLSSPINLOCK(lt->IsActive) ; continue ; } ; \
	      HANGLOOSE(1) ; continue ; \
	    } ; upidcnt = 0 ; \
	   HANGLOOSE((lt->LastUniqueLockId % 5)+1) ; if ((tx % 500) != 1) continue ; \
	   if (PROCESS_GONE(lt->ActivePid)) { lt->ActivePid = UNUSEDPID ; RLSSPINLOCK(lt->IsActive) ; continue ; } ; \
	   if (tx > 1000) \
	    { tx = (int)lt->ActivePid ; lt->ActivePid = UNUSEDPID ; RLSSPINLOCK(lt->IsActive) ; \
	      sprintf(ebuf,"Area (%d %s) Lock table appears hung by process (%d)",ax,UCretASC(mmm->Areas[ax].UCFileName),tx) ; \
	      v4is_BugChk(areaid,ebuf) ; tx = 0 ; continue ; \
	    } ; \
	 } ; \
	if (lt->ActivePid != UNUSEDPID) \
	 { sprintf(ebuf,"LOCKLT(%d) - ActivePid = %d, NestLockCount = %d, IsActive = %lld", \
			locnum,lt->ActivePid,lt->NestLockCount,(B64INT)lt->IsActive) ; v4is_BugChk(areaid,ebuf) ; \
	 } ; \
	lt->ActivePid = mmm->MyPid ; lt->NestLockCount = 0 ; \
      } ;
#define RELLT(SRC) \
   if (lt->ActivePid != mmm->MyPid) \
    { sprintf(ebuf,"?? RELLT(%d) releasing lock table locked by pid=%d Nest=%d\n",SRC,lt->ActivePid,lt->NestLockCount) ; \
      v4is_BugChk(areaid,ebuf) ; \
      /* v4_error(V4E_BADPID,0,"V4MM","RELLT","BADPID",ebuf) ; */ \
    } else { if (--lt->NestLockCount < 0) { lt->ActivePid = UNUSEDPID ; RLSSPINLOCK(lt->IsActive) ; } ; \
	 } ;
#define RELLTE(SRC) \
   if (lt->ActivePid != mmm->MyPid) { printf("?? RELLT(%d) releasing lock table locked by pid=%d\n",SRC,lt->ActivePid) ; } \
    else { lt->NestLockCount = 0 ; lt->ActivePid = UNUSEDPID ; RLSSPINLOCK(lt->IsActive) ; } ;


/*	v4mm_MemMgmtInit - Allocates & Initializes Memory Management Modules	*/
/*	Call: mmmNew = v4mm_MemMgmtInit(mmm)
	  where	mmm is pointer to (new) mmm,
		mmm is pointer to mmm to init or NULL to init master		*/


struct V4MM__MemMgmtMaster *v4mm_MemMgmtInit(mmm)
  struct V4MM__MemMgmtMaster *mmm ;
{ 
  void v4mm_spurious_alarm_handler() ;
#ifdef segshare
#ifdef UNIX
  struct sigaction act ;
#endif
#endif
#ifdef WINNT
  FILETIME ftCreate,ftExit,ftKernel,ftUser ;
#endif
  INDEX ax,bx ;

//	if (V4_GLOBAL_MMM_PTR != NULL) return(V4_GLOBAL_MMM_PTR) ;	/* Already set up! */
	if (mmm == NULL)
	 { mmm = (struct V4MM__MemMgmtMaster *)v4mm_AllocChunk(sizeof *mmm,FALSE) ;
	   gpi->mmm = mmm ;
#ifdef WINNT
	   gpi->mmmTLS = TlsAlloc() ; TlsSetValue(gpi->mmmTLS,mmm) ;
#endif
	 } ;
	mmm->NumAreas = 0 ; mmm->NumBkts = 0 ; mmm->TotalCalls = 0 ;
	mmm->rfr = NULL ;
#ifdef POSIX
	mmm->MyPid = CURRENTPID ;
#if defined UNIX && defined segshare
	act.sa_handler = v4mm_spurious_alarm_handler ;		/* Set up handler for ALRM signal */
	sigemptyset(&act.sa_mask) ;
	sigaddset(&act.sa_mask,SIGALRM) ;			/* Create signal set to block SIGALRM during call to handler */
	sigaction(SIGALRM,&act,NULL) ;				/* Tell POSIX */
#endif
#endif
#ifdef WINNT
	mmm->MyPid = WINNT_PID ;
	GetThreadTimes(GetCurrentThread(),&ftCreate,&ftExit,&ftKernel,&ftUser) ;
	mmm->MyPidLoginDT = ftCreate.dwLowDateTime ;
	mmm->MyidProcess = CURRENTPID ;
	if (!DuplicateHandle(GetCurrentProcess(),GetCurrentThread(),GetCurrentProcess(),&mmm->MyhThread,THREAD_ALL_ACCESS,TRUE,0))
	 v4_error(V4E_DUPTHREADHANDLE,0,"V4MM","MMMInit","DUPTHREADHANDLE","Error (%s) duplicating current thread handle",v_OSErrString(GetLastError())) ;
	InitializeCriticalSection(&wincs) ;
#endif
	for(ax=0;ax<V4MM_Area_Max;ax++) { mmm->Areas[ax].AreaId = UNUSED ; mmm->Areas[ax].abi = NULL ; } ;
	for(bx=0;bx<V4MM_AreaBkt_Max;bx++) { mmm->Bkts[bx].BktPtr = NULL ; } ;
	return(mmm) ;
}

/*	v4mm_MemMgmtShutdown - Release memory associated with mmm */

void v4mm_MemMgmtShutdown(mmm)
  struct V4MM__MemMgmtMaster *mmm ;
{ 
  int i ;

//	mmm = V4_GLOBAL_MMM_PTR ;
	if (mmm == NULL) return ;
	for(i=0;i<mmm->NumBkts;i++)
	 { if (mmm->Bkts[i].BktPtr != NULL) v4mm_FreeChunk(mmm->Bkts[i].BktPtr) ;
	 } ;
	v4mm_FreeChunk(mmm) ;
/*	Now go thru master process mmm & reset any areas back to the master */
//#ifdef NEWMMM
//	for(i=0;i<V4MM_Area_Max;i++)
//	 { if (gpi->Areas[i].mmm == mmm) gpi->Areas[i].mmm = gpi->mmm ;
//	 } ;
//#endif
}

#ifdef NEWMMM
/*	v4mm_copyToNewMMM - Moves aid (and file) to new, private mmm for multi-threaded environments	*/
/*	Call: ok = v4mm_copyToNewMMM( mmmSrc , mmmDst , aid , linkSrc )
	  where ok is TRUE if OK, FALSE if problem,
		mmmSrc is source (usually gpi->mmm) mmm,
		mmmDst is destination (private) mmm,
		aid is areaId of file,
		linkSrc is TRUE if really want to link back to mmmSrc					*/

LOGICAL v4mm_copyToNewMMM(mmmSrc,mmmDst,aid,linkSrc)
  struct V4MM__MemMgmtMaster *mmmSrc ;
  struct V4MM__MemMgmtMaster *mmmDst ;
  AREAID aid ;
  LOGICAL linkSrc ;
{
  INDEX ax,bx,nbx ;
  
	for(ax=0;ax<V4MM_Area_Max;ax++) { if (mmmSrc->Areas[ax].AreaId == aid) break ; } ;
	if (ax >= V4MM_Area_Max) return(FALSE) ;
	for(bx=0;bx<mmmSrc->NumBkts;bx++)	/* Look for root bucket in src */
	 { if (mmmSrc->Bkts[bx].AreaIndex == ax && mmmSrc->Bkts[bx].BktNum == 0) break ; } ;
	if (bx >= mmmSrc->NumBkts) return(FALSE) ;
	mmmDst->Areas[ax] = mmmSrc->Areas[ax] ;	/* Copy the area into new */
	if (linkSrc) mmmDst->Areas[ax].mmmReal = mmmSrc ;	/* Maybe add link back to src mmm (for V4 work area in multi-thread environment) */
	if (ax >= mmmDst->NumBkts) mmmDst->NumBkts = (ax + 1) ;
	mmmDst->Areas[ax].FreeBkt1 = NULL ; mmmDst->Areas[ax].FreeBkt2 = NULL ;
	mmmDst->Areas[ax].NumBkts = 1 ;		/* Only want root bucket in new mmm */
	for(nbx=0;nbx<V4MM_AreaBkt_Max;nbx++) { if (mmmDst->Bkts[nbx].BktPtr == NULL) break ; } ;
	if (nbx >= mmmDst->NumBkts) mmmDst->NumBkts = (nbx + 1) ;
	mmmDst->Bkts[nbx] = mmmSrc->Bkts[bx] ;
/*	Flip pointer in master process Area table to new mmm */
//	gpi->Areas[ax].mmm = mmmDst ;
	return(TRUE) ;
}
#endif

/*	v4mm_GetMMMPtr - Function to return pointer to current V4_GLOBAL_MMM_PTR	*/
/*	Call: ptr = v4mm_GetMMMPtr()							*/

struct V4MM__MemMgmtMaster *v4mm_GetMMMPtr()
{
	return(gpi->mmm) ;
}

/*	v4mm_SetUserJobId - Sets users/application job id for mmm	*/
/*	Call: oldjobid = v4mm_SetUserJobId( newjobid )			*/

int v4mm_SetUserJobId(newid,mmm)
  int newid ;
  struct V4MM__MemMgmtMaster *mmm ;
{ 
  int oldid ;

//	mmm = v4mm_MemMgmtInit(NULL) ;
	oldid = mmm->UserJobId ; mmm->UserJobId = newid ;
	return(oldid) ;
}

/*	v4mm_spurious_alarm_handler - Handles (via ignore) spurious alarm signals	*/
void v4mm_spurious_alarm_handler()
{
#ifdef segshare
#ifdef UNIX
  struct sigaction act ;

	act.sa_handler = v4mm_spurious_alarm_handler ;		/* Set up handler for ALRM signal */
	sigemptyset(&act.sa_mask) ;
	sigaddset(&act.sa_mask,SIGALRM) ;			/* Create signal set to block SIGALRM during call to handler */
	sigaction(SIGALRM,&act,NULL) ;				/* Tell POSIX */
#endif
#endif
}

/*	v4mm_MakeNewAId - Makes/Allocates new Area	*/
/*	Call: newaid = v4mm_MakeNewAid( acb , bktsize , fileptr , filename )
	  where newaid is Area Id,
		acb is point to Area Control Block,
		bktsize is default bucket size,
		fileptr is pointer to (Unix) file,
		filename is pointer to file name */

int v4mm_MakeNewAId(acb,bktsize,fileptr,filename)
  struct V4IS__AreaCB *acb ;
  int bktsize ;
#ifdef WINNT
  HANDLE fileptr ;
#else
  FILE *fileptr ;
#endif
  UCCHAR *filename ;
{ struct V4MM__MemMgmtMaster *mmm ;
  int ax ;

	if( (mmm = gpi->mmm) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
#ifdef WINNT
	EnterCriticalSection(&wincs) ;		/* Only one thread at a time thru here! */
#endif
/*	Look for free slot */
	for(ax=0;ax<mmm->NumAreas;ax++) { if (mmm->Areas[ax].AreaId == UNUSED) break ; } ;
	if (ax >= mmm->NumAreas)
	 { if (mmm->NumAreas >= V4MM_Area_Max)
	    { printf("mmm->NumAreas = %d, Max = %d\n",mmm->NumAreas,V4MM_Area_Max) ;
	      for(ax=0;ax<mmm->NumAreas;ax++) { printf(" %d %d %s\n",ax,mmm->Areas[ax].AreaId,UCretASC(mmm->Areas[ax].UCFileName)) ; } ;
	      v4_error(V4E_TOMNYAREAS,0,"V4MM","MakeNewAId","TOMNYAREAS","Too many AREAS (%d) already in place",V4MM_Area_Max) ;
	    } ;
	   ax = mmm->NumAreas++ ;
	 } ;
	memset(&mmm->Areas[ax],0,sizeof(mmm->Areas[ax])) ;
	mmm->Areas[ax].AreaId = ax ; acb->AreaId = ax ;
	mmm->Areas[ax].AreaCBPtr = acb ;
	mmm->Areas[ax].BktSize = bktsize ;
	mmm->Areas[ax].MinBkts = 3 ; mmm->Areas[ax].MaxBkts = 5 ; mmm->Areas[ax].NumBkts = 0 ;
	mmm->Areas[ax].FreeBkt1 = NULL ; mmm->Areas[ax].FreeBkt2 = NULL ; mmm->Areas[ax].mmmReal = NULL ; mmm->Areas[ax].abi = NULL ;
#ifdef V4ENABLEMULTITHREADS
	INITMTLOCK(mmm->Areas[ax].AreaCBPtr->areaLock) ;		/* Multi-thread lock */
#endif
//#ifdef NEWMMM
//	gpi->Areas[ax].mmm = mmm ;
//	gpi->Areas[ax].aid = ax ;
//	gpi->Areas[ax].acb = acb ;
//#endif
#ifdef WINNT
	mmm->Areas[ax].hfile = (HANDLE)fileptr ;
#else
	mmm->Areas[ax].FilePtr = fileptr ;
#endif
	mmm->Areas[ax].SegId = 0 ;			/* No sharing (yet) */
	mmm->Areas[ax].GBPtr = NULL ;
	mmm->Areas[ax].LockIDX = UNUSED ; mmm->Areas[ax].LockDAT = UNUSED ; mmm->Areas[ax].LockRec = UNUSED ;
	mmm->Areas[ax].LockTree = UNUSED ; mmm->Areas[ax].LockExtra = UNUSED ;
	UCstrcpy(mmm->Areas[ax].UCFileName,filename) ;
#ifdef WINNT
	LeaveCriticalSection(&wincs) ;		/* Only one thread at a time thru here! */
#endif
	return(mmm->Areas[ax].AreaId) ;
}

/*	v4mm_AreaSegId - Returns global segment ID for an area		*/
/*	Call: segid = v4mm_AreaSegId( areaid )
	  where segid is global segment id (0 if none),
		areaid is area id					*/

SEGID v4mm_AreaSegId(areaid)
  int areaid ;
{ struct V4MM__MemMgmtMaster *mmm ;
  int ax ;

	FINDAREAINPROCESS(ax,areaid)
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++) { if (mmm->Areas[ax].AreaId == areaid) break ; } ;
	if (ax == UNUSED) return(0) ;			/* Could not find area? */

	return(mmm->Areas[ax].SegId) ;
}

/*	v4mm_AreaCheckPoint - CheckPoints all buckets in an area		*/
/*	Call: v4mm_AreaCheckPoint( areaid )
	  where areaid is area id to be flushed			*/

void v4mm_AreaCheckPoint(areaid)
  int areaid ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__AreaCB *acb ;
  struct V4__BktHdr *sbh ;
  struct V4MM__GlobalBuf *glb ;
  int ax,bx,lkx,lkax ; int *lockidptr ;

	if (areaid == V4IS_AreaId_SeqOnly) return ;	/* Nothing to checkpoint on this type of file */
	FINDAREAINPROCESS(ax,areaid)
	if (ax == UNUSED) return ;			/* Could not find area? */
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++)
//	 { if (mmm->Areas[ax].AreaId == areaid) break ; } ;
//	if (ax >= mmm->NumAreas) return ;			/* Could not find area? */

	if (mmm->Areas[ax].SegId != 0)				/* Global area ? */
	 { if ( GETGLB(ax) == NULL)
	    v4_error(V4E_AREANOTGLOBAL,0,"V4MM","AreaCheckPoint","AREANOTGLOBAL","Area (%d %s) not global",ax,UCretASC(mmm->Areas[ax].UCFileName)) ;
	   acb = (struct V4IS__AreaCB *)v4mm_ACBPtr(areaid) ;
	   LOCKALLOC(40) ;						/* Lock down buffer allocation */
	   for(bx=0;bx<glb->BktCnt;bx++)
	    { if (!glb->Bkt[bx].Updated) continue ;		/* Not updated */
	      sbh = (struct V4__BktHdr *)(&glb->Buffer[glb->BktOffset + bx*glb->BktSize]) ;
if (glb->Bkt[bx].BktNum == PENDING)
 { printf("??? Attempt of CheckPoint to write pending bucket (bx=%d, pbkt=%d)\n",bx,glb->Bkt[bx].PendingBktNum) ; continue ; } ;
	      lockidptr = (sbh->BktType==V4_BktHdrType_Data ? &mmm->Areas[ax].LockDAT : &mmm->Areas[ax].LockIDX) ;
	      lkx = v4mm_LockSomething(areaid,sbh->BktNum,V4MM_LockMode_Read,
			(sbh->BktType==V4_BktHdrType_Data ? V4MM_LockIdType_Data : V4MM_LockIdType_Index),0,lockidptr,21) ;
	      if (lkx < 0 && lkx != UNUSED) continue ;		/* If can't grab lock then skip- another pid will update */
	      RELALLOC("a") ; v4mm_BktWrite(ax,sbh->BktNum,sbh,"CheckPoint",bx) ; glb->Bkt[bx].Updated = FALSE ;
	      v4mm_LockRelease(areaid,lkx,mmm->MyPid,areaid,"CheckPoint") ; *lockidptr = UNUSED ;
	      if (bx < glb->BktCnt-1) { LOCKALLOC(41) ; } ;
	    } ; xRELALLOC ;
	   return ;
	 } ;

/*	Here to checkpoint nonGlobal */
	for(bx=0;bx<mmm->NumBkts;bx++)
	 { if (mmm->Bkts[bx].AreaIndex != ax) continue ;
	   if (mmm->Bkts[bx].Updated)
	    { v4mm_BktWrite(ax,mmm->Bkts[bx].BktNum,mmm->Bkts[bx].BktPtr,"CheckPoint",bx) ; mmm->Bkts[bx].Updated = FALSE ; } ;
	 } ;
}


/*	v4mm_AreaFlush - Flushes all buckets in an area		*/
/*	Call: v4mm_AreaFlush( areaid , closeflag )
	  where areaid is area id to be flushed,
	  closeflag is TRUE if coming from io_close routine	*/

void v4mm_AreaFlush(areaid,closeflag)
  int areaid ;
  LOGICAL closeflag ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__LockTable *lt ;
  struct V4__BktHdr *sbh ;
  struct V4MM__GlobalBuf *glb ;
#ifdef segshare
  HANGLOOSEDCL
#ifdef UNIX
  struct shmid_ds buf ;
  int noupdates, pidcnt, cnt ;
#endif
  int lkax ;
#endif
  int tx,ax,bx,lkx,i,retries ; int *lockidptr ; char ebuf[200] ;

	FINDAREAINPROCESS(ax,areaid)
	if (ax == UNUSED) return ;			/* Could not find area? */
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++)
//	 { if (mmm->Areas[ax].AreaId == areaid) break ; } ;
//	if (ax >= mmm->NumAreas) return ;			/* Could not find area? */
	if (mmm->Areas[ax].SegId != 0)
	 { if ( GETGLB(ax) == NULL)
	    v4_error(V4E_AREANOTGLOBAL,0,"V4MM","AreaFlush","AREANOTGLOBAL","Area (%d %s) not global",ax,UCretASC(mmm->Areas[ax].UCFileName)) ;
#ifdef segshare
	   LOCKALLOC(42) ;
	   retries = (closeflag ? 10 : 0) ;
	   for(bx=0;bx<glb->BktCnt;bx++)
	    { if (!glb->Bkt[bx].Updated) continue ;		/* Not updated */
	      if (glb->Bkt[bx].BktNum < 0) continue ;		/* UNUSED (or invalid) bucket */
	      if (!mmm->Areas[ax].WriteAccess) continue ;	/* This process can't update it */
	      sbh = (struct V4__BktHdr *)(&glb->Buffer[glb->BktOffset + bx*glb->BktSize]) ;
	      lockidptr = (sbh->BktType==V4_BktHdrType_Data ? &mmm->Areas[ax].LockDAT : &mmm->Areas[ax].LockIDX) ;
	      lkx = v4mm_LockSomething(areaid,sbh->BktNum,V4MM_LockMode_Read,
			(sbh->BktType==V4_BktHdrType_Data ? V4MM_LockIdType_Data : V4MM_LockIdType_Index), 0,lockidptr,22) ;
	      if (lkx < 0 && lkx != UNUSED)			/* Didn't get lock ? */
	       { if ((retries--) > 0)				/* Try again for same lock */
		  { RELALLOC("c1") ; HANGLOOSE(V4IS_HANG_TIME) ; LOCKALLOC(43) ; bx -- ; } ;
		 continue ;					/* Screw it - just keep plugging */
	       } ;
	      RELALLOC("c") ;
	      v4mm_BktWrite(ax,sbh->BktNum,sbh,"AreaFlush",bx) ; glb->Bkt[bx].Updated = FALSE ;
	      v4mm_LockRelease(areaid,lkx,mmm->MyPid,areaid,"AreaFlush") ; *lockidptr = UNUSED ;
	      if (bx < glb->BktCnt-1) { LOCKALLOC(44) ; } ;
	    } ; xRELALLOC("d") ;
	   v4mm_LockRelease(areaid,V4MM_Lock_ReleasePid,mmm->MyPid,areaid,"AreaFlush") ; /* Free any remaining locks for area */
/*	   Maybe release this global area (if no other processes & we don't have other area linked to it) */
	   lt = (struct V4MM__LockTable *)&glb->Buffer[glb->LockTableOffset] ;
	   LOCKLT(1)						/* Sorta put global area into limbo */
	   for(i=0;i<lt->PidCnt;i++)				/* Get rid of PID entry */
	    { if (lt->LogPids[i] != mmm->MyPid) continue ;
#ifdef WINNT
	      lt->PidLoginDTs[i] = lt->PidLoginDTs[lt->PidCnt-1] ;
	      lt->hThreads[i] = lt->hThreads[lt->PidCnt-1] ; lt->idProcesses[i] = lt->idProcesses[lt->PidCnt-1] ;
#endif
	      lt->LogPids[i] = lt->LogPids[--lt->PidCnt] ; break ;
	    } ; RELLT(1)
#ifdef UNIX
	   for(bx=0;bx<glb->BktCnt;bx++) { if (glb->Bkt[bx].Updated) break ; } ;
	   noupdates = (bx >= glb->BktCnt) ; pidcnt = lt->PidCnt ;
#ifdef HPUX
	   for(cnt=0,i=0;i<mmm->NumAreas;i++)			/* Are we attached to segment via more than one area? */
	    { if (mmm->Areas[i].GBPtr == mmm->Areas[ax].GBPtr) cnt++ ; } ;
#else
	   cnt = 1 ;						/* If not HPUX then only one area per segment */
#endif
	   if (cnt == 1)					/* If last area attached to segment- release */
	    { if (shmdt(mmm->Areas[ax].GBPtr) == -1)
	      printf("? Err=%s detaching %d @%p\n",v_OSErrString(errno),mmm->Areas[ax].SegId,mmm->Areas[ax].GBPtr) ;
	      if (mmm->Areas[ax].wvb != NULL)			/* Maybe get rid of write-verify buffer/segment */
	       { shmdt(mmm->Areas[ax].wvb) ; mmm->Areas[ax].wvb = NULL ; } ;
	    } ;
	   mmm->Areas[ax].GBPtr = NULL ;			/* Flag as unused */
	   v4mm_OpenCloseLock(TRUE) ;
	   if (shmctl(mmm->Areas[ax].SegId,IPC_STAT,&buf) == -1)		/* Can't get info ? */
	    { printf("? Err=%s obtaining info for seg %d\n",v_OSErrString(errno),mmm->Areas[ax].SegId) ; v4mm_OpenCloseLock(FALSE) ; return ; } ;
	   if (buf.shm_nattch <= 0)
	    { if (noupdates)
	       { if (shmctl(mmm->Areas[ax].SegId,IPC_RMID,&buf) == 0) { /* printf("\n[Removed %d]\r",mmm->Areas[ax].SegId) */ ; }
	          else { /* printf("? Err=%d removing %d\n",errno,mmm->Areas[ax].SegId) ; */ } ;
	       } else { mmm->Areas[ax].GBPtr = NULL ;
			sprintf(ebuf,"Bucket (%d) marked as updated, abort IPC_RMID of segment",bx) ; v4is_BugChk(areaid,ebuf) ;
			mmm->Areas[ax].GBPtr = glb ;
		      } ;
	    } ;
#endif /* UNIX */
#ifdef WINNT
	   v4mm_OpenCloseLock(TRUE) ;
	   UnmapViewOfFile((LPCVOID)mmm->Areas[ax].GBPtr) ; CloseHandle(mmm->Areas[ax].SegId) ;
#endif
	   v4mm_OpenCloseLock(FALSE) ;
#endif	/* segshare */
	 } else							/* Not a global area */
	 { if (mmm->Areas[ax].FreeBkt1 != NULL) { v4mm_FreeChunk(mmm->Areas[ax].FreeBkt1) ; mmm->Areas[ax].FreeBkt1 = NULL ; } ;
	   if (mmm->Areas[ax].FreeBkt2 != NULL) { v4mm_FreeChunk(mmm->Areas[ax].FreeBkt2) ; mmm->Areas[ax].FreeBkt2 = NULL ; } ;
	   for(bx=0;bx<mmm->NumBkts;bx++)
	    { if (mmm->Bkts[bx].AreaIndex != ax) continue ;
	      if (mmm->Bkts[bx].Updated)
	       { v4mm_BktWrite(ax,mmm->Bkts[bx].BktNum,mmm->Bkts[bx].BktPtr,"AreaFlush",bx) ; mmm->Bkts[bx].Updated = FALSE ; } ;
	    } ;
	   for(bx=0;bx<mmm->NumBkts;bx++)
	    { if (mmm->Bkts[bx].AreaIndex != ax) continue ;
	      mmm->Bkts[bx].AreaIndex = UNUSED ;
	      v4mm_FreeChunk(mmm->Bkts[bx].BktPtr) ;
	      mmm->Bkts[bx].BktPtr = NULL ; mmm->CurTotalBytes -= mmm->Areas[ax].BktSize ;
	    } ;
	 } ;
#ifdef WINNT
	EnterCriticalSection(&wincs) ;		/* Only one thread at a time thru here! */
#endif
	mmm->Areas[ax].AreaId = UNUSED ;				/* Free up area slot */
	if (ax == mmm->NumAreas) mmm->NumAreas-- ;		/* Reduce count if freeing "last" one */
#ifdef WINNT
	LeaveCriticalSection(&wincs) ;		/* Only one thread at a time thru here! */
#endif
}

/*	v4mm_ZapBucket - Immediately flags bucket as "UNUSED" (only in case of disastrous problem with contents!) */
/*	Call: v4mm_ZapBucket( aix, bktnum )
	  where aix is internal mmm index,
		bktnum is bucket to be zapped		*/

void v4mm_ZapBucket(aix,bktnum)
  int aix,bktnum ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__GlobalBuf *glb ;
  int bx ;

	GETMMMFROMAREAX(mmm,aix) ;
//#ifdef NEWMMM
//	mmm = CURRENTMMM ;
//#else
//	if( (mmm = gpi->mmm) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//#endif
	if (bktnum <= 0) return ;
	if (mmm->Areas[aix].SegId != 0)				/* Global Buffers */
	 { if ( GETGLB(aix) == NULL) return ;
	   for(bx=0;bx<glb->BktCnt;bx++)
	    { if (glb->Bkt[bx].BktNum != bktnum) continue ;
	      glb->Bkt[bx].MemLock = 0 ; glb->Bkt[bx].Updated = FALSE ;
	      glb->Bkt[bx].Updated = FALSE ; glb->Bkt[bx].BktNum = UNUSED ;
	      return ;
	    } ;
	 } ;
/*	Look for the bucket in in-core pool */
	for(bx=0;bx<mmm->NumBkts;bx++)
	 { if (mmm->Bkts[bx].BktNum != bktnum) continue ; if (mmm->Bkts[bx].AreaIndex != aix) continue ;
	   mmm->Bkts[bx].Updated = FALSE ; mmm->Bkts[bx].AreaIndex = UNUSED ; mmm->Bkts[bx].BktNum = UNUSED ;
	   if (mmm->Areas[aix].FreeBkt1 == NULL) { mmm->Areas[aix].FreeBkt1 = mmm->Bkts[bx].BktPtr ; }
	    else { if (mmm->Areas[aix].FreeBkt2 == NULL) { mmm->Areas[aix].FreeBkt2 = mmm->Bkts[bx].BktPtr ; }
		    else { v4mm_FreeChunk(mmm->Bkts[bx].BktPtr) ; } ;
		 } ;
	   mmm->Bkts[bx].BktPtr = NULL ;
	   mmm->CurTotalBytes -= mmm->Areas[aix].BktSize ;
	   mmm->Areas[aix].NumBkts -- ;
	   if (bx == mmm->NumBkts-1) mmm->NumBkts-- ;
	   return ;
	 } ;
}

/*	v4mm_BktFlush - Releases a bucket from in-core area	*/
/*	Call: logical = v4mm_BktFlush( numid , aidflg )
	  where logical is TRUE if bucket flushed, FALSE if not (like it has been updated but this process has read only)
		bktnum is the bucket number,
		aidflg is area id (if -1 then bktnum is really BId)	*/

LOGICAL v4mm_BktFlush(numid,aidflg)
  int numid,aidflg ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__LockTable *lt ;
  struct V4__BktHdr *sbh ;
  struct V4MM__GlobalBuf *glb ;
  union V4MM__BIdMask bidm ;
  int ax,bx,lkx,lkax,i,tx ; int areaid,bktnum ; int *lockidptr ; char ebuf[150] ;

//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
	if (aidflg != -1) { bktnum = numid ; areaid = aidflg ; }
	 else { bidm.bid = numid ; bktnum = bidm.fld.BktNum ; areaid = bidm.fld.AreaIndex ; } ;
//	for(ax=0;ax<mmm->NumAreas;ax++)
//	 { if (mmm->Areas[ax].AreaId == areaid) break ; } ;
//	if (ax >= mmm->NumAreas) return(FALSE) ;		/* Could not find area? */
	FINDAREAINPROCESS(ax,areaid)
	if (ax == UNUSED) return(FALSE) ;			/* Could not find area? */
	if (bktnum == 0)
	 { v4_error(V4E_FLUSHBKT0,0,"V4MM","BktFlush","FLUSHBKT0","Area (%d %s) Cannot flush bucket 0 (root)",ax,UCretASC(mmm->Areas[ax].UCFileName)) ; } ;
	if (bktnum < 0) { printf("?? Attempt to flush invalid (%d) bucket\n",bktnum) ; return(FALSE) ; } ;
	if (mmm->Areas[ax].SegId != 0)				/* Global Buffers */
	 { if ( GETGLB(ax) == NULL)
	    v4_error(V4E_AREANOTGLOBAL,0,"V4MM","BktFlush","AREANOTGLOBAL","Area (%d %s) not global",ax,UCretASC(mmm->Areas[ax].UCFileName)) ;
	   xLOCKALLOC(30) ;
	   lt = (struct V4MM__LockTable *)&glb->Buffer[glb->LockTableOffset] ;
	   for(bx=0;bx<glb->BktCnt;bx++)
	    { if (glb->Bkt[bx].BktNum != bktnum) continue ;
	      if (glb->Bkt[bx].MemLock)
	       { RELALLOC("f") ; return(FALSE) ; } ;		/* Bucket locked in memory (don't flush) */
	      if (!glb->Bkt[bx].Updated)			/* Bucket NOT updated ? */
	       { LOCKLT(2)
		 for(i=0;i<lt->Count;i++)			/* Don't even THINK about flushing if locked by somebody else */
		  { if (lt->Entry[i].LockId == bktnum)
		    { if (lt->Entry[i].PidCount == 1 && lt->Entry[i].Pids[0] == mmm->MyPid)
		       { break ; }				/* Bucket locked by current process */
		       else { RELLT(2) RELALLOC("e") ; return(FALSE) ; } ;
		    } ;
		  } ;
		 glb->Bkt[bx].BktNum = UNUSED ; RELLT(3) break ;
	       } ;
	      if (!mmm->Areas[ax].WriteAccess)
	       { xRELALLOC ; return(FALSE) ; } ;			/* Updated but we can't write it out! */
	      sbh = (struct V4__BktHdr *)(&glb->Buffer[glb->BktOffset + bx*glb->BktSize]) ;
	      if (sbh->BktNum != bktnum)
	       { printf("?? Flush err: Bucket in slot %d s/b #%d but sbh=%d\n",bx,bktnum,sbh->BktNum) ;
		 xRELALLOC ;
		 return(FALSE) ;				/* Don't flush, but then don't crap out either! */
	         v4_error(V4E_INVBKTHDR,0,"V4MM","BktFlush","INVBKTHDR",
			"Area (%d %s) Bucket header (%d) does not match bucket for this position (%d) in file",
				ax,UCretASC(mmm->Areas[ax].UCFileName),sbh->BktNum,bktnum) ;
	       } ;
	      lockidptr = (sbh->BktType==V4_BktHdrType_Data ? &mmm->Areas[ax].LockDAT : &mmm->Areas[ax].LockIDX) ;
	      lkx = v4mm_LockSomething(areaid,sbh->BktNum,V4MM_LockMode_Write,
			(sbh->BktType==V4_BktHdrType_Data ? V4MM_LockIdType_Data : V4MM_LockIdType_Index),0,lockidptr,23) ;
	      if (lkx < 0 && lkx != UNUSED)		/* Could not lock? - skip it cause another process will take care */
	       { xRELALLOC ; return(FALSE) ; } ;
	      xRELALLOC ;
	      v4mm_BktWrite(ax,sbh->BktNum,sbh,"BktFlush",bx) ;
	      glb->Bkt[bx].Updated = FALSE ; glb->Bkt[bx].BktNum = UNUSED ;
	      v4mm_LockRelease(areaid,lkx,mmm->MyPid,areaid,"BktFlush") ; *lockidptr = UNUSED ;
	      break ;
	    } ;
	   xRELALLOC ;
	   return(TRUE) ;
	 } ;
/*	Look for the bucket in in-core pool */
	for(bx=0;bx<mmm->NumBkts;bx++)
	 { if (mmm->Bkts[bx].BktNum != bktnum) continue ; if (mmm->Bkts[bx].AreaIndex != ax) continue ;
	   sbh = (struct V4__BktHdr *)mmm->Bkts[bx].BktPtr ;
#ifdef USEINUSE
	   if (sbh->InUse != 0)
	    v4_error(V4E_BKTINUSESNGL,0,"V4MM","BktFlush","BKTINUSESNGL","Area (%d %s) Bucket (%d) marked as INUSE (%d) on a single process area",
				ax,UCretASC(mmm->Areas[ax].UCFileName),sbh->BktNum,areaid,sbh->InUse) ;
#endif
/*	   Found bucket - flush it! */
	   if (mmm->Bkts[bx].Updated)
	    { if (sbh->BktNum != bktnum)
	       { sprintf(ebuf,"Flush err: Bucket in slot %d s/b #%d but sbh=%d",bx,bktnum,sbh->BktNum) ;
	         v4is_BugChk(areaid,ebuf) ;
	         v4_error(V4E_INVBKTHDR,0,"V4MM","BktFlush","INVBKTHDR",
			"Area (%d %s) Bucket header (%d) does not match bucket for this position (%d) in file",
				ax,UCretASC(mmm->Areas[ax].UCFileName),sbh->BktNum,bktnum) ;
	       } ;
	      v4mm_BktWrite(ax,mmm->Bkts[bx].BktNum,mmm->Bkts[bx].BktPtr,"BktFlush",bx) ;
	      mmm->Bkts[bx].Updated = FALSE ;
	   } ;
	   mmm->Bkts[bx].AreaIndex = UNUSED ; mmm->Bkts[bx].BktNum = UNUSED ;
	   if (mmm->Areas[ax].FreeBkt1 == NULL) { mmm->Areas[ax].FreeBkt1 = mmm->Bkts[bx].BktPtr ; }
	    else { if (mmm->Areas[ax].FreeBkt2 == NULL) { mmm->Areas[ax].FreeBkt2 = mmm->Bkts[bx].BktPtr ; }
		    else { v4mm_FreeChunk(mmm->Bkts[bx].BktPtr) ; } ;
		 } ;
	   mmm->Bkts[bx].BktPtr = NULL ;
	   mmm->CurTotalBytes -= mmm->Areas[ax].BktSize ;
	   mmm->Areas[ax].NumBkts -- ;
	   if (bx == mmm->NumBkts-1) mmm->NumBkts-- ;
	   return(TRUE) ;
	 } ;
	return(FALSE) ;
}

/*	v4mm_ACBPtr - Returns pointer to area's Area Control Block	*/
/*	Call: areaptr = v4mm_ACBPtr( aid )
	  where areaptr is point to acb,
		aid is area id						*/

struct V4IS__AreaCB *v4mm_ACBPtr(aid)
  int aid ;
{ struct V4MM__MemMgmtMaster *mmm ;
  int ax ;

//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++)
//	 { if (mmm->Areas[ax].AreaId == aid) return(mmm->Areas[ax].AreaCBPtr) ; } ;
	FINDAREAINPROCESS(ax,aid)
	if (ax != UNUSED) return(mmm->Areas[ax].AreaCBPtr) ;
	v4_error(V4E_AIDNOTINMM,0,"V4MM","ACBPtr","AIDNOTINMM","Could not locate aid (%d) in mmm",aid) ;
	return(NULL) ;
}

/*	v4mm_ACBPtrRE - Same as above but returns NULL on error (instead of trap)	*/
/*	Call: areaptr = v4mm_ACBPtr( aid )
	  where areaptr is point to acb,
		aid is area id						*/

struct V4IS__AreaCB *v4mm_ACBPtrRE(aid)
  int aid ;
{ struct V4MM__MemMgmtMaster *mmm ;
  int ax ;

//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++)
//	 { if (mmm->Areas[ax].AreaId == aid) return(mmm->Areas[ax].AreaCBPtr) ; } ;
	FINDAREAINPROCESS(ax,aid)
	if (ax != UNUSED) return(mmm->Areas[ax].AreaCBPtr) ;
	return(NULL) ;
}

/*	v4mm_MakeDataId - Returns new Data ID from current bucket	*/
/*	Call: dataid = v4mm_MakeDataId( bktptr )
	  where dataid is new data id,
		bktptr is point to bucket where data is to reside	*/

int v4mm_MakeDataId(bktptr)
  struct V4IS__Bkt *bktptr ;
{ struct V4IS__Bkt *bkt ;
  struct V4IS__IndexBktHdr *ibh ;
  union V4MM__DataIdMask dim ;

	bkt = bktptr ; ibh = (struct V4IS__IndexBktHdr *)bkt ;
	dim.fld.BktNum = ibh->sbh.BktNum ; dim.fld.BktSeq = ibh->sbh.AvailBktSeq++ ;
	return(dim.dataid) ;
}

int v4mm_MakeBId(num,aid)
  int num,aid ;
{ struct V4MM__MemMgmtMaster *mmm ;
  union V4MM__BIdMask bid ;
  int ax ;

	FINDAREAINPROCESS(ax,aid)
	if (ax == UNUSED) v4_error(V4E_AIDNOTINMM,0,"V4MM","MakeBId","AIDNOTINMM","Could not locate aid (%d) in mmm",aid) ;
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++) { if (mmm->Areas[ax].AreaId == aid) break ; } ;
//	if(ax >= mmm->NumAreas) v4_error(V4E_AIDNOTINMM,0,"V4MM","MakeBId","AIDNOTINMM","Could not locate aid (%d) in mmm",aid) ;
	bid.fld.AreaIndex = ax ; bid.fld.BktNum = num ;
	return(bid.bid) ;
}

#define GLOBALBUFTRYMAX 3509

/*	v4mm_NewBktPtr - Allocates an new (empty) bucket for area	*/
/*	Call: bktptr = v4mm_NewBktPtr( bktnum , areaid , flags , lockidtype )
	  where bktptr = pointer to bucket,
		bktnum is bucket number (usually) to allocate/create,
		areaid is area, if = -1 then bktnum is actually the BId!,
		flags are marker flags for bucket - V4MM_BktPtr_xxx,
		lockidtype is V4MM_LockIdType_xxx indicating bucket type	*/

struct V4IS__Bkt *v4mm_NewBktPtr(bktnum,areaid,flags,lockidtype)
  int bktnum, areaid, flags, lockidtype ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4__BktHdr *sbh ;
  struct V4MM__GlobalBuf *glb ;
  union V4MM__BIdMask bidm ;
  HANGLOOSEDCL
  int ax,bx,tries,lid ; int lkax ; char ebuf[150] ;

	FINDAREAINPROCESS(ax,areaid)
	if (ax == UNUSED) v4_error(V4E_MAXAREAS,0,"V4MM","NewBktPtr","MAXAREAS","Too many areas (%d)",mmm->NumAreas) ;
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++)
//	 { if (mmm->Areas[ax].AreaId == areaid) break ; } ;
//	if (ax >= mmm->NumAreas) v4_error(V4E_MAXAREAS,0,"V4MM","NewBktPtr","MAXAREAS","Too many areas (%d)",mmm->NumAreas) ;
	if (bktnum < 0)
	 v4_error(V4E_INVBKTNUM,0,"V4MM","NewBktPtr","INVBKTNUM","Area (%d %s) Bucket number (%d) cannot be less than 0",
		areaid,UCretASC(mmm->Areas[ax].UCFileName),bktnum) ;
/*	Using global buffers ? */
	if (mmm->Areas[ax].SegId != 0)
	 { if ( GETGLB(ax) == NULL)
	    v4_error(V4E_AREANOTGLOBAL,0,"V4MM","NewBktPtr","AREANOTGLOBAL","Area (%d %s) not global",ax,UCretASC(mmm->Areas[ax].UCFileName)) ;
try_again:
	   for(tries=0;tries<GLOBALBUFTRYMAX;tries++)
	    { LOCKALLOC(45) ;
	      for(bx=0;bx<glb->BktCnt;bx++)
	       { if (glb->Bkt[bx].BktNum == UNUSED) break ;	/* Found a free bucket */
	       } ;
	      if (bx >= glb->BktCnt)				/* No free buckets? */
	       { bx = v4mm_GlobalBufGrab(ax,bktnum) ;		/*  Go get one */
		 if (bx >= 0) break ;
	         RELALLOC("m") ; HANGLOOSE(V4IS_HANG_TIME) ; continue ;		/* Couldn't get anything- try later */
	       } else
	       { glb->Bkt[bx].BktNum = PENDING ; glb->Bkt[bx].PendingBktNum = bktnum ; glb->Bkt[bx].PendingPid = mmm->MyPid ;
		 break ;
	       } ;
	    } ;
	   if (tries >= GLOBALBUFTRYMAX)
	    { RELALLOC("n") ;
	      sprintf(ebuf,"Could not allocate (new) global buffer (%d)",bktnum) ; v4is_BugChk(areaid,ebuf) ;
	      v4_error(V4E_NOALLOCGLBL,0,"V4MM","NewBktPtr","NOALLOCGLBL","Area (%d %s) Could not allocate global buffer (%d)",
		areaid,UCretASC(mmm->Areas[ax].UCFileName),bktnum) ;
	    } ;
	   lid = v4mm_LockSomething(areaid,bktnum,V4MM_LockMode_Write,lockidtype,
		0,(lockidtype == V4MM_LockIdType_Data ? &mmm->Areas[ax].LockDAT : &mmm->Areas[ax].LockIDX),24) ;
	   if (lid < 0)			/* Did not get lock ? */
	    { glb->Bkt[bx].BktNum = UNUSED ;
	      RELALLOC("nn") ;
	      sprintf(ebuf,"Could not grab lock bkt=%d, type=%d in NewBktPtr",bktnum,lockidtype) ; v4is_BugChk(areaid,ebuf) ;
	      HANGLOOSE(V4IS_HANG_TIME) ; goto try_again ;
	    } ;
	   sbh = (struct V4__BktHdr *)(&glb->Buffer[glb->BktOffset + bx*glb->BktSize]) ;
	   sbh->InUse = 1 ;		/* New bucket - one process using it ! */
	   sbh->BktNum = bktnum ;	/* Flag in actual bucket so error checking will work properly */
	   glb->Bkt[bx].PendingPid = UNUSEDPID ;
	   glb->Bkt[bx].Updated = TRUE ; glb->Bkt[bx].BktNum = bktnum ; glb->Bkt[bx].PendingBktNum = UNUSED ;
	   glb->Bkt[bx].CallCnt = (glb->TotalCalls++) ;		/* Update access count */
glb->Bkt[bx].PendingBktNum = -(int)mmm->MyPid ;		/* *** TEMP CODE TO TRACK WHO INIT'ed *** */
	   if (flags & V4MM_BktPtr_MemLock)
	    { glb->Bkt[bx].MemLock |= LOCKPERM ;		/* Have to lock the bucket */
	    } ;
	   RELALLOC("o") ;
#ifdef V4MM_GlobalTrace
	   tx = (++glb->LastTrace) % V4MM_GlobalTrace ; glb->Trace[tx].CallCnt = glb->Bkt[bx].CallCnt ;
	   glb->Trace[tx].BBktNum = bktnum ; glb->Trace[tx].sbhBktNum = sbh->BktNum ;
	   glb->Trace[tx].bx = bx ; glb->Trace[tx].Where = 1 ; glb->Trace[tx].Pid = mmm->MyPid ; glb->Trace[tx].LockId = lid ;
	   glb->Trace[tx].glbBktNum = glb->Bkt[bx].BktNum ;
#endif
	   return((struct V4IS__Bkt *)sbh) ;
	 } ;
/*	Look for the bucket in in-core pool */
	for(bx=0;bx<mmm->NumBkts;bx++)
	 { if (mmm->Bkts[bx].BktNum != bktnum) continue ; if (mmm->Bkts[bx].AreaIndex != ax) continue ;
	   v4_error(V4E_BKTINMMM,0,"V4MM","NewBktPtr","BKTINMMM","Bucket (%d) already in mmm",bktnum) ;
	 } ;
/*	Bucket not found - have to get it, but first do we have enough room? */
	bidm.fld.BktNum = bktnum ; bidm.fld.AreaIndex = ax ;
	if (mmm->Areas[ax].NumBkts >= mmm->Areas[ax].MaxBkts)			/* Are we over quota for this area ? */
	 { v4mm_AreaFlushOld(ax) ; } ;						/* Not any more */
	for(bx=0;bx<mmm->NumBkts;bx++) { if (mmm->Bkts[bx].BktPtr == NULL) break ; } ;
	if (bx >= V4MM_AreaBkt_Max)						/* Over the total process limit? */
	 { v4mm_AreaFlushOld(UNUSED) ;						/*   then flush whatever we can */
	   for(bx=0;bx<mmm->NumBkts;bx++) { if (mmm->Bkts[bx].BktPtr == NULL) break ; } ;
	   if (bx >= V4MM_AreaBkt_Max)
	    v4_error(V4E_TOOMANYBKTS,0,"V4MM","NewBktPtr","TOOMANYBKTS","Too many buckets (%d) for area (#%d) in mmm",V4MM_AreaBkt_Max,ax) ;
	 } ;
/*	Set up for new bucket */
	if (bx >= mmm->NumBkts) mmm->NumBkts++ ;
	mmm->Bkts[bx].BktNum = bktnum ;
	if (mmm->Areas[ax].FreeBkt1 != NULL)
	 { mmm->Bkts[bx].BktPtr = mmm->Areas[ax].FreeBkt1 ; mmm->Areas[ax].FreeBkt1 = NULL ; }
	 else { if (mmm->Areas[ax].FreeBkt2 != NULL)
		 { mmm->Bkts[bx].BktPtr = mmm->Areas[ax].FreeBkt2 ; mmm->Areas[ax].FreeBkt2 = NULL ; }
		 else { mmm->Bkts[bx].BktPtr = (struct V4IS__Bkt *)v4mm_AllocChunk(mmm->Areas[ax].BktSize,FALSE) ;
		      } ;
	      } ;
	sbh = (struct V4__BktHdr *)mmm->Bkts[bx].BktPtr ; sbh->InUse = 1 ;	/* New bucket - one process using it ! */
	mmm->Bkts[bx].AreaIndex = ax ;
	mmm->Bkts[bx].Updated = TRUE ;
	mmm->Bkts[bx].CallCnt = (mmm->TotalCalls++) ;		/* Update access count */
	mmm->Areas[ax].NumBkts ++ ;				/* Increment buckets for this area */
	if (flags & V4MM_BktPtr_MemLock)
	 { mmm->Bkts[bx].MemLock = LOCKPERM ;			/* Have to lock the bucket */
	 } else mmm->Bkts[bx].MemLock = 0 ;
	return(mmm->Bkts[bx].BktPtr) ;
}

/*	v4mm_BktPtr - Converts bucket id into a real pointer, loads bucket if necessary */
/*	Call: bktptr = v4mm_BktPtr( numid , aidflg , flags )
	  where bktptr = pointer to bucket,
		numid is bucket number (usually) to get,
		aidflg is area, if = -1 then numid is actually the BId!,
		flags are marker flags for bucket - V4MM_BktPtr_xxx			*/

struct V4IS__Bkt *v4mm_BktPtr(numid,aidflg,flags)
  int numid, aidflg, flags ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4__BktHdr *sbh ;
  struct V4MM__GlobalBuf *glb ;
  union V4MM__BIdMask bidm ;
  struct V4MM__LockTable *lt ;
  HANGLOOSEDCL
  int i,lx,ax,bx,tries,lkax,tx ; int areaid,bktnum,bn ; char ebuf[250] ;
int lockcnt,grabcnt,funkcnt,pendcnt,bncnt ;

top_of_module:
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
	if (aidflg != -1) { bktnum = numid ; areaid = aidflg ; }
	 else { bidm.bid = numid ; bktnum = bidm.fld.BktNum ; areaid = bidm.fld.AreaIndex ; } ;
	FINDAREAINPROCESS(ax,areaid)
	if (ax == UNUSED) v4_error(V4E_MAXAREAS,0,"V4MM","BktPtr","MAXAREAS","Too many areas (%d)",mmm->NumAreas) ;
//	for(ax=0;ax<mmm->NumAreas;ax++)
//	 { if (mmm->Areas[ax].AreaId == areaid) break ; } ;
//	if (ax >= mmm->NumAreas) v4_error(V4E_MAXAREAS,0,"V4MM","BktPtr","MAXAREAS","Too many areas (%d)",mmm->NumAreas) ;
	if (bktnum < 0)
	 v4_error(V4E_INVBKTNUM,0,"V4MM","BktPtr","INVBKTNUM","Area (%d %s) Bucket number (%d) cannot be less than 0",
		areaid,UCretASC(mmm->Areas[ax].UCFileName),bktnum) ;
	if (flags & V4MM_BktPtr_ReRead) v4mm_BktFlush(bktnum,areaid) ; /* If REREAD then try to flush out bucket first */
	lockcnt=0 ; grabcnt=0 ; funkcnt=0 ; pendcnt=0 ;
	if (mmm->Areas[ax].SegId != 0)
	 { if ( GETGLB(ax) == NULL)
	    v4_error(V4E_AREANOTGLOBAL,0,"V4MM","BktPtr","AREANOTGLOBAL","Area (%d %s) not global",ax,UCretASC(mmm->Areas[ax].UCFileName)) ;
	   lt = (struct V4MM__LockTable *)&glb->Buffer[glb->LockTableOffset] ;
	   for(tries=1;tries<GLOBALBUFTRYMAX;tries++)
	    { LOCKALLOC(46) ;
	      for(bx=0;bx<glb->BktCnt;bx++)			/* Look for the bucket */
	       { if (glb->Bkt[bx].BktNum == bktnum) break ;
		 if (glb->Bkt[bx].BktNum == PENDING && glb->Bkt[bx].PendingBktNum == bktnum) break ;
	       } ;
	      if (bx < glb->BktCnt)				/* Did we find it? */
	       { if (glb->Bkt[bx].BktNum == PENDING && glb->Bkt[bx].PendingPid != mmm->MyPid)
		  { if (PROCESS_GONE(glb->Bkt[bx].PendingPid))
		     { glb->Bkt[bx].BktNum = UNUSED ;		/* PendingPid gone - trash bucket */
		       sprintf(ebuf,"BufGrab- trashing pending bkt (%d %d) - defunct pid %d\n",
				bx,glb->Bkt[bx].PendingBktNum,glb->Bkt[bx].PendingPid) ;
		       v4is_BugChk(areaid,ebuf) ;
	  	     } ;
	  	    pendcnt++ ;
	  	    if (pendcnt > 1000)
	  	     { sprintf(ebuf,"Bucket (%d/%d) appears hung by process (%d), grabbing control",
				bktnum,bx,glb->Bkt[bx].PendingPid) ;
		       v4is_BugChk(areaid,ebuf) ;
	  	       glb->Bkt[bx].PendingPid = mmm->MyPid ;
	  	       goto read_bucket ;
	  	     } ;
	            RELALLOC("q") ; HANGLOOSE(V4IS_HANG_TIME) ;
		    continue ;	/* Got pending bucket (may be what we want!) - try again */
		  } ;
		 if (glb->Bkt[bx].BktNum == PENDING)		/* Should not get this ! */
		  { glb->Bkt[bx].BktNum = UNUSED ; RELALLOC("q1") ; continue ; } ;
		 if (glb->Bkt[bx].PendingPid != UNUSEDPID)		/* Should not get this! */
		  { sprintf(ebuf,"BktPtr(pend!=UNUSED) bx=%d, glbBkt=%d, sbhBkt=%d, PendPid=%d, TotalCalls=%d ... will retry\n",
				bx,glb->Bkt[bx].BktNum,sbh->BktNum,glb->Bkt[bx].PendingPid,glb->TotalCalls) ;
		    v4is_BugChk(areaid,ebuf) ;
		    glb->Bkt[bx].BktNum = PENDING ; RELALLOC("zz") ; HANGLOOSE(V4IS_HANG_TIME) ; continue ;
		  } ;
		 sbh = (struct V4__BktHdr *)(&glb->Buffer[glb->BktOffset + bx*glb->BktSize]) ; sbh->InUse ++ ;
		 if (sbh->BktNum != bktnum)
		  { sprintf(ebuf,"Bucket (bx=%d num=%d) does not match contents (BktNum=%d PendingBkt=%d sbh->BktNum=%d)",
			bx,bktnum,glb->Bkt[bx].BktNum,glb->Bkt[bx].PendingBktNum,sbh->BktNum) ;
		    v4is_BugChk(areaid,ebuf) ;
		    funkcnt++ ; v4mm_ZapBucket(ax,bktnum) ;		/* Try to flush the offending bucket */
		    RELALLOC("q1") ; HANGLOOSE(V4IS_HANG_TIME) ; continue ;
		  } ;
	         glb->Bkt[bx].CallCnt = (glb->TotalCalls++) ;	/* Update access count (even if we don't get- we want it!) */
	         lx = v4mm_LockSomething(areaid,bktnum,(flags & V4MM_BktPtr_Update ? V4MM_LockMode_Write : V4MM_LockMode_Read),
			   (sbh->BktType==V4_BktHdrType_Data ? V4MM_LockIdType_Data : V4MM_LockIdType_Index),0,
			   (sbh->BktType==V4_BktHdrType_Data ? &mmm->Areas[ax].LockDAT : &mmm->Areas[ax].LockIDX),25) ;
		 lockcnt++ ;
		 if (lx < 0 && (flags & V4MM_BktPtr_IgnoreLock) == 0)	/* Didn't get lock - release ALLOC lock, wait & try again */
		  { if ((tries % V4IS_LOCKTRY_PANIC) != 0) goto try_again ;
		    LOCKLT(3)
		    if (glb->PanicLockout <= 0)					/* If no lockout then start one */
		     { glb->PanicLockout = lt->PidCnt*2 ; glb->PanicPid = mmm->MyPid ; RELLT(4)
		       sprintf(ebuf,"Panic lockout from BktPtr for bucket %d",bktnum) ;
		       v4is_BugChk(areaid,ebuf) ;
		       for(bx=0;bx<glb->BktCnt;bx++)			/* See if all pending reads are valid */
		        { if (glb->Bkt[bx].BktNum != PENDING) continue ;
			  if (!PROCESS_GONE(glb->Bkt[bx].PendingPid)) continue ;
			  glb->Bkt[bx].BktNum = UNUSED ;		/* PendingPid gone - trash bucket */
		          sprintf(ebuf,"Trashing pending bkt (%d %d) - defunct pid %d\n",
				bx,glb->Bkt[bx].PendingBktNum,glb->Bkt[bx].PendingPid) ;
			  v4is_BugChk(areaid,ebuf) ;
	  	        } ;
		     } else { RELLT(5) } ;
		    goto try_again ;
		  } ;
	         if (flags & V4MM_BktPtr_Update) glb->Bkt[bx].Updated = TRUE ;
	         if (flags & V4MM_BktPtr_MemLock) glb->Bkt[bx].MemLock |= LOCKPERM ;	/* Have to lock the bucket */
		 RELALLOC("r") ;
		 if (sbh->BktNum != bktnum)
		  { sprintf(ebuf,"Bucket (bx=%d num=%d) does not match(2) contents (BktNum=%d PendingBkt=%d sbh->BktNum=%d)",
			bx,bktnum,glb->Bkt[bx].BktNum,glb->Bkt[bx].PendingBktNum,sbh->BktNum) ;
		    v4is_BugChk(areaid,ebuf) ;
		    v4mm_ZapBucket(ax,bktnum) ;		/* Try to flush the offending bucket */
		    HANGLOOSE(V4IS_HANG_TIME) ; continue ;
		  } ;
#ifdef V4MM_GlobalTrace
	   tx = (++glb->LastTrace) % V4MM_GlobalTrace ; glb->Trace[tx].CallCnt = glb->Bkt[bx].CallCnt ;
	   glb->Trace[tx].BBktNum = bktnum ; glb->Trace[tx].sbhBktNum = sbh->BktNum ;
	   glb->Trace[tx].bx = bx ; glb->Trace[tx].Where = 2 ; glb->Trace[tx].Pid = mmm->MyPid ; glb->Trace[tx].LockId = lx ;
	   glb->Trace[tx].glbBktNum = glb->Bkt[bx].BktNum ;
#endif
	         return((struct V4IS__Bkt *)sbh) ;
	       } ;
	      for(bx=0;bx<glb->BktCnt;bx++)			/* Could not find it - have to read in existing bucket */
	       { if (glb->Bkt[bx].BktNum != UNUSED) continue ;
		 glb->Bkt[bx].BktNum = PENDING ; glb->Bkt[bx].PendingBktNum = bktnum ; glb->Bkt[bx].PendingPid = mmm->MyPid ;
		 break ;
	       } ;
	      if (bx >= glb->BktCnt)				/* No free buckets? */
	       { bx = v4mm_GlobalBufGrab(ax,bktnum) ;		/*  Go get one */
grabcnt++ ;
		 if (bx >= 0) break ;
	         RELALLOC("s") ; HANGLOOSE(V4IS_HANG_TIME) ; continue ;
	       } else break ;
try_again:    RELALLOC("t") ; HANGLOOSE(V4IS_HANG_TIME) ;			/* Here to try again */
	      continue ;
	    } ;
	   if (tries >= GLOBALBUFTRYMAX)
	    { sprintf(ebuf,"BktPtr: Could not allocate global buffer (%d) grab=%d, lock=%d, tries=%d, funk=%d, flags=%x, bx=%d",
				bktnum,grabcnt,lockcnt,tries,funkcnt,flags,bx) ;
	      v4is_BugChk(areaid,ebuf) ;
	      v4_error(V4E_NOALLOCGLBL,0,"V4MM","BktPtr","NOALLOCGLBL","Area (%d %s) Could not allocate global buffer (%d)",
		areaid,UCretASC(mmm->Areas[ax].UCFileName),bktnum) ;
	    } ;
read_bucket:
	   glb->Bkt[bx].MemLock = ( (flags & V4MM_BktPtr_MemLock) ? TRUE : FALSE ) ;
	   sbh = (struct V4__BktHdr *)(&glb->Buffer[glb->BktOffset + bx*glb->BktSize]) ;
	   lx = v4mm_LockSomething(areaid,bktnum,((flags & V4MM_BktPtr_Update) ? V4MM_LockMode_Write : V4MM_LockMode_Read),
			((flags & 0xFF) == V4MM_LockIdType_Data ? V4MM_LockIdType_Data : V4MM_LockIdType_Index),
			04,((flags & 0xFF) == V4MM_LockIdType_Data ? &mmm->Areas[ax].LockDAT : &mmm->Areas[ax].LockIDX),26) ;
	   if (glb->Bkt[bx].BktNum != PENDING || glb->Bkt[bx].PendingBktNum != bktnum || mmm->MyPid != glb->Bkt[bx].PendingPid)
	    { sprintf(ebuf,"Ptr1 bx=%d, bktnum=%d, glbBkt=%d, sbhBkt=%d, PendPid=%d, TotalCalls=%d",
			bx,bktnum,glb->Bkt[bx].BktNum,sbh->BktNum,glb->Bkt[bx].PendingPid,glb->TotalCalls) ;
	      v4is_BugChk(areaid,ebuf) ;
	      if (glb->Bkt[bx].PendingPid == mmm->MyPid) glb->Bkt[bx].BktNum = PENDING ;
	    } ;
	   RELALLOC("v") ;
	   if (glb->Bkt[bx].BktNum != PENDING || glb->Bkt[bx].PendingBktNum != bktnum || mmm->MyPid != glb->Bkt[bx].PendingPid)
	    { sprintf(ebuf,"BktPtr2 bx=%d, bktnun=%d ,glbBkt=%d, sbhBkt=%d, PendPid=%d, TotalCalls=%d",
			bx,bktnum,glb->Bkt[bx].BktNum,sbh->BktNum,glb->Bkt[bx].PendingPid,glb->TotalCalls) ;
	      v4is_BugChk(areaid,ebuf) ;
	      if (glb->Bkt[bx].PendingPid == mmm->MyPid) glb->Bkt[bx].BktNum = PENDING ;
	    } ;
	   glb->Bkt[bx].MemLock |= LOCKTEMP ;			/* Set temporary lock on this bucket/slot */
#ifdef V4MM_GlobalTrace
	   tx = (++glb->LastTrace) % V4MM_GlobalTrace ; glb->Trace[tx].CallCnt = glb->Bkt[bx].CallCnt ;
	   glb->Trace[tx].BBktNum = bktnum ; glb->Trace[tx].sbhBktNum = sbh->BktNum ;
	   glb->Trace[tx].bx = bx ; glb->Trace[tx].Where = 5 ; glb->Trace[tx].Pid = mmm->MyPid ; glb->Trace[tx].LockId = lx ;
	   glb->Trace[tx].glbBktNum = glb->Bkt[bx].BktNum ;
#endif
	   v4mm_BktRead(ax,bktnum,sbh) ;
/*	   GETGLB(ax) ;			/* Maybe this will fix bug-from-hell */
	   if (glb->Bkt[bx].BktNum != PENDING && glb->Bkt[bx].PendingPid == mmm->MyPid)
	    { sprintf(ebuf,"BktPtrRd bx=%d, bktnum=%d, glbBkt=%d, sbhBkt=%d, PendPid=%d, TotalCalls=%d ... will correct & retry\n",
			bx,bktnum,glb->Bkt[bx].BktNum,sbh->BktNum,glb->Bkt[bx].PendingPid,glb->TotalCalls) ;
	      v4is_BugChk(areaid,ebuf) ;
	      glb->Bkt[bx].BktNum = PENDING ;
	    } ;
	   for(i=0,bn=bktnum;i<mmm->Areas[ax].IndexOnlyCount;i++)	/* See if we have linked to another area */
	    { if (bn < mmm->Areas[ax].IndexOnly[i].BktOffset) break ; } ;
	   if (i > 0) bn -= mmm->Areas[ax].IndexOnly[i-1].BktOffset ;	/*  yes - adjust bucket number */
	   if (glb->Bkt[bx].BktNum != PENDING || glb->Bkt[bx].PendingBktNum != bktnum || sbh->BktNum != bn || mmm->MyPid != glb->Bkt[bx].PendingPid)
	    { LOCKLT(4)
#ifdef V4MM_GlobalTrace
	      tx = (++glb->LastTrace) % V4MM_GlobalTrace ; glb->Trace[tx].CallCnt = glb->Bkt[bx].CallCnt ;
	      glb->Trace[tx].BBktNum = bktnum ; glb->Trace[tx].sbhBktNum = sbh->BktNum ;
	      glb->Trace[tx].bx = bx ; glb->Trace[tx].Where = 4 ; glb->Trace[tx].Pid = mmm->MyPid ; glb->Trace[tx].LockId = lx ;
	      glb->Trace[tx].glbBktNum = glb->Bkt[bx].BktNum ;
	      v4mm_DumpTrace(glb) ;
#endif
	      RELLT(6)
	      sprintf(ebuf,"GrabCnt=%d, LockCnt=%d, FunkCnt=%d, tx = %d, Buffer (%d) s/b PENDING (%d), Pending=%d, bktnum = %d, sbh->BktNum = %d, Pids (mine=%d,pend=%d)",
		grabcnt,lockcnt,funkcnt,tx,bx,glb->Bkt[bx].BktNum,glb->Bkt[bx].PendingBktNum,bktnum,sbh->BktNum,mmm->MyPid,glb->Bkt[bx].PendingPid) ;
	      v4is_BugChk(areaid,ebuf) ;
	      goto top_of_module ;			/* VEH980605 - Don't error - just try again */
	    } ;
	   LOCKALLOC(47) ;
	   for(i=0,bncnt=0;i<glb->BktCnt;i++)		/* Make sure bucket only in glb once */
	    { if (glb->Bkt[i].BktNum == bktnum) { bncnt++ ; continue ; } ;
	      if (glb->Bkt[i].BktNum != PENDING) continue ;
	      if (glb->Bkt[i].PendingBktNum == bktnum) bncnt++ ;
	    } ;
	   if (bncnt != 1)
	    { sprintf(ebuf,"In BktPtr with duplicate (%d) bucket (%d bx=%d)",bncnt,bktnum,bx) ; v4is_BugChk(areaid,ebuf) ;
	      glb->Bkt[bx].BktNum = UNUSED ; glb->Bkt[bx].PendingBktNum = UNUSED ;
	      RELALLOC("7c") ;
	      goto top_of_module ;				/* Start all over again! */
	    } ;
	   glb->Bkt[bx].MemLock &= (~LOCKTEMP) ;		/* Remove temp lock */
	   sbh->InUse = 1 ;
	   glb->Bkt[bx].Updated = ( (flags & V4MM_BktPtr_Update) ? TRUE : FALSE ) ;
	   glb->Bkt[bx].BktNum = bktnum ; glb->Bkt[bx].PendingBktNum = UNUSED ; glb->Bkt[bx].PendingPid = UNUSEDPID ;
glb->Bkt[bx].PendingBktNum = 1000000+(int)mmm->MyPid ;	/* *** TEMP to track who read in bucket *** */
	   glb->Bkt[bx].CallCnt = (glb->TotalCalls++) ;		/* Update access count */
#ifdef V4MM_GlobalTrace
	   tx = (++glb->LastTrace) % V4MM_GlobalTrace ; glb->Trace[tx].CallCnt = glb->Bkt[bx].CallCnt ;
	   glb->Trace[tx].BBktNum = bktnum ; glb->Trace[tx].sbhBktNum = sbh->BktNum ;
	   glb->Trace[tx].bx = bx ; glb->Trace[tx].Where = 3 ; glb->Trace[tx].Pid = mmm->MyPid ; glb->Trace[tx].LockId = lx ;
	   glb->Trace[tx].glbBktNum = glb->Bkt[bx].BktNum ;
#endif
	   RELALLOC("7b") ;
	   return((struct V4IS__Bkt *)sbh) ;
	 } ;
/*	Look for the bucket in in-core pool */
	for(bx=0;bx<mmm->NumBkts;bx++)
	 { if (mmm->Bkts[bx].BktNum != bktnum) continue ; if (mmm->Bkts[bx].AreaIndex != ax) continue ;
/*	   Found bucket - return pointer */
	   if (flags & V4MM_BktPtr_Update) mmm->Bkts[bx].Updated = TRUE ;
	   if (flags & V4MM_BktPtr_MemLock)
	    { mmm->Bkts[bx].MemLock = TRUE ;			/* Have to lock the bucket */
	    } ;
	   mmm->Bkts[bx].CallCnt = (mmm->TotalCalls++) ;		/* Update access count */
	   sbh = (struct V4__BktHdr *)mmm->Bkts[bx].BktPtr ; sbh->InUse ++ ;	/* Increment InUse counter */
	   return(mmm->Bkts[bx].BktPtr) ;
	 } ;
/*	Bucket not found - have to get it, but first do we have enough room? */
	if (mmm->Areas[ax].NumBkts >= mmm->Areas[ax].MaxBkts)			/* Are we over quota for this area ? */
	 { v4mm_AreaFlushOld(ax) ; } ;						/* Not any more */
	if (mmm->NumBkts >= V4MM_AreaBkt_Max-1)
	 { v4mm_AreaFlushOld(UNUSED) ; } ;
	for(bx=0;bx<mmm->NumBkts;bx++) { if (mmm->Bkts[bx].BktPtr == NULL) break ; } ;
	if (bx >= mmm->NumBkts)
	 { if (mmm->NumBkts >= V4MM_AreaBkt_Max-1)
	    v4_error(V4E_MAXBKTS,0,"V4MM","BktPtr","MAXBKTS","Area (%d %s) Too many buckets (%d)",
		ax,UCretASC(mmm->Areas[ax].UCFileName),mmm->NumBkts) ;
	   bx = mmm->NumBkts++ ;
	 } ;
	mmm->Bkts[bx].BktNum = bktnum ;
	mmm->Bkts[bx].Updated = FALSE ; mmm->Bkts[bx].MemLock = FALSE ;
	if (mmm->Areas[ax].FreeBkt1 != NULL)
	 { mmm->Bkts[bx].BktPtr = mmm->Areas[ax].FreeBkt1 ; mmm->Areas[ax].FreeBkt1 = NULL ; }
	 else { if (mmm->Areas[ax].FreeBkt2 != NULL)
		 { mmm->Bkts[bx].BktPtr = mmm->Areas[ax].FreeBkt2 ; mmm->Areas[ax].FreeBkt2 = NULL ; }
		 else { mmm->Bkts[bx].BktPtr = (struct V4IS__Bkt *)v4mm_AllocChunk(mmm->Areas[ax].BktSize,FALSE) ;
		      } ;
	      } ;
	if (mmm->Bkts[bx].BktPtr == NULL)
	 v4_error(V4E_ALLOCFAIL,0,"V4MM","BktPtr","ALLOCFAIL","Area (%d %s) Memory Allocation failure bkt=%d",
		aidflg,UCretASC(mmm->Areas[ax].UCFileName),numid) ;
	sbh = (struct V4__BktHdr *)mmm->Bkts[bx].BktPtr ;
	mmm->Bkts[bx].AreaIndex = ax ;
/*	Read in the bucket & return pointer */
	v4mm_BktRead(ax,mmm->Bkts[bx].BktNum,mmm->Bkts[bx].BktPtr) ;
	mmm->Bkts[bx].CallCnt = (mmm->TotalCalls++) ;		/* Update access count */
	mmm->Areas[ax].NumBkts ++ ;				/* Increment buckets for this area */
	if (flags & V4MM_BktPtr_Update) mmm->Bkts[bx].Updated = TRUE ;
	if (flags & V4MM_BktPtr_MemLock)
	 { mmm->Bkts[bx].MemLock = TRUE ;			/* Have to lock the bucket */
	 } ;
	sbh->InUse = 1 ;					/* Set inuse to 1 user */
	return(mmm->Bkts[bx].BktPtr) ;
}

#ifdef V4MM_GlobalTrace
v4mm_DumpTrace(glb)
  struct V4MM__GlobalBuf *glb ;
{ int tx,tx1,ctx ;

	ctx = glb->LastTrace ;
	for(tx1=ctx;;)
	 { tx = tx1 % V4MM_GlobalTrace ;
	   printf("where=%d pid=%d bx=%d lockid=%d callcnt=%d bktnum=%d sbh=%d glb=%d\n",
		   glb->Trace[tx].Where,glb->Trace[tx].Pid,glb->Trace[tx].bx,glb->Trace[tx].LockId,
		   glb->Trace[tx].CallCnt,glb->Trace[tx].BBktNum,glb->Trace[tx].sbhBktNum,glb->Trace[tx].glbBktNum) ;
	   tx1-- ; if (tx1 < 0) break ; if (ctx-tx1 >= V4MM_GlobalTrace) break ;
	 } ;
}
#endif

#ifdef CHECKALLOC
v4mm_CheckALLOC(lt,MyPid)
  struct V4MM__LockTable *lt ;
  pid_t MyPid ;
{ struct V4MM__MemMgmtMaster *mmm ;
  int i,j ;

	for(i=0;i<lt->Count;i++)
	 { if (lt->Entry[i].Pids[0] != MyPid) continue ;
	   if (lt->Entry[i].LockIdType != V4MM_LockIdType_Alloc) continue ;
	   if (lt->Entry[i].PidCount <= 0) continue ;
	   mmm = V4_GLOBAL_MMM_PTR ;
	   printf("CheckALLOC found problem: i=%d, Area=%d, ULI=%d MyPid=%d\n",
			i,lt->Entry[i].AreaIds[0],lt->Entry[i].UniqueLockId,mmm->MyPid) ;
	   for(j=0;j<lt->Count;j++)
	    { printf("? Lock #%d type=%d, mode=%d, id=%d, uid=%d, pids=%d (%d %d %d), wait=%d (%d), Tree=%d\n",
		j,lt->Entry[j].LockIdType,lt->Entry[j].LockMode,lt->Entry[j].LockId,lt->Entry[j].UniqueLockId,
		lt->Entry[j].PidCount,lt->Entry[j].Pids[0],lt->Entry[j].Pids[1],lt->Entry[j].Pids[2],lt->Entry[j].WaitCount,
		lt->Entry[j].WaitPids[0],lt->LocksTree) ;
	    } ;
	   v4mm_LockRelease(lt->Entry[i].AreaIds[0],lt->Entry[i].UniqueLockId,mmm->MyPid,lt->Entry[i].AreaIds[0],"CheckALLOC") ;
	   return(TRUE) ;
	 } ;
	return(FALSE) ;
}
#endif

/*	v4mm_GlobalBufGrab - Grabs a global bucket (maybe flushes old if necessary) */
/*	Call: index = v4mm_GlobalBufGrab( ax , bktnum )
	  where index is index into buffer space where this bucket belongs,
		ax is area index,
		bktnum is bucket number to be placed into this spot (not handled in this module)	*/

int v4mm_GlobalBufGrab(ax,bktnum)
  int ax,bktnum ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__GlobalBuf *glb ;
  struct V4MM__LockTable *lt ;
  int i,bx,minx,mincc,tries,tx,areaid,ok ; char bktlocks[V4MM_GlobalBufBkt_Max] ; char ebuf[250] ;

	GETMMMFROMAREAX(mmm,ax) ;
//	mmm = CURRENTMMM ;
	GETGLB(ax) ;
	lt = (struct V4MM__LockTable *)&glb->Buffer[glb->LockTableOffset] ;	/* Get pointer to lock table */
	areaid = mmm->Areas[ax].AreaId ;
	memset(&bktlocks,0,V4MM_GlobalBufBkt_Max) ;			/* Zero out array */
	for(tries=0;tries<500;tries++)
	 { mincc = V4LIM_BiggestPositiveInt ; minx = -1 ; LOCKLT(5)			/* Lock down lock table, init for min */
	   for(i=0;i<lt->Count;i++)
	    { if (lt->Entry[i].LockIdType != V4MM_LockIdType_Alloc) continue ;
	      if (lt->Entry[i].PidCount != 1) continue ; if (lt->Entry[i].Pids[0] == mmm->MyPid) break ;
	    } ;
	   if (i >= lt->Count)
	    { v4is_BugChk(mmm->Areas[ax].AreaId,"In GlobalBufGrab without proper lock") ; RELLT(7) return(-1) ; } ;
	   for(i=0;i<lt->Count;i++)					/* Make sure bucket we want is not locked */
	    { if (lt->Entry[i].LockId != bktnum) continue ;
	      if (lt->Entry[i].PidCount == 1 && lt->Entry[i].Pids[0] == mmm->MyPid)
	       { sprintf(ebuf,"BufGrab of bkt (%d), lt->Entry[%d] belongs to this process, ignoring",bktnum,i) ;
		 v4is_BugChk(areaid,ebuf) ;
	         continue ;						/* If we have locked (???) ignore & hope for best */
	       } ;
	      RELLT(71) return(-1) ;					/* Bucket locked by other process- can't grab */
	    } ;
	   for(bx=0;bx<glb->BktCnt;bx++)				/* Look for empty or least recently used */
	    { if (glb->Bkt[bx].BktNum == UNUSED) break ;		/* Bucket unused */
	      if (glb->Bkt[bx].BktNum == PENDING)			/* Bucket about to be allocated to somebody else */
	       { if (glb->Bkt[bx].PendingBktNum == bktnum)		/* Pending bucket is one we want ? */
		  { bx = V4LIM_BiggestPositiveInt ; minx = -1 ; break ; }			/*  then don't try to get it again! */
		  else { continue ; } ;
	       } ;
	      if (glb->Bkt[bx].PendingPid != UNUSEDPID)			/* Should not get this condition */
	       { /* printf("?GlobalBufGrab, bx=%d, BktNum=%d, PendingPid=%d, TotalCalls=%d ...ignoring bucket\n",
			bx,glb->Bkt[bx].BktNum,glb->Bkt[bx].PendingPid,glb->TotalCalls) ; */
		 continue ;
	       } ;
	      if (glb->Bkt[bx].CallCnt > mincc) continue ;		/* Bucket used more recently than ... */
	      if (glb->Bkt[bx].MemLock) continue ;			/* Bucket locked in memory (like the root) */
	      if (glb->Bkt[bx].Updated)
	       { if (!mmm->Areas[ax].WriteAccess) continue ; } ;	/* Bucket Updated, we don't have write access- skip */
	      if (bktlocks[bx] > 5) continue ;				/* Don't beat a dead horse */
	      for(i=0;i<lt->Count;i++)
	       { if (lt->Entry[i].LockId == glb->Bkt[bx].BktNum) break ; } ;
	      if (i < lt->Count) { bktlocks[bx]++ ; continue ; } ;	/* Bucket currently locked - skip it */
	      mincc = glb->Bkt[bx].CallCnt ; minx = bx ;		/* Got a likely candidate */
	    } ; RELLT(8)					/* Release lock table */
	   if (bx >= glb->BktCnt)					 /* Have to get rid of old contents and update to new */
	    { if (minx < 0)
	       { for(bx=0;bx<glb->BktCnt;bx++)			/* See if all pending reads are valid */
		  { if (glb->Bkt[bx].BktNum != PENDING) continue ;
		    if (!PROCESS_GONE(glb->Bkt[bx].PendingPid)) continue ;
		    glb->Bkt[bx].BktNum = UNUSED ;		/* PendingPid gone - trash bucket */
		    sprintf(ebuf,"BufGrab- trashing pending bkt (%d %d) - defunct pid %d\n",
			bx,glb->Bkt[bx].PendingBktNum,glb->Bkt[bx].PendingPid) ;
		    v4is_BugChk(areaid,ebuf) ;
	  	  } ;
	         return(-1) ;					/* Could not allocate buffer ? */
	       } ;
	      bx = minx ;
	      if (glb->Bkt[bx].BktNum == 0) v4is_BugChk(mmm->Areas[ax].AreaId,"Attempt to flush bucket 0") ;
	      ok = v4mm_BktFlush(glb->Bkt[bx].BktNum,mmm->Areas[ax].AreaId) ;	/* Could not flush out bucket??? */
	      LOCKLT(201)
	      for(i=0;i<lt->Count;i++)
	       { if (lt->Entry[i].LockIdType != V4MM_LockIdType_Alloc) continue ;
	         if (lt->Entry[i].PidCount != 1) continue ; if (lt->Entry[i].Pids[0] == mmm->MyPid) break ;
	       } ;
	      if (i >= lt->Count)
	       { v4is_BugChk(mmm->Areas[ax].AreaId,"In GlobalBufGrab (after BktFlush) without proper lock") ;
		 RELLT(202) return(-1) ;
	       } ;
	      RELLT(203)
	      if (!ok) { bktlocks[bx]++ ; continue ; } ;				/*  then have to start all over again! */
	    } ;
	   if (glb->Bkt[bx].Updated)
	    { sprintf(ebuf,"GlobalBufGrab got bx=%d (to plug bkt=%d), but still flagged updated",bx,bktnum) ;
	      v4is_BugChk(mmm->Areas[ax].AreaId,ebuf) ; break ;
	    } ;
	   if (glb->Bkt[bx].BktNum == PENDING)
	    { sprintf(ebuf,"Attempt to grab PENDING bucket bx=%d, bktnum=%d, PendingBkt=%d, PendingPid=%d, MyPid=%d",
			bx,bktnum,glb->Bkt[bx].PendingBktNum,glb->Bkt[bx].PendingPid,mmm->MyPid) ;
	      v4is_BugChk(mmm->Areas[ax].AreaId,ebuf) ; break ;
	    } ;
	   glb->Bkt[bx].BktNum = PENDING ; glb->Bkt[bx].PendingBktNum = bktnum ; glb->Bkt[bx].PendingPid = mmm->MyPid ;
	   glb->Bkt[bx].CallCnt = glb->TotalCalls++ ; glb->Bkt[bx].Updated = FALSE ;
#ifdef V4MM_GlobalTrace
	   tx = (++glb->LastTrace) % V4MM_GlobalTrace ; glb->Trace[tx].CallCnt = glb->Bkt[bx].CallCnt ;
	   glb->Trace[tx].BBktNum = bktnum ; glb->Trace[tx].sbhBktNum = 0 ;
	   glb->Trace[tx].bx = bx ; glb->Trace[tx].Where = 6 ; glb->Trace[tx].Pid = mmm->MyPid ; glb->Trace[tx].LockId = 0 ;
	   glb->Trace[tx].glbBktNum = glb->Bkt[bx].BktNum ;
#endif
	   return(bx) ;							/* Return pointer to bucket */
	 } ;
/*	If here then big trouble - dump out what we know & quit */
	v4is_BugChk(mmm->Areas[ax].AreaId,"Global buffer grab appears to be hung") ;
	v4_error(V4E_GRABHUNG,0,"V4MM","GLBBUFGRB","GRABHUNG","Global buffer grab appears to be hung") ;
	return(UNUSED) ;
}

/*	v4mm_AreaFlushOld - Flushes oldest bucket out of an area */
/*	Call: v4mm_AreaFlushOld( areaindex )
	  where areaindex is index to area to be flushed, (UNUSED for global flush)	*/

void v4mm_AreaFlushOld(areaindex)
  int areaindex ;
{ struct V4MM__MemMgmtMaster *mmm ;
  int bx,obx,mincnt ;

	GETMMMFROMAREAX(mmm,areaindex) ;
//	mmm = CURRENTMMM ;
try_again:
	mincnt = V4LIM_BiggestPositiveInt ; obx = -1 ;
	if (areaindex != UNUSED)
	 { for(bx=0;bx<mmm->NumBkts;bx++)
	    { if (mmm->Bkts[bx].AreaIndex != areaindex) continue ;
	      if (mmm->Bkts[bx].CallCnt >= mincnt) continue ;
	      if (mmm->Bkts[bx].MemLock) continue ;		/* Don't flush locked buckets (e.g. root) */
	      obx = bx ; mincnt = mmm->Bkts[bx].CallCnt ;		/* Track oldest one */
	    } ;
	   if (obx >= 0)
	    { if (!v4mm_BktFlush(mmm->Bkts[obx].BktNum,mmm->Areas[areaindex].AreaId)) goto try_again ;
	      return ;
	    } ;
	 } ;
/*	Was not able to flush out anything in current area- if close to limit then flush global oldest */
	if (mmm->NumBkts < V4MM_AreaBkt_Max - 1) return ;
try_again1:
	mincnt = V4LIM_BiggestPositiveInt ; obx = -1 ;
	for(bx=0;bx<mmm->NumBkts;bx++)
	 { if (mmm->Bkts[bx].CallCnt >= mincnt) continue ;
	   if (mmm->Bkts[bx].BktNum == UNUSED) continue ;	/* Don't flush one that is already flushed */
	   if (mmm->Bkts[bx].MemLock) continue ;		/* Don't flush locked buckets (e.g. root) */
	   if (mmm->Bkts[bx].AreaIndex == UNUSED || mmm->Bkts[bx].BktPtr == NULL) continue ;	/* Don't flush empty bucket */
	   obx = bx ; mincnt = mmm->Bkts[bx].CallCnt ;		/* Track oldest one */
	 } ;
	if (obx >= 0 && mmm->Bkts[obx].BktNum != UNUSED)
	 { if (!v4mm_BktFlush(mmm->Bkts[obx].BktNum,mmm->Areas[mmm->Bkts[obx].AreaIndex].AreaId)) goto try_again1 ;
	   return ;
	 } ;
}

/*	v4mm_TempLock - Sets/Resets temporary memory lock flag for a bucket (and sets UPDATE flag)	*/
/*	Call: v4mm_TempLock( areaid , bktnum , mode )
	  where areaid is the area id,
		bktnum is the bucket under consideration,
		mode is TRUE to set lock, FALSE to reset			*/

void v4mm_TempLock(areaid,bktnum,mode)
  int areaid,bktnum,mode ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__GlobalBuf *glb ;
  int ax,bx ;

	FINDAREAINPROCESS(ax,areaid)
	if (ax == UNUSED) return ;			/* Could not find area? */
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++)
//	 { if (mmm->Areas[ax].AreaId == areaid) break ; } ;
//	if (ax >= mmm->NumAreas) return ;		/* Could not find area? */
	if (mmm->Areas[ax].SegId != 0)				/* Global Buffers */
	 { if ( GETGLB(ax) == NULL)
	    v4_error(V4E_AREANOTGLOBAL,0,"V4MM","TempLock","AREANOTGLOBAL","Area (%d %s) not global",ax,UCretASC(mmm->Areas[ax].UCFileName)) ;
	   for(bx=0;bx<glb->BktCnt;bx++)
	    { if (glb->Bkt[bx].BktNum != bktnum) continue ;
	      glb->Bkt[bx].Updated = TRUE ;			/* Set update flag for this buffer */
	      if (mode) { glb->Bkt[bx].MemLock |= LOCKTEMP ; } else glb->Bkt[bx].MemLock &= (~LOCKTEMP) ;
	      break ;
	    } ;
	   return ;
	 } ;
/*	Look for the bucket in in-core pool */
	for(bx=0;bx<mmm->NumBkts;bx++)
	 { if (mmm->Bkts[bx].BktNum != bktnum) continue ; if (mmm->Bkts[bx].AreaIndex != ax) continue ;
	   if (mode) { mmm->Bkts[bx].MemLock |= LOCKTEMP ; } else mmm->Bkts[bx].MemLock &= (~LOCKTEMP) ;
	   mmm->Bkts[bx].Updated = TRUE ;			/* Set update flag */
	   return ;
	 } ;
}

/*	v4mm_BktUpdated - Sets Update flag on a bucket */
/*	Call: v4mm_BktUpdated( areaid , bktnum )
	  where areaid is the area id,
		bktnum is the bucket to be flaggged as updated 	*/

int v4mm_BktUpdated(areaid,bktnum)
  int areaid,bktnum ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__GlobalBuf *glb ;
  int ax,bx ;

	FINDAREAINPROCESS(ax,areaid)
	if (ax == UNUSED) return(-1) ;			/* Could not find area? */
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++)
//	 { if (mmm->Areas[ax].AreaId == areaid) break ; } ;
//	if (ax >= mmm->NumAreas) return(-1) ;		/* Could not find area? */
	if (mmm->Areas[ax].SegId != 0)				/* Global Buffers */
	 { if ( GETGLB(ax) == NULL)
	    v4_error(V4E_AREANOTGLOBAL,0,"V4MM","TempLock","AREANOTGLOBAL","Area (%d %s) not global",ax,UCretASC(mmm->Areas[ax].UCFileName)) ;
	   for(bx=0;bx<glb->BktCnt;bx++)
	    { if (glb->Bkt[bx].BktNum != bktnum) continue ;
	      glb->Bkt[bx].Updated = TRUE ;			/* Set update flag for this buffer */
	      return(bx) ;
	    } ;
	   return(-1) ;
	 } ;
/*	Look for the bucket in in-core pool */
	for(bx=0;bx<mmm->NumBkts;bx++)
	 { if (mmm->Bkts[bx].BktNum != bktnum) continue ; if (mmm->Bkts[bx].AreaIndex != ax) continue ;
	   mmm->Bkts[bx].Updated = TRUE ;			/* Set update flag */
	   return(bx) ;
	 } ;
	return(-1) ;
}

/*	B U C K E T   I / O   M O D U L E S				*/

/*	v4mm_BktRead - Reads data into specified bucket			*/
/*	Call: v4mm_BktRead( aix , bktnum , bktptr )
	  where aix is area index,
		bktnum is bucket number,
		bktptr is pointer to bucket buffer			*/

void v4mm_BktRead(aix,bktnum,bktptr)
  int aix,bktnum ;
  void *bktptr ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__RootInfo *ri ;
  struct V4__BktHdr *sbh ;
  char ebuf[250] ;
  int bn,i,err ;
#ifdef WINNT
  LARGE_INTEGER li ;
  DWORD bytes ;
  HANDLE hfile ;
#else
  FILE *fp ; int fn ;
#endif

//if (bktnum > 100000)
// printf("??? bktnum = %d\n",bktnum) ;
	GETMMMFROMAREAX(mmm,aix) ;
//	mmm = CURRENTMMM ;
#ifdef WINNT
	if (mmm->Areas[aix].IndexOnlyCount == 0)
	 { hfile = mmm->Areas[aix].hfile ; bn = bktnum ; }
	 else { hfile = mmm->Areas[aix].hfile ; bn = bktnum ;
		for(i=0;i<mmm->Areas[aix].IndexOnlyCount;i++)	/* See if we should link to another area */
		 { if (bn < mmm->Areas[aix].IndexOnly[i].BktOffset) break ; } ;
		if (i > 0)					/* Is bucket number up in range for index-only link ? */
		 { bn -= mmm->Areas[aix].IndexOnly[i-1].BktOffset ; hfile = mmm->Areas[aix].IndexOnly[i-1].iohfile ; } ;
	      } ;
	li.QuadPart = (__int64)((__int64)bktnum * (__int64)mmm->Areas[aix].BktSize) ;
	if (SetFilePointer(hfile,li.LowPart,&li.HighPart,FILE_BEGIN) == 0xFFFFFFFF && GetLastError() != NO_ERROR)
	 { printf("SetFilePointer error %s, bkt =%d/%d\n",v_OSErrString(GetLastError()),bn,bktnum) ;
	   v4_error(V4E_BKTSEEKFAIL1,0,"V4MM","BktRead","BKTSEEKFAIL1","Area (%d %s) SetFilePointer failure for bucket (%d/%d)",
			aix,UCretASC(mmm->Areas[aix].UCFileName),bn,bktnum) ;
	 } ;
	if (!ReadFile(hfile,bktptr,mmm->Areas[aix].BktSize,&bytes,NULL))
	 { err = GetLastError() ;
	   sprintf(ebuf,"ReadFile err (%s), bkt=%d/%d, bktsize=%d, filepos=%d",
		v_OSErrString(err),bn,bktnum,mmm->Areas[aix].BktSize,bktnum*mmm->Areas[aix].BktSize) ;
	   v4is_BugChk(mmm->Areas[aix].AreaId,ebuf) ;
	   v4_error(V4E_BKTREADFAIL,0,"V4MM","BktRead","BKTREADFAIL","Area (%d %s) ReadFile failure (%s) for bucket (%d/%d)",
			aix,UCretASC(mmm->Areas[aix].UCFileName),v_OSErrString(err),bn,bktnum) ;
	 } ;
	if (bytes != mmm->Areas[aix].BktSize)
	 v4_error(V4E_BKTREADFAIL,0,"V4MM","BktRead","BKTREADFAIL","Attempt to read past EOF (bkt=%d, bktSize=%d, file=%s)",bktnum,mmm->Areas[aix].BktSize,UCretASC(mmm->Areas[aix].UCFileName)) ;
#else
	if (mmm->Areas[aix].IndexOnlyCount == 0)
	 { fp = mmm->Areas[aix].FilePtr ; bn = bktnum ; }
	 else { fp = mmm->Areas[aix].FilePtr ; bn = bktnum ;
		for(i=0;i<mmm->Areas[aix].IndexOnlyCount;i++)	/* See if we should link to another area */
		 { if (bn < mmm->Areas[aix].IndexOnly[i].BktOffset) break ; } ;
		if (i > 0)					/* Is bucket number up in range for index-only link ? */
		 { bn -= mmm->Areas[aix].IndexOnly[i-1].BktOffset ; fp = mmm->Areas[aix].IndexOnly[i-1].ioFilePtr ; } ;
	      } ;
reread_bucket:
	fn = fileno(fp) ;			/* Try to re-read bucket using raw UNIX IO */
	if (lseek(fn,(off_t)((off_t)bn*(off_t)mmm->Areas[aix].BktSize),0) == -1)
	 { printf("lseek error %s, bkt =%d\n",v_OSErrString(errno),bn) ;
	   v4_error(V4E_BKTSEEKFAIL,0,"V4MM","BktRead","BKTSEEKFAIL","Area (%d %s) lseek failure (%s) for bucket (%d/%d)",
			aix,UCretASC(mmm->Areas[aix].UCFileName),v_OSErrString(errno),bn,bktnum) ;
	 } ;
	if (read(fn,bktptr,mmm->Areas[aix].BktSize) != mmm->Areas[aix].BktSize)
	 { err = errno ;
	   sprintf(ebuf,"read err (%s), bkt=%d, bktsize=%d, filepos=%d",
		v_OSErrString(err),bn,mmm->Areas[aix].BktSize,(off_t)(bn*mmm->Areas[aix].BktSize)) ;
	   v4is_BugChk(mmm->Areas[aix].AreaId,ebuf) ;
	   v4_error(V4E_BKTREADFAIL,0,"V4MM","BktRead","BKTREADFAIL","Area (%d %s) read failure (%s) for bucket (%d/%d)",
			aix,UCretASC(mmm->Areas[aix].UCFileName),v_OSErrString(err),bn,bktnum) ;
	 } ;
#ifdef NOREADSTUFF
	if (fseek(fp,(FSEEKINT)((FSEEKINT)bn*(FSEEKINT)mmm->Areas[aix].BktSize),0) != 0)
	 { printf("fseek error %s, bkt =%d\n",v_OSErrString(errno),bn) ;
	   v4_error(V4E_BKTSEEKFAIL,0,"V4MM","BktRead","BKTSEEKFAIL","Area (%d %s) fseek failure (%s) for bucket (%d/%d)",
			aix,UCretASC(mmm->Areas[aix].UCFileName),v_OSErrString(errno),bn,bktnum) ;
	 } ;
	if (fread(bktptr,mmm->Areas[aix].BktSize,1,fp) <= 0)
	 { sprintf(ebuf,"fread err (%d), bkt=%d, bktsize=%d, filepos=%d",
		errno,bn,mmm->Areas[aix].BktSize,bn*mmm->Areas[aix].BktSize) ;
	   v4is_BugChk(mmm->Areas[aix].AreaId,ebuf) ;
	   v4_error(V4E_BKTREADFAIL,0,"V4MM","BktRead","BKTREADFAIL","Area (%d %s) fread failure (%s) for bucket (%d/%d)",
			aix,UCretASC(mmm->Areas[aix].UCFileName),v_OSErrString(errno),bn,bktnum) ;
	 } ;
#endif
#endif
	sbh = (struct V4__BktHdr *)bktptr ;
	if (sbh->BktNum != bn)
	 { if (bn == 0 && sbh->BktNum > 1000000)
	    { printf("Area (%s) does not appear to be in valid V4IS format\n",UCretASC(mmm->Areas[aix].UCFileName)) ; }
	    else { printf("Attempt to get bkt #%d (%s) has resulted in #%d\n",bn,UCretASC(mmm->Areas[aix].UCFileName),sbh->BktNum) ; } ;
	   v4_error(V4E_BKTHDRBAD,0,"V4MM","BktRead","BKTHDRBAD","Area (%d %s) Bucket header (%d) does not match request bucket (%d)",
			aix,UCretASC(mmm->Areas[aix].UCFileName),sbh->BktNum,bn) ;
	 } ;
	mmm->Areas[aix].BktReads++ ;
	acb = (struct V4IS__AreaCB *)v4mm_ACBPtr(mmm->Areas[aix].AreaId) ; ri = acb->RootInfo ;
 	if (mmm->Areas[aix].GBPtr != NULL)
 	 { IFTRACEAREA
	    { 
	      if (mmm->Areas[aix].wvb != NULL)			/* Keep track of write version number */
	       { if (bktnum < V4MM_WriteVerBuf_Max)
	          { if (mmm->Areas[aix].wvb->WriteCnt[bktnum] != 0 && mmm->Areas[aix].wvb->WriteCnt[bktnum] != sbh->Unused)
		     { sprintf(ebuf,"WVB Failure in BktRead(bkt=%d U=%d aix=%d wvb=%d)",
				bktnum,sbh->Unused,aix,mmm->Areas[aix].wvb->WriteCnt[bktnum]) ;
		       v4is_BugChk(mmm->Areas[aix].AreaId,ebuf) ;
#ifdef UNIX
		       fn = fileno(fp) ;			/* Try to re-read bucket using raw UNIX IO */
		       if (lseek(fn,(off_t)(bn*mmm->Areas[aix].BktSize),0) == -1) goto reread_bucket ;
		       if (read(fn,bktptr,mmm->Areas[aix].BktSize) != mmm->Areas[aix].BktSize) goto reread_bucket ;
		       sprintf(ebuf,"Using lseek/read in BktRead(bkt=%d U=%d aix=%d wvb=%d)",
				bktnum,sbh->Unused,aix,mmm->Areas[aix].wvb->WriteCnt[bktnum]) ;
		       v4is_BugChk(mmm->Areas[aix].AreaId,ebuf) ;
		       if (mmm->Areas[aix].wvb->WriteCnt[bktnum] != sbh->Unused)
		        { sprintf(ebuf,"Still have WVB Failure in BktRead(bkt=%d U=%d aix=%d wvb=%d)",
				bktnum,sbh->Unused,aix,mmm->Areas[aix].wvb->WriteCnt[bktnum]) ;
		          v4is_BugChk(mmm->Areas[aix].AreaId,ebuf) ;
			} ;
		       mmm->Areas[aix].wvb->WriteCnt[bktnum] = sbh->Unused ;	/* Force this to prevent multiple retries */
#endif
		     } ;
		  } ;
	       } ;
	    } ;
	 } ;
}

/*	v4mm_BktWrite - Writes out specified bucket			*/
/*	Call: v4mm_BktWrite( aix , bktnum , bktptr , srcmod , index )
	  where aix is area index,
		bktknum is bucket number,
		bktptr is point to bucket buffer,
		srcmod is name of source/calling module,
		index is index within buffer pool			*/

void v4mm_BktWrite(aix,bktnum,bktptr,srcmod,index)
  int aix,bktnum ;
  void *bktptr ;
  char *srcmod ; int index ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4C__ProcessInfo *pi ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__RootInfo *ri ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__Bkt *dbkt ;
  struct V4IS__DataBktHdr *dbh ;
  struct V4IS__DataBktEntryHdr *dbeh ;
  union V4MM__DataIdMask dim ;
  struct V4__BktHdr *sbh ;
  int bx ; char ebuf[150] ;
#ifdef WINNT
  LARGE_INTEGER li ;
  int bytes ;
#endif

	GETMMMFROMAREAX(mmm,aix) ;
//	mmm = CURRENTMMM ;
	acb = (struct V4IS__AreaCB *)v4mm_ACBPtr(mmm->Areas[aix].AreaId) ;
	sbh = (struct V4__BktHdr *)bktptr ;
	ri = acb->RootInfo ;
 	if (mmm->Areas[aix].GBPtr != NULL)
 	 { IFTRACEAREA
	    { 
	      ++sbh->Unused ;		/* Increment as check for possibly duplicate buckets??? */
	      if (sbh->BktType == V4_BktHdrType_Index)
	       { ibh = (IBH *)bktptr ;
	         sprintf(ebuf,"Entering BktWrite(bkt=%d U=%d bx=%d) from %s (Top=%d Num=%d Free=%d)",
				bktnum,ibh->sbh.Unused,index,srcmod,ibh->KeyEntryTop,ibh->KeyEntryNum,ibh->FreeBytes) ;
	       } else { sprintf(ebuf,"Entering BktWrite(bkt=%d U=%d bx=%d) from %s",bktnum,sbh->Unused,index,srcmod) ; } ;
	      if (mmm->Areas[aix].wvb != NULL)			/* Keep track of write version number */
	       { if (bktnum < V4MM_WriteVerBuf_Max) mmm->Areas[aix].wvb->WriteCnt[bktnum] = sbh->Unused ;
	       } ;
	    } ;
	 } ;
	if (sbh->BktNum != bktnum)
	 { printf("Attempt from %s to write bkt #%d has resulted in #%d\n",srcmod,bktnum,sbh->BktNum) ;
	   v4mm_ZapBucket(aix,bktnum) ;		/* Zap in-core bucket so we don't get gazillion errors */
	   v4_error(V4E_BKTNOTMATCH,0,"V4MM","BktWrite","BKTNOTMATCH","Area (%d %s) Bucket (%s/%d) header (%d) does not match (%d)",
			aix,UCretASC(mmm->Areas[aix].UCFileName),srcmod,index,sbh->BktNum,bktnum) ;
	 } ;
	switch (sbh->BktType)				/* Perform some sanity checks before writing out */
	 { default:
		v4mm_ZapBucket(aix,bktnum) ;		/* Zap in-core bucket so we don't get gazillion errors */
		v4_error(V4E_WRITEINVBKTTYPE,0,"V4MM","BktWrite","WRITEINVBKTTYPE","Area (%d %s) Bucket Type (%d) not valid in bucket (%d)",
				aix,UCretASC(mmm->Areas[aix].UCFileName),sbh->BktType,bktnum) ;
	   case V4_BktHdrType_SeqOnly:	break ;
//	   case V4_BktHdrType_Unused:	break ;		/* Same as SeqOnly */
	   case V4_BktHdrType_Root:
		ibh = (IBH *)bktptr ;
	   case V4_BktHdrType_Index:
		ibh = (IBH *)bktptr ;
		if (ibh->KeyEntryTop < sizeof *ibh || ibh->KeyEntryTop > mmm->Areas[aix].BktSize / 2 ||
		    ibh->KeyEntryNum < 0 || ibh->KeyEntryNum > mmm->Areas[aix].BktSize / (sizeof (IKDE) + V4IS_IntKey_Bytes) ||
		    ibh->FreeBytes < 0 || ibh->FreeBytes >  mmm->Areas[aix].BktSize)
		 { v4mm_ZapBucket(aix,bktnum) ;		/* Zap in-core bucket so we don't get gazillion errors */
		   v4_error(V4E_INVIBH,0,"V4MM","BktWrite","INVIBH","Area (%d %s) Bucket (%d) KeyTop (%d) KeyNum (%d) Free (%d) Size (%d) bad",
			  aix,UCretASC(mmm->Areas[aix].UCFileName),bktnum,ibh->KeyEntryTop,ibh->KeyEntryNum,ibh->FreeBytes,
			  mmm->Areas[aix].BktSize) ;
		 } ;
		if (ibh->DataTop > mmm->Areas[aix].BktSize || DATATOPVAL(ibh->DataTop) < ibh->KeyEntryNum)
		 { v4mm_ZapBucket(aix,bktnum) ;		/* Zap in-core bucket so we don't get gazillion errors */
		   v4_error(V4E_INVDATATOP,0,"V4MM","BktWrite","INVDATATOP","Area (%d %s) Bucket (%d) DataTop (%d) bad",
			  aix,UCretASC(mmm->Areas[aix].UCFileName),bktnum,ibh->DataTop) ;
		 } ;
	   	ibkl = (IBKL *)&(((struct V4IS__Bkt *)bktptr)->Buffer[ibh->KeyEntryTop]) ;
		if (ibh->KeyEntryNum > 0)
		 { if (ibkl->DataIndex[0] < sizeof *ibh || ibkl->DataIndex[0] > mmm->Areas[aix].BktSize ||
		       ibkl->DataIndex[ibh->KeyEntryNum-1] < sizeof *ibh ||
		       ibkl->DataIndex[ibh->KeyEntryNum-1] > mmm->Areas[aix].BktSize)
		    { v4mm_ZapBucket(aix,bktnum) ;		/* Zap in-core bucket so we don't get gazillion errors */
		      v4_error(V4E_INVDATAINDEX,0,"V4MM","BktWrite","INVDATAINDEX","Area (%d %s) Bucket (%d) Key0x (%d) Key%dx (%d) bad",
			  aix,UCretASC(mmm->Areas[aix].UCFileName),bktnum,ibkl->DataIndex[0],ibh->KeyEntryNum,ibkl->DataIndex[ibh->KeyEntryNum-1]) ;
		    } ;
		 } ;
		break ;
	   case V4_BktHdrType_Data:
		dbkt = (struct V4IS__Bkt *)bktptr ; dbh = (DBH *)dbkt ; /* Link to data bucket */
		if (dbh->FreeBytes < 0 || dbh->FreeBytes > mmm->Areas[aix].BktSize)
		 v4_error(V4E_INVFREEBYTES,0,"V4MM","BktWrite","INVFREEBYTES","Area (%d %s) Bucket (%d) Free Bytes (%d) exceeds bucket size (%d)",
			aix,UCretASC(mmm->Areas[aix].UCFileName),bktnum,dbh->FreeBytes,mmm->Areas[aix].BktSize) ;
		if (dbh->FirstEntryIndex > mmm->Areas[aix].BktSize || dbh->FirstEntryIndex < sizeof *dbh ||
		    dbh->FirstEntryIndex > dbh->FreeEntryIndex || dbh->FreeEntryIndex < sizeof *dbh ||
		    dbh->FreeEntryIndex > mmm->Areas[aix].BktSize)
		 { v4mm_ZapBucket(aix,bktnum) ;		/* Zap in-core bucket so we don't get gazillion errors */
		   v4_error(V4E_INVDBH,0,"V4MM","BktWrite","INVDBH","Area (%d %s) Bkt (%d) Data Header First (%d) Free (%d) Bktsize (%d) bad",
			aix,UCretASC(mmm->Areas[aix].UCFileName),bktnum,dbh->FirstEntryIndex,dbh->FreeEntryIndex,mmm->Areas[aix].BktSize) ;
		 } ;
		for(bx=dbh->FirstEntryIndex;bx<dbh->FreeEntryIndex;bx+=ALIGN(dbeh->Bytes))
		 { dbeh = (struct V4IS__DataBktEntryHdr *)&dbkt->Buffer[bx] ;
		   if (dbeh->Bytes <= sizeof *dbeh || dbeh->Bytes > ri->BktSize)
		    { sprintf(ebuf,"Bucket (%d) dbeh->Bytes=%d, min is %d, max is %d",bktnum,(int)dbeh->Bytes,(int)sizeof *dbeh,ri->BktSize) ;
		      v4is_BugChk(mmm->Areas[aix].AreaId,ebuf) ;
		      v4mm_ZapBucket(aix,bktnum) ;		/* Zap in-core bucket so we don't get gazillion errors */
		      v4_error(V4E_INVDBEHLEN,0,"V4MM","BktWrite","INVDBEHLEN","Area (%d %s) Bucket (%d) invalid dbeh->Bytes",
				mmm->Areas[aix].AreaId,UCretASC(mmm->Areas[aix].UCFileName),dbh->sbh.BktNum) ;
		    } ;
		   dim.dataid = dbeh->DataId ;
		   if (dim.fld.BktNum != bktnum)
		    { sprintf(ebuf,"Bucket (%d) dbeh->DataId=%x links to bucket %d",bktnum,dbeh->DataId,dim.fld.BktNum) ;
		      v4is_BugChk(mmm->Areas[aix].AreaId,ebuf) ;
		      v4mm_ZapBucket(aix,bktnum) ;		/* Zap in-core bucket so we don't get gazillion errors */
		      v4_error(V4E_INVDATAIDBKT,0,"V4MM","BktWrite","INVDATAIDBKT","Area (%d %s) Bucket (%d), DataId (%x) -> bkt %d",
				mmm->Areas[aix].AreaId,UCretASC(mmm->Areas[aix].UCFileName),dbh->sbh.BktNum,dbeh->DataId,dim.fld.BktNum) ;
		    } ;
		   if (dim.fld.BktSeq > dbh->sbh.AvailBktSeq)
		    { sprintf(ebuf,"Bucket (%d) dbeh->DataId=%x has sequence (%d) > header sequence (%d)",
				bktnum,dbeh->DataId,dim.fld.BktSeq,dbh->sbh.AvailBktSeq) ;
		      v4is_BugChk(mmm->Areas[aix].AreaId,ebuf) ;
		      v4mm_ZapBucket(aix,bktnum) ;		/* Zap in-core bucket so we don't get gazillion errors */
		      v4_error(V4E_INVDATAIDSEQ,0,"V4MM","BktWrite","INVDATAIDSEQ","Area (%d %s) Bucket (%d), DataId (%x) - Invalid sequence",
				mmm->Areas[aix].AreaId,UCretASC(mmm->Areas[aix].UCFileName),dbh->sbh.BktNum,dbeh->DataId) ;
		    } ;
		 } ;
		if (bx == V4IS_BktSize_Max) bx-- ;	/* VEH100129 - Adjust if right at bucket max */
		if ((bx & V4IS_BktSize_Max_Mask) != dbh->FreeEntryIndex)
		 { sprintf(ebuf,"Bucket (%d) ending data index (%d) <> dbh->FreeEntryIndex (%d)",
			bktnum,bx,dbh->FreeEntryIndex) ;
		   v4is_BugChk(mmm->Areas[aix].AreaId,ebuf) ;
		   v4mm_ZapBucket(aix,bktnum) ;		/* Zap in-core bucket so we don't get gazillion errors */
		   v4_error(V4E_INVENDBX,0,"V4MM","BktWrite","INVENDBX","Area (%d %s) Bucket (%d), Ending bx (%d) <> Free (%d)",
				mmm->Areas[aix].AreaId,UCretASC(mmm->Areas[aix].UCFileName),dbh->sbh.BktNum,bx,dbh->FreeEntryIndex) ;
		 } ;
		break ;
	 } ;
#ifdef WINNT
	li.QuadPart = (__int64)((__int64)mmm->Areas[aix].hfile,bktnum * (__int64)mmm->Areas[aix].BktSize) ;
	if (SetFilePointer(mmm->Areas[aix].hfile,li.LowPart,&li.HighPart,FILE_BEGIN) == 0xFFFFFFFF && GetLastError() != NO_ERROR)
	 { printf("SetFilePointer error %s, bkt =%d\n",v_OSErrString(GetLastError()),bktnum) ;
	   v4_error(V4E_BKTSEEKFAIL,0,"V4MM","BktWrite","BKTSEEKFAIL","Area (%d %s) SetFilePointer failure for bucket (%d)",
			aix,UCretASC(mmm->Areas[aix].UCFileName),bktnum) ;
	 } ;
	if (!WriteFile(mmm->Areas[aix].hfile,bktptr,mmm->Areas[aix].BktSize,&bytes,NULL))
	 { printf("WriteFile error %s, bkt=%d\n",v_OSErrString(GetLastError()),bktnum) ;
	   pi = v_GetProcessInfo() ; pi->ErrCount = pi->MaxAllowedErrs + 1 ;
	   v4_error(V4E_BKTWRITEFAIL,0,"V4MM","BktWrite","BKTWRITEFAIL","Area (%d %s) WriteFile failure (%s) for bucket (%d)",
			aix,UCretASC(mmm->Areas[aix].UCFileName),v_OSErrString(GetLastError()),bktnum) ;
	 } ;
#else
	if (fseek(mmm->Areas[aix].FilePtr,(FSEEKINT)((FSEEKINT)bktnum*(FSEEKINT)mmm->Areas[aix].BktSize),0) != 0)
	 { printf("fseek error %d, bkt =%d\n",errno,bktnum) ;
	   v4_error(V4E_BKTSEEKFAIL,0,"V4MM","BktWrite","BKTSEEKFAIL","Area (%d %s) fseek failure (%s) on bucket (%d)",
			aix,UCretASC(mmm->Areas[aix].UCFileName),v_OSErrString(errno),bktnum) ;
	 } ;
	if (fwrite(bktptr,mmm->Areas[aix].BktSize,1,mmm->Areas[aix].FilePtr) <= 0)
	 { sprintf(ebuf,"fwrite() err (%d), bkt=%d, bktsize=%d, filepos=%d",
		errno,bktnum,mmm->Areas[aix].BktSize,bktnum*mmm->Areas[aix].BktSize) ;
	   pi = v_GetProcessInfo() ; pi->ErrCount = pi->MaxAllowedErrs + 1 ;
	   v4is_BugChk(mmm->Areas[aix].AreaId,ebuf) ;
	   v4_error(V4E_BKTWRITEFAIL,0,"V4MM","BktWrite","BKTWRITEFAIL","Area (%d %s) fwrite error (%s) on bucket (%d)",
			aix,UCretASC(mmm->Areas[aix].UCFileName),v_OSErrString(errno),bktnum) ;
	 } ;
	if (fflush(mmm->Areas[aix].FilePtr) != 0)
	 { printf("fflush error %d, bkt=%d\n",errno,bktnum) ;
	   v4_error(V4E_BKTFLUSHFAIL,0,"V4MM","BktWrite","BKTFLUSHFAIL","Area (%d %s) fflush error (%s) on bucket (%d)",
			aix,UCretASC(mmm->Areas[aix].UCFileName),v_OSErrString(errno),bktnum) ;
	 } ;
#endif
	if (sbh->BktNum != bktnum)
	 { sprintf(ebuf,"Attempt from %s to write bkt #%d/index=%d has resulted in #%d\n",srcmod,bktnum,index,sbh->BktNum) ;
	   v4is_BugChk(mmm->Areas[aix].AreaId,ebuf) ;
	   v4_error(V4E_BKTNOTMATCH,0,"V4MM","BktWrite","BKTNOTMATCH","Area (%d %s) Bucket (%s/%d) header (%d) does not match (%d)",
			aix,UCretASC(mmm->Areas[aix].UCFileName),srcmod,index,sbh->BktNum,bktnum) ;
	 } ;
 	if (mmm->Areas[aix].GBPtr != NULL)
 	 { IFTRACEAREA
	    { sprintf(ebuf,"Leaving BktWrite(bkt=%d bx=%d) back to %s",bktnum,index,srcmod) ;
	    } ;
	 } ;
	mmm->Areas[aix].BktWrites++ ;
}

/*	L O C K   M O D U L E S							*/


/*	v4mm_LockSomething - Grabs a single lock				*/
/*	Call: lockid = v4mm_LockSomething( areaid , lockid , lockmode , lockidtype , maxtries , lockxptr , source )
	  where lockid is unique lock id for release,
		areaid is area id,
		lockid is lock (bucket, dataid, whatever,
		lockidtype is V4MM_LockIdType_xxx,
		lockmode is locking mode (read/write),
		maxtries is max number of times to try (if others have the lock),
		lockxptr (if non-null) is pointer to lockxptr to be updated with lock,
		source is integer code identifying source of lock	*/

int v4mm_LockSomething(areaid,lockid,lockmode,lockidtype,maxtries,lockxptr,source)
  int areaid,lockid,lockmode,lockidtype,maxtries,source ;
  int *lockxptr ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__GlobalBuf *glb ;
  struct V4MM__LockTable *lt ;
  HANGLOOSEDCL
  int try1,try2,try3,try4,try5,try6,try7,de1,de2,de3,wait0mode ;
  int i,j,ax,lockx,tries,tx,id,SaveLockId ; char ebuf[150] ;
  PID pid,wait0pid ;

	FINDAREAINPROCESS(ax,areaid)
	if (ax == UNUSED) v4_error(V4E_AREANOTFOUNDLS,0,"V4MM","LockSomething","AREANOTFOUNDLS","LockSomething: Could not locate area (%d)",areaid) ;
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++)
//	 { if (mmm->Areas[ax].AreaId == areaid) break ; } ;
//	if (ax >= mmm->NumAreas)
//	 v4_error(V4E_AREANOTFOUNDLS,0,"V4MM","LockSomething","AREANOTFOUNDLS","LockSomething: Could not locate area (%d)",areaid) ;
	if (mmm->Areas[ax].SegId == 0) return(0) ;			/* Locking not being done on this area */
	if ( GETGLB(ax) == NULL)
	 v4_error(V4E_AREANOTGLOBAL,0,"V4MM","LockSomething","AREANOTGLOBAL","Area (%d %s) not global",ax,UCretASC(mmm->Areas[ax].UCFileName)) ;
	lt = (struct V4MM__LockTable *)&glb->Buffer[glb->LockTableOffset] ;	/* Get pointer to lock table */
	if (lockxptr != NULL)						/* Maybe check for existing lock of same type? */
	 { LOCKLT(6)							/* First see if lock to release is lock we want */
	   for(lockx=0;lockx<lt->Count;lockx++)
	    { if (lt->Entry[lockx].UniqueLockId != *lockxptr) continue ;
	      if (lt->Entry[lockx].LockId != lockid) break ;
	      if (lt->Entry[lockx].LockIdType != lockidtype) break ;
	      if (lt->Entry[lockx].LockMode != lockmode) break ;
	      RELLT(9) return(*lockxptr) ;					/* Already have lock we want - return id */
	    } ;
	   RELLT(10)
	   if (*lockxptr >= 0) v4mm_LockRelease(areaid,*lockxptr,mmm->MyPid,areaid,"LockSomething") ; *lockxptr = UNUSED ;
	 } ;
	if (lt->Count >= lt->MaxAllowed)
	 v4_error(V4E_LOCKTBLFULL,0,"V4MM","LockSomething","LOCKTBLFULL","Area (%d %s) Lock table for is full (%d)",
			areaid,UCretASC(mmm->Areas[ax].UCFileName),lt->MaxAllowed) ;
	tries = 0 ; lockx = 0 ; wait0pid = UNUSEDPID ;
	try1 = 0 ; try2 = 0 ; try3 = 0 ; try4 = 0 ; try5 = 0 ; try6 = 0 ; try7 = 0 ; SaveLockId = UNUSED ;
	if (maxtries > 0) maxtries += V4IS_LOCKTRY_BASE ;		/* Adjust max number of tries */
try_again:
	if (((tries+1)%V4IS_LOCKTRY_PANIC) == 0 && maxtries > 0)	/* Are we getting close to crapping out ? */
	 { LOCKLT(7)
/*	   Before going into Panic Lockout - check for deadly embrace */
	   for(de1=0;de1<lt->Entry[lockx].PidCount;de1++)		/* Go thru each pid owning lock I want */
	    { for (de2=0;de2<lt->Count;de2++)
	       { for(de3=0;de3<lt->Entry[de2].PidCount;de3++)
	          { if (lt->Entry[de2].Pids[de3] == mmm->MyPid) break ; } ;
		 if (de3 >= lt->Entry[de2].PidCount) continue ;		/* I am not an owner of this lock */
	         for(de3=0;de3<lt->Entry[de2].WaitCount;de3++)		/* Is owner of lock we want (index=lockx) waiting for lock I have? */
	          { if (lt->Entry[de2].WaitPids[de3] == lt->Entry[lockx].Pids[de1]) break ; } ;
		 if (de3 >= lt->Entry[de2].WaitCount) continue ;
		 sprintf(ebuf,"Deadly Embrace: MyPid=%d, Lockx=%d, Mode=%d,Type=%d,Area=(%d %s) - (%d Pid=%d) waiting for LockX=%d",
				mmm->MyPid,lockx,lockmode,lockidtype,areaid,UCretASC(mmm->Areas[ax].UCFileName),de1,lt->Entry[lockx].Pids[de1],de2) ;
		 v4is_BugChk(areaid,ebuf) ;
	       } ;
	    } ;
	   if (glb->PanicLockout <= 0)					/* If no lockout then start one */
	    { glb->PanicLockout = lt->PidCnt*5 ; glb->PanicPid = mmm->MyPid ; RELLT(11)
	      if (tries > V4IS_LOCKTRY_PANIC)				/* No bugchk on first time */
	       { sprintf(ebuf,"Panic lockout mode=%d, type=%d, id=%d, tries=%d, source=%d",lockmode,lockidtype,lockid,tries,source) ;
	         v4is_BugChk(areaid,ebuf) ;
	       } ;
	      HANGLOOSE(V4IS_HANG_TIME) ; tries++ ;
	    } else { RELLT(12) } ;
	 } ;
	if (tries++ > maxtries && maxtries > 0)
	 { sprintf(ebuf,"Lock in Use: Src=%d, Pid=%d,Id=%d,Mode=%d,Type=%d,Area=(%d %s),Index=%d, MaxTries=%d %d %d %d %d %d %d %d",
		source,lt->Entry[lockx].Pids[0],lockid,lockmode,lockidtype,areaid,UCretASC(mmm->Areas[ax].UCFileName),lockx,maxtries,
		try1,try2,try3,try4,try5,try6,try7) ;
	   if (glb->PanicPid == mmm->MyPid)				/* Nice try but lockout didn't work */
	    { glb->PanicLockout = 0 ; glb->PanicPid = 0 ; } ;
	   v4is_BugChk(areaid,ebuf) ;
	   v4_error(V4E_ITEMLOCKED,0,"V4MM","LockSomething","ITEMLOCKED",ebuf) ;
	 } ;
	LOCKLT(8)
	for(lockx=0;lockx<lt->Count;lockx++)
	 {
	   if (lt->Entry[lockx].PidCount == 0)
	    { if (lt->Entry[lockx].WaitCount == 0)			/* No locks active & no pids waiting */
	       { lt->Entry[lockx] = lt->Entry[--lt->Count] ; lockx-- ; continue ; } ;
	    } ;
	   if (lt->Entry[lockx].LockId != lockid) continue ;
	   if (lt->Entry[lockx].LockIdType != lockidtype) continue ;
	   if (lt->Entry[lockx].PidCount == 0)				/* Check again for no active pids */
	    {
/*	      Pid(s) waiting- are we first in line or trying to lock tree ? (grab if tree to prevent deadlock) */
	      if (lt->Entry[lockx].WaitPids[0] == mmm->MyPid || lockidtype == V4MM_LockIdType_Tree
		   || tries > (V4IS_LOCKTRY_PANIC/2) )			/* If waiting "long" time then grab it */
	       { for(i=0,j=0;i<lt->Entry[lockx].WaitCount;i++)		/*  Yes - go thru and delete any references to "us" */
	          { if (lt->Entry[lockx].WaitPids[i] == mmm->MyPid) continue ;
		    lt->Entry[lockx].WaitLockModes[j] = lt->Entry[lockx].WaitLockModes[i] ;
	            lt->Entry[lockx].WaitPids[j++] = lt->Entry[lockx].WaitPids[i] ;
	          } ; lt->Entry[lockx].WaitCount = j ;
	         if (lt->Entry[lockx].WaitCount > 0) goto make_lock ;	/* Go make lock if still got a wait queue */
		 lt->Entry[lockx] = lt->Entry[--lt->Count] ; lockx-- ; continue ; /* otherwise, trash lock & continue */
	       } else
	       { if (PROCESS_GONE(lt->Entry[lockx].WaitPids[0]))
		  { lt->Entry[lockx].WaitPids[lt->Entry[lockx].WaitCount] = 0 ;
		    for(i=0;i<lt->Entry[lockx].WaitCount;i++)
		     { lt->Entry[lockx].WaitPids[i] = lt->Entry[lockx].WaitPids[i+1] ;
		       lt->Entry[lockx].WaitLockModes[i] = lt->Entry[lockx].WaitLockModes[i+1] ;
		     } ;
		    lt->Entry[lockx].WaitCount-- ; lockx-- ; continue ;
		  } ;
	       } ;
////VEH030117 This chunk of code is in for unknown reasons - causes horrendous slowdown when mult processes reading
////	      for(i=0;i<lt->Entry[lockx].WaitCount;i++)		/* See if we are already waiting */
////	       { if (lt->Entry[lockx].WaitPids[i] == mmm->MyPid) break ; } ;
////	      if (i >= lt->Entry[lockx].WaitCount)			/* Add to end of list if possible */
////	       { if (lt->Entry[lockx].WaitCount < V4MM_LockTable_PidsPerLock-1)
////		  { i = lt->Entry[lockx].WaitCount++ ;
////		    lt->Entry[lockx].WaitPids[i] = mmm->MyPid ; lt->Entry[lockx].WaitLockModes[i] = lockmode ;
////		  } ;
////	       } ;

//VEH030423 Adding this chunk back in
	      if (try1 > 10)						/* Waiting too long - see if first in line is hung? */
	       { if (wait0pid == lt->Entry[lockx].WaitPids[0])		/* Is it still the same ? */
	          { 


//VEH050401 Added check for dead pid in WaitPids[0]
		    if (PROCESS_GONE(lt->Entry[lockx].WaitPids[0]))
		     { lt->Entry[lockx].WaitPids[lt->Entry[lockx].WaitCount] = 0 ;
		       for(i=0;i<lt->Entry[lockx].WaitCount;i++)
		        { lt->Entry[lockx].WaitPids[i] = lt->Entry[lockx].WaitPids[i+1] ;
		          lt->Entry[lockx].WaitLockModes[i] = lt->Entry[lockx].WaitLockModes[i+1] ;
		        } ;
		       lt->Entry[lockx].WaitCount-- ; lockx-- ; wait0pid = UNUSEDPID ; try1 = 0 ; continue ;
		     } ;
//


		    wait0mode = lt->Entry[lockx].WaitLockModes[0] ;
		    for(i=0;i<lt->Entry[lockx].WaitCount;i++)		/* Push everybody down & put entry 0 at end */
		     { lt->Entry[lockx].WaitPids[i] = lt->Entry[lockx].WaitPids[i+1] ;
		       lt->Entry[lockx].WaitLockModes[i] = lt->Entry[lockx].WaitLockModes[i+1] ;
		     } ; lt->Entry[lockx].WaitPids[i-1] = wait0pid ; lt->Entry[lockx].WaitLockModes[i-1] = wait0mode ;
		  } ;
	         wait0pid = UNUSEDPID ; try1 = 0 ;
	       } ;
	      if (wait0pid == UNUSEDPID)
	       wait0pid = lt->Entry[lockx].WaitPids[0] ;
//END030423
	      RELLT(13) HANGLOOSE(V4IS_HANG_TIME) ;			/*  Someone else getting lock- try again */
	      try1++ ; goto try_again ;
	    } ;
	   switch (lt->Entry[lockx].LockMode)
	    { default:	RELLT(14)
			v4_error(V4E_INVLOCKMODE,0,"V4MM","LockSomething","INVLOCKMODE","Invalid Lock Mode (%d)",lt->Entry[lockx].LockMode) ;
	      case V4MM_LockMode_AnythingGoes:	goto make_lock ;
	      case V4MM_LockMode_Read:
		if (lockmode == V4MM_LockMode_Read) goto make_lock ;	/* Allow multiple read locks */
		/* If not a read then fall thru to below */
	      case V4MM_LockMode_Write:
		for(i=0;i<lt->Entry[lockx].PidCount;i++)
		 { if (lt->Entry[lockx].Pids[i] == mmm->MyPid && lt->Entry[lockx].AreaIds[i] == areaid) break ; } ;
		if (i < lt->Entry[lockx].PidCount)
		 { if (lockidtype == V4MM_LockIdType_Alloc)
		    { RELLT(15) return(UNUSED) ; } ;	/* Allow "nesting" of this lock type */
		   if (lt->Entry[lockx].UniqueLockId == mmm->Areas[ax].LockIDX)
		    { mmm->Areas[ax].LockIDX = UNUSED ; }			/* If we sorta know about this, don't squawk! */
		    else printf("Trashing lock (pos=%d,id=%d,type=%d, pidcnt=%d) - belongs to self!\n",
			   lockx,lt->Entry[lockx].LockId,lt->Entry[lockx].LockIdType,lt->Entry[lockx].PidCount) ;
		   v4mm_LockRelease(areaid,lt->Entry[lockx].UniqueLockId,mmm->MyPid,areaid,"LockSomething1") ; tries-- ;
		   RELLT(16)				/* Release lock table */
		   try2++ ; goto try_again ;
		 } ;
		for(i=0;i<lt->Entry[lockx].PidCount && tries>25;i++)
		 { if (!PROCESS_GONE(lt->Entry[lockx].Pids[i])) continue ;
		   v4mm_LockRelease(areaid,lt->Entry[lockx].UniqueLockId,
					lt->Entry[lockx].Pids[i],lt->Entry[lockx].AreaIds[i],"LockSomething2") ;
		   tries-- ; RELLT(17) ; try3++ ; goto try_again ;
		 } ;
		if (maxtries <= 0)
		 { pid = lt->Entry[lockx].Pids[0] ;
		   if (pid == (PID)0) { printf("??? Lock #%d has pid[0] = %d\n",lockx,pid) ; pid = (PID)99999 ; } ;
		   RELLT(18)			/* Release lock table */
		   return(-(int)(pid)) ;		/* Don't wait around */
		 } ;
		for(i=0;i<lt->Entry[lockx].WaitCount;i++)		/* See if we are already waiting */
		 { if (lt->Entry[lockx].WaitPids[i] == mmm->MyPid) break ; } ;
		if (i >= lt->Entry[lockx].WaitCount)			/* Add to end of list if possible */
		 { if (lt->Entry[lockx].WaitCount < V4MM_LockTable_PidsPerLock-1)
		    { i = lt->Entry[lockx].WaitCount++ ;
		      lt->Entry[lockx].WaitPids[i] = mmm->MyPid ; lt->Entry[lockx].WaitLockModes[i] = lockmode ;
		    } ;
		 } ;
//VEH030423 Adding this chunk back in
		if (SaveLockId == UNUSED) SaveLockId = lt->Entry[lockx].UniqueLockId ;
		if (try4 > (maxtries > 1000 ? maxtries - 500 : V4IS_LOCKTRY_PANIC+10) && SaveLockId == lt->Entry[lockx].UniqueLockId)
		 { RELLT(27)
		   v4mm_LockRelease(areaid,lt->Entry[lockx].UniqueLockId,
					lt->Entry[lockx].Pids[i],lt->Entry[lockx].AreaIds[i],"LockSomething3") ;
		   sprintf(ebuf,"Grabbing (stuck) Lock in Use: Src=%d, Pid=%d,Id=%d,Mode=%d,Type=%d,Area=(%d %s),Index=%d, MaxTries=%d %d %d %d %d %d %d %d",
			source,lt->Entry[lockx].Pids[0],lockid,lockmode,lockidtype,areaid,UCretASC(mmm->Areas[ax].UCFileName),lockx,maxtries,
			try1,try2,try3,try4,try5,try6,try7) ;
		   v4is_BugChk(areaid,ebuf) ;
		   SaveLockId = UNUSED ; try4 = 0 ; goto try_again ;
		 } ;
//END030423
		RELLT(19)					/* Release lock table */
		HANGLOOSE(V4IS_HANG_TIME) ;						/* Someone else got lock- try again */
		try4++ ; goto try_again ;
	    } ;
	 } ;
make_lock:
	if (lockx >= lt->MaxAllowed)
	 v4_error(V4E_MAXLOCKS,0,"V4MM","LockSomething","MAXLOCKS","Area (%d %s) - Exceeded lock maximum (%d)",
		ax,UCretASC(mmm->Areas[ax].UCFileName),lt->MaxAllowed) ;
	if (lockx >= lt->Count)
	 { lt->Entry[lockx].Pids[0] = 0 ; lt->Entry[lockx].AreaIds[0] = 0 ;
	   lt->Entry[lockx].WaitCount = 0 ; lt->Entry[lockx].PidCount = 0 ;
	 } ;
	switch (lockidtype)					/* Maybe keep track of total type of lock */
	 { case V4MM_LockIdType_Index:
		if (lt->LocksTree > 0 && lockmode == V4MM_LockMode_Write) /* If locking index, check to see if entire tree locked */
		 { if (mmm->Areas[ax].LockTree < 0)			/* It is- by us ? */
		    {
		      lt->LocksTree = 0 ;				/* Before refusing lock, make sure this is up-to-date */
		      for(i=0;i<lt->Count;i++)
		       { if (lt->Entry[i].LockIdType == V4MM_LockIdType_Tree ? lt->Entry[i].LockMode == V4MM_LockMode_Write : FALSE)
			  lt->LocksTree++ ;
		       } ;
		      if (lt->LocksTree == 0)
		       { printf("?? Reset lt->LocksTree to 0\n") ; goto make_lock ;
		       } ;
		      if (maxtries <= 0) { RELLT(20) return(-9998) ; } ;
		      RELLT(21)				/* Release lock table */
		      HANGLOOSE(V4IS_HANG_TIME) ; try5++ ; goto try_again ;
		    } ;	/* No - then can't grab this bucket */
		 } ;
		if (lockmode == V4MM_LockMode_Write) lt->LocksIDX ++ ;
		break ;
	   case V4MM_LockIdType_Tree:
		if (lt->LocksIDX > 0 && lockmode == V4MM_LockMode_Write)
		 { if (maxtries <= 0) { RELLT(22) return(-9999) ; } ;
		   for(lt->LocksIDX=0,i=0;i<lt->Count;i++)	/* Resynch the LocksIDX count- just in case! */
		    { if (lt->Entry[i].LockIdType == V4MM_LockIdType_Index && lt->Entry[i].LockMode == V4MM_LockMode_Write)
		       lt->LocksIDX++ ;
		    } ;
		   RELLT(23) HANGLOOSE(V4IS_HANG_TIME) ; try6++ ; goto try_again ;
		 } ;
		if (lockmode == V4MM_LockMode_Write) lt->LocksTree ++ ;
		break ;
	 } ;
	if (lt->Entry[lockx].PidCount >= V4MM_LockTable_PidsPerLock)
	 { /* sprintf(ebuf,"Lock index (%d) - exceeded max pids",lockx) ; v4is_BugChk(areaid,ebuf) ; */
	   if (maxtries <= 0) { RELLT(24) return(-9999) ; } ;
	   RELLT(25) HANGLOOSE(V4IS_HANG_LONGTIME) ; try7++ ; goto try_again ;
	 } ;
	if (glb->PanicPid == mmm->MyPid)				/* Panic lockout worked! - OK to turn off */
	 { glb->PanicLockout = 0 ; glb->PanicPid = 0 ; } ;
	if (lockx >= lt->Count) lt->Count++ ;				/* Keep track of total number of locks */
	lt->Entry[lockx].LockId = lockid ;
	lt->Entry[lockx].Source = source ;
	lt->Entry[lockx].LockIdType = lockidtype ;
	if (lockidtype == V4MM_LockIdType_Tree && lockmode == V4MM_LockMode_Write)
	 glb->TotalTreeLocks++ ;					/* Track total number of tree locks for other users */
	lt->Entry[lockx].LockMode = lockmode ;
	lt->Entry[lockx].AreaIds[lt->Entry[lockx].PidCount] = areaid ;
	lt->Entry[lockx].Pids[lt->Entry[lockx].PidCount++] = mmm->MyPid ; /* Got lock- grab it */
	if (lt->Entry[lockx].PidCount == 1)				/* If "first" pid to grab lock then */
	 lt->Entry[lockx].UniqueLockId = (++lt->LastUniqueLockId) ;	/*   assign unique id */
	for(i=0,j=0;i<lt->Entry[lockx].WaitCount;i++)			/*  Make sure we are not in wait queue for this lock */
	 { if (lt->Entry[lockx].WaitPids[i] == mmm->MyPid) continue ;
	   lt->Entry[lockx].WaitLockModes[j] = lt->Entry[lockx].WaitLockModes[i] ;
	   lt->Entry[lockx].WaitPids[j++] = lt->Entry[lockx].WaitPids[i] ;
	 } ; lt->Entry[lockx].WaitCount = j ;
	if (lockxptr != NULL) *lockxptr = lt->Entry[lockx].UniqueLockId ;			/* Set it */
	id = lt->Entry[lockx].UniqueLockId ;				/* Save so we don't lose when RELLT below */
	RELLT(26)
	return(id) ;
}

/*	v4mm_LockRelease - Release a lock					*/
/*	Call: v4mm_LockRelease( areaid , uniquelockid , pid , lockareaid , srcmod )
	  where areaid is area id,
		uniquelockid is unique lock id
		pid is pid to release lock for,
		lockareaid is areaid in lock (same as areaid unless pid not our pid!),
		srcmod is calling/source module (for debugging)			*/

void v4mm_LockRelease(areaid,uniquelockid,pid,lockareaid,srcmod)
  int areaid,uniquelockid ;
  PID pid ;
  char *srcmod ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__GlobalBuf *glb ;
  struct V4MM__LockTable *lt ;
  int ax,i,j,lockx ; int tx ; char ebuf[200] ;

	if (uniquelockid == UNUSED) return ;				/* Nothing to release */
	FINDAREAINPROCESS(ax,areaid)
	if (ax == UNUSED) v4_error(V4E_AREANOTFOUNDLR,0,"V4MM","LockRelease","AREANOTFOUNDLR","LockRelease: Could not locate area (%d)",areaid) ;
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++)
//	 { if (mmm->Areas[ax].AreaId == areaid) break ; } ;
//	if (ax >= mmm->NumAreas)
//	 v4_error(V4E_AREANOTFOUNDLR,0,"V4MM","LockRelease","AREANOTFOUNDLR","LockRelease: Could not locate area (%d)",areaid) ;
	if (mmm->Areas[ax].SegId == 0) return ;				/* Locking not being done on this area */
	if ( GETGLB(ax) == NULL)
	 v4_error(V4E_AREANOTGLOBAL,0,"V4MM","LockRelease","AREANOTGLOBAL","Area (%d %s) not global",ax,UCretASC(mmm->Areas[ax].UCFileName)) ;
	lt = (struct V4MM__LockTable *)&glb->Buffer[glb->LockTableOffset] ;	/* Get pointer to lock table */
	LOCKLT(9)
	if (uniquelockid == V4MM_Lock_ReleasePid)			/* Free all locks assigned to current pid ? */
	 { for(i=0;i<lt->Count;i++)			/*  Yes - scan table for pid & release */
	    { for(j=0;j<lt->Entry[i].PidCount;j++)
	       { if (lt->Entry[i].Pids[j] != pid) continue ; if (lt->Entry[i].AreaIds[j] != areaid) continue ;
		 v4mm_LockRelease(areaid,lt->Entry[i].UniqueLockId,pid,lt->Entry[i].AreaIds[j],"LockRelease_PID") ;
		 i = -1 ; break ;			/* Start scan all over again, locks may have moved! */
	       } ;
	    } ;
	   RELLT(27) return ;
	 } ;
	for(lockx=0;lockx<lt->Count;lockx++) { if (lt->Entry[lockx].UniqueLockId == uniquelockid) break ; } ;
	if (lockx >= lt->Count)
	 { RELLTE(28)
	   sprintf(ebuf,"Unique lock id (%d) passed from (%s) to LockRelease not found",uniquelockid,srcmod) ;
	   v4is_BugChk(areaid,ebuf) ;
/*	   v4_error(V4E_INVLOCKX,0,"V4MM","LockRelease","INVLOCKX","Area (%d %s) Unique Lock ID (%d) passed from (%s) to LockRelease not found",
			areaid,UCretASC(mmm->Areas[ax].UCFileName),uniquelockid,srcmod) ; */
	   return ;
	 } ;
	if (lt->Entry[lockx].LockMode == V4MM_LockMode_Write)
	 { switch (lt->Entry[lockx].LockIdType)				/* Maybe keep track of total type of lock */
	    { case V4MM_LockIdType_Index:
		if (--lt->LocksIDX < 0) { printf("LocksIDX going negative!\n") ; lt->LocksIDX = 0 ; } ;
		break ;
	      case V4MM_LockIdType_Tree:
		if (--lt->LocksTree < 0) { printf("LockxTree going negative!\n") ; lt->LocksTree = 0 ; } ;
		break ;
	    } ;
	 } ;
	lt->Entry[lockx].Pids[lt->Entry[lockx].PidCount] = 0 ; lt->Entry[lockx].AreaIds[lt->Entry[lockx].PidCount] = 0 ;
	for(i=0,j=0;i<lt->Entry[lockx].PidCount;i++)
	 { if (lt->Entry[lockx].Pids[i] == pid && lt->Entry[lockx].AreaIds[i] == lockareaid) continue ;
	   lt->Entry[lockx].Pids[j] = lt->Entry[lockx].Pids[i] ; lt->Entry[lockx].AreaIds[j++] = lt->Entry[lockx].AreaIds[i] ;
	 } ;
	if (j == lt->Entry[lockx].PidCount)
	 { RELLTE(29)
	   sprintf(ebuf,"Area (%d) Unique Lock ID (%d/index %d/i=%d) - no Pid (%d) to release from (%s)",
		   areaid,uniquelockid,lockx,i,pid,srcmod) ;
	   v4is_BugChk(areaid,ebuf) ;
/*	   v4_error(V4E_LOCKNOTRELPID,0,"V4MM","LockRelease","LOCKNOTRELPID",
			"Area (%d %s) Unique Lock ID (%d/index %d/i=%d) - no Pid (%d) to release from (%s)",
			areaid,UCretASC(mmm->Areas[ax].UCFileName),uniquelockid,lockx,i,pid,srcmod) ; */
	 } else { lt->Entry[lockx].PidCount = j ; } ;
	if (lt->Entry[lockx].PidCount == 0)
	 { if (lt->Entry[lockx].WaitCount == 0)			/* No locks active & no pids waiting */
	    { lt->Entry[lockx] = lt->Entry[--lt->Count] ;
	      RELLT(30) return ;
	    } ;
	   lt->Entry[lockx].LockMode = V4MM_LockMode_AnythingGoes ;	/* Lock mode OK with read or write */
	   RELLT(31)
#ifdef DOALARM
	   kill(lt->Entry[lockx].WaitPids[0],SIGALRM) ;		/* Issue wakeup to first in line */
#endif
	   return ;
	 } ;
	RELLT(32)							/* Free up lock table */
}

/*	S H A R A B L E   S E G M E N T   M O D U L E S			*/

#ifdef segshare

/*	v4mm_OpenCloseLock - Ensures that only one process can open/close an area at a time	*/
/*	Call: v4mm_OpenCloseLock( lockind )
	  where lockind is TRUE to get exclusive lock, FALSE to release				*/

#define OPNCLS_MAXWAIT_MS 1000		/* Max time (ms) to wait for this lock before grabbing it anyway */
void v4mm_OpenCloseLock(lockind)
{ 
#ifdef UNIX
  static struct sem_t *sem = SEM_FAILED ;
  struct timespec semTS ;
  int res ; UCCHAR errbuf[512] ;

	if (sem == SEM_FAILED) sem = sem_open("/var",O_CREAT,(S_IRWXU | S_IRWXG | S_IRWXO),1) ;
	if (sem == SEM_FAILED)
	 { v_Msg(NULL,errbuf,"SystemCallFail","sem_open",UClit("Log Test"),errno) ; vout_UCText(VOUT_Err,0,errbuf) ;
	   return ;
	 } ;
/*	Release the semaphore if lockind is FALSE */
	if (!lockind) { sem_post(sem) ; return ; } ;
/*	Set timeout for 250ms from now */
	clock_gettime(CLOCK_REALTIME, &semTS) ;
	semTS.tv_sec += (OPNCLS_MAXWAIT_MS / 1000) ;
	semTS.tv_nsec += (OPNCLS_MAXWAIT_MS % 1000) * 1000 ; if (semTS.tv_nsec > 999999999) { semTS.tv_sec += 1 ; semTS.tv_nsec -= 1000000000 ; } ;
	res = sem_timedwait(sem,&semTS) ;
	if (res != 0)
	 { if (errno == ETIMEDOUT ) { vout_UCText(VOUT_Err,0,UClit("*v4mm_OpenCloseLock() - timed out waiting for semaphore, clearing & continuing\n")) ; vout_UCText(VOUT_Err,0,errbuf) ; }
	    else { v_Msg(NULL,errbuf,"SystemCallFail","sem_timedwait",UClit("Log Test"),errno) ; vout_UCText(VOUT_Err,0,errbuf) ; } ;
	   sem_post(sem) ;		/* Try to clear semaphore */
	   sem_trywait(sem) ;		/*  & try to set it */
	 } ;
#endif
#ifdef WINNT
  static HANDLE hMutex ;
  UCCHAR errbuf[512] ;

	if (!lockind)
	 { if (hMutex != NULL) { ReleaseMutex(hMutex) ; CloseHandle(hMutex) ; hMutex = NULL ; } ;
	   return ;
	 } ;
	hMutex = CreateMutex(NULL,FALSE,"Global\\V4OpenClose") ;
	if (hMutex == NULL)
	 { v_Msg(NULL,errbuf,"SystemCallFail","CreateMutex",UClit("V4LockSegment"),GetLastError()) ; vout_UCText(VOUT_Err,0,errbuf) ; ; } ;
	switch (WaitForSingleObject(hMutex,OPNCLS_MAXWAIT_MS))
	 { default:		break ;
	   case WAIT_FAILED:	v_Msg(NULL,errbuf,"SystemCallFail","WaitForSingleObject(opncls)",UClit(""),GetLastError()) ; vout_UCText(VOUT_Err,0,errbuf) ; break ;
	   case WAIT_TIMEOUT:	vout_UCText(VOUT_Err,0,UClit("*v4mm_OpenCloseLock() - timed out waiting for mutex, clearing & continuing\n")) ; vout_UCText(VOUT_Err,0,errbuf) ; break ;
	 } ;
#endif
}


void v4mm_OpenCloseLockOld(lockind)
  LOGICAL lockind ;
{
#ifdef UNIX
  int OCLKey ;			/* Segment Key */
  int OCSegId ;
#endif
  static PID MyPid ;
  struct lcl__OCLockStruct {
    DCLSPINLOCK Lock ;
    PID LockPid ;
   } ;
  static struct lcl__OCLockStruct *locl = NULL ;
  PID lpid ;
  int llock ;
#ifdef WINNT
  HANDLE hMapObject ; int newmap ;
  struct V4MM__LockTable *lt = NULL ;		/* Dummy up to make PROCESS_GONE work */
#endif
  int tx ;

#ifdef VMSOS
	return(TRUE) ;				/* Not a problem on VMS */
#endif
#ifdef UNIX

	if (locl == NULL)					/* Have to initialize ? */
	 { char resbuf[128] ;
//	   if (v_UCGetEnvValue(UClit("v3_data"),resbuf,UCsizeof(resbuf)))	/* New form (v3_data) or old form (v3_sapsdat) */
//	    { OCLKey = (key_t)ftok(v3_logical_decoder("v3_data:opncls.seg",TRUE),0x99) ; }
//	    else { OCLKey = (key_t)ftok(v3_logical_decoder("v3_sapsdat:opncls.seg",TRUE),0x99) ; } ;
	   if (v_UCGetEnvValue(UClit("v3_data"),resbuf,UCsizeof(resbuf)))	/* New form (v3_data) or old form (v3_sapsdat) */
	    { OCLKey = (key_t)ftok(UCretASC(v_UCLogicalDecoder(UClit("v3_data:opncls.seg"),VLOGDECODE_Exists,0,resbuf)),0x99) ; }
	    else { OCLKey = (key_t)ftok(UCretASC(v_UCLogicalDecoder(UClit("v3_sapsdat:opncls.seg"),VLOGDECODE_Exists,0,resbuf)),0x99) ; } ;
	   if ( (OCSegId = shmget(OCLKey,sizeof *locl,0440)) >= 0)
	    { if ((int)(locl = (struct lcl__OCLockStruct *)shmat(OCSegId,(char*)0,0)) == -1)
	       v4_error(V4E_NOSEG,0,"V4MM","OpenCloseLock","NOSEG","Error (%s) attaching to open-close segment",v_OSErrString(errno)) ;
	    } else
	    { if ((OCSegId = shmget(OCLKey,sizeof *locl,IPC_CREAT+0660)) < 0)
	       v4_error(V4E_SHMGETFAIL,0,"V4MM","OpenCloseLock","SHMGETFAIL","Error (%s) creating open-close segment",v_OSErrString(errno)) ;
	      if ((int)(locl = (struct lcl__OCLockStruct *)shmat(OCSegId,(char*)0,0)) == -1)
	       v4_error(V4E_NOSEG1,0,"V4MM","OpenCloseLock","NOSEG1","Error1 (%s) attaching to open-close segment",v_OSErrString(errno)) ;
	    } ;
	   MyPid = CURRENTPID ;
	   if (locl->LockPid == 0) { RLSSPINLOCK(locl->Lock) ; } ;
	 } ;
#endif
#ifdef WINNT
	if (locl == NULL)
	 { hMapObject = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof *locl,"opncls.seg") ;
	   if (hMapObject == NULL)
	    v4_error(V4E_OCCREFILMAP,0,"V4MM","OPNCLSLOCK","OCCREFILMAP","Error (%s) in CreateFileMapping of opncls.seg",v_OSErrString(GetLastError())) ;
	   newmap = (GetLastError() != ERROR_ALREADY_EXISTS) ;
	   locl = (struct lcl__OCLockStruct *)MapViewOfFile(hMapObject,FILE_MAP_WRITE,0,0,0) ;
	   if (locl == NULL)
	    v4_error(V4E_NOMAPVIEW,0,"V4MM","OPNCLSLOCK","NOMAPVIEW","Error (%s) in MapViewOfFile for opncls.seg",v_OSErrString(GetLastError())) ;
	   if (newmap) locl->Lock = UNUSED ;
	   MyPid = WINNT_PID ;
	   if (locl->LockPid == 0) { RLSSPINLOCK(locl->Lock) ; } ;
	 } ;
#endif
	if (locl->LockPid == MyPid && lockind) { locl->LockPid = UNUSEDPID ; RLSSPINLOCK(locl->Lock) ; } ;
	if (lockind)						/* Are we locking or releasing ? */
	 { for(tx=0;;tx++)
	    { if (GETSPINLOCK(&locl->Lock)) break ;
	      HANGLOOSE(V4IS_HANG_TIME) ; if ((tx % 100) != 5) continue ;
	      if (PROCESS_GONE(locl->LockPid)) { locl->LockPid = UNUSEDPID ; RLSSPINLOCK(locl->Lock) ; continue ; } ;
	      if (tx > 25)
	       { lpid = locl->LockPid ; llock = locl->Lock ;
		 locl->LockPid = UNUSEDPID ; RLSSPINLOCK(locl->Lock) ;
		 printf("Grabbing open-close lock (pid=%d lock=%d)\n",lpid,llock) ;
		 continue ;
		 v4_error(V4E_OCLOCKHUNG,0,"V4MM","LOCKOCL","OCLOCKHUNG","Open-Close Lock table appears hung (%d) by process (%d)",
				locl->Lock,locl->LockPid) ;
	       } ;
	    } ; locl->LockPid = MyPid ;
         } else
	 { if (locl->LockPid != MyPid) { printf("?? Releasing open-close lock table locked by pid=%d\n",locl->LockPid) ; }
    	    else { locl->LockPid = UNUSEDPID ; RLSSPINLOCK(locl->Lock) ; } ;
	 } ;
}

/*	v4mm_ShareSegInit	*/

void v4mm_ShareSegInit(areaid,filename,bktcnt,bktsize,lckcnt,keyflag,clearflag)
  int areaid ;
  UCCHAR *filename ;
  int bktcnt,bktsize ;
  int lckcnt ;
  int keyflag ;
  int clearflag ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__GlobalBuf *glb ;
  struct V4MM__LockTable *lt ;
#ifdef WINNT
  HANDLE hMapObject ; int newmap,j ; char namebuf[250] ;
#endif
#ifdef UNIX
  struct shmid_ds buf ;
  int tries ;
#endif
#ifdef VMSOS
  globalvalue int SS$_NORMAL,SS$_CREATED,SEC$M_PAGFIL,SEC$M_GBL ;
  struct vms__dsc {
    short int length ;		/* Length of value */
    short int desc ;		/* Descriptive info */
    char *pointer ;		/* Pointer to value */
   } ; struct vms__dsc ndsc ;
  int inaddr[2],retaddr[2] ;
  int j,res ; char namebuf[250] ; short int reslen ;
#endif
  int i,ax,bx,lx,px,segbytes,validuser,tx ; char ebuf[250] ;

	FINDAREAINPROCESS(ax,areaid)
	if (ax == UNUSED) v4_error(V4E_AREANOTFOUNDSSI,0,"V4MM","ShareSegInit","AREANOTFOUNDSSI","ShareSegInit: Could not locate area (%d)",areaid) ;
//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) v4_error(V4E_MMMNOTINIT,0,"V4MM","MakeNewAId","MMMNOTINIT","MM not initialized") ;
//	for(ax=0;ax<mmm->NumAreas;ax++)
//	 { if (mmm->Areas[ax].AreaId == areaid) break ; } ;
//	if (ax >= mmm->NumAreas)
//	 v4_error(V4E_AREANOTFOUNDSSI,0,"V4MM","ShareSegInit","AREANOTFOUNDSSI","ShareSegInit: Could not locate area (%d)",areaid) ;

	segbytes = sizeof *glb - sizeof glb->Buffer
			+ bktcnt*bktsize							/* Add in for buckets */
			+ (sizeof *lt - sizeof lt->Entry + (sizeof lt->Entry[0] * lckcnt)) ;	/* Add in for locks */
#ifdef UNIX
	if ((keyflag & 0xff) == 0) keyflag = 1 ;				/* Not allowed to be 0 in OSF land! */
	mmm->Areas[ax].SegKey = (SEGKEY)ftok(UCretASC(filename),(keyflag & 0xff)) ;	/* Create unique segment key for this package */
#endif

	if (clearflag) v4mm_OpenCloseLock(FALSE) ;		/* VEH060225 - force a clear of this just in case */
	v4mm_OpenCloseLock(TRUE) ;
#ifdef VMSOS
	for(j=0,i=0;;j++,i++)
	 { switch (filename[i])
	    { default:	ebuf[j] = toupper(filename[i]) ; break ;
	      case 0:	ebuf[j] = 0 ; goto end_nameloop ;
	      case ':':	j = -1 ; break ;
	      case ']':	j = -1 ; break ;
	    } ;
	 } ;
end_nameloop:
	sprintf(namebuf,"%s-%d",ebuf,keyflag) ;		/* Create unique name for segment */
	ndsc.length = strlen(namebuf) ; ndsc.desc = 0 ; ndsc.pointer = &namebuf) ;
	segbytes = (segbytes/V3_VMS_PAGE_SIZE + 1)*V3_VMS_PAGE_SIZE ;
	res = sys$crmpsc(&inaddr,&retaddr,0,SEC$M_GBL+SEC$M_PAGFIL,&ndsc,0,0,0,segbytes,0,0,0) ;
	glb = (struct V4MM__GlobalBuf *)retaddr[0] ;
	if (res == SS$_CREATED) goto init_seg ;
	if (res != SS$_NORMAL)
	 { ndsc.length = sizeof ebuf ; ndsc.desc = 0 ; ndsc.pointer = ebuf ; sys$getmsg(res,&reslen,&ndsc,1,0) ;
	   ebuf[reslen] = 0 ;
	   v4mm_OpenCloseLock(FALSE) ;
	   v4_error(V4E_CRMPSCFAIL,0,"V4MM","ShareSegInit","CRMPSCFAIL","Error (%s) creating global segment (%s) of %d bytes",
			ebuf,namebuf,segbytes) ;
	 } else
	 {
#endif
#ifdef WINNT
	for(j=0,i=0;;j++,i++)
	 { switch (filename[i])
	    { default:	ebuf[j] = toupper(filename[i]) ; break ;
	      case 0:	ebuf[j] = 0 ; goto end_nameloop ;
	      case ':':	j = -1 ; break ;
	      case '\\':	j = -1 ; break ;
	    } ;
	 } ;
end_nameloop:
	sprintf(namebuf,"%s-%d",ebuf,keyflag) ;		/* Create unique name for segment */
	hMapObject = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,segbytes,namebuf) ;
	if (hMapObject == NULL)
	 { v4mm_OpenCloseLock(FALSE) ;
	   v4_error(V4E_NOCREFILMAP,0,"V4MM","ShareSegInit","NOCREFILMAP","Error (%s) in CreateFileMapping (%d) of %s",v_OSErrString(GetLastError()),segbytes,namebuf) ;
	 } ;
	mmm->Areas[ax].SegKey = (SEGKEY)(mmm->Areas[ax].SegId = hMapObject) ;
	newmap = (GetLastError() != ERROR_ALREADY_EXISTS) ;
	glb = (struct V4MM__GlobalBuf *)MapViewOfFile(hMapObject,FILE_MAP_WRITE,0,0,0) ;
	if (glb == NULL)
	 { v4mm_OpenCloseLock(FALSE) ;
	   v4_error(V4E_NOMAPVIEW,0,"V4MM","ShareSegInit","NOMAPVIEW","Error (%s) in MapViewOfFile for %s",v_OSErrString(GetLastError()),namebuf) ;
	 } ;
	if (!newmap)
	 {	/* Drop thru conditionals to "quick-check" */
#endif
#ifdef UNIX
	if ( (mmm->Areas[ax].SegId = shmget(mmm->Areas[ax].SegKey,segbytes,0440)) >= 0)
	 {
#ifdef HPUX
	   for(i=0;i<mmm->NumAreas;i++)
	    { if (i == ax) continue ;				/* No fair looking at ourselves! */
	      if (mmm->Areas[i].AreaId == UNUSED) continue ;
	      if (mmm->Areas[i].SegId == mmm->Areas[ax].SegId)	/* Already got this segment mapped? */
	       { glb = mmm->Areas[i].GBPtr ; tries = 0 ; goto quick_check ;
	       } ;
	    } ;
#endif
	   for(tries=0;tries<5;tries++)				/* Try to attatch to the segment */
	    { if ((int)(glb = (struct V4MM__GlobalBuf *)shmat(mmm->Areas[ax].SegId,(char*)0,0)) != -1) break ;
	      for(px=0;px<mmm->NumAreas;px++)			/* Attach failed - see if already attached in other area */
	       { if (px == ax) continue ;
		 if (mmm->Areas[px].SegKey == mmm->Areas[ax].SegKey) { glb = mmm->Areas[px].GBPtr ; goto quick_check ; } ;
	       } ;
	      HANGLOOSE(V4IS_HANG_TIME) ;			/* Hang and try to attach a little later */
	    } ;
	   if (tries >= 5)					/* Did we attach ? */
	    { shmctl(mmm->Areas[ax].SegId,IPC_STAT,&buf) ;
	      shmctl(mmm->Areas[ax].SegId,IPC_RMID,&buf) ;	/* Try to remove area and then ... */
	      sprintf(ebuf,"Calling create_seg- Error (%s) in shmat for Area (%s) after %d tries",v_OSErrString(errno),filename,tries) ;
	      mmm->Areas[ax].GBPtr = NULL ; v4is_BugChk(areaid,ebuf) ;
	      goto create_seg ;					/*   re-create it */
	    } ;
quick_check:
#endif
/*	   Make a quick pass thru buckets & locks to see if anything belonging to defunct processes */
	   validuser = FALSE ;					/* Assume no valid users */
	   if (glb->LockTableOffset < 0 || glb->LockTableOffset > sizeof glb->Buffer)
	    { printf("Global segment for file (%ws) appears to be trashed - resetting it...\n",filename) ; goto init_seg ; } ;
	   lt = (struct V4MM__LockTable *)&glb->Buffer[glb->LockTableOffset] ;		/* Link up */
	   if ((lt->ActivePid == UNUSEDPID) && (!SPINLOCKFREE(lt->IsActive)))
	    { HANGLOOSE(V4IS_HANG_TIME) ;			/* Hang and try to attach a little later */
	      if ((lt->ActivePid == UNUSEDPID) && (!SPINLOCKFREE(lt->IsActive)))
	       { validuser = FALSE ;				/* Should not get this condition - reset if no other users */
		 for(px=0;px<lt->PidCnt;px++)
		  { if (!PROCESS_GONE(lt->LogPids[px])) { validuser = TRUE ; break ; } ; } ;
		 if ((validuser == FALSE) || clearflag)
	          { px = lt->IsActive ; RLSSPINLOCK(lt->IsActive) ;
		    sprintf(ebuf,"Forced clear of (empty) lock table, ActivePid=%d, IsActive=%d, Clear=%d",
				lt->ActivePid,px,clearflag) ;
		    printf("? Abnormal area startup: %s\n",ebuf) ; v4is_BugChk(areaid,ebuf) ;
		  } ;
	       } ;
	    } ;
	   if (clearflag)
	    { if (SPINLOCKFREE(lt->IsActive)) { printf("Lock table not currently hung, no ForceClear necessary\n") ; }
	       else { sprintf(ebuf,"Forced clear of lock table, ActivePid=%d, IsActive=%lld, Clear=%d",
				lt->ActivePid,(B64INT)lt->IsActive,clearflag) ;
		      lt->ActivePid = UNUSEDPID ; RLSSPINLOCK(lt->IsActive) ;
		      printf("? Abnormal area startup: %s\n",ebuf) ; v4is_BugChk(areaid,ebuf) ;
		   } ;
	    } ;
#ifdef WINNT
	   lt->PidLoginDTs[lt->PidCnt] = mmm->MyPidLoginDT ;
	   lt->hThreads[lt->PidCnt] = mmm->MyhThread ; lt->idProcesses[lt->PidCnt] = mmm->MyidProcess ;
	   lt->LogPids[lt->PidCnt++] = mmm->MyPid ;		/* Add as new user */
#endif
	   LOCKLT(10)
	   for(lx=0;lx<lt->Count;lx++)
	    { if (lt->Entry[lx].PidCount <= 0) continue ;
	      for(i=0;i<lt->Entry[lx].PidCount;i++)
	       { if (!PROCESS_GONE(lt->Entry[lx].Pids[i])) continue ;
		 lt->Entry[lx].Pids[i] = lt->Entry[lx].Pids[lt->Entry[lx].PidCount-1] ;
		 lt->Entry[lx].AreaIds[i] = lt->Entry[lx].AreaIds[lt->Entry[lx].PidCount-1] ;
		 lt->Entry[lx].PidCount-- ; i-- ;
	       } ;
	      if (lt->Entry[lx].PidCount > 0) { validuser = TRUE ; }	/* If still a pid then valid, else trash lock */
	       else { lt->Entry[lx] = lt->Entry[lt->Count-1] ; lt->Count-- ; lx-- ; } ;
	    } ;
	   if (!validuser)						/* If no valid users then reset lock info */
	    { lt->Count = 0 ; lt->LocksIDX = 0 ; lt->LocksTree = 0 ; lt->LastUniqueLockId = 123 ; } ;
	   validuser = FALSE ;
	   for(px=0;px<lt->PidCnt;px++)
	    { if (PROCESS_GONE(lt->LogPids[px]))
	       {
/* --------------------------
		 printf("Area %d (%s) - Trashing defunct user at position %d, invalid Pid=%d\n",
				areaid,filename,px,lt->Pids[px]) ;
	         mmm->Areas[ax].GBPtr = glb ;
		 sprintf(ebuf,"Trashing defunct pid (%d) at %d of %d",lt->LogPids[px],px,lt->PidCnt) ;
		 v4is_BugChk(areaid,ebuf) ;
*/ /* -------------------------- */
#ifdef WINNT
	         lt->PidLoginDTs[i] = lt->PidLoginDTs[lt->PidCnt-1] ;
	         lt->hThreads[i] = lt->hThreads[lt->PidCnt-1] ; lt->idProcesses[i] = lt->idProcesses[lt->PidCnt-1] ;
#endif
		 lt->LogPids[px] = lt->LogPids[--lt->PidCnt] ;
	       } else validuser = TRUE ;
	    } ;
	   if (lt->Count == 0 && (!validuser))			/* If no valid users then reset lock info */
	    { /* printf("Resetting global segment for area #%d - no active users\n",areaid) ; */
	      for(bx=0;bx<glb->BktCnt;bx++) { if (glb->Bkt[bx].Updated) break ; } ;
	      if (bx >= glb->BktCnt)
	       {
/*
	         mmm->Areas[ax].GBPtr = glb ;
	         v4is_BugChk(areaid,"Calling init_seg - appear to have empty area") ;
*/
	         goto init_seg ;		/* Reset if no locks, no known users, no updated buckets */
	       } ;
	    } ;
	   if (lt->PidCnt >= V4MM_LockTable_PidMax)
	    { v4mm_OpenCloseLock(FALSE) ; RELLT(33)
	      v4_error(V4E_MAXGLBPIDS,0,"V4MM","ShareSegInit","MAXGLBPIDS","Exceeded max number of concurrent pids to global segment: %s",filename) ;
	    } ;
#ifdef WINNT
	   lt->PidLoginDTs[lt->PidCnt] = mmm->MyPidLoginDT ;
	   lt->hThreads[lt->PidCnt] = mmm->MyhThread ; lt->idProcesses[lt->PidCnt] = mmm->MyidProcess ;
#endif
	   lt->LogPids[lt->PidCnt++] = mmm->MyPid ;		/* Add as new user */
#ifdef WINNT
	   lt->PidCnt-- ;					/* User added twice, remove one */
#endif
	   v4mm_OpenCloseLock(FALSE) ;
	   RELLT(34)				/* Lock table available for use */
	   mmm->Areas[ax].GBPtr = glb ;				/* Save pointer to global area */
	   mmm->Areas[ax].LockIDX = UNUSED ; mmm->Areas[ax].LockDAT = UNUSED ;
	   mmm->Areas[ax].LockRec = UNUSED ; mmm->Areas[ax].LockTree = UNUSED ; mmm->Areas[ax].LockExtra = UNUSED ;
	   return ;
	 } ;
#ifdef UNIX
	if (errno != ENOENT)
	 { v4mm_OpenCloseLock(FALSE) ;
	   sprintf(ebuf,"Unexpected err (%d) in shmget(%d,%d,xx) on file (%s)",
			errno,mmm->Areas[ax].SegKey,segbytes,UCretASC(mmm->Areas[ax].UCFileName)) ;
	   v4is_BugChk(areaid,ebuf) ; v4_error(V4E_SHMGETERR,0,"V4MM","ShareSegInit","SHMGETERR",ebuf) ;
	 } ;
#endif
/*	If here then have to create a global segment */
#ifdef UNIX
create_seg:
	if ((mmm->Areas[ax].SegId = shmget(mmm->Areas[ax].SegKey,segbytes,IPC_CREAT+0660)) < 0)
	 { sprintf(ebuf,"Err in shmget: errno=%d, Area=(%d %s), bytes=%d, locks=%d",
			errno,areaid,UCretASC(mmm->Areas[ax].UCFileName),segbytes,lckcnt) ;
	   v4mm_OpenCloseLock(FALSE) ;
	   v4_error(V4E_SHMGETFAIL,0,"V4MM","ShareSegInit","SHMGETFAIL",ebuf) ;
	 } ;
	if ((int)(glb = (struct V4MM__GlobalBuf *)shmat(mmm->Areas[ax].SegId,(char*)0,0)) == -1)
	 { sprintf(ebuf,"Err in shmat: errno=%d, aid=%d, file=%s, segid=%d, bytes=%d, locks=%d",
		errno,areaid,filename,mmm->Areas[ax].SegId,segbytes,lckcnt) ;
	   v4mm_OpenCloseLock(FALSE) ;
	   shmctl(mmm->Areas[ax].SegId,IPC_STAT,&buf) ;
	   shmctl(mmm->Areas[ax].SegId,IPC_RMID,&buf) ;	/* Try to remove to avoid problems later */
	   v4_error(V4E_SHMATFAIL,0,"V4MM","ShareSegInit","SHMATFAIL",ebuf) ;
	 } ;
#endif
/*	Have to initialize the global area */
init_seg:
	glb->BktCnt = bktcnt ; glb->BktSize = bktsize ; glb->TotalCalls = 0 ;
	for(bx=0;bx<glb->BktCnt;bx++) { glb->Bkt[bx].BktNum = UNUSED ; glb->Bkt[bx].CallCnt = 0 ; } ;
	glb->LockTableOffset = 0 ;				/* Initialize the lock table */
	lt = (struct V4MM__LockTable *)&glb->Buffer[glb->LockTableOffset] ;		/* Link up */
	lt->Count = 0 ; lt->MaxAllowed = lckcnt ;		/* Set up maxes */
	lt->LocksIDX = 0 ; lt->LocksTree = 0 ; lt->LastUniqueLockId = 345 ;
	for(lx=0;lx<lt->MaxAllowed;lx++) { lt->Entry[lx].PidCount = 0 ; } ;	/* Make entire lock table "empty" */
	glb->BktOffset = ((sizeof *lt - sizeof lt->Entry + (sizeof lt->Entry[0] * lckcnt)) + ALIGN_MAX) & ~ALIGN_MAX ;
	mmm->Areas[ax].LockIDX = UNUSED ; mmm->Areas[ax].LockDAT = UNUSED ;
	mmm->Areas[ax].LockRec = UNUSED ; mmm->Areas[ax].LockTree = UNUSED ; mmm->Areas[ax].LockExtra = UNUSED ;
	lt->PidCnt = 0 ;
#ifdef WINNT
	lt->PidLoginDTs[lt->PidCnt] = mmm->MyPidLoginDT ;
	lt->hThreads[lt->PidCnt] = mmm->MyhThread ; lt->idProcesses[lt->PidCnt] = mmm->MyidProcess ;
#endif
	lt->LogPids[lt->PidCnt++] = mmm->MyPid ;	/* Add as new user */
	v4mm_OpenCloseLock(FALSE) ;
	lt->ActivePid = UNUSEDPID ; RLSSPINLOCK(lt->IsActive) ;	/* Lock table ready to rock & roll */
	mmm->Areas[ax].GBPtr = glb ;				/* Save pointer to global area */
}
#endif	/* segshare from way back at top of page */

#ifdef WINNT

/*	v4mm_WinNTProcessGone - Returns TRUE if specified process no longer running */
/*	Call: logical = v4mm_WinNTProcessGone( pid )
	  where logical is TRUE if process not active,
		pid is pid under question				*/

LOGICAL v4mm_WinNTProcessGone(pid,lt)
 PID pid ;
 struct V4MM__LockTable *lt ;
{ 
//  struct V4MM__MemMgmtMaster *mmm ;
  FILETIME ftCreate,ftExit,ftKernel,ftUser ;
  HANDLE hProcess,hThread ;
  int i ;

//	if( (mmm = V4_GLOBAL_MMM_PTR) == NULL) return(FALSE) ;
	if (lt == NULL) return(FALSE) ;			/* If no lock table then assume thread still exists */
	if (pid == UNUSEDPID) return(FALSE) ;		/* If UNUSED (from LOCKLT) then return "exists" */
	if (pid == WINNT_PID) return(FALSE) ;		/* We are still here! */
	for(i=0;i<lt->PidCnt;i++) { if (lt->LogPids[i] == pid) break ; } ;
	if (i >= lt->PidCnt)
	 { printf("Could not find pid (%d) in LogPid list\n",pid) ;
	   return(TRUE) ;		/* Not in table- process/thread is gone */
	 } ;
	hProcess = OpenProcess(PROCESS_ALL_ACCESS,TRUE,lt->idProcesses[i]) ;	/* Get handle for process */
	if (hProcess == NULL)
	 { printf("Error %s in OpenProcess for Pid (%d), idProcess[%d] (%d)\n",v_OSErrString(GetLastError()),pid,i,lt->idProcesses[i]) ;
	   return(TRUE) ;		/* No handle = no process */
	 } ;
	if (!DuplicateHandle(hProcess,lt->hThreads[i],GetCurrentProcess(),&hThread,THREAD_ALL_ACCESS,TRUE,0))
	 { CloseHandle(hProcess) ;
	   v4_error(V4E_DUPTHREADHANDLE,0,"V4MM","WinNTProcessGone","DUPTHREADHANDLE","Error (%s) duplicating current thread handle",v_OSErrString(GetLastError())) ;
	 } ;
	CloseHandle(hProcess) ;
	if (!GetThreadTimes(hThread,&ftCreate,&ftExit,&ftKernel,&ftUser))
	 { printf("Error %s getting ThreadTimes\n",v_OSErrString(GetLastError())) ; CloseHandle(hThread) ; return(TRUE) ; } ;
	CloseHandle(hThread) ;
	return(lt->PidLoginDTs[i] != ftCreate.dwLowDateTime) ;
}
#endif /* WINNT */

#ifdef VMSOS

/*	v4mm_VaxVMSProcessGone - Returns TRUE if specified process no longer running */
/*	Call: logical = v4mm_VaxVMSProcessGone( pid )
	  where logical is TRUE if process not active,
		pid is pid under question				*/

v4mm_VaxVMSProcessGone(pid)
 int pid ;
{
  struct {
    short int buffer_length ;			/* Length of buffer */
    short int item_code ;			/* Code of what we want- JPI$_xxx */
    int buffer_address ;			/* Where VMS is to put result */
    int return_length_address ;
    int end_of_list ;
   } jpi_table ;
  int datetime[2] ;
  globalvalue int JPI$_LOGINTIM,SS$_NONEXPR ;

	jpi_table.buffer_length = 8 ; jpi_table.item_code = JPI$_LOGINTIM ; jpi_table.buffer_address = &datetime ;
	if (sys$getjpiw(0,&pid,0,&jpi_table,0,0,0) == SS$_NONEXPR) return(TRUE) ;
	return(FALSE) ;			/* If not non-existant then must be OK */
}

/*	V 4 I S  -  R M S   I N T E R F A C E			*/

/*	v4is_RMSOpen - Opens up area as VMS/RMS File		*/
/*	Call: v4is_RMSOpen( pcb )
	  where pcb is parameter control block			*/

v4is_RMSOpen(pcb)
  struct V4IS__ParControlBlk *pcb ;
{
   struct FAB *fabp ; struct RAB *rabp ; struct NAM *namp ;
   static int LastRMSAreaId ;
   char errbuf[250] ;
   char xnam[V_FileName_Max] ;		/* Holds expanded file name */
   int rms_res,newfile ;

/*	Allocate & init rms structures */
	fabp = malloc(sizeof *fabp) ; *fabp = cc$rms_fab ;
	rabp = malloc(sizeof *rabp) ; *rabp = cc$rms_rab ;
	namp = malloc(sizeof *namp) ; *namp = cc$rms_nam ;
	namp->nam$l_rsa = &xnam ; namp->nam$b_rss = V4_SysFileName_Max ;	/* Set up file name */
	rabp->rab$l_fab = fabp ; fabp->fab$l_nam = namp ;
	fabp->fab$l_fna = &pcb->FileName ; fabp->fab$b_fns = strlen(pcb->FileName) ;
	fabp->fab$l_dna = NULL ; fabp->fab$b_dns = 0 ;			/* No name defaults */
	pcb->FilePtr = (FILE *)rabp ;					/* Link fab address to user's unit */
	if (pcb->AccessMode & V4IS_PCB_AM_Get) fabp->fab$b_fac |= FAB$M_GET ;
	if (pcb->AccessMode & V4IS_PCB_AM_Delete) fabp->fab$b_fac |= FAB$M_DEL ;
	if (pcb->AccessMode & V4IS_PCB_AM_Update) fabp->fab$b_fac |= FAB$M_UPD ;
	if (pcb->AccessMode & V4IS_PCB_AM_Insert) fabp->fab$b_fac |= FAB$M_PUT ;
	fabp->fab$w_mrs = pcb->MaxRecordLen ;				/* Max record length */
	if (pcb->AccessMode & V4IS_PCB_AMF_Sequential)			/* Opening for Sequential Output only ? */
	 { fabp->fab$b_org = FAB$C_SEQ ; } else { fabp->fab$b_org = FAB$C_IDX ; } ;
	switch (pcb->OpenMode)
	 { default:
	   case V4IS_PCB_OM_Read:	newfile = FALSE ; break ;
	   case V4IS_PCB_OM_Update:	newfile = FALSE ; break ;
	   case V4IS_PCB_OM_New:	newfile = TRUE ; break ;
	   case V4IS_PCB_OM_NewIf:	newfile = TRUE ; fabp->fab$l_fop |= FAB$M_CIF ; break ;
	   case V4IS_PCB_OM_NewAppend:	newfile = FALSE ; break ;
	   case V4IS_PCB_OM_NewTemp:	newfile = TRUE ; fabp->fab$l_fop | FAB$M_DLT ; break ;
	 } ;
	fabp->fab$b_rfm = FAB$C_VAR ;				/* Assume variable length records */
	fabp->fab$b_rat = FAB$M_CR ;				/*  & carriage control */
	if (pcb->LockMode != 0) fabp->fab$b_shr = (FAB$M_PUT | FAB$M_GET | FAB$M_DEL | FAB$M_UPD) ;
/*	Ready to roll - try to open the file */
	if (newfile) { rms_res = sys$create(fabp) ; } else { rms_res = sys$open(fabp) ; } ;
	if ((rms_res & 1) == 0) v4_error(V4E_RMSOPENERR,0,"V4IS","RMSOpen","RMSOPENERR","V4IS/RMS Open Error on Area (%s) - %s",
					pcb->FileName,v4is_RMSErrMsg(rms_res,errbuf)) ;
	rms_res = sys$connect(rabp) ;
	if ((rms_res & 1) == 0) v4_error(V4E_RMSOPENERR,0,"V4IS","RMSOpen","RMSOPENERR","V4IS/RMS Open Error on Area (%s) - %s",
					pcb->FileName,v4is_RMSErrMsg(rms_res,errbuf)) ;
	xnam[namp->nam$b_rsl] = 0 ; strcpy(pcb->FileName,xnam) ;
	if (LastRMSAreaId == 0) LastRMSAreaId = 10000 ;		/* Have to assign unique RMS area id */
	pcb->AreaId = (LastRMSAreaId++) ;
}

/*	v4is_RMSClose - Closes a V4IS/RMS Area			*/

v4is_RMSClose(pcb)
  struct V4IS__ParControlBlk *pcb ;
{
   int rms_res ; char errbuf[250] ;

/*	Must be an RMS file */
	rms_res = sys$close((((struct RAB *)pcb->FilePtr)->rab$l_fab)) ;
	if ((rms_res & 1) == 0) v4_error(V4E_RMSCLOSEERR,0,"V4IS","RMSClose","RMSCLOSEERR","V4IS/RMS CloseError on Area (%s) - %s",
					pcb->FileName,v4is_RMSErrMsg(rms_res,errbuf)) ;
	v4mm_FreeChunk(((struct RAB *)pcb->FilePtr)->rab$l_fab->fab$l_nam) ;
	v4mm_FreeChunk(((struct RAB *)pcb->FilePtr)->rab$l_fab) ;
	v4mm_FreeChunk(((struct RAB *)pcb->FilePtr)) ; pcb->FilePtr = NULL ;
}

/*	v4is_RMSGet - Gets a V4IS/RMS Record				*/

v4is_RMSGet(pcb)
  struct V4IS__ParControlBlk *pcb ;
{ char errbuf[250] ;
   struct RAB *rabp ;
   struct V4CS__Msg_DataRec *dr ;
   char lclbuf[0x7fff] ;		/* Local buffer for record */
   int i,mode,rms_res,bof ; char *iobuf ;

/*	Best be an rms file */
	if ((rabp = (struct RAB *)pcb->FilePtr) == NULL) v4_error(V4E_NOTOPEN,0,"V4IS","RMSGet","NOTOPEN","Unit is not open") ;
	pcb->GetCount ++ ;
	rabp->rab$l_rop = 0 ; rabp->rab$l_rfa0 = pcb->DataId ; rabp->rab$w_rfa4 = pcb->DataId2 ;
	if (pcb->GetMode != 0) { mode = pcb->GetMode ; pcb->GetMode = 0 ; } else { mode = pcb->DfltGetMode ; } ;
	bof = FALSE ;
	switch (mode & 0xffff)
	 { default:
	   case V4IS_PCB_GP_Keyed:	rabp->rab$b_rac = RAB$C_KEY ; break ;
	   case V4IS_PCB_GP_DataOnly:	/* RMS doe not have data-only - default to sequential next */
	   case V4IS_PCB_GP_Next:	rabp->rab$b_rac = RAB$C_SEQ ; break ;
	   case V4IS_PCB_GP_BOF:	bof = TRUE ; break ;
	   case V4IS_PCB_GP_EOF:	rabp->rab$l_rop |= RAB$M_EOF ; break ;
	   case V4IS_PCB_GP_KeyedNext:	rabp->rab$b_rac = RAB$C_KEY ; rabp->rab$l_rop |= RAB$M_KGE ; break ;
	   case V4IS_PCB_GP_DataId:	rabp->rab$b_rac = RAB$C_RFA ; break ;
	 } ;
/*	Link user buffer to rab */
	if (pcb->GetBufPtr != NULL) { iobuf = pcb->GetBufPtr ; pcb->GetBufPtr = NULL ; }
	 else { iobuf = pcb->DfltGetBufPtr ; } ;
	rabp->rab$l_ubf = (mode & V4IS_PCB_CSDataRec ? lclbuf : iobuf) ;
	if (pcb->GetBufLen != 0) { i = pcb->GetBufLen ; pcb->GetBufLen = 0 ; } else { i = pcb->DfltGetBufLen ; } ;
	rabp->rab$w_usz = (i > 0x7fff ? 0x7fff : i) ;		/* RMS can only handle 32K record size */
	if ((mode & (V4IS_PCB_NoLock | V4IS_PCB_IgnoreLock)) != 0)
	 rabp->rab$l_rop |= (RAB$M_NLK | RAB$M_RRL) ;
/*	Figure out key info */
	if (pcb->KeyNum > 0) { rabp->rab$b_krf = pcb->KeyNum-1 ; pcb->KeyNum = 0 ; }
	 else { rabp->rab$b_krf = (pcb->DfltKeyNum > 0 ? pcb->DfltKeyNum-1 : 0) ; } ;
	if (pcb->KeyPtr != NULL) { rabp->rab$l_kbf = pcb->KeyPtr ; pcb->KeyPtr = NULL ; }
	 else { rabp->rab$l_kbf = pcb->DfltKeyPtr ; } ;
	if (pcb->KeyLen != 0) { rabp->rab$b_ksz = pcb->KeyLen ; pcb->KeyLen = 0 ; }
	 else { rabp->rab$b_ksz = pcb->DfltKeyLen ; } ;
	if (bof) rms_res = sys$rewind(rabp) ;
	rms_res = sys$get(rabp) ;
	if ((rms_res & 1) != 0) { pcb->GetLen = rabp->rab$w_rsz ; goto got_record ; } ;
/*	Got an error - see if we can figure it out */
	if (rms_res == RMS$_EOF) v4_error(V4E_EOF,0,"IO","GET","EOF","End of file reached") ;
	if (rms_res == RMS$_RLK) v4_error(V4E_LOCKED,0,"IO","GET","LOCKED","Record is locked") ;
	if (rms_res == RMS$_RTB) v4_error(V4E_RECTOOBIG,0,"IO","GET","RECTOOBIG","Record too big for buffer") ;
/*	Just generate read error */
	v4_error(V4E_GET,0,"IO","GET","GET","V4IS/RMS Area (%s), Get error: %s",pcb->FileName,v4is_RMSErrMsg(rms_res,errbuf)) ;
/*	Here to do final processing on record */
got_record:
/*	Save the VMS/RMS RFA */
	pcb->DataId = rabp->rab$l_rfa0 ; pcb->DataId2 = rabp->rab$w_rfa4 ;
#ifdef V4_BUILD_CS_FUNCTIONALITY
	if (mode & V4IS_PCB_CSDataRec)
	 { dr = (struct V4CS__DataRec *)iobuf ;
	   dr->DataLen = data_compress(&dr->Buffer,lclbuf,pcb->GetLen,0,sizeof dr->Buffer) ;
	   dr->DataId = pcb->DataId ; dr->DataId2 = pcb->DataId2 ;
	   dr->AreaId = pcb->AreaId ; dr->CmpMode = V4IS_DataCmp_Mth1 ;
	 } ;
#endif
}

/*	v4is_RMSPut - Writes out a V4IS/RMS Record			*/

v4is_RMSPut(pcb)
  struct V4IS__ParControlBlk *pcb ;
{
   struct RAB *rabp ;
   char errbuf[250] ;
   int rms_res,i ;

	if ((rabp = ((struct RAB *)pcb->FilePtr)) == NULL)
	 v4_error(V4E_NOTOPEN,0,"V4IS","RMSPut","NOTOPEN","Unit is not open") ;
	pcb->PutCount ++ ;
	if (pcb->PutBufPtr != NULL) { rabp->rab$l_rbf = pcb->PutBufPtr ; pcb->PutBufPtr = NULL ; }
	 else { rabp->rab$l_rbf = pcb->DfltPutBufPtr ; } ;
	if (pcb->PutBufLen != 0) { rabp->rab$w_rsz = pcb->PutBufLen ; pcb->PutBufLen = 0 ; }
	 else { rabp->rab$w_rsz = pcb->DfltPutBufLen ; } ;
	if (pcb->PutMode != 0) { i = pcb->PutMode ; pcb->PutMode = 0 ; } else { i = pcb->DfltPutMode ; } ;
	rabp->rab$l_rop = 0 ; rabp->rab$b_rac = RAB$C_KEY ;
	switch (i)
	 { default:
		v4_error(V4E_INVPUTMODE,0,"IO","PUT","INVPUTMODE","V4IS/RMS Area (%s), Put error: Invalid put mode (%d)",pcb->FileName,i) ;
	   case V4IS_PCB_GP_Delete:		rms_res = sys$delete(rabp) ; break ;
	   case V4IS_PCB_GP_Obsolete:		rms_res = sys$delete(rabp) ; break ;	/* Just delete to "mark" as obsolete */
	   case V4IS_PCB_GP_Update:		rms_res = sys$update(rabp) ; break ;
	   case V4IS_PCB_GP_Insert:		rms_res = sys$put(rabp) ; break ;
	   case V4IS_PCB_GP_Write:		rabp->rab$l_rop |= RAB$M_UIF ; rms_res = sys$put(rabp) ; break ;
	   case V4IS_PCB_GP_Unlock:		rms_res = sys$free(rabp) ; break ;
	 } ;
	if ((rms_res & 1) == 0)
	 v4_error(V4E_RMSPUTERR,0,"IO","PUT","RMSPUTERR","V4IS/RMS Area (%s), Put error: %s",pcb->FileName,v4is_RMSErrMsg(rms_res,errbuf)) ;
/*	Save the VMS/RMS RFA */
	pcb->DataId = rabp->rab$l_rfa0 ; pcb->DataId2 = rabp->rab$w_rfa4 ;
}

/*	v4is_RMSErrMsg - Converts VMS error code to ascii string */

v4is_RMSErrMsg(vms_code,str)
  int vms_code ;			/* Standard vms error value */
  char *str ;				/* Updated with string equivalent */
 { struct {
      short int length,desc ;
      char *ptr ;
    } str_dsc ;				/* VMS string descriptor */
   short int res_len ;

	str_dsc.length = 250 ; str_dsc.desc = 0 ; str_dsc.ptr = str ;
	sys$getmsg(vms_code,&res_len,&str_dsc,1,0) ; *(str+res_len) = 0 ;
	return(str) ;
}
#endif
