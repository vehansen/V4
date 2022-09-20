/*	vxmlserver.c - Server for V4-MIDAS-XML Interface			*/

#include <signal.h>
#include <time.h>
#define NEED_SOCKET_LIBS 1
#include "vconfig.c"
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>

#ifdef UNIX
#include <termio.h>
#include <sys/time.h>
#include <sys/socket.h>
#ifdef HPUX
#include <sys/fcntl.h>
#else
//#include <mode.h>
#endif
#endif
#ifdef WINNT
#include <windows.h>
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EINPROGRESS WSAEINPROGRESS
#endif

#ifdef ALPHAOSF
#include <pthread.h>
/*	To build under Dec Unix & Linux:
	  cc -o xvxmlserver -O3 -w  vxmlserver.c  -lm -lc -lrt

	  cl /O2 /Fexvxmlserver.exe /w vxmlserver.c kernel32.lib wsock32.lib advapi32.lib user32.lib
	  = socket(host::OSINfo(ipaddress?) Port::3011 message::"now is the time\Z")
*/
#endif

#define TRUE 1
#define FALSE 0
#define UNUSED -1


#ifdef ALTVARGS
#include <varargs.h>            /* Funky include for SCO */
#else
#include <stdarg.h>
#endif

void Log(char *, ...) ;
void xml_Process() ;

#define XIMAX 50
struct XTP__Info {
  int ConnectSocket ;
  int jobno1, jobno2 ;
  char xv3Command[128] ;
  char filePrefix[128] ;
  char dirWork[128] ;
 } ;

#define DFLT_HEARTBEAT_WAIT 300	/* Seconds to wait for idle heart beat status */
#define EOM 26

char inikeylist[][16] =
 { "HEARTBEAT", "PORT", "XV3COMMAND", "RESTART", "FILEPREFIX", "DIRECTORY",  ""
 } ;

 #define VXMLServer_Major 1
 #define VXMLServer_Minor 3

