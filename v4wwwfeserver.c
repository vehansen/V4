/*	V4WWWFEServer - V4 Front-End Server for WWW Interface	*/

#include <signal.h>
#define NEED_SOCKET_LIBS 1
#define UNUSED -1
//#define INHIBIT_WANTMYSQL
#include "v4defs.c"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
//#include <io.h>
#include <fcntl.h>

enum MSGTYPE { unknown, get, post, image, options } ;
enum MSGFILEARGTYPE { none, file, fileAndArgs, args } ;

void vsrv_ParseDefaults(char *,int) ;
void vsrv_SetNextJobNo(COUNTER *, COUNTER*) ;
void v4SrvSubProcess(struct v4fe__SubProcessArguments *) ;
void amIAliveThread(int *) ;
void v4SrvProcError(struct v4fe__SubProcessArguments *,int,int,int,int) ;
void v4SrvRet400Error(struct v4fe__SubProcessArguments *,int,int,int,int,char *) ;
enum MSGFILEARGTYPE v4SrvSubHTTPGet(char *,struct v4fe__SubProcessArguments *) ;
int v4SrvSubHTTPPost(char *,struct v4fe__SubProcessArguments *,int,int,char *) ;
LOGICAL v4SrvReturnFile(char *,struct v4fe__SubProcessArguments *,int,int,int,enum MSGTYPE,int) ;
void v4SrvProcStreamLine(struct v4fe__SubProcessArguments *,int,int,int,char *,char *,char *,struct v4fe__StreamLine *) ;
int x2c(char *) ;
char *logFileName() ;
void Log(int,char *,...) ;
void cLog(int,char *,...) ;
void loggingThread() ;
void endProcess(int) ;
struct V4DPI__LittlePoint protoInt, protoDbl, protoFix, protoDict, protoAlpha, protoLog, protoForeign, protoNone, protoUCChar ;

#define DCHAR '\r'		/* Delimiter character */
#define DSTR "\r"		/* Delimiter character as string */



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
#endif

/*	Ver	Date		Comments
	 5	15-May-08	PMA Support, Restart, KillPattern
	 4	30-May-07	Handles cookies properly, multiple Area commands only result in single V4 Area command
	 6	09-Aug-08	Changed startup logging to output logfile name
	 7	11-Sep-08	Added ability to preface point with '__' for security handling (credit card numbers)
	 8	24-Oct-08	Added AJAX support
	 9	15-Nov-08	Added HTMLMask & ExcelMask options to use v_Msg for command line setup
	 10	25-Apr-09	Beefed up HTTP handling for new xxxCGI that simply passes HTTP instead of converting
	 11	29-Jun-10	Added XLSX support
	 12	29-Aug-10	Added INFO=xxx for CDF migration, default _V4 & _Process
	 13	7-Nov-10	Changed recv() to read in larger chunks
	 14	8-Nov-10	Added _V4=condRestart to exit with different codes
	 15	22-Mar-11	Added Startup option, changed handling of jobno1/2
	 16	02-Nov-11	Added Output startup option
	 17	14-Dec-11	Increased pass-thru buffer size, fixed bug with uninitialized pass-thru file name
	 18	22-Dec-11	Fixed problem with ignoring '\n' instead of DCHAR
	 19	10-Feb-12	Added streamline facility, initialWait parameter, ResponseWait parameter
	 20	19-Mar-12	Added _v4=JSONProcess to go with new xv4rpp -J option
	 21	20-Aug-12	Added MIDAS1 option and v4www_HandleMIDASAction() routine
	 22	25-Apr-13	Added option to put ',xxx' on pre & post eval routines
	 23	17-Jun-13	Added Evaluate startup parameter
	 24	26-Dec-13	Removed _v4={midas1, restov4r, html, spawn, activity} and resformat=xxx and securityvalidate conditional, securityServer, validationMask
	 25	24-Nov-14	Cleaned up parsing of GET command to handle file+arguments
	 26	20-Feb-15	Added Site=xxx startup parameter
	 27	24-Feb-15	Added File=index;name & _File=index to quickly return local files
	 28	23-Mar-15	Fixed issue with $.ajax() and passing multi-parts & with boundaries not explicitly given
	 29	19-May-15	Added _callBack & JSONP support
	 30	12-Jun-15	Got _v4=menu to work, updated xlibmenu.v4 to use it properly
	 31	22-Sep-15	Added extra fclose to v4=amIAlive so that we don't get open file leakage
	 32	05-Oct-15	Added IPAddress parameter to force listen on specific IP address (when using software VPN)
	 33	14-Dec-15	Fixed bug in passthru when returned file has no extension
	 34	29-Mar-16	Increased size of isct[] to handle very large JSON strings
	 35	05-Apr-16	Added v4=JSONJBT to handle straight JSON return
	 36	05-Apr-16	Added EncryptArgs=variableName to handle server encryption of credit card numbers
	 37	02-Jun-16	Increased size of isct to handle huge posts
	 38	14-Nov-16	Changed _v4=argsJSON to send arguments as dim:jbt (used to be dim:vbt)
	 39	12-Sep-17	Changed command buffer for spawn from ASCretUC to local buffer
	 40	20-Sep-17	Changed all spawns of xv4rpp to use new maco to convert arg to UC
				Also increased ARGMax to 256
	 41	13-Nov-17	Changed v4SrvProcError() to return valid HTTP response, added check for JSON return
	 42	18-Jun-18	Fixed bug in return of SL results (embedded quotes)
	 43	18-Jul-18	Added check in GET FILE if we have ?_V4 then assume it's a V4 request
	 44	07-Jul-18	Tweaks to GET FILE to handle various cases
	 45	12-Sep-18	Commented ("//XT" at begin of line) out a bunch of old unused features
	 46	23-Sep-21	Added _work=d to delete work files when job is finished
	 47	27-Oct-21	Added check for missing _Func parameter - return 404 page
	 48	03-May-22	Changed order of special context points & preEval to handle different locales
	 49	17-May-22	Change to move 'Context ADV ajaxMod:xxx' to before preEval
*/

#define V4FE_Version 49
//#define V4LIM_BiggestPositiveInt 0x7fffffff

#define SEND_WAIT_SECONDS 60		/* How long to wait for send channel to clear to next chunk xmit */

#define SERVER_PORT 2340
#define NOSOCKET 0x80000000		/* Indicates 'no socket' connection */

char *v_OSErrString() ;
void Log() ;

#define MAX_AREAS 5				/* Max number of Area's allowed */
#define MAX_STREAMLINED 20			/* Max number of stream-lined streams allowed */
#define MAX_FILES 16
#define INITIAL_WAIT 30				/* Default number of seconds to wait for begin of reply from sub-process */

struct v4fe__SubProcessArguments {		/* All arguments passed to thread via single argument block */
  int ConnectSocket ;
//  char jobId[64] ;
  int jobno1,jobno2 ;
  int initialWait ;
  int honorKeepAlive ;				/* If TRUE then honor keep-alive header requests, if FALSE then ignore */
  char site[64], cgiserver[128], xv4mask[512], xvresmask[512], htmlmask[512], installmask[512], excelmask[512], excel2mask[512], xlsxmask[512], jsonmask[512], ajaxErrPattern[512], ajaxJSONPattern[512] ;
  char includefile1[V_FileName_Max],includefile2[V_FileName_Max], v4prefix[128], startupMsg[512] ;
  char area[MAX_AREAS][256] ;
  char *files[MAX_FILES] ;
  char preeval[128], posteval[128], maineval[1024], output[128] ;
  char *httpMsg ;		/* If not NULL then http 'message' to parse (i.e. don't read from socket, this is injected message) */
  struct v4fe__StreamLine *slList[MAX_STREAMLINED] ;
 } ;
char logdirectory[256] ;	/* Location of log file */

struct v4fe__StreamLine {
  UCCHAR slName[64] ;			/* Stream-line name */
  UCCHAR urlHost[V_FileName_Max] ;	/* URL to host server */
  UCCHAR cmdLineRestart[512] ;		/* Command line to restart server if it is not listening */
  int portHost ;			/* Port number to communicate on to host server */
} ;

#define SESKEYINDEXMAPMAX 16	/* Allow up to 16 sessions with different set indexes */
struct v4fe__sesKey2IndexMap {
  INDEX count ;			/* Number in use below */
  struct {
    SESKEY sesKey ;		/* Session key for this entry */
    INDEX setX ;		/* 'Set' index for this session */
   } entry[SESKEYINDEXMAPMAX] ;
} ;
struct v4fe__sesKey2IndexMap *skim = NULL ;

#define DFLT_HEARTBEAT_WAIT 300	/* Seconds to wait for idle heart beat status */
#define EOM 26

char inikeylist[][16] =
 { "HEARTBEAT", "PORT", "SITE", "FILE", "XV4MASK", "XVRESMASK", "XLMASK", "CGISERVER",
   "LOGDIRECTORY", "INCLUDE1", "INCLUDE2", "V4PREFIX", "AREA1", "AREA2", "AREA3", "AREA4", "AREA5", 
   "PREEVAL", "POSTEVAL", "EVALUATE", "RESTART", "KILLPATTERN", "SEQUENCESUFFIX", "ALIVEPATTERN", "AMIALIVE",
   "VERBOSE", "BACKLOG", "AJAXERRORS", "AJAXJSON", "HTMLMASK", "EXCELMASK", "JOBID", "INSTALLMASK", "DEBUG",
   "XLSXMASK", "STARTUP", "OUTPUT", "STREAMLINE", "RESPONSEWAIT", "JSONMASK", "IPADDRESS", "ENCRYPTARGS", "KEEPALIVE", ""
 } ;

#ifdef WINNT
UCCHAR **startup_envp ;
#else
char **startup_envp ;
#endif

#ifdef WINNT
  CRITICAL_SECTION CS ;
  int initCS = FALSE ;
#endif

char killPattern[64], ruAlivePattern[64], wfSequenceSuffix[16], encryptArgs[64] ;
int restartHHMM=UNUSED, restartDayOfMonth=0 ; int amIAliveInterval=UNUSED ;
int heartbeat, logVerbose ;
B64INT xorEncryptValue ;
LOGICAL isDebug = FALSE ;
unsigned short port ;
#define ARGMax 256
#define ARGSETMAX 5
   struct v4fe__SubProcessArguments v4feargs[ARGMax],*ap ; int apcnt=0 ;
   struct v4fe__SubProcessArguments v4ArgSets[ARGSETMAX] ; INDEX argSetX ;

#define LINESTOKEEP 0x400
#define LINESINDEXMASK 0x3FF
#define MAXLINELEN 512
char lineBuf[LINESTOKEEP][MAXLINELEN+1] ; static int curX=0 ;

struct in_addr localIPAddress4 ;

#define	DOPREEVAL \
 if (strlen(v4fearg->preeval) > 0) \
  { char *pep, peFunc[sizeof v4fearg->preeval], peErr[sizeof v4fearg->preeval] ; \
    strcpy(peFunc,v4fearg->preeval) ;		/* func = what to evaluate in post-eval */ \
    pep = strchr(peFunc,',') ;			/* If has ',xxx' then func is just up to the comma */ \
    if (pep == NULL) { peErr[0] = '\0' ; }	/*  and err is the ',xxx' */ \
     else { strcpy(peErr,pep) ; *pep = '\0' ; } ; \
    fprintf(fp,"Eval [%s \"%s,%s\"]%s > %s\n",peFunc,func,jobId,peErr,v4rfile) ; \
    if (strlen(v4fearg->maineval) == 0) fprintf(fp,"If1 Errors Exit File\n") ; \
  } ;


#define	DOPOSTEVAL \
 if (strlen(v4fearg->posteval) > 0) \
  { char *pep, peFunc[sizeof v4fearg->posteval], peErr[sizeof v4fearg->posteval] ; \
    strcpy(peFunc,v4fearg->posteval) ;		/* func = what to evaluate in post-eval */ \
    pep = strchr(peFunc,',') ;			/* If has ',xxx' then func is just up to the comma */ \
    if (pep == NULL) { peErr[0] = '\0' ; }	/*  and err is the ',xxx' */ \
     else { strcpy(peErr,pep) ; *pep = '\0' ; } ; \
    fprintf(fp,"Eval [%s \"%s,%s\"]%s\n",peFunc,func,jobId,peErr) ; \
  } ;



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
{ fd_set sset ;
  struct timeval tev ;
   struct hostent *hp ;
   struct sockaddr_in sin,fsin ;
   int PrimarySocket,ConnectSocket ;
   char hostname[128] ;
   FILE *fp ; char tbuf[1024] ; char *b,*b1,*b2 ; int len,line ;
   int heatbeat, listenBacklog ;
   COUNTER jobno1, jobno2 ;
#ifdef WINNT
  SYSTEMTIME st ;
  int didWSAStartup = FALSE ;
  OFSTRUCT ofs ;
  WSADATA wsad ;
  DWORD dwThreadId ;
#endif
#define UCARGVMAX 50
  static UCCHAR *ucargv[UCARGVMAX] ;		/* Temp (hopefully) structure converting command line to Unicode */
  int i,err ;

	startup_envp = envp ;
	v_GetProcessInfo() ;		/* Have to call this to init vcommon stuff */
	setbuf(stdout,NULL) ;		/* Turn off output buffering */
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

#define SCOPY(DST,SRC) \
{ int _max = sizeof DST - 1 ; char *_d=DST, *_s=SRC ; \
if (_max < 4) printf("???????? %s\n",SRC) ;\
  for(;_max>0&&*_s !='\0';_max--) { *(_d++) = *(_s++) ; } ; \
  *_d = '\0' ; \
}
#define SCAT(DST,SRC) \
{ char *_d=DST, *_s=SRC ; int _max = sizeof DST - 1 ; \
  for(;_max>0&&*_d!='\0';_d++) { _max-- ; } ; \
  for(;_max>0&&*_s !='\0';_max--) { *(_d++) = *(_s++) ; } ; \
  *_d = '\0' ; \
}

/*	Set up some defaults */
	memset(&v4ArgSets,0,sizeof v4ArgSets) ; argSetX = 0 ;
	for(i=0;i<ARGMax;i++) { v4feargs[i].ConnectSocket = UNUSED ; } ;
	SCOPY(v4ArgSets[argSetX].xv4mask,"cmd /C \"xv4 -X %s\" > %s") ;
	v4ArgSets[argSetX].initialWait = INITIAL_WAIT ;
	v4ArgSets[argSetX].honorKeepAlive = TRUE ;
	SCOPY(v4ArgSets[argSetX].v4prefix,"v4") ; killPattern[0] = '\1' ; killPattern[1] = '\0' ; ZS(wfSequenceSuffix) ; ruAlivePattern[0] = '\1' ; ruAlivePattern[1] = '\0' ;
	port = SERVER_PORT ; heartbeat = DFLT_HEARTBEAT_WAIT ; SCOPY(logdirectory,".\\") ; logVerbose = FALSE ; listenBacklog = 10 ; ZS(encryptArgs) ; xorEncryptValue = 0 ;
	localIPAddress4.s_addr = UNUSED ;

	CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)loggingThread,NULL,0,NULL) ;

/*	Get name of initialization file & parse results */
	if (argc > 1)
	 { fp = UCfopen(argv[1],"r") ;
	   if (fp == NULL)
	    { Log(0,"? Error (%s) accessing initialization file (%s)\n",v_OSErrString(errno),argv[1]) ; endProcess(EXITABORT) ; } ;
	   for(line=1;;line++)
	    { if (fgets(tbuf,sizeof tbuf,fp) == NULL) break ;
	      if (tbuf[0] == '!') continue ; if (tbuf[0] == '/') continue ;
/*	      Look for [x] or [x=y] */
	      if (tbuf[0] == '[')		/* Are we starting another parameter set? */
	       { char *eb ; argSetX = strtol(&tbuf[1],&eb,10) ;
		 if (argSetX < 1 || argSetX > ARGSETMAX || !(*eb == '\0' || *eb == ']' || *eb == ' ' || *eb == '='))
		  { Log(0,"? Initialization file line %d: Invalid argument set specification (%s)",line,tbuf) ; endProcess(EXITABORT) ; } ;
		 argSetX-- ;			/* Convert to 0-based index */
		 if (*eb == '=')		/* Want to init with another set */
		  { INDEX initX = strtol(&eb[1],&eb,10) ;
		    if (initX < 1 || initX > ARGSETMAX || !(*eb == '\0' || *eb == ']' || *eb == ' ' || *eb == '='))
		     { Log(0,"? Initialization file line %d: Invalid argument set specification (%s)",line,tbuf) ; endProcess(EXITABORT) ; } ;
		    v4ArgSets[argSetX] = v4ArgSets[initX-1] ;
		  } ;
		 continue ;
	       } ;
	      len = strlen(tbuf)-1 ; if (tbuf[len] < 20) tbuf[len] = '\0' ;
	      b = strchr(tbuf,'=') ;		/* Parse keyword=value */
	      if (b == NULL) { Log(0,"? Syntax error, ignoring... - %s\n",tbuf) ; continue ; } ;
	      *b = '\0' ; b++ ;
	      for(b1=tbuf;*b1!='\0';b1++) { *b1 = toupper(*b1) ; } ;
	      len = strlen(tbuf) ;
	      if (len < 3) { Log(0,"? Initialization file line %d: keyword (%s) must be at least 3 characters",line,tbuf) ; endProcess(EXITABORT) ; } ;
	      for(i=0;strlen(inikeylist[i])>0;i++)
	       { if (strncmp(inikeylist[i],tbuf,len) != 0) continue ;
	         if (strncmp(inikeylist[i+1],tbuf,len) != 0) break ;
		 Log(0,"? Initialization file line %d: keyword (%s) is ambiguous (%s vs. %s)",line,tbuf,inikeylist[i],inikeylist[i+1]) ; endProcess(EXITABORT) ;
	       }
	      if (strlen(inikeylist[i]) == 0)
	       { Log(0,"? Initialization file line %d: invalid keyword(%s)",line,tbuf) ; endProcess(EXITABORT) ; } ;
#define VMSGMASK(DST,SRC)  \
 { char *d=DST,*s=SRC ; \
   *(d++) = '@' ; \
   for(;;s++) { *(d++) = *s ; if (*s == '\0') break ; if (*s == '\\') *(d++) = '\\' ; } ; \
 }
	      switch (i+1)
	       { case 1:	/* HeartBeat */		heartbeat = atoi(b) ; break ;
	         case 2:	/* Port */		port = atoi(b) ; break ;
	         case 3:	/* Site */		SCOPY(v4ArgSets[argSetX].site,b) ; break ;
	         case 4:	/* File=num;name */	{ INDEX i ; char *e ;
							  i = strtol(b,&e,10) ; if (*e != ';') { Log(0,"? Initialization file line %d: Expecting File=index;filename",line) ; endProcess(EXITABORT) ; } ;
							  if (i < 1 || i > MAX_FILES) { Log(0,"? Initialization file line %d: File index must be 1 through %d",line,MAX_FILES) ; endProcess(EXITABORT) ; } ;
							  e++ ; if (strlen(e) >= V_FileName_Max) { Log(0,"? Initialization file line %d: File name too long",line) ; endProcess(EXITABORT) ; } ;
							  v4ArgSets[argSetX].files[i-1] = v4mm_AllocChunk(strlen(e)+1,FALSE) ; strcpy(v4ArgSets[argSetX].files[i-1],e) ;
							} break ;
	         case 5:	/* xv4Mask */		SCOPY(v4ArgSets[argSetX].xv4mask,b) ; break ;
	         case 6:	/* xvResMask */		SCOPY(v4ArgSets[argSetX].xvresmask,b) ; break ;
	         case 7:	/* XLMask */		SCOPY(v4ArgSets[argSetX].excelmask,b) ; break ;
		 case 8:	/* CGIServer */		SCOPY(v4ArgSets[argSetX].cgiserver,b) ; break ;
		 case 9:	/* LogDirectory */	SCOPY(logdirectory,b) ; break ;
		 case 10:	/* Include1 file */	SCOPY(v4ArgSets[argSetX].includefile1,b) ; break ;
		 case 11:	/* Include2 file */	SCOPY(v4ArgSets[argSetX].includefile2,b) ; break ;
		 case 12:	/* v4Prefix xxx */	SCOPY(v4ArgSets[argSetX].v4prefix,b) ; break ;
		 case 13:	/* Area1 */		SCOPY(v4ArgSets[argSetX].area[0],b) ; break ;
		 case 14:	/* Area2 */		SCOPY(v4ArgSets[argSetX].area[1],b) ; break ;
		 case 15:	/* Area3 */		SCOPY(v4ArgSets[argSetX].area[2],b) ; break ;
		 case 16:	/* Area4 */		SCOPY(v4ArgSets[argSetX].area[3],b) ; break ;
		 case 17:	/* Area5 */		SCOPY(v4ArgSets[argSetX].area[4],b) ; break ;
		 case 18:	/* PreEval */		SCOPY(v4ArgSets[argSetX].preeval,b) ; break ;
		 case 19:	/* PostEval */		SCOPY(v4ArgSets[argSetX].posteval,b) ; break ;
		 case 20:	/* Evaluate */		strcpy(v4ArgSets[argSetX].maineval,"@") ; strcat(v4ArgSets[argSetX].maineval,b) ; break ;
	         case 21:	/* Restart */		restartHHMM = atoi(b) ; break ;
	         case 22:	/* KillPattern */	SCOPY(killPattern,b) ; break ;
	         case 23:	/* SequenceSuffix */	SCOPY(wfSequenceSuffix,b) ; break ;
	         case 24:	/* AlivePattern */	SCOPY(ruAlivePattern,b) ; break ;
	         case 25:	/* AmIAlive */		amIAliveInterval = atoi(b) ; break ;
	         case 26:	/* Verbose */		logVerbose = atoi(b) ; break ;
	         case 27:	/* Backlog */		listenBacklog = atoi(b) ; break ;
	         case 28:	/* AJAXERRORS */	SCOPY(v4ArgSets[argSetX].ajaxErrPattern,b) ; break ;
	         case 29:	/* AJAXJSON */		SCOPY(v4ArgSets[argSetX].ajaxJSONPattern,b) ; break ;
	         case 30:	/* HTMLMask */		VMSGMASK(v4ArgSets[argSetX].htmlmask,b) ; break ;
	         case 31:	/* ExcelMask */		VMSGMASK(v4ArgSets[argSetX].excel2mask,b) ; break ;
	         case 33:	/* InstallMask */	VMSGMASK(v4ArgSets[argSetX].installmask,b) ; break ;
	         case 34:	/* Debug 1(yes)/0(no) */isDebug = atoi(b) ; break ;
	         case 35:	/* XLSXMask */		VMSGMASK(v4ArgSets[argSetX].xlsxmask,b) ; break ;
		 case 36:	/* Startup */		SCOPY(v4ArgSets[argSetX].startupMsg,b) ; break ;
		 case 37:	/* Output mode */	SCOPY(v4ArgSets[argSetX].output,b) ; break ;
		 case 38:	/* StreamLine=name;host:port;startup */
							{ INDEX i ; char *s, *e, *e1 ;
							  struct v4fe__StreamLine *vfsl ;
							  for(i=0;i<MAX_STREAMLINED;i++) { if (v4ArgSets[argSetX].slList[i] == NULL) break ; } ;
							  if (i >= MAX_STREAMLINED) goto slFail ;
							  v4ArgSets[argSetX].slList[i] = (vfsl = v4mm_AllocChunk(sizeof *vfsl,TRUE)) ;
							  e = strchr(b,';') ; if (e == NULL) goto slFail ;
							  *e = '\0' ; UCstrcatToUC(vfsl->slName,b) ; b = e+1 ;
							  e = strchr(b,';') ; if (e == NULL) {  } ;
							  *e = '\0' ; e1 = strchr(b,':') ; if (e1 == NULL) goto slFail ;
							  *e1 = '\0' ; UCstrcatToUC(vfsl->urlHost,b) ; b = e1+1 ; vfsl->portHost = strtol(b,&b,10) ; if (*b != '\0') goto slFail ; b = e+1 ;
							  UCstrcatToUC(vfsl->cmdLineRestart,b) ;
							  break ;
						slFail:	  Log(0,"? Initialization file line %d: Expecting Streamline=name;host:port;startup",line) ; endProcess(EXITABORT) ;
							} break ;
		 case 39:	/* ResponseWait secs */	v4ArgSets[argSetX].initialWait = atoi(b) ;
							if (v4ArgSets[argSetX].initialWait < 1 || v4ArgSets[argSetX].initialWait > 300)
							 { Log(0,"[Initialization file line %d: ResponseWait=%d out of range, resetting to default of %d",line,v4ArgSets[argSetX].initialWait,INITIAL_WAIT) ;
							   v4ArgSets[argSetX].initialWait = INITIAL_WAIT ;
							 } ;
							break ;
	         case 40:	/* JSONMask */		VMSGMASK(v4ArgSets[argSetX].jsonmask,b) ; break ;
		 case 41:	/* IPAddress */		localIPAddress4.s_addr = inet_addr(b) ; strcpy(hostname,"*set from .ini file*") ; break ;
		 case 42:	/* EncryptArgs=variable-name */
							SCOPY(encryptArgs,b) ; xorEncryptValue = vRan64_RandomU64() ;
							sprintf(tbuf,"%I64d",xorEncryptValue) ; SetEnvironmentVariable(encryptArgs,tbuf) ;
							break ;
		 case 43:	/* KEEPALIVE=1/0 */	v4ArgSets[argSetX].honorKeepAlive = atoi(b) ; break ;
	       } ;
	    } ;
	   fclose(fp) ;
	 } else
	 { Log(0,"? No initialization file specified\n") ; endProcess(EXITABORT) ; } ;

