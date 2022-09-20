
/*	V4TEST.C - Test Program

	Created 6-Apr-92 by Victor E. Hansen		*/

#include "v4defs.c"
#include <setjmp.h>
#include <time.h>
#ifdef UNIX
#include <sys/ioctl.h>
#endif
#ifdef ALPHANT
#include <io.h>
#endif
#ifdef ALPHAOSF
#include <sys/sysinfo.h>
#include <sys/proc.h>
#endif
#ifdef WINNT
#include <io.h>
#endif
#include <signal.h>
//void v_signal_ill() ;
//void v_signal_fpe() ;
//void v_signal_bus() ;
#ifdef VMSOS
void vax_signal_ill() ;
void vax_signal_fpe() ;
#endif

//#define V4_GLOBAL_MMM_PTR v4mm_GetMMMPtr()

V4DEBUG_SETUP
extern struct V4C__ProcessInfo *gpi  ;	/* Global process information */
extern ETYPE traceGlobal ;

static int inV4Error = FALSE ;		/* Set to TRUE when in V4 error routine(s) - to prevent nested calls */

#define TOMAKE 7000

#ifdef WINNT
UCCHAR **startup_envp ;
#else
char **startup_envp ;
#endif

#ifdef WINNT
  FILETIME ftKernel2,ftUser2 ;
#endif

#if defined WINNT && defined V4UNICODE
int wmain(argc,argv,envp)
  int argc ;			/* Number of arguments passed via DCL */
  UCCHAR *argv[] ;		/* Argument values */
  UCCHAR **envp ;			/* Environment pointer */
#else
int main(argc,argv,envp)
  int argc ;			/* Number of arguments passed via DCL */
  char *argv[] ;		/* Argument values */
  char **envp ;			/* Environment pointer */
#endif
{
  struct V4IS__RootInfo root ;
  struct V4IS__IndexBktHdr ibh ;
  struct V4IS__ParControlBlk pcb,*dpcb ;
  struct V4IS__AreaCB *acb,*dacb ;
  struct V4IS__RefResults refr ;
  struct V4IS__FreeDataInfo fdi ;
  struct V4FFI__KeyInfo ki ;
  struct V4FFI__KeyInfoDtl *kid ;
  struct V4C__Context *ctx ;
  struct V4LEX__TknCtrlBlk *tcb = NULL ;
  struct V4LEX__TablePrsOpt *tpo ;
  struct V4IS__SeqFileList sfl ;
  struct V4IS__IndexOnly io ;
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4MM__LockTable *lt ;
  struct V4MM__GlobalBuf *glb ;
  struct UC__File UCFile ;
  time_t time_of_day ;
//  double cpuseconds ;
//  FILE *ifp,*ofp ;
  struct UC__File iUCFile,oUCFile ;
  int bucketsize,bad ;
  UCCHAR pstr[200],result[250],tbuf[512] ;
  UCCHAR inpprompt[32] ;
  UCCHAR *arg,*resptr,*bp1,*bp2,*bp3 ;
  BYTE *bktp ;
  int i,ax,bx,t,num,fileref,priv,tx,havedasho,v4argx,argx ; LOGICAL ok ;
  int bucketcnt,maxreccnt,echo,silent,bucketnum,fillpercent,listing,listutil,fastrebuild,existsok,recordsize ;
  int outseqhdr,outseqdata,bucketoffset,indexonly,globalbufs,lockmax,verifyforeign,runcmdfile ;
  UCCHAR includefile[V_FileName_Max],rekeyfile[V_FileName_Max],rebuildfile[V_FileName_Max],mergefile[V_FileName_Max],sequentialfile[V_FileName_Max],
	outfile[V_FileName_Max],copyfile[V_FileName_Max],inifile[V_FileName_Max] ;
  UCCHAR sections[1000] ; 
#define UCARGVMAX 50
  static UCCHAR *ucargv[UCARGVMAX] ;		/* Temp (hopefully) structure converting command line to Unicode */
#ifdef ALPHAOSF
  int uacbuf[2] ;
#endif
#ifdef VMSOS
void vms_ctrlp() ;
static short int channel ;
globalvalue IO$_SETMODE,IO$M_OUTBAND ;
struct {
  int zeros ;
  int mask ;
 } ctrl ;
#endif
#ifdef UNIX
#include <sys/resource.h>
   struct rlimit rlim ;
   void unix_ctrlp() ;
#endif

#ifdef ALPHAOSF
	uacbuf[0] = SSIN_UACPROC ; uacbuf[1] = UAC_NOPRINT | UAC_SIGBUS | UAC_NOFIX ; /* Want unaligned access to blow big time! */
	setsysinfo(SSI_NVPAIRS,&uacbuf,1,0,0) ;
#endif
#ifdef VMSOS
	signal(SIGILL,vax_signal_ill) ; signal(SIGFPE,vax_signal_fpe) ;
	signal(SIGBUS,v_signal_bus) ;
/*	Set up qio for ^P interrupt */
	ctrl.mask = 0x10000 ;	/* Set to interrupt on ^P */
	sys$qiow(0,channel,IO$_SETMODE | IO$M_OUTBAND,0,0,0,vms_ctrlp,&ctrl,0,0,0,0) ;
#else
//	signal(SIGILL,v_signal_ill) ; signal(SIGFPE,v_signal_fpe) ;
#endif
	signal(SIGSEGV,v_signal_bus) ;
#ifdef UNIX
	signal(SIGTSTP,unix_ctrlp) ;
	signal(SIGBUS,v_signal_bus) ;
	setvbuf(stdout,NULL,_IONBF,0) ;		/* Disable all buffering on controlling terminal */
#endif
#ifdef WINNT
//	signal(SIGABRT,v_signal_fpe) ;
#endif
#if defined LINUX486 || defined RASPPI
	rlim.rlim_cur = 0xA00000 ; rlim.rlim_max = 0x1700000 ;
	if (setrlimit(RLIMIT_STACK,&rlim) != 0)
	 printf("? Could not setrlimit(RLIMIT_STACK: %d %d) - error #%d\n",rlim.rlim_cur,rlim.rlim_max,errno) ;
#endif


#ifdef _DEBUG
	if (sizeof(struct V4__BktHdr) != 8 || sizeof(struct V4IS__DataBktEntryHdr) != 8 || sizeof(struct V4IS__IndexKeyDataEntry) != 8
		|| sizeof(union V4IS__KeyPrefix) != 4 || sizeof(union V4MM__BIdMask) != 4 || sizeof(union V4MM__DataIdMask) != 4
		|| sizeof(struct V4DPI__LittlePoint) != 20)
	 vout_Text(VOUT_Err,0,"**** PROBLEM - Structure sizes should be checked out! ****\n") ;
#endif
	startup_envp = envp ;
	gpi = v_GetProcessInfo() ;
	if (setjmp(gpi->environment) != 0) exit(EXITABORT) ;

/*	Save startup environment */
	bucketnum = -1 ;
	bucketsize = 0 ;
	recordsize = 0 ;
	bucketcnt = 0 ;
	fileref = -1 ;
	maxreccnt = 0 ;
	echo = FALSE ;
	silent = FALSE ;		/* -1 for no error recovery, 0 for messages & recovery, 1 for no messages/recovery */
	fillpercent = 0 ;
	listing = FALSE ;
	listutil = FALSE ;
	fastrebuild = FALSE ;
	havedasho = FALSE ;		/* Set to TRUE with -o switch */
	v4argx = UNUSED ;		/* Set to argument index of first argument after V4 program (if any) */
	ZUS(includefile) ;		/* Include File Name */
	ZUS(sections) ;
	existsok = FALSE ;
	verifyforeign = FALSE ;
	ZUS(rekeyfile) ;
	ZUS(inifile) ;
	ZUS(rebuildfile) ;
	ZUS(mergefile) ;
	ZUS(outfile) ;
	ZUS(copyfile) ;
	ZUS(rebuildfile) ;
	ZUS(sequentialfile) ;
	bucketoffset = 0 ;		/* Offset to use for index-only areas */
	outseqhdr = -1 ;		/* Output header on sequential dump */
	outseqdata = -1 ;		/* Output data on sequential dump */
	globalbufs = 0 ;		/* Let system determine global buffers */
	lockmax = 0 ;			/* Let system determine lock max */
	runcmdfile = FALSE ;		/* If TRUE then running from a single command file - auto-exit at EOF */
	UCstrcpy(inpprompt,UClit("V4>")) ;	/* Input prompt */
	memset(&iUCFile,0,sizeof iUCFile) ;

#ifdef WINNT
  for(i=0;argv[i]!=NULL&&i<UCARGVMAX-1;i++) { ucargv[i] = argv[i] ; } ; ucargv[i] = NULL ;
#else
{ /* Convert arguments to Unicode */
  UCCHAR *uarg ; char *arg ; int j ;
  for(i=0;argv[i]!=NULL&&i<UCARGVMAX-1;i++)
   { num = strlen(arg=argv[i]) ;
     uarg = (ucargv[i] = v4mm_AllocChunk((num+1)*sizeof(UCCHAR),FALSE)) ;
     for(j=0;;j++) { uarg[j] = arg[j] ; if (arg[j] == '\0') break ; } ;
   } ; ucargv[i] = NULL ;
}
#endif


#define NA if (*arg == 0 || *(++arg) == UCEOS) arg = ucargv[++argx] ; \
	   if (arg == NULL) goto err_eol ; if (*arg == UClit('-')) goto err_av ;
#define NUM(VAR) VAR = UCstrtol(arg,&resptr,10) ; if (*resptr != 0) goto err_ia ;


	for(argx=1;argx<argc;argx++)
	 {
	   arg = ucargv[argx] ;					/* Next argument */
//#ifdef WINNT
//	   if (*arg == '\\') continue ;				/* Skip if begins with "\" (funky NT handling!) */
//#endif
	   if (*arg == UClit('-')) { arg++ ; }
	    else { 						/* Assume an include file */
//		   if (UCstrlen(includefile) != 0)
		   if (UCnotempty(includefile))
		    { if (havedasho)				/* If got -o then have error */
		       { vout_Text(VOUT_Err,0,"*Use \',\' to separate multiple input files\n") ; exit(EXITABORT) ; } ;
		      v4argx = argx ;				/* This is first argument after v4 program */
		      goto run_interpreter ;
		    } ;
	           UCstrcpy(includefile,arg) ; continue ;
		 } ;
	   switch (*arg)
	    { default:	v_Msg(ctx,tbuf,"@*Invalid command line switch (%1U)\n",arg) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ;
	      case UClit('a'):		signal(SIGSEGV,SIG_DFL) ; break ;
	      case UClit('B'):		gpi->breakOnEval = TRUE ; break ;
	      case UClit('b'):
		switch(*(++arg))
		 { default: vout_Text(VOUT_Err,0,"*Invalid -b (expecting -bc or -bs)\n") ; exit(EXITABORT) ;
		   case UClit('c'):	NA ; NUM(bucketcnt) ; break ;
		   case UClit('o'):	NA ; NUM(bucketoffset) ; break ;
		   case UClit('s'):
			switch(*(++arg))
			 { default:	vout_Text(VOUT_Err,0,"*Invalid -bs (expecting -bs or -bsr)\n") ; exit(EXITABORT) ;
			   case 0:	NA ; NUM(bucketsize) ; break ;
			   case UClit('r'):	NA ; NUM(recordsize) ; break ;
			 } ;
			break ;
		 } ; break ;
	      case UClit('c'):
		switch(*(++arg))
		 { default: vout_Text(VOUT_Err,0,"*Invalid -c (expecting -cb or -cl)\n") ; exit(EXITABORT) ;
		   case UClit('b'):	NA ; NUM(globalbufs) ; break ;
		   case UClit('l'):
			NA ; NUM(lockmax) ;
			if (lockmax > V4MM_LockTableEntry_Max)
			 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("*Argument (-bl %d) - cannot exceed %d\n"),lockmax,V4MM_LockTableEntry_Max) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
			break ;
		 } ; break ;
	      case UClit('d'):
		if (*(++arg) == UClit('i')) { bucketnum = 1000000001 ; }	/* Wants all index buckets */
		 else if (*arg == UClit('d')) { bucketnum = 1000000002 ; }	/* Wants all data buckets */
		 else if (*arg == UClit('a')) { bucketnum = 1000000003 ; }	/* Wants all buckets */
		 else if (*arg == UClit('l')) { NA ; NUM(bucketnum) ; bucketnum += 1100000000 ; }
		 else { NA ; NUM(bucketnum) ; } ;			/* Just a particular bucket */
		break ;
	      case UClit('f'):
		switch(*(++arg))
		 { default: vout_Text(VOUT_Err,0,"*Invalid -f (expecting -f -fs or -fm)\n") ; exit(EXITABORT) ;
		   case 0:	NA ; NUM(fileref) ; break ;
		   case UClit('m'):	fileref = 0 ; break ;
		   case UClit('s'):	fileref = -1 ; break ;
		 } ; break ;
	      case UClit('h'):
//		UCsprintf(tbuf,UCsizeof(tbuf),UClit("V4 %d.%d - \"Data that thinks like us\"\nProtected by U.S. Patent 6,470,490 - MKS Inc. (2002)\n"),V4IS_MajorVersion,V4IS_MinorVersion) ;
		v_Msg(NULL,tbuf,"*V4TBanner",V4IS_MajorVersion,V4IS_MinorVersion,gpi->rtOptString) ;vout_UCText(VOUT_Status,0,tbuf) ;
		if (!v_UCFileOpen(&iUCFile,v_GetV4HomePath(UClit("v4help.v4i")),UCFile_Open_Read,TRUE,result,0))
		 { v_Msg(NULL,tbuf,"*V4THelpGone",result) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
		for(;;)
		 { if (v_UCReadLine(&iUCFile,UCRead_UC,tbuf,UCsizeof(tbuf),tbuf) < 0) break ;
		   if (tbuf[0] == UClit('!')) continue ; if (tbuf[0] == UClit('/')) continue ;
		   vout_UCText(VOUT_Status,0,tbuf) ;
		 } ; v_UCFileClose(&iUCFile) ;
		exit(EXITOK) ;
	      case UClit('i'):
		if (*(++arg) == UClit('i')) { UCstrcpy(inifile,UClit("v4.v4i")) ; break ; }
		NA ; UCstrcpy(inifile,arg) ; break ;
	      case UClit('k'):
		NA ; UCstrcpy(rekeyfile,arg) ; break ;
	      case UClit('l'):
		listing = TRUE ;
		switch (*(++arg))
		 { default: vout_Text(VOUT_Err,0,"*Invalid -l option\n") ; exit(EXITABORT) ;
		   case 0:	break ;
		   case UClit('u'):
			switch (*(++arg))
			 { default:		vout_Text(VOUT_Err,0,"*Invalid -lu option\n") ; exit(EXITABORT) ;
			   case 0:		listutil = 3 ; break ;	/* Recap only */
			   case UClit('d'):	listutil = 2 ; break ;	/* Detail listing */
			 } ;
			break ;
		 } ;
		break ;
	      case UClit('m'):
		NA ; NUM(maxreccnt) ; break ;
	      case UClit('o'):
		havedasho = TRUE ;
		switch(*(++arg))
		 { default: vout_Text(VOUT_Err,0,"*Invalid -o (expecting -o, -oc, -of, -om, -or, or -os)\n") ; exit(EXITABORT) ;
		   case UClit('c'):	NA ; UCstrcpy(copyfile,arg) ; break ;
		   case UClit('f'):	NA ; UCstrcpy(outfile,arg) ; fastrebuild = TRUE ; break ;
		   case UClit('m'):	NA ; UCstrcpy(mergefile,arg) ; break ;
		   case UClit('o'):	NA ; UCstrcpy(outfile,arg) ; break ;
		   case UClit('r'):	NA ; UCstrcpy(rebuildfile,arg) ; break ;
		   case UClit('s'):
			switch (*(arg+1))
			 { case UClit('h'):	arg++ ; outseqhdr = TRUE ; if (outseqdata == -1) outseqdata = FALSE ; break ;
			   case UClit('d'):	arg++ ; outseqdata = TRUE ; if (outseqhdr == -1) outseqhdr = FALSE ; break ;
			 } ;
			NA ; UCstrcpy(sequentialfile,arg) ; break ;
		 } ;
		break ;
#ifdef V4_BUILD_SECURITY
	      case UClit('P'):
		NA ; gpi->spwHash64 = v_Hash64(arg) ;
		break ;
#endif
	      case UClit('p'):
		switch (*(++arg))
		 { default: vout_Text(VOUT_Err,0,"*Invalid -p (expecting -pl, -pR, -pC, -pO)\n") ; exit(EXITABORT) ;
		   case UClit('l'):	priv = 1 ; NA ; break ;
		   case UClit('R'):	priv = 2 ; NA ; break ;
		   case UClit('C'):
			switch (*(++arg))
			 { default: vout_Text(VOUT_Err,0,"*Invalid -pC (expecting -pCW, -pCE, -pCR)\n") ; exit(EXITABORT) ;
			   case UClit('W'): priv = 3 ; NA ; break ;
			   case UClit('E'): priv = 8 ; NA ; break ;
			   case UClit('R'): priv = 13 ; NA ; break ;
			 } ; break ;
		   case UClit('O'):	priv = 4 ; NA ; break ;
		   case UClit('B'):	priv = 5 ; NA ; NUM(bx) ; arg = ucargv[++argx] ; break ;
		   case UClit('L'):	priv = 6 ; NA ; NUM(bx) ; arg = ucargv[++argx] ; break ;
		   case UClit('S'):	priv = 7 ; NA ; break ;
		   case UClit('J'):	priv = 9 ; NA ; NUM(bx) ; arg = ucargv[++argx] ; break ;
		   case UClit('b'):	priv = 10 ; NA ; break ;
		   case UClit('N'):	priv = 11 ; NA ; NUM(bx) ; arg = ucargv[++argx] ; break ;
		   case UClit('K'):	priv = 12 ; NA ; break ;
		 } ;
		memset(&pcb,0,sizeof pcb) ; strcpy(pcb.V3name,"testu") ;
		pcb.OpenMode = V4IS_PCB_OM_Read ; pcb.DfltGetMode = V4IS_PCB_GP_Next ;
		if (priv == 11) pcb.OpenMode = V4IS_PCB_OM_Update ;
		if (priv == 1) { pcb.OpenMode = V4IS_PCB_OM_Partial ; pcb.AreaFlags |= V4IS_PCB_OF_ForceOpen ; } ;
		if (priv == 12 || priv == 2 || priv == 6)
		 { if (!existsok) goto err_anc ;
		   if (priv == 12) pcb.AreaFlags |= V4IS_PCB_OF_ForceClear ;
		   pcb.OpenMode = V4IS_PCB_OM_Partial ;		/* Only open enough to get glb/lt */
		 } ;
		UCstrcpy(pcb.UCFileName,arg) ; pcb.LockMode = 1 ;
		v4is_Open(&pcb,NULL,NULL) ;
		if (pcb.AreaId == V4IS_AreaId_SeqOnly)
		 v4_error(V4E_ISSEQONLY,0,"V4TEST","Reformat","ISSEQONLY","Cannot reformat SeqOnly files") ;
		if( (mmm = v4mm_GetMMMPtr()) == NULL)
		 { vout_Text(VOUT_Err,0,"*MM not initialized\n") ; exit(EXITABORT) ; } ;
		for(ax=0;ax<mmm->NumAreas;ax++) { if (mmm->Areas[ax].AreaId == pcb.AreaId) break ; } ;
		if (ax >= mmm->NumAreas) exit(EXITABORT) ;			/* Could not find area? */
		if ( (glb = mmm->Areas[ax].GBPtr) == NULL) exit(EXITABORT) ;
		acb = (struct V4IS__AreaCB *)v4mm_ACBPtr(pcb.AreaId) ;
		lt = (struct V4MM__LockTable *)&glb->Buffer[glb->LockTableOffset] ;
		switch (priv)
		 { case 1:	/* Just list what is going on */
			UCsprintf(tbuf,UCsizeof(tbuf),UClit("SegId is %p, BktCnt is %d, BktSize is %d, TotalCalls are %d\n"),
				mmm->Areas[ax].SegId,glb->BktCnt,glb->BktSize,glb->TotalCalls) ; vout_UCText(VOUT_Status,0,tbuf) ;
			if (glb->PanicLockout != 0)
			 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("Panic Lockout is %d, Pid is %d\n"),glb->PanicLockout,glb->PanicPid) ;
			   vout_UCText(VOUT_Status,0,tbuf) ;
			 } ;
			UCsprintf(tbuf,UCsizeof(tbuf),UClit("Last Unique LockId is %d\n"),lt->LastUniqueLockId) ; vout_UCText(VOUT_Status,0,tbuf) ;
			if (lt->PidCnt > 1) vout_UCText(VOUT_Status,0,UClit("Users logged in:\n")) ;
			for(i=0;i<lt->PidCnt;i++)
			 { if (lt->LogPids[i] != mmm->MyPid)
			    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("   %d. %d\n"),i+1,lt->LogPids[i]) ; vout_UCText(VOUT_Status,0,tbuf) ; } ;
			 } ;
			if (lt->PidCnt <= 1) vout_UCText(VOUT_Status,0,UClit("NO USERS IN AREA\n")) ;
			for(i=0,bx=0;bx<glb->BktCnt;bx++)
			 { if (glb->Bkt[bx].BktNum < 0) continue ;
			   if (i == 0) {i = 1 ; vout_UCText(VOUT_Status,0,UClit("Current Bucket Status\n")) ; } ;
			   UCsprintf(tbuf,UCsizeof(tbuf),UClit("   %d: Num=%d, Pending=%d, PendPid=%d, CallCnt=%d, MemLock=%d, Updated=%d\n"),
					 bx,glb->Bkt[bx].BktNum,glb->Bkt[bx].PendingBktNum,glb->Bkt[bx].PendingPid,
					 glb->Bkt[bx].CallCnt,glb->Bkt[bx].MemLock,glb->Bkt[bx].Updated) ;
			   vout_UCText(VOUT_Status,0,tbuf) ;
			 } ;
			if (lt->Count > 0) vout_UCText(VOUT_Status,0,UClit("Current Lock Status\n")) ;
			for(i=0;i<lt->Count;i++)
			 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("   %d src=%d, type=%d, mode=%d, id=%d, uid=%d, pids=%d (%d %d %d), wait=%d (%d), Tree=%d\n"),
				i,lt->Entry[i].Source,lt->Entry[i].LockIdType,lt->Entry[i].LockMode,lt->Entry[i].LockId,lt->Entry[i].UniqueLockId,
				lt->Entry[i].PidCount,lt->Entry[i].Pids[0],lt->Entry[i].Pids[1],lt->Entry[i].Pids[2],lt->Entry[i].WaitCount,
				lt->Entry[i].WaitPids[0],lt->LocksTree) ; vout_UCText(VOUT_Status,0,tbuf) ;
			 } ;
			break ;
		   case 2:	/* Reset all buffers & locks */
			if (glb->PanicLockout == V4IS_Lockout_Wait || glb->PanicLockout == V4IS_Lockout_Error)
			 { vout_UCText(VOUT_Err,0,UClit("*  Lockout on area already enabled!\n")) ; }
			 else { glb->PanicLockout = V4IS_Lockout_Wait ; glb->PanicPid = mmm->MyPid ;
				vout_UCText(VOUT_Status,0,UClit("   Set lockout on area, waiting for activity to fall off\n")) ; HANGLOOSE(2500) ;
			      } ;
			lt->Count = 0 ; vout_UCText(VOUT_Status,0,UClit("   Reset all locks\n")) ;
			for(bx=0;bx<glb->BktCnt;bx++)
			 { if (glb->Bkt[bx].Updated && glb->Bkt[bx].BktNum != UNUSED)
			    { UCsprintf(tbuf,UCsizeof(tbuf),UClit(" %% Bucket slot (%d), number (%d) marked as updated, not setting as UNUSED\n"),
					bx,glb->Bkt[bx].BktNum) ; vout_UCText(VOUT_Status,0,tbuf) ;
			      continue ;
			    } ;
			   glb->Bkt[bx].BktNum = UNUSED ; glb->Bkt[bx].Updated = FALSE ;
			 } ;
			vout_UCText(VOUT_Status,0,UClit("   Reset global buffers to UNUSED\n")) ;
			v4mm_BktPtr(0,pcb.AreaId,V4MM_LockIdType_Index+V4MM_BktPtr_MemLock) ;
			vout_UCText(VOUT_Status,0,UClit("   Re-read root bucket from file\n")) ;
			glb->PanicLockout = 0 ; glb->PanicPid = UNUSEDPID ;
			vout_Text(VOUT_Status,0,"   Lockout on area has been reset\n") ;
			break ;
		   case 13:	/* Allow only reads on area, user hangs if write attempted */
			if (!existsok) goto err_anc ;
			glb->PanicLockout = V4IS_Lockout_WaitReadOK ;
			glb->PanicPid = mmm->MyPid ;
