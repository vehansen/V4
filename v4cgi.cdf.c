/*	v4CGI.c - Handles cgi interface from web requests	*/

/*	To Build on Windows-
	  
	  cl /O2 /Fecdfwhcgi.exe /w -D CDFWH=1 v4cgi.c kernel32.lib wsock32.lib advapi32.lib user32.lib
	  cl /O2 /Fecdfwhcgitest.exe /w -D CDFWHTEST=1 v4cgi.c kernel32.lib wsock32.lib advapi32.lib user32.lib
	  cl /O2 /Fecdfmkscgi.exe /w -D CDFMKS=1 v4cgi.c kernel32.lib wsock32.lib advapi32.lib user32.lib
	  cl /O2 /Fecdfcriscgi.exe /w -D CDFCRIS=1 v4cgi.c kernel32.lib wsock32.lib advapi32.lib user32.lib
	  cl /O2 /Fecdftestcriscgi.exe /w -D CDFTESTCRIS=1 v4cgi.c kernel32.lib wsock32.lib advapi32.lib user32.lib
	  
	  	
*/


#include <signal.h>
#define NEED_SOCKET_LIBS 1
#include "vconfig.c"
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>

#ifdef UNIX
#include <termio.h>
#include <time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef LINUX486
#include <varargs.h>
#define _O_BINARY 0
#define TRUE 1
#define FALSE 0
#endif
#endif

#ifdef WINNT
#include <windows.h>
#ifndef GETADDR
#include <winsock.h>
#endif
#include <io.h>
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EINPROGRESS WSAEINPROGRESS
#endif

#define CGI_CHECK1SEND TRUE				/* If TRUE then check before send() of each byte */
#define LOGLINEMAX 256

#if defined CDFWTE
 #ifdef GETADDR
  #define V4SERVERNAME "cdf-www.cherrydale.local/2351"	/* IP Address of V4 server when running WTE on CDF Webserver (www.cherrydalefarms.com) */
 #else
  #define V4SERVERNAME "172.31.10.129/2351"		/* IP Address of V4 server when running WTE on CDF Webserver (www.cherrydalefarms.com) */
 #endif
  #define CGI_LOG_PREFIX "wte"
#elif defined CDFWH
  #define V4SERVERNAME "172.31.10.25/2370"		/* IP Address of V4 server when running CRIS on CDF V4 Server */
  #define CGI_LOG_PREFIX "cdfwh"
#elif defined CDFWHTEST
  #define V4SERVERNAME "172.31.10.25/2378"		/* Temporary build to diagnose problems (VEH091211) */
  #define CGI_LOG_PREFIX "cdfwhtest"
#elif defined CDFMKS
  #define V4SERVERNAME "192.168.120.134/2382"		/* IP Address of CDF (demo) running on MKS server */
  #define CGI_LOG_PREFIX "cdfmks"
#elif defined CDFCRIS
  #define V4SERVERNAME "172.31.10.26/2390"		/* IP Address of CDF (demo) running on MKS server */
  #define CGI_LOG_PREFIX "cdfcris"
#elif defined CDFTESTCRIS
  #define V4SERVERNAME "172.31.10.27/2390"		/* IP Address of CDF TEST running on CDF server */
  #define CGI_LOG_PREFIX "cdftestcris"
#elif ENVIRONMENT
  #define V4SERVERNAME "ENVIRONMENT"			/* Pull ip-address/port from environment variable */
  #define CGI_LOG_PREFIX "xxx"
#else
#error No V4SERVER defined
#endif

void vcgi_ErrExit() ;
void vcgi_Log(char *,...) ;
double v_CPUTime(LOGICAL) ;
char *v_OSErrString() ;

char **startup_envp, *peerAddr=NULL ;
char lbuf[LOGLINEMAX+256] ;	/* Holds current/last log line */
char serverport[64],exeName[64] ;

