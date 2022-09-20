/*	vsort.c - StandAlone Sort Utility for MIDAS

	Created 7-Sep-1993 by Victor E. Hansen			*/

/*	To build on Alpha/OSF use-
	$cc -Olimit 600 -O2 -o xvsort -w vsort.c

/*	To build on Linux use-
	$cc -m32 -O3 -o xvsort -DINCLUDE_VCOMMON -w vsort.c vcommon.c -lm
	$cc -m32 -g -o dxvsort -DINCLUDE_VCOMMON -w vsort.c vcommon.c -lm

	To build on Windows-
	cl /w /O2 /DINCLUDE_VCOMMON /Fexvsort.exe vsort.c vcommon.c shell32.lib wsock32.lib 
	cl /w /DINCLUDE_VCOMMON /DINHIBIT_WANTMYSQL /INHIBIT_WANTORACLE /Fexvsort.exe vsort.c vcommon.c shell32.lib wsock32.lib Ws2_32.lib
*/

#include "v4defs.c"
#ifdef UNIX
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#ifdef VMSOS
#include <stat.h>
#include <file.h>
#endif
#ifdef WINNT
#include <windows.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
#endif
#include <stdio.h>
#define UNIX 1

#define VSRT_Version 1
#define VSRT_VersionMinor 16

struct V4DPI__LittlePoint protoInt, protoDbl, protoFix, protoDict, protoAlpha, protoLog, protoForeign, protoNone, protoUCChar, protoNull ;


//#include "vsortdef.c"

#ifdef VSORT_AS_SUBROUTINE
#define EXITErr return(FALSE)
#define EXITOk return(TRUE)
#else
#define EXITErr vout_UCText(VOUT_Err,0,msg) ; exit(EXITABORT)
#define EXITOk exit(EXITOK)
#endif

struct VSRT__ControlBlock *VSRT_CONTROL_BLOCK ;
void vsort_HeapSortTallyList(int,struct V4IM__TallySort[]) ;

#ifdef VMSOS
#ifndef VSORT_AS_SUBROUTINE
mscu_vms_errmsg(vms_code,str)
  int vms_code ;			/* Standard vms error value */
  char *str ;				/* Updated with string equivalent */
 { struct {
      short int length,desc ;
      BYTE *ptr ;
    } str_dsc ;				/* VMS string descriptor */
   short int res_len ;

	str_dsc.length = 250 ; str_dsc.desc = 0 ; str_dsc.ptr = str ;
	sys$getmsg(vms_code,&res_len,&str_dsc,1,0) ; *(str+res_len) = 0 ;
	return(str) ;
}
#endif
#endif

/*	vsrt_KeyCompare - Generic Key Comparison Module		*/
/*	Call: compare = vsrt_KeyCompare( e1ptr , e2ptr )
	  where compare is -1/0/+1 reflecting comparison,
		e1ptr,e2ptr are pointers to 2 entries to be compared	*/

int vsrt_KeyCompare(e1ptr,e2ptr)
  const void *e1ptr,*e2ptr ;
{ struct VSRT__ControlBlock *vcb ;
  int i,c ;
  struct V4IM__TallySort *vts1,*vts2;
  struct V4IS__DataIdKey *key ;
  short *sptr1,*sptr2 ; int *iptr1,*iptr2 ; B64INT *lptr1,*lptr2 ; char *aptr1,*aptr2 ; char *vptr1,*vptr2 ; double *dptr1,*dptr2 ; UCCHAR *uptr1,*uptr2 ;
  struct lki { int DataId,int1,int2,int3 ; } *ik1, *ik2 ;
  B64INT b641,b642 ; B64INT *pB641,*pB642 ;

	vcb = VSRT_CONTROL_BLOCK ;		/* Link to global control block */
	vcb->TotalCompares++ ;
	for(i=0;i<vcb->KeyCount;i++)
	 { switch (vcb->Key[i].Type)
	    { default: printf("? Key (%d) - Invalid type (%d)\n",i,vcb->Key[i].Type) ; exit(EXITABORT) ;
	      case VSRT_KeyType_Key:
		ik1 = (struct lki *)((char *)e1ptr + vcb->Key[i].Offset2) ; ik2 = (struct lki *)((char *)e2ptr + vcb->Key[i].Offset2) ;
		key = (struct V4IS__DataIdKey *)ik1 ;		/* Link to key structure to look at components */
		switch (key->KeyP.fld.KeyMode)
	 	 { default: printf("? Invalid KeyMode (%d)\n",key->KeyP.fld.KeyMode) ; exit(EXITABORT) ;
		   case V4IS_KeyMode_Int:
			if ( ik1->int1 == ik2->int1) { c = ik1->int2 - ik2->int2 ; }
			 else { c = ik1->int1 - ik2->int1 ; } ;
			break ;
		   case V4IS_KeyMode_Int2:
			if ( ik1->int1 == ik2->int1)
			 { if (ik1->int2 == ik2->int2) { c = ik1->int3 - ik2->int3 ; }
			    else { c = ik1->int2 - ik2->int2 ; } ;
			 } else { c = ik1->int1 - ik2->int1 ; } ;
			break ;
		    case V4IS_KeyMode_RevInt:
			if ( ik1->int1 == ik2->int1)
			 { if (ik1->int2 == ik2->int2) { c = 0 ; }
			    else { c = (ik2->int2 > ik1->int2 ? 1 : -1) ; } ;
			 } else { c = (ik1->int1 > ik2->int1 ? 1 : -1) ; } ;
			break ;
		    case V4IS_KeyMode_Fixed:
			if ( ik1->int1 == ik2->int1)
			 { memcpy(&b641,&ik1->int2,sizeof b641) ; memcpy(&b642,&ik2->int2,sizeof b641) ;
			   c = b641 - b642 ;
			 } else { c = ik1->int1 - ik2->int1 ; } ;
			break ;
		    case V4IS_KeyMode_Alpha:
			if ( ik1->int1 == ik2->int1)
			 { c = memcmp(&ik1->int2,&ik2->int2,key->KeyP.fld.Bytes-(sizeof ik1->int1)) ; }
			 else { c = ik1->int1 - ik2->int1 ; } ;
			break ;
		 } ;
		break ;
	      case VSRT_KeyType_Short:
		sptr1 = (short *)((char *)e1ptr + vcb->Key[i].Offset2) ; sptr2 = (short *)((char *)e2ptr + vcb->Key[i].Offset2) ;
		c = *sptr1 - *sptr2 ; break ;
	      case VSRT_KeyType_Int:
		iptr1 = (int *)((char *)e1ptr + vcb->Key[i].Offset2) ; iptr2 = (int *)((char *)e2ptr + vcb->Key[i].Offset2) ;
		c = *iptr1 - *iptr2 ; break ;
	      case VSRT_KeyType_V4TallyList:
		vts1 = (struct V4IM__TallySort *)e1ptr ; vts2 = (struct V4IM__TallySort *)e2ptr ;
		c = vts1->HashX - vts2->HashX ; if (c == 0) c = vts1->Order - vts2->Order ;
		return(c) ;
	      case VSRT_KeyType_Long:
		lptr1 = (B64INT *)((char *)e1ptr + vcb->Key[i].Offset2) ; lptr2 = (B64INT *)((char *)e2ptr + vcb->Key[i].Offset2) ;
		c = *lptr1 - *lptr2 ; break ;
	      case VSRT_KeyType_Alpha:
		aptr1 = (char *)((char *)e1ptr + vcb->Key[i].Offset2) ; aptr2 = (char *)((char *)e2ptr + vcb->Key[i].Offset2) ;
		c = strncmp(aptr1,aptr2,vcb->Key[i].Bytes) ; break ;
	      case VSRT_KeyType_UCChar:
		uptr1 = (UCCHAR *)((char *)e1ptr + vcb->Key[i].Offset2) ; uptr2 = (UCCHAR *)((char *)e2ptr + vcb->Key[i].Offset2) ;
		c = UCstrncmp(uptr1,uptr2,vcb->Key[i].Bytes) ; break ;
	      case VSRT_KeyType_VAlpha:
		vptr1 = (char *)((char *)e1ptr + vcb->Key[i].Offset2) ; vptr2 = (char *)((char *)e2ptr + vcb->Key[i].Offset2) ;
		c = strncmp(vptr1,vptr2,vcb->Key[i].Bytes) ; break ;
	      case VSRT_KeyType_Double:
		dptr1 = (double *)((char *)e1ptr + vcb->Key[i].Offset2) ; dptr2 = (double *)((char *)e2ptr + vcb->Key[i].Offset2) ;
//if ((dptr1 & 0x7) != 0 || (dptr2 & 0x7 != 0)) printf("%d: %p + %d = %p, %p + %d = %p\n",i,e1ptr,vcb->Key[i].Offset2,dptr1,e2ptr,vcb->Key[i].Offset2,dptr2) ;
		c = (*dptr1 < *dptr2 ? -1 : (*dptr1 == *dptr2 ? 0 : 1) ) ; break ;
	      case VSRT_KeyType_Fixed:
		pB641 = (B64INT *)((char *)e1ptr + vcb->Key[i].Offset2) ; pB642 = (B64INT *)((char *)e2ptr + vcb->Key[i].Offset2) ;
		c = (*pB641 < *pB642 ? -1 : (*pB641 == *pB642 ? 0 : 1) ) ; break ;
	    } ; if (c != 0) break ;
	 } ;
	return(vcb->Key[i].Decending ? -c : c) ;
}

