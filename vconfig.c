/*      VCONFIG.C - Configuration Header for V3/V4

	Created 6-July-92 by Victor E. Hansen                    */

#ifndef __DidVConfig		/* Only do this once! */

#define V4IS_MajorVersion 1
#define V4IS_MinorVersion 683

/*
#define NT486 1
#define RASPPI
#define LINUX486 1
#define ALPHAVMS 1
#define MULTINET 1
#define ALPHAOSF 1
#define RS6000AIX
#define ALPHANT 1
#define HPUX 1
#define VAXVMS 1
#define ULTRIX 1
#define SU486 1
*/


//#define NASTYBUG 1

#ifdef RASPPI
//#define HAVECURL			/* Have the CURL library */
//#define WANTODBC 1
//#define WANTV4IS 1
//#define WANTMYSQL 1
//#define getcdf(a,b,c) 0
#define V3_CONFIG_OS "RASPPI"
#define V3_CONFIG_BOX "ARM"
#define UNIX 1
#define POSIX 1                 /* Supports POSIX libraries & calls */
#define SIGNALS 1               /* Enable signal (interrupt) trapping for critical V4IS routines */
#define USOCKETS 1      /* Enable support for Unix sockets */
#define ISOCKETS 1      /* Enable support for TCP/IP sockets */
typedef long V3STACK ;          /* Declare format of V3 value stack entry */
typedef long int LONGINT ;
typedef long long int B64INT ;			/* Default for 64 bit integer */
typedef unsigned long long int UB64INT ;			/* Default for 64 bit integer */
typedef long int FSEEKINT ;
typedef unsigned BIGFIELD ;     /* Declare format for big field (more than 16 bits) */
typedef unsigned char CHARFIELD ;       /* Declare format for a little (byte) field */
typedef int TLSINDEX ;		/* Thread-local-storage index/key */
#define TIMEOFFSETSEC 315532800         /* Adjustment to convert time() to V3 time */
#define TIMEOFFSETDAY 40588             /* Adjustment for day */
#define segshare 1      /* If defined then load in UNIX sharable segment stuff */
//#define V3PCKSHRSEG 1           /* Set up for shared V3 package segments */
#define v3v4 1          /* Build in interface to V4 */
#define v4is 1          /* Build in interface to V4 ISAM */
#define INCSYS 1        /* Pull include entries from "sys/" */
#define ISLITTLEENDIAN 1
#define HANGLOOSE(MSEC) v_Sleepms(MSEC)
#define V4IS_HANG_TIME 15               /* milliseconds to sleep */
//#define ALTVARGS 1                      /* Use alternate variable-arg format */
#define SCPCVAL 0               /* For outputting PC location on interrupt */
#define SCPCZAP { }
#define O_BINARY 0		/* Used in open() - apparently only needed in Windows?? */
#ifndef PATH_MAX
#define PATH_MAX 512
#endif
#define CURRENTPID getpid()
#define V_FILEPATH_DELIM UClit('/')
#define DCLSPINLOCK pthread_spinlock_t
//#define GETSPINLOCK(ptrLock) (spin_lock(ptrLock) != 0)
//#define RLSSPINLOCK(LOCKVAR) spin_unlock(&LOCKVAR)
#define GETSPINLOCK(ptrLock) (TRUE)
#define RLSSPINLOCK(LOCKVAR) LOCKVAR = UNUSED ;
#define UNUSEDSPINLOCKVAL -1
//#define EXITABORT 0		Don't override - linux defines standard
//#define EXITOK 1

#endif



#ifdef RS6000AIX
#define getcdf(a,b,c) 0
#define V3_CONFIG_OS "UNIX_AIX"
#define V3_CONFIG_BOX "RS6000"
#define UNIX 1
#define POSIX 1                 /* Supports POSIX libraries & calls */
#define SIGNALS 1               /* Enable signal (interrupt) trapping for critical V4IS routines */
#define USOCKETS 1      /* Enable support for Unix sockets */
#define ISOCKETS 1      /* Enable support for TCP/IP sockets */
typedef int V3STACK ;           /* Declare format of V3 value stack entry */
typedef long LONGINT ;
typedef __int64 B64INT ;		/* Default for 64 bit integer */
typedef unsigned __int64 UB64INT ;		/* Default for 64 bit integer */
typedef xxx FSEEKINT ;
typedef unsigned BIGFIELD ;     /* Declare format for big field (more than 16 bits) */
typedef unsigned CHARFIELD ;    /* Declare format for a little (byte) field */
#define TIMEOFFSETSEC 315532800         /* Adjustment to convert time() to V3 time */
#define TIMEOFFSETDAY 40588             /* Adjustment for day */
#define HANGLOOSE(MSEC) sleepms(MSEC)
#define segshare 1      /* If defined then load in UNIX sharable segment stuff */
/* #define V3PCKSHRSEG 1                /* Set up for shared V3 package segments */
#define GETGLB(ax) (glb = (struct V4MM__GlobalBuf *)v4mm_GetGLB(ax))
#define v3v4 1          /* Build in interface to V4 */
#define v4is 1          /* Build in interface to V4 ISAM */
#define INCSYS 1        /* Pull include entries from "sys/" */
#define V4IS_HANG_TIME 15               /* milliseconds to sleep */
#define ALTVARGS 1                      /* Use alternate variable-arg format */
#define SCPCVAL 0               /* For outputting PC location on interrupt */
#define SCPCZAP { }
#endif