#ifdef POSIX
			glb->PanicPid = getppid() ;
#endif
#ifdef WINNT
			glb->PanicPid = UNUSEDPID ;
#endif
			break ;
		   case 3:	/* Close off area - users hangs until OK */
			if (!existsok) goto err_anc ;
			glb->PanicLockout = V4IS_Lockout_Wait ;
			glb->PanicPid = mmm->MyPid ;
#ifdef POSIX
			glb->PanicPid = getppid() ;
#endif
#ifdef WINNT
			glb->PanicPid = UNUSEDPID ;
#endif
			break ;
		   case 8:	/* Close off area - users gets error if attempt to access */
			if (!existsok) goto err_anc ;
			glb->PanicLockout = V4IS_Lockout_Error ;
			glb->PanicPid = mmm->MyPid ;
#ifdef POSIX
			glb->PanicPid = getppid() ;
#endif
#ifdef WINNT
			glb->PanicPid = UNUSEDPID ;
#endif
			break ;
		   case 4:	/* Reopen the area */
			if (!existsok) goto err_anc ;
			glb->PanicLockout = 0 ; glb->PanicPid = UNUSEDPID ;
			vout_Text(VOUT_Status,0,"   Lockout on area has been reset\n") ;
		   	break ;
		   case 5:	/* Zap a particular bucket */
			if (bx < 0 || bx >= glb->BktCnt)
			 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("? Invalid bucket index, must be 0 thru %d\n"),glb->BktCnt-1) ; vout_UCText(VOUT_Err,0,tbuf) ; break ; }
			if (glb->Bkt[bx].BktNum == 0)
			 { vout_UCText(VOUT_Err,0,UClit("*Zapping root- cure will be worse than disease (use -pR to reset entire area)\n")) ; break ; } ;
			if (!existsok) goto err_anc ;
			t = v4mm_LockSomething(pcb.AreaId,V4MM_LockId_Alloc,V4MM_LockMode_Write,V4MM_LockIdType_Alloc,01,NULL,26) ;
			glb->Bkt[bx].BktNum = UNUSED ;
			v4mm_LockRelease(pcb.AreaId,t,mmm->MyPid,pcb.AreaId,"xRELALLOC") ;
			break ;
		  case 6:	/* Zap a particular lock */
			if (bx < 0)
			 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("? Invalid lock id (%d), must be positive\n"),bx) ; vout_UCText(VOUT_Err,0,tbuf) ; break ; }
			if (!existsok) goto err_anc ;
			for(tx=0;;tx++)
			 { if (GETSPINLOCK(&lt->IsActive)) break ;
			   HANGLOOSE(V4IS_HANG_TIME) ; if (tx < 10) continue ;
			   if (PROCESS_GONE(lt->ActivePid)) { lt->ActivePid = UNUSEDPID ; RLSSPINLOCK(lt->IsActive) ; continue ; } ;
			   if (tx > 100)
			    { vout_UCText(VOUT_Status,0,UClit("*Forcing a grab on lock table...\n")) ; RLSSPINLOCK(lt->IsActive) ; } ;
			 } ; lt->ActivePid = mmm->MyPid ; lt->NestLockCount = 0 ;
			for(i=0;i<lt->Count;i++) { if (lt->Entry[i].UniqueLockId == bx) break ; } ;
			if (i < lt->Count) { lt->Entry[i] = lt->Entry[lt->Count-1] ; lt->Count-- ; }
			 else { i = -1 ; } ;
			lt->ActivePid = UNUSEDPID ; RLSSPINLOCK(lt->IsActive) ;
			if (i == -1)
			 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("? No action taken, could not find lock with specified ID (%d)\n"),bx) ; vout_UCText(VOUT_Err,0,tbuf) ; } ;
			break ;
		  case 7:	/* Reset lt->LocksIDX if necessary */
			if (!existsok) goto err_anc ;
			for(tx=0;;tx++)
			 { if (GETSPINLOCK(&lt->IsActive)) break ;
			   HANGLOOSE(V4IS_HANG_TIME) ; if (tx < 10) continue ;
			   if (PROCESS_GONE(lt->ActivePid)) { lt->ActivePid = UNUSEDPID ; RLSSPINLOCK(lt->IsActive) ; continue ; } ;
			   if (tx > 100)
			    { vout_UCText(VOUT_Status,0,UClit("*Forcing a grab on lock table...\n")) ; RLSSPINLOCK(lt->IsActive) ; } ;
			 } ; lt->ActivePid = mmm->MyPid ; lt->NestLockCount = 0 ;
			for(num=0,i=0;i<lt->Count;i++)
			 { if (lt->Entry[i].LockIdType == V4MM_LockIdType_Index && lt->Entry[i].LockMode == V4MM_LockMode_Write)
			    num++ ;
			 } ;
			if (num != lt->LocksIDX)
			 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("... Changed lt->LocksIDX from %d to %d\n"),lt->LocksIDX,num) ;
			   vout_UCText(VOUT_Status,0,tbuf) ; lt->LocksIDX = num ;
			 } else { vout_UCText(VOUT_Status,0,UClit("... No change to lt->LocksIDX was necessary\n")) ; } ;
			lt->ActivePid = UNUSEDPID ; RLSSPINLOCK(lt->IsActive) ;
			break ;
		  case 9:	/* Jerk-off lt->IsActive bx-million times */
			if (!existsok) goto err_anc ;
			HANGLOOSE(5000) ;		/* Give other particpants a chance to open file */
			UCsprintf(tbuf,UCsizeof(tbuf),UClit("Pid %d is starting, ActivePid=%d, IsActive=%d\n"),mmm->MyPid,lt->ActivePid,lt->IsActive) ; vout_UCText(VOUT_Status,0,tbuf) ;
			for(num=0;bx>0;bx--)
			 { for(i=1000000;i>0;i--)
			    { if (!GETSPINLOCK(&lt->IsActive))
			       { if ((i % 1000) == 0) { HANGLOOSE(1) ; } ; continue ; } ;
			      if (lt->ActivePid != UNUSEDPID)
			       { UCsprintf(tbuf,UCsizeof(tbuf),UClit("*Err- Pid = %d, Nest = %d, IsActive = %d"),lt->ActivePid,lt->NestLockCount,lt->IsActive) ;
			         vout_UCText(VOUT_Status,0,tbuf) ;
			       } ;
			      lt->ActivePid = mmm->MyPid ; lt->NestLockCount = 0 ; num++ ;
			      if ((i % 1000) == 0) { HANGLOOSE(1) ; } ;
			      if (lt->ActivePid != mmm->MyPid)
			       { UCsprintf(tbuf,UCsizeof(tbuf),UClit("? Rel- pid=%d Nest=%d\n"),lt->ActivePid,lt->NestLockCount) ; vout_UCText(VOUT_Err,0,tbuf) ; } ;
			      lt->ActivePid = UNUSEDPID ; RLSSPINLOCK(lt->IsActive) ;
			      if ((i % 1000) == 0) { HANGLOOSE(1) ; } ;
		            } ;
			   UCsprintf(tbuf,UCsizeof(tbuf),UClit("  pid = %d, loop = %d, locks = %d, IsActive=%d, ActivePid=%d\n"),mmm->MyPid,bx,num,lt->IsActive,lt->ActivePid) ; vout_UCText(VOUT_Status,0,tbuf) ;
			 } ;
			break ;
		  case 10:				/* Just generate a bug check dump */
			v4is_BugChk(pcb.AreaId,"Test bug-check generated by: xv4 -pb") ;
			break ;
		  case 11:				/* -pN num: Set NextAvailUserNum in RootInfo */
			if (!existsok) goto err_anc ;
			UCsprintf(tbuf,UCsizeof(tbuf),UClit(" Next available user number: %d => %d\n"),acb->RootInfo->NextAvailUserNum,bx) ;
			vout_UCText(VOUT_Status,0,tbuf) ;
			acb->RootInfo->NextAvailUserNum = bx ;
			v4mm_BktUpdated(pcb.AreaId,0) ;
			break ;
		  case 12:				/* pK - Force clear of lock table (done at open!) */
			break ;
		 } ;
		v4is_Close(&pcb) ;
	      	exit(EXITOK) ;
	      case UClit('r'):
		switch(*(++arg))
		 { default:	vout_UCText(VOUT_Err,0,UClit("*Invalid -r (expecting -v or -vf)\n")) ; exit(EXITABORT) ;
		   case	UClit('e'):		/* -re echomode */
			if (*(arg+1) == 0) { echo = 1 ; break ; } else { arg++ ; } ;
			NUM(echo) ; break ;
		   case UClit('o'):		/* -rpo user-runtime-options */
			NA
			if (UCstrlen(arg) >= UCsizeof(gpi->userOptString)) { vout_UCText(VOUT_Err,0,UClit("*-ro string exceeds max allowed\n")) ; exit(EXITABORT) ; } ;
			UCstrcpy(gpi->userOptString,arg) ; break ;
		   case UClit('p'):		/* -rpx value */
			if (tcb == NULL)
			 { tcb = (struct V4LEX__TknCtrlBlk *)v4mm_AllocChunk(sizeof *tcb,FALSE) ; v4lex_InitTCB(tcb,0) ; } ;
		      { UCCHAR t, q ; UCCHAR *wantQuote = UClit("") ;
			t = *(++arg) ;
			if (t >= UClit('A') && t <= UClit('Z')) { t -= UClit('A') ; }
			 else if (t >= UClit('a') && t <= UClit('z')) { t -= UClit('a') ; }
			 else { vout_UCText(VOUT_Err,0,UClit("*Invalid -rp parameter- must be -rpX where X is letter of alphabet\n")) ; exit(EXITABORT) ; } ;
/*			Look for a 'q' or 'Q' after letter of alphabet */
			q = *(++arg) ;
			switch (q)
			 { default:		vout_UCText(VOUT_Err,0,UClit("*Invalid -rp suffix- must be -rpXq or -rpXQ\n")) ; exit(EXITABORT) ;
			   case UCEOS:		break ;
			   case UClit('q'):	wantQuote = UClit("'") ; break ;
			   case UClit('Q'):	wantQuote = UClit("\"") ; break ;
			 } ;
			NA num = UCstrtol(arg,&resptr,10) ;
			if (UCstrlen(arg) >= V4LEX_Tkn_ParamMax - (q == UCEOS ? 0 : 2)) { vout_UCText(VOUT_Err,0,UClit("*Invalid -rp Value is too long\n")) ; exit(EXITABORT) ; } ;
			if (*resptr == UCEOS) { tcb->poundparam[t].numvalue = num ; }	/* Got numeric value */
			 else { if (*wantQuote == UCEOS) { UCstrcpy(tcb->poundparam[t].value,arg) ; }
				 else { v_StringLit(arg,tcb->poundparam[t].value,V4LEX_Tkn_ParamMax,*wantQuote,UCEOS) ; } ;
//			        UCstrcpy(tcb->poundparam[t].value,wantQuote) ;		/* Got alpha value */
//				UCstrcat(tcb->poundparam[t].value,arg) ;p
//				UCstrcat(tcb->poundparam[t].value,wantQuote) ;
			      } ;
			tcb->poundparam[t].SetAtStartup = TRUE ;			/* Remember set here! */
		      }
			break ;
		   case UClit('q'):		/* -rq Quiet/Silent mode */
			silent = TRUE ; break ;
		   case UClit('s'):		/* -rs sectionname */
			NA ; UCstrcpy(sections,arg) ;
//			for(t=0;sections[t]!=0;t++) { sections[t] = toupper(sections[t]) ; } ;
			break ;
		 } ;
		break ;
	      case UClit('S'):
		traceGlobal |= V4TRACE_TimeStamp ; break ;