main(argc,argv,envp)
  int argc ;			/* Number of arguments passed via DCL */
  char *argv[] ;		/* Argument values */
  char **envp ;			/* Environment pointer */
{ fd_set sset ;
  struct timeval tev ;
   struct hostent *hp ;
   struct sockaddr_in sin,fsin ;
   int PrimarySocket,ConnectSocket ;
   char hostname[128] ;
   FILE *fp ; char tbuf[1024] ; char *b,*b1 ; int len,line ;
   int port ;
   struct XTP__Info xibufs[XIMAX],*xip ; int xicnt = 0 ;
   struct XTP__Info xi ;
   int oldjobno1 ;
   int restartHHMM=UNUSED, restartDayOfMonth=0 ;
   time_t timer ;
   struct tm *tm ;
#ifdef WINNT
  int didWSAStartup = FALSE ;
  OFSTRUCT ofs ;
  WSADATA wsad ;
  DWORD dwThreadId ;
  SYSTEMTIME st ;
#endif
#ifdef UNIX
  pthread_t thread ;
  time_t clock ;
#endif
  int i,err,heartbeat ; char ackmsg[128] ;


	Log("xvXMLServer %d.%d",VXMLServer_Major,VXMLServer_Minor) ;

/*	Set up some defaults */
//	strcpy(xi.xv3Command,"xv3 midasxml %d %d \"%s\"") ;
	strcpy(xi.xv3Command,"xv3 midasxml \"%s\"") ;
	strcpy(xi.dirWork,"./") ;
	ackmsg[0] = '\0' ; strcpy(xi.filePrefix,"midasxml-") ;
	port = 3011 ; heartbeat = 600 ;
/*	Get name of initialization file & parse results */
	if (argc > 1)
	 { fp = fopen(argv[1],"r") ;
	   if (fp == NULL)
	    { printf("? Error (%d) accessing initialization file (%s)\n",errno,argv[1]) ; exit(EXITABORT) ; } ;
	   for(line=1;;line++)
	    { if (fgets(tbuf,sizeof tbuf,fp) == NULL) break ;
	      if (tbuf[0] == '!') continue ; if (tbuf[0] == '/') continue ;
	      len = strlen(tbuf)-1 ; if (tbuf[len] < 20) tbuf[len] = '\0' ;
	      b = strchr(tbuf,'=') ;		/* Parse keyword=value */
	      if (b == NULL) { printf("? Syntax error, ignoring... - %s\n",tbuf) ; continue ; } ;
	      *b = '\0' ; b++ ;
	      for(b1=tbuf;*b1!='\0';b1++) { *b1 = toupper(*b1) ; } ;
	      len = strlen(tbuf) ;
	      if (len < 3) { printf("? Initialization file line %d: keyword (%s) must be at least 3 characters",line,tbuf) ; continue ; } ;
	      for(i=0;strlen(inikeylist[i])>0;i++) { if (strncmp(inikeylist[i],tbuf,len) == 0) break ; }
	      if (strlen(inikeylist[i]) == 0)
	       { printf("? Initialization file line %d: invalid keyword(%s)",line,tbuf) ; continue ; } ;
	      switch (i+1)
	       { case 1:	/* HeartBeat */		heartbeat = atoi(b) ; break ;
	         case 2:	/* Port */		port = atoi(b) ; break ;
	         case 3:	/* xv3Command */	strcpy(xi.xv3Command,b) ; break ;
	         case 4:	/* Restart */		restartHHMM = atoi(b) ; break ;
	         case 5:	/* FilePrefix */	strcpy(xi.filePrefix,b) ; break ;
	         case 6:	/* Directory */		strcpy(xi.dirWork,b) ;
							if (xi.dirWork[strlen(xi.dirWork)-1] != '/') strcat(xi.dirWork,"/") ;
							break ;
	       } ;
	    } ;
	   fclose(fp) ;
	 } ;
#ifdef SIGCLD
	signal(SIGCLD,SIG_IGN) ;	/* Don't want any zombies hanging around because we are not doing explicit "wait()s" */
#endif
#ifdef WINNT
	if (!didWSAStartup)
	 { didWSAStartup = TRUE ;
	   i = MAKEWORD(1,1) ;		/* Always accept highest version */
	   WSAStartup((WORD)i,&wsad) ;
	 } ;
	if (restartHHMM != UNUSED)	/* Don't want to restart today if already past restart time */
	 { GetLocalTime(&st) ;
	   if(((st.wHour * 100) + st.wMinute) > restartHHMM)
	    { Log("Current time past scheduled restart, will not restart until tomorrow") ;
	      restartDayOfMonth = st.wDay ;
	    } else { Log("Scheduled restart for %d",restartHHMM) ; } ;
	 } ;
#endif
#ifdef UNIX
	if (restartHHMM != UNUSED)	/* Don't want to restart today if already past restart time */
	 { time(&clock) ; tm = localtime(&clock) ;
	   if(((tm->tm_hour * 100) + tm->tm_min) > restartHHMM)
	    { Log("Current time past scheduled restart, will not restart until tomorrow") ;
	      restartDayOfMonth = tm->tm_mday ;
	    } else { Log("Scheduled restart for %d",restartHHMM) ; } ;
	 } ;
#endif
	if ((PrimarySocket = socket(AF_INET, SOCK_STREAM,0)) < 0)
	 { Log("? Error (%d) creating INET Socket",errno) ; exit(EXITABORT) ;
	 } ;
	i = 1 ;
	if (NETIOCTL(PrimarySocket,FIONBIO,&i) != 0)
	 { Log("? Could not set socket to NOBLOCK") ; exit(EXITABORT) ;
	 } ;
	memset(&sin,0,sizeof sin) ; sin.sin_family = AF_INET ; sin.sin_port = htons(port) ;
	gethostname(hostname,sizeof(hostname)) ;		/* Get host name */
	Log("Starting server on host %s, port %d, hearbeat %d, directory %s",hostname,port,heartbeat,xi.dirWork) ;
	hp = gethostbyname(hostname) ;
	if (hp != NULL) { memcpy(&sin.sin_addr,hp->h_addr,hp->h_length) ; }
	 else { sin.sin_addr.s_addr = inet_addr(hostname) ;
		if (sin.sin_addr.s_addr == -1)
		 { Log("? Error (%d) obtaining network address of host (gethostbyname/addr(%s))",errno,hostname) ; exit(EXITABORT) ;
		 } ;
	      } ;	
	if (bind(PrimarySocket,&sin,sizeof sin) < 0)
	 { Log("? Error (%d) bind'ing INET Socket to name (%s)",NETERROR,hostname) ; exit(EXITABORT) ; } ;
	if (listen(PrimarySocket,25) < 0)
	 { Log("? Error (%d) listen'ing on INET Socket (%s)",NETERROR,hostname) ; exit(EXITABORT) ; } ;
	for(xi.jobno2=1;;xi.jobno2++)
	 { Log("Waiting on accept") ;
	   for(;;)
	    {
	      FD_ZERO(&sset) ; FD_SET(PrimarySocket,&sset) ;
	      tev.tv_sec = heartbeat ; tev.tv_usec = 0 ;
	      i = select(FD_SETSIZE,&sset,NULL,NULL,&tev) ;	/* Wait for something! */
	      if (i > 0) break ;
#ifdef WINNT
	      if (restartHHMM != UNUSED)
	       { GetLocalTime(&st) ;
	         if (restartDayOfMonth != st.wDay && ((st.wHour * 100) + st.wMinute) > restartHHMM)
	          { Log("Scheduled daily shutdown: %d",restartHHMM) ; goto shutdown ; } ;
	       } ;
#endif
#ifdef UNIX
	      if (restartHHMM != UNUSED)
	       { time(&clock) ; tm = localtime(&clock) ;
	         if (restartDayOfMonth != tm->tm_mday && ((tm->tm_hour * 100) + tm->tm_min) > restartHHMM)
	          { Log("Scheduled daily shutdown: %d",restartHHMM) ; goto shutdown ; } ;
	       } ;
#endif
	      Log("idle heartbeat") ;
	    } ;
	   i = sizeof fsin ;
	   if ((ConnectSocket = accept(PrimarySocket,&fsin,&i)) < 0)
	    { Log("?Error (%d) accept'ing on INET Socket (%s)",NETERROR,hostname) ; exit(EXITABORT) ; } ;
	   time(&timer) ; tm = localtime(&timer) ;
	   xi.jobno1 = (tm->tm_year+1900) * 10000 + (tm->tm_mon + 1) * 100 + tm->tm_mday ;
	   if (oldjobno1 != xi.jobno1)
	    { oldjobno1 = xi.jobno1 ; xi.jobno2 = 1 ;
	      Log("Starting begin of new day: %d/%d/%d",tm->tm_mon+1,tm->tm_mday,tm->tm_year+1900) ;
	    } ;
	   if (strlen(ackmsg) > 0) send(ConnectSocket,ackmsg,strlen(ackmsg),0) ;
	   Log("Sent Ack, spawning job %d-%d for socket %d",xi.jobno1,xi.jobno2,ConnectSocket) ;
	   xi.ConnectSocket = ConnectSocket ;
	   xicnt++ ; xip = &xibufs[xicnt % XIMAX] ; *xip = xi ;
#ifdef WINNT
	   CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)xml_Process,xip,0,&dwThreadId) ;