/*	Make sure log directory ends with proper terminator */
	i = strlen(logdirectory) ;
#ifdef WINNT
	if (logdirectory[i-1] != '\\') SCAT(logdirectory,"\\") ;
#endif

	Log(0,"Startup (v%d): Heartbeat %d, Port %d, listen() backlog %d, log file is %s",V4FE_Version,heartbeat,port,listenBacklog,logFileName()) ;

#ifdef LOCALTEST
	   v4SrvSubProcess(&v4fearg) ;
	   endProcess(EXITOK) ;
#endif

#ifdef WINNT
	if (!didWSAStartup)
	 { didWSAStartup = TRUE ;
//	   i = MAKEWORD(1,1) ;		/* Always accept highest version */
	   WSAStartup(MAKEWORD(1,1),&wsad) ;
	 } ;
	if (restartHHMM != UNUSED)	/* Don't want to restart today if already past restart time */
	 { GetLocalTime(&st) ;
	   if(((st.wHour * 100) + st.wMinute) > restartHHMM)
	    { Log(0,"Current time past scheduled restart, will not restart until tomorrow") ;
	      restartDayOfMonth = st.wDay ;
	    } else { Log(0,"Scheduled restart for %d",restartHHMM) ; } ;
	 } ;
#endif

/*	Maybe start up thread to periodically check to see if still alive */
	if (amIAliveInterval != UNUSED)
	 { Log(0,"Creating Am-I-Alive thread with interval of %d seconds",amIAliveInterval) ;
	   CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)amIAliveThread,&amIAliveInterval,0,&dwThreadId) ;
	 } ;



	if ((PrimarySocket = socket(AF_INET, SOCK_STREAM,0)) < 0)
	 { Log(0,"? Error (%s) creating INET Socket",v_OSErrString(errno)) ; endProcess(EXITABORT) ;
	 } ;
	i = 1 ;
	if (NETIOCTL(PrimarySocket,FIONBIO,&i) != 0)
	 { Log(0,"? Could not set socket to NOBLOCK") ; endProcess(EXITABORT) ;
	 } ;
	memset(&sin,0,sizeof sin) ; sin.sin_family = AF_INET ; sin.sin_port = htons(port) ;
	if (localIPAddress4.s_addr == UNUSED)
	 { gethostname(hostname,sizeof(hostname)) ;		/* Get host name */
	   hp = gethostbyname(hostname) ;
#define MASKBYTE(b) (((int)hp->h_addr[b])&0xff)
	   Log(0,"Local hostname is %s (%d.%d.%d.%d)\n",hostname,MASKBYTE(0),MASKBYTE(1),MASKBYTE(2),MASKBYTE(3));
	   if (hp != NULL)
	    { memcpy(&localIPAddress4.s_addr,hp->h_addr,hp->h_length) ; }
	 } ;
	if (localIPAddress4.s_addr == UNUSED)
	 { Log(0,"? Error (%s) obtaining network address of host (gethostbyname/addr(%s))",v_OSErrString(errno),hostname) ; endProcess(EXITABORT) ; } ;
	sin.sin_addr.s_addr = localIPAddress4.s_addr ;
	if (bind(PrimarySocket,(const struct sockaddr *)&sin,sizeof sin) < 0)
	 { Log(0,"? Error (%s) bind'ing INET Socket to name (%s)",v_OSErrString(NETERROR),hostname) ; endProcess(EXITABORT) ; } ;
	if ((err = listen(PrimarySocket,listenBacklog)) < 0)
	 { Log(0,"? Error (%s) listen'ing on INET Socket (%s)",v_OSErrString(NETERROR),hostname) ; endProcess(EXITABORT) ; } ;

cLog(FALSE,"Listen result = %d\n",err) ;
	Log(0,"Listening on IP address %s port %d",inet_ntoa(localIPAddress4),port) ;
	argSetX = 0 ;

/*	Do we have a job to be run every time the server starts up? */
	if (v4ArgSets[argSetX].startupMsg[0] != '\0')
	 { vsrv_SetNextJobNo(&jobno1,&jobno2) ;
	   v4ArgSets[argSetX].ConnectSocket = NOSOCKET ;
/*	   Copy arguments into "new" spot to pass to thread */
	   for(i=0;i<ARGMax*2;i++)
	    { apcnt++ ; ap = &v4feargs[apcnt % ARGMax] ; if (ap->ConnectSocket == UNUSED) break ;
	    } ;
	   *ap = v4ArgSets[argSetX] ; ; ap->jobno1 = jobno1 ; ap->jobno2 = jobno2 ;
	   ap->httpMsg = ap->startupMsg ;
	   for(b=ap->httpMsg;;b++)		/* Replace all '&' with DCHAR */
	    { b = strchr(b,'&') ; if (b == NULL) break ;
	      *b = DCHAR ;
	    } ;
	   if (strchr(ap->httpMsg,EOM) == NULL)
	    { b = strchr(ap->httpMsg,'\0') ; *b = EOM ; *(b+1) = '\0' ; } ;
	   CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)v4SrvSubProcess,ap,0,&dwThreadId) ;
	   Log(jobno2,"Created startup job thread arg(%x)",ap) ;
	   jobno2++ ;				/* Increment to next available job number */
	 } ;

	for(;;jobno2++)
	 { Log(0,"Waiting on accept") ;
	   for(;;)
	    {
	      FD_ZERO(&sset) ; FD_SET(PrimarySocket,&sset) ;
	      tev.tv_sec = heartbeat ; tev.tv_usec = 0 ;
	      i = select(1,&sset,NULL,NULL,&tev) ;	/* Wait for something! */
	      if (i > 0) break ;
	      if (restartHHMM != UNUSED)
	       { GetLocalTime(&st) ;
	         if (restartDayOfMonth != st.wDay && ((st.wHour * 100) + st.wMinute) > restartHHMM)
	          { Log(0,"Scheduled daily shutdown: %d",restartHHMM) ; goto shutdown ; } ;
	       } ;
	      Log(0,"idle heartbeat") ;
	    } ;
	   i = sizeof fsin ;
	   if ((ConnectSocket = accept(PrimarySocket,(struct sockaddr *)&fsin,&i)) < 0)
	    { Log(0,"?Error (%s) accept'ing on INET Socket (%s)",v_OSErrString(NETERROR),hostname) ; endProcess(EXITABORT) ; } ;

	   vsrv_SetNextJobNo(&jobno1,&jobno2) ;

#ifdef WINNT
	   v4ArgSets[argSetX].ConnectSocket = ConnectSocket ;
/*	   Copy arguments into "new" spot to pass to thread */
	   for(i=0;i<ARGMax*2;i++)
	    { apcnt++ ; ap = &v4feargs[apcnt % ARGMax] ; if (ap->ConnectSocket == UNUSED) break ;
	    } ;
	   if (i >= ARGMax*2)
	    { SOCKETCLOSE(ConnectSocket) ;
	      Log(jobno2,"Insufficient server thread slots to complete job") ; continue ;
	    } ;
	   *ap = v4ArgSets[argSetX] ; ; ap->jobno1 = jobno1 ; ap->jobno2 = jobno2 ;
//if (jobno2 & 1) Sleep(1000) ;	   
	   { HANDLE tHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)v4SrvSubProcess,ap,0,&dwThreadId) ;
	     CloseHandle(tHandle) ;
	   } ;
	   Log(jobno2,"Created thread arg(%x) socket(%d)",ap,ConnectSocket) ;
#endif
	 } ;

shutdown:
	SOCKETCLOSE(PrimarySocket) ;
	for(i=0;i<10;i++)
	 { WSACleanup() ; err = WSAGetLastError() ;
	   if (err == WSANOTINITIALISED) break ;
	   if (err == WSAEINPROGRESS) WSACancelBlockingCall() ;
	 } ;
	endProcess(EXITOK) ;
}


void vsrv_SetNextJobNo(jobno1,jobno2)
  COUNTER *jobno1, *jobno2 ;
{
  static COUNTER oldjobno1 = UNUSED ;
  char logBuf[2048],*b,*end ; FILE *fp ; int seq ;
#ifdef WINNT
  SYSTEMTIME st ;
#endif
#ifdef WINNT
	GetLocalTime(&st) ;				/* Check for new day */
	*jobno1 = st.wYear * 10000 + st.wMonth * 100 + st.wDay ;
#endif

	if (oldjobno1 == *jobno1) return ;

	oldjobno1 = *jobno1 ;
	Log(0,"Begin of New Day") ;
	fp = fopen(logFileName(),"r") ;		/* Try to open today's log file to get last sequence number */
	switch (wfSequenceSuffix[0])
	 { default:	*jobno2 = 1 ; break ;
	   case 'a':	*jobno2 = 1000001 ; break ;
	   case 'b':	*jobno2 = 2000001 ; break ;
	   case 'c':	*jobno2 = 3000001 ; break ;
	   case 'd':	*jobno2 = 4000001 ; break ;
	   case 'e':	*jobno2 = 5000001 ; break ;
	 } ;
	if (fp != NULL)
	 { for(;;)
	    { if (fgets(logBuf,sizeof logBuf,fp) == NULL) break ;
	      if (strstr(logBuf,"Begin of Message") == NULL) continue ;
	      b = strchr(logBuf,':') ; if (b == NULL) continue ;	/* Look for 'hh:' */
	      b = strchr(b+1,':') ; if (b == NULL) continue ;	/* Look for 'mm:' */
	      if (b[3] != '-') continue ;				/* Look for 'ss-' */
	      seq = strtol(&b[4],&end,10) ;				/* Parse the sss after ':ss-' */
	      if (*end != ' ') continue ;
	      if (seq >= *jobno2) *jobno2 = seq + 1 ;
	    } ; fclose(fp) ;
	 } ;
	if (*jobno2 > 1)
	 Log(0,"Restart detected (%s), continuing with sequence %d - %d",logFileName(),*jobno1,*jobno2) ;
}

#define doMenu 1
#define doProcess 2
#define doXLProcess 4			/* Same as doProcess but return binary excel (biff) file */
#define doResToXL 5			/* Convert previous .v4r to XL file without regenerating */
#define doFilePassThru 7		/* Pass a file through "as is" */
#define doRestart 10			/* Force restart of this server */
#define doIAmAlive 11			/* Return I-Am-Alive Message */
#define doActivity 12			/* Return current activity */
#define doAjax 13			/* Return AJAX */
#define doInstall 14			/* Circumvent normal V4 processing so that new v4a's can be installed */
#define doResToXLSX 15			/* Convert previous V4R to XLSX file without regenerating */
#define doCondRestart 16		/* Do conditional restart - subprocess results is exit code to be used (see EXITxxx in vconfig.c) */
#define doSetIndex 17			/* Ask application if OK to switch startup parameter sets (from default of 1/0) */
#define doSL 18				/* Do Stream-line #1 processing */
#define doJSONProcess 19		/* Same as doProcess but call xv4rpp to create JSON instead of html/excel */
#define doJSONJBT 20			/* Same as doProcess but result is JSON to be passed back as Context ADV jbt:xxxxx */

#define VFE_TableMax 50			/* Max number of tables */
#define VFE_NameMax 32			/* Max length of column name */
#define VFE_ColumnMax 250		/* Max number of columns we support */

#define INITIALMSGBYTES 0x80000

#define DEFAULT_MAX 50
struct lcl__Defaults {
  int Count ;				/* Number of defaults below */
  char Dimbuf[32] ;			/* Temp buffer for "current" dimension */
  struct {
    int IgnoreIfBlank ;			/* If TRUE then dimension below is not to be passed to V4 if value is blank */
    char Dimname[32] ;			/* Dimension name (UPPERCASE) */
    char Value[128] ;			/* Default value: Dim:value */
   } Entry[DEFAULT_MAX] ;
} ; struct lcl__Defaults ldef ;


void vsrv_ParseDefaults(dstr,iib)
  char *dstr ; int iib ;
{
  char *str,*b1 ; int i ;

	str = dstr ;
	if (iib)			/* Ignore if blank list ? */
	 { for(;ldef.Count<DEFAULT_MAX;)
	    { b1 = strchr(str,' ') ; if (b1 == NULL) b1 = strchr(str,',') ; if (b1 != NULL) *b1 = '\0' ;
	      for(i=0;;i++) { ldef.Entry[ldef.Count].Dimname[i] = toupper(str[i]) ; if (str[i] == '\0') break ; } ;
	      ldef.Entry[ldef.Count].IgnoreIfBlank = TRUE ;
	      ldef.Count++ ;
	      if (b1 == NULL) break ;
	      str = b1 + 1 ;
	    } ;
	   return ;
	 } ;
	for(;;)
	 { b1 = strchr(str,':') ;	/* Look for next dimension terminator */
	   if (b1 == NULL) break ;
	   if (ldef.Count >= DEFAULT_MAX) break ;
	   for(i=0;;i++) { ldef.Entry[ldef.Count].Dimname[i] = toupper(str[i]) ; if (str[i] == ':') break ; } ;
	   ldef.Entry[ldef.Count].Dimname[i] = '\0' ;
	   if (b1[1] == '"')		/* Starts with quote - advance to ending quote */
	    { for(i=2;;i++)
	       { if (b1[i] == '\0') break ;	/* Hit end of string without ending quote? */
	         if (b1[i] == '"') break ;	/* Hit end of string */
		 if (b1[i] == '\\' && b1[i+1] == '"') i++ ;	/* Skip over embedded quote */
	       } ; b1[i+1] = '\0' ; b1 = &b1[i+1] ;
	    } else
	    { b1 = strchr(&b1[1],' ') ;	/* Advance to next space (or end of string) */
	      if (b1 != NULL) *b1 = '\0' ;
	    } ;
	   if (strlen(str) < sizeof ldef.Entry[0].Value) SCOPY(ldef.Entry[ldef.Count].Value,str) ;
	   str = (b1 == NULL ? "" : b1+1) ;
	   ldef.Entry[ldef.Count].IgnoreIfBlank = FALSE ;
	   ldef.Count++ ;
	 } ;
}

void amIAliveThread(intSeconds)
  int *intSeconds ;
{ fd_set sset ;
  struct timeval tev ;
  struct hostent *hp ;
  struct sockaddr_in sin ;
  int aiaSocket, mlen ;
  char msg[256],hostname[128] ;
  
	for(;;)
	 { HANGLOOSE(*intSeconds * 1000) ;
	   if ((aiaSocket = socket(AF_INET, SOCK_STREAM,0)) < 0) goto fail ;
	   memset(&sin,0,sizeof sin) ; sin.sin_family = AF_INET ;  sin.sin_port = htons(port) ;
	   gethostname(hostname,sizeof(hostname)) ;		/* Get host name */
	   hp = gethostbyname(hostname) ;
	   if (hp != NULL) { memcpy(&sin.sin_addr,hp->h_addr,hp->h_length) ; }
	    else { sin.sin_addr.s_addr = inet_addr(hostname) ;
		   if (sin.sin_addr.s_addr == -1) goto fail ;
	         } ;	
	   if (connect(aiaSocket,(struct sockaddr *)&sin,sizeof sin) < 0)
	    { if (!(NETERROR == EWOULDBLOCK || NETERROR == EINPROGRESS)) goto fail ; } ;
	   sprintf(msg,"_V4=%s" DSTR "%c",ruAlivePattern,EOM) ; mlen = strlen(msg) ;
	   if (send(aiaSocket,msg,mlen,0) < mlen) goto fail ;
	   for(;;)
	    { 
	      FD_ZERO(&sset) ; FD_SET(aiaSocket,&sset) ;
	      tev.tv_sec = 2 ; tev.tv_usec = 0 ;
	      if (select(FD_SETSIZE,&sset,NULL,NULL,&tev) <= 0) goto fail ;
	      if (recv(aiaSocket,msg,1,0) < 1) break ;
	      if (msg[0] == EOM) break ;
	    } ;
	   SOCKETCLOSE(aiaSocket) ;
	 } ;
fail:
	Log(UNUSED,"?? Am I Alive failed!") ;
	
	GenerateConsoleCtrlEvent(CTRL_C_EVENT,0) ;
	
	FreeConsole() ;
	endProcess(EXITABORT) ;
}



