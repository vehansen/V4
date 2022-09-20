//#include "vconfig.c"
#include "v4imdefs.c"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

/*	
Updated		Version	Who	Description
---------	-------	------	--------------------------------------------------------
20-Oct-02	2.0	VEH	Added HTML optimizations & compression (via -n switch)
10-Apr-03	2.1	VEH	Added -x num switch to control compression threshold
27-Jun-03	2.2	VEH	Added Width to columns - either % or pixels
30-Jun-03	2.3	VEH	Added Frame support
23-Feb-04	2.4	VEH	Added script include file support & new "option" DHTML in header
15-Jul-04	2.5	VEH	Increased RTH_MINCOMPRESS_BYTES to 5m to effectively disable default compression
25-Apr-05	2.6	VEH	Added -e switch for example
*/

#define RTH_MajorVersion 3
#define RTH_MinorVersion 1

#define UNUSED -1

void ResToHTML(UCCHAR *,UCCHAR *,UCCHAR *,int,UCCHAR *,UCCHAR *,UCCHAR *,int,UCCHAR *,UCCHAR *,int,int,int,UCCHAR *,UCCHAR *,UCCHAR *,int,UCCHAR *) ;
void rth_OutputJavaScriptUtils(int) ;
char *rth_FormatStr(struct V4SS__DimToFormatMap *,char *) ;
void rth_Output(char *,char *,FILE *) ;
void rth_CheckEmpty(int *,FILE *,char *) ;
void rth_FormatInt(char *, char *, char *) ;
void rth_FormatDate(char *, char *) ;
void rth_FormatDbl(char *, char *, char *) ;
char *rth_CSS(char *,struct V4SS__DimToFormatMap *,int,struct V4SS__FormatSpec *,int,int,int,int,int,int *,int *) ;
void rth_GetColors(char *,char *,struct V4SS__FormatSpec *,int,int,int) ;
void rth_DisplayProblem(char *) ;

struct V4DPI__LittlePoint protoInt, protoDbl, protoFix, protoDict, protoAlpha, protoLog, protoForeign, protoNone, protoUCChar ;



int compress,mincompress ;
#define RTH_MINCOMPRESS_BYTES 5000000

#ifdef WINNT
UCCHAR **startup_envp ;
#else
char **startup_envp ;
#endif

#ifdef WINNT
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
{ int ax,sval,linktomks ;
  UCCHAR *b, *arg,*ifile,*ofile,*bkg,*headerimg,*headerurl,*tranid,*cgiserver,*menu,*areas,*option,*bodyonload, *excelLogo ;
  int i, jobno1, jobno2, leKey, hdrimgsize, isExample ;
  UCCHAR infile[V_FileName_Max], outfile[V_FileName_Max], areabuf[V_FileName_Max] ;
 struct V4C__Context ctx ;
 #define UCARGVMAX 50
  static UCCHAR *ucargv[UCARGVMAX] ;		/* Temp (hopefully) structure converting command line to Unicode */
	
	startup_envp = envp ;
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

	ifile = NULL ; ofile = NULL ; bkg = NULL ; linktomks = FALSE ; headerimg = NULL ; headerurl = NULL ; compress = TRUE ;
	tranid = UClit("") ; cgiserver = NULL ; jobno1 = UNUSED ; jobno2 = UNUSED ; leKey = UNUSED ; menu = NULL ; areas = NULL ;
	mincompress = RTH_MINCOMPRESS_BYTES ; hdrimgsize = UNUSED ; option = NULL ; bodyonload = NULL ; isExample = FALSE ; excelLogo = NULL ;

	for(ax=1;ax<argc;ax++)
	 { arg = argv[ax] ;
	   if (*arg != UClit('-'))			/* Don't have a switch - then have file */
	    { if (ifile == NULL) { ifile = arg ; continue ; }
	       else if (ofile == NULL) { ofile = arg ; continue ; }
	       else { continue ; } ;
	    } ;
	   sval = *(++arg) ;			/* Get the switch value */
	   switch (sval)
	    { default:	printf("? Invalid switch value\n") ; exit(EXITABORT) ;
	         case UClit('a'):
		   { int i,j,k ; UCCHAR *a ;
		     a = argv[++ax] ;
		     for(i=0,j=0;a[i]!=UClit('\0');i++)			/* Have to URL encode this string */
		      { if ((a[i] >= UClit('A') && a[i] <= UClit('Z')) || (a[i] >= UClit('a') && a[i] <= UClit('z')) || (a[i] >= UClit('0') && a[i] <= UClit('9')))
		         { areabuf[j++] = a[i] ; continue ; } ;
			areabuf[j++] = UClit('%') ; k = a[i] ;
			areabuf[j++] = UClit("0123456789abcdef")[(k >> 4)] ; areabuf[j++] = UClit("0123456789abcdef")[(k & 0xF)] ;
		      } ; areabuf[j++] = '\0' ;
		   }
		   areas = areabuf ; break ;
		 case UClit('b'):	bkg = argv[++ax] ; break ;		/* Background image  */
		 case UClit('l'):	linktomks = TRUE ; break ;		/* Enable hot link to MKS site */
		 case UClit('h'):	if (argv[++ax] == NULL)	goto help_entry ; /* Probably wants help! */
					i = UCstrtol(argv[ax],&b,10) ;		/* See if numeric argument */
					if (*b != UClit('\0'))				/*  no - then name of header image */
					 { headerimg = argv[ax] ; }
					 else { hdrimgsize = i ; } ;		/*  yes - then save it */
					break ;
		 case UClit('j'):						/* Javascript options */
			switch (arg[1])
			 { default:		printf("? Invalid -j argument (%s)\n",UCretASC(arg)) ; exit(EXITABORT) ;
			   case UClit('b'):	bodyonload = argv[++ax] ; break ;
			 } ;
			break ;
		 case UClit('t'):	tranid = argv[++ax] ; break ;		/* Transaction Id */
		 case UClit('u'):	headerurl = argv[++ax] ; break ;	/* Header URL link */
		 case UClit('c'):	cgiserver = argv[++ax] ;  break ;	/* CGI Server address */
		 case UClit('e'):	isExample = TRUE ; break ;		/* Generate an example - strip out lots of stuff */
		 case UClit('E'):	excelLogo = argv[++ax] ; break ;	/* URL of Excel logo */
		 case UClit('1'):	jobno1 = UCatoi(argv[++ax]) ; break ;	/* First part of job number */
		 case UClit('2'):	jobno2 = UCatoi(argv[++ax]) ; break ;	/* Second part */
		 case UClit('k'):	leKey = UCatoi(argv[++ax]) ; break ;	/* leKey */
		 case UClit('m'):	menu = argv[++ax] ; break ;		/* link back to Menu */
		 case UClit('n'):	compress = FALSE ; break ;		/* Normal HTML (default is compressed) */
		 case UClit('o'):	option = argv[++ax] ; break ;		/* Option DHTML (java)script code */
		 case UClit('x'):	mincompress = UCatoi(argv[++ax]) ; break ;/* Compress if over this number of bytes */
		 case UClit('?'):
help_entry:			printf("\nxvrestohml (v%d.%d) - MKS 2006\n\
Command format:\n\
	xvrestohtml switches input-file output-file\n\
	xvrestohtml switches input-file   (without extension, .v4r assumed)\n\n\
  -a area-string	Use the specified area-strings for link back to a main menu\n\
  -b image		The URL of an image that is to be used as the page background\n\
  -c url		The URL address of a CGI process\n\
  -e			Generate example from v4r file - disable javascript, strip leKey, etc\n\
  -E url		The URL addres of convert-to-Excel icon\n\
  -h image		The URL of an image to appear in the upper left corner of the page\n\
  -h file		The name of a txt or htm file to be inserted as the header\n\
  -h num		% of top line to allocate for header image, if not given then header appears on separate line\n\
  -k number		The sesion id to be passed onto any other V4 processes\n\
  -l			Enable hot-link back to MKS home page\n\
  -m menu		The URL of the page's main menu\n\
  -n			Do not compress this page\n\
  -o script		Script (java) to invoke top-of-page options\n\
  -t id			Transaction ID to appear at bottom of page (normally jobnum1-jobnum2)\n\
  -u url		URL to follow if header image (-h) is clicked\n\
  -x bytes		Compress HTML if exceeds bytes\n\
  -1 number		First part of processing job number\n\
  -2 number		Second part of processing job number\n",RTH_MajorVersion,RTH_MinorVersion) ;
				exit(EXITOK) ;
	    } ;
	 } ;
	if (ifile != NULL && ofile == NULL && UCstrchr(ifile,'.') == NULL)	/* Got single file, no extension */
	 { UCstrcpy(infile,ifile) ; UCstrcat(infile,UClit(".v4r")) ;
	   UCstrcpy(outfile,ifile) ; UCstrcat(outfile,UClit(".htm")) ;
	   ifile = infile ; ofile = outfile ;
	 } ;
	if (ifile == NULL) { printf("? Input file name missing\n") ; exit(EXITABORT) ; } ;
	if (ofile == NULL ) { printf("? Output file name missing\n") ; exit(EXITABORT) ; } ;

	ctx.pi = v_GetProcessInfo() ;

//compress = FALSE ;
	ResToHTML(ifile,ofile,bkg,linktomks,headerimg,menu,headerurl,hdrimgsize,tranid,cgiserver,jobno1,jobno2,leKey,areas,option,bodyonload,isExample,excelLogo) ;

	exit(EXITOK) ;
}


#define RI_ColMax 256
#define RI_HTMLMax 100
#define RI_INCMAX 10
#define RI_SRCMAX 10
#define RI_CSSMAX 50
#define RI_DIVMAX 50
#define RI_JustifyLeft 1
#define RI_JustifyRight 2
#define RI_JustifyCenter 3

#define RI_DefaultBGColor "#c0c0c0"
#define RI_DefaultHeaderFont "Arial"

#define RI_DefaultFontSize 0		/* Default is whatever browser makes it */

char JustifyStr[4][12] = { "", "left", "right", "center" } ;
char *tdstr, *tdidstr, *tdcolstr, *tdidcolstr, *tdmtstr, *tdcolmtstr ;
char *tdnowrapstr, *tdidnowrapstr ;
char *defaulttd="" ;

struct RTH__ReportInfo {
  char Title[10][1024] ;		/* First title line */
  char Id[64] ;				/* Report ID */
  char ErrMsg[2048] ;			/* If an error then this holds the message - return JavaScript */
  char ErrURL[2048] ;			/* A URL to link to after error message pop-up window (via above ErrMsg) */
  char DescInfo[1024] ;			/* Descriptive info about the output */
  char URLBase[V4SS_URLBaseMax][512] ;		/* URL base (if defined then URL's starting with '+' are suffix) */
  int CurURLx ;				/* Current URL index (into URLBase above) */
  char CurURL[2048] ;			/* URL associated with "next" column */
  char CurTitle[2048] ;			/* Title associated with "next" column */
  int HTMLCount ;			/* Number of HTML's below */
  int xHTML ;				/* Current HTML Index */
  int GridLines ;			/* If > 0 then show table gridlines */
  char GlobalFont[128] ;		/* Name of global font */
  int GlobalFontSize ;			/* Global Font Size */
  struct {
    char Buf[2048] ;
   } HTML[RI_HTMLMax] ;
  int IncCount ;			/* Number of include files below */
  struct {
    char Name[V_FileName_Max] ;		/* Name of include file */
   } IncFile[RI_INCMAX] ;
  int srcCount ;			/* Number of include files below */
  struct {
    char Name[V_FileName_Max] ;		/* Name of include file */
   } srcFile[RI_SRCMAX] ;
  struct V4SS__FormatSpec *vfsTable ;	/* Special format for entire table */
  struct V4SS__FormatSpec *vfsRow ;
  struct V4SS__FormatSpec *vfsCell ;
  struct V4SS__FormatSpec *vfsHdrRow ;
  struct V4SS__FormatSpec *vfsHdrCell ;
  struct V4SS__FormatSpec *vfsTopOfPage ;
  int CSSCount ;			/* Number of CSS style entries below */
  struct {
    char Entry[2048] ;
   } CSS[RI_CSSMAX] ;
  int DivCount ;			/* Number of tables/divisions/sheets */
  int DivSheets ;			/* TRUE if making worksheets (like Excel) */
  int CurDiv ;				/* Index into current divsion below */
  struct {
    int Initialized ;			/* TRUE after outputting html to initialize/start this division */
    char Name[64] ;			/* Name of the sheet */
    char Attributes[256] ;		/* CSS Attributes */
    char Title[10][1024] ;		/* Title line(s) for this division/sheet */
    int ColCapRowCount ;		/* Number of rows in column captions */
   } Div[RI_DIVMAX] ;
  int RowCount ;			/* Number of rows */
  int CurRow ;				/* Current row */
  int CurCol ;				/* Current Column */
  int HorRuleEveryRow ;			/* If > 0 then display horizontal rule every n rows */
  char ColCapBGColor[12] ;		/* Column caption background color */
  int ColCount ;			/* Number of Columns */
  int AutoHead ;			/* If TRUE then auto-format column headings */
  struct {
    int ColWidth ;			/* Column width (0=unused, 1-100 is %, 101-255 is pixels+100) */
    int CharWidth ;			/* Max number of characters in column */
    int WFactor ;			/* Factor for converting characters width to pixels */
    int FontSize ;
    char Heading[5][64] ;		/* Column headings */
    int Justify ;			/* Justification (see RI_Justifyxxx) */
    int HSpan ;				/* If non-zero then number of columns spanned by heading */
    char ColFormat[32] ;		/* Default column format */
    char ColStyle[64] ;			/* Default column style */
   } Col[RI_ColMax] ;			/* Column 0 is first column */
 } ;
struct RTH__ReportInfo *ri ;

#define LCL_STYLE_CSSMax 256

struct RTH__CSSList {
  int Count ;			/* Number of styles defined */
  int InitialCount ;		/* Count after outputting initial <style> section */
  struct {
    int Usage ;			/* Number of times it has been referenced (take highest as default!) */
    char CSSName[12] ;		/* Style name */
    char CSSValue[256] ;	/*  and its value */
  } CSS[LCL_STYLE_CSSMax] ;
} *rcl = NULL ;

#define OUTBUFMAX 2048
char outbuf[OUTBUFMAX+1] ;
FILE *ofp = NULL ;

#define TRACE TRUE