main(argc,argv,envp)
  int argc ;			/* Number of arguments passed via DCL */
  char *argv[] ;		/* Argument values */
  char **envp ;			/* Environment pointer */
{
#ifdef GETADDR
  struct addrinfo hints, *res;
  char *port,*server ;
#else
  struct sockaddr_in sin ;
  struct hostent *hp ;
  char *bp ;
#endif
  unsigned char hostname[128],rbuf[512] ;
  int sres ;
  int ConnectSocket,PrimarySocket ;
  fd_set sset ;				/* Socket set */
#ifdef WINNT
  WSADATA wsad ;
  int didWSAStartup=FALSE ;
  struct timezone {
   int  tz_minuteswest; /* minutes W of Greenwich */
   int  tz_dsttime;     /* type of dst correction */
  };
  ULARGE_INTEGER uli,Baseuli ;
#define CHKPTTIME(TV) GetSystemTimeAsFileTime(&uli) ; TV = (uli.QuadPart - Baseuli.QuadPart) / 10 ;
#else
#define CHKPTTIME(TV) gettimeofday(&tev,&tez) ; TV = (tev.tv_sec - Basetv_sec) * 1000000 + tev.tv_usec ;
#endif
  struct timeval tev ;
  int tvStart,tvConnect ;
  char *reqMethod, *contentType, *content, *queryString ; char sbuf[4096] ;
  int contentLen ;
  int ok,bytessent,i ;
  double fpVar ;

	startup_envp = envp ;
	v_CPUTime(TRUE) ;
	fpVar = 123.456 ;			/* Do this to force load of floating point libraries (http://msdn.microsoft.com/en-us/library/k1x26e0x(VS.71).aspx) */

	setvbuf (stdout, NULL, _IONBF, 0) ;	/* Make sure we don't buffer to stdout */
	strcpy(serverport,V4SERVERNAME) ;	/* Set up default server/port */
	strcpy(exeName,CGI_LOG_PREFIX) ; strcat(exeName,"cgi") ;
	
	if (strcmp(serverport,"ENVIRONMENT") == 0)
	 { char *b, *d, exePath[V_FileName_Max] ;
	   GetModuleFileName(NULL,exePath,sizeof exePath) ;
	   b = strrchr(exePath,'.') ; if (b != '\0') *b = '\0' ;	/* Strip off extension */
	   for(b=exePath,d=exePath;*b!='\0';b++)
	    { switch(*b)
	       { case '\\':
	         case '.':
	         case ':':	d = b + 1 ; break ;
	       } ;
	    } ;
	   b = getenv(d) ; if (b == NULL) { vcgi_Log("? Cannot find environment value for %s",d) ; vcgi_ErrExit() ; } ;
	   if(strchr(b,'/') == NULL) { vcgi_Log("? Cannot find '/port' in variable value: %s",b) ; vcgi_ErrExit() ; } ;
	   strcpy(serverport,b) ;
	   strcpy(exeName,d) ;
	 } ;

/*	Rather than try to parse all this stuff (as we did with older versions) just convert this back to HTTP & send to v4feserver to decode */
	reqMethod = getenv("REQUEST_METHOD") ; if (reqMethod == NULL) reqMethod = "GET" ;
	queryString = getenv("QUERY_STRING") ; if (queryString == NULL) queryString = "" ;
	peerAddr = getenv("REMOTE_ADDR") ;
	if (stricmp(reqMethod,"GET") == 0)
	 { if (strlen(queryString) == 0)
	    { vcgi_Log("? No arguments given to program") ; vcgi_ErrExit() ; } ;
	 } else if (stricmp(reqMethod,"POST") == 0)
	 { int res ;
	   contentLen = atoi(getenv("CONTENT_LENGTH")) ;
	   if (contentLen <= 0 || contentLen > 10000000)
	    { char buf[512] ; char *cl = getenv("CONTENT_LENGTH") ;
	      if (cl == NULL) cl = "(null)" ;
	      sprintf(buf,"? Content length must be greater than 0 & less than 10mb - /cl %d '%s' /rm %p '%s' /qs %p '%s'",contentLen,cl,getenv("REQUEST_METHOD"),reqMethod,getenv("QUERY_STRING"),queryString) ;
	      vcgi_Log(buf) ;
	      vcgi_ErrExit() ;
	    } ;
	   content = (char *)malloc(contentLen+10) ;
	   if (setmode(fileno(stdin),_O_BINARY) == -1)			/* Binary translation - don't want any text translation */
	    vcgi_Log("? Error (%s) setting binary mode",v_OSErrString(errno)) ;
           res = fread(content,1,contentLen,stdin) ;
           if (!res)
	    { char xx[512] ; sprintf(xx,"? Could not read (res=%d) CGI input (contentLen=%d) from STDIN: %s",res,contentLen,v_OSErrString(errno)) ;
	      vcgi_Log(xx) ; vcgi_ErrExit() ;
	    } ;
	   if (res != contentLen)
	    { char xx[512],fileName[64] ; FILE *fp ;
	      sprintf(fileName,"cgidump-%d.bin",clock()) ;
	      sprintf(xx,"? Bytes actually read (%d) not equal to content-length (%d). See dump file %s. Continuing anyway...",res,contentLen,fileName) ; vcgi_Log(xx) ;
	      fp = fopen(fileName,"wb") ;
	      if (fp != NULL) { fwrite(content,1,res,fp) ; fclose(fp) ; } ;
	    } ;
	   content[contentLen] = '\0' ;
	   contentType = getenv("CONTENT_TYPE") ;
         } else
	 { vcgi_Log("? unsupported CGI Request Method (%s)",reqMethod) ; vcgi_ErrExit() ; } ;

/*	Make connection to server */
	CHKPTTIME(tvConnect) ;
#ifdef WINNT
	if (!didWSAStartup)
	 { didWSAStartup = TRUE ;
	   i = MAKEWORD(1,1) ;		/* Always accept highest version */
	   WSAStartup(i,&wsad) ;
	 } ;
#endif
#ifdef GETADDR
/*	Check SERVERNAME for host/port number, if no host then use current host */
	port = strchr(serverport,'/') ;			/* Look for host delimiter */
	if (port == NULL) { server = NULL ; port = serverport ; }
	 else { strncpy(hostname,serverport,port-serverport) ; hostname[port-serverport] = '\0' ;
		server = hostname ; port = port + 1 ;
	      } ;
	memset(&hints,0,sizeof hints) ;
	 hints.ai_family = AF_UNSPEC ; hints.ai_socktype = SOCK_STREAM ; hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
	if (getaddrinfo(server,port,&hints,&res) != 0)
	 { vcgi_Log("? Error (%s) getting IP address info for %s/%s",v_OSErrString(NETERROR),server,port) ;
	   vcgi_ErrExit() ;
	 } ;
	if ((PrimarySocket = socket(res->ai_family,res->ai_socktype,res->ai_protocol)) < 0)
	 { vcgi_Log("? Error (%s) creating INET Socket (%s:%s)",v_OSErrString(NETERROR),hostname,port) ;
	   vcgi_ErrExit() ;
	 } ;
	if (connect(PrimarySocket,res->ai_addr,res->ai_addrlen) < 0)
	 { if (!(NETERROR == EWOULDBLOCK || NETERROR == EINPROGRESS))
	    { vcgi_Log(" Error (%s) in connect()",v_OSErrString(NETERROR)) ; vcgi_ErrExit() ; } ;
	 } ;
#else
	if ((PrimarySocket = socket(AF_INET, SOCK_STREAM,0)) < 0)
	 { vcgi_Log("? Error (%s) creating INET Socket (%s)",v_OSErrString(NETERROR),serverport) ;
	   vcgi_ErrExit() ;
	 } ;
/*	Check SERVERNAME for host/port number, if no host then use current host */
	bp = strchr(serverport,'/') ;			/* Look for host delimiter */
	memset(&sin,0,sizeof sin) ; sin.sin_family = AF_INET ;
	if (bp == NULL)
	 { sin.sin_port = htons(atoi(serverport)) ;
	   gethostname(hostname,sizeof(hostname)) ;		/* Get host name */
	   hp = gethostbyname(hostname) ;
	 } else
	 { sin.sin_port = htons(atoi(bp+1)) ;			/* Convert after '/' as port number */
	   i = bp - serverport ;				/* Length of host name */
	   strncpy(hostname,serverport,i) ; hostname[i] = 0 ;
	   hp = gethostbyname(hostname) ;
	 } ;
	if (hp != NULL) { memcpy(&sin.sin_addr,hp->h_addr,hp->h_length) ; }
	 else { sin.sin_addr.s_addr = inet_addr(hostname) ;
		if (sin.sin_addr.s_addr == -1)
		 { vcgi_Log("? Error (%s) obtaining network address of host (gethostbyname/addr(%s))",v_OSErrString(NETERROR),hostname) ;
		   vcgi_ErrExit() ;
		 } ;
	      } ;
	if (connect(PrimarySocket,&sin,sizeof sin) < 0)
	 { if (!(NETERROR == EWOULDBLOCK || NETERROR == EINPROGRESS))
	    { char xx[512] ; sprintf(xx,"? Error (%s) connect'ing INET Socket to name (%s:%s)",v_OSErrString(NETERROR),hostname,serverport) ;
	      vcgi_Log(xx) ;
	      vcgi_ErrExit() ;
	    } ;
	 } ;
#endif
	ConnectSocket = PrimarySocket ;

/*	Send reformatted CGI input back to MIDAS server */
	if (CGI_CHECK1SEND)
	 { FD_ZERO(&sset) ; FD_SET(ConnectSocket,&sset) ;
	   tev.tv_sec = 45 ; tev.tv_usec = 0 ;			/* Set max wait for 45 seconds */
	   sres = select(FD_SETSIZE,NULL,&sset,NULL,&tev) ;	/* Wait for something to happen */
	   if (sres <= 0)
	    { vcgi_Log("? Socket does not appear to be ready to send()") ;
	      vcgi_ErrExit() ;
	    } ;
	 } ;

#define SENDSOCKET(socket,buf,len,arg) \
 { int _len = len, rlen ; \
   rlen = send(socket,buf,_len,0) ; \
   if (rlen != _len) \
    { vcgi_Log("? Error (%s) (len = %d/%d) in send on socket (%s)",v_OSErrString(NETERROR),_len,rlen,serverport) ; vcgi_ErrExit() ; } ; \
 }

/*	Send off HTTP Headers */
	if (stricmp(reqMethod,"GET") == 0)
	 { sprintf(sbuf,"GET ?%s HTTP/1.1\r\n",queryString) ;
	 } else
	 { if (strlen(queryString) == 0) { sprintf(sbuf,"POST / HTTP/1.1\r\nCONTENT-LENGTH: %d\r\nCONTENT-TYPE: %s\r\n",contentLen,contentType) ; }
	    else { sprintf(sbuf,"POST ?%s HTTP/1.1\r\nCONTENT-LENGTH: %d\r\nCONTENT-TYPE: %s\r\n",queryString,contentLen,contentType) ; } ;
	 } ;


	SENDSOCKET(ConnectSocket,sbuf,strlen(sbuf),0) ;
#define SENDHDR(caption,value) if(getenv(value) != NULL) { sprintf(sbuf,"%s: %s\r\n",caption,getenv(value)) ; SENDSOCKET(ConnectSocket,sbuf,strlen(sbuf),0) ; } ;
	SENDHDR("SERVER-NAME","SERVER_NAME") ;
	SENDHDR("SERVER-PORT","SERVER_PORT") ;
	SENDHDR("SCRIPT-NAME","SCRIPT_NAME") ;
	SENDHDR("COOKIE","HTTP_COOKIE") ;
	SENDHDR("REMOTE-ADDR","REMOTE_ADDR") ;
	SENDSOCKET(ConnectSocket,"FROM-XXXCGI: YES\r\n",strlen("FROM-XXXCGI: YES\r\n"),0) ;
#ifdef V4NS_JSON
	sprintf(sbuf,"V4NS-JSON: %s\r\n",V4NS_JSON) ; SENDSOCKET(ConnectSocket, sbuf,strlen(sbuf),0) ;
#endif
	SENDSOCKET(ConnectSocket,"\r\n",strlen("\r\n"),0) ;

/*	Now send the data */
	if (stricmp(reqMethod,"POST") == 0)
	 { SENDSOCKET(ConnectSocket,content,contentLen,0) ;
	 } ;
/*	Suck up results and send back down the line - s/b fully formatted HTML file */
#ifdef WINNT
	if (setmode(fileno(stdout),_O_BINARY) == -1)			/* Binary translation - don't want any text translation */
	 vcgi_Log("? Error (%s) setting binary mode",v_OSErrString(errno)) ;
#endif	    

/*	Now grab everything coming back & send it through to stdout */
#define NEWWAY 1
#ifdef NEWWAY
	for(bytessent=0;;)
	 { int rbytes,wbytes ;
	   rbytes = recv(ConnectSocket,rbuf,sizeof rbuf,0) ;
	   if (rbytes <= 0)
	    { if (rbytes != 0) vcgi_Log("? Error (%s) in binary recv on socket (%d)",v_OSErrString(NETERROR),serverport) ;
	      break ;
	    } ;
	   wbytes = fwrite(rbuf,1,rbytes,stdout) ; bytessent += wbytes ;
	   if (wbytes != rbytes) vcgi_Log("? Read %d bytes but only wrote %d bytes on socket (%d)",rbytes,wbytes,serverport) ;
	 } ;
#else
	for(bytessent=0;;bytessent++)
	 { 
	   if ((ok=recv(ConnectSocket,rbuf,1,0)) <= 0)
	    { if (ok != 0) vcgi_Log("? Error (%s) in binary recv on socket (%d)",v_OSErrString(NETERROR),serverport) ;
	      break ;
	    } ;
	   putchar(rbuf[0]) ;
	 } ;
	goto close_up ;
#endif

/*	Ready to close up shop */
close_up:
	if (SOCKETCLOSE(PrimarySocket) != 0)
	 { if (serverport == NULL)
	    vcgi_Log("? Error (%s) while attempting close of primary socket (%d) - %s\n",v_OSErrString(NETERROR),PrimarySocket,serverport) ;
	 } ;
	if (PrimarySocket != ConnectSocket)
	 { if (SOCKETCLOSE(ConnectSocket) != 0)
	    vcgi_Log("? Error (%s) in close of connect socket (%d) - %s\n",v_OSErrString(NETERROR),ConnectSocket,serverport) ;
	 } ;

	fclose(stdout) ;
	exit(1) ;
}