void v4SrvSubProcess(v4fearg)
  struct v4fe__SubProcessArguments *v4fearg ;
{ FILE *fp=NULL,*ifp,*lfp ;
  fd_set sset ;				/* Socket set */
  struct sockaddr_in peeraddress ;
  struct stat statbuf ; int ifd ;
  time_t curtime ;
  struct timeval tev ;
  unsigned char MsgBuf[INITIALMSGBYTES],*Msg,*MsgFree ; int MsgMaxBytes = INITIALMSGBYTES ;
  char recvBuf[0x10000] ; int recvBytes, recvX ;
  char *ptr,*b,*b1,*b2,*end,*bol ;
//#define DCLDIMMAX 50
//  char dcldim[DCLDIMMAX][128] ; int dcldimcnt = 0 ;
  char area[2048],func[64],altfunc[64],ajaxMod[1024],isct[0xF0000],dim[1024],tdim[1024] ;
  int pmaKey,pmaKeyCookie ; char cookieList[8192], areaCookie[256] ; LOGICAL gotArea[MAX_AREAS] ;
  char sesKeyCookie[128], serverName[128], serverPort[32], scriptName[V_FileName_Max] ;
  SESKEY sesKey ;
  char comFileName[128] ;
  char cmdfile[V_FileName_Max],logfile[V_FileName_Max],v4rfile[V_FileName_Max],htmfile[V_FileName_Max],ajaxfile[V_FileName_Max],tmpfile[V_FileName_Max],
	excelfile[V_FileName_Max],passthrufile[V_FileName_Max],clientFileName[V_FileName_Max],tblfile[V_FileName_Max], cmdfilesuffix[V_FileName_Max] ;
  char *resfile ;	/* Points to name of results file (either .htm or .ajax) */
  char valstring[128], security[2048] ; char tranid[512], matchbuf[2048], matchstr[20], balist[4096], tbldata[4096] ;
  char imgHdrSave[2048] ;
  char mpboundary[128] ;
  int i,j,k,res,bytes,len,v4,hasval ; char errmsg[512],argbuf[1024],httpbuf[2048],peerIPBuf[128],*peerIP ;
  LOGICAL ok,argsJSON ;
  int jobno1, jobno2 ;			/* Job numbers for this thread */
  char jobId[64] ;			/* Job Id for this thread */
  int urlJobNo1, urlJobNo2 ;		/* Possible job numbers passed in via POST/GET */
  char urlJobId[64] ;			/* Possible jobId passed in via POST/GET */
  UCCHAR slName[64] ;			/* If going stream-lined then the name of it */
  char callBack[64] ;			/* Name of call-back function for JSONP */
  int conSocket ;			/* Connection socket for this thread */
  LOGICAL delWork = FALSE ;		/* If TRUE then delete all work files created for this process instance (xxx.v4, xxx.log, ...) */
  LOGICAL keepalive = FALSE ;
  UCCHAR ctbuf[128],uerrmsg[512],ucargbuf[1024] ;
  LOGICAL delV4WhenDone, fromXXXCGI, startupMsg ;
  int maxbytes,olen,maxbytessize,sent,totalsent,appendbr,contentlen,inheader,restarts,zerores,localFile ;
  enum MSGTYPE msgType ; 
  enum CONTENTTYPE { unknown, imgJPEG, multiPart }  ; enum CONTENTTYPE contentType ;
  char *outbuf = NULL ;

  #define SAARGBUF(AB) \
  { INDEX _i ;\
    for(_i=0;;_i++) { ucargbuf[_i] = AB[_i] ; if (AB[_i] == '\0') break ; };\
    memset(&sa,0,sizeof sa) ; sa.waitFlags = V_SPAWNPROC_Wait ; sa.errBuf = uerrmsg ; sa.fileId = UNUSED ; sa.argBuf = ucargbuf ;\
  } ;

/*	Copy job numbers to local variables (we may flip v4fearg to another set) */
	jobno1 = v4fearg->jobno1 ; jobno2 = v4fearg->jobno2 ;
	sprintf(jobId,"%d-%06d",jobno1,jobno2) ;
/*	comFileName - common file name (JUST name, NOT extension) for all work files for this subprocess */
	sprintf(comFileName,"%s%s",v4fearg->v4prefix,jobId) ;
	sprintf(cmdfile,"%s.v4",comFileName) ; sprintf(v4rfile,"%s.v4r",comFileName) ; sprintf(logfile,"%s.log",comFileName) ;
	sprintf(htmfile,"%s.htm",comFileName) ;	sprintf(ajaxfile,"%s.ajax",comFileName) ; sprintf(excelfile,"%s.xls",comFileName) ;
	sprintf(tblfile,"%s.tbl",comFileName) ; sprintf(tmpfile,"%s.tmp",comFileName) ;
/*	Grab socket from arguments and put into local variable */
	conSocket = v4fearg->ConnectSocket ;

	Msg = MsgBuf ; MsgFree = NULL ; argsJSON = FALSE ;
	ZS(cookieList) ; ZS(areaCookie) ; pmaKeyCookie = 0 ; delV4WhenDone = FALSE ; fromXXXCGI = FALSE ;
	ZS(sesKey) ; ZS(sesKeyCookie) ; ZS(serverName) ; ZS(serverPort) ; ZS(scriptName) ; startupMsg = FALSE ; contentType = unknown ;

	ZS(isct) ;					/* Construct intersection to be eval'ed */
	ZS(area) ; ZS(func) ; ZUS(ajaxMod) ; ZS(altfunc) ; ZS(dim) ; pmaKey = 0 ; ZS(sesKey)
	urlJobNo1 = UNUSED ; urlJobNo2 = UNUSED ; ZS(balist) ; ZS(cmdfilesuffix) ; ZS(urlJobId) ; localFile = UNUSED ; ZS(callBack) ;

	v4 = doProcess ;		/* Default _V4=process */

	Log(jobno2,"Begin of Message") ;

/*	If we have preloaded HTTP request then run with it */
	if (v4fearg->httpMsg != NULL)
	 { strcpy(Msg,v4fearg->httpMsg) ; v4fearg->httpMsg = NULL ;
	   startupMsg = TRUE ; msgType = unknown ; goto process_msg ;
	 } ;

	restarts = -1 ;
keepalive_restart:
	restarts ++ ;				/* Keep track of number of keep-alive restarts */
	bol = Msg ; msgType = unknown ; contentlen = V4LIM_BiggestPositiveInt ; ZS(mpboundary) ; ZS(imgHdrSave) ;
	ldef.Count = 0 ; zerores = 0 ;
//	keepalive = TRUE ;			/* VEH050909 - Default to keepalive = TRUE */
	keepalive = v4fearg->honorKeepAlive ;	/* VEH170920 - Change default */
	recvX = 0 ; recvBytes = 0 ;
	for(inheader=TRUE,ptr=Msg,bytes=0;contentlen>0&&bytes<MsgMaxBytes;bytes++,ptr++)
	 { if (bytes == 7 && msgType == unknown)			/* After first few bytes see if we have HTTP/HTML */
	    { if (strncmp(Msg,"GET ",4) == 0 || strncmp(Msg,"get ",4) == 0) { msgType = get ; }
	       else if (strncmp(Msg,"POST ",5) == 0 || strncmp(Msg,"post ",5) == 0) { msgType = post ; }
	       else if (strncmp(Msg,"OPTIONS",7) == 0 || strncmp(Msg,"options",7) == 0) { msgType = options ; } ;
	    } ;
//	   if (ptr != Msg)			/* Don't wait on first byte */

/*	   Get next byte. If the recvBuf is empty then we have to grab as much as we can from the socket. If not empty then just take next byte */
	   if (recvX >= recvBytes)
	    {
	      if (TRUE)
	       { FD_ZERO(&sset) ; FD_SET(conSocket,&sset) ;
	         tev.tv_sec = (ptr == Msg && restarts == 0 ? v4fearg->initialWait : 1) ; tev.tv_usec = 0 ;	/* Wait a little longer for first byte */
	         res = select(FD_SETSIZE,&sset,NULL,NULL,&tev) ;	/* Wait for something to happen */
//printf("bytes=%d, recvX=%d, recvBytes=%d, contentlen=%d\n",bytes,recvX,recvBytes,contentlen) ;
	         if (res <= 0)
	          { 

			if (restarts == 0) Log(jobno2,"Potential timeout, waiting one more time") ;
			FD_ZERO(&sset) ; FD_SET(conSocket,&sset) ;
			tev.tv_sec = (ptr == Msg && restarts == 0 ? v4fearg->initialWait : 5) ; tev.tv_usec = 0 ;	/* Wait a little longer for first byte */
			res = select(FD_SETSIZE,&sset,NULL,NULL,&tev) ;	/* Wait for something to happen */
			if (res > 0 && restarts == 0) { Log(jobno2,"Nope, got something, continuing...") ; goto dorcv ; } ;

	            if (restarts == 0)
	             Log(jobno2,"? Timeout waiting for input on job(1)") ;
	            goto thread_end ;
	          } ;
	       } ;
dorcv:
	      res = recv(conSocket,recvBuf,sizeof recvBuf,0) ;
//printf("   res=%d\n",res) ;
	      if (res <= 0)
	       { if (res == 0)
	          { if (++zerores < 10)		/* Allow a few of these before getting serious about shutting down */
		     { bytes-- ; ptr-- ; continue ; } ;
		    Log(jobno2,"[Remote connection has shut down (1) gracefully on job]") ;
	          } else
	          { if (bytes > 0 && bytes < 30)
	             { *ptr = '\0' ;
		    Log(jobno2,"? Error (%s) in recv on job (byte #%d, msg=*%s*)",v_OSErrString(NETERROR),bytes,Msg) ;
		     } ;
	          } ;
	         if (bytes < 4)
	          { Msg[bytes] = '\0' ;		/* Didn't get anything worth-while - shut down thread */
	            if (NETERROR != 0) { Log(jobno2,"? Error2 (%s) in recv on job (byte #%d, msg=*%s*)",v_OSErrString(NETERROR),bytes,Msg) ; } ;
	            goto thread_end ;
	          } ;
	         break ;					/* Break out of read loop & process what we got */
	       } ;
	      recvX = 0 ; recvBytes = res ;			/* Initialize into recv buffer */
//printf("recvBytes=%d, processed=%d\n",res,bytes) ;
	    } ;
	   *ptr = recvBuf[recvX++] ;				/* Grab next byte from recv buffer */
	    
	   if (inheader && *ptr == '\r') { ptr-- ;  continue ; } ;	/* Ignore returns from windows systems */
	   if (inheader && *ptr == '\n') *ptr = DCHAR ;			/* Replace \n (newline) with special delimiting character */
	   if (!inheader) { contentlen-- ; continue ; } ;	/* Don't do header checks if past header */
//if (msgType == options) printf("char=%c %d\n",(*ptr <= 26 ? '.' : *ptr),*ptr) ;
	   switch (msgType)
	    { case unknown:	if (*ptr == EOM) { ptr++ ; *ptr = '\0' ; goto process_msg ; } ;
//				if (*ptr == EOM) { *ptr = '\0' ; ptr++ ; goto process_msg ; } ;	/* ^Z indicates end of message */
				break ;
	      case get:
	      case post:
	      case options:
//		if (!(*ptr == '\n' || *ptr == DCHAR)) break ;
		if (*ptr != DCHAR) break ;
		*ptr = '\0' ;
		if (strlen(bol) == 0)	/* Blank line = end of header section */
		 { 
		   if (msgType == get || msgType == options) goto process_msg ;
		   ptr = Msg - 1 ; bytes = 0 ; inheader = FALSE ;
		   break ;
		 } ;
		for(i=0;bol[i]!=':' && bol[i]!='\0';i++) { bol[i] = toupper(bol[i]) ; } ;
{ char save ; save = bol[900] ; bol[900] = '\0' ; cLog(FALSE,"BOL: %s\n",bol) ; bol[900] = save ; }
		if (strncmp(bol,"CONTENT-LENGTH: ",16) == 0)
		 { contentlen = strtol(&bol[16],&end,10) ;
		   if (contentlen > MsgMaxBytes)
		    { SCOPY(imgHdrSave,Msg) ;			/* Save header line */
		      Msg = malloc(contentlen + 100) ;		/* Have to increase buffer for large message */
								/* ptr gets set back to Msg at end of header so everything will adjust OK */
		      MsgFree = Msg ;				/* Set so we know to deallocate on thread termination */
		      MsgMaxBytes = contentlen + 10 ;
		      Log(jobno2,"Increasing message buffer to %d",MsgMaxBytes) ;
		    } ;
		 } else if (strncmp(bol,"CONTENT-TYPE: ",14) == 0)
		 { for(j=0,i=14;bol[i]!=';' && bol[i]!='\0';i++)
		    { ctbuf[j++] = toupper(bol[i]) ; } ; ctbuf[j++] = UClit('\0') ;
		   if (UCstrstr(ctbuf,UClit("MULTIPART/FORM-DATA")) != NULL || TRUE)
		    { b1 = strstr(bol,"boundary=") ; if (b1 == NULL) b1 = strstr(bol,"BOUNDARY=") ;
		      if (b1 != NULL) { SCOPY(mpboundary,"--") ; SCAT(mpboundary,&b1[9]) ; } ;
		      contentType = multiPart ;
		    } else if (UCstrstr(ctbuf,UClit("IMAGE/JPEG")) != NULL)
			    { contentType = imgJPEG ; if (strlen(imgHdrSave) == 0) { SCOPY(imgHdrSave,Msg) ; } ; } ;		/* Save begin of message for handling below */
		 } else if (strncmp(bol,"REMOTE-ADDR:",12) == 0)
		 { char *bc ;
		   for(bc=&bol[12];*bc==' ';bc++) { } ;		/* Get rid of any leading spaces */
		   SCOPY(peerIPBuf,bc) ; peerIP = peerIPBuf ;
		 } else if (strncmp(bol,"FROM-XXXCGI:",12) == 0)
		 { fromXXXCGI = TRUE ;
		 } else if (strncmp(bol,"V4NS-JSON:",10) == 0)
		 { char *bc ;
		   for(bc=&bol[11];*bc==' ';bc++) { } ;
		   SCOPY(func,bc) ; argsJSON = TRUE ;
		 } else if (strncmp(bol,"SERVER-NAME:",12) == 0)
		 { char *bc ;
		   for(bc=&bol[12];*bc==' ';bc++) { } ;
		   SCOPY(serverName,bc) ;
		 } else if (strncmp(bol,"SERVER-PORT:",12) == 0)
		 { char *bc ;
		   for(bc=&bol[12];*bc==' ';bc++) { } ;
		   SCOPY(serverPort,bc) ;
		 } else if (strncmp(bol,"SCRIPT-NAME:",12) == 0)
		 { char *bc ;
		   for(bc=&bol[12];*bc==' ';bc++) { } ;
		   SCOPY(scriptName,bc) ;
		 } else if (strncmp(bol,"COOKIE:",7) == 0)
		 { 
		   if (strlen(bol) + strlen(cookieList) < sizeof cookieList)
		    { char *bc, *bcEnd ; int more ;		/* May have multiple cookies delimited with semi-colon */
		      for(bc=&bol[7];*bc==' ';bc++) { } ;	/* Get rid of starting "COOKIE:" keyword and any leading spaces */
		      for(more=TRUE;more;bc=bcEnd+1)
		       { bcEnd = strchr(bc,';') ; if (bcEnd != NULL) { *bcEnd = '\0' ; } else { more = FALSE ; } ;
		         for(;*bc==' ';bc++) { } ;		/* Get rid of leading spaces */
			 if (*bc == '\0') break ;		/* End of line */
		         if (*bc == '_')			/* Do we possible have cookie: _Area=xxx ? */
		          { INDEX i ;
/*			    May have any number of spaces before or after the '=' - handle it */
		            for(i=1;i<20&&bc[i]!='='&&bc[i]!=' ';i++) { bc[i] = toupper(bc[i]) ; } ;
		            if (bc[i] == '=') { bc[i] = '\0' ; }
		             else { bc[i] = '\0' ; for(i++;bc[i]!='='&&bc[i]!='\0';i++) { } ; } ;
		            for(i++;bc[i]==' ';i++) { } ; 
		            if (strcmp(bc,"_PMAKEY") == 0) { pmaKeyCookie = strtol(&bc[i],&b1,10) ; } ;
		            if (strcmp(bc,"_SESKEY") == 0) { SCOPY(sesKeyCookie,&bc[i]) ; } ;
		            if (strcmp(bc,"_AREA") == 0 && strlen(bc) < sizeof areaCookie) { SCOPY(areaCookie,&bc[i]) ; } ;
		          } ;
		         if (strlen(cookieList) == 0) SCAT(cookieList,"(") ;
		         { char *b = &cookieList[strlen(cookieList)] ;
		           *b = '"' ; b++ ;
		           for(;*bc!='\0';bc++)
		            { if (*bc == '"') { *b = '\\' ; b++ ; } ;
		              *b = *bc ; b++ ;
		            } ;
		           *b = '"' ; b++ ; *b = ' ' ; b++ ; *b = '\0' ;
		         } ;
		       } ;
		    } ;
		 } else if (strncmp(bol,"CONNECTION: ",11) == 0)
		 { for(j=0,i=11;bol[i]!=';' && bol[i]!='\0';i++)
		    { ctbuf[j++] = toupper(bol[i]) ; } ; ctbuf[j++] = UClit('\0') ;
		   if ((UCstrstr(ctbuf,UClit("KEEP-ALIVE")) != NULL) && v4fearg->honorKeepAlive) keepalive = TRUE ;
		 } ;
		bol = ptr + 1 ;
		
		break ;
	    } ;
	 } ;
	if (bytes >= MsgMaxBytes)
	 { Log(jobno2,"? Input message exceeds %d bytes",MsgMaxBytes) ; goto thread_end ; } ;
	*ptr = '\0' ;					/* Terminate with null */


process_msg:
cLog(FALSE,"********************************************************\n") ;
cLog(FALSE,"%d-%d\n",jobno1,jobno2) ;
if (contentType == imgJPEG)
 { char imgFile[V_FileName_Max] ;
   sprintf(imgFile,"%s.jpg",comFileName) ;
   fp = fopen(imgFile,"wb") ;
   Log(jobno2,"Created image file (%s %d bytes)",imgFile,bytes) ;
   fwrite(&Msg[1],1,bytes-1,fp) ; fclose(fp) ;
   msgType = image ;
#ifdef MOOOO
		fp = fopen(htmfile,"w") ;
		if (fp == NULL) { Log(jobno2,"?Could not create error/log html file") ; goto thread_end ; } ;
		fputs("http://192.168.120.146:2350/image.jpg\n",fp) ;
		fputc(EOM,fp) ;
		fclose(fp) ;
		fp = fopen(htmfile,"r") ;
		resfile = htmfile ; goto return_to_client ;
#endif
 } else
 { int i ; char pbuf[1000] ;
   for(i=0;i<sizeof pbuf && Msg[i]!='\0';i++)
    { pbuf[i] = (isprint(Msg[i]) || isspace(Msg[i]) ? Msg[i] : ' ') ; } ;
   pbuf[i<sizeof pbuf ? i : (sizeof pbuf) - 1] = '\0' ;
   cLog(FALSE,"%s\n",pbuf) ;

/*Try to determine peer IP address */
	if (!fromXXXCGI)
	 { if (msgType != unknown)
	    { i = sizeof peeraddress ;
	      if (getpeername(conSocket,(struct sockaddr *)&peeraddress,&i) == 0)
	       { peerIP = inet_ntoa(peeraddress.sin_addr) ;
	       } else { peerIP = "?.?.?.?" ; } ;
	    } else
/*	      ******* THIS CODE NOT NECESSARY WITH NEW xxxCGI (we get id via HTTP headers)  ************** */
	    { b1 = strstr(pbuf,"peerIPAddress") ; if (b1 != NULL) b1 = strchr(b1,'=') ;
/*	      IP address ends with ' ' because formatting above replaces all non-printables with spaces */
	      if (b1 != NULL) { b1++ ; b2 = strchr(b1,' ') ; if (b2 != NULL) { *b2 = '\0' ; } else { b1 = NULL ; } ; } ;
	      if (b1 != NULL) { SCOPY(peerIPBuf,b1) ; peerIP = peerIPBuf ; } else { peerIP = "?.?.?.x" ; } ;
	    } ;
	 } ;



 } ;
cLog(FALSE,"\n********************************************************\n") ;


	switch (msgType)
	 { case unknown:	Log(jobno2,"Processing xxxCGI Message job - %s",peerIP) ; break ;
	   case get:		Log(jobno2,"Processing HTTP GET job - %s",peerIP) ;
				switch (v4SrvSubHTTPGet(Msg,v4fearg))
				 { case file:
/*					Got a request for a file - handle it */
					ptr = strchr(Msg,'/') ; end = strchr(ptr+1,' ') ;
					if (end == NULL) { ptr = "xxx" ; } else { *end = '\0' ; } ;
					Log(jobno2,"Attempting to return file: %s",ptr+1) ;
					v4SrvReturnFile(ptr+1,v4fearg,jobno1,jobno2,conSocket,msgType,TRUE) ; goto finish_job ;
				   case fileAndArgs:
/*					If got _V4 argument then assume it's a V4 request & handle as such */
					if (strstr(Msg,"_V4") != NULL || strstr(Msg,"_v4") != NULL)
					 { ptr = strchr(Msg,'?') ; if (ptr != NULL) Msg = ptr + 1 ;
					   break ;
					 } ;
/*					Got a request for a file AND arguments. For now just ignore arguments */
					ptr = strchr(Msg,'/') ; end = strchr((ptr == NULL ? Msg : ptr+1),'?') ;
					if (end == NULL) { ptr = "xxx" ; } else { *end = '\0' ; } ;
					Log(jobno2,"Attempting to return file + args: %s",ptr+1) ;
					if (v4SrvReturnFile(ptr+1,v4fearg,jobno1,jobno2,conSocket,msgType,TRUE)) goto finish_job ;
					goto thread_end ;
				   case unknown:
				   case args:		break ;
				 } ;
				break ;			/* Parse (post) arguments as V4 process */ 
	   case post:		Log(jobno2,"Processing HTTP POST job - %s",peerIP) ;
/*				If begin of content looks like boundary then treat it as such */
				if (contentType == multiPart && mpboundary[0] == '\0' && strncmp(Msg,"------",6) == 0)
				 { char *d = mpboundary, *s = Msg ;
				   for(;*s!='\n';s++) { *(d++) = *s ; } ;
				   *d = '\0' ; if (*(d-1) < 20) *(d-1) = '\0' ;
				 } ;
				v4SrvSubHTTPPost(Msg,v4fearg,jobno1,jobno2,(mpboundary[0] == '\0' ? NULL : mpboundary)) ;
					break ;
	   case image:		Log(jobno2,"IMAGE **%s**",imgHdrSave) ;
				v4SrvSubHTTPGet(imgHdrSave,v4fearg) ;
				Msg = imgHdrSave ; break ;
	   case options:	{ char *oMsg ; int len ;
				  oMsg =
"HTTP/1.1 200 OK\n\
Cache-Control: private\n\
Allow: OPTIONS,POST,GET\n\
Server: V4Server\n\
Public: OPTIONS,POST,GET\n\
Content-Length: 0\n\n" ;
			  len = send(conSocket,oMsg,strlen(oMsg),0) ;
printf("Sent %d/%d bytes as return message from OPTIONS\n",len,strlen(oMsg)) ;
			  goto thread_end ;
			}

	 } ;

/*	Have to parse all message components - those with "_" prefix are control, others are dimension=value pairs */

