/*	V4DEFS.C - V4 Definitions

	Created 30-Mar-92 by Victor E. Hansen			*/

#ifdef WINNT
#define NEWMMM 1
#endif

#ifndef _DidV4Defs

#include <errno.h>
#include <string.h>
#include <wchar.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <stdlib.h>
#include "vconfig.c"		/* Pull in platform specific parameters */
#ifdef WINNT
#include <windows.h>
#pragma warning(disable : 4996)		/* 'function': was declared deprecated */
//#pragma warning(disable : 4115)		/* named type definition in parentheses */
#pragma warning(disable : 4214)		/* nonstandard extension used : bit field types other than int */
#pragma warning(disable : 4131)		/* uses old-style declarator */
//#pragma warning(error : 4101)		/* unreferenced local variable */
//#pragma warning(disable : 4102)		/* unreferenced label */
#pragma warning(disable : 4244)		/* conversion from 'xxx' to 'yyy', possible loss of data */
#pragma warning(disable : 4018)		/* signed/unsigned mismatch */
//#pragma warning(disable : 4013)		/* 'xxx' undefined; assuming extern returning int */
#pragma warning(disable : 4057)		/* 'xxx *' differs in indirection to slightly different base types from 'xxx *' */
//#pragma warning(disable : 4701)		/* potentially uninitialized local variable 'xxx' used */
//#pragma warning(disable : 4127)		/* conditional expression is constant */
#pragma warning(disable : 4389)		/* ==' : signed/unsigned mismatch */
//#pragma warning(disable : 4313)		/* '%x' in format string conflicts with argument 2 of type 'yyy' */
#pragma warning(disable : 4267)		/* conversion from 'xx' to 'yy', possible loss of data */
#endif

#define XDICTMU 1			/* If defined then compile multi-user (concurrent) access to External dictionary */

#ifdef WANTODBC
#include <sql.h>
#include <sqlext.h>
#endif

#ifdef WANTMYSQL
#ifdef UNIX
#include <mysql/mysql.h>
#else
#include <mysql.h>	/* Additional include directory - c:\Program Files\MySQL\MySQL Server 5.0\include */
// "c:\Program Files\MySQL\MySQL Server 5.0\lib\opt\libmysql.lib"
#endif
#endif

#ifdef WANTORACLE
#include "v4ocilibh.c"		/* NOTE - this is ocilib.h renamed so that it 'travels' with the V4 source code. See http://orclib.sourceforge.net/download/ */
/*
Uncompress the archive (ocilib-x.y.z-windows.zip)
Copy ocilib\include\ocilib.h to a folder listed in the compiler headers folders
Copy ocilib[32|64][x].lib to a folder listed in the linker libraries folders
Copy ocilib[32|64][x].dll to a folder included in the PATH environment variable
[x] is the compiled version of OCILIB ('a' -> ANSI, 'w' -> Unicode, 'm' -> Mixed)

To use OCILIB in a project :

include "ocilib.h" in your application
define call convention (OCI_API) to __stdcall
define charset mode (OCI_CHARSET_ANSI | OCI_CHARSET_MIXED| OCI_CHARSET_WIDE | OCI_CHARSET_UFT8)
*/
#endif

#ifndef FILE
#include <stdio.h>
#include <setjmp.h>

#ifdef ALTVARGS
#include <varargs.h>		/* Funky include for SCO */
#else
#include <stdarg.h>
#endif

#ifdef POSIX
#ifdef INCSYS
#include <sys/types.h>
#ifdef UNIX
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <glob.h>
#endif
#else
#include <types.h>
#ifdef UNIX
#include <ipc.h>
#include <shm.h>
#include <sem.h>
#include <glob.h>
#endif
#endif /* ifdef INCSYS */
#endif /* ifdef POSIX */

#endif /* ifndef FILE */

#include "v4regexpdefs.c"

//#define NULLID 0	/* Null DPI Id */
#undef TRUE
#undef FALSE
#define TRUE (LOGICAL)1
#define FALSE (LOGICAL)0

/*	Useful MACROS						*/

#define ALIGN(number) ( (number+3)&0xFFFFFFFC )
#define ALIGN64(number) ( (number+7)&0xFFFFFFF8 )
#define ALIGNDBL(NUM) ((NUM+7) & 0xfffffff8)
#define ALIGNL(number) ( (number+( sizeof (BYTE *) - 1 )) & ( ~((sizeof (BYTE *)) - 1 )) )
#define IFTRACEAREA if (acb->RootInfo->NextAvailUserNum == 5)
#define CLEARCACHE {}			/* Don't want it to do anything now, used to do above!

/*	Common Bucket Header Format				*/

#define V4_BktHdrType_SeqOnly 0 	/* Sequential Only File */
#define V4_BktHdrType_Root 1		/* Root bucket */
#define V4_BktHdrType_Index 2		/* Directory bucket */
#define V4_BktHdrType_Data 3		/* Data bucket */
#define V4_BktHdrType_Unused 0		/* Unused bucket (same as SeqOnly - should not be a problem since SeqOnly and Indexed are mutually exclusive) */

#ifdef USEINUSE
#define IBHINUSE ibh->sbh.InUse --
#define NIBHINUSE nibh->sbh.InUse --
#define DBHINUSE dbh->sbh.InUse --
#define SBHINUSE sbh->InUse --
#else
#define IBHINUSE
#define NIBHINUSE
#define DBHINUSE
#define SBHINUSE
#endif

typedef unsigned char BYTE ;		/* Used for non-character oriented byte allocation/addressing */
typedef char SBYTE ;			/* Used for non-character oriented byte numerics (i.e. it is signed) */
#define UBYTE unsigned char		/* Unsigned     byte (1 byte )        */
#define UWORD unsigned int		/* Make all these 32 bit int's */
#define ULONG unsigned int		/* Make all these int's */
typedef double CALENDAR ;		/* A Calendar value */

typedef int LOGICAL ;
typedef int INDEX ;
typedef int COUNTER ;
typedef int ETYPE ;			/* Holds type code */
typedef int LENMAX ;			/* Length or Max length of string, buffer, etc. */
typedef unsigned short  U16INT;         /*  Alternative for double-byte      */
typedef int HASH32 ;			/* 32 Bit Hash Value */
typedef int CCOUNT ;			/* Character Count */
typedef int FILEID ;			/* V4 Stream fileId */
typedef int VSTREAM ;			/* V4 stream id */
typedef int SYMID ;			/* Symbol Id */
typedef int VTRACE ;			/* More or less globally used trace flag (see V4TRACE_xxx) */

typedef int FILEFD ;			/* Unix/C file descriptor */
typedef int SOCKETNUM ;			/* Unix/C socket number */
typedef int SOCKETPORT ;
typedef int DATAID ;			/* V4IS DataId */
typedef int AREAID ;			/* V4IS Area Id */
typedef int FLAGS32 ;			/* 32-bit flag word */
typedef int V4LOCKX ;			/* Lock index */

typedef int DIMID ;			/* Dimension Id (allocated as int, but in reality cannot exceed 16 bits) */
typedef int DICTID ;			/* Dictionary id ref */
typedef short ACTDIMID ;		/* 16 bit version to be used where really critical that exact length used */
typedef int PNTTYPE ;			/* Point type */
typedef int UDTVAL ;			/* Universal date-time value */
typedef int INTMODX ;			/* IntMod index */
typedef int TAGVAL ;			/* 32-bit tag-flags,,tag-value */
typedef int FRAMEID ;			/* Context frame id */
typedef int BRKPTID ;			/* Breakpoint Id */
typedef int MIDASODBCHANDLE ;		/* MIDAS 'ODBC' Handle */
typedef int UNIQUEID ;			/* Process/thread/Lock Id (always > 0) */
typedef int V4MSEGID ;			/* V4 Memory Segment Id */
typedef char SESKEY[32+1] ;		/* Xlib Session key (32 bytes + terminating null) */
#ifdef V4UNICODE
typedef wchar_t USESKEY[32+1] ;		/* Xlib Session key (32 bytes + terminating null) */
#else
typedef char USESKEY[32+1] ;		/* Xlib Session key (32 bytes + terminating null) */
#endif

typedef int XDBID ;			/* Database ID ODBC/MYSQL - Connection/Handle */
typedef int XDBROW ;			/* XDB row number (want to be able to handle HUGE tables!) */
typedef int XDBRECID ;			/* XDB record id */

typedef unsigned int BITS ;		/* A sub-word allocation */
typedef int SBITS ;			/* A SIGNED sub-word allocation */

#define UINT8  unsigned char
#define UINT32 unsigned long int

#ifdef WINNT
#define TXTEOL "\r\n"			/* Windows text file EOL */
#else
#define TXTEOL "\n"			/* non Windows text file EOL */
#endif

/*	U N I C O D E   S U P P O R T	*/

#ifdef V4UNICODE
#define V4RUNTIMEINFOFILEPREFIX UClit("v4runtimeinfo")
typedef wchar_t UCCHAR ;
#define UClit(str) L ## str
#define UCEOS L'\0'			/* End of string */
#define UCNL L'\n'			/* New line */
#define UCEOLbt L'\r'
#define UCEOLbts L"\r"
#ifdef WINNT
#define UCTXTEOL L"\r\n"		/* Windows text file EOL */
#else
#define UCTXTEOL L"\n"			/* non Windows text file EOL */
#endif
#define UCempty(ucstr) (ucstr[0] == UCEOS)
#define UCnotempty(ucstr) (ucstr[0] != UCEOS)
#define UCstrlen(ucstr) ((LENMAX)wcslen(ucstr))
#define UCstrchr(ucstr,charlit) wcschr(ucstr,UClit(charlit))
#define UCstrchrV(ucstr,charvar) wcschr(ucstr,charvar)
#define UCstrrchr wcsrchr
#define UCstrcmp(ucstr1,ucstr2) wcscmp(ucstr1,ucstr2)
#define UCstrncmp(ucstr1,ucstr2,slen) wcsncmp(ucstr1,ucstr2,slen)
#define UCstrcpy(ucstr1,ucstr2) wcscpy(ucstr1,ucstr2)
#define UCstrcat(ucstr1,ucstr2) wcscat(ucstr1,ucstr2)
#define UCstrncat(ucstr1,ucstr2,len) wcsncat(ucstr1,ucstr2,len)
#define UCstrncpy(ucstr1,ucstr2,len) wcsncpy(ucstr1,ucstr2,len)
#define UCcnvupper(ucdst,ucsrc,max) { INDEX _i_ ; for(_i_=0;_i_<(max)-1;_i_++) { (ucdst)[_i_] = UCTOUPPER((ucsrc)[_i_]) ; if ((ucsrc)[_i_] == UCEOS) break ; } ; (ucdst)[(max)-1] = UCEOS ; } ;
#define UCcnvlower(ucdst,ucsrc) { INDEX _i_ ; for(_i_=0;;_i_++) { ucdst[_i_] = UCTOLOWER(ucsrc[_i_]) ; if (ucsrc[_i_] == UCEOS) break ; } ; } ;
#define UCTOUPPER(ucchar) (uci->Entry[ucchar].IsLower ? uci->Entry[ucchar].OtherValue : ucchar)
#define UCSTRTOUPPER(ucstr) { UCCHAR *_ptr_ ; INDEX _i_ ; _ptr_ = ucstr ; for(_i_=0;_ptr_[_i_]!=UClit('\0');_i_++) { _ptr_[_i_] = UCTOUPPER(_ptr_[_i_]) ; } ; }
#define UCTOLOWER(ucchar) (uci->Entry[ucchar].IsUpper ? uci->Entry[ucchar].OtherValue : ucchar)
#define UCSTRTOLOWER(ucstr) { UCCHAR *_ptr_ ; INDEX _i_ ; _ptr_ = ucstr ; for(_i_=0;_ptr_[_i_]!=UClit('\0');_i_++) { _ptr_[_i_] = UCTOLOWER(_ptr_[_i_]) ; } ; }
#define UCsscanf swscanf
#define UCstrspn(ucstr,lit) wcsspn(ucstr,lit)
#define UCstrcspn wcscspn
#define UCstrpbrk wcspbrk
#ifdef WINNT
#define _CRT_NON_CONFORMING_SWPRINTFS 1
#endif
#define UCsprintf swprintf
//#undef UCsprintf
//#define UCsprintf _snwprintf          /* Temp for office PC */
#define UCstrcpyToASC(ascdst,ucsrc) \
 { INDEX _i_ ; for(_i_=0;;_i_++) { ((char *)ascdst)[_i_] = ((UCCHAR *)ucsrc)[_i_] ; if (((UCCHAR *)ucsrc)[_i_] == UCEOS) break ; } ; } ;
#define UCstrcatToASC(ascdst,ucsrc) \
 { INDEX _i_,_j_ ; \
   for(_j_=0;;_j_++) { if (ascdst[_j_] == UCEOS) break ; } \
   for(_i_=0;;_i_++) { ascdst[_j_++] = ((UCCHAR *)ucsrc)[_i_] ; if (((UCCHAR *)ucsrc)[_i_] == UCEOS) break ; } ; } ;
#define UCstrcatToUC(ucdst,ascsrc) \
 { INDEX _i_,_j_ ; UCCHAR *__dst = ucdst ; \
   for(_j_=0;;_j_++) { if (__dst[_j_] == UCEOS) break ; } \
   for(_i_=0;;_i_++) { __dst[_j_++] = ((char *)ascsrc)[_i_] ; if (((char *)ascsrc)[_i_] == UCEOS) break ; } ; } ;
#define UCretASC(ucsrc) v_ConvertUCtoASC(ucsrc)
#define ASCretUC(src) v_ConvertASCtoUC(src)
#define UCstrstr wcsstr
#define UCfgets fgetws
#define UCfputs fputws
#define UCfputc fputwc
#define UCstrcpyAtoU(ucdst,ascrc) { INDEX _i_ ; for(_i_=0;;_i_++) { ((UCCHAR *)ucdst)[_i_] = (ascrc[_i_]) ; if (ascrc[_i_] == UCEOS) break ; } ; } ;
#define UCstrtol wcstol
#define UCstrtod wcstod
#define UCstrtoll _wcstoi64
#define UCatoi _wtoi
#define UCsizeof(element) ((sizeof element)/sizeof(UCCHAR))
#define vuc_IsAlpha(ucchar) (uci->Entry[ucchar].IsAlpha)
#define vuc_IsLetter(ucchar) (uci->Entry[ucchar].IsLetter)
#define vuc_IsUpper(ucchar) (uci->Entry[ucchar].IsUpper)
#define vuc_IsLower(ucchar) (uci->Entry[ucchar].IsLower)
#define vuc_IsDigit(ucchar) (uci->Entry[ucchar].IsDigit)
#define vuc_IsWSpace(ucchar) (uci->Entry[ucchar].IsWSpace)
#define vuc_IsPunc(ucchar) (uci->Entry[ucchar].IsPunc)
#define vuc_IsBgnId(ucchar) (uci->Entry[ucchar].IsBgnId)
#define vuc_IsContId(ucchar) (uci->Entry[ucchar].IsContId)
#define vuc_IsMath(ucchar) (uci->Entry[ucchar].IsMath)
#define vuc_IsPrint(ucchar) (uci->Entry[ucchar].IsPrint)
#define vuc_IsAlphaNum(ucchar) (uci->Entry[ucchar].IsAlpha || uci->Entry[ucchar].IsDigit)

#ifdef WINNT
#define UCstrcmpIC(ucstr1,ucstr2) wcsicmp(ucstr1,ucstr2)
#define UCstrncmpIC(ucstr1,ucstr2,len) _wcsnicmp(ucstr1,ucstr2,len)
#define UCfopen(file,mode) _wfopen(file,UClit(mode))
#define UCfopenAlt(file,mode) _wfopen(file,mode)
#define UCfreopen(file,mode,ofp) _wfreopen(file,UClit(mode),ofp)
#define UCremove(file) _wremove(file)
#define UCrename(oldfile,newfile)  MoveFileW(oldfile,newfile)
#define UCstat(file,statbuf) _wstat(file,(statbuf))
#define UCrmdir(file) _wrmdir(file,(statbuf))
#define UCopen(file,mode,perm) _wopen(file,mode,perm)
#define UCcreat(file,perm) _wcreat(file,perm)
#define UCasctime _wasctime
#define UCctime _wctime
#define UCCreateFile CreateFileW
#define UCGetFullPathName GetFullPathNameW
#define UCGetStartupInfo GetStartupInfoW
#define UCGetModuleFileName GetModuleFileNameW
#define UCCreateProcess CreateProcessW
#define UCShellExecuteEx ShellExecuteExW
#define UCGetFileAttributes GetFileAttributesW
#define UCFindNextFile FindNextFileW
#define UCFindFirstFile FindFirstFileW
#define UCCreateDirectory CreateDirectoryW
#define UCRemoveDirectory RemoveDirectoryW
#define UCCopyFile CopyFileW
#define UCchdir(DIRECTORY) (_wchdir(DIRECTORY) == 0)
#define UCmove MoveFileW
#define UCmoveEx MoveFileExW
#define UCSQLDriverConnect SQLDriverConnectW
#define UCSQLGetDiagRec SQLGetDiagRecW
#define UCSQLConnect SQLConnectW
#define UCSQLExecDirect SQLExecDirectW
#define UCSQLTables SQLTablesW
#define UCSQLDescribeCol SQLDescribeColW
#define UCcputs _cputws
//VEH100405 These are kinda strange because GetAddrInfoW/GetNameInfoW not supported on all versions of windows???
#define UCgetaddrinfo(OK,NODE,SERVICE,HINTS,RESULT) OK = getaddrinfo((NODE==NULL?NULL:UCretASC(NODE)),(SERVICE==NULL?NULL:UCretASC(SERVICE)),HINTS,RESULT)
#define UCgetnameinfo(OK,SOCKADDR,SOCKLEN,NODEBUF,NODEBUFSIZE,SERVICEBUF,SERVICEBUFSIZE,FLAGS) \
 { char _node[NI_MAXHOST],_service[256] ;\
   OK = getnameinfo(SOCKADDR,SOCKLEN,_node,sizeof _node,_service,sizeof _service,FLAGS) ;\
   UCstrcpyAtoU(NODEBUF,_node) ; if (SERVICEBUF != NULL) UCstrcpyAtoU(SERVICEBUF,_service) ;\
 }
#else
#define UCstrcmpIC(ucstr1,ucstr2) wcscasecmp(ucstr1,ucstr2)
#define UCstrncmpIC(ucstr1,ucstr2,len) wcsncasecmp(ucstr1,ucstr2,len)
#define UCfopen(file,mode) fopen(UCretASC(file),mode)
#define UCfopenAlt(file,mode) fopen(UCretASC(file),mode)
#define UCfreopen(file,mode,ofp) freopen(UCretASC(file),mode,ofp)
#define UCremove(file) remove(UCretASC(file))
#define UCrename(oldfile,newfile) (rename(UCretASC(oldfile),UCretASC(newfile)) ==  0)
#define UCmove(oldfile,newfile,ignored)  (rename(UCretASC(oldfile),UCretASC(newfile)) ==  0)
#define UCmoveEx(oldfile,newfile)  (rename(UCretASC(oldfile),UCretASC(newfile)) ==  0)
#define UCstat(file,statbuf) stat(UCretASC(file),(statbuf))
#define UCopen(file,mode,perm) open(UCretASC(file),mode,perm)
#define UCcreat(file,perm) creat(UCretASC(file),perm)
#define UCasctime(targ) ASCretUC(asctime(targ))
#define UCctime(targ) ASCretUC(ctime(targ))
#define UCgetaddrinfo(OK,NODE,SERVICE,HINTS,RESULT) OK = getaddrinfo((NODE==NULL?NULL:UCretASC(NODE)),(SERVICE==NULL?NULL:UCretASC(SERVICE)),HINTS,RESULT)
#define UCgetnameinfo(OK,SOCKADDR,SOCKLEN,NODEBUF,NODEBUFSIZE,SERVICEBUF,SERVICEBUFSIZE,FLAGS) \
 { char _node[NI_MAXHOST],_service[256] ;\
   OK = getnameinfo(SOCKADDR,SOCKLEN,_node,sizeof _node,_service,sizeof _service,FLAGS) ;\
   UCstrcpyAtoU(NODEBUF,_node) ; if (SERVICEBUF != NULL) UCstrcpyAtoU(SERVICEBUF,_service) ;\
 }
#endif

#else		/* BELOW FOR NO V4UNICODE */

#define V4RUNTIMEINFOFILEPREFIX UClit("v4runtimeinfo_ascii")
typedef char UCCHAR ;
#define UClit(str) str
#define UCEOS '\0'			/* End of string */
#define UCNL '\n'			/* New line */
#define UCEOLbt '\r'
#define UCEOLbts "\r"
#ifdef WINNT
#define UCTXTEOL "\r\n"			/* Windows text file EOL */
#else
#define UCTXTEOL "\n"			/* non Windows text file EOL */
#endif
#define UCempty(ucstr) (ucstr[0] == UCEOS)
#define UCnotempty(ucstr) (ucstr[0] != UCEOS)
#define UCstrlen(ucstr) ((LENMAX)strlen(ucstr))
#define UCstrchr(ucstr,charlit) strchr(ucstr,charlit)
#define UCstrchrV(ucstr,charvar) strchr(ucstr,charvar)
#define UCstrrchr strrchr
#define UCstrcmp(ucstr1,ucstr2) strcmp(ucstr1,ucstr2)
#define UCstrncmp(ucstr1,ucstr2,slen) strncmp(ucstr1,ucstr2,slen)
#define UCstrcpy(ucstr1,ucstr2) strcpy(ucstr1,ucstr2)
#define UCstrcat(ucstr1,ucstr2) strcat(ucstr1,ucstr2)
#define UCstrncat(ucstr1,ucstr2,len) strncat(ucstr1,ucstr2,len)
#define UCstrncpy(ucstr1,ucstr2,len) strncpy(ucstr1,ucstr2,len)
#define UCcnvupper(ucdst,ucsrc,max) { INDEX _i_ ; for(_i_=0;_i_<(max)-1;_i_++) { (ucdst)[_i_] = UCTOUPPER((ucsrc)[_i_]) ; if ((ucsrc)[_i_] == UCEOS) break ; } ; (ucdst)[max-1] = UCEOS ;} ;
#define UCcnvlower(ucdst,ucsrc) { INDEX _i_ ; for(_i_=0;;_i_++) { ucdst[_i_] = UCTOLOWER(ucsrc[_i_]) ; if (ucsrc[_i_] == UCEOS) break ; } ; } ;
#define UCTOUPPER(ucchar) toupper(ucchar)
#define UCSTRTOUPPER(ucstr) { UCCHAR *_ptr_ ; INDEX _i_ ; _ptr_ = ucstr ; for(_i_=0;_ptr_[_i_]!=UClit('\0');_i_++) { _ptr_[_i_] = UCTOUPPER(_ptr_[_i_]) ; } ; }
#define UCTOLOWER(ucchar) tolower(ucchar)
#define UCSTRTOLOWER(ucstr) { UCCHAR *_ptr_ ; INDEX _i_ ; _ptr_ = ucstr ; for(_i_=0;_ptr_[_i_]!=UClit('\0');_i_++) { _ptr_[_i_] = UCTOLOWER(_ptr_[_i_]) ; } ; }
#define UCSTRTOUPPER(ucstr) { UCCHAR *_ptr_ ; INDEX _i_ ; _ptr_ = ucstr ; for(_i_=0;_ptr_[_i_]!=UClit('\0');_i_++) { _ptr_[_i_] = UCTOUPPER(_ptr_[_i_]) ; } ; }
#define UCsscanf sscanf
#define UCstrspn(ucstr,lit) strspn(ucstr,lit)
#define UCstrcspn strcspn
#define UCstrpbrk strpbrk
#define UCstrcpyToASC(ascdst,ucsrc) strcpy(ascdst,ucsrc)
#define UCstrcatToASC(ascdst,ucsrc) strcat(ascdst,ucsrc)
#define UCstrcatToUC(ucdst,ascsrc) strcat(ucdst,ascsrc)
#define UCretASC(ucsrc) ucsrc
#define ASCretUC(src) src
#define UCstrstr strstr
#define UCfgets fgets
#define UCfputs fputs
#define UCfputc fputc
#define UCstrcpyAtoU(ucdst,ascrc) strcpy(ucdst,ascrc)
#define UCstrtol strtol
#define UCstrtod strtod
#define UCatoi _atoi
#define UCsizeof(element) ((sizeof element)/sizeof(UCCHAR))
#define vuc_IsAlpha(ucchar) isalpha(ucchar)
#define vuc_IsLetter(ucchar) isalpha(ucchar)
#define vuc_IsUpper(ucchar) isupper(ucchar)
#define vuc_IsLower(ucchar) islower(ucchar)
#define vuc_IsDigit(ucchar) isdigit(ucchar)
#define vuc_IsWSpace(ucchar) (ucchar == ' ' || ucchar == '\t')
#define vuc_IsPunc(ucchar) ispunct(ucchar)
#define vuc_IsBgnId(ucchar) (isalpha(ucchar) || (ucchar == '_'))
#define vuc_IsContId(ucchar) (isalnum(ucchar) || (ucchar == '_'))
#define vuc_IsMath(ucchar) isdigit(ucchar)
#define vuc_IsPrint(ucchar) isprint(ucchar)
#define vuc_IsAlphaNum(ucchar) isalnum(ucchar)
#define UCgetaddrinfo(OK,NODE,SERVICE,HINTS,RESULT) OK = getaddrinfo(NODE,SERVICE,HINTS,RESULT)
#define UCgetnameinfo(OK,SOCKADDR,SOCKLEN,NODEBUF,NODEBUFSIZE,SERVICEBUF,SERVICEBUFSIZE,FLAGS) OK = getnameinfo(SOCKADDR,SOCKLEN,NODEBUF,sizeof NODEBUF,SERVICEBUF,sizeof SERVICEBUF,FLAGS) ;

#ifdef WINNT
#define UCsprintf _snprintf
#define UCstrcmpIC(ucstr1,ucstr2) stricmp(ucstr1,ucstr2)
#define UCstrncmpIC(ucstr1,ucstr2,len) _strnicmp(ucstr1,ucstr2,len)
#define UCfopen(file,mode) fopen(file,UClit(mode))
#define UCfopenAlt(file,mode) fopen(file,mode)
#define UCfreopen(file,mode,ofp) freopen(file,UClit(mode),ofp)
#define UCremove(file) remove(file)
#define UCrename(oldfile,newfile) MoveFile(oldfile,newfile)
#define UCstat(file,statbuf) stat(file,(statbuf))
#define UCopen(file,mode,perm) open(file,mode,perm)
#define UCcreat(file,perm) creat(file,perm)
#define UCasctime asctime
#define UCctime ctime
#define UCCreateFile CreateFile
#define UCGetFullPathName GetFullPathName
#define UCGetStartupInfo GetStartupInfo
#define UCGetModuleFileName GetModuleFileName
#define UCCreateProcess CreateProcess
#define UCShellExecuteEx ShellExecuteEx
#define UCGetFileAttributes GetFileAttributes
#define UCFindNextFile FindNextFile
#define UCFindFirstFile FindFirstFile
#define UCCopyFile CopyFile
#define UCchdir(DIRECTORY) (_chdir(DIRECTORY) == 0)
#define UCCreateDirectory CreateDirectory
#define UCRemoveDirectory RemoveDirectory
#define UCmove(old,newfile) MoveFile(old,newfile)
#define UCmoveEx(old,newfile,ignored) MoveFile(old,newfile)
#define UCSQLDriverConnect SQLDriverConnect
#define UCSQLGetDiagRec SQLGetDiagRec
#define UCSQLConnect SQLConnect
#define UCSQLExecDirect SQLExecDirect
#define UCSQLTables SQLTables
#define UCSQLDescribeCol SQLDescribeCol
#define UCcputs _cputs
#define UCstrtoll _strtoi64
#else
#define UCsprintf snprintf
#define UCstrcmpIC(ucstr1,ucstr2) strcasecmp(ucstr1,ucstr2)
#define UCstrncmpIC(ucstr1,ucstr2,len) strncasecmp(ucstr1,ucstr2,len)
#define UCfopen(file,mode) fopen(UCretASC(file),mode)
#define UCfopenAlt(file,mode) fopen(file,mode)
#define UCfreopen(file,mode,ofp) freopen(UCretASC(file),mode,ofp)
#define UCremove(file) remove(UCretASC(file))
#define UCrename(oldfile,newfile) (rename(UCretASC(oldfile),UCretASC(newfile)) == 0)
#define UCmove(oldfile,newfile) (rename(UCretASC(oldfile),UCretASC(newfile)) == 0)
#define UCmoveEx(oldfile,newfile,ignored) (rename(UCretASC(oldfile),UCretASC(newfile)) == 0)
#define UCstat(file,statbuf) stat(UCretASC(file),(statbuf))
#define UCopen(file,mode,perm) open(UCretASC(file),mode,perm)
#define UCcreat(file,perm) creat(UCretASC(file),perm)
#define UCasctime(targ) ASCretUC(asctime(targ))
#define UCctime(targ) ASCretUC(ctime(targ))
#define UCchdir(DIRECTORY) (chdir(DIRECTORY) == 0)
#define UCstrtoll strtoll
#endif


#endif		/* V4UNICODE */

#ifdef WINNT
#define STRNCMPIC(str1,str2,len) strnicmp(str1,str2,len)
#else
#define STRNCMPIC(str1,str2,len) strncasecmp(str1,str2,len)
#endif

#define UI_MAXFOLD 521		/* Max number of case folding entries (currently about 180 - s/b prime with room for hashing) */
#define UI_FOLDCHARMAX 3

#define UIHASH1(val) (val % UI_MAXFOLD)			/* Initial hash into fold[] */
#define UIHASH2(val) ((val*val) % UI_MAXFOLD)		/* Subsequent hash if UIHASH1 is occupied */

#define UIFOLDHASH(val) \
 { for(hx=UIHASH1(val);uci->fold[hx].ucval!=val;hx=UIHASH2(hx)) { } ; }

struct V__UnicodeInfo {
  COUNTER foldcount ;
  struct {
    UCCHAR ucval ;		/* Unicode value */
    short chars ;		/* Number of fold characters */
    UCCHAR foldchars[UI_FOLDCHARMAX] ; /* Folding character values */
   } fold[UI_MAXFOLD] ;
  UCCHAR asciiLC[128] ;
  UCCHAR asciiUC[128] ;
  struct {
    CHARFIELD IsAlpha : 1 ;
    CHARFIELD IsLetter : 1 ;
    CHARFIELD IsUpper : 1 ;
    CHARFIELD IsLower : 1 ;
    CHARFIELD IsDigit : 1 ; 
    CHARFIELD IsWSpace : 1 ;
    CHARFIELD IsPunc : 1 ;
    CHARFIELD IsBgnId : 1 ;
    CHARFIELD IsContId : 1 ;
    CHARFIELD IsPrint : 1 ;
    CHARFIELD IsMath : 1 ;
    CHARFIELD IsFolded : 1 ;	/* If set then have to hash into fold[] above for case-insensitive compares */
    short OtherValue ;		/* ex: If IsUpper set then this is lower case value, if IsLower set then is upper case value */
    } Entry[512] ;
} ;


#define V4L_TextFileType_ASCII 0	/* ASCII text file */
#define V4L_TextFileType_UTF16be 1	/* Unicode UTF-16 big-endian */
#define V4L_TextFileType_UTF16le 2	/* Unicode UTF-16 little-endian */
#define V4L_TextFileType_UTF8 3		/* Unicode UTF-8 */
#define V4L_TextFileType_UTF32be 4	/* Unicode UTF-32 big-endian */
#define V4L_TextFileType_UTF32le 5	/* Unicode UTF-32 little-endian */
#define V4L_TextFileType_UTF8nh 6	/* Unicode UTF-8 (but don't output file prefix) */
#ifdef ISLITTLEENDIAN
#define V4L_TextFileType_UTF16 V4L_TextFileType_UTF16le
#else
#define V4L_TextFileType_UTF16 V4L_TextFileType_UTF16be
#endif

#define UCReadLine_MaxLine 0x20000		/* Max number of characters in input line */
#define UCFile_GetBufDfltChars 0x4000		/* Default size of read buffer */
#define UCFile_GetBufMaxChars 0x100000		/* Maximum size of read buffer */

#define VLOGDECODE_NewFile 1			/* File need not exist in v_UCLogicalDecoder */
#define VLOGDECODE_Exists 2			/* File must exist */
#define VLOGDECODE_KeepCase 4			/* Do not convert filename to lower case */

struct UC__File {
 FILE *fp ;
 FILEFD fd ;		/* File descriptor associated with fp */
 ETYPE filetype ;	/* Type of file - see V4L_TextFileType_xxx */
 ETYPE filemode ;	/* Open mode of file - see UCFile_Open_xxx */
 LOGICAL wantEOL ;	/* TRUE if want standard V4 UCEOL terminating character on lines, FALSE for no EOL character */
 LOGICAL hitEOF ;	/* If TRUE then hit EOF */
 ETYPE lineTerm ;	/* Line terminator - see UCFile_Term_xxx */
 LENMAX bufSize ;	/* "Suggested" number of bytes in read-from-file buffer */
 wchar_t *wtbuf ;	/* Temp WIDE buffer we may need for conversion between modes */
 LENMAX tbufmax ;	/* Number of CHARACTERS (not bytes) available in tbuf */
 char *cbuf ;		/* Temp CHAR buffer we may need for whatever */
 char *cbptr ;		/* Current pointer in cbuf */
 LENMAX cbufmax ;	/* Number of bytes allocated in cbuf */
 INDEX lineNum ;	/* Line number of last line read (via v_UCReadLine()) */
} ;

#define UCFile_Open_Read 1
#define UCFile_Open_Write 2
#define UCFile_Open_Append 3
#define UCFile_Open_ReadBin 4
#define UCFile_Open_WriteBin 5
#define UCFile_Open_UpdateBin 6

#define UCRead_ASCII V4L_TextFileType_ASCII		/* Read file as ASCII (convert if necessary/possible) */
#define UCRead_UTF8 V4L_TextFileType_UTF8		/* Read file in UTF-8 format */
#define UCRead_UTF16be V4L_TextFileType_UTF16be
#define UCRead_UTF16le V4L_TextFileType_UTF16le
#define UCFile_Term_EOF	0	/* End of file was 'line terminator' */
#define UCFile_Term_NL 1	/* Single NL character */
#define UCFile_Term_CR 2	/* Single CR character */
#define UCFile_Term_CRLF 3	/* CR-LF pair */
#define UCFile_Term_FF 4	/* Formfeed */
#define UCFile_Term_VT 5	/* Vertical tab */
#define UCFile_Term_EOB 6	/* Hit end of input buffer before end-of-line */

#define UCEOL UClit('\n')	/* Standard End-Of-Line terminator */

#ifdef V4UNICODE
#ifdef BIG_ENDIAN
#define UCRead_UC UCRead_UTF16be			/* Read file in UTF-16 mode (endian of local environment) */
#define UCUTF8toUTF16(u16str,u16len,u8str,u8len) UTF8ToUTF16BE(u16str,u16len,u8str,u8len+1)
#define UCUTF16toUTF8(u8str,u8len,u16str,u16len) UTF16BEToUTF8(u8str,u8len,u16str,u16len)
#else
#define UCRead_UC UCRead_UTF16le			/* Read file in UTF-16 mode (endian of local environment) */
#define UCUTF8toUTF16(u16str,u16len,u8str,u8len) UTF8ToUTF16LE(u16str,u16len,u8str,u8len+1)
#define UCUTF16toUTF8(u8str,u8len,u16str,u16len) UTF16LEToUTF8(u8str,u8len,u16str,u16len)
#endif

#else		/* not V4UNICODE */
#define UCRead_UC UCRead_UTF8				/* If not UC then treat UC as UTF8 */
#define UCUTF8toUTF16(u16str,u16len,u8str,u8len) (strncpy(u16str,u8str,u8len),(u16str)[u8len]='\0',strlen(u16str))
#define UCUTF16toUTF8(u8str,u8len,u16str,u16len) (strncpy(u8str,u16str,u16len),(u8str)[u16len]='\0',strlen(u8str))

#endif		/* V4UNICODE */

struct V4__BktHdr
{ unsigned int BktNum : 30 ;		/* Bucket number within Area */
  unsigned int BktType : 2 ;		/* Type of bucket - see V4_BktHdrType_xxx */
  unsigned short AvailBktSeq ;		/* Next available sequence number for DataId */
  BYTE InUse ;				/* Incremented each time process uses bucket, decremented when done */
					/* Used to prevent process from swapping out bucket when another is still using */
  BYTE Unused ; 			/* Pad out to word boundary */
  /* subheader follows here */
} ;

/*	Index-Sequential Structures				*/

#define V4IS_BktSize_Dflt 2048		/* Default bucket size in bytes */
#define V4IS_BktSize_Max 0x10000	/* Max bucket size */
#define V4IS_BktSize_Max_Mask (V4IS_BktSize_Max - 1)
#define V4IS_BktIncrement 512		/* Incremental bucket size */
#define V4AreaAgg_DfltBktSize 0xFA00	/* Default V4 aggregate bucket size */
#define V4AreaAgg_UsableBytes 0xFA00-64
#define V4TempArea_BktSize V4AreaAgg_DfltBktSize /* Bucket size for auto-created V4 temp working area */
#define V4TempArea_UsableBytes 0xf000 - 64

struct V4IS__Bkt
{ BYTE Buffer[V4IS_BktSize_Max] ;
} ;

struct V4IS__IndexBktHdr
{ struct V4__BktHdr sbh ;		/* Always begin with standard header */
  int ParentBktNum ;			/* Bucket number of parent (0 = top level) */
  unsigned short KeyEntryTop ;		/* Index to top of KeyIndex (s/b directly below header except for root bucket) */
  unsigned short KeyEntryNum ;		/* Number of key/datarec entries in this bucket */
  unsigned short FreeBytes ;		/* Number of remaining (free) bytes */
  unsigned short DataTop ;		/* Index to current top of data region */
  int IBHUnused ;			/* Currently Unused */
} ;
#define DATATOPVAL(DT) ((DT) == 0 ? 0x10000 : (DT))	/* Max bucket size is 0x10000 but can't hold that in unsigned-short (VEH100129) */

struct V4IS__DataBktHdr
{ struct V4__BktHdr sbh ;
  unsigned short FreeBytes ;		/* Number of free bytes remaining */
  unsigned short FirstEntryIndex ;	/* Index to first data entry */
  unsigned short FreeEntryIndex ;	/* Index to first free slot */
  unsigned short DBHUnused ;		/* Currently Unused */
} ;

#define UBHFlagVal 0x10203040		/* Flag for unused buckets */
struct V4IS__UnusedBktHdr
{ struct V4__BktHdr sbh ;
  int NextUnusedBkt ;			/* Bucket number of next unused, 0 if end of chain */
  int UBHFlag ; 			/* Must be UBHFlagVal or bucket should not be re-used */
} ;

#define V4IS_DataCmp_None 0		/* No data compression */
#define V4IS_DataCmp_Mth1 1		/* Normal V3 compression (method 1) */
#define V4IS_DataCmp_Mth2 2		/* lzrw1 compression (much fancier than method 1) */

#define V4IS_DataCmp_Mth2MinSize 20000	/* Minimum number of bytes to auto flip to Method 2 compression */

#define V4IS_FileRef_Obsolete 0x3fff	/* Special FileRef below indicating obsolete entry */

struct V4IS__DataBktEntryHdr
{ DATAID DataId ;			/* Unique Data ID for this record */
  BITS CmpMode : 2 ;			/* Data compression mode- V4IS_DataCmp_xxx */
  BITS FileRef : 14 ;			/* FileRef associated with this data record */
  BITS Bytes : 16 ;			/* Bytes in entry (including this header)  */
} ;

#define V4IS_EntryType_IndexLink 1	/* Is a link to sub directory */
#define V4IS_EntryType_DataLink 2	/* Data can be found in data bucket */
#define V4IS_EntryType_DataLen 3	/* Data immediately follows */

struct V4IS__IndexKeyDataEntry
{ BITS EntryType : 4 ;			/* Type of Entry- see V4IS_EntryType_xxx */
  BITS HasMultKeys : 1 ;		/* TRUE if entry has multiple keys */
  BITS KeyNotInRec : 1 ;		/* if TRUE then key following ikde is not part of data record */
  BITS CmpMode : 2 ;			/* Data Compression - see V4IS_DataCmp_xxx */
  BITS	AuxVal : 24 ;			/* Aux Value such as index/data bucket number, data length, etc. */
  DATAID DataId ;			/* Unique Data ID for this record */
} ;

#define V4IS_StashAt_End -1		/* Used to force IndexStash at end of bucket/chain-of-duplicates */
#define V4IS_IndexBktKeyList_Max 200
#define V4IS_DataIndex_Null 0xffff	/* Used to denote an empty place holder in list below */

struct V4IS__IndexBktKeyList
{ unsigned short DataIndex[V4IS_IndexBktKeyList_Max] ;	/* Index via current bucket to V4IS__IndexKeyDataEntry */
} ;

#define V4IS_KeyType_FrgnKey1 0 	/* Foreign entry (primary key) - format defined by user (AuxVal is user's "FileRef") */
#define V4IS_KeyType_FrgnKey2 1 	/* Foreign key (secondary) */
#define V4IS_KeyType_FrgnKey3 2
#define V4IS_KeyType_FrgnKey4 3
#define V4IS_KeyType_FrgnKey5 4
#define V4IS_KeyType_FrgnKey6 5
#define V4IS_KeyType_FrgnKey7 6
#define V4IS_KeyType_FrgnKey8 7
//#define V4IS_KeyType_FrgnFile 5 	/* Info (e.g. keys) for a foreign file */
#define V4IS_KeyType_V4Segments 8	/* Large data items that may be broken into segments, AuxVal is 0, key is segment number */
#define V4IS_KeyType_V4 9		/* V4 Entry, AuxVal is V4IS_SubType_xxx */
#define V4IS_KeyType_Binding 10 	/* Record is binding list- AuxVal is Dimension, key is point within dimension */
#define V4IS_KeyType_AggShell 11	/* Record is an aggregate- key similar to Binding (above) */
#define V4IS_KeyType_xdbRow 12		/* XDB row data, AuxVal is Dimension, key is record number (see struct V4XDB_SavedRow) */

#define V4IS_SubType_IntDict 1		/* Internal Dictionary Entry */
#define V4IS_SubType_RevDict 2		/* Reverse Dictionary Entry */
#define V4IS_SubType_Value 3		/* Actual Value */
#define V4IS_SubType_ExtDict 4		/* External Dictionary Entry */
#define V4IS_SubType_DimPnt 5		/* Verification of a Dimension Point */
#define V4IS_SubType_DimInfo 6		/* Access to V4DPI__DimInfo record */
#define V4IS_SubType_DimDict 7		/* Dictionary entry for dimensions in area */
#define V4IS_SubType_BindInfo 8 	/* List of dimensions with bindings in area */
#define V4IS_SubType_V3PicMod 9 	/* Record is a PIC format V3 module */
#define V4IS_SubType_BigText 10 	/* Record is a "big" text string (e.g. V4 macro, V3 record structure) */
#define V4IS_SubType_DimUnique 11	/* Record holds next available unique point number for a dimension */
					   /* Integer key is dimension number */
#define V4IS_SubType_DUList 12		/* Record contains list of points (Via POINT command) for dimension */
#define V4IS_SubType_VHdr 13		/* Record contains VHdr info for dimension */
#define V4IS_SubType_AggDims 14 	/* Record is V4Agg__AggDims - one per Aggregate area */
#define V4IS_SubType_V4ISStEl 15	/* Record is structure/element definitions */
#define V4IS_SubType_BigIsct 16 	/* Record is a big intersection */
#define V4IS_SubType_AggConstant 17	/* Record is point value (V4Agg__Constant) which is constant within Agg (key is dimension) */
#define V4IS_SubType_ProjectionInfo 18	/* Record is info for projecting from dim1 to dim2 */
#define V4IS_SubType_ProjectionData 19	/* Record is data for projecting from dim1:pt to dim2 */
#define V4IS_SubType_IntXDict 20	/* String->Num XDict Mapping Entry (struct V4DPI__XDictEntry) */
#define V4IS_SubType_RevXDict 21	/* Num->String XDict Mapping Entry (struct V4DPI__RevXDictEntry) */
#define V4IS_SubType_XDictDimPoints 22	/* XDict Entry of Last Point Number on Dimension */
#define V4IS_SubType_Drawer 23		/* V4IM__Drawer record */
#define V4IS_SubType_TreeDir 24		/* Tree Directory Record (struct V4Tree__Directory) */
#define V4IS_SubType_CompileDir 25	/* Compilation directory of source files in an area (struct V4LEX__CompileDir) */
#define V4IS_SubType_MacroBoot 26	/* New (as of 11/1/2008) macro definition boot structure */

#define V4IS_KeyMode_Int 0		/* Key is 32 bit integer */
#define V4IS_KeyMode_RevInt 1		/* Key is 32 bit integer sorted in reverse order */
#define V4IS_KeyMode_Alpha 2		/* Key is alpha string */
#define V4IS_KeyMode_DblFlt 3		/* Key is 64 bit floating (type D) */
#define V4IS_KeyMode_Int2 4		/* Key is 32*2 integer */
#define V4IS_KeyMode_Fixed 5		/* Key is 64 bit implied decimal */

#define V4IS_KeyBytes_Max 250		/* Max bytes in key data */
#define V4IS_IntKey_Bytes 8		/* Number of bytes in an integer V4IS key */
#define V4IS_Int2Key_Bytes 12		/* Number of bytes in an Int2 V4IS key */
#define V4IS_FixedKey_Bytes 12		/* Number of bytes in an Fixed V4IS key */
#define V4IS_xdbRow_Bytes 8		/* Bytes in xdbRow key */

union V4IS__KeyPrefix
{ struct
#ifdef ISLITTLEENDIAN
   { unsigned Bytes : 8 ;
     unsigned KeyMode : 4  ;
     unsigned KeyType : 4 ;
     unsigned AuxVal : 16 ;		/* These fields MUST match below! */
   } fld ;
#else
   { unsigned AuxVal : 16 ;		/* These fields MUST match below! */
     unsigned KeyType : 4 ;
     unsigned KeyMode : 4  ;
     unsigned Bytes : 8 ;
   } fld ;
#endif
  int all ;
} ;

struct V4IS__Key
#ifdef ISLITTLEENDIAN
{ unsigned Bytes : 8 ;
  unsigned KeyMode : 4	;
  unsigned KeyType : 4 ;
  unsigned AuxVal : 16 ;		/* These fields MUST match below! */
#else
{ unsigned AuxVal : 16 ;		/* These fields MUST match below! */
  unsigned KeyType : 4 ;
  unsigned KeyMode : 4	;
  unsigned Bytes : 8 ;
#endif
  union
   { int IntVal ;			/* Integer key value */
     int Int2Val[2] ;			/* For KeyMode_Int2 */
     BYTE B64[8] ;			/* 64 bit whatever */
     char Alpha[V4IS_KeyBytes_Max] ;	/* Additional portion of key (ex: dictionary word) */
   } KeyVal ;
} ;

struct V4IS__DataIdKey			/* DataId + Key (used by vsort) */
{ DATAID DataId ;
  union V4IS__KeyPrefix KeyP ;
  int IntVal ;				/* Just track integer key (alpha keys start at same position!) */
} ;

#define V4IS_DataIdListMax 512
struct V4IS__DataIdList {
  int Bucket ;			/* Current bucket number */
  int Count ;			/* Number below */
  int NextX ;			/* Index to next one to return */
  DATAID DataIds[V4IS_DataIdListMax] ; /* List of data ids in current bucket */
 } ;

struct V4IS__KeyCmpList
{ DATAID DataId ;			/* Old dataid */
  short count ; 			/* Number below */
  struct
   { struct V4IS__Key OKey ;		/* Old Key Value for n'th key */
     struct V4IS__Key NKey ;		/* New Key Value for n'th key */
     unsigned short StartIndex ;	/* Starting index in record */
     BYTE Differ ;			/* TRUE if old key differs from new */
     BYTE DupsOK ;			/* TRUE if duplicates allowed */
     BYTE Primary ;			/* TRUE if this is the primary key */
   } Entry[10] ;
} ;

#define V4IS_ExtLink_OpenMode_Read 0	/* Open link for read only */
#define V4IS_ExtLink_OpenMode_Auto 1	/* Open link same mode as master file */

struct V4IS__ExtLinkInfo
{
  UCCHAR UCFileName[V_FileName_Max] ;	/* File name for auto-linking */
  BYTE LinkOpenMode ;			/* How file is to be opened- V4IS_ExtLink_OpenMode_xxx */
} ;

#define V4IS_AreaCB_LinkMax 10		/* Max number of linked areas per area */
#define V4IS_AreaCB_LvlMax 25		/* Max number of index levels */

#define V4IS_PCB_AM_Get 1		/* Open for GET access */
#define V4IS_PCB_AM_Delete 2		/* Delete */
#define V4IS_PCB_AM_Insert 4
#define V4IS_PCB_AM_Update 8
#define V4IS_PCB_AMF_Indexed 0x10000	/* Create indexed file (default *) */
#define V4IS_PCB_AMF_Sequential 0x20000 /* Create sequential only file */

#define V4IS_AreaId_SeqOnly 0xfff	/* Special AreaId for sequential only files */
#define V4IS_EOFMark_SeqOnly -1234	/* Special EOF length marker for sequential only files */

#define V4IS_PCB_LM_Get 1		/* Lock Mode- Get Access */
#define V4IS_PCB_LM_Delete 2		/* Delete */
#define V4IS_PCB_LM_Insert 4
#define V4IS_PCB_LM_Update 8

#define V4IS_PCB_OM_Read 1		/* Open existing file (read only) */
#define V4IS_PCB_OM_Update 2		/* Open existing file for update */
#define V4IS_PCB_OM_New 3		/* Create new file */
#define V4IS_PCB_OM_NewIf 4		/* Create new if necessary */
#define V4IS_PCB_OM_NewAppend 5 	/* Open new file for Append-Only (for loading/rebuilding) */
#define V4IS_PCB_OM_NewTemp 6		/* Create new file & delete on close */
#define V4IS_PCB_OM_MustBeNew 7 	/* Create new file, cannot already exist */
#define V4IS_PCB_OM_Partial 8		/* Only open (read/existing) to get glb/lt set up */
#define V4IS_PCB_OM_Nil 20		/* Don't really open file - allow puts to bit bucket */

#define V4IS_PCB_OF_DisconnectOnClose 0x40000	/* Disconnect socket to server on close of file */
#define V4IS_PCB_OF_ForceClear 0x80000		/* Force clear of lock table on open (if necessary) */
#define V4IS_PCB_OF_SeqScan 0x100000	 /* Assume sequential scan thru entire area */
#define V4IS_PCB_OF_NoError 0x200000	/* Don't generate v4_error call if v4is_Open fails */
#define V4IS_PCB_OF_ForceOpen 0x400000	/* Try to force v4is_Open() (ignore some regular checks on open) */

#define V4IS_PCB_GP_Keyed 1		/* Keyed Access */
#define V4IS_PCB_GP_Next 2		/* Sequential Access */
#define V4IS_PCB_GP_Prior 3		/* Prior Sequential Access */
#define V4IS_PCB_GP_BOF 4		/* Begin of File */
#define V4IS_PCB_GP_EOF 5		/* End of File */
#define V4IS_PCB_GP_Append 6		/* Append sequential */
#define V4IS_PCB_GP_KeyedNext 7 	/* Keyed access (or next higher) */
#define V4IS_PCB_GP_KeyedPrior 8	/* Keyed access or next lower */
#define V4IS_PCB_GP_Delete 9		/* Delete record */
#define V4IS_PCB_GP_Update 10		/* Update existing (overwrite) */
#define V4IS_PCB_GP_Insert 11		/* Insert new record (error if exists) */
#define V4IS_PCB_GP_Write 12		/* Insert if new, update if exists */
#define V4IS_PCB_GP_DataId 13		/* Return record associated with pcb->DataId */
#define V4IS_PCB_GP_DataOnly 14 	/* Write out data only, no keys */
#define V4IS_PCB_GP_Unlock 15		/* Unlock current record (via put) */
#define V4IS_PCB_GP_NextNum1 16 	/* Return next available number, increment */
#define V4IS_PCB_GP_Cache 17		/* Cached put mode (hashed areas only) */
#define V4IS_PCB_GP_Reset 18		/* Reset (clear) all records (hashed areas only) */
#define V4IS_PCB_GP_Obsolete 19 	/* Mark current record as obsolete (will be dumped in next xv4 -or/-of) */

#define V4IS_PCB_NoLock 0x10000
#define V4IS_PCB_IgnoreLock 0x20000
#define V4IS_PCB_LengthOnly 0x40000	/* Do not return data, only update DataId & GetLen */
#define V4IS_PCB_SafePut 0x80000
#define V4IS_PCB_DataPtr 0x100000	/* Update pcb->DataPtr/DataLen with pointer to data (if possible) */
#define V4IS_PCB_RetPtr 0x200000	/* Return pointer to data ** INTERNAL USE ONLY ** */
#define V4IS_PCB_AllocBuf 0x400000	/* Allocate buffer for user & update with record */
#define V4IS_PCB_NoDataCmp 0x800000	/* Inhibit data compression */
#define V4IS_PCB_CheckTree 0x1000000	/* Check entire directory tree after put */
#define V4IS_PCB_CSDataRec 0x2000000	/* Return data in V4CS__DataRec format (INTERNAL USE ONLY) */
#define V4IS_PCB_ReLock 0x4000000	/* Relock record after v4is_Put() */
#define V4IS_PCB_NoError 0x8000000	/* Don't generate error if v4is_Get() fails, return UNUSED as GetLen */
#define V4IS_PCB_LockWait 0x10000000	/* Wait & retry if record locked on get */
#define V4IS_PCB_CmpOnOverload 0x20000000 /* Compress only if record overloaded (i.e. over max allowed record length) */
#define V4IS_PCB_CSExpDataRec 0x40000000    /* Expand record before blowing through v4server (INTERNAL USE ONLY) */

#define V4IS_PCB_DataMode_Auto 0	/* V4 Determines data placement */
#define V4IS_PCB_DataMode_Index -1	/* Place data in index bucket */
#define V4IS_PCB_DataMode_Data -2	/* Place data in data bucket */
					/* +n for placement in index bucket if length < n */

#define V4IS_V3Name_Max 31		/* Max characters in V3 I/O Unit Name */
#define V4IS_PCB_LinkMax 10		/* Max number of AuxLink PCBs */

#define V4IS_Reformat_XDict 0x1 	/* Rebuilding External Dictionary File - only copy Dimension info */
#define V4IS_Reformat_Silent 0x2	/* Perform "silent" reformatting */
#define V4IS_Reformat_NoContPrompt 0x4	/* Do not prompt for "Enter 'C' to continue" - just return error */

struct V4IS__ParControlBlk
{ char V3name[V4IS_V3Name_Max] ;	/* V3 name for linkage to V3 error controls */
//  char FileName[V_FileName_Max] ;	/* Area File Name */
  UCCHAR UCFileName[V_FileName_Max] ;	/* Area File Name */
  ETYPE OpenMode,AreaFlags,LockMode ;
  LENMAX BktSize ;
  ETYPE AccessMode ;
  ETYPE MinCmpBytes ;			/* Compress any records greater than this number */
  LENMAX MaxRecordLen ;			/* Max record length allowed */
  int RelHNum ; 			/* Relative Heirarchy Number (0 for foreign only) */
  char BindCatList[250] ;		/* *** UNUSED **** Binding Categories for Area */
#ifdef WINNT
  HANDLE hfile ;
#else
  FILE *FilePtr ;			/* Point to actual file */
#endif
  ETYPE GetMode,PutMode ; 		/* Get & Put Modes */
  ETYPE DfltGetMode,DfltPutMode ;
  LENMAX GetLen ;
  COUNTER GetCount,PutCount ;
  ETYPE DataMode,DfltDataMode ;
  AREAID AreaId;				/* AreaID associated with this PCB */
  DATAID DataId ;			/* DataId currently/last associated */
  DATAID DataId2 ; 			/* Extra DataId for compat with VMS RFA (which is 4+2 bytes) */
  BYTE *DfltPutBufPtr,*PutBufPtr ;	/* Put Buffer */
  LENMAX DfltPutBufLen,PutBufLen ;
  BYTE *DfltGetBufPtr,*GetBufPtr ;
  LENMAX DfltGetBufLen,GetBufLen ;
  struct V4IS__Key *DfltKeyPtr,*KeyPtr ;
  LENMAX DfltKeyLen,KeyLen ;
  LENMAX DfltKeyNum,KeyNum ;
  int DfltFileRef,FileRef ;		/* Current FileRef (0 for internal) */
  BYTE *DataPtr ;			/* Updated to point to data in V4MM Buffer */
  LENMAX DataLen ; 			/*  to len if no copying */
  struct V4FFI__KeyInfo *KeyInfo ;	/* Points to key info on create of new area */
  INDEX CurPosKeyIndex,CurLvl,BktNums[V4IS_AreaCB_LvlMax] ;	/* Saved after each GET so REPLACE can properly reposition */
  union V4IS__KeyPrefix KeyPrefix ;	/* Prefix of last record (for sequential scans in multi-key areas) */
  double MaxBlockFillPC ;		/* Tracks maximum % compressed records of max-record-length */
  short AuxLinkCnt ;			/* Number of link PCBs below */
  UCCHAR errMsgBuf[256] ;		/* Hold last error message associated with this file */
  struct
   { int LinkFileRef ;			/* FileRef we are linking to */
     struct V4IS__ParControlBlk *PCBPtr ; /* Link to other PCB if FileRef is in different area */
//     struct V4FFI__FileSpec *ffs ;	/* Link to foreign file info */
   } Link[V4IS_PCB_LinkMax] ;
} ;

#define V4IS_OCBType_IndexOnly 1	/* ocbPtr points to V4IS__IndexOnly struct */
#define V4IS_OCBType_Hash 2		/* ocbPtr points to V4FH__OCBxxxHash struct */
#define V4IS_OCBType_mmm 3		/* ocbPtr points to mmm (struct V4MM__MemMgmtMaster) */

struct V4IS__OptControlBlk		/* Format of option control block */
{ ETYPE ocbType ; 			/* Type of block: V4IS_OCBType_xxx */
  struct V4IS__OptControlBlk *Nextocb ; /* Pointer to next ocb, NULL if end of list */
  BYTE *ocbPtr ;			/* Point to actual data */
} ;

/*	A R E A   S T R U C T U R E S				*/

/*	V4IS__RootInfo - Found in top level bucket below bucket header */

struct V4IS__RootInfo
{ int BktSize ; 			/* Number of bytes in a bucket (0 = sequential only file) */
  int Version ;
  int BktStartByte ;			/* Starting byte for first Dir/Data buffer */
  int AreaHInfoByte ;			/* If non-zero then start byte for area heirarchy info */
  int FFIKeyInfoByte ;			/* If non-zero then start byte for foreign key info */
  int FreeDataInfoByte ;		/* If non-zero then start byte for available data bucket space */
  int NextAvailBkt ;			/* Next available bucket number */
  int NextAvailDimDictNum ;		/* Next available dictionary entry number (don't forget 3 bit heirarchy prefix!) */
  int NextAvailExtDictNum ;		/* Next available external dictionary entry number */
  int NextAvailIntDictNum ;		/* Next available internal dictionary entry number */
  int NextAvailValueNum ;		/* Next available point number */
  int NextUnusedBkt ;			/* If nonzero then bucket number we can re-use */
  int NumLevels ;			/* Number of index levels in directory (-1 for hashed files) */
  int MinCmpBytes ;			/* Compress any record longer than this */
  int MaxRecordLen ;			/* Maximum record length */
  int DataMode ;			/* Determines where data is to be stored (index or data buckets) */
  int FillPercent ;			/* Percentage to fill index/data buckets on reformat (25-100) */
  int AllocationIncrement ;		/* Allocation increment (bytes) */
  int RecordCount ;			/* Number of records in area */
  int GlobalBufCount ;			/* Number of global buffers to allocate (0 for auto-determine) */
  int CreateTime ;			/* Time Area created (used by ftok to uniqely id different versions of area) */
  int IndexOnlyInfoByte ;		/* If non-zero then start byte for Index-Only linked areas */
  int LockMax ; 			/* Maximum number of locks in area (0 for auto-determine) */
  int NextAvailUserNum ;		/* Next available user number */
  int lastSegNum ;			/* Last used segment number (don't want to start with 0) */
  int Expansion[5] ;
} ;

/*	V4IS__IndexOnly - Information for linked index-only areas		*/

#define V4IS_IndexOnly_Max 10		/* Max number of index-only areas we can link up */

struct V4IS__IndexOnly
{ unsigned short Bytes ;		/* Number of bytes in this structure */
  unsigned short count ;		/* Number of entries to follow */
  struct {
    UCCHAR AreaName[V_FileName_Max] ;	/* Area file name */
    int BktOffset ;			/* Bucket offset for dataid's */
   } Entry[V4IS_IndexOnly_Max] ;
} ;

/*	V4IS__FreeDataInfo - (Partial) list of free space in data buckets	*/

#define V4IS_FreeDataInfo_Max 10	/* Max number of blocks to track below */

struct V4IS__FreeDataInfo
{ unsigned short count ;		/* Number of blocks */
  unsigned short unused ;		/* Field currently undefined */
  struct {
    int DataBktNum ;			/* Data bucket number */
    int FreeBytes ;			/* Number of free bytes in the bucket */
   } Entry[V4IS_FreeDataInfo_Max] ;
} ;

/*	V4IS__AreaCB - Process level area control block 	*/

struct V4IS__AreaCB
{ struct V4IS__RootInfo *RootInfo ;	/* Pointer to Area Master in top level bucket (must be locked!) */
  struct V4C__AreaHInfo *ahi ;		/* If non-null then pointer to Heirarchy info for this area */
  struct V4FFI__KeyInfo *KeyInfo ;	/* If non-null then pointer to foreign file key info for this area */
  struct V4IS__FreeDataInfo *FreeData ; /* If non-null then pointer to free data list */
  struct V4IS__IndexOnly *io ;		/* If non-null then pointer to Index-Only info */
  struct V4IS__DataIdList *dil ;	/* If non-null then pointer to list for Data-Only read of area */
  int *HashInfo ;			/* If non-null then have hash file - points to hash information */
  struct V4IS__ParControlBlk *pcb ;	/* Parent pcb */
  AREAID AreaId;			/* Area ID associated with this control block */
#ifdef V4ENABLEMULTITHREADS
  DCLSPINLOCK areaLock ;		/* Multi-thread lock */
#endif
  FLAGS32 CSFlags ; 			/* Flags for Client-Server linkage: see V4CS_FLAG_xxx */
  SOCKETNUM CSSocket ;			/* Socket to server */
  int CSaid ;				/* AreaId on server */
  ETYPE AccessMode ;
  LENMAX BktSize ;
  INDEX CurPosKeyIndex ;		/* Current Position- Key within bucket */
  INDEX CurLvl ;			/* Current Level in structure below (current tree positioning) */
  struct
   { int BktNum ;			/* Bucket Number for index at specified level */
   } Lvl[V4IS_AreaCB_LvlMax] ;
  int SeqDataBktNum ;			/* Last accessed Data Bucket via Get-Next */
  DATAID DataId ;			/* Current Data Identifier */
  int KeyNum ;				/* Current Key Number for multiple key areas */
  union V4IS__KeyPrefix KeyPrefix ;	/* Prefix of last record (for sequential scans in multi-key areas) */
  struct V4IS__Key LastKey ;		/* Last key (used with DataId for panic repositioning) */
  int LastPosResult ;			/* Results of last positioning - tells us if we are at a key */
  short LinkAreaCnt ;			/* Number of linked areas below */
  BYTE *AuxDataBufPtr ;			/* If nonNULL then pointer to compression/expansion buffer */
  ETYPE DataMode ;
  LENMAX MaxRecordLen ;
  LENMAX MinCmpBytes ;			/* Compress any records greater than this number */
  struct
   { unsigned short FileRef ;		/* Foreign/External FileRef used by user */
     AREAID AreaId;			/* Can be found in this area */
   } LinkArea[V4IS_AreaCB_LinkMax] ;
} ;
 
/*	H A S H   S T R U C T U R E S				*/

#define V4H_EntryBuf_Max 4096		/* Max size of hash file entry */

struct V4H__HashEntry
{ int Hash ;				/* Hash code for entry (cannot be 0) */
  BYTE EntryBuf[V4H_EntryBuf_Max] ;	/* Data area */
} ;

#define V4H_HashType_Fixed 1		/* Fixed size hash file */

struct V4H__FixHash			/* structure (type = V4IS_HashType_Fixed) for v4is_Open for Fixed Hash file */
{ int HashType ;			/* Type of Hash File (V4H_HashType_Fixed) */
  int RecordBytes ;			/* Number of bytes in a (user) record */
  int EntryBytes ;			/* Number of bytes in an entry */
  int MaxRecords ;			/* Max number of records */
  int FillPercent ;			/* Max fill percentage */
  int HashFunction ;			/* Hash function to use */
  int HashKeyOffset ;			/* Offset into record for key */
  int HashKeyBytes ;			/* Number of bytes in key */
} ;

/*	Mapping into RootInfo
	BktSize 	RecordBytes*MaxRecords		Bucket is entire hash space
	GlobalBufCnt	1				so BktSize*GlobalBufCnt works out
	MaxRecordLen	RecordBytes
	FillPercent	FillPercent
	RecordCount	current number of records	(cannot exceed (MaxRecord*FillPercent/100))
	AreaHInfoByte	nn				index in root to V4F__xxxHash structure
							(acb->HashInfo is runtime pointer to structure)
	NumLevels	-1				Indicates a hashed file
*/

#define V4H_HashFunc_Int1 1			/* Integer Hash, HashKeyBytes/(sizeof int) = number of ints */
#define V4H_HashFunc_Alpha1 2			/* Plain alpha hash */

#define V4H_HashFuncKeyOffset_Default 0 	/* (will attempt to pull key info from FFIKeyInfo) */
#define V4H_HashFuncKeyBytes_Default (sizeof int)

#define V4H_FillPercent_Default 80	/* Default Fill Percentage */

#define V4H_PutHash_Insert 1			/* Insert new record */
#define V4H_PutHash_Update 2			/* Update existing record */
#define V4H_PutHash_Write 3			/* Insert/Update as needed */
#define V4H_PutHash_Delete 4			/* Delete record */
#define V4H_PutHash_Cache 5			/* Cache record (overwrite another if necessary) */
#define V4H_PutHash_Reset 6			/* Clear (zap) all records in hash table */

typedef struct					/* MD5 Hashing */
{ UINT32 total[2];
  UINT32 state[4];
  UINT8 buffer[64];
} VMD5__Context;


/*	M E M O R Y   M A N A G E M E N T			*/

#define V4MM_BktPtr_Update 0x10000	/* Flag bucket for update */
#define V4MM_BktPtr_MemLock 0x20000	/* Lock bucket into memory (permanently) */
#define V4MM_BktPtr_TempLock 0x40000	/* Lock bucket into memory temporarily */
#define V4MM_BktPtr_ReRead 0x80000	/* Force re-read of bucket (in case we suspect bucket was corrupted) */
#define V4MM_BktPtr_IgnoreLock 0x100000 /* Ignore any locking on the bucket (for pulling root on v4is_Open) */

/*	Format of Bucket ID */

union V4MM__BIdMask
{ struct
   { BITS AreaIndex : 8 ;
     BITS BktNum : 24 ;
   } fld ;
  int bid ;
} ;

 /*	Format of Area ID */

union V4MM__AreaMask
{ struct
   { int AreaIndex ;
   } fld ;
  int aid ;
} ;

/*	Format of Data ID */

#define V4MM_MaxDataIdSeq 0xfff-1	/* Max BktSeq Id (to prevent overflow of 12 bits) */
union V4MM__DataIdMask
{ struct
   { BITS BktNum : 20 ;			/* Bucket Number */
     BITS BktSeq : 12 ;			/* Sequence Number in Bucket */
   } fld ;
  DATAID dataid ;
} ;

#ifdef V4_BUILD_LMTFUNC_MAX_AREA
#define V4MM_Area_Max V4_BUILD_LMTFUNC_MAX_AREA
#else
#define V4MM_Area_Max 150		/* Max number of concurrent areas */
#endif

#ifdef NEWMMM
#define CURRENTMMM ((struct V4MM__MemMgmtMaster *)TlsGetValue(gpi->mmmTLS))
#define GETMMMFROMAREAX(MMM,AREAX) \
 MMM = ((struct V4MM__MemMgmtMaster *)TlsGetValue(gpi->mmmTLS)) ; \
 if (MMM->Areas[AREAX].mmmReal != NULL) MMM = MMM->Areas[AREAX].mmmReal ;
#define GRABAREAMTLOCK(AREAX) GRABMTLOCK(mmm->Areas[AREAX].AreaCBPtr->areaLock)
#define FREEAREAMTLOCK(AREAX) FREEMTLOCK(mmm->Areas[AREAX].AreaCBPtr->areaLock)
#else
#define CURRENTMMM (gpi->mmm)
#define GETMMMFROMAREAX(MMM,AREAX) MMM = gpi->mmm ;
#define GRABAREAMTLOCK(AREAX) TRUE
#define FREEAREAMTLOCK(AREAX) TRUE
#endif

#define FINDAREAINPROCESS(INDEX,AREAID) \
 mmm = CURRENTMMM ; \
 INDEX = AREAID ; \
 if (mmm->Areas[INDEX].mmmReal != NULL) mmm = mmm->Areas[INDEX].mmmReal ;
 
// for(INDEX=0;INDEX<mmm->NumAreas;INDEX++) { if (mmm->Areas[INDEX].AreaId == AREAID) break ; } ; \
// if (INDEX >= mmm->NumAreas) INDEX = UNUSED ; \
// if (INDEX == UNUSED ? FALSE : mmm->Areas[INDEX].mmmReal != NULL) mmm = mmm->Areas[INDEX].mmmReal ;

//#define V4MM_Area_Max 150		/* Max number of concurrent areas */
#define V4MM_AreaBkt_Max 200		/* Max number of concurrent buckets */

struct V4MM__MemMgmtMaster
{
  struct V4CS__RemoteFileRef *rfr ;	/* If not NULL then links to Client-Server info */
  LENMAX MaxAllocSize ;			/* Max number of bytes we have to allocate */
  LENMAX CurTotalBytes ;		/* Number of bytes chewed up in buckets below */
  COUNTER TotalCalls ;			/* Total calls to Bucket Mgmt Modules */
  PID MyPid ;
  int UserJobId ;			/* Application specific job id/number */
#ifdef WINNT				/* NOTE: for WINNT, MyPid is unique ThreadID */
  int MyPidLoginDT ;			/* Hold thread login/create date-time to uniquely id thread */
  HANDLE MyhThread ;			/* My thread handle */
  DWORD MyidProcess ;			/*   and process ID */
#endif
  short NumAreas ;			/* Highest used area slot below */
  struct
   { struct V4MM__MemMgmtMaster *mmmReal ; /* If not NULL then point to 'real' mmm for this area (ex: temp V4 work area shared by multiple threads) */
     struct V4IS__AreaCB *AreaCBPtr ;	/* Pointer to Area Top-Level Info (nil if slot empty) */
     struct V4DPI__AreaBindInfo *abi ; /* If non NULL then points to binding info for the area */
     struct V4IS__Bkt *FreeBkt1 ; 	/* Point to freed (flushed) bucket space */
     struct V4IS__Bkt *FreeBkt2 ;
#ifdef WINNT
     HANDLE hfile ;
#else
     FILE *FilePtr ;			/* File associated with this area */
#endif
     AREAID AreaId;			/* Area ID for this area (0 if slot is free) */
     int BktSize ;			/* Size of bucket for this area */
     int BktReads,BktWrites ;		/* Number of calls to BktRead/BktWrite */
     short MinBkts ;			/* Minimum number of buckets for this area */
     short MaxBkts ;			/* Maximum number of buckets for this area */
     short NumBkts ;			/* Number of buckets allocated below for this area */
     short WriteAccess ;		/* If TRUE then process has write access to this area */
     UCCHAR UCFileName[V_FileName_Max] ;	/* File associated with this area */
     COUNTER IndexOnlyCount ;		/* Number of index-only areas below */
     struct {
       LENMAX BktOffset ;		/* Bucket offset for this area */
#ifdef WINNT
       HANDLE iohfile ;
#else
       FILE *ioFilePtr ;		/* File associated with this area */
#endif
      } IndexOnly[V4IS_IndexOnly_Max] ;
     SEGID SegId ;
     SEGKEY SegKey ;			/* Sharable segment key */
     struct V4MM__GlobalBuf *GBPtr ;	/* If sharable then pointer to shared global buffer */
     struct V4MM__WriteVerBuf *wvb ;	/* If non-null then points to global Write Verify Buffer */
     V4LOCKX LockIDX ;			/* Index into lock table for current index lock (can only have 1) */
     V4LOCKX LockDAT ;			/* Index into lock table for current data lock */
     V4LOCKX LockRec ;			/* Index into lock table for current record */
     V4LOCKX LockTree ;			/* Index into lock table if entire index tree locked */
     V4LOCKX LockExtra ;		/* Index into temp/extra lock (like for index bucket splits) */
   } Areas[V4MM_Area_Max] ;
  short NumBkts ;			/* Number of buckets below */
  struct
   { int BktNum ;			/* Bucket number */
     struct V4IS__Bkt *BktPtr ;		/* Pointer to the bucket (0 if slot is free) */
     int CallCnt ;			/* Last call counter (used to swap out oldest bucket) */
     short AreaIndex ;			/* Index to area above */
     short Updated ;			/* TRUE if bucket has been updated */
     short MemLock ;			/* Memory Locked - do not move this bucket */
   } Bkts[V4MM_AreaBkt_Max] ;
} ;

/*	L O C K   M A N A G E M E N T								*/

#define V4MM_WriteVerBuf_Max 0xc000	/* Max number of bytes in WriteVerBuf */
struct V4MM__WriteVerBuf
{ BYTE WriteCnt[V4MM_WriteVerBuf_Max] ;	/* Slot for each possible block in area */
} ;

#define V4MM_GlobalBufBkt_Max 25	/* Max number of global buffers */
#define V4MM_LockTableEntry_Max 75	/* Max number of locks per file */
#define V4MM_GlobalBufUpd_Max 30	/* Max number of DataId updates to track at a time */
#define V4IS_Lockout_Wait 1000000000	/* Lockout area from access- user waits */
#define V4IS_Lockout_Error 1000000001	/* Lockout area from access- user gets error */
#define V4IS_Lockout_WaitReadOK 1000000002 /* Lockout area from access- user allowed to get, put places user in wait */
#define V4MM_GlobalBuf_Max V4MM_GlobalBufBkt_Max*32000 + V4MM_LockTableEntry_Max*50

#define UNUSED -1			/* Marks unused bucket */
#define PENDING -2			/* Bucket about to be used (nothing in it, but don't use it) */
#define LOCKPERM 1			/* Permanent lock */
#define LOCKTEMP 2			/* Temporary lock */

#define UNUSEDPID (PID)UNUSED

/* #define V4MM_GlobalTrace 25000	*/

struct V4MM__GlobalBuf			/* Definition of global buffer for shared file access	*/
{ int BktCnt ;				/* Number of buckets below */
  int BktSize ; 			/* Number of bytes in each bucket */
  int PanicLockout ;			/* If > 0 then area in panic lockout mode, decrement 1 & try later */
  PID PanicPid ;			/* Pid declaring panic lockout */
  int TotalCalls ;			/* Total number of calls for buckets */
  int TotalTreeLocks ;			/* Total number of locks on entire index tree (so other users can re-keypos if necessary) */
#ifdef V4MM_GlobalTrace
  int LastTrace ;			/* Used to track last trace (index via LastTrace % V4MM_GlobalTrace) */
  struct {
    int LockId,CallCnt ;
    PID Pid ;
    short BBktNum,sbhBktNum,glbBktNum ;
    BYTE bx ;
    BYTE Where ;			/* Where entry was made */
   } Trace[V4MM_GlobalTrace] ;
#endif
  int LastDataIdUpdCnt ;		/* Used to cycle thru structure below */
  struct
   { int DataIdRmv ;			/* DataId being removed for update (maybe replaced with same/new dataid) */
     int DataIdNew ;			/* New DataId for above */
   } DataUpd[V4MM_GlobalBufUpd_Max] ;
  struct
   { int BktNum ;			/* Bucket Number (-1 indicates empty/free) */
     int PendingBktNum ;		/* Bucket Number if pending (waiting for IO) */
     PID PendingPid ; 			/* Pid with pending IO (in case it gets zapped before completion) */
     int CallCnt ;			/* Last call count - used to swap out oldest */
     short Updated ;			/* TRUE if bucket has been updated */
     short MemLock  ;			/* If nonzero then: Memory Locked - do not move this bucket */
   } Bkt[V4MM_GlobalBufBkt_Max] ;
  int LockTableOffset ; 		/* Offset below to lock table */
  LONGINT BktOffset ;			/* Offset below to first bucket (&bkt[n] = &Buffer[BktOffset + n*BktSize]) */
					/* Force Buffer to max alignment */
  BYTE Buffer[V4MM_GlobalBuf_Max] ;	/* Buffer for locks & buckets */
} ;

#define V4MM_LockId_Tree -1
#define V4MM_LockId_Alloc -2
#define V4MM_LockId_Root 0

#define V4MM_LockIdType_Record 1	/* LockId is DataId, used to lock a particular record */
#define V4MM_LockIdType_Data 2		/* LockId is bucket number, used to lock a particular data bucket */
#define V4MM_LockIdType_Index 3 	/* LockId is bucket number, used to lock an index bucket */
#define V4MM_LockIdType_Tree 4		/* LockId is V4MM_LockId_Tree, used to lock entire index tree on splits */
#define V4MM_LockIdType_Alloc 5 	/* Lock down buffer allocation (no buffers can be (re)allocated) */
#define V4MM_LockIdType_Root 6		/* Lock down header info in root bucket */
#define V4MM_LockIdType_User 7		/* User specified lock on file (see V3 io_misc(unit,/manual_xxx/)) */

#define V4MM_LockMode_Read 1		/* Read lock */
#define V4MM_LockMode_Write 2		/* Write lock */
#define V4MM_LockMode_AnythingGoes 3	/* Dummy mode for pending lock, compatible with read & write */

#define V4MM_Lock_ReleasePid -998	/* Special lockx value for v4mm_LockRelease to release all locks for current pid */

#define V4MM_LockTable_PidMax 300	/* Max number of users in a lock table */
#define V4MM_LockTable_PidsPerLock 10	/* Max number of Pids for any given lock */

struct V4MM__LockTable			/* Lock info for shared file */
{ int Count ;				/* Number of locks active below */
  int MaxAllowed ;			/* Max number we have allocated for */
  DCLSPINLOCK IsActive ;		/* -1 if available, 0 or greater if in use (ActivePid set below) */
  int LastUniqueLockId ;		/* Last Unique Lock ID used below */
  PID ActivePid ;
  int NestLockCount ;			/* Tracks nested calls to RELLT for current pid */
  unsigned short LocksIDX ;		/* Total number of index locks below (with mode=Write) */
  unsigned short LocksTree ;		/* Total number of locks on entire index tree (with mode=Write) */
  unsigned short PidCnt ;		/* Number of Pids attached to this lock table */
  PID LogPids[V4MM_LockTable_PidMax] ;	/* List of active pids attached to this table */
#ifdef WINNT				/* NOTE: for WINNT LogPids contains list of ThreadIDs */
  int PidLoginDTs[V4MM_LockTable_PidMax] ;	/* Hold thread login/create date-time to uniquely id thread */
  HANDLE hThreads[V4MM_LockTable_PidMax] ; /* Handles for threads */
  DWORD idProcesses[V4MM_LockTable_PidMax] ;	/* IDs for processes */
#endif
  struct
   { int LockId ;			/* Lock Id (DataId, bucket number, V4MM_LockId_xxx) */
     int UniqueLockId ; 		/* Unique lock id assigned by lock manager */
     PID Pids[V4MM_LockTable_PidsPerLock] ; /* Process ID (used to determine hung locks) */
     PID WaitPids[V4MM_LockTable_PidsPerLock] ;	/* List of pids waiting for the lock */
     int AreaIds[V4MM_LockTable_PidsPerLock] ;	/* Corresponding AreaIds (in case pid has area opened twice!) */
     unsigned short WaitCount ; 	/* Number of Pids waiting for the lock */
     BYTE WaitLockModes[V4MM_LockTable_PidsPerLock] ;
     BYTE PidCount ;			/* Number of pids holding lock */
     BYTE LockIdType ;			/* Type of lock, V4MM_LockIdType_xxx */
     BYTE LockMode ;			/* Type of lock, V4MM_LockMode */
     BYTE Source ;			/* Source of lock */
   } Entry[V4MM_LockTableEntry_Max] ;
} ;

/*	R E F O R M A T T I N G   S T R U C T U R E S						*/


#define V4IS_RefHash_Max 250007 			/* Default number of hash entries for reformat */
							/* Other possible values: 600011, 750019, 1000003 */
#define V4IS_RefHash_Limit (V4IS_RefHash_Max*9)/10	/* Max number in table (90% to allow for hash conflicts) */

struct V4IS__RefHash
{ COUNTER Count ;
  LENMAX HashMax ; 					/* Max number allocated (may be > RefHash_Max!) */
  LENMAX HashLimit ;					/* Max number of entries allowed (s/b 90% of max) */
  struct { DATAID odataid,ndataid ; } entry[V4IS_RefHash_Max] ;
} ;

struct V4IS__RefResults 		/* Updated with results of reformat */
{ COUNTER DataIdCnt ;			/* Number of data records reformatted */
  COUNTER ObsoleteCnt ;			/* Number of obsolete records */
  COUNTER KeyCnt ;			/* Total number of keys reformatted */
  INDEX MaxLevels ;			/* Max number of levels in directory tree */
  COUNTER BktReads,BktWrites ;		/* Number of calls to BktRead/Write */
} ;

/*	P O I N T S ,  D I M E N S I O N S ,  &   I N T E R S E C T I O N S			*/

//#define V4Area_DfltBktSize 0x8fff	/* Default V4 area bucket size (36836) */
#define V4Area_DfltBktSize V4AreaAgg_DfltBktSize	/* Default V4 area bucket size */
#define V4Area_UsableBytes (V4AreaAgg_DfltBktSize-128)	/* Guarenteed bytes available for whatever */

#define V4DPI_PntType_Int 0		/* Integer Point Value */
#define V4DPI_PntType_Char 1		/* Character String */
#define V4DPI_PntType_Dict 2		/* Dictionary Entry */
#define V4DPI_PntType_Real 3		/* Real Number */
#define V4DPI_PntType_Isct 4		/* Intersection (Grouping is number of points in the intersection) */
#define V4DPI_PntType_SymDef 5		/* $symbol definition/reference */
#define V4DPI_PntType_Special 6 	/* Special points (e.g. {ALL}, {CURRENT}) */
#define V4DPI_PntType_Shell 7		/* This is a shell for other points */
#define V4DPI_PntType_FrgnDataEl 8	/* Field specification for foreign file */
#define V4DPI_PntType_FrgnStructEl 9	/* Structure specification for a foreign file */
#define V4DPI_PntType_BinObj 10 	/* Binary Object (must be aligned to 32 bits) */
#define V4DPI_PntType_Tree 11		/* An internal V4 'tree' */
//#define V4DPI_PntType_V3Mod 11		/* V3 Module Call */
//#define V4DPI_PntType_V3ModRaw 12	/* V3 Module Call with no translation (points passed "as is") */
//#define V4DPI_PntType_Context 13	/* A context point */
#define V4DPI_PntType_List 14		/* A "special" (internally maintained) list */
//#define V4DPI_PntType_Pragma 15 	/* A point which holds junk for IntMods, not to be pushed onto context */
#define V4DPI_PntType_IntMod 16 	/* A point which evaluates via internal (to V4) module */
#define V4DPI_PntType_PntIdx 17 	/* A point which points to internal point buffer ("expanded" by V4) */
#define V4DPI_PntType_MemPtr 18 	/* A memory pointer */
//#define V4DPI_PntType_V3PicMod 19	/* A V3 PIC module (module stored in same area as point?) */
#define V4DPI_PntType_BigText 20	/* A big text string (see V3PicMod above) */
#define V4DPI_PntType_Unused 21 	/* unused - (formally PIntMod) */
#define V4DPI_PntType_UTime 22		/* Universal time - time of day in seconds */
//#define V4DPI_PntType_Binding 23	/* Binding info - used for erasing/tracking bindings */
#define V4DPI_PntType_Logical 24	/* Logical (Yes/No) */
//#define V4DPI_PntType_Decomp 25 	/* Special type for decomposition of isct's */
#define V4DPI_PntType_UDate 26		/* Universal date format */
#define V4DPI_PntType_UMonth 27 	/* Universal month format */
#define V4DPI_PntType_Int2 28		/* 2*32 bit integer */
//#define V4DPI_PntType_Hashed 29 	/* String hashed to a number */
#define V4DPI_PntType_UDT 30		/* Universal date-time */
#define V4DPI_PntType_OSHandle 31	/* Operating System Handle */
//#define V4DPI_PntType_AggVal 32 	/* AggVal point */
#define V4DPI_PntType_UPeriod 33	/* Financial period (1-13) per year */
#define V4DPI_PntType_UQuarter 34	/* Financial quarter (1-4) per year */
#define V4DPI_PntType_UWeek 35		/* Financial week (1-52) per year */
#define V4DPI_PntType_QIsct 36		/* Quoted Intersection (temp point type used by MakeQIsct() - reset in IsctEval) */
#define V4DPI_PntType_AggRef 37 	/* Local/Temp point created to scan through Multi-Area aggregates */
#define V4DPI_PntType_TagVal 38 	/* A tagged value: tag::value */
#define V4DPI_PntType_SSVal 39		/* Special spread sheet value: !REF?, !UNK?, ... */
#define V4DPI_PntType_XDB 40		/* An ODBC connection/table, whatever */
#define V4DPI_PntType_UYear 41		/* Universal Year */
#define V4DPI_PntType_Delta 42		/* A delta (+ or -) */
#define V4DPI_PntType_V4IS 43		/* V4IS Area */
#define V4DPI_PntType_BigIsct 44	/* A big isct (won't fit in V4DPI__Point) */
#define V4DPI_PntType_ParsedJSON 45	/* Internal-only point type from 'Context ADV json:.....' to indicate JSON string has already been parsed */
#define V4DPI_PntType_Fixed 46		/* Fixed decimal */
#define V4DPI_PntType_Time 47		/* Time/Transaction - Bindings searched in reverse "chronological" order */
#define V4DPI_PntType_IMArg 48		/* Represents argument to intmod */
#define V4DPI_PntType_FChar 49		/* Character String with "embedded" PointType format (see LHSCnt) */
#define V4DPI_PntType_TeleNum 50	/* Telephone Number (stored as Int2: <ac><exchange> <number><extension> */
#define V4DPI_PntType_UOM 51		/* Unit-of-Measure Amount */
#define V4DPI_PntType_UOMPer 52 	/* Quantity per UOM */
#define V4DPI_PntType_XDict 53		/* External Dictionary Entry */
#define V4DPI_PntType_GeoCoord 54	/* Geographic Coordinate */
#define V4DPI_PntType_Complex 55	/* Complex Number */
#define V4DPI_PntType_URL 56		/* Uniform Resource Locator */
#define V4DPI_PntType_MDArray 57	/* Multi-dimensional array (IntVal is handle for (struct V4IM__MDArray *)) */
#define V4DPI_PntType_Color 58		/* A color (24 bit RGB, links to Excel color picker & HTML color names) */
#define V4DPI_PntType_CodedRange 59	/* Integer with coded ranges */
#define V4DPI_PntType_RegExpPattern 60	/* Regular Expression Pattern */
#define V4DPI_PntType_Tag 61		/* Tag Point (as opposed to TagVal above) */
#define V4DPI_PntType_Calendar 62	/* Calendar (date + fractional day as (double)) */
#define V4DPI_PntType_Country 63	/* Country */
#define V4DPI_PntType_SegBitMap 64	/* Segmented bitmap (flipped to PntType_List internally) */
#define V4DPI_PntType_Drawer 65 	/* A "drawer" (container holding whatever) */
#define V4DPI_PntType_UOMPUOM 66 	/* UOM per UOM (ex: 5lbs per 100 each) */
#define V4DPI_PntType_UCChar 67		/* Unicode character string */

//#define V4DPI_PntType_MIDASZIP 998	/* Dummy point type for passing UZIP from MIDAS to V4 via MIDAS-SQL */
//#define V4DPI_PntType_MIDASUDT 999	/* Dummy point type for passing UDT from MIDAS to xv4rpp */

#define V4DPI_PntType_Last 70		/* Last defined point type index */
struct V4__PTBitMap
 { int Map[3] ; 			/* Bitmap big enough to hold all defined point types */
 } ;

#define CASEofChar \
	case V4DPI_PntType_Char: \
	case V4DPI_PntType_FChar: \
	case V4DPI_PntType_URL: \
	case V4DPI_PntType_BigText: \
	case V4DPI_PntType_UCChar:

#define CASEofCharMBT \
	case V4DPI_PntType_Char: \
	case V4DPI_PntType_FChar: \
	case V4DPI_PntType_URL: \
	case V4DPI_PntType_UCChar:

#define CASEofCharmU \
	case V4DPI_PntType_Char: \
	case V4DPI_PntType_FChar: \
	case V4DPI_PntType_URL:

#define CASEofINT \
	case V4DPI_PntType_UDT: \
	case V4DPI_PntType_UDate: \
	case V4DPI_PntType_UWeek: \
	case V4DPI_PntType_UMonth: \
	case V4DPI_PntType_UPeriod: \
	case V4DPI_PntType_UQuarter: \
	case V4DPI_PntType_UYear: \
	case V4DPI_PntType_Time: \
	case V4DPI_PntType_Logical: \
	case V4DPI_PntType_OSHandle: \
	case V4DPI_PntType_Delta: \
	case V4DPI_PntType_SegBitMap: \
	case V4DPI_PntType_IMArg: \
	case V4DPI_PntType_CodedRange: \
	case V4DPI_PntType_Int:

#define CASEofINTmT \
	case V4DPI_PntType_Time: \
	case V4DPI_PntType_Logical: \
	case V4DPI_PntType_OSHandle: \
	case V4DPI_PntType_Delta: \
	case V4DPI_PntType_SegBitMap: \
	case V4DPI_PntType_IMArg: \
	case V4DPI_PntType_CodedRange: \
	case V4DPI_PntType_Int:

#define CASEofDBL \
	case V4DPI_PntType_Real: \
	case V4DPI_PntType_UTime: \
	case V4DPI_PntType_Calendar:

#define V4DPI_PntFormat_Real UClit("%.15g")	/* c format string to display PntType_Real values */

#define V4DPI_PntVerify_InArea 1	/* Verify point in Area */
#define V4DPI_PntVerify_Dict 2		/* Verify point in dictionary */
#define V4DPI_PntVerify_Range 3 	/* Point must be within range */
#define V4DPI_PntVerify_Ref 4		/* Verify point as REF in another area */

#define V4DPI_Grouping_Single 0 	/* Single Point */
/* Numbers 1-12 indicate a mix-list of so many entries */
#define V4DPI_Grouping_MaxMix 100	/* Max number for mix-list of points (anything greater is a special point below) */
#define V4DPI_Grouping_All 115		/* Point represents all points in dimension (e.g. dim=all) */
#define V4DPI_Grouping_Current 116	/* Point represents current value of dimension in context */
#define V4DPI_Grouping_Now 119		/* Point represents "current" time */
//#define V4DPI_Grouping_LastBind 120	/* Point represents bind point of last binding */
#define V4DPI_Grouping_Undefined 121	/* Point represents "undefined" point (used for Context Add dim:{undefined}) */
//#define V4DPI_Grouping_DecompRHS 122	/* Point represents dim? on RHS of a bind (i.e. substitute in on isct) */
//#define V4DPI_Grouping_DecompLHS 123	/* Point represents dim? on LHS of a bind (i.e. match on rest of dims in isct) */
#define V4DPI_Grouping_Sample 124	/* Point represents a sample point on dimension */
#define V4DPI_Grouping_LT 125		/* Point represents all points LT this one */
#define V4DPI_Grouping_LE 126
#define V4DPI_Grouping_GE 127
#define V4DPI_Grouping_GT 128
#define V4DPI_Grouping_NE 129
#define V4DPI_Grouping_New 130		/* New point to be created with each reference */
#define V4DPI_Grouping_PCurrent 131	/* Prior current point (1 behind in context) */
#define V4DPI_Grouping_None 132 	/* Dimension with "no" value (i.e. dim:none) */
#define V4DPI_Grouping_AllCnf 133	/* Same as V4DPI_Grouping_All except "confirming" ok to use in dim.. within binding (no warning msgs) */
/*	ISGroupingEval(point) is TRUE if special point should be evaluated */
#define SPECIALToEVAL(POINT) ((POINT)->Grouping == V4DPI_Grouping_Current || (POINT)->Grouping == V4DPI_Grouping_Now || (POINT)->Grouping == V4DPI_Grouping_New || (POINT)->Grouping == V4DPI_Grouping_PCurrent)

#define V4DPI_AlphaVal_Max 0x800 	/* Max bytes in a point value */
#define V4DPI_UCVal_Max ((int)(V4DPI_AlphaVal_Max/sizeof(UCCHAR))) 	/* Max bytes in a UCCHAR value */
#define V4DPI_UCVAL_MaxSafe (V4DPI_UCVal_Max - 5)
#define V4DPI_PointHdr_Bytes 8		/* Number of bytes in point header */
#define V4DPI_MDArray_Max 16		/* Max number of dimensions in MDArray */
#define V4DPI_Fixed_MaxDecimals 8	/* Max number of fixed decimals places */
#define V4DPI_AlphaUC_SortValMax 255	/* Max number of characters to sort (first argument of sort) */
#define V4DPI_AlphaUC_SortByMax 100	/* Max number of characters to sort in By/Reverse tag */

#define V4DPI_FormatOpt_ShowDim 0x1	/* Show "dim:" in output (except Alpha/List points) */
#define V4DPI_FormatOpt_ShowBytes 0x2	/* Show bytes required to store point */
#define V4DPI_FormatOpt_Echo 0x4	/* Format for Echo() (no dim, expand lists, no bytes, replace "none" with value) */
#define V4DPI_FormatOpt_ListNL 0x8	/* Separate list elements with NL */
#define V4DPI_FormatOpt_NoDisplayer 0x10 /* Don't expand out "displayer" dimensions (prevents endless recursion!) */
#define V4DPI_FormatOpt_ShowCtxVal 0x20 /* Show value of dim* dimensions */
#define V4DPI_FormatOpt_AlphaQuote 0x40 /* Show alpha/char dimensions in quotes */
#define V4DPI_FormatOpt_ShowDimAlways 0x80 /* Show "dim:" ALWAYS */
#define V4DPI_FormatOpt_ListInParens 0x100 /* Show lists enclosed in parens: () */
#define V4DPI_FormatOpt_DisplayerTrace 0x200 /* We are in 'Context Examine' - use [UV4:displayerTrace] if we got it */
#define V4DPI_FormatOpt_Point 0x400	/* Display as parsable text point (used by Format(pt Point?)) */
#define V4DPI_FormatOpt_Trace (V4DPI_FormatOpt_ShowDim | V4DPI_FormatOpt_NoDisplayer | V4DPI_FormatOpt_ShowCtxVal | V4DPI_FormatOpt_AlphaQuote | V4DPI_FormatOpt_DisplayerTrace)
#define V4DPI_FormatOpt_Dump (V4DPI_FormatOpt_ShowDim | V4DPI_FormatOpt_NoDisplayer | V4DPI_FormatOpt_AlphaQuote | V4DPI_FormatOpt_ListInParens | V4DPI_FormatOpt_DisplayerTrace)
#define V4DPI_FormatOpt_Default 0

#define V4DPI_PointParse_LHS 1		/* Left hand side of a bind */
#define V4DPI_PointParse_NestedDot 2	/* Nested call to PointParse for "xxx.yyy" parsing */
//#define V4DPI_PointParse_CacheIsct 8	/* Mark iscts with Cache Id */
#define V4DPI_PointParse_InV4Exp 0x10	/* Parsing a { ... } expression */
#define V4DPI_PointParse_RetFalse 0x20	/* Return FALSE if syntax (no v4_error() call) */
#define V4DPI_PointParse_InColonEq 0x40 /* Parsing point xxx in dim:=xxx (don't want to include possible "/" as part of xxx */
#define V4DPI_PointParse_NoMult 0x80	/* Don't allow multiple values (v..v or v,v,...) */
//#define V4DPI_PointParse_BindVal 0x100	/* Point immediately follows binding isct - allow leading "=" */
#define V4DPI_PointParse_MultiLine 0x200 /* Point specification may be multiple lines */
#define V4DPI_PointParse_Pattern 0x400	/* Parsing a pattern - allow duplication dimensnions */
#define V4DPI_PointParse_NotADim 0x800	/* Parse xxx:value as (xxx rel value) if xxx not a dimension */
#define V4DPI_PointParse_InJSON 0x1000	/* Within $(...) - don't treat x.y as [x* y] */

#pragma pack(4)
struct V4DPI__Value_UOM
{ short Ref ;				/* Unique UOM Number/Id/Ref */
  short Index ; 			/* Index within UOM */
  double Num ;				/* Numeric value - ALWAYS in base units (Index = 1) */
} ;
struct V4DPI__Value_UOMPer
{ short Ref ;				/* Unique UOM Number/Id/Ref */
  short Index ; 			/* Index within UOM */
  double Num ;				/* Numeric value - ALWAYS in base units (Index = 1) */
  double Amount ;			/* Amount that the above is "per" (ex: 2.98/dozen: Amount=2.98, Num=12) */
} ;
struct V4DPI__Value_UOMPUOM
{ DIMID uomDim, puomDim ;		/* Dimensions of two source UOMs */
  struct V4DPI__Value_UOM UOM ;		/* Top UOM amount */
  struct V4DPI__Value_UOM PUOM ;	/* Bottom (or "per") UOM amount */
} ;
#pragma pack()

struct V4DPI__Value_Complex
{ double r,i ;				/* Real & imaginary values */
} ;

#define V4DPI__Tele_IDCSpecial 0xffff	/* International Dial Code for special acceptor/displayers on telephone */

#define V4DPI_TeleType_Home 1
#define V4DPI_TeleType_Work 2
#define V4DPI_TeleType_Cell 3
#define V4DPI_TeleType_Fax 4

struct V4DPI__Value_Tele		/* Telephone numbers */
{ short IntDialCode ;			/* International country code (if negative then have phone type and/or extension) */
  unsigned short AreaCode ;		/* Area/City code */
  unsigned int Number ; 		/* The actual number */
  unsigned char Type ;			/* Type (use) of phone number (see V4DPI_TeleType_xxx_) */
  unsigned int Extension : 24 ;		/* Optional extension */
} ;

struct V4DPI__PntV4IS			/* Structure of point Value for V4IS points */
{ 
  int Handle ;				/* Handle within V4IS__V4Areas structure */
  DATAID DataId ;			/* If not UNUSED then DataId of record */
#ifdef VMSOS
  int DataId2 ;
#endif
  INDEX SubIndex ;			/* If not UNUSED then index to substructure */
} ;

union V4DPI__IsctSrc
{ int iVal ;
 struct {
   BYTE HNum ;				/* Area hierarchy number */
   BYTE vcdIndex ;			/* Index into area's V4LEX__CompileDir.File[] structure */
   unsigned short lineNumber ;		/* Line number within file referenced by sfIndex */
  } c ;					/* Components */
} ;

struct V4DPI__Value_Isct
{ 
  union V4DPI__IsctSrc vis ;		/* Source of this intersection */
  BYTE pntBuf[0] ;			/* Remainder of structure for holding contiguous points in Isct */
} ;

struct V4DPI__Value_BigIsct
{ union V4DPI__IsctSrc vis ;		/* Source of this intersection (THIS MUST BE FIRST TO ALIGN WITH V4DPI__Value_Isct!) */
  DIMID DimUID ;			/* Unique Id for this BigIsct (used to form key to get V4DPI__BigIsct record) */
} ;

struct V4DPI__Value_SymDef		/* Symbol definition/reference */
{ SYMID symId ;				/* Unique symbol Id */
  DICTID dictId ;			/* Dictionary Id value in case we want to 'display' this point */
  ETYPE lsOp ;				/* 0 = reference, 1 = plus-equal reference, 2 = minus-equal reference */
  BYTE pntBuf[4] ;			/* Symbol's value (a point) if defining, not present if just a reference (can tell from point.Bytes) */
} ;

/*	INITISCT - Standard Isct initialization. Sets PNT->Bytes to size of Isct with 0 points (i.e. add point sizes into PNT->Bytes) */
#define INITISCT(PNT) ZPH(PNT) ; (PNT)->PntType = V4DPI_PntType_Isct ; (PNT)->Bytes = (V4DPI_PointHdr_Bytes + sizeof(union V4DPI__IsctSrc))  ;
#define INITISCT2(DST,SRC) { memcpy((DST),(SRC),V4DPI_PointHdr_Bytes) ; (DST)->Value.Isct.vis.iVal = (SRC)->Value.Isct.vis.iVal ; }
#define ISCTVCD(PNT) \
 if (tcb->ilvl[tcb->ilx].vcdIndex != UNUSED) \
  { (PNT)->Value.Isct.vis.c.vcdIndex = tcb->ilvl[tcb->ilx].vcdIndex ; \
    (PNT)->Value.Isct.vis.c.lineNumber = tcb->ilvl[tcb->ilx].current_line * 10 ; \
    if (tcb->ilvl[tcb->ilx].current_line == tcb->ilvl[tcb->ilx].lastVCDLine) \
     { ++tcb->ilvl[tcb->ilx].subVCDLine ; (PNT)->Value.Isct.vis.c.lineNumber += (tcb->ilvl[tcb->ilx].subVCDLine < 10 ? tcb->ilvl[tcb->ilx].subVCDLine : 9) ; } \
     else { tcb->ilvl[tcb->ilx].lastVCDLine = tcb->ilvl[tcb->ilx].current_line ; tcb->ilvl[tcb->ilx].subVCDLine = 0 ; } ; \
    (PNT)->Value.Isct.vis.c.HNum = (gpi->ppHNum >= gpi->LowHNum ? gpi->ppHNum : V4DPI_WorkRelHNum) ; \
  } else { memset(&(PNT)->Value.Isct.vis,0,sizeof (PNT)->Value.Isct.vis) ; } ;
/*	NOTE: Must duplicate above. Only difference is tcb->ilx is given as argument */
#define ISCTVCDX(PNT,INDEX) \
 if (tcb->ilvl[INDEX].vcdIndex != UNUSED) \
  { (PNT)->Value.Isct.vis.c.vcdIndex = tcb->ilvl[INDEX].vcdIndex ; \
    (PNT)->Value.Isct.vis.c.lineNumber = tcb->ilvl[INDEX].current_line * 10 ; \
    if (tcb->ilvl[INDEX].current_line == tcb->ilvl[INDEX].lastVCDLine) \
     { ++tcb->ilvl[INDEX].subVCDLine ; (PNT)->Value.Isct.vis.c.lineNumber += (tcb->ilvl[INDEX].subVCDLine < 10 ? tcb->ilvl[INDEX].subVCDLine : 9) ; } \
     else { tcb->ilvl[INDEX].lastVCDLine = tcb->ilvl[INDEX].current_line ; tcb->ilvl[INDEX].subVCDLine = 0 ; } ; \
    (PNT)->Value.Isct.vis.c.HNum = (gpi->ppHNum >= gpi->LowHNum ? gpi->ppHNum : V4DPI_WorkRelHNum) ; \
  } else { memset(&(PNT)->Value.Isct.vis,0,sizeof (PNT)->Value.Isct.vis) ; } ;
#define NOISCTVCD(PNT) memset(&(PNT)->Value.Isct.vis,0,sizeof (PNT)->Value.Isct.vis)

/*	ISCT1STPNT - Returns pointer to address of first point in Isct */
#define ISCT1STPNT(PNT) ((P *)&(PNT)->Value.Isct.pntBuf[0])
#define ISCTLEN(ISCT,NXTPNT) ((ISCT)->Bytes = ((char *)(NXTPNT) - (char *)(ISCT)))
/*	ADVPNT - Increment PNT pointer to memory just beyond it, ADVPNT advances PNT pointer by BYTES */
#define ADVPNT(PNT) (PNT = (P *)(((char *)PNT) + PNT->Bytes))
#define ADVPNT2(PNT,BYTES) (PNT = (P *)(((char *)PNT) + ALIGN(BYTES)))

#define ISCTSETNESTED(ISCT,TPNT) \
  switch ((TPNT)->PntType) \
   { case V4DPI_PntType_Isct: case V4DPI_PntType_BigIsct: case V4DPI_PntType_PntIdx: case V4DPI_PntType_SymDef:		(ISCT)->NestedIsct = TRUE ; break ; \
     case V4DPI_PntType_Special: \
	switch ((TPNT)->Grouping) \
	 { case V4DPI_Grouping_Current: case V4DPI_Grouping_PCurrent: case V4DPI_Grouping_Now:	(ISCT)->NestedIsct = TRUE ; break ; \
	 } ; \
   } ;
/*	Sets point Bytes for points with Grouping <> Single */
#define SETBYTESGRPINT(POINT) (POINT)->Bytes = ((POINT)->Grouping == 0 || (POINT)->Grouping > V4DPI_Grouping_MaxMix ? V4PS_Int : V4DPI_PointHdr_Bytes + ((POINT)->Grouping * (sizeof(int)*2))) ;
#define SETBYTESGRPDBL(POINT) (POINT)->Bytes = ((POINT)->Grouping == 0 || (POINT)->Grouping > V4DPI_Grouping_MaxMix ? V4PS_Real : V4DPI_PointHdr_Bytes + ((POINT)->Grouping * (sizeof(double)*2))) ;

#define MEANEARTHRADIUS 6371		/* Earth's radius in KM (mean) (from Enc. Britannica) */
#define KMtoMILE 0.62137
#define METERSinNAUTICALMILE 1852
#define GregorianEpoch 1
#define MeanTropicalYear 365.242189
#define IDIVFLOOR(x,y) (int)floor((double)x / (double)y)
#define IMOD(x,y) (x - y * (IDIVFLOOR(x,y)))	/* Mod function for integers */
#define MOD(x,y) ((x) - (y) * (floor((double)(x) / (double)(y))))
#define SIGNUM(x) (x < 0 ? -1 : (x > 0 ? 1 : 0))
#define ABS(x) (x < 0 ? -x : x)

#define V4DPI_GCType_LatLonMeter 0	/* Coord1/2 = Lat & Longitude in degrees * 1000000, Height in meters */
#define V4DPI_GCType_Distance 1		/* Distance/Speed/Velocity - see struct V4DPI__Value_DSV */
#define V4DPI_GCType_None 15		/* Null / Empty point */

#define V4DPI_GeoCoord_NoBearing 999	/* Indicates no azimuth or elevation */

#define V4DPI_GeoCoord_Factor 1000000.0 /* Factor multiplier for Coord1/Coord2 */
#define V4DPI_GeoCoordDist_Factor 1000000.0 /* distFrac conversion factor */

#define V4DPI_GeoCoordCoord3_Null 0	/* Indicates null/non value for height */
#define V4DPI_GeoCoordCoord3_Max 1000000
#define V4DPI_GeoCoordCoord3_Offset 1000 /* This is added onto offset to allow for negative height */
#define V4DPI_GeoCoordCoord3_Scale 10.0	/* Scaling factor */

#define V4DPI_GeoCoordDist_Meter 0	/* Meters */
#define V4DPI_GeoCoordDist_KMeter 1	/* Kilometers */
#define V4DPI_GeoCoordDist_Mile 2	/* Miles */
#define V4DPI_GeoCoordDist_Feet 3	/* Feet */
#define V4DPI_GeoCoordDist_Yard 4	/* Yards */
#define V4DPI_GeoCoordDist_NautMile 5	/* Nautical mile */
#define V4DPI_GeoCoordDist_MaxVal 5	/* Maximum allowed value */

#define V4DPI_GeoCoordSpeed_None 0	/* not a speed, just a distance */
#define V4DPI_GeoCoordSpeed_Sec 1	/* per Second */
#define V4DPI_GeoCoordSpeed_Min 2	/* per Minute */
#define V4DPI_GeoCoordSpeed_Hour 3	/* per Hour */
#define V4DPI_GeoCoordSpeed_Day 4	/* per Day */
#define V4DPI_GeoCoordSpeed_milliSec 5	/* per millisecond */
#define V4DPI_GeoCoordSpeed_microSec 6	/* per microsecond */
#define V4DPI_GeoCoordSpeed_MaxVal 6	/* Maximum allowed value */

#define GEODISTFACTORS 1,1000,1609.3472186944375,0.30480060960121924,0.9144018288036576,1852
#define GEOTIMEFACTORS 0,1,60,60*60,24*60*60,0.001,0.000001

struct V4DPI__Value_GeoCoord
{ BITS GCType : 4 ;			/* GeoCoordinate Type (V4DPI_GCType_LatLonMeter) */
  SBITS TimeZone : 7 ;			/* Timezone (+/- 12) from GMT (see VCAL_TimeZone_xxx )*/
  BITS Coord3 : 21 ;			/* Third Coordinate (height in meters * V4DPI_GeoCoordCoord3_Scale) (see V4DPI_GeoCoordCoord3_Offset) */
  int Coord1,Coord2 ;			/* Coordinates (Lat & Long) */
} ;
struct V4DPI__Value_GCDSV
{ BITS GCType : 4 ;			/* GeoCoordinate Type (V4DPI_GCType_Distance) */
  BITS TimeZone : 7 ;			/* Timezone (+/- 12) from GMT (see VCAL_TimeZone_xxx )*/
  BITS Coord3 : 21 ;			/* Third Coordinate (height in meters * V4DPI_GeoCoordCoord3_Scale) (see V4DPI_GeoCoordCoord3_Offset) */
  int Coord1,Coord2 ;			/* Coordinates (Lat & Long) */
  BITS hgtUOM : 4 ;			/* UOM of height (for display only, Coord3 in meters!) (V4DPI_GeoCoordDist_xxx) */
  BITS distUOM : 4 ;			/* Type of distance (V4DPI_GeoCoordDist_xxx) */
  BITS speedUnit : 4 ;			/* If nonzero then speed factor (V4DPI_GeoCoordSpeed_xxx) */
  BITS distFrac : 20 ;			/* Distance fractional (scaled by V4DPI_GeoCoordDist_Factor) */
  CALENDAR calDateTime ;		/* Calendar date time (if none then = VCal_NullDate) */
  int distInt ;				/* Integer distance */
  int bearingDeg1 ;			/* Bearing (azimuth) in degrees * V4DPI_GeoCoord_Factor */
  int bearingDeg2 ;			/* Bearing (altitude/elevation) in degrees * V4DPI_GeoCoord_Factor */
 } ;

#define GETGEOLAT(GEO) (((double)((struct V4DPI__Value_GeoCoord *)(GEO))->Coord1) / (double)V4DPI_GeoCoord_Factor)
#define SETGEOLAT(GEO,LAT) ((struct V4DPI__Value_GeoCoord *)(GEO))->Coord1 = DtoI(LAT * V4DPI_GeoCoord_Factor)
#define GETGEOLON(GEO) (((double)((struct V4DPI__Value_GeoCoord *)(GEO))->Coord2) / (double)V4DPI_GeoCoord_Factor)
#define SETGEOLON(GEO,LON) ((struct V4DPI__Value_GeoCoord *)(GEO))->Coord2 = DtoI(LON * V4DPI_GeoCoord_Factor)
#define GETGEOALT(GEO) (((struct V4DPI__Value_GeoCoord *)(GEO))->Coord3 == V4DPI_GeoCoordCoord3_Null ? 0 : DtoI((double)((struct V4DPI__Value_GeoCoord *)(GEO))->Coord3 / V4DPI_GeoCoordCoord3_Scale) - V4DPI_GeoCoordCoord3_Offset)
#define SETGEOALT(GEO,ALT) ((struct V4DPI__Value_GeoCoord *)(GEO))->Coord3 = (ALT + V4DPI_GeoCoordCoord3_Offset) * V4DPI_GeoCoordCoord3_Scale

#define GETGEODSTSPD(GCDSV) (((struct V4DPI__Value_GCDSV *)(GCDSV))->distInt + ((struct V4DPI__Value_GCDSV *)(GCDSV))->distFrac / V4DPI_GeoCoordDist_Factor)
#define SETGEODSTSPD(GCDSV,DSTSPD) \
 { double frac,ipart ; frac = modf(DSTSPD,&ipart) ; ((struct V4DPI__Value_GCDSV *)(GCDSV))->distInt = DtoI(ipart) ; ((struct V4DPI__Value_GCDSV *)(GCDSV))->distFrac = DtoI(frac * V4DPI_GeoCoordDist_Factor) ; }
#define GETGEOAZ(GCDSV) ((double)((struct V4DPI__Value_GCDSV *)(GCDSV))->bearingDeg1 / (double)V4DPI_GeoCoord_Factor)
#define SETGEOAZ(GCDSV,AZ) ((struct V4DPI__Value_GCDSV *)(GCDSV))->bearingDeg1 = DtoI((AZ) * (double)V4DPI_GeoCoord_Factor)
#define GETGEOEL(GCDSV) ((double)((struct V4DPI__Value_GCDSV *)(GCDSV))->bearingDeg2 / (double)V4DPI_GeoCoord_Factor)
#define SETGEOEL(GCDSV,EL) ((struct V4DPI__Value_GCDSV *)(GCDSV))->bearingDeg2 = DtoI((EL) * (double)V4DPI_GeoCoord_Factor)

struct V4DPI__PntXDB			/* An external data point */
{ XDBID xdbId ;				/* Unique id for connection(-)/statement(+) */
//  XDBROW rowNum ;			/* Current row number we want */
  XDBRECID recId ;			/* Record number (not same as rowNum if multiple gets on a same dimension) */
} ;

struct V4DPI__Point
{ unsigned short Dim ;			/* Dimension of this point */
  unsigned short Bytes ;		/* Number of bytes in this point */
  BYTE PntType ;			/* Type of point - see V4DPI_PntType_xxx */
  BYTE Grouping ;			/* Grouping of points- see V4DPI_Grouping_xxx */
  BYTE LHSCnt ;				/* If intersection then number of points below before "|" (left-hand-size count) */
					/*  If point is FIXED then this contains implied decimals! */
					/*  If point is FChar then this contains the "embedded" point type */
  CHARFIELD NestedIsct : 1 ;		/* If this is intersection then TRUE if nested intersections within */
  CHARFIELD QuotedX : 1 ;		/* If TRUE then point is "quoted" and should not be evaluated */
  CHARFIELD Continued : 1 ;		/* Isct continues with another one immediately following */
  CHARFIELD TraceEval : 1 ;		/* If TRUE then trace every call to this isct */
  CHARFIELD CondEval : 1 ;		/* If TRUE then don't fail if binding has no value (but maybe fail if nested eval of value fails) */
  CHARFIELD ForceEval : 1 ;		/* If TRUE then force evaluation, sorta opposite of Quoted */
  CHARFIELD AltDim : 1 ;		/* If TRUE then alternate dimension used via tables/aggregates */
					/* ALSO used to indicate JSON return argument (so we don't enclose in quotes in nested JSON) */
					/* ALSO used to indicate auto-removal of point from context when binding matched */
#pragma pack(4)
  union
   { int IntVal ;
     int TreeNodeVal ;
     double _dblVal ;			/* For debugging - SHOULD NOT BE REFERENCED */
     unsigned char AlphaVal[V4DPI_AlphaVal_Max] ;
     UCCHAR UCVal[V4DPI_UCVal_Max] ;
     int Int2Val[2] ;
     BYTE MemPtr[SIZEofPTR] ;
     BYTE RealVal[SIZEofDOUBLE] ;
     BYTE FixVal[sizeof(B64INT)] ;
     struct V4DPI__Value_UOM UOMVal ;
     struct V4DPI__Value_UOMPer UOMPerVal ;
     struct V4DPI__Value_UOMPUOM UOMPUOMVal ;
     struct V4DPI__Value_GeoCoord GeoCoord ;
     struct V4DPI__Value_Complex Complex ;
     struct V4DPI__Value_Tele Tele ;
     struct V4DPI__PntV4IS V4IS ;
     struct V4DPI__Value_Isct Isct ;
     struct V4DPI__Value_BigIsct BigIsct ;
     struct V4DPI__PntXDB XDB ;
     struct V4DPI__Value_SymDef SymDef ;
   } Value ;
#pragma pack()
} ;
typedef struct V4DPI__Point P ; 	/* To make life easier */

#define NEWQUOTE
#ifdef NEWQUOTE
#define QUOTE(p) memmove(&(p)->Value,p,(p)->Bytes) ; memcpy(p,&protoQuote,protoQuote.Bytes) ; (p)->Bytes += ((P *)(&(p)->Value))->Bytes ;
#define UNQUOTE(p) memcpy(p,&(p)->Value,((P *)(&(p)->Value))->Bytes) ;
#define ISQUOTED(p)((p)->PntType == V4DPI_PntType_Shell ? (p)->Dim == Dim_UQuote : FALSE)
#define UNQUOTECOPY(d,s)  memcpy(d,&(s)->Value,((P *)(&(s)->Value))->Bytes) ;
#define UNQUOTEPTR(p) ((P *)&(p)->Value)
#else
#define QUOTE(p) (p)->Quoted = TRUE				/* Quote a point */
#define UNQUOTE(p) (p)->Quoted = FALSE				/* Un-quote a point */
#define ISQUOTED(p)((p)->Quoted)				/* TRUE if point is quoted */
#define UNQUOTECOPY(d,s) memcpy(d,s,(s)->Bytes)			/* Unquote point and copy (old way did not actually unquote) */
#define UNQUOTEPTR(p) (p)					/* Return pointer to unquoted portion of point */
#endif
#define ZPH(pp) memset(pp,0,V4DPI_PointHdr_Bytes)		/* Zap Point Header */
#define CPH(dst,src) memcpy(dst,src,V4DPI_PointHdr_Bytes)	/* Copy Point Header */
#define PTFROMHEAP (struct V4DPI__Point *)v4mm_AllocChunk(sizeof(struct V4DPI__Point),TRUE)
#define intPNT(p) memcpy(p,&protoInt,V4DPI_PointHdr_Bytes) ;
#define intPNTv(p,v) intPNT(p) ; (p)->Value.IntVal = v ; 
#define int2PNT(p) ZPH(p) ; (p)->Dim = Dim_Int2 ; (p)->PntType = V4DPI_PntType_Int2 ; (p)->Bytes = V4PS_Int2 ;
#define int2PNTv(p,v1,v2) int2PNT(p) ; (p)->Value.Int2Val[0] = v1 ;  (p)->Value.Int2Val[1] = v2 ; 
#define dblPNT(p) memcpy(p,&protoDbl,V4DPI_PointHdr_Bytes) ;
#define dblPNTv(p,v) dblPNT(p) ; memcpy(&(p)->Value.IntVal,&v,sizeof (double)) ; 
#define dblPNTi(p,i) dblPNT(p) ; { double _dd_ ; _dd_ = i ; memcpy(&(p)->Value.IntVal,&_dd_,sizeof (double)) ; } ;
#define dictPNT(p,dim) memcpy(p,&protoDict,V4DPI_PointHdr_Bytes) ; (p)->Dim = dim ;
#define dictPNTv(p,dim,v) dictPNT(p,dim) ; (p)->Value.IntVal = v ; 
#define alphaPNT(p) memcpy(p,&protoAlpha,V4DPI_PointHdr_Bytes) ;
#define alphaPNTv(p,v) alphaPNT(p) ; strcpy(&(p)->Value.AlphaVal[1],v) ; CHARPNTBYTES1((p)) ;
#define alphaPNTvl(p,v,l) alphaPNT(p) ; strcpy(&(p)->Value.AlphaVal[1],v) ; CHARPNTBYTES2((p),(l)) ;
#define uccharPNT(p) memcpy(p,&protoUCChar,V4DPI_PointHdr_Bytes) ;
#define uccharPNTv(p,v) uccharPNT(p) ; UCstrncpy(&(p)->Value.UCVal[1],v,V4DPI_UCVAL_MaxSafe) ; UCCHARPNTBYTES1(p) ;
#define uccharPNTvl(p,v,l) uccharPNT(p) ; UCstrncpy(&(p)->Value.UCVal[1],v,V4DPI_UCVAL_MaxSafe) ; UCCHARPNTBYTES2(p,l) ;
#define logPNT(p) memcpy(p,&protoLog,V4DPI_PointHdr_Bytes) ;
#define logPNTv(p,v) logPNT(p) ; (p)->Value.IntVal = v ;
#define fixPNT(p) memcpy(p,&protoFix,V4DPI_PointHdr_Bytes) ;
#define fixPNTv(p,v) memcpy(p,&protoFix,V4DPI_PointHdr_Bytes) ; COPYB64((p)->Value.FixVal,v) ;
#define treePNT(p) memcpy(p,&protoTree,V4DPI_PointHdr_Bytes) ;
#define treePNTv(p,v) treePNT(p) ; (p)->Value.IntVal = v ;
#define memPNTv(p,v) { void *__tPtr = v ; ZPH(p) ; (p)->Dim = Dim_UBLOB ; (p)->Bytes = V4PS_MemPtr ; (p)->PntType = V4DPI_PntType_MemPtr ; memcpy(&(p)->Value.MemPtr,&__tPtr,SIZEofPTR) ; }
#define V4DPI_SymDefBase_Bytes (V4DPI_PointHdr_Bytes + sizeof(SYMID) + sizeof(DICTID) + sizeof(ETYPE))
#define symdefPNTv(p,ID,SYMNAME,defpt,lsOpVal) \
 ZPH(p) ; (p)->PntType = V4DPI_PntType_SymDef ; (p)->Value.SymDef.symId = ID ; (p)->Value.SymDef.lsOp = lsOpVal ; \
 { struct V4DPI__DimInfo *_di ; DIMINFO(_di,ctx,Dim_NId) ; (p)->Value.SymDef.dictId = v4dpi_DictEntryGet(ctx,0,SYMNAME,_di,NULL) ; } \
 if (defpt == NULL)  { (p)->Bytes =V4DPI_SymDefBase_Bytes ; } \
  else { P *_DEFPT=defpt ; (p)->Bytes = (V4DPI_SymDefBase_Bytes + _DEFPT->Bytes) ; memcpy(&(p)->Value.SymDef.pntBuf,defpt,_DEFPT->Bytes) ;} ;
#define ZS(str) str[0] = UCEOS ;
#define ZUS(str) str[0] = UCEOS ;
#define intPC(num1,num2) (num2 == 0 ? 0 : DtoI((num1) * 100.0 / num2))
//#define GETREAL(dbl,pntp) memcpy(&dbl,&((pntp)->Value.RealVal),sizeof(double))
#define GETREAL(dbl,pntp) { int *_ld,*_lp ; _ld = (int *)&dbl ; _lp = (int *)&((pntp)->Value.RealVal) ; *(_ld++) = *(_lp++) ; *_ld = *_lp ; }
//#define PUTREAL(pntp,dbl) memcpy(&((pntp)->Value.RealVal),&dbl,sizeof(double))
#define PUTREAL(pntp,dbl) { int *_ld,*_lp ; _ld = (int *)&dbl ; _lp = (int *)&((pntp)->Value.RealVal) ; *(_lp++) = *(_ld++) ; *_lp = *_ld ; }
#define SETDBL(dstd,srcd) memcpy(&dstd,&srcd,sizeof(double))
/*	Converts fixed decimal to floating point (double) and rounds up to correct representation problem	*/
/*	  (this is to correct problems like 123.35 -> 123.34999999... which we round up to 123.35000000999....)	*/
/*	DO NOT CALL THIS MACRO WITH INVALID 'DECIMALS' VALUE (i.e. < 0 or > V_MAX_LITERAL_DECIMALS)		*/
#define FIXEDtoDOUBLE(DOUBLE,FIXED,DECIMALS) \
 { DOUBLE = (double)FIXED ; \
   if (DECIMALS > 0) { DOUBLE /= powers[DECIMALS] ; } ; \
 }
#define V4LIM_BiggestPositiveInt (0x7fffffff)
#define V4LIM_BiggestUnsignedInt ((unsigned int)(0xffffffff))
#define V4LIM_SmallestNegativeInt ((int)0x80000000)
#define V4LIM_BiggestPositiveLong ((B64INT)0x7fffffffffffffff)
#define V4LIM_SmallestNegativeLong ((B64INT)0x8000000000000000)
#define COPYB64(dstbi,srcbi) memcpy(&(dstbi),&(srcbi),sizeof (B64INT))
/* IGNOREARG - TRUE if argument is to be ignored */
#define IGNOREARG(pntp) (memcmp((pntp),&protoNone,V4PS_Int) == 0 || memcmp((pntp),&CondEvalRet,CondEvalRet.Bytes) == 0)
/* IGNOREARGOREMPTY - TRUE if argument is 0-len string or should be ignored */
#define IGNOREARGOREMPTY(pntp) ((pntp)->PntType == V4DPI_PntType_Char ? (pntp)->Value.AlphaVal[0] == '\0' : ((pntp)->PntType == V4DPI_PntType_UCChar ? (pntp)->Value.UCVal[0] == 0 : IGNOREARG(pntp)))

#define V4PS_Int V4DPI_PointHdr_Bytes+sizeof(int)
#define V4PS_Dict V4DPI_PointHdr_Bytes+sizeof(int)
#define V4PS_XDict V4DPI_PointHdr_Bytes+sizeof(int)
//#define V4PS_LInt V4DPI_PointHdr_Bytes+sizeof(B64INT)
#define V4PS_Int2 V4DPI_PointHdr_Bytes+sizeof(int)+sizeof(int)
#define V4PS_Real V4DPI_PointHdr_Bytes+sizeof(double)
#define V4PS_UTime V4DPI_PointHdr_Bytes+sizeof(double)
#define V4PS_Calendar V4DPI_PointHdr_Bytes+sizeof(double)
#define V4PS_Fixed V4DPI_PointHdr_Bytes+sizeof(B64INT)
#define V4PS_UOM V4DPI_PointHdr_Bytes+sizeof(struct V4DPI__Value_UOM)
#define V4PS_UOMPer V4DPI_PointHdr_Bytes+sizeof(struct V4DPI__Value_UOMPer)
#define V4PS_UOMPUOM V4DPI_PointHdr_Bytes+sizeof(struct V4DPI__Value_UOMPUOM)
#define V4PS_GeoCoord V4DPI_PointHdr_Bytes+sizeof(struct V4DPI__Value_GeoCoord)
#define V4PS_GeoCoordGCDSV V4DPI_PointHdr_Bytes+sizeof(struct V4DPI__Value_GCDSV)
#define V4PS_Complex V4DPI_PointHdr_Bytes+sizeof(struct V4DPI__Value_Complex)
#define V4PS_Tree V4DPI_PointHdr_Bytes+sizeof(int)
#define V4PS_Tele V4DPI_PointHdr_Bytes+sizeof(struct V4DPI__Value_Tele)
#define V4PS_V4IS V4DPI_PointHdr_Bytes+sizeof(struct V4DPI__PntV4IS)
#define V4PS_MemPtr V4DPI_PointHdr_Bytes+SIZEofPTR
#define V4PS_BigIsct V4DPI_PointHdr_Bytes+sizeof(struct V4DPI__Value_BigIsct)
#define V4PS_SpecNone V4DPI_PointHdr_Bytes			/* Size of dim:none point (dim = int/real) */
#define V4PS_XDB V4DPI_PointHdr_Bytes+sizeof(struct V4DPI__PntXDB)
#define V4PS_UCChar(LEN) (V4DPI_PointHdr_Bytes+((LEN+1)*sizeof(UCCHAR)))

#define PNTintVAL(OK,PNT,CTX) ((PNT)->PntType == V4DPI_PntType_Int ? (*(OK) = TRUE, (PNT)->Value.IntVal) : v4im_GetPointInt(OK,PNT,CTX))
#define PNTlogVAL(OK,PNT,CTX) ((PNT)->PntType == V4DPI_PntType_Int || (PNT)->PntType == V4DPI_PntType_Logical ? (*(OK) = TRUE, (PNT)->Value.IntVal > 0) : v4im_GetPointLog(OK,PNT,CTX))

/*	NOTES ON V4 Alpha/Character Points
*	All string data begins with Value.AlphaVal[1]
*	The length of short string (<255) is store in Value.AlphaVal[0], the string is not null terminated
*	Longer strings have 255 in Value.AlphaVal[0] and must be null terminated
*	All V4 points must be 32-bit aligned/padded. Strings must be consistently padded with nulls or various routines will not work properly
*/

#define V4PT_MaxCharLenIn0 254		/* Maximum bytes in AlphaVal[0], anything larger stored as 255 as ASCIZ */
#define V4PT_CharBigStrPrefix 0xff	/* Value of Value.AlphaVal[0] to indicate long ASCIZ string */
/*	Macros to set proper PntType_Char lengths & pad end-of-words with nulls */
#define CHARPNTBYTES1(pnt) \
	{ LENMAX len = (LENMAX)strlen(&(pnt)->Value.AlphaVal[1]) ;\
	  memset(&(pnt)->Value.AlphaVal[len+1],0,3) ;\
	  if (len >= V4PT_CharBigStrPrefix) { (pnt)->Value.AlphaVal[0] = V4PT_CharBigStrPrefix ; (pnt)->Bytes = ALIGN(V4DPI_PointHdr_Bytes + len + 2) ; }\
	   else { (pnt)->Value.AlphaVal[0] = len ; (pnt)->Bytes = (unsigned short)ALIGN(V4DPI_PointHdr_Bytes + len + 1) ; } ; }
#define CHARPNTBYTES2(pnt,len) \
	{ memset(&(pnt)->Value.AlphaVal[len+1],0,3) ;\
	  if (len >= V4PT_CharBigStrPrefix) { (pnt)->Value.AlphaVal[0] = V4PT_CharBigStrPrefix ; (pnt)->Bytes = ALIGN(V4DPI_PointHdr_Bytes + len + 2) ; }\
	   else { (pnt)->Value.AlphaVal[0] = len ; (pnt)->Bytes = ALIGN(V4DPI_PointHdr_Bytes + len + 1) ; } ; }
/*	Macro to get length of a Char string */
#define CHARSTRLEN(pnt)\
	((pnt)->Value.AlphaVal[0] < V4PT_CharBigStrPrefix ? (pnt)->Value.AlphaVal[0] : strlen(&(pnt)->Value.AlphaVal[1]))
#define CHARISEMPTY(pnt) ((pnt)->Value.AlphaVal[0] == 0)

#ifdef V4UNICODE
#define UCCHARPNTBYTES1(pnt) \
	{ LENMAX len = (LENMAX)UCstrlen((&(pnt)->Value.UCVal[1])) ; \
	  (pnt)->Value.UCVal[len+1] = UCEOS ;\
	  (pnt)->Value.UCVal[0] = len ;\
	  (pnt)->Bytes = (unsigned short)ALIGN(V4DPI_PointHdr_Bytes + sizeof(UCCHAR)*(len + 2)) ; }

#define UCCHARPNTBYTES2(pnt,len) \
	(pnt)->Value.UCVal[len+1] = UCEOS ;\
	(pnt)->Value.UCVal[0] = len ;\
	(pnt)->Bytes = ALIGN(V4DPI_PointHdr_Bytes + sizeof(UCCHAR)*(len + 2))

#define UCCHARSTRLEN(pnt) ((pnt)->Value.UCVal[0])
#define UCCHARISEMPTY(pnt) ((pnt)->Value.UCVal[0] == 0)
#define UCPNTSETLEN(pnt,len) (pnt)->Value.UCVal[0] = len ;
#else
#define UCCHARPNTBYTES1(pnt) CHARPNTBYTES1(pnt)
#define UCCHARPNTBYTES2(pnt,len) CHARPNTBYTES2(pnt,len)
#define UCCHARSTRLEN(pnt) CHARSTRLEN(pnt)
#define UCPNTSETLEN(pnt,len) (pnt)->Value.UCVal[0] = len ;
#define UCCHARISEMPTY(pnt) ((pnt)->Value.UCVal[0] == 0)
#endif

/*	Macro to calculate 32-bit hash of a string, LEN = number of characters in string (not bytes) */
#define VHASH32_FWD(RESULT,STR,LEN) \
 { INDEX _i ; LENMAX _len ; extern int vhash32_table[] ; \
    _len = (LEN)*sizeof(UCCHAR) ; RESULT = _len | ((char *)STR)[0] ; \
    for(_i=0;_i<_len;_i++) { RESULT = (RESULT < 0 ? ((RESULT << 1) | 1) : (RESULT << 1)) ; RESULT ^= vhash32_table[((char *)STR)[_i]] ; } ; \
 }
/*	Same as above ** BUT ** LEN = number of bytes in string */
#define VHASH32_FWDb(RESULT,STR,LEN) \
 { INDEX _i ; LENMAX _len ; extern int vhash32_table[] ; \
    _len = (LEN) ; RESULT = _len | ((char *)STR)[0] ; \
    for(_i=0;_i<_len;_i++) { RESULT = (RESULT < 0 ? ((RESULT << 1) | 1) : (RESULT << 1)) ; RESULT ^= vhash32_table[((char *)STR)[_i]] ; } ; \
 }
/*	Calculates 32-bit hash in REVERSE order ** AND ** LEN = number of bytes in string */
#define VHASH32_REVb(RESULT,STR,LEN) \
 { INDEX _i ; LENMAX _len ; extern int vhash32_table[] ; \
    _len = (LEN) ; RESULT = _len | ((char *)STR)[0] ; \
    for(_i=_len-1;_i>=0;_i--) { RESULT = (RESULT < 0 ? ((RESULT << 1) | 1) : (RESULT << 1)) ; RESULT ^= vhash32_table[((char *)STR)[_i]] ; } ; \
 }


/*	Macro to copy all point bit-flags from one point to another */
#define COPYPNTFLAGS(dst,src) \
  dst->NestedIsct = src->NestedIsct ; if (ISQUOTED(src)) { QUOTE(dst) ; } else { UNQUOTE(dst) ; } ; dst->Continued = src ->Continued ; dst->TraceEval = src ->TraceEval ;\
  dst->CondEval = src ->CondEval ; dst->ForceEval = src ->ForceEval ; dst->AltDim = src ->AltDim ;

#define V4DPI_LittleAlpha_Max 12
#define V4DPI_CacheIsct_PointMax 100

struct V4DPI__LittlePoint		/* Format of little point (holds numeric value, little else) */
{ unsigned short Dim ;			/* ** NOTE ** this junk MUST match above */
  unsigned short Bytes ;
  BYTE PntType ;
  BYTE Grouping ;
  BYTE LHSCnt ;
  CHARFIELD NestedIsct : 1 ;
  CHARFIELD QuotedX : 1 ;
  CHARFIELD Continued : 1 ;
  CHARFIELD TraceEval : 1 ;		/* If TRUE then trace every call to this isct */
  CHARFIELD CondEval : 1 ;		/* If TRUE then don't fail if binding has no value (but maybe fail if nested eval of value fails) */
  CHARFIELD ForceEval : 1 ;		/* If TRUE then force evaluation, sorta opposite of Quoted */
  CHARFIELD AltDim : 1 ;		/* If TRUE then alternate dimension used via tables/aggregates */
  union
   { int IntVal ;
     int Int2Val[2] ;
     BYTE MemPtr[SIZEofPTR] ;
     BYTE RealVal[SIZEofDOUBLE] ;
     BYTE FixVal[sizeof(B64INT)] ;
     unsigned char AlphaVal[V4DPI_LittleAlpha_Max] ;
     UCCHAR UCVal[V4DPI_LittleAlpha_Max/sizeof(UCCHAR)] ;
     struct V4DPI__PntXDB XDB ;
  } Value ;
} ;

#define V4DPI_BigIsctBufMax V4Area_UsableBytes - V4DPI_PointHdr_Bytes
struct V4DPI__BigIsct			/* Structure for big intersection */
{ union V4IS__KeyPrefix kp ;		/* Type=V4, SubType=BigIsct, Mode=Int, Length=8 */
  DIMID DimUID ;			/* Unique code (DimUnique for Dim:Isct) */
  LENMAX Bytes ;			/* Number of bytes in this record */
  struct V4DPI__Point biPnt ;
  BYTE ExtraBuf[V4DPI_BigIsctBufMax + V4DPI_PointHdr_Bytes] ;
} ;

#define V4DPI_ShortAlphaVal_Max 8
#define V4DPI_ShortPointHdr_Bytes 4
#define V4DPI_ShortPoint_Limit V4DPI_ShortAlphaVal_Max+V4DPI_PointHdr_Bytes

struct V4DPI__ShortPoint		/* Used for "little points" stored directly within bindlist */
{ unsigned short Dim ;			/* Dimension of this point */
  unsigned short Bytes ;		/* Number of bytes in this point */
  BYTE PntType ;			/* Type of point - see V4DPI_PntType_xxx */
  BYTE LHSCnt ;				/* LHSCnt or decimals (see V4DPI__Point) */
  union
   { int IntVal ;
     char AlphaVal[V4DPI_ShortAlphaVal_Max] ;
   } Value ;
} ;

#define V4DPI_TagFlag_Colon3 0x10000	/* Flag indicating ':::' */
#define V4DPI_TagFlag_MaskOut 0xffff	/* Mask to remove any flags */

struct V4DPI__TagVal			/* Structure of a tagged value */
{ TAGVAL TagVal ;			/* Tag Flags,,Tag internal value */
  struct V4DPI__Point TagPt ;		/* Tagged point */
} ;


/*	Fast macro to obtain tag value from tag point
	NOTE: EVALPT must be defined (i.e. not NULL)
*/
#define TAGVALUE(CTX,POINT,RESPNT,EVALPT) \
  if ((POINT)->PntType == V4DPI_PntType_TagVal) \
   { struct V4DPI__TagVal *tv ; struct V4DPI__Point *_rpt ; \
     tv = (struct V4DPI__TagVal *)&(POINT)->Value ; \
     if ((POINT)->ForceEval)  { v4im_LastTagNum = -(tv->TagVal & V4DPI_TagFlag_MaskOut) ; } \
       else { v4im_LastTagNum = (tv->TagVal & V4DPI_TagFlag_MaskOut) ; \
		_rpt = &tv->TagPt ; \
		switch(tv->TagPt.PntType) \
		  {  \
		    case V4DPI_PntType_Special: \
		    case V4DPI_PntType_Isct: \
		    case V4DPI_PntType_SymDef: \
			switch(tv->TagVal & V4DPI_TagFlag_MaskOut) \
			 { default: \
				if (ISQUOTED(&tv->TagPt)) break ; \
				_rpt = v4dpi_IsctEval(EVALPT,&tv->TagPt,CTX,0,NULL,NULL) ; \
				if (_rpt == NULL) { v_Msg(CTX,CTX->ErrorMsgAux,"TagNoEval",POINT) ; v4im_LastTagNum = V4IM_Tag_Unk ; } ; \
				break ; \
			   case V4IM_Tag_If: case V4IM_Tag_While: case V4IM_Tag_Parent: case V4IM_Tag_PIf: \
			   case V4IM_Tag_Maximum: case V4IM_Tag_Minimum: case V4IM_Tag_Group: break ; \
			 } ; break ; \
		    } ; \
		if (RESPNT != NULL) *RESPNT = _rpt ; \
	     } ; \
   } else  { v4im_CheckPtArgNew(CTX,POINT,RESPNT,EVALPT) ; } ;



#ifdef NEWQUOTE
#define GLOBALDIMSEXTERN \
 extern DIMID Dim_Dim,Dim_Logical,Dim_List,Dim_Int,Dim_Int2,Dim_Alpha,Dim_Num,Dim_UCal,Dim_UDate,Dim_NId,Dim_UV4,Dim_UDT,Dim_UDelta,Dim_IntMod,Dim_Tag,Dim_UMacro,Dim_UTable,Dim_UFix,Dim_UTree ; \
 extern DIMID Dim_UMonth,Dim_UTime,Dim_UYear,Dim_UWeek,Dim_UQuarter,Dim_UPeriod,Dim_Isct,Dim_UDim,Dim_UBLOB,Dim_UCountry,Dim_UCChar,Dim_UDB,Dim_UQuote,Dim_UArray,Dim_UTele,Dim_UDBArgs,Dim_UDBSpec ;\
 extern struct V4DPI__LittlePoint protoInt, protoDbl, protoDict, protoAlpha, protoLog, protoForeign, protoNone, protoQNone, protoUCChar, protoFix, protoQuote, protoEQ, protoSkip, protoEvalStream, protoTree ;\
 extern struct V4DPI__LittlePoint Log_True,Log_False ;
#else
#define GLOBALDIMSEXTERN \
 extern DIMID Dim_Dim,Dim_Logical,Dim_List,Dim_Int,Dim_Int2,Dim_Alpha,Dim_Num,Dim_UCal,Dim_UDate,Dim_NId,Dim_UV4,Dim_UDT,Dim_UDelta,Dim_IntMod,Dim_Tag,Dim_UMacro,Dim_UTable,Dim_UFix,Dim_UTree ; \
 extern DIMID Dim_UMonth,Dim_UTime,Dim_UYear,Dim_UWeek,Dim_UQuarter,Dim_UPeriod,Dim_Isct,Dim_UDim,Dim_UBLOB,Dim_UCountry,Dim_UCChar,Dim_UDB ;\
 extern struct V4DPI__LittlePoint protoInt, protoDbl, protoDict, protoAlpha, protoLog, protoForeign, protoNone, protoQNone, protoUCChar, protoFix, protoSkip, protoEQ, protoEvalStream, protoTree ;\
 extern struct V4DPI__LittlePoint Log_True,Log_False ;
#endif

#define CHECKV4INIT (Dim_Dim == 0 ? v4im_InitConstVals(ctx) : FALSE)

/*	T H R E A D   C R E A T I O N				*/

#ifdef WINNT
#define CREATE_THREAD(PROC,THREADID,THREADARG,RESPNT,ERRBUF) \
 { DWORD dwThreadId ; \
   THREADID = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)PROC,THREADARG,0,&dwThreadId) ; \
   if (THREADID == NULL) { v_Msg(NULL,ERRBUF,"SpawnErrSys","CreateThread",OSERROR) ; goto fail ; } ; \
   intPNTvMod(RESPNT,dwThreadId) ; \
 }
#define CLOSE_THREAD(THREADID) { TerminateThread(THREADID,0) ; CloseHandle(THREADID) ; }
#elif defined UNIX
#define CREATE_THREAD(PROC,THREADID,THREADARG,RESPNT,ERRBUF) \
   if (pthread_create(&THREADID,NULL,PROC,THREADARG) != 0) \
    { v_Msg(NULL,ERRBUF,"SpawnErrSys","pthread_create",OSERROR) ; goto fail ; } ; \
   intPNTvMod(RESPNT,THREADID) ;
#define CLOSE_THREAD(THREADID) {}
#endif



/*	M U L T I D I M E N S I O N A L   A R R A Y		*/

#define V4IM_MDArrayIndexesMax 1000	/* Max number of explicit dimension indexes allowed */

struct V4IM__MDArray
{ LENMAX Elements ;			/* Total number of elements in array */
  LENMAX Bytes ;			/* Total bytes in array */
  DIMID Dim ;				/* Dimension of elements in array */
  ETYPE PntType ; 			/* Point Type (e.g. _Int, _Real, _UDate, ... ) */
					/* If point not numeric, then element are pointers to V4 points */
  LENMAX ElBytes ; 			/* Bytes in array element (e.g. 4 for _Int) */
  LENMAX MDDimCount ;			/* Number of dimensions */
  P *DfltVal ;				/* If elements pointers & pointer NULL then use this value */
  BYTE *Data ;				/* Pointer to actual array */
  struct {
    LENMAX MaxNum ;			/* Maximum number in this dimension */
    int BaseVal ;			/* Base value */
    int Offset ;			/* Offset for each row(column) of this dimension */
    DIMID dimId ;			/* Id of this dimension */
    PNTTYPE PntType ;			/* PointType of this dimension */
    struct V4IM__MDArrayIndexes *vmdi ; /* Pointer to list of indexes */
					/* If not NULL then BaseVal is meaningless */
   } MDDim[V4DPI_MDArray_Max] ;
} ;

struct V4IM__MDArrayIndexes
{ DIMID dimId ;				/* Dimension id for this array dimension */
  COUNTER Count ;			/* Number of entries below */
  int Indexes[V4IM_MDArrayIndexesMax] ;
} ;
#define MDIndexBytes(NUM) ((2 * sizeof(int)) + (NUM * sizeof(int))) /* Macro to determine size of above structure */


/*	Drawer Structure (NOTE: not using kp/key info just yet) */
#define V4IM_DrawerBufInit 0x8000	/* Initial size of Drawer buffer space */
#define V4IM_DrawerBufInc 0x8000	/* Default increment size */
struct V4IM__Drawer {
  union V4IS__KeyPrefix kp ;		/* Type=V4, SubType=Drawer, Mode=Int2, Length=16 */
  DIMID Dim ;				/* Dimension associated with this entry */
  int Id ;				/* Unique DrawerId */
  LENMAX Bytes ;			/* Number of used bytes in this record (including this header) */
  LENMAX maxBytes ;			/* Number bytes allocated for this record */
  COUNTER Points ;			/* Number of points stored below */
  BYTE DrawerBuf[0] ;			/* Buffer for points (stored contiguously) */
} ;

/*	C A L E N D A R   I N F O			*/

#define VCAL_YMDOrderMax 3		/* Max number of y-m-d orderings */
typedef unsigned short YMDORDER ;
#define VCal_MJDOffset (678576-1)	/* Offset from calendar base to Modified Julian Date (Smithsonian/UDate) */
#define VCal_NullDate -9999999 		/* Unique number indicating "no date" */
#define VCAL_MidnightValue 0.0000001	/* If a time then want fractional component to be non-zero */
#define VCAL_BadVal -9999		/* Invalid date, month, whatever */
#define VCAL_UDateMax 100000		/* Max allowed internal UDate */
#define VCAL_UDate_None 0		/* Value for 'no date' */
#define VCAL_UDT_None 0
#define VCAL_UYear_None 0
#define VCAL_UMonth_None 0
#define VCAL_UPeriod_None 0
#define VCAL_UWeek_None 0
#define VCAL_UQuarter_None 0
#define VCAL_UTime_None 0.0
#define VCAL_UDTMax 69076		/* Max allowed UDate value for UDT points */
#define VCAL_BaseYear 1850		/* Base year for many Uxxx calculations */
#define VCAL_UYearMin 1858
#define VCAL_UYearMax 2200		/* Range of allowed UYears */
#define VCAL_UDTUDateOffset 44240	/* Offset number of days between UDate & UDT */
#define VCAL_LOCALNOW -987654321	/* Local current time (only supported in a few routines) */
#define VCAL_SecsInDay (24*60*60)	/* Number of seconds in a day */
#define VCALADJYEAR(isHistory,year) \
 if (year < 100) \
  { if (isHistory && (year + 2000) > gpi->curYear) { year += 1900 ; } \
     else { year += 2000 ; } ; \
  } ; 

#define VCAL_CalType_UseDeflt -1	/* Use default from current Locale */
#define VCAL_CalType_Gregorian 0
#define VCAL_CalType_Julian 1
#define VCAL_CalType_Islamic 2
#define VCAL_CalType_ISO 3
#define VCAL_CalType_Hebrew 4
#define VCAL_CalType_Chinese 5
#define VCAL_CalType_Hindu 6

#define VCAL_TimeZone_Local 60		/* Local Time Zone (default) */
#define VCAL_TimeZone_GMT 0		/* Zulu */
#define VCAL_TimeZone_EST -5		/* Eastern Standard */

#define VCAL_Flags_Historical 1		/* Date must be in the past */

#define VRELD_Current	1		/* Want 'current' date/date-time/whatever */
#define VRELD_CurrentPlus 2		/*  started as 'current' but altered via v_CalCalc() */
#define VRELD_Today 3			/*  want today */
#define VRELD_Tomorrow 4		/*  tomorow */
#define VRELD_Yesterday 5		/*  yesterday */

struct V4Cal__Args {
  double rd ;			/* Rata die - standard date */
  int Year,Month,Week,Day ;
} ;

#define UDTtoUD(UDT) (((UDT) / VCAL_SecsInDay) + VCAL_UDTUDateOffset)
#define UDTtoSeconds(UDT) ((UDT) % VCAL_SecsInDay)
#define UDtoUDT(UD) (((UD) - VCAL_UDTUDateOffset) * VCAL_SecsInDay)
#define UDtoUDTcurrentTime(UD) ((UDtoUDT(UD)) + ((valUDTisNOW) % VCAL_SecsInDay))
#define UDTtoCAL(UDT) ((UDT) == 0 ? VCal_NullDate : ((CALENDAR)(((UDT) / VCAL_SecsInDay) + VCAL_UDTUDateOffset + VCal_MJDOffset) + (double)((UDT) % VCAL_SecsInDay) / ((double)VCAL_SecsInDay) + gpi->MinutesWest / (24.0 * 60.0)))
#define UDtoCAL(UD) ((UD) == 0 ? VCal_NullDate : (UD) + VCal_MJDOffset)
#define UDtoUMONTH(UMONTH,UD) { int _ymd = mscu_udate_to_yyyymmdd(UD) ; UMONTH = (_ymd/10000 - VCAL_BaseYear)*12 + ((_ymd/100) % 100)-1 ; }
#define YYMMtoUMONTH(YY,MM) ((YY - VCAL_BaseYear) * 12 + MM - 1)

#define YYQQtoUQTR(YY,QQ) (((YY) - VCAL_BaseYear) * 4 + ((QQ) - 1))
#define YYMMtoUQTR(YY,MM) (((YY) - VCAL_BaseYear) * 4 + ((MM) - 1)/3)
#define YYYYMMDDtoUQTR(YYYYMMDD) ((((YYYYMMDD)/10000) - VCAL_BaseYear)*4 + (((((YYYYMMDD)/100)%100) + 2) / 3) - 1)
#define UQTRtoUYEAR(UQTR) (((UQTR) / 4) + VCAL_BaseYear)
#define UQTRtoQTR(UQTR) (((UQTR) % 4) + 1)

#define UMONTHtoUQTR(UMON) YYMMtoUQTR(UMONTHtoUYEAR(UMON),(UMONTHtoMONTH(UMON) - 1 / 3) + 1)
#define UMONTHtoUYEAR(UMON) (((UMON) / 12) + VCAL_BaseYear)
#define UMONTHtoMONTH(UMON) (((UMON) % 12) + 1)

#define YYWWtoUWEEK(YY,WW) (((YY) - VCAL_BaseYear) * 52 + (WW) - 1)
#define UWEEKtoUYEAR(UW) ((UW / 52) + VCAL_BaseYear)
#define UWEEKtoWEEK(UW) (((UW) % 52) + 1)

#define YYPPtoUPERIOD(YY,PP,PpY) (((YY) - VCAL_BaseYear) * (PpY) + (PP) - 1)
#define UPERIODtoUYEAR(UP,PpY)  (((UP) / (PpY)) + VCAL_BaseYear)
#define UPERIODtoPERIOD(UP,PpY) (((UP) % (PpY)) + 1)

#define valUDATEisNOW (TIMEOFFSETDAY + (time(NULL)-(60*gpi->MinutesWest))/VCAL_SecsInDay)	/* UDate for 'today' */
#define valUDTisNOW (time(NULL) - (60*gpi->MinutesWest) - TIMEOFFSETSEC)			/* UDT for 'now' */
#define setUTIMEisNOW(_utime) _utime = (double)(((int)(time(NULL)-(60*gpi->MinutesWest))) % VCAL_SecsInDay) ;
#define valUYEARisNOW (mscu_udate_to_yyyymmdd(valUDATEisNOW) / 10000)
#define POSIXtoUDT(_posixDT) (_posixDT - (60*gpi->MinutesWest) - TIMEOFFSETSEC)			/* Convert POSIX/OS date-time to UDT */

#ifdef WINNT
#define setCALisNOW(_cal) \
{ SYSTEMTIME _sts ; \
  GetSystemTime(&_sts) ; \
  _cal = (CALENDAR)FixedFromGregorian(_sts.wYear,_sts.wMonth,_sts.wDay) + (double)(_sts.wHour*3600 + _sts.wMinute*60 + _sts.wSecond + ((double)_sts.wMilliseconds / 1000.0)) / (double)VCAL_SecsInDay ; \
  if (_sts.wSecond + _sts.wMilliseconds == 0) _cal += VCAL_MidnightValue ; \
}
#else
#define setCALisNOW(_cal) \
{ double _fraction, _ipart ; _cal = VCal_MJDOffset + TIMEOFFSETDAY +(double)(time(NULL))/(double)VCAL_SecsInDay ; \
  _fraction = modf(_cal,&_ipart) ; if (_fraction == 0.0) _cal += VCAL_MidnightValue ; \
}
#endif


#define WH_EMPTY 0x80000000		/* Int value to indicate empty Src slot */
#define V4DPI_WHType_IntInt 1		/* Simplest wormhole type, dims are fixed, values are integer */
#define V4DPI_WHType_IntDbl 2		/* Next simplest wormhole type, dims are fixed, values are double */
#define V4DPI_WHType_IntAgg 3		/* Next simplest wormhole type, dims are fixed, values are agg refs */
#define V4DPI_WHType_IntUOMPer 4	/* Integer -> UOMPer */
#define V4DPI_WHType_LIntLInt 10	/* LongInt -> LongInt */
#define V4DPI_WHType_IntGeo 5		/* Int -> GeoCoordinate */

#define V4DPI_WHIntInt_Max 10007	/* Default number of entries for IntInt WH's */

struct V4DPI__WormHoleIntInt		/* Structure defining wormholes (IntInt type) */
{ ETYPE WHType ;			/* Type of wormhole- V4DPI_WHType_xxx */
  int Han ;				/* Handle associated with this structure */
  COUNTER Count ;			/* Number of entries below */
  COUNTER Maximum ; 			/* Max number of entries for below */
  DIMID SrcDim,DestDim ;		/* Dimensions of src and destination points */
  P Fail ;				/* Point to be executed if no mapping from src to dest */
  struct V4DPI__LittlePoint rPt ;	/* Mask for result point (for speedy execution) */
  struct {
    int Src ;				/* Source value */
    int Dest ;				/* Destination value */
   } Entry[V4DPI_WHIntInt_Max] ;
} ;

struct V4DPI__WormHoleIntDbl		/* Structure defining wormholes (Int->Double type) */
{ ETYPE WHType ;			/* Type of wormhole- V4DPI_WHType_xxx */
  int Han ;				/* Handle associated with this structure */
  COUNTER Count ;			/* Number of entries below */
  COUNTER Maximum ; 			/* Max number of entries for below */
  DIMID SrcDim,DestDim ;			/* Dimensions of src and destination points */
  P Fail ;				/* Point to be executed if no mapping from src to dest */
  struct V4DPI__LittlePoint rPt ;	/* Mask for result point (for speedy execution) */
  struct {
    int Src ;				/* Source value */
    double Dest ;			/* Destination value */
   } Entry[V4DPI_WHIntInt_Max] ;
} ;

struct V4DPI__WormHoleIntAgg		/* Structure defining wormholes (Int->Agg type) */
{ ETYPE WHType ;			/* Type of wormhole- V4DPI_WHType_xxx */
  int Han ;				/* Handle associated with this structure */
  COUNTER Count ;			/* Number of entries below */
  COUNTER Maximum ; 			/* Max number of entries for below */
  DIMID SrcDim,DestDim ;		/* Dimensions of src and destination points */
  P Fail ;				/* Point to be executed if no mapping from src to dest */
  struct V4DPI__LittlePoint rPt ;	/* Mask for result point (for speedy execution) */
  struct {
    int Src ;				/* Source value */
    int AggIndex,AggKeyValue ;		/* Destination value */
   } Entry[V4DPI_WHIntInt_Max] ;
} ;

struct V4DPI__WormHoleIntUOMPer 	/* Structure defining wormholes (Int->UOMPer type) */
{ ETYPE WHType ;			/* Type of wormhole- V4DPI_WHType_xxx */
  int Han ;				/* Handle associated with this structure */
  COUNTER Count ;			/* Number of entries below */
  COUNTER Maximum ; 			/* Max number of entries for below */
  DIMID SrcDim,DestDim ;		/* Dimensions of src and destination points */
  P Fail ;				/* Point to be executed if no mapping from src to dest */
  struct {
    int Src ;				/* Source value */
    struct V4DPI__Value_UOMPer UOMPer ; /* Destination value */
   } Entry[V4DPI_WHIntInt_Max] ;
} ;

struct V4DPI__WormHoleIntGeo		/* Structure defining wormholes (Int->Geocoordinate type) */
{ ETYPE WHType ;			/* Type of wormhole- V4DPI_WHType_xxx */
  int Han ;				/* Handle associated with this structure */
  COUNTER Count ;			/* Number of entries below */
  COUNTER Maximum ; 			/* Max number of entries for below */
  DIMID SrcDim,DestDim ;			/* Dimensions of src and destination points */
  P Fail ;				/* Point to be executed if no mapping from src to dest */
  struct {
    int Src ;				/* Source value */
    struct V4DPI__Value_GeoCoord Geo ;	/* Destination value */
   } Entry[V4DPI_WHIntInt_Max] ;
} ;

struct V4DPI__WormHoleLIntLInt		/* Structure defining wormholes (long Int to long Int type) */
{ ETYPE WHType ;			/* Type of wormhole- V4DPI_WHType_xxx */
  int Han ;				/* Handle associated with this structure */
  COUNTER Count ;			/* Number of entries below */
  COUNTER Maximum ; 			/* Max number of entries for below */
  DIMID SrcDim,DestDim ;		/* Dimensions of src and destination points */
  P Fail ;				/* Point to be executed if no mapping from src to dest */
  struct V4DPI__LittlePoint rPt ;	/* Mask for result point (for speedy execution) */
  struct {
    B64INT Src ;			/* Source value */
    B64INT Dest ;			/* Destination value */
   } Entry[V4DPI_WHIntInt_Max] ;
} ;


#define V4DPI_WorkRelHNum 7			/* RelH for process-specific work area */
#define V4DPI_dfltRelHNum 5			/* Default RelH on area creation */
#define V4DPI_DimInfo_DimNameMax 31		/* Max bytes in dimension name */

#define V4DPI_DimInfo_UOkNew 1			/* Points on dimension to be created via {new} */
#define V4DPI_DimInfo_UOkPoint 2		/* Points to be created via V4EVAL "Point" command */
#define V4DPI_DimInfo_UOkAgg 3			/* Points to be created via Aggregate header */
#define V4DPI_DimInfo_UOkInt 4			/* Points to be created internal V4 routines (similar to UOkNew) */

#define V4DPI_DimInfo_RangeOK 0x1		/* TRUE if point ranges (xxx..yyy) are allowed */
#define V4DPI_DimInfo_ListOK 0x2		/* TRUE if point lists (xxx,yyy,zzz) are allowed */
#define V4DPI_DimInfo_unused4 0x4		/* unused */
#define V4DPI_DimInfo_IsSet 0x8 		/* TRUE if list is a set (point may only occur once) */
#define V4DPI_DimInfo_NoAutoCreate 0x10		 /* If TRUE then don't auto create points (only via POINT command in V4EVAL) */
#define V4DPI_DimInfo_DisplayerTrace 0x20	/* This dimension has [UV4:DisplayerTrace] binding to mask values when dumping context & outputting traces */
#define V4DPI_DimInfo_NoCtxExp 0x40		/* If TRUE then don't expand points in context (for PntType List) */
#define V4DPI_DimInfo_AutoIsct 0x80		/* If TRUE then convert point to isct & evaluate (where appropriate!) */
#define V4DPI_DimInfo_Dim 0x100 		/* This is the Dim dimension */
#define V4DPI_DimInfo_BindEval 0x200		/* If TRUE then perform isct eval instead of binding */
#define V4DPI_DimInfo_HasIsA 0x400		/* May be "isa" dimensions off of this one - have to check */
#define V4DPI_DimInfo_Acceptor 0x800		/* Dimension has an acceptor function associated with it */
#define V4DPI_DimInfo_Displayer 0x1000		/* Dimension has a displayer function associated with it */
#define V4DPI_DimInfo_DotDotToList 0x2000	/* Eval [UV4:DimToList Dim:xxx] to convert dimension to list (e.g. dim..) */
#define V4DPI_DimInfo_used4000 0x4000		/* unused */
#define V4DPI_DimInfo_used8000 0x8000		/* unused */
#define V4DPI_DimInfo_Normalize 0x10000		/* Normalize Int2 so first number always less-equal than second */
#define V4DPI_DimInfo_CaseSensitive 0x20000	/* XDict entries are case-sensitive */
#define V4DPI_DimInfo_Structure 0x40000		/* List point used to declare XML structures - (name value) pairs */
#define V4DPI_DimInfo_DotIndex 0x80000		/* Allow syntax of form 'dim.pt' to be converted to 'List(dim* Nth::pt)' */
#define V4DPI_DimInfo_RDBNumeric 0x100000	/* For Dict/XDict/Displayer values - store numeric value in RDB (see Format(xxx RDB::type)) */
#define V4DPI_DimInfo_HaveNone 0x200000		/* Have a dim:none default value specified */
#define V4DPI_DimInfo_DotList 0x400000		/* Allow syntax of form 'dim.pt.pt' to be converted to '[UV4:Parse dim:(pt pt ...)]' */
#define V4DPI_DimInfo_NoNamePrefix 0x800000	/* Don't show dimension name (name:) when outputting points on the dimension */
#define V4DPI_DimInfo_ValueList 0x1000000	/* Allow dim:(xxx xxx) -> [UV4:Value(s) dim (xxx xxx)] */
#define V4DPI_DimInfo_ValueTree 0x2000000	/* Allow dim:(xxx xxx) -> [UV4:Value(s) dim treeNode] */

#define V4DPI_rtDimInfo_Initvcr 1	/* Have gone thru initialization for di->vcr for this dimension */
#define V4DPI_rtDimInfo_Flatten 2	/* This dimension participates in one (or more) multi-dim flattens */
#define V4DPI_rtDimInfo_Hidden 4	/* If TRUE then this dimension is hidden & cannot be referenced */
#define V4DPI_rtDimInfo_Saved 8		/* TRUE if dimension value has been 'saved' somewhere such that it may be referenced later (e.g. Sort(dim.. ...) - dim* is 'saved' because it is returned in resulting list) */
#define V4DPI_rtDimInfo_v4rpp 0x10	/* If set then this dimension has been output to v4rpp (more or less) */

#define V4DPI_DimInfoAlpha_XMLOK 1		/* Allow XML data via 'Context ADV xxx' (i.e. from Web to prevent cross-site scripting) */
#define V4DPI_DimInfoAlpha_IsFileName 0x2	/* Values of this dimension are filenames (stored as Alpha) */
#define V4DPI_DimInfoAlpha_AsIs 0x4		/* Take 'Context ADV' literal as is (i.e. no escape processing) - use with JSON strings */
#define V4DPI_DimInfoAlpha_ParseJSON 0x8	/* Take 'Context ADV' JSON string and parse immediately */

#define V4DPI_DimInfoList_IsctModOK 1		/* Allow Intersections and modules to be list points (Not good for lists passed in from WWW!) */

/* NOTE - if additional V4DPI_DimInfo_xxx flags added then check V4(Attributes?) module!!! */

struct V4DPI__DimInfo
{ union V4IS__KeyPrefix kp ;		/* Type=V4, SubType=DimInfo, Mode=Int, Length=8 */
  DIMID DimId ;				/* Dimension code (actually dictionary entry for Dim name) */
  FLAGS32 Flags ;			/* Holds V4DPI_DimInfo_xxx flags */
  UCCHAR DimName[V4DPI_DimInfo_DimNameMax+1] ;
  short PointType ;			/* Type of point allowed for this dimension- V4DPI_PntType_xxx */
  short AllValue ;			/* If non-zero then value to use for binding "dim:{all}" point */
  short UniqueOK ;			/* If not zero then how point to be created: V4DPI_DimInfo_UOkxxx */
  short DictType ;			/* Dictionary type- V4DPI_DictType_xxx */
  int BindList ;			/* If > 0 then dimension can become key to BindList entry */
  short UOMRef ;			/* Default UOM Ref for points on this dimension */
  short RelHNum ;			/* Relative Heirarchy Number for this dimension (for bindings) */
  UCCHAR Desc[127+1] ;			/* Optional description of the dimension */
  UCCHAR OutFormat[32] ;		/* Output format for point (if starts with "%" then C printf specification) */
  UCCHAR Attributes[128] ;		/* Any textual attributes associated with the dimension */
  short PCAggExtend ;			/* Percentage from 0 to 100 indicating % over standard record max for BFAggs on this dimension */
  short Decimals ;			/* Number of implied decimal places for this FIXED dimension */
  DIMID ListDimId ;			/* Dimension of list entries (optional) */
  union {
    int Filler[5] ;			/* Structure limited to 20 bytes */
    struct {				/* Specific to Char/UCChar/BigText (i.e. alpha) */
      int aFlags ;			/* Alpha specific flags (V4DPI_DimInfoAlpha_xxx) */
     } Alpha ;
    struct {				/* Specific to Lists */
      int lFlags ;			/*  flags - (V4DPI_DimInfoList_xxx) */
     } List ;
    struct  {				/* If Calendar PointType then... */
     short CalendarType ;		/* Type of calendar - see VCAL_CalType_xxx */
     short TimeZone ;			/* Time zone - see VCAL_TimeZone_xxx */
     YMDORDER YMDOrder[VCAL_YMDOrderMax] ;	/* Ordering of year-month-day in dates (see V4LEX_YMDOrder_xxx) */
     short IFormat ;			/* Input format for AcceptPoint (see V4LEX_TablePT_xxx) */
      } Geo ;				/* Dimension Specific */
    struct  {				/* If Calendar PointType then... */
     short CalendarType ;		/* Type of calendar - see VCAL_CalType_xxx */
     short TimeZone ;			/* Time zone - see VCAL_TimeZone_xxx */
     YMDORDER YMDOrder[VCAL_YMDOrderMax] ;	/* Ordering of year-month-day in dates (see V4LEX_YMDOrder_xxx) */
     short IFormat ;			/* Input format for AcceptPoint (see V4LEX_TablePT_xxx) */
     short calFlags ;			/* Calendar-Date flags- VCAL_Flags_xxx */
     } Cal ;				/* Dimension Specific */
    struct {				/* UDate specific formats */
     YMDORDER YMDOrder[VCAL_YMDOrderMax] ;	/* Ordering of year-month-day in dates (see V4LEX_YMDOrder_xxx) */
     short IFormat ;			/* Input format for AcceptPoint (see V4LEX_TablePT_xxx) */
     short calFlags ;			/* Calendar-Date flags- VCAL_Flags_xxx */
    } UDate ;
    struct {				/* Date-Time specific formats */
     YMDORDER YMDOrder[VCAL_YMDOrderMax] ;	/* Ordering of year-month-day in dates (see V4LEX_YMDOrder_xxx) */
     short IFormat ;			/* Input format for AcceptPoint (see V4LEX_TablePT_xxx) */
     short calFlags ;			/* Calendar-Date flags- VCAL_Flags_xxx */
      } UDT ;
    struct {				/* UMonth specific formats */
     short YMOrder ;			/* Ordering of year-month-day in dates (see V4LEX_YMOrder_xxx) */
     short IFormat ;			/* Input format for AcceptPoint (see V4LEX_TablePT_xxx) */
     short calFlags ;			/* Calendar-Date flags- VCAL_Flags_xxx */
      } UMonth ;
    struct {				/* UMonth specific formats */
     short periodsPerYear ;		/* Number of periods in a year (ex: 13 or 12) */
     short calFlags ;			/* Calendar-Date flags- VCAL_Flags_xxx */
      } UPeriod ;
    struct {				/* UMonth specific formats */
     short calFlags ;			/* Calendar-Date flags- VCAL_Flags_xxx */
      } UQuarter ;
    struct {				/* UWeek specific formats */
     int baseUDate ;			/* If != VCAL_UDate_None then base universal date for UWeek calcuations */
     short calFlags ;			/* Calendar-Date flags- VCAL_Flags_xxx */
      } UWeek ;
    struct {				/* UWeek specific formats */
     short calFlags ;			/* Calendar-Date flags- VCAL_Flags_xxx */
      } UYear ;
    struct {				/* Integer specific formats */
     short IFormat ;			/* Input format for AcceptPoint (see V4LEX_TablePT_xxx) */
     int NoneValue ;			/* Value to use for dim:none (only if V4DPI_DimInfo_HaveNone is set) */
      } Int ;
    struct {				/* Real specific formats */
     double NoneValue ; 		/* Value to use for dim:none (only if V4DPI_DimInfo_HaveNone is set) */
      } Real ;
   } ds ;
  struct {
    XDBRECID lastRecId ;		/* Last record Id associated with this dimension (updated at run-time) */
   } XDB ;
  int ToBeDetermined[5] ;		/* For future expansion */
  UCCHAR IsA[V4DPI_DimInfo_DimNameMax+1] ; /* Name of dimension this dimension "isa" */
  struct V4DPI__LittlePoint ADPnt ;	/* Acceptor/Displayer Point */
  UCCHAR ADPntStr[32] ;			/* String version of above point */
//  int AcceptorObject[2] ;		  /* OBJREF for acceptor module */
//  char DisplayerModule[31+1] ;	  /* Name of V3 Displayer module */
//  int DisplayerObject[2] ;		  /* OBJREF for displayer */
					/* Following MUST be at end of dimension info !!! */
  struct V4DPI__LittlePoint AutoCtxPnt ; /* If not NULL then point to be used for Auto-Context lookups */
// From here down is filled in at runtime */
  struct V4DPI__CodedRange *vcr ;	/* If not NULL then pointer to coded range structure */
  FLAGS32 rtFlags ; 			/* Flags set at runtime for dimension - V4DPI_rtDimInfo_xxx */
} ;


#define DIMDIR_Max 1000			/* Max number of dimension references per block below */
#define DIMREF_IRT 0			/* Dimension reference - internal runtime reference - don't track */
#define DIMREF_DCL 1			/* Dimension reference - dimension declaration */
#define DIMREF_DIMDIM 2			/* Referenced as Dim:xxx */
#define DIMREF_REF 3			/* Normal reference xxx:value */
#define DIMREF_LEX 4			/* Lexical reference (ex: If defined xxx ...) */
#define DIMREF_POINT 5			/* Within POINT command (ex: POINT xxx val val ...) */
struct V4LEX__DIMDIR {			/* Tracks source/line number of every dimension reference */
  struct V4LEX__DIMDIR *vlddPrior ;
  COUNTER count ;
  struct {
    ACTDIMID dimId ;
    unsigned short usage ;		/* 1 = defined, 2 = dim:xxx usage, 3 = Dim:dim reference */
    union V4DPI__IsctSrc vis ;
  } entry[DIMDIR_Max] ;
} ;
    


struct V4DPI__DimUnique 		/* Structure of record holding next available point number for a dimension */
{ union V4IS__KeyPrefix kp ;		/* Type=V4, SubType=DimInfo, Mode=Int, Length=8 */
  DIMID DimId ;				/* Dimension code */
  int Revision ;			/* Usually 0, if > 0 then supercedes those of lower revision (for multi-areas) */
  unsigned int NextAvailPntNum ;	/* Next available number (will have RelH prefix!) [SHOULD BE UNSIGNED!!!] */
} ;

/*	Macro to update res with current value of dimension in context (replaces call to v4ctx_DimValue) */
/*	DIMVAL - Updates res (pointer) with value of dimid in current context (NULL if not in ctx) */
#define DIMVAL(res,ctx,dimid) \
 { struct V4DPI__DimInfo *_di = NULL ; _DIMVAL(res,ctx,dimid,_di) \
 }
/*	DIMVALwX - same as above put also updates hx with index within context table so that it can be updated quickly */
#define DIMVALwX(res,ctx,dimid,diUpd) { _DIMVAL(res,ctx,dimid,diUpd) }
 
/*	_DIMVAL - common macro body for macros above */
#ifdef V4ENABLEMULTITHREADS
#define _DIMVAL(res,ctx,dimid,diUpd) \
   INDEX _hx,_i ; LOGICAL _keepgoing ; DIMID _dimid ; \
   struct V4C__Context *cctx ; \
   _dimid = dimid ; res = NULL ; _keepgoing = (_dimid > 0) ; \
   for(cctx=ctx;cctx!=NULL&&_keepgoing;cctx=cctx->pctx) \
    { _hx = DIMIDHASH(_dimid) ; \
      for(;;) \
       { if (cctx->DimHash[_hx].Dim == _dimid) \
          { _i = cctx->DimHash[_hx].CtxValIndex ; \
	    if (_i != -1) \
	     { if (cctx->CtxVal[_i].UnDefined) { res = NULL ; _keepgoing = FALSE ; break ; } ; \
	       if (diUpd != NULL) diUpd = cctx->DimHash[_hx].di ; res = &cctx->CtxVal[_i].Point ; _keepgoing = FALSE ; break ; \
	     } ; \
	    if (cctx->DimHash[_hx].di->AutoCtxPnt.Bytes == 0) { break ; } ; \
	    res = v4ctx_DimValue(cctx,_dimid,&diUpd) ; _keepgoing = FALSE ; break ; \
          } ; \
         if (cctx->DimHash[_hx].Dim == 0) { res = v4ctx_DimValue(cctx,_dimid,&diUpd) ; _keepgoing = FALSE ; break ; } ; \
         _hx++ ; _hx = DIMIDHASH(_hx) ; \
       } ; \
    } ;
#else
#define _DIMVAL(res,ctx,dimid,diUpd) \
  INDEX _hx,_i ; \
   _hx = DIMIDHASH(dimid) ; \
   for(;;) \
    { if (ctx->DimHash[_hx].Dim == dimid) \
       { _i = ctx->DimHash[_hx].CtxValIndex ; \
	 if (_i != -1) \
	  { if (ctx->CtxVal[_i].UnDefined) { res = NULL ; break ; } ; \
	    if (diUpd != NULL) diUpd = ctx->DimHash[_hx].di ; res = &ctx->CtxVal[_i].Point ; break ; \
	  } ; \
	 if (ctx->DimHash[_hx].di->AutoCtxPnt.Bytes == 0) { res = NULL ; break ; } ; \
	 res = v4ctx_DimValue(ctx,dimid,&diUpd) ; break ; \
       } ; \
      if (ctx->DimHash[_hx].Dim == 0) { res = v4ctx_DimValue(ctx,dimid,&diUpd) ; break ; } ; \
      _hx++ ; _hx = DIMIDHASH(_hx) ; \
   } ;
#endif

#define DIMPVAL(res,ctx,dimid) res = (P *)v4ctx_DimPValue(ctx,dimid)

/*	Macro to update res with di pointer (replaces call to v4dpi_DimInfoGet) */
#define DIMINFO(res,ctx,dimid) \
 { INDEX _hx ; register DIMID did=dimid ; \
   _hx = DIMIDHASH(did) ; \
   for(;;) \
    { if (ctx->DimHash[_hx].Dim == did) \
       { if (ctx->DimHash[_hx].di != NULL) { res = ctx->DimHash[_hx].di ; break ; } ; \
	 res = (struct V4DPI__DimInfo *)v4dpi_DimInfoGet(ctx,did) ; break ; \
       } ; \
      if (ctx->DimHash[_hx].Dim == 0) { res = (struct V4DPI__DimInfo *)v4dpi_DimInfoGet(ctx,did) ; break ; } ; \
      _hx++ ; _hx = DIMIDHASH(_hx) ; \
    } ; \
 }

/*	Structure to hold blocks of Dimension Names	*/

#define V4DPI_DimNameBlockMax 50	/* Max number of nested blocks */
#define V4DPI_DimNameBlockNameMax 100	/* Max number of dimension names */

struct V4DPI__DimNameBlocks
{ INDEX BlockX ;			/* Current Block index */
  struct {
    COUNTER NameCount ;			/* Count of Names below */
   } Block[V4DPI_DimNameBlockMax] ;
  struct {
    UCCHAR IDimName[V4DPI_DimInfo_DimNameMax+1] ; /* Internal dimension name */
    UCCHAR DimName[V4DPI_DimInfo_DimNameMax+1] ;	/* External dimension name */
   } Names[V4DPI_DimNameBlockNameMax] ;
} ;

#define V4DPI_PointIntMix_Max 125

struct V4DPI__Point_IntMix		/* Defines a list of integer points/ranges */
{ struct { int BeginInt,EndInt ;	/* Begin & End in range (end=begin for single) */
	 } Entry[V4DPI_PointIntMix_Max] ;
} ;

#define V4DPI_PointRealMix_Max 100

struct V4DPI__Point_RealMix		/* Defines a list of real(double) points/ranges */
{ struct { BYTE BeginReal[sizeof(double)],EndReal[sizeof(double)] ;
	 } Entry[V4DPI_PointRealMix_Max] ;
} ;

#define V4DPI_PointFixMix_Max 50

struct V4DPI__Point_FixMix		/* Defines a list of Fix(B64INT) points/ranges */
{ struct { BYTE BeginFix[sizeof(B64INT)],EndFix[sizeof(B64INT)] ;
	 } Entry[V4DPI_PointFixMix_Max] ;
} ;

#define V4DPI_PointAlphaMix_Max 50	/* Max number of Alpha mixes in point */

struct V4DPI__Point_AlphaMix
{ struct { unsigned short BeginIndex ;
	   unsigned short EndIndex ;
	 } Entry[V4DPI_PointAlphaMix_Max] ;
} ;

#define V4DPI_PointUOMMix_Max 50	/* Max number of UOM mixes in point */

struct V4DPI__Point_UOMMix
{ struct {struct V4DPI__Value_UOM BeginUOM,EndUOM ;
	 } Entry[V4DPI_PointUOMMix_Max] ;
} ;

#define V4DPI_PointUOMPerMix_Max 50	/* Max number of UOM/Per mixes in point */

struct V4DPI__Point_UOMPerMix
{ struct {struct V4DPI__Value_UOMPer BeginUOMPer,EndUOMPer ;
	 } Entry[V4DPI_PointUOMPerMix_Max] ;
} ;

#define V4DPI_PointUOMPUOMMix_Max 50	/* Max number of UOM/PerUOM mixes in point */

struct V4DPI__Point_UOMPUOMMix
{ struct {struct V4DPI__Value_UOMPUOM BeginUOMPUOM,EndUOMPUOM ;
	 } Entry[V4DPI_PointUOMPUOMMix_Max] ;
} ;

union V4DPI__PointMask
{ struct
   { unsigned AreaId : 8 ;			/* Area "owning" point */
     unsigned long PointId : 24 ;		/* Point Id */
   } fld ;
  int all ;
} ;

#define V4TMBufMax UCReadLine_MaxLine		/* Text/Error Message buffer max (cannot be smaller than UCReadLine_MaxLine) */
#define V4DPI_EM_Normal 0			/* Normal evaluation */
#define V4DPI_EM_EvalQuote 1			/* Eval even if quoted */
#define V4DPI_EM_NoIsctFail 2			/* Don't try & eval via v4dpi_IsctEvalFail() */
#define V4DPI_EM_NoCTXisAll 4			/* If item not in context, assume dim.. in context */
#define V4DPI_EM_NoNest 8			/* Don't continue nested evaluation if result is Isct/IntMod */
#define V4DPI_EM_NoRegisterErr 0x10		/* Don't call REGISTER_ERROR on failure */
#define V4DPI_EM_FailOK 0x20			/* Failing is OK - don't perform any error msg generation, retry, etc. - just return NULL */
#define V4DPI_EM_SavedErrSet 0x40 		/* Set by IsctEval if SavedErrorMsg already set */
#define V4DPI_EM_NestUV4Fail 0x80		/* Evaluating UV4:IsctFail, insert UV4:{undefined} into context before nesting */
#define V4DPI_EM_NoContinue 0x100		/* Don't try ',xxx' if isct fails, return failure */
#define V4DPI_EM_NoTrace 0x200			/* Don't try to echo trace statements (to prevent nasty recursion with V4DPI_FormatOpt_DisplayerTrace option) */

//#define V4TRACE_MaxNestedIsctEval 80		/* Max number of nested Isct's */
//#define V4TRACE_MaxNestedIntMod 80		/* Max number of nested IntMod's */
//#define V4TRACE_WarnNestedIntMod (V4TRACE_MaxNestedIntMod - 15) /* When to issue stack warning */

#define V4TRACE_MaxNestedrtStack 200		/* Max number of nested Isct's */
#define V4TRACE_WarnNestedrtStack (V4TRACE_MaxNestedrtStack - 5) /* When to issue stack warning */
#define V4C_Frame_Max V4TRACE_MaxNestedrtStack+32   /* Max number of nested frames */


#define V4TRACE_LogIscts 2			/* Log/Trace all Isct evaluations, results, failures */
#define V4TRACE_BindList 4			/* Trace bindlists (NBLs) as accessed for lookups */
#define V4TRACE_EvalPt 8			/* Trace EvalPt() */
#define V4TRACE_PointCompare 0x10		/* Trace point compares between isct & nbl */
#define V4TRACE_ContextAdd 0x20 		/* Trace points added to context as part of isct eval */
#define V4TRACE_AutoContext 0x40		/* Trace context points tried & failed via auto-context */
#define V4TRACE_EvalToContext 0x80		/* Trace dim:=[] updates to context */
#define V4TRACE_Progress 0x100			/* Trace progress of Tally() & Enum() */
#define V4TRACE_TallyBind 0x200 		/* Trace all Tally() binds */
//#define V4TRACE_Prompt 0x400			  /* Prompt user on what to do if Isct fails */
#define V4TRACE_Bindings 0x800			/* Show all bindings as they are formed */
#define V4TRACE_Macros 0x1000			/* Show all Macros being written */
#define V4TRACE_Lists 0x2000			/* Show Lists */
#define V4TRACE_Context 0x4000			/* Show points added to context */
#define V4TRACE_Points 0x8000			/* Show points added via POINT in vreval */
#define V4TRACE_BindEval 0x10000		/* Trace all Bind-Eval's */
#define V4TRACE_Arith 0x20000			/* Trap arithmetic errors (like div by 0) */
#define V4TRACE_V4IS 0x40000			/* Trace V4ISxxx() failures */
#define V4TRACE_IsctFail 0x80000		/* Trace [UV4:IsctFail] attempts */
#define V4TRACE_Optimize 0x100000		/* Display all optimized Do() entries */
#define V4TRACE_Errors 0x200000 		/* Trace errors (that fail, as opposed to generate V4 Error) */
#define V4TRACE_XDB 0x400000 			/* Trace XDB events */
#define V4TRACE_TimeStamp 0x800000		/* Output time-cpu stamp on log outputs */
#define V4TRACE_IsAEval 0x40000000		/* Calling IsctEval to eval [IsA xxx] - don't recurse again */
#define V4TRACE_Recursion 0x80000000		/* Track calls & auto-break if recursion detected */
//#define V4TRACE_Return 0x80000000		/* Return from IsctEvalPH if no eval (internal only) */

//#define V4TRACE_All (-1 & ~V4TRACE_Prompt)	/* Enable all tracing except heavy duty stuff */
#define V4TRACE_All -1				/* Enable all tracing except heavy duty stuff */

#define V4_EXSTATE_Stack 1			/* Show runtime stack in v4trace_ExamineState */
#define V4_EXSTATE_Context 2			/*  show context */
#define V4_EXSTATE_Areas 4			/*  show open areas */
#define V4_EXSTATE_Aggs 8			/*  show open aggregates */

#define V4_EXSTATE_All -1			/* Show it all */

/*	C O N T I N U A T I O N   S O U R C E   C O D E S	*/
#define V4DPI_ContFrom_NestedArgEval 0		/* 2nd arg is argument index */
#define V4DPI_ContFrom_EvalCurVal 1		/* 2nd arg is DimId */
#define V4DPI_ContFrom_EvalPCurVal 2		/* 2nd arg is DimId */
#define V4DPI_ContFrom_NoBinding 3		/* 2nd arg ignored */
#define V4DPI_ContFrom_IntModFail 4		/* 2nd arg is IntModX */
#define V4DPI_ContFrom_2ndIsctEval 5		/* 2nd arg ignored */
#define V4DPI_ContFrom_NoDim 6			/* 2nd arg ignored */

/*	Macro to to perform fast IsctEval if there is a good chance the Isct is an IntMod	*/
/*	NOTE: if debugging (i.e. gpi->bpl is not NULL) then don't optimize, got thru IsctEval so that breakpoints can be handled properly */
#ifdef NASTYBUG
#define FASTISCTEVAL(ISCT,RESPNT,RESPNTBUF) \
 if ((ISCT)->PntType == V4DPI_PntType_Isct && (ISCT)->Grouping > 0 && ISCT1STPNT(ISCT)->PntType == V4DPI_PntType_IntMod && gpi->bpl == NULL) \
  { ctx->IsctEvalCount++ ;ctx->rtStackX++ ; ctx->rtStack[ctx->rtStackX].isctPtr = ISCT ; RESPNT = v4im_IntModHandler((ISCT),(RESPNTBUF),ctx,V4DPI_EM_EvalQuote) ; ctx->rtStackX-- ; \
    ctx->isctIntMod = NULL ; \
   } else { RESPNT = v4dpi_IsctEval((RESPNTBUF),(ISCT),ctx,V4DPI_EM_EvalQuote,NULL,NULL) ; } ;
#else
#define FASTISCTEVAL(ISCT,RESPNT,RESPNTBUF) \
 if ((ISCT)->PntType == V4DPI_PntType_Isct && (ISCT)->Grouping > 0 && ISCT1STPNT(ISCT)->PntType == V4DPI_PntType_IntMod && gpi->bpl == NULL) \
  { ctx->IsctEvalCount++ ;ctx->rtStackX++ ; ctx->rtStack[ctx->rtStackX].isctPtr = ISCT ; RESPNT = v4im_IntModHandler((ISCT),(RESPNTBUF),ctx,V4DPI_EM_EvalQuote) ; ctx->rtStackX-- ; \
   } else { RESPNT = v4dpi_IsctEval((RESPNTBUF),(ISCT),ctx,V4DPI_EM_EvalQuote,NULL,NULL) ; } ;
#endif

#define ISVALIDDIM(DIMID,ARG,MODULE) \
  if (DIMID < 0 || DIMID > 0xffff) v4_error(0,0,"V4","DimInfo","NOTVALIDDIM","Argument #%d to %s not a valid dimension",ARG,MODULE) ;
#define XTRHNUMDIM(dimid) (dimid>>13)		/* Extract heirarch num from dimension ID */
#define ADDHNUMDIM(hnum,dimid) ((hnum<<13)|(dimid&0x1FFF))	/* Add heirarchy number to dimension */
#define XTRHNUMDICT(dictnum) ((((unsigned int)dictnum)>>28)&7)	/* Extract hnum from dictionary entry (external) */
#define HNUMDICTMASK 0x0FFFFFFF
#define ADDHNUMDICT(hnum,dictnum) ((hnum<<28)|(dictnum&HNUMDICTMASK))
#define MAXDIMNUM 0xFFFF			/* Highest allowed dimension number (including 3 bit prefix) */
#define DIMDICT (struct V4DPI__DimInfo *)-1	/* Special flag to tell DictEntryGet to look for dimension name */
#define V4DPI_DictType_Dim 1			/* A dimension entry */
#define V4DPI_DictType_Int 2			/* An internal (to area) entry */
#define V4DPI_DictType_Ext 3			/* An external entry (can participate in bindings in other areas) */
#define V4DPI_DictEntryVal_Max 31		/* Max number of bytes in dictionary entry */
#define V4DPI_XDictEntryVal_Max 96		/* Max number of bytes in external dictionary entry */
#define DICTENTRYMT UClit("\01")		/* Special string to denote empty string */

struct V4DPI__DictEntry
{ union V4IS__KeyPrefix kp ;			/* Type=V4, SubType=Dict, Mode=Alpha, */
  UCCHAR EntryVal[V4DPI_DictEntryVal_Max+32] ;	/* Dictionary entry */
  /* int EntryId ; */				/* Corresponding numeric entry (immediately follows EntryVal) */
} ;

struct V4DPI__DictEntryDim			/* Fits in DictEntry above (instead of EntryId) for entry,dim combo */
{ int Marker ;					/* Must be 0 so we know what follows */
  int EntryId ; 				/* Dictionary entry */
  DIMID DimId ;					/* Preferred dimension for this entry */
} ;

struct V4DPI__RevDictEntry
{ union V4IS__KeyPrefix kp ;			/* Type=V4, SubType=RevDict, Mode=Int, Len=8 */
  DICTID EntryId ; 				/* Numeric entry (ref) */
  UCCHAR EntryVal[V4DPI_DictEntryVal_Max+4] ;	/* Corresponding Dictionary entry */
} ;

#define V4DPI_DictListMax 75000

struct V4DPI__DictList				/* For enumerating thru all dictionary entries */
{ COUNTER Count ;
  struct {
    DIMID DimId ;
    DICTID DictId ;
   } Entry[V4DPI_DictListMax] ;
} ;

/*	UOM Format Structure			*/

#define V4DPI_UOMTable_Max 100			/* Max number of defined UOM Tables */
#define V4DPI_UOMDetails_Max 50 		/* Max number of entries in a UOM Table */

#define V4DPI_CaseCount_Mult 100		/* Multiplier for CaseCounts (casecount*multiplier+index) */

#define V4DPI_UOMAttr_FractionOK 1		/* Fractional Qty's OK */
#define V4DPI_UOMAttr_CoMingle 2		/* Entries within a UOM cannot be co-mingled (ex: added together) */
#define V4DPI_UOMAttr_DisplayPSfix 4		/* If set then display prefix/suffix of UOM */

#define UOM_GENERIC_REF 0			/* Ref number for auto-created generic UOM */

struct V4DPI__UOMTable
{ COUNTER Count ;				/* Number of entries to follow */
  struct {
    int Ref ;					/* UOM Ref Number */
    struct V4DPI_UOMDetails *uomd ;		/* Pointer to UOM Details */
   } Entry[V4DPI_UOMTable_Max] ;
} ;

struct V4DPI_UOMDetails 			/* Details for a Particular UOM Table */
{ UCCHAR Name[31+1] ;				/* Optional name for this table */
  UCCHAR Desc[99+1] ;				/* Optional description for this table */
  struct V4DPI__LittlePoint TypePt ;		/* User defined UOM Type */
  int Ref ;					/* UOM Ref this table belongs to */
  INDEX DfltDispX ;				/* Subscript to default display below (UNUSED for "AsIs") */
  INDEX DfltAcceptX ;				/* Default acceptor below (UNUSED for unit (i.e. UOM of 1) */
  ETYPE Attributes ;				/* Attributes for this UOM - see V4DPI_UOMAttr_xxx */
  COUNTER CaseCount ;				/* If > 0 then CaseCount (i.e. casecount embedded in UOM Index) */
  COUNTER Rounding ;				/* Number of decimals places to keep (-n = number to left of decimal) */
						/*   9999 indicates unused */
  COUNTER Count ;					/* Number of detailed entries to follow */
  struct {
    double preOffset,postOffset ;		/* Pre & Post offsets (ex: for converting C->F => 9/5*C + 32, F->C => (F - 32)*0.55555556) */
    double Factor ;				/* Factor to multiply by to get base UOM (if 0 then assumed to be casecount) */
    INDEX Index ; 				/* Internal entry index (can be 1 thru V4DPI_CaseCount_Mult) */
						/* If -1 then entry for uom's with uom->Index < 0 (embedded factor) */
    int Prefix ;				/* True if PSStr is Prefix, False if Suffix */
    UCCHAR PSStr[8] ;				/* Prefix/Suffix String */
   } UEntry[V4DPI_UOMDetails_Max] ;
} ;

/*	E X T E R N A L   D I C T I O N A R Y	*/

#define V4DPI_XDict_MustExist 1 		/* Don't auto-create if entry does not exist */
#define V4DPI_XDict_DimDim 2			/* Looking up mapping from DimId to DimXId */

struct V4DPI__XDictEntry			/* String->Number (use for Point->Number mapping) */
{ union V4IS__KeyPrefix kp ;			/* Type=V4, SubType=IntXDict, Mode=Alpha, Len=variable */
  DIMID DimXId ;				/* Dimension index (UNUSED for mapping Dim->DimXId) */
  UCCHAR EntryVal[V4DPI_XDictEntryVal_Max+32] ;	/* Dictionary entry */
  /* int EntryId ; */				/* Corresponding numeric entry (immediately follows EntryVal) */
} ;

struct V4DPI__RevXDictEntry
{ union V4IS__KeyPrefix kp ;			/* Type=V4, SubType=RevXDict, Mode=Int2, Len=12 */
  DIMID DimXId ;				/* XDict dimension  */
  DICTID EntryId ; 				/* Numeric entry (ref) */
  UCCHAR EntryVal[V4DPI_XDictEntryVal_Max+4] ;	/* Corresponding Dictionary entry */
} ;

struct V4DPI_XDictDimPoints
{ union V4IS__KeyPrefix kp ;			/* Type=V4, SubType=XDictDimPoints, Mode=Int, Len=8 */
  DIMID DimXId ;				/* XDict dimension */
  DICTID LastPoint ;				/* Last used point number for this dimension */
} ;

/*	P R O J E C T I O N   D A T A		*/

#define V4DPI_ProjErr_Point 1			/* On error, return contents of ErrorPt */
						/*   Note- if isct then eval & auto-update projection
							   if @isct then eval & return as value (no update) */
#define V4DPI_ProjErr_Fail 2			/* On error, return failure (NIL) */
#define V4DPI_ProjErr_Error 3			/* On error, generate V4 error */

struct V4DPI__ProjectionInfo			/* Defines how to project from one dimension to another */
{ union V4IS__KeyPrefix kp ;			/* Type=V4, SubType=V4IS_SubType_ProjectionInfo, Mode=Int, Len=8 */
  struct {
    unsigned short TargetDim ;			/* Target dimension */
    unsigned short SrcDim ;			/* Source dimension for projection */
   } KeyVal ;
  LENMAX Bytes ;				/* Number of bytes in this info record (depends on ErrorPt) */
  int aid ;					/* Updated at runtime with aid for this projection */
  LENMAX CacheSize ;				/* Number of entries in cache */
  LENMAX MaxKeySize ;				/* Max number of bytes in key */
  LOGICAL Permanent ;				/* TRUE if projection permanently maintained, FALSE if process local */
  LOGICAL MultiTarget ;				/* TRUE if single source can have multiple targets */
  ETYPE ErrorType ;				/* How to handle errors- see V4DPI_ProjErr_xxx */
  LOGICAL Update ;				/* If TRUE then update projection on error with ErrorPt value */
//  int hanWhii ; 				/* Handle to whii if a cached projection */
  void *ptrWH ;					/* Pointer to WH structure */
  ETYPE TargetPntType ;				/* PointType of Target */
#ifdef V4ENABLEMULTITHREADS
  DCLSPINLOCK prjLock ;				/* Multi-thread lock */
#endif
  struct V4DPI__Point ErrorPt ; 		/* Possible error point */
} ;

struct V4DPI__ProjectionData			/* Format of entry for projecting dim:1 to dim2 */
{ union V4IS__KeyPrefix kp ;			/* Type=V4, SubType=V4IS_SubType_ProjectionData, Mode=Alpha, Len=nn */
  DIMID TargetDim ;				/* Target Dimension */
  P ptSrc ;					/* Source Point */
						/* NOTE: kp.Byes = kp + TargetDim + ptSrc.Bytes */
  BYTE Filler[1000] ;
  /* Target Dim point appears here after ptSrc */
} ;

/*	Structure for Coded Ranges		*/

struct V4DPI__CodedRange
{ int InternalValue ;				/* Coded Range - internal value */
  struct V4DPI__CodedRange *vcr ;		/* If not NULL then link to next one */
  struct V4DPI__Point *crPoint ;		/* Coded range point corresponding to internal value */
} ;

/*	Define special dimensions		*/

#define Dim_StartDimIndex 10			/* Starting number for other dimensions (must be greater than last above) */

/*	B I N D I N G	S T R U C T U R E S			*/

#define V4DPI_Binding_BufMax 0x5000		/* Max size of binding buffer */
#define V4DPI_BindWgt_PrecOffset 100		/* Offset for absolute bind-weight "precedence" in BindWgtAdj below */

#define NOWGTADJUST 0				/* Do not adjust binding weight on call to v4dpi_BindListMake() */
#define DFLTRELH UNUSED				/* Auto-select appropriate RelH[] area to insert binding */

struct V4DPI__Binding
{ union V4IS__KeyPrefix kp ;			/* Type=V4, SubType=Value, Mode=Int, Len=8 */
  int ValueId ; 				/* Key to this value (see bindlist below for linkage) */
  unsigned short Bytes ;			/* Number of bytes in this binding */
  unsigned short IsctIndex ;			/* Index into Buffer for start of intersection */
#ifdef NOTYET
  short BindWgtAdj ;				/* Initial adjustment to binding weight */
#endif
  int BindWgt ; 				/* Binding Weight */
  BYTE Buffer[V4DPI_Binding_BufMax] ;		/* Value then Isct in here (NOTE: Must be "int" aligned!!!) */
} ;

/*	Format of BindList (this is rather complicated- schematic below should help */

/*	header info (V4DPI__BindList)
	BindList_Value entry			Points immediate follow (Buffer[ValueStart])
	BindList_Value entry			Multiple points in decreasing weight
	BindList_Value entry			 so that first one to "match" is the one to take
	...
	BindList_Dim				at Buffer[DimStart] & contains list of offsets to points below
						  (Note: first offset is 0 so that all this can float)
	dim=value point 			starting at Buffer[PointStart]
	dim=value point 			these points are one after the other, offset by "size" field
	...
*/

#define V4DPI_BindList_BufMax 14000		/* Max bytes in binding list */

//#define V4DPI_BindLM_Pattern 0x1		/* Make binding a pattern */
#define V4DPI_BindLM_WarnDupBind 0x2		/* Output warning if duplicate binding being replaced */

struct V4DPI__BindList
{
  unsigned short Bytes ;			/* Number of bytes in this binding list */
  unsigned short ValueCnt ;			/* Number of values */
  unsigned short ValueStart ;			/* Starting index below for values */
  unsigned short DimStart ;			/* Index below to start of dim=value points */
  unsigned short PointCnt ;			/* Number of dim=value points */
  unsigned short PointStart ;			/* Start within Buffer below */
  int LUBindId ;				/* Last used bind id for this list */
  BYTE Buffer[V4DPI_BindList_BufMax] ;		/* Data buffer (NOTE: Must be "int" aligned) */
} ;

struct V4DPI__BindListShell			/* Outer shell to hold __BindList as V4 record	*/
{ struct V4IS__Key blsk ;			/* Type=Binding, AuxVal=Dim, key=point, Mode=Int, Len=8 */
  BYTE Buffer[sizeof(struct V4DPI__BindList)] ; /* nbl info follows in next free (int aligned) position in above key */
} ;

#define V4DPI_IsctDimMax 24			/* Max number of dimensions in an intersection */

struct V4DPI__BindList_Value
{ struct V4DPI__ShortPoint sp ; 		/* Bind point value (if sp.Bytes == 0 then sp.Value.IntVal is ValueId) */
  int BindWgt ; 				/* Binding Weight */
  BYTE Bytes ; 					/* Number of bytes (aligned) in this entry */
  BYTE BindId ;					/* Binding Id for this value */
  BYTE IndexCnt ;				/* Number of index entries below */
  BYTE BindFlags ;				/* Optional flags: V4DPI_BindLM_xxx */
  BYTE DimPos[V4DPI_IsctDimMax] ;		/* Position in DimEntryIndex below of dim=value associated with this value */
} ;
typedef struct V4DPI__BindList_Value BLV ;	/* To make life easier */

#define V4DPI_BindList_DimMax 500		/* Max dimensions below */

struct V4DPI__BindList_Dim
{ short DimCnt ;				/* Number of dimensions indexed below */
  unsigned short DimEntryIndex[V4DPI_BindList_DimMax] ; /* Index to dim=value point in BindList.Buffer */
} ;

struct V4DPI__BindPointVal			/* Format of Value for a Binding Point */
{ unsigned short Dim ;				/* Dimension */
  unsigned short BindId ;			/* Binding Id in list */
  unsigned short AreaId ;			/* AreaId of area containing binding list */
  struct V4IS__Key bpKey ;			/* Bind point key */
} ;

#define V4DPI_EvalList_AreaMax 100

struct V4DPI__EvalList
{ LOGICAL Init ;				/* TRUE if this has been initialized */
  COUNTER SkipCnt ; 				/* Number of calls to skip before returning value (0 to return value) */
  COUNTER ReturnCnt ;				/* Number of times to return value */
  COUNTER AreaCnt ; 				/* Number of areas below */
  int AreaMatch ;				/* Set by IsctEval to area/nbl below which finally matched */
  int bindWgt ;					/* Binding weight of match */
  unsigned char MatchedDims[V4DPI_IsctDimMax] ;	/* Byte TRUE if correspondig point in isct matched */
  INDEX autoRmvDimCnt ;				/* Number of dimensions in list below */
  DIMID autoRmvDimList[V4DPI_IsctDimMax] ;	/* List of dimensions to be removed from context on successful evaluation of isct */
  struct V4DPI__BindList_Value *Resnblv ;	/* Resulting nblv from lookup */
  struct V4DPI__Point *ResPattern ;		/*  or resulting isct from pattern match-lookup-insert */
  struct
   { struct V4DPI__BindList *nbl ;		/* Bind list for this area */
     struct V4DPI__BindList_Dim *nbld ; 	/* Pointer to dimension list */
     struct V4DPI__BindList_Value *nblv ;	/* Pointer to current value in list */
     int Valx ; 				/* Current value index (0 thru nbl->ValueCnt) */
     int CurWgt ;				/* Weight of current contender (-1 pick next, 0= no more) */
     SBYTE DimTest[V4DPI_BindList_DimMax] ;	/* By dimension in nbld, 0=no test, 1=test OK, -1=test False */
     AREAID AreaId;				/* Area ID */
   } Area[V4DPI_EvalList_AreaMax] ;
} ;

#define V4DPI_AreaBindInfo_Dim_Max 250		/* Max number of dimensions in record below */

struct V4DPI__AreaBindInfo
{ union V4IS__KeyPrefix kp ;			/* Type = V4, Subtype=BindInfo, Mode=Int, Len=8 */
  int key ;					/* 0 for now */
  unsigned short Bytes ;			/* Number of bytes in this record */
  short DimCnt ;				/* Number of dimensions below */
  unsigned short DimsWithBind[V4DPI_AreaBindInfo_Dim_Max] ; /* List of dimensions in area with bind records */
} ;

/*	X M L S T A R T   R E T U R N   C O D E S		*/

#define XMLSTART_Fail 0				/* Routine failed, error in ctx->ErrorMsg */
#define XMLSTART_None 1				/* Encounted UV4:none so did nothing */
#define XMLSTART_Normal 2			/* Normal return */
#define XMLSTART_Defer 3			/* Calling module should defer output to '2nd pass' */

/*	F L A T T E N	S T R U C T U R E	*/

#define V4DPI_Flatten_NIdMax 20 		/* Max number of NId points to track */
						/* Used to prevent multiple bindings of form [nid dim.. dim.. ...] = [nid Flatten(dim* dim* ...)] */
#define V4DPI_Flatten_MaxEntry 256		/* Max number of Flatten() entries */

#define V4DPI_FTran_Copy 0			/* Just copy point value */
#define V4DPI_FTran_AlphaToHash64 -1		/* Use v_Hash64() routine to convert Alpha to 64-bit hash */
#define V4DPI_FTran_Hash32 -2			/* Use v_Hash32() to hash all args to 32 bits */
#define V4DPI_FTran_Hash64 -3			/* Use v_Hash64() to hash all args to 32 bits */
						/* Positive values for multi-dimensional indexing */

struct V4DPI__Flatten {
  struct V4DPI__Flatten *nftn ; 		/* If not NULL then link to next flatten structure */
  DIMID TargetDimId ;				/* The dimension of the resulting flattened point */
  PNTTYPE TargetPntType ;			/* The point type of resulting point */
  COUNTER Count ;				/* The number of participating dimensions below */
  LOGICAL Matched[V4DPI_IsctDimMax] ;		/* Set within v4dpi_FlattenIsct: TRUE if point[i] in srcisct matches Dim in flatten structure */
  int SeenNIds[V4DPI_Flatten_NIdMax] ;		/* Keep track of NIds seen when creating flattened bindings */
  COUNTER NIdCount ;				/* Number of NIds above */
  struct {
    DIMID DimId ; 				/* A participating dimension */
    LENMAX Bytes ; 				/* The number of bytes of value to use */
    ETYPE Transform ;				/* What sort of transform to use in flattening - V4DPI_FTran_xxx */
    P *spt ;					/* Set within v4dpi_FlattenIsct to corresponding point within srcisct */
   } Entry[V4DPI_IsctDimMax] ;
} ;

/*	J S O N   B L O B   S T R U C T U R E S			*/

#define VJSON_Type_None 0
#define VJSON_Type_Int 1
#define VJSON_Type_Double 2
#define VJSON_Type_String 3
#define VJSON_Type_Array 4
#define VJSON_Type_Object 5
#define VJSON_Type_Logical 6
#define VJSON_Type_Null 7

#define VJSON_Type_Undefined 15


struct VJSON__Value {
  unsigned jvType : 4 ;
  unsigned wIndex : 28 ;		/* Index to value in JSON data buffer */
} ;

struct VJSON__Value_Int {
  int intVal ;
} ;

struct VJSON__Value_Double {
  double dblVal ;
} ;

struct VJSON__Value_String {
  UCCHAR strVal[UCReadLine_MaxLine] ;	/* NULL-terminated string */
} ;

#define VJSON_Value_ArrayMax 0x20000	/* Maximum number of array elements */
struct VJSON__Value_Array {
  COUNTER count ;			/* Number of elements in array */
  struct VJSON__Value arrayVal[VJSON_Value_ArrayMax] ;
} ;

#define VJSON_Value_ObjectMax 0x20000	/* Maximum number of array elements */
struct VJSON__Value_Object {
  COUNTER count ;
  struct {
    DICTID dictId ;			/* Dictionary 'name' for this JSON blob */
    struct VJSON__Value jsonVal ;
   } objEntry[VJSON_Value_ObjectMax] ;	/* Name-value pairing */
} ;

#define VJSON_Blob_Max 0x1000
#define VJSON_Blob_Increment 0x1000
struct VJSON__Blob { 
  struct VJSON__Blob *vjblobNext ;	/* Pointer to next blob, NULL if end of chain */
  COUNTER bMax ;			/* Size allocated for this structure */
  COUNTER bRemain ;			/* Number bytes remaining in blob below */
  COUNTER bIndex ;			/* Current size of the blob */
  DICTID dictId ;			/* Dictionary name of this blob */
  struct VJSON__Value topVal ;		/* Top level value */
  BYTE blob[1] ;
} ;

struct VJSON__ParseInfo {
  struct V4C__Context *ctx ;
  struct V4LEX__TknCtrlBlk *tcb ;
  struct VJSON__Blob *vjblob ;
  struct V4DPI__DimInfo *di ;
} ;


#define VJSON_Parse_RelaxComma	1	/* Relax parsing of commas - allow ',,' in JSON expression */
#define VJSON_Parse_LeadingZero	2	/* Treat numeric token with leading zeros as keyword (i.e. zipcode:01234 -> zipcode:'01234' NOT zipcode:1234) */

/*	L I S T   S T R U C T U R E S				*/

/*	Fast macro to return V4L__ListPoint for list point */
#define VERIFYLIST(RESPOINT,CTX,LISTPOINT,INTMODX) \
  ((LISTPOINT)->PntType == V4DPI_PntType_List ? (struct V4L__ListPoint *)&(LISTPOINT)->Value : v4im_VerifyList(RESPOINT,CTX,LISTPOINT,INTMODX))

/*	Fast macro to return number of elements in a list */
#define SIZEofLIST(LISTPTR) \
 ((LISTPTR)->Entries > 0 ? (LISTPTR)->Entries : v4l_ListPoint_Value(ctx,LISTPTR,V4L_ListSize,NULL))

/*	Fast macro to return next value in list (especially if list is V4L_ListType_AggRef) */
#define FASTLISTPOINTVALUE(VALCODE,LP,INDEXVAL,RESPNTBUF) \
 if ((LP)->ListType != V4L_ListType_AggRef) { VALCODE = v4l_ListPoint_Value(ctx,LP,INDEXVAL,RESPNTBUF) ; } \
  else { struct V4L__ListAggRef *lar ; INDEX bx ; int num ; \
	 lar = (struct V4L__ListAggRef *)&LP->Buffer[0] ; VALCODE = 0 ; \
	 for(bx=0,num=INDEXVAL;bx<lar->Count;bx++) \
	  { if (num > (lar->Agg[bx].EndPt - lar->Agg[bx].BeginPt + 1)) \
	     { num -= (lar->Agg[bx].EndPt - lar->Agg[bx].BeginPt + 1) ; continue ; } ; \
	    ZPH((RESPNTBUF)) ; (RESPNTBUF)->PntType = V4DPI_PntType_AggRef ; \
	    (RESPNTBUF)->Bytes = V4PS_Int ; (RESPNTBUF)->Grouping = lar->AreaAggIndex[bx] ; (RESPNTBUF)->Dim = lar->Dim ; \
	    (RESPNTBUF)->Value.IntVal = lar->Agg[bx].BeginPt + num - 1 ; \
	    VALCODE = 1 ; break ; \
	  } ; \
       } ;

#define V4L_ListType_Point 1		/* List described by V4L__ListPoint */
#define V4L_ListType_CmpndRange 2	/* List described by V4L__ListPoint with sub record V4L__ListCmpndRange */
#define V4L_ListType_HugeInt 3		/* List described by V4L__ListHugeInt with sub record V4L__BlockEntry */
#define V4L_ListType_Isct 4		/* List is described by [isct] which is eval'ed until undefined */
#define V4L_ListType_AggRef 5		/* List is multiple ranges of aggregates to pull from one or more already loaded areas */
#define V4L_ListType_XDBGet 6		/* List of XDB fetched rows */
#define V4L_ListType_CmpndRangeDBL 7	/* Same as CmpndRange but for type double instead of int */
#define V4L_ListType_V4IS 8		/* List of V4IS records and maybe substructures */
#define V4L_ListType_Lazy 9		/* A list that is lazy-evaluated (i.e. a point at a time when needed) */
#define V4L_ListType_BitMap 10		/* A list is represented as a bitmap */
#define V4L_ListType_TextTable 11	/* A list of text lines parsed via TABLE entry */
#define V4L_ListType_HugeGeneric 12	/* A generic (as opposed to numeric) huge list */
#define V4L_ListType_MultiAppend 13	/* A list made up of multiple sub-lists */
#define V4L_ListType_MDArray 14 	/* A list from multi-dimensional array */
//#define V4L_ListType_HTMLTable 15	/* A list from an HTML TABLE (either rows or columns) */
#define V4L_ListType_Files 16		/* A list of files */
#define V4L_ListType_TextFile 17	/* A list of lines within a file */
#define V4L_ListType_BitMap64 18	/* A list is represented as a (64-bit word) bitmap */
#define V4L_ListType_Token 19		/* A list of tokens pulled from a tcb  */
#define V4L_ListType_XMLTokens 20	/* Return list of XML tokens */
#define V4L_ListType_BigText 21		/* A list of lines within a BigText buffer */
#define V4L_ListType_HTMLTokens 22	/* Return list of HTML tokens (very similar to XMLTokens) */
#define V4L_ListType_JavaTokens 23	/* Return list of Java(script) tokens */
#define V4L_ListType_JSON 24		/* A JSON BLOB reference - either array or object */
#define V4L_ListType_Drawer 25		/* A list of points within a drawer */

#define V4L_ListAction_Insert 50	/* Insert point at begin of list */
#define V4L_ListAction_Append 51	/* Append point at end of list */
#define V4L_ListAction_AppendUnique 52	/* Append point at end (if not in list already) */
#define V4L_ListAction_AppendMult 53	/* Append multiple points at end of list (append point is actually struct V4L_MultPoints) */
#define V4L_ListAction_AppendUniqueMult 54 /* Same as AppendMult but with Unique */

#define V4L_MultPointBufMax 4096
struct V4L_MultPoints {
  COUNTER pointCount ;			/* Number of points below */
  COUNTER totalBytes ;			/* Total number of bytes consumed by points below */
  BYTE pointBuf[V4L_MultPointBufMax] ;	/* Buffer for points */
} ;

#define V4L_ListPoint_BufMax V4DPI_AlphaVal_Max-34	/* Max bytes in ListPoint Buffer */
#define V4L_ListPoint_EntryMax 500	/* Max points in a list */
#define V4L_ListHuge_BlockMax 360	/* Max number of "directory block" entries for huge list */
#define V4L_BlockEntry_IntMax 4000	/* Max integers in list block */
#define V4L_BlockEntry_DblMax V4L_BlockEntry_IntMax/(sizeof(double)/sizeof(int))	/* Max doubles in list block */
#define V4L_BlockEntryGeneric_Max V4L_BlockEntry_IntMax * sizeof(int)

#define LISTVALERR -1			/* Error return value for v4l_ListPoint_Value */
#define V4L_ListSize -1			/* 'Index' to v4l_ListPoint_Value() to return number of elements in list */
#define V4L_ListSize_Unk -1		/* Returned as number of elements in list if number is not known */
#define V4L_NextElAsTempPt -2		/* Return "next" element to be used temporarily (i.e. return as quick-and-dirty as possible) */
#define V4L_AutoEndLvl 0x10000		/* Had to auto-end HTML level */

#define V4L_ModRes_OK 1			/* v4l_ListPoint_Modify returns OK */
#define V4L_ModRes_Fail 0		/*  fails */
#define V4L_ModRes_DupPnt 3		/* Point already exists with V4L_ListAction_AppendUnique */

/*	V4L__ListPoint - List of points 	*/
/*	This structure must be pointer aligned within V4 point Value (since substructures may contain pointers)
	  one must use the ALIGNLP macro to ensure this alignment!!!
*/

#define V4L_ListPointHdr_Bytes 24	/* NOTE: MUST BE NUMBER OF BYTES IN HEADER BELOW (see INITLP macro) */
struct V4L__ListPoint
{ unsigned short Bytes ;		/* Number of bytes in list */
  unsigned short ListType ;		/* Type of list- V4L_ListType_xxx */
  unsigned short MultiListIndex ;	/* Index to master table (xxx) with info on MultiAppend list */
  unsigned short BytesPerEntry ;	/* Number of bytes in each list entry (0 = entries variable len, use LPIOffset, etc.) */
  unsigned short Dim ;			/* Dimension of points (0 means each entry is own point) */
  unsigned short PntType ;		/* Point type for each entry (see above) */
  unsigned short LPIOffset ;		/* Offset to V4L__ListPointIndex structure (if necessary) */
  COUNTER Entries ; 			/* Number of entries in list (increment with each call if ListIsct) */
  int FFBufx ;				/* First free buffer index */
#pragma pack(8)
  BYTE Buffer[V4L_ListPoint_BufMax] ;	/* NOTE: must be pointer aligned!! */
#pragma pack()
} ;

/*	Macro to quickly create a little list from a list of points within a V4L_MultPoints (VMP) structure
	NOTE: OKToUse_littleListPNTv(VMP) MUST return TRUE in order to use this macro properly
*/
#define OKToUse_littleListPNTv(VMP) \
 ((VMP)->totalBytes + ((VMP)->pointCount * sizeof(short)) + V4L_ListPointHdr_Bytes < V4DPI_AlphaVal_Max - 128)
#define littlelistPNTv(POINT,VMP) \
 { struct V4L__ListPoint *_lp ; struct V4L__ListPointIndex *_lpi ; struct V4DPI__Point *_p ; INDEX _i ; \
   ZPH(POINT) ; (POINT)->Dim = Dim_List ; (POINT)->PntType = V4DPI_PntType_List ; _lp = (struct V4L__ListPoint *)&(POINT)->Value ; \
   _lp->ListType = V4L_ListType_Point ; _lp->Entries = (VMP)->pointCount ; \
   memcpy(_lp->Buffer,(VMP)->pointBuf,(VMP)->totalBytes) ; \
   _lp->LPIOffset = (VMP)->totalBytes ; _lpi = (struct V4L__ListPointIndex *)&_lp->Buffer[(VMP)->totalBytes] ; \
   for(_i=0,_p=(P *)(VMP)->pointBuf;_i<(VMP)->pointCount;_i++) \
    { _lpi->Offset[_i] = (char *)_p - (char *)(VMP)->pointBuf ; _p = (P *)((char *)_p + _p->Bytes) ; \
    } ; \
   _lp->Bytes = V4L_ListPointHdr_Bytes + (VMP)->totalBytes + ALIGN((VMP)->pointCount * sizeof(_lpi->Offset[0])) ; \
   (POINT)->Bytes = V4DPI_PointHdr_Bytes + _lp->Bytes ; \
 }

struct V4L__ListPointIndex		/* List of offsets for variable size list entries */
{ unsigned short Offset[V4L_ListPoint_EntryMax] ;
} ;

struct V4L__ListPoint_Jacket		/* Jacket structure to hold variable len entries (which are not complete points) */
{ unsigned short Bytes ;		/* Number of bytes */
  BYTE Buffer[V4L_ListPoint_BufMax] ;	/* Buffer for value */
} ;

struct V4L__ListCmpndRange
{ unsigned short Bytes ;
  unsigned short Count ;
  COUNTER Entries ;
  struct {
    int Begin,End ;			/* Begin & End numbers in range */
    int Increment ;			/* Increment to use */
   } Cmpnd[100] ;			/* May have multiple ranges */
} ;

struct V4L__ListMultiAppend
{ unsigned short Bytes ;
  unsigned short Count ;
  COUNTER Entries ;
  struct {
    struct V4L__ListPoint *lp ; 	/* Pointer to this list point */
    COUNTER Entries ;			/* Number of points in this list */
   } Multi[100] ;
} ;

struct V4L__ListCmpndRangeDBL
{ unsigned short Bytes ;
  unsigned short Count ;
  COUNTER Entries ;
  struct {
    double Begin,End ;			/* Begin & End numbers in range */
    double Increment ;			/* Increment to use */
   } Cmpnd[100] ;			/* May have multiple ranges */
} ;

struct V4L__ListIsct			/* Format of [isct] list type */
{ int pix ;				/* Handle to allocated point (CvtPIDX) */
} ;

#define V4L_AreaAgg_Max V4MM_Area_Max - 5
#define V4C_AreaAgg_Max V4L_AreaAgg_Max

struct V4L__ListAggRef			/* List of points within one or more aggregates */
{ DIMID Dim ;				/* Dimension of points */
  COUNTER Count ;			/* Number of areas below */
  short AreaAggIndex[V4L_AreaAgg_Max] ;	/* Index into AreaAgg in gpi */
  struct {
    COUNTER BeginPt,EndPt ; 		/* Range of points */
//    int NumPts ;			/* Number of points */ - don't need - can figure out from above
   } Agg[V4L_AreaAgg_Max] ;
} ;

struct V4L__ListMDArray 		/* List of points - subset of multi-dim array */
{ int han_MDArray ;			/* Handle to array header */
  COUNTER MDDimCount ;			/* Number of dimensions (below) in array spec */
  struct
   { int Start,End ;			/* Starting/Ending values (integer & zero adjusted) */
   } MDDim[V4DPI_MDArray_Max] ;
} ;

#ifdef WANTHTMLMODULE
struct V4L__ListHTMLTable
{ struct V4HTML__Page *vhp ;		/* Pointer to HTML page (Must be type V4HTML_PageType_Table) */ 
  COUNTER NumRows ; 			/* If list of rows then number of rows else UNUSED */
  COUNTER NumColumns ;			/* If list of cells/columns then number else UNUSED */
} ;
#endif

struct V4L__ListFiles
{ DIMID Dim ;				/* Dimension to place file names into */
  UCCHAR FileSpec[V_FileName_Max+1] ;	/* Original file specification (with wild cards) */
  struct V4DPI__LittlePoint Updatept ;	/* Holds update date-time range */
  struct V4DPI__LittlePoint Bytespt ;	/* Holds size range */
  struct V4L__ListFilesInfo *lfi ;	/* Pointer to where all the action really is */
} ;

struct V4L__ListTextFile
{ DIMID Dim ;				/* Dimension to return points on */
  ETYPE fileType ;			/* Type of text file - see V4L_TextFileType_xxx */
  COUNTER lineCount ;			/* Number of lines read */
  UCCHAR FileSpec[V_FileName_Max+1] ;/* Source file name */
  struct V4DPI__Point *fltr ;		/* If non-null then filter point */
  struct V4DPI__Point *fltrres ;	/* If filter then this is allocated point & holds result of calling filter */
  INDEX fltrindex ;			/* If filter result is list then this is the index of last point returned (otherwise UNUSED) */
  struct UC__File UCFile ;		/* File access structure */
  UCCHAR *line ;			/* Pointer to current line, if NULL then need new line */
  UCCHAR *lineBuf ;			/* Line buffer */
  LENMAX lineBufSize ;			/* Characters in lineBuf */
  LENMAX lineLen ;				/* Length of ltf->line (remaining) in UCCHAR bytes */
} ;

struct V4L__ListBigText			/* Lists of type V4L_ListType_BigText */
{ UCCHAR *ptrText ;			/* Pointer to begin of CURRENT line in bt->BigBuf */
  UCCHAR *btBuf ;			/* Pointer to text buffer (MUST RETURN TO HEAP WHEN DONE WITH LIST!) */
  COUNTER lineNum ;			/* Current line number (0 if unreferenced) */
} ;

struct V4L__ListJSON			/* Lists of type V4L_ListType_JSON */
{ struct VJSON__Blob *vjblob ;		/* JSON blob pointer */
  struct VJSON__Value vjval ;		/* Referenced value within the blob */
  DICTID objName ;			/* Name of this object (UNUSED if we don't know/have it) */
  LOGICAL dimAsName ;			/* If TRUE then try to use attribute name as dimension (via $$(x.x.x.x)) */
} ;

struct V4L__ListDrawer			/* Lists of type V4L_ListType_Drawer */
{ struct V4IM__Drawer *drw ;	/* Pointer to the drawer */
  INDEX index ;				/* Next index to reference */
  P *dPt ;				/* Pointer to next point to return */
} ;

#define V4_ListFilesType_Directory 0	/* A directory */
#define V4_ListFilesType_RegFile 1	/* A regular file */

struct V4L__ListFilesInfo
{ 
  struct V4L__ListFilesInfo *nestlfi ;	/* Pointer to nested directory if nesting */
  struct V4L__ListFilesInfo *priorlfi ; /* Pointer to prior if nesting directories */
  LOGICAL WantNestDir ;			/* TRUE if nesting directories */
  LOGICAL showHidden ;			/* If TRUE then show hidden files (LINUX) */
  INDEX LogDeviceIndex ;		/* Keeps track of multiple directories in logical device specification */
  UCCHAR FileNamePattern[V_FileName_Max+1] ; /* Original file name pattern */
  COUNTER Calls ;				/* Number of calls to v_NextFile() */
#ifdef VMSOS
  INDEX Index ;
#endif
#ifdef WINNT
  HANDLE Handle ;
#endif
#ifdef UNIX
  glob_t *globt ;
  INDEX Index ;
#endif
  UCCHAR FileName[V_FileName_Max+1] ;
  UCCHAR DirectoryPath[V_FileName_Max+1] ;
  UDTVAL UpdateDT ;
  LENMAX FileBytes ;
  ETYPE FileType ;			/* Type of file, see V4_ListFilesType_xxx */
} ;


struct V4L__ListTextTable		/* List of points described by text file & TABLE */
{ DIMID Dim ;				/* Dimension of list entry points (uses AggRef) */
  INDEX TextAggIndex ;			/* Index into text aggregate in V4C__Context */
  COUNTER Count ;			/* Current line counter */
  COUNTER ActCount ;			/* Actual line count */
  UCCHAR FileName[V_FileName_Max+1] ;
  UCCHAR *line ;			/* Pointer to text input buffer */
  struct UC__File UCFile ;
//  FILE *fp ;				/* Pointer to file input */
  struct V4LEX__Table *vlt ;		/* Pointer to current TABLE */

} ;

#define V4L_ListToken_EndOfLine 0x1	/* Return end-of-line as token */
#define V4L_ListToken_LowerCase 0x2	/* Return all keywords as lower-case */
#define V4L_ListToken_UpperCase 0x4	/* Return all keywords as upper-case */
#define V4L_ListToken_Negative 0x8	/* Treat -nnn has single negative token (not '-' & 'nnn') */
#define V4L_ListToken_Space 0x10	/* Treat white-space as token */

struct V4L__ListToken
{ struct V4LEX__TknCtrlBlk *tcb ;	/* Pointer to tcb */
  struct V4DPI__Point *listPt ;		/* Source list point (NULL if not used) */
  struct V4LEX_ListXMLCtrl *lxc ;	/* If parsing XML then pointer to XML control structure */
  FLAGS32 parseFlags ;			/* Parsing flags (see V4L_ListToken_xxx) */
  INDEX listPtIndex ;			/* Current index into list via listPt */
  LOGICAL lastWasOper ;			/* Used for Java parsing, TRUE if last token operator */
} ;

#define XML_State_BOM 1		/* At begin of XML message */
#define XML_State_Err 2		/* Detected error - cannot leave this state */
#define XML_State_StartLvl 3	/* After '<' - get name & push new level */
#define XML_State_InLvl 4	/* Within '< .... >' - look for end, attribute name, etc. */
#define XML_State_CheckAttVal 5	/* Got attribute name, look for '=' or whatever */
#define XML_State_GetAttVal 6	/* Got '=', look for value */
#define XML_State_AttrValue 7	/* Returning prior attribute name's value */
#define XML_State_LookForLitVal 8 /* After '<xxxx ...>' - look for literal value */
#define XML_State_LitVal 9	/* Returning literal value */
#define XML_State_StartEndLvl 10 /* Got '</' - parse level name */
#define XML_State_EndLvl 11	/* In '</' and name in CurValue, make sure matches Lvl[] & pop off */
#define XML_State_EOM 12	/* All done */
#define XML_State_InHdr 13	/* Got '<?' - parsing header */
#define XML_State_InCom 14	/* Got '<!' - parse remaining comment until '>' */
#define XML_State_InCDATA 15	/* Got '<![CDATA[' - parse remaining text until ']]>' */
#define XML_State_InCom1 16	/* Got '<!--' - parse remaining comment until '-->' */

#define XML_Error_InvName 1	/* Invalid '<xxx' name */
#define XML_Error_TooMnyLvl 2	/* Too many nested '<xxx' levels */
#define XML_Error_Syntax 3	/* Invalid XML syntax */
#define XML_Error_NameMatch 4	/* Name in '<xxx' does not match that in '</yyy' */
#define XML_Error_List 5	/* Error creating (XMLAttr:xxx value) list */

#define V4LEX_XML_NamxMax 31		/* Max characters in level name */
#define V4LEX_XML_LvlMax 50		/* Max number of nexted levels */

#define V4LEX_XML_LvlMax 50
struct V4LEX_ListXMLCtrl
{ INDEX nestLvl ;			/* Nexting level */
  LOGICAL includeSchema ;		/* If TRUE then include Schema info, if FALSE then ignore it */
  LOGICAL includeAttr ;			/* If TRUE then include Attribute info, if FALSE then ignore */
  ETYPE xmlState ;			/* Current state (see XML_State_xxx) */
  UCCHAR curKeyword[V4LEX_XML_NamxMax+1] ; /* Temp save of '<xxx keyword=value...' */
  struct {
    UCCHAR lvlName[V4LEX_XML_NamxMax+1] ;
   } Lvl[V4LEX_XML_LvlMax] ;
} ;


/*	XML Nesting Structure		*/

#define XMLSegMax 15
struct V4IM__XMLNest {
  UCCHAR *b ;
  INDEX Count ;				/* Number of xml segments defined */
  struct {
    struct V4DPI__Point *eolPt ;	/* If not NULL then end-of-level processing point */
    UCCHAR seg[64] ;			/* XML entry name */
    COUNTER indent ;			/* Indentation */
    LOGICAL UseNL ;			/* Use new line (i.e. output on new line) */
   } Nest[XMLSegMax] ;
} ;



struct V4L__ListXDBGet 		/* List for getting XDB records */
{ XDBID xdbId ;				/* Unique connection(-)/statement(+) for this point */
} ;

struct V4L__ListV4IS
{ int Handle ;				/* Handle to the V4IS area */
  INDEX SubIndex ;			/* Current substructure index */
  COUNTER KeyNum ;			/* Which key to use to sequence through (0 = DataOnly = Default) */
  LENMAX CountOffset ;			/* Offset (bytes) into record for substructure count */
  COUNTER CountBytes ;			/* Number of bytes in count (s/b 2 or 4) */
  int Sample ;				/* If not UNUSED then sample, on average, every nth */
  int Number ;				/* If not UNUSED then only take first n */
  int Firstpix ;			/* If not UNUSED then evaluate to obtain first record */
  int Ifpix ;				/* Handle to allocated point to test record */
  int Whilepix ;			/* Handle to allocated point to test record */
  int Subpix ;				/* Handle (or UNUSED) to allocated point to test substructure */
} ;

struct V4L__ListLazy			/* Holds links to lazy list info */
{ struct V4L__ListLazyInfo *lli ;	/* All info in separate in-core  structure (lazy lists can't be "saved") */
} ;


#define V4L_LLInfo_areaMAX 64		/* Max number of areas per enumerating list entry */
#define V4L_LLInfo_actMAX 32		/* Max number of actions per list entry */

#define V4IM_Tag_MaxValuelocal 635

struct V4L__ListLazyInfo		/* Lazy List - version 2 */
{ 
  struct V4L__ListLazyInfo *lliCur ;	/* Pointer to "current" lli (with lazy list, V4 list pointer always points to first lli - need to link to current) */
  struct V4L__ListLazyInfo *lliPrior ;	/* Pointer to prior entry if we got multiple (NULL = begin of chain) */
  struct V4L__ListLazyInfo *lliNext ;	/* Pointer to next entry if we got multiple (NULL = end of chain) */

  BYTE gotTag[V4IM_Tag_MaxValuelocal] ;	/* Contents holds actionindex+1 for each last-defined actions */

  INDEX startIndex ;			/* Starting (next) index for enumerating point (used when Lazy list) */
  LOGICAL needInitEnumList ;		/* If TRUE then need to init enumerating list (used when Lazy list) */
  P elptbuf, *elpt ;			/* Enumerating list point */
					/* ** NOTE: elpt must be "saved" for lazy list ** */
  LOGICAL gotValue ;			/* TRUE if we have at least one value point (if FALSE then use enumerating point as list values) */
  struct V4L__ListPoint *lp ;		/* List pointer */
  P *ignpt ;				/* Point to ignore (i.e. if resulting point = this then don't append to list) */
					/*  (this pointer is set when looping through actions so multiple ignores are possible) */
  COUNTER resNum ;			/* Number of resulting points (i.e. number appended to result list) */
  INDEX lastNum ;			/* Index of last point in enumerating list (so we can get Last::xxx) */
  LOGICAL needAggLoad ;			/* If TRUE then need to load aggs below, set to FALSE after loading */
  LOGICAL aggNoError ;			/* If TRUE then don't abort on Agg load error - just ignore */
  INDEX aggCount ;			/* Number of aggregates associated with this entry */
  struct {
    UCCHAR *areaName ;			/* Points to area name (to be opened on demand) (pointer allocated when Agg::xxx tag parsed) */
    int AggUId ;			/* AreaId (so we can properly close it) */
   } Area[V4L_LLInfo_areaMAX] ;
  DIMID DimId ;				/* Dimension of resulting list (default: Dim:List) */
  DIMID shellDim ;			/* DimId of Shell dimension (if not UNUSED) */
  LOGICAL evalResVal ;			/* If TRUE then evaluate result point as long as it is Isct */
  LOGICAL popFrame ;			/* If TRUE then pop frame stack with each iteration */
  LOGICAL setOf ;			/* If TRUE then return "set of points" (no duplicates) instead of list */
  P oldparptbuf ;			/* Value of "prior" parent point */
  LOGICAL oldparptPIfres ;		/* Result (True/False) of prior parent point PIf */
  INDEX actCount ;			/* Action count */
  struct {
    ETYPE Tag ;				/* V4IM_Tag_xxx of this action */
    COUNTER Counter ;			/* Counter (used by First::xxx, and Num:xxx) */
    P *pt ;				/* Holds tag value point */
					/* ** NOTE: initially points to argument, must be "saved" for lazy list!! ** */
    } Act[V4L_LLInfo_actMAX] ;
} ;


#define V4L_LBM_AllocTemp 0		/* Bit map is temporarily allocated - may dissapear any time */

struct V4L__ListBitMap			/* List is a bitmap */
{ COUNTER Entries ; 			/* Number of entries in the bitmap */
  INDEX LastIndex ;			/* Last index referenced via v4l_ListPoint_Value */
  INDEX LastBit ; 			/*  & corresponding last bit */
  int AllocHandle ;			/* Handle referencing the bitmap (not yet used) */
  ETYPE AllocMode ;			/* Allocation mode (see V4L_LBM_Allocxxx) */
  struct V4L__ListBMData1 *bm1 ;	/* Pointer to the actual bitmap */
} ;

struct V4L__ListBMData1 		/* Actual bitmap (represented as string of 32-bit int's */
{ LENMAX MaxBit ;			/* Highest bit allowed (1 is assumed lowest) */
  int Seg32[1] ;			/* List of 32 bit segments (actual length varies on MaxBit) */
} ;

#define BM1_Dcl(bm1,maxbits) (bm1)->MaxBit = maxbits ;
#define BM1_CalcMaxBit(bits) ((bits+0x1f) &  0xffffffe0)	/* Rounds bits to next higher 32 */
#define BM1_StructBytes(bits) ((sizeof (int)) + ((bits+0x7)>>3)) /* Number bytes in BM1 structure (bits must be MaxBits!) */
#define BM1_MapBytes(bits) ((bits+0x7)>>3)			/* Number of bytes in BM1 bitmap */
#define BM1_MapWord32(bits) ((bits+0x1f)>>5)			/* Number of 32-bit words in BM1 bitmap */
/*	Note: bit (below) starts at 1, not 0 */
#define BM1_IsBitSet(bm1,bit) (((bm1)->Seg32[((bit)-1)>>5] >> ((((bit) - 1) & 0x1f))) & 1)
#define BM1_SetBit(bm1,bit) ((bm1)->Seg32[((bit)-1)>>5] |= (1 << (((bit) - 1) & 0x1f))) /* Set bit in map */
#define BM1_NumBitsSet(bm1,count) \
 { INDEX _i,_j ; \
   for(count=0,_i=0;_i<(((bm1)->MaxBit+0x1f)>>5);_i++) \
    { if ((bm1)->Seg32[_i] == 0) continue ; \
      for(_j=0,k=(bm1)->Seg32[_i];k!=0&&_j<32;_j++) { if ((k & 1) != 0) count++ ; k = k >> 1 ; } ; \
    } ; \
 }

/*	B I T M A P   F O R   6 4  B I T   A R C H I T E C T U R E	*/
struct V4L__ListBMData2 		/* Actual bitmap (represented as string of 64-bit int's */
{ B64INT MaxBit ;			/* Highest bit allowed (1 is assumed lowest) */
  B64INT Seg64[1] ;			/* List of 64 bit segments (actual length varies on MaxBit) */
} ;

#define BM2_CalcMaxBit(bits) ((bits+0x3f) &  0xffffff80)	/* Rounds bits to next higher 64 */
#define BM2_StructBytes(bits) ((sizeof (int)) + (bits>>3))	/* Number bytes in BM2 structure (bits must be MaxBits!) */
#define BM2_MapBytes(bits) (bits>>3)				/* Number of bytes in BM2 bitmap */
#define BM2_MapWord64(bits) (bits>>6)				/* Number of 64-bit words in BM2 bitmap */
#define BM2_IsBitSet(BM2,bit) ((BM2->Seg64[bit>>6] >> (bit & 0x3f)) & 1)
#define BM2_SetBit(BM2,bit) (BM2->Seg64[(bit-1)>>6] |= (1 << ((bit-1) & 0x3f))) /* Set bit in map */
#define BM2_NumBitsSet(BM2,count) \
 { INDEX i,j ; \
   for(count=0,i=0;i<BM2->MaxBit>>6;i++) \
    { if (BM2->Seg64[i] == 0) continue ; \
      for(j=0,k=BM2->Seg64[i];k!=0&&j<64;j++) { if ((k & 1) != 0) count++ ; k = k >> 1 ; } ; \
    } ; \
 }

//#define BYTESinBM1(bm1ptr) ( (sizeof (int)) + bm1ptr->MaxBit / 8) /* Number of bytes in a BM1 bitmap */

/*	NOTE: This structure must fit within a V4L_ListPoint (which must fit within V4DPI__Point! */
struct V4L__ListHugeInt 		/* "Directory" for huge list */
{ unsigned short Bytes ;		/* Number of bytes (fixed at sizeof V4L__ListHuge) */
  unsigned short Blocks ;		/* Number of blocks defined below */
  unsigned short RelHNum ;		/* RelHNum of this list (so we can get aid for Block access) */
  LENMAX TotalElCount ;			/* Total number of list elements */
  struct {
    BITS ElCount : 12 ;			/* Number of list entries in this block */
    BITS BlockValueNum : 20 ;		/* Value number - used to form key to get V4L__BlockEntry */
   } Block[V4L_ListHuge_BlockMax] ;
} ;

struct V4L__BlockEntry			/* Data entry for huge lists (numeric) */
{ union V4IS__KeyPrefix kp ;		/* Type=V4, SubType=Value, Mode=Int, Len=8 */
  int ValueNum ;			/* Unique ValueNum for this entry (from RootInfo->NextAvailValueNum) */
  INDEX EntryCount ;			/* Number of points below */
#pragma pack(8)				/* NOTE: MUST BE DOUBLE ALIGNED */
  int PointValBuf[V4L_BlockEntry_IntMax] ;/* Data/point buffer */
#pragma pack()
} ;

struct V4L__ListIntArray		/* List of integers */
{ int PointIntVal[V4L_BlockEntry_IntMax] ;/* Data/point buffer */
} ;

struct V4L__ListDblArray		/* List of integers */
{ double PointDblVal[V4L_BlockEntry_DblMax] ;/* Data/point buffer */
} ;

/*	NOTE: This structure must fit within a V4L_ListPoint (which must fit within V4DPI__Point! */
struct V4L__ListHugeGeneric		/* "Directory" for huge list */
{ unsigned short Bytes ;		/* Number of bytes (fixed at sizeof V4L__ListHuge) */
  unsigned short Blocks ;		/* Number of blocks defined below */
  unsigned short RelHNum ;		/* RelHNum of this list (so we can get aid for Block access) */
  INDEX TotalElCount ;			/* Total number of list elements */
  struct {
    BITS ElCount : 12 ;			/* Number of list entries in this block */
    BITS BlockValueNum : 20 ;		/* Value number - used to form key to get V4L__BlockEntryGeneric */
   } Block[V4L_ListHuge_BlockMax] ;
} ;

struct V4L__BlockEntryGeneric		/* Data entry for huge lists */
{ union V4IS__KeyPrefix kp ;		/* Type=V4, SubType=Value, Mode=Int, Len=8 */
  int ValueNum ;			/* Unique ValueNum for this entry (from RootInfo->NextAvailValueNum) */
  INDEX EntryCount ;			/* Number of points below */
  BYTE PointValBuf[V4L_BlockEntryGeneric_Max] ;/* Data/point buffer */
} ;

struct V4DPI__DUList			/* Tracks all points in a dimension */
{ union V4IS__KeyPrefix kp ;		/* Type=V4, SubType=DUList, Mode=Int, Length=8 */
  DIMID DimId ;
  int Revision ;			/* Holds revision number if points saved in multiple areas (highest rev wins) */
#pragma pack(8)
  struct V4L__ListPoint lp ;		/* List to holds points on dimension */
#pragma pack()
} ;

#define ALIGNLP(PT) (struct V4L__ListPoint *)PT

#define INITLP(RESPT,LP,DIMLIST)	/* Init lp for new list (RESPT is point holding list, LP is lp, DIMLIST is Dim:List) */ \
   LP = ALIGNLP(&(RESPT)->Value) ; memset(LP,0,V4L_ListPointHdr_Bytes) ; LP->ListType = V4L_ListType_Point ; \
   ZPH(RESPT) ; (RESPT)->Dim = DIMLIST ; (RESPT)->PntType = V4DPI_PntType_List ;

#define ENDLP(RESPT,LP) 	/* End a list via LP & RESPT */ \
   (RESPT)->Bytes = ALIGN((BYTE *)&(RESPT)->Value.AlphaVal[LP->Bytes] - (BYTE *)(RESPT)) ; \
   if ((RESPT)->Bytes < V4DPI_PointHdr_Bytes + (&LP->Buffer[0] - (BYTE *)LP)) \
    (RESPT)->Bytes = V4DPI_PointHdr_Bytes + (&LP->Buffer[0] - (BYTE *)LP) ;

#define V4IM_SetPointMax 1000003	/* Max number of points we can handle (s/b/ prime) */
#define V4IM_SetPointMapMax (V4IM_SetPointMax+31)/(sizeof(int)*8)
#define V4IM_SetPointListMax 20 	/* Max number of lists we can handle */
#define V4DPI_IntModArg_Max 256 	/* Max number of IntMod arguments */

struct V4IM__SetPoint {
  COUNTER Count ;			/* Number of points we have seen */
  int Hash[V4IM_SetPointMax] ;		/* Hash values for fast lookup */
  struct V4DPI__Point *pt[V4IM_SetPointMax] ;	/* Pointer to corresponding point */
  struct V4IM__SetPointBuf *vspb ;	/* Pointer to current point buffer */
  INDEX ListCount ;			/* Number of lists added to hash table */
  struct {
    DIMID Dim ;				/* Dimension of this list */
    int Map[V4IM_SetPointMapMax] ;
   } List[V4IM_SetPointListMax] ;	/* Don't need separate entry for each list right now (2 would do) - but may be usefull later */
} ;

#define V4IM_SetPointBufSize 0x10000	/* Number of bytes to allocate for buffer below */

struct V4IM__SetPointBuf {		/* Structure to hold big chunks of memory (as needed) */
  struct V4IM__SetPointBuf *Prior ;	/* Pointer to prior, NULL if none */
  INDEX NextFree ;			/* Index to next free byte below */
  BYTE Buf[V4IM_SetPointBufSize] ;
} ;

/*	D E F E R R E D   D O   S T R U C T U R E		*/

struct V4IM__DeferDoSteps
{ COUNTER argcnt ;			/* Number of arguments below */
  struct V4DPI__Point *argpnts[V4DPI_IntModArg_Max] ;
  struct V4IM__XMLNest xml ;		/* Also want to save a nested xml states */
  COUNTER ctxcnt ;			/* Number of context points */
  struct V4DPI__Point *ctxPts ;		/* Pointer to list of context points at time of defer */
} ;

/*	A G G R E G A T E   S T U F F   */

//VEH020315 Doubled V4IM_AggShell to allow for extensions & compression to whack back to record max
#define V4IM_AggShell_BufMax (V4AreaAgg_UsableBytes)*2
#define V4IM_AggValMax 1000
#define V4IM_AggIndex_Undef 1		/* Index value (in Indexes[n]) denoting undefineed value */

#define V4IM_AggHdrType_Value -1	/* Via FAggregate */
#define V4IM_AggHdrType_BValue -2	/* Via BAggregate */

#define DFLTAGGAREA 0			/* Write aggregate out to default (first open for update) */

struct V4IM__AggShell			/* Outer shell for an aggregate */
{ struct V4IS__Key Key ;		/* Key to the aggregate */
  char Buffer[V4IM_AggShell_BufMax] ;	/* Buffer for remain of shell */
} ;

struct V4IM__AggHdr			/* Header for the aggregate */
					/* NOTE:  First 2 elements must match that of AggVData ********** */
{ LENMAX Bytes ;			/* Number of bytes (including shell overhead) */
  short Count ;				/* Number of elements (if negative then see V4IM_AggHdrType_xxx) */
  unsigned short Indexes[V4IM_AggValMax] ; /* Index into data for each element (first offset always 0) */
} ;

struct V4IM__AggVHdr			/* Header for Value Mapping (when separate from data) */
{ union V4IS__KeyPrefix kp ;		/* Key to the aggregate */
  DIMID Dim ;				/* The dimension owning this record */
  LENMAX Bytes ;			/* Number of bytes in this structure */
  ETYPE Type ;				/* Type of VHdr - V4IM_AddHdrType_xxx */
  LENMAX BytesPerEntry ;		/* Number of data bytes per "record" */
  LENMAX EntriesPerBuffer ;		/* If nonzero then number of record entries per buffer (see below for types) */
  INDEX Count ;				/* Number of elements */
  INDEX Indexes[V4IM_AggValMax] ;	/* Index into data for each element (first offset always 0) */
} ;

struct V4IM__AggVData			/* Data buffer if we got a VHdr */
					/* NOTE: First 2 elements must match that of AggHdr ************* */
{ 
  LENMAX Bytes ;			/* Number of bytes VEH020320 - changed from "unsigned short" - need to rebuild all v4a's */
  short Count ;				/* = V4IM_AggHdrType_Value or V4IM_AggHdrType_BValue */
  char Buffer[V4IM_AggShell_BufMax] ;	/* Buffer for remain of shell */
} ;

struct V4IM__AggData			/* Buffer to hold data elements */
					/* NOTE ***** if anything added before buffer then better fix REF001 */
{ char Buffer[V4IM_AggShell_BufMax] ;
} ;

struct V4IM__AggBDataI			/* Buffer to hold multiple entries with Int keys */
{ B64INT biKey ;			/* big-integer key */
  char Buffer[V4IM_AggShell_BufMax] ;
} ;


/*	Structure for Multi-Threaded Aggregate Buffer Look-Ahead & Load */
/*	Thread1 = main consuming thread, Thread2 = sub thread that performs look-ahead & prefetch */

#define V4IM_AggLA_Max 100		/* Max of 100 look-ahead buffers per dimension */

#define V4IM_ALHStatus_Init 0		/* Thread is in intial state */
#define V4IM_ALHStatus_Wait 1		/* Thread is waiting for other thread to either consume or grab buffers */
#define V4IM_ALHStatus_Working 2	/* Thread is working on either consuming or grabbing buffers */
#define V4IM_ALHStatus_Done 3		/* Set when thread2 is done */
#define V4IM_ALHStatus_Err 4		/* Thread encountered error, see ala->ErrorMsg */

#define V4IM_ALHFlags_Deallocate 1	/* Deallocate ala structure on Cleanup() */

struct V4IM__AggLookAhead		/* Look-ahead structure for multi-threaded aggregate pre-loading */
{
  struct V4C__Context *ctx ;
  COUNTER BufCount ;			/* Number of look-ahead buffers */
  int MinForWakeup ;			/* If pre-fetch goes below this value then wakeup producer */
  FLAGS32 Flags ;			/* See V4IM_ALHFlags_xxx */
  INDEX Index1 ;			/* Count to current buffer (index = Index1 % BufCount) */
  INDEX Index2 ;			/* Count to look-ahead buffer (again, index = Index2 % BufCount) */
  DCLSPINLOCK Interlock ;		/* Common interlock word (Status1/Status2 cannot be altered without grabbing Interlock first) */
  int Status1 ;				/* Main thread status (thread consuming buffers) */
  int Status2 ;				/* Sub thread status (thread performing buffer look-ahead filling) */
  COUNTER Wakeups1 ;			/* Number of consumer wakeups required */
  COUNTER Wakeups2 ;			/*  and producer */
#ifdef WINNT
  HANDLE synchEvent1,synchEvent2 ;	/* Handle of synch event between threads */
  HANDLE hThread ;
#endif
  struct V4L__ListAggRef *lar ;		/* Pointer to current list-agg-reference */
  INDEX larIndex ;			/* Current agg index within lar (so Thread2 can go to next one when hit end of current) */
  int biCurVal ;			/* Current agg key value (increment by 1 to get next agg buffer) */
  struct V4IM__AggShell *aggs[V4IM_AggLA_Max] ;
  UCCHAR ErrorMsg[256] ;			/* An error message */
} ;

/*	X M L   ( R P C )   S T U F F	*/

#define VXMLTkn_Value 1		/* Returning a literal value */
#define VXMLTkn_EOM 0		/* End-of-Message */
#define VXMLTkn_Equal 4		/* Got '=' */
#define VXMLTkn_NewLvl 5	/* Got an new-level '<' */
#define VXMLTkn_EndLvl 6	/* Got an end-level '</' */
#define VXMLTkn_XMLHdrStart 7	/* Got <?xml */
#define VXMLTkn_EndAngle 8	/* Got '>' */
#define VXMLTkn_EndNewLvl 10	/* Got '/>' */
#define VXMLTkn_XMLHdrEnd 11	/* Got '?>' */
#define VXMLTkn_Keyword 12	/* Got a keyword/attribute */

#define VXMLTkn_PremEOM -2	/* Premature End-of-Message */
#define VXMLTkn_TooBig -3	/* Error - name too big */
#define VXMLTkn_Unexpected -9	/* Something unexpected ? */

#define VXMLFlag_LitVal 1	/* Parse literal value */

#define XML_MsgCurValMax 4096
#define XML_MsgLvlMax 100
struct VXML__Message {
  UCCHAR *Begin ;		/* Pointer to begin of message */
  UCCHAR *Cur ;			/* Current position in message */
  INDEX Lvlx ;			/* Current nesting level */
  int State ;			/* Current internal state */
  UCCHAR *ErrCode ;		/* Last error mnemonic string */
  UCCHAR Value[XML_MsgCurValMax] ;
  struct {
    UCCHAR Name[128] ;		/* Name of this level */
   } Lvl[XML_MsgLvlMax] ;
} ;

#define XML_ValType_Int 1
#define XML_ValType_Bool 2
#define XML_ValType_String 3
#define XML_ValType_UDT 4
#define XML_ValType_Dbl 5
#define XML_ValType_Base64 6
#define XML_ValType_Struct 7
#define XML_ValType_Array 8

/*	G R A P H   S T U F F								*/

#define V4GRAPH_DepthMax 500
#define V4GRAPH_EdgeMax 1000		/* Max number of edges (initial max - may be expanded) */

#define V4GRAPH_ConnectRes_Nodes 1	/* Return list of nodes (default) */
#define V4GRAPH_ConnectRes_Maximum 2	/* Return path which maximizes weights */
#define V4GRAPH_ConnectRes_Minimum 3	/* Return path which minimizes weights */

struct V4GRAPH__Connect {		/* Defines structure for connecting 2 points on graph */
  int Success ;
  COUNTER Entries ;
  struct V4L__ListPoint *lpr ;		/* Pointer to list result */
  int StartNode ;			/* Starting node id */
  int Target ;				/* Ending (target) node id */
  LENMAX DepthMax ;			/* Max number of paths allowed (cannot exceed V4GRAPH_DepthMax) */
  LENMAX ResultMax ;			/* Max number of results to return */
  INDEX NodeCnt ;			/* Number of paths below */
  int Nodes[V4GRAPH_DepthMax+1] ;	/* Nodes seen so far */
  INDEX WeightCnt ;			/* Number of linked weight structures below */
  struct V4GRAPH__Weights *vgw ;	/* If not NULL then pointer to weights below (edge wgt defaults to 1 if NULL) */
  INDEX EdgeMax ;
  INDEX EdgeCnt ;			/* Number of edges below */
  struct {				/* Actual number is dynamic */
    int NodeA ;
    int NodeB ;
   } Edge[V4GRAPH_EdgeMax] ;
} ;

struct V4GRAPH__Weights {		/* Holds weights of edges */
  struct V4DPI__Point *wpt ;		/* Point to be eval'ed for weight of an edge */
  struct V4GRAPH__Weights *nvgw ;	/* If not null then pointer to next one (when multiple weights) */
  double CurMinMaxVal ;			/* Current Min/Max value so far */
  ETYPE ResultType ;			/* Result type - V4GRAPH_ConnectRes_xxx */
  LENMAX MinMaxCnt ;			/* Number of nodes in current min/max path */
  LENMAX MinMaxNodes[V4GRAPH_DepthMax+1] ;	/* Current Min/Max path */
  int ConstantWeight ;			/* If TRUE then all edges have constant weight (don't use Weight[] below) */
  double Nodes[V4GRAPH_DepthMax+1] ;	/* Weight for each nested node in search (matches Nodes[] above) */
  double Weight[V4GRAPH_EdgeMax] ;	/* Actual number is dynamic & tied with EdgeCnt above */
} ;


/*	L I S T   B I T M A P   S T U F F  */

struct V4IM__ListBitMap {		/* Format of cached list bitmap */
  DIMID Dim ;				/* Dimension associated with bitmap */
  PNTTYPE PntType ;			/*  and point type */
  int BitWCount ;			/* Number of active words in the map (map must be increments of 32) */
  LENMAX MaxBits ;			/* Max number of bits possible */
  COUNTER SetCount ;			/* Indicates number of bits, UNUSED = unknown, 0 = none, 1 = at least one bit set */
  int *Bits ;				/* Pointer to bitmap */
} ;


#define V4IM_ListCache_EntryMax 3009	/* Max number of cached points/lists below */
#define V4IM_ListCache_WorkMax 10	/* Working bitmaps (hold temp results) */
#define V4IM_ListCacheAct_Init 1	/* First list is series of action */
#define V4IM_ListCacheAct_Union 2	/* Union this with working */
#define V4IM_ListCacheAct_Intersect 3	/* Intersection */
#define V4IM_ListCacheAction_Subtract 4	/* Substract */
#define V4IM_ListCacheAction_XUnion 5	/* Exclusive union */

struct V4IM__ListCache {
  COUNTER StatBuildMap ;		/* Calls to BuildMap */
  COUNTER StatBuildMapLoad ;		/*  number of times had to load */
  COUNTER StatNullIntersect ;		/* Number of hits on intersect of empty list */
  COUNTER StatNullTest ;			/* Number of times checked for empty bitmap */
  INDEX Count ;				/* Number of cached lists below */
  struct V4IM__ListBitMap *wlbm[V4IM_ListCache_WorkMax] ; /* Working bitmaps */
  struct {
    int PtHash ;			/* Hash value for cached intersection */
    struct V4DPI__Point *Pt ;		/* Isct evaluating to list of points reflected by this bitmap */
    struct V4IM__ListBitMap *lbm ;
   } Entry[V4IM_ListCache_EntryMax] ;
} ;


/*	T A L L Y   S T R U C T U R E S		*/

#ifdef V4ENABLEMULTITHREADS
/*	GRABMTLOCK - Try and lock portion of Tally table. Make quick attempt and if fails then call routine */
/*	FREEMTLOCK is same as RLSSPINLOCK except is defined as nop when V4ENABLEMULTITHREADS is undefined */
#define GRABMTLOCK(flag) (GETSPINLOCK(&flag) ? TRUE : v4im_MTGrabLock(ctx,&flag))
#define GRABMTLOCKnoCTX(flag) (GETSPINLOCK(&flag) ? TRUE : v4im_MTGrabLock(gpi->ctx,&flag))
#define FREEMTLOCK(flag) RLSSPINLOCK(flag)
#define INITMTLOCK(flag) RLSSPINLOCK(flag)
#else
#define GRABMTLOCK(flag) TRUE		/* Always 'return TRUE' so we think all is well */
#define GRABMTLOCKnoCTX(flag) TRUE
#define FREEMTLOCK(flag)		/* Do nothing if not MT enabled */
#define INITMTLOCK(flag)
#endif


#define V4IM_TallyHash_Max 250007	/* Max number of tally combinations (SHOULD BE PRIME) */
#define V4IM_Tally_HashHitInterval 0xfffff	/* Output warning on every "x" number of hash conflicts */
#define V4IM_TallyAction_Max 50		/* Max number of actions in a single Tally() */
#define V4IM_TallyEntryValue_Max 15	/* Max number of different values to tally at once */
#define V4IM_TallyEntryBuf_Max 5000	/* Max bytes for combo points */
#define V4IM_TallyListBitMapMax 100000	/* Max number of bits for SetOf:: bitmaps */

#define V4IM_ActType_Sum 0		/* Sum values (default) */
#define V4IM_ActType_List 1		/* Make a list */
#define V4IM_ActType_Avg 2
#define V4IM_ActType_Count 3
#define V4IM_ActType_StdDev 4
#define V4IM_ActType_Sub 5		/* Same as Sum except negates result */
#define V4IM_ActType_Min 6		/* Pick minimum */
#define V4IM_ActType_Max 7		/* Pick maximum */
#define V4IM_ActType_UList 8		/* Make list of unique points (point only in list once!) */
#define V4IM_ActType_UCount 9		/* Count of unique points (same as ListSize(unique-list)) */


struct V4IM_TallyEnumEnvironment
{ LOGICAL finishOK ;			/* Set to TRUE if thread finishes OK, FALSE if error */
  struct V4C__Context *ctx ;		/* Local context to be allocated to this partition */
  struct V4MM__MemMgmtMaster *mmm ;	/* Pointer to sub-thread mmm */
  struct V4IM__TallyMaster *vtm ;	/* Master Tally() info structure */
  struct V4L__ListPoint *lp ;		/* List pointer to enumerate thru */
  struct V4DPI__Point *isct ;		/* Pointer to original Tally() point that is invoking all of this */
  INTMODX intmodx ;			/* Calling module index */
  VTRACE trace ;			/* Trace flag */
  INDEX startX,endX ;			/* Starting & Ending indexes within above list */
  LENMAX skip ;				/* If not UNUSED then number of initial points to skip */
  COUNTER traceInc ;			/* Trace output increment/interval */
  COUNTER totalEnumPts ;		/* Total number of points to enumerate through */
  COUNTER totalScan ;			/* Total number of points scanned */
  COUNTER SelectCount ;			/* Total points selected (added into vtm->SelectCount) */
  COUNTER HashHits ;			/* Number of hash conflict hits (for performance measuring) */
  LOGICAL IfResults[3] ;		/* Results (TRUE/FALSE) of IFa, IFb, IFc */
  LOGICAL showProgress ;		/* If true then trace progress of Tally() */
} ;


struct V4IM__TallyMaster
{ struct V4IM__TallyBuf *CurBuf ;	/* Pointer to current entry buffer */
  INDEX TallyEntryMax ;			/* Number of pointers below */
  struct V4IM__TallyEntryBuf *teb ;
  struct V4DPI__Point *GlobalSelect ;	/* If nonNULL then eval to get global selection criteria */
  struct V4DPI__Point *GlobalWhile ;	/* If nonNULL then eval to get global While criteria */
  struct V4DPI__Point *GlobalHold ;	/* If nonNULL then eval to get global hold-context point */
  struct V4DPI__Point *GlobalContext ;	/* If nonNULL then eval to get global context point */
  struct V4DPI__Point *GlobalContextP ;	/* If nonNULL then eval to get global context point */
  struct V4DPI__Point *pifpt,*parpt ;	/* PIf:: & Parent:: points (if not NULL) */
  struct V4DPI__Point *sumpt ;		/* Sum:: point (if not NULL) */
  struct V4L__ListPoint *lpsum ;	/* Sum:: lp if it is a list (NULL otherwise) */
  struct V4DPI__Point *dopt ;		/* Do:: point (if not NULL) */
  struct V4L__ListPoint *lpdo ;		/* Do:: lp if it is a list (NULL otherwise) */
  struct V4DPI__Point *awpt ;		/* AWhile:: point (if not NULL) */
  struct V4IM__TallyEntryWith *vtew ;	/* With:: & WBind:: info */
  DIMID indexDimId ;			/* If not UNUSED then DIMID for global index number */
  INDEX indexStart ;			/*  set to index starting number (default 1) */
  PNTTYPE indexPntType ;		/*  set to index point type */
  DIMID resDim ;			/* Dimension to use when inserting enum point into context (if <> 0) */
  LOGICAL wantCount ;			/* If TRUE then use cntpt for inserting count into context */
  struct V4DPI__LittlePoint cntpt ;	/* If wantCount set then use this as base point */
  struct UC__File UCFile ; 		/* If UCFile.fp nonNULL then write results to this area, not make bindings */
  COUNTER EntryCount ;			/* Number of entries used above */
  COUNTER SelectCount ;
  COUNTER totalScan ;			/* Total number of points scanned */
  INDEX ActionCount ;			/* Number of semi-independant actions going on */
  DIMID GlobalResDim ;			/* Dimension for resulting points */
  COUNTER Sample ;			/* Sample approximately every n points */
  COUNTER maxNum ;			/* If > 0 then only look at first maxNum points in list */
  int LastOrder ;			/* Last Order value for V4IM__TallySort.Order */
#ifdef V4ENABLEMULTITHREADS
    DCLSPINLOCK Threads ;		/* Number of threads to run (0=auto calculate) */
    DCLSPINLOCK sfpLock ;		/* Interlock to prevent multi-threads writing to sfp (below) at same time */
    DCLSPINLOCK tebLock ;		/* Interlock to prevent multi-threads from creating new vte entry at same time */
    DCLSPINLOCK traceLock ;		/* Interlock to prevent MTs from interweaving output traces */
#endif
  struct {
    struct V4DPI__Point *p1,*p2,*p3,*p4,*p5, *p6 ; /* Points in action (NULL if not defined) */
    struct V4DPI__Point *With,*WBind ;	/* Points for With:: & WBind:: */
    struct V4DPI__Point *ByIf ;		/* ByIf test point */
    struct V4DPI__Point *BindIf ;	/* BindIf test point */
    struct V4DPI__Point *Do ;
    struct V4DPI__Point *Project ;	/* Point for final projection(s) */
    struct V4DPI__Point *Array ;	/* Array::xxx point value */
    struct V4L__ListPoint *vallp ;	/* Pointer to value (if a list, NULL otherwise) */
    struct V4L__ListPoint *Withlp ;	/* Pointer to value (if a list, NULL otherwise) */
    struct V4IM__TallySortCache *tsc ;	/* If not NULL then points to sorting buffer/cache for lists */
    double Cache ;			/* Cache indicator */
    LENMAX bitsInMap ;			/* Number of bits in UCount/SetOf bitmap */
    DIMID listDimId ;			/* If not 0 then dimension of points on resulting list (i.e. result of this action is going to be bitmap list) */
    ETYPE IFabc ;			/* If nonzero then +1..3 if setting value, -1..-3 if checking IFa/b/c */
    COUNTER biMin,biMax ;		/* If not UNUSED then BindIf Min/Max values for number of elements in ListOf::xxx */
    INDEX NumWith ;			/* Number of With values */
    ETYPE ActionType ;			/* Type of action: V4IM_ActType_xxx */
    INDEX NumValues ;			/* Number of values to track in each entry (1 to V4IM_TallyEntryValue_Max) */
    INDEX NumComboPoints ;		/* Number of points for each entry combination */
    int DfltHash ;			/* Default hash value if NumComboPoint is 0 (i.e. no By::point) */
    LOGICAL SelectOnce ;		/* If have Select for this Action & a match then don't do following actions */
    DIMID ResDim ;			/* Resulting dim for this action */
    struct V4L__ListPoint *comlp ;	/* Pointer to combos (if a list, NULL otherwise) */
    UCCHAR FileName[V_FileName_Max] ;		/* File name for temp sort file */
    FILE *sfp ;				/* Pointer to temp sort file (for lists) */
   } Action[V4IM_TallyAction_Max] ;
} ;

struct V4IM__TallyEntryBuf		/* Holds pointers to TallyEntry's (size may be increased from default) */
{					/* NOTE: this structure should only hold *Entry's (or have to change allocation in v4imu) */
  struct V4IM__TallyEntry *Entry[V4IM_TallyHash_Max] ;	/* Corresponding entries (V4IM_TallyHash_Max is min size) */
} ;

#define V4IM_vteFlags_NoBind 0x1	/* If set then bindings not created for this entry (so don't append to any ByList) */
#define V4IM_vteFlags_bitMap 0x2	/* auxPtr points to V4IM__TallyListBitMap, not V4IM__TallyListPoints in ListOf::xxx */

#define V4IM_TEeType_ns	1		/* "Normal" sum/average */
#define V4IM_TEeType_sd 2		/* Standard deviation */
#define V4IM_TEeType_fs 3		/* Fixed sum/average */
#define V4IM_TEeType_us 4		/* UOM sum/average */


struct V4IM__TallyEntry			/* Entry for each tally combo */
{ 
  void *auxPtr ;			/* Pointer to aux. structure (ex: V4IM__TallyListBitMap *tlbm or struct V4IM__TallyEntryWith *vtew) */
  int HashVal ;				/* Hash value for this entry */
#ifdef V4ENABLEMULTITHREADS
  DCLSPINLOCK mtLock ;			/* Multi-thread lock (0 if available, <> 0 if locked by another thread) */
#endif
  unsigned short Bytes ;		/* Number of bytes in this entry */
  unsigned short Offset ;		/* Offset to point buffer in this entry (beyond last Entry) */
  unsigned short ActionX ;		/* Action index */
  unsigned short vteFlags ;		/* Optional flags - see V4IM_vteFlags_xxx */

  struct {
    union {
      struct { double Sum ; } ns ;			/* 'normal' sums/counts/mins/maxs */
      struct { double sdSum, sdSumSq ; } sd ;		/* standard deviation */
      struct { B64INT fSum ; short decimals ; } fs ;	/* fixed decimal */
      struct { double uSum ; short uRef, uIndex ; } us ;/* unit-of-measure */
     } tec ;				/* Tally-Entry-Counts */
    COUNTER Count ;			/* Total count (for Count, Average, StdDev) */
    short Dim ;				/* Dimension id of entry */
    short eType ;			/* Type of entry above - V4IM_TEeType_xxx */

   } Entry[V4IM_TallyEntryValue_Max] ;
/* Space allocated after last entry for by-points */
} ;

struct V4IM__TallyEntryWith		/* Entry to link to TallyEntry to With values (via auxPtr pointer) */
{ struct V4DPI__Point wpt[V4IM_TallyEntryValue_Max] ;	/* Buffer for each possible With value */
} ;

//#define V4IM_TallyNumList_Max 250
//
//struct V4IM__TallyNumList		/* List of value points for StdDev Calculation */
//{ struct V4IM__TallyNumList *PrevTNL ;	/* Pointer to previous list (if chained) */
//  int Count ;				/* Number of entries in this list */
//  double Num[V4IM_TallyNumList_Max] ;	/* Entry for each number */
//} ;

#define V4IM_TallyBuf_Max (512*32)-32	/* Try and get V4IM__TallyBuf close to 512*32 */
#define V4IM_TallySortMax 50021

struct V4IM__TallyBuf			/* Work buffer for tally entries (for better mem mgmt) */
{ struct V4IM__TallyBuf *PriorBuf ;	/* Pointer to prior (NULL = last one) */
  INDEX NextFree ;			/* Index to next free byte below */
  double align ;			/* Buffer (below) must be double aligned! */
  char Buffer[V4IM_TallyBuf_Max] ;
} ;

struct V4IM__TallySort			/* Temp record for sorting lists as part of Tally() */
{ int HashX ;				/* Master hash index into TallyMaster */
  int Order ;				/* Incremented for each entry to ensure lists in order of entry */
  COUNTER Repeat ;			/* Number of times this point is to be repeated */
  struct V4DPI__LittlePoint sortpt ;	/* Actual list point */
} ;

#define V4IM_ListOfPointMax 5		/* Number of in-memory ListOf points to keep before writing out to sort file */
struct V4IM__TallyListPoints		/* Hold a limited number of Tally() ListOf:: points (pointed to by vte->auxPtr) */
{ INDEX numEntries ;			/* Number of entries below (cannot exceed V4IM_ListOfPointMax) */
  INDEX totalEntries ;			/* Total number of list entries associated with this vte (can be larger than V4IM_ListOfPointMax) */
  struct {
    struct V4DPI__LittlePoint loPnt ;	/* A ListOf::xxx point */
   } entry[V4IM_ListOfPointMax] ;
} ;

#if V4IM_ListOfPointMax * 24  >= V4L_MultPointBufMax
#error sizeof (struct V4IM__TallyListPoints)  >= V4L_MultPointBufMax
#endif

struct V4IM__TallySortCache		/* Sort Cache buffer ********* NOTE SEE xxxHeader BELOW ********** */
{ INDEX Count ;				/* Number of entries (starts at V4IM_TallySortMax) */
  INDEX Bits ;				/* Number of bits for UList/UCount */
  struct V4IM__TallySort tse[V4IM_TallySortMax] ;
} ;
#define  TallySortCacheHeader 2*sizeof(INDEX)	/* Size of V4IM__TallySortCache header */

struct V4IM__TallyListBitMap		/* SetOf:: bitmap - much faster when possible */
					/* NOTE: See Max below - defined off of size of header */
{ INDEX Count ;				/* Number of bits set in Map below (-1 indicates values detected outside of map) */
  unsigned short Dim ;			/* Dimension of point below */
  unsigned short PointType ;		/*   and point type */
  INDEX MaxIntVal ;			/* Max IntVal allowed in map (i.e. map goes from 1 to MaxIntVal) */
//  int Map[(V4IM_TallyListBitMapMax+31)/32] ;
  struct V4L__ListBMData1 bm1 ;
} ;
#define  V4IM_TallyListBitMapHeader (3 * sizeof (int))	/* Size of V4IM_TallyListBitMapMax header */


/*	L I S T   P A R T I T I O N I N G			*/

#define V4L_Partition_Max 32		/* Max number of partition segments */
struct V4L__Partitions {
  INDEX Count ;				/* Number of partitions below */
  struct V4L__ListPoint lpbuf ;		/* ListPoint buffer for (potentially rearranged) list point */
  struct {
    struct V4IM_TallyEnumEnvironment vtee ; /* Local Tally environment */
    INDEX Start,End ;			/* Starting and ending indexes within the list */
    COUNTER aggIndexCnt ;			/* Number of Agg indexes below (into gpi->AreaAgg[]) */
    INDEX aggIndexes[V4C_AreaAgg_Max] ;	/* Indexes into gpi->AreaAgg[] */
   } Seg[V4L_Partition_Max] ;
} ;

/*	S T R I N G   B R E A K		*/

#define vstr_SBMax 200
struct vstr__StringBreak {
  INDEX Count ;
  struct {
    INDEX StartByte ;
    INDEX EndByte ;
   } Piece[vstr_SBMax] ;
 } ;


/*	P O I N T   O P T I M I Z E   S T R U C T U R E		*/

#define OptimizeDimMax 10
struct V__Optimize {
  P *vpt ;					/* Value point */
  double MinMax ;
  FRAMEID FrameId ;
  LOGICAL isMax ;
  INTMODX intmodx ;
  INDEX Count ;
  struct {
    P ePnt ;					/* Point specification */
    P curPnt ;					/* Current Point */
    P oPnt ;					/* Optimized point */
    struct V4L__ListPoint *lp ;			/* list pointer for dimension */
    INDEX Index ;					/* Current iteration index */
   } Level[OptimizeDimMax] ;
 } ;

/*	S T A T   L I N - F I T   S T R U C T U R E	*/

#define V4IM_StatLF_XMax 20			/* Max number of x's we can have */
#define V4IM_StatLF_AtMax 100

#define V4IM_StatLF_MethodSLLS 0		/* Straight Line Least Squares */
#define V4IM_StatLF_MethodSLLAD 1		/* Straight Line Least Absolute Deviations */

struct V4IM__StatLinFit {
  ETYPE Method ;					/* Method to use: V4IM_StatLF_Methodxxx */
  struct V4DPI__Point *PEnum ;			/* Point to enumerate on */
  struct V4DPI__Point *PY ;			/* Y point or isct or list */
  struct V4DPI__Point *PStdDev ;		/* Standard deviations */
  struct V4DPI__Point *PBind ;			/* Resulting values bound via this isct */
  struct V4DPI__Point *PAt ;			/* Point or list of "at" points to evaluate */
  struct V4DPI__Point *PDo ;			/* Point (isct) to eval for each resulting point (see DimContext) */
  INDEX XYCount ;				/* Number of X/Y values points */
  struct V4IM_DblArray *Y ;			/* Actual Y points */
  INDEX XCount ;				/* Number of different x's */
  struct {
    struct V4DPI__Point *PX ;			/* Can have multiple X points */
    struct V4IM_DblArray *X ;			/* Actual X points */
    double Coefficient ;			/* Resulting coefficient (slope if single X) */
   } Xs[V4IM_StatLF_XMax] ;
  int Next ;					/* If > 0 then wants next n projected */
  double Intercept,ChiSq ;
  double AbsDev ;				/* Absolute deviation (in y) with Least Absolute Deviation fit */
  DIMID DimContext ;				/* Dimension to add resulting points (used with PDo) */
  DIMID DimSlope ;				/* Put slope/coefficients in this dimension */
  DIMID DimIntercept ;				/* Put Y intercept */
  DIMID DimChiSq ;				/* Chi-square result */
  DIMID DimAbsDev ;
} ;

/*	T R E E   S T R U C T U R E S		*/

//#define NODES_AS_PTRS 1
#ifdef NODES_AS_PTRS
typedef struct V4Tree__Node * NODELOC ;
typedef struct V4DPI__Point * POINTLOC ;
#define isNodeNull(NODE) (NODE == NULL)
#define isNodeDef(NODE) (NODE != NULL)
#define getNodePtr(NODE) NODE
#define getNodeValPt(NODE) NODE
#define setNodePtr(NODE,PTR) NODE = PTR
#define setNodeNULL(NODE) NODE = NULL
#define copyNodeValue(NODE1,NODE2) NODE1 = NODE2
#else
typedef int NODELOC ;
typedef int POINTLOC ;
#define isNodeNull(NODELOC) (NODELOC == 0)
#define isNodeDef(NODELOC) (NODELOC != 0)
#define getNodePtr(NODELOC) ((NODELOC) == 0 ? NULL : ((struct V4Tree__Node *)(((char *)tmas->tmcd->entry[(NODELOC)>>16].tmc)+(((NODELOC)&0xffff)-1))))
#define getNodeValPt(NODELOC) ((NODELOC) == 0 ? NULL : ((struct V4DPI__Point *)(((char *)tmas->tmcd->entry[(NODELOC)>>16].tmc)+(((NODELOC)&0xffff)-1))))
#define getChunkOff(PTR,INDEX) ((char *)PTR - (char *)tmas->tmcd->entry[INDEX].tmc)
/* This is more complicated, have to convert PTR to <memchkindex><offset> before saving it (NOTE: offset increased by one so we can use '0' as same as NULL) */
#define setNodePtr(NODELOC,PTR) \
  if (PTR == NULL) { setNodeNULL(NODELOC) ; }\
   else { int _i, _off ; void *_ptr = (PTR) ;\
	  for(_i=tmas->tmcd->curX;_i>=0;_i--) { _off = getChunkOff(_ptr,_i) ; if (_off >= 0 && _off < V4TREE_MemChunkSize) { NODELOC = (_i<<16)+ (_off + 1) ; break ; } ; } ; \
	} ;
#define setNodeNULL(NODELOC) NODELOC = 0
#define copyNodeValue(NODELOC1,NODELOC2) NODELOC1 = NODELOC2
#endif

#define V4TREE_MasterCacheMax 5003	/* Number of slots in id->pointer cache for a tree */
#define V4TREE_PNCacheMax 5003		/* Number of slots in 'parent-node' cache for a tree */
#define TREEHASH4CACHE(nodeId) (((nodeId) & 0x7fffffff) % V4TREE_MasterCacheMax)	/* Don't want this to ever be negative */
#define TREEHASHPNCACHE(nodeId) (((nodeId) & 0x7fffffff) % V4TREE_PNCacheMax)	/* Don't want this to ever be negative */

#define V4TREE_RelPos_Left 1
#define V4TREE_RelPos_Right 2
#define V4TREE_RelPos_Child 3
#define V4TREE_RelPos_Parent 4
#define V4TREE_RelPos_FarLeft 5
#define V4TREE_RelPos_FarRight 6

/*	TreeNode is combination of TreeId & NodeId */
//#define TREE_MAX_NUMBER 0x100		/* Max number of concurrent trees */
//#define TREEID(_NODEID) ((_NODEID >> 24) & 0xff)
//#define TREENODE(_TREEID,_NODEID) (((_TREEID & 0xff) << 24) | _NODEID)
//VEH110103 - Increase tree limit to 512
//#define TREE_MAX_NUMBER 0x200		/* Max number of concurrent trees */
//#define TREEID(_NODEID) ((_NODEID >> 23) & 0x1ff)
//#define TREENODE(_TREEID,_NODEID) (((_TREEID & 0x1ff) << 23) | _NODEID)
//VEH151117 - Increase tree limit to 1024
//#define TREE_MAX_NUMBER 0x800		/* Max number of concurrent trees */
//#define TREEID(_NODEID) ((_NODEID >> 21) & 0x7ff)
//#define TREENODEID(_NODEID) ((_NODEID) & 0x1fffff)
//#define TREENODE(_TREEID,_NODEID) (((_TREEID & 0x7ff) << 21) | _NODEID)
//VEH200810 - Increase limit to 4096

//#define TREE_MAX_NUMBER 0x1000		/* Max number of concurrent trees */
#define TREE_MAX_NUMBER 3270			/* Above is max based on number of bits, this is max based on max allowd size of V4Tree__Directory (to fit in V4IS buffer) */
#define TREEID(_NODEID) ((_NODEID >> 20) & 0xfff)
#define TREENODEID(_NODEID) ((_NODEID) & 0xfffff)
#define TREENODE(_TREEID,_NODEID) (((_TREEID & 0xfff) << 20) | _NODEID)

#define TREE_NONE -1			/* Value for no tree (UTree:none) */

typedef int TREEID ;			/* V4 Tree Id */
typedef int TREEMEMCHUNK ;		/* Tree - Memory Chunk Id */
typedef int NODEID ;			/* V4 Tree Node Id */

#define SIZEOFVTD(VTD) (((char *)&(VTD)->tree[(VTD)->nextTreeX]) - (char *)(VTD))

struct V4Tree__Directory {		/* Directory of all trees (in area) */
  union V4IS__KeyPrefix kp ;		/* Type=V4, SubType=TreeDir, Mode=Int, Len=8 */
  int mustBeZero ;			/* Must be 0 */
  INDEX nextTreeX ;			/* Next free tree[index] */
  struct {
    TREEID treeId ;
    NODEID nextNodeId ;			/* Next available node Id */
    NODELOC topNodeLoc ;		/* Top level node for tree (used to update topNode in Master) */
    DIMID dimId ;			/* Dimension associated with this tree */
    V4MSEGID tmcdSegId ;		/* Segment Id of memory chunk directory */
    B64INT treeHash ;			/* If non-zero then hash of tree (used to find duplicate XDB Value-Tree trees) */
    struct V4Tree__Master *tmas ;	/* Runtime pointer to tree master */
    } tree[TREE_MAX_NUMBER] ;
} ;

struct V4Tree__Master {			/* Master info for particular tree (created at runtime) */
  TREEID treeId ;			/* Unique tree id for this tree */
  NODEID nextNodeId ;			/* Next available node Id */
  DIMID dimId ;				/* Dimension associated with this tree */
  LOGICAL isDirty ;			/* TRUE iff tree has been updated */
  struct V4Tree__Node *topNode ;	/* Pointer to top node of tree */
  struct V4Tree__MemChunkDir *tmcd ;	/* Pointer to memory chunk 'directory' */
  struct V4Tree__RTCache *rtc ;		/* Pointer to run-time cache structure */
} ;

struct V4Tree__RTCache			/* Run-time cache(s) for a tree */
{ struct {
     struct V4Tree__Node *Node ;	/* Pointer to node s.t. index in cache = V4TREEHashIdToCache(Node->id)) */
   } Cache[V4TREE_MasterCacheMax] ;
  struct {
     struct V4Tree__Node *pNode ;	/* Cache from right-most child node -> parent */
     struct V4Tree__Node *rmNode ;
   } PNCache[V4TREE_PNCacheMax] ;
} ;

#define V4Tree_MemChunkDirInitial 64
#define SIZEOFTMCD(TMCD) ((char *)&(TMCD)->entry[(TMCD)->curX + 1] - (char *)(TMCD))
struct V4Tree__MemChunkDir
{ NODELOC locTopNode ;			/* Location of top node */
  INDEX maxEntry ;			/* Max number of tmc's defined below (initially V4Tree_MemChunkDirInitial) */
  INDEX curX ;				/* Index into current tmc[] (the one that is being filled) */
  struct {
    V4MSEGID tmcSegId ;			/* Segment containing memory chunk */
    struct V4Tree__MemChunk *tmc ;	/* Pointer (filled when tree created or loaded from area) */
   } entry[V4Tree_MemChunkDirInitial] ;
} ;

#define V4TREE_MemChunkSize V4Area_UsableBytes
#define SIZEOFTMC(TMC) ((char *)&(TMC)->Chunk[(TMC)->NextAvail] - (char *)(TMC))
struct V4Tree__MemChunk
{ 
  INDEX NextAvail ;			/* Next available byte in this chunk */
					/* *** Chunk must be aligned properly *** */
#pragma pack(8)				/* MUST BE POINTER ALIGNED */
  char Chunk[V4TREE_MemChunkSize] ;	/* Memory chunk */
#pragma pack()
} ;

struct V4Tree__Node {
  NODEID Id ;
  NODELOC Parent ;
  NODELOC LeftSibling,RightSibling ;
  NODELOC Child ;
  POINTLOC Value ;
} ;


/*	F O R E I G N	F I L E   I N T E R F A C E		*/

#define V4FFI_FileSpecSub_Max 10	/* Max number of sub-structures in a foreign file specification */
#define V4FFI_FileSpecKey_Max 5 	/* Max number of keys in a file */
#define V4FFI_FileName_Max 100		/* Max characters in foreign file name spec */
#define V4FFI_DataElName_Max 32 	/* Max characters in data element/field name */
#define V4FFI_DataEl_Max 200		/* Max fields in DataElList record */
#define V4FFI_FileSpecAutoUpd_Max 10	/* Max number of auto-update elements */
#define V4V4IS_Field_Max 512		/* Max number of fields/structures in V4IS layout */
#define V4V4IS_RTList_Max 128		/* Max number of field/structure layouts loaded at runtime */

struct V4FFI__DataElSpec		/* Specification of a field within a Foreign File */
{ unsigned short FileRef ;		/* FileRef for the file */
  unsigned short Element ;		/* Element Number */
  UCCHAR Name[V4DPI_DimInfo_DimNameMax] ;
  UCCHAR DimName[V4DPI_DimInfo_DimNameMax] ;
  unsigned short V3DT ; 		/* V3 Data Type (VFT_xxx) */
  unsigned short KeyNum ;		/* Key number (0 if not a key) */
  unsigned short Owner ;		/* Owner Structure number (0 if element corresponding to top level structure) */
  unsigned short Offset ;		/* Starting Byte within (sub)structure */
  unsigned short Bytes ;		/* Length in bytes */
  BYTE Decimals ;			/* Number of decimal places */
} ;

struct V4FFI__StructElSpec
{ unsigned short FileRef ;		/* Aux Value for this structure */
  unsigned short StructNum ;		/* Structure within structure (top level is 1) */
  unsigned short Element ;		/* Element number describing this structure */
  unsigned short Bytes ;		/* Number of bytes in a single occurance of structure */
  unsigned short Occurs ;		/* Number of times it occurs */
  BYTE CountField[V4DPI_DimInfo_DimNameMax] ; /* If not empty then name of field with count for this substructure */
  unsigned short Offset ;		/* Offset for begin of substructure within parent */
} ;

struct V4V4IS__RuntimeList		/* List of pointers to V4IS structure/element definitions */
{ INDEX Count ;				/* Number below */
  BYTE IsNew[V4V4IS_RTList_Max] ;	/* Flag if structure info below is "new/dirty" & s/b flushed to V4 area */
  int FileRefs[V4V4IS_RTList_Max] ;	/* List of file refs */
  struct V4V4IS__StructEl *stel[V4V4IS_RTList_Max] ;	/* Pointers to loaded structures/elements */
} ;

struct V4V4IS__StructEl 		/* List of structures & elements of a V4IS area */
{ union V4IS__KeyPrefix kp ;		/* Type=V4, AuxVal= V4IS_SubType_V4ISStEl, Mode=Int, Bytes=8, Key=FileRef */
  int FileRef ; 			/* Area's file ref number */
  LENMAX Bytes ;			/* Total number of bytes in this structure */
  INDEX Count ;				/* Number below */
  struct {
    unsigned short StructNum ;		/* Structure number this element is member of (or structure number if structure) */
    unsigned short Index ;		/* Element index (index of top level element if this is structure) */
    unsigned short Bytes,Offset ;	/* Number of bytes and offset from top of parent structure */
    unsigned short SubBytes ;		/* Number of bytes in parent (sub)structure */
    short V3DataType ;			/* Data type of this element (see V3 VFT_xxx) (UNUSED if structure) */
    unsigned short Decimals ;		/* Number of decimal places (max number of repeats if this is structure) */
   } Field[V4V4IS_Field_Max] ;
} ;

//struct V4FFI__DataElList		/* List of data elements in Foreign File */
//{ union V4IS__KeyPrefix kp ;		/* Type=FrgnFile, AuxVal= File's FileRef, Mode=Int, Bytes=8, Key=1 */
//  int DataElListKey ;			/* =1 */
//  struct {
//    unsigned short Dim ;		/* Substructure dimension (for index) */
//    unsigned short Bytes ;		/* Number of bytes in substucture */
//    unsigned short StartByte ;		/* Starting byte within parent structure */
//    unsigned short MaxOccurs ;		/* Max times substructure can occur */
//    unsigned char Within ;		/* If nonzero then this is within another substructure */
//   } SubStruct[V4FFI_FileSpecSub_Max] ;
//  short DataElCnt ;			/* Number of fields below */
//  struct {
//    char Name[V4FFI_DataElName_Max] ;	/* Field Name */
//    struct V4FFI__DataElSpec des ;	/* Specs for field */
//   } DataEl[V4FFI_DataEl_Max] ;
//} ;

//struct V4FFI__FileSpec			/* Specification of Foreign File */
//{ union V4IS__KeyPrefix kp ;		/* Type=FrgnFile, AuxVal = File's FileRef, Mode=Int, Bytes=8, Key=0 */
//  int FileSpecKey ;			/* 0 for now */
//  char FileName[V4FFI_FileName_Max] ;	/* If given then this is external foreign file, if null then internal */
//					/* NOTE: this may be overridden with [FileName FileRef=xxx] */
//  unsigned short Length ;		/* (Max) length of records in this filespec */
//  short VariesSubEl ;			/* Index (above) to substructure which determines length */
//  short VariesCntEl ;			/* Index (above) to field which specifies number of substructure */
//  short KeyCnt ;
//  struct {
//    struct V4FFI__DataElSpec des ;	/* Data Element Spec for this key */
//    union V4IS__KeyPrefix Prefix ;	/* Key Prefix for this key (AuxVal filled from kp above) */
//    short KeyEl ;			/* Index into DataElList to first field in key */
//    short KeyEl2,KeyEl3 ;		/*  and second/third if necessary */
//    unsigned short KeyValDim ;		/* Dimension where key value (via context) will be found */
//    unsigned short KeyValDim2 ; 	/* Dimension where second part of key (compound key) (Not often needed) */
//    unsigned short KeyValDim3 ;
//    char DuplicatesOK ; 		/* If TRUE then duplicates allowed on this key */
//    char UpdateOK ;			/* If TRUE then allowed to update key on v4is_Replace() */
//   } Keys[V4FFI_FileSpecKey_Max] ;	/* Info for each key in file/area */
//  struct {
//    short Trigger ;			/* When to trigger auto-update */
//    short ToDo ;			/* What to do */
//    short KeyEl ;			/* Field (index to above) to be updated */
//   } AutoUpdate[V4FFI_FileSpecAutoUpd_Max] ;
//} ;

#define V4FFI__KeyInfoBuf_Max 2000	/* Max bytes in buffer below */
#define V4FFI_KeyInfoDtl_KeyMax 10	/* Max number of keys per FileRef */

struct V4FFI__KeyInfo			/* Holds key info in root index (after RootInfo) for Foreign file info */
{ unsigned short Count ;		/* Number of detail records in Buffer */
  unsigned short Bytes ;		/* Total number of bytes for this structure */
  BYTE Buffer[V4FFI__KeyInfoBuf_Max] ;
} ;

struct V4FFI__KeyInfoDtl		/* Holds detail for keys in FileRef */
{ unsigned short FileRef ;
  BYTE Bytes ; 				/* Total number of bytes allocated */
  BYTE KeyCount ;			/* Number of keys below */
  struct
   { union V4IS__KeyPrefix KeyPrefix ;	/* V4 Key prefix (if 0 then prefix embedded in record) */
     unsigned short Offset ;		/* Offset into record for this key */
     BYTE DupsOK ;			/* If TRUE then duplicates allowed */
     BYTE UpdateOK ;			/* If TRUE then allowed to change this key */
   } Key[V4FFI_KeyInfoDtl_KeyMax] ;
} ;

/*	S E Q U E N T I A L   D U M P	S T R U C T U R E S	*/

#define V4IS_SeqDumpType_RootInfo 0	/* RootInfo info to follow */
#define V4IS_SeqDumpType_DataRec 1	/* Data record to follow */
#define V4IS_SeqDumpType_AreaHI 2	/* V4 Information */
#define V4IS_SeqDumpType_FFIKeys 3	/* Foreign Key Information */
#define V4IS_SeqDumpType_EOF 4		/* Marks end of sequential file */

#define V4IS_SeqDumpPlace_Index 0	/* Data found in index bucket */
#define V4IS_SeqDumpPlace_Data 1	/* Data found in data bucket */

struct V4IS__SeqDumpHdr 		/* Format of header for sequential area dump file */
{  unsigned SeqDumpType : 6 ;		/* Type of record to follow- see V4IS_SeqDumpType_xxx */
   unsigned CmpMode : 2 ;		/* Data Compression - see V4IS_DataCmp_xxx */
   unsigned Placement : 3 ;		/* Where data was placed */
   BIGFIELD Bytes : 21 ;		/* Bytes in record to follow */
   int FileRef ;			/* Record's FileRef if a foreign key */
} ;

struct V4IS__SeqDumpRI			/* RootInfo Information for reconstruction */
{ int BktSize ; 			/* Number of bytes in a bucket */
  int Version ;
  int NextAvailDimDictNum ;		/* Next available dictionary entry number (don't forget 3 bit heirarchy prefix!) */
  int NextAvailExtDictNum ;		/* Next available external dictionary entry number */
  int NextAvailIntDictNum ;		/* Next available internal dictionary entry number */
  int NextAvailValueNum ;		/* Next available point number */
  int MinCmpBytes ;			/* Compress any record longer than this */
  int MaxRecordLen ;			/* Maximum record length */
  int DataMode ;			/* Determines where data is to be stored (index or data buckets) */
  int FillPercent ;			/* Percentage to fill index buckets on reformat (0-100) */
  int AllocationIncrement ;		/* Allocation increment (bytes) */
  int RecordCount ;			/* Number of records in area */
  int GlobalBufCount ;			/* Number of global buffers to allocate (0 for auto-determine) */
  int LockMax ;
  int NextAvailUserNum ;
  V4MSEGID lastSegNum ;
  int Expansion[9] ;
} ;

struct V4IS__SeqDumpFFIK		/* Holds Foreign File Key Information for Reconstruction */
{ unsigned short Count ;		/* Number of detail records in Buffer */
  unsigned short Bytes ;		/* Total number of bytes for this structure */
  BYTE Buffer[V4FFI__KeyInfoBuf_Max] ;
} ;

struct V4IS__SeqDumpFFIKDetail
{ unsigned short FileRef ;
  BYTE Bytes ; 				/* Total number of bytes allocated */
  BYTE KeyCount ;			/* Number of keys below */
  struct
   { union V4IS__KeyPrefix KeyPrefix ;	/* V4 Key prefix (if 0 then prefix embedded in record) */
     unsigned short Offset ;		/* Offset into record for this key */
     BYTE DupsOK ;			/* If TRUE then duplicates allowed */
     BYTE UpdateOK ;			/* If TRUE then allowed to change this key */
   } Key[V4FFI_KeyInfoDtl_KeyMax] ;
} ;

struct V4IS__SeqDumpAreaHI		/* Area Information for V4 reconstruction */
{ short RelHNum ;			/* Relative Hierarchy Number */
  short ExtDictUpd ;			/* If TRUE then OK to update external */
  short IntDictUpd ;			/* If TRUE then OK to update internal */
  short BindingsUpd ;			/* If TRUE then OK to update bindings */
} ;

#define V4IS_SeqFileList_Max 20

#define V4IS_SeqFileType_Seq 0
#define V4IS_SeqFileType_V4IS 1
#define V4IS_SeqFileType_Index 2	/* Append index only */
#define V4IS_SeqFile_BktOffset 750000	/* Default Bucket Offset */

struct V4IS__SeqFileList		/* List of file names */
{ INDEX BktOffset ;			/* Bucket offset to use for Index-Only */
  INDEX Count ;
  struct {
     ETYPE Type ; 			/* Type of file: V4IS_SeqFileType_xxx */
     LENMAX BktSize,BktCount ;		/* Bucket size & number of buckets if V4IS file */
     UCCHAR Name[V_FileName_Max] ;
   } File[V4IS_SeqFileList_Max] ;
} ;

/*	O P T I M I Z A T I O N   S T R U C T U R E S		*/

#define V4EVAL_OptDimList_Max 50

struct V4EVAL_OptDimList		/* List of Dimensions which are OK to optimize on via v4eval_OptPoint() */
{ INDEX Count ;				/* Number of dimensions in list below */
  DIMID Dims[V4EVAL_OptDimList_Max] ;	/* Dimensions OK to optimize */
} ;


/*	C O N T E X T	S T R U C T U R E S			*/

#define V4DPI_XDictRTCacheMax 503

#define XDRNEW 1
#define XDRDIRTY 2

struct V4DPI__XDictRuntime		/* Runtime area pointed to by ctx->xdr */
{ int aid ;				/* Area Id */
#ifdef V4ENABLEMULTITHREADS
  DCLSPINLOCK mtLock ;			/* Used for multi-threaded spin-lock */
#endif
  struct V4IS__ParControlBlk *pcb ;	/* Parameter control block for this area dictionary */
  struct V4DPI__XDictRevCache *xdrc ;	/* If not NULL then pointer to Reverse lookup cache */
  struct V4DPI__XDictCache *xdc ;	/* If not NULL then pointer to lookup cache */
  COUNTER LookupCount ;			/* Number of XDict lookups (used to trigger enabling of lookup cache) */
  DIMID LastDimId ;			/* Last used DimId value */
  INDEX Count ;
  struct {
    DIMID DimId ; 			/* Cache for dimension associated with DimXId */
    DICTID DimXId ;			/* Dimension XId in dictionary */
    DICTID LastPoint ;			/* Last point number used for this dimension */
    FLAGS32 Flags ; 			/* Flags associated with entry- XDRxxx */
   } Cache[V4DPI_XDictRTCacheMax] ;
} ;

#define V4DPI_XDictRevCacheMax 211
struct V4DPI__XDictRevCache
{ COUNTER Attempts,Hits ;			/* Number of cache attempts and successful hits */
  struct {
    DIMID DimId ;
    DICTID DictId ;
    UCCHAR StrVal[V4DPI_XDictEntryVal_Max+1] ;
   } Cache[V4DPI_XDictRevCacheMax] ;
} ;

#define V4DPI_XDictCacheTrigger 200	/* Do not trigger lookup cache until after this number of lookups */
#define V4DPI_XDictCacheMax 211
#define V4DPI_XDictCacheMaxThrash 197 
struct V4DPI__XDictCache
{ int HashDiv ;
  struct {
    DIMID DimId ;
    DICTID DictId ;			/* Value of Dict Entry */
    COUNTER Thrash ;
    UCCHAR Name[V4DPI_XDictEntryVal_Max+1] ;
   } Cache[V4DPI_XDictCacheMax] ;
} ;


#define V4AGG_AggDims_Max 50
struct V4AGG__AggDims			/* Holds list of dimensions in an Aggregate, also linked to V4C__Context */
{ union V4IS__KeyPrefix kp ;		/* Type=V4, AuxVal/SubType=AggDims, Mode=Int, Len=8 */
  int KeyVal ;				/* must be 0 */
  INDEX Count ;
  DIMID Dims[V4AGG_AggDims_Max] ;
  struct {
    DICTID BeginPt,EndPt ; 		/* Start & end point values for this dim in this Aggregate */
   } Entry[V4AGG_AggDims_Max] ;
} ;

struct V4AGG__Constant
{ union V4IS__KeyPrefix kp ;		/* Type=V4, AuxVal/SubType=AggConstant, Mode=Int, Len=8 */
  DIMID Dim ;				/* Dimension associated with point */
  struct V4DPI__Point dimpt ;
} ;


/*	V4__IsctEval_Cache - Isct Evaluation Cache */

#define V4_IsctEval_CacheMax 103
#define V4_IsctEval_CacheBytes 128
struct V4__IsctEval_Cache {
  struct {
   AREAID AreaId;
   int KeyVal ;
   struct V4DPI__Binding *bind ;
  } Entry[V4_IsctEval_CacheMax] ;
 } ;



#define V4C_FrameId_Real -9		/* Special code to add point to last "real" (i.e. user defined) frame */
#define V4C_FrameId_Hold -10		/* Special code to add point to current frame but put on "HOLD" */
#define V4C_FrameId_NoOpt -11		/* Special code to add point - disable any optimizations (called from BindListMake) */

/************************************************************************/
#define V4C_CtxDimHash_Max 0x800 	/* Number of entries in hash table ** MUST BE POWER OF 2 - see DIMIDHASH macro below ** */
#define DIMIDHASH(DIM)	(DIM & 0x7ff)	/* Function to map dimid to hash index ** SEE V4C_CtxDimHash_Max above - must be in synch! ** */
/************************************************************************/

#ifdef V4_BUILD_LMTFUNC_MAX_CTX_PTS
#define V4C_CtxVal_Max V4_BUILD_LMTFUNC_MAX_CTX_PTS
#else
#define V4C_CtxVal_Max 300		/* Max number of distinct dimension points for all context frames */
#endif

#define V4C_FrameDimEntry_Max 100	/* Max points in a given frame */
#define V4C_FrameName_Max 31		/* Max chars in frame name */
//#define V4C_Frame_Max V4TRACE_MaxNestedIsctEval+5   /* Max number of nested frames */
					/* NOTE: s/b >= V4TRACE_MaxNestedIsctEval */
#define V4C_PragmaHash_Max 203		/* NUmber of entries in "pragma" structure */
#define V4C_CtxRelH_Max 8		/* Max number of relative heirarchy slots in context (1 thru 7, 0 denotes foreign only) */
#define V4C_CtxVal_nblMax 5		/* Max number of nbl's per context point */


#define V4C_FrameAdd_BaseMask 0xfffffff /* Used to mask out dimension from basedim argument */
#define V4C_CheckIsA 0x80000000 	/* On context-add, check dimension:point for any IsA links & added to context */
#define V4C_TraceIsA 0x40000000 	/* Output trace on IsA evals */

struct V4C__AreaHInfo
{ short RelHNum ;			/* Relative Hierarchy Number */
  short ExtDictExists ; 		/* UNUSED If TRUE then External dictionary exists in this area */
  short IntDictExists ; 		/* UNUSED If TRUE then Internal dictionary exists in this area */
  short ExtDictUpd ;			/* If TRUE then OK to update external */
  short IntDictUpd ;			/* If TRUE then OK to update internal */
  short BindingsExist ; 		/* UNUSED If TRUE then bindings exists */
  short BindingsUpd ;			/* If TRUE then OK to update bindings */
  short IsPrimary ;			/* UNUSED If TRUE then primary area (otherwise a slave) */
  char BindCatList[250] ;		/* UNUSED String list of binding categories */
  short BindCatUpd ;			/* UNUSED 1 to replace, 2 to append */
} ;

#define V4C_Cache_Max 64
#define V4C_Cache_Mask 0x3F		/* Mask must match Cache_Max above!!! */
#define V4C_FreeNblListMax 50		/* Max number of free nbl pointers to keep */
#define V4DPI_BigTextBuf_Max 3		/* Number of BigText buffers */

struct V4C__Context
{
#ifdef V4ENABLEMULTITHREADS
  struct V4C__Context *pctx ;		/* Pointer (if not NULL) to prior context */
  INDEX aggStream ;			/* Index into aggregate stream(s) for this context */
#endif
  LOGICAL disableFailures ;		/* If TRUE - Globally turns OFF Register-Error & v_Msg processing (see DefQ() handler) */
  BYTE HaveScopedPts[V4C_Frame_Max] ;	/* Set to TRUE if level allocated a scoped point */
  COUNTER IsctEvalCount ;		/* Number of calls to IsctEval() with this context */
  COUNTER IsctEvalValueGetCount ;	/* Number of v4is gets for Isct eval values */
  COUNTER IsctEvalValueCache ;		/* Number of v4is gets we pulled from cache (subset of IsctEvalValueGetCount) */
  COUNTER ContextAddCount ; 		/* Number of calls to FrameAddDim() */
  COUNTER ContextAddGetCount ;		/* Number of calls to v4is within FrameAddDim() */
  COUNTER ContextAddCache ; 		/* Number of cache hits (instead of calls to v4is) */
  COUNTER ContextAddSlams ; 		/* Number of context add "slams" (memcpy overwrite) we were able to do */
  COUNTER CtxnblCachePuts ; 		/* Number of nbl cache insertions */
  COUNTER CtxnblCacheHits ; 		/*  and number of successful hits */
//  DIMID xdbDimId ;			/* Dimension of current dbGet() - if xdbRowIsPerm TRUE (below) then save current dbGet() row (see v4xdb_SaveXDBRow()) */
//  LOGICAL xdbRowIsPerm ;		/*   works with above to determine if current dbGet() row s/b saved */
  struct V4DPI__Point *bpm ;		/* If non-NULL then EvalDoit updates with Isct that match last binding */
  int LastBindValAid ;			/* Aid of value from last intersection evaluation */
  struct V4DPI__BindPointVal bpv ;	/* Updated with info on last binding */
  struct V4C__ProcessInfo *pi ; 	/* Process info "owning" this context */
  struct V4C__GBBCache *gbbc ;		/* Pointer to v4dpi_GetBindBuf() cache */
  struct V4IM__DeferDoSteps *vdds ;	/* If not NULL then structure holding arguments for deferred Do() steps (via XML::UV4:Defer) */
  struct V4__IsctEval_Cache viec ;	/* Isct evaluation cache for v4dpi_IsctEval() */
  COUNTER bigtextBufCnt ;		/* Number of calls to v4_BigTextCharValue() */
  struct V4LEX__BigText *bigtextCur ;	/* 'Current' (last accessed) bigtext buffer for v4_BigTextCharValue() */
  struct V4LEX__BigText *bigtextBufs[V4DPI_BigTextBuf_Max] ; /* Pointers to bigtext buffers for v4_BigTextCharValue() */
  UCCHAR rxdName[V4DPI_XDictEntryVal_Max+1] ; /* Buffer for v4dpi_RevXDictEntryGet() returned name */
  UCCHAR rdName[V4DPI_DictEntryVal_Max+1] ; /* Buffer for v4dpi_RevDictEntryGet() returned name */
  BYTE *cmpBuf ;			/* Buffer for data compression/expansion used by v4dpi_GetBindBuf() */
  COUNTER EvalListCnt ;			/* Number of entries in array below */
  INDEX EvalListDimIndexes[V4C_CtxDimHash_Max] ; /* List of hash indexes to "active dimensions" with BindList potential */
  INDEX DimHashCnt ;			/* Number of dimensions in hash table */
  struct
   { DIMID Dim ;			/* Dimension */
     INDEX CtxValIndex ;		/* Index to current value below (-1 if no point) */
     struct V4DPI__DimInfo *di ;	/* Pointer to dimension information */
   } DimHash[V4C_CtxDimHash_Max] ;
  INDEX FnblCnt ; 			/* Number of free nbl pointers below */
  struct V4DPI__BindList *fnbl[V4C_FreeNblListMax] ;
  short CtxValCnt ;			/* Number of dimension points below */
  short FirstFreeIndex ;	/* Index to first free entry below (FrameID links to next) */
  struct
   {
     struct V4DPI__Point Point ;	/* Context point */
     struct V4DPI__BindList *nbl[V4C_CtxVal_nblMax] ;	/* Pointer to BindList for this context point */
     LOGICAL UnDefined ;		/* TRUE if point is undefined (if PntType=Special && Grouping=Undefined) */
     int AreaIds[V4C_CtxVal_nblMax] ;	/* Area where nbl was found */
     INDEX nblCnt ;			/* Number declared above */
     INDEX PriorCtxValIndex ;		/* Prior dimension point index */
     INDEX FrameIndex ;			/* Frame Index (below) this entry belongs to (or index to next free) */
     DIMID BaseDim ;			/* If nonzero then the base dimension (this dimension added via "isa" link) */
#ifdef V4_BUILD_SECURITY
     LOGICAL Hold ; 			/* If TRUE then hold context point (don't allow another point to displace) */
#endif
     LOGICAL HaveNBL ;			/* -1 = no BindList for point, 0 = don't know yet, 1 got one in nbl above */
   } CtxVal[V4C_CtxVal_Max] ;		/* Holds dimension points for context */
  short FrameCnt ;			/* Number of frames below */
  FRAMEID NextAvailFrameId ;		/* Next available frame ID */
  struct
   { FRAMEID FrameId ;			/* Frame ID */
     INDEX DataStreamFileX ;		/* File index for output to Data stream. May be set by Do() module. (UNUSED for default output data stream) */
     LOGICAL FreeMemory ;		/* If TRUE then points in this frame have memory to be freed when frame is popped */
     UCCHAR FrameName[V4C_FrameName_Max] ;
     struct V4IM__BaA *baf ;		/* Pointer to Before-and-After test structure or NULL if none */
     struct V4IM__CatchExcept *vce ;	/* If not NULL then pointer to exceptions to be caught */
     INDEX PointCnt ;			/* Number of dimension points below */
     struct
      { short CtxValIndex2 ;		/* Index to point above */
	short DimHashIndex ;		/* Index to dimension above */
      } FrameDimEntry[V4C_FrameDimEntry_Max] ;
   } Frame[V4C_Frame_Max] ;
  UCCHAR ErrorMsg[V4TMBufMax] ;		/* Temp spot to build error messages */
  UCCHAR ErrorMsgAux[V4TMBufMax] ;	/* Temp spot to build aux error messages */
  LOGICAL InIsctFail ;			/* Set to TRUE while in IsctFail() */
  LOGICAL FailProgress ;		/* How far IsctEval got before failing - see V4FailProgress_xxx */
//  int LastFailIndex ;			/* Saved index below to last saved error message */
  UCCHAR utbuf1[V4TMBufMax] ;		/* Scratch very-large text buffer (accessed via Macro UCTBUF1) */
  UCCHAR utbuf2[V4TMBufMax] ;		/*   and another one (UCTBUF2) */
  char tbuf1[V4TMBufMax] ;		/* Scratch byte-text buffer (access via Macro ASCTBUF1) */
  UCCHAR *imArgBuf[V4DPI_IntModArg_Max] ; /* If trace enabled then holds argument strings */
  INDEX rtStackX ;			/* Runtime stack nesting index */
  INDEX rtStackFail ;			/* If unused then index into rtStack[] at time of first failure */
#ifdef NASTYBUG
  P *isctIntMod ;
  P *isct ;
#endif
  struct {
     struct V4DPI__Point *isctPtr ;	/* Pointer to intersection */
     UCCHAR *failText ;			/* Error message associated with error */
     union V4DPI__IsctSrc vis ;		/* Source location of intersection */
     struct V4DPI__BigIsct *biBuf ;	/* If not NULL then allocated big-intersection buffer for this frame */
   } rtStack[V4C_Frame_Max] ;		/* Run time statck (indexed by ctx->NestedIsctEval) */
  LOGICAL LCacheDirty ;			/* Set to TRUE whenever cache below has something in it */
  struct
   { struct V4DPI__LittlePoint lpt ;	/* Holds point NOT found in areas */
   } Cache[V4C_Cache_Max] ;
} ;

#define UCTBUF1 ctx->utbuf1
#define UCTBUF2 ctx->utbuf2
#define ASCTBUF1 ctx->tbuf1

/*	V4C__GBBCache - Context cache for binding buffers	*/

#define GBB_BufMax 512			/* Max number of bytes to in cache buffer */
#define GBBMax 97			/* Number of cache slots */
struct V4C__GBBCache {
  struct {
    DATAID DataId ;			/* DataId we cached */
    int aid ;				/*  and aid */
    INDEX Bytes ;			/* Number of data bytes below */
    BYTE buf[GBB_BufMax] ;		/* Buffer */
   } Entry[GBBMax] ;
} ;

/*	This macro is a fast way to add a point to the context
	 It is to be used only with GREAT CARE in places like the main loops of Enum() and Tally()
	 It uses "goto labels" and therefore UID must be different if multiple uses within single subroutine
	 Yes it is ugly, but fast ugly (I can live with that)
	 Same as: v4ctx_FrameAddDim(ctx,0,POINT,0,0)
*/
#define FASTCTXADD(POINT,UID) \
 { INDEX _hx,_cx,_fx ; \
   _hx = DIMIDHASH((POINT)->Dim) ; \
   if (ctx->DimHash[_hx].Dim != (POINT)->Dim) goto use_v4ctx_FrameAddDim##UID ; \
   _cx = ctx->DimHash[_hx].CtxValIndex ; \
   if (_cx == UNUSED) goto use_v4ctx_FrameAddDim##UID ; \
   _fx = ctx->CtxVal[_cx].FrameIndex ; \
   if (ctx->Frame[_fx].FrameId == ctx->Frame[ctx->FrameCnt-1].FrameId \
	 && (ctx->DimHash[_hx].di == NULL ? FALSE : ctx->DimHash[_hx].di->BindList==0)) \
    { memcpy(&ctx->CtxVal[_cx].Point,POINT,(POINT)->Bytes) ; \
      ctx->ContextAddCount++ ; ctx->ContextAddSlams++ ; \
      goto fastctxadddone##UID ; \
    } ; \
use_v4ctx_FrameAddDim##UID: \
  v4ctx_FrameAddDim(ctx,0,POINT,0,0) ; \
fastctxadddone##UID: ; \
 }


#define V4FailProgress_Unk 0		/* Fail progress is unknown, or no failure */
#define V4FailProgress_NoBind 1 	/* IsctEval failed because no binding found */
#define V4FailProgress_NestEval 2	/* IsctEval failed because nested evaluation (isct or intmod) failed */

//#define V4IM_FHL_Max 10
//struct V4IM__FrameHanList
//{ struct V4IM__FrameHanList *fhl ;	/* Link to next in chain or NULL for end of list/chain */
//  INDEX Count ;				/* Number of handles below */
//  int Handles[V4IM_FHL_Max] ;
//} ;


/*	V4DPI__RecogBlock - Recognition Descriptor Block */

#define V4DPI_RegBlock_Trace 0x1	/* Trace substitutions */
#define V4DPI_RegBlock_Eval 0x2 	/* Evaluate substitution and return result as point */
#define V4DPI_RegBlock_IgnCase 0x4	/* Use case-insensitive on regexp match */

struct V4DPI__RecogBlock
{ FLAGS32 Flags ;			/* Processing flags V4DPI_RegBlock_xx */
  struct V4DPI__RecogBlock *nrcgb ;	/* If not NULL then pointer to next in chain */
  struct re_pattern_buffer rpb ;	/* Pointer to pattern buffer */
  UCCHAR subbuf[1000] ;			/* Holds string to be used (regexp groups denoted by \n)" */
} ;

/*	S P A W N   */

#define V_SPAWN_MAX_KBOUT 10000	/* Default to 10mb output before error */
struct V__SpawnArgs {		/* Spawn argument block */
  UCCHAR *exeFile ;		/* Program to run, if null then inferred from argBuf */
  UCCHAR *argBuf ;		/* Arguments to program, NULL if no arguments */
  UCCHAR *errBuf ;		/* Pointer to string for any error message */
  FLAGS32 waitFlags ;		/* Spawn flags - V_SPAWNPROC_xxx */
  LENMAX waitSeconds ;		/* If > 0 then max number of seconds to wait for subprocess completion */
  LENMAX kilobytesOut ;		/* Max number of k-bytes output allowed from subprocess (to prevent runaway log files) */
  P *respnt ;			/* If not null then point updated with results (maybe process/thread info) */
  int *sockPtr ;		/* If not NULL, is pointer to socket to monitor and if disconnect then terminate process immediately */
  FILEID fileId ;		/* If not UNUSED, is fileId for subprocess output to be streamed to (and closed when subprocess terminates) */
} ;
//LOGICAL v_SpawnProcess(exefile, argbuf,waitflag,errbuf,respnt,sockptr,fileId)

/*	V_SPAWNPROC_xxx - wait options (cannot exceed 0xffff) */
#define V_SPAWNPROC_Wait 0		/* v_SpwanProcess is to wait for subprocess to complete */
#define V_SPAWNPROC_NoWait 1		/* v_SpwanProcess is to return immediately (before subprocess finishes) */
#define V_SPAWNPROC_Application 2	/* v_SpwanProcess is to start up new (Windows) application */
#define V_SPAWNPROC_Any 3		/* Wait for ANY of several processes to complete */
#define V_SPAWNPROC_All 4		/* Wait for ALL of several processes to complete */

#define SPAWNMAXWAITENC(SECS) ((SECS)<<16)	/* Convert seconds into proper format for v_SpawnProcess waitflag argument */
#define SPAWNMAXWAITDEC(WAITFLAG) \
 (WAITFLAG == V_SPAWNWAIT_Infinite ? 0 : (((WAITFLAG)>>16) & 0xffff)) /* Convert waitflag to number of seconds */
#define V_SPAWNWAIT_Infinite 0		/* Wait forever */
#define V_SPAWNWAIT_MaxSecs 0xffff	/* Maximum number of seconds to wait (other than infinite) */

#define V_SPAWNSORT_DfltWait SPAWNMAXWAITENC(5*60)|V_SPAWNPROC_Wait

#define V_SPAWNPROC_NewWindow 0x10000	/* Create a new window */

#define V_Restrict_OSFileListOf 0x10	/* OSFile(ListOf?) */
#define V_Restrict_OSFileInfo 0x20	/* Other OSFile info options */
#define V_Restrict_OSFileCopyRenDel 0x40 /* Copy, Rename & delete options in OSFile() */
#define V_Restrict_EvalNest 0x80	/* Allow use of Nest::logical tag in Eval() module */
#define V_Restrict_ProjectArea 0x100	/* Use areas for Projection() */
#define V_Restrict_AreaUpdate 0x4000	/* Area Create/Update/Index command */
#define V_Restrict_AreaCommand 0x8000	/* All modes of Area command */
#define V_Restrict_DataStream 0x10000	/* Restrict access to Output DATA stream */
#define V_Restrict_redirection 0x40000	/* Use of the > and >> redirections in Eval */
#define V_Restrict_QuotaEcho 0x80000	/* Check Echo() output quotas */
#define V_Restrict_QuotaHTML 0x100000	/* Check HTML() quotas */
#define V_Restrict_FileRead 0x800000	/* Ability to read/access/include arbitrary files */
#define V_Restrict_XDBAccess 0x1000000	/* Restrict ability to connect/read from external database */
#define V_Restrict_XDBXCT 0x2000000	/* Restrict ability to perform dbXct() on external database */
#define V_Restrict_Test 0x4000000	/* Restrict use of V4(Test::xxx) option */
//#define V_Restrict_Value 0x8000000	/* Restrict use of Secure(Value::xxx) option */

#define V_RtOpt_Unicode 0x1		/* Unicode version (if not set then ASCII) */
#define V_RtOpt_MultiThread 0x2		/* Multi-threading for certain functions */
#define V_RtOpt_64Bit 0x4		/* Running native 64-bit */
#define V_RtOpt_Expires 0x8		/* This version is set to expire on certain date */
#define V_RtOpt_Limited 0x10		/* This version has limited functionality */
#define V_RtOpt_ClientServer 0x20	/* This version has client-server functionality */
#define V_RtOpt_Telnet 0x40		/* This version is for remote sites with Telnet access (all console IO is non-Unicode) */
#define V_RtOpt_advSecurity 0x80	/* This version has advanced security */

#define V_DUPDICTACT_Undef 0		/* No action has been specified - decide for user (default) */
#define V_DUPDICTACT_Warn 1		/* Output warning if duplicate point in POINT command */
#define V_DUPDICTACT_Err 2		/* Generate error if duplicate point */
#define V_DUPDICTACT_Ignore 3		/* Ignore (no warning) if duplicate point */

/*	V4C__ProcessInfo - Process specific info */

#define V4DEBUG_SETUP \
 extern int v4NutCracker ; extern char v4NCBuf[512] ;


#define V4CTX_DfltMaxAllowedErrs 100	/* Default max allowed errors before quitting */
#define V4CTX_MaxPointOutputInit 250	/* Initial value for point printable output */
#define V4CTX_MaxPointOutputMax 16384	/* Largest allowed value */

#define V4BUILD_OpCodeMax 275		/* Must not be less than V4IM_OpCode_LastOpcode */

struct V4C__ProcessInfo
{
  INDEX v4argx ;			/* If not UNUSED then argument after v4 program name */
  INDEX argc ;				/* Number of arguments to xv4 process call */
  UCCHAR **ucargv ; 			/* Pointer to arguments */
  UCCHAR **envp ; 			/* Pointer to processes environment */
  jmp_buf environment ; 		/* Error recovery environment */
  struct V4LEX__TknCtrlBlk  *MainTcb ;	/* Main token control block */
  struct V4LEX__CompileDir *vcd ;	/* Updated with source files, etc. when compiling */
  struct V4Tree__Directory *vtd	;	/* Tree directory */
  struct V4LEX__MacroDirectory *vlmd ;	/* If not NULL then points to top-level macro directory */
  struct VJSON__Blob *vjblob ;		/* If not NULL then points to first blob in possible chain of JSON blobs */
  int ppHNum ;				/* Set to current output area HNum on entry to PointParse */
  struct V4LEX__DIMDIR *vldd ;		/* If not NULL then track all compile-time dimension references */
  LOGICAL Compiling ;			/* If TRUE then V4 is compiling code - limit extent of error messages */
  struct V4IM__Matrix *Matrix ; 	/* Pointer to current Matrix() via v4im */
  struct V4IS__V4Areas *v4a ;		/* If non NULL then points to V4IS areas being accessed by V4 */
  struct V4DBG__BreakPointList *bpl ;	/* If non NULL then points to break point list */
  LOGICAL breakOnEval ;			/* If TRUE then break on first Evaluate command within interpreter (set by -B startup switch) */
  double DblEpsilon ;			/* Amount 2 reals can differ yet still be considered equal (default is 0) */
  double DblEpsilonZero ;		/* If Abs(real result) < DblEpsilonZero then force it to 0 */
  FLAGS32 rtOptions ;			/* Run-time optiosn - V_RtOpt_xxx */
  UCCHAR rtOptString[32] ;		/* Run-time option string (set by ProcessInfo) */
  UCCHAR userOptString[128] ;		/* User run-time options (set via startup -ro switch) */
  LOGICAL DoEchoS ; 			/* If TRUE then EchoS() is Echos(), else use EchoT() */
  UCCHAR formatMasks[V4DPI_PntType_Last][32] ; /* Process-specific formatting masks indexed by point type */
  ETYPE OutputCharSet ;			/* Default format for output character set (see V4L_TextFileType_xxx) */
  ETYPE inputCharSet ;			/* Default input format (see V4L_TextFileType_xxx) */
  LOGICAL DoLogToIsct ;			/* If TRUE then actually do LogToIsct command, otherwise ignore */
  LOGICAL SaveMacros ;			/* If TRUE then save macro defs as [UMacro:name UV4:Definition] */
  LOGICAL SaveTables ;			/* If TRUE then save table defs as [UTable:name UV4:Definition] */
  LOGICAL inMIDAS ;			/* If TRUE then parsing midaslib style 'macro' definitions */
//  LOGICAL doSummaryAtExit ;		/* If TRUE then output one line processing summary at V4 exit */
  ETYPE xmlCase ;			/* 0 = output xml tags as-is, 1 = output upper case, 2 = output lower case */
//  LOGICAL EchoZ ;			/* If TRUE then EchoS() becomes EchoZ() (hopefully TEMPOROARY veh011005) */
  ETYPE dupPointAction ;		/* Action on duplicate point in POINT command (dictionary/external points), see V_DUPDICTACT_xxx */
  COUNTER mtLockConflicts ;		/* Count of locking conflicts while multi-threading */
  ETYPE exitCode ;			/* If non-zero then exit xv4 process with this code */
  UB64INT spwHash64 ;			/* If non-zero then hash of user's startup (-P xxx) password */
#ifdef V4_BUILD_SECURITY
  BYTE lockModule[V4BUILD_OpCodeMax+1] ;/* If TRUE then intmod with that index is locked and may not be accessed */
  BYTE lockModuleParse[V4BUILD_OpCodeMax+1] ;/* If TRUE then intmod with that index cannot be parsed/interpreted */
#endif
  LOGICAL watchDogEnabled ;		/* TRUE if watchdog thread running */
  COUNTER maxIdle ;			/* Max number of seconds of idle before self-destruct (<= 0 = wait forever) */
  COUNTER maxCPU ;			/* Max CPU seconds */
  COUNTER maxKIscts ;			/* Max number of kilo-iscts (thousands of intersections evaluated) before self-destruct */
  COUNTER sortWaitFlag ;		/* waitflag argument to pass to v_SpawnProcess() when spawning xvsort.exe */
#ifdef WINNT
  LOGICAL didWSASetup ;			/* TRUE if we have performed Windows Socket initialization */
  HANDLE watchDogThread ;		/* Thread of maxIdle, maxCPU, maxIsct watchdog */
#endif
#if defined LINUX486 || defined RASPPI
  pthread_t watchDogThread ;
#endif
#ifdef V4_BUILD_RUNTIME_STATS
  COUNTER V4IntModTotalByArg[32] ;		/* Total number of calls by number of arguments (e.g. V4IntModTotalByArg[3] = total calls with 3 arguments) */
  COUNTER V4DtlModCalls[V4BUILD_OpCodeMax+1] ;	/* Counts number of module calls by module */
  COUNTER V4ArgsInMod[V4BUILD_OpCodeMax+1] ;	/* Total number arguments in all calls to module */
  COUNTER V4TicksInMod[V4BUILD_OpCodeMax+1] ;	/* Total number of clock() ticks in all calls to module */
#endif
  int ErrNum ;				/* Updated by v4_error to last error number */
  COUNTER ErrCount ;			/* Number of calls to v4_error() */
  COUNTER WarnCount ;			/* Number of warnings generated - EchoW() */
#ifdef CHECKCLASSICMODE
  int ClassicMode ;			/* If TRUE then revert to V4 Classic-Edition */
#endif
  char patternAJAXJSON[256] ;		/* If set then v_Msg format pattern for outputting AJAX data (i.e. normal return) */
  FILEID fileIdDefer ;			/* If not UNUSED then fileId of deferred output stream */
  char patternAJAXErr[256] ;		/* If set then v_Msg format pattern for outputting AJAX error messages */
  UCCHAR CtxADVEchoE[256] ;		/* If set then name of file for "Context ADV" errors to flow into */
  UCCHAR CtxADVURLonError[1024] ; 	/* Set to URL to follow after Context ADV EchoE */
  struct V4ERR__Dump *ved ;		/* If not NULL then hold info of last error dump */
  UCCHAR ErrMsg[1024] ;			/* Last error message via v4_error() call */
  UCCHAR WarnMsg[1024] ;		/* Last warning message via v4_error() call */
  INDEX LastV4CommandIndex ;		/* V4Eval command index of last V4 command executed */
  LOGICAL ErrEcho ; 			/* If TRUE then output error message within v4_error() */
  LENMAX MaxAllowedErrs ;		/* Max allowed before quitting */
  LENMAX MaxPointOutput ;		/* Max number of bytes when outputting point in printable format */
  COUNTER BindingCount ;		/* Total number of bindings */
  COUNTER MinutesWest ;			/* Number of minutes west of GMT */
  COUNTER curYear ;			/* Current year */
  COUNTER AggPutCount ;			/* Number of Aggregate Puts */
  COUNTER AggPutWCount ;		/* Number of IO Writes */
  FLAGS32 RestrictionMap ;		/* Map of current restrictions - see V_Restrict_xxx */
  COUNTER HTMLGetBytes,HTMLPutBytes ;	/* Total number of HTML bytes received & sent */
  COUNTER QuotaHTMLGet ;		/* Max number of bytes to receive in HTML (if not UNUSED) */
  COUNTER QuotaHTMLPut ;		/* Max number of bytes to xmit (if not UNUSED) */
  COUNTER DataStreamBytes ; 		/* Total bytes sent out Data stream */
  COUNTER QuotaData ;			/* Max number of bytes to write to Data stream (if not UNUSED) */
#ifdef WANTHTMLMODULE
  COUNTER XMLRPCPutBytes,XMLRPTGetBytes ;	/* Total bytes put/rcv'd via XMLRPC */
  COUNTER QuotaXMLRPCPut ;		/* Max number of bytes to write via XMLRPC() */
  COUNTER QuotaXMLRPCGet ;		/* Max bytes to read via XMLRPC() */
#endif
  INTMODX SavedIntModx ;		/* Saved IntMod Index */
  UCCHAR SavedErrMsg[V4TMBufMax] ;	/* Holds error message (set when empty string, not reset until cleared) */
  UCCHAR SavedErrIsct[V4TMBufMax] ;	/*   ditto for offending isct generating error */
  struct V4IM__LocalVHdr *lvh ; 	/* If nonNULL then pointer to local list of VHdr's */
  struct V4IM__AggLookAhead *ala ;	/* If nonNULL then points to current look-ahead structure */
  struct V4C__Context *ctx ;		/* Current context, NULL if none */
  struct V4V4IS__RuntimeList *rtl ;	/* If not NULL then points to loaded structure/element definitions */
  struct V4LEX__Table *vlt ;		/* Pointer to first (of possible chain) of V4 TABLE entries */
  struct V4DPI__UOMTable *uomt ;	/* Pointer to UOM Table(s) */
  struct lcl__Projection *lprc ;	/* Pointer to Projection Cache */
  struct V4HTML__Page *vhp ;		/* Pointer to HTML page info */
  struct V4OSH__Table *osh ;		/* Pointer (if not NULL) to OSHandle table */
  struct V4Tree__Master *TreeChain ;	/* Pointer to chain of V4 Tree structures */
  struct V4RPT__RptInfo *rri ;		/* If not NULL then chain of report structures */
  struct V4RPT__RptInfo *rriCur ;	/* If not NULL then 'current' (last one referenced) rri (for RCV() module) */
  DCLSPINLOCK msgLock ;			/* Spin-lock for updating pi->vme & pi->vml */
  int luseqNum ;			/* Last used unique message sequence number */
  int luuId ;				/* Last used unique listener id */
  int lIdDebug ;			/* If greater than 0 then listener Id used by V4(Debug::Web) */
  UCCHAR hostDebug[256] ;		/* Host/ip to send output if web debugging */
  int portDebug ;			/* Port number to send output if web debugging (0 if none) */
  USESKEY sesKeyDebug ;			/* Handshake session key if web debugging */
  struct V4Msg_Entry *vme ;		/* If not NULL then pointer to head of incoming Message() queue */
  struct V4Msg_Listener *vml ;		/* If not NULL then pointer to head of Message() listener queue */
#ifdef WINNT
  HANDLE hMsgWait ;			/* Handle used by Message(Next/Wait), listener threads signal this event when a message has been received */
#endif
  struct V4__MasterCustomTable *mct ;	/* Pointer to Customization structure */
					/* *** THESE POINTERS MUST BE SET AFTER mct is FINALIZED - THEY CANNOT MOVE !!! *** */
  struct V4CI__CountryInfo *ci ;	/* Pointer to Country Info */
  struct V4MSG__MsgInfo *mi ;		/* Pointer to Message Info */
  struct V4HTML_Entities *hent ;	/* Pointer to HTML entities (ex: "&nbsp;") */
  struct vIni__SectionEntry *vise ;	/* If not NULL then pointer to v4.ini sections */
  struct V4DPI__Flatten *ftn ;		/* If not NULL then pointer to first of chain of flatten structures */
  struct V4DPI__DimNameBlocks *dnb ;	/* If not NULL then pointer to local dimension name translation structure */
  struct V4DPI__RecogBlock *rcgb ;	/* If not NULL then pointer to first Recognition descriptor block */
  struct V4DPI__Point pntTest ;		/* Development test point - set/tested with V4(Test::xxxx / Test?) */
  INDEX TextAggCount ;			/* Number of text aggregates below */
  struct
   { UCCHAR *line ;			/* Pointer to current text line (NULL if empty) */
     struct V4LEX__Table *vlt ; 	/* Pointer to TABLE (NULL if empty) */
     int TextUId ;			/* Unique Id Assigned to this area */
     COUNTER Count ;			/* Current line counter */
   } TextAgg[V4C_AreaAgg_Max] ;
  INDEX dfltAreaAggX  ;			/* Default area index to use below (if DFLTAGGAREA then auto-determine at write time) */
  INDEX AreaAggCount ;			/* Number of entries below **NOTE: highest number, slot may be empty- pcb == NULL** */
  struct
   { struct V4IS__ParControlBlk *pcb ;	/* Pointer to pcb for the aggregate area */
     int AggUId ;			/* Unique Id Assigned to this area */
     struct V4DPI__LittlePoint aggPId ;	/* Unique Id assigned by user to this area */
     struct V4AGG__AggDims *vad ;	/* If not NULL then pointer to dimensions in aggregate flagged PointCreate Agg */
   } AreaAgg[V4C_AreaAgg_Max] ;
  struct V4LEX__Table *vltCur ;		/* If not NULL then pointer to current TABLE (used for xxx* parsing in macros) */
  UCCHAR xdbLastError[V4DPI_AlphaVal_Max+1] ;/* Last XDB/MYSQL error message */
  XDBID xdbIdLU ;			/* Last used db Id */
  struct V4XDB__Master *xdb ; 	/* If non-null then points to XDB master information */
  struct V4DPI__XDictRuntime *xdr ;	/* If not NULL then pointer to runtime XDict info (files, caches, etc.) */
#ifdef NEWMMM
  COUNTER aidLastUsed ;			/* Next free area id */
#endif
  struct V4MM__MemMgmtMaster *mmm ;	/* Pointer to main process mmm (sub-thread may have their own) */
  TLSINDEX mmmTLS ;			/* Thread-Local index to 'current' mmm */
//  struct
//   { 
//#ifdef V4ENABLEMULTITHREADS
//     DCLSPINLOCK areaLock ;		/* Multi-thread lock */
//#endif
//#ifdef NEWMMM
//     AREAID aid ;			/* Area id (if <= 0 then not in use) */
//     struct V4MM__MemMgmtMaster *mmm ;	/* Pointer to memory management structure for the area */
//     struct V4IS__AreaCB *acb ;		/* area control block for area */
//#endif
//   } Areas[V4MM_Area_Max] ;
  int NextUId ; 			/* Next Unique Id to assign below */
  int LowHNum ; 			/* Lowest RelHNum below */
  int HighHNum ;
  struct
   { int aid ;				/* Area Id for this heirarchy */
     struct V4IS__ParControlBlk *pcb ;	/* Parameter control block for this area */
     struct V4LEX__CompileDir *vcd ;	/* If not NULL then points to compile info for this hnum */
     struct V4C__AreaHInfo ahi ;	/* Information about this area */
     int AreaUId ;			/* Unique Id Assigned to this area */
     struct V4DPI__LittlePoint areaPId ; /* Unique Id assigned by user to this area */
     INDEX SlaveCnt ;			/* Number of slaves */
     int SlaveAids[10] ;		/* Area IDs for any slaves at this level */
   } RelH[V4C_CtxRelH_Max] ;
  COUNTER AggValueCount ;		/* Number of calls to AggValueGet */
  COUNTER AggValueRepeatCount ;		/* Out of above, number that are to same value as previous call */
  COUNTER XDictLookups,XDictCacheHits ;
  COUNTER PointMax ;			/* Max number points to enumerate in a Tally()/Enum() */
//  int CacheIsctLastId ; 		/* Last assigned isct id for cache-ing */
  COUNTER CacheCalls ;			/* Number of calls to Cache() intmod */
  COUNTER CacheHits ;			/*  and number of successful hits */
  COUNTER StartTraceOutput ;		/* If (trace & V4TRACE_LogIscts) <> 0 and IsctEvalCount > StartTraceOutput then output trace info */
  COUNTER traceProgressMin ;		/* If (trace & V4TRACE_Progress) <> 0 and listsize > traceProgressMin then output trace info */
					/* *** END OF POINTERS *** */
//#ifdef WINNT
//  HANDLE hEscThread ;			/* Handle to escape-handler thread */
//#endif
//  int EscFlag ; 			/* Indicator: UNUSED - suspend, 0 = waiting, +n = escape character */
  INDEX XMLIndent ;			/* Nesting of XML tags - used for indenting */
  LOGICAL NestMax ; 			/* TRUE if in Max-Nested Condition */
#ifdef V4_BUILD_LMTFUNC_EXP_DATE
  int Expired ; 			/* If 0 then V4 has expired - abort at any time! */
#endif
  LENMAX PointBytes[V4DPI_PntType_Last] ;	/* Size of point - indexed by point type (0 = dynamic) */
  clock_t StartClock ;			/* clock() value at start of process */
#ifdef UNIX
  struct timeval bgnTime ;
#endif
  time_t StartTime ;			/* Time this V4 process started */
#ifdef WINNT
  FILETIME ftCreate,ftExit,ftKernel1,ftUser1 ;		/* Start kernel/user CPU times */
#endif
} ;


#define V4IM_CatchExceptMax 16		/* Max number of exception (groups) per exception catching level */
#define V4IM_CatchExceptNameMax 16	/* Max number of exceptions names per group below */

struct V4IM__CatchExcept {		/* Exception handling structure */
 INDEX CatchCnt ;			/* Number sub-structures below */
 struct {
    BYTE excpartial[V4IM_CatchExceptNameMax] ; /* if > 0 then name below is partial (i.e. ended with "*") and this is number of characters */
    UCCHAR exceptionName[V4IM_CatchExceptNameMax][V4DPI_DimInfo_DimNameMax+1] ;
    struct V4DPI__Point excpnt ;	/* Execption point to evaluate */
  } Catch[V4IM_CatchExceptMax] ;
} ;

#define RETURNFAILURE \
  { if (!ctx->pi->mi->SetToThrow[gpi->mi->LastMsgIndex]) return(NULL) ;\
    return(v4im_ThrowLastErrorMnemonic(ctx,respnt,intmodx)) ;\
  }

/*	Return codes from v4eval_Eval() (low bits return value, high bits are flags) */

#define V4EVAL_Res_ExitNoStats 0	/* Exit with no statistics */
#define V4EVAL_Res_ExitStats 1		/* Exit with stats */
#define V4EVAL_Res_NestRetOK 2		/* Normal return from nested call */
#define V4EVAL_Res_NestRetErr 3		/* Error return from nested call - ctx->ErrorMsg with error */
#define V4EVAL_Res_BPContinue 4		/* Continue from break point */
#define V4EVAL_Res_BPStep 5		/* Continue with eval on 'next line' from break point */
#define V4EVAL_Res_BPStepInto 6		/* Continue with next eval from break point */
#define V4EVAL_Res_StopXct 7		/* Stop Execution */
#define V4EVAL_Res_BPStepNC 8		/* Continue with eval on 'next line' from break point BUT DON'T HONOR ANY ',xxx' on the intersection */

#define V4EVAL_ResMask 0xffff

#define V4EVAL_Res_Flag_Point 0x10000	/* Point result being returned in respnt */
#define V4EVAL_Res_Flag_ShowRes 0x20000	/* Want to show result of evaluation */
#define V4EVAL_Res_Flag_RetFail 0x40000	/* Return failure */

#define V4DBG_Breakpoint_Max 25		/* Max number of breakpoints */

#define V4DBG_bpFlag_Break 0x1		/* Break into interactive (if not set then just continue evaluations) */
#define V4DBG_bpFlag_Loc 0x2		/* Show current location (file:linenumber) */
#define V4DBG_bpFlag_Stack 0x4		/* Show execution stack */
#define V4DBG_bpFlag_WallCPU 0x8	/* Show connect time & cpu time */
#define V4DBG_bpFlag_DeltaWallCPU 0x10	/* Show delta connect time & cpu time (from last break) */
#define V4DBG_bpFlag_Isct 0x20		/* Show current intersection to be evaluated */
#define V4DBG_bpFlag_EvalRes 0x40	/* Show result of intersection evaluation */
#define V4DBG_bpFlag_EvalCount 0x80	/* Show current evaluation count */
#define V4DBG_bpFlag_Context 0x100	/* Show context */

#define V4DBG_bpFlag_BreakOnFail 0x10000 /* Break on isct failure */
#define V4DBG_bpFlag_DoNotEval 0x20000	/* Don't evaluate isct at break (can't have with BreakOnFail) */
#define V4DBG_bpFlag_OneTime 0x40000	/* This is one-time break, delete after breakpoint handling */
#define V4DBG_bpFlag_FromV4 0x80000	/* Break generated by V4(Break?) module */
#define V4DBG_bpFlag_Disable 0x100000	/* Break point is disabled */

#define V4DBG_bpFlag_Default (V4DBG_bpFlag_Break | V4DBG_bpFlag_Loc | V4DBG_bpFlag_EvalCount)

#define INITBP(VBP) \
  memset((VBP),0,sizeof (struct V4DBG__BreakPoint)) ;

struct V4DBG__BreakPoint {
  struct V4DPI__Point *condPt ;		/* Only break if evaluation of this results in TRUE */
  struct V4DPI__Point *evalPt ;		/* Evaluate this point on break */
  struct V4DPI__Point *resPt ;		/* Evaluate this point on break and return as result (instead of isct at break) */
  union V4DPI__IsctSrc vis ;		/* Source-file / line number to break on (if non-zero) */
  int rtStackX ;			/* If not zero, only break if current rtStackX = this value AND ... */
  int lineNumber ;			/*    the current line number <> this */
  INTMODX intmodX ;			/* If not zero, only break if current isct is this intmod */
  BRKPTID bpId ;			/* If not zero then breakpoint Id */
  FLAGS32 bpFlags ;			/* Misc. flags effecting breakpoint - see V4DBG_bpFlag_xxx */
  COUNTER isctEvals ;			/* Only break after this many evaluations */
} ;

struct V4DBG__BreakPointList {
  union V4DPI__IsctSrc visSource ;	/* If non-zero then hnum & src file index of 'current' source file */
  BRKPTID luId ;			/* Last used breakpoint Id */
  INDEX rtStackXStep ;			/* If not UNUSED then rtStack level for 'step' breakpoints */
  INDEX curcallSkip ;			/* Don't check breakpoints if curcall == curcallSkip (for step'ing) */
  INDEX Count ;
  struct V4DBG__BreakPoint vbp[V4DBG_Breakpoint_Max] ;
} ;


#define V4OSH_Max 255			/* Max number of active OSH entries supported */
#define V4OSH_Type_SubProc 1		/* A sub-process */
#define V4OSH_Type_Thread 2		/* A thread */
#define V4OSH_Type_Fiber 3		/* A fiber */

#define V4OSH_Status_Unknown 0		/* Unknown status */
#define V4OSH_Status_Active 1		/* An active process */
#define V4OSH_Status_Done 2		/* A terminated process (Result contains result code) */

struct V4OSH__Table {			/* Sub-Process Table to track all Spawn(Wait::No)'ed OSHrocesses */
  INDEX Count ;				/* Number of currently active processes below */
  int NextAvailRef ;			/* Next available unique process ref (returned as value in OSHandle point) */
  struct {
   int Ref ;				/* Unique reference for this process */
   int Status ; 			/* Status of the entry - V4OSH_Status_xxx */
   int Result ; 			/* A result code */
   int Type ;				/* What is it- V4OSH_Type_xxx */
#ifdef WINNT
    HANDLE hProcess,hThread ;		/* Windows handle for process & thread */
#endif
   } Entry[V4OSH_Max] ;
} ;

/*	M E S S A G E   S T R U C T U R E S	*/

#define MESSAGE_EOM 26			/* End-of-message character */
#define MESSAGE_EOMS "\x1A"		/* End-of-message string */
#define MESSAGE_UCEOMS UClit("\x1A")	/* End-of-message string */

#define MESSAGE_EOD 25			/* Delimits columns/fields within message */
#define MESSAGE_EODC '\x19'
#define MESSAGE_UCEODC UClit('\x19')
#define MESSAGE_EODS "\x19"
#define MESSAGE_UCEODS UClit("\x19")

struct V4Msg_Entry {				/* Message Entry */
  struct V4Msg_Entry *vmeNext ;			/* If not NULL then pointer to next message */
  struct V4Msg_Listener *vml ;			/* Listener that generated this message */
//  UCCHAR ackVal[128] ;				/* Acknowledgement sent back to sender on this message */
  UCCHAR sndrId[V4DPI_DimInfo_DimNameMax+1] ;	/* Sender's Id */
  UCCHAR sndrIPAddressPort[128] ;		/* Sender's IP address */
  UCCHAR lstnrId[V4DPI_DimInfo_DimNameMax+1] ;	/* Listener's Id */
  UCCHAR msgDimName[V4DPI_DimInfo_DimNameMax+1] ; /* Dimension of message contents */
  UCCHAR msgVal[V4DPI_AlphaVal_Max] ;		/* Message contents */
  CALENDAR msgRcvCal ;				/* When message received (Calendar value, not UDT) */
  LOGICAL msgOK ;				/* If TRUE then have valid message, if FALSE then msgVal is error */
  CALENDAR delCalDT ;				/* If non-zero then delivery date-time (CALENDAR/GMT value) */
  int uId ;					/* Unique listener Id */
  int seqNum ;					/* Unique message sequence number */
  int connectSocket ;				/* If not UNUSED then socket (sender) expecting a reply */
} ;

#define V4Msg_Status_Starting 1			/* Starting up */
#define V4Msg_Status_Listening 2		/* Listening (normal state) */
#define V4Msg_Status_Error 3			/* Error State (see vml->msgBuf for error) */
#define V4Msg_Status_Terminate 4		/* If set then thread terminates at next appropriate time */
#define V4Msg_Status_Done 5			/* Listener process is no longer running */

#define V4MsgL_Action_Queue 0			/* Queue message on receipt (default) */
#define V4MsgL_Action_Ctx 1			/* Immediately inject message (as point) into context */

#define VMultiCast_TTL_Host 0			/* Datagram restricted to same host */
#define VMultiCast_TTL_Subnet 1			/* Same subnet */
#define VMultiCast_TTL_Site 32			/* Same site */
#define VMultiCast_TTL_Region 64		/* Same region */
#define VMultiCast_TTL_Continent 128		/* Same continent */
#define VMultiCast_TTL_Unrestricted 255		/* Unrestricted in scope */

#define VML_Default_Port 4000			/* Default port number for listener thread */

struct V4Msg_Listener {				/* Info for Message listener thread */
  struct V4Msg_Listener *vmlNext ;		/* If not NULL then pointer to next listener structure */
//  struct V4DPI__Point ackPnt ;			/* Evaluate this each time to send ack back to sender */
#ifdef WINNT
  HANDLE idThread ;		/* The thread this listener is running as */
#endif
#if defined LINUX486 || defined RASPPI
  pthread_t idThread ;				/*   ditto */
#endif
  struct V4C__Context *ctx ;			/* Context associated with this listener */
  UCCHAR lstnrId[V4DPI_DimInfo_DimNameMax+1] ;	/* Listener's Id */
  UCCHAR spawnCmdLine[V4DPI_AlphaVal_Max] ;	/* Optional command line to spawn listener process */
  UCCHAR Host[256] ;				/* Host name (or group name if multicasting - see ttl) */
  int mqlDimId ;				/* Dimension in context to update with message queue length */
  int ttl ;					/* Time-to-live parameter for mulitcasting (UNUSED if not multicasting - default) */
  int status ;					/* Status of this listener - V4Msg_Status_xxx */
  SOCKETNUM socket ;
  SOCKETPORT port ;				/* Listening on this port */
  COUNTER msgCount ;				/* Number of messages received */
  int uId ;					/* Unique listener Id */
  ETYPE vmlAction ;				/* Action to take on receipt of message: V4MsgL_Action_xxx */
} ;

#define V4IM_LocalVHdrMax 256		/* Max number of headers to keep in process */
#ifdef V4ENABLEMULTITHREADS
#define V4IM_AggStreamMax 8
#else
#define V4IM_AggStreamMax 1
#endif

struct V4IM__LocalVHdr
{ INDEX Count ;				/* Number below */
  struct {
    DIMID Dim ;				/* The dimension for this entry */
    LOGICAL Dirty ; 			/* If true then aggs below has been updated & needs to be flushed */
    ETYPE EntryKeyMode ;		/* Type of key: V4IS_KeyMode_xxx */
    INDEX areaX ;			/* Index into area to write to (DFLTAGGAREA for default) */
    B64INT biKeyLastPut ; 		/* Last big-int key written to agg (to ensure in increasing order!) */
    struct V4IM__AggVHdr *avh ; 	/* The indexes for the data */
    struct {
      INDEX OffsetToFirst ; 		/* recordptr = aggs->Buffer[OffsetToFirst + (recnum-1)*BytesPerEntry */
      INDEX AggIndex ;			/* Aggregate index number (for currently loaded aggregates) */
      COUNTER CurrentRecNum ; 		/* Current record number being created/accessed */
      struct V4IM__AggShell *aggs ;	/* If nonNULL then current block/buffer */
      struct V4IM__AggLookAhead *ala ;	/* If nonNULL then points to look-ahead structure for pre-loaded agg blocks (multi-threading environment) */
     } Stream[V4IM_AggStreamMax] ;	/* Support for concurrent Aggregate streams */
   } Entry[V4IM_LocalVHdrMax] ;
} ;

#ifdef V4ENABLEMULTITHREADS
#define STREAM Stream[ctx->aggStream]	/* Define STREAM to access proper entry */
#else
#define STREAM Stream[0]		/*   or just grab first(last) if not multi-threading */
#endif


/*	V4IS__V4Areas - Holds info on V4IS areas being accessed by V4 */

#define V4IS_V4AreaMax 20
#define V4IS_V4Key_Max 5		/* Track max number of keys per area */
#define V4IS_V4KeyP_Max 5		/* Max number of parts to a key (s/b odd number) */
struct V4IS__V4Areas
{ INDEX Count ;				/* Number open below */
  int LUHandle ;			/* Last used handle below */
  struct
   { int Handle ;			/* Unique handle (used in point to reference) */
     int FileRef ;			/* FileRef associated with this area */
     DIMID DimId ;			/* Dimension associated with this area */
     DATAID DataId ;			/* Current DataId */
     DATAID DataId2 ;			/* (optional second id for VMS systems) */
     BYTE *RecBuf ;			/* Pointer to record buffer */
     COUNTER Count ;			/* Total number of points (including substructures), used for List maximum */
     COUNTER SubIndex ;			/* Current sub-index for use in list access */
     struct V4IS__ParControlBlk *pcb ;	/* Pointer to parameter control block for area */
     struct V4V4IS__StructEl *stel ;	/* Pointer to associated structure/element info */
     struct {
       short KeyParts ; 		/* Number of key parts for this key */
       short KeyP[V4IS_V4KeyP_Max] ;	/* Field index (into stel) for the part of the key */
      } Key[V4IS_V4Key_Max] ;
   } Area[V4IS_V4AreaMax] ;
} ;

/*	B E F O R E  -	A F T E R   C O N D I T I O N	S T U F F	*/

#define V4IM_BaA_CondMax 15		/* Max number of conditions */

#define NEWBAFSTUFF
#ifdef NEWBAFSTUFF
#define V4IM_BaA_LevelMax 32
#define V4IM_BaA_LevelDfltMax 4
#define V4IM_BaA_ColMax 2048
#define V4IM_BaA_ColDfltMax 100
#define V4IM_BaA_CalcMax 100
#define V4IM_BaA_CalcDfltMax 4
#else
#define V4IM_BaA_ColMax 150		/* Max number of columns */
#define V4IM_BaA_CalcMax 15		/* Max number of invisible calculating columns */
#define V4IM_BaA_LevelMax 8		/* Max number of nested totals (1=Grand Total) */
#endif

#define V4IM_BaA_Disable 0		/* Not in a condition */
#define V4IM_BaA_CondBegin 1		/* Begin of "report" */
#define V4IM_BaA_CondBefore 2		/* Before a change */
#define V4IM_BaA_CondAfter 3		/* After a change */
#define V4IM_BaA_CondEnd1 4		/* End of "report" - do all Before/After recaps */
#define V4IM_BaA_CondEnd2 5		/* End of "report" - do grand totals */

struct V4IM__BaA			/* Master Before-and-After Structure */
{ INDEX CurCol ;			/* Current column number */
  INDEX CurCalc ; 			/* Current Calculating "column" */
  INDEX CurRow ;			/* Current row (if necessary!) */
  INDEX CurLevel ;			/* Current (last) recap level */
  INDEX CurCond ; 			/* Current condition being tested/executed (0 if none) */
  P CurPt ;				/* Current context point */
  INDEX Count ;				/* Number of conditions below */
  struct {
    ETYPE CondCode ;			/* Condition code: V4IM_BaA_Condxxx */
    P TestPt ;				/* Test point */
    P DoPt ;				/* Point to be executed on change of test point */
    P TestRes ; 			/* Result of test point (for comparison to next) */
  } Cond[V4IM_BaA_CondMax] ;

#ifdef NEWBAFSTUFF
  COUNTER baaLevelMax ;			/* Number of recapping levels currently allocated (default is V4IM_BaA_LevelDfltMax) */
  LENMAX baaColMax ;			/* Make number of columns (default is V4IM_BaA_ColDfltMax) */
  LENMAX baaCalcMax ;			/* Make number of calc's (default is V4IM_BaA_CalcDfltMax) */
  struct V4IM__BaAColCalcInfo *ccInfo ;
  struct V4IM__BaATotals *ccTot[V4IM_BaA_LevelMax] ;
					/* Absolute max, but only allocate what will be used for current Enum
					   All ccInfo and ccTot levels allocated with single call to v4mm_AllocChunk() therefore only want to free pointer to ccInfo */
#else
  struct {
    double SubTotals[V4IM_BaA_LevelMax] ; /* Holds subtotals for each level */
    B64INT fSubTotals[V4IM_BaA_LevelMax] ; /* Holds (fixed) subtotals for each level */
    unsigned short Dim ;		/* Dimension if UOM/Fixed */
    short Decimals ;			/* If not UNUSED then decimals of fSubTotal */
    short Ref ; 			/* If > 0 then Unique UOM Number/Id/Ref */
    short Index ;			/* If > 0 then Index within UOM */
    COUNTER Count[V4IM_BaA_LevelMax] ;	/* Holds counts for each level */
  } Col[V4IM_BaA_ColMax] ;
  struct {
    double SubTotals[V4IM_BaA_LevelMax] ; /* Holds subtotals for each level */
    COUNTER Count[V4IM_BaA_LevelMax] ;	/* Holds counts for each level */
  } Calc[V4IM_BaA_CalcMax] ;
#endif
} ;

#ifdef NEWBAFSTUFF
struct V4IM__BaATotals {		/* This is allocated ccTot[level] = allocate((sizeof(struct V4IM__BaATotals) * (baaColMax + baaCalcMax)) */
 struct { 
    double dblSubTotal ;
    B64INT fixSubTotal ;
    COUNTER count ;
   } colCalc[0] ;			/* This is allocated to be (baaColMax + baaCalcMax) */
} ;

struct V4IM__BaAColCalcInfo {
  struct {
    unsigned short dim ;		/* Dimension if UOM/Fixed */
    short decimals ;			/* If not UNUSED then decimals of fSubTotal */
    short ref ; 			/* If > 0 then Unique UOM Number/Id/Ref */
    short index ;			/* If > 0 then Index within UOM */
  } colCalc[0] ;			/* This is allocated to be (baaColMax + baaCalcMax) */
} ;
#endif

/*	X D B   ( E x t e r n a l   D a t a b a s e )   I n f o r m a t i o n 	*/

#define XDBODBC 0			/* 0 is ODBC access */
#define XDBMYSQL 1			/* 1 is MYSQL access */
#define XDBV4IS 2			/* 2 is PSUEDO SQL access to MIDAS V4IS */
#define XDBORACLE 3			/* 3 is ORACLE native mode access */

#define XDBCONNECTION 0
#define XDBSTATEMENT 1

#define XDBNEWID(_id,_access,_type) \
  _id = (++gpi->xdbIdLU) ; XDBSETDBACCESS(_id,_access) ; \
  switch(_type) { case XDBCONNECTION: XDBSETCONNECTION(_id) ; break ; case XDBSTATEMENT: XDBSETSTMT(_id) ; break ; } ;
#define XDBIDPART(_id) ((_id) & 0x0fffffff)
#define XDBISCON(_id) ((_id) < 0)	/* Connections are always < 0 */
#define XDBISSTMT(_id) ((_id) > 0)
#define XDBSETCONNECTION(_id) _id |= 0x80000000
#define XDBSETSTMT(_id) _id &= 0x7fffffff
#define XDBGETDBACCESS(_id) (((_id) >> 28) & 0x7)
#define XDBSETDBACCESS(_id,_access) (_id) &= 0x8fffffff ; (_id) |= (((_access)&7) << 28) ;

#define XDB_FREE_StmtOnly 1		/* On call to FreeStuff() only free statements even if handle is for connection */

#define V4XDB_ConnectMax 25
#define V4XDB_StmtMax 50
#define V4XDB_StmtColMax 255
#define V4ODBC_ConnectRetryMax 8	/* Maximum number of times to retry ODBC connect before generating V4 failure */
#define V4ODBC_ConnectRetryWait 250	/* Number of milliseconds to wait between retries */

#define VDTYPE_b1SInt 0			/* One byte signed integer */
#define VDTYPE_b2SInt 1			/* Two byte signed integer */
#define VDTYPE_b4SInt 2			/* Four byte signed integer */
#define VDTYPE_b8SInt 3			/* Eight byte signed integer */
#define VDTYPE_b4FP 4			/* Four byte floating point */
#define VDTYPE_b8FP 5			/* Eight byte floating point */
#define VDTYPE_Char 6			/* 8-bit character string */
#define VDTYPE_UCChar 7			/* Unicode character string */
#define VDTYPE_UTF8 8			/* 8-bit Unicode encoding via UTF8 (lenval should be bytes, not characters!) */
#define VDTYPE_ODBC_DS 9		/* ODBC DATE_STRUCT */
#define VDTYPE_ODBC_TS 10		/* ODBC TIME_STRUCT */
#define VDTYPE_ODBC_TSS 11		/* ODBC TIMESTAMP_STRUCT */
#define VDTYPE_ODBC_SNS 12		/* ODBC SQL_NUMERICL_STRUCT */
#define VDTYPE_MYSQL_TS 13		/* MYSQL MYSQL_TIME STRUCT */
#define VDTYPE_MIDAS_Log 14		/* Text '0' or '1' */
#define VDTYPE_MIDAS_UDT 15		/* MIDAS internal UDT as ASCII */
#define VDTYPE_MIDAS_Char 16		/* Character */
#define VDTYPE_MIDAS_Int 17		/* Integer in ASCII format */
#define VDTYPE_MIDAS_Real 18		/* Real in ASCII format */
//#define VDTYPE_MIDAS_UDate 19		/* MIDAS internal UDate as ASCII */
//#define VDTYPE_MIDAS_Zip 20		/* MIDAS zipcode (may be blank, 5 digits, 5 digits+'0000', 9 digits) */
#define VDTYPE_NEWDEC 21		/* MYSQL New-Decimal format */
#define VDTYPE_ORACLE_TS 22		/* ORACLE OCI_Date STRUCT */
#define VDTYPE_ORACLE_NUMERIC 23	/* ORACLE Numeric */
#define VDTYPE_ORACLE_TEXT 24		/* ORACLE Text */

#define V4ODBC_MIDAS_UNIQUE_HANDLE_START 0xfffffff
#define V4MYSQL_DFLT_DATALEN 2048


/*	Updates stmts[SX].curRecId with next record id (based on targetDim) */
#define GETNEXTRECID(_XDB,SX) \
  if ((_XDB)->stmts[SX].targetDimId == UNUSED) { (_XDB)->stmts[sx].curRecId++ ; } \
   else { struct V4XDB__SaveStmtInfo *vxssi = (_XDB)->stmts[SX].vxssi ; \
	  if (vxssi->valCol == UNUSED) { struct V4DPI__DimInfo *di ; DIMINFO(di,ctx,(_XDB)->stmts[SX].targetDimId) ; (_XDB)->stmts[sx].curRecId = (++di->XDB.lastRecId) ; } \
	   else { struct V4DPI__LittlePoint lpnt ; \
		  v4db_GetCurColVal(ctx,SX,vxssi->valCol,(_XDB)->stmts[SX].targetDimId,(P *)&lpnt) ; (_XDB)->stmts[sx].curRecId = lpnt.Value.IntVal ; \
		} ; \
	}
//#define GETNEXTRECID(_XDB,SX) \
//  if ((_XDB)->stmts[SX].targetDimId == UNUSED) { (_XDB)->stmts[sx].curRecId++ ; } \
//   else { struct V4XDB__SaveStmtInfo *vxssi = (_XDB)->stmts[SX].vxssi ; \
//	  if (vxssi->valCol == UNUSED) { struct V4DPI__DimInfo *di ; DIMINFO(di,ctx,(_XDB)->stmts[SX].targetDimId) ; (_XDB)->stmts[sx].curRecId = (++di->XDB.lastRecId) ; } \
//	   else { struct V4DPI__LittlePoint lpnt ; \
//		  v4db_GetCurColVal(ctx,SX,vxssi->valCol,(_XDB)->stmts[SX].targetDimId,(P *)&lpnt) ; (_XDB)->stmts[sx].curRecId = lpnt.Value.IntVal ; \
//		  if (vxssi->lp != NULL) v4l_ListPoint_Modify(ctx,vxssi->lp,V4L_ListAction_Append,(P *)&lpnt,0) ; \
//		} ; \
//	}


/*	Removes DIM from list of dimensions to watch, closes off vxssi->lp if we have one */
#define XDBRMVTARGETDIM(SX) \
 if (xdb->stmts[SX].targetDimId == UNUSED ? FALSE : gpi->xdb->vxsri != NULL) \
  { struct V4XDB__SaveRowInfo *vxsri = gpi->xdb->vxsri ; INDEX _i ; \
    for(;!GETSPINLOCK(&vxsri->sriLock);) {} ; \
    for(_i=0;_i<vxsri->dimCount;_i++) { if (vxsri->dimList[_i].dimId == xdb->stmts[SX].targetDimId) break ; } ; \
    if (_i < vxsri->dimCount) \
     { INDEX _j = _i+1 ; for(;_j < vxsri->dimCount;_j++) { vxsri->dimList[_i++] = vxsri->dimList[_j] ; } ; vxsri->dimCount-- ; \
       memset(&vxsri->dimHash,0,V4XDB_SaveInfo_HashMax) ; \
       for(_i=0;_i<vxsri->dimCount;_i++) { vxsri->dimHash[vxsri->dimList[_i].dimId & (V4XDB_SaveInfo_HashMax - 1)] = 1 ; } ; \
     } ; \
    if (xdb->stmts[sx].vxssi->lp != NULL) { ENDLP(xdb->stmts[sx].vxssi->lpnt,xdb->stmts[sx].vxssi->lp) ; } ; \
    RLSSPINLOCK(vxsri->sriLock) ; \
  } ;


//VEH141114 - added '> 0' in two places to _save test below
#define XDBSAVECURROW(XDB,SX) \
  if ((XDB)->vxsri != NULL && (XDB)->stmts[SX].curRow > 0) \
   { struct V4XDB__SaveRowInfo *vxsri = (XDB)->vxsri ; LOGICAL _save=FALSE ; \
     for(;!GETSPINLOCK(&vxsri->sriLock);) {} ; \
     if ((XDB)->stmts[SX].targetDimId == UNUSED ? FALSE : (vxsri->dimHash[(XDB)->stmts[SX].targetDimId & (V4XDB_SaveInfo_HashMax - 1)])) \
      { INDEX i ; \
	for(i=0;i<vxsri->dimCount;i++) \
	 { if (vxsri->dimList[i].dimId == (XDB)->stmts[SX].targetDimId) \
	    { _save = (vxsri->dimList[i].explicitSave == UNUSED ? (vxsri->dimList[i].saveRow > 0 || vxsri->dimList[i].saveAllRows > 0) : vxsri->dimList[i].explicitSave) ; \
	      vxsri->dimList[i].saveRow = FALSE ; vxsri->dimList[i].explicitSave = UNUSED ; break ; \
	    } ; \
	 } ; \
      } ; \
     RLSSPINLOCK(vxsri->sriLock) ; \
     if (_save) { v4xdb_SaveXDBRow(ctx,(XDB),SX) ; } ; \
   } ;



/*	Sets internal flag that row associated with POINT is to be saved (i.e. XDBISDIMSAVE() will return TRUE) */
#define XDBSAVEROW(POINT)  \
 if (gpi->xdb == NULL ? FALSE : gpi->xdb->vxsri != NULL) \
  { struct V4XDB__SaveRowInfo *vxsri = gpi->xdb->vxsri ; \
    for(;!GETSPINLOCK(&vxsri->sriLock);) {} ; \
    if (vxsri->dimHash[(POINT)->Dim & (V4XDB_SaveInfo_HashMax - 1)]) \
     { INDEX _i ; for(_i=0;_i<vxsri->dimCount;_i++) { if (vxsri->dimList[_i].dimId == (POINT)->Dim) { vxsri->dimList[_i].saveRow = TRUE ; break ; } ; } ; \
     } ; \
    RLSSPINLOCK(vxsri->sriLock) ; \
  } ;

#define V4XDB_SaveInfo_DimMax 64		/* Max number of dimensions to check on */
#define V4XDB_SaveInfo_HashMax 128		/* Size of hash table (MUST BE POWER OF 2) */
struct V4XDB__SaveRowInfo {
  DCLSPINLOCK sriLock ;				/* Multi-thread lock */
  BYTE dimHash[V4XDB_SaveInfo_HashMax] ;
  COUNTER dimCount ;
  struct {
    struct V4DPI__Point idPnt ;			/* Id associated with this statement */
    DIMID dimId ;				/* (target) Dimension associated with the statement */
						/* NOTE: The next two fields below get set to FALSE with each row */
    LOGICAL saveRow ;				/* If TRUE then save current row associated with this dimension */
    ETYPE explicitSave ;			/* If not UNUSED then T/F to save current row (overrides saveRow if not UNUSED) */
    LOGICAL saveAllRows ;			/* If TRUE then save all rows (overridden with explicitSave) */
   } dimList[V4XDB_SaveInfo_DimMax] ;
} ;

struct V4XDB__Master			/* Master XDB structure (linked to gpi) */
{ 
  DCLSPINLOCK xdbLock ;			/* Multi-thread lock */
  int MIDASluHandle ;			/* Last used MIDAS hdbc/hstmt handles (MUST be unique from ODBC assigned!) */
#ifdef WANTODBC
  HENV hEnv ;				/* ODBC environment handle */
#endif
  struct V4XDB__SaveStmtInfo *vxssi ;	/* If not NULL then pointer to first saved-statement information block */
  char extraData[V4DPI_AlphaVal_Max+1] ;/* Extra data passed back on last SQL call */
  struct V4XDB__SaveRowInfo *vxsri ;	/* If not NULL then pointer to save-row dimension structure */
  int LastErrNum ;			/* Last error number */
  COUNTER conCount ;			/* Highest connection index */
  struct {
    XDBID xdbId ;			/* Unique id for this connection (always negative) (UNUSED = empty slot) */
    LOGICAL resetCon ;			/* If TRUE then reset connection if necessary (e.g. to recover from mysql out-of-synch problem) */
    LOGICAL isActive ;			/* TRUE if connection is processing a stream of data (ex: pulling rows from a get) */
    struct V4DPI__LittlePoint idPnt ;	/* Optional Id::xxx point for this connection */
    DIMID DimId ; 			/* Host dimension for this connection */
    UCCHAR *ConnectionStr ;		/* If connected via connection string then full string is saved here */
#ifdef WANTODBC
    HDBC hdbc ; 			/* Connection handle */
#endif
#ifdef WANTV4IS
    MIDASODBCHANDLE mhdbc ;		/* MIDAS handle */
    SOCKETNUM MIDASSocket ;		/* If not UNUSED then socket for a MIDAS/ODBC connection */
#endif
#ifdef WANTMYSQL
    MYSQL *mysql ;
    UCCHAR *host, *dsn, *user, *password ;
    int port ;
    LOGICAL autocom ;			/* Save connect params in case we need to reconnect */
#endif
#ifdef WANTORACLE
    OCI_Connection *oracle ;
    UCCHAR lastErrMsg[0x400] ;		/* Last error message */
    int lastErrCode ;			/*  and error number */
#if !defined WANTMYSQL
    UCCHAR *host, *dsn, *user, *password ;
    int port ;
    LOGICAL autocom ;			/* Save connect params in case we need to reconnect */
#endif
#endif
   } con[V4XDB_ConnectMax] ;
#ifdef WANTORACLE
    INDEX lastCX ;			/* Set to last con[] index (used by error trap routine) */
#endif
  int stmtCount ; 			/* Number of active statements */
  struct {
    DIMID dimId ;			/* Dimension associated with this statement (and connection) */
    struct V4DPI__LittlePoint idPnt ;	/* Optional Id::xxx point for this connection */
    DIMID targetDimId ;			/* If not UNUSED then dimension of xdbGet() points */
    XDBID xdbId ;			/* Unique id for this statement (always positive) (UNUSED = empty slot) */
    COUNTER hxdbId ;			/* Unique id for connection of this statement */
    P *fetchPnt ;			/* If not NULL then point to be evaluated after each row is fetched */
    P *eofPnt ;				/* If not NULL then point to be evaluated on 'eof' of this statement */
    struct V4XDB__SaveStmtInfo *vxssi ;	/* If not NULL then pointer to save-statement structure */
    LOGICAL relConEOF ;			/* If TRUE then release connection on EOF (this statement is always released on EOF) */
#ifdef WANTODBC
    SQLLEN rowCnt ;
#else
    INDEX rowCnt ;			/* Number of rows selected/updated (UNUSED if unknown) */
#endif
    LENMAX dataLen ;			/* Length of data buffer */
    BYTE *data ;			/* Allocated buffer for all columns returned by this statement (NOT SO FOR MIDAS ACCESS - this is updated within major buffer) */
    XDBRECID curRecId ;			/* Current record Id for V4 point (see di->XDB) (NOTE: SEE NEXT FIELD) */
    XDBROW curRow ;			/* Current "Fetch" row */
#ifdef WANTODBC
//    XDBROW CurRow ;			/* Current "Fetch" row */
    HSTMT hstmt ;			/* Statement handle */
    HDBC hdbc ; 			/* Connection handle */
#endif
#if defined WANTODBC || defined WANTV4IS
    int mstmt ;				/* MIDAS statement */
#endif
#ifdef WANTV4IS
    MIDASODBCHANDLE mhdbc ;
    struct V4XDB_MIDASTable *vmt ;	/* If not NULL then points to MIDAS table (data will be NULL in this case) */
#endif
#ifdef WANTMYSQL
    MYSQL_STMT *stmt ;
    MYSQL_RES *res ;
    MYSQL_FIELD *fld ;
    MYSQL_BIND bind[V4XDB_StmtColMax] ;
#endif
#ifdef WANTORACLE
    OCI_Statement *st ;
    OCI_Resultset *rs ;
#endif
    COUNTER colCount ;			/* Total number of columns */
    struct {
//      PNTTYPE V4PntType ;		/* Target point type: V4DPI_PntType_xxx */
      LENMAX length ;			/* Max number of bytes in this column */
//      DIMID dimId ;			/* Default dimension for column */
      ETYPE vdType ;			/* VDTYPE_xxx for this column */
      INDEX Offset ;			/* Offset into column buffer (data) */
#ifdef WANTODBC
      SQLLEN UBytes ;			/* Updated number of bytes */
      short SQLCType ;		 	/* Target type: SQL_C_xxx */
#endif
#ifdef WANTMYSQL
      my_bool isNull;			/* Set to TRUE if column value is null */
      my_bool isErr;			/* Set to TRUE if column has error */
#endif
#ifdef WANTORACLE
      int isNull;			/* Set to TRUE if column value is null */
      int isErr;			/* Set to TRUE if column has error */
#endif
#ifdef WANTORACLE
      ETYPE ociCDT ;			/* Data type (OCI_CDT_xxx) as returned by OCI_ColumnGetType() */
#endif
     } col[V4XDB_StmtColMax] ;
   } stmts[V4XDB_StmtMax] ;
} ;

#define SIZEOFVXSSI(vxssi) (((char *)&(vxssi)->col[(vxssi)->colCount]) - ((char *)(vxssi)))
struct V4XDB__SaveStmtInfo {
  struct V4XDB__SaveStmtInfo *vxssiNext ; /* Link to next one (NULL if last) */
  struct V4XDB_SavedRow *vxsr ;		/* If not NULL then pointer to buffer for saving row information in temp Area */
  struct V4DPI__LittlePoint idPnt ;	/* Id point */
  XDBID xdbId ;				/* Statement this corresponds to */
  DIMID dimId ;				/* XDB dimension associated with this info */
  XDBRECID firstRecId,lastRecId ;	/* First & last record Ids in this selection (NOT VALID IF valCol != UNUSED) */
  INDEX valCol ;			/* If not UNUSED then column to use as 'curRecId' (in stmts[sx]) */
  struct V4L__ListPoint *lp ;		/* If not NULL then lp to list of points gotten in this statement */
  struct V4DPI__Point *lpnt ;		/* If lp not NULL then lpnt is allocated point that lp refers to */
  LENMAX colCount ;			/* Number of columns in this statement */
  struct {
    ETYPE vdType ;			/* Data type of this column (VDTYPE_xxx) */
    } col[V4XDB_StmtColMax] ;
} ;

struct V4XDB_SavedRow {
  union V4IS__KeyPrefix kp ;		/* Type=xdbRow, AuxVal=dimension, Mode=Int, Length=V4IS_xdbRow_Bytes */
  XDBRECID recId ;
  BYTE data[V4Area_UsableBytes] ;
} ;

#if defined WANTODBC || defined WANTMYSQL || defined WANTORACLE || defined WANTV4IS
#define V4ODBC_FetchNext ((XDBROW)-1)	/* Fetch next row in statement/access */
#define V4ODBC_FetchFirst ((XDBROW)-2)	/* Fetch first row */
#define V4ODBC_FetchLast ((XDBROW)-3)	/* Fetch last row */
#define V4ODBC_FetchEOF ((XDBROW)-4)	/* To force EOF & close off statement */
					/* +n to fetch row n */

#define V4XDB_InitialMIDASDatBuf 8096	/* Initial number of bytes to allocate to data buffer */

#define MIDAS_EOC '\t'		/* End-of-Column */
#define MIDAS_EOL '\r'		/* End-of-Line */
#define MIDAS_EOM 26			/* End-of-Message */

struct V4XDB_MIDASTable		/* Structure of MIDAS/ODBC returned data set */
{ LENMAX databufbytes ;			/* Max bytes in data buffer */
  BYTE *databuf ;			/* Pointer to current data buffer (points to header, data starts on row 2) */
  INDEX RowCount ;			/* Maximum rows returned (UNUSED if none currently in buffer) */
  INDEX currowx ; 			/* Row currow currently points to (UNUSED if not set) */
  BYTE *currow ;			/* Pointer to current row */
  INDEX colCount ;			/* Number of defined columns */
  struct {
//   unsigned short V4PntType ;		/* Source point type: V4DPI_PntType_xxx  (but data is actually text) */
   char Name[V4DPI_DimInfo_DimNameMax] ;/* Column name */
  } Col[V4XDB_StmtColMax] ;
} ;
#endif /* ODBC */

#define V4HTML_EntityMaxSize 150	/* Max size of HTML entity structure */
struct V4HTML_Entities {
  int startIndex[128] ;			/* Starting index for entities below (offset by ASCII value of first byte) */
  UCCHAR charVal[V4HTML_EntityMaxSize] ;	/* Character value (this array MUST be sorted) */
  INDEX entryX[V4HTML_EntityMaxSize] ;	/* Index into entry[] below for this character */
  int count ;				/* Number of entries */
  struct {
     int numVal ;			/* Numeric value of entity */
     UCCHAR alphaVal[10] ;		/* Alpha-numeric value */
   } entry[V4HTML_EntityMaxSize] ;
} ;

/*	Z I P   S T R U C T U R E S		*/

#define VZIP_MaxBytesExtra 1024

#pragma pack(2)
struct VZIP__zipFileHeader {	/* See http://en.wikipedia.org/wiki/ZIP_(file_format) for details */
  int hdrSignature ;		/* 0x04034b50 */
  unsigned short versionMin ;
  unsigned short bits ;
  unsigned short compression ;
  unsigned short fileModTime ;
  unsigned short fileModDate ;
  int crc32 ;
  int bytesCompressed ;
  int bytesUncompressed ;
  unsigned short bytesfilename ;
  unsigned short bytesExtra ;
  char filename[0] ;
//char extra[n] ;
} ;

struct VZIP__CentralDirFileHeader {
  int vcdSignature ;		/* 0x02014b50 */
  unsigned short versionMade ;
  unsigned short versionMin ;
  unsigned short bits ;
  unsigned short compression ;
  unsigned short fileModTime ;
  unsigned short fileModDate ;
  int crc32 ;
  int bytesCompressed ;
  int bytesUncompressed ;
  unsigned short bytesfilename ;
  unsigned short bytesExtra ;
  unsigned short bytesComments ;
  unsigned short diskNum ;
  unsigned short fileAttr ;
  int fileAttrExternal ;
  int offsetFile ;
  char filename[0] ;
//char extra[n] ;
//char comments[n] ;
} ;

struct VZIP__EndCentralDir {
  int ecdSignature ;		/* 0x06054b50 */
  unsigned short diskNum ;
  unsigned short diskNumCentralDir ;
  unsigned short numCDRThisDisk ;
  unsigned short totalCDR ;
  int bytesCentralDir ;
  int offsetCentralDir ;
  unsigned short bytesComment ;
  char comment[0] ;
} ;
#pragma pack()

struct VZIP__Master {
  struct VZIP__Entry *vzeFirst ;	/* Pointer to first entry, NULL if none */
  int offsetFile ;			/* Next file offset */
  struct UC__File UCFile ;		/* Unit for resulting output file */
  UCCHAR filename[V_FileName_Max+1] ;	/* Name of resulting zip file */
} ;

struct VZIP__Entry {
  struct VZIP__Entry *vzeNext ;		/* Pointer to next entry, NULL if end */
  BYTE *contents ;
  LENMAX bytes ;
  struct VZIP__CentralDirFileHeader vcd ;
} ;

/*	R E S U L T   C O D E S 		*/

#define V4RES_PosKey 1
#define V4RES_PosNext 2
#define V4RES_xxx 0

#define V4IS_PosBOF V4IS_PCB_GP_BOF		/* Position at begin of file */
#define V4IS_PosEOF V4IS_PCB_GP_EOF		/* Position at end of file */
#define V4IS_PosPrior V4IS_PCB_GP_Prior 	/* Position at prior key */
#define V4IS_PosNext V4IS_PCB_GP_Next		/* Position at next higher key */

#define V4IS_PosDCKL 0				/* Position anywhere in key list (don't care or s/b only one key) */
#define V4IS_PosBoKL 1				/* Position to beginning of key list (i.e. begin of list of dup's) */
#define V4IS_PosEoKL 2				/* Position to the end of key list */
#define V4IS_PosDCKLx 3 			/* same as DCKL except return pointer to ikde, not kp */

/*	E R R O R   C O D E S			*/

#define V4EN_IS_InvBktType 1			/* Invalid Bucket Type */
#define V4EN_IS_IKDE_InvEntryType 2		/* Invalid entry type in IKDE structure */
#define V4EN_IS_InvKeyMode 3			/* Invalid key mode */
#define V4EN_IS_InvGetMode 4			/* Invalid get mode */
#define V4EN_IS_DataMissing 5			/* Could not locate data with dataid in specified bucket */
#define V4EN_IS_NoStash 6			/* Could not stash key-data (probably too big for bucket size */

/*	C O M P I L A T I O N   D I R E C T O R Y	*/

#define V4LEX__CompileDir_Max 75
struct V4LEX__CompileDir
{ union V4IS__KeyPrefix kp ;		/* Type = V4, Subtype=CompileDir, Mode=Int, Len=8 */
  int Key0 ;				/* MUST BE 0 for now */
  int Bytes ;				/* Number of bytes in this structure */
  CALENDAR compUpdCal ;			/* Date-time of compilation as UCal value */
  short verMajor,verMinor ;		/* Version of V4 used in compilation */
  INDEX fileCount ;			/* Number of files below */
  struct {
    CALENDAR fileUpdCal ;		/* Source file update-dt as UCal value */
    B64INT spwHash64 ;			/* If non-zero then must use "-P xxx" startup switch to see source in this file */
    UCCHAR fileName[V_FileName_Max] ;	/* File name */
   }  File[V4LEX__CompileDir_Max] ;
} ;

/*	T O K E N   G E N E R A T O R			*/

#define V4LEX_TknType_String 1		/* A string literal (enclosed in quotes) */
#define V4LEX_TknType_Keyword 2 	/* Keyword, converted to upper case */
#define V4LEX_TknType_Integer 3
#define V4LEX_TknType_Float 4
#define V4LEX_TknType_EOL 5
#define V4LEX_TknType_Punc 6		/* Punctuation, stored in tcb->keyword */
#define V4LEX_TknType_LInteger 7
#define V4LEX_TknType_BString 8 	/* A binary value string literal (^4num,num,num,...) */
#define V4LEX_TknType_VString 9 	/* A string to be evaluated as a command immediately (^v'xxxxxx') */
#define V4LEX_TknType_PString 10	/* A string to be evaluated as a point immediately (^p'xxxxxx') */
#define V4LEX_TknType_Isct 20		/* ValPtr points to V4 intersection */
#define V4LEX_TknType_DES 21		/* ValPtr points to DataEl Spec */
#define V4LEX_TknType_Point 22		/* ValPtr points to a V4 point */
#define V4LEX_TknType_PointWithContext 23 /* ValPtr points to a V4 point followed by a context point (or list) */
#define V4LEX_TknType_Error 24		/* Syntax error, tcb->string contains error message */
#define V4LEX_TknType_Function 25	/* A function reference (func(args)) */
#define V4LEX_TknType_EOA 26		/* An EndOfArgument marker for functions */
#define V4LEX_TknType_PointWithStar 27	/* ValPtr points to a V4 point which ended with "*" (for EvalLE) */
#define V4LEX_TknType_RegExpStr 28	/* A regular expression string */
#define V4LEX_TknType_MLString 29	/* A not-yet-terminated multi-line string literal */

#define V4LEX_TknOp_CurrentPtr 1	/* Return (BYTE *) to current input */
#define V4LEX_TknOp_ThruCurrent 2	/* Returns string of bytes from base to current pointer */

#define V4LEX_InpMode_Stdin 1		/* Pull input from stdin */
#define V4LEX_InpMode_File 2		/* Pull from a file */
#define V4LEX_InpMode_String 3		/* Pull from a string */
#define V4LEX_InpMode_MacArg 4		/* A macro argument */
#define V4LEX_InpMode_TempFile 5	/* Pull from a file, delete file when EOF reached */
#define V4LEX_InpMode_CmpFile 6 	/* Pull from a compressed (text) file (via xv4 -zt option) */
#define V4LEX_InpMode_StringML 7	/* Pull from a multi-line string literal */
#define V4LEX_InpMode_RetEOF 8		/* Dummy mode to force EOF when we hit this tcb->ilvl[] */
#define V4LEX_InpMode_QString 9		/* For PushString ONLY - enclose in quotes */
#define V4LEX_InpMode_Error 10		/* Generate error if trying to grab input from this ilvl */
#define V4LEX_InpMode_Stage 11		/* Set up next nested level but don't link to it (tcb->ilvl[tcb->ilx+1].input_str will be allocated & available) */
#define V4LEX_InpMode_Commit 12		/* Commit to prior staging */
#define V4LEX_InpMode_StringPtr 13	/* Pull string pointer (i.e. don't copy string). NOTE: input_ptr is not allocated & should not be freed. USE WITH CARE!!!! */
#define V4LEX_InpMode_List 14		/* Pull from internal V4 list */

#define V4LEX_ReadLine_Bang 1		/* Don't treat "!" in column one as a comment */
#define V4LEX_ReadLine_FlushCmd 2	/* Flush input until a "command" in column 1 (i.e. an alpha character) */
#define V4LEX_ReadLine_NLNone 4 	/* BigText - don't keep new lines */
#define V4LEX_ReadLine_NLAsIs 8 	/* BigText - new lines - as is */
#define V4LEX_ReadLine_NLNL 0x10	/* BigText - new lines denoted with \n */
#define V4LEX_ReadLine_MTLAsIs 0x20	/* BigText - empty line - as is */
#define V4LEX_ReadLine_MTLTrash 0x40	/* BigText - trash empty lines */
#define V4LEX_ReadLine_IndentNone 0x80	/* BigText - indents - trim leading spaces & tabs */
#define V4LEX_ReadLine_IndentAsIs 0x100 /* BigText - indents - keep as is */
#define V4LEX_ReadLine_IndentBlank 0x200 /* BigText - indents - single space */
#define V4LEX_ReadLine_IndentTab 0x400	/* BigText - ignore leading tab */
#define V4LEX_ReadLine_Javascript 0x800	/* BigText - treat as Javascript source and remove unneeded spaces, comments, etc. */
#define V4LEX_ReadLine_NestTable 0x1000	/* Calling read-next-line from read-next-line-table - don't recurse back into table parsing (for CSV handling) */

#define V4LEX_AppendTo_LineTerm 1	/* If set then terminate LogToIsct lines with '\r' */
#define V4LEX_AppendTo_LogToIsct 2	/* If set then in LogToIsct (otherwise in BigText) */

#define V4LEX_Option_PushCur 0x1	/* Re-push current token so next call returns it again */
#define V4LEX_Option_RetEOL 0x2 	/* Return EOL as a "token" (for line oriented parsing) */
#define V4LEX_Option_ForceKW 0x4	/* Try to force next token as keyword */
#define V4LEX_Option_NegLit 0x8 	/* Treat leading "-" as part of negative number */
#define V4LEX_Option_ForceAsIs 0x10	/* Break input into tokens, return "as is" as string (for accepting macro defs) */
#define V4LEX_Option_ExpandArgs 0x20	/* Force expansion of #n# (overrides ForceAsIs) */
#define V4LEX_Option_FileSpec 0x40	/* Parse file name specification */
#define V4LEX_Option_NextChar 0x80	/* Look ahead for next character & place in tcb->keyword */
#define V4LEX_Option_AllowDot 0x100	/* Allow single "." in a keyword */
#define V4LEX_Option_WantInt 0x200	/* Expecting an (int) */
#define V4LEX_Option_WantDbl 0x400	/* Expecting a (double) */
#define V4LEX_Option_WantLInt 0x800	/* Expecting a (long int) */
#define V4LEX_Option_WantUOM 0x1000	/* Expecting a Unit-Of-Measure */
#define V4LEX_Option_Excel 0x2000	/* Expecting an Excel row-column spec */
#define V4LEX_Option_PatternSub 0x4000	/* Parsing a Recognition command substitution string */
#define V4LEX_Option_MakeNeg 0x8000	/* Make token negative (i.e. have implied leading minus sign) */
#define V4LEX_Option_NextChunk 0x10000	/* Return next generic token (up to next white space, comma, dotdot, etc.) */
#define V4LEX_Option_ForceAsIsULC 0x20000 /* Same as ForceAsIs but with upper/lower case preserved in tcb->string */
#define V4LEX_Option_XMLLitVal 0x40000	/* Parsing XML literal value - i.e. up to next '<' */
#define V4LEX_Option_ForceXMLKW (V4LEX_Option_ForceKW | 0x80000)	/* Try to force next token as an XML keyword */
#define V4LEX_Option_XMLCDATA 0x100000	/* Parsing XML CDATA literal value - i.e. up to next ']]>' */
#define V4LEX_Option_XML 0x200000	/* Parsing XML - disable certain punctuation combinations */
#define V4LEX_Option_HTML 0x400000	/* Parsing HTML */
#define V4LEX_Option_Hex 0x800000	/* Parse as hexadecimal (number) */
#define V4LEX_Option_Java 0x1000000	/* Parse as java(script) (number) */
#define V4LEX_Option_RegExp 0x2000000	/* Parse /xxxxx/ as regular expression pattern */
#define V4LEX_Option_Space 0x4000000	/* Return (any number of white spaces) as token: V4LEX_TknType_Punc/V_OpCode_Space */
#define V4LEX_Option_PushCurHard 0x8000000 /* Same as PushCur except resets input_ptr (as opposed to setting flag) */
#define V4LEX_Option_NextChunkExt 0x10000000 /* Return next generic token (up to next white space, comma, etc.) - INCLUDES '..' */
#define V4LEX_Option_MLStringLit 0x20000000 /* Allow for multi-line string literals */
#define V4LEX_Option_JSON 0x40000000	 /* Parsing JSON - don't escape characters, don't treat embedded NL as end-of-line, fall back to floating if integer parse fails */

#define V4LEX_Tkn_PromptMax 20		/* Max characters in prompt for more input */
#define V4LEX_Tkn_InpLvlMax 10		/* Number of nested input levels */
#define V4LEX_Tkn_ArgMax 100		/* Max number of macro arguments */
#define V4LEX_Tkn_ArgLineMax 2048	/* Max bytes in argument line */
#define V4LEX_Tkn_InpLineMax UCReadLine_MaxLine	/* Max bytes in input line */
#define V4LEX_Tkn_InpStrMax UCReadLine_MaxLine
#define V4LEX_Tkn_SrcFileLineMax 0x100000	/* Max characters in initial V4 source file line (to handle very large JSON strings) */
#define V4LEX_Tkn_KeywordMax 63 	/* Max bytes in keyword */
#define V4LEX_Tkn_StringMax UCReadLine_MaxLine	/* Max bytes in string literal */
#define V4LEX_Tkn_ParamMax 1024		/* Max bytes in #X# parameter */
#define V4LEX_Tkn_PParamMax 26		/* Max number of #X# parameters (one for each letter) */

#define V4LEX_Tkn_PPNumUndef 0x80000000 /* Value for undefined numvalue */

#define V4LEX_RPPFlag_EOL 1		/* Treat end-of-line as end of expression */
#define V4LEX_RPPFlag_Semi 2		/* Treat ";" as end of expression */
#define V4LEX_RPPFlag_PushParens 4	/* Push parens, brackets, braces, etc. as operators */
#define V4LEX_RPPFlag_ImplyAnd 8	/* Inject implied "&" between two adjacent values */
#define V4LEX_RPPFlag_ColonKW 0x10	/* Force a "keyword" after a colon */
#define V4LEX_RPPFlag_V4IMEval 0x20	/* Parse for V4 Internal-Module EvalAE() */
#define V4LEX_RPPFlag_BraceDelim 0x40	/* Braces delimit expression (i.e. "}" ends expression */
#define V4LEX_RPPFlag_NoCTX 0x80	/* Don't check for "|" meaning in-context (let it mean "or") */
#define V4LEX_RPPFlag_Excel 0x100	/* Parse as if Excel cell function */
#define V4LEX_RPPFlag_EvalLE 0x200	/* Parse within EvalLE (handle "*" special) */
#define V4LEX_RPPFlag_RParenBrkt 0x400	/* Non-nested right paren or bracket ends expression */
#define V4LEX_RPPFlag_NotADim 0x800	/* see V4DPI_PointParse_NotADim */

#define V4LEX_CSVStatus_OK 1		/* CSV grabber returns OK */
#define V4LEX_CSVError_TooBig -1	/* CSV grabber - string to big for destination */
#define V4LEX_CSVError_UnexpEOS -2	/* CSV grabber - hit UCEOS unexpectedly */
#define V4LEX_CSVError_InvQuote -3	/* CSV grabber - invalid quoted string syntax */

#define V4LEX_TCBINIT_NoStdIn 1		/* Don't init tcb with stdin as default base input (generate error instead) */

#define V4LEX_EchoEveryStartNum 500	/* If "echo" >= then echo every n'th input line */

#define V_OpCode_BOX		999
#define V_OpCode_EOX		998
#define V_OpCode_EOF		997
					/* Opcode >= 1000 are really values */
#define V_OpCode_QString	1000	/* (Single) quoted string (TknType = String) */
#define V_OpCode_DQString	1001	/* (Double) quoted string (TknType = String) */
#define V_OpCode_Keyword	1002	/* (no) quoted string (TknType = Keyword) */
#define V_OpCode_DQUString	1003	/* (Double) quoted string containing Unicode characters (i.e. at least one character > 255) */
#define V_OpCode_XMLLit		1004	/* An XML literal value (TknType = String) */
#define V_OpCode_XMLLitPartial	1005	/* An XML literal to be continued on next line (TknType = String) */
#define V_OpCode_RegExpStr	1006	/* A Regular Expression string */
#define V_OpCode_Numeric	1007	/* A numeric literal */
#define V_Opcode_EOA		1008	/* End-of-argument marker */
#define V_Opcode_V4Point	1009	/* A V4 point */
#define V_OpCode_MLString	1010	/* A multi-line string */

#define V_Opcode_GenericValue	1999	/* A generic value */
#define OPCODEISVALUE(OC) (OC >= 1000)
#define OPCODEISOPER(OC) (OC < 1000)

#define V_OpCode_Ampersand	1
#define V_OpCode_Colon		2
#define V_OpCode_ColonColon	3
#define V_OpCode_ColonEqual	4
#define V_OpCode_Comma		5
#define V_OpCode_DashRangle	6
#define V_OpCode_Dot		7
#define V_OpCode_DotDot 	8
#define V_OpCode_NCDot		9	/* Non contiguous dot, i.e. character before is white space */
#define V_OpCode_Equal		10
#define V_OpCode_EqualEqual	11
#define V_OpCode_LBrace 	12
#define V_OpCode_LBraceSlash	13
#define V_OpCode_LBracket	14
#define V_OpCode_LParen 	15
#define V_OpCode_LRBracket	16
#define V_OpCode_Langle 	17
#define V_OpCode_LangleDash	18
#define V_OpCode_LangleEqual	19
#define V_OpCode_LangleLangle	20
#define V_OpCode_LangleRangle	21
#define V_OpCode_Line		22
#define V_OpCode_Minus		23
#define V_OpCode_MinusEqual	24
#define V_OpCode_Not		25
#define V_OpCode_NotEqual	26
#define V_OpCode_Plus		27
#define V_OpCode_PlusEqual	28
#define V_OpCode_Pound		29
#define V_OpCode_Question	30
#define V_OpCode_RBrace 	31
#define V_OpCode_RBraceCross	32
#define V_OpCode_RBraceSlash	33
#define V_OpCode_RBracket	34
#define V_OpCode_RParen 	35
#define V_OpCode_Rangle 	36
#define V_OpCode_RangleEqual	37
#define V_OpCode_RangleRangle	38
#define V_OpCode_Semi		39
#define V_OpCode_Slash		40
#define V_OpCode_SlashEqual	41
#define V_OpCode_SlashSlash	42
#define V_OpCode_SlashStar	43
#define V_OpCode_Star		44
#define V_OpCode_NCStar 	45
#define V_OpCode_Tilde		46
#define V_OpCode_AtSign 	47
#define V_OpCode_UMinus 	48
#define V_OpCode_Percent	49
#define V_OpCode_BackQuote	50
#define V_OpCode_BackSlash	51
#define V_OpCode_Possessive	52
#define V_OpCode_StarEqual	53
#define V_OpCode_StarStar	54
#define V_OpCode_NCLParen	55
#define V_OpCode_TildeEqual	56
#define V_OpCode_NCColon	57
#define V_OpCode_NCLBrace	58
#define V_OpCode_LineLine	59
#define V_OpCode_DotDotDot	60
#define V_OpCode_EqualRangle	61
#define V_OpCode_LangleSlash	62
#define V_OpCode_SlashRangle	63
#define V_OpCode_LangleQuestion	64
#define V_OpCode_QuestionRangle	65
#define V_OpCode_LangleBang	66
#define V_OpCode_LangleBangDashDash 67	/* <!-- - start of XML comment */
#define V_OpCode_DashDashRangle	68	/* --> - end of XML comment */
#define V_OpCode_NCLBracket	69	/* Non-contiguous '[' (character before is white-space) */
#define V_OpCode_ColonColonColon 70
#define V_OpCode_Space		71
#define V_OpCode_CommaComma	72
#define V_OpCode_DollarSign	73
#define V_OpCode_DollarLParen	74
#define V_OpCode_DolDolLParen	75

#define V_Opcode_Starting	1	/* NOTE - these two must reflect range of punctuation opcodes */
#define V_Opcode_Ending		75	/* Also need to update in v4dictionary & opcodeDE[] in vcommon */

#define V_OpCode_MacArg0	100
#define V_OpCode_MacArg1	101
#define V_OpCode_MacArg2	102
#define V_OpCode_MacArg3	103
#define V_OpCode_MacArg4	104
#define V_OpCode_MacArg5	105
#define V_OpCode_MacArg6	106
#define V_OpCode_MacArg7	107
#define V_OpCode_MacArg8	108
#define V_OpCode_MacArg9	109
#define V_OpCode_MacArgA	110
#define V_OpCode_MacArgZ	(V_OpCode_MacArgA + 25)
#define V_OpCode_IncMacArgA	(V_OpCode_MacArgZ+1)
#define V_OpCode_IncMacArgZ	(V_OpCode_IncMacArgA + 25)

#define V4LEX_TCBMacro_Tab 1		/* Treat input as tab delimited */
#define V4LEX_TCBMacro_CSV 2		/* Comma delimited */
#define V4LEX_TCBMacro_Header 4 	/* Skip 1st header line of file */
#define V4LEX_TCBMacro_Auto 8		/* Pull macro name from first column of spreadsheet */
#define V4LEX_TCBMacro_XML 0x10 	/* Parse as XML stream */
/* Greater than 0xffff for global, less than 0x10000 apply only to macros */
#define V4LEX_TCBMacro_IgBlanks 0x10000 /* Ignore blank lines */
#define V4LEX_TCBMacro_Continued 0x20000 /* Long lines may be continued by ending with backslash */

#define V4LEX_TCBMacro_NameMax V4LEX_Tkn_KeywordMax	/* Max characters in macro name */
#define V4LEX_NestedIfThenElse 20	/* Max nested lexical If/Then/Else's */

#define V4LEX_Radix_V4EvalStr 100	/* Special tcb->radix number for ^v'xxxxx' strings */
#define V4LEX_Radix_V4PointStr 101	/* Special tcb->radix number for ^p'xxxxx' strings */

/*	Token generator result structure	*/
struct V4LEX__TknCtrlBlk
{ COUNTER tcb_lines ;			/* Total lines processed thru TCB */
  COUNTER tcb_tokens ;			/* Total tokens */
  LOGICAL AutoExitEOF ;			/* If TRUE then auto-exit at EOF of initial command file */
  short opcode ;			/* Opcode- V_Oper_xxx is type is punctuation */
  short prec ;				/* Default operator precedence if punctuation */
  short type ;				/* Token result - see V4LEX_TknType_xxx */
  short need_input_line ;		/* Nonzero if need new input line */
  UCCHAR sections[V4LEX_Tkn_ArgLineMax] ; /* List of sections to execute of an include file */
  short ifxilx ;			/* ilx index defining ifx */
  int LastifxID ;			/* Last used ifxID for below */
  short ifx ;				/* Index to below */
  struct {
    int ifxID ; 			/* Unique id for this IF/THEN/ELSE */
    short doif ;			/* Execute commands in IF portion */
    short doelse ;			/* Execute commands in ELSE portion */
    short inif ;			/* In the IF portion if TRUE else in else */
    UCCHAR name[V_FileName_Max+30] ; /* Optional if-level name */
   } ifs[V4LEX_NestedIfThenElse] ;
  short ilx ;				/* Index to below */
  struct {
    ETYPE mode ;			/* Input mode- V4LEX_InpMode_xxx */
//    FILE *file ;			/* Input file desc pointer (NULL if for user tty) */
    struct UC__File *UCFile ;		/* Pointer to input file (replaces prior *file) */
    INDEX vcdIndex ;			/* If not UNUSED then index of this file in gpi->vcd (struct V4LEX__CompileDir) */
    INDEX lastVCDLine ;			/* Last line number used in vcd (to track .1 .2 ... sub line numbers) */
    int subVCDLine ;			/* Sub line number */
    struct V_COMPRESS_IncludeInfo *cii ;/* If not NULL then file is compressed - buffers & info via cfi */
    struct V4LEX__TablePrsOpt *tpo ;	/* Table parsing options */
    UCCHAR *strptr ;			/* Points to string if mode = V4LEX_InpMode_String */
    UCCHAR *input_str ;			/* Current input line - allocated to V4LEX_Tkn_InpStrMax characters in v4lex_NextInput() */
    UCCHAR *input_ptr ;			/* Pointer to next byte in input_str */
    COUNTER listX ;			/* List index counter */
    struct V4L__ListPoint *listp ;	/* Point to list if input to be taken from V4 list */
    struct V4DPI__Point *bcPt ;		/* If not NULL then binding-context for this lexical level (and beneath) */
    UCCHAR *lvlSections ;		/* If not NULL then list of sections to execute (scope is just this include level) */
    COUNTER echo ;			/* If TRUE then echo input lines (if > 1000 then echo from file every n'th) */
    LOGICAL echoDisable ;		/* If TRUE then disable echo (to prevent sensitive info from appearing in logs, such as credit card numbers */
    LOGICAL echoHTML ;			/* If TRUE then echo HTML span's to format prompt/input/output to/from V4 */
    UCCHAR prompt[V4LEX_Tkn_PromptMax] ;/* Prompt if input is user tty */
//    UCCHAR MacroName[32] ;		/* If non-empty then name of macro to use for each line */
    struct V4LEX__Table *vlt ;		/* If not NULL then pointer to current TABLE (used for xxx* parsing in macros) */
//    ETYPE MacroFlags ;			/* See V4LEX_TCBMacro_xxx */
    int ifxID_on_entry ;		/* ifxID value on entry (used in check for mismatched If/then/else) */
    int statement_start_line ;		/* Line number current statement started on */
    int current_line ;			/* Current line number */
    int total_lines ;			/* Total lines in this file */
    short indent_line ; 		/* Indentation of current line (spaces + tabs) */
    UCCHAR file_name[V_FileName_Max] ;	/* File name (or nothing) for this level */
    struct V4LEX__MacroCallArgs *vlmca ;/* If not NULL then calling arguments for this level (structure allocated once & may be reused multiple) */
    short last_page_printed ;		/* Set to current page on error (to avoid extra print-out) */
    UCCHAR arglist[V4LEX_Tkn_ArgLineMax] ;/* Pointer to string of macro arguments */
    short BoundMacroArgs ;		/* TRUE if macro arguments have been bound below */
    short HaveMacroArgNames ;		/* True if we got macro argument names below */
					/* If > 1 then also the character argument name prefix! */
    struct {
      UCCHAR name[V4LEX_Tkn_KeywordMax+1] ; /* Optional macro argument name */
      UCCHAR *value ;			/* Pointer to macro argument (if any) */
     } macarg[V4LEX_Tkn_ArgMax] ;
//    struct V4DPI__Point *fltrpt ;	/* If not NULL then intersection to eval on each line as a filter */
   } ilvl[V4LEX_Tkn_InpLvlMax] ;
  UCCHAR *prior_input_ptr ;		/* Pointer to begin of token (for lookahead */
  int cpmStartLine ;			/* Starting line number of currently parsing macro (CPM) definition */
  LENMAX maxCharInpBuf ;		/* If greater than 0 then max number of characters in input line (default = V4LEX_Tkn_InpStrMax) */
  UCCHAR cpmName[V4LEX_TCBMacro_NameMax+1] ; /* Currently parsing macro name */
  short have_lookahead ;		/* If true then re-tokenize prior token */
  short lookahead_flags ;		/* Flags from prior call to NextTkn (so maybe next call can be real fast!) */
  short have_value ;			/* If true then last token returns a value (for unary minus) */
  short in_comment ;			/* If true then in a slash-star ... star-slash comment */
  short in_jsonRef ;			/* If true then in $(json reference) - treat decimal points ALWAYS as dot, not numeric decimal point */
  short check_initialized ;		/* If true then check for initialized stack variables */
//  char keyword[V4LEX_Tkn_KeywordMax+1] ;	/* If a keyword then the keyword string */
  unsigned short keyword_len ;		/* Length of keyword */
  unsigned short actual_keyword_len ;	/* Actual length before possible truncation */
  UCCHAR UCkeyword[V4LEX_Tkn_KeywordMax+1] ;	/* Unicode version of keyword */
  short default_radix ; 		/* Default Radix (usually 10) of literal integer */
  short radix ; 			/* Radix (usually 10) of current literal integer */
  unsigned short literal_len ;		/* If a literal then length */
  int integer ; 			/* Value if literal integer */
  short decimal_places ;		/* Number of decimal places */
  double floating ;
//  char string[V4LEX_Tkn_StringMax] ;
  UCCHAR UCstring[V4LEX_Tkn_StringMax] ;
  struct
   { LOGICAL SetAtStartup ; 		/* TRUE if parameter set via v4 startup */
     int numvalue ;			/* Numeric value: #A=0#, #A++#, etc. */
     UCCHAR value[V4LEX_Tkn_ParamMax+1] ;
   } poundparam[V4LEX_Tkn_PParamMax] ;	/* Values for #A# ... #Z# */
  UCCHAR *appendto ;			/* If not null then append lines to this pointer */
  LENMAX maxappend ;			/* Max number of bytes we got */
  LENMAX bytesappend ;			/* Number used */
  FLAGS32 appendopt ;			/* Append options (see V4LEX_AppendTo_xxx) */
  B64INT Linteger ;			/* Long integer */
} ;

/*	L E X I C A L   M A C R O   S T R U C T U R E		*/

struct V4LEX__MacDirBoot		/* Small structure that has segment key of Macro directory structure */
{ union V4IS__KeyPrefix kp ;		/* Type=V4, SubType=MacroBoot, Mode=Int, Len=8 */
  int mustBeZero ;			/* Must be 0 */
  V4MSEGID segId ;			/* Segment key to V4LEX__MacroDirectory structure */
} ;

#define V4LEX_MacDir_MaxInit 100	/* Initial max number of macros (may be expanded at run-time) */
#define V4LEX_MACTYPE_Macro 1		/* A V4 macro */
#define V4LEX_MACTYPE_Table 2		/* A V4 table definition */
#define V4LEX_MACTYPE_MIDAS 3		/* A V3/MIDAS 'macro' definition */

#define V4LEX_MACRO_UDALIST UClit("UDALIST")	/* Name of starred argument */
#define V4LEX_MACRO_BTARG UClit("BTARG")	/* Name of starred argument */

#define SIZEOFVLMD(VLMDptr,MAXENTRIES) ((sizeof *VLMDptr) + ((sizeof ((VLMDptr)->macro[0])) * ((MAXENTRIES) -  V4LEX_MacDir_MaxInit)))
 
struct V4LEX__MacroDirectory		/* Top level directory of all macros */
{ 
  LOGICAL isDirty ;			/* If TRUE then structure has been updated & must be written on area close */
  LOGICAL isSorted ;			/* If TRUE then macro names sort & we can do binary search */
  COUNTER maxEntries ;			/* Max number we can handle */
  COUNTER numEntries ;			/* Number of macros defined below */
  struct {				/* Directory entry for each macro */
    struct V4LEX__MacroEntry *vlme ;	/* If not NULL then points to in-memory macro definition */
    V4MSEGID segId ;			/* Segment key to macro definition (same as above key, but not 0) */
    ETYPE macType ;			/* Type of macro- V4LEX_MACTYPE_xxx */
    UCCHAR macNameNC[V4LEX_Tkn_KeywordMax+1] ; /* Macro name (NC = normalized case (upper case)) */
    } macro[V4LEX_MacDir_MaxInit] ;
} ;

struct V4LEX__MacroEntry
{ UCCHAR macNameNC[V4LEX_Tkn_KeywordMax+1] ;
  LENMAX bytes ;			/* Number of bytes in this entry */
  COUNTER refCount ;			/* Number times this macro has been called/referenced */
  INDEX bodyOffset ;			/* Offset into macData[] to macro body */
  INDEX udaArgX ;			/* If not UNUSED then index to udaList argument */
  INDEX btArgX ;			/* If not UNUSED then index to btArg argument */
  COUNTER argCount ;			/* Number of arguments */
  struct {
    UCCHAR argName[V4LEX_Tkn_KeywordMax+1] ;
   } arg[1] ;				/* This varies with actual number of arguments */
  BYTE macData[V4AreaAgg_UsableBytes] ;	/* Space for arguments with macro body immediately following */ 
} ;

struct V4LEX__MacroBody
{ UCCHAR macBody[0] ;			/* Macro body (terminated with UCEOS) */
} ;

#define V4LEX_ARGBUF_MAX 0x10000
struct V4LEX__MacroCallArgs		/* Holds parsed macro call argument values */
{ COUNTER argCount ;			/* Number of arguments */
  UCCHAR firstChar ;			/* If not UCEOS then common-prefix character for arguments */
  struct {
    UCCHAR argName[V4LEX_Tkn_KeywordMax+1] ;
    INDEX argVX ;			/* Index to argument value - argBuf[] - (UNUSED if none) */
    } arg[V4LEX_Tkn_ArgMax] ;
  INDEX abIndex ;			/* Index to next free spot in argBuf */
  UCCHAR argBuf[V4LEX_ARGBUF_MAX] ;	/* Holds argument values */
} ;

#define V4LEX_RPP_LineMax 200		/* Max characters in parse line */
#define V4LEX_RPP_AlphaValMax 100	/* Max characters in alpha token */
#define V4LEX_RPP_TokenMax 100		/* Max number of tokens */

struct V4LEX__RevPolParse		/* Structure to hold reverse polish parsing */
{ 
//  char ParseLine[V4LEX_RPP_LineMax] ;	/* Line parsed */
  short MapCount ;			/* First "n" below are to be evaluated as "bitmap" once */
  short Count ; 			/* Number below */
  struct
   { short Type ;			/* Type of token (same as tcb->type, V4LEX_TknType_xxx) */
     short Prec ;			/* Precedence (if operator) */
     short dps ;			/* Decimal places */
     short OpCode ;			/* Opcode (0 if not operator)- see V_Oper_xxx */
     short IsMap ;			/* TRUE if operator to be handled via keyed-get or Inverted map lookup */
     short KeyNum ;			/* If keyed lookup then key number (highest value wins) */
     short Arguments ;			/* Number of arguments taken by operator */
     int IntVal ;			/* Integer value of token */
     double Floating ;			/* Floating value */
     BYTE *ValPtr ;			/* Pointer to the value */
     UCCHAR Dimension[32+1] ;		/* Dimension of the Token */
     UCCHAR AlphaVal[V4LEX_RPP_AlphaValMax] ; /* Alpha value of token */
   } Token[V4LEX_RPP_TokenMax] ;
} ;

#define V4LEX_BigText_Max ((V4Area_UsableBytes / sizeof(UCCHAR)) - 512) /* Max bytes in big buffer */
#define EOLbt '\r'
#define EOLbts "\r"

struct V4LEX__BigText			/* Format of a V4 "Big Text" Entry */
{ union V4IS__KeyPrefix kp ;		/* Type = V4, Subtype=BigText, Mode=Int, Len=8 */
  int Key ;				/* Key for entry (usually {Unique} V4 point) */
  int Bytes ;
  UCCHAR BigBuf[V4LEX_BigText_Max] ;	/* Buffer for text */
} ;

#define V4LEX_TablePT_Default 0 	/* Default format */
#define V4LEX_TablePT_Internal 1	/* Internal format (e.g. internal universal date format instead of yymmdd) */
#define V4LEX_TablePT_YYMMDD 2		/* Date is yymmdd format */
#define V4LEX_TablePT_MMDDYY 3		/* Date is mmddyy format */
#define V4LEX_TablePT_DDMMMYY 4 	/* Date is ddmmmyy format */
#define V4LEX_TablePT_DDMMYY 5		/* Date is ddmmyy format */
#define V4LEX_TablePT_Hex4 6		/* String represented as 4-byte hex numbers */
#define V4LEX_TablePT_Hexadecimal 7	/* Number represented in hexadecimal */
#define V4LEX_TablePT_Money 8		/* Number is money - allow spaces, "$", commas, etc. */
#define V4LEX_TablePT_YYMM 9		/* Month format is yymm */
#define V4LEX_TablePT_MMYY 10		/* Month format is mmyy */
#define V4LEX_TablePT_MMM 11		/* Month format is yy-MMM or MMM-yy (month is alpha) */

#define V4LEX_YMDOrder_None 0		/* None specified */
#define V4LEX_YMDOrder_YMD 1
#define V4LEX_YMDOrder_MDY 2		/* Order is month-day-year */
#define V4LEX_YMDOrder_MYD 3		/* (you get the idea) */
#define V4LEX_YMDOrder_DMY 4
#define V4LEX_YMDOrder_DYM 5
#define V4LEX_YMDOrder_YDM 6

#define V4LEX_YMOrder_YM 0		/* Just year-month for UMonths */
#define V4LEX_YMOrder_MY 1

#define V4LEX_TableCT_Ignore 1		/* Ignore this column */
#define V4LEX_TableCT_TableName 2	/* This is the name of the table to parse this row */
#define V4LEX_TableCT_AggKey 4		/* This column (s/b first) is going to be key in Agg */
#define V4LEX_TableCT_NPTableName 8	/* This column value is name of table (same as TableName but no prefix) */
#define V4LEX_TableCT_Trim 0x10 	/* Trim leading & trailing spaces from field */
#define V4LEX_TableCT_Skip 0x20 	/* Want to skip n columns (n stored in Decimals field) */
#define V4LEX_TableCT_Quoted 0x40	/* Text string may be quoted, if so then strip leading/trailing quotes */
#define V4LEX_TableCT_Kernel 0x80	/* Defaults taken from kernel column (e.g. don't deallocate vtt) */
#define V4LEX_TableCT_Point 0x100	/* Column value taken from point value found via Col[]->Dflt */
#define V4LEX_TableCT_Expression 0x200	/* Column value determined by evaluating expression, should not be inserted into Aggregate (via #table-name#) but into binding (via Loop #table-name#) */

#define V4LEX_Table_XML_Defined 1	/* Bit is set when column has definition */

#define V4LEX_ReadXML_OK 0x1		/* Parse of XML file went ok */
#define V4LEX_ReadXML_StructureErr 0x2	/* Error in structure/syntax of XML file */
#define V4LEX_ReadXML_ValueErr 0x4	/* Error in value */
#define V4LEX_ReadXML_LimitErr 0x8	/* Limit (most likely length) exceeded */
#define V4LEX_ReadXML_NestErr 0x10	/* Nested table error */
#define V4LEX_ReadXML_UndefErr 0x20	/* Undefined column/table */

#define V4LEX_TableHT_IgnErr 1		/* Ignore value in this (repeating) column if in error */
#define V4LEX_TableHT_IgnDflt 2 	/* Ignore value in this (repeating) column if blank/empty */

#define V4LEX_Table_ColMax 200
//#define V4LEX_Table_HdrMax 20
#define V4LEX_Table_DimMax 5

struct V4LEX__TablePrsOpt		/* Table parsing options */
{
  P *filterPt ;				/* If not NULL then filter point for reading lines */
  int Delim ;				/* Table delimiter ('\t' or ',' usually) */
  COUNTER HeaderLines ;			/* If TRUE then skip first n lines of file */
  UCCHAR BgnMacro[V4LEX_TCBMacro_NameMax+1] ;  /* If not null then name of macro to call @ begin of this table */
  UCCHAR Macro[V4LEX_TCBMacro_NameMax+1] ;	/* If not null then name of macro to process rows of this table */
  UCCHAR Comment[12] ;			/* Ignore lines beginning with this character */
  LENMAX MinLength ;			/* If not UNUSED then ignore lines with less than this number of characters */
  LOGICAL isEmbed ;			/* If TRUE then macro name embedded in input line */
  FLAGS32 readLineOpts ;		/* ReadLine options (V4LEX_ReadLine_xxx) */
} ;
#define INITTPO \
 { if (tpo == NULL) { tpo = v4mm_AllocChunk(sizeof *tpo,TRUE) ; tpo->Delim = '\t' ; tpo->MinLength = UNUSED ; } ; }

struct V4LEX__Table			/* Format of a table definition */
{ struct V4LEX__Table *link ;		/* Link to next/prior table (NULL = end of list) (ctx->pi->vlt = ptr to first) */
  struct V4C__Context *ctx ;		/* Pointer to current context */
  struct V4LEX__TablePrsOpt *tpo ;	/* Link to table parsing options when table is actually being used (i.e. called from Include) */
  struct V4LEX__TablePrsOpt tpoT ;	/* Parse options defined with table (may be overridden at runtime by Include via *tpo) */
  struct V4LEX__TableHash *lth ;	/* If not NULL then pointer to column name hash table */
  UCCHAR *otherList ;			/* If not NULL then 'Other' option list (as a string) */
  COUNTER inclusionIndex ;		/* If not UNUSED then indicates which inclusionMask entry (below) to include, otherwise use all */
  LOGICAL IsDefault ;			/* TRUE if this is the defaults table */
  LOGICAL InXML ;			/* TRUE if table is active XML table */
  UCCHAR Name[V4LEX_TCBMacro_NameMax+1] ;	/* Name of this table */
  COUNTER dimCount ;
  DIMID dimIds[V4LEX_Table_DimMax] ;	/* One or more dimensions associated with this table */
//  INDEX Headers ; 			/* Number of defined header lines below */
//  struct {
//    INDEX StartColumn ;			/* Starting column of info (if last column defined below then info on repeatng */
//    struct V4DPI__DimInfo *di ; 	/* Dimension info */
//    UCCHAR Name[V4DPI_DimInfo_DimNameMax+1] ; /* Name of the header entry (referenced like dimension: xxx* within macro) */
//    ETYPE HeaderType ;			/* Mask of header types: V4LEX_TableHT_xxx */
//    P *Dflt,*Err,*Missing ;		/* Default, Error, and Missing points */
//    P *Cur ;				 /* Current point value */
//    struct V4LEX__Table_ColHdr *tch ;	/* If not NULL then points to repeating column information */
//   } Hdr[V4LEX_Table_HdrMax] ;
  INDEX Columns ; 			/* Number below */
//  INDEX RepeatColumns ;			/* Number of repeating columns (starting with last defined below) */
  UCCHAR colNamePrefix[16] ;		/* If not empty then a character prefix to be stuck on front of each column name */
  UCCHAR colNameSuffix[16] ;		/*   ditto but as suffix */
  struct {
    P *Dflt,*Err,*Missing ;		/* Default, Error, and Missing points */
    P *Cur ;				/* Current point value */
    struct V4DPI__DimInfo *di ; 	/* Point to dimensional info about this column */
    struct V4DPI__DimInfo *di1 ;	/* Extra Point to dimensional info about this column (data don't match first) */
    struct V4LEX__Table_Tran *vtt ;	/* If non NULL then links column to a transformation table (see below) */
    UCCHAR *Desc ;			/* If non NULL then pointer to description string for this column */
    PNTTYPE PntType ;			/* If not UNUSED then parse via this point type */
    PNTTYPE PntType1 ;			/*  ditto point type */
    ETYPE PntTypeInfo ;			/* Point-type specific information: V4LEX_TablePT_xxx */
    int Decimals ;			/* Number of implied decimal places */
    FLAGS32 inclusionMask ;		/* Set to indicate which subsets of the table (which columns) is wanted within particular #table-name:nnn# construct */
    INDEX StartX,EndX,Width ;		/* If not UNUSED then begin/end columns within row */
    ETYPE ColumnType ;			/* Column Attributes (mask): V4LEX_TableCT_xxx */
    int FixedLength ;			/* If > 0 then column s/b fixed length (i.e. pad trailing spaces) */
    FLAGS32 XMLFlags ;			/* Flags used for XML parsing: V4LEX_Table_XMLxxx */
    UCCHAR Name[V4DPI_DimInfo_DimNameMax+1] ; /* Name of the column (upper case) (referenced like dimension: xxx* within macro) */
//    char ulName[V4DPI_DimInfo_DimNameMax+1] ; /* Name of the column (referenced like dimension: xxx* within macro) */
   } Col[V4LEX_Table_ColMax] ;
} ;

struct V4LEX__Table_ColHdr		/* Defines column headers for repeating column */
 { INDEX Count ;				/* Number of values defined */
   struct {
     P Cur ;				/* Current value of the column */
    } Col[V4LEX_Table_ColMax] ;
 } ;

#define V4LEX_Table_Tran_SubMax 50	/* Max number of substitutions below */
#define V4LEX_Table_Tran_ErrVal 0x80000000	/* If OutValue of substitution equal to this then value is in error */

#define V4LEX_TT_SubStr_Before 0x1	/* Match before */
#define V4LEX_TT_SubStr_After 0x2	/* Match after */

#define V4LEX_TT_Tran_UpperCase 0x1	/* Convert to upper case */
#define V4LEX_TT_Tran_LowerCase 0x2	/* Convert to lower case */

struct V4LEX__Table_Tran		/* Transformations to apply to a column value */
 { UCCHAR *UCprefix ;			/* A prefix string to add */
   UCCHAR *UCsuffix ;			/* A suffix string to add */
   ETYPE Transforms ;			/* Other transforms - V4LEX_TT_Tran_xxx */
   INDEX offset ; 			/* An offset to add to the value, if numeric */
   ETYPE substrHow ;			/* How to interpret substring - see V4LEX_TT_SubStr_xxx */
   UCCHAR *UCsubstr ;			/* Substring to match on */
   double scale ;			/* A scaling factor to be applied to numeric value */
   struct V4DPI_UOMDetails *uomd ;	/* If not NULL then UOM Details for this column */
   struct V4LEX__Table_Tran_Sub *vtts ; /* If not NULL then pointer to substitutions below */
   struct V4DPI__Point *acpt ;		/* If not NULL then Acceptor isct to evaluate to get point value (column text -> Alpha*) */
 } ;

struct V4LEX__Table_Tran_Sub
 { INDEX Count ;			/* Number of substitutions below */
   struct {
     int InValue ;			/* Incoming value */
     int OutValue ;			/* Outgoing value to use */
    } Sub[V4LEX_Table_Tran_SubMax] ;
 } ;

#define V4LEX_HASH_NUM (V4LEX_Table_ColMax*2+1)
struct V4LEX__TableHash
 { INDEX Count ;			/* Number of defined entries */
   struct {
     int Hash ; 			/* Hash value */
     INDEX ColX ; 			/* Column Index */
    } Entry[V4LEX_HASH_NUM] ;
 } ;


//struct kwlist1				/* Format of key-word structures */
// { int value ; char entry[25] ; } ;
//struct kwlist2
// { int value ; char entry[25] ; char display[25] ; } ;

/*	V 4 E V A L   S T R U C T U R E S	*/

#define V4E_Loop_Int 1			/* Integer number loop */
#define V4E_Loop_Keywords 2		/* List of keywords loop */
#define V4E_Loop_Table 3		/* Table/Column loop */

struct V4Eval__Loop			/* Structure for loops */
 { struct V4Eval__Loop *Self ;		/* Point to self (for sanity checking) */
   struct V4LEX__Table *vlt ;		/* Pointer to vlt if table loop */
   ETYPE LoopType ;			/* Type of loop- V4E_Loop_xxx */
   INDEX Num,End ;			/* Values for integer & table loops */
   INDEX Param ;			/* Parameter index (0-25) to update */
   UCCHAR KWBuf[V4LEX_Tkn_InpLineMax] ;	/* Table list of keywords */
   UCCHAR V4Com[V4LEX_Tkn_InpLineMax] ;	/* V4 command to be looped */
 } ;

#define MacCacheMax 150
  struct  V4Eval__MacroCache {
    LENMAX NextFree ;
    UCCHAR SetArgs[1024] ;		/* Bytes for "Set Argument" */
    INDEX lru ;
    INDEX Count ;
    struct {
      UCCHAR macname[V4LEX_TCBMacro_NameMax+1] ;	/* Macro Name */
      UCCHAR *macdef ;			/* Pointer to macro definition */
      INDEX lrux ;			/* Last used index (oldest gets zapped out!) */
     } Cache[MacCacheMax] ;
   } ;




/*	R P P	E X E C U T I O N   D E F S			*/

#define VRPP_XctEntry_Max 50		/* Max number of entries (tokens) in execution structure */
#define VRPP_XctFile_Max 10		/* Max number of file buffers */
#define VRPP_XctValBuf_Size 2000	/* Number of bytes in value buffer */

#define VRPP_ValType_LiteralInt 1	/* Integer literal in Val.IntVal */
#define VRPP_ValType_LiteralAlpha 2	/* Alpha literal via Val.ValPtr */
#define VRPP_ValType_LiteralDbl 3	/* Double literal vial Val.FloatVal */
#define VRPP_ValType_Isct 4		/* V4 Isct via Val.ValPtr */
#define VRPP_ValType_Field 5		/* DES via Val.DES & appropriate File[].BufferPtr */
#define VRPP_ValType_V3Mod 6		/* V3 Module reference via Val.ValPtr */
#define VRPP_ValType_V3Op 7		/* V3 internal operation in Val.IntVal */
#define VRPP_ValType_PushEOA 8		/* Push V3 EndOfArg onto stack */
#define V4PP_ValType_ModCall 9		/* Module call */

struct VRPP__Xct			/* Definition structure for vrpp_Execute */
{ INDEX EntryCount ;			/* Number of entries below */
  struct {
    ETYPE ValueType ;			/* Type of value - VRPP_ValType_xxx */
    int Aux1 ;				/* Extra information (e.g. decimal places for IntVal) */
    union {
     struct V4FFI__DataElSpec DES ;	/* Data Element Specification */
     int IntVal ;
     double FloatVal ;
     BYTE *ValPtr ;			/* Pointer to value (ex: to V3 module mep) */
    } Val ;
   } Entry[VRPP_XctEntry_Max] ;
  INDEX FileCount ;
  struct {
    int FileRef ;			/* File Ref */
    BYTE *BufferPtr ;			/* Pointer to record buffer */
   } File[VRPP_XctFile_Max] ;
  int ValBufLUX ;			/* Index to last used byte */
  BYTE ValBuf[VRPP_XctValBuf_Size] ;	/* Buffer for a value */
} ;

/*	E R R O R   S T R U C T U R E S 	*/

#define V_StdErr_Unknown 0		/* Generic Error Code */
#define V_StdErr_IO 1			/* General IO error */
#define V_StdErr_EOF 2			/* End-of-File */
#define V_StdErr_Locked 3		/* Record locked */
#define V_StdErr_RecTooBig 4		/* Record too big for user buffer */

struct V4ERR__Info
{ struct V4LEX__TknCtrlBlk *tcb ;	/* Pointer to tcb (if any, otherwise NULL) */
  char subsys[50],module[50],mne[50] ;
  UCCHAR UCmsg[250] ;			/* Expanded error message */
  int code ;				/* V4 Error Code */
} ;

struct V4ERR__Dump			/* Hold all info on last 'dumped' (via v4trace_ExamineState) error */
{ UCCHAR *ucbuf ;			/* Buffer for all text below */
  COUNTER bSize ;			/* Number of characters in buffer */
  COUNTER bUsed ;			/* Number of characters used so far */
  BYTE *ctxbuf ;			/* Holds context points */
  COUNTER cSize,cUsed ;
  struct V4DPI__Point *ctxpts[V4C_CtxDimHash_Max] ;	/* Context points */
  COUNTER lx ;				/* Current level index */
  struct {
    COUNTER lineNumber ;		/* Line in source */
    UCCHAR *srcFile ;			/* Source file name */
    UCCHAR *errMsg ;			/* Error message */
   } level[V4C_Frame_Max] ;
} ;

/*	Client structure for Server Linkage */

#define V4CS_RFRFileRef_Max 50
#define V4CS_RFRServer_Max 5
#define V4CS_OpenFile_Max 25

#define V4CS_RFROpt_Remote 0x1		/* Always pull from server */
#define V4CS_RFROpt_LocalRemote 0x2	/* Check client for record first */
#define V4CS_RFROpt_DupLocal 0x4	/* Save record on client if pulled from server */
#define V4CS_RFROpt_TimeStamp 0x8	/* Verify client version via timestamp */

struct V4CS__RemoteFileRef
{ INDEX FileRefCount ;
  struct {
   int FileRef ;			/* FileRef being/to-be remotely served */
   char HostName[32] ;
   SOCKETPORT PortId ; 			/* Port ID */
   SOCKETNUM Socket ; 			/* Socket assigned (-1 if none assigned) */
   FLAGS32 Options ;			/* Options- see V4CS_RFROpt_xxx */
   } FileRefs[V4CS_RFRFileRef_Max] ;
  INDEX ServerCount ;
  struct {
   char HostName[32] ;
   SOCKETPORT PortId ; 			/* Port ID */
   SOCKETNUM Socket ; 			/* Socket to the server */
   } Servers[V4CS_RFRServer_Max] ;
} ;

/*	Message Types */

#define V4CS_Msg_OpenFileRef 1		/* Open area on server for given fileref */
#define V4CS_Msg_CloseAID 2		/* Close specified aid on server */
#define V4CS_Msg_GetKey 3		/* Get keyed record on server & return to client */
#define V4CS_Msg_GetNext 4		/* Get next record */
#define V4CS_Msg_Put 5			/* Put a record */
#define V4CS_Msg_PutOK 6		/* Put operation @ server OK */
#define V4CS_Msg_OpenOK 7		/* Open operation went OK */
#define V4CS_Msg_GetDataId 8		/* Get record based on data id */
#define V4CS_Msg_Error 9		/* Returing error from server */
#define V4CS_Msg_DataRec 10		/* Resulting data record */
#define V4CS_Msg_ShutdownAID 11 	/* Server has closed specified AID */
#define V4CS_Msg_ShutdownServer 12	/* Server is shutting down */
#define V4CS_Msg_ReqTime 13		/* Client requesting time stamp from server */
#define V4CS_Msg_TimeStamp 14		/* Time stamp from server */
#define V4CS_Msg_FileRefAID 15		/* Return msg from server from OpenFileRef */
#define V4CS_Msg_ReqTPort 16		/* Request from client for Transaction Port */
#define V4CS_Msg_TPort 17		/* Reply to client request for Transaction Port */
#define V4CS_Msg_ReqOpenFiles 18	/* Request for info on open files */
#define V4CS_Msg_OpenFiles 19		/* Reply to client request for open file info */
#define V4CS_Msg_ReqFileMap 20		/* Request for configured filemap */
#define V4CS_Msg_FileMap 21		/* Reply to client request for configured filemapMap[fx]. */
#define V4CS_Msg_ReqModFileMap 22	/* Request to modify configured filemap */
#define V4CS_Msg_ModFileMap 23		/* Reply to modify configured filemap request */
#define V4CS_Msg_ReqAddFileToMap 24	/* Request to add file to configured filemap */
#define V4CS_Msg_AddFileToMap 25	/* Reply to add file to configured filemap request */
#define V4CS_Msg_ReqDelFileFromMap 26	/* Request to delete file from configured filemap */
#define V4CS_Msg_DelFileFromMap 27	/* Reply to delete file from configured filemap request */

//#define V4CS_Flag_ISLITTLEENDIAN 0x1	       /* Node seding message is ISLITTLEENDIAN */
//#define V4CS_Flag_MoreToCome 0x2	  /* Another message after this (usually from server to client) */
#define V4CS_Flag_FlipEndian 0x1	/* Have to flip from big-little/little-big endian (client flips, not server) */
#define V4CS_Flag_NoCompress 0x2	/* Don't compress record when passing over network */

#define FLIPSHORT(dst,src) dst = (((src & 0xff) << 8) | ((src & 0xff00) >> 8))
#define FLIPLONG(dst,src) dst = ( ((src & 0xff000000) >> 24) | ((src & 0xff0000) >> 8) | ((src & 0xff00) << 8) | ((src & 0xff) << 24) )

struct V4CS__MsgHeader			/* All messages begin with this structure */
{ BYTE MsgType ;			/* Type of message- see V4CS_Msg_xxx */
  BYTE Flags ;				/* Message flags, see V4CS_Flag_xxx */
  short Bytes ; 			/* Number of bytes in message (including this header) */
} ;

/*	V4CS__FileMap - Map of all file refs, types, and names which may
		       be opened */

#define V4CS_MapFile_Max 200
#define V4CS_MapFileName_Len 63
struct V4CS__FileEnt {
  char FileRef[6] ;
  BYTE FileType ;
  char FileName[V4CS_MapFileName_Len] ;
} ;
struct V4CS__FileEntry {
  int FileRef ;
  BYTE FileType ;
  char FileName[V4CS_MapFileName_Len] ;
} ;
struct V4CS__FileMap {
 short Count ;
 struct V4CS__FileEntry Map[V4CS_MapFile_Max] ;
} ;

/*	Message Formats */

struct V4CS__Msg_OpenFileRef		/* Instructs server to open area with given fileref (return is FileRefAID msg) */
{ struct V4CS__MsgHeader mh ;
  int FileRef ; 			/* File to be opened */
  ETYPE OpenMode ;			/* How to open: V4IS_PCB_OM_xxx */
  ETYPE AccessMode ;			/* Allowed access to file: V4IS_PCB_AM_xxx */
  FLAGS32 ShareMode ;			/* Sharing options */
  char UserName[15] ;			/* User name */
  char NodeName[15] ;			/* Node name */
} ;

struct V4CS__Msg_FileRefAID		/* Return from server with aid for open fileref/area */
{ struct V4CS__MsgHeader mh ;
  AREAID AreaId;
  char AreaFileName[200] ;		/* File name of area containing specified fileref */
} ;

struct V4CS__Msg_CloseAID		/* Instruct server to close specified AreaID */
{ struct V4CS__MsgHeader mh ;
  AREAID AreaId;				/* AreaId to be closed */
} ;

#define V4CS_GetMode_EQ 1		/* Go for exact match */
#define V4CS_GetMode_GE 2		/* Go for key greater-equal */
#define V4CS_GetMode_Next 3		/* Get next record */
#define V4CS_GetMode_Prior 4		/* Get prior record */
#define V4CS_GetMode_BOF 5		/* Get first record */
#define V4CS_GetMode_EOF 6		/* Get last record */
#define V4CS_GetMode_DataOnly 7 	/* Get data-only (scan for just data, no index) */

struct V4CS__Msg_GetKey 		/* Request keyed record from server */
{ struct V4CS__MsgHeader mh ;
  AREAID AreaId;
  BYTE LockFlag ;			/* TRUE if record to be locked, FALSE otherwise */
  BYTE GetMode ;			/* How to search for key: V4CS_GetMode_EQ or _GE */
  short ActualBytes ;			/* Actual number of bytes in key (key below is aligned/padded) */
  struct V4IS__Key key ;		/* Key being requested */
} ;

struct V4CS__Msg_GetNext		/* Get "next" record */
{ struct V4CS__MsgHeader mh ;
  AREAID AreaId;
  BYTE LockFlag ;			/* TRUE if record to be locked, FALSE otherwise */
  BYTE GetMode ;			/* How to get "next" record: V4CS_GetMode_xxx */
  struct V4IS__Key key ;
} ;

#define V4CS_PutMode_Write 1		/* Insert/replace as necessary */
#define V4CS_PutMode_Update 2		/* Update existing */
#define V4CS_PutMode_Insert 3		/* Insert new */
#define V4CS_PutMode_Delete 4		/* Delete current */
#define V4CS_PutMode_DataOnly 5 	/* Data only (no keys) */
#define V4CS_PutMode_Unlock 6		/* Unlock currently (locked) held record */
#define V4CS_PutMode_Cache 7		/* Cached put mode */
#define V4CS_PutMode_Reset 8		/* Reset hash area */
#define V4CS_PutMode_Obsolete 9 	/* Mark data as obsolete */

struct V4CS__Msg_Put			/* Put record (various modes) */
{ struct V4CS__MsgHeader mh ;
  AREAID AreaId;
  short PutMode ;			/* Type of put: V4CS_MsgPut_xxx */
  short CmpMode ;			/* How databuffer is compressed */
  LENMAX PutBytes ;			/* Number of bytes in record */
  BYTE DataBuffer[V4IS_BktSize_Max] ;	/* Data record */
} ;

struct V4CS__Msg_ActionOK		/* Action (OPEN/PUT) went OK (ack from server back to client) */
{ struct V4CS__MsgHeader mh ;
  AREAID AreaId;
  BYTE PutMode ;			/* Type of put: V4CS_MsgPut_xxx */
} ;

struct V4CS__Msg_GetDataId		/* Get record based on Data ID */
{ struct V4CS__MsgHeader mh ;
  AREAID AreaId;
  DATAID DataId ;
  DATAID DataId2 ; 			/* Extra DataId for compat with VMS RFA (which is 4+2 bytes) */
  short LockFlag ;			/* TRUE if record to be locked, FALSE otherwise */
} ;

struct V4CS__Msg_Error			/* Error message from server */
{ struct V4CS__MsgHeader mh ;
  ETYPE VStdErrNum ;			/* V3/V4 Standard error code: V_StdErr_xxx */
  int SysErrorNum ;			/* System error code (if any) */
  char Message[250] ;			/* Error message */
} ;

struct V4CS__Msg_DataRec		/* Data record from server */
{ struct V4CS__MsgHeader mh ;
  AREAID AreaId;
  DATAID DataId ;			/* Data Id */
  DATAID DataId2 ; 			/* Extra DataId for compat with VMS RFA (which is 4+2 bytes) */
  unsigned short DataLen ;		/* Length of data in buffer below */
  BYTE CmpMode ;			/* Compression mode */
  BYTE Buffer[V4IS_BktSize_Max] ;	/* Data record buffer */
} ;

struct V4CS__Msg_ShutdownAID		/* Server shutdown of AID (unrequested) */
{ struct V4CS__MsgHeader mh ;
  AREAID AreaId;				/* Error message should follow */
} ;

struct V4CS__Msg_ShutdownServer 	/* Message indicating server is shutting down */
{ struct V4CS__MsgHeader mh ;
} ;

struct V4CS__Msg_ReqTime		/* Time request to server */
{ struct V4CS__MsgHeader mh ;
} ;

struct V4CS__Msg_TimeStamp		/* Time stamp from server */
{ struct V4CS__MsgHeader mh ;
  UDTVAL TimeFormat ;			/* Format of time */
  char TimeBuffer[100] ;		/* Time in whatever format */
} ;

struct V4CS__Msg_TPort			/* Instructs server to send transaction port number (return is TPort msg) */
{ struct V4CS__MsgHeader mh ;
  int TPort ;				/* Transaction port number */
} ;

struct V4CS__OpenFile
{ int FileRef ; 			/* FileRef for open file */
  SOCKETNUM Socket ;			/* Socket for open file */
  COUNTER GetCount ;			/* Count of gets for open file */
  COUNTER PutCount ;			/* Count of puts for open file */
  char UserName[16] ;			/* UserName for open file */
  char NodeName[16] ;			/* NodeName for open file */
  char FileName[64] ;			/* Name of open file */
} ;

struct V4CS__Msg_OpenFile
{ struct V4CS__MsgHeader mh ;
  INDEX	 Count ;
  INDEX	StartNum,TotalNum ;		/* Starting number & total number (if sending a series of OpenFile messages) */
  struct V4CS__OpenFile Ofiles[V4CS_OpenFile_Max] ;
} ;

struct V4CS__Msg_FileMap
{ struct V4CS__MsgHeader mh ;
  INDEX	 Count ;
  struct V4CS__FileEntry File[V4CS_MapFile_Max] ;
} ;

struct V4CS__Msg_ReqFileMapUpdate
{ struct V4CS__MsgHeader mh ;
  struct V4CS__FileEnt File ;
} ;

struct V4CS__Msg_ReqFileMapDelete
{ struct V4CS__MsgHeader mh ;
  int FileRef ;
} ;

#define V_IPLookup_IPV4 1		/* Return IPV4 address as string */
#define V_IPLookup_IPV6 2		/* Return IPV6 address as string */
#define V_IPLookup_All 3		/* Return all addresses as tab delimited string */
#define V_IPLookup_Connect 4		/* Connect to address, resInt updated with connection socket */
#define V_IPLookup_Listen 5		/* Set up listening socket on port, resInt updated with listening socket */


/*	S T U F F   F O R   C	L A N G U A G E   I N T E R F A C E		*/

#define V4CLI_RES_Undef 0		/* Result is undefined */
#define V4CLI_RES_Int 1 		/* Integer (IntVal) */
#define V4CLI_RES_Dbl 2 		/* Floating Point (DblVal) */
#define V4CLI_RES_String 3		/* Null terminated ASCII string (StrVal) */
#define V4CLI_RES_List 4		/* Result is a list */
#define V4CLI_RES_Ses 5 		/* Structure Element Spec */
#define V4CLI_RES_Des 6 		/* Data Element Spec */

union V4CLI__Value {			/* Format of common "value" */
  int IntVal ;
  double DblVal ;
  char StrVal[2048] ;
  struct V4DPI__Point V4Pnt ;
  struct V4FFI__StructElSpec Ses ;
  struct V4FFI__DataElSpec Des ;
 } ;

/*	G U I	S T R U C T U R E S   F O R   V 4			*/

#define V4GUI_MenuName_Max 32
#define V4GUI_MenuEntry_Max 100

#define V4GUI_MenuFlag_Disable	0x1	/* Item is disabled */
#define V4GUI_MenuFlag_Check 0x2	/* Item is checked */

struct V4GUI__Menu			/* Internal structure to define menus for a window */
{ INDEX Count ;				/* Number of entries below */
  struct {
    char Name[V4GUI_MenuName_Max] ;	/* Name of the menu */
    char Display[V4GUI_MenuName_Max] ;	/* How menu is to be displayed */
    int Id ;				/* Numeric Id for menu (used within GUI code) */
    FLAGS32 Flags ; 			/* Menu flags - see V4GUI_MenuFlag_xxx */
    struct V4GUI__Menu *Menu ;		/* Pointer to nested (popup) menu (Id = 0) */
   } Entry[V4GUI_MenuEntry_Max] ;
} ;

#define V4GUI_TableName_Max 100 	/* Max characters in table name */
#define V4GUI_TableDescription 100
#define V4GUI_TableDesc_Max 1000
#define V4GUI_TableHelp_Max 1000
#define V4GUI_TableButton_Max 20
#define V4GUI_TableDisplay_Max 50
#define V4GUI_TableEntry_Max 255	/* Max number of entries in a table list */

struct V4GUI__Table			/* Internal structure for Table Help */
{ char TableName[V4GUI_TableName_Max] ; /* Name of the table */
  char TableDescription[V4GUI_TableDesc_Max] ; /* Desc to appear at top of window */
  struct V4DPI__LittlePoint TablePoint ;	/* Main "point" associated with this table */
  char TableHelp[V4GUI_TableHelp_Max] ; /* Full help text associated with this table */
  char OKText[V4GUI_TableButton_Max] ;
  char CancelText[V4GUI_TableButton_Max] ;
  char ClearText[V4GUI_TableButton_Max] ;
  char AllText[V4GUI_TableButton_Max] ;
  char HelpText[V4GUI_TableButton_Max] ;
  LOGICAL ChooseMult ;			/* TRUE if OK to select multiple */
  INDEX Count ;
  struct {
    struct V4DPI__LittlePoint Point ;	/* Table Entry point */
    char DisplayText[V4GUI_TableDisplay_Max] ; /* Table entry display name */
    char Name[V4GUI_TableName_Max] ;
    char EntryHelp[V4GUI_TableHelp_Max] ;
    int Internal ;			/* Internal (to databases/programs) value */
    LOGICAL Selected ;			/* TRUE if this element selected */
   } Entry[V4GUI_TableEntry_Max] ;
} ;

/*	S T R U C T U R E S   F O R   D I S P L A Y   W I N D O W S	*/

#define V4GUI_WindowField_Max 75

#define V4GUI_WindowStyle_Main 1	/* Main top-level window */
#define V4GUI_WindowStyle_DataEntry 2	/* Vanilla data entry screen */
#define V4GUI_WindowStyle_Table 3	/* List of table entries */
#define V4GUI_WindowStyle_MessageBox 4	/* Donna's cop-out from hell */
#define V4GUI_WindowStyle_Matrix 5	/* Row-Column Matrix Window */
#define V4GUI_WindowStyle_Max 6 	/* Max number of window styles */

#define V4GUI_PromptMax 40		/* Max number of characters in a prompt */

#define V4GUI_HandleWindow 1		/* Handle type of window */
#define V4GUI_HandleField 2		/* Handle type of field */

struct V4GUI__WindowInfo		/* Global info for a window */
{
  char *ValueBuf ;			/* Pointer to value buffer */
  int NextValueIndex ;			/* Next "available" value index */
  int MaxValueIndex ;			/* Maximum allowed value index */
  int CurrentPromptWidth ;		/* Current "width" of prompts */
  int RowMax ;				/* Maximum number of rows in matrix style window */
  int MenuAsButtons ;			/* If TRUE then display menu as row of buttons */
  int CurrentRowCount ; 		/* Current number of rows (i.e. with data) */
  int MinLineCount ;			/* Minimum number of lines in a row */
  char *DataBuf ;			/* Pointer to data buffer (may change to handle?) */
  int WindowStyle ;			/* Style of window: V4GUI_WindowStyle_xxx */
  int FocusFieldHandle ;		/* V4 Handle of field with focus (HAN_UNUSED if none) */
  int FocusRow ;			/* Row of field with focus (Matrix style) */
  struct V4GUI__Menu *Menu ;		/* Pointer to menu structure (NULL if none for window) */
  int hWindow ; 			/* GUI specific window handle (set by GUI modules) */
  struct V4GUI__Table *TableInfo ;	/* If this is Style_Table window then pointer to info */
  struct V4C__Context *ctx ;
  struct V4DPI__LittlePoint pWindow ;	/* V4 Point of the window */
  char Name[50] ;			/* Name/Title of the window */
} ;

struct V4GUI__FieldInfo 		/* Info for a particular field */
{ int ValueIndex ;			/* Index to current value (via v4gui_ValueIndexToPtr()) */
  int ValueMaxBytes ;			/* Max number of bytes allowed for value */
  int hWindow ; 			/* GUI specific handle for field window */
  int StaticColumn ;			/* If TRUE then a static column (i.e. cannot be changed by user) */
  struct V4DPI__LittlePoint pField ;	/* V4 Point of the field */
  char Prompt[V4GUI_PromptMax] ;	/* Current prompt string */
} ;

#define V4GUI_UOM_Characters 1		/* Value is number of characters */
#define V4GUI_UOM_Lines 2		/* Value is number of lines */
#define V4GUI_UOM_Pixels 3		/* Value is in pixels */
#define V4GUI_UOM_Inches 4 ;
#define V4GUI_UOM_Points 5 ;

struct V4GUI__UOMValue
{ double Value ;
  int UnitOfMeasure ;
} ; typedef struct V4GUI__UOMValue UOMV ;

#define V4GUI_ContentsType_SingleLine 1 /* Contents is single line value */
#define V4GUI_ContentsType_MultiLine 2	/* Contents is (usually) multi-line */
#define V4GUI_ContentsType_Picture 3
#define V4GUI_ContentsType_Movie 4

struct V4GUI__DisplaySetup		/* Common info for entire display window */
{ char Font[20] ;			/* Font name to use */
  int FontPoint ;			/* Pointsize fo use */
} ;

struct V4GUI__PromptInfo		/* Information for a prompt on a field	*/
{ char PromptText[40] ; 		/* Prompt text */
  int Format ;				/* How to format (not in use??) */
} ;

struct V4GUI__FieldSetup		/* Info on field size, etc. for display window */
{ UOMV SuggestedWidth ; 		/* Suggested Width */
  UOMV SuggestedHeight ;		/* Suggested height for the field */
  int ContentsType ;			/* What will go in this field */
} ;

struct V4GUI__DataText			/* Structure with text version of a data field */
{ char Text[512] ;			/* Text value */
  int LinesOfText ;			/* Number of lines of text returned */
} ;


#define V4GUI_GUIMenuComId_Offset 25000 /* Menu/Command ID's greater than this are local to GUI (ignored by ComInt()) */

#define V4GUI_Button_TableOK 20000	/* Clicked OK on table style window button */
#define V4GUI_Button_TableHelp 20001	/* Clicked HELP on table style window button */

#ifdef WINNT
#define V4GUI_MsgMenuCommand WM_COMMAND /* Menu selection from host GUI */
#define MENUNUMBER(ciab) LOWORD(ciab->Arg1) /* GUI specific macro to pull menu number from CmdIntArgBlk */
#define V4GUI_MsgPrivateBase WM_USER	/* Starting message for MKS/MIDAS specific messages */
#endif

#define PB V4GUI_MsgPrivateBase
#define V4GUI_MsgMakeWindow	PB+1	/* Create a window, see wi for details (Arg1=v4handle) */
#define V4GUI_MsgCloseWindow	PB+2	/* Close current window (Arg1 = v4handle of window) */
#define V4GUI_MsgFocusOnField	PB+3	/* Set focus on field (Arg1 = v4handle of field) */
#define V4GUI_MsgQuickMessage	PB+4	/* Blast out error message (Arg1 = message type, ResultMsg is text) */
#define V4GUI_MsgExpandField	PB+5	/* Expand field to new window (Arg1 = field to expand, updated with new window) */
#define V4GUI_MsgModalWindow	PB+6	/* Create modal message/reply window */
#define V4GUI_MsgQuitApp	PB+7	/* Quit application */

#define V4GUI_QuickMessage_Err 1
#define V4GUI_QuickMessage_Help 2
#define V4GUI_QuickMessage_Warn 3
#define V4GUI_QuickMessage_Max 5

struct V4GUI__ComIntArgBlk		/* Argument block for command interpreter */
{ unsigned int MsgType ;		/* Message type: V4GUI_Msgxxx */
  unsigned int Arg1, Arg2, Arg3 ;	/* Message specific arguments */
  struct V4C__Context *ctx ;
  char Header[64] ;			/* Header (window title) info */
  char Text[512] ;			/* Possible resulting (error) message */
} ;

#define V4GUI_MenuClassNewWindow 1	/* Create new window */
#define V4GUI_MenuClassCloseWindow 2	/* Close a window */
#define V4GUI_MenuClassTopField 3	/* Position to top field */
#define V4GUI_MenuClassBottomField 4	/* Bottom field */
#define V4GUI_MenuClassHelpField 5	/* Want help on current field */
#define V4GUI_MenuClassHelpWindow 6	/* Want help on current window */
#define V4GUI_MenuClassExpandField 7	/* Want to exapand current field to a window */
#define V4GUI_MenuClassCloseQuit 8	/* Close/Quit window, no prompting */
#define V4GUI_MenuClassNoOp 9		/* No-op - just ignore this command */
#define V4GUI_MenuClassFile 10		/* File current record */
#define V4GUI_MenuClassDelete 11	/* Delete current record */
#define V4GUI_MenuClassNext 12		/* Get next record */
#define V4GUI_MenuClassQuitApp 13	/* Quit application */

/*	D E F I N I T I O N S	F O R	E L E M E N T	W I N D O W S	*/

#define V4GUI_El_Max 100		/* Max number of fields/elements */
#define V4GUI_Prompt_Max 100
#define V4GUI_PromptShort_Max 20
#define V4GUI_FieldName_Max 50

#define V4GUI_PromptType_Long 0 	/* Prompt with long field/element name */
#define V4GUI_PromptType_Short 1
#define V4GUI_PromptType_Name 2 	/* Prompt with field name */
#define V4GUI_PromptType_Number 0x100	/* Prompt with field number (may be used in conjunction with above) */

#define V4GUI_PromptJustify_Left 0		/* Left justify prompts */
#define V4GUI_PromptJustify_Right 1
#define V4GUI_PromptJustify_Center 2

struct V4GUI__ElList
{
  struct V4GUI__Menu *Menu ;		/* Pointer to window's initial menu structure */
  int PromptType ;			/* How to prompt fields- V4GUI_PromptType_xxx */
  int PromptJustify ;			/* How to justify prompts- V4GUI_PromptJustify_xxx */
  int PromptMaxChars ;			/* Max number of CHARACTERS in prompts */

  int yChar ;				/* Height of characters */
  int xChar ;				/* Average width of characters */
  int Top,Bottom,Left,Right ;		/* Dimensions of window */
  int PromptWidth ;			/* Width of prompt window */
  int DataWidth ;			/* Width of data window */
  int GapWidth ;			/* Width of gap between prompts & data */

  int StartEl ; 			/* Starting element at top of display */
  int EndEl ;

  int ElCount ;
  struct {
   struct V4DPI__LittlePoint ElPoint ;		/* Actual point associated with this element */
   char Prompt[V4GUI_Prompt_Max] ;		/* Long prompt */
   char PromptShort[V4GUI_PromptShort_Max] ;
   char Label[V4GUI_FieldName_Max] ;		/* Prompt label */
   char ElName[V4GUI_FieldName_Max] ;
   int ElHeight ;				/* Height of field window */
   char ElDataType[V4GUI_FieldName_Max] ;	/* Field datatype */
   int ElDataTypeX ;				/* If nonzero then index to internal handler */
   union V4CLI__Value ElVal ;			/* Field value */
  } Entry[V4GUI_El_Max] ;
} ;

/*	V 4   M A S T E R   C U S T O M I Z A T I O N S   T A B L E			*/

#define V4_MasterCustomTableRevLvl 1	/* Current internal V4 table revision level */
#define V4_MCT_InitialDataAlloc 0x10000
#define V4_MCT_ReallocBytes 0x10000	/* Number of bytes increment on reallocate */
#define V4_MCT_HeaderBytes (2 * (sizeof(int))) /* Number of bytes in "header" */
#define V4_MCT_SiteSecretBytes 32
struct V4__MasterCustomTable
 { LENMAX TotalBytes ;			/* Total number of bytes to this structure */
   int RevLevel ;			/* Revision level (in case we change structures and have to reload) */
   INDEX CountryInfoOffset ;		/* Byte offset into struct V4CI__CountryInfo */
   INDEX LanguageInfoOffset ;		/* Byte offset into struct V4LI_LanguageInfo */
   INDEX CalendarInfoOffset ;		/* Byte offset into struct V4LI_CalendarInfo */
   INDEX MsgInfoOffset ;		/* Byte offset into struct V4MSG__MsgInfo */
   INDEX UnicodeOffset ;		/* Byte offset into struct V__UnicodeInfo */
   INDEX ColorOffset ;			/* Byte offset into struct v__ColorValues */
   INDEX HTMLEntityOffset ;		/* Byte offset into struct V4HTML_Entities */
   B64INT iniValXORKey ;		/* Random key for 'encrypting' ini values so they don't appear as clear-text in bin file */
   INDEX iniValsOffset ;		/* Bytes offset into struct V4INI__Values  (UNUSED if none)*/
   LENMAX iniValBytes ;			/* Number of bytes in structure */
   int FutureExpansion[10] ;		/* Reserved for future expansion */
   LENMAX Free ;			/* Number of free bytes in Data[] below */
   INDEX NextAvail ;			/* Next available byte in Data[] below */
   BYTE Data[V4_MCT_InitialDataAlloc] ;	/* Data buffer (will be expanded as is necessary) */
 } ;

/*	D E F I N I T I O N S	F O R	C O U N T R Y	S P E C I F I C   I N F O	*/


struct V4CI__CountryInfo
{ INDEX CurX ;				/* Index to current country below */
  BYTE curDDMap[64] ;			/* Entry is TRUE indicating position for digit delimiters for current country */
  INDEX Count ;				/* Number of countries below */
  struct V4LI_LanguageInfo *li ;	/* Link to laguage info */
  struct V4LI_CalendarInfo *cli ;	/* Link to calendar info */
  struct {
    int UNCode ;			/* U.N. Country Code */
    UCCHAR UNAbbr[8] ;			/* U.N. Country Abbreviation */
    UCCHAR iso2[3] ;			/* ISO 2-character abbreviation */
    UCCHAR UNName[48] ;			/* U.N. Name */
    int IntDialCode ;			/* International Dialing Code */
    unsigned short IDD ;		/* International Direct-Dial Prefix */
    unsigned short NDD ;		/* National Direct-Dial Prefix */
    UCCHAR ITeleMask[32] ;		/* Mask for international calls */
    UCCHAR NTeleMask[32] ;		/* Mask for national calls */
    UCCHAR DateMask[24] ; 		/* Mask for dates */
    UCCHAR MonthMask[24] ;		/* Mask for months */
    UCCHAR DateTimeMask[24] ;		/* Mask for date-times */
    YMDORDER ymdOrder[VCAL_YMDOrderMax] ;	/* Default year-month-day ordering of dates on input */
    unsigned short Calendar ;		/* Calendar - see VCAL_CalType_xxx */
    unsigned short Language ;		/* Language - see xxx */
//    UCCHAR NumInfo[8] ;			/* Numeric info - Currency,delimiter,decimalpoint */
    UCCHAR CurrencySign[4] ;		/* Currency (up to 3 characters) */
    UCCHAR DigiDelim ;			/* Digit delimiter (ex: "," for every 3 digits) */
    UCCHAR RadixPnt ;			/* i.e. decimal point */
    BYTE DDList[8] ;			/* List of number of digits in delimited group, terminated with '\0', last entry applies to all subsequent digits */
    BYTE CurSgnPrefix ;			/* TRUE if currency sign is prefix, FALSE if suffix */
   } Cntry[1] ; 			/* This expands as v4countryinfo.v4i is read in */
} ;

#define V4LI_TZMax 50			/* Max timezones per language */
#define V4LI_DstMax 20			/* Max number of distance abbreviations */
#define V4LI_SpdMax 20			/* Max number of speed abbreviations */
#define V4LI_HTMLMax 16
struct V4LI_LanguageInfo
{ INDEX Count ;				/* Number of Languages Supported Below */
  INDEX CurX ;				/* Index to current */
  struct {
    ETYPE Language ;			/* Language Numeric Code */
    UCCHAR LangId[32] ;			/* Language name/id */
    UCCHAR Yes[12],No[12] ;		/* Yes & No */
    UCCHAR YesUC[12],NoUC[12] ;		/* Yes & No */
    UCCHAR Yesterday[32],Today[32],Tomorrow[32],Now[32] ;
    UCCHAR YesterdayUC[32],TodayUC[32],TomorrowUC[32],NowUC[32] ; /* THESE MUST BE SAME SIZE !!! */
    UCCHAR Hours[32],Minutes[32],Seconds[32] ;
    UCCHAR HoursUC[32],MinutesUC[32],SecondsUC[32] ;		/* THESE MUST BE SAME SIZE !!! */
    UCCHAR True[12],False[12] ;
    UCCHAR TrueUC[12],FalseUC[12] ;
    UCCHAR None[12],None1UC[12],None2UC[12],None3UC[12] ;
    UCCHAR Num120[20][24] ;		/* Numbers 1-20 */
    UCCHAR Tens2090[8][24] ;		/* 20 thru 90 in tens */
    UCCHAR Num000[8][24] ;		/* Hundred, Thousand, ... */
    UCCHAR OrdSfx[10][8] ;		/* Ordinal suffix - (zero)th, (fir)st, (seco)nd, rd, th, ... */
    UCCHAR htmlTag[V4LI_HTMLMax][12] ;	/* List of HTML tags that don't require ending (</xxx>) element (ex: meta, input, br, hr) */
    UCCHAR gcLatitude[8],gcLongitude[8],gcHeight[8],gcTimeZone[8],gcDistance[8],gcSpeed[8],gcDateTime[8],gcAzimuth[8],gcElevation[8] ;
    UCCHAR gcNorth[2],gcSouth[2],gcEast[2],gcWest[2] ;
    struct {
      UCCHAR dstAbbr[7] ;		/* Distance abbreviation */
      short dstIntVal ;			/* Distance internal code - V4DPI_GeoCoordDist_xxx */
     } Dst[V4LI_DstMax] ;
    struct {
      UCCHAR spdAbbr[7] ;		/* Speed abbreviation */
      short spdIntVal ;			/* Speed internal code - V4DPI_GeoCoordSpeed_xxx */
     } Spd[V4LI_SpdMax] ;
    UCCHAR tzLocal[8] ;			/* Abbreviation for 'local' timezone */
    struct {
      UCCHAR tzAbbr[7] ;		/* Timezone abbreviation */
      short tzOffset ;			/* -12..+12 offset from GMT */
     } TZ[V4LI_TZMax] ;
   } LI[1] ;				/* Expands as necessary */
} ;

struct V4LI_CalendarInfo
{ INDEX Count ;				/* Number of Entries */
  INDEX CurX ;				/* Index to current */
  struct {
    ETYPE Language ;			/* Entries in this language */
    ETYPE Calendar ;			/* For this calenadar */
    UCCHAR CalMask[32] ;			/* Format Mask for Output */
    UCCHAR CalTimeMask[32] ;		/* Format Mask for Output if time also included */
    UCCHAR SDaysOfWeek[7][16] ;		/* Short Days of the Week */
    UCCHAR LDaysOfWeek[7][24] ;		/* Long Days of the Week */
    UCCHAR SMonths[24][8] ;		/* Short Months of Year */
    UCCHAR LMonths[24][24] ;		/* Long Months of Year */
   } CLI[1] ;				/* Expands as necessary */
} ;

/*	M E S S A G E	T A B L E   S T R U C T U R E	*/

#define V4MSG_MIMax 1550		/* Max number of messages */

struct V4MSG__MsgInfo {
  INDEX Count ;
  LOGICAL InOrder ; 			/* Set to TRUE if Mnemonics in ascending order (can do fast binary search lookup) */
  INDEX LastMsgIndex ;			/* Index to last message generated */
  BYTE OKtoThrow[V4MSG_MIMax] ;		/* Set to TRUE if corresponding message can be "thrown" as an exception */
  BYTE SetToThrow[V4MSG_MIMax] ;	/* Set to TRUE if corresponding message should be "thrown" as an exception */
  struct {
    char Mnemonic[16] ; 		/* Message name/mnemonic */
    INDEX TextOffset ;			/* Offset into mct->Data[] to UTF-8 string with message */
  } Entry[V4MSG_MIMax] ;
} ;


/*	H A N D L E   S T R U C T U R E S		*/

#define HANType_Unk 0		/* Unknown handle type */

#define HANFlag_IsUpdated 1	/* Handle entry has been updated (by caller) */
#define HANFlag_HasChild 2	/* Handle has children (set via SetParent call) */
#define HANFlag_FreeMem 4	/* Free allocated memory (via Pn) on handle close */

#define HAN_UNUSED -1		/* Special code for unused/bad handle */

struct han__Master		/* Main handle structure */
{ int NextId ;			/* Next available handle id for this group */
  INDEX Count ;			/* Current Group below */
  struct han__Group *Group[255] ; /* Pointers to handle groups */
} ;

struct han__Id			/* Format of a handle ID */
{ BYTE GroupX ;			/* Index to handle group */
  BYTE EntryX ;			/* Entry index within group */
  unsigned short Id ;		/* Unique Id */
} ;

#define HANGroup_Max 255

struct han__Group		/* Group of handles */
{ INDEX Count ;			/* Number below */
  struct {
    BYTE *P1 ;			/* Pointer to whatever */
    int HandleId ;		/* Actual handle ID */
    int ParentHandleId ;	/* Id of parent (HAN_UNUSED if none) */
    int PriorHandle,NextHandle ;/* Handles for linked list */
    FLAGS32 Info ;		/* User/Call specified information about handle */
    BYTE Type ; 		/* Type of handle: HANType_xxx */
    BYTE Flags ;		/* Handle flags: HANFlag_xxx */
   } Entry[HANGroup_Max] ;
} ;

#define V4SS_SSVal_Null 1		/* #NULL! */
#define V4SS_SSVal_Div0 2		/* #DIV/0 */
#define V4SS_SSVal_Value 3		/* #VALUE! */
#define V4SS_SSVal_Ref 4		/* #REF! */
#define V4SS_SSVal_Name 5		/* #NAME! */
#define V4SS_SSVal_Num 6		/* #NUM! */
#define V4SS_SSVal_NA 7 		/* #N/A */
#define V4SS_SSVal_Empty 8		/* no value, blank cell */
#define V4SS_SSVal_Row 9		/* Current Row */
#define V4SS_SSVal_Column 10		/* Current Column */

/*	O U T P U T   T Y P E S 		*/

#define VOUT_Status 1			/* Status Message */
#define VOUT_Data 2			/* Data */
#define VOUT_Debug 3			/* Debug Message */
#define VOUT_Prompt 4
#define VOUT_Err 5
#define VOUT_Trace 6			/* Debug Trace Messages */
#define VOUT_Warn 7			/* V4 Warnings */
#define VOUT_Progress 8 		/* V4 Progress Messages */
#define VOUT_StreamMax 10		/* Max number above cannot exceed this! */

//#define V4_SummaryPrefix "~V4Sum\t"	/* Prefix to V4 end-of-job summary (see Summary() module) */
//#define V4_DeltaPrefix "~V4Delta\t"	/* Prefix to V4 delta progress string (see Summary() module) */

//#define VOUT_MethodNone 0		/* Nothing assigned */
//#define VOUT_MethodConsole 1		/* Default - to console */
//#define VOUT_MethodFile 2		/* To a file */
//#define VOUT_MethodSocket 3		/* To a socket */
//#define VOUT_MethodNull 4		/* To null device (bit bucket) */

#define VOUT_FileType_StdOut 1		/* To process StdOut handle/pointer */
#define VOUT_FileType_Console 2 	/* Process Console */
#define VOUT_FileType_None 3		/* Null output (bit-bucket) */
#define VOUT_FileType_File 4		/* Plain old file */
#define VOUT_FileType_Socket 5		/* TCP/IP Socket */
#define VOUT_FileType_BigText 6 	/* A V4/BigText Point */
#define VOUT_FileType_Buffer 7		/* All output is internally buffered */
#define VOUT_FileType_BufJSON 8		/* All output is internally buffered with JSON output */

#define VOUT_FileType_Buffer_Max 0x20000	/* Size of internal buffer */

/*	R E G I S T E R   E R R O R   M A C R O 	*/
#define REGISTER_ERROR(ECODE) if (!ctx->disableFailures) v4trace_RegisterError(ctx) ;

/*	I N T E R N A L   C O M R E S S I O N	S T U F F	*/

#define V_COMPRESS_DfltBlock 128000		/* Default block size for compression */
#define V_COMPRESS_MaxTextLine 2048		/* Maximum line of text for text compression */
struct VFC__LZRW1Hdr {
 LENMAX SrcBytes ; 				/* Number of bytes in source */
 LENMAX CmpBytes ; 				/* Number of compressed bytes to follow */
 } ;

struct V_COMPRESS_IncludeInfo			/* Structure holding info about current include file */
{ LENMAX BufMax ;				/* Max size of buffers */
  BYTE *CmpBuf ;				/* Holds compressed data (read from include file) */
  BYTE *Buf ;					/* Hold uncompressed data */
  BYTE *curPtr ;				/* Pointer to current line in Buf (NULL if end of buffer) */
} ;

/*	E N C O D E  /  D E C O D E   S T U F F		*/

#define V4_EDMETHOD_DES       0               /*  DES algorithm                    */
#define V4_EDMETHOD_XOR       1               /*  A basic XOR algorithm            */

struct VED__desKeyBlock		/* 8 byte block for DES keys */
{ BYTE stuff[8] ;
} ;

#pragma pack(8)
struct VED__desKeySched		/* Array of DES keys filled in on DES init */
{ struct VED__desKeyBlock sched[16] ;
} ;

/*	T R A V E L I N G   S A L E S M A N   S T U F F		*/

#define VTS_MAX_NODES 2500	/* Maximum number of nodes allowed */
//#define DISTANCE(_X,_Y) graph->distance[_X][_Y]
#define DISTANCE(_X,_Y) graph->distance[_X*graph->Nodes + _Y]
struct VTS__Graph {
  INDEX Nodes ;			/* Number of nodes in current graph */
//  double distance[VTS_MAX_NODES][VTS_MAX_NODES] ;	/* Distance between two points */
  double distance[0] ;		/* Distance between two points */
} ;

struct VTS__Path {
  int From[VTS_MAX_NODES], To[VTS_MAX_NODES] ;
  double Length ;
} ;

/*	V 4   L O C K I N G   P R I M I T I V E   S T R U C T U R E S		*/

#define PROCSTILLALIVE(pid) v4pl_isProcessGone(pid)

#define WAITMAXFOREVER 1E20
#define WAITMAXNONE 0
#define V4PL_procIdBase 0x70000000	/* Starting id for process (so we can tell difference between lock & process Id) */

#define V4PL_maxNumberOfLocks 50	/* Max number of locks */
#define V4PL_maxNumberOfProcs 100	/* Max number of logged in processes */
#define V4PL_maxOwnersOfLock 128	/* Max number of concurrent owners of a lock */
#define V4PL_maxWaitOnLock 64		/* Max number of wait processes for a lock */
#define V4PL_maxLocksOneCall 32		/* Maximum number of locks to grab in one call */

#define V4PL_lockNameMax 32		/* Max characters in lock name */
#define V4PL_lockNameBufMax 256		/* Max characters in lock name(s) buffer in gla->lockNameBuf */
#define V4PL_procNameMax 64		/* Max characters in process name */

/*	V4PL__ProcLock - Process Locking Segment/Table for V4 inter-process synchronization & locking */
/*	Note: V4PL__ProcLock allocated for both lock[] substructure and V4PL__ProcInfo structure that immediately follows */
/*	Max locks and Max users determined when lock table/segment initially created */

struct V4PL__ProcLock {
  DCLSPINLOCK lockOnLockTbl ;		/* Spin lock on this table */
  COUNTER bytesSegment ;		/* Number of bytes allocated to this segment */
  INDEX offsetInfo ;			/* Offset to V4PL__ProcInfo structure */
  UNIQUEID luProcId ;			/* Last used unique process id */
  UNIQUEID luLockId ;			/* Last used unique lock id */
  COUNTER procMax ;			/* Max number of processes below (within V4PL__ProcInfo table) */
  INDEX procCnt ;			/* Number of registered processes below */
  COUNTER lockMax ;			/* Maximum number of locks we can have */
  COUNTER lockCnt ;			/* Number of locks below */
  struct {
    LOGICAL surviveProc ;		/* If TRUE then lock survives after death of process - must be explicitly released (or timeout) */
    time_t releaseTime ;		/* If not zero then time to auto-release lock if requested by another process */
    UNIQUEID lockId ;			/* Unique lock id */
    UCCHAR lockName[V4PL_lockNameMax] ;	/* Lock name */
    ETYPE lockMode ;			/* Read/Write mode of lock - V4MM_LockMode_xxx */
    COUNTER ownerCnt ;			/* Number of current owners of lock */
    UNIQUEID ownerList[V4PL_maxOwnersOfLock] ;	/* procId into V4PL__ProcInfo structure */
    COUNTER waitCnt ;			/* Number of processes waiting for lock */
    UNIQUEID waitList[V4PL_maxWaitOnLock] ;	/* procId into V4PL__ProcInfo structure */
    } lock[0] ;
} ;

struct V4PL__ProcInfo {
  struct {
    PID procPid ;			/* Process pid */
    UNIQUEID procId ;			/* Unique process (thread) Id */
    UCCHAR procName[V4PL_procNameMax] ;	/* Optional process name */
    } info[0] ;
} ;

struct V4PL__grabLockArgs {
  struct V4C__Context *ctx ;		/* Current context */
  struct V4PL__ProcLock *vpl ;		/* pointer to (struct V4PL__ProcLock) global lock table */
  UNIQUEID procId ;			/* Process Id of calling process */
  UCCHAR lockNameBuf[V4PL_lockNameBufMax] ; /* Lock name(s) buffer */
  ETYPE lockMode ;			/* V4MM_LockMode_xxx (xxx = Read/Write) */
  int tryInterval ;			/* Retry interval in milliseconds (UNUSED for default) */
  double waitMax ;			/* Max seconds to wait for lock (then return error), WAITMAXFOREVER for infinite, WAITMAXNONE for immediate return */
  DIMID dimId ;				/* Dimension to update in context (FALSE=waiting, TRUE=got lock), UNUSED for none */
  LOGICAL surviveProc ;			/* If TRUE then lock survives after death of process - must be explicitly released (or timeout) */
  COUNTER holdSeconds ;			/* Max number of seconds to hold lock before auto-release (0 for no auto-release) */
  COUNTER lockIdCnt ;			/* Number of lockIds in array below */
  UNIQUEID lockIds[V4PL_maxLocksOneCall] ;	/* Updated with lockId(s) grabbed */	
} ;

/*	R P T ( )   S T R U C T U R E S		*/

#define V4RPT_Type_V4R 0			/* Normal/default V4R output */
#define V4RPT_Type_Tab 1			/* Output tab delimited text (heading & detail lines only) */
#define V4RPT_Type_CSV 2			/* Same as tab but with commas */
#define V4RPT_Type_HTML 3			/* Output in HTML format */
#define V4RPT_Type_XML 4			/* Output in XML format (support this ?) */

#define V4RPT_LineType_Default 1		/* Default Format (determined once at begin of report) */
#define V4RPT_LineType_Heading 2		/* Heading line */
#define V4RPT_LineType_Detail 3			/* Detail line */
#define V4RPT_LineType_Recap 4			/* Recap line */
#define V4RPT_LineType_Footer 5			/* End-of-report footer/total line */

#define V4RPT_ColMax 512			/* Max number of columns */
#define V4RPT_xmlTagMax 32			/* Max number of xml/html tags to wrap around this report */
#define V4RPT_TitleMax 32			/* Max number of begin lines (for common top-of-page) */
#define V4RPT_BeginMax 16			/* Max number of title lines */
#define V4RPT_URLBaseMax 10			/* Max number of URL bases (digits 0-9) */
#define V4RPT_CSSMax 32				/* Max number of CSS entries */
#define V4RPT_HTMLEntryMax 2048			/* Max number of characters in HTML entry */
#define V4RPT_HTMLMax 32			/* Max number of embedded HTML entries */
#define V4RPT_URLSizeMax 1024			/* Max number of bytes in nextURL */
#define V4RPT_TargetMax 128			/* Max size of URL target (window) */
#define V4RPT_NoteSizeMax 1024			/* Max number of bytes in nextNote */
#define V4RPT_LinkLibMax 32			/* Max number of linked JS/CSS libraries */
#define V4RPT_KeySizeMax 1024			/* Max number of bytes in a row 'key' */
#define V4RPT_RptMemChunkSize 8092

#define V4RH_XLSXURL_none 1			/* Don't embed hyperlinks in XLSX output */
#define V4RH_XLSXURL_exOnly 2			/* Only include external (non-V4) links */
#define V4RH_XLSXURL_userKey 3			/* Embed with running user's sesKey (NOT SECURE) */
#define V4RH_XLSXURL_secure 4			/* Embed with separate sesKey & run via [func:xlsxDrilldown] */

struct V4RPT__RptInfo {
  struct V4RPT__RptInfo *rriNext ;		/* If not NULL then link to next one */
  struct V4RPT__RptMemChunk *rriMC ;		/* Current buffer memory chunk */
  struct V4DPI__LittlePoint idPnt ;		/* Optional Id (via Out::xxx tag) of this report */
  ETYPE rptType ;				/* Type of report, see V4RPT_Type_xxx */
  ETYPE lineType ;				/* What type of line is being filled in below, see V4RPT_LineType_xxx */
  LOGICAL didInit ;				/* TRUE if whatever first-time initialization has been performed */
  LOGICAL didCountryLanguage ;			/* TRUE if output country/language to v4r */
  COUNTER hdrLines ;				/* Number of header lines output */
  COUNTER dtlLines ;				/* Number of lines output so far */
  ETYPE gridOption ;				/* Grid-line option */
  ETYPE hyperLinks ;				/* How embedded hyperlinks to be handled in XLSX (V4RH_XLSXURL_xxx) */
  INDEX filex ;					/* Index to output stream assigned to this report */
  INDEX initialLevel ;				/* Initial hierarchy level (0 is default) */
  COUNTER xmlCount ;				/* Count of xml tags we have to envelope this report */
  UCCHAR *xmlTags[V4RPT_xmlTagMax] ;		/* If non-null then an xml tag */
  COUNTER htmlCount ;				/* Count of html tags we have to envelope this report */
  UCCHAR *htmlTags[V4RPT_xmlTagMax] ;		/* If non-null then an html tag */
  COUNTER titleCount ;				/* Count of title lines for this report */
  UCCHAR *valTitle[V4RPT_TitleMax] ;		/* Points to n'th title line */
  COUNTER beginCount ;				/* Number of valBegin lines below */
  ETYPE beginType ;				/* if 1 then valBegin contains name of file, if 2 then valBegin contains strings */
  UCCHAR *valBegin[V4RPT_BeginMax] ;		/* Lines/output to begin the report */
  UCCHAR *privileges ;				/* If not null then privileges necessary to view this report */
  UCCHAR *user ;				/* If not null then the user generating report */
  UCCHAR *setup ;				/* The setup (context) used to generate this report */
  UCCHAR *server ;				/* The HTTP server processing the report request */
  UCCHAR *jobId ;				/* The job 'id' of the request */
  UCCHAR *name ;				/* The name of the report */
  UCCHAR *memo ;				/* If not NULL then a text memo for the report */
  UCCHAR *valId ;				/* If not null then points to report Id */
  UCCHAR *cookieValues ;			/* If not null then a semi-colon delimited list of cookies */
  COUNTER scale ;				/* If not 0 then scaling factor (-1 = fit to page) */
  ETYPE dfltFormat ;				/* 0=Standard, -1=Datatable, +n=number of lines - How to initially display HTML report */
  COUNTER bottomMax,topMax ;			/* Max number of characters allocated in bottomStr/top*/
  UCCHAR *bottomStr,*topStr ;			/* If not null then '\r' delimited set of lines to be appended at bottom/top of report */
  int sheetCount ;				/* Number of sheets so far (below is current one) */
  LOGICAL sheetInit ;				/* True if current sheet has been initialized */
  LOGICAL sheetClosed ;				/* True if current sheet has already been closed (Close::Sheet) */
  UCCHAR sheetName[256] ;			/* If not empty then current sheet name */
  UCCHAR sheetXMLName[256] ;			/* If not blank then name of XML segment for current sheet */
  UCCHAR rowInfo[512] ;				/* If not blank then row identification info */
  UCCHAR selectInfo[512] ;			/* If not blank then row selection info */
  LOGICAL urlBaseSent[V4RPT_URLBaseMax] ;	/* TRUE iff sent urlBase info off */
  UCCHAR *urlBase[V4RPT_URLBaseMax] ;		/* The base URL segments */
  COUNTER numCols ;				/* Number of columns below */
  int cssValSent[V4RPT_CSSMax] ;		/* TRUE iff sent cssVal info off */
  UCCHAR *cssVal[V4RPT_CSSMax] ;		/* CSS entries */
  INDEX linkLibCount ;
  UCCHAR *linkLib[V4RPT_LinkLibMax] ;		/* Linked JS/CSS libraries */
  INDEX embedCount ;				/* Number of HTML entries below (reset on each call to Rpt()) */
  UCCHAR *embedVal[V4RPT_HTMLMax] ;		/* Embedded HTML entries */
  UCCHAR *nextURL ;				/* If non NULL, non blank then URL for next field */
  UCCHAR *nextTarget ;				/* If non NULL then target window for nextURL */
  UCCHAR *nextNote ;				/* If non NULL, non blank then 'note' text for next field */
  UCCHAR *nextImage ;				/* If non NULL, non blank then image to reside in next field */
//  UCCHAR *nextKey ;				/* If non NULL, non blank then key associated with current row */
  struct {
    struct V4DPI__LittlePoint idCol ;		/* Unique column id */
    LOGICAL disable ;				/* If TRUE then disable output of this column */
    COUNTER nthGrp ;				/* If non zero then this column id represents the nth column of a group */
    COUNTER totalGrp ;				/* Total number in group (first column in group will have nthGrp=1, last column nthGrp=totalGrp) */
    INDEX colRedef ;				/* If not UNUSED then column index this redefines */
    INDEX dfltAlign ;				/* Default horizontal alignment (V4SS_Align_xxx) */
    LENMAX maxChar ;				/* Maximum number of characters allocated below */
    LENMAX maxURL ;				/* Maximum number of characters allocated for URL below */
    LENMAX maxNote ;				/* Maximum number of characters allocated for Note below */
    COUNTER spanCol ;				/* If non-zero then number of columns this column should span */
    DIMID valDim ;				/* Dimension of value (used in RCV to supply defaults) */
    struct V4DPI__Point *ptCol ;		/* Value of this column as V4 point */
    UCCHAR *valURL ;				/* If not NULL then URL to be linked to this column value */
    UCCHAR *valTarget ;				/* If not NULL then target (window) for this URL */
    UCCHAR *valNote ;				/* If not NULL then text 'note' to be linked to this column value */
    UCCHAR *valImage ;				/* If not NULL then path to image for this column value */
    UCCHAR *xmlName ;				/* If not NULL then pointer to XML name for column (otherwise use idCol as name) */
    struct V4SS__FormatSpec *vfsDflt ;		/* If not NULL then default formatting for ths column */
    struct V4SS__FormatSpec *vfs ;		/* If not NULL then formatting for ths column */
   } col[V4RPT_ColMax] ;
} ;

struct V4RPT__RptMemChunk {
  struct V4RPT__RptMemChunk *priorMC ;		/* If non-null then link to prior (full) chunk */
  LENMAX bytesUsed ;				/* Number bytes used in chunk */
  BYTE mcBuf[V4RPT_RptMemChunkSize] ;		/* Buffer */
} ;

struct V4RPT__URLSpec {				/* URL Structure */
  UCCHAR urlText[V4RPT_URLSizeMax] ;		/* URL Text (or suffix if begins with '+') */
  UCCHAR target[V4RPT_TargetMax] ;		/* Target frame or window (if blank then overwite current) */
} ;

/*	E R R O R   C O D E S			*/

#define V4E_Warn -1			/* Output as warning, not Error */
#define V4E_Alert -2			/* Output as alert, not Error */

#define V4E_AIDNOTINMM 200000		/* Could not locate aid (%d) in mmm */
#define V4E_ALLNOTALLOWED 200010		/* Cannot have {ALL} in intersection")  */
#define V4E_ALLOCFAIL 200020		/* Area (%d %s) Memory Allocation failure bkt=%d */
#define V4E_AREALOCKHUNG 200060 	/* Area (%d %s) Lock table appears hung (%d) by process (%d) */
#define V4E_AREANOBIND 200070		/* Could not find any Area available for binding updates")  */
#define V4E_AREANODELETE 200080 	/* Delete operation not permitted for this area (%s) */
#define V4E_AREANOFFI 200090		/* No foreign file key information in area")  */
#define V4E_AREANOFFS 200100		/* Could not locate FFS record for FileRef in area (%s) */
#define V4E_AREANOGET 200110		/* Get operation not allowed on this area (%s) */
#define V4E_AREANOKEY 200120		/* No such key in area")  */
#define V4E_AREANOPUT 200130		/* Insert operation not permitted for this area (%s) */
#define V4E_AREANOTFOUNDLR 200140		/* LockRelease: Could not locate area (%d) */
#define V4E_AREANOTFOUNDLS 200150		/* LockSomething: Could not locate area (%d) */
#define V4E_AREANOTFOUNDSSI 200160		/* ShareSegInit: Could not locate area (%d) */
#define V4E_AREANOTGLOBAL 200170		/* Area (%d %s) not global */
#define V4E_AREANOTHASH1 200180 	/* Area (%s) not hashe */
#define V4E_AREANOTHASH2 200190 	/* Area (%s) not hashe */
#define V4E_AREANOUPD 200200		/* Update operation not permitted for this area (%s) */
#define V4E_AREANOWRT 200210		/* Write (Update+Insert) not permitted for this area (%s) */
#define V4E_BADDATAID 200280		/* Data record associated with dataid (%x) cannot be found */
#define V4E_BADDDATAID 200290		/* Area (%d %s) data dataid (%x) cannot be found */
#define V4E_BADIDATAID 200295		/* Area (%d %s) index dataid (%x) cannot be found */
#define V4E_BADKEYINLINK 200330 	/*  */
#define V4E_BADKEYTYPE 200340		/* Unknown key type (%d) */
#define V4E_BADPOSKEYRES 200410 	/* Unknown result code from PositionKey (%d) */
#define V4E_BADPRIMEKEYTYPE 200420		/* Invalid primary key type (%d) */
#define V4E_BKTFLUSHFAIL 200520 	/* Area (%d %s) fflush error (%d) on bucket (%d) */
#define V4E_BKTHDRBAD 200530		/* Area (%d %s) Bucket header (%d) does not match request bucket (%d) */
#define V4E_BKTINMMM 200540		/* Bucket (%d) already in mmm */
#define V4E_BKTINUSESNGL 200550 	/* Area (%d %s) Bucket (%d) marked as INUSE (%d) on a single process area */
#define V4E_BKTNOTDATA 200560		/* Bucket (%d) not type data (%d) */
#define V4E_BKTNOTMATCH 200570		/* Area (%d %s) Bucket (%s/%d) header (%d) does not match (%d) */
#define V4E_BKTREADFAIL 200580		/* Area (%d %s) ReadFile failure (%d) for bucket (%d/%d) */
#define V4E_BKTSEEKFAIL 200590		/* Area (%d %s) SetFilePointer failure for bucket (%d) */
#define V4E_BKTSEEKFAIL1 200600 	/* Area (%d %s) SetFilePointer failure for bucket (%d/%d) */
#define V4E_BKTTOOSMALL 200610		/* Bucket size is too small for data-in-index Insert")  */
#define V4E_BKTWRITEFAIL 200620 	/* Area (%d %s) WriteFile failure (%d) for bucket (%d) */
#define V4E_CHNGPRIMONREP 200650		/* Cannot change primary key on REPLACE")  */
#define V4E_DATAIDBAD 200760		/* Not valid Id in DataId")  */
#define V4E_DATAIDNOTIDX 200770 	/* Data record with dataid (%d) cannot be found in index */
#define V4E_DBEHLENZERO 200780		/* Data Entry Header cannot be 0 in Bkt (%d) */
#define V4E_DIRTREEBAD 200850		/* Directory Tree out of whack!")  */
#define V4E_DUPTHREADHANDLE 200860		/* Error (%d) duplicating current thread handle */
#define V4E_EOF 200870			/* End of file reached")  */
#define V4E_EOFINCOMMENT 200880 	/* Reached end of source file while in comment")  */
#define V4E_EOFINIF 200890			/* Reached end of source file while still in IF statement")  */
#define V4E_EOMINIF 200900			/* Reached end of macro while still in IF:%s statement */
#define V4E_ERRORS 200910			/* Detected %d errors reformating/verifying area */
#define V4E_EXCEEDLVLMAX 200950 	/*  */
#define V4E_FILEREFNOTINAREA 200960		/* Could not locate fileref (%d) in area */
#define V4E_FILEREFNOTINPCB 200970		/* Could not locate FileRef (%d) in PCB (%s) */
#define V4E_FIRSTNOTSAMEASPARENT 200980 	/*  */
#define V4E_FIRSTNOTSAMEASPARENT1 200990		/*  */
#define V4E_FLUSHBKT0 201000		/* Area (%d %s) Cannot flush bucket 0 (root) */
#define V4E_FOPENERR 201010		/* Could not write/append to file %s (%d) */
#define V4E_GET 201030			/* V4IS/RMS Area (%s */
#define V4E_GETHOSTADDR 201040		/* Error (%d) obtaining network address of host (gethostbyname(%s)) */
#define V4E_GRABHUNG 201050		/* Global buffer grab appears to be hung")  */
#define V4E_HITEOF 201060			/* EOF hit")  */
#define V4E_IKDEBAD 201090			/* Unknown EntryType")  */
#define V4E_INPSTRTOOBIG 201100 	/* Nested input line length (%d) exceeds maximum allowed (%d) */
#define V4E_INSRECEXISTS 201110 	/* Record already exists with specified key")  */
#define V4E_INVAREAID 201150		/* Invalid AreaId on this PCB (%s) */
#define V4E_INVBKTHDR 201190		/*  */
#define V4E_INVBKTNUM 201200		/* Area (%d %s) Bucket number (%d) cannot be less than 0 */
#define V4E_INVBKTSIZE 201210		/* Bucket size (%d) must be between %d and %d */
#define V4E_INVBKTSIZENUM 201220		/* Expecting an integer bucket size number")  */
#define V4E_INVBKTTYPE 201230		/* Unknown bucket type (%d) */
#define V4E_INVCOMPMODE 201250		/* Invalid compression mode (%d) */
#define V4E_INVDATAIDBKT 201280 	/* Area (%d %s) Bucket (%d */
#define V4E_INVDATAIDSEQ 201290 	/* Area (%d %s) Bucket (%d */
#define V4E_INVDATAINDEX 201300 	/* Area (%d %s) Bucket (%d) Key0x (%d) Key%dx (%d) bad */
#define V4E_INVDATALEN 201310		/* Invalid data length (%d) to copy to user buffer */
#define V4E_INVDATATOP 201320		/* Area (%d %s) Bucket (%d) DataTop (%d) bad */
#define V4E_INVDBEHLEN 201330		/* Area (%d %s) Bucket (%d) invalid dbeh->Bytes */
#define V4E_INVDBEHLENIS0 201340		/* Area (%d %s) Bucket (%d) invalid dbeh->Bytes */
#define V4E_INVDBH 201350			/* Area (%d %s) Bkt (%d) Data Header First (%d) Free (%d) Bktsize (%d) bad */
#define V4E_INVDBLPNT 201360		/* Point cannot be coerced to (double)")  */
#define V4E_INVDBLUPD 201370		/* Point cannot be updated with number")  */
#define V4E_INVENDBX 201450		/* Area (%d %s) Bucket (%d */
#define V4E_INVFILEEXT 201460		/* Cannot output to file (%s) with .dat extension */
#define V4E_INVFORHASH 201470		/* Invalid v4is_Get mode (0x%x) for hashed area (%s) */
#define V4E_INVFREEBYTES 201490 	/* Area (%d %s) Bucket (%d) Free Bytes (%d) exceeds bucket size (%d) */
#define V4E_INVGETMODE 201500		/* Invalid GetMode (%d) */
#define V4E_INVGLBBKT 201510		/*  */
#define V4E_INVHASHFUNC 201520		/* Invalid hash function (%d) */
#define V4E_INVHASHGETMODE 201530		/* Function (%x) not allowed for hashed area(%s) */
#define V4E_INVHASHMODE 201540		/* Invalid hash mode (%d) */
#define V4E_INVIBH 201550			/* Area (%d %s) Bucket (%d) KeyTop (%d) KeyNum (%d) Free (%d) Size (%d) bad */
#define V4E_INVIKDE 201560			/* Invalid IKDE Entry Type (%d) in Bucket % */
#define V4E_INVIKDETYPE 201570		/* Unknown EntryType")  */
#define V4E_INVKEYMODE 201610		/* Invalid KeyMode (%d) */
#define V4E_INVLINK 201620			/* Invalid link from parent to child")  */
#define V4E_INVLISTACTCODE 201630		/* Invalid action code (%d) */
#define V4E_INVLISTTYPE 201640		/* Unknown list type (%d) */
#define V4E_INVLOCKMODE 201650		/* Invalid Lock Mode (%d) */
#define V4E_INVLOCKX 201660		/* Area (%d %s) Unique Lock ID (%d) passed from (%s) to LockRelease not found */
#define V4E_INVMAXRECS 201690		/*  */
#define V4E_INVMODE 201700			/* Invalid mode")  */
#define V4E_INVOCBTYPE 201720		/* Invalid ocbType (%d) for area (%s) */
#define V4E_INVOPCODE 201730		/* Invalid (%d) opcode in expression */
#define V4E_INVOPNMODE 201740		/* Open Mode not yet supported (%d) */
#define V4E_INVOUTFILE 201750		/* Could not write/append to file %s (%d) */
#define V4E_INVPARAMINMACRO 201760		/* Invalid {xxx} in dimension macro")  */
#define V4E_INVPOSFLAG 201800		/* Invalid Position Flag (%d) */
#define V4E_INVPOSIND 201810		/* Invalid position indicator (%d) in call */
#define V4E_INVPTTYPE 201820		/* Isct (arg %d) not valid type for sort */
#define V4E_INVPUTMODE 201830		/* Invalid PUT mode on %s (%d) */
#define V4E_INVRECBYTES 201860		/*  */
#define V4E_INVSEQHDR 201880		/* Invalid SeqOnly Header- BktNum <> 0")  */
#define V4E_INVSYNEXP 201970		/* Invalid expression syntax")  */
#define V4E_INVTKNLVLMODE 201990		/* Invalid token level (%d) mode (%d) */
#define V4E_ISSEQONLY 202060		/* Cannot reformat SeqOnly files")  */
#define V4E_ITEMLOCKED 202070		/* ebuf) ; */
#define V4E_KEYNUMTOOBIG 202080 	/* KeyNum too big for specified foreign file FileRef")  */
#define V4E_KEYPREFIXSEQERR 202090		/* Key Prefix Sequence Error: %x s/b greater than %x */
#define V4E_LINKNOTINDEX 202110 	/*  */
#define V4E_LOADMACFAIL 202180		/* Could not load macro (%s) from V4 area (%d) */
#define V4E_LOCKED 202190			/* Record is locked")  */
#define V4E_LOCKNOTRELPID 202200		/*  */
#define V4E_LOCKOUTERR 202210		/* Cannot PUT to area (%d %s) - Area has been locked-out */
#define V4E_LOCKTBLFULL 202220		/* Area (%d %s) Lock table for is full (%d) */
#define V4E_MAKEINVBKTTYPE 202260		/* Invalid bucket type (%d) for area %d */
#define V4E_MAXAREALEN 202280		/* Record length exceeds maximum allowed for this area (%d > %d) */
#define V4E_MAXAREAS 202290		/* Too many areas (%d) */
#define V4E_MAXBKTS 202300			/* Area (%d %s) Too many buckets (%d) */
#define V4E_MAXCTXFRAMES 202310 	/* Too many existing frames (%d */
#define V4E_MAXGLBPIDS 202340		/* Exceeded max number of concurrent pids to global segment: %s */
#define V4E_MAXLEVELS 202350		/* Exceeded max number of nested parser input levels")  */
#define V4E_MAXLISTAREA 202360		/* Exceeded max number of context points with bind value areas")  */
#define V4E_MAXLOCKS 202380		/* Area (%d %s) - Exceeded lock maximum (%d) */
#define V4E_MAXRECORD 202390		/* Area (%s) at record max (%d */
#define V4E_MAXWASTE 202400		/* Wasted space (%d) exceeds allowed threshold (%d mb) */
#define V4E_MEMALLOCFAIL 202410 	/* Could not malloc requested memory")  */
#define V4E_MMMNOTINIT 202450		/* MM not initialized")  */
#define V4E_MUSTBENEW 202470		/* File (%s) already exist */
#define V4E_NOALLOCGLBL 202520		/* Area (%d %s) Could not allocate global buffer (%d) */
#define V4E_NOCREFILMAP 202590		/* Error (%d) in CreateFileMapping of %s */
#define V4E_NOCURPOS 202600		/* Cannot position NEXT/PRIOR- No current position")  */
#define V4E_NOCURRECREP 202610		/* No current record to replace")  */
#define V4E_NODATAID 202620		/* No such hash entry in area (%s) with dataid (%x) */
#define V4E_NOFILEACCESS 202690 	/* Could not access file: %s (%d) */
#define V4E_NOFILECREATE 202700 	/* Could not create file: %s (%d) */
#define V4E_NOFINDKEY 202710		/* Area (%d) Could not locate specified key to replace DataId */
#define V4E_NOHASHINFO 202720		/* Area (%s) - missing HashInfo */
#define V4E_NOKEYINFO 202770		/* Area (%s) - no key info specified */
#define V4E_NOKEYPOSTODEL 202780		/* Could not position to key in index to delete")  */
#define V4E_NOKEYWITHDATAID 202790		/* Could not locate key with specified DataId")  */
#define V4E_NOMAPVIEW 202830		/* Error (%d) in MapViewOfFile for %s */
#define V4E_NOPARAM 202840			/* Parameter has not been defined/set")  */
#define V4E_NOPOSPAR 202850		/*  */
#define V4E_NORECORDDEL 202870		/* No record found in area (%s) to delete */
#define V4E_NORECORDGET 202880		/* Record not found in area (%s) */
#define V4E_NORECORDUPD 202890		/* No record found in area (%s) to update */
#define V4E_NORECTODEL 202900		/* No current record to delete")  */
#define V4E_NOSEG 202910			/* Error (%d) attaching to open-close segment */
#define V4E_NOSEG1 202920			/* Error1 (%d) attaching to open-close segment */
#define V4E_NOSETMAC 202930		/* Cannot invoke macro without prior local macro or SET MacroCall")  */
#define V4E_NOSSRBRACE 202940		/* Missing ending \"}\" in special symbol")  */
#define V4E_NOV3LOADUNS 203130		/* Load of V3 module not supported outside of V3")  */
#define V4E_NOV3UNLOADUNS 203140		/* UnLoad of V3 module not supported outside of V3")  */
#define V4E_NOVALREC 203150		/* Could not access value record")  */
#define V4E_OCCREFILMAP 203170		/* Error (%d) in CreateFileMapping of opncls.seg */
#define V4E_OCLOCKHUNG 203180		/* Open-Close Lock table appears hung (%d) by process (%d) */
#define V4E_OPENTOOMANYLVLS 203190		/* Area (%s) cannot be opened for update- must reformat index first! */
#define V4E_PNTIDXNOTVALID 203220		/* Index (%d) no longer valid (slot has been re-used) */
#define V4E_POSPASTEOF 203270		/* No such key & positioned past end of file")  */
#define V4E_PUTAPND 203290			/* Append is only Put mode allowed for this area (%s) */
#define V4E_PUTIDXMLTKEY 203300 	/* Cannot PUT data in index bucket with multiple keys (%s) */
#define V4E_PUTTOOMANYLVLS 203310		/* Cannot PUT to area (%d %s) - Must reformat index */
#define V4E_RECCURLOCK 203330		/* Record locked by Pid=%d */
#define V4E_RECEXCEEDBKT 203340 	/* Record size (%d) exceeds that allowed for given bucket size (%d) in area (%d) */
#define V4E_RECNOTLOCKED 203350 	/* Area (%d %s) - cannot update without prior lock */
#define V4E_RECTOOBIG 203360		/* Area (%d %s) Record too big for user buffer (%d > %d) */
#define V4E_RECVERR 203370			/* Error (%d) in recv on socket (%s) */
#define V4E_REFHASHLIM 203380		/* Exceeded limit (%d) on DataId hash table (record %d) */
#define V4E_RMSCLOSEERR 203390		/* V4IS/RMS CloseError on Area (%s) - %s */
#define V4E_RMSOPENERR 203400		/* V4IS/RMS Open Error on Area (%s) - %s */
#define V4E_RMSPUTERR 203410		/* V4IS/RMS Area (%s */
#define V4E_RMVPNTTYPE 203420		/* Cannot Remove point of this type")  */
#define V4E_SEMGETFAIL 203450		/* Could not create semaphore (err %d) for %s */
#define V4E_SEMOPFAIL 203460		/* Call to semop failed (%d) within LOCKLT macro */
#define V4E_SENDERR 203470			/*  */
#define V4E_SEQIOERR 203480		/* Area (xxx %s) Error (%d) reading sequential record */
#define V4E_SEQOPENFAIL 203490		/* Err (%d) opening text file (%s) for sequential access */
#define V4E_SEQRECTOOBIG 203500 	/* File (%s) next record (#%d) length (%d) too big for user buffer (%d) */
#define V4E_SHMATFAIL 203510		/* ebuf) ; */
#define V4E_SHMGETERR 203520		/* ebuf) ; */
#define V4E_SHMGETFAIL 203530		/* Error (%d) creating open-close segment */
#define V4E_SNBINDEXLINK 203550 	/* Positioned at IndexLink key- should not be here")  */
#define V4E_SOCKETCONNECT 203560		/* Error (%d) connect'ing INET Socket to name (%s) */
#define V4E_SOCKETMAKE 203570		/* Error (%d) creating INET Socket (%s) */
#define V4E_SORTFAIL 203580		/* Sort (%s) appears to have failed */
#define V4E_SRVERR 203590			/* err.Message) ; */
#define V4E_STASHFAILDATA 203600		/* Cannot stash data (after multiple attempts)")  */
#define V4E_STASHFAILKEY 203610 	/* Cannot stash data (after multiple attempts)")  */
#define V4E_TOMNYAREAS 203670		/* Too many AREAS (%d) already in place */
#define V4E_TOOMANYBKTS 203680		/* Too many buckets (%d) for area (%d) in mmm */
#define V4E_UNEXEOF 203740			/* Unexpected end-of-file")  */
#define V4E_UNEXPSRVMSG 203750		/* Unexpected server message (%d) on Get to file (%s) */
#define V4E_UNKAID 203760			/* Specified AId (%d) not open */
#define V4E_UNKGETMODE 203790		/* Area (%s) - Invalid Get mode (%d) */
#define V4E_UPDATELINK 203890		/* Area (%d %s) - cannot update record in linked area */
#define V4E_USERERROR 203900		/* result) ; } ; */
#define V4E_USOCKETNOTSUP 203910		/* Unix Sockets not supported in this version of V4")  */
#define V4E_V3LOADUNS 203920		/* Load of V3 module not supported outside of V3")  */
#define V4E_V3MODUNS 203930		/* V3Mod/V3ModRaw point types not supported outside of V3")  */
#define V4E_V3UNLOADUNS 203940		/* UnLoad of V3 module not supported outside of V3")  */
#define V4E_WRITEINVBKTTYPE 203950		/* Area (%d %s) Bucket Type (%d) not valid in bucket (%d) */

#define V4E_RMSCONNECT 210000
#define V4E_RMSOPEN 210001
#define V4E_RMSCLOSE 210002
#define V4E_RMSGET 210003
#define V4E_RMSPUT 210004

#define V4E_DILMAX 220000
#define V4E_KEYNOTINBUF 220001
#define V4E_BADV4AREA 220002
#define V4E_SHMDTERR 220003

/*	Copied from v3defs - DO NOT CHANGE !!! */
#define VFT_V3INT 0		/* Internal V3 Value (see VFTV3_xxx) */
#define VFT_BININT 1		/* Binary integer */
#define VFT_FLOAT 2		/* Floating point */
#define VFT_FIXSTR 3		/* Fixed length string */
#define VFT_VARSTR 4		/* Variable length string */
#define VFT_PACDEC 5		/* Packed decimal */
#define VFT_STRINT 6		/* String integer (e.g. PIC 999) */
#define VFT_MODREF 7		/* Module call/reference */
				/* NOTE: If VFM_PTR then a pointer to db__module_objentry
					  else if VFM_OFFSET then index into module's db__module_link_list
					  else if VFM_MEP then pointer to db__module_entry
					  else if VFM_IMMEDIATE then V3 Opcode index to v3_operations() */
#define VFT_V3NUM 8		/* A V3 internal number (see val__v3num) */
#define VFT_OBJREF 9		/* An object reference (next word points to object-id/ptr) */
#define VFT_STRUCTPTR 10	/* A structure pointer */
#define VFT_INDIRECT 11		/* Indirect pointer to string */
#define VFT_POINTER 12		/* Value is a machine pointer */
#define VFT_OBJECT VAL_SOB	/* Value is an object */
#define VFT_OBJNAME 14		/* Value is pointer to struct db__dcl_objref */
#define VFT_BINWORD 15		/* A 16 bit binary integer */
#define VFT_BINLONG 16		/* A 64 bit binary integer */
#define VFT_FIXSTRSK 17		/* Fixed length string (returned by SKELs- for special error checks) */





#include "v4ssdefs.c"
#include "vsortdef.c"
#include "v4dictionary.c"
#include "v4proto.c"

#define _DidV4Defs 1
#endif	/* _DidV4Defs */

