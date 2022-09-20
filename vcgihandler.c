/*	vcgihandler.c - Handles cgi interface from WWW requests	*/
/*	NOTE: This is the base code, copy for client specifics
	  be sure to change SERVERNAME, V4SERVERNAME, & log file name!
	  cl /O2 /Fexvcgi.exe /w vcgihandler.c libc.lib kernel32.lib wsock32.lib advapi32.lib user32.lib
*/

#include <signal.h>
#define NEED_SOCKET_LIBS 1
#include "vconfig.c"
#include <errno.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

#ifdef MACTHINK
#include <console.h>
#endif /* MACTHINK */

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
#include <winsock.h>
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EINPROGRESS WSAEINPROGRESS
#endif

#define CGI_CHECK1RCV TRUE				/* If TRUE then check before recv() of each byte */
#define CGI_CHECK1SEND TRUE				/* If TRUE then check before send() of each byte */
//#define SERVERNAME "cdf/2345"
//#define SERVERNAME "216.178.74.234/2345"		/* Firewall for CDF - links into CDF/2345 */
#define SERVERNAME "192.168.120.146/2345"
#define V4SERVERNAME "192.168.120.146/2345"

//#define TRACE 1

void vcgi_ErrExit() ;
void vcgi_Log() ;
char *v3_logical_decoder() ;
char *v_OSErrString() ;

char **startup_envp ;