//	      case UClit('S'):
//		gpi->doSummaryAtExit = TRUE ;
//		break ;
	      case UClit('s'):
		NA
		if ((bp1 = UCstrchr(arg,'=')) != NULL)
		 { *bp1 = 0 ; vlog_StoreSymValue(arg,bp1+1) ; break ;
		 } ;
/*		No UClit('=') - assume file name - read it for list of assignments */
//		ifp = fopen(v3_logical_decoder(arg,TRUE),"r") ;
//		if (ifp == NULL)
		if (!v_UCFileOpen(&iUCFile,arg,UCFile_Open_Read,TRUE,result,0))
		 { v_Msg(NULL,tbuf,"V4TInvSSyn",arg,result) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ;
//		   UCsprintf(tbuf,UCsizeof(tbuf),UClit("? Invalid -s syntax (%ls) - Missing equal sign and/or not a valid file\n"),arg) ; vout_UCText(VOUT_Err,0,tbuf) ;
//		   exit(EXITABORT) ;
		 } ; UCFile.wantEOL = FALSE ;
		for(i=1;;i++)
		 { 
//		   if (fgets(tbuf,sizeof tbuf,ifp) == NULL) break ;
		   if (v_UCReadLine(&iUCFile,UCRead_UC,tbuf,UCsizeof(tbuf),tbuf) < 0) break ;
		   if (tbuf[0] == UClit('!')) continue ; if (tbuf[0] == UClit('/')) continue ;
//		   num = UCstrlen(tbuf) ; if (tbuf[num-1] == '\n') tbuf[num-1] = 0 ;
		   bp2 = (UCstrncmp(tbuf,UClit("-s "),3) == 0 ? &tbuf[3] : tbuf) ;
		   bp1 = UCstrchr(bp2,'=') ;
		   if (bp1 == NULL) { v_Msg(ctx,tbuf,"@% Invalid '-s' syntax [%1U %2d]: %3U   ignoring...\n",arg,i,tbuf) ; vout_UCText(VOUT_Warn,0,tbuf) ; continue ; } ;
		   *bp1 = UCEOS ; vlog_StoreSymValue(bp2,bp1+1) ;
		 } ;
//		fclose(ifp) ;
		v_UCFileClose(&iUCFile) ;
		break ;