#ifdef HPUX
#define ALTVARGS 1                      /* Use alternate variable-arg format */
#define V3_CONFIG_OS "UNIX_HP_UX"
#define V3_CONFIG_BOX "HP_PA_RISC"
typedef int V3STACK ;           /* Declare V3 execution stack */
typedef long LONGINT ;
typedef long long B64INT ;		/* Default for 64 bit integer */
typedef unsigned long long UB64INT ;		/* Default for 64 bit integer */
typedef long int FSEEKINT ;
typedef unsigned BIGFIELD ;     /* Declare format for big field (more than 16 bits) */
typedef unsigned CHARFIELD ;    /* Declare format for a little (byte) field */
#define TIMEOFFSETSEC 315532800         /* Adjustment to convert time() to V3 time */
#define TIMEOFFSETDAY 40588             /* Adjustment for day */
#define v3v4 1          /* Build in interface to V4 */
#define v4is 1          /* Build in interface to V4 ISAM */
#define HANGLOOSE(MSEC) sleepms(MSEC)
#define segshare 1      /* If defined then load in UNIX sharable segment stuff */
#define V3PCKSHRSEG 1           /* Set up for shared V3 package segments */
#define UNIX 1
#define POSIX 1                 /* Supports POSIX libraries & calls */
#define SIGNALS 1               /* Enable signal (interrupt) trapping for critical V4IS routines */
#define USOCKETS 1      /* Enable support for Unix sockets */
#define ISOCKETS 1      /* Enable support for TCP/IP sockets */
#define INCSYS 1        /* Pull include entries from "sys/" */
#define SCPCVAL 0               /* For outputting PC location on interrupt */
#define SCPCZAP { }
#define O_BINARY 0		/* Not defined on HP-UX ? */
#endif

#ifdef ULTRIX
#define getcdf(a,b,c) 0
#define V3_CONFIG_OS "UNIX_ULTRIX"
#define V3_CONFIG_BOX "MIPS_Rx000"
typedef int V3STACK ;           /* Declare format of V3 value stack entry */
typedef long LONGINT ;
typedef __int64 B64INT ;		/* Default for 64 bit integer */
typedef unsigned __int64 UB64INT ;		/* Default for 64 bit integer */
typedef xxx FSEEKINT ;
typedef unsigned BIGFIELD ;     /* Declare format for big field (more than 16 bits) */
typedef unsigned CHARFIELD ;    /* Declare format for a little (byte) field */
#define TIMEOFFSETSEC 315532800         /* Adjustment to convert time() to V3 time */
#define TIMEOFFSETDAY 40588             /* Adjustment for day */
#define CLOCKS_PER_SEC 60
#define HZ 60
#define INCSYS 1        /* Pull include entries from "sys/" */
#define ISLITTLEENDIAN 1
#define cisam 1
/* #define segshare 1 */        /* If defined then load in UNIX sharable segment stuff */
#define UNIX 1
#define POSIX 1                 /* Supports POSIX libraries & calls */
#define SIGNALS 1               /* Enable signal (interrupt) trapping for critical V4IS routines */
#define USOCKETS 1      /* Enable support for Unix sockets */
#define ISOCKETS 1      /* Enable support for TCP/IP sockets */
#endif