/*	vsrt_KeyCmpTally - Standalone Compare for V4 Tally List Sort		*/
/*	Call: compare = vsrt_KeyCmpTally( e1ptr , e2ptr )
	  where compare is -1/0/+1 reflecting comparison,
		e1ptr,e2ptr are pointers to 2 entries to be compared	*/

int vsrt_KeyCmpTally(e1ptr,e2ptr)
  const void *e1ptr,*e2ptr ;
{ struct VSRT__ControlBlock *vcb ;
  B64INT *lptr1,*lptr2 ;

	vcb = VSRT_CONTROL_BLOCK ;		/* Link to global control block */
	vcb->TotalCompares++ ;
	lptr1 = (B64INT *)e1ptr ; lptr2 = (B64INT *)e2ptr ;
	return(*lptr1 - *lptr2) ;
}

#ifdef VSORT_AS_SUBROUTINE
LOGICAL vsort_entry(argstr,databuf,datalen,errbuf)
  UCCHAR *argstr ;
  void *databuf ;		/* If not NULL then points to buffer of records to be sort (i.e. files will be ignored) */
  int datalen ;			/* Length of databuf */
  UCCHAR *errbuf ;
{ int argc ; UCCHAR *p ;
  UCCHAR *argv[30] ;		/* List of pointers to arguments */
#else

UCCHAR **startup_envp ;
#ifdef WINNT
wmain(argc,argv,envp)
  int argc ;			/* Number of arguments passed via DCL */
  UCCHAR *argv[] ;		/* Argument values */
  UCCHAR **envp ;			/* Environment pointer */
#else
main(argc,argv,envp)
  int argc ;			/* Number of arguments passed via DCL */
  char *argv[] ;		/* Argument values */
  char **envp ;			/* Environment pointer */
#endif
{ 
#endif
  struct VSRT__ControlBlock *vcb ;
  struct stat statbuf ;
  struct V4__BktHdr sosbh ;
#ifdef VMSOS
  typedef unsigned long dclFPOS ;
#else
#if defined LINUX486 || defined RASPPI
  typedef off_t dclFPOS ;
#else
  typedef fpos_t dclFPOS ;
#endif /* LINUX486 */
 dclFPOS *fpos ;
#endif
  struct lcl__vkey {
     struct V4IS__DataIdKey dkey ;
     char AlphaValCont[500] ;	/* Continuation of alpha key value (dkey has only 1st four bytes) */
   } vkey,*vkeyp ;
  struct UC__File iUCFile ;
  BYTE *trbuf = NULL ;		/* Used as temp record buffer */
  BYTE *ibptr ;			/* Pointer into internal buffer of current "record" */
  int seqbuflen,ok ;
  int bytes,totalbytes,i,maxalign,maxoffset,ipos ; UCCHAR ucbuf[1024], *msg, *arg,*resptr ;
  clock_t cpuseconds ;
#ifdef WINNT
  FILETIME ftCreate,ftExit,ftKernel1,ftUser1 ;		/* Start kernel/user CPU times */
#else
  struct timeval bgnTime, endTime ;
#endif
#ifdef POSIXMT
  char ebuf[10] ;
#endif
#ifndef VSORT_AS_SUBROUTINE
  UCCHAR errmsg[1024] ;
#endif


#ifdef WINNT
	GetProcessTimes(GetCurrentProcess(),&ftCreate,&ftExit,&ftKernel1,&ftUser1) ;
#else
	gettimeofday (&bgnTime,NULL) ;
#endif
	cpuseconds = clock() ;



	vcb = (struct VSRT__ControlBlock *)malloc(sizeof *vcb) ; memset(vcb,0,sizeof *vcb) ;
	VSRT_CONTROL_BLOCK = vcb ;
	vcb = VSRT_CONTROL_BLOCK ;		/* Link to global control block */
	vcb->WorkSetMax = 0x3000000 ;		/* 0x3000000 about 50 million */
	vcb->InputFileType = V_IFT_FixBin ;	/* Fixed length binary */
	vcb->DeleteInput = FALSE ;

#ifdef VSORT_AS_SUBROUTINE
	argv[argc=0] = argstr ;
	for(p=argstr;;p++)
	 { if (*p == 0) break ; if (*p != UClit(' ')) continue ;
	   *p = 0 ; for(;;) { p++ ; if (*p == UCEOS) break ; if (*p != UClit(' ')) break ; } ;
	   if (*p == UCEOS) break ;
	   argv[++argc] = p ;
	 } ; argc++ ;
	vcb->NoIO = (databuf != NULL) ;
	msg = errbuf ;
#else
	msg = errmsg ; startup_envp = envp ;
#endif

#define NA if (*(++arg) == 0) arg = argv[++i] ; if (*arg == UClit('-')) goto err_arg ;
#define NUM(VAR) VAR = UCstrtol(arg,&resptr,10) ; if (*resptr != UCEOS) goto err_int ;

	for(i=1;i<argc;i++)
	 {
	   arg = argv[i] ;					/* Next argument */
	   if (*arg == UClit('-')) { arg++ ; }
	    else { 
//		   if (UCstrlen(vcb->InputFileName) == 0) { UCstrcpy(vcb->InputFileName,arg) ; continue ; }
//		   if (UCstrlen(vcb->OutputFileName) == 0) { UCstrcpy(vcb->OutputFileName,arg) ; continue ; } ;
		   if (UCempty(vcb->InputFileName)) { UCstrcpy(vcb->InputFileName,arg) ; continue ; }
		   if (UCempty(vcb->OutputFileName)) { UCstrcpy(vcb->OutputFileName,arg) ; continue ; } ;
		   v_Msg(NULL,msg,"VSrtCmdLine") ; EXITErr ;
		 } ;
	   switch (*arg)
	    { default:	v_Msg(NULL,msg,"VSrtSwtch",arg) ; EXITErr ;
	      case UClit('d'):		vcb->DeleteInput = TRUE ; break ;
	      case UClit('h'):
		printf("mooo\n") ;
		v_Msg(NULL,msg,"VSrtBanner",VSRT_Version,VSRT_VersionMinor) ; vout_UCText(VOUT_Status,0,msg) ;
		v_Msg(NULL,msg,"V4TBanner",V4IS_MajorVersion,V4IS_MinorVersion,UClit("")) ; vout_UCText(VOUT_Status,0,msg) ;
		if (!v_UCFileOpen(&iUCFile,v_GetV4HomePath(UClit("vsorthelp.v4i")),UCFile_Open_Read,TRUE,ucbuf,0)) { v_Msg(NULL,msg,"VSrtHelpGone",ucbuf) ; EXITErr ; } ;
		for(;;)
		 { if (v_UCReadLine(&iUCFile,UCRead_UC,ucbuf,UCsizeof(ucbuf),ucbuf) < 0) break ;
		   if (ucbuf[0] == UClit('!')) continue ; if (ucbuf[0] == UClit('/')) continue ;
		   vout_UCText(VOUT_Status,0,ucbuf) ;
		 } ; v_UCFileClose(&iUCFile) ;
		EXITOk ;
	      case UClit('k'):
		if (*(arg+1) == UClit('r')) { arg++ ; vcb->Key[vcb->KeyCount].Decending = TRUE ; } ;
		switch(*(++arg))
		 { default: v_Msg(NULL,msg,"VSrtSwtchOpt",arg-1,UClit("-ki/l/a/v/d/f/u")) ; EXITErr ;
		   case UClit('i'):	NA ; NUM(vcb->Key[vcb->KeyCount].Offset1) ; vcb->Key[vcb->KeyCount].Alignment = 3 ;
				vcb->Key[vcb->KeyCount].Type = VSRT_KeyType_Int ; vcb->Key[vcb->KeyCount].Bytes = sizeof (int) ;
				break ;
		   case UClit('l'):	NA ; NUM(vcb->Key[vcb->KeyCount].Offset1) ; vcb->Key[vcb->KeyCount].Alignment = ALIGN_LONG ;
				vcb->Key[vcb->KeyCount].Type = VSRT_KeyType_Long ; vcb->Key[vcb->KeyCount].Bytes = sizeof (B64INT) ;
				break ;
		   case UClit('f'):	NA ; NUM(vcb->Key[vcb->KeyCount].Offset1) ; vcb->Key[vcb->KeyCount].Alignment = ALIGN_LONG ;
				vcb->Key[vcb->KeyCount].Type = VSRT_KeyType_Fixed ; vcb->Key[vcb->KeyCount].Bytes = sizeof (B64INT) ;
				break ;
		   case UClit('s'):	NA ; NUM(vcb->Key[vcb->KeyCount].Offset1) ; vcb->Key[vcb->KeyCount].Alignment = 1 ;
				vcb->Key[vcb->KeyCount].Type = VSRT_KeyType_Short ; vcb->Key[vcb->KeyCount].Bytes = sizeof (short) ;
				break ;
		   case UClit('k'):	NA ;
				vcb->Key[vcb->KeyCount].Type = VSRT_KeyType_Key ; vcb->Key[vcb->KeyCount].Alignment = 3 ;
				vcb->Key[vcb->KeyCount].Offset1 = UCstrtol(arg,&resptr,10) ;
				switch (*resptr)
				 { default: goto err_int ;
				   case 0:  v_Msg(NULL,msg,"VSrtKeyVal",arg) ; EXITErr ;
				   case UClit('.'): vcb->Key[vcb->KeyCount].Bytes = UCstrtol(++resptr,&resptr,10) ;
					if (*resptr != 0) goto err_int ;
				 } ;
				vcb->InputFileType = V_IFT_V4Key ; break ;
		   case UClit('a'):	NA ;
				vcb->Key[vcb->KeyCount].Type = VSRT_KeyType_Alpha ; vcb->Key[vcb->KeyCount].Alignment = 0 ;
				vcb->Key[vcb->KeyCount].Offset1 = UCstrtol(arg,&resptr,10) ;
				switch (*resptr)
				 { default: goto err_int ;
				   case 0:  v_Msg(NULL,msg,"VSrtKeyVal",arg) ; EXITErr ;
				   case UClit('.'): vcb->Key[vcb->KeyCount].Bytes = UCstrtol(++resptr,&resptr,10) ;
					if (*resptr != 0) goto err_int ;
				 } ;
				break ;
		   case UClit('u'):	NA ;
				vcb->Key[vcb->KeyCount].Type = VSRT_KeyType_UCChar ; vcb->Key[vcb->KeyCount].Alignment = 0 ;
				vcb->Key[vcb->KeyCount].Offset1 = UCstrtol(arg,&resptr,10) ;
				switch (*resptr)
				 { default: goto err_int ;
				   case 0:  v_Msg(NULL,msg,"VSrtKeyVal",arg) ; EXITErr ;
				   case UClit('.'): vcb->Key[vcb->KeyCount].Bytes = UCstrtol(++resptr,&resptr,10) ;
					if (*resptr != 0) goto err_int ;
				 } ;
				break ;
		   case UClit('v'):	NA ;
				vcb->Key[vcb->KeyCount].Type = VSRT_KeyType_Alpha ; vcb->Key[vcb->KeyCount].Alignment = 0 ;
				vcb->Key[vcb->KeyCount].Offset1 = UCstrtol(arg,&resptr,10) ;
				switch (*resptr)
				 { default: goto err_int ;
				   case 0:  v_Msg(NULL,msg,"VSrtKeyVal",arg) ; EXITErr ;
				   case UClit('.'): vcb->Key[vcb->KeyCount].Bytes = UCstrtol(++resptr,&resptr,10) ;
					if (*resptr != 0) goto err_int ;
				 } ;
				break ;
		   case UClit('d'):	NA ; NUM(vcb->Key[vcb->KeyCount].Offset1) ;
				vcb->Key[vcb->KeyCount].Type = VSRT_KeyType_Double ; vcb->Key[vcb->KeyCount].Alignment = ALIGN_LONG ;
				vcb->Key[vcb->KeyCount].Bytes = sizeof (double) ;
				break ;
		   case UClit('x'):	NA ; NUM(vcb->Key[vcb->KeyCount].Offset1) ;
				vcb->Key[vcb->KeyCount].Type = VSRT_KeyType_V4TallyList ; vcb->Key[vcb->KeyCount].Alignment = ALIGN_DOUBLE ;
				vcb->Key[vcb->KeyCount].Bytes = sizeof (B64INT) ;
				break ;
		 } ;
		vcb->KeyCount++ ; break ;
	      case UClit('l'):
		NA ; NUM(vcb->InputBytes) ;
		if (vcb->InputBytes <= 0) { v_Msg(NULL,msg,"VSrtInpLen",arg) ; EXITErr ; } ;
		break ;
	      case UClit('m'):
		NA ; NUM(vcb->MaxEntriesToSort) ; break ;
	      case UClit('n'):
		NA ; NUM(vcb->EntryCount) ; break ;
	      case UClit('o'):
		NA ; NUM(vcb->OffsetBase) ; break ;
	      case UClit('q'):
		vcb->Quiet = TRUE ; break ;
	      case UClit('s'):
		NA ; NUM(vcb->StartEntryToSort) ; break ;
	      case UClit('v'):
		vcb->InputFileType = V_IFT_V4Seq ; break ;
	      case UClit('w'):
		NA ; NUM(vcb->WorkSetMax) ; vcb->WorkSetMax *= 1024 ; break ;
	    } ;
	 } ;
//	if (UCstrlen(vcb->InputFileName) == 0 || UCstrlen(vcb->OutputFileName) == 0)
	if (UCempty(vcb->InputFileName) || UCempty(vcb->OutputFileName))
	 { v_Msg(NULL,msg,"VSrtFiles",vcb->InputFileName,vcb->OutputFileName) ; EXITErr ; } ;

#ifdef VSORT_AS_SUBROUTINE
	if (vcb->NoIO)					/* If already got data in buffer, then link up */
	 { vcb->TotalBytes = datalen ; vcb->InternalBuffer = databuf ;
	   vcb->EntryBytes = vcb->InputBytes ;
	   vcb->EntryCount = datalen / vcb->EntryBytes ;
	   goto skipIO1 ;
	 } ;
#endif

	if (vcb->InputBytes == 0 && vcb->InputFileType == V_IFT_FixBin)
	 vcb->InputFileType = V_IFT_Text ;	/* If no record length then assume text */
#ifdef POSIXMT
	if ((vcb->ifd = open(vcb->InputFileName,O_RDONLY|O_BINARY)) == -1)
	 { v_Msg(NULL,msg,"VSrtInpErr",vcb->InputFileName,errno) ; EXITErr ; } ;

	fstat(vcb->ifd,&statbuf) ;		/* Get size of the file */
	vcb->TotalBytes = statbuf.st_size ;
#endif
#ifdef WINNT
	if (vcb->InputFileType == V_IFT_Text)	/* If a text file then use POSIX routines to read (stream) */
	 { if ((vcb->ifd = UCopen(vcb->InputFileName,O_RDONLY,0)) == -1)
	    { v_Msg(NULL,msg,"VSrtInpErr",vcb->InputFileName,errno) ; EXITErr ; } ;
	   fstat(vcb->ifd,&statbuf) ;		/* Get size of the file */
	   vcb->TotalBytes = statbuf.st_size ;
	 } else
	 { 
	   vcb->ihandle = UCCreateFile(vcb->InputFileName,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL) ;
	   if (vcb->ihandle == INVALID_HANDLE_VALUE)
	    { v_Msg(NULL,msg,"VSrtInpErr",vcb->InputFileName,GetLastError()) ; EXITErr ; } ;
	   vcb->TotalBytes = GetFileSize(vcb->ihandle,NULL) ;
	 } ;
#endif
	switch (vcb->InputFileType)
	 {
	   case V_IFT_V4Seq:
		vcb->InputBytes = V_V4Seq_MaxBytes ;
#ifdef POSIXMT
		if ((vcb->ifp = (FILE *)fdopen(vcb->ifd,"r")) == 0)
		 { v_Msg(NULL,msg,"VSrtfdopen",vcb->InputFileName,errno) ; EXITErr ; } ;
		fread(&sosbh,sizeof sosbh,1,vcb->ifp) ;			/* Suck up bucket header for root */
		if (sosbh.BktType != V4_BktHdrType_SeqOnly || (sosbh.BktNum != 0))
		 { v_Msg(NULL,msg,"VSrtNotV4IS",vcb->InputFileName) ; EXITErr ; } ;
#endif
#ifdef WINNT
		ReadFile(vcb->ihandle,&sosbh,sizeof sosbh,&bytes,NULL) ;
		if (sosbh.BktType != V4_BktHdrType_SeqOnly || (sosbh.BktNum != 0))
		 { v_Msg(NULL,msg,"VSrtNotV4IS",vcb->InputFileName) ; EXITErr ; } ;
#endif
		vcb->EntryCount = 0 ;			/* Will have to count as we read ! */
		break ;
	   case V_IFT_Text:
		vcb->EntryCount = 0 ;			/* Will have to count as we read ! */
		vcb->InputBytes = V_Text_MaxBytes ;
		trbuf = v4mm_AllocChunk(V_Text_MaxBytes,FALSE) ;
		if ((vcb->ifp = fdopen(vcb->ifd,"r")) == 0)
		 { v_Msg(NULL,msg,"VSrtfdopen",vcb->InputFileName,errno) ; EXITErr ; } ;
		break ;
	   case V_IFT_V4Key:
		vcb->InputBytes = V_V4Seq_MaxBytes ;
		break ;
	   case V_IFT_FixBin:
		if (vcb->EntryCount == 0 && vcb->InputBytes != 0) vcb->EntryCount = vcb->TotalBytes/vcb->InputBytes ;
		if (vcb->InputBytes == 0 && vcb->EntryCount != 0) vcb->InputBytes = vcb->TotalBytes/vcb->EntryCount ;
		vcb->EntryBytes = vcb->InputBytes ;			/* Default entry to entire record for now */
		if (vcb->TotalBytes != vcb->InputBytes*vcb->EntryCount)
		 { v_Msg(NULL,msg,"VSrtSize",vcb->TotalBytes,vcb->EntryCount,vcb->InputBytes) ; EXITErr ; } ;
		break ;
	 } ;

skipIO1:
	maxoffset = 0 ;
	for(i=0;i<vcb->KeyCount;i++)
	 { if (((vcb->Key[i].Offset1-vcb->OffsetBase) & vcb->Key[i].Alignment) != 0)
	    { v_Msg(NULL,msg,"VSrtAlign",i+1,vcb->Key[i].Offset1,vcb->OffsetBase,vcb->Key[i].Alignment) ; EXITErr ; } ;
	   if ((vcb->Key[i].Offset1-vcb->OffsetBase) + vcb->Key[i].Bytes - 1 > vcb->InputBytes)
	    { v_Msg(NULL,msg,"VSrtOffset",i+1,vcb->Key[i].Offset1,vcb->Key[i].Bytes,vcb->InputBytes) ; EXITErr ; } ;
	   if (vcb->Key[i].Offset1-vcb->OffsetBase < 0)
	    { v_Msg(NULL,msg,"VSrtOffset1",i+1,vcb->Key[i].Offset1,vcb->OffsetBase) ; EXITErr ; } ;
	   vcb->Key[i].Offset1 -= vcb->OffsetBase ;
	   vcb->Key[i].Offset2 = vcb->Key[i].Offset1 ;
	   if (vcb->Key[i].Offset1+vcb->Key[i].Bytes > maxoffset) maxoffset = vcb->Key[i].Offset1+vcb->Key[i].Bytes ;
	 } ;

#ifdef VSORT_AS_SUBROUTINE
	if (vcb->NoIO)					/* Got data already, just sort & return */
	 { qsort(vcb->InternalBuffer,vcb->EntryCount,vcb->EntryBytes,vsrt_KeyCompare) ;
	   free(vcb) ;
	   EXITOk ;
	 } ;
#endif
	if (vcb->TotalBytes > vcb->WorkSetMax && vcb->InputFileType == V_IFT_FixBin)
	 vcb->InputFileType = V_IFT_FixBinBig ;		/* If total bytes is too big then just track keys */

	switch (vcb->InputFileType)
	 {
	   case V_IFT_V4Key:
	   case V_IFT_FixBinBig:
	   case V_IFT_V4Seq:
	   case V_IFT_Text:
		vcb->EntryPosOffset = 0 ;		/* Position in sort record for position in main file */
		vcb->Key[0].Offset2 = sizeof (*fpos) ;
		maxalign = vcb->Key[0].Offset2 - 1 ;	/* Set up initial maxalign for (dclFPOS) */
		for(i=0;i<vcb->KeyCount;i++)
		 { vcb->Key[i].Offset2 = (vcb->Key[i].Offset2 + vcb->Key[i].Alignment) & ~vcb->Key[i].Alignment ;
		   vcb->Key[i+1].Offset2 = vcb->Key[i].Offset2 + vcb->Key[i].Bytes ;
		   maxalign |= vcb->Key[i].Alignment ;
		 } ;
		vcb->EntryBytes = (vcb->Key[i].Offset2+maxalign) & ~maxalign ;	/* Get length of entire key entry */
		break ;
	 } ;
	switch (vcb->InputFileType)
	 {
	   case V_IFT_FixBinBig:
		vcb->TotalBytes = vcb->EntryBytes * vcb->EntryCount ;
		break ;
	   case V_IFT_V4Key:
	   case V_IFT_V4Seq:
		if (vcb->EntryCount > 0) { vcb->EntryCount+=20 ; vcb->TotalBytes = vcb->EntryBytes * vcb->EntryCount ; }
		 else { vcb->TotalBytes = (vcb->TotalBytes*150)/100 ; } ; /* Guess that can't be more than 150% of entire file */
		break ;
	   case V_IFT_Text:
		vcb->TotalBytes = (vcb->TotalBytes*250)/100 ;	/* Guess that can't be more than 250% of entire file */
		trbuf = (BYTE *)malloc(vcb->InputBytes) ; totalbytes = 0 ;
		for(i=0;;i++)				/* Have to get a record count */
		 { if (fgets(trbuf,vcb->InputBytes,vcb->ifp) == NULL) break ;
		 } ; rewind(vcb->ifp) ;			/* Set back to BOF */
		vcb->TotalBytes = vcb->EntryBytes * i ; /* Figure out number of bytes for internal buffer */
		break ;
	 } ;

	if (!vcb->Quiet)
	 { v_Msg(NULL,msg,"VSrtBanner",VSRT_Version,VSRT_VersionMinor) ; vout_UCText(VOUT_Status,0,msg) ;
	   v_Msg(NULL,msg,"V4TBanner",V4IS_MajorVersion,V4IS_MinorVersion,UClit("")) ; vout_UCText(VOUT_Status,0,msg) ;
	 } ;

	vcb->InternalBuffer = (BYTE *)malloc(vcb->TotalBytes == 0 ? 4 : vcb->TotalBytes) ;
	if (vcb->InternalBuffer == NULL || vcb->InternalBuffer == (BYTE *)-1)
	 { v_Msg(NULL,msg,"VSrtBufAlloc",vcb->TotalBytes) ; EXITErr ; } ;

	switch (vcb->InputFileType)
	 {
	   case V_IFT_V4Key:
		trbuf = (BYTE *)malloc(vcb->InputBytes) ;
		if (vcb->EntryCount == 0) vcb->EntryCount = V4LIM_BiggestPositiveInt ;
		for(i=0;i<vcb->EntryCount;i++)		/* Have to read each "record" and convert keys */
		 { ibptr = (BYTE *)(vcb->InternalBuffer+(i*vcb->EntryBytes)) ;
		   fpos = (dclFPOS *)ibptr+vcb->EntryPosOffset ;
		   *fpos = totalbytes ;			/* Remember position of this record */
#ifdef POSIXMT
		   bytes = read(vcb->ifd,&vkey,sizeof vkey.dkey) ;
		   if (bytes < sizeof vkey.dkey) break ;		/* Hit EOF */
		   ok = vkey.dkey.KeyP.fld.Bytes + sizeof vkey.dkey.DataId - sizeof vkey.dkey ;	/* Any more to read ? */
		   if (ok < 0)
		    { v_Msg(NULL,msg,"VSrtKey4mat") ; EXITErr ; } ;
		   if (ok + sizeof vkey.dkey > vcb->EntryBytes)
		    { v_Msg(NULL,msg,"VSrtRecSize",i,ok+sizeof vkey.dkey,vcb->EntryBytes) ; EXITErr ; } ;
		   totalbytes += (ok + sizeof vkey.dkey) ;
		   if (ok > 0)						/* Read rest of alpha key */
		    { bytes = read(vcb->ifd,&vkey.AlphaValCont,ok) ;
		      if (bytes < ok) { v_Msg(NULL,msg,"VSrtReadErr",i+1,vcb->InputFileName,errno) ; EXITErr ; } ;
		    } ;
#endif
#ifdef WINNT
		   ReadFile(vcb->ihandle,&vkey,sizeof vkey.dkey,&bytes,NULL) ;
		   if (bytes < sizeof vkey.dkey) break ;		/* Hit EOF */
		   ok = vkey.dkey.KeyP.fld.Bytes + sizeof vkey.dkey.DataId - sizeof vkey.dkey ;	/* Any more to read ? */
		   if (ok < 0)
		    { v_Msg(NULL,msg,"VSrtKey4mat") ; EXITErr ; } ;
		   if (ok + sizeof vkey.dkey > vcb->EntryBytes)
		    { v_Msg(NULL,msg,"VSrtRecSize",i,ok+sizeof vkey.dkey,vcb->EntryBytes) ; EXITErr ; } ;
		   totalbytes += (ok + sizeof vkey.dkey) ;
		   if (ok > 0)						/* Read rest of alpha key */
		    { ReadFile(vcb->ihandle,&vkey.AlphaValCont,ok,&bytes,NULL) ;
		      if (bytes < ok) { v_Msg(NULL,msg,"VSrtReadErr",i+1,vcb->InputFileName,GetLastError()) ; EXITErr ; } ;
		    } ;
#endif
		   vsort_MakeInternalKey(vcb,&vkey,ibptr) ;
		 } ;
		if (i >= vcb->EntryCount)
		 { v_Msg(NULL,msg,"VSrtRecCount",vcb->EntryCount) ; EXITErr ; } ;
		vcb->EntryCount = i ;
		break ;
	   case V_IFT_V4Seq:
		trbuf = (BYTE *)malloc(vcb->InputBytes) ; totalbytes = sizeof sosbh ;
		for(i=0;;vcb->EntryCount++,i++)		/* Have to read each "record" and convert keys */
		 { ibptr = (BYTE *)(vcb->InternalBuffer+(i*vcb->EntryBytes)) ;
		   fpos = (dclFPOS *)ibptr+vcb->EntryPosOffset ;
		   *fpos = totalbytes ;			/* Remember position of this record */
#ifdef POSIXMT
		   ok = fread(&seqbuflen,1,sizeof seqbuflen,vcb->ifp) ; /* First read in number of bytes in record */
		   if (ok == EOF || ok < sizeof seqbuflen) seqbuflen = V4IS_EOFMark_SeqOnly ;	/* If EOF then force V4 eof */
		   if (seqbuflen == V4IS_EOFMark_SeqOnly) break ;		/* Hit EOF ? */
		   totalbytes += (seqbuflen + sizeof seqbuflen) ;
		   ok = fread(trbuf,1,(seqbuflen > V_V4Seq_MaxBytes ? V_V4Seq_MaxBytes : seqbuflen),vcb->ifp) ;
		   if (ok == EOF || ok < (seqbuflen > V_V4Seq_MaxBytes ? V_V4Seq_MaxBytes : seqbuflen))
		    { v_Msg(NULL,msg,"VSrtReadErr",i+1,vcb->InputFileName,errno) ; EXITErr ; } ;
		   if (seqbuflen > V_V4Seq_MaxBytes)
		    { for(ok=seqbuflen-V_V4Seq_MaxBytes;ok>0;ok--)
		       { if (fread(ebuf,1,1,vcb->ifp) != 1) break ; } ; /* Flush until end of record */
		    } ;
#endif
		   vsort_MakeInternalKey(vcb,trbuf,ibptr) ;
		 } ;
		break ;
	   case V_IFT_FixBinBig:
		trbuf = (BYTE *)malloc(vcb->InputBytes) ;
		for(i=0;i<vcb->EntryCount;i++)		/* Have to read each "record" and convert keys */
		 { ibptr = (BYTE *)(vcb->InternalBuffer+(i*vcb->EntryBytes)) ;
		   fpos = (dclFPOS *)ibptr+vcb->EntryPosOffset ;
		   *fpos = i * vcb->InputBytes ;	/* Remember position of this record */
#ifdef POSIXMT
		   bytes = read(vcb->ifd,trbuf,vcb->InputBytes) ;
		   if (bytes != vcb->InputBytes)
		    { v_Msg(NULL,msg,"VSrtReadErr",i+1,vcb->InputFileName,errno) ; EXITErr ; } ;
#endif
#ifdef WINNT
		   ReadFile(vcb->ihandle,trbuf,vcb->InputBytes,&bytes,NULL) ;
		   if (bytes != vcb->InputBytes)
		    { v_Msg(NULL,msg,"VSrtReadErr",i+1,vcb->InputFileName,GetLastError()) ; EXITErr ; } ;
#endif
		   vsort_MakeInternalKey(vcb,trbuf,ibptr) ;
		 } ;
		break ;
	   case V_IFT_Text:
		trbuf = (BYTE *)malloc(vcb->InputBytes) ; totalbytes = 0 ;
		for(i=0;;i++,vcb->EntryCount++)		/* Have to read each "record" and convert keys */
		 { ibptr = (BYTE *)(vcb->InternalBuffer+(i*vcb->EntryBytes)) ;
		   fpos = (dclFPOS *)ibptr+vcb->EntryPosOffset ;
		   *fpos = totalbytes ;			/* Remember current position */
		   memset(trbuf,0,maxoffset) ;		/* Make sure to clear up to last key for variable len input */
		   if (fgets(trbuf,vcb->InputBytes,vcb->ifp) == NULL) break ;
		   totalbytes = ftell(vcb->ifp) ;
		   vsort_MakeInternalKey(vcb,trbuf,ibptr) ;
		 } ;
		break ;
	   case V_IFT_FixBin:
#ifdef POSIXMT
		bytes = read(vcb->ifd,vcb->InternalBuffer,vcb->TotalBytes) ;
#endif
#ifdef WINNT
		ReadFile(vcb->ihandle,vcb->InternalBuffer,vcb->TotalBytes,&bytes,NULL) ;
#endif
		if (bytes != vcb->TotalBytes)
		 { v_Msg(NULL,msg,"VSrtread",i+1,vcb->InputFileName,bytes,vcb->TotalBytes,errno) ; EXITErr ; } ;
		break ;
	 } ;

	if (vcb->Key[0].Type == VSRT_KeyType_V4TallyList)
	 { if (vcb->InputFileType == V_IFT_FixBin)
	    { vsort_HeapSortTallyList(vcb->EntryCount,(struct V4IM__TallySort *)vcb->InternalBuffer) ;
	    } else { qsort(vcb->InternalBuffer,vcb->EntryCount,vcb->EntryBytes,vsrt_KeyCmpTally) ; } ;
	 } else { qsort(vcb->InternalBuffer,vcb->EntryCount,vcb->EntryBytes,vsrt_KeyCompare) ; }

#ifdef POSIXMT
	if ((vcb->ofd = creat(vcb->OutputFileName,0660)) == -1)
	 { v_Msg(NULL,msg,"VSrtOutFile",vcb->OutputFileName,errno) ; EXITErr ; } ;
#endif
#ifdef WINNT
	if (vcb->InputFileType == V_IFT_Text)	/* If a text file then use POSIX routines to write (stream) */
	 { if ((vcb->ofd = UCcreat(vcb->OutputFileName,0660)) == -1)
	    { v_Msg(NULL,msg,"VSrtOutFile",vcb->OutputFileName,errno) ; EXITErr ; } ;
	 } else
	 { vcb->ohandle = UCCreateFile(vcb->OutputFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0) ;
	   if (vcb->ohandle == INVALID_HANDLE_VALUE) { v_Msg(NULL,msg,"VSrtOutFile",vcb->OutputFileName,GetLastError()) ; EXITErr ; } ;
	 } ;
#endif

	switch (vcb->InputFileType)
	 {
	   case V_IFT_FixBinBig:
		for(i=0;i<vcb->EntryCount;i++)		/* Have to get input records & re-write in correct order */
		 { ibptr = (BYTE *)(vcb->InternalBuffer+(i*vcb->EntryBytes)) ;
		   fpos = (dclFPOS *)ibptr+vcb->EntryPosOffset ;
#ifdef POSIXMT
		   if (lseek(vcb->ifd,*fpos,SEEK_SET) == -1)
		    { v_Msg(NULL,msg,"VSrtlseek",i+1,*ibptr,*ibptr*vcb->InputBytes,vcb->InputFileName,errno) ;
		      EXITErr ;
		    } ;
		   bytes = read(vcb->ifd,trbuf,vcb->InputBytes) ;
#endif
#ifdef WINNT
		   if (SetFilePointer(vcb->ihandle,(LONG)*fpos,NULL,FILE_BEGIN) == 0xFFFFFFFF)
		    { v_Msg(NULL,msg,"VSrtSetFilPtr",i+1,*ibptr,*ibptr*vcb->InputBytes,vcb->InputFileName,GetLastError()) ; EXITErr ; } ;
		   ReadFile(vcb->ihandle,trbuf,vcb->InputBytes,&bytes,NULL) ;
#endif
		   if (bytes != vcb->InputBytes)
		    { v_Msg(NULL,msg,"VSrtread",i+1,vcb->InputFileName,bytes,vcb->InputBytes,errno) ;
		      EXITErr ;
		    } ;
#ifdef POSIXMT
		   bytes = write(vcb->ofd,trbuf,vcb->InputBytes) ;	/* Write out the record */
#endif
#ifdef WINNT
		   WriteFile(vcb->ohandle,trbuf,vcb->InputBytes,&bytes,NULL) ;
#endif
		   if (bytes != vcb->InputBytes)
		    { v_Msg(NULL,msg,"VSrtwrite",i+1,vcb->OutputFileName,bytes,vcb->InputBytes,errno) ;
		      EXITErr ;
		    } ;
	         } ;
		break ;
	   case V_IFT_V4Key:
		for(i=0;i<vcb->EntryCount;i++)		/* Have to get input records & re-write in correct order */
		 { ibptr = (BYTE *)(vcb->InternalBuffer+(i*vcb->EntryBytes)) ;
		   if (vcb->KeyCount == 1)		/* If only sorted 1 key then pull right from memory */
		    { vkeyp = (struct lcl__vkey *)(ibptr + vcb->Key[0].Offset2) ;
		      seqbuflen = vkeyp->dkey.KeyP.fld.Bytes + sizeof vkey.dkey.DataId ;
#ifdef POSIXMT
		      bytes = write(vcb->ofd,vkeyp,seqbuflen) ;	/* Write out the record */
#endif
#ifdef WINNT
		      WriteFile(vcb->ohandle,vkeyp,seqbuflen,&bytes,NULL) ;
#endif
		      if (bytes != seqbuflen)
		       { v_Msg(NULL,msg,"VSrtwrite",i+1,vcb->OutputFileName,bytes,seqbuflen,errno) ;
			 EXITErr ;
		       } ;
		      continue ;
		    } ;
		   fpos = (dclFPOS *)ibptr+vcb->EntryPosOffset ;
#ifdef POSIXMT
		   if (lseek(vcb->ifd,*fpos,SEEK_SET) == -1)
		    { v_Msg(NULL,msg,"VSrtlseek",i+1,*ibptr,*ibptr*vcb->InputBytes,vcb->InputFileName,errno) ;
		      EXITErr ;
		    } ;
		   bytes = read(vcb->ifd,&vkey,sizeof vkey.dkey) ;
#endif
#ifdef WINNT
		   if (!SetFilePointer(vcb->ihandle,(LONG)*fpos,NULL,FILE_BEGIN))
		    { v_Msg(NULL,msg,"VSrtlseek",i+1,*ibptr,(*ibptr * vcb->InputBytes),vcb->InputFileName,GetLastError()) ;
		      EXITErr ;
		    } ;
		   ReadFile(vcb->ihandle,&vkey,sizeof vkey.dkey,&bytes,NULL) ;
#endif
		   if (bytes != sizeof vkey.dkey)
		    { v_Msg(NULL,msg,"VSrtread",i+1,vcb->InputFileName,bytes,sizeof vkey.dkey,errno) ;
		      EXITErr ;
		    } ;
		   ok = vkey.dkey.KeyP.fld.Bytes + sizeof vkey.dkey.DataId - sizeof vkey.dkey ;
#ifdef POSIXMT
		   if (ok > 0) bytes = read(vcb->ifd,&vkey.AlphaValCont,ok) ;
#endif
#ifdef WINNT
		   ReadFile(vcb->ihandle,&vkey.AlphaValCont,ok,&bytes,NULL) ;
#endif
		   if (ok > 0 && bytes != ok)
		    { v_Msg(NULL,msg,"VSrtread",i+1,vcb->InputFileName,bytes,ok,errno) ;
		      EXITErr ;
		    } ;
		   seqbuflen = vkey.dkey.KeyP.fld.Bytes + sizeof vkey.dkey.DataId ;
#ifdef POSIXMT
		   bytes = write(vcb->ofd,&vkey,seqbuflen) ;	/* Write out the record */
#endif
#ifdef WINNT
		   WriteFile(vcb->ohandle,&vkey,seqbuflen,&bytes,NULL) ;
#endif
		   if (bytes != seqbuflen)
		    { v_Msg(NULL,msg,"VSrtwrite",i+1,vcb->OutputFileName,bytes,seqbuflen,errno) ;
		      EXITErr ;
		    } ;
	         } ;
		if (vcb->InputBytes == V_V4Seq_MaxBytes) vcb->InputBytes = vcb->EntryBytes ;
		break ;
	   case V_IFT_V4Seq:
		sosbh.BktType = V4_BktHdrType_SeqOnly ; sosbh.BktNum = 0 ;
#ifdef POSIXMT
		write(vcb->ofd,&sosbh,sizeof sosbh) ;	/* Write out V4 Sequential-Only Header */
#endif
#ifdef WINNT
		WriteFile(vcb->ohandle,&sosbh,sizeof sosbh,&bytes,NULL) ;
#endif
		for(i=0;i<vcb->EntryCount;i++)		/* Have to get input records & re-write in correct order */
		 { ibptr = (BYTE *)(vcb->InternalBuffer+(i*vcb->EntryBytes)) ;
		   fpos = (dclFPOS *)ibptr+vcb->EntryPosOffset ;
#ifdef POSIXMT
		   if (lseek(vcb->ifd,*fpos,SEEK_SET) == -1)
		    { v_Msg(NULL,msg,"VSrtlseek",i+1,*ibptr,*ibptr*vcb->InputBytes,vcb->InputFileName,errno) ; EXITErr ; } ;
		   bytes = read(vcb->ifd,&seqbuflen,sizeof seqbuflen) ;
#endif
#ifdef WINNT
		   if (!SetFilePointer(vcb->ihandle,(LONG)*fpos,NULL,FILE_BEGIN))
		    { v_Msg(NULL,msg,"VSrtlseek",i+1,*ibptr,(*ibptr * vcb->InputBytes),vcb->InputFileName,GetLastError()) ; EXITErr ;  } ;
		   ReadFile(vcb->ihandle,&seqbuflen,sizeof seqbuflen,&bytes,NULL) ;
#endif
		   if (bytes != sizeof seqbuflen)
		    { v_Msg(NULL,msg,"VSrtread",i+1,vcb->InputFileName,bytes,sizeof seqbuflen,errno) ;
		      EXITErr ;
		    } ;
#ifdef POSIXMT
		   bytes = read(vcb->ifd,trbuf,seqbuflen) ;
#endif
#ifdef WINNT
		   ReadFile(vcb->ihandle,trbuf,seqbuflen,&bytes,NULL) ;
#endif
		   if (bytes != seqbuflen) { v_Msg(NULL,msg,"VSrtread",i+1,vcb->InputFileName,bytes,seqbuflen,errno) ; EXITErr ; } ;
#ifdef POSIXMT
		   bytes = write(vcb->ofd,&seqbuflen,sizeof seqbuflen) ;
#endif
#ifdef WINNT
		   WriteFile(vcb->ohandle,&seqbuflen,sizeof seqbuflen,&bytes,NULL) ;
#endif
		   if (bytes != sizeof seqbuflen)
		    { v_Msg(NULL,msg,"VSrtwrite",i+1,vcb->OutputFileName,bytes,sizeof seqbuflen,errno) ; EXITErr ; } ;
#ifdef POSIXMT
		   bytes = write(vcb->ofd,trbuf,seqbuflen) ;	/* Write out the record */
#endif
#ifdef WINNT
		   WriteFile(vcb->ohandle,trbuf,seqbuflen,&bytes,NULL) ;
#endif
		   if (bytes != seqbuflen) { v_Msg(NULL,msg,"VSrtwrite",i+1,vcb->OutputFileName,bytes,seqbuflen,errno) ; EXITErr ; } ;
	         } ;
		seqbuflen = V4IS_EOFMark_SeqOnly ;
#ifdef POSIXMT
		write(vcb->ofd,&seqbuflen,sizeof seqbuflen) ;	/* Write out EOF mark */
#endif
#ifdef WINNT
		WriteFile(vcb->ohandle,&seqbuflen,sizeof seqbuflen,&bytes,NULL) ;
#endif
		break ;
	   case V_IFT_Text:
		for(i=0;i<vcb->EntryCount;i++)		/* Have to get input records & re-write in correct order */
		 { ibptr = (BYTE *)(vcb->InternalBuffer+(i*vcb->EntryBytes)) ;
		   fpos = (dclFPOS *)ibptr+vcb->EntryPosOffset ;
		   ipos = *fpos ;
		   if ((int)fseek(vcb->ifp,(FSEEKINT)ipos,SEEK_SET) == -1)
		    { v_Msg(NULL,msg,"VSrtlseek",i+1,*ibptr,(*ibptr * vcb->InputBytes),vcb->InputFileName,errno) ;
		      EXITErr ;
		    } ;
		   if (fgets(trbuf,vcb->InputBytes,vcb->ifp) == NULL) { v_Msg(NULL,msg,"VSrtfgets",i+1,*fpos,vcb->InputFileName,errno) ; EXITErr ; } ;
		   bytes = write(vcb->ofd,trbuf,strlen(trbuf)) ;	/* Write out the record */
		   if (bytes != strlen(trbuf))
		    { v_Msg(NULL,msg,"VSrtwrite",i+1,vcb->OutputFileName,bytes,strlen(trbuf),errno) ;
		      EXITErr ;
		    } ;
	         } ;
		break ;
	   case V_IFT_FixBin:
#ifdef POSIXMT
		write(vcb->ofd,vcb->InternalBuffer,vcb->TotalBytes) ;
#endif
#ifdef WINNT
		WriteFile(vcb->ohandle,vcb->InternalBuffer,vcb->TotalBytes,&bytes,NULL) ;
#endif
		break ;
	 } ;
#ifdef POSIXMT
	close(vcb->ofd) ;
	close(vcb->ifd) ; if (vcb->ifp != NULL) fclose(vcb->ifp) ;
	if (vcb->DeleteInput) { UCremove(vcb->InputFileName) ; } ;
#endif
#ifdef WINNT
	if (vcb->InputFileType == V_IFT_Text)
	 { close(vcb->ofd) ;
	 } else { CloseHandle(vcb->ohandle) ; CloseHandle(vcb->ihandle) ; } ;
	if (vcb->DeleteInput) { UCremove(vcb->InputFileName) ; } ;
#endif
	if (!vcb->Quiet)
	 { 
	   double deltaWall ;
#ifdef WINNT
	   B64INT t1,t2 ;
	   GetSystemTimeAsFileTime(&ftExit) ;
	   memcpy(&t1,&ftCreate,sizeof t1) ; memcpy(&t2,&ftExit,sizeof t2) ;
	   deltaWall = (t2 - t1) / 10000000.0 ;
#else
	   gettimeofday (&endTime,NULL) ;
	   deltaWall = ((endTime.tv_sec - bgnTime.tv_sec)* 1000.0 + (endTime.tv_usec - bgnTime.tv_usec)/1000.0) / 1000.0 ;
#endif
	   v_Msg(NULL,msg,"VSrtDone",vcb->EntryCount,vcb->InputBytes,vcb->EntryBytes,(vcb->TotalBytes+500)/1000,vcb->TotalCompares,
		   ((double)(clock() -  cpuseconds) / (double)CLOCKS_PER_SEC),deltaWall) ; 
	   vout_UCText(VOUT_Status,0,msg) ;
	 } ;

	EXITOk ;				/* That was easy! */

err_arg: v_Msg(NULL,msg,"VSrtArgNotSwtch",arg) ; EXITErr ;

err_int: v_Msg(NULL,msg,"VSrtArgNotInt",arg) ; EXITErr ;


}

void vsort_MakeInternalKey(vcb,vtrbuf,vibptr)
  struct VSRT__ControlBlock *vcb ;
  void *vtrbuf ;			/* Used as temp record buffer */
  void *vibptr ;
{ short *sptr1,*sptr2 ; int *iptr1,*iptr2 ; B64INT *lptr1,*lptr2 ; char *aptr1,*aptr2 ; char *vptr1,*vptr2 ; double *dptr1,*dptr2 ; UCCHAR *uptr1,*uptr2 ;
  char *trbuf,*ibptr ;
  int k ;

	trbuf = (char *)vtrbuf ; ibptr = (char *)vibptr ;
	
	for(k=0;k<vcb->KeyCount;k++)			/* Now copy each key into InternalBuffer */
	 { switch (vcb->Key[k].Type)
	    { default: printf("? Key (%d) - Invalid type (%d)\n",k,vcb->Key[k].Type) ; exit(EXITABORT) ;
	      case VSRT_KeyType_Short:
		sptr1 = (short *)(trbuf + vcb->Key[k].Offset1) ; sptr2 = (short *)(ibptr + vcb->Key[k].Offset2) ;
		*sptr2 = *sptr1 ; break ;
	      case VSRT_KeyType_Int:
		iptr1 = (int *)(trbuf + vcb->Key[k].Offset1) ; iptr2 = (int *)(ibptr + vcb->Key[k].Offset2) ;
		*iptr2 = *iptr1 ; break ;
	      case VSRT_KeyType_V4TallyList:
	      case VSRT_KeyType_Long:
		lptr1 = (B64INT *)(trbuf + vcb->Key[k].Offset1) ; lptr2 = (B64INT *)(ibptr + vcb->Key[k].Offset2) ;
		*lptr2 = *lptr1 ; break ;
	      case VSRT_KeyType_Key:
	      case VSRT_KeyType_Alpha:
		aptr1 = (char *)(trbuf + vcb->Key[k].Offset1) ; aptr2 = (char *)(ibptr + vcb->Key[k].Offset2) ;
		memcpy(aptr2,aptr1,vcb->Key[k].Bytes) ; break ;
	      case VSRT_KeyType_UCChar:
		uptr1 = (UCCHAR *)(trbuf + vcb->Key[k].Offset1) ; uptr2 = (UCCHAR *)(ibptr + vcb->Key[k].Offset2) ;
		memcpy(uptr2,uptr1,(vcb->Key[k].Bytes * sizeof(UCCHAR))) ; break ;
	      case VSRT_KeyType_VAlpha:
		vptr1 = (char *)(trbuf + vcb->Key[k].Offset1) ; vptr2 = (char *)(ibptr + vcb->Key[k].Offset2) ;
		strncpy(vptr2,vptr1,vcb->Key[k].Bytes) ; break ;
	      case VSRT_KeyType_Double:
		dptr1 = (double *)(trbuf + vcb->Key[k].Offset1) ; dptr2 = (double *)(ibptr + vcb->Key[k].Offset2) ;
//if (((int)dptr1 & 0x7) != 0 || ((int)dptr2 & 0x7 != 0)) printf("%d: %p + %d = %p, %p + %d = %p\n",k,trbuf,vcb->Key[k].Offset2,dptr1,ibptr,vcb->Key[k].Offset2,dptr2) ;
		*dptr2 = *dptr1 ; break ;
	    } ;
	 } ;
}



/*	Q (u i c k )   S O R T   C O D E		*/
//#include "qsortcode.c"

//#include <sys/cdefs.h>

//#include <sys/param.h>
//#include <sys/libkern.h>

typedef int             cmp_t(const void *, const void *);
static __inline char    *med3(char *, char *, char *);
static __inline void     swapfunc(char *, char *, int, int);

#define qsmin(a, b)       (a) < (b) ? (a) : (b)

#define swapcode(TYPE, parmi, parmj, n) {               \
        long i = (n) / sizeof (TYPE);                   \
        register TYPE *pi = (TYPE *) (parmi);           \
        register TYPE *pj = (TYPE *) (parmj);           \
        do {                                            \
                register TYPE   t = *pi;                \
                *pi++ = *pj;                            \
                *pj++ = t;                              \
        } while (--i > 0);                              \
}

