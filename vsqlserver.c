/*	vsqlserver.c - Server for V4-MIDAS-SQL Interface			*/

#include <signal.h>
#include <time.h>
#define NEED_SOCKET_LIBS 1
#include "vconfig.c"
#include <errno.h>
#include <stdio.h>

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
	  cc -o xvsqlserver -O3 -w  vsqlserver.c  -lm -lc -lrt
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
void sql_Process() ;

#define XIMAX 50
struct XTP__Info {
  int ConnectSocket ;
  int jobno1, jobno2 ;
  char xv3Command[128] ;
 } ;

#define DFLT_HEARTBEAT_WAIT 300	/* Seconds to wait for idle heart beat status */
#define EOM 26

char inikeylist[][16] =
 { "HEARTBEAT", "PORT", "XV3COMMAND", "RESTART", ""
 } ;
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


/*	Set up some defaults */
//	strcpy(xi.xv3Command,"xv3 midassql %d %d \"%s\"") ;
	strcpy(xi.xv3Command,"xv3 midassql \"%s\"") ;
	ackmsg[0] = '\0' ;
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
	Log("Starting server on host %s",hostname) ;
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
	   CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)sql_Process,xip,0,&dwThreadId) ;
#endif
#ifdef UNIX
	   err = pthread_create(&thread,NULL,sql_Process,xip) ;
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


void sql_Process(xiptr)
  struct XTP__Info *xiptr ;
{ 
  struct timeval tev ;
  char Msg[2048] ;		/* Messages should not be that big */
  fd_set sset ;
  FILE *fp ;
  struct XTP__Info xi ;
  char *ptr ; int res ; char cmd[2048] ;
  char *outbuf,*b ; int len,sent,olen,bytes,maxbytes,maxbytessize,ok ;

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
//	sprintf(cmd,xi.xv3Command,xi.jobno1,xi.jobno2,Msg) ;
	sprintf(cmd,xi.xv3Command,Msg) ;
	Log("Processing:*%s*\n",cmd) ;

#ifdef UNIX
	fp = popen(cmd,"r") ;
#endif
#ifdef WINNT
	fp = _popen(cmd,"r") ;
#endif
return_to_client:		/* Return contents of "fp" to client */
	maxbytessize = sizeof maxbytes ;
	if (getsockopt(xi.ConnectSocket,SOL_SOCKET,SO_SNDBUF,&maxbytes,&maxbytessize) != 0) maxbytes = 256 ;
	if (maxbytes > 0xfffff) maxbytes = 8192 ;
//	Log("Send buffer size = %d",maxbytes) ;
	outbuf = malloc(maxbytes +100) ; olen = 0 ;
	for(ok=TRUE;fp!=NULL && ok;)
	 { if ((b=fgets(Msg,sizeof Msg,fp)) == NULL)
	    { outbuf[olen++] = EOM ; ok = FALSE ; }
	    else { char save ;
//		   if (Msg[0] == '?' || Msg[0] == '%')		/* Got V3 error - then log it but don't include in return */
//		    { Log("Ignore %d-%d(%d):*%s*",xi.jobno1,xi.jobno2,len-1,Msg) ;  continue ; }
		   if (Msg[0] != '~')				/* If does not begin with '`' then log it but don't include in return */
		    { Log("Ignore %d-%d(%d):*%s*",xi.jobno1,xi.jobno2,len-1,Msg) ;  continue ; }
		    else { Log("Reply %d-%d(%d):*%s*",xi.jobno1,xi.jobno2,len-1,Msg) ; } ;
		   len = strlen(b) - 1 ; if (len > 0) { save = b[len-1] ; b[len-1] = '\0' ; } ;
		   if (len > 0) b[len-1] = save ;
		   if ((olen + len) < maxbytes - 5)		/* Build up message about as big as we can go */
		    { strcpy(&outbuf[olen],&Msg[1]) ; olen += (len + 1) ; continue ; } ;
		 } ;
	   FD_ZERO(&sset) ; FD_SET(xi.ConnectSocket,&sset) ;
	   tev.tv_sec = 30 ; tev.tv_usec = 0 ;			/* Set max wait for 30 seconds */
	   res = select(FD_SETSIZE,NULL,&sset,NULL,&tev) ;	/* Wait for ok to send */
	   if (res <= 0)
	    { Log("? Timeout waiting for send-ok on job(%d-%d)",xi.jobno1,xi.jobno2) ; SOCKETCLOSE(xi.ConnectSocket) ; return ; } ;


	   for(sent=0;sent<olen;sent+=(res > 0 ? res : 0))
	    { res = send(xi.ConnectSocket,&outbuf[sent],olen-sent,0) ;
	      if (res >= olen-sent) break ;
	      if (NETERROR == EWOULDBLOCK)
	       { HANGLOOSE(50) ; 
	         Log("Got blocking condition after sending %d bytes...",sent) ;
		 continue ;
	       } ;
	      Log("? Error (%d) (sent = %d, len = %d/%d) in send() for job (%d-%d)",NETERROR,sent,len,res,xi.jobno1,xi.jobno2) ;
	      Msg[0] = EOM ; send(xi.ConnectSocket,Msg,1,0) ;	/* Attempt to end with EOM */
	      SOCKETCLOSE(xi.ConnectSocket) ; free(outbuf) ; return ;
	    } ;
	   olen = 0 ;						/* Reset output buffer & init with current Msg */
	   strcpy(&outbuf[olen],Msg) ; olen += (len + 1) ; 
	 } ;
	if (fp != NULL)
#ifdef UNIX
	 pclose(fp) ;
#endif
#ifdef WINNT
	 _pclose(fp) ;
#endif
finish_job:
/*	All done - SOCKETCLOSE the socket */
	Log("Finished job (%d-%d) - closing socket",xi.jobno1,xi.jobno2) ;
	SOCKETCLOSE(xi.ConnectSocket) ;
	free(outbuf) ;
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
	sprintf(filename,"vsqlserver%02d%02d.log",tm->tm_mon+1,tm->tm_mday) ;
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