#ifdef VAXVMS
#define isspace(c) (c <= ' ')	/* Macro s/b defined by compiler? */
#define VMSOS 1                 /* VMS Operating system */
#define getcdf(a,b,c) 0
#define V3_CONFIG_OS "VMS"
#define V3_CONFIG_BOX "VAX"
typedef int V3STACK ;           /* Declare format of V3 value stack entry */
typedef long LONGINT ;
typedef long FSEEKINT ;
typedef long int B64INT ;		/* Default for 64 bit integer */
typedef unsigned long int UB64INT ;		/* Default for 64 bit integer */
typedef unsigned BIGFIELD ;     /* Declare format for big field (more than 16 bits) */
typedef unsigned CHARFIELD ;    /* Declare format for a little (byte) field */
typedef int pid_t ;
typedef int key_t ;
#define ALTVARGS 1                      /* Use alternate va_args format */
#define TIMEOFFSETSEC 315532800         /* Adjustment to convert time() to V3 time */
#define TIMEOFFSETDAY 40588             /* Adjustment for day */
#define CLOCKS_PER_SEC 60
#define CLOCKS_PER_SEC 100
#define HZ 60
#define ISLITTLEENDIAN 1
#define v3v4 1          /* Build in interface to V4 */
#define v4is 1          /* Build in interface to V4 ISAM */
/* #define segshare 1 */        /* If defined then load in UNIX sharable segment stuff */
#define PROCESS_GONE(pid) v4mm_VaxVMSProcessGone(pid)
#define ISOCKETS 1      /* Enable support for TCP/IP sockets */
/* #define UNIX 1 */
#define POSIX 1                 /* Supports POSIX libraries & calls */
/* #define SIGNALS 1            /* Enable signal (interrupt) trapping for critical V4IS routines */
#define ALIGN_BYTE 0
#define ALIGN_SHORT 0
#define ALIGN_WORD 0
#define ALIGN_DOUBLE 0
#define ALIGN_LONG 0
#define ALIGN_MAX 0                     /* Max alignment needed */
#define V3_VMS_PAGE_SIZE 512
#define V3_VMS_PAGELET_SIZE 512
#endif

#ifdef ALPHAVMS
#define VMSOS 1                 /* VMS Operating system */
#define getcdf(a,b,c) 0
#define V3_CONFIG_OS "VMS"
#define V3_CONFIG_BOX "ALPHA"
typedef long V3STACK ;          /* Declare format of V3 value stack entry */
typedef long LONGINT ;
typedef __int64 B64INT ;		/* Default for 64 bit integer */
typedef unsigned __int64 UB64INT ;		/* Default for 64 bit integer */
typedef long int FSEEKINT ;
#define SIZEofSTACK 4                   /* Number of bytes in a stack entry */
#define ALIGNofPTR ALIGN_WORD
#define SIZEofPTR 4                     /* Number of bytes in pointer */
typedef unsigned BIGFIELD ;     /* Declare format for big field (more than 16 bits) */
typedef unsigned CHARFIELD ;    /* Declare format for a little (byte) field */
/* Line below may be unnecessary on some VMS systems */
typedef int pid_t ;
#define ALTVARGS 1                      /* Use alternate va_args format */
#define TIMEOFFSETSEC 315532800         /* Adjustment to convert time() to V3 time */
#define TIMEOFFSETDAY 40588             /* Adjustment for day */
#define CLOCKS_PER_SEC 60
#define CLOCKS_PER_SEC 100
#define HZ 60
#define ISLITTLEENDIAN 1
#define v3v4 1          /* Build in interface to V4 */
#define v4is 1          /* Build in interface to V4 ISAM */
/* #define segshare 1 */        /* If defined then load in UNIX sharable segment stuff */
#define PROCESS_GONE(pid) v4mm_VaxVMSProcessGone(pid)
#define ISOCKETS 1      /* Enable support for TCP/IP sockets */
/* #define UNIX 1 */
#define POSIX 1                 /* Supports POSIX libraries & calls */
/* #define SIGNALS 1            /* Enable signal (interrupt) trapping for critical V4IS routines */
#define V3_VMS_PAGE_SIZE 8192
#define V3_VMS_PAGELET_SIZE 512
#define O_BINARY 0		/* Used in open() - apparently only needed in Windows?? */
#define WILDCARDFILECHARS UClit("%*")
#endif

