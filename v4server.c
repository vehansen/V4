/*	V4SERVER.C - V4IS Remote File Server

	Created 12/26/93 by Victor E. Hansen		*/

/*
Commands to compile/link on Alpha/OSF-
cc  -c -g -w v4server.c
ld -o xv4server -w -g0 -call_shared -nocount /usr/lib/cmplrs/cc/crt0.o -count \
	v4server.o v4is.o v4isu.o v4mm.o v4im.o v4eval.o v4ctx.o v4dpi.o v4atomic.o vcommon.o \
	-lm -lc -lcurses -lc
*/

#define V4S_Version_Major 1
#define V4S_Version_Minor 15

/*	1.4	09/21/94	VEH	Fixed up lal logic in v4is_SocketHandler
	1.4	09/21/94	VEH	Added closeflag to v4is_SocketHandler to force close of files if lost connection
	1.5	09/22/94	VEH	Reply sends() - if error then return, don't exit
	1.6	09/23/94	VEH	Changed error on accepting INET socket from exit() to continue
	1.7	10/06/94	VEH	Free up pcb on close of area, don't print error if 32 (broken pipe)
	1.8	10/18/94	VEH	Cleaned up stuff to run on UNIX/WINNT
	1.9	11/21/94	VEH	Added support for Cache & Reset for hashed areas
	1.10	02/09/95	VEH	Added support for Obsolete on Put()
	1.11	05/09/95	DMM	Added init and transaction ports to replace single port for comm
	1.12	06/30/95	VEH	Stuff for Alpha-VMS/MULTINET, fixed bugs from 1.11
	1.13	12/20/95	VEH	Added DataOnly option on gets
	1.14	06/11/98	VEH	Added v4s_recv to try & catch problem on cherry/riscey
	1.15	10/06/98	VEH	Added extra error checking to catch endless-loop problem
*/

#define NEED_SOCKET_LIBS 1
#include <time.h>
#include "v4defs.c"
#include <asm/ioctls.h>
#include <setjmp.h>

//#ifdef VMSOS
//#include rms
//#include <ioctl.h>
//#endif
//
//#ifdef ALPHAOSF
//#include <sys/sysinfo.h>
//#include <sys/proc.h>
//#include <ioctl.h>
//#endif
//
//#ifdef HPUX
//#include <sys/fcntl.h>
//#else if UNIX
//#include <sys/mode.h>
//#endif

#ifdef WINNT
#include <fcntl.h>
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#endif

#ifdef USOCKETS
#define SOCKETS 1
#endif
#ifdef ISOCKETS
#define SOCKETS 1
#endif

#ifdef VMSOS
#define USESELECT 1			/* If defined then use select() to schedule requests */
#endif
#ifdef ALPHAOSF
#define USESELECT 1			/* If defined then use select() to schedule requests */
#endif
#ifdef HPUX
#define USESELECT 1			/* If defined then use select() to schedule requests */
#endif
#ifdef WINNT
#define USESELECT 1			/* If defined then use select() to schedule requests */
#endif

char **startup_envp ;
int ConnectSocket, TransactPort ;
int Trace ;
void v4s_Log(char *, ...) ;

/*	lcl__arealist - Holds all open files for one or more socket connections	*/

#define V4SERVER_Area_Max 200
struct lcl__arealist {
 short count ;
 struct {
  int FileRef ;
  int Socket ;
  char UserName[16] ;
  char NodeName[16] ;
  struct V4IS__ParControlBlk *pcb ;
 } Area[V4SERVER_Area_Max] ;
} ;

main(argc,argv,envp)
  int argc ;			/* Number of arguments passed via DCL */
  char *argv[] ;		/* Argument values */
  char **envp ;			/* Environment pointer */
{
  int i,iport ;
  char socketfile[V_FileName_Max], MapFile[60] ; char *bp ;
#ifdef WINNT
  OFSTRUCT ofs ;
  WSADATA wsad ;
  static int didWSAStartup = FALSE ;
#endif
#ifdef ALPHAOSF
  int uacbuf[2] ;
#endif

#ifdef ALPHAOSF
	uacbuf[0] = SSIN_UACPROC ; uacbuf[1] = UAC_NOPRINT | UAC_SIGBUS | UAC_NOFIX ; /* Want unaligned access to blow big time! */
	setsysinfo(SSI_NVPAIRS,&uacbuf,1,0,0) ;
#endif
	v4s_Log("V4Server (v %d.%d) - Copyright 1996 - MKS, Inc.\n",V4S_Version_Major,V4S_Version_Minor) ;
	startup_envp = envp ;			/* Save startup environment */
	if (setjmp(((struct V4C__ProcessInfo *)v_GetProcessInfo())->environment) != 0) { v4s_Log("Aborting!\n") ; exit(EXITABORT) ; } ;
/*	Parse the command line */
	iport=TransactPort = 0 ; socketfile[0] = 0 ; Trace = FALSE ;
	for(i=1;i<argc;i++)
	 { if (strcmp(argv[i],"-map") == 0)
	    { strcpy(MapFile,argv[i+1]) ;
	      i++ ; continue ;
	    } ;
	   if (strcmp(argv[i],"-iport") == 0)
	    { iport = strtol(argv[i+1],&bp,10) ;
	      if (*bp != 0) { v4s_Log("? Invalid port number: %s\n",argv[i+1]) ; exit(EXITABORT) ; } ;
	      i++ ; continue ;
	    } ;
	   if (strcmp(argv[i],"-tport") == 0)
	    { TransactPort = strtol(argv[i+1],&bp,10) ;
	      if (*bp != 0) { v4s_Log("? Invalid port number: %s\n",argv[i+1]) ; exit(EXITABORT) ; } ;
	      i++ ; continue ;
	    } ;
	   if (strcmp(argv[i],"-file") == 0)
	    { strcpy(socketfile,argv[i+1]) ; iport = TransactPort = -1 ;
	      i++ ; continue ;
	    } ;
	   if (strcmp(argv[i],"-trace") == 0) { Trace = TRUE ; continue ; } ;
	   v4s_Log("? Invalid startup option: %s\n",argv[i]) ; exit(EXITABORT) ;
	 } ;
	if ((iport == 0)||(TransactPort==0))
	 { v4s_Log("? Must specify either \"-iport n -tport m\" or \"-file name\" in startup\n") ; exit(EXITABORT) ; } ;

#ifdef WINNT
	if (!didWSAStartup)
	 { didWSAStartup = TRUE ;
	   i = MAKEWORD((WORD)1,1) ;		/* Always accept highest version */
	   WSAStartup(i,&wsad) ;
	 } ;
#endif

#ifdef UCOCKETS
	if (iport == -1) { v4s_UnixSocket(socketfile) ; }
	 else { v4s_InternetSocket(iport,TransactPort,MapFile) ; } ;
#else
	if (iport == -1) { v4s_Log("? UNIX type sockets not supported in this version!\n") ; exit(EXITABORT) ; }
	 else { v4s_InternetSocket(iport,TransactPort,MapFile) ; } ;
#endif
}

