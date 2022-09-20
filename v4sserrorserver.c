/*	V4SSErrorServer - WWW Server for V4/Excel Errors	*/

#include <signal.h>
#define NEED_SOCKET_LIBS 1
#define UNUSED -1
#include "vconfig.c"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
//#include <io.h>
#include <fcntl.h>

#ifdef UNIX
#include <termio.h>
#include <sys/time.h>
#include <sys/socket.h>
#ifdef HPUX
#include <sys/fcntl.h>
#else
#include <sys/mode.h>
#endif
#endif
#ifdef WINNT
#include <windows.h>
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EINPROGRESS WSAEINPROGRESS
#endif

#define SERVER_PORT 3000

int v4fe_SecurityValidate() ;
char *v_OSErrString() ;
void Log() ;
int v_SpawnProcess() ;

void v4SrvSubProcess() ;
struct v4fe__SubProcessArguments {		/* All arguments passed to thread via single argument block */
  unsigned int ConnectSocket ;
  int jobno1,jobno2 ;
  FILE *ufp ;					/* File to be unpacked */
 } *v4fearg ;


#define DFLT_HEARTBEAT_WAIT 300	/* Seconds to wait for idle heart beat status */

char inikeylist[][16] =
 { "HEARTBEAT", "PORT", ""
 } ;