void ResToHTML(ifile,ofile,bkg,linktomks,headerimg,menu,headerurl,hdrimgsize,tranid,cgiserver,jobno1,jobno2,leKey,areas,option,bodyonload,isExample,excelLogo)
  UCCHAR *ifile,*ofile,*headerimg,*bkg,*menu,*headerurl,*tranid,*cgiserver,*areas,*option,*bodyonload,*excelLogo ;
  int jobno1,jobno2,leKey,isExample ;
  int linktomks,hdrimgsize ;
{ time_t curtime ;
  FILE *fp,*ifp ;
  struct stat statbuf ;
  int ifd ;
  unsigned char type ;
  int newrow,ok,len,sig,justify,cselect,nobr,returnv4r,bytesread,i,j,ud,iscap,emptycnt,dhtml ;
  unsigned char buf[25000], prefix[8192], suffix[8192], sbuf[8192], t1[5000], t2[5000], pbuf[25000] ; 
  UCCHAR usbuf[512] ;
  UCCHAR errbuf[1024],newofile[V_FileName_Max],dupfilename[V_FileName_Max],bkgbuf[256], headerimgbuf[256], headerurlbuf[256], tranidbuf[128], cgiserverbuf[256] ;
  char bgcolor[128], fldColor[64] ;
  UCCHAR dynmenubuf[512] ;
  unsigned char defaultsbuf[512] ;
  int rowcolornum, cellcolornum, colcolornum[RI_ColMax] ; double dnum ;
  char *p, *p1, *b1, *b2, *ha, *css ; char tdcls[128] ;
#define HTTP_HeaderMax 50
  char *HTTP_HTML[HTTP_HeaderMax] ; int HTTP_HTML_Count ;
  int ssdim,bytes ;
  struct V4SS__DimToFormatMap *vdfm=NULL ;
  struct V4SS__FormatSpec *vfs ;
  struct V4SS__FormatSpec vfsnp ;		/* Format for next point */
  int gotfnp ;					/* Set to TRUE if vfsnp has valid info */
  int framecnt ;
  int maxu,maxi ;


	tdstr = "<td%s>%s%s%s" ;
	tdidstr = "<td%s %s>%s%s%s" ;
	tdnowrapstr = "<td%s nowrap>%s%s%s" ;
	tdidnowrapstr = "<td%s %s nowrap>%s%s%s" ;
	tdcolstr = "<td%s colspan='%d'>%s%s%s" ;
	tdidcolstr = "<td%s %s colspan='%d'>%s%s%s" ;
	tdmtstr = "<td>" ;
	tdcolmtstr = "<td colspan=%d>" ;
	if(isExample)
	 { leKey = 0 ;		/* Clear leKey if generating an example */
	   cgiserver = UClit("#") ;	/* Clear out CGI server */
	   areas = UClit("0") ;		/* Clear out areas */
	   jobno1 = 0 ; jobno2 = 0 ;
	 } ;

	fp = UCfopen(ifile,"rb") ;
	if (fp == NULL) { v_Msg(NULL,errbuf,"VSrtInpErr",ifile,errno) ; vout_UCText(VOUT_Err,0,errbuf) ; exit(EXITABORT) ; } ;
	if (TRACE) { v_Msg(NULL,errbuf,"VResHTMLTrace1",ifile) ; vout_UCText(VOUT_Trace,0,errbuf) ; } ;
/*	Init report info structure (ri) */
	ri = (struct RTH__ReportInfo *)malloc(sizeof *ri) ; memset(ri,0,sizeof *ri) ;

/*	If file starts with "<html>" or "<?xml>" then don't convert it! */
	fread(buf,1,5,fp) ; buf[5] = '\0' ;
	if (strcmp(buf,"<html") != 0 && strcmp(buf,"<HTML") != 0 && strcmp(buf,"<?xml") && strcmp(buf,"<?XML")) { rewind(fp) ; }
	 else { fclose(fp) ; UCfopen(ifile,"r") ;
		ofp = UCfopen(ofile,"w") ;
		if (ofp == NULL) { v_Msg(NULL,errbuf,"VSrtOutFile",ofile,errno) ; vout_UCText(VOUT_Err,0,errbuf) ; exit(EXITABORT) ; } ;
		if (TRACE) { v_Msg(NULL,errbuf,"VResHTMLTrace2",ofile) ; vout_UCText(VOUT_Trace,0,errbuf) ; } ;
		for(;;)
		 { if (fgets(t1,sizeof t1,fp) == NULL) break ;
//		   if (strlen(t1) >= 4550 || strlen(t1) < 2)
//		    printf("huh %d\n",strlen(t1)) ;
		   if ((b1=strstr(t1,"<V4ErrURL>")) != NULL)
		    { char saveb ;
		      b2 = strstr(t1,"</V4ErrURL>") ; if (b2 != NULL) { saveb = *b2 ; *b2 = '\0' ; } ;
		      strncpy(ri->ErrURL,b1+10,sizeof ri->ErrURL) ; if (b2 != NULL) *b2 = saveb ;
		    } ;
		   if ((b1=strstr(t1,"<V4Err>")) == NULL)		/* Got an embedded error? */
		    { fputs(t1,ofp) ; continue ; } ;
		   fclose(ofp) ;					/* Yes - recreate output file for just error */
		   b2 = strstr(t1,"</V4Err>") ; if (b2 != NULL) *b2 = '\0' ;
		   strncat(ri->ErrMsg,b1+7,sizeof ri->ErrMsg) ;
		   if (strlen(ri->ErrMsg) == 0) strcpy(ri->ErrMsg,"An error occurred during the processing of you request.") ;
		   goto gen_error ;
		 } ;
		fclose(fp) ;fclose(ofp) ; exit(EXITOK) ;
	      } ;
	strcpy(ri->ColCapBGColor,RI_DefaultBGColor) ; ZS(bgcolor) ;
	ssdim = UNUSED ; cellcolornum = UNUSED ; rowcolornum = UNUSED ;
	HTTP_HTML_Count = 0 ; dhtml = TRUE ; ZUS(dynmenubuf) ;
	ZUS(dupfilename) ;
	for(i=0;i<RI_ColMax;i++) { colcolornum[i] = UNUSED ; } ;

/*	Make pass thru file to determine rows & columns, pull out headers, titles, etc. */
	ri->RowCount = 0 ; bytesread = 0 ; gotfnp = FALSE ; framecnt = 0 ;
	ri->CurCol = 0 ; ok = TRUE ; returnv4r = FALSE ; ri->GlobalFontSize = RI_DefaultFontSize ;
	for(;ok;)
	 { len = fread(buf,1,2,fp) ;
	   if (len < 2)
	    break ;
	   type = buf[0] ;			/* Entry type */
	   bytes = buf[1] ;			/* Length of entry */
//printf("type=%d, bytes=%d\n",type,bytes) ;

	   if (type == V4SS_Type_LongEntry)	/* Long Entry - have to handle special! */
	    { len = fread(&buf[2],1,2,fp) ;	/* Read extra 2 bytes of header */
	      if (len < 2) break ;
	      type = buf[1] ;			/* Real header is second byte */
	      bytes = (buf[2] << 8) + buf[3] ;	/* Construct real length */
	      len = fread(buf,1,bytes-2,fp) ;	/* Read remainder of entry */
	      buf[bytes-2] = '\0' ;
	      if (len < bytes-2) break ;
	    } else if (bytes > 2)
		    { len = fread(buf,1,bytes-2,fp) ;	/* Read remainder of entry */
		      buf[bytes-2] = '\0' ;
		      if (len < bytes-2) break ;
		    } else { ZS(buf) ; } ;

	   bytesread += bytes ;
	   sig = FALSE ; justify = UNUSED ; ri->CurRow = 1 ;
	   switch(type)
	    { default:
		printf("Invalid type = %d\n",type) ; break ;
	      case V4SS_Type_EOF:		ok = FALSE ; break ;
	      case V4SS_Type_EOL:
		if (ri->CurCol > 0) ri->RowCount++ ;	/* Only increment row if we got at least something in a column */
		ri->CurCol = 0 ; ri->xHTML = 0 ; rowcolornum = UNUSED ;
		for(i=0;i<ri->HTMLCount;i++) ZS(ri->HTML[i].Buf) ;
		ri->CurRow++ ; ri->HTMLCount = 0 ; gotfnp = FALSE ;
		break ;
	      case V4SS_Type_Int:
		rth_CSS(NULL,vdfm,ssdim,(gotfnp ? &vfsnp : NULL),rowcolornum,colcolornum[ri->CurCol],cellcolornum,RI_JustifyRight,FALSE,&cselect,&nobr) ;
		sig = TRUE ; justify = RI_JustifyRight ; ssdim = UNUSED ; gotfnp = FALSE ;
		break ;
	      case V4SS_Type_Double:
		rth_CSS(NULL,vdfm,ssdim,(gotfnp ? &vfsnp : NULL),rowcolornum,colcolornum[ri->CurCol],cellcolornum,RI_JustifyRight,FALSE,&cselect,&nobr) ;
		sig = TRUE ; justify = RI_JustifyRight ; ssdim = UNUSED ; gotfnp = FALSE ;
		break ;
	      case V4SS_Type_UDate:
		rth_CSS(NULL,vdfm,ssdim,(gotfnp ? &vfsnp : NULL),rowcolornum,colcolornum[ri->CurCol],cellcolornum,RI_JustifyRight,FALSE,&cselect,&nobr) ;
		sig = TRUE ; justify = RI_JustifyRight ; ssdim = UNUSED ; gotfnp = FALSE ;
		break ;
	      case V4SS_Type_FAlpha:
	      case V4SS_Type_Alpha:
		{ static int once=TRUE ;
		  if (once)
		   { rth_CSS(NULL,vdfm,ssdim,(gotfnp ? &vfsnp : NULL),rowcolornum,colcolornum[ri->CurCol],cellcolornum,RI_JustifyLeft,TRUE,&cselect,&nobr) ;
		     rth_CSS(NULL,vdfm,ssdim,(gotfnp ? &vfsnp : NULL),rowcolornum,colcolornum[ri->CurCol],cellcolornum,RI_JustifyRight,FALSE,&cselect,&nobr) ;
		     rth_CSS(NULL,vdfm,ssdim,(gotfnp ? &vfsnp : NULL),rowcolornum,colcolornum[ri->CurCol],cellcolornum,RI_JustifyRight,TRUE,&cselect,&nobr) ;
		     once = FALSE ;
		   } ;
		}
		rth_CSS(NULL,vdfm,ssdim,(gotfnp ? &vfsnp : NULL),rowcolornum,colcolornum[ri->CurCol],cellcolornum,RI_JustifyLeft,FALSE,&cselect,&nobr) ;
		sig = TRUE ; ssdim = UNUSED ; gotfnp = FALSE ;
		break ;
	      case V4SS_Type_UMonth:		sig = TRUE ; justify = RI_JustifyRight ; break ;
	      case V4SS_Type_UQtr:		sig = TRUE ; justify = RI_JustifyRight ; break ;
	      case V4SS_Type_UPeriod:		sig = TRUE ; justify = RI_JustifyRight ; break ;
	      case V4SS_Type_UWeek:		sig = TRUE ; justify = RI_JustifyRight ; break ;
	      case V4SS_Type_UndefVal:		sig = TRUE ; break ;
	      case V4SS_Type_Formula:		sig = TRUE ; break ;
	      case V4SS_Type_SSVal:		sig = TRUE ; break ;
	      case V4SS_Type_Fixed:		sig = TRUE ; break ;
	      case V4SS_Type_Warn:
	      case V4SS_Type_Alert:
	      case V4SS_Type_Err:
		p = buf ;
		if (strncmp(p,"<V4Err>",7) == 0)
		 { p += 7 ; p1 = strstr(p,"</V4Err>") ; if (p1 != NULL) *p1 = '\0' ; } ;
		strcat(ri->ErrMsg,p) ;
		if (strlen(ri->ErrMsg) == 0) strcpy(ri->ErrMsg,"An error occurred during the processing of you request.") ;
		break ;
	      case V4SS_Type_URLAfterErr:
		p = buf ;
		if (strncmp(p,"<V4ErrURL>",10) == 0)
		 { p += 10 ; p1 = strstr(p,"</V4ErrURL>") ; if (p1 != NULL) *p1 = '\0' ; } ;
		strcpy(ri->ErrURL,p) ; break ;
	      case V4SS_Type_HTTP_HTML:
		if (HTTP_HTML_Count >= HTTP_HeaderMax) break ;		/* Don't go over max */
		HTTP_HTML[HTTP_HTML_Count] = v4mm_AllocChunk(strlen(buf)+1,FALSE) ; strcpy(HTTP_HTML[HTTP_HTML_Count],buf) ;
		HTTP_HTML_Count++ ; break ;
	      case V4SS_Type_HTTP_Other:	break ;			/* We are generating HTML - can ignore this */
	      case V4SS_Type_FileName:		break ;			/* Ignored in this program */
	      case V4SS_Type_Duplicate:		if (strstr(buf,".htm") != NULL || strstr(buf,".HTM") != NULL) UCstrcpyAtoU(dupfilename,buf) ; break ;
	      case V4SS_Type_FormatControl:
	      case V4SS_Type_FormatStyleName:
	      case V4SS_Type_FormatFont:
	      case V4SS_Type_FormatStyle:
	      case V4SS_Type_FormatFontSize:
	      case V4SS_Type_FormatFormat:
	      case V4SS_Type_PageBreak:
	      case V4SS_Type_InsertRow:		break ;
	      case V4SS_Type_SetColWidth:	dnum = atof(buf) ;
						/* If width > 1 then convert from Excel character width to pixels
						   (about 6 pixels per character as approximation) */
						if (dnum > 0.0 && dnum < 1.0) { ri->Col[ri->CurCol].ColWidth = dnum * 100.0 ; }
						 else { ri->Col[ri->CurCol].ColWidth = (dnum * 6) + 100 ; } ;
						break ;
	      case V4SS_Type_ScriptInclude:	if (ri->IncCount < RI_INCMAX) strcpy(ri->IncFile[ri->IncCount].Name,buf) ;
						ri->IncCount++ ; break ;
	      case V4SS_Type_CSS:		if (ri->CSSCount < RI_CSSMAX) strcpy(ri->CSS[ri->CSSCount].Entry,buf) ;
						ri->CSSCount++ ; break ;
	      case V4SS_Type_SourceCSS:
	      case V4SS_Type_SourceJS:		if (ri->srcCount < RI_SRCMAX) strcpy(ri->srcFile[ri->srcCount].Name,buf) ;
						ri->srcCount++ ; break ;
	      case V4SS_Type_Menu1:
	      case V4SS_Type_Menu2:
	      case V4SS_Type_Eval:
	      case V4SS_Type_Justify:
	      case V4SS_Type_Echo:
	      case V4SS_Type_NoEcho:
	      case V4SS_Type_Header:
	      case V4SS_Type_Footer:
	      case V4SS_Type_LMargin:
	      case V4SS_Type_RMargin:
	      case V4SS_Type_TMargin:
	      case V4SS_Type_BMargin:
	      case V4SS_Type_Portrait:
	      case V4SS_Type_Landscape:
	      case V4SS_Type_Scale:
	      case V4SS_Type_Page:		break ;
	      case V4SS_Type_Grid:		ri->GridLines = buf[0] ; break ;
	      case V4SS_Type_SetPrintTitles1:	ri->Div[ri->DivCount > 0 ? ri->DivCount-1 : 0].ColCapRowCount = buf[0] ; break ;
	      case V4SS_Type_AutoHead:		ri->AutoHead = buf[0] ; break ;
	      case V4SS_Type_SetPrintTitles2:
	      case V4SS_Type_SetColGap:
	      case V4SS_Type_SetRowGap:
	      case V4SS_Type_SetRows:
	      case V4SS_Type_SetColumns:
	      case V4SS_Type_AutoFit:
	      case V4SS_Type_Run:
	      case V4SS_Type_XOffset:
	      case V4SS_Type_YOffset:
	      case V4SS_Type_UpdOK:		break ;
	      case V4SS_Type_Sheet:
		if (ri->DivCount >= RI_DIVMAX) { printf("? Exceeded max number of divisons/sheets!\n") ; exit(EXITABORT) ; } ;
		strcpy(ri->Div[ri->DivCount].Name,buf) ;
		ri->DivCount++ ; break ;
	      case V4SS_Type_Position:
/*		If this is first time or "last" division/sheet has attributes then increment to next one */
		if (ri->DivCount == 0 ? TRUE : strlen(ri->Div[ri->DivCount-1].Attributes) > 0) ri->DivCount++ ;
		if (strlen(ri->Div[ri->DivCount-1].Name) == 0) sprintf(ri->Div[ri->DivCount-1].Name,"Table%d",ri->DivCount) ;
		strcpy(ri->Div[ri->DivCount-1].Attributes,buf) ;
		break ;
	      case V4SS_Type_HorRuleEvery:	ri->HorRuleEveryRow = buf[0] ; break ;
	      case V4SS_Type_DHTMLEnable:	dhtml = buf[0] ;

	      case V4SS_Type_BackgroundImage:	UCstrcpyAtoU(bkgbuf,buf) ; bkg = bkgbuf ; break ;
	      case V4SS_Type_BackgroundColor:	strcpy(bgcolor,buf) ; break ;
	      case V4SS_Type_Server:		UCstrcpyAtoU(cgiserverbuf,buf) ; cgiserver = cgiserverbuf ; break ;
	      case V4SS_Type_LogoImage:		UCstrcpyAtoU(headerimgbuf,buf) ; headerimg = headerimgbuf ; break ;
	      case V4SS_Type_LogoURL:		UCstrcpyAtoU(headerurlbuf,buf) ; headerurl = headerurlbuf ; break ;
	      case V4SS_Type_Session:		UCstrcpyAtoU(tranidbuf,buf) ; tranid = tranidbuf ; break ;
	      case V4SS_Type_DHTMLCode:		UCstrcpyAtoU(dynmenubuf,buf) ; option = dynmenubuf ; break ;

	      case V4SS_Type_Row:
	      case V4SS_Type_Column:		break ;
	      case V4SS_Type_RowColor:		rowcolornum = atoi(buf) ; break ;
	      case V4SS_Type_ColColor:		colcolornum[ri->CurCol] = atoi(buf) ; break ;
	      case V4SS_Type_CellColor:		break ;
	      case V4SS_Type_GlobalFont:	strcpy(ri->GlobalFont,buf) ; if (vdfm != NULL) strcpy(vdfm->GlobalFont,buf) ; break ;
	      case V4SS_Type_GlobalFontSize:	ri->GlobalFontSize = atoi(buf) ; if (vdfm != NULL) vdfm->GlobalFontSize = ri->GlobalFontSize ; break ;
	      case V4SS_Type_InsertObject:
	      case V4SS_Type_Note:
	      case V4SS_Type_Message:		break ;
	      case V4SS_Type_End:
		for(b2=buf,i=0,b1=b2;b1!=NULL && i<=4;i++,b2=b1+1)
		 { b1 = strchr(b2,'\n') ; if (b1 != NULL) *b1 = '\0' ;
		   if (strchr(b2,27) == NULL) { strcpy(ri->Title[i],b2) ; }
		    else { char *b ; int x ;
			   for(t1[0]='\0',x=0;;)
			    { b=strchr(b2,27) ; if (b == NULL) { strcat(t1,b2) ; break ; } ;
			      *b = '\0' ; strcat(t1,b2) ; strcat(t1,ri->HTML[x++].Buf) ; b2 = b + 1 ;
			    } ;
			   strcpy(ri->Title[i],t1) ;
			 } ;
/*		   If we got multiple divisions then also copy title line(s) into division */
		   if (ri->DivCount > 0) strcpy(ri->Div[ri->DivCount - 1].Title[i],ri->Title[i]) ;
		 } ;
		ri->Div[ri->DivCount > 0 ? ri->DivCount-1 : 0].ColCapRowCount = 1 ; ri->AutoHead = TRUE ;
		if (strlen(ri->Title[0]) == 0) strcpy(ri->Title[0]," ") ;		/* Put something in it so we know we hit End:: tag */
		break ;
	      case V4SS_Type_PageBreakAfter:	break ;
	      case V4SS_Type_FrameCnt:		framecnt = buf[0] ; break ;
	      case V4SS_Type_Id:		strcpy(ri->Id,buf) ; break ;
	      case V4SS_Type_Desc:		strcat(ri->DescInfo,buf) ; break ;
	      case V4SS_Type_DescX:		strcat(ri->DescInfo,buf) ; break ;
	      case V4SS_Type_URLBaseX:		strcat(ri->URLBase[0],buf) ; break ;
	      case V4SS_Type_URLBase:		strcat(ri->URLBase[0],buf) ; break ;
	      case V4SS_Type_URLBase2X:		strcat(ri->URLBase[1],buf) ; break ;
	      case V4SS_Type_URLBase2:		strcat(ri->URLBase[1],buf) ; break ;
	      case V4SS_Type_URLBase3X:		strcat(ri->URLBase[2],buf) ; break ;
	      case V4SS_Type_URLBase3:		strcat(ri->URLBase[2],buf) ; break ;
	      case V4SS_Type_URLBase4X:		strcat(ri->URLBase[3],buf) ; break ;
	      case V4SS_Type_URLBase4:		strcat(ri->URLBase[3],buf) ; break ;
	      case V4SS_Type_URLBase5X:		strcat(ri->URLBase[4],buf) ; break ;
	      case V4SS_Type_URLBase5:		strcat(ri->URLBase[4],buf) ; break ;
	      case V4SS_Type_URLLink:		break ;
	      case V4SS_Type_URLLink2:		break ;
	      case V4SS_Type_URLLink3:		break ;
	      case V4SS_Type_URLLink4:		break ;
	      case V4SS_Type_URLLink5:		break ;
	      case V4SS_Type_URLLinkX:		break ;
	      case V4SS_Type_HTML:		if (ri->HTMLCount >= RI_HTMLMax)
						 { rth_DisplayProblem("Exceeded max embedded HTML expressions") ; break ; } ;
						strcat(ri->HTML[ri->HTMLCount].Buf,buf) ; ri->HTMLCount++ ; break ;
	      case V4SS_Type_HTMLX:		if (ri->HTMLCount < RI_HTMLMax) strcat(ri->HTML[ri->HTMLCount].Buf,buf) ; break ;
	      case V4SS_Type_SSNotDim:		ssdim = -buf[0] ; break ;
	      case V4SS_Type_SSDim:		ssdim = buf[0] ; break ;
	      case V4SS_Type_FormatSpecNP:
		memcpy(&vfsnp,buf,sizeof vfsnp) ; gotfnp = TRUE ; break ;
	      case V4SS_Type_FormatSpec:
#define GetVFS(entry) if (ri->entry == NULL) ri->entry = (struct V4SS__FormatSpec *)v4mm_AllocChunk(sizeof *vfs,TRUE) ; memcpy(ri->entry,buf,sizeof *vfs) ; break ;
		switch (ssdim)			/* Check if ssdim is a 'special dimension' */
		 {
		   case V4SS_DimId_Table:	GetVFS(vfsTable) ;
		   case	V4SS_DimId_Row:		GetVFS(vfsRow) ;
		   case V4SS_DimId_Cell:	GetVFS(vfsCell) ;
		   case V4SS_DimId_HdrRow:	GetVFS(vfsHdrRow) ;
		   case V4SS_DimId_HdrCell:	GetVFS(vfsHdrCell) ;
		   case V4SS_DimId_TopOfPage:	GetVFS(vfsTopOfPage) ;
		 } ; if (ssdim < 0) break ;
		if (vdfm == NULL)			/* Allocate structure if not already done */
		 { vdfm = (struct V4SS__DimToFormatMap *)v4mm_AllocChunk(sizeof *vdfm,TRUE) ; } ;
		if (ssdim != UNUSED && ssdim < V4SS__DimToFormat_Max)
		 { if (vdfm->vfs[ssdim] == NULL) vdfm->vfs[ssdim] = (struct V4SS__FormatSpec *)v4mm_AllocChunk(sizeof *vfs,TRUE) ;
		   vfs = vdfm->vfs[ssdim] ; memcpy(vfs,buf,sizeof *vfs) ;
//		   if (vfs->FontSize == 0) vfs->FontSize = RI_DefaultFontSize ;		/* Default for now */
		 } ;
		ssdim = UNUSED ;
		vdfm->GlobalFontSize = ri->GlobalFontSize ; strcpy(vdfm->GlobalFont,ri->GlobalFont) ;
	    } ;
	   if (sig)						/* Track width of column */
	    { 
	      if (ri->CurCol >= RI_ColMax) ri->CurCol = RI_ColMax-1 ;
	      if (ri->Col[ri->CurCol].CharWidth < strlen(buf)) ri->Col[ri->CurCol].CharWidth = strlen(buf) ;
	      if (ri->Col[ri->CurCol].FontSize == 0) ri->Col[ri->CurCol].FontSize = RI_DefaultFontSize ;
	      if (justify != UNUSED) ri->Col[ri->CurCol].Justify = justify ;
	      ri->CurCol ++ ; if (ri->CurCol > ri->ColCount) ri->ColCount = ri->CurCol ;	/* Track highest column */
	    } ;
	 } ;

//	if (bytesread < 10)
//	 { printf("? Input file appears empty\n") ; exit(EXITABORT) ; } ;

gen_error:
	if (ri->ErrMsg[0] != '\0')			/* Got an error? */
	 { int i,j ;
	   ofp = UCfopen(ofile,"w") ;
	   if (ofp == NULL) { v_Msg(NULL,errbuf,"VSrtOutFile",ofile,errno) ; vout_UCText(VOUT_Err,0,errbuf) ; exit(EXITABORT) ; } ;
	   if (TRACE) { v_Msg(NULL,errbuf,"VResHTMLTrace3",ofile) ; vout_UCText(VOUT_Trace,0,errbuf) ; } ;
	   fputs("<html>\n",ofp) ;
	   fputs("\n",ofp) ;
	   fputs("<SCRIPT LANGUAGE=JavaScript>",ofp) ;	/* Return JavaScript to pop up  alert & return to prior page */
	   for(i=0,j=0;;i++)				/* Take care of any embedded double-quotes */
	    { if (ri->ErrMsg[i] == '"' && (i == 0 ? TRUE : ri->ErrMsg[i-1] != '\\')) { t1[j++] = '\\' ; }
	      if (ri->ErrMsg[i] == '\\' && ri->ErrMsg[i+1] != '\\') { t1[j++] = '\\' ; }
	      t1[j++] = ri->ErrMsg[i] ;
	      if (ri->ErrMsg[i] == '\0') break ;
	    } ;
	   fprintf(ofp,"alert(\"%s\") ;",t1) ;
	   if (strlen(ri->ErrURL) == 0) {  fputs("history.back() ;",ofp) ; }
	    else { fprintf(ofp,"location.href = '%s' ;",ri->ErrURL) ; } ;
	   fputs("</SCRIPT>",ofp) ;
	   fclose(ofp) ;				/* Close the output file */
	   exit(EXITOK) ;
	 } ;

/*	If file less than mincompress bytes then don't bother to compress */
	if (bytesread < mincompress) compress = FALSE ;

/*	If generating multi-frame output then make main output file (ofile) the frameset
	 then close & reopen new file for actual output and link that to the frameset */

//framecnt = 2 ;
//cgiserver="http://www.mksinc.com/scripts/dfj2cgi.exe" ;
//areas="Read v4_datav4a:dfj2setup" ;
//leKey=12345 ;
	if (framecnt > 1)
	 { ofp = UCfopen(ofile,"w") ;
	   if (ofp == NULL) { v_Msg(NULL,errbuf,"VSrtOutFile",ofile,errno) ; vout_UCText(VOUT_Err,0,errbuf) ; exit(EXITABORT) ; } ;
	   if (TRACE) { v_Msg(NULL,errbuf,"VResHTMLTrace4",ofile) ; vout_UCText(VOUT_Trace,0,errbuf) ; } ;
/*	   Make new output file name by appending "l" (presumably making .htm -> .html) */
	   UCstrcpy(newofile,ofile) ; UCstrcat(newofile,UClit("l")) ; ofile = newofile ;
	   fputs("<html>\n\n <head>\n",ofp) ;
	   fprintf(ofp,"   <title>%s</title>\n",ri->Title[0]) ;
	   fprintf(ofp,"   <base target='_top'>\n") ;
	   if (strlen(ri->URLBase[0]) > 0 || compress) rth_OutputJavaScriptUtils(isExample) ;
	   fputs(" </head>\n",ofp) ;
	   switch (framecnt)
	    { case 2:	fprintf(ofp," <frameset rows='1,*'>\n") ; break ;
	      default:
	      case 3:	fprintf(ofp," <frameset rows='1,*,1'>\n") ; break ;
	    } ;
/*	   Now here's the tricky part - the source is going to be URL calling of V4 server to resend newofile (now ofile) */
	   sprintf(t1,"%s?_V4=Process&_Func=ReXmitHTMLforFrames&_LEKey=%d&_Area=%s&OFile=%s",UCretASC(cgiserver == NULL ? UClit("") : cgiserver),leKey,UCretASC(areas == NULL ? UClit("") : areas),UCretASC(ofile)) ;
//strcpy(t1,"abr.html") ;
	   for(i=0;i<framecnt;i++)
	    { fprintf(ofp,"  <frame name='part%d' scrolling='yes' src='%s'>\n",i+1,t1) ; } ;
	   fputs("  <noframes>\n   <body><p>This page uses frames, but your browser doesn't support them.</p></body>\n  </noframes>\n",ofp) ;
	   fputs(" </frameset>\n\n</html>\n",ofp) ;
	   fclose(ofp) ;
	 } ;

/*	Second pass - generate HTML */
	fseek(fp,0,SEEK_SET) ;			/* Position to begin of file */

	if (returnv4r)				/* Return as binary file ? */
	 { ofp = UCfopen(ofile,"wb") ;
	   if (ofp == NULL) { v_Msg(NULL,errbuf,"VSrtOutFile",ofile,errno) ; vout_UCText(VOUT_Err,0,errbuf) ; exit(EXITABORT) ; } ;
	   if (TRACE) { v_Msg(NULL,errbuf,"VResHTMLTrace5",ofile) ; vout_UCText(VOUT_Trace,0,errbuf) ; } ;
	   ifd = UCopen(ifile,O_RDONLY|O_BINARY,0) ;
	   fstat(ifd,&statbuf) ;		/* Get size of the file */
	   fputs("Content-type: application/v4r\n",ofp) ;
	   fprintf(ofp,"Content-length: %d\n\n",statbuf.st_size) ;
	   for(;ok;)
	    { len = fread(buf,1,2,fp) ;
	      if (len < 2) break ;
	      type = buf[0] ;			/* Entry type */
	      bytes = buf[1] ;			/* Length of entry */
	      if (bytes > 2)
	       { len = fread(&buf[2],1,bytes-2,fp) ;	/* Read remainder of entry */
	         if (len < bytes-2) break ;
	       } ;
	      if (type == V4SS_Type_EOF) ok = FALSE ;
	      fwrite(buf,1,bytes,ofp) ;
	    } ;
	   fclose(fp) ; fclose(ofp) ;
	   exit(EXITOK) ;
	 } ;

/*	Open up the output file */
	ofp = UCfopen(ofile,"w") ;
	if (ofp == NULL) { v_Msg(NULL,errbuf,"VSrtOutFile",ofile,errno) ; vout_UCText(VOUT_Err,0,errbuf) ; exit(EXITABORT) ; } ;
	if (TRACE) { v_Msg(NULL,errbuf,"VResHTMLTrace6",ofile) ; vout_UCText(VOUT_Trace,0,errbuf) ; } ;
/*	If no title then assume V4 did not finish to completion (EchoS(... End::xxx)) */
	if (strlen(ri->Title[0]) == 0)
	 { strcpy(ri->Title[0],"<p align='center' style='text-size: larger; color: white; background-color: #c22217'>Report did not appear to finish to completion</p>") ;
	 } ;

	if (HTTP_HTML_Count == 0)
	 { HTTP_HTML[HTTP_HTML_Count] = v4mm_AllocChunk(1 + sizeof "Content-Type: text/html",FALSE) ;
	   strcpy(HTTP_HTML[HTTP_HTML_Count++],"Content-Type: text/html") ;
	 } ;
	fputs("<html>\n",ofp) ;
	fputs("\n",ofp) ;
	fputs("<head>\n",ofp) ;
	for(i=0;i<HTTP_HTML_Count;i++)
	 { b1 = strchr(HTTP_HTML[i],':') ; if (b1 == NULL) continue ;	/* If no ":" then ignore */
	   *b1 = '\0' ; for(;;) { if (*(++b1) != ' ') break ; } ;
	   fprintf(ofp,"<meta http-equiv='%s' content='%s'>\n",HTTP_HTML[i],b1) ;
	 } ;
//	fputs("<meta http-equiv='Content-Type' content='text/html; charset=windows-1252'>\n",ofp) ;
	if (strstr(ri->Title[0],"Report did not apear to finish to completion") != NULL)
	 { fprintf(ofp,"   <title>%s</title>\n","** Report did not appear to finish to completion **") ;
	 } else { fprintf(ofp,"   <title>%s</title>\n",ri->Title[0]) ; } ;
	fprintf(ofp,"   <base target='_top'>\n") ;

	if (rcl == NULL || bytesread < 10)
	 { fputs("<script language='javascript'>\n",ofp) ;
	   fputs("alert('An error occurred, please try again or contact your help-desk for further assistance.');\n",ofp) ;
	   fputs("history.back() ;\n",ofp) ;
	   fputs("</script>\n</html>\n",ofp) ;
	   fclose(ofp) ; fclose(fp) ;
	   printf("? Source file not in valid format!\n") ;
	   exit(EXITABORT) ;
	 } ;

/*	Dump out any javascript/CSS source files to link to */
	for(i=0;i<ri->srcCount;i++)
	 { if (strstr(ri->srcFile[i].Name,".js") != NULL)
	    { fprintf(ofp,"<script language='javascript' src='%s'></script>\n",ri->srcFile[i].Name) ;
	     } else { fprintf(ofp,"<link rel='StyleSheet' href='%s' type='text/css' />\n",ri->srcFile[i].Name) ; } ;
	 } ;
/*	Define default styles */
	for(maxu=0,i=0;i<rcl->Count;i++)
	 { if (rcl->CSS[i].Usage > maxu)
	   { maxi = i ; maxu = rcl->CSS[i].Usage ; } ;
	 } ; defaulttd = rcl->CSS[maxi].CSSValue ;
	for(i=0;i<ri->CSSCount;i++)			/* If CSS entry does not contain "{" then assume it is link to file */
	 { if (strstr(ri->CSS[i].Entry,"{") == NULL)
	    fprintf(ofp,"<link rel='StyleSheet' href='%s' type='text/css' />\n",ri->CSS[i].Entry) ;
	 } ;
	fputs("<style>\n<!--\n",ofp) ;
	switch (ri->GridLines)
	 { default:	
//			fputs("td { text-align: right; }\n",ofp) ; defaulttd = "text-align: right; " ;
			break ;
	   case 1:
	   case 2:	
//			fputs("td { text-align: right; border-bottom: 1px solid darkgray; border-left-width: 0px; border-right: 1px solid darkgray; border-top-width: 0px;}\n",ofp) ;
//			defaulttd = "text-align: right; border-bottom: 1px solid darkgray; border-left-width: 0px; border-right: 1px solid darkgray; border-top-width: 0px; " ;
//			fputs("td.first { text-align: right; border-bottom: 1px solid darkgray; border-left: 1px solid darkgray; border-right: 1px solid darkgray; border-top-width: 0px;}\n",ofp) ;
//			fputs("td.top { text-align: right; border-bottom: 1px solid darkgray; border-left-width: 0px; border-right: 1px solid darkgray; border-top: 1px solid darkgray;}\n",ofp) ;
//			fputs("td.firsttop { text-align: right; border-bottom: 1px solid darkgray; border-left: 1px solid darkgray; border-right: 1px solid darkgray; border-top: 1px solid darkgray;}\n",ofp) ;
			break ;
	 } ;
	fputs("hr { border:0; height:'1px'; color:#c3fdb8 }\n",ofp) ;
	fputs("h1 { text-align: center; font-family: Arial Narrow; font-weight:bold; margin-bottom: 1% ; font-size: 120%;}\n",ofp) ;

	if (ri->vfsTable != NULL) { v_VFStoCSS(NULL,ri->vfsTable,usbuf,UNUSED) ; fprintf(ofp,"table { %s }\n",UCretASC(usbuf)) ; } ;
	if (ri->vfsRow != NULL) { v_VFStoCSS(NULL,ri->vfsRow,usbuf,UNUSED) ; fprintf(ofp,"tr { %s }\n",UCretASC(usbuf)) ; } ;
	if (ri->vfsCell != NULL) { v_VFStoCSS(NULL,ri->vfsCell,usbuf,UNUSED) ; fprintf(ofp,"td { %s }\n",UCretASC(usbuf)) ; } ;
	if (ri->vfsHdrRow != NULL) { v_VFStoCSS(NULL,ri->vfsHdrRow,usbuf,UNUSED) ; fprintf(ofp,"thead { %s }\n",UCretASC(usbuf)) ; } ;
	if (ri->vfsHdrCell != NULL) { v_VFStoCSS(NULL,ri->vfsHdrCell,usbuf,UNUSED) ; fprintf(ofp,"th { %s }\n",UCretASC(usbuf)) ; } ;
	if (ri->vfsTopOfPage != NULL)
	 { v_VFStoCSS(NULL,ri->vfsTopOfPage,usbuf,UNUSED) ;
	   fprintf(ofp,"table.TopOfPage { %s }\n",UCretASC(usbuf)) ;
	   fprintf(ofp,"tr.TopOfPage { %s }\n",UCretASC(usbuf)) ;
	   fprintf(ofp,"ttd.TopOfPage { %s }\n",UCretASC(usbuf)) ;
	 } ;

	for(i=0;i<ri->CSSCount;i++)		/* Dump out all 'literal' CSS entries (but not those assumed to be link to file) */
	 { if (strstr(ri->CSS[i].Entry,"{") != NULL) fprintf(ofp,"%s\n",ri->CSS[i].Entry) ;
	 } ;
	for(i=0;i<rcl->Count;i++)
	 { if (strlen(rcl->CSS[i].CSSName) == 0) continue ;
	   if (i == maxi) { fprintf(ofp,"td { %s}\n",rcl->CSS[i].CSSValue) ; }
	    else { fprintf(ofp,"#%s { %s}\n",rcl->CSS[i].CSSName,rcl->CSS[i].CSSValue) ; } ;
	 } ; rcl->InitialCount = rcl->Count ;			/* Save this count - if over then new style section */
	fputs("-->\n</style>\n",ofp) ;

	sprintf(defaultsbuf,"size=\"%d\"",RI_DefaultFontSize) ;	/* Keep default for checks down the line */
	if (isExample)
	 { for(i=0;i<V4SS_URLBaseMax;i++) strcpy(ri->URLBase[i],"xxx") ;	/* Dummy in fake url base */
	 } ;
	if (strlen(ri->URLBase[0]) > 0 || compress || framecnt > 1) rth_OutputJavaScriptUtils(isExample) ;
	fputs("</head>\n",ofp) ;

	if ((strlen(ri->URLBase[0]) > 0 || compress) && framecnt <= 1)		/* Define URL1 javascript routine */
	 rth_OutputJavaScriptUtils(isExample) ;

	if (strlen(ri->DescInfo) > 0 && !isExample)				/* Create javascript function to link to example-saver */
	 { fputs("<script language='javascript'>\n",ofp) ;
	   fputs("function ExampleSaver() {\n",ofp) ;
	   fputs("  var bd = prompt('Enter a brief description of the example (you may change it later)')\n",ofp) ;
	   fputs("  if(bd == null) return ;\n",ofp) ;
	   fprintf(ofp," location.href='%s?_V4=Process&_Func=ExampleSaver&_leKey=%d&_Area=%s&List=%s&Alpha=' + escape(bd) ;\n",UCretASC(cgiserver == NULL ? UClit("") : cgiserver),leKey,UCretASC(areas == NULL ? UClit("") : areas),ri->DescInfo) ;
	   fputs("}\n",ofp) ;
	   fputs("</script>\n",ofp) ;
	 } ;
	
	if (ri->IncCount >= RI_INCMAX)
	 { rth_DisplayProblem("? Exceeded max number of script includes") ; } ;
	for(i=0;i<ri->IncCount && i < RI_INCMAX;i++)			/* Any script files to include? */
	 { ifp = fopen(ri->IncFile[i].Name,"r") ;
	   if (ifp == NULL)
	    { char err[256] ;
	      sprintf(err,"? Could not access (%d) script include file (%s), continuing...",errno,ri->IncFile[i].Name) ;
	      rth_DisplayProblem(err) ; continue ;
	    } ;
	   if (TRACE) { v_Msg(NULL,errbuf,"VResHTMLTrace8",ASCretUC(ri->IncFile[i].Name)) ; vout_UCText(VOUT_Trace,0,errbuf) ; } ;
	   for(;;)
	    { if (fgets(buf,sizeof buf,ifp) == NULL) break ;
	      fputs(buf,ofp) ;
	    } ; fclose(ifp) ;
	 } ;

	
	strcpy(t1,"<body") ; if (bkg != NULL) { sprintf(t2," background='%s'",UCretASC(bkg)) ; strcat(t1,t2) ; } ;
	if (strlen(bgcolor) > 0) { sprintf(t2," bgcolor='%s'",bgcolor) ; strcat(t1,t2) ; } ;
	if (bodyonload != NULL) { sprintf(t2," onload='%s'",UCretASC(bodyonload)) ; strcat(t1,t2) ; } ;
	strcat(t1,">") ; fprintf(ofp,"%s\n",t1) ;
//	fprintf(ofp,"<body %s>",
//	if (bkg != NULL)				/* Handle background */
//	 { fprintf(ofp,"<body background=\"%s\">\n",bkg) ; }
//	 else { fputs("<body>\n",ofp) ; } ;
	fputs("<div align=\"left\">\n",ofp) ;

/*	Output header image / link to main menu ? */
	if (headerimg != NULL ? (UCstrstr(headerimg,UClit(".htm")) || UCstrstr(headerimg,UClit(".txt"))) : FALSE)
	 { ifp = UCfopen(headerimg,"r") ;		/* Insert contents of "include" file here */
	   if (ifp == NULL) { v_Msg(NULL,errbuf,"VSrtInpErr",headerimg,errno) ; vout_UCText(VOUT_Err,0,errbuf) ; exit(EXITABORT) ; } ;
	   if (ifp != NULL)
	    { for(;;) { if (fgets(pbuf,sizeof pbuf,ifp) == NULL) break ; fputs(pbuf,ofp) ; } ; fclose(ifp) ; } ;
	 } else if (headerimg != NULL || menu != NULL || (option != NULL && dhtml))
	 { fprintf(ofp,"<table class='TopOfPage' border=0 width='100%%'><tr class='TopOfPage'>\n") ;
	   if (hdrimgsize != UNUSED)
	    { fprintf(ofp,"<td class='TopOfPage' width='%d%%' style='border: 0px;'>",hdrimgsize) ; }
	    else { fprintf(ofp,"<td class='TopOfPage' style='border: 0px;'>") ; } ;
	   if (headerimg != NULL)
	    { if (headerurl != NULL)
	       { fprintf(ofp,"<a href=\"%s\"><img border=\"0\" src=\"%s\"></a>",UCretASC(headerurl),UCretASC(headerimg)) ;
	       } else
	       { fprintf(ofp,"<img border=\"0\" src=\"%s\">",UCretASC(headerimg)) ;
	       } ;
	    } ;
	   if (menu != NULL)
	    { if ((UCstrcmp(menu,UClit("V4")) == 0 || UCstrcmp(menu,UClit("v4")) == 0) && areas != NULL)
	       { fprintf(ofp,"<td class='TopOfPage' style='border: 0px;'><a href=%s?_V4=Menu&_Func=MainMenu&_LEKey=%d&_Area=%s>(main menu)</a>\n",UCretASC(cgiserver == NULL ? UClit("") : cgiserver),leKey,UCretASC(areas == NULL ? UClit("") : areas)) ;
	       } else if ((UCstrcmp(menu,UClit("MIDAS")) == 0 || UCstrcmp(menu,UClit("midas")) == 0))
		 { fprintf(ofp,"<td class='TopOfPage' style='border: 0px;'><a href=%s?Info=MainMenu&_LEKey=%d&_Area=%s>(main menu)</a>>\n",UCretASC(cgiserver == NULL ? UClit("") : cgiserver),leKey,UCretASC(areas == NULL ? UClit("") : areas)) ;
	       } else { fprintf(ofp,"<td class='TopOfPage' style='border: 0px;'><a href=\"%s\"><p align=\"center\">(main menu)</a>\n",UCretASC(menu)) ; } ;
	    } ;
	   if (option != NULL && dhtml)				/* Link to any "option" DHTML */
	    { if (option[0] == UClit('<')) { fputs(UCretASC(option),ofp) ; }
	       else { fprintf(ofp,"<br><a href=# onmouseover=\"showRelativePanel('%s',event)\" onmouseout='hidePanel()'>Options</a>",UCretASC(option)) ; } ;
//	       else { fprintf(ofp,"<a href=# onmouseover=\"showRelativePanel('%s',event)\" onmouseout='hidePanel()'><img border=0 width='100' src='optionbutton.gif'></a>",option) ; } ;
	    } ;
	   if (hdrimgsize == UNUSED)			/* Don't end table if got size - have to include title (below) */
	    { fprintf(ofp,"</td></tr></table>\n") ; }
	    else { fprintf(ofp,"</td>\n") ; } ;
	 } ;

/*	Output the Title Line(s) */
//	if (hdrimgsize != UNUSED && (headerimg != NULL || menu != NULL))
//	 fprintf(ofp,"<td class='TopOfPage' width='%d%%' style='border: 0px;'>",100-2*hdrimgsize) ;
//	for(i=0;strlen(ri->Title[i])>0 && i<=4;i++)
//	 { fprintf(ofp,"<h1 class='TopOfPage' %s>%s</h1>\n",(i>0 ? "style='margin-top:-5'" : ""),rth_FormatStr(vdfm,ri->Title[i])) ;
//	 } ;
//	if (hdrimgsize != UNUSED && (headerimg != NULL || menu != NULL))
//	 { fprintf(ofp,"</td>\n") ;
//	   fprintf(ofp,"<td class='TopOfPage' width='%d%%' style='border: 0px;'>&nbsp;</td>\n",hdrimgsize) ;
//	   fputs("</tr></table>\n",ofp) ;
//	 } ;

/*	Do we need to dump out global report title(s) ? */
	if (ri->DivCount == 0)
	 { for(i=0;strlen(ri->Title[i])>0 && i<=4;i++)
	    { fprintf(ofp,"<h1 class='TopOfPage' %s>%s</h1>\n",(i>0 ? "style='margin-top:-5'" : ""),rth_FormatStr(vdfm,ri->Title[i])) ;
	    } ;
	 } ;



/*	If got multiple divisions & no positioning attributes the create multiple Sheets (like Excel) */
	ri->DivSheets = FALSE ;
	if (ri->DivCount > 0) ri->DivSheets = (strlen(ri->Div[0].Attributes) == 0) ;
	if (ri->DivSheets)
	 { ZS(t1) ;
	   for(i=0;i<ri->DivCount;i++) { strcat(t1,(i > 0 ? ",'" : "'")) ; strcat(t1,ri->Div[i].Name) ; strcat(t1,"'") ; } ;
	   fputs("\
<style>\n\
td.sheet { color:#3bb9ff ; border:0px; padding: 5px; cursor:pointer; }\n\
td.Selsheet { color:blue ; border:0px; padding: 5px; cursor: pointer; }\n\
table.sheet { padding: 2px;}\n\
td.sheetCap { text-align: center; text-size: smaller; color: white; background-color: blue; padding: 0px; }\n\
</style>\n\
<script language='JavaScript' type='text/javascript'>\n",ofp) ;
	   fprintf(ofp," var panels = new Array(%s);\n",t1) ;
	   fputs("\
 function showDivTab(tab, name)\n\
  { for(i=0;i<panels.length;i++)\n\
     { document.getElementById('S_' + panels[i]).style.display = (name == panels[i] ? 'block':'none');\n\
       document.getElementById('TD_' + panels[i]).className = (name == panels[i] ? 'Selsheet':'sheet');\n\
     } ;\n\
    return false;\n\
  }\n\
</script>",ofp) ;

	   fputs("<table class='sheet' align='center' border='0'>\n",ofp) ;
//	   fputs("  <tr style='background: #f6f6f5'>\n",ofp) ;
	   fprintf(ofp,"<tr><td class='sheetCap' colspan='%d'>click on any of the sections below</td></tr>\n",ri->DivCount) ;
	   fputs("  <tr>\n",ofp) ;
	   for(i=0;i<ri->DivCount;i++)
	    { fprintf(ofp,"  <td id='TD_%s' class='%s' onmouseover='javascript:this.style.backgroundColor=\"#e0ffff\"' onmouseout='javascript:this.style.backgroundColor=\"\"' onclick='showDivTab(this,\"%s\");'>%s</td>\n",
		ri->Div[i].Name,(i==0 ? "Selsheet" : "sheet"),ri->Div[i].Name,ri->Div[i].Name) ;
	    } ;
	   fputs("  </tr>\n",ofp) ; fputs("</table>\n",ofp) ;

	 } ;

	
/*	Output TopOfPage bookmark (just for so) */
	fprintf(ofp,"<a name='__TopOfPage__'>\n") ;

/*	Start the main "table" */
	ZS(outbuf) ;
	ri->CurDiv = 0 ;
	if (ri->DivCount > 0)			/* If we got divisions/sheet then setup first before table */
	 { if (strlen(ri->Div[0].Attributes) > 0)
	    { sprintf(pbuf,"<div style='overflow: auto; position: absolute; %s'>",ri->Div[ri->CurDiv].Attributes) ;
	      rth_Output(outbuf,pbuf,ofp) ;
	    } else
	    { sprintf(pbuf,"<div id='S_%s' style='display: %s;'>\n",ri->Div[ri->CurDiv].Name,"block") ;
	      rth_Output(outbuf,pbuf,ofp) ;
	    } ;
/*	   Dump out sheet titles(s) for first sheet/division */
	   for(i=0;strlen(ri->Div[ri->CurDiv].Title[i])>0 && i<=4;i++)
	    { sprintf(pbuf,"<h1 class='TopOfPage' %s>%s</h1>\n",(i>0 ? "style='margin-top:-5'" : ""),rth_FormatStr(vdfm,ri->Div[ri->CurDiv].Title[i])) ;
	      rth_Output(outbuf,pbuf,ofp) ;
	    } ;
	   ri->Div[ri->CurDiv].Initialized = TRUE ;	/* Remember we did this */
	 } ;
	ri->CurDiv = -1 ;			/* Set to one less that 0 so first Sheet/Position will increment to 0 */
	switch (ri->GridLines)
	 { default:		sprintf(t1,"<table align='center' border='%d'>\n",ri->GridLines) ; rth_Output(outbuf,t1,ofp) ; break ;
//	   case 1:		sprintf(t1,"<table align='center' border='1' cellspacing='0' cellpadding='0' bordercolorlight='darkgray' rules='lhs'>\n",ri->GridLines) ; rth_Output(outbuf,t1,ofp) ; break ;
	   case 1:
	   case 2:		sprintf(t1,"<table align='center' border='0' cellspacing='0' cellpadding='0' nowrap>\n",ri->GridLines) ; rth_Output(outbuf,t1,ofp) ; break ;
	 } ;
	ri->CurCol = 0 ; newrow = TRUE ; ok = TRUE ; ri->CurRow = 1 ; emptycnt = 0 ;
	ri->GlobalFontSize = RI_DefaultFontSize ; ZS(ri->GlobalFont) ;
	for(;ok;)
	 { 

	   len = fread(buf,1,2,fp) ;
	   if (len < 2) break ;
	   type = buf[0] ;			/* Entry type */
	   bytes = buf[1] ;			/* Length of entry */
	   if (type == V4SS_Type_LongEntry)	/* Long Entry - have to handle special! */
	    { len = fread(&buf[2],1,2,fp) ;	/* Read extra 2 bytes of header */
	      if (len < 2) break ;
	      type = buf[1] ;			/* Real header is second byte */
	      bytes = (buf[2] << 8) + buf[3] ;	/* Construct real length */
	      len = fread(buf,1,bytes-2,fp) ;	/* Read remainder of entry */
	      buf[bytes-2] = '\0' ;
	      if (len < bytes-2) break ;
	    } else if (bytes > 2)
		    { len = fread(buf,1,bytes-2,fp) ;	/* Read remainder of entry */
		      buf[bytes-2] = '\0' ;
		      if (len < bytes-2) break ;
		    } else { ZS(buf) ; } ;
	   if (ri->CurCol == 0 && newrow)		/* If first column then output new row */
	    { 
 /*	      Got style info for header/captions & within header/caption row range? */
	      if (ri->vfsHdrRow != NULL && ri->CurRow <= ri->Div[(ri->CurDiv < 0 ? 0 : ri->CurDiv)].ColCapRowCount)
	       { rth_Output(outbuf,"<thead>",ofp) ; }
/*	      Got style info and just past header/caption row range? */
	       else if (ri->vfsHdrRow != NULL && ri->CurRow == ri->Div[(ri->CurDiv < 0 ? 0 : ri->CurDiv)].ColCapRowCount+1)
	        { rth_Output(outbuf,"</thead><tr>",ofp) ; }
/*	      Start a plain old new-row */
	       else { rth_Output(outbuf,"<tr>",ofp) ; } ;
	      newrow = FALSE ;
	    } ;

	   vfs = (ssdim != UNUSED && vdfm != NULL ? vdfm->vfs[ssdim] : NULL) ;
//	   if (ri->CurRow == 1)
//	    { strcpy(tdcls,(ri->CurCol > 0 ? " class=top" : " class='firsttop'")) ;
//	    } else { strcpy(tdcls,(ri->CurCol > 0 ? "" : " class='first'")) ; } ;
	   ZS(tdcls) ZS(fldColor)
/*	   If column width specified then deal with it here */
	   if (ri->Col[ri->CurCol].ColWidth != 0)
	    { if (ri->Col[ri->CurCol].ColWidth <= 100) { sprintf(t1," width='%d%%'",ri->Col[ri->CurCol].ColWidth) ; }
	       else { sprintf(t1," width='%d'",ri->Col[ri->CurCol].ColWidth-100) ; } ;
	      strcat(tdcls,t1) ;
	    } ;

switch_again:
	   switch(type)
	    { 
	      case V4SS_Type_EOF:		ok = FALSE ; break ;
	      case V4SS_Type_EOL:
		if (ri->CurCol > 0) ri->CurRow++ ;	/* Only increment row if we got at least something in a column */
		ri->CurCol = 0 ; newrow = TRUE ; emptycnt = 0 ;
		for(i=0;i<ri->HTMLCount;i++) ZS(ri->HTML[i].Buf) ; ri->HTMLCount = 0 ; ri->xHTML = 0 ;
		rth_Output(outbuf,"</tr>\n",ofp) ;
		if (ri->HorRuleEveryRow > 0 && (ri->CurRow - ri->Div[(ri->CurDiv < 0 ? 0 : ri->CurDiv)].ColCapRowCount - 1) > 0)
		 { if (((ri->CurRow - ri->Div[(ri->CurDiv < 0 ? 0 : ri->CurDiv)].ColCapRowCount - 1) % ri->HorRuleEveryRow) == 0)
		    { sprintf(pbuf,"<tr height='3px'><td colspan='%d'><hr></td></tr>",ri->ColCount) ;
		      rth_Output(outbuf,pbuf,ofp) ;
		    } ;
		 } ;
		break ;
	      case V4SS_Type_Int:
		if (gotfnp ? vfsnp.MaskX > 0 : FALSE) { rth_FormatInt(buf,&vfsnp.VarString[vfsnp.MaskX-1],fldColor) ; }
		 else if (vfs != NULL) { if (vfs->MaskX > 0) rth_FormatInt(buf,&vfs->VarString[vfs->MaskX-1],fldColor) ; } ;
		ZS(prefix) ; ZS(suffix) ; cselect = FALSE ;
		iscap = ((ri->CurRow <= ri->Div[(ri->CurDiv < 0 ? 0 : ri->CurDiv)].ColCapRowCount && strlen(buf) > 0) & ri->AutoHead) ;
		css = rth_CSS(buf,vdfm,ssdim,(gotfnp ? &vfsnp : NULL),rowcolornum,colcolornum[ri->CurCol],cellcolornum,RI_JustifyRight,iscap,&cselect,&nobr) ;
		if (ri->CurTitle[0] != '\0')
		 { strcat(tdcls," title='") ; strcat(tdcls,ri->CurTitle) ; strcat(tdcls,"'") ; ZS(ri->CurTitle) ; } ;
		if (strlen(fldColor) > 0) { strcat(tdcls," style='color: ") ; strcat(tdcls,fldColor) ; strcat(tdcls,";'") ; } ;
		if (ri->CurURL[0] != '\0')		/* NOTE: This must be last entry in prefix!!! */
		 { if (ri->CurURL[0] == '+')		/* If starts with "+" then link up with base */
		    { sprintf(t1,"<a href=\"javascript:URL%d('%s')\">",ri->CurURLx,&ri->CurURL[1]) ; strcat(prefix,t1) ;
		    } else { if (strncmp(ri->CurURL,"javascript:",11) == 0)
			      { sprintf(t1,"<a href=\"#\" onclick=\"%s\">",&ri->CurURL[11]) ;
			      } else { sprintf(t1,"<a href=\"%s\">",ri->CurURL) ; } ;
			     strcat(prefix,t1) ;
			   } ;
		   strcat(suffix,"</a>") ;
		 } ;
		if (strlen(buf) == 0 && ri->GridLines != 0) strcpy(buf,"&nbsp;") ;	/* If gridlines then put something in cell */
		if (strlen(buf) == 0 && strlen(ri->CurURL) == 0)
		 { emptycnt++ ;
		 } else
		 { rth_CheckEmpty(&emptycnt,ofp,outbuf) ;
		   if (strlen(css) == 0) { sprintf(pbuf,tdstr,tdcls,prefix,buf,suffix) ; rth_Output(outbuf,pbuf,ofp) ; }
		    else { sprintf(pbuf,tdidstr,tdcls,css,prefix,buf,suffix) ; rth_Output(outbuf,pbuf,ofp) ; } ;
		 } ;
		cellcolornum = UNUSED ;
		ZS(ri->CurURL) ;		/* Clear "current column" URL */
		ri->CurCol++ ; ssdim = UNUSED ; gotfnp = FALSE ; break ;
	      case V4SS_Type_Double:
		if (gotfnp ? vfsnp.MaskX > 0 : FALSE) { rth_FormatDbl(buf,&vfsnp.VarString[vfsnp.MaskX-1],fldColor) ; }
		 else if (vfs != NULL) { if (vfs->MaskX > 0) rth_FormatDbl(buf,&vfs->VarString[vfs->MaskX-1],fldColor) ; } ;
		ZS(prefix) ; ZS(suffix) ; cselect = FALSE ;
		iscap = ((ri->CurRow <= ri->Div[(ri->CurDiv < 0 ? 0 : ri->CurDiv)].ColCapRowCount && strlen(buf) > 0) & ri->AutoHead) ;
		if (ri->CurTitle[0] != '\0')
		 { strcat(tdcls," title='") ; strcat(tdcls,ri->CurTitle) ; strcat(tdcls,"'") ; ZS(ri->CurTitle) ; } ;
		if (strlen(fldColor) > 0) { strcat(tdcls," style='color: ") ; strcat(tdcls,fldColor) ; strcat(tdcls,";'") ; } ;
		css = rth_CSS(buf,vdfm,ssdim,(gotfnp ? &vfsnp : NULL),rowcolornum,colcolornum[ri->CurCol],cellcolornum,RI_JustifyRight,iscap,&cselect,&nobr) ;
		if (ri->CurURL[0] != '\0')		/* NOTE: This must be last entry in prefix!!! */
		 { if (ri->CurURL[0] == '+')		/* If starts with "+" then link up with base */
		    { sprintf(t1,"<a href=\"javascript:URL%d('%s')\">",ri->CurURLx,&ri->CurURL[1]) ; strcat(prefix,t1) ;
		    } else { if (strncmp(ri->CurURL,"javascript:",11) == 0)
			      { sprintf(t1,"<a href=\"#\" onclick=\"%s\">",&ri->CurURL[11]) ;
			      } else { sprintf(t1,"<a href=\"%s\">",ri->CurURL) ; } ;
			     strcat(prefix,t1) ;
			   } ;
		   strcat(suffix,"</a>") ;
		 } ;
		if (strlen(buf) == 0 && ri->GridLines != 0) strcpy(buf,"&nbsp;") ;	/* If gridlines then put something in cell */
		if (strlen(buf) == 0 && strlen(ri->CurURL) == 0)
		 { emptycnt++ ;
		 } else
		 { rth_CheckEmpty(&emptycnt,ofp,outbuf) ;
		   if (strlen(css) == 0) { sprintf(pbuf,tdstr,tdcls,prefix,buf,suffix) ; rth_Output(outbuf,pbuf,ofp) ; }
		    else { sprintf(pbuf,tdidstr,tdcls,css,prefix,buf,suffix) ; rth_Output(outbuf,pbuf,ofp) ; } ;
		 } ;
		cellcolornum = UNUSED ;
		ZS(ri->CurURL)	;		/* Clear "current column" URL */
		ri->CurCol++ ; ssdim = UNUSED ; gotfnp = FALSE ; break ;
	      case V4SS_Type_UDate:
		if (gotfnp ? vfsnp.MaskX > 0 : FALSE) { rth_FormatDate(buf,&vfsnp.VarString[vfsnp.MaskX-1]) ; }
		 else rth_FormatDate(buf,((vfs != NULL ? vfs->MaskX > 0 : FALSE) ? &vfs->VarString[vfs->MaskX-1] : NULL)) ;
		ZS(prefix) ; ZS(suffix) ; cselect = FALSE ;
		iscap = ((ri->CurRow <= ri->Div[(ri->CurDiv < 0 ? 0 : ri->CurDiv)].ColCapRowCount && strlen(buf) > 0) & ri->AutoHead) ;
		if (ri->CurTitle[0] != '\0')
		 { strcat(tdcls," title='") ; strcat(tdcls,ri->CurTitle) ; strcat(tdcls,"'") ; ZS(ri->CurTitle) ; } ;
		css = rth_CSS(buf,vdfm,ssdim,(gotfnp ? &vfsnp : NULL),rowcolornum,colcolornum[ri->CurCol],cellcolornum,RI_JustifyRight,iscap,&cselect,&nobr) ;
		if (ri->CurURL[0] != '\0')		/* NOTE: This must be last entry in prefix!!! */
		 { if (ri->CurURL[0] == '+')		/* If starts with "+" then link up with base */
		    { sprintf(t1,"<a href=\"javascript:URL%d('%s')\">",ri->CurURLx,&ri->CurURL[1]) ; strcat(prefix,t1) ;
		    } else { if (strncmp(ri->CurURL,"javascript:",11) == 0)
			      { sprintf(t1,"<a href=\"#\" onclick=\"%s\">",&ri->CurURL[11]) ;
			      } else { sprintf(t1,"<a href=\"%s\">",ri->CurURL) ; } ;
			     strcat(prefix,t1) ;
			   } ;
		   strcat(suffix,"</a>") ;
		 } ;
		if (strlen(buf) == 0 && ri->GridLines != 0) strcpy(buf,"&nbsp;") ;	/* If gridlines then put something in cell */
		if (strlen(buf) == 0 && strlen(ri->CurURL) == 0)
		 { emptycnt++ ;
		 } else
		 { rth_CheckEmpty(&emptycnt,ofp,outbuf) ;
		   if (strlen(css) == 0) { sprintf(pbuf,tdnowrapstr,tdcls,prefix,buf,suffix) ; rth_Output(outbuf,pbuf,ofp) ; }
		    else { sprintf(pbuf,tdidnowrapstr,tdcls,css,prefix,buf,suffix) ; rth_Output(outbuf,pbuf,ofp) ; } ;
		 } ;
		ZS(ri->CurURL)	;		/* Clear "current column" URL */
		cellcolornum = UNUSED ;
		ri->CurCol++ ; ssdim = UNUSED ; gotfnp = FALSE ; break ;
	      case V4SS_Type_FAlpha:
		if (strlen(ri->CurURL) > 0) break ;	/* Ignore this if we have a URL specified, entry after this will be target value for URL */
		type = buf[0] ;				/* Get target type code */
		b1 = &buf[1] ; b1 = strchr(b1,',') ;	/* Scan past "=If(V4SSExpand," */
		if (b1 == NULL) { type = V4SS_Type_Alpha ; goto switch_again ; } ;
		for(b1=b1+2;;b1++)				/* Scan past second argument */
		 { b1 = strchr(b1,'"') ; if (*(b1-1) != '\\') break ;
		   continue ;
		 } ;
		b1++ ; b1++ ;				/* Should now be on last argument */
		if (*b1 == '"') b1++ ;
		i = strlen(b1) ; if (b1[i-2] == '"') b1[i-2] = '\0' ;
		strcpy(buf,b1) ; goto switch_again ;
	      case V4SS_Type_Alpha:
		if ((b1=strchr(buf,'\n')) != NULL)		/* Do we have multiple lines? */
		 { for(b2=buf,t2[0]='\0';;b1=strchr(b2,'\n'))
		     { if (b1 != NULL) *b1 = '\0' ;
		       sprintf(t1,"<p style=\"margin-bottom: -%d\">%s</p>",(b1==NULL ? 0 : 15),b2) ; strcat(t2,t1) ;
		       if (b1 == NULL) break ;
		       b2 = b1 + 1 ;
		     } ;
		    strcpy(buf,t2) ;
		 } ;
		if ((b1=strchr(buf,27)) != NULL)			/* Do we have embedded escapes for embedded HTML */
		 { for(b2=buf,t2[0]='\0';;b1=strchr(b2,27))
		     { if (b1 != NULL) *b1 = '\0' ;
		       strcat(t2,b2) ; if (b1 == NULL) break ;
		       if (ri->xHTML < ri->HTMLCount) { strcat(t2,ri->HTML[ri->xHTML++].Buf) ; } ;
		       b2 = b1 + 1 ;
		     } ;
		    strcpy(buf,t2) ;
		 } ;
		iscap = ((ri->CurRow <= ri->Div[(ri->CurDiv < 0 ? 0 : ri->CurDiv)].ColCapRowCount && strlen(buf) > 0) & ri->AutoHead) ;
		ZS(prefix) ; ZS(suffix) ;
		if (ri->CurTitle[0] != '\0')
		 { strcat(tdcls," title='") ; strcat(tdcls,ri->CurTitle) ; strcat(tdcls,"'") ; ZS(ri->CurTitle) ; } ;
//		css = rth_CSS(buf,vdfm,ssdim,(gotfnp ? &vfsnp : NULL),rowcolornum,colcolornum[ri->CurCol],cellcolornum,(ri->Col[ri->CurCol].Justify == 0 ? RI_JustifyLeft : ri->Col[ri->CurCol].Justify),iscap,&cselect,&nobr) ;
		css = rth_CSS(buf,vdfm,ssdim,(gotfnp ? &vfsnp : NULL),rowcolornum,colcolornum[ri->CurCol],cellcolornum,RI_JustifyLeft,iscap,&cselect,&nobr) ;
		if (nobr) strcat(prefix,"<nobr>") ;
		if (ri->CurURL[0] != '\0')		/* NOTE: This must be last entry in prefix!!! */
		 { if (ri->CurURL[0] == '+')		/* If starts with "+" then link up with base */
		    { if (strlen(css) > 0)
		       { sprintf(t1,"<a href=\"javascript:URL%d('%s')\">",ri->CurURLx,&ri->CurURL[1]) ; strcat(prefix,t1) ;
		       } else { sprintf(t1,"<a href=\"javascript:URL%d('%s')\">",ri->CurURLx,&ri->CurURL[1]) ; strcat(prefix,t1) ; strcat(suffix,"</a>") ; } ;
		    } else if (ri->CurURL[0] == '&')
		    { sprintf(t1," onclick=\"URL%d('%s')\"",ri->CurURLx,&ri->CurURL[1]) ; strcat(tdcls,t1) ;
		    } else { if (strncmp(ri->CurURL,"javascript:",11) == 0)
			      { sprintf(t1,"<a href=\"#\" onclick=\"%s\">",&ri->CurURL[11]) ;
			      } else { sprintf(t1,"<a href=\"%s\">",ri->CurURL) ; } ;
			     strcat(prefix,t1) ;
			   } ;
		   
		 } ;
		if (nobr) strcat(suffix,"</nobr>") ;
		if (cselect)				/* Look for {n} (may have shifted over if multiline!) */
		 { ha = JustifyStr[cselect] ;
		   if (buf[0] == '{') { cselect = strtol(&buf[1],&b1,10) ; strcpy(buf,b1+1) ; }
		    else { char *b2 ; b2 = strstr(buf,">{") ;
			   if (b2 != NULL) { cselect = strtol(b2+2,&b1,10) ; strcpy(b2+1,b1+1) ; }
			    else { cselect = 1 ; } ;
			 } ;
		   rth_CheckEmpty(&emptycnt,ofp,outbuf) ;
		   if (strlen(css) == 0) { sprintf(pbuf,tdcolstr,tdcls,cselect,prefix,rth_FormatStr(vdfm,buf),suffix) ; rth_Output(outbuf,pbuf,ofp) ; }
		    else { sprintf(pbuf,tdidcolstr,tdcls,css,cselect,prefix,rth_FormatStr(vdfm,buf),suffix) ; rth_Output(outbuf,pbuf,ofp) ; } ;
		   
		 } else
		 { if (strlen(buf) == 0 && ri->GridLines != 0) strcpy(buf,"&nbsp;") ;	/* If gridlines then put something in cell */
		   if (strlen(buf) == 0 && strlen(ri->CurURL) == 0)
		    { emptycnt++ ;
		    } else
		    { if (strcmp(buf," ") == 0) strcpy(buf,"&nbsp;") ;	/* If single space then replace with "&nbsp;" so tables work out ok */
		      rth_CheckEmpty(&emptycnt,ofp,outbuf) ;
			      if (strlen(css) == 0) { sprintf(pbuf,tdstr,tdcls,prefix,rth_FormatStr(vdfm,buf),suffix) ; rth_Output(outbuf,pbuf,ofp) ; }
		       else { sprintf(pbuf,tdidstr,tdcls,css,prefix,rth_FormatStr(vdfm,buf),suffix) ; rth_Output(outbuf,pbuf,ofp) ; } ;
		    } ;
		 } ;
		ZS(ri->CurURL)	;		/* Clear "current column" URL */
		cellcolornum = UNUSED ;
		ri->CurCol++ ; ssdim = UNUSED ; gotfnp = FALSE ; break ;
	      case V4SS_Type_UMonth:		ri->CurCol++ ; ssdim = UNUSED ; break ;
	      case V4SS_Type_UQtr:		ri->CurCol++ ; ssdim = UNUSED ; break ;
	      case V4SS_Type_UPeriod:		ri->CurCol++ ; ssdim = UNUSED ; break ;
	      case V4SS_Type_UWeek:		ri->CurCol++ ; ssdim = UNUSED ; break ;
	      case V4SS_Type_UndefVal:		ri->CurCol++ ; ssdim = UNUSED ; break ;
	      case V4SS_Type_Formula:		ri->CurCol++ ; ssdim = UNUSED ; break ;
	      case V4SS_Type_SSVal:
		switch(buf[0])
		 {
		   case V4SS_SSVal_Null:	strcpy(buf,"") ; break ;
		   case V4SS_SSVal_Div0:	strcpy(buf,"#DIV0!") ; break ;
		   case V4SS_SSVal_Value:	strcpy(buf,"#VAL!") ; break ;
		   case V4SS_SSVal_Ref:		strcpy(buf,"#REF!") ; break ;
		   case V4SS_SSVal_Name:	strcpy(buf,"#NAME!") ; break ;
		   case V4SS_SSVal_Num:		strcpy(buf,"#NUM!") ; break ;
		   case V4SS_SSVal_NA:		strcpy(buf,"#N/A") ; break ;
		   case V4SS_SSVal_Empty:	strcpy(buf,"") ; break ;
		   case V4SS_SSVal_Row:		break ;	/* Current Row?  */
		   case V4SS_SSVal_Column:		/* Current column - save any formats & styles */
			break ;
		 } ;
		ri->CurCol++ ; ssdim = UNUSED ; break ;
	      case V4SS_Type_Fixed:		ri->CurCol++ ; ssdim = UNUSED ; break ;
	      case V4SS_Type_Warn:
	      case V4SS_Type_Alert:
	      case V4SS_Type_Err:		ri->CurCol++ ; ssdim = UNUSED ; break ;
	      case V4SS_Type_FormatControl:
	      case V4SS_Type_FormatStyleName:
	      case V4SS_Type_FormatFont:
	      case V4SS_Type_FormatStyle:	/* not used any more strcpy(ri->CurStyle,buf) ; */ break ;
	      case V4SS_Type_FormatFontSize:	break ;
	      case V4SS_Type_FormatFormat:	/* not used any more strcpy(ri->CurFormat,buf) ; */ break ;
	      case V4SS_Type_HTTP_HTML:
	      case V4SS_Type_HTTP_Other:
	      case V4SS_Type_FileName:		break ;			/* Ignored in this program */
	      case V4SS_Type_PageBreak:
	      case V4SS_Type_PageBreakAfter:	rth_Output(outbuf,"<td><span style='page-break-before: always'>&nbsp;</span></td>",ofp) ; break ;
	      case V4SS_Type_ScriptInclude:	break ;
	      case V4SS_Type_HorRuleEvery:	ri->HorRuleEveryRow = buf[0] ; break ;
	      case V4SS_Type_InsertRow:
	      case V4SS_Type_SetColWidth:
	      case V4SS_Type_Menu1:
	      case V4SS_Type_Menu2:
	      case V4SS_Type_Eval:
	      case V4SS_Type_Justify:
	      case V4SS_Type_Echo:
	      case V4SS_Type_NoEcho:
	      case V4SS_Type_Header:
	      case V4SS_Type_Footer:
	      case V4SS_Type_LMargin:
	      case V4SS_Type_RMargin:
	      case V4SS_Type_TMargin:
	      case V4SS_Type_BMargin:
	      case V4SS_Type_Portrait:
	      case V4SS_Type_Landscape:
	      case V4SS_Type_Scale:
	      case V4SS_Type_Page:
	      case V4SS_Type_SetPrintTitles1:
	      case V4SS_Type_SetPrintTitles2:
	      case V4SS_Type_SetColGap:
	      case V4SS_Type_SetRowGap:
	      case V4SS_Type_SetRows:
	      case V4SS_Type_SetColumns:
	      case V4SS_Type_AutoFit:
	      case V4SS_Type_Run:
	      case V4SS_Type_XOffset:
	      case V4SS_Type_YOffset:		break ;
	      case V4SS_Type_Sheet:
	      case V4SS_Type_Position:
		ri->CurDiv ++ ;
		if (ri->Div[ri->CurDiv].Initialized) break ;	/* Don't go thru here twice */
		if (ri->CurDiv > 0)			/* Already have a division? - then end it 'cause we are gonna create a new one */
		 { rth_Output(outbuf,"</table></div>",ofp) ; } ;
		if (ri->DivSheets)
		 { sprintf(pbuf,"<div id='S_%s' style='display: %s;'>\n",ri->Div[ri->CurDiv].Name,"none") ;
		   rth_Output(outbuf,pbuf,ofp) ;
		 } else
		 { sprintf(pbuf,"<div style='overflow: auto; position: absolute; %s'>",buf) ; rth_Output(outbuf,pbuf,ofp) ;
		 } ;
/*		Dump out sheet titles(s) */
	        for(i=0;strlen(ri->Div[ri->CurDiv].Title[i])>0 && i<=4;i++)
	          { sprintf(pbuf,"<h1 class='TopOfPage' %s>%s</h1>\n",(i>0 ? "style='margin-top:-5'" : ""),rth_FormatStr(vdfm,ri->Div[(ri->CurDiv < 0 ? 0 : ri->CurDiv)].Title[i])) ;
		    rth_Output(outbuf,pbuf,ofp) ;
	          } ;
		sprintf(pbuf,"<table border='0' align='center'>") ; rth_Output(outbuf,pbuf,ofp) ;
		ri->CurRow = 1 ;		/* VEH040528 - set back to "top of page" */
		ri->Div[(ri->CurDiv < 0 ? 0 : ri->CurDiv)].Initialized = TRUE ;
		break ;
	      case V4SS_Type_CSS:		i = 0 ; break ;
	      case V4SS_Type_SourceCSS:
	      case V4SS_Type_SourceJS:		i = 0 ; break ;
	      case V4SS_Type_UpdOK:
						break ;
	      case V4SS_Type_Row:
	      case V4SS_Type_Column:		break ;
	      case V4SS_Type_GlobalFont:	strcpy(ri->GlobalFont,buf) ; if (vdfm != NULL) strcpy(vdfm->GlobalFont,buf) ; break ;
	      case V4SS_Type_GlobalFontSize:	ri->GlobalFontSize = atoi(buf) ; if (vdfm != NULL) vdfm->GlobalFontSize = ri->GlobalFontSize ; break ;
	      case V4SS_Type_RowColor:		rowcolornum = atoi(buf) ; break ;
	      case V4SS_Type_ColColor:		colcolornum[ri->CurCol] = atoi(buf) ; break ;
	      case V4SS_Type_CellColor:		cellcolornum = atoi(buf) ; break ;
	      case V4SS_Type_Title:		strcpy(ri->CurTitle,buf) ; break ;
	      case V4SS_Type_InsertObject:
	      case V4SS_Type_Note:
	      case V4SS_Type_Message:		break ;
	      case V4SS_Type_End:		break ;
	      case V4SS_Type_Id:		break ;
	      case V4SS_Type_URLBaseX:		break ;
	      case V4SS_Type_URLBase:		break ;
	      case V4SS_Type_URLBase2X:		break ;
	      case V4SS_Type_URLBase2:		break ;
	      case V4SS_Type_URLBase3X:		break ;
	      case V4SS_Type_URLBase3:		break ;
	      case V4SS_Type_URLBase4X:		break ;
	      case V4SS_Type_URLBase4:		break ;
	      case V4SS_Type_URLBase5X:		break ;
	      case V4SS_Type_URLBase5:		break ;
	      case V4SS_Type_URLLink:		ri->CurURLx = 1 ; goto urllink ;
	      case V4SS_Type_URLLink2:		ri->CurURLx = 2 ; goto urllink ;
	      case V4SS_Type_URLLink3:		ri->CurURLx = 3 ; goto urllink ;
	      case V4SS_Type_URLLink4:		ri->CurURLx = 4 ; goto urllink ;
	      case V4SS_Type_URLLink5:		ri->CurURLx = 5 ; goto urllink ;
	      case V4SS_Type_URLLinkX:		
urllink:	for(i=0,j=0;;i++)				/* Take care of any embedded double-quotes */
		 { if (buf[i] == '"' && (i == 0 ? TRUE : buf[i-1] != '\\')) { t1[j++] = '\\' ; }
		   if (buf[i] == '\\' && buf[i+1] != '\\') { t1[j++] = '\\' ; }
		   t1[j++] = buf[i] ;
		   if (buf[i] == '\0') break ;
		 } ;
		strcat(ri->CurURL,t1) ;
		if (isExample) strcpy(ri->CurURL,"+") ;
		break ;
	      case V4SS_Type_HTML:		if (ri->HTMLCount >= RI_HTMLMax)
						 { rth_DisplayProblem("Exceeded max embedded HTML expressions") ; break ; } ;
						strcat(ri->HTML[ri->HTMLCount].Buf,buf) ; ri->HTMLCount++ ; break ;
	      case V4SS_Type_HTMLX:		if (ri->HTMLCount < RI_HTMLMax) strcat(ri->HTML[ri->HTMLCount].Buf,buf) ; break ;
	      case V4SS_Type_SSNotDim:		ssdim = -buf[0] ; break ;
	      case V4SS_Type_SSDim:		ssdim = buf[0] ; break ;
	      case V4SS_Type_FormatSpecNP:
		memcpy(&vfsnp,buf,sizeof vfsnp) ; gotfnp = TRUE ; break ;
	      case V4SS_Type_FormatSpec:
#define GetVFS(entry) if (ri->entry == NULL) ri->entry = (struct V4SS__FormatSpec *)v4mm_AllocChunk(sizeof *vfs,TRUE) ; memcpy(ri->entry,buf,sizeof *vfs) ; break ;
		switch (ssdim)			/* Check if ssdim is a 'special dimension' */
		 {
		   case V4SS_DimId_Table:	GetVFS(vfsTable) ;
		   case	V4SS_DimId_Row:		GetVFS(vfsRow) ;
		   case V4SS_DimId_Cell:	GetVFS(vfsCell) ;
		   case V4SS_DimId_HdrRow:	GetVFS(vfsHdrRow) ;
		   case V4SS_DimId_HdrCell:	GetVFS(vfsHdrCell) ;
		   case V4SS_DimId_TopOfPage:	GetVFS(vfsTopOfPage) ;
		 } ; if (ssdim < 0) break ;
		if (vdfm == NULL)			/* Allocate structure if not already done */
		 { vdfm = (struct V4SS__DimToFormatMap *)v4mm_AllocChunk(sizeof *vdfm,TRUE) ; } ;
		if (ssdim != UNUSED && ssdim < V4SS__DimToFormat_Max)
		 { if (vdfm->vfs[ssdim] == NULL) vdfm->vfs[ssdim] = (struct V4SS__FormatSpec *)v4mm_AllocChunk(sizeof *vfs,TRUE) ;
		   vfs = vdfm->vfs[ssdim] ; memcpy(vfs,buf,sizeof *vfs) ;
//		   if (vfs->FontSize == 0) vfs->FontSize = RI_DefaultFontSize ;		/* Default for now */
		 } ;
		ssdim = UNUSED ;
		vdfm->GlobalFontSize = ri->GlobalFontSize ; strcpy(vdfm->GlobalFont,ri->GlobalFont) ;
	    } ;
	 } ;

	if (ri->DivCount > 0)			/* Have a division? - then end it */
	 { rth_Output(outbuf,"</table></div>\n",ofp) ; }
	 else { rth_Output(outbuf,"</table>\n",ofp) ; } ;
	if (compress) rth_Output(outbuf,NULL,ofp) ;
	fputs("</div>\n",ofp) ;
	curtime = time(NULL) ;
//	fputs("<p style='font-size: smaller;'>\n",ofp) ;
	fputs("<table border='0' style='padding:0;'>\n",ofp) ;
	fputs("<tr><td style='font-size:smaller;'>",ofp) ;
	if (linktomks)
	 { fprintf(ofp,"%s - <a href=\"http://www.mksinc.com\">MKS</a> V4 v%d.%d / RTH v%d.%d<br/>",ri->Id,
		V4IS_MajorVersion,V4IS_MinorVersion,RTH_MajorVersion,RTH_MinorVersion) ;
	 } else
	 { fprintf(ofp,"%s - MKS V4 v%d.%d / RTH v%d.%d<br/>",ri->Id,
		V4IS_MajorVersion,V4IS_MinorVersion,RTH_MajorVersion,RTH_MinorVersion) ;
	 } ;
	if (jobno1 == UNUSED || jobno2 == UNUSED || leKey == UNUSED || cgiserver == NULL)
	 { fprintf(ofp,"%s (%s)",asctime(localtime(&curtime)),UCretASC(tranid)) ;
	 } else
	 { if (excelLogo == NULL)
	    { fprintf(ofp,"%s (<a href=\"%s?_V4=ResToXL&_JobNo1=%d&_JobNo2=%d&_leKey=%d\">%s)</a>",
		asctime(localtime(&curtime)),UCretASC(cgiserver),jobno1,jobno2,leKey,UCretASC(tranid)) ;
	    } else
	    { fprintf(ofp,"%s (%s)&nbsp;",asctime(localtime(&curtime)),UCretASC(tranid)) ;
	    } ;
	 } ;
	fputs("</td>",ofp) ;
	if (jobno1 != UNUSED && jobno2 != UNUSED && leKey != UNUSED && cgiserver != NULL && excelLogo != NULL)
	 { fprintf(ofp,"<td><a href='%s?_V4=ResToXL&_JobNo1=%d&_JobNo2=%d&_leKey=%d'><img border='0' src='%s'/></a></td>",UCretASC(cgiserver),jobno1,jobno2,leKey,UCretASC(excelLogo)) ;
	 } ;
	fputs("</tr>",ofp) ;
	fputs("</table>\n",ofp) ;
//	fputs("</p>\n",ofp) ;
	fputs("</body>\n",ofp) ;
	fputs("</html>\n",ofp) ;

	fclose(fp) ;
	fclose(ofp) ;				/* Close the output file */
/*	Should we make a copy/duplicate of the file ? */
	if (UCstrlen(dupfilename) > 0)
	 { int dup ;
	   fp = UCfopen(ofile,"r") ; ofp = UCfopen(dupfilename,"w") ;
	   if (fp == NULL) { v_Msg(NULL,errbuf,"VSrtInpErr",ifile,errno) ; vout_UCText(VOUT_Err,0,errbuf) ; exit(EXITABORT) ; } ;
	   if (ofp == NULL) { v_Msg(NULL,errbuf,"VSrtOutFile",dupfilename,errno) ; vout_UCText(VOUT_Err,0,errbuf) ; exit(EXITABORT) ; } ;
	   if (TRACE) { v_Msg(NULL,errbuf,"VResHTMLTrace7",dupfilename) ; vout_UCText(VOUT_Trace,0,errbuf) ; } ;
	   if (fp != NULL && ofp != NULL)
	    { for(dup=TRUE;;)
	       { if (fgets(pbuf,sizeof pbuf,fp) == NULL) break ;
	         if (strstr(pbuf,"V__STOP_DUPLICATE__V") != NULL) { dup = FALSE ; continue ; } ;
	         if (strstr(pbuf,"V__START_DUPLICATE__V") != NULL) { dup = TRUE ; continue ; } ;
	         if (dup) fputs(pbuf,ofp) ;
	       } ;
	      fclose(fp) ; fclose(ofp) ;
	    } ;
	 } ;
}