#ifdef NT486
#define V4UNICODE			/* Want UNICODE build */
#define HAVECURL			/* Have the CURL library */
//#define HAVEWININET
#define WIN32_EXTRA_LEAN		/* Supposed to make projects build faster? */
#define getcdf(a,b,c) 0
#define V3_CONFIG_OS "WINDOWS"		/* NOTE: either WINNT or WINDOWS via v3operx! */
#define V3_CONFIG_BOX "x86"
#define ISOCKETS 1      /* Enable support for TCP/IP sockets */
#define SOCKETINIT if (!gpi->didWSASetup) { WSADATA wsad ; WSAStartup(MAKEWORD(1,1),&wsad) ; gpi->didWSASetup = TRUE ; } ;
#define SOCKETCLOSE(h) 	(HANGLOOSE(200),closesocket(h))
#define ISFINITE _finite
typedef int V3STACK ;           /* Declare format of V3 value stack entry */
typedef long LONGINT ;
#define ALIGN_LONG 3
typedef __int64 B64INT ;		/* Default for 64 bit integer */
typedef unsigned __int64 UB64INT ;		/* Default for 64 bit integer */
typedef long FSEEKINT ;
typedef unsigned BIGFIELD ;     /* Declare format for big field (more than 16 bits) */
typedef unsigned char CHARFIELD ;       /* Declare format for a little (byte) field */
typedef int pid_t ;
typedef int key_t ;
typedef int TLSINDEX ;		/* Thread-local-storage index/key */
#define TIMEOFFSETSEC 315532800         /* Adjustment to convert time() to V3 time */
#define TIMEOFFSETDAY 40588             /* Adjustment for day */
#define v3v4 1          /* Build in interface to V4 */
#define v4is 1          /* Build in interface to V4 ISAM */
#define segshare 1      /* If defined then load in UNIX sharable segment stuff */
//#define V3PCKSHRSEG 1           /* Set up for shared V3 package segments */
#define ISLITTLEENDIAN 1
#define HANGLOOSE(MSEC) Sleep(MSEC)
#define PROCESS_GONE(pid) v4mm_WinNTProcessGone(pid,lt)
#define V4IS_HANG_TIME 15               /* milliseconds to sleep */
#define ISOCKETS 1      /* Enable support for TCP/IP sockets */
#define WINNT 1                         /* WindowsNT */
#define NETERROR WSAGetLastError()
#define OSERROR GetLastError()
#define SETNETERROR(num) WSASetLastError(num) ;
#define NETIOCTL ioctlsocket
#define ALIGN_BYTE 0                    /* Number of bits to address-align */
#define ALIGN_SHORT 1
#define ALIGN_WORD 3
#define ALIGN_DOUBLE 3                  /* Double-Real not double aligned ? */
#define ALIGN_MAX 7                     /* Max alignment needed */
#define WINNT_PID (((GetCurrentProcessId() & 0xFFFF) * 0x10000) + GetCurrentThreadId())
#define WANTODBC 1
#define WANTV4IS 1
#define WANTMYSQL 1
#define PID int
#define SEGID HANDLE
#define SEGKEY HANDLE
#define DCLSPINLOCK LONG64
#define RLSSPINLOCK(LOCKVAR) LOCKVAR = -1
#define GETSPINLOCK(ptrLock) (InterlockedIncrement64(ptrLock) == 0)
#define CURRENTPID GetCurrentProcessId()
#define V4ENABLEMULTITHREADS 1
#define V_FILEPATH_DELIM UClit('\\')
#ifdef _WIN64
#define SIZEofPTR 8
#else
#define SIZEofPTR 4
#endif
/*	Define these to make Windows happy */
//#define isatty _isatty
//#define wcsicmp _wcsicmp
//#define read _read
//#define close _close
//#define strnicmp _strnicmp
//#define write _write
//#define fileno _fileno
//#define fdopen _fdopen
//#define iob _iob
//#define stricmp _stricmp
//#define putenv _putenv
//#define strdup _strdup
#endif

#ifdef TESTING
//#undef HAVECURL
//#undef WANTODBC
//#undef WANTMYSQL
#endif

#ifdef SU486
#define getcdf(a,b,c) 0
#define V3_CONFIG_OS "UNIX_SCO"
#define V3_CONFIG_BOX "x86"
/* #define HANGLOOSEDCL int hlv ;
#define HANGLOOSE(MSEC) for(hlv=0;hlv<1000000;hlv+=2) { hlv = hlv - 1 ; } */
#define HANGLOOSE(MSEC) nap(MSEC)
#define UNIX 1
#define POSIX 1                 /* Supports POSIX libraries & calls */
#define SIGNALS 1               /* Enable signal (interrupt) trapping for critical V4IS routines */
#define USOCKETS 1      /* Enable support for Unix sockets */
#define ISOCKETS 1      /* Enable support for TCP/IP sockets */
typedef int V3STACK ;           /* Declare format of V3 value stack entry */
typedef long LONGINT ;
typedef __int64 B64INT ;		/* Default for 64 bit integer */
typedef unsigned __int64 UB64INT ;		/* Default for 64 bit integer */
typedef xxx FSEEKINT ;
typedef unsigned BIGFIELD ;     /* Declare format for big field (more than 16 bits) */
typedef unsigned char CHARFIELD ;       /* Declare format for a little (byte) field */
#define TIMEOFFSETSEC 315532800         /* Adjustment to convert time() to V3 time */
#define TIMEOFFSETDAY 40588             /* Adjustment for day */
#define CLOCKS_PER_SEC 100              /* 10 millisecond clock ticks */
#define segshare 1      /* If defined then load in UNIX sharable segment stuff */
//#define V3PCKSHRSEG 1           /* Set up for shared V3 package segments */
#define v3v4 1          /* Build in interface to V4 */
#define v4is 1          /* Build in interface to V4 ISAM */
#define INCSYS 1        /* Pull include entries from "sys/" */
#define ISLITTLEENDIAN 1
#define ALIGN_BYTE 0                    /* Number of bits to address-align */
#define ALIGN_SHORT 1
#define ALIGN_WORD 3
#define ALIGN_DOUBLE 3                  /* Double-Real not double aligned ? */
#define ALIGN_MAX 7                     /* Max alignment needed */
#define ALTVARGS 1                      /* Use alternate variable-arg format */
#endif