main(argc,argv,envp)
  int argc ;			/* Number of arguments passed via DCL */
  char *argv[] ;		/* Argument values */
  char **envp ;			/* Environment pointer */
{ fd_set sset ;
  struct timeval tev ;
   struct sockaddr_in sin,fsin ;
   unsigned int PrimarySocket,ConnectSocket ;
   FILE *fp ;
   int oldjobno1 ;
   unsigned short port ;
#define ARGMax 15
   struct v4fe__SubProcessArguments v4feargs[ARGMax],*ap ; int apcnt=0 ;
   struct v4fe__SubProcessArguments v4fearg ;
#define REQMax 50		/* Track last 50 requests for duplicate peer names */
#define REQMinSeconds 30	/* Must have at least 30 seconds between requests */
   struct v4fe__PeerList {
     int Index ;
     struct sockaddr_in csin ;
     struct {
       struct sockaddr_in sin ;
       time_t time ;
     } Request[REQMax] ;
    } pl ;
#ifdef WINNT
  SYSTEMTIME st ;
  int didWSAStartup = FALSE ;
  WSADATA wsad ;
  DWORD dwThreadId ;
#endif
  int i,heartbeat,ax ; char *arg ;


/*	Set up some defaults */
	memset(&v4fearg,0,sizeof v4fearg) ; memset(&pl,0,sizeof pl) ;
	for(i=0;i<ARGMax;i++) { v4feargs[i].ConnectSocket = UNUSED ; } ;
	port = SERVER_PORT ; heartbeat = DFLT_HEARTBEAT_WAIT ;

	for(ax=1;ax<argc;ax++)
	 { arg = argv[ax] ;
	   if (*arg != '-') { printf("? Expecting -switch, not: %s\n",arg) ; exit(EXITABORT) ; } ;
	   switch (arg[1])
	    { default:	printf("? Invalid switch value\n") ; exit(EXITABORT) ;
	         case 'b':	/* Heartbeat n */
		   if (ax == argc) { printf("? -b expecting argument to follow\n") ; exit(EXITABORT) ; } ;
		   heartbeat = atoi(argv[++ax]) ; break ;
		 case 'h':	/* Help */
		   printf("xv4ssErrorServer Switches\n\
  -b seconds	Number of seconds between log hearbeats (%d)\n\
  -h		This help message\n\
  -p port	TCP/IP port to listen on (%d)\n\
  -u file	Unpack file sent via email\n",DFLT_HEARTBEAT_WAIT,SERVER_PORT) ;
		   exit(EXITOK) ;
		   break ;
		 case 'p':	/* Port n */
		   if (ax == argc) { printf("? -u expecting argument to follow\n") ; exit(EXITABORT) ; } ;
		   port = atoi(argv[++ax]) ; break ;
		 case 'u':	/* Unpack file */
		   if (ax == argc) { printf("? -u expecting argument to follow\n") ; exit(EXITABORT) ; } ;
		   fp = fopen(argv[++ax],"r") ;
		   if (fp == NULL)
		    { printf("? Error (%s) accessing error file (%s)\n",v_OSErrString(errno),argv[ax]) ; exit(EXITABORT) ; } ;
		   memset(&v4fearg,0,sizeof v4fearg) ; v4fearg.ufp = fp ;
#ifdef WINNT
		   GetLocalTime(&st) ;
		   v4fearg.jobno1 = st.wYear * 10000 + st.wMonth * 100 + st.wDay ;
		   v4fearg.jobno2 = st.wHour * 100 + st.wMinute ;
#endif
		   v4SrvSubProcess(&v4fearg) ; fclose(fp) ;
		   exit(EXITOK) ;
	    } ;
	 } ;

	Log("Startup: Hearbeat %d, Port %d",heartbeat,port) ;

#ifdef WINNT
	if (!didWSAStartup)
	 { didWSAStartup = TRUE ;
//	   i = MAKEWORD(1,1) ;		/* Always accept highest version */
	   WSAStartup(MAKEWORD(1,1),&wsad) ;
	 } ;
#endif
	if ((PrimarySocket = socket(AF_INET, SOCK_STREAM,0)) < 0)
	 { Log("? Error (%s) creating INET Socket",v_OSErrString(errno)) ; exit(EXITABORT) ;
	 } ;
	i = 1 ;
	if (NETIOCTL(PrimarySocket,FIONBIO,&i) != 0)
	 { Log("? Could not set socket to NOBLOCK") ; exit(EXITABORT) ;
	 } ;
	memset(&sin,0,sizeof sin) ; sin.sin_family = AF_INET ; sin.sin_port = htons(port) ;
	sin.sin_addr.s_addr = htonl(INADDR_ANY) ;
	if (bind(PrimarySocket,&sin,sizeof sin) < 0)
	 { Log("? Error (%s) bind'ing INET Socket",v_OSErrString(NETERROR)) ; exit(EXITABORT) ; } ;
	if (listen(PrimarySocket,10) < 0)
	 { Log("? Error (%s) listen'ing on INET Socket",v_OSErrString(NETERROR)) ; exit(EXITABORT) ; } ;
	oldjobno1 = UNUSED ;
	for(;;v4fearg.jobno2++)
	 { 
wait_on_accept:
	   Log("Waiting on accept") ;
	   for(;;)
	    {
	      FD_ZERO(&sset) ; FD_SET(PrimarySocket,&sset) ;
	      tev.tv_sec = heartbeat ; tev.tv_usec = 0 ;
	      i = select(1,&sset,NULL,NULL,&tev) ;	/* Wait for something! */
	      if (i > 0) break ;
	      Log("idle heartbeat") ;
	    } ;
	   i = sizeof fsin ;
	   if ((ConnectSocket = accept(PrimarySocket,&fsin,&i)) < 0)
	    { Log("?Error (%s) accept'ing on INET Socket",v_OSErrString(NETERROR)) ; exit(EXITABORT) ; } ;
/*	   See if already received request from peer - if so & if in recent past then trash this */
	   if (getpeername(ConnectSocket,&pl.csin,&i) == 0)
	    { int x ;
	      for(x=0;x<REQMax;x++)
	       { if (memcmp(&pl.csin.sin_addr,&pl.Request[x].sin.sin_addr,sizeof pl.csin.sin_addr) != 0) continue ;
	         if (time(NULL) - pl.Request[x].time <= REQMinSeconds)
		  { Log("? Denying request from %s - coming too frequently",inet_ntoa(pl.csin.sin_addr)) ;
		    pl.Request[x].time = time(NULL) ; SOCKETCLOSE(ConnectSocket) ;
		    goto wait_on_accept ;
		  } ;
		 pl.Request[x].time = time(NULL) ; break ;
	       } ;
	      if (x >= REQMax)
	       { x = (++pl.Index) % REQMax ;
	         pl.Request[x].sin = pl.csin ; pl.Request[x].time = time(NULL) ;
	       } ;
	    } ;
#ifdef WINNT
	   GetLocalTime(&st) ;				/* Check for new day */
	   v4fearg.jobno1 = st.wYear * 10000 + st.wMonth * 100 + st.wDay ;
#endif
	   if (oldjobno1 != v4fearg.jobno1)
	    { char cmdfile[V_FileName_Max] ; FILE *fp ;
	      oldjobno1 = v4fearg.jobno1 ;
	      for(v4fearg.jobno2=1;;v4fearg.jobno2++)
	       { sprintf(cmdfile,"v4ss%d-%d.err",v4fearg.jobno1,v4fearg.jobno2) ;
	         fp = fopen(cmdfile,"r") ; if (fp == NULL) break ;
		 fclose(fp) ; continue ;
	       } ;
	      if (v4fearg.jobno2 > 1)
	       Log("Restart detected, continuing with sequence %d-%d",v4fearg.jobno1,v4fearg.jobno2) ;
	    } ;
//	   Log("Spawning subprocess on connect socket (%d)",ConnectSocket) ;
	   v4fearg.ConnectSocket = ConnectSocket ;
/*	   Copy arguments into "new" spot to pass to thread */
	   for(;;)
	    { apcnt++ ; ap = &v4feargs[apcnt % ARGMax] ; if (ap->ConnectSocket == UNUSED) break ;
	    } ;
	   *ap = v4fearg ;
#ifdef WINNT
	   CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)v4SrvSubProcess,ap,0,&dwThreadId) ;
	   Log("Created thread arg(%x) socket(%d) peer(%s) job(%d-%d)",
		ap,ConnectSocket,inet_ntoa(pl.csin.sin_addr),v4fearg.jobno1,v4fearg.jobno2) ;