#endif
#ifdef UNIX
	   err = pthread_create(&thread,NULL,xml_Process,xip) ;
	   if (err != 0)
	    { Log("Error(%d) in ALPHA pthread_create - restarting program...",err) ; goto shutdown ; } ;
#endif
	 } ;

shutdown:
	SOCKETCLOSE(PrimarySocket) ;
#ifdef WINNT
	for(i=0;i<10;i++)
	 { WSACleanup() ; err = WSAGetLastError() ;
	   if (err == WSANOTINITIALISED) break ;
	   if (err == WSAEINPROGRESS) WSACancelBlockingCall() ;
	 } ;
#endif
	exit(EXITOK) ;
}


void xml_Process(xiptr)
  struct XTP__Info *xiptr ;
{ 
  struct timeval tev ;
#define MAX_MSG 0x10000
  char Msg[MAX_MSG] ;		/* Messages should not be that big */
  fd_set sset ;
  FILE *fp ;
  struct XTP__Info xi ;
  char *ptr ; int res ; char cmd[2048] ; char errBuf[2048] ;
  char *outbuf,*b ; int chunk,sent,lenToSend,bytes,olen,tries ;

	outbuf = NULL ; lenToSend = -1 ;
	xi = *xiptr ;		/* Copy into local buffer */
	Log("Begin of Message on job(%d-%d)",xi.jobno1,xi.jobno2) ;