#ifdef LINUX486
#define HAVECURL			/* Have the CURL library */
#define getcdf(a,b,c) 0
#define V3_CONFIG_OS "LINUX"
#define V3_CONFIG_BOX "x86"
#define UNIX 1
#define POSIX 1                 /* Supports POSIX libraries & calls */
#define SIGNALS 1               /* Enable signal (interrupt) trapping for critical V4IS routines */
#define USOCKETS 1      /* Enable support for Unix sockets */
#define ISOCKETS 1      /* Enable support for TCP/IP sockets */
typedef long V3STACK ;          /* Declare format of V3 value stack entry */
typedef long int LONGINT ;
typedef long long int B64INT ;			/* Default for 64 bit integer */
typedef unsigned long long int UB64INT ;			/* Default for 64 bit integer */
typedef long int FSEEKINT ;
typedef unsigned BIGFIELD ;     /* Declare format for big field (more than 16 bits) */
typedef unsigned char CHARFIELD ;       /* Declare format for a little (byte) field */
typedef int TLSINDEX ;		/* Thread-local-storage index/key */
#define TIMEOFFSETSEC 315532800         /* Adjustment to convert time() to V3 time */
#define TIMEOFFSETDAY 40588             /* Adjustment for day */
#define segshare 1      /* If defined then load in UNIX sharable segment stuff */
//#define V3PCKSHRSEG 1           /* Set up for shared V3 package segments */
#define v3v4 1          /* Build in interface to V4 */
#define v4is 1          /* Build in interface to V4 ISAM */
#define INCSYS 1        /* Pull include entries from "sys/" */
#define ISLITTLEENDIAN 1
#define HANGLOOSE(MSEC) v_Sleepms(MSEC)
#define V4IS_HANG_TIME 15               /* milliseconds to sleep */
//#define ALTVARGS 1                      /* Use alternate variable-arg format */
#define SCPCVAL 0               /* For outputting PC location on interrupt */
#define SCPCZAP { }
#define O_BINARY 0		/* Used in open() - apparently only needed in Windows?? */
#ifndef PATH_MAX
#define PATH_MAX 512
#endif
#define CURRENTPID getpid()
#define V_FILEPATH_DELIM UClit('/')
#define GETSPINLOCK(ptrLock)\
 (({int __temp ;\
asm volatile(\
	"subl   %%ebx,%%ebx;"\
        "xchgl  %%ebx,(%1);"\
        "movl   %%ebx,%0;"\
          :"=r"(__temp)\
          :"r"(ptrLock)\
          :"%ebx");\
   __temp;\
 }) == -1)
//#define EXITABORT 0		Don't override - linux defines standard
//#define EXITOK 1
#define SIZEofPTR 8
#endif