/*	rth_OutputJavaScriptUtils - Dumps out some java utilities */

void rth_OutputJavaScriptUtils(isExample)
  int isExample ;
{
  int i ;

	fputs("<script language='javascript'>\n",ofp) ;

	for(i=0;i<V4SS_URLBaseMax;i++)
	 { if (strlen(ri->URLBase[i]) == 0) continue ;
	   fprintf(ofp,"function URL%d(suffix)\n",i+1) ;
	   if (isExample)
	    { fprintf(ofp,"{  alert('This is an example report - links are disabled!') ; return false ; } ;\n") ; }
	    else { fprintf(ofp,"{  window.top.location.href = '%s' + suffix ; }\n",ri->URLBase[i]) ; } ;
	 } ;
	fputs("function Expand(src)\n",ofp) ;
	fputs("{ var dst = '' ; var controlbits = 0 ;\n",ofp) ;
	fputs("  if (src.charCodeAt(0) == 1) { document.write(src.substr(4,src.length()-4)) ; return ; } ;\n",ofp) ;
	fputs("  px = 4 ; ex = src.length ;\n",ofp) ;
	fputs("  while (px < ex)\n",ofp) ;
	fputs("   { if (controlbits == 0)\n",ofp) ;
	fputs("      { control = src.charCodeAt(px++) ;  control |= (src.charCodeAt(px++) << 8) ; controlbits = 16 ; } ;\n",ofp) ;
	fputs("     if (control & 1)\n",ofp) ;
	fputs("      { offset = ((src.charCodeAt(px) & 0xF0) << 4) ; len = 1 + (src.charCodeAt(px++) & 0xF) ;\n",ofp) ;
	fputs("       offset += (src.charCodeAt(px++) & 0xFF) ; p = dst.length - offset ;\n",ofp) ;
	fputs("	while(len--) { dst = dst + dst.charAt(p++) ; } ;\n",ofp) ;
	fputs("      } else\n",ofp) ;
	fputs("      { dst = dst + src.charAt(px++) ; } ;\n",ofp) ;
	fputs("     control >>= 1 ; controlbits-- ;\n",ofp) ;
	fputs("   } ;\n",ofp) ;
	fputs("  document.write(dst) ;\n",ofp) ;
	fputs("}\n",ofp) ;
	fputs("</script>\n",ofp) ;
}