//	      case UClit('u'):
//		if (v_LoadUnicodeData(TRUE,result) == UNUSED)
//		 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("*Err creating Unicode binary: %ls\n"),result) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ;
//		 } ;
//		exit(EXITOK) ;
	      case UClit('v'):
		switch(*(++arg))
		 { default:	vout_UCText(VOUT_Err,0,UClit("*Invalid -v (expecting -v or -vf)\n")) ; exit(EXITABORT) ;
		   case 0:	break ;
		   case UClit('f'):	verifyforeign = TRUE ;  break ;
		 } ;
		NA ; UCstrcpy(includefile,arg) ; UCstrcpy(rebuildfile,UClit("nil")) ;
		break ;
	      case UClit('X'):	runcmdfile = TRUE ; break ;
	      case UClit('x'):	existsok = TRUE ; break ;
	      case UClit('y'):	NA if (UCstrlen(arg) < UCsizeof(inpprompt)) UCstrcpy(inpprompt,arg) ; break ;
	      case UClit('z'):
		switch(*(++arg))
		 { default:	vout_UCText(VOUT_Err,0,UClit("*Invalid -z (expecting -zb, zt or zx)\n")) ; exit(EXITABORT) ;
		   case UClit('b'):	NA UCstrcpy(includefile,arg) ; arg = UClit("") ; NA
				{ double cmpratio ;
				  if (!v_FileCompress(arg,includefile,0,FALSE,result,&cmpratio))
				   { v_Msg(ctx,tbuf,"@* FileCompress() error: %1U\n",result) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
				  v_Msg(ctx,tbuf,"@  %1U => %2U (%3d%%)\n",arg,includefile,(int)(cmpratio+0.5)) ;
				  vout_UCText(VOUT_Status,0,tbuf) ;
				}
				exit(EXITOK) ;
		   case UClit('t'):	NA UCstrcpy(includefile,arg) ; arg = UClit("") ; NA
	if (UCstrstr(arg,UClit(".v4c")) != NULL || UCstrstr(arg,UClit(".V4C")) != NULL)
	 { UCCHAR tfile[V_FileName_Max] ;
	   vout_UCText(VOUT_Warn,0,UClit("*********************************************************************************\n")) ;
	   vout_UCText(VOUT_Warn,0,UClit("Files appear reversed - fixing for you!!!\n")) ;
	   vout_UCText(VOUT_Warn,0,UClit("*********************************************************************************\n")) ;
	   UCstrcpy(tfile,includefile) ; UCstrcpy(includefile,arg) ; UCstrcpy(arg,tfile) ;
	 } ;
				{ double cmpratio ;
				  if (!v_FileCompress(arg,includefile,0,TRUE,result,&cmpratio)) { v_Msg(ctx,tbuf,"@* FileCompress() error: %1U\n",result) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
				  v_Msg(ctx,tbuf,"@  %1U => %2U (%3d%%)\n",arg,includefile,(int)(cmpratio+0.5)) ;
				  vout_UCText(VOUT_Status,0,tbuf) ;
				}
				exit(EXITOK) ;
		   case UClit('x'):	NA UCstrcpy(includefile,arg) ; arg = UClit("") ; NA
				if (!v_FileExpand(arg,includefile,result)) { v_Msg(ctx,tbuf,"@* FileExpand() error: %1U\n",result) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
				exit(EXITOK) ;
		 } ;
		NA ; UCstrcpy(includefile,arg) ; NA 
		break ;
	      case UClit('%'):
		NA ; NUM(fillpercent) ; break ;
	    } ;
	 } ;
	if (isatty(0) == 0) silent = -1 ;		/* Force into silent mode if not going to a terminal */
//	UCsprintf(tbuf,UCsizeof(tbuf),UClit("V4 %d.%d - \"Data that thinks like us\"\nProtected by U.S. Patent 6,470,490 - MKS Inc. (2002)\n"),V4IS_MajorVersion,V4IS_MinorVersion) ;
	if (!silent)
	 { v_Msg(NULL,tbuf,"V4TBanner",V4IS_MajorVersion,V4IS_MinorVersion,gpi->rtOptString) ;vout_UCText(VOUT_Status,0,tbuf) ;
#ifdef V4_BUILD_LMTFUNC_EXP_DATE
	v_Msg(NULL,tbuf,"V4TWarnExpDate",mscu_udate_to_ddmmmyy(V4_BUILD_LMTFUNC_EXP_DATE)) ; vout_UCText(VOUT_Warn,0,tbuf) ;
#endif
	 } ;
	if (bucketnum >= 0)			/* Wants to dump out a bucket ? */
//	 { if (UCstrlen(includefile) == 0)
	 { if (UCempty(includefile))
	    { vout_UCText(VOUT_Err,0,UClit("*Must specify source (input) file with -d option\n")) ; exit(EXITABORT) ; } ;
	   memset(&pcb,0,sizeof pcb) ; strcpy(pcb.V3name,"testu") ;
	   pcb.OpenMode = V4IS_PCB_OM_Read ; pcb.DfltGetMode = V4IS_PCB_GP_Next ;
	   UCstrcpy(pcb.UCFileName,includefile) ; v4is_Open(&pcb,NULL,NULL) ;
	   if (bucketnum > 1100000000)
	    { v4is_SearchForLink(bucketnum-1100000000,pcb.AreaId) ; exit(EXITOK) ; } ;
	   if (bucketnum > 1000000000)
	    { switch (bucketnum - 1000000000)
	       { case 1:	v4is_ExamineBkts(V4_BktHdrType_Index,pcb.AreaId) ; break ;
	         case 2:	v4is_ExamineBkts(V4_BktHdrType_Data,pcb.AreaId) ; break ;
	         case 3:	v4is_ExamineBkts(0,pcb.AreaId) ; break ;
	       } ;
	    } else { v4is_ExamineBkt(bucketnum,pcb.AreaId) ; } ;
	   exit(EXITOK) ;
	 } ;
	if ((UCstrlen(sequentialfile) > 0 || listutil) && UCstrlen(includefile)>0)
	 {
	   if (UCstrlen(mergefile) > 0)
	    { v_Msg(ctx,tbuf,"@? Cannot merge into file (%1U) with sequential file (%2U)\n",mergefile,sequentialfile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	   if (UCstrlen(rebuildfile) > 0)
	    { v_Msg(ctx,tbuf,"@? Cannot write sequential file (%1U) with output file (%2U)\n",sequentialfile,rebuildfile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
//	   ifp = fopen(v3_logical_decoder(includefile,TRUE),"rb") ;
//	   if (ifp == NULL) { UCsprintf(tbuf,UCsizeof(tbuf),UClit("*Error (%ls) accessing file %ls\n"),v_OSErrString(errno),includefile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	   if (!v_UCFileOpen(&iUCFile,includefile,UCFile_Open_ReadBin,TRUE,tbuf,0))
	    { v_Msg(NULL,tbuf,"V4TOpnInErr",includefile,tbuf) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	   fread(&ibh,sizeof ibh,1,iUCFile.fp) ; bad = FALSE ;
	   if (ibh.sbh.BktNum != 0)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("Bucket Header indicates BOF bucket is %d (s/b 0)\n"),ibh.sbh.BktNum) ; vout_UCText(VOUT_Status,0,tbuf) ;
	      bad = TRUE ;
	    } ;
	   if (ibh.sbh.BktType != V4_BktHdrType_Root)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("Bucket Header indicates type %d bucket (s/b %d for ROOT)\n"),ibh.sbh.BktType,V4_BktHdrType_Root) ;
	      vout_UCText(VOUT_Status,0,tbuf) ; bad = TRUE ;
	    } ;
	   if (ibh.ParentBktNum != 0)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("Bucket Header indicates Parent of %d (s/b 0 for ROOT)\n"),ibh.ParentBktNum) ; vout_UCText(VOUT_Status,0,tbuf) ; bad = TRUE ; } ;
	   fread(&root,sizeof root,1,iUCFile.fp) ;		/* Try to read root info */
	   if (bad && bucketcnt == 0) { vout_UCText(VOUT_Err,0,UClit("*Must specify bucket count (-bc n) with invalid header\n")) ; exit(EXITABORT) ; } ;
	   if (bad && bucketsize == 0) { vout_UCText(VOUT_Err,0,UClit("*Must specify bucket size (-bs n) with invalid header\n")) ; exit(EXITABORT) ; } ;
	   if (bucketcnt == 0) bucketcnt = root.NextAvailBkt ;
	   if (bucketsize == 0) bucketsize = root.BktSize ;
	   if (UCstrlen(sequentialfile) > 0)
	    { if (!existsok)
	       { 
//	         ofp = fopen(v3_logical_decoder(sequentialfile,TRUE),"rb") ;
//		 if (ofp != NULL)
		 if (v_UCFileOpen(&oUCFile,sequentialfile,UCFile_Open_ReadBin,TRUE,tbuf,0))
		  { 
//		    fclose(ofp) ; UCsprintf(tbuf,UCsizeof(tbuf),UClit("? Output file (%s) already exists, use -x to overwrite\n"),sequentialfile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ;
		    v_UCFileClose(&oUCFile) ; v_Msg(NULL,tbuf,"V4TOutExist",sequentialfile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ;
		  } ;
	       } ;
//	      ofp = fopen(v3_logical_decoder(sequentialfile,FALSE),"wb") ;
//	      if (ofp == NULL) { UCsprintf(tbuf,UCsizeof(tbuf),UClit("*Error (%s) creating output sequential file (%s)\n"),v_OSErrString(errno),sequentialfile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	      if (!v_UCFileOpen(&oUCFile,sequentialfile,UCFile_Open_WriteBin,TRUE,tbuf,0))
	       { v_Msg(NULL,tbuf,"V4TCreOutErr",sequentialfile,tbuf) ; exit(EXITABORT) ; } ;
	    } else oUCFile.fp = NULL ;
	   if (outseqhdr == -1) outseqhdr = TRUE ; if (outseqdata == -1) outseqdata = TRUE ;
	   if (oUCFile.fp != NULL)
	    { num = v4is_DumpArea(iUCFile.fp,oUCFile.fp,bucketcnt,bucketsize,maxreccnt,silent,outseqhdr,outseqdata,0) ;
	      if (silent != TRUE) { UCsprintf(tbuf,UCsizeof(tbuf),UClit("Copied %d data records to output file\n"),num) ; vout_UCText(VOUT_Status,0,tbuf) ; } ;
	    } else
	    { v_Msg(ctx,tbuf,"@Space utilization for %1U (%2U)-\n",v_UCLogicalDecoder(includefile,VLOGDECODE_Exists,0,tbuf),includefile) ;
	      vout_UCText(VOUT_Status,0,tbuf) ;
	      v4is_DumpArea(iUCFile.fp,oUCFile.fp,bucketcnt,bucketsize,maxreccnt,silent,FALSE,FALSE,listutil) ;
	    } ;
//	   fclose(ifp) ; if (ofp != NULL) fclose(ofp) ;
	   v_UCFileClose(&iUCFile) ; if (oUCFile.fp != NULL) v_UCFileClose(&oUCFile) ;
	   exit(EXITOK) ;
	 } ;
	if (listing && !listutil)
//	 { if (UCstrlen(includefile) == 0)
	 { if (UCempty(includefile))
	    { vout_UCText(VOUT_Err,0,UClit("*Must specify source (input) file with -l option\n")) ; exit(EXITABORT) ; } ;
//	   ifp = fopen(v3_logical_decoder(includefile,TRUE),"rb") ;
//	   if (ifp == NULL) { UCsprintf(tbuf,UCsizeof(tbuf),UClit("*Error (%s) accessing file %s\n"),v_OSErrString(errno),includefile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	   if (!v_UCFileOpen(&iUCFile,includefile,UCFile_Open_ReadBin,TRUE,tbuf,0))
	    { v_Msg(NULL,tbuf,"V4TOpnInErr",includefile,tbuf) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	   fread(&ibh,sizeof ibh,1,iUCFile.fp) ; bad = FALSE ;
	   if (ibh.sbh.BktNum != 0)
	    { UCsprintf(tbuf,200,UClit("Bucket Header indicates BOF bucket is %d (s/b 0)\n"),ibh.sbh.BktNum) ; vout_UCText(VOUT_Status,0,tbuf) ; bad = TRUE ; } ;
	   if (ibh.sbh.BktType != V4_BktHdrType_Root)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("Bucket Header indicates type %d bucket (s/b %d for ROOT)\n"),ibh.sbh.BktType,V4_BktHdrType_Root) ;
	      vout_UCText(VOUT_Status,0,tbuf) ; bad = TRUE ;
	    } ;
	   if (ibh.ParentBktNum != 0)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("Bucket Header indicates Parent of %d (s/b 0 for ROOT)\n"),ibh.ParentBktNum) ; vout_UCText(VOUT_Status,0,tbuf) ; bad = TRUE ; } ;
	   if (bad) { vout_UCText(VOUT_Status,0,UClit("*** Following Information Probably NOT Valid ***\n")) ; } ;
	   fread(&root,sizeof root,1,iUCFile.fp) ;	/* Try to read root info */
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Bucket Size %d\n"),root.BktSize) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Version %d.%d\n"),root.Version/1000,root.Version%1000) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Next Available Bucket %d (Number of buckets = %d)\n"),root.NextAvailBkt,root.NextAvailBkt) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Compress Records over %d Bytes\n"),root.MinCmpBytes) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Maximum Record Length %d\n"),root.MaxRecordLen) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Data Storage Mode %d"),root.DataMode) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Number of levels in directory %d\n"),root.NumLevels) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Fill Percentage on Rebuild %d\n"),root.FillPercent) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Allocation Increment %d\n"),root.AllocationIncrement) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Record Count %d\n"),root.RecordCount) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Global Buffer Count %d\n"),root.GlobalBufCount) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Lock Max %d\n"),root.LockMax) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   v_Msg(ctx,tbuf,"@  Created %1U",UCctime((time_t *)&root.CreateTime)) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   if (root.NextAvailUserNum != 0)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Next Available User Number %d\n"),root.NextAvailUserNum) ; vout_UCText(VOUT_Status,0,tbuf) ; } ;
	   if (root.NextUnusedBkt != 0)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Next Unused Bucket %d\n"),root.NextUnusedBkt) ; vout_UCText(VOUT_Status,0,tbuf) ; } ;
	   if (root.FFIKeyInfoByte != 0)
	    { fseek(iUCFile.fp,(FSEEKINT)root.FFIKeyInfoByte,0) ; fread(&ki,sizeof ki,1,iUCFile.fp) ;
	      for((kid=(struct V4FFI__KeyInfoDtl *)&ki.Buffer,i=0);i<ki.Count;(i++,kid=(struct V4FFI__KeyInfoDtl *)(kid+kid->Bytes)))
	       { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Foreign File Keys for FileRef %d\n"),kid->FileRef) ; vout_UCText(VOUT_Status,0,tbuf) ;
		 for(t=0;t<kid->KeyCount;t++)
		  { v_Msg(ctx,tbuf,"@    %1d. Offset %2d, Prefix %3x (Bytes %4d, Mode %5d, Type %6d), DupsOK %7U\n",t+1,
			kid->Key[t].Offset,kid->Key[t].KeyPrefix.all,kid->Key[t].KeyPrefix.fld.Bytes,
			kid->Key[t].KeyPrefix.fld.KeyMode,kid->Key[t].KeyPrefix.fld.KeyType,(kid->Key[t].DupsOK ? UClit("Yes") : UClit("No"))) ;
		    vout_UCText(VOUT_Status,0,tbuf) ;
		  } ;
	       } ;
	    } ;
	   if (root.FreeDataInfoByte != 0)
	    { fseek(iUCFile.fp,(FSEEKINT)root.FreeDataInfoByte,0) ; fread(&fdi,sizeof fdi,1,iUCFile.fp) ;
	      pstr[0] = 0 ;
	      for(i=0;i<fdi.count;i++)
	       { UCsprintf(result,UCsizeof(result),UClit("B%d:%d "),fdi.Entry[i].DataBktNum,fdi.Entry[i].FreeBytes) ; UCstrcat(pstr,result) ; } ;
	      v_Msg(ctx,tbuf,"@  Free Space: %1U\n",pstr) ; vout_UCText(VOUT_Status,0,tbuf) ;
	    } ;
	  if (root.IndexOnlyInfoByte != 0)
	   { fseek(iUCFile.fp,(FSEEKINT)root.IndexOnlyInfoByte,0) ; fread(&io,sizeof io,1,iUCFile.fp) ;
	     for(i=0;i<io.count;i++)
	      { v_Msg(ctx,tbuf,"@  Index-Only %1d - %2d %3U\n",i+1,io.Entry[i].BktOffset,io.Entry[i].AreaName) ; vout_UCText(VOUT_Status,0,tbuf) ; } ;
	   } ;
//	  if (UCstrlen(outfile) == 0 && UCstrlen(mergefile) == 0 && UCstrlen(rebuildfile) == 0 && UCstrlen(sequentialfile) == 0)
	  if (UCempty(outfile) && UCempty(mergefile) && UCempty(rebuildfile) && UCempty(sequentialfile))
	   { exit(EXITOK) ; } ;
	 } ;
	if (UCstrlen(copyfile) > 0 && UCstrlen(includefile) > 0)
	 {
//	   ifp = fopen(v3_logical_decoder(includefile,TRUE),"rb") ;
//	   if (ifp == NULL) { UCsprintf(tbuf,UCsizeof(tbuf),UClit("*Error (%s) accessing file %ls\n"),v_OSErrString(errno),includefile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	   if (!v_UCFileOpen(&iUCFile,includefile,UCFile_Open_ReadBin,TRUE,tbuf,0))
	    { v_Msg(NULL,tbuf,"V4TOpnInErr",includefile,tbuf) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	   fread(&ibh,sizeof ibh,1,iUCFile.fp) ;
	   if (ibh.sbh.BktNum != 0)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("Bucket Header indicates BOF bucket is %d (s/b 0)\n"),ibh.sbh.BktNum) ; vout_UCText(VOUT_Status,0,tbuf) ; v_UCFileClose(&iUCFile) ; exit(EXITABORT) ; } ;
	   if (ibh.sbh.BktType != V4_BktHdrType_Root)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("Bucket Header indicates type %d bucket (s/b %d for ROOT)\n"),ibh.sbh.BktType,V4_BktHdrType_Root) ;
	      vout_UCText(VOUT_Status,0,tbuf) ; v_UCFileClose(&iUCFile) ; exit(EXITABORT) ;
	    } ;
	   if (ibh.ParentBktNum != 0)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("Bucket Header indicates Parent of %d (s/b 0 for ROOT)\n"),ibh.ParentBktNum) ; vout_UCText(VOUT_Status,0,tbuf) ; v_UCFileClose(&iUCFile) ; exit(EXITABORT) ; } ;
	   fread(&root,sizeof root,1,iUCFile.fp) ;	/* Try to read root info */
	   bucketsize = root.BktSize ;
	   bp1 = v_UCLogicalDecoder(copyfile,VLOGDECODE_Exists,0,tbuf) ;
	   UCstrcpy(outfile,bp1) ;