	for(ptr=Msg,bytes=0;bytes<sizeof Msg;bytes++,ptr++)
	 { 
	   if (ptr != Msg)			/* Don't wait on first byte */
	    { FD_ZERO(&sset) ; FD_SET(xi.ConnectSocket,&sset) ;
	      tev.tv_sec = 30 ; tev.tv_usec = 0 ;		/* Set max wait for 30 seconds */
	      res = select(FD_SETSIZE,&sset,NULL,NULL,&tev) ;	/* Wait for something to happen */
	      if (res <= 0)
	       { Log("? Timeout waiting for input on job(%d-%d)",xi.jobno1,xi.jobno2) ; SOCKETCLOSE(xi.ConnectSocket) ; return ; } ;
	    } ;
	   res = recv(xi.ConnectSocket,ptr,1,0) ;
	   if (res <= 0)
	    { if (res == 0)
	       { Log("? Remote connection has shut down gracefully on job(%d-%d)",xi.jobno1,xi.jobno2) ;
	       } else { Log("? Error (%d) in recv(=%d socket=%d) on job(%d-%d)",NETERROR,res,xi.ConnectSocket,xi.jobno1,xi.jobno2) ; } ;
	      SOCKETCLOSE(xi.ConnectSocket) ; return ;
	    } ;
	   if (*ptr == EOM) break ;				/* ^Z indicates end of message */
	 } ;
	if (bytes >= sizeof Msg)
	 { Log("? Input message exceeds %d bytes",sizeof Msg) ; SOCKETCLOSE(xi.ConnectSocket) ; return ; } ;
	*ptr = '\0' ;					/* Terminate with null */

process_msg:
/*	Create midasxml-jobnumber.in file */
	{ FILE *fp ; char inFile[128] ;
	  sprintf(inFile,"%s%s%d-%d.in",xi.dirWork,xi.filePrefix,xi.jobno1,xi.jobno2) ;
	  fp = fopen(inFile,"w") ; fwrite(Msg,bytes,1,fp) ; fclose(fp) ;
	  b = strchr(inFile,'.') ; *b = '\0' ;		/* Strip off extension before including in command below */
	  sprintf(cmd,xi.xv3Command,inFile) ;
	}
	Log("Processing:*%s*\n",cmd) ;


	if (!v_SpawnProcess(NULL,cmd,errBuf))
	 { Log("? Could not spawn subprocess job(%d-%d)",xi.jobno1,xi.jobno2) ; goto finish_job ; } ;

	{ struct stat statbuf ; FILE *fp ; char outFile[128] ;
	  sprintf(outFile,"%s%s%d-%d.out",xi.dirWork,xi.filePrefix,xi.jobno1,xi.jobno2) ;
	  stat(outFile,&statbuf) ; lenToSend = statbuf.st_size ;
	  fp = fopen(outFile,"r") ;
	  if (fp == NULL)
	   { Log("? Could not open result file: %s\n",outFile) ; outbuf = malloc(128) ; strcpy(outbuf,"0\tCould not locate result file") ; lenToSend = strlen(outbuf) ; }
	   else { outbuf = malloc(lenToSend) ; fread(outbuf,1,lenToSend,fp) ; fclose(fp) ; } ;
	}

	chunk = 4096 ; if (chunk > lenToSend) chunk = lenToSend ;
#define SEND_WAIT_SECONDS 3
	for(sent=0,res=1,tries=0;tries<100 && res>0&&sent<lenToSend;)
	 { fd_set sset ; struct timeval tev ;
	   FD_ZERO(&sset) ; FD_SET(xi.ConnectSocket,&sset) ;
	   tev.tv_sec = SEND_WAIT_SECONDS ; tev.tv_usec = 0 ;
	   res = select(FD_SETSIZE,NULL,&sset,NULL,&tev) ;	/* Wait for ok to send */
	   if (res <= 0)
	    { Log("? Timeout waiting for input on job(%d-%d)",xi.jobno1,xi.jobno2) ; goto finish_job ;
	    } ;
	   res = send(xi.ConnectSocket,&outbuf[sent],chunk,0) ; if (res > 0) sent+=res ; 
	   if (res <= 0 && NETERROR == EWOULDBLOCK) { HANGLOOSE(50) ; tries++ ; res = 1 ; continue ; } ;
	   if (chunk > lenToSend-sent) chunk = lenToSend-sent ; tries = 0 ;
	 } ;

finish_job:
/*	All done - SOCKETCLOSE the socket */
	Log("Finished job (%d-%d) - closing socket after sending %d / %d bytes",xi.jobno1,xi.jobno2,lenToSend,sent) ;
	SOCKETCLOSE(xi.ConnectSocket) ;
	if (outbuf != NULL) free(outbuf) ;
}

/*	Log - Logs message to stdout & date dependent log file */

#ifdef WINNT
  CRITICAL_SECTION CS ;
  int initCS = FALSE ;
#endif