#define SWAPINIT(a, es) swaptype = ((char *)a - (char *)0) % sizeof(long) || \
        es % sizeof(long) ? 2 : es == sizeof(long)? 0 : 1;

static __inline void swapfunc(char *a, char *b, int n, int swaptype)
{	if (swaptype <= 1) swapcode(long, a, b, n)
	  else swapcode(char, a, b, n) ;
}

#define swap(a, b)                                      \
 if (swaptype == 0)                            \
  { long t = *(long *)(a);                  \
    *(long *)(a) = *(long *)(b);            \
    *(long *)(b) = t;                       \
  } else swapfunc(a, b, es, swaptype)

#define vecswap(a, b, n) if ((n) > 0) swapfunc(a, b, n, swaptype)

//#define CMP(x, y) (cmp((x), (y)))

#define CMP(x,y) ( *((B64INT *)(x)) - *((B64INT *)(y)) )

static __inline char *
med3(char *a, char *b, char *c)
{
        return CMP(a, b) < 0 ? (CMP(b, c) < 0 ? b : (CMP(a, c) < 0 ? c : a )) : (CMP(b, c) > 0 ? b : (CMP(a, c) < 0 ? a : c )) ;
}

void qsortxxx(void *a, size_t n, size_t es)
{
        char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
        int d, r, swaptype, swap_cnt;
loop:   SWAPINIT(a, es);
        swap_cnt = 0;
        if (n < 7)
         { for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
            { for (pl = pm; pl > (char *)a && CMP(pl - es, pl) > 0;pl -= es) swap(pl, pl - es);
            } ;
           return;
         } ;
        pm = (char *)a + (n / 2) * es;
        if (n > 7)
         { pl = a; pn = (char *)a + (n - 1) * es;
           if (n > 40)
            { d = (n / 8) * es;
              pl = med3(pl, pl + d, pl + 2 * d);pm = med3(pm - d, pm, pm + d); pn = med3(pn - 2 * d, pn - d, pn);
            } ;
           pm = med3(pl, pm, pn);
         } ;
        swap(a, pm);
        pa = pb = (char *)a + es;

        pc = pd = (char *)a + (n - 1) * es;
        for (;;)
         {
           while (pb <= pc && (r = CMP(pb, a)) <= 0)
            { if (r == 0) { swap_cnt = 1; swap(pa, pb); pa += es; } ;
              pb += es;
            }
           while (pb <= pc && (r = CMP(pc, a)) >= 0)
            { if (r == 0) { swap_cnt = 1; swap(pc, pd); pd -= es; } ;
              pc -= es;
            } ;
           if (pb > pc) break;
           swap(pb, pc);
           swap_cnt = 1;
           pb += es;
           pc -= es;
         } ;
        if (swap_cnt == 0)	/* Switch to insertion sort */
         { for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
            { for (pl = pm; pl > (char *)a && CMP(pl - es, pl) > 0;pl -= es) swap(pl, pl - es); } ;
           return;
         } ;

        pn = (char *)a + n * es;
        r = qsmin(pa - (char *)a, pb - pa);
        vecswap(a, pb - r, r);
        r = qsmin(pd - pc, pn - pd - es);
        vecswap(pb, pn - r, r);
        if ((r = pb - pa) > es) qsortxxx(a, r / es, es);
        if ((r = pd - pc) > es)		/* Iterate rather than recurse to save stack space */
         { a = pn - r ; n = r / es;
           goto loop;
         } ;
}