#ifdef USOCKETS
v4s_UnixSocket(socketfile)
  char *socketfile ;
{
   int PrimarySocket ;
   struct sockaddr_un saun,fsaun ;
   struct hostent *hp ;
   char hostname[128] ; char *bp ; int fromlen ;

	for(;;)
	 {
#ifdef USOCKETS
	   if ((PrimarySocket = socket(AF_UNIX, SOCK_STREAM,0)) < 0)
	    { v4s_Log("Error (%s) creating Unix Socket (%s)",v_OSErrString(NETERROR),socketfile) ; exit(EXITABORT) ; } ;
	   memset(&saun,0,sizeof saun) ;
	   saun.sun_family = AF_UNIX ; strcpy(saun.sun_path,socketfile) ;
	   remove(socketfile) ;				/* Remove any existing server name */
	   if (bind(PrimarySocket,&saun,sizeof saun.sun_family + strlen(saun.sun_path)) < 0)
	    { v4s_Log("Error (%s) bind'ing Unix Socket to name (%s)",v_OSErrString(NETERROR),socketfile) ; exit(EXITABORT) ;
	    } ;
	   if (listen(PrimarySocket,5) < 0)
	    { v4s_Log("Error (%s) listen'ing on Unix Socket (%s)",v_OSErrString(NETERROR),socketfile) ; exit(EXITABORT) ;
	    } ;
	   fromlen = sizeof fsaun ;
	   if ((ConnectSocket = accept(PrimarySocket,&fsaun,&fromlen)) < 0)
	    { v4s_Log("Error (%s) accept'ing on Unix Socket (%s)\n",v_OSErrString(NETERROR),socketfile) ; exit(EXITABORT) ;
	    } ;
#else
	   v3_error(V3E_SOCKETNOTSUP,"IO","OPEN","SOCKETNOTSUP","Sockets not supported in this version of V3",0) ;
#endif
	 } ;
}
#endif

v4s_InitFileMap(mapfile,lfm)
  char *mapfile ;
  struct V4CS__FileMap *lfm ;
{
  int i,line,fx ;
  char buf[250],*refptr,*typeptr,*nameptr,*ptr,*nptr ;
  FILE *fp ;

    fp = fopen(mapfile,"r") ;     /* Open the map file with read access */
    printf("Loading -map file: %s\n",mapfile) ;
    if (fp == NULL)
    { printf("? Could not access map file: %s\n",mapfile) ; return ;
    } ;

    /* Read a line at a time from the reference file */
    for(line=1;;line++)
    { if (fgets(buf,sizeof buf,fp) == NULL)
        break ;
      /* Convert linefeeds to NULLs */
      for (i=0 ; i < sizeof(buf) ; i++)
      { if (buf[i] == 10)
        { buf[i] = NULL ;
          break ;
        }
      }
      /* Skip comments */
      if (buf[0] == '!')
        continue ;

      fx = lfm->Count ;
      refptr = strstr(buf, "REF=") ;
      if (refptr == NULL)
      { lfm->Map[fx].FileRef = -1 ;
        nptr = buf ;
      } else
      { ptr = strchr(refptr, '\t') ;
        if (ptr == NULL)
        { lfm->Map[fx].FileRef = -1 ;
          nptr = buf ;
        } else
        { *ptr = 0 ;
          lfm->Map[fx].FileRef = strtol(refptr+4,NULL,10) ;
          nptr = ptr+1 ;
        }
      }

      typeptr = strstr(nptr,"TYPE=") ;
      if (typeptr == NULL)
        lfm->Map[fx].FileType = NULL ;
      else
      { ptr = strchr(typeptr,'\t') ;
        if (ptr == NULL)
          lfm->Map[fx].FileType = NULL ;
        else
        { *ptr = lfm->Map[fx].FileType = 0 ;
          if (strcmp(typeptr+5,"RMS") == 0)
            lfm->Map[fx].FileType = 'R' ;
          else if (strcmp(typeptr+5,"V4IS") == 0)
            lfm->Map[fx].FileType = 'V' ;
          nptr = ptr+1 ;
        }
      }

      nameptr = strstr(nptr, "NAME=") ;
      if (nameptr == NULL)
        lfm->Map[fx].FileName[0] = NULL ;
      else
      { ptr = strchr(nameptr, 0) ;
        if (ptr == NULL)
          lfm->Map[fx].FileName[0] = NULL ;
        else
        { strcpy(lfm->Map[fx].FileName, nameptr+5) ;
	  printf(" Added map: %d => %s\n",lfm->Map[fx].FileRef,lfm->Map[fx].FileName) ;
          lfm->Count++ ;
        }
      }
    }
}