	for(i=0;i<MAX_AREAS;i++) { gotArea[i] = FALSE ; } ;
	for(ptr=Msg;*ptr!='\0'&&*ptr!=EOM;ptr=end+1)
	 { end = strchr(ptr,DCHAR) ;				/* Look for end of line */
	   if (end == NULL) end = strchr(ptr,EOM) ;
	   if (end == NULL)					/* Should not get this? */
	    { Log(jobno2,"?Improperly formed message for job: %s",ptr) ;
	      goto thread_end ;
	    } ;
	   *end = '\0' ;
	   if (ptr[0] != '_' || ptr[1] == '_')					/* Got component of isct */
	    { if (strcmp(ptr,"Generate=Go") == 0) continue ;	/* Skip this one! */
//	      if (stricmp(ptr,"INFO=LOGIN") == 0)		/* VEH100827 - Temp hack to allow old style CDF Logins */
//	       { SCOPY(func,"cdfOldCRISLogin") ; SCOPY(sesKey,"none") ;
//	         continue ;
//	       } ;
	      if (strlen(isct) + strlen(ptr) >= sizeof isct)
	       { isct[200] = '\0' ; Log(jobno2,"? Isct (%d+%d) too big: %s",strlen(isct),strlen(ptr),isct) ; goto thread_end ; } ;
	      b = strchr(ptr,'=') ;
	      if (b != NULL)					/* Compare dimension with last dimension */
	       { *b = '\0' ; SCOPY(tdim,ptr) ;			/* tdim = dimension */
	         
		 if (*(b-1) == '_')				/* If name ends with "_" then keep in list */
		  { if(strlen(balist) + strlen(ptr) + 10 < sizeof balist)
		     { SCAT(balist,"(") ; *(b-1) = '\0' ; SCAT(balist,ptr) ; SCAT(balist," \"") ; } ;
		    b++ ;
		    if (*b == '"')
		     { b++ ;					/* Strip off possible leading/ending quote */
		       j = strlen(b) ; if (j > 0 ? b[j-1] == '"' : FALSE) b[j-1] = '\0' ;
		     } ;
		    b1 = &balist[strlen(balist)] ;
		    for(i=0;;b1++,i++)
		     { *b1 = b[i] ; if (b[i] == '\0') break ;
		       if (*b1 == '"') { *(b1++) = '\\' ; *b1 = '"' ; } ;
		     } ;
		    SCAT(balist,"\")") ;
		    continue ;
		  } ;
		 if (strcmp(tdim,dim) == 0)			/* Are they the same? */
		  { b =  b + 1 ;				/*   yes - get to value */
		    isct[strlen(isct)-1] = '\0' ;		/* Zap ending DCHAR */
		    SCAT(isct,",") ; SCAT(isct,b) ; SCAT(isct,DSTR) ;
		    continue ;
		  } else { *b = '=' ; } ;			/*   no - put back the way it was */
	       } ;
	      len = strlen(isct) ;
	      b1 = &isct[len] ;					/* b1 = ptr to begin where this dim:value to be saved */
//	      SCOPY(b1,ptr) ;
	      b = strchr(ptr,'=') ; if (b != NULL) *b = ':' ;	/* Replace "-" with ":" */
	      hasval = (b == NULL ? FALSE : strlen(b) > 1) ;	/* hasval is TRUE if this dimension has a value */
	      if (ptr[0] == '_' && ptr[1] == '_')			/* If begins with '__' then sensitive data - don't let it be displayed */
	       { B64INT eval ; char *eptr, ebuf[64] ;
	         strcat(b1,"Set Echo Disable" DSTR) ;
	         ptr += 2 ;					/* Skip over leading '__' */
		 b = strchr(ptr,':') ;				/* Scan to end of dimension */
		 if (b != NULL) { *b = '\0' ; strcat(b1,ptr) ; strcat(b1,":") ; ptr = b + 1 ; } ;
		 eval = _strtoi64(ptr,&eptr,10) ;		/* Can we parse this as an integer (i.e. is it a credit card or other sensitive number) */
		 if (*eptr == '\0' && xorEncryptValue != 0)	/* Should we encrypt the argument? */
		  { eval = (eval ^ xorEncryptValue) ; sprintf(ebuf,"%I64d",eval) ;
		    strcat(b1,ebuf) ; strcat(b1,DSTR) ;
		  } else
		  { strcat(b1,ptr) ; strcat(b1,DSTR) ;
		  } ;
	         strcat(b1,"Set Echo Enable" DSTR) ;
	         if (!isDebug)
	          delV4WhenDone = TRUE ;			/* Set flag to delete V4 file when done (unless in 'debug' mode) */
	       } else
	       { 
/*		 veh220517 - need to add ajaxMod to Context before calling first preeval */
		 if (stricmp(tdim,"AJAXMOD") == 0)
		  { char *cdim = strchr(ptr,':') ;
		    strcpy(ajaxMod,cdim+1) ;
		  } else
		  { strcat(b1,ptr) ; strcat(b1,DSTR) ;
		  } ;
	       } ;
	      SCOPY(dim,tdim) ;				/* Save last dimension */
next_data:    continue ;
	    } ;
/*	   Have a "_command" - figure out & handle */
	   b = strchr(ptr,'=') ;
	   if (b == NULL)					/* Should not get this? */
	    { Log(jobno2,"Improperly formed message (no \"=\") on job") ;
	      goto thread_end ;
	    } ;
	   *b = '\0' ; for(b=ptr;*b!='\0';b++) *b = toupper(*b) ;
	   b++ ; ptr++ ;						/* b = ptr to argument after "=" */
/*	   Before taking argument, check for eol to prevent possible injection of V4 command of form: "Eval something awful" */
	   for(b1=b;*b1!='\0';b1++) { if (*b1 < ' ') *b1 = ' ' ; } ;
	   if (strlen(ptr) > 32)
	    { Log(jobno2,"Parameter length(%d) exceed max allowed",strlen(ptr)) ; goto thread_end ; } ;
	   if (strcmp(ptr,"AREA") == 0)
	      { if (strlen(area) + strlen(b) + 5 < sizeof area) { SCAT(area,b) ; SCAT(area,DSTR) ; } ;
	    } else if (strcmp(ptr,"FUNC") == 0)
	      { if (strlen(b) < sizeof func) SCOPY(func,b) ;
	    } else if (strcmp(ptr,"ALTFUNC") == 0)
	      { if (strlen(b) < sizeof altfunc) SCOPY(altfunc,b) ;
	    } else if (strcmp(ptr,"PMAKEY") == 0) 
	      { pmaKey = atoi(b) ;
	    } else if (strcmp(ptr,"SESKEY") == 0) 
	      { SCOPY(sesKey,b) ;
	    } else if (strcmp(ptr,"JOBNO1") == 0)
	      { urlJobNo1 = atoi(b) ;
	    } else if (strcmp(ptr,"JOBNO2") == 0)
	      { urlJobNo2 = atoi(b) ;
	    } else if (strcmp(ptr,"CALLBACK") == 0)
	      { SCOPY(callBack,b) ;
	    } else if (strcmp(ptr,"WORK") == 0)
	      { delWork = (b[0] == 'D' || b[0] == 'd') ;
	    } else if (strcmp(ptr,"JOBID") == 0)
	      { char *e ;
	        SCOPY(urlJobId,b) ;
		urlJobNo1 = strtol(urlJobId,&e,10) ;				/* Break into component pieces */
		urlJobNo2 = (*e != '\0' ? strtol(e+1,&e,10) : -1) ; 
	    } else if (strcmp(ptr,"SUBMIT") == 0)			/* _Submit=xxx - ignore this one */
	      {  ;
	    } else if (strcmp(ptr,"CMDFILESUFFIX") == 0)		/* Command file suffix to be spawned */
	      { SCOPY(cmdfilesuffix,b) ;
	    } else if (strcmp(ptr,"DEFAULTS") == 0)			/* Value is list of dim:value dim:value ... */
	      { vsrv_ParseDefaults(b,FALSE) ;
	    } else if (strcmp(ptr,"IGNOREBLANK") == 0)			/* Value is list of dimensions */
	      { vsrv_ParseDefaults(b,TRUE) ;
	    } else if (strcmp(ptr,"V4") == 0)
	      { for(i=0;*(b+i)!='\0';i++) *(b+i) = toupper(*(b+i)) ;
	        if (strcmp(b,"MENU") == 0) { v4 = doMenu ; }
		 else if (strcmp(b,"PROCESS") == 0) { v4 = doProcess ; }
		 else if (strcmp(b,"ARGSJSON") == 0) { v4 = doProcess ; argsJSON = TRUE ; }
		 else if (strcmp(b,"XLPROCESS") == 0) { v4 = doXLProcess ; }
		 else if (strcmp(b,"JSONPROCESS") == 0) { v4 = doJSONProcess ; }
		 else if (strcmp(b,"FILEPASSTHRU") == 0) { v4 = doFilePassThru ; }
		 else if (strcmp(b,"RESTOXL") == 0) { v4 = doResToXL ; }
		 else if (strcmp(b,"V4R2XLSX") == 0) { v4 = doResToXLSX ; sprintf(excelfile,"%s.xlsx",comFileName) ; }
		 else if (strcmp(b,"INSTALL") == 0) { v4 = doInstall ; }
		 else if (strcmp(b,"AJAX") == 0) { v4 = doAjax ; SCOPY(v4rfile,ajaxfile) ; }
		 else if (strcmp(b,killPattern) == 0) { v4 = doRestart ; }
		 else if (strcmp(b,"CONDRESTART") == 0) { v4 = doCondRestart ; }
		 else if (strcmp(b,"SETINDEX") == 0) { v4 = doSetIndex ; }
		 else if (strcmp(b,ruAlivePattern) == 0) { v4 = doIAmAlive ; }
		 else if (strncmp(b,"SL",2) == 0) { v4 = doSL ; for(b+=2,i=0;i<UCsizeof(slName);i++) { slName[i] = b[i] ; if (b[i] == '\0') break ; } ; }
		 else if (strcmp(b,"ACTIVITY") == 0) { v4 = doActivity ; }
		 else if (strcmp(b,"JSONJBT") == 0) { v4 = doJSONJBT ; }
		 else { Log(jobno2,"Invalid V4 server V4 command (%s) on job",b) ;
			goto thread_end ;
		      } ;
	    } else if (strcmp(ptr,"FILE") == 0)
	      { localFile = atoi(b) ;
	    } else
	      { Log(jobno2,"Invalid V4 server keyword (%s)",ptr) ;
	        goto thread_end ;
	      } ;
	 } ;

/*	Want to immediately return local file */
	if (localFile != UNUSED)
	 { char *name ;
	   name = (localFile < 1 || localFile > MAX_FILES ? NULL : v4fearg->files[localFile-1]) ;
	   v4SrvReturnFile((name == NULL ? "invalidfilespec" : name),v4fearg,jobno1,jobno2,conSocket,get,FALSE) ;
	   goto thread_end ;
	 } ;

/*	If no explicit leKey given AND we have one given as cookie then take it */
	if (pmaKey == 0 && pmaKeyCookie != 0) pmaKey = pmaKeyCookie ;
	if (sesKey[0] == '\0' && sesKeyCookie[0] != '\0') SCOPY(sesKey,sesKeyCookie) ;
	if (strlen(altfunc) > 0) SCOPY(func,altfunc) ;		/* If got alternate Func:xxx then it supercedes any regular Func */
/*	Do we have this session (as defined by session key) set up for alternate parameter set? */
	if (skim != NULL)
	 { INDEX i ;
	   for(i=0;i<skim->count;i++)
	    { if (strcmp(sesKey,skim->entry[i].sesKey) == 0) break ; } ;
	   if (i < skim->count)
	    { *v4fearg = v4ArgSets[skim->entry[i].setX - 1] ;
	    } ;
	 } ;

/*	Construct the V4 command file */
	if (strlen(func) == 0)
	 { v4SrvRet400Error(v4fearg,jobno1,jobno2,conSocket,v4,"_Func argument not defined") ; goto finish_job ; } ;
	appendbr = FALSE ;			/* Set to TRUE on error return so V4 error dump is readable */
	if (v4 == doMenu)			/* Maybe return main menu real fast (if exists) */
	 { SYSTEMTIME st ;
	   GetLocalTime(&st) ;
	   sprintf(argbuf,"%s%04d%02d%02d-%s.htm",v4fearg->v4prefix,st.wYear,st.wMonth,st.wDay,sesKey) ;
	    fp = fopen(argbuf,"rb") ;
	    if (fp != NULL)
	     { Log(jobno2,"Returning previously generated menu: %s",argbuf) ;
	       SCOPY(htmfile,argbuf) ; resfile = htmfile ; goto return_to_client ;
	     } ;
	    strcpy(htmfile,argbuf) ;		/* Menu does not exist - rename htmfile so we can grab it */
	    v4 = doProcess ;			/* Continue with menu generation */
	 } ;
	if (v4 == doSL)
	 { struct v4fe__StreamLine *vfsl = NULL ;
	   for(i=0;(i < MAX_STREAMLINED ? v4fearg->slList[i] != NULL : FALSE);i++)
	    { if (UCstrcmpIC(v4fearg->slList[i]->slName,slName) == 0) { vfsl = v4fearg->slList[i] ; break ; } ; } ;
	   if (vfsl == NULL) { Log(jobno2,"Unknown SL reference: %s",UCretASC(slName)) ; goto finish_job ; }
	   v4SrvProcStreamLine(v4fearg,jobno1,jobno2,conSocket,sesKey,jobId,isct,vfsl) ;
	   goto finish_job ;
	 } ;

	fp = fopen(cmdfile,"w") ;
	if (fp == NULL)
	 { Log(jobno2,"? Error (%s) opening V4 command file(%s)",v_OSErrString(errno),cmdfile) ; goto thread_end ; } ;
	fprintf(fp,"Set Echo\n") ;
	if (v4fearg->site[0] != '\0')		/* If site=xxx paramter given then output LIDS (log ID stamp) as comment */
	 { fprintf(fp,"!LIDS %s %s %s\n",v4fearg->site,func,logfile) ;
	 } ;
	if (strlen(v4fearg->output) > 0) fprintf(fp,"Set Output %s\n",v4fearg->output) ;
//Log(jobno2,"*6* Area1='%s'",v4fearg->area[0]) ;
	if (v4 == doAjax)
	 { fprintf(fp,"Set ADV EchoE %s\n",ajaxfile) ; fprintf(fp,v4fearg->ajaxErrPattern) ; fprintf(fp,"\n") ;
	   if (strlen(v4fearg->ajaxJSONPattern)) { fprintf(fp,v4fearg->ajaxJSONPattern) ; fprintf(fp,"\n") ; } ;
	 } else { fprintf(fp,"Set ADV EchoE %s\n",v4rfile) ; } ;
	if (strlen(area) == 0 && strlen(areaCookie) > 0) { SCOPY(area,areaCookie) ; SCAT(area,DSTR) ; } ;
	if (strlen(area) == 0) { SCOPY(area,"1") ; } ;	/* Default to _Area=1 */
	for(ptr=area;*ptr!='\0';ptr=b+1)
	 { b = strchr(ptr,DCHAR) ; if (b != NULL) *b = '\0' ;
	   if (strlen(ptr) > 1)
	    { if (strlen(v4fearg->area[0]) > 0) { fprintf(fp,"Area ** Invalid Area argument (%s) - cannot override predefined Areas! **\n",ptr) ; continue ; } ;
	      fprintf(fp,"Area %s\n",ptr) ;
	    } else
	    { i = strtol(ptr,&end,10) ;
	      if (*end != '\0' || i < 1 || i > MAX_AREAS) { fprintf(fp,"Area **Invalid Area code(%s) **\n",ptr) ; continue ; } ;
	      if (gotArea[i-1]) continue ;			/* Already did this area (may get duplicates from CGI handling of cookies) VEH070527 */
	      fprintf(fp,"Area %s\n",v4fearg->area[i-1]) ; gotArea[i-1] = TRUE ;
	    } ;
//	   fprintf(fp,"If1 not Dimension Area Dimension Area Alpha\n") ;
	   fprintf(fp,"Context Add Area:\"%s\"\n",ptr) ;
	   if (b == NULL) break ;
	 } ;
/*	Add in current leKey & job number */
	if (pmaKey != 0) fprintf(fp,"Context Add pmaKey:%d\n",pmaKey) ;
	if (sesKey[0] != '\0') fprintf(fp,"Context Add sesKey:%1s\n",sesKey) ;
	fprintf(fp,"Context Add jobId:%s\n",jobId) ;
	if (strlen(ajaxMod) > 0)
	 { fprintf(fp,"Context Add ajaxMod:%s\n",ajaxMod) ;
	 } ;
	if (strlen(cookieList) > 0)
	 { fprintf(fp,"If1 not Dimension cookieList Dimension cookieList List\n") ;
	   SCAT(cookieList,")") ;
	   fprintf(fp,"Context Add cookieList:%s\n",cookieList) ;
	 } ;
	if (strlen(serverName) > 0)
	 { char tbuf[4096] ;
	   SCOPY(tbuf,serverName) ;
/*	   Append port only if defined AND not equal to 80 (default) */
	   if (strlen(serverPort) > 0 ? strcmp(serverPort,"80") != 0 : FALSE) { SCAT(tbuf,":") ; SCAT(tbuf,serverPort) ; } ;
	   if (strlen(scriptName) > 0) { SCAT(tbuf,";") ; SCAT(tbuf,scriptName) ; } ;
	   fprintf(fp,"Context Add httpServer:\"%s\"\n",tbuf) ;
	 } ;
//	fprintf(fp,"If1 not Dimension XLProcess Dimension XLProcess Logical\n") ;
/*	If this program handling HTTP then determine client IP address (otherwise xxxCGI will have sent it as part of URL) */
	if (fromXXXCGI)
	 { fprintf(fp,"Context Add peerIPAddress:\"%s\"\n",peerIP) ;
	 } else	if (msgType != unknown)
	 { i = sizeof peeraddress ;
	   if (getpeername(conSocket,(struct sockaddr *)&peeraddress,&i) == 0)
	    { 
	      fprintf(fp,"Context Add peerIPAddress:\"%s\"\n",inet_ntoa(peeraddress.sin_addr)) ;
	    } else { Log(jobno2,"Problems with getpeername() - errno=%d",NETERROR) ; } ;
	 } else { fprintf(fp,"Context Add peerIPAddress:\"127.0.0.1\"\n") ; } ;
	if (strlen(v4fearg->includefile1) > 0) fprintf(fp,"Include %s\n",v4fearg->includefile1) ;

	switch (v4)
	 { default:
		Log(jobno2,"? No _V4=xxx parameter specified") ; goto thread_end ;
	   case doRestart:
	   case doIAmAlive:
		break ;
	   case doXLProcess:
		fprintf(fp,"Context Add wwwV4:XLProcess\n") ;
		DOPREEVAL
		break ;
	   case doMenu:
		fprintf(fp,"Context Add wwwV4:Menu\n") ;
		DOPREEVAL
		break ;
	   case doAjax:
		fprintf(fp,"Context Add wwwV4:AJAX\n") ;
		DOPREEVAL				/* Note - ajaxfile has been copied into v4rfile */
		break ;
	   case doJSONJBT:
		fprintf(fp,"Context Add wwwV4:JSONJBT\n") ;
		DOPREEVAL				/* Note - ajaxfile has been copied into v4rfile */
		break ;
	   case doProcess:
		fprintf(fp,(startupMsg ? "Context Add wwwV4:Startup\n" : "Context Add wwwV4:Process\n")) ;
		DOPREEVAL
		break ;
	   case doJSONProcess:
		fprintf(fp,"Context Add wwwV4:JSONProcess\n") ;
		DOPREEVAL
		break ;
	   case doActivity:
		fprintf(fp,"Context Add wwwV4:Activity\n") ;
		DOPREEVAL
		break ;
	   case doInstall:
		fprintf(fp,"Context Add wwwV4:Install\n") ;
		DOPREEVAL
		break ;
	   case doFilePassThru:
		fprintf(fp,"Context Add wwwV4:Passthru\n") ;
		DOPREEVAL
		break ;
	   case doCondRestart:
		fprintf(fp,"Context Add wwwV4:condRestart\n") ;
		DOPREEVAL
		break ;
	   case doSetIndex:
		fprintf(fp,"Context Add wwwV4:setIndex\n") ;
		DOPREEVAL
		break ;
	   case doResToXLSX:
	   case doResToXL:
		break ;
	 } ;
	if (strlen(v4fearg->includefile2) > 0) fprintf(fp,"Include %s\n",v4fearg->includefile2) ;
//	for(i=0;i<dcldimcnt;i++) { fprintf(fp,"Dim %s\n",dcldim[i]) ; } ;	/* Have to declare any dimensions? */
	for(ptr=isct;(!argsJSON) && *ptr!='\0';ptr=b+1)
	 { b = strchr(ptr,DCHAR) ; *b = '\0' ;
/*	   Look for "Set Echo xxx" & if found then output as stand-alone command, not part of 'Context ADV ...' */
	   if (strncmp(ptr,"Set Echo ",9) == 0)
	    { fprintf(fp,"%s\n",ptr) ; continue ; } ;

/*	   See if this is to be ignored or defaulted */
	   for(j=0;;j++) { ldef.Dimbuf[j] = toupper(ptr[j]) ; if (ptr[j] == '\0' || ptr[j] == ' ' || ptr[j] == ':') break ; } ; ldef.Dimbuf[j] = '\0' ;
#define SKIP(DIM) if (strcmp(ldef.Dimbuf,DIM) == 0) continue ;
	   SKIP("JOBID") ; SKIP("COOKIELIST") ; SKIP("HTTPSERVER") ; SKIP("PEERIPADDRESS") ; SKIP("WWWV4") ; SKIP("FUNC") ; SKIP("AREA") ;
	   for(j=0;j<ldef.Count;j++)
	    { if (strcmp(ldef.Dimbuf,ldef.Entry[j].Dimname) != 0) continue ;
	      ZS(ldef.Entry[j].Dimname) ;
	      if (ldef.Entry[j].IgnoreIfBlank)		/* Should we skip this if no value to pass? */
	       { b1 = strchr(ptr,':') ;
	         if (b1 == NULL ? TRUE : *(b1 + 1) == '\0') ptr = NULL ;	/* No value - set ptr to NULL so we don't output it below */
	       } ;
	      break ;
	    } ;
	   if (ptr == NULL) continue ;			/* Skip this one */
	   b1 = strchr(ptr,':') ; if (b1 == NULL ? TRUE : (strcmp(b1+1,"_!NULL!_") != 0)) fprintf(fp,"Context ADV %s\n",ptr) ;
//	    } ;
	 } ;