/*	U T I L I T I E S  */

/*	vcgi_ErrExit - Error return: opens file & dumps back (via stdout) to WWW client	*/
/*	Call: vcgi_ErrExit()
	** NOTE: this routine does NOT return **					*/

void vcgi_ErrExit()
{ FILE *fp=NULL ;
  static int nesting = 0 ;
  int i ;

	puts("Content-Type: text/html") ;
	puts("") ;
	puts("<html><head>") ;
#ifdef LOGIN_REDIRECT
	puts("<script type='text/javascript'>") ;
	puts("function gotoLogin() { location.href='" LOGIN_REDIRECT "'; }") ;
	puts("</script>") ;
#endif
	puts("</head><body>") ;
	puts("<p style='color: red; size: 16pt; text-align:center; margin-top: 2in;'>The request could not be handled:<br>") ;
	puts(lbuf) ;				/* Write out last log line (hopefully a message) */
	puts("</p>") ;
#ifdef LOGIN_REDIRECT
	puts("<p align='center'><button onclick='gotoLogin()'>Login Page</button></p>") ;
#endif
	puts("</body></html>") ;
	exit(0) ;
}

/*	v_OSErrString - Returns formatted error message based on OS error code */

char *v_OSErrString(errnum)
 int errnum ;
{ static char EMsgBuf[280] ;
  char tbuf[256] ; int l ;

#ifdef WINNT
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,NULL,errnum,0,tbuf,sizeof tbuf,NULL );
	l = strlen(tbuf) - 1 ; for(;l>=0&&tbuf[l]<=0x20;l--) { tbuf[l] = '\0' ; } ;
	sprintf(EMsgBuf,"%d:[%s]",errnum,tbuf) ;
	return(EMsgBuf) ;