#ifdef ALPHAOSF
#define getcdf(a,b,c) 0
#define V3_CONFIG_OS "UNIX_OSF"
#define V3_CONFIG_BOX "ALPHA"
#define UNIX 1
#define POSIX 1                 /* Supports POSIX libraries & calls */
#define SIGNALS 1               /* Enable signal (interrupt) trapping for critical V4IS routines */
#define USOCKETS 1      /* Enable support for Unix sockets */
#define ISOCKETS 1      /* Enable support for TCP/IP sockets */
typedef long V3STACK ;          /* Declare format of V3 value stack entry */
typedef long LONGINT ;
typedef long B64INT ;			/* Default for 64 bit integer */
typedef unsigned long UB64INT ;			/* Default for 64 bit integer */
typedef long int FSEEKINT ;
#define SIZEofSTACK 8                   /* Number of bytes in a stack entry */
#define ALIGNofPTR ALIGN_DOUBLE
#define SIZEofPTR 8                     /* Number of bytes in pointer */
#define COPYPTR(dst,src) memcpy(&dst,&src,SIZEofPTR) ;
#define COPYPTR1(dst,src) { char *z1z ; z1z=(char *)src ; memcpy(&dst,&z1z,SIZEofPTR) ; } ;
typedef unsigned BIGFIELD ;     /* Declare format for big field (more than 16 bits) */
typedef unsigned char CHARFIELD ;       /* Declare format for a little (byte) field */
#define TIMEOFFSETSEC 315532800         /* Adjustment to convert time() to V3 time */
#define TIMEOFFSETDAY 40588             /* Adjustment for day */
#define segshare 1      /* If defined then load in UNIX sharable segment stuff */
#define V3PCKSHRSEG 1           /* Set up for shared V3 package segments */
#define v3v4 1          /* Build in interface to V4 */
#define v4is 1          /* Build in interface to V4 ISAM */
#define INCSYS 1        /* Pull include entries from "sys/" */
#define ISLITTLEENDIAN 1
#define HANGLOOSE(MSEC) napms(MSEC)
#define V4IS_HANG_TIME 15               /* milliseconds to sleep */
#define ALTVARGS 1                      /* Use alternate variable-arg format */
#define LONG64 1                        /* If set then type "long" is 64 bits */
#define O_BINARY 0		/* Used in open() - apparently only needed in Windows?? */
#endif

#ifdef ALPHANT
#define getcdf(a,b,c) 0
#define V3_CONFIG_OS "WINDOWS"		/* NOTE: either WINNT or WINDOWS via v3operx! */
#define V3_CONFIG_BOX "ALPHA"
#define CLOCKS_PER_SEC 100
typedef int V3STACK ;           /* Declare format of V3 value stack entry */
typedef __int64 LONGINT ;
typedef __int64 B64INT ;		/* Default for 64 bit integer */
typedef unsigned __int64 UB64INT ;		/* Default for 64 bit integer */
typedef long FSEEKINT ;
#define SIZEofSTACK 4                   /* Number of bytes in a stack entry */
#define ALIGNofPTR ALIGN_WORD
#define SIZEofPTR 4                     /* Number of bytes in pointer */
typedef unsigned BIGFIELD ;     /* Declare format for big field (more than 16 bits) */
typedef unsigned char CHARFIELD ;       /* Declare format for a little (byte) field */
typedef int pid_t ;
typedef int key_t ;
#define TIMEOFFSETSEC 315532800         /* Adjustment to convert time() to V3 time */
#define TIMEOFFSETDAY 40588             /* Adjustment for day */
#define v3v4 1          /* Build in interface to V4 */
#define v4is 1          /* Build in interface to V4 ISAM */
#define segshare 1      /* If defined then load in UNIX sharable segment stuff */
#define V3PCKSHRSEG 1           /* Set up for shared V3 package segments */
#define ISLITTLEENDIAN 1
#define HANGLOOSE(MSEC) Sleep(MSEC)
#define PROCESS_GONE(pid) v4mm_WinNTProcessGone(pid,lt)
#define V4IS_HANG_TIME 15               /* milliseconds to sleep */
#define ALTVARGS 1                      /* Use alternate variable-arg format */
#define LONG64 1                        /* If set then type "long" is 64 bits */
#define ISOCKETS 1      /* Enable support for TCP/IP sockets */
#define SOCKETCLOSE(h) closesocket(h)
#define WINNT 1                         /* WindowsNT */
#define NETERROR WSAGetLastError()
#define SETNETERROR(num) WSASetLastError(num) ;
#define NETIOCTL ioctlsocket
#define WINNT_PID (((GetCurrentProcessId() & 0xFFFF) * 0x10000) + GetCurrentThreadId())
#define CURRENTPID GetCurrentProcessId()
#define PID HANDLE
#define SEGID HANDLE
#define SEGKEY HANDLE
#endif

char *v_OSErrString() ;

#ifdef INHIBIT_V4UNICODE			/* If really don't want UNICODE then make sure it is turned off */
#undef V4UNICODE
#endif

#ifdef INHIBIT_WANTMYSQL
#undef WANTMYSQL
#endif

#ifdef INHIBIT_WANTORACLE
#undef WANTORACLE
#endif