	 if (argsJSON)
	  { char jbt[0x20000] ; int i ;
	    strcpy(jbt,"args:{") ;
	    for(i=0,ptr=isct;*ptr!='\0';i++,ptr=b+1)
	     { b = strchr(ptr,DCHAR) ; *b = '\0' ;
	       b1 = strchr(ptr,':') ; if (b1 == NULL) continue ;
	       if (i > 0) strcat(jbt,",") ;
	       *b1 = '\0' ; strcat(jbt,"'") ; strcat(jbt,ptr) ; strcat(jbt,"':'") ;
	       b2 = &jbt[strlen(jbt)] ;
	       for(len=0,b1++;len<512&&*b1!='\0';len++,b1++)
	        { if (*b1 == '\'') *(b2++) = '\\' ;
		  if (*b1 == '\\' && *(b1+1) == 'r')
		   *(b1+1) = 'n' ;
		  if (*b1 == '\r') continue ;
		  if (*b1 == '\n') { *(b2++) = '\\' ; *(b2++) = 'n' ; continue ; } ;
		  *(b2++) = *b1 ;
		} ; *b2 = '\0' ;
	       strcat(jbt,"'") ;
	     } ;
	    strcat(jbt,"}") ;
	    fprintf(fp,"Context ADV jbt:%s\n",jbt) ;
	  } ;

	for(i=0;i<ldef.Count;i++)			/* Go through list & supply any unused defaults */
	 { if (ldef.Entry[i].IgnoreIfBlank || strlen(ldef.Entry[i].Dimname) == 0) continue ;
	   b1 = strchr(ldef.Entry[i].Value,':') ;
	   if (b1 == NULL ? TRUE : (strcmp(b1+1,"_!NULL!_") != 0)) fprintf(fp,"Context ADV %s\n",ldef.Entry[i].Value) ;
	 } ;
	if (strlen(balist) > 0)					/* Did we get a big-argument list? */
	 { fprintf(fp,"If1 not Dimension BAList Dimension BAList List\n") ;
	   fprintf(fp,"Context Add BAList:(%s)\n",balist) ;
	 } ;
		
	ok = fprintf(fp,"Set Trace +Progress\n") ;

	switch (v4)
	 { default:
		Log(jobno2,"? No _V4=xxx parameter specified") ; goto thread_end ;
	   case doRestart:
		restartHHMM = 0 ; restartDayOfMonth = 0 ;		/* This will force restart on next top-level cycle */
		heartbeat = 1 ;						/* Set heartbeat to 1 second so we don't wait too long */
/*		Return short alert to inform sender */
		fp = fopen(htmfile,"w") ;
		if (fp == NULL) { Log(jobno2,"Could not create error/log html file") ; goto thread_end ; } ;
		Log(jobno2,"Received kill request") ;
		fputs("<html><head><title>Shutdown Request</title>\n",fp) ;
		fputs("<script language='javascript'>alert('V4 Server Shutdown in next couple of minutes!');</script>",fp) ;
		fputs("</head></html>\n",fp) ;
		break ;
	   case doIAmAlive:
/*		Return short message to inform sender */
		fclose(fp) ;		/* fp already open for v4 file - close it since we don't need it */
		fp = fopen(htmfile,"w") ;
		if (fp == NULL) { Log(jobno2,"?Could not create error/log html file") ; goto thread_end ; } ;
		Log(jobno2,"Received Am-I-Alive Request") ;
		fputs("<html><head><title>Am I Alive</title></head>\n",fp) ;
		fputs("<body>",fp) ;
		fputs("Yes I Am",fp) ;
		fputs("</body></html>\n",fp) ;
		fputc(EOM,fp) ;
		break ;
	   case doXLProcess:
		if (strlen(v4fearg->maineval) > 0) { UCCHAR ucBuf[512] ; v_Msg(NULL,ucBuf,v4fearg->maineval,func,v4rfile) ; UCstrcpyToASC(argbuf,ucBuf) ; fputs(argbuf,fp) ; }
		 else { fprintf(fp,"Eval [Func:%s] > %s\n",func,v4rfile) ; } ;
		break ;
	   case doMenu:
		if (strlen(v4fearg->maineval) > 0) { UCCHAR ucBuf[512] ; v_Msg(NULL,ucBuf,v4fearg->maineval,func,v4rfile) ; UCstrcpyToASC(argbuf,ucBuf) ; fputs(argbuf,fp) ; }
		 else { fprintf(fp,"Eval [Func:%s] > %s\n",func,v4rfile) ; } ;
		break ;
	   case doAjax:
		if (strlen(v4fearg->ajaxJSONPattern))	/* Do this again because V4 resets after each Eval */
		 { fprintf(fp,v4fearg->ajaxJSONPattern) ; fprintf(fp,"\n") ; } ;
		if (strlen(v4fearg->maineval) > 0) { UCCHAR ucBuf[512] ; v_Msg(NULL,ucBuf,v4fearg->maineval,func,ajaxfile) ; UCstrcpyToASC(argbuf,ucBuf) ; fputs(argbuf,fp) ; }
		 else { fprintf(fp,"Eval [Func:%s] > %s\n",func,ajaxfile) ; } ;
		DOPOSTEVAL
		break ;
	   case doJSONJBT:
		if (strlen(v4fearg->maineval) > 0) { UCCHAR ucBuf[512] ; v_Msg(NULL,ucBuf,v4fearg->maineval,func,ajaxfile) ; UCstrcpyToASC(argbuf,ucBuf) ; fputs(argbuf,fp) ; }
		 else { fprintf(fp,"Eval [Func:%s] > %s\n",func,ajaxfile) ; } ;
		DOPOSTEVAL
		break ;
	   case doProcess:
		if (strlen(v4fearg->maineval) > 0) { UCCHAR ucBuf[512] ; v_Msg(NULL,ucBuf,v4fearg->maineval,func,v4rfile) ; UCstrcpyToASC(argbuf,ucBuf) ; fputs(argbuf,fp) ; }
		 else { fprintf(fp,"Eval [Func:%s] > %s\n",func,v4rfile) ; } ;
		DOPOSTEVAL
		break ;
	   case doJSONProcess:
		if (strlen(v4fearg->maineval) > 0) { UCCHAR ucBuf[512] ; v_Msg(NULL,ucBuf,v4fearg->maineval,func,v4rfile) ; UCstrcpyToASC(argbuf,ucBuf) ; fputs(argbuf,fp) ; }
		 else { fprintf(fp,"Eval [Func:%s] > %s\n",func,v4rfile) ; } ;
		DOPOSTEVAL
		break ;
	   case doActivity:
		if (strlen(v4fearg->maineval) > 0) { fprintf(fp,v4fearg->maineval,func,v4rfile) ; }
		 else { fprintf(fp,"Eval [Func:%s] > %s\n",func,v4rfile) ; } ;
		break ;
	   case doInstall:
		if (strlen(v4fearg->maineval) > 0) { UCCHAR ucBuf[512] ; v_Msg(NULL,ucBuf,v4fearg->maineval,func,v4rfile) ; UCstrcpyToASC(argbuf,ucBuf) ; fputs(argbuf,fp) ; }
		 else { fprintf(fp,"Eval [Func:%s] > %s\n",func,v4rfile) ; } ;
		DOPOSTEVAL
		break ;
	   case doFilePassThru:
		if (strlen(v4fearg->maineval) > 0) { UCCHAR ucBuf[512] ; v_Msg(NULL,ucBuf,v4fearg->maineval,func,v4rfile) ; UCstrcpyToASC(argbuf,ucBuf) ; fputs(argbuf,fp) ; }
		 else { fprintf(fp,"Eval [Func:%s] > %s\n",func,v4rfile) ; } ;
		DOPOSTEVAL
		break ;
	   case doCondRestart:
		if (strlen(v4fearg->maineval) > 0) { UCCHAR ucBuf[512] ; v_Msg(NULL,ucBuf,v4fearg->maineval,func,v4rfile) ; UCstrcpyToASC(argbuf,ucBuf) ; fputs(argbuf,fp) ; }
		 else { fprintf(fp,"Eval [Func:%s] > %s\n",func,v4rfile) ; } ;
		DOPOSTEVAL
		break ;
	   case doSetIndex:
		if (strlen(v4fearg->maineval) > 0) { UCCHAR ucBuf[512] ; v_Msg(NULL,ucBuf,v4fearg->maineval,func,v4rfile) ; UCstrcpyToASC(argbuf,ucBuf) ; fputs(argbuf,fp) ; }
		 else { fprintf(fp,"Eval [Func:%s] > %s\n",func,v4rfile) ; } ;
		DOPOSTEVAL
		break ;
	   case doResToXLSX:
	   case doResToXL:
		break ;
	 } ;