/*	rth_FormatStr - Formats string by checking for embedded color, style, etc. */

char *rth_FormatStr(vdfm,istr)
  struct V4SS__DimToFormatMap *vdfm ;
  char *istr ;
{ struct V4SS__FormatSpec *vfs ;
  static char res[25000],*b ; int i ;
  char color[24] ; int italic,font,bold,nobr ;

  	if ((b=strchr(istr,'\001')) == NULL || vdfm == NULL) return(istr) ;	/* No embedded formatting - return as is */
	ZS(res) ; ZS(color) ; font = FALSE ; italic = FALSE ; bold = FALSE ; nobr = FALSE ;
	for(;;)
	 { if (b != NULL) *b = '\0' ; strcat(res,istr) ;	/* Copy up to first format */
	   if (font) strcat(res,"</font>") ;
	   if (nobr) strcat(res,"</nobr>") ;
	   if (italic) strcat(res,"</i>") ;
	   if (bold) strcat(res,"</b>") ;
	   if (b == NULL) break ;
	   if (*(b+1) == 1)						/* 1 = no formatting */
	    { strcat(res,istr) ; istr = b + 2 ; b = strchr(istr,'\001') ;
	      continue ; 
	    }  ;
/*	   Here to substitute formatting */
	   ZS(color) ; italic = FALSE ; font = FALSE ; bold = FALSE ;
	   i = *(b+1) - 2 ;
	   if (i >= V4SS__DimToFormat_Max) break ;	/* Huh?? */
	   vfs = vdfm->vfs[i] ;
	   if (vfs == NULL) break ;			/* Huh?? */
	   if (vfs->FontColor != 0) { sprintf(color,"color: %s; ",UCretASC(v_ColorRefToHTML(vfs->FontColor))) ; font = TRUE ; } ;
	   if (vfs->FontBold > V4SS_FontBold_Normal) { strcat(res,"<b>") ; bold = TRUE ; } ;
	   if (vfs->FontAttr & V4SS_Font_Italic) { strcat(res,"<i>") ; italic = TRUE ; } ;
	   if (vfs->FontAttr & V4SS_Font_NoBreak) { strcat(res,"<nobr>") ; nobr = TRUE ; } ;
	   if (font)
	    { strcat(res,"<font style='") ;
	        strcat(res,color) ;
	      strcat(res,"'>") ;
	    } ;
	   istr = b + 2 ; b = strchr(istr,'\001') ;
	 } ;
	return(res) ;
}