//#define V4_BUILD_RUNTIME_STATS		/* If defined then compile in code to track detail runtime stats */
#define V4_BUILD_V4IS_MAINT			/* If defined then compile in V4 Area maintenance routines */
//#define V4_BUILD_CS_FUNCTIONALITY		/* If defined then compile in V4CS (client-server) functionality */
#define V4_BUILD_SECURITY			/* If defined then compile in advanced V4 security features */

//#define V4_BUILD_LMTFUNC_MAX_LIST 1000	/* If defined then limits size of lists (and enumerations of said lists) to this number of points */
//#define V4_BUILD_LMTFUNC_MAX_AREA 10		/* If defined then limits the max number of open areas */
//#define V4_BUILD_LMTFUNC_EXP_DATE 54480	/* If defined then V4 will not run properly after this UDate */
//#define V4_BUILD_LMTFUNC_MAX_CTX_PTS 50	/* If defined then max number of CTX points */


//#define V4_BUILD_TELNET_XFACE 1		/* If defined then build for telnet interface */

#ifdef MULTINET
#define NETERROR socket_errno
#define SETNETERROR(num) socket_errno = num ;
#define NETIOCTL socket_ioctl
#endif

#ifndef SizeofINT
#define SIZEofINT 4                     /* Number of bytes in int */
#endif

#ifndef SIZEofPTR
#define SIZEofPTR 4                     /* Number of bytes in pointer */
#endif

#ifndef SIZEofSTACK
#define SIZEofSTACK SIZEofPTR           /* Number of bytes in a stack entry */
#endif

#ifndef ALIGNofPTR
#define ALIGNofPTR ALIGN_WORD
#endif

#ifndef COPYPTR
#define COPYPTR(dst,src) dst = (void *)(src) ;
#define COPYPTR1(dst,src) dst = (void *)(src) ;
#endif

#ifndef ALIGN_WORD
#define ALIGN_BYTE 0                    /* Number of bits to address-align */
#define ALIGN_SHORT 1
#define ALIGN_WORD 3
#define ALIGN_LONG 7
#define ALIGN_DOUBLE 7
#define ALIGN_MAX 7                     /* Max alignment needed */
#endif

#ifndef DtoI
#define DtoI(dbl) ((int)(dbl < 0 ? dbl - 0.5 : dbl + 0.5))
#endif

#ifndef DtoLI
#define DtoLI(dbl) ((B64INT)(dbl < 0 ? dbl - 0.5 : dbl + 0.5))
#endif

#ifndef ALLOCATE_EXTRA
#ifdef cisam
#define ALLOCATE_EXTRA 1                /* Extra bytes to allocate on heap/stack */
#define ALLOCATE_MIN 9                  /* Structure minimum to trigger allocation */
#else
#define ALLOCATE_EXTRA 0                /* If no CISAM then don't bother with extra allocation */
#define ALLOCATE_MIN 0
#endif
#endif

#ifndef HANGLOOSEDCL
#define HANGLOOSEDCL
#endif
#ifndef HANGLOOSE
#define HANGLOOSE(MSECS) sleep(1)       /* UNIX - can only sleep min 1 sec */
#endif

#ifndef V4IS_HANG_TIME
#define V4IS_HANG_TIME 50               /* milliseconds to sleep/hang on V4IS lock contention */
#endif

#ifndef V4IS_HANG_LONGTIME
#define V4IS_HANG_LONGTIME 5000         /* milliseconds to sleep/hang "for a long time" */
#endif

#ifndef V4IS_LOCKTRY_BASE
#define V4IS_LOCKTRY_BASE 5000          /* Minimum number of retries for a lock */
#endif

#ifndef V4IS_LOCKTRY_PANIC
#define V4IS_LOCKTRY_PANIC 150          /* Attempt PanicLockout every PANIC lock tries */
#endif

#ifndef PROCESS_GONE
#ifdef UNIX
#define PROCESS_GONE(pid) \
  ( (kill(pid,0) == -1) ? (errno == ESRCH ? TRUE : FALSE) : FALSE )
#endif
#endif

#ifndef COPYJMP
#define COPYJMP(LJMP,RJMP) memcpy(&LJMP,&RJMP,sizeof RJMP) ;
#endif

#ifndef OSERROR
#define OSERROR errno
#endif

#ifndef NETERROR
#define NETERROR errno
#define SETNETERROR(num) errno = num ;
#define NETIOCTL ioctl
#endif

#ifndef SOCKETINIT		/* This is only defined for Windows, otherwise make NOP */
#define SOCKETINIT
#endif

#ifndef SOCKETCLOSE
#define SOCKETCLOSE(h) close(h)
#endif

#ifndef WILDCARDFILECHARS
#define WILDCARDFILECHARS UClit("?*")
#endif