main(argc,argv,envp)
  int argc ;			/* Number of arguments passed via DCL */
  char *argv[] ;		/* Argument values */
  char **envp ;			/* Environment pointer */
{ struct sockaddr_in sin ;
  struct hostent *hp ;
  unsigned char hostname[128],rbuf[1024] ; char *bp ;
  int clen,i,j,sres,first,binaryresult ;
  int ConnectSocket,PrimarySocket ;
  fd_set sset ;				/* Socket set */
  struct timeval tev ;
#ifdef WINNT
  OFSTRUCT ofs ;
  WSADATA wsad ;
  int didWSAStartup=FALSE ;
#endif
  char *rm,*cgii,*ct,*v ; char serverport[64] ;
  int inheader ; char lastbyte ;

	startup_envp = envp ;
	setvbuf (stdout, NULL, _IONBF, 0) ;	/* Make sure we don't buffer to stdout */
	strcpy(serverport,SERVERNAME) ;		/* Set up default server/port */

/*	Grab argument(s) from WWW client - may come in several different formats */
	rm = getenv("REQUEST_METHOD") ;
	if (rm == NULL) rm = "GET" ;
	if (strcmp(rm,"GET") == 0)
	 { v = getenv("QUERY_STRING") ; cgii = (char *)malloc(strlen(v)+64) ; strcpy(cgii,v) ;
	   if (cgii == NULL && argc > 0) cgii = argv[1] ;
	   if (cgii == NULL)
	    { vcgi_Log(0,"? No arguments given to program") ;
	      vcgi_ErrExit("v3_www:vcgiInternalErr.htm") ;
	    } ;
	 } else if (strcmp(rm,"POST") == 0)
	 {
/*	   strcasecmp() is not supported in Windows-- use strcmpi() instead */
           if ( strcmp((ct=getenv("CONTENT_TYPE")), "application/x-www-form-urlencoded"))
	    { vcgi_Log(0,"? Unsupported Content-Type (%s)",ct) ; vcgi_ErrExit("v3_www:vcgiInternalErr.htm") ; } ;
           if ( !(clen = atoi(getenv("CONTENT_LENGTH"))) )
	    { vcgi_Log(0,"? No Content-Length was sent with the POST request") ; vcgi_ErrExit("v3_www:vcgiInternalErr.htm") ; } ;
           if ( !(cgii= (char *) malloc(clen+64)) )
	    { vcgi_Log(0,"? Could not malloc(%d) for cgii",clen+1) ; vcgi_ErrExit("v3_www:vcgiInternalErr.htm") ; } ;
           if (!fread(cgii, clen, 1, stdin))
	    { vcgi_Log(0,"? Could not read CGI input from STDIN") ; vcgi_ErrExit("v3_www:vcgiInternalErr.htm") ; } ;
           cgii[clen]='\0' ; cgii[clen+1] = '\0' ; cgii[clen+2] = '\0' ; cgii[clen+3] = '\0' ;
         } else
	 { vcgi_Log(0,"? unsupported CGI Request Method (%s)",rm) ; vcgi_ErrExit("v3_www:vcgiInternalErr.htm") ; } ;
/*	Change all plusses back to spaces & all '&' (arg delmiters) to '\r' for MIDAS server */
	vcgi_Log(0,"Processing request: %s",cgii) ;
	for(i=0,j=0;cgii[i]!='\0'; i++)
	 { 
	   switch(cgii[i])
	    { default:	cgii[j++] = cgii[i] ; break ;
	      case '+':	cgii[j++] = ' ' ; break ;
	      case '&':	cgii[j++] = '\r' ; break ;
	      case '%': cgii[j++] = x2c(&cgii[i+1]) ; i+=2 ; break ;
	    } ;
	 } ;

/*	We must terminate string with ^Z (EOM) */
	cgii[j++] = 26 ; cgii[j] = '\0' ;

/*	Look for "_V4=" string - if found we are going to V4 Server */
	binaryresult = FALSE ;
	v = strstr(cgii,"_V4=") ;
	if (v != NULL)
	 { if (v == cgii ? TRUE : (*(v-1) == '\r'))
	    { strcpy(serverport,V4SERVERNAME) ;
	      if (strstr(cgii,"_V4=XL") != NULL) binaryresult = TRUE ;
	      if (strstr(cgii,"_V4=ResToXL") != NULL) binaryresult = TRUE ;
	    } ;
	 } ;

/*	Make connection to MIDAS server */
#ifdef WINNT
	if (!didWSAStartup)
	 { didWSAStartup = TRUE ;
	   i = MAKEWORD(1,1) ;		/* Always accept highest version */
	   WSAStartup((WORD)i,&wsad) ;
	 } ;
#endif
	if ((PrimarySocket = socket(AF_INET, SOCK_STREAM,0)) < 0)
	 { vcgi_Log(0,"? Error (%s) creating INET Socket (%s)",v_OSErrString(NETERROR),serverport) ;
	   vcgi_ErrExit("v3_www:vcgiNetworkErr.htm") ;
	 } ;
	memset(&sin,0,sizeof sin) ; sin.sin_family = AF_INET ;
/*	Check SERVERNAME for host/port number, if no host then use current host */
	bp = strchr(serverport,'/') ;			/* Look for host delimiter */
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
		 { vcgi_Log(0,"? Error (%s) obtaining network address of host (gethostbyname/addr(%s))",v_OSErrString(NETERROR),hostname) ;
		   vcgi_ErrExit("v3_www:vcgiNetworkErr.htm") ;
		 } ;
	      } ;
	
	if (connect(PrimarySocket,&sin,sizeof sin) < 0)
	 { if (!(NETERROR == EWOULDBLOCK || NETERROR == EINPROGRESS))
	    { vcgi_Log(0,"? Error (%s) connect'ing INET Socket to name (%s)",v_OSErrString(NETERROR),serverport) ;
	      vcgi_ErrExit("v3_www:vcgiNetworkErr.htm") ;
	    } ;
	 } ;
	ConnectSocket = PrimarySocket ;

/*	Send reformatted CGI input back to MIDAS server */
	if (CGI_CHECK1SEND)
	 { FD_ZERO(&sset) ; FD_SET(ConnectSocket,&sset) ;
	   tev.tv_sec = 45 ; tev.tv_usec = 0 ;			/* Set max wait for 45 seconds */
	   sres = select(FD_SETSIZE,NULL,&sset,NULL,&tev) ;	/* Wait for something to happen */
	   if (sres <= 0)
	    { vcgi_Log(0,"? Socket does not appear to be ready to send()") ;
	      vcgi_ErrExit("v3_www:vcgiNetworkErr.htm") ;
	    } ;
	 } ;
	clen = strlen(cgii) ;
	if ((sres = send(ConnectSocket,cgii,clen,0)) < clen)
	 { vcgi_Log(0,"? Error (%s) (len = %d/%d) in send on socket (%s)",v_OSErrString(NETERROR),clen,sres,serverport) ;
	   vcgi_ErrExit("v3_www:vcgiNetworkErr.htm") ;
	 } ;