#endif
#ifdef VMSOS
	mscu_vms_errmsg(errnum,tbuf) ;
	sprintf(EMsgBuf,"%d:[%s]",errnum,tbuf) ;
	return(EMsgBuf) ;
#endif
#ifdef UNIX
	strcpy(tbuf,strerror(errnum)) ;
	sprintf(EMsgBuf,"%d:[%s]",errnum,tbuf) ;
	return(EMsgBuf) ;
#endif
	sprintf(EMsgBuf,"%d",errnum) ;
	return(EMsgBuf) ;
}

#ifdef WINNT
/*	vcgi_Log - Logs a message to stdout & to dated log file		*/
/*	Call: vcgi_Log( idnum , argstring , arg1 , ... )
	  where idnum is id number (unused - s/b 0),
		argstring is log error string,
		argn are optional format arguments			*/
void vcgi_Log(msg)
 char *msg ;
{ SYSTEMTIME st ;
  FILE *fp ;
  va_list ap ;
  char filename[100],timebuf[50] ;

/*	Do special formatting on error message */
	va_start(ap,msg) ; vsprintf(lbuf,msg,ap) ; va_end(ap) ;
	GetLocalTime(&st) ;
	sprintf(timebuf,"%02d:%02d:%02d",st.wHour,st.wMinute,st.wSecond) ;
//	printf("%s %s\n",timebuf,lbuf) ;
	sprintf(filename,"%s%02d%02d.log",exeName,st.wMonth,st.wDay) ;
	fp = fopen(filename,"a") ;			/* Open up log file */
	if (fp == NULL)
	 { printf("Error (%s) opening log file (%s) for message: %s %s\n",v_OSErrString(errno),filename,timebuf,lbuf) ;
	   exit(0) ;
	 } ;
	if (fp != NULL)
	 { fprintf(fp,"%s (%s) %s\n",timebuf,(peerAddr == NULL ? "?.?.?.?" : peerAddr),lbuf) ;
	   fclose(fp) ;
	 } ;
}
#endif