#ifdef ISOCKETS
v4s_InternetSocket(iport,tport,mapfile)
  int iport,tport ;
  char *mapfile ;
{
   struct sockaddr_in sin,fsin ;
   struct hostent *hp ;
   char hostname[128] ; char *bp ; int fromlen ;
   int ISocket,TSocket,ConnectSocket ;
#ifdef VMSOS
   char sbuf[250] ; int res ;
   struct vms__dsc {
     short int length ;		/* Length of value */
     short int desc ;		/* Descriptive info */
     char *pointer ;		/* Pointer to value */
    } strdsc ;
#endif
#ifdef USESELECT
#define V4S_SocketStatus_Accept 1	/* Waiting for an "accept" */
#define V4S_SocketStatus_Message 2	/* Have a message */
#define lcl_sockets_Max 100
struct lcl__arealist lal ;	/* Want to keep lal as part of top level module */
struct lcl__sockets		/* List of active sockets */
 { int SocketCount ;		/* Number in list */
   struct
    { int Socket ;
      int Port ;
      int Status ;		/* Status of the socket- V4S_SocketStatus_xxx */
    } Entry[lcl_sockets_Max] ;
 } ; struct lcl__sockets lis,lts,*lsp ;
struct V4CS__FileMap lfm ;      /* Want to keep lfm as part of top level module */
  fd_set sset,eset ;			/* Socket set */
int i,j ;
#endif
#ifdef WINNT
  SECURITY_ATTRIBUTES sa ;
  int idThread ;
  LPTHREAD_START_ROUTINE v4s_SocketHandler() ;
#endif

#ifdef USESELECT
        /* Initialize filemap from file specified in command line */
        memset(&lfm,0,sizeof lfm) ;
	v4s_InitFileMap(mapfile,&lfm) ;
#endif
	if ((ISocket = socket(AF_INET, SOCK_STREAM,0)) < 0)
	 { v4s_Log("Error (%s) creating INET Socket (%d)",v_OSErrString(NETERROR),iport) ; exit(EXITABORT) ; } ;
	memset(&sin,0,sizeof sin) ;
	sin.sin_family = AF_INET ;
	sin.sin_port = htons(iport) ;
	gethostname(hostname,sizeof(hostname)) ;		/* Get host name */
	if ((hp = gethostbyname(hostname)) == NULL)
	 { v4s_Log("Error (%s) obtaining network address of host (gethostbyname(%s))",v_OSErrString(NETERROR),hostname) ;
	   exit(EXITABORT) ;
	 } ;
	memcpy(&sin.sin_addr,hp->h_addr,hp->h_length) ;
	if (bind(ISocket,&sin,sizeof sin) < 0)
	 { v4s_Log("Error (%s) bind'ing INET Socket to name (%s %d)",v_OSErrString(NETERROR),hostname,iport) ;
	   exit(EXITABORT) ;
	 } ;
	if ((TSocket = socket(AF_INET, SOCK_STREAM,0)) < 0)
	 { v4s_Log("Error (%s) creating INET Socket (%d)",v_OSErrString(NETERROR),tport) ; exit(EXITABORT) ; } ;
	memset(&sin,0,sizeof sin) ;
	sin.sin_family = AF_INET ;
	sin.sin_port = htons(tport) ;
	memcpy(&sin.sin_addr,hp->h_addr,hp->h_length) ;
	if (bind(TSocket,&sin,sizeof sin) < 0)
	 { v4s_Log("Error (%s) bind'ing INET Socket to name (%s %d)",v_OSErrString(NETERROR),hostname,tport) ;
	   exit(EXITABORT) ;
	 } ;
#ifdef USESELECT
	lis.SocketCount = 0 ;
	lis.Entry[lis.SocketCount].Socket = ISocket ; lis.Entry[lis.SocketCount].Status = V4S_SocketStatus_Accept ;
        lis.Entry[lis.SocketCount].Port = iport ;
	lis.SocketCount++ ;
	if (listen(ISocket,5) < 0)
	 { v4s_Log("Error (%s) listen'ing on INET Socket (%s %d)",v_OSErrString(NETERROR),hostname,iport) ;
	   exit(EXITABORT) ;
	 } ;
	lts.SocketCount = 0 ;
	lts.Entry[lts.SocketCount].Socket = TSocket ; lts.Entry[lts.SocketCount].Status = V4S_SocketStatus_Accept ;
        lts.Entry[lts.SocketCount].Port = TransactPort ;
	lts.SocketCount++ ;
	if (listen(TSocket,5) < 0)
	 { v4s_Log("Error (%s) listen'ing on INET Socket (%s %d)",v_OSErrString(NETERROR),hostname,TransactPort) ;
	   exit(EXITABORT) ;
	 } ;
	memset(&lal,0,sizeof(lal)) ;
#endif
	for(;;)
	 {
#ifdef USESELECT
	   FD_ZERO(&sset) ;
	   FD_ZERO(&eset) ;
	   for(i=0;i<lis.SocketCount;i++)
	   { FD_SET(lis.Entry[i].Socket,&sset) ;
	     FD_SET(lis.Entry[i].Socket,&eset) ;
	   } ;
	   for(i=0;i<lts.SocketCount;i++)
	   { FD_SET(lts.Entry[i].Socket,&sset) ;
	     FD_SET(lts.Entry[i].Socket,&eset) ;
	   } ;
	   i = select(FD_SETSIZE,&sset,NULL,&eset,NULL) ;	/* Wait for something to happen */
           lsp = &lis ;
           for (j=0 ; j < 2 ; j++, lsp = (&lts))
           { for(i=0;i<lsp->SocketCount;i++)
	     { if (!FD_ISSET(lsp->Entry[i].Socket,&sset)) continue ;
	       FD_CLR(lsp->Entry[i].Socket,&sset) ;		/* Clear this so we don't get into error loop */
	       switch (lsp->Entry[i].Status)
	       { default:		break ;			/* ??? */
		 case V4S_SocketStatus_Accept:
			fromlen = sizeof fsin ;
			if ((ConnectSocket = accept(lsp->Entry[i].Socket,&fsin,&fromlen)) < 0)
			 { v4s_Log("Error (%s) accept'ing on INET Socket (%s %d)",v_OSErrString(NETERROR),hostname,lsp->Entry[lsp->SocketCount].Port) ;
			   lsp->Entry[i] = lsp->Entry[lsp->SocketCount-1] ; lsp->SocketCount-- ; i-- ;
			   continue ;
			 } ;
			if ((lis.SocketCount+lts.SocketCount) >= lcl_sockets_Max)
			 { v4s_Log("Exceeded max number of socket connections... dropping this one\n") ;
			   if (SOCKETCLOSE(ConnectSocket) != 0)
			    { v4s_Log("? Error (%s) in close of connect socket (%d)\n",v_OSErrString(NETERROR),ConnectSocket) ;
			    } ;
			   continue ;
			 } ;
			lsp->Entry[lsp->SocketCount].Socket = ConnectSocket ;
			lsp->Entry[lsp->SocketCount].Status = V4S_SocketStatus_Message ;
			if (Trace) v4s_Log("Connection made on TCP/IP Port %d, Socket %d\n",lsp->Entry[lsp->SocketCount].Port,ConnectSocket) ;
			lsp->SocketCount++ ;
			break ;
		 case V4S_SocketStatus_Message:
			if (!v4s_SocketHandler(lsp->Entry[i].Socket,&lal,&lfm,FALSE))
			 { v4s_Log("v4s_SocketHandler detected error on socket %d ... closing\n",lsp->Entry[i].Socket) ;
			   v4s_SocketHandler(lsp->Entry[i].Socket,&lal,&lfm,TRUE) ;	/* Close any open files */
			   if (SOCKETCLOSE(lsp->Entry[i].Socket) != 0)
			    { v4s_Log("? Error (%s) in close of socket\n",v_OSErrString(NETERROR)) ; }
			   lsp->Entry[i] = lsp->Entry[lsp->SocketCount-1] ; lsp->SocketCount-- ; i-- ;
			 } ;
			break ;
	       } ;
	     } ;
	     for(i=0;i<lsp->SocketCount;i++)
	      { if (!FD_ISSET(lsp->Entry[i].Socket,&eset)) continue ;
		FD_CLR(lsp->Entry[i].Socket,&sset) ;		/* Clear this so we don't get into error loop */
	        v4s_Log("Detected error on socket %d... closing\n",lsp->Entry[i].Socket) ;
	        v4s_SocketHandler(lsp->Entry[i].Socket,&lal,&lfm,TRUE) ;	/* Close any open files */
	        if (SOCKETCLOSE(lsp->Entry[i].Socket) != 0)
	         { v4s_Log("? Error (%s) in close of connect socket (%d)\n",v_OSErrString(NETERROR),lsp->Entry[i].Socket) ;
	         } ;
	        lsp->Entry[i] = lsp->Entry[lsp->SocketCount-1] ; lsp->SocketCount-- ;
	      } ;
	   } ;
           continue ;
#endif
	   fromlen = sizeof fsin ;
	   if ((ConnectSocket = accept(ISocket,&fsin,&fromlen)) < 0)
	    { v4s_Log("Error (%s) accept'ing on INET Socket (%s %d)",v_OSErrString(NETERROR),hostname,iport) ;
	      exit(EXITABORT) ;
	    } ;
	   if (Trace) v4s_Log("Connection made on TCP/IP Port %d, Socket %d\n",iport,ConnectSocket) ;
/*	   Have a socket - call SocketHandler via fork/thread and then loop back for another listen */
#ifdef UNIX
	   if (fork() == 0) { v4s_SocketHandler(ConnectSocket,NULL,NULL,FALSE) ; } ;
#endif
#ifdef WINNT
	   memset(&sa,0,sizeof sa) ; sa.nLength = sizeof sa ;
	   CreateThread(&sa,0,(LPTHREAD_START_ROUTINE)v4s_SocketHandler,ConnectSocket,0,0,&idThread) ;
#endif
	   if (SOCKETCLOSE(ConnectSocket) != 0)
	    { v4s_Log("? Error (%s) in close of connect socket (%d)\n",v_OSErrString(NETERROR),ConnectSocket) ;
	    } ;
	 } ;
}
#endif