#endif
	 } ;

}


void v4SrvSubProcess(v4fearg)
  struct v4fe__SubProcessArguments *v4fearg ;
{ FILE *fp=NULL ;
  fd_set sset ;				/* Socket set */
  struct timeval tev ;
  char LenBuf[10] ; int TotalLen, UsedLen ; char *MsgBuf ;
  char filename[128] ; int bytes,len ; char *ptr,*p ;
  int res ; int file ;

	MsgBuf = NULL ;
	Log("Begin of Message on job(%d-%d)",v4fearg->jobno1,v4fearg->jobno2) ;
/*	First read in total length and then decide what to do */
	for(ptr=LenBuf,bytes=0;bytes<sizeof LenBuf;bytes++,ptr++)
	 { 
	   if (v4fearg->ConnectSocket != 0)
	    { FD_ZERO(&sset) ; FD_SET(v4fearg->ConnectSocket,&sset) ;
	      tev.tv_sec = (ptr == LenBuf ? 60 : 5) ; tev.tv_usec = 0 ;	/* Wait a little longer for first byte */
	      res = select(FD_SETSIZE,&sset,NULL,NULL,&tev) ;	/* Wait for something to happen */
	      if (res <= 0)
	       { Log("? Timeout waiting for input on job(%d-%d)",v4fearg->jobno1,v4fearg->jobno2) ; goto thread_end ; } ;
	    } ;
	   if (v4fearg->ufp != NULL)
	    { *ptr = fgetc(v4fearg->ufp) ;
	    } else
	    { res = recv(v4fearg->ConnectSocket,ptr,1,0) ;
	      if (res <= 0)
	       { if (res == 0)
	          { Log("? Remote connection has shut down gracefully on job(%d-%d)",v4fearg->jobno1,v4fearg->jobno2) ;
	          } else { Log("? Error (%s) in recv on job(%d-%d)",v_OSErrString(NETERROR),v4fearg->jobno1,v4fearg->jobno2) ; } ;
	         SOCKETCLOSE(v4fearg->ConnectSocket) ;
	       } ;
	    } ;
	   if (*ptr < ' ') break ;			/* Anything less than ' ' means end of line */
	 } ;
	if (bytes >= sizeof LenBuf)
	 { Log("? Input message exceeds %d bytes",sizeof LenBuf) ; goto thread_end ; } ;
	ptr++ ; *ptr = '\0' ;					/* Terminate with null */
	TotalLen = strtol(LenBuf,&ptr,10) ; if (*ptr >= ' ') TotalLen = -1 ;
	if (TotalLen < 100 || TotalLen > 5000000)	/* Set limits for what to suck up */
	 { Log("? Invalid Header (%s) on job(%d-%d",LenBuf,v4fearg->jobno1,v4fearg->jobno2) ;
	   goto thread_end ;
	 } ;