/*	H E A P   S O R T   C O D E		*/

void vsort_HeapSortTallyList(entries,vtsArray)
  int entries ;
  struct V4IM__TallySort vtsArray[] ;
{
	int i,ir,j,l;
	struct V4IM__TallySort tvts;

	if (entries < 2) return;
	l=(entries >> 1)+1;
	ir=entries;
	for (;;) {
		if (l > 1) {
			tvts=vtsArray[--l - 1];
		} else {
			tvts=vtsArray[ir - 1];
			vtsArray[ir - 1]=vtsArray[1 - 1];
			if (--ir == 1) {
				vtsArray[1 - 1]=tvts;
				break;
			}
		}
		i=l;
		j=l+l;
#define vtsLT(vts1,vts2) (vts1.HashX < vts2.HashX ? TRUE : (vts1.HashX == vts2.HashX ? vts1.Order < vts2.Order : FALSE))
		while (j <= ir) {
			if (j < ir && vtsLT(vtsArray[j - 1],vtsArray[j+1 - 1])) j++;
			if (vtsLT(tvts,vtsArray[j - 1])) {
				vtsArray[i - 1]=vtsArray[j - 1];
				i=j;
				j <<= 1;
			} else j=ir+1;
		}
		vtsArray[i - 1]=tvts;
	}
}
/* (C) Copr. 1986-92 Numerical Recipes Software 6)#41jQ.n.. */