	ok = fclose(fp) ;
	if (ok != 0) Log(jobno2,"fclose error(%d)",errno) ;
	fp = NULL ;

/*	Now spawn off V4 subprocess & let it rip */
	switch (v4)
	 { case doResToXL:	break ;
	   case doResToXLSX:	break ;
	   case doXLProcess:
	   case doActivity:
	   case doFilePassThru:
	   case doCondRestart:
	   case doSetIndex:
	   case doAjax:
	   case doJSONJBT:
	   case doMenu:
	   case doProcess:
	   case doJSONProcess:
		sprintf(argbuf,v4fearg->xv4mask,cmdfile,logfile) ;
		Log(jobno2,"Spawn of subprocess: %s",argbuf) ;
//		if (!v_SpawnProcess(NULL,ASCretUC(argbuf),V_SPAWNPROC_Wait,uerrmsg,NULL,NULL,UNUSED))
//		 { Log(jobno2,"Error in xv4 spawn: %s",UCretASC(uerrmsg)) ; goto thread_end ; } ;
		{ struct V__SpawnArgs sa ; SAARGBUF(argbuf) ;
		  if (!v_SpawnProcess(&sa)) { Log(jobno2,"Error in xv4 spawn: %s",UCretASC(uerrmsg)) ; goto thread_end ; } ;
		}
		if (delV4WhenDone)
		 { remove(cmdfile) ; Log(jobno2,"Deleting source V4 file: %s",cmdfile) ;
		 } ;
		break ;
	   case doInstall:
/*		Make sure install file exists - we don't want an unauthorized user trying to do this */
		{ char installFile[V_FileName_Max] ; UCCHAR *ufp, uerrbuf[512] ;
		  sprintf(installFile,"v4lWork:ins-%s.v4",sesKey) ;
		  ufp = v_UCLogicalDecoder(ASCretUC(installFile),VLOGDECODE_Exists,0,uerrbuf) ;
		  if (stat(UCretASC(ufp),&statbuf) != 0) goto thread_end ;	/* If this file does not exists then holder of sesKey is not doing an install */
		} ;
///*		Before we issue install process, make sure none of the streamline services are running - issue restart command to each active one */
//		{ INDEX i ; char slRestart[128] ;
//		  strcpy(slRestart,"slStatus:restart" DSTR) ;		/* Copy into local because v4SrvProcStreamLine() will try to update the string */
//		  for(i=0;i<MAX_STREAMLINED;i++)
//		   { if (v4ArgSets[argSetX].slList[i] == NULL) continue ;
//		     v4SrvProcStreamLine(v4fearg,jobno1,jobno2,UNUSED,sesKey,jobId,slRestart,v4ArgSets[argSetX].slList[i]) ;
//		   } ;
//		}
/*		Arguments: 1=v4rFile, 2=logFile, 3=jobId, 4=sesKey, 5=func, 6=area, 7=cgiLink */
		{ UCCHAR ucBuf[512] ;
		  v_Msg(NULL,ucBuf,v4fearg->installmask,v4rfile,logfile,jobId,sesKey,func,area,v4fearg->cgiserver) ;
		  UCstrcpyToASC(argbuf,ucBuf) ;
		  Log(jobno2,"Install: %s",argbuf) ;
//		  if (!v_SpawnProcess(NULL,ASCretUC(argbuf),V_SPAWNPROC_Wait,uerrmsg,NULL,NULL,UNUSED))
//		   { Log(jobno2,"Error in installation spawn: %s",uerrmsg) ; goto thread_end ; } ;
		  { struct V__SpawnArgs sa ; SAARGBUF(argbuf) ;
		    if (!v_SpawnProcess(&sa)) { Log(jobno2,"Error in installation spawn: %s",UCretASC(uerrmsg)) ; goto thread_end ; } ;
		  }
		}
	 } ;

/*	Convert V4 result file to HTML/BIFF */
handle_result:
	switch (v4)
	 { 
	   case doInstall:
	   case doProcess:
dp_entry:
		sprintf(tranid,"%d-%d",jobno1,jobno2) ;
		if (v4fearg->htmlmask[0] != '\0')		/* Got new switch ? */
		 { UCCHAR ucBuf[512] ;
/*		      Arguments: 1=v4rFile, 2=htmlFile, 3=logFile, 4=jobId, 5=sesKey, 6=area, 7=cgiLink */
		   v_Msg(NULL,ucBuf,v4fearg->htmlmask,v4rfile,htmfile,logfile,jobId,sesKey,area,v4fearg->cgiserver) ;
		   UCstrcpyToASC(argbuf,ucBuf) ;
		 } else
		 { 
		 } ;
		Log(jobno2,"V4R->HTML: %s",argbuf) ;
		{ struct V__SpawnArgs sa ; SAARGBUF(argbuf) ;
		  if (!v_SpawnProcess(&sa)) { Log(jobno2,"Error in xvrestohtml spawn: %s",UCretASC(uerrmsg)) ; goto thread_end ; } ;
		}
		break ;
	   case doJSONProcess:
		sprintf(tranid,"%d-%d",jobno1,jobno2) ;
		if (v4fearg->jsonmask[0] != '\0')		/* Got new switch ? */
		 { UCCHAR ucBuf[512] ;
		   v_Msg(NULL,ucBuf,v4fearg->jsonmask,v4rfile,ajaxfile,logfile,jobId,sesKey,area,v4fearg->cgiserver) ;
		   UCstrcpyToASC(argbuf,ucBuf) ;
		 } ;
		Log(jobno2,"V4R->JSON: %s",argbuf) ;
		{ struct V__SpawnArgs sa ; memset(&sa,0,sizeof sa) ; sa.argBuf = ASCretUC(argbuf) ;
		  sa.waitFlags = V_SPAWNPROC_Wait ; sa.errBuf = uerrmsg ; sa.fileId = UNUSED ;
		  if (!v_SpawnProcess(&sa)) { Log(jobno2,"Error in xvrestojson spawn: %s",UCretASC(uerrmsg)) ; goto thread_end ; } ;
		}
		break ;
	   case doActivity:
		ZS(passthrufile) ;
		ifp = fopen(v4rfile,"r") ;		/* Open result file to get name of passthru file */
		if (ifp == NULL) { v4SrvProcError(v4fearg,jobno1,jobno2,conSocket,v4) ; goto finish_job ; } ;
		fp = fopen(tmpfile,"w") ;
		if (fp == NULL) { Log(jobno2,"?Could not create actvitity temp file") ; goto thread_end ; } ;
		for(;ifp != NULL;)
		 { int start, end, rawX ;
		   if (fgets(tbldata,sizeof tbldata,ifp) == NULL) break ;
		   if (strstr(tbldata,"**ACTIVITY**") == NULL) { fputs(tbldata,fp) ; continue ; } ;
		   start = curX - 500 ; if (start < 0) start = 0 ; end = curX ;
		   for(rawX=start;rawX<end;rawX++)
		    { fputs(lineBuf[rawX % LINESINDEXMASK],fp) ; fputs("\n",fp) ;
		    } ;
		 } ;
		fclose(fp) ; fclose(ifp) ;
		SCOPY(v4rfile,tmpfile) ;		/* Make the temp file with activity the new 'v4r' file & then call v4rpp */
		SCOPY(passthrufile,htmfile) ;
		goto dp_entry ;
	   case doCondRestart:
/*		The result contains the 'conditional' exit code (if not a number then do not exit, return as 'result') */
		ifp = fopen(v4rfile,"r") ;		/* Open result file to get exit code */
		if (ifp == NULL) goto dp_entry ;
		 { char crBuf[128],*end ; int exitCode ;
		   fgets(crBuf,sizeof crBuf,ifp) ; fclose(ifp) ;
		   exitCode = strtol(crBuf,&end,10) ;
		   if (!(*end == '\0' || *end <= 26))	/* Make sure we got a valid exit code */
		    goto dp_entry ;
		   endProcess(exitCode) ;
		 } ;
		break ;
	   case doSetIndex:
/*		The result contains the session key & set index to use: 'sessionkey index' */
		ifp = fopen(v4rfile,"r") ;		/* Open result file to get the key & index */
		if (ifp == NULL) goto dp_entry ;
		 { SESKEY sesKey1 ; INDEX i ;
		   char setBuf[128],msg[256],*end ; INDEX index ;
		   fgets(setBuf,sizeof setBuf,ifp) ; fclose(ifp) ;
		   end = strchr(setBuf,' ') ;
		   if (end == NULL) { Log(jobno2,"Invalid _V4=setIndex data: %s",setBuf) ; v4SrvProcError(v4fearg,jobno1,jobno2,conSocket,v4) ; goto finish_job ; } ;
		   *end = '\0' ; SCOPY(sesKey1,setBuf) ;
		   index = strtol(&end[1],&end,10) ;
		   if (index < 1 || index > ARGSETMAX)
		    { Log(jobno2,"Specified index (%d) out of allowed range (1-%d)",index,ARGSETMAX) ; v4SrvProcError(v4fearg,jobno1,jobno2,conSocket,v4) ; goto finish_job ; } ;
		   if (UCempty(v4ArgSets[index-1].xv4mask))
		    { Log(jobno2,"Failed attempt to switch to an empty parameter set (%d)",index) ;
		      sprintf(msg,"content-type: text/html\n\n<html><script type='text/javascript'>alert(\"Could not switch to parameter set - it is undefined\");history.go(-1);</script></html>\n",jobno1,jobno2) ;
		      send(conSocket,msg,strlen(msg),0) ;
		      goto finish_job ;
		    } ;
/*		   Save this info so on subsequent calls we can tweak set index */
		   if (skim == NULL) skim = (struct v4fe__sesKey2IndexMap *)v4mm_AllocChunk(sizeof *skim,TRUE) ;
		   for(i=0;i<skim->count;i++)
		    { if (strcmp(skim->entry[i].sesKey,sesKey1) == 0) break ; } ;
		   if (i >= skim->count)		/* Have to add new entry? */
		    { if (index > 1)			/* Don't add if resetting to default */
		       { if (i >= SESKEYINDEXMAPMAX) { Log(jobno2,"Exceeded max number (%d) of skim entries",SESKEYINDEXMAPMAX) ; v4SrvProcError(v4fearg,jobno1,jobno2,conSocket,v4) ; goto finish_job ; } ;
		         i = skim->count ; skim->count++ ;
		       } ;
		    } ;
		   if (index == 1)			/* Remove from skim table (but only if already in table) */
		    { if (i < skim->count) { skim->count-- ; skim->entry[i] = skim->entry[skim->count] ; } ;
		      Log(jobno2,"Removing skim[%d]=%s/%d entry, new count=%d",i,skim->entry[i].sesKey,skim->entry[i].setX,skim->count) ;
		      sprintf(msg,"content-type: text/html\n\n<html><script type='text/javascript'>alert(\"The parameter has been reset to the default\");history.go(-1);</script></html>\n",jobno1,jobno2) ;
		      send(conSocket,msg,strlen(msg),0) ;
		      goto finish_job ;
		    } ;
		   strcpy(skim->entry[i].sesKey,sesKey1) ; skim->entry[i].setX = index ;
		   Log(jobno2,"Adding skim[%d]=%s/%d entry",skim->count,skim->entry[i].sesKey,skim->entry[i].setX) ;
		   sprintf(msg,"content-type: text/html\n\n<html><script type='text/javascript'>alert(\"The parameter set for this session has been modified\");history.go(-1);</script></html>\n",jobno1,jobno2) ;
		   send(conSocket,msg,strlen(msg),0) ;
		   goto finish_job ;
		 } ;
		goto finish_job ;
	   case doFilePassThru:
/*		The result contains the name of the file to be passed back- grab it, open it, and return as binary */
		ZS(passthrufile) ; ZS(clientFileName) ;
		ifp = fopen(v4rfile,"r") ;		/* Open result file to get name of passthru file */
		if (ifp != NULL)
		 { fgets(passthrufile,sizeof passthrufile,ifp) ;	/* First line is the actual name of the file */
		   for(i=strlen(passthrufile)-1;i>0;i--) { if (passthrufile[i] > 20) break ; passthrufile[i] = '\0' ; } ;
		   fgets(clientFileName,sizeof clientFileName,ifp) ;	/* Optional second line is the name to pass to client */
		   for(i=strlen(clientFileName)-1;i>0;i--) { if (clientFileName[i] > 20) break ; clientFileName[i] = '\0' ; } ;
		   fclose(ifp) ;
		 } ;
pt_entry:
		ifd = open(passthrufile,O_RDONLY|O_BINARY) ;
		if (ifd == -1)
		 { Log(jobno2,"Error (%d) opening pass-thru file '%s' - treating as .v4r file",errno,passthrufile) ;
		   v4 = doProcess ; goto handle_result ;
		 } ;
		fstat(ifd,&statbuf) ; close(ifd) ;		/* Get size of the file */
		fp = fopen(passthrufile,"rb") ;
		if (fp == NULL)
		 { 
		   v4SrvProcError(v4fearg,jobno1,jobno2,conSocket,v4) ; goto finish_job ;
//		   b = "Content-Type: text/html\n\n" ;  send(conSocket,b,strlen(b),0) ;
//		   b = "An error occurred while processing your request.\n" ; send(conSocket,b,strlen(b),0) ;
//		   b = "The following is a log of what transpired-\n" ; send(conSocket,b,strlen(b),0) ;
//		   fp = fopen(logfile,"r") ; appendbr = TRUE ;
//		   SCOPY(htmfile,logfile) ;			/* Copy name of log file into htmfile because this is what we are returning */
//		   resfile = htmfile ; goto return_to_client ;
		 } ;
		maxbytes = 256 ;			/* BIFF files always in chunks of 256 */
		maxbytes = 4096 ;			/* Increased for better performance - VEH111214 */
		ZS(httpbuf) ;
		Log(jobno2,"Send Passthru(%s) as(%s) buffer size = %d",passthrufile,(strlen(clientFileName) == 0 ? "same" : clientFileName),maxbytes) ;
		outbuf = malloc(maxbytes +100) ; olen = 0 ;
		if (msgType == get || msgType == post)		/* If feserver playing HTTP handler then return http header */
		 { sprintf(argbuf,"HTTP/1.1 200 OK\n") ; SCAT(httpbuf,argbuf) ; } ;
		sprintf(argbuf,"Server: V4WWWFEServer %d / V4 %d.%d\n",V4FE_Version,V4IS_MajorVersion,V4IS_MinorVersion) ; SCAT(httpbuf,argbuf) ;
		time(&curtime) ; sprintf(argbuf,"Date: %s",asctime(localtime(&curtime))) ; SCAT(httpbuf,argbuf) ;
		SCAT(httpbuf,"Connection: Close\n") ;
		b = strrchr(passthrufile,'.') ;
		if (b == NULL) { UCstrcpy(ctbuf,UClit("application/unknown")) ; } 
		 else { v_FileExtToContentType(ASCretUC(b),ctbuf,UCsizeof(ctbuf),uerrmsg) ; } ;
/*		If type is HTML then flow back normally */
		if (UCstrstr(ctbuf,UClit("text/html")) != NULL) { resfile = passthrufile ; goto return_to_client ; } ;
		sprintf(argbuf,"Content-Type: %s\n",UCretASC(ctbuf)) ; SCAT(httpbuf,argbuf) ;
		sprintf(argbuf,"Content-Length: %d\n",statbuf.st_size) ; SCAT(httpbuf,argbuf) ;
		b = strrchr((strlen(clientFileName) == 0 ? passthrufile : clientFileName),'\\') ;				/* Parse past all path info */
		if (b == NULL) { b = (strlen(clientFileName) == 0 ? passthrufile : clientFileName) ; }
		 else { b ++ ; } ;
		for(i=0;b[i]!='\0';i++) { if (b[i] == ' ') b[i] = '_' ; } ;	/* Convert spaces to underline */	
		sprintf(argbuf,"Content-Disposition: attachment; filename=%s\n",b) ; SCAT(httpbuf,argbuf) ;
		SCAT(httpbuf,"\n") ;		/* End with blank line */
		send(conSocket,httpbuf,strlen(httpbuf),0) ;
		for(totalsent=0;fp!=NULL;)
		 { bytes = fread(outbuf,1,maxbytes,fp) ;
		   if (bytes <= 0) break ;
		   FD_ZERO(&sset) ; FD_SET(conSocket,&sset) ;
		   tev.tv_sec = SEND_WAIT_SECONDS ; tev.tv_usec = 0 ;
		   res = select(FD_SETSIZE,NULL,&sset,NULL,&tev) ;	/* Wait for ok to send */
		   if (res <= 0)
		    { Log(jobno2,"? Timeout waiting for send-ok(2)") ;
		      goto thread_end ;
		    } ;
		   for(sent=0;sent<bytes;sent+=(res > 0 ? res : 0))
		    { res = send(conSocket,&outbuf[sent],bytes-sent,0) ;
		      totalsent += res ;
		      if (res >= bytes-sent) break ;
		      if (NETERROR == WSAEWOULDBLOCK)
		       { HANGLOOSE(50) ; 
		         Log(jobno2,"Got blocking condition after sending %d bytes...",sent) ;
			 continue ;
		       } ;
		      Log(jobno2,"? Error (%s) (sent = %d, len = %d/%d) in send()",v_OSErrString(NETERROR),sent,maxbytes,res) ;
		      goto thread_end ;
		    } ;
		 } ;
		goto finish_job ;

	   case doResToXLSX:
	   case doResToXL:
//		if (newJobId || jobno1 == UNUSED)
//		 { sprintf(v4rfile,"%s%s.v4r",v4fearg->v4prefix,jobId) ; }
//		 else { sprintf(v4rfile,"%s%d-%d.v4r",v4fearg->v4prefix,jobno1,jobno2) ; } ;
		sprintf(v4rfile,"%s%d-%06d.v4r",v4fearg->v4prefix,urlJobNo1,urlJobNo2) ;
		goto xlp_entry ;
	   case doXLProcess:
//		sprintf(argbuf,v4fearg->excelmask,jobno1,jobno2,v4rfile,excelfile,logfile) ;
xlp_entry:	
		if (v4fearg->excel2mask[0] != '\0')
		 { UCCHAR ucBuf[512] ;
/*		      Arguments: 1=v4rFile, 2=excelFile, 3=logFile, 4=jobId, 5=sesKey, 6=area, 7=cgiLink */
		   if (v4 == doResToXLSX)
		    { v_Msg(NULL,ucBuf,v4fearg->xlsxmask,v4rfile,excelfile,logfile,jobId,sesKey,area,v4fearg->cgiserver) ; }
		    else { v_Msg(NULL,ucBuf,v4fearg->excel2mask,v4rfile,excelfile,logfile,jobId,sesKey,area,v4fearg->cgiserver) ; } ;
		   UCstrcpyToASC(argbuf,ucBuf) ;
		 } else { sprintf(argbuf,v4fearg->excelmask,jobno1,jobno2,v4rfile,excelfile,logfile) ; } ;
		if (v4 == doResToXLSX)
		 { Log(jobno2,"Converting result to XLSX: %s",argbuf) ; }
		 else { Log(jobno2,"Converting result to Excel/BIFF: %s",argbuf) ; } ;
		{ struct V__SpawnArgs sa ; SAARGBUF(argbuf) ;
		  if (!v_SpawnProcess(&sa)) { Log(jobno2,"Error in xv4rpp spawn: %s",UCretASC(uerrmsg)) ; goto thread_end ; } ;
		}
#ifdef LOCALTEST
		goto thread_end ;
#endif

		ifd = open(excelfile,O_RDONLY|O_BINARY) ;
		if (ifd == -1)
		 { Log(jobno2,"Error (%d) opening excel file %s",errno,excelfile) ; goto thread_end ; } ;
		fstat(ifd,&statbuf) ; close(ifd) ;		/* Get size of the file */
		fp = fopen(excelfile,"rb") ;
		if (fp == NULL)
		 { 
		   v4SrvProcError(v4fearg,jobno1,jobno2,conSocket,v4) ; goto finish_job ;
//		   b = "Content-Type: text/html\n\n" ;  send(conSocket,b,strlen(b),0) ;
//		   b = "An error occurred while processing your request.\n" ; send(conSocket,b,strlen(b),0) ;
//		   b = "The following is a log of what transpired-\n" ; send(conSocket,b,strlen(b),0) ;
//		   fp = fopen(logfile,"r") ; appendbr = TRUE ; resfile = logfile ; goto return_to_client ;
		 } ;
		maxbytes = 1024 ;			/* BIFF files always in chunks of 128 */
		ZUS(httpbuf) ;
		Log(jobno2,"Send Excel(%s) buffer size = %d",excelfile,maxbytes) ;
		outbuf = malloc(maxbytes +100) ; olen = 0 ;
		if (msgType == get || msgType == post)		/* If feserver playing HTTP handler then return http header */
		 { sprintf(argbuf,"HTTP/1.1 200 OK\n") ; SCAT(httpbuf,argbuf) ; } ;
		sprintf(argbuf,"Server: V4WWWFEServer %d / V4 %d.%d\n",V4FE_Version,V4IS_MajorVersion,V4IS_MinorVersion) ; SCAT(httpbuf,argbuf) ;
		time(&curtime) ; sprintf(argbuf,"Date: %s",asctime(localtime(&curtime))) ; SCAT(httpbuf,argbuf) ;
		SCAT(httpbuf,"Connection: Close\n") ;
		sprintf(argbuf,"Content-Type: application/octet-stream\n") ; SCAT(httpbuf,argbuf) ;
		sprintf(argbuf,"Content-Length: %d\n",statbuf.st_size) ; SCAT(httpbuf,argbuf) ;
		if (v4 == doResToXLSX)
		 { sprintf(argbuf,"Content-Disposition: attachment; filename=%d-%06d.xlsx\n",jobno1,jobno2) ; SCAT(httpbuf,argbuf) ; }
		 else { sprintf(argbuf,"Content-Disposition: attachment; filename=%d-%06d.xls\n",jobno1,jobno2) ; SCAT(httpbuf,argbuf) ; } ;
		SCAT(httpbuf,"\n") ;		/* End with blank line */
		if (conSocket == NOSOCKET) goto finish_job ;		/* If no socket then nothing to send back */
		send(conSocket,httpbuf,strlen(httpbuf),0) ;

		for(totalsent=0;fp!=NULL;)
		 { bytes = fread(outbuf,1,maxbytes,fp) ;
		   if (bytes <= 0) break ;
		   FD_ZERO(&sset) ; FD_SET(conSocket,&sset) ;
		   tev.tv_sec = SEND_WAIT_SECONDS ; tev.tv_usec = 0 ;
		   res = select(FD_SETSIZE,NULL,&sset,NULL,&tev) ;	/* Wait for ok to send */
		   if (res <= 0)
		    { Log(jobno2,"? Timeout waiting for send-ok(4)") ;
		      goto thread_end ;
		    } ;
		   for(sent=0;sent<bytes;sent+=(res > 0 ? res : 0))
		    { res = send(conSocket,&outbuf[sent],bytes-sent,0) ;
		      totalsent += res ;
		      if (res >= bytes-sent) break ;
		      if (NETERROR == WSAEWOULDBLOCK)
		       { HANGLOOSE(50) ; 
		         Log(jobno2,"Got blocking condition after sending %d bytes...",sent) ;
			 continue ;
		       } ;
		      Log(jobno2,"? Error (%s) (sent = %d, len = %d/%d) in send()",v_OSErrString(NETERROR),sent,maxbytes,res) ;
		      goto thread_end ;
		    } ;
		 } ;
		goto finish_job ;
	 } ;


/*	Open result file & send back via conSocket */
#ifdef LOCALTEST
	goto thread_end ;
#endif
	resfile = ((v4 == doAjax || v4 == doJSONProcess || v4 == doJSONJBT) ? ajaxfile : htmfile) ;
	fp = fopen(resfile,"rb") ;
/*	If we cannot find resulting html file then create one with log results (with error) */
	if (fp == NULL)
	 { char ebuf[1024] ;
//	   fp = fopen(resfile,"w") ;
//	   if (fp == NULL) { Log(jobno2,"Could not create error/log html file") ; goto thread_end ; } ;
	   sprintf(ebuf,"Could not open result file (%s), v4=%d",resfile,v4) ;
	   Log(jobno2,ebuf) ;
	   v4SrvProcError(v4fearg,jobno1,jobno2,conSocket,v4) ; goto finish_job ;
//	   lfp = fopen(logfile,"r") ; 
//	   if (lfp == NULL) { Log(jobno2,"Could not find log file") ; goto thread_end ; } ;
//	   fputs("<html><head><title>Processing Error</title></head><body>\n",fp) ;
//	   fputs("<p>An error occurred while processing your request.<br>The text below is a log of what transpired...</p>",fp) ;
//	   fputs("<table border='0' width='100%'>\n",fp) ;
//	   fputs("<tr><td bgcolor='#E0E0E0'><font face='Arial' size='2'>",fp) ;
//	   for(;;)
//	    { b = fgets(Msg,MsgMaxBytes,lfp) ; if (b == NULL) break ;
//	      strcat(Msg,"<br>") ; fputs(Msg,fp) ;
//	    } ;
//	   fputs("</font></td></tr></table></body></html>",fp) ;
//	   fclose(lfp) ; fclose(fp) ;
//	   fp = fopen(resfile,"rb") ;
//	   if (fp == NULL) { Log(jobno2,"Could not reopen error/log html file") ; goto thread_end ; } ;
	 } ;
return_to_client:		/* Return contents of "fp" to client */
	if (conSocket == NOSOCKET) goto finish_job ;		/* If no socket then nothing to send back */
	maxbytessize = sizeof maxbytes ;
	if (getsockopt(conSocket,SOL_SOCKET,SO_SNDBUF,(char *)&maxbytes,&maxbytessize) != 0) maxbytes = 256 ;
	if (maxbytes > 0xfffff) maxbytes = 8192 ;
	outbuf = malloc(maxbytes +100) ; olen = 0 ;
/*	Get length of results & load outbuf with HTTP headers */
	ZS(httpbuf) ;
	if (msgType == get || msgType == post)		/* If feserver playing HTTP handler then return http header */
	 { sprintf(argbuf,"HTTP/1.1 200 OK\n") ; SCAT(httpbuf,argbuf) ; } ;
	sprintf(argbuf,"Server: V4WWWFEServer %d / V4 %d.%d\n",V4FE_Version,V4IS_MajorVersion,V4IS_MinorVersion) ; SCAT(httpbuf,argbuf) ;
	time(&curtime) ; sprintf(argbuf,"Date: %s",asctime(localtime(&curtime))) ; SCAT(httpbuf,argbuf) ;
	SCAT(httpbuf,"Connection: Close\n") ;
	if (sesKeyCookie[0] != '\0')
	 {sprintf(argbuf,"Set-Cookie: _sesKey=%s\n",sesKeyCookie) ; SCAT(httpbuf,argbuf) ; } ;
	sprintf(argbuf,"Content-Type: TEXT/HTML\n") ; SCAT(httpbuf,argbuf) ;
	stat(resfile,&statbuf) ;
	if (v4 == doAjax && callBack[0] != '\0')	/* If JSON with callback then handle as JSONP - add in callback wrapper */
	 { statbuf.st_size += (strlen(callBack) + 2) ; } ;
	if (statbuf.st_size == 0)			/* Nothing to return - then an error */
	 { Log(jobno2,"?Result file from xv4 is empty") ;
	   v4SrvProcError(v4fearg,jobno1,jobno2,conSocket,v4) ; goto finish_job ;
	 } ;
	sprintf(argbuf,"Content-Length: %d\n",statbuf.st_size) ; SCAT(httpbuf,argbuf) ;
 Log(jobno2,"Send HTML(%s %d bytes) buffer size = %d",resfile,statbuf.st_size,maxbytes) ;
	SCAT(httpbuf,"\n") ;		/* End with blank line */
	if (v4 == doAjax && callBack[0] != '\0')	/* If JSON with callback then handle as JSONP */
	 { strcat(httpbuf,callBack) ; strcat(httpbuf,"(") ; } ;
	send(conSocket,httpbuf,strlen(httpbuf),0) ;
	for(totalsent=0;fp!=NULL;)
	 { 
	   if (appendbr)			/* Got error file - read as text */
	    { if ((b=fgets(Msg,MsgMaxBytes,fp)) == NULL)
	       { if (msgType == unknown) outbuf[olen++] = EOM ; fclose(fp) ; fp = NULL ; }
	       else { strcat(Msg,"<br>") ;
		      for(len=strlen(Msg);len>0;len--) { if (Msg[len-1] >= 20) break ; Msg[len-1] = '\0' ; } ;
		      if ((olen + len) < maxbytes - 5)		/* Build up message about as big as we can go */
		       { strcpy(&outbuf[olen],Msg) ; olen += (len + 1) ; continue ; } ;
		    } ;
	    } else
	    { res = getc(fp) ;
	      if (res == EOF)
	      { if (msgType == unknown) outbuf[olen++] = EOM ; fclose(fp) ; fp = NULL ; }
	      else { outbuf[olen++] = res ; if (olen < maxbytes - 5) continue ;
		   } ;
	    } ;


	   FD_ZERO(&sset) ; FD_SET(conSocket,&sset) ;
	   tev.tv_sec = SEND_WAIT_SECONDS ; tev.tv_usec = 0 ;
	   res = select(FD_SETSIZE,NULL,&sset,NULL,&tev) ;	/* Wait for ok to send */
	   if (res <= 0)
	    { Log(jobno2,"? Timeout waiting for send-ok(5) res=%d errno=%d",res,errno) ; goto thread_end ; } ;


	   for(sent=0;sent<olen;sent+=(res > 0 ? res : 0))
	    { 
	      res = send(conSocket,&outbuf[sent],olen-sent,0) ;
	      totalsent += res ;
	      if (res >= olen-sent) break ;
	      if (NETERROR == WSAEWOULDBLOCK)
	       { HANGLOOSE(50) ; 
	         Log(jobno2,"Got blocking condition after sending %d bytes...",sent) ;
		 continue ;
	       } ;
	      Log(jobno2,"? Error (%s) (sent = %d, len = %d/%d) in send()",v_OSErrString(NETERROR),sent,len,res) ;
	      Msg[0] = EOM ; send(conSocket,Msg,1,0) ;	/* Attempt to end with EOM */
	      goto thread_end ;
	    } ;
	   olen = 0 ;						/* Reset output buffer & init with current Msg */
	   if (appendbr)
	    { strcpy(&outbuf[olen],Msg) ; olen += (len + 1) ; } ;
	 } ; 
	if (v4 == doAjax && callBack[0] != '\0')	/* If JSON with callback then handle as JSONP - append closing paren */
	 { send(conSocket,")",1,0) ; } ;
	Log(jobno2,"Sent %d bytes",totalsent) ;
finish_job:
	Log(jobno2,"Finished processing for thread") ;
	if (keepalive)			/* If we are supposed to keep connection alive then note & return to top form more */
	 { /* Log(jobno2,"Got 'Connection: Keep-Alive' - continue this connection/job for another request") ; */
	   goto keepalive_restart ;
	 } ;
	if (msgType == get && conSocket != NOSOCKET)		/* If HTTP/GET then suck up whatever is leftover (god knows why!) */
	 { 
	   for(ptr=Msg,bol=Msg;;ptr++)
	    { if (TRUE)
	       { FD_ZERO(&sset) ; FD_SET(conSocket,&sset) ;
	         tev.tv_sec = 0 ; tev.tv_usec = 500000 ;
	         res = select(FD_SETSIZE,&sset,NULL,NULL,&tev) ;	/* Wait for something to happen */
	         if (res <= 0)
	          { Log(jobno2,"[Timeout waiting for input]") ; goto thread_end ; } ;
	       } ;
	      res = recv(conSocket,ptr,1,0) ;
	      if (res <= 0)
	       { if (res == 0)
	          { Log(jobno2,"[Remote connection has shut down gracefully (2)]") ;
	          } else { Log(jobno2,"[Error (%s) in recv()]",v_OSErrString(NETERROR)) ; } ;
	         goto thread_end ;
	       } ;
//	      if (!(*ptr == DCHAR || *ptr == '\n')) continue ;
	      if (*ptr != DCHAR) continue ;
	      *ptr = '\0' ; if (*(ptr-1) == '\0') { ptr-- ; continue ; } ;
	      if (strlen(bol) == 0) break ;
	      bol = ptr + 1 ;
	    } ;
	 } ;

/*	All done - SOCKETCLOSE the socket */
thread_end:
	if (conSocket != NOSOCKET) { SOCKETCLOSE(conSocket) ; } ;
	conSocket = UNUSED ; v4fearg->ConnectSocket = UNUSED ;
	if (MsgFree != NULL) free(MsgFree) ;		/* If we had to allocate larger buffer then free it up now */
	if (outbuf != NULL) free(outbuf) ; if (fp != NULL) fclose(fp) ;
/*veh20210923 - if doAjaxQuick then delete working files now */
	if (delWork)
	 { Log(jobno2,"Deleting work files including %s",logfile) ;
	   remove(logfile) ; remove(cmdfile) ;
	   switch (v4)
	    { default:		break ;
	      case doProcess:	remove(htmfile) ; remove(v4rfile) ;
	      case doAjax:	remove(ajaxfile) ; break ;
	    } ;
	 } ;
	return ;
}

