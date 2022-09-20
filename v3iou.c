/*	V3IOU.C - V3 I/O UTILITIES

	LAST EDITED 6/28/84 BY VICTOR E. HANSEN		*/

#include <signal.h>
#include <time.h>
#define NEED_SOCKET_LIBS 1
#include "v3defs.c"
#include <errno.h>

#ifdef VMSOS
#include rms		/* Include rms defs if for VAX/VMS */
#endif

#ifdef UNIX
#include <termio.h>
#include <sys/socket.h>
#endif

#ifdef HPUX
#include <sys/fcntl.h>
#endif

#ifdef ALPHAOSF
#include <sys/mode.h>
#include <sys/time.h>
#endif

#ifdef LINUX486
#include <sys/stat.h>
#include <sys/timex.h>
#endif

#ifdef WINNT
#include <windows.h>
#include <winsock.h>
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EINPROGRESS WSAEINPROGRESS
#endif

#ifdef v4is
struct iou__unit *v3unitp ;		/* Holds pointer to last called unit (for error handling) */
struct V4MM__MemMgmtMaster *v4mm_GetMMMPtr() ;
#define V4_GLOBAL_MMM_PTR v4mm_GetMMMPtr()
char *v3_logical_decoder() ;
#endif

#ifdef cisam
#include <time.h>
#include <isam.h>
/*	Declare special strucure to hold all keys for C-ISAM file */
#define CISAM_MAX_KEYS 7	/* Max number of keys in C-ISAM file */
struct cisam__keys
 { int count ;		/* Number of keys */
   struct keydesc key[CISAM_MAX_KEYS] ;
 } ;
#endif /* CISAM */

/*	Linkage to rest of V3 system */
int termSocket = 0 ;		/* If nonzero then socket to use for terminal I/O */
extern struct db__process *process ;
extern struct db__psi *psi ;
extern struct iou__openlist ounits ;
extern int ctrlp_interrupt ;

#define IO_SET_STATIC 1		/* To initialize "static" IO unit */
#define IO_GET_STATIC 2		/* For GET static */
#define IO_PUT_STATIC 4		/* For PUT static */
#define IO_UPDATE_STATIC 8	/* For PUT/Update Static */
#define IO_NULL_STATIC 16	/* For NL: device */

/*	Compaction/Expansion Buffer				*/
char *compact_buffer = 0 ;

/*	I / O   O P E N   R O U T I N E				*/

#ifdef WINNT
  static int didWSAStartup = FALSE ;
#endif

/*	iou_open - Opens a V3 unit				*/

void iou_open(unitp)
  struct iou__unit *unitp ;
{
#define V3_VMS_ERR_BUF_MAX 250
#ifdef cisam
   struct dictinfo info ;
   struct cisam__keys *ckeys ;
   int mode ;
#endif
#ifdef v4is
   struct V4IS__ParControlBlk *pcb ;
   struct V4H__FixHash vfh ;
   struct V4IS__OptControlBlk vocb ;
   struct V4IS__OptControlBlk *vocbp = NULL ;
#endif
#ifdef USOCKETS
   struct sockaddr_un saun,fsaun ;
#define SOCKETS 1
#endif
#ifdef ISOCKETS
   struct sockaddr_in sin,fsin ;
#define SOCKETS 1
#endif
#ifdef SOCKETS
   struct hostent *hp ;
   char hostname[128] ; char *bp ; int fromlen ;
#endif
#ifdef WINNT
  OFSTRUCT ofs ;
  WSADATA wsad ;
#endif
   char tmp[10],errbuf[V3_VMS_ERR_BUF_MAX] ; char namebuf[200] ;
   char xnam[V3_IO_FILENAME_MAX] ;	/* Holds expanded file name */
   int rms_res ;			/* Result of system call */
   int i ; char *b ;

/*	Maybe call v3 module to handle IO */
	i = (unitp->file_info & V3_FLAGS_IOF_NOHANDLER) != 0 ; unitp->file_info &= ~V3_FLAGS_IOF_NOHANDLER ;
	if (unitp->handler_mep != NULL ? !i : FALSE)	/* Call v3 module to handle I/O ? */
	 { PUSHVI(0) ; PUSHF(V3_FORMAT_EOA) ;
	   PUSHMP(unitp) ; PUSHF(V3_FORMAT_POINTER) ; PUSHINT(1) ;
	   xctu_call_mod(unitp->handler_mep) ;
	   return ;
	 } ;
/*	Make sure info is here */
	if (unitp->v3_name[0] == NULLV) strcpy(unitp->v3_name,"NONAME") ;
	if (unitp->file_name[0] == NULLV)
	 v3_error(V3E_NONAME,"IO","OPEN","NONAME","No file name has been specified",unitp) ;
/*	Going to a null (NL:) device ? */
	if ((strcmp("nl:",unitp->file_name) == 0) || (strcmp("NL:",unitp->file_name) == 0))
	 { unitp->is_static = IO_NULL_STATIC ; return ; } ;
	if (unitp->name_dflt_format.all != NULLV)
	 { b = unitp->file_name ;	/* Make a guess as to whether or not this already has an extension */
	   for(i=FALSE;*b!=0;b++) { if (*b=='.') { i=TRUE ; } else if (*b=='/') { i=FALSE ; } ; } ;
	   if (i == FALSE)	/* If no extension then give it one */
	    { b = (char *)unitp->name_dflt_ptr ;	/* Pointer to default (assume it's an extension) */
	      i = (unitp->name_dflt_format.fld.type == VFT_FIXSTR || unitp->name_dflt_format.fld.type == VFT_FIXSTRSK
			? unitp->name_dflt_format.fld.length : strlen(b) ) ;
	      strncat(unitp->file_name,b,i) ;
	    } ;
	 } ;
/*	Figure out type of handler */
	if ((unitp->file_info & V3_FLAGS_IOF_C) != 0) goto c_handler ;
	if ((unitp->file_info & V3_FLAGS_IOF_UNIXSOCKET) != 0) goto unixsocket_handler ;
	if ((unitp->file_info & V3_FLAGS_IOF_INETSOCKET) != 0) goto inetsocket_handler ;
#ifdef v4is
#ifndef VMSOS
	unitp->file_info |= V3_FLAGS_IOF_V4IS ;			/* If not VMS then force into V4IS */
#endif
	if ((unitp->file_info & V3_FLAGS_IOF_V4IS) != 0)
	 { pcb = (struct V4IS__ParControlBlk *)v4mm_AllocChunk(sizeof *pcb,FALSE) ;			/* Allocate a PCB */
	   v3unitp = unitp ;
	   memset(pcb,0,sizeof *pcb) ;				/* Clear it out */
	   unitp->file_ptr = (FILE *)pcb ;			/* Save for later */
	   strcpy(pcb->V3name,unitp->v3_name) ;			/* Start copying stuff, translating where necessary */
	   strcpy(unitp->file_name,v3_logical_decoder(unitp->file_name,((unitp->file_info & V3_FLAGS_IOF_CREATE) == 0))) ;
	   UCstrcpy(pcb->UCFileName,ASCretUC(unitp->file_name)) ;
	   if (unitp->file_info & (V3_FLAGS_IOF_UPDATE | V3_FLAGS_IOF_PUT | V3_FLAGS_IOF_DELETE))
	    { pcb->OpenMode = V4IS_PCB_OM_Update ; } else pcb->OpenMode = V4IS_PCB_OM_Read ;
#define FIF(FLAG,V4MODE) if ( (unitp->file_info & FLAG) != 0) pcb->OpenMode = V4MODE ;
	   FIF(V3_FLAGS_IOF_CREATE,V4IS_PCB_OM_New) FIF(V3_FLAGS_IOF_CREATE_IF,V4IS_PCB_OM_NewIf)
	   FIF(V3_FLAGS_IOF_DELETE_FILE,V4IS_PCB_OM_NewTemp) FIF(V3_FLAGS_IOF_NEW_FILE,V4IS_PCB_OM_MustBeNew)
	   if (unitp->file_info & V3_FLAGS_IOF_DISCONNECTonCLOSE) pcb->AreaFlags |= V4IS_PCB_OF_DisconnectOnClose ;
#define FAF(FLAG,V4MODE) if (unitp->file_info & FLAG) pcb->AccessMode |= V4MODE ;
	   pcb->AccessMode = 0 ; FAF(V3_FLAGS_IOF_UPDATE,V4IS_PCB_AM_Update) FAF(V3_FLAGS_IOF_GET,V4IS_PCB_AM_Get)
	   FAF(V3_FLAGS_IOF_INDEXED,V4IS_PCB_AMF_Indexed)	/* Create Indexed file (rather than sequential only */
	   FAF(V3_FLAGS_IOF_PUT,V4IS_PCB_AM_Insert) FAF(V3_FLAGS_IOF_DELETE,V4IS_PCB_AM_Delete)
	   if ((pcb->AccessMode & V4IS_PCB_AMF_Indexed) == 0) pcb->AccessMode |= V4IS_PCB_AMF_Sequential ;
#define LAF(FLAG,V4MODE) if (unitp->share_info & FLAG) pcb->LockMode |= V4MODE ;
	   pcb->LockMode = 0 ; LAF(V3_FLAGS_IOF_UPDATE,V4IS_PCB_LM_Update) LAF(V3_FLAGS_IOF_GET,V4IS_PCB_LM_Get)
	   LAF(V3_FLAGS_IOF_PUT,V4IS_PCB_LM_Insert) LAF(V3_FLAGS_IOF_DELETE,V4IS_PCB_LM_Delete)
	   pcb->MaxRecordLen = unitp->file_record_length ;
	   if (unitp->record_info & V3_FLAGS_IOR_COMPRESSED)
	    { pcb->MinCmpBytes = 50 ; } else pcb->MinCmpBytes = 250 ;	/* Always compress over 250 bytes (XXXXXXXX) */
	   pcb->KeyInfo = (struct V4FFI__KeyInfo *)unitp->keydesc_ptr ;	/* Maybe copy pointer to key info */
	   if (unitp->file_info & V3_FLAGS_IOF_HASHED)			/* Hashed file ? */
	    { vocb.ocbType = V4IS_OCBType_Hash ; vocb.Nextocb = NULL ; vocb.ocbPtr = (char *)&vfh ;
	      memset(&vfh,0,sizeof(vfh)) ;
	      vfh.HashType = V4H_HashType_Fixed ;
	      vfh.MaxRecords = unitp->put_count ; unitp->put_count = 0 ;
	      vocbp = &vocb ;
	    } ;
	   v4is_Open(pcb,vocbp,NULL) ;
/*	   Check for an Area FileRef - if none then use first defined in area */
	   if (unitp->dflt_fileref == 0 && unitp->fileref == 0)
	    { if (pcb->KeyInfo != NULL) unitp->dflt_fileref = pcb->FileRef ;
	    } else unitp->fileref = 0 ;
	   unitp->file_record_length = pcb->GetLen ;		/* Copy max record length */
	   if (getcdf(unitp->file_name,namebuf,sizeof namebuf) !=NULLV) strcpy(unitp->file_name,namebuf) ;
	   if (ounits.count < V3_IO_UNIT_LIST)
	    { ounits.unit[ounits.count].unitp = unitp ; strcpy(ounits.unit[ounits.count++].file_name,unitp->file_name) ; } ;
	   v3unitp = NULL ; return ;
	 } ;
#endif
#ifdef VMSOS
	vms_open(unitp) ; return ;				/* Call module to handle RMS interface */
#endif
#ifdef cisam
/*	Maybe open a C-ISAM file ? */
/*	Strip off trailing extension - C-ISAM provides its own */
	strcpy(unitp->file_name,v3_logical_decoder(unitp->file_name,((unitp->file_info & V3_FLAGS_IOF_CREATE) == 0))) ;
	b = (char *)strrchr(unitp->file_name,'.') ; if (b != NULLV) *b = NULLV ;
	mode = 0 ; unitp->lock_ind = FALSE ;
	if ((unitp->file_info & V3_FLAGS_IOF_GET) != 0)
	 { mode = ( (unitp->file_info & (V3_FLAGS_IOF_PUT | V3_FLAGS_IOF_UPDATE)) != 0 ? ISINOUT : ISINPUT) ;
	 } else mode = ISOUTPUT ;
/*	Are we creating a new file or opening an existing one ? */
	if ((unitp->file_info & V3_FLAGS_IOF_CREATE_IF) != 0)
	 { unitp->file_ptr = (FILE *)isopen(unitp->file_name,mode+ISMANULOCK) ;
	   if ((int)unitp->file_ptr >= 0) { isclose(unitp->file_ptr) ; }
	    else {
		   if (iserrno == 2) { unitp->file_info |= V3_FLAGS_IOF_CREATE ; }
#ifdef ISVARLEN
		    else { unitp->file_ptr = (FILE *)isopen(unitp->file_name,mode+ISMANULOCK+ISVARLEN) ;
			   if ((int)unitp->file_ptr >= 0) { isclose(unitp->file_ptr) ; }
			    else iou_cisam_error("OPEN",unitp) ;
			 } ;
#else
		    else iou_cisam_error("OPEN",unitp) ;
#endif
		  } ;
	  } ;
	if ((unitp->file_info & (V3_FLAGS_IOF_CREATE)) != 0)
	 { ckeys = (struct cisam__keys *)unitp->keydesc_ptr ;	/* Keys set up via IO_MISC */
	   mode = ISINOUT + ISEXCLLOCK ;
/*	   Maybe set up for CISAM variable length records */
#ifdef ISVARLEN
	   if (unitp->over_length > 0 ) mode |= ISVARLEN ; isreclen = unitp->over_length ;
#endif
	   unitp->file_ptr = (FILE *)isbuild(unitp->file_name,unitp->file_record_length,&ckeys->key[0],mode) ;
	   if ((int)unitp->file_ptr < 0)
	    { if (iserrno == 17 && ((unitp->file_info & V3_FLAGS_IOF_CREATE) != 0))
	       { if (iserase(unitp->file_name) < 0) iou_cisam_error("OPEN",unitp) ;
	   	 unitp->file_ptr = (FILE *)isbuild(unitp->file_name,unitp->file_record_length,&ckeys->key[0],mode) ;
	       } ;
	    } ; if ((int)unitp->file_ptr < 0) iou_cisam_error("OPEN",unitp) ;
/*	   Define all additional keys */
	   for(i=1;i<ckeys->count;i++)
	    { if (isaddindex(unitp->file_ptr,&ckeys->key[i]) < 0) iou_cisam_error("OPEN",unitp) ; } ;
	 } else
	 { unitp->file_ptr = (FILE *)isopen(unitp->file_name,mode+ISMANULOCK) ;
#ifdef ISVARLEN
	   if ((int)unitp->file_ptr < 0)
	    { unitp->file_ptr = (FILE *)isopen(unitp->file_name,mode+ISMANULOCK+ISVARLEN) ; } ;
#endif
	   if ((int)unitp->file_ptr < 0 ) iou_cisam_error("OPEN",unitp) ;
	 } ;
/*	Now get all of the keys for this file */
	ckeys = (struct cisam__keys *)(unitp->keydesc_ptr = (int *)v4mm_AllocChunk(sizeof *ckeys,FALSE)) ;
	isindexinfo(unitp->file_ptr,&info,0) ;	/* Get master dictionary */
	unitp->get_length = info.di_recsize ;	/* Plug input record length (fixed length therefore always the same!) */
	for(i=1;i<=(info.di_nkeys & 0xF);i++)
	 { if (isindexinfo(unitp->file_ptr,&ckeys->key[i-1],i) < 0) iou_cisam_error("OPEN",unitp) ; } ;
	ckeys->count = (info.di_nkeys & 0xF) ;		/* Mask out most significant bit (and others) for VARLEN kludge */
#ifdef ISVARLEN
	unitp->over_length = isreclen ;			/* Save minimum size of record */
#else
	unitp->over_length = 0 ;
#endif
/*	Any errors ? */
	if ((int)unitp->file_ptr < 0 ) iou_cisam_error("OPEN",unitp) ;
/*	All done C-ISAM open - looks good */
	if (getcdf(unitp->file_name,namebuf,sizeof namebuf) !=NULLV) strcpy(unitp->file_name,namebuf) ;
	if (ounits.count < V3_IO_UNIT_LIST)
	 { ounits.unit[ounits.count].unitp = unitp ; strcpy(ounits.unit[ounits.count++].file_name,unitp->file_name) ; } ;
	return ;
#endif
/*	Here to open straight C sequential file */
c_handler:
	tmp[0] = NULLV ;
	if ((unitp->file_info & V3_FLAGS_IOF_GET) != 0)
	 { strcpy(tmp,((unitp->record_info & V3_FLAGS_IOR_CARRIAGE_RETURN) ? "r" : "rb")) ;
#ifdef WINNTxxvv
	   i = ((unitp->record_info & V3_FLAGS_IOR_CARRIAGE_RETURN) ? -1 : OF_READ) ;
#endif
	 } ;
	if ((unitp->file_info & V3_FLAGS_IOF_PUT) != 0 )
	 { strcpy(tmp,((unitp->record_info & V3_FLAGS_IOR_CARRIAGE_RETURN) ? "w" : "wb")) ;
#ifdef WINNTxxvv
	   i = OF_CREATE+OF_WRITE ;
#endif
	 } ;
	if ((unitp->file_info & V3_FLAGS_IOF_APPEND) != 0)
	 { strcpy(tmp,((unitp->record_info & V3_FLAGS_IOR_CARRIAGE_RETURN) ? "a" : "ab")) ;
#ifdef WINNTxxvv
	   i = -1 ;
#endif
	 } ;
	if (tmp[0] == NULLV)
	 v3_error(V3E_BADMODE,"IO","OPEN","BADMODE","Invalid mode for C handler",unitp) ;
	strcpy(unitp->file_name,v3_logical_decoder(unitp->file_name,((unitp->file_info & V3_FLAGS_IOF_CREATE) == 0))) ;
#ifdef WINNTxxvv
	if (i != -1)
	 { ofs.cBytes = sizeof ofs ;
	   if ((unitp->hfile = OpenFile(unitp->file_name,&ofs,i)) == HFILE_ERROR)
	    { sprintf(errbuf,"Error (%s) opening file (%s)",v_OSErrString(GetLastError()),unitp->file_name) ;
	      v3_error(V3E_FILEOPEN,"IO","OPEN","FILEOPEN",errbuf,unitp) ;
	    } ;
	   strcpy(unitp->file_name,ofs.szPathName) ;
	   unitp->file_ptr = (FILE *)1 ; goto end_open ;
	 } else { unitp->hfile = HFILE_ERROR ; } ;
#endif
	if ((unitp->file_ptr = fopen(unitp->file_name,tmp)) == NULLV)
	 { sprintf(errbuf,"Error (%s) opening file (%s)",v_OSErrString(errno),unitp->file_name) ;
	   v3_error(V3E_FILEOPEN,"IO","OPEN","FILEOPEN",errbuf,unitp) ;
	 } ;
/*	Looks ok */
#ifdef VMSOSxx
	fgetname(unitp->file_ptr,unitp->file_name,0) ;
#else
	if (getcdf(unitp->file_name,namebuf,sizeof namebuf) !=NULLV) strcpy(unitp->file_name,namebuf) ;
#endif
end_open:
	if (ounits.count < V3_IO_UNIT_LIST)
	 { ounits.unit[ounits.count].unitp = unitp ; strcpy(ounits.unit[ounits.count++].file_name,unitp->file_name) ; } ;
	return ;

/*	Here to open Unix Socket */
unixsocket_handler:
#ifdef USOCKETS
	if ((unitp->PrimarySocket = socket(AF_UNIX, SOCK_STREAM,0)) < 0)
	 { sprintf(errbuf,"Error (%s) creating Unix Socket (%s)",v_OSErrString(errno),unitp->file_name) ;
	   v3_error(V3E_SOCKETMAKE,"IO","OPEN","SOCKETMAKE",errbuf,unitp) ;
	 } ;
	memset(&saun,0,sizeof saun) ;
	strcpy(unitp->file_name,v3_logical_decoder(unitp->file_name,FALSE)) ;
	saun.sun_family = AF_UNIX ; strcpy(saun.sun_path,unitp->file_name) ;
	if ((unitp->file_info & V3_FLAGS_IOF_SERVER) != 0)
	 { remove(unitp->file_name) ;				/* Remove any existing server name */
	   if (bind(unitp->PrimarySocket,&saun,sizeof saun.sun_family + strlen(saun.sun_path)) < 0)
	    { sprintf(errbuf,"Error (%s) bind'ing Unix Socket to name (%s)",v_OSErrString(errno),unitp->file_name) ;
	      v3_error(V3E_SOCKETBIND,"IO","OPEN","SOCKETBIND",errbuf,unitp) ;
	    } ;
	   if (listen(unitp->PrimarySocket,5) < 0)
	    { sprintf(errbuf,"Error (%s) listen'ing on Unix Socket (%s)",v_OSErrString(errno),unitp->file_name) ;
	      v3_error(V3E_SOCKETLISTEN,"IO","OPEN","SOCKETLISTEN",errbuf,unitp) ;
	    } ;
	   fromlen = sizeof fsaun ;
	   if ((unitp->ConnectSocket = accept(unitp->PrimarySocket,&fsaun,&fromlen)) < 0)
	    { sprintf(errbuf,"Error (%s) accept'ing on Unix Socket (%s)",v_OSErrString(errno),unitp->file_name) ;
	      v3_error(V3E_SOCKETACCEPT,"IO","OPEN","SOCKETACCEPT",errbuf,unitp) ;
	    } ;
	 } else		/* else create client socket */
	 {
	   if (connect(unitp->PrimarySocket,&saun,sizeof saun.sun_family + strlen(saun.sun_path)) < 0)
	    { sprintf(errbuf,"Error (%s) connect'ing Unix Socket to name (%s)",v_OSErrString(errno),unitp->file_name) ;
	      v3_error(V3E_SOCKETCONNECT,"IO","OPEN","SOCKETCONNECT",errbuf,unitp) ;
	    } ;
	   unitp->ConnectSocket = unitp->PrimarySocket ;
	 } ;
	unitp->file_ptr = (FILE *)0 ;				/* Set up dummy file pointer */
	return ;
#else
	v3_error(V3E_SOCKETNOTSUP,"V3","OPEN","SOCKETNOTSUP","Sockets not supported in this version of V3",0) ;
#endif
/*	Here to open Internet Socket */
inetsocket_handler:
#ifdef ISOCKETS
#ifdef WINNT
	if (!didWSAStartup)
	 { didWSAStartup = TRUE ;
	   i = MAKEWORD(1,1) ;		/* Always accept highest version */
	   WSAStartup(i,&wsad) ;
	 } ;
#endif
	if ((unitp->PrimarySocket = socket(AF_INET, SOCK_STREAM,0)) < 0)
	 { sprintf(errbuf,"Error (%s) creating INET Socket (%s)",v_OSErrString(errno),unitp->file_name) ;
	   v3_error(V3E_SOCKETMAKE,"IO","OPEN","SOCKETMAKE",errbuf,unitp) ;
	 } ;
	if ((unitp->file_info & V3_FLAGS_IOF_NOBLOCK) != 0)
	 { i = 1 ;
	   if (NETIOCTL(unitp->PrimarySocket,FIONBIO,&i) != 0)
	    v3_error(V3E_CLOSESOCKETFAIL,"IO","OPEN","NOBLOCKSOCKET","Could not set socket to NOBLOCK",unitp) ;
	 } ;
	memset(&sin,0,sizeof sin) ;
	sin.sin_family = AF_INET ;
/*	Check file_name for host/port number, if no host then use current host */
	strcpy(unitp->file_name,v3_logical_decoder(unitp->file_name,FALSE)) ;
	bp = strchr(unitp->file_name,'/') ;			/* Look for host delimiter */
	if (bp == NULLV)
	 { sin.sin_port = htons(atoi(unitp->file_name)) ;
	   gethostname(hostname,sizeof(hostname)) ;		/* Get host name */
	   hp = gethostbyname(hostname) ;
	 } else
	 { sin.sin_port = htons(atoi(bp+1)) ;			/* Convert after '/' as port number */
	   i = bp - unitp->file_name ;				/* Length of host name */
	   strncpy(hostname,unitp->file_name,i) ; hostname[i] = 0 ;
	   hp = gethostbyname(hostname) ;
	 } ;
	if (hp != NULL) { memcpy(&sin.sin_addr,hp->h_addr,hp->h_length) ; }
	 else { sin.sin_addr.s_addr = inet_addr(hostname) ;
		if (sin.sin_addr.s_addr == -1)
		 { if ((unitp->file_info & V3_FLAGS_IOF_SERVER) != 0)
		    { sin.sin_addr.s_addr = htonl(INADDR_ANY) ; } /* If a server then don't really care */
		    else { sprintf(errbuf,"Error (%s) obtaining network address of host (gethostbyname/addr(%s))",v_OSErrString(errno),hostname) ;
			   v3_error(V3E_GETHOSTADDR,"IO","OPEN","GETHOSTADDR",errbuf,unitp) ;
			 } ;
		 } ;
	      } ;
	
