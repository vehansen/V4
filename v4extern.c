/*	v4extern - Handles interfaces to external stuff (HTML, ODBC, V4IS, ... ) */

#define NEED_SOCKET_LIBS 1
#include <time.h>
#include "v4imdefs.c"
//#include "v3defs.c"
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

#ifdef UNIX
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef WINNT
#include <windows.h>
#include <process.h>
#include <io.h>
#endif

GLOBALDIMSEXTERN
extern struct V4C__ProcessInfo *gpi ;	/* Global process information */
extern struct V__UnicodeInfo *uci ;	/* Global structure of Unicode Info */
V4DEBUG_SETUP
extern int vout_TotalBytes ;
extern int opcodeDE[] ;
extern ETYPE traceGlobal ;
extern struct V4SS__DimToFormatMap *vdfm ;
extern struct V4DPI__LittlePoint CondEvalRet ;

#include "v4fin.c"
#include "v4stat.c"
#define REGEX_MALLOC
#include "v4regexp.c"

#ifndef WINNT
#undef TXTEOL
#define TXTEOL "\r\n"			/* Force this to be cr/lf until all clients running at least 1.18 of V4Server */
#endif

#define ONCE(param) if (param != NULL) { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"ModArgOnce",intmodx) ; break ; } ;
#define ONCEU(param) if (param >= 0) { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"ModArgOnce",intmodx) ; break ; } ;

#define V4HTML_ContentType_Undef 0	/* Unknown content type */
#define V4HTML_ContentType_HTML 1	/* Content is HTML */
#define V4HTML_ContentType_Text 2	/* Content is plain text */
#define V4HTML_ContentType_Appl 3	/* Content is application - probably binary */
#define V4HTML_ContentType_Image 4	/* Content is an image */
#define V4HTML_ContentType_XML 5	/* Content is an XML packet */
#define V4HTML_ContentType_Sound 6	/* Content is a sound file (ex: mp3) */

#define V4HTML_Trace_URL 0x1
#define V4HTML_Trace_Header 0x2
#define V4HTML_Trace_Body 0x4
#define V4HTML_Trace_Request 0x8


int v4html_BytesSend = 0 ;		/* Total number of bytes sent to ? */
int v4html_BytesRecv = 0 ;		/* Total number of bytes received from ? */


#ifdef WANTHTMLMODULE

#define V4HTML_TagMax 50		/* Max number of nested tags */
#define V4HTML_TagNameMax 76
#define V4HTML_InputValueMax 512
#define V4HTML_InputMax 50
#define V4HTML_MaxFileBytes 1000000	/* Max bytes to read from local file */
#define V4HTML_MaxURL_Length 0x8000	/* Max length of URL */

#define V4HTML_InputType_Unknown 0
#define V4HTML_InputType_Submit 1
#define V4HTML_InputType_Hidden 2
#define V4HTML_InputType_Text 3
#define V4HTML_InputType_Radio 4
#define V4HTML_InputType_Reset 5
#define V4HTML_InputType_Password 6
#define V4HTML_InputType_TextArea 7
#define V4HTML_InputType_FileName 8	/* File - have to send request enctype='multipart/form-data' */

#define V4HTML_PageType_Undef 0		/* Page type not yet known */
#define V4HTML_PageType_Page 1		/* Regular old page */
#define V4HTML_PageType_Goto 3		/* A nested page that is a "goto label" of prior (share same .Page pointer) */
#define V4HTML_PageType_Section 4	/* A nested page that is a section of prior (does NOT share .Page pointer) */
#define V4HTML_PageType_File 5		/* A page that was pulled from a local file */


#define V4HTML_XfrEnc_Undef 0		/* Transfer Encoding */
#define V4HTML_XfrEnc_Chunked 1		/*   Chunked */

#define V4HTMP_Page_Get 1
#define V4HTMP_Page_Post 2
#define V4HTMP_Page_Head 3
#define V4HTMP_Page_XMLRPC 4		/* Making XML Remote-Procedure-Call */

#define V4HTML_URL_UNKOWN 0
#define V4HTML_URL_HTTP 1
#define V4HTML_URL_FTP 2
#define V4HTML_URL_FilePath 3

#define V4HTML_GNP_EOP 0		/* End of page */
#define V4HTML_GNP_PlainText 1		/* In plain text segment */
#define V4HTML_GNP_StartSeg 2		/* Starting segment <xxx> */
#define V4HTML_GNP_EndSeg 3		/* Ending segment </xxx> */
#define V4HTML_GNP_Error 4		/* Invalid syntax detected */

struct V4HTML__Page {
  struct V4HTML__Page *PriorHTMLPage ;	/* Pointer to prior page, NULL if none */
  int MaxPageChars ;			/* Max number of characters that will fit into Page below */
  int PageIncrement ;			/* Number of characters to increment MaxPageChars on page GET */
  UCCHAR *Page ;			/* Pointer to current page text */
  int PageChars ;			/* Number of characters in this page */
  UCCHAR BaseHostName[128] ;		/* Base host name for this page */
  UCCHAR BasePath[128] ;		/* Base path for this page */
  UCCHAR UserName[64] ;			/* User name for basic authentication */
  UCCHAR UserPassword[64] ;		/*  and password */
  int Port ;				/* Port number (80 is default for HTTP/HTML) */
  int PageType ;			/* Type of page - V4HTML_PageType_xxx */
  int ContentType ;			/* Content of page - V4HTML_ContentType_xxx */
  int TransferEncoding ;		/* How page data to be transferred - V4HTML_XfrEnc_xxx */
  int ContentLength ;			/* Content byte count (from Content-Length header) */
  int curStart,curEnd ;			/* Current start/end on page */
  UCCHAR TagName[128] ;			/* Current tag name in UPPERCASE */
  int HTag ;				/* Current HTag value (UNUSED if unknown) */
  int TagX ;				/* Nesting */
  struct {
    int HTag ;				/* HTag value-  V4HTM_HTag_xxx */
    UCCHAR Name[V4HTML_TagNameMax] ;	/* Name of entry (ex: <table id=xxx>) */
    int NumName ;			/* Numeric name (e.g. column number) */
    int curStart,curEnd ;		/* Start/End index for entry (what is enclosed by < ... >) */
    int ChildNum ;			/* Current child number (if this is table then current row, if this is row then column) */
    UCCHAR Value[256] ;			/* Value to track (e.g. href=xxx in <a ...> entry) */
  } Tag[V4HTML_TagMax] ;

  int SectionStart, SectionBytes ;	/* Updated with boundaries of a section in v4html_SectionBounds() */

  int NumRows ;				/* If this is a table then number of rows */
  int NumColumns ;			/*  and columns */
  int CurRow, CurColumn ;		/* Current Row/Column */

					/* Section below filled in for "current" form */
  int InputCount ;			/* Number of <input...>'s below */
  UCCHAR FormAction[V4HTML_InputValueMax] ;
  UCCHAR FormEncType[V4HTML_InputValueMax] ;
  struct {
    int Type ;				/* Type of input- V4HTML_InputType_xxx */
    UCCHAR Name[V4HTML_TagNameMax] ;	/* Input name */
    UCCHAR UCName[V4HTML_TagNameMax] ;	/* Input name - UPPERCASE */
    UCCHAR Value[V4HTML_InputValueMax] ;
    int DataChar ;			/* If > 0 then number of characters in data buffer below */
    UCCHAR *Data ;			/* If not NULL then pointer to data buffer */
   } Input[V4HTML_InputMax] ;
    
} ;

#define V4HTML_HTag_Unknown -1
#define V4HTML_HTag_HTML 0
#define V4HTML_HTag_Body 1
#define V4HTML_HTag_Section 2
#define V4HTML_HTag_Table 3
#define V4HTML_HTag_Row 4
#define V4HTML_HTag_Cell 5
#define V4HTML_HTag_Form 6
#define V4HTML_HTag_Image 7
#define V4HTML_HTag_Input 8
#define V4HTML_HTag_A 9
#define V4HTML_HTag_Area 10
#define V4HTML_HTag_Layer 11
#define V4HTML_HTag_Frame 12
#define V4HTML_HTag_TextArea 13
#define V4HTML_HTag_Text 100		/* Match a text segment in PositionInPage() */
#define V4HTML_HTag_Any 101		/* Match any HTag in PositionInPage() */
#define V4HTML_HTag_RegExp 102		/* Match a regular expression */

struct V4HTML__TagInfo {
  int HTag ;				/* V4HTML_HTag_xxx value */
  UCCHAR HTMLTag[V4HTML_TagNameMax] ;	/* HTML Tag name (upper case) */
} ;

struct V4HTML__TagInfo hti[] = {
	{ V4HTML_HTag_HTML, UClit("HTML") },
	{ V4HTML_HTag_Body, UClit("BODY") },
	{ V4HTML_HTag_Section, UClit("SECTION") },
	{ V4HTML_HTag_Table, UClit("TABLE") },
	{ V4HTML_HTag_Row, UClit("TR") },
	{ V4HTML_HTag_Cell, UClit("TD") },
	{ V4HTML_HTag_Form, UClit("FORM") },
	{ V4HTML_HTag_Image, UClit("IMG") },
	{ V4HTML_HTag_Input, UClit("INPUT") },
	{ V4HTML_HTag_A, UClit("A") },
	{ V4HTML_HTag_Area, UClit("AREA") },
	{ V4HTML_HTag_Layer, UClit("LAYER") },
	{ V4HTML_HTag_Frame, UClit("FRAME") },
	{ V4HTML_HTag_Frame, UClit("IFRAME") },
	{ V4HTML_HTag_TextArea, UClit("TEXTAREA") },
	{ -1, UClit("END") }
 } ;


static int Echo ;			/* Echo Flags */

LOGICAL v4html_GetNextPiece(vhp,errs)
  struct V4HTML__Page *vhp ;
  UCCHAR *errs ;
{
  int bx,l,i,res ; UCCHAR *p ;

	p = vhp->Page ;					/* p = current page */
top:
	bx=vhp->curEnd+1 ;				/* Position to next starting byte */
	vhp->HTag = V4HTML_HTag_Unknown ;
	if (p[bx] == UCEOS) return(V4HTML_GNP_EOP) ;	/* At end of page */
	if (vhp->Page[bx] != UClit('<'))			/* Is it a < ... > ? */
	 { for(;p[bx]!=UCEOS;bx++)
	    { if (p[bx] == UClit('<')) break ;
	    } ;
	   if (p[bx] == UCEOS && vhp->ContentType != V4HTML_ContentType_Text) return(V4HTML_GNP_EOP) ;	/* At end of page */
	   if (p[bx] != UClit('<') && vhp->ContentType != V4HTML_ContentType_Text)
	    { if (errs != NULL) v_Msg(NULL,errs,"HTMLSecStart") ;
	      return(V4HTML_GNP_Error) ;
	    } ;
	   vhp->curStart = vhp->curEnd+1 ;		/* Set new bounds */
	   vhp->curEnd = bx - 1 ;
	   return(V4HTML_GNP_PlainText) ;
	 } ;
/*	Starting on "<" - find boundaries */
	for(bx=bx+1;p[bx]!=UCEOS;bx++)
	 { if (p[bx] == UClit('>')) break ;			/* Got ending UClit('>') */
	 } ;
	vhp->curStart = vhp->curEnd+1 ; vhp->curEnd = bx ; /* Set new bounds */
	res = (p[vhp->curStart+1] == UClit('/') ? V4HTML_GNP_EndSeg : V4HTML_GNP_StartSeg) ;
	l = vhp->curEnd - vhp->curStart + 1 ;
	if (l >= V4HTML_TagNameMax-1) l = V4HTML_TagNameMax - 1 ;
	for(i=0;i<l;i++)
	 { int x ; x = i + vhp->curStart + (res == V4HTML_GNP_StartSeg ? 1 : 2) ;
	   if ((p[x] >= UClit('A') && p[x] <= UClit('Z')) || (p[x] >= UClit('a') && p[x] <= UClit('z')) || p[x] == UClit('-') || p[x] == UClit('.'))
	    { vhp->TagName[i] = toupper(p[x]) ; }
	    else { break ; } ;
	 } ;
	vhp->TagName[i] = UCEOS ;
	for(i=0;hti[i].HTag != -1;i++) { if (UCstrcmp(vhp->TagName,hti[i].HTMLTag) == 0) break ; } ;
	vhp->HTag = hti[i].HTag ;
	if (vhp->HTag == UNUSED)			/* If known tag then update vhp->Tag[] */
	 return(res) ;
	if (res == V4HTML_GNP_EndSeg)			/* Popping off segment ? */
	 { for(i=vhp->TagX;i>=0;i--)
	    { if (vhp->Tag[i].HTag != vhp->HTag) continue ;
	      vhp->TagX = i ; return(res) ;
	    } ;
	   
	   goto top ;					/* Most browsers ignore these - we will too! */
	   if (errs != NULL) v_Msg(NULL,errs,"HTMLSecEnd",vhp->HTag) ;
	   return(V4HTML_GNP_Error) ;			/* Could not find - return error */
	 } ;
/*	Here to add new level */
	if (vhp->HTag == V4HTML_HTag_Row)		/* If prior level also row then don't nest */
	 { if (vhp->Tag[vhp->TagX-1].HTag == V4HTML_HTag_Row) goto top ;
	 } ;
	if (vhp->TagX >= V4HTML_TagMax)		/* Too many nested levels? */
	 { if (errs != NULL) v_Msg(NULL,errs,"HTMLMaxLevels",V4HTML_TagMax) ;
	   return(V4HTML_GNP_Error) ;
	 } ;
	vhp->Tag[vhp->TagX].HTag = vhp->HTag ;		/* Set up new level */
	ZS(vhp->Tag[vhp->TagX].Name) ;
	vhp->Tag[vhp->TagX].curStart = vhp->curStart ;
	vhp->Tag[vhp->TagX].curEnd = vhp->curEnd ;
	vhp->Tag[vhp->TagX].ChildNum = UNUSED ;
	ZS(vhp->Tag[vhp->TagX].Value) ;
	vhp->TagX++ ;
	return(res) ;
}

/*	v4html_SectionBounds - Determines start/end boundaries of a section (e.g. <table>....</table>) */
/*	Call: res = v4html_SectionBounds( vhp , htag , errs )
	  where res = TRUE if OK, FALSE if errors,
		vhp is current page (vhp->SectionStart, vhp->SectionBytes updated on success),
		htag is section to bound,
		errs, if not NULL, updated with error				*/

LOGICAL v4html_SectionBounds(vhp,htag,errs)
  struct V4HTML__Page *vhp ;
  int htag ;
  UCCHAR *errs ;
{
  int i,res,tagx,rows,curcol,columns,nesting,saveStart,saveEnd ; UCCHAR *begin ;

	for(i=vhp->TagX-1;i>=0;i--) { if (vhp->Tag[i].HTag == htag) break ; } ;
	if (i < 0)
	 { if (errs != NULL) v_Msg(NULL,errs,"HTMLNotInHTag",htag) ; return(FALSE) ; } ;
	begin = &vhp->Page[vhp->Tag[i].curStart] ;	/* Save begin of bounded area */
	saveStart = vhp->curStart ; saveEnd = vhp->curEnd ;
	vhp->curEnd = vhp->Tag[i].curStart - 1 ;	/* Position to begin of section */
	vhp->TagX = i ;
	tagx = i ;					/* Current nesting level */
	if (htag == V4HTML_HTag_Table) { rows = 0 ; columns = 0 ; nesting = -1 ;} ;
	for(;;)
	 { res = v4html_GetNextPiece(vhp,errs) ;
	   switch(res)
	    {
	      case V4HTML_GNP_EOP:
		if (errs != NULL) v_Msg(NULL,errs,"HTMLHitEOP") ; res = FALSE ; goto done ;
	      case V4HTML_GNP_Error:
		res = FALSE ; goto done ;
	      case V4HTML_GNP_StartSeg:
		if (htag == V4HTML_HTag_Table)		/* If a table then track number of rows & columns */
		 { switch(vhp->HTag)
		    { 
		      case V4HTML_HTag_Table:		nesting++ ; break ;
		      case V4HTML_HTag_Row:		if (nesting == 0) { curcol = 0 ; rows++ ; } ; break ;
		      case V4HTML_HTag_Cell:		if (nesting == 0) curcol++ ; break ;
		    } ;
		 } ;
		break ;
	      case V4HTML_GNP_EndSeg:
		if (vhp->HTag == V4HTML_HTag_Table) nesting-- ;
		if (vhp->HTag == V4HTML_HTag_Row)
		 { if (curcol > columns) columns = curcol ; } ;
		if (vhp->HTag == htag && vhp->TagX == tagx)
		 { vhp->SectionStart = (int)(begin - vhp->Page) ; vhp->SectionBytes = (int)(&vhp->Page[vhp->curEnd] - begin) + 1 ;
		   if (htag == V4HTML_HTag_Table)	/* Tracking a table? */
		    { vhp->NumRows = rows ; vhp->NumColumns = columns ; } ;
		   res = TRUE ; goto done ;
		 } ;
	    } ;
	 } ;
done:
	vhp->curStart = saveStart ; vhp->curEnd = saveEnd ;
	return(res) ;
}

/*	v4html_GetCurrentId - Attempts to retrieve "id" at current position	*/
/*	Call: res = v4html_GetCurrentId( vhp , name , errs )
	  where res is TRUE if OK, FALSE if error,
		vhp is current page,
		name is updated with name,
		errs is updated with errors					*/

LOGICAL v4html_GetCurrentId(vhp,name,errs)
  struct V4HTML__Page *vhp ;
  UCCHAR *name ;
  UCCHAR *errs ;
{ 
  int res ;
  UCCHAR keyword[V4HTML_TagNameMax] ; UCCHAR value[V4HTML_InputValueMax] ;

	if (vhp->TagX <= 0) { if (errs != NULL) UCstrcpy(errs,UClit("Not positioned in page")) ; return(FALSE) ; } ;
	switch (vhp->Tag[vhp->TagX-1].HTag)
	 { case V4HTML_HTag_Area:
	   case V4HTML_HTag_Layer:
		for(res=UNUSED;;)
		 { res = v4html_KeywordValuePair(vhp,vhp->TagX-1,res,keyword,value,errs) ;
		   if (res == 0) goto gci1 ;
		   if (res == UNUSED) goto gci1 ;
		   if (UCstrcmp(keyword,UClit("ALT")) == 0) break ;
		 } ;
		UCstrcpy(name,value) ; return(TRUE) ;
	 } ;
gci1:	if (errs != NULL) v_Msg(NULL,errs,"HTMLNoId") ; return(FALSE) ;
}

/*	v4html_GetCurrentSrc - Attempts to retrieve "source" at current position	*/
/*	Call: res = v4html_GetCurrentSrc( vhp , name , errs )
	  where res is TRUE if OK, FALSE if error,
		vhp is current page,
		name is updated with name,
		errs is updated with errors					*/

LOGICAL v4html_GetCurrentSrc(vhp,name,errs)
  struct V4HTML__Page *vhp ;
  UCCHAR *name ;
  UCCHAR *errs ;
{ 
  int res ;
  UCCHAR keyword[V4HTML_TagNameMax] ; UCCHAR value[V4HTML_InputValueMax] ;

	if (vhp->TagX <= 0) { if (errs != NULL) UCstrcpy(errs,UClit("Not positioned in page")) ; return(FALSE) ; } ;
	switch (vhp->Tag[vhp->TagX-1].HTag)
	 { case V4HTML_HTag_Image:
	   case V4HTML_HTag_Layer:
		for(res=UNUSED;;)
		 { res = v4html_KeywordValuePair(vhp,vhp->TagX-1,res,keyword,value,errs) ;
		   if (res == 0) goto gci1 ;
		   if (res == UNUSED) goto gci1 ;
		   if (UCstrcmp(keyword,UClit("SRC")) == 0) break ;
		 } ;
		UCstrcpy(name,value) ; return(TRUE) ;
	 } ;
gci1:	if (errs != NULL) v_Msg(NULL,errs,"HTMLNoSRC") ; return(FALSE) ;
}

/*	v4html_GetHyperLink - Attempts to return URL enclosing current position */
/*	Call: res = v4html_GetHyperLink( vhp , url , errs )
	  where res is TRUE if OK, FALSE if error,
		vhp is current page,
		url is updated with link (as it appears in href='xxx', i.e. not fully formed!),
		errs is updated with any errors					*/

LOGICAL v4html_GetHyperLink(vhp,url,errs)
  struct V4HTML__Page *vhp ;
  UCCHAR *url ;
  UCCHAR *errs ;
{
  int i,res ;
  UCCHAR keyword[V4HTML_TagNameMax] ; UCCHAR value[V4HTML_InputValueMax] ;

	if (vhp->TagX <= 0) { if (errs != NULL) UCstrcpy(errs,UClit("No enclosing hyperlink")) ; return(FALSE) ; } ;
/*	If in an AREA then look for HREF attribute */
	if (vhp->Tag[vhp->TagX-1].HTag == V4HTML_HTag_Area)
	 { for(res=UNUSED;;)
	    { res = v4html_KeywordValuePair(vhp,vhp->TagX-1,res,keyword,value,errs) ;
	      if (res == 0) goto ghl1 ;
	      if (res == UNUSED) goto ghl1 ;
	      if (UCstrcmp(keyword,UClit("HREF")) == 0) break ;
	    } ;
	   UCstrcpy(url,value) ; return(TRUE) ;
	 } else if (vhp->Tag[vhp->TagX-1].HTag == V4HTML_HTag_Frame)
	 { for(res=UNUSED;;)
	    { res = v4html_KeywordValuePair(vhp,vhp->TagX-1,res,keyword,value,errs) ;
	      if (res == 0) goto ghl1 ;
	      if (res == UNUSED) goto ghl1 ;
	      if (UCstrcmp(keyword,UClit("SRC")) == 0) break ;
	    } ;
	   UCstrcpy(url,value) ; return(TRUE) ;
	 } ;
ghl1:	for(i=vhp->TagX-1;i>=0;i--)
	 { if (vhp->Tag[i].HTag == V4HTML_HTag_A) break ;
	 } ;
	if (i < 0) { if (errs != NULL) UCstrcpy(errs,UClit("No enclosing hyperlink")) ; return(FALSE) ; } ;
	for(res=UNUSED;;)
	 { res = v4html_KeywordValuePair(vhp,i,res,keyword,value,errs) ;
	   if (res == 0) return(FALSE) ;
	   if (res == UNUSED) return(FALSE) ;
	   if (UCstrcmp(keyword,UClit("HREF")) == 0) break ;
	 } ;
	UCstrcpy(url,value) ;
	return(TRUE) ;
}

/*	v4html_GetFormValue - Gets value for named item in currently loaded form	*/
/*	Call: res = v4html_GetFormValue( vhp , name , errs )
	  where res is pointer to value, NULL if error,
		vhp is current page (vhp->Input[] must be loaded!),
		name is pointer to name to be updated,
		errs, if not NULL, is updated with any error				*/

UCCHAR *v4html_GetFormValue(vhp,name,errs)
  struct V4HTML__Page *vhp ;
  UCCHAR *name ;
  UCCHAR *errs ;
{
  int i ;

	for(i=0;i<vhp->InputCount;i++)
	 { if (UCstrcmpIC(vhp->Input[i].UCName,name) == 0) return(vhp->Input[i].Value) ;
	 } ;
	if (errs != NULL) v_Msg(NULL,errs,"HTMLNoName",name) ;
	return(NULL) ;
}

/*	v4html_SetFormValue - Sets value for named item in currently loaded form	*/
/*	Call: res = v4html_SetFormValue( vhp , name , value , errs )
	  where res is TRUE if OK, FALSE if error,
		vhp is current page (vhp->Input[] must be loaded!),
		name is pointer to name to be updated,
		value is value to update with,
		errs, if not NULL, is updated with any error				*/

LOGICAL v4html_SetFormValue(vhp,name,value,errs)
  struct V4HTML__Page *vhp ;
  UCCHAR *name, *value ;
  UCCHAR *errs ;
{
  struct stat statbuf ;
  UCCHAR *fn ; int i,j,ifd,bytes,have ; char abuf[1024] ;

	if (UCstrlen(name) >= V4HTML_TagNameMax)
	 { if (errs != NULL) v_Msg(NULL,errs,"HTMLNameTooLong",name,V4HTML_TagNameMax) ;
	   return(FALSE) ;
	 } ;
	if (UCstrlen(value) >= V4HTML_InputValueMax)
	 { if (errs != NULL) v_Msg(NULL,errs,"HTMLValTooLong",value,V4HTML_InputValueMax) ;
	   return(FALSE) ;
	 } ;
	for(i=0;i<vhp->InputCount;i++)
	 { if (UCstrcmpIC(vhp->Input[i].UCName,name) == 0)
	    { UCstrcpy(vhp->Input[i].Value,value) ;
	      switch (vhp->Input[i].Type)
	       { case V4HTML_InputType_FileName:		/* If a file then read it in */
		   fn = v_UCLogicalDecoder(value,VLOGDECODE_Exists,0,errs) ; if (fn == NULL) return(FALSE) ;
		   if ((ifd = UCopen(fn,O_RDONLY|O_BINARY,0)) == -1)
		    { if (errs != NULL) v_Msg(NULL,errs,"HTMLFileErr",errno,value) ; return(FALSE) ; } ;
		   fstat(ifd,&statbuf) ;  vhp->Input[i].DataChar = statbuf.st_size ;
		   vhp->Input[i].Data = v4mm_AllocUC(vhp->Input[i].DataChar) ;
/*		   Read file in as stream of bytes and then convert each byte to UCCHAR to keep consistent with rest of page */
		   for(have=0;have<vhp->Input[i].DataChar;have+=bytes)
		    { bytes = read(ifd,abuf,sizeof abuf) ;
		      if (bytes == 0) break ;			/* Got EOF before we expected one? */
		      if (bytes == -1)
		       { if (errs != NULL) v_Msg(NULL,errs,"HTMLFileReadErr",vhp->Input[i].DataChar,have,bytes,value,errno) ; return(FALSE) ; } ;
		      for(j=0;j<bytes;j++) { vhp->Input[i].Data[have+j] = abuf[j] ; } ;
		    } ;
		   close(ifd) ; vhp->Input[i].DataChar = have ;
		   break ;
	       } ;
	      return(TRUE) ;
	    } ;
	 } ;
	if (errs != NULL) v_Msg(NULL,errs,"HTMLNoName",name) ;
	return(FALSE) ;
}

/*	v4html_LoadFormInfo - Loads vhp->Input[] with parameters for current form */
/*	Call: ok = v4html_LoadFormInfo( vhp , errs )
	  where ok = TRUE if OK, FALSE if problem
		vhp MUST be positioned at <form> or within form
		errs, if not NULL, updated with error				*/

LOGICAL v4html_LoadFormInfo(vhp,errs)
  struct V4HTML__Page *vhp ;
  UCCHAR *errs ;
{
  int saveTagX,saveStart,saveEnd,tx,i,k,res,checked ;
  UCCHAR keyword[V4HTML_TagNameMax] ; UCCHAR value[V4HTML_InputValueMax] ;

	for(tx=vhp->TagX-1;tx>=0;tx--)
	 { if (vhp->Tag[tx].HTag == V4HTML_HTag_Form) break ;
	 } ;
	if (tx < 0)
	 { FILE *fp ; fp = fopen("v4err.htm","w") ;
	   fprintf(fp,"Error: Not positioned at FORM\n") ;
	   fprintf(fp,"  BaseHostName: %s\n",UCretASC(vhp->BaseHostName)) ;
	   fprintf(fp,"  BasePath: %s\n",UCretASC(vhp->BasePath)) ;
	   fprintf(fp,"  PageType=%d, curStart=%d, curEnd=%d, TagName=%s, TaxX=%d\n",vhp->PageType,vhp->curStart,vhp->curEnd,UCretASC(vhp->TagName),vhp->TagX) ;
	   fprintf(fp,"  Page to follow\n%s\n",UCretASC(vhp->Page)) ;
	   fclose(fp) ;
	   if (errs != NULL) UCstrcpy(errs,UClit("Not positioned at FORM")) ; return(FALSE) ;
	 } ;
	saveTagX = vhp->TagX ;				/* Save this because </form> will back it off */
	saveStart = vhp->curStart ; saveEnd = vhp->curEnd ;

/*	Parse the <form ... > entry (look for Action=xxx) */
	for(res=UNUSED;;)
	 { res = v4html_KeywordValuePair(vhp,tx,res,keyword,value,errs) ;
	   if (res == 0) return(FALSE) ;
	   if (res == UNUSED) break ;
	   if (UCstrcmpIC(keyword,UClit("ACTION")) == 0) UCstrcpy(vhp->FormAction,value) ;
	   if (UCstrcmpIC(keyword,UClit("ENCTYPE")) == 0) UCstrcpy(vhp->FormEncType,value) ;
	 } ;

/*	Now scan ahead in page looking for all <input...> entries for this form */
	vhp->InputCount = 0 ;
	for(;vhp->TagX>=tx;)
	 { i = v4html_GetNextPiece(vhp,errs) ;
	   if (vhp->HTag == V4HTML_HTag_Form && i == V4HTML_GNP_EndSeg) break ;
	   if (i != V4HTML_GNP_StartSeg) continue ;
	   if (vhp->HTag == V4HTML_HTag_TextArea)	/* Handle text area right here */
	    { vhp->Input[vhp->InputCount].Type = V4HTML_InputType_TextArea ;
	      for(res=UNUSED;;)
	       { res = v4html_KeywordValuePair(vhp,vhp->TagX - 1,res,keyword,value,errs) ;
	         if (res == 0) return(FALSE) ; if (res == UNUSED) break ;
	         if (UCstrcmpIC(keyword,UClit("NAME")) == 0)
	          { 
	            if (UCstrlen(value) > V4HTML_TagNameMax - 1) value[V4HTML_TagNameMax-1] = UCEOS ;
/*		    Save all upper-case & given versions of name */
		    UCcnvupper(vhp->Input[vhp->InputCount].UCName,value,UCsizeof(vhp->Input[vhp->InputCount].UCName)) ;
		    UCstrcpy(vhp->Input[vhp->InputCount].Name,value) ;
		  } ;
	       } ;
	      for(;;)					/* Position to end- </textarea> */
	       { i = v4html_GetNextPiece(vhp,errs) ;
		 if (vhp->HTag == V4HTML_HTag_TextArea && i == V4HTML_GNP_EndSeg) break ;
	       } ;
	      if (UCstrlen(vhp->Input[vhp->InputCount].Name) > 0)		/* If no NAME=xxx given then ignore */
	       vhp->InputCount++ ;
	      continue ;
	    } ;

	   if (vhp->HTag != V4HTML_HTag_Input) continue ;
	   if (vhp->InputCount >= V4HTML_InputMax)
	    { if (errs != NULL) v_Msg(NULL,errs,"HTMLMaxInputs",V4HTML_InputMax) ; return(FALSE) ; } ;
/*	   Got <input xxxx> - parse it */
	   checked = FALSE ; vhp->Input[vhp->InputCount].Type = V4HTML_InputType_Text ; /* Assume TEXT until otherwise informed */
	   for(res=UNUSED;;)
	    { res = v4html_KeywordValuePair(vhp,vhp->TagX - 1,res,keyword,value,errs) ;
	      if (res == 0) return(FALSE) ;
	      if (res == UNUSED) break ;
/*	      See what we have & then do whatever */
	      if (UCstrcmpIC(keyword,UClit("TYPE")) == 0)
	       { for(k=0;value[k]!=UCEOS;k++) value[k] = toupper(value[k]) ;
	         if (UCstrcmpIC(value,UClit("SUBMIT")) == 0) { vhp->Input[vhp->InputCount].Type = V4HTML_InputType_Submit ; }
		  else if (UCstrcmpIC(value,UClit("HIDDEN")) == 0) { vhp->Input[vhp->InputCount].Type = V4HTML_InputType_Hidden ; }
		  else if (UCstrcmpIC(value,UClit("TEXT")) == 0) { vhp->Input[vhp->InputCount].Type = V4HTML_InputType_Text ; }
		  else if (UCstrcmpIC(value,UClit("RADIO")) == 0) { vhp->Input[vhp->InputCount].Type = V4HTML_InputType_Radio ; }
		  else if (UCstrcmpIC(value,UClit("RESET")) == 0) { vhp->Input[vhp->InputCount].Type = V4HTML_InputType_Reset ; }
		  else if (UCstrcmpIC(value,UClit("PASSWORD")) == 0) { vhp->Input[vhp->InputCount].Type = V4HTML_InputType_Password ; }
		  else if (UCstrcmpIC(value,UClit("FILE")) == 0) { vhp->Input[vhp->InputCount].Type = V4HTML_InputType_FileName ; }
		  else { vhp->Input[vhp->InputCount].Type = V4HTML_InputType_Unknown ; } ;
	       } else if (UCstrcmpIC(keyword,UClit("NAME")) == 0)
	       { 
	         if (UCstrlen(value) > V4HTML_TagNameMax - 1) value[V4HTML_TagNameMax-1] = UCEOS ;
		 UCcnvupper(vhp->Input[vhp->InputCount].UCName,value,UCsizeof(vhp->Input[vhp->InputCount].UCName)) ;
		 UCstrcpy(vhp->Input[vhp->InputCount].Name,value) ;
	       } else if (UCstrcmpIC(keyword,UClit("VALUE")) == 0)
	       { UCstrcpy(vhp->Input[vhp->InputCount].Value,value) ;
	       } else if (UCstrcmpIC(keyword,UClit("CHECKED")) == 0) checked = TRUE ;
	    } ;
	   if (vhp->Input[vhp->InputCount].Type == V4HTML_InputType_Radio)
	    { for(i=0;i<vhp->InputCount;i++)			/* If Radio buttons only keep one - that with CHECKED value */
	       { if (vhp->Input[i].Type != V4HTML_InputType_Radio || UCstrcmpIC(vhp->Input[i].UCName,vhp->Input[vhp->InputCount].UCName) != 0) continue ;
	         if (checked) UCstrcpy(vhp->Input[i].Value,vhp->Input[vhp->InputCount].Value) ;
		 vhp->InputCount-- ; break ;
	       } ;
	    } ;
	   if (UCstrlen(vhp->Input[vhp->InputCount].Name) > 0)		/* If no NAME=xxx given then ignore */
	    { vhp->InputCount++ ;
	    } else							/* Unless it's a SUBMIT then default name */
	    { if (vhp->Input[vhp->InputCount].Type == V4HTML_InputType_Submit)
	       { UCstrcpy(vhp->Input[vhp->InputCount].Name,UClit("SUBMIT")) ; vhp->InputCount++ ; } ;
	    }

	 } ;
	vhp->TagX = saveTagX ;				/* Restore vhp->TagX */
	vhp->curStart = saveStart ; vhp->curEnd = saveEnd ;
	return(TRUE) ;
}

/*	v4html_PostForm - Constructs URL for current form & posts (assumes vhp->Input[] has been set up) */
/*	Call: nvhp = v4html_PostForm( vhp , errs )
	  where nvhp = new vhp for return page (NULL if error),
		errs updated with error				*/

struct V4HTML__Page *v4html_PostForm(vhp,errs)
  struct V4HTML__Page *vhp ;
  UCCHAR *errs ;
{ 
  struct V4HTML__Page *vhp1 ;
  int i,j,argbytes ;
  UCCHAR path[1024], *args, tval[4096], boundary[64], contenttype[128], *v,*t,*b,*ip,*e ;

	vhp1 = malloc(sizeof *vhp1) ;			/* Allocate new vhp */
	*vhp1 = *vhp ;
	vhp1->PriorHTMLPage = vhp ;			/* Link to prior page */
	vhp1->Page = NULL ; vhp1->PageChars = 0 ;
	vhp1->PageType = V4HTML_PageType_Page ;
	for(i=0,j=0;i<vhp->InputCount;i++) { if (vhp->Input[i].Type == V4HTML_InputType_Submit) j++ ; } ;
	if (j != 1)
	 { if (errs != NULL) v_Msg(NULL,errs,"HTMLMultiSUBMIT",j) ; free(vhp1) ; return(NULL) ;
	 } ;
	if (v4html_ParseURL(vhp1,vhp->FormAction,vhp1->BaseHostName,vhp1->BasePath,path,UCsizeof(path),&vhp1->Port) != V4HTML_URL_HTTP)
	 { if (errs != NULL) v_Msg(NULL,errs,"HTMLBadURL",vhp1->BaseHostName,vhp1->BasePath) ;
	   free(vhp1) ; return(NULL) ;
	 } ;
	ip = v_URLAddressLookup(vhp1->BaseHostName,gpi->ctx->ErrorMsgAux) ;
	if (ip == NULL)
	 { if (errs != NULL) v_Msg(NULL,errs,"HTTPURLResolve",vhp1->BaseHostName) ;
	   free(vhp1) ; return(NULL) ;
	 } ;

/*	Build simple argument string if not doing multipart encoding */
	if (UCstrncmpIC(vhp->FormEncType,UClit("MULTIPART"),9) != 0)
	 {
	   for(argbytes=0,i=0;i<=vhp->InputCount;i++)		/* Determine rough size of form */
	    { argbytes += (UCstrlen(vhp->Input[i].Name) + UCstrlen(vhp->Input[i].Value) + 50) ; } ;
	   args = v4mm_AllocUC(argbytes) ; ZUS(args) ;
	   for(i=0;i<vhp->InputCount;i++)
	    { if (vhp->Input[i].Type == V4HTML_InputType_Reset || vhp->Input[i].Type == V4HTML_InputType_Unknown) continue ;
	      UCstrcat(args,vhp->Input[i].Name) ;
	      if (UCstrlen(vhp->Input[i].Value) > 0) UCstrcat(args,UClit("=")) ;
/*	      Convert value to HTML encoding */
	      for(v=vhp->Input[i].Value,t=tval;*v!=UCEOS;v++,t++)
	       { if (*v == UClit(' ')) { *t = UClit('+') ; continue ; } ;
	         if (*v < 128 && !(*v == UClit('"') || *v == UClit('&') || *v == UClit('<') || *v == UClit('>'))) { *t = *v ; continue ; } ;
	         *(t++) = UClit('&') ; *(t++) = UClit('#') ;
	         *(t++) = (*v / 100) + UClit('0') ; *(t++) = ((*v % 100) / 10) + UClit('0') ; *(t) = (*v % 10) + UClit('0') ;
	       } ;
	      *t = UCEOS ;  UCstrcat(args,tval) ;
	      UCstrcat(args,UClit("&")) ;
	    } ;
	   if (UCstrlen(args) > 0) args[UCstrlen(args)-1] = UCEOS ;	/* Get rid of trailing "&" */
	   if (v4html_RequestPage(vhp1,V4HTMP_Page_Post,ip,vhp1->Port,path,args,0,NULL,errs) == 0)
	    { v4mm_FreeChunk(args) ; free(vhp1) ; return(NULL) ; } ;
	   v4mm_FreeChunk(args) ; return(vhp1) ;			/* Return pointer to current vhp */
	 } ;

/*	Have to build up multi-part form to send to host */
	v4html_MakeBoundary(vhp,boundary,sizeof boundary) ;	/* Create suitable boundary */
	for(argbytes=0,i=0;i<=vhp->InputCount;i++)		/* Determine rough size of form */
	 { argbytes += (UCstrlen(boundary) + 2 + UCstrlen(vhp->Input[i].Name) + UCstrlen(vhp->Input[i].Value) + vhp->Input[i].DataChar + 100) ; } ;
	args = v4mm_AllocUC(argbytes) ; ZUS(args) ;/* Allocate argument buffer */
	for(argbytes=0,b=args,i=0;i<vhp->InputCount;i++)
	 { switch (vhp->Input[i].Type)
	    { default:
		v_Msg(NULL,b,"@--%1U" TXTEOL "nContent-Disposition: form-data; name=\"%2U\"" TXTEOL TXTEOL "%3U" TXTEOL TXTEOL ,boundary,vhp->Input[i].Name,vhp->Input[i].Value) ;
		argbytes += UCstrlen(b) ; b += UCstrlen(b) ;
		break ;
	      case V4HTML_InputType_Reset:
	      case V4HTML_InputType_Unknown:	break ;
	      case V4HTML_InputType_FileName:
		e = UCstrrchr(vhp->Input[i].Value,'.') ;
		v_FileExtToContentType(e,contenttype,UCsizeof(contenttype),tval) ;
		v_Msg(NULL,b,"@--%1U" TXTEOL "Content-Disposition: form-data; name=\"%2U\"; filename=\"%3U\"" TXTEOL "Content-Type: %4U" TXTEOL TXTEOL,boundary,vhp->Input[i].Name,vhp->Input[i].Value,contenttype) ;
		argbytes += UCstrlen(b) ; b += UCstrlen(b) ;
		memcpy(b,vhp->Input[i].Data,vhp->Input[i].DataChar*sizeof(UCCHAR)) ; argbytes += vhp->Input[i].DataChar ;
		b += vhp->Input[i].DataChar ; *b = UCEOS ;
		UCstrcat(b,UCTXTEOL) ; argbytes += UCstrlen(UCTXTEOL) ; b += UCstrlen(UCTXTEOL) ;
		break ;
	    } ;
	 } ;
	v_Msg(NULL,b,"@--%1U--" TXTEOL,boundary) ; argbytes += UCstrlen(b) ; b += UCstrlen(b) ;
	if (v4html_RequestPage(vhp1,V4HTMP_Page_Post,ip,vhp1->Port,path,args,argbytes,boundary,errs) == 0)
	 { v4mm_FreeChunk(args) ; free(vhp1) ; return(NULL) ; } ;

	v4mm_FreeChunk(args) ; return(vhp1) ;
}

void v4html_MakeBoundary(vhp,res,reslen)
  struct V4HTML__Page *vhp ;
  UCCHAR *res ; int reslen ;
{
  UCCHAR boundary[64] ; int i,j ;

	for(;;)
	 { UCsprintf(boundary,UCsizeof(boundary),UClit("---------------------------%08x%08x"),clock()-time(NULL),time(NULL)+clock()) ;
	   boundary[45] = UCEOS ;
	   for(i=0;i<vhp->InputCount;i++)
	    { if (UCstrstr(vhp->Input[i].Value,boundary) != NULL) break ;
	      if (vhp->Input[i].Data == NULL) continue ;
	      for(j=0;j<vhp->Input[i].DataChar;j++)
	       { if (vhp->Input[i].Data[j] != boundary[0]) continue ;
	         if (UCstrcmp(&vhp->Input[i].Data[j],boundary) == 0) break ;
	       } ; if (j < vhp->Input[i].DataChar) break ;
	    } ; if (i >= vhp->InputCount) break ;
	 } ;
	UCstrcpy(res,boundary) ;
	return ;
}

/*	v4html_PositionInPage - Moves to requested position in current page	*/
/*	Call: res = v4html_PositionInPage( vhp , direction , htag , name , errs )
	  where res is TRUE if OK, FALSE if problem,
		direction is one of V4HTML_Pos_xxx,
		htag is V4HTML_HTag_xxx value (UNUSED for any),
		name is pointer to UPPERCASE string for section id,
		  if htag=V4HTML_HTag_Text then name is text string to match,
		  if htag=V4HTML_HTag_RegExp then name is (regex_t *)pattern,
		errs, if not NULL, is updated with problem			*/

#define V4HTML_Pos_Forward 1
#define V4HTML_Pos_Reverse 2
#define V4HTML_Pos_TopOfPage 3
#define V4HTML_Pos_BottomOfPage 4
#define V4HTML_Pos_Label 5

LOGICAL v4html_PositionInPage(vhp,direction,htag,name,errs)
  struct V4HTML__Page *vhp ;
  int direction, htag ;
  UCCHAR *name ;
  UCCHAR *errs ;
{
  UCCHAR keyword[V4HTML_TagNameMax] ; UCCHAR value[V4HTML_InputValueMax] ;
  UCCHAR *b, *e, s[2] ; int res ;

	switch (direction)
	 { default:
		if (errs != NULL) v_Msg(NULL,errs,"HTMLBadDIR",direction) ; return(FALSE) ;
	   case V4HTML_Pos_Forward:
		for(;;)
		 { res = v4html_GetNextPiece(vhp,errs) ;
		   if (res == V4HTML_GNP_Error) return(FALSE) ;
		   if (res == V4HTML_GNP_EOP)
		    { if (errs != NULL) v_Msg(NULL,errs,"HTMLHitEOP1") ; return(FALSE) ; } ;
		   if (res == V4HTML_GNP_EndSeg) continue ;
		   if (htag != V4HTML_HTag_Any)		/* Matching Any segment type? */
		    { if ((htag == V4HTML_HTag_Text || htag == V4HTML_HTag_RegExp) ? res != V4HTML_GNP_PlainText : TRUE)
		       { if (vhp->HTag != htag) continue ; } ;
		    } ;
		   if (name == NULL)			/* No text/id match wanted - positioned where we want */
		    { if (vhp->curStart > vhp->curEnd) continue ; /* Except if no text (i.e. have <tag><tag> with nothing in between) */
		      return(TRUE) ;
		    } ;
		   if (htag == V4HTML_HTag_RegExp)	/* Look for regular expression */
		    { b = &vhp->Page[vhp->curStart] ; e = &vhp->Page[vhp->curEnd+1] ; s[0] = *e ; *e = UCEOS ;
		      res = vregexp_RegExec((regex_t *)name,UCretASC(b),0,NULL,0) ; *e = s[0] ; 
		      if (!res) return(TRUE) ;
		      continue ;			/* Pattern does not match - keep plugging */
		    } ;
		   if (htag == V4HTML_HTag_Text)	/* Tag matches - now try text match */
		    { b = &vhp->Page[vhp->curStart] ; e = &vhp->Page[vhp->curEnd+1] ; s[0] = *e ; *e = UCEOS ;
		      if (UCstrstr(b,name) != NULL) { *e = s[0] ; return(TRUE) ; } ;
		      *e = s[0] ; continue ;		/* Text does not match - keep plugging */
		    } ;
		   for(res=UNUSED;;)
		    { res = v4html_KeywordValuePair(vhp,vhp->TagX-1,res,keyword,value,errs) ;
		      if (res == 0) return(FALSE) ;
		      if (res == UNUSED) break ;
		      if (UCstrcmpIC(keyword,UClit("ID")) == 0)	/* Does id=xxx match name we want? */
		       { if (UCstrcmpIC(value,name) == 0) return(TRUE) ; } ;
		      if (UCstrcmpIC(keyword,UClit("NAME")) == 0)	/* Does id=xxx match name we want? */
		       { if (UCstrcmpIC(value,name) == 0) return(TRUE) ; } ;
		      if (UCstrcmpIC(keyword,UClit("ALT")) == 0)	/* Does id=xxx match name we want? */
		       { if (UCstrcmpIC(value,name) == 0) return(TRUE) ; } ;
		    } ;
		 } ;
	   case V4HTML_Pos_Reverse:
	   case V4HTML_Pos_Label:
		vhp->curStart = 0 ; vhp->curEnd = -1 ; vhp->TagX = 0 ;	/* Position to top of page */
		for(;;)
		 { res = v4html_GetNextPiece(vhp,errs) ;
		   if (res == V4HTML_GNP_Error) return(FALSE) ;
		   if (res == V4HTML_GNP_EOP)
		    { if (errs != NULL) v_Msg(NULL,errs,"HTMLHitEOP2",name) ; return(FALSE) ; } ;
		   if (res == V4HTML_GNP_PlainText || res == V4HTML_GNP_EndSeg) continue ;
		   if (vhp->HTag == UNUSED) continue ;	/* Unrecognized entry */
		   for(res=UNUSED;;)
		    { res = v4html_KeywordValuePair(vhp,vhp->TagX-1,res,keyword,value,errs) ;
		      if (res == 0) return(FALSE) ;
		      if (res == UNUSED) break ;
		      if (UCstrcmpIC(keyword,UClit("NAME")) == 0)	/* Does id=xxx match name we want? */
		       { if (UCstrcmpIC(value,name) == 0) return(TRUE) ; } ;
		    } ;
		 } ;
	   case V4HTML_Pos_TopOfPage:
		vhp->curStart = 0 ; vhp->curEnd = -1 ; vhp->TagX = 0 ;	/* Position to top of page */
		return(TRUE) ;
	   case V4HTML_Pos_BottomOfPage:
		vhp->curStart = (vhp->curEnd = UCstrlen(vhp->Page)) ; return(TRUE) ;
	 } ;
}


/*	v4html_GoBackPage - Goes back to prior page		*/
/*	Call: pvhp = v4html_GoBackPage( vhp , errs )
	  where pvhp = pointer to prior page (NULL if problem),
		vhp is current page,
		errs is updated with problem			*/

struct V4HTML__Page *v4html_GoBackPage(vhp,errs)
  struct V4HTML__Page *vhp ;
  UCCHAR *errs ;
{
  struct V4HTML__Page *pvhp ;

	if ((pvhp=vhp->PriorHTMLPage) == NULL)
	 { if (errs != NULL) v_Msg(NULL,errs,"HTMLNoPriorPage") ; return(NULL) ; } ;
	if (pvhp->Page != vhp->Page && vhp->Page != NULL)
	 free(vhp->Page) ;			/* If text of current page not same as prior then free it up */
	free(vhp) ;				/* Free up current page */
	return(pvhp) ;				/* Return prior page */
}

/*	v4html_PushSection - Creates "new" page from delimited section in current page	*/
/*	Call: nvhp = v4html_PushSection( vhp , errs )
	  where nvhp = pointer to new page (NULL if errors),
		errs, if not NULL, updated with error					*/

struct V4HTML__Page *v4html_PushSection(vhp,errs)
  struct V4HTML__Page *vhp ;
  UCCHAR *errs ;
{
  struct V4HTML__Page *vhp1 ;

	if (vhp->SectionBytes <= 0)
	 { if (errs != NULL) v_Msg(NULL,errs,"HTMLNoSection") ; return(NULL) ; } ;
	vhp1 = malloc(sizeof *vhp1) ;		/* Allocate new vhp */
	*vhp1 = *vhp ;
	vhp1->PriorHTMLPage = vhp ;		/* Don't have to reallocate data for page, just reposition */
	vhp1->PageType = V4HTML_PageType_Section ;
	vhp1->Page = v4mm_AllocUC(vhp->SectionBytes+1) ; vhp1->MaxPageChars = (vhp1->PageChars = vhp->SectionBytes) ;
	memcpy(vhp1->Page,&vhp->Page[vhp->SectionStart],vhp->SectionBytes) ;
	vhp1->Page[vhp->SectionBytes] = UCEOS ;
	vhp1->SectionStart = (vhp1->SectionBytes = 0) ;
	vhp1->curStart = 0 ; vhp1->curEnd = -1 ; vhp1->HTag = V4HTML_HTag_Unknown ;
	vhp1->TagX = 0 ; vhp1->InputCount = 0 ;
	return(vhp1) ;
}

/*	v4html_PositionRowColumn - "Positions" to a row and/or column in a V4HTML_PageType_Section page */
/*	Call: res = v4html_PositionRowColumn( vhp , row , column , errs )
	  where res is TRUE if OK, FALSE if problem,
		row is row to position to (1 = first row),
		column is column within row (1 = first, UNUSED to just position to a row),
		errs, if not NULL, is updated with error			*/

LOGICAL v4html_PositionRowColumn(vhp,row,column,errs)
  struct V4HTML__Page *vhp ;
  int row, column ;
  UCCHAR *errs ;
{
  int currow,curcol,nesting,res ;

	if (row < 1 || row > vhp->NumRows || column > vhp->NumColumns)
	 { if (errs != NULL) v_Msg(NULL,errs,"HTMLBadRowCol",row,column) ;
	   return(FALSE) ;
	 } ;
	v4html_PositionInPage(vhp,V4HTML_Pos_TopOfPage,UNUSED,NULL,errs) ;
	for(currow=0,nesting=-1;currow<row;)			/* First position to the proper row */
	 { res = v4html_GetNextPiece(vhp,errs) ;
	   switch(res)
	    { case V4HTML_GNP_EOP:	if (errs != NULL) v_Msg(NULL,errs,"HTMLHitEOP3") ; return(FALSE) ;
	      case V4HTML_GNP_Error:	return(FALSE) ;
	      case V4HTML_GNP_StartSeg:	switch(vhp->HTag)
					 { case V4HTML_HTag_Table:	nesting++ ; break ;
					   case V4HTML_HTag_Row:	if (nesting == 0) currow++ ; break ;
					 } ; break ;
	      case V4HTML_GNP_EndSeg:	if (vhp->HTag == V4HTML_HTag_Table) nesting-- ;
	    } ;
	 } ;
	vhp->CurRow = row ;
	if (column < 1)						/* Only want position to row */
	 { vhp->CurColumn = UNUSED ; return(TRUE) ; } ;
	for(curcol=0,nesting=0;curcol<column;)
	 { res = v4html_GetNextPiece(vhp,errs) ;
	   switch(res)
	    { case V4HTML_GNP_EOP:	if (errs != NULL) v_Msg(NULL,errs,"HTMLHitEOP3") ; return(FALSE) ;
	      case V4HTML_GNP_Error:	return(FALSE) ;
	      case V4HTML_GNP_StartSeg:	switch(vhp->HTag)
					 { case V4HTML_HTag_Table:	nesting++ ; break ;
					   case V4HTML_HTag_Cell:	if (nesting == 0) curcol++ ; break ;
					 } ; break ;
	      case V4HTML_GNP_EndSeg:	if (vhp->HTag == V4HTML_HTag_Table) nesting-- ;
	    } ;
	 } ;
	vhp->CurColumn = column ;
	return(TRUE) ;
}

/*	v4html_FollowHyperLink - Follows hyperlink to wherever		*/
/*	Call: newvhp = v4html_FollowHyperLink( vhp, link , user , password , errs , mode , newvhp)
	  where newvhp is point to new vhp if OK, NULL if error,
		vhp is current page (NULL if first call),
		link is hyperlink (assumed in "raw" fashion unless firsttime is TRUE),
		user & password are strings containing authentication user name & password (empty string if not used),
		errs is updated with any error,
		mode is page access (V4HTMP_Page_xxx),
		newvhp is pointer to new vhp to fill, if NULL then will auto-allocte		*/

struct V4HTML__Page *v4html_FollowHyperLink(vhp,link,user,password,errs,mode,newvhp)
  struct V4HTML__Page *vhp ;
  UCCHAR *link ;
  UCCHAR *user, *password ;
  UCCHAR *errs ;
  int mode ;
  struct V4HTML__Page *newvhp ;
{
  struct V4HTML__Page *vhp1 ;
  UCCHAR *ip, path[V4HTML_MaxURL_Length] ;
  int res ;

	if (vhp == NULL)				/* If this is first call then decode as necessary */
	 { vhp = malloc(sizeof *vhp) ;
	   memset(vhp,0,sizeof *vhp) ; vhp->curEnd = -1 ;
	   if (user != NULL) UCstrcpy(vhp->UserName,user) ; if (password != NULL) UCstrcpy(vhp->UserPassword,password) ;
	   switch (v4html_ParseURL(vhp,link,vhp->BaseHostName,vhp->BasePath,path,UCsizeof(path),&vhp->Port))
	    { default:
		if (errs != NULL) v_Msg(NULL,errs,"HTTPBadURL",link) ; return(NULL) ;
	      case V4HTML_URL_HTTP:
		vhp->PageType = V4HTML_PageType_Page ; break ;	/* Assume this for now */
	      case V4HTML_URL_FilePath:
		vhp->PageType = V4HTML_PageType_File ; break ;
	    } ;
	   ip = v_URLAddressLookup(vhp->BaseHostName,gpi->ctx->ErrorMsgAux) ;
	   if (ip == NULL && vhp->PageType != V4HTML_PageType_File)
	    { if (errs != NULL) v_Msg(NULL,errs,"HTTPURLResolve",vhp->BaseHostName) ; free(vhp) ; return(NULL) ; } ;
	   if (v4html_RequestPage(vhp,mode,ip,vhp->Port,path,NULL,0,NULL,errs) == 0) { free(vhp) ; return(NULL) ; } ;
	   return(vhp) ;			/* Return pointer to current vhp */
	 } ;
/*	Not first time - see if link is to position in current page */
	if (link[0] == UClit('#'))
	 { vhp1 = malloc(sizeof *vhp1) ;	/* Allocate new vhp */
	   *vhp1 = *vhp ;
	   vhp1->PriorHTMLPage = vhp ;		/* Don't have to reallocate data for page, just reposition */
	   vhp1->PageType = V4HTML_PageType_Goto ;
	   res = v4html_PositionInPage(vhp1,V4HTML_Pos_Label,0,&link[1],errs) ;
	   if (!res)				/* Could not find label - zap new page & return NULL */
	    { free(vhp1) ; return(NULL) ; } ;
	   return(vhp1) ;
	 } ;
/*	Linking to new URL - get full URL address from current base */
	vhp1 = (newvhp == NULL ? malloc(sizeof *vhp1) : newvhp) ;	/* Allocate new vhp ?*/
	*vhp1 = *vhp ;
	vhp1->PriorHTMLPage = vhp ;		/* Link to prior page */
	vhp1->Page = NULL ; vhp1->PageChars = 0 ;
	if (user != NULL) UCstrcpy(vhp1->UserName,user) ; if (password != NULL) UCstrcpy(vhp1->UserPassword,password) ;
	switch (v4html_ParseURL(vhp1,link,vhp1->BaseHostName,vhp1->BasePath,path,UCsizeof(path),&vhp1->Port))
	 { default:
	     if (errs != NULL) v_Msg(NULL,errs,"HTTPBadURL",link) ; free(vhp1) ; return(NULL) ;
	   case V4HTML_URL_HTTP:
	     vhp1->PageType = V4HTML_PageType_Page ; break ;
	   case V4HTML_URL_FilePath:
	     vhp1->PageType = V4HTML_PageType_File ; break ;
	 } ;
	ip = v_URLAddressLookup(vhp1->BaseHostName,gpi->ctx->ErrorMsgAux) ;
	if (ip == NULL)
	 { if (errs != NULL) v_Msg(NULL,errs,"HTTPURLResolve",vhp1->BaseHostName) ;
	   free(vhp1) ; return(NULL) ;
	 } ;
	if (v4html_RequestPage(vhp1,mode,ip,vhp1->Port,path,NULL,0,NULL,errs) == 0)
	 { free(vhp1) ; return(NULL) ; } ;
	return(vhp1) ;			/* Return pointer to current vhp */
}



/*	v4html_RequestPage - Get/Post page and loads into vhp->Page		*/
/*	Call: res = v4html_RequestPage( vhp , mode, ipaddress , port , path , arguments, arglen, boundary , errs )
	  where res is number of bytes in page if ok, FALSE if problem,
		vhp is current page,
		mode is get/request mode - V4HTMP_Page_xxx,
		ipaddress is string version of ip address,
		port is port to connect through,
		path is GET/POST path name,
		arguments are POST command arguments to server (NULL on GET),
		arglen is 0 if arguments are text string, nonzero of arguments is (binary) multi-part,
		boundary, if not NULL is multi-part boundary string,
		errs, if not NULL, is updated with error message	*/

int v4html_RequestPage(vhp,mode,ipaddress,port,path,arguments,arglen,boundary,errs)
  struct V4HTML__Page *vhp ;
  int mode ;
  UCCHAR *ipaddress ;
  int port ;
  UCCHAR *path, *arguments ; int arglen ; UCCHAR *boundary ;
  UCCHAR *errs ;
{
  struct sockaddr_in sin ;
  fd_set sset,eset ;			/* Socket set */
  struct timeval tev ;
  int Socket,wait,res,ok,i,tries,status,bol,cmax,chunk,sent,chkchunk,chklen,chkigneol ; UCCHAR *b,*e ; char abuf[64] ;
#define HTTP_HeaderMax 100
  struct V4HTML__Page *rvhp ;
  UCCHAR *Headers[HTTP_HeaderMax], *beginofline ; int headerline ;
  UCCHAR htmlstuff[V4HTML_MaxURL_Length], basicauth[256], newuri[V4HTML_MaxURL_Length], httperr[256], emsg[256] ;
  struct stat statbuf ;

/*	If from local file then grab here */
	if (vhp->PageType == V4HTML_PageType_File)
	 { UCCHAR ifile[V_FileName_Max] ;
	   struct UC__File UCFile ; UCCHAR errbuf[512],*pp ;
	   UCstrcpy(ifile,vhp->BasePath) ; UCstrcat(ifile,vhp->BaseHostName) ;
	   if (!v_UCFileOpen(&UCFile,ifile,UCFile_Open_Read,TRUE,errbuf,0)) { v_Msg(NULL,errs,"HTMLLocalFile",errbuf) ; return(0) ; } ;
	   fstat(UCFile.fd,&statbuf) ;		/* Get size of the file */
	   if (statbuf.st_size * sizeof(UCCHAR) > V4HTML_MaxFileBytes)
	    { if (errs != NULL) v_Msg(NULL,errs,"HTMLLclFileMax",ifile,statbuf.st_size,V4HTML_MaxFileBytes) ;
	      v_UCFileClose(&UCFile) ; return(0) ;
	    } ;
	   vhp->Page = v4mm_AllocUC(statbuf.st_size) ;
	   UCFile.filetype = V4L_TextFileType_UTF8 ;	/* Force file type to UTF8 */
	   for(pp=vhp->Page;;pp+=UCstrlen(pp))
	    { if (v_UCReadLine(&UCFile,UCRead_UC,pp,statbuf.st_size,errbuf) < 0) break ;
	    } ;
	   v_UCFileClose(&UCFile) ;
/*	   If file starts with <html> then assume HTML, otherwise unknown */
	   vhp->ContentType = V4HTML_ContentType_Undef ;
	   if (UCstrncmp(UClit("<html"),vhp->Page,5) == 0 || UCstrncmp(UClit("<HTML"),vhp->Page,5) == 0 || UCstrncmp(UClit("HTTP/"),vhp->Page,5) == 0 || UCstrncmp(UClit("http/"),vhp->Page,5) == 0)
	    { vhp->ContentType = V4HTML_ContentType_HTML ; }
	    else { cmax = (statbuf.st_size < 250 ? statbuf.st_size : 250) ;
		   for(i=0;i<cmax;i++) { if (vhp->Page[i] <= 0 || vhp->Page[i] > 126) break ; } ;
		   if (i >= cmax) vhp->ContentType = V4HTML_ContentType_Text ;
		 } ;
	   return(statbuf.st_size) ;
	 } ;

	SOCKETINIT
	Socket = UNUSED ;
	ok = ((Socket = socket(AF_INET, SOCK_STREAM,0)) >= 0) ;
	memset(&sin,0,sizeof sin) ;
	sin.sin_family = AF_INET ; sin.sin_port = htons((u_short)port) ;
	if (ok)
	 { sin.sin_addr.s_addr = inet_addr(UCretASC(ipaddress)) ; ok = (sin.sin_addr.s_addr != -1) ;
	 } ;
	if (ok)
	 { ok = (connect(Socket,(struct sockaddr *)&sin,sizeof sin) >= 0) ;
	 } ;
	if (!ok)
	 { if (errs != NULL) v_Msg(NULL,errs,"HTMLSocketErr",NETERROR,ipaddress,port,Socket) ;
	   if (Socket != UNUSED) SOCKETCLOSE(Socket) ;
	   return(FALSE) ;
	 } ;
	if (UCempty(path)) path = UClit("/") ;		/* Default path if none given */
	if (UCempty(vhp->UserName) && UCempty(vhp->UserPassword)) { ZUS(basicauth) ; }
	 else { UCCHAR namepassword[132] ; UCCHAR encode[200] ;
		UCstrcpy(namepassword,vhp->UserName) ; UCstrcat(namepassword,UClit(":")) ; UCstrcat(namepassword,vhp->UserPassword) ;
		if (!VUC_Encode64(namepassword,UCstrlen(namepassword),encode,UCsizeof(encode),emsg))
		 { if (errs != NULL) v_Msg(NULL,errs,"HTMLBASE64",emsg) ; return(FALSE) ; } ;
		v_Msg(NULL,basicauth,"@Authorization: Basic %1U" TXTEOL,encode) ;
	      } ;
	switch (mode)
	 { default:
		if (errs != NULL) v_Msg(NULL,errs,"HTMLInvRequest",mode) ;
		SOCKETCLOSE(Socket) ; return(FALSE) ;
	   case V4HTMP_Page_Get:
		v_Msg(NULL,htmlstuff,"@GET %1U HTTP/1.1" TXTEOL
"Accept: */*" TXTEOL
"Accept-Language: en-us" TXTEOL
"Accept-Encoding: gzip, deflate" TXTEOL
"%2UHost: %3U:%4d" TXTEOL
"Connection: Close" TXTEOL
"User-Agent: Mozilla/4.0 (compatible; MSIE 5.0; Windows NT; DigExt)" TXTEOL TXTEOL,path,basicauth,vhp->BaseHostName,port) ;
		break ;
	   case V4HTMP_Page_Head:
		v_Msg(NULL,htmlstuff,"@HEAD %1U HTTP/1.1" TXTEOL
"Accept: */*" TXTEOL
"Accept-Language: en-us" TXTEOL
"Accept-Encoding: gzip, deflate" TXTEOL
"%2UHost: %3U:%4d" TXTEOL
"Connection: Close" TXTEOL
"User-Agent: Mozilla/4.0 (compatible; MSIE 5.0; Windows NT; DigExt)" TXTEOL TXTEOL,path,basicauth,vhp->BaseHostName,port) ;
		break ;
	   case V4HTMP_Page_XMLRPC:
		v_Msg(NULL,htmlstuff,"@POST %1U HTTP/1.1" TXTEOL
"%2UHost: %3U:%4d" TXTEOL
"Content-Type: text/xml" TXTEOL
"Content-Length: %d" TXTEOL TXTEOL,path,basicauth,vhp->BaseHostName,port,UCstrlen(arguments)) ;
		break ;
	   case V4HTMP_Page_Post:
		if (arglen == 0)			/* If arglen is 0 then arguments are simple text string */
		 { v_Msg(NULL,htmlstuff,"@POST %1U HTTP/1.1" TXTEOL
"Accept: */*" TXTEOL
"Accept-Language: en-us" TXTEOL
"Accept-Encoding: gzip, deflate" TXTEOL
"Content-Type: application/x-www-form-urlencoded" TXTEOL
"%2UHost: %3U:%4d" TXTEOL
"Connection: Close" TXTEOL
"Content-Length: %d" TXTEOL
"User-Agent: Mozilla/4.0 (compatible; MSIE 5.0; Windows NT; DigExt)" TXTEOL TXTEOL,path,basicauth,vhp->BaseHostName,port,UCstrlen(arguments)) ;
		 } else					/* arglen not 0 - arguments in multi-part format */
		 { 
		   v_Msg(NULL,htmlstuff,"@POST %1U HTTP/1.1" TXTEOL
"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-excel, application/msword, application/vnd.ms-powerpoint, */*" TXTEOL
"Accept-Language: en-us" TXTEOL
"Content-Type: multipart/form-data; boundary=%2U" TXTEOL
"Accept-Encoding: gzip, deflate" TXTEOL
"User-Agent: Mozilla/4.0 (compatible; MSIE 5.0; Windows NT; DigExt)" TXTEOL
"%3UHost: %4U:%5d" TXTEOL
"Content-Length: %6d" TXTEOL
"Connection: Keep-Alive" TXTEOL
"Cache-Control: no-cache" TXTEOL TXTEOL,path,boundary,basicauth,vhp->BaseHostName,port,arglen) ;
		 } ;
		break ;
	 } ;
	if (Echo & V4HTML_Trace_Request)
	 { printf("V4-Trace-HTML: Request to %s:%d\n",UCretASC(ipaddress),port) ;
	   printf("%s",UCretASC(htmlstuff)) ;
	   if (arguments != NULL) printf("V4-Trace-Args: *%s*\n",UCretASC(arguments)) ;
	   printf("V4-Trace-HTML: End of request\n") ;
	 } ;
	res = send(Socket,UCretASC(htmlstuff),UCstrlen(htmlstuff),0) ;
	v4html_BytesSend += res ;
	if (res <= 0)
	 { if (errs != NULL) v_Msg(NULL,errs,"HTTPSendHdr",NETERROR) ;
	   SOCKETCLOSE(Socket) ; return(FALSE) ;
	 } ;
	if (arguments != NULL && arglen == 0)
	 { res = send(Socket,UCretASC(arguments),UCstrlen(arguments),0) ;
	   v4html_BytesSend += res ;
	 } else
	 { char abuf[4096] ;
	   chunk = (arglen < 4096 ? arglen : 4096) ;
	   for(sent=0,res=1;res>0&&sent<arglen;)
	    { int i ; for(i=0;i<chunk;i++) { abuf[i] = arguments[sent+i] ; } ;
	      res = send(Socket,abuf,chunk,0) ; if (res > 0) { sent+=res ; v4html_BytesSend += res ; } ;
	      if (chunk > arglen-sent) chunk = arglen-sent ;
	    } ;
	 } ;
	if (res <= 0)
	 { if (errs != NULL) v_Msg(NULL,errs,"HTTPSendGetPost",NETERROR) ;
	   SOCKETCLOSE(Socket) ; return(FALSE) ;
	 } ;
/*	Now grab results & put into file */
	if (vhp->Page == NULL) { vhp->Page = v4mm_AllocUC(vhp->MaxPageChars = 5000) ; } ;
grab_results:
	wait = 60 ;						/* Initial wait - up to 60 seconds */
	headerline = 0 ;					/* Current header line (UNUSED if in body) */
	beginofline = vhp->Page ;
	status = 200 ;						/* Assume success */
	bol = FALSE ;						/* Tracks begin of line in body */
	chkchunk = UNUSED ; chkigneol = FALSE ;
	ZS(newuri) ;
	vhp->ContentType = V4HTML_ContentType_HTML ;		/* Assume HTML until otherwise */
	vhp->ContentLength = V4LIM_BiggestPositiveInt ;		/* Default length is very large */
	for(i=0,tries=0;tries<10&&i<vhp->ContentLength;i++)
	 { if (TRUE)
	    { FD_ZERO(&sset) ; FD_SET(Socket,&sset) ; FD_ZERO(&eset) ; FD_SET(Socket,&eset) ;
	      if (wait > 0)
	       { tev.tv_sec = wait ; tev.tv_usec = 0 ;		/* Wait a little longer for first byte */
	       } else
	       { tev.tv_sec = 0 ; tev.tv_usec = 500000 ;
	       } ;
	      res = select(FD_SETSIZE,&sset,NULL,&eset,&tev) ;	/* Wait for something to happen */
	      if (res <= 0)
	       { if (status == 100)				/* Status=100 means hang around - rest might take a while */
		  { wait = 30 ;  tries++ ; continue ; } ;
	         if (i > 30 && vhp->ContentLength == V4LIM_BiggestPositiveInt) /* If got something but no Content-Length then go with it */
		  break ;
		 if (i > 15)					/* Sometimes Content-Length is off - look for end */
		  { vhp->Page[i] = UCEOS ;
		    if (UCstrstr(&vhp->Page[i-10],UClit("</html>")) != NULL) break ;
		    if (UCstrstr(&vhp->Page[i-10],UClit("</HTML>")) != NULL) break ;
		  } ;
		 if (tries < 6) continue ;
		 if (errs != NULL) v_Msg(NULL,errs,"HTTPTimeOut",wait,i,tries) ;
	         SOCKETCLOSE(Socket) ; return(FALSE) ;
	       } ;
	      wait = (vhp->ContentLength == V4LIM_BiggestPositiveInt ? 1 : 10) ; /* Wait depends on whether or not we know what's coming */
	      wait = UNUSED ;
	    } ;
	   if (i >= vhp->MaxPageChars)
	    { if (vhp->PageIncrement == 0) vhp->PageIncrement = 5000 ;
	      vhp->MaxPageChars += vhp->PageIncrement ;
	      vhp->Page = realloc(vhp->Page,vhp->MaxPageChars*sizeof(UCCHAR)) ;
	    } ;
	   res = recv(Socket,abuf,1,0) ; vhp->Page[i] = abuf[0] ;
	   v4html_BytesRecv ++ ;
	   if (res <= 0)
	    { if (res == 0) break ;
	      if (errs != NULL) v_Msg(NULL,errs,"HTTPRecvErr",NETERROR) ;
	      SOCKETCLOSE(Socket) ; return(FALSE) ;
	    } ;
	   if (headerline != UNUSED)
	    { 
	      if (vhp->Page[i] == 13 && (i > 0 ? vhp->Page[i-1] == UCEOS : TRUE))
	       { i-- ; continue ; } ;				/* Dump CR's in header section */
	      if (vhp->Page[i] == UCNL)				/* End of line? */
	       { if (headerline == 0)				/* First line has status - parse it out */
		  { if(UCstrncmp(vhp->Page,UClit("HTTP/"),5) == 0)	/* Format must be HTTP/version status xxx */
		     { b = UCstrchr(vhp->Page,' ') ;
		       if (b != NULL) {  status = UCstrtol(b,&b,10) ; if (*b != UClit(' ')) status = 200 ; } ;
		     } ;
		  } ;
		 if (headerline < HTTP_HeaderMax)
		  { Headers[headerline++] = beginofline ; } ;	/* Save pointer to begin of line */
		 vhp->Page[i] = UCEOS ;
		 if (Echo & V4HTML_Trace_Header)
		  printf("V4-Trace-Header: %s\n",UCretASC(beginofline)) ;
		 if (UCempty(beginofline))			/* Blank line means end of HTTP header */
		  { 
		    if (status == 100)				/* Status 100 is "to be continued" - go grab headers again */
		     { if (Echo & V4HTML_Trace_Header) printf("V4-Trace-Continuation...\n") ;
		       goto grab_results ;
		     } ;
/*		    End of headers - parse any/all */
		    for(i=1;i<headerline;i++)
		     { UCSTRTOUPPER(Headers[i]) ;
		       if ((b=UCstrchr(Headers[i],':')) != NULL)	/* Got a keyword? */
		        { *b = UCEOS ; b++ ;
			  if (UCstrcmpIC(UClit("CONTENT-TYPE"),Headers[i]) == 0)
			   { if (UCstrstr(b,UClit("TEXT/HTML")) != NULL) vhp->ContentType = V4HTML_ContentType_HTML ;
			     if (UCstrstr(b,UClit("TEXT/PLAIN")) != NULL) vhp->ContentType = V4HTML_ContentType_Text ;
			     if (UCstrstr(b,UClit("APPLICATION/")) != NULL) vhp->ContentType = V4HTML_ContentType_Appl ;
			     if (UCstrstr(b,UClit("IMAGE/")) != NULL) vhp->ContentType = V4HTML_ContentType_Image ;
			     if (UCstrstr(b,UClit("AUDIO/")) != NULL) vhp->ContentType = V4HTML_ContentType_Sound ;
			     if (UCstrstr(b,UClit("TEXT/XML")) != NULL) vhp->ContentType = V4HTML_ContentType_XML ;
			   } else if (UCstrcmpIC(UClit("CONTENT-LENGTH"),Headers[i]) == 0)
			   { for(;*b!=' ';b++) {} ;
			     vhp->ContentLength = UCstrtol(b,&b,10) ; if (*b > ' ') vhp->ContentLength = UNUSED ;
			   } else if (UCstrcmpIC(UClit("TRANSFER-ENCODING"),Headers[i]) == 0)
			   { if (UCstrstr(b,UClit("CHUNKED")) != NULL) vhp->TransferEncoding = V4HTML_XfrEnc_Chunked ;
			     chkchunk = 0 ; chklen = 0 ;
			   } else if (UCstrcmpIC(UClit("LOCATION"),Headers[i]) == 0)
			   { for(;*b==' ';b++) {} ; e = UCstrchr(b,'\n') ; if (e == NULL) e = UCstrchr(b,'\r') ;
			     if (e != NULL)			/* Save new URI */
			      { *e = UCEOS ;
			        if (UCstrlen(b) < sizeof newuri) { UCstrcpy(newuri,b) ; } ;
			      } ;
			   } ;
			} ;
		     } ;
		    i = -1 ; headerline = UNUSED ;		/* Set index back to begin - don't want headers any more */
		  } ;
		 beginofline = &vhp->Page[i+1] ;		/* Save begin of next line */
	       } ;
	      continue ;
	    } ;
/*	   Here to process body portion */
	   if (chkchunk == 0)
	    { switch (vhp->Page[i])
	       { case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'): case UClit('5'): case UClit('6'): case UClit('7'): case UClit('8'): case UClit('9'):
			if (chkigneol) { i-- ; continue ; } ;
			chklen = (chklen << 4) + vhp->Page[i] - UClit('0') ; i-- ; continue ;
		 case UClit('A'): case UClit('B'): case UClit('C'): case UClit('D'): case UClit('E'): case UClit('F'):
			if (chkigneol) { i-- ; continue ; } ;
			chklen = (chklen << 4) + vhp->Page[i] - UClit('A') + 10 ; i-- ; continue ;
		 case UClit('a'): case UClit('b'): case UClit('c'): case UClit('d'): case UClit('e'): case UClit('f'):
			if (chkigneol) { i-- ; continue ; } ;
			chklen = (chklen << 4) + vhp->Page[i] - UClit('a') + 10 ; i-- ;
			continue ;
		 case UClit(';'):
			chkigneol = TRUE ; i-- ; continue ;
		 case '\r':
			i-- ; continue ;
		 case '\n':
			if (chkigneol) { chkigneol = FALSE ; i-- ; continue ; } ;
/*			End of length specification  - if current chunk is 0 then we are done */
			if (chklen == 0) { vhp->ContentLength = i - 10 ; i-- ; continue ; } ;
			chkchunk = chklen ; chklen = 0 ; i-- ;
			chkigneol = TRUE ;		/* Set to ignore everything until we get end-of-line ('\n') */
			continue ;	/* Next character again starts data */
		 default:
			i-- ; continue ;			/* Invalid hex character ? - skip it */
	       } ;
	    } ;
	   chkchunk-- ;
	   if ((vhp->Page[i] == UCNL || vhp->Page[i] == UClit('\r')) && vhp->ContentType == V4HTML_ContentType_HTML)
	    { i-- ; bol = TRUE ; continue ; } ;			/* Ignore end-of-lines */
	   if (bol)						/* Are we begin-of-line ? */
	    { if (vhp->Page[i] == ' ' && (i >= 1 ? vhp->Page[i-1] == ' ' : FALSE))
	       { i-- ; continue ; } ;				/* Treat multiple spaces at bol as 1 space */
	      if (vhp->Page[i] != UClit(' '))				/* If end of prior line was '>' and first non blank on */
	       { bol = FALSE ;					/*  this line is '<' then trash all spaces */
	         if (UCstrncmp(&vhp->Page[i-2],UClit("> <"),3) == 0)
		  { vhp->Page[i-1] = vhp->Page[i] ; i -- ; } ;
	       } ;
	    } ;
	 } ;
	vhp->Page[i] = UCEOS ; vhp->PageChars = i ;
	v4html_PositionInPage(vhp,V4HTML_Pos_TopOfPage,UNUSED,NULL,errs) ;	
/*	Lets do some quick checking for common error pages */
	switch (status)
	 { default:	v_Msg(NULL,httperr,"HTTPErrUnk",status) ; break ;
	   case 100:	goto return_ok ;
	   case 502:	goto return_ok ;		/* ?? Not really ok, but probably is */
	   case 300:	/* Multiple Choices - see Location header */
	   case 301:	/* URI permanently moved */
	   case 307:	/* Temporary Redirect - resend new URI with same method */
	   case 303:	/* See Other - issue GET to URI in Location header */
			if (errs != NULL) v_Msg(NULL,errs,"HTTPRedirect",status,newuri) ;
			return(FALSE) ;
	   case 302:	if (Echo & V4HTML_Trace_Header) printf("V4-Trace-Redirect to: %s\n",newuri) ;
			rvhp = v4html_FollowHyperLink(NULL,newuri,UClit(""),UClit(""),errs,V4HTMP_Page_Get,vhp) ;
			if (rvhp != NULL) goto return_ok ;
			return(FALSE) ;
	   case 304:	v_Msg(NULL,httperr,"HTTPErr304") ; break ;
	   case 305:	v_Msg(NULL,httperr,"HTTPErr305") ; break ;
	   case 400:	v_Msg(NULL,httperr,"HTTPErr400") ; break ;
	   case 401:	v_Msg(NULL,httperr,"HTTPErr401") ; break ;
	   case 402:	v_Msg(NULL,httperr,"HTTPErr402") ; break ;
	   case 403:	v_Msg(NULL,httperr,"HTTPErr403") ; break ;
	   case 404:	v_Msg(NULL,httperr,"HTTPErr404") ; break ;
	   case 500:	v_Msg(NULL,httperr,"HTTPErr500") ; break ;
	   case 501:	v_Msg(NULL,httperr,"HTTPErr501") ; break ;
	 } ;
	if (status / 100 != 2)
	 { if (errs != NULL) v_Msg(NULL,errs,"HTTPError",status,httperr) ;
	   return(FALSE) ;
	 } ;
return_ok:
	if (Echo & V4HTML_Trace_Body)
	 { if (UCstrlen(vhp->Page) < 1000) { printf("V4-Trace-Body:\n%s\n",UCretASC(vhp->Page)) ; }
	    else { UCCHAR save[1] ; save[0] = vhp->Page[1000] ; vhp->Page[1000] = UCEOS ;
		   printf("V4-Trace-PartialBody:\n%s...\n",UCretASC(vhp->Page)) ; vhp->Page[1000] = save[0] ;
		   if (UCstrlen(vhp->Page) < 2000) { printf("V4-Trace-Body2:\n%s\n",UCretASC(&vhp->Page[1000])) ; }
		    else { save[0] = vhp->Page[2000] ; vhp->Page[2000] = UCEOS ;
			   printf("V4-Trace-PartialBody2:\n%s...\n",UCretASC(&vhp->Page[1000])) ; vhp->Page[2000] = save[0] ;
			   save[0] = vhp->Page[3000] ; vhp->Page[3000] = UCEOS ;
			   printf("V4-Trace-PartialBody3:\n%s...\n",UCretASC(&vhp->Page[2000])) ; vhp->Page[3000] = save[0] ;
			 } ;
		 } ;
	 } ;
/*	Return length of what we got unless grabbing HEAD only then return what we would have gotten */
	vhp->ContentLength = V4LIM_BiggestPositiveInt ;		/* Default length is very large */
	res = (mode == V4HTMP_Page_Head ? (vhp->ContentLength == V4LIM_BiggestPositiveInt ? 0 : vhp->ContentLength) : i) ;
	if (res < 20)
	 { if (errs != NULL) v_Msg(NULL,errs,"HTTPErrData") ;
	   return(FALSE) ;
	 } ;
	return(res) ;
}

/*	v4html_ParseURL - parses URL into hostname & arguments			*/
/*	Call: res = v4html_ParseURL( url , hostname , basepath, arguments , argmax , port )
	  where res = V4HTML_URL_xxx indicating type of request,
		vhp is pointer to current vhp (for resolving partial urls),
		url is pointer to url spec,
		hostname is ptr to be updated with host name (or filename if FilePath url),
		basepath is ptr to be updated with base directory path
		arguments to be updated with url arguments
		argmax is max length of arguments,
		port is int* to be updated with port number (default to 80)	*/

int v4html_ParseURL(vhp,url,hostname,basepath,arguments,argmax,port)
  struct V4HTML__Page *vhp ;
  UCCHAR *url ;
  UCCHAR *hostname ;
  UCCHAR *basepath ;
  UCCHAR *arguments ; int argmax ;
  int *port ;
{
  UCCHAR *h,*a,*e ; int res,l ; UCCHAR path[1024], localurl[2048] ;

	h = NULL ; *port = 80 ;
/*	If current page type unknown, or type File then maybe parse as a filename */
	if (vhp->PageType == V4HTML_PageType_Undef)
	 { if (url[0] == UClit('\\') || url[1] == UClit(':') || url[0] == UClit('/')) vhp->PageType = V4HTML_PageType_File ;
	 } ;
	if (vhp->PageType == V4HTML_PageType_File)
	 { UCCHAR save[1] ;
/*	   If file specification appears to be relative then start with prior base path */
	   if (url[0] == UClit('\\') || url[0] == UClit('/') || url[1] == UClit(':')) { UCstrcpy(localurl,url) ; }
	    else { UCstrcpy(localurl,vhp->BasePath) ; UCstrcat(localurl,url) ; } ;
	   e = UCstrrchr(localurl,'\\') ; if (e == NULL) e = UCstrrchr(localurl,'/') ;
	   if (e == NULL) return(V4HTML_URL_UNKOWN) ;
	   e++ ; save[0] = *e ; *e = UCEOS ; UCstrcpy(basepath,localurl) ; *e = save[0] ; UCstrcpy(hostname,e) ;
	   return(res = V4HTML_URL_FilePath) ;
	 }
	if (UCstrncmp(UClit("http://"),url,7) == 0) { h = &url[7] ; res = V4HTML_URL_HTTP ; }
	 else if (UCstrncmp(UClit("HTTP://"),url,7) == 0) { h = &url[7] ; res = V4HTML_URL_HTTP ; }
	 else if (UCstrstr(url,UClit("://")) != NULL) { return(V4HTML_URL_UNKOWN) ; }
	 else { h = url ; res = V4HTML_URL_HTTP ;
/*		Must have partial specification - pull host & base path from vhp */
/*		  unless starts with "/" - then just take base host name */
		if (url[0] == UClit('/'))
		 { UCstrcpy(localurl,vhp->BaseHostName) ; UCstrcat(localurl,url) ; }
		 else { v_Msg(NULL,localurl,"@%1U%2U/%3U",vhp->BaseHostName,vhp->BasePath,url) ; } ;
		url = localurl ; h = url ;
	      } ;					/* Default to http protocol */
	a = UCstrchr(h,'/') ;
	if (a == NULL)					/* No arguments? */
	 { *arguments = UCEOS ; UCstrcpy(hostname,h) ; *basepath = UCEOS ; }
	 else { l = UCstrlen(a) ; if (l > argmax - 1) { l = argmax - 1 ; } ;
		UCstrncpy(arguments,a,l) ; arguments[l] = UCEOS ;
		l = a - h ; UCstrncpy(hostname,h,l) ; hostname[l] = UCEOS ;
/*		Now find base path from arguments string */
		if (UCempty(arguments)) { *basepath = UCEOS ; }
		 else { e = UCstrchr(arguments,'?') ;
			if (e == NULL) { UCstrcpy(path,arguments) ; }
			 else { UCstrncpy(path,arguments,e-arguments) ; path[e-arguments] = UCEOS ; } ;
			e = UCstrrchr(path,'/') ;		/* Scan back to rightmost UClit('/')) */
			if (e == NULL) { *basepath = UCEOS ; }
			 else { *e = UCEOS ; UCstrcpy(basepath,path) ; } ;
		      } ;
	      } ;
	a = UCstrchr(hostname,':') ;			/* Did we get a port number in host name? */
	if (a != NULL)
	 { *a = UCEOS ; *port = UCstrtol(a+1,&a,10) ;
	   if (*a != UCEOS) return(V4HTML_URL_UNKOWN) ;
	 } ;
	return(res) ;
}

/*	v4html_KeywordValuePair - Iteratively returns key=value pairs for an entry	*/
/*	Call: res = v4html_KeywordValuePair( vhp , tagx , index , keyword , value ,errs )
	  where res = UNUSED for end of entry, 0 for error, +n for seed index for next call,
		vhp is page,
		tagx is set to current TagX for current entry,
		index is UNUSED to start, +n for next,
		keyword & value are pointers updated with data,
		errs, if not NULL, is updated on error					*/

int v4html_KeywordValuePair(vhp,tagx,index,keyword,value,errs)
  struct V4HTML__Page *vhp ;
  int tagx,index ;
  UCCHAR *keyword,*value ;
  UCCHAR *errs ;
{
  static UCCHAR input[1024], term[2] ;
  int i,j ;

	if (index == UNUSED)
	 { UCstrncpy(input,&vhp->Page[vhp->Tag[tagx].curStart],vhp->Tag[tagx].curEnd-vhp->Tag[tagx].curStart) ;
	   input[vhp->Tag[tagx].curEnd-vhp->Tag[tagx].curStart] = UCEOS ;	/* Copy into input to make life easier */
	   for(i=0;input[i]!=UClit(' ');i++) {} ;		/* Position past "<input " */
	 } else
	 { i = index ; } ;
	if (input[i] == UCEOS) return(UNUSED) ;	/* All done */
	for(;input[i]==UClit(' ');i++) {} ;		/* Position past any spaces */
	for(j=0;input[i]!=UCEOS;j++,i++)		/* Grab attribute up to "=" */
	 { if (input[i] == UClit('=') || input[i] == UClit(' ')) break ;
	   keyword[j] = UCTOUPPER(input[i]) ;
	 } ; keyword[j] = UCEOS ; if (input[i] == UCEOS) return(UNUSED) ;
	if (input[i] == UClit(' '))			/* If broke on space then no value */
	 { ZUS(value) ; return(i) ; } ;
	i++ ;					/* Past the UClit('=') */
	if (input[i] == UClit('"') || input[i] == UClit('\''))
	 { term[0] = input[i] ; i++ ; }		/* Value enclosed in quotes */
	 else { term[0] = UClit(' ') ; } ;		/*   not enclosed in quotes - ends with space */
	for(j=0;input[i]!=UCEOS;j++,i++)
	 { if (input[i] == term[0]) { if (term[0] != UClit(' ')) i++ ; break ; } ;
	   if (j >= V4HTML_InputValueMax)
	    { if (errs != NULL) v_Msg(NULL,errs,"HTMLValTooLong",keyword,V4HTML_InputValueMax) ;
	      return(0) ;
	     } ;
	   value[j] = input[i] ;
	 } ; value[j] = UCEOS ;

	return(i) ;
}


/*	v4im_DoHTML - Handles HTML() module	*/

#define MHVHP if (gpi->vhp == NULL) { v_Msg(ctx,NULL,"HTMLNoPage",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;

P *v4im_DoHTML(ctx,respnt,argcnt,argpnts,intmodx,trace)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  P *respnt,*argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ P *ipt,*cpt, argpt ;
  struct V4L__ListPoint *lp ;
  struct V4DPI__DimInfo *di ;
  struct V4HTML__Page *vhp ;
  struct V4LEX__TknCtrlBlk *tcb = NULL ;
  struct UC__File UCFile ;
  struct V4L__ListHTMLTable *lht ;
  regex_t comp;
  regmatch_t matches[10] ;
  static UCCHAR LastErrorMsg[256] ; UCCHAR *ucb ;
  UCCHAR UserName[64], UserPassword[64] ;
  UCCHAR *fp ; double dnum,dp ; int inexp, first, sign, exp, ismoney, isRegExp, uc ;
  int ax,row,col,ok,len,i,j ; UCCHAR formfield[V4HTML_TagNameMax+1], *b, *regexpStart, tbuf[V4TMBufMax] ;

	if (gpi->RestrictionMap & V_Restrict_QuotaHTML)
	 { if (v4html_BytesSend > gpi->QuotaHTMLPut || v4html_BytesRecv > gpi->QuotaHTMLGet)
	   { v_Msg(ctx,NULL,"RestrictQuota",intmodx,gpi->QuotaHTMLGet,v4html_BytesRecv,gpi->QuotaHTMLPut,v4html_BytesSend) ; ipt = NULL ; goto html_end ; } ;
	 } ;
	ipt = (P *)&Log_True ; ZS(formfield) ; isRegExp = FALSE ; ZUS(UserName) ; ZUS(UserPassword) ;
	uc = FALSE ;
	for(ax=1,ok=TRUE;ok && ax<=argcnt;ax++)			/* Loop thru all arguments */
	 { 
	   switch (v4im_CheckPtArgNew(ctx,argpnts[ax],&cpt,&argpt))
	    { default:	    		v_Msg(ctx,NULL,"TagBadUse",intmodx) ; ipt = NULL ; goto html_end ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; ipt = NULL ; goto html_end ;
	      case V4IM_Tag_Append:
		if (gpi->RestrictionMap & V_Restrict_redirection) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; ipt = NULL ; goto html_end ; } ;
		MHVHP
		v4im_GetPointFileName(&ok,UCTBUF1,V4TMBufMax,cpt,ctx,NULL) ; if (!ok) break ;
		if (!v_UCFileOpen(&UCFile,UCTBUF1,UCFile_Open_Append,TRUE,ctx->ErrorMsgAux,intmodx))
		 { v_Msg(ctx,NULL,"ModInvArg2",intmodx,argpnts[ax]) ; ipt = NULL ; goto html_end ; } ;
		{ char abuf[1024] ; int done ;
		  for(done=0;done<UCstrlen(vhp->Page);done+=strlen(abuf))
		   { for (i=0;i<(sizeof abuf)-1;i++) { abuf[i] = vhp->Page[done+i] ; if (vhp->Page[done+i] == UCEOS) break ; } ;
		     abuf[i] = '\0' ; fprintf(UCFile.fp,"%s",abuf) ;
		     if (i >= sizeof abuf) break ;
		   } ;
		}
		v_UCFileClose(&UCFile) ;
		break ;
	      case V4IM_Tag_Area:
		MHVHP
		v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,cpt,ctx) ; if (!ok) break ;
		if (!v4html_PositionInPage(gpi->vhp,V4HTML_Pos_Forward,V4HTML_HTag_Area,UCTBUF1,ctx->ErrorMsgAux))
		 { v_Msg(ctx,NULL,"ModInvTagVal",intmodx,V4IM_Tag_Area,ax) ; ipt = NULL ; goto html_end ; } ;
		break ;
	      case -V4IM_Tag_Begin:
		MHVHP
		v4html_PositionInPage(gpi->vhp,V4HTML_Pos_TopOfPage,UNUSED,NULL,ctx->ErrorMsgAux) ;
		break ;
	      case V4IM_Tag_Body:
	      case V4IM_Tag_Cell:
	      case V4IM_Tag_Column:
		MHVHP
		col = v4im_GetPointInt(&ok,cpt,ctx) ;
		if (!v4html_PositionRowColumn(gpi->vhp,gpi->vhp->CurRow,col,ctx->ErrorMsgAux))
		 { v_Msg(ctx,NULL,"ModInvTagVal",intmodx,V4IM_Tag_Column,ax) ; ipt = NULL ; goto html_end ; } ;
		break ;
	      case -V4IM_Tag_Column:
		MHVHP
		if (gpi->vhp->NumRows <= 0)
		 { v_Msg(ctx,NULL,"HTMLNotInTABLE",intmodx,-V4IM_Tag_Column,ax) ; ipt = NULL ; goto html_end ; } ;
		ipt = respnt ; intPNTv(ipt,gpi->vhp->CurColumn) ; break ;
	      case V4IM_Tag_Dim:
		MHVHP
		if (!v4html_PositionInPage(gpi->vhp,V4HTML_Pos_Forward,V4HTML_HTag_Text,NULL,ctx->ErrorMsgAux))
		 { v_Msg(ctx,NULL,"ModInvTagVal",intmodx,V4IM_Tag_Dim,ax) ; ipt = NULL ; goto html_end ; } ;
		len = (int)(&gpi->vhp->Page[gpi->vhp->curEnd+1] - &gpi->vhp->Page[gpi->vhp->curStart]) ;
		if (len >= V4DPI_AlphaVal_Max - 1) { v_Msg(ctx,NULL,"HTMLTooBig",intmodx,ax,di->DimId) ; ipt = NULL ; goto html_end ; } ;
		UCstrncpy(tbuf,&gpi->vhp->Page[gpi->vhp->curStart],len) ; tbuf[len] = UCEOS ;
		DIMINFO(di,ctx,cpt->Value.IntVal) ;
		switch (di->PointType)
		 { default:
			INITTCB ; v4lex_NestInput(tcb,NULL,tbuf,V4LEX_InpMode_String) ;
			if (!v4dpi_PointAccept(respnt,di,tcb,ctx,V4DPI_PointParse_RetFalse))
			 { v4lex_FreeTCB(tcb) ;
			   if (len > 10) { tbuf[10] = UCEOS ; UCstrcat(tbuf,UClit("...")) ; } ;
			   v_Msg(ctx,NULL,"HTMLInvVal",intmodx,ax,di->DimId,tbuf) ; ipt = NULL ; goto html_end ;
			 } ;
			v4lex_FreeTCB(tcb) ; break ;
		   case V4DPI_PntType_Int:
		   case V4DPI_PntType_Real:
#define ERR(ARG) { v_Msg(ctx,ctx->ErrorMsgAux,ARG) ; v_Msg(ctx,NULL,"HTMLCellErr",intmodx,gpi->vhp->CurRow,gpi->vhp->CurColumn,tbuf) ; ipt = NULL ; goto html_end ; }
			for(fp=tbuf;*fp!=UCEOS;fp++) { if (*fp != UClit(' ')) break ; } ;
			dnum = 0.0 ; inexp = FALSE ; first = TRUE ; sign = FALSE ; dp = 0 ; ismoney = TRUE ;
			for(;*fp!=UCEOS;fp++)
			 { switch(*fp)
			    { default:	ERR("ParseInvNum") ;
			      case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'): case UClit('5'): case UClit('6'):
			      case UClit('7'): case UClit('8'): case UClit('9'):
				first = FALSE ;
				if (inexp)
				 { if (dp != 0) ERR("ParseDecPts") ;
				   exp *= 10 ; exp += *fp - UClit('0') ;
				 } else
				 { dnum *= 10.0 ; dnum += (double)(*fp - UClit('0')) ; dp *= 10.0 ;
				 } ;
				break ;
			      case UClit('$'):	if (ismoney) continue ;
			      case UClit('('): case UClit(')'):
				if (ismoney) { sign = TRUE ; continue ; } ;
				ERR("ParseParens")
			      case '\r':
			      case '\n':
			      case UClit(' '): goto num_end ;
			      case UClit(','): continue ;	/* And commas */
			      case UClit('"'): continue ;	/*  and quotes */
			      case UClit('E'): case UClit('e'):
				if (inexp) ERR("ParseExponent") ;
				if (dp > 0.0) dnum /= dp ;
				if (sign) { dnum = -dnum ; sign = FALSE ; } ;
				first = FALSE ; inexp = TRUE ; exp = 0 ; dp = 0.0 ;
				if (*(fp+1) == UClit('+')) fp++ ;
				if (*(fp+1) == UClit('-')) { fp++ ; sign = TRUE ; } ;
				break ;
			      case UClit('.'):
				if (dp != 0) ERR("ParseDecPts") ;
				first = FALSE ; dp = 1.0 ; continue ;
			      case UClit('-'):
				if (ismoney) { sign = TRUE ; continue ; } ;
				if (!first)			/* Error if not first unless last */
				 { if (*(fp+1) == UClit(' ') || *(fp+1) == UCEOS) { sign = TRUE ; continue ; } ;
				   ERR("ParseMinus") ; 
				 } ;
				first = FALSE ; sign = TRUE ; continue ;
			    } ;
			 } ;
num_end:
			if (inexp)
			 { if (sign) { for(dp=1.0;exp>0;exp--) { dp /= 10.0 ; } ; }
			    else { for(dp=1.0;exp>0;exp--) { dp *= 10.0 ; } ; }
			   dnum = dnum * dp ;
			 } else
			 { if (dp > 0.0) dnum /= dp ;
			   if (sign) dnum = -dnum ;
			 } ;
			ipt = respnt ; ZPH(ipt) ; ipt->Dim = di->DimId ; ipt->PntType = di->PointType ;
			if (di->PointType == V4DPI_PntType_Int)
			 { ipt->Bytes = V4PS_Int ; ipt->Value.IntVal = DtoI(dnum) ;
			 } else
			 { ipt->Bytes = V4PS_Real ; PUTREAL(ipt,dnum) ;
			 } ;
			break ;
		   CASEofChar
			ipt = respnt ; ZPH(respnt) ; ipt->PntType = di->PointType ; ipt->Dim = di->DimId ;
			UCstrncpy(&ipt->Value.UCVal[1],tbuf,len) ; ipt->Value.UCVal[1+len] = UCEOS ;
			UCCHARPNTBYTES2(ipt,len) ;
			break ;
		 } ;
		ipt = respnt ; break ;
	      case V4IM_Tag_Echo:
		switch(cpt->PntType)
		 { default:
			v_Msg(ctx,NULL,"ModArgPntType",intmodx,ax,cpt,cpt->PntType) ; ipt = NULL ; goto html_end ;
		   CASEofChar
			v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,cpt,ctx) ;
			v_Msg(ctx,UCTBUF2,"TraceHTML",intmodx,V4IM_Tag_Echo,UCTBUF1) ; vout_UCText(VOUT_Status,0,UCTBUF2) ;
			break ;
		   case V4DPI_PntType_Dict:
			switch (v4im_GetDictToEnumVal(ctx,cpt))
			 { default:		v_Msg(ctx,NULL,"ModTagValue",intmodx,ax,V4IM_Tag_Echo,cpt) ; ipt = NULL ; goto html_end ; 
			   case _URL:		Echo |= V4HTML_Trace_URL ; break ;
			   case _None:		Echo = 0 ; break ;
			   case _Header:	Echo |= V4HTML_Trace_Header ; break ;
			   case _Body:		Echo |= V4HTML_Trace_Body ; break ;
			   case _Request:	Echo |= V4HTML_Trace_Request ; break ;
			   case _All:		Echo |= 0xffffffff ; break ;
			 } ;
			break ;
		 } ;
		break ;
	      case -V4IM_Tag_Error:
		ucb = LastErrorMsg ; if (UCstrlen(ucb) >= V4PT_MaxCharLenIn0) { ucb[V4PT_MaxCharLenIn0-1] = UCEOS ; UCstrcat(ucb,UClit("...")) ; } ;
		ipt = respnt ; uccharPNTv(ipt,ucb) ; break ;
	      case V4IM_Tag_Exists:
		v4im_GetPointUC(&ok,tbuf,V4TMBufMax,cpt,ctx) ;
		vhp = v4html_FollowHyperLink(NULL,tbuf,(UCstrlen(UserName) > 0 ? UserName : NULL),
						(UCstrlen(UserPassword) > 0 ? UserPassword : NULL),ctx->ErrorMsgAux,V4HTMP_Page_Head,NULL) ;
		if (vhp == NULL) { ok = FALSE ; break ; } ;
		ipt = respnt ; intPNTv(ipt,vhp->ContentLength) ; return(ipt) ;
	      case V4IM_Tag_Field:
		MHVHP
		if (gpi->vhp->InputCount <= 0) { v_Msg(ctx,NULL,"HTMLNoForm",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
		v4im_GetPointUC(&ok,formfield,UCsizeof(formfield),cpt,ctx) ; if (!ok) break ;
		break ;					/* Just save form name, need Value::xxx to set */
	      case V4IM_Tag_Form:
	      case -V4IM_Tag_Group:			/* If Group? then return list of all groups */
		if (!isRegExp) { v_Msg(ctx,NULL,"HTMLNoRegExp",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
		INITLP(respnt,lp,Dim_List) ;
		for(i=1;i<=10;i++)			/* Start with second element (first is entire pattern) */
		 { if (matches[i].rm_so == -1) break ;
		   ipt = &argpt ; uccharPNT(ipt)
		   len= matches[i].rm_eo - matches[i].rm_so ;
		   if (len >= V4DPI_AlphaVal_Max-1) len = V4DPI_AlphaVal_Max - 2 ;
		   UCstrncpy(&ipt->Value.UCVal[1],&regexpStart[matches[i].rm_so],len) ;
		   UCCHARPNTBYTES2(ipt,len) ;
		   v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,ipt,0) ;
		 } ;
		ENDLP(respnt,lp) ; ipt = respnt ;
		goto html_end ;
	      case V4IM_Tag_Group:
		if (!isRegExp) { v_Msg(ctx,NULL,"HTMLNoRegExp",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
		i = v4im_GetPointInt(&ok,cpt,ctx) ; if (!ok) break ;
		if (i < 0 || i > 9 ? TRUE : matches[i].rm_so == -1)
		 { v_Msg(ctx,NULL,"HTMLBadGroup",intmodx,ax,V4IM_Tag_Group,i) ; ipt = NULL ; goto html_end ; } ;
		ipt = respnt ; uccharPNT(respnt) ;
		len= matches[i].rm_eo - matches[i].rm_so ;
		if (len >= V4DPI_AlphaVal_Max-1) len = V4DPI_AlphaVal_Max - 2 ;
		UCstrncpy(&ipt->Value.UCVal[1],&regexpStart[matches[i].rm_so],len) ;
		UCCHARPNTBYTES2(ipt,len) ;
		goto html_end ;
	      case -V4IM_Tag_Id:
		MHVHP
		if (!v4html_GetCurrentId(gpi->vhp,&respnt->Value.UCVal[1],ctx->ErrorMsgAux))
		 { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; ipt = NULL ; goto html_end ; } ;
		ipt = respnt ; uccharPNT(ipt) ; UCCHARPNTBYTES1(ipt) ;
		break ;
	      case V4IM_Tag_ListOf:
		MHVHP
		INITLP(respnt,lp,Dim_List) ; lp->ListType = V4L_ListType_HTMLTable ;
		lht = (struct V4L__ListHTMLTable *)&lp->Buffer ; lp->Bytes = (char *)&lp->Buffer - (char *)lp + sizeof *lht ;
		lht->vhp = gpi->vhp ; ENDLP(respnt,lp) ;
		switch(cpt->PntType)
		 { default:
			v_Msg(ctx,NULL,"ModTagValue",intmodx,ax,V4IM_Tag_ListOf,cpt) ; ipt = NULL ; goto html_end ;
		    case V4DPI_PntType_Dict:
			switch (v4im_GetDictToEnumVal(ctx,cpt))
			 { default:		v_Msg(ctx,NULL,"ModTagValue",intmodx,ax,V4IM_Tag_ListOf,cpt) ; ipt = NULL ; goto html_end ;
			   case _Rows:
				if (gpi->vhp->PageType != V4HTML_PageType_Section || gpi->vhp->NumRows <= 0)
				 { v_Msg(ctx,NULL,"HTMLNoTable",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
				lp->Entries = (lht->NumRows = gpi->vhp->NumRows) ; lht->NumColumns = UNUSED ; break ;
			   case _Columns:
				if (gpi->vhp->PageType != V4HTML_PageType_Section || gpi->vhp->NumRows <= 0)
				 { v_Msg(ctx,NULL,"HTMLNoTable",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
				lht->NumRows = UNUSED ; lp->Entries = (lht->NumColumns = gpi->vhp->NumColumns) ; break ;
			   case _Fields:
				{ P argbuf ; int i ;
				  INITLP(respnt,lp,Dim_List) ZPH(&argbuf) ;
				  vhp = gpi->vhp ;
				  for(i=0;i<vhp->InputCount;i++)
				   { UCstrcpy(tbuf,vhp->Input[i].Name) ; UCstrcat(tbuf,UClit("=")) ; UCstrcat(tbuf,vhp->Input[i].Value) ;
				     uccharPNTv(&argbuf,tbuf) ;
				     if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&argbuf,0))
				      { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto html_end ; } ;
				   } ;
				  ENDLP(respnt,lp)
				} ; break ;
			 } ;
			break ;	
		 } ;
		ipt = respnt ; break ;
	      case V4IM_Tag_Next:
		MHVHP
		switch(cpt->PntType)
		 { default:
			v_Msg(ctx,NULL,"ModTagValue",intmodx,ax,V4IM_Tag_Next,cpt) ; ipt = NULL ; goto html_end ;
		    CASEofChar
			v4im_GetPointUC(&ok,tbuf,V4TMBufMax,cpt,ctx) ; if (!ok) break ;
			if (!v4html_PositionInPage(gpi->vhp,V4HTML_Pos_Forward,V4HTML_HTag_Text,tbuf,ctx->ErrorMsgAux))
			 { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
			break ;
		    case V4DPI_PntType_RegExpPattern:
			if (!v4html_PositionInPage(gpi->vhp,V4HTML_Pos_Forward,V4HTML_HTag_RegExp,ASCretUC((char *)cpt + cpt->Bytes - sizeof(regex_t)),ctx->ErrorMsgAux))
			 { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
			break ;
		    case V4DPI_PntType_Dict:
			switch (v4im_GetDictToEnumVal(ctx,cpt))
			 { default:	v_Msg(ctx,NULL,"ModTagValue",intmodx,ax,V4IM_Tag_Next,cpt) ; ipt = NULL ; goto html_end ;
			   case _Table:	ok = v4html_PositionInPage(gpi->vhp,V4HTML_Pos_Forward,V4HTML_HTag_Table,NULL,ctx->ErrorMsgAux) ; break ;
			   case _Row:	ok = v4html_PositionInPage(gpi->vhp,V4HTML_Pos_Forward,V4HTML_HTag_Row,NULL,ctx->ErrorMsgAux) ; break ;
			   case _Cell:	ok = v4html_PositionInPage(gpi->vhp,V4HTML_Pos_Forward,V4HTML_HTag_Cell,NULL,ctx->ErrorMsgAux) ; break ;
			   case _URL:	ok = v4html_PositionInPage(gpi->vhp,V4HTML_Pos_Forward,V4HTML_HTag_A,NULL,ctx->ErrorMsgAux) ; break ;
			   case _Area:	ok = v4html_PositionInPage(gpi->vhp,V4HTML_Pos_Forward,V4HTML_HTag_Area,NULL,ctx->ErrorMsgAux) ; break ;
			   case _Image:	ok = v4html_PositionInPage(gpi->vhp,V4HTML_Pos_Forward,V4HTML_HTag_Image,NULL,ctx->ErrorMsgAux) ; break ;
			   case _Layer:	ok = v4html_PositionInPage(gpi->vhp,V4HTML_Pos_Forward,V4HTML_HTag_Layer,NULL,ctx->ErrorMsgAux) ; break ;
			   case _Frame:	ok = v4html_PositionInPage(gpi->vhp,V4HTML_Pos_Forward,V4HTML_HTag_Frame,NULL,ctx->ErrorMsgAux) ; break ;
			   case _Form:	
				ok = v4html_PositionInPage(gpi->vhp,V4HTML_Pos_Forward,V4HTML_HTag_Form,NULL,ctx->ErrorMsgAux) ;
				if (!v4html_LoadFormInfo(gpi->vhp,ctx->ErrorMsgAux))
				 { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
				break ;
			 } ;
			if (!ok)
			 { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
			break ;	
		 } ; break ;
	      case V4IM_Tag_Password:	v4im_GetPointUC(&ok,UserPassword,UCsizeof(UserPassword),cpt,ctx) ; break ;
	      case -V4IM_Tag_Pop:
		MHVHP
		vhp = v4html_GoBackPage(gpi->vhp,ctx->ErrorMsgAux) ;
		if (vhp == NULL) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
		gpi->vhp = vhp ;
		break ;
	      case V4IM_Tag_Push:
		switch(cpt->PntType)
		 { default:
			v_Msg(ctx,NULL,"ModTagValue",intmodx,ax,V4IM_Tag_Push,cpt) ; ipt = NULL ; goto html_end ;
		    CASEofChar
			v4im_GetPointUC(&ok,tbuf,V4TMBufMax,cpt,ctx) ;
			vhp = v4html_FollowHyperLink(gpi->vhp,tbuf,(UCstrlen(UserName) > 0 ? UserName : NULL),
							(UCstrlen(UserPassword) > 0 ? UserPassword : NULL),ctx->ErrorMsgAux,V4HTMP_Page_Get,NULL) ;
			if (vhp == NULL)
			 { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
			gpi->vhp = vhp ; break ;
		    case V4DPI_PntType_Dict:
			switch (v4im_GetDictToEnumVal(ctx,cpt))
			 { default:	v_Msg(ctx,NULL,"ModTagValue",intmodx,ax,V4IM_Tag_Push,cpt) ; ipt = NULL ; goto html_end ;
			   case _Table:
				MHVHP
				if (!v4html_SectionBounds(gpi->vhp,V4HTML_HTag_Table,ctx->ErrorMsgAux))
				 { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
				vhp = v4html_PushSection(gpi->vhp,ctx->ErrorMsgAux) ;
				if (vhp == NULL) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
				gpi->vhp = vhp ; break ;
			   case _Cell:
				MHVHP
				if (!v4html_SectionBounds(gpi->vhp,V4HTML_HTag_Cell,ctx->ErrorMsgAux))
				 { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
				vhp = v4html_PushSection(gpi->vhp,ctx->ErrorMsgAux) ;
				if (vhp == NULL) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
				gpi->vhp = vhp ; break ;
			   case _URL:
				MHVHP
				if (!v4html_GetHyperLink(gpi->vhp,UCTBUF1,ctx->ErrorMsgAux))
				 { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
				if ((vhp=v4html_FollowHyperLink(gpi->vhp,UCTBUF1,(UCstrlen(UserName) > 0 ? UserName : NULL),
					(UCstrlen(UserPassword) > 0 ? UserPassword : NULL),ctx->ErrorMsgAux,V4HTMP_Page_Get,NULL)) == NULL)
				 { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
				gpi->vhp = vhp ; break ;
			   case _Form:
				MHVHP
				UCstrcpy(vhp->UserName,UserName) ; UCstrcpy(vhp->UserPassword,UserPassword) ;
				if ((vhp = v4html_PostForm(vhp,ctx->ErrorMsgAux)) == NULL) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
				gpi->vhp = vhp ; break ;
			   case _Frame:
				MHVHP
				if (!v4html_GetHyperLink(gpi->vhp,UCTBUF1,ctx->ErrorMsgAux)) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
				if ((vhp=v4html_FollowHyperLink(gpi->vhp,UCTBUF1,(UCstrlen(UserName) > 0 ? UserName : NULL),
							(UCstrlen(UserPassword) > 0 ? UserPassword : NULL),ctx->ErrorMsgAux,V4HTMP_Page_Get,NULL)) == NULL)
				 { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
				gpi->vhp = vhp ; break ;
			 } ;
			break ;	
		 } ; break ;
	      case V4IM_Tag_Put:
	      case V4IM_Tag_Create:
		if (gpi->RestrictionMap & V_Restrict_redirection) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; ipt = NULL ; goto html_end ; } ;
		MHVHP
		v4im_GetPointFileName(&ok,tbuf,V4TMBufMax,cpt,ctx,NULL) ; if (!ok) break ;
		switch (vhp->ContentType)
		 {
		   case V4HTML_ContentType_Undef:
		   case V4HTML_ContentType_HTML:
		   case V4HTML_ContentType_XML:
		   case V4HTML_ContentType_Text:
			if (!v_UCFileOpen(&UCFile,tbuf,UCFile_Open_Write,TRUE,ctx->ErrorMsgAux,intmodx))
			 { v_Msg(ctx,NULL,"ModInvArg2",intmodx,argpnts[ax]) ; ipt = NULL ; goto html_end ; } ;
			{ char abuf[1024+1] ; int done ;
			  for(done=0;done<vhp->PageChars;done+=strlen(abuf))
			   { for (i=0;i<(sizeof abuf)-1;i++) { abuf[i] = vhp->Page[done+i] ; if (vhp->Page[done+i] == UCEOS) break ; } ;
			     abuf[i] = '\0' ; fprintf(UCFile.fp,"%s",abuf) ;
			   } ;
			}
			v_UCFileClose(&UCFile) ;
			break ;
		   case V4HTML_ContentType_Sound:
		   case V4HTML_ContentType_Appl:
		   case V4HTML_ContentType_Image:
			if (!v_UCFileOpen(&UCFile,tbuf,UCFile_Open_WriteBin,TRUE,ctx->ErrorMsgAux,intmodx))
			 { v_Msg(ctx,NULL,"ModInvArg2",intmodx,argpnts[ax]) ; ipt = NULL ; goto html_end ; } ;
			{ char abuf[1024] ; int done ;
			  for(done=0;done<vhp->PageChars;done+=sizeof(abuf))
			   { for (i=0;i<(sizeof abuf) && i+done<vhp->PageChars;i++) { abuf[i] = vhp->Page[done+i] ; } ;
			     fwrite(abuf,i,1,UCFile.fp) ;
			   } ;
			}
			v_UCFileClose(&UCFile) ;
			break ;
		 } ;
		break ;
	      case V4IM_Tag_RegExp:
		MHVHP vhp = gpi->vhp ;
		switch (cpt->PntType)
		 { default:
			v_Msg(ctx,NULL,"ModArgPntType3",intmodx,ax,cpt->PntType,V4DPI_PntType_RegExpPattern,V4DPI_PntType_Char) ;
			ipt = NULL ; goto html_end ;
		   CASEofChar


			v4im_GetPointChar(&ok,ASCTBUF1,V4TMBufMax,cpt,ctx) ; if (!ok) break ;
			j = REG_EXTENDED ;
			if (uc) j |= REG_ICASE ;
			if (i=vregexp_RegComp(&comp,ASCTBUF1,j))
			 { vregexp_Error(i,&comp,ctx->ErrorMsgAux,sizeof ctx->ErrorMsgAux) ;
			   v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; goto html_end ;
			 } ;
			i = vregexp_RegExec(&comp,UCretASC(&vhp->Page[vhp->curStart]),10,matches,0) ;
			if (i) { v_Msg(ctx,NULL,"StrRegExpNoMtch",intmodx,V4IM_Tag_RegExp,cpt) ; ipt = NULL ; goto html_end ; } ;
			regexpStart = &vhp->Page[vhp->curStart] ;		/* Remember starting position */
			vhp->curStart += matches[0].rm_so ;			/* Set current start to begin of pattern */
			isRegExp = TRUE ;					/* Remember we have matched a pattern */

			break ;



		   case V4DPI_PntType_RegExpPattern:
			i = vregexp_RegExec((regex_t *)((char *)cpt + cpt->Bytes - sizeof(regex_t)),UCretASC(&vhp->Page[vhp->curStart]),10,matches,0) ;
			if (i) { v_Msg(ctx,NULL,"StrRegExpNoMtch",intmodx,V4IM_Tag_RegExp,cpt) ; ipt = NULL ; goto html_end ; } ;
			regexpStart = &vhp->Page[vhp->curStart] ;		/* Remember starting position */
			vhp->curStart += matches[0].rm_so ;			/* Set current start to begin of pattern */
			isRegExp = TRUE ;					/* Remember we have matched a pattern */
		 } ;
		break ;
	      case -V4IM_Tag_Reset:
		for(vhp=gpi->vhp;vhp!=NULL;)
		 { struct V4HTML__Page *vhpx ;
		   if(vhp->PageType != V4HTML_PageType_Goto && vhp->Page != NULL) free(vhp->Page) ;
		   vhpx=vhp->PriorHTMLPage ; free(vhp) ; vhp = vhpx ;
		 } ;
		gpi->vhp = NULL ;
		break ;
	      case V4IM_Tag_Row:
		MHVHP
		row = v4im_GetPointInt(&ok,cpt,ctx) ;
		if (!v4html_PositionRowColumn(gpi->vhp,row,0,ctx->ErrorMsgAux))
		 { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
		break ;
	      case -V4IM_Tag_Row:
		MHVHP
		if (gpi->vhp->NumRows <= 0)
		 { v_Msg(ctx,NULL,"HTMLNotInTABLE",intmodx,-V4IM_Tag_Row,ax) ; ipt = NULL ; goto html_end ; } ;
		ipt = respnt ; intPNTv(ipt,gpi->vhp->CurRow) ; break ;
	      case -V4IM_Tag_Source:
		MHVHP
		if (!v4html_GetCurrentSrc(gpi->vhp,&respnt->Value.UCVal[1],ctx->ErrorMsgAux))
		 { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; ipt = NULL ; goto html_end ; } ;
		ipt = respnt ; uccharPNT(ipt) ; UCCHARPNTBYTES1(ipt) ; break ;
	      case V4IM_Tag_Submit:
		MHVHP vhp=gpi->vhp ;
		v4im_GetPointUC(&ok,tbuf,V4TMBufMax,cpt,ctx) ; if (!ok) break ;
		UCSTRTOUPPER(tbuf) ;
		for(i=0,ok=FALSE;i<vhp->InputCount;i++)
		 { if (vhp->Input[i].Type != V4HTML_InputType_Submit) continue ;
		   if (UCstrcmp(tbuf,vhp->Input[i].UCName) == 0) { ok = TRUE ; continue ; } ;
		   vhp->Input[i] = vhp->Input[--vhp->InputCount] ; i-- ;
		 } ;
		if (!ok) { v_Msg(ctx,NULL,"HTMLNoSUBMIT",intmodx) ; ipt = NULL ; goto html_end ; } ;
		break ;
	      case V4IM_Tag_Table:
		break ;
	      case -V4IM_Tag_URL:
		MHVHP b = UCTBUF1 ;
		if (!v4html_GetHyperLink(gpi->vhp,UCTBUF1,ctx->ErrorMsgAux))
		 { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
		ipt = respnt ; uccharPNTv(ipt,b) ; break ;
	      case -V4IM_Tag_UC:	uc = TRUE ; break ;
	      case V4IM_Tag_User:	v4im_GetPointUC(&ok,UserName,UCsizeof(UserName),cpt,ctx) ; break ;
	      case V4IM_Tag_Value:
		MHVHP
		if (formfield[0] == UCEOS)
		 { v_Msg(ctx,NULL,"HTMLNoField",intmodx,ax,V4IM_Tag_Value,V4IM_Tag_Field) ; ipt = NULL ; goto html_end ; } ;
		v4im_GetPointUC(&ok,tbuf,V4TMBufMax,cpt,ctx) ; if (!ok) break ;
		if (!v4html_SetFormValue(gpi->vhp,formfield,tbuf,ctx->ErrorMsgAux))
		 { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
		ZUS(formfield) ;
		break ;					/* Just save form name, need Value::xxx to set */
	      case -V4IM_Tag_Value:
		MHVHP
		if (formfield[0] == UCEOS)
		 { v_Msg(ctx,NULL,"HTMLNoField",intmodx,ax,-V4IM_Tag_Value,V4IM_Tag_Field) ; ipt = NULL ; goto html_end ; } ;
		if ((b = v4html_GetFormValue(vhp,formfield,ctx->ErrorMsgAux)) == NULL)
		 { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; ipt = NULL ; goto html_end ; } ;
		ipt = respnt ; uccharPNTv(ipt,b) ; break ;
	   } ;
	 } ;

	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax-1) ; ipt = NULL ; goto html_end ; } ;
	return(ipt) ;

html_end:
	if (ipt == NULL)
	 { if (UCstrlen(ctx->ErrorMsg) >= 255) ctx->ErrorMsg[250] = UCEOS ; UCstrcpy(LastErrorMsg,ctx->ErrorMsg) ;
	   REGISTER_ERROR(0) ;
	 } ;
	return(ipt) ;
}
#endif

#define VHSTS_Initial_ResBuf 0x1000	/* Initial size of result buffer if Content-Length not in returned headers */
#define VHTTP_Arg_Max 128		/* Max number of POST arguments */

#define VHTTP_Verb_Get 1		/* Do a GET */
#define VHTTP_Verb_Post 2		/* Do a POST */
#define VHTTP_Verb_Put 3
#define VHTTP_Verb_Delete 4

struct vHTTP__Result {
  LENMAX contentLength ;		/* Updated with number of bytes to be in returned content */
  ETYPE contentType ;			/* Updated with content type- see V4HTML_ContentType_xxx */
  struct UC__File *dstFile ;		/* If not NULL then write returned data to this file */
  LENMAX bytes ;			/* Number of bytes actually in returned content */
  LENMAX bytesAlloc ;			/* Number of bytes allocated in dstBuffer */
  BYTE *dstBuffer ;			/* Updated to allocated buffer with returned content */
  UCCHAR **hdrBufPtr ;			/* If not null then updated with received HTTP headers */
} ;

struct lcl__args {
  COUNTER argCnt ;			/* Number of arguments */
  struct {
    LENMAX bytes ;			/* Number of bytes for this argument */
    LOGICAL isFile ;			/* TRUE if data is a file name */
    LENMAX fileBytes ;			/* If file, then this is eventually set with # bytes in file */
    char name[64] ;			/* Name of the argument */
    BYTE *data ;			/* Pointer to the argument (or filename if isFile is TRUE) */
   } arg[VHTTP_Arg_Max] ;
} ;


#ifdef HAVEWININET
//#define HAVEWINHTTP
/*	The WINDOWS version of this routine uses the WinHTTPxxx routines that are a part of the Windows release
	Need to add Winhttp.lib to VS configuration properties->C->Linker->Additional Dependencies for build to go
*/
#ifdef HAVEWINHTTP
#include <Winhttp.h>
#else
#include <Wininet.h>
#endif

LOGICAL vHTTP_SendToServer(la,url,httpVerb,objectName,contentType,cookies,authorization,data,dataBytes,vhr,verbose,logFile,errbuf)
  struct lcl__args *la ;
  UCCHAR *url ;
  ETYPE httpVerb ;			/* See VHTTP_Verb_xxx */
  UCCHAR *objectName ;
  UCCHAR *contentType ;
  UCCHAR *cookies ;
  UCCHAR *authorization ;		/* If not NULL then [userName]:[password] */
  BYTE *data ;
  LENMAX dataBytes ;
  LOGICAL verbose ;
  UCCHAR *logFile ;
  struct vHTTP__Result *vhr ;
  UCCHAR *errbuf ;
{
  DWORD dwBytesWritten = 0;
  LOGICAL ok = FALSE ; char *module ;
  HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL ;
  URL_COMPONENTSW urlComp;
  DWORD dwSize = 0;
  UCCHAR httpHeaders[4096],hostName[512] ; UCCHAR *verb ;

#ifdef V4UNICODE
#define UCInternetCrackUrl InternetCrackUrlW
#define UCInternetOpen InternetOpenW
#define UCInternetConnect InternetConnectW
#define UCHttpOpenRequest HttpOpenRequestW
#define UCHttpAddRequestHeaders HttpAddRequestHeadersW
#define UCHttpSendRequest HttpSendRequestW
#define UCHttpQueryInfo HttpQueryInfoW
#define UCHttpQueryInfo HttpQueryInfoW
#define UCFtpSetCurrentDirectory FtpSetCurrentDirectoryW
#define UCFtpGetFile FtpGetFileW
#define UCFtpFindFirstFile FtpFindFirstFileW
#define UCInternetFindNextFile InternetFindNextFileW
#define UCFtpGetFile FtpGetFileW
#define UCFtpPutFile FtpPutFileW
#define UCInternetGetLastResponseInfo InternetGetLastResponseInfoW
#else
#define UCInternetCrackUrl InternetCrackUrl
#define UCInternetOpen InternetOpen
#define UCInternetConnect InternetConnect
#define UCHttpOpenRequest HttpOpenRequest
#define UCHttpAddRequestHeaders HttpAddRequestHeaders
#define UCHttpSendRequest HttpSendRequest
#define UCHttpQueryInfo HttpQueryInfo
#define UCHttpQueryInfo HttpQueryInfo
#define UCFtpSetCurrentDirectory FtpSetCurrentDirectory
#define UCFtpGetFile FtpGetFile
#define UCFtpFindFirstFile FtpFindFirstFile
#define UCInternetFindNextFile InternetFindNextFile
#define UCFtpGetFile FtpGetFile
#define UCFtpPutFile FtpPutFile
#define UCInternetGetLastResponseInfo InternetGetLastResponseInfo
#endif

#define HTTPOK(MODULE) if (!ok) { module = "WinHttp" #MODULE ; goto mod_fail ; } ;

/*	Crack/Decode URL */
	memset(&urlComp,0,sizeof(urlComp)) ; urlComp.dwStructSize = sizeof(urlComp);
	switch (httpVerb)
	 { default:			verb = UClit("GET") ; break ;
	   case VHTTP_Verb_Delete:	verb = UClit("DELETE") ; break ;
	   case VHTTP_Verb_Post:	verb = UClit("POST") ; break ;
	   case VHTTP_Verb_Put:		verb = UClit("PUT") ; break ;
	 } ;
/*	Set required component lengths to non-zero so that they are cracked */
	urlComp.dwSchemeLength = (DWORD)-1 ; urlComp.dwHostNameLength = (DWORD)-1 ; urlComp.dwUrlPathLength = (DWORD)-1 ; urlComp.dwExtraInfoLength = (DWORD)-1 ;
#ifdef HAVEWINHTTP
	ok = WinHttpCrackUrl(url,UCstrlen(url),0,&urlComp) ;
#else
	ok = UCInternetCrackUrl(url,UCstrlen(url),0,&urlComp) ;
#endif
	if (ok) ok = (urlComp.dwHostNameLength < UCsizeof(hostName)) ;
	HTTPOK(CrackUrl)
	UCstrncpy(hostName,urlComp.lpszHostName,urlComp.dwHostNameLength) ; hostName[urlComp.dwHostNameLength] = UCEOS ;
	if (objectName == NULL) objectName = urlComp.lpszUrlPath ;	/* Pull from URL if not explicitly given */
/*	Use WinHttpOpen to obtain a session handle */
#ifdef HAVEWINHTTP
	hSession = WinHttpOpen(UClit("V4"),WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS, 0) ;
#else
	hSession = UCInternetOpen(UClit("V4"),INTERNET_OPEN_TYPE_DIRECT,NULL,NULL, 0) ;
#endif
	ok = hSession != NULL ; HTTPOK(Open)
/*	Specify an HTTP server */
#ifdef HAVEWINHTTP
        hConnect = WinHttpConnect( hSession,hostName,urlComp.nPort, 0) ;
#else
	hConnect = UCInternetConnect(hSession,hostName,urlComp.nPort,NULL,NULL,INTERNET_SERVICE_HTTP,0,0) ;
#endif
	ok = hConnect != NULL ; HTTPOK(Connect)
/*	Create an HTTP Request handle */
#ifdef HAVEWINHTTP
        hRequest = WinHttpOpenRequest(hConnect,verb,objectName,NULL /* HTTP/1.1 */,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,
					(urlComp.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0) /* Set to WINHTTP_FLAG_SECURE for HTTPS */) ;
#else
	{ UCCHAR *allowList[] = { UClit("*/*"),NULL } ;
	  hRequest = UCHttpOpenRequest(hConnect,verb,objectName,NULL,NULL,(LPCWSTR *)&allowList,(urlComp.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_FLAG_SECURE : 0)|INTERNET_FLAG_NO_COOKIES,0) ;
	}
#endif
	ok = hRequest != NULL ; HTTPOK(WinHttpOpenRequest)

/*	Send a Request */
	UCsprintf(httpHeaders,UCsizeof(httpHeaders),UClit("Content-Type: %s\n"),contentType) ;
	if (UCnotempty(authorization))
	 { UCCHAR buf1[2048],buf2[2048] ;
	   if (VUC_Encode64(authorization,UCstrlen(authorization),buf2,UCsizeof(buf2),errbuf) < 0) { ok = FALSE ; goto cleanup ; } ;
	   UCsprintf(buf1,UCsizeof(buf1),UClit("Authorization: Basic %s"),buf2) ;
	   UCstrcat(httpHeaders,UCTXTEOL) ; UCstrcat(httpHeaders,buf1) ;
	 } ;
	if (UCstrlen(cookies) > 0)
	 { UCCHAR xHeader[8192] ;
	   UCstrcpy(xHeader,UClit("Cookie: ")) ; UCstrcat(xHeader,cookies) ;
#ifdef HAVEWINHTTP
	   ok = WinHttpAddRequestHeaders(hRequest,xHeader,-1,WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON) ; HTTPOK(AddRequestHeaders) ;
#else
	   ok = UCHttpAddRequestHeaders(hRequest,xHeader,-1,HTTP_ADDREQ_FLAG_ADD|HTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON) ;  HTTPOK(AddRequestHeaders) ;
#endif
	 } ;
		
#ifdef HAVEWINHTTP
        ok = WinHttpSendRequest(hRequest, 
                                       httpHeaders,				/* WINHTTP_NO_ADDITIONAL_HEADERS or Additional headers, ex: "HEADER: value\r\n" */
                                       -1L,					/* Length of headers, if -1L then assumes above headers are UCEOS terminated */
				        WINHTTP_NO_REQUEST_DATA, 0,		/* optional data - don't need it */
                                       dataBytes,				/* Size of POST data to be sent below */
				       0);					/* Pointer passed to callback functions */
#else
	ok = UCHttpSendRequest(hRequest,httpHeaders,-1,data,dataBytes) ;
#endif
	HTTPOK(SendRequest)
/*	Write data to the server */
#ifdef HAVEWINHTTP
	if (dataBytes > 0)
        ok = WinHttpWriteData(hRequest,data,dataBytes,&dwBytesWritten) ; HTTPOK(WriteData)
#else
//	if (dataBytes > 0)
//        ok = InternetWriteFile(hRequest,data,dataBytes,&dwBytesWritten) ; HTTPOK(WriteData)
#endif
#ifdef HAVEWINHTTP
        ok = WinHttpReceiveResponse(hRequest,NULL) ; HTTPOK(ReceiveResponse)
#else
//	ok = HttpEndRequest(hRequest,NULL,0,0) ; HTTPOK(EndRequest)
#endif

/*	Use WinHttpQueryHeaders to obtain the size of the buffer */
#ifdef HAVEWINHTTP
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,WINHTTP_HEADER_NAME_BY_INDEX, NULL,&dwSize, WINHTTP_NO_HEADER_INDEX) ;

	ok = (GetLastError() == ERROR_INSUFFICIENT_BUFFER) ; HTTPOK(WinHttpQueryHeaders)
	{ UCCHAR *hdrs,*b,*b2 ;
	  hdrs = v4mm_AllocUC(dwSize) ;
/*	  Now, use WinHttpQueryHeaders to retrieve the header */
	  ok = WinHttpQueryHeaders( hRequest,WINHTTP_QUERY_RAW_HEADERS_CRLF,WINHTTP_HEADER_NAME_BY_INDEX,hdrs, &dwSize, WINHTTP_NO_HEADER_INDEX) ;
	  HTTPOK(WinHttpQueryHeaders)
	  for(b=hdrs;;)
	   { b2 = UCstrchr(b,'\n') ; if (b2 == NULL) b2 = UCstrchr(b,'\r') ; if (b2 != NULL) *b2 = UCEOS ;
	     if (UCstrncmpIC(UClit("CONTENT-TYPE"),b,12) == 0)
	      { for(b=b+12;!vuc_IsWSpace(*b);b++) {} ;
	        if (UCstrncmpIC(b,UClit("TEXT/HTML"),9) == 0) { vhr->contentType = V4HTML_ContentType_HTML ; }
		 else if (UCstrncmpIC(b,UClit("TEXT/PLAIN"),10) == 0)  {vhr->contentType = V4HTML_ContentType_Text ; }
		 else if (UCstrncmpIC(b,UClit("APPLICATION/"),12) == 0)  {vhr->contentType = V4HTML_ContentType_Appl ; }
		 else if (UCstrncmpIC(b,UClit("IMAGE/"),6) == 0)  {vhr->contentType = V4HTML_ContentType_Image ; }
		 else if (UCstrncmpIC(b,UClit("AUDIO/"),6) == 0)  {vhr->contentType = V4HTML_ContentType_Sound ; }
		 else if (UCstrncmpIC(b,UClit("TEXT/XML"),8) == 0)  {vhr->contentType = V4HTML_ContentType_XML ; } ;
	      } else if (UCstrncmpIC(UClit("CONTENT-LENGTH"),b,14) == 0)
	      { for(;*b!=' ';b++) {} ;
		vhr->contentLength = UCstrtol(b,&b,10) ; if (*b > ' ') vhr->contentLength = UNUSED ;
	      } ;
	     if (b2 == NULL) break ;
	     for(b=b2+1;!vuc_IsLetter(*b);b++) {} ;
            } ;
	   if (vhr->hdrBufPtr == NULL) { v4mm_FreeChunk(hdrs) ; }
	    else { *vhr->hdrBufPtr = hdrs ; } ;
	}
#else
	{ DWORD bytes = sizeof vhr->contentLength ;
	  UCCHAR *hdrBuf = NULL ;
	  if (!HttpQueryInfo(hRequest,HTTP_QUERY_FLAG_NUMBER|HTTP_QUERY_CONTENT_LENGTH,&vhr->contentLength,&bytes,NULL))
	   vhr->contentLength = UNUSED ;
/*	  Grab all the headers - first call to get size */
	  if (vhr->hdrBufPtr != NULL)
	   { bytes = 0 ;
	     UCHttpQueryInfo(hRequest,HTTP_QUERY_RAW_HEADERS_CRLF,(LPVOID)hdrBuf,&bytes,NULL) ;
	     if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	      { *vhr->hdrBufPtr = v4mm_AllocChunk(bytes,FALSE) ;
		UCHttpQueryInfo(hRequest,HTTP_QUERY_RAW_HEADERS_CRLF,(LPVOID)*vhr->hdrBufPtr,&bytes,NULL) ;
	      } ;
	   } ;

	}
#endif
 /*	Pull in data until there is nothing left */
	{ 
	  if (vhr->dstFile == NULL)
	   { vhr->bytesAlloc = (vhr->contentLength != UNUSED ? vhr->contentLength : VHSTS_Initial_ResBuf) ;
	     vhr->dstBuffer = v4mm_AllocChunk(vhr->bytesAlloc+16,FALSE) ;		/* Allocate a little extra for terminating EOS */
	   } else { vhr->dstBuffer = NULL ; } ;
	  vhr->bytes = 0 ;
	  for(;;)
	   { char *dbuf ; LENMAX dwDownloaded ;
	     dwSize = 0 ;
#ifdef HAVEWINHTTP
	     ok = WinHttpQueryDataAvailable(hRequest,&dwSize) ;
#else
	     ok = InternetQueryDataAvailable(hRequest,&dwSize,0,0) ;
#endif


	     if (dwSize <= 0) break ;
	     HTTPOK(QueryDataAvailable)
	     dbuf = v4mm_AllocChunk(dwSize,FALSE);
	     if (vhr->dstBuffer != NULL && vhr->bytes+dwSize > vhr->bytesAlloc)
	      { vhr->bytesAlloc = (int)(vhr->bytes*1.2) ; vhr->dstBuffer = realloc(vhr->dstBuffer,vhr->bytesAlloc+16) ; } ; 
#ifdef HAVEWINHTTP
	     ok = WinHttpReadData(hRequest,(LPVOID)dbuf,dwSize,&dwDownloaded) ; HTTPOK(ReadData)
#else
	     ok = InternetReadFile(hRequest,(LPVOID)dbuf,dwSize,&dwDownloaded) ; HTTPOK(ReadFile)
#endif

	     if (vhr->dstFile == NULL) { memcpy(vhr->dstBuffer+vhr->bytes,dbuf,dwSize) ; }
	      else { fwrite(dbuf,dwSize,1,vhr->dstFile->fp) ; } ;
	     vhr->bytes += dwSize ;
	     v4mm_FreeChunk(dbuf) ;
           } ;
	  if (vhr->dstBuffer != NULL) vhr->dstBuffer[vhr->bytes] = '\0' ;
	}
	ok = TRUE ; goto cleanup ;

mod_fail:
	v_Msg(NULL,errbuf,"@Error in %1s: %2O",module,GetLastError()) ;
	ok = FALSE ; goto cleanup ;
cleanup:
#ifdef HAVEWINHTTP
	if (hRequest) WinHttpCloseHandle(hRequest) ;
	if (hConnect) WinHttpCloseHandle(hConnect) ;
	if (hSession) WinHttpCloseHandle(hSession) ;
#else
	if (hRequest) InternetCloseHandle(hRequest) ;
	if (hConnect) InternetCloseHandle(hConnect) ;
	if (hSession) InternetCloseHandle(hSession) ;
#endif
	return(ok) ;
}
#endif

#ifdef HAVECURL
/*	The NON-WINDOWS version of vHTTP_SendToServer relies on the libCURL suite of routines. The necessary include & library files MUST be available for this to compile properly
	It is also possible TO BUILD ON WINDOWS - see details in v3build.c
*/
#ifdef WINNT
#include <curl.h>
#else
//#include <curl/curl.h>
#include "/usr/include/curl/curl.h"	/* This seems to work better? */
#endif

#define HTTP_OPTS_MAX_HDRS 25

struct http__SendToServOptions {
  UCCHAR *url ;			/* Target URL */
  ETYPE httpVerb ;		/* HTTP verb */
  UCCHAR *objectName ;		/* Object after URL */
  UCCHAR contentType[512] ;		/* Content Type */
  UCCHAR *cookies ;		/* Any cookies (delimited with ' ; ' */
  UCCHAR authorization[512] ;	/* Authorization: username:password */
  UCCHAR userAgent[512] ;	/* User-agent */
  BYTE *data ;			/* Data to be passed */
  LENMAX dataBytes ;		/* Number of bytes of data */
  UCCHAR *sslCert ;		/* SSL certificate - path to file */
  UCCHAR *sslCertPassword ;	/* Certificate password */
  UCCHAR *sslCypherList ;	/* SSL Cypher List */
  struct lcl__args *la ;	/* Structure with arguments */
  struct vHTTP__Result *vhr ;	/* Header results structure */
  LOGICAL verbose ;		/* Debug verbosity */
  UCCHAR *logFile ;		/* Log file for output */
  UCCHAR *errbuf ;		/* Gets error message */
  UDTVAL UDT ;			/* Optional date-time stamp */
  COUNTER maxWait ;		/* Max wait time (seconds) for response, 0 for infinite */
  INDEX hdrCount ;		/* Number of optional headers below */
  struct {
    UCCHAR hdrName[64] ;	/* Header name */
    UCCHAR hdrValue[2048] ;	/* Header value */
   } hdrs[HTTP_OPTS_MAX_HDRS] ;
} ;

LOGICAL curlNeedInit = TRUE ;		/* Used to track whether or not one-time CURL init has been done */
#define CURL_MAX_HDRBUF_CHARS 2048	/* Maximum number of bytes we can hold in header buffer */

size_t curlWriteFunc(dataPtr,dataSize,dataNum,vhr)
  char *dataPtr ;
  size_t dataSize, dataNum ;
  struct vHTTP__Result *vhr ;
{ size_t dataBytes ;

//printf("in curWriteFunc dataBytes=%d, bytesAlloc=%d, bytes=%d, contentLength=%d\n",dataBytes,vhr->bytesAlloc,vhr->bytes,vhr->contentLength) ;
/*	If writing to a file then just do it */
	if (vhr->dstFile != NULL)
	 { return(fwrite(dataPtr,dataSize,dataNum,vhr->dstFile->fp)) ;
	 } ;
	dataBytes = dataSize * dataNum ;
	if (vhr->bytesAlloc == 0)
	 { vhr->bytesAlloc = (vhr->contentLength > (B64INT)dataBytes ? vhr->contentLength : (dataBytes * 2)) ; vhr->dstBuffer = v4mm_AllocChunk(vhr->bytesAlloc,FALSE) ;
//printf("   initial alloc = %d\n",vhr->bytesAlloc) ;
	 } ;
	if (vhr->bytes+dataBytes + 64 > vhr->bytesAlloc)
	 { LENMAX need = vhr->bytes + dataBytes - vhr->bytesAlloc ;
	   if (need < 0x1000) { need = 0x1000 ; }
	    else { need *= 1.5 ; } ;
	   vhr->bytesAlloc += need ; vhr->dstBuffer = realloc(vhr->dstBuffer,vhr->bytesAlloc) ;
//printf("   realloc: need=%d, bytesAlloc =%d\n",need,vhr->bytesAlloc) ;
	 } ;
	memcpy(vhr->dstBuffer+vhr->bytes,dataPtr,dataBytes) ;
	vhr->bytes += dataBytes ;
//printf("   bytes on exit=%d\n",vhr->bytes) ;
	vhr->dstBuffer[vhr->bytes] = '\0' ;
	return(dataBytes) ;
}
size_t curlWriteHeader(dataPtr,dataSize,dataNum,vhr)
  char *dataPtr ;
  size_t dataSize, dataNum ;
  struct vHTTP__Result *vhr ;
{ char buf[1024],*bp ;
  size_t hdrBytes ; char *hdrs, *b, *b2 ;

	hdrBytes = dataSize * dataNum ;
/*	Copy header into local buffer BUT HAVE TO ALLOCATE IF TOO LARGE */
	bp = (hdrBytes + 2 > sizeof buf ? v4mm_AllocChunk(hdrBytes+2,FALSE) : buf) ;
	strncpy(bp,dataPtr,hdrBytes) ; bp[hdrBytes] = UCEOS ; bp[hdrBytes+1] = UCEOS;
/*	Should have headers as one chunk, no need to allocate - just scan for info */
	hdrs = bp ;
	for(b=hdrs;*b!=UCEOS;)
	 { b2 = strchr(b,'\r') ; if (b2 == NULL) b2 = strchr(b,'\n') ;
	   if (b2 != NULL) *b2 = UCEOS ;
	   if (STRNCMPIC("CONTENT-TYPE",b,12) == 0)
	    { for(b=b+12;!vuc_IsWSpace(*b);b++) {} ;
	      if (STRNCMPIC(b,"TEXT/HTML",9) == 0) { vhr->contentType = V4HTML_ContentType_HTML ; }
	       else if (STRNCMPIC(b,"TEXT/PLAIN",10) == 0)  {vhr->contentType = V4HTML_ContentType_Text ; }
	       else if (STRNCMPIC(b,"APPLICATION/",12) == 0)  {vhr->contentType = V4HTML_ContentType_Appl ; }
	       else if (STRNCMPIC(b,"IMAGE/",6) == 0)  {vhr->contentType = V4HTML_ContentType_Image ; }
	       else if (STRNCMPIC(b,"AUDIO/",6) == 0)  {vhr->contentType = V4HTML_ContentType_Sound ; }
	       else if (STRNCMPIC(b,"TEXT/XML",8) == 0)  {vhr->contentType = V4HTML_ContentType_XML ; } ;
	    } else if (STRNCMPIC("CONTENT-LENGTH",b,14) == 0)
	    { for(;*b!=' ';b++) {} ;
	      vhr->contentLength = strtol(b,&b,10) ;
	      if (*b > ' ')
	       vhr->contentLength = UNUSED ;
	    } ;
	   if (b2 == NULL) break ;
	   for(b=b2+1;;b++)
	    { if (*b == UCEOS || *b > ' ') break ;
	    } ;
	 } ;
/*	Save headers ? */
	if (vhr->hdrBufPtr != NULL)
	 { if (hdrBytes + UCstrlen(*vhr->hdrBufPtr) + 1 < CURL_MAX_HDRBUF_CHARS) 
	    { UCstrcatToUC(*vhr->hdrBufPtr,hdrs) ; UCstrcat(*vhr->hdrBufPtr,UClit("\r")) ; } ;
	 } ;
	if (bp != buf) v4mm_FreeChunk(bp) ;
	return(hdrBytes) ;
}

int debug_callback(CURL *handle, curl_infotype type, char *data, size_t size,void *userptr)
{ char buf[512] ;
  if (size > 511) size = 511 ;
  strncpy(buf,data,size) ; buf[size] = '\0' ;
//  printf("\n\nDebug Type #%d\n",type) ;
  if (type != 5 && type != 6) printf("%s",buf) ;
  return(0) ;
}

LOGICAL vHTTP_SendToServer(hsso)
  struct http__SendToServOptions *hsso ;
//LOGICAL vHTTP_SendToServer(la,url,httpVerb,objectName,contentType,cookies,authorization,data,dataBytes,vhr,verbose,logFile,errbuf)
//  struct lcl__args *la ;
//  UCCHAR *url ;
//  ETYPE httpVerb ;
//  UCCHAR *objectName ;
//  UCCHAR *contentType ;
//  UCCHAR *cookies ;
//  UCCHAR *authorization ;
//  BYTE *data ;
//  LENMAX dataBytes ;
//  struct vHTTP__Result *vhr ;
//  LOGICAL verbose ;
//  UCCHAR *logFile ;
//  UCCHAR *errbuf ;
{

  static CURL *curl = NULL ;
  char curlErrs[CURL_ERROR_SIZE], *curlEMne ;
  struct curl_slist *curlHList = NULL ;
  struct curl_httppost *firstArg = NULL, *lastArg = NULL ;
  CURLcode res ; LOGICAL ok ;

	if (curlNeedInit) { curl_global_init(CURL_GLOBAL_ALL) ; curlNeedInit = FALSE ; } ;

//veh130801	If curl has been called before then clean up from prior run. There seems to be issues with the cleanup failing so I am moving it so that the prior HTTP() call succeeds */
	if (curl != NULL)
	 { curl_easy_cleanup(curl); } ;

	curl = curl_easy_init() ;

#define CURLOPTION(_OPT,_ARG) res = curl_easy_setopt(curl,_OPT,_ARG) ; if (res != CURLE_OK) { curlEMne = #_OPT ; goto curl_fail ; } ;
/*	Set session options with as many of these as necessary */

	CURLOPTION(CURLOPT_NOPROGRESS,1)				/* Shut off progress meter */
	CURLOPTION(CURLOPT_WRITEFUNCTION,curlWriteFunc)		/* curlWriteFunc(char *ptr, size_t size, size_t nmemb, void *userdata) */
	  CURLOPTION(CURLOPT_WRITEDATA,hsso->vhr) ;		/*  (want to pass this as extra argument) */
	CURLOPTION(CURLOPT_HEADERFUNCTION,curlWriteHeader)		/* Same as above but to receive header info */
	  CURLOPTION(CURLOPT_WRITEHEADER,hsso->vhr) ;
	CURLOPTION(CURLOPT_ERRORBUFFER,curlErrs)			/* To receive readable error message */

//	curl_easy_setopt(curl, CURLOPT_TRANSFER_ENCODING, 1);

//	CURLOPTION(CURLOPT_FAILONERROR,1)

	if (hsso->verbose)					/* Switch on full protocol/debug output */
	 { CURLOPTION(CURLOPT_VERBOSE, 1L) ;
	   if (hsso->logFile != NULL) { UCfreopen(hsso->logFile,"w",stdout ) ; } ;
//	   curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION,debug_callback);	/* Uncomment this for really verbose tracing */
	 } ;


/*	If objectName not NULL then append to end of URL */
	if (hsso->objectName == NULL) { char urlObj[4096] ; UCstrcpyToASC(urlObj,hsso->url) ; CURLOPTION(CURLOPT_URL,urlObj) }
	 else { char urlObj[4096] ;
		UCstrcpyToASC(urlObj,hsso->url) ; if (urlObj[strlen(urlObj)-1] != '/') strcat(urlObj,"/") ; UCstrcatToASC(urlObj,hsso->objectName) ;
		CURLOPTION(CURLOPT_URL,urlObj) ;
	      } ;

	CURLOPTION(CURLOPT_SSLVERSION,CURL_SSLVERSION_DEFAULT)
	CURLOPTION(CURLOPT_SSL_VERIFYPEER,0)

	if (UCnotempty(hsso->authorization))
	 { CURLOPTION(CURLOPT_USERPWD,UCretASC(hsso->authorization))	/* Authorization password- [user name]:[password] */
	   CURLOPTION(CURLOPT_HTTPAUTH,CURLAUTH_BASIC) ;			/* (use CURLAUTH_BASIC if DIGEST gives us trouble) */
	 } ;

	if (hsso->cookies != NULL)				/* Sets outgoing cookies */
	 { CURLOPTION(CURLOPT_COOKIE,UCretASC(hsso->cookies)) ; } ;

	switch (hsso->httpVerb)
	 { default:			CURLOPTION(CURLOPT_HTTPGET,1) ; break ;
	   case VHTTP_Verb_Delete:	CURLOPTION(CURLOPT_POST,1) ; CURLOPTION(CURLOPT_CUSTOMREQUEST,"DELETE") ;
					break ;
	   case VHTTP_Verb_Post:	CURLOPTION(CURLOPT_POST,1) ; break ;
	   case VHTTP_Verb_Put:		CURLOPTION(CURLOPT_CUSTOMREQUEST,"PUT") ; /* CURLOPTION(CURLOPT_PUT,1) ; */
					break ;
	 } ;

	if (hsso->maxWait > 0)
	 { CURLOPTION(CURLOPT_NOSIGNAL,1) ; CURLOPTION(CURLOPT_TIMEOUT,hsso->maxWait) ; }
	 else { CURLOPTION(CURLOPT_NOSIGNAL,0) ; CURLOPTION(CURLOPT_TIMEOUT,0) ; } ;

	if (hsso->sslCert != NULL)
	 { char certFile[V_FileName_Max] ;
	   UCstrcpyToASC(certFile,hsso->sslCert) ; 
	   CURLOPTION(CURLOPT_SSLCERT,certFile) ;
	   if (hsso->sslCertPassword != NULL)
	    { char certPW[512] ; UCstrcpyToASC(certPW,hsso->sslCertPassword) ;
	      CURLOPTION(CURLOPT_KEYPASSWD,certPW) ;
	    } ;
	 } ;

	if (hsso->sslCypherList != NULL)
	 { char sslCypherList[128] ; UCstrcpyToASC(sslCypherList,hsso->sslCypherList) ;
	   CURLOPTION(CURLOPT_SSL_CIPHER_LIST,sslCypherList) ;
	 } ;

	if (UCnotempty(hsso->userAgent))
	 { char userAgent[512] ; UCstrcpyToASC(userAgent,hsso->userAgent) ; CURLOPTION(CURLOPT_USERAGENT,userAgent) ;
	 } ;

/*	For re-directs */
	CURLOPTION(CURLOPT_FOLLOWLOCATION,1) ;
	CURLOPTION(CURLOPT_POSTREDIR,CURL_REDIR_POST_ALL) ;

	if (hsso->UDT != 0)
	 { char hbuf[1024] ; UCCHAR udtBuf[128] ;
	   v_FormatDate(&hsso->UDT,V4DPI_PntType_UDT,VCAL_CalType_Gregorian,VCAL_TimeZone_GMT,UClit("www, d mmm yyyy 0h:0n:0s"),udtBuf) ;
	   UCstrcat(udtBuf,UClit(" GMT")) ;
	   sprintf(hbuf,"Date: %s",UCretASC(udtBuf)) ; curlHList = curl_slist_append(curlHList,hbuf);
	 } ;

	{ char hbuf[2048] ;
	  sprintf(hbuf,"Content-Type: %s",UCretASC(hsso->contentType)) ; curlHList = curl_slist_append(curlHList,hbuf);	/* For each header */
	}

	if (hsso->hdrCount > 0)
	 { INDEX i ;
	   for(i=0;i<hsso->hdrCount;i++)
	    { char hbuf[2014] ;
	      sprintf(hbuf,"%s: %s",UCretASC(hsso->hdrs[i].hdrName),UCretASC(hsso->hdrs[i].hdrValue)) ; curlHList = curl_slist_append(curlHList,hbuf);
	    } ;
	 } ;

	curlHList = curl_slist_append(curlHList, "Expect:");


//printf("hsso->la = %p, hsso->dataBytes = %d\n",hsso->la,hsso->dataBytes) ;
	if (hsso->la != NULL)					/* Do we have explicit POST arguments - if so then handle, otherwise assume data/dataBytes has already-formatted POST data */
	 { INDEX i ;
	   for(i=0;i<hsso->la->argCnt;i++)
	    { if (hsso->la->arg[i].isFile)
	       { curl_formadd(&firstArg,&lastArg,CURLFORM_PTRNAME,hsso->la->arg[i].name,CURLFORM_FILE,hsso->la->arg[i].data,CURLFORM_END) ;
	       } else
	       { curl_formadd(&firstArg,&lastArg,CURLFORM_PTRNAME,hsso->la->arg[i].name,CURLFORM_PTRCONTENTS,hsso->la->arg[i].data,CURLFORM_END) ;
	       } ;
	    } ;
	   CURLOPTION(CURLOPT_HTTPPOST,firstArg)			/* Linked list of curl_httppost structures for multipart/formdata posts */

	 } else if ((hsso->dataBytes > 0) || (hsso->httpVerb == VHTTP_Verb_Post))
	 { CURLOPTION(CURLOPT_POSTFIELDS,hsso->data)			/* Post data */
	   CURLOPTION(CURLOPT_POSTFIELDSIZE,hsso->dataBytes)		/* Defines length of above data */
	 } ;

	if (curlHList != NULL)
	 { CURLOPTION(CURLOPT_HTTPHEADER,curlHList) ; } ;

	if (hsso->vhr->dstFile != NULL) hsso->vhr->dstBuffer = NULL ;
	hsso->vhr->bytes = 0 ; hsso->vhr->bytesAlloc = 0 ;

/*	If we want to capture all the headers then allocate buffer for them */
	if (hsso->vhr->hdrBufPtr != NULL)
	 { *hsso->vhr->hdrBufPtr = v4mm_AllocUC(CURL_MAX_HDRBUF_CHARS) ; ZUS(*hsso->vhr->hdrBufPtr) ; } ;

/*	Actually perform HTTP request */
	res = curl_easy_perform(curl) ;
	if (res != CURLE_OK) { curlEMne = "curl_easy_perform" ; goto curl_fail ; } ;

	ok = TRUE ; goto curl_cleanup ;

curl_fail:
	v_Msg(NULL,hsso->errbuf,"@libcurl %1s failed-  (%2d) %3s",curlEMne,res,curlErrs) ;
	ok = FALSE ; goto curl_cleanup ;

/*	When all done */
curl_cleanup:
//	curl_easy_cleanup(curl);
	if (hsso->logFile != NULL) { UCfreopen(UClit("CON"),"w",stdout ) ; } ;
	if (curlHList != NULL) curl_slist_free_all(curlHList) ; 
	if (firstArg != NULL) curl_formfree(firstArg) ;
	return(ok) ;
}
#endif

LOGICAL v4im_EncHTTPArgs(la,resPtr,resBytes,conType,doMultiPart,errbuf)
  struct lcl__args *la ;
  BYTE **resPtr ;
  LENMAX *resBytes ;
  UCCHAR *conType ;
  LOGICAL doMultiPart ;
  UCCHAR *errbuf ;
{
  INDEX i ; LENMAX size = 0, argBytes ; char *argptr ; LOGICAL multiPart ;

	*resBytes = 0 ;				/* Haven't done anything yet */
/*	Do we need to do multi-part or can we get away with simple URL-Encoded ? */
	multiPart = FALSE ; for(i=0;i<la->argCnt;i++) { if (la->arg[i].isFile) { multiPart = TRUE ; break ; } ; } ;
	if (multiPart)				/* Need multi-part encoding or just '&' delmited ? */
	 { char boundary[64], *b ;
	   if (!doMultiPart) return(FALSE) ;
	   sprintf(boundary,"---------------------------%08x%08x",(int)clock()-(int)time(NULL),(int)time(NULL)+(int)clock()) ; boundary[45] = '\0' ;
	   UCsprintf(conType,256,UClit("multipart/form-data; boundary=%s"),ASCretUC(boundary)) ;
	   for(i=0;i<la->argCnt;i++)		/* Determine rough size of form */
	     { size += (strlen(boundary) + 2 + strlen(la->arg[i].name) + la->arg[i].bytes) + 256 ;
	       if (la->arg[i].isFile) size += la->arg[i].fileBytes ;
	     } ;
	   argptr = (char *)v4mm_AllocChunk(size,FALSE) ; ZS(argptr) ;/* Allocate argument buffer */
	   argBytes = 0 ;
	   for(b=argptr,i=0;i<la->argCnt;i++)
	    { if (la->arg[i].isFile)
	       { UCCHAR contentType[128] ; char *e = strrchr(la->arg[i].data,'.') ;
		 struct UC__File UCFile ;

		 v_FileExtToContentType(ASCretUC(e),contentType,UCsizeof(contentType),errbuf) ;
		 sprintf(b,"--%s" TXTEOL "Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"" TXTEOL "Content-Type: %s" TXTEOL TXTEOL,boundary,la->arg[i].name,la->arg[i].data,UCretASC(contentType)) ;
		 argBytes += strlen(b) ; b += strlen(b) ;

/*		 Have already verified that file exists */
/*		 Have to read in file data- this must be a text file, if it is UNICODE then must already be in UTF8 format */
		 v_UCFileOpen(&UCFile,ASCretUC(la->arg[i].data),UCFile_Open_ReadBin,TRUE,errbuf,V4IM_OpCode_HTTP) ;
		 read(UCFile.fd,b,la->arg[i].fileBytes) ; v_UCFileClose(&UCFile) ;
		 b[la->arg[i].fileBytes] = '\0' ;			/* Just for good measure... */

		 argBytes += la->arg[i].fileBytes ;
		 b += la->arg[i].fileBytes ; *b = '\0' ;
	       } else
	       { sprintf(b,"--%s" TXTEOL "Content-Disposition: form-data; name=\"%s\"" TXTEOL TXTEOL "%s" TXTEOL ,boundary,la->arg[i].name,la->arg[i].data) ;
		 argBytes += strlen(b) ; b += strlen(b) ;
	       } ;
//		 strcat(b,TXTEOL) ; argBytes += strlen(TXTEOL) ; b += strlen(TXTEOL) ; *b = '\0' ;
	    } ;
	   sprintf(b,"--%1s--" TXTEOL,boundary) ; argBytes += strlen(b) ; b += strlen(b) ; *b = '\0' ;
	   *resPtr = argptr ; *resBytes = argBytes ;
	   return(TRUE) ;
	 } ;
/*	Not multi-part - do simple URL encoded */
	for(i=0;i<la->argCnt;i++)			/* First need to approximate size of argument buffer */
	 { size += strlen(la->arg[i].name) + 1 + 2 * la->arg[i].bytes ; } ;
	argptr = (char *)v4mm_AllocChunk(size,FALSE) ; ZS(argptr) ;
	for(i=0;i<la->argCnt;i++)			/* Spin through again to construct master arg list */
	 { char s,*d ; INDEX j ;
	   strcat(argptr,la->arg[i].name) ;
	   if (la->arg[i].bytes > 0) strcat(argptr,"=") ;
/*	   Convert value to HTML encoding */
	   d = &argptr[strlen(argptr)] ;
	   for (j=0;j<la->arg[i].bytes;j++,d++)
	    { s = la->arg[i].data[j] ;
	      if (s == UClit(' ')) { *d = UClit('+') ; continue ; } ;
	      if (s < 128 && !(s == UClit('"') || s == UClit('&') || s == UClit('<') || s == UClit('>'))) { *d = s ; continue ; } ;
	      *(d++) = UClit('&') ; *(d++) = UClit('#') ;
	      *(d++) = (s / 100) + UClit('0') ; *(d++) = ((s % 100) / 10) + UClit('0') ; *(d) = (s % 10) + UClit('0') ;
	    } ; *d = '\0' ;
	   if (i < la->argCnt - 1) strcat(argptr,"&") ;
	 } ;
	*resPtr = argptr ; *resBytes = strlen(argptr) ;
	return(TRUE) ;
}

#if defined HAVECURL || defined HAVEWININET
P *v4im_DoHTTP(ctx,respnt,argcnt,argpnts,intmodx,trace)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  P *respnt,*argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ struct V4DPI__Point argpt,*cpt,*ofpt ;
  struct vHTTP__Result vhr ;
  INDEX ax,lax ; LOGICAL ok,isText,isAppend,multiPart ;
  UCCHAR url[512],object[512],outFile[V_FileName_Max],logFile[V_FileName_Max],certFile[V_FileName_Max],cookies[4096], userName[128], password[128], certPassword[128], sslCypherList[128] ;
  DIMID dimIdHdr ; UCCHAR *httpHeaders = NULL ; INDEX filex ;
  char args[4096],*argptr,*argFile ; LENMAX argBytes ;
  struct http__SendToServOptions hsso ;
  struct lcl__args *la ;
  UCCHAR* ub = NULL ; LENMAX ublen ;
  COUNTER retries=UNUSED ;				/* If HTTP fails then retry if > 0 */

retry:
	if (gpi->RestrictionMap & V_Restrict_QuotaHTML)
	 { if (v4html_BytesSend > gpi->QuotaHTMLPut || v4html_BytesRecv > gpi->QuotaHTMLGet)
	   { v_Msg(ctx,NULL,"RestrictQuota",intmodx,gpi->QuotaHTMLGet,v4html_BytesRecv,gpi->QuotaHTMLPut,v4html_BytesSend) ; goto fail ; } ;
	 } ;
	memset(&hsso,0,sizeof hsso) ;
	hsso.httpVerb = VHTTP_Verb_Get ; UCstrcpy(hsso.contentType,UClit("text/plain")) ; ZS(args) ; ZUS(cookies) ; ZUS(object) ; ZUS(outFile) ; isText = TRUE ; ofpt = NULL ; la = NULL ; isAppend = FALSE ; multiPart = FALSE ;
	ZUS(userName) ; ZUS(password) ; hsso.verbose = FALSE ; ZUS(logFile) ; dimIdHdr = UNUSED ;
	memset(&vhr,0,sizeof vhr) ; vhr.contentType = UNUSED ; vhr.contentLength = UNUSED ;
	lax = UNUSED ; argFile = NULL ; argptr = NULL ; filex = UNUSED ;

	for(ax=1,ok=TRUE;ok && ax<=argcnt;ax++)			/* Loop thru all arguments */
	 { if (argpnts[ax]->PntType != V4DPI_PntType_TagVal)
	    { if (lax == UNUSED) { v_Msg(ctx,NULL,"HTTPNoName",intmodx,ax,V4IM_Tag_Name) ; goto fail ; } ;
/*	      Grab as UCCHAR but convert to UTF8 and terminate with EOS */
	      v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,argpnts[ax],ctx) ;
	      la->arg[lax].bytes = UCUTF16toUTF8(ASCTBUF1,V4TMBufMax,UCTBUF1,UCstrlen(UCTBUF1)) ;
	      la->arg[lax].data = (BYTE *)v4mm_AllocChunk(la->arg[lax].bytes+1,FALSE) ; memcpy(la->arg[lax].data,ASCTBUF1,la->arg[lax].bytes) ; la->arg[lax].data[la->arg[lax].bytes] = '\0' ;
	      lax = UNUSED ; continue ;
	    } ;
	   switch (v4im_CheckPtArgNew(ctx,argpnts[ax],&cpt,&argpt))
	    { default:	    		v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	      case V4IM_Tag_Append:	isAppend = TRUE ;
	      case V4IM_Tag_Create:	ofpt = cpt ; v4im_GetPointFileName(&ok,outFile,UCsizeof(outFile),cpt,ctx,NULL) ; break ;
	      case V4IM_Tag_Arg:	v4im_GetPointChar(&ok,args,sizeof(args),cpt,ctx) ; break ;
	      case V4IM_Tag_ArgFile:	if (cpt->PntType == V4DPI_PntType_Dict)
					 { FILEID fileId ; COUNTER chars ; UCCHAR *obuf ;
					   fileId = vout_PntIdToFileId(ctx,(struct V4DPI__LittlePoint *)cpt) ;
					   if (fileId == UNUSED)  { v_Msg(ctx,NULL,"OutputNoId",intmodx,V4IM_Tag_ArgFile,cpt) ; goto fail ; } ;
/*					   First have to convert UCCHAR output buffer to UTF8 so that it is char */
					   chars = vout_CountForFile(fileId,UNUSED,FALSE,ctx->ErrorMsgAux) ;
					   argFile = v4mm_AllocChunk(chars * 2,FALSE) ;
					   obuf = vout_GetOutputBuffer(fileId,UNUSED,ctx->ErrorMsgAux) ;
					   if (obuf == NULL) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,argpnts[ax]) ; goto fail ; } ;
					   argBytes = UCUTF16toUTF8(argFile,chars*2,obuf,chars) ;
					 } else
					 { struct UC__File UCFile ; UCCHAR fileName[V_FileName_Max] ;
					   struct stat statbuf ;
					   v4im_GetPointFileName(&ok,fileName,V_FileName_Max,cpt,ctx,NULL) ; if (!ok) break ;
					   if (!v_UCFileOpen(&UCFile,fileName,UCFile_Open_ReadBin,TRUE,ctx->ErrorMsgAux,intmodx)) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,cpt) ; goto fail ; } ;
					   fstat(UCFile.fd,&statbuf) ; argFile = v4mm_AllocChunk(statbuf.st_size,FALSE) ;
					   read(UCFile.fd,argFile,statbuf.st_size) ; v_UCFileClose(&UCFile) ; argBytes = statbuf.st_size ;
					 } break ;
	      case V4IM_Tag_Bearer:	{ UCCHAR bearer[2048] ;
					  v4im_GetPointUC(&ok,bearer,UCsizeof(bearer),cpt,ctx) ;
					  if (hsso.hdrCount >= HTTP_OPTS_MAX_HDRS)
					   { v_Msg(ctx,NULL,"HTTPMaxHdr",HTTP_OPTS_MAX_HDRS,V4IM_Tag_Bearer) ; goto fail ; } ;
					  UCstrcpy(hsso.hdrs[hsso.hdrCount].hdrName,UClit("Authorization")) ;
					  UCstrcpy(hsso.hdrs[hsso.hdrCount].hdrValue,UClit("Bearer ")) ; UCstrcat(hsso.hdrs[hsso.hdrCount].hdrValue,bearer) ;
					  hsso.hdrCount++ ;
					}
					break ;
	      case V4IM_Tag_Certificate:if(cpt->PntType == V4DPI_PntType_List)
					 { struct V4L__ListPoint *lp = v4im_VerifyList(cpt,ctx,cpt,0) ; INDEX i ; P lpnt ;
					   for(i=1;v4l_ListPoint_Value(ctx,lp,i,&lpnt) > 0;i++)
					    { switch (i)
					       { default:	break ;
					         case 1:	v4im_GetPointFileName(&ok,certFile,UCsizeof(certFile),&lpnt,ctx,NULL) ; hsso.sslCert = certFile ; break ;
						 case 2:	v4im_GetPointUC(&ok,certPassword,UCsizeof(certPassword),&lpnt,ctx) ; hsso.sslCertPassword = certPassword ; break ;
						 case 3:	v4im_GetPointUC(&ok,sslCypherList,UCsizeof(sslCypherList),&lpnt,ctx) ; hsso.sslCypherList = sslCypherList ; break ;
					       } ;
					    } ;
					 } else { v4im_GetPointFileName(&ok,certFile,UCsizeof(certFile),cpt,ctx,NULL) ; hsso.sslCert = certFile ; } ;
					break ;
	      case V4IM_Tag_Cookie:	v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,cpt,ctx) ; if (!ok) break ;
					if (UCstrlen(cookies) + UCstrlen(UCTBUF1) + 5 >= UCsizeof(cookies)) { v_Msg(ctx,NULL,"HTTPMaxCookie",intmodx,ax,cpt,UCsizeof(cookies)) ; goto fail ; } ;
					UCstrcat(cookies,UCTBUF1) ; UCstrcat(cookies,UClit(" ; ")) ;
					hsso.cookies = cookies ; break ;
	      case -V4IM_Tag_Data:	isText = FALSE ; break ;
	      case V4IM_Tag_DateTime:	if (cpt->PntType != V4DPI_PntType_UDT)
					 { v_Msg(ctx,NULL,"",intmodx,cpt) ; goto fail ; } ;
					hsso.UDT = cpt->Value.IntVal + (60 * gpi->MinutesWest) ; break ;
	      case -V4IM_Tag_Delete:	hsso.httpVerb = VHTTP_Verb_Delete ; break ;
	      case V4IM_Tag_File:	{ UCCHAR fileName[V_FileName_Max] ; LENMAX len ;
#if defined WINNT && defined V4UNICODE
					  struct _stat statbuf ;
#else
					  struct stat statbuf ;
#endif
//					  struct UC__File UCFile ;
					  if (lax == UNUSED) { v_Msg(ctx,NULL,"HTTPNoName",intmodx,ax,V4IM_Tag_Name) ; goto fail ; } ;
					  v4im_GetPointFileName(&ok,fileName,V_FileName_Max,cpt,ctx,NULL) ; if (!ok) break ;
					  la->arg[lax].data = v4mm_AllocChunk(V_FileName_Max*2+1,FALSE) ;
					  len = UCUTF16toUTF8(la->arg[lax].data,V_FileName_Max*2,fileName,UCstrlen(fileName)) ; la->arg[lax].data[len] = '\0' ;
					  if (UCstat(fileName,&statbuf) == -1)
					   { v_Msg(ctx,ctx->ErrorMsgAux,"FileError1",fileName,errno) ; v_Msg(ctx,NULL,"ModInvArg2",intmodx,cpt) ; goto fail ; } ;
					  la->arg[lax].fileBytes = statbuf.st_size ; la->arg[lax].bytes = len ;
///*					  This must be a text file, if it is UNICODE then must already be in UTF8 format */
//					  if (!v_UCFileOpen(&UCFile,fileName,UCFile_Open_ReadBin,TRUE,ctx->ErrorMsgAux,intmodx)) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,cpt) ; goto fail ; } ;
//					  fstat(UCFile.fd,&statbuf) ; la->arg[lax].fileData = v4mm_AllocChunk(statbuf.st_size+1,FALSE) ; la->arg[lax].fileBytes = statbuf.st_size ;
//					  read(UCFile.fd,la->arg[lax].fileData,statbuf.st_size) ; v_UCFileClose(&UCFile) ;
//					  la->arg[lax].fileData[la->arg[lax].fileBytes] = '\0' ;
					  la->arg[lax].isFile = TRUE ; multiPart = TRUE ; lax = UNUSED ;
					} break ;
	      case -V4IM_Tag_Get:	hsso.httpVerb = VHTTP_Verb_Get ; break ;
	      case V4IM_Tag_Header:	if (cpt->PntType == V4DPI_PntType_List)		/* Do we have (header-name header-value) or dimension? */
					 { struct V4DPI__Point hdrpt ;
					   struct V4L__ListPoint *lp = (struct V4L__ListPoint *)&cpt->Value ; COUNTER num ;
					   if (hsso.hdrCount >= HTTP_OPTS_MAX_HDRS)
					    { v_Msg(ctx,NULL,"HTTPMaxHdr",HTTP_OPTS_MAX_HDRS,V4IM_Tag_Header) ; goto fail ; } ;
					   num = SIZEofLIST(lp) ;
					   if (num != 2) { v_Msg(ctx,NULL,"HTTPHdrVal",V4IM_Tag_Header,cpt) ; goto fail ; } ;
					   v4l_ListPoint_Value(ctx,lp,1,&hdrpt) ;
					   if (memcmp(&hdrpt,&protoNone,protoNone.Bytes) == 0) break ;
					   v4im_GetPointUC(&ok,hsso.hdrs[hsso.hdrCount].hdrName,UCsizeof(hsso.hdrs[hsso.hdrCount].hdrName),&hdrpt,ctx) ;
					   v4l_ListPoint_Value(ctx,lp,2,&hdrpt) ; if (memcmp(&hdrpt,&protoNone,protoNone.Bytes) == 0) break ; v4im_GetPointUC(&ok,hsso.hdrs[hsso.hdrCount].hdrValue,UCsizeof(hsso.hdrs[hsso.hdrCount].hdrValue),&hdrpt,ctx) ;
					   hsso.hdrCount++ ;
					 } else						/* Have dimension */
					 { struct V4DPI__DimInfo *di ;
					   vhr.hdrBufPtr = &httpHeaders ;		/* Use this as trigger to save headers */
					   dimIdHdr = v4dpi_PointToDimId(ctx,cpt,&di,intmodx,ax) ;
					   if (dimIdHdr == UNUSED) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax) ; goto fail ; } ;
					   if (di->PointType != V4DPI_PntType_List)
					    { v_Msg(ctx,NULL,"ModArgPntType2",intmodx,ax,di->PointType,V4DPI_PntType_List) ; goto fail ; } ;
					 } ;
					break ;
	      case V4IM_Tag_Include:	{ UCCHAR fileName[V_FileName_Max] ; struct stat statbuf ; struct UC__File UCFile ;
					  if (lax == UNUSED) { v_Msg(ctx,NULL,"HTTPNoName",intmodx,ax,V4IM_Tag_Name) ; goto fail ; } ;
					  v4im_GetPointFileName(&ok,fileName,V_FileName_Max,cpt,ctx,NULL) ; if (!ok) break ;
					  if (!v_UCFileOpen(&UCFile,fileName,UCFile_Open_ReadBin,TRUE,ctx->ErrorMsgAux,intmodx)) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,cpt) ; goto fail ; } ;
					  fstat(UCFile.fd,&statbuf) ; la->arg[lax].data = v4mm_AllocChunk(statbuf.st_size+1,FALSE) ;
					  read(UCFile.fd,la->arg[lax].data,statbuf.st_size) ; v_UCFileClose(&UCFile) ; la->arg[lax].data[statbuf.st_size] = '\0' ;
					  lax = UNUSED ;
					} break ;
	      case V4IM_Tag_Object:	v4im_GetPointUC(&ok,object,UCsizeof(object),cpt,ctx) ; hsso.objectName = object ; break ;
	      case V4IM_Tag_Out:	filex = vout_PntIdToFileX(ctx,(struct V4DPI__LittlePoint *)cpt) ;
					if (filex == UNUSED) { v_Msg(ctx,NULL,"StreamNoOutput",intmodx,V4IM_Tag_Out,cpt) ; goto fail ; } ;
					break ;
	      case V4IM_Tag_Name:	if (la == NULL) { la = (struct lcl__args *)v4mm_AllocChunk(sizeof *la,FALSE) ; la->argCnt = 0 ; } ;
					if (la->argCnt >= VHTTP_Arg_Max) { v_Msg(ctx,NULL,"HTTPMaxArgs",intmodx,ax,VHTTP_Arg_Max) ; goto fail ; } ;
					v4im_GetPointChar(&ok,la->arg[la->argCnt].name,UCsizeof(la->arg[la->argCnt].name),cpt,ctx) ;
					la->arg[la->argCnt].bytes = 0 ; la->arg[la->argCnt].isFile = FALSE ;
					lax = la->argCnt ; la->argCnt++ ;
					break ;
	      case V4IM_Tag_Password:	v4im_GetPointUC(&ok,password,UCsizeof(password),cpt,ctx) ; break ;
	      case -V4IM_Tag_Post:	hsso.httpVerb = VHTTP_Verb_Post ; break ;
	      case -V4IM_Tag_Put:	hsso.httpVerb = VHTTP_Verb_Put ; break ;
	      case V4IM_Tag_Retry:	if (retries != UNUSED) break ;
					retries = v4im_GetPointInt(&ok,cpt,ctx) ; if (retries < 0 || retries > 20) retries = 0 ;
					break ;
	      case -V4IM_Tag_Text:	isText = TRUE ; break ;
	      case V4IM_Tag_Trace:	if (cpt->PntType == V4DPI_PntType_Logical)
					 { hsso.verbose = v4im_GetPointLog(&ok,cpt,ctx) ; }
					 else { hsso.verbose = TRUE ; v4im_GetPointUC(&ok,logFile,UCsizeof(logFile),cpt,ctx) ; } ;
					break ;
	      case V4IM_Tag_Type:	v4im_GetPointUC(&ok,hsso.contentType,UCsizeof(hsso.contentType),cpt,ctx) ; break ;
	      case V4IM_Tag_URL:	v4im_GetPointUC(&ok,url,UCsizeof(url),cpt,ctx) ; hsso.url = url ; break ;
	      case V4IM_Tag_User:	v4im_GetPointUC(&ok,userName,UCsizeof(userName),cpt,ctx) ; break ;
	      case V4IM_Tag_UserAgent:	v4im_GetPointUC(&ok,hsso.userAgent,UCsizeof(hsso.userAgent),cpt,ctx) ; break ;
	      case V4IM_Tag_Wait:	hsso.maxWait = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax-1) ; goto fail ; } ;
/*	Do we have single argument string (in args or argFile) or an array of named arguments (in la) */
	if (la != NULL)
	 { 
#ifdef HAVEWINHTTP
	   LOGICAL doMultiPart ;	/* Set to TRUE for windows because WinHTTPxxx does not do argument encoding, libCURL does it for us */
	   doMultiPart = TRUE ;
	   v4im_EncHTTPArgs(la,&argptr,&argBytes,hsso.contentType,doMultiPart,ctx->ErrorMsgAux) ;
	   hsso.data = argptr ; hsso.dataBytes = argBytes ;
#else
//	   doMultiPart = FALSE ;
	   hsso.la = la ;
#endif
	 } else { hsso.data = (argFile != NULL ? argFile : args) ; hsso.dataBytes = (argFile != NULL ? argBytes : strlen(args)) ; } ;
/*	Do we have userName/password ? */
	if (UCstrlen(userName) + UCstrlen(password) > 0)
	 { UCsprintf(hsso.authorization,UCsizeof(hsso.authorization),UClit("%s:%s"),userName,password) ; 
	 } ;
	if (UCnotempty(outFile))
	 { vhr.dstFile = v4mm_AllocChunk(sizeof *vhr.dstFile,FALSE) ;
	   if (!v_UCFileOpen(vhr.dstFile,outFile,(isAppend ? UCFile_Open_Append : (isText ? UCFile_Open_Write : UCFile_Open_WriteBin)),TRUE,ctx->ErrorMsgAux,intmodx))
	    { v_Msg(ctx,NULL,"ModInvArg2",intmodx,ofpt) ; goto fail ; } ;
	 } ;

//------------------------------------------------------------------------------------
	hsso.vhr = &vhr ; hsso.errbuf = ctx->ErrorMsgAux ;
	if (!vHTTP_SendToServer(&hsso))
	 { if (retries > 0)
	    { 
v_Msg(ctx,NULL,"@V4T - %1E failed (%0A) -- retries=%2d\n",intmodx,retries) ;  vout_UCText(VOUT_Trace,0,ctx->ErrorMsg) ;
	      retries-- ; HANGLOOSE(1 * 1000) ;
	      goto retry ;
	    } ;
	   v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ;
	 } ;
//printf("return from sendtoserver vhr.bytes = %d, outFile len=%d\n",vhr.bytes,UCstrlen(outFile)) ;

/*	Update a dimension in the context with the headers ? */
	if (dimIdHdr != UNUSED)
	 { P lpnt,cpnt ; struct V4L__ListPoint *lp ; UCCHAR *s,*e ;
	   INITLP(&lpnt,lp,dimIdHdr) ;
	   ZPH(&cpnt) ; cpnt.PntType = V4DPI_PntType_UCChar ; cpnt.Dim = Dim_Alpha ;
	   for(e=(s=httpHeaders);(e == NULL ? FALSE : *s != UCEOS);s=e+1)
	    { e = UCstrpbrk(s,UClit("\r\n")) ;
	      if (e != NULL) *e = UCEOS ;  if (UCempty(s)) continue ;
	      uccharPNTv(&cpnt,s) ;
	      if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&cpnt,0))
	       { v_Msg(NULL,gpi->ctx->ErrorMsgAux,"LPModMaxSize") ; goto fail ; } ;
	    } ;
	   v4mm_FreeChunk(httpHeaders) ;
	   ENDLP(&lpnt,lp) ; if (!v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,&lpnt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto fail ; } ;
	 } ;
/*	What are we returning? */
	if (UCnotempty(outFile))
	 { v_UCFileClose(vhr.dstFile) ;
	   memcpy(respnt,ofpt,ofpt->Bytes) ; return(respnt) ;
	 } ;
/*	Return result as buffered output or BIGTEXT string */

	if (isText)
	 { ub = v4mm_AllocUC(vhr.bytes*1.5) ;
	   ublen = UCUTF8toUTF16(ub,V4TMBufMax,vhr.dstBuffer,vhr.bytes) ;
	 } else
	 { INDEX i ;
	   ub = v4mm_AllocUC(vhr.bytes+10) ; ublen = vhr.bytes ;
	   for(i=0;i<vhr.bytes;i++) { ub[i] = vhr.dstBuffer[i]; } ; ub[i++] = UCEOS ;
	 } ;

	if (filex != UNUSED && vhr.bytes > 0)
	 {  
	   vout_UCTextFileX(filex,vhr.bytes,ub) ;
	   intPNTv(respnt,vhr.bytes) ;
	 } else if (vhr.bytes > 0)
	 { 
	  if (vhr.bytes >= V4LEX_BigText_Max)
	   { if (vhr.dstBuffer != NULL) v4mm_FreeChunk(vhr.dstBuffer) ;
	     v_Msg(ctx,NULL,"@%1E - HTTP reply (vhr.bytes=%2d) exceeds max length allowed(%3d)",intmodx,vhr.bytes,V4LEX_BigText_Max) ;
	     goto fail ;
	   } ;
//printf("ublen=%d, vhr.bytes=%d XYXYXYXY\n",ublen,vhr.bytes) ;
	  if (ublen < 0)
	   { if (vhr.bytes < V4DPI_AlphaVal_Max - 16)
	      { alphaPNTvl(respnt,vhr.dstBuffer,vhr.bytes) ; v4NutCracker=107 ;	/* Must be binary or some invalid UTF-8 character combination - take what we can */
	      } else { ublen = V4LEX_BigText_Max + 1 ; } ;	/* Make big number to force error below */
	   } ;
	  if (ublen >= (V4DPI_UCVAL_MaxSafe))			/* Can we return as string or do we have to convert to BigText */
	   { struct V4LEX__BigText *bt ;
	     if (ublen >= V4LEX_BigText_Max)
	      { v_Msg(ctx,NULL,"@%1E - Result string (vhr.ublen=%2d) exceeds max length allowed(%3d)",intmodx,vhr.bytes,V4LEX_BigText_Max) ; goto fail ; } ;
	     bt = (struct V4LEX__BigText *)v4mm_AllocChunk(sizeof *bt,FALSE) ;
	     UCstrcpy(bt->BigBuf,ub) ;
	     if (!v4dpi_SaveBigTextPoint(ctx,bt,UCstrlen(bt->BigBuf),respnt,Dim_Alpha,TRUE))
	      { v_Msg(ctx,NULL,"StrSaveBigText",intmodx,V4DPI_PntType_BigText) ; goto fail ; } ;
	     v4mm_FreeChunk(bt) ;		
	   } else if (ublen >= 0)				/* Store as Alpha point */	
	   { uccharPNTvl(respnt,ub,ublen) ;
	   } ;
	} else
	{ logPNTv(respnt,FALSE) ;				/* No result? - return FALSE */
	} ;
	if (la != NULL)		/* If we used explicit arguments then clean up after ourselves */
	 { INDEX i ;
	   for (i=0;i<la->argCnt;i++) { v4mm_FreeChunk(la->arg[i].data) ; } ;
	   v4mm_FreeChunk(la) ;
	 } ;
	if (argptr != NULL) v4mm_FreeChunk(argptr) ;
	if (vhr.dstBuffer != NULL) v4mm_FreeChunk(vhr.dstBuffer) ;
	if (argFile != NULL) v4mm_FreeChunk(argFile) ;
	if (ub != NULL) v4mm_FreeChunk(ub) ;
	return(respnt) ;
fail:
	if (ub != NULL) v4mm_FreeChunk(ub) ;
	REGISTER_ERROR(0) ; RETURNFAILURE ;
}
#endif

#if defined HAVECURL || defined HAVEWININET

/*	F T P   M O D U L E			*/

struct lcl__ftpCallback {			/* Common call back structure */
  LOGICAL isOpen ;
  LOGICAL isText ;
  COUNTER fileCount ;
  LENMAX fileBytes ;
  struct UC__File UCFile ;
  UCCHAR fileName[V_FileName_Max] ;
  UCCHAR dirLocal[V_FileName_Max] ;
  struct V4L__ListPoint *lp ;
};
#ifdef HAVECURL

/*	G E T   C A L L B A C K S			*/
static size_t vFTP_SingleFileWrite(chunk, size, nmemb, lfcb)
  void *chunk ;
  size_t size, nmemb ;
  struct lcl__ftpCallback *lfcb ;
{
  size_t bytes ;

/*	We are only opening local output file if/when we know we have something */
	if (lfcb != NULL ? !lfcb->isOpen : FALSE)
	 { if (!v_UCFileOpen(&lfcb->UCFile,lfcb->fileName,(lfcb->isText ? UCFile_Open_Write : UCFile_Open_WriteBin),TRUE,gpi->ctx->ErrorMsgAux,V4IM_OpCode_FTP)) return(-1) ;
	   lfcb->isOpen = TRUE ;
	 } ;
	bytes = fwrite(chunk,size,nmemb,lfcb->UCFile.fp) ;
	lfcb->fileBytes += bytes ;
	return(bytes) ;
}

/*	M G E T   C A L L B A C K S		*/

static long vFTP_mget_BeginNextFile(cfi,lfcb,remains)
  struct curl_fileinfo *cfi ;
  struct lcl__ftpCallback *lfcb ;
  int remains ;
{ UCCHAR fileName[V_FileName_Max*2] ;

/*	Currently only handle files (e.g. ignore directories) */
	UCstrcpy(fileName,lfcb->dirLocal) ; UCstrcatToUC(fileName,cfi->filename) ;
	if (cfi->filetype != CURLFILETYPE_FILE) return(CURL_CHUNK_BGN_FUNC_SKIP) ;
	if (!v_UCFileOpen(&lfcb->UCFile,fileName,(lfcb->isText ? UCFile_Open_Write : UCFile_Open_WriteBin),TRUE,gpi->ctx->ErrorMsgAux,V4IM_OpCode_FTP))
	 return(CURL_CHUNK_BGN_FUNC_FAIL) ;
	lfcb->isOpen = TRUE ;
	if (lfcb->lp != NULL)
	 { P fpnt ; uccharPNTv(&fpnt,fileName) ;
	   if (!v4l_ListPoint_Modify(gpi->ctx,lfcb->lp,V4L_ListAction_Append,&fpnt,0))
	    { v_Msg(NULL,gpi->ctx->ErrorMsgAux,"LPModMaxSize") ; return(CURL_CHUNK_BGN_FUNC_FAIL) ; } ;
	 } ;
	lfcb->fileCount ++ ;
	return(CURL_CHUNK_BGN_FUNC_OK) ;
}
static long vFTP_mget_EndFile(lfcb)
  struct lcl__ftpCallback *lfcb ;
{
	if (lfcb->isOpen)
	 { v_UCFileClose(&lfcb->UCFile) ; lfcb->isOpen = FALSE ; } ;
	return(CURL_CHUNK_END_FUNC_OK) ;
}
 
static size_t vFTP_mget_WriteChunk(chunk,size,nmemb,lfcb)
  char *chunk ;
  size_t size ;
  size_t nmemb ;
  struct lcl__ftpCallback *lfcb ;
{ size_t bytes ;

	if (!lfcb->isOpen) return(-1) ;
	bytes = fwrite(chunk,size,nmemb,lfcb->UCFile.fp) ;
	lfcb->fileBytes += bytes ;
	return(bytes) ;
}

/*	P U T   C A L L B A C K			*/

static size_t vFTP_put_GetChunk(chunk, size, nmemb, lfcb)
  void *chunk ;
  size_t size, nmemb ;
  struct lcl__ftpCallback *lfcb ;
{ size_t bytes ;
 
	bytes = fread(chunk, size, nmemb, lfcb->UCFile.fp) ;
	lfcb->fileBytes += bytes ;
	return(bytes) ;
}
#endif /* HAVECURL */

P *v4im_DoFTP(ctx,respnt,argcnt,argpnts,intmodx,trace)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  P *respnt,*argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ struct V4DPI__Point argpt,*cpt, pflPnt ;
  struct V4L__ListPoint *pflp ;
  INDEX ax ; LOGICAL ok, isText, verbose ;
  UCCHAR userName[128], password[128], url[1024], host[V_FileName_Max], dirLocal[V_FileName_Max], dirRemote[V_FileName_Max], getFile[V_FileName_Max], putFile[V_FileName_Max], asFile[V_FileName_Max], logFile[V_FileName_Max] ;
  UCCHAR *fnBegin ;
  enum lcl__ftpMode { get, mget, put, mfput } ftpMode ;
  enum lcl__resMode { bytes, files, list } resMode ;

	isText = TRUE ; verbose = FALSE ; ZUS(userName) ; ZUS(password) ; ZUS(url) ; ZUS(dirLocal) ; ZUS(dirRemote) ; ZUS(getFile) ; ZUS(putFile) ; ZUS(logFile) ; ZUS(host) ;
	pflp = NULL ; resMode = files ;

	for(ax=1,ok=TRUE;ok && ax<=argcnt;ax++)			/* Loop thru all arguments */
	 { 
	   switch (v4im_CheckPtArgNew(ctx,argpnts[ax],&cpt,&argpt))
	    { default:	    		v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	      case V4IM_Tag_As:		v4im_GetPointUC(&ok,asFile,UCsizeof(asFile),cpt,ctx) ; break ;
	      case -V4IM_Tag_Data:	isText = FALSE ; break ;
	      case V4IM_Tag_Get:	v4im_GetPointUC(&ok,getFile,UCsizeof(getFile),cpt,ctx) ; break ;	/* Don't use GetPointFileName - filename syntax/semantics on remote may not be same as local machine */
	      case V4IM_Tag_Host:	v4im_GetPointUC(&ok,host,UCsizeof(host),cpt,ctx) ; break ;
	      case V4IM_Tag_Local:	v4im_GetPointFileName(&ok,UCTBUF1,UCsizeof(dirLocal),cpt,ctx,NULL) ;
					{UCCHAR *s,*d ;
#if defined WINNT && defined V4UNICODE
					 struct _stat di ;
#else
					 struct stat di ;
#endif
/*					 Normalize the directory name */
					 if (UCTBUF1[UCstrlen(UCTBUF1)-1] != UClit('/') && UCTBUF1[UCstrlen(UCTBUF1)-1] != UClit('\\')) { UCstrcat(UCTBUF1,UClit("/")) ; } ;
#ifdef WINNT
					 for(s=UCTBUF1,d=dirLocal;;s++) { *(d++) = (*s == UClit('/') ? UClit('\\') : *s) ; if (*s == UCEOS) break ; } ;
#endif
					 if (UCstat(dirLocal,&di) != 0) di.st_mode = 0 ;
					 if ((di.st_mode & S_IFDIR) != 0) { v_Msg(ctx,NULL,"FTPNotDir",intmodx,V4IM_Tag_Local,dirLocal) ; goto fail ; } ;
					} break ;
	      case V4IM_Tag_Password:	v4im_GetPointUC(&ok,password,UCsizeof(password),cpt,ctx) ; break ;
	      case V4IM_Tag_Put:	if (cpt->PntType == V4DPI_PntType_List)
					 { memcpy(&pflPnt,cpt,cpt->Bytes) ; pflp = v4im_VerifyList(NULL,ctx,cpt,intmodx) ;
					 } else { v4im_GetPointUC(&ok,putFile,UCsizeof(putFile),cpt,ctx) ; } ;
					break ;
	      case -V4IM_Tag_Text:	isText = TRUE ; break ;
	      case V4IM_Tag_Remote:	v4im_GetPointUC(&ok,dirRemote,UCsizeof(dirRemote),cpt,ctx) ; break ;
	      case V4IM_Tag_Result:	switch (v4im_GetDictToEnumVal(ctx,cpt))
					 { default:		v_Msg(ctx,NULL,"ModTagValue",intmodx,ax,V4IM_Tag_Result,cpt) ; goto fail ;
					   case _Bytes:		resMode = bytes ; break ;
					   case _Files:		resMode = files ; break ;
					   case _List:		resMode = list ; break ;
					 } ;
					break ;
	      case V4IM_Tag_Trace:	if (cpt->PntType == V4DPI_PntType_Logical)
					 { verbose = v4im_GetPointLog(&ok,cpt,ctx) ; }
					 else { verbose = TRUE ; v4im_GetPointUC(&ok,logFile,UCsizeof(logFile),cpt,ctx) ; } ;
					break ;
	      case V4IM_Tag_User:	v4im_GetPointUC(&ok,userName,UCsizeof(userName),cpt,ctx) ; break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax-1) ; goto fail ; } ;
	if (UCempty(host)) { v_Msg(ctx,NULL,"TagMissing",intmodx,V4IM_Tag_Host) ; goto fail ; } ;
	if (UCnotempty(getFile) && (pflp != NULL || UCnotempty(putFile))) { v_Msg(ctx,NULL,"ModOnlyOne",intmodx,V4IM_Tag_Get,V4IM_Tag_Put) ; goto fail ; } ;

/*	Ready to rock-n-roll, construct ftp url */
	UCstrcpy(url,UClit("ftp://")) ;
#ifdef FTPEMBEDDUSERPW
	if (UCnotempty(userName) || UCnotempty(password))
	 { UCstrcat(url,userName) ; UCstrcat(url,UClit(":")) ; UCstrcat(url,password) ; UCstrcat(url,UClit("@")) ; } ;
#endif
	UCstrcat(url,host) ; if (url[UCstrlen(url)-1] != UClit('/')) UCstrcat(url,UClit("/")) ;
/*	Append target directory, convert backslashes, make sure it ends with a slash */
	{ UCCHAR *d,*s ; d = &url[UCstrlen(url)] ; s = dirRemote ;
	  for (;*s!=UCEOS;s++,d++) { *d = (*s == UClit('\\') ? UClit('/') : *s) ; } ;
	  if (*d != UClit('/')) *(d++) = UClit('/') ; *d = UCEOS ;
	  fnBegin = d ;				/* Remember where file name begins in case we are doing multiple files */
	}
/*	What are we doing - a GET, MGET, PUT or multiple puts */
	if (UCnotempty(getFile))
	 { ftpMode = ((UCstrchr(getFile,'%') != NULL || UCstrchr(getFile,'*') != NULL) ? mget : get) ;
	 } else { ftpMode = (pflp == NULL ? put : mfput) ; } ;
	if ((ftpMode == mget || ftpMode == mfput) && UCnotempty(asFile)) { v_Msg(ctx,NULL,"FTPMultFile",intmodx,V4IM_Tag_As) ; goto fail ; } ;

#ifdef HAVECURL
	{ CURL *curl ;
	  char curlErrs[CURL_ERROR_SIZE], *curlEMne = "" ;
	  CURLcode res ; LOGICAL ok ;
	  struct lcl__ftpCallback lfcb ;

	  memset(&lfcb,0,sizeof(lfcb)) ;
	  if (resMode == list) { INITLP(respnt,lfcb.lp,Dim_List) } ;	/* Initi result - list of files */


	  if (curlNeedInit) { curl_global_init(CURL_GLOBAL_ALL) ; curlNeedInit = FALSE ; } ;
	  curl = curl_easy_init() ;
#ifndef FTPEMBEDDUSERPW
	  CURLOPTION(CURLOPT_USERNAME,UCretASC(userName)) ;
	  CURLOPTION(CURLOPT_PASSWORD,UCretASC(password)) ;
#endif
	  CURLOPTION(CURLOPT_ERRORBUFFER,curlErrs)				/* To receive readable error message */
	   ZUS(ctx->ErrorMsgAux) ;					/*  (we may get curl or V4 errors, if ErrorMsgAux is not blank then use that message) */
	  CURLOPTION(CURLOPT_NOPROGRESS,1)					/* Shut off progress meter */
	  if (verbose)							/* Switch on full protocol/debug output */
	   { CURLOPTION(CURLOPT_VERBOSE, 1L) ;
	     if (UCnotempty(logFile)) { UCfreopen(logFile,"w",stdout ) ; } ;
	   } ;

	  switch (ftpMode)
	   {
	     case get:
		{ UCstrcpy(lfcb.fileName,(UCnotempty(asFile) ? asFile : getFile)) ;
		  if (lfcb.lp != NULL)
		   { P fpnt ; uccharPNTv(&fpnt,(UCnotempty(asFile) ? asFile : getFile)) ;
		     if (!v4l_ListPoint_Modify(gpi->ctx,lfcb.lp,V4L_ListAction_Append,&fpnt,0))
		      { v_Msg(NULL,gpi->ctx->ErrorMsgAux,"LPModMaxSize") ; goto curl_fail ; } ;
		   } ;
		  UCstrcat(url,getFile) ;
		  CURLOPTION(CURLOPT_URL,UCretASC(url)) ;
		  CURLOPTION(CURLOPT_WRITEFUNCTION, vFTP_SingleFileWrite) ;	/* Define our callback to get called when there's data to be written */ 
		  CURLOPTION(CURLOPT_WRITEDATA, &lfcb) ;				/* Set a pointer to our struct to pass to the callback */ 
		  res = curl_easy_perform(curl) ;
		  if (res != CURLE_OK) { curlEMne = "curl_easy_perform" ; goto curl_fail ; } ;
		  if (lfcb.isOpen)
		   { lfcb.isOpen = FALSE ; v_UCFileClose(&lfcb.UCFile) ; } ;	/* Close the local file */
		  lfcb.fileCount = 1 ;
		} break ;
	     case mget:
		{ 
		  lfcb.isOpen = FALSE ; lfcb.isText = FALSE ; UCstrcpy(lfcb.dirLocal,dirLocal) ;
		  UCstrcat(url,getFile) ; CURLOPTION(CURLOPT_URL,UCretASC(url)) ;
		  CURLOPTION(CURLOPT_CHUNK_BGN_FUNCTION,vFTP_mget_BeginNextFile) ;	/* callback is called before download of concrete file started */ 
		  CURLOPTION(CURLOPT_CHUNK_END_FUNCTION,vFTP_mget_EndFile) ;	/* callback is called after data from the file have been transferred */ 
		  CURLOPTION(CURLOPT_WRITEFUNCTION, vFTP_mget_WriteChunk) ;	/* this callback will write contents into files */ 
		  CURLOPTION(CURLOPT_CHUNK_DATA,&lfcb) ; /* put transfer data into callbacks */ 
		  CURLOPTION(CURLOPT_WRITEDATA,&lfcb) ;
		  CURLOPTION(CURLOPT_WILDCARDMATCH, 1L) ;				/* turn on wildcard matching */ 
		  res = curl_easy_perform(curl) ;
		  if (res != CURLE_OK) { curlEMne = "curl_easy_perform" ; goto curl_fail ; } ;
		} break ;
	     case put:
	     case mfput:
		{ UCCHAR localFile[V_FileName_Max] ; struct stat pfInfo ; INDEX fx ; P fnPnt ;
		  CURLOPTION(CURLOPT_READFUNCTION, vFTP_put_GetChunk) ;		/* Want to use our own read function */ 
		  CURLOPTION(CURLOPT_READDATA,&lfcb) ;				/*  and pointer to callback data */
		  CURLOPTION(CURLOPT_UPLOAD, 1L) ;					/* Enable uploading */ 
/*		  Verify that the local file exists */
		  if (pflp != NULL)						/* If a list of files - verify all of them */
		   { for(fx=1;v4l_ListPoint_Value(ctx,pflp,fx,&fnPnt) > 0;fx++)
		      { v4im_GetPointUC(&ok,putFile,UCsizeof(putFile),&fnPnt,ctx) ;
		        UCstrcpy(localFile,dirLocal) ; UCstrcat(localFile,putFile) ;
			if (!v_UCFileOpen(&lfcb.UCFile,localFile,UCFile_Open_ReadBin,TRUE,ctx->ErrorMsgAux,intmodx)) goto curl_fail ;
			v_UCFileClose(&lfcb.UCFile) ;
		      } ; if (fx == 1) { v_Msg(ctx,ctx->ErrorMsgAux,"FTPEmptyList",V4IM_Tag_Put,&pflPnt) ; goto curl_fail ; } ;
		   } ;
		  for(fx=1;;fx++)
		   { UCCHAR *fn,*s ;
		     if (pflp != NULL)						/* Doing a list of files- get next one */
		      { if (v4l_ListPoint_Value(ctx,pflp,fx,&fnPnt) <= 0) break ;
		        v4im_GetPointUC(&ok,putFile,UCsizeof(putFile),&fnPnt,ctx) ;
		      } ;
		     UCstrcpy(localFile,dirLocal) ; UCstrcat(localFile,putFile) ;
		     if (!v_UCFileOpen(&lfcb.UCFile,localFile,UCFile_Open_ReadBin,TRUE,ctx->ErrorMsgAux,intmodx)) goto curl_fail ;
		     for(s=localFile,fn=localFile;*s!=UCEOS;s++)			/* Once local file is open then strip off device & directory so we pass just the filename do the remote */
		      { if (*s == UClit('/') || *s == UClit('\\') || *s == UClit(':')) fn = s + 1 ; } ;
		     *fnBegin = UCEOS ;							/* Strip off prior file name before append this one */
		     UCstrcat(url,(UCnotempty(asFile) ? asFile : fn)) ;
		     if (lfcb.lp != NULL)
		      { P fpnt ; uccharPNTv(&fpnt,(UCnotempty(asFile) ? asFile : fn)) ;
		        if (!v4l_ListPoint_Modify(gpi->ctx,lfcb.lp,V4L_ListAction_Append,&fpnt,0))
		         { v_Msg(NULL,gpi->ctx->ErrorMsgAux,"LPModMaxSize") ; goto curl_fail ; } ;
		      } ;
		     CURLOPTION(CURLOPT_URL,UCretASC(url)) ;				/* The FTP URL */
		     fstat(lfcb.UCFile.fd,&pfInfo) ;
		     CURLOPTION(CURLOPT_INFILESIZE_LARGE,(curl_off_t)pfInfo.st_size) ;	/* Size of the file we are going to PUT */
		     res = curl_easy_perform(curl);
		     v_UCFileClose(&lfcb.UCFile) ; lfcb.fileCount += 1 ;
		     if (res != CURLE_OK) { curlEMne = "curl_easy_perform" ; goto curl_fail ; } ;
		     if (pflp == NULL) break ;						/* Not doing a list - quit */
		   } ;
		} break ;
	   } ;

/*	What are we returning ? */
	switch (resMode)
	 { case bytes:		intPNTv(respnt,lfcb.fileBytes) ; break ;
	   case files:		intPNTv(respnt,lfcb.fileCount) ; break ;
	   case list:		ENDLP(respnt,lfcb.lp) ; break ;
	 } ;

	if (UCnotempty(logFile)) { UCfreopen(UClit("CON"),"w",stdout ) ; } ;
	ok = TRUE ; goto curl_cleanup ;

curl_fail:
	  ok = FALSE ; goto curl_cleanup ;

/*	When all done */
curl_cleanup:
	  curl_easy_cleanup(curl) ;
	  if (UCempty(ctx->ErrorMsgAux)) { UCstrcpyAtoU(ctx->ErrorMsgAux,curlErrs) ; } ;
	  if (!ok) 
	  { if (UCempty(ctx->ErrorMsgAux)) { v_Msg(ctx,NULL,"FTPCURLErr",intmodx,curlEMne,res) ; } else { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; } ;
	    goto fail ;
	  } ;
	}
#endif /* HAVECURL */

#if defined HAVEWININET && !defined HAVEWINHTTP
/*
HINTERNET InternetConnect(
  __in  HINTERNET hInternet,
  __in  LPCTSTR lpszServerName,
  __in  INTERNET_PORT nServerPort,
  __in  LPCTSTR lpszUsername,
  __in  LPCTSTR lpszPassword,
  __in  DWORD dwService,
  __in  DWORD dwFlags,
  __in  DWORD_PTR dwContext
);
BOOL FtpSetCurrentDirectory(
  __in  HINTERNET hConnect,
  __in  LPCTSTR lpszDirectory
);
HINTERNET FtpFindFirstFile(
  __in   HINTERNET hConnect,
  __in   LPCTSTR lpszSearchFile,
  __out  LPWIN32_FIND_DATA lpFindFileData,
  __in   DWORD dwFlags,
  __in   DWORD_PTR dwContext
);
typedef struct _WIN32_FIND_DATA {
  DWORD    dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD    nFileSizeHigh;
  DWORD    nFileSizeLow;
  DWORD    dwReserved0;
  DWORD    dwReserved1;
  TCHAR    cFileName[MAX_PATH];
  TCHAR    cAlternateFileName[14];
} WIN32_FIND_DATA, *PWIN32_FIND_DATA, *LPWIN32_FIND_DATA;
BOOL FtpGetFile(
  __in  HINTERNET hConnect,
  __in  LPCTSTR lpszRemoteFile,
  __in  LPCTSTR lpszNewFile,
  __in  BOOL fFailIfExists,	//If TRUE and local file exists, then fails
  __in  DWORD dwFlagsAndAttributes,
  __in  DWORD dwFlags,
  __in  DWORD_PTR dwContext
);
BOOL FtpPutFile(
  __in  HINTERNET hConnect,
  __in  LPCTSTR lpszLocalFile,
  __in  LPCTSTR lpszNewRemoteFile,
  __in  DWORD dwFlags,
  __in  DWORD_PTR dwContext
);
BOOL FtpDeleteFile(
  __in  HINTERNET hConnect,
  __in  LPCTSTR lpszFileName
);
BOOL InternetCloseHandle(
  __in  HINTERNET hInternet
);
*/
#define WININETOK(MODNAME) if (!ok) { errMne = UClit(#MODNAME) ; goto wininet_fail ; } ;
	{ HINTERNET hanIN = NULL, hanFTP = NULL, hanFile = NULL ;
	  COUNTER fileCount = 0 ; size_t fileBytes = 0 ;
#if defined WINNT && defined V4UNICODE
	  struct _stat statbuf ;
#else
	  struct stat statbuf ;
#endif
	  struct V4L__ListPoint *lpRes = NULL ;
	  UCCHAR *errMne ;
	 
	  if (resMode == list) { INITLP(respnt,lpRes,Dim_List) } ;	/* Initi result - list of files */
	  ZUS(ctx->ErrorMsgAux) ;					/*  (we may get Windows or V4 errors, if ErrorMsgAux is not blank then use that message) */
	  hanIN = UCInternetOpen(UClit("V4"),INTERNET_OPEN_TYPE_DIRECT,NULL,NULL,0) ; ok = (hanIN != NULL) ; WININETOK(InternetOpen)
	  hanFTP = UCInternetConnect(hanIN,host,INTERNET_DEFAULT_FTP_PORT,userName,password,INTERNET_SERVICE_FTP,INTERNET_FLAG_PASSIVE,0) ;  ok = (hanFTP != NULL) ; WININETOK(InternetConnect)

	  if (UCnotempty(dirRemote))
	   { ok = UCFtpSetCurrentDirectory(hanFTP,dirRemote) ;
	     WININETOK(FtpSetCurrentDirectory) ;
	   } ;
	
	  if (resMode == list) { INITLP(respnt,lpRes,Dim_List) } ;	/* Initi result - list of files */
	
	  switch (ftpMode)
	   {
	     case get:
		if (lpRes != NULL)
		 { P fpnt ; uccharPNTv(&fpnt,(UCnotempty(asFile) ? asFile : getFile)) ;
		   if (!v4l_ListPoint_Modify(gpi->ctx,lpRes,V4L_ListAction_Append,&fpnt,0))
		    { v_Msg(NULL,gpi->ctx->ErrorMsgAux,"LPModMaxSize") ; goto wininet_fail ; } ;
		 } ;
		ok = UCFtpGetFile(hanFTP,getFile,(UCnotempty(asFile) ? asFile : getFile),FALSE,0,(isText ? FTP_TRANSFER_TYPE_ASCII : FTP_TRANSFER_TYPE_BINARY),0) ;
		fileCount++ ;
		if (resMode == bytes)
		  { if (UCstat((UCnotempty(asFile) ? asFile : getFile),&statbuf) != -1) fileBytes += statbuf.st_size ; } ;
		WININETOK(FtpGetFile) ;
		break ;
	     case mget:
		{ WIN32_FIND_DATAW findData ; UCCHAR localFile[V_FileName_Max] ;
		  hanFile = UCFtpFindFirstFile(hanFTP,getFile,(WIN32_FIND_DATAW *)&findData,0,0) ;
		  for(;;)
		   { if (!UCInternetFindNextFile(hanFile,&findData))
		      { ok = (GetLastError() == ERROR_NO_MORE_FILES) ; WININETOK(InternetFindNextFileW)
		        break ;
		      } ;
		     UCstrcpy(localFile,dirLocal) ; UCstrcat(localFile,findData.cFileName) ;
		     if (lpRes != NULL)
		      { P fpnt ; uccharPNTv(&fpnt,localFile) ;
		        if (!v4l_ListPoint_Modify(gpi->ctx,lpRes,V4L_ListAction_Append,&fpnt,0))
		         { v_Msg(NULL,gpi->ctx->ErrorMsgAux,"LPModMaxSize") ; goto wininet_fail ; } ;
		      } ;
		     ok = UCFtpGetFile(hanFTP,findData.cFileName,localFile,FALSE,0,(isText ? FTP_TRANSFER_TYPE_ASCII : FTP_TRANSFER_TYPE_BINARY),0) ;
		     WININETOK(FtpGetFile) ;
		     fileCount++ ; fileBytes += findData.nFileSizeLow ;
		   } ;
		}
		break ;
	     case put:
	     case mfput:
		{ UCCHAR localFile[V_FileName_Max] ; INDEX fx ; P fnPnt ;
/*		  Verify that the local file exists */
		  if (pflp != NULL)						/* If a list of files - verify all of them */
		   { for(fx=1;v4l_ListPoint_Value(ctx,pflp,fx,&fnPnt) > 0;fx++)
		      { struct stat statbuf ; struct UC__File UCFile ;
		        v4im_GetPointUC(&ok,putFile,UCsizeof(putFile),&fnPnt,ctx) ; UCstrcpy(localFile,dirLocal) ; UCstrcat(localFile,putFile) ;
			if (!v_UCFileOpen(&UCFile,localFile,UCFile_Open_ReadBin,TRUE,ctx->ErrorMsgAux,intmodx)) goto wininet_fail ;
			fstat(UCFile.fd,&statbuf) ; fileBytes += statbuf.st_size ; fileCount++ ;
			v_UCFileClose(&UCFile) ;
		      } ; if (fx == 1) { v_Msg(ctx,ctx->ErrorMsgAux,"FTPEmptyList",V4IM_Tag_Put,&pflPnt) ; goto wininet_fail ; } ;
		   } ;
		  for(fx=1;;fx++)
		   { UCCHAR *fn,*s ;
		     if (pflp != NULL)						/* Doing a list of files- get next one */
		      { if (v4l_ListPoint_Value(ctx,pflp,fx,&fnPnt) <= 0) break ;
		        v4im_GetPointUC(&ok,putFile,UCsizeof(putFile),&fnPnt,ctx) ;
		      } ;
		     UCstrcpy(localFile,dirLocal) ; UCstrcat(localFile,putFile) ;
		     for(s=localFile,fn=localFile;*s!=UCEOS;s++)			/* Strip off device & directory so we pass just the filename do the remote */
		      { if (*s == UClit('/') || *s == UClit('\\') || *s == UClit(':')) fn = s + 1 ; } ;
		     if (lpRes != NULL)
		      { P fpnt ; uccharPNTv(&fpnt,(UCnotempty(asFile) ? asFile : fn)) ;
		        if (!v4l_ListPoint_Modify(gpi->ctx,lpRes,V4L_ListAction_Append,&fpnt,0))
		         { v_Msg(NULL,gpi->ctx->ErrorMsgAux,"LPModMaxSize") ; goto wininet_fail ; } ;
		      } ;
		     ok = UCFtpPutFile(hanFTP,localFile,fn,(isText ? FTP_TRANSFER_TYPE_ASCII : FTP_TRANSFER_TYPE_BINARY),0) ; WININETOK(FtpPutFile) ;
		     if (pflp == NULL) break ;						/* Not doing a list - quit */
		   } ;
		ok = UCFtpPutFile(hanFTP,putFile,(UCnotempty(asFile) ? asFile : putFile),(isText ? FTP_TRANSFER_TYPE_ASCII : FTP_TRANSFER_TYPE_BINARY),0) ;
		} break ;
//	     case delete:
//		ok = FtpDeleteFile(hanFTP,delFile) ;
//		WININETOK(FtpDeleteFile) ; break ;
//	     case rename
//		ok = FtpRenameFile(hanFTP,existingFile,newFile) ;
//		WININETOK(FtpRenameFile) ; break ;
	   } ;	  

	  switch (resMode)
	   { case bytes:	intPNTv(respnt,fileBytes) ; break ;
	     case files:	intPNTv(respnt,fileCount) ; break ;
	     case list:		ENDLP(respnt,lpRes) ; break ;
	   } ;

	  ok = TRUE ; goto wininet_cleanup ;

wininet_fail:
	  ok = FALSE ;
	  { DWORD lastErr,bufSize ; LOGICAL ok ;
	    bufSize = sizeof ctx->ErrorMsgAux ;
	    ok = UCInternetGetLastResponseInfo(&lastErr,ctx->ErrorMsgAux,&bufSize) ;
	  }

/*	When all done */
wininet_cleanup:
	  if (hanIN != NULL) InternetCloseHandle(hanIN) ; if (hanFTP != NULL) InternetCloseHandle(hanFTP) ; if (hanFile != NULL) InternetCloseHandle(hanFile) ;
	  if (!ok) 
	  { if (UCempty(ctx->ErrorMsgAux)) { v_Msg(ctx,NULL,"FTPWININETErr",intmodx,errMne,GetLastError()) ; } else { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; } ;
	    goto fail ;
	  } ;
	}

#endif /* HAVEWININET */

	return(respnt) ;
fail:
	REGISTER_ERROR(0) ; RETURNFAILURE ;
}
#endif
	
/*	S E N D M A I L   M O D U L E		*/

#ifdef HAVECURL
enum SEGMENT_TYPE { text, html, attachment, nameValue } ;
enum CONNECTION_TYPE { plain, tryTLS, tls } ;
  struct lcl__SMTPSegment {
    struct lcl__SMTPSegment *lssNext ;		/* Link to next segment, NULL if end of chain */
    enum SEGMENT_TYPE segType ;
    LOGICAL firstAttachment ;			/* TRUE if this is the first attachment */
    UCCHAR segmentFile[V_FileName_Max] ;	/* File name if to be pulled from external file */
    UCCHAR alias[V_FileName_Max] ;		/* Name to be used as the attachment file name */
    BYTE *segmentPtr ;				/* Holds segment data if passed in directly from V4 */
    size_t segmentBytes ;			/* Number of bytes in segment data. NOTE: if segmentPtr not NULL then segmentFile s/b empty & vice versa */
    FILEID fileId ;				/* If not UNUSED then take data from Output() buffer pointed to by this fileId */
    struct UC__File UCFile ;			/* If a file, then open on this */
   } ;
struct lcl__SMTPCallback {
  COUNTER callCount ;			/* Incremented with each call to vSMTP_GetChunk */
  COUNTER segCount ;			/* Sub counter for within segment */
  INDEX totalBytes ;			/* Tracks number of bytes sent so far for current segment */
  LOGICAL haveHTML ;			/* TRUE if have HTML message */
  LOGICAL haveText ;			/* TRUE if have Text message */
  LOGICAL haveAttach ;			/* TRUE if have attachment(s) */
  enum CONNECTION_TYPE connection ;
  LOGICAL verbose ;
  UCCHAR to[8192] ;
  UCCHAR cc [8192] ;
  UCCHAR from[128] ;
  UCCHAR subject[256] ;
  UCCHAR replyTo[256] ;
  UCCHAR host[256] ;
  UCCHAR userName[256] ;
  UCCHAR password[254] ;
  UCCHAR logFile[V_FileName_Max] ;
  char boundary[64] ;			/* MIME multi-part boundary pattern */
  char boundary2[64] ;			/* Secondary MIME multi-part boundary pattern */
  struct lcl__SMTPSegment *lssFirst ;	/* First segment */
  struct lcl__SMTPSegment *lss ;	/* Current segment */
} ;
#define ALLOC_LSS \
  lss = (struct lcl__SMTPSegment *)v4mm_AllocChunk(sizeof *lss,TRUE) ; \
  if (lscb.lss != NULL) lscb.lss->lssNext = lss ; lscb.lss = lss ; \
  if (lscb.lssFirst == NULL) lscb.lssFirst = lss ; \
  lss->fileId = UNUSED ;
   
static size_t vSMTP_GetChunk(chunk, size, nmemb, lscb)
  void *chunk ;
  size_t size, nmemb ;
  struct lcl__SMTPCallback *lscb ;
{
  char data[512] ;
  size_t bytes ;

	switch (++lscb->callCount)
	 {
	   default:	goto segment ;
	   case 1:	sprintf(data,"Date: %s\n",v_FormatHTTPDateTime(UNUSED,0)) ; break ;
	   case 2:	sprintf(data,"To: %s\n",UCretASC(lscb->to)) ; break ;
	   case 3:	if (UCempty(lscb->cc)) { lscb->callCount++ ; }
			 else { sprintf(data,"CC: %s\n",UCretASC(lscb->cc)) ; break ; } ;
	   case 4:	sprintf(data,"From: %s\n",UCretASC(lscb->from)) ; break ;
	   case 5:	sprintf(data,"Subject: %s\n",UCretASC(lscb->subject)) ; break ;
	   case 6:	strcpy(data,"MIME-Version: 1.0\n") ; break ;
	   case 7:	if (UCempty(lscb->replyTo)) { lscb->callCount++ ; }	/* And just fall thru to next case */
			 else { sprintf(data,"Reply-To: %s\n",UCretASC(lscb->replyTo)) ; break ; } ;
	   case 8:	sprintf(lscb->boundary,"---------------------------%08x%08xa",(int)clock()-(int)time(NULL),(int)time(NULL)+(int)clock()) ; lscb->boundary[46] = '\0' ;
			sprintf(lscb->boundary2,"---------------------------%08x%08xb",(int)clock()-(int)time(NULL),(int)time(NULL)+(int)clock()) ; lscb->boundary[46] = '\0' ;
			{ 
//			  struct lcl__SMTPSegment *lss = lscb->lssFirst ; LOGICAL haveAttach = FALSE ;
//			  for(;lss!=NULL;lss=lss->lssNext)
//			   { if (lss->segType == attachment) { haveAttach = TRUE ; break ; } ;
//			   } ;
//			  sprintf(data,"Content-Type: multipart/%s; boundary=%s\n",(haveAttach ? "mixed" : "alternative"),lscb->boundary) ;		//Use multipart/alternative for HTML & text versions of same email

/*			  Outer boundary between messages and attachments */
			  sprintf(data,"Content-Type: multipart/%s; boundary=%s\n","mixed",lscb->boundary) ;
			  
			}
			break ;
/*			Inner for different message types (html & text) */
	   case 9:	sprintf(data,"\n\n--%s\n",lscb->boundary) ; break ;
	   case 10:	sprintf(data,"Content-Type: multipart/%s; boundary=%s\n","alternative",lscb->boundary2) ; break ;
	   case 11:	lscb->lss = lscb->lssFirst ; lscb->segCount = 0 ; lscb->totalBytes = 0 ; goto segment ;
	 } ;
	bytes = strlen(data) ; memcpy(chunk,data,bytes) ; return(bytes) ;
/*	Here to handle current segment */
segment:
	if (lscb->lss == NULL) return(0) ;		/* All done */
	switch(++lscb->segCount)
	 { 
	   default:
/*		This section actually transfers the data, when done it advances to next segment */
/*		If we have file name then read in file (already open) */
		if (lscb->lss->segmentPtr == NULL)	/* If this is a file, get its size (if it's a pointer then we already know it) */
		 { BYTE *temp ; size_t b64Bytes ; struct stat statbuf ; fstat(lscb->lss->UCFile.fd,&statbuf) ;
		   temp = v4mm_AllocChunk(statbuf.st_size+10,TRUE) ;  read(lscb->lss->UCFile.fd,temp,statbuf.st_size) ; v_UCFileClose(&lscb->lss->UCFile) ;
		   temp[statbuf.st_size] = '\0' ;
		   switch(lscb->lss->segType)
		    { case text:
		      case html:
			lscb->lss->segmentPtr = temp ; lscb->lss->segmentBytes = statbuf.st_size ;
			break ;
		      case attachment:
			b64Bytes = (statbuf.st_size * 4) / 3 + 128 ; lscb->lss->segmentPtr = v4mm_AllocChunk(b64Bytes,FALSE) ;
			lscb->lss->segmentBytes = V_Encode64(temp,statbuf.st_size,lscb->lss->segmentPtr,b64Bytes,gpi->ctx->ErrorMsgAux) ;
			v4mm_FreeChunk(temp) ; break ;
		    } ;
		 } ;
/*		Are we done with this segment? If so then advance to next  */
		if (lscb->totalBytes >= lscb->lss->segmentBytes)
		 { v4mm_FreeChunk(lscb->lss->segmentPtr) ;		/* Free up memory allocated for this segment's data */
		   lscb->lss = lscb->lss->lssNext ; lscb->segCount = 0 ; lscb->totalBytes = 0 ;
		   if (lscb->lss != NULL) goto segment ;
/*		   No more segments - output final boundary with trailing '--' to indicate EOM */
		   sprintf(data,"\n--%s--\n",lscb->boundary) ;
		   bytes = strlen(data) ; memcpy(chunk,data,bytes) ; return(bytes) ;
		 } ;
		{ LENMAX chunkBytes ;
		  chunkBytes =  lscb->lss->segmentBytes - lscb->totalBytes ; if (chunkBytes > size * nmemb) chunkBytes = size * nmemb ;
		  if (chunkBytes > 998) chunkBytes = 998 ;		/* Don't let line exceed 1000 (data + CR/LF) */
		  memcpy(chunk,&lscb->lss->segmentPtr[lscb->totalBytes],chunkBytes) ; *(((char *)chunk)+chunkBytes) = '\0' ;
		  lscb->totalBytes += chunkBytes ;
		  return(chunkBytes) ;
		} 
	   case 1:
//		sprintf(data,"\n\n--%s\n",lscb->boundary) ; break ;
	   	switch(lscb->lss->segType)
		 { case text:
		   case html:		sprintf(data,"\n\n--%s\n",lscb->boundary2) ; break ;
		   case nameValue:
		   case attachment:	if (lscb->lss->firstAttachment)
					 { sprintf(data,"\n\n--%s--\n--%s\n",lscb->boundary2,lscb->boundary) ;
					 } else { sprintf(data,"\n\n--%s\n",lscb->boundary) ; } ;
					break ;
		 } ;
		break ;
	   case 2:
	   	switch(lscb->lss->segType)
		 { case text:		sprintf(data,"Content-Transfer-Encoding: 8BIT\nContent-Type: text/plain\n") ; break ;
		   case html:		sprintf(data,"Content-Transfer-Encoding: 8BIT\nContent-Type: text/html\n") ; break ;
		   case nameValue:	sprintf(data,"Content-Disposition: form-data\nname=%ws\n",lscb->lss->alias) ; break ;
		   case attachment:	{ char conType[128] ; UCCHAR *fn ;
					  fn = (UCnotempty(lscb->lss->alias) ? lscb->lss->alias : lscb->lss->segmentFile) ;
					  if(UCstrstr(fn,UClit(".pdf")) != NULL || UCstrstr(fn,UClit(".PDF")) != NULL)
					   { strcpy(conType,"application/pdf") ; } else { strcpy(conType,"application/octet-stream") ; } ;
					  sprintf(data,"Content-Transfer-Encoding: BASE64\nContent-Type: %s\nContent-Disposition: attachment; filename=%s\n",conType,UCretASC(fn)) ;
					} break ;
		 } ;
		break ;
	   case 3:
		strcpy(data,"\n") ; break ;
	 } ;
	bytes = strlen(data) ; memcpy(chunk,data,bytes) ; return(bytes) ;
 }

P *v4im_DoSendMail(ctx,respnt,argcnt,argpnts,intmodx,trace)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  P *respnt,*argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ 
  P *cpt, argpt ;
  struct lcl__SMTPCallback lscb ;
  struct lcl__SMTPSegment *lss = NULL ;
  enum SEGMENT_TYPE segType ;
  
  INDEX ax ; LOGICAL ok ; COUNTER port ;

#define SMSKIPNONE if (memcmp(cpt,&protoNone,protoNone.Bytes) == 0) continue ;
	port = 587 ;
	memset(&lscb,0,sizeof(lscb)) ; segType = text ; lscb.connection = tls ;

	for(ax=1,ok=TRUE;ok && ax<=argcnt;ax++)			/* Loop thru all arguments */
	 { if (argpnts[ax]->PntType != V4DPI_PntType_TagVal)
	    { if (lss == NULL ? TRUE : lss->segType != nameValue || lss->segmentBytes != UNUSED)
	       { v_Msg(ctx,NULL,"HTTPNoName",intmodx,ax,V4IM_Tag_Name) ; goto fail ; } ;
	      v4im_GetPointChar(&ok,ASCTBUF1,V4TMBufMax,argpnts[ax],ctx) ;
	      lss->segmentBytes = strlen(ASCTBUF1)+1 ; lss->segmentPtr = v4mm_AllocChunk(lss->segmentBytes,FALSE) ; strcpy(lss->segmentPtr,ASCTBUF1) ;
	      continue ;
	    } ;
	   switch (v4im_CheckPtArgNew(ctx,argpnts[ax],&cpt,&argpt))
	    { default:	    		v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	      case V4IM_Tag_Attach:	SMSKIPNONE
					ALLOC_LSS ; lss->segType = attachment ;
					{ P idptBuf, *idpt = NULL ;
					  if (cpt->PntType == V4DPI_PntType_List)
					   { struct V4L__ListPoint *lp = v4im_VerifyList(cpt,ctx,cpt,0) ; INDEX i ; P lpnt ;
					     for(i=1;v4l_ListPoint_Value(ctx,lp,i,&lpnt) > 0;i++)
					      { switch (i)
					         { default:	break ;
						   case 1:	switch(lpnt.PntType)
								 { default:	memcpy(&idptBuf,&lpnt,lpnt.Bytes) ; idpt = &idptBuf ; break ;
								   CASEofChar	v4im_GetPointUC(&ok,lss->segmentFile,UCsizeof(lss->segmentFile),&lpnt,ctx) ;
								 } ; break ;
						   case 2:	v4im_GetPointUC(&ok,lss->alias,UCsizeof(lss->alias),&lpnt,ctx) ; break ;
					         } ;
					      } ;
					   } else
					   { switch(cpt->PntType)
					      { default:	idpt = cpt ; break ;
					        CASEofChar	v4im_GetPointUC(&ok,lss->segmentFile,UCsizeof(lss->segmentFile),cpt,ctx) ;
					      } ;
					   } ;
					  if (idpt == NULL)
					   { if (!v_UCFileOpen(&lss->UCFile,lss->segmentFile,UCFile_Open_ReadBin,TRUE,gpi->ctx->ErrorMsgAux,V4IM_OpCode_SendMail))
					      { v_Msg(ctx,NULL,"FileError1a",intmodx,ctx->ErrorMsgAux) ; goto fail ; } ;
					   } else
					   { LENMAX b64Bytes,chars,bytes ; char *temp ; UCCHAR *obuf ;
					     lss->fileId = vout_PntIdToFileId(ctx,(struct V4DPI__LittlePoint *)idpt) ;
					     if (lss->fileId == UNUSED)  { v_Msg(ctx,NULL,"OutputNoId",intmodx,V4IM_Tag_Attach,idpt) ; goto fail ; } ;
/*					     First have to convert UCCHAR output buffer to UTF8 so that it is char */
					     chars = vout_CountForFile(lss->fileId,UNUSED,FALSE,ctx->ErrorMsgAux) ;
					     temp = v4mm_AllocChunk(chars * 2,FALSE) ;
					     obuf = vout_GetOutputBuffer(lss->fileId,UNUSED,ctx->ErrorMsgAux) ; if (obuf == NULL) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,argpnts[ax]) ; goto fail ; } ;
					     bytes = UCUTF16toUTF8(temp,chars*2,obuf,chars) ;
/*					     Now convert char stream to encoded-64 */
					     b64Bytes = (bytes * 4) / 3 + 128 ;
					     lss->segmentPtr = v4mm_AllocChunk(b64Bytes,FALSE) ;
					     lss->segmentBytes = V_Encode64(temp,bytes,lss->segmentPtr,b64Bytes,gpi->ctx->ErrorMsgAux) ;
					     v4mm_FreeChunk(temp) ;
					     if (UCempty(lss->alias)) UCstrcpy(lss->alias,UClit("attachment")) ;
					   } ;
					}
					break ;
	      case V4IM_Tag_CC:		SMSKIPNONE
					{ P lpnt,lvpnt ; struct V4L__ListPoint *lp = v4im_VerifyList(&lpnt,ctx,cpt,0) ; INDEX i ;
					  for(i=1;v4l_ListPoint_Value(ctx,lp,i,&lvpnt) > 0;i++)
					   { UCCHAR toStr[256] ; LENMAX len ; v4im_GetPointUC(&ok,toStr,UCsizeof(toStr),&lvpnt,ctx) ;
					     len = UCstrlen(lscb.cc) ; if (len + UCstrlen(toStr) + 2 >= UCsizeof(lscb.cc)) { v_Msg(ctx,NULL,"RptMaxChars",intmodx,ax,UCsizeof(lscb.cc),V4IM_Tag_CC) ; goto fail ; } ;
					     if (len > 0) { UCstrcat(lscb.cc,UClit("\r")) ; } ;
					     UCstrcat(lscb.cc,toStr) ;
					   } ;
					} break ;
	      case V4IM_Tag_Connection:	switch (v4im_GetDictToEnumVal(ctx,cpt))
					 { default:		v_Msg(ctx,NULL,"ModTagValue",intmodx,ax,V4IM_Tag_Connection,cpt) ; goto fail ;
					   case _Plain:		lscb.connection = plain ; break ;
					   case _TLS:		lscb.connection = tls ; break ;
					   case _Try:		lscb.connection = tryTLS ;break ;
					 } ; break ;
	      case V4IM_Tag_Format:	switch (v4im_GetDictToEnumVal(ctx,cpt))
					 { default:		v_Msg(ctx,NULL,"ModTagValue",intmodx,ax,V4IM_Tag_Format,cpt) ; goto fail ;
					   case _HTML:		break ;
					   case _Text:		break ;
					 } ; break ;
	      case V4IM_Tag_From:	SMSKIPNONE v4im_GetPointUC(&ok,lscb.from,UCsizeof(lscb.from),cpt,ctx) ; break ;
	      case V4IM_Tag_Host:	SMSKIPNONE v4im_GetPointUC(&ok,lscb.host,UCsizeof(lscb.host),cpt,ctx) ; break ;
	      case -V4IM_Tag_HTML:	segType = html ; break ;
	      case V4IM_Tag_HTML:	SMSKIPNONE
					ALLOC_LSS lss->segType = html ;

					switch (cpt->PntType)
					 { default:	lss->fileId = vout_PntIdToFileId(ctx,(struct V4DPI__LittlePoint *)cpt) ;
							if (lss->fileId == UNUSED)  { v_Msg(ctx,NULL,"OutputNoId",intmodx,V4IM_Tag_Include,cpt) ; goto fail ; } ;
							lss->segmentBytes = vout_CountForFile(lss->fileId,UNUSED,FALSE,ctx->ErrorMsgAux) ;
							lss->segmentPtr = v4mm_AllocChunk(lss->segmentBytes * 2,FALSE) ;
							{ UCCHAR *obuf ;
							  obuf = vout_GetOutputBuffer(lss->fileId,UNUSED,ctx->ErrorMsgAux) ; if (obuf == NULL) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,argpnts[ax]) ; goto fail ; } ;
							  lss->segmentBytes = UCUTF16toUTF8(lss->segmentPtr,lss->segmentBytes*2,obuf,lss->segmentBytes) ;
							}
							break ;
					   CASEofChar	v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,cpt,ctx) ;
							{ LENMAX len = UCUTF16toUTF8(ASCTBUF1,V4TMBufMax,UCTBUF1,UCstrlen(UCTBUF1)) ;
							  lss->segmentBytes = len ; lss->segmentPtr = (char *)v4mm_AllocChunk(len+1,FALSE) ; strcpy(lss->segmentPtr,ASCTBUF1) ;
							} break ;
					 } ;
					break ;
	      case V4IM_Tag_Include:	SMSKIPNONE
					ALLOC_LSS lss->segType = segType ;
					switch (cpt->PntType)
					 { default:	lss->fileId = vout_PntIdToFileId(ctx,(struct V4DPI__LittlePoint *)cpt) ;
							if (lss->fileId == UNUSED)  { v_Msg(ctx,NULL,"OutputNoId",intmodx,V4IM_Tag_Include,cpt) ; goto fail ; } ;
							lss->segmentBytes = vout_CountForFile(lss->fileId,UNUSED,FALSE,ctx->ErrorMsgAux) ;
							lss->segmentPtr = v4mm_AllocChunk(lss->segmentBytes * 2,FALSE) ;
							{ UCCHAR *obuf = vout_GetOutputBuffer(lss->fileId,UNUSED,ctx->ErrorMsgAux) ; if (obuf == NULL) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,argpnts[ax]) ; goto fail ; } ;
							  lss->segmentBytes = UCUTF16toUTF8(lss->segmentPtr,lss->segmentBytes*2,obuf,lss->segmentBytes) ;
							}
							break ;
					   CASEofChar	v4im_GetPointUC(&ok,lss->segmentFile,UCsizeof(lss->segmentFile),cpt,ctx) ;
							if (!v_UCFileOpen(&lss->UCFile,lscb.lss->segmentFile,UCFile_Open_ReadBin,TRUE,gpi->ctx->ErrorMsgAux,V4IM_OpCode_SendMail))
							 { v_Msg(ctx,NULL,"FileError1a",intmodx,ctx->ErrorMsgAux) ; goto fail ; } ;
							break ;
					 } ;
					break ;
	      case V4IM_Tag_Name:	ALLOC_LSS ; lss->segType = nameValue ;
					v4im_GetPointUC(&ok,lss->alias,UCsizeof(lss->alias),cpt,ctx) ; lss->segmentBytes = UNUSED ;
					break ;
	      case V4IM_Tag_Password:	SMSKIPNONE v4im_GetPointUC(&ok,lscb.password,UCsizeof(lscb.password),cpt,ctx) ; break ;
	      case V4IM_Tag_Port:	port = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Reply:	SMSKIPNONE v4im_GetPointUC(&ok,lscb.replyTo,UCsizeof(lscb.replyTo),cpt,ctx) ; break ;
	      case V4IM_Tag_Subject:	SMSKIPNONE v4im_GetPointUC(&ok,lscb.subject,UCsizeof(lscb.subject),cpt,ctx) ; break ;
	      case -V4IM_Tag_Text:	segType = text ; break ;
	      case V4IM_Tag_Text:	SMSKIPNONE
					ALLOC_LSS lss->segType = text ;
					
					
					switch (cpt->PntType)
					 { default:	lss->fileId = vout_PntIdToFileId(ctx,(struct V4DPI__LittlePoint *)cpt) ;
							if (lss->fileId == UNUSED)  { v_Msg(ctx,NULL,"OutputNoId",intmodx,V4IM_Tag_Include,cpt) ; goto fail ; } ;
							lss->segmentBytes = vout_CountForFile(lss->fileId,UNUSED,FALSE,ctx->ErrorMsgAux) ;
							lss->segmentPtr = v4mm_AllocChunk(lss->segmentBytes * 2,FALSE) ;
							{ UCCHAR *obuf = vout_GetOutputBuffer(lss->fileId,UNUSED,ctx->ErrorMsgAux) ; if (obuf == NULL) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,argpnts[ax]) ; goto fail ; } ;
							  lss->segmentBytes = UCUTF16toUTF8(lss->segmentPtr,lss->segmentBytes*2,obuf,lss->segmentBytes) ;
							}
							break ;
					   CASEofChar	v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,cpt,ctx) ;
							{ LENMAX len = UCUTF16toUTF8(ASCTBUF1,V4TMBufMax,UCTBUF1,UCstrlen(UCTBUF1)) ;
							  lss->segmentBytes = len ; lss->segmentPtr = (char *)v4mm_AllocChunk(len+1,FALSE) ; strcpy(lss->segmentPtr,ASCTBUF1) ;
							} break ;
					 } ;
					break ;
					
					
//					v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,cpt,ctx) ;
//					{ LENMAX len = UCUTF16toUTF8(ASCTBUF1,V4TMBufMax,UCTBUF1,UCstrlen(UCTBUF1)) ;
//					  lss->segmentBytes = len ; lss->segmentPtr = (char *)v4mm_AllocChunk(len+1,FALSE) ; strcpy(lss->segmentPtr,ASCTBUF1) ;
//					} break ;



	      case V4IM_Tag_To:		SMSKIPNONE
					{ P lpnt,lvpnt ; struct V4L__ListPoint *lp = v4im_VerifyList(&lpnt,ctx,cpt,0) ; INDEX i ;
					  for(i=1;v4l_ListPoint_Value(ctx,lp,i,&lvpnt) > 0;i++)
					   { UCCHAR toStr[256] ; LENMAX len ; v4im_GetPointUC(&ok,toStr,UCsizeof(toStr),&lvpnt,ctx) ;
					     len = UCstrlen(lscb.to) ; if (len + UCstrlen(toStr) + 2 >= UCsizeof(lscb.to)) { v_Msg(ctx,NULL,"RptMaxChars",intmodx,ax,UCsizeof(lscb.to),V4IM_Tag_To) ; goto fail ; } ;
					     if (len > 0) { UCstrcat(lscb.to,UClit("\r")) ; } ;
					     UCstrcat(lscb.to,toStr) ;
					   } ;
					} break ;
	      case V4IM_Tag_Trace:	SMSKIPNONE
					if (cpt->PntType == V4DPI_PntType_Logical)
					 { lscb.verbose = v4im_GetPointLog(&ok,cpt,ctx) ; }
					 else { lscb.verbose = TRUE ; v4im_GetPointUC(&ok,lscb.logFile,UCsizeof(lscb.logFile),cpt,ctx) ; } ;
					break ;
	      case V4IM_Tag_User:	SMSKIPNONE v4im_GetPointUC(&ok,lscb.userName,UCsizeof(lscb.userName),cpt,ctx) ; break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax-1) ; goto fail ; } ;

/*	Now reorder the sections: messages first then attachments */
#define MAX_LSS_TYPE 128
	{ struct lcl__SMTPSegment *lss, *lssMsg[MAX_LSS_TYPE], *lssAttach[MAX_LSS_TYPE] ;
	  INDEX i, lm=0, la=0 ;
	  for(lss=lscb.lssFirst;lss!=NULL;lss=lss->lssNext)
	   { switch(lss->segType)
	      { case html:		lscb.haveHTML = TRUE ; lssMsg[lm++] = lss ; break ;
	        case text:		lscb.haveText = TRUE ; lssMsg[lm++] = lss ; break ;
		case attachment:
		case nameValue:		lscb.haveAttach = TRUE ; lssAttach[la++] = lss ; break ;
	      } ;
	   } ;
	  lscb.lssFirst = NULL ;
	  for(i=0;i<lm;i++)
	   { if (lscb.lssFirst == NULL) { lscb.lssFirst = lssMsg[i] ; }
	      else { lss->lssNext = lssMsg[i] ; }
	     lss = lssMsg[i] ; lss->lssNext = NULL ;
	   } ;
	  for(i=0;i<la;i++)
	   { if (lscb.lssFirst == NULL) { lscb.lssFirst = lssAttach[i] ; }
	      else { lss->lssNext = lssAttach[i] ; }
	     lss = lssAttach[i] ; lss->lssNext = NULL ;
	     if (i == 0) lss->firstAttachment = TRUE ;
	   } ;
	}

/* JUST FOR TESTING!!!!! */
for(;FALSE;)
 { char chunk[16384] ; LENMAX res ;
   res = vSMTP_GetChunk(chunk, 1, sizeof(chunk), &lscb) ;
   if (res == 0) return(respnt) ;
   chunk[res] = '\0' ;
//   printf("********************res=%d, call=%d, seg=%d, bytes=%d\n",res,lscb.callCount,lscb.segCount,lscb.totalBytes) ;
   printf(chunk) ;
 } ;

	{ CURL *curl ;
	  struct curl_slist *toList = NULL ;		/* List of recipients */
	  char curlErrs[CURL_ERROR_SIZE], *curlEMne="", host[512] ;
	  CURLcode res ; LOGICAL ok ;

	  ZUS(ctx->ErrorMsgAux) ;					/*  (we may get Windows or V4 errors, if ErrorMsgAux is not blank then use that message) */
	  if (curlNeedInit) { curl_global_init(CURL_GLOBAL_ALL) ; curlNeedInit = FALSE ; } ;
	  curl = curl_easy_init() ;
	  CURLOPTION(CURLOPT_ERRORBUFFER,curlErrs)	ZS(curlErrs)		/* To receive readable error message */

    /* This is the URL for your mailserver. Note the use of port 587 here,
     * instead of the normal SMTP port (25). Port 587 is commonly used for
     * secure mail submission (see RFC4403), but you should use whatever
     * matches your server configuration. */ 
	  sprintf(host,"smtp://%s:%d",UCretASC(lscb.host),port) ;
	  CURLOPTION(CURLOPT_URL,host);
 
    /* In this example, we'll start with a plain text connection, and upgrade
     * to Transport Layer Security (TLS) using the STARTTLS command. Be careful
     * of using CURLUSESSL_TRY here, because if TLS upgrade fails, the transfer
     * will continue anyway - see the security discussion in the libcurl
     * tutorial for more details. */
	  switch(lscb.connection)
	   { case plain:	CURLOPTION(CURLOPT_USE_SSL, (long)CURLUSESSL_NONE) ; break ;
	     case tryTLS:	CURLOPTION(CURLOPT_USE_SSL, (long)CURLUSESSL_TRY) ; break ;
	     case tls:		CURLOPTION(CURLOPT_USE_SSL, (long)CURLUSESSL_ALL) ; break ;
	   } ;
////	  CURLOPTION(CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
 
    /* If your server doesn't have a valid certificate, then you can disable
     * part of the Transport Layer Security protection by setting the
     * CURLOPT_SSL_VERIFYPEER and CURLOPT_SSL_VERIFYHOST options to 0 (false). */
	  CURLOPTION(CURLOPT_SSL_VERIFYPEER, 0L);
	  CURLOPTION(CURLOPT_SSL_VERIFYHOST, 0L);
    /* That is, in general, a bad idea. It is still better than sending your
     * authentication details in plain text though.
     * Instead, you should get the issuer certificate (or the host certificate
     * if the certificate is self-signed) and add it to the set of certificates
     * that are known to libcurl using CURLOPT_CAINFO and/or CURLOPT_CAPATH. See
     * docs/SSLCERTS for more information.
     */ 
//	  CURLOPTION(CURLOPT_CAINFO, "/path/to/certificate.pem");
 
    /* A common reason for requiring transport security is to protect
     * authentication details (user names and passwords) from being "snooped"
     * on the network. Here is how the user name and password are provided: */ 
	  if (UCnotempty(lscb.userName)) { CURLOPTION(CURLOPT_USERNAME,UCretASC(lscb.userName)) ; } ;
	  if (UCnotempty(lscb.password)) { CURLOPTION(CURLOPT_PASSWORD,UCretASC(lscb.password)) ; } ;

    /* value for envelope reverse-path */ 
	  if (UCstrchr(lscb.from,'<') == NULL)
	   { CURLOPTION(CURLOPT_MAIL_FROM,UCretASC(lscb.from)) ; } ;
    /* Add two recipients, in this particular case they correspond to the
     * To: and Cc: addressees in the header, but they could be any kind of
     * recipient. */ 
	  { UCCHAR *s,*e ;
	    for(s=lscb.to;;s=e+1)
	     { e = UCstrpbrk(s,UClit(" ;,\r\t")) ; if (e != UCEOS) *e = UCEOS ;
	       if (UCnotempty(s))
	        { toList = curl_slist_append(toList,UCretASC(s)) ; } ;		/* Append next 'to' name to the list */
	       if (e == NULL) break ;
	     } ;
	    for(s=lscb.cc;;s=e+1)
	     { e = UCstrpbrk(s,UClit(" ;,\r\t")) ; if (e != UCEOS) *e = UCEOS ;
	       if (UCnotempty(s))
	        { toList = curl_slist_append(toList,UCretASC(s)) ; } ;		/* Append next 'cc' name to the list */
	       if (e == NULL) break ;
	     } ;
	  } CURLOPTION(CURLOPT_MAIL_RCPT,toList);
 
    /* In this case, we're using a callback function to specify the data. You
     * could just use the CURLOPT_READDATA option to specify a FILE pointer to
     * read from.
     */ 
	  CURLOPTION(CURLOPT_READFUNCTION,vSMTP_GetChunk) ;
	  CURLOPTION(CURLOPT_READDATA,&lscb) ;
	  CURLOPTION(CURLOPT_UPLOAD,1) ;

    /* Since the traffic will be encrypted, it is very useful to turn on debug
     * information within libcurl to see what is happening during the transfer.
     */ 
	  if (lscb.verbose)							/* Switch on full protocol/debug output */
	   { CURLOPTION(CURLOPT_VERBOSE, 1L) ;
	     if (UCnotempty(lscb.logFile)) { UCfreopen(lscb.logFile,"w",stdout ) ; } ;
	   } ;
 
    /* send the message (including headers) */ 
	  res = curl_easy_perform(curl) ;
	  if (res != CURLE_OK) { curlEMne = "curl_easy_perform" ; goto curl_fail ; } ;

	  ok = TRUE ; goto curl_cleanup ;

curl_fail:
	  ok = FALSE ; goto curl_cleanup ;

/*	When all done */
curl_cleanup:
	  if (toList != NULL) curl_slist_free_all(toList) ;
	  curl_easy_cleanup(curl) ;
	  if (UCempty(ctx->ErrorMsgAux)) { UCstrcpyAtoU(ctx->ErrorMsgAux,curlErrs) ; } ;
	  if (!ok) 
	  { if (UCempty(ctx->ErrorMsgAux)) { v_Msg(ctx,NULL,"FTPCURLErr",intmodx,curlEMne,res) ; } else { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; } ;
	    goto fail ;
	  } ;
	}

	logPNTv(respnt,TRUE) ; goto cleanup ;
	
fail:	respnt = NULL ; goto cleanup ;

cleanup:
	for(;lscb.lss != NULL;)
	 { struct lcl__SMTPSegment *lssX ;
	   lssX = lscb.lss ; lscb.lss = lscb.lss->lssNext ; v4mm_FreeChunk(lssX) ;
	 } ;
	if (respnt !=NULL) return(respnt) ;
	REGISTER_ERROR(0) ; RETURNFAILURE ;
}
#endif


/*	v4im_DoSocket - Handles Socket() module	*/

#define vSocketEcho_Send 1	/* Echo all sends */
#define vSocketEcho_Rcv 2	/* Echo all receives */

P *v4im_DoSocket(ctx,respnt,argcnt,argpnts,intmodx,trace)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  P *respnt,*argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ P *cpt, argpt, *ofpt ;
  int ax,ok,clsdone,reply, bytes,untilchar,echoflag,cnt ; int i,len,sent,chunk,res,wait,wait2,initialWait ;
  static int haveSocket = FALSE, socket ;
  fd_set sset,eset ;			/* Socket set */
  struct timeval tev ;
  struct stat statbuf ;
  struct UC__File UCFile ;
  UCCHAR hostOrURL[NI_MAXHOST], portOrSvc[NI_MAXHOST], fileName[V_FileName_Max],outFile[V_FileName_Max] ; char untilstr[256] ; int ifd ;
  BYTE *incdata ; int incbytes, tries ; LOGICAL isText ;
  static struct vsocket__Session {
    int Open ;			/* TRUE if session is open, FALSE to close, UNUSED if no session */
    int Bytes ;			/* Number of bytes received below */
    int EchoFlag ;		/* Echo flags - see vSocketEcho_xxx */
    int ResBytes ;		/* Number of bytes in result */
    char Result[V4TMBufMax] ;	/* Session returned results */
   } *vss = NULL ;

	clsdone = TRUE ; reply = TRUE ; bytes = UNUSED ; ZUS(hostOrURL) ; ZUS(portOrSvc) ; ZS(ASCTBUF1) ; ZUS(outFile) ;
	incbytes = UNUSED ; ofpt = NULL ; isText = TRUE ;
	initialWait = 30 ;					/* Default to wait 30 seconds for reply */
	untilchar = 26 /* MIDAS_EOM */ ; ZS(untilstr) ; echoflag = 0 ;
	for(ax=1,ok=TRUE;ok && ax<=argcnt;ax++)			/* Loop thru all arguments */
	 { 
	   switch (v4im_CheckPtArgNew(ctx,argpnts[ax],&cpt,&argpt))
	    { default:	    		v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto socket_fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto socket_fail ;
	      case V4IM_Tag_Bytes:	bytes = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Close:	clsdone = v4im_GetPointLog(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Create:	
		v4im_GetPointFileName(&ok,outFile,UCsizeof(outFile),cpt,ctx,NULL) ; if (!ok) break ;
		ofpt = cpt ; break ;
	      case V4IM_Tag_Echo:
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:		v_Msg(ctx,NULL,"SocketSesVal",intmodx,cpt,V4IM_Tag_Echo) ; goto socket_fail ;
		   case _Send:		echoflag |= vSocketEcho_Send ; break ;
		   case _Receive:	echoflag |= vSocketEcho_Rcv ; break ;
		   case _All:		echoflag |= (vSocketEcho_Send | vSocketEcho_Rcv) ; break ;
		 } ;
		break ;
	      case V4IM_Tag_Host:	v4im_GetPointUC(&ok,hostOrURL,UCsizeof(hostOrURL),cpt,ctx) ; break ;
	      case V4IM_Tag_Include:
		switch (cpt->PntType)
		 { default:	{ LENMAX ucBytes ; FILEID fileId = vout_PntIdToFileId(ctx,(struct V4DPI__LittlePoint *)cpt) ; UCCHAR *obuf ;
				  if (fileId == UNUSED)  { v_Msg(ctx,NULL,"OutputNoId",intmodx,V4IM_Tag_Include,cpt) ; goto socket_fail ; } ;
/*				  Convert stream data to 8-bit characters */
				  ucBytes = vout_CountForFile(fileId,UNUSED,FALSE,ctx->ErrorMsgAux) ;
				  incdata = v4mm_AllocChunk(ucBytes * 2,FALSE) ;
				  obuf = vout_GetOutputBuffer(fileId,UNUSED,ctx->ErrorMsgAux) ; if (obuf == NULL) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,argpnts[ax]) ; goto socket_fail ; } ;
				  incbytes = UCUTF16toUTF8(incdata,ucBytes*2,obuf,ucBytes) ;
				}
				break ;
		   CASEofChar	
				v4im_GetPointFileName(&ok,fileName,UCsizeof(fileName),cpt,ctx,NULL) ; if (!ok) break ;
				if (!v_UCFileOpen(&UCFile,fileName,UCFile_Open_ReadBin,TRUE,ctx->ErrorMsgAux,intmodx))
				 { v_Msg(ctx,NULL,"FileError1a",intmodx,ctx->ErrorMsgAux) ; goto socket_fail ; } ;
				fstat(UCFile.fd,&statbuf) ; incbytes = statbuf.st_size ;
				incdata = v4mm_AllocChunk(incbytes,FALSE) ; ifd = UCFile.fd ;
				bytes = read(ifd,incdata,incbytes) ;
				if (bytes != incbytes)
				  { v_Msg(ctx,ctx->ErrorMsgAux,"HTMLFileReadErr",incbytes,bytes,bytes,fileName,errno) ; ok = FALSE ; break ; } ;
				v_UCFileClose(&UCFile) ;
				break ;
		 } ;
//		v4im_GetPointFileName(&ok,fileName,UCsizeof(fileName),cpt,ctx,NULL) ; if (!ok) break ;
//		if (!v_UCFileOpen(&UCFile,fileName,UCFile_Open_ReadBin,TRUE,ctx->ErrorMsgAux,intmodx))
//		 { v_Msg(ctx,NULL,"FileError1a",intmodx,ctx->ErrorMsgAux) ; goto socket_fail ; } ;
//		fstat(UCFile.fd,&statbuf) ; incbytes = statbuf.st_size ;
//		incdata = v4mm_AllocChunk(incbytes,FALSE) ; ifd = UCFile.fd ;
//		bytes = read(ifd,incdata,incbytes) ;
//		if (bytes != incbytes)
//		  { v_Msg(ctx,ctx->ErrorMsgAux,"HTMLFileReadErr",incbytes,bytes,bytes,fileName,errno) ; ok = FALSE ; break ; } ;
//		v_UCFileClose(&UCFile) ;


		break ;
	      case V4IM_Tag_Message:	v4im_GetPointChar(&ok,ASCTBUF1,V4TMBufMax,cpt,ctx) ; break ;
	      case V4IM_Tag_Port:	v4im_GetPointUC(&ok,portOrSvc,UCsizeof(portOrSvc),cpt,ctx) ; break ;
	      case V4IM_Tag_Reply:	reply = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Session:
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:	v_Msg(ctx,NULL,"SocketSesVal",intmodx,cpt,V4IM_Tag_Session) ; goto socket_fail ;
		   case _Open:	if (vss == NULL) { vss = (struct vsocket__Session *)v4mm_AllocChunk(sizeof *vss,TRUE) ; } ;
				vss->Open = TRUE ; vss->EchoFlag = UNUSED ; vss->Result[vss->ResBytes=0] = '\0' ; break ;
		   case _Close:	if (vss != NULL ? vss->Open : FALSE) { vss->Open = FALSE ; } else { v_Msg(ctx,NULL,"SocketSesClose",intmodx) ; goto socket_fail ; } ; break ;
		 } ;
		break ;
	      case V4IM_Tag_Text:	isText = v4im_GetPointLog(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Until:	if (cpt->PntType == V4DPI_PntType_Int) { untilchar = cpt->Value.IntVal ; }
					 else { v4im_GetPointChar(&ok,untilstr,sizeof untilstr,cpt,ctx) ;
						if (strlen(untilstr) == 1) { untilchar = untilstr[0] ; ZS(untilstr) ; } ;
					      } ;
					break ;
	      case V4IM_Tag_URL:	v4im_GetPointUC(&ok,hostOrURL,UCsizeof(hostOrURL),cpt,ctx) ; break ;
	      case V4IM_Tag_Wait:	initialWait = v4im_GetPointInt(&ok,cpt,ctx) ; if (!ok) break ;
					if (initialWait < 1 || initialWait > (24 * 3600)) { v_Msg(ctx,NULL,"ModArgRange",intmodx,ax,cpt,1,(24*3600)) ; goto socket_fail ; } ;
					break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax-1) ; goto socket_fail ; } ;

	if (vss != NULL ? vss->Open != UNUSED : FALSE)
	 { 
	   if (vss->EchoFlag == UNUSED) vss->EchoFlag = echoflag ;
	   echoflag = vss->EchoFlag ;
	   reply = (vss->Open != UNUSED) ;		/* Want to get reply if have a session */
	   clsdone = (vss->Open == FALSE) ;		/* Want to close socket only on session close */
	 } ;

	if (haveSocket)
	 { if (UCnotempty(hostOrURL) || UCnotempty(portOrSvc))
	    { v_Msg(ctx,NULL,"SocketCurOpen",intmodx,V4IM_Tag_Host,V4IM_Tag_Port) ; goto socket_fail ; } ;
	 } else if (UCempty(hostOrURL) || UCempty(portOrSvc))
		 { v_Msg(ctx,NULL,"SocketNoneOpen",intmodx,V4IM_Tag_Host,V4IM_Tag_Port) ; goto socket_fail ; } ;

	SOCKETINIT
	if (!haveSocket)
	 { 
	   if (!v_ipLookup(ctx,intmodx,hostOrURL,portOrSvc,SOCK_STREAM,V_IPLookup_Connect,&socket,UCTBUF1,NULL)) goto socket_fail ;
	   haveSocket = TRUE ;
	 } ;
	i = 1 ;
	if (NETIOCTL(socket,FIONBIO,&i) != 0) { v_Msg(ctx,NULL,"SocketNoBlock",intmodx) ; goto socket_fail ; } ;

	len = strlen(ASCTBUF1) ;
	chunk = (len < 4096 ? len : 4096) ;		/* Send message in pieces */
	for(sent=0,res=1,tries=0;tries<100 && res>0&&sent<len;)
	 { res = send(socket,ASCTBUF1+sent,chunk,0) ; if (res > 0) sent+=res ; 
	   if (res <= 0 && NETERROR == EWOULDBLOCK) { HANGLOOSE(100) ; tries++ ; res = 1 ; continue ; } ;
	   if (chunk > len-sent) chunk = len-sent ; tries = 0 ;
	 } ;
	if (echoflag & vSocketEcho_Send)
	 { v_Msg(ctx,UCTBUF2,"*SocketTraceSnd",ASCTBUF1) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ; } ;
	if (res <= 0 || tries >= 100) { v_Msg(ctx,NULL,"SocketSendErr",intmodx,NETERROR) ; goto socket_fail ; } ;
/*	Send Include::xxx file if we have one */
	len = incbytes ;
	chunk = (len < 4096 ? len : 4096) ;		/* Send message in pieces */
	for(sent=0,res=1,tries=0;tries<100 && res>0&&sent<len;)
	 { res = send(socket,&incdata[sent],chunk,0) ; if (res > 0) sent+=res ; 
	   if (res <= 0 && NETERROR == EWOULDBLOCK) { HANGLOOSE(100) ; tries++ ; res = 1 ; continue ; } ;
	   if (chunk > len-sent) chunk = len-sent ; tries = 0 ;
	 } ; if (len > 0) v4mm_FreeChunk(incdata) ;
	if (res <= 0 || tries >= 100) { v_Msg(ctx,NULL,"SocketSendErr",intmodx,NETERROR) ; goto socket_fail ; } ;

	if (!reply)
	 { if (clsdone) { SOCKETCLOSE(socket) ; haveSocket = FALSE ; } ;
	   return((P *)&Log_True) ;
	 } ;

/*	Now get reply */
	if (ofpt != NULL)
	 { if (!v_UCFileOpen(&UCFile,outFile,(isText ? UCFile_Open_Write : UCFile_Open_WriteBin),TRUE,ctx->ErrorMsgAux,intmodx))
	    { v_Msg(ctx,NULL,"ModInvArg2",intmodx,ofpt) ; goto socket_fail ; } ;
	 } ;
	FD_ZERO(&sset) ; FD_SET(socket,&sset) ; FD_ZERO(&eset) ; FD_SET(socket,&eset) ;
	if (bytes == UNUSED) bytes = (ofpt == NULL ? sizeof(ASCTBUF1) : 10000000) ;
	ZS(ASCTBUF1) ;
	if (vss != NULL ? vss->Open != UNUSED : FALSE)			/* If close session then just wait a tad */
	 { wait = -250000 ; wait2 = -100000 ;
	 } else { wait = 30 ; wait2 = 1 ; } ;
	for(i=0,cnt=0;cnt<bytes;wait=wait2,i++,cnt++)
	 { if (wait > 0) { tev.tv_sec = wait ; tev.tv_usec = 0 ; }
	    else { tev.tv_sec = 0 ; tev.tv_usec = -wait ; } ;
	   res = select(FD_SETSIZE,&sset,NULL,&eset,&tev) ;		/* Wait for something to happen */
	   if (res <= 0)
	    { if (vss != NULL ? vss->Open != UNUSED : FALSE) break ;	/* Closing session - don't error on end-of-input */
	      v_Msg(ctx,NULL,"SocketTimeOut",intmodx,wait,i) ; goto socket_fail ;
	    } ;
	   res = recv(socket,&ASCTBUF1[i],1,0) ; ASCTBUF1[i+1] = '\0' ; 
	   if (res <= 0)
	    { if (res == 0) break ;
	      v_Msg(ctx,NULL,"SocketRcvErr",intmodx,NETERROR) ; goto socket_fail ;
	    } ;
	   if (untilstr[0] != 0)
	    { if (strstr(ASCTBUF1,untilstr) != NULL) break ;
	    } else { if (ASCTBUF1[i] == untilchar) break ; } ;
	   if (ofpt != NULL)
	    { if (isText)	/* If writing text then block by line */
	       { if (ofpt != NULL && ASCTBUF1[i] == '\r') { i-- ; continue ; } ;	/* If writing to file then ignore CR */
		 if (ofpt != NULL && ASCTBUF1[i] == '\n')
		  { if (i > 0) { ASCTBUF1[i] = '\0' ; fprintf(UCFile.fp,"%s\n",ASCTBUF1) ; } ;
		    i = -1 ;
		  } ;
	       } else		/* Else we are outputting binary data as-is */
	       { fputc(ASCTBUF1[i],UCFile.fp) ; i = -1 ;
	       } ;
	    } ;
	 } ;
	if (ofpt != NULL)			/* If writing output file then close it off */
	 { if (i > 0) { fprintf(UCFile.fp,"%s\n",ASCTBUF1) ; } ;
	   v_UCFileClose(&UCFile) ;
	 } ;
	if (vss != NULL ? vss->Open != UNUSED : FALSE)
	 { if (i + vss->ResBytes < sizeof vss->Result)
	    { memcpy(&vss->Result[vss->ResBytes],ASCTBUF1,i) ; vss->ResBytes += i ; } ;
	 } ;
	if ((vss != NULL ? vss->Open : TRUE) && ofpt == NULL)	/* Don't return string if writing to output file */
	 { LENMAX len=strlen(ASCTBUF1) ;
	   if (len < V4DPI_AlphaVal_Max-1) { alphaPNTv(respnt,ASCTBUF1) ; }
	    else { struct V4LEX__BigText bt ;
		   if (len > V4LEX_BigText_Max) { v_Msg(ctx,NULL,"BigTextMaxLen",intmodx,len,V4LEX_BigText_Max,V4DPI_PntType_BigText) ; goto socket_fail ; } ;
		   UCstrcpyAtoU(bt.BigBuf,ASCTBUF1) ;
		   if (!v4dpi_SaveBigTextPoint(ctx,&bt,strlen(ASCTBUF1),respnt,Dim_Alpha,TRUE)) { v_Msg(ctx,NULL,"BigTextNoVal",intmodx) ; goto socket_fail ; } ;
		 } ;
	 } else
	 { memcpy(respnt,&Log_True,Log_True.Bytes) ;
	 } ;
	if (clsdone) { SOCKETCLOSE(socket) ; haveSocket = FALSE ; } ;
	if (vss != NULL ? vss->Open == FALSE : FALSE) vss->Open = UNUSED ;	/* Close session */
	if (echoflag & vSocketEcho_Rcv)
	 { v_Msg(ctx,UCTBUF2,"*SocketTraceRcv",ASCTBUF1) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;} ;

	return(respnt) ;

socket_fail:
	if (haveSocket) { SOCKETCLOSE(socket) ; haveSocket = FALSE ; } ;
	REGISTER_ERROR(0) ; RETURNFAILURE ;
}

/*	X D B  -  E X T E R N A L   D A T A B A S E   A C C E S S		*/

/*	v4xdb_VDTYPEtoDim - Converts VDTYPE to default dimension (UNUSED if invalid vdtype) */

DIMID v4xdb_VDTYPEtoDim(vdtype)
  ETYPE vdtype ;
{
	switch (vdtype)
	 {
	   case VDTYPE_b1SInt:
	   case VDTYPE_b2SInt:
	   case VDTYPE_b4SInt:		return(Dim_Int) ;
	   case VDTYPE_b8SInt:		return(Dim_UFix) ;
	   case VDTYPE_b4FP:
	   case VDTYPE_b8FP:		return(Dim_Num) ;
	   case VDTYPE_Char:
	   case VDTYPE_UCChar:
	   case VDTYPE_UTF8:		return(Dim_Alpha) ;
	   case VDTYPE_ODBC_DS:		return(Dim_UDate) ;
	   case VDTYPE_ODBC_TS:		return(Dim_UTime) ;
	   case VDTYPE_ODBC_TSS:	return(Dim_UDT) ;
	   case VDTYPE_ODBC_SNS:	return(Dim_UFix) ;
	   case VDTYPE_MYSQL_TS:	return(Dim_UDT) ;
	   case VDTYPE_ORACLE_TS:	return(Dim_UDT) ;
	 } ;
	return(UNUSED) ;
}

#ifdef WANTORACLE
void v4oracle_ErrorHandler(err)
 OCI_Error *err ;
{ if (gpi->xdb == NULL) return ;
  gpi->xdb->con[gpi->xdb->lastCX].lastErrCode = OCI_ErrorGetOCICode(err) ;
  UCstrcpyAtoU(gpi->xdb->con[gpi->xdb->lastCX].lastErrMsg,OCI_ErrorGetString(err)) ;
}
#endif

P *v4im_DodbXct(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt,*argpnts[] ;
  int argcnt,intmodx,trace ;
{ P *cpt, *ipt, argPt ;
  XDBID xdbId = 0 ; LOGICAL autoClose ;
  enum DictionaryEntries retval ;
  INDEX ix,cx ; LOGICAL ok ; UCCHAR tb[V4TMBufMax] ;

	if (gpi->RestrictionMap & V_Restrict_XDBXCT) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; return(NULL) ; } ;
	if (gpi->xdb == NULL ? TRUE : gpi->xdb->conCount == 0)
	 { v_Msg(ctx,NULL,"XDBNoEnv",intmodx) ; return(NULL) ; } ;
	autoClose = FALSE ; ZUS(tb) ; retval = 0 ; cx = UNUSED ;
	for(ix=1,ok=TRUE;ix<=argcnt;ix++)
	 { switch (v4im_CheckPtArgNew(ctx,argpnts[ix],&cpt,&argPt))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; return(NULL) ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; return(NULL) ;
	      case V4IM_Tag_Dim:	for(cx=0;cx<gpi->xdb->conCount;cx++)
					 { if (gpi->xdb->con[cx].DimId == cpt->Value.IntVal && gpi->xdb->con[cx].xdbId != UNUSED) break ; } ;
					if (cx >= gpi->xdb->conCount)
					 { v_Msg(ctx,NULL,"XDBDimBad",intmodx,ix,cpt) ; return(NULL) ; } ;
					xdbId = gpi->xdb->con[cx].xdbId ; break ;
	      case -V4IM_Tag_Close:	autoClose = TRUE ; break ;
	      case V4IM_Tag_Connection:	for(cx=0;cx<gpi->xdb->conCount;cx++)
					 { if (gpi->xdb->con[cx].xdbId != UNUSED && memcmp(cpt,&gpi->xdb->con[cx].idPnt,gpi->xdb->con[cx].idPnt.Bytes) == 0) break ; } ;
					if (cx >= gpi->xdb->conCount)
					 { v_Msg(ctx,NULL,"XDBIdBad",intmodx,cpt) ; return(NULL) ; } ;
					xdbId = gpi->xdb->con[cx].xdbId ; break ;
	      case -V4IM_Tag_Key:	retval = DE(Key) ; break ;
	      case -V4IM_Tag_Rows:	retval = DE(Rows) ; break ;
	      case V4IM_Tag_SQL:	v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; break ;
	    } ;
	   if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ix) ; return(NULL) ; } ;
	 } ;
	if (cx == UNUSED)
	 { v_Msg(ctx,NULL,"XDBNoDBSel",intmodx) ; return(NULL) ; } ;
	switch (XDBGETDBACCESS(xdbId))
	 {
#ifdef WANTODBC
	   case XDBODBC:	ipt = v4odbc_Xct(ctx,respnt,autoClose,xdbId,tb,retval,intmodx,trace) ; break ;
#endif
#ifdef WANTV4IS
/*				Force autoClose on MIDAS SQL because MIDAS side does not stay open */
	   case XDBV4IS:	ipt = v4odbc_MIDASXct(ctx,respnt,TRUE,xdbId,tb,retval,intmodx,trace) ; break ;

#endif
#ifdef WANTMYSQL
	   case XDBMYSQL:	ipt = v4mysql_Xct(ctx,respnt,autoClose,xdbId,tb,retval,intmodx,trace,(gpi->xdb->con[cx].resetCon ? 4 : 0)) ; break ;
#endif
#ifdef WANTORACLE
	   case XDBORACLE:	ipt = v4oracle_Xct(ctx,respnt,autoClose,xdbId,tb,retval,intmodx,trace,(gpi->xdb->con[cx].resetCon ? 4 : 0)) ; break ;
#endif
	 } ;
	
	return(ipt) ;
}


/*	v4im_DodbGet - Handles xdbGet() Module		*/

P *v4im_DodbGet(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt,*argpnts[] ;
  int argcnt,intmodx,trace ;
{ 
  struct V4DPI__DimInfo *di ;
  struct V4L__ListPoint *lp ;
  struct V4L__ListXDBGet *log ;
  struct V4XDB__Master *xdb ;
  struct V4DPI__LittlePoint idPnt, idPntStmt ;
  P *cpt, argPnt ;
  INDEX cx,sx,res,ix,valCol ; DIMID dimId=UNUSED, targetDimId=UNUSED ; UCCHAR tb[V4TMBufMax] ; ETYPE cacheCur=UNUSED ;
  XDBID xdbId = 0 ; P *eofPnt = NULL ; P *fetchPnt = NULL ; P *bosPnt = NULL ; LOGICAL relConEOF=FALSE, ok, listOf=FALSE, wantList=FALSE ;
  enum { rows, tableInfo } getType ;

	if (gpi->RestrictionMap & V_Restrict_XDBAccess) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; return(NULL) ; } ;
	if (gpi->xdb == NULL ? TRUE : gpi->xdb->conCount == 0)
	 { v_Msg(ctx,NULL,"XDBNoEnv",intmodx) ; return(NULL) ; } ;
	xdb = gpi->xdb ;
	ZUS(tb) ; getType = rows ; ZUS(UCTBUF1) ; res = V4IM_Tag_ListOf ; cx = UNUSED ; sx  = UNUSED ; valCol = UNUSED ; ZPH(&idPnt) ; ZPH(&idPntStmt) ;
	for(ix=1,ok=TRUE;ok&&ix<=argcnt;ix++)
	 { switch (v4im_CheckPtArgNew(ctx,argpnts[ix],&cpt,&argPnt))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; return(NULL) ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; return(NULL) ;
	      case V4IM_Tag_Begin:	bosPnt = cpt ; break ;
	      case V4IM_Tag_Cache:	cacheCur = v4im_GetPointLog(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Close:	relConEOF = v4im_GetPointLog(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Connection:	for(cx=0;cx<gpi->xdb->conCount;cx++)
					 { if (gpi->xdb->con[cx].xdbId != UNUSED && memcmp(cpt,&gpi->xdb->con[cx].idPnt,gpi->xdb->con[cx].idPnt.Bytes) == 0) break ; } ;
					if (cx >= gpi->xdb->conCount) { v_Msg(ctx,NULL,"XDBDimBad",intmodx,ix,cpt) ; return(NULL) ; } ;
					break ;
	      case V4IM_Tag_Dim:	dimId = cpt->Value.IntVal ; break ;
	      case V4IM_Tag_Do:		fetchPnt = cpt ; break ;
	      case V4IM_Tag_End:	eofPnt = cpt ; break ;
	      case V4IM_Tag_Id:		if (cpt->PntType == V4DPI_PntType_Int) { v_Msg(ctx,NULL,"XDBBadId",intmodx,V4IM_Tag_Id,V4DPI_PntType_Int,intmodx) ; return(NULL) ; } ;
					if (cpt->Bytes > sizeof gpi->xdb->stmts[0].idPnt) { v_Msg(ctx,NULL,"DimTooBig",intmodx,cpt) ; return(NULL) ; } ;
					memcpy(&idPnt,cpt,cpt->Bytes) ; break ;
	      case V4IM_Tag_ListOf:	if (cpt->PntType == V4DPI_PntType_Logical) { wantList = v4im_GetPointLog(&ok,cpt,ctx) ; break ; }
					switch (v4im_GetDictToEnumVal(ctx,cpt))
					 { default:		v_Msg(ctx,NULL,"ModTagValue",intmodx,2,V4IM_Tag_ListOf,cpt) ; return(NULL) ;
					   case DE(Ids):	listOf = 1 ; break ;
					   case DE(Rows):	listOf = 2 ; break ;
					   case DE(Save):	wantList = TRUE ; break ;
					 } ;
					break ;
	      case -V4IM_Tag_ListOf:	wantList = TRUE ; break ;
	      case -V4IM_Tag_Num:	res = V4IM_Tag_Num ; break ;
	      case V4IM_Tag_SQL:	v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; break ;
	      case V4IM_Tag_Statement:	if (cpt->Bytes > sizeof idPntStmt) { v_Msg(ctx,NULL,"DimTooBig",intmodx,cpt) ; return(NULL) ; } ;
					memcpy(&idPntStmt,cpt,cpt->Bytes) ; break ;
	      case V4IM_Tag_Table:	getType = tableInfo ; v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; break ;
	      case -V4IM_Tag_Table:	getType = tableInfo ; ZUS(tb) ; break ;
	      case V4IM_Tag_Target:	targetDimId = v4dpi_PointToDimId(ctx,cpt,&di,intmodx,ix) ; break ;
	      case V4IM_Tag_Type:	getType = tableInfo ; v4im_GetPointUC(&ok,UCTBUF1,UCsizeof(tb),cpt,ctx) ; break ;
	      case -V4IM_Tag_Unique:	res = V4IM_Tag_Unique ; break ;
	      case V4IM_Tag_Value:	{ INDEX tx = v4im_GetPointInt(&ok,cpt,ctx) ; if (tx > 0) valCol = tx ; }	/* Only update if > 0 */
					break ;
	       } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ix-1) ; return(NULL) ; } ;
	if (wantList && (res == V4IM_Tag_Unique)) { v_Msg(ctx,NULL,"ModOnlyOne",intmodx,-V4IM_Tag_Unique,-V4IM_Tag_ListOf) ; return(NULL) ; } ;
/*	We may not be doing a new 'get', but just returning something related to a prior one... let's see */
	if ((dimId != UNUSED || idPntStmt.Bytes > 0) && cacheCur != UNUSED)	/* Want to explicity save (or not) the current record */
	 { struct V4XDB__SaveRowInfo *vxsri = xdb->vxsri ; INDEX i ;
	   if (vxsri == NULL) { v_Msg(ctx,NULL,"XDBDimNotAct",intmodx,dimId,V4IM_Tag_Target) ; return(NULL) ; } ;
	   for(;!GETSPINLOCK(&vxsri->sriLock);) {} ;
	   for(i=0;i<V4XDB_SaveInfo_DimMax;i++) { if (vxsri->dimList[i].dimId == dimId || (idPntStmt.Bytes > 0 ? memcmp(&vxsri->dimList[i].idPnt,&idPntStmt,idPntStmt.Bytes) == 0 : FALSE)) break ; } ;
	   if (i >= V4XDB_SaveInfo_DimMax) { v_Msg(ctx,NULL,"XDBDimNotAct",intmodx,dimId,V4IM_Tag_Target) ; return(NULL) ; } ;
	   vxsri->dimList[i].explicitSave = cacheCur ;
	   RLSSPINLOCK(vxsri->sriLock) ;
	   logPNTv(respnt,cacheCur) ; return(respnt) ;
	 } ;
	if (listOf == 1 && (dimId != UNUSED || idPntStmt.Bytes > 0))	/* Wants list of statement Ids related to this dimension */
	 { struct V4XDB__SaveStmtInfo *vxssi ;
	   INITLP(respnt,lp,Dim_List) ;
	   for(vxssi=xdb->vxssi;vxssi!=NULL;vxssi=vxssi->vxssiNext)
	    { if (vxssi->dimId == dimId || (idPntStmt.Bytes > 0 ? memcmp(&vxssi->idPnt,&idPntStmt,idPntStmt.Bytes) == 0 : FALSE))
	       { struct V4DPI__LittlePoint lpnt ; intPNTv((P *)&lpnt,vxssi->xdbId) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,(P *)&lpnt,0) ; } ;
	    } ;
	   ENDLP(respnt,lp) ;
	   return(respnt) ;
	 } ;
	if (listOf == 2 && idPntStmt.Bytes > 0)		/* Wants list of db points associated with statement id/xdbId */
	 { 
	   struct V4L__ListCmpndRange *lcr ;
	   struct V4XDB__SaveStmtInfo *vxssi ;
	   for(vxssi=xdb->vxssi;vxssi!=NULL;vxssi=vxssi->vxssiNext)	/* If idPntStmt is integer then it's an xdbId, otherwise it's just an id */
	    { if (idPntStmt.PntType == V4DPI_PntType_Int ? vxssi->xdbId == idPntStmt.Value.IntVal : memcmp(&vxssi->idPnt,&idPntStmt,idPntStmt.Bytes) == 0) break ; } ;
	   if (vxssi == NULL) { v_Msg(ctx,NULL,"XDBNoStmt",intmodx,V4IM_OpCode_dbGet,V4IM_Tag_Statement,&idPntStmt) ; return(NULL) ; } ;
	   if (vxssi->lpnt != NULL)
	    { memcpy(respnt,vxssi->lpnt,vxssi->lpnt->Bytes) ; return(respnt) ; } ;	/* Return list of points */
/*	   Don't have explicit list, return list corresponding to points between firstRecId & lastRecId */
	   INITLP(respnt,lp,Dim_List) lp->ListType = V4L_ListType_CmpndRange ;
	   lcr = (struct V4L__ListCmpndRange *)&lp->Buffer[0] ; lcr->Entries = 0 ; lcr->Count = 0 ;
	   respnt->PntType = V4DPI_PntType_List ; respnt->Dim = Dim_List ;
	   lcr->Cmpnd[lcr->Count].Begin = vxssi->firstRecId ; lcr->Cmpnd[lcr->Count].End = vxssi->lastRecId ;
	   lcr->Cmpnd[lcr->Count++].Increment = 1 ;
	   lcr->Entries += (1 + vxssi->lastRecId - vxssi->firstRecId) ;
	   lcr->Bytes = (char *)&lcr->Cmpnd[lcr->Count] - (char *)lcr ;
	   lp->Bytes = (char *)&lp->Buffer[ALIGN(lcr->Bytes)] - (char *)lp ;
	   ENDLP(respnt,lp) lp->Entries = lcr->Entries ; lp->Dim = vxssi->dimId ; lp->PntType = V4DPI_PntType_Int ;
	   return(respnt) ;
	 } ;
/*	Looks like we are going to do a 'get', lookup connection based on dimension given */
	if (cx == UNUSED)
	 { for(cx=0;cx<gpi->xdb->conCount;cx++)
	    { if (gpi->xdb->con[cx].DimId == dimId && gpi->xdb->con[cx].xdbId != UNUSED) break ; } ;
	   if (cx >= gpi->xdb->conCount) { v_Msg(ctx,NULL,"XDBDimBad",intmodx,ix,cpt) ; return(NULL) ; } ;
	 } ;
	xdbId = gpi->xdb->con[cx].xdbId ;
	if (xdbId == 0) { v_Msg(ctx,NULL,"XDBNoDBSel",intmodx) ; return(NULL) ; } ;
	if (valCol != UNUSED && targetDimId == UNUSED) { v_Msg(ctx,NULL,"XDBValCol",intmodx,V4IM_Tag_Value,V4IM_Tag_Target) ; return(NULL) ; } ;
	if (wantList && targetDimId == UNUSED) { v_Msg(ctx,NULL,"XDBValCol",V4IM_Tag_ListOf,V4IM_Tag_Target) ; return(NULL) ; } ;

switch_type:
	switch (getType)
	 { case rows:
		switch (XDBGETDBACCESS(xdbId))
		 {
#ifdef WANTODBC
		   case XDBODBC:	sx = v4odbc_NewStmt(ctx,xdbId,tb,intmodx,trace) ; break ;
#endif
#ifdef WANTV4IS
		   case XDBV4IS:	sx = v4odbc_MIDASNewStmt(ctx,xdbId,tb,intmodx,trace) ; break ;
#endif
#ifdef WANTMYSQL
		   case XDBMYSQL:	sx = v4mysql_NewStmt(ctx,xdbId,tb,intmodx,trace,(gpi->xdb->con[cx].resetCon ? 4 : 0)) ; break ;
#endif
#ifdef WANTORACLE
		   case XDBORACLE:	sx = v4oracle_NewStmt(ctx,xdbId,tb,intmodx,trace,(gpi->xdb->con[cx].resetCon ? 4 : 0)) ; break ;
#endif
		 } ;

		if (sx == UNUSED)
		 { return(NULL) ; } ;
		xdb->stmts[sx].targetDimId = targetDimId ;
		if (targetDimId != UNUSED)				/* If got target dimension then get starting recId from dimension recId */
		 { struct V4DPI__DimInfo *di ; struct V4XDB__SaveStmtInfo vxssi,*vxssiPtr ; INDEX col,vx ;
		   struct V4XDB__SaveRowInfo *vxsri ;
//VEH110907
		   v4xdb_FreeSavedInfo(targetDimId) ;			/* Free up any prior rows saved that are associated with this dimension */
		   DIMINFO(di,ctx,targetDimId) ;
		   xdb->stmts[sx].curRecId = di->XDB.lastRecId ;
/*		   Adds DIM to list of dimensions to watch (for saving row) */
		   if (gpi->xdb->vxsri == NULL) { gpi->xdb->vxsri = (struct V4XDB__SaveRowInfo *)v4mm_AllocChunk(sizeof *gpi->xdb->vxsri,TRUE) ; gpi->xdb->vxsri->sriLock = UNUSEDSPINLOCKVAL ; } ; \
		   vxsri = gpi->xdb->vxsri ;
		   for(;!GETSPINLOCK(&vxsri->sriLock);) {} ;
		   for(vx=0;vx<vxsri->dimCount;vx++) { if (vxsri->dimList[vx].dimId == xdb->stmts[sx].dimId) break ; } ;
		   if (vx >= vxsri->dimCount && vx < V4XDB_SaveInfo_DimMax)
		    { vxsri->dimList[vx].dimId = xdb->stmts[sx].targetDimId ; if (idPnt.Bytes > 0) memcpy(&vxsri->dimList[vx].idPnt,&idPnt,idPnt.Bytes) ;
		      vxsri->dimList[vx].saveRow = FALSE ; vxsri->dimList[vx].explicitSave = UNUSED ; vxsri->dimList[vx].saveAllRows = cacheCur ;
		      vxsri->dimHash[(xdb->stmts[sx].targetDimId) & (V4XDB_SaveInfo_HashMax - 1)] = 1 ; vxsri->dimCount++ ;
		    } ;
		   RLSSPINLOCK(vxsri->sriLock) ;
		   memset(&vxssi,0,sizeof vxssi) ;
		   vxssi.xdbId = xdb->stmts[sx].xdbId ; vxssi.colCount = xdb->stmts[sx].colCount ; vxssi.dimId = xdb->stmts[sx].targetDimId ;
		   vxssi.firstRecId = xdb->stmts[sx].curRecId + 1 ; if (idPnt.Bytes > 0) memcpy(&vxssi.idPnt,&idPnt,idPnt.Bytes) ;
		   if (valCol <= 0) { vxssi.valCol = UNUSED ; }
		    else { if (valCol > xdb->stmts[sx].colCount) valCol = xdb->stmts[sx].colCount ;
			   vxssi.valCol = valCol - 1 ;			/* Remember - we start with 0 offset! */
			 } ;
		   if (wantList)
		    { vxssi.lpnt = (P *)v4mm_AllocChunk(sizeof(P),FALSE) ; INITLP(vxssi.lpnt,vxssi.lp,Dim_List) ;
		    } ;
		   for(col=0;col<vxssi.colCount;col++)
		    { vxssi.col[col].vdType = xdb->stmts[sx].col[col].vdType ; } ;
		   vxssiPtr = (struct V4XDB__SaveStmtInfo *)v4mm_AllocChunk(SIZEOFVXSSI(&vxssi),FALSE) ;
		   memcpy(vxssiPtr,&vxssi,SIZEOFVXSSI(&vxssi)) ;
		   vxssiPtr->vxssiNext = xdb->vxssi  ; xdb->vxssi = vxssiPtr ;
		   xdb->stmts[sx].vxssi = vxssiPtr ;
		 } ;
		if (idPnt.Bytes > 0) { memcpy(&xdb->stmts[sx].idPnt,&idPnt,idPnt.Bytes) ; } ;
		xdb->stmts[sx].relConEOF = relConEOF ;
		if (eofPnt != NULL)
		 { xdb->stmts[sx].eofPnt = v4mm_AllocChunk(eofPnt->Bytes,FALSE) ; memcpy(xdb->stmts[sx].eofPnt,eofPnt,eofPnt->Bytes) ;
		 } ;
		if (fetchPnt != NULL)
		 { xdb->stmts[sx].fetchPnt = v4mm_AllocChunk(fetchPnt->Bytes,FALSE) ; memcpy(xdb->stmts[sx].fetchPnt,fetchPnt,fetchPnt->Bytes) ;
		 } ;
/*		Have a begin-of-statement point to evaluate ? */
		if (bosPnt != NULL)
		 { P *ipt = v4dpi_IsctEval(respnt,bosPnt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (ipt == NULL) { v_Msg(ctx,NULL,"XDBBOSPnt",intmodx,V4IM_Tag_Begin,bosPnt) ; return(NULL) ; } ;
		 } ;
		switch(res)
		 { case V4IM_Tag_Num:
			gpi->xdb->con[cx].isActive = TRUE ;			/* Flag connection as active */			
			intPNTv(respnt,xdb->stmts[sx].rowCnt) ; break ;
		   case V4IM_Tag_Unique:
		   case V4IM_Tag_ListOf:
			lp = ALIGNLP(&respnt->Value) ;	/* Link up list structure */
			memset(lp,0,V4L_ListPointHdr_Bytes) ; lp->ListType = V4L_ListType_XDBGet ; lp->PntType = V4DPI_PntType_XDB ;
			lp->Dim = xdb->con[cx].DimId ;
			log = (struct V4L__ListXDBGet *)&lp->Buffer[0] ; log->xdbId = xdb->stmts[sx].xdbId ;
			respnt->PntType = V4DPI_PntType_List ; respnt->Dim = Dim_List ;
			lp->Entries = -1 ; lp->Bytes = (char *)&lp->Buffer[ALIGN(sizeof *log)] - (char *)lp ;
			respnt->Bytes = (char *)&respnt->Value.AlphaVal[lp->Bytes] - (char *)respnt ;
			if (respnt->Bytes < V4DPI_PointHdr_Bytes + ((char *)&lp->Buffer[0] - (char *)lp))
			 respnt->Bytes = V4DPI_PointHdr_Bytes + ((char *)&lp->Buffer[0] - (char *)lp) ;	/* Make sure min size */
/*			If want unique record then don't return this list */
			if (res == V4IM_Tag_ListOf)
			 { gpi->xdb->con[cx].isActive = TRUE ;			/* Flag connection as active */		
			   return(respnt) ;
			 } ;
/*			Get first point in this list & return it (but make sure no other points in the list) */
			{ P upnt ;
			  if (v4l_ListPoint_Value(ctx,lp,1,&upnt) <= 0)
			   { v_Msg(ctx,NULL,"XDBNoRows",intmodx,tb) ; return(NULL) ; } ;
			  if (v4l_ListPoint_Value(ctx,lp,2,&argPnt) > 0)
			   { INDEX lx ;			/* Not unique - spin through the remaining rows so we don't get a out-of-synch error on the next mySQL operation */
			     for(lx=3;;lx++) { if (v4l_ListPoint_Value(ctx,lp,lx,&argPnt) <= 0) break ; } ;
			     v_Msg(ctx,NULL,"XDBNotUnique",intmodx,tb) ; return(NULL) ;
			   } ;
			  memcpy(respnt,&upnt,upnt.Bytes) ;
			} ;
			break ;
		 } ;
		break ;
	   case tableInfo:
#if defined WANTODBC || defined WANTMYSQL || defined WANTORACLE
		switch (XDBGETDBACCESS(xdbId))
		 {
#ifdef WANTODBC
		   case XDBODBC:
			v4odbc_NewTableStmt(ctx,respnt,xdbId,tb,UCTBUF1,intmodx,trace) ; break ;
#endif
#ifdef WANTV4IS
		   case XDBV4IS:
			v_Msg(ctx,NULL,"XDBNoSup",intmodx) ; return(NULL) ;
#endif
#ifdef WANTMYSQL
		   case XDBMYSQL:
			UCstrcpy(UCTBUF1,tb) ; UCstrcpy(tb,UClit("SHOW COLUMNS FROM `")) ; UCstrcat(tb,UCTBUF1) ; UCstrcat(tb,UClit("`;")) ;
			getType = rows ; goto switch_type ;
#endif
#ifdef WANTORACLE
		   case XDBORACLE:
			UCstrcpy(UCTBUF1,tb) ; UCstrcpy(tb,UClit("SHOW COLUMNS FROM `")) ; UCstrcat(tb,UCTBUF1) ; UCstrcat(tb,UClit("`;")) ;
			getType = rows ; goto switch_type ;
#endif
		 } ;
#else
		v_Msg(ctx,NULL,"NYIonthisOS",intmodx) ; return(NULL) ;
#endif
	 } ;
	return(respnt) ;
}

P *v4im_Dodb(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt,*argpnts[] ;
  int argcnt,intmodx,trace ;
{ P *cpt,*ipt,argPnt ;
  struct V4DPI__LittlePoint imPnt ;
  struct V4L__ListPoint *lp ;
  static DIMID dimTable = UNUSED, dimAction = UNUSED ; DIMID dimArgs ;
  INDEX ix ; LOGICAL ok, gotInfo, gotList ; FRAMEID frameid ;

#ifdef USE_NEW_XDB
	if (dimTable == UNUSED) dimTable = Dim_UDBSpec ;	/* Set this as the default (once) */
	dimArgs = Dim_UDBArgs ;
#else
	if (dimTable == UNUSED) dimTable = Dim_Alpha ;		/* Set this as the default (once) */
	dimArgs = Dim_List ;
#endif

	gotInfo = FALSE ; gotList = FALSE ;
	frameid = v4ctx_FramePush(ctx,NULL) ;
/*	Note - none of the arguments have been evaluated, first add IntMod:db to context */
	ZPH((P *)&imPnt) ; imPnt.Dim = Dim_IntMod ; imPnt.PntType = V4DPI_PntType_Dict ; imPnt.Bytes = V4PS_Dict ; imPnt.Value.IntVal = intmodx ;
	if (!v4ctx_FrameAddDim(ctx,0,(P *)&imPnt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto fail ; } ;
/*	Initialize the argument list */
	INITLP(respnt,lp,dimArgs) ;
	for(ix=1,ok=TRUE;ok&&ix<=argcnt;ix++)
	 { if (argpnts[ix]->PntType != V4DPI_PntType_TagVal)
	    { ipt = v4dpi_IsctEval(&argPnt,argpnts[ix],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ; 
	      if (ipt == NULL) { v_Msg(ctx,NULL,"ModArgNewEval",intmodx,ix,argpnts[ix]) ; goto fail ; } ;
	      if (memcmp(ipt,&protoNone,protoNone.Bytes) == 0) continue ;		/* Ignore UV4:none point */
/*	      If got point on special dimensions then just enter into context, don't append to list */
	      if (ipt->Dim == dimAction || ipt->Dim == dimTable)
	       { if (!v4ctx_FrameAddDim(ctx,0,ipt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto fail ; } ;
	         gotInfo = TRUE ; continue ;
	       } ;
	      if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,ipt,0))
	       { v_Msg(ctx,NULL,"ModInvArg",intmodx,ix) ; goto fail ; } ;
	      gotList = TRUE ; continue ;
	    } ;
	   switch (v4im_CheckPtArgNew(ctx,argpnts[ix],&cpt,&argPnt))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	      case V4IM_Tag_Action:	dimAction = v4dpi_PointToDimId(ctx,cpt,NULL,intmodx,ix) ; if (dimAction == UNUSED) goto fail ; break ;
	      case V4IM_Tag_Table:	dimTable = v4dpi_PointToDimId(ctx,cpt,NULL,intmodx,ix) ; if (dimTable == UNUSED) goto fail ; break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ix-1) ; return(NULL) ; } ;
/*	Finish off list and put it into the context */
	ENDLP(respnt,lp) ;
	if (!(gotInfo || gotList)) { logPNTv(respnt,FALSE) ; return(respnt) ; } ;
	if (!v4ctx_FrameAddDim(ctx,0,respnt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto fail ; } ;
/*	Now construct isct: [UV4:db] */
	dictPNTv((P *)&imPnt,Dim_UV4,v4im_GetEnumToDictVal(ctx,DE(DB),Dim_UV4)) ;
	INITISCT(&argPnt) ; NOISCTVCD(&argPnt) ; argPnt.Grouping = 1 ; argPnt.Bytes += imPnt.Bytes ;
	ipt = ISCT1STPNT(&argPnt) ; memcpy(ipt,&imPnt,imPnt.Bytes) ;
/*	And now lets evaluate it */
	ZUS(gpi->xdbLastError) ;		/* Clear db error buffer */
	ipt = v4dpi_IsctEval(respnt,&argPnt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	if (ipt == NULL)			/* Failed - use xdb error message if we got, if not then use generic XDBEvalFail message */
	 { if (UCempty(gpi->xdbLastError)) { v_Msg(ctx,NULL,"XDBEvalFail",intmodx,DE(DB),dimTable,dimArgs) ; }
	    else { UCstrcpy(ctx->ErrorMsg,gpi->xdbLastError) ; } ;
	   goto fail ;
	 } ;
	if (!v4ctx_FramePop(ctx,frameid,NULL)) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdCtxPopId",frameid) ; goto fail ; } ;
	return(respnt) ;
fail:
	v4ctx_FramePop(ctx,frameid,NULL) ;
	return(NULL) ;
}


P *v4db_DodbConnect(ctx,respnt,intmodx,argcnt,argpnts,trace)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  int argcnt ;
  P *respnt,*argpnts[] ;
  int trace ;
{ struct V4DPI__DimInfo *di ;
  P *cpt,spnt ;
  struct V4DPI__LittlePoint idPnt ;
  int i,ok,autocom,access,commit,port ; INDEX cx ;
  UCCHAR connection[255],dsn[255],user[255],pass[255] ; LOGICAL testActive,testOpen, reset ;

	if (gpi->RestrictionMap & V_Restrict_XDBAccess) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; return(NULL) ; } ;
	di = NULL ; autocom = UNUSED ; access = UNUSED ; ZPH(&idPnt) ;
	ZS(connection) ZS(dsn) ZS(user) ZS(pass) testOpen = FALSE ; testActive = FALSE ; commit = UNUSED ; reset = FALSE ; port = UNUSED ;

	for(i=1,ok=TRUE;ok&&i<=argcnt;i++)
	 {
	   switch (v4im_CheckPtArgNew(ctx,argpnts[i],&cpt,&spnt))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:		v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	      case -V4IM_Tag_Active:	testActive = TRUE ; break ;
	      case V4IM_Tag_Access:
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:		v_Msg(ctx,NULL,"XDBAccess",intmodx,cpt) ; goto fail ;
#ifdef WANTV4IS
		   case _MIDAS:		access = 3 ; break ;
#endif
#ifdef WANTMYSQL
		   case _MySQL:		access = 2 ; break ;
#endif
#ifdef WANTORACLE
		   case _Oracle:	access = 4 ; break ;
#endif
#ifdef WANTODBC
		   case _ODBC:		access = 1 ; break ;
#endif
		 } ;
		break ;
	      case V4IM_Tag_AutoCommit:		autocom = v4im_GetPointLog(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Commit:		commit = v4im_GetPointLog(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Host:
	      case V4IM_Tag_Connection:
		if (IGNOREARG(cpt)) break ;
		v4im_GetPointUC(&ok,connection,UCsizeof(connection),cpt,ctx) ; break ;
	      case V4IM_Tag_Dim:
		ISVALIDDIM(cpt->Value.IntVal,i,"ODBCConnect()") ;
		DIMINFO(di,ctx,cpt->Value.IntVal) ;
		if (di == NULL ? TRUE : di->PointType != V4DPI_PntType_XDB)
		 { v_Msg(ctx,NULL,"XDBDimBad",intmodx,i,cpt) ; goto fail ; } ;
		break ;
	      case V4IM_Tag_DSN:
		if (IGNOREARG(cpt)) break ;
		v4im_GetPointUC(&ok,dsn,UCsizeof(dsn),cpt,ctx) ; break ;
	      case V4IM_Tag_Id:			if (cpt->Bytes > sizeof gpi->xdb->con[0].idPnt) { v_Msg(ctx,NULL,"DimTooBig",intmodx,cpt) ; goto fail ; } ;
						memcpy(&idPnt,cpt,cpt->Bytes) ; break ;
	      case -V4IM_Tag_Open:		testOpen = TRUE ; break ;
	      case V4IM_Tag_Port:		port = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_User:
		if (IGNOREARG(cpt)) break ;
		v4im_GetPointUC(&ok,user,UCsizeof(user),cpt,ctx) ; break ;
	      case V4IM_Tag_Password:
		if (IGNOREARG(cpt)) break ;
		v4im_GetPointUC(&ok,pass,UCsizeof(pass),cpt,ctx) ; break ;
	      case -V4IM_Tag_Reset:
		reset = TRUE ; break ;
	    } ; if (!ok) break ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ; } ;
	if (di == NULL) { DIMINFO(di,ctx,Dim_UDB) ; } ;
	if (testOpen)
	 { if (idPnt.Bytes == 0) { v_Msg(ctx,NULL,"TagMissing",intmodx,V4IM_Tag_Id) ; goto fail ; } ;
	   for(cx=0;cx<gpi->xdb->conCount;cx++)
	    { if (gpi->xdb->con[cx].xdbId != UNUSED && memcmp(&idPnt,&gpi->xdb->con[cx].idPnt,gpi->xdb->con[cx].idPnt.Bytes) == 0) break ; } ;
	   if (cx >= gpi->xdb->conCount)
	    { v_Msg(ctx,NULL,"XDBIdBad",intmodx,&idPnt) ; return(NULL) ; } ;
	   logPNTv(respnt,TRUE) ; return(respnt) ;
	 } ;
	if (testActive)
	 { if (idPnt.Bytes == 0) { v_Msg(ctx,NULL,"TagMissing",intmodx,V4IM_Tag_Id) ; goto fail ; } ;
	   for(cx=0;cx<gpi->xdb->conCount;cx++)
	    { if (gpi->xdb->con[cx].xdbId != UNUSED && memcmp(&idPnt,&gpi->xdb->con[cx].idPnt,gpi->xdb->con[cx].idPnt.Bytes) == 0) break ; } ;
	   if (cx >= gpi->xdb->conCount)
	    { v_Msg(ctx,NULL,"XDBIdBad",intmodx,&idPnt) ; return(NULL) ; } ;
	   logPNTv(respnt,gpi->xdb->con[cx].isActive) ; return(respnt) ;
	 } ;
/*	Are we tweaking an existing connection, or creating a new one? */
	if (idPnt.Bytes != 0 && UCempty(dsn))		/* If got an Id then assume it's an existing connection */
	 { for(cx=0;cx<gpi->xdb->conCount;cx++)
	    { if (gpi->xdb->con[cx].xdbId != UNUSED && memcmp(&idPnt,&gpi->xdb->con[cx].idPnt,gpi->xdb->con[cx].idPnt.Bytes) == 0) break ; } ;
	   if (cx >= gpi->xdb->conCount)
	    { v_Msg(ctx,NULL,"XDBIdBad",intmodx,&idPnt) ; return(NULL) ; } ;
	   if (reset)
	    { gpi->xdb->con[cx].resetCon = TRUE ; logPNTv(respnt,TRUE) ; return(respnt) ; } ;
	   if (autocom != UNUSED)
	    {
	      switch (XDBGETDBACCESS(gpi->xdb->con[cx].xdbId))
	       {
#ifdef WANTODBC
		 case XDBODBC:
			v_Msg(ctx,NULL,"XDBNoSup",intmodx) ; return(NULL) ;
#endif
#ifdef WANTV4IS
		 case XDBV4IS:
			v_Msg(ctx,NULL,"XDBNoSup",intmodx) ; return(NULL) ;
#endif
#ifdef WANTMYSQL
		 case XDBMYSQL:
			mysql_autocommit(gpi->xdb->con[cx].mysql,FALSE) ;
			logPNTv(respnt,TRUE) ; return(respnt) ;
#endif
#ifdef WANTORACLE
		 case XDBORACLE:
			OCI_SetAutoCommit(gpi->xdb->con[cx].oracle,FALSE) ;
			logPNTv(respnt,TRUE) ; return(respnt) ;
#endif
		 } ;
	    } ;
#ifdef WANTORACLE
	   gpi->xdb->lastCX = cx ;
#endif
	   if (commit != UNUSED)
	    {
	      switch (XDBGETDBACCESS(gpi->xdb->con[cx].xdbId))
	       {
#ifdef WANTODBC
		 case XDBODBC:
			v_Msg(ctx,NULL,"XDBNoSup",intmodx) ; return(NULL) ;
#endif
#ifdef WANTV4IS
		 case XDBV4IS:
			v_Msg(ctx,NULL,"XDBNoSup",intmodx) ; return(NULL) ;
#endif
#ifdef WANTMYSQL
		 case XDBMYSQL:
			{ int res ;
			  if (commit)
			   { res = mysql_commit(gpi->xdb->con[cx].mysql) ; }
			   else { res = mysql_rollback(gpi->xdb->con[cx].mysql) ; } ;
			  if (res != 0)
			   { v_Msg(ctx,NULL,"XDBComFail",intmodx,gpi->xdb->con[cx].ConnectionStr,DE(MySQL),mysql_errno(gpi->xdb->con[cx].mysql),ASCretUC((char *)mysql_error(gpi->xdb->con[cx].mysql))) ;
			     goto fail ;
			   } ;
			  logPNTv(respnt,TRUE) ; return(respnt) ;
			}
#endif
#ifdef WANTORACLE
		 case XDBORACLE:
			{ int res ;
			  if (commit)
			   { res = OCI_Commit(gpi->xdb->con[cx].oracle) ; }
			   else { res = OCI_Rollback(gpi->xdb->con[cx].oracle) ; } ;
			  if (res != 0)
			   { v_Msg(ctx,NULL,"XDBComFail",intmodx,gpi->xdb->con[cx].ConnectionStr,DE(Oracle),gpi->xdb->con[cx].lastErrCode,gpi->xdb->con[cx].lastErrMsg) ;
			     goto fail ;
			   } ;
			  logPNTv(respnt,TRUE) ; return(respnt) ;
			}
#endif
		} ;
	    } ;
	 } ;
	if (UCempty(dsn) && UCempty(connection)) { v_Msg(ctx,NULL,"XDBDimDSN",intmodx,V4IM_Tag_DSN) ; goto fail ; } ;
	cpt = respnt ; ZPH(cpt) ; cpt->PntType = V4DPI_PntType_XDB ; cpt->Bytes = V4PS_XDB ; cpt->Dim = di->DimId ;
	if (trace & V4TRACE_XDB)
	 { v_Msg(ctx,UCTBUF1,"XDBtConnect",intmodx,connection,dsn,user,pass) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
#ifdef WANTV4IS
	if (access == UNUSED || access == 1)
	 { if (UCstrchr(dsn,':') != NULL)		/* If connection name has ":port" then assume a MIDAS connection */
	    access = 3 ;
	 } ;
#endif
	if (access == UNUSED)				/* If still undefined then default to ODBC unless we don't have it */
	 {
#ifdef WANTODBC
	   access = 1 ;
#else
	   v_Msg(ctx,NULL,"XDBAccessNYI",intmodx,DE(ODBC)) ; goto fail ;
#endif
	 } ;
/*	Here to open a new connection. If autocom no set then default it to TRUE */
	if (autocom == UNUSED) autocom = TRUE ;
	switch (access)
	 { default:
		v_Msg(ctx,NULL,"NYIonthisOS",intmodx) ; goto fail ;
#ifdef WANTODBC
	   case 1:	/* ODBC */
		{ INDEX rcnt ;
		  for(rcnt=1;;rcnt++)
		    { cpt->Value.XDB.xdbId = v4odbc_Connect(ctx,di,intmodx,(UCnotempty(connection)?connection:NULL),dsn,(UCnotempty(user)?user:NULL),(UCnotempty(pass)?pass:NULL),(autocom > 0),&ok) ;
		      if (ok || rcnt > V4ODBC_ConnectRetryMax) break ;
		      v_Msg(ctx,UCTBUF1,"ODBCConRetry",rcnt,ctx->ErrorMsg) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
		      HANGLOOSE(V4ODBC_ConnectRetryWait) ;
		    } ;
		}
		break ;
#endif
#ifdef WANTMYSQL
	   case 2:	/* MYSQL */
		cpt->Value.XDB.xdbId = v4mysql_Connect(ctx,di,intmodx,(UCnotempty(connection)?connection:NULL),(port == UNUSED ? 0 : port),dsn,(UCnotempty(user)?user:NULL),(UCnotempty(pass)?pass:NULL),(autocom > 0),&ok) ;
		break ;
#endif
#ifdef WANTV4IS
	   case 3:	/* MIDAS */
		cpt->Value.XDB.xdbId = v4odbc_MIDASConnect(ctx,intmodx,trace,di,(UCnotempty(connection)?connection:NULL),dsn,(UCnotempty(user)?user:NULL),(UCnotempty(pass)?pass:NULL),&ok) ;
		break ;
#endif
#ifdef WANTORACLE
	   case 4:	/* ORACLE */
		cpt->Value.XDB.xdbId = v4oracle_Connect(ctx,di,intmodx,(UCnotempty(connection)?connection:NULL),(port == UNUSED ? 0 : port),dsn,(UCnotempty(user)?user:NULL),(UCnotempty(pass)?pass:NULL),(autocom > 0),&ok) ;
		break ;
#endif
	 } ;
	if (!ok) goto fail ;
/*	Maybe move some stuff into connection */
	for(cx=0;cx<gpi->xdb->conCount;cx++) { if (cpt->Value.XDB.xdbId == gpi->xdb->con[cx].xdbId) break ; } ;
	if (cx >= gpi->xdb->conCount) { v_Msg(ctx,NULL,"XDBConHandle",intmodx) ; goto fail ; } ;
	if (idPnt.Bytes != 0)
	 { memcpy(&gpi->xdb->con[cx].idPnt,&idPnt,idPnt.Bytes) ; } ;
	return(cpt) ;
fail:	if (gpi->xdb != NULL) { ctx->ErrorMsg[UCsizeof(gpi->xdbLastError)-1] = UCEOS ; UCstrcpy(gpi->xdbLastError,ctx->ErrorMsg) ; } ;
	if (trace & V4TRACE_XDB)
	 { v_Msg(ctx,UCTBUF1,"XDBtFail",intmodx,ctx->ErrorMsg) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
	REGISTER_ERROR(0) ; RETURNFAILURE ;
}

P *v4db_Error(ctx,respnt,intmodx,argpnts,argcnt,trace)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  P *respnt,*argpnts[] ;
  int argcnt ;
  int trace ;
{ P *cpt ;
  int i,ok ;

	if (gpi->xdb == NULL) { v_Msg(ctx,NULL,"XDBNoEnv",intmodx) ; goto err_fail ;} ;
	if (argcnt == 0)			/* No arguments - default to last error */
	 { uccharPNTv(respnt,gpi->xdbLastError) ; return(respnt) ; } ;
	for(i=1,ok=TRUE;ok&&i<=argcnt;i++)
	 {
	   switch (v4im_CheckPtArgNew(ctx,argpnts[i],&cpt,NULL))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto err_fail ;
	      case V4IM_Tag_Unk:		v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto err_fail ;
	      case -V4IM_Tag_Error:
		uccharPNTv(respnt,gpi->xdbLastError) ; return(respnt) ;
	    } ; if (!ok) break ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto err_fail ; } ;
err_fail:
	REGISTER_ERROR(0) ; RETURNFAILURE ;
}

/*	v4db_GetCurColVal - Returns value of column cx for current row of statment sx	*/
/*	Call: ok = V4IM_OpCode_dbVal( ctx , sx , cx , dimId , respnt )
	  where ok is TRUE if success, FALSE otherwise,
		ctx is context,
		sx is statment index,
		cx is column number (0 offset),
		dimId is dimension of returned value,
		respnt is updated with the value					*/
		
LOGICAL v4db_GetCurColVal(ctx,sx,cx,dimId,respnt)
  struct V4C__Context *ctx ;
  INDEX sx,cx ;
  DIMID dimId ;
  P *respnt ;
{ P *ipt ;
  struct V4DPI__DimInfo *di ;
  struct V4XDB__Master *xdb ;
  ETYPE vdType ; BYTE *data ; LENMAX bytes ;
  
	xdb = gpi->xdb ;
	vdType = gpi->xdb->stmts[sx].col[cx].vdType ;
	if (cx >= gpi->xdb->stmts[sx].colCount)
	 { v_Msg(ctx,NULL,"XDBMaxCol",V4IM_OpCode_dbVal,cx+1,gpi->xdb->stmts[sx].colCount) ; return(FALSE) ; } ;
 	switch (XDBGETDBACCESS(gpi->xdb->stmts[sx].xdbId))
 	 {
#ifdef WANTODBC
	   case XDBODBC:
		data = gpi->xdb->stmts[sx].data+gpi->xdb->stmts[sx].col[cx].Offset ;
		bytes = gpi->xdb->stmts[sx].col[cx].UBytes ;
		break ;
#endif
#ifdef WANTMYSQL
	   case XDBMYSQL:
		data = gpi->xdb->stmts[sx].bind[cx].buffer ;
		bytes = (gpi->xdb->stmts[sx].col[cx].isNull ? -1 : gpi->xdb->stmts[sx].col[cx].length) ;
		break ;
#endif
#ifdef WANTORACLE
	   case XDBORACLE:
		if (OCI_IsNull(gpi->xdb->stmts[sx].rs,cx))
		 { bytes = -1 ; break ; } ;
		data = (BYTE *)gpi->xdb->stmts[sx].rs ;		/* For ORACLE, pass rs and do the conversions within v4dpi_AcceptValue() */
		bytes = (sx << 16) | (cx + 1) ;			/* Pack sx and cx (V4 is zero based, Oracle starts with 1) into single int */
		break ;
#endif
#ifdef WANTV4IS
	   case XDBV4IS:
		data = gpi->xdb->stmts[sx].data+gpi->xdb->stmts[sx].col[cx].Offset ;
		bytes = gpi->xdb->stmts[sx].col[cx].UBytes ;
		break ;
#endif
	 } ;

	DIMINFO(di,ctx,(dimId == UNUSED ? v4xdb_VDTYPEtoDim(vdType) : dimId)) ;
	if (bytes < 0)				/* Null value - can't return anything */
	 { v_Msg(ctx,NULL,"XDBNullVal",V4IM_OpCode_dbVal,cx+1,di->DimId) ; return(FALSE) ; } ;
/*	Now convert xdb value to corresponding V4 point */
	ipt = v4dpi_AcceptValue(ctx,respnt,di,vdType,data,bytes) ;
	return(TRUE) ;
}

#ifdef WANTODBC

/*	v4odbc_Connect - Connect to ODBC Server				*/
/*	Call: xdbId = v4odbc_Connect( ctx , di, intmodx, connection, dsn , user , password , autocom , ok )
	  where xdbId = Connection handle (0 if error),
		di = diminfo of "host" dimension,
		intmodx is module index,
		connection is ptr to connection info,
		dsn is string ptr to service/db name,
		user/password are ptrs to string or NULL,
		autocom is TRUE for auto-commit, FALSE for manual commit,
		ok is updated TRUE/FALSE if all is well or not (ctx->ErrorMsg updated with error)	*/

COUNTER v4odbc_Connect(ctx,di,intmodx,connection,dsn,user,password,autocom,ok)
  struct V4C__Context *ctx ;
  struct V4DPI__DimInfo *di ;
  UCCHAR *connection,*dsn,*user,*password ;
  int *ok,autocom ; INTMODX intmodx ;
{ struct V4XDB__Master *xdb ;
  RETCODE rc ;
  HDBC hdbc ;
  int xdbId ;
  UCCHAR szSQLState[6] ;
  INDEX newx ; int bytes ; UCCHAR realcon[512] ;

	if (gpi->xdb->hEnv == NULL)
	 { rc = SQLAllocEnv(&gpi->xdb->hEnv) ;
	   if (rc == SQL_ERROR)
	    { v_Msg(ctx,NULL,"@Could not allocate hEnv") ; *ok = FALSE ; return(0) ; } ;
	 } ;
	xdb = gpi->xdb ;
	XDBNEWID(xdbId,XDBODBC,XDBCONNECTION) ;
	ZUS(realcon) ; ZUS(ctx->ErrorMsgAux) ;	/* Clear ErrorMsgAux because it doesn't always get updated by these ODBC routines */
/*	Look for empty connection slot */
	for (newx=0;newx<xdb->conCount;newx++) { if (xdb->con[newx].xdbId == UNUSED) break ; } ;
	if (newx == xdb->conCount)
	 { if (xdb->conCount >= V4XDB_ConnectMax) { v_Msg(ctx,NULL,"XDBMaxCon",intmodx,V4XDB_ConnectMax) ; *ok = FALSE ; return(0) ; } ;
	 } ;
	rc = SQLAllocConnect(xdb->hEnv,&hdbc) ;
	if (rc == SQL_ERROR || rc == SQL_INVALID_HANDLE) { v_Msg(ctx,NULL,"ODBCAllochdbc",intmodx) ; *ok = FALSE ; return(0) ; } ;
	if (connection != NULL)
	 { rc = UCSQLDriverConnect(hdbc,(HWND)0,connection,(SQLSMALLINT)UCstrlen(connection),realcon,UCsizeof(realcon),(SQLSMALLINT *)&bytes,SQL_DRIVER_NOPROMPT) ;
	   if (rc == SQL_ERROR)
	    { UCSQLGetDiagRec(SQL_HANDLE_DBC,hdbc,1,szSQLState,NULL,ctx->ErrorMsgAux,1000,NULL) ;
	      v_Msg(ctx,NULL,"ODBCConErr",intmodx,dsn,szSQLState,1) ; *ok = FALSE ; return(0) ;
	    } ;
	   if (UCnotempty(realcon))		/* Save the connection string for possible access via ODBCInfo(Connection?) */
	    { xdb->con[newx].ConnectionStr = v4mm_AllocUC(UCstrlen(realcon)) ;
	      UCstrcpy(xdb->con[newx].ConnectionStr,realcon) ;
	    } ;
	 } else
	 { rc = UCSQLConnect(hdbc,dsn,SQL_NTS,user,SQL_NTS,password,SQL_NTS) ;
	   if (rc == SQL_ERROR || rc == SQL_INVALID_HANDLE)
	    { SQLSMALLINT mlen ; UCSQLGetDiagRec(SQL_HANDLE_DBC,hdbc,1,szSQLState,NULL,ctx->ErrorMsgAux,1000,&mlen) ;
	      v_Msg(ctx,NULL,"ODBCConErr",intmodx,dsn,szSQLState,2) ; *ok = FALSE ; return(0) ;
	    } ;
	 } ;
	xdb->con[newx].DimId = di->DimId ;
	xdb->con[newx].MIDASSocket = UNUSED ;		/* Not a MIDAS/ODBC connection */
	if (UCempty(realcon))                             /* If we still don't have connection string, then make one */
	 v_Msg(ctx,realcon,"@DSN=%1U",(dsn != NULL ? dsn : UClit("unkown"))) ;
	xdb->con[newx].ConnectionStr = v4mm_AllocUC(UCstrlen(realcon)) ;
	UCstrcpy(xdb->con[newx].ConnectionStr,realcon) ;
	xdb->con[newx].hdbc = hdbc ;
	SQLSetConnectOption(hdbc,SQL_AUTOCOMMIT,(autocom ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF)) ;
	if (rc == SQL_ERROR)
	 { UCSQLGetDiagRec(SQL_HANDLE_DBC,hdbc,1,szSQLState,NULL,ctx->ErrorMsgAux,1000,NULL) ;
	   v_Msg(ctx,NULL,"ODBCConErr",intmodx,dsn,szSQLState,3) ; *ok = FALSE ; return(0) ;
	 } ;
	xdb->con[newx].xdbId = xdbId ; if (newx == xdb->conCount) xdb->conCount++ ;
	*ok = TRUE ; return(xdbId) ;
}
#endif
#ifdef WANTMYSQL
/*	v4mysql_Connect - Connect to mySQL Server				*/
/*	Call: xdbId = v4mysql_Connect( ctx , di, intmodx, host, port, dsn , user , password , autocom , ok )
	  where xdbId = Connection handle (0 if error),
		di = diminfo of "host" dimension,
		intmodx is module index,
		host is host name NULL for default localhost,
		port is port number to connect with (0 for default),
		dsn is string ptr to service/db name,
		user/password are ptrs to string or NULL,
		autocom is TRUE for auto-commit, FALSE for manual commit,
		ok is updated TRUE/FALSE if all is well or not (ctx->ErrorMsg updated with error)	*/

XDBID v4mysql_Connect(ctx,di,intmodx,host,port,dsn,user,password,autocom,ok)
  struct V4C__Context *ctx ;
  struct V4DPI__DimInfo *di ;
  UCCHAR *host,*dsn,*user,*password ;
  int port ;
  LOGICAL *ok,autocom ; INTMODX intmodx ;
{
  struct V4XDB__Master *xdb ;
  INDEX newx ;
  

	xdb = gpi->xdb ;
/*	Look for empty connection slot */
	for (newx=0;newx<xdb->conCount;newx++) { if (xdb->con[newx].xdbId == UNUSED) break ; } ;
	if (newx == xdb->conCount)
	 { if (xdb->conCount >= V4XDB_ConnectMax) { v_Msg(ctx,NULL,"XDBMaxCon",intmodx,V4XDB_ConnectMax) ; *ok = FALSE ; return(0) ; } ;
	 } ;
	XDBNEWID(gpi->xdb->con[newx].xdbId,XDBMYSQL,XDBCONNECTION) ;
	gpi->xdb->con[newx].mysql = mysql_init(NULL) ;
	if (mysql_real_connect(gpi->xdb->con[newx].mysql,(host == NULL ? "" : UCretASC(host)),(user == NULL ? "" : UCretASC(user)),(password == NULL ? "" : UCretASC(password)),UCretASC(dsn),port,NULL,0) == NULL)
	 { v_Msg(ctx,NULL,"XDBConFail",intmodx,dsn,DE(MySQL),mysql_errno(gpi->xdb->con[newx].mysql),ASCretUC((char *)mysql_error(gpi->xdb->con[newx].mysql))) ; *ok = FALSE ; return(0) ; } ;
	gpi->xdb->con[newx].DimId = di->DimId ;
	 if (newx == xdb->conCount) xdb->conCount++ ; *ok = TRUE ;
/*	By default mySQL auto-commit is enabled, only turn it off if requested */
	if (!autocom)
	 mysql_autocommit(gpi->xdb->con[newx].mysql,FALSE) ;

/*	Save params in case we need to reconnect */
#define CUC(DST,SRC) if (SRC == NULL) { gpi->xdb->con[newx].DST = NULL ; } else { gpi->xdb->con[newx].DST = v4mm_AllocUC(UCstrlen(SRC)) ; UCstrcpy(gpi->xdb->con[newx].DST,SRC) ; } ;
	CUC(host,host) CUC(dsn,dsn) CUC(user,user) CUC(password,password) gpi->xdb->con[newx].autocom = autocom ; gpi->xdb->con[newx].port = port ;

	return(gpi->xdb->con[gpi->xdb->conCount-1].xdbId) ;
}
#endif

#ifdef WANTORACLE
/*	v4oracle_Connect - Connect to mySQL Server				*/
/*	Call: xdbId = v4oracle_Connect( ctx , di, intmodx, host, port, dsn , user , password , autocom , ok )
	  where xdbId = Connection handle (0 if error),
		di = diminfo of "host" dimension,
		intmodx is module index,
		host is host name NULL for default localhost,
		port is port number to connect with (0 for default),
		dsn is string ptr to service/db name,
		user/password are ptrs to string or NULL,
		autocom is TRUE for auto-commit, FALSE for manual commit,
		ok is updated TRUE/FALSE if all is well or not (ctx->ErrorMsg updated with error)	*/

XDBID v4oracle_Connect(ctx,di,intmodx,host,port,dsn,user,password,autocom,ok)
  struct V4C__Context *ctx ;
  struct V4DPI__DimInfo *di ;
  UCCHAR *host,*dsn,*user,*password ;
  int port ;
  LOGICAL *ok,autocom ; INTMODX intmodx ;
{
  struct V4XDB__Master *xdb ;
  INDEX newx ;
  

	xdb = gpi->xdb ;
/*	Look for empty connection slot */
	for (newx=0;newx<xdb->conCount;newx++) { if (xdb->con[newx].xdbId == UNUSED) break ; } ;
	if (newx == xdb->conCount)
	 { if (xdb->conCount >= V4XDB_ConnectMax) { v_Msg(ctx,NULL,"XDBMaxCon",intmodx,V4XDB_ConnectMax) ; *ok = FALSE ; return(0) ; } ;
	 } ;
	XDBNEWID(gpi->xdb->con[newx].xdbId,XDBORACLE,XDBCONNECTION) ;
	gpi->xdb->lastCX = newx ;
	gpi->xdb->con[newx].oracle = OCI_ConnectionCreate(UCretASC(dsn),UCretASC(user),UCretASC(password),OCI_SESSION_SYSDBA /* OCI_SESSION_DEFAULT */) ;
	if (gpi->xdb->con[newx].oracle == NULL)
	 { v_Msg(ctx,NULL,"XDBConFail",intmodx,dsn,DE(Oracle),gpi->xdb->con[newx].lastErrCode,gpi->xdb->con[newx].lastErrMsg) ; *ok = FALSE ; return(0) ; } ;
	gpi->xdb->con[newx].DimId = di->DimId ;
	if (newx == xdb->conCount) xdb->conCount++ ; *ok = TRUE ;
	OCI_SetAutoCommit(gpi->xdb->con[newx].oracle,autocom) ;

/*	Save params in case we need to reconnect */
#define CUC(DST,SRC) if (SRC == NULL) { gpi->xdb->con[newx].DST = NULL ; } else { gpi->xdb->con[newx].DST = v4mm_AllocUC(UCstrlen(SRC)) ; UCstrcpy(gpi->xdb->con[newx].DST,SRC) ; } ;
	CUC(host,host) CUC(dsn,dsn) CUC(user,user) CUC(password,password) gpi->xdb->con[newx].autocom = autocom ; gpi->xdb->con[newx].port = port ;

	return(gpi->xdb->con[gpi->xdb->conCount-1].xdbId) ;
}
#endif

/*	v4xdb_NewStmtInitCommon - Common setup for new XDB statement	*/

LOGICAL v4xdb_NewStmtInitCommon(ctx,xdb,hxdbId,xdbType,sqlstmt,intmodx,cxPtr,sxPtr,xdbIdPtr,trace)
  struct V4C__Context *ctx ;
  struct V4XDB__Master *xdb ;
  XDBID hxdbId ;
  ETYPE xdbType ;
  UCCHAR *sqlstmt ;
  INTMODX intmodx ;
  INDEX *cxPtr,*sxPtr ;
  XDBID *xdbIdPtr ;
  VTRACE trace ;
{
  XDBID xdbId ; INDEX sx,cx ;
  
	for(;!GETSPINLOCK(&xdb->xdbLock);) {} ;
	if (trace & V4TRACE_XDB) { v_Msg(ctx,UCTBUF1,"XDBtNewStmt",intmodx,sqlstmt) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
	for (cx=0;cx<xdb->conCount;cx++) { if (hxdbId == xdb->con[cx].xdbId) break ; } ;
	if (cx >= xdb->conCount) { v_Msg(ctx,NULL,"XDBConHandle",intmodx) ; goto fail ; } ;
	for (sx=0;sx<xdb->stmtCount;sx++) { if (xdb->stmts[sx].xdbId == UNUSED) break ; } ;
	if (sx == xdb->stmtCount && sx > 0)
	 { if (xdb->stmtCount >= V4XDB_StmtMax) { v_Msg(ctx,NULL,"XDBMaxStmnt",intmodx,V4XDB_StmtMax) ; goto fail ; } ;
	 } ;
	XDBNEWID(xdbId,xdbType,XDBSTATEMENT) ;
	memset(&xdb->stmts[sx],0,sizeof xdb->stmts[sx]) ;	/* Clear out next free statement slot */
	xdb->stmts[sx].hxdbId = xdb->con[cx].xdbId ;
	xdb->stmts[sx].dimId = xdb->con[cx].DimId ;
	*cxPtr = cx ; *sxPtr = sx ; *xdbIdPtr = xdbId ;
	return(TRUE) ;

fail:	FREEMTLOCK(xdb->xdbLock) ;
	return(FALSE) ;
}

void v4xdb_NewStmtFinish(ctx,xdb,sx,xdbId)
  struct V4C__Context *ctx ;
  struct V4XDB__Master *xdb ;
  INDEX sx ;
  XDBID xdbId ;
{ 
/*	These two assignments 'commit' the statement and make it real */
	xdb->stmts[sx].xdbId = xdbId ; if (sx == xdb->stmtCount) xdb->stmtCount ++ ;
	FREEMTLOCK(xdb->xdbLock) ;
}


/*	v4xdb_FreeSavedInfo - Frees up any prior rows saved for a dimension	*/
/*	Call: logical = v4xdb_FreeSavedInfo( dimId )
	  where logical is TRUE if dimId found & released, FALSE if not found,
		dimId is dimension to release					*/

LOGICAL v4xdb_FreeSavedInfo(dimId)
  DIMID dimId ;
{ struct V4XDB__SaveStmtInfo *vxssi ;

/*	Find this entry in linked list and link around it so we can deallocate it */
	for(vxssi=gpi->xdb->vxssi;vxssi!=NULL;vxssi=vxssi->vxssiNext)
	 { if (vxssi->dimId == dimId) break ; } ;
	if (vxssi == NULL) return(FALSE) ;
	if (vxssi->vxsr != NULL) v4mm_FreeChunk(vxssi->vxsr) ;
	if (vxssi->lpnt != NULL) v4mm_FreeChunk(vxssi->lpnt) ;
	if (vxssi == gpi->xdb->vxssi)			/* Now remove this vxssi from linked list of active ones */
	 { gpi->xdb->vxssi = vxssi->vxssiNext ;		/* If first one then it's easy */
	 } else
	 { struct V4XDB__SaveStmtInfo *vxssif ;		/* Have to start at beginning and when found, link around */
	   for(vxssif=gpi->xdb->vxssi;vxssif!=NULL;vxssif=vxssif->vxssiNext)
	    { if (vxssif->vxssiNext == vxssi) { vxssif->vxssiNext = vxssi->vxssiNext ; break ; } ;
	    } ;
	 } ;
	v4mm_FreeChunk(vxssi) ;
	return(TRUE) ;
}


#ifdef WANTODBC
INDEX v4odbc_NewStmt(ctx,hxdbId,sqlstmt,intmodx,trace)
  struct V4C__Context *ctx ;
  COUNTER hxdbId ;
  UCCHAR *sqlstmt ;
  INTMODX intmodx ; VTRACE trace ;
{ struct V4XDB__Master *xdb ;
  RETCODE rc ;
  HSTMT hstmt ;
  XDBID xdbId ;
  int col,ttl,res ; SQLLEN rowCnt ; INDEX cx,sx ;
  UCCHAR szSQLState[6] ;

	if ((xdb = gpi->xdb) == NULL) { v_Msg(ctx,NULL,"XDBNoCon",intmodx) ; goto fail ; } ;
	if (!v4xdb_NewStmtInitCommon(ctx,xdb,hxdbId,XDBODBC,sqlstmt,intmodx,&cx,&sx,&xdbId,trace)) goto fail ; ;
	rc = SQLAllocStmt(xdb->con[cx].hdbc,&hstmt) ;
	if (rc == SQL_ERROR) { v_Msg(ctx,NULL,"ODBCNoHstmt",intmodx) ; goto fail ; } ;
	rc = UCSQLExecDirect(hstmt,sqlstmt,SQL_NTS) ;
	if (rc == SQL_ERROR)
	 { UCSQLGetDiagRec(SQL_HANDLE_STMT,hstmt,1,szSQLState,NULL,ctx->ErrorMsgAux,1000,NULL) ;
	   v_Msg(ctx,NULL,"ODBCXDirectErr",intmodx,szSQLState,sqlstmt) ; goto fail ;
	 } ;
	rc = SQLRowCount(hstmt,&rowCnt) ;		/* Note: this most often returns -1 because of driver limitations */
	if (rc == SQL_ERROR) rowCnt = UNUSED ;
	if (trace & V4TRACE_XDB)
	 { v_Msg(ctx,UCTBUF1,"XDBtRows",intmodx,rowCnt) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
	xdb->stmts[sx].curRow = 0 ; xdb->stmts[sx].rowCnt = rowCnt ;
	xdb->stmts[sx].hstmt = hstmt ; xdb->stmts[sx].hdbc = xdb->con[cx].hdbc ;
	res = -999 ;
	for(ttl=0,col=0;col<V4XDB_StmtColMax;col++)
	 { if ((res = v4odbc_SetupRowCol(ctx,xdb,sx,col)) != TRUE) break ; /* Set up each returned column */
	   xdb->stmts[sx].col[col].Offset =
	    (col == 0 ? 0 : (ttl=xdb->stmts[sx].col[col-1].Offset + xdb->stmts[sx].col[col-1].length)) ;
	 } ; ttl += xdb->stmts[sx].col[col-1].length ;
	if (col >= V4XDB_StmtColMax) { v_Msg(ctx,NULL,"XDBMaxCol",intmodx,col,V4XDB_StmtColMax) ; goto fail ; } ;
	if (res == -999) { v_Msg(ctx,NULL,"ODBCColErr",intmodx,col,sqlstmt) ; goto fail ; } ;
	xdb->stmts[sx].colCount = col ;
	xdb->stmts[sx].data = v4mm_AllocChunk(ttl,FALSE) ;	/* Allocate big buffer for all columns */
	for(col=0;col<xdb->stmts[sx].colCount;col++)
	 { SWORD nbytes,type,scale,null ; SQLLEN precision ; SQLHDESC hdesc ; UCCHAR name[255] ;
/*	   C indexes starting with 0, ODBC starts with 1 so have to add one to each reference to col */
	   SQLBindCol(hstmt,(SQLUSMALLINT)(col+1),xdb->stmts[sx].col[col].SQLCType,
		xdb->stmts[sx].data+xdb->stmts[sx].col[col].Offset,
		xdb->stmts[sx].col[col].length, &xdb->stmts[sx].col[col].UBytes) ;
	   switch (xdb->stmts[sx].col[col].SQLCType)
	    { case SQL_C_NUMERIC:
		rc = UCSQLDescribeCol(xdb->stmts[sx].hstmt,(SQLSMALLINT)col+1,name,UCsizeof(name),&nbytes,&type,&precision,&scale,&null) ;
		if (rc != SQL_ERROR) rc = SQLGetStmtAttr(xdb->stmts[sx].hstmt, SQL_ATTR_APP_ROW_DESC,&hdesc,0,NULL);
		if (rc != SQL_ERROR) rc = SQLSetDescField (hdesc,col+1,SQL_DESC_PRECISION,(VOID*)precision,0);
		if (rc != SQL_ERROR) rc = SQLSetDescField (hdesc,col+1,SQL_DESC_SCALE,(VOID*)scale,0);
		if (rc == SQL_ERROR)
		 { UCSQLGetDiagRec(SQL_HANDLE_STMT,hstmt,1,szSQLState,NULL,ctx->ErrorMsgAux,1000,NULL) ;
		   v_Msg(ctx,NULL,"ODBCColErr10",intmodx,col+1,name) ; goto fail ;
		 } ;
		break ;
	    } ;
	 } ;
	v4xdb_NewStmtFinish(ctx,xdb,sx,xdbId) ;
	return(sx) ;		/* Return index of this statement */

fail:	FREEMTLOCK(xdb->xdbLock) ;
	if (gpi->xdb != NULL)  { ctx->ErrorMsg[UCsizeof(gpi->xdbLastError)-1] = UCEOS ; UCstrcpy(gpi->xdbLastError,ctx->ErrorMsg) ; } ;
	if (trace & V4TRACE_XDB)
	 { v_Msg(ctx,UCTBUF1,"XDBtFail",intmodx,ctx->ErrorMsg) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
	return(UNUSED) ;
}

/*	v4odbc_NewTableStmt - Returns result-set for tables in current DSN	*/

P *v4odbc_NewTableStmt(ctx,respnt,hxdbId,tablepattern,tabletype,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt ;
  COUNTER hxdbId ;
  UCCHAR *tablepattern,*tabletype ;
  INTMODX intmodx ; VTRACE trace ;
{ struct V4XDB__Master *xdb ;
  RETCODE rc ;
  HSTMT hstmt ;
  XDBID xdbId ;
  P *ipt ;
  int cx,col,ttl,res ;
  UCCHAR szSQLState[6] ;

	if ((xdb = gpi->xdb) == NULL) { v_Msg(ctx,NULL,"XDBNoCon",intmodx) ; goto fail ; } ;
	XDBNEWID(xdbId,XDBODBC,XDBSTATEMENT) ;
	for (cx=0;cx<xdb->conCount;cx++) { if (hxdbId == xdb->con[cx].xdbId) break ; } ;
	if (cx >= xdb->conCount) { v_Msg(ctx,NULL,"XDBConHandle",intmodx) ; goto fail ; } ;
	if (xdb->stmtCount >= V4XDB_StmtMax) { v_Msg(ctx,NULL,"XDBMaxStmnt",intmodx,V4XDB_StmtMax) ; goto fail ; } ;
	rc = SQLAllocStmt(xdb->con[cx].hdbc,&hstmt) ;
	if (rc == SQL_ERROR) { v_Msg(ctx,NULL,"ODBCNoHstmt",intmodx) ; goto fail ; } ;
	rc = UCSQLTables(hstmt,UClit(""),0,UClit(""),0,(UCnotempty(tablepattern) ? tablepattern : NULL),(SQLSMALLINT)UCstrlen(tablepattern),(UCnotempty(tabletype) ? tabletype : NULL),(SQLSMALLINT)UCstrlen(tabletype)) ;
	if (rc == SQL_ERROR)
	 { UCSQLGetDiagRec(SQL_HANDLE_STMT,hstmt,1,szSQLState,NULL,ctx->ErrorMsgAux,1000,NULL) ;
	   v_Msg(ctx,NULL,"ODBCTablesErr",intmodx,szSQLState,tablepattern,tabletype) ; goto fail ;
	 } ;
	memset(&xdb->stmts[xdb->stmtCount],0,sizeof xdb->stmts[xdb->stmtCount]) ;		/* Clear out next free statement slot */
	xdb->stmts[xdb->stmtCount].hstmt = hstmt ; xdb->stmts[xdb->stmtCount].hdbc = xdb->con[cx].hdbc ;
	xdb->stmts[xdb->stmtCount].curRow = 0 ; xdb->stmts[xdb->stmtCount].vmt = NULL ;
	xdb->stmts[xdb->stmtCount].xdbId = xdbId ; xdb->stmts[xdb->stmtCount].hxdbId = xdb->con[cx].xdbId ;
	if (UCempty(tablepattern)) UCstrcpy(tablepattern,UClit("*")) ;
	if (UCempty(tabletype)) UCstrcpy(tabletype,UClit("*")) ;
	res = -999 ; ttl = 0 ;
	for(col=0;col<V4XDB_StmtColMax;col++)
	 { if ((res = v4odbc_SetupRowCol(ctx,xdb,xdb->stmtCount,col)) != TRUE) break ; /* Set up each returned column */
	   xdb->stmts[xdb->stmtCount].col[col].Offset =
	    (col == 0 ? 0 : (ttl=xdb->stmts[xdb->stmtCount].col[col-1].Offset + xdb->stmts[xdb->stmtCount].col[col-1].length)) ;
	 } ; ttl += xdb->stmts[xdb->stmtCount].col[col-1].length ;
	if (col >= V4XDB_StmtColMax) { v_Msg(ctx,NULL,";",intmodx,V4XDB_StmtColMax) ; goto fail ; } ;
	if (res == -999) { v_Msg(ctx,NULL,"ODBCColErr",intmodx,col,tablepattern) ; goto fail ; } ;
	xdb->stmts[xdb->stmtCount].colCount = col - 1 ;
	xdb->stmts[xdb->stmtCount].data = v4mm_AllocChunk(ttl,FALSE) ;	/* Allocate big buffer for all columns */
	for(col=0;col<xdb->stmts[xdb->stmtCount].colCount;col++)
	 { SQLBindCol(hstmt,(SQLSMALLINT)col,xdb->stmts[xdb->stmtCount].col[col].SQLCType,
		xdb->stmts[xdb->stmtCount].data+xdb->stmts[xdb->stmtCount].col[col].Offset,
		xdb->stmts[xdb->stmtCount].col[col].length, &xdb->stmts[xdb->stmtCount].col[col].UBytes)  ;
	 } ;
	xdb->stmtCount ++ ;
	ipt = respnt ;
	ZPH(ipt) ; ipt->PntType = V4DPI_PntType_XDB ; ipt->Bytes = V4PS_XDB ;
	ipt->Dim = xdb->con[cx].DimId ; ipt->Value.XDB.xdbId = xdbId ;
	return(ipt) ;

fail:	if (gpi->xdb != NULL)  { ctx->ErrorMsg[UCsizeof(gpi->xdbLastError)-1] = UCEOS ; UCstrcpy(gpi->xdbLastError,ctx->ErrorMsg) ; } ;
	REGISTER_ERROR(0) ; RETURNFAILURE ;
}
#endif

#ifdef WANTMYSQL
INDEX v4mysql_NewStmt(ctx,hxdbId,sqlstmt,intmodx,trace,nest)
  struct V4C__Context *ctx ;
  XDBID hxdbId ;
  UCCHAR *sqlstmt ;
  INTMODX intmodx ; VTRACE trace ;
  COUNTER nest ;		/* If > 0 then number of times allowed to nest with connection reset in-between */
{ struct V4XDB__Master *xdb ;
  INDEX i,offset ; LENMAX l ; DIMID did ; ETYPE vdType ; XDBID xdbId ;
  INDEX cx,sx ; INDEX colOffsets[V4XDB_StmtColMax] ;

	if ((xdb = gpi->xdb) == NULL) { v_Msg(ctx,NULL,"XDBNoCon",intmodx) ; goto fail ; } ;
	if (!v4xdb_NewStmtInitCommon(ctx,xdb,hxdbId,XDBMYSQL,sqlstmt,intmodx,&cx,&sx,&xdbId,trace)) goto fail ;

	xdb->stmts[sx].stmt = mysql_stmt_init(gpi->xdb->con[cx].mysql) ;
	if (mysql_stmt_prepare(xdb->stmts[sx].stmt,UCretASC(sqlstmt),UCstrlen(sqlstmt)) != 0)
	 {
/*	   If got out-of-synch error then close & reopen the connection */
	   if (nest > 0 && mysql_errno(xdb->con[cx].mysql) == 2014 /* CR_COMMANDS_OUT_OF_SYNC */)
	    { struct V4DPI__DimInfo *di ; LOGICAL ok ; COUNTER hxdbId2 ;
	      UCCHAR *host, *dsn, *user, *password ; LOGICAL autocom ;
	      DIMINFO(di,ctx,xdb->con[cx].DimId) ; autocom = xdb->con[cx].autocom ;
#define CCI(NAME) if (xdb->con[cx].NAME == NULL) { NAME = NULL ; } else { NAME = v4mm_AllocUC(UCstrlen(xdb->con[cx].NAME)) ; UCstrcpy(NAME,xdb->con[cx].NAME) ; } ;
	      CCI(host) CCI(dsn) CCI(user) CCI(password) ;
	      v_Msg(ctx,UCTBUF1,"XDBSynch",sqlstmt) ; vout_UCText(VOUT_Warn,0,UCTBUF1) ;
	      v4mysql_FreeStuff(ctx,hxdbId,0) ;			/* Close off connection */
/*	      Re-open connection */
	      hxdbId2 = v4mysql_Connect(ctx,di,intmodx,host,(xdb->con[cx].port == UNUSED ? 0 : xdb->con[cx].port),dsn,user,password,autocom,&ok) ;
/*	      And try again */
	      if (ok) return(v4mysql_NewStmt(ctx,hxdbId2,sqlstmt,intmodx,trace,nest-1)) ;
	    } ;
	   v_Msg(ctx,NULL,"XDBPrepErr",intmodx,mysql_errno(xdb->con[cx].mysql),sqlstmt,ASCretUC((char *)mysql_error(xdb->con[cx].mysql))) ; goto fail ;
	 } ;


	if (mysql_stmt_execute(xdb->stmts[sx].stmt) != 0)
	 { v_Msg(ctx,NULL,"XDBXctErr",intmodx,mysql_errno(xdb->con[cx].mysql),ASCretUC((char *)mysql_error(xdb->con[cx].mysql)),sqlstmt) ; goto fail ; } ;
	xdb->stmts[sx].res = mysql_stmt_result_metadata(xdb->stmts[sx].stmt) ;
	if (xdb->stmts[sx].res == NULL)
	 { v_Msg(ctx,NULL,"XDBMetaErr",intmodx,mysql_errno(xdb->con[cx].mysql),sqlstmt,ASCretUC((char *)mysql_error(xdb->con[cx].mysql))) ; goto fail ; } ;
	xdb->stmts[sx].colCount = mysql_num_fields(xdb->stmts[sx].res) ;
	if (xdb->stmts[sx].colCount >= V4XDB_StmtColMax) { v_Msg(ctx,NULL,"XDBMaxCol",intmodx,xdb->stmts[sx].colCount,V4XDB_StmtColMax) ; goto fail ; } ;
	memset(xdb->stmts[sx].bind,0,(sizeof(MYSQL_BIND))*xdb->stmts[sx].colCount) ;
	xdb->stmts[sx].rowCnt = mysql_num_rows(xdb->stmts[sx].res) ;
	offset = 0 ;
	for(i=0;i<xdb->stmts[sx].colCount;i++)
	 { xdb->stmts[sx].fld = mysql_fetch_field_direct(xdb->stmts[sx].res,i) ;
	   memset(&xdb->stmts[sx].bind[i],0,sizeof xdb->stmts[sx].bind[i]) ;
	   xdb->stmts[sx].bind[i].buffer_type = xdb->stmts[sx].fld->type ;
	   switch (xdb->stmts[sx].fld->type)
	    { default:				v_Msg(ctx,NULL,"XDBUnkSQLType",intmodx,xdb->stmts[sx].fld->type) ; goto fail ;
	      case MYSQL_TYPE_TINY:		vdType = VDTYPE_b1SInt ; l = sizeof(char) ; did = Dim_Int ; break ;
	      case MYSQL_TYPE_SHORT:		vdType = VDTYPE_b2SInt ; offset = ALIGN(offset) ; did = Dim_Int ; l = sizeof(int) ; break ;
	      case MYSQL_TYPE_INT24:		vdType = VDTYPE_b4SInt ; offset = ALIGN(offset) ; l = sizeof(int) ; did = Dim_Int ; break ;
	      case MYSQL_TYPE_LONG:		vdType = VDTYPE_b4SInt ; offset = ALIGN(offset) ; l = sizeof(int) ; did = Dim_Int ; break ;	
	      case MYSQL_TYPE_LONGLONG:		vdType = VDTYPE_b8SInt ; offset = ALIGNDBL(offset) ; l = sizeof(B64INT) ; did = Dim_UFix ; break ;
	      case MYSQL_TYPE_FLOAT:		vdType = VDTYPE_b4FP ; offset = ALIGN(offset) ; l = sizeof(float) ; did = Dim_Num ; break ;
	      case MYSQL_TYPE_DOUBLE:		vdType = VDTYPE_b8FP ; offset = ALIGNDBL(offset) ; l = sizeof(double) ; did = Dim_Num ; break ;	
	      case MYSQL_TYPE_NEWDECIMAL:	vdType = VDTYPE_NEWDEC ; l = xdb->stmts[sx].fld->length ; did = Dim_UFix ; break ;
	      case MYSQL_TYPE_YEAR:		vdType = VDTYPE_b2SInt ; offset = ALIGN(offset) ; l = sizeof(short) ; did = Dim_UYear ; break ;
	      case MYSQL_TYPE_TIME:		vdType = VDTYPE_MYSQL_TS ; offset = ALIGN(offset) ; l = sizeof(MYSQL_TIME) ; did = Dim_UTime ; break ;
	      case MYSQL_TYPE_DATE:		vdType = VDTYPE_MYSQL_TS ; offset = ALIGN(offset) ; l = sizeof(MYSQL_TIME) ; did = Dim_UDate ; break ;
	      case MYSQL_TYPE_DATETIME:		vdType = VDTYPE_MYSQL_TS ; offset = ALIGN(offset) ; l = sizeof(MYSQL_TIME) ; did = Dim_UDT ; break ;
	      case MYSQL_TYPE_TIMESTAMP:	vdType = VDTYPE_MYSQL_TS ; offset = ALIGN(offset) ; l = sizeof(MYSQL_TIME) ; did = Dim_UDT ; break ;
	      case MYSQL_TYPE_STRING:
	      case MYSQL_TYPE_VARCHAR:
	      case MYSQL_TYPE_VAR_STRING:
	      case MYSQL_TYPE_TINY_BLOB:
	      case MYSQL_TYPE_BLOB:
	      case MYSQL_TYPE_MEDIUM_BLOB:
	      case MYSQL_TYPE_LONG_BLOB:	vdType = VDTYPE_Char ; l = xdb->stmts[sx].fld->length ; did = Dim_Alpha ; break ;
	      case MYSQL_TYPE_BIT:		vdType = VDTYPE_b1SInt ; l = xdb->stmts[sx].fld->length ; did = Dim_Logical ; break ;
	    } ;
	   xdb->stmts[sx].bind[i].buffer_length = l ; xdb->stmts[sx].col[i].vdType = vdType ;
	   xdb->stmts[sx].bind[i].is_null = &xdb->stmts[sx].col[i].isNull ;
	   xdb->stmts[sx].bind[i].length = &xdb->stmts[sx].col[i].length ; xdb->stmts[sx].col[i].length = l ;
	   xdb->stmts[sx].bind[i].error = &xdb->stmts[sx].col[i].isErr ;
	   colOffsets[i] = offset ;
	   offset += l ;
	 } ;
	memset(&xdb->stmts[sx].bind[i], 0, sizeof xdb->stmts[sx].bind[i]) ;		/* Zero out next (empty) one */
/*	Do we already have big enough buffer? If not then reallocate for what we need */
	if (xdb->stmts[sx].dataLen < offset)
	 { if (xdb->stmts[sx].data != NULL) v4mm_FreeChunk(xdb->stmts[sx].data) ;
	   if (offset < V4MYSQL_DFLT_DATALEN) offset = V4MYSQL_DFLT_DATALEN ;
	   xdb->stmts[sx].data = (BYTE *)v4mm_AllocChunk(offset,FALSE) ;
	   xdb->stmts[sx].dataLen = offset ;
	 } ;
/*	Now make another pass through the bind[] array and set proper data point for each column */
	for(i=0;i<xdb->stmts[sx].colCount;i++)
	 { xdb->stmts[sx].bind[i].buffer = (xdb->stmts[sx].data + colOffsets[i]) ;
	 } ;
	if (mysql_stmt_bind_result(xdb->stmts[sx].stmt,xdb->stmts[sx].bind) != 0)
	 { v_Msg(ctx,NULL,"XDBBindErr",intmodx,mysql_errno(xdb->con[cx].mysql),sqlstmt,ASCretUC((char *)mysql_error(xdb->con[cx].mysql))) ; goto fail ; } ;
	v4xdb_NewStmtFinish(ctx,xdb,sx,xdbId) ;
	return(sx) ;		/* Return index of this statement */

fail:	FREEMTLOCK(xdb->xdbLock) ;
	if (gpi->xdb != NULL)  { ctx->ErrorMsg[UCsizeof(gpi->xdbLastError)-1] = UCEOS ; UCstrcpy(gpi->xdbLastError,ctx->ErrorMsg) ; } ;
	if (trace & V4TRACE_XDB)
	 { v_Msg(ctx,UCTBUF1,"XDBtFail",intmodx,ctx->ErrorMsg) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
	return(UNUSED) ;
}

#endif

#ifdef WANTORACLE
INDEX v4oracle_NewStmt(ctx,hxdbId,sqlstmt,intmodx,trace,nest)
  struct V4C__Context *ctx ;
  XDBID hxdbId ;
  UCCHAR *sqlstmt ;
  INTMODX intmodx ; VTRACE trace ;
  COUNTER nest ;		/* If > 0 then number of times allowed to nest with connection reset in-between */
{ struct V4XDB__Master *xdb ;
  INDEX i ; ETYPE vdType ; XDBID xdbId ;
  INDEX cx,sx ;

	if ((xdb = gpi->xdb) == NULL) { v_Msg(ctx,NULL,"XDBNoCon",intmodx) ; goto fail ; } ;
	if (!v4xdb_NewStmtInitCommon(ctx,xdb,hxdbId,XDBORACLE,sqlstmt,intmodx,&cx,&sx,&xdbId,trace)) goto fail ;
	gpi->xdb->lastCX = cx ;

	xdb->stmts[sx].st = OCI_StatementCreate(gpi->xdb->con[cx].oracle) ;	/* Create statement */
	if (!OCI_Prepare(xdb->stmts[sx].st,UCretASC(sqlstmt)))
	 { v_Msg(ctx,NULL,"XDBPrepErr",intmodx,gpi->xdb->con[cx].lastErrCode,sqlstmt,gpi->xdb->con[cx].lastErrMsg) ; goto fail ; } ;
	if (!OCI_Execute(xdb->stmts[sx].st))
	 { v_Msg(ctx,NULL,"XDBPrepErr",intmodx,gpi->xdb->con[cx].lastErrCode,sqlstmt,gpi->xdb->con[cx].lastErrMsg) ; goto fail ; } ;
	xdb->stmts[sx].rs = OCI_GetResultset(xdb->stmts[sx].st) ;
 /*	Determining column info */
	xdb->stmts[sx].colCount = OCI_GetColumnCount(xdb->stmts[sx].rs) ;
	for(i=0;i<xdb->stmts[sx].colCount;i++)
	 { OCI_Column *col = OCI_GetColumn(xdb->stmts[sx].rs,i+1) ;
	   xdb->stmts[sx].col[i].ociCDT = OCI_ColumnGetType(col) ;
	   switch (xdb->stmts[sx].col[i].ociCDT)
	    { default:				v_Msg(ctx,NULL,"XDBUnkSQLType",intmodx,xdb->stmts[sx].col[i].ociCDT) ; goto fail ;
	      case OCI_CDT_NUMERIC:		vdType = VDTYPE_ORACLE_NUMERIC ; break ;	
	      case OCI_CDT_TIMESTAMP:
	      case OCI_CDT_DATETIME:		vdType = VDTYPE_ORACLE_TS ; break ;
	      case OCI_CDT_TEXT:		vdType = VDTYPE_ORACLE_TEXT ; break ;
//	      case OCI_CDT_LONG:
//	      case OCI_CDT_CURSOR : OCI_Statement *
//	      case OCI_CDT_LOB : OCI_Lob *
//	      case OCI_CDT_FILE : OCI_File *
//	      case OCI_CDT_INTERVAL : OCI_Interval *
//	      case OCI_CDT_RAW : void *
//	      case OCI_CDT_OBJECT : OCI_Object *
//	      case OCI_CDT_COLLECTION : OCI_Coll *
//	      case OCI_CDT_REF : OCI_Ref *
	    } ;
	   xdb->stmts[sx].col[i].vdType = vdType ;
	 } ;
	v4xdb_NewStmtFinish(ctx,xdb,sx,xdbId) ;
	return(sx) ;		/* Return index of this statement */

fail:	FREEMTLOCK(xdb->xdbLock) ;
	if (gpi->xdb != NULL)  { ctx->ErrorMsg[UCsizeof(gpi->xdbLastError)-1] = UCEOS ; UCstrcpy(gpi->xdbLastError,ctx->ErrorMsg) ; } ;
	if (trace & V4TRACE_XDB)
	 { v_Msg(ctx,UCTBUF1,"XDBtFail",intmodx,ctx->ErrorMsg) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
	return(UNUSED) ;
}

#endif

#ifdef WANTODBC
P *v4odbc_Xct(ctx,respnt,autoclose,xdbId,sqlstmt,retval,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt ;
  XDBID xdbId ;
  LOGICAL autoclose ;
  UCCHAR *sqlstmt ;
  enum DictionaryEntries retval ;
  INTMODX intmodx ; VTRACE trace ;
{ struct V4XDB__Master *xdb ;
  RETCODE rc ;
  HSTMT hstmt ;
  INDEX cx ;
  UCCHAR szSQLState[6] ;

	if (trace & V4TRACE_XDB)
	 { v_Msg(ctx,UCTBUF1,"XDBtNewStmt",intmodx,sqlstmt) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
	if ((xdb = gpi->xdb) == NULL) { v_Msg(ctx,NULL,"XDBNoCon",intmodx) ; goto fail ; } ;
	for (cx=0;cx<xdb->conCount;cx++) { if (xdbId == xdb->con[cx].xdbId) break ; } ;
	if (cx >= xdb->conCount) { v_Msg(ctx,NULL,"XDBConHandle",intmodx) ; goto fail ; } ;
	if (xdb->stmtCount >= V4XDB_StmtMax) { v_Msg(ctx,NULL,"XDBMaxStmnt",intmodx,V4XDB_StmtMax) ; goto fail ; } ;
	rc = SQLAllocStmt(xdb->con[cx].hdbc,&hstmt) ;
	if (rc == SQL_ERROR) { v_Msg(ctx,NULL,"ODBCNoHstmt",intmodx) ; goto fail ; } ;
	rc = UCSQLExecDirect(hstmt,sqlstmt,SQL_NTS) ;
	if (rc != SQL_SUCCESS)
	 { UCSQLGetDiagRec(SQL_HANDLE_STMT,hstmt,1,szSQLState,NULL,ctx->ErrorMsgAux,1000,NULL) ;
	   v_Msg(ctx,NULL,"ODBCXDirectErr",intmodx,szSQLState,sqlstmt) ; goto fail ;
	 } ;
	switch (retval)
	 { default:
		logPNTv(respnt,TRUE) ; break ;
	   case DE(Rows):	/* Number of rows */
		rc = SQLRowCount(hstmt,&xdb->stmts[xdb->stmtCount].rowCnt) ;
		if (rc == SQL_ERROR)
		 { UCSQLGetDiagRec(SQL_HANDLE_STMT,hstmt,1,szSQLState,NULL,ctx->ErrorMsgAux,1000,NULL) ;
		   v_Msg(ctx,NULL,"ODBCRowCountErr",intmodx,szSQLState) ; goto fail ;
		 } ;
		intPNTv(respnt,xdb->stmts[xdb->stmtCount].rowCnt) ;
		if (trace & V4TRACE_XDB)
		 { v_Msg(ctx,UCTBUF1,"XDBtRows",intmodx,xdb->stmts[xdb->stmtCount].rowCnt) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
		break ;
	   case DE(Key):	/* Key field */
		{ INDEX sx = v4odbc_NewStmt(ctx,xdbId,UClit("Select Last_Insert_Id()"),intmodx,trace) ;
		  if (sx == UNUSED) { intPNTv(respnt,0) ;	}		/* Return 0 if cannot do it */
		   else { LOGICAL ok ;
			  xdb->stmts[sx].targetDimId = UNUSED ;
			  sx = v4odbc_Fetch(ctx,xdb->stmts[sx].xdbId,V4ODBC_FetchNext,&ok,0,respnt) ;
			  if (!ok || sx < 0 || xdb->stmts[sx].col[0].vdType != VDTYPE_b8FP) { intPNTv(respnt,0) ; }
			   else { double *ddata = (double *)(xdb->stmts[sx].data+xdb->stmts[sx].col[0].Offset) ;
				  intPNTv(respnt,(int)*ddata) ;
			        } ;
		        } ;
		}
		if (trace & V4TRACE_XDB)
		 { v_Msg(ctx,UCTBUF1,"XDBtKeyVal",intmodx,respnt->Value.IntVal) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
	 } ;
	rc = SQLEndTran(SQL_HANDLE_DBC,xdb->con[cx].hdbc,SQL_COMMIT) ;
	if (rc != SQL_SUCCESS)
	 { UCSQLGetDiagRec(SQL_HANDLE_STMT,hstmt,1,szSQLState,NULL,ctx->ErrorMsgAux,1000,NULL) ;
	   v_Msg(ctx,NULL,"ODBCEndTranErr",intmodx,szSQLState) ; goto fail ;
	 } ;
	SQLFreeStmt(hstmt,SQL_DROP) ;
	if (autoclose) v4xdb_FreeStuff(ctx,xdbId,0) ;
	return(respnt) ;

fail:	if (autoclose) v4xdb_FreeStuff(ctx,xdbId,0) ;
	if (gpi->xdb != NULL)  { ctx->ErrorMsg[UCsizeof(gpi->xdbLastError)-1] = UCEOS ; UCstrcpy(gpi->xdbLastError,ctx->ErrorMsg) ; } ;
	if (trace & V4TRACE_XDB)
	 { v_Msg(ctx,UCTBUF1,"XDBtFail",intmodx,ctx->ErrorMsg) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
	REGISTER_ERROR(0) ; RETURNFAILURE ;
}
#endif

#ifdef WANTMYSQL

P *v4mysql_Xct(ctx,respnt,autoclose,xdbId,sqlstmt,retval,intmodx,trace,nest)
  struct V4C__Context *ctx ;
  P *respnt ;
  XDBID xdbId ;
  LOGICAL autoclose ;
  UCCHAR *sqlstmt ;
  enum DictionaryEntries retval ;
  INTMODX intmodx ; VTRACE trace ;
  COUNTER nest ;		/* If > 0 then number of times allowed to nest with connection reset in-between */
{ struct V4XDB__Master *xdb ;
  INDEX cx,retry ; LOGICAL ok ; int mysqlErr ;

	if (trace & V4TRACE_XDB)
	 { v_Msg(ctx,UCTBUF1,"XDBtNewStmt",intmodx,sqlstmt) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
	if ((xdb = gpi->xdb) == NULL) { v_Msg(ctx,NULL,"XDBNoCon",intmodx) ; goto fail ; } ;
	for (cx=0;cx<xdb->conCount;cx++) { if (xdbId == xdb->con[cx].xdbId) break ; } ;
	if (cx >= xdb->conCount) { v_Msg(ctx,NULL,"XDBConHandle",intmodx) ; goto fail ; } ;
	if (xdb->stmtCount >= V4XDB_StmtMax) { v_Msg(ctx,NULL,"XDBMaxStmnt",intmodx,V4XDB_StmtMax) ; goto fail ; } ;
	for(retry=1;retry<=5;retry++)
	 { ok = (mysql_query(xdb->con[cx].mysql,UCretASC(sqlstmt)) == 0) ;
	   if (ok) break ;
	   mysqlErr = mysql_errno(xdb->con[cx].mysql) ;
	   switch(mysqlErr)
	    { default:
		break ;
	      case 1205:			/* Deadlock */
	      case 1206:			/* Lock table full */
	      case 1213:			/* 1213 - ER_LOCK_DEADLOCK Deadlock found when trying to lock */
	      case 1223:			/* Conflicting read lock */
		HANGLOOSE(500) ; continue ;
	    } ;
	   break ;
	 } ;
	if (!ok)
	 { if (nest > 0 && mysqlErr == 2014)		 /* 2014 - CR_COMMANDS_OUT_OF_SYNC  */
	    { struct V4DPI__DimInfo *di ; LOGICAL ok ; COUNTER xdbId2 ;
	      UCCHAR *host, *dsn, *user, *password ; LOGICAL autocom ;
	      DIMINFO(di,ctx,xdb->con[cx].DimId) ; autocom = xdb->con[cx].autocom ;
#define CCI(NAME) if (xdb->con[cx].NAME == NULL) { NAME = NULL ; } else { NAME = v4mm_AllocUC(UCstrlen(xdb->con[cx].NAME)) ; UCstrcpy(NAME,xdb->con[cx].NAME) ; } ;
	      CCI(host) CCI(dsn) CCI(user) CCI(password) ;
	      v_Msg(ctx,UCTBUF1,"XDBSynch",sqlstmt) ; vout_UCText(VOUT_Warn,0,UCTBUF1) ;
	      v4mysql_FreeStuff(ctx,xdbId,0) ;			/* Close off connection */
/*	      Re-open connection */
	      xdbId2 = v4mysql_Connect(ctx,di,intmodx,host,(xdb->con[cx].port == UNUSED ? 0 : xdb->con[cx].port),dsn,user,password,autocom,&ok) ;
/*	      And try again */
	      if (ok) return(v4mysql_Xct(ctx,respnt,autoclose,xdbId2,sqlstmt,retval,intmodx,trace,nest-1)) ;
	    } ;
	   v_Msg(ctx,NULL,"XDBXctErr",intmodx,mysql_errno(xdb->con[cx].mysql),ASCretUC((char *)mysql_error(xdb->con[cx].mysql)),sqlstmt) ; goto fail ;
	 } ;
	switch (retval)				/* Return the number of rows? */
	 { default:
		logPNTv(respnt,TRUE) ; break ;
	   case DE(Rows):
		intPNTv(respnt,mysql_affected_rows(xdb->con[cx].mysql)) ;
		if (trace & V4TRACE_XDB)
		 { v_Msg(ctx,UCTBUF1,"XDBtRows",intmodx,respnt->Value.IntVal) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
		break ;
	   case DE(Key):
		intPNTv(respnt,mysql_insert_id(xdb->con[cx].mysql)) ;
		if (trace & V4TRACE_XDB)
		 { v_Msg(ctx,UCTBUF1,"XDBtKeyVal",intmodx,respnt->Value.IntVal) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
		break ;
	 } ;
	if (autoclose) v4xdb_FreeStuff(ctx,xdbId,0) ;
	return(respnt) ;

fail:	if (autoclose) v4xdb_FreeStuff(ctx,xdbId,0) ;
	if (gpi->xdb != NULL)  { ctx->ErrorMsg[UCsizeof(gpi->xdbLastError)-1] = UCEOS ; UCstrcpy(gpi->xdbLastError,ctx->ErrorMsg) ; } ;
	if (trace & V4TRACE_XDB)
	 { v_Msg(ctx,UCTBUF1,"XDBtFail",intmodx,ctx->ErrorMsg) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
	REGISTER_ERROR(0) ; RETURNFAILURE ;
}
#endif

#ifdef WANTORACLE

P *v4oracle_Xct(ctx,respnt,autoclose,xdbId,sqlstmt,retval,intmodx,trace,nest)
  struct V4C__Context *ctx ;
  P *respnt ;
  XDBID xdbId ;
  LOGICAL autoclose ;
  UCCHAR *sqlstmt ;
  enum DictionaryEntries retval ;
  INTMODX intmodx ; VTRACE trace ;
  COUNTER nest ;		/* If > 0 then number of times allowed to nest with connection reset in-between */
{ struct V4XDB__Master *xdb ;
  OCI_Statement *st ;
  INDEX cx ;

	if (trace & V4TRACE_XDB)
	 { v_Msg(ctx,UCTBUF1,"XDBtNewStmt",intmodx,sqlstmt) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
	if ((xdb = gpi->xdb) == NULL) { v_Msg(ctx,NULL,"XDBNoCon",intmodx) ; goto fail ; } ;
	for (cx=0;cx<xdb->conCount;cx++) { if (xdbId == xdb->con[cx].xdbId) break ; } ;
	if (cx >= xdb->conCount) { v_Msg(ctx,NULL,"XDBConHandle",intmodx) ; goto fail ; } ;
	if (xdb->stmtCount >= V4XDB_StmtMax) { v_Msg(ctx,NULL,"XDBMaxStmnt",intmodx,V4XDB_StmtMax) ; goto fail ; } ;
	gpi->xdb->lastCX = cx ;

	st = OCI_StatementCreate(gpi->xdb->con[cx].oracle) ;	/* Create statement */
	if (!OCI_Prepare(st,UCretASC(sqlstmt)))
	 { v_Msg(ctx,NULL,"XDBPrepErr",intmodx,gpi->xdb->con[cx].lastErrCode,sqlstmt,gpi->xdb->con[cx].lastErrMsg) ; goto fail ; } ;
	if (!OCI_Execute(st))
	 { v_Msg(ctx,NULL,"XDBPrepErr",intmodx,gpi->xdb->con[cx].lastErrCode,sqlstmt,gpi->xdb->con[cx].lastErrMsg) ; goto fail ; } ;

	switch (retval)				/* Return the number of rows? */
	 { default:
		logPNTv(respnt,TRUE) ; break ;
	   case DE(Rows):
		intPNTv(respnt,OCI_GetAffectedRows(st)) ;
		if (trace & V4TRACE_XDB)
		 { v_Msg(ctx,UCTBUF1,"XDBtRows",intmodx,respnt->Value.IntVal) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
		break ;
	   case DE(Key):
//		intPNTv(respnt,oracle_insert_id(xdb->con[cx].mysql)) ;		/* This is a problem !!! */
		intPNTv(respnt,-9999) ;		/* This is a problem !!! */
		if (trace & V4TRACE_XDB)
		 { v_Msg(ctx,UCTBUF1,"XDBtKeyVal",intmodx,respnt->Value.IntVal) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
		break ;
	 } ;
	if (autoclose) v4xdb_FreeStuff(ctx,xdbId,0) ;
	return(respnt) ;

fail:	if (autoclose) v4xdb_FreeStuff(ctx,xdbId,0) ;
	if (gpi->xdb != NULL)  { ctx->ErrorMsg[UCsizeof(gpi->xdbLastError)-1] = UCEOS ; UCstrcpy(gpi->xdbLastError,ctx->ErrorMsg) ; } ;
	if (trace & V4TRACE_XDB)
	 { v_Msg(ctx,UCTBUF1,"XDBtFail",intmodx,ctx->ErrorMsg) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
	REGISTER_ERROR(0) ; RETURNFAILURE ;
}
#endif

#ifdef WANTODBC
/*	v4odbc_Fetch - Fetches (or sets internal pointers) (next) row in current table */
/*	Call sx = v4odbc_Fetch( ctx , xdbId , row , ok , intmodx , respnt)
	  where sx = xdb statement index (UNUSED if no more data),
		ctx is context,
		xdbId is statement Id,
		row is requested row number, V4ODBC_FetchNext for next row, V4ODBC_FetchFirst/Last for first last rows,
		*ok is updated to TRUE if OK (and on normal EOT), FALSE if error (ctx->ErrorMsgAux),
		intmodx is calling module,
		respnt is resulting point that may be updated by evaluation of fetchPnt
*/

INDEX v4odbc_Fetch(ctx,xdbId,row,ok,intmodx,respnt)
  struct V4C__Context *ctx ;
  XDBID xdbId ;
  XDBROW row ;
  INTMODX intmodx ; LOGICAL *ok ;
  P *respnt ;
{ struct V4XDB__Master *xdb ;
  RETCODE rc ;
  UCCHAR szSQLState[6] ;
  int sx ;

	if ((xdb = gpi->xdb) == NULL) { v_Msg(ctx,NULL,"XDBNoCon",intmodx) ; *ok = FALSE ; return(0) ; } ;
	for(sx=0;sx<xdb->stmtCount;sx++) { if (xdb->stmts[sx].xdbId == xdbId) break ; } ;
	if (sx >= xdb->stmtCount) { v_Msg(ctx,NULL,"ODBCHstmtBad",intmodx) ; *ok = FALSE ; return(0) ; } ;

/*	Before we get next row, should we save current row ? */
	XDBSAVECURROW(xdb,sx) ;
	switch (row)
	 { default:				/* Wants a particular row */
		v_Msg(ctx,NULL,"ODBCNYI1",intmodx) ; *ok = FALSE ; return(0) ;
	   case V4ODBC_FetchNext:
		rc = SQLFetch(xdb->stmts[sx].hstmt) ;
		xdb->stmts[sx].curRow ++ ;

  if ((xdb)->stmts[sx].targetDimId == UNUSED) { (xdb)->stmts[sx].curRecId++ ; }
   else { struct V4XDB__SaveStmtInfo *vxssi = (xdb)->stmts[sx].vxssi ;
	  if (vxssi->valCol == UNUSED) { struct V4DPI__DimInfo *di ; DIMINFO(di,ctx,(xdb)->stmts[sx].targetDimId) ; (xdb)->stmts[sx].curRecId = (++di->XDB.lastRecId) ; }
	   else { struct V4DPI__LittlePoint lpnt ;
		  v4db_GetCurColVal(ctx,sx,vxssi->valCol,(xdb)->stmts[sx].targetDimId,(P *)&lpnt) ; (xdb)->stmts[sx].curRecId = lpnt.Value.IntVal ;
		} ;
	} ;


		break ;
	   case V4ODBC_FetchFirst:
	   case V4ODBC_FetchLast:
		v_Msg(ctx,NULL,"ODBCNYI2",intmodx) ; *ok = FALSE ; return(0) ;
	   case V4ODBC_FetchEOF:
		rc = SQL_NO_DATA_FOUND ; break ;	/* Force end-of-data */
	 } ;
	if (rc == SQL_NO_DATA_FOUND)		/* End of line? */
	 { if (xdb->stmts[sx].data != NULL) { v4mm_FreeChunk(xdb->stmts[sx].data) ; xdb->stmts[sx].data = NULL ; } ;
	   XDBRMVTARGETDIM(sx) ;
	   if (xdb->stmts[sx].relConEOF)
	    { v4xdb_FreeStuff(ctx,xdb->stmts[sx].hxdbId,0) ;		/* Free connection (and all child statements) */
	    } else { v4xdb_FreeStuff(ctx,xdb->stmts[sx].xdbId,0) ; } ;	/* Just free this statement */
	   
	   if (xdb->stmts[sx].eofPnt != NULL)
	    { P epnt,*ipt ;
	      ipt = v4dpi_IsctEval(&epnt,xdb->stmts[sx].eofPnt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	      if (ipt == NULL)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"XDBNestEval",V4IM_Tag_End,xdb->stmts[sx].eofPnt,ctx->ErrorMsg) ;
	         v4mm_FreeChunk(xdb->stmts[sx].eofPnt) ; xdb->stmts[sx].eofPnt = NULL ;
	         *ok = FALSE ; return(0) ;
	       } ;
	    } ;
	   v4mm_FreeChunk(xdb->stmts[sx].eofPnt) ; xdb->stmts[sx].eofPnt = NULL ;
	   *ok = TRUE ; return(UNUSED) ;
	 } ;
	if (rc == SQL_ERROR)
	 { UCSQLGetDiagRec(SQL_HANDLE_STMT,xdb->stmts[sx].hstmt,1,szSQLState,NULL,ctx->ErrorMsgAux,1000,NULL) ;
	   v_Msg(ctx,NULL,"ODBCFetchErr2",intmodx,szSQLState) ;
	   *ok = FALSE ; return(0) ;
	 } ;
	if (xdb->stmts[sx].fetchPnt != NULL)
	 { P *ipt ; struct V4DPI__LittlePoint lpnt ; int cx ;		/* Put ODBC point in context & evaluate point */
	   for (cx=0;cx<xdb->conCount;cx++) { if (xdb->stmts[sx].hxdbId == xdb->con[cx].xdbId) break ; } ;
	   if (cx >= xdb->conCount) { v_Msg(ctx,ctx->ErrorMsgAux,"XDBConHandle",1) ; *ok = FALSE ; return(0) ; } ;
	   ZPH(&lpnt) ; lpnt.PntType = V4DPI_PntType_XDB ; lpnt.Dim = xdb->con[cx].DimId ;
	   lpnt.Bytes = V4PS_XDB ;
	   lpnt.Value.XDB.recId = xdb->stmts[sx].curRecId ; lpnt.Value.XDB.xdbId = xdbId ;
	   if (!v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,(P *)&lpnt,0,0)) { *ok = FALSE ; return(0) ; } ;
	   ipt = v4dpi_IsctEval(respnt,xdb->stmts[sx].fetchPnt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	   if (ipt == NULL) { *ok = FALSE ; return(0) ; } ;
	 } ;
	*ok = TRUE ; return(sx) ;
}
#endif

#ifdef WANTMYSQL

/*	v4mysql_Fetch - Fetches (or sets internal pointers) (next) row in current table */
/*	Call sx = v4mysql_Fetch( ctx , xdbId , row , ok , intmodx , respnt)
	  where res = statment index or UNUSED if end-of-table,
		ctx is context,
		xdbId is statement Id,
		row is requested row number, V4ODBC_FetchNext for next row, V4ODBC_FetchFirst/Last for first last rows,
		*ok is updated to TRUE if OK (and on normal EOT), FALSE if error (ctx->ErrorMsgAux),
		intmodx is calling module,
		respnt is resulting point that may be updated by evaluation of fetchPnt
*/

INDEX v4mysql_Fetch(ctx,xdbId,row,ok,intmodx,respnt)
  struct V4C__Context *ctx ;
  XDBID xdbId ;
  XDBROW row ;
  INTMODX intmodx ; LOGICAL *ok ;
  P *respnt ;
{ struct V4XDB__Master *xdb ;
  INDEX sx ; int rc ;

	if ((xdb = gpi->xdb) == NULL) { v_Msg(ctx,NULL,"XDBNoCon",intmodx) ; *ok = FALSE ; return(0) ; } ;
	for(sx=0;sx<xdb->stmtCount;sx++) { if (xdb->stmts[sx].xdbId == xdbId) break ; } ;
	if (sx >= xdb->stmtCount) { v_Msg(ctx,NULL,"ODBCHstmtBad",intmodx) ; *ok = FALSE ; return(0) ; } ;
	XDBSAVECURROW(xdb,sx) ;
	switch (row)
	 { default:				/* Wants a particular row */
		v_Msg(ctx,NULL,"ODBCNYI1",intmodx) ; *ok = FALSE ; return(0) ;
	   case V4ODBC_FetchNext:
		rc = mysql_stmt_fetch(xdb->stmts[sx].stmt) ;
		xdb->stmts[sx].curRow++ ; /* mysql_row_tell(MYSQL_RES *result) also returns the row */
		GETNEXTRECID(xdb,sx) ; break ;
	   case V4ODBC_FetchFirst:
	   case V4ODBC_FetchLast:
		v_Msg(ctx,NULL,"ODBCNYI2",intmodx) ; *ok = FALSE ; return(0) ;
	   case V4ODBC_FetchEOF:
		rc = MYSQL_NO_DATA ; break ;				/* Force end-of-data */
	 } ;
	if (rc == 1)
	 { INDEX cx ;
	   for(cx=0;cx<xdb->conCount;cx++)
	    { if (xdb->stmts[sx].hxdbId == xdb->con[cx].xdbId) break ; } ;
	   v_Msg(ctx,ctx->ErrorMsgAux,"XDBNextErr",intmodx,mysql_errno(xdb->con[cx].mysql),xdb->stmts[sx].curRow,ASCretUC((char *)mysql_error(xdb->con[cx].mysql))) ;
	   *ok = FALSE ; return(0) ;
	 } ;
//	if (rc == MYSQL_NO_DATA || rc == MYSQL_DATA_TRUNCATED)		/* End of line? */
	if (rc == MYSQL_NO_DATA)					/* End of line? */
	 { 
	   XDBRMVTARGETDIM(sx) ;
	   if (xdb->stmts[sx].relConEOF)
	    { v4xdb_FreeStuff(ctx,xdb->stmts[sx].hxdbId,0) ;		/* Free connection (and all child statements) */
	    } else { v4xdb_FreeStuff(ctx,xdb->stmts[sx].xdbId,0) ; } ;	/* Just free this statement */
	   if (xdb->stmts[sx].eofPnt != NULL)
	    { P epnt,*ipt ;
	      ipt = v4dpi_IsctEval(&epnt,xdb->stmts[sx].eofPnt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	      if (ipt == NULL)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"XDBNestEval",V4IM_Tag_End,xdb->stmts[sx].eofPnt,ctx->ErrorMsg) ;
	         v4mm_FreeChunk(xdb->stmts[sx].eofPnt) ; xdb->stmts[sx].eofPnt = NULL ;
	         *ok = FALSE ; return(0) ;
	       } ;
	    } ;
	   v4mm_FreeChunk(xdb->stmts[sx].eofPnt) ; xdb->stmts[sx].eofPnt = NULL ;
	   *ok = TRUE ; return(UNUSED) ;
	 } ;
	if (xdb->stmts[sx].fetchPnt != NULL)
	 { P *ipt ; struct V4DPI__LittlePoint lpnt ; int cx ;		/* Put ODBC point in context & evaluate point */
	   for (cx=0;cx<xdb->conCount;cx++) { if (xdb->stmts[sx].hxdbId == xdb->con[cx].xdbId) break ; } ;
	   if (cx >= xdb->conCount) { v_Msg(ctx,ctx->ErrorMsgAux,"XDBConHandle",1) ; *ok = FALSE ; return(0) ; } ;
	   ZPH(&lpnt) ; lpnt.PntType = V4DPI_PntType_XDB ; lpnt.Dim = xdb->con[cx].DimId ;
	   lpnt.Bytes = V4PS_XDB ;
	   lpnt.Value.XDB.recId = xdb->stmts[sx].curRecId ; lpnt.Value.XDB.xdbId = xdbId ;
	   if (!v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,(P *)&lpnt,0,0)) { *ok = FALSE ; return(0) ; } ;
	   ipt = v4dpi_IsctEval(respnt,xdb->stmts[sx].fetchPnt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	   if (ipt == NULL) { *ok = FALSE ; return(0) ; } ;
	 } ;
	*ok = TRUE ; return(sx) ;
}
#endif

#ifdef WANTORACLE

/*	v4oracle_Fetch - Fetches (or sets internal pointers) (next) row in current table */
/*	Call sx = v4oracle_Fetch( ctx , xdbId , row , ok , intmodx , respnt)
	  where res = statment index or UNUSED if end-of-table,
		ctx is context,
		xdbId is statement Id,
		row is requested row number, V4ODBC_FetchNext for next row, V4ODBC_FetchFirst/Last for first last rows,
		*ok is updated to TRUE if OK (and on normal EOT), FALSE if error (ctx->ErrorMsgAux),
		intmodx is calling module,
		respnt is resulting point that may be updated by evaluation of fetchPnt
*/

INDEX v4oracle_Fetch(ctx,xdbId,row,ok,intmodx,respnt)
  struct V4C__Context *ctx ;
  XDBID xdbId ;
  XDBROW row ;
  INTMODX intmodx ; LOGICAL *ok ;
  P *respnt ;
{ struct V4XDB__Master *xdb ;
  INDEX sx ; LOGICAL ook ;

	if ((xdb = gpi->xdb) == NULL) { v_Msg(ctx,NULL,"XDBNoCon",intmodx) ; *ok = FALSE ; return(0) ; } ;
	for(sx=0;sx<xdb->stmtCount;sx++) { if (xdb->stmts[sx].xdbId == xdbId) break ; } ;
	if (sx >= xdb->stmtCount) { v_Msg(ctx,NULL,"ODBCHstmtBad",intmodx) ; *ok = FALSE ; return(0) ; } ;
	XDBSAVECURROW(xdb,sx) ;
	switch (row)
	 { default:				/* Wants a particular row */
		v_Msg(ctx,NULL,"ODBCNYI1",intmodx) ; *ok = FALSE ; return(0) ;
	   case V4ODBC_FetchNext:
		ook = OCI_FetchNext(xdb->stmts[sx].rs) ;
		xdb->stmts[sx].curRow++ ; /* mysql_row_tell(MYSQL_RES *result) also returns the row */
		GETNEXTRECID(xdb,sx) ; break ;
	   case V4ODBC_FetchFirst:
	   case V4ODBC_FetchLast:
		v_Msg(ctx,NULL,"ODBCNYI2",intmodx) ; *ok = FALSE ; return(0) ;
	   case V4ODBC_FetchEOF:
		ook = FALSE ; break ;
	 } ;
	if (!ook)				/* End of line? */
	 { 
	   XDBRMVTARGETDIM(sx) ;
	   if (xdb->stmts[sx].relConEOF)
	    { v4xdb_FreeStuff(ctx,xdb->stmts[sx].hxdbId,0) ;		/* Free connection (and all child statements) */
	    } else { v4xdb_FreeStuff(ctx,xdb->stmts[sx].xdbId,0) ; } ;	/* Just free this statement */
	   if (xdb->stmts[sx].eofPnt != NULL)
	    { P epnt,*ipt ;
	      ipt = v4dpi_IsctEval(&epnt,xdb->stmts[sx].eofPnt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	      if (ipt == NULL)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"XDBNestEval",V4IM_Tag_End,xdb->stmts[sx].eofPnt,ctx->ErrorMsg) ;
	         v4mm_FreeChunk(xdb->stmts[sx].eofPnt) ; xdb->stmts[sx].eofPnt = NULL ;
	         *ok = FALSE ; return(0) ;
	       } ;
	    } ;
	   v4mm_FreeChunk(xdb->stmts[sx].eofPnt) ; xdb->stmts[sx].eofPnt = NULL ;
	   *ok = TRUE ; return(UNUSED) ;
	 } ;
	if (xdb->stmts[sx].fetchPnt != NULL)
	 { P *ipt ; struct V4DPI__LittlePoint lpnt ; int cx ;		/* Put ODBC point in context & evaluate point */
	   for (cx=0;cx<xdb->conCount;cx++) { if (xdb->stmts[sx].hxdbId == xdb->con[cx].xdbId) break ; } ;
	   if (cx >= xdb->conCount) { v_Msg(ctx,ctx->ErrorMsgAux,"XDBConHandle",1) ; *ok = FALSE ; return(0) ; } ;
	   ZPH(&lpnt) ; lpnt.PntType = V4DPI_PntType_XDB ; lpnt.Dim = xdb->con[cx].DimId ;
	   lpnt.Bytes = V4PS_XDB ;
	   lpnt.Value.XDB.recId = xdb->stmts[sx].curRecId ; lpnt.Value.XDB.xdbId = xdbId ;
	   if (!v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,(P *)&lpnt,0,0)) { *ok = FALSE ; return(0) ; } ;
	   ipt = v4dpi_IsctEval(respnt,xdb->stmts[sx].fetchPnt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	   if (ipt == NULL) { *ok = FALSE ; return(0) ; } ;
	 } ;
	*ok = TRUE ; return(sx) ;
}
#endif

#if defined WANTODBC || defined WANTMYSQL || defined WANTORACLE
P *v4db_Info(ctx,respnt,intmodx,argpnts,argcnt,trace)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  P *respnt,*argpnts[] ;
  int argcnt ;
  int trace ;
{ struct V4XDB__Master *xdb ;
  P *cpt,pnt ;
  int t,col,res ; LOGICAL ok ; DIMID dimId ; INDEX ix,cx,sx ;

	if ((xdb = gpi->xdb) == NULL) { v_Msg(ctx,NULL,"XDBNoCon",intmodx) ; goto fail ; } ;
	col = 0 ; res = 0 ; cx = UNUSED ; dimId = UNUSED ; sx = UNUSED ;
	for(ok=TRUE,ix=1;ok&&ix<=argcnt;ix++)
	 {
	   switch (t=v4im_CheckPtArgNew(ctx,argpnts[ix],&cpt,&pnt))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:		v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	      case V4IM_Tag_Dim:		for(cx=0;cx<gpi->xdb->conCount;cx++)
						 { if (gpi->xdb->con[cx].DimId == cpt->Value.IntVal && gpi->xdb->con[cx].xdbId != UNUSED) break ; } ;
						if (cx >= gpi->xdb->conCount) { v_Msg(ctx,NULL,"XDBDimBad",intmodx,ix,cpt) ; goto fail ; } ;
						dimId = cpt->Value.IntVal ; break ;
	      case V4IM_Tag_Connection:		for(cx=0;cx<gpi->xdb->conCount;cx++)
						 { if (gpi->xdb->con[cx].xdbId != UNUSED && memcmp(cpt,&gpi->xdb->con[cx].idPnt,gpi->xdb->con[cx].idPnt.Bytes) == 0) break ; } ;
						if (cx >= gpi->xdb->conCount) { v_Msg(ctx,NULL,"XDBDimBad",intmodx,ix,cpt) ; return(NULL) ; } ;
						break ;
	      case V4IM_Tag_Statement:		for(sx=0;sx<gpi->xdb->stmtCount;sx++)
						 { if (gpi->xdb->stmts[sx].xdbId != UNUSED && memcmp(cpt,&gpi->xdb->stmts[sx].idPnt,gpi->xdb->stmts[sx].idPnt.Bytes) == 0) break ; } ;
						if (sx >= gpi->xdb->stmtCount) { v_Msg(ctx,NULL,"XDBDimBad",intmodx,ix,cpt) ; return(NULL) ; } ;
						for(cx=0;cx<gpi->xdb->conCount;cx++)	/* Get corresponding connection */
						 { if (gpi->xdb->con[cx].xdbId == gpi->xdb->stmts[sx].hxdbId) break ; } ;
						break ;
	      case -V4IM_Tag_Connection:
	      case -V4IM_Tag_Data:		alphaPNTv(respnt,xdb->extraData) ; return(respnt) ;
	      case -V4IM_Tag_ColCap:
	      case -V4IM_Tag_Columns:
	      case -V4IM_Tag_Length:
	      case -V4IM_Tag_Precision:
	      case -V4IM_Tag_Scale:
	      case -V4IM_Tag_Table:
	      case -V4IM_Tag_Type:
		if (res != 0) { v_Msg(ctx,NULL,"ODBCColInfo",intmodx,ix) ; goto fail ; } ;
		res = -t ; break ;
	      case V4IM_Tag_Column:		col = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ix-1) ; goto fail ; } ;

	if (cx == UNUSED)
	 { v_Msg(ctx,NULL,"XDBConHandle",intmodx) ; goto fail ; } ;

	if (res == V4IM_Tag_Connection)
	 { uccharPNTv(respnt,xdb->con[cx].ConnectionStr) ; return(respnt) ;
	 } ;

	switch (XDBGETDBACCESS(gpi->xdb->con[cx].xdbId))
	 {
#ifdef WANTODBC
	   case XDBODBC:
	     { SWORD nbytes,type,scale,null ; SQLULEN coldef ; RETCODE rc ; UCCHAR name[255]; P *ipt ;
	       UCCHAR szSQLState[6];
		for (sx=0;sx<xdb->stmtCount;sx++)
		 { if (xdb->stmts[sx].hxdbId == xdb->con[cx].xdbId) break ; } ;
		if (sx >= xdb->stmtCount) { v_Msg(ctx,NULL,"ODBCNoStmt",intmodx,xdb->con[cx].DimId) ; goto fail ; } ;
		if (res == V4IM_Tag_Columns)
		 { rc = SQLNumResultCols(xdb->stmts[sx].hstmt,&nbytes) ; }
		 else { rc = UCSQLDescribeCol(xdb->stmts[sx].hstmt,(SQLSMALLINT)col,name,UCsizeof(name),&nbytes,&type,&coldef,&scale,&null) ; } ;
		if (rc == SQL_ERROR)
		 { UCSQLGetDiagRec(SQL_HANDLE_STMT,xdb->stmts[sx].hstmt,1,szSQLState,NULL,ctx->ErrorMsgAux,1000,NULL) ;
		   v_Msg(ctx,NULL,"ODBCNumResErr",intmodx,szSQLState) ; goto fail ;
		 } ;
		ipt = respnt ; intPNT(ipt) ;
		switch(res)				/* Now return proper result */
		 {
		   case V4IM_Tag_ColCap:	uccharPNTv(ipt,name) ; break ;
		   case V4IM_Tag_Columns:	ipt->Value.IntVal = nbytes ; break ;	/* See above - did separate call for number of columns */
		   case V4IM_Tag_Length:	ipt->Value.IntVal = coldef ; break ;
		   case V4IM_Tag_Precision:	ipt->Value.IntVal = coldef ; break ;
		   case V4IM_Tag_Scale:		ipt->Value.IntVal = scale ; break ;
		   case V4IM_Tag_Type:		ipt->Value.IntVal = type ; break ;
		 } ;
		return(ipt) ;
	     }
#endif
#ifdef WANTV4IS
	   case XDBV4IS:
	      {	struct V4XDB_MIDASTable *vmt ;
		for (sx=0;sx<xdb->stmtCount;sx++)
		 { if (xdb->stmts[sx].hxdbId == xdb->con[cx].xdbId) break ; } ;
		if (sx >= xdb->stmtCount) { v_Msg(ctx,NULL,"ODBCNoStmt",intmodx,xdb->con[cx].DimId) ; goto fail ; } ;
		vmt = xdb->stmts[sx].vmt ;
		switch(res)				/* Now return proper result */
		 {
		   case V4IM_Tag_ColCap:	if (col < 1 || col > vmt->colCount) { v_Msg(ctx,NULL,"XDBInvColSpec",intmodx,col,vmt->colCount) ; goto fail ; } ;
						alphaPNTv(respnt,vmt->Col[col-1].Name) ; return(respnt) ;
		   case V4IM_Tag_Columns:	intPNTv(respnt,vmt->colCount) ; return(respnt) ;
		 } ;
	      }
		break ;

#endif
#ifdef WANTMYSQL
	   case XDBMYSQL:
		switch (res)
		 { 
		   case V4IM_Tag_Table:
			{ MYSQL_RES *res ; MYSQL_ROW row ; struct V4L__ListPoint *lp ;
			  res = mysql_list_tables(gpi->xdb->con[cx].mysql,NULL)  ;
			   if (res == NULL)
			    { v_Msg(ctx,NULL,"XDBMetaErr",intmodx,mysql_errno(xdb->con[cx].mysql),UClit(""),ASCretUC((char *)mysql_error(xdb->con[cx].mysql))) ; goto fail ; } ;
			  INITLP(respnt,lp,Dim_List)
			  for(;(row = mysql_fetch_row(res)) != NULL;)
			   { unsigned long *lengths = mysql_fetch_lengths(res) ;
			     strncpy(ASCTBUF1,row[0],lengths[0]) ; ASCTBUF1[lengths[0]] = '\0' ;
			     alphaPNTv(&pnt,ASCTBUF1) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&pnt,0) ;
			   } ;
			  ENDLP(respnt,lp)
			  mysql_free_result(res) ;
			  return(respnt) ;
			}
		 } ;
		break ;
#endif

#ifdef WANTORACLE
	   case XDBORACLE:
		gpi->xdb->lastCX = cx ;
		switch (res)
		 { 
		   case V4IM_Tag_Table:
			{ OCI_Statement *st ; OCI_Resultset *rs ; INDEX i ;
			  struct V4L__ListPoint *lp ;
			  st = OCI_StatementCreate(gpi->xdb->con[cx].oracle) ;
			  OCI_ExecuteStmt(st,"SELECT table_name FROM all_tables") ;
			  rs = OCI_GetResultset(st) ;
			  INITLP(respnt,lp,Dim_List)
			  for(i=1;OCI_FetchNext(rs);i++)
			   { alphaPNTv(&pnt,OCI_GetString(rs,i)) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&pnt,0) ;
			   } ;
			  ENDLP(respnt,lp)
			  return(respnt) ;
			}
		 } ;
		break ;
#endif
	 } ;


fail:	if (gpi->xdb != NULL)  { ctx->ErrorMsg[UCsizeof(gpi->xdbLastError)-1] = UCEOS ; UCstrcpy(gpi->xdbLastError,ctx->ErrorMsg) ; } ;
	REGISTER_ERROR(0) ; RETURNFAILURE ;
}
#endif


#ifdef WANTODBC
/*	v4odbc_SetupRowCol - Determines Column Type Info for Statement Column	*/
/*	Call: ok = v4odbc_SetupRowCol( ctx , xdb , i , column )
	  where ok is TRUE if went ok, FALSE if not (column not defined for this SQL statement), -999 if error,
		ctx is current context,
		xdb is ptr to xdb master,
		i is stmt index within xdb,
		column is column to be updated (0 is first column)					*/
LOGICAL v4odbc_SetupRowCol(ctx,xdb,i,column)
  struct V4C__Context *ctx ;
  struct V4XDB__Master *xdb ;
  int i,column ;
{
  RETCODE rc ;
  UCCHAR szSQLState[6] ;
  UCCHAR name[255] ; SWORD nbytes,type,scale,null ; SQLULEN coldef ;
  int c,l,v ;

/*	Get column information (add +1 to column index because we are 0-offset, ODBC is not) */
	rc = UCSQLDescribeCol(xdb->stmts[i].hstmt,(SQLSMALLINT)column+1,name,UCsizeof(name),&nbytes,&type,&coldef,&scale,&null) ;
	if (rc == SQL_ERROR || rc == SQL_INVALID_HANDLE)
	 { UCSQLGetDiagRec(SQL_HANDLE_STMT,xdb->stmts[i].hstmt,1,szSQLState,NULL,ctx->ErrorMsgAux,1000,NULL) ;
	   v_Msg(ctx,ctx->ErrorMsgAux,"ODBCNumResErr2",szSQLState) ;
	   return(column > 0 ? 0 : -999) ;		/* If not first column then assume error is that we are past last column in table */
	 } ;
	switch(type)
	 { 
	   default: v_Msg(ctx,ctx->ErrorMsgAux,"ODBCColType",column,name,type) ; return(-999) ;
	   case SQL_BIGINT:		l = sizeof(double) ; c = SQL_C_DOUBLE ; v = VDTYPE_b8FP ; break ;
	   case SQL_BINARY:
	   case SQL_BIT:		l = sizeof(int) ; c = SQL_C_SLONG ; v = VDTYPE_b4SInt ; break ;
	   case SQL_CHAR:		l = ALIGN(coldef+1) ; c = SQL_C_CHAR ; v = VDTYPE_Char ; break ;
	   case SQL_DATE:		l = sizeof(DATE_STRUCT) ; c = SQL_C_DATE ; v = VDTYPE_ODBC_DS ; break ;
	   case SQL_DECIMAL:		l = sizeof(double) ; c = SQL_C_DOUBLE ; v = VDTYPE_b8FP ; break ;
	   case SQL_DOUBLE:		l = sizeof(double) ; c = SQL_C_DOUBLE ; v = VDTYPE_b8FP ; break ;
	   case SQL_FLOAT:		l = sizeof(double) ; c = SQL_C_DOUBLE ; v = VDTYPE_b8FP ; break ;
	   case SQL_INTEGER:		l = sizeof(int) ; c = SQL_C_SLONG ; v = VDTYPE_b4SInt ; break ;
	   case SQL_LONGVARBINARY:
	   case SQL_LONGVARCHAR:	l = sizeof(UCCHAR) * ALIGN(V4LEX_BigText_Max) ; c = SQL_C_CHAR ; v = VDTYPE_Char ; break ;
	   case SQL_NUMERIC:		l = sizeof(SQL_NUMERIC_STRUCT) ; c = SQL_C_NUMERIC ; v = VDTYPE_ODBC_SNS ; break ;
	   case SQL_REAL:		l = sizeof(double) ; c = SQL_C_DOUBLE ; v = VDTYPE_b8FP ; break ;
	   case SQL_SMALLINT:		l = sizeof(int) ; c = SQL_C_SLONG ; v = VDTYPE_b2SInt ; break ;
	   case SQL_TIME:		l = sizeof(TIME_STRUCT) ; c = SQL_C_TIME ; v = VDTYPE_ODBC_TS ; break ;
	   case SQL_TIMESTAMP:		l = sizeof(TIMESTAMP_STRUCT) /* sizeof(int) */ ; c = SQL_C_TIMESTAMP ; v = VDTYPE_ODBC_TSS ; break ;
	   case SQL_TINYINT:		l = sizeof(int) ; c = SQL_C_SLONG ; v = VDTYPE_b4SInt ; break ;
	   case SQL_VARBINARY:
	   case SQL_VARCHAR:		l = ALIGN(coldef+1) ; c = SQL_C_CHAR ; v = VDTYPE_Char ; break ;
	   case SQL_WLONGVARCHAR:	l = sizeof(UCCHAR) * ALIGN(V4LEX_BigText_Max) ; c = SQL_C_WCHAR ; v = VDTYPE_UCChar ; break ;
	   case SQL_WVARCHAR:		l = sizeof(UCCHAR) * ALIGN(coldef+1) ; c = SQL_C_WCHAR ; v = VDTYPE_UCChar ; break ;
	   case SQL_WCHAR:		l = sizeof(UCCHAR) * ALIGN(coldef+1) ; c = SQL_C_WCHAR ; v = VDTYPE_UCChar ; break ;
	 } ;
	xdb->stmts[i].col[column].SQLCType = c ;
	xdb->stmts[i].col[column].length = l ;
	xdb->stmts[i].col[column].vdType = v ;
	return(TRUE) ;
}
#endif


/*	v4xdb_FreeStuff - Frees up XDB resources			*/
/*	Call: v4xdb_FreeStuff(ctx,xdbId,flags)
	  where ctx is current context,
		xdbId is id for connection (frees all statments under connection also) or statement,
		flags are optional processing flags (see XDB_FREE_xxx)	*/

/*	NOTE: v4xdb_FreeStuff MUST NOT move remaining connections or statements to other slots (multi-threading issues!) */

void v4xdb_FreeStuff(ctx,xdbId,flags)
  struct V4C__Context *ctx ;
  XDBID xdbId ;
  FLAGS32 flags ;
{
	switch (XDBGETDBACCESS(xdbId))
	 {
#ifdef WANTODBC
	   case XDBODBC:	v4odbc_FreeStuff(ctx,xdbId,flags) ; break ;
#endif
#ifdef WANTV4IS
	   case XDBV4IS:	v4midas_FreeStuff(ctx,xdbId,flags) ; break ;
#endif
#ifdef WANTMYSQL
	   case XDBMYSQL:	v4mysql_FreeStuff(ctx,xdbId,flags) ; break ;
#endif
#ifdef WANTORACLE
	   case XDBORACLE:	v4oracle_FreeStuff(ctx,xdbId,flags) ; break ;
#endif
	 } ;
}

#ifdef WANTODBC
/*	v4odbc_FreeStuff - Frees up ODBC Handles			*/
/*	Call: v4odbc_FreeStuff(ctx,xdbId,flags)
	  where ctx is current context,
		xdbId is id for connection (frees all statments under connection also) or statement,
		flags is optional processing flags			 */

void v4odbc_FreeStuff(ctx,xdbId,flags)
  struct V4C__Context *ctx ;
  XDBID xdbId ;
  FLAGS32 flags ;
{ struct V4XDB__Master *xdb ;
  INDEX i,j ;

	if ((xdb = gpi->xdb) == NULL) return ;
/*	If id is < 0 then negative, then free up connection and all statements under that connection */
	if (XDBISCON(xdbId))
	 { for(i=0;i<xdb->stmtCount;i++)
	    { if (xdb->stmts[i].hxdbId == xdbId) v4odbc_FreeStuff(ctx,xdb->stmts[i].xdbId,flags) ; } ;
	   for(i=0;i<xdb->conCount && ((flags & XDB_FREE_StmtOnly)==0);i++)
	    { if (xdb->con[i].xdbId != xdbId) continue ;
	      if (xdb->con[i].MIDASSocket != UNUSED) { v4midas_FreeStuff(ctx,xdbId,flags) ; continue ; } ;	/* Handle MIDAS here */
	      SQLDisconnect(xdb->con[i].hdbc) ; SQLFreeConnect(xdb->con[i].hdbc) ;
/*	      If this is last connection in list then just drop conCount otherwise flag the slot as empty */
	      if (i == xdb->conCount - 1) { xdb->conCount-- ; }
	       else { xdb->con[i].xdbId = UNUSED ; } ;
	    } ;
	 } else				/* Free up a statement */
	 { for(i=0;i<xdb->stmtCount;i++)
	    { if (xdb->stmts[i].xdbId != xdbId) continue ;
	      if (xdb->stmts[i].vmt != NULL) { v4midas_FreeStuff(ctx,xdbId,flags) ; continue ; } ;	/* Handle MIDAS here */
	      SQLFreeStmt(xdb->stmts[i].hstmt,SQL_DROP) ;
	      for(j=0;j<xdb->conCount;j++)
	       { if (xdb->stmts[i].hxdbId == xdb->con[j].xdbId)
		  gpi->xdb->con[j].isActive = FALSE ;
	       } ;
/*	      If this is the last statement in the list then drop stmtCount otherwise flag slot as empty */
	      if (i == xdb->stmtCount - 1) { xdb->stmtCount-- ; }
	       else { xdb->stmts[i].xdbId = UNUSED ; } ;
	    } ;
	 } ;
	return ;
}
#endif
#ifdef WANTV4IS
/*	v4midas_FreeStuff - Frees up ODBC Handles (may be called from vodbc_FreeStuff())		*/
/*	Call: v4midas_FreeStuff(ctx,xdbId,flags)
	  where ctx is current context,
		hdbc is 0 or connection handle or UNUSED for all,
		hstmt is 0 or statement handle or UNUSED for all,
		flags is optional processing flags							*/


void v4midas_FreeStuff(ctx,xdbId,flags)
  struct V4C__Context *ctx ;
  XDBID xdbId ;
  FLAGS32 flags ;
{ struct V4XDB__Master *xdb ;
  INDEX i,j ; XDBID xdbIdCon ;


	if ((xdb = gpi->xdb) == NULL) return ;
/*	If id is < 0 then negative, then free up connection and all statements under that connection */
	if (XDBISCON(xdbId))
	 { for(i=0;i<xdb->stmtCount;i++)
	    { if (xdb->stmts[i].hxdbId == xdbId) v4midas_FreeStuff(ctx,xdb->stmts[i].xdbId,flags) ; } ;
	   for(i=0;i<xdb->conCount && ((flags & XDB_FREE_StmtOnly)==0);i++)
	    { if (xdb->con[i].xdbId != xdbId) continue ;
	      if (xdb->con[i].MIDASSocket != UNUSED) { SOCKETCLOSE(xdb->con[i].MIDASSocket) ; xdb->con[i].MIDASSocket = UNUSED ; } ;
/*	      If this is last connection in list then just drop conCount otherwise flag the slot as empty */
	      if (i == xdb->conCount - 1) { xdb->conCount-- ; }
	       else { xdb->con[i].xdbId = UNUSED ; } ;
	    } ;
	 } else				/* Free up a statement */
	 { xdbIdCon = 0 ;
	   for(i=0;i<xdb->stmtCount;i++)
	    { if (xdb->stmts[i].xdbId != xdbId) continue ;
#if defined WANTMYSQL || defined WANTORACLE || defined WANTODBC
	      if (xdb->stmts[i].vmt->databuf != NULL) { v4mm_FreeChunk(xdb->stmts[i].vmt->databuf) ; xdb->stmts[i].vmt->databuf = NULL ; } ;
	      v4mm_FreeChunk(xdb->stmts[i].vmt) ; xdb->stmts[i].vmt = NULL ; xdbIdCon = xdb->stmts[i].hxdbId ;
#endif
/*	      If this is the last statement in the list then drop stmtCount otherwise flag slot as empty */
	      if (i == xdb->stmtCount - 1) { xdb->stmtCount-- ; }
	       else { xdb->stmts[i].xdbId = UNUSED ; } ;
	      for(j=0;j<xdb->conCount;j++)
	       { if (xdb->stmts[i].hxdbId == xdb->con[j].xdbId)
		  gpi->xdb->con[j].isActive = FALSE ;
	       } ;
	    } ;
	   if (xdbIdCon != 0) v4midas_FreeStuff(ctx,xdbIdCon,flags) ;	/* Free up the connection always (the MIDAS side has shut down - we better also) */
	 } ;
	return ;
}
#endif

#ifdef WANTMYSQL

/*	v4mysql_FreeStuff - Frees up ODBC Handles			*/
/*	Call: v4mysql_FreeStuff(ctx,xdbId,flags)
	  where ctx is current context,
		xdbId is id for connection (frees all statments under connection also) or statement,
		flags are optional flags (see XDB_FREE_xxx)		*/

void v4mysql_FreeStuff(ctx,xdbId,flags)
  struct V4C__Context *ctx ;
  XDBID xdbId ;
  FLAGS32 flags ;
{ struct V4XDB__Master *xdb ;
  INDEX i,j ;

	if ((xdb = gpi->xdb) == NULL) return ;
/*	If id is < 0 then negative, then free up connection and all statements under that connection */
	if (XDBISCON(xdbId))
	 { for(i=0;i<xdb->stmtCount;i++)
	    { if (xdb->stmts[i].hxdbId == xdbId) v4mysql_FreeStuff(ctx,xdb->stmts[i].xdbId,0) ; } ;
	   for(i=0;i<xdb->conCount && ((flags & XDB_FREE_StmtOnly)==0);i++)
	    { if (xdb->con[i].xdbId != xdbId) continue ;
	      mysql_close(gpi->xdb->con[i].mysql) ;
/*	      If this is last connection in list then just drop conCount otherwise flag the slot as empty */
	      if (i == xdb->conCount - 1) { xdb->conCount-- ; }
	       else { xdb->con[i].xdbId = UNUSED ; } ;
	    } ;
	 } else				/* Free up a statement */
	 { for(i=0;i<xdb->stmtCount;i++)
	    { if (xdb->stmts[i].xdbId != xdbId) continue ;
/*	      Do we have any saved rows to deallocate? */
	      mysql_free_result(xdb->stmts[i].res) ;
	      mysql_stmt_close(xdb->stmts[i].stmt) ;
	      if (xdb->stmts[i].data != NULL) { v4mm_FreeChunk(xdb->stmts[i].data) ; xdb->stmts[i].data = NULL ; } ;
	      for(j=0;j<xdb->conCount;j++)
	       { if (xdb->stmts[i].hxdbId == xdb->con[j].xdbId)
		  gpi->xdb->con[j].isActive = FALSE ;
	       } ;
/*	      If this is the last statement in the list then drop stmtCount otherwise flag slot as empty */
	      if (i == xdb->stmtCount - 1) { xdb->stmtCount-- ; }
	       else { xdb->stmts[i].xdbId = UNUSED ; } ;
	    } ;
	 } ;
	return ;
}
#endif	/* End of WANTMYSQL */

#ifdef WANTORACLE

/*	v4oracle_FreeStuff - Frees up ODBC Handles			*/
/*	Call: v4oracle_FreeStuff(ctx,xdbId,flags)
	  where ctx is current context,
		xdbId is id for connection (frees all statments under connection also) or statement,
		flags are optional flags (see XDB_FREE_xxx)		*/

void v4oracle_FreeStuff(ctx,xdbId,flags)
  struct V4C__Context *ctx ;
  XDBID xdbId ;
  FLAGS32 flags ;
{ struct V4XDB__Master *xdb ;
  INDEX i,j ;

	if ((xdb = gpi->xdb) == NULL) return ;
/*	If id is < 0 then negative, then free up connection and all statements under that connection */
	if (XDBISCON(xdbId))
	 { for(i=0;i<xdb->stmtCount;i++)
	    { if (xdb->stmts[i].hxdbId == xdbId) v4oracle_FreeStuff(ctx,xdb->stmts[i].xdbId,0) ; } ;
	   for(i=0;i<xdb->conCount && ((flags & XDB_FREE_StmtOnly)==0);i++)
	    { if (xdb->con[i].xdbId != xdbId) continue ;
	      OCI_ConnectionFree(gpi->xdb->con[i].oracle) ;
/*	      If this is last connection in list then just drop conCount otherwise flag the slot as empty */
	      if (i == xdb->conCount - 1) { xdb->conCount-- ; }
	       else { xdb->con[i].xdbId = UNUSED ; } ;
	    } ;
	 } else				/* Free up a statement */
	 { for(i=0;i<xdb->stmtCount;i++)
	    { if (xdb->stmts[i].xdbId != xdbId) continue ;
	      for(j=0;j<xdb->conCount;j++)
	       { if (xdb->stmts[i].hxdbId == xdb->con[j].xdbId)
		  gpi->xdb->con[j].isActive = FALSE ;
	       } ;
	      if (i == xdb->stmtCount - 1) { xdb->stmtCount-- ; }
	       else { xdb->stmts[i].xdbId = UNUSED ; } ;
	    } ;
	 } ;
	return ;
}
#endif	/* End of WANTORACLE */


#ifdef WANTV4IS


/*	v4odbc_MIDASConnect - Connect to ODBC Server				*/
/*	Call: xdbId = v4odbc_MIDASConnect( ctx, intmodx, trace, di, connection, dsn , user , password , ok )
	  where xdbId = Connection id or 0 if error,
		intmodx is calling V4 module index,
		di = diminfo of "host" dimension,
		connection is ptr to connection info,
		dsn is string ptr to service/db name,
		user/password are ptrs to string or NULL,
		ok is updated TRUE/FALSE if all is well or not		*/

COUNTER v4odbc_MIDASConnect(ctx,intmodx,trace,di,connection,dsn,user,password,ok)
  struct V4C__Context *ctx ;
  INTMODX intmodx ; VTRACE trace ;
  struct V4DPI__DimInfo *di ;
  UCCHAR *connection,*dsn,*user,*password ;
  LOGICAL *ok ;
{ struct V4XDB__Master *xdb ;
  int hdbc ;
  XDBID xdbId ;
  int Socket = UNUSED ;
  int res ; UCCHAR *b, servername[256], *portsvc ; INDEX newx ;

	xdb = gpi->xdb ;
/*	Look for empty connection slot */
	for (newx=0;newx<xdb->conCount;newx++) { if (xdb->con[newx].xdbId == UNUSED) break ; } ;
	if (newx == xdb->conCount)
	 { if (xdb->conCount >= V4XDB_ConnectMax) { v_Msg(ctx,NULL,"XDBMaxCon",intmodx,V4XDB_ConnectMax) ; *ok = FALSE ; return(0) ; } ;
	 } ;
	if (xdb->MIDASluHandle == 0) xdb->MIDASluHandle = V4ODBC_MIDAS_UNIQUE_HANDLE_START ;

	hdbc = ++xdb->MIDASluHandle ;
	XDBNEWID(xdbId,XDBV4IS,XDBCONNECTION) ;
	xdb->con[newx].mhdbc = hdbc ; xdb->con[newx].DimId = di->DimId ;

	SOCKETINIT
	Socket = UNUSED ;
/*	Check file_name for host/port number, if no host then use current host */
	b = UCstrchr(dsn,':') ; if (b == NULL) b = UCstrchr(dsn,'/') ;

	if (b == NULL) { portsvc = UClit("3000") ; UCstrcpy(servername,dsn) ; }
	 else { portsvc = b+1 ; UCstrncpy(servername,dsn,b-dsn) ; servername[b-dsn] = UCEOS ; } ;
	if (!v_ipLookup(ctx,intmodx,servername,portsvc,SOCK_STREAM,V_IPLookup_Connect,&Socket,UCTBUF1,NULL)) goto connect_fail ;

	if (NETIOCTL(Socket,FIONBIO,&res) != 0) { v_Msg(ctx,NULL,"SocketNoBlock",intmodx) ; goto connect_fail ; } ;

	v_Msg(ctx,UCTBUF1,"@MIDASSQL:%1U:%2U",servername,portsvc) ;
	xdb->con[newx].ConnectionStr = v4mm_AllocUC(UCstrlen(UCTBUF1)) ;
	UCstrcpy(xdb->con[newx].ConnectionStr,UCTBUF1) ;
	xdb->con[newx].MIDASSocket = Socket ; xdb->con[newx].xdbId = xdbId ;
	if (newx == xdb->conCount) xdb->conCount++ ;
	*ok = TRUE ; return(xdbId) ;

connect_fail:
	if (Socket != UNUSED) { SOCKETCLOSE(Socket) ; } ;
	*ok = FALSE ; return(0) ;
}


/*	v4odbc_MIDASXct - Execute a MIDAS - ODBC statement */

P *v4odbc_MIDASXct(ctx,respnt,autoclose,hxdbId,sqlstmt,retval,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt ;
  XDBID hxdbId ;
  LOGICAL autoclose ;
  UCCHAR *sqlstmt ;
  enum DictionaryEntries retval ;
  INTMODX intmodx ; VTRACE trace ;
{ struct V4XDB__Master *xdb ;
  struct V4XDB_MIDASTable *vmt ;
  char *b,*b1,retcode[32] ; int i ; COUNTER numRows ;
  int mstmt ; INDEX cx,sx ; XDBID xdbId ;

/*	Set up a new mstmt */
	xdb = gpi->xdb ;
	if (!v4xdb_NewStmtInitCommon(ctx,xdb,hxdbId,XDBODBC,sqlstmt,intmodx,&cx,&sx,&xdbId,trace)) { RETURNFAILURE ; } ;
	mstmt = (++xdb->MIDASluHandle) ;
	xdb->stmts[sx].mstmt = mstmt ;
//	xdb->stmts[sx].hdbc = xdb->con[cx].hdbc ;
	xdb->stmts[sx].curRow = 0 ; xdb->stmts[sx].xdbId = xdbId ;
	vmt = v4mm_AllocChunk(sizeof *vmt,TRUE) ;
	vmt->databufbytes = V4XDB_InitialMIDASDatBuf ;
	vmt->databuf = v4mm_AllocChunk(V4XDB_InitialMIDASDatBuf,FALSE) ; ZS(vmt->databuf) ;
	xdb->stmts[sx].vmt = vmt ;
	if (!v4odbc_MIDASSendRcvRequest(ctx,intmodx,vmt,xdb->con[cx].MIDASSocket,UCretASC(sqlstmt))) goto fail ;
/*	Parse header message */
	b = vmt->databuf ;
	for(i=0;i<((sizeof retcode) - 1) && b[i]!=MIDAS_EOC;i++) { retcode[i] = toupper(b[i]) ; } ; retcode[i] = '\0' ;
/*	b[i] = tab just before first column name */
	b = &b[i+1] ;
	if (strcmp(retcode,"ERROR") == 0)
	 { b1 = strchr(b,MIDAS_EOL) ; if (b1 != NULL) *b1 = '\0' ;
	   v_Msg(ctx,NULL,"XDBMIDASErr",intmodx,b) ; goto fail ;
	 } else if (strcmp(retcode,"RECCOUNT") != 0)
	 { v_Msg(ctx,NULL,"XDBMIDASHdr",intmodx,retcode) ; goto fail ; } ;
/*	b = ptr to number of rows updated/deleted by last SQL request - return it */
	numRows = strtol(b,&b1,10) ;
/*	Do we have any extra data ? */
	if (*b1 == '\t') 
	 { b = b1 + 1 ; b1 = strchr(b,MIDAS_EOL) ; if (b1 != NULL) *b1 = '\0' ;
	   if (strlen(b) < sizeof(xdb->extraData)) { strcpy(xdb->extraData,b) ; } ;
	 } else { ZS(xdb->extraData) ; } ;
	switch (retval)
	 { default:
		logPNTv(respnt,TRUE) ; break ;
	   case DE(Rows):
		intPNTv(respnt,numRows) ;
		if (trace & V4TRACE_XDB)
		 { v_Msg(ctx,UCTBUF1,"XDBtRows",intmodx,respnt->Value.IntVal) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
		break ;
	   case DE(Key):
		intPNTv(respnt,strtol(xdb->extraData,&b1,10)) ;
		if (trace & V4TRACE_XDB)
		 { v_Msg(ctx,UCTBUF1,"XDBtKeyVal",intmodx,respnt->Value.IntVal) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
		break ;
	 } ;
	if (trace & V4TRACE_XDB)
	 { v_Msg(ctx,UCTBUF1,"XDBtRows",intmodx,respnt->Value.IntVal) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
/*	Free up this statement & if autoclose then the entire connection */
	v4midas_FreeStuff(ctx,xdbId,0) ;
	v4xdb_NewStmtFinish(ctx,xdb,sx,xdbId) ;
	if (autoclose) v4midas_FreeStuff(ctx,xdb->con[cx].xdbId,0) ;
	return(respnt) ;

fail:	v4midas_FreeStuff(ctx,xdbId,0) ;
	if (autoclose) v4midas_FreeStuff(ctx,xdb->con[cx].xdbId,0) ;
	REGISTER_ERROR(0) ; RETURNFAILURE ;
}


INDEX v4odbc_MIDASNewStmt(ctx,hxdbId,sqlstmt,intmodx,trace)
  struct V4C__Context *ctx ;
  XDBID hxdbId ;
  UCCHAR *sqlstmt ;
  INTMODX intmodx ; VTRACE trace ;
{ struct V4XDB__Master *xdb ;
  struct V4XDB_MIDASTable *vmt ;
  int i ;
  char retcode[32],*term, *b, *b1 ;
  int mstmt ; INDEX cx,sx ;
  XDBID xdbId ;

	xdb = gpi->xdb ;
	if (!v4xdb_NewStmtInitCommon(ctx,xdb,hxdbId,XDBV4IS,sqlstmt,intmodx,&cx,&sx,&xdbId,trace)) goto fail ;
	mstmt = (++xdb->MIDASluHandle) ;
	xdb->stmts[sx].mstmt = mstmt ; xdb->stmts[sx].mhdbc = xdb->con[cx].mhdbc ;
	vmt = v4mm_AllocChunk(sizeof *vmt,TRUE) ;
	vmt->databufbytes = V4XDB_InitialMIDASDatBuf ;
	vmt->databuf = v4mm_AllocChunk(V4XDB_InitialMIDASDatBuf,FALSE) ; ZS(vmt->databuf) ;
	xdb->stmts[sx].vmt = vmt ;
#ifdef TESTNewStmt	
{ FILE *fp ;
	vmt->RowCount = -1 ;			/* Start with -1 because first line in file is header, not data */
	fp = fopen("midasodbc.test","r") ;
	for(tbytes=0;;)
	 { if (fgets(tbuf,sizeof tbuf,fp) == NULL) { fclose(fp) ; break ; } ;
	   if ((bytes=strlen(tbuf)) + tbytes > vmt->databufbytes)
	    { /* Have to realloc here */
	    } ;
	   for(bytes;bytes>0;bytes--) { if (tbuf[bytes-1] > MIDAS_EOL) break ; } ;
	   tbuf[bytes++] = MIDAS_EOL ; tbuf[bytes] = '\0' ;		/* Ensure line terminator is MIDAS_EOL */
	   strcat(&vmt->databuf[tbytes],tbuf) ; tbytes += bytes ;
	   vmt->RowCount++ ;
	 } ;
}
#else
	if (!v4odbc_MIDASSendRcvRequest(ctx,intmodx,vmt,xdb->con[cx].MIDASSocket,UCretASC(sqlstmt))) goto fail ;
	xdb->stmts[sx].rowCnt = vmt->RowCount ;
#endif
/*	Parse header message */
	b = vmt->databuf ;
	for(i=0;i<((sizeof retcode) - 2) && b[i]!=MIDAS_EOC;i++) { retcode[i] = toupper(b[i]) ; } ; retcode[i] = '\0' ;
/*	b[i] = tab just before first column name */
	b = &b[i+1] ;
	if (strcmp(retcode,"ERROR") == 0)
	 { b1 = strchr(b,MIDAS_EOL) ; if (b1 != NULL) *b1 = '\0' ;
	   v_Msg(ctx,NULL,"XDBMIDASErr",intmodx,b) ; goto fail ;
	 } else if (strcmp(retcode,"RECCOUNT") == 0)		/* Return record count as Int point (or -1 if any problems) */
	 { xdb->stmts[sx].rowCnt = strtol(b,&term,10) ; if (!(*term == MIDAS_EOL || *term == '\0')) xdb->stmts[sx].rowCnt = UNUSED ;
	   return(sx) ;
	 } else if (strcmp(retcode,"DATA") != 0)
	 { v_Msg(ctx,NULL,"XDBMIDASHdr",intmodx,retcode) ; goto fail ; } ;
/*	Got DATA - parse column names & v4 types */
	vmt->colCount = 0 ; vmt->currowx = UNUSED ;
	for(;vmt->colCount<V4XDB_StmtColMax;)
	 { for(i=0;i<V4DPI_DimInfo_DimNameMax && !(*b ==':' || *b =='/') && *b != '\0';i++,b++) { vmt->Col[vmt->colCount].Name[i] = toupper(*b) ; } ;
	   vmt->Col[vmt->colCount].Name[i] = '\0' ;
/*	   *b = ':', following up to tab is v4 point type number */
	   switch(i=strtol(++b,&term,10))
	    { default:				v_Msg(ctx,NULL,"XDBMIDASCol",vmt->colCount,i) ; goto fail ;
	      case V4DPI_PntType_Int:		xdb->stmts[sx].col[vmt->colCount].vdType = VDTYPE_MIDAS_Int ; break ;
	      case V4DPI_PntType_UDT:		xdb->stmts[sx].col[vmt->colCount].vdType = VDTYPE_MIDAS_UDT ; break ;
//	      case V4DPI_PntType_UDate:		xdb->stmts[sx].col[vmt->colCount].vdType = VDTYPE_MIDAS_UDate ; break ;
	      case V4DPI_PntType_Logical:	xdb->stmts[sx].col[vmt->colCount].vdType = VDTYPE_MIDAS_Log ; break ;
	      case V4DPI_PntType_Char:		xdb->stmts[sx].col[vmt->colCount].vdType = VDTYPE_MIDAS_Char ; break ;
	      case V4DPI_PntType_Real:		xdb->stmts[sx].col[vmt->colCount].vdType = VDTYPE_MIDAS_Real ; break ;
//	      case V4DPI_PntType_MIDASZIP:	xdb->stmts[sx].col[vmt->colCount].vdType = VDTYPE_MIDAS_Zip ; break ;
	    } ;
	   vmt->colCount++ ;
	   for(;;term++) { if (*term == '\t') continue ; if (*term == ' ') continue ; break ; } ;
	   if (*term == MIDAS_EOL || *term == '\0') break ;
	   b = term  ;
	 } ;
	if (vmt->colCount >= V4XDB_StmtColMax)
	 { v_Msg(ctx,NULL,"XDBMaxCol",intmodx,vmt->colCount,V4XDB_StmtColMax) ; goto fail ; } ;
	
	xdb->stmts[sx].colCount = vmt->colCount ;
	v4xdb_NewStmtFinish(ctx,xdb,sx,xdbId) ;
	return(sx) ;

fail:	FREEMTLOCK(xdb->xdbLock) ;
	v4midas_FreeStuff(ctx,xdbId,0) ;				/* Have to free up everything because MIDAS side has already shut down */
	if (gpi->xdb != NULL)  { ctx->ErrorMsg[UCsizeof(gpi->xdbLastError)-1] = UCEOS ; UCstrcpy(gpi->xdbLastError,ctx->ErrorMsg) ; } ;
	return(UNUSED) ;
}

LOGICAL v4odbc_MIDASSendRcvRequest(ctx,intmodx,vmt,socket,odbcreq)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  struct V4XDB_MIDASTable *vmt ;
  int socket ;
  char *odbcreq ;
{
  fd_set sset ;				/* Socket set */
  struct timeval tev ;
  int mx,res,reqlen,sent ; char EOMbuf[2] ;
#define MIDASRECVCHUNK 2048
  char mrBuf[MIDASRECVCHUNK] ; INDEX mrx ; LOGICAL ok ;

#define MTRACE TRUE
/*	Send request off to server */
	reqlen = strlen(odbcreq) ;
#ifdef MTRACE
v_Msg(ctx,UCTBUF1,"@VMIDASSQL: %1d bytes: %2s\n",reqlen,odbcreq) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
#endif
top:
	for(mx=0,sent=0;sent<reqlen;)
	 { FD_ZERO(&sset) ; FD_SET(socket,&sset) ;
	   tev.tv_sec = 30 ; tev.tv_usec = 0 ;			/* Set max wait for 30 seconds */
	   res = select(FD_SETSIZE,NULL,&sset,NULL,&tev) ;	/* Wait for ok to send */
	   if (res <= 0) { v_Msg(ctx,NULL,"SocketTimeOut",intmodx,(mx==0 ? 60 : 5),mx+1) ; return(FALSE) ; } ;
	   for(sent=0;;sent+=(res > 0 ? res : 0))
	    { res = send(socket,&odbcreq[sent],reqlen-sent,0) ;
	      sent += res ;
	      if (res >= reqlen-sent)
	       { if (odbcreq == EOMbuf) break ;
		 EOMbuf[0] = MIDAS_EOM ; odbcreq = EOMbuf ; reqlen = 1 ;
		 goto top ;
	       } ;
	      if (NETERROR == WSAEWOULDBLOCK) { HANGLOOSE(50) ; continue ; } ;
	      v_Msg(ctx,NULL,"SocketSendErr",intmodx,NETERROR) ; return(FALSE) ;
	    } ;
	 } ;
#ifdef MTRACE
v_Msg(ctx,UCTBUF1,"@VMIDASSQL: Waiting for reply\n") ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
#endif

/*	Now get reply */
	vmt->RowCount = -1 ;			/* Start with -1 because first line in file is header, not data */
#define MIDASRECVCHUNK 2048
	ok = TRUE ;
	for(mx=0;ok;mx++)
	 { FD_ZERO(&sset) ; FD_SET(socket,&sset) ;
	   tev.tv_sec = (mx==0 ? 60 : 5) ; tev.tv_usec = 0 ;	/* Wait a little longer for first byte */
	   res = select(FD_SETSIZE,&sset,NULL,NULL,&tev) ;	/* Wait for something to happen */
	   if (res <= 0) { v_Msg(ctx,NULL,"SocketTimeOut",intmodx,(mx==0 ? 60 : 5),mx+1) ; return(FALSE) ; } ;
#ifdef MIDASRECVCHUNK
	   res = recv(socket,mrBuf,MIDASRECVCHUNK,0) ;
#ifdef MTRACE
{ char obuf[10000] ; int ix,ox ;
  for(ix=0,ox=0;ix<res&&ox<(sizeof obuf)-5;ix++)
   { if (mrBuf[ix] > 26) { obuf[ox++] = mrBuf[ix] ; continue ; }
     obuf[ox++] = '\\' ; obuf[ox++] = 'A' + mrBuf[ix] ;
   } ; obuf[ox++] = '\0' ;
v_Msg(ctx,UCTBUF1,"@VMIDASSQL: Received %1d bytes: %2s\n",res,obuf) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
}
#endif
	   if (res <= 0)
	    { if (res == 0)
	       { v_Msg(ctx,NULL,"XDBMIDASPreEOM",intmodx,NETERROR) ; return(FALSE) ;
	       } else { v_Msg(ctx,NULL,"SocketRcvErr",intmodx,NETERROR) ; return(FALSE) ; } ;
	    } ;
	   for(mrx=0;mrx<res>0;mx++,mrx++)
	    { 
	      if (mx >= vmt->databufbytes)					/* Are we at the end of our buffer ? */
	       { vmt->databufbytes = (int)(vmt->databufbytes * 1.5) ;		/* Increase it by 50% */
	         vmt->databuf = (char *)realloc(vmt->databuf,vmt->databufbytes) ;
	       } ;
	      vmt->databuf[mx] = mrBuf[mrx] ;
	      if (vmt->databuf[mx] == '\0') { mx-- ; continue ; } ;		/* Ignore nulls */
	      if (vmt->databuf[mx] == MIDAS_EOM)
	       { vmt->databuf[mx] = '\0' ; {ok = FALSE ; break ; } ; } ;	/* End-Of-Message */
	      if (vmt->databuf[mx] <= 26 && vmt->databuf[mx] != MIDAS_EOC) vmt->databuf[mx] = MIDAS_EOL ;
	      if (vmt->databuf[mx] == MIDAS_EOL)				/* Check for line terminator(s) */
	       { if (mx > 0 ? vmt->databuf[mx-1] == MIDAS_EOL : FALSE) { mx-- ; continue ; } ;
	         vmt->databuf[mx] = MIDAS_EOL ;					/* Only want one byte line terminator */
	         vmt->RowCount++ ;
	       } ;
	    } ;
#else
	   if (mx >= vmt->databufbytes)	/* Are we at the end of our buffer ? */
	    { vmt->databufbytes = (int)(vmt->databufbytes * 1.5) ;	/* Increase it by 50% */
	      vmt->databuf = (char *)realloc(vmt->databuf,vmt->databufbytes) ;
	    } ;
	   res = recv(socket,&vmt->databuf[mx],1,0) ;
	   if (res <= 0)
	    { if (res == 0)
	       { v_Msg(ctx,NULL,"XDBMIDASPreEOM",intmodx,NETERROR) ; return(FALSE) ;
	       } else { v_Msg(ctx,NULL,"SocketRcvErr",intmodx,NETERROR) ; return(FALSE) ; } ;
	    } ;
	   if (vmt->databuf[mx] == '\0') { mx-- ; continue ; } ;		/* Ignore nulls */
	   if (vmt->databuf[mx] == MIDAS_EOM) { vmt->databuf[mx] = '\0' ; break ; } ;	/* End-Of-Message */
	   if (vmt->databuf[mx] <= 26 && vmt->databuf[mx] != MIDAS_EOC) vmt->databuf[mx] = MIDAS_EOL ;
	   if (vmt->databuf[mx] == MIDAS_EOL)		/* Check for line terminator(s) */
	    { if (mx > 0 ? vmt->databuf[mx-1] == MIDAS_EOL : FALSE) { mx-- ; continue ; } ;
	      vmt->databuf[mx] = MIDAS_EOL ;				/* Only want one byte line terminator */
	      vmt->RowCount++ ;
	    } ;
#endif
	 } ;
	return(TRUE) ;
}


/*	v4midas_Fetch - Fetches (or sets internal pointers) (next) row in current table */
/*	Call sx = v4midas_Fetch( ctx , xdbId , row , ok , intmodx , respnt)
	  where sx = xdb statement index (UNUSED if no more data),
		ctx is context,
		xdbId is statement Id,
		row is requested row number, V4ODBC_FetchNext for next row, V4ODBC_FetchFirst/Last for first last rows,
		*ok is updated to TRUE if OK (and on normal EOT), FALSE if error (ctx->ErrorMsgAux),
		intmodx is calling module,
		respnt is resulting point that may be updated by evaluation of fetchPnt
*/

INDEX v4midas_Fetch(ctx,xdbId,row,ok,intmodx,respnt)
  struct V4C__Context *ctx ;
  XDBID xdbId ;
  XDBROW row ;
  INTMODX intmodx ; LOGICAL *ok ;
  P *respnt ;
{ struct V4XDB__Master *xdb ;
  struct V4XDB_MIDASTable *vmt ;
  INDEX sx,i,cx ; char *data,*bp ;

	if ((xdb = gpi->xdb) == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"XDBNoCon",intmodx) ; *ok = FALSE ; return(0) ; } ;
	for(sx=0;sx<xdb->stmtCount;sx++) { if (xdb->stmts[sx].xdbId == xdbId) break ; } ;
	if (sx >= xdb->stmtCount) { v_Msg(ctx,ctx->ErrorMsgAux,"ODBCHstmtBad",intmodx) ; *ok = FALSE ; return(0) ; } ;
	XDBSAVECURROW(xdb,sx) ;
	if (row == V4ODBC_FetchNext) row = ++xdb->stmts[sx].curRow ;
	vmt = xdb->stmts[sx].vmt ;
	if (row > xdb->stmts[sx].vmt->RowCount || row == V4ODBC_FetchEOF)
	 { 
	   XDBRMVTARGETDIM(sx) ;
	   if (xdb->stmts[sx].data != NULL) { /* v4mm_FreeChunk(xdb->stmts[sx].data) ; DON'T FREE - major buffer is vmt->databuf VEH110401 */ xdb->stmts[sx].data = NULL ; } ;
	   v4midas_FreeStuff(ctx,xdb->stmts[sx].xdbId,0) ;
	   if (xdb->stmts[sx].fetchPnt != NULL) { v4mm_FreeChunk(xdb->stmts[sx].fetchPnt) ; xdb->stmts[sx].fetchPnt = NULL ; } ;
	   if (xdb->stmts[sx].eofPnt != NULL)
	    { P epnt,*ipt ;
	      ipt = v4dpi_IsctEval(&epnt,xdb->stmts[sx].eofPnt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	      v4mm_FreeChunk(xdb->stmts[sx].eofPnt) ; xdb->stmts[sx].eofPnt = NULL ;
	      if (ipt == NULL)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"XDBNestEval",V4IM_Tag_End,xdb->stmts[sx].eofPnt,ctx->ErrorMsg) ;
	         v4mm_FreeChunk(xdb->stmts[sx].eofPnt) ; xdb->stmts[sx].eofPnt = NULL ;
	         *ok = FALSE ; return(0) ;
	       } ;
	    } ;
	   v4mm_FreeChunk(xdb->stmts[sx].eofPnt) ; xdb->stmts[sx].eofPnt = NULL ;
	   *ok = TRUE ; return(UNUSED) ;
	 } ;
/*	First position to proper row (probably the "next" row) */
	if (gpi->xdb->stmts[sx].curRow == vmt->currowx) { data = (vmt->currow == NULL ? NULL : vmt->currow - 1) ; }
	 else if (gpi->xdb->stmts[sx].curRow == vmt->currowx + 1) { data = strchr(vmt->currow,MIDAS_EOL) ; }
	 else { data = vmt->databuf ;
	        for(i=0;i<gpi->xdb->stmts[sx].curRow;i++) { data = strchr(vmt->databuf,MIDAS_EOL) ; if (data == NULL) break ; } ;
	      } ;
	vmt->currowx = gpi->xdb->stmts[sx].curRow ;
/*	data = pointer to proper row (or NULL if row not available) */
	if (data == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"XDBMIDASNoRow",intmodx,gpi->xdb->stmts[sx].curRow) ; *ok = FALSE ; return(0) ; } ;
	data++ ; vmt->currow = data ;			/* Postion over EOL to next cx */
	gpi->xdb->stmts[sx].data = data ;
/*	Now get offsets to each column */
	bp = strchr(data,MIDAS_EOC) ;		/* Skip first column */
	if (bp == NULL) { v_Msg(ctx,NULL,"XDBMIDASTrash",intmodx) ; *ok = FALSE ; return(0) ; }  ;
	bp++ ;					/* Skip first column (continued) */
	for(cx=0;cx<xdb->stmts[sx].colCount;cx++)
	 { char *nbp = strchr(bp,MIDAS_EOC) ;
	   gpi->xdb->stmts[sx].col[cx].Offset = (char *)bp - (char *)data ;
	   gpi->xdb->stmts[sx].col[cx].UBytes = (nbp != NULL ? (nbp - bp) : strlen(bp)) ;
	   if (nbp == NULL) break ;
	   bp = nbp + 1 ;
	 } ;
	if (xdb->stmts[sx].fetchPnt != NULL)
	 { P *ipt ;
	   ipt = v4dpi_IsctEval(respnt,xdb->stmts[sx].fetchPnt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	   if (ipt == NULL)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"XDBNestEval",V4IM_Tag_Do,xdb->stmts[sx].fetchPnt,ctx->ErrorMsg) ;
	      *ok = FALSE ; return(0) ;
	    } ;
	 } ;
	GETNEXTRECID(xdb,sx) ;
	*ok = TRUE ; return(sx) ;
}

#endif /* WANTV4IS */

/*	v4xdb_SaveXDBRow - Saves current XDB Row in process temp Area	*/
/*	Call: len = v4xdb_SaveXDBRow( ctx , xdb , sx )
	  where len is number of bytes save (UNUSED if error in ctx->ErrorMsgAux),
	  ctx is context,
	  xdb is pointer to xdb (i.e. gpi->xdb)
	  sx is statement index within xdb				*/

LENMAX v4xdb_SaveXDBRow(ctx,xdb,sx)
  struct V4C__Context *ctx ;
  struct V4XDB__Master *xdb ;
  INDEX sx ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4XDB_SavedRow *vxsr, vxsrBuf ;
  BYTE *sptr, *data ; LENMAX maxBytes ;
  INDEX cx,areax ; LENMAX bytes,off ; AREAID aid ;
  unsigned short *colOff ;

/*	Should we save the row 'point' in a list? */
	if (xdb->stmts[sx].targetDimId != UNUSED)
	 { struct V4XDB__SaveStmtInfo *vxssi = xdb->stmts[sx].vxssi ;
	   struct V4DPI__LittlePoint lpnt ;
	   if (vxssi->lp != NULL)
	    { intPNTv(&lpnt,xdb->stmts[sx].curRecId) ; lpnt.Dim = xdb->stmts[sx].targetDimId ;
	      v4l_ListPoint_Modify(ctx,vxssi->lp,V4L_ListAction_Append,(P *)&lpnt,0) ;
	    } ;
	 } ;
	vxsr = &vxsrBuf ;
	sptr = vxsr->data ; maxBytes = sizeof vxsr->data ;
/*	colOff = array at begin of each saved record with index to data (after colOff array but indexed from begin of record, i.e. first offset is not 0) for each column
	  array is 1+number-of-columns with last column being total length of data record
	  column with null value as 0 in colOff - ex: if column 2 is null then colOff[2] = 0
	  size of column x = colOff[x+1] - colOff[x]
	    zero length column has same index as next column
	    if next column is null then have to go to next-next column	*/
	  
	off = ALIGN((xdb->stmts[sx].colCount + 1) * sizeof *colOff) ;
	colOff = (unsigned short *)sptr ;
	for(cx=0;cx<xdb->stmts[sx].colCount;cx++)
	 { 
	   switch (XDBGETDBACCESS(gpi->xdb->stmts[sx].xdbId))
	    { 
#if defined WANTODBC || defined WANTV4IS
	      case XDBODBC: case XDBV4IS:
		bytes = gpi->xdb->stmts[sx].col[cx].UBytes ;
		data = xdb->stmts[sx].data+gpi->xdb->stmts[sx].col[cx].Offset ;
		break ;
#endif
#ifdef WANTMYSQL
	      case XDBMYSQL:
		bytes = (gpi->xdb->stmts[sx].col[cx].isNull ? -1 : gpi->xdb->stmts[sx].col[cx].length) ;
		data = gpi->xdb->stmts[sx].bind[cx].buffer ;
		break ;
#endif
#ifdef WANTORACLE
	      case XDBORACLE:
		bytes = (gpi->xdb->stmts[sx].col[cx].isNull ? -1 : gpi->xdb->stmts[sx].col[cx].length) ;
		data = gpi->xdb->stmts[sx].rs ;
		break ;
#endif
	    } ;
	   if (bytes < 0)						/* null field - record as 0 in offset table */
	    { colOff[cx] = 0 ; continue ; } ;
	   if (off + bytes >= maxBytes) return(UNUSED) ;		/* Data too big to fit in sptr */
	   colOff[cx] = off ;
	   memcpy(sptr+off,data,bytes) ;
	   off = off + bytes ;
	 } ;
	colOff[xdb->stmts[sx].colCount] = off ;			/* Last column = total size */
/*	Now write out this to temp AREA */
	vxsr->kp.fld.KeyType = V4IS_KeyType_xdbRow ; vxsr->kp.fld.KeyMode = V4IS_KeyMode_Int ; vxsr->kp.fld.Bytes = V4IS_xdbRow_Bytes ;
//VEH110907	vxsr->kp.fld.AuxVal = (xdb->stmts[sx].targetDimId == UNUSED ? xdb->stmts[sx].dimId : xdb->stmts[sx].targetDimId) ;
	vxsr->kp.fld.AuxVal = XDBIDPART(xdb->stmts[sx].xdbId) ;
	vxsr->recId = xdb->stmts[sx].curRecId ;
	aid = gpi->RelH[V4DPI_WorkRelHNum].aid ;
	bytes = (char *)(sptr + off) - (char *)vxsr ;
	FINDAREAINPROCESS(areax,aid)
	GRABAREAMTLOCK(areax) ;
	if (v4is_Insert(aid,(struct V4IS__Key *)vxsr,vxsr,bytes,V4IS_PCB_DataMode_Auto,0,0,FALSE,FALSE,0,ctx->ErrorMsgAux) == -1) return(0) ;
	FREEAREAMTLOCK(areax) ;
	return(off) ;
}

struct V4XDB_SavedRow *v4xdb_GetXDBRow(ctx,xdb,vxssi,recId)
  struct V4C__Context *ctx ;
  struct V4XDB__Master *xdb ;
  struct V4XDB__SaveStmtInfo *vxssi ;
  XDBRECID recId ;
{
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4XDB_SavedRow *vxsr ;
  void *ptr ; LENMAX bytes ; AREAID aid ; INDEX areax ;
  
	if (vxssi->vxsr == NULL) vxssi->vxsr = (struct V4XDB_SavedRow *)v4mm_AllocChunk(sizeof *vxsr,FALSE) ;
	vxsr = vxssi->vxsr ;
	vxsr->kp.fld.KeyType = V4IS_KeyType_xdbRow ; vxsr->kp.fld.KeyMode = V4IS_KeyMode_Int ; vxsr->kp.fld.Bytes = V4IS_xdbRow_Bytes ;
//VEH110907	vxsr->kp.fld.AuxVal = vxssi->dimId ;
	vxsr->kp.fld.AuxVal = XDBIDPART(vxssi->xdbId) ;
	vxsr->recId = recId ;
	aid = gpi->RelH[V4DPI_WorkRelHNum].aid ;
	FINDAREAINPROCESS(areax,aid)
	GRABAREAMTLOCK(areax) ;
	if (v4is_PositionKey(aid,(struct V4IS__Key *)vxsr,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey)
	 { FREEAREAMTLOCK(areax) ; return(NULL) ; } ;
	ptr = (void *)v4dpi_GetBindBuf(ctx,aid,ikde,FALSE,&bytes) ;
	memcpy(vxsr,ptr,bytes) ;
	FREEAREAMTLOCK(areax) ;
	return(vxsr) ;
}

/*	V 4 I S   I N T E R F A C E			*/

P *v4im_V4ISCon(ctx,respnt,intmodx,argpnts,argcnt,trace)
  struct V4C__Context *ctx ;
  P *respnt ;						/* Ptr for result */
  INTMODX intmodx ;
  P *argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ 
  P pnt,*cpt ;
  struct V4DPI__PntV4IS *pis ;
  struct V4IS__V4Areas *v4a ;
  struct V4V4IS__RuntimeList *rtl ;
  struct V4DPI__DimInfo *di ;
  struct V4IS__ParControlBlk *pcb ;
  int ax,t,i,j,k,rx,fx ;
  struct {
    struct {
      short KeyParts ;				/* Number of key parts for this key */
      short KeyP[V4IS_V4KeyP_Max] ;		/* Field index for the part of the key */
     } Key[V4IS_V4Key_Max] ;
   } Keys ;
  int open,share,lock,put,get,fileref,keynum,ok ; UCCHAR areaname[250] ;

	ZUS(areaname) ; open = UNUSED ; share = UNUSED ; lock = UNUSED ;
	put = UNUSED ; get = UNUSED ; fileref = UNUSED ; keynum = UNUSED ; memset(&Keys,0,sizeof Keys) ;
	for(ok=TRUE,ax=2;ok&&ax<=argcnt;ax++)
	 { switch (t=v4im_CheckPtArgNew(ctx,argpnts[ax],&cpt,&pnt))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto v4iscon_fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto v4iscon_fail ;
	      case V4IM_Tag_Area:	v4im_GetPointUC(&ok,areaname,UCsizeof(areaname),cpt,ctx) ; break ;
	      case V4IM_Tag_Open:	open = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Share:	share = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_KeyNum:	keynum = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Key:
		if (keynum == UNUSED || keynum < 1 || keynum > V4IS_V4Key_Max)
		 { v_Msg(ctx,NULL,"V4ISKeyNum",intmodx,V4IM_Tag_KeyNum,keynum,V4IS_V4Key_Max) ; goto v4iscon_fail ; } ;
		if (Keys.Key[keynum-1].KeyParts >= V4IS_V4KeyP_Max)
		 { v_Msg(ctx,NULL,"V4ISKeyNumParts",intmodx,V4IM_Tag_KeyNum,keynum,Keys.Key[keynum-1].KeyParts,V4IS_V4KeyP_Max) ; goto v4iscon_fail ; } ;
		Keys.Key[keynum-1].KeyP[Keys.Key[keynum-1].KeyParts++] = v4im_GetPointInt(&ok,cpt,ctx) ;
		break ;
	      case V4IM_Tag_Lock:	lock = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Put:	put = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Get:	get = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_FileRef:	fileref = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	    } ; if (!ok) break ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax-1) ; goto v4iscon_fail ; } ;
	v4a = gpi->v4a ;					/* Pull v4a from process info */
	if (open == UNUSED) { v_Msg(ctx,NULL,"V4ISNoOpen",intmodx,V4IM_Tag_Open) ; goto v4iscon_fail ; }
	 else if (open == 3)
	  { if (v4a == NULL)  { v_Msg(ctx,NULL,"V4ISNoArea",intmodx) ; goto v4iscon_fail ; } ;
	    if (argpnts[1]->PntType != V4DPI_PntType_V4IS)
	     { v_Msg(ctx,NULL,"ModArgPntType2t",intmodx,1,argpnts[1]->PntType,V4DPI_PntType_V4IS) ; goto v4iscon_fail ; } ;
	    pis = (struct V4DPI__PntV4IS *)&argpnts[1]->Value ;	/* Link up to V4IS info */
	    logPNT(respnt) ;
	    respnt->Bytes = V4PS_Int ;
	    for(i=0;i<v4a->Count;i++) { if (pis->Handle == v4a->Area[i].Handle) break ; } ;
	    if (i >= v4a->Count)				/* Could not find area? */
	     { respnt->Value.IntVal = FALSE ; return(respnt) ; } ;
	    v4is_Close(v4a->Area[i].pcb) ;				/* Close the area */
	    v4mm_FreeChunk(v4a->Area[i].pcb) ; if (v4a->Area[i].RecBuf != NULL) v4mm_FreeChunk(v4a->Area[i].RecBuf) ;
	    v4a->Area[i] = v4a->Area[--v4a->Count] ;
	    respnt->Value.IntVal = TRUE ; return(respnt) ;
	  } ;
	ISVALIDDIM(argpnts[1]->Value.IntVal,1,"V4ISCon()") ;
	DIMINFO(di,ctx,argpnts[1]->Value.IntVal) ;
	if (di->PointType != V4DPI_PntType_V4IS)
	 { v_Msg(ctx,NULL,"ModArgPntType2t",intmodx,1,argpnts[1]->PntType,V4DPI_PntType_V4IS) ; goto v4iscon_fail ; } ;
	if (areaname[0] == UCEOS) { v_Msg(ctx,NULL,"TagMissing",intmodx,V4IM_Tag_Area) ; goto v4iscon_fail ; } ;
	if (fileref == UNUSED)
	 { v_Msg(ctx,NULL,"TagMissing",intmodx,V4IM_Tag_FileRef) ; goto v4iscon_fail ; } ;
	if (v4a == NULL)					/*  gotta allocate one first */
	 { gpi->v4a = (struct V4IS__V4Areas *)v4mm_AllocChunk(sizeof *v4a,TRUE) ;
	   v4a = gpi->v4a ;
	 } ;
	if (v4a->Count >= V4IS_V4AreaMax) { v_Msg(ctx,NULL,"V4ISMaxAreas",intmodx,V4IS_V4AreaMax) ; goto v4iscon_fail ;} ;
	i = v4a->Count ; v4a->Area[i].Handle = ++v4a->LUHandle ; v4a->Area[i].RecBuf = NULL ;
	v4a->Area[i].DataId = UNUSED ; v4a->Area[i].DimId = argpnts[1]->Value.IntVal ;
	v4a->Area[i].Count = 0 ; v4a->Area[i].SubIndex = UNUSED ; v4a->Area[i].FileRef = fileref ;
	rtl = gpi->rtl ;
	if (rtl == NULL)
	 { if (trace & V4TRACE_V4IS)
	    { v_Msg(ctx,UCTBUF1,"*V4ISNoStructWrn",v4a->Area[i].FileRef) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
	   return(NULL) ;					/* Can't find Struct/El info? */
	 } ;
	for(rx=0;rx<rtl->Count;rx++) { if (rtl->FileRefs[rx] == v4a->Area[i].FileRef) break ; } ;
	if (rx < rtl->Count) { v4a->Area[i].stel = rtl->stel[rx] ; }
	 else { v4a->Area[i].stel = (struct V4V4IS__StructEl *)v4v4is_LoadFileRef(ctx,v4a->Area[i].FileRef) ;
		if (v4a->Area[i].stel == NULL)
		 { if (trace & V4TRACE_V4IS)
		    { v_Msg(ctx,UCTBUF1,"*V4ISNoStrElWrn",v4a->Area[i].FileRef) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
		    return(NULL) ;				/* Can't find Struct/El info? */
		 } ;
	      } ;
	if (rtl->IsNew[rx]) v4v4is_FlushStEl(ctx) ;		/* If new entry then end it & flush it to V4 area */
	for(j=0;j<V4IS_V4Key_Max;j++)				/* Copy key specs into v4a */
	 { v4a->Area[i].Key[j].KeyParts = Keys.Key[j].KeyParts ;
	   for(k=0;k<Keys.Key[j].KeyParts;k++)
	    { for(fx=0;fx<v4a->Area[i].stel->Count;fx++)	/* Now look for the element */
	       { if (v4a->Area[i].stel->Field[fx].Index == Keys.Key[j].KeyP[k]) break ; } ;
	      if (fx >= v4a->Area[i].stel->Count)
	       { if (trace & V4TRACE_V4IS)
	          { v_Msg(ctx,UCTBUF1,"*V4ISNoFldXWrn",Keys.Key[j].KeyP[k],v4a->Area[i].FileRef) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
		 return(NULL) ;
	       } ;
	      v4a->Area[i].Key[j].KeyP[k] = fx ;
	    } ;
	 } ;
	pcb = (v4a->Area[i].pcb = (struct V4IS__ParControlBlk *)v4mm_AllocChunk(sizeof *v4a->Area[i].pcb,TRUE)) ;
	UCstrcpy(pcb->UCFileName,areaname) ;
	pcb->OpenMode = (open == 2 ? V4IS_PCB_OM_Update : V4IS_PCB_OM_Read) ;
	pcb->AccessMode = -1 ;					/* Enable all access */
	pcb->LockMode = (lock == 1 ? -1 : 0) ;
	v4is_Open(pcb,NULL,NULL) ;
	pcb->DfltFileRef = pcb->FileRef ;			/* ****** TEMP CODE FOR NOW ***** */
	v4a->Area[i].RecBuf = v4mm_AllocChunk(pcb->GetLen,FALSE) ;	/* Allocate for maximum record length */
	pcb->DfltGetBufPtr = v4a->Area[i].RecBuf ; pcb->DfltGetBufLen = pcb->GetLen ;
	pcb->DfltGetMode =	/* Set up defaults to read area sequentially */
	 V4IS_PCB_GP_Next + (V4IS_PCB_NoLock|V4IS_PCB_IgnoreLock|V4IS_PCB_NoError) ;
	v4a->Count++ ;						/* Open OK, make it official */
	ZPH(respnt) ; respnt->Dim = argpnts[1]->Value.IntVal ; respnt->PntType = V4DPI_PntType_V4IS ;
	respnt->Bytes = V4DPI_PointHdr_Bytes + sizeof *pis ;
	pis = (struct V4DPI__PntV4IS *)&respnt->Value ;
	pis->Handle = v4a->Area[i].Handle ; pis->DataId = UNUSED ; pis->SubIndex = UNUSED ;
	return(respnt) ;
v4iscon_fail:
	REGISTER_ERROR(0) ; return(NULL) ;
}

P *v4im_V4ISVal(ctx,respnt,intmodx,argpnts,argcnt,trace)
  struct V4C__Context *ctx ;
  P *respnt ;							/* Ptr for result */
  INTMODX intmodx ;
  P *argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ struct V4V4IS__StructEl *stel ;
  struct V4V4IS__RuntimeList *rtl ;
  struct V4DPI__PntV4IS *pis ;
  struct V4IS__V4Areas *v4a ;
  P *rpt,*vpt ; int i,ival ; char *ptr ; char *sval ;
  int fx,index,ss,*iptr,ok ; double dnum ;
  B64INT ilong ;

	ISVALIDDIM(argpnts[2]->Value.IntVal,2,"V4ISVal()") ;
	index = v4im_GetPointInt(&ok,argpnts[3],ctx) ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",index,3) ; goto v4isval_fail ; } ;
	vpt = argpnts[1] ;
	if (vpt->PntType != V4DPI_PntType_V4IS)
	 { v_Msg(ctx,NULL,"ModArgPntType2",intmodx,1,vpt->PntType,V4DPI_PntType_V4IS) ; goto v4isval_fail ; } ;
	pis = (struct V4DPI__PntV4IS *)&vpt->Value ;	/* Link up to V4IS info */
	v4a = gpi->v4a ;				/* Pull v4a from process info */
	if (v4a == NULL) { v_Msg(ctx,NULL,"V4ISNoArea",intmodx) ; goto v4isval_fail ; } ;
	for(i=0;i<v4a->Count;i++) { if (pis->Handle == v4a->Area[i].Handle) break ; } ;
	if (i >= v4a->Count)				/* Could not find area? */
	 { v_Msg(ctx,NULL,"V4ISNotFound",intmodx) ; goto v4isval_fail ; } ;
	if (v4a->Area[i].DataId == UNUSED || (pis->DataId != UNUSED && pis->DataId != v4a->Area[i].DataId))
	 { v_Msg(ctx,NULL,"V4ISNoRecord",intmodx) ; goto v4isval_fail ; } ;
	if (gpi->rtl == NULL) v4v4is_LoadFileRef(ctx,v4a->Area[i].FileRef) ;
	rtl = gpi->rtl ;
	stel = v4a->Area[i].stel ;
	for(fx=(index < 1 ? 99999 : index-1);fx<stel->Count;fx++) /* Now look for the element - make guess at starting point */
	 { if (stel->Field[fx].Index == index) break ; } ;
	if (fx >= stel->Count)				/* That didn't work, try from begin */
	 { for(fx=0;fx<stel->Count;fx++)			/* Now look for the element */
	    { if (stel->Field[fx].Index == index) break ; } ;
	   if (fx >= stel->Count)
	    { v_Msg(ctx,NULL,"V4ISNoFldIndex",intmodx,index,v4a->Area[i].FileRef) ; goto v4isval_fail ; } ;
	 } ;
	if (stel->Field[fx].StructNum > 0)		/* Have nested structure? */
	 { if (argcnt < 4)
	    { if (pis->SubIndex == UNUSED) { v_Msg(ctx,NULL,"ModArgMissing",intmodx,4,argcnt) ; goto v4isval_fail ; } ;
	      ss = pis->SubIndex ;
	    } else { ss = v4im_GetPointInt(&ok,argpnts[4],ctx) ; if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,4) ; goto v4isval_fail ; } ; } ;
	   ptr = v4a->Area[i].RecBuf + stel->Field[fx].Offset + (stel->Field[fx].SubBytes * (ss-1)) ;
	 } else { ptr = v4a->Area[i].RecBuf + stel->Field[fx].Offset ; } ;
	switch(stel->Field[fx].V3DataType)		/* Maybe do something based on internal data representation */
	 { default:
		v_Msg(ctx,NULL,"V4ISV3DT",intmodx,stel->Field[fx].V3DataType) ;	goto v4isval_fail ;
	   case VFT_BININT:				/* Int - convert to double if decimals */
		if (stel->Field[fx].Decimals == 0)
		 { rpt = v4im_AggUpdRes(ctx,respnt,argpnts[2]->Value.IntVal,ptr,stel->Field[fx].Bytes,stel->Field[fx].Bytes,0) ; break ; } ;
		iptr = (int *)ptr ; dnum = (double)*iptr ; /* Have to convert */
		dnum /= powers[stel->Field[fx].Decimals] ;
		rpt = v4im_AggUpdRes(ctx,respnt,argpnts[2]->Value.IntVal,&dnum,sizeof dnum,sizeof dnum,0) ; break ;
	   case VFT_FIXSTRSK:
	   case VFT_FIXSTR:				/* Fixed strings - go as is */
		rpt = v4im_AggUpdRes(ctx,respnt,argpnts[2]->Value.IntVal,ptr,stel->Field[fx].Bytes,stel->Field[fx].Bytes,0) ; break ;
	   case VFT_BINWORD:				/* Short - convert to int, maybe double if have decimals */
		sval = ptr ; ival = *sval ; ptr = (char *)&ival ;	/* Move short into int */
		if (stel->Field[fx].Decimals == 0)
		 { rpt = v4im_AggUpdRes(ctx,respnt,argpnts[2]->Value.IntVal,ptr,sizeof(int),sizeof(int),0) ; break ; } ;
		dnum = (double)ival ;		/* Have to convert */
		dnum /= powers[stel->Field[fx].Decimals] ;
		rpt = v4im_AggUpdRes(ctx,respnt,argpnts[2]->Value.IntVal,&dnum,sizeof dnum,sizeof dnum,0) ; break ;
	   case VFT_FLOAT:
		rpt = v4im_AggUpdRes(ctx,respnt,argpnts[2]->Value.IntVal,ptr,sizeof(int),sizeof(int),0) ; break ;
	   case VFT_VARSTR:				/* Variable strings - figure out length */
		rpt = v4im_AggUpdRes(ctx,respnt,argpnts[2]->Value.IntVal,ptr,strlen(ptr),strlen(ptr),0) ; break ;
	   case VFT_STRINT:				/* Convert zoned decimal to double */
		dnum = 0 ;
		for(i=1;i<stel->Field[fx].Bytes;i++)	/* Loop thru all but last byte */
		 { dnum += *ptr - '9' ; ptr++ ; dnum *= 10.0 ; } ;
		if (*ptr <= '9') { dnum += (double)(*ptr) ; }
		 else { dnum += (double)(*ptr - '0') ; dnum = -dnum ; } ;
		dnum /= powers[stel->Field[fx].Decimals] ;
		rpt = v4im_AggUpdRes(ctx,respnt,argpnts[2]->Value.IntVal,&dnum,sizeof dnum,sizeof dnum,0) ; break ;
	   case VFT_BINLONG:
		memcpy(&ilong,ptr,sizeof ilong) ; dnum = (double)ilong ;
		dnum /= powers[stel->Field[fx].Decimals] ;
		rpt = v4im_AggUpdRes(ctx,respnt,argpnts[2]->Value.IntVal,&dnum,sizeof dnum,sizeof dnum,0) ; break ;
	 } ;
	if (rpt == NULL) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto v4isval_fail ; } ;
	return(rpt) ;
v4isval_fail:
	REGISTER_ERROR(0) ; return(NULL) ;
}

P *v4im_V4ISOp(ctx,respnt,intmodx,argpnts,argcnt,trace)
  struct V4C__Context *ctx ;
  P *respnt ;						/* Ptr for result */
  INTMODX intmodx ;
  P *argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ 
  P pnt,*cpt ;
  int ax,t,i ;
  struct V4DPI__PntV4IS *pis ;
  struct V4V4IS__StructEl *stel ;
  struct V4IS__ParControlBlk *pcb ;
  struct V4IS__V4Areas *v4a ;
  int k,fx,keynum,keyparts,lock,put,get,len,ok,dataid ; P *keys[V4IS_V4KeyP_Max],*vpt ; char *ptr,*dptr ; int *iptr ;

	vpt = argpnts[1] ;
	if (vpt->PntType != V4DPI_PntType_V4IS)
	 { v_Msg(ctx,NULL,"ModArgPntType2",intmodx,1,vpt->PntType,V4DPI_PntType_V4IS) ; goto v4isop_fail ; } ;
	pis = (struct V4DPI__PntV4IS *)&argpnts[1]->Value ;	/* Link up to V4IS info */
	v4a = gpi->v4a ;		/* Pull v4a from process info */
	if (v4a == NULL) { v_Msg(ctx,NULL,"V4ISNoArea",intmodx) ; goto v4isop_fail ; } ;
	for(i=0;i<v4a->Count;i++) { if (pis->Handle == v4a->Area[i].Handle) break ; } ;
	if (i >= v4a->Count)	/* Could not find area? */
	 return(NULL) ;
	pcb = v4a->Area[i].pcb ; stel = v4a->Area[i].stel ;
	keynum = UNUSED ; lock = UNUSED ; put = UNUSED ; get = UNUSED ; keyparts = UNUSED ; dataid = UNUSED ;
	for(ok=TRUE,ax=2;ok&&ax<=argcnt;ax++)
	 { switch (t=v4im_CheckPtArgNew(ctx,argpnts[ax],&cpt,&pnt))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto v4isop_fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto v4isop_fail ;
	      case V4IM_Tag_DataId:	dataid = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case -V4IM_Tag_DataId:	dataid = 0 ; break ;
	      case V4IM_Tag_KeyNum:	keynum = v4im_GetPointInt(&ok,cpt,ctx) ; keyparts = 0 ; break ;
	      case V4IM_Tag_Key:
		if (keynum < 1 || keynum > V4IS_V4Key_Max)
		 { v_Msg(ctx,NULL,"V4ISKeyNum",intmodx,V4IM_Tag_KeyNum,keynum,V4IS_V4Key_Max) ; goto v4isop_fail ; } ;
		if (keyparts >= V4IS_V4KeyP_Max)
		 { v_Msg(ctx,NULL,"V4ISMaxKeyParts",intmodx,V4IS_V4KeyP_Max,V4IM_Tag_KeyNum,keynum) ; goto v4isop_fail ; } ;
		keys[keyparts++] = cpt ; break ;
	      case V4IM_Tag_Lock:	lock = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Put:	put = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Get:
		if (cpt->PntType == V4DPI_PntType_Int) { get = v4im_GetPointInt(&ok,cpt,ctx) ; break ; } ;
		if (cpt->PntType != V4DPI_PntType_Dict) { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"@Invalid Get::xxx argument") ; break ; } ;
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:		ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"@Invalid Get::xxx argument") ; break ;
		   case _Keyed:		get = V4IS_PCB_GP_Keyed ; break ;
		   case _KeyedPartial:	get = V4IS_PCB_GP_KeyedNext ; break ;
		   case _Next:		get = V4IS_PCB_GP_Next ; break ;
		   case _BOF:		get = V4IS_PCB_GP_BOF ; break ;
		 } ;
		break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax-1) ; goto v4isop_fail ; } ;
	if (put != UNUSED) goto do_put ;
	if (dataid > 0) get = V4IS_PCB_GP_DataId ;
	if (keynum != UNUSED)
	 { if (keyparts != v4a->Area[i].Key[keynum-1].KeyParts)
	    { v_Msg(ctx,NULL,"V4ISKeyNumMatch",intmodx,v4a->Area[i].Key[keynum-1].KeyParts,V4IM_Tag_KeyNum,keynum,keyparts) ;
	      goto v4isop_fail ;
	    } ;
	   if (get == UNUSED) get = V4IS_PCB_GP_Keyed ;
	   for(k=0;k<keyparts;k++)			/* Take care of the key(s) */
	    { fx = v4a->Area[i].Key[keynum-1].KeyP[k] ;
	      ptr = v4a->Area[i].RecBuf + stel->Field[fx].Offset ;
	      switch(stel->Field[fx].V3DataType)
	       { default:		break ;
		 case VFT_BININT:	iptr = (int *)ptr ; *iptr = v4im_GetPointInt(&ok,keys[k],ctx) ;
					if (!ok) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,keys[k]) ; goto v4isop_fail ; } ;
					break ;
		 case VFT_FIXSTRSK:
		 case VFT_VARSTR:
		 case VFT_FIXSTR:
			switch (keys[k]->PntType)
			 { default:
				memset(ptr,' ',stel->Field[fx].Bytes) ; t = keys[k]->Value.AlphaVal[0] ;
				strncpy(ptr,&keys[k]->Value.AlphaVal[1],(t < stel->Field[fx].Bytes ? t : stel->Field[fx].Bytes)) ;
				break ;
			   case V4DPI_PntType_Color:
			   case V4DPI_PntType_Country:
			   case V4DPI_PntType_CodedRange:
			   case V4DPI_PntType_Int:	/* If integer -> alpha then assume V3 "dec" format */
				sprintf(ptr,"%0*d",stel->Field[fx].Bytes,v4im_GetPointInt(&ok,keys[k],ctx)) ;
				if (!ok) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,keys[k]) ; goto v4isop_fail ; } ;
				break ;
			   case V4DPI_PntType_XDict:
				memset(ptr,' ',stel->Field[fx].Bytes) ;
				dptr = UCretASC(v4dpi_RevXDictEntryGet(ctx,keys[k]->Dim,keys[k]->Value.IntVal)) ; len = strlen(dptr) ;
				strncpy(ptr,dptr,(len <= stel->Field[fx].Bytes ? len : stel->Field[fx].Bytes)) ;
				break ;
			   case V4DPI_PntType_Dict:
				memset(ptr,' ',stel->Field[fx].Bytes) ;
				dptr = UCretASC(v4dpi_RevDictEntryGet(ctx,keys[k]->Value.IntVal)) ; len = strlen(dptr) ;
				strncpy(ptr,dptr,(len <= stel->Field[fx].Bytes ? len : stel->Field[fx].Bytes)) ;
				break ;
			  } ; break ;
		 case VFT_STRINT:	sprintf(ptr,"%0*d",stel->Field[fx].Bytes,v4im_GetPointInt(&ok,keys[k],ctx)) ;
					if (!ok) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,keys[k]) ; goto v4isop_fail ; } ;
					break ;
		 case VFT_BINWORD:
		 case VFT_FLOAT:
		 case VFT_BINLONG:	return(NULL) ;
	       } ;
	    } ;
	 } ;
	switch (get)
	 { case UNUSED:				break ;
	   case V4IS_PCB_GP_KeyedNext:
	   case V4IS_PCB_GP_Keyed:		pcb->KeyNum = keynum ; pcb->GetMode = get | V4IS_PCB_NoError ; break ;
	   case V4IS_PCB_GP_BOF:
	   case V4IS_PCB_GP_Next:		pcb->GetMode = get | V4IS_PCB_NoError ; break ;
	   case V4IS_PCB_GP_DataId:		pcb->GetMode = get | V4IS_PCB_NoError ; pcb->DataId = dataid ; break ;
	   case V4IS_PCB_GP_Prior:
	   case V4IS_PCB_GP_EOF:
	   case V4IS_PCB_GP_Append:
	   case V4IS_PCB_GP_KeyedPrior:
	   case V4IS_PCB_GP_Delete:
	   case V4IS_PCB_GP_Update:
	   case V4IS_PCB_GP_Insert:
	   case V4IS_PCB_GP_Write:
	   case V4IS_PCB_GP_DataOnly:
	   case V4IS_PCB_GP_Unlock:
	   case V4IS_PCB_GP_NextNum1:
	   case V4IS_PCB_GP_Cache:
	   case V4IS_PCB_GP_Reset:
	   case V4IS_PCB_GP_Obsolete:
	   default:				v_Msg(ctx,NULL,"V4ISGetVal",intmodx,V4IM_Tag_Get,get) ; goto v4isop_fail ;
	 } ;

	if (dataid == 0)				/* Only wants DataId, no data */
	 { pcb->GetMode |= V4IS_PCB_LengthOnly ; } ;
	v4is_Get(pcb) ;					/* Do the get */
	if (pcb->GetLen == UNUSED)			/* Got an error? */
	 { v4a->Area[i].DataId = UNUSED ; v_Msg(ctx,NULL,"V4ISGetFail",intmodx) ; goto v4isop_fail ; } ;
	if (dataid == 0)
	 { intPNTv(respnt,pcb->DataId) ; return(respnt) ; } ;
	ZPH(respnt) ; respnt->Dim = vpt->Dim ; respnt->PntType = V4DPI_PntType_V4IS ;
	respnt->Bytes = V4DPI_PointHdr_Bytes + sizeof *pis ;
	pis = (struct V4DPI__PntV4IS *)&respnt->Value ;
	pis->Handle = v4a->Area[i].Handle ; pis->DataId = pcb->DataId ; pis->SubIndex = UNUSED ;
	v4a->Area[i].DataId = pcb->DataId ;
	return(respnt) ;

do_put:
	return(respnt) ;

v4isop_fail:
	REGISTER_ERROR(0) ; return(NULL) ;
}


/*	v4im_DoOSExt - Handles the OSExt() intmod		*/

#define OSHType_Library 1		/* Handle references a library */
#define OSHType_Module 2		/* Handle references a module within a library */

#define OSHExt_MaxArgs 32		/* Max number of arguments allowed */

struct V4OSH__Library {
#ifdef WINNT
 HINSTANCE hLib ;
#endif
 UCCHAR LibName[128] ;
} ;

typedef double (*DBLPROC)() ;
typedef int (*INTPROC)() ;

struct V4OSH__Module {
 UCCHAR ModName[32] ;
#ifdef WINNT
 DBLPROC ModAddressDbl ;		/* Address of module */
 INTPROC ModAddressInt ;		/* Address of module */
#endif
 int ResPntType ;			/* PointType of result */
 int ResDim ;
 int ArgCnt ;
 int ArgPntType[OSHExt_MaxArgs] ;	/* PointTypes of arguments */
} ;


P *v4im_DoOSExt(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt,*argpnts[] ;
  int argcnt,intmodx,trace ;
{
#ifdef WINNT
  P *ipt,spnt ;
  struct V4OSH__Library *lib ;
  struct V4OSH__Module *mod ;
  struct V4DPI__DimInfo *di,*dios,*resargs[OSHExt_MaxArgs] ;
  union LCL__Args {
    struct { char b8[8] ; } a8 ;
    struct { char b16[16] ; } a16 ;
    struct { char b24[24] ; } a24 ;
    struct { char b32[32] ; } a32 ;
    struct { char b48[48] ; } a48 ;
    struct { char b64[64] ; } a64 ;
   } la ; char *ap ; int *ip ;
  UCCHAR library[UCsizeof(lib->LibName)],module[UCsizeof(mod->ModName)] ;
  int i,racnt,osh,oshm,b,al,ok ; double dnum ;

	ZUS(library) ; ZUS(module) ; i = 1 ; dios = NULL ; racnt = 0 ; osh = UNUSED ; ok = TRUE ;
	if (argpnts[i]->PntType == V4DPI_PntType_OSHandle)
	 { osh = argpnts[i]->Value.IntVal ;		/* Get handle to OS element */
	   i++ ;
	 } ;
	if (osh == UNUSED ? FALSE : han_GetInfo(osh) == OSHType_Module)		/* If first arg is module then we got a call to do */
	 { mod = (struct V4OSH__Module *)han_GetPointer(osh,0) ;
	   if (argcnt-1 != mod->ArgCnt) { v_Msg(ctx,NULL,"OSModArgNum",intmodx,mod->ModName,mod->ArgCnt,argcnt-1) ; goto fail ; } ;
	   for(al=0,ap=(char *)&la.a64;ok&&i<=argcnt;i++)
	    { switch(mod->ArgPntType[i-2])
	       { default:
	         case V4DPI_PntType_CodedRange:
		 case V4DPI_PntType_Int:
			ip = (int *)ap ; *ip = v4im_GetPointInt(&ok,argpnts[i],ctx) ; b = sizeof(int) ; break ;
		 case V4DPI_PntType_Real:
			dnum = v4im_GetPointDbl(&ok,argpnts[i],ctx) ;
			memcpy(ap,&dnum,sizeof(dnum)) ; b = sizeof(dnum) ; break ;
	       } ;
	      ap += b ; al += b ;
	    } ;
	   if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i) ; ipt = NULL ; goto fail ; } ;
	   switch (mod->ResPntType)
	    { case V4DPI_PntType_Int:
		intPNT(respnt) respnt->Dim = mod->ResDim ;
		if (al <= 8) { respnt->Value.IntVal = (int)(mod->ModAddressInt)(la.a8) ; }
		 else if (al <= 16) { respnt->Value.IntVal = (int)(mod->ModAddressInt)(la.a16) ; }
		 else if (al <= 24) { respnt->Value.IntVal = (int)(mod->ModAddressInt)(la.a24) ; }
		 else if (al <= 32) { respnt->Value.IntVal = (int)(mod->ModAddressInt)(la.a32) ; }
		 else if (al <= 48) { respnt->Value.IntVal = (int)(mod->ModAddressInt)(la.a48) ; }
		 else if (al <= 64) { respnt->Value.IntVal = (int)(mod->ModAddressInt)(la.a64) ; } ;
		return(respnt) ;
	      case V4DPI_PntType_Real:
		dblPNT(respnt) ; respnt->Dim = mod->ResDim ;
		if (al <= 8) { dnum = (double)(mod->ModAddressDbl)(la.a8) ; }
		 else if (al <= 16) { dnum = (double)(mod->ModAddressDbl)(la.a16) ; }
		 else if (al <= 24) { dnum = (double)(mod->ModAddressDbl)(la.a24) ; }
		 else if (al <= 32) { dnum = (double)(mod->ModAddressDbl)(la.a32) ; }
		 else if (al <= 48) { dnum = (double)(mod->ModAddressDbl)(la.a48) ; }
		 else if (al <= 64) { dnum = (double)(mod->ModAddressDbl)(la.a64) ; } ;
		PUTREAL(respnt,dnum) ;
		return(respnt) ;
	    } ;
	 } ;
	for(;ok&&i<=argcnt;i++)
	 { 
	   switch (v4im_CheckPtArgNew(ctx,argpnts[i],&ipt,&spnt))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ;
	      case -V4IM_Tag_Error:	memcpy(NULL,"hohoho",5) ; break ;
	      case V4IM_Tag_Library:	v4im_GetPointUC(&ok,library,UCsizeof(library),ipt,ctx) ; continue ;
	      case V4IM_Tag_Dim:
		ISVALIDDIM(ipt->Value.IntVal,i,"OSExt()") ;
		DIMINFO(di,ctx,ipt->Value.IntVal) ;
		if (di == NULL) { v_Msg(ctx,NULL,"ModArgNotDim",intmodx,i) ; goto fail ; } ;
		if (i == 1) { dios = di ; }
		 else { if (racnt >= OSHExt_MaxArgs)
			 { v_Msg(ctx,NULL,"MaxNumModArgs",intmodx,OSHExt_MaxArgs) ; goto fail ; } ;
		        resargs[racnt++] = di ;
		      } ;
		break ;
	      case V4IM_Tag_Module:	v4im_GetPointUC(&ok,module,UCsizeof(module),ipt,ctx) ; continue ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; ipt = NULL ; goto fail ; } ;

/*	Now have to figure out what to do - get library, module, whatever... */
	if (dios == NULL && UCnotempty(library)) { v_Msg(ctx,NULL,"OSExtLib",intmodx) ; goto fail ; } ;
	if (UCnotempty(library))
	 { lib = (struct V4OSH__Library *)v4mm_AllocChunk(sizeof *lib,FALSE) ;
	   UCstrcpy(lib->LibName,library) ;
	   osh = han_Make() ; han_SetPointer(osh,0,lib) ; han_SetInfo(osh,OSHType_Library) ;
#ifdef WINNT
	   lib->hLib = LoadLibrary(UCretASC(library)) ;
	   if (lib->hLib == 0) { v_Msg(ctx,NULL,"OSExtNoLib",intmodx,library) ; goto fail ; } ;
#endif
	   if (UCempty(module))
	    { ZPH(respnt) ; respnt->Dim = dios->DimId ; respnt->PntType = dios->PointType ; respnt->Bytes = V4PS_Int ;
	      respnt->Value.IntVal = osh ;
	      return(respnt) ;
	    } ;
	 } ;
	if (UCnotempty(module))
	 { if (osh == UNUSED ? TRUE : han_GetInfo(osh) != OSHType_Library)
	   lib = (struct V4OSH__Library *)han_GetPointer(osh,0) ;
	   mod = (struct V4OSH__Module *)v4mm_AllocChunk(sizeof *mod,FALSE) ;
	   oshm = han_Make() ; han_SetPointer(oshm,0,mod) ; han_SetInfo(oshm,OSHType_Module) ;
	   UCstrcpy(mod->ModName,module) ;
	   if (racnt <= 0) { v_Msg(ctx,NULL,"OSExtMissing",intmodx) ; goto fail ; } ;
	   mod->ResPntType = resargs[0]->PointType ; mod->ResDim = resargs[0]->DimId ;
	   for(i=0;i<racnt;i++) { mod->ArgPntType[i] = resargs[i]->PointType ; } ;
	   mod->ArgCnt = racnt - 1 ;			/* Subtract one because first dimension is return result */
#ifdef WINNT
	   mod->ModAddressInt = (INTPROC)GetProcAddress(lib->hLib,UCretASC(module)) ;
	   mod->ModAddressDbl = (DBLPROC)mod->ModAddressInt ;
#else
	   mod->ModAddressInt = NULL ;			/* No support yet */
#endif
	   if (mod->ModAddressInt == NULL) { v_Msg(ctx,NULL,"OSExtNoMod",intmodx,V4IM_Tag_Module,mod->ModName,V4IM_Tag_Library,lib->LibName) ; goto fail ; } ;
	   ZPH(respnt) ; respnt->Dim = dios->DimId ; respnt->PntType = dios->PointType ; respnt->Bytes = V4PS_Int ;
	   respnt->Value.IntVal = oshm ;
	   return(respnt) ;
	 } ;
#endif
	v_Msg(ctx,NULL,"OSExtNoResult",intmodx,V4IM_Tag_Module,V4IM_Tag_Library) ;
fail:	REGISTER_ERROR(0) ; return(NULL) ;
}

/*	v4im_DoOSInfo - Does/Returns O/S related stuff	*/

P *v4im_DoOSInfo(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt,*argpnts[] ;
  int argcnt,intmodx,trace ;
{ 
  struct V4DPI__DimInfo *di ;
  P *cpt,spnt ;
  struct V4L__ListPoint *lp ;
#ifdef VMSOS
  char *str_ptr ;
#endif
#ifdef WINNT
  UINT priorMode ;
#endif
  enum DictionaryEntries deval ;
  extern UCCHAR **startup_envp ;
  int i,len,ok,start,type ; char *bp,buf[256],volume[V_FileName_Max] ; double dnum ;
  UCCHAR *ubp, ubuf[256] ;


	ZS(volume) ;

	for(i=1,ok=TRUE;ok&&i<=argcnt;i++)
	 { 
	   switch (v4im_CheckPtArgNew(ctx,argpnts[i],&cpt,&spnt))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto osinfo_fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto osinfo_fail ;
	      case -V4IM_Tag_Bits:
		intPNTv(respnt,(sizeof &type == 4 ? 32 : 64)) ; return(respnt) ;
	      case -V4IM_Tag_Bytes:
		if (strlen(volume) == 0) { v_Msg(ctx,NULL,"OSInfoNoVol",intmodx,V4IM_Tag_Volume) ; goto osinfo_fail ; } ;
		INITLP(respnt,lp,Dim_List) ;
#ifdef WINNT
		{ ULARGE_INTEGER avail,total,totfree ;
		  priorMode = SetErrorMode(SEM_FAILCRITICALERRORS) ;
		  if (!GetDiskFreeSpaceEx(volume,&avail,&total,&totfree))
		   { SetErrorMode(priorMode) ; v_Msg(ctx,NULL,"OSInfoOSErr",intmodx,i,argpnts[i],GetLastError()) ; goto osinfo_fail ; } ;
		  SetErrorMode(priorMode) ;
		  dnum = (__int64)total.QuadPart / 1000000.0 ; dblPNTv(&spnt,dnum) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&spnt,0) ;
		  dnum = (__int64)totfree.QuadPart / 1000000.0 ; dblPNTv(&spnt,dnum) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&spnt,0) ;
		  dnum = (__int64)avail.QuadPart / 1000000.0 ; dblPNTv(&spnt,dnum) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&spnt,0) ;
		}
#else
		dnum = -1 ; dblPNTv(&spnt,dnum) ;
		v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&spnt,0) ;
		v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&spnt,0) ;
		v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&spnt,0) ;

#endif
		ENDLP(respnt,lp) ; return(respnt) ;
	      case -V4IM_Tag_ComFileExt:
#ifdef WINNT
		alphaPNTv(respnt,"cmd") ;
#elif defined UNIX
		alphaPNTv(respnt,"ucf") ;
#elif defined VMS
		alphaPNTv(respnt,"com") ;
#else
		alphaPNTv(respnt,"xxx") ;
#endif
		 break ;
	      case -V4IM_Tag_CPU:
		DIMINFO(di,ctx,Dim_UV4) ; dictPNTv(respnt,Dim_UV4,v4dpi_DictEntryGet(ctx,0,ASCretUC(V3_CONFIG_BOX),di,NULL)) ;
		return(respnt) ;
	      case V4IM_Tag_CommandLine:
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:	v_Msg(ctx,NULL,"ModTagValue",intmodx,i,V4IM_Tag_CommandLine,cpt) ; goto osinfo_fail ;
		   case _All:	start = 0 ; goto return_command_line ; break ;
		 } ;
	      case -V4IM_Tag_CommandLine:
		if (gpi->v4argx == UNUSED) { v_Msg(ctx,NULL,"OSInfoNoCmd",intmodx) ; goto osinfo_fail ; } ;
		start = gpi->v4argx ;
return_command_line:
		INITLP(respnt,lp,Dim_List) ; uccharPNT(&spnt) ;
		for(i=start;i<gpi->argc;i++)
		 { ubp = gpi->ucargv[i] ;	/* ubp = pointer to next item in environment */
		   if (ubp == NULL) break ;
		   len = UCstrlen(ubp) ; if (len > UCsizeof(spnt.Value.UCVal)-2) len = UCsizeof(spnt.Value.UCVal) - 5 ;
		   UCstrncpy(&spnt.Value.UCVal[1],ubp,len) ; UCCHARPNTBYTES2(&spnt,len) ;
		   if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&spnt,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto osinfo_fail ; } ;
		 } ;
		ENDLP(respnt,lp) ;
		return(respnt) ;
	      case -V4IM_Tag_Endian:
		dictPNTv(respnt,Dim_NId,v4im_GetEnumToDictVal(ctx,(ISLITTLEENDIAN ? (deval=_Little) : (deval=_Big)),Dim_NId)) ;
		return(respnt) ;
	      case V4IM_Tag_Environment:
		v4im_GetPointUC(&ok,ubuf,UCsizeof(ubuf),cpt,ctx) ; if (!ok) break ;
		if (!v_UCGetEnvValue(ubuf,&respnt->Value.UCVal[1],V4DPI_UCVal_Max-1))
		 { v_Msg(ctx,NULL,"OSInfoNoEnvVar",intmodx,ubuf) ; goto osinfo_fail ; } ;
		uccharPNT(respnt) ; UCCHARPNTBYTES1(respnt) ; return(respnt) ;
	      case -V4IM_Tag_Environment:
list_environment:
		INITLP(respnt,lp,Dim_List) ;
#ifdef WINNT
		uccharPNT(&spnt) ;
		for(i=0;;i++)
		 { UCCHAR *def,*bp ;
		   def = startup_envp[i] ;	/* def = pointer to next item in environment */
		   if (def == NULL) break ;
		   bp = UCstrchr(def,'=') ; if (bp == NULL) bp = &def[UCstrlen(def)] ;
		   len = bp - def ; if (len >= V4DPI_UCVal_Max) len = V4DPI_UCVAL_MaxSafe ;
		   UCstrncpy(&spnt.Value.UCVal[1],def,len) ;
		   UCCHARPNTBYTES2(&spnt,len) ;
		   if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&spnt,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto osinfo_fail ; } ;
		 } ;
#else
		alphaPNT(&spnt) ;
		for(i=0;;i++)
		 { char *def = startup_envp[i] ;	/* def = pointer to next item in environment */
		   if (def == NULL) break ;
		   bp = (char *)strchr(def,'=') ; if (bp == NULL) bp = &def[strlen(def)] ;
		   len = bp - def ; if (len >= V4DPI_AlphaVal_Max) len = V4DPI_AlphaVal_Max - 1 ;
		   strncpy(&spnt.Value.AlphaVal[1],def,len) ;
		   CHARPNTBYTES2(&spnt,len)
		   if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&spnt,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto osinfo_fail ; } ;
		 } ;
#endif
		ENDLP(respnt,lp) ;
		return(respnt) ;
	      case -V4IM_Tag_IPAddress:
#define WANTIPV6 TRUE
#ifndef WANTIPV6
{struct hostent *hp; char szHost[256]; // should be big enough. 

		type = 1 ;
ipaddress:
		SOCKETINIT
		memset(szHost,0,sizeof szHost) ; i = gethostname(szHost,sizeof szHost) ;
		if (i != 0) { v_Msg(ctx,NULL,"SocketGetHost",intmodx,NETERROR) ; goto osinfo_fail ; } ;
		hp = gethostbyname(szHost) ;
		if (hp == NULL) { v_Msg(ctx,NULL,"SocketGHbyN",intmodx,NETERROR,szHost) ; goto osinfo_fail ; } ;
#define MASKBYTE(b) (((int)hp->h_addr[b])&0xff)
		switch (type)
		 { case 1:
			sprintf(&respnt->Value.AlphaVal[1],"%d.%d.%d.%d", MASKBYTE(0),MASKBYTE(1),MASKBYTE(2),MASKBYTE(3));
			alphaPNT(respnt) ; CHARPNTBYTES1(respnt) ; break ;
		   case 2:
			intPNTv(respnt,(MASKBYTE(0) << 24) | (MASKBYTE(1) << 16) | (MASKBYTE(2) << 8) | MASKBYTE(3)) ; break ;
		 } ;
}
		return(respnt) ;
#else
		if (!v_ipLookup(ctx,intmodx,UClit(""),UClit("80"),SOCK_STREAM,V_IPLookup_IPV4,&i,UCTBUF1,NULL)) goto osinfo_fail ;
		uccharPNTv(respnt,UCTBUF1) ; return(respnt) ;
#endif
	      case V4IM_Tag_IPAddress:
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:		v_Msg(ctx,NULL,"OSInfoIPArg",intmodx,i,cpt,V4IM_Tag_IPAddress) ; goto osinfo_fail ;
		   case _V4:
#ifndef WANTIPV6
			type = 1 ; goto ipaddress ;
#else
			if (!v_ipLookup(ctx,intmodx,UClit(""),UClit("80"),SOCK_STREAM,V_IPLookup_IPV4,&i,UCTBUF1,respnt)) goto osinfo_fail ;
//			uccharPNTv(respnt,UCTBUF1) ;
			return(respnt) ;
#endif
		   case _V6:
#ifndef WANTIPV6
			v_Msg(ctx,NULL,"OSInfoNYI",intmodx) ; goto osinfo_fail ;
#else
			if (!v_ipLookup(ctx,intmodx,UClit(""),UClit("80"),SOCK_STREAM,V_IPLookup_IPV6,&i,UCTBUF1,respnt)) goto osinfo_fail ;
//			uccharPNTv(respnt,UCTBUF1) ;
			return(respnt) ;
#endif
		 } ;
		break ;
	      case -V4IM_Tag_Id:
		if (strlen(volume) == 0)
		 { intPNTv(respnt,CURRENTPID) ; return(respnt) ; } ;
#ifdef WINNT
		priorMode = SetErrorMode(SEM_FAILCRITICALERRORS) ;
		if (!GetVolumeInformation(volume,NULL,0,&i,NULL,NULL,NULL,0))
		 { SetErrorMode(priorMode) ; v_Msg(ctx,NULL,"OSInfoOSErr",intmodx,i,argpnts[i],GetLastError()) ; goto osinfo_fail ; } ;
		SetErrorMode(priorMode) ;
#else
		v_Msg(ctx,NULL,"OSInfoNYI",intmodx) ; goto osinfo_fail ;
#endif
		intPNTv(respnt,i) ; return(respnt) ;
	      case V4IM_Tag_ListOf:
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:		v_Msg(ctx,NULL,"OSInfoListArg",intmodx,i,cpt,V4IM_Tag_ListOf) ; goto osinfo_fail ;
		   case _Environment:	goto list_environment ;
		   case _Volumes:
list_volumes:
			INITLP(respnt,lp,Dim_List) ;
#ifdef WINNT
			{ char dbuf[1024] ; len = GetLogicalDriveStrings(sizeof dbuf,dbuf) ;
			  for(bp=dbuf;*bp!='\0';bp+=(strlen(bp)+1))
			   { alphaPNTv(&spnt,bp) ; if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&spnt,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto osinfo_fail ; } ; } ;
			}
#endif
			ENDLP(respnt,lp) ;  return(respnt) ;
		 } ;

	      case -V4IM_Tag_Name:
		if (strlen(volume) == 0)		/* If no Volume:xxx then return host name */
		 { SOCKETINIT
		   alphaPNT(respnt) ;
		   i = gethostname(&respnt->Value.AlphaVal[1],(sizeof respnt->Value.AlphaVal)-10) ;
		   if (i != 0) { v_Msg(ctx,NULL,"SocketGetHost",intmodx,NETERROR) ; goto osinfo_fail ; } ;
		   CHARPNTBYTES1(respnt) ; return(respnt) ;
		 } ;
#ifdef WINNT
		priorMode = SetErrorMode(SEM_FAILCRITICALERRORS) ;
		if (!GetVolumeInformation(volume,buf,sizeof buf,NULL,NULL,NULL,NULL,0))
		 { SetErrorMode(priorMode) ; v_Msg(ctx,NULL,"OSInfoOSErr",intmodx,i,argpnts[i],GetLastError()) ; goto osinfo_fail ; } ;
		SetErrorMode(priorMode) ;
		bp = buf ;
#else
		v_Msg(ctx,NULL,"OSInfoNYI",intmodx) ; goto osinfo_fail ;
#endif
		alphaPNTv(respnt,bp) ; return(respnt) ;
	      case -V4IM_Tag_Type:
		if (strlen(volume) == 0) { v_Msg(ctx,NULL,"OSInfoNoVol",intmodx,V4IM_Tag_Volume) ; goto osinfo_fail ; } ;
		dictPNT(respnt,Dim_UV4) ; DIMINFO(di,ctx,respnt->Dim) ;
#ifdef WINNT
		i = GetDriveType(volume) ;
		switch (i)
		 { default:			deval = _Undefined ; break ;
		   case DRIVE_REMOVABLE:	deval = _Removable ; break ;
		   case DRIVE_FIXED:		deval = _Fixed ; break ;
		   case DRIVE_REMOTE:		deval = _Network ; break ;
		   case DRIVE_CDROM:		deval = _CDRom ; break ;
		   case DRIVE_RAMDISK:		deval = _RamDisk ; break ;
		 } ;
#else
		v_Msg(ctx,NULL,"OSInfoNYI",intmodx) ; goto osinfo_fail ;
#endif
		respnt->Value.IntVal = v4im_GetEnumToDictVal(ctx,deval,Dim_UV4) ; ;
		return(respnt) ;
	      case -V4IM_Tag_User:

#ifdef VMSOS
		alphaPNTv(respnt,cuserid(NULL)) ;
#endif
#ifdef WINNT
		alphaPNT(respnt) ; len = 500 ; GetUserName(&respnt->Value.AlphaVal[1],&len) ; CHARPNTBYTES1(respnt) ;
#endif
#ifdef UNIX
		alphaPNTv(respnt,getlogin()) ;
#endif

		return(respnt) ;
	      case -V4IM_Tag_Volume:
		goto list_volumes ;
	      case V4IM_Tag_Volume:
		v4im_GetPointChar(&ok,volume,sizeof volume,cpt,ctx) ; if (!ok) break ;
		break ;
	      case V4IM_Tag_Directory:
/*		Code below is duplicate of OSInfo(Directory?) to return original working directory */
//******************************************************************************************************************
//
#ifdef WINNT
		GetCurrentDirectory(250,&respnt->Value.AlphaVal[1]) ; strcat(&respnt->Value.AlphaVal[1],"\\") ;
#endif
#ifdef UNIX
		bp = getenv("PWD") ;		/* Get current value of current working directory */
		if (bp == NULL) bp = "./" ;	/* If no value then default to "./" */
		strcpy(&respnt->Value.AlphaVal[1],bp) ;		/* Copy path into temp string buffer */
#endif
#ifdef VMSOS
		str_ptr = getenv("PATH") ;
		if (str_ptr == NULL) str_ptr = "[]" ;
		strcpy(&respnt->Value.AlphaVal[1],str_ptr) ;
#endif
		alphaPNT(respnt) ; CHARPNTBYTES1(respnt)
//
//******************************************************************************************************************
	    { UCCHAR *dn,filename[V_FileName_Max] ;
		v4im_GetPointFileName(&ok,filename,UCsizeof(filename),cpt,ctx,NULL) ; if (!ok) break ;
		if ((dn = v_UCLogicalDecoder(filename,0,0,ctx->ErrorMsgAux)) == NULL)
		 { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto osinfo_fail ; } ;
		if (!UCchdir(dn))
		 { v_Msg(ctx,NULL,"FileWorkDir",intmodx,errno,dn) ; goto osinfo_fail ; } ;
	    }
		return(respnt) ;
	      case -V4IM_Tag_Directory:

#ifdef WINNT
		GetCurrentDirectory(250,&respnt->Value.AlphaVal[1]) ; strcat(&respnt->Value.AlphaVal[1],"\\") ;
#endif
#ifdef UNIX
		bp = getenv("PWD") ;		/* Get current value of current working directory */
		if (bp == NULL) bp = "./" ;	/* If no value then default to "./" */
		strcpy(&respnt->Value.AlphaVal[1],bp) ;		/* Copy path into temp string buffer */
#endif
#ifdef VMSOS
		str_ptr = getenv("PATH") ;
		if (str_ptr == NULL) str_ptr = "[]" ;
		strcpy(&respnt->Value.AlphaVal[1],str_ptr) ;
#endif
		alphaPNT(respnt) ; CHARPNTBYTES1(respnt)
		return(respnt) ;
	      case -V4IM_Tag_OS:
		DIMINFO(di,ctx,Dim_UV4) ; dictPNTv(respnt,Dim_UV4,v4dpi_DictEntryGet(ctx,0,ASCretUC(V3_CONFIG_OS),di,NULL)) ;
		return(respnt) ;
	      case -V4IM_Tag_Processors:
		intPNTv(respnt,v_AvailableProcessors()) ; return(respnt) ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; goto osinfo_fail ; } ;
	return(respnt) ;
osinfo_fail:
	REGISTER_ERROR(0) ; RETURNFAILURE ;
}

/*	v4im_Init_ltf - Inits list in respnt as TextFile				*/
/*	Call: respnt = v4im_Init_ltf( ctx , intmodx , respnt , fltrpt , filename , dimid , maxline)
	  where respnt is resulting point (list), or NULL if error (in ctx->ErrorMsg),
		ctx is context,
		intmodx is internal module index,
		respnt is point to be update with result,
		fltrpt is NULL or filter point associated with stream,
		filename is the file to be openned,
		dimid is dimension of file list points (e.g. Dim_Alpha),
		maxline is maximum width (characters) in line (0 for default)		*/

P *v4im_Init_ltf(ctx,intmodx,respnt,fltrpt,filename,dimid,maxline)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  P *respnt, *fltrpt ;
  UCCHAR *filename ;
  int dimid,maxline ;
{
  struct V4L__ListPoint *lp ;
  struct V4L__ListTextFile *ltf ;

	INITLP(respnt,lp,Dim_List) ; lp->ListType = V4L_ListType_TextFile ;
	ltf = (struct V4L__ListTextFile *)&lp->Buffer ; memset(ltf,0,sizeof *ltf) ;
	ltf->lineBufSize = (maxline == 0 ? UCReadLine_MaxLine : maxline) ;
	ltf->lineBuf = v4mm_AllocUC(ltf->lineBufSize) ;
	UCstrcpy(ltf->FileSpec,filename) ; ltf->fltrindex = UNUSED ;
	if (fltrpt != NULL)
	 { ltf->fltr = (P *)v4mm_AllocChunk(fltrpt->Bytes,FALSE) ; memcpy(ltf->fltr,fltrpt,fltrpt->Bytes) ;
	   ltf->fltrres = (P *)v4mm_AllocChunk(sizeof *ltf->fltrres,FALSE) ;
	 } ;
	
	if (!v_UCFileOpen(&ltf->UCFile,filename,UCFile_Open_Read,TRUE,ctx->ErrorMsgAux,intmodx))
	 { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; return(NULL) ; } ;
	ltf->UCFile.wantEOL = FALSE ;		/* Don't want MIDAS_EOL on these records */
	ltf->Dim = dimid ;
	lp->Bytes = (char *)&lp->Buffer - (char *)lp + sizeof *ltf ;
	ENDLP(respnt,lp) ;
	return(respnt) ;
}

/*	v4im_OSFile - Handles OSFile() module	*/
#include "v4tc_tomcrypt.h"

P *v4im_DoOSFile(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt,*argpnts[] ;
  int argcnt,intmodx,trace ;
{ P *cpt, spnt, *fltr ;
  struct V4LEX__Table *vlt ; int vltnum ;
  struct V4L__ListPoint *lp ;
  struct V4L__ListFiles *vlf ;
  struct V4LEX__TablePrsOpt tpo ;
  struct V4DPI__LittlePoint bytespt,updatept ;
  UCCHAR filename[V_FileName_Max],filename2[V_FileName_Max],extension[V_FileName_Max] ;
  int wantsList, wantsDim, wantsNest ;
  int ix,ok,maxline,icFlag,tx ; UCCHAR *bp ; double size ; char *abp ; LOGICAL showHidden ;

	wantsList = 0 ; wantsDim = Dim_Alpha ; bytespt.Dim = 0 ; updatept.Dim = 0 ; wantsNest = FALSE ; icFlag = 0 ;
	vlt = NULL ; vltnum = UNUSED ; fltr = NULL ; maxline = 0 ; showHidden = FALSE ;
	ok=TRUE ; v4im_GetPointFileName(&ok,filename,UCsizeof(filename),argpnts[1],ctx,NULL) ;
/*	If only 1 argument then assume user wants to normalize file to current OS path name syntax */
	if (argcnt == 1)
	 { if (!v_GetFileInfo(ctx,icFlag,filename,NULL,NULL,NULL,NULL,NULL,NULL,NULL,&respnt->Value.UCVal[1]))
	    { v_Msg(ctx,NULL,"OSFileSysErr",intmodx,1) ; goto fail ; } ;
	   uccharPNT(respnt) ; UCCHARPNTBYTES1(respnt) ; return(respnt) ;
	 } ;
	for(ix=2;ok&& ix<=argcnt;ix++)
	 { 
	   switch (tx=v4im_CheckPtArgNew(ctx,argpnts[ix],&cpt,&spnt))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	      case -V4IM_Tag_Delete:
		if (gpi->RestrictionMap & V_Restrict_OSFileCopyRenDel) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto fail ; } ;
		if ((bp = v_UCLogicalDecoder(filename,(VLOGDECODE_Exists|icFlag),0,ctx->ErrorMsgAux)) == NULL)
		 { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ; } ;
		ok = UCremove(bp) ;
		if (ok != 0) { v_Msg(ctx,NULL,"OSFileDel",intmodx,bp,errno) ; goto fail ; } ;
		uccharPNT(respnt) ; v_Msg(ctx,&respnt->Value.UCVal[1],"OSFileDelOK",bp) ; UCCHARPNTBYTES1(respnt) ;
		return(respnt) ;
	      case V4IM_Tag_Dim:
		wantsDim = cpt->Value.IntVal ; break ;
	      case -V4IM_Tag_Exists:
		if (gpi->RestrictionMap & V_Restrict_OSFileInfo) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto fail ; } ;
#ifdef WINNT
		if ((bp = v_UCLogicalDecoder(filename,(VLOGDECODE_Exists|icFlag),0,ctx->ErrorMsgAux)) != NULL)
		 { logPNTv(respnt,(UCGetFileAttributes(bp) != INVALID_FILE_ATTRIBUTES)) ;
		 } else { logPNTv(respnt,FALSE) ; } ;
#else
//		{ struct UC__File UCFile ;
//		  logPNT(respnt) ;
//		  if (respnt->Value.IntVal = v_UCFileOpen(&UCFile,filename,UCFile_Open_Read,TRUE,ctx->ErrorMsgAux,intmodx)) v_UCFileClose(&UCFile) ;
//		}

		if ((bp = v_UCLogicalDecoder(filename,(VLOGDECODE_Exists|icFlag),0,ctx->ErrorMsgAux)) != NULL)
		 { struct UC__File UCFile ;
		   logPNT(respnt) ;
		   if (respnt->Value.IntVal = v_UCFileOpen(&UCFile,bp,UCFile_Open_Read,TRUE,ctx->ErrorMsgAux,intmodx)) v_UCFileClose(&UCFile) ;
		 } else { logPNTv(respnt,FALSE) ; } ;

#endif
		return(respnt) ;
	      case V4IM_Tag_Directory:
		if (gpi->RestrictionMap & V_Restrict_OSFileInfo) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto fail ; } ;
		if (gpi->RestrictionMap & V_Restrict_OSFileCopyRenDel) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto fail ; } ;
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:		v_Msg(ctx,NULL,"OSInfoListOf",intmodx,ix,cpt) ; goto fail ;
		   case _Exists:	{
#if defined WINNT && defined V4UNICODE
					  struct _stat di ;
#else
					  struct stat di ;
#endif
					  if (UCstat(filename,&di) != 0)
					   { printf("errno = %d\n",errno) ; } ;
					  logPNTv(respnt,((di.st_mode & S_IFDIR) != 0)) ;
					  return(respnt) ;
					}
		   case _Create:	
#ifdef WINNT
					if (!UCCreateDirectory(filename,NULL))
					 { v_Msg(ctx,NULL,"FileError2",intmodx,filename,GetLastError()) ; goto fail ; } ;
#else
					if ((ok = mkdir(filename,-1)) != 0)
					 { v_Msg(ctx,NULL,"FileError2",intmodx,filename,errno) ; goto fail ; } ;
#endif
					logPNTv(respnt,TRUE) ;
					return(respnt) ;
		   case _CreateIf:
#ifdef WINNT
					if (!(ok = UCCreateDirectory(filename,NULL)))
					 { if (GetLastError() != ERROR_ALREADY_EXISTS)
					    { v_Msg(ctx,NULL,"FileError2",intmodx,filename,GetLastError()) ; goto fail ; } ;
					 } ;
#else
					if (!(ok = (mkdir(filename,-1)) != 0))
					 { if (errno != EEXIST)
					    { v_Msg(ctx,NULL,"FileError2",intmodx,filename,errno) ; goto fail ; } ;
					 } ;
#endif
					logPNTv(respnt,ok) ;			/* Return TRUE if created directory, FALSE otherwise */
					return(respnt) ;
		   case _Delete:
#ifdef WINNT
					if (!UCRemoveDirectory(filename))
					 { v_Msg(ctx,NULL,"OSFileDel",intmodx,filename,GetLastError()) ; goto fail ; } ;
#else
					if (rmdir(filename) != 0)
					 { v_Msg(ctx,NULL,"OSFileDel",intmodx,filename,errno) ; goto fail ; } ;
#endif
					logPNTv(respnt,TRUE) ;
					return(respnt) ;
					break ;
		 } ;
		break ;
	      case V4IM_Tag_IC:		if (v4im_GetPointLog(&ok,cpt,ctx)) { icFlag = 0 ; }
					 else { icFlag = VLOGDECODE_KeepCase ; } ;
					break ;
	      case V4IM_Tag_Hash:
	      { struct stat statbuf ;
		struct UC__File UCFile ;
		char *buf ;
		if (!v_UCFileOpen(&UCFile,filename,UCFile_Open_ReadBin,TRUE,ctx->ErrorMsgAux,intmodx)) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,cpt) ; goto fail ; } ;
		fstat(UCFile.fd,&statbuf) ; buf = v4mm_AllocChunk(statbuf.st_size,FALSE) ;
		read(UCFile.fd,buf,statbuf.st_size) ; v_UCFileClose(&UCFile) ;
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:		v_Msg(ctx,NULL,"ModTagValue",intmodx,ix,V4IM_Tag_Hash,cpt) ; goto fail ;
		   case _32:
			{ int hash32 ;
			  VHASH32_FWDb(hash32,buf,statbuf.st_size) ; intPNTv(respnt,hash32) ;
			  break ;
			}
		   case _64:
			{ UB64INT ub64 ;
			  ZPH(respnt) ; respnt->PntType = V4DPI_PntType_Int2 ; respnt->Dim = Dim_Int2 ; respnt->Bytes = V4PS_Int2 ;
			  ub64 = (UB64INT)v_Hash64b(buf,statbuf.st_size) ; memcpy(&respnt->Value.FixVal,&ub64,sizeof (UB64INT)) ;
			  break ;
			}
		   case _CRC:
			ZPH(respnt) ; respnt->PntType = V4DPI_PntType_Int2 ; respnt->Dim = Dim_Int2 ; respnt->Bytes = V4PS_Int2 ;
			intPNTv(respnt,vcrc_Calculate(buf,statbuf.st_size)) ; 
			break ;
		   case _MD5:
		      { abp = v_MD5ChecksumFile(ctx,NULL,buf,statbuf.st_size) ;
			alphaPNTv(respnt,abp) ;
			break ;
		      }
		   case _SHA1:
		      { INDEX i ; hash_state md; unsigned char hash[32] ; char res[45] ;
			sha1_init(&md);
			sha1_process(&md,buf,statbuf.st_size) ;
		        sha1_done(&md,hash);
			for(i=0;i<20;i++)
			 { res[2*i] = "0123456789abcdef"[hash[i] / 16] ; res[2*i+1] = "0123456789abcdef"[hash[i] % 16] ; } ;
			alphaPNTvl(respnt,res,40) ;
			break ;
		      }
		   case _SHA256:
		      { INDEX i ; hash_state md; unsigned char hash[32] ; char res[65] ;
			sha256_init(&md);
			sha256_process(&md, (unsigned char*)buf,statbuf.st_size) ;
		        sha256_done(&md,hash);
			for(i=0;i<32;i++)
			 { res[2*i] = "0123456789abcdef"[hash[i] / 16] ; res[2*i+1] = "0123456789abcdef"[hash[i] % 16] ; } ;
			alphaPNTvl(respnt,res,64) ;
			break ;
		      }
		} ;
		v4mm_FreeChunk(buf) ;
		return(respnt) ;
	      }
	      case V4IM_Tag_ListOf:
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:		v_Msg(ctx,NULL,"OSInfoListOf",intmodx,ix,cpt) ; goto fail ;
		   case _Contents:	if (gpi->RestrictionMap & V_Restrict_FileRead) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto fail ; } ;
					wantsList = 2 ; break ;
		   case _Files:		if (gpi->RestrictionMap & V_Restrict_OSFileListOf) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto fail ; } ;
					wantsList = 1 ; break ;
		 } ;
		break ;
	      case V4IM_Tag_Width:	maxline = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case -V4IM_Tag_ListOf:
/*		If filename contains wildcards then want list of files */
		if (UCstrpbrk(filename,WILDCARDFILECHARS) != NULL)
		 { if (gpi->RestrictionMap & V_Restrict_OSFileListOf) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto fail ; } ;
		   wantsList = 1 ; break ;
		 } ;
/*		If filename does NOT contain wildcards then want contents of the file */
		if (gpi->RestrictionMap & V_Restrict_FileRead) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto fail ; } ;
		wantsList = 2 ; break ;
	      case (V4DPI_TagFlag_Colon3|V4IM_Tag_Move):
	      case V4IM_Tag_Move:
		if (gpi->RestrictionMap & V_Restrict_OSFileCopyRenDel) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto fail ; } ;
		v4im_GetPointUC(&ok,filename2,UCsizeof(filename2),cpt,ctx) ; if (!ok) break ;
		{ UCCHAR from[V_FileName_Max], to[V_FileName_Max] ;
		  if ((bp = v_UCLogicalDecoder(filename,(VLOGDECODE_Exists|icFlag),0,ctx->ErrorMsgAux)) == NULL)
		   { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ; } ;
		  UCstrcpy(from,bp) ;
		  if ((bp = v_UCLogicalDecoder(filename2,(VLOGDECODE_NewFile|icFlag),0,ctx->ErrorMsgAux)) == NULL)
		   { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ; } ;
		  UCstrcpy(to,bp) ;
		  if (!UCmove(from,to))
		   { if ((tx & V4DPI_TagFlag_Colon3) == 0)
		      { v_Msg(ctx,NULL,"FileMoveErr",intmodx,OSERROR,from,to) ; goto fail ; } ;
/*		     Here to do 'extended' move - overwrite target file if necessary. */
		     if (!UCmoveEx(from,to,MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING))
		      { v_Msg(ctx,NULL,"FileMoveErr",intmodx,OSERROR,from,to) ; goto fail ; } ;
		   } ;
		  uccharPNT(respnt) ; v_Msg(ctx,&respnt->Value.UCVal[1],"OSFileMoveOK",from,to) ; UCCHARPNTBYTES1(respnt) ;
		  return(respnt) ;
		}
	      case V4IM_Tag_Rename:
		if (gpi->RestrictionMap & V_Restrict_OSFileCopyRenDel) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto fail ; } ;
		v4im_GetPointUC(&ok,filename2,UCsizeof(filename2),cpt,ctx) ; if (!ok) break ;
		{ UCCHAR from[V_FileName_Max], to[V_FileName_Max] ;
		  if ((bp = v_UCLogicalDecoder(filename,(VLOGDECODE_Exists|icFlag),0,ctx->ErrorMsgAux)) == NULL)
		   { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ; } ;
		  UCstrcpy(from,bp) ;
		  if ((bp = v_UCLogicalDecoder(filename2,(VLOGDECODE_NewFile|icFlag),0,ctx->ErrorMsgAux)) == NULL)
		   { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ; } ;
		  UCstrcpy(to,bp) ;
		  if (!UCrename(from,to)) { v_Msg(ctx,NULL,"FileRenameErr",intmodx,errno,from,to) ; goto fail ; } ;
		  uccharPNT(respnt) ; v_Msg(ctx,&respnt->Value.UCVal[1],"OSFileRenOK",from,to) ; UCCHARPNTBYTES1(respnt) ;
		  return(respnt) ;
		}
	      case V4IM_Tag_Copy:
		if (gpi->RestrictionMap & V_Restrict_OSFileCopyRenDel) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto fail ; } ;
		v4im_GetPointUC(&ok,filename2,UCsizeof(filename2),cpt,ctx) ; if (!ok) break ;
		{ 
		  UCCHAR from[V_FileName_Max], to[V_FileName_Max] ;
		  if ((bp = v_UCLogicalDecoder(filename,(VLOGDECODE_Exists|icFlag),0,ctx->ErrorMsgAux)) == NULL)
		   { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ; } ;
		  UCstrcpy(from,bp) ;
		  if ((bp = v_UCLogicalDecoder(filename2,(VLOGDECODE_NewFile|icFlag),0,ctx->ErrorMsgAux)) == NULL)
		   { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ; } ;
		  UCstrcpy(to,bp) ;
#ifdef WINNT
		  if (UCCopyFile(from,to,0) == 0) { v_Msg(ctx,NULL,"FileCopy",intmodx,from,to,GetLastError()) ; goto fail ; } ;
		  uccharPNT(respnt) ; v_Msg(ctx,&respnt->Value.UCVal[1],"OSFileCopyOK1",from,to) ; UCCHARPNTBYTES1(respnt) ;
		  return(respnt) ;
#else

		  { struct UC__File sUCFile, dUCFile ; int tbytes,rbytes,wbytes ; char buf[1024] ;
		    if (!v_UCFileOpen(&sUCFile,from,UCFile_Open_ReadBin,TRUE,ctx->ErrorMsgAux,intmodx)) { v_Msg(ctx,NULL,"FileCopy1",intmodx,from,to) ; goto fail ; } ;
		    if (!v_UCFileOpen(&dUCFile,to,UCFile_Open_WriteBin,TRUE,ctx->ErrorMsgAux,intmodx)) { v_UCFileClose(&sUCFile) ; v_Msg(ctx,NULL,"FileCopy1",intmodx,from,to) ; goto fail ; } ;
		    for(tbytes=0;;)
		     { rbytes = fread(buf,1,sizeof buf,sUCFile.fp) ;
		       if (rbytes <= 0) { if (feof(sUCFile.fp) != 0) break ; v_UCFileClose(&sUCFile) ; v_UCFileClose(&dUCFile) ; v_Msg(ctx,NULL,"FileCopy",intmodx,from,to,errno) ; goto fail ; } ;
		       wbytes = fwrite(buf,1,rbytes,dUCFile.fp) ;
		       if (wbytes != rbytes) { v_UCFileClose(&sUCFile) ; v_UCFileClose(&dUCFile) ; v_Msg(ctx,NULL,"FileCopy",intmodx,from,to,errno) ; goto fail ; } ;
		       tbytes += wbytes ;
		     } ;
		    v_UCFileClose(&sUCFile) ; v_UCFileClose(&dUCFile) ;
		    uccharPNT(respnt) ; v_Msg(ctx,&respnt->Value.AlphaVal[1],"OSFileCopyOK2",from,to,tbytes) ; CHARPNTBYTES1(respnt) ;
		    return(respnt) ;
		  }
#endif
		}
	      case V4IM_Tag_Filter:
		if (ISQUOTED(cpt)) cpt = UNQUOTEPTR(cpt) ;
		if (cpt->PntType != V4DPI_PntType_Isct) { v_Msg(ctx,NULL,"ModArgQIsct",intmodx,ix) ; goto fail ; } ;
		fltr = cpt ;
		break ;
	      case V4IM_Tag_Num:
	      case V4IM_Tag_Number:
		vltnum = v4im_GetPointInt(&ok,cpt,ctx) ; if (!ok) break ;
		break ;
	      case V4IM_Tag_Table:
		v4im_GetPointUC(&ok,filename2,UCsizeof(filename2),cpt,ctx) ; if (!ok) break ;
		vlt = v4eval_GetTable(ctx,filename2,NULL) ; if (vlt == NULL) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ; } ;
		vlt->tpo = &tpo ; *vlt->tpo = vlt->tpoT ;	/* Set up runtime options to be default for the table */
		break ;
	      case V4IM_Tag_Update:
		if (cpt->Bytes > sizeof updatept) { v_Msg(ctx,NULL,"OSFileTooCmplx",intmodx,ix,V4IM_Tag_Update) ; goto fail ; } ;
		if (cpt->Grouping == V4DPI_Grouping_Single && cpt->Value.IntVal == 0) break ;	/* Ignore UDT:none */
		memcpy(&updatept,cpt,cpt->Bytes) ; break ;
	      case -V4IM_Tag_Update:
		if (gpi->RestrictionMap & V_Restrict_OSFileInfo) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto fail ; } ;
		if (!v_GetFileInfo(ctx,icFlag,filename,NULL,NULL,NULL,NULL,&respnt->Value.IntVal,NULL,NULL,NULL))
		 { v_Msg(ctx,NULL,"OSFileSysErr",intmodx,ix) ; goto fail ; } ;
		ZPH(respnt) ; respnt->Dim = Dim_UDT ; respnt->PntType = V4DPI_PntType_UDT ; respnt->Bytes = V4PS_Int ;
		return(respnt) ;
	      case -V4IM_Tag_Hidden:
		showHidden = TRUE ; break ;
	      case -V4IM_Tag_FullPathName:
		if (!v_GetFileInfo(ctx,icFlag,filename,NULL,NULL,NULL,NULL,NULL,NULL,NULL,&respnt->Value.UCVal[1]))
		 { v_Msg(ctx,NULL,"OSFileSysErr",intmodx,ix) ; goto fail ; } ;
		uccharPNT(respnt) ; UCCHARPNTBYTES1(respnt)
		return(respnt) ;
	      case -V4IM_Tag_Name:
		if (!v_GetFileInfo(ctx,icFlag,filename,NULL,&respnt->Value.UCVal[1],NULL,NULL,NULL,NULL,NULL,NULL))
		 { v_Msg(ctx,NULL,"OSFileSysErr",intmodx,ix) ; goto fail ; } ;
		uccharPNT(respnt) ; UCCHARPNTBYTES1(respnt)
		return(respnt) ;
	      case -V4IM_Tag_FileName:
		if (!v_GetFileInfo(ctx,icFlag,filename,NULL,&respnt->Value.UCVal[1],extension,NULL,NULL,NULL,NULL,NULL))
		 { v_Msg(ctx,NULL,"OSFileSysErr",intmodx,ix) ; goto fail ; } ;
		if (UCnotempty(extension))
		 { UCstrcat(&respnt->Value.UCVal[1],UClit(".")) ; UCstrcat(&respnt->Value.UCVal[1],extension) ; } ;
		uccharPNT(respnt) ; UCCHARPNTBYTES1(respnt)
		return(respnt) ;
	      case -V4IM_Tag_Nest:
		wantsNest = TRUE ; break ;
	      case -V4IM_Tag_Path:
		if (!v_GetFileInfo(ctx,icFlag,filename,&respnt->Value.UCVal[1],NULL,NULL,NULL,NULL,NULL,NULL,NULL))
		 { v_Msg(ctx,NULL,"OSFileSysErr",intmodx,ix) ; goto fail ; } ;
		uccharPNT(respnt) ; UCCHARPNTBYTES1(respnt)
		return(respnt) ;
	      case -V4IM_Tag_Extension:
		if (!v_GetFileInfo(ctx,icFlag,filename,NULL,NULL,&respnt->Value.UCVal[1],NULL,NULL,NULL,NULL,NULL))
		 { v_Msg(ctx,NULL,"OSFileSysErr",intmodx,ix) ; goto fail ; } ;
		uccharPNT(respnt) ; UCCHARPNTBYTES1(respnt)
		return(respnt) ;
	      case -V4IM_Tag_Access:
		if (!v_GetFileInfo(ctx,icFlag,filename,NULL,NULL,NULL,NULL,NULL,&respnt->Value.IntVal,NULL,NULL))
		 { v_Msg(ctx,NULL,"OSFileSysErr",intmodx,ix) ; goto fail ; } ;
		ZPH(respnt) ; respnt->Dim = Dim_UDT ; respnt->PntType = V4DPI_PntType_UDT ; respnt->Bytes = V4PS_Int ;
		return(respnt) ;
	      case -V4IM_Tag_Directory:
		if (!v_GetFileInfo(ctx,icFlag,filename,NULL,NULL,NULL,&respnt->Value.IntVal,NULL,NULL,NULL,NULL))
		 { v_Msg(ctx,NULL,"OSFileSysErr",intmodx,ix) ; goto fail ; } ;
		logPNT(respnt) ; return(respnt) ;
	      case V4IM_Tag_Bytes:
		if (cpt->Bytes > sizeof bytespt) { v_Msg(ctx,NULL,"OSFileTooCmplx",intmodx,ix,V4IM_Tag_Update) ; goto fail ; } ;
		memcpy(&bytespt,cpt,cpt->Bytes) ; break ;
	      case -V4IM_Tag_Bytes:
		if (gpi->RestrictionMap & V_Restrict_OSFileInfo) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto fail ; } ;
		if (!v_GetFileInfo(ctx,icFlag,filename,NULL,NULL,NULL,NULL,NULL,NULL,&size,NULL))
		 { v_Msg(ctx,NULL,"OSFileSysErr",intmodx,ix) ; goto fail ; } ;
		dblPNT(respnt) ; PUTREAL(respnt,size) ; return(respnt) ;
	      case -V4IM_Tag_HashMD5:
		abp = v_MD5ChecksumFile(ctx,filename,NULL,0) ;
		if (abp == NULL) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ; } ;
		alphaPNTv(respnt,abp) ; return(respnt) ;
	      case -V4IM_Tag_Zip:
		logPNTv(respnt,TRUE) ; return(respnt) ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ix-1) ; goto fail ; } ;
	if (vlt != NULL)		/* Here to verify some/all of an include file */
	 { struct UC__File UCFile ;
	   int i, col,errs = 0 ;
	   if (!v_UCFileOpen(&UCFile,filename,UCFile_Open_Read,FALSE,ctx->ErrorMsgAux,intmodx)) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ; } ;
	   if (vltnum < 0) vltnum = V4LIM_BiggestPositiveInt ;
	   for(i=1;vltnum==0||i<=vltnum;i++)
	    { UCCHAR tbuf[V4TMBufMax] ; UCCHAR errfld[V4DPI_DimInfo_DimNameMax+1], lblfld[256], errmsg[sizeof ctx->ErrorMsg] ;
	      int readLen ;
	      if ((readLen = v_UCReadLine(&UCFile,UCRead_UC,tbuf,UCsizeof(tbuf),errmsg)) < 0) { v_UCFileClose(&UCFile) ; break ; } ;
	      if (i <= vlt->tpo->HeaderLines) continue ;
	      if (vlt->tpo->MinLength != UNUSED ? readLen < vlt->tpo->MinLength : FALSE) continue ;
	      if (vlt->tpo->Comment[0] == UCEOS ? FALSE : UCstrchrV(vlt->tpo->Comment,tbuf[0]) != NULL) continue ;	/* Skip if line begins with comment character */
	      if ((col = v4lex_ReadNextLine_Table(tbuf,UNUSED,errfld,lblfld,errmsg,vlt,NULL,0,NULL,filename,ix)) > 0) continue ;
	      errs ++ ;
	      if (vltnum == 0)
	       { v_Msg(ctx,NULL,"OSFileTableErr",intmodx,filename,ix,-col,lblfld,errmsg) ; goto fail ; } ;
	    } ;
	   intPNTv(respnt,errs) ; return(respnt) ;
	 } ;
	switch (wantsList)
	 { case 1:		/* Wants a directory listing */
	     ZPH(respnt) ; respnt->Dim = Dim_List ; respnt->PntType = V4DPI_PntType_List ;
	     INITLP(respnt,lp,Dim_List) ; lp->ListType = V4L_ListType_Files ;
	     vlf = (struct V4L__ListFiles *)&lp->Buffer ; memset(vlf,0,sizeof *vlf) ;
	     UCstrcpy(vlf->FileSpec,filename) ; vlf->Dim = wantsDim ;
	     if (bytespt.Dim != 0) { memcpy(&vlf->Bytespt,&bytespt,bytespt.Bytes) ; } ;
	     if (updatept.Dim != 0) { memcpy(&vlf->Updatept,&updatept,updatept.Bytes) ; } ;
	     vlf->lfi = (struct V4L__ListFilesInfo *)v4mm_AllocChunk(sizeof *vlf->lfi,TRUE) ;
	     UCstrcpy(vlf->lfi->FileNamePattern,filename) ; vlf->lfi->WantNestDir = wantsNest ; vlf->lfi->showHidden = showHidden ;
	     lp->Bytes = (char *)&lp->Buffer - (char *)lp + sizeof *vlf ;
	     ENDLP(respnt,lp) ;
	     return(respnt) ;
	   case 2:		/* Wants a file content list */
	     cpt = v4im_Init_ltf(ctx,intmodx,respnt,fltr,filename,wantsDim,maxline) ;
	     if (cpt == NULL) goto fail ;
	     return(cpt) ;
	 } ;
	v_Msg(ctx,NULL,"OSFileNoAct",intmodx) ;
fail:	REGISTER_ERROR(0) ; RETURNFAILURE ;
}


LOGICAL v_NextFile(lfi)
  struct V4L__ListFilesInfo *lfi ;
{
struct V4L__ListFilesInfo *tlfi ;
#ifdef UNIX
   struct stat stat_buf ;
   static int MinutesWest=UNUSED ;
#endif
#ifdef WINNT
#ifdef V4UNICODE
  WIN32_FIND_DATAW FileData ;
#else
  WIN32_FIND_DATA FileData ;
#endif
  SYSTEMTIME systime ;
  UCCHAR *bp2, save ;
#endif
#ifdef VMSOS
   struct vms__dsc {
     short int length ;		/* Length of value */
     short int desc ;		/* Descriptive info */
     char *pointer ;		/* Pointer to value */
    } strdsc,strdsc2 ;
   struct stat stat_buf ;
   static int MinutesWest=UNUSED ;
#endif
  int i ; UCCHAR *bp1,errmsg[256] ;

	if (lfi->Calls == 0) UCstrcpy(lfi->FileName,lfi->FileNamePattern) ;
	lfi->Calls ++ ;
	if (lfi->nestlfi != NULL) return(v_NextFile(lfi->nestlfi)) ;
#ifdef VMSOS
	if (MinutesWest == UNUSED) MinutesWest = mscu_minutes_west() ;
	if (lfi->Index == 0)		/* First call ? */
	 { UCstrcpy(lfi->DirectoryPath,lfi->FileName) ; } ;
	strdsc.length = UCsizeof(lfi->FileName)-2 ; strdsc.desc = 0 ; strdsc.pointer = UCretASC(lfi->FileName) ;
	strdsc2.length = UCsizeof(lfi->DirectoryPath) ; strdsc2.desc = 0 ; strdsc2.pointer = UCretASC(lfi->DirectoryPath) ;
	res = LIB$FIND_FILE(&strdsc2,&strdsc,&lfi->Index,0,0,0,0) ;
	if ((res & 1) == 0)			/* Failed ? */
	 { LIB$FIND_FILE_END(&lfi->Index) ; return(FALSE) ; } ;
	UCstrcpyAtoU(lfi.FileName,strdsc.pointer) ;	/* Not very good code to grab ASCII result VEH051220 */
	lfi->FileName[strdsc.length] = 0 ;
	bp1 = UCstrchr(lfi->FileName,' ') ; if (bp1 != NULL) *bp1 = UCEOS ;
	stat(UCretASC(lfi->FileName),&stat_buf) ;
	lfi->FileBytes = stat_buf.st_size ;
	lfi->UpdateDT = stat_buf.st_mtime - (60*MinutesWest) - TIMEOFFSETSEC ;
	return(TRUE) ;
#endif
#ifdef UNIX
	if (MinutesWest == UNUSED) MinutesWest = mscu_minutes_west() ;
	for(;;)
	 { if (lfi->globt == NULL)		/* First time ? */
	    { int globFlags = 0 ;
	      for(;;)
	       { UCstrcpy(lfi->FileName,lfi->FileNamePattern) ;
		 lfi->LogDeviceIndex ++ ;
		 if (lfi->LogDeviceIndex == 1) { i = 1 ; }
		  else { i = (lfi->priorlfi == NULL ? lfi->LogDeviceIndex : 99999) ; } ;
		 bp1 = v_UCLogicalDecoder(lfi->FileName,VLOGDECODE_NewFile,1,errmsg) ;
	         if (bp1 == NULL)
		  { if (lfi->priorlfi == NULL) return(FALSE) ;
	            tlfi = lfi->priorlfi ;v4mm_FreeChunk(lfi) ; tlfi->nestlfi = NULL ; return(v_NextFile(tlfi)) ;
		  } ;
	         UCstrcpy(lfi->FileName,bp1) ;
		 break ;
	       } ;
	      if (lfi->showHidden) globFlags |= GLOB_PERIOD ;
	      lfi->globt = (struct glob_t *)v4mm_AllocChunk(sizeof *lfi->globt,TRUE) ;
	      glob(UCretASC(lfi->FileName),globFlags,NULL,lfi->globt) ;	/* Get list of files */
	      lfi->Index = 0 ;
	    } ;
	   if (lfi->Index >= lfi->globt->gl_pathc)
	    { globfree(lfi->globt) ; v4mm_FreeChunk(lfi->globt) ;
	      if (lfi->priorlfi == NULL) { lfi->globt = NULL ; return(FALSE) ; } ;
	      tlfi = lfi->priorlfi ; v4mm_FreeChunk(lfi) ; tlfi->nestlfi = NULL ;
	      return(v_NextFile(tlfi)) ;
	    } ;
	   UCstrcpy(lfi->FileName,ASCretUC(lfi->globt->gl_pathv[lfi->Index++])) ;
	   stat(UCretASC(lfi->FileName),&stat_buf) ;
	   lfi->FileBytes = stat_buf.st_size ;
	   lfi->UpdateDT = stat_buf.st_mtime - (60*MinutesWest) - TIMEOFFSETSEC ;
	   lfi->FileType = ((stat_buf.st_mode & S_IFDIR) != 0 ? V4_ListFilesType_Directory : V4_ListFilesType_RegFile) ;	/* Is it a directory? */
	   if (lfi->FileType == V4_ListFilesType_Directory && lfi->WantNestDir)
	    { tlfi = v4mm_AllocChunk(sizeof *tlfi,TRUE) ; tlfi->priorlfi = lfi ; lfi->nestlfi = tlfi ; tlfi->WantNestDir = TRUE ;
	      UCstrcpy(tlfi->DirectoryPath,lfi->FileName) ;
	      UCstrcpy(tlfi->FileNamePattern,lfi->FileName) ; UCstrcat(tlfi->FileNamePattern,UClit("\\*")) ; lfi = tlfi ;
	      continue ;
	    } ;
	   return(TRUE) ;
	 } ;
#endif
#ifdef WINNT
	for(;;)
	 { if (lfi->Handle == 0)
	    { 
	      for(;;)
	       { UCstrcpy(lfi->FileName,lfi->FileNamePattern) ;
		 lfi->LogDeviceIndex ++ ;
		 if (lfi->LogDeviceIndex == 1) { i = 1 ; }
		  else { i = (lfi->priorlfi == NULL ? lfi->LogDeviceIndex : 99999) ; } ;
		 bp1 = v_UCLogicalDecoder(lfi->FileName,VLOGDECODE_NewFile,i,errmsg) ;
	         if (bp1 == NULL)
		  { if (lfi->priorlfi == NULL) return(FALSE) ;
	            tlfi = lfi->priorlfi ;v4mm_FreeChunk(lfi) ; tlfi->nestlfi = NULL ; return(v_NextFile(tlfi)) ;
		  } ;
	         if (UCGetFullPathName(bp1,UCsizeof(lfi->FileName),lfi->FileName,&bp2) == 0)
	          { printf("? GetFullPathName(%s) err (%s)\n",UCretASC(bp1),v_OSErrString(GetLastError())) ; return(FALSE) ; } ;
	         if (bp2 != NULL) { save = *bp2 ; *bp2 = UCEOS ; } ;
	         UCstrcpy(lfi->DirectoryPath,lfi->FileName) ; if (bp2 != NULL) *bp2 = save ;
	         lfi->Handle = UCFindFirstFile(lfi->FileName,&FileData) ;
	         if (lfi->Handle == INVALID_HANDLE_VALUE) continue ;
	         break ;
	       } ;
	    } else
	    { if (!UCFindNextFile(lfi->Handle,&FileData))
	       { FindClose(lfi->Handle) ;
	         if (lfi->priorlfi == NULL) { lfi->Handle = 0 ; continue ; } ;
	         tlfi = lfi->priorlfi ;v4mm_FreeChunk(lfi) ; tlfi->nestlfi = NULL ; return(v_NextFile(tlfi)) ;
	       } ;
	    } ;
	   if (lfi->WantNestDir && (UCstrcmp(FileData.cFileName,UClit(".")) == 0 || UCstrcmp(FileData.cFileName,UClit("..")) == 0)) continue ;
	   if ((FileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0 && !lfi->showHidden) continue ;	/* Don't want to return hidden files */
	   UCstrcpy(lfi->FileName,lfi->DirectoryPath) ; UCstrcat(lfi->FileName,FileData.cFileName) ;
	   lfi->FileType = (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ? V4_ListFilesType_Directory : V4_ListFilesType_RegFile ;
	   if (lfi->FileType == V4_ListFilesType_Directory && lfi->WantNestDir)
	    { tlfi = v4mm_AllocChunk(sizeof *tlfi,TRUE) ; tlfi->priorlfi = lfi ; lfi->nestlfi = tlfi ; tlfi->WantNestDir = TRUE ;
	      UCstrcpy(tlfi->DirectoryPath,lfi->FileName) ;
	      UCstrcpy(tlfi->FileNamePattern,lfi->FileName) ; UCstrcat(tlfi->FileNamePattern,UClit("\\*")) ; lfi = tlfi ;
	      continue ;
	    } ;
	   lfi->FileBytes = FileData.nFileSizeLow ;
	   FileTimeToLocalFileTime(&FileData.ftLastWriteTime,&FileData.ftLastAccessTime) ;
	   FileTimeToSystemTime(&FileData.ftLastAccessTime,&systime) ;
	   lfi->UpdateDT = vcal_UDTFromYMD(systime.wYear,systime.wMonth,systime.wDay,systime.wHour,systime.wMinute,systime.wSecond,gpi->ctx->ErrorMsgAux) ;
	   return(TRUE) ;
	 } ;
#endif
	return(FALSE) ;
}

/*	v_GetFileInfo - Returns info about a file			*/
/*	Call: ok = v_GetFileInfo(ctx , flags , file , path , name , extension , directory , update , access , size )
	  where ok is TRUE if OK, FALSE if not,
		flags are parsing flags (ex: VLOGDECODE_KeepCase)
		file is file name as ASCIZ string,
		other parameters are update if not NULL			*/

LOGICAL v_GetFileInfo(ctx,flags,file,path,name,extension,isdir,update,access,size,fullname)
  struct V4C__Context *ctx ;
  FLAGS32 flags ;
  UCCHAR *file, *path, *name, *extension, *fullname ;
  int *isdir, *update, *access ;
  double *size ;
{
#ifdef UNIX
   struct stat stat_buf ;
   static int MinutesWest=UNUSED ;
   char *b1 ;
#endif
#ifdef WINNT
  SYSTEMTIME systime ;
  HANDLE han ;
  BY_HANDLE_FILE_INFORMATION hfi ;
#endif
#ifdef VMSOS
   struct stat stat_buf ;
   UCCHAR *b1 ;
   static int MinutesWest=UNUSED ;
#endif
  UCCHAR *b ;
  UCCHAR FNBuf[V_FileName_Max] ; int i ;

	if ((b = v_UCLogicalDecoder(file,VLOGDECODE_NewFile|flags,0,ctx->ErrorMsgAux)) == NULL) return(FALSE) ;
	UCstrcpy(FNBuf,b) ; file = FNBuf ;	/* Convert to real file/path */
#ifdef WINNT
/*	V4 allows Windows files to use '/' as directory delimiter - fix here to avoid confusion later */
	for(i=0;file[i]!=UCEOS;i++) { if (file[i] == UClit('/')) file[i] = UClit('\\') ; } ;
#endif

#ifdef VMSOS
	if (MinutesWest == UNUSED) MinutesWest = mscu_minutes_west() ;
	if (extension != NULL)
	 { b = UCstrrchr(file,'.') ;
	   if (b == NULL) { *extension = UCEOS ; }
	    else { b1 = UCstrchr(b,';') ; if (b1 != NULL) *b1 = UCEOS ; UCstrcpy(extension,b+1) ; } ;
	 } ;
	if (path != NULL)
	 { b = UCstrchr(file,']') ; if (b == NULL) b = UCstrchr(file,UClit(':')) ;
	   if (b == NULL) { UCstrcpy(path,UClit("")) ; } else { *(b+1) = UCEOS ; UCstrcpy(path,file) ; } ;
	 } ;
	if (fullname != NULL)
	 { UCstrcpy(fullname,file) ;
	 } ;
	if (name != NULL)
	 { b = UCstrrchr(file,']') ; if (b == NULL) b = UCstrrchr(file,':') ;
	   if (b != NULL) { b++ ; }
	   else { b = UCstrrchr(file,UClit(':')) ; if (b == NULL) b = file ; } ;
	   b1 = UCstrchr(b,UClit('.')) ; if (b1 != NULL) *b1 = UCEOS ;  UCstrcpy(name,b) ;
	 } ;
	if (isdir != NULL || update != NULL || access != NULL || size != NULL)
	 { if (stat(UCretASC(file),&stat_buf) == -1)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"SystemCallFail","stat",file,errno) ; return(FALSE) ; } ;
	 } ;
	if (isdir != NULL) *isdir = S_ISDIR(stat_buf.st_mode) ;
	if (update != NULL) *update = stat_buf.st_mtime - (60*MinutesWest) - TIMEOFFSETSEC ;
	if (access != NULL) *access = stat_buf.st_atime - (60*MinutesWest) - TIMEOFFSETSEC ;
	if (size != NULL) *size = (double)stat_buf.st_size ;
	return(TRUE) ;
#endif
#ifdef UNIX
	if (MinutesWest == UNUSED) MinutesWest = mscu_minutes_west() ;
	if (isdir != NULL || update != NULL || access != NULL || size != NULL)
	 { if (stat(UCretASC(file),&stat_buf) == -1)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"SystemCallFail","stat",file,errno) ; return(FALSE) ; } ;
	 } ;
	if (extension != NULL)
	 { b = UCstrrchr(file,'.') ;
	   if (b == NULL) { *extension = UCEOS ; }
	    else { b1 = UCstrchr(b,'~') ; if (b1 != NULL) *b1 = UCEOS ; UCstrcpy(extension,b+1) ; } ;
	 } ;
	if (path != NULL)
	 { b = UCstrrchr(file,'/') ;
	   if (b == NULL) { UCstrcpy(path,UClit("")) ; } else { *(b+1) = UCEOS ; UCstrcpy(path,file) ; } ;
	 } ;
	if (fullname != NULL)
	 { UCstrcpy(fullname,file) ;
	   if (S_ISDIR(stat_buf.st_mode))	/* If this is a directory then make sure it ends with a slash */
	    { if (fullname[UCstrlen(fullname)-1] != UClit('/')) { UCstrcat(fullname,UClit("/")) ; } ; } ;
	 } ;
	if (name != NULL)
	 { b = UCstrrchr(file,'/') ; if (b == NULL) b = file ;
	   b1 = UCstrchr(b,'.') ; if (b1 != NULL) *b1 = UCEOS ;  UCstrcpy(name,b) ;
	 } ;
	if (isdir != NULL) *isdir = S_ISDIR(stat_buf.st_mode) ;
	if (update != NULL) *update = stat_buf.st_mtime - (60*MinutesWest) - TIMEOFFSETSEC ;
	if (access != NULL) *access = stat_buf.st_atime - (60*MinutesWest) - TIMEOFFSETSEC ;
	if (size != NULL) *size = (double)stat_buf.st_size ;
	return(TRUE) ;
#endif
#ifdef WINNT
	han = INVALID_HANDLE_VALUE ;
	if (extension != NULL)
	 { b = UCstrrchr(file,'.') ;
	   if (b == NULL) { *extension = UCEOS ; } else { UCstrcpy(extension,b+1) ; } ;
	 } ;
	if (path != NULL)
	 { if (UCGetFullPathName(file,V4DPI_UCVAL_MaxSafe,UCTBUF1,&b) == 0)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"SystemCallFail","GetFullPathName",file,GetLastError()) ; return(FALSE) ; } ;
	   if (b != NULL) *b = UCEOS ; UCstrcpy(path,UCTBUF1) ;
	 } ;
	if (fullname != NULL)
	 { int fileAttr ; LOGICAL isDir ;
	   if (UCGetFullPathName(file,V4DPI_UCVAL_MaxSafe,fullname,&b) == 0)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"SystemCallFail","GetFullPathName",file,GetLastError()) ; return(FALSE) ; } ;
	   fileAttr = UCGetFileAttributes(file) ;
	   if (fileAttr == INVALID_FILE_ATTRIBUTES) { isDir = FALSE ; }
	    else { isDir = ((fileAttr & FILE_ATTRIBUTE_DIRECTORY) != 0) ; } ;
	   if (isDir)	/* If this is a directory then make sure it ends with a slash */
	    { if (fullname[UCstrlen(fullname)-1] != UClit('\\')) { UCstrcat(fullname,UClit("\\")) ; } ; } ;
	 } ;
	if (name != NULL)
	 { if (UCGetFullPathName(file,V4DPI_UCVAL_MaxSafe,UCTBUF1,&b) == 0)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"SystemCallFail","GetFullPathName",file,GetLastError()) ; return(FALSE) ; } ;
	   if (b == NULL) { ZUS(name) ; }
	    else { UCstrcpy(name,b) ; b = UCstrrchr(name,'.') ; if (b != NULL) *b = UCEOS ; } ;
	 } ;
	if (isdir != NULL)
	 { i = UCGetFileAttributes(file) ;
	   if (i == INVALID_FILE_ATTRIBUTES) { *isdir = FALSE ; }
	    else { *isdir = ((i & FILE_ATTRIBUTE_DIRECTORY) != 0) ; } ;
	 } ;
	if (update != NULL || access != NULL || size != NULL)
	 { 
	   han = UCCreateFile(file,0,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL) ;
	   if (han == INVALID_HANDLE_VALUE)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"SystemCallFail","CreateFile",file,GetLastError()) ; return(FALSE) ; } ;
	   GetFileInformationByHandle(han,&hfi) ;
	 } ;
	if (update != NULL)
	 { FileTimeToLocalFileTime(&hfi.ftLastWriteTime,&hfi.ftLastAccessTime) ;
	   FileTimeToSystemTime(&hfi.ftLastAccessTime,&systime) ;
	   *update = vcal_UDTFromYMD(systime.wYear,systime.wMonth,systime.wDay,systime.wHour,systime.wMinute,systime.wSecond,ctx->ErrorMsgAux) ;
	 } ;
	if (access != NULL)
	 { FileTimeToLocalFileTime(&hfi.ftLastAccessTime,&hfi.ftLastWriteTime) ;
	   FileTimeToSystemTime(&hfi.ftLastWriteTime,&systime) ;
	   *access = vcal_UDTFromYMD(systime.wYear,systime.wMonth,systime.wDay,systime.wHour,systime.wMinute,systime.wSecond,ctx->ErrorMsgAux) ;
	 } ;
	if (size != NULL)
	 { *size = (double)((((__int64)hfi.nFileSizeHigh) << 32) | hfi.nFileSizeLow) ;
	 } ;
	if (han != INVALID_HANDLE_VALUE) CloseHandle(han) ;
	return(TRUE) ;
#endif
}


int v_ReadChunk(file,buffer,bytes)
 int file,bytes ;
 void *buffer ;
{ int num,total ;

	for(total=0;total<bytes;total+=num)
	 { num = read(file,(char *)buffer+total,bytes-total) ;
	   if (num == 0) break ; if (num < 0) return(-total) ;
	 } ;
	return(total) ;
}

/*	v_FileCompress - Compresses a file using LZRW1 public domain compression */

int v_FileCompress(infile,outfile,maxblock,istext,errmsg,cmpratio)
 UCCHAR *infile,*outfile ; UCCHAR *errmsg ;
 int maxblock ; LOGICAL istext ;
 double *cmpratio ;
{ struct VFC__LZRW1Hdr lhdr ;
  char *ibuf,*obuf ; UCCHAR *bp ; int in, out ; int inbytes,outbytes,totalin,totalout ; int i ;

	totalin = 0 ; totalout = 0 ; outbytes = UNUSED ;
	if (UCstrstr(infile,UClit(".v4c")) != NULL || UCstrstr(infile,UClit(".V4C")) != NULL)
	 { if (errmsg != NULL) v_Msg(NULL,errmsg,"UtilCompress",infile) ; return(FALSE) ; } ;
	if ((bp = v_UCLogicalDecoder(infile,VLOGDECODE_Exists,0,errmsg)) == NULL) return(FALSE) ;
	in = UCopen(bp,O_RDONLY|O_BINARY,0) ;
	if (in <= 0)
	 { if (errmsg != NULL) v_Msg(NULL,errmsg,"UtilFileRead",infile,errno) ; return(FALSE) ; } ;
	if ((bp = v_UCLogicalDecoder(outfile,VLOGDECODE_NewFile,0,errmsg)) == NULL) return(FALSE) ;
	out= UCopen(bp,O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,0770) ;
	if (out <= 0)
	 { close(in) ; close(out) ;
	   if (errmsg != NULL) v_Msg(NULL,errmsg,"UtilFileWrite",outfile,errno) ; return(FALSE) ; } ;

	if (maxblock == 0) maxblock = V_COMPRESS_DfltBlock ;
	ibuf = v4mm_AllocChunk(maxblock,FALSE) ; obuf = v4mm_AllocChunk(maxblock+V_COMPRESS_MaxTextLine+128,FALSE) ;

	i = 0 ;						/* Number of bytes at end skipped for text alignment */
	for(;;)
	 { if (i > 0) memcpy(ibuf,&ibuf[inbytes-i],i) ; /* Copy leftovers from last read */
	   inbytes = read(in,&ibuf[i],maxblock-i) ; if (inbytes <= 0) break ; totalin += inbytes ;
	   inbytes += i ; i = 0 ;
	   if (istext)				/* If text then scan back to last MIDAS_EOL */
	    { for(i=0;i<inbytes;i++) { if (ibuf[inbytes-1-i] == '\n') break ; } ;
	      if (i >= V_COMPRESS_MaxTextLine)
	       { inbytes = -1 ; break ; } ;
	    } ;
	   lzrw1_compress(ibuf,inbytes-i,obuf,&outbytes) ;
	   lhdr.SrcBytes = inbytes-i ; lhdr.CmpBytes = outbytes ;
	   write(out,&lhdr,sizeof lhdr) ; totalout += (sizeof lhdr + outbytes) ;
	   if (outbytes != write(out,obuf,outbytes)) { outbytes = -1 ; break ; } ;
	   if (inbytes < maxblock) break ;
	 } ;
/*	If anything left over then write out last piece (i.e. last line did not end in NL) */
	if (i > 0)
	 { memcpy(ibuf,&ibuf[inbytes-i],i) ; lzrw1_compress(ibuf,i,obuf,&outbytes) ;
	   lhdr.SrcBytes = i ; lhdr.CmpBytes = outbytes ; write(out,&lhdr,sizeof lhdr) ;
	   if (outbytes != write(out,obuf,outbytes)) outbytes = -1 ; totalout += (sizeof lhdr + outbytes) ;
	 } ;
	close(in) ; close(out) ; v4mm_FreeChunk(ibuf) ; v4mm_FreeChunk(obuf) ;
	if (inbytes < 0) { v_Msg(NULL,errmsg,"FileReadErr",errno,infile) ; return(FALSE) ; } ;
	if (outbytes < 0) { v_Msg(NULL,errmsg,"FileWriteErr",errno,outfile) ; return(FALSE) ; } ;

	if (cmpratio != NULL) { *cmpratio = (double)(totalout) * 100.0 / (double)(totalin) ; } ;
	return(TRUE) ;
}

/*	v_FileExpand - Expands previously compressed file	*/

int v_FileExpand(infile,outfile,errmsg)
 UCCHAR *infile,*outfile ; UCCHAR *errmsg ;
{ struct VFC__LZRW1Hdr lhdr ;
  char *ibuf=NULL,*obuf=NULL ; UCCHAR *bp ; int i,in, out ; int inbytes,outbytes=-1 ; int maxblock = UNUSED ;

	if ((bp = v_UCLogicalDecoder(infile,VLOGDECODE_Exists,0,errmsg)) == NULL) return(FALSE) ;
	in = UCopen(bp,O_RDONLY|O_BINARY,0) ;
	if (in <= 0)
	 { if (errmsg != NULL) v_Msg(NULL,errmsg,"FileReadErr",errno,infile) ; return(FALSE) ; } ;
	if ((bp = v_UCLogicalDecoder(outfile,VLOGDECODE_NewFile,0,errmsg)) == NULL) return(FALSE) ;
	out= UCopen(bp,O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,0770) ;
	if (out <= 0)
	 { close(in) ; close(out) ;
	   if (errmsg != NULL) v_Msg(NULL,errmsg,"FileWriteErr",errno,outfile) ; return(FALSE) ; } ;

	for(i=0;;i++)
	 { inbytes = read(in,&lhdr,sizeof lhdr) ; if (inbytes <= 0) break ;
	   if (lhdr.SrcBytes < 0 || lhdr.SrcBytes > 0xfffff || lhdr.CmpBytes < 0 || lhdr.CmpBytes > 0xfffff)
	    { if (i == 0) printf("File (%s) appears to have been created on a reverse-endian machine!\n",UCretASC(infile)) ;
	      FLIPLONG(lhdr.SrcBytes,lhdr.SrcBytes) ; FLIPLONG(lhdr.CmpBytes,lhdr.CmpBytes) ;
	    } ;
	   if (maxblock == UNUSED)
	    { maxblock = lhdr.SrcBytes + V_COMPRESS_MaxTextLine ; /* Add a little extra for text compressions */
	      if (maxblock < 100 || maxblock > 250000) { v_Msg(NULL,errmsg,"FileInvCmpForm",infile,0,maxblock,0) ; return(FALSE) ; } ;
	      ibuf = v4mm_AllocChunk(maxblock,FALSE) ; obuf = v4mm_AllocChunk(maxblock+256,FALSE) ;
	    } ;
	   inbytes = read(in,ibuf,lhdr.CmpBytes) ; if (inbytes < lhdr.CmpBytes) { inbytes = -inbytes ; break ; } ;
	   lzrw1_decompress(ibuf,inbytes,obuf,&outbytes,maxblock+256) ;
	   if (outbytes != write(out,obuf,outbytes)) { outbytes = -1 ; break ; } ;
	 } ; if (ibuf != NULL) v4mm_FreeChunk(ibuf) ; if (obuf != NULL) v4mm_FreeChunk(obuf) ;
	close(in) ; close(out) ;
	if (inbytes < 0) { v_Msg(NULL,errmsg,"FileReadErr",errno,infile) ; return(FALSE) ; } ;
	if (outbytes < 0) { v_Msg(NULL,errmsg,"FileWriteErr",errno,outfile) ; return(FALSE) ; } ;

	return(TRUE) ;
}

/*	G R A P H   S T U F F								*/

/*	v4im_DoGraphConnect - Finds all connections from nodea to nodeb in a graph	*/

struct V4DPI__Point *v4im_DoGraphConnect(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt ;
  INTMODX intmodx ;
  P *argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ P spnt, wpnt, npnt, oldpar, oldpif ;
  P *ipt, *cpt, *wpntp, *npntp, *onewaypt, *parpt, *pifpt, *ifpt, *wpt, *awpt, *untilpt, *ctxpt, *ctxPpt, *xpt, *ypt ;
#define GC_EdgePtsMax 20
  P *edgepts[GC_EdgePtsMax] ; int edgecnt ;
  struct V4DPI__LittlePoint lpnt ;
  struct V4GRAPH__Connect *vgcon ;
  struct V4GRAPH__Weights *vgw, **vgwp ;
  struct V4L__ListPoint *lp,*wlp, *nlp, *lpctx ;
  int i,ix,ex,ok,tx,gotnode,gottarget,gotenum,oneway,frameid,sx,iter,distdim,uomfctr ; int gap,j,nA,nB ; double wgt ;

	vgcon = (struct V4GRAPH__Connect *)v4mm_AllocChunk(sizeof *vgcon,FALSE) ;
	vgcon->EdgeCnt = 0 ; vgcon->EdgeMax = V4GRAPH_EdgeMax ; vgcon->NodeCnt = 0 ; vgcon->DepthMax = V4GRAPH_DepthMax ;
	gotnode = FALSE ; gottarget = FALSE ; edgecnt = 0 ; iter = UNUSED ; distdim = UNUSED ; uomfctr = V4DPI_GeoCoordDist_KMeter ;
	vgcon->vgw = NULL ; vgcon->ResultMax = V4LIM_BiggestPositiveInt ; lp = NULL ; gotenum = FALSE ; frameid = UNUSED ;
	parpt = NULL ; pifpt = NULL ; awpt = NULL ; wpt = NULL ; untilpt = NULL ; ctxpt = NULL ; ctxPpt = NULL ;
	vgcon->WeightCnt = 0 ; onewaypt = NULL ; oneway = FALSE ; ifpt = NULL ; xpt = NULL ; ypt = NULL ;

	for(ok=TRUE,ix=1;ok&&ix<=argcnt;ix++)
	 { 
	   switch (tx=v4im_CheckPtArgNew(ctx,argpnts[ix],&cpt,NULL))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	      case V4IM_Tag_Begin:	vgcon->StartNode = v4im_GetPointInt(&ok,cpt,ctx) ; gotnode = TRUE ; break ;
	      case V4IM_Tag_Context:	ONCE(ctxpt) ; ctxpt = cpt ; break ;
	      case V4IM_Tag_ContextP:	ONCE(ctxPpt) ; ctxPpt = cpt ; break ;
	      case V4IM_Tag_Depth:	vgcon->DepthMax = v4im_GetPointInt(&ok,cpt,ctx) - 1 ; break ;
	      case V4IM_Tag_Distance:	distdim = v4dpi_DimIndexDimId(&ok,ctx,cpt,NULL) ; break ;
	      case V4IM_Tag_Edge:
		if (edgecnt >= GC_EdgePtsMax) { v_Msg(ctx,ctx->ErrorMsgAux,"GraphEdgeMax",GC_EdgePtsMax) ; ok = FALSE ; break ; } ;
		edgepts[edgecnt++] = cpt ; break ;
	      case V4IM_Tag_End:	vgcon->Target = v4im_GetPointInt(&ok,cpt,ctx) ; gottarget = TRUE ; break ;
	      case V4IM_Tag_Enum:	gotenum = TRUE ; lp = v4im_VerifyList(NULL,ctx,cpt,intmodx) ; break ;
	      case V4IM_Tag_If:		ONCE(ifpt) ; ifpt = cpt ; break ;
	      case V4IM_Tag_List:	ONCE(lp) lp = v4im_VerifyList(NULL,ctx,argpnts[ix],intmodx) ; break ;
	      case V4IM_Tag_UOM:
		{ int cx = gpi->ci->li->CurX ; UCCHAR tag[64],sbbuf[512] ;
		  v4im_GetPointUC(&ok,tag,UCsizeof(tag),cpt,ctx) ; if (!ok) break ;
		  for(i=0;i<V4LI_DstMax;i++) { if (UCstrcmpIC(tag,gpi->ci->li->LI[cx].Dst[i].dstAbbr) == 0) break ; } ;
		  if (i < V4LI_DstMax) { uomfctr = gpi->ci->li->LI[cx].Dst[ix].dstIntVal ; break ; } ;
		  ZUS(sbbuf) ; for(i=0;i<V4LI_DstMax;i++) { if (UCnotempty(gpi->ci->li->LI[cx].Dst[i].dstAbbr)) { UCstrcat(sbbuf,gpi->ci->li->LI[cx].Dst[i].dstAbbr) ; UCstrcat(sbbuf,UClit(",")) ; } ; } ;
		  sbbuf[UCstrlen(sbbuf) - 1] = UCEOS ; v_Msg(ctx,ctx->ErrorMsgAux,"DPIGeoUOMBad",tag,sbbuf) ; ok = FALSE ; break ;
		}
	      case V4IM_Tag_Maximum:
	      case V4IM_Tag_Minimum:
	      case -V4IM_Tag_Maximum:
	      case -V4IM_Tag_Minimum:
		vgw = (struct V4GRAPH__Weights *)v4mm_AllocChunk(sizeof *vgw,FALSE) ; vgw->nvgw = NULL ;
		if (tx < 0) { tx = -tx ; vgw->ConstantWeight = TRUE ; vgw->wpt = NULL ; }
		 else { vgw->wpt = cpt ; vgw->ConstantWeight = FALSE ; } ;
		vgw->ResultType = (tx == V4IM_Tag_Maximum ? V4GRAPH_ConnectRes_Maximum : V4GRAPH_ConnectRes_Minimum) ;
		vgw->CurMinMaxVal = (vgw->ResultType == V4GRAPH_ConnectRes_Maximum ? -DBL_MAX : DBL_MAX) ;
		if (vgcon->vgw == NULL)		/* First one ? */
		 { vgcon->vgw = vgw ;
		 } else				/* Postion to end of chain */
		 { struct V4GRAPH__Weights *tvgw ;
		   for(tvgw=vgcon->vgw;;tvgw=tvgw->nvgw)
		    { if (tvgw->nvgw != NULL) continue ; tvgw->nvgw = vgw ; break ; } ;
		 } ;
		vgcon->WeightCnt++ ; break ;
	      case V4IM_Tag_OneWay:	ONCE(onewaypt) onewaypt = cpt ; break ;
	      case V4IM_Tag_Parent:	ONCE(parpt) ; parpt = cpt ; break ;
	      case V4IM_Tag_PIf:	ONCE(pifpt) ; pifpt = cpt ; break ;
	      case V4IM_Tag_Result:	vgcon->ResultMax = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case -V4IM_Tag_Route:	iter = 0 ; break ;
	      case V4IM_Tag_Route:	iter = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Until:	ONCE(untilpt) ; untilpt = cpt ; break ;
	      case V4IM_Tag_While:	ONCE(wpt) ; wpt = cpt ; break ;
	      case V4IM_Tag_AWhile:	ONCE(awpt) ; awpt = cpt ; break ;
	      case V4IM_Tag_X:		ONCE(xpt) ; xpt = cpt ; break ;
	      case V4IM_Tag_Y:		ONCE(ypt) ; ypt = cpt ; break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ix-1) ; goto fail ; } ;

/*	Are we connecting points for Traveling Salesman ? */
	if (iter != UNUSED)
	 { ipt = vts_Driver(ctx,respnt,iter,distdim,uomfctr,lp,xpt,ypt,intmodx,trace) ;
	   if (ipt == NULL) goto fail ;
	   return(ipt) ;
	 } ;

	if (edgecnt < 2) { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"Graph2Edges",V4IM_Tag_Edge) ; } ;
	if (!(gotnode && gottarget))
	 { v_Msg(ctx,NULL,"GraphNoNode",intmodx,V4IM_Tag_Begin,V4IM_Tag_End) ; goto fail ; } ;
	if (lp == NULL) { v_Msg(ctx,NULL,"GraphDef",intmodx,V4IM_Tag_Enum) ; goto fail ; } ;
	if (pifpt != NULL && parpt == NULL)
	 { v_Msg(ctx,NULL,"TallyTagWithTag",intmodx,V4IM_Tag_PIf,V4IM_Tag_Parent) ; goto fail ; } ;
	if (pifpt == NULL && parpt != NULL)
	 { v_Msg(ctx,NULL,"TallyTagWthTag2",intmodx,V4IM_Tag_Parent,V4IM_Tag_PIf,V4IM_Tag_Context) ; goto fail ; } ;

	vgcon->Entries = 0 ; vgcon->Success = 0 ; ZUS(ctx->ErrorMsgAux) ;

	if (gotenum)					/* Got Enum or explicit list defining graph ? */
	 { frameid = v4ctx_FramePush(ctx,NULL) ;
	   for(i=1;;i++)					/* Load up graph */
	    { if (v4l_ListPoint_Value(ctx,lp,i,&spnt) <= 0) break ;
	      if (!v4ctx_FrameAddDim(ctx,0,&spnt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto fail ; } ;
	      CLEARCACHE
	      if (ctxpt != NULL)
	       { if (ctxpt->PntType == V4DPI_PntType_List)
		  { lpctx = v4im_VerifyList(NULL,ctx,ctxpt,intmodx) ;
		    for(sx=1;;sx++)
		     { if (v4l_ListPoint_Value(ctx,lpctx,sx,&spnt) <= 0) break ;
		       ipt = v4dpi_IsctEval(respnt,&spnt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		       if (ipt == NULL) break ;
		       if (!v4ctx_FrameAddDim(ctx,0,ipt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto fail ; } ;
		       CLEARCACHE
		     } ;
		  } else
		  { ipt = v4dpi_IsctEval(&spnt,ctxpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		    if (ipt != NULL) { if (!v4ctx_FrameAddDim(ctx,0,ipt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto fail ; } ; CLEARCACHE } ;
		    if (memcmp(ipt,&protoSkip,protoSkip.Bytes) == 0) continue ;	/* If result is UV4:Skip then skip this point */
		  } ;
		 if (ipt == NULL)
		  { v_Msg(ctx,NULL,"TagEvalFail",intmodx,V4IM_Tag_Context,ctxpt) ; goto fail ; } ;
	       } ;
	      if (ctxPpt != NULL)
	       { 
		 ipt = v4dpi_IsctEval(&spnt,ctxPpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		 if (ipt == NULL) { v_Msg(ctx,NULL,"TagEvalFail",intmodx,V4IM_Tag_Context,ctxpt) ; goto fail ; } ;
		 if (memcmp(ipt,&protoSkip,protoSkip.Bytes) == 0) continue ;	/* If result is UV4:Skip then skip this point */
	         if (ctxpt->PntType == V4DPI_PntType_List)
		  { lpctx = v4im_VerifyList(NULL,ctx,ctxpt,intmodx) ;
		    for(sx=1;;sx++)
		     { if (v4l_ListPoint_Value(ctx,lpctx,sx,&spnt) <= 0) break ;
		       ipt = v4dpi_IsctEval(respnt,&spnt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		       if (ipt == NULL) break ;
		       if (!v4ctx_FrameAddDim(ctx,0,ipt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto fail ; } ;
		       CLEARCACHE
		     } ;
		  } else
		  { if (ipt != NULL) { if (!v4ctx_FrameAddDim(ctx,0,ipt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto fail ; } ; CLEARCACHE } ;
		  } ;
	       } ;
	      if (pifpt != NULL)					/* Have PIf:xxx point? */
	       { ipt = v4dpi_IsctEval(&spnt,parpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		 if (ipt == NULL) { v_Msg(ctx,NULL,"TagEvalFail",intmodx,V4IM_Tag_Parent,parpt) ; goto fail ; } ;
		 if (memcmp(&oldpar,ipt,ipt->Bytes) != 0)		/* Parent different? */
		  { memcpy(&oldpar,ipt,ipt->Bytes) ;		/* Copy for next go-around */
		    if (!v4ctx_FrameAddDim(ctx,0,ipt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto fail ; } ;		/* Add parent point to current context */
		    CLEARCACHE
		    ipt = v4dpi_IsctEval(&oldpif,pifpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		    if (ipt == NULL) { v_Msg(ctx,NULL,"TagEvalFail",intmodx,V4IM_Tag_PIf,pifpt) ; goto fail ; } ;
		  } else { ipt = &oldpif ; } ;
		 if (ipt->Value.IntVal <= 0) continue ; 	/* If FALSE then try next point */
	       } ;
	      if (ifpt != NULL) 				/* Have If::xxx point? */
	       { ipt = v4dpi_IsctEval(&spnt,ifpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		 if (ipt == NULL) { v_Msg(ctx,NULL,"TagEvalFail",intmodx,V4IM_Tag_If,ifpt) ; goto fail ; } ;
		 if (ipt->Value.IntVal <= 0) continue ; 	/* FALSE = continue with next point */
	       } ;
	      if (wpt != NULL)					/* Have While:xxx point? */
	       { ipt = v4dpi_IsctEval(&spnt,wpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	         if (ipt == NULL) { v_Msg(ctx,NULL,"TagEvalFail",intmodx,V4IM_Tag_While,wpt) ; goto fail ; } ;
	         if (ipt->Value.IntVal <= 0) break ;		/* FALSE = quit now! */
	       } ;
	      if (vgcon->EdgeCnt >= (vgcon->EdgeMax - (edgecnt*2)))	/* At limit - then increase by 50% */
	       { vgcon->EdgeMax = (int)(vgcon->EdgeMax * 1.5) ;
	         vgcon = realloc(vgcon,sizeof *vgcon + ((vgcon->EdgeMax - V4GRAPH_EdgeMax) * sizeof vgcon->Edge[0])) ;
		 for(vgwp=&vgcon->vgw;*vgwp!=NULL;vgwp=&(*vgwp)->nvgw)
		  { if ((*vgw).ConstantWeight) continue ;
		    *vgwp = realloc(*vgwp,sizeof *vgw + ((vgcon->EdgeMax - V4GRAPH_EdgeMax) * sizeof vgw->Weight[0])) ;
		  } ;

	       } ;

/********************************************************************/
	      for(ex=0;ex<edgecnt-1;ex++)
	       { 
	         ipt = v4dpi_IsctEval(&spnt,edgepts[ex],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	         if (ipt == NULL) { v_Msg(ctx,NULL,"GraphEdgeFail",intmodx,1,ex) ; goto fail ; } ;
	         vgcon->Edge[vgcon->EdgeCnt].NodeA = v4im_GetPointInt(&ok,ipt,ctx) ;
	         if (!ok) { v_Msg(ctx,NULL,"GraphEdgeFail2",intmodx,1,ex) ; goto fail ; } ;
	         ipt = v4dpi_IsctEval(&spnt,edgepts[ex+1],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	         if (ipt == NULL) { v_Msg(ctx,NULL,"GraphEdgeFail",intmodx,2,ex) ; goto fail ; } ;
	         vgcon->Edge[vgcon->EdgeCnt++].NodeB = v4im_GetPointInt(&ok,ipt,ctx) ;
	         if (!ok) { v_Msg(ctx,NULL,"GraphEdgeFail2",intmodx,2,ex) ; goto fail ; } ;
	         if (onewaypt != NULL)
	          { ipt = v4dpi_IsctEval(&spnt,onewaypt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	            if (ipt == NULL) { v_Msg(ctx,NULL,"GraphTagFail",intmodx,V4IM_Tag_OneWay,ex) ; goto fail ; } ;
	            oneway = v4im_GetPointLog(&ok,ipt,ctx) ;
	            if (!ok) { v_Msg(ctx,NULL,"GraphTagFail2",intmodx,V4IM_Tag_OneWay,ex) ; goto fail ; } ;
	          } ;
	         if (!oneway)				/* Double them up */
	          { vgcon->Edge[vgcon->EdgeCnt].NodeB = vgcon->Edge[vgcon->EdgeCnt-1].NodeA ;
	            vgcon->Edge[vgcon->EdgeCnt].NodeA = vgcon->Edge[vgcon->EdgeCnt-1].NodeB ; vgcon->EdgeCnt++ ;
	          } ;

	         for(vgw=vgcon->vgw;vgw!=NULL;vgw=vgw->nvgw)	/* Grab all weights */
	          { if (vgw->ConstantWeight) continue ;
	            ipt = v4dpi_IsctEval(&spnt,vgw->wpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	            if (ipt == NULL) { v_Msg(ctx,NULL,"GraphWeightFail",intmodx,ex) ; goto fail ; } ;
	            wgt = v4im_GetPointDbl(&ok,ipt,ctx) ;
	            if (!ok) { v_Msg(ctx,NULL,"GraphWgtFail2",intmodx,ex,ipt) ; goto fail ; } ;
		    if (oneway) { vgw->Weight[vgcon->EdgeCnt-1] = wgt ; }
		     else { vgw->Weight[vgcon->EdgeCnt-2] = wgt ; vgw->Weight[vgcon->EdgeCnt-1] = wgt ; } ;
	          } ;
	       } ;
/********************************************************************/


#ifdef FOOOO
	      ipt = v4dpi_IsctEval(&spnt,edgept1,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	      if (ipt == NULL) { sprintf(ctx->ErrorMsg,"%s First Edge of element #%d failed",v4im_ModFailStr(intmodx),i) ; goto fail ; } ;
	      vgcon->Edge[vgcon->EdgeCnt].NodeA = v4im_GetPointInt(&ok,ipt,ctx) ;
	      if (!ok) { sprintf(ctx->ErrorMsg,"%s First Edge of element #%d - %s",v4im_ModFailStr(intmodx),i,ctx->ErrorMsgAux) ; goto fail ; } ;
	      ipt = v4dpi_IsctEval(&spnt,edgept2,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	      if (ipt == NULL) { sprintf(ctx->ErrorMsg,"%s Second Edge of element #%d failed",v4im_ModFailStr(intmodx),i) ; goto fail ; } ;
	      vgcon->Edge[vgcon->EdgeCnt++].NodeB = v4im_GetPointInt(&ok,ipt,ctx) ;
	      if (!ok) { sprintf(ctx->ErrorMsg,"%s Second Edge of element #%d - %s",v4im_ModFailStr(intmodx),i,ctx->ErrorMsgAux) ; goto fail ; } ;
	      if (onewaypt != NULL)
	       { ipt = v4dpi_IsctEval(&spnt,onewaypt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	         if (ipt == NULL) { sprintf(ctx->ErrorMsg,"%s OneWay::xxx of element #%d failed",v4im_ModFailStr(intmodx),i) ; goto fail ; } ;
	         oneway = v4im_GetPointLog(&ok,ipt,ctx) ;
	         if (!ok) { sprintf(ctx->ErrorMsg,"%s OneWay:xxx of element #%d - %s",v4im_ModFailStr(intmodx),i,ctx->ErrorMsgAux) ; goto fail ; } ;
	       } ;
	      if (!oneway)				/* Double them up */
	       { vgcon->Edge[vgcon->EdgeCnt].NodeB = vgcon->Edge[vgcon->EdgeCnt-1].NodeA ;
	         vgcon->Edge[vgcon->EdgeCnt].NodeA = vgcon->Edge[vgcon->EdgeCnt-1].NodeB ; vgcon->EdgeCnt++ ;
	       } ;

	      for(vgw=vgcon->vgw;vgw!=NULL;vgw=vgw->nvgw)	/* Grab all weights */
	       { if (vgw->ConstantWeight) continue ;
	         ipt = v4dpi_IsctEval(&spnt,vgw->wpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	         if (ipt == NULL) { sprintf(ctx->ErrorMsg,"%s Weight of element #%d failed",v4im_ModFailStr(intmodx),i) ; goto fail ; } ;
	         wgt = v4im_GetPointDbl(&ok,ipt,ctx) ;
	         if (!ok) { sprintf(ctx->ErrorMsg,"%s Weight of element #%d - %s",v4im_ModFailStr(intmodx),i,ctx->ErrorMsgAux) ; goto fail ; } ;
		 if (oneway) { vgw->Weight[vgcon->EdgeCnt-1] = wgt ; }
		  else { vgw->Weight[vgcon->EdgeCnt-2] = wgt ; vgw->Weight[vgcon->EdgeCnt-1] = wgt ; } ;
	       } ;
#endif


	      if (awpt != NULL)					/* Have AWhile:xxx point? */
	       { ipt = v4dpi_IsctEval(&spnt,awpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	         if (ipt == NULL)
	          { v_Msg(ctx,NULL,"TagEvalFail",intmodx,V4IM_Tag_AWhile,awpt) ; goto fail ; } ;
	         if (ipt->Value.IntVal <= 0) break ;		/* FALSE = continue with next point */
	       } ;
	    } ;
	 } else						/* Have explicit list defining graph */
	 { 
	   for(i=1;;i++)				/* Load up graph */
	    { if (v4l_ListPoint_Value(ctx,lp,i,&spnt) <= 0) break ;
	      if (vgcon->EdgeCnt >= vgcon->EdgeMax - 2)	/* At limit - then increase by 50% */
	       { vgcon->EdgeMax = (int)(vgcon->EdgeMax * 1.5) ;
	         vgcon = realloc(vgcon,sizeof *vgcon + ((vgcon->EdgeMax - V4GRAPH_EdgeMax) * sizeof vgcon->Edge[0])) ;
	       } ;
	      vgcon->Edge[vgcon->EdgeCnt].NodeA = spnt.Value.Int2Val[0] ;
	      vgcon->Edge[vgcon->EdgeCnt++].NodeB = spnt.Value.Int2Val[1] ;
	      vgcon->Edge[vgcon->EdgeCnt].NodeB = spnt.Value.Int2Val[0] ;
	      vgcon->Edge[vgcon->EdgeCnt++].NodeA = spnt.Value.Int2Val[1] ;
	    } ;

	 } ;

	for(gap=vgcon->EdgeCnt/2;gap>0;gap=gap/2)
	 { for(i=gap;i<=vgcon->EdgeCnt-1;i++)
	    { for(j=i-gap;j>=0;j-=gap)
	       { if (vgcon->Edge[j].NodeA < vgcon->Edge[j+gap].NodeA) break ;
	         if (vgcon->Edge[j].NodeA == vgcon->Edge[j+gap].NodeA)
		  { if (vgcon->Edge[j].NodeB <= vgcon->Edge[j+gap].NodeB) break ; } ;
		 nA = vgcon->Edge[j].NodeA ; nB = vgcon->Edge[j].NodeB ;
		 vgcon->Edge[j].NodeA = vgcon->Edge[j+gap].NodeA ; vgcon->Edge[j+gap].NodeA = nA ;
		 vgcon->Edge[j].NodeB = vgcon->Edge[j+gap].NodeB ; vgcon->Edge[j+gap].NodeB = nB ;
		 for(vgw=vgcon->vgw;vgw!=NULL;vgw=vgw->nvgw)	/* Grab all weights & flip */
		  { if (vgw->ConstantWeight) continue ;
		    wgt = vgw->Weight[j] ; vgw->Weight[j] = vgw->Weight[j+gap] ; vgw->Weight[j+gap] = wgt ;
		  } ;
	       } ;
	    } ;
	 } ;
	for(i=1,j=0;i<vgcon->EdgeCnt;i++)
	 { if (vgcon->Edge[j].NodeA == vgcon->Edge[i].NodeA && vgcon->Edge[j].NodeB == vgcon->Edge[i].NodeB) continue ;
	   if (vgcon->Edge[j].NodeA > vgcon->Edge[i].NodeA ||
		(vgcon->Edge[j].NodeA == vgcon->Edge[i].NodeA ? vgcon->Edge[j].NodeB > vgcon->Edge[i].NodeB : FALSE))
	    { printf("V4-Error-Problem @i=%d\n",i) ; } ;
	   vgcon->Edge[++j].NodeA = vgcon->Edge[i].NodeA ; vgcon->Edge[j].NodeB = vgcon->Edge[i].NodeB ;
	 } ;
	if ((trace & V4TRACE_Progress) != 0)
	 { v_Msg(ctx,UCTBUF1,"*GraphConTrace",intmodx,vgcon->EdgeCnt,j+1,vgcon->EdgeCnt-(j+1)) ; vout_UCText(VOUT_Progress,0,UCTBUF1) ; } ;
	vgcon->EdgeCnt = j+1 ;

	INITLP(respnt,vgcon->lpr,Dim_List)
	if (!v4graph_NextEdge2(ctx,vgcon,vgcon->StartNode)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto fail ; } ;

	if (vgcon->Success <= 0) { v_Msg(ctx,NULL,"GraphNoCon",intmodx) ; goto fail ; } ;
/*	Figure out what to return based on any Min/Max tags */
	for(vgw=vgcon->vgw;vgw!=NULL;vgw=vgw->nvgw)
	 { wpntp = &wpnt ; INITLP(wpntp,wlp,Dim_List)
	   dblPNTv(&lpnt,vgw->CurMinMaxVal) ; if (!v4l_ListPoint_Modify(ctx,wlp,V4L_ListAction_Append,(P *)&lpnt,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto fail ; } ;
	   npntp = &npnt ; INITLP(npntp,nlp,Dim_List) intPNT(npntp) ;
	   for(i=0;i<vgw->MinMaxCnt;i++)
	    { lpnt.Value.IntVal = vgw->MinMaxNodes[i] ;
	      if (!v4l_ListPoint_Modify(ctx,nlp,V4L_ListAction_Append,(P *)&lpnt,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto fail ; } ;
	    } ;
	   ENDLP(npntp,nlp) if (!v4l_ListPoint_Modify(ctx,wlp,V4L_ListAction_Append,&npnt,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto fail ; } ;
	   ENDLP(wpntp,wlp) if (!v4l_ListPoint_Modify(ctx,vgcon->lpr,V4L_ListAction_Append,&wpnt,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto fail ; } ;
	 } ;
	ENDLP(respnt,vgcon->lpr)

end:
	if (frameid != UNUSED)
	 { if (!v4ctx_FramePop(ctx,frameid,NULL))
	    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdCtxPopId",frameid) ; goto fail ; } ;
	 } ;
	for(vgw=vgcon->vgw;vgw!=NULL;vgw=vgcon->vgw)		/* Return all storage */
	 { vgcon->vgw = vgw->nvgw ; v4mm_FreeChunk(vgw) ;
	 } ; v4mm_FreeChunk(vgcon) ;
	
	return(respnt) ;

fail:
	if (frameid != UNUSED) { v4ctx_FramePop(ctx,frameid,NULL) ; frameid = UNUSED ; } ;
	REGISTER_ERROR(0) ; respnt = NULL ; goto end ;
}


/*	Recursive module assuming doubling & sorting of edges */

int v4graph_NextEdge2(ctx,vgcon,node)
  struct V4C__Context *ctx ;
  struct V4GRAPH__Connect *vgcon ;
  int node ;
{ 
  struct V4GRAPH__Weights *vgw ;
  int i,j,first,last ;

	if (vgcon->Success >= vgcon->ResultMax) return(TRUE) ;
	for(first=0,last=vgcon->EdgeCnt-1;;)
	 { i = (last + first) / 2 ;
	   if (node == vgcon->Edge[i].NodeA) break ;
	   if (node < vgcon->Edge[i].NodeA) { last = i - 1 ; } else { first = i + 1 ; } ;
	   if (first > last) return(999) ;
	 } ;
/*	Maybe slide down to first of several repeats */
	for(;i>0;i--) { if (vgcon->Edge[i-1].NodeA != node) break ; } ;
	for(;i<vgcon->EdgeCnt && node == vgcon->Edge[i].NodeA;i++)
	 { for(j=0;j<vgcon->NodeCnt;j++) { if (node == vgcon->Nodes[j]) return(FALSE) ; } ;
	   for(vgw=vgcon->vgw;vgw!=NULL;vgw=vgw->nvgw)	/* Grab all weights & flip */
	    { if (vgw->ConstantWeight) continue ;
	      vgw->Nodes[vgcon->NodeCnt] = vgw->Weight[i] ;
	    } ;
	   if (vgcon->Edge[i].NodeB == vgcon->Target)
	    { vgcon->Nodes[vgcon->NodeCnt++] = node ; if (!v4graph_ConnectSuccess(ctx,vgcon)) return(TRUE) ; ;
	      vgcon->NodeCnt-- ; return(TRUE) ;
	    } else
	    { if (vgcon->NodeCnt >= vgcon->DepthMax) return(999) ;
	      vgcon->Nodes[vgcon->NodeCnt++] = node ;
	      v4graph_NextEdge2(ctx,vgcon,vgcon->Edge[i].NodeB) ;
	      vgcon->NodeCnt-- ;
	    } ;
	 } ;
	return(TRUE) ;
}

/*	Called when path from source to destination node is found - appends edges to resulting list */
LOGICAL v4graph_ConnectSuccess(ctx,vgcon)
  struct V4C__Context *ctx ;
  struct V4GRAPH__Connect *vgcon ;
{ struct V4DPI__Point ppnt,*ppntp ;
  struct V4DPI__LittlePoint lpnt ;
  struct V4L__ListPoint *lpr ;
  struct V4GRAPH__Weights *vgw ;
  int i ; double wgt ;

	vgcon->Success++ ;
	if (vgcon->Success > vgcon->ResultMax) return(TRUE) ;

	if (vgcon->vgw == NULL)
	 { ppntp = &ppnt ;			/* Need this to make INITLP & ENDLP macros to work */
	   INITLP(ppntp,lpr,Dim_List)	/* Create an Int2 point for each edge & append into new list */
	   intPNT(&lpnt) ;
	   for(i=0;i<vgcon->NodeCnt;i++)
	    { lpnt.Value.IntVal = vgcon->Nodes[i] ; if (!v4l_ListPoint_Modify(ctx,lpr,V4L_ListAction_Append,(P *)&lpnt,0)) return(FALSE) ; ;
	    } ;
	   lpnt.Value.IntVal = vgcon->Target ; if (!v4l_ListPoint_Modify(ctx,lpr,V4L_ListAction_Append,(P *)&lpnt,0)) return(FALSE) ;
	   ENDLP(ppntp,lpr)		/* Finish list & append this list as one of the results to vgcon */
	   if (!v4l_ListPoint_Modify(ctx,vgcon->lpr,V4L_ListAction_Append,&ppnt,0)) return(FALSE) ; ;
	   return(TRUE) ;
	 } ;
/*	Have Min/Max to determine */
	for(vgw=vgcon->vgw;vgw!=NULL;vgw=vgw->nvgw)
	 { if (vgw->ConstantWeight)	/* Get weight of this path */
	    { wgt = vgcon->NodeCnt+1 ; }
	    else { for(i=0,wgt=0;i<vgcon->NodeCnt;i++) { wgt += vgw->Nodes[i] ; } ; } ;
	   if (vgw->ResultType == V4GRAPH_ConnectRes_Minimum ? wgt < vgw->CurMinMaxVal : wgt > vgw->CurMinMaxVal)
	    { vgw->CurMinMaxVal = wgt ;
	      for(i=0;i<vgcon->NodeCnt;i++) { vgw->MinMaxNodes[i] = vgcon->Nodes[i] ; } ;
	      vgw->MinMaxNodes[i] = vgcon->Target ; vgw->MinMaxCnt = vgcon->NodeCnt+1 ;
	    } ;
	 } ;
	return(TRUE) ;
}

/*	T R A V E L I N G   S A L E S M A N   S T U F F		*/


/*	Generate a random path through all points on graph */
void vts_Random(graph,path)
  struct VTS__Graph *graph ;
  struct VTS__Path *path ;
{ int i,j,i0,j0,k ;

	for(i=0;i<graph->Nodes;i++) { path->To[i] = -1 ; } ;
	for(i0=i=0;i<graph->Nodes-1;i++)
	 { j = (int)floor(v4stat_ran0() * (graph->Nodes - i)) ;		/* (int)(r.nextLong()%(N-i)) */
	   path->To[i0] = 0 ;
	   for(j0=k=0;k<j;k++)
	    { j0++ ; for(;path->To[j0]!=-1;) { j0++ ; } } ;
	   for(;path->To[j0]!=-1;) { j0++ ; } ;
	   path->To[i0] = j0 ; path->From[j0] = i0 ;
	   i0 = j0 ;
	 } ;
	path->To[i0] = 0 ; path->From[0] = i0 ;
	vts_GetLength(graph,path) ;
}

LOGICAL vts_Improve(graph,path)
  struct VTS__Graph *graph ;
  struct VTS__Path *path ;
{ int i,j,h ; double d1,d2 ; double H[VTS_MAX_NODES] ;

	for(i=0;i<graph->Nodes;i++)
	 { H[i] = -DISTANCE(path->From[i],i) - DISTANCE(i,path->To[i]) + DISTANCE(path->From[i],path->To[i]) ;
	 } ;
	for(i=0;i<graph->Nodes;i++)
	 { d1 = DISTANCE(i,path->To[i]) ;
	   j = path->To[path->To[i]] ;
	   for(;j!=i;)
	    { d2 = H[j] + DISTANCE(i,j) + DISTANCE(j,path->To[i]) + d1 ;
	      if (d2 < 1e-10)
	       { h = path->From[j] ;
	         path->To[h] = path->To[j] ; path->From[path->To[j]] = h ;
	         h = path->To[i] ; path->To[i] = j ; path->To[j] = h ;
	         path->From[h] = j ; path->From[j] = i ;
	         vts_GetLength(graph,path) ;
	         return(TRUE) ;
	       } ;
	      j = path->To[j] ;
	    } ;
	 } ;
	return(FALSE) ;
}

LOGICAL vts_ImproveCross(graph,path)
  struct VTS__Graph *graph ;
  struct VTS__Path *path ;
{ int i,j,h,h1,hj ; double d1,d2,d ;

	for(i=0;i<graph->Nodes;i++)
	 { d1 = -DISTANCE(i,path->To[i]) ;		/* d1 = -distance from node i to next node */
	   j = path->To[path->To[i]] ;			/* j = next-next node from i */
	   d = 0 ;
	   for(;path->To[j]!=i;)
	    { d += DISTANCE(j,path->From[j]) - DISTANCE(path->From[j],j) ;
	      d2 = d1 + DISTANCE(i,j) + d + DISTANCE(path->To[i],path->To[j]) - DISTANCE(j,path->To[j]) ;
//VEH091210 Not sure why i==j, but if it does then don't count this as an improvement
	      if (i == j) { path->Length = 1E50 ; return(FALSE) ; } ;
	      if (d2 < -1e-10)
	       { h = path->To[i] ; h1 = path->To[j] ;
	         path->To[i] = j ;
	         path->To[h] = h1 ; path->From[h1] = h ;
	         hj = i ;
	         for(;j!=h;)
	          { h1 = path->From[j] ;
	            path->To[j] = h1 ;
	            path->From[j] = hj ;
	            hj = j ; j = h1 ;
	          } ;
	         path->From[j] = hj ;
	         vts_GetLength(graph,path) ;
	         return(TRUE) ;
	       } ;
	      j = path->To[j] ;
	    } ;
	 } ;
	return(FALSE) ;
}

/*	Calculates the length of a path on a given graph */
void vts_GetLength(graph,path)
  struct VTS__Graph *graph ;
  struct VTS__Path *path ;
{ int i ;

	path->Length = 0 ;
	for(i=0;i<graph->Nodes;i++)
	 { path->Length += DISTANCE(i,path->To[i]) ; } ;
} ;

/*	Optimze a path within a graph ('1' iteration, see vts_Optimize below) */
void vts_LocalOptimize(graph,path)
  struct VTS__Graph *graph ;
  struct VTS__Path *path ;
{
	for(;vts_Improve(graph,path);) {} ;
	for(;vts_ImproveCross(graph,path);)
	 { for(;vts_Improve(graph,path);) {} ;
	 } ;
}

/*	Optimize a path on graph with maximum iterations	*/

void vts_Optimize(graph,path,iterations)
  struct VTS__Graph *graph ;
  struct VTS__Path *path ;
{ struct VTS__Path mPath ;
  int count ;
  double lmin ;

	lmin = 1e50 ;
	count = 0 ;
	for(;count<iterations;)
	 { vts_Random(graph,path) ;
	   vts_LocalOptimize(graph,path) ;
	   if (path->Length < lmin - 1e-10)
	    { mPath = *path ;
	      lmin = path->Length ;
	      count = 0 ;
	    } else { count++ ; } ;
	 } ;
	*path = mPath ;
} ;

/*	Called from GraphConnect to perform Traveling Salesman Analysis	*/

struct V4DPI__Point *vts_Driver(ctx,respnt,iter,distdim,uomfctr,lp,xpt,ypt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt ;
  struct V4L__ListPoint *lp ;
  P *xpt, *ypt ;
  int iter,distdim,intmodx, trace ;
  LOGICAL uomfctr ;
{ P spnt, *ipt ;
  struct V4L__ListPoint *rlp ;
  struct VTS__Graph *graph ;
  struct VTS__Path path ;
  static double distFactors[] = { GEODISTFACTORS } ;
  struct {
    int Count ;
    int isGeo ;			/* TRUE then xCor & yCor are latitude/longitude, FALSE then positions on x-y plane */
    struct {
      double xCor,yCor ;
     } Node[VTS_MAX_NODES] ;
   } ts ;
  int i,j,ix,pnttype,ok ;
  
	graph = NULL ;
	for(i=1;i<=VTS_MAX_NODES;i++)				/* Load up graph */
	 { if (v4l_ListPoint_Value(ctx,lp,i,&spnt) <= 0) break ;
	   if (i == 1) { pnttype = spnt.PntType ; ts.isGeo = (pnttype == V4DPI_PntType_GeoCoord) ; }
	    else { if (spnt.PntType != pnttype) { v_Msg(ctx,NULL,"GraphTSSameType",intmodx,i,&spnt,pnttype) ; goto fail ; } ;
	         } ;
	   switch (spnt.PntType)
	    { default:
		if (xpt == NULL || ypt == NULL) { v_Msg(ctx,NULL,"GraphTSXY",intmodx,V4IM_Tag_X,V4IM_Tag_Y) ; goto fail ; } ;
		if (!v4ctx_FrameAddDim(ctx,0,&spnt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto fail ; } ;
		ipt = v4dpi_IsctEval(&spnt,xpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		if (ipt == NULL) { v_Msg(ctx,NULL,"TagEvalFail",intmodx,V4IM_Tag_X,xpt) ; goto fail ; } ;
		ts.Node[i-1].xCor = v4im_GetPointDbl(&ok,ipt,ctx) ;
	        if (!ok) { v_Msg(ctx,NULL,"GraphDblFail",intmodx,V4IM_Tag_X,i,ipt) ; goto fail ; } ;
		ipt = v4dpi_IsctEval(&spnt,ypt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		if (ipt == NULL) { v_Msg(ctx,NULL,"TagEvalFail",intmodx,V4IM_Tag_Y,ypt) ; goto fail ; } ;
		ts.Node[i-1].yCor = v4im_GetPointDbl(&ok,ipt,ctx) ;
	        if (!ok) { v_Msg(ctx,NULL,"GraphDblFail",intmodx,V4IM_Tag_Y,i,ipt) ; goto fail ; } ;
		break ;
	      case V4DPI_PntType_Int2:
		ts.Node[i-1].xCor = spnt.Value.Int2Val[0] ; ts.Node[i-1].yCor = spnt.Value.Int2Val[1] ; break ;
	      case V4DPI_PntType_GeoCoord:
		ts.Node[i-1].xCor = GETGEOLAT((struct V4DPI__Value_GeoCoord *)&spnt.Value) ;
		ts.Node[i-1].yCor = GETGEOLON((struct V4DPI__Value_GeoCoord *)&spnt.Value) ;
		break ;
	    } ;
	 } ;
	if (i > VTS_MAX_NODES) { v_Msg(ctx,NULL,"GraphMaxNode",intmodx,VTS_MAX_NODES) ; goto fail ; } ;
	ts.Count = i - 1 ;
/*	Set up graph structure - allocate big array of distances */
	graph = (struct VTS__Graph *)v4mm_AllocChunk(sizeof *graph + (ts.Count * ts.Count * sizeof(graph->distance[0])),FALSE) ;
	graph->Nodes = ts.Count ;
/*	Now init the graph with distances between nodes */
	for(i=0;i<graph->Nodes;i++)
	 { for(j=0;j<graph->Nodes;j++)
	    { if (ts.isGeo)
	       { double dist,phi,theta,x1,y1,z1,x2,y2,z2 ;
		 phi = 90.0 - ts.Node[i].xCor ; phi = phi * 2.0 * PI / 360.0 ;
		 theta = ts.Node[i].yCor * 2.0 * PI / 360.0 ;
		 x1 = MEANEARTHRADIUS * cos(theta) * sin(phi) ; y1 = MEANEARTHRADIUS * sin(theta) * sin(phi) ; z1 = MEANEARTHRADIUS * cos(phi) ;
		 phi = 90.0 - ts.Node[j].xCor ; phi = phi * 2.0 * PI / 360.0 ;
		 theta = ts.Node[j].yCor * 2.0 * PI / 360.0 ;
		 x2 = MEANEARTHRADIUS * cos(theta) * sin(phi) ; y2 = MEANEARTHRADIUS * sin(theta) * sin(phi) ; z2 = MEANEARTHRADIUS * cos(phi) ;
		 dist = sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2)) ;
		 dist = 2.0 * MEANEARTHRADIUS * asin(dist / (2.0 * MEANEARTHRADIUS)) ;
		 dist = dist * 1000 ;				/* dist = meters */
		 dist = dist / distFactors[uomfctr] ;
		 DISTANCE(i,j) = dist ;
	       } else
	       { DISTANCE(i,j) = sqrt((ts.Node[i].xCor - ts.Node[j].xCor)*(ts.Node[i].xCor - ts.Node[j].xCor) +
					     (ts.Node[i].yCor - ts.Node[j].yCor)*(ts.Node[i].yCor - ts.Node[j].yCor)) ;
	       } ;
	    } ;
	 } ;

//#define OKTOTRACETHISSTUFF
#ifdef OKTOTRACETHISSTUFF
for(i=0;i<graph->Nodes;i++)
 { UCCHAR tbuf[1024],tnum[20] ;
   UCsprintf(tbuf,20,UClit("%d\t"),i) ;
   for(j=0;j<graph->Nodes;j++)
    { UCsprintf(tnum,20,UClit("%g\t"),DISTANCE(i,j)) ; UCstrcat(tbuf,tnum) ; } ;
   UCstrcat(tbuf,UClit("\n")) ;
   vout_UCText(VOUT_Trace,0,tbuf) ;
 } ;
#endif

/*	Now do the optimization */
	if (iter == 0)
	 { vts_Random(graph,&path) ;
	   vts_LocalOptimize(graph,&path) ;
	 } else
	 { vts_Optimize(graph,&path,iter) ; } ;
/*	If we got distance dimenssion (distdim) then update in context */
	if (distdim != UNUSED)
	 { struct V4DPI__DimInfo *di ;
	   DIMINFO(di,ctx,distdim) ;
	   if (di->PointType == V4DPI_PntType_Real)	/* This dimension can only be Real or Int (see v4dpi_DimIndexDimId()) */
	    { dblPNTv(&spnt,path.Length) ; } else { intPNTv(&spnt,DtoI(path.Length)) ; } ;
	   spnt.Dim = di->DimId ;
	   if (!v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,&spnt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto fail ; } ;
	 } ;
/*	Return list of points that link correspond to the path */
	INITLP(respnt,rlp,Dim_List) ;
	for(ix=0;;)
	 { intPNTv(&spnt,ix+1) ;
	   if (!v4l_ListPoint_Modify(ctx,rlp,V4L_ListAction_Append,(P *)&spnt,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto fail ; } ;
	   ix = path.To[ix] ; if (ix == 0) break ;
	 } ;
	ENDLP(respnt,rlp)
	return(respnt) ;

fail:
	if (graph != NULL) v4mm_FreeChunk(graph) ;	/* This might be huge - better return it to the heap */
	return(NULL) ;
}


/*	v4im_DoOutput - Controls Output Streams in V4	*/

struct V4DPI__Point *v4im_DoOutput(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt ;
  INTMODX intmodx ;
  P *argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ P *cpt,spnt,bindpt ;
  struct V4DPI__LittlePoint idpt ;
  LOGICAL ok,gotid,doCreate,wantLines,wantBuffer ; INDEX i, tx, fx, icFlag ;
  VSTREAM targetStream, closeStream, countStream ;
  ETYPE texttype ;
  FILEID fileid,closeFileId,countFileId ;
  UCCHAR filename[V_FileName_Max] ;

	doCreate = TRUE ; ZUS(filename) ; gotid = FALSE ; targetStream = UNUSED ; fileid = UNUSED ; texttype = UNUSED ;
	closeFileId = UNUSED ; closeStream = UNUSED ; countFileId = UNUSED ; countStream = UNUSED ; wantBuffer = FALSE ; wantLines = TRUE ;
	logPNTv(respnt,TRUE) ; bindpt.Bytes = 0 ; icFlag = 0 ;
	for(ok=TRUE,i=1;ok&&i<=argcnt;i++)
	 { 
	   switch (tx=v4im_CheckPtArgNew(ctx,argpnts[i],&cpt,&spnt))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	      case V4IM_Tag_Append:	if (UCnotempty(filename)) { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"@Only one Append/Create argument is allowed") ; break ; } ;
					v4im_GetPointUC(&ok,filename,UCsizeof(filename),cpt,ctx) ; if (!ok) break ;
					doCreate = FALSE ; break ;
//	      case -V4IM_Tag_ASCII:	texttype = V4L_TextFileType_ASCII ; break ;
	      case V4IM_Tag_Binding:	memcpy(&bindpt,cpt,cpt->Bytes) ; break ;
	      
	      case -V4IM_Tag_Buffer:	wantBuffer = TRUE ; break ;
	      case V4IM_Tag_Close:	if ((closeStream = v4im_OutputGetStream(ctx,cpt)) == UNUSED) ok = FALSE ;
					break ;
	      case -V4IM_Tag_Close:	if (gotid)
					 { fileid = vout_PntIdToFileId(ctx,&idpt) ;
					   if (fileid == UNUSED) { v_Msg(ctx,NULL,"OutputNoId",intmodx,V4IM_Tag_Id,&idpt) ; goto fail ; } ;
					 } ;
					if (fileid != UNUSED) { closeFileId = fileid ; fileid = UNUSED ; }
					 else if (targetStream != UNUSED) { closeStream = targetStream ; targetStream = UNUSED ; }
					 else { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"OutputPriorFile",-V4IM_Tag_Close) ; } ;
					break ;
	      case V4IM_Tag_Create:	if (UCnotempty(filename)) { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"ModOnlyOne",V4IM_Tag_Append,V4IM_Tag_Create) ; break ; } ;
					v4im_GetPointUC(&ok,filename,UCsizeof(filename),cpt,ctx) ; if (!ok) break ;
					uccharPNTv(respnt,filename) ;		/* Return file name */
					break ;
	      case V4IM_Tag_Encode:	switch (v4im_GetDictToEnumVal(ctx,cpt))
					 { default:	v_Msg(ctx,NULL,"ListColArg",intmodx,V4IM_Tag_Length,DE(Bytes),DE(Line)) ; goto fail ;
					   case _ASCII:		texttype = V4L_TextFileType_ASCII ; break ;
					   case _UTF8:		texttype = V4L_TextFileType_UTF8 ; break ;
					   case _UTF8nh:	texttype = V4L_TextFileType_UTF8nh ; break ;
					   case _UTF16:		texttype = V4L_TextFileType_UTF16 ; break ;
					   case _UTF16le:	texttype = V4L_TextFileType_UTF16le ; break ;
					   case _UTF16be:	texttype = V4L_TextFileType_UTF16be ; break ;
					 } ;
					break ;
//	      case -V4IM_Tag_UTF8:	texttype = V4L_TextFileType_UTF8 ; break ;
//	      case -V4IM_Tag_UTF8nh:	texttype = V4L_TextFileType_UTF8nh ; break ;
//	      case -V4IM_Tag_UTF16:	texttype = V4L_TextFileType_UTF16 ; break ;
//	      case -V4IM_Tag_UTF16le:	texttype = V4L_TextFileType_UTF16le ; break ;
//	      case -V4IM_Tag_UTF16be:	texttype = V4L_TextFileType_UTF16be ; break ;
	      case -V4IM_Tag_Data:	
					i = ctx->Frame[ctx->FrameCnt - 1].DataStreamFileX ;
					uccharPNTv(respnt,vout_FileName(i == UNUSED ? VOUT_Data : -i)) ;
					return(respnt) ;
	      case -V4IM_Tag_FileName:	
					if (targetStream == VOUT_Data || (targetStream == UNUSED && !gotid))
					 { i = ctx->Frame[ctx->FrameCnt - 1].DataStreamFileX ;
					   i = (i == UNUSED ? VOUT_Data : -i) ;
					 } else
					 { if (gotid) { i = vout_PntIdToFileX(ctx,&idpt) ;
							if (i == UNUSED) { v_Msg(ctx,NULL,"OutputNoId",intmodx,V4IM_Tag_Id,&idpt) ; goto fail ; } ;
							i = -i ;
						      }
					    else if (targetStream != UNUSED) { i = targetStream ; }
					 } ;
					uccharPNTv(respnt,vout_FileName(i)) ;
					return(respnt) ;
	      case V4IM_Tag_Get:	fileid = vout_PntIdToFileId(ctx,&idpt) ;
					if (fileid == UNUSED) { v_Msg(ctx,NULL,"OutputNoId",intmodx,V4IM_Tag_Id,&idpt) ; goto fail ; } ;
					{ struct V4LEX__BigText *bt ; COUNTER maxChars,retChars ; FILEID fileid = UNUSED ;
					  if (gotid)
					   { fileid = vout_PntIdToFileId(ctx,&idpt) ;
					     if (fileid == UNUSED) { v_Msg(ctx,NULL,"OutputNoId",intmodx,V4IM_Tag_Id,&idpt) ; goto fail ; } ;
					   } ;
					  if (fileid == UNUSED && targetStream == UNUSED) { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"OutputPriorFile",-V4IM_Tag_Count) ; } ;
					  maxChars = v4im_GetPointInt(&ok,cpt,ctx) ;
					  if ((maxChars < 0 ? -maxChars : maxChars) > V4LEX_BigText_Max)
					   { v_Msg(ctx,NULL,"@%1E - Result string (%2d) exceeds max length allowed(%3d)",intmodx,maxChars,V4LEX_BigText_Max) ; goto fail ; } ;
					  bt = (struct V4LEX__BigText *)v4mm_AllocChunk(sizeof *bt,FALSE) ;
					  retChars = vout_GetStreamBuffer(fileid,targetStream,maxChars,bt->BigBuf,ctx->ErrorMsg) ;
					  if (retChars == UNUSED) { v_Msg(ctx,NULL,"OutputFileErr",intmodx) ; goto fail ; } ;
					  if (!v4dpi_SaveBigTextPoint(ctx,bt,UCstrlen(bt->BigBuf),respnt,Dim_Alpha,TRUE))
					   { v_Msg(ctx,NULL,"StrSaveBigText",intmodx,V4DPI_PntType_BigText) ; goto fail ; } ;
					  v4mm_FreeChunk(bt) ;	
					  return(respnt) ;	
					}
	      case V4IM_Tag_IC:		if (v4im_GetPointLog(&ok,cpt,ctx)) { icFlag = 0 ; }
					 else { icFlag = VLOGDECODE_KeepCase ; } ;
					break ;
	      case V4IM_Tag_Id:		if (gotid) { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"OutputOnly1a",V4IM_Tag_Id) ; break ; } ;
					if (cpt->Bytes != V4PS_Int) { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"OutputLittlePt",cpt,V4DPI_PntType_Dict,V4DPI_PntType_XDict) ; break ; } ;
					gotid = TRUE ; memcpy(&idpt,cpt,cpt->Bytes) ; break ;
	      case -V4IM_Tag_Length:	cpt = NULL ;
	      case V4IM_Tag_Length:	if (cpt != NULL)
					 { switch (v4im_GetDictToEnumVal(ctx,cpt))
					    { default:	v_Msg(ctx,NULL,"ListColArg",intmodx,V4IM_Tag_Length,DE(Bytes),DE(Line)) ; goto fail ;
					      case _Characters:	wantLines = FALSE ; break ;
					      case _Lines:	wantLines = TRUE ; break ;
					    } ;
					   } else { wantLines = FALSE ; } ;
					if (gotid)
					 { fileid = vout_PntIdToFileId(ctx,&idpt) ;
					   if (fileid == UNUSED) { v_Msg(ctx,NULL,"OutputNoId",intmodx,V4IM_Tag_Id,&idpt) ; goto fail ; } ;
					 } ;
					if (fileid != UNUSED) { countFileId = fileid ; fileid = UNUSED ; }
					 else if (targetStream != UNUSED) { countStream = targetStream ; targetStream = UNUSED ; }
					 else { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"OutputPriorFile",-V4IM_Tag_Count) ; } ;
					break ;
	      case -V4IM_Tag_ListOf:	break ;
	      case V4IM_Tag_To:		if (gpi->RestrictionMap & V_Restrict_DataStream)	/* If we have locked down the data stream then don't redirection of any streams */
					 { v_Msg(ctx,NULL,"OutputNoRedir",intmodx) ; goto fail ; } ;
					if (targetStream != UNUSED) { ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"OutputOnly1a",V4IM_Tag_To) ; break ; } ;
					if ((targetStream = v4im_OutputGetStream(ctx,cpt)) == UNUSED) ok = FALSE ;
					break ;
	      case -V4IM_Tag_To:	break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; goto fail ; } ;
/*	Got all argument - figure out what to do now */
	if (UCnotempty(filename))	/* Create/append file */
	 { if (texttype == UNUSED) texttype = gpi->OutputCharSet ;
	   if ((fileid = vout_OpenStreamFile((gotid ? &idpt : NULL),filename,NULL,NULL,(!doCreate),texttype,icFlag,ctx->ErrorMsgAux)) == UNUSED)
	    { v_Msg(ctx,NULL,"OutputFileErr",intmodx) ; goto fail ; } ;
	   gotid = FALSE ;
	 } ;
	if (wantBuffer)
	 { if ((fileid = vout_OpenStreamBuffer((gotid ? &idpt : NULL),ctx->ErrorMsgAux,FALSE)) == UNUSED)
	    { v_Msg(ctx,NULL,"OutputFileErr",intmodx) ; goto fail ; } ;
	   bindpt.Bytes = 0 ; gotid = FALSE ;
	 } ;
	if (bindpt.Bytes != 0)
	 { if ((fileid = vout_OpenStreamBigText((gotid ? &idpt : NULL),&bindpt,ctx->ErrorMsgAux)) == UNUSED)
	    { v_Msg(ctx,NULL,"OutputFileErr",intmodx) ; goto fail ; } ;
	   gotid = FALSE ;
	 } ;
	if (gotid)
	 { fileid = vout_PntIdToFileId(ctx,&idpt) ;
	   if (fileid == UNUSED)  { v_Msg(ctx,NULL,"OutputNoId",intmodx,V4IM_Tag_Id,&idpt) ; goto fail ; } ;
	 } ;
	if (targetStream != UNUSED)
	 { if (fileid == UNUSED) { v_Msg(ctx,NULL,"OutputNoFile",intmodx,V4IM_Tag_To) ; goto fail ; } ;
	   if (!vout_BindStreamFile(fileid,UNUSED,targetStream,ctx->ErrorMsgAux))
	    { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ; } ;
	 } ;
	if (closeStream != UNUSED || closeFileId != UNUSED)
	 { 
/*	   If we are closing default output for current frame (or any prior) then set output back to default */
	   if (closeFileId != UNUSED)
	    { fx = vout_FileIdToFileX(closeFileId) ;
	      if (ctx->Frame[ctx->FrameCnt-1].DataStreamFileX == fx)
	       ctx->Frame[ctx->FrameCnt-1].DataStreamFileX = UNUSED ;
	      if (ctx->FrameCnt <= 1 ? FALSE : ctx->Frame[ctx->FrameCnt-2].DataStreamFileX == fx)
	       { v_Msg(ctx,NULL,"OutputPriorFrm",intmodx) ; goto fail ; } ;
	      uccharPNTv(respnt,vout_FileName(-fx)) ;			/* Return file name */
	    } else
	    { uccharPNTv(respnt,vout_FileName(closeStream)) ;		/* Return file name */
	    } ;
	   if (!vout_CloseFile(closeFileId,closeStream,ctx->ErrorMsgAux))
	    { v_Msg(ctx,NULL,"OutputFileErr",intmodx) ; goto fail ; } ;
	 } ;
	if (countStream != UNUSED || countFileId != UNUSED)
	 { intPNT(respnt) ;
	   if ((respnt->Value.IntVal = vout_CountForFile(countFileId,countStream,wantLines,ctx->ErrorMsgAux)) == UNUSED)
	    { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ; } ;
	 } ;

	return(respnt) ;

fail:
	REGISTER_ERROR(0) ; RETURNFAILURE ;
}

int v4im_OutputGetStream(ctx,point)
  struct V4C__Context *ctx ;
  P *point ;
{
	switch (v4im_GetDictToEnumVal(ctx,point))
	 { default:		v_Msg(ctx,ctx->ErrorMsgAux,"@Invalid To::stream argument") ; return(UNUSED) ;
	   case _Data:		return(VOUT_Data) ;
	   case _Debug:		return(VOUT_Debug) ;
	   case _Error:		return(VOUT_Err) ;
	   case _Progress:	return(VOUT_Progress) ;
	   case _Prompt:	return(VOUT_Prompt) ;
	   case _Status:	return(VOUT_Status) ;
	   case _Trace:		return(VOUT_Trace) ;
	   case _Warn:		return(VOUT_Warn) ;
	 } ;
}


struct V4DPI__Point *v4im_DoGuiAlert(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt ;
  INTMODX intmodx ;
  P *argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ P *cpt,pntbuf ;
  LOGICAL ok ;
  INDEX i ;
  char msgbuf[V4TMBufMax] ;

	ZS(msgbuf) ;
	for(i=1,ok=TRUE;ok&&i<=argcnt;i++)
	 { if (argpnts[i]->PntType != V4DPI_PntType_TagVal)
	    { 
	      v4im_GetPointChar(&ok,ASCTBUF1,V4TMBufMax,argpnts[i],ctx) ; if (!ok) break ;
	      if (strlen(msgbuf) + strlen(ASCTBUF1) >= sizeof msgbuf)
	       { v_Msg(ctx,NULL,"StrLitTooBig2",intmodx,i,sizeof msgbuf) ; goto fail ; } ;
	      strcat(msgbuf,ASCTBUF1) ; continue ;
	    } ;
	   switch (v4im_CheckPtArgNew(ctx,argpnts[i],&cpt,&pntbuf))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; goto fail ; } ;

#ifdef WINNT
	MessageBoxEx(NULL,msgbuf,"V4 Alert",MB_OK|MB_SETFOREGROUND,LANG_NEUTRAL) ;
	return((P *)&Log_True) ;
#else
	v_Msg(ctx,NULL,"NYIonthisOS",intmodx) ; goto fail ;
#endif

fail:	REGISTER_ERROR(0) ; return(NULL) ;
}

struct V4DPI__Point *v4im_DoGuiMsgBox(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt ;
  INTMODX intmodx ;
  P *argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ P *cpt,pnt, fltrpt ;
  struct V4DPI__DimInfo *di ;
  struct V4L__ListPoint *fltrlp, *lp2 ; P listpt,descpt,patpt ; char fltrbuf[1024], *fbp ;\
  int ok, i, fx, msgtype, icon ;
  char title[256], message[512], desc[128], pattern[128], dfltdir[V_FileName_Max] ;
  enum DictionaryEntries deval ;
  
#ifdef WINNT

	msgtype = MB_OK ; strcpy(title,"V4 Message") ; icon = 0 ; fltrlp = NULL ; ZS(dfltdir)
	for(i=1,ok=TRUE;ok&&i<=argcnt;i++)
	 { if (argpnts[i]->PntType != V4DPI_PntType_TagVal)
	    { v4im_GetPointChar(&ok,message,sizeof message,argpnts[i],ctx) ; continue ;
	    } ;
	   switch (v4im_CheckPtArgNew(ctx,argpnts[i],&cpt,&pnt))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto MsgBox_fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto MsgBox_fail ;
	      case V4IM_Tag_Directory:	v4im_GetPointChar(&ok,dfltdir,sizeof dfltdir,cpt,ctx) ; break ;
	      case V4IM_Tag_Filter:
		fltrlp = v4im_VerifyList(&fltrpt,ctx,cpt,0) ; ZS(fltrbuf) ; fbp = fltrbuf ;
		for(fx=1;v4l_ListPoint_Value(ctx,fltrlp,fx,&listpt)>0;fx++)
		 { lp2 = v4im_VerifyList(NULL,ctx,&listpt,0) ;
	 	   if (v4l_ListPoint_Value(ctx,lp2,1,&descpt) == 0) { v_Msg(ctx,NULL,"GUIFilterBad",intmodx,argpnts[i]) ; goto MsgBox_fail ; } ;
		   v4im_GetPointChar(&ok,desc,sizeof desc,&descpt,ctx) ; if (!ok) break ;
		   if (v4l_ListPoint_Value(ctx,lp2,2,&patpt) == 0) { v_Msg(ctx,NULL,"GUIFilterBad",intmodx,argpnts[i]) ; goto MsgBox_fail ; } ;
		   v4im_GetPointChar(&ok,pattern,sizeof pattern,&patpt,ctx) ; if (!ok) break ;
		   if ((fbp - fltrbuf) + strlen(desc) + strlen(pattern) + 10 > sizeof fltrbuf)
		    { v_Msg(ctx,NULL,"GUIFilterBad",intmodx,argpnts[i]) ; goto MsgBox_fail ; } ;
		   strcpy(fbp,desc) ; fbp += strlen(desc) + 1 ;
		   strcpy(fbp,pattern) ; fbp += strlen(pattern) + 1 ;
		 } ; *fbp = '\0' ;
		break ;
	      case V4IM_Tag_Icon:
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:		v_Msg(ctx,NULL,"ModTagValue",intmodx,i,V4IM_Tag_Icon,cpt) ; goto MsgBox_fail ;
		   case _Exclamation:	icon = MB_ICONEXCLAMATION ; break ;
		   case _Hand:		icon = MB_ICONHAND ; break ;
		   case _Information:	icon = MB_ICONINFORMATION ; break ;
		   case _Question:	icon = MB_ICONQUESTION ; break ;
		   case _Stop:		icon = MB_ICONSTOP ; break ;
		 } ;
		break ;
	      case V4IM_Tag_Message:	v4im_GetPointChar(&ok,message,sizeof message,cpt,ctx) ; break ;
	      case V4IM_Tag_Title:	v4im_GetPointChar(&ok,title,sizeof title,cpt,ctx) ; break ;
	      case V4IM_Tag_Type:
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:			v_Msg(ctx,NULL,"ModTagValue",intmodx,i,V4IM_Tag_Type,cpt) ; goto MsgBox_fail ;
		   case _AbortRetryIgnore:	msgtype = MB_ABORTRETRYIGNORE ; break ;
		   case _OK:			msgtype = MB_OK ; break ;
		   case _RetryCancel:		msgtype = MB_RETRYCANCEL ; break ;
		   case _YesNo:			msgtype = MB_YESNO ; break ;
		   case _YesNoCancel:		msgtype = MB_YESNOCANCEL ; break ;
		   case _FileOpen:		msgtype = -1 ; break ;
		   case _FileSave:		msgtype = -2 ; break ;
		 } ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i-1) ; goto MsgBox_fail ; } ;
/*	If prompting for a file name then handle with different call */
	if (msgtype < 0)
	 { OPENFILENAME ofn ; char filebuf[V_FileName_Max] ;
	   memset(&ofn,0,sizeof ofn) ; ofn.lStructSize = sizeof ofn ;
	   ofn.lpstrFile = filebuf ; ofn.nMaxFile = sizeof filebuf ;
	   if (fltrlp != NULL) ofn.lpstrFilter = fltrbuf ;
	   ofn.lpstrInitialDir =  dfltdir ;
	   switch (msgtype)
	    { case -1:	   if (GetOpenFileName(&ofn) == 0) { v_Msg(ctx,NULL,"GUINoFileSpec",intmodx) ; goto MsgBox_fail ; } ; break ;
	      case -2:	   if (GetSaveFileName(&ofn) == 0) { v_Msg(ctx,NULL,"GUINoFileSpec",intmodx) ; goto MsgBox_fail ; } ; break ;
	    } ;
	   alphaPNT(respnt) ; strcpy(&respnt->Value.AlphaVal[1],ofn.lpstrFile) ; CHARPNTBYTES2(respnt,strlen(ofn.lpstrFile)) ;
	   return(respnt) ;
	 } ;
	i = MessageBoxEx(NULL,message,title,msgtype|icon|MB_SETFOREGROUND,LANG_NEUTRAL) ;
	dictPNT(respnt,Dim_UV4) ; DIMINFO(di,ctx,respnt->Dim) ;
	switch (i)
	 { case 0:		v_Msg(ctx,NULL,"GUIMsgBoxFail",intmodx,GetLastError()) ; goto MsgBox_fail ;
	   case IDABORT:	i = v4im_GetEnumToDictVal(ctx,deval=_Abort,Dim_UV4) ; break ;
	   case IDCANCEL:	i = v4im_GetEnumToDictVal(ctx,deval=_Cancel,Dim_UV4) ; break ;
	   case IDIGNORE:	i = v4im_GetEnumToDictVal(ctx,deval=_Ignore,Dim_UV4) ; break ;
	   case IDNO:		i = v4im_GetEnumToDictVal(ctx,deval=_No,Dim_UV4) ; break ;
	   case IDOK:		i = v4im_GetEnumToDictVal(ctx,deval=_OK,Dim_UV4) ; break ;
	   case IDRETRY:	i = v4im_GetEnumToDictVal(ctx,deval=_Retry,Dim_UV4) ; break ;
	   case IDYES:		i = v4im_GetEnumToDictVal(ctx,deval=_Yes,Dim_UV4) ; break ;
	 } ;
	respnt->Value.IntVal = i ; return(respnt) ;

#else
	v_Msg(ctx,NULL,"NYIonthisOS",intmodx) ; goto MsgBox_fail ;
#endif
MsgBox_fail:
	REGISTER_ERROR(0) ; return(NULL) ;
}

/*	v4im_DoTimer - Handles Timer() module		*/


#ifdef V4ENABLEMULTITHREADS
#define V4TIMER_ENTRY_MAX 100
struct V4TIMER__Entries {
  int count ;
  DCLSPINLOCK mtLock ;			/* Multi-thread lock */
  LOGICAL haveTimerThread ;		/* If TRUE then timer thread has been started */
  struct {
    struct V4DPI__LittlePoint idPnt ;	/* Optional Id point */
    P *ctxPt ;				/* Point to be added to context */
    int triggerUDT ;			/* UDT at which point is to be added */
   } entry[V4TIMER_ENTRY_MAX] ;
} *vtmr = NULL ;

void v4thread_Timer(arg)
  void *arg ;
{ struct V4C__Context *ctx = gpi->ctx ;		/* Need this for macros below to work */
  INDEX i ; int now ;

	for(;;)
	 { HANGLOOSE(1000) ;			/* Sleep 1 second */
	   now = valUDTisNOW ;
	   GRABMTLOCK(vtmr->mtLock) ;
	   for(i=0;i<vtmr->count;i++)
	    { if (vtmr->entry[i].triggerUDT > now) continue ;
	      v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,vtmr->entry[i].ctxPt,0,0) ;
	      v4mm_FreeChunk(vtmr->entry[i].ctxPt) ;
	      memcpy(&vtmr->entry[i],&vtmr->entry[vtmr->count-1],sizeof vtmr->entry[0]) ;
	      vtmr->count-- ; i-- ;
	    } ;
	   FREEMTLOCK(vtmr->mtLock) ;
	   if (vtmr->count <= 0) break ;	/* No sense to continue if nothing else to do */
	 } ;
	vtmr->haveTimerThread = FALSE ;
}
#endif

P *v4im_DoTimer(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  P *respnt,*argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ P t1pnt,t2pnt,*ipt,*cpt,ctxPnt ; struct V4DPI__LittlePoint idPnt ;
  struct V4L__ListPoint *lp ;
  int timeres ;
  double cpuseconds,wallseconds,delta ;
  LOGICAL ok ; INDEX i,ix,ex ;
  enum action { none, remove, udt, seconds } ; enum action toDo ;
#define NEWEX \
  if (ex == UNUSED) \
   { if (vtmr->count >= V4TIMER_ENTRY_MAX) { v_Msg(ctx,NULL,"TimerEntries",intmodx,V4TIMER_ENTRY_MAX) ; goto fail ; } ; \
     ex = vtmr->count ; memset(&vtmr->entry[ex],0,sizeof vtmr->entry[ex]) ; \
   } ;

#ifdef V4ENABLEMULTITHREADS
	if (vtmr == NULL) { vtmr = (struct V4TIMER__Entries *)v4mm_AllocChunk(sizeof *vtmr,FALSE) ; vtmr->count = 0 ; INITMTLOCK(vtmr->mtLock) ; vtmr->haveTimerThread = FALSE ; } ;
#endif
	cpuseconds = v_CPUTime() ; wallseconds = v_ConnectTime() ; ZPH(&ctxPnt) ; delta = UNUSED ; ZPH(&idPnt) ;
	timeres = 1 ; ex = UNUSED ; toDo = none ;

	ix = (argpnts[1]->PntType == V4DPI_PntType_Isct ? 2 : 1) ;
	for(;ix<=argcnt;ix++)
	 { switch (v4im_CheckPtArgNew(ctx,argpnts[ix],&cpt,&t1pnt))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:		v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
#ifdef V4ENABLEMULTITHREADS
	      case V4IM_Tag_At:			NEWEX
						switch (cpt->PntType)
						 { default:				v_Msg(ctx,NULL,"TimerVal",intmodx,V4IM_Tag_At,cpt,cpt->PntType) ; goto fail ;
						   case V4DPI_PntType_Int:		delta = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
						   case V4DPI_PntType_Real:		delta = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
						   case V4DPI_PntType_UDT:		delta = v4im_GetPointInt(&ok,cpt,ctx) - valUDTisNOW ;
						   case V4DPI_PntType_Calendar:		delta = vcal_CalToUDT(v4im_GetPointCal(&ok,cpt,ctx),VCAL_TimeZone_Local,&ok) - valUDTisNOW ; break ;
						   case V4DPI_PntType_UTime:		delta = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
						 } ;
						if (delta <= 0) { v_Msg(ctx,NULL,"TimerInPast",intmodx,cpt) ; goto fail ; } ;
						break ;
	      case V4IM_Tag_Context:		NEWEX memcpy(&ctxPnt,cpt,cpt->Bytes) ; break ;
	      case V4IM_Tag_Id:			if (cpt->Bytes > sizeof idPnt)
						 { v_Msg(ctx,NULL,"DimNoneVal",intmodx,V4IM_Tag_Id,V4DPI_PntType_Int,V4DPI_PntType_Dict,V4DPI_PntType_XDict) ; goto fail ; } ;
						NEWEX memcpy(&idPnt,cpt,cpt->Bytes) ; break ;
	      case -V4IM_Tag_ListOf:		INITLP(respnt,lp,Dim_List) ;
						if (vtmr == NULL) { ENDLP(respnt,lp) ; return(respnt) ; } ;
						GRABMTLOCK(vtmr->mtLock) ;
						{ for(i=0;i<(vtmr==NULL ? 0 : vtmr->count);i++)
						   { v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,(P *)&vtmr->entry[i].idPnt,0) ; } ;
						}
						ENDLP(respnt,lp) ;
						FREEMTLOCK(vtmr->mtLock) ;
						return(respnt) ;
	      case -V4IM_Tag_Remove:		toDo = remove ; break ;
	      case V4IM_Tag_Value:		switch (v4im_GetDictToEnumVal(ctx,cpt))
						 { default:		v_Msg(ctx,NULL,"ModTagValue",intmodx,ix,V4IM_Tag_Value,cpt) ; goto fail ;
						   case _Delta:		toDo = seconds ; break ;
						   case _UDT:		toDo = udt ; break ;
						 } ;
						break ;
#endif
	      case -V4IM_Tag_ConnectTime:	timeres = 2 ; break ;
	      case -V4IM_Tag_CPUTime:		timeres = 3 ; break ;
	    } ;
	 } ;

#ifdef V4ENABLEMULTITHREADS
/*	Are we timing an evaluation or setting a timer entry ? */
	if (ex == UNUSED && toDo == none) goto time_evaluation ;

/*	Here to add/remove timer entry or return value */
	if (toDo != none)
	 { if (ex == UNUSED ? TRUE : idPnt.Bytes == 0)
	    { v_Msg(ctx,NULL,"TimerNoId",intmodx,V4IM_Tag_Id) ; goto fail ; } ; 
	   GRABMTLOCK(vtmr->mtLock) ;
	   for (i=0;i<vtmr->count;i++)
	    { if (memcmp(&idPnt,&vtmr->entry[i].idPnt,vtmr->entry[i].idPnt.Bytes) == 0) break ;
	    } ;
	   if (i < vtmr->count)
	    { switch (toDo)
	       { case remove:		v4mm_FreeChunk(vtmr->entry[i].ctxPt) ;
					memcpy(&vtmr->entry[i],&vtmr->entry[vtmr->count-1],sizeof vtmr->entry[0]) ;
					vtmr->count-- ; intPNTv(respnt,vtmr->count) ;
					FREEMTLOCK(vtmr->mtLock) ;
					return(respnt) ;
		 case seconds:		intPNTv(respnt,vtmr->entry[i].triggerUDT-valUDTisNOW) ; FREEMTLOCK(vtmr->mtLock) ; return(respnt) ;
		 case udt:		intPNTv(respnt,vtmr->entry[i].triggerUDT) ; respnt->Dim = Dim_UDT ; respnt->PntType = V4DPI_PntType_UDT ; FREEMTLOCK(vtmr->mtLock) ; return(respnt) ;
	       } ;
	    } ;
	   FREEMTLOCK(vtmr->mtLock) ;
	   v_Msg(ctx,NULL,"TimerIdNotFnd",intmodx,&idPnt) ; goto fail ;
	 } ;
/*	Add a new timer entry */
	if (ctxPnt.Bytes == 0) { v_Msg(ctx,NULL,"TimerNoCtx",intmodx) ; goto fail ; } ;
	GRABMTLOCK(vtmr->mtLock) ;
	vtmr->entry[ex].ctxPt = (P *)v4mm_AllocChunk(ctxPnt.Bytes,FALSE) ; memcpy(vtmr->entry[ex].ctxPt,&ctxPnt,ctxPnt.Bytes) ;
	vtmr->entry[ex].triggerUDT = valUDTisNOW + delta ;
	if (idPnt.Bytes == 0) v4_UniqueDictPoint(ctx,(P *)&idPnt) ;
	memcpy(&vtmr->entry[ex].idPnt,&idPnt,idPnt.Bytes) ;
	vtmr->count++ ;
	FREEMTLOCK(vtmr->mtLock) ;
//	printf("Added timer #%d in +%d seconds\n",ex,(int)delta) ;
	if (!vtmr->haveTimerThread)
	 {
#ifdef CREATE_THREAD
	   { 
#ifdef WINNT
	     HANDLE threadId ;
#else
	     int threadId ;
#endif
	     CREATE_THREAD(v4thread_Timer,threadId,NULL,respnt,ctx->ErrorMsg)
	   } ;
#else
	   v_Msg(ctx,NULL,"ModTagNYI",intmodx,V4IM_Tag_At) ; goto fail ;
#endif
	 } ;
	 memcpy(respnt,&idPnt,idPnt.Bytes) ; return(respnt) ;
#endif

time_evaluation:
	if (argpnts[1]->PntType != V4DPI_PntType_Isct)
	 { v_Msg(ctx,NULL,"ModArgPntType2",intmodx,1,argpnts[1]->PntType,V4DPI_PntType_Isct) ; goto fail ; } ;
	ipt = v4dpi_IsctEval(respnt,argpnts[1],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	if (ipt == NULL) { v_Msg(ctx,NULL,"ModArgEval",intmodx,1,argpnts[1]) ; goto fail ; } ;

	cpuseconds = v_CPUTime() - cpuseconds ; wallseconds = v_ConnectTime() - wallseconds ;
	dblPNTv(&t1pnt,wallseconds) ; dblPNTv(&t2pnt,cpuseconds) ;
	switch (timeres)
	 {
	   case 1:		/* Return both times as a list */
		INITLP(respnt,lp,Dim_List) ;
		v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&t1pnt,0) ;
		v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&t2pnt,0) ;
		ENDLP(respnt,lp) ; break ;
	   case 2:
		memcpy(respnt,&t1pnt,t1pnt.Bytes) ; break ;
	   case 3:
		memcpy(respnt,&t2pnt,t2pnt.Bytes) ; break ;
	 } ;
	return(respnt) ;
fail:
	REGISTER_ERROR(0) ; return(NULL) ;
}




#ifdef WANTHTMLMODULE

/*	X M L  -  R P C   M O D U L E S						*/

#define TestMsg "\
<?xml version=\"1.0\"?>\n\
<methodResponse>\n\
 <fault>\n\
  <value>\n\
   <struct>\n\
     <member><name>faultCode</name>\n\
     <value><int>4</int></value></member>\n\
     <member> <name>faultString</name>\n\
      <value>\n\
        <string>Can't add a new story because there is no discussion group named \"dialogname\".</string>\n\
      </value> </member> </struct> </value>  </fault></methodResponse>"

#define cur (vxm->Cur)

struct xmlkwlist { int entry ; UCCHAR value[32] ; } ;
struct xmlkwlist xmlvaltypes[] =
 { { XML_ValType_Int, UClit("I4") }, { XML_ValType_Int, UClit("INT") }, { XML_ValType_Bool, UClit("BOOLEAN") }, { XML_ValType_String, UClit("STRING") },
   { XML_ValType_UDT, UClit("DATETIME.ISO8601") }, { XML_ValType_Dbl, UClit("DOUBLE") }, { XML_ValType_Base64, UClit("BASE64") },
   { XML_ValType_Struct, UClit("STRUCT") }, { XML_ValType_Array, UClit("ARRAY") }, { -1, UClit("") }
 } ;
P *vxml_RetrieveValueAsV4Point(ctx,vxm,respnt,errmsg)
  struct V4C__Context *ctx ;
  struct VXML__Message *vxm ;
  struct V4DPI__Point *respnt ;
  UCCHAR *errmsg ;
{ P elpt1,elpt2,lp1pt ;
  struct V4L__ListPoint *lp,*lp1 ;
  int state,valtype,i,len,ymd ; double dnum ; UCCHAR *b,*b1,*b2, tbuf[4096] ;

	state = vxml_MessageNext(vxm) ;		/* Get value type */
	if (UCstrcmp(vxm->Value,UClit("VALUE")) == 0) state = vxml_MessageNext(vxm) ;
	if (state != XML_State_InLvl) { v_Msg(ctx,(UCCHAR *)vxm->Value,"XMLInvValType",cur) ; vxm->State = XML_State_Err ; return(NULL) ; } ;
	for (i=0;;i++)
	 { if (xmlvaltypes[i].entry == -1) { v_Msg(ctx,vxm->Value,"XMLInvValType",vxm->Value) ; vxm->State = XML_State_Err ; return(NULL) ; } ;
	   if (UCstrcmp(xmlvaltypes[i].value,vxm->Value) == 0) break ;
	 } ;
	valtype = xmlvaltypes[i].entry ;
	state = vxml_MessageNext(vxm) ;
	switch (valtype)
	 {  
	   case XML_ValType_Int:
		if (state != XML_State_LitVal) { v_Msg(NULL,errmsg,"XMLNotLitVal",state,cur) ; return(NULL) ; } ;
		intPNTv(respnt,UCstrtol(vxm->Value,&b,10)) ;
		if (*b != UCEOS) { v_Msg(NULL,errmsg,"XMLInvIntVal",vxm->Value) ; return(NULL) ; } ;
		state = vxml_MessageNext(vxm) ; state = vxml_MessageNext(vxm) ;
		return(respnt) ;
	   case XML_ValType_Bool:
		if (state != XML_State_LitVal) { v_Msg(NULL,errmsg,"XMLNotLitVal",state,cur) ; return(NULL) ; } ;
		logPNTv(respnt,UCstrtol(vxm->Value,&b,10)) ;
		if (*b != UCEOS) { v_Msg(NULL,errmsg,"XMLInvIntVal",vxm->Value) ; return(NULL) ; } ;
		state = vxml_MessageNext(vxm) ; state = vxml_MessageNext(vxm) ;
		return(respnt) ;
	   case XML_ValType_UDT:
		if (state != XML_State_LitVal) { v_Msg(NULL,errmsg,"XMLNotLitVal",state,cur) ; return(NULL) ; } ;
		ZPH(respnt) ; respnt->Dim = Dim_UDT ; respnt->PntType = V4DPI_PntType_UDT ; respnt->Bytes = V4PS_Int ;
		b1 = UCstrchr(vxm->Value,'T') ; if (b1 != NULL) *b1 = UCEOS ;
		ymd = UCstrtol(vxm->Value,&b,10) ; if (*b != UCEOS) { v_Msg(NULL,errmsg,"XMLInvIntVal",vxm->Value) ; return(NULL) ; } ;
		respnt->Value.IntVal = vcal_UDTFromYMD(ymd/10000,((ymd/100)%100),(ymd%100),0,0,0,tbuf) ;
		if (respnt->Value.IntVal == VCAL_BadVal) { v_Msg(NULL,errmsg,"XMLResValue",vxm->Value,tbuf) ; return(NULL) ; } ;
		if (b1 != NULL)			/* Did we get a time portion? */
		 { int hh=0,mm=0,ss=0 ;
		   b2 = b1 + 1 ; b1 = UCstrchr(b2,':') ;
		   if (b1 != NULL) { *b1 = UCEOS ; hh = UCstrtol(b2,&b,10) ; if (*b != UCEOS) hh = 0 ; b2 = b1 + 1 ; b1 = UCstrchr(b2,':') ; } ;
		   if (b1 != NULL) { *b1 = UCEOS ; mm = UCstrtol(b2,&b,10) ; if (*b != UCEOS) mm = 0 ; b2 = b1 + 1 ; } ;
		   if (b1 != NULL) { *b1 = UCEOS ; ss = UCstrtol(b2,&b,10) ; if (*b != UCEOS) ss = 0 ; b2 = b1 + 1 ; } ;
		   respnt->Value.IntVal += (hh * 3600) + (mm * 60) + ss ;
		 } ;
		state = vxml_MessageNext(vxm) ; state = vxml_MessageNext(vxm) ;
		return(respnt) ;
	   case XML_ValType_String:
		if (state != XML_State_LitVal)
		 { if (!(state == XML_State_EndLvl && UCstrcmp(vxm->Value,UClit("STRING")) == 0))
		    { v_Msg(NULL,errmsg,"XMLNotLitVal",state,cur) ; return(NULL) ; } ;
		   ZS(vxm->Value) ;
		 } ;
		uccharPNT(respnt) ; len = UCstrlen(vxm->Value) ;
		if (len >= V4DPI_AlphaVal_Max - 1) { v_Msg(NULL,errmsg,"XMLAlphaMax",len,V4DPI_AlphaVal_Max) ; return(NULL) ; } ;
		UCstrcpy(&respnt->Value.UCVal[1],vxm->Value) ;
		UCCHARPNTBYTES2(respnt,len) ;
		if (len != 0) state = vxml_MessageNext(vxm) ; state = vxml_MessageNext(vxm) ;
		return(respnt) ;
	   case XML_ValType_Dbl:
		if (state != XML_State_LitVal) { v_Msg(NULL,errmsg,"XMLNotLitVal",state,cur) ; return(NULL) ; } ;
		dnum = UCstrtod(vxm->Value,&b) ; dblPNTv(respnt,dnum) ;
		if (*b != UCEOS) { v_Msg(NULL,errmsg,"XMLInvDblVal",vxm->Value) ; return(NULL) ; } ;
		state = vxml_MessageNext(vxm) ; state = vxml_MessageNext(vxm) ;
		return(respnt) ;
	   case XML_ValType_Base64:
	   case XML_ValType_Struct:
/*		Have to convert structure into a list of two-element lists */
		INITLP(respnt,lp,Dim_List) ;
		for(;;)
		 { if (UCstrcmp(vxm->Value,UClit("MEMBER")) != 0) state = vxml_MessageNext(vxm) ;
		    if (state == XML_State_EndLvl) break ;	/* Got </struct> - exit loop */
		    if (UCstrcmp(vxm->Value,UClit("MEMBER")) != 0) { v_Msg(NULL,errmsg,"XMLUnexpName",vxm->Value) ; return(NULL) ; } ;
		   state = vxml_MessageNext(vxm) ; if (UCstrcmp(vxm->Value,UClit("NAME")) != 0) { v_Msg(NULL,errmsg,"XMLUnexpName",vxm->Value) ; return(NULL) ; } ;
		   state = vxml_MessageNext(vxm) ; if (state != XML_State_LitVal) { v_Msg(NULL,errmsg,"XMLSyntaxErr",cur) ; return(NULL) ; } ; 
		   uccharPNT(&elpt1) ;
		   len = UCstrlen(vxm->Value) ; if (len >= V4DPI_AlphaVal_Max - 1) { v_Msg(NULL,errmsg,"XMLAlphaMax",len,V4DPI_AlphaVal_Max) ; return(NULL) ; } ;
		   UCstrcpy(&elpt1.Value.UCVal[1],vxm->Value) ;
		   UCCHARPNTBYTES2(&elpt1,len) ;
		   state = vxml_MessageNext(vxm) ; if (state != XML_State_EndLvl) { v_Msg(NULL,errmsg,"XMLSyntaxErr",cur) ; return(NULL) ; } ;
		   if (vxml_RetrieveValueAsV4Point(ctx,vxm,&elpt2,errmsg) == NULL) return(NULL) ;
		   state = vxml_MessageNext(vxm) ;
		   state = vxml_MessageNext(vxm) ;
		   INITLP((&lp1pt),lp1,Dim_List) ;
		   v4l_ListPoint_Modify(ctx,lp1,V4L_ListAction_Append,&elpt1,0) ;
		   v4l_ListPoint_Modify(ctx,lp1,V4L_ListAction_Append,&elpt2,0) ;
		   ENDLP((&lp1pt),lp1) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&lp1pt,0) ;
		 } ;
		ENDLP(respnt,lp) ; return(respnt) ;
	   case XML_ValType_Array:
/*		Have to convert structure into a list */
		INITLP(respnt,lp,Dim_List) ;
		if (UCstrcmp(vxm->Value,UClit("DATA")) != 0) state = vxml_MessageNext(vxm) ;
		for(;;)
		 { if (state == XML_State_EndLvl) break ;	/* Got </array> - exit loop */
		   state = vxml_MessageNext(vxm) ;
		   if (vxml_RetrieveValueAsV4Point(ctx,vxm,&elpt1,errmsg) == NULL) return(NULL) ;
		   if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&elpt1,0)) { UCstrcpy(errmsg,ctx->ErrorMsgAux) ; return(NULL) ; } ;
		 } ;
		ENDLP(respnt,lp) ; return(respnt) ;
		break ;
	 } ;
	return(NULL) ;
}

/*	v4im_DoXMLRPC - Handles XMLRPC() Module					*/

P *v4im_DoXMLRPC(ctx,respnt,argcnt,argpnts,intmodx,trace)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  P *respnt,*argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ P *ipt,*cpt, argpt ;
  struct V4HTML__Page *vhp ;
  struct VXML__Message vxm ;
  static UCCHAR LastErrorMsg[256] ; UCCHAR *ucb ;
  UCCHAR UserName[64], UserPassword[64], Module[512], URL[1024], path[1024], *ip ;
#define ARGMAX 30
  P *arguments[ARGMAX] ; int args ; UCCHAR *msg ;
  int ax,ok,i,len,Echo ; UCCHAR tbuf[512] ;

	ZUS(UserName) ; ZUS(UserPassword) ; args = 0 ; ZUS(Module) ; ZUS(URL) ;
	vhp = NULL ; msg = NULL ; Echo = 0 ;
	for(ax=1,ok=TRUE;ok && ax<=argcnt;ax++)			/* Loop thru all arguments */
	 { 
	   if (argpnts[ax]->PntType != V4DPI_PntType_TagVal)	/* If not a tag then RPC module argument */
	    { if (args >= ARGMAX) { v_Msg(ctx,NULL,"MaxNumModArgs",intmodx,ARGMAX) ; ipt = NULL ; goto xmlrpc_end ; } ;
	      arguments[args++] = argpnts[ax] ; continue ;
	    } ;
	   switch (v4im_CheckPtArgNew(ctx,argpnts[ax],&cpt,&argpt))
	    { default:	    		v_Msg(ctx,NULL,"TagBadUse",intmodx) ; ipt = NULL ; goto xmlrpc_end ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; ipt = NULL ; goto xmlrpc_end ;
	      case V4IM_Tag_Echo:
		switch(cpt->PntType)
		 { default:
			v_Msg(ctx,NULL,"ModArgPntType",intmodx,ax,cpt,cpt->PntType) ; ipt = NULL ; goto xmlrpc_end ;
		    CASEofChar
			v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,cpt,ctx) ;
			v_Msg(ctx,UCTBUF2,"TraceXMLRPC",UCTBUF1) ; vout_UCText(VOUT_Status,0,UCTBUF2) ;
			break ;
		   case V4DPI_PntType_Dict:
			switch (v4im_GetDictToEnumVal(ctx,cpt))
			 { default:		v_Msg(ctx,NULL,"ModTagValue",intmodx,ax,V4IM_Tag_Echo,cpt) ; ipt = NULL ; goto xmlrpc_end ;
			   case _None:		Echo = 0 ; break ;
			   case _Response:	Echo |= V4HTML_Trace_Body ; break ;
			   case _Request:	Echo |= V4HTML_Trace_Request ; break ;
			   case _All:		Echo |= 0xffffffff ; break ;
			 } ;
			break ;
		 } ; break ;
	      case -V4IM_Tag_Error:
		ucb = LastErrorMsg ; if (UCstrlen(ucb) >= 255) { ucb[250] = UCEOS ; UCstrcat(ucb,UClit("...")) ; } ;
		uccharPNTv(respnt,ucb) ; return(respnt) ;
	      case V4IM_Tag_Module:	v4im_GetPointUC(&ok,Module,UCsizeof(Module),cpt,ctx) ; break ;
	      case V4IM_Tag_Password:	v4im_GetPointUC(&ok,UserPassword,UCsizeof(UserPassword),cpt,ctx) ; break ;
	      case V4IM_Tag_URL:	v4im_GetPointUC(&ok,URL,UCsizeof(URL),cpt,ctx) ; break ;
	      case V4IM_Tag_User:	v4im_GetPointUC(&ok,UserName,UCsizeof(UserName),cpt,ctx) ; break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax-1) ; ipt = NULL ; goto xmlrpc_end ; } ;
	if (UCempty(Module) || UCempty(URL)) { v_Msg(ctx,NULL,"ModArgMand",intmodx) ; ipt = NULL ; goto xmlrpc_end ; } ;
/*	Now construct the XML-RPC module call packet */
	msg = v4mm_AllocUC(500000) ; ZUS(msg) ;	/* Allocate a mess-o-space for message */
	UCstrcat(msg,UClit(" <methodCall>\n")) ;
	v_Msg(ctx,tbuf,"@  <methodName>%1U</methodName>\n",Module) ; UCstrcat(msg,tbuf) ;
	UCstrcat(msg,UClit("  <params>\n")) ;
	for(i=0;i<args;i++)
	 { UCstrcat(msg,UClit("   <param>")) ;
	   len = UCstrlen(msg) ;
	   ok = vxml_RPCArg(ctx,arguments[i],&msg[len],500000-len-200) ;
	   if (ok == UNUSED) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; ipt = NULL ; goto xmlrpc_end ; } ;
	   UCstrcat(msg,UClit("</param>\n")) ;
	 } ;
	UCstrcat(msg,UClit("  </params>\n")) ;
	UCstrcat(msg,UClit(" </methodCall>\n")) ;
	if (Echo & V4HTML_Trace_Request) { vout_UCText(VOUT_Trace,0,msg) ; } ;
	vhp = malloc(sizeof *vhp) ;
	memset(vhp,0,sizeof *vhp) ; vhp->curEnd = -1 ;
	UCstrcpy(vhp->UserName,UserName) ; UCstrcpy(vhp->UserPassword,UserPassword) ;
	switch (v4html_ParseURL(vhp,URL,vhp->BaseHostName,vhp->BasePath,path,UCsizeof(path),&vhp->Port))
	 { default:
		v_Msg(ctx,NULL,"HTTPBadURL2",intmodx,URL) ; ipt = NULL ; goto xmlrpc_end ;
	   case V4HTML_URL_HTTP:
		vhp->PageType = V4HTML_PageType_Page ; break ;
	 } ;
	ip = v_URLAddressLookup(vhp->BaseHostName,ctx->ErrorMsgAux) ;
	if (ip == NULL) { v_Msg(ctx,NULL,"HTTPURLResolve2",intmodx,vhp->BaseHostName) ; ipt = NULL ; goto xmlrpc_end ; } ;
	if (v4html_RequestPage(vhp,V4HTMP_Page_XMLRPC,ip,vhp->Port,path,msg,UCstrlen(msg),NULL,ctx->ErrorMsgAux) == 0)
	 { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; ipt = NULL ; goto xmlrpc_end ; } ;
	if (vhp->ContentType != V4HTML_ContentType_XML) { v_Msg(ctx,NULL,"XMLResNotXML",intmodx,vhp->ContentType,vhp->Page) ; ipt = NULL ; goto xmlrpc_end ; } ;
/*	Got the response - parse it and convert to V4 point, if FAULT then create error message & fail */
	if (Echo & V4HTML_Trace_Body) vout_UCText(VOUT_Trace,0,vhp->Page) ;
	vxml_MessageSetup(&vxm,vhp->Page) ;
	for(ok=TRUE;ok;)
	 { vxml_MessageNext(&vxm) ;
	   switch(vxm.State)
	    { case XML_State_InLvl:	if (UCstrcmp(vxm.Value,UClit("METHODRESPONSE")) == 0) goto parse_result ; break ;
	      case XML_State_EOM:	ok = FALSE ; v_Msg(NULL,ctx->ErrorMsgAux,"XMLPremEOM") ; break ;	
	      case XML_State_Err:	ok = FALSE ; UCstrcpy(ctx->ErrorMsgAux,vxm.Value) ; break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ax-1) ; ipt = NULL ; goto xmlrpc_end ; } ;
parse_result:
	vxml_MessageNext(&vxm) ;
	if (UCstrcmp(vxm.Value,UClit("FAULT")) == 0)		/* Got error response? */
	 { vxml_RetrieveValueAsV4Point(ctx,&vxm,respnt,ctx->ErrorMsgAux) ;
	   v_Msg(ctx,NULL,"XMLRPCFault",intmodx,respnt) ; ipt = NULL ; goto xmlrpc_end ;
	 } ;
	if (UCstrcmp(vxm.Value,UClit("PARAMS")) != 0)
	 { v_Msg(ctx,NULL,"XMLPRCInvResp",intmodx,vxm.Value) ; ipt = NULL ; goto xmlrpc_end ; } ;
	 vxml_MessageNext(&vxm) ;
	if (UCstrcmp(vxm.Value,UClit("PARAM")) != 0)
	 { v_Msg(ctx,NULL,"XMLPRCInvResp",intmodx,vxm.Value) ; ipt = NULL ; goto xmlrpc_end ; } ;
/*	Got real result - return it as V4 point */
	ipt = vxml_RetrieveValueAsV4Point(ctx,&vxm,respnt,ctx->ErrorMsgAux) ;
	if (ipt == NULL) v_Msg(ctx,NULL,"XMLRPCNotValue",intmodx) ;

xmlrpc_end:
	if (vhp != NULL) v4mm_FreeChunk(vhp) ; if (msg != NULL) v4mm_FreeChunk(msg) ;
	if (ipt == NULL)
	 { if (UCstrlen(ctx->ErrorMsg) >= 255) ctx->ErrorMsg[250] = UCEOS ; UCstrcpy(LastErrorMsg,ctx->ErrorMsg) ;
	   REGISTER_ERROR(0) ;
	 } ;
	return(ipt) ;
}

/*	vxml_ParseNext - Parses and returns next token in XML message */

int vxml_ParseNext(vxm,flags)
  struct VXML__Message *vxm ;
  int flags ;
{
  int i ; char start ;

	if (flags & VXMLFlag_LitVal)		/* Parsing value between > & < ? */
	 { for(i=0;;)
	    { if (*cur == '&')
	       { int j ; char ebuf[20] ;
	         for (j=0,cur++;j<sizeof ebuf && *cur != ';';) { ebuf[j++] = toupper(*(cur++)) ; } ; ebuf[j] = '\0' ; cur++ ;
		 if (strcmp(ebuf,"LT") == 0) { vxm->Value[i < (XML_MsgCurValMax - 1) ? i++ : i ] = '<' ; }
		  else if (strcmp(ebuf,"GT") == 0) { vxm->Value[i < (XML_MsgCurValMax - 1) ? i++ : i ] = '>' ; }
		  else if (strcmp(ebuf,"AMP") == 0) { vxm->Value[i < (XML_MsgCurValMax - 1) ? i++ : i ] = '&' ; }
		  else { vxm->Value[i < (XML_MsgCurValMax - 1) ? i++ : i ] = '?' ; } ;
	       } else if (*cur == '<')
	          { vxm->Value[i] = '\0' ; if (i == 0) goto not_litval ; return(VXMLTkn_Value) ; }
	         else if (*cur == '\0') { return(VXMLTkn_PremEOM) ; }	/* End-of-Message ?? */
		 else if (*cur <= 26) {  cur++ ; }
		 else if ((*cur == ' ' || *cur == '\t') && i == 0) { cur ++ ; }	/* Ignore leading spaces, etc. */
		 else { vxm->Value[i < (XML_MsgCurValMax - 1) ? i++ : i ] = *(cur ++) ; } ;
	    } ;
	   return(VXMLTkn_TooBig) ;
	 } ;
not_litval:
	for(;;)
	 { switch (*cur)
	    { case '\0':	return(VXMLTkn_EOM) ;	/* End of message ? */
	      case '\n':
	      case '\r':
	      case ' ':		cur++ ; continue ;	/* Skip spaces */
	      case '\t':	cur++ ; continue ;	/*  and tabs */
	      case '=':		cur++ ; return(VXMLTkn_Equal) ;
	      case '<':		cur++ ;
				if (*cur == '/') { cur++ ; return(VXMLTkn_EndLvl) ; } ;
				if (*cur == '?') { cur++ ; return(VXMLTkn_XMLHdrStart) ; } ;
				return(VXMLTkn_NewLvl) ;
	      case '>':		cur ++ ; return(VXMLTkn_EndAngle) ;
	      case '/':		cur++ ;
				if (*cur == '>') { cur++ ; return(VXMLTkn_EndNewLvl) ; } ;
				return(VXMLTkn_Unexpected) ;
	      case '?':		cur++ ;
				if (*cur == '>') { cur++ ; return(VXMLTkn_XMLHdrEnd) ; } ;
				return(VXMLTkn_Unexpected) ;
	      case '\'':
	      case '"':		start = *cur ; cur++ ;
				for(i=0;i<=XML_MsgCurValMax-1;i++)
				 { if (*cur == '\0') return(VXMLTkn_PremEOM) ;
				   if (*cur == start) { vxm->Value[i] = '\0' ; cur++ ; return(VXMLTkn_Value) ; } ;
				   vxm->Value[i] = *cur ; cur++ ;
				 } ;
				return(VXMLTkn_TooBig) ;
	      default:		if (!((*cur >= 'A' && *cur <= 'Z') || (*cur >= 'a' && *cur <= 'z')))
				 return(VXMLTkn_Unexpected) ;
				for(i=0;i<=XML_MsgCurValMax-1;i++)
				 { char b ; b = toupper(*cur) ;
				   if ((b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9') || b == '.' || b == '_')
				    { vxm->Value[i] = b ; cur++ ; continue ; } ;
				   vxm->Value[i] = '\0' ; return(VXMLTkn_Keyword) ;
				 } ;
				return(VXMLTkn_TooBig) ;
	    } ;
	 } ;
}

/*	vxml_MessageSetup - Sets up vxm structure for parsing new XML message		*/
/*	Call: vxml_MessageSetup( vxm , msgbuf )
	  where vxm is pointer to (struct VXML__Message),
		msgbuf is point to null terminated ASCII string				*/

void vxml_MessageSetup(vxm,msgbuf)
  struct VXML__Message *vxm ;
  UCCHAR *msgbuf ;
{
	vxm->Begin = msgbuf ; cur = msgbuf ; vxm->Lvlx = 0 ;
	vxm->State = XML_State_BOM ;
/*	Position to the first "<" */
	for(;;cur++) { if (*cur == '<' || *cur == '\0') break ; } ;
	return ;
}

/*	vxml_MessageNext - Returns "next" component of XML Message as defined in vxm	*/
/*	Call: state = vxml_MessageNext( vxm )
	  where state is current state (see XML_State_xxx),
		vxm is pointer to (struct VXML__Message)				*/

int vxml_MessageNext(vxm)
  struct VXML__Message *vxm ;
{
  int type ;
  UCCHAR name[256] ;

top:
	switch (vxm->State)
	 { case XML_State_BOM:			/* Begin of XML message - maybe grab <?xml ... ?> header */
		type = vxml_ParseNext(vxm,0) ;
		switch (type)
		 { default:
		 	v_Msg(NULL,vxm->Value,"XMLInvBOM",cur) ; vxm->State = XML_State_Err ; goto top ;
		   case VXMLTkn_XMLHdrStart:
		   case VXMLTkn_NewLvl:
		   	vxm->State = XML_State_StartLvl ; goto top ;
		 } ;
		break ;
	   case XML_State_Err:
		return(vxm->State) ;
	   case XML_State_StartLvl:			/* Here to start a new level */
		type = vxml_ParseNext(vxm,0) ;		/* Maybe name='value' or maybe '/>' or '>' */
		if (type != VXMLTkn_Keyword)
		 { v_Msg(NULL,vxm->Value,"XMLNoLvlName",cur) ; vxm->State = XML_State_Err ; goto top ; } ;
		if (vxm->Lvlx >= XML_MsgLvlMax) { v_Msg(NULL,vxm->Value,"XMLNestMax",XML_MsgLvlMax) ; vxm->State = XML_State_Err ; goto top ; } ;
		UCstrcpy(vxm->Lvl[vxm->Lvlx].Name,vxm->Value) ; vxm->Lvlx++ ;
		return(vxm->State = XML_State_InLvl) ;
	   case XML_State_InLvl:			/* Here when in new level */
		type = vxml_ParseNext(vxm,0) ;		/* Maybe name='value' or maybe '/>' or '>' */
		switch (type)
		 { case VXMLTkn_Keyword:		return(vxm->State = XML_State_CheckAttVal) ;
		   case VXMLTkn_EndAngle:		vxm->State = XML_State_LookForLitVal ; goto top ;
		   case VXMLTkn_XMLHdrEnd:
		   case VXMLTkn_EndNewLvl:		UCstrcpy(vxm->Value,vxm->Lvl[vxm->Lvlx-1].Name) ;
							return(vxm->State=XML_State_EndLvl) ;
		 } ;
		break ;
	   case XML_State_CheckAttVal:
		type = vxml_ParseNext(vxm,0) ;		/* Maybe name='value' or maybe '/>' or '>' */
		switch (type)
		 { case VXMLTkn_Equal:			vxm->State = XML_State_GetAttVal ; goto top ;
		   case VXMLTkn_Keyword:		/* Got another keyword/attribute - return it */
			return(vxm->State = XML_State_CheckAttVal) ;
		   case VXMLTkn_EndAngle:
			vxm->State = XML_State_LookForLitVal ; goto top ;
		   case VXMLTkn_XMLHdrEnd:
		   case VXMLTkn_EndNewLvl:		UCstrcpy(vxm->Value,vxm->Lvl[vxm->Lvlx-1].Name) ;
							return(vxm->State=XML_State_EndLvl) ;
		 } ;
		break ;
	   case XML_State_GetAttVal:
		type = vxml_ParseNext(vxm,0) ;		/* Maybe name='value' or maybe '/>' or '>' */
		switch (type)
		 { case VXMLTkn_Keyword:
		   case VXMLTkn_Value:			return(vxm->State = XML_State_AttrValue) ;
		 } ;
		break ;
	   case XML_State_AttrValue:
		vxm->State = XML_State_InLvl ; goto top ;
	   case XML_State_LookForLitVal:
		type = vxml_ParseNext(vxm,VXMLFlag_LitVal) ;
		switch (type)
		 { case VXMLTkn_Value:
			return(vxm->State = XML_State_LitVal) ;
		   case VXMLTkn_EndLvl:
			vxm->State = XML_State_StartEndLvl ; goto top ;
		   case VXMLTkn_NewLvl:
			if (UCnotempty(vxm->Value))
			 { v_Msg(NULL,vxm->Value,"XMLNoEndLvl") ; vxm->State = XML_State_Err ; goto top ; } ;
			vxm->State = XML_State_StartLvl ; goto top ;
		 } ;
		break ;
	   case XML_State_LitVal:
		type = vxml_ParseNext(vxm,0) ;		/* Maybe name='value' or maybe '/>' or '>' */
		switch (type)
		 { case VXMLTkn_EndLvl:
			vxm->State = XML_State_StartEndLvl ; goto top ;
		   case VXMLTkn_NewLvl:
			if (UCnotempty(vxm->Value))
			 { v_Msg(NULL,vxm->Value,"XMLNoEndLvl") ; vxm->State = XML_State_Err ; goto top ; } ;
			vxm->State = XML_State_StartLvl ; goto top ;
		 } ;
		break ;
	   case XML_State_StartEndLvl:
		type = vxml_ParseNext(vxm,0) ;		/* Maybe name='value' or maybe '/>' or '>' */
		if (type != VXMLTkn_Keyword)
		 { v_Msg(NULL,vxm->Value,"XMLNoLvlName",cur) ; vxm->State = XML_State_Err ; goto top ; } ;
		return(vxm->State=XML_State_EndLvl) ;
	   case XML_State_EndLvl:
		if (UCstrcmp(vxm->Value,vxm->Lvl[vxm->Lvlx-1].Name) != 0)
		 { UCstrcpy(name,vxm->Value) ; v_Msg(NULL,vxm->Value,"XMLEndLevel",name,vxm->Lvl[vxm->Lvlx-1].Name) ; vxm->State = XML_State_Err ; goto top ; } ;
		vxm->Lvlx -- ;
		type = vxml_ParseNext(vxm,0) ;		/* Best have '>' */
		if (type == VXMLTkn_EndAngle) type = vxml_ParseNext(vxm,0) ;
		switch (type)
		 { case VXMLTkn_NewLvl:
			vxm->State = XML_State_StartLvl ; goto top ;
		   case VXMLTkn_EndLvl:
			vxm->State = XML_State_StartEndLvl ; goto top ;
		   case VXMLTkn_EOM:
			if (vxm->Lvlx != 0) { v_Msg(NULL,vxm->Value,"XMLPremEOM") ; vxm->State = XML_State_Err ; goto top ; } ;
			return(vxm->State = XML_State_EOM) ;
		 } ;
		break ;
	   case XML_State_EOM:
		return(vxm->State) ;
	 } ;
/*	If here then invalid syntax */
	v_Msg(NULL,vxm->Value,"XMLSyntaxErr",cur) ; vxm->State = XML_State_Err ; goto top ;
}

#endif

/*	vxml_RPCArg - Converts V4 point to XML-RPC argument (value) string	*/
/*	Call: len = vxml_RPCArg( ctx , point , argbuf , bufmax )
	  where len is length of result, UNUSED if errors (& ctx->ErrorMsgAux updated with message),
		ctx is context,
		point is point to be formatted,
		argbuf is destination buffer,
		bufmax is max number of bytes in argbuf				*/

int vxml_RPCArg(ctx,point,argbuf,bufmax)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *point ;
  UCCHAR *argbuf ; int bufmax ;
{
  struct V4DPI__DimInfo *di ;
  struct V4L__ListPoint *lp,*lp1 ; P ptbuf,ptbuf2 ;
  UCCHAR *type ; double dbl ;
  int i,j,len,tlen,rlen,rx,ok ; UCCHAR sname[128] ;

	switch (point->PntType)
	 { default:			v4dpi_PointToString(UCTBUF1,point,ctx,V4DPI_FormatOpt_Echo) ; type = UClit("string") ; break ;
	   case V4DPI_PntType_Int:	UCsprintf(UCTBUF1,50,UClit("%d"),point->Value.IntVal) ; type = UClit("int") ; break ;
	   case V4DPI_PntType_Real:	GETREAL(dbl,point) ;
					UCsprintf(UCTBUF1,50,UClit("%g"),dbl) ; type = UClit("double") ; break ;
	   case V4DPI_PntType_Logical:	UCstrcpy(UCTBUF1,(point->Value.IntVal > 0 ? UClit("1") : UClit("0"))) ; type = UClit("boolean") ; break ;
	   case V4DPI_PntType_UDate:	
	   case V4DPI_PntType_Calendar:
	   case V4DPI_PntType_UDT:	v_FormatDate(&point->Value,point->PntType,VCAL_CalType_UseDeflt,UNUSED,UClit("yyyy0m0dT0h:0n:0s"),UCTBUF1) ;
					type = UClit("datetime.iso8601") ; break ;
	   case V4DPI_PntType_List:
/*		List is a little more complicated - is it to be an array or structure? */
		DIMINFO(di,ctx,point->Dim) ;
		lp = v4im_VerifyList(NULL,ctx,point,0) ;
		if (di->Flags & V4DPI_DimInfo_Structure)
		 { if (bufmax <= 15) goto too_long ; UCstrcpy(argbuf,UClit("<value><struct>")) ; rx = UCstrlen(argbuf) ;
		   for(i=1;v4l_ListPoint_Value(ctx,lp,i,&ptbuf) > 0;i++)
		    { rx += 8 ; if (rx >= bufmax) goto too_long ; UCstrcat(argbuf,UClit("<member>")) ;
		      if (ptbuf.PntType != V4DPI_PntType_List) { v_Msg(ctx,ctx->ErrorMsgAux,"XMLArgNotStrct") ; return(UNUSED) ; } ;
		      lp1 = v4im_VerifyList(NULL,ctx,&ptbuf,0) ;
		      if (v4l_ListPoint_Value(ctx,lp1,1,&ptbuf2) < 0) { v_Msg(ctx,ctx->ErrorMsgAux,"XMLArgNoName") ; return(UNUSED) ; } ;
		      v4im_GetPointUC(&ok,sname,UCsizeof(sname),&ptbuf2,ctx) ; if (!ok) return(UNUSED) ;
		      rx += (6 + 7 + UCstrlen(sname)) ; if (rx > bufmax) goto too_long ;
		      UCstrcat(argbuf,UClit("<name>")) ; UCstrcat(argbuf,sname) ; UCstrcat(argbuf,UClit("</name>")) ;
		      if (v4l_ListPoint_Value(ctx,lp1,2,&ptbuf2) < 0) { v_Msg(ctx,ctx->ErrorMsgAux,"XMLArgNoName") ; return(UNUSED) ; } ;
		      len = vxml_RPCArg(ctx,&ptbuf2,&argbuf[rx],bufmax-rx) ;
		      if (len < 0) goto too_long ; rx += len ;
		      rx += 9 ; if (rx >= bufmax) goto too_long ; UCstrcat(argbuf,UClit("</member>")) ;
		    } ;
		   if (rx + 23 >= bufmax) goto too_long ;
		   UCstrcat(argbuf,UClit("</struct></value>")) ; rx += 23 ;
		 } else
		 { if (bufmax <= 20) goto too_long ; UCstrcpy(argbuf,UClit("<value><array><data>")) ; rx = UCstrlen(argbuf) ;
		   for(i=1;v4l_ListPoint_Value(ctx,lp,i,&ptbuf) > 0;i++)
		    { len = vxml_RPCArg(ctx,&ptbuf,&argbuf[rx],bufmax-rx) ;
		      if (len < 0) goto too_long ; rx += len ;
		    } ;
		   if (rx + 23 >= bufmax) goto too_long ;
		   UCstrcat(argbuf,UClit("</data></array></value>")) ; rx += 23 ;
		 } ;
		return(rx) ;
	 } ;
/*	Now put everything together: <value><type>argument</type></value> */
	for(i=0,j=0;;i++,j++)
	 { switch(UCTBUF2[j] = UCTBUF1[i])
	    { default:		if (UCTBUF2[j] == '\0') goto done ; continue ;
	      case UClit('<'):		UCTBUF2[j++] = UClit('&') ; UCTBUF2[j++] = UClit('l') ; UCTBUF2[j++] = UClit('t') ; break ;
	      case UClit('>'):		UCTBUF2[j++] = UClit('&') ; UCTBUF2[j++] = UClit('g') ; UCTBUF2[j++] = UClit('t') ; break ;
	      case UClit('\''):		UCTBUF2[j++] = UClit('&') ; UCTBUF2[j++] = UClit('a') ; UCTBUF2[j++] = UClit('p') ; UCTBUF2[j++] = UClit('o') ; UCTBUF2[j++] = UClit('s') ; break ;
	      case UClit('"'):		UCTBUF2[j++] = UClit('&') ; UCTBUF2[j++] = UClit('q') ; UCTBUF2[j++] = UClit('u') ; UCTBUF2[j++] = UClit('o') ; UCTBUF2[j++] = UClit('t') ; break ;
	      case UClit('&'):		UCTBUF2[j++] = UClit('&') ; UCTBUF2[j++] = UClit('a') ; UCTBUF2[j++] = UClit('m') ; UCTBUF2[j++] = UClit('p') ; break ;
	    } ; UCTBUF2[j] = UClit(';') ;
	 } ;
done:	len = UCstrlen(UCTBUF2) ; tlen = UCstrlen(type) ;
	if ((rlen = (7 + tlen+2 + len + tlen+3 + 8)) >= bufmax) goto too_long ;
	v_Msg(ctx,argbuf,"@<value><%1U>%2U</%3U></value>",type,UCTBUF2,type) ;
	return(rlen) ;

too_long:
	v_Msg(ctx,ctx->ErrorMsgAux,"XMLArgErrLen",point) ; return(UNUSED) ;
}

#define V4SXI_EntryMax 50		/* Max supported entries */
struct V4SXI__Map {
  int Count ;				/* Number below */
  int LUHandle ;			/* Last used handle value */
  struct {
   double IVal ;			/* Internal value */
   int EHandle ;			/* Unique entry handle */
   struct V4__PTBitMap PTMap ;		/* Bitmap of applicable point types */
   UCCHAR XVal[12] ;			/* External (display) value */
  } Entry[V4SXI_EntryMax] ;
} ;

static struct V4SXI__Map *vsxi = NULL ;

/*	v4sxi_SpecialListOfKW - Returns List of Special Keywords for a PointType	*/
/*	  Call: ok = v4sxi_SpecialListOfKW( ctx , respnt , pointtype , intmodx )
	  where ok is TRUE if respnt updated, FALSE if problem (ctx->ErrorMsg updated),
		ctx is context,
		respnt is updates with list value,
		pointtype is V4 point type to check
		intmodx is calling internal module index				*/

LOGICAL v4sxi_SpecialListOfKW(ctx,respnt,pointtype,intmodx)
  struct V4C__Context *ctx ;
  P *respnt ;
  int pointtype,intmodx ;
{
  P *ipt,epnt,spnt ;
  struct V4L__ListPoint *lp,*lp2 ;
  int mx,mb,i,ok ;

	INITLP(respnt,lp,Dim_List)
	mx = pointtype / 32 ; mb = 1 << (pointtype % 32) ;
	for(ok=FALSE,i=0;(vsxi == NULL ? FALSE : i<vsxi->Count);i++)
	 { if ((vsxi->Entry[i].PTMap.Map[mx] & mb) == 0) continue ;
/*	   Got a match - create epnt list of (internal-val external-val handle) */
	   ok = TRUE ; INITLP((ipt = &epnt),lp2,Dim_List)
	   dblPNTv(&spnt,vsxi->Entry[i].IVal) ;
	   v4l_ListPoint_Modify(ctx,lp2,V4L_ListAction_Append,&spnt,0) ;
	   uccharPNTv(&spnt,vsxi->Entry[i].XVal) ; v4l_ListPoint_Modify(ctx,lp2,V4L_ListAction_Append,&spnt,0) ;
	   intPNTv(&spnt,vsxi->Entry[i].EHandle) ; v4l_ListPoint_Modify(ctx,lp2,V4L_ListAction_Append,&spnt,0) ;

	   ENDLP(ipt,lp2) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&epnt,0) ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"V4NoSpecKW",intmodx,pointtype) ; return(FALSE) ; } ;
	ENDLP(respnt,lp) ; return(TRUE) ;
}

/*	v4sxi_SpecialAddKW - Adds new internal-external pair entry			*/
/*	  Call: entry = v4sxi_SpecialAddKW( ctx , ptmap , ival, xval )
	  where entry is integer index of new entry, UNUSED if a problem (ctx->ErrorMsg updated),
		ctx is context,
		ptmap is pointer to point-type bitmap,
		ival is internal value,
		xval is external value (display)					*/

int v4sxi_SpecialAddKW(ctx,intmodx,ptmap,ival,xval)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  struct V4__PTBitMap *ptmap ;
  double ival ;
  UCCHAR *xval ;
{
#ifdef V4ENABLEMULTITHREADS
  static DCLSPINLOCK sakLock = UNUSEDSPINLOCKVAL ;
#endif

	GRABMTLOCK(sakLock) ;
	if (vsxi == NULL)
	 { vsxi = (struct V4SXI__Map *)v4mm_AllocChunk(sizeof *vsxi,FALSE) ; vsxi->Count = 0 ; vsxi->LUHandle = 0 ; } ;
	if (vsxi->Count >= V4SXI_EntryMax) { v_Msg(ctx,NULL,"V4SpecADEntry",intmodx,V4SXI_EntryMax) ; FREEMTLOCK(sakLock) ; return(UNUSED) ; } ;
	vsxi->Entry[vsxi->Count].EHandle = ++vsxi->LUHandle ;
	vsxi->Entry[vsxi->Count].IVal = ival ;
	if (UCstrlen(xval) >= sizeof vsxi->Entry[0].XVal)
	 { v_Msg(ctx,NULL,"V4SpecADTooBig",intmodx,xval,(sizeof vsxi->Entry[0].XVal)-1) ; FREEMTLOCK(sakLock) ; return(UNUSED) ; } ;
	vsxi->Entry[vsxi->Count].PTMap = *ptmap ;
	UCstrcpy(vsxi->Entry[vsxi->Count].XVal,xval) ;
	vsxi->Count ++ ;
	FREEMTLOCK(sakLock) ;
	return(vsxi->LUHandle) ;
}

/*	v4sxi_SpecialDelEntry - Adds new internal-external pair entry			*/
/*	  Call: ok = v4sxi_SpecialDelEntry( ctx , intmodx , entry )
	  where ok is TRUE if deleted, FALSE if problem (ctx->ErrorMsg with error),
		ctx is context,
		entry is the entry handle to delete					*/

LOGICAL v4sxi_SpecialDelEntry(ctx,intmodx,entry)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  int entry ;
{
  int i ;

	if (vsxi == NULL) { v_Msg(ctx,NULL,"V4NoSpecAD",intmodx,entry) ; return(FALSE) ; } ;
	for(i=0;i<vsxi->Count;i++)
	 { if(vsxi->Entry[i].EHandle != entry) continue ;
	   for(i++;i<vsxi->Count;i++) { vsxi->Entry[i-1] = vsxi->Entry[i] ; } ;
	   vsxi->Count-- ; return(TRUE) ;
	 } ;
	v_Msg(ctx,NULL,"V4NoSpecAD",intmodx,entry) ; return(FALSE) ;
}

/*	v4sxi_SpecialAcceptor - Updates point with special value if text matches	*/
/*	  Call: ok = v4sxi_SpecialAcceptor( ctx , respnt , di , tval )
	  where ok is TRUE if respnt updated, FALSE if not,
		ctx is context,
		respnt is point to be updated,
		di is DimInfo for that point,
		tval is text string to check out					*/

LOGICAL v4sxi_SpecialAcceptor(ctx,respnt,di,tval)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *respnt ;
  struct V4DPI__DimInfo *di ;
  UCCHAR *tval ;
{ int mx,mb,i ;

	if (vsxi == NULL)
	 { return(FALSE) ; } ;		/* Nothing to check */
	mx = di->PointType / 32 ; mb = 1 << (di->PointType % 32) ;
	for(i=0;i<vsxi->Count;i++)
	 { if ((vsxi->Entry[i].PTMap.Map[mx] & mb) == 0) continue ;
	   if (UCstrcmpIC(tval,vsxi->Entry[i].XVal) == 0)
	    { ZPH(respnt) ; respnt->Dim = di->DimId ; respnt->PntType = di->PointType ;
	      switch(di->PointType)
	       { default:			respnt->Bytes = V4PS_Int ; respnt->Value.IntVal = (int)vsxi->Entry[i].IVal ; break ;
	         case V4DPI_PntType_TeleNum:	respnt->Bytes = V4PS_Tele ; PUTREAL(respnt,vsxi->Entry[i].IVal) ; break ;
		 case V4DPI_PntType_GeoCoord:
	         case V4DPI_PntType_Real:	respnt->Bytes = V4PS_Real ; PUTREAL(respnt,vsxi->Entry[i].IVal) ; break ;
	       } ;
	      return(TRUE) ;
	    } ;
	 } ;
	return(FALSE) ;					/* No match found */
}
/*	v4sxi_SpecialDisplayer - Updates string with special display value if point value matches	*/
/*	  Call: ok = v4sxi_SpecialDisplayer( ctx , pnt , bytes , tval )
	  where ok is TRUE if tval updated, FALSE if not,
		ctx is context,
		pnt is point to be checked,
		bytes is max bytes in tval,
		tval is text string to be updated							*/

LOGICAL v4sxi_SpecialDisplayer(ctx,pnt,bytes,tval)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *pnt ;
  int bytes ;
  UCCHAR *tval ;
{ int mx,mb,i,once ; double dnum ;

	if (vsxi == NULL) return(FALSE) ;		/* Nothing to check */
	if (bytes < UCsizeof(vsxi->Entry[0].XVal)) return(FALSE) ;
	mx = pnt->PntType / 32 ; mb = 1 << (pnt->PntType % 32) ;
	for(i=0,once=TRUE;i<vsxi->Count;i++)
	 { if ((vsxi->Entry[i].PTMap.Map[mx] & mb) == 0) continue ;
	   if (once)
	    { switch(pnt->PntType)
	       { default:			dnum = pnt->Value.IntVal ; break ;
	         case V4DPI_PntType_Color:	dnum = v_ColorRefToRGB(pnt->Value.IntVal) ; break ;
		 case V4DPI_PntType_GeoCoord:
		 case V4DPI_PntType_TeleNum:
		 case V4DPI_PntType_Real:	GETREAL(dnum,pnt) ; break ;
	       } ; once = FALSE ;
	    } ;
	   if (vsxi->Entry[i].IVal == dnum) { UCstrcpy(tval,vsxi->Entry[i].XVal) ; return(TRUE) ; } ;
	 } ;
	return(FALSE) ;					/* No match found */
}


//#include "optnumreac.c"

/**********************************************************************************************************************/
/*	T R E E   R O U T I N E S										      */
/**********************************************************************************************************************/

/*	v4tree_MakeTree - Makes a new tree and associates with a dimension	*/
/*	Call: tree = v4tree_MakeTree( ctx , dimId , maxnodes)
	  where tree is pointer to root of newly created tree (NULL if error in ctx->ErrorMsgAux),
		ctx is context,
		dimId is dimension associated with the tree (i.e. dim to return node ids on),
		maxnodes is expected number of nodes in tree (0 for default)	*/

struct V4Tree__Node *v4tree_MakeTree(ctx,dimId,maxnodes)
  struct V4C__Context *ctx ;
  DIMID dimId ;
  int maxnodes ;
{ 
  struct V4Tree__Master *tmas ;
  struct V4Tree__Node *tnode ;
  struct V4Tree__MemChunkDir *tmcd ;
  int tx ;

	if (gpi->vtd == NULL) v4tree_TreeMaster(ctx,UNUSED) ;

	for(tx=0;tx<gpi->vtd->nextTreeX;tx++)
	 { if (gpi->vtd->tree[tx].treeId == UNUSED) break ;
	 } ;
	if (tx >= TREE_MAX_NUMBER) { v_Msg(ctx,ctx->ErrorMsgAux,"TreeMaxTree",TREE_MAX_NUMBER) ; return(NULL) ; } ;
	gpi->vtd->tree[tx].treeId = tx ;
	if (tx == gpi->vtd->nextTreeX) gpi->vtd->nextTreeX++ ;
	tmas = (struct V4Tree__Master *)v4mm_AllocChunk(sizeof *tmas,TRUE) ;
	gpi->vtd->tree[tx].tmas = tmas ;
	tmas->rtc = (struct V4Tree__RTCache *)v4mm_AllocChunk(sizeof *tmas->rtc,TRUE) ;
	tmas->treeId = tx ; gpi->vtd->tree[tx].treeId = tx ;
	tmcd = (struct V4Tree__MemChunkDir *)v4mm_AllocChunk(sizeof *tmcd,FALSE) ;
	  tmcd->maxEntry = V4Tree_MemChunkDirInitial ; tmcd->curX = -1 ;
	tmas->tmcd = tmcd ; tmas->dimId = dimId ;
	tnode = v4tree_NewNode(ctx,tmas,FALSE) ;
	tmas->topNode = tnode ; tmas->isDirty = TRUE ;
	setNodePtr(gpi->vtd->tree[tx].topNodeLoc,tnode) ;
	return(tnode) ;
}

/*	v4tree_SaveTrees - Called on Area Close to save any trees in updateable area	*/
/*	Call ok = v4tree_SaveTree( ctx )						*/

LOGICAL v4tree_SaveTrees(ctx)
  struct V4C__Context *ctx ;
{
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4Tree__Master *tmas ;
  LOGICAL needVTD ; INDEX i,j,rx,areax ;
  
	if (gpi->vtd == NULL) return(TRUE) ;		/* Nothing to save */
	for(rx=gpi->HighHNum;rx>=gpi->LowHNum;rx--)
	 { if (rx == V4DPI_WorkRelHNum || gpi->RelH[rx].aid == UNUSED) continue ;
/*	   Is Area open for update? */
	   if ((gpi->RelH[rx].pcb->AccessMode & (V4IS_PCB_AM_Insert | V4IS_PCB_AM_Update)) != 0) break ;
	 } ;
/*	If rx too small then did not find update-able are, return FALSE */
	if (rx < gpi->LowHNum) return(FALSE) ;
	needVTD = FALSE ;
	FINDAREAINPROCESS(areax,gpi->RelH[rx].aid) ;
	GRABAREAMTLOCK(areax) ;
	for(i=0;i<gpi->vtd->nextTreeX;i++)
	 { if (gpi->vtd->tree[i].tmas == NULL) continue ;
	   tmas = gpi->vtd->tree[i].tmas ; gpi->vtd->tree[i].tmas = NULL ;
	   if (!tmas->isDirty) continue ;
/*	   Have to save this tree */
	   tmas->isDirty = FALSE ;
	   gpi->vtd->tree[i].dimId = tmas->dimId ; setNodePtr(gpi->vtd->tree[i].topNodeLoc,tmas->topNode) ;
	   gpi->vtd->tree[i].nextNodeId = tmas->nextNodeId ;
	   for(j=0;j<=tmas->tmcd->curX;j++)
	    { tmas->tmcd->entry[j].tmc->NextAvail = V4TREE_MemChunkSize ;	/* Set this to max so when we read in tmem in another process we won't erroneously think we have free memory (i.e. force allocation of new memory chunk) */
	      tmas->tmcd->entry[j].tmcSegId = v4seg_PutSegments(ctx,tmas->tmcd->entry[j].tmc,SIZEOFTMC(tmas->tmcd->entry[j].tmc),FALSE,FALSE) ;
	    } ;
	   gpi->vtd->tree[i].tmcdSegId = v4seg_PutSegments(ctx,tmas->tmcd,SIZEOFTMCD(tmas->tmcd),FALSE,FALSE) ;
	   needVTD = TRUE ;
	 } ;
	if (needVTD)				/* Save tree directory */
	 { gpi->vtd->kp.fld.KeyType = V4IS_KeyType_V4 ; gpi->vtd->kp.fld.KeyMode = V4IS_KeyMode_Int ; gpi->vtd->kp.fld.AuxVal = V4IS_SubType_TreeDir ;
	   gpi->vtd->kp.fld.Bytes = V4IS_IntKey_Bytes ; gpi->vtd->mustBeZero = 0 ;	/* Set up key field */
	   gpi->RelH[rx].pcb->PutBufPtr = (BYTE *)gpi->vtd ; gpi->RelH[rx].pcb->PutBufLen = SIZEOFVTD(gpi->vtd) ;
	   gpi->RelH[rx].pcb->KeyPtr = (struct V4IS__Key *)gpi->vtd,gpi->vtd ;
//printf("xvxx Saving tree len=%d\n", gpi->RelH[rx].pcb->PutBufLen) ;
	   if (!v4is_Put(gpi->RelH[rx].pcb,ctx->ErrorMsgAux)) { FREEAREAMTLOCK(areax) ; return(FALSE) ; } ;
	 } ;
	FREEAREAMTLOCK(areax) ;
	return(TRUE) ;
}


/*	v4tree_TreeMaster - Finds and returns (V4Tree__Master *) associated with a dimension	*/
/*	Call tmas = v4tree_TreeMaster( ctx , treeId )
	  where tmas is pointer to tree-master structure associated with dimension (NULL if error in ctx->ErrorMsgAux),
		ctx is context,
		treeId is tree we are looking for (UNUSED to just init tree stuff- returns NULL)		*/

struct V4Tree__Master *v4tree_TreeMaster(ctx,treeId)
  struct V4C__Context *ctx ;
  TREEID treeId ;
{
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4Tree__Master *tmas = NULL ; INDEX tx ;
  INDEX areax,rx,mx ; void *ptr ; LENMAX lenBytes ;
	
	if (gpi->vtd == NULL)
	 { gpi->vtd = v4mm_AllocChunk(sizeof *gpi->vtd,TRUE) ;
/*	   See if we can pull existing directory from any of the areas */
	   for(rx=gpi->LowHNum;rx<=gpi->HighHNum;rx++)
	    { if (rx == V4DPI_WorkRelHNum || gpi->RelH[rx].aid == UNUSED) continue ;
	      gpi->vtd->kp.fld.KeyType = V4IS_KeyType_V4 ; gpi->vtd->kp.fld.KeyMode = V4IS_KeyMode_Int ; gpi->vtd->kp.fld.AuxVal = V4IS_SubType_TreeDir ;
	      gpi->vtd->kp.fld.Bytes = V4IS_IntKey_Bytes ; gpi->vtd->mustBeZero = 0 ;	/* Set up key field */
	      FINDAREAINPROCESS(areax,gpi->RelH[rx].aid)
	      if (!GRABAREAMTLOCK(areax)) continue ;
	      if (v4is_PositionKey(gpi->RelH[rx].aid,(struct V4IS__Key *)gpi->vtd,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey)
	       { FREEAREAMTLOCK(areax) ; continue ; } ;
//printf("xvxx 811\n") ;
	      ptr = (void *)v4dpi_GetBindBuf(ctx,gpi->RelH[rx].aid,ikde,FALSE,&lenBytes) ;
//printf("812 %p, sizeof vtd=%d, lenBytes=%d\n",ptr, sizeof* gpi->vtd,lenBytes) ;
	      memcpy(gpi->vtd,ptr,lenBytes) ;
//printf("815\n") ;
	      FREEAREAMTLOCK(areax) ;
	      break ;
	    } ;
	 } ;
	if (treeId == UNUSED) return(NULL) ;
	for(tx=0;tx<gpi->vtd->nextTreeX;tx++)
	 { 
	   if (gpi->vtd->tree[tx].treeId != treeId) continue ;
	   tmas = gpi->vtd->tree[tx].tmas ; if (tmas != NULL) return(tmas) ;
	   tmas = (struct V4Tree__Master *)v4mm_AllocChunk(sizeof *tmas,TRUE) ;
/*	   Have to pull tree in from an area - let's get lookin */
	   ptr = v4seg_GetSegments(ctx,gpi->vtd->tree[tx].tmcdSegId,&lenBytes,FALSE,UNUSED) ;
	   if (ptr == NULL) break ;
	   tmas->tmcd = (struct V4Tree__MemChunkDir *)v4mm_AllocChunk(lenBytes+(5 * sizeof(tmas->tmcd->entry[0])),FALSE) ; memcpy(tmas->tmcd,ptr,lenBytes) ;
	    tmas->tmcd->maxEntry = tmas->tmcd->curX + 1 ;				/* VEH100930 - Set to max size to force allocation from heap on future processes trying to update this tree */
	   tmas->treeId = treeId ; tmas->nextNodeId = gpi->vtd->tree[tx].nextNodeId ; tmas->dimId = gpi->vtd->tree[tx].dimId ;
	   tmas->topNode = getNodePtr(gpi->vtd->tree[tx].topNodeLoc) ;
	   tmas->nextNodeId = gpi->vtd->tree[tx].nextNodeId ;
	   tmas->dimId = gpi->vtd->tree[tx].dimId ;
	   tmas->rtc = (struct V4Tree__RTCache *)v4mm_AllocChunk(sizeof *tmas->rtc,TRUE) ;
	   gpi->vtd->tree[tx].tmas = tmas ;
/*	   Now pull all memory chunks related to this tree */
	   for(mx=0;mx<=tmas->tmcd->curX;mx++)
	    { ptr = v4seg_GetSegments(ctx,tmas->tmcd->entry[mx].tmcSegId,&lenBytes,FALSE,UNUSED) ;
	      if (ptr == NULL) goto fail ;	/* Could not load the segment ? */
	      tmas->tmcd->entry[mx].tmc = (struct V4Tree__MemChunk *)v4mm_AllocChunk(lenBytes,FALSE) ;
	      memcpy(tmas->tmcd->entry[mx].tmc,ptr,lenBytes) ;
	      tmas->tmcd->entry[mx].tmc->NextAvail = V4TREE_MemChunkSize ;	/* VEH100930 - Set to max size to force allocation from heap on future processes trying to update this tree */
	    } ;
	   tmas->topNode = getNodePtr(gpi->vtd->tree[tx].topNodeLoc) ;
	   return(tmas) ;
	 } ;
fail:
	v_Msg(ctx,ctx->ErrorMsgAux,"TreeNotFnd",treeId) ; return(NULL) ;
}

/*	v4tree_NewNode - Creates a new node for a tree (but does not link it to the tree)	*/
/*	Call: node = v4tree_NewNode( ctx , tmas , cache )
	  where node is pointer to newly created tree node,
		ctx is context,
		tmas is tree-master pointer,
		cache is TRUE to cache new node							*/

struct V4Tree__Node *v4tree_NewNode(ctx,tmas,cache)
  struct V4C__Context *ctx ;
  struct V4Tree__Master *tmas ;
  LOGICAL cache ;
{
  struct V4Tree__MemChunk *tmem,*nmem ;
  struct V4Tree__Node *nnode ;

	tmem = (tmas->tmcd->curX < 0 ? NULL : tmas->tmcd->entry[tmas->tmcd->curX].tmc) ;
	if (tmem == NULL ? TRUE : tmem->NextAvail + sizeof *nnode >= V4TREE_MemChunkSize)
	 { nmem = (struct V4Tree__MemChunk *)v4mm_AllocChunk(sizeof *nmem,FALSE) ; nmem->NextAvail = 0 ;
	   tmas->tmcd->curX++ ;
	   if (tmas->tmcd->curX >= tmas->tmcd->maxEntry)	/* Out of slots - have to allocate bigger tmcd */
	    { int newmax = (tmas->tmcd->maxEntry == 1 ? 2 : tmas->tmcd->maxEntry * 1.5) ;
	      int newsize = (char *)&tmas->tmcd->entry[newmax] - (char *)tmas->tmcd ;
	      tmas->tmcd = (struct V4Tree__MemChunkDir *)realloc(tmas->tmcd,newsize) ;
	      tmas->tmcd->maxEntry = newmax ;
	    } ;
	   tmas->tmcd->entry[tmas->tmcd->curX].tmc = nmem ;
	   tmem = nmem ;
	 } ;
	nnode = (struct V4Tree__Node *)&tmem->Chunk[tmem->NextAvail] ; tmem->NextAvail += sizeof *nnode ; memset(nnode,0,sizeof *nnode) ;
	nnode->Id = TREENODE(tmas->treeId,tmas->nextNodeId) ; tmas->nextNodeId++ ;
	if (cache)
	 tmas->rtc->Cache[TREEHASH4CACHE(nnode->Id)].Node = nnode ;
	return(nnode) ;
}

/*	v4tree_AllocChunk - Allocates chunk of memory within tree's memory buffer	*/
/*	Call: ptr = v4tree_AllocChunk( ctx , tmas, bytes )
	  where ptr is pointer to available space,
		ctx is context,
		tmas is tree-master pointer,
		bytes is number of bytes to allocate					*/

BYTE *v4tree_AllocChunk(ctx,tmas,bytes)
  struct V4C__Context *ctx ;
  struct V4Tree__Master *tmas ;
  int bytes ;
{
  struct V4Tree__MemChunk *tmem,*nmem ;
  BYTE *chunk ;

	tmem = (tmas->tmcd->curX < 0 ? NULL : tmas->tmcd->entry[tmas->tmcd->curX].tmc) ;
	if (tmem == NULL ? TRUE : tmem->NextAvail + bytes >= V4TREE_MemChunkSize)
	 { nmem = (struct V4Tree__MemChunk *)v4mm_AllocChunk(sizeof *nmem,FALSE) ; nmem->NextAvail = 0 ;
	   tmas->tmcd->curX++ ;
	   if (tmas->tmcd->curX >= tmas->tmcd->maxEntry)	/* Out of slots - have to allocate bigger tmcd */
	    { int newmax = (tmas->tmcd->maxEntry == 1 ? 2 : tmas->tmcd->maxEntry * 1.5) ;
	      int newsize = (char *)&tmas->tmcd->entry[newmax] - (char *)tmas->tmcd ;
	      tmas->tmcd = (struct V4Tree__MemChunkDir *)realloc(tmas->tmcd,newsize) ;
	      tmas->tmcd->maxEntry = newmax ;
	    } ;
	   tmas->tmcd->entry[tmas->tmcd->curX].tmc = nmem ;
	   tmem = nmem ;
	 } ;
	chunk = (BYTE *)&tmem->Chunk[tmem->NextAvail] ; tmem->NextAvail += bytes ;
	return(chunk) ;
}


struct V4Tree__Node *v4tree_FindNode2(tmas,tnode,id,depth)
  struct V4Tree__Master *tmas ;
  struct V4Tree__Node *tnode ;
  NODEID id ;
  int depth ;
{ struct V4Tree__Node *resnode ;

	for(;;)
	 { if (tnode->Id == id) return(tnode) ;
/*	   Try searching children */
	   resnode = (isNodeDef(tnode->Child) ? v4tree_FindNode2(tmas,getNodePtr(tnode->Child),id,depth+1) : NULL) ;
	   if (resnode != NULL) return(resnode) ;
/*	   Did not find ? then try sibling */
	   if (isNodeNull(tnode->RightSibling)) return(NULL) ;
	   tnode = getNodePtr(tnode->RightSibling) ;
	 } ;
}

/*	v4tree_FindNode - Finds a node within a tree			*/
/*	Call: node = v4tree_FindNode( ctx , tmas , id )
	  where node is pointer to the node (NULL if not found, no error message),
		ctx is context,
		tmas is tree master,
		id is integer id of the node to be found		*/
struct V4Tree__Node *v4tree_FindNode(ctx,tmas,id)
  struct V4C__Context *ctx ;
  struct V4Tree__Master *tmas ;
  NODEID id ;
{
  struct V4Tree__Node *tnode ;
  int cx ;

	cx = TREEHASH4CACHE(id) ;			/* Calculate hash from Id */
	if (tmas->rtc->Cache[cx].Node == NULL ? FALSE : tmas->rtc->Cache[cx].Node->Id == id)
	 return(tmas->rtc->Cache[cx].Node) ;
/*	Not in cache - have to go find it (and insert into cache) */
	tnode = v4tree_FindNode2(tmas,tmas->topNode,id,0) ;
	if (tnode == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"TreeNodeNotFnd",tmas->treeId,id) ; return(NULL) ; } ;
	return(tmas->rtc->Cache[cx].Node = tnode) ;
}

/*	v4tree_ParentNode - Returns parent node of a node		*/
/*	Call: pnode = v4tree_ParentNode( tmas , node )
	  where pnode is parent node (NULL if no parent),
		node is node to find parent of				*/

struct V4Tree__Node *v4tree_ParentNode(tmas,tnode)
  struct V4Tree__Master *tmas ;
  struct V4Tree__Node *tnode ;
{ int cx,cx2 ;
  struct V4Tree__Node *entryNode ;

	cx = TREEHASHPNCACHE(tnode->Id) ;
	if (tmas->rtc->PNCache[cx].rmNode == tnode)
	 return(tmas->rtc->PNCache[cx].pNode) ;
	entryNode = tnode ;
	for(;tnode!=NULL;tnode=getNodePtr(tnode->LeftSibling))
	 { if (isNodeDef(tnode->Parent))
	    { /* tmas->rtc->PNCache[cx].rmNode = entryNode ; tmas->PNCache[cx].pNode = tnode->Parent ; */
	      return(getNodePtr(tnode->Parent)) ;
	    } ;
	   cx2 = TREEHASHPNCACHE(tnode->Id) ;
	   if (tmas->rtc->PNCache[cx2].rmNode == tnode)
	    return(tmas->rtc->PNCache[cx2].pNode) ;
	 } ; return(NULL) ;			/* No parent - better be top level node */
}

/*	v4tree_RemoveNode - Removes a node and all of its sub nodes from a tree				*/
/*	Call: ok = v4tree_RemoveNode( ctx , tmas , id )
	  where ok is TRUE if node successfully removed (FALSE if not with ctx->ErrorMsgAux updated),
		ctx is context,
		tmas is tree master,
		id is integer id of the node to be removed (root deletes/deallocates entire tree)	*/

LOGICAL v4tree_RemoveNode(ctx,tmas,id)
  struct V4C__Context *ctx ;
  struct V4Tree__Master *tmas ;
  NODEID id ;
{
  struct V4Tree__Node *tnode,*pnode,*lnode,*rnode ;
  int cx ;

	tnode = v4tree_FindNode(ctx,tmas,id) ;
	if (tnode == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"TreeNodeNotFnd",tmas->treeId,id) ; return(FALSE) ; } ;
	if ((pnode = v4tree_ParentNode(tmas,tnode)) == NULL) { v4tree_FellTree(ctx,tmas->treeId) ; return(TRUE) ; } ;
	pnode = getNodePtr(tnode->Parent) ; rnode = getNodePtr(tnode->RightSibling) ;
	lnode = getNodePtr(tnode->LeftSibling) ;
/*	Start by seeing if node has parent - if so link to this nodes right sibling */
	if (pnode != NULL)
	 { setNodePtr(pnode->Child,rnode) ;
	   if (rnode != NULL) { setNodePtr(rnode->Parent,pnode) ; copyNodeValue(rnode->LeftSibling,tnode->LeftSibling) ; } ;
	 } ;
/*	If this node has left/right sibling then link with other */
	if (lnode != NULL && pnode == NULL) setNodePtr(lnode->RightSibling,rnode) ;	/* Don't set if this is the left-most node */
	if (rnode != NULL) setNodePtr(rnode->LeftSibling,lnode) ;
/*	Be sure to flush from cache! */
	cx = TREEHASH4CACHE(id) ; tmas->rtc->Cache[cx].Node = NULL ;

	return(TRUE) ;
}

/*	v4tree_ElevateChildren - Makes children of node (id) children of id's parent node & then removes node id */
/*	Call: node = v4tree_ElevateChildren( ctx , tmas , id )
	  where node is id of parent node, NULL if failure,
		ctx is context,
		tmas is tree master,
		id is id of the node whose children are to be elevated		*/

struct V4Tree__Node *v4tree_ElevateChildren(ctx,tmas,id)
  struct V4C__Context *ctx ;
  struct V4Tree__Master *tmas ;
  NODEID id ;
{
  struct V4Tree__Node *tnode,*pnode,*cnode,*rsnode ;

	tnode = v4tree_FindNode(ctx,tmas,id) ;
	if (tnode == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"TreeNodeNotFnd",tmas->treeId,id) ; return(NULL) ; } ;
	if ((pnode = v4tree_ParentNode(tmas,tnode)) == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"TreeNoParent",tmas->treeId,id) ; return(NULL) ; } ;
	if (isNodeNull(tnode->Child))
	 { v_Msg(ctx,ctx->ErrorMsgAux,"TreeNoChildren",tmas->treeId,id) ; return(NULL) ; } ;
/*	Elevate all children of current node to be children of parent node */
	for(cnode=getNodePtr(tnode->Child);cnode!=NULL;cnode=rsnode)
	 { rsnode=getNodePtr(cnode->RightSibling) ;	/* Remember the next sibling before clearing out below */
	   setNodeNULL(cnode->Parent) ; setNodeNULL(cnode->LeftSibling) ; setNodeNULL(cnode->RightSibling) ;
	   v4tree_SproutNode(ctx,tmas,pnode->Id,V4TREE_RelPos_Child,cnode) ;
	 } ;
/*	Remove this node from tree */
	v4tree_RemoveNode(ctx,tmas,id) ;
/*	Return parent node */
	return(pnode) ;
}


/*	v4tree_SwapNodes - Swaps two nodes within a tree			*/
/*	Call: ok = v4tree_SwapNodes( ctx , tmas , id1 , id2 )
	  where ok is TRUE if node successfully removed (FALSE if not with ctx->ErrorMsgAux updated),
		ctx is context,
		tmas is tree master,
		id1/2 are integer ids of the nodes to be swapped		*/


LOGICAL v4tree_SwapNodes(ctx,tmas,id1,id2)
  struct V4C__Context *ctx ;
  struct V4Tree__Master *tmas ;
  NODEID id1,id2 ;
{ int cx ;
  struct V4Tree__Node *node1,*node2,*cnode ;

	node1 = v4tree_FindNode(ctx,tmas,id1) ; node2 = v4tree_FindNode(ctx,tmas,id2) ;
	if (node1 == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"TreeNodeNotFnd",tmas->treeId,id1) ; return(FALSE) ; } ;
	if (node2 == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"TreeNodeNotFnd",tmas->treeId,id2) ; return(FALSE) ; } ;
/*	All we really have to do is swap ids and child links */
	node1->Id = id2 ; node2->Id = id1 ;
	cnode = getNodePtr(node1->Child) ; copyNodeValue(node1->Child,node2->Child) ; setNodePtr(node2->Child,cnode) ;
/*	Be sure to flush from cache! */
	cx = TREEHASH4CACHE(id1) ; tmas->rtc->Cache[cx].Node = NULL ;
	cx = TREEHASH4CACHE(id2) ; tmas->rtc->Cache[cx].Node = NULL ;
	return(TRUE) ;
}

/*	v4tree_FellTree - Deletes a tree and all nodes			*/
/*	Call: ok = v4tree_FellTree( ctx , treeId )
	  where ok is TRUE if tree is removed, FALSE if error (in ctx->ErrorMsgAux),
		ctx is context,
		treeId is ID with the tree		*/

LOGICAL v4tree_FellTree(ctx,treeId)
  struct V4C__Context *ctx ;
  TREEID treeId ;
{
  struct V4Tree__Master *tmas ;
  INDEX i ;
	
	tmas = v4tree_TreeMaster(ctx,treeId) ;
	if (tmas == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"TreeNotFnd",treeId) ; return(FALSE) ; } ;
	for(i=0;i<tmas->tmcd->curX;i++) { v4mm_FreeChunk(tmas->tmcd->entry[i].tmc) ; } ;
/*	Finally remove tmas entry from chain of active trees */
	for(i=0;i<gpi->vtd->nextTreeX;i++)
	 { if (gpi->vtd->tree[i].treeId != treeId) continue ;
	   memset(&gpi->vtd->tree[i],0,sizeof(gpi->vtd->tree[i])) ;
	   gpi->vtd->tree[i].treeId = UNUSED ;
	   break ;
	 } ;
	v4mm_FreeChunk(tmas) ;		/* And finally the tree master */
	return(TRUE) ;
}

LOGICAL v4tree_SetTreeHash64(ctx,treeId)
  struct V4C__Context *ctx ;
  TREEID treeId ;
{ B64INT hash64 ;
  struct V4Tree__Master *tmas ;
  INDEX tx ; LOGICAL ok ;

	tmas = v4tree_TreeMaster(ctx,treeId) ;
	if (tmas == NULL) return(FALSE) ;
	hash64 = 0 ; ok = TRUE ; v4tree_Hash64(ctx,tmas,tmas->topNode,&hash64,0,&ok) ;
	if (!ok) return(FALSE) ;
	for(tx=0;tx<gpi->vtd->nextTreeX;tx++)
	 { if (gpi->vtd->tree[tx].treeId != treeId) continue ;
	   gpi->vtd->tree[tx].treeHash = hash64 ;
	   return(TRUE) ;
	 } ;
	return(FALSE) ;
}

/*	v4tree_FindIdenticalTree - returns treeId of (another) tree identical to one given	*/
/*	Call: treeId2 = v4tre_FindIdenticalTree( ctx , treeId1 )
	  where treeId2 is id of an identical tree (UNUSED if none found),
		ctx is context,
		treeId1 is tree to be used as base for search					*/

TREEID v4tree_FindIdenticalTree(ctx,treeId)
  struct V4C__Context *ctx ;
  TREEID treeId ;
{
  struct V4Tree__Master *tmas ;
  INDEX tx ; B64INT treeHash ;

	tmas = v4tree_TreeMaster(ctx,treeId) ; treeHash = UNUSED ;
	if (tmas == NULL) return(UNUSED) ;
	for(tx=0;tx<gpi->vtd->nextTreeX;tx++)
	 { if (gpi->vtd->tree[tx].treeId != treeId) continue ;
	   treeHash = gpi->vtd->tree[tx].treeHash ; break ;
	 } ;
	for(tx=0;tx<gpi->vtd->nextTreeX;tx++)
	 { if (gpi->vtd->tree[tx].treeId == UNUSED) continue ;
	   if (gpi->vtd->tree[tx].treeId == treeId) continue ;
	   if (treeHash == gpi->vtd->tree[tx].treeHash)
	    return(gpi->vtd->tree[tx].treeId) ;
	 } ;
	return(UNUSED) ;
}

/*	v4tree_ChildList - Returns list of children points from a given node	*/
/*	Call: ok = v4tree_ChildList( ctx , tmas , tnode , lp , condpt)
	  where ok is TRUE if lp updated with children, FALSE if no children,
		ctx is context,
		tmas is tree master,
		tnode is reference node,
		lp is list pointer to be updated with children points,
		condpt, if not NULL, is test intersection to apply		*/

LOGICAL v4tree_ChildList(ctx,tmas,tnode,lp,condpt)
  struct V4C__Context *ctx ;
  struct V4Tree__Master *tmas ;
  struct V4Tree__Node *tnode ;
  struct V4L__ListPoint *lp ;
  struct V4DPI__Point *condpt ;
{ struct V4DPI__LittlePoint npt ;
  struct V4DPI__Point spnt,*ipt ;
  struct V4Tree__Node *cnode ;
  INDEX cx ; LOGICAL ok,res ;

	if (isNodeNull(tnode->Child))
	 { v_Msg(ctx,ctx->ErrorMsgAux,"TreeNoChildren",tmas->treeId,tnode->Id) ; return(FALSE) ; } ;
	treePNT(&npt) ; npt.Dim = tmas->dimId ;
	for(cnode=getNodePtr(tnode->Child);cnode!=NULL;cnode=getNodePtr(cnode->RightSibling))
	 { npt.Value.IntVal = cnode->Id ;
	   if (condpt != NULL)
	    { if (!v4ctx_FrameAddDim(ctx,0,(P *)&npt,0,0)) return(FALSE) ; CLEARCACHE ;
/*	      Cache this point because we will most likely try to reference it */
	      cx = TREEHASH4CACHE(cnode->Id) ; tmas->rtc->Cache[cx].Node = cnode ;
	      if ((ipt = v4dpi_IsctEval(&spnt,condpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL)) == NULL)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"TreeEnumIsct",condpt,condpt) ; return(FALSE) ; } ;
	      res = v4im_GetPointLog(&ok,ipt,ctx) ; if (!ok) return(FALSE) ;
	      if (!res) continue ;
	    } else
	    { cx = TREEHASH4CACHE(cnode->Id) ; tmas->rtc->Cache[cx].Node = cnode ;
	    } ;
	   if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,(P *)&npt,0)) return(FALSE) ;
	 } ;
	return(TRUE) ;
}

/*	v4tree_BottomList - Returns list of leaf (bottom) points from a given node	*/
/*	Call: ok = v4tree_BottomList( ctx , tmas , tnode , lp , condpt)
	  where ok is TRUE if lp updated with leave, FALSE if no leaves,
		ctx is context,
		tmas is tree master,
		tnode is reference node,
		lp is list pointer to be updated with leaf points,
		condpt, if not NULL, is test intersection to apply		*/

LOGICAL v4tree_BottomList(ctx,tmas,tnode,lp,condpt)
  struct V4C__Context *ctx ;
  struct V4Tree__Master *tmas ;
  struct V4Tree__Node *tnode ;
  struct V4L__ListPoint *lp ;
  struct V4DPI__Point *condpt ;
{ struct V4DPI__LittlePoint npt ;
  struct V4DPI__Point spnt,*ipt ;
  struct V4Tree__Node *cnode ;
  int ok,res,cx ;

/*	If this node does not have a child then it is a bottom point - append to list & return OK */
	if (isNodeNull(tnode->Child))
	 { treePNTv(&npt,tnode->Id) ; npt.Dim = tmas->dimId ;
	   if (condpt != NULL)
	    { if (!v4ctx_FrameAddDim(ctx,0,(P *)&npt,0,0)) return(FALSE) ; CLEARCACHE ;
/*	      Cache this point because we will most likely try to reference it */
	      cx = TREEHASH4CACHE(tnode->Id) ; tmas->rtc->Cache[cx].Node = tnode ;
	      if ((ipt = v4dpi_IsctEval(&spnt,condpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL)) == NULL)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"TreeEnumIsct",condpt,condpt) ; return(FALSE) ; } ;
	      res = v4im_GetPointLog(&ok,ipt,ctx) ; if (!ok) return(FALSE) ;
	      if (!res) return(TRUE) ;
	    } ;
	   if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,(P *)&npt,0)) return(FALSE) ;
	   return(TRUE) ;
	 } ;
/*	Go through each of the children */
	cnode = getNodePtr(tnode->Child) ;
	for(;cnode!=NULL;cnode=getNodePtr(cnode->RightSibling))
	 { if(!v4tree_BottomList(ctx,tmas,cnode,lp,condpt)) return(FALSE) ;
	 } ;
	return(TRUE) ;
}

/*	v4tree_RelNode - Returns relative node from reference point		*/
/*	Call: node = v4tree_RelNode( ctx , tmas , id , relpos )
	  where node is relative node from reference (NULL if none),
		ctx is context,
		tmas is tree master,
		id is id of reference node,
		relpos is relative position from reference - see V4TREE_RelPos_xxx */


struct V4Tree__Node *v4tree_RelNode(ctx,tmas,id,relpos)
  struct V4C__Context *ctx ;
  struct V4Tree__Master *tmas ;
  NODEID id ;
  int relpos ;
{
  struct V4Tree__Node *tnode ;

	tnode = v4tree_FindNode(ctx,tmas,id) ; if (tnode == NULL) return(NULL) ;
	switch(relpos)
	 { default:
	     v_Msg(ctx,ctx->ErrorMsgAux,"TreeRelPosBad",relpos) ; return(NULL) ;
	   case V4TREE_RelPos_Left:
	     tnode = (isNodeNull(tnode->Parent) ? getNodePtr(tnode->LeftSibling) : NULL) ; break ;
	   case V4TREE_RelPos_Right:
	     tnode = getNodePtr(tnode->RightSibling) ; break ;
	   case V4TREE_RelPos_Child:
	     tnode = getNodePtr(tnode->Child) ; break ;
	   case V4TREE_RelPos_Parent:
	     tnode = v4tree_ParentNode(tmas,tnode) ; break ;
	   case V4TREE_RelPos_FarLeft:
	     for(;;tnode=getNodePtr(tnode->LeftSibling)) { if (isNodeDef(tnode->Parent) || isNodeNull(tnode->LeftSibling)) break ; } ;
	     break ;
	   case V4TREE_RelPos_FarRight:
	     for(;;tnode=getNodePtr(tnode->RightSibling)) { if (isNodeNull(tnode->RightSibling)) break ; } ;
	     break ;
	 } ;
	if (tnode == NULL) v_Msg(ctx,ctx->ErrorMsgAux,"TreeNoRelNode",id) ;
	return(tnode) ;
}

/*	v4tree_SproutNode - Creates new node relative to reference point	*/
/*	Call: node = v4tree_SproutNode( ctx , tmas , id , relpos , nnode )
	  where node is pointer to new node (NULL if error in ctx->ErrorMsgAux),
		ctx is context,
		tmas is tree master,
		id is id of reference node,
		relpos is relative position from reference - see V4TREE_RelPos_xxx,
		nnode is node to be sprouted (NULL to allocate a new one)	 */

struct V4Tree__Node *v4tree_SproutNode(ctx,tmas,id,relpos,nnode)
  struct V4C__Context *ctx ;
  struct V4Tree__Master *tmas ;
  NODEID id ;
  int relpos ;
  struct V4Tree__Node *nnode ;
{ int cx ;
  struct V4Tree__Node *tnode,*tnode2,*pnode ;
  struct V4Tree__Node *entryNode ;
  
	entryNode = (tnode = v4tree_FindNode(ctx,tmas,id)) ; if (tnode == NULL) return(NULL) ;
	switch(relpos)
	 { default:
	     v_Msg(ctx,ctx->ErrorMsgAux,"TreeRelPosBad",relpos) ; return(NULL) ;
	   case V4TREE_RelPos_Left:
	     if (isNodeDef(tnode->Parent) || isNodeNull(tnode->LeftSibling)) goto farleft ;
	     if (v4tree_ParentNode(tmas,tnode) == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"TreeRelPosTop") ; return(NULL) ; } ;
	     if (nnode == NULL) nnode = v4tree_NewNode(ctx,tmas,FALSE) ;
	     setNodePtr(nnode->RightSibling,tnode) ; copyNodeValue(nnode->LeftSibling,tnode->LeftSibling) ;
	     setNodePtr(tnode->LeftSibling,tnode) ; setNodePtr(getNodePtr(nnode->LeftSibling)->RightSibling,nnode) ;
	     break ;
	   case V4TREE_RelPos_Right:
	     if (v4tree_ParentNode(tmas,tnode) == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"TreeRelPosTop") ; return(NULL) ; } ;
	     if (nnode == NULL) nnode = v4tree_NewNode(ctx,tmas,FALSE) ;
	     setNodePtr(nnode->LeftSibling,tnode) ; copyNodeValue(nnode->RightSibling,tnode->RightSibling) ;
	     setNodePtr(tnode->RightSibling,tnode) ; setNodePtr(getNodePtr(nnode->RightSibling)->LeftSibling,nnode) ;
	     break ;
	   case V4TREE_RelPos_Child:
	     if (isNodeNull(tnode->Child))		/* Current node has no child - just create it */
	      { if (nnode == NULL) nnode = v4tree_NewNode(ctx,tmas,FALSE) ; setNodePtr(tnode->Child,nnode) ; setNodePtr(nnode->Parent,tnode) ; break ; } ;
/*	     By definition, create new child as rightmost sibling to existing children */
/*	     Rightmost node via LeftSibling of leftmost node (i.e. first child of parent) */
	     tnode2 = (isNodeNull(getNodePtr(tnode->Child)->LeftSibling) ? getNodePtr(tnode->Child) : getNodePtr(getNodePtr(tnode->Child)->LeftSibling)) ;
	     if (nnode == NULL) nnode = v4tree_NewNode(ctx,tmas,FALSE) ;
	     setNodePtr(nnode->LeftSibling,tnode2) ; setNodePtr(tnode2->RightSibling,nnode) ;
	     setNodePtr(getNodePtr(entryNode->Child)->LeftSibling,nnode) ;	/* Save rightmost sibling */
	     cx = TREEHASHPNCACHE(nnode->Id) ;
	     tmas->rtc->PNCache[cx].pNode = entryNode ; tmas->rtc->PNCache[cx].rmNode = nnode ;
	     break ;
	   case V4TREE_RelPos_Parent:
	   case V4TREE_RelPos_FarLeft:
farleft:     for(tnode2=tnode;;tnode2=getNodePtr(tnode2->LeftSibling)) { if (isNodeDef(tnode2->Parent) || isNodeNull(tnode2->LeftSibling)) break ; } ;
	     pnode = getNodePtr(tnode2->Parent) ;
	     if (pnode == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"TreeRelPosTop") ; return(NULL) ; } ;
/*	     Create a new node, position to left of tnode2 & link it to parent */
	     if (nnode == NULL) nnode = v4tree_NewNode(ctx,tmas,FALSE) ;
	     setNodePtr(pnode->Child,nnode) ; setNodePtr(nnode->Parent,pnode) ; setNodeNULL(tnode2->Parent) ;
	     setNodePtr(nnode->RightSibling,tnode2) ; setNodePtr(tnode2->LeftSibling,nnode) ;
	     break ;
	   case V4TREE_RelPos_FarRight:
	     pnode = v4tree_ParentNode(tmas,tnode) ;
	     if (pnode) { v_Msg(ctx,ctx->ErrorMsgAux,"TreeRelPosTop") ; return(NULL) ; } ;
/*	     Rightmost node via LeftSibling of leftmost node (i.e. first child of parent) */
	     tnode2 = getNodePtr(isNodeNull(getNodePtr(pnode->Child)->LeftSibling) ? pnode->Child : getNodePtr(pnode->Child)->LeftSibling) ;
	     if (nnode == NULL) nnode = v4tree_NewNode(ctx,tmas,FALSE) ;
	     setNodePtr(nnode->LeftSibling,tnode2) ; setNodePtr(tnode2->RightSibling,nnode) ;
	     setNodePtr(getNodePtr(pnode->Child)->LeftSibling,nnode) ;	/* Save rightmost sibling */
	     cx = TREEHASHPNCACHE(nnode->Id) ;
	     tmas->rtc->PNCache[cx].pNode = entryNode ; tmas->rtc->PNCache[cx].rmNode = nnode ;
	     break ;
	 } ;

/*	nnode = New node, cache it & return */
	tmas->rtc->Cache[TREEHASH4CACHE(nnode->Id)].Node = nnode ;
	return(nnode) ;
}

/*	v4tree_Hash64 - Enumerates (depth-first) through a tree & calculates 64-bit hash		*/
/*	Call: ok = v4tree_Hash64( ctx , tmas , node, h64ptr , depth , ok )
	  where ok is TRUE if enumerated OK, FALSE if not (ctx->ErrorMsgAux with error),
		ctx is context,
		tmas is tree master,
		node is starting node (i.e. "top of tree" for this enumeration),
		h64ptr is pointer to (B64INT) to receive result	(NOTE: set *h64ptr = 0 before calling),
		depth is current depth of the tree (NOTE: set to 0 on first call),
		ok is set to FALSE if a problem								*/

int v4tree_Hash64(ctx,tmas,node,h64ptr,depth,ok)
  struct V4C__Context *ctx ;
  struct V4Tree__Master *tmas ;
  struct V4Tree__Node *node ;
  B64INT *h64ptr ;
  COUNTER depth ;
  LOGICAL *ok ;
{ P *ipt ;
  struct V4Tree__Node *cnode ;
  B64INT ha[2] ;

	if (depth > 75000)				/* Something is wrong with tree, prevent recursion -> oblivion */
	 { *ok = FALSE ; return(FALSE) ; } ;
/*	Hash the node & value for this node */
	ipt = getNodeValPt(node->Value) ;
	if (ipt != NULL)
	 { ha[0] = *h64ptr ;
	   ha[1] = v_Hash64b((char *)ipt,ipt->Bytes) ;
	   *h64ptr = v_Hash64b((char *)ha,sizeof ha) ;
	 } ;
/*	Go through each of the children */
	cnode = getNodePtr(node->Child) ;
	if (cnode != NULL)
	 { depth++ ;
	   ha[0] = depth << (depth & 0x1F) ; ha[1] = *h64ptr ; *h64ptr = v_Hash64b((char *)ha,sizeof ha) ;
	 } ;
	for(;cnode!=NULL;cnode=getNodePtr(cnode->RightSibling))
	 { if(!v4tree_Hash64(ctx,tmas,cnode,h64ptr,depth,ok)) return(FALSE);
	 } ;
	return(TRUE) ;
}

/*	v4tree_Enum - Enumerates (depth-first) through a tree							*/
/*	Call: ok = v4tree_Enum( ctx , tmas , node , enterpt , leavept , returnpt , bottompt , lp , depthdim , curdepth)
	  where ok is TRUE if enumerated OK, FALSE if not (ctx->ErrorMsgAux with error),
		ctx is context,
		tmas is tree master,
		node is starting node (i.e. "top of tree" for this enumeration),
		enterpt is isct to be executed when node is entered,
		leavept is isct to be executed when node is left,
		returnpt is isct to be executed when node is reentered (i.e. if node has mulitple children),
		bottompt is isct to be executed if node is leaf (no children). NOTE: if given then enter/return/leave are not executed for leaf nodes,
		lp is list pointer to be updated with points enumerated (a list of all enumerated points),
		depthdim, if not UNUSED, is dimension to be updated with depth of the enumeration,
		curdepth is current depth of enumeration							*/

int v4tree_Enum(ctx,tmas,node,enterpt,leavept,returnpt,bottompt,lp,depthdim,curdepth)
  struct V4C__Context *ctx ;
  struct V4Tree__Master *tmas ;
  struct V4Tree__Node *node ;
  struct V4DPI__Point *enterpt,*leavept,*returnpt,*bottompt ;
  struct V4L__ListPoint *lp ;
  int depthdim, curdepth ;
{ struct V4DPI__LittlePoint npt, dpnt ;
  struct V4DPI__Point *ipt, *failpt, spnt ;
  struct V4Tree__Node *cnode ;
  int trace = 0, cx ;
#define EVALNODE(enode,isct) \
  { treePNTv(&npt,enode->Id) ; npt.Dim = tmas->dimId ; if (!v4ctx_FrameAddDim(ctx,0,(P *)&npt,0,0)) goto fail1 ; CLEARCACHE ; \
    cx = TREEHASH4CACHE(enode->Id) ; tmas->rtc->Cache[cx].Node = enode ; \
    if ((ipt = v4dpi_IsctEval(&spnt,isct,ctx,V4DPI_EM_EvalQuote,NULL,NULL)) == NULL) { failpt = isct ; goto fail2 ; } ; \
    if (lp != NULL) { if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,ipt,0)) goto fail1 ; } ; \
  }

	if (depthdim != UNUSED)
	 { intPNTv(&dpnt,curdepth) ; dpnt.Dim = depthdim ; if (!v4ctx_FrameAddDim(ctx,0,(P *)&dpnt,0,0)) goto fail1 ; CLEARCACHE ; } ;
	if (isNodeNull(node->Child) && bottompt != NULL)
	 { EVALNODE(node,bottompt) ; return(TRUE) ; } ;
	if (enterpt != NULL) EVALNODE(node,enterpt) ;
/*	Go through each of the children */
	cnode = getNodePtr(node->Child) ;
	for(;cnode!=NULL;cnode=getNodePtr(cnode->RightSibling))
	 { if(!v4tree_Enum(ctx,tmas,cnode,enterpt,leavept,returnpt,bottompt,lp,depthdim,curdepth+1)) return(FALSE);
	   if (returnpt != NULL && isNodeDef(cnode->RightSibling))
	    { if (depthdim != UNUSED)
	       { intPNTv(&dpnt,curdepth) ; dpnt.Dim = depthdim ; if (!v4ctx_FrameAddDim(ctx,0,(P *)&dpnt,0,0)) goto fail1 ; CLEARCACHE ; } ;
	      EVALNODE(node,returnpt) ;
	    } ;
	 } ;
	if (leavept != NULL)
	 { if (depthdim != UNUSED)
	    { intPNTv(&dpnt,curdepth) ; dpnt.Dim = depthdim ; if (!v4ctx_FrameAddDim(ctx,0,(P *)&dpnt,0,0)) goto fail1 ; CLEARCACHE ; } ;
	   EVALNODE(node,leavept) ;
	 } ;
	return(TRUE) ;

fail1:	return(FALSE) ;
fail2:	v_Msg(ctx,ctx->ErrorMsgAux,"TreeEnumIsct",failpt,&npt) ; return(FALSE) ;
}

/*	v4tree_FindNodeDepth - Finds first node (or list of all nodes) within a tree using depth-first search	*/
/*	Call: v4tree_FindNodeDepth( ctx , tmas , node , condpt , lp )
	  where ctx is context,
		tmas is tree master,
		node is starting node of the tree,
		condpt is isct to test each node (if eval results in TRUE then node is taken),
		lp is list pointer to be updated with list of nodes matching condpt	*/

struct V4Tree__Node *v4tree_FindNodeDepth(ctx,tmas,node,condpt,lp)
  struct V4C__Context *ctx ;
  struct V4Tree__Master *tmas ;
  struct V4Tree__Node *node ;
  struct V4DPI__Point *condpt ;
  struct V4L__ListPoint *lp ;
{ struct V4DPI__LittlePoint npt ;
  struct V4DPI__Point *ipt, spnt ;
  struct V4Tree__Node *rnode, *cnode ;
  int res, ok, trace = 0 ;

	treePNTv(&npt,node->Id) ; npt.Dim = tmas->dimId ; if (!v4ctx_FrameAddDim(ctx,0,(P *)&npt,0,0)) return(NULL) ; CLEARCACHE ;
	if ((ipt = v4dpi_IsctEval(&spnt,condpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL)) == NULL)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"TreeEnumIsct",condpt,&npt) ; return(NULL) ; } ;
	res = v4im_GetPointLog(&ok,ipt,ctx) ; if (!ok) return(NULL) ;
	if (res)
	 { if (lp == NULL) return(node) ;
	   if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,(P *)&npt,0)) return(NULL) ;
	 } ;
/*	Go through each of the children */
	cnode = getNodePtr(node->Child) ;
	for(;cnode!=NULL;cnode=getNodePtr(cnode->RightSibling))
	 { rnode = v4tree_FindNodeDepth(ctx,tmas,cnode,condpt,lp) ;
	   if (lp == NULL && rnode != NULL) return(rnode) ;
	 } ;
	if (lp != NULL) return(node) ;			/* Have to return a node - might as well be first one */

/*	If here then nothing found - fail */
	v_Msg(ctx,ctx->ErrorMsgAux,"TreeFindNone",condpt,tmas->dimId,node->Id) ; return(NULL) ;
}

/*	v4tree_FindNodeBreadth - Finds first node (or list of all nodes) within a tree using breadth-first search	*/
/*	Call: v4tree_FindNodeBreadth( ctx , tmas , node , condpt , lp , ok )
	  where ctx is context,
		tmas is tree master,
		node is starting node of the tree,
		depth is current "wanted" depth of search,
		curdepth is current depth of search,
		condpt is isct to test each node (if eval results in TRUE then node is taken),
		lp is list pointer to be updated with list of nodes matching condpt,
		ok is set to TRUE/FALSE if OK or not									*/

struct V4Tree__Node *v4tree_FindNodeBreadth(ctx,tmas,node,depth,curdepth,nodecnt,condpt,lp,ok)
  struct V4C__Context *ctx ;
  struct V4Tree__Master *tmas ;
  struct V4Tree__Node *node ;
  int depth,curdepth,*nodecnt ;
  struct V4DPI__Point *condpt ;
  struct V4L__ListPoint *lp ;
  LOGICAL *ok ;
{ struct V4DPI__LittlePoint npt ;
  struct V4DPI__Point *ipt, spnt ;
  struct V4Tree__Node *rnode, *cnode ;
  int res, cx, trace = 0 ;

	if (curdepth > depth) return(NULL) ;
	if (depth == curdepth)
	 { (*nodecnt)++ ;
	   treePNTv(&npt,node->Id) ; npt.Dim = tmas->dimId ; if (!v4ctx_FrameAddDim(ctx,0,(P *)&npt,0,0)) return(NULL) ; CLEARCACHE ;
/*	   Cache this point because we will most likely try to reference it */
	   cx = TREEHASH4CACHE(node->Id) ; tmas->rtc->Cache[cx].Node = node ;
	   if ((ipt = v4dpi_IsctEval(&spnt,condpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL)) == NULL)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"TreeEnumIsct",condpt,&npt) ; *ok = FALSE ; return(NULL) ; } ;
	   res = v4im_GetPointLog(ok,ipt,ctx) ; if (!*ok) return(NULL) ;
	   if (res)
	    { if (lp == NULL) return(node) ;
	      if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,(P *)&npt,0)) return(NULL) ;
	    } ;
	 } ;
/*	Go through each of the children */
	cnode = getNodePtr(node->Child) ;
	for(;cnode!=NULL;cnode=getNodePtr(cnode->RightSibling))
	 { rnode = v4tree_FindNodeBreadth(ctx,tmas,cnode,depth,curdepth+1,nodecnt,condpt,lp,ok) ;
	   if (!*ok) return(NULL) ;
	   if (lp == NULL && rnode != NULL) return(rnode) ;
	 } ;
	if (lp != NULL) return(node) ;			/* Have to return a node - might as well be first one */

/*	If here then nothing found - fail */
	v_Msg(ctx,ctx->ErrorMsgAux,"TreeFindNone",condpt,tmas->dimId,node->Id) ; *ok = FALSE ; return(NULL) ;
}

/*	v4im_RPPtoTree - Converts Reverse-Polish-Parse Structure to a tree		*/
/*	Call - tree = v4im_RPPtoTree( ctx , rpp , srchSame )
	  where tnode is top-most node of new tree (NULL if error in ctx->ErrorMsgAux),
		ctx is context,
		rpp is reverse polish structure (struct V4LEX__RevPolParse *),
		srchSame, if TRUE, causes setting of tree's hashValue and searching for existing with same		*/

struct V4Tree__Node *v4im_RPPtoTree(ctx,rpp,srchSame)
  struct V4C__Context *ctx ;
  struct V4LEX__RevPolParse *rpp ;
  LOGICAL srchSame ;
{
  struct V4Tree__Master *tmas ;
  struct V4Tree__Node *nodeTop ;
  struct V4Tree__Node *nodeList[V4LEX_RPP_TokenMax] ;
  struct V4DPI__Point *tpt ;
  enum DictionaryEntries de ;
  INDEX stack[V4LEX_RPP_TokenMax],sx ;
  INDEX i,j ; int operands ;

/*	First allocate nodes for each rpp element (right-most is going to be root of tree) */
	if ((nodeTop = v4tree_MakeTree(ctx,Dim_UTree,0)) == NULL) return(NULL) ;
	nodeList[rpp->Count-1] = nodeTop ;
	tmas = v4tree_TreeMaster(ctx,TREEID(nodeTop->Id)) ;
	for(i=rpp->Count-2;i>=0;i--) { nodeList[i] = v4tree_NewNode(ctx,tmas,TRUE) ; } ;
	sx = 0 ;			/* Initialize stack index */
/*	Now scan left-to-right and create tree (and corresponding linked V4 points) */
	for(i=0;i<rpp->Count;i++)
	 { 
	   switch (rpp->Token[i].Type)
	    {
	      default:
		v_Msg(ctx,ctx->ErrorMsgAux,"RevPolTknType",rpp->Token[i].Type) ; return(NULL) ;
	      case V4LEX_TknType_Punc:		/* Got an operator, link node to appropriate values & push onto stack */
		operands = 2 ;
		switch (rpp->Token[i].OpCode)
		 { 
		   default:			v_Msg(ctx,ctx->ErrorMsgAux,"RevPolOper",opcodeDE[rpp->Token[i].OpCode]) ; return(NULL) ;
		   case V_OpCode_Plus:		de = DE(Plus) ; break ;
		   case V_OpCode_Minus:		de = DE(Minus) ; break ;
		   case V_OpCode_UMinus:	operands = 1 ; de = DE(Minus) ; break ;
		   case V_OpCode_NCStar:
		   case V_OpCode_Star:		de = DE(Star) ; break ;
		   case V_OpCode_Slash:		de = DE(Slash) ; break ;
		   case V_OpCode_SlashSlash:	de = DE(SlashSlash) ; break ;
		   case V_OpCode_Equal:		de = DE(EQ) ; break ;
		   case V_OpCode_Langle:	de = DE(LT) ; break ;
		   case V_OpCode_LangleEqual:	de = DE(LE) ; break ;
		   case V_OpCode_TildeEqual:	de = DE(TildeEqual) ; break ;
		   case V_OpCode_EqualEqual:	de = DE(EqualEqual) ; break ;
		   case V_OpCode_LangleRangle:	de = DE(NE) ; break ;
		   case V_OpCode_Rangle:	de = DE(GT) ; break ;
		   case V_OpCode_RangleEqual:	de = DE(GE) ; break ;
		   case V_OpCode_Tilde:		operands = 1 ; de = DE(Tilde) ; break ;
		   case V_OpCode_Ampersand:	de = DE(Ampersand) ; break ;
		   case V_OpCode_Line:		de = DE(Line) ; break ;
		   case V_OpCode_Percent: 	de = DE(Percent) ; break ;
		 } ;
/*		Create operator (UV4:operation) point & link to node */
		tpt = (P *)v4tree_AllocChunk(ctx,tmas,V4PS_Dict) ;
		dictPNTv(tpt,Dim_UV4,v4im_GetEnumToDictVal(ctx,de,Dim_UV4)) ;
/*		Pop off as many arguments to operator as we need and link as children to this node */
		for(j=0;j<operands;j++)
		 { struct V4Tree__Node *nodeChild ;
		   if (sx-operands+j < 0)			/* Don't have enough operands ? */
		    { v_Msg(ctx,ctx->ErrorMsgAux,"TreeBadFormat",1) ; return(NULL) ; } ;
		   nodeChild = nodeList[stack[sx-operands+j]] ;	/* Want to pull operands off in reverse order, hence convoluted indexing */ 
		   v4tree_SproutNode(ctx,tmas,nodeList[i]->Id,V4TREE_RelPos_Child,nodeChild) ;
		 } ;
		sx -= operands ;
		break ;
	      case V4LEX_TknType_Keyword:
	        tpt = (P *)v4tree_AllocChunk(ctx,tmas,V4PS_Dict) ;
	        dictPNTv(tpt,Dim_NId,v4dpi_DictEntryGet(ctx,0,rpp->Token[i].AlphaVal,NULL,NULL)) ;
	        break ;
	      case V4LEX_TknType_String:
		tpt = (P *)v4tree_AllocChunk(ctx,tmas,V4PS_UCChar(UCstrlen(rpp->Token[i].AlphaVal))) ; 
		uccharPNTv(tpt,rpp->Token[i].AlphaVal) ;
		break ;
	      case V4LEX_TknType_Integer:
		tpt = (P *)v4tree_AllocChunk(ctx,tmas,V4PS_Dict) ; intPNTv(tpt,rpp->Token[i].IntVal) ; break ;
	      case V4LEX_TknType_Float:
		tpt = (P *)v4tree_AllocChunk(ctx,tmas,V4PS_Real) ; dblPNTv(tpt,rpp->Token[i].Floating) ; break ;
	      case V4LEX_TknType_Isct:
	      case V4LEX_TknType_Point:
		tpt = (P *)v4tree_AllocChunk(ctx,tmas,((P *)rpp->Token[i].ValPtr)->Bytes) ; 
		memcpy(tpt,rpp->Token[i].ValPtr,((P *)rpp->Token[i].ValPtr)->Bytes) ;
		break ;
	    } ;
/*	   Link new point (tpt) with tree node & push this node back onto stack */
	   setNodePtr(nodeList[i]->Value,tpt) ; stack[sx++] = i ;
	 } ;
/*	Should we search existing trees to see if we already have one like this? */
	if (srchSame)
	 { TREEID sameTree ;
	   if (!v4tree_SetTreeHash64(ctx,tmas->treeId))
	    { v_Msg(ctx,ctx->ErrorMsgAux,"TreeBadFormat",2) ; return(NULL) ; } ;
	   sameTree = v4tree_FindIdenticalTree(ctx,tmas->treeId) ;
	   if (sameTree != UNUSED)
	    { v4tree_FellTree(ctx,tmas->treeId) ;
	      tmas = v4tree_TreeMaster(ctx,sameTree) ;
	      return(tmas->topNode) ;
	    } ;
	 } ;
	return(nodeTop) ;
}

/*	v4im_DoTree - Handles Tree() module		*/

P *v4im_DoTree(ctx,respnt,argpnts,argcnt,intmodx)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  P *respnt,*argpnts[] ;
  COUNTER argcnt ;
{ P *cpt, spnt, *treept ;
  P *enterpt, *returnpt, *leavept, *bottompt ;
  struct V4L__ListPoint *lp ;
  struct V4DPI__DimInfo *di ;
  struct V4Tree__Master *tmas ;
  struct V4Tree__Node *tnode,*snode ;
  FRAMEID frameId ;
  int ix,ok,tag,cnt,depth, depthdim, max, cx ;
#ifdef NEWQUOTE
/*	This s/b temporary - Return::@xxx, Bottom::@xxx should not be quoted VEH100605 */
#define ONCET(pt) if (pt != NULL) { v_Msg(ctx,NULL,"TreeDupTag",intmodx,ix,tag) ; goto fail ; } ; if(ISQUOTED(cpt)) { pt = UNQUOTEPTR(cpt) ; } else { pt = cpt ; } ; break ;
#else
#define ONCET(pt) if (pt != NULL) { v_Msg(ctx,NULL,"TreeDupTag",intmodx,ix,tag) ; goto fail ; } ; pt = cpt ; break ;
#endif
	tnode = NULL ; tmas = NULL ; treept = NULL ; lp = NULL ; depthdim = UNUSED ;
	enterpt = NULL ; returnpt = NULL ; leavept = NULL ; bottompt = NULL ; frameId = UNUSED ;
	
	for(ok=TRUE,ix=1;ok && ix<=argcnt && (tmas == NULL ? TRUE : tnode != NULL);ix++)
	 { if (!(argpnts[ix]->PntType == V4DPI_PntType_TagVal || argpnts[ix]->Dim == Dim_Dim))
	    { treept = argpnts[ix] ;
	      if (treept->PntType != V4DPI_PntType_Tree || treept->Grouping != V4DPI_Grouping_Single)
	       { v_Msg(ctx,NULL,"TreeInvSpec",intmodx,ix,treept) ; goto fail ; } ;
	      if (treept->Dim != Dim_Dim)
	       { tmas = v4tree_TreeMaster(ctx,TREEID(treept->Value.TreeNodeVal)) ; if (tmas == NULL) { ok = FALSE ; break ; } ;
	         tnode = v4tree_FindNode(ctx,tmas,treept->Value.TreeNodeVal) ; if (tnode == NULL) { ok = FALSE ; break ; } ;
/*		 On the remote chance that we might be iterating through all siblings, cache address of right sibling to save some time later */
		 snode = getNodePtr(tnode->RightSibling) ;
		 if (snode != NULL)
		  { cx = TREEHASH4CACHE(snode->Id) ; tmas->rtc->Cache[cx].Node = snode ; } ;
	       } ;
	      continue ;
	    } ;
	   switch (tag=v4im_CheckPtArgNew(ctx,argpnts[ix],&cpt,&spnt))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:		v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	      case V4IM_Tag_Dim:
		if (ix == 1)			/* If first argument is Dim:xxx then just save for assumed Create? to follow */
		 { treept = argpnts[ix] ; continue ; } ;
		if (tnode == NULL) { v_Msg(ctx,NULL,"TreeNoPos",intmodx,ix,V4IM_Tag_Left) ; goto fail ; } ;
		if (isNodeNull(tnode->Value))
		 { v_Msg(ctx,NULL,"TreeNoVal",intmodx,tnode->Id) ; goto fail ; } ;
		if (getNodeValPt(tnode->Value)->Dim != cpt->Value.TreeNodeVal) { v_Msg(ctx,NULL,"TreeNoVal1",intmodx,tnode->Id,getNodeValPt(tnode->Value)->Dim,cpt->Value.TreeNodeVal) ; goto fail ; } ;
		memcpy(respnt,getNodeValPt(tnode->Value),getNodeValPt(tnode->Value)->Bytes) ;
		return(respnt) ;
	      case V4IM_Tag_Bottom:		ONCET(bottompt)
	      case -V4IM_Tag_Bottom:
		if (tnode == NULL) { v_Msg(ctx,NULL,"TreeNoPos",intmodx,ix,V4IM_Tag_Left) ; goto fail ; } ;
		if (frameId == UNUSED) frameId = v4ctx_FramePush(ctx,NULL) ;
		INITLP(respnt,lp,Dim_List) ; if (!(ok = v4tree_BottomList(ctx,tmas,tnode,lp,NULL))) break ; ENDLP(respnt,lp) ;
		goto success ;
	      case -V4IM_Tag_Child:
		if (tnode == NULL) { v_Msg(ctx,NULL,"TreeNoPos",intmodx,ix,V4IM_Tag_Left) ; goto fail ; } ;
		if (frameId == UNUSED) frameId = v4ctx_FramePush(ctx,NULL) ;
		INITLP(respnt,lp,Dim_List) ; if (!(ok = v4tree_ChildList(ctx,tmas,tnode,lp,NULL))) break ; ENDLP(respnt,lp) ;
		goto success ;
	      case V4IM_Tag_Child:
		if (tnode == NULL) { v_Msg(ctx,NULL,"TreeNoPos",intmodx,ix,V4IM_Tag_Left) ; goto fail ; } ;
		if (frameId == UNUSED) frameId = v4ctx_FramePush(ctx,NULL) ;
#ifdef NEWQUOTE
		if(ISQUOTED(cpt)) { cpt = UNQUOTEPTR(cpt) ; } ;		//VEH100606 - Do this temporarily (should not have to use Child::@xxx)
#endif
		INITLP(respnt,lp,Dim_List) ; if (!(ok = v4tree_ChildList(ctx,tmas,tnode,lp,cpt))) break ; ENDLP(respnt,lp) ;
		goto success ;
	      case V4IM_Tag_Create:
		max = v4im_GetPointInt(&ok,cpt,ctx) ; if (!ok) break ;
		if (treept == NULL ? TRUE : treept->Dim != Dim_Dim) { v_Msg(ctx,NULL,"ModArgNotDim",intmodx,1,treept) ; goto fail ; } ;
		DIMINFO(di,ctx,treept->Value.IntVal) ;
		if (di->PointType != V4DPI_PntType_Int) { v_Msg(ctx,NULL,"ModArgPntType2",intmodx,ix,di->PointType,V4DPI_PntType_Int) ; goto fail ; } ;
		if ((tnode = v4tree_MakeTree(ctx,di->DimId,max)) == NULL) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ; } ;
		tmas = v4tree_TreeMaster(ctx,TREEID(tnode->Id)) ; if (tmas == NULL) { ok = FALSE ; break ; } ;
		break ;
	      case -V4IM_Tag_Create:
		if (treept == NULL ? TRUE : treept->Dim != Dim_Dim) { v_Msg(ctx,NULL,"ModArgNotDim",intmodx,1,treept) ; goto fail ; } ;
		DIMINFO(di,ctx,treept->Value.IntVal) ;
		if (di->PointType != V4DPI_PntType_Int) { v_Msg(ctx,NULL,"ModArgPntType2",intmodx,ix,di->PointType,V4DPI_PntType_Int) ; goto fail ; } ;
		if ((tnode = v4tree_MakeTree(ctx,di->DimId,0)) == NULL) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ; } ;
		tmas = v4tree_TreeMaster(ctx,TREEID(tnode->Id)) ; if (tmas == NULL) { ok = FALSE ; break ; } ;
		break ;
	      case V4IM_Tag_Depth:
		if (cpt->Dim == Dim_Dim)				/* Are we specifying a dimension to track depth of enumeration ? */
		 { depthdim = cpt->Value.IntVal ; break ; } ;
		if (frameId == UNUSED) frameId = v4ctx_FramePush(ctx,NULL) ;
#ifdef NEWQUOTE
		if(ISQUOTED(cpt)) { cpt = UNQUOTEPTR(cpt) ; } ;		//VEH100606 - Do this temporarily (should not have to use Child::@xxx)
#endif
		if ((tnode = v4tree_FindNodeDepth(ctx,tmas,tnode,cpt,lp)) == NULL) { ok = FALSE ; break ; } ;
		if (lp != NULL)
		 { if (lp->Entries == 0) { v_Msg(ctx,ctx->ErrorMsgAux,"TreeFindNone",cpt,tmas->dimId,tnode->Id) ; ok = FALSE ; break ; } ;
		   ENDLP(respnt,lp) ; goto success ;
		 } ;
		break ;
	      case -V4IM_Tag_Elevate:
		tnode = v4tree_ElevateChildren(ctx,tmas,tnode->Id) ;
		if (tnode == NULL) ok = FALSE ;
		break ;
	      case V4IM_Tag_Enter:		ONCET(enterpt)
	      case -V4IM_Tag_Hash64:
		if (tnode == NULL) { v_Msg(ctx,NULL,"TreeNoPos",intmodx,ix,V4IM_Tag_Left) ; goto fail ; } ;
		{ B64INT h64 = 0 ;
		  v4tree_Hash64(ctx,tmas,tnode,&h64,0,&ok) ;
		  fixPNTv(respnt,h64) ;
		}
		return(respnt) ;
	      case V4IM_Tag_Leave:		ONCET(leavept)
	      case -V4IM_Tag_Left:
		cnt = 1 ; goto left_entry ;
	      case V4IM_Tag_Left:
		cnt = v4im_GetPointInt(&ok,cpt,ctx) ; if (!ok) break ;
left_entry:	if (tnode == NULL) { v_Msg(ctx,NULL,"TreeNoPos",intmodx,ix,V4IM_Tag_Left) ; goto fail ; } ;
		snode = tnode ;
		if (cnt == 0) { tnode = v4tree_RelNode(ctx,tmas,tnode->Id,V4TREE_RelPos_FarLeft) ; }
		 else { for(;tnode!=NULL && cnt>0;cnt--)
			 { if ((tnode = v4tree_RelNode(ctx,tmas,tnode->Id,V4TREE_RelPos_Left)) == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"TreeNoRelNode",snode->Id) ; ok = FALSE ; } ; } ;
		      } ;
		continue ;			/* Any other arguments to chain through ? */
	      case -V4IM_Tag_ListOf:
		INITLP(respnt,lp,Dim_List) ; break ;
	      case -V4IM_Tag_Nodes:
		if (tnode == NULL) { v_Msg(ctx,NULL,"TreeNoPos",intmodx,ix,-V4IM_Tag_Nodes) ; goto fail ; } ;
		treePNTv(respnt,tmas->nextNodeId) ; return(respnt) ;
	      case V4IM_Tag_New:
		if (tnode == NULL) { v_Msg(ctx,NULL,"TreeNoPos",intmodx,ix,-V4IM_Tag_New) ; goto fail ; } ;
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:		v_Msg(ctx,NULL,"TreeInvNew",cpt) ; ok = FALSE ; break ;
		   case _Child:		tnode = v4tree_SproutNode(ctx,tmas,tnode->Id,V4TREE_RelPos_Child,NULL) ; if (tnode == NULL) ok = FALSE ; break ;
		   case _Left:		tnode = v4tree_SproutNode(ctx,tmas,tnode->Id,V4TREE_RelPos_Left,NULL) ; if (tnode == NULL) ok = FALSE ; break ;
		   case _Right:		tnode = v4tree_SproutNode(ctx,tmas,tnode->Id,V4TREE_RelPos_Right,NULL) ; if (tnode == NULL) ok = FALSE ; break ;
		   case _Parent:	tnode = v4tree_SproutNode(ctx,tmas,tnode->Id,V4TREE_RelPos_Parent,NULL) ; if (tnode == NULL) ok = FALSE ; break ;
		 } ;
		continue ;			/* Any other arguments to chain through ? */
	      case -V4IM_Tag_Parent:
		snode = tnode ;
		if ((tnode = v4tree_ParentNode(tmas,tnode)) == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"TreeNoRelNode",snode->Id) ; ok = FALSE ; } ;
		continue ;			/* Any other arguments to chain through ? */
	      case -V4IM_Tag_Quoted:
		if (tnode == NULL) { v_Msg(ctx,NULL,"TreeNoPos",intmodx,ix,-V4IM_Tag_Quoted) ; goto fail ; } ;
		if (isNodeNull(tnode->Value))
		 { v_Msg(ctx,NULL,"TreeNoVal",intmodx,tnode->Id) ; goto fail ; } ;
		memcpy(respnt,getNodeValPt(tnode->Value),getNodeValPt(tnode->Value)->Bytes) ;
		return(respnt) ;
	      case V4IM_Tag_Delete:
	      case V4IM_Tag_Remove:
		if (tnode == NULL) { v_Msg(ctx,NULL,"TreeNoPos",intmodx,ix,-V4IM_Tag_Delete) ; goto fail ; } ;
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:		v_Msg(ctx,NULL,"TreeInvNew",cpt) ; ok = FALSE ; break ;
		   case _Child:
		      { COUNTER numChildren = 0 ;
			struct V4Tree__Node *cnode,*rsnode ;
			for(cnode=getNodePtr(tnode->Child);cnode!=NULL;cnode=rsnode)
			 { rsnode=getNodePtr(cnode->RightSibling) ;	/* Remember the next sibling before clearing out below */
			   if (!v4tree_RemoveNode(ctx,tmas,cnode->Id)) { ok = FALSE ; break ; } ;
			   numChildren++ ;
			 } ;
			if (!ok) break ;
			intPNTv(respnt,numChildren) ; return(respnt) ;
		      }
		   case _Node:
			if (!v4tree_RemoveNode(ctx,tmas,tnode->Id)) { ok = FALSE ; break ; } ; return((P *)&Log_True) ;
		 } ;
		break ;
	      case -V4IM_Tag_Delete:
	      case -V4IM_Tag_Remove:
		if (tnode == NULL) { v_Msg(ctx,NULL,"TreeNoPos",intmodx,ix,V4IM_Tag_Delete) ; goto fail ; } ;
		if (!v4tree_RemoveNode(ctx,tmas,tnode->Id)) { ok = FALSE ; break ; } ;
		return((P *)&Log_True) ;
	      case V4IM_Tag_Return:		ONCET(returnpt)
	      case -V4IM_Tag_Right:
		cnt = 1 ; goto right_entry ;
	      case V4IM_Tag_Right:
		cnt = v4im_GetPointInt(&ok,cpt,ctx) ; if (!ok) break ;
right_entry:	if (tnode == NULL) { v_Msg(ctx,NULL,"TreeNoPos",intmodx,ix,V4IM_Tag_Left) ; goto fail ; } ;
		snode = tnode ;
		if (cnt == 0) { tnode = v4tree_RelNode(ctx,tmas,tnode->Id,V4TREE_RelPos_FarRight) ; }
		 else { for(;tnode!=NULL && cnt>0;cnt--)
			 { if((tnode = v4tree_RelNode(ctx,tmas,tnode->Id,V4TREE_RelPos_Right)) == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"TreeNoRelNode",snode->Id) ; ok = FALSE ; } ; } ;
		      } ;
		continue ;			/* Any other arguments to chain through ? */
	      case -V4IM_Tag_Root:
		if (tnode == NULL) { v_Msg(ctx,NULL,"TreeNoPos",intmodx,ix,-V4IM_Tag_Value) ; goto fail ; } ;
		tnode = tmas->topNode ;
		continue ;			/* Any other arguments to chain through ? */
	      case V4IM_Tag_Swap:
		if (tnode == NULL) { v_Msg(ctx,NULL,"TreeNoPos",intmodx,ix,V4IM_Tag_Replace) ; goto fail ; } ;
		if (cpt->Dim != tmas->dimId) { v_Msg(ctx,ctx->ErrorMsgAux,"TreeDifDim",V4IM_Tag_Swap,cpt->Dim,tmas->dimId) ; ok = FALSE ; break ; } ;
		if (!v4tree_SwapNodes(ctx,tmas,tnode->Id,cpt->Value.TreeNodeVal)) { ok = FALSE ; break ; } ;
		return((P *)&Log_True) ;
	      case -V4IM_Tag_Value:
		if (tnode == NULL) { v_Msg(ctx,NULL,"TreeNoPos",intmodx,ix,-V4IM_Tag_Value) ; goto fail ; } ;
		if (isNodeNull(tnode->Value))
		 { v_Msg(ctx,NULL,"TreeNoVal",intmodx,tnode->Id) ; goto fail ; } ;
		return(v4dpi_IsctEval(respnt,getNodeValPt(tnode->Value),ctx,0,NULL,NULL)) ;
	      case V4IM_Tag_Value:
		if (tnode == NULL) { v_Msg(ctx,NULL,"TreeNoPos",intmodx,ix,-V4IM_Tag_Value) ; goto fail ; } ;
		if (isNodeDef(tnode->Value))			/* Can we "re-use" current value position? */
		 { if (getNodeValPt(tnode->Value)->Bytes >= cpt->Bytes)
		    { memcpy(getNodeValPt(tnode->Value),cpt,cpt->Bytes) ; memcpy(respnt,cpt,cpt->Bytes) ;
		      return(respnt) ;
		    } ;
		 } ;
/*		Have to allocate space for this value, take from tree buffer */
		setNodePtr(tnode->Value,(P *)v4tree_AllocChunk(ctx,tmas,cpt->Bytes)) ;
		memcpy(getNodeValPt(tnode->Value),cpt,cpt->Bytes) ; memcpy(respnt,cpt,cpt->Bytes) ;
		return(respnt) ;
	      case V4IM_Tag_Width:
		frameId = v4ctx_FramePush(ctx,NULL) ;		/* Start new context frame */
#ifdef NEWQUOTE
		if(ISQUOTED(cpt)) { cpt = UNQUOTEPTR(cpt) ; } ;		//VEH100606 - Do this temporarily (should not have to use Child::@xxx)
#endif
		for(ok=TRUE,depth=0;;depth++)
		 { cnt = 0 ;
		   if ((snode = v4tree_FindNodeBreadth(ctx,tmas,tnode,depth,0,&cnt,cpt,lp,&ok)) != NULL)
		    { if (lp == NULL) { tnode = snode ; break ; } ; } ;
		   if (!ok) break ;
		   if (cnt == 0) { tnode = snode ;  break ; } ;	/* If cnt = 0 then nothing searched - reached bottom of tree */
		 } ; if (!ok) break ;
		if (lp != NULL)
		 { if (lp->Entries == 0) { v_Msg(ctx,ctx->ErrorMsgAux,"TreeFindNone",cpt,tmas->dimId,tnode->Id) ; ok = FALSE ; break ; } ;
		   ENDLP(respnt,lp) ; goto success ;
		 } ;
		if (tnode == NULL) ok = FALSE ;
		break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ix-1) ; goto fail ; } ;
	if (tmas == NULL && tnode == NULL) { v_Msg(ctx,NULL,"TreeNoAct",intmodx) ; goto fail ; } ;
	if (tnode == NULL) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ; } ;

	if (bottompt != NULL || enterpt != NULL || leavept != NULL || returnpt != NULL)
	 { frameId = v4ctx_FramePush(ctx,NULL) ;
	   if (!v4tree_Enum(ctx,tmas,tnode,enterpt,leavept,returnpt,bottompt,lp,depthdim,0))
	    { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ; } ;
	   if (lp != NULL) { ENDLP(respnt,lp) ; goto success ; } ;
	 } ;

	treePNTv(respnt,tnode->Id) ; respnt->Dim = tmas->dimId ;

success:
	if (frameId != UNUSED)
	 { if (!v4ctx_FramePop(ctx,frameId,NULL)) { frameId = UNUSED ; v_Msg(ctx,NULL,"ModCtxPopFrame",intmodx,frameId) ; goto fail ; } ;
	 } ;
	if (ix+1 <= argcnt)		/* Any other arguments to process ? */
	 { switch (respnt->PntType)
	    { default:		break ;
	      CASEofChar
		if (v4imu_NestToModule(ctx,respnt,argcnt,argpnts,ix+1,intmodx,v4im_DoStr,V4IM_OpCode_Str) == NULL) goto fail ;
		break ;
	      case V4DPI_PntType_List:
		if (v4imu_NestToModule(ctx,respnt,argcnt,argpnts,ix+1,intmodx,v4im_DoList,V4IM_OpCode_List) == NULL) goto fail ;
		break ;
	    } ;
	 } ;
	return(respnt) ;

fail:	REGISTER_ERROR(0) ;
	if (frameId != UNUSED) v4ctx_FramePop(ctx,frameId,NULL) ;
	return(NULL) ;
}


P *v4im_DoMessage(ctx,respnt,argcnt,argpnts,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt,*argpnts[] ;
  int argcnt,intmodx,trace ;
{ P *cpt, *tpt, *ackpt, isctbuf ;
  struct V4DPI__DimInfo *di ;
  struct V4L__ListPoint *lp ;
  struct V4Msg_Listener *vml, *pvml ;
  struct V4Msg_Entry *vme, *pvme, *cvme ;
  int ulid,port,mqldim,seqNum,ttl ; double secWait ; CALENDAR calNow, delCalDT ;
  INDEX ix, tx ; COUNTER cnt ; LOGICAL ok, wantsReply ;
  ETYPE listenAction ;
  UCCHAR Id[V4DPI_DimInfo_DimNameMax+1],host[256], ackBuf[1024], spawnCmdLine[V4DPI_AlphaVal_Max], *b,*b1 ;

	SOCKETINIT
	port = UNUSED ; mqldim = UNUSED ; ZUS(Id) ; seqNum = UNUSED ; cvme = NULL ; ackpt = NULL ; ZUS(spawnCmdLine) ; wantsReply = FALSE ;
	secWait = UNUSED ; ttl = UNUSED ;
	for(ok=TRUE,ix=1;ok&&ix<=argcnt;ix++)
	 { 
	   switch (tx=v4im_CheckPtArgNew(ctx,argpnts[ix],&cpt,&isctbuf))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:		v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
//	      case V4IM_Tag_Ack:		ackpt = cpt ; break ;
//		if (cvme == NULL) { v_Msg(ctx,NULL,"MessageNoCur",intmodx,V4IM_Tag_Message) ; goto fail ; } ;
//		uccharPNTv(respnt,cvme->ackVal) ; return(respnt) ;
//	      case -V4IM_Tag_Ack:
	      case -V4IM_Tag_Calendar:
		if (cvme == NULL) { v_Msg(ctx,NULL,"MessageNoCur",intmodx,V4IM_Tag_Message) ; goto fail ; } ;
		dblPNTv(respnt,cvme->msgRcvCal) ; respnt->PntType = V4DPI_PntType_Calendar ; respnt->Dim = Dim_UCal ;
		return(respnt) ;
	      case V4IM_Tag_Close:
		ulid = v4im_GetPointInt(&ok,cpt,ctx) ; if (!ok) break ;
		for(vml=gpi->vml;(vml== NULL ? FALSE : vml->uId != ulid);pvml=vml,vml=vml->vmlNext) { } ;
		if (vml == NULL) { v_Msg(ctx,NULL,"MessageNoLstnr",intmodx,cpt) ; goto fail ; } ;
/*		Tell listener to terminate & wait */
		vml->status = V4Msg_Status_Terminate ; for(;vml->status!=V4Msg_Status_Done;) { HANGLOOSE(V4IS_HANG_TIME) ; } ;
/*		Release resource used by this listener */
		CLOSE_THREAD(vml->idThread) ;
		GRABMTLOCK(gpi->msgLock) ;
		if (gpi->vml == vml)			/* Remove listener from gpi chain of listeners */
		 { gpi->vml = vml->vmlNext ; } else { pvml->vmlNext = vml->vmlNext ; } ;
		for(cnt=0,vml=gpi->vml;vml!=NULL;cnt++,vml=vml->vmlNext) { } ;
		FREEMTLOCK(gpi->msgLock) ;
		v4mm_FreeChunk(vml) ;
/*		Return with new number of active listeners */
		intPNTv(respnt,cnt) ; return(respnt) ;
	      case V4IM_Tag_Delete:
		seqNum = v4im_GetPointInt(&ok,cpt,ctx) ;
		GRABMTLOCK(gpi->msgLock) ;
		for (vme=gpi->vme;vme!=NULL;pvme=vme,vme=vme->vmeNext)
		 { if (vme->seqNum != seqNum) continue ;
		   if (vme->connectSocket != UNUSED)
		    { v_Msg(ctx,NULL,"MessageDelete",intmodx,V4IM_Tag_Delete,seqNum,V4IM_Tag_Reply) ; goto fail ; } ;
		   if (vme == gpi->vme) { gpi->vme = vme->vmeNext ; }
		    else { pvme->vmeNext = vme->vmeNext ; } ;
		   mqldim = (vme->vml != NULL ? vme->vml->mqlDimId : UNUSED) ;
		   v4mm_FreeChunk(vme) ; break ;
		 } ;
/*		Count number of messages in the queue */
		for(cvme=gpi->vme,cnt=0;cvme!=NULL;cvme=cvme->vmeNext,cnt++) { } ;
		intPNTv(respnt,cnt) ;
/*		Should we update message-queue-length context point ? */
		if (vme != NULL && mqldim != UNUSED)
		 { respnt->Dim = mqldim ; v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,respnt,0,0) ;
		 } ;
		FREEMTLOCK(gpi->msgLock) ;
		if (vme == NULL) { v_Msg(ctx,NULL,"MessageNotFnd",intmodx,argpnts[ix]) ; goto fail ; } ;
/*		Return number of remaining messages in queue */
		intPNTv(respnt,cnt) ; return(respnt) ;
	      case V4IM_Tag_Dim:
		mqldim = cpt->Value.IntVal ; DIMINFO(di,ctx,mqldim) ;
		if (di->PointType != V4DPI_PntType_Int) { v_Msg(ctx,NULL,"ModArgPntType2",intmodx,ix,di->PointType,V4DPI_PntType_Int) ; goto fail ; } ;
		DIMVAL(tpt,ctx,mqldim) ;		/* Is this point in the context ? */
		if (tpt == NULL)			/* No - then inject with integer value 0 */
		 { intPNTv(&isctbuf,0) ; isctbuf.Dim = mqldim ; v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,&isctbuf,0,0) ; } ;
		break ;
	      case V4IM_Tag_Group:
		if (ttl == UNUSED) ttl = VMultiCast_TTL_Subnet ;	/* Set ttl to local subnet (also if ttl != UNUSED then multicasting) */
		goto host_entry ;					/* Parse as if Host: */
	      case V4IM_Tag_Host:
host_entry:
		v4im_GetPointUC(&ok,host,sizeof host,cpt,ctx) ;
/*		Parse host name - may have embedded port number (after ':') */
		b = UCstrrchr(host,':') ;
		if (b != NULL)
		 { *b = UCEOS ; b++ ; port = UCstrtol(b,&b1,10) ;
		   if (*b1 != UCEOS) { v_Msg(ctx,NULL,"MessagePort",intmodx,b,cpt) ; goto fail ; } ;
		 } ;
		break ;
	      case V4IM_Tag_Id:			v4im_GetPointUC(&ok,Id,sizeof Id,cpt,ctx) ; break ;
	      case -V4IM_Tag_Id:
		if (cvme == NULL) { v_Msg(ctx,NULL,"MessageNoCur",intmodx,V4IM_Tag_Message) ; goto fail ; } ;
		INITLP(respnt,lp,Dim_List)
		if (cvme->msgOK)		/* Is this an error, if so then map as UV4:error-message */
		 { uccharPNTv(&isctbuf,cvme->sndrId) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&isctbuf,0) ;
		   uccharPNTv(&isctbuf,cvme->lstnrId) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&isctbuf,0) ;
		 } ;
		ENDLP(respnt,lp) ; return(respnt) ;
	      case V4IM_Tag_Listen:
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:		v_Msg(ctx,NULL,"ModTagValue",intmodx,ix,V4IM_Tag_Listen,cpt) ; goto fail ; 
		   case _Context:	listenAction = V4MsgL_Action_Ctx ; break ;
		   case _Queue:		listenAction = V4MsgL_Action_Queue ; break ;
		 }
		goto listen_entry ;
	      case -V4IM_Tag_Listen:
	        listenAction = V4MsgL_Action_Queue ;
listen_entry:
		vml = (struct V4Msg_Listener *)v4mm_AllocChunk(sizeof *vml,FALSE) ;
		vml->ctx = ctx ; vml->status = V4Msg_Status_Starting ; vml->ttl = ttl ; UCstrncpy(vml->Host,host,UCsizeof(vml->Host)) ;
		vml->port = (port == UNUSED ? VML_Default_Port : port) ; vml->msgCount = 0 ; vml->mqlDimId = mqldim ; UCstrcpy(vml->lstnrId,Id) ;
		vml->uId = ++gpi->luuId ; vml->vmlAction = listenAction ;
//		if (ackpt == NULL) { ZPH(&vml->ackPnt) ; }
//		 else { if (ISQUOTED(ackpt)) { UNQUOTECOPY(&vml->ackPnt,ackpt) ; } else { memcpy(&vml->ackPnt,ackpt,ackpt->Bytes) ; } ; } ;

#ifdef CREATE_THREAD
	      { UCCHAR *d ; int ip1 = UCstrtol(vml->Host,&d,10) ;
	        CREATE_THREAD(((ip1 >= 224 && ip1 <= 239) ? v4im_MessageListenMC : v4im_MessageListen),vml->idThread,vml,NULL,ctx->ErrorMsg)
	      }
#else
	      v_Msg(ctx,NULL,"ModTagNYI",intmodx,V4IM_Tag_Listen) ; goto fail ;
#endif


#ifdef WINNT
		if (gpi->vml == NULL)
		 { gpi->hMsgWait = CreateEvent(NULL,TRUE,FALSE,"MessageRcvEvent") ; 
		 } ;
#endif

/*		Wait for listener to crank up, check for errors and then continue */
		for(;vml->status==V4Msg_Status_Starting;)
		 { HANGLOOSE(V4IS_HANG_TIME) ; } ;
		if (vml->status != V4Msg_Status_Listening)
		 {
		   CLOSE_THREAD(vml->idThread) ;
		   v4mm_FreeChunk(vml) ; goto fail ;
		 } ;

		GRABMTLOCK(gpi->msgLock) ;
		vml->vmlNext = gpi->vml ; gpi->vml = vml ;	/* Link to any other listeners in pi->vml queue */
		FREEMTLOCK(gpi->msgLock) ;
/*		Return unique listener id */
		intPNTv(respnt,vml->uId) ; return(respnt) ;
	      case V4IM_Tag_ListOf:
		ulid = v4im_GetPointInt(&ok,cpt,ctx) ; if (!ok) break ;
		INITLP(respnt,lp,Dim_List) ;
		setCALisNOW(calNow) ;
		for(cnt=0,vme=gpi->vme;vme!=NULL;vme=vme->vmeNext)
		 { intPNTv(&isctbuf,vme->seqNum) ;
		   if (vme->delCalDT > calNow || vme->uId != ulid) continue ;
		   cnt++ ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&isctbuf,0) ;
		 } ;
		if (cnt == 0) { v_Msg(ctx,NULL,"MessageQueMT",intmodx) ; goto fail ; } ;
		ENDLP(respnt,lp) ;
		return(respnt) ;
	      case -V4IM_Tag_ListOf:
		INITLP(respnt,lp,Dim_List) ;
		calNow = VCal_MJDOffset + TIMEOFFSETDAY +(double)(time(NULL))/(double)VCAL_SecsInDay ;
		for(cnt=0,vme=gpi->vme;vme!=NULL;vme=vme->vmeNext)
		 { intPNTv(&isctbuf,vme->seqNum) ;
		   if (vme->delCalDT > calNow) continue ;
		   cnt++ ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&isctbuf,0) ;
		 } ;
		if (cnt == 0) { v_Msg(ctx,NULL,"MessageQueMT",intmodx) ; goto fail ; } ;
		ENDLP(respnt,lp) ;
		return(respnt) ;
	      case -V4IM_Tag_Message:
		if (cvme == NULL) { v_Msg(ctx,NULL,"MessageNoCur",intmodx,V4IM_Tag_Message) ; goto fail ; } ;
		if (!cvme->msgOK)		/* Is this an error, if so then map as UV4:error-message */
		 { uccharPNTv(respnt,cvme->msgVal) ; respnt->Dim = Dim_UV4 ; return(respnt) ;
		 } ;
	        if (v4im_MessageToPoint(ctx,respnt,cvme,intmodx) == NULL)
		 { v_Msg(ctx,NULL,"MessagePoint2",intmodx,cvme->msgVal) ; goto fail ; } ;
		return(respnt) ;
	      case V4IM_Tag_Next:
		if (cpt->PntType == V4DPI_PntType_Int)
		 { ulid = v4im_GetPointInt(&ok,cpt,ctx) ;
		   for(vml=gpi->vml;vml!=NULL;vml=vml->vmlNext)
		    { if (vml->uId == ulid) break ; } ;
		 } else
		 { v4im_GetPointUC(&ok,Id,UCsizeof(Id),cpt,ctx) ;
		   for(vml=gpi->vml;vml!=NULL;vml=vml->vmlNext)
		    { if (UCstrcmpIC(Id,vml->lstnrId) == 0) break ; } ;
		 } ;
		if (vml == NULL)
		 { v_Msg(ctx,NULL,"MessageNoLstnr",intmodx,cpt) ; goto fail ; } ;
	      case -V4IM_Tag_Next:
	        if (tx == -V4IM_Tag_Next)
		 { vml = NULL ; } ;
		if (secWait < 0) secWait = 0 ;
		calNow = -1 ;
#ifdef WINNT
/*		Use the WaitForSingleObject call to sleep until we get a message. v4im_MessageListen() issues SetEvent() to wake up this thread */
		for(;;)
		 {
		   for(vme=gpi->vme;vme!=NULL;vme=vme->vmeNext)		/* Before we sleep - see if we have anything */
		    { if (vml != NULL && vme->vml != vml) continue ;
		      if (calNow < 0) calNow = VCal_MJDOffset + TIMEOFFSETDAY +(double)(time(NULL))/(double)VCAL_SecsInDay ;
		      if (vme->delCalDT <= calNow)
		       { intPNTv(respnt,gpi->vme->seqNum) ; return(respnt) ; } ;
		    } ;
		   switch (WaitForSingleObject(gpi->hMsgWait,(secWait == 0 ? INFINITE : (secWait * 1000))))
		    { default:
		      case WAIT_TIMEOUT:	v_Msg(ctx,NULL,"MessageQueMT",intmodx) ; goto fail ;
		      case WAIT_OBJECT_0:	ResetEvent(gpi->hMsgWait) ; break ;
		    } ;
		 } ;
#else
/*		This should be fixed up to use interrupts, waiting for a second reduces turn-around time */
		for(;secWait>=0;secWait--)
		 { for(vme=gpi->vme;vme!=NULL;vme=vme->vmeNext)
		    { if (calNow < 0) calNow = VCal_MJDOffset + TIMEOFFSETDAY +(double)(time(NULL))/(double)VCAL_SecsInDay ;
		      if (vme->delCalDT <= calNow)
		       { intPNTv(respnt,gpi->vme->seqNum) ; return(respnt) ; } ;
		    } ;
		   HANGLOOSE(1000) ; calNow = -1 ;
		 } ;
		v_Msg(ctx,NULL,"MessageQueMT",intmodx) ; goto fail ;
#endif
	      case -V4IM_Tag_Peer:
		if (cvme == NULL) { v_Msg(ctx,NULL,"MessageNoCur",intmodx,V4IM_Tag_Message) ; goto fail ; } ;
		uccharPNTv(respnt,cvme->sndrIPAddressPort) ; return(respnt) ;
	      case V4IM_Tag_Port:		port = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case -V4IM_Tag_Reply:		wantsReply = TRUE ; break ;
	      case V4IM_Tag_Reply:
		if (cpt->PntType == V4DPI_PntType_Logical)
		 { wantsReply = v4im_GetPointLog(&ok,cpt,ctx) ; break ; } ;


		if (cvme == NULL ? TRUE : !cvme->msgOK) { v_Msg(ctx,NULL,"MessageNoCur",intmodx,V4IM_Tag_Message) ; goto fail ; } ;
		if (cvme->connectSocket == UNUSED)
		 { v_Msg(ctx,NULL,"MessageNoReply",intmodx,V4IM_Tag_Reply) ; goto fail ; } ;
		{ BYTE *oPtr ; LENMAX res,sent,chunk,lenToSend ; INDEX tries ;


		  if (cpt->PntType == V4DPI_PntType_MemPtr)
		   { UCCHAR *p ; LENMAX len ;
		     memcpy(&p,&cpt->Value.MemPtr,sizeof p) ; len = UCstrlen(p) ;
		     oPtr = v4mm_AllocChunk(len*3,FALSE) ;		/* Allocate UTF-8 buffer */
		     UCUTF16toUTF8(oPtr,len*3,p,len) ;			/* Convert UCCHAR to UTF-8 */
		     strcat(oPtr,MESSAGE_EOMS) ;			/* Terminate with EOM */
		     lenToSend = strlen(oPtr) ;				/* Get length of entire mess */
		     v4mm_FreeChunk(p) ;				/* Deallocate all temp memory */
		   } else
		   { oPtr = ASCTBUF1 ;
		     v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,cpt,ctx) ; if (!ok) break ;
		     UCUTF16toUTF8(oPtr,sizeof ASCTBUF1,UCTBUF1,UCstrlen(UCTBUF1)) ;
		     strcat(oPtr,MESSAGE_EOMS) ;
		     lenToSend = strlen(oPtr) ;
		   } ;
//		  send(cvme->connectSocket,ASCTBUF1,strlen(ASCTBUF1),0) ;
		  chunk = (lenToSend < 4096 ? lenToSend : 4096) ;
#define SEND_WAIT_SECONDS 3
		  for(sent=0,res=1,tries=0;tries<100 && res>0&&sent<lenToSend;)
		   { 
		     fd_set sset ; struct timeval tev ; LENMAX res ;
		     FD_ZERO(&sset) ; FD_SET(cvme->connectSocket,&sset) ;
		     tev.tv_sec = SEND_WAIT_SECONDS ; tev.tv_usec = 0 ;
		     res = select(FD_SETSIZE,NULL,&sset,NULL,&tev) ;	/* Wait for ok to send */
		     if (res <= 0)
		      { v_Msg(ctx,NULL,"SocketTimeOut",intmodx,SEND_WAIT_SECONDS,sent) ; goto fail ;
		      } ;


		     res = send(cvme->connectSocket,&oPtr[sent],chunk,0) ; if (res > 0) sent+=res ; 
		     if (res <= 0 && NETERROR == EWOULDBLOCK) { HANGLOOSE(50) ; tries++ ; res = 1 ; continue ; } ;
		     if (chunk > lenToSend-sent) chunk = lenToSend-sent ; tries = 0 ;
		   } ;
		  if (oPtr != ASCTBUF1) v4mm_FreeChunk(oPtr) ;
		}


		SOCKETCLOSE(cvme->connectSocket) ; cvme->connectSocket = UNUSED ;
		logPNTv(respnt,TRUE) ;
		break ;


	      case V4IM_Tag_Send:
		delCalDT = (secWait > 0 ? VCal_MJDOffset + TIMEOFFSETDAY +(double)(secWait + time(NULL))/(double)VCAL_SecsInDay : 0) ;
		if (!v4im_MessageSend(ctx,intmodx,host,(port == UNUSED ? VML_Default_Port : port),ttl,cpt,Id,wantsReply,ackBuf,sizeof ackBuf,spawnCmdLine,delCalDT,trace)) goto fail ;
		uccharPNTv(respnt,ackBuf) ; return(respnt) ;
	      case V4IM_Tag_Sequence:
		seqNum = v4im_GetPointInt(&ok,cpt,ctx) ;
		for (cvme=gpi->vme;cvme!=NULL;cvme=cvme->vmeNext)
		 { if (cvme->seqNum == seqNum) break ; } ;
		if (cvme == NULL) { v_Msg(ctx,NULL,"MessageNotFnd",intmodx,argpnts[ix]) ; goto fail ; } ;
		break ;
	      case V4IM_Tag_Spawn:
		v4im_GetPointUC(&ok,spawnCmdLine,sizeof spawnCmdLine,cpt,ctx) ;
		break ;
	      case V4IM_Tag_TTL:
		if (cpt->PntType == V4DPI_PntType_Int) { ttl = v4im_GetPointInt(&ok,cpt,ctx) ;  break ; } ;
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:		v_Msg(ctx,NULL,"ModTagValue",intmodx,ix,V4IM_Tag_TTL,cpt) ; goto fail ; 
		   case _Continent:	ttl = VMultiCast_TTL_Continent ; break ;
		   case _Host:		ttl = VMultiCast_TTL_Host ; break ;
		   case _Region:	ttl = VMultiCast_TTL_Region ; break ;
		   case _Site:		ttl = VMultiCast_TTL_Site ; break ;
		   case _Subnet:	ttl = VMultiCast_TTL_Subnet ; break ;
		   case _Unrestricted:	ttl = VMultiCast_TTL_Unrestricted ; break ;
		 } ;
		break ;
	      case V4IM_Tag_Wait:
		secWait = v4im_GetPointDur(&ok,cpt,ctx) ; if (!ok) break ;
		break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ix-1) ; goto fail ; } ;
	return(respnt) ;
	
fail:	REGISTER_ERROR(0) ;
	return(NULL) ;
}

LOGICAL msgSendQueueDidInit=FALSE, msgSendQueueBusy=FALSE ;
#define V4_MESSAGE_SEND_QUEUE_BUF_MAX 1023
static UCCHAR msgSendQueueBuf[V4_MESSAGE_SEND_QUEUE_BUF_MAX+1] ;
#ifdef WINNT
  HANDLE msgQueueFlushThread ;		/* Thread of maxIdle, maxCPU, maxIsct watchdog */
#endif
#if defined LINUX486 || defined RASPPI
  pthread_t msgQueueFlushThread ;
#endif

/*	v4im_MessageSendFlushQueue - Flushes out message queue periodically (run as separate thread)	*/
/*	Call: v4im_MessageSendFlushQueue()								*/

void v4im_MessageSendFlushQueue()
{
  P *sendPt=NULL ; INTMODX intmodx=0 ; int ttl=UNUSED ; UCCHAR msgId[32],spawnCmdLine[32] ; LOGICAL wantsReply=FALSE ; CALENDAR delCalDT=0 ; VTRACE trace=0 ;
  UCCHAR msBufQ[V4_MESSAGE_SEND_QUEUE_BUF_MAX*2], msBufSend[V4_MESSAGE_SEND_QUEUE_BUF_MAX*2] ;
  LOGICAL ok ;

	for(;;)
	 { HANGLOOSE(50) ;
	   if (UCempty(msgSendQueueBuf)) continue ;
	   if (msgSendQueueBusy) continue ;
	   ZUS(msgId) ; ZUS(spawnCmdLine) ;
	   v_StringLit(msgSendQueueBuf,msBufQ,V4_MESSAGE_SEND_QUEUE_BUF_MAX*2,UClit('"'),UClit('\\')) ;
	   ZUS(msgSendQueueBuf) ;
	   UCsprintf(msBufSend,UCsizeof(msBufSend),UClit("{\"handshake\":{\"sesKey\":\"%s\"},\"forWeb\":%s}"),gpi->sesKeyDebug,msBufQ) ;
	   ok = v4im_MessageSend(gpi->ctx,intmodx,gpi->hostDebug,gpi->portDebug,ttl,sendPt,msgId,wantsReply,msBufSend,UCsizeof(msBufSend),spawnCmdLine,delCalDT,trace) ;
	 } ;
}

/*	v4im_MessageSendQueue - Used to bundle up multiple text messages into a single V4 message	*/
/*	Call: same as v4im_MessageSend()								*/

LOGICAL v4im_MessageSendQueue(ctx,intmodx,host,port,ttl,sendPt,msgId,wantsReply,ackBuf,ackBufsize,spawnCmdLine,delCalDT,trace)
  struct V4C__Context *ctx ;
  int intmodx,port,ttl,trace,ackBufsize ;
  UCCHAR *host, *msgId, *ackBuf, *spawnCmdLine ;
  LOGICAL wantsReply ;
  double delCalDT ;
  struct V4DPI__Point *sendPt ;
{ 
  LOGICAL ok ;

	msgSendQueueBusy = TRUE ;
/*	Only queue up messages if a text message (i.e. sendPt is NULL)	*/
	if (sendPt != NULL)
	 { ok = v4im_MessageSend(ctx,intmodx,host,port,ttl,sendPt,msgId,wantsReply,ackBuf,ackBufsize,spawnCmdLine,delCalDT,trace) ;
	   msgSendQueueBusy = FALSE ;
	   return(ok) ;
	 } ;
	if (!msgSendQueueDidInit)
	 { ZUS(msgSendQueueBuf) ;
	   msgSendQueueDidInit = TRUE ;
#ifdef CREATE_THREAD
	   CREATE_THREAD(v4im_MessageSendFlushQueue,msgQueueFlushThread,NULL,NULL,gpi->ctx->ErrorMsg)
#endif
	 } ;
/*	Time to flush queued buffer? */
//printf("msgSendQueueBuf=%d, ackBuf=%d, max=%d, buf: %s\n",UCstrlen(msgSendQueueBuf),UCstrlen(ackBuf),V4_MESSAGE_SEND_QUEUE_BUF_MAX,UCretASC(ackBuf)) ;
	if (UCstrlen(msgSendQueueBuf) + UCstrlen(ackBuf) >= V4_MESSAGE_SEND_QUEUE_BUF_MAX)
	 { UCCHAR msBufQ[V4_MESSAGE_SEND_QUEUE_BUF_MAX*2], msBufSend[V4_MESSAGE_SEND_QUEUE_BUF_MAX*2] ;
	   v_StringLit(msgSendQueueBuf,msBufQ,V4_MESSAGE_SEND_QUEUE_BUF_MAX*2,UClit('"'),UClit('\\')) ;
	   ZUS(msgSendQueueBuf) ; if (UCstrlen(ackBuf) < V4_MESSAGE_SEND_QUEUE_BUF_MAX) { UCstrcat(msgSendQueueBuf,ackBuf) ; } ;
	   UCsprintf(msBufSend,UCsizeof(msBufSend),UClit("{\"handshake\":{\"sesKey\":\"%s\"},\"forWeb\":%s}"),gpi->sesKeyDebug,msBufQ) ;
//printf("Message: %s\n",UCretASC(msBufSend)) ;
	   ok = v4im_MessageSend(ctx,intmodx,host,port,ttl,sendPt,msgId,wantsReply,msBufSend,UCsizeof(msBufSend),spawnCmdLine,delCalDT,trace) ;
	   msgSendQueueBusy = FALSE ;
	   return(ok) ;
	 } ;
/*	Is current message bigger then queue size? */
	if (UCstrlen(ackBuf) >= V4_MESSAGE_SEND_QUEUE_BUF_MAX)
	 { UCCHAR msBufQ[V4_MESSAGE_SEND_QUEUE_BUF_MAX*2], msBufSend[V4_MESSAGE_SEND_QUEUE_BUF_MAX*2] ;
	   v_StringLit(msgSendQueueBuf,msBufQ,V4_MESSAGE_SEND_QUEUE_BUF_MAX*2,UClit('"'),UClit('\\')) ;
	   UCsprintf(msBufSend,UCsizeof(msBufSend),UClit("{\"handshake\":{\"sesKey\":\"%s\"},\"forWeb\":%s}"),gpi->sesKeyDebug,msBufQ) ;
	   ok = v4im_MessageSend(ctx,intmodx,host,port,ttl,sendPt,msgId,wantsReply,msBufSend,UCsizeof(msBufSend),spawnCmdLine,delCalDT,trace) ;
	   msgSendQueueBusy = FALSE ;
	   return(ok) ;
	 } ;

	UCstrcat(msgSendQueueBuf,ackBuf) ;
//printf("Queued message, len=%d\n",UCstrlen(msgSendQueueBuf)) ;
	msgSendQueueBusy = FALSE ;
	return(TRUE) ;

fail:	msgSendQueueBusy = FALSE ;
	return(FALSE) ;
}

/*	v4im_MessageSend - Sends a Message() off to address:port somewhere										 */
/*	Call: v4im_MessageSend( ctx, intmodx, host, port, ttl, sendPt, msgId, wantsReply, ackBuf, ackBufsize, spawnCmdLine, delCalDT, trace )
	  where ctx is context
		intmodx is V4 module making the call,
		host is an IP address or domanin name,
		port is the socket port number,
		ttl is time-to-live parameter or UNUSED,
		sendPt is the V4 point to be sent or NULL, then ackBuf holds the UCCHAR text to send,
		msgId is message Id or BLANK,
		wantsReply is true/false if reply is expected,
		ackbuf is point to buffer to get received ack and/or the text message to be sent,
		ackBufsize is size of buffer to receive ack,
		spawnCmdLine is command line to be spawned at target,
		delCatDT is CALENDAR date-time when message is to be delivered (NOT BEFORE),
		trace is trace flag															*/

LOGICAL v4im_MessageSend(ctx,intmodx,host,port,ttl,sendPt,msgId,wantsReply,ackBuf,ackBufsize,spawnCmdLine,delCalDT,trace)
  struct V4C__Context *ctx ;
  int intmodx,port,ttl,trace,ackBufsize ;
  UCCHAR *host, *msgId, *ackBuf, *spawnCmdLine ;
  LOGICAL wantsReply ;
  double delCalDT ;
  struct V4DPI__Point *sendPt ;
{ 
  struct V4DPI__DimInfo *di ;
  fd_set sset ; struct timeval tev ;
  struct sockaddr_in sin ;
  struct hostent *hp ;
  int Socket ; UCCHAR *b ;
  int i,len,chunk,sent,res,tries ;
  BYTE msgBuf[V4DPI_AlphaVal_Max * 3], *mPtr ;
  UCCHAR ucmsgBuf[V4DPI_AlphaVal_Max * 2] ;

	Socket = UNUSED ;

/*	Format message: id <nl> wantsReply(0/1) <nl> delCalDT <nl> dimension <nl> value  <control-Z> */
/*	NOTE: if this changes then you MUST also change listeners & v4SrvProcStreamLine() in v4wwwFEServer.c */

	UCstrcpy(ucmsgBuf,msgId) ; UCstrcat(ucmsgBuf,MESSAGE_UCEODS) ;
	if (wantsReply) { UCstrcat(ucmsgBuf,UClit("1") MESSAGE_UCEODS) ; } else { UCstrcat(ucmsgBuf,UClit("0") MESSAGE_UCEODS) ; } 
	UCsprintf(&ucmsgBuf[UCstrlen(ucmsgBuf)],40,V4DPI_PntFormat_Real,delCalDT) ; UCstrcat(ucmsgBuf,MESSAGE_UCEODS) ;
	if (sendPt == NULL)
	 { UCstrcat(ucmsgBuf,UClit("Alpha") MESSAGE_UCEODS) ; UCstrcat(ucmsgBuf,ackBuf) ;
	 } else
	 { DIMINFO(di,ctx,sendPt->Dim) ; UCstrcat(ucmsgBuf,di->DimName) ;  UCstrcat(ucmsgBuf,MESSAGE_UCEODS) ;
/*	   If point to send is form: `( . . . ) then create list of EVALUATED contents */
	   if (sendPt->PntType == V4DPI_PntType_List && sendPt->ForceEval)
	    { P spnt,vpnt,*tpt ; struct V4L__ListPoint *lp = v4im_VerifyList(NULL,ctx,sendPt,0) ;
	      UCstrcat(ucmsgBuf,UClit("`(")) ;
	      for(i=1;;i++)
	       { if (v4l_ListPoint_Value(ctx,lp,i,&spnt) <= 0) break ;
		 tpt = v4dpi_IsctEval(&vpnt,&spnt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		 if (tpt == NULL) { v_Msg(ctx,NULL,"MessageEvalFail",intmodx,i,&vpnt) ; goto fail ; } ;
		 v4dpi_PointToStringML(&ucmsgBuf[UCstrlen(ucmsgBuf)],tpt,ctx,V4DPI_FormatOpt_ShowDim|V4DPI_FormatOpt_AlphaQuote,V4DPI_AlphaVal_Max) ;
		 UCstrcat(ucmsgBuf,UClit(" ")) ;
	       } ;
	      UCstrcat(ucmsgBuf,UClit(")")) ;
	    } else
	    { v4dpi_PointToStringML(&ucmsgBuf[UCstrlen(ucmsgBuf)],sendPt,ctx,(sendPt->PntType == V4DPI_PntType_List ? V4DPI_FormatOpt_ShowDim|V4DPI_FormatOpt_AlphaQuote : V4DPI_FormatOpt_Echo),V4DPI_AlphaVal_Max) ;
	    } ;
	 } ;
	UCstrcat(ucmsgBuf,MESSAGE_UCEOMS) ;
/*	Now convert to UTF-8 */
	UCUTF16toUTF8(msgBuf,sizeof msgBuf,ucmsgBuf,UCstrlen(ucmsgBuf)) ;

/*	If doing multicast then handle here */
	if (ttl != UNUSED)
	 {
/*	   Create what looks like an ordinary UDP socket */
	   if ((Socket = socket(AF_INET,SOCK_DGRAM,0)) < 0) { v_Msg(ctx,NULL,"SocketINETErr",intmodx,errno) ; Socket = UNUSED ; goto fail ; } ;
	   memset(&sin,0,sizeof(sin)) ; sin.sin_family = AF_INET ;
	   sin.sin_addr.s_addr = inet_addr(UCretASC(host)) ; sin.sin_port = htons(port);
/*	   Just sendto() our destination! */
	  if (sendto(Socket,msgBuf,strlen(msgBuf),0,(struct sockaddr *) &sin, sizeof(sin)) < 0)
	   { v_Msg(ctx,NULL,"SocketSendErr",intmodx,NETERROR) ; goto fail ; } ;
	  SOCKETCLOSE(Socket) ;
	  return(TRUE) ;
	 } ;	

	if ((Socket = socket(AF_INET,SOCK_STREAM,0)) < 0) { v_Msg(ctx,NULL,"SocketINETErr",intmodx,errno) ; Socket = UNUSED ; goto fail ; } ;
	memset(&sin,0,sizeof sin) ; sin.sin_family = AF_INET ;
	sin.sin_port = htons((u_short)port) ;
/*	Do we have vanilla host name or something more exotic like www.dot.com */
	if (UCstrchr(host,'.') == NULL) { hp = gethostbyname(UCretASC(host)) ; }
	 else { if ((b = v_URLAddressLookup(host,ctx->ErrorMsgAux)) == NULL)
		 { v_Msg(ctx,NULL,"SocketHost",intmodx,V4IM_Tag_Host,host) ; goto fail ; } ;
		UCstrcpy(host,b) ; hp = NULL ;
	      } ;
	if (hp != NULL) { memcpy(&sin.sin_addr,hp->h_addr,hp->h_length) ; }
	 else { sin.sin_addr.s_addr = inet_addr(UCretASC(host)) ;
		if (sin.sin_addr.s_addr == -1) { v_Msg(ctx,NULL,"SocketGHbyN",intmodx,errno,host) ; goto fail ; } ;
	      } ;
/*	If connection fails AND we have a command line, then spawn it, wait and retry */
	for(tries=0;tries<20;tries++)
	 { if ((i=connect(Socket,(struct sockaddr *)&sin,sizeof sin)) >= 0) break ;
	   if (tries > 0) { HANGLOOSE(100) ; continue ; } ;
	   if (UCempty(spawnCmdLine)) { tries = 999999 ; continue ; } ;
//	   if (!v_SpawnProcess(NULL,spawnCmdLine,V_SPAWNPROC_NoWait,ctx->ErrorMsgAux,NULL,NULL,UNUSED)) goto fail ;
	   { struct V__SpawnArgs sa ; memset(&sa,0,sizeof sa) ; sa.argBuf = spawnCmdLine ; sa.waitFlags = V_SPAWNPROC_NoWait ; sa.errBuf = ctx->ErrorMsgAux ; sa.fileId = UNUSED ;
 	     if (!v_SpawnProcess(&sa)) goto fail ;
	   }
	   HANGLOOSE(250) ;
	 } 
	if (tries > 20)
	 { v_Msg(ctx,NULL,"SocketConErr",intmodx,V4IM_Tag_Host,host,V4IM_Tag_Port,port,NETERROR) ; goto fail ; } ;
	i = 1 ;
	if (NETIOCTL(Socket,FIONBIO,&i) != 0) { v_Msg(ctx,NULL,"SocketNoBlock",intmodx) ; goto fail ; } ;

	len = strlen(msgBuf) ;
	chunk = (len < 4096 ? len : 4096) ;		/* Send message in pieces */
	for(sent=0,res=1,tries=0;tries<100 && res>0&&sent<len;)
	 { res = send(Socket,msgBuf+sent,chunk,0) ; if (res > 0) sent+=res ; 
	   if (res <= 0 && NETERROR == EWOULDBLOCK) { HANGLOOSE(100) ; tries++ ; res = 1 ; continue ; } ;
	   if (chunk > len-sent) chunk = len-sent ; tries = 0 ;
	 } ;
	if (res <= 0 || tries >= 100) { v_Msg(ctx,NULL,"SocketSendErr",intmodx,NETERROR) ; goto fail ; } ;
/*	Now read (any) ack back from listener */
	for(mPtr=msgBuf;wantsReply && mPtr<&msgBuf[sizeof msgBuf];mPtr++)
	 { 
	   FD_ZERO(&sset) ; FD_SET(Socket,&sset) ;
	   tev.tv_sec = (mPtr == msgBuf ? 5 : 1) ; tev.tv_usec = 0 ;	/* Wait a little longer for first byte */
	   res = select(FD_SETSIZE,&sset,NULL,NULL,&tev) ;	/* Wait for something to happen */
	   if (res <= 0) break ;
	   res = recv(Socket,mPtr,1,0) ;
	   if (*mPtr == MESSAGE_EOM) break ;		/* Got control-z - break */
	   if (res <= 0) break ;
	 } ; *mPtr = '\0' ;
/*	Convert ack to UTF-16 and we are done */
	UCUTF8toUTF16(ackBuf,ackBufsize,msgBuf,mPtr-msgBuf) ;
	
	SOCKETCLOSE(Socket) ;
	return(TRUE) ;
fail:
	if (Socket != UNUSED) { SOCKETCLOSE(Socket) ; } ;
	return(FALSE) ;
}


void v4im_MessageListen(vml)
  struct V4Msg_Listener *vml ;
{
  struct V4C__Context *ctx ;
  struct hostent *hp; char szHost[256]; // should be big enough. 
  struct sockaddr_in sin,peer ;
  fd_set sset ;			/* Socket set */
  struct timeval tev ;
  struct V4DPI__Point epnt ;
  struct V4DPI__LittlePoint lpnt ;
  struct V4Msg_Entry vme, *tvme, *nvme ;
  int i,res,len ;
  int ConnectSocket ;
#define VML_msgBuf_Max 2048
  BYTE msgBuf[V4DPI_AlphaVal_Max * 3], *mPtr ;
  UCCHAR ucmsgBuf[V4DPI_AlphaVal_Max * 2], *ubp, *ubp1 ;
  
	ctx = vml->ctx ;			/* Define ctx to make various macros happy */

	if ((vml->socket = socket(AF_INET, SOCK_STREAM,0)) < 0)
	 { v_Msg(ctx,ucmsgBuf,"SocketINETErr",V4IM_OpCode_Message,errno) ; goto listen_error ; } ;
	i = 1 ;
	if (NETIOCTL(vml->socket,FIONBIO,&i) != 0)
	 { v_Msg(ctx,ucmsgBuf,"SocketNoBlock",V4IM_OpCode_Message,errno) ; goto listen_error ; } ;
	memset(&sin,0,sizeof sin) ; sin.sin_family = AF_INET ; sin.sin_port = htons(vml->port) ;
	if (UCempty(vml->Host))
	 { gethostname(szHost,sizeof(szHost)) ;		/* Get host name */
	   hp = gethostbyname(szHost) ;
	   if (hp != NULL) { memcpy(&sin.sin_addr,hp->h_addr,hp->h_length) ; }
	    else { sin.sin_addr.s_addr = inet_addr(szHost) ;
		   if (sin.sin_addr.s_addr == -1)
		    { v_Msg(ctx,ucmsgBuf,"SocketGHbyN",V4IM_OpCode_Message,errno,ASCretUC(szHost)) ; goto listen_error ; } ;
		 } ;	
	 } else
	 { struct in_addr localIPAddress4 ;
	   localIPAddress4.s_addr = inet_addr(UCretASC(vml->Host)) ;
	   if (localIPAddress4.s_addr == INADDR_NONE)
	    { v_Msg(ctx,ucmsgBuf,"SocketGHbyN",V4IM_OpCode_Message,errno,vml->Host) ; goto listen_error ; } ;
	  sin.sin_addr.s_addr = localIPAddress4.s_addr ;
	 } ;
	if (bind(vml->socket,(struct sockaddr *)&sin,sizeof sin) < 0)
	 { v_Msg(ctx,ucmsgBuf,"SocketBindErr",V4IM_OpCode_Message,UClit("127.0.0.1"),vml->port,errno) ; goto listen_error ; } ;
	if (listen(vml->socket,10) < 0)
	 { v_Msg(ctx,ucmsgBuf,"SocketListenErr",V4IM_OpCode_Message,errno) ; goto listen_error ; } ;

	vml->status = V4Msg_Status_Listening ;
	for(;;)
	 { 
	   for(;;)
	    {
	      FD_ZERO(&sset) ; FD_SET(vml->socket,&sset) ;
	      tev.tv_sec = 5 ; tev.tv_usec = 0 ;	/* Wait a little longer for first byte */
	      i = select(1,&sset,NULL,NULL,&tev) ;	/* Wait for something! */
	      if (vml->status == V4Msg_Status_Terminate)
	       { SOCKETCLOSE(vml->socket) ; vml->status = V4Msg_Status_Done ; return ; } ;
	      if (i > 0) break ;
	    } ;
	   i = sizeof sin ;
	   if ((ConnectSocket = accept(vml->socket,(struct sockaddr *)&sin,&i)) < 0)
	    { v_Msg(ctx,ucmsgBuf,"SocketAccptErr",V4IM_OpCode_Message,errno) ; goto listen_error ; } ;
	   setCALisNOW(vme.msgRcvCal) ;
/*	   Read in message */
	   for(mPtr=msgBuf;mPtr<&msgBuf[sizeof msgBuf];mPtr++)
	    { 
	      if (TRUE)
	       { FD_ZERO(&sset) ; FD_SET(ConnectSocket,&sset) ;
	         tev.tv_sec = (mPtr == msgBuf ? 20 : 1) ; tev.tv_usec = 0 ;	/* Wait a little longer for first byte */
	         res = select(FD_SETSIZE,&sset,NULL,NULL,&tev) ;	/* Wait for something to happen */
	         if (res <= 0)
	          { v_Msg(ctx,ucmsgBuf,"SocketTimeOut",V4IM_OpCode_Message,tev.tv_sec,msgBuf-mPtr) ; goto listen_error ; } ;
	       } ;
	      res = recv(ConnectSocket,mPtr,1,0) ;
	      if (*mPtr == MESSAGE_EOM) break ;		/* Got control-z - break */
	      if (res <= 0)
	       { /* Got error or premature end-of-transmission */
		 v_Msg(ctx,ucmsgBuf,"SocketRcvErr",V4IM_OpCode_Message,errno) ; goto listen_error ;
	       } ;
	    } ;
/*	   Get sender's IP address & port (Note: port number for this connection only) */
	   i = sizeof peer ;
	   if (getpeername(ConnectSocket,(struct sockaddr *)&peer,&i) == 0)
	    { v_Msg(ctx,vme.sndrIPAddressPort,"@%1s",inet_ntoa(peer.sin_addr)) ;
	    } else { ZUS(vme.sndrIPAddressPort) ; } ;

/*	   Message in UTF-8, format of message:  id <nl> wantsReply(0/1) <nl> delCalDT <nl> dimension <nl> value  <control-Z> */
	   *mPtr = '\0' ;			/* Terminate with end-of-string */
	   UCUTF8toUTF16(ucmsgBuf,UCsizeof(ucmsgBuf),msgBuf,mPtr-msgBuf) ;
/*	   Now parse message into vme.xxx */
	   ubp = UCstrchr(ucmsgBuf,MESSAGE_EODC) ;
	   if (ubp == NULL) { v_Msg(ctx,ucmsgBuf,"MessageInvMsg",V4IM_OpCode_Message) ; goto listen_error ; } ;
	   *ubp = UCEOS ;
	   if (UCstrlen(ucmsgBuf) >= UCsizeof(vme.sndrId))
	    { v_Msg(ctx,ucmsgBuf,"MessageCmpSize",V4IM_OpCode_Message,1,UCstrlen(ucmsgBuf),UCsizeof(vme.sndrId)) ; goto listen_error ; } ;
	   UCstrcpy(vme.sndrId,ucmsgBuf) ; ubp1 = ubp + 1 ;
	   vme.connectSocket = (*ubp1 == UClit('1') ? ConnectSocket : UNUSED) ;
	   ubp1 =  UCstrchr(ubp1,MESSAGE_EODC) ; ubp1++ ;
	   vme.delCalDT = UCstrtod(ubp1,&ubp) ; ubp1 = ubp + 1 ;
	   if (*ubp != MESSAGE_UCEODC) { v_Msg(ctx,ucmsgBuf,"MessageInvMsg",V4IM_OpCode_Message) ; goto listen_error ; } ; 
	   ubp = UCstrchr(ubp1,MESSAGE_EODC) ;
	   if (ubp == NULL) { v_Msg(ctx,ucmsgBuf,"MessageInvMsg",V4IM_OpCode_Message) ; goto listen_error ; } ;
	   *ubp = UCEOS ;
	   if (UCstrlen(ubp1) >= UCsizeof(vme.msgDimName))
	    { v_Msg(ctx,ucmsgBuf,"MessageCmpSize",V4IM_OpCode_Message,3,UCstrlen(ubp1),UCsizeof(vme.msgDimName)) ; goto listen_error ; } ;
	   UCstrcpy(vme.msgDimName,ubp1) ; ubp1 = ubp + 1 ;
	   if (UCstrlen(ubp1) >= UCsizeof(vme.msgVal))
	    { v_Msg(ctx,ucmsgBuf,"MessageCmpSize",V4IM_OpCode_Message,4,UCstrlen(ubp1),UCsizeof(vme.msgVal)) ; goto listen_error ; } ;
	   UCstrcpy(vme.msgVal,ubp1) ;
/*	   Now append this message to main message queue */
	   vme.msgOK = TRUE ;
append_vme:
	   switch (vml->vmlAction)
	    {
	      case V4MsgL_Action_Queue:
		UCstrcpy(vme.lstnrId,vml->lstnrId) ; vme.uId = vml->uId ;
		tvme = (struct V4Msg_Entry *)v4mm_AllocChunk(sizeof *tvme,FALSE) ; *tvme = vme ;
		tvme->seqNum = (++gpi->luseqNum) ;		/* Assign unique sequence number fo this message */
		tvme->vml = vml ;
		if (!GRABMTLOCK(gpi->msgLock)) continue ;	/* Got major problems if this fails, skip message altogether */
		if (gpi->vme == NULL) { gpi->vme = tvme ; len = 1 ; }
		 else { for(len=2,nvme=gpi->vme;;nvme=nvme->vmeNext,len++)	/* Put at end of message queue */
			 { if (nvme->vmeNext != NULL) continue ;
			   nvme->vmeNext = tvme ; tvme->vmeNext = NULL ; break ;
			 } ;
		      } ;
/*		If we should update context point with new message length then do it */
		if (vml->mqlDimId != UNUSED)
		 { intPNTv((P *)&lpnt,len) ; lpnt.Dim = vml->mqlDimId ; v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,(P *)&lpnt,0,0) ; } ;
		FREEMTLOCK(gpi->msgLock) ;
		break ;
	      case V4MsgL_Action_Ctx:
		if (v4im_MessageToPoint(ctx,&epnt,&vme,V4IM_OpCode_Message) == NULL)
		 { v_Msg(ctx,UCTBUF1,"MessagePoint",vme.msgDimName,vme.msgVal) ; vout_UCText(VOUT_Warn,0,UCTBUF1) ;
		   dictPNTv(&epnt,Dim_UV4,v4im_GetEnumToDictVal(ctx,DE(Fail),Dim_UV4)) ;
		 } ;
		if (!GRABMTLOCK(gpi->msgLock)) continue ;	/* Got major problems if this fails, skip message altogether */
/*		If point is list and has ForceEval then put each component of list into context */
		if (epnt.PntType == V4DPI_PntType_List && epnt.ForceEval)
		 { P spnt ; INDEX i ; struct V4L__ListPoint *lp = v4im_VerifyList(NULL,ctx,&epnt,0) ;
		   for(i=1;;i++)
		    { if (v4l_ListPoint_Value(ctx,lp,i,&spnt) <= 0) break ;
		      if (!v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,&spnt,0,0)) { v_Msg(ctx,ucmsgBuf,"CtxAddFail",0) ; goto listen_error ; } ;
//v_Msg(ctx,NULL,"@*Message() -> Ctx(%1P)\n",&epnt) ; vout_UCText(VOUT_Trace,0,ctx->ErrorMsg) ;
		    } ;
		 } else
		 { v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,&epnt,0,0) ;
//v_Msg(ctx,NULL,"@*Message() -> Ctx(%1P)\n",&epnt) ; vout_UCText(VOUT_Trace,0,ctx->ErrorMsg) ;
		 } ;
		FREEMTLOCK(gpi->msgLock) ;
		break ;
	    } ;
///*	   If we need to send Ack then do it now */
//	   if (vml->ackPnt.Bytes != 0)
//	    { 
//	      tpt = v4dpi_IsctEval(&epnt,&vml->ackPnt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
//	      if (tpt != NULL) { v4dpi_PointToStringML(UCTBUF1,tpt,ctx,V4DPI_FormatOpt_Echo,V4DPI_AlphaVal_Max) ; }
//	       else { v_Msg(ctx,UCTBUF1,"@Eval of %1P failed",&vml->ackPnt) ; } ;
//	      if (UCstrlen(UCTBUF1) > V4DPI_AlphaVal_Max / 2) UCTBUF1[V4DPI_AlphaVal_Max / 2] = UCEOS ;
//	    } else { ZUS(UCTBUF1) ; } ;
//	   UCUTF16toUTF8(ackBuf,sizeof ackBuf,UCTBUF1,UCstrlen(UCTBUF1)) ;
//	   strcat(ackBuf,MESSAGE_EOMS) ;
//	   res = send(ConnectSocket,ackBuf,strlen(ackBuf),0) ;
/*	   Close the socket now if a reply is not wanted */
	   if (vme.connectSocket == UNUSED) SOCKETCLOSE(ConnectSocket) ;
#ifdef WINNT
	   SetEvent(gpi->hMsgWait) ;				/* Signal main thread that we got a message */
#endif
	   continue ;
listen_error:
	   if (vml->status == V4Msg_Status_Starting)		/* If still in startup then quit with error */
	    { vml->status = V4Msg_Status_Error ; UCstrcpy(ctx->ErrorMsg,ucmsgBuf) ; return ; } ;
	   vme.msgOK = FALSE ; UCstrcpy(vme.msgVal,ucmsgBuf) ;
	   goto append_vme ;
	 } ;


	return ;
}
void v4im_MessageListenMC(vml)
  struct V4Msg_Listener *vml ;
{
  struct V4C__Context *ctx ;
  struct sockaddr_in sin ;
  struct ip_mreq mreq;
  struct V4DPI__LittlePoint lpnt ;
  struct V4Msg_Entry vme, *tvme, *nvme ;
  int i,len ;
#define VML_msgBuf_Max 2048
  BYTE msgBuf[V4DPI_AlphaVal_Max * 3], *mPtr ;
  UCCHAR ucmsgBuf[V4DPI_AlphaVal_Max * 2], *ubp, *ubp1 ;
  
	ctx = vml->ctx ;			/* Define ctx to make various macros happy */

	if ((vml->socket = socket(AF_INET,SOCK_DGRAM,0)) < 0)
	 { v_Msg(ctx,ucmsgBuf,"SocketINETErr",V4IM_OpCode_Message,errno) ; goto listen_error ; } ;
	i = 1 ;
	if (setsockopt(vml->socket,SOL_SOCKET,SO_REUSEADDR,(char *)&i,sizeof(i)) < 0)
	 { v_Msg(ctx,ucmsgBuf,"SocketINETErr",V4IM_OpCode_Message,errno) ; goto listen_error ; } ;

#ifdef WINNT
	vml->port = 0 ;			/* Use of port 0 when using INADDR_ANY recommended on windows systems (http://msdn.microsoft.com/en-us/library/windows/desktop/ms737550(v=vs.85).aspx */
#endif
	memset(&sin,0,sizeof sin) ; sin.sin_family = AF_INET ; sin.sin_port = htons(vml->port) ;
	sin.sin_addr.s_addr = htonl(INADDR_ANY) ;

	if (bind(vml->socket,(struct sockaddr *)&sin,sizeof sin) < 0)
	 { v_Msg(ctx,ucmsgBuf,"SocketBindErr",V4IM_OpCode_Message,UClit("multicast INADDR_ANY"),vml->port,NETERROR) ; goto listen_error ; } ;

/*	Request that the kernel join a multicast group */
	mreq.imr_multiaddr.s_addr = inet_addr(UCretASC(vml->Host)) ; mreq.imr_interface.s_addr = htonl(INADDR_ANY) ;
	if (setsockopt(vml->socket,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *)&mreq,sizeof(mreq)) < 0)
	 { v_Msg(ctx,ucmsgBuf,"SocketINETErr",V4IM_OpCode_Message,errno) ; goto listen_error ; } ;

	vml->status = V4Msg_Status_Listening ;
	for(;;)
	 { 

	   len=sizeof(sin);
	   if ((len=recvfrom(vml->socket,msgBuf,sizeof msgBuf,0,(struct sockaddr *) &sin,&len)) < 0)
	    { v_Msg(ctx,ucmsgBuf,"SocketRcvErr",V4IM_OpCode_Message,errno) ; goto listen_error ; } ;
	   mPtr = &msgBuf[len-1] ; if (*mPtr != MESSAGE_EOM) *mPtr++ ;	/* If last byte is not MIDAS_EOM then skip over so we don't UCEOS it below */
	   setCALisNOW(vme.msgRcvCal) ;
	   UCstrcpy(vme.sndrIPAddressPort,UClit("multicast")) ;
/*	   Message in UTF-8, format of message:  id <nl> wantsReply <nl> delCalDT <nl> dimension <nl> value  <control-Z> */
	   *mPtr = '\0' ;			/* Terminate with end-of-string */
	   UCUTF8toUTF16(ucmsgBuf,UCsizeof(ucmsgBuf),msgBuf,mPtr-msgBuf) ;
/*	   Now parse message into vme.xxx */
	   ubp = UCstrchr(ucmsgBuf,MESSAGE_EODC) ;
	   if (ubp == NULL) { v_Msg(ctx,ucmsgBuf,"MessageInvMsg",V4IM_OpCode_Message) ; goto listen_error ; } ;
	   *ubp = UCEOS ;
	   if (UCstrlen(ucmsgBuf) >= UCsizeof(vme.sndrId))
	    { v_Msg(ctx,ucmsgBuf,"MessageCmpSize",V4IM_OpCode_Message,1,UCstrlen(ucmsgBuf),UCsizeof(vme.sndrId)) ; goto listen_error ; } ;
	   UCstrcpy(vme.sndrId,ucmsgBuf) ; ubp1 = ubp + 1 ;
//	   vme.connectSocket = (*ubp1 == UClit('1') ? ConnectSocket : UNUSED) ;
	   ubp =  UCstrchr(ubp1,MESSAGE_EODC) ; ubp++ ;
	   vme.delCalDT = UCstrtod(ubp1,&ubp) ; ubp1 = ubp + 1 ;
	   if (*ubp != MESSAGE_UCEODC) { v_Msg(ctx,ucmsgBuf,"MessageInvMsg",V4IM_OpCode_Message) ; goto listen_error ; } ; 
	   ubp = UCstrchr(ubp1,MESSAGE_EODC) ;
	   if (ubp == NULL) { v_Msg(ctx,ucmsgBuf,"MessageInvMsg",V4IM_OpCode_Message) ; goto listen_error ; } ;
	   *ubp = UCEOS ;
	   if (UCstrlen(ubp1) >= UCsizeof(vme.msgDimName))
	    { v_Msg(ctx,ucmsgBuf,"MessageCmpSize",V4IM_OpCode_Message,3,UCstrlen(ubp1),UCsizeof(vme.msgDimName)) ; goto listen_error ; } ;
	   UCstrcpy(vme.msgDimName,ubp1) ; ubp1 = ubp + 1 ;
	   if (UCstrlen(ubp1) >= UCsizeof(vme.msgVal))
	    { v_Msg(ctx,ucmsgBuf,"MessageCmpSize",V4IM_OpCode_Message,4,UCstrlen(ubp1),UCsizeof(vme.msgVal)) ; goto listen_error ; } ;
	   UCstrcpy(vme.msgVal,ubp1) ;
/*	   Now append this message to main message queue */
	   vme.msgOK = TRUE ;
append_vme:
	   UCstrcpy(vme.lstnrId,vml->lstnrId) ;
	   tvme = (struct V4Msg_Entry *)v4mm_AllocChunk(sizeof *tvme,FALSE) ; *tvme = vme ;
	   tvme->seqNum = (++gpi->luseqNum) ;		/* Assign unique sequence number fo this message */
	   tvme->vml = vml ;
	   if (!GRABMTLOCK(gpi->msgLock)) continue ;	/* Got major problems if this fails, skip message altogether */
	   if (gpi->vme == NULL) { gpi->vme = tvme ; len = 1 ; }
	    else { for(len=2,nvme=gpi->vme;;nvme=nvme->vmeNext,len++)	/* Put at end of message queue */
		    { if (nvme->vmeNext != NULL) continue ;
		      nvme->vmeNext = tvme ; tvme->vmeNext = NULL ; break ;
		    } ;
		 } ;
/*	   If we should update context point with new message length then do it */
	   if (vml->mqlDimId != UNUSED)
	    { intPNTv((P *)&lpnt,len) ; lpnt.Dim = vml->mqlDimId ; v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,(P *)&lpnt,0,0) ; } ;
	   FREEMTLOCK(gpi->msgLock) ;
#ifdef WINNT
	   SetEvent(gpi->hMsgWait) ;				/* Signal main thread that we got a message */
#endif
	   continue ;
listen_error:
	   if (vml->status == V4Msg_Status_Starting)		/* If still in startup then quit with error */
	    { vml->status = V4Msg_Status_Error ; UCstrcpy(ctx->ErrorMsg,ucmsgBuf) ; return ; } ;
	   vme.msgOK = FALSE ; UCstrcpy(vme.msgVal,ucmsgBuf) ;
	   goto append_vme ;
	 } ;


	return ;
}

/*	v4im_MessageToPoint - Converts message (dim + value as string) to V4 Point */
struct V4DPI__Point *v4im_MessageToPoint(ctx,respnt,vme,intmodx)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *respnt ;
  struct V4Msg_Entry *vme ;
  INTMODX intmodx ;
{
  struct V4DPI__DimInfo *di ;
  struct V4LEX__TknCtrlBlk *tcb = NULL ;
  COUNTER mlen ; UCCHAR *b ; LOGICAL forceEval ;

	ZPH(respnt) ;
	if ((respnt->Dim = v4dpi_DimGet(ctx,vme->msgDimName,DIMREF_IRT)) == 0) respnt->Dim = Dim_Alpha ;
	DIMINFO(di,ctx,respnt->Dim) ; if (di == NULL) { DIMINFO(di,ctx,respnt->Dim) ; } ;
	mlen = UCstrlen(vme->msgVal) ;
	switch (di->PointType)
	 { default:
		INITTCB ;
/*		Check to see if any embedded spaces or punctuation, if so enclose value in quotes */
		if (UCstrpbrk(vme->msgVal,UClit(" ,\"'|~`!@#$%")) != NULL)
		 { UCCHAR *s=vme->msgVal, *d=UCTBUF1 ;
		   *(d++) = UClit('"') ;				/* Enclose in quotes */
		   for(;*s!=UCEOS;s++)
		    { if (*s == UClit('"')) *(d++) = UClit('\\') ;	/* Preface any quote with backslash */
		      *(d++) = *s ;
		    } ; *(d++) = UClit('"') ; *(d++) = UCEOS ;
		   v4lex_NestInput(tcb,NULL,UCTBUF1,V4LEX_InpMode_String) ;
		 } else
		 { v4lex_NestInput(tcb,NULL,vme->msgVal,V4LEX_InpMode_String) ;
		 } ;
		if (!v4dpi_PointAccept(respnt,di,tcb,ctx,V4DPI_PointParse_RetFalse))
		 { goto fail ;
//		   respnt->Dim = Dim_Alpha ; DIMINFO(di,ctx,respnt->Dim) ;			/* Force into Dim:Alpha */
//		   INITTCB ; v4lex_NestInput(tcb,NULL,vme->msgVal,V4LEX_InpMode_String) ;
//		   v4dpi_PointAccept(respnt,di,tcb,ctx,V4DPI_PointParse_RetFalse) ;
		 } ;
		v4lex_FreeTCB(tcb) ;
		respnt->Dim = di->DimId ; break ;
	   case V4DPI_PntType_List:
		INITTCB ;
		b = vme->msgVal ;
		if (*b == UClit('`')) { forceEval = TRUE ; b++ ; } else { forceEval = FALSE ; } ;
		v4lex_NestInput(tcb,NULL,b,V4LEX_InpMode_String) ;
		if (!v4dpi_PointAccept(respnt,di,tcb,ctx,V4DPI_PointParse_RetFalse))
		 { goto fail ;
//		   respnt->Dim = Dim_Alpha ; DIMINFO(di,ctx,respnt->Dim) ;			/* Force into Dim:Alpha */
//		   v4lex_FreeTCB(tcb) ; tcb = NULL ; INITTCB ; v4lex_NestInput(tcb,NULL,vme->msgVal,V4LEX_InpMode_String) ;
//		   v4dpi_PointAccept(respnt,di,tcb,ctx,V4DPI_PointParse_RetFalse) ;
		 } ;
		v4lex_FreeTCB(tcb) ;
		respnt->Dim = di->DimId ; respnt->ForceEval = forceEval ; break ;
	   case V4DPI_PntType_BigText:
		if (!v4dpi_SaveBigTextPoint2(ctx,vme->msgVal,respnt,di->DimId,TRUE))
		 goto fail ;
//		 { v_Msg(ctx,NULL,"MessageVal",intmodx,di->DimId) ; return(NULL) ; } ;
		break ;
	   CASEofCharmU
		if (mlen >= V4DPI_AlphaVal_Max - 3)
		 { if (!v4dpi_SaveBigTextPoint2(ctx,vme->msgVal,respnt,di->DimId,TRUE))
		    goto fail ;
//		    { v_Msg(ctx,NULL,"MessageVal",intmodx,di->DimId) ; return(NULL) ; } ;
		 } else
		 { ZPH(respnt) ; respnt->Dim = di->DimId ; respnt->PntType = di->PointType ;
		   UCUTF16toUTF8(&respnt->Value.AlphaVal[1],V4DPI_AlphaVal_Max-2,vme->msgVal,mlen) ;
		   CHARPNTBYTES1(respnt) ;
		 } ;
		break ;
	   case V4DPI_PntType_UCChar:
		if (mlen >= V4DPI_UCVAL_MaxSafe)
		 { if (!v4dpi_SaveBigTextPoint2(ctx,vme->msgVal,respnt,di->DimId,TRUE))
		    goto fail ;
//		    { v_Msg(ctx,NULL,"MessageVal",intmodx,di->DimId) ; return(NULL) ; } ;
		 } else { uccharPNTv(respnt,vme->msgVal) ; } ;
		respnt->Dim = di->DimId ; break ;
	 } ;
	return(respnt) ;

fail:	if (tcb != NULL) v4lex_FreeTCB(tcb) ;
	return(NULL) ;
}

/*	v4im_MessageNextMessageText - Returns text of 'next' Message() (waits if necessary)	*/
/*	Call: text = v4im_MessageNextMessageText()
	  where text is text of next message							*/

UCCHAR *v4im_MessageNextMessageText()
{
  struct V4C__Context *ctx ;
  struct V4Msg_Entry *vme, *pvme, *cvme ;
  int secWait, calNow ;
  static UCCHAR mbuf[V4DPI_AlphaVal_Max] ;

	ctx = gpi->ctx ;				/* Do this so macros below work */
	secWait = 0 ; calNow = -1 ;
/*	Get next message entry. If none then wait until we got one */
#ifdef WINNT
/*	Use the WaitForSingleObject call to sleep until we get a message. v4im_MessageListen() issues SetEvent() to wake up this thread */
	for(;;)
	 {
	   for(cvme=gpi->vme;cvme!=NULL;cvme=cvme->vmeNext)		/* Before we sleep - see if we have anything */
	    { if (cvme->vml->uId != gpi->lIdDebug) continue ;
	      if (calNow < 0) calNow = VCal_MJDOffset + TIMEOFFSETDAY +(double)(time(NULL))/(double)VCAL_SecsInDay ;
	      if (cvme->delCalDT <= calNow) break ;
	    } ;
	   if (cvme != NULL) break ;
	   switch (WaitForSingleObject(gpi->hMsgWait,(secWait == 0 ? INFINITE : (secWait * 1000))))
	    { default:
	      case WAIT_OBJECT_0:	ResetEvent(gpi->hMsgWait) ; break ;
	    } ;
	 } ;
#else
/*	This should be fixed up to use interrupts, waiting for a second reduces turn-around time */
	for(;secWait>=0;secWait--)
	 { for(cvme=gpi->vme;cvme!=NULL;cvme=cvme->vmeNext)
	    { if (calNow < 0) calNow = VCal_MJDOffset + TIMEOFFSETDAY +(double)(time(NULL))/(double)VCAL_SecsInDay ;
	      if (vme->delCalDT <= calNow) break ;
	    } ;
	   if (cvme != NULL) break ;
	   HANGLOOSE(1000) ; calNow = -1 ;
	 } ;
//	v_Msg(ctx,NULL,"MessageQueMT",intmodx) ; goto fail ;
#endif
/*	Save the message text value */	
	UCstrcpy(mbuf,cvme->msgVal) ;
/*	Release the message resources */
	GRABMTLOCK(gpi->msgLock) ;
	for (vme=gpi->vme;vme!=NULL;pvme=vme,vme=vme->vmeNext)
	 { if (vme != cvme) continue ;
	   if (vme == gpi->vme) { gpi->vme = vme->vmeNext ; }
	    else { pvme->vmeNext = vme->vmeNext ; } ;
	   v4mm_FreeChunk(vme) ; break ;
	 } ;
	FREEMTLOCK(gpi->msgLock) ;

	return(mbuf) ;
}

/*	V 4   L O C K I N G   P R I M I T I V E			*/

struct V4PL__ProcLock *v4pl_linkToSharedSegment(ctx,ssName,lockMax,procMax)
  struct V4C__Context *ctx ;
  UCCHAR *ssName ;
  LENMAX lockMax,procMax ;
{
  struct V4PL__ProcLock *vpl ;
  struct V4PL__ProcInfo *vpi ;
  LENMAX ssBytes ;
#ifdef WINNT
  HANDLE hMapObject ; LOGICAL newmap ;
  HANDLE hMutex ;
#endif

	ssBytes = sizeof *vpl + (lockMax * sizeof vpl->lock[0]) + sizeof *vpi + (procMax * sizeof vpi->info[0]) ;

#ifdef WINNT
/*	Grab a system mutex to prevent race state in creating & initializing global segment */
	hMutex = CreateMutex(NULL,FALSE,"Global\\V4LockSegment") ;
	if (hMutex == NULL)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"SystemCallFail","CreateMutex",UClit("V4LockSegment"),GetLastError()) ; goto fail ; } ;
	if (WaitForSingleObject(hMutex,INFINITE) == WAIT_FAILED)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"SystemCallFail","WaitForSingleObject",UClit(""),GetLastError()) ; goto fail ; } ;

	hMapObject = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,ssBytes,UCretASC(ssName)) ;
	if (hMapObject == NULL)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"LockMapObj",ssName,lockMax,procMax,"CreateFileMapping",GetLastError()) ; goto fail ;
	 } ;
	newmap = (GetLastError() != ERROR_ALREADY_EXISTS) ;
	vpl = (struct V4PL__ProcLock *)MapViewOfFile(hMapObject,FILE_MAP_WRITE,0,0,0) ;
	if (vpl == NULL)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"",ssName,lockMax,procMax,"MapViewOfFile",GetLastError()) ; goto fail ;
	 } ;
	if (!newmap)
	 { INDEX lx ;
/*	   Let's do quick check to make sure global segment not corrupt */
	   if (vpl->procMax < procMax || vpl->lockMax < lockMax)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"LockMaxNotSame",ssName,lockMax,procMax,vpl->lockMax,vpl->procMax) ; goto fail ; } ;
	   if (vpl->lockMax != V4PL_maxNumberOfLocks || vpl->procMax != V4PL_maxNumberOfProcs ||
		vpl->lockCnt < 0 || vpl->lockCnt > vpl->lockMax || vpl->procCnt < 0 || vpl->procCnt > vpl->procMax) { newmap = TRUE ; goto end_check ; } ;
	   for(lx=0;lx<vpl->lockCnt;lx++)
	    { if (vpl->lock[lx].ownerCnt < 0 || vpl->lock[lx].ownerCnt > V4PL_maxOwnersOfLock) { newmap = TRUE ; goto end_check ; } ;
	      if (vpl->lock[lx].waitCnt < 0 || vpl->lock[lx].waitCnt > V4PL_maxWaitOnLock) { newmap = TRUE ; goto end_check ; } ;
	    } ;
	   if (vpl->offsetInfo != sizeof *vpl + (lockMax * sizeof vpl->lock[0])) { newmap = TRUE ; goto end_check ; } ;
/*	   Everything looks OK */
	   v4pl_removeDefunctPids(ctx,vpl,UNUSED) ;	/* Clear out any old processes */
end_check:
	   if (newmap) printf("**********SEGMENT SCREWED, RE-INITIALIZING IT*********************\n") ;
	 } ;
/*	If new segment then initialize it, if not then make sure we are in synch with lockMax & procMax */
	if (newmap)
	 { 
	   FREEMTLOCK(vpl->lockOnLockTbl) ;
	   vpl->bytesSegment = ssBytes ; vpl->lockMax = lockMax ; vpl->procMax = procMax ;
	   vpl->lockCnt = 0 ; vpl->procCnt = 0 ;
	   vpl->offsetInfo = sizeof *vpl + (lockMax * sizeof vpl->lock[0]) ;
	   vpl->luProcId = V4PL_procIdBase ; vpl->luLockId = 0 ;
	   vpi = (struct V4PL__ProcInfo *)((char *)vpl + vpl->offsetInfo) ;
	 } ;
	ReleaseMutex(hMutex) ; CloseHandle(hMutex) ;
	return(vpl) ;

fail:
	if (hMutex != NULL) { ReleaseMutex(hMutex) ; CloseHandle(hMutex) ; } ;
	return(NULL) ;
#endif
}

UNIQUEID v4pl_RegisterProc(ctx,vpl,procName)
  struct V4C__Context *ctx ;
  struct V4PL__ProcLock *vpl ;
  UCCHAR *procName ;
{ 
  struct V4PL__ProcInfo *vpi ;
  INDEX procIndex ;
  UNIQUEID procId ;

	vpi = (struct V4PL__ProcInfo *)((char *)vpl + vpl->offsetInfo) ;
	for(;!GETSPINLOCK(&vpl->lockOnLockTbl);) {} ;
	if (vpl->procCnt >= vpl->procMax)
	 { FREEMTLOCK(vpl->lockOnLockTbl) ;
	   v_Msg(ctx,ctx->ErrorMsgAux,"LockUserMax",vpl->lockOnLockTbl) ;
	   return(UNUSED) ;
	 } ;
	procId = ++vpl->luProcId ;
	procIndex = vpl->procCnt ;
	vpi->info[procIndex].procPid = CURRENTPID ;
	vpi->info[procIndex].procId = procId ;
	if (UCstrlen(procName) >= V4PL_procNameMax) { UCstrncpy(vpi->info[procIndex].procName,procName,V4PL_procNameMax-1) ; vpi->info[procIndex].procName[V4PL_procNameMax-1] = UCEOS ; }
	 else { UCstrcpy(vpi->info[procIndex].procName,procName) ; } ;
	vpl->procCnt++ ;
	FREEMTLOCK(vpl->lockOnLockTbl) ;
	return(procId) ;
}

/*	v4pl_removeDefunctPids - Removes defunct Pid from lock table (or all defunct pids)	*/
/*	Call: ok = v4pl_removeDefunctPids( ctx , vpl , procId )
	  where ok = TRUE if done, FALSE if problems,
		ctx is context,
		vpl is pointer to global segment,
		procId is unique process/thread Id to be removed, UNUSED for all defunct pids		*/
		
LOGICAL v4pl_removeDefunctPids(ctx,vpl,procId)
  struct V4C__Context *ctx ;
  struct V4PL__ProcLock *vpl ;
  UNIQUEID procId ;
{
  struct V4PL__ProcInfo *vpi ;
  INDEX lx,ox,px,wx,i ;
  
	vpi = (struct V4PL__ProcInfo *)((char *)vpl + vpl->offsetInfo) ;
	if (procId == UNUSED)				/* If UNUSED then remove all defunct pids */
	 { for(px=0;px<vpl->procCnt;px++)
	    { 
	      if (PROCSTILLALIVE(vpi->info[px].procPid)) continue ;
	      procId = vpi->info[px].procId ;
	      v4pl_removeDefunctPids(ctx,vpl,procId) ;
	      px-- ;
	    } ;
	   return(TRUE) ;
	 } ;
/*	Here to remove specified pid */
	for(;!GETSPINLOCK(&vpl->lockOnLockTbl);) {} ;
	for(px=0;px<vpl->procCnt;px++)			/* First remove from procInfo table */
	 { if (procId != vpi->info[px].procId) continue ;
	   for(i=px+1;i<vpl->procCnt;i++) { vpi->info[i-1] = vpi->info[i] ; } ;
	   vpl->procCnt-- ; px-- ;
	 } ;
	for(lx=0;lx<vpl->lockCnt;lx++)
	 { if (vpl->lock[lx].surviveProc) continue ;	/* Don't remove lock if it is to survive its creating process */
	   for(ox=0;ox<vpl->lock[lx].ownerCnt;ox++)
	    { if (vpl->lock[lx].ownerList[ox] != procId) continue ;
	      for(i=ox+1;i<vpl->lock[lx].ownerCnt;i++) { vpl->lock[lx].ownerList[i-1] = vpl->lock[lx].ownerList[i] ; } ;
	      vpl->lock[lx].ownerCnt-- ; ox-- ;
	    } ;
	   for(wx=0;wx<vpl->lock[lx].waitCnt;wx++)
	    { if (vpl->lock[lx].waitList[wx] != procId) continue ;
	      for(i=wx+1;i<vpl->lock[lx].waitCnt;i++) { vpl->lock[lx].waitList[i-1] = vpl->lock[lx].waitList[i] ; } ;
	      vpl->lock[lx].waitCnt-- ; wx-- ;
	    } ;
/*	   If nothing attached to lock (or waiting) then trash it */
	   if (vpl->lock[lx].ownerCnt <= 0 && vpl->lock[lx].waitCnt <= 0)
	    { for(i=lx+1;i<vpl->lockCnt;i++) { vpl->lock[i-1] = vpl->lock[i] ; } ;
	      vpl->lockCnt-- ; lx-- ;
	    } ;
	 } ;
	FREEMTLOCK(vpl->lockOnLockTbl) ;
	return(TRUE) ;
}

/*	Returns lockId or UNUSED */
UNIQUEID v4pl_grabLockMultiple(gla)
  struct V4PL__grabLockArgs *gla ;
{ time_t lockTime = 0 ;
  struct V4PL__ProcLock *vpl ;
  INDEX ix,rlx,lx ; COUNTER tries, cdChkDefunct, resetChkDefunct = UNUSED ; UNIQUEID uId ;
  UCCHAR *ucb,*ucd,*uct, lockNameBuf[V4PL_lockNameBufMax] ;
  
	vpl = gla->vpl ; uId = UNUSED ;
	
	for(tries=0;;tries++)
	 { for(;!GETSPINLOCK(&vpl->lockOnLockTbl);) {} ;
	   gla->lockIdCnt = 0 ;
/*	   Try to grab all locks (delimited by comma) at once. Otherwise have potential for deadly-embrace with another process */
	   UCstrcpy(lockNameBuf,gla->lockNameBuf) ;
	   for(rlx=0,ucb=lockNameBuf;rlx<V4PL_maxLocksOneCall;rlx++)
	    { for(;*ucb==UClit(' ');ucb++) { } ;	/* Trim leading spaces */
	      ucd = UCstrchr(ucb,',') ;
	      if (ucd != NULL)
	       { *ucd = UCEOS ;
	          for(uct=ucd-1;*uct==UClit(' ');uct--)	/* Trim trailing spaces */
		   { *uct = UCEOS ; } ;
	       } ;
	      uId = v4pl_grabSingleLock(gla,ucb) ;	/* Try to grab this lock */
	      if (uId > 0)				/* Did we get it ? */
	       { gla->lockIds[gla->lockIdCnt++] = uId ;	/*  Yes */
	         if (ucd == NULL) break ;		/* End of lock-name list - all done */
	         ucb = ucd + 1 ;			/* Advance to next lock */
	         continue ;
	       } ;
/*	      Could not grab lock (or an error) - release any locks we may have already grabbed */
	      FREEMTLOCK(vpl->lockOnLockTbl) ;
	      for(ix=0;ix<gla->lockIdCnt;ix++)
	       { vrpl_releaseLock(gla->ctx,vpl,gla->procId,gla->lockIds[ix]) ; } ;
	      if (uId == 0) goto nolock ;		/* Could not grab lock - have to try later */
	      return(UNUSED) ;				/* Got error */
	    } ;
/*	   If here then allocated all the locks - return OK */
	   FREEMTLOCK(vpl->lockOnLockTbl) ;
	   if (gla->dimId != UNUSED)
	    { struct V4DPI__Point *ipt ;
	      DIMVAL(ipt,gla->ctx,gla->dimId) ;
	      if (ipt != NULL) ipt->Value.IntVal = TRUE ;	/* If we found flag dimension in context then set it to true */
	      v4mm_FreeChunk(gla) ;			/* Since we are a thread, deallocate argument block */
	    } ;
	   return(uId) ;				/* Return last lock grabbed (full list in gla->lockIds[]) */

/*	   Could not grab lock - release any locks we may have just obtained */
nolock:
	   if (resetChkDefunct == UNUSED)
	    { resetChkDefunct = 1500 / gla->tryInterval ;		/* Want to check about every 1500 milliseconds */
	      if (resetChkDefunct <= 0) resetChkDefunct = 1 ;
	      cdChkDefunct = resetChkDefunct ;
	    } ;
	   if ((cdChkDefunct--) <= 0)
	    { v4pl_removeDefunctPids(gla->ctx,vpl,UNUSED) ; cdChkDefunct = resetChkDefunct ; } ;
	   if (gla->waitMax == WAITMAXNONE) goto fail ;
	   if (gla->waitMax != WAITMAXFOREVER)
	    { if (lockTime == 0) time(&lockTime) ;		/* Get initial time so we can set up to wait max */
	      if (time(NULL) - lockTime >= gla->waitMax) goto fail ;	/* Waited too long - return error */
	    } ;
	   HANGLOOSE(gla->tryInterval) ;
	 } ;

fail:

/*	Before returning, make sure we are not on any wait lists */
	for(;!GETSPINLOCK(&vpl->lockOnLockTbl);) {} ;
	for(lx=0;lx<vpl->lockCnt;lx++)
	 { INDEX wx,i ;
	   for(wx=0;wx<vpl->lock[lx].waitCnt;wx++)
	    { if (vpl->lock[lx].waitList[wx] != gla->procId) continue ;
	      for(i=wx+1;i<vpl->lock[lx].waitCnt;i++) { vpl->lock[lx].waitList[i-1] = vpl->lock[lx].waitList[i] ; } ;
	      vpl->lock[lx].waitCnt-- ; wx-- ;
	    } ;
	 } ;
	FREEMTLOCK(vpl->lockOnLockTbl) ;

	v_Msg(gla->ctx,gla->ctx->ErrorMsgAux,"LocksNotAvail") ;
	return(UNUSED) ;
	    
}

/*	Returns lockid, UNUSED if error, 0 if lock not available */
UNIQUEID v4pl_grabSingleLock(gla,lockName)
  struct V4PL__grabLockArgs *gla ;
  UCCHAR *lockName ;
{
  struct V4PL__ProcLock *vpl ;
  COUNTER lx ; INDEX i,wx ; UNIQUEID lockId ;
  
	vpl = gla->vpl ; 
	for(lx=0;lx<vpl->lockCnt;lx++)
	 { if (UCstrcmp(vpl->lock[lx].lockName,lockName) == 0) break ;
	 } ;
/*	If this is this a new lock then job is pretty easy */
	if (lx >= vpl->lockCnt)
	 { vpl->lock[lx].lockId = (lockId = ++vpl->luLockId) ;
	   if (UCstrlen(lockName) < V4PL_lockNameMax) { UCstrcpy(vpl->lock[lx].lockName,lockName) ; }
	    else { UCstrncpy(vpl->lock[lx].lockName,lockName,V4PL_lockNameMax-1) ; vpl->lock[lx].lockName[V4PL_lockNameMax-1] = UCEOS ; } ;
	   vpl->lock[lx].lockMode = gla->lockMode ;
	   vpl->lock[lx].ownerCnt = 1 ; vpl->lock[lx].ownerList[0] = gla->procId ;
	   vpl->lock[lx].waitCnt = 0 ;
	   vpl->lock[lx].releaseTime = (gla->holdSeconds <= 0 ? 0 : gla->holdSeconds + time(NULL)) ;
	   vpl->lock[lx].surviveProc = gla->surviveProc ;
	   vpl->lockCnt++ ;
	   return(lockId) ;
	 } ;
/*	Make sure we don't already own this lock */
	for(i=0;i<vpl->lock[lx].ownerCnt;i++)
	 { if (vpl->lock[lx].ownerList[i] == gla->procId) 
	    { FREEMTLOCK(vpl->lockOnLockTbl) ;
	      v_Msg(gla->ctx,gla->ctx->ErrorMsgAux,"LockAlreadyOwn",vpl->lock[lx].lockName,vpl->lock[lx].lockId) ;
	      return(UNUSED) ;
	    } ;
	 } ;
/*	Lock exists - this can get messy */
/*	   If requesting Read access & currently set to Read then just grab lock */
	   if (gla->lockMode == V4MM_LockMode_Read && gla->lockMode == vpl->lock[lx].lockMode)
	    { if (vpl->lock[lx].ownerCnt >= V4PL_maxOwnersOfLock) goto nolock ;
	      vpl->lock[lx].ownerList[vpl->lock[lx].ownerCnt] = gla->procId ;
	      vpl->lock[lx].ownerCnt++ ;
	      vpl->lock[lx].releaseTime = (gla->holdSeconds <= 0 ? 0 : gla->holdSeconds + time(NULL)) ;
	      lockId = vpl->lock[lx].lockId ;
	      return(lockId) ;
	    } ;
/*	   Have to get in line and wait our turn */
	    { 
	      for(wx=0;wx<vpl->lock[lx].waitCnt;wx++) { if (vpl->lock[lx].waitList[wx] == gla->procId) break ; } ;
	      if (wx >= vpl->lock[lx].waitCnt)
	       { if (vpl->lock[lx].waitCnt >= V4PL_maxWaitOnLock) goto nolock ;
	         vpl->lock[lx].waitList[vpl->lock[lx].waitCnt] = gla->procId ; vpl->lock[lx].waitCnt++ ;
	       } ;
	    } ;
/*	   Is the lock free? */
	   if (vpl->lock[lx].releaseTime > 0)		/* If lock set to auto-release and the max time is exceeded then release the lock */
	    { if (time(NULL) >= vpl->lock[lx].releaseTime) vpl->lock[lx].ownerCnt = 0 ;
	    } ;
	   if (!(vpl->lock[lx].ownerCnt <= 0 || (gla->lockMode == V4MM_LockMode_Read && gla->lockMode == vpl->lock[lx].lockMode))) goto nolock ;
/*	   Lock is free, are we next in line? */
	   if (!(gla->lockMode == V4MM_LockMode_Read || vpl->lock[lx].waitList[0] == gla->procId)) goto nolock ;
/*	   Looks like we can grab lock, remove ourselves from waitList */
	   for(wx=0;wx<vpl->lock[lx].waitCnt;wx++)
	    { if (vpl->lock[lx].waitList[wx] != gla->procId) continue ;
	      for(i=wx+1;i<vpl->lock[lx].waitCnt;i++) { vpl->lock[lx].waitList[i-1] = vpl->lock[lx].waitList[i] ; } ;
	      vpl->lock[lx].waitCnt-- ; wx-- ;
	    } ;
/*	   Grab the lock */
	   lockId = vpl->lock[lx].lockId ;
	   vpl->lock[lx].lockMode = gla->lockMode ;
/*	   If max hold time set then set release time to new max time, or leave alone if current max is greater (possible with multiple read locks) */
	   if (gla->holdSeconds > 0)
	    { time_t maxTime = time(NULL) + gla->holdSeconds ;
	      if (maxTime > vpl->lock[lx].releaseTime) vpl->lock[lx].releaseTime = maxTime ;
	    } ;
	   vpl->lock[lx].ownerList[vpl->lock[lx].ownerCnt] = gla->procId ; vpl->lock[lx].ownerCnt++ ;
	   return(lockId) ;

nolock:
	return(0) ;
}

UNIQUEID v4pl_getLockId(ctx,vpl,lockName)
  struct V4C__Context *ctx ;
  struct V4PL__ProcLock *vpl ;
  UCCHAR *lockName ;
{ INDEX lx ;

	for(lx=0;lx<vpl->lockCnt;lx++)
	 { if (UCstrcmp(vpl->lock[lx].lockName,lockName) == 0) return(vpl->lock[lx].lockId) ; } ;
	return(UNUSED) ;
}

LOGICAL vrpl_releaseLock(ctx,vpl,procId,lockId)
  struct V4C__Context *ctx ;
  struct V4PL__ProcLock *vpl ;
  UNIQUEID procId ;
  UNIQUEID lockId ;
{
  INDEX lx,ox,wx,i ;

	for(;!GETSPINLOCK(&vpl->lockOnLockTbl);) {} ;
	for(lx=0;lx<vpl->lockCnt;lx++)
	 { if (vpl->lock[lx].lockId != lockId) continue ;
	   vpl->lock[lx].releaseTime = 0 ;
	   for(ox=0;ox<vpl->lock[lx].ownerCnt;ox++)
	    { if (vpl->lock[lx].ownerList[ox] != procId) continue ;
	      for(i=ox+1;i<vpl->lock[lx].ownerCnt;i++) vpl->lock[lx].ownerList[i-1] = vpl->lock[lx].ownerList[i] ;
	      vpl->lock[lx].ownerCnt-- ; ox-- ;
	    } ;
	   for(wx=0;wx<vpl->lock[lx].waitCnt;wx++)
	    { if (vpl->lock[lx].waitList[wx] != procId) continue ;
	      for(i=wx+1;i<vpl->lock[lx].waitCnt;i++) vpl->lock[lx].waitList[i-1] = vpl->lock[lx].waitList[i] ;
	      vpl->lock[lx].waitCnt-- ; wx-- ;
	    } ;
	   if (vpl->lock[lx].surviveProc)	/* If this option set then force lock to be completely free */
	    { vpl->lock[lx].ownerCnt = 0 ; vpl->lock[lx].waitCnt = 0 ; } ;
	   FREEMTLOCK(vpl->lockOnLockTbl) ;
	   return(TRUE) ;
	 } ;
	FREEMTLOCK(vpl->lockOnLockTbl) ;
	return(FALSE) ;
}

/*	v4pl_isProcessGone - Returns TRUE if specified process no longer running */
/*	Call: logical = v4pl_isProcessGone( pid )
	  where logical is TRUE if process is active,
		pid is pid under question				*/


#ifdef WINNT
#include <psapi.h>		// also need to link in Psapi.lib & kernel32.lib
LOGICAL v4pl_isProcessGone(pid)
 PID pid ;
{ 
  DWORD pidList[1000] ; DWORD actBytes ;
  INDEX i ;

	if (EnumProcesses(pidList,sizeof(pidList),&actBytes) == 0) return(FALSE) ;	/* If this fails assume process still active */
	for(i=0;i<actBytes/sizeof(pidList[0]);i++)
	 { if (pid == pidList[i]) return(TRUE) ; } ;
	return(FALSE) ;
}
#endif
#ifdef UNIX
LOGICAL v4pl_isProcessGone(pid)
 PID pid ;
{ 
	return(PROCESS_GONE(pid)) ;
}
#endif

struct V4DPI__Point *v4im_DoLock(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt ;						/* Ptr for result */
  INTMODX intmodx ;
  P *argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ P *cpt,argbuf ;
  struct V4DPI__DimInfo *di ;
  static struct V4PL__ProcLock *vpl = NULL ;
  struct V4PL__ProcInfo *vpi ;
  struct V4L__ListPoint *lp, *lp1 ;
  struct V4DPI__Point vpnt,lpnt ;
  static UNIQUEID procId = UNUSED ;
  static struct V4PL__grabLockArgs *gla, glabuf ;
  LOGICAL ok, surviveProc ; COUNTER ix, lx, px, wx, ox, tryInterval, holdSeconds ;
  ETYPE lockMode, action ; UCCHAR lockNameBuf[V4PL_lockNameBufMax], procName[V4PL_procNameMax] ; UNIQUEID lockId,uId ;
  double waitSeconds ; DIMID waitDim ;

	lockMode = V4MM_LockMode_Write ; UCstrcpy(lockNameBuf,UClit("default")) ; v_Msg(ctx,procName,"@process-%1d",CURRENTPID) ;
	ok = TRUE ; action = UNUSED ; waitSeconds = WAITMAXFOREVER ; waitDim = UNUSED ; tryInterval = 200 ; holdSeconds = 0 ; surviveProc = FALSE ;
	if (vpl == NULL) 
	 { vpl = v4pl_linkToSharedSegment(ctx,UClit("V4LockTable"),V4PL_maxNumberOfLocks,V4PL_maxNumberOfProcs) ;
	   if (vpl == NULL) goto fail0 ;
	 } ;
	for (ok=TRUE,ix=1;ix<=argcnt && ok;ix++)
	 { if (argpnts[ix]->PntType != V4DPI_PntType_TagVal)
	    { v4im_GetPointUC(&ok,lockNameBuf,UCsizeof(lockNameBuf),argpnts[ix],ctx) ; if (!ok) break ;
	      continue ;
	    } ;
	   switch (v4im_CheckPtArgNew(ctx,argpnts[ix],&cpt,&argbuf))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	      case V4IM_Tag_Data:
		uId = v4im_GetPointInt(&ok,cpt,ctx) ; if (!ok) break ;
		INITLP(respnt,lp,Dim_List) ;
		for(;!GETSPINLOCK(&vpl->lockOnLockTbl);) {} ;
		if (uId >= V4PL_procIdBase)		/* Is this a lock or process ? */
		 { vpi = (struct V4PL__ProcInfo *)((char *)vpl + vpl->offsetInfo) ;
		   for(px=0;px<vpl->procCnt;px++) { if (vpi->info[px].procId == uId) break ; } ;
		   if (px >= vpl->procCnt) { v_Msg(ctx,NULL,"LockBadId",intmodx,uId) ; goto fail ; } ;
/*		   Return (procId procPid procName) */
		   intPNTv(&vpnt,uId) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&vpnt,0) ;
		   intPNTv(&vpnt,vpi->info[px].procPid) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&vpnt,0) ;
		   uccharPNTv(&vpnt,vpi->info[px].procName) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&vpnt,0) ;
		 } else
		 { for(lx=0;lx<vpl->lockCnt;lx++) { if (vpl->lock[lx].lockId == uId) break ; } ;
		   if (lx >= vpl->lockCnt) { v_Msg(ctx,NULL,"LockBadId",intmodx,uId) ; goto fail ; } ;
		   intPNTv(&vpnt,uId) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&vpnt,0) ;
		   uccharPNTv(&vpnt,vpl->lock[lx].lockName) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&vpnt,0) ;
		   intPNTv(&vpnt,vpl->lock[lx].lockMode) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&vpnt,0) ;
		   INITLP(&lpnt,lp1,Dim_List)	/* Create list of owners */
		   for(ox=0;ox<vpl->lock[lx].ownerCnt;ox++)
		    { intPNTv(&vpnt,vpl->lock[lx].ownerList[ox]) ;  v4l_ListPoint_Modify(ctx,lp1,V4L_ListAction_Append,&vpnt,0) ; } ;
		   ENDLP(&lpnt,lp1) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&lpnt,0) ;
		   INITLP(&lpnt,lp1,Dim_List)	/* Create list of waiting processes */
		   for(wx=0;wx<vpl->lock[lx].waitCnt;wx++)
		    { intPNTv(&vpnt,vpl->lock[lx].waitList[wx]) ;  v4l_ListPoint_Modify(ctx,lp1,V4L_ListAction_Append,&vpnt,0) ; } ;
		   ENDLP(&lpnt,lp1) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&lpnt,0) ;
		 } ;
		FREEMTLOCK(vpl->lockOnLockTbl) ;
		ENDLP(respnt,lp) ; return(respnt) ;
	      case V4IM_Tag_Hold:	holdSeconds = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
	      case V4IM_Tag_Id:		v4im_GetPointUC(&ok,procName,UCsizeof(procName),cpt,ctx) ; break ;
	      case V4IM_Tag_Interval:	tryInterval = v4im_GetPointLog(&ok,cpt,ctx) ;
					if (tryInterval < 10 || tryInterval > 10000) tryInterval = 200 ;
					break ;
	      case V4IM_Tag_Life:
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:		v_Msg(ctx,NULL,"ModTagValue",intmodx,ix,V4IM_Tag_Type,cpt) ; goto fail ;
		   case _Process:	surviveProc = FALSE ; break ;
		   case _Release:	surviveProc = TRUE ; break ;
		 } ; break ;
	      case V4IM_Tag_ListOf:
		INITLP(respnt,lp,Dim_List) ;
		if (vpl == NULL)	/* If vpl not defined then just return empty list */
		 { ENDLP(respnt,lp) ; return(respnt) ; } ;
		for(;!GETSPINLOCK(&vpl->lockOnLockTbl);) {} ;
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:		v_Msg(ctx,NULL,"ModTagValue",intmodx,ix,V4IM_Tag_Type,cpt) ; goto fail ;
		   case _Locks:
			for(lx=0;lx<vpl->lockCnt;lx++) { intPNTv(&vpnt,vpl->lock[lx].lockId) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&vpnt,0) ; } ;
			break ;
		   case _Processes:
			vpi = (struct V4PL__ProcInfo *)((char *)vpl + vpl->offsetInfo) ;
			for(px=0;px<vpl->procCnt;px++)
			 { intPNTv(&vpnt,vpi->info[px].procId) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&vpnt,0) ; } ;
			break ;
		 } ;
		FREEMTLOCK(vpl->lockOnLockTbl) ;
		ENDLP(respnt,lp) ; return(respnt) ;
	      case V4IM_Tag_Name:	v4im_GetPointUC(&ok,lockNameBuf,UCsizeof(lockNameBuf),cpt,ctx) ;
					if (action == UNUSED) action = 2 ;	/* Default to grab-lock */
					break ;
	      case V4IM_Tag_Release:	v4im_GetPointUC(&ok,lockNameBuf,UCsizeof(lockNameBuf),cpt,ctx) ; action = 1 ; break ;
	      case -V4IM_Tag_Release:	action = 1 ; break ;
	      case V4IM_Tag_Type:
		switch (v4im_GetDictToEnumVal(ctx,cpt))
		 { default:		v_Msg(ctx,NULL,"ModTagValue",intmodx,ix,V4IM_Tag_Type,cpt) ; goto fail ;
		   case _Read:		lockMode = V4MM_LockMode_Read ; break ;
		   case _Write:		lockMode = V4MM_LockMode_Write ; break ;
		 } ; break ;
	      case V4IM_Tag_Wait:
		if (cpt->PntType == V4DPI_PntType_Logical)
		 { waitSeconds = (v4im_GetPointLog(&ok,cpt,ctx) ? WAITMAXFOREVER : WAITMAXNONE) ; break ; } ;
		if (cpt->Dim == Dim_Dim)
		 { DIMINFO(di,ctx,cpt->Value.IntVal) ;
		   if (di->PointType != V4DPI_PntType_Logical) { v_Msg(ctx,NULL,"ModArgPntType2",intmodx,ix,di->PointType,V4DPI_PntType_Logical) ; goto fail ; } ;
		   waitDim = cpt->Value.IntVal ; waitSeconds = WAITMAXFOREVER ;
		   break ;
		 } ;
		waitSeconds = v4im_GetPointDbl(&ok,cpt,ctx) ; break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ix-1) ; goto fail ; } ;
	
	if (procId == UNUSED)
	 { procId = v4pl_RegisterProc(ctx,vpl,procName) ;
	 } ;
	switch (action)
	 { default:
	   case UNUSED:
		logPNTv(respnt,FALSE) ; return(respnt) ;
	   case 1:		/* Release lock */
		lockId = v4pl_getLockId(ctx,vpl,lockNameBuf) ;
		if (lockId == UNUSED) { v_Msg(ctx,NULL,"LockNotFound",intmodx,lockNameBuf) ; goto fail ; } ;
		logPNTv(respnt,vrpl_releaseLock(ctx,vpl,procId,lockId)) ;
		return(respnt) ;
	   case 2:		/* Grab lock */
		gla = (waitDim == UNUSED ? &glabuf : (struct V4PL__grabLockArgs *)v4mm_AllocChunk(sizeof *gla,FALSE)) ;	/* If creating thread then allocate new arg buffer (thread will deallocate before terminating) */
		gla->ctx = ctx ; gla->vpl = vpl ; gla->procId = procId ; UCstrcpy(gla->lockNameBuf,lockNameBuf) ; gla->lockMode = lockMode ;
		gla->tryInterval = tryInterval ; gla->waitMax = waitSeconds ; gla->dimId = waitDim ; gla->holdSeconds = holdSeconds ; gla->surviveProc = surviveProc ;
		if (surviveProc && lockMode != V4MM_LockMode_Write)
		 { v_Msg(ctx,NULL,"LockWrite",intmodx,V4IM_Tag_Life,DE(Process),V4IM_Tag_Type,DE(Write)) ; goto fail ; } ;
/*		If got a waitDim then start independent thread to allocate lock, indicate success by setting dimension to TRUE in context */
#ifdef WINNT
		if (waitDim != UNUSED) 
		 { HANDLE hThread ; DWORD dwThreadId ;
		   logPNTv(respnt,FALSE) ;
		   if (!v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,respnt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; goto fail ; } ;
		   hThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)v4pl_grabLockMultiple,gla,0,&dwThreadId) ;
		   return(respnt) ;
		 } ;
#else
		goto fail0 ;
#endif
		lockId = v4pl_grabLockMultiple(gla) ;
		if (lockId == UNUSED) { v_Msg(ctx,NULL,"LockGrab",intmodx) ; goto fail ; } ;
		if (gla->lockIdCnt <= 1) { intPNTv(respnt,lockId) ; }
		 else { INITLP(respnt,lp,Dim_List) ;
			for(lx=0;lx<gla->lockIdCnt;lx++)
			 { intPNTv(&vpnt,gla->lockIds[lx]) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&vpnt,0) ; } ;
			ENDLP(respnt,lp) ;
		      } ;
		return(respnt) ;
	 } ;

fail0:	v_Msg(ctx,NULL,"LockSysErr",intmodx) ;
fail:
	REGISTER_ERROR(0) ; return(NULL) ;
}


struct V4DPI__Point *v4im_DoEcho(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt ;						/* Ptr for result */
  INTMODX intmodx ;
  P *argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ P *cpt, *ipt, *opt, ptbuf ;
  struct UC__File UCFile ;
  struct V4IM__XMLNest xml ;
  INDEX filex,ix ; UCCHAR tb[V4TMBufMax],*bp,*bp1 ; LOGICAL ok, gotJSON, gotJS, toDfltData ;

	if (gpi->RestrictionMap & V_Restrict_QuotaEcho)
	 { if (vout_TotalBytes > gpi->QuotaData) { v_Msg(ctx,NULL,"RestrictQuotaO",intmodx,gpi->QuotaData) ; goto fail ; } ;
	 } ;
	filex = ctx->Frame[ctx->FrameCnt - 1].DataStreamFileX ;
	toDfltData = (filex == UNUSED) ;	/* TRUE if going to default data stream */
	gotJSON = FALSE ; gotJS = FALSE ;
/*	If we are buffering AJAX output or Deferred XML then flip filex and make this EchoP to prevent cr/lf at end */
	if (filex == UNUSED && gpi->fileIdDefer != UNUSED)
	 { filex = vout_FileIdToFileX(gpi->fileIdDefer) ;
//veh120410 - moved logic to bottom	   if (UCnotempty(gpi->patternAJAXJSON)) intmodx = V4IM_OpCode_EchoP ;	/* If this is because we are doing JSON then flip to EchoP to eliminate EOL characers */
	 } ;
	if (intmodx == V4IM_OpCode_EchoP) goto echop_entry ;
	
	if (argcnt > 0 ? argpnts[1]->PntType == V4DPI_PntType_TagVal : FALSE)
	 { switch (v4im_CheckPtArgNew(ctx,argpnts[1],&cpt,&ptbuf))
	    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	      case V4IM_Tag_Include:
	      case V4IM_Tag_JSON:
	      case V4IM_Tag_Buffer:
	      case V4IM_Tag_Javascript:
	      case V4IM_Tag_Out:
	      case V4IM_Tag_XML:	break ; 	/* Will handle below in EchoP() */
	      case V4IM_Tag_NoPrefix:
		return(v4imu_EchoTable(ctx,respnt,intmodx,argpnts,argcnt,trace,V4IM_Tag_NoPrefix)) ;
	      case V4IM_Tag_Table:
		return(v4imu_EchoTable(ctx,respnt,intmodx,argpnts,argcnt,trace,V4IM_Tag_Table)) ;
	    } ;
	 } ;

echop_entry:
	xml.Count = 0 ;
	for(ix=1;ix<=argcnt;ix++)
	 { ipt = argpnts[ix] ;
	   if (ipt->PntType == V4DPI_PntType_TagVal)
	    { switch (v4im_CheckPtArgNew(ctx,ipt,&cpt,&ptbuf))
	       { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
		 case V4IM_Tag_Unk:		v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;


		 case V4IM_Tag_Buffer:
		 { UCCHAR *o, *fbuf ; FILEID fileId ; COUNTER bytes ; INDEX i ;
		   fileId = vout_PntIdToFileId(ctx,(struct V4DPI__LittlePoint *)cpt) ;
		   if (fileId == UNUSED) { v_Msg(ctx,NULL,"StreamNoOutput",intmodx,V4IM_Tag_Out,cpt) ; goto fail ; } ;
		   fbuf = vout_GetOutputBuffer(fileId,UNUSED,ctx->ErrorMsgAux) ;
		   if (fbuf == NULL) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,cpt) ; goto fail ; } ;
		   bytes = vout_CountForFile(fileId,UNUSED,FALSE,ctx->ErrorMsgAux) ;
		   if (gotJSON)
		    { o = UCTBUF1 ; *(o++) = UClit('"') ;
		      for(i=0,o=UCTBUF1;i<bytes && i < V4TMBufMax;i++)
		       { switch (fbuf[i])
			  { default:		*(o++) = fbuf[i] ; break ;
			    case '\r':	*(o++) = UClit('\\') ; *(o++) = UClit('r') ; break ;
			    case '\n':	*(o++) = UClit('\\') ; *(o++) = UClit('n') ; break ;
			    case '"':	*(o++) = UClit('\\') ; *(o++) = UClit('"') ; break ;
			  } ;
		       } ;  *(o++) = UClit('"') ; *(o++) = UCEOS ; vout_UCTextFileXCont(filex,0,UCTBUF1) ;
		    } else
		    { UCCHAR *eol = UClit("") ;
		      for(;eol!=NULL;fbuf=eol+1)
		       { UCCHAR *b1,*b2,*src,*dst ; LOGICAL nl ;
		         eol = UCstrchr(fbuf,EOLbt) ; if (eol != NULL) *eol = UCEOS ;
			 
			 if (!gotJS)
			  { vout_UCTextFileX(filex,0,fbuf) ; if (eol != NULL) *eol = EOLbt ; continue ; } ;
/*			 Outputting Javascript - strip out comments, extra spaces, etc. */
			 for(b1=fbuf;;b1++) { if (!vuc_IsWSpace(*b1)) break ; } ;	/* Get rid of leading white-space */
		         for(b2=b1;*b2!=UCEOS;b2++)
			  { if (*b2 == UClit('/') && *(b2+1) == UClit('/') && (b2 == b1 || vuc_IsWSpace(*(b2-1))) ) *b2 = UCEOS ; } ;	/* Ignore after //comment (as long as begin of line or prior character is white-space)*/
			 for(src=b1+1,dst=b1+1;*src!=UCEOS;src++,dst++)
			  { if (*src == UClit(';')) { if (*(dst-1) == UClit(' ')) dst-- ; } ;
			    if (*src == UClit('{')) { if (*(dst-1) == UClit(' ')) dst-- ; } ;
			    if (*src == UClit('=')) { if (*(dst-1) == UClit(' ')) dst-- ;  if (*(src+1) == UClit(' ')) { src++ ; *dst = UClit('=') ; continue ; } ; } ;
			    if (src != dst) *dst = *src ;
			    if (*src == UCEOS) break ;
			  } ; *dst = UCEOS ;
			 b2 = &b1[UCstrlen(b1)-1] ;				/* b2 = last character in line */
			 for(;*b2<26 && b2!=b1;b2--) {} ;			/* Skip left over end-of-line */
			 nl = TRUE ;
			 if (*b2 == UClit(';') || *b2 == UClit(',') || *b2 == UClit('{'))
			  { *(b2+1) = UCEOS ; nl = FALSE ; } ;		/* If ';' last character in line then continue next line with this one */
			 vout_UCTextFileX(filex,0,b1) ; if (nl) vout_NLFileX(filex) ;
			 if (eol != NULL) *eol = EOLbt ;
		       } ;
		    } ;
		   logPNTv(respnt,TRUE) ;  return(respnt) ; ;
		 }


		 case V4IM_Tag_Javascript:	gotJS = TRUE ;	/* Fall through and handle as Include::xxx */
	         case V4IM_Tag_Include:
		   if (gpi->RestrictionMap & V_Restrict_FileRead) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto fail ; } ;
		   v4im_GetPointFileName(&ok,tb,UCsizeof(tb),cpt,ctx,NULL) ;
		   if (gotJSON)
		    { if (!v_UCFileOpen(&UCFile,tb,UCFile_Open_ReadBin,TRUE,ctx->ErrorMsgAux,intmodx)) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,cpt) ; goto fail ; } ;
//VEH140721 - Take out this leading quote, it is injected in JSON::xxx section right below
//		      vout_UCTextFileXCont(filex,0,UClit("'")) ;
		      for(;;)
		       { char fbuf[4096] ; UCCHAR *o ; COUNTER bytes ; INDEX i ;
			 bytes = fread(fbuf,1,sizeof fbuf,UCFile.fp) ; if (bytes <= 0) break ;
			 for(i=0,o=UCTBUF1;i<bytes;i++)
			  { switch (fbuf[i])
			     { default:		*(o++) = fbuf[i] ; break ;
			       case '\r':	*(o++) = UClit('\\') ; *(o++) = UClit('r') ; break ;
			       case '\n':	*(o++) = UClit('\\') ; *(o++) = UClit('n') ; break ;
			       case '"':	*(o++) = UClit('\\') ; *(o++) = UClit('"') ; break ;
			     } ;
			  } ; *(o++) = UCEOS ;
			 vout_UCTextFileXCont(filex,0,UCTBUF1) ;
		       } ;
		      vout_UCTextFileXCont(filex,0,UClit("\"")) ;
		    } else
		    { if (!v_UCFileOpen(&UCFile,tb,UCFile_Open_Read,TRUE,ctx->ErrorMsgAux,intmodx)) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,cpt) ; goto fail ; } ;
		      UCFile.wantEOL = FALSE ;
		      for(;;)
		       { UCCHAR *b1,*b2,*src,*dst ; LOGICAL nl ;
		         if (v_UCReadLine(&UCFile,UCRead_UC,tb,V4TMBufMax,ctx->ErrorMsgAux) < 0) break ;
			 if (!gotJS)
			  { vout_UCTextFileX(filex,0,tb) ; vout_NLFileX(filex) ; continue ; } ;
/*			 Outputting Javascript - strip out comments, extra spaces, etc. */
			 for(b1=tb;;b1++) { if (!vuc_IsWSpace(*b1)) break ; } ;	/* Get rid of leading white-space */
		         for(b2=b1;*b2!=UCEOS;b2++)
			  { if (*b2 == UClit('/') && *(b2+1) == UClit('/') && (b2 == b1 || vuc_IsWSpace(*(b2-1)))) *b2 = UCEOS ; } ;	/* Ignore after //comment */
			 for(src=b1+1,dst=b1+1;*src!=UCEOS;src++,dst++)
			  { if (*src == UClit(';')) { if (*(dst-1) == UClit(' ')) dst-- ; } ;
			    if (*src == UClit('{')) { if (*(dst-1) == UClit(' ')) dst-- ; } ;
			    if (*src == UClit('=')) { if (*(dst-1) == UClit(' ')) dst-- ;  if (*(src+1) == UClit(' ')) { src++ ; *dst = UClit('=') ; continue ; } ; } ;
			    if (src != dst) *dst = *src ;
			    if (*src == UCEOS) break ;
			  } ; *dst = UCEOS ;
			 b2 = &b1[UCstrlen(b1)-1] ;				/* b2 = last character in line */
			 for(;*b2<26 && b2!=b1;b2--) {} ;			/* Skip left over end-of-line */
			 nl = TRUE ;
			 if (*b2 == UClit(';') || *b2 == UClit(',') || *b2 == UClit('{'))
			  { *(b2+1) = UCEOS ; nl = FALSE ; } ;		/* If ';' last character in line then continue next line with this one */
			 vout_UCTextFileX(filex,0,b1) ; if (nl) vout_NLFileX(filex) ; 
		       } ;
		    } ;
		   v_UCFileClose(&UCFile) ; logPNTv(respnt,TRUE) ;  return(respnt) ; ;
	         case V4IM_Tag_JSON:
		   v4im_GetPointUC(&ok,UCTBUF1,128,cpt,ctx) ;
		   if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ix-1) ; goto fail ; } ;
		   UCstrcpy(tb,UClit("\"")) ; UCstrcat(tb,UCTBUF1) ; UCstrcat(tb,UClit("\":\"")) ;
		   vout_UCTextFileX(filex,0,tb) ;
		   gotJSON = TRUE ; break ;
		 case V4IM_Tag_Out:
		   opt = v4dpi_IsctEval(respnt,cpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (opt == NULL) { v_Msg(ctx,NULL,"ModArgEval",intmodx,ix,cpt) ; goto fail ; } ;
		   filex = vout_PntIdToFileX(ctx,(struct V4DPI__LittlePoint *)opt) ;
		   if (filex == UNUSED)			/* If not a specified file output then try for internal V4 stream */
		    { filex = vout_StreamToFileX(v4im_OutputGetStream(ctx,cpt)) ;
		      if (filex == UNUSED)
		       { v_Msg(ctx,NULL,"StreamNoOutput",intmodx,V4IM_Tag_Out,opt) ; goto fail ; } ;
		    } ;
		   toDfltData = FALSE ;			/* No longer going to default stream */
		   break ;
		 case V4IM_Tag_XML:
		   switch (v4imu_XMLSTART(ctx,&xml,filex,cpt,FALSE,intmodx,NULL))
		    { default:				goto fail ;
		      case XMLSTART_None:		break ;
		      case XMLSTART_Normal:		break ;
		      case XMLSTART_Defer:		v_Msg(ctx,NULL,"XMLDeferInv",intmodx) ; goto fail ;
		    } ;
		   break ;
	       } ;
	      continue ;
	    } ;
	   if (memcmp(ipt,&protoNone,protoNone.Bytes) == 0) continue ;		/* Don't print UV4:none point */
	   if (ipt->PntType == V4DPI_PntType_BigText)
	    { bp=v4_BigTextCharValue(ctx,ipt) ;
	      if (bp == NULL) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; goto fail ; } ;
	      if (gotJSON) { goto do_json ; } ;			/* Handle bigtext->JSON here */
	      for(;;)
	       { bp1 = UCstrchr(bp,EOLbt) ;
		 if (bp1 == NULL)
		  { if (filex == UNUSED) { vout_UCText(VOUT_Data,0,bp) ; } else { vout_UCTextFileX(filex,0,bp) ; } ;
		    if (UCnotempty(gpi->patternAJAXJSON) && intmodx == V4IM_OpCode_EchoP)
		    vout_UCTextFileXCont(filex,UNUSED,UClit("")) ;	/* VEH120410 - see comments below */
		    break ;
		  } ;
		 *bp1 = UCEOS ;
		 vout_UCTextFileX(filex,0,bp) ; vout_NLFileX(filex) ;
		 *bp1 = EOLbt ; bp = bp1 + 1 ;
	       } ;
	      if (argcnt == 1) intmodx = V4IM_OpCode_EchoP ;	/* If single BigText argument, don't append CR at end */
	      continue ;
	    } else if (ipt->PntType == V4DPI_PntType_MemPtr)
	    { UCCHAR *p ; memcpy(&p,&ipt->Value.MemPtr,sizeof p) ;
	      if (!vout_UCTextFileX(filex,0,p))
	       { v_Msg(ctx,NULL,"OutputErr",intmodx) ; goto fail ; } ;
	      v4mm_FreeChunk(p) ;
	      continue ;
	    } else
	    { if (!v4sxi_SpecialDisplayer(ctx,ipt,V4TMBufMax,tb)) v4dpi_PointToStringML(tb,ipt,ctx,V4DPI_FormatOpt_Echo,UCsizeof(tb)) ;
	      if (gotJSON) { bp = tb ; goto do_json ; } ;
	      if (!vout_UCTextFileX(filex,0,tb))
	       { v_Msg(ctx,NULL,"OutputErr",intmodx) ; goto fail ; } ;
/*	      If doing JSON then each output element is considered separate JSON value and will be delimited by commas later on */
/*	      However if doing EchoP() then assume we want all of this contiguous so get rid of terminating EOS so everything becomes one string */
	      if (UCnotempty(gpi->patternAJAXJSON) && intmodx == V4IM_OpCode_EchoP)
	       vout_UCTextFileXCont(filex,UNUSED,UClit("")) ;	/* VEH120410 */
	    } ;
	   continue ;			/* Continue with next argument */
/*	   Here to convert string in bp to valid JSON string literal contents */
do_json:
	   { UCCHAR *bt ; LENMAX max = V4LEX_BigText_Max*2 ;
	     bp1 = (bt = v4mm_AllocUC(max)) ;
	     for(;*bp!=UCEOS && max>0;bp++,max--)
	      { switch(*bp)
	         { default:		*(bp1++) = *bp ; break ;
		   case UClit('"'):	*(bp1++) = UClit('\\') ; *(bp1++) = *bp ; break ;
		   case UClit('\r'):	*(bp1++) = UClit('\\') ; *(bp1++) = UClit('r') ; break ;
		   case UClit('\n'):	*(bp1++) = UClit('\\') ; *(bp1++) = UClit('n') ; break ;
	         } ;
	      } ;
	     *(bp1++) = UCEOS ;
	     vout_UCTextFileXCont(filex,0,bt) ;
	     v4mm_FreeChunk(bt) ;
	   }
	   continue ;
	 } ;
	if (gotJSON)			/* Terminate JSON string */
	 { vout_UCTextFileXCont(filex,0,UClit("\"")) ; } ;
	if (!v4imu_XMLEND(ctx,&xml,filex,intmodx,NULL)) goto fail ;
/*	End with NL if not EchoP() and not doing JSON output to the default DATA stream (filex = UNUSED) */
	if (!(intmodx == V4IM_OpCode_EchoP || (toDfltData && UCnotempty(gpi->patternAJAXJSON))))
	 vout_NLFileX(filex) ;
	memcpy(respnt,&protoNone,protoNone.Bytes) ;
	return(respnt) ;

fail:	REGISTER_ERROR(0) ; return(NULL) ;
}


struct V4DPI__Point *v4im_DoEchoE(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt ;						/* Ptr for result */
  INTMODX intmodx ;
  P *argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ P *cpt, *ipt, ptbuf ;
  INDEX ix, filex ; LOGICAL ok ; UCCHAR tb[V4TMBufMax] ;

	ZUS(tb) ; ZUS(UCTBUF2) ; gpi->exitCode = 0 ;
	for(ix=1,ok=TRUE;ix<=argcnt&&ok;ix++)
	 { ipt = argpnts[ix] ;
	   if (ipt->PntType == V4DPI_PntType_TagVal)
	    { switch (v4im_CheckPtArgNew(ctx,ipt,&cpt,&ptbuf))
	       { default:		v_Msg(ctx,NULL,"TagBadUse",intmodx) ; return(NULL) ;
		 case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; return(NULL) ;
		 case V4IM_Tag_Exit:	gpi->exitCode = v4im_GetPointInt(&ok,cpt,ctx) ; break ;
		 case V4IM_Tag_URL:	v4im_GetPointUC(&ok,UCTBUF2,V4TMBufMax,cpt,ctx) ; break ;
	       } ;
	      continue ;
	    } ;
	   v4dpi_PointToStringML(UCTBUF1,ipt,ctx,V4DPI_FormatOpt_Echo,V4TMBufMax-256) ;
	   if (UCstrlen(tb) + UCstrlen(UCTBUF1) < UCsizeof(tb)) UCstrcat(tb,UCTBUF1) ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ix-1) ; return(NULL) ; } ;
/*VEH050210	If data stream not redirected to file AND we have error-output-file specified then redirect data stream now */
	if (vout_StreamToFileX(VOUT_Data) == 0 && UCstrlen(gpi->CtxADVEchoE))
	 { int i ;
	   if ((i=vout_OpenStreamFile(NULL,gpi->CtxADVEchoE,NULL,NULL,FALSE,V4L_TextFileType_ASCII,0,ctx->ErrorMsgAux)) == UNUSED) v4_UCerror(0,NULL,"V4","Eval","NOOUT",ctx->ErrorMsgAux) ;
	   vout_BindStreamFile(i,VOUT_FileType_File,VOUT_Data,ctx->ErrorMsgAux) ;
	 } ;

/*	If data stream going to anything other than stdout OR we are running with stdout being directed from parent process,
	 then send EchoS(Error::text)) to the stream */
	if ((filex=vout_StreamToFileX(VOUT_Data)) != 0)
	 { UCCHAR *eLevel ;
	   switch (intmodx)
	    { default:
	      case V4IM_OpCode_EchoE: eLevel = UClit("error") ; break ;
	      case V4IM_OpCode_EchoW: eLevel = UClit("warn") ; break ;
	      case V4IM_OpCode_EchoA: eLevel = UClit("alert") ; break ;
	    } ;
/*	   If result supposed to be ajax then don't want to create old-fashioned EchoE (for xvrestoxxx), but want to create valid AJAX - use the given pattern */
	   if (strlen(gpi->patternAJAXErr) > 0)
	    {
	      v_StringLit(tb,UCTBUF2,V4TMBufMax,'"','\\') ;
#ifdef USEOLDAJAXSUB	
	      v_Msg(ctx,UCTBUF1,gpi->patternAJAXErr,eLevel,UCTBUF2) ; vout_UCTextFileX(filex,0,UCTBUF1) ;
#else
	      vjson_SubParamsInJSON(UCTBUF1,V4TMBufMax,gpi->patternAJAXErr,UCTBUF2,eLevel) ;
	      vout_UCTextFileX(filex,0,UCTBUF1) ;
#endif
	    } else
	    { 
/*	      Dump out error message and linking URL (if we have it) */
	      v_Msg(ctx,UCTBUF1,"@BSError\nMQ%1U\nTY%2U\n",tb,eLevel) ; vout_UCTextFileX(filex,0,UCTBUF1) ;
	      if (UCnotempty(UCTBUF2))
	       { v_Msg(ctx,UCTBUF1,"@UR%1U\n",UCTBUF2) ; vout_UCTextFileX(filex,0,UCTBUF1) ; } ;
	    } ;
	 } ;
/*	Now generate V4 Error */
	switch (intmodx)
	 { case V4IM_OpCode_EchoE: gpi->ErrCount++ ; v4_UCerror(0,0,"V4E","EchoE","ECHOERR",tb) ; break ;
	   case V4IM_OpCode_EchoW: v_Msg(ctx,NULL,"@*%1M: %2U\n",intmodx,tb) ; vout_UCText(VOUT_Warn,0,ctx->ErrorMsg) ; v4_UCerror(V4E_Warn,0,"V4W","EchoW","ECHOWARN",tb) ; break ;
	   case V4IM_OpCode_EchoA: v_Msg(ctx,NULL,"@*%1M: %2U\n",intmodx,tb) ; vout_UCText(VOUT_Status,0,ctx->ErrorMsg) ; v4_UCerror(V4E_Alert,0,"V4A","EchoA","ECHOALERT",tb) ; break ;
	 } ;
	return(NULL) ;				/* Return NULL (if we ever get here) */
}

struct V4DPI__Point *v4im_DoEchoT(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt ;						/* Ptr for result */
  INTMODX intmodx ;
  P *argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ P *ipt, *cpt, *opt, isctbuf ;
  struct V4L__ListPoint *lp ;
  struct V4IM__BaA *baf ;
  LOGICAL ok,okToEcho,needTab ; INDEX filex ; UCCHAR coldelim[256],outbuf[V4TMBufMax],*obp ; INDEX ix,lx,colNum ;
  
	if (gpi->RestrictionMap & V_Restrict_QuotaEcho)
	 { if (vout_TotalBytes > gpi->QuotaData) { v_Msg(ctx,NULL,"RestrictQuotaO",intmodx,gpi->QuotaData) ; return(NULL) ; } ;
	 } ;
	okToEcho = TRUE ;
	filex = ctx->Frame[ctx->FrameCnt - 1].DataStreamFileX ;
	baf = v4ctx_FrameBaAInfo(ctx) ; 	/* Doing totals? */
	needTab = FALSE ;			/* If TRUE then output tab delimiter */
	UCstrcpy(coldelim,UClit("\t")) ;	/* coldelim = column delimiter, default to tab */
	ZUS(outbuf) ; obp = outbuf ;		/* Output buffer */
	for(colNum=0,ix=1,ok=TRUE;ok&&ix<=argcnt;ix++)
	 { LOGICAL rcv ; if (ix == 1) rcv = UNUSED ;
/*	   EchoT does not preevaluate arguments (as of 9/9/09) */
	   ipt = v4dpi_IsctEval(respnt,argpnts[ix],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	   if (ipt == NULL) { v_Msg(ctx,NULL,"ModArgEval",intmodx,ix,argpnts[ix]) ; return(NULL) ; } ;
	   if (memcmp(ipt,&protoNone,V4PS_Int) == 0) continue ;	/* Don't output UV4:none point */
	   if (ipt->PntType != V4DPI_PntType_List) { lp = NULL ; }
	    else { lp = ALIGNLP(&ipt->Value) ; } ;
	   for(lx=1;ok;lx++)
	    { if (lp != NULL)
	       { if (v4l_ListPoint_Value(ctx,lp,lx,&isctbuf) < 1) break ;
		 ipt = &isctbuf ;
	       } ;
	      if (ipt->PntType == V4DPI_PntType_TagVal) 		/* Allow for all (may be from EchoS()) */
	       { switch (v4im_CheckPtArgNew(ctx,ipt,&cpt,NULL))
		  { default:
			if (intmodx == V4IM_OpCode_EchoS) break ;	/* No errors if coming from EchoS() */
			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; return(NULL) ;
		    case V4IM_Tag_Unk:
			if (intmodx == V4IM_OpCode_EchoS) break ;	/* No errors if coming from EchoS() */
			v_Msg(ctx,NULL,"TagUnknown",intmodx) ; return(NULL) ;
		    case V4IM_Tag_Out:
			opt = v4dpi_IsctEval(respnt,cpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
			if (opt == NULL) { v_Msg(ctx,NULL,"ModArgEval",intmodx,ix,cpt) ; return(NULL) ; } ;
			filex = vout_PntIdToFileX(ctx,(struct V4DPI__LittlePoint *)opt) ;
			if (filex == UNUSED) { v_Msg(ctx,NULL,"StreamNoOutput",intmodx,V4IM_Tag_Out,opt) ; return(NULL) ; } ;
			goto EchoT_nextarg ;
		    case V4IM_Tag_Column:
			v4im_GetPointUC(&ok,coldelim,sizeof coldelim,cpt,ctx) ; break ;
		    case V4IM_Tag_If:
			okToEcho = v4im_GetPointLog(&ok,cpt,ctx) ;
			if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,lx) ; return(NULL) ; } ;
			break ;
		    case V4IM_Tag_RCV:
			rcv = v4im_GetPointLog(&ok,cpt,ctx) ;
			break ;
		  } ;
		 if (lp == NULL) { break ; } else { continue ; } ;
	       } ;
	      if (needTab && okToEcho) { UCstrcat(obp,coldelim) ; obp += UCstrlen(coldelim) ; } ;
	      if (okToEcho)
	       { if (!v4sxi_SpecialDisplayer(ctx,ipt,V4TMBufMax,UCTBUF1)) v4dpi_PointToStringML(UCTBUF1,ipt,ctx,V4DPI_FormatOpt_Echo,V4TMBufMax) ;
		 UCstrcat(obp,UCTBUF1) ; obp += UCstrlen(UCTBUF1) ;
		 needTab = TRUE ;			/* Start output of tabs after first column */
	       } ;
	      if (baf != NULL)
	       { ++colNum ;			/* Update to current column */
		 v4im_BaAIncrement(ctx,baf,ipt,colNum,rcv) ;
	       } ;
EchoT_nextarg:
	      if (lp == NULL) break ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,lx) ; return(NULL) ; } ;
	if (okToEcho)
	 { if (filex == UNUSED) { vout_UCText(VOUT_Data,0,outbuf) ; vout_NL(VOUT_Data) ; }
	    else { vout_UCTextFileX(filex,0,outbuf) ; vout_NLFileX(filex) ; } ;
	 } ;
	logPNTv(respnt,TRUE) ;
	return(respnt) ;
}


struct V4DPI__Point *v4im_DoSSDim(ctx,respnt,argpnts,argcnt,intmodx,trace)
  struct V4C__Context *ctx ;
  P *respnt ;						/* Ptr for result */
  INTMODX intmodx ;
  P *argpnts[] ;
  COUNTER argcnt ; VTRACE trace ;
{ P *ipt,*cpt, isctbuf ;
  extern struct V4SS__DimToFormatMap *vdfm ;
  DIMID dimid ; INDEX ix,tx,vx ; LOGICAL first,ok ; UCCHAR tb[V4TMBufMax] ; double dnum ;
 
	for(ix=1,ok=TRUE;;)
	 { ipt = argpnts[ix] ;
	   if (ipt->Dim == Dim_Dim) { dimid = ipt->Value.IntVal ; }
	    else { switch (v4im_GetDictToEnumVal(ctx,ipt))
		    { default:		v_Msg(ctx,NULL,"SSDimName",intmodx,ipt) ; return(NULL) ;
		      case _Table:	dimid = V4SS_DimId_Table ; break ;
		      case _Row:	dimid = V4SS_DimId_Row ; break ;
		      case _Cell:	dimid = V4SS_DimId_Cell ; break ;
		      case _HdrRow:	dimid = V4SS_DimId_HdrRow ; break ;
		      case _HdrCell:	dimid = V4SS_DimId_HdrCell ; break ;
		      case _TopOfPage:	dimid = V4SS_DimId_TopOfPage ; break ;
		    } ;
		 } ;
	   if (vdfm == NULL)			/* Allocate structure if not already done */
	    { vdfm = (struct V4SS__DimToFormatMap *)v4mm_AllocChunk(sizeof *vdfm,TRUE) ; } ;
	   vdfm->RevLevel++ ;
	   for(vx=0;vx<vdfm->Count;vx++) { if (dimid == vdfm->dimIdList[vx]) break ; } ;
	   if (vx >= vdfm->Count)
	    { if (vdfm->Count >= V4SS__DimToFormat_Max) { v_Msg(ctx,NULL,"SSDimMaxDim",intmodx,V4SS__DimToFormat_Max) ; return(NULL) ; } ;
	      vx = (vdfm->Count++) ;
	      vdfm->dimIdList[vx] = dimid ;
	      vdfm->vfs[vx] = (struct V4SS__FormatSpec *)v4mm_AllocChunk(sizeof *vdfm->vfs[0],FALSE) ;
	    } ;
//VEH121019 - Used to do this only on new entry - want to do it each time we get Dim:xxxx to reset everything related to that dimension
	   v_ParseFormatSpecs(ctx,vdfm->vfs[vx],V4SS_FormatType_Init,NULL) ;
	   if (argcnt == 1)			/* No arguments other than dimension, then clear out everything */
	    { v4mm_FreeChunk(vdfm->vfs[vx]) ; vdfm->vfs[vx] = vdfm->vfs[--vdfm->Count] ; vdfm->dimIdList[vx] = vdfm->dimIdList[vdfm->Count] ; } ;
	   for(ix++,first=TRUE;ix<=argcnt;ix++,first=FALSE)		/* Step thru the remaining arguments */
	    { ipt = argpnts[ix] ;
	      if (ipt->Dim == Dim_NId) goto next_dim ;	/* Treat NId's as special dimensions (see check above) */
	      tx = v4im_CheckPtArgNew(ctx,argpnts[ix],&cpt,&isctbuf) ;
/*		      If first time for dimension clear out any old stuff (unless CSS/Mask tags) */
	      if (first && (!(tx == -V4IM_Tag_Mask || tx == V4IM_Tag_Mask || tx == -V4IM_Tag_Format || tx == -V4IM_Tag_CSS)))
		v_ParseFormatSpecs(ctx,vdfm->vfs[vx],V4SS_FormatType_Init,NULL) ;
	      switch (tx)
	       { default:		v_Msg(ctx,NULL,"TagBadUse",intmodx) ; return(NULL) ;
		 case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; return(NULL) ;
		 case V4IM_Tag_Dim:	goto next_dim ;
		 case V4IM_Tag_Font:	
		   v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; if (!ok) break ;
		   ok = v_ParseFormatSpecs(ctx,vdfm->vfs[vx],V4SS_FormatType_Font,tb) ; break ;
		 case V4IM_Tag_StyleName:
		 case V4IM_Tag_Style:
		   v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; if (!ok) break ;
		   ok = v_ParseFormatSpecs(ctx,vdfm->vfs[vx],V4SS_FormatType_Style,tb) ; break ;
		 case V4IM_Tag_Mask:
		 case V4IM_Tag_Format:
		   v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; if (!ok) break ;
		   ok = v_ParseFormatSpecs(ctx,vdfm->vfs[vx],V4SS_FormatType_Mask,tb) ; break ;
		 case V4IM_Tag_FontSize:
		   v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; if (!ok) break ;
		   ok = v_ParseFormatSpecs(ctx,vdfm->vfs[vx],V4SS_FormatType_Size,tb) ; break ;
		 case V4IM_Tag_CellColor:
		   v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; if (!ok) break ;
		   ok = v_ParseFormatSpecs(ctx,vdfm->vfs[vx],V4SS_FormatType_CellColor,tb) ; break ;
		 case V4IM_Tag_TextColor:
		   v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; if (!ok) break ;
		   ok = v_ParseFormatSpecs(ctx,vdfm->vfs[vx],V4SS_FormatType_TextColor,tb) ; break ;
		 case V4IM_Tag_PageBreak:
		   break ;
		 case V4IM_Tag_URL:
		   v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; if (!ok) break ;
		   ok = v_ParseFormatSpecs(ctx,vdfm->vfs[vx],V4SS_FormatType_URLLink,tb) ; break ;
		 case V4IM_Tag_Width:
		   dnum = v4im_GetPointDbl(&ok,cpt,ctx) ;
		   if (dnum < 0 || dnum > 150.0) { v_Msg(ctx,NULL,"ModArgRange",intmodx,ix,cpt,0,150) ; return(NULL) ; } ;
		   vdfm->vfs[vx]->Width = (int)(dnum < 1.0 ? dnum * 100 : dnum + 100) ;
		   break ;
		 case -V4IM_Tag_Mask:
		   if (vdfm->vfs[vx]->MaskX <= 0) { v_Msg(ctx,NULL,"FormatNoMask",intmodx,vdfm->dimIdList[vx]) ; return(NULL) ;} ;
		   ZPH(respnt) ; respnt->Dim = Dim_Alpha ; respnt->PntType = V4DPI_PntType_Char ; 
		   strcpy(&respnt->Value.AlphaVal[1],&vdfm->vfs[vx]->VarString[vdfm->vfs[vx]->MaskX-1]) ; CHARPNTBYTES1(respnt) ;
		   return(respnt) ;
		 case V4IM_Tag_CSS:
		   v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; if (!ok) break ;
		   ok = v_ParseFormatSpecs(ctx,vdfm->vfs[vx],V4SS_FormatType_CSSStyle,tb) ; break ;
		 case V4IM_Tag_Id:
		   v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; if (!ok) break ;
		   ok = v_ParseFormatSpecs(ctx,vdfm->vfs[vx],V4SS_FormatType_CSSId,tb) ; break ;
		 case V4IM_Tag_Class:
		   v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; if (!ok) break ;
		   ok = v_ParseFormatSpecs(ctx,vdfm->vfs[vx],V4SS_FormatType_CSSClass,tb) ; break ;
		 case -V4IM_Tag_CSS:			/* Not a format - want to return CSS entries corresponding to this format */
		   v_VFStoCSS(ctx,vdfm->vfs[vx],UCTBUF1,UNUSED) ;
		   { COUNTER len ;
		     if ((len=UCstrlen(UCTBUF1)) >= V4DPI_UCVAL_MaxSafe) { v_Msg(ctx,NULL,"PointMaxSize",intmodx,len,V4DPI_UCVAL_MaxSafe,V4DPI_PntType_UCChar) ; return(NULL) ; } ;
		   }
		   uccharPNTv(respnt,UCTBUF1) ;
		   return(respnt) ;
	       } ;
	      if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ix-1) ; return(NULL) ; } ;
	    } ;
next_dim:  if (ix >= argcnt) break ;
	 } ;
	logPNTv(respnt,TRUE) ;
	return(respnt) ;
}

INDEX v4im_SSDimSetMask(ctx,di,mask,intmodx)
  struct V4C__Context *ctx ;
  struct V4DPI__DimInfo *di ;
  UCCHAR *mask ;
  INTMODX intmodx ;
{
  INDEX dimFX ;
  
	if (vdfm == NULL)			/* Allocate structure if not already done */
	 { vdfm = (struct V4SS__DimToFormatMap *)v4mm_AllocChunk(sizeof *vdfm,TRUE) ; } ;
	for(dimFX=0;dimFX<vdfm->Count;dimFX++) { if (di->DimId == vdfm->dimIdList[dimFX]) break ; } ;
	vdfm->RevLevel++ ;
	if (dimFX >= vdfm->Count)
	 { if (vdfm->Count >= V4SS__DimToFormat_Max) { v_Msg(ctx,NULL,"SSDimMaxDim",intmodx,V4SS__DimToFormat_Max) ; return(UNUSED) ; } ;
	   dimFX = (vdfm->Count++) ; vdfm->dimIdList[dimFX] = di->DimId ;
	   vdfm->vfs[dimFX] = (struct V4SS__FormatSpec *)v4mm_AllocChunk(sizeof *vdfm->vfs[0],FALSE) ;
	   v_ParseFormatSpecs(ctx,vdfm->vfs[dimFX],V4SS_FormatType_Init,NULL) ;
	 } ;
	v_ParseFormatSpecs(ctx,vdfm->vfs[dimFX],V4SS_FormatType_Mask,di->OutFormat) ;
	di->rtFlags |= V4DPI_rtDimInfo_v4rpp ;	/* Once is enough, set flag so we don't repeat this a gazillion times */
	return(dimFX) ;
}

/*	v4im_DoZip - Handles Zip() module		*/

enum zipActions { none, create, append, zclose } ;
#define LCL_OPENZIP_MAX 10
struct lcl__OpenZips {
  BYTE *zbuf ; size_t zbufLen ; LOGICAL zIsText ; UDTVAL udt ;
  UCCHAR zipName[V_FileName_Max] ;
  struct V4DPI__LittlePoint *idPt ;
  enum zipActions zAct ;
  INDEX zx ;			/* If not UNUSED then active entry below */
  COUNTER num ;
  struct {
    struct V4DPI__LittlePoint idPt ;
    struct VZIP__Master *vzm ;
   } entry[LCL_OPENZIP_MAX] ;
} ;

/*	v_ZipHandler - performs action & updates respnt with result (file name or compression ratio) */
LOGICAL v_ZipHandler(ctx,loz,lastArg,respnt)
  struct V4C__Context *ctx ;
  struct lcl__OpenZips *loz ;
  LOGICAL lastArg ;
  P *respnt ;
{
	switch (loz->zAct)
	 { default:
	     if (loz->idPt != NULL)
	      { for(loz->zx=loz->num;loz->zx >= 0;loz->zx--) { if (memcmp(loz->idPt,&loz->entry[loz->zx].idPt,loz->idPt->Bytes) == 0) break ; } ;
		if (loz->zx < 0) { v_Msg(ctx,ctx->ErrorMsgAux,"ZIPNoSuchId",loz->idPt) ; return(FALSE) ; } ;
		loz->idPt = NULL ;
	      } ;
	     if (loz->zx < 0)
	      { if (loz->num == 1) { loz->zx = 0 ; }	/* Default to first if that is the only one */
		 else { if (!lastArg) return(TRUE) ;	/* If not last argument then maybe last one will identify */
		        v_Msg(ctx,ctx->ErrorMsgAux,"ZIPNoId",V4IM_Tag_Id,loz->num) ; return(FALSE) ;
		      } ;
	      } ;
	     break ;
	   case none:
	   case create:
	     break ;
	 } ;
	switch (loz->zAct)
	 { case append:
	     if (UCempty(loz->zipName))
	      { if (!lastArg) return(TRUE) ;
	        v_Msg(ctx,ctx->ErrorMsgAux,"ZIPNoName") ; return(FALSE) ;
	      } ;
	     { double compRatio ; size_t bytes = vzip_AppendFile(loz->entry[loz->zx].vzm,loz->zipName,loz->zbufLen,loz->zbuf,NULL,loz->zIsText,loz->udt) ;
	       compRatio = (double)bytes / (double)loz->zbufLen ;
	       dblPNTv(respnt,compRatio) ;
	     }
	     loz->zbuf = NULL ; ZUS(loz->zipName) ;
	     break ;
	   case zclose:
	     uccharPNTv(respnt,loz->entry[loz->zx].vzm->filename) ;
	     if (!vzip_CloseZipFile(loz->entry[loz->zx].vzm,ctx->ErrorMsgAux)) return(FALSE) ;
	     loz->entry[loz->zx] = loz->entry[loz->num-1] ; loz->num-- ; loz->zx = UNUSED ;
	     break ;
	   case create:
	     if (loz->num >= LCL_OPENZIP_MAX) { v_Msg(ctx,ctx->ErrorMsgAux,"ZIPTooMany",LCL_OPENZIP_MAX,V4IM_OpCode_Zip) ; return(FALSE) ; } ;
	     loz->entry[loz->num].vzm = vzip_InitZipFile(loz->zipName,ctx->ErrorMsgAux) ;
	     if (loz->entry[loz->num].vzm == NULL) return(FALSE) ;
	     loz->zx = loz->num ; loz->num++ ;
	     if (loz->idPt != NULL)
	      { memcpy(&loz->entry[loz->zx].idPt,loz->idPt,loz->idPt->Bytes) ; loz->idPt = NULL ; } else { loz->entry[loz->zx].idPt.Bytes = 0 ; } ;
	     uccharPNTv(respnt,loz->entry[loz->zx].vzm->filename) ;
	     break ;
	   case none:
	     break ;
	 } ;
	loz->zAct = none ;
	return(TRUE) ;
}

P *v4im_DoZip(ctx,respnt,argpnts,argcnt,intmodx)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  P *respnt,*argpnts[] ;
  COUNTER argcnt ;
{ P *cpt, spnt ;
  static struct lcl__OpenZips *loz = NULL ;
  UCCHAR zipFile[V_FileName_Max] ;
  INDEX ix,tx ; LOGICAL ok ;
  
	if (loz == NULL) loz = (struct lcl__OpenZips *)v4mm_AllocChunk(sizeof *loz,TRUE) ;
	loz->idPt = NULL ; ZUS(loz->zipName) ; loz->zAct = none ; loz->zx = UNUSED ; loz->zbuf = NULL ; ZUS(zipFile) ;
	for(ok=TRUE,ix=1;ok && ix<=argcnt;ix++)
	 { if (!(argpnts[ix]->PntType == V4DPI_PntType_TagVal || argpnts[ix]->Dim == Dim_Dim))
	    { LENMAX ucLen,utf8Bytes ;
/*	      Flush out any outstanding 'command' */
	      if (!v_ZipHandler(ctx,loz,ix>=argcnt,respnt)) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,argpnts[ix]) ; goto fail ; } ;
	      if (loz->zAct != none) { v_Msg(ctx,NULL,"ZIPPriorAct") ; goto fail ; } ;
	      v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,argpnts[ix],ctx) ; if (!ok) break ;
	      ucLen = UCstrlen(UCTBUF1) ; utf8Bytes = (ucLen * 1.5) ; loz->zbuf = (char *)v4mm_AllocChunk(utf8Bytes,FALSE) ;
	      loz->zbufLen = UCUTF16toUTF8(loz->zbuf,utf8Bytes,UCTBUF1,ucLen) ;
	      loz->zIsText = TRUE ; loz->zAct = append ; loz->udt = 0 ;
	      continue ;
	    } ;
	   switch (tx=v4im_CheckPtArgNew(ctx,argpnts[ix],&cpt,&spnt))
	    { default:				v_Msg(ctx,NULL,"TagBadUse",intmodx) ; goto fail ;
	      case V4IM_Tag_Unk:		v_Msg(ctx,NULL,"TagUnknown",intmodx) ; goto fail ;
	      case V4IM_Tag_Data:
	      case V4IM_Tag_Text:
		v4im_GetPointUC(&ok,loz->zipName,UCsizeof(loz->zipName),cpt,ctx) ; if (!ok) break ;
		{ struct stat statbuf ; struct UC__File UCFile ;
		  if (!v_UCFileOpen(&UCFile,loz->zipName,UCFile_Open_ReadBin,TRUE,ctx->ErrorMsgAux,0))
		   { v_Msg(ctx,NULL,"ZIPFileError",intmodx,ix,argpnts[ix]) ; goto fail ; } ;
		  fstat(UCFile.fd,&statbuf) ; loz->zbufLen = statbuf.st_size ;		/* Get size of the file */
		  loz->udt = statbuf.st_mtime - (60*gpi->MinutesWest) - TIMEOFFSETSEC ;
		  loz->zbuf = v4mm_AllocChunk(loz->zbufLen,FALSE) ; fread(loz->zbuf,1,loz->zbufLen,UCFile.fp) ;
		  v_UCFileClose(&UCFile) ;
		  loz->zIsText = (tx == V4IM_Tag_Text) ; loz->zAct = append ;
		}
		break ;
	      case -V4IM_Tag_Close:
		loz->zAct = zclose ;
		if (loz->zx != UNUSED)
		 { if (!v_ZipHandler(ctx,loz,ix>=argcnt,respnt)) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,argpnts[ix]) ; goto fail ; } ;
		 } ;
		break ;
	      case V4IM_Tag_Create:
		if (loz->zx != UNUSED) { v_Msg(ctx,NULL,"ZIPAlreadyOpn") ; goto fail ; } ;
		v4im_GetPointUC(&ok,loz->zipName,UCsizeof(loz->zipName),cpt,ctx) ; if (!ok) break ;
		loz->zAct = create ;
		break ;
	      case V4IM_Tag_Id:
		if (cpt->Bytes > sizeof *loz->idPt)
		 { v_Msg(ctx,NULL,"DimTooBig",intmodx,cpt) ; goto fail ; } ;
		loz->idPt = (struct V4DPI__LittlePoint *)cpt ;
		if (!v_ZipHandler(ctx,loz,ix>=argcnt,respnt)) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,argpnts[ix]) ; goto fail ; } ;
		continue ;
	      case V4IM_Tag_Open:
		v4im_GetPointFileName(&ok,zipFile,UCsizeof(zipFile),cpt,ctx,NULL) ; if (!ok) break ;
		break ;
	      case -V4IM_Tag_ListOf:
	       {struct UC__File UCFile ; struct stat statbuf ; off_t fpos ;
	        struct VZIP__EndCentralDir *vec = NULL ; struct VZIP__CentralDirFileHeader *vcd,*vcd1 ;
		struct V4L__ListPoint *lp ;
		char zipEnd[1000] ; INDEX i ;
		if (UCempty(zipFile)) { v_Msg(ctx,NULL,"TagMissing2",intmodx,V4IM_Tag_Open,-V4IM_Tag_ListOf) ; goto fail ; } ;
		if (!v_UCFileOpen(&UCFile,zipFile,UCFile_Open_ReadBin,TRUE,ctx->ErrorMsgAux,intmodx))
		 { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; return(NULL) ; } ;
		fstat(UCFile.fd,&statbuf) ;			/* Get size of the file */
		fpos = statbuf.st_size - sizeof zipEnd ;
		lseek(UCFile.fd,fpos,SEEK_SET) ;
		fread(zipEnd,1,sizeof zipEnd,UCFile.fp) ;	/* Read last section of file */
		for(i=sizeof zipEnd-1;i>=0;i--)			/* Scan backwards from end looking for special tag denoting begin of end-central-directory block */
		 { if (zipEnd[i] != 0x06) continue ;
		   if (zipEnd[i-1] != 0x05) continue ; if (zipEnd[i-2] != 0x4b) continue ; if (zipEnd[i-3] != 0x50) continue ;
		   vec = (struct VZIP__EndCentralDir *)&zipEnd[i-3] ; break ;
		 } ;
		if (vec == NULL)
		 { v_Msg(ctx,NULL,"ZIPNotZip",intmodx,zipFile) ; goto fail ; } ;
		vcd1 = (struct VZIP__CentralDirFileHeader *)v4mm_AllocChunk(vec->bytesCentralDir,FALSE) ;
		vcd = vcd1 ;
		fpos = vec->offsetCentralDir ; lseek(UCFile.fd,fpos,SEEK_SET) ;
		fread(vcd,1,vec->bytesCentralDir,UCFile.fp) ;	/* vec gives us starting position for main directory - read it in */
		INITLP(respnt,lp,Dim_List) ;
		for(i=0;i<vec->totalCDR;i++)			/* Go through all entries in central directory */
		 { struct V4L__ListPoint *lp1 ;
		   P iList, tpnt ;
		   INITLP(&iList,lp1,Dim_List) ;
		   alphaPNTvl(&tpnt,vcd->filename,vcd->bytesfilename) ; v4l_ListPoint_Modify(ctx,lp1,V4L_ListAction_Append,&tpnt,0) ;
		   intPNTv(&tpnt,vcd->bytesUncompressed) ; v4l_ListPoint_Modify(ctx,lp1,V4L_ListAction_Append,&tpnt,0) ;
		   intPNTv(&tpnt,mscu_msdos_to_udt(((vcd->fileModDate<<16) | vcd->fileModTime))) ; tpnt.Dim = Dim_UDT ; tpnt.PntType = V4DPI_PntType_UDT ;
		    v4l_ListPoint_Modify(ctx,lp1,V4L_ListAction_Append,&tpnt,0) ;
		   intPNTv(&tpnt,vcd->bytesCompressed) ; v4l_ListPoint_Modify(ctx,lp1,V4L_ListAction_Append,&tpnt,0) ;
/*		   Append inner list: (filename bytes updateDT compressedBytes) */
		   ENDLP(&iList,lp1) ; v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&iList,0) ;
		   vcd = (struct VZIP__CentralDirFileHeader *)((char *)vcd + sizeof *vcd + vcd->bytesfilename + vcd->bytesExtra + vcd->bytesComments) ;
		 } ; ENDLP(respnt,lp) ;
		v4mm_FreeChunk(vcd1) ; v_UCFileClose(&UCFile) ;
		return(respnt) ;
	       }
	      case V4IM_Tag_Name:
		v4im_GetPointUC(&ok,loz->zipName,UCsizeof(loz->zipName),cpt,ctx) ; if (!ok) break ;
		if (!v_ZipHandler(ctx,loz,ix>=argcnt,respnt)) { v_Msg(ctx,NULL,"ModInvArg2",intmodx,argpnts[ix]) ; goto fail ; } ;
		continue ;
	    } ;
	 } ;
	if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ix-1) ; goto fail ; } ;
/*	What did we get (what do we have to do to clean up & finish) */
	if (loz->zAct != none)
	 { if (!v_ZipHandler(ctx,loz,TRUE,respnt))
	   { v_Msg(ctx,NULL,"ModInvArg2",intmodx,argpnts[argcnt]) ; goto fail ; } ;
	 } ;
	return(respnt) ;

fail:
	return(NULL) ;
}

/*	D E F I N E   S P I N L O C K   F O R   R A S P P I		*/

#if defined RASPPI && defined NOTQUITEYET

static inline void arch_spin_lock(arch_spinlock_t *lock)
{
	unsigned long tmp;
	__asm__ __volatile__(
1: "1:	ldrex	%0, [%1]\n"
2: "	teq	%0, #0\n"
3:	WFE("ne")
4: "	strexeq	%0, %2, [%1]\n"
5: "	teqeq	%0, #0\n"
6: "	bne	1b"
 	: "=&r" (tmp)
	: "r" (&lock->lock), "r" (1)
	: "cc");
7:	smp_mb();
}

#define RLSSPINLOCK(LOCKVAR)
static inline void arch_spin_unlock(arch_spinlock_t *lock)
{
1:	smp_mb();
	__asm__ __volatile__(
2:"	str	%1, [%0]\n"
	:
	: "r" (&lock->lock), "r" (0)
	: "cc");
	
3:	dsb_sev();
}
#endif