#ifndef SIZEofDOUBLE
#define SIZEofDOUBLE 8
#endif

#ifndef GETGLB
#define GETGLB(ix) (glb = mmm->Areas[ix].GBPtr)
#endif

#ifndef PID
#define PID pid_t
#endif
#ifndef SEGID
#define SEGID int
#endif
#ifndef SEGKEY
#define SEGKEY key_t
#endif

#ifdef POSIX
#define POSIXMT 1               /* Combo - POSIX & MACTHINK */
#endif

#ifndef GETSPINLOCK		/* Mulit-thread/cpu safe increment */
#define GETSPINLOCK(ptrLock) (v4mm_GrabLockTable(ptrLock))
#endif

#ifndef DCLSPINLOCK
typedef int DCLSPINLOCK ;
#endif

#ifndef RLSSPINLOCK
#define RLSSPINLOCK(LOCKVAR) LOCKVAR = -1
#endif

#ifndef SPINLOCKFREE
#define SPINLOCKFREE(LOCKVAR) (LOCKVAR == -1)
#endif

#ifndef UNUSEDSPINLOCKVAL
#define UNUSEDSPINLOCKVAL -1
#endif

//#ifndef V_SysFileName_Max
//#define V_SysFileName_Max 255+1	/* Take POSIX/UNIX/Windows Max */
//#endif

#ifndef ISFINITE
#define ISFINITE finite
#endif

#ifndef EXITABORT			/* If not defined then take system defaults */
#define EXITABORT EXIT_FAILURE
#define EXITOK EXIT_SUCCESS
#define EXITRESTART 101			/* Exit & immediately restart */
#define EXITDELAY60RESTART 102		/* Exit, wait 60 seconds and restart */
#define EXITNORESTART 103		/* Exit, do not restart */
#define EXITNEWVERSION 110		/* Exit & restart with new version (assuming command file support!) */
#endif

#ifndef V4XLIB_sesKey_Length		/* Defines V4/Web session key length & component elements */
#define V4XLIB_sesKey_Length 32
#define V4XLIB_sesKey_Elements UClit("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
#define V4XLIB_XLSX_URLHdr_Lines 10	/* Number of header lines in 'secure' URL file for XLSX linkage */
#endif

#ifdef NEED_SOCKET_LIBS
#ifdef VMSOS
#ifdef MULTINET
#include "multinet_root:[multinet.include.sys]types.h"
#include "multinet_root:[multinet.include.sys]socket.h"
#include "multinet_root:[multinet.include]netdb.h"
#include "multinet_root:[multinet.include.netinet]in.h"
#include "multinet_root:[multinet.include]errno.h"
#include "multinet_root:[multinet.include.sys]ioctl.h"
#define __IN_LOADED 1           /* veh 7/1/95 - who knows why? */
#else /*multinet*/
#include <socket.h>
#include <in.h>
#include <netdb.h>
#include <inet.h>
#include <ioctl.h>
#define ioctlsocket ioctl
//#include <ucx$inetdef.h>
//#include "[sys0.syscommon.rpc$include]ucx$typedef.h"
/* Above line may need to be replaced with below on newer versions of
#include <ucx$rpcxdr.h>
*/
#define bzero(a,b) memset(a,0,b)        /* Can't seem to find bzero in runtime libs ??? */
#endif /*multinet*/
#endif /*vmsos*/

#ifdef UNIX
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
/* #include <sys/mode.h> */
#endif /*unix*/

#ifdef WINNT
#ifdef ISOCKETS
#include <winsock2.h>
#include <ws2tcpip.h>
#ifndef EWOULDBLOCK
  #define EWOULDBLOCK WSAEWOULDBLOCK		/* These may or may not be defined in Windows libraries */
  #define EINPROGRESS WSAEINPROGRESS
#endif
#endif /*isockets*/
#endif /*winnt*/
#endif /*need_socket_libs*/

#ifndef V_FileName_Max
#ifdef WINNT
#define V_FileName_Max (MAX_PATH + 1)
#else
#define V_FileName_Max (PATH_MAX + 1)
#endif
#endif

#if defined HAVECURL && defined HAVEWININET
#error Cannot define both HAVECURL and HAVEWININET - one at-a-time PLEASE
#endif
#ifdef HAVECURL
#define CURL_STATICLIB
#endif

#ifdef INHIBIT_CURL
#undef HAVECURL
#endif

#define USE_NEW_XDB 1				/* If set then use UDBArgs/UDBSpec instead of List/Alpha dimensions in xdb routines */

#define __DidVConfig 1
#endif					/* End of entire file: __DidVConfig */