	if ((unitp->file_info & V3_FLAGS_IOF_SERVER) != 0)
	 { char trueOpt = TRUE ;
	   if (setsockopt(unitp->PrimarySocket,SOL_SOCKET,SO_REUSEADDR,&trueOpt,sizeof i) < 0)
	    { sprintf(errbuf,"Error (%s) in setsockopt(SO_REUSEADDR) INET Socket to name (%s)",v_OSErrString(NETERROR),unitp->file_name) ;
	      v3_error(V3E_SOCKETBIND,"IO","OPEN","SETSOCKOPT",errbuf,unitp) ;
	    } ;
	   if (bind(unitp->PrimarySocket,(struct sockaddr *)&sin,sizeof sin) < 0)
	    { sprintf(errbuf,"Error (%s) bind'ing INET Socket to name (%s)",v_OSErrString(NETERROR),unitp->file_name) ;
	      v3_error(V3E_SOCKETBIND,"IO","OPEN","SOCKETBIND",errbuf,unitp) ;
	    } ;
	   if (listen(unitp->PrimarySocket,10) < 0)
	    { sprintf(errbuf,"Error (%s) listen'ing on INET Socket (%s)",v_OSErrString(errno),unitp->file_name) ;
	      v3_error(V3E_SOCKETLISTEN,"IO","OPEN","SOCKETLISTEN",errbuf,unitp) ;
	    } ;
	   fromlen = sizeof fsin ;
	   if ((unitp->ConnectSocket = accept(unitp->PrimarySocket,(struct sockaddr *)&fsin,&fromlen)) < 0)
	    { sprintf(errbuf,"Error (%s) accept'ing on INET Socket (%s)",v_OSErrString(errno),unitp->file_name) ;
	      v3_error(V3E_SOCKETACCEPT,"IO","OPEN","SOCKETACCEPT",errbuf,unitp) ;
	    } ;
	 } else		/* else create client socket */
	 {
	   if (connect(unitp->PrimarySocket,(struct sockaddr *)&sin,sizeof sin) < 0)
	    { if (!(NETERROR == EWOULDBLOCK || NETERROR == EINPROGRESS))
	       { sprintf(errbuf,"Error (%s) connect'ing INET Socket to name (%s)",v_OSErrString(NETERROR),unitp->file_name) ;
	         v3_error(V3E_SOCKETCONNECT,"IO","OPEN","SOCKETCONNECT",errbuf,unitp) ;
	       } ;
	    } ;
	   unitp->ConnectSocket = unitp->PrimarySocket ;
	 } ;
	unitp->file_ptr = (FILE *)0 ;				/* Set up dummy file pointer */
	return ;
#else
	v3_error(V3E_SOCKETNOTSUP,"V3","OPEN","SOCKETNOTSUP","Sockets not supported in this version of V3",0) ;
#endif
}

/*	I / O   C L O S E   R O U T I N E			*/

/*	iou_close_all - Close all open files			*/

jmp_buf iou_close_all_continue ;			/* For IO close errors - continue with next unit */

void iou_close_all_trap(arg)				/* Here to handle any funk during IO unit close */
  int arg ;
{
	longjmp(iou_close_all_continue,1) ;
}

void iou_close_all(arg)
  int arg ;
{
   struct iou__unit *unitp ;
   struct iou__openlist savelist ;
   static int i ;

	savelist = ounits ;				/* Save for final ^P */
/*	Close any open files */
	for(i=0;i<ounits.count && i<V3_IO_UNIT_LIST;i++)
	 { process->have_V4intercept = TRUE ;		/* Set up to intercept all V4 errors */
	   if (setjmp(process->error_intercept) != 0)
	    { printf("? Error while closing V4IS file %s ... continuing\n",ounits.unit[i].file_name) ;
	      continue ;
	    } ;
	   switch (setjmp(iou_close_all_continue))
	    { case 0:	break ;				/* Init jmpbuf - flow down */
	      default:
		printf("? Bus/IllMem error while closing V4IS file %s ... continuing\n",ounits.unit[i].file_name) ;
		continue ;				/* Continue with next unit */
	    } ;
 	   signal(SIGILL,iou_close_all_trap) ; signal(SIGFPE,iou_close_all_trap) ; signal(SIGSEGV,iou_close_all_trap) ;
#ifdef UNIX
	   signal(SIGQUIT,iou_close_all_trap) ; signal(SIGBUS,iou_close_all_trap) ;
#endif
	   unitp = ounits.unit[i].unitp ;
	   iou_close(unitp,ounits.unit[i].file_name) ;
	 } ;
	process->have_V4intercept = FALSE ;
	ounits = savelist ;				/* Reset so ^P will work on exit */
	if (termSocket != 0) SOCKETCLOSE(termSocket) ;	/* If terminal via socket then close it off */
}

/*	iou_close - Closes a V3 file */

void iou_close(unitp,file_name)
  struct iou__unit *unitp ;		/* Pointer to the I/O unit */
  char *file_name ;			/* Pointer to file name (only from close-all module above) */
{
   short int i ;
   int rms_res ; char errbuf[V3_VMS_ERR_BUF_MAX] ;

/*	First see if in open file list */
	if (file_name == NULLV)
	 { for(i=0;i<ounits.count;i++)
	    { if (ounits.unit[i].unitp != unitp) continue ;
	      ounits.unit[i] = ounits.unit[ounits.count-1] ; i-- ; ounits.count-- ;
	    } ;
	 } ;
/*	If file has already been closed then just return */
	if (unitp->file_ptr == (FILE *)-1) return ;
/*	Maybe call v3 module to handle IO */
	i = (unitp->file_info & V3_FLAGS_IOF_NOHANDLER) != 0 ; unitp->file_info &= ~V3_FLAGS_IOF_NOHANDLER ;
	if (unitp->handler_mep != NULL ? !i : FALSE)	/* Call v3 module to handle I/O ? */
	 { PUSHVI(0) ; PUSHF(V3_FORMAT_EOA) ;
	   PUSHMP(unitp) ; PUSHF(V3_FORMAT_POINTER) ; PUSHINT(2) ;
	   xctu_call_mod(unitp->handler_mep) ;
	   unitp->handler_mep = NULL ;			/* Zap handler so we don't get trapped */
	   return ;
	 } ;
/*	NL: file ? */
	if ((unitp->is_static & IO_NULL_STATIC) != 0) { unitp->file_ptr = (FILE *)-1 ; return ; } ;
/*	See what type of file */
	if ((unitp->file_info & V3_FLAGS_IOF_C) != 0)
	 {
#ifdef WINNTxxvv
	   if (unitp->hfile != HFILE_ERROR)
	    { if (!CloseHandle(unitp->hfile))
	       { sprintf(errbuf,"Error (%s) while closing C file (%s)",v_OSErrString(GetLastError()),(file_name == NULL ? "?" : file_name)) ;
		 if (file_name == NULLV) v3_error(V3E_CLOSEFAIL,"IO","CLOSE","CLOSEFAIL",errbuf,unitp) ;
	         printf("%s",errbuf) ; return ;
	       } ;
	      if ((unitp->file_info & V3_FLAGS_IOF_DELETE_FILE) != 0) DeleteFile(unitp->file_name) ;
	      unitp->file_ptr = (FILE *)-1 ; return ;
	   } ;
#endif
	   if (fclose(unitp->file_ptr) != NULLV)
	    { sprintf(errbuf,"Error (%s) while closing C file (%s)",v_OSErrString(errno),(file_name == NULL ? "?" : file_name)) ;
	      if (file_name == NULLV) v3_error(V3E_CLOSEFAIL,"IO","CLOSE","CLOSEFAIL",errbuf,unitp) ;
	      printf("%s",errbuf) ; return ;
	    } ;
	   if ((unitp->file_info & V3_FLAGS_IOF_DELETE_FILE) != 0) remove(unitp->file_name) ;	/* Maybe delete the file ? */
#ifdef UNIX
	   if ((unitp->file_info & V3_FLAGS_IOF_EXECUTABLE) != 0)
	    chmod(unitp->file_name,S_IRWXU+S_IRWXG) ;		/* Maybe change ownership to permit execution of file */
#endif
	   unitp->file_ptr = (FILE *)-1 ; return ;
	 } ;
#ifdef SOCKETS
	if ((unitp->file_info & (V3_FLAGS_IOF_UNIXSOCKET | V3_FLAGS_IOF_INETSOCKET)) != 0)
	 { if (SOCKETCLOSE(unitp->PrimarySocket) != 0)
	    { if (file_name == NULLV) v3_error(V3E_CLOSESOCKETFAIL,"IO","CLOSE","CLOSESOCKETFAIL","Could not close socket",unitp) ;
	      printf("? Error (%s) while attempting close of primary socket (%d) - %s\n",v_OSErrString(errno),unitp->PrimarySocket,file_name) ;
	      return ;
	    } ;
	   if (unitp->PrimarySocket != unitp->ConnectSocket)
	    { if (SOCKETCLOSE(unitp->ConnectSocket) != 0)
	       { if (file_name == NULLV) v3_error(V3E_CLOSESOCKETFAIL,"IO","CLOSE","CLOSESOCKETFAIL","Could not close socket",unitp) ;
	         printf("? Error (%s) in close of connect socket (%d) - %s\n",v_OSErrString(errno),unitp->ConnectSocket,file_name) ;
	         return ;
	       } ;
	    } ;
	   unitp->file_ptr = (FILE *)-1 ; return ;
	 } ;
#endif
/*	Are we closing a V4IS file */
#ifdef v4is
	if (unitp->file_info & V3_FLAGS_IOF_V4IS)
	 { v3unitp = unitp ;
	   v4is_Close((struct V4IS__ParControlBlk *)unitp->file_ptr) ;			/* Pass PCB to v4 */
	   v4mm_FreeChunk(unitp->file_ptr) ;
	   unitp->file_info &= ~V3_FLAGS_IOF_V4IS ;
	   unitp->file_ptr = (FILE *)-1 ; v3unitp = NULL ; return ;
	 } ;
#endif
#ifdef VMSOS
	if (file_name == NULL) vms_close(unitp) ;		/* Call module to handle RMS interface - only if not exitting! */
	return ;
#endif
/*	Are we closing C-ISAM file ? */
#ifdef cisam
	isclose(unitp->file_ptr) ; unitp->file_ptr = (FILE *)-1 ;
	if ((unitp->file_info & V3_FLAGS_IOF_DELETE_FILE) != 0)
	 { if (iserase(unitp->file_name) < 0) iou_cisam_error("CLOSE",unitp) ; } ;
	return ;
#endif
	if (file_name == NULLV) v3_error(V3E_NOTOPEN,"IO","CLOSE","NOTOPEN","Unit is not open",unitp) ;
	printf("? Unit associated with file %s does not appear to be open\n",file_name) ; return ;
}

/*	I / O   G E T   R O U T I N E				*/

/*	iou_get - Reads a record */

void iou_get(unitp)
  struct iou__unit *unitp ;		/* Pointer to the I/O unit */
{ char *ptr,errbuf[V3_VMS_ERR_BUF_MAX] ;
   union val__format get_format ;	/* Format of get buffer */
   union val__format key_format ;	/* Format of key buffer */
#ifdef cisam
   struct keydesc kdesc ;
   struct cisam__keys *ckeys ;
   int mode,*li,t ;
#endif
#ifdef SOCKETS
  fd_set sset ;				/* Socket set */
  struct timeval tev ;
  int sres ;
#endif
#ifdef v4is
   int vmode ;
   struct V4IS__ParControlBlk *pcb ;
#endif
   int rms_res ;			/* Updated with result of rms call */
   int i,flags,len,tries ;		/* Set to V3 processing flags */
   char *cptr ; int clen ;		/* Pointer & length if compaction */

/*	Maybe call v3 module to handle IO */
	i = (unitp->file_info & V3_FLAGS_IOF_NOHANDLER) != 0 ; unitp->file_info &= ~V3_FLAGS_IOF_NOHANDLER ;
	if (unitp->handler_mep != NULL ? !i : FALSE)	/* Call v3 module to handle I/O ? */
	 { PUSHVI(0) ; PUSHF(V3_FORMAT_EOA) ;
	   PUSHMP(unitp) ; PUSHF(V3_FORMAT_POINTER) ; PUSHINT(3) ;
	   xctu_call_mod(unitp->handler_mep) ;
	   return ;
	 } ;
/*	Should we force an EOF ? */
	if ((unitp->is_static & IO_NULL_STATIC) != 0)
	 v3_error(V3E_EOF,"IO","GET","EOF","End of file reached (io_get on nl: device)",unitp) ;
	if (unitp->file_ptr == (FILE *)-1) v3_error(V3E_NOTOPEN,"IO","GET","NOTOPEN","Unit is not open",unitp) ;
/*	Have a static I/O unit ? */
/*	Get buffer address & pointer */
	if (unitp->get_ptr != NULLV)
	 { ptr = (char *)unitp->get_ptr ;
	   get_format.all = unitp->get_format.all ;
	   unitp->get_ptr = NULLV ; unitp->get_format.all = 0 ;
	 } else
	 { if (unitp->dflt_get_ptr == NULLV)
	    v3_error(V3E_NOGETBUF,"IO","GET","NOGETBUF","No get record buffer has been specified",unitp) ;
	   ptr = (char *)unitp->dflt_get_ptr ;
	   get_format.all = unitp->dflt_get_format.all ;
	 } ;
	len = get_format.fld.length ;
	unitp->last_pointer = ptr ;	/* Save "last buffer pointer" in case of error dump */
	if (unitp->get_mode == 0) { flags = unitp->dflt_get_mode ; }
	 else { flags = unitp->get_mode ; unitp->get_mode = 0 ; } ;
/*	Are we reading from a V4IS file ? */
#ifdef v4is
	if (unitp->file_info & V3_FLAGS_IOF_V4IS)
	 { v3unitp = unitp ;
	   pcb = (struct V4IS__ParControlBlk *)unitp->file_ptr ; unitp->get_count ++ ;
	   pcb->GetBufLen = len ; pcb->GetBufPtr = ptr ;
	   if (unitp->get_key > 0) { i = unitp->get_key ; unitp->get_key = 0 ; } else i = unitp->dflt_get_key ;
	   if (i == 0) i = 1 ; pcb->KeyNum = i ;
	   if (unitp->key_ptr != NULLV)
	    { pcb->KeyPtr = (struct V4IS__Key *)unitp->key_ptr ; unitp->key_ptr = 0 ;
	    } else { pcb->KeyPtr = (struct V4IS__Key *)unitp->dflt_key_ptr ; } ;
	   if ((flags & V3_FLAGS_IOPG_NEXT) != 0) { vmode = V4IS_PCB_GP_Next ; }
	    else if ((flags & V3_FLAGS_IOPG_PARTIAL) != 0 ) { vmode = V4IS_PCB_GP_KeyedNext ; }
	    else if ((flags & V3_FLAGS_IOPG_KEYED) != 0) { vmode = V4IS_PCB_GP_Keyed ; }
	    else if ((flags & V3_FLAGS_IOPG_BOF) != 0) { vmode = V4IS_PCB_GP_BOF ; }
	    else if ((flags & V3_FLAGS_IOPG_EOF) != 0) { vmode = V4IS_PCB_GP_EOF ; }
	    else if ((flags & V3_FLAGS_IOPG_DATAONLY) != 0) { vmode = V4IS_PCB_GP_DataOnly ; }
	    else if ((flags & V3_FLAGS_IOPG_VMSRFA) != 0)
		  { vmode = V4IS_PCB_GP_DataId ; pcb->DataId = unitp->vms_rfa4 ; pcb->DataId2 = unitp->vms_rfa2 ; }
	    else if ((flags & V3_FLAGS_IOPG_NEXTNUM1) != 0) { vmode = V4IS_PCB_GP_NextNum1 ; }
	    else
	     { if (pcb->AreaId != V4IS_AreaId_SeqOnly) v3_error(V3E_INVGET,"IOF","GET","INVGET","Invalid io_get mode specified",(void *)flags) ; } ;
	   if (flags & V3_FLAGS_IOPG_NOLOCK) vmode |= (V4IS_PCB_NoLock | V4IS_PCB_IgnoreLock) ;
	   if (flags & V3_FLAGS_IOPG_LOCK_WAIT) vmode |= V4IS_PCB_LockWait ;
	   pcb->GetMode = vmode ;
	   if (unitp->fileref != 0) { pcb->FileRef = unitp->fileref ; unitp->fileref = 0 ; }
	    else if (unitp->dflt_fileref != 0) { pcb->FileRef = unitp->dflt_fileref ; } ;
	   v4is_Get(pcb) ;
	   unitp->vms_rfa4 = pcb->DataId ; unitp->vms_rfa2 = pcb->DataId2 ;		/* Store last data id */
	   unitp->get_length = pcb->GetLen ;
	   if (unitp->max_gets != 0)
	    { if ((unitp->get_count % unitp->max_gets) == 0)
	       v3_error(V3E_LIMIT,"IOMAX",unitp->v3_name,"LIMIT","Reached limit as per unit's max_gets",unitp) ;
	    } ;
	   v3unitp = NULL ; return ;
	 } ;
#endif
/*	See if we have record compaction */
	if (unitp->record_info & V3_FLAGS_IOR_COMPRESSED)
	 { cptr = ptr ; clen = len ; unitp->is_static = 0 ;
	   if (compact_buffer == 0) compact_buffer = (char *)v4mm_AllocChunk(V3_IO_COMPACT_BUF,FALSE) ;
	   ptr = compact_buffer ; len = V3_IO_COMPACT_BUF ;
	 } ;
/*	What kind of file is this ? */
#ifdef SOCKETS
	if ((unitp->file_info & (V3_FLAGS_IOF_UNIXSOCKET | V3_FLAGS_IOF_INETSOCKET)) != 0)
	 { unitp->get_count++ ;
	   for(i=0;i<len;)
	    { if (len == 1 || (unitp->file_info & V3_FLAGS_IOF_NOBLOCK) != 0)
	       { FD_ZERO(&sset) ; FD_SET(unitp->ConnectSocket,&sset) ;
	         tev.tv_sec = 30 ; tev.tv_usec = 0 ;		/* Set max wait for 30 seconds */
		 sres = select(FD_SETSIZE,&sset,NULL,NULL,&tev) ;	/* Wait for something to happen */
	         if (sres <= 0) v3_error(V3E_SOCKETRECV,"IO","GET","SOCKETRECV","No more input appearing on socket",unitp) ;
	       } ;
	      if ((clen = recv(unitp->ConnectSocket,ptr,len-i,0)) <= 0)
	       { if (clen == 0) { strcpy(errbuf,"Remote connection has shut down the connection gracefully") ; }
	          else { sprintf(errbuf,"Error (%s) in recv on socket (%d)",v_OSErrString(errno),unitp->file_name) ; } ;
	         v3_error(V3E_SOCKETRECV,"IO","GET","SOCKETRECV",errbuf,unitp) ;
	       } ;
	      i += clen ; ptr += clen ;			/* May not receive entire buffer in one recv packet */
	    } ; unitp->get_length = len ;
	   return ;
	 } ;
#endif
	if ((unitp->file_info & V3_FLAGS_IOF_C) != 0)
	 {
/*	   Use different reads depending on whether or not a text file */
	   if ((unitp->record_info & V3_FLAGS_IOR_CARRIAGE_RETURN) != 0)
	    { if (fgets(ptr,len,unitp->file_ptr) == NULL)
	       v3_error(V3E_EOF,"IO","GET","EOF","End of file reached",unitp) ;
	      if (*(ptr + unitp->get_length-1) == '\n') unitp->get_length -- ;
	    } else
	    {
#ifdef WINNTxxvv
	      if (unitp->hfile != HFILE_ERROR)
	       { if (!ReadFile(unitp->hfile,ptr,len,&clen,NULL)) clen = 0 ;
	         if (clen < len) v3_error(V3E_EOF,"IO","GET","EOF","End of file reached",unitp) ;
		 goto end_cget ;
	       } ;
#endif
	      if (fread(ptr,len,1,unitp->file_ptr) == NULLV)
	       v3_error(V3E_EOF,"IO","GET","EOF","End of file reached",unitp) ;
	    } ;
end_cget:
	   unitp->get_count++ ; unitp->get_length = strlen(ptr) ;
/*	   If text processing (/CARRIAGE_RETURN/) then maybe strip off trailing LF */
	   if ((unitp->record_info & V3_FLAGS_IOR_CARRIAGE_RETURN) != 0)
	    { if (*(ptr + unitp->get_length-1) == '\n') unitp->get_length -- ; } ;
	   goto got_record ;
	 } ;
#ifdef VMSOS
	vms_get(unitp,ptr,len,flags,get_format.all) ; return ;	/* Call module to handle RMS interface */
#endif
/*	Are we reading from a C-ISAM file ? */
#ifdef cisam
	if (unitp->lock_ind) { isrelease(unitp->file_ptr) ; unitp->lock_ind = FALSE ; } ;
	unitp->get_count++ ;
	if ((flags & V3_FLAGS_IOPG_VMSRFA) != 0)
	 { kdesc.k_nparts = 0 ; isrecnum = unitp->vms_rfa4 ;
	   mode = ISEQUAL ; unitp->last_get_key = -1 ;	/* Flag so we can reset index with isstart! */
	   if (isstart(unitp->file_ptr,&kdesc,len,ptr,mode) < 0) iou_cisam_error("GET",unitp) ;
/*	   Should we do record locking ? */
	   if ((flags & V3_FLAGS_IOPG_NOLOCK) == 0)
	    { if ((unitp->share_info) != 0) { mode += ISLOCK ; unitp->lock_ind = TRUE ; } ; } ;
	   if (isread(unitp->file_ptr,ptr,mode) == 0) { unitp->vms_rfa4 = isrecnum ; goto got_cisam_record ; }
	    else iou_cisam_error("GET",unitp) ;
	 } ;
	if ((flags & V3_FLAGS_IOPG_NEXT) != 0) { mode = ISNEXT ; }
	 else if ((flags & V3_FLAGS_IOPG_PARTIAL) != 0 ) { mode = ISGTEQ ; }
	 else if ((flags & V3_FLAGS_IOPG_KEYED) != 0) { mode = ISEQUAL ; }
	 else if ((flags & V3_FLAGS_IOPG_BOF) != 0) { mode = ISFIRST ; }
	 else if ((flags & V3_FLAGS_IOPG_EOF) != 0) { mode = ISLAST ; }
	 else v3_error(V3E_INVGET,"IOF","GET","INVGET","Invalid io_get mode specified",flags) ;
	ckeys = (struct cisam__keys *)unitp->keydesc_ptr ;		/* Loop thru all keys & convert integers */
	for(i=0;i<ckeys->count;i++)
	 { if (ckeys->key[i].k_type != LONGTYPE) continue ;
	   li = (int*)(ptr+ckeys->key[i].k_start) ; t = *li ; stlong(t,ptr+ckeys->key[i].k_start) ;
	 } ;
	if (mode != ISNEXT)
	 { if (unitp->get_key > 0) { i = unitp->get_key ; unitp->get_key = 0 ; } else i = unitp->dflt_get_key ;
	   if (i == 0) i = 1 ;
	   if (unitp->last_get_key != i)
	    { unitp->last_get_key = i ; ckeys = (struct cisam__keys *)unitp->keydesc_ptr ;
	      if (isstart(unitp->file_ptr,&ckeys->key[unitp->last_get_key-1],0,ptr,mode) < 0) iou_cisam_error("GET",unitp) ;
	    } ;
	 } ;
/*	Should we do record locking ? */
	if ((flags & V3_FLAGS_IOPG_NOLOCK) == 0)
	 { if ((unitp->share_info) != 0) { mode += ISLOCK ; unitp->lock_ind = TRUE ; } ; } ;
	if (isread(unitp->file_ptr,ptr,mode) == 0)
	 { unitp->vms_rfa4 = isrecnum ;	/* Save C-ISAMs rfa */
	   goto got_cisam_record ;
	 } else iou_cisam_error("GET",unitp) ;
got_cisam_record:
#ifdef ISVARLEN
	unitp->get_length = isreclen ;					/* Save number of bytes read */
#endif
	ckeys = (struct cisam__keys *)unitp->keydesc_ptr ;		/* Loop thru all keys & convert integers */
	for(i=0;i<ckeys->count;i++)
	 { if (ckeys->key[i].k_type != LONGTYPE) continue ;
	   li = (int *)(ptr+ckeys->key[i].k_start) ; t = ldlong(ptr+ckeys->key[i].k_start) ; *li = t ;
	 } ;
#endif
got_record:
/*	Should we expand a compressed record ? */
	if (unitp->record_info & V3_FLAGS_IOR_COMPRESSED)
	 { ptr = cptr ; unitp->get_length = data_expand(cptr,compact_buffer,unitp->get_length) ; } ;
/*	If variable length record then append null */
	if (get_format.fld.type == VFT_VARSTR) *(ptr+unitp->get_length) = NULLV ;
/*	See if we should trap as per limits */
	if (unitp->max_gets != 0)
	 { if ((unitp->get_count % unitp->max_gets) == 0)
	    v3_error(V3E_LIMIT,"IOMAX",unitp->v3_name,"LIMIT","Reached limit as per unit's max_gets",unitp) ;
	 } ;
	return ;
}