/*	v4s_SocketHandler - Handles command(s) from client socket		*/
/*	Call: ok = v4s_SocketHandler( ConnectSocket )
	  where ok is TRUE if socket ok, FALSE if returning because of errors,
		ConnectSocket is socket to connected client			*/

#ifdef WINNT
LPTHREAD_START_ROUTINE v4s_SocketHandler(ConnectSocket,lalptr,lfm,closeflag)
#else
v4s_SocketHandler(ConnectSocket,lalptr,lfm,closeflag)
#endif
  int ConnectSocket ;
  struct lcl__arealist *lalptr ;
  struct V4CS__FileMap *lfm ;
  int closeflag ;
{
  struct V4CS__MsgHeader smh ;	/* Standard message header */
  struct V4CS__Msg_Error err ;
  struct V4CS__Msg_OpenFileRef ofr ;
  struct V4CS__Msg_OpenFile rof ;
  struct V4CS__Msg_FileMap rfm ;
  struct V4CS__Msg_ReqFileMapUpdate rfmu ;
  struct V4CS__Msg_ReqFileMapDelete rfmd ;
  struct V4CS__Msg_TPort rtp ;
  struct V4CS__Msg_FileRefAID fra ;
  struct V4CS__Msg_CloseAID ca ;
  struct V4CS__Msg_GetKey gkey ;
  struct V4CS__Msg_DataRec dr ;
  struct V4CS__Msg_GetNext gnext ;
  struct V4CS__Msg_GetDataId gdid ;
  struct V4CS__Msg_Put mput ;
  struct V4CS__Msg_ActionOK mao ;
  struct V4IS__ParControlBlk *cpcb, pcbp ;
  union V4CLI__Value cli ;
  struct lcl__arealist *lal ;
  struct lcl__arealist laltmp ;
  char socketfile[V_FileName_Max] ;	/* If using socket file then the file name to use */
  int i,ref,looper,blen,ok,count ; char buf[200],*bp,result ;
  static char *expand = NULL ;
  jmp_buf save_environment ;
  extern struct V4ERR__Info v4errinfo ;

/*	Link up to proper lal, from calling module or local stack */
	if (lalptr == NULL)
	 { lal = &laltmp ; memset(lal,0,sizeof *lal) ; }	/* Pull from stack */
	 else { lal = lalptr ; } ;

/*	If lost connect (closeflag = true) then close all areas for socket */
	if (closeflag)
	 { for(i=0;i<lal->count;i++)
	    { if (lal->Area[i].Socket != ConnectSocket) continue ;
#ifdef VMSOS
	      v4is_RMSClose(lal->Area[i].pcb) ;
#else
	      v4is_Close(lal->Area[i].pcb) ;
#endif
	      lal->Area[i] = lal->Area[lal->count-1] ; lal->count-- ; i-- ;
	    } ;
	   return(TRUE) ;
	 } ;

/*	Start up main loop, wait for request, service, reply, and start all over again */

	for(looper=0;;looper++)
	 {
	   if (setjmp(((struct V4C__ProcessInfo *)v_GetProcessInfo())->environment) != 0)			/* Error trap - send error back to client */
	    { err.mh.MsgType = V4CS_Msg_Error ; err.mh.Flags = 0 ;
	      if (UCstrlen(v4errinfo.UCmsg) >= sizeof err.Message)
	       { Log("Trapped error message too long (%d bytes) - truncating",UCstrlen(v4errinfo.UCmsg)) ; v4errinfo.UCmsg[(sizeof err.Message)-2] = UCEOS ; } ;
	      UCstrcpyToASC(err.Message,v4errinfo.UCmsg) ;
	      if (Trace) { v4errinfo.UCmsg[100] = UCEOS ; v4s_Log("Error Trap: %s",UCretASC(v4errinfo.UCmsg)) ; } ;
	      err.mh.Bytes = &err.Message[strlen(err.Message)] - (char *)&err ;
	      err.VStdErrNum = v4errinfo.code ; err.SysErrorNum = 0 ;
	      send(ConnectSocket,&err,err.mh.Bytes,0) ;
	      continue ;
	    } ;
#ifdef USESELECT
	   if (looper > 0) return(TRUE) ;			/* If on USESELECT then only do once (return to select()!) */
#endif
	   if ((i = v4s_recv(ConnectSocket,&smh,sizeof smh,TRUE)) <= 0)
	    {
	      if (NETERROR != 32)					/* PIPE BROKEN error when other side goes away */
	       v4s_Log("Error (%s) in v4s_recv on socket (%d)\n",v_OSErrString(NETERROR),ConnectSocket) ;
	      return(FALSE) ;
	    } ;
	   if (Trace) v4s_Log("Received request id (%d) on ConnectSocket %d\n",smh.MsgType,ConnectSocket) ;
	   switch (smh.MsgType)
	    { default:
		v4s_Log("? Unexpected server message (%d)\n",smh.MsgType) ; return(FALSE) ;
              case V4CS_Msg_ReqTPort:
		rtp.mh.MsgType = V4CS_Msg_TPort ;		/* Send OK back to client with transaction port */
		rtp.mh.Flags = 0 ;
		rtp.mh.Bytes = sizeof rtp ;
                rtp.TPort = TransactPort ;
		if (send(ConnectSocket,&rtp,sizeof rtp,0) < sizeof rtp) /* Send response back to client */
		 { v4s_Log("Error (%s) (len = %d) in sending ReqTPort ack on socket (%d)",v_OSErrString(NETERROR),sizeof rtp,ConnectSocket) ;
		   return(FALSE) ;
		 } ;
		if (Trace) v4s_Log("Request for TPort received- sent TPort (%d) back to client\n",rtp.TPort) ;
                continue ;
              case V4CS_Msg_ReqOpenFiles:
		rof.mh.MsgType = V4CS_Msg_OpenFiles ;		/* Send OK back to client with open file info */
		rof.mh.Flags = 0 ;
		rof.mh.Bytes = sizeof(rof.mh) + sizeof(rof.Count) + lal->count*sizeof(struct V4CS__OpenFile) ;
                rof.Count = lal->count ;
                for (i=0 ; i < rof.Count ; i++)
                { rof.Ofiles[i].FileRef = lal->Area[i].FileRef ;
		  rof.Ofiles[i].Socket = lal->Area[i].Socket ;
		  rof.Ofiles[i].GetCount = lal->Area[i].pcb->GetCount ;
		  rof.Ofiles[i].PutCount = lal->Area[i].pcb->PutCount ;
		  strcpy(rof.Ofiles[i].UserName,lal->Area[i].UserName) ;
		  strcpy(rof.Ofiles[i].NodeName,lal->Area[i].NodeName) ;
		  UCstrcpyToASC(rof.Ofiles[i].FileName,lal->Area[i].pcb->UCFileName) ;
		}
		if (send(ConnectSocket,&rof,rof.mh.Bytes,0) < sizeof rof) /* Send response back to client */
		 { v4s_Log("Error (%s) (len = %d) in sending ReqOpenFiles ack on socket (%d)",v_OSErrString(NETERROR),sizeof rof,ConnectSocket) ;
		   return(FALSE) ;
		 } ;
		if (Trace) v4s_Log("Request for OpenFile info received - sent info back to client\n") ;
                continue ;
              case V4CS_Msg_ReqFileMap:
		rfm.mh.MsgType = V4CS_Msg_FileMap ;		/* Send open file info back to client */
		rfm.mh.Flags = 0 ;
		rfm.mh.Bytes = sizeof(rfm.mh) + sizeof(rfm.Count) + lfm->Count*sizeof(struct V4CS__FileEnt) ;
                rfm.Count = lfm->Count ;
                for (i=0 ; i < rfm.Count ; i++)
                { rfm.File[i].FileRef = lfm->Map[i].FileRef ;
		  rfm.File[i].FileType = lfm->Map[i].FileType ;
		  strcpy(rfm.File[i].FileName,lfm->Map[i].FileName) ;
		}
		if (send(ConnectSocket,&rfm,rfm.mh.Bytes,0) < sizeof rfm) /* Send response back to client */
		 { v4s_Log("Error (%s) (len = %d) in sending FileMap Reply on socket (%d)",v_OSErrString(NETERROR),sizeof rfm,ConnectSocket) ;
		   return(FALSE) ;
		 } ;
		if (Trace) v4s_Log("Request for FileMap info received - sent info back to client\n") ;
                continue ;
              case V4CS_Msg_ReqModFileMap:
		i = v4s_recv(ConnectSocket,(char *)&rfmu + sizeof smh,smh.Bytes - sizeof smh,0) ;	/* Get rest of message */
                for (i=0 ; i < lfm->Count ; i++)
                { ref = strtol(rfmu.File.FileRef,NULL,10) ;
                  if (ref == lfm->Map[i].FileRef)
                  { if ((rfmu.File.FileType == 'V') ||
                        (rfmu.File.FileType == 'R'))
                    { if (strlen(rfmu.File.FileName) <= V4CS_MapFileName_Len)
                      { lfm->Map[i].FileType = rfmu.File.FileType ;
                        strcpy(lfm->Map[i].FileName,rfmu.File.FileName) ;
                        result = 1 ;  /* Good request - include success code with reply */
                      } else
                        result = 0 ;  /* Invalid request - include failure code with reply */
                    } else
                      result = 0 ;    /* Invalid request - include failure code with reply */
                    break ;
                  }
                }
                if (i == lfm->Count)
                  result = 0 ;        /* Can't find fileref - include failure code with reply */
		smh.MsgType = V4CS_Msg_ModFileMap ;	/* Send reply back to client */
		smh.Flags = result ;
		smh.Bytes = sizeof(smh) ;
		if (send(ConnectSocket,&smh,smh.Bytes,0) < sizeof smh) /* Send response back to client */
		 { v4s_Log("Error (%s) (len = %d) in sending ModFileMap Reply on socket (%d)",v_OSErrString(NETERROR),sizeof smh,ConnectSocket) ;
		   return(FALSE) ;
		 } ;
		if (Trace) v4s_Log("Request to Modify FileMap received - sent status back to client\n") ;
                continue ;
              case V4CS_Msg_ReqAddFileToMap:
		i = v4s_recv(ConnectSocket,(char *)&rfmu + sizeof smh,smh.Bytes - sizeof smh,0) ;	/* Get rest of message */
		result = 1 ;  /* Assume reply will include a success code, until err is found */
		ref = strtol(rfmu.File.FileRef,NULL,10) ;
                for (i=0 ; i < lfm->Count ; i++)
                { if (ref == lfm->Map[i].FileRef)
                  { result = 0 ;  /* Reply with failure code if ref is not unique */
                    break ;
                  }
                }
                if (i == lfm->Count)
                { if (lfm->Count == V4CS_MapFile_Max)
                    result = 0 ;  /* Reply with failure code if FileMap is currently full */
                  else
                  { if ((rfmu.File.FileType != 'V') &&
                        (rfmu.File.FileType != 'R'))
                      result = 0 ; /* Reply with failure code if new FileType is invalid */
                    else
                    { if (strlen(rfmu.File.FileName) > V4CS_MapFileName_Len)
                        result = 0 ; /* Reply with failure code if new FileName is too long */
                      else
                      { lfm->Map[lfm->Count].FileRef = ref ;
                        lfm->Map[lfm->Count].FileType = rfmu.File.FileType ;
                        strcpy(lfm->Map[lfm->Count].FileName,rfmu.File.FileName) ;
                        lfm->Count++ ;
                      }
                    }
                  }
                }
		smh.MsgType = V4CS_Msg_AddFileToMap ;		/* Send reply back to client */
		smh.Flags = result ;
		smh.Bytes = sizeof(smh) ;
		if (send(ConnectSocket,&smh,smh.Bytes,0) < sizeof smh) /* Send response back to client */
		 { v4s_Log("Error (%s) (len = %d) in sending AddToFileMap Reply on socket (%d)",v_OSErrString(NETERROR),sizeof smh,ConnectSocket) ;
		   return(FALSE) ;
		 } ;
		if (Trace) v4s_Log("Request to Add file to FileMap received - sent status back to client\n") ;
                continue ;
              case V4CS_Msg_ReqDelFileFromMap:
		i = v4s_recv(ConnectSocket,(char *)&rfmd + sizeof smh,smh.Bytes - sizeof smh,0) ;	/* Get rest of message */
		result = 1 ;  /* Assume reply will include a success code, until err is found */
                count = lfm->Count ;
                for (i=0 ; i < count ; i++)
                { if (rfmd.FileRef == lfm->Map[i].FileRef)
                  { lfm->Map[i] = lfm->Map[lfm->Count-1] ;
                    lfm->Count-- ;
                    break ;
                  }
                }
                if (i == count)
                  result = 0 ;
		smh.MsgType = V4CS_Msg_DelFileFromMap ;		/* Send reply back to client */
		smh.Flags = result ;
		smh.Bytes = sizeof(smh) ;
		if (send(ConnectSocket,&smh,smh.Bytes,0) < sizeof smh) /* Send response back to client */
		 { v4s_Log("Error (%s) (len = %d) in sending DelFileMap Reply on socket (%d)",v_OSErrString(NETERROR),sizeof smh,ConnectSocket) ;
		   return(FALSE) ;
		 } ;
		if (Trace) v4s_Log("Request to Delete from FileMap received - sent status back to client\n") ;
                continue ;
	      case V4CS_Msg_OpenFileRef:
		i = v4s_recv(ConnectSocket,(char *)&ofr + sizeof smh,smh.Bytes - sizeof smh,0) ;	/* Get rest of message */
		if (i <= 0)
		 { v4s_Log("Error receiving remainder of OpenFileRef message") ; return(FALSE) ; } ;
		if (lal->count >= V4SERVER_Area_Max)
		 v4_error(0,0,"V4SERVER","OPENFILEREF","MAXAREA","Exceeded max number of areas") ;
		cpcb = (struct V4IS__ParControlBlk *)v4mm_AllocChunk(sizeof *cpcb,TRUE) ;
		lal->Area[lal->count].pcb = cpcb ;		/* Save new area control block */
		lal->Area[lal->count].FileRef = ofr.FileRef ; lal->Area[lal->count].Socket = ConnectSocket ;
		for (i=0 ; i < lfm->Count ; i++)
                { if (lfm->Map[i].FileRef == ofr.FileRef)
                    break ;
                } ;
                if (i >= lfm->Count)
		   v4_error(0,0,"V4SERVER","OPENFILEREF","UNVRES","Name resolution for FileRef %d Failed",ofr.FileRef) ;
                UCstrcpyAtoU(cpcb->UCFileName,lfm->Map[i].FileName) ;
		cpcb->OpenMode = ofr.OpenMode ; cpcb->AccessMode = ofr.AccessMode ;
		if (cpcb->OpenMode == 0) cpcb->OpenMode = V4IS_PCB_OM_Read ;
		if (cpcb->AccessMode == 0) cpcb->AccessMode = V4IS_PCB_AM_Get ;
		cpcb->LockMode = ofr.ShareMode ;		/* Copy shared-noshared access flags */
		if (Trace) v4s_Log("Attempting to open file (%s) mode (O=%d L=%d A=%x)\n",
					UCretASC(cpcb->UCFileName),cpcb->OpenMode,cpcb->LockMode,cpcb->AccessMode) ;
                ofr.UserName[sizeof(ofr.UserName)-1] = 0 ;
                ofr.NodeName[sizeof(ofr.NodeName)-1] = 0 ;
                strcpy(lal->Area[lal->count].UserName, ofr.UserName) ;
                strcpy(lal->Area[lal->count].NodeName, ofr.NodeName) ;
#ifdef VMSOS
		v4is_RMSOpen(cpcb) ;
#else
		v4is_Open(cpcb,NULL,NULL) ;				/* Call V4IS to open file */
#endif
		fra.mh.MsgType = V4CS_Msg_FileRefAID ;		/* Send OK back to client with AID */
		fra.mh.Flags = 0 ;
		fra.mh.Bytes = sizeof fra ;
		fra.AreaId = cpcb->AreaId ;
		if (send(ConnectSocket,&fra,sizeof fra,0) < sizeof fra) /* Send response back to client */
		 { v4s_Log("Error (%s) (len = %d) in sending OPEN ack on socket (%d)",v_OSErrString(NETERROR),sizeof fra,ConnectSocket) ;
		   return(FALSE) ;
		 } ;
		lal->count ++ ;
		if (Trace) v4s_Log("File open- sent Aid (%d) back to client\n",fra.AreaId) ;
		continue ;
	      case V4CS_Msg_CloseAID:
		i = v4s_recv(ConnectSocket,(char *)&ca + sizeof smh,smh.Bytes - sizeof smh,0) ;	/* Get rest of message */
		if (i <= 0)
		 { v4s_Log("Error receiving remainder of CloseAID message") ; return(FALSE) ; } ;
		for(i=0;i<lal->count;i++) { if (lal->Area[i].pcb->AreaId == ca.AreaId) break ; } ;
		if (i >= lal->count) v4_error(V4E_UNKAID,0,"V4SERVER","CLOSEAID","UNKAID","Specified AId (%d) not open",ca.AreaId) ;
		if (Trace) v4s_Log("Closing AId (%d)\n",ca.AreaId) ;
#ifdef VMSOS
		v4is_RMSClose(lal->Area[i].pcb) ;
#else
		v4is_Close(lal->Area[i].pcb) ;
#endif
		v4mm_FreeChunk(lal->Area[i].pcb) ;
		lal->Area[i] = lal->Area[lal->count-1] ; lal->count-- ;
		continue ;
	      case V4CS_Msg_GetKey:
		i = v4s_recv(ConnectSocket,(char *)&gkey + sizeof smh,smh.Bytes - sizeof smh,0) ;	/* Get rest of message */
		if (i <= 0)
		 { v4s_Log("Error receiving remainder of GetKey message") ; return(FALSE) ; } ;
		for(i=0;i<lal->count;i++) { if (lal->Area[i].pcb->AreaId == gkey.AreaId) break ; } ;
		if (i >= lal->count) v4_error(V4E_UNKAID,0,"V4SERVER","CLOSEAID","UNKAID","Specified AId (%d) not open",gkey.AreaId) ;
		cpcb = lal->Area[i].pcb ;
		cpcb->KeyPtr = &gkey.key ; cpcb->DfltFileRef = 0 ;
		cpcb->GetBufPtr = (char *)&dr ; cpcb->GetBufLen = sizeof dr ;
		switch (gkey.GetMode)
		 { default:			/* Assume EQ */
		   case V4CS_GetMode_EQ:	cpcb->GetMode = V4IS_PCB_GP_Keyed + V4IS_PCB_CSDataRec ; break ;
		   case V4CS_GetMode_GE:	cpcb->GetMode = V4IS_PCB_GP_KeyedNext + V4IS_PCB_CSDataRec ; break ;
		 } ;
		if (gkey.LockFlag != TRUE) cpcb->GetMode |= (V4IS_PCB_NoLock | V4IS_PCB_IgnoreLock) ;
		if (Trace) v4s_Log("Attempting keyed (lock=%d) get on Aid (%d)\n",gkey.LockFlag,cpcb->AreaId) ;
#ifdef VMSOS
		switch (gkey.key.KeyType)
		 { default:	cpcb->KeyNum = 1 ; break ;
		   case V4IS_KeyType_FrgnKey1:	cpcb->KeyNum = 1 ; break ;
		   case V4IS_KeyType_FrgnKey2:	cpcb->KeyNum = 2 ; break ;
		   case V4IS_KeyType_FrgnKey3:	cpcb->KeyNum = 3 ; break ;
		   case V4IS_KeyType_FrgnKey4:	cpcb->KeyNum = 4 ; break ;
		   case V4IS_KeyType_FrgnKey5:	cpcb->KeyNum = 5 ; break ;
		   case V4IS_KeyType_FrgnKey6:	cpcb->KeyNum = 6 ; break ;
		   case V4IS_KeyType_FrgnKey7:	cpcb->KeyNum = 7 ; break ;
		 } ;
/*		cpcb->KeyLen = gkey.key.Bytes - sizeof (union V4IS__KeyPrefix) ; */
		cpcb->KeyLen = gkey.ActualBytes ;
		cpcb->KeyPtr = &gkey.key.KeyVal ;
		v4is_RMSGet(cpcb) ;
#else
		v4is_Get(cpcb) ;
#endif
		dr.mh.MsgType = V4CS_Msg_DataRec ;		/* Send data record back to client */
		dr.mh.Flags = 0 ;
		dr.mh.Bytes = (char *)&dr.Buffer[dr.DataLen] - (char *)&dr ;
		if (Trace) v4s_Log("Sending data (%d bytes) back to client\n",dr.mh.Bytes) ;
		dr.AreaId = cpcb->AreaId ; dr.DataId = cpcb->DataId ;
#ifdef VMSOS
		dr.DataId2 = cpcb->DataId2 ;
#endif
		if (send(ConnectSocket,&dr,dr.mh.Bytes,0) < dr.mh.Bytes) /* Send response back to client */
		 { v4s_Log("Error (%s) (len = %d) in sending GET results  on socket (%d)",v_OSErrString(NETERROR),dr.mh.Bytes,ConnectSocket) ;
		   return(FALSE) ;
		 } ;
		continue ;
	      case V4CS_Msg_GetNext:
		i = v4s_recv(ConnectSocket,(char *)&gnext + sizeof smh,smh.Bytes - sizeof smh,0) ;	/* Get rest of message */
		if (i <= 0)
		 { v4s_Log("Error receiving remainder of GetNext message") ; return(FALSE) ; } ;
		for(i=0;i<lal->count;i++) { if (lal->Area[i].pcb->AreaId == gnext.AreaId) break ; } ;
		if (i >= lal->count) v4_error(V4E_UNKAID,0,"V4SERVER","CLOSEAID","UNKAID","Specified AId (%d) not open",gnext.AreaId) ;
		cpcb = lal->Area[i].pcb ;
		cpcb->GetBufPtr = (char *)&dr ; cpcb->GetBufLen = sizeof dr ;
		switch (gnext.GetMode)
		 { default: v4_error(V4E_UNKGETMODE,0,"V4SERVER","GetNext","UNKGETMODE","Area (%s) - Invalid Get mode (%d)",
					UCretASC(cpcb->UCFileName),gnext.GetMode) ;
		   case V4CS_GetMode_BOF:	cpcb->GetMode = V4IS_PCB_GP_BOF + V4IS_PCB_CSDataRec ; break ;
		   case V4CS_GetMode_Next:	cpcb->GetMode = V4IS_PCB_GP_Next + V4IS_PCB_CSDataRec ; break ;
		   case V4CS_GetMode_DataOnly:	cpcb->GetMode = V4IS_PCB_GP_DataOnly + V4IS_PCB_CSDataRec ; break ;
		 } ;
		if (!gnext.LockFlag) cpcb->GetMode |= (V4IS_PCB_NoLock | V4IS_PCB_IgnoreLock) ;
		if (Trace) v4s_Log("Attempting next get on Aid (%d)\n",cpcb->AreaId) ;
		if (smh.Flags & V4CS_Flag_NoCompress) cpcb->GetMode |= V4IS_PCB_CSExpDataRec ;
#ifdef VMSOS
		v4is_RMSGet(cpcb) ;
#else
		v4is_Get(cpcb) ;
#endif
		dr.mh.MsgType = V4CS_Msg_DataRec ;		/* Send data record back to client */
		dr.mh.Flags = 0 ;
		dr.mh.Bytes = (char *)&dr.Buffer[dr.DataLen] - (char *)&dr ;
		if (Trace) v4s_Log("Sending data (%d bytes) back to client\n",dr.mh.Bytes) ;
		dr.AreaId = cpcb->AreaId ; dr.DataId = cpcb->DataId ;
#ifdef VMSOS
		dr.DataId2 = cpcb->DataId2 ;
#endif
		if (send(ConnectSocket,&dr,dr.mh.Bytes,0) < dr.mh.Bytes) /* Send response back to client */
		 { v4s_Log("Error (%s) (len = %d) in sending GETNext results on socket (%d)",v_OSErrString(NETERROR),dr.mh.Bytes,ConnectSocket) ;
		   return(FALSE) ;
		 } ;
		continue ;
	      case V4CS_Msg_GetDataId:
		i = v4s_recv(ConnectSocket,(char *)&gdid + sizeof smh,smh.Bytes - sizeof smh,0) ;	/* Get rest of message */
		if (i <= 0)
		 { v4s_Log("Error receiving remainder of GetDataId message") ; return(FALSE) ; } ;
		for(i=0;i<lal->count;i++) { if (lal->Area[i].pcb->AreaId == gdid.AreaId) break ; } ;
		if (i >= lal->count) v4_error(V4E_UNKAID,0,"V4SERVER","CLOSEAID","UNKAID","Specified AId (%d) not open",gdid.AreaId) ;
		cpcb = lal->Area[i].pcb ;
		cpcb->DataId = gdid.DataId ; cpcb->DfltFileRef = 0 ;
#ifdef VMSOS
		cpcb->DataId2 = gdid.DataId2 ;
#endif
		cpcb->GetBufPtr = (char *)&dr ; cpcb->GetBufLen = sizeof dr ;
		cpcb->GetMode = V4IS_PCB_GP_DataId + V4IS_PCB_CSDataRec ;
		if (!gnext.LockFlag) cpcb->GetMode |= (V4IS_PCB_NoLock | V4IS_PCB_IgnoreLock) ;
		if (Trace) v4s_Log("Attempting DataId get on Aid (%d)\n",cpcb->AreaId) ;
#ifdef VMSOS
		v4is_RMSGet(cpcb) ;
#else
		v4is_Get(cpcb) ;
#endif
		dr.mh.MsgType = V4CS_Msg_DataRec ;		/* Send data record back to client */
		dr.mh.Flags = 0 ;
		dr.mh.Bytes = (char *)&dr.Buffer[dr.DataLen] - (char *)&dr ;
		if (Trace) v4s_Log("Sending data (%d bytes) back to client\n",dr.mh.Bytes) ;
		dr.AreaId = cpcb->AreaId ; dr.DataId = cpcb->DataId ;
#ifdef VMSOS
		dr.DataId2 = cpcb->DataId2 ;
#endif
		if (send(ConnectSocket,&dr,dr.mh.Bytes,0) < dr.mh.Bytes) /* Send response back to client */
		 { v4s_Log("Error (%s) (len = %d) in sending GET DataID results on socket (%d)",v_OSErrString(NETERROR),dr.mh.Bytes,ConnectSocket) ;
		   return(FALSE) ;
		 } ;
		continue ;
	      case V4CS_Msg_Put:
		blen= smh.Bytes - sizeof smh ;		/* Number of bytes to grab */
		bp = (char *)&mput + sizeof smh ;
		for(i=0;i<blen;)
		 { if ((ok = v4s_recv(ConnectSocket,bp,blen-i,0)) <= 0)
		    v4_error(V4E_RECVERR,0,"V4IS","GET","RECVERR","Error (%s) in recv on socket (%d)",v_OSErrString(NETERROR),ConnectSocket) ;
		   i += ok ; bp += ok ;			/* May not receive entire buffer in one recv packet */
		 } ;
		for(i=0;i<lal->count;i++) { if (lal->Area[i].pcb->AreaId == mput.AreaId) break ; } ;
		if (i >= lal->count) v4_error(V4E_UNKAID,0,"V4SERVER","CLOSEAID","UNKAID","Specified AId (%d) not open",mput.AreaId) ;
		cpcb = lal->Area[i].pcb ;
		cpcb->DfltFileRef = lal->Area[i].FileRef ;
		switch (mput.CmpMode)
		 { default: v4_error(V4E_INVCOMPMODE,0,"V4IS","CopyData","INVCOMPMODE","Invalid compression mode (%d)",mput.CmpMode) ;
		   case V4IS_DataCmp_Mth1:
			if (expand == NULL) expand = (int *)v4mm_AllocChunk(V4IS_BktSize_Max,FALSE) ;
			cpcb->PutBufLen = data_expand(expand,(char *)&mput.DataBuffer,mput.PutBytes) ;
			cpcb->PutBufPtr = expand ;
			break ;
		   case V4IS_DataCmp_Mth2:
			if (expand == NULL) expand = (int *)v4mm_AllocChunk(V4IS_BktSize_Max*2,FALSE) ;
			lzrw1_decompress((char *)&mput.DataBuffer,mput.PutBytes,expand,&cpcb->PutBufLen,V4IS_BktSize_Max*2) ;
			cpcb->PutBufPtr = expand ;
			break ;
		   case V4IS_DataCmp_None:
			cpcb->PutBufPtr = (char *)&mput.DataBuffer ; cpcb->PutBufLen = mput.PutBytes ;
			break ;
		 }
		switch (cpcb->PutMode)
		 { default:
		   case V4CS_PutMode_Write:	cpcb->PutMode = V4IS_PCB_GP_Write ; break ;
		   case V4CS_PutMode_Update:	cpcb->PutMode = V4IS_PCB_GP_Update ; break ;
		   case V4CS_PutMode_Insert:	cpcb->PutMode = V4IS_PCB_GP_Insert ; break ;
		   case V4CS_PutMode_Delete:	cpcb->PutMode = V4IS_PCB_GP_Delete ; break ;
		   case V4CS_PutMode_DataOnly:	cpcb->PutMode = V4IS_PCB_GP_DataOnly ; break ;
		   case V4CS_PutMode_Unlock:	cpcb->PutMode = V4IS_PCB_GP_Unlock ; break ;
		   case V4CS_PutMode_Cache:	cpcb->PutMode = V4IS_PCB_GP_Cache ; break ;
		   case V4CS_PutMode_Reset:	cpcb->PutMode = V4IS_PCB_GP_Reset ; break ;
		   case V4CS_PutMode_Obsolete:	cpcb->PutMode = V4IS_PCB_GP_Obsolete ; break ;
		 } ;
		if (Trace) v4s_Log("Attempting put (%d bytes) on Aid (%d)\n",cpcb->PutBufLen,cpcb->AreaId) ;
#ifdef VMSOS
		v4is_RMSPut(cpcb) ;
#else
		v4is_Put(cpcb,NULL) ;
#endif
		mao.mh.MsgType = V4CS_Msg_PutOK ; mao.mh.Flags = 0 ; mao.mh.Bytes = sizeof mao ;
		mao.AreaId = mput.AreaId ; mao.PutMode = mput.PutMode ;
		send(ConnectSocket,&mao,mao.mh.Bytes,0) ;
		continue ;
	    } ;
	 } ;

}