#ifdef UNIX
void vcgi_Log(va_alist)
  va_dcl
{ 
  struct tm *stm ;
  FILE *fp ;
  va_list args ;
  time_t timer ;
  char *msg,filename[100],timebuf[50] ;
  int tries ;

/*	Do special formatting on error message */
	va_start(args) ; msg = va_arg(args,char *) ; vsprintf(lbuf,msg,args) ; va_end(args) ;
	time(&timer) ; stm = localtime(&timer) ;
	sprintf(timebuf,"%02d:%02d:%02d",stm->tm_hour,stm->tm_min,stm->tm_sec) ;
	sprintf(filename,"%s%02d%02d.log",exeName,st.wMonth,st.wDay) ;
	fp = fopen(filename,"a") ;			/* Open up log file */
	for(tries=0;tries<20;tries++)
	 { fp = fopen(filename,"a") ;			/* Open up log file */
	   if (fp != NULL) break ;			/* File may be busy - wait a few millisecs */
	   HANGLOOSE(25) ;
	 } ;
	if (fp != NULL)
	 { fprintf(fp,"%s (%s) %s\n",timebuf,(peerAddr == NULL ? "?.?.?.?" : peerAddr),lbuf) ; fclose(fp) ; } ;
}
#endif
/*	v_CPUTime - Returns cumulative CPU used by V4 process (and its children) as double */

double v_CPUTime(init)
{ static clock_t startClock ;
  double cpuseconds ;

	if (init) startClock = clock() ;
	cpuseconds = ((clock() -  startClock)*100.0) / (double)CLOCKS_PER_SEC ;
	cpuseconds /= 100.0 ;
	return(cpuseconds) ;
}