	MsgBuf = malloc(TotalLen+100) ;
	for(ptr=MsgBuf,bytes=0;bytes<TotalLen;bytes++,ptr++)
	 { 
	   if (v4fearg->ConnectSocket != 0)
	    { FD_ZERO(&sset) ; FD_SET(v4fearg->ConnectSocket,&sset) ;
	      tev.tv_sec = 5 ; tev.tv_usec = 0 ;
	      res = select(FD_SETSIZE,&sset,NULL,NULL,&tev) ;	/* Wait for something to happen */
	      if (res <= 0)
	       { Log("? Timeout waiting for input on job(%d-%d)",v4fearg->jobno1,v4fearg->jobno2) ; goto thread_end ; } ;
	    } ;
	   if (v4fearg->ufp != NULL)
	    { *ptr = fgetc(v4fearg->ufp) ;
	    } else
	    { res = recv(v4fearg->ConnectSocket,ptr,1,0) ;
	      if (res <= 0)
	       { if (res == 0)
	          { Log("? Remote connection has shut down gracefully on job(%d-%d)",v4fearg->jobno1,v4fearg->jobno2) ;
	          } else { Log("? Error (%s) in recv on job(%d-%d)",v_OSErrString(NETERROR),v4fearg->jobno1,v4fearg->jobno2) ; } ;
	         SOCKETCLOSE(v4fearg->ConnectSocket) ;
	       } ;
	    } ;
	 } ;
/*	Got entire message in MsgBuf - now split into different pieces based on segment lengths */
	ptr = MsgBuf ; UsedLen = 0 ;
	sprintf(filename,"v4ss%d-%d.err",v4fearg->jobno1,v4fearg->jobno2) ;
	len = strtol(ptr,&p,10) ; if (*p != '\0') len = -1 ;
	if (len < 50 || len + UsedLen > TotalLen)
	 { Log("? First header length (%d) not valid on job(%d-%d)",len,v4fearg->jobno1,v4fearg->jobno2) ;
	   fclose(fp) ;
	   goto thread_end ;
	 } ;
	fp = fopen(filename,"w") ;
	if (fp == NULL)
	 { Log("? File (#%s) could not open (%s) job(%d-%d)",filename,v_OSErrString(errno),v4fearg->jobno1,v4fearg->jobno2) ;
	   goto thread_end ;
	 } ;
	UsedLen += (strlen(ptr) + 1) ;			/* Add in length of header */
	ptr = p + 1 ;					/* Move pointer to data */
	fwrite(ptr,len,1,fp) ;				/* Write it out */
	fclose(fp) ;
	ptr += len ; UsedLen += len ;			/* Advance to next section */
/*	Loop for each remaining file embedded in this message */
	for(file=1;UsedLen<TotalLen;file++,ptr+=len,UsedLen+=len)
	 { int isAscii ;
	   len = strtol(ptr,&p,10) ; if (*p != '\0') len = 0xfffffff ;
	   if (len < 0) { len = -len ; isAscii = TRUE ; }
	    else { isAscii = FALSE ; }
	   if (len + UsedLen > TotalLen)
	    { Log("? File (#%d) header length (%d) not valid on job(%d-%d)",file,len,v4fearg->jobno1,v4fearg->jobno2) ;
	      goto thread_end ;
	    } ;
	   UsedLen += (strlen(ptr) + 1) ;
	   ptr = p + 1 ;		/* Move pointer to data */
	   sprintf(filename,"v4ss%d-%d.%d",v4fearg->jobno1,v4fearg->jobno2,file) ;
	   fp = fopen(filename,(isAscii ? "w" : "wb")) ;
	   if (fp == NULL)
	    { Log("? File (#%s) could not open (%s) job(%d-%d)",filename,v_OSErrString(errno),v4fearg->jobno1,v4fearg->jobno2) ;
	      goto thread_end ;
	    } ;
	   if (isAscii)			/* If len < 0 then ASCII file, otherwise binary */
	    { int used,l ;
	      for(used=0,p=ptr;used<len;)
	       { l = strlen(p) ;	/* Get length of next line */
	         if (l + used > len)
		  { Log("? File (#%s) has invalid format, job(%d-%d)",filename,v4fearg->jobno1,v4fearg->jobno2) ;
		    goto thread_end ;
		  } ;
	         fputs(p,fp) ; fputs("\n",fp) ;
	         p += (l + 1) ; used += (l + 1) ;
	       } ;
	    } else			/* Write out binary */
	    { fwrite(ptr,len,1,fp) ;
	    } ;
	   fclose(fp) ;
	 } ;