//	   ofp = fopen(outfile,"rb+") ; fseek(ofp,(FSEEKINT)0,0) ;
//	   if (ofp == NULL) { UCsprintf(tbuf,UCsizeof(tbuf),UClit("*Error (%s) accessing file %s\n"),v_OSErrString(errno),outfile) ; vout_UCText(VOUT_Err,0,tbuf) ; v_UCFileClose(&iUCFile) ; exit(EXITABORT) ; } ;
	   if (!v_UCFileOpen(&oUCFile,outfile,UCFile_Open_UpdateBin,FALSE,tbuf,0))
	    { v_Msg(NULL,tbuf,"V4TOpnUpdErr",outfile,tbuf) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	   memset(&ibh,0,sizeof(ibh)) ;
	   if ((num = fread(&ibh,sizeof ibh,1,oUCFile.fp)) != 1)
	    { v_Msg(ctx,tbuf,"@*Error (%1U/%2d) reading ibh info for %3U\n",v_OSErrString(errno),num,outfile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	   if (ibh.sbh.BktType != V4_BktHdrType_Root)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("Bucket Header indicates type %d bucket (s/b %d for ROOT)\n"),ibh.sbh.BktType,V4_BktHdrType_Root) ;
	      vout_UCText(VOUT_Status,0,tbuf) ; v_UCFileClose(&iUCFile) ; v_UCFileClose(&oUCFile) ; exit(EXITABORT) ;
	    } ;
	   if (ibh.ParentBktNum != 0)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("Bucket Header indicates Parent of %d (s/b 0 for ROOT)\n"),ibh.ParentBktNum) ; vout_UCText(VOUT_Status,0,tbuf) ; v_UCFileClose(&iUCFile) ; v_UCFileClose(&oUCFile) ; exit(EXITABORT) ; } ;
	   memset(&root,0,sizeof(root)) ;
	   if ((num = fread(&root,sizeof root,1,oUCFile.fp)) != 1)
	    { v_Msg(ctx,tbuf,"@*Error (%1U/%2d) reading root info for %3U\n",v_OSErrString(errno),num,copyfile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	   if (bucketsize != root.BktSize)
	    { v_Msg(ctx,tbuf,"@? Cannot copy, bucket sizes do not match: %1U (%2d) <= %3U (%4d)\n",outfile,root.BktSize,includefile,bucketsize) ;
	      vout_UCText(VOUT_Status,0,tbuf) ; v_UCFileClose(&iUCFile) ; v_UCFileClose(&oUCFile) ;
	      exit(EXITABORT) ;
	    } ;
	   if (!existsok) goto err_anc ;
	   fseek(iUCFile.fp,(FSEEKINT)0,0) ; fseek(oUCFile.fp,(FSEEKINT)0,0) ;		/* Reposition to BOF */
	   bktp = (BYTE *)v4mm_AllocChunk(bucketsize,FALSE) ;
	   for(i=0;;i++)
	    { num = fread(bktp,bucketsize,1,iUCFile.fp) ;
	      if (num == 0 && errno == 0) break ;
	      if (num != 1)
	       { v_Msg(ctx,tbuf,"@*Error (%1x/%2d) reading bucket (%3d) from %4U - You be in BIG trouble!\n",v_OSErrString(errno),num,i,includefile) ; vout_UCText(VOUT_Err,0,tbuf) ; break ; } ;
	      if (fwrite(bktp,bucketsize,1,oUCFile.fp) != 1)
	       { v_Msg(ctx,tbuf,"@*Error (%1U) writing bucket (%2d) into %3U - You be in BIG trouble!\n",v_OSErrString(errno),i,outfile) ; vout_UCText(VOUT_Err,0,tbuf) ; break ; } ;
	    } ;
	   v_Msg(ctx,tbuf,"@Copied %1d buckets: %2U => %3U\n",i,includefile,outfile) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   v_Msg(ctx,tbuf,"@ Note: be sure to use -pR to reset & reopen area (%1U)\n",outfile) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   v_UCFileClose(&iUCFile) ; v_UCFileClose(&oUCFile) ;
#ifdef UNIX
	   truncate(outfile,(i-1)*bucketsize) ;			/* Truncate file to new size */
#endif
	   exit(EXITOK) ;
	 } ;
//	if (UCstrlen(rekeyfile) > 0 && UCstrlen(includefile) == 0 && UCstrlen(rebuildfile) == 0 && UCstrlen(sequentialfile) == 0)
	if (UCnotempty(rekeyfile) && UCempty(includefile) && UCempty(rebuildfile) && UCempty(sequentialfile))
	 { memset(&pcb,0,sizeof pcb) ; strcpy(pcb.V3name,"testu") ;
	   pcb.OpenMode = V4IS_PCB_OM_Update ; pcb.DfltGetMode = V4IS_PCB_GP_Next ;
	   UCstrcpy(pcb.UCFileName,rekeyfile) ; pcb.AreaFlags |= V4IS_PCB_OF_ForceOpen ;
	   v4is_Open(&pcb,NULL,NULL) ;
	   if (pcb.AreaId == V4IS_AreaId_SeqOnly)
	    v4_error(V4E_ISSEQONLY,0,"V4TEST","Reformat","ISSEQONLY","Cannot reformat SeqOnly files") ;
	   if (!existsok)
	    {  vout_UCText(VOUT_Err,0,UClit("*The -k switch must be used in conjunction with the -x switch\n")) ; exit(EXITABORT) ; } ;
	   v4is_RebuildAreaIndex(&pcb) ;
	   exit(EXITOK) ;
	 } ;
run_interpreter:
//	if (UCstrlen(includefile) >= 0 && UCstrlen(outfile) == 0 && UCstrlen(mergefile) == 0
//		&& UCstrlen(rebuildfile) == 0 && UCstrlen(sequentialfile) == 0)
	if (UCempty(outfile) && UCempty(mergefile) && UCempty(rebuildfile) && UCempty(sequentialfile))
	 { if (tcb == NULL)
	    { tcb = (struct V4LEX__TknCtrlBlk *)v4mm_AllocChunk(sizeof *tcb,FALSE) ; v4lex_InitTCB(tcb,0) ; tcb->maxCharInpBuf = V4LEX_Tkn_SrcFileLineMax ; } ;
	   UCstrcpy(tcb->ilvl[0].prompt,inpprompt) ;
	   ctx = (struct V4C__Context *)v4mm_AllocChunk(sizeof *ctx,TRUE) ;
	   v4ctx_Initialize(ctx,NULL,NULL) ;			/* Initialize context */
	   ctx->pi->argc = argc ; ctx->pi->ucargv = ucargv ; ctx->pi->envp = envp ;	/* Copy process startup info */
	   ctx->pi->v4argx = v4argx ;			/* Save argument index after v4 program */
	   ctx->pi->MainTcb = tcb ;
	   tcb->AutoExitEOF = runcmdfile ;		/* Set flag to auto-Exit and end of command file */
	   UCstrcpy(tcb->sections,sections) ;
	   tpo = NULL ; INITTPO ;
	   if (UCstrlen(includefile) > 0)
	    { 
//	      ifp = fopen(v3_logical_decoder(includefile,TRUE),"r") ;
//	      if (ifp == NULL)
//	       { strcat(includefile,".v4") ; ifp = fopen(v3_logical_decoder(includefile,TRUE),"r") ; } ;
//	      if (ifp == NULL)
//	       { strcat(includefile,"b") ; ifp = fopen(v3_logical_decoder(includefile,TRUE),"r") ; } ;
//	      if (ifp == NULL) { UCsprintf(tbuf,UCsizeof(tbuf),UClit("*Error (%s) accessing include file (%s)\n",v_OSErrString(errno),includefile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	      if (!v_UCFileOpen(&UCFile,includefile,UCFile_Open_Read,TRUE,ctx->ErrorMsgAux,0))
	       { UCstrcat(includefile,UClit(".v4")) ;
	         if (!v_UCFileOpen(&UCFile,includefile,UCFile_Open_Read,TRUE,ctx->ErrorMsgAux,0))
	          { UCstrcat(includefile,UClit("b")) ;
	            if (!v_UCFileOpen(&UCFile,includefile,UCFile_Open_Read,TRUE,ctx->ErrorMsgAux,0))
	             { v_Msg(ctx,tbuf,"V4TOpnInErr",includefile,ctx->ErrorMsgAux) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	          } ;
	       } ;
	      v4lex_NestInput(tcb,&UCFile,includefile,V4LEX_InpMode_File) ; tcb->ilvl[tcb->ilx].tpo = tpo ;
	    } ;
	   if (UCstrlen(inifile) > 0)		/* Do we have a user specified ini file (-i) */
	    { 
//	      ifp = fopen(v3_logical_decoder(inifile,TRUE),"r") ;
//	      if (ifp == NULL)
//	       { strcat(inifile,".v4i") ; ifp = fopen(v3_logical_decoder(inifile,TRUE),"r") ; } ;
//	      if (ifp == NULL) { UCsprintf(tbuf,UCsizeof(tbuf),UClit("*Error (%s) accessing -i file (%s)\n",v_OSErrString(errno),inifile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	      if (!v_UCFileOpen(&UCFile,inifile,UCFile_Open_Read,TRUE,ctx->ErrorMsgAux,0))
	       { v_Msg(ctx,tbuf,"@*Error (%1O) accessing -i file (%2s)\n",errno,inifile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	      v4lex_NestInput(tcb,&UCFile,inifile,V4LEX_InpMode_File) ; tcb->ilvl[tcb->ilx].tpo = tpo ;
	    } ;				/* No user ini file - look for default- v4.ini */
	   tcb->ilvl[tcb->ilx].echo = echo ;
	   for(ok=TRUE;ok;)
	    { int res = v4eval_Eval(tcb,ctx,TRUE,traceGlobal,TRUE,FALSE,FALSE,NULL) ;
	      switch (res & V4EVAL_ResMask)
	       {
		 case V4EVAL_Res_ExitNoStats:		silent = TRUE ;		/* Stop */
		 case V4EVAL_Res_StopXct:					/* Stop */
		 case V4EVAL_Res_ExitStats:		ok = FALSE ; break ;	/* Stop */
		 case V4EVAL_Res_BPContinue:
		 case V4EVAL_Res_BPStep:
		 case V4EVAL_Res_BPStepNC:
		 case V4EVAL_Res_BPStepInto:		break ;			/* Just continue */
	       } ;
	    } ;
	   vout_CloseFile(UNUSED,UNUSED,NULL) ;			/* Close off all/any output stream files */
	   if (!silent)
	    { v4_ExitStats(ctx,tcb) ;
	    } ;

/*	   Set up new trapping environment (don't want to trap back into v4eval!) */
	   if (setjmp(ctx->pi->environment) != 0)
	    { vout_UCText(VOUT_Err,0,UClit("*Fatal errors in V4-EXIT routines - quitting\n")) ; signal(SIGSEGV,SIG_DFL) ; exit(EXITABORT) ; } ;
//	   if (gpi->doSummaryAtExit) v4im_DoSummaryOutput(ctx,NULL,FALSE,NULL) ;
	   v4mm_FreeResources(ctx->pi) ;
	   v4mm_MemoryExit() ;
	   exit(gpi->exitCode > 0 ? gpi->exitCode : (gpi->ErrCount > 0 ? EXITABORT : EXITOK)) ;
	 } ;
	if (UCstrlen(includefile) > 0 && (UCstrlen(outfile) > 0 || UCstrlen(mergefile) > 0))
	 {
	   for(bp1=includefile,sfl.Count=0;;)
	    { bp2 = UCstrchr(bp1,',') ;			/* Search for file,file,... */
	      indexonly = FALSE ;
	      if (bp2 != NULL) *bp2 = 0 ;
	      if (*bp1 == UClit('['))				/* Got: (file) - index-only */
	       { bp1++ ; bp3 = UCstrchr(bp1,']') ;
		 if (bp3 == NULL) { v_Msg(ctx,tbuf,"@? Missing closing bracket on index-only file %1U\n",bp1) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
		 *bp3 = 0 ; indexonly = TRUE ;
	       } ;
	      UCstrcpy(sfl.File[sfl.Count].Name,bp1) ;
//	      ifp = fopen(v3_logical_decoder(UCretASC(sfl.File[sfl.Count].Name),TRUE),"rb") ;
//	      if (ifp == NULL) { UCsprintf(tbuf,UCsizeof(tbuf),UClit("*Error (%s) accessing input file (%s)\n"),v_OSErrString(errno),sfl.File[sfl.Count].Name) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	      if (!v_UCFileOpen(&iUCFile,sfl.File[sfl.Count].Name,UCFile_Open_ReadBin,TRUE,tbuf,0))
	       { v_Msg(NULL,tbuf,"V4TOpnInErr",includefile,tbuf) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	      fread(&ibh,sizeof ibh,1,iUCFile.fp) ;
	      if (ibh.sbh.BktNum == 0 && ibh.sbh.BktType == V4_BktHdrType_Root && ibh.ParentBktNum == 0)
	       { fread(&root,sizeof root,1,iUCFile.fp) ;		/* Try to read root info */
		 sfl.File[sfl.Count].Type = (indexonly ? V4IS_SeqFileType_Index : V4IS_SeqFileType_V4IS) ;
		 sfl.File[sfl.Count].BktSize = root.BktSize ; sfl.File[sfl.Count].BktCount = root.NextAvailBkt ;
	       } else sfl.File[sfl.Count].Type = V4IS_SeqFileType_Seq ;
	      v_UCFileClose(&iUCFile) ; sfl.Count++ ;
	      if (bp2 == NULL) { break ; } else { bp1 = bp2+1 ; } ;
	    } ;
	   if (UCstrlen(outfile) > 0 && UCstrlen(mergefile) > 0)
	    { 
//	      UCsprintf(tbuf,UCsizeof(tbuf),UClit("? Cannot specify both output file (%s) and merge file (%s)\n"),outfile,mergefile) ;
	      v_Msg(NULL,tbuf,"V4TOutMrge",outfile,mergefile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ;
	    } ;
	   if (UCstrlen(outfile) > 0)
	    { 
//	      ofp = fopen(v3_logical_decoder(outfile,FALSE),"rb") ;
//	      if (ofp != NULL)
	      if (v_UCFileOpen(&oUCFile,outfile,UCFile_Open_ReadBin,TRUE,tbuf,0))
	       { v_UCFileClose(&oUCFile) ;
		 if (existsok != TRUE)
		  { 
//		    UCsprintf(tbuf,UCsizeof(tbuf),UClit("? File (%s) already exists, use -om to merge or -x to inhibit this error\n"),outfile) ; vout_UCText(VOUT_Err,0,tbuf) ;
		    v_Msg(NULL,tbuf,"V4TOutExist2",outfile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ;
		  } ;
	       } ;
	    } ;
	   if (UCstrlen(mergefile) > 0)
	    { 
//	      ofp = fopen(v3_logical_decoder(mergefile,TRUE),"r") ;
//	      if (ofp == NULL)
	      if (!v_UCFileOpen(&oUCFile,mergefile,UCFile_Open_ReadBin,TRUE,tbuf,0))
	       { 
//	         UCsprintf(tbuf,UCsizeof(tbuf),UClit("*Error (%s) access mergefile (%s)\n"),v_OSErrString(errno),mergefile) ; vout_UCText(VOUT_Err,0,tbuf) ;
	         v_Msg(NULL,tbuf,"V4TOpnInErr",mergefile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ;
	       } ; v_UCFileClose(&oUCFile) ;
	    } ;
	   sfl.BktOffset = (bucketoffset == 0 ? V4IS_SeqFile_BktOffset : bucketoffset) ;
	   num = v4is_RestoreArea(&sfl,(UCstrlen(outfile) > 0 ? outfile : mergefile),(UCstrlen(outfile)>0),fileref,silent,bucketsize,fillpercent,fastrebuild,recordsize) ;
	   if (silent != TRUE)
	    { 
//	      UCsprintf(tbuf,UCsizeof(tbuf),UClit("Copied %d data records to output file\n"),num) ;
	      v_Msg(NULL,tbuf,"*V4TCopyOK",num) ;  vout_UCText(VOUT_Status,0,tbuf) ;
	    } ;
//	   v_UCFileClose(&iUCFile) ;
	   exit(EXITOK) ;
	 } ;
	if (UCstrlen(includefile) > 0 && UCstrlen(rebuildfile) > 0)
	 { if (UCstrlen(mergefile) > 0)
	    { v_Msg(ctx,tbuf,"@? Cannot merge into file (%1U) with output file (%2U)\n",mergefile,rebuildfile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	   if (UCstrlen(sequentialfile) > 0)
	    { v_Msg(ctx,tbuf,"@? Cannot generate sequential file (%1U) & output file (%2U)\n",sequentialfile,rebuildfile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ; } ;
	   memset(&pcb,0,sizeof pcb) ; strcpy(pcb.V3name,"testu") ;
	   pcb.OpenMode = V4IS_PCB_OM_Read ; pcb.DfltGetMode = V4IS_PCB_GP_Next ;
	   UCstrcpy(pcb.UCFileName,includefile) ;
	   v4is_Open(&pcb,NULL,NULL) ;
	   if (pcb.AreaId == V4IS_AreaId_SeqOnly)
	    v4_error(V4E_ISSEQONLY,0,"V4TEST","Reformat","ISSEQONLY","Cannot reformat SeqOnly files") ;
	   dpcb = (struct V4IS__ParControlBlk *)v4mm_AllocChunk(sizeof *dpcb,TRUE) ;
	   if (!existsok)
	    { 
//	      ofp = fopen(v3_logical_decoder(rebuildfile,TRUE),"rb") ;
//	      if (ofp != NULL)
	      if (v_UCFileOpen(&oUCFile,rebuildfile,UCFile_Open_ReadBin,TRUE,tbuf,0))
	       { v_UCFileClose(&oUCFile) ;
//	         UCsprintf(tbuf,UCsizeof(tbuf),UClit("? Output file (%s) already exists, use -x to overwrite\n"),rebuildfile) ;
	         v_Msg(NULL,tbuf,"V4TOutExist",rebuildfile) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ;
	       } ;
	    } ;
	   strcpy(dpcb->V3name,"dstu") ; UCstrcpy(dpcb->UCFileName,rebuildfile) ;
	   dpcb->AccessMode = -1 ; dpcb->OpenMode = V4IS_PCB_OM_NewAppend ; dpcb->DfltPutMode = V4IS_PCB_GP_Append ;
	   acb = (struct V4IS__AreaCB *)v4mm_ACBPtr(pcb.AreaId) ;
	   dpcb->BktSize = (bucketsize > 0 ? bucketsize : acb->RootInfo->BktSize) ;
	   if (recordsize > 0) dpcb->MaxRecordLen = recordsize ;
	   v4is_Open(dpcb,NULL,NULL) ;
	   dacb = (struct V4IS__AreaCB *)v4mm_ACBPtr(dpcb->AreaId) ;
	   dacb->RootInfo->FillPercent = (fillpercent != 0 ? fillpercent : acb->RootInfo->FillPercent) ;
	   if (lockmax != 0) dacb->RootInfo->LockMax = lockmax ;
	   if (globalbufs != 0) dacb->RootInfo->GlobalBufCount = globalbufs ;
	   if (silent == -1) { v_Msg(ctx,tbuf,"@Reformatting %1U => %2U\n",pcb.UCFileName,dpcb->UCFileName) ; vout_UCText(VOUT_Status,0,tbuf) ; } ;
	   time(&time_of_day) ;
	   i = v4is_Reformat(&pcb,dpcb,&refr,verifyforeign,(silent ? V4IS_Reformat_Silent : 0)) ;
	   v4is_Close(&pcb) ; v4is_Close(dpcb) ;
	   if (UCstrcmp(rebuildfile,UClit("nil")) == 0)			/* Verifying area ? */
	    { 
//	      UCsprintf(tbuf,UCsizeof(tbuf),UClit("%s: Verified %d records, %d keys, %d BktReads, %d elapsed seconds\n"),
//			pcb.FileName,refr.DataIdCnt,refr.KeyCnt,refr.BktReads,time(NULL)-time_of_day) ; vout_UCText(VOUT_Status,0,tbuf) ;
	      v_Msg(NULL,tbuf,"V4TVerOK",includefile,refr.DataIdCnt,refr.KeyCnt,refr.BktReads,time(NULL)-time_of_day) ; vout_UCText(VOUT_Status,0,tbuf) ;
	      if (refr.ObsoleteCnt > 0)
	       { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  ... %d records marked obsolete\n"),refr.ObsoleteCnt) ; vout_UCText(VOUT_Status,0,tbuf) ; } ;
	      if (refr.MaxLevels >= V4IS_AreaCB_LvlMax-4)
	       { 
//	         UCsprintf(tbuf,UCsizeof(tbuf),UClit("?  but approaching maximum number of nested index buckets- should reformat soon!\n")) ;
		 v_Msg(NULL,tbuf,"V4TVerNearMax") ; vout_UCText(VOUT_Warn,0,tbuf) ; exit(EXITABORT) ;
	       } ;
	    } else
	    { if (silent != TRUE)
	       { v_Msg(NULL,tbuf,"*V4TCopyOK2",refr.DataIdCnt,refr.KeyCnt,refr.BktReads,refr.BktWrites,time(NULL)-time_of_day) ; vout_UCText(VOUT_Status,0,tbuf) ;
//	         UCsprintf(tbuf,UCsizeof(tbuf),UClit("Copied %d records, %d keys, %d+%d BktReads+Writes, %d elapsed seconds\n"),
//			refr.DataIdCnt,refr.KeyCnt,refr.BktReads,refr.BktWrites,time(NULL)-time_of_day) ; vout_UCText(VOUT_Status,0,tbuf) ;
		 if (refr.ObsoleteCnt > 0)
		  { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  ... trashed %d obsolete records\n"),refr.ObsoleteCnt) ; vout_UCText(VOUT_Status,0,tbuf) ; } ;
	       } ;
	    } ;
	   exit(EXITOK) ;
	 } ;


err_eol: v_Msg(NULL,tbuf,"*V4TUnexpEOL") ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ;

err_av:	v_Msg(NULL,tbuf,"*V4TUnexpSwtch",arg) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ;

err_ia:	v_Msg(NULL,tbuf,"*V4TInvIntArg",arg) ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ;

err_anc: v_Msg(NULL,tbuf,"*V4TActNotCmplt") ; vout_UCText(VOUT_Err,0,tbuf) ; exit(EXITABORT) ;
}


void v4_ExitStats(ctx,tcb)
  struct V4C__Context *ctx ;
  struct V4LEX__TknCtrlBlk *tcb ;
{
  time_t time_of_day ; double wallseconds ;
  double cpusecondsU=0,cpusecondsK=0 ;
#ifdef UNIX
  struct timeval endTime ;
#endif
  UCCHAR tbuf[512] ;

	time(&time_of_day) ;
	if (TRUE)
	 {
	 } else
	 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("V4 Processing Summary - %.19s\n"),ASCretUC(ctime(&time_of_day))) ; vout_UCText(VOUT_Status,0,tbuf) ; } ;
	wallseconds = v_ConnectTime() ;
#ifdef WINNT
	if (GetProcessTimes(GetCurrentProcess(),&ctx->pi->ftCreate,&ctx->pi->ftExit,&ftKernel2,&ftUser2))	/* Only do if call implemented (NT) */
	 { B64INT t1,t2 ; memcpy(&t1,&ctx->pi->ftUser1,sizeof t1) ; memcpy(&t2,&ftUser2,sizeof t2) ;
	   cpusecondsU = (t2 - t1) / 10000000.0 ;
	   memcpy(&t1,&ctx->pi->ftKernel1,sizeof t1) ; memcpy(&t2,&ftKernel2,sizeof t2) ;
	   cpusecondsK = (t2 - t1) / 10000000.0 ;
	 } ;
#else
	cpusecondsU = v_CPUTime() ; cpusecondsK = 0.0 ;
#endif
	if (TRUE)
	 { UCCHAR errBuf[32] ;
	   if (ctx->pi->ErrCount == 0) { ZUS(errBuf) }
	    else { UCsprintf(errBuf,UCsizeof(errBuf),UClit("Errs:%d, "),ctx->pi->ErrCount) ; } ;
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("%s- %.19s, %sWall:%.02f, CPU:%.03f, Lines:%d, Tokens:%d, Evals:%d, Ctx:%d, CGet:%d\n"),
			(ctx->pi->ErrCount > 0 ? UClit("V4ExitErr") : UClit("V4Exit")),ASCretUC(ctime(&time_of_day)),errBuf,wallseconds,cpusecondsU+cpusecondsK,tcb->tcb_lines,tcb->tcb_tokens,ctx->IsctEvalCount,ctx->ContextAddCount,ctx->ContextAddGetCount) ;
	   vout_UCText(VOUT_Status,0,tbuf) ;
	 } else
	 {
#ifdef WINNT
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Time: %g elapsed seconds, %g CPU seconds (%g user + %g kernel)\n"),wallseconds,(cpusecondsU+cpusecondsK),cpusecondsU,cpusecondsK) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   cpusecondsU += cpusecondsK ;
#else
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Time: %g elapsed seconds, %g CPU seconds\n"),wallseconds,cpusecondsU) ; vout_UCText(VOUT_Status,0,tbuf) ;
#endif
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Scan: %d lines, %d tokens\n"),tcb->tcb_lines,tcb->tcb_tokens) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   if (ctx->pi->BindingCount != 0)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Bind: %d added\n"),(ctx->pi->BindingCount)) ; vout_UCText(VOUT_Status,0,tbuf) ; } ;
	   if (ctx->IsctEvalCount != 0)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Isct: %d eval\'s, %d value I/O\'s, %d%% cache hits\n"),ctx->IsctEvalCount,ctx->IsctEvalValueGetCount,
			(ctx->IsctEvalValueGetCount == 0 ? 0 : DtoI(((double)ctx->IsctEvalValueCache*100.00)/(double)ctx->IsctEvalValueGetCount))) ;
		 vout_UCText(VOUT_Status,0,tbuf) ;
	    } ;
	   if (ctx->ContextAddCount != 0)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("   Ctx: %d adds, %d I/O, %d Cache\'s, %d Slams\n"),ctx->ContextAddCount,ctx->ContextAddGetCount,
			ctx->ContextAddCache,ctx->ContextAddSlams) ; vout_UCText(VOUT_Status,0,tbuf) ;
	    } ;
	   if (ctx->pi->CacheCalls > 0)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Cache:%d calls, %d%% hits\n"),ctx->pi->CacheCalls,DtoI((double)(ctx->pi->CacheHits*100.0)/(double)ctx->pi->CacheCalls)) ;  vout_UCText(VOUT_Status,0,tbuf) ; } ;
	   if (ctx->CtxnblCachePuts > 0)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Cnbl: %d insertions, %d hits\n"),ctx->CtxnblCachePuts,ctx->CtxnblCacheHits) ; vout_UCText(VOUT_Status,0,tbuf) ; } ;
	   if (ctx->pi->XDictLookups > 0)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit(" XDict: %d XDict lookups, %d%% cache hits\n"),ctx->pi->XDictLookups,
			DtoI((double)(ctx->pi->XDictCacheHits*100)/(double)ctx->pi->XDictLookups)) ;
	      vout_UCText(VOUT_Status,0,tbuf) ;
	    } ;
	   if (ctx->pi->AggPutCount != 0)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  WAgg: %d logical puts, %d IOs\n"),ctx->pi->AggPutCount,ctx->pi->AggPutWCount) ;
		 vout_UCText(VOUT_Status,0,tbuf) ;
	    } ;
	   if (ctx->pi->AggValueCount > 0)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  RAgg: %d calls, %d repeats (%d %%)\n"),ctx->pi->AggValueCount,ctx->pi->AggValueRepeatCount,intPC(ctx->pi->AggValueRepeatCount,ctx->pi->AggValueCount)) ;
		 vout_UCText(VOUT_Status,0,tbuf) ;
	    } ;
	   if (ctx->pi->mtLockConflicts > 0)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit(" MThrd: %d interlock conflicts\n"),ctx->pi->mtLockConflicts) ;
		 vout_UCText(VOUT_Status,0,tbuf) ;
	    } ;
	   if (cpusecondsU == 0) cpusecondsU = 1 ; if (wallseconds == 0) wallseconds = 1 ;
	   if (ctx->IsctEvalCount != 0)
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Wall: %d evals/sec, %d context-adds/second\n"),DtoI(ctx->IsctEvalCount/wallseconds),DtoI(ctx->ContextAddCount/wallseconds)) ; vout_UCText(VOUT_Status,0,tbuf) ;
	      UCsprintf(tbuf,UCsizeof(tbuf),UClit("   CPU: %d evals/cpusec, %d context-adds/cpusec\n"),DtoI(ctx->IsctEvalCount/cpusecondsU),DtoI(ctx->ContextAddCount/cpusecondsU)) ; vout_UCText(VOUT_Status,0,tbuf) ;
	    } ;
	   if (ctx->pi->ErrCount > 0) { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Errs: %d\n"),(ctx->pi->ErrCount)) ; vout_UCText(VOUT_Status,0,tbuf) ; } ;
	 } ;