/*	I / O   P U T   R O U T I N E				*/

/*	iou_put - Writes a record */

void iou_put(unitp)
  struct iou__unit *unitp ;		/* Pointer to the I/O unit */
{ union val__format put_format ;	/* Put buffer format desc */
#ifdef cisam
   int cc,t,*li,i ;
   struct cisam__keys *ckeys ;
#endif
#ifdef SOCKETS
  fd_set sset ;				/* Socket set */
  struct timeval tev ;
#endif
#ifdef v4is
   int vmode ;
   struct V4IS__ParControlBlk *pcb ;
#endif
   FILE *fp ;
   char *ptr,errbuf[V3_VMS_ERR_BUF_MAX] ;
   int i,len,rms_res,flags,sres,retry ;
   int clen ; unsigned char *cptr ;		/* For compaction */

/*	Maybe call v3 module to handle IO */
	i = (unitp->file_info & V3_FLAGS_IOF_NOHANDLER) != 0 ; unitp->file_info &= ~V3_FLAGS_IOF_NOHANDLER ;
	if (unitp->handler_mep != NULL ? !i : FALSE)	/* Call v3 module to handle I/O ? */
	 { PUSHVI(0) ; PUSHF(V3_FORMAT_EOA) ;
	   PUSHMP(unitp) ; PUSHF(V3_FORMAT_POINTER) ; PUSHINT(4) ;
	   xctu_call_mod(unitp->handler_mep) ;
	   return ;
	 } ;
/*	Output to a null (NL:) device ? */
	if ((unitp->is_static & IO_NULL_STATIC) != 0) { unitp->put_count++ ; return ; } ;
/*	Should we be shadowing this unit ? */
	if (unitp->shadow_flag != 0) v3_error(V3E_NOTSHADOW,"IO","PUT","NOTSHADOW","Attempted io_put without first shadowing",unitp) ;
	unitp->shadow_flag = unitp->shadow_index ;	/* Copy over to ensure that V3 code clears before next io_put */
/*	Get put buffer address & pointer */
	if (unitp->put_ptr != NULLV)
	 { ptr = (char *)unitp->put_ptr ;
	   put_format.all = unitp->put_format.all ;
	   unitp->put_ptr = NULLV ; unitp->put_format.all = 0 ;
	 } else
	 { if (unitp->dflt_put_ptr == NULLV)
	    v3_error(V3E_NOPUTBUF,"IO","PUT","NOPUTBUF","No put record buffer has been specified",unitp) ;
	   ptr = (char *)unitp->dflt_put_ptr ; put_format.all = unitp->dflt_put_format.all ;
	 } ;
/*	Figure out the buffer length */
	len = (put_format.fld.type == VFT_FIXSTR || put_format.fld.type == VFT_FIXSTRSK ?
		put_format.fld.length : strlen(ptr)) ;
/*	Get the put flags */
	if (unitp->put_mode == 0) { flags = unitp->dflt_put_mode ; }
	 else { flags = unitp->put_mode ; unitp->put_mode = 0 ; } ;
	if (flags & (V3_FLAGS_IOPG_FILETEXT | V3_FLAGS_IOPG_FILEBINARY))
	 { strncpy(errbuf,ptr,len) ; errbuf[len] = '\0' ;
	   fp = fopen(v3_logical_decoder(errbuf,TRUE),(flags & V3_FLAGS_IOPG_FILETEXT ? "r" : "rb")) ;
	   if (fp == NULL)
	    v3_error(0,"IO","PUT","NOFILE","Could not access file to write to unit",unitp) ;
	 } ;
/*	Should we chop off trailing spaces ? */
	if (flags & V3_FLAGS_IOPG_TRUNCATE)
	 { for (;len>0 && *(ptr+len-1)==' ';len--) ; } ;
/*	Are we going to a V4IS File ? */
#ifdef v4is
	if (unitp->file_info & V3_FLAGS_IOF_V4IS)
	 { v3unitp = unitp ;
	   pcb = (struct V4IS__ParControlBlk *)unitp->file_ptr ;
	   pcb->PutBufLen = len ; pcb->PutBufPtr = ptr ;
	   if ((flags & V3_FLAGS_IOPG_DELETE) != 0) { vmode = V4IS_PCB_GP_Delete ; }
	    else if ((flags & V3_FLAGS_IOPG_EOF) != 0) { vmode = V4IS_PCB_GP_Append ; }
	    else if ((flags & V3_FLAGS_IOPG_NEW_RECORD) != 0 ) { vmode = V4IS_PCB_GP_Insert ; }
	    else if ((flags & V3_FLAGS_IOPG_UPDATE_EXISTING) != 0) { vmode = V4IS_PCB_GP_Update ; }
	    else if ((flags & V3_FLAGS_IOPG_DATAONLY) != 0) { vmode = V4IS_PCB_GP_DataOnly ; }
	    else if ((flags & V3_FLAGS_IOPG_CACHE) != 0) { vmode = V4IS_PCB_GP_Cache ; }
	    else if ((flags & V3_FLAGS_IOPG_RESET) != 0) { vmode = V4IS_PCB_GP_Reset ; }
	    else if ((flags & V3_FLAGS_IOPG_KEYED) != 0) { vmode = V4IS_PCB_GP_Write ; }
	    else if ((flags & V3_FLAGS_IOPG_OBSOLETE) != 0) { vmode = V4IS_PCB_GP_Obsolete ; }
	    else
	     { if (pcb->AreaId != V4IS_AreaId_SeqOnly) v3_error(V3E_INVPUT,"IOF","PUT","INVPUT","Invalid io_put mode specified",(void *)flags) ; } ;
	   if (flags & V3_FLAGS_IOPG_RELOCK) vmode |= V4IS_PCB_ReLock ;
	   if (unitp->pragmas & V3_FLAGS_IOPRAG_PUT_MASS) { pcb->PutMode = vmode ; }
	    else { pcb->PutMode = V4IS_PCB_SafePut | vmode ; } ;	/* Force all buffer writes (for now?) */
	   if (unitp->fileref != 0) { pcb->FileRef = unitp->fileref ; unitp->fileref = 0 ; }
	    else if (unitp->dflt_fileref != 0) { pcb->FileRef = unitp->dflt_fileref ; } ;
	   unitp->put_count++ ;
	   v4is_Put(pcb,NULL) ;
	   unitp->vms_rfa4 = pcb->DataId ;			/* Store last data id */
	   v3unitp = NULL ; return ;
	 } ;
#endif
/*	See if we have record compaction */
	if (unitp->record_info & V3_FLAGS_IOR_COMPRESSED)
	 { if (compact_buffer == 0) compact_buffer = (char *)v4mm_AllocChunk(V3_IO_COMPACT_BUF,FALSE) ;
	   len = data_compress(compact_buffer,ptr,len,unitp->header_bytes,V3_IO_COMPACT_BUF) ; ptr = compact_buffer ;
	   unitp->is_static = 0 ;
	 } ;
#ifdef SOCKETS
	if ((unitp->file_info & (V3_FLAGS_IOF_UNIXSOCKET | V3_FLAGS_IOF_INETSOCKET)) != 0)
	 { if (flags & V3_FLAGS_IOPG_FILETEXT)
	    { char tbuf[2] ;
//	      if (compact_buffer == 0) compact_buffer = (char *)v4mm_AllocChunk(V3_IO_COMPACT_BUF,FALSE) ;
	      for(;;)
	       { 
	         
//		 if ((cptr=fgets(compact_buffer,V3_IO_COMPACT_BUF,fp)) == NULL) break ;
//		 len = strlen(cptr) ; cptr += (len-1) ;
//		 for(;;cptr--) { if (*cptr >= '\20' || *cptr == '\0') break ; } ;
//		 if (*cptr != 0) *(cptr+1) = '\0' ; len = strlen(compact_buffer)+1 ;


		 if (fread(tbuf,1,1,fp) == 0) break ;
		 if (TRUE || (unitp->file_info & V3_FLAGS_IOF_NOBLOCK) != 0)
		  { FD_ZERO(&sset) ; FD_SET(unitp->ConnectSocket,&sset) ;
		    tev.tv_sec = 30 ; tev.tv_usec = 0 ;		/* Set max wait for 30 seconds */
		    sres = select(FD_SETSIZE,NULL,&sset,NULL,&tev) ;	/* Wait for something to happen */
		    if (sres <= 0) v3_error(V3E_SOCKETSEND,"IO","PUT","SOCKETSEND","Socket does not appear to be ready to send()",unitp) ;
		  } ;
		 for(retry=0;;retry++)
		  { if ((sres = send(unitp->ConnectSocket,tbuf,1,0)) >= 1) break ; ;
		    if (sres < 0 || retry > 20)
		     { char buf[100] ; strncpy(buf,ptr,25) ; buf[25]='\0' ;
		       printf("???tf ptr=*%s*, len=%d, sres=%d, flags=%d, errno=%d, file_info=%x\n",buf,len,sres,flags,errno,unitp->file_info) ;
		       sprintf(errbuf,"Error (%d %s) (sres=%d retry=%d) in tf-send on socket (%s)",errno,v_OSErrString(errno),sres,retry,unitp->file_name) ;
		       v3_error(V3E_SOCKETSEND,"IO","GET","SOCKETSEND",errbuf,unitp) ;
		     } ;
		    HANGLOOSE(100) ;
		  } ;



//		 for(retry=0,cptr=compact_buffer;;retry++)
//		  { if ((sres = send(unitp->ConnectSocket,cptr,len,0)) >= len) break ;
//		    if (sres < 0 || retry > 20)
//		     { sprintf(errbuf,"Error (%s) (len = %d/%d) in send on socket (%s)",v_OSErrString(errno),len,sres,unitp->file_name) ;
//		       v3_error(V3E_SOCKETSEND,"IO","GET","SOCKETSEND",errbuf,unitp) ;
//		     } ;
//		    cptr += sres ; len -= sres ; HANGLOOSE(100) ;
//		  } ;


	       } ; fclose(fp) ;
	      return ;
	    } ;

/*	   If socket non-blocking then wait via select() until ready */
	   if ((unitp->file_info & V3_FLAGS_IOF_NOBLOCK) != 0)
	    { FD_ZERO(&sset) ; FD_SET(unitp->ConnectSocket,&sset) ;
	      tev.tv_sec = 45 ; tev.tv_usec = 0 ;		/* Set max wait for 45 seconds */
	      sres = select(FD_SETSIZE,NULL,&sset,NULL,&tev) ;	/* Wait for something to happen */
	      if (sres <= 0) v3_error(V3E_SOCKETSEND,"IO","PUT","SOCKETSEND","Socket does not appear to be ready to send()",unitp) ;
	    } ;
	   if ((sres = send(unitp->ConnectSocket,ptr,len,0)) < len)
	    { char buf[100] ; strncpy(buf,ptr,25) ; buf[25]='\0' ;
	      printf("??? ptr=*%s*, len=%d, sres=%d, flags=%d, errno=%d, file_info=%x\n",buf,len,sres,flags,errno,unitp->file_info) ;
	      sprintf(errbuf,"Error (%d %s) (len = %d/%d) in send on socket (%s)",errno,v_OSErrString(errno),len,sres,unitp->file_name) ;
	      v3_error(V3E_SOCKETSEND,"IO","GET","SOCKETSEND",errbuf,unitp) ;
	    } ;
	   return ;
	 } ;
#endif
/*	Are we writing to a C file ? */
#ifdef WINNTxxvv
	if ((unitp->file_info & V3_FLAGS_IOF_C) != 0 && unitp->hfile != HFILE_ERROR)
	 { if (len > 0)
	    { if (!WriteFile(unitp->hfile,ptr,len,&clen,NULL))
	       { sprintf(errbuf,"Error (%s) in WriteFile(%d bytes) to file (%s)",v_OSErrString(GetLastError()),len,unitp->file_name) ;
	         v3_error(V3E_PUTERRC,"IO","PUT","PUTERRC",errbuf,unitp) ;
	       } ;
	    } ;
	   if ((unitp->record_info & V3_FLAGS_IOR_CARRIAGE_RETURN) != 0) WriteFile(unitp->hfile,"\r\n",2,&clen,NULL) ;
	   unitp->put_count++ ; return ;
	 } ;
#endif
	if ((unitp->file_info & V3_FLAGS_IOF_C) != 0)
	 { if (len > 0)
	    { if (fwrite(ptr,len,1,unitp->file_ptr) == 0)
	       { sprintf(errbuf,"Error (%s) in fwrite(%d bytes) to file (%s)",v_OSErrString(errno),len,unitp->file_name) ;
	         v3_error(V3E_PUTERRC,"IO","PUT","PUTERRC",errbuf,unitp) ;
	       } ;
	    } ;
	   if ((unitp->record_info & V3_FLAGS_IOR_CARRIAGE_RETURN) != 0)
	    { fwrite("\n",1,1,unitp->file_ptr) ; } ;
	   unitp->put_count++ ;
	   return ;
	 } ;
#ifdef VMSOS
	vms_put(unitp,ptr,len,flags) ; return ;			/* Call module to handle RMS interface */
#endif
#ifdef cisam
#ifdef ISVARLEN
	isreclen = (len > unitp->over_length ? len : unitp->over_length) ;	/* Set length for variable record files */
#endif
	ckeys = (struct cisam__keys *)unitp->keydesc_ptr ;		/* Loop thru all keys & convert integers */
	for(i=0;i<ckeys->count;i++)
	 { if (ckeys->key[i].k_type != LONGTYPE) continue ;
	   li = (int*)(ptr+ckeys->key[i].k_start) ; t = *li ; stlong(t,ptr+ckeys->key[i].k_start) ;
	 } ;
	if (unitp->lock_ind) { isrelease(unitp->file_ptr) ; unitp->lock_ind = FALSE ; } ;
	if ((flags & V3_FLAGS_IOPG_DELETE) != 0) { cc = isdelcurr(unitp->file_ptr) ; }
	 else if ((flags & V3_FLAGS_IOPG_NEW_EOF) != 0)
	  { v3_error(V3E_CISAMNYI,"IO","PUT","CISAMNYI","/new_eof/ function not implemented in C-ISAM",unitp) ; }
	 else if ((flags & V3_FLAGS_IOPG_NEW_RECORD) != 0) { cc = iswrite(unitp->file_ptr,ptr) ; }
	 else if ((flags & V3_FLAGS_IOPG_UPDATE_EXISTING) != 0) { cc = isrewcurr(unitp->file_ptr,ptr) ; }
/*		General write - try new record - if fails then try to rewrite "current" */
	 else { cc = iswrite(unitp->file_ptr,ptr) ;
		if (cc < 0 && (iserrno == EDUPL)) cc = isrewrite(unitp->file_ptr,ptr) ;
		if (cc < 0 && iserrno == ENOTOPEN)
		 { ckeys = (struct cisam__keys *)unitp->keydesc_ptr ;
	   	   if (isstart(unitp->file_ptr,&ckeys->key[0],0,ptr,ISEQUAL) < 0) iou_cisam_error("PUT",unitp) ;
		   if (isdelcurr(unitp->file_ptr) < 0) iou_cisam_error("PUT",unitp)  ;
		   cc = iswrite(unitp->file_ptr,ptr) ;
		 } ;
	      } ;
	ckeys = (struct cisam__keys *)unitp->keydesc_ptr ;		/* Loop thru all keys & re-convert integers */
	for(i=0;i<ckeys->count;i++)
	 { if (ckeys->key[i].k_type != LONGTYPE) continue ;
	   li = (int *)(ptr+ckeys->key[i].k_start) ; t = ldlong(ptr+ckeys->key[i].k_start) ; *li = t ;
	 } ;
/*	Did we get any errors ? */
	if (cc < 0) iou_cisam_error("PUT",unitp) ;
	unitp->put_count ++ ; unitp->vms_rfa4 = isrecnum ;
	return ;
#endif
}

/*	I / O   M I S C   R O U T I N E				*/