	if (v4fearg->ConnectSocket != 0)
	 { Log("Finished event (%d-%d)",v4fearg->jobno1,v4fearg->jobno2) ; } ;
/*	All done - SOCKETCLOSE the socket */
thread_end:
	if (v4fearg->ConnectSocket != 0)
	 { SOCKETCLOSE(v4fearg->ConnectSocket) ; v4fearg->ConnectSocket = UNUSED ; } ;
	if (MsgBuf != NULL) free(MsgBuf) ;
	return ;
}



/*	Log - Logs message to stdout & date dependent log file */

#ifdef WINNT
  CRITICAL_SECTION CS ;
  int initCS = FALSE ;
#endif
void Log(msg)
   char *msg ;
{ SYSTEMTIME st ;
  FILE *fp ;
  va_list ap ;
  char filename[100],timebuf[50],lbuf[512] ;

/*	Do special formatting on error message */
	if (!initCS) { initCS = TRUE ; InitializeCriticalSection(&CS) ; } ;
	va_start(ap,msg) ; vsprintf(lbuf,msg,ap) ; va_end(ap) ;
	GetLocalTime(&st) ;
	sprintf(timebuf,"%02d:%02d:%02d",st.wHour,st.wMinute,st.wSecond) ;
	printf("%s %s\n",timebuf,lbuf) ;
	sprintf(filename,"v4sserrorserver%02d%02d.log",st.wMonth,st.wDay) ;
#ifdef WINNT
	EnterCriticalSection(&CS) ;
#endif
	fp = fopen(filename,"a") ;			/* Open up log file */
	if (fp != NULL)
	 { fprintf(fp,"%s %s\n",timebuf,lbuf) ; fclose(fp) ; } ;
#ifdef WINNT
	LeaveCriticalSection(&CS) ;
#endif
}

/*	v_SpawnProcess - Spawns a subprocess, with/without argument, with/without waiting for return	*/
/*	Call: res = v_SpawnProcess( exefile, argbuf , waitflag , errbuf , sockptr )
	  where res is TRUE/FALSE if OK, not OK,
		exefile is executable to run, if NULL then exe assumed to be part of argbuf,
		argbuf is ptr to string argument, NULL if none,
		waitflag is TRUE to wait for subprocess to complete, FALSE for immediate return,
		errbuf is optional buffer for error message if problem, NULL if none,
		sockptr, if not NULL, is pointer to socket to monitor and if disconnect then terminate process immediately	*/