/*	Suck up results and send back down the line - s/b fully formatted HTML file */
	if (binaryresult)					/* If returning binary file, not HTML then do special */
	 { 
	   if (setmode(fileno(stdout),_O_BINARY) == -1)			/* Binary translation */
	   vcgi_Log(0,"Error (%s) setting binary mode",v_OSErrString(errno)) ;
	    
//	   setvbuf(stdout, NULL, _IONBF, 0 );			/* Turn off buffering */

//	   puts("Content-Disposition: attachment; filename=\"result.xls\"") ;
//	   puts("Content-Type: application/force-download") ;
//	   puts("") ;

	   for(i=0;;i++)
	    { 
	      if (recv(ConnectSocket,rbuf,1,0) <= 0)
	       { vcgi_Log(0,"? Error (%s) in binary recv on socket (%d)",v_OSErrString(NETERROR),serverport) ;
	         break ;
	       } ;
	      if (i == 0)
	       { 
	         puts("Content-Type: application/octet-stream") ;
		 puts("Expires: -1") ;
		 puts("Cache-Control: max-age=1") ;
	         puts("Content-Disposition: attachment; filename=\"result.xls\"") ;
	         puts("") ;
	       } ;
	      putchar(rbuf[0]) ;
	    } ;
	   vcgi_Log(0,"Sent %d binary bytes",i) ;
	   goto close_up ;
	 } ;

	first = TRUE ;						/* First time - wait a little longer for server processing */
	for(i=0;i<sizeof rbuf;i++)
	 { if (CGI_CHECK1RCV)					/* Check before each recv ? */
	    { FD_ZERO(&sset) ; FD_SET(ConnectSocket,&sset) ;
	      tev.tv_sec = (first ? 300 : 30) ;			/* If first time 300 seconds, otherwise 30 */
	      tev.tv_usec = 0 ;					/*  no microseconds */
	      sres = select(FD_SETSIZE,&sset,NULL,NULL,&tev) ;	/* Wait for something to happen */
	      if (sres <= 0)
	       { vcgi_Log(0,"? Timeout waiting for reply from MIDAS server") ;
	         vcgi_ErrExit("v3_www:vcgiNetworkErr.htm") ;
	       } ;
	    } ;
	   if ((clen = recv(ConnectSocket,&rbuf[i],1,0)) <= 0)
	    { vcgi_Log(0,"? Error (%s) in recv on socket (%d)",v_OSErrString(NETERROR),serverport) ;
	      vcgi_ErrExit("v3_www:vcgiNetworkErr.htm") ;
	    } ;
	   if (rbuf[i] == 26) break ;				/* 26 = EOM */
	   if (rbuf[i] < 20)					/* Got control character? */
	    { if (first ? strcmp("<HTML>",rbuf) == 0 || strcmp("<html>",rbuf) == 0 : FALSE)
	       puts("Content-type: text/html\n\n") ;		/* Start out with content type html for Netscape! */
	      rbuf[i+1] = '\0' ; puts(rbuf) ;			/* Dump to stdout - should eventually get back to WWW client */
	      i = -1 ;						/* Start with new rbuf */
	      first = FALSE ;
	    } ;
	 } ;
	if (i > 1) { rbuf[i] = '\0' ; printf(rbuf) ; } ;	/* Flush out any stragglers */

/*	Ready to close up shop */
close_up:
	if (SOCKETCLOSE(PrimarySocket) != 0)
	 { if (serverport == NULL)
	    vcgi_Log(0,"? Error (%s) while attempting close of primary socket (%d) - %s\n",v_OSErrString(NETERROR),PrimarySocket,serverport) ;
	 } ;
	if (PrimarySocket != ConnectSocket)
	 { if (SOCKETCLOSE(ConnectSocket) != 0)
	    vcgi_Log(0,"? Error (%s) in close of connect socket (%d) - %s\n",v_OSErrString(NETERROR),ConnectSocket,serverport) ;
	 } ;

	exit(EXITOK) ;
}

/*	U T I L I T I E S  */

/*	vcgi_ErrExit - Error return: opens file & dumps back (via stdout) to WWW client	*/
/*	Call: vcgi_ErrExit( filename )
	  where filename is name of file to be echoed to stdout
	** NOTE: this routine does NOT return **					*/