void Log(char *msg, ...)
{ 
  struct tm *tm ;
  FILE *fp ;
  va_list ap ;
  time_t timer ;
  char filename[V_FileName_Max],timebuf[50],lbuf[4096] ;

/*	Do special formatting on error message */
#ifdef WINNT
	if (!initCS) { initCS = TRUE ; InitializeCriticalSection(&CS) ; } ;
#endif
	va_start(ap,msg) ; vsprintf(lbuf,msg,ap) ; va_end(ap) ;
	time(&timer) ; tm = localtime(&timer) ;
	sprintf(timebuf,"%02d:%02d:%02d",tm->tm_hour,tm->tm_min,tm->tm_sec) ;
	printf("%s %s\n",timebuf,lbuf) ;
	sprintf(filename,"vxmlserver%02d%02d.log",tm->tm_mon+1,tm->tm_mday) ;
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


#ifdef LINUX486
v_Sleepms(ms)			/* Sleep for ms milliseconds */
  int ms ;
{ struct timeval tev ;

	tev.tv_sec = 0 ; tev.tv_usec = ms * 1000 ;
	select(1,NULL,NULL,NULL,&tev) ;
}
#endif


/*	v_SpawnProcess - Spawns a subprocess, with/without argument, with/without waiting for return	*/
/*	Call: res = v_SpawnProcess( exefile, argbuf , errbuf)
	  where res is TRUE/FALSE if OK, not OK,
		exefile is executable to run, if NULL then exe implied in argbuf,
		argbuf is ptr to string argument, NULL if none,
		errbuf is optional buffer for error message if problem, NULL if none			*/


int v_SpawnProcess(exefile, argbuf,errbuf)
  char *exefile ;
  char *argbuf ;
  char *errbuf ;
{
#ifdef UNIX
  char tb[512],*bp1 ;
#endif
#ifdef VMSOS
  int res,resx ;
   struct vms__dsc {
     short int length ;		/* Length of value */
     short int desc ;		/* Descriptive info */
     char *pointer ;		/* Pointer to value */
    } strdsc ;
#endif
#ifdef WINNT
   char *param,file[V_FileName_Max] ;
   int i ;
#endif
  int waitMS ;
#ifdef WINNT

	        { DWORD threadExitCode,res ; PROCESS_INFORMATION pi ;	/* Get process info from CreateProcess */
	          HANDLE hThread ;
		  STARTUPINFO si ;		/* Process startup info */
		  memset(&si,0,sizeof si) ; si.cb = sizeof si ;
		  GetStartupInfo(&si) ;
		  si.lpTitle = "V4 Spawn" ;
		  si.dwFlags |= STARTF_USESTDHANDLES ;		/* Want subprocess to use handles of this process */
		  { HANDLE hDupStdOutput ;
		    if (!DuplicateHandle(GetCurrentProcess(),GetStdHandle(STD_OUTPUT_HANDLE),GetCurrentProcess(),&hDupStdOutput,0,TRUE,DUPLICATE_SAME_ACCESS))
		     { printf("???DuplicateHandle() failed\n") ;
		     } ;
		    si.hStdOutput = hDupStdOutput ;
		  } ;
		  if (!CreateProcess(exefile,(argbuf == NULL ? "cmd" : argbuf),NULL,NULL,TRUE,CREATE_UNICODE_ENVIRONMENT,NULL,NULL,&si,&pi))
		   { 
//		     if (errbuf != NULL) v_Msg(NULL,errbuf,"SpawnFail","CreateProcess",(argbuf == NULL ? "cmd" : argbuf),GetLastError()) ;
		     return(FALSE) ;
		   } ;
		  res = WaitForSingleObject(pi.hProcess,10000) ;
		  if (res != WAIT_OBJECT_0)		/* If process did not terminate normally then force a termination */
		   { TerminateProcess(pi.hProcess,EXITABORT) ;
		   } ;
		  CloseHandle(pi.hProcess) ; CloseHandle(pi.hThread) ;
		  switch (res)
		   { default:			return(FALSE) ; //if (errbuf != NULL) v_Msg(NULL,errbuf,"SpawnFail","WaitForSingleObject",(argbuf == NULL ? "cmd" : argbuf),GetLastError()) ;
		     case WAIT_TIMEOUT:		return(FALSE) ; //v_Msg(NULL,errbuf,"SpawnTO",(argbuf == NULL ? UClit("cmd") : argbuf),(waitMS/1000)) ; return(FALSE) ;
		     case WAIT_OBJECT_0:	break ;
		   } ;
		}
#endif
#ifdef UNIX
	bp1 = getenv("SHELL") ;			/* Want to run same shell as parent */
	if (bp1 == NULL) bp1 = "/bin/sh" ;
	strcpy(tb,bp1) ;
	if (fork() == 0)
	 { if (execlp(tb,tb,(argbuf != NULL ? "-c" : NULL),argbuf,0) == -1)
	    { //if (errbuf != NULL) v_Msg(NULL,errbuf,"execlp","ShellExecuteEx",(argbuf == NULL ? "" : argbuf),errno) ;
	      return(FALSE) ;
	    } ;
	 } else wait(NULL) ;
#endif
	return(TRUE) ;
}
