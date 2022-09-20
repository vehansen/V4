#include "v4defs.c"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef ALPHAOSF
#include <sys/sysinfo.h>
#include <sys/proc.h>
#endif

#include "vexceldefs.c"

#define RTH_MajorVersion 1
#define RTH_MinorVersion 4

#define UNUSED -1

struct V4DPI__LittlePoint protoInt, protoDbl, protoFix, protoDict, protoAlpha, protoLog, protoForeign, protoNone, protoUCChar ;

/*	T O   C O M P I L E   O N   L I N U X
	cc -o xvrestobiff -pthread -w  -DINCLUDE_VCOMMON -DVSORT_AS_SUBROUTINE v4atomiclinux486.s vrestobiff.c vexcel.c vcommon.c -lm -lc
*/

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
{ int ax,sval,towww,i ; UCCHAR *arg,*ifile,*ofile,*tranid ; UCCHAR infile[V_FileName_Max], outfile[V_FileName_Max] ;
#define UCARGVMAX 50
  static UCCHAR *ucargv[UCARGVMAX] ;		/* Temp (hopefully) structure converting command line to Unicode */
 struct V4C__Context ctx ;
#ifdef ALPHAOSF
  int uacbuf[2] ;
#endif
	

	startup_envp = envp ;

#ifdef WINNT
  for(i=0;argv[i]!=NULL&&i<UCARGVMAX-1;i++) { ucargv[i] = argv[i] ; } ; ucargv[i] = NULL ;
#else
{ /* Convert arguments to Unicode */
  UCCHAR *uarg ; char *arg ; int j ;
  for(i=0;argv[i]!=NULL&&i<UCARGVMAX-1;i++)
   { int num = strlen(arg=argv[i]) ;
     uarg = (ucargv[i] = v4mm_AllocChunk((num+1)*sizeof(UCCHAR),FALSE)) ;
     for(j=0;;j++) { uarg[j] = arg[j] ; if (arg[j] == '\0') break ; } ;
   } ; ucargv[i] = NULL ;
}
#endif

#ifdef ALPHAOSF
	uacbuf[0] = SSIN_UACPROC ; uacbuf[1] = UAC_NOPRINT ;
	setsysinfo(SSI_NVPAIRS,&uacbuf,1,0,0) ;
#endif
	ifile = NULL ; ofile = NULL ; tranid = NULL ; towww = FALSE ;

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
		 case UClit('t'):	tranid = argv[++ax] ; break ;	/* Transaction Id */
		 case UClit('w'):	towww = TRUE ; break ;		/* Going back to WWW client */
	    } ;
	 } ;
	if (ifile != NULL && ofile == NULL && UCstrchr(ifile,'.') == NULL)	/* Got single file, no extension */
	 { UCstrcpy(infile,ifile) ; UCstrcat(infile,UClit(".v4r")) ;
	   UCstrcpy(outfile,ifile) ; UCstrcat(outfile,UClit(".xls")) ;
	   ifile = infile ; ofile = outfile ;
	 } ;
	if (ifile == NULL) { printf("? Input file name missing\n") ; exit(EXITABORT) ; } ;
	if (ofile == NULL ) { printf("? Output file name missing\n") ; exit(EXITABORT) ; } ;

	ctx.pi = v_GetProcessInfo() ;
	ResToExcel(UCretASC(ifile),UCretASC(ofile),tranid,towww) ;
	exit(EXITOK) ;
}


#define RI_ColMax 256
#define RI_JustifyLeft 0
#define RI_JustifyRight 1
#define RI_JustifyCenter 2

#define RI_DefaultBGColor "#C0C0C0"

char JustifyStr[3][12] = { "left", "right", "center" } ;