/*	rth_Output - Outputs string to htm file OR appends to temp buf for compaction */

void rth_Output(outbuf,ostr,ofp)
 char *outbuf ;
 char *ostr ;
 FILE *ofp ;
{ static int isInit=FALSE ;
  char cbuf[OUTBUFMAX+10], jsout[OUTBUFMAX*3] ; unsigned char *p1,*p2 ;
  int clen,oslen ;

	if (!compress) { fputs(ostr,ofp) ; return ; } ;

	if (ostr == NULL ? TRUE : (strlen(outbuf) + (oslen = strlen(ostr)) > OUTBUFMAX - 10))
	 { if (!isInit) { isInit = TRUE ; fputs("<script language=Javascript>\n",ofp) ; } ;
	   clen = OUTBUFMAX ;
	   lzrw1_compress(outbuf,strlen(outbuf),cbuf,&clen) ;
	   strcpy(jsout,"Expand(\"") ;
	   for(p1=cbuf,p2=&jsout[strlen(jsout)];p1-cbuf < clen;p1++)
	    { if (*p1 > 127)
	       { *(p2++) = '\\' ; *(p2++) = '0' + (*p1 >> 6) ; *(p2++) = '0' + ((*p1 >> 3) & 0x7) ; *(p2++) = '0' + (*p1 & 0x7) ; continue ; } ;
	      if (*p1 == 26)
	       { *(p2++) = '\\' ; *(p2++) = '0' + (*p1 >> 6) ; *(p2++) = '0' + ((*p1 >> 3) & 0x7) ; *(p2++) = '0' + (*p1 & 0x7) ; continue ; } ;
	      switch(*p1)
	       { default:	*(p2++) = *p1 ; break ;
	         case 0:	if (*(p1+1) < '0' || *(p1+1) > '9') { *(p2++) = '\\' ; *(p2++) = '0' ; }
				 else { *(p2++) = '\\' ; *(p2++) = '0' ; *(p2++) = '0' ; *(p2++) = '0' ; } ;
				break ; 
		 case '"':	*(p2++) = '\\' ; *(p2++) = '"' ; break ;
		 case '\n':	*(p2++) = '\\' ; *(p2++) = 'n' ; break ;
		 case '\r':	*(p2++) = '\\' ; *(p2++) = 'r' ; break ;
		 case '\\':	*(p2++) = '\\' ; *(p2++) = '\\' ; break ;
		 case '\f':	*(p2++) = '\\' ; *(p2++) = 'f' ; break ;
	       } ;
	    } ;
	   *(p2++) = '\0' ;
	   fputs(jsout,ofp) ; fputs("\") ;\n",ofp) ; ZS(outbuf) ;
	 } ;
	if (ostr != NULL)
	 { if (oslen >= OUTBUFMAX)		/* String to output is bigger than buffer ? */
	    { char save ; int start ;
	      for(start=0;start<oslen;)
	       { save = ostr[start+OUTBUFMAX] ; ostr[start+OUTBUFMAX] = '\0' ;
	         rth_Output(outbuf,&ostr[start],ofp) ;
		 ostr[start+OUTBUFMAX] = save ; start += OUTBUFMAX ;
	       } ;
	    } else { strcat(outbuf,ostr) ; } ;
	 } else { fputs("</script>\n",ofp) ; } ;
	return ;
}