/*	iou_misc - Handles miscellaneous I/O functions		*/
#ifdef cisam
struct cisam__keys ckbuf ;
#endif
void iou_misc()
{
#ifdef cisam
   struct cisam__keys *ckeys ;
   struct keydesc *ck ;
   int cc ;
#endif
#ifdef USOCKETS
   struct sockaddr_un saun,fsaun ;
#endif
#ifdef ISOCKETS
   struct sockaddr_in sin,fsin ;
#endif
#ifdef SOCKETS
   int fromlen ;
#endif
#ifdef v4is
   struct V4MM__MemMgmtMaster *mmm ;
   struct V4IS__ParControlBlk *pcb ;
   struct V4FFI__KeyInfo *ki ;
   struct V4FFI__KeyInfoDtl *kid ;
#endif
#ifdef VMSOS
   struct FAB *fabp ;
   struct RAB *rabp ;
   struct XABKEY *xabp ;
#endif
   FILE *fp ;
   struct iou__unit *unitp ;
   struct str__ref sref ;
   V3STACK *base_ptr ;
   V3STACK *ptr ;			/* Holds stack pointer */
   int func_code ;
   int rms_res ; char errbuf[V3_VMS_ERR_BUF_MAX],*bufp,*tptr,dupstr[50],*duptr,filename[255] ;
   int ac,i,j,bytes ;
   union val__format format ;

/*	See how many arguments */
	for (ac=0;;ac++)
	 { POPF(format.all) ; POPVI(i) ; if (format.all == V3_FORMAT_EOA) break ; } ;
/*	ac = number of arguments, base_ptr = current stack pointer */
	base_ptr = psi->stack_ptr ;
/*	Get pointer to I/O unit */
	psi->stack_ptr = base_ptr - (2*SIZEOFSTACKENTRY) ;
	stru_popstr(&sref) ; unitp = (struct iou__unit *)sref.ptr ; psi->stack_ptr = base_ptr ;
/*	And the IO_MISC function code */
	psi->stack_ptr = base_ptr - (3*SIZEOFSTACKENTRY) ; func_code = xctu_popint() ; psi->stack_ptr = base_ptr ;
/*	Now branch on function code */
	switch (func_code & 0xFF) /* Mask out any high order flags */
	 { default:
		v3_error(V3E_BADFUNC,"IOU","MISC","BADFUNC","Invalid function code",unitp) ;
	   case V3_FLAGS_IOM_CHECKPOINT:
#ifdef v4is
		if ((unitp->file_info & V3_FLAGS_IOF_V4IS) != 0)
		 { v3unitp = unitp ; pcb = (struct V4IS__ParControlBlk *)unitp->file_ptr ; v4mm_AreaCheckPoint(pcb->AreaId) ;
		   return ;
		 } ;
#endif
#ifdef VMSOS
		if ((rabp = unitp->rab_pointer) == NULL)
		 v3_error(V3E_NOTOPENRMS,"IOU","MISC","NOTOPENRMS","Unit not openned as RMS",unitp) ;
		if (((rms_res = sys$flush(rabp)) & 1) != 1)
		 v3_error(V3E_CHECKPOINT,"IOU","MISC","CHECKPOINT",mscu_vms_errmsg(rms_res,errbuf),unitp) ;
		return ;
#endif
#ifdef cisam
		if ((unitp->file_info & V3_FLAGS_IOF_INDEXED) != 0)
		 { cc = isflush(unitp->file_ptr) ; if (cc < 0) iou_cisam_error("MISC",unitp) ;
		 } ;
#endif
		return ;
	   case V3_FLAGS_IOM_IS_OPEN:
		PUSHINT((unitp->file_ptr == (FILE *)-1 ? FALSE : TRUE)) ; return ;
	   case V3_FLAGS_IOM_UNLOCK:
#ifdef v4is
		if ((unitp->file_info & V3_FLAGS_IOF_V4IS) != 0)
		 { pcb = (struct V4IS__ParControlBlk *)unitp->file_ptr ;
		   pcb->PutMode = V4IS_PCB_GP_Unlock ; v4is_Put(pcb,NULL) ;
#ifdef OLDLOCK
		   mmm = V4_GLOBAL_MMM_PTR ;
		   if (mmm->Areas[pcb->AreaId].LockRec >= 0)
		    { v4mm_LockRelease(pcb->AreaId,mmm->Areas[pcb->AreaId].LockRec,mmm->MyPid,pcb->AreaId,"V3Unlock") ;
		      mmm->Areas[pcb->AreaId].LockRec = -1 ;
		    } ;
#endif
		   return ;
		 } ;
#endif
#ifdef VMSOS
		if ((rabp = unitp->rab_pointer) == NULL) v3_error(V3E_NOTOPENRMS,"IOU","MISC","NOTOPENRMS","Unit not openned as RMS",unitp) ;
		if (((rms_res = sys$free(rabp)) & 1) != 1) v3_error(V3E_UNLOCK,"IOU","MISC","UNLOCK",mscu_vms_errmsg(rms_res,errbuf),unitp) ;
		return ;
#endif
#ifdef cisam
		if ((unitp->file_info & V3_FLAGS_IOF_INDEXED) != 0)
		 { unitp->lock_ind = FALSE ; cc = isrelease(unitp->file_ptr) ; if (cc < 0) iou_cisam_error("MISC",unitp) ;
		 } ;
#endif
		return ;
	   case V3_FLAGS_IOM_SET_KEYSV:
/*		First get the base buffer */
		psi->stack_ptr = base_ptr - (5*SIZEOFSTACKENTRY) ; POPF(j) ; POPVP(bufp,(char *)) ;
		duptr = (char *)stru_popcstr(dupstr) ;
#ifdef v4is
/*if ( (unitp->file_info & V3_FLAGS_IOF_V4IS) == 0) goto skip_vv4 ; */
	 	v3unitp = unitp ;
/*		Loop thru each argument */
		ki = (struct V4FFI__KeyInfo *)(unitp->keydesc_ptr = (int *)v4mm_AllocChunk(sizeof *ki,FALSE)) ;
		ki->Count = 0 ; kid = (struct V4FFI__KeyInfoDtl *)&ki->Buffer ; kid->KeyCount = 0 ;
		if (unitp->fileref != 0) { kid->FileRef = unitp->fileref ; unitp->fileref = 0 ; }
		 else if (unitp->dflt_fileref != 0) { kid->FileRef = unitp->dflt_fileref ; }
		 else kid->FileRef = 9999 ;
		for ((j=ac,ptr=base_ptr-(2*(ac+1)));j>=5;(j--,ptr+=2))
		 {  kid->Key[j-5].DupsOK = (*(duptr+j-5) == 'Y' ? TRUE : FALSE) ;
/*		    See what kind of key */
		    format.all = *ptr ;
		    kid->Key[j-5].KeyPrefix.fld.AuxVal = kid->FileRef ;
		    switch (format.fld.type)
		     { default:
			v3_error(V3E_BADKEY,"IOU","MISC","BADKEY","Invalid datatype for key",unitp) ;
		       case VFT_BININT:
				kid->Key[j-5].KeyPrefix.fld.KeyType = j-5 ;	/* Key number = V4IS_KeyType_Frxx */
				kid->Key[j-5].KeyPrefix.fld.KeyMode = V4IS_KeyMode_Int ;
				kid->Key[j-5].KeyPrefix.fld.Bytes = sizeof i ;
				break ;
		       case VFT_BINWORD:
				v3_error(V3E_NOWORD,"IOU","MISC","NOWORD","Keys of type short are not supported in V4",0) ;
		       case VFT_STRINT:
		       case VFT_VARSTR:
		       case VFT_FIXSTRSK:
		       case VFT_FIXSTR:
				kid->Key[j-5].KeyPrefix.fld.KeyType = j-5 ;	/* Key number = V4IS_KeyType_Frxx */
				kid->Key[j-5].KeyPrefix.fld.KeyMode = V4IS_KeyMode_Alpha ;
				kid->Key[j-5].KeyPrefix.fld.Bytes = format.fld.length ;
				break ;
		       case VFT_PACDEC: v3_error(V3E_NOPACKV4,"IOU","MISC","NOPACKV4","Packed decimal keys not supported in V4",0) ;
		     } ;
/*		    Now get key position & length */
		    kid->Key[j-5].Offset = (tptr = (char *)*(ptr+1))-bufp ;
/*		    kid->Key[j-5].Offset = (tptr = (char *)xctu_pointer(*(ptr+1)))-bufp ; */
		    kid->KeyCount ++ ;
		 } ;
		kid->Bytes = sizeof *kid - sizeof kid->Key + (kid->KeyCount)*sizeof kid->Key[0] ;
		ki->Count ++ ; ki->Bytes = sizeof *ki - sizeof ki->Buffer + kid->Bytes ;
		v3unitp = NULL ;
/*		return ;  (want to fall thru to vms stuff if it is there) */
skip_vv4:
#endif
#ifdef VMSOS
/*		Loop thru each argument */
		for ((j=ac,ptr=base_ptr-(2*(ac+1)));j>=5;(j--,ptr+=2))
		 { xabp = malloc(sizeof *xabp) ;
		   *xabp = cc$rms_xabkey ;
		   xabp->xab$l_nxt = unitp->xabp ; unitp->xabp = xabp ;
/*		    See what kind of key */
		    format.all = *ptr ;
		    switch (format.fld.type)
		     { default:
			v3_error(V3E_BADKEY,"IOU","MISC","BADKEY","Invalid datatype for key",unitp) ;
		       case VFT_BININT: xabp->xab$b_dtp = XAB$C_IN4 ; break ;
		       case VFT_BINWORD: xabp->xab$b_dtp = XAB$C_IN2 ; break ;
		       case VFT_STRINT:
		       case VFT_VARSTR:
		       case VFT_FIXSTRSK:
		       case VFT_FIXSTR: xabp->xab$b_dtp = XAB$C_STG ; break ;
		       case VFT_PACDEC: xabp->xab$b_dtp = XAB$C_PAC ; break ;
		     } ;
/*		    Now get key position & length */
		    xabp->xab$w_pos0 = (tptr = (char *)(*(ptr+1)))-bufp ; xabp->xab$b_ref = j-5 ;
		    xabp->xab$b_siz0 = format.fld.length ;
		    xabp->xab$b_flg = (*(duptr + j-5) == 'Y' ? (j==5 ? XAB$M_DUP : XAB$M_CHG | XAB$M_DUP) : 0) ;
/*		    Keep track of highest byte for possible compaction */
		    if (xabp->xab$w_pos0+xabp->xab$b_siz0 > unitp->header_bytes)
		     unitp->header_bytes = xabp->xab$w_pos0+xabp->xab$b_siz0 ;
		 } ;
		return ;
#endif
#ifdef cisam
/*		Loop thru each argument */
/*		ckeys = (unitp->keydesc_ptr = v4mm_AllocChunk(sizeof *ckeys,FALSE)) ;*/		/* Allocate special key buffer */
		ckeys = (struct cisam__keys *)&ckbuf ; unitp->keydesc_ptr = (int *)&ckbuf ;
		ckeys->count = 0 ;
		for ((j=ac,ptr=base_ptr-(2*(ac+1)));j>=5;(j--,ptr+=2))
		 {  ck = &ckeys->key[j-5] ; ckeys->count ++ ;
		    ck->k_nparts = 1 ;		/* Only one part per key */
		    ck->k_flags = COMPRESS ;
		    ck->k_flags += (*(duptr+j-5) == 'Y' ? ISDUPS : ISNODUPS) ;
/*		    See what kind of key */
		    format.all = *ptr ;
		    switch (format.fld.type)
		     { default:
			v3_error(V3E_BADKEY,"IOU","MISC","BADKEY","Invalid datatype for key",unitp) ;
		       case VFT_BININT: ck->k_part[0].kp_type = LONGTYPE ; break ;
		       case VFT_BINWORD: ck->k_part[0].kp_type = INTTYPE ; break ;
		       case VFT_STRINT:
		       case VFT_VARSTR:
		       case VFT_FIXSTRSK:
		       case VFT_FIXSTR: ck->k_part[0].kp_type = CHARTYPE ; break ;
		       case VFT_PACDEC: v3_error(V3E_NOPACKCISAM,"IOU","MISC","NOPACKCISAM","Packed decimal keys not supported in C-ISAM",0) ;
		     } ;
/*		    Now get key position & length */
		    ck->k_part[0].kp_start = (tptr = (char *)(*(ptr+1)))-bufp ;
/*		    ck->k_part[0].kp_start = (tptr = (char *)xctu_pointer(*(ptr+1)))-bufp ; */
		    ck->k_part[0].kp_leng = format.fld.length ;
/*		    Keep track of highest byte for possible compaction */
		    if (ck->k_part[0].kp_start + ck->k_part[0].kp_leng > unitp->header_bytes)
		     unitp->header_bytes = ck->k_part[0].kp_start + ck->k_part[0].kp_leng ;
		 } ;
#endif
		return ;
	   case V3_FLAGS_IOM_SET_KEYS:
/*		First get the base buffer */
		psi->stack_ptr = base_ptr - (4*SIZEOFSTACKENTRY) ;
		stru_popstr(&sref) ; bufp = sref.ptr ; psi->stack_ptr = base_ptr ;
#ifdef v4is
/*if ( (unitp->file_info & V3_FLAGS_IOF_V4IS) == 0) goto skip_v4 ; */
	 	v3unitp = unitp ;
/*		Loop thru each argument */
		ki = (struct V4FFI__KeyInfo *)(unitp->keydesc_ptr = (int *)v4mm_AllocChunk(sizeof *ki,FALSE)) ;
		ki->Count = 0 ; kid = (struct V4FFI__KeyInfoDtl *)&ki->Buffer ; kid->KeyCount = 0 ;
		if (unitp->fileref != 0) { kid->FileRef = unitp->fileref ; unitp->fileref = 0 ; }
		 else if (unitp->dflt_fileref != 0) { kid->FileRef = unitp->dflt_fileref ; }
		 else kid->FileRef = 9999 ;
		for ((j=ac,ptr=base_ptr-(2*(ac+1)));j>=4;(j--,ptr+=2))
		 {  kid->Key[j-4].DupsOK = (j == 4 ? FALSE : TRUE) ;
/*		    See what kind of key */
		    format.all = *ptr ;
		    kid->Key[j-4].KeyPrefix.fld.AuxVal = kid->FileRef ;
		    switch (format.fld.type)
		     { default:
			v3_error(V3E_BADKEY,"IOU","MISC","BADKEY","Invalid datatype for key",unitp) ;
		       case VFT_BININT:
				kid->Key[j-4].KeyPrefix.fld.KeyType = j-4 ;	/* Key number = V4IS_KeyType_Frxx */
				kid->Key[j-4].KeyPrefix.fld.KeyMode = V4IS_KeyMode_Int ;
				kid->Key[j-4].KeyPrefix.fld.Bytes = sizeof i ;
				break ;
		       case VFT_BINWORD:
				v3_error(V3E_NOWORD,"IOU","MISC","NOWORD","Keys of type short are not supported in V4",0) ;
		       case VFT_STRINT:
		       case VFT_VARSTR:
		       case VFT_FIXSTRSK:
		       case VFT_FIXSTR:
				kid->Key[j-4].KeyPrefix.fld.KeyType = j-4 ;	/* Key number = V4IS_KeyType_Frxx */
				kid->Key[j-4].KeyPrefix.fld.KeyMode = V4IS_KeyMode_Alpha ;
				kid->Key[j-4].KeyPrefix.fld.Bytes = format.fld.length ;
				break ;
		       case VFT_PACDEC: v3_error(V3E_NOPACKV4,"IOU","MISC","NOPACKV4","Packed decimal keys not supported in V4",0) ;
		     } ;
/*		    Now get key position & length */
		    kid->Key[j-4].Offset = (tptr = (char *)(*(ptr+1)))-bufp ;
/*		    kid->Key[j-4].Offset = (tptr = (char *)xctu_pointer(*(ptr+1)))-bufp ; */
		    kid->KeyCount ++ ;
		 } ;
		kid->Bytes = sizeof *kid - sizeof kid->Key + (kid->KeyCount)*sizeof kid->Key[0] ;
		ki->Count ++ ; ki->Bytes = sizeof *ki - sizeof ki->Buffer + kid->Bytes ;
		v3unitp = NULL ;
/*		return ; */
skip_v4:
#endif
#ifdef VMSOS
/*		First get the base buffer */
		bufp = (char *)(*(base_ptr - 7)) ;
/*		bufp = xctu_pointer(*(base_ptr - 7)) ; */
/*		Loop thru each argument */
		for ((j=ac,ptr=base_ptr-(2*(ac+1)));j>=4;(j--,ptr+=2))
		 { xabp = malloc(sizeof *xabp) ;
		   *xabp = cc$rms_xabkey ;
		   xabp->xab$l_nxt = unitp->xabp ; unitp->xabp = xabp ;
/*		    See what kind of key */
		    format.all = *ptr ;
		    switch (format.fld.type)
		     { default:
			v3_error(V3E_BADKEY,"IOU","MISC","BADKEY","Invalid datatype for key",unitp) ;
		       case VFT_BININT: xabp->xab$b_dtp = XAB$C_IN4 ; break ;
		       case VFT_BINWORD: xabp->xab$b_dtp = XAB$C_IN2 ; break ;
		       case VFT_STRINT:
		       case VFT_VARSTR:
		       case VFT_FIXSTRSK:
		       case VFT_FIXSTR: xabp->xab$b_dtp = XAB$C_STG ; break ;
		       case VFT_PACDEC: xabp->xab$b_dtp = XAB$C_PAC ; break ;
		     } ;
/*		    Now get key position & length */
		    xabp->xab$w_pos0 = (tptr = (char *)(*(ptr+1)))-bufp ; xabp->xab$b_ref = j-4 ;
/*		    xabp->xab$w_pos0 = (tptr = xctu_pointer(*(ptr+1)))-bufp ; xabp->xab$b_ref = j-4 ; */
		    xabp->xab$b_siz0 = format.fld.length ;
		    xabp->xab$b_flg = (j == 4 ? 0 : XAB$M_CHG | XAB$M_DUP) ;
/*		    Keep track of highest byte for possible compaction */
		    if (xabp->xab$w_pos0+xabp->xab$b_siz0 > unitp->header_bytes)
		     unitp->header_bytes = xabp->xab$w_pos0+xabp->xab$b_siz0 ;
		 } ;
		return ;
#endif
#ifdef cisam
/*		Loop thru each argument */
/*		ckeys = (unitp->keydesc_ptr = (int *)v4mm_AllocChunk(sizeof *ckeys,FALSE)) ;*/ /* Allocate special key buffer */
		ckeys = (struct cisam__keys *)&ckbuf ; unitp->keydesc_ptr = (int *)&ckbuf ;
		ckeys->count = 0 ;
		for ((j=ac,ptr=base_ptr-(2*(ac+1)));j>=4;(j--,ptr+=2))
		 {  ck = &ckeys->key[j-4] ; ckeys->count ++ ;
		    ck->k_nparts = 1 ;		/* Only one part per key */
		    ck->k_flags = COMPRESS ; ck->k_flags += (j == 4 ? ISNODUPS : ISDUPS) ;
/*		    See what kind of key */
		    format.all = *ptr ;
		    switch (format.fld.type)
		     { default:
			v3_error(V3E_BADKEY,"IOU","MISC","BADKEY","Invalid datatype for key",unitp) ;
		       case VFT_BININT: ck->k_part[0].kp_type = LONGTYPE ; break ;
		       case VFT_BINWORD: ck->k_part[0].kp_type = INTTYPE ; break ;
		       case VFT_STRINT:
		       case VFT_VARSTR:
		       case VFT_FIXSTRSK:
		       case VFT_FIXSTR: ck->k_part[0].kp_type = CHARTYPE ; break ;
		       case VFT_PACDEC: v3_error(V3E_NOPACKCISAM,"IOU","MISC","NOPACKCISAM","Packed decimal keys not supported in C-ISAM",0) ;
		     } ;
/*		    Now get key position & length */
		    ck->k_part[0].kp_start = (tptr = (char *)(*(ptr+1)))-bufp ;
/*		    ck->k_part[0].kp_start = (tptr = (char *)xctu_pointer(*(ptr+1)))-bufp ; */
		    ck->k_part[0].kp_leng = format.fld.length ;
/*		    Keep track of highest byte for possible compaction */
		    if (ck->k_part[0].kp_start + ck->k_part[0].kp_leng > unitp->header_bytes)
		     unitp->header_bytes = ck->k_part[0].kp_start + ck->k_part[0].kp_leng ;
		 } ;
#endif
		return ;
	   case V3_FLAGS_IOM_BLOCKING:				/* io_misc(unit,/BLOCKING/,flag) */
#ifdef SOCKETS
		psi->stack_ptr = base_ptr - (4*SIZEOFSTACKENTRY) ;
		i = xctu_popint() ; psi->stack_ptr = base_ptr ;
		if (i > 0) { i = 0 ; } else { i = 1 ; } ;	/* Flip flag */
		if (NETIOCTL(unitp->ConnectSocket,FIONBIO,&i) != 0)
		 v3_error(V3E_CLOSESOCKETFAIL,"IO","MISC","BLOCKSOCKET","Could not set socket BLOCKing",unitp) ;
		unitp->file_info |= V3_FLAGS_IOF_NOBLOCK ;
#endif
		return ;
	   case V3_FLAGS_IOM_PEERNAME:
#ifdef ISOCKETS
		i = sizeof sin ;
#ifdef UNIX
		if ((unitp->file_info & V3_FLAGS_IOF_INETSOCKET) != 0)
	   	 { if (getpeername(unitp->ConnectSocket,&sin,&i) != 0)
		    v3_error(V3E_SOCKETACCEPT,"IO","MISC","GETPEERFAIL","Could not get PEER name",unitp) ;
		   { int *num ; num = (int *)&sin.sin_addr ; PUSHINT(*num) ; return ; }
#endif
#ifdef VMSOS
		i = sizeof sin ;
		if ((unitp->file_info & V3_FLAGS_IOF_INETSOCKET) != 0)
	   	 { if (getpeername(unitp->ConnectSocket,&sin,&i) != 0)
		    v3_error(V3E_SOCKETACCEPT,"IO","MISC","GETPEERFAIL","Could not get PEER name",unitp) ;
		   PUSHINT(sin.sin_addr.s_addr) ; return ;
#ifdef foooo
		   { unsigned int num,byte ; char *b ;
		     b = inet_ntoa(sin.sin_addr.s_addr) ;
 printf("peername=%s\n",b) ;
		     for(num=0,byte=0;*b!='\0';b++)
		      { if (*b == '.') { num << 8 ; num |= byte ; byte = 0 ; continue ; } ;
		        byte *= 10 ; byte += *b - '0' ;
printf("byte=%d %x, byte=%d %x\n",byte,byte,b,b) ;
		      } ; num << 8 ; num |= byte ;
		     PUSHINT(num) ; return ;
		   }
#endif
#endif
#ifdef WINNT
		if ((unitp->file_info & V3_FLAGS_IOF_INETSOCKET) != 0)
	   	 { if (getpeername(unitp->ConnectSocket,(struct sockaddr *)&sin,&i) != 0)
		    v3_error(V3E_SOCKETACCEPT,"IO","MISC","GETPEERFAIL","Could not get PEER name",unitp) ;
		   PUSHINT(sin.sin_addr.S_un.S_addr) ; return ;
#endif
		 } ;
#endif
		PUSHINT(0) ;					/* Don't know? */
		return ;
	   case V3_FLAGS_IOM_RECONNECT:
#ifdef SOCKETS
		if (unitp->PrimarySocket == unitp->ConnectSocket)
		 v3_error(V3E_RECONNECT,"IO","MISC","RECONNECT","Cannot reconnect non-Server socket",unitp) ;
		if (SOCKETCLOSE(unitp->ConnectSocket) != 0)
		 v3_error(V3E_CLOSESOCKETFAIL,"IO","MISC","CLOSESOCKETFAIL","Could not close socket",unitp) ;
#ifdef USOCKETS
		if ((unitp->file_info & V3_FLAGS_IOF_UNIXSOCKET) != 0)
		 { fromlen = sizeof fsaun ;
		   if ((unitp->ConnectSocket = accept(unitp->PrimarySocket,&fsaun,&fromlen)) < 0)
		    { sprintf(errbuf,"Error (%s) accept'ing on Unix Socket (%s)",v_OSErrString(errno),unitp->file_name) ;
		      v3_error(V3E_SOCKETACCEPT,"IO","OPEN","SOCKETACCEPT",errbuf,unitp) ;
		    } ;
		 } ;
#endif
#ifdef ISOCKETS
		if ((unitp->file_info & V3_FLAGS_IOF_INETSOCKET) != 0)
	   	 { fromlen = sizeof fsin ;
		   if ((unitp->ConnectSocket = accept(unitp->PrimarySocket,(struct sockaddr *)&fsin,&fromlen)) < 0)
		    { sprintf(errbuf,"Error (%s) accept'ing on INET Socket (%s)",v_OSErrString(errno),unitp->file_name) ;
		      v3_error(V3E_SOCKETACCEPT,"IO","OPEN","SOCKETACCEPT",errbuf,unitp) ;
		    } ;
		 } ;
#endif
#endif
		return ;
	   case V3_FLAGS_IOM_MANUAL_LOCK:			/* io_misc(unit,/manual_lock/,id,mode) */
#ifdef v4is
		if ((unitp->file_info & V3_FLAGS_IOF_V4IS) == 0)
		 v3_error(V3E_NOTV4IS,"IO","MISC","NOTV4IS","Function only supported on V4IS files",unitp) ;
		psi->stack_ptr = base_ptr - (5*SIZEOFSTACKENTRY) ;
		i = xctu_popint() ; j = xctu_popint() ;		/* i = TRUE/FALSE (mode), j = lock id */
		psi->stack_ptr = base_ptr ;
		pcb = (struct V4IS__ParControlBlk *)unitp->file_ptr ;
		PUSHINT(v4mm_LockSomething(pcb->AreaId,j,(i ? V4MM_LockMode_Write : V4MM_LockMode_Read),
						V4MM_LockIdType_User,0,NULL,0)) ;
		return ;
#else
		v3_error(V3E_NOTYETSUP,"IO","MISC","NOTYETSUP","Not yet supported on this version of V3",unitp) ;
#endif
	   case V3_FLAGS_IOM_HANDLER_MODULE:
		psi->stack_ptr = base_ptr - (4*SIZEOFSTACKENTRY) ;
		duptr = (char *)stru_popcstr(dupstr) ; stru_csuc(duptr) ;
		unitp->handler_mep = (struct db__module_entry *)pcku_module_defined(duptr) ;
		if (unitp->handler_mep == NULL) v3_error(0,"IO","MISC","UNDEF","Module not currently defined",0) ;
		psi->stack_ptr = base_ptr ; return ;
	   case V3_FLAGS_IOM_GET_BUF:
		if (unitp->get_ptr != NULLV)
		 { PUSHMP(unitp->get_ptr) ; PUSHF(unitp->get_format.all) ;
		 } else
		 { if (unitp->dflt_get_ptr == NULLV)
		    v3_error(V3E_NOPUTBUF,"IO","MISC","NOGETBUF","No get record buffer has been specified",unitp) ;
		   PUSHMP(unitp->dflt_get_ptr) ; PUSHF(unitp->dflt_get_format.all) ;
		 } ;
		return ;
	   case V3_FLAGS_IOM_GET_MODE:
		if (unitp->get_mode == 0) { PUSHINT(unitp->dflt_get_mode) ; } else { PUSHINT(unitp->get_mode) ; } ;
		return ;
	   case V3_FLAGS_IOM_PUT_BUF:
		if (unitp->put_ptr != NULLV)
		 { PUSHMP(unitp->put_ptr) ; PUSHF(unitp->put_format.all) ;
		 } else
		 { if (unitp->dflt_put_ptr == NULLV)
		    v3_error(V3E_NOPUTBUF,"IO","MISC","NOPUTBUF","No put record buffer has been specified",unitp) ;
		   PUSHMP(unitp->dflt_put_ptr) ; PUSHF(unitp->dflt_put_format.all) ;
		 } ;
		return ;
	   case V3_FLAGS_IOM_PUT_MODE:
		if (unitp->put_mode == 0) { PUSHINT(unitp->dflt_put_mode) ; } else { PUSHINT(unitp->put_mode) ; } ;
		return ;
	   case V3_FLAGS_IOM_MANUAL_LOCKWAIT:
#ifdef v4is
		if ((unitp->file_info & V3_FLAGS_IOF_V4IS) == 0)
		 v3_error(V3E_NOTV4IS,"IO","MISC","NOTV4IS","Function only supported on V4IS files",unitp) ;
		psi->stack_ptr = base_ptr - (5*SIZEOFSTACKENTRY) ;
		i = xctu_popint() ; j = xctu_popint() ;		/* i = TRUE/FALSE (mode), j = lock id */
		psi->stack_ptr = base_ptr ;
		pcb = (struct V4IS__ParControlBlk *)unitp->file_ptr ;
		PUSHINT(v4mm_LockSomething(pcb->AreaId,j,(i ? V4MM_LockMode_Write : V4MM_LockMode_Read),
						V4MM_LockIdType_User,10000,NULL,1)) ;
		return ;
#else
		v3_error(V3E_NOTYETSUP,"IO","MISC","NOTYETSUP","Not yet supported on this version of V3",unitp) ;
#endif
	   case V3_FLAGS_IOM_MANUAL_UNLOCK:
#ifdef v4is
		if ((unitp->file_info & V3_FLAGS_IOF_V4IS) == 0)
		 v3_error(V3E_NOTV4IS,"IO","MISC","NOTV4IS","Function only supported on V4IS files",unitp) ;
		psi->stack_ptr = base_ptr - (4*SIZEOFSTACKENTRY) ; i = xctu_popint() ;
		pcb = (struct V4IS__ParControlBlk *)unitp->file_ptr ; mmm = V4_GLOBAL_MMM_PTR ;
		v4mm_LockRelease(pcb->AreaId,i,mmm->MyPid,pcb->AreaId,"ManualUnlock") ;
		psi->stack_ptr = base_ptr ; return ;
#else
		v3_error(V3E_NOTYETSUP,"IO","MISC","NOTYETSUP","Not yet supported on this version of V3",unitp) ;
#endif
	   case V3_FLAGS_IOM_FILL_BUF_TEXT:
		psi->stack_ptr = base_ptr - (2*SIZEOFSTACKENTRY) ; stru_popcstr(filename) ;
		psi->stack_ptr = base_ptr - (4*SIZEOFSTACKENTRY) ; stru_popstr(&sref) ;
		psi->stack_ptr = base_ptr - (5*SIZEOFSTACKENTRY) ; bytes = xctu_popint() ;
		psi->stack_ptr = base_ptr ;
		if ((fp = fopen(filename,"r")) == NULL)
		 v3_error(V3E_NOFILE,"IO","MISC","NOFILE","Could not access specified file",unitp) ;
		for(i=0;;)
		 { if ((tptr=fgets(sref.ptr,bytes,fp)) == NULL) break ;
		   j = strlen(tptr) ; sref.ptr += (j-1) ;
		   for(;;sref.ptr--) { if (*sref.ptr >= '\20' || *sref.ptr == '\0') break ; } ;
		   bytes -= j+1 ; i += j+1 ; *(++sref.ptr) = '\0' ; sref.ptr++ ;
		 } ;
		fclose(fp) ;
		PUSHINT(i) ;
		return ;
	   case V3_FLAGS_IOM_FILL_BUF_BINARY:
		psi->stack_ptr = base_ptr - (2*SIZEOFSTACKENTRY) ; stru_popcstr(filename) ;
		psi->stack_ptr = base_ptr - (4*SIZEOFSTACKENTRY) ; stru_popstr(&sref) ;
		psi->stack_ptr = base_ptr - (5*SIZEOFSTACKENTRY) ; bytes = xctu_popint() ;
		psi->stack_ptr = base_ptr ;
		if ((fp = fopen(filename,"rb")) == NULL)
		 v3_error(V3E_NOFILE,"IO","MISC","NOFILE","Could not access specified file",unitp) ;
		i = fread(sref.ptr,1,bytes,fp) ;
		fclose(fp) ;
		PUSHINT(i) ;
		return ;
	   case V3_FLAGS_IOM_BOF:
		if ((unitp->file_info & V3_FLAGS_IOF_C) == 0)
		 v3_error(0,"IO","MISC","INVBOF","BOF only allowed on C type files",unitp) ;
#ifdef WINNTxxvv
		SetFilePointer(unitp->hfile,0,NULL,FILE_BEGIN) ;
		PUSHINT(1) ; return ;
#endif
		if (fseek(unitp->file_ptr,0,SEEK_SET) != 0)	/* Position to BOF */
		 v3_error(0,"IO","MISC","SETBOF","Error in fseek() setting BOF",unitp) ;
		PUSHINT(1) ; return ;
	   case V3_FLAGS_IOM_INIT:
		bufp = (char *)unitp ;
/*		Zero out the unit */
		for (i=0;i<sizeof *unitp;i++) *(bufp++) = 0 ; unitp->file_ptr = (FILE *)-1 ;
/*		Look for "SEQUENTIAL_INPUT" or "SEQUENTIAL_OUTPUT" */
		if (func_code & V3_FLAGS_IOM_SEQ_INPUT)
		 { unitp->file_info = V3_FLAGS_IOF_EXISTS | V3_FLAGS_IOF_GET | V3_FLAGS_IOF_SEQUENTIAL ;
		   unitp->share_info = V3_FLAGS_IOS_GET ;
		   unitp->dflt_get_mode = V3_FLAGS_IOPG_NEXT ;
		   if (func_code & V3_FLAGS_IOR_COMPRESSED) unitp->record_info = V3_FLAGS_IOR_COMPRESSED ;
		 } ;
		if (func_code & V3_FLAGS_IOM_TEXT_INPUT)
		 { unitp->file_info = V3_FLAGS_IOF_EXISTS | V3_FLAGS_IOF_GET | V3_FLAGS_IOF_SEQUENTIAL | V3_FLAGS_IOF_C ;
		   unitp->share_info = V3_FLAGS_IOS_GET ; unitp->record_info = V3_FLAGS_IOR_CARRIAGE_RETURN ;
		   unitp->dflt_get_mode = V3_FLAGS_IOPG_NEXT ;
		   if (func_code & V3_FLAGS_IOR_COMPRESSED) unitp->record_info = V3_FLAGS_IOR_COMPRESSED ;
		 } ;
		if (func_code & V3_FLAGS_IOM_SEQ_OUTPUT)
		 { unitp->file_info = V3_FLAGS_IOF_CREATE | V3_FLAGS_IOF_PUT | V3_FLAGS_IOF_SEQUENTIAL ;
		   unitp->dflt_put_mode = V3_FLAGS_IOPG_NEXT ;
		   unitp->record_info = V3_FLAGS_IOR_CARRIAGE_RETURN | V3_FLAGS_IOR_VARIABLE_LENGTH ;
		   if (func_code & V3_FLAGS_IOR_COMPRESSED) unitp->record_info |= V3_FLAGS_IOR_COMPRESSED ;
		 } ;
		if (func_code & V3_FLAGS_IOM_TEXT_OUTPUT)
		 { unitp->file_info = V3_FLAGS_IOF_CREATE | V3_FLAGS_IOF_PUT | V3_FLAGS_IOF_SEQUENTIAL | V3_FLAGS_IOF_C ;
		   unitp->dflt_put_mode = V3_FLAGS_IOPG_NEXT ; unitp->record_info = V3_FLAGS_IOR_CARRIAGE_RETURN ;
		   unitp->record_info = V3_FLAGS_IOR_CARRIAGE_RETURN | V3_FLAGS_IOR_VARIABLE_LENGTH ;
		   if (func_code & V3_FLAGS_IOR_COMPRESSED) unitp->record_info |= V3_FLAGS_IOR_COMPRESSED ;
		 } ;
		if (func_code & V3_FLAGS_IOM_EXECUTABLE) unitp->file_info |= V3_FLAGS_IOF_EXECUTABLE ;
		return ;
	 } ;
}

/*	H E R E   T O   H A N D L E   C O N T R O L   P		*/

void iou_ctrlp(channel)
   int channel ;			/* Channel to output to */
{ struct iou__unit *unitp ;
   short int i ;
   char buf[200] ;
#ifdef v4is
   struct V4IS__ParControlBlk *pcb ;
   struct V4IS__AreaCB *acb ;
#endif
   struct vms__dsc {
     short int length ;		/* Length of value */
     short int desc ;		/* Descriptive info */
     char *pointer ;		/* Pointer to value */
    } strdsc ;

/*	Loop thru open unit list & print out anything that looks interesting */
	for(i=0;i<ounits.count;i++)
	 {
/*	   Got a live one, print out statistics */
	   unitp = ounits.unit[i].unitp ;
	   if (unitp->file_info & V3_FLAGS_IOF_V4IS)
	    {
#ifdef v4is
	      pcb = (struct V4IS__ParControlBlk *)unitp->file_ptr ;
	      acb = (struct V4IS__AreaCB *)v4mm_ACBPtr(pcb->AreaId) ;
	      if (acb->CSaid != UNUSED)			/* Accessing via remote server ? */
	       { printf("\n  %s(remote %d) - %d gets, %d puts\r",
			ounits.unit[i].file_name,acb->CSaid,unitp->get_count,unitp->put_count) ;
	       } else { printf("\n  %s(%d) - %d gets, %d puts\r",
			ounits.unit[i].file_name,v4mm_AreaSegId(pcb->AreaId),unitp->get_count,unitp->put_count) ;
		      } ;
#endif
	    } else { printf("\n  %s - %d gets, %d puts\r",ounits.unit[i].file_name,unitp->get_count,unitp->put_count) ; } ;
	 } ;
/*	That's it */
	return ;
}

/*	U S E R   T T Y   R O U T I N E S			*/

#ifdef ISOCKETS
/*	iou_OpenTerminalSocket - Opens Socket for "terminal" IO	*/
/*	Call: iou_OpenTerminalSocket(hostname,port,reset)		*/

void iou_OpenTerminalSocket(hostname,port,reset)
  char *hostname ; int port,reset ;
{
   struct sockaddr_in sin ;
   struct hostent *hp ;
   static char SaveHost[100] ; static int SavePort ;
#ifdef WINNT
  OFSTRUCT ofs ;
  WSADATA wsad ; int i ;
#endif

#ifdef WINNT
	if (!didWSAStartup)
	 { didWSAStartup = TRUE ; i = MAKEWORD(1,1) ;		/* Take first version */
	   WSAStartup(i,&wsad) ;
	 } ;
#endif
	if (reset) { hostname = SaveHost ; port = SavePort ; SOCKETCLOSE(termSocket) ; }
	 else { strcpy(SaveHost,hostname) ; SavePort = port ; }
	termSocket = socket(AF_INET, SOCK_STREAM,0) ;	/* Connect to packer server */
	if (termSocket < 0)
	 { printf("? Error (%s) creating socket",v_OSErrString(NETERROR)) ; exit(EXIT_FAILURE) ; } ;
	memset(&sin,0,sizeof sin) ;
	sin.sin_family = AF_INET ; sin.sin_port = htons(port) ;
	if ((hp = gethostbyname(hostname)) == NULL)
	 { printf("? Error (%s) obtaining network address for host (%s)\n",v_OSErrString(NETERROR),hostname) ; exit(EXIT_FAILURE) ; } ;
	memcpy(&sin.sin_addr,hp->h_addr,hp->h_length) ;
	if (connect(termSocket,(struct sockaddr *)&sin,sizeof sin) < 0)
	 { printf("? Error (%s) connecting INET socket (%d) to host (%s.%d)\n",v_OSErrString(NETERROR),termSocket,hostname,port) ; exit(EXIT_FAILURE) ; } ;
}
#endif

/*	iou_input_tty - Inputs from user terminal (SYS$INPUT) */
int iou_input_tty(prompt,result,maxlen,parse_flag)
  char *prompt ;		/* Prompt string */
  char *result ;		/* Result string to be updated */
  int maxlen ;			/* Maximum input length */
  int parse_flag ;		/* TRUE iff called from V3 parser otherwise from runtime */
{ int i ;
  struct lcl__PreInput		/* Structure holds "pre-input" strings */
   { int used ;			/* Number used as tty input */
     int saved ;		/* Number currently saved */
     struct {
       char *text ;		/* Pointer to text */
      } entry[20] ;
   } ;
  static struct lcl__PreInput *lpi ;
#ifdef WINNT
   HANDLE hOutput ;
   int isConsole ;
   int res ; char *cp ; char errbuf[250] ;
#endif

	if (result == NULL)		/* Maybe save input ? */
	 { if (lpi == NULL) lpi = (struct lcl__PreInput *)v4mm_AllocChunk(sizeof *lpi,TRUE) ;
	   lpi->entry[lpi->saved].text = (char *)v4mm_AllocChunk(strlen(prompt)+2,TRUE) ;
	   strcpy(lpi->entry[lpi->saved].text,prompt) ; lpi->saved++ ;
	   return(TRUE) ;			/* Store for later use */
	 } ;
	if (lpi != NULL)		/* Don't go get input if we already have it */
	 { if (lpi->used < lpi->saved)
	    { printf("%s%s\n",prompt,lpi->entry[lpi->used].text) ;	/* "Echo" it */
	      strcpy(result,lpi->entry[lpi->used].text) ;
	      lpi->used++ ; return(TRUE) ;
	    } ;
	 } ;
#ifdef VMSOS
	return(vms_input_tty(prompt,result,maxlen,parse_flag)) ;
#endif

#ifdef WINNT
	hOutput = GetStdHandle(STD_OUTPUT_HANDLE) ; isConsole = termGetConsoleMode(hOutput,&res) ;
start_again:
	termWriteFile(hOutput,prompt,strlen(prompt),&res) ;
	for(cp=result,i=0;i<maxlen;cp++)
	 {
	   if (termSocket != 0)
	    { if (recv(termSocket,cp,1,0) <= 0)
	       { sprintf(errbuf,"Error (%s) in recv on terminal socket (%d)",v_OSErrString(NETERROR),termSocket) ;
	         v3_error(V3E_SOCKETRECV,"TTY","GET","SOCKETRECV",errbuf,0) ;
	       } ;
	    } else { ReadFile(GetStdHandle(STD_INPUT_HANDLE),cp,1,&res,NULL) ; } ;
	   switch (*cp)
	    { default:
		if (!isConsole) termWriteFile(hOutput,cp,1,&res) ;
	        break ;
	      case 17: case 18: case 19: case 20: case 22:
	        cp -- ; break ;
	      case 26:		/* ^Z */
		if (parse_flag) exit(process->exit_value=EXIT_SUCCESS) ;
		 v3_error(V3E_EOF,"TTY","GET","EOF","End of file reached (^Z)",0) ;
	      case 21:		/* Control U */
	        if (!isConsole) termWriteFile(hOutput,"\r\n",2,&res) ;
	        goto start_again ;
	      case 127:		/* Delete */
		if (!isConsole) termWriteFile(hOutput,"\010 \010",3,&res) ;
		cp -= 2 ; i -= 2 ; break ;
	      case '\r':
		if (isConsole) { cp -- ; i -- ; break ; } ;	/* If Console then ignore CR, else drop thru */
	      case '\n':
		if (!isConsole) termWriteFile(hOutput,"\r\n",2,&res) ;
	      	*cp = 0 ; if (cp != result) { cp-- ; if (*cp < 20) *cp = 0 ; } ;
		return(TRUE) ;
	    } ;
	 } ;
	res = GetLastError() ;
	printf("Error %s on read\n",v_OSErrString(res)) ;
	*cp = 0 ; return(TRUE) ;
#else
	printf("%s",prompt) ;
	if(gets(result) == 0)
	 { if (parse_flag) exit(process->exit_value=EXIT_SUCCESS) ;
	   v3_error(V3E_EOF,"TTY","GET","EOF","End of file reached (^Z)",0) ;
	 } ;
	i = strlen(result) ; if (*(result+i-1) == '\n') *(result+i-1) = 0 ;	/* Strip terminating newline if there */
	return(TRUE) ;
#endif
}

/*	iou_scr_ansi - Converts /row.col,attributes/ into ANSII codes */

char *iou_scr_ansi()
{ union flag__ref attr ;	/* Holds the attributes */
   union val__format format ;
   char tmpstr[300],posstr[100] ;
   static char res[500] ;	/* Result returned in this buffer */
   int row,col ;

/*	First pull arguments off of V3 stack */
	POPF(format.all) ; PUSHF(format.all) ;
	if (format.fld.type == VFT_BININT || format.fld.type == VFT_BINWORD || format.fld.type == VFT_BINLONG)
	 { col = xctu_popint() ; row = xctu_popint() ; stru_popcstr(tmpstr) ; attr.all = xctu_popint() ;
	 } else
	 { stru_popcstr(tmpstr) ; attr.all = xctu_popint() ; row = attr.fld.byte3 ; col = attr.fld.byte2 ;
	 } ;
	POPFV ;			/* Pop off EOA */
	res[0] = 0 ;
start_conversion:
	sprintf(posstr,"\33[%d;%dH",row,col) ;	/* Position cursor */
	strcat(res,posstr) ;
/*	See if any special attribute flags */
	if (attr.all &
		 (V3_FLAGS_SCR_BLINK | V3_FLAGS_SCR_BOLD | V3_FLAGS_SCR_DOUBLE_HEIGHT | V3_FLAGS_SCR_DOUBLE_WIDTH |
		  V3_FLAGS_SCR_REVERSE_VIDEO | V3_FLAGS_SCR_UNDER_LINE))
	 { strcat(res,"\33[") ;
#define AA(flag,str) if (attr.fld.mask & flag) strcat(res,str)
	   AA(V3_FLAGS_SCR_BLINK,";5") ; AA(V3_FLAGS_SCR_BOLD,";1") ; AA(V3_FLAGS_SCR_REVERSE_VIDEO,";7") ;
	   AA(V3_FLAGS_SCR_UNDER_LINE,";4") ;
	   strcat(res,"m") ; strcat(res,tmpstr) ; strcat(res,"\33[m") ;
	 } else { strcat(res,tmpstr) ; } ;
/*	How about double-height/width ? */
	if (attr.all & V3_FLAGS_SCR_DOUBLE_HEIGHT)
	 { attr.all &= ~V3_FLAGS_SCR_DOUBLE_HEIGHT ;	/* Zap the flag */
	   strcat(res,"\33#3\n\33#4") ;
	   row++ ; goto start_conversion ;	/* Do second line */
	 } ;
/*	All done, return pointer to result */
	return(res) ;
}

#ifdef WINNT
static int scrolldelayms ;
#endif

/*	iou_tty_misc - Handles the tty_misc() module		*/

void iou_tty_misc()
{ struct {
     short int status ;		/* IOSB status */
     short int data_len ;	/* Number data bytes input */
     short int terminator ;	/* Terminating byte */
     short int term_len ;	/* Number of terminating bytes */
    } iosb ;
   struct {
     short int bytes ;		/* Number of bytes typeahead */
     char first_byte ;
     char reserved[5] ;
   } char_buf ;			/* TTY characteristics buffer */
   struct {
     short int len ;
     short int type ;
     char *pointer ;
    } chan_desc ;
   int func_code,res ;
#ifdef WINNT
   static int scroll1,scroll2 ;
   static WORD defcolor ;
   WORD attr ;
   HANDLE ohandle,hDC,hWnd,hFont ;
   HKEY hKey ;
   static HANDLE stdbuffer,altbuffer ;
   CONSOLE_SCREEN_BUFFER_INFO csbi ;
   COORD xypos,ixypos ;
   SMALL_RECT rect1,rect2 ;
   CHAR_INFO ci ;
   SECURITY_ATTRIBUTES winnt_sa ;
   char spaces[300] ;
   int row,col,j ;
#endif
   int ac,i ; V3STACK *base_ptr ;
   union val__format format ;
   char temp_name[250] ;		/* Temp device name */

	/* See how many arguments */
	for (ac=0;;ac++)
	 { POPF(format.all) ; POPVI(i) ; if (format.all == V3_FORMAT_EOA) break ; } ;
/*	ac = number of arguments, base_ptr = current stack pointer */
	base_ptr = psi->stack_ptr ;
	psi->stack_ptr = base_ptr - (2*SIZEOFSTACKENTRY) ; func_code = xctu_popint() ; psi->stack_ptr = base_ptr ;
	switch (func_code)
	 {
	   case V3_FLAGS_TTY_TYPEAHEAD_COUNT:
#ifdef WINNT
		GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE),&res) ;
		PUSHINT(res) ;			/* Returns number of events (includes mouse + resizing!) */
#endif
#ifdef UNIX
#ifdef FIONREAD		/* Not supported under SCO ?? */
	   	ioctl(0,FIONREAD,&res) ; PUSHINT(res) ;
#else
	   	PUSHINT(0) ;
#endif
#else
	   	PUSHINT(0) ;
#endif
		return ;
	 case V3_FLAGS_TTY_SET_DEVICE:
		return ;
	 case V3_FLAGS_TTY_FLUSH:
#ifdef UNIX
	    ioctl(0,TCFLSH,2) ;	/* Flush both input & output queues */
#endif
#ifdef WINNT
		FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE)) ;