struct RTH__ReportInfo {
  char Title1[512] ;			/* First title line */
  char Id[64] ;				/* Report ID */
  char ErrMsg[512] ;			/* If an error then this holds the message - return JavaScript */
  char CurFormat[32] ;			/* Format of "next" column */
  char CurStyle[64] ;			/* Style of "next" column */
  char CurURL[4096] ;			/* URL associated with "next" column */
  int RowCount ;			/* Number of rows */
  int CurRow ;				/* Current row */
  int CurCol ;
  int ColCapRowCount ;			/* Number of rows in column captions */
  int ColCount ;			/* Number of Columns */
  struct {
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

ResToExcel(ifile,ofile,tranid,towww)
  char *ifile,*ofile,*tranid ;
  int towww ;
{ 
  struct ss__WorkBook *wb ;
  struct ss__WorkSheet *ws ;
  struct V4SS__FormatSpec vfs ;
  struct ss__Format *dateformat = NULL ;
  struct ss__Format *defaultformat = NULL ;
  struct ss__Format *format ;
  struct ss__Format tformat ;
  struct ss__Format formatnp ; int gotfnp ;
  FILE *fp ; char FileName[V_FileName_Max] ;
  struct stat statbuf ;
  unsigned char type ;
  int i,ok,len,sig,justify,returnv4r,bytesread,num,checkmt,bytes ; double dnum,maxdnum ; 
  unsigned char buf[25000],tbuf[25000] ; 
  char *b1, *p, *p1, *p2 ; int fix ;
  short widths[RI_ColMax] ;
  struct RTH__ReportInfo *ri ;
  int ssdim,gridlines ;
  struct ss__Format *ssdimformats[V4SS__DimToFormat_Max] ;
#define HTTP_HeaderMax 50
  char *HTTP_Other[HTTP_HeaderMax] ; int HTTP_Other_Count ;

	fp = fopen(ifile,"rb") ;
	if (fp == NULL) { printf("? Could not access (%s) input file (%s)\n",v_OSErrString(errno),ifile) ; exit(EXITABORT) ; } ;

/*	Init report info structure (ri) & set up defaults */
	ri = (struct RTH__ReportInfo *)malloc(sizeof *ri) ; memset(ri,0,sizeof *ri) ;

/*	Make pass thru file to determine rows & columns, pull out headers, titles, etc. */
	ri->RowCount = 0 ; bytesread = 0 ; gridlines = UNUSED ;
	ssdim = UNUSED ; for(i=0;i<V4SS__DimToFormat_Max;i++) { ssdimformats[i] = NULL ; } ;
	for(i=0;i<RI_ColMax;i++) { widths[i] = UNUSED ; } ;
	ri->CurCol = 0 ; ok = TRUE ; returnv4r = FALSE ; HTTP_Other_Count = 0 ; ZS(FileName) ;
	for(;ok;)
	 { len = fread(buf,1,2,fp) ;
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

	   bytesread += bytes ;
// printf("type=%d, bytes =%d data=%s\n",type,bytes,buf) ;
	   sig = FALSE ; justify = UNUSED ;
	   switch(type)
	    { 
	      case V4SS_Type_EOF:		ok = FALSE ; break ;
	      case V4SS_Type_EOL:		ri->CurCol = 0 ; ri->RowCount ++ ; break ;
	      case V4SS_Type_Int:		sig = TRUE ; justify = RI_JustifyRight ; break ;
	      case V4SS_Type_Double:		sig = TRUE ; justify = RI_JustifyRight ; break ;
	      case V4SS_Type_UDate:		sig = TRUE ; justify = RI_JustifyRight ; break ;
	      case V4SS_Type_Alpha:		sig = TRUE ; break ;
	      case V4SS_Type_UMonth:		sig = TRUE ; justify = RI_JustifyRight ; break ;
	      case V4SS_Type_UQtr:		sig = TRUE ; justify = RI_JustifyRight ; break ;
	      case V4SS_Type_UPeriod:		sig = TRUE ; justify = RI_JustifyRight ; break ;
	      case V4SS_Type_UWeek:		sig = TRUE ; justify = RI_JustifyRight ; break ;
	      case V4SS_Type_UndefVal:		sig = TRUE ; break ;
	      case V4SS_Type_Formula:		sig = TRUE ; break ;
	      case V4SS_Type_SSVal:		sig = TRUE ; break ;
	      case V4SS_Type_Fixed:		sig = TRUE ; break ;
	      case V4SS_Type_FAlpha:		sig = TRUE ; break ;
	      case V4SS_Type_Warn:
	      case V4SS_Type_Alert:
	      case V4SS_Type_Err:
		p = buf ;
		if (strncmp(p,"<V4Err>",7) == 0)
		 { p += 7 ; p1 = strstr(p,"</V4Err>") ; if (p1 != NULL) *p1 = '\0' ; } ;
		strcpy(ri->ErrMsg,p) ; break ;
	      case V4SS_Type_HTTP_HTML:		break ;			/* We are NOT generating HTML - can ignore this */
	      case V4SS_Type_HTTP_Other:
		if (HTTP_Other_Count >= HTTP_HeaderMax) break ;		/* Don't go over max */
		HTTP_Other[HTTP_Other_Count] = v4mm_AllocChunk(strlen(buf)+1,FALSE) ; strcpy(HTTP_Other[HTTP_Other_Count],buf) ;
		HTTP_Other_Count++ ; break ;
	      case V4SS_Type_FileName:		strcpy(FileName,buf) ; break ;
	      case V4SS_Type_FormatControl:
	      case V4SS_Type_FormatStyleName:
	      case V4SS_Type_FormatFont:
	      case V4SS_Type_FormatStyle:
	      case V4SS_Type_FormatFontSize:
	      case V4SS_Type_FormatFormat:
	      case V4SS_Type_PageBreak:
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
	      case V4SS_Type_Page:		break ;
	      case V4SS_Type_SetPrintTitles1:
	      case V4SS_Type_SetPrintTitles2:
	      case V4SS_Type_SetColGap:
	      case V4SS_Type_SetRowGap:
	      case V4SS_Type_SetRows:
	      case V4SS_Type_SetColumns:
	      case V4SS_Type_AutoFit:
	      case V4SS_Type_Run:
	      case V4SS_Type_XOffset:
	      case V4SS_Type_YOffset:
	      case V4SS_Type_UpdOK:
	      case V4SS_Type_Sheet:
	      case V4SS_Type_Row:
	      case V4SS_Type_Column:
	      case V4SS_Type_RowColor:
	      case V4SS_Type_ColColor:
	      case V4SS_Type_CellColor:
	      case V4SS_Type_InsertObject:
	      case V4SS_Type_Note:
	      case V4SS_Type_Message:		break ;
	      case V4SS_Type_FrameCnt:		break ;
	      case V4SS_Type_Grid:		gridlines = buf[0] ; break ;
	      case V4SS_Type_End:		if (strchr(buf,27) != NULL)	/* Check for escape characters - delete */
						 { int i,j ;
						   for(i=0,j=0;;i++) { if (buf[i] != 27) { buf[j++] = buf[i] ; } ; if (buf[i] == '\0') break ; } ;
						 } ;
						strcpy(ri->Title1,buf) ; ri->ColCapRowCount = 1 ; break ;
	      case V4SS_Type_PageBreakAfter:	break ;
	      case V4SS_Type_Id:		strcpy(ri->Id,buf) ; break ;
	      case V4SS_Type_URLLink:		break ;
	      case V4SS_Type_URLLinkX:		break ;
	      case V4SS_Type_HTML:		break ;		/* Embedded HTML ignored for now */
	      case V4SS_Type_HTMLX:		break ;
	      case V4SS_Type_SSDim:		break ;
	      case V4SS_Type_FormatSpec:	break ;
	    } ;
	   if (sig)						/* Track width of column */
	    { 
	      if (ri->CurCol >= RI_ColMax) ri->CurCol = RI_ColMax-1 ;
	      if (ri->Col[ri->CurCol].CharWidth < strlen(buf)) ri->Col[ri->CurCol].CharWidth = strlen(buf) ;
	      if (ri->Col[ri->CurCol].FontSize == 0) ri->Col[ri->CurCol].FontSize = 2 ;
	      if (justify != UNUSED) ri->Col[ri->CurCol].Justify = justify ;
	      ri->CurCol ++ ; if (ri->CurCol > ri->ColCount) ri->ColCount = ri->CurCol ;	/* Track highest column */
	    } ;
	 } ;

	if (bytesread < 10)
	 { printf("? Input file appears empty\n") ; exit(EXITABORT) ; } ;

	if (ri->ErrMsg[0] != '\0')			/* Got an error? */
	 { 
	   wb = wbNew(ofile) ;
	   ws = wbAddWorkSheet(wb,"Error",0) ;
	   wsWriteString(ws,0,1,ri->ErrMsg,NULL) ;
	   goto end_job ;
	 } ;

/*	Second pass - generate Excel/BIFF */
	fseek(fp,0,SEEK_SET) ;			/* Position to begin of file */

/*	Open up the output file */
	wb = wbNew(ofile) ;
/*	Generating HTTP Headers ? */
/*	If going back to WWW client then add some necessary HTTP headers */
	if (towww)
	 { fprintf(wb->ole->fp,"Content-Type: application/octet-stream\n") ;
	   fprintf(wb->ole->fp,"Expires: -1\n") ;
	   fprintf(wb->ole->fp,"Cache-Control: max-age=1\n") ;
	   fprintf(wb->ole->fp,"Content-Disposition: attachment; filename=\"%s\"\n",(strlen(FileName) == 0 ? "result.xls" : FileName)) ;
	   if (HTTP_Other_Count == 0) fprintf(wb->ole->fp,"\n") ;
	 } ;
	if (HTTP_Other_Count > 0)
	 { for(i=0;i<HTTP_Other_Count;i++) { fprintf(wb->ole->fp,"%s\n",HTTP_Other[i]) ; } ;
	   fprintf(wb->ole->fp,"\n") ;				/* Empty line ends header section */
	 } ;
	stat(ifile,&statbuf) ; wb->wsSize = statbuf.st_size * 2 ;
	if (wb->wsSize < 1000 || wb->wsSize > 5000000) wb->wsSize = WS_DataSize ;
	ws = wbAddWorkSheet(wb,"Sheet1",wb->wsSize) ;
	if (gridlines != UNUSED) ws->GridLines = gridlines ;

/*	Set up some default formats */
	dateformat = wbAddFormat(wb) ; strcpy(dateformat->mask,"dd-mmm-yy") ;
	switch (ws->GridLines)
	 { default:	break ;
	   case 2:	dateformat->top = (dateformat->bottom = (dateformat->left = (dateformat->right = 0x7))) ;
			defaultformat = wbAddFormat(wb) ;
			defaultformat->top = (defaultformat->bottom = (defaultformat->left = (defaultformat->right = 0x7))) ;
			break ;
	 } ;



	ri->CurCol = 0 ; ok = TRUE ; ri->CurRow = 0 ;		/* Excel upper left is (0,0) */
	checkmt = FALSE ;					/* If TRUE check for empty row - don't advance ri->CurRow */
	gotfnp = FALSE ;
	for(;ok;)
	 { len = fread(buf,1,2,fp) ;
	   if (len < 2) break ;
	   type = buf[0] ;			/* Entry type */
	   bytes = buf[1] ;			/* Length of entry */
	   if (ri->CurRow < 0) ri->CurRow = 0 ;	/* Don't let this go negative */

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

/*	   If any column format/style then copy into "current" */
	   if (ri->CurFormat[0] == '\0') strcpy(ri->CurFormat,ri->Col[ri->CurCol].ColFormat) ;
	   if (ri->CurStyle[0] == '\0') strcpy(ri->CurStyle,ri->Col[ri->CurCol].ColStyle) ;

//if (ri->CurRow < 3) printf("row=%d, type=%d\n",ri->CurRow,type) ;
	   switch(type)
	    { default: printf("? Type=%d\n",type) ; break ;
	      case V4SS_Type_EOF:		ok = FALSE ; break ;
	      case V4SS_Type_EOL:
		if (checkmt && ri->CurCol == 0) { checkmt = FALSE ; break ; } ;
		ri->CurCol = 0 ; ri->CurRow++ ;
		break ;
	      case V4SS_Type_Int:
		num = strtol(buf,&b1,10) ;
		format = (ssdim != UNUSED ? ssdimformats[ssdim] : defaultformat) ; ssdim = UNUSED ;
		format = wbMergeFormats(wb,format,(gotfnp ? &formatnp : NULL)) ;
		wsWriteNumber(ws,ri->CurRow,ri->CurCol,(double)num,format) ;
		if (format != NULL)			/* Make educated guess as to length of column */
		 { UCCHAR ucbuf[100] ; v_FormatInt(num,ASCretUC(format->mask),ucbuf,NULL) ; UCstrcpyToASC(tbuf,ucbuf) ; len = strlen(tbuf) ;
		 } else
		 { sprintf(tbuf,"%d",num) ; len = strlen(tbuf) ;
		 } ;
		if (widths[ri->CurCol] < len) widths[ri->CurCol] = len ;
		ZS(ri->CurFormat) ;		/* Clear "current column" format */
		ZS(ri->CurStyle) ;		/* Clear "current column" style */
		ri->CurCol++ ; gotfnp = FALSE ; break ;
	      case V4SS_Type_Double:
		dnum = strtod(buf,NULL) ;
		format = (ssdim != UNUSED ? ssdimformats[ssdim] : defaultformat) ; ssdim = UNUSED ;
		format = wbMergeFormats(wb,format,(gotfnp ? &formatnp : NULL)) ;
		wsWriteNumber(ws,ri->CurRow,ri->CurCol,dnum,format) ;
		if (format != NULL)			/* Make educated guess as to length of column */
		 { UCCHAR ucbuf[100] ; v_FormatDbl(dnum,ASCretUC(format->mask),ucbuf,NULL) ; UCstrcpyToASC(tbuf,ucbuf) ; len = strlen(tbuf) ;
		 } else
		 { sprintf(tbuf,"%d",num) ; len = strlen(tbuf) ;
		 } ;
		if (widths[ri->CurCol] < len) widths[ri->CurCol] = len ;
		ZS(ri->CurFormat) ;		/* Clear "current column" format */
		ZS(ri->CurStyle) ;		/* Clear "current column" style */
		ri->CurCol++ ; gotfnp = FALSE ; break ;
	      case V4SS_Type_UDate:
		num = strtol(buf,&b1,10) ; num = num - 50084 + 35065 ;
		format = dateformat ;
		if (ssdim != UNUSED) { format = wbMergeFormats(wb,format,ssdimformats[ssdim]) ; ssdim = UNUSED ; } ;
		if (gotfnp) format = wbMergeFormats(wb,format,&formatnp) ;
		if (num <= 0) { wsWriteString(ws,ri->CurRow,ri->CurCol,"",format) ; }
		 else { wsWriteNumber(ws,ri->CurRow,ri->CurCol,(double)num,format) ; } ;
		if (widths[ri->CurCol] < 10) widths[ri->CurCol] = 10 ;
		ZS(ri->CurFormat) ;		/* Clear "current column" format */
		ZS(ri->CurStyle) ;		/* Clear "current column" style */
		ri->CurCol++ ; gotfnp = FALSE ; break ;
	      case V4SS_Type_Alpha:
		buf[255] = '\0' ;			/* Make sure buffer does not exceed 255 */
		format = (ssdim != UNUSED ? ssdimformats[ssdim] : defaultformat) ; ssdim = UNUSED ;
		format = wbMergeFormats(wb,format,(gotfnp ? &formatnp : NULL)) ;
/*		Have to check format for multi-column centered string - if so parse number of columns */
		if (format != NULL ? format->text_h_align == 6 : FALSE)
		 { /* {{ */
		   if (buf[0] == '{' && (buf[2] == '}' || buf[3] == '}'))
		    { num = buf[1] - '0' ;
		      if (buf[2] == '}') { b1 = &buf[3] ; } else { num = num * 10 + buf[2] - '0' ; b1 = &buf[4] ; } ;
		      FixUpStrings(b1) ;
		      wsWriteString(ws,ri->CurRow,ri->CurCol,b1,format) ; ri->CurCol ++ ;
		      for(i=1;i<num;i++)		/* Fill out following columns with blank cells */
		       { wsWriteString(ws,ri->CurRow,ri->CurCol++,"",format) ; } ;
		      break ;
		    } ;
		 } ;
/*		Multi-column left/right justify not supported - strip "{n}" and write out */
		if (format != NULL ? (format->text_h_align == 20 || format->text_h_align == 21) : FALSE)
		 { 
		   /* {{ */
		   if (buf[0] == '{' && (buf[2] == '}' || buf[3] == '}'))
		    { num = buf[1] - '0' ;
		      if (buf[2] == '}') { b1 = &buf[3] ; } else { num = num * 10 + buf[2] - '0' ; b1 = &buf[4] ; } ;
		      FixUpStrings(b1) ;
		      wsWriteString(ws,ri->CurRow,ri->CurCol,b1,format) ; ri->CurCol ++ ;
		      for(i=1;i<num;i++)		/* Fill out following columns with blank cells */
		       { wsWriteString(ws,ri->CurRow,ri->CurCol++,"",format) ; } ;
		      break ;
		    } ;
		 } ;
/*		If have escape character (for substitution by HTML) then pull out */
		if (strchr(buf,27) != NULL)
		 { int i,j ;
		   for(i=0,j=0;;i++) { if (buf[i] != 27) { buf[j++] = buf[i] ; } ; if (buf[i] == '\0') break ; } ;
		 } ;
/*		If any embedded HTML stuff or &xxx; then strip out */
		FixUpStrings(buf) ;

		wsWriteString(ws,ri->CurRow,ri->CurCol,buf,format) ;
		maxdnum = 0.0 ;
		for(i=0,dnum=0.0;buf[i]!='\0';i++)
		 { if (buf[i] >= 'a' && buf[i] <= 'z')
		    { switch (buf[i])
		       { default:
				dnum += 1.0 ; break ;
		         case 'i': case 'l': case 'j': case 't':
				dnum += 0.6 ; break ;
		       } ; continue ;
		    } ;
		   if (buf[i] >= 'A' && buf[i] <= 'Z')
		    { switch (buf[i])
		       { default:
				dnum += 1.3 ; break ;
		         case 'I':
				dnum += 0.9 ; break ;
		       } ; continue ;
		    } ;
		   switch (buf[i])
		    { default:
			dnum += 1 ; break ;
		      case 10:				/* New line - start count all over again */
			if (dnum > maxdnum) maxdnum = dnum ; dnum = 0.0 ; break ;
		      case '.': case ',': case ';':
			dnum += 0.8 ; break ;
		    } ;
		 } ; if (maxdnum > dnum) dnum = maxdnum ;
		num = (int)(dnum + 0.50) ;
		if (format != NULL) { if (format->bold >= 0x2bc) num += 2 ; } ;
		if (widths[ri->CurCol] < num) widths[ri->CurCol] = num ;
		ZS(ri->CurURL)	;		/* Clear "current column" URL */
		ZS(ri->CurFormat) ;		/* Clear "current column" format */
		ZS(ri->CurStyle) ;		/* Clear "current column" style */
		ri->CurCol++ ; gotfnp = FALSE ; break ;
	      case V4SS_Type_UMonth:		ri->CurCol++ ; break ;
	      case V4SS_Type_UQtr:		ri->CurCol++ ; break ;
	      case V4SS_Type_UPeriod:		ri->CurCol++ ; break ;
	      case V4SS_Type_UWeek:		ri->CurCol++ ; break ;
	      case V4SS_Type_UndefVal:		ri->CurCol++ ; break ;
	      case V4SS_Type_Formula:
		format = (ssdim != UNUSED ? ssdimformats[ssdim] : defaultformat) ; ssdim = UNUSED ;
		wsWriteFormula(ws,ri->CurRow,ri->CurCol,(buf[0] == '=' ? &buf[1] : buf),0.0,1,format) ;
		if (widths[ri->CurCol] < 8) widths[ri->CurCol] = 8 ;
		ZS(ri->CurURL)	;		/* Clear "current column" URL */
		ZS(ri->CurFormat) ;		/* Clear "current column" format */
		ZS(ri->CurStyle) ;		/* Clear "current column" style */
		ri->CurCol++ ; gotfnp = FALSE ; break ;
	      case V4SS_Type_SSVal:
		num = UNUSED ;
		switch(buf[0])
		 {
		   case V4SS_SSVal_Null:	num = 0 ; break ;
		   case V4SS_SSVal_Div0:	num = 0x07 ; break ;
		   case V4SS_SSVal_Value:	num = 0x0f ; break ;
		   case V4SS_SSVal_Ref:		num = 0x17 ; break ;
		   case V4SS_SSVal_Name:	num = 0x1d ; break ;
		   case V4SS_SSVal_Num:		num = 0x24 ; break ;
		   case V4SS_SSVal_NA:		num = 0x2a ; break ;
		   case V4SS_SSVal_Empty:	strcpy(buf,"") ; break ;
		   case V4SS_SSVal_Row:		break ;	/* Current Row?  */
		   case V4SS_SSVal_Column:		/* Current column - save any formats & styles */
			strcpy(ri->Col[ri->CurCol].ColFormat,ri->CurFormat) ;
			strcpy(ri->Col[ri->CurCol].ColStyle,ri->CurStyle) ;
			break ;
		 } ;
		format = (ssdim != UNUSED ? ssdimformats[ssdim] : defaultformat) ; ssdim = UNUSED ;
		if (num != UNUSED) wsWriteError(ws,ri->CurRow,ri->CurCol,num,format) ;
		if (widths[ri->CurCol] < 8) widths[ri->CurCol] = 8 ;
		ZS(ri->CurFormat) ;		/* Clear "current column" format */
		ZS(ri->CurStyle) ;		/* Clear "current column" style */
		ri->CurCol++ ; gotfnp = FALSE ; break ;
	      case V4SS_Type_Fixed:		ri->CurCol++ ; break ;
	      case V4SS_Type_FAlpha:		
		if (strlen(ri->CurURL) > 0) break ;	/* Skip over this if prior was URL::xxx tag, next entry will be value */
		ri->CurCol++ ; break ;
	      case V4SS_Type_Warn:
	      case V4SS_Type_Alert:
	      case V4SS_Type_Err:		ri->CurCol++ ; break ;
	      case V4SS_Type_FormatControl:
	      case V4SS_Type_FormatStyleName:
	      case V4SS_Type_FormatFont:
	      case V4SS_Type_FormatStyle:	strcpy(ri->CurStyle,buf) ; break ;
	      case V4SS_Type_FormatFontSize:	break ;
	      case V4SS_Type_FormatFormat:	strcpy(ri->CurFormat,buf) ; break ;
	      case V4SS_Type_PageBreakAfter:
	      case V4SS_Type_PageBreak:
		if (ws->PBCount < WS_PBMax) ws->PBRows[ws->PBCount++] = ri->CurRow ;
		checkmt = TRUE ; break ;
	      case V4SS_Type_InsertRow:
	      case V4SS_Type_SetColWidth:	widths[ri->CurCol] = atof(buf) ; break ;
	      case V4SS_Type_SetColWidthList:
		for(i=0,p1=buf;;i++,p1=b1+1)
		 { num = strtol(p1,&b1,10) ; if (*b1 != ',') break ;
		   if (num <= 0) continue ;	/* If not a real width then skip */
		   widths[i] = num ;		/* Set width of this column */
		 } ;
		break ;
	      case V4SS_Type_Desc:
	      case V4SS_Type_DescX:
	      case V4SS_Type_ExcelIni:
	      case V4SS_Type_CSS:		ri->CurRow-- ; break ;		/* Decrement row so eol does not increment */
	      case V4SS_Type_Menu1:
	      case V4SS_Type_Menu2:
	      case V4SS_Type_Eval:
	      case V4SS_Type_Justify:
	      case V4SS_Type_Echo:
	      case V4SS_Type_NoEcho:		break ;
	      case V4SS_Type_HTTP_HTML:		break ;
	      case V4SS_Type_HTTP_Other:	break ;
	      case V4SS_Type_FileName:		break ;
	      case V4SS_Type_ScriptInclude:	ri->CurRow-- ; break ;		/* Decrement row so eol does not increment */
	      case V4SS_Type_Header:		strcpy(ws->header,buf) ; break ;
	      case V4SS_Type_Footer:		strcpy(ws->footer,buf) ; break ;
	      case V4SS_Type_LMargin:		ws->LMargin = atof(buf) ; break ;
	      case V4SS_Type_RMargin:		ws->RMargin = atof(buf) ; break ;
	      case V4SS_Type_TMargin:		ws->TMargin = atof(buf) ; break ;
	      case V4SS_Type_BMargin:		ws->BMargin = atof(buf) ; break ;
	      case V4SS_Type_Portrait:		ws->Portrait = TRUE ; break ;
	      case V4SS_Type_Landscape:		ws->Portrait = FALSE ; break ;
	      case V4SS_Type_Grid:		break ;
	      case V4SS_Type_FrameCnt:		break ;
	      case V4SS_Type_Scale:
		num = atof(buf) ;
		if (num == 0)
		 { ws->FitToHeight = 1 ; }			/* Wants to fit on single page */
		 else if (num < 0) { ws->FitToHeight = -num ; }	/* If negative then number of pages down */
		 else if (num < 10) { ws->FitToWidth = num ; }	/* If < 10 then number of pages across */
		 else { ws->Scale = num ;			/* Else global scale for worksheet */
			ws->FitToHeight = 0 ; ws->FitToWidth = 0 ;/* Reset these */
		      } ;
		checkmt = TRUE ; break ;
	      case V4SS_Type_Page:
	      case V4SS_Type_SetPrintTitles1:	ri->ColCapRowCount = buf[0] ; break ;
	      case V4SS_Type_SetPrintTitles2:
	      case V4SS_Type_SetColGap:
	      case V4SS_Type_SetRowGap:
	      case V4SS_Type_SetRows:
	      case V4SS_Type_SetColumns:
	      case V4SS_Type_AutoFit:
	      case V4SS_Type_Run:
	      case V4SS_Type_XOffset:
	      case V4SS_Type_YOffset:
	      case V4SS_Type_UpdOK:
		break ;
	      case V4SS_Type_Sheet:
		if (ri->CurRow <= 1 && ri->CurCol == 0)			/* Very begin? */
		 { strcpy(ws->name,buf) ; }				/*  then update current ws name */
		 else { 
			if (ri->ColCapRowCount > 0)
			 { char tbuf[32] ; sprintf(tbuf,"%s!$1:$%d",ws->name,ri->ColCapRowCount) ;
			   wsStoreDefinedName(ws,NULL,7,1,tbuf,ri->ColCapRowCount) ;
			 } ;
			for(i=0;i<RI_ColMax;i++)
			 { if (widths[i] == UNUSED) continue ;
			   ws->ColInfo[ws->colCount].firstcol = (ws->ColInfo[ws->colCount].lastcol = i) ;
			   ws->ColInfo[ws->colCount].width = widths[i] ;
			   ws->colCount++ ;
			 } ;
			ws = wbAddWorkSheet(wb,buf,wb->wsSize) ;	/*  otherwise create a new sheet */
			ri->CurRow = 0 ; ri->CurCol = 0 ; memset(ri,0,sizeof *ri) ;
			for(i=0;i<RI_ColMax;i++) { widths[i] = UNUSED ; } ;
		      } ;
		checkmt = TRUE ; break ;
	      case V4SS_Type_Row:
		if (buf[0] == '+' || buf[0] == '-')
		 { ri->CurRow += atol(&buf[1]) ; }
		 else { ri->CurRow = atol(buf)-1 ; } ;
		checkmt = TRUE ; break ;
	      case V4SS_Type_Column:
		if (buf[0] == '+' || buf[0] == '-')
		 { ri->CurCol += atol(&buf[1]) ; }
		 else { ri->CurCol = atol(buf)-1 ; } ;
		checkmt = TRUE ; break ;
	      case V4SS_Type_RowColor:
	      case V4SS_Type_ColColor:
	      case V4SS_Type_CellColor:
	      case V4SS_Type_InsertObject:
	      case V4SS_Type_Note:
	      case V4SS_Type_Message:		break ;
	      case V4SS_Type_End:
		if (strchr(buf,27) != NULL)	/* Check for escape characters - delete */
		 { int i,j ;
		   for(i=0,j=0;;i++) { if (buf[i] != 27) { buf[j++] = buf[i] ; } ; if (buf[i] == '\0') break ; } ;
		 } ;
	      	ws->Portrait = (ws->dim_colmax > 10 ? FALSE : TRUE) ;
		ws->FitToWidth = 1 ;			/* Try to fit one page across */
		ws->LMargin = 0.0 ; ws->RMargin = 0.0 ; ri->ColCapRowCount = 1 ;
		strcpy(ws->header,"&C&12&\"Arial,Bold Italic\"") ; strcat(ws->header,buf) ;
		checkmt = TRUE ; break ;
	      case V4SS_Type_Id:
		sprintf(ws->footer,"&L%s&CPage &P&R",(tranid == NULL ? "&D" : tranid)) ;
		strcat(ws->footer,buf) ;
		checkmt = TRUE ; break ;
	      case V4SS_Type_URLLinkX:		strcat(ri->CurURL,buf) ; break ;
	      case V4SS_Type_URLLink:		strcat(ri->CurURL,buf) ; break ;
	      case V4SS_Type_HTML:		break ;
	      case V4SS_Type_HTMLX:		break ;
	      case V4SS_Type_SSDim:		ssdim = buf[0] ; break ;
	      case V4SS_Type_FormatSpecNP:
		memset(&formatnp,0,sizeof formatnp) ;
		gotfnp = TRUE ;			/* Fall thru to below */
	      case V4SS_Type_FormatSpec:
		memcpy(&vfs,buf,sizeof vfs) ;	/* Copy into structure (may overextend Length but that's OK) */
		memset(&tformat,0,sizeof tformat) ; formatNew(0,&tformat) ;
		if (vfs.MaskX != 0) { strcpy(tformat.mask,&vfs.VarString[vfs.MaskX-1]) ; } ;
		if (vfs.FontBold == V4SS_FontBold_Bold) tformat.bold = 0x2bc ;
		if (vfs.FontAttr & V4SS_Font_Italic) tformat.italic = 1 ;
		if (vfs.FontAttr & V4SS_Font_StrikeThru) tformat.font_strikeout = 1 ;
		if (vfs.FontAttr & V4SS_Font_Wrap) tformat.text_wrap = TRUE ;
		if (vfs.FontAttr & V4SS_Font_SizeToFit) tformat.size_to_fit = TRUE ;
		switch (vfs.HAlign)
		 { case V4SS_Align_Left:		tformat.text_h_align = 1 ; break ;
		   case V4SS_Align_Right:		tformat.text_h_align = 3 ; break ;
		   case V4SS_Align_Center:		tformat.text_h_align = 2 ; break ;
		   case V4SS_Align_Justify:		tformat.text_h_align = 5 ; break ;
		   case V4SS_Align_Fill:		tformat.text_h_align = 4 ; break ;
		   case V4SS_Align_MultiColCenter:	tformat.text_h_align = 6 ; break ;
		   case V4SS_Align_MultiColLeft:	tformat.text_h_align = 20 ; break ;
		   case V4SS_Align_MultiColRight:	tformat.text_h_align = 21 ; break ;
		 } ;
		switch (vfs.VAlign)
		 { case V4SS_Align_Center:		tformat.text_v_align = 1 ; break ;
		   case V4SS_Align_Top:			tformat.text_v_align = 0 ; break ;
		   case V4SS_Align_Bottom:		tformat.text_v_align = 2 ; break ;
		 } ;
		tformat.fg_color = v_ColorRefToXL(vfs.FgColor,FALSE) ; tformat.bg_color = v_ColorRefToXL(vfs.BgColor,FALSE) ;
		tformat.pattern = vfs.FillPattern ; tformat.color = v_ColorRefToXL(vfs.FontColor,FALSE) ;
		switch (ws->GridLines)
		 { default:	break ;
		   case 2:	tformat.top = (tformat.bottom = (tformat.left = (tformat.right = 0x7))) ; break ;
		 } ;
		if (type == V4SS_Type_FormatSpec)
		 { for(i=0;i<wb->formatCount;i++)
		    { tformat.xf_index = wb->format[i]->xf_index ;
		      if (memcmp(wb->format[i],&tformat,sizeof tformat) == 0) { format = wb->format[i] ; break ; } ;
		    } ;
/*		   Have to make a new format */
		   if (i >= wb->formatCount) 
		    { int xfi ;
		      format = wbAddFormat(wb) ; xfi = format->xf_index ;
		      memcpy(format,&tformat,sizeof tformat) ; format->xf_index = xfi ;
		    } ;
		   if (ssdim != UNUSED && ssdim < V4SS__DimToFormat_Max) ssdimformats[ssdim] = format ;
		 } else
		 { memcpy(&formatnp,&tformat,sizeof tformat) ;
		 } ;
//		format = (type == V4SS_Type_FormatSpec ? wbAddFormat(wb) : &formatnp) ;
		ssdim = UNUSED ;
	    } ;
	 } ;

	if (ri->ColCapRowCount > 0)
	 { char tbuf[32] ; sprintf(tbuf,"%s!$1:$%d",ws->name,ri->ColCapRowCount) ;
	   wsStoreDefinedName(ws,NULL,7,1,tbuf,ri->ColCapRowCount) ;
	 } ;

	for(i=0;i<RI_ColMax;i++)
	 { if (widths[i] == UNUSED) continue ;
	   ws->ColInfo[ws->colCount].firstcol = (ws->ColInfo[ws->colCount].lastcol = i) ;
	   ws->ColInfo[ws->colCount].width = widths[i] ;
	   ws->colCount++ ;
	 } ;
end_job:
	fclose(fp) ;
	wbClose(wb) ;
	exit(EXITOK) ;
}

void FixUpStrings(buf)
  char *buf ;
{
  int fix ; char *p, *p1, *p2 ;
  char tbuf[2048] ;

/*	If any embedded HTML stuff then strip out */
/*	p1 = begin of string, p = ptr to '<', p2 = ptr to '>', tbuf holds "fixed" string up to p1 */
	for(tbuf[0]='\0',p1=buf,fix=FALSE;(p=strchr(p1,'<')) != NULL;)
	 { p2 = strchr(p,'>') ; fix = TRUE ;
	   if (p2 == NULL || (p2 - p) > 15)
	    { char save ; save = *(p+1) ; *(p+1) = '\0' ; strcat(tbuf,p1) ; *(p+1) = save ;
	      p1 = p + 1 ; continue ;
	    } ;
	   *p = '\0' ; strcat(tbuf,p1) ; 
	   p1 = p2 + 1 ; continue ; 
	 } ;
	if (fix) { strcat(tbuf,p1) ; strcpy(buf,tbuf) ; } ;
/*	Now sortof repeat above for "&xxx;" construct */
	for(tbuf[0]='\0',p1=buf,fix=FALSE;(p=strchr(p1,'&')) != NULL;)
	 { p2 = strchr(p,';') ; fix = TRUE ;
	   if (p2 == NULL || (p2 - p) > 7)
	    { char save ; save = *(p+1) ; *(p+1) = '\0' ; strcat(tbuf,p1) ; *(p+1) = save ;
	      p1 = p + 1 ; continue ;
	    } ;
	   *p = '\0' ; strcat(tbuf,p1) ; 
	   p1 = p2 + 1 ; continue ; 
	 } ;
	if (fix)
	 { strcat(tbuf,p1) ; strcpy(buf,tbuf) ; } ;
}