/*	rth_CheckEmpty - Checks number of empty cells and outputs necessary <td></td> to skip over */
void rth_CheckEmpty(emptycnt,ofp,outbuf)
  int *emptycnt ;
  FILE *ofp ;
  char *outbuf ;
{ char tstr[128] ;
  int i ;
  
  	if (*emptycnt <= 2)			/* Faster to just output <td></td> pairs */
	 { for(i=0;i<*emptycnt;i++) { rth_Output(outbuf,tdmtstr,ofp) ; } ;
	   *emptycnt = 0 ; return ;
	 } ;
	sprintf(tstr,tdcolmtstr,*emptycnt) ; rth_Output(outbuf,tstr,ofp) ;
	*emptycnt = 0 ;
	return ;
}

/*	rth_FormatInt - Formats integer via Excel format string (e.g. "#,##0") */

void rth_FormatInt(src,format,fldColor)
  char *src, *format, *fldColor ;
{ char *b ; int num ;


	if (strlen(format) == 0) return ;
	num = strtol(src,&b,10)	 ;			/* Convert to integer */
	if (*b != '\0') return ;			/* Not valid number - can't convert */
//	v_FormatInt(num,format,src) ;
	{ UCCHAR ucbuf[100], color[64] ; v_FormatInt(num,ASCretUC(format),ucbuf,color) ; UCstrcpyToASC(src,ucbuf) ; UCstrcpyToASC(fldColor,color) ; }
	return ;
}