void vcgi_ErrExit(char *filename)
{ FILE *fp=NULL ;
  static int nesting = 0 ;
  char buf[1024],*b ;
  int i ;

	if (nesting++ == 0) fp = fopen(v3_logical_decoder(filename,TRUE),"r") ;
	if (fp == NULL)
	 { printf("Error attempting to return error HTML page: %s",filename) ; exit(0) ; }
	for(;;)
	 { if ((b = fgets(buf,sizeof buf,fp)) == NULL) break ;
	   i = strlen(buf) ;
	   if (buf[i-1] = '\n') buf[i-1] = '\0' ;
	   puts(buf) ;
	 } ;
	fclose(fp) ;
	exit(0) ;
}

/*	vcgi_Log - Logs a message to stdout & to dated log file		*/
/*	Call: vcgi_Log( idnum , argstring , arg1 , ... )
	  where idnum is id number (unused - s/b 0),
		argstring is log error string,
		argn are optional format arguments			*/
void vcgi_Log(int idnum)
{ SYSTEMTIME st ;
  FILE *fp ;
  va_list ap ;
  char filename[V_FileName_Max],timebuf[50],lbuf[2048] ; char *msg ;

/*	Do special formatting on error message */
	va_start(ap,idnum) ; msg = va_arg(ap,char *) ; vsprintf(lbuf,msg,ap) ; va_end(ap) ;
	GetLocalTime(&st) ;
	sprintf(timebuf,"%02d:%02d:%02d",st.wHour,st.wMinute,st.wSecond) ;
//	printf("%s %s\n",timebuf,lbuf) ;
	sprintf(filename,"vcgi%02d%02d.log",st.wMonth,st.wDay) ;
	fp = fopen(filename,"a") ;			/* Open up log file */
	if (fp == NULL)
	 { printf("Error (%s) opening log file (%s) for message: %s %s\n",v_OSErrString(errno),filename,timebuf,lbuf) ;
	   exit(0) ;
	 } ;
	if (fp != NULL)
	 { fprintf(fp,"%s %s\n",timebuf,lbuf) ;
	   fclose(fp) ;
	 } ;
}

/*	x2c - Converts two characters to corresponding hex number */
int x2c(char *what)
{ int digit;

	digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A')+10 : (what[0] - '0')) * 16 ;
	digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A')+10 : (what[1] - '0'));
	return(digit);
}

/*	unescape_url -  Reduce any %xx escape sequences to the characters they represent */
void unescape_url(char *url)
{ int i,j;

	for(i=0,j=0;url[j]!=0;++i,++j)
         { if((url[i] = url[j]) == '%')
	    { url[i] = x2c(&url[j+1]) ; j+= 2 ; } ;
	 } ;
	url[i] = '\0' ;
}
/*	Logical Device Decoder */