#endif
		return ;
	 } ;
#ifdef WINNT
	if (stdbuffer == 0) stdbuffer = GetStdHandle(STD_OUTPUT_HANDLE) ;
/*	Try for WINNT functions */
	switch (func_code)
	 {
	   case 100:		/* tty_misc(100,maxrow,maxcol,colors)- Declare size of screen */
		FreeConsole() ; AllocConsole() ;		/* Grab new console */
		memset(&winnt_sa,0,sizeof winnt_sa) ; winnt_sa.nLength = sizeof winnt_sa ;
		winnt_sa.lpSecurityDescriptor = NULL ; winnt_sa.bInheritHandle = TRUE ;
		ohandle = CreateConsoleScreenBuffer(GENERIC_READ+GENERIC_WRITE,0,&winnt_sa,CONSOLE_TEXTMODE_BUFFER,NULL) ;
		psi->stack_ptr = base_ptr - (5*SIZEOFSTACKENTRY) ;
		defcolor = xctu_popint() ; xypos.X = xctu_popint() ; xypos.Y = xctu_popint() ; psi->stack_ptr = base_ptr ;
		if (defcolor == 0) defcolor = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY ;
		SetConsoleScreenBufferSize(ohandle,xypos) ;
		ixypos.X = 0 ; ixypos.Y = 0 ; SetConsoleCursorPosition(ohandle,ixypos) ;
		FillConsoleOutputAttribute(ohandle,defcolor,xypos.X*xypos.Y,ixypos,&res) ;
		SetConsoleTextAttribute(ohandle,defcolor) ;
		SetConsoleMode(ohandle,ENABLE_LINE_INPUT+ENABLE_ECHO_INPUT+ENABLE_PROCESSED_INPUT+ENABLE_PROCESSED_OUTPUT) ;
		SetConsoleActiveScreenBuffer(ohandle) ;
		stdbuffer = ohandle ; SetStdHandle(STD_OUTPUT_HANDLE,ohandle) ;
		SetStdHandle(STD_ERROR_HANDLE,ohandle) ;
		return ;
	   case 101:		/* Clear EOS */
		ohandle = stdbuffer ; GetConsoleScreenBufferInfo(ohandle,&csbi) ;
		i = csbi.dwCursorPosition.Y * csbi.dwSize.X + csbi.dwCursorPosition.X ;	/* Current Position */
		j = csbi.dwSize.X * csbi.dwSize.Y ;					/* Max "position" */
		if (defcolor == 0) defcolor = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY ;
		FillConsoleOutputCharacter(ohandle,' ',j-i,csbi.dwCursorPosition,&res) ;
		FillConsoleOutputAttribute(ohandle,defcolor,j-i,csbi.dwCursorPosition,&res) ;
		return ;
	   case 102:		/* tty_misc(102,row,col)- Position cursor */
		ohandle = stdbuffer ;
		psi->stack_ptr = base_ptr - (4*SIZEOFSTACKENTRY) ;
		xypos.X = xctu_popint()-1 ; xypos.Y = xctu_popint()-1 ; psi->stack_ptr = base_ptr ;
		SetConsoleCursorPosition(ohandle,xypos) ;
		return ;
	   case 103:		/* Clear EOL  */
		ohandle = stdbuffer ; GetConsoleScreenBufferInfo(ohandle,&csbi) ;
		FillConsoleOutputCharacter(ohandle,' ',csbi.dwSize.X-csbi.dwCursorPosition.X,csbi.dwCursorPosition,&res) ;
		FillConsoleOutputAttribute(ohandle,defcolor,csbi.dwSize.X-csbi.dwCursorPosition.X,csbi.dwCursorPosition,&res) ;
		return ;
	   case 104:		/* tty_misc(104,row,col,width,attributes)- set screen attributes */
		ohandle = stdbuffer ;
		psi->stack_ptr = base_ptr - (6*SIZEOFSTACKENTRY) ; attr = xctu_popint() ; res = xctu_popint() ;
		xypos.X = xctu_popint()-1 ; xypos.Y = xctu_popint()-1 ;
		SetConsoleCursorPosition(ohandle,xypos) ; psi->stack_ptr = base_ptr ;
		FillConsoleOutputAttribute(ohandle,attr,res,xypos,&res) ;
		return ;
	   case 105:		/* tty_misc(105,row,col,width) - clear field */
		ohandle = stdbuffer ;
		psi->stack_ptr = base_ptr - (5*SIZEOFSTACKENTRY) ; res = xctu_popint() ;
		xypos.X = xctu_popint()-1 ; xypos.Y = xctu_popint()-1 ;
		SetConsoleCursorPosition(ohandle,xypos) ; psi->stack_ptr = base_ptr ;
		FillConsoleOutputCharacter(ohandle,' ',res,xypos,&j) ;
		FillConsoleOutputAttribute(ohandle,defcolor,res,xypos,&j) ;
		return ;
	   case 106:		/* tty_misc(106) - return maxrow*1000+maxcol */
		ohandle = stdbuffer ;
		GetConsoleScreenBufferInfo(ohandle,&csbi) ;
		PUSHINT(csbi.dwSize.Y*1000+csbi.dwSize.X) ;
		return ;
	   case 107:		/* tty_misc(107,row1,row2) - set scrolling region */
		ohandle = stdbuffer ;
		psi->stack_ptr = base_ptr - (4*SIZEOFSTACKENTRY) ;
		scroll2 = xctu_popint()-1 ; scroll1 = xctu_popint()-1 ; psi->stack_ptr = base_ptr ;
		return ;
	   case 108:		/* tty_misc(108,width)- set window width */
		ohandle = stdbuffer ;
		GetConsoleScreenBufferInfo(ohandle,&csbi) ;
		psi->stack_ptr = base_ptr - (3*SIZEOFSTACKENTRY) ; res = xctu_popint() - 1 ; psi->stack_ptr = base_ptr ;
		switch(res)
		 { case 79:	j = 18 << 16 ; break ;
		   case 131:	j = 12 << 16 ; break ;
		   default:	j = UNUSED ; break ;
		 } ;
		if (j != UNUSED)
		 { 
/*		   GetConsoleTitle(temp_name,sizeof temp_name) ;
		   hWnd = FindWindow(NULL,temp_name) ;
		   hDC = GetWindowDC(hWnd) ;
		   j = -MulDiv(j,GetDeviceCaps(hDC,LOGPIXELSY),72) ;
		   hFont = CreateFont(j,-j,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
				CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"Lucida Console") ;
		   j = SelectObject(hDC,hFont) ; /* CloseHandle(hFont) ;
		   ReleaseDC(hWnd,hDC) ;
*/
		   for(;;)
		    { 
		      if (RegOpenKeyEx(HKEY_CURRENT_USER, "Console", 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) break ;
		      RegSetValueEx(hKey, "FontSize", 0, REG_DWORD, (BYTE *)&j, 4) ;
		      RegCloseKey(hKey);
		      if (RegOpenKeyEx(HKEY_CURRENT_USER, "Console\\Command Prompt", 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) break ;
		      RegQueryValueEx(hKey, "FontSize", 0, &row, (LPBYTE)&ac, &i) ;
		      RegSetValueEx(hKey, "FontSize", 0, REG_DWORD, (BYTE *)&j, 4) ;
		      RegCloseKey(hKey);
		      FreeConsole() ; AllocConsole() ;
		      break ;
		    } ;
		 } ;
		rect1.Top = 0 ; rect1.Bottom = 24-1 ; rect1.Left = 0 ; rect1.Right = res-1 ;
		GetConsoleScreenBufferInfo(ohandle,&csbi) ;
		if (rect1.Right > csbi.dwMaximumWindowSize.X-1) rect1.Right = csbi.dwMaximumWindowSize.X-1 ;
		SetConsoleWindowInfo(ohandle,TRUE,&rect1) ;
		return ;
	   case 109:		/* tty_misc(109,dir)- scroll current region */
		ohandle = stdbuffer ; GetConsoleScreenBufferInfo(ohandle,&csbi) ;
		if (scrolldelayms <= 0) scrolldelayms = 100 ;
		Sleep(scrolldelayms) ;
		psi->stack_ptr = base_ptr - (3*SIZEOFSTACKENTRY) ; res = xctu_popint() ;
		psi->stack_ptr = base_ptr ;
		rect1.Top = scroll1 ; rect1.Bottom = scroll2 ; rect1.Left = 0 ; rect1.Right = csbi.dwSize.X-1 ;
		rect2 = rect1 ; xypos.X = 0 ; xypos.Y = (res < 0 ? rect1.Top-1 : rect1.Top+1) ;
		ci.Char.AsciiChar = ' ' ; ci.Attributes = defcolor ;
		ScrollConsoleScreenBuffer(ohandle,&rect1,&rect2,xypos,&ci) ;
		return ;
	   case 110:		/* tty_misc(110) - Flip to alternate screen buffer */
	   	if (altbuffer == 0)
	   	 { stdbuffer = GetStdHandle(STD_OUTPUT_HANDLE) ;
	   	   altbuffer = CreateConsoleScreenBuffer(GENERIC_READ+GENERIC_WRITE,FILE_SHARE_READ+FILE_SHARE_WRITE,NULL,
	   	   			CONSOLE_TEXTMODE_BUFFER,NULL) ;
	   	 } ;
	   	SetConsoleActiveScreenBuffer(altbuffer) ;
	   	return ;
	   case 111:		/* tty_misc(111) - Flip to primary screen buffer */
	   	SetConsoleActiveScreenBuffer(stdbuffer) ;
	   	return ;
	   case 112:		/* tty_misc(112) - Return last recorded mouse position- X */
		res = process->MouseX ; PUSHINT(res) ; return ;
	   case 113:		/* tty_misc(113) - Return last recorded mouse position- Y */
		res = process->MouseY ; PUSHINT(res) ; return ;
	   case 114:		/* tty_misc(114) - Return last recorded mouse clicks 1=single, 2=double, ... */
		res = process->Clicks ; PUSHINT(res) ; return ;
	   case 115:		/* tty_misc(115) - Return last recorded mouse button: 1=left, 2=right, 3=middle */
		res = process->Button ; PUSHINT(res) ; return ;
	   case 116:		/* tty_misc(116) - Return last recorded other keys: 1=shift, 2=ctrl, 4=alt */
		res = process->OtherKeys ; PUSHINT(res) ; return ;
	 } ;
#endif
	v3_error(V3E_INVFUNC,"TTY","MISC","INVFUNC","Invalid TTY_MISC function",(void *)func_code) ;
}

/*	T T Y   I N P U T   ( C R T )   R O U T I N E		*/

/*	iou_scr_get - Reads input from crt screen		*/

int iou_scr_get(buffer,length,flags,maxtime,mask)
  char *buffer ;	/* Pointer to user buffer for input */
  int length ;			/* Number of bytes to input */
  int flags ;			/* User flags */
  int maxtime ;			/* Number of seconds to wait */
  char *mask ;			/* NULL or data input mask */
{ struct {
     short int status ;		/* IOSB status */
     short int data_len ;	/* Number data bytes input */
     short int terminator ;	/* Terminating byte */
     short int term_len ;	/* Number of terminating bytes */
    } iosb ;

#ifdef UNIX
   struct termio tio,tiobkp ;
#endif
#ifdef WINNT
   HANDLE ihandle,ohandle ;
   int conmode,isConsole ;
#endif

   char *input_ptr ;	/* Pointer into buffer */
   char byte,*esc_start,*ep ;	/* Start of escape sequence (after escape) & temp pointer */
   short int input_len ;	/* Number bytes to input (remaining) */
   int count ;
   int looper,innerlooper,func,vms_res,waittime ; int i,j ;
   struct {
     short int len ;
     short int type ;
     char *pointer ;
    } chan_desc ;
   struct {
     int mask_size ;
     int *ptr ;
     int mask[4] ;
    } read_term ;
   struct {
     short int bytes ;		/* Number of bytes typeahead */
     char first_byte ;
     char reserved[5] ;
   } char_buf ;			/* TTY characteristics buffer */

#ifdef WINNT
/*	Set up some pointers */
	input_ptr = buffer ; input_len = 0 ; waittime = time(NULLV) ;
/*	Set up proper terminal attributes */
	ihandle = GetStdHandle(STD_INPUT_HANDLE) ; ohandle = GetStdHandle(STD_OUTPUT_HANDLE) ;
	isConsole = termGetConsoleMode(ihandle,&conmode) ;		/* Save current attributes */
start_again:
	if (isConsole)
	 SetConsoleMode(ihandle,ENABLE_LINE_INPUT+ENABLE_MOUSE_INPUT
			+ ((flags & (V3_FLAGS_TTY_PASSALL+V3_FLAGS_TTY_NOECHO)) == 0 ? ENABLE_ECHO_INPUT : 0)) ;

/*	Are we in passall mode ? */
	if (flags & V3_FLAGS_TTY_PASSALL)
	 { *input_ptr = iou_ReadEvent(ihandle,maxtime,isConsole,FALSE) ;
	   if (*input_ptr == 0) goto got_timeout ;
	   if (*buffer == 10) { *buffer = 13 ; } ;		/* Convert LF to CR */
	   input_ptr ++ ; if (*buffer == 27) goto got_escape ; input_len ++ ; goto all_done ;
	 } ;
/*	Here to read "length" bytes or until a terminator */
	for(count=0;;count++)
	 { byte = iou_ReadEvent(ihandle,maxtime,isConsole,(count>0)) ;
	   *input_ptr = byte ; input_ptr ++ ;
/*	   Check for special characters */
	   switch (byte)
	    { case 0: input_ptr-- ; continue ;			/* Ignore nulls */
	      case 17: case 18: case 19: case 20:		/* Ignore these (for some reason come thru ??? */
	      	continue ;
	      case 21:						/* Control U ^U */
		if (!(flags & V3_FLAGS_TTY_NOECHO))
		 { for(i=1;i<=input_len;i++)
		    { if (isConsole) { WriteConsole(ohandle,"\010 \010",3,&j,NULL) ; }
		       else { termWriteFile(ohandle,"\010 \010",3,&j) ; } ;
		    } ;
		 } ;
		input_ptr = buffer ; input_len = 0 ;
		continue ;
	      case 25:
		if (isConsole) WriteConsole(ohandle,"[^Y]",4,&j,NULL) ; input_len += 4 ;
		if (!(flags & V3_FLAGS_TTY_NOECHO))
		 { for(i=1;i<=input_len;i++)
		    { if (isConsole) { WriteConsole(ohandle,"\010 \010",3,&j,NULL) ; }
		       else { termWriteFile(ohandle,"\010 \010",3,&j) ; } ;
		    } ;
		 } ;
		input_ptr = buffer ; *(input_ptr++) = 13 ; input_len = 1 ;
		if (isConsole) SetConsoleMode(ihandle,conmode) ;
		raise(2) ; goto all_done ;			/* ^Y - raise INT */
	      case 16:
		if (isConsole) WriteConsole(ohandle,"[^Y]",4,&j,NULL) ; input_len += 4 ;
		if (!(flags & V3_FLAGS_TTY_NOECHO))
		 { for(i=1;i<=input_len;i++)
		    { if (isConsole) { WriteConsole(ohandle,"\010 \010",3,&j,NULL) ; }
		       else { termWriteFile(ohandle,"\010 \010",3,&j) ; } ;
		    } ;
		 } ;
		input_ptr = buffer ; input_len = 0 ;
		raise(3) ; continue ;	/* ^P - raise STP */
	      case 27:	goto got_escape ;
	      case 8:
	      case 127:			/* Delete */
		if (input_len <= 0) { input_ptr -- ; break ; } ;		/* Don't delete nothing */
		if (!(flags & V3_FLAGS_TTY_NOECHO))
		 { if (isConsole) { WriteConsole(ohandle,"\010 \010",3,&j,NULL) ; }
		    else { termWriteFile(ohandle,"\010 \010",3,&j) ; } ;
		 };
		input_ptr -= 2 ; input_len -- ; continue ;
	      case '\t':				/* Tab - treat as if terminator */
	      case 10: *(input_ptr-1) = (byte = 13) ;	/* Line feed */
	      case 13:			/* Carriage return */
	      case 15:
		goto all_done ;
	      default:			/* Not a terminator */
		if (byte > 127) goto all_done ;		/* Anything over ASCII is terminator */
		if (input_len == 0 ? ((flags & V3_FLAGS_TTY_CLEAR_AFTER_FIRST) != 0) : FALSE)
		 { if (isConsole)
		    { for(i=1;i<=length;i++) { WriteConsole(ohandle," ",1,&j,NULL) ; } ;
		      for(i=1;i<=length;i++) { WriteConsole(ohandle,"\010",1,&j,NULL) ; } ;	/* Clear remainder of field */
		    } else
		    { for(i=1;i<=length;i++) { termWriteFile(ohandle," ",1,&j) ; } ;
		      for(i=1;i<=length;i++) { termWriteFile(ohandle,"\010",1,&j) ; } ;	/* Clear remainder of field */
		    } ;
		 } ;
		if (input_len >= length ? ((flags & V3_FLAGS_TTY_TRUNCATE) != 0) : FALSE)
		 { if (isConsole) { WriteConsole(ohandle,"\007",1,&j,NULL) ; } else { termWriteFile(ohandle,"\007",1,&j) ; } ;
		   continue ;
		 } ;
/*		Maybe echo character */
		if ((flags & V3_FLAGS_TTY_NOECHO) == 0)
		 { if (isConsole) { WriteConsole(ohandle,&byte,1,&j,NULL) ; }
		    else { termWriteFile(ohandle,&byte,1,&j) ; } ;
		 } ;
		input_len ++ ;
/*		If end of field then maybe all done ? */
		if (input_len == length ? ((flags & V3_FLAGS_TTY_TRUNCATE) == 0) : FALSE)
		 { *(input_ptr++) = 15 ; goto all_done ; } ;
	    } ;
	 } ;

/*	Here to handle timeouts */
got_timeout:
	input_len = -1 ; goto all_done ;

/*	Here to handle escape sequences */
got_escape:
/*	Read remainder of escape sequence */
got_escape_continue:
#define NB for(i=0;i<100 && (byte = iou_ReadEvent(ihandle,0,isConsole,TRUE))==0;i++) {} ; *(input_ptr++) = byte ;
	for(looper=0;;looper++)
	 { if (looper > 100) v3_error(V3E_BADESC,"TTY","GET","BADESC","Invalid input escape sequence",0) ;
	   NB
	   if (byte == '[')
	    { for(;;) { NB ; if (byte < 0x30 || byte > 0x3F) break ; } ;
	    } ;
	   if (byte == ';' || byte == '?' || byte == 'O') { NB ; } ;
	   for(;;) { if (byte < 0x20 || byte > 0x2F) break ; NB ; } ;
	   if (byte >= 0x30 && byte <= 0x7E) break ;
	 } ;
	if (flags & V3_FLAGS_TTY_PASSALL) { input_len = input_ptr - buffer ; } ; /* Include escape seq in buffer for /passall/ */
	goto all_done ;
/*	Here when all done */
all_done:
	*input_ptr = 0 ;	/* Terminate with null */
	if (isConsole) SetConsoleMode(ihandle,conmode) ;
	return(input_len) ;
#endif
#ifdef UNIX
/*	Set up some pointers */
	input_ptr = buffer ; input_len = 0 ; waittime = time(NULLV) ;
/*	If first time then open the channel */

/*	Set up proper terminal attributes */
	ioctl(0,TCGETA,&tio) ;	/* Get terminal attributes */
	tiobkp = tio ;		/* Save for restore */
	looper = 0 ;
start_again:
//	if (++looper > 10000) v3_error(0,"TTY","GET","IOERRS","Too many IO read errors on stdin",0) ;
/*	VEH050216 Change v3_error to exit - Linux does not deal well with hung ports */
	if (++looper > 10000) exit(EXIT_FAILURE) ;
	tio.c_lflag = 0000001 ; /* Turn off echo, canonical processing, only allow interrupts */
	tio.c_cc[VMIN] = 1 ;	/* Want one character */
	tio.c_cc[VTIME] = (maxtime > 50 ? 50 : maxtime)*10 ;	/* Set time in 0.10 seconds */
	if ((flags & V3_FLAGS_TTY_PASSALL) == 0) ioctl(0,TCSETA,&tio) ;	/* Reset terminal attributes */

/*	Are we in passall mode ? */
	if (flags & V3_FLAGS_TTY_PASSALL)
	 { tio.c_lflag = 0000001 ; tio.c_cc[VMIN] = 0 ; tio.c_cc[VTIME] = (maxtime > 50 ? 50 : maxtime)*10 ; ioctl(0,TCSETA,&tio) ;
	   if (read(0,input_ptr,1) <= 0)
	    { if (ctrlp_interrupt) { ctrlp_interrupt = FALSE ; goto start_again ; } ;
	      if (time(NULLV) - waittime > maxtime) goto got_timeout ;
	      ioctl(0,TCSETA,&tiobkp) ; goto start_again ; 	/* Reset terminal attributes */
	    } ;
	   if (*buffer == 10) { *buffer = 13 ; } ;	/* Convert LF to CR */
	   input_ptr ++ ; if (*buffer == 27) goto got_escape ; input_len ++ ; goto all_done ;
	 } ;
/*	Here to read "length" bytes or until a terminator */
	for(innerlooper=0;innerlooper<10000;innerlooper++)
	 { if (read(0,&byte,1) <= 0)
	    { if (ctrlp_interrupt) { ctrlp_interrupt = FALSE ; goto start_again ; } ;
	      if (time(NULLV) - waittime > maxtime) { goto got_timeout ; } else { goto start_again ; } ;
	    } ;
	   *input_ptr = byte ; input_ptr ++ ;
/*	   Check for special characters */
	   switch (byte)
	    { case 0: continue ;	/* Ignore nulls */
	      case 21:			/* Control U ^U */
		if (!(flags & V3_FLAGS_TTY_NOECHO))
		 { for(i=1;i<=input_len;i++) { write(0,"\010 \010",3) ; } ; } ;	/* Backspace over data */
		input_ptr = buffer ; input_len = 0 ;
		continue ;
	      case 25:
		write(0,"[^Y]",4) ; input_len += 4 ;
		if (!(flags & V3_FLAGS_TTY_NOECHO))
		 { for(i=1;i<=input_len;i++) { write(0,"\010 \010",3) ; } ; } ;	/* Backspace over data */
		input_ptr = buffer ; *(input_ptr++) = 13 ; input_len = 1 ;
		ioctl(0,TCSETA,&tiobkp) ;	/* Reset terminal attributes (in case raise(2) below trashes us */
		raise(2) ; goto all_done ;	/* ^Y - raise INT */
	      case 16:
		write(0,"[^P]",4) ; input_len += 4 ;
		if (!(flags & V3_FLAGS_TTY_NOECHO))
		 { for(i=1;i<=input_len;i++) { write(0,"\010 \010",3) ; } ; } ;	/* Backspace over data */
		input_ptr = buffer ; input_len = 0 ;
		raise(3) ; continue ;	/* ^P - raise STP */
	      case 27:	goto got_escape ;
	      case 8:
	      case 127:			/* Delete */
		if (input_len <= 0) { input_ptr -- ; break ; } ;		/* Don't delete nothing */
		if (!(flags & V3_FLAGS_TTY_NOECHO)) { write(0,"\010 \010",3) ; } ;
		input_ptr -= 2 ; input_len -- ; continue ;
	      case '\t':				/* Tab - treat as if terminator */
	      case 10: *(input_ptr-1) = (byte = 13) ;	/* Line feed */
	      case 13:			/* Carriage return */
	      case 15:
		goto all_done ;
	      default:			/* Not a terminator */
		if (input_len == 0 ? ((flags & V3_FLAGS_TTY_CLEAR_AFTER_FIRST) != 0) : FALSE)
		 { for(i=1;i<=length;i++) { write(0," ",1) ; } ;
		   for(i=1;i<=length;i++) { write(0,"\010",1) ; } ;	/* Clear remainder of field */
		 } ;
		if (input_len >= length ? ((flags & V3_FLAGS_TTY_TRUNCATE) != 0) : FALSE)
		 { write(0,"\007",1) ; input_ptr-- ; continue ; } ;
/*		Maybe echo character */
		if ((flags & V3_FLAGS_TTY_NOECHO) == 0) { write(0,&byte,1) ; } ;
		input_len ++ ;
/*		If end of field then maybe all done ? */
		if (input_len == length ? ((flags & V3_FLAGS_TTY_TRUNCATE) == 0) : FALSE)
		 { *(input_ptr++) = 15 ; goto all_done ; } ;
	    } ;
	 } ;
/*	If we get here then read too much of something - probably a hung port - just exit */
	if (innerlooper >= 10000) exit(EXIT_FAILURE) ;

/*	Here to handle timeouts */
got_timeout:
	input_len = -1 ; goto all_done ;

/*	Here to handle escape sequences */
got_escape:
/*	Read remainder of escape sequence */
got_escape_continue:
#define NB for(i=0;i<100 && read(0,&byte,1)<=0;i++) {} ; *(input_ptr++) = byte ;
	for(looper=0;;looper++)
	 { if (looper > 100) v3_error(V3E_BADESC,"TTY","GET","BADESC","Invalid input escape sequence",0) ;
	   NB
	   if (byte == '[')
	    { for(;;) { NB ; if (byte < 0x30 || byte > 0x3F) break ; } ;
	    } ;
	   if (byte == ';' || byte == '?' || byte == 'O') { NB ; } ;
	   for(;;) { if (byte < 0x20 || byte > 0x2F) break ; NB ; } ;
	   if (byte >= 0x30 && byte <= 0x7E) break ;
	 } ;
	if (flags & V3_FLAGS_TTY_PASSALL) { input_len = input_ptr - buffer ; } ; /* Include escape seq in buffer for /passall/ */
	goto all_done ;
/*	Here when all done */
all_done:
	*input_ptr = 0 ;	/* Terminate with null */
	ioctl(0,TCSETA,&tiobkp) ;	/* Reset terminal attributes */
	return(input_len) ;
#endif

}

#ifdef WINNT
int iou_ReadEvent(handle,maxtime,isConsole,Continuation)
  HANDLE handle ;
  int maxtime,isConsole,Continuation ;
{
  fd_set sset ;				/* Socket set */
  struct timeval tev ;
  static int repeat,result ;
  INPUT_RECORD ir ;
  COMMTIMEOUTS cto ;
  int res,wait,mpX=-1,mpY=-1 ; char buf[5],errbuf[250] ;

	if ((--repeat) >= 0) return(result) ;		/* Maybe return remainder of repeats */
	wait = (maxtime < 0 ? -maxtime : maxtime*1000) ; /* < 0 is msecs, >0 is seconds */
	if (termSocket != 0)
	 { if (Continuation)
	    { FD_ZERO(&sset) ; FD_SET(termSocket,&sset) ; tev.tv_sec = 30 ; tev.tv_usec = 0 ;
	      res = select(FD_SETSIZE,&sset,NULL,NULL,&tev) ;	/* Wait for something to happen */
	      if (res <= 0)					/* If timeout then reset port */
	       { iou_OpenTerminalSocket(NULL,0,TRUE) ;
	         return(0) ;
	       } ;
	    } ;
	   if (recv(termSocket,buf,1,0) <= 0)
	    { sprintf(errbuf,"Error (%s) in recv on terminal socket (%d)",v_OSErrString(NETERROR),termSocket) ;
	      v3_error(V3E_SOCKETRECV,"TTY","GET","SOCKETRECV",errbuf,0) ;
	    } ;
	   return(buf[0]) ;
	 } ;
	if (!isConsole)			/* If going via COM port then just read char */
	 { cto.ReadIntervalTimeout = 0 ; cto.ReadTotalTimeoutMultiplier = 0 ;
	   cto.ReadTotalTimeoutConstant = wait ; cto.WriteTotalTimeoutMultiplier = 0 ;
	   cto.WriteTotalTimeoutConstant = 0 ; SetCommTimeouts(handle,&cto) ;
	   ReadFile(handle,&buf,1,&res,NULL) ;
	   if (res == 0) return(0) ;	/* If we timed out then should not have gotten anything */
	   return(buf[0]) ;
	 } ;
	for(;;)
	 { if (wait > 0)				/* Maybe have to set timeout? */
	    { if (WaitForSingleObject(handle,wait) == WAIT_TIMEOUT)
	       { repeat = 0 ; return(0) ; } ;
	    } ;
	   if (!ReadConsoleInput(handle,&ir,1,&res))	/* Read an input record */
	    {
printf("Got read error %d\n",GetLastError()) ;
	      continue ;
	    } ;
	   switch (ir.EventType)
	    { default:		continue ;		/* Unknown event type - try another */
	      case KEY_EVENT:
		if (!ir.Event.KeyEvent.bKeyDown) continue ;	/* Ignore KeyUp */
		repeat = ir.Event.KeyEvent.wRepeatCount - 1 ;	/* Save repeats */
		switch (ir.Event.KeyEvent.wVirtualKeyCode)	/* Branch on key that was pressed */
		 { default:
			result = ir.Event.KeyEvent.uChar.AsciiChar ;	/* Default is to let system translate */
			if (result < 1 || result > 127) { repeat = 0 ; continue ; } ;
			break ;
		   case VK_F1: case VK_F2: case VK_F3: case VK_F4: case VK_F5:
		   case VK_F6: case VK_F7: case VK_F8: case VK_F9: case VK_F10:
		   case VK_F11: case VK_F12:
			if (ir.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED)
			 { result = 0xA0 + (ir.Event.KeyEvent.wVirtualKeyCode - VK_F1) ; }
			 else { result = 0x90 + (ir.Event.KeyEvent.wVirtualKeyCode - VK_F1) ; } ;
			break ;
		   case VK_BACK:	result = 127 ; break ;	/* Convert BACKSPACE key to ASCII delete */
		   case VK_PRIOR:	result = 0xB0 ; break ;
		   case VK_NEXT:	result = 0xB1 ; break ;
		   case VK_END:		result = 0xB2 ; break ;
		   case VK_HOME:	result = 0xB3 ; break ;
		   case VK_LEFT:	result = 0xB4 ; break ;
		   case VK_RIGHT:	result = 0xB5 ; break ;
		   case VK_UP:
			if (ir.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED)
			 { scrolldelayms -= 25 ; if (scrolldelayms <= 0) scrolldelayms = 1 ; repeat = 0 ; continue ; } ;
		   	result = 0xB6 ; break ;
		   case VK_DOWN:
			if (ir.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED)
			 { scrolldelayms += 25 ; if (scrolldelayms > 1500) scrolldelayms = 1500 ; repeat = 0 ; continue ; } ;
		   	result = 0xB7 ; break ;
		   case VK_SELECT:	result = 0xB8 ; break ;
		   case VK_INSERT:	result = 0xB9 ; break ;
		   case VK_DELETE:	result = 0xBA ; break ;
		   case VK_HELP:	result = 0xBB ; break ;
		 } ;
		return(result) ;
	      case MOUSE_EVENT:
		switch( ir.Event.MouseEvent.dwEventFlags )
		 { case 0:
		   case DOUBLE_CLICK:
			if ((ir.Event.MouseEvent.dwButtonState & 1+2) != 0)
			 { mpX = ir.Event.MouseEvent.dwMousePosition.X ; mpY = ir.Event.MouseEvent.dwMousePosition.Y ;
			   if ((ir.Event.MouseEvent.dwButtonState & 1) != 0) { process->Button = 1 ; }
			    else if ((ir.Event.MouseEvent.dwButtonState & 2) != 0) { process->Button = 2 ; }
			    else { process->Button = 0 ; } ;
			   if (ir.Event.MouseEvent.dwEventFlags == DOUBLE_CLICK)
			    { process->MouseX = mpX ; process->MouseY = mpY ; process->Clicks = 2 ;
			      mpX = -1 ; mpY = -1 ; result = 0xBC ; return(result) ;
			    } ;
			 }
			 else { if (mpX < 0 | mpY < 0) continue ;
				process->MouseX = mpX ; process->MouseY = mpY ; process->Clicks = 1 ;
				mpX = -1 ; mpY = -1 ; result = 0xBC ; return(result) ;
			      } ;
			break ;
		 } ;
		continue ;				/* Ignore mouse events for now */
	    } ;
	 } ;
} ;
#endif

#ifdef WINNT
/*	termGetConsoleMode - Determine if connected to console or other port */

int termGetConsoleMode(oHandle,resptr)
  HANDLE oHandle ; int *resptr ;
{
	if (termSocket != 0) return(FALSE) ;
	return(GetConsoleMode(oHandle,resptr)) ;
}

/*	termWriteFile - WINNT WriteFile with possible socket action	*/

int termWriteFile(oHandle,bp,len,olenptr)
  HANDLE oHandle ;
  char *bp ; int len ; int *olenptr ;
{ int sres ; char errbuf[250] ;

	if (termSocket != 0)
	 { if ((sres = send(termSocket,bp,len,0)) == len) return(sres) ;
	   sprintf(errbuf,"Error (%s) (len = %d/%d) in send on socket (%d)",v_OSErrString(NETERROR),len,sres,termSocket) ;
	   v3_error(V3E_SOCKETSEND,"TTY","GET","SOCKETSEND",errbuf,0) ; return(0) ;
	 } else
	 { return(WriteFile(oHandle,bp,len,olenptr,NULL)) ; } ;
}
#endif

/*	A N S I I   S C R E E N   F O R M A T			*/

void iou_scr_display_rows()
{
   int col,erow,srow,row,i,os,lx,cx,x,y,r1,r2 ; char *txt ;
   int stack[10],sp ;
   struct str__ref sstr ;
   char *ip,*ip1 ; short int *pp,*pp1,*prm ; char *bp,*bps1,*bps2,*bps3 ;

	col = xctu_popint() ;	/* Number of columns */
	erow = xctu_popint() ; srow = xctu_popint() ;	/* Start & end rows */
	stru_popstr(&sstr) ; ip = sstr.ptr ;		/* Start of image */
	stru_popstr(&sstr) ; pp = (short int *)sstr.ptr ;	/* Parameters */
	stru_popstr(&sstr) ; bp = sstr.ptr ;		/* Output buffer */

	for(row=srow;row<=erow;row++)		/* Loop thru each row */
	 { bps1 = bp ;		/* Save start of line */
/*	   Set up code to position to begin of row */
	   *(bp++) = 27 ; *(bp++) = '[' ;
	   for((i=row,sp=0);i>0;) { stack[sp++] = i%10 ; i/= 10 ; } ; for(;sp>0;) { *(bp++) = '0'+stack[--sp] ; } ;
	   *(bp++) = ';' ; *(bp++) = '1' ; *(bp++) = 'H' ; bps2 = bp ;  /* Save again to check for empty line */
/*	   Calculate offset into image & parameters */
	   os = (row-1)*135 ; pp1 = pp + os ; ip1 = ip + os ;
/*	   Convert the line */
	   for((lx=0,cx=0);cx<col;cx++)
	    {
	      if ( *(pp1+cx) == NULLV ) continue ;
/*	      Got something - figure out how to get there */
	      r1 = cx & ~7 ; r2 = lx & ~7 ;
	      if ((r1 -= r2) > 0)
	       { lx = cx & ~7 ; r1 = r1 >> 3 ; for(;r1>0;r1--) { *(bp++) = '\t' ; } ;	/* Append tabs */
	       } ;
	      r1 = cx - lx ; for(;r1>0;r1--) { *(bp++) = ' ' ; } ;	/* Append spaces */

/*	      Now append special attributes */
	      x = *(pp + os + cx) ;	/* x = attributes */
	      if ((x & 0xFFF) != 0)
	       { *(bp++) = 27 ; *(bp++) = '[' ;
		 if ((x & V3_FLAGS_SCR_BOLD) != 0) { *(bp++) = '1' ; *(bp++) = ';' ; } ;
		 if ((x & V3_FLAGS_SCR_BLINK) != 0) { *(bp++) = '5' ; *(bp++) = ';' ; } ;
		 if ((x & V3_FLAGS_SCR_UNDER_LINE) != 0) { *(bp++) = '4' ; *(bp++) = ';' ; } ;
		 if ((x & V3_FLAGS_SCR_REVERSE_VIDEO) != 0) { *(bp++) = '7' ; *(bp++) = ';' ; } ;
		 *(bp-1) = 'm' ;
	       } ;
/*	      Now append actual image */
	      txt = ip + os + cx ; cx++ ; *(bp++) = (*txt == NULLV ? ' ' : *txt) ; txt++ ; 	/* Copy first byte */
	      for(prm=pp+os+cx;*(prm++)==-1;) { cx++ ; *(bp++) = (*txt == NULLV ? ' ' : *txt) ; txt++ ; } ;
/*	      Get rid of trailing spaces */
	      lx = cx ; cx-- ; /* cx updated as part of for() loop */
	      if ((x & 0xFFF) == 0 )		/* Only do this if no special attributes */
	       { for(;*(bp-1) == ' ';bp--) { lx-- ;} ; }
	       else { *(bp++) = 27 ; *(bp++) = '[' ; *(bp++) = 'm' ; 	/* Terminate special attributes */
		      if ((x & V3_FLAGS_SCR_DOUBLE_HEIGHT) != 0)
		       { bps3 = bp ; *(bp++) = 27 ; *(bp++) = '#' ; *(bp++) = '3' ; *(bp++) = 13 ; *(bp++) = 10 ;
			 *(bp++) = 27 ; *(bp++) = '#' ; *(bp++) = '4' ;
/*			 Duplicate text & attributes */
			 for(txt=bps2;txt<bps3;) { *(bp++) = *(txt++) ; } ;
		       } ;
		    } ;
	    } ;
	   if (bps2 == bp) bp = bps1 ;	/* If nothing done then back out positioning */
	 } ;
/*	All done - make sure string terminated with null */
	*(bp++) = NULLV ;
}

/*	R E P O R T   U T I L I T I E S				*/

/*	rptu_colpos - Updates string_ref & width to correct pointer in report line (returns report type) */

int rptu_colpos(str_ref,width,col_index,coltype)
  struct str__ref *str_ref ;	/* String ref argument */
  int *width ;			/* Updated with column width */
  int col_index ;		/* The column index to find */
  int coltype ;			/* Type of column (see V3_COLTYP_xxx) */
{ struct rpt__master *rm ;
   struct rpt__line *rl ;
   short int column ;

/*	The string pointer points to report line */
	rl = (struct rpt__line *)str_ref->ptr ; rm = rl->master ;
/*	If the column index is 0 then append to end of line */
	if (col_index == 0 || col_index == V3_FLAGS_EDT_RPTCAT)
	 { column = rl->current_index ; *width = 0 ; }
	 else { column = rm->col[rl->col_set-1].start[col_index-1] ;
		*width = rm->col[rl->col_set-1].length[col_index-1] ;
		rl->current_index += *width ; rm->col[rl->col_set-1].coltype[col_index-1] = coltype ;
	      } ;
/*	Now figure out the ptr for the column */
	str_ref->ptr = rl->buf + column - 1 ;
/*	And return the report type */
	return(rm->rpt_type) ;
}

/*	rptu_colwidth - Pops off format & line & determines rpt column width */

int rptu_colwidth()
{ struct rpt__master *rm ;
   struct rpt__line *rl ;
   struct str__ref sr ;
   union flag__ref flags ;
   short int length ;

/*	Pop off format & string pointer */
	flags.all = xctu_popint() ; stru_popstr(&sr) ;
/*	If string is not a report line then return its length or format */
	if (*sr.ptr != -1)
	 { length = stru_sslen(&sr) ;
	   return(flags.fld.byte3 != 0 && flags.fld.byte3 < length ? flags.fld.byte3 : length) ;
	 } ;
/*	Got a report line - if column 0 then return 0 else width of column */
	if (flags.fld.byte3 == 0) return(0) ;
	rl = (struct rpt__line *)sr.ptr ; rm = rl->master ;
	return(rm->col[rl->col_set-1].length[flags.fld.byte3-1]) ;
}

/*	rptu_lineupd - Updates column index for a report line */

void rptu_lineupd(rl,width)
  struct rpt__line *rl ;
  int width ;
{
	rl->current_index += width ;
	return ;
}

/*	S Y S T E M   Q U E U E   M O D U L E S			*/

void sys_queue(dflt_queue_name)
   char *dflt_queue_name ;
{ struct v3__queue_info *qi ;
   struct str__ref sr ;
   int i,j,k ; char mbuf[200],buf[200] ; char idb[13] ; char tbuf[50] ; UCCHAR errmsg[512] ;
   char server[100],*p ;
#ifdef ULTRIX
   char month[5] ; struct tm *lcp ; time_t tloc ;
#endif
#ifdef ALPHAOSF
   char *dp ;
#endif
#ifdef LINUX486
   char *dp ;
#endif
#ifdef WINNT
   SYSTEMTIME st ;
   char tmp[50],next[30] ;
   struct v3__date_table dt ; int v3time ;
   char days_of_week[7][12] = { "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY", "SUNDAY" } ;
#endif
#ifdef POSIX
   struct tm *lt ;
#endif

#ifdef VMSOS
	return(vax_queue(dflt_queue_name)) ;
#endif
#ifdef WINNT
	stru_popstr(&sr) ; qi = (struct v3__queue_info *)sr.ptr ;
	strcpy(qi->filename,v3_logical_decoder(qi->filename,TRUE)) ;
/*	Are we going to processing batch queue or printer ? */
	if (strcmp(dflt_queue_name,"SYS$BATCH") == 0)
	 { strcpy(buf,"AT ") ;
	   if (qi->submit_date_time == 0)		/* Submit now ? */
	    { GetLocalTime(&st) ;
	      st.wSecond += 10 ;
	      if (st.wSecond > 59) { st.wSecond -= 60 ; st.wMinute++ ; } ;
	      if (st.wMinute > 59) { st.wMinute -= 60 ; st.wHour++ ; } ;
	      if (st.wHour > 23) st.wHour = 0 ;
	      sprintf(tmp,"%02d:%02d:%02d",st.wHour,st.wMinute,st.wSecond) ;
	    } else
	    { memset(&dt,0,sizeof dt) ; dt.inp_date_time = qi->submit_date_time ; mscu_date_info(&dt) ;
	      v3time = qi->submit_date_time % (60*60*24) ;		/* Pull out time from date-time */
	      sprintf(tmp,"%02d:%02d:%02d",(v3time/(60*60)),((v3time/60)%60),(v3time%60)) ;
	      i = time(NULLV) - (60*mscu_minutes_west()) ; i -= TIMEOFFSETSEC ;
	      if ((i / (60*60*24)) != (qi->submit_date_time / (60*60*24)))	/* On a different day? */
	       { sprintf(next," /NEXT:%s",days_of_week[dt.day_of_week-1]) ;
	         strcat(tmp,next) ;
	       } ;
	    } ;
	   strcat(buf,tmp) ; strcat(buf," cmd /C \"") ; strcat(buf,qi->filename) ; strcat(buf," ") ;
	   for(i=0;i<2;i++)
	    { if (strlen(qi->parameters[i]) > 0) { strcat(buf,qi->parameters[i]) ; strcat(buf," ") ; }
	       else { strcat(buf,"x ") ; } ;
	    } ;
/*	   Take care of the log file */
	   strcat(buf," >") ;
	   if (strlen(qi->log_name) == 0 )
	    { strncpy(idb,qi->job_name,12) ; idb[12] = NULLV ; /* Truncate job name to max of 12 & convert to lower case */
	      for(i=0;i<12;i++) { if (idb[i] >= 'A' && idb[i] <= 'Z') idb[i] = idb[i] - 'A' + 'a' ; } ;
	      strcpy(mbuf,"v3_common:") ; strcat(mbuf,idb) ; strcat(mbuf,".l") ;
	      strcat(buf,v3_logical_decoder(mbuf,FALSE)) ;
	    }
	    else { strcat(buf,qi->log_name) ; } ;
	   strcat(buf,"\"") ;
	   printf("Queued with: %s\n",buf) ;
	   { struct V__SpawnArgs sa ; memset(&sa,0,sizeof sa) ; sa.argBuf = buf ; sa.waitFlags = V_SPAWNPROC_Wait ; sa.errBuf = (UCCHAR *)mbuf ; sa.fileId = UNUSED ;
//	     if (!v_SpawnProcess(NULL,buf,V_SPAWNPROC_Wait,ASCretUC(mbuf),NULL,NULL,UNUSED))
//	      v3_error(V3E_SYSERR,"SYS","SPAWN","SYSERR",mbuf,0) ;
	     if (!v_SpawnProcess(&sa)) v3_error(V3E_SYSERR,"SYS","SPAWN","SYSERR",mbuf,0) ;
	   }
	   PUSHINT(TRUE) ; return ;
	 } else		/* Here to handle printer queues */
	 {
	   p = strrchr(qi->parameters[0],']') ;
	   if (qi->parameters[0][0] == '[' && p != NULL)			/* Handle queing via vrc ? */
	    { *p = 0 ; strcpy(server,&qi->parameters[0][1]) ;
	      sprintf(buf,"xvrc -n %s -o \"%s\" -f %s",server,p+1,qi->filename) ;
	      printf("Printed with: %s\n",buf) ;
	      { struct V__SpawnArgs sa ; memset(&sa,0,sizeof sa) ; sa.argBuf = buf ; sa.waitFlags = V_SPAWNPROC_Wait ; sa.errBuf = (UCCHAR *)mbuf ; sa.fileId = UNUSED ;
//	        if (!v_SpawnProcess(NULL,buf,V_SPAWNPROC_Wait,ASCretUC(mbuf),NULL,NULL,UNUSED)) v3_error(V3E_SYSERR,"SYS","SPAWN","SYSERR",mbuf,0) ;
	        if (!v_SpawnProcess(&sa)) v3_error(V3E_SYSERR,"SYS","SPAWN","SYSERR",mbuf,0) ;
	      }
	      PUSHINT(TRUE) ; return ;
	    } ;
	   strcpy(buf,"lpr -s ") ;
	   if (strlen(qi->queuename) > 0) { strcat(buf,"-P") ; strcat(buf,qi->queuename) ; strcat(buf," ") ; } ;
           if (strlen(qi->notes) > 0) { strcat(buf,"-t \"") ; strcat(buf,qi->notes) ; strcat(buf,"\" ") ; } ;
/*         if (qi->banner <= 0) strcat(buf,"-h ") ;             */
           if (strlen(qi->setup) > 0) { strcat(buf," $") ; strcat(buf,qi->setup) ; strcat(buf," ") ; } ;
           if (qi->passall) strcat(buf,"-oraw ") ;
/*         if (qi->notify) strcat(buf,"-m ") ;                  */
           if (qi->copies > 1) { sprintf(tbuf,"-#%d ",qi->copies) ; strcat(buf,tbuf) ; } ;	   strcat(buf,qi->filename) ;
/*	   Blast out correct number of copies */
	   PUSHINT(1) ; return ;
	 } ;
#endif
#ifdef UNIX
/*	Link up to the queue information block */
	stru_popstr(&sr) ; qi = (struct v3__queue_info *)sr.ptr ;
/*	Convert file name */
	strcpy(qi->filename,v3_logical_decoder(qi->filename,TRUE)) ;
/*	Are we going to processing batch queue or printer ? */
	if (strcmp(dflt_queue_name,"SYS$BATCH") == 0)
	 { strcpy(buf,"echo \"") ; strcat(buf,qi->filename) ; strcat(buf," ") ;
	   for(i=0;i<8;i++)
	    { if (strlen(qi->parameters[i]) > 0) { strcat(buf,qi->parameters[i]) ; strcat(buf," ") ; }
	       else { strcat(buf,"\"\" ") ; } ;
	    } ;
/*	   Take care of the log file */
	   strcat(buf," >") ;
	   if (strlen(qi->log_name) == 0 )
	    { strncpy(idb,qi->job_name,12) ; idb[12] = NULLV ; /* Truncate job name to max of 12 & convert to lower case */
	      for(i=0;i<12;i++) { if (idb[i] >= 'A' && idb[i] <= 'Z') idb[i] = idb[i] - 'A' + 'a' ; } ;
	      strcat(buf,idb) ; strcat(buf,".l ") ;
	    }
	    else { strcat(buf,qi->log_name) ; } ;
	   strcat(buf," 2>&1") ;
#ifdef LINUX486
	   if (qi->submit_date_time <= 0)
	    { strcat(buf,"\" | at -qa now 1>at.tmp 2>&1") ; }
	    else { 
		   i = qi->submit_date_time % 86400 ;
		   sprintf(tbuf,"\" | at %02d:%02d ",i/3600,(i/60)%60) ; strcat(buf,tbuf) ;
		   dp = (char *)UCretASC(mscu_udate_to_ddmmmyy((qi->submit_date_time/86400)+44240)) ;
		   strncpy(tbuf,dp+3,3) ; tbuf[3] = ' ' ; tbuf[4] = '\0' ; strcat(buf,tbuf) ;
		   strncpy(tbuf,dp,2) ; tbuf[2] = ' ' ; tbuf[3] = '\0' ; strcat(buf,tbuf) ;
		   sprintf(tbuf,",%d",mscu_udate_to_yyyymmdd((qi->submit_date_time/86400)+44240)/10000) ; strcat(buf,tbuf) ;
	           strcat(buf," 1>at.tmp 2>&1") ;
		 } ;
#endif
#ifdef ALPHAOSF
	   if (qi->submit_date_time <= 0)
	    { strcat(buf,"\" | batch -k ") ; }
	    else { 
		   i = qi->submit_date_time % 86400 ;
		   sprintf(tbuf,"\" | at -k %02d:%02d ",i/3600,(i/60)%60) ; strcat(buf,tbuf) ;
		   dp = (char *)UCretASC(mscu_udate_to_ddmmmyy((qi->submit_date_time/86400)+44240)) ;
		   strncpy(tbuf,dp+3,3) ; tbuf[3] = ' ' ; tbuf[4] = '\0' ; strcat(buf,tbuf) ;
		   strncpy(tbuf,dp,2) ; tbuf[2] = ' ' ; tbuf[3] = '\0' ; strcat(buf,tbuf) ;
		   sprintf(tbuf,",%d",mscu_udate_to_yyyymmdd((qi->submit_date_time/86400)+44240)/10000) ; strcat(buf,tbuf) ;
		 } ;
#endif
#ifdef RS6000AIX
	   if (qi->submit_date_time <= 0)
	    { strcat(buf,"\" | batch") ; }
	    else { strcat(buf,"\" | at now +") ;
/*		   Adjust for time zone & maybe daylight savings, finally adjust for V3 /date_time/ */
		   i = time(NULLV) - (60*mscu_minutes_west()) ;
		   i -= TIMEOFFSETSEC ; i = (qi->submit_date_time - i) / 60 ;	/* i = number of minutes until run */
		   sprintf(mbuf,"%d minutes",i) ; strcat(buf,mbuf) ;
		 } ;
#endif
#ifdef HPUX
	   if (qi->submit_date_time <= 0)
	    { strcat(buf,"\" | batch") ; }
	    else { strcat(buf,"\" | at now +") ;
/*		   Adjust for time zone & maybe daylight savings, finally adjust for V3 /date_time/ */
		   i = time(NULLV) - (60*mscu_minutes_west()) ;
		   i -= TIMEOFFSETSEC ; i = (qi->submit_date_time - i) / 60 ;	/* i = number of minutes until run */
		   sprintf(mbuf,"%d minutes",i) ; strcat(buf,mbuf) ;
		 } ;
#endif
#ifdef SU486
	   if (qi->submit_date_time <= 0)
	    { strcat(buf,"\" | at now + 1 min") ; }
	    else { strcat(buf,"\" | at now +") ;
/*		   Adjust for time zone & maybe daylight savings, finally adjust for V3 /date_time/ */
		   i = time(NULLV) - (60*mscu_minutes_west()) ;
		   i -= TIMEOFFSETSEC ; i = (qi->submit_date_time - i) / 60 ;	/* i = number of minutes until run */
		   sprintf(mbuf,"%d minutes",i) ; strcat(buf,mbuf) ;
		 } ;
#endif
#ifdef ULTRIX
	   if (qi->submit_date_time <= 0) { i = 2 ; }		/* If submit "now" then i = 2 minutes */
	    else { i = time(NULLV) - (60*mscu_minutes_west()) ;
		   i -= TIMEOFFSETSEC ; i = (qi->submit_date_time - i) / 60 ;	/* i = number of minutes until run */
		 } ;
	   time(&tloc) ; tloc += (i*60) ; lcp = localtime(&tloc) ;
	   strncpy(month,((char *)process->ci.UnixMonthList)+lcp->tm_mon*3,3) ; month[3] = 0 ;
	   sprintf(mbuf,"\" | at %02d%02d %s %d",lcp->tm_hour,lcp->tm_min,month,lcp->tm_mday) ;
	   strcat(buf,mbuf) ;
#endif
printf("\n%s\r",buf) ;
	   { struct V__SpawnArgs sa ; memset(&sa,0,sizeof sa) ; sa.argBuf = buf ; sa.waitFlags = V_SPAWNPROC_Wait ; sa.errBuf = errmsg ; sa.fileId = UNUSED ;
//	     if (!v_SpawnProcess(NULL,buf,V_SPAWNPROC_Wait,errmsg,NULL,NULL,UNUSED)) v3_error(V3E_SYSERR,"SYS","SPAWN","SYSERR",UCretASC(errmsg),0) ;
	     if (!v_SpawnProcess(&sa)) v3_error(V3E_SYSERR,"SYS","SPAWN","SYSERR",mbuf,0) ;
	   }
	 } else
/*	   Must be going to print queue */
	 {
	   p = strrchr(qi->parameters[0],']') ;
	   if (qi->parameters[0][0] == '[' && p != NULL)			/* Handle queing via vrc ? */
	    { *p = 0 ; strcpy(server,&qi->parameters[0][1]) ;
	      sprintf(buf,"xvrc -n %s -o \"%s\" -f %s",server,p+1,qi->filename) ;
//	      if (!v_SpawnProcess(NULL,ASCretUC(buf),V_SPAWNPROC_Wait,errmsg,NULL,NULL,UNUSED))
//	       v3_error(V3E_SYSERR,"SYS","SPAWN","SYSERR",UCretASC(errmsg),NULL) ;
	      { struct V__SpawnArgs sa ; memset(&sa,0,sizeof sa) ; sa.argBuf = ASCretUC(buf) ; sa.waitFlags = V_SPAWNPROC_Wait ; sa.errBuf = errmsg ; sa.fileId = UNUSED ;
	        if (!v_SpawnProcess(&sa)) v3_error(V3E_SYSERR,"SYS","SPAWN","SYSERR",errmsg,0) ;
	      }
	      printf("Printed with: %s\n",buf) ;
	      PUSHINT(TRUE) ; return ;
	    } ;
#ifdef HPUX
	   strcpy(buf,"lp -s ") ;
	   if (strlen(qi->queuename) > 0) { strcat(buf,"-d") ; strcat(buf,qi->queuename) ; strcat(buf," ") ; } ;
	   if (qi->notify) strcat(buf,"-w ") ;
	   if (strlen(qi->notes) > 0) { strcat(buf,"-t \"") ; strcat(buf,qi->notes) ; strcat(buf,"\" ") ; } ;
	   if (qi->copies > 1) { sprintf(tbuf,"-n%d ",qi->copies) ; strcat(buf,tbuf) ; } ;
	   if (qi->banner <= 0) strcat(buf,"-onb ") ;
	   if (strlen(qi->forms) > 0) { strcat(buf,"-o") ; strcat(buf,qi->forms) ; strcat(buf," ") ; } ;
	   if (qi->delete) strcat(buf,"-c ") ;
	   strcat(buf,qi->filename) ;
//	   if (!v_SpawnProcess(NULL,ASCretUC(buf),V_SPAWNPROC_Wait,errmsg,NULL,NULL,UNUSED))
//	    v3_error(V3E_SYSERR,"SYS","SPAWN","SYSERR",UCretASC(errmsg)) ;
	   { struct V__SpawnArgs sa ; memset(&sa,0,sizeof sa) ; sa.argBuf = ASCretUC(buf) ; sa.waitFlags = V_SPAWNPROC_Wait ; sa.errBuf = errmsg ; sa.fileId = UNUSED ;
	     if (!v_SpawnProcess(&sa)) v3_error(V3E_SYSERR,"SYS","SPAWN","SYSERR",UCretASC(errmsg),0) ;
	   }
	   if (qi->delete) { remove(qi->filename) ; } ;		/* Maybe delete file after spooling */
#else
#ifdef LINUX486
	   strcpy(buf,"lpr ") ;
	   if (strlen(qi->queuename) > 0) { strcat(buf,"-P") ; strcat(buf,qi->queuename) ; strcat(buf," ") ; } ;
	   if (strlen(qi->notes) > 0) { strcat(buf,"-t \"") ; strcat(buf,qi->notes) ; strcat(buf,"\" ") ; } ;
/*	   if (qi->banner <= 0) strcat(buf,"-h ") ;		*/
	   if (strlen(qi->setup) > 0) { strcat(buf," $") ; strcat(buf,qi->setup) ; strcat(buf," ") ; } ;
	   if (qi->passall) strcat(buf,"-oraw ") ;
/*         if (qi->notify) strcat(buf,"-m ") ;			*/
	   if (qi->copies > 1) { sprintf(tbuf,"-#%d ",qi->copies) ; strcat(buf,tbuf) ; } ;
	   strcat(buf,qi->filename) ;
	   printf("%s\n",buf) ;
/*	   Blast out correct number of copies */
//	   if (!v_SpawnProcess(NULL,ASCretUC(buf),V_SPAWNPROC_Wait,errmsg,NULL,NULL,UNUSED))
//	    v3_error(V3E_SYSERR,"SYS","SPAWN","SYSERR",UCretASC(errmsg),NULL) ;
	   { struct V__SpawnArgs sa ; memset(&sa,0,sizeof sa) ; sa.argBuf = ASCretUC(buf) ; sa.waitFlags = V_SPAWNPROC_Wait ; sa.errBuf = errmsg ; sa.fileId = UNUSED ;
	     if (!v_SpawnProcess(&sa)) v3_error(V3E_SYSERR,"SYS","SPAWN","SYSERR",UCretASC(errmsg),0) ;
	   }
	   if (qi->delete) { /* remove(qi->filename) ; */ } ;		/* Maybe delete file after spooling */
#else
	   strcpy(buf,"lpr -s ") ;
	   if (strlen(qi->queuename) > 0) { strcat(buf,"-P") ; strcat(buf,qi->queuename) ; strcat(buf," ") ; } ;
	   if (strlen(qi->notes) > 0) { strcat(buf,"-t \"") ; strcat(buf,qi->notes) ; strcat(buf,"\" ") ; } ;
	   if (qi->banner <= 0) strcat(buf,"-h ") ;
	   if (strlen(qi->setup) > 0) { strcat(buf," $") ; strcat(buf,qi->setup) ; strcat(buf," ") ; } ;
	   if (qi->passall) strcat(buf,"-x ") ;
	   if (qi->notify) strcat(buf,"-m ") ;
	   if (qi->copies > 1) { sprintf(tbuf,"-#%d ",qi->copies) ; strcat(buf,tbuf) ; } ;
	   strcat(buf,qi->filename) ;
//	   printf("%s\n",buf) ;
/*	   Blast out correct number of copies */
//	   if (!v_SpawnProcess(NULL,ASCretUC(buf),V_SPAWNPROC_Wait,errmsg,UNUSED))
//	    v3_error(V3E_SYSERR,"SYS","SPAWN","SYSERR",UCretASC(errmsg),NULL,NULL) ;
	   { struct V__SpawnArgs sa ; memset(&sa,0,sizeof sa) ; sa.argBuf = ASCretUC(buf) ; sa.waitFlags = V_SPAWNPROC_Wait ; sa.errBuf = errmsg ; sa.fileId = UNUSED ;
	     if (!v_SpawnProcess(&sa)) v3_error(V3E_SYSERR,"SYS","SPAWN","SYSERR",UCretASC(errmsg),0) ;
	   }
	   if (qi->delete) { /* remove(qi->filename) ; */ } ;		/* Maybe delete file after spooling */
#endif
#endif
	 } ;
	j = 0 ;
	if (j == 0) { PUSHTRUE ; } else { PUSHFALSE ; } ;
#endif
}
/*	S E G M E N T   R O U T I N E S			*/

#ifndef VMSOS		/* VMS versions defined in v3vax.c */

#define SEG_LIST_MAX 25

struct seg__list	/* List of current open shared segments */
{ short count ;		/* Number of shared segments currently open */
   struct
    { int pointer ;		/* Pointer to the segment in this process memory */
      int end_address ;		/* Pointer to last page in this segment */
      int lock_id ;		/* If nonzero then lock-id */
      char name[50] ;		/* Name of the segment */
    } segment[SEG_LIST_MAX] ;
} ;
static struct seg__list *seglist = 0 ;

/*	iou_seg_create( buffer , name ) - Creates a segment file from buffer */

void iou_seg_create()
{ char name[250] ;		/* Holds segment file name */
   struct str__ref sstr ;
   FILE *file_ptr ;

	stru_popcstr(name) ;		/* Get the segment name */
	stru_popstr(&sstr) ;		/* And buffer */
	strcat(name,".seg") ;
	if ((file_ptr = fopen(name,"w")) == NULLV)
	 v3_error(V3E_CRESEGFILE,"SEG","CREATE","CRESEGFILE","Could not create segment file",name) ;
	fwrite(sstr.ptr,sstr.format.fld.length,1,file_ptr) ;
	fclose(file_ptr) ;
}

/*	iou_seg_lock( buffer , access ) - Attempts to lock a buffer for read/write access */

void iou_seg_lock()
{ char errbuf[150],msgbuf[150] ;

}

/*	iou_seg_share( name , access ) - Sets up shared segment */

void iou_seg_share()
{ char name[150],errbuf[150] ;

}

/*	iou_seg_unlock( buffer ) - Unlocks buffer previously locked via seg_lock */

void iou_seg_unlock()
{ char errbuf[150] ;

}

/*	iou_seg_update( buffer ) - Writes out all updated pages within a segment to disk */

void iou_seg_update()
{ char errbuf[150] ;
   struct str__ref sstr ;
  int i,flags ;

	stru_popstr(&sstr) ;		/* Get the buffer location */
/*	Look for the buffer in the seglist table */
	for(i=0;i<seglist->count;i++) { if (sstr.ptr == (char *)seglist->segment[i].pointer) break ; } ;
	if (i >= seglist->count)
	 v3_error(V3E_NOSEGMENT,"SEG","UPDATE","NOSEGMENT","Segment not entered into V3 table via SEG_SHARE",0) ;
/*	Make call to update */
}
#endif

/*	C - I S A M   U T I L I T I E S					*/

#ifdef cisam

/*	iou_cisam_error - Called to decode C-ISAM error			*/
/*	Call: iou_cisam_error(module,arg)
	  where module is module name (e.g. "GET", "PUT", etc.),
		arg is 5th V3 error argument				*/

iou_cisam_error(module,arg)
   char *module ; struct iou__unit *arg ;
{ struct iou__unit *unitp ;
   char *msg,*mne ; char buf[100] ;

	switch(iserrno)
	 { default:
		  unitp = arg ;
		  sprintf(buf,"C-ISAM error #%d, (%s,%s)",iserrno,unitp->v3_name,unitp->file_name) ;
		  msg = buf ; mne = "CISAM" ; break ;
	   case EDUPL: msg = "Key already exists" ; mne = "EXISTS" ; break ;
	   case ELOCKED: msg = "Record or file locked by another user" ; mne = "LOCKED" ; break ;
	   case EENDFILE: msg = "Begin/End of file reached" ; mne = "EOF" ; break ;
	   case ENOREC: msg = "No such record with specified key" ; mne = "NOREC" ; break ;
	   case EROWSIZE: msg = "Record too long" ; mne = "RECTOOLONG" ; break ;
	 } ;
	v3_error(0,"IO",module,mne,msg,arg) ;
}
#endif