v_SpawnProcess(exebuf,argbuf,waitflag,errbuf,sockptr)
  char *exebuf ;
  char *argbuf ;
  int waitflag ;
  char *errbuf ;
  int *sockptr
{
#ifdef UNIX
  char *bp1 ;
#endif
#ifdef VMSOS
  char tb[2000] ;
  int res,resx ;
   struct vms__dsc {
     short int length ;		/* Length of value */
     short int desc ;		/* Descriptive info */
     char *pointer ;		/* Pointer to value */
    } strdsc ;
#endif
#ifdef WINNT
   STARTUPINFO winnt_si ;		/* Process startup info */
   PROCESS_INFORMATION winnt_pi ;	/* Get process info from CreateProcess */
#endif

#ifdef WINNT
	memset(&winnt_si,0,sizeof winnt_si) ; winnt_si.cb = sizeof winnt_si ; winnt_si.lpTitle = "V4 Spawn" ;
	GetStartupInfo(&winnt_si) ;
	if (!CreateProcess(NULL,(argbuf == NULL ? "cmd" : argbuf),NULL,NULL,FALSE,0,NULL,NULL,&winnt_si,&winnt_pi))
	 { if (errbuf != NULL) sprintf(errbuf,"Error (%s) in CreateProcess\n",v_OSErrString(GetLastError())) ;
	   return(FALSE) ;
	 } ;
	WaitForSingleObject(winnt_pi.hProcess,INFINITE) ;
	CloseHandle(winnt_pi.hProcess) ; CloseHandle(winnt_pi.hThread) ;
#endif
#ifdef VMSOS
	if (argbuf != NULL)
	 { strdsc.pointer = argbuf ; strdsc.length = strlen(argbuf) ; strdsc.desc = 0 ;
	   res = LIB$SPAWN(&strdsc,0,0,0,0,0,&resx) ;
	 } else { res = LIB$SPAWN() ; } ;
	if ((res & 1) == 0)
	 { if (errbuf != NULL) sprintf(errbuf,"Error (%s) in LIB$SPAWN",mscu_vms_errmsg(res,tb)) ;
	   return(FALSE) ;
	 } ;
#endif
#ifdef UNIX
	bp1 = getenv("SHELL") ;			/* Want to run same shell as parent */
	if (bp1 == NULL) bp1 = "/bin/sh" ;
	strcpy(tb,bp1) ;
	if (fork() == 0)
	 { if (execlp(tb,tb,(argbuf != NULL ? "-c" : NULL),argbuf,0) == -1)
	    { if (errbuf != NULL) sprintf(errbuf,"Error (%s) trying to execlp to shell %s (with command line)",v_OSErrString(errno),tb) ;
	      return(FALSE) ;
	    } ;
	 } else wait(NULL) ;
#endif
	return(TRUE) ;
}

/*	v_OSErrString - Returns formatted error message based on OS error code */

char *v_OSErrString(errno)
{ static char EMsgBuf[280] ;
  char tbuf[256] ; int l ;

#ifdef WINNT
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,NULL,errno,0,tbuf,sizeof tbuf,NULL );
	l = strlen(tbuf) - 1 ; for(;l>=0&&tbuf[l]<=0x20;l--) { tbuf[l] = '\0' ; } ;
	sprintf(EMsgBuf,"%d:[%s]",errno,tbuf) ;
	return(EMsgBuf) ;
#endif
#ifdef VMSOS
	mscu_vms_errmsg(errno,tbuf) ;
	sprintf(EMsgBuf,"%d:[%s]",errno,tbuf) ;
	return(EMsgBuf) ;
#endif
#ifdef UNIX
	strerror(errno,tbuf) ;
	sprintf(EMsgBuf,"%d:[%s]",errno,tbuf) ;
	return(EMsgBuf) ;
#endif
	sprintf(EMsgBuf,"%d",errno) ;
	return(EMsgBuf) ;
}