/*	rth_FormatDate - Formats Date via Excel format string (e.g. "#,##0") */

void rth_FormatDate(src,format)
  char *src, *format ;
{ int udate ; UCCHAR ucmask[100],ucbuf[100],*ucformat ; 
	
	udate = strtol(src,NULL,10) ;  if (format != NULL) { UCstrcpyAtoU(ucmask,format) ; ucformat = ucmask ; } else { ucformat = NULL ; } ;
	if (udate == 0) { strcpy(src,"") ; }
	 else { v_FormatDate(&udate,V4DPI_PntType_UDate,VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,ucformat,ucbuf) ; UCstrcpyToASC(src,ucbuf) ; } ;
}

/*	rth_FormatDbl - Formats double (real) via Excel format string (e.g. "#,##0") */
/*	NOTE: format string may be updated!!!! */

void rth_FormatDbl(src,format,fldColor)
  char *src, *format,*fldColor ;
{ char *b,*b1,*delim,*sign,stack[100],nsrc[64] ;
  double dnum ; int i,j,s,t,num,zeros,neg,dps ;

	if (strlen(format) == 0) return ;
	dnum = strtod(src,NULL) ;
//	v_FormatDbl(dnum,format,src) ;
	{ UCCHAR ucbuf[100], color[64] ; v_FormatDbl(dnum,ASCretUC(format),ucbuf,color) ;
	  UCstrcpyToASC(src,ucbuf) ; UCstrcpyToASC(fldColor,color) ;
	}
	return ;
}