void v4SrvProcError(v4fearg,jobno1,jobno2,conSocket,v4)
  struct v4fe__SubProcessArguments *v4fearg ;
  int jobno1,jobno2 ;
  int conSocket ;
  int v4 ;
{ char hdr[256],msg[512] ;

	if (conSocket == NOSOCKET)
	 { Log(jobno2,"An error occurred while processing internal job %d-%d",jobno1,jobno2) ;
	 } else if (v4 == doJSONProcess)
	 { sprintf(msg,"{meta:{status:'error', msg:'An internal error occurred processing job %d-%d. Please try again.'}}\n",jobno1,jobno2) ;
	   sprintf(hdr,"HTTP/1.1 200 OK\ncontent-type: application/json\ncontent-length: %d\n\n",strlen(msg)) ;
	   send(conSocket,hdr,strlen(hdr),0) ; send(conSocket,msg,strlen(msg),0) ;
	 } else
	 { sprintf(msg,"<html><script type='text/javascript'>alert(\"An error occurred while processing job %d-%d\");history.go(-1);</script></html>\n",jobno1,jobno2) ;
	   sprintf(hdr,"HTTP/1.1 200 OK\ncontent-type: text/html\ncontent-length: %d\n\n",strlen(msg)) ;
	   send(conSocket,hdr,strlen(hdr),0) ; send(conSocket,msg,strlen(msg),0) ;
	 } ;
	return ;
}

void v4SrvRet400Error(v4fearg,jobno1,jobno2,conSocket,v4,etext)
  struct v4fe__SubProcessArguments *v4fearg ;
  int jobno1,jobno2 ;
  int conSocket ;
  int v4 ;
  char *etext ;
{ char hdr[256],msg[512] ;

	if (conSocket == NOSOCKET)
	 { Log(jobno2,"An error occurred while processing internal job %d-%d: ",jobno1,jobno2,msg) ;
	 } else if (v4 == doJSONProcess)
	 { sprintf(msg,"{meta:{status:'error', msg:'An internal error occurred processing job %d-%d (%s). Please try again.'}}\n",jobno1,jobno2,etext) ;
	   sprintf(hdr,"HTTP/1.1 400 OK\ncontent-type: application/json\ncontent-length: %d\n\n",strlen(msg)) ;
	   send(conSocket,hdr,strlen(hdr),0) ; send(conSocket,msg,strlen(msg),0) ;
	 } else
	 { sprintf(msg,"<html><h1>\"An invalid request was made in job %d-%d: %s\");history.go(-1);</1>\n",jobno1,jobno2,etext) ;
	   sprintf(hdr,"HTTP/1.1 400 Invalid Request\ncontent-type: text/html\ncontent-length: %d\n\n",strlen(msg)) ;
	   send(conSocket,hdr,strlen(hdr),0) ; send(conSocket,msg,strlen(msg),0) ;
	 } ;
	return ;
}


void v4SrvProcStreamLine(v4fearg,jobno1,jobno2,conSocket,sesKey,jobId,isct,vfsl)
  struct v4fe__SubProcessArguments *v4fearg ;
  int jobno1,jobno2 ;
  int conSocket ;			/* NOTE: if UNUSED then just do a one-way call (e.g. to restart/terminate), if server process not running then don't bother starting it */
  char *sesKey, *jobId, *isct ;
  struct v4fe__StreamLine *vfsl ;
{
  struct V4DPI__DimInfo *di ;
  fd_set sset ; struct timeval tev ;
  struct sockaddr_in sin ;
  struct hostent *hp ;
  int Socket ; char *b, *d, *e ;
  int i,len,chunk,sent,res ; INTMODX intmodx ;
  COUNTER tries ;
  BYTE *mPtr, *rPtr ; LENMAX mBytes, bytesRcv ;
  #define STREAMLINE_MESSAGE_BUF_BYTES 0x8000
  #define STREAMLINE_RETURN_BUF_BYTES 0x8000
  BYTE msgBuf[STREAMLINE_MESSAGE_BUF_BYTES], retBuf[STREAMLINE_RETURN_BUF_BYTES] ;
  UCCHAR ucErrBuf[512] ;

	Socket = UNUSED ; intmodx = UNUSED ; mPtr = NULL ; rPtr = retBuf ;
/*	First format a V4 Message() message */
/*	Format message: id <nl> wantsReply(0/1) <nl> delCalDT <nl> dimension <nl> value  <control-Z> */
	strcpy(msgBuf,jobId) ; strcat(msgBuf,MESSAGE_EODS) ;	/* jobId will be the message id */
	if (conSocket == UNUSED)
	 { strcat(msgBuf,"0" MESSAGE_EODS) ;			/* Don't want a reply */
	 } else { strcat(msgBuf,"1" MESSAGE_EODS) ; } ;		/* Want a reply */
	strcat(msgBuf,"0.0" MESSAGE_EODS) ;			/* null DT */
	strcat(msgBuf,"List" MESSAGE_EODS) ;			/* dimension is LIST */
	strcat(msgBuf,"(sesKey:\"") ; strcat(msgBuf,sesKey) ; strcat(msgBuf,"\" ") ;
	for(b=isct,d = &msgBuf[strlen(msgBuf)];;)
	 { e = strchr(b,DCHAR) ; if (e != NULL) *e = '\0' ;
/*	   Append the next point - dim:"value" - enclose value in quotes */
	   for(;;d++,b++) { *d = *b ; if (*b == ':') break ; } ;
	   b++ ; d++ ; *(d++) = '"' ;
	   for(;*b!='\0';d++,b++) { if (*b == '"') *(d++) = '\\' ; *d = *b ; } ;
	   *(d++) = '"' ; *(d++) = ' ' ;
	   if (e == NULL) { *d = '\0' ; break ; } ;
	   b = e + 1 ; if (*b == '\0') { *d = '\0' ; break ; } ;
	 } ;
	strcat(msgBuf,")" MESSAGE_EOMS) ;			/* Terminate list and message */

	if ((Socket = socket(AF_INET,SOCK_STREAM,0)) < 0) { sprintf(rPtr,"Socket initialization failed (%d)",errno) ; Socket = UNUSED ; goto fail ; } ;
	memset(&sin,0,sizeof sin) ; sin.sin_family = AF_INET ;
	sin.sin_port = htons((u_short)vfsl->portHost) ;
/*	Do we have vanilla host name or something more exotic like www.dot.com */
	if (UCstrchr(vfsl->urlHost,'.') == NULL) { hp = gethostbyname(UCretASC(vfsl->urlHost)) ; }
	 else { UCCHAR *b ;
		if ((b = v_URLAddressLookup(vfsl->urlHost,ucErrBuf)) == NULL)
		 { sprintf(rPtr,"URL lookup/translation failed (%s)",UCretASC(vfsl->urlHost)) ; goto fail ; } ;
		UCstrcpy(vfsl->urlHost,b) ; hp = NULL ;
	      } ;
	if (hp != NULL) { memcpy(&sin.sin_addr,hp->h_addr,hp->h_length) ; }
	 else { sin.sin_addr.s_addr = inet_addr(UCretASC(vfsl->urlHost)) ;
		if (sin.sin_addr.s_addr == -1) { sprintf(rPtr,"Socket initialization (inet_addr) failed (%d)",errno) ; goto fail ; } ;
	      } ;
/*	If connection fails AND we have a command line, then spawn it, wait and retry */
	for(tries=0;tries<20;tries++)
	 { if ((connect(Socket,(struct sockaddr *)&sin,sizeof sin)) >= 0) break ;
	   if (tries > 0) { HANGLOOSE(100) ; continue ; } ;
	   if (UCempty(vfsl->cmdLineRestart) || conSocket == UNUSED) { tries = 999999 ; continue ; } ;
//	   if (!v_SpawnProcess(NULL,vfsl->cmdLineRestart,V_SPAWNPROC_NoWait,ucErrBuf,NULL,NULL,UNUSED)) { sprintf(rPtr,"Initialization SPAWN (%s) failed",UCretASC(vfsl->cmdLineRestart)) ; goto fail ; } ;
	   { struct V__SpawnArgs sa ; memset(&sa,0,sizeof sa) ; sa.argBuf = vfsl->cmdLineRestart ;
	     sa.waitFlags = V_SPAWNPROC_NoWait ; sa.errBuf = ucErrBuf ; sa.fileId = UNUSED ;
	     if (!v_SpawnProcess(&sa)) { Log(jobno2,"Initialization SPAWN (%s) failed",UCretASC(ucErrBuf)) ; goto fail ; } ;
	   }
	   HANGLOOSE(250) ;
	 } 
	if (tries > 20)
	 { sprintf(rPtr,"Could not connect to %s",UCretASC(vfsl->urlHost)) ; goto fail ; } ;
	i = 1 ;
	if (NETIOCTL(Socket,FIONBIO,&i) != 0) { sprintf(rPtr,"NETIOCTL failed (%d)",NETERROR) ; goto fail ; } ;

	len = strlen(msgBuf) ;
	chunk = (len < 4096 ? len : 4096) ;		/* Send message in pieces */
	for(sent=0,res=1,tries=0;tries<100 && res>0&&sent<len;)
	 { res = send(Socket,msgBuf+sent,chunk,0) ; if (res > 0) sent+=res ; 
	   if (res <= 0 && NETERROR == EWOULDBLOCK) { HANGLOOSE(100) ; tries++ ; res = 1 ; continue ; } ;
	   if (chunk > len-sent) chunk = len-sent ; tries = 0 ;
	 } ;
	if (res <= 0 || tries >= 100) { sprintf(rPtr,"Error (%d) send'ing to server",NETERROR) ; goto fail ; } ;
/*	Now read (any) ack back from listener */
	mBytes = STREAMLINE_MESSAGE_BUF_BYTES ; mPtr = msgBuf ;
	for(bytesRcv=0;;)
	 { if (bytesRcv >= mBytes-128)
	    { mBytes *= 1.5 ;
	      if (mPtr == msgBuf) { char *__b = v4mm_AllocChunk(mBytes,FALSE) ; memcpy(__b,mPtr,bytesRcv) ; mPtr = __b ; }
	       else { mPtr = realloc(mPtr,mBytes) ; } ;
	    } ;
	   FD_ZERO(&sset) ; FD_SET(Socket,&sset) ;
	   tev.tv_sec = (bytesRcv == 0 ? v4fearg->initialWait : 1) ; tev.tv_usec = 0 ;	/* Wait a little longer for first byte */
	   res = select(FD_SETSIZE,&sset,NULL,NULL,&tev) ;	/* Wait for something to happen */
	   if (res <= 0) break ;
	   res = recv(Socket,&mPtr[bytesRcv],1024,0) ;
	   if (res <= 0) break ;
	   bytesRcv += res ;					/* Position to last byte received */
	   if (mPtr[bytesRcv-1] == MESSAGE_EOM)
	    { mPtr[bytesRcv-1] = '\0' ; break ;	} ;		/* Got control-z - break */
	 } ;
	mPtr[bytesRcv] = '\0' ;

/*	Incorporate the result into OK ajax pattern */
	if (bytesRcv < STREAMLINE_RETURN_BUF_BYTES - 1024) { rPtr = retBuf ; }
	 else { rPtr = v4mm_AllocChunk(bytesRcv+1024,FALSE) ; } ;
	if (mPtr[0] == '\0') strcpy(mPtr,"?Nothing returned from streamline server?") ;
	if (mPtr[0] == '?') goto fail ;		/* If first character back is '?' then whatever we just did failed - pass back error */


	{ char mask[512] ; LOGICAL instr,done ; char *s, *d ;
/*	  Have to convert ajaxJSONPattern to sprintf mask */
	  for(s=v4fearg->ajaxJSONPattern,d=mask,instr=FALSE,done=FALSE;!done;s++)
	   { if (!instr) { if (*s == '"') instr = TRUE ; continue ; } ;
	     switch (*s)
	      { case '\0':
	        case '\\':	s++ ; *(d++) = *s ; break ;
		case '"':	*(d++) = '\0' ; done = TRUE ; break ; 
		case '%':	if (*(s+1) != '%') *(d++) = *s ; break ;
		case 'U':	*(d++) = (*(s-2) == '%' ? 's' : *s) ; break ;
		default:	*(d++) = *s ; break ;
	      } ;
	   } ;
	  sprintf(rPtr,mask,mPtr) ;
	}
	goto return_JSON ;

fail:
/*	Return error Ajax pattern to caller - rcvBuf must contain error message */
	{ UCCHAR jsErrStr[512] ; char mask[512] ; LOGICAL instr,done ; char *s, *d ;
/*	  Have to convert error message to valid JS string */
	  if (mPtr == NULL) { mPtr = msgBuf ; strcpy(msgBuf,"?Streamline process aborted for unknown reasons") ; } ;
	  mPtr[STREAMLINE_MESSAGE_BUF_BYTES/2] = '\0' ;			/* Make sure error string not too long */
	  v_StringLit(ASCretUC(mPtr),jsErrStr,UCsizeof(jsErrStr),UClit('\''),UClit('\\')) ;
/*	  Have to convert ajaxJSONPattern to sprintf mask */
	  for(s=v4fearg->ajaxErrPattern,d=mask,instr=FALSE,done=FALSE;!done;s++)
	   { if (!instr) { if (*s == '"') instr = TRUE ; continue ; } ;
	     switch (*s)
	      { case '\0':
	        case '"':	*(d++) = '\0' ; done = TRUE ; break ; 
		case '%':	if (*(s+1) != '%') *(d++) = *s ; break ;
		case 'U':	*(d++) = (*(s-2) == '%' ? 's' : *s) ; break ;
		default:	*(d++) = *s ; break ;
	      } ;
	   } ;
	  sprintf(rPtr,mask,"error",UCretASC(jsErrStr)) ;
	}
	goto return_JSON ;

return_JSON:
	if (Socket != UNUSED) SOCKETCLOSE(Socket) ;
	if (conSocket == UNUSED) return ;
/*	Here to send result (in msgBuf) back to caller via Socket */
	{ COUNTER maxBytes, olen, totalsent, tosend ; char *outbuf, httpbuf[1024], argbuf[512] ;
	  time_t curtime ;
	  maxBytes = 4096 ;			/* Increased for better performance - VEH111214 */
	  ZS(httpbuf) ;
	  tosend = strlen(rPtr) ;	/* Get number of bytes to send */
	  Log(jobno2,"Send SL (%d bytes) JSON buffer size = %d",tosend,maxBytes) ;
	  if (tosend < 200)
	   Log(jobno2," Msg: %s",rPtr) ;
	  outbuf = malloc(maxBytes +100) ; olen = 0 ;
	  sprintf(argbuf,"HTTP/1.1 200 OK\n") ; SCAT(httpbuf,argbuf) ;
	  sprintf(argbuf,"Server: V4WWWFEServer %d / V4 %d.%d\n",V4FE_Version,V4IS_MajorVersion,V4IS_MinorVersion) ; SCAT(httpbuf,argbuf) ;
	  time(&curtime) ; sprintf(argbuf,"Date: %s",asctime(localtime(&curtime))) ; SCAT(httpbuf,argbuf) ;
	  SCAT(httpbuf,"Connection: Close\n") ;
	  sprintf(argbuf,"Content-Type: %s\n","text/javascript") ; SCAT(httpbuf,argbuf) ;
	  sprintf(argbuf,"Content-Length: %d\n",tosend) ; SCAT(httpbuf,argbuf) ;
	  SCAT(httpbuf,"\n") ;		/* End with blank line */
	  send(conSocket,httpbuf,strlen(httpbuf),0) ;
	  for(totalsent=0;totalsent<tosend;)
	   { COUNTER packetlen ;
	     FD_ZERO(&sset) ; FD_SET(conSocket,&sset) ;
	     tev.tv_sec = SEND_WAIT_SECONDS ; tev.tv_usec = 0 ;
	     res = select(FD_SETSIZE,NULL,&sset,NULL,&tev) ;	/* Wait for ok to send */
	     if (res <= 0)
	      { Log(jobno2,"? Timeout waiting for send-ok(2)") ;
	        goto cleanup ;
	      } ;
	     packetlen = (maxBytes <= tosend-totalsent ? maxBytes : tosend-totalsent) ;
	     res = send(conSocket,&rPtr[totalsent],packetlen,0) ;
	     totalsent += res ;
	     if (res < packetlen)
	      { if (NETERROR == WSAEWOULDBLOCK)
		 { HANGLOOSE(50) ; Log(jobno2,"Got blocking condition after sending %d bytes...",sent) ; continue ; } ;
	        Log(jobno2,"? Error (%s) (sent = %d, len = %d/%d) in send()",v_OSErrString(NETERROR),tosend,packetlen,res) ;
		goto cleanup ;
	      } ;
	   } ;
	}
cleanup:
	if (mPtr == NULL ? FALSE : mPtr != msgBuf) v4mm_FreeChunk(mPtr) ;
	if (rPtr == NULL ? FALSE : rPtr != retBuf) v4mm_FreeChunk(rPtr) ;
}

/*	v4SrvSubHTTPGet - Converts HTTP/GET to form we like (fields separated by DCHAR, message terminated with EOM) */
enum MSGFILEARGTYPE v4SrvSubHTTPGet(Msg,v4fearg)
  char *Msg ;
  struct v4fe__SubProcessArguments *v4fearg ;
{ int i,j,f ;

	for(i=0;;i++)
	 { if (Msg[i] == ' ') break ;	/* Scan past the initial 'GET ' */
	   if (Msg[i] == '\0') return(none) ;
	 } ;
	for(f=0,i++;;f++,i++)
	 { if (Msg[i] == '?') break ;	/* Up to the arguments */
	   if (Msg[i] == '\0')		/* If not '?' then just a file name */
	    { return(file) ; } ;
	 } ;
/*	Got '?' - parse arguments */

	j = (f > 1 ? i+1 : 0) ;
	for(i++;Msg[i]!='\0'&&Msg[i]!=' '; i++)	/* Convert to format we know & love */
	 { 
	   switch(Msg[i])
	    { default:	Msg[j++] = Msg[i] ; break ;
	      case '+':	Msg[j++] = ' ' ; break ;
	      case '&':	Msg[j++] = DCHAR ; break ;
	      case '%': Msg[j] = x2c(&Msg[i+1]) ; i+=2 ;
			if (Msg[j] == 10 && Msg[j-1] == 13) { Msg[j-1] = '\\' ; Msg[j] = 'r' ; } ;
			j++ ; break ;
	    } ;
	 } ;
	Msg[j++] = EOM ;		/* Terminate with EOM */
	Msg[j++] = '\0' ;
/*	f = length of filename, if gt than one (i.e. more than just a "/") then we have a filename specified also */
	return(f > 1 ? fileAndArgs : args) ;

}