char *v3_logical_decoderx(cspec,exists,index)
  char *cspec ;
  int exists ;			/* TRUE if looking for existing file, FALSE for new file */
  int index ;			/* If > 0 then return index'th path in logical definitions */
{ static char ffn[200],normspec[200] ;
   char *def,*b,*b1,*ns,*spec ;
   char *comma,*colon ; char firstffn[300],path[500],npath[500] ;
   int i,cnt ; FILE *tf ;

#ifdef VMSOS
	return(cspec) ;						/* VMS does all this for us */
#endif
/*	Convert to lowercase */
#ifdef WINNT
	for((b=normspec,spec=cspec);*spec!=NULL;spec++) { *(b++) = tolower(*spec) ; } ; *b = NULL ;
	ns = strstr(normspec,"  .") ;				/* Any spaces before extension? */
	if (ns != NULL)
	 { for(;*ns == ' ';ns--) {} ;				/* Backup to last non-space */
	   for(i=TRUE,b1=path,b=normspec;;)
	    { if (!i && *b == '.') i = TRUE ;
	      if (i) *(b1++) = *b ; if (*b == '\0') break ;
	      if (b == ns) i = FALSE ; b++ ;
	    } ; strcpy(normspec,path) ;
	 } ;
	spec = normspec ;
	if (*(spec+1) == ':') return(index > 1 ? NULL : spec) ;		/* Don't decode if single letter device: c:xxx */
#endif
#ifdef UNIX
	for((b=normspec,spec=cspec);*spec!=NULL;spec++) { if (*spec > ' ') *(b++) = tolower(*spec) ; } ;
	spec = normspec ; *b = NULL ;
#endif
#ifdef MACTHINK
	spec = cspec ;
#endif
	if (*spec == '$') spec++ ;				/* Get rid of possible leading $ */
	if ((b = (char *)strrchr(spec,';')) != NULL) *b = NULL ;	/* Strip of trailing ";xxx" - VMS generation numbers */
/*	First see if we have a device (xxx:file) */
	if ((colon = (char *)strchr(spec,':')) == 0)
#ifdef UNIX
	 return(spec) ;	/* No device - just return argument */
#endif
#ifdef WINNT
	 return(index > 1 ? NULL : spec) ;	/* No device - just return argument */
#endif
#ifdef MACTHINK		/* On mac- convert "/" to ":" */
	 { if (strchr(spec,'/') == NULL)
	    { normspec[0] = ':' ; normspec[1] = 0 ; strcat(normspec,spec) ; return(normspec) ;
	    } else
	    { for (b=normspec;*spec!=NULL;spec++) { *(b++) = (*spec == '/' ? ':' : *spec) ; } ;
	      *b = NULL ; return(normspec) ;
	    } ;
	 } ;
#endif
/*	Got a colon - lets see if we can decode it */
	strncpy(ffn,spec,colon-spec) ; ffn[colon-spec] = 0 ;	/* Copy up to the colon */
/*	Now zip-zop thru the environment to see we have an path */
	if (!v3_GetEnvValue(ffn,path,sizeof path))
	 { vcgi_Log(0,"? Could not evaluate logical (%s)",cspec) ;
	   vcgi_ErrExit("v3_www:vcgiInternalErr.htm") ;
	 } ;
/*	Loop thru all directories specified */
	firstffn[0] = '\0' ;
	if (index > 0) exists = TRUE ;			/* Set to TRUE to force sequence through logical defs */
	for(cnt=1;;cnt++)
	 { if (strlen(path) == 0)			/* If nothing in path, return current file */
	    { if (index > 0) return(NULL) ;		/* If looking for n'th then return null (end of list) */
	      if (firstffn[0] != '\0') strcpy(ffn,firstffn) ;
	      return(ffn) ;
	    } ;
	   if ((comma = (char *)strchr(path,',')) == 0) /* Look for "," in path */
	    {
	      if (path[strlen(path)-1] == ':')		/* Got a nested logical ? */
	       { path[strlen(path)-1] = 0 ;
	         if (!v3_GetEnvValue(path,ffn,sizeof ffn))
	          { vcgi_Log(0,"? Could not evaluate nested logical (%s)",path) ;
	            vcgi_ErrExit("v3_www:vcgiInternalErr.htm") ;
	          } ;
	       } else { strcpy(ffn,path) ; } ;
#ifdef WINNT
	      if (ffn[strlen(ffn)-1] != '\\') strcat(ffn,"\\") ;
#endif
#ifdef UNIX
	      if (ffn[strlen(ffn)-1] != '/') strcat(ffn,"/") ;
#endif
#ifdef MACTHINK
	      if (ffn[strlen(ffn)-1] != ':') strcat(ffn,":") ;
#endif
	      strcat(ffn,colon+1) ;	/* And remainder of file spec */
#ifdef WINNT
	      i = strlen(ffn) ; if (ffn[i-1] == '\\') ffn[i-1] = 0 ;	/* Don't return with trailing "/" */
#endif
#ifdef UNIX
	      i = strlen(ffn) ; if (ffn[i-1] == '/') ffn[i-1] = 0 ;	/* Don't return with trailing "/" */
#endif
#ifdef MACTHINK
	      i = strlen(ffn) ; if (ffn[i-1] == ':') ffn[i-1] = 0 ;	/* Don't return with trailing ":" */
#endif
	      return(index > cnt ? NULL : ffn) ;
	    } ;
	   strncpy(ffn,path,comma-path) ; ffn[comma-path] = 0 ; strcpy(path,&path[1+comma-path]) ;
	   if (ffn[strlen(ffn)-1] == ':')		/* Got a nested logical ? */
	    { ffn[strlen(ffn)-1] = 0 ;
	      if (!v3_GetEnvValue(ffn,npath,sizeof npath))
	       { vcgi_Log(0,"? Could not evaluate nested logical (%s)",ffn) ;
	         vcgi_ErrExit("v3_www:vcgiInternalErr.htm") ;
	       } ;
	      strcpy(ffn,npath) ;
	    } ;
#ifdef WINNT
	   if (ffn[strlen(ffn)-1] != '\\') strcat(ffn,"\\") ;
#endif
#ifdef UNIX
	   if (ffn[strlen(ffn)-1] != '/') strcat(ffn,"/") ;
#endif
#ifdef MACTHINK
	   if (ffn[strlen(ffn)-1] != ':') strcat(ffn,":") ;
#endif
	   strcat(ffn,colon+1) ;	/* And remainder of file spec */
/*	   Try and read this file to make sure it exists */
	   if (index > 0 && cnt == index) return(ffn) ;
	   if ((tf=fopen(ffn,"r")) != NULL) { fclose(tf) ; return(ffn) ; } ;
	   if (!exists)
	    { if (firstffn[0] == '\0') strcpy(firstffn,ffn) ;	/* Save first path in case we don't find file */
	    } ;
	 } ;
}