/*	v4s_Log - Log a message					*/

#ifdef ALTVARGS						/* Need funky header for SCO variable-arg handler */
v4s_Log(va_alist)
  va_dcl
{
  va_list args ; char *format ;
  time_t timer ;
  char ebuf[250] ; int code ;

/*	Do special formatting on error message */
	va_start(args) ;
	format = va_arg(args,char *) ; vsprintf(ebuf,format,args) ; va_end(args) ;
#else							/* All other UNIX */
void v4s_Log(format)
  char *format ;
{
  va_list ap ;
  time_t timer ;
  char ebuf[250] ;

/*	Do special formatting on error message */
	va_start(ap,format) ; vsprintf(ebuf,format,ap) ; va_end(ap) ;
#endif
	time(&timer) ;
	printf("%.19s %s",ctime(&timer),ebuf) ;
}


/*	v4s_recv - Reads bytes from socket - detects hung socket & returns error	*/
/*	Call: bytes = v4s_recv(socket , buf , bytes , first )
	  where bytes is number of bytes read (<=0 for error),
		socket is socket number,
		bytes is number we want to read,
		first is TRUE if first call (i.e. block)				*/

int v4s_recv(socket,buf,bytes,first)
 int socket,bytes,first ;
 char *buf ;
{ int avail,toread,need,tries,i ;

	need = bytes ;
	for(tries=0;tries<100;tries++)
	 { if (first)
	    { toread = bytes ;
	    } else
	    { if (NETIOCTL(socket,FIONREAD,&avail) != 0)
	       { v4s_Log("NETIOCTL() error (%s) in v4s_recv()",v_OSErrString(NETERROR)) ; return(0) ; } ;
	      if (avail <= 0)				/* Nothing to read yet */
	       { HANGLOOSE(25) ;			/* Wait 25 milliseconds */
	         continue ;
	       } ;
	      toread = (avail > need ? need : avail) ;
	    } ;
	   i = recv(socket,buf,toread,0) ;
	   if (i <= 0)
	    { v4s_Log("Error (%s) in recv, return value (%d)",v_OSErrString(NETERROR),i) ; return(0) ; } ;
	   need -= i ;
	   if (need <= 0) return(bytes) ;		/* All done */
	   buf += i ;					/* Update pointer & keep plugging */
	   tries = 0 ;					/* Reset tries */
	 } ;
	v4s_Log("Timeout waiting for data in v4s_recv() - returning error") ; return(0) ;
}