#ifdef V4_BUILD_RUNTIME_STATS
	vout_UCText(VOUT_Status,0,UClit("Begin of detailed runtime statistics\n")) ;
	vout_UCText(VOUT_Status,0,UClit("Args  Number of Calls\n")) ;
	{ INDEX i ;
	  for(i=0;i<31;i++)
	   { if (ctx->pi->V4IntModTotalByArg[i] == 0) continue ;
	     UCsprintf(tbuf,UCsizeof(tbuf),UClit("%4d  %10d\n"),i,ctx->pi->V4IntModTotalByArg[i]) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   } ;
	}
	if (ctx->pi->V4IntModTotalByArg[31] != 0)
	 { UCsprintf(tbuf,UCsizeof(tbuf),UClit(" >30  %10d\n"),ctx->pi->V4IntModTotalByArg[31]) ; vout_UCText(VOUT_Status,0,tbuf) ; } ;
	{ int i ; double totClock = clock() ;
	  vout_UCText(VOUT_Status,0,UClit("Module         Calls     %CPU\n")) ; 
	  for(i=1;i<=V4BUILD_OpCodeMax;i++)
	   { if (ctx->pi->V4DtlModCalls[i] <= 0) continue ;
	     UCsprintf(tbuf,UCsizeof(tbuf),UClit("%-15s%5d%9.2f\n"),v4im_Display(i),ctx->pi->V4DtlModCalls[i],((double)ctx->pi->V4TicksInMod[i] / totClock) * 100.0) ;
	     vout_UCText(VOUT_Status,0,tbuf) ;
	   } ;
	}
	vout_UCText(VOUT_Status,0,UClit("    Calls  Module      Avg Num Args     %CPU\n")) ;
	for(;;)
	 { INDEX max,maxi,i ;
	   for(max = 0,i=1;i<=V4BUILD_OpCodeMax;i++)
	    { if (ctx->pi->V4DtlModCalls[i] > max) { max = ctx->pi->V4DtlModCalls[i] ; maxi = i ; } ; } ;
	   if (max == 0) break ;
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("%9d  %-12s %7.2f\n"),ctx->pi->V4DtlModCalls[maxi],v4im_Display(maxi),(double)ctx->pi->V4ArgsInMod[maxi]/(double)ctx->pi->V4DtlModCalls[maxi]) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   ctx->pi->V4DtlModCalls[maxi] = 0 ;
	 } ;