/*	v4SrvSubHTTPPost - Converts HTTP/Post to form we like (fields separated by DCHAR, message terminated with EOM) */
int v4SrvSubHTTPPost(Msg,v4fearg,jobno1,jobno2,mpboundary)
  char *Msg ;
  struct v4fe__SubProcessArguments *v4fearg ;
  int jobno1,jobno2 ;
  char *mpboundary ;			/* If not NULL then multi-part boundary string */
{ FILE *fp ;
  int i,j ;
  char name[128],line[256] ; UCCHAR tfname[V_FileName_Max],sfname[V_FileName_Max] ; char *p, *mp, *sect, *nmp, quote ;
  int out,nx,lx,colon,gotfile,binary ;

	if (mpboundary != NULL) goto do_mpboundary ;
	for(i=0,j=0;Msg[i]!='\0'&&Msg[i]!=' '; i++)	/* Convert to format we know & love */
	 { 
	   switch(Msg[i])
	    { default:	Msg[j++] = Msg[i] ; break ;
	      case '\r': break ;
	      case '\n': break ;
	      case '+':	Msg[j++] = ' ' ; break ;
	      case '&':	Msg[j++] = DCHAR ; break ;
	      case '%': Msg[j] = x2c(&Msg[i+1]) ; i+=2 ;
			if (Msg[j] == 10 && Msg[j-1] == 13) { Msg[j-1] = '\\' ; Msg[j] = 'r' ; } ;
			j++ ; break ;
	    } ;
	 } ;
	Msg[j++] = EOM ;		/* Terminate with EOM */
	Msg[j++] = '\0' ;
	return(TRUE) ;
/*	Here to process multi-part POST (most likely with embedded file data) */
do_mpboundary:
	out = 0 ;				/* out = index for appending reconstructed Msg */
	for(mp=Msg;;)
	 { sect = strstr(mp,mpboundary) ;	/* sect = pointer to next multi-part section */
	   gotfile = FALSE ;			/* Don't have file (yet) */
	   if (sect == NULL) break ;		/* this should not happen */
	   if (strncmp(&sect[strlen(mpboundary)],"--",2) == 0)
	    break ;				/* Hit end of line - quit */
/*	   Here to handle next section of multi-part */
	   mp = sect + strlen(mpboundary) ; for(;;mp++) { if (*mp != '\r' && *mp != '\n') break ; } ;
	   for(;;)				/* Parse all headers for this section */
	    { for(lx=0,colon=FALSE;;mp++)
	       { 
	         if (*mp == '\r') continue ;	/* Ignore CR */
	         if (*mp == '\n') break ;	/* end of header line */
		 if (*mp == ':') colon = TRUE ;
	         line[lx++] = (colon ? *mp : toupper(*mp)) ;
	       } ; line[lx++] = '\0' ;		/* At this point line has next header line, up to colon is UpperCase */
	      if (strlen(line) == 0) break ;	/* Blank line indicates end of headers */
	      if (strncmp(line,"CONTENT-TYPE",12) == 0)
	       { 
	       } ;
	      if (strncmp(line,"CONTENT-DISPOSITION",19) == 0)
	       { p = strstr(line,"name=") ; if (p == NULL) p = strstr(line,"NAME=") ;
	         if (p != NULL)
		  { p += 5 ; quote = *p ;	/* Look for double or single quote to enclose name */
		    if (quote == '"' || quote == '\'') { p ++ ; } else { quote = '\0' ; } ;
		    for(nx=0;;p++)
		     { if (quote != '\0' ? *p == quote : (*p == ' ' || *p == ';' || *p < 20)) break ;
		       name[nx++] = *p ;
		     } ; name[nx++] = '\0' ;
		    for(nx=0;name[nx]!='\0';nx++) /* Now append name to Msg */
		     { Msg[out++] = name[nx] ; } ;
		    Msg[out++] = '=' ;		/* End name with an equal */
		  } ;
/*		 Search for filename="xxx" - but don't assume file if name (xxx) is empty/zero-length */
		 if ((p = strstr(line,"filename=")) != NULL ? TRUE : (p = strstr(line,"FILENAME=")) != NULL)
		  { if (strstr(p,"=''") == NULL && strstr(p,"=\"\"") == NULL)
		     { gotfile = TRUE ;
		       p = strstr(p,"=") ; p++ ; quote = *p ; p++ ;
/*		       Copy original (source) file name into sfname (enclosed in double-quotes) */
		       sfname[0] = '"' ; for(i=1;;p++) { if (*p == quote) break ; sfname[i++] = *p ; } ;
		       sfname[i++] = '"' ; sfname[i++] = '\0' ;
		     } ;
		  } ;
	       } ;
/*	      Begin of next line */
	      mp = strchr(mp,'\n') ; if (mp == NULL) mp = "\n\n\n" ; mp++ ;
	    } ;
/*	   Here to handle data portion of this section */
	   nmp = strstr(mp,mpboundary) ;	/* sect = pointer to begin of next section */
	   binary = (nmp == NULL) ;		/* If not found then we got binary data - handle differently from text */
/* ********** This is very poor - need to dynamically allocate Msg buffer, get proper byte length for memchr below */
	   if (binary)			/* If not found then probably binary data - keep looking */
	    { char *temp ; int blen = strlen(mpboundary), res ;
	      for(temp=mp;;temp++)
	       { if (*temp != *mpboundary) continue ;
	         if ((res=strncmp(mpboundary,temp,blen)) == 0) { nmp = temp ; break ; } ;
	       } ;
	    } ;
	   if (*(nmp-1) == '\n') nmp-- ;
	   if (*(nmp-1) == '\r') nmp-- ;
/*	   From mp..nmp is our data */
	   for(;;mp++) { if (*mp != '\r' && *mp != '\n') break ; } ;
/*	   If an embedded file then grab it & write to temp file, then substitute the temp file name as value */
	   if (gotfile)
	    { 
	      v_MakeOpenTmpFile(UClit("vsrv"),tfname,UCsizeof(tfname),NULL,NULL) ;
//	      TEMPFILE(tfname,"vsrv") ;
	      if (UCstrchr(tfname,'.') == NULL) UCstrcat(tfname,UClit(".tmp")) ;
	      fp = fopen(UCretASC(tfname),(binary ? "wb" : "w")) ;
	      if (fp == NULL) { Log(jobno2,"Error (%d) opening temp file: %s",errno,UCretASC(tfname)) ; }
	       else { if (binary)
		       { fwrite(mp,1,nmp-mp,fp) ; mp = nmp ;
		       } else
		       { for(;nmp-mp > 0;mp++)
		          { 
#ifdef WINNT
		            if (*mp == '\r') continue ;		/* WINNT fputc appears to add '\r' before '\n' */
#endif
		            fputc(*mp,fp) ;
			  } ;
		       } ;
		      fclose(fp) ;
		    } ;
/*	      Copy real name in quotes followed by comma & then source file in quotes */
	      Msg[out++] = '"' ; for(i=0;tfname[i]!='\0';i++) { Msg[out++] = tfname[i] ; } ;
	      Msg[out++] = '"' ; Msg[out++] = ',' ; for(i=0;sfname[i]!='\0';i++) { Msg[out++] = sfname[i] ; } ;
	    } else
	    { for(;nmp-mp > 0;mp++) { Msg[out++] = *mp ; } ;
	    } ;
	   Msg[out++] = DCHAR ;
	 } ;
	Msg[out++] = EOM ;			/* Terminate with EOM */
	Msg[out] = '\0' ;			/* Terminate with null character */
	return(TRUE) ;
}


/*	v4SrvReturnFile - Returns a file (e.g. response to GET of an image or whatever) */
LOGICAL v4SrvReturnFile(file,v4fearg,jobno1,jobno2,conSocket,msgType,webFile)
  char *file ;
  struct v4fe__SubProcessArguments *v4fearg ;
  int jobno1,jobno2 ;
  int conSocket ;
  enum MSGTYPE msgType ;
  LOGICAL webFile ;	/* TRUE if a web file and name s/b checked, FALSE if a local file to immediately return */
{ FILE *fp ;
  fd_set sset ;
  struct stat statbuf ;
  time_t curtime ;
  struct timeval tev ;
  UCCHAR contenttype[128],uerrbuf[512],*ufp ; char argbuf[1024],httpbuf[1024],*b ;
  char filebuf[V_FileName_Max] ;
  int i,j,maxbytes,maxbytessize,totalsent,res,sent,bytes,ok,loop ;
  char outbuf[8192+64] ;

/*	Loop thru file name converting '%hh' to corresponding character, also convert "/" to "\" for windows */
	if (webFile)
	 { for(i=0,j=0;j<V_FileName_Max;i++)
	    { switch (file[i])
	       { default:		filebuf[j++] = file[i] ; break ;
#ifdef WINNT
		 case '/':	if (i == 0) continue ;				/* For security reasons, don't allow absolute paths, strip of leading "/" */
				filebuf[j++] = '\\' ; break ;
		 case '\\':	if (i != 0) filebuf[j++] = file[i] ; break ;
#endif
		 case '%':		filebuf[j++] = x2c(&file[++i]) ; break ;
	       } ; if (file[i] == 0) break ;
	    } ;
	   if (file[i] != 0)
	    { filebuf[100] = 0 ; strcat(filebuf,"...") ;
	      Log(jobno2,"Invalid file specification - filename too long: %s",filebuf) ;
	      b = "HTTP/1.1 404 Invalid file name - not found\n\n" ; send(conSocket,b,strlen(b),0) ;
	      return(FALSE) ;
	    } ;
/*	   Check to see if we have a logical device, if single letter then do not allow, for security only allow v3/v4 type logicals (of at least 3 characters) */
	   ok = TRUE ; loop = TRUE ;
	   for(i=0;filebuf[i]!='\0'&&loop;i++)
	    { switch (filebuf[i])
	       { case '/':		loop = FALSE ; break ;
		 case ':':		if (i < 3) ok = FALSE ; loop = FALSE ; break ;
	       } ;
	    } ;
	   if (!ok)
	    { Log(jobno2,"Invalid file specification - unsecure device/logical: %s",filebuf) ;
	      b = "HTTP/1.1 401 Unauthorized request\n\n" ; send(conSocket,b,strlen(b),0) ;
	      return(FALSE) ;
	    } ;
	 } else		/* Else we have a local file to return immediately - don't check anything */
	 { strcpy(filebuf,file) ;
	 } ;
/*	Scan through V4HTMLContentType.v4i to determine content type based on file's extension */
	b = strrchr(filebuf,'.') ;
	if (b == NULL)
	 { Log(jobno2,"File (%s) has no extension, cannot determine Content-Type",filebuf) ;
	   b = "HTTP/1.1 404 File not found\n\n" ; send(conSocket,b,strlen(b),0) ;
	   return(FALSE) ;
	 }
	if (strcmp(b,".jpg") == 0) { UCstrcpy(contenttype,UClit("image/jpeg")) ; }
	 else if (strcmp(b,".gif") == 0) { UCstrcpy(contenttype,UClit("image/gif")) ; }
	 else if (strcmp(b,".js") == 0) { UCstrcpy(contenttype,UClit("application/x-javascript")) ; }
	 else if (strcmp(b,".css") == 0) { UCstrcpy(contenttype,UClit("text/css")) ; }
	else if (!v_FileExtToContentType(ASCretUC(b),contenttype,UCsizeof(contenttype),uerrbuf))
	 Log(jobno2,"Error determining Content-Type: %s",uerrbuf) ;
//	stat(v3_logical_decoder(filebuf,TRUE),&statbuf) ;
#ifdef WINNT
	EnterCriticalSection(&CS) ;
#endif
	ufp = v_UCLogicalDecoder(ASCretUC(filebuf),VLOGDECODE_Exists,0,uerrbuf) ;
	if (ufp == NULL)
	 { Log(jobno2,"Error decoding logical for file (%s) - %s",filebuf,UCretASC(uerrbuf)) ;
	   b = "HTTP/1.1 404 File not found\n\n" ; send(conSocket,b,strlen(b),0) ;
#ifdef WINNT
	   LeaveCriticalSection(&CS) ;
#endif
	   return(FALSE) ;
	 } ;
	stat(UCretASC(ufp),&statbuf) ;

//	fp = fopen(v_LogicalDecoder(filebuf,TRUE,errbuf),"rb") ;
	fp = UCfopen(ufp,"rb") ;
	if (fp == NULL)
	 { Log(jobno2,"Error (%d) opening file (%s)",errno,UCretASC(ufp)) ;
	   b = "HTTP/1.1 404 File not found\n" ; send(conSocket,b,strlen(b),0) ;
	   b = "Context-Type: text/html\n" ; send(conSocket,b,strlen(b),0) ;
	   sprintf(argbuf,"<html><body>The requested file (%s) was not found</body></html>\n",filebuf) ;
	   sprintf(httpbuf,"Content-Length: %d\n\n",strlen(argbuf)) ; send(conSocket,httpbuf,strlen(httpbuf),0) ;
	   send(conSocket,argbuf,strlen(argbuf),0) ;

#ifdef WINNT
	   LeaveCriticalSection(&CS) ;
#endif
	   return(FALSE) ;
	 }
	maxbytes = (sizeof outbuf) - 64 ; ZS(httpbuf) ;
//	outbuf = malloc(maxbytes +100) ;
	if (msgType == get || msgType == post)		/* If feserver playing HTTP handler then return http header */
	 { sprintf(argbuf,"HTTP/1.1 200 OK\n") ; SCAT(httpbuf,argbuf) ; } ;
	sprintf(argbuf,"Server: V4WWWFEServer %d / V4 %d.%d\n",V4FE_Version,V4IS_MajorVersion,V4IS_MinorVersion) ; SCAT(httpbuf,argbuf) ;
	time(&curtime) ; sprintf(argbuf,"Date: %s",asctime(localtime(&curtime))) ; SCAT(httpbuf,argbuf) ;
	SCAT(httpbuf,"Connection: Close\n") ;
/*	Cache static items for 2 hours */
	sprintf(argbuf,"Expires: %s\n",(char *)v_FormatHTTPDateTime(UNUSED,60*60*2)) ; SCAT(httpbuf,argbuf) ;
	sprintf(argbuf,"Content-Type: %s\n",UCretASC(contenttype)) ; SCAT(httpbuf,argbuf) ;
	sprintf(argbuf,"Content-Length: %d\n",statbuf.st_size) ; SCAT(httpbuf,argbuf) ;
	SCAT(httpbuf,"\n") ;	/* End with blank line */
	Log(jobno2,"Send file(%s) Content-Length: %d, Content-Type: %s (maxbytes=%d)",filebuf,statbuf.st_size,UCretASC(contenttype),maxbytes) ;
	send(conSocket,httpbuf,strlen(httpbuf),0) ;
	for(totalsent=0;fp!=NULL;)
	 { bytes = fread(outbuf,1,maxbytes,fp) ;
	   if (bytes <= 0)
	    { if (totalsent < statbuf.st_size) { Log(jobno2,"???????? Read error after %d bytes - will retry",totalsent) ; HANGLOOSE(50) ; continue ;} ;
	      break ;
	    } ;
	   FD_ZERO(&sset) ; FD_SET(conSocket,&sset) ;
	   tev.tv_sec = SEND_WAIT_SECONDS ; tev.tv_usec = 0 ;
	   res = select(FD_SETSIZE,NULL,&sset,NULL,&tev) ;	/* Wait for ok to send */
	   if (res <= 0)
	    { Log(jobno2,"? Timeout waiting for send-ok on job(6), res=0, error=%d",res,NETERROR) ;
	      goto end ;
	    } ;
	   for(sent=0;sent<bytes;sent+=(res > 0 ? res : 0))
	    { 
	      res = send(conSocket,&outbuf[sent],bytes-sent,0) ;
	      if (res > 0) totalsent += res ;
	      if (res >= bytes-sent) break ;
	      HANGLOOSE(50) ;				/* Could not send all of it- wait and then keep trying */
	      if (NETERROR == WSAEWOULDBLOCK)
	       { Log(jobno2,"Got blocking condition after sending %d bytes...",sent) ; continue ; }; 
	      if (res >= 0) continue ;
	      Log(jobno2,"? Error (%s) (sent = %d, len = %d/%d) in send() for job (%d-%d)",v_OSErrString(NETERROR),sent,maxbytes,res,jobno1,jobno2) ;
	      goto end ;
	    } ;
	 } ;

end:
#ifdef WINNT
	LeaveCriticalSection(&CS) ;
#endif
	if (fp != NULL) fclose(fp) ;
	Log(jobno2,"Total bytes sent in file(%s) = %d",filebuf,totalsent) ;
	return(TRUE) ;
}


/*	x2c - Converts two characters to corresponding hex number */
int x2c(char *what)
{ int digit;

	digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A')+10 : (what[0] - '0')) * 16 ;
	digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A')+10 : (what[1] - '0'));
	return(digit);
}


/*	Log - Logs message to stdout & date dependent log file */

char *logFileName()
{ SYSTEMTIME st ;
  static char filename[V_FileName_Max] ;
  
	GetLocalTime(&st) ;
	sprintf(filename,"%sv4wwwserver%04d%02d%02d%s.log",logdirectory,st.wYear,st.wMonth,st.wDay,wfSequenceSuffix) ;
	return(filename) ;
}

static int fCnt=0, mCnt=0 ;		/* fCnt = number logs actually filed, mCnt = number of log messages */
#define LOGMSG_QUEUE_MAX 512		/* Allow queue (backlog of un-filed message) to grow to 512 */
static char *lMsgs[LOGMSG_QUEUE_MAX] ;	/* Log message queue */
static int lIds[LOGMSG_QUEUE_MAX] ;	/* Associated log Id numbers */

void Log(id,msg)
   int id ;
   char *msg ;
{ SYSTEMTIME st ;
  FILE *fp ;
  va_list ap ;
  static char Months[37]="JanFebMarAprMayJunJulAugSepOctNovDec" ;
  char timebuf[50],lbuf[2048], Month[4] ; int i ;
  INDEX qi ;

#ifdef WINNT
	if (!initCS) { initCS = TRUE ; InitializeCriticalSection(&CS) ; } ;
	if (id != UNUSED) EnterCriticalSection(&CS) ;
	qi = (mCnt+1) % LOGMSG_QUEUE_MAX ;
	if (lMsgs[qi] == NULL) lMsgs[qi] = malloc(2048) ;
#endif
	va_start(ap,msg) ; vsprintf(lMsgs[qi],msg,ap) ; va_end(ap) ;
	lIds[qi] = id ;
	mCnt++ ;
#ifdef WINNT
	if (id != UNUSED) LeaveCriticalSection(&CS) ;
#endif
	return ;


/*	Do special formatting on error message */
	va_start(ap,msg) ; vsprintf(lbuf,msg,ap) ; va_end(ap) ;
	for(i=0;lbuf[i]!='\0';i++) { if (lbuf[i] < 20) lbuf[i] = '.' ; } ;
	GetLocalTime(&st) ;
	strncpy(Month,&Months[(st.wMonth-1)*3],3) ; Month[3] = '\0' ;
	if (id > 0)
	 { sprintf(timebuf,"%02d-%3s %02d:%02d:%02d-%d",st.wDay,Month,st.wHour,st.wMinute,st.wSecond,id) ;
	 } else { sprintf(timebuf,"%02d-%3s %02d:%02d:%02d",st.wDay,Month,st.wHour,st.wMinute,st.wSecond) ; } ;
//	if (id != UNUSED) printf("%s %s\n",timebuf,lbuf) ;
	cLog(TRUE,"%s %s\n",timebuf,lbuf) ;
	fp = fopen(logFileName(),"a") ;			/* Open up log file */
	if (fp != NULL)
	 { fprintf(fp,"%s %s\n",timebuf,lbuf) ; fclose(fp) ; } ;
#ifdef WINNT
	if (id != UNUSED) LeaveCriticalSection(&CS) ;
#endif
}

FILE *fpLog = NULL ;
#define logThreadHangTime 250
/*	Use this instead of endProcess() so we close off logging cleanly */
void endProcess(status)
{
	HANGLOOSE((logThreadHangTime * 2)) ;
	for(;fpLog!=NULL;) { HANGLOOSE(logThreadHangTime) ; } ;

	exit(status) ;
}


void loggingThread()
{ SYSTEMTIME st ;
  static char Months[37]="JanFebMarAprMayJunJulAugSepOctNovDec" ;
  LOGICAL logFileOpen = FALSE ;
  INDEX qx,qi,idle,ocnt ; char month[4], timebuf[50] ;
  
/*	This is basically a continuous loop waiting for stuff to log to the 'current' file */
	idle = 0 ;
	for(;;)
	 { if (fCnt >= mCnt)
	    { 
/*	      If we are idle for a while then close off the log file */
	      idle++ ;
	      if (idle > 10 && fpLog != NULL)
	       { fclose(fpLog) ; fpLog = NULL ; } ;
	      HANGLOOSE(logThreadHangTime) ; continue ;
	    } ;
	   GetLocalTime(&st) ; idle = 0 ;
	   strncpy(month,&Months[(st.wMonth-1)*3],3) ; month[3] = '\0' ;
	   if (fpLog == NULL) { ocnt = 0 ; fpLog = fopen(logFileName(),"a") ; } ;
	   for(qx=fCnt+1;qx<=mCnt;qx++)
	    { qi = qx % LOGMSG_QUEUE_MAX ;
	      if (lIds[qi] > 0)
	       { sprintf(timebuf,"%02d-%3s %02d:%02d:%02d-%d",st.wDay,month,st.wHour,st.wMinute,st.wSecond,lIds[qi]) ;
	       } else { sprintf(timebuf,"%02d-%3s %02d:%02d:%02d",st.wDay,month,st.wHour,st.wMinute,st.wSecond) ; } ;
	      cLog(TRUE,"%s %s\n",timebuf,lMsgs[qi]) ;
	      if (fpLog != NULL) { fprintf(fpLog,"%s %s\n",timebuf,lMsgs[qi]) ; }
	       else { printf("%s %s\n",timebuf,lMsgs[qi]) ; } ;
	      fCnt++ ; ocnt++ ;
	    } ;
	   if (ocnt > 25 && fpLog != NULL)		/* If we have written out a bunch of lines then close file */
	    { fclose(fpLog) ; fpLog = NULL ; } ;
	 } ;
}


/*	cLog - Logs message to stdout */

void cLog(nest,msg)
   int nest ;
   char *msg ;
{ SYSTEMTIME st ;
  FILE *fp ;
  va_list ap ;
  char timebuf[50],lbuf[2048] ; int i ;


/*	Do special formatting on error message */
	if (!initCS) { initCS = TRUE ; InitializeCriticalSection(&CS) ; } ;
#ifdef WINNT
	if (!nest) EnterCriticalSection(&CS) ;
#endif
	va_start(ap,msg) ; vsprintf(lbuf,msg,ap) ; va_end(ap) ;
	for(i=0;lbuf[i]!='\0';i++) { if (lbuf[i] < 20) lbuf[i] = '.' ; } ;
	if (i > MAXLINELEN) lbuf[MAXLINELEN] = '\0' ;
	strcpy(lineBuf[curX & LINESINDEXMASK],lbuf) ; curX++ ;
	if (logVerbose) puts(lbuf) ;
	
#ifdef WINNT
	if (!nest) LeaveCriticalSection(&CS) ;
#endif
}


//hOutput = GetStdHandle(STD_OUTPUT_HANDLE) ;
//WriteFile(oHandle,bp,len,olenptr,NULL)
//FreeConsole()