char *v3_logical_decoder(cspec,exists)	/* Decodes & returns string to expanded file name */
  char *cspec ;
  int exists ;			/* TRUE if looking for existing file, FALSE for new file */
{
	return(v3_logical_decoderx(cspec,exists,0)) ;
}

v3_GetEnvValue(varname,resbuf,resmax)
  char *varname ;		/* Variable to look-up */
  char *resbuf ;		/* Hold value */
  int resmax ;			/* Max bytes in value */
{
#ifdef WINNT
  extern char **startup_envp ;
  char *b,*def,*dl ;
  int i,j ;
  int *type ; int valstrsize,valbufsize ;
  char valstr[500],valbuf[500], name[500],name1[500] ;
  static HKEY rkey ;

	for(i=0;;i++)
	 { def = startup_envp[i] ;	/* def = pointer to next item in environment */
	   if (def == 0) break ;	/* No def, not found, try registry below */
	   dl = (char *)strchr(def,'=') ; strncpy(name,def,dl-def) ; name[dl-def] = 0 ;
	   for(b=name;*b != 0;b++) { *b = tolower(*b) ; } ;
	   if (strcmp(varname,name) == 0) /* Got a match */
	    { strcpy(resbuf,dl+1) ;		 /* Copy logical into path buffer - may be more than one directory */
	      return(TRUE) ;
	    } ;
	 } ;
	for(i=0;*(varname+i)!=0;i++) { name[i] = tolower(*(varname+i)) ; } ; name[i] = 0 ;
	if (rkey == 0)
	 { RegOpenKeyEx(HKEY_CURRENT_USER,"Environment",NULL,KEY_EXECUTE,&rkey) ; } ;
	for(i=0;;i++)
	 { valstrsize = sizeof valstr ; valbufsize = sizeof valbuf ;
	   if (RegEnumValue(rkey,i,valstr,&valstrsize,NULL,&type,valbuf,&valbufsize) != ERROR_SUCCESS) break ;
	   for(j=0;;j++) { name1[j] = tolower(valstr[j]) ; if (valstr[j] == 0) break ; } ;
	   if (strcmp(name,name1) != 0) continue ;
	   strncpy(resbuf,valbuf,(j = (resmax > valbufsize ? valbufsize : resmax-1))) ;
	   *(resbuf+j) = 0 ;
	   return(TRUE) ;
	 } ;
	return(FALSE) ;
#else
   extern char **startup_envp ;
   int i ; char *b,*def,*dl,name[300] ;

	if ((b = vlog_GetSymValue(varname)) != NULL)
	 { i = strlen(b) ; i = (i > resmax ? resmax-1 : i) ;
	   strncpy(resbuf,b,i) ; *(resbuf+i) = 0 ;
	   return(TRUE) ;
	 } ;
#ifdef MACTHINK
	return(FALSE) ;			/* No startup_envp on MAC */
#endif
	for(i=0;;i++)
	 { def = startup_envp[i] ;	/* def = pointer to next item in environment */
	   if (def == 0) return(FALSE) ;
	   dl = (char *)strchr(def,'=') ; strncpy(name,def,dl-def) ; name[dl-def] = 0 ;
	   for(b=name;*b != 0;b++) { *b = tolower(*b) ; } ;
	   if (strcmp(varname,name) == 0) break ;	/* Got a match */
	 } ;
	strcpy(resbuf,dl+1) ;		 /* Copy logical into path buffer - may be more than one directory */
	return(TRUE) ;
#endif
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