#endif
}






/*	Here on ^P from VMS Systems		*/

#ifdef VMSOS
void vms_ctrlp()
{
	ctrlp(TRUE) ;
}
#endif

/*	Here on ^P from Unix Systems		*/

void unix_ctrlp()
{ void unix_ctrlp() ;

#ifdef UNIX
	signal(SIGTSTP,unix_ctrlp) ;	/* Reset the signal */
	ioctl(0,TCFLSH,2) ;		/* Flush terminal I/O buffers */
#endif
	ctrlp(TRUE) ;
}

void ctrlp()
{ 
  struct V4C__ProcessInfo *pi ;
  struct V4LEX__TknCtrlBlk *tcb ;
  UCCHAR bbuf[128],sbuf[64] ;

	pi = v_GetProcessInfo() ;
	if ((tcb = pi->MainTcb) == NULL) { vout_UCText(VOUT_Warn,0,UClit("@*Not within include file\n")) ; return ; } ;
	if (tcb->ilvl[tcb->ilx].mode == V4LEX_InpMode_File)
	 { if (UCstrlen(tcb->ilvl[tcb->ilx].input_str) > UCsizeof(sbuf) - 5)
	    { UCstrncpy(sbuf,tcb->ilvl[tcb->ilx].input_str,UCsizeof(sbuf) - 5) ; UCstrcpy(&sbuf[UCsizeof(sbuf) - 5],UClit("...")) ; }
	    else { UCstrcpy(sbuf,tcb->ilvl[tcb->ilx].input_str) ; } ;
	   v_Msg(NULL,bbuf,"@* Reading %1U (line %2d): %3U\n",tcb->ilvl[tcb->ilx].file_name,tcb->ilvl[tcb->ilx].current_line,sbuf) ;
	   vout_UCText(VOUT_Trace,0,bbuf) ;
	   return ;
	 } ;
	vout_UCText(VOUT_Warn,0,UClit("*Not within include file\n")) ; return ;
}



#ifndef WINNTDLL
//void v3_error(errnum,subsys,module,mnemonic,msg,optarg)
//  int errnum ;
//  char *subsys,*module,*mnemonic,*msg ;
//  void *optarg ;
//{
//	v4_error(errnum,0,subsys,module,mnemonic,msg,optarg) ;
//}