/*	rth_CSS - Sets up for cascading style sheet		*/

char *rth_CSS(vbuf,vdfm,ssdim,vfsnp,rowcolornum,colcolornum,cellcolornum,dfltjustify,iscap,cselect,nobr)
  char *vbuf ;					/* If defined then contents of output value (used in URLLink) */
  struct V4SS__DimToFormatMap *vdfm ;
  struct V4SS__FormatSpec *vfsnp ;		/* If defined then overrides other attributes */
  int ssdim,rowcolornum,colcolornum,cellcolornum,dfltjustify,iscap ;
  int *cselect,*nobr ;
{ struct V4SS__FormatSpec *vfs,vfsDflt ;
  static char sbuf[8192] ; char tb[8192],tb2[8192] ; int color,newfontsize,i,j,ha ;

	ZS(sbuf) ; *cselect = FALSE ; *nobr = FALSE ;
	if (vdfm == NULL || ssdim == UNUSED)		/* No special formatting yet? */
	 { if (vfsnp == NULL ? FALSE : vfsnp->FgColor != 0) { color = vfsnp->FgColor ; }
	    else { color = (cellcolornum != UNUSED ? cellcolornum : (rowcolornum != UNUSED ? rowcolornum : (colcolornum != UNUSED ? colcolornum : 0))) ;
		 } ;
	   if (color != 0) { sprintf(tb,"background-color: %s; ",UCretASC(v_ColorRefToHTML(color))) ; strcat(sbuf,tb) ; } ;
	   if (vdfm == NULL ? FALSE : vdfm->GlobalFontSize != UNUSED)
	    { newfontsize = vdfm->GlobalFontSize ; } else { newfontsize = RI_DefaultFontSize ; } ;
//	   newfontsize = (vfsnp == NULL ? RI_DefaultFontSize : (vfsnp->FontSize == 0 ? RI_DefaultFontSize : vfsnp->FontSize)) ;
	   if (vfsnp != NULL ? vfsnp->FontSize != 0 : FALSE) newfontsize = vfsnp->FontSize ;
/*	   Check to see if in caption - but only if have not got explicit style info for header row/cell! */
	   if (iscap && ri->vfsHdrRow == NULL && ri->vfsHdrCell == NULL)
	    { if (color == 0) { sprintf(tb,"background-color: %s; ",RI_DefaultBGColor) ; strcat(sbuf,tb) ; } ;
	      sprintf(tb," font-family: '%s'; ",RI_DefaultHeaderFont) ; strcat(sbuf,tb) ;
//	      newfontsize += 2 ;
	    } else
	    { if (vdfm != NULL ? strlen(vdfm->GlobalFont) > 0 : FALSE)
	       { sprintf(tb," font-family: '%s'; ",vdfm->GlobalFont) ; strcat(sbuf,tb) ; } ;
	    } ;
	   sprintf(tb,"text-align: %s; ",JustifyStr[dfltjustify]) ; strcat(sbuf,tb) ;
	   if (newfontsize > 0) { sprintf(tb,"font-size: %dpt; ",newfontsize) ; strcat(sbuf,tb) ; }
	    else if (newfontsize < 0) { sprintf(tb,"font-size: %d%%;",-newfontsize) ; strcat(sbuf,tb) ; } ;
	   goto add_if_new ;
	 } ;
	vfs = vdfm->vfs[ssdim] ;
	if (vfs == NULL) { memset(&vfsDflt,0,sizeof vfsDflt) ; vfs = &vfsDflt ; } ;

/*	If we got a URL link then set ri->CurURL (unless it has already been locally overridden with another value) */
	ZS(tb) ;
	if ((vfsnp != NULL ? vfsnp->URLLink != 0 : FALSE) && strlen(ri->CurURL) == 0)
	 { strcpy(tb,&vfsnp->VarString[vfsnp->URLLink-1]) ; }
	 else if (vfs->URLLink != 0 && strlen(ri->CurURL) == 0) { strcpy(tb,&vfs->VarString[vfs->URLLink-1]) ; } ;
	if (strlen(tb) > 0 && vbuf != NULL)
	 { for(i=0,j=0;;i++)
	    { ri->CurURL[j++] = tb[i] ; if (tb[i] == '\0') break ;
	      if (tb[i] != '*') continue ;
	      strcpy(&ri->CurURL[j-1],vbuf) ; j += (strlen(vbuf) - 1) ;
	    } ;
	 } ;


/*	If we got a CSS Id or Class name then return it immediately */
	if (vfs != NULL ? vfs->CSSId > 0 : FALSE)
	 { strcpy(sbuf,"id=") ; strcat(sbuf,&vfs->VarString[vfs->CSSId-1]) ; return(sbuf) ; } ;
	if (vfs != NULL ? vfs->CSSClass > 0 : FALSE)
	 { strcpy(sbuf,"class=") ; strcat(sbuf,&vfs->VarString[vfs->CSSClass-1]) ; return(sbuf) ; } ;
/*	Start with any explicit style-sheet data */
	if (vfsnp != NULL ? vfsnp->CSSStyle > 0 : FALSE) strcat(sbuf,&vfsnp->VarString[vfsnp->CSSStyle-1]) ;
	if (vfs != NULL ? vfs->CSSStyle > 0 : FALSE) strcat(sbuf,&vfs->VarString[vfs->CSSStyle-1]) ;
/*	If this format specifies a width then set it */
	if (vfs == NULL ? FALSE : vfs->Width != 0) ri->Col[ri->CurCol].ColWidth = vfs->Width ;
/*	Determine font color & bgcolor */
	if (vfsnp == NULL ? FALSE : vfsnp->FgColor != 0) { color = vfsnp->FgColor ; }
	 else { color = (vfs->FgColor != 0 ? vfs->FgColor :
		  (cellcolornum != UNUSED ? cellcolornum : (rowcolornum != UNUSED ? rowcolornum : (colcolornum != UNUSED ? colcolornum : 0)))) ;
	      } ;
	if (color == 0 && iscap) { sprintf(tb,"background-color: %s; ",RI_DefaultBGColor) ; strcat(sbuf,tb) ; } ;
	if (color != 0) { sprintf(tb,"background-color: %s; ",UCretASC(v_ColorRefToHTML(color))) ; strcat(sbuf,tb) ; } ;
	if (vfsnp == NULL ? FALSE : vfsnp->FontColor != 0)
	  { sprintf(tb,"color: %s; ",UCretASC(v_ColorRefToHTML(vfsnp->FontColor))) ; strcat(sbuf,tb) ; }
	  else if (vfs->FontColor != 0) { sprintf(tb,"color: %s; ",UCretASC(v_ColorRefToHTML(vfs->FontColor))) ; strcat(sbuf,tb) ; } ;
/*	Now determine formatting characteristics */
	newfontsize = (vfsnp == NULL ? vfs->FontSize : (vfsnp->FontSize == 0 ? vfs->FontSize : vfsnp->FontSize)) ;
	if (newfontsize == 0) newfontsize = (vdfm->GlobalFontSize == UNUSED ? RI_DefaultFontSize : vdfm->GlobalFontSize)  ;
	if (vfsnp == NULL ? FALSE : vfsnp->FontBold > V4SS_FontBold_Normal)
	 { strcat(sbuf,"font-weight: bold; ") ; /* if (newfontsize == RI_DefaultFontSize) newfontsize += 2 ; */ }
	 else if (vfs->FontBold > V4SS_FontBold_Normal)
	       { strcat(sbuf,"font-weight: bold; ") ; /* if (newfontsize == RI_DefaultFontSize) newfontsize += 2 ; */ } ;
	if ((vfs->FontAttr | (vfsnp == NULL ? 0 : vfsnp->FontAttr)) & V4SS_Font_Italic)
	  strcat(sbuf,"font-style: italic; ") ;
	if ((vfs->FontAttr | (vfsnp == NULL ? 0 : vfsnp->FontAttr)) & V4SS_Font_StrikeThru) strcat(sbuf,"text-decoration: line-through; ") ;
	if ((vfs->FontAttr | (vfsnp == NULL ? 0 : vfsnp->FontAttr)) & V4SS_Font_NoBreak) *nobr = TRUE ;
	if ((vfs->FontULStyle | (vfsnp == NULL ? 0 : vfsnp->FontULStyle)) != 0) strcat(sbuf,"text-decoration: underline; ") ;
	ha = (vfsnp == NULL ? vfs->HAlign : (vfsnp->HAlign != 0 ? vfsnp->HAlign : vfs->HAlign)) ;
	if (ha == V4SS_Align_MultiColCenter) { *cselect = (ha = RI_JustifyCenter) ; } ;
	if (ha == V4SS_Align_MultiColLeft) { *cselect = (ha = RI_JustifyLeft) ; } ;
	if (ha == V4SS_Align_MultiColRight) { *cselect = (ha = RI_JustifyRight) ; } ;
	if (vfsnp != NULL ? vfsnp->FontNameX != 0 : FALSE)
	 { strcpy(tb2,&vfsnp->VarString[vfsnp->FontNameX-1]) ;
	   if (strcmp(tb2,"HEADING") == 0) strcpy(tb2,RI_DefaultHeaderFont) ;
	   sprintf(tb," font-family: '%s'; ",tb2) ; strcat(sbuf,tb) ;
	 } else if (vfs->FontNameX != 0)		/* Get font - if HEADING then default to column heading font */
	 { strcpy(tb2,&vfs->VarString[vfs->FontNameX-1]) ;
	   if (strcmp(tb2,"HEADING") == 0) strcpy(tb2,RI_DefaultHeaderFont) ;
	   sprintf(tb," font-family: '%s'; ",tb2) ; strcat(sbuf,tb) ;
	 } else if (iscap)
	 { sprintf(tb," font-family: '%s'; ",(strlen(vdfm->GlobalFont) > 0 ? vdfm->GlobalFont : RI_DefaultHeaderFont)) ; strcat(sbuf,tb) ;
	 } else if (strlen(vdfm->GlobalFont) > 0)
	 { sprintf(tb," font-family: '%s'; ",vdfm->GlobalFont) ; strcat(sbuf,tb) ;
	 } ;

	switch (ha)
	 { default:			break ;
	   case V4SS_Align_Left:	dfltjustify = RI_JustifyLeft ; break ;
	   case V4SS_Align_Right:	dfltjustify = RI_JustifyRight ; break ;
	   case V4SS_Align_Center:	dfltjustify = RI_JustifyCenter ; break ;
	 } ; sprintf(tb,"text-align: %s; ",JustifyStr[dfltjustify]) ; strcat(sbuf,tb) ;
	if (newfontsize > 0) { sprintf(tb,"font-size: %dpt; ",newfontsize) ; strcat(sbuf,tb) ; }
	 else if (newfontsize < 0) { sprintf(tb,"font-size: %d%%;",-newfontsize) ; strcat(sbuf,tb) ; } ;
add_if_new:
	if (rcl == NULL)
	 { rcl = (struct RTH__CSSList *)v4mm_AllocChunk(sizeof *rcl,FALSE) ; rcl->Count = 0 ;
	   rcl->InitialCount = UNUSED ;
	   strcpy(rcl->CSS[rcl->Count].CSSName,"") ;		/* Set up default for <td> */
	   sprintf(rcl->CSS[rcl->Count].CSSValue,"text-align: right;") ;
	   rcl->Count++ ;
	 } ;
	if (strcmp(sbuf,defaulttd) == 0) return("") ;	/* If default style then return empty string */
	for(i=0;i<rcl->Count;i++) { if (strcmp(sbuf,rcl->CSS[i].CSSValue) == 0) break ; } ;
	rcl->CSS[i].Usage ++ ;				/* Increment usage count */
	if (i < rcl->Count)
	 { strcpy(sbuf,"id=") ; strcat(sbuf,rcl->CSS[i].CSSName) ; return(sbuf) ; } ;
/*	Have to add a new one */
	if (rcl->Count >= LCL_STYLE_CSSMax)
	 { printf("? Exceeded max number of styles\n") ; return("") ; } ;
	if (rcl->Count < 26) { sprintf(rcl->CSS[rcl->Count].CSSName,"%c",('A' + rcl->Count)) ; }
	 else { sprintf(rcl->CSS[rcl->Count].CSSName,"S%d",rcl->Count) ; } ;
	strcpy(rcl->CSS[rcl->Count].CSSValue,sbuf) ;
	if (rcl->Count >= rcl->InitialCount && rcl->InitialCount != UNUSED)
	 { rth_Output(outbuf,"<style>\n<!--\n",ofp) ;
	   sprintf(sbuf,"#%s { %s}\n",rcl->CSS[rcl->Count].CSSName,rcl->CSS[rcl->Count].CSSValue) ;
	   rth_Output(outbuf,sbuf,ofp) ;
	   rth_Output(outbuf,"-->\n</style>\n",ofp) ;
	   rcl->InitialCount = rcl->Count+1 ;
	 } ;
	strcpy(sbuf,"id=") ; strcat(sbuf,rcl->CSS[rcl->Count].CSSName) ;
	rcl->CSS[rcl->Count].Usage = 1 ;
	rcl->Count++ ; return(sbuf) ;
}

/*	rth_GetColors - Get cell & font colors associated with current format */
/*	Note: rowcolornum, colcolornum, & cellcolornum can be overridden by specific dimension formats */

void rth_GetColors(cellcolor,fontcolor,vfs,rowcolornum,colcolornum,cellcolornum)
  char *cellcolor, *fontcolor ;
  struct V4SS__FormatSpec *vfs ;
  int rowcolornum,colcolornum,cellcolornum ;
{ int color ;

	if (vfs->FontColor != 0) sprintf(fontcolor,"color='%s'",UCretASC(v_ColorRefToHTML(vfs->FontColor))) ;
	color = (vfs->FgColor != 0 ? vfs->FgColor :
		  (cellcolornum != UNUSED ? cellcolornum : (rowcolornum != UNUSED ? rowcolornum : (colcolornum != UNUSED ? colcolornum : 0)))) ;
	if (color != 0) sprintf(cellcolor,"bgcolor='%s'",UCretASC(v_ColorRefToHTML(vfs->FgColor))) ;
}

/*	rth_DisplayProblem - Output a problem via Javascript */

void rth_DisplayProblem(str)
 char *str ;
{ char tbuf[8192] ; int i,j ;

	for(i=0,j=0;;)					/* Go thru string - maybe convert "\" to "\\" */
	 { if (str[j] == '\'') tbuf[i++] = '\\' ;	/* If got a single quote then preface with "\" */
	   tbuf[i] = str[j] ; if (str[j++] == '\0') break ;
	   if (tbuf[i++] != '\\') continue ;
	   if (str[j] == '\\') continue ;		/* Look like already got "\\" - leave it */
	   tbuf[i++] = '\\' ;				/* Add an extra "\" so it prints OK */
	 } ;
	if (ofp != NULL) fprintf(ofp,"<script language='Javascript'>alert('%s');</script>\n",tbuf) ;
}