#ifdef ALTVARGS						/* Need funky header for SCO variable-arg handler */
void v4_error(va_alist)
  va_dcl
{ extern int *v3unitp ;
  struct V4LEX__TknCtrlBlk *tcb ;
  struct V4C__ProcessInfo *pi ;
  int errnum ; char *subsys,*module,*mne,*msg ;
  va_list args ; char *format ;
  char ebuf[250] ; int code ; UCCHAR v4ubuf[2500] ;
  char path[200],tmp[10000], *b ; int i,j,nx ;

/*	Do special formatting on error message */
	va_start(args) ; errnum = va_arg(args,int) ; tcb = va_arg(args,struct V4LEX__TknCtrlBlk *) ;
	subsys = va_arg(args,char *) ; module = va_arg(args,char *) ; mne = va_arg(args,char *) ;
	format = va_arg(args,char *) ; vsprintf(ebuf,format,args) ; va_end(args) ;
#else							/* All other UNIX */
void v4_error(errnum,tcb,subsys,module,mne,msg)
  int errnum ;
  struct V4LEX__TknCtrlBlk *tcb ;
  char *subsys,*module,*mne,*msg ;

{ extern int *v3unitp ;
  va_list ap ;
  struct V4C__ProcessInfo *pi ;
  char ebuf[2500] ; UCCHAR v4ubuf[2500] ;
  UCCHAR path[256] ; UCCHAR tmp[10000] ; int i,j ;

	inV4Error = TRUE ;
/*	Do special formatting on error message */
//	va_start(ap,mne) ; msg = va_arg(ap,char *) ; vsprintf(ebuf,msg,ap) ; va_end(ap) ;
	va_start(ap,msg) ; vsprintf(ebuf,msg,ap) ; va_end(ap) ;
#endif
	pi = v_GetProcessInfo() ;
//	pi->EscFlag = UNUSED ;				/* Disable/suspend escape-handler thread */
	pi->XMLIndent = UNUSED ;			/* Reset any indentation for XML */
	pi->NestMax = FALSE ;
#define MainTcbx (pi->MainTcb)
/*	Look for primary file & line number for main error message */
	if (tcb == NULL)
	 { ZUS(tmp) ;
	   for(i=(MainTcbx == NULL ? 0 : MainTcbx->ilx);i>0;i--)
	    { if (MainTcbx->ilvl[i].file_name[0] == 0) continue ;		/* No file associated with this level */
	      v_Msg(NULL,path,"@(%1U %2d)",vlex_FullPathNameOfFile(MainTcbx->ilvl[i].file_name,NULL,0),MainTcbx->ilvl[i].current_line) ;
	      if (UCstrlen(tmp) > 0) { UCstrcat(tmp,UClit(",")) ; } ;
	      UCstrcat(tmp,path) ;
	    } ;
	 } else
	 { ZUS(tmp) ;
	   for(i=tcb->ilx;i>0;i--)
	    { if (tcb->ilvl[i].file_name[0] == 0) continue ;		/* No file associated with this level */
	      v_Msg(NULL,path,"@(%1U %2d)",vlex_FullPathNameOfFile(tcb->ilvl[i].file_name,NULL,0),tcb->ilvl[i].current_line) ;
	      if (UCstrlen(tmp) > 0) { UCstrcat(tmp,UClit(",")) ; } ;
	      UCstrcat(tmp,path) ;
	    } ;
	 } ;

	if (pi->ErrEcho)
	 { if (strlen(subsys) == 0)
	    { v_Msg(NULL,v4ubuf,"@*%1U %2U\n",tmp,pi->ctx->ErrorMsg) ; }
	    else { v_Msg(NULL,v4ubuf,"@*%1s %2s.%3s.%4s -  %5s\n",tmp,subsys,module,mne,ebuf) ; } ;
	   vout_UCText(VOUT_Err,0,v4ubuf) ;
	 } ;
/*	Should we point to offending whatever ? */
	if (tcb < (struct V4LEX__TknCtrlBlk *)100) tcb = NULL ;
/*	Only update error count if not a warning or alert */
	if (!(errnum == V4E_Warn || errnum == V4E_Alert)) pi->ErrCount++ ;
	pi->ErrNum = errnum ;
printf("V4Error tcb=%p\n",tcb) ;
	if (tcb != NULL)
	 {
	   UCstrcpy(tmp,tcb->ilvl[tcb->ilx].input_str) ;
/*	   Set up to make pointer */
	   j = tcb->ilvl[tcb->ilx].input_ptr - tcb->ilvl[tcb->ilx].input_str ;
	   for (i=0;i<j-1;i++)
	    { if (tmp[i] > UClit(' ')) tmp[i] = UClit(' ') ; } ;
	   tmp[i] = UClit('^') ; tmp[++i] = UCEOS ;
	   if (pi->ErrEcho)
	    { v_Msg(NULL,v4ubuf,"@  %1U",tcb->ilvl[tcb->ilx].input_str) ; vout_UCText(VOUT_Err,0,v4ubuf) ;
	      if (tcb->ilvl[tcb->ilx].input_str[UCstrlen(tcb->ilvl[tcb->ilx].input_str)-1] > 15) vout_NL(VOUT_Err) ;
	      v_Msg(NULL,v4ubuf,"@  %1U\n",tmp) ; vout_UCText(VOUT_Err,0,v4ubuf) ;
	    } ;
	   if (strlen(subsys) > 0)		/* Only do this for old-style error messages */
	    { switch(tcb->type)
	       { default:				ZUS(v4ubuf) ; break ;
		   case V4LEX_TknType_String:		v_Msg(NULL,v4ubuf,"@  Token-String : %1U\n",tcb->UCstring) ; break ;
		   case V4LEX_TknType_Keyword:		v_Msg(NULL,v4ubuf,"@  Token-Keyword: %1U\n",tcb->UCkeyword) ; break ;
		   case V4LEX_TknType_Punc:		v_Msg(NULL,v4ubuf,"@  Token-Punc: %1U\n",tcb->UCkeyword) ; break ;
		   case V4LEX_TknType_Integer:		v_Msg(NULL,v4ubuf,"@  Token-Integer: %1d(%2d)\n",tcb->integer,tcb->decimal_places) ; break ;
		   case V4LEX_TknType_LInteger:		UCsprintf(v4ubuf,255,UClit("  Token-LInteger: %I64d(%d)\n"),tcb->Linteger,tcb->decimal_places) ; break ;
		   case V4LEX_TknType_Float:		UCsprintf(v4ubuf,255,UClit("  Token-Float  : %G\n"),tcb->floating) ; break ;
		   case V4LEX_TknType_EOL:		UCstrcpy(v4ubuf,UClit("  Token-End-of-Line\n")) ; break ;
	       } ; if (pi->ErrEcho && UCstrlen(v4ubuf) > 0) vout_UCText(VOUT_Err,0,v4ubuf) ;
	    } ;
	 } ;
//printf("V4Error ctx=%d, errecho=%d, compiling=%d\n",pi->ctx,pi->ErrEcho,pi->Compiling) ;
	if (pi->ctx != NULL && pi->ErrEcho && (--pi->Compiling < 0 || tcb == NULL))	/* If got a context & not compiling then try to dump out current state */
	 {  
	   v4trace_ExamineState(pi->ctx,NULL,V4_EXSTATE_All,VOUT_Err) ;
///*	   Try and output Isct stack information */
//	   vout_Text(VOUT_Err,0,"*V4 Call Stack Detail at time of Error-\n") ;
//	   for(i=pi->ctx->NestedIntMod-1;i>=0;i--)
//	    { v_Msg(pi->ctx,NULL,"@*    %1d. %2P\n",i+1,pi->ctx->IsctStack[i].IsctPtr) ; vout_UCText(VOUT_Err,0,pi->ctx->ErrorMsg) ;
//	    } ;
	   v_Msg(pi->ctx,NULL,"@*Last V4Eval command was: %1S\n",pi->LastV4CommandIndex) ; vout_UCText(VOUT_Err,0,pi->ctx->ErrorMsg) ;
	 } ;
/*	If we are running as batch job then abort immediately, otherwise longjmp to GKW */
	{ static COUNTER v4ErrCount=0 ;
	  if(!v_IsAConsole(stdin) && (v4ErrCount++) > 5)
	   { v4_ExitStats(pi->ctx,(tcb == NULL ? MainTcbx : tcb)) ; v4mm_FreeResources(pi) ; v4mm_MemoryExit() ;
	     exit(EXITABORT) ;
	   } ;
	}
//	strncpy(pi->ErrMsg,ebuf,(sizeof pi->ErrMsg)-1) ;
	inV4Error = FALSE ;
	UCstrcpyAtoU(pi->ErrMsg,ebuf) ;
	longjmp(pi->environment,1) ;
}


/*	Alternate v4_error when error message is UTF-16 (Unicode) (no further formatting) */
void v4_UCerror(errnum,tcb,subsys,module,mne,ucmsg)
  int errnum ;
  struct V4LEX__TknCtrlBlk *tcb ;
  char *subsys,*module,*mne ;
  UCCHAR *ucmsg ;
{ extern int *v3unitp ;
  struct V4C__ProcessInfo *pi ;
  UCCHAR tbuf[UCReadLine_MaxLine] ;
  UCCHAR path[200],tmp[10000] ; int i,j,stream ;

	inV4Error = TRUE ;
	pi = v_GetProcessInfo() ;
//	pi->EscFlag = UNUSED ;				/* Disable/suspend escape-handler thread */
	pi->XMLIndent = UNUSED ;			/* Reset any indentation for XML */
	pi->NestMax = FALSE ;
	

	switch(errnum)
	 { default:		stream = VOUT_Err ; pi->ErrCount++ ; break ;
	   case V4E_Warn:	stream = VOUT_Warn ; pi->WarnCount++ ; UCstrncpy(pi->WarnMsg,ucmsg,UCsizeof(pi->WarnMsg)) ; goto end_erroutput ;	/* Warn & Alert - not really error so don't output error info */
	   case V4E_Alert:	stream = VOUT_Status ; goto end_erroutput ;
	 } ;
	pi->ErrNum = errnum ;

#define MainTcbx (pi->MainTcb)
/*	Look for primary file & line number for main error message */
	if (tcb == NULL)
	 { ZUS(tmp) ;
	   for(i=(MainTcbx == NULL ? 0 : MainTcbx->ilx);i>0;i--)
	    { if (MainTcbx->ilvl[i].file_name[0] == 0) continue ;		/* No file associated with this level */
	      v_Msg(NULL,path,"@(%1U %2d)",vlex_FullPathNameOfFile(MainTcbx->ilvl[i].file_name,NULL,0),MainTcbx->ilvl[i].current_line) ;
	      if (UCstrlen(tmp) > 0) { UCstrcat(tmp,UClit(",")) ; } ;
	      UCstrcat(tmp,path) ;
	    } ;
	 } else
	 { ZUS(tmp) ;
	   for(i=tcb->ilx;i>0;i--)
	    { if (tcb->ilvl[i].file_name[0] == 0) continue ;		/* No file associated with this level */
	      v_Msg(NULL,path,"@(%1U %2d)",vlex_FullPathNameOfFile(tcb->ilvl[i].file_name,NULL,0),tcb->ilvl[i].current_line) ;
	      if (UCstrlen(tmp) > 0) { UCstrcat(tmp,UClit(",")) ; } ;
	      UCstrcat(tmp,path) ;
	    } ;
	 } ;

	if (pi->ctx->rtStack[0].isctPtr != NULL) goto skip_tcb ;	/* If V4 runtime then don't output compilation junk */

	if (pi->ErrEcho)
	 { if (strlen(subsys) == 0)
	    { v_Msg(NULL,tbuf,"@*%1U %2U\n",tmp,pi->ctx->ErrorMsg) ; }
	    else { v_Msg(NULL,tbuf,"@*%1U %2s.%3s.%4s -  %5U\n",tmp,subsys,module,mne,ucmsg) ; } ;
	   vout_UCText(stream,0,tbuf) ;
	 } ;
/*	Should we point to offending whatever ? */
	if (tcb < (struct V4LEX__TknCtrlBlk *)100) tcb = NULL ;
	if (tcb != NULL)
	 { if (UCstrlen(tcb->ilvl[tcb->ilx].input_str) >= UCsizeof(tmp)) { tcb->ilvl[tcb->ilx].input_str[UCsizeof(tmp) - 5] = UCEOS ; } ;
	   UCstrcpy(tmp,tcb->ilvl[tcb->ilx].input_str) ;
/*	   Set up to make pointer */
	   j = tcb->ilvl[tcb->ilx].input_ptr - tcb->ilvl[tcb->ilx].input_str ;
	   if (j > UCstrlen(tcb->ilvl[tcb->ilx].input_str)) j = UCstrlen(tcb->ilvl[tcb->ilx].input_str) ;
	   for (i=0;i<j-1;i++)
	    { if (tmp[i] > UClit(' ')) tmp[i] = UClit(' ') ; } ;
	   tmp[i] = UClit('^') ; tmp[++i] = UCEOS ;
	   if (pi->ErrEcho)
	    { v_Msg(NULL,tbuf,"@  %1U",tcb->ilvl[tcb->ilx].input_str) ; vout_UCText(stream,0,tbuf) ;
	      if (tcb->ilvl[tcb->ilx].input_str[UCstrlen(tcb->ilvl[tcb->ilx].input_str)-1] > 15) vout_NL(stream) ;
	      v_Msg(NULL,tbuf,"@  %1U\n",tmp) ; vout_UCText(stream,0,tbuf) ;
	    } ;
	   if (strlen(subsys) > 0)		/* Only do this for old-style error messages */
	    { switch(tcb->type)
	       { default:				ZUS(tbuf) ; break ;
		   case V4LEX_TknType_String:		v_Msg(NULL,tbuf,"@  Token-String : %1U\n",tcb->UCstring) ; break ;
		   case V4LEX_TknType_Keyword:		v_Msg(NULL,tbuf,"@  Token-Keyword: %1U\n",tcb->UCkeyword) ; break ;
		   case V4LEX_TknType_Punc:		v_Msg(NULL,tbuf,"@  Token-Punc: %1U\n",tcb->UCkeyword) ; break ;
		   case V4LEX_TknType_Integer:		UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Token-Integer: %d(%d)\n"),tcb->integer,tcb->decimal_places) ; break ;
		   case V4LEX_TknType_LInteger:		UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Token-LInteger: %I64d(%d)\n"),tcb->Linteger,tcb->decimal_places) ; break ;
		   case V4LEX_TknType_Float:		UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Token-Float  : %G\n"),tcb->floating) ; break ;
		   case V4LEX_TknType_EOL:		UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Token-End-of-Line\n")) ; break ;
	       } ; if (pi->ErrEcho && UCstrlen(tbuf) > 0) vout_UCText(stream,0,tbuf) ;
	    } ;
	 } ;

skip_tcb:
	if (pi->ctx != NULL && pi->ErrEcho && --pi->Compiling < 0)	/* If got a context & not compiling then try to dump out current state */
	 {  
	   v4trace_ExamineState(pi->ctx,NULL,V4_EXSTATE_All,stream) ;
/*	   Try and output Isct stack information */
//	   vout_UCText(stream,0,UClit("*V4 Call Stack Detail at time of Error-\n")) ;
//	   for(i=pi->ctx->NestedIntMod-1;i>=0;i--)
//	    { v_Msg(pi->ctx,NULL,"@*    %1d. %2P\n",i+1,pi->ctx->IsctStack[i].IsctPtr) ; vout_UCText(stream,0,pi->ctx->ErrorMsg) ;
//	    } ;
	   v_Msg(pi->ctx,NULL,"@*Last V4Eval command was: %1S\n",pi->LastV4CommandIndex) ; vout_UCText(stream,0,pi->ctx->ErrorMsg) ;
	 } ;
end_erroutput:
/*	If we are running as batch job then abort immediately, otherwise longjmp to GKW */
/*	If we are running as batch job then abort immediately, otherwise longjmp to GKW */
	{ static COUNTER v4ErrCount=0 ;
	  if(!v_IsAConsole(stdin) && (v4ErrCount++) > 5)
	   { v4_ExitStats(pi->ctx,(tcb == NULL ? MainTcbx : tcb)) ; v4mm_FreeResources(pi) ; v4mm_MemoryExit() ;
	     exit(EXITABORT) ;
	   } ;
	}
//	strncpy(pi->ErrMsg,ebuf,(sizeof pi->ErrMsg)-1) ;
	inV4Error = FALSE ;
	UCstrcpy(pi->ErrMsg,ucmsg) ;
	longjmp(pi->environment,1) ;
}

//v3v4_v4im_Load_V3PicMod()
//{
//	v4_error(V4E_V3LOADUNS,0,"V4TEST","V3PicMod","V3LOADUNS","Load of V3 module not supported outside of V3") ;
//}
//
//v3v4_v4im_Unload_V3PicMod()
//{
//	v4_error(V4E_V3UNLOADUNS,0,"V4TEST","V3PicMod","V3UNLOADUNS","UnLoad of V3 module not supported outside of V3") ;
//}
#endif /* WINNTDLL */

#ifndef SCPCVAL
#define SCPCVAL scp->sc_pc
#define SCPCZAP scp->sc_pc = NULL
#endif
/*	v_signal_bus - Bus Error */

#ifdef ALPHAOSF
void v_signal_bus(sig,code,scp)
  int sig,code ;
  struct sigcontext *scp ;
#else
void v_signal_bus(code)
  int code ;
#endif
{ 
  void v_signal_bus() ;
#ifdef ALPHAOSF
  struct sigcontext scpx ;
#endif
  char errbuf[200] ;

	if (inV4Error) { exit(EXITABORT) ; } ;	/* Don't mess around and get stuck - just quit immediately */
#ifdef ALPHAOSF
	signal(SIGBUS,v_signal_bus) ;	/* Reset signal */
#endif
#ifdef VMSOS
	signal(SIGBUS,v_signal_bus) ;	/* Reset signal */
#endif
	signal(SIGSEGV,v_signal_bus) ;
#ifdef ALPHAOSF
	if (scp == NULL) { scp = &scpx ; SCPCZAP ; } ;
	sprintf(errbuf,"Access violation trap, code (%d), pc (%p)",code,SCPCVAL) ;
#else
	sprintf(errbuf,"Access violation trap, code (%d), nutcracker (%d %s)",code,v4NutCracker,v4NCBuf) ;
#ifdef NASTYBUG
	{ struct V4C__Context *ctx = gpi->ctx ; struct V4LEX__CompileDir *vcd ;
	  if (ctx->isctIntMod != NULL)
	   { P *ipt = ctx->isctIntMod ; P *imPt = ISCT1STPNT(ctx->isctIntMod) ;
	     UCCHAR *srcFile ; int hnum = ipt->Value.Isct.vis.c.HNum ;
	     printf("Current intmodx = %d\n",imPt->Value.IntVal) ;
	     if ((vcd = v4trace_LoadVCDforHNum(ctx,hnum,TRUE)) != NULL)
	      { srcFile = (ipt->Value.Isct.vis.c.vcdIndex < vcd->fileCount ? vcd->File[ipt->Value.Isct.vis.c.vcdIndex].fileName : NULL) ;
		if (vcd->File[ipt->Value.Isct.vis.c.vcdIndex].spwHash64 != 0 && vcd->File[ipt->Value.Isct.vis.c.vcdIndex].spwHash64 != gpi->spwHash64)
		 { v_Msg(ctx,UCTBUF2,"@%1d restricted-file:%3U: restricted-source\n",0,v4dbg_FormatLineNum(ipt->Value.Isct.vis.c.lineNumber)) ;
		 } else
		 { v_Msg(ctx,UCTBUF2,"@%1d %2U:%3U: %4P\n",0,(srcFile == NULL ? UClit("") : srcFile),v4dbg_FormatLineNum(ipt->Value.Isct.vis.c.lineNumber),ipt) ;
		 } ;
		vout_UCText(VOUT_Debug,0,UCTBUF2) ;
	      } ;
	   } ;
	  if (ctx->isct != NULL)
	   { P *ipt = ctx->isct ;
	     UCCHAR *srcFile ; int hnum = ipt->Value.Isct.vis.c.HNum ;
	     printf("rtStackX = %d rtStackFail=%d\n",ctx->rtStackX,ctx->rtStackFail) ;
	     if ((vcd = v4trace_LoadVCDforHNum(ctx,hnum,TRUE)) != NULL)
	      { srcFile = (ipt->Value.Isct.vis.c.vcdIndex < vcd->fileCount ? vcd->File[ipt->Value.Isct.vis.c.vcdIndex].fileName : NULL) ;
		if (vcd->File[ipt->Value.Isct.vis.c.vcdIndex].spwHash64 != 0 && vcd->File[ipt->Value.Isct.vis.c.vcdIndex].spwHash64 != gpi->spwHash64)
		 { v_Msg(ctx,UCTBUF2,"@%1d restricted-file:%3U: restricted-source\n",0,v4dbg_FormatLineNum(ipt->Value.Isct.vis.c.lineNumber)) ;
		 } else
		 { v_Msg(ctx,UCTBUF2,"@%1d %2U:%3U: %4P\n",0,(srcFile == NULL ? UClit("") : srcFile),v4dbg_FormatLineNum(ipt->Value.Isct.vis.c.lineNumber),ipt) ;
		 } ;
		vout_UCText(VOUT_Debug,0,UCTBUF2) ;
	      } ;
	   } ;
	}
#endif
#endif
	v4_error(0,0,"V4","TRAP","BUS",errbuf) ;
}
/*	v_signal_fpe - Handles Arithmetic Traps			*/

#ifdef ALPHAOSF
void v_signal_fpe(sig,code,scp)
  int sig,code ;
  struct sigcontext *scp ;
#else
void v_signal_fpe(code)
  int code ;
#endif
{ 
#ifdef ALPHAOSF
  struct sigcontext scpx ;
#endif
  char errbuf[250] ;

	signal(SIGFPE,v_signal_fpe) ;	/* Reset signal */
#ifdef ALPHAOSF
	if (scp == NULL) { scp = &scpx ; SCPCZAP ; } ;
	sprintf(errbuf,"Arithmetic exception trap, code (%d), pc (%p)",code,SCPCVAL) ;
#else
	sprintf(errbuf,"Arithmetic exception trap, code (%d)",code) ;
#endif
	v4_error(0,0,"V4","TRAP","ARITH",errbuf) ;
}

/*	v_signal_ill - Illegal Instruction Trap			*/

#ifdef ALPHAOSF
void v_signal_ill(sig,code,scp)
  int sig,code ;
  struct sigcontext *scp ;
#else
void v_signal_ill(code)
  int code ;
#endif
{ 
#ifdef ALPHAOSF
  struct sigcontext scpx ;
#endif
  char errbuf[200] ;

	signal(SIGILL,v_signal_ill) ; signal(SIGFPE,v_signal_fpe) ; /* Reset signal */
#ifdef ALPHAOSF
	if (scp == NULL) { scp = &scpx ; SCPCZAP ; } ;
	sprintf(errbuf,"Illegal instruction/addressing trap, code (%d), pc (%p)",code,SCPCVAL) ;
#else
	sprintf(errbuf,"Illegal instruction/addressing trap, code (%d)",code) ;
#endif
	v4_error(0,0,"V4","TRAP","INST",errbuf) ;
}

#ifdef VMSOS
/*	vax_signal_fpe - Handles Arithmetic Traps			*/

void vax_signal_fpe(sigint,type)
  int sigint,type ;
 { char msg[30],buf[100] ;
   void vax_signal_fpe() ;

	signal(SIGFPE,vax_signal_fpe) ;	/* Reset signal */
	switch(type)
	 { default: sprintf(msg,"*Unknown FPE trap=%d *",type) ; break ;
	   case FPE_INTOVF_TRAP: strcpy(msg,"Integer Overflow") ; break ;
	   case FPE_INTDIV_TRAP: strcpy(msg,"Integer Division by Zero") ; break ;
	   case FPE_FLTOVF_TRAP: strcpy(msg,"Floating Overflow") ; break ;
	   case FPE_FLTDIV_TRAP: strcpy(msg,"Floating/decimal division by zero") ; break ;
	   case FPE_FLTUND_TRAP: strcpy(msg,"Floating underflow") ; break ;
	   case FPE_DECOVF_TRAP: strcpy(msg,"Decimal overflow") ; break ;
	   case FPE_FLTOVF_FAULT: strcpy(msg,"Floating overflow fault") ; break ;
	   case FPE_FLTDIV_FAULT: strcpy(msg,"Floating divide by zero fault") ; break ;
	   case FPE_FLTUND_FAULT: strcpy(msg,"Floating underflow fault") ; break ;
	 } ;
	v4_error(0,0,"V4","TRAP","ARITH",buf,0) ;
 } ;

/*	vax_signal_ill - Illegal Instruction Trap			*/

void vax_signal_ill(type)
  int type ;
 { char msg[30],buf[100] ;
   void vax_signal_fpe(),vax_signal_ill() ;

	signal(SIGILL,vax_signal_ill) ; signal(SIGFPE,vax_signal_fpe) ; /* Reset signal */
	switch(type)
	 { default: sprintf(msg,"*Unknown ILL trap=%d *",type) ; break ;
	   case ILL_PRIVIN_FAULT: strcpy(msg,"Reserved instruction") ; break ;
	   case ILL_RESOP_FAULT: strcpy(msg,"Reserved operand") ; break ;
	   case ILL_RESAD_FAULT: strcpy(msg,"Reserved addressing") ; break ;
	 } ;
	v4_error(0,0,"V4","TRAP","ILL",buf,0) ;
}
#endif