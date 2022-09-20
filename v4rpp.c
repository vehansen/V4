/*	v4rpp.c - Next Generation V4R -> HTML/Excel Postprocessor		*/

/*	To compile on linux
		cc -m32 -O3 -o xv4rpp -DLINUX486 -DINCLUDE_VCOMMON -w v4rpp.c vcommon.c -lm
*/

#define V4RPP_Version_Major 1
#define V4RPP_Version_Minor 54

/*
File		Ver	Who	When		What
--------------	-----	------	------------	-----------------------------------------------------------------------------------
v4rpp.c		1.54	VEH	8/17/22		Added _DF (dfltFormat) for initial formatting options in HTML output
v4rpp.c		1.53	VEH	5/19/22		Fixed issue with (Canadian) date format for XLS/XLSX output
v4rppxlsx.c	1.52	VEH	5/10//22	Format date-time - if ymd format then change width from 15 to 17
v4rpp.c		1.51	VEH	5/8/22		Added country/language support (_LA<country-code>)
v4rpp.c		1.50	VEH	2/24/22		Added -P switch
v4rpp.v4i	1.50	VEH	2/24/22		Support for -P switch
v4rpp.c		1.49	VEH	8/12/20		Bug in output <link>, switched to 64 bit
v4rpptab.c	1.48	VEH	4/29/19		Added '-dt' & '-dc' switches to create tab/csv output files
v4rppxlsx.c	1.47	VEH	3/4/19		Added cutoff to number of 'invalid value' warnings output
xv4rpp.exe	1.46	VEH	11/6/18		Now building as 64-bit to handle larger V4R/Excel files
v4rpp.c		1.45	VEH	6/5/18		Added check of '-k sessionkey' to cookie session key in V4R (CKseskey=xxxxxx...)
v4rppxlsx.c	1.44	VEH	4/20/18		Added cutoff to warning message
v4rppxlsx.c	1.43	VEH	10/17/17	Increased size of SSH_INITIAL_TABLE to cut down on hash conflicts
v4rppxlsx.c	1.42	VEH	8/7/17		Problem with converting some URLs to XLSX format - needed to escape '&' in URL to '&amp;'
v4rpp.c		1.41	VEH	7/6/17		Fixed problem with colors that had leading zeros in hex value (ex: blue & green)
v4rppjson.c	1.40	VEH	3/10/17		Fixed bug in -J with urlBases - was double quoting
v4rppxlsx.c	1.39	VEH	2/22/17		Tweaked to add ht="xx" customHeight="1" if multi-line row
v4rppxlsx.c	1.38	VEH	11/3/16		Minor tweaks to handle 'bad' data from v4r's created by MIDAS
v4rpp.c		1.37	VEH	9/22/16		Added support for V4DPI_PntType_Delta
v4rpp.c		1.36	VEH	6/22/16		Changed command parsing so '-i xxx' converted to '-i xxxv4rpp.ini'
v4rppxlsx.c	1.36	VEH	6/22/16		Dynamically allocate/reallocate *vcs & *vhl when generating xlsx to get around limit
v4rpp.c		1.35	VEH	9/9/15		Changed handling of real numbers so large non-fractional values don't show in scientific notation
v4rppxlsx.c	1.34	VEH	7/20/15		Fixed bug in string hashing (was able to set hash index 1 slot too high)
v4rpp.c		1.33	VEH	7/14/15		Added LI (JS/CSS libraries) handling
v4rppxlsx.c	1.32	VEH	5/21/15		Changed sheet->scale to handle 'fit-to-page'
v4rpp.c		1.31	VEH	2/27/15		Modified v4rh_SetURLBase() to use v_StringLit() so we can have embedded single quotes
v4rpp.c		1.30	VEH	2/05/15		Added support for UPeriod
v4rppxlsx.c	1.29	VEH	9/18/14		Fixed zip init so only xls extension changed to xlsx
v4rppjson.c	1.28	VEH	6/2/14		Added Row::xxx (_KE) and Select::xxx (_SE) handlers 
v4rpp.c		1.27	VEH	1/13/13		Fixed bug in handling of commands within conditionals
v4rppjson	1.26	VEH	9/10/12		Fixed bug in URL handling - was chopping off first character of +xxxx link
v4rppxlsx.c	1.25	VEH	7/24/12		Took out '<colors><mruColors>...', added '<sheetPr><pageSetUpPr fitToPage=\"1\"/></sheetPr>'
v4rppxlsx.c	1.24	VEH	5/26/12		Added extra checks to prevent invalid data in cells (ex: alpha characters in integer field)
v4rpp.c		1.23	VEH	3/19/12		Added -J switch and associated routines to generate JSON verison of report
						Added KE (row key), and CN (column name) options
v4rpp.c		1.22	VEH	3/3/12		Got Footers to work properly
v4rpp.c		1.22	VEH	3/1/12		Fixed issue with multiple <thead>'s - now create single <thead> with multiple embedded <tr>'s
v4rpp.c		1.21	VEH	11/17/11	Fixed problem with cookieValues containing multiple cookies
v4rppxlsx.c	1.20	VEH	6/1/11		Fixed problem in v4rx_sshInsert() when increasing size of table (was not clearing out newly realloc() entries)
v4rpp.c		1.19	VEH	3/17/11		Added IB/IT (insert html bottom/top) and linked to v4
v4rpp.c		1.18	VEH	3/4/10		Added -q option
v4rpp.c		1.17	VEH	1/21/10		Added Indent1/2/3
v4rpp.c		1.16	VEH	12/27/10	Fixed problems with leading spaces & blank lines
vcommon.c	1.15	VEH	12/3/10		Finally got XLSX format to compress properly
v4rpp.c		1.14	VEH	11/21/10	Tweaked memory allocation for xlsx, do not output '<xxx...' & '</xxx>' tags on HTML
v4rpp.c		1.13	VEH	7/8/10		Added XLSX support, handle _IM: (image) tag for both HTML & Excel
v4rpp.c		1.12	VEH	5/10/10		Added -D switch, fixed problem with uninitiallized variable
v4rpp.c		1.11	VEH	2/03/10		Added support for reserved symbol 'cookieValues'
v4rpp.c		1.9	VEH	11/18/09	Moved v_ParseDbl/Int/Log to vcommon
v4rpp.c		1.9	VEH	11/10/09	Fixed problem in v_ParseDbl() with numbers like "1E-07"
v4rpp.c		1.8	VEH	11/3/09		Added TA (target window) option to work with UR (url)
v4rpp.c		1.7	VEH	10/22/09	Added support for shell dimensions in CV entry
*/

#include "v4defs.c"
#include "vexcel.c"
#include <sys/types.h>
#include <sys/stat.h>
#ifdef UNIX
#include <unistd.h>
#endif

extern struct V__UnicodeInfo *uci ;
struct V4C__ProcessInfo *gpi  ;		/* Global process information */

#define V4RH_DimNum_Int 0		/* dimNum's for standard types */
#define V4RH_DimNum_Alpha 1
#define V4RH_DimNum_Date 2
#define V4RH_DimNum_Logical 3
#define V4RH_DimNum_Float 4
#define V4RH_DimNum_XL 5		/* For 'special' excel values (ex: div0) */
#define V4RH_DimNum_MIDASUDT 6		/* MIDAS UDT integer value */

#define DEFTAG(NAME) ok = (v4rh_neUpdate(vrhm,UClit(NAME),V4RH_ETYPE_Tag,arg,V4RH_UPDOPER_Update) != NULL) ; break ;
#define DELTAG(NAME) ok = (v4rh_neUpdate(vrhm,UClit(NAME),V4RH_ETYPE_Tag,UClit(""),V4RH_UPDOPER_Delete) != NULL) ;

UCCHAR lHdr[64] ;


#define V4RH_INIFILE_MAX 10
#define V4RH_VRNE_MAX 1009
#define V4RH_SCRIPT_MAX 512
#define V4RH_VDI_MAX 100
#define V4RH_DIM_MAX 100
#define V4RH_CSS_MAX 100
#define V4RH_URLBASE_MAX 32
#define V4RH_EMBED_HTML_MAX 16
#define V4RH_INCLUDE_MAX 16
#define V4RH_LINK_MAX 64

enum V4R_OutputFormat { html, xls, xlsx, json, tab, csv } ;

struct V4RH__Main {
  struct V4XLSX__Main *xlsx ;			/* Structure for generating xlsx file */
  UCCHAR *inpFile,*outFile ;			/* Input & output files */
  ETYPE outEncoding ;				/* How output file to be encoded (default is V4L_TextFileType_ASCII) */
  struct V4LEX__TknCtrlBlk *tcb ;		/* Token control block for various inputs */
  struct UC__File iFile ;			/* Input File */
  LOGICAL minimizeHTML ;			/* If TRUE then use shortcuts to minimize HTML output */
  LOGICAL nextPass ;				/* If TRUE then stop current pass & immediately go to next one (for error 'aborting') */
  LOGICAL gotEOF ;				/* If TRUE then got BSEOF in input (i.e. report finished normally) */
  LOGICAL gotError ;				/* If TRUE then got BSErr section */
  LOGICAL v4rIsHTML ;				/* If TRUE then the V4R file (input) appears to be HTML already */
  LOGICAL forPrinting ;				/* If TRUE then format HTML for printing rather than screen display */
  //LOGICAL inputIsText ;				/* If TRUE then input is not V4R but text (report image) */
  enum V4R_OutputFormat outFormat ;		/* Output format (default is html) */
  LOGICAL wantJSON ;				/* If TRUE then generating JSON file */
  LOGICAL iLineLookAhead ;			/* If true then re-scan current line */
  LOGICAL pageBreak ;				/* Global variable set to TRUE on page break (must be reset after handling) */
  INDEX outFileId ;				/* Output file Id */
  INDEX outFileX ;				/* Output file */
  UCCHAR errMessage[1024] ;
  UCCHAR tagToIgnore[V4RH_Name_Max+1] ;		/* If set then ignore html tag of this type */
  ETYPE dfltOutFormat ;				/* Default output format for report (0=Standard, -1=DataTable, n=#lines) */
  COUNTER sheetNum ;				/* Number of sheets */
  struct V4RH__Sheet *sheetFirst ;		/* Pointer to first sheet */
  struct V4RH__Sheet *sheetCur ;		/* Current sheet */
  struct V4RH__Cell *cellCur ;			/* Current cellFirst */
  INDEX vrneCount ;				/* Number of Named Entries */
  INDEX vrneMax ;				/* Max number */
  struct V4RH__NamedEntry *vrne[V4RH_VRNE_MAX] ;
  COUNTER incNum ;				/* Number of include files below */
  UCCHAR *incFileName[V4RH_INCLUDE_MAX] ;	/* (HTML ONLY) Name of file to be included at end */
  COUNTER linkNum ;				/* Number of JS/CSS sources to link to */
  UCCHAR *linkURL[V4RH_LINK_MAX] ;		/* Name of link file */
  INDEX scriptNum ;				/* Number of script entries below */
  UCCHAR *script[V4RH_SCRIPT_MAX] ;		/* (Java)script to be included in ##js.internal## */
  struct V4RH__CSSEntries *vce ;		/* Pointer to CSS entries */
  COUNTER cssNum ;				/* Number of other css entries below */
  UCCHAR *cssLine[V4RH_CSS_MAX] ;
  struct V4RH__DimInfo *vdi[V4RH_DIM_MAX] ;	/* Pointers to dimension links */
  struct V4RH__MaskInfo *vmi ;
  UCCHAR *urlBases[V4RH_URLBASE_MAX] ;		/* Points to base URL strings */
  struct ss__WorkBook *wb ;			/* Workbook if creating Excel */
  COUNTER readHTMLX ;				/* Current HTML index (below) for reading */
  COUNTER writeHTMLX ;				/* Current for writing (i.e. slot for next one to be written) */
  UCCHAR *embedHTML[V4RH_EMBED_HTML_MAX] ;	/* Pointers to HTML to embed in something */
  struct ss__Format *dateformat ;		/* Special format for Excel */
  struct ss__Format *dtFormat ;			/* Special format for Excel */
  struct ss__Format *defaultformat ;		/*  ditto */
  double epsilon ;				/* Any FABS(floating number) smaller than this is to be considered exactly 0 */
  ETYPE xlsxURL ;				/* How embedded URLs are to be handled in XLSX files (see V4RH_XLSXURL_xxx) */
  UCCHAR *dirXLSXLclURLs ;			/* If not NULL then directory path for local 'secure' URL information file */
  COUNTER xlsxLifeInDays ;			/* Lifetime of secure links (days), <= 0 then take default */
} ;

struct V4RH__DimInfo {
  struct V4RH__FormatInfo *vfi ;		/* Format info (if not NULL) */
  struct ss__Format *ssf ;			/* Pointer to corresponding Excel/BIFF format */
  INDEX dimNum ;
  PNTTYPE v4PntType ;				/* V4 pointtype - V4DPI_PntType_xxx */
  INDEX cssX ;					/* Index (if not UNUSED) into vrhm->vce->css[] */
  INDEX maskX ;					/* Index (if not UNUSED) into vrhm->vme->mask[] */
} ;

#define	V4RH_MASK_MAX 100
struct V4RH__MaskInfo {
  INDEX numMask ;
  struct {
    UCCHAR *formatMask ;
   } mask[V4RH_MASK_MAX] ;
} ;

struct V4RH__FormatInfo {
  ETYPE border ;				/* Type of border - V4SS_Border_xxx */
  UCCHAR htmlClass[V4RH_Name_Max+1] ;
  UCCHAR htmlId[V4RH_Name_Max+1] ;
  UCCHAR *cssStyle ;
  UCCHAR *fontName ;
  ETYPE bgColor ;				/* Background color (V4 color code) */
  ETYPE textColor ;				/* Text color */
  FLAGS32 fontAttr ;				/* Font attributes (V4SS_Font_xxx) */
  LENMAX fontSize ;
  ETYPE horAlign ;				/* Horizontal alignment (V4SS_Align_xxx) */
  ETYPE vertAlign ;
  LENMAX colWidth ;				/* Column width */
  LENMAX colSpan ;				/* Column span */
} ;

#define V4RH_CSS_Max 200			/* Max number of CSS entries */
struct V4RH__CSSEntries {
  INDEX numCSS ;				/* Number of entries below */
  INDEX muX ;					/* Index to most-used CSS entry */
  struct {
    UCCHAR *cssDef ;				/* Definition of this entry: { xxx; yyy; zzz; ... } */
    COUNTER refCount ;				/* Number of times this entry referenced */
    UCCHAR *className ;				/* Class name for this entry, if NULL after first pass then auto-assigned */
    UCCHAR *jsonAttr ;				/* If creating JSON output then this has corresponding JSON attributes */
   } css[V4RH_CSS_Max] ;
} ;

#define V4RH_Max_Titles 20			/* Max number of title lines per sheet */
struct V4RH__Sheet {
  UCCHAR sheetName[V4RH_Name_Max+1] ;		/* Sheet name */
  struct V4RH__Sheet *sheetNext ;		/* Pointer to next sheet, NULL if this is last */
  INDEX titleNum ;				/* Number of titles below */
  INDEX titleX ;				/* Current title Index */
  INDEX initialLevel ;				/* Initial hierarchy to display */
  UCCHAR *ucTitles[V4RH_Max_Titles] ;		/* Each of the title lines */
  UCCHAR *selectInfo ;				/* Any row-selection routine to call for this sheet */
  UCCHAR *colIds ;				/* If not NULL then string of pipe-delimited column names */
  struct V4RH__Column *colFirst ;		/* Pointer to first column */
  struct V4RH__Row *hdrFirst ;			/* Pointer to first header row */
  struct V4RH__Row *hdrCur ;			/* Current header row */
  struct V4RH__Row *rowFirst ;			/* Pointer to first row */
  struct V4RH__Row *rowCur ;			/* Current row */
  struct V4RH__Row *ftrFirst ;			/* Pointer to first footer */
  struct V4RH__Row *ftrCur ;			/* Current footer */
  LENMAX bottomMax ;				/* Max number of characters allocated in bottomStr */
  UCCHAR *bottomStr ;				/* If not NULL then append '\r' delimited string to end of HTML report/table */
  LENMAX topMax ;				/*   ditto for top */
  UCCHAR *topStr ;
  INDEX rowNum ;				/* Current row number */
  INDEX hdrNum ;				/* Number of header lines */
  UCCHAR *sheetInfo ;
  double marginTop,marginBottom,marginLeft,marginRight ;
  ETYPE gridLines ;
  COUNTER colCap,rowCap ;			/* If > 0 then number of caption rows (colCap) or columns (rowCap) */
  ETYPE orientPage ;				/* -1 = landscape, 1 = portrait, 0 = undefined */
  double scale ;
} ;

struct V4RH__Column {
  struct V4RH__Column *colNext ;
  INDEX colIndex ;				/* Column number/index */
  INDEX dimNumMax ;				/* Index to dimNum below with highest usage in column */
  LENMAX width ;				/* Column width (used when creating XL file */
  unsigned short dimNumCount[V4RH_DIM_MAX] ;	/* Number of times a particular dimId found for this column */
} ;

#define V4RH_RowType_Header 1			/* Row type */
#define V4RH_RowType_Data 2
#define V4RH_RowType_Footer 3

struct V4RH__Row {
  struct V4RH__Row *rowNext ;			/* Pointer to next row, NULL if this is the last */
  struct V4RH__Cell *cellFirst ;		/* Pointer to first cell in the row */
  struct V4RH__Cell *cellCur ;			/* Pointer to current cell in the row */
  UCCHAR *rowKey ;				/* If non-NULL then the 'key' value(s) associated with this row */
  LOGICAL skipRow ;				/* Row is to be ignored (via 'IFcondition') */
  COUNTER level ;				/* Hierarchy level */
  LOGICAL levelAtEnd ;				/* TRUE if level row is at end of children, FALSE if at begin of children */
  INDEX cellNum ;				/* Cell (column) number */
  ETYPE rowType ;				/* Type of row- V4RH_RowType_xxx */
  UCCHAR *rowInfo ;				/* Pointer to any/all information about this row */
  INDEX lineNum ;
} ;

struct V4RH__Cell {
  struct V4RH__Cell *cellNext ;			/* Pointer to the next cellFirst */
//  ETYPE cellType ;				/* Data-type of the cellFirst */
  COUNTER colSpan ;				/* If <> 0 then number of columns cell spans */
  INDEX dimNum ;				/* 'dimension' format index */
  PNTTYPE pntType ;				/* If not UNUSED - Cell point type - overrides implicit point type of dimNum */
  UCCHAR *cellValue ;				/* Pointer to cellFirst's value */
  UCCHAR *cellFormula ;				/* If not NULL then a formula associated with the cell */
  UCCHAR *cellInfo ;				/* Pointer to any/all information about the cell */
  INDEX lineNum ;
  LOGICAL useColGroup ;				/* If TRUE then using column-group for style */
} ;

#define V4RH_UPDOPER_Update 1			/* Update entry with new value */
#define V4RH_UPDOPER_Append 2			/* Append this value to end of current */
#define V4RH_UPDOPER_Delete 3			/* Remove the entry */

struct V4RH__NamedEntry {
  UCCHAR eName[V4RH_Name_Max+1] ;		/* Section name */
  ETYPE eType ;					/* Type of entry (V4RH_ETYPE_xxx) */
  CCOUNT maxLenVal ;				/* Max allocated length */
  UCCHAR *eVal ;				/* Pointer to entry 'value' */
} ;

/*	Prototypes	*/
void v4rh_ExitStats() ;
struct V4RH__NamedEntry *v4rh_neLookup(struct V4RH__Main *,UCCHAR *) ;
struct V4RH__NamedEntry *v4rh_neUpdate(struct V4RH__Main *,UCCHAR *,ETYPE,UCCHAR *,ETYPE) ;
LOGICAL v4rh_ReadMaskFile(struct V4RH__Main *,UCCHAR *) ;
LOGICAL v4rh_XctSection(struct V4RH__Main *,UCCHAR *) ;
LOGICAL v4rh_SetCurSheet(struct V4RH__Main *,struct V4RH__Sheet *) ;
LOGICAL v4rh_SetCurTitle(struct V4RH__Main *) ;
LOGICAL v4rh_SetCurRow(struct V4RH__Main *,struct V4RH__Row *,ETYPE,UCCHAR *) ;
LOGICAL v4rh_SetCurCell(struct V4RH__Main *,struct V4RH__Row *,struct V4RH__Cell *,UCCHAR *) ;
void v4rh_ParseCellValue(struct V4RH__Main *,struct V4RH__Cell *,UCCHAR *,enum DictionaryEntries) ;
LOGICAL v4rh_XctIfNest(struct V4RH__Main *,struct V4LEX__TknCtrlBlk *,LOGICAL *) ;
LOGICAL v4rh_XctSectionLine(struct V4RH__Main *,UCCHAR *,UCCHAR *,INDEX) ;

ETYPE v4rh_ReadV4RPass1(struct V4RH__Main *) ;
LOGICAL v4rh_ReadToEndSection(struct V4RH__Main *,UCCHAR *,UCCHAR **) ;
LOGICAL v4rh_ReadColumnInfo(struct V4RH__Main *) ;
struct V4RH__DimInfo *v4rh_DefDimEntry(struct V4RH__Main *,INDEX,struct V4RH__FormatInfo *,INDEX,INDEX,PNTTYPE) ;
LOGICAL v4rh_ReadFormat(struct V4RH__Main *) ;
UCCHAR *v4rh_MakeCSSFromFormat(struct V4RH__Main *,struct V4RH__FormatInfo *) ;
INDEX v4rh_GetCSSIndex(struct V4RH__Main *,UCCHAR *) ;
INDEX v4rh_GetMaskIndex(struct V4RH__Main *,UCCHAR *) ;
LOGICAL v4rh_ReadError(struct V4RH__Main *) ;
LOGICAL v4rh_ReadMeta(struct V4RH__Main *) ;
LOGICAL v4rh_ReadOther(struct V4RH__Main *) ;
LOGICAL v4rh_ParseRow(struct V4RH__Main *,struct V4RH__Row *) ;
LOGICAL v4rh_ReadXLInfo(struct V4RH__Main *) ;
LOGICAL v4rh_Read(struct V4RH__Main *) ;
void v4rh_SetURLBase(struct V4RH__Main *,UCCHAR *) ;

LOGICAL v4rh_GenerateJS(struct V4RH__Main *) ;
LOGICAL v4rh_GenerateCSS(struct V4RH__Main *) ;
LOGICAL v4rh_GenerateColGroups(struct V4RH__Main *,struct V4RH__Sheet *) ;
LOGICAL v4rh_GenerateHTMLInclude(struct V4RH__Main *) ;
LOGICAL v4rh_GenerateIncludeBottom(struct V4RH__Main *) ;
LOGICAL v4rh_GenerateIncludeTop(struct V4RH__Main *) ;
LOGICAL v4rh_GenerateLinkInclude(struct V4RH__Main *) ;

void v4rx_FixUpString(UCCHAR *) ;
COUNTER v4rx_CreateCell(struct V4RH__Main *,INDEX,INDEX,struct V4RH__Row *,struct V4RH__Cell *,struct V4RH__Column *,struct ss__WorkSheet *) ;
struct ss__Format *v4rx_CreateXLFormat(struct V4RH__Main *,struct V4RH__FormatInfo *,UCCHAR *,LOGICAL) ;
void v4rx_GenerateXLS(struct V4RH__Main *vrhm,LOGICAL) ;
LOGICAL v4rx_GenerateXLSX(struct V4RH__Main *vrhm,LOGICAL) ;
INDEX v4rx_AddContentType(struct V4RH__Main *,UCCHAR *,UCCHAR *, UCCHAR *) ;

LOGICAL v4j_GenerateJSON(struct V4RH__Main *,LOGICAL) ;
LOGICAL v4t_GenerateTABCSV(struct V4RH__Main *) ;

LOGICAL wantTrace ;

/* More efficient allocUC routine */
int remain = 0 ;
char *memPtr = NULL ;
#define V4RPP_MEMCHUNKSIZE 0x10000

UCCHAR *v4rpp_AllocUC(bytes)
  int bytes ;
{ UCCHAR *uc ;

	bytes = (bytes + 1) * sizeof(UCCHAR) ;
	if (bytes > remain)
	 { memPtr = (char *)malloc(V4RPP_MEMCHUNKSIZE) ; remain = V4RPP_MEMCHUNKSIZE ;
	 } ;
	uc = (UCCHAR *)memPtr ;
	memPtr += bytes ; remain -= bytes ;
	return(uc) ;
}
void *v4rpp_AllocChunk(bytes,clear)
  int bytes ;
  LOGICAL clear ;
{ void *c ;

	if (bytes > remain)
	 { memPtr = (char *)malloc(V4RPP_MEMCHUNKSIZE) ; remain = V4RPP_MEMCHUNKSIZE ;
	 } ;
	c = memPtr ;
	memPtr += bytes ; remain -= bytes ;
	if (clear) memset(c,0,bytes) ;
	return(c) ;
}

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
{
  struct V4RH__Main vrhm ;
  INDEX ax ; LOGICAL wantCore,loop,wantQuiet ; COUNTER iniFileNum ;
  UCCHAR *arg, *iniFile[V4RH_INIFILE_MAX], sval, tbuf[512] ;
#define UCARGVMAX 50
  static UCCHAR *ucargv[UCARGVMAX] ;		/* Temp (hopefully) structure converting command line to Unicode */

	startup_envp = envp ;
#ifdef WINNT
  { int i ;
    for(i=0;argv[i]!=NULL&&i<UCARGVMAX-1;i++) { ucargv[i] = argv[i] ; } ; ucargv[i] = NULL ;
  }
#else
{ /* Convert arguments to Unicode */
  UCCHAR *uarg ; char *arg ; int i,j ;
  for(i=0;argv[i]!=NULL&&i<UCARGVMAX-1;i++)
   { int num = strlen(arg=argv[i]) ;
     uarg = (ucargv[i] = v4rpp_AllocUC(num)) ;
     for(j=0;;j++) { uarg[j] = arg[j] ; if (arg[j] == '\0') break ; } ;
   } ; ucargv[i] = NULL ;
}
#endif

	memset(&vrhm,0,sizeof vrhm) ; vrhm.vrneMax = V4RH_VRNE_MAX ; vrhm.outEncoding = V4L_TextFileType_ASCII ; vrhm.xlsxURL = V4RH_XLSXURL_none ;
	vrhm.outFormat = html ;
	gpi = v_GetProcessInfo() ;

#define CASEPP(LETTER,TAGNAME) \
 case UClit(LETTER): \
	ax++ ; if (ax >= argc) { printf("? Missing value for -%c option\n",LETTER) ; exit(EXITABORT) ; } ; \
	v4rh_neUpdate(&vrhm,UClit(TAGNAME),V4RH_ETYPE_Tag,argv[ax],V4RH_UPDOPER_Update) ;
   
	iniFileNum = 0 ; wantCore = TRUE ; wantTrace = FALSE ; wantQuiet = FALSE ;
	for(ax=1;ax<argc;ax++)
	 { arg = argv[ax] ;
	   if (*arg != UClit('-'))			/* Don't have a switch - then have file */
	    { if (vrhm.inpFile == NULL) { vrhm.inpFile = arg ; continue ; }
	       else if (vrhm.outFile == NULL) { vrhm.outFile = arg ; continue ; }
	       else { printf("? Too many files specified (argument #%d)\n",ax) ; exit(EXITABORT) ; } ;
	    } ;
	   sval = *(++arg) ;			/* Get the switch value */
	   switch (sval)
	    { default:	printf("? Invalid switch value (%c)\n",sval) ; exit(EXITABORT) ;
	      case UClit('c'):
		wantCore = FALSE ; break ;
	      case UClit('D'):
		wantTrace = TRUE ; break ;
	      case UClit('d'):
		switch(*(++arg))
		 { default:		printf("? Invalid switch 'd' value (%c), expecting -dc or -dt\n",*arg) ; exit(EXITABORT) ;
		   case UClit('c'):	vrhm.outFormat = csv ; break ;
		   case UClit('t'):	vrhm.outFormat = tab ; break ;
		 }
		break ;
	      case UClit('i'):
		ax++ ; if (ax >= argc) { printf("? Missing value for -i option\n") ; exit(EXITABORT) ; } ;
		if (iniFileNum >= V4RH_INIFILE_MAX) { printf("? Exceeded maximum (%d) number of initialization files\n",V4RH_INIFILE_MAX) ; exit(EXITABORT) ; } ;
		iniFile[iniFileNum] = v4rpp_AllocUC(UCstrlen(argv[ax])+32) ;
		UCstrcpy(iniFile[iniFileNum],argv[ax]) ;
		if (UCstrchr(iniFile[iniFileNum],'.') == NULL) UCstrcat(iniFile[iniFileNum],UClit("v4rpp.ini")) ;
		iniFileNum++ ; break ;
	      case UClit('h'):
		v_Msg(NULL,tbuf,"*V4RPPBanner",V4RPP_Version_Major,V4RPP_Version_Minor) ;vout_UCText(VOUT_Status,0,tbuf) ;
		if (!v_UCFileOpen(&vrhm.iFile,v_GetV4HomePath(UClit("v4rppHelp.v4i")),UCFile_Open_Read,TRUE,vrhm.errMessage,0))
		 goto fail ;
		for(;;)
		 { UCCHAR tbuf[512] ;
		   if (v_UCReadLine(&vrhm.iFile,UCRead_UC,tbuf,UCsizeof(tbuf),tbuf) < 0) break ;
		   if (tbuf[0] == UClit('!')) continue ; if (tbuf[0] == UClit('/')) continue ;
		   vout_UCText(VOUT_Status,0,tbuf) ;
		 } ; v_UCFileClose(&vrhm.iFile) ;
		exit(EXITOK) ;
	      CASEPP('j',"jobId")
		{ INDEX i=0,j,jn ;
		  for(jn=1;;jn++,i++)
		   { for(j=0;;i++,j++)
		      { if (argv[ax][i] == UCEOS || argv[ax][i] == '-')
		         { UCCHAR jname[32] ; UCsprintf(jname,UCsizeof(jname),UClit("jobId%d"),jn) ;
		           tbuf[j] = UCEOS ;
			   v4rh_neUpdate(&vrhm,jname,V4RH_ETYPE_Tag,tbuf,V4RH_UPDOPER_Update) ;
			   break ;
		         } ;
		        tbuf[j] = argv[ax][i] ;
		      } ;
		     if (argv[ax][i] == UCEOS) break ;
		   } ;
		}
		break ;
	      case UClit('J'):
		vrhm.outFormat = json ; break ;
	      CASEPP('k',"sessionKey") break ;
	      case UClit('l'):
		switch (*(++arg))
		 { default:		printf("? Invalid -l option\n") ; exit(EXITABORT) ;
		   case UClit('d'):
			ax++ ; if (ax >= argc) { printf("? Expecting directory path after '-ld'\n") ; exit(EXITABORT) ; } ;
			arg = argv[ax] ;
			vrhm.dirXLSXLclURLs = v4mm_AllocUC(UCstrlen(arg)+2) ; UCstrcpy(vrhm.dirXLSXLclURLs,arg) ;
#ifdef WINNT
/*			Make sure path ends with '\' so when we append filename we still have a good path */
			if (vrhm.dirXLSXLclURLs[UCstrlen(vrhm.dirXLSXLclURLs)-1] != UClit('\\')) UCstrcat(vrhm.dirXLSXLclURLs,UClit("\\")) ;
#endif
			break ;
		   case UClit('e'):	vrhm.xlsxURL = V4RH_XLSXURL_exOnly ; break ;
		   case UClit('l'):
			ax++ ; if (ax >= argc) { printf("? Expecting lifetime (days) after '-ll'\n") ; exit(EXITABORT) ; } ;
			{ UCCHAR *tc ; vrhm.xlsxLifeInDays = v_ParseInt(argv[ax],&tc,10) ;
			  if (*tc != UCEOS) { printf("? Expecting integer lifetime (days) after '-ll'\n") ; exit(EXITABORT) ; } ;
			}
			break ;
		   case UClit('n'):	vrhm.xlsxURL = V4RH_XLSXURL_none ; break ;
		   case UClit('s'):	vrhm.xlsxURL = V4RH_XLSXURL_secure ; break ;
		   case UClit('u'):	vrhm.xlsxURL = V4RH_XLSXURL_userKey ; break ;
		 } ;
		break ;
	      case UClit('m'):
		vrhm.minimizeHTML = TRUE ; break ;
	      CASEPP('p',"appPrefix") break ;
	      case UClit('P'):
	        vrhm.forPrinting = TRUE ; break ;
	      case UClit('q'):
		wantQuiet = TRUE ; break ;
	      CASEPP('s',"serverURL") break ;
	      case UClit('t'):
		ax++ ; if (ax >= argc) { printf("? Missing value for -t option\n") ; exit(EXITABORT) ; } ;
		if (UCstrlen(argv[ax]) >= UCsizeof(tbuf)) { printf("? -t argument too long\n") ; exit(EXITABORT) ; } ;
		UCstrcpy(tbuf,argv[ax]) ; 
		{ UCCHAR *b = UCstrchr(tbuf,'=') ; if (b == NULL) { printf("? Invalid -t syntax (expecting -t tag=value)\n") ; exit(EXITABORT) ; } ;
		  *b = UCEOS ;
		  v4rh_neUpdate(&vrhm,tbuf,V4RH_ETYPE_Tag,b+1,V4RH_UPDOPER_Update) ;
		}
		break ;
	      case UClit('u'):
		vrhm.outEncoding = V4L_TextFileType_UTF8 ; break ;
	      case UClit('x'):
		vrhm.outFormat = xls ; break ;
	      case UClit('X'):
		vrhm.outFormat = xlsx ; break ;
	    } ;
	 } ;

	if (wantTrace) vout_UCText(VOUT_Trace,0,UClit("*Parsed startup switches\n")) ;

//	if (vrhm.inputIsText)
//	 { //v4rt_ScopeOutTxtReport(&vrhm) ;
//	   exit(EXITOK) ;
//	 } ;

	if (vrhm.outFile == NULL && vrhm.inpFile == NULL) { printf("? No input/output files given\n") ; exit(EXITABORT) ; } ;
	if (vrhm.outFile == NULL)
	 { if (UCstrchr(vrhm.inpFile,'.') == NULL)
	    { UCCHAR *t ;
	      vrhm.outFile = vrhm.inpFile ;
	      t = v4rpp_AllocUC(UCstrlen(vrhm.inpFile) + 10) ; UCstrcpy(t,vrhm.inpFile) ; UCstrcat(t,UClit(".v4r")) ; vrhm.inpFile = t ;
	    } else { printf("? Missing output file\n") ; exit(EXITABORT) ; } ;
	 } ;

/*	If no initialization then default, append core ini file unless explicitly requested not to */
	if (iniFileNum == 0) iniFile[iniFileNum++] = UClit("v4rpp.ini") ;
	if (wantCore)
	 { UCCHAR *fn = v_GetV4HomePath(UClit("v4rppCore.v4i")) ;
	   iniFile[iniFileNum] = v4rpp_AllocUC(UCstrlen(fn)) ; UCstrcpy(iniFile[iniFileNum],fn) ;
	   iniFileNum++ ;
	 } ;

/*	Set up default section names */
	v4rh_neUpdate(&vrhm,UClit("main"),V4RH_ETYPE_Tag,(vrhm.forPrinting ? UClit("main.v4rMSPrint") : UClit("main.v4r")),V4RH_UPDOPER_Update) ;
	v4rh_neUpdate(&vrhm,UClit("css.internal"),V4RH_ETYPE_GenCSS,UClit(""),V4RH_UPDOPER_Update) ;
	v4rh_neUpdate(&vrhm,UClit("js.internal"),V4RH_ETYPE_GenJS,UClit(""),V4RH_UPDOPER_Update) ;
	v4rh_neUpdate(&vrhm,UClit("columnGroups"),V4RH_ETYPE_GenColGroup,UClit(""),V4RH_UPDOPER_Update) ;
	v4rh_neUpdate(&vrhm,UClit("html.include"),V4RH_ETYPE_GenHTMLInclude,UClit(""),V4RH_UPDOPER_Update) ;
	v4rh_neUpdate(&vrhm,UClit("html.includeTop"),V4RH_ETYPE_GenIncludeTop,UClit(""),V4RH_UPDOPER_Update) ;
	v4rh_neUpdate(&vrhm,UClit("html.includeBottom"),V4RH_ETYPE_GenIncludeBottom,UClit(""),V4RH_UPDOPER_Update) ;
	v4rh_neUpdate(&vrhm,UClit("html.linkLibraries"),V4RH_ETYPE_GenLinkInclude,UClit(""),V4RH_UPDOPER_Update) ;
	{ time_t curTime ;
	  time(&curTime) ; UCsprintf(tbuf,UCsizeof(tbuf),UClit("%.19s"),ASCretUC(ctime(&curTime))) ;
	  v4rh_neUpdate(&vrhm,UClit("dateTime"),V4RH_ETYPE_Tag,tbuf,V4RH_UPDOPER_Update) ;
	}

	switch (vrhm.outFormat)
	 { case html:	v4rh_neUpdate(&vrhm,UClit("isHTML"),V4RH_ETYPE_Tag,UClit("true"),V4RH_UPDOPER_Update) ; break ;
	   case xls:	v4rh_neUpdate(&vrhm,UClit("isXLS"),V4RH_ETYPE_Tag,UClit("true"),V4RH_UPDOPER_Update) ; break ;
	   case xlsx:	v4rh_neUpdate(&vrhm,UClit("isXLSX"),V4RH_ETYPE_Tag,UClit("true"),V4RH_UPDOPER_Update) ; break ;
	   case json:	v4rh_neUpdate(&vrhm,UClit("isJSON"),V4RH_ETYPE_Tag,UClit("true"),V4RH_UPDOPER_Update) ; break ;
	   case tab:	v4rh_neUpdate(&vrhm,UClit("isTAB"),V4RH_ETYPE_Tag,UClit("true"),V4RH_UPDOPER_Update) ; break ;
	   case csv:	v4rh_neUpdate(&vrhm,UClit("isCSV"),V4RH_ETYPE_Tag,UClit("true"),V4RH_UPDOPER_Update) ; break ;
	 } ;
	

/*	Set up default CSS entries for Numeric(0), Alpha(1), Date cells(2) */
	v4rh_DefDimEntry(&vrhm,V4RH_DimNum_Int,NULL,v4rh_GetCSSIndex(&vrhm,UClit("text-align:right;")),UNUSED,V4DPI_PntType_Int) ;
	v4rh_DefDimEntry(&vrhm,V4RH_DimNum_Float,NULL,v4rh_GetCSSIndex(&vrhm,UClit("text-align:right;")),UNUSED,V4DPI_PntType_Real) ;
	v4rh_DefDimEntry(&vrhm,V4RH_DimNum_Alpha,NULL,UNUSED,UNUSED,V4DPI_PntType_UCChar) ;
	v4rh_DefDimEntry(&vrhm,V4RH_DimNum_Date,NULL,v4rh_GetCSSIndex(&vrhm,UClit("text-align:right;")),v4rh_GetMaskIndex(&vrhm,UClit("dd-mmm-yy")),V4DPI_PntType_Calendar) ;
	v4rh_DefDimEntry(&vrhm,V4RH_DimNum_Logical,NULL,v4rh_GetCSSIndex(&vrhm,UClit("text-align:center;")),UNUSED,V4DPI_PntType_Logical) ;
//	v4rh_DefDimEntry(&vrhm,V4RH_DimNum_MIDASUDT,NULL,UNUSED,UNUSED,V4DPI_PntType_MIDASUDT) ;
	v4rh_DefDimEntry(&vrhm,V4RH_DimNum_XL,NULL,UNUSED,UNUSED,V4DPI_PntType_SSVal) ;
	if (wantTrace) vout_UCText(VOUT_Trace,0,UClit("*Set up default names and CSS\n")) ;

/*	Open up the V4R (input) file */
	vrhm.tcb = v4mm_AllocChunk(sizeof *vrhm.tcb,FALSE) ;
	for(loop=TRUE;loop;)
	 {
	   if (!v_UCFileOpen(&vrhm.iFile,vrhm.inpFile,UCFile_Open_Read,TRUE,vrhm.errMessage,0))
	    goto fail ;
	   vrhm.iFile.wantEOL = FALSE ;
	   v4lex_InitTCB(vrhm.tcb,V4LEX_TCBINIT_NoStdIn) ; v4lex_NestInput(vrhm.tcb,&vrhm.iFile,vrhm.inpFile,V4LEX_InpMode_File) ;
/*	   First pass read the input file & build a tree of the report */
	   switch (v4rh_ReadV4RPass1(&vrhm))
	    {
	      case 0:		/* Error */
		vout_UCText(VOUT_Err,0,vrhm.errMessage) ; vout_NL(VOUT_Err) ;
/*		Pretend we read OK but execute a different section */
		vrhm.gotEOF = TRUE ;
		v4rh_neUpdate(&vrhm,UClit("errMessage"),V4RH_ETYPE_Tag,vrhm.errMessage,V4RH_UPDOPER_Update) ;
/*		Something went wrong - if we have a "Section:main.internalErrors" then zap any existing output & regenerate from that */
		v4rh_neUpdate(&vrhm,UClit("main"),V4RH_ETYPE_Tag,UClit("main.internalErrors"),V4RH_UPDOPER_Update) ;
	      case 1:		/* Normal pass1 */
		loop = FALSE ;break ;
	      case 2:		/* Have to read another v4r file, tag 'fileName' better have the new name */
		{ struct V4RH__NamedEntry *ne ;
		  ne = v4rh_neLookup(&vrhm,UClit("fileName")) ;
		  if (ne == NULL)
		   { v_Msg(NULL,vrhm.errMessage,"@Improper linkage to prior V4R - no 'fileName' tag defined") ; goto fail ; } ;
		  vrhm.inpFile = ne->eVal ;
		}
		break ;
	    } ;
	 } ;
	if (wantTrace) vout_UCText(VOUT_Trace,0,UClit("*Parsed input V4R file\n")) ;

	if (vrhm.outFormat != html && vrhm.v4rIsHTML)
	 { vrhm.outFormat = html ; printf("[-x/X/J switch ignored, source v4r contains HTML code]\n") ; }

/*	Did we only get one file specification (and no extension) */
	if (UCstrchr(vrhm.outFile,'.') == NULL)
	 { UCCHAR *t ;
	   t = v4rpp_AllocUC(UCstrlen(vrhm.outFile) + 10) ; UCstrcpy(t,vrhm.outFile) ;
	   switch (vrhm.outFormat)
	    { case html:	UCstrcat(t,UClit(".htm")) ; break ;
	      case xls:		UCstrcat(t,UClit(".xls")) ; break ;
	      case xlsx:	UCstrcat(t,UClit(".xlsx")) ; break ;
	      case json:	UCstrcat(t,UClit(".ajax")) ; break ;
	      case tab:		UCstrcat(t,UClit(".txt")) ; break ;
	      case csv:		UCstrcat(t,UClit(".csv")) ; break ;
	    } ;
	   vrhm.outFile = t ;
	 } ;


/*	Parse the ini file(s) */
	{ INDEX ix ; LOGICAL gotEOFSave = vrhm.gotEOF ;
	  for(ix=0;ix<iniFileNum;ix++)
	   { if (!v_UCFileOpen(&vrhm.iFile,iniFile[ix],UCFile_Open_Read,TRUE,vrhm.errMessage,0))
	      goto fail ;
	     v4lex_InitTCB(vrhm.tcb,V4LEX_TCBINIT_NoStdIn) ; v4lex_NestInput(vrhm.tcb,&vrhm.iFile,iniFile[ix],V4LEX_InpMode_File) ;
	     if (!v4rh_ReadMaskFile(&vrhm,iniFile[ix]))
	      goto fail ;
	   } ;
	  vrhm.gotEOF = gotEOFSave ;
	}
	if (wantTrace) vout_UCText(VOUT_Trace,0,UClit("*Parsed ini file(s)\n")) ;

/*	Assign CSS style names */
	if (vrhm.vce != NULL)
	 { INDEX i ;
	   for(i=0;i<vrhm.vce->numCSS;i++)
	    { if (vrhm.vce->css[i].className != NULL) continue ;
	      vrhm.vce->css[i].className = v4rpp_AllocUC(8) ;
	      v_Msg(NULL,vrhm.vce->css[i].className,"@c%1d",i) ;
	    } ;
	 } ;

/*	Go through all sheets/rows/cells & see if it is worth it to create column groups */
	{ struct V4RH__Sheet *sheet ; struct V4RH__Row *row ; struct V4RH__Cell *cell ;
	  struct V4RH__Column *col ; INDEX sheetNum ;
	  for (sheetNum=1,sheet=vrhm.sheetFirst;sheet!=NULL;sheet=sheet->sheetNext,sheetNum++)
	   { 
/*	     Make sure each sheet has a name */
	     if (UCempty(sheet->sheetName))
	      { UCsprintf(sheet->sheetName,UCsizeof(sheet->sheetName),UClit("Sheet%d"),sheetNum) ; } ;
/*	     If last sheet then set paramter */
/*	     Go through all of the headers first */
	     for(row=sheet->hdrFirst;row!=NULL;row=row->rowNext)
	      { if (sheet->colFirst == NULL) sheet->colFirst = v4rpp_AllocChunk(sizeof *col,TRUE) ;
	        col = NULL ;
	        for(cell=row->cellFirst;cell!=NULL;cell=cell->cellNext)
		 { if (col == NULL) { col = sheet->colFirst ; }
		    else { if (col->colNext == NULL) col->colNext = v4rpp_AllocChunk(sizeof *col,TRUE) ;
			   col = col->colNext ;
			 } ;
		   if (cell->dimNum >= 0 && cell->dimNum < V4RH_DIM_MAX)
		    { col->dimNumCount[cell->dimNum]++ ;
		    } ;
		 } ;
	      } ;
/*	     Repeat for all the data rows */
	     for(row=sheet->rowFirst;row!=NULL;row=row->rowNext)
	      { if (sheet->colFirst == NULL) sheet->colFirst = v4rpp_AllocChunk(sizeof *col,TRUE) ;
	        col = NULL ;
	        for(cell=row->cellFirst;cell!=NULL;cell=cell->cellNext)
		 { if (col == NULL) { col = sheet->colFirst ; }
		    else { if (col->colNext == NULL) col->colNext = v4rpp_AllocChunk(sizeof *col,TRUE) ;
			   col = col->colNext ;
			 } ;
		   if (cell->dimNum >= 0 && cell->dimNum < V4RH_DIM_MAX)
		    { col->dimNumCount[cell->dimNum]++ ;
		    } ;
		 } ;
	      } ;
	   } ;
	}

/*	If single sheet then define it as the current (otherwise hope we see 'Repeat sheet') */
	if (vrhm.sheetNum == 1)
	 { if (!v4rh_SetCurSheet(&vrhm,vrhm.sheetFirst)) goto fail ; } ;
	UCsprintf(tbuf,UCsizeof(tbuf),UClit("%d"),vrhm.sheetNum) ;
	v4rh_neUpdate(&vrhm,UClit("sheetNum"),V4RH_ETYPE_Tag,tbuf,V4RH_UPDOPER_Update) ;
/*	If we did not get a 'BSEOF' section then flip to Section:main.noEOF */
	if (!vrhm.gotEOF && !vrhm.v4rIsHTML)
	 v4rh_neUpdate(&vrhm,UClit("main"),V4RH_ETYPE_Tag,UClit("main.noEOF"),V4RH_UPDOPER_Update) ;

/*	Are we creating something other than HTML ? */
	if (vrhm.outFormat != html)
	 { 
	   switch(vrhm.outFormat)
	    { case xlsx:	if (!v4rx_GenerateXLSX(&vrhm,FALSE)) goto fail ; break ;
	      case xls:		v4rx_GenerateXLS(&vrhm,FALSE) ; break ;
	      case json:	if (!v4j_GenerateJSON(&vrhm,FALSE)) goto fail ; break ;
	      case tab:		if (!v4t_GenerateTABCSV(&vrhm)) goto fail ; break ;
	      case csv:		if (!v4t_GenerateTABCSV(&vrhm)) goto fail ; break ;
	    } ;
	   if (!wantQuiet) v4rh_ExitStats() ;
	   exit(EXITOK) ;
	 } ;

/*	Set up output file (stream) */
	if (vrhm.outFile == NULL) { printf("? No output files given\n") ; exit(EXITABORT) ; } ;
	vout_Init() ; vrhm.outFileX = UNUSED ;
	if ((vrhm.outFileId = vout_OpenStreamFile(NULL,vrhm.outFile,UClit("htm"),NULL,FALSE,vrhm.outEncoding,0,vrhm.errMessage)) == UNUSED)
	 goto fail ;
	vrhm.outFileX = vout_FileIdToFileX(vrhm.outFileId) ;

/*	Set up json object with runtime parameters */
	{ struct V4RH__Sheet *sheet ; struct V4RH__Row *row ;
	  struct V4RH__NamedEntry *ne ; UCCHAR json[2046], jparam[256] ; INDEX i,rows ;
	  UCstrcpy(json,UClit("{")) ;
	  ne = v4rh_neLookup(&vrhm,UClit("jobId")) ; UCsprintf(jparam,UCsizeof(jparam),UClit("jobId:'%s'"),(ne == NULL ? UClit("none") : ne->eVal)) ; UCstrcat(json,jparam) ;
	  ne = v4rh_neLookup(&vrhm,UClit("serverURL")) ; if (ne != NULL) { UCsprintf(jparam,UCsizeof(jparam),UClit(",cgiLink:'%s'"),ne->eVal) ; UCstrcat(json,jparam) ; } ;
	  ne = v4rh_neLookup(&vrhm,UClit("sessionKey")) ; if (ne != NULL) { UCsprintf(jparam,UCsizeof(jparam),UClit(",sessionKey:'%s'"),ne->eVal) ; UCstrcat(json,jparam) ; } ;
	  ne = v4rh_neLookup(&vrhm,UClit("reportName")) ; if (ne != NULL) { UCsprintf(jparam,UCsizeof(jparam),UClit(",reportName:'%s'"),ne->eVal) ; UCstrcat(json,jparam) ; } ;
	  ne = v4rh_neLookup(&vrhm,UClit("version")) ; if (ne != NULL) { UCsprintf(jparam,UCsizeof(jparam),UClit(",version:'%s'"),ne->eVal) ; UCstrcat(json,jparam) ; } ;
	  ne = v4rh_neLookup(&vrhm,UClit("dateTime")) ; if (ne != NULL) { UCsprintf(jparam,UCsizeof(jparam),UClit(",dateTime:'%s'"),ne->eVal) ; UCstrcat(json,jparam) ; } ;
	  ne = v4rh_neLookup(&vrhm,UClit("dfltFormat")) ; if (ne != NULL) { UCsprintf(jparam,UCsizeof(jparam),UClit(",dfltFormat:%s"),ne->eVal) ; UCstrcat(json,jparam) ; } ;
	  UCsprintf(jparam,UCsizeof(jparam),UClit(",type:'%s'"),(vrhm.sheetNum > 1 ? UClit("multi") : UClit("single"))) ; UCstrcat(json,jparam) ;
	  ne = v4rh_neLookup(&vrhm,UClit("userName")) ; if (ne != NULL) { UCsprintf(jparam,UCsizeof(jparam),UClit(",userName:'%s'"),ne->eVal) ; UCstrcat(json,jparam) ; } ;
/*	  Get page counts for each sheet */
	  UCstrcat(json,UClit(",rows:[")) ;
	  for(i=0,sheet=vrhm.sheetFirst;sheet!=NULL;sheet=sheet->sheetNext,i++)
	   { for(rows=0,row=sheet->rowFirst;row!=NULL;row=row->rowNext,rows++) { } ;
	     UCsprintf(jparam,UCsizeof(jparam),UClit("%s%d"),(i > 0 ? UClit(",") : UClit("")),rows) ; UCstrcat(json,jparam) ;
	   } ;
	  UCstrcat(json,UClit("]")) ;
	  UCstrcat(json,UClit("}")) ;
	  v4rh_neUpdate(&vrhm,UClit("jsonParams"),V4RH_ETYPE_Tag,json,V4RH_UPDOPER_Update) ;
	}



/*	Now start processing of masks - Execute Section:main */
	if (wantTrace) vout_UCText(VOUT_Trace,0,UClit("*Starting output processing\n")) ;
	if (vrhm.sheetFirst == NULL && !vrhm.gotError && !vrhm.v4rIsHTML)
	 v4rh_neUpdate(&vrhm,UClit("main"),V4RH_ETYPE_Tag,UClit("main.noResults"),V4RH_UPDOPER_Update) ;
	if (!v4rh_XctSection(&vrhm,UClit("main")))
	 goto fail ;

/*	Close output & all done */
	vout_CloseFile(vrhm.outFileId,UNUSED,vrhm.errMessage) ;
	if (!wantQuiet) v4rh_ExitStats() ;
	exit(EXITOK) ;

fail:	if (!wantQuiet)
	 { vout_UCText(VOUT_Err,0,UClit("V4E- ?")) ; vout_UCText(VOUT_Err,0,vrhm.errMessage) ; vout_NL(VOUT_Err) ; } ;
/*	Something went wrong - if we have a "Section:main.internalErrors" then zap any existing output & regenerate from that */
	v4rh_neUpdate(&vrhm,UClit("main"),V4RH_ETYPE_Tag,UClit("main.internalErrors"),V4RH_UPDOPER_Update) ;
	v4rh_neUpdate(&vrhm,UClit("errMessage"),V4RH_ETYPE_Tag,vrhm.errMessage,V4RH_UPDOPER_Update) ;
	if (vrhm.outFileX != UNUSED) vout_CloseFile(vrhm.outFileId,UNUSED,vrhm.errMessage) ;
/*	Try to re-open the file */
	if ((vrhm.outFileId = vout_OpenStreamFile(NULL,vrhm.outFile,UClit("htm"),NULL,FALSE,vrhm.outEncoding,0,vrhm.errMessage)) != UNUSED)
	 { vrhm.outFileX = vout_FileIdToFileX(vrhm.outFileId) ;
	   v4rh_XctSection(&vrhm,UClit("main")) ;
	   vout_CloseFile(vrhm.outFileId,UNUSED,vrhm.errMessage) ;
	 } ;
	exit(EXITABORT) ;
}

void v4rh_ExitStats()
{
  time_t time_of_day ; double wallseconds ;
  double cpusecondsU,cpusecondsK ;
#ifdef WINNT
  FILETIME ftKernel2,ftUser2 ;
#endif
#ifdef UNIX
  struct timeval endTime ;
#endif
  UCCHAR tbuf[512] ;

	time(&time_of_day) ; UCsprintf(tbuf,UCsizeof(tbuf),UClit("V4RPP(64) %d.%d Processing Summary - %.19s\n"),V4RPP_Version_Major,V4RPP_Version_Minor,ASCretUC(ctime(&time_of_day))) ; vout_UCText(VOUT_Status,0,tbuf) ;
	wallseconds = v_ConnectTime() ;
#ifdef WINNT
	if (GetProcessTimes(GetCurrentProcess(),&gpi->ftCreate,&gpi->ftExit,&ftKernel2,&ftUser2))	/* Only do if call implemented (NT) */
	 { B64INT t1,t2 ; memcpy(&t1,&gpi->ftUser1,sizeof t1) ; memcpy(&t2,&ftUser2,sizeof t2) ;
	   cpusecondsU = (t2 - t1) / 10000000.0 ;
	   memcpy(&t1,&gpi->ftKernel1,sizeof t1) ; memcpy(&t2,&ftKernel2,sizeof t2) ;
	   cpusecondsK = (t2 - t1) / 10000000.0 ;
	 } ;
#else
	cpusecondsU = v_CPUTime() ; cpusecondsK = 0.0 ;
#endif
/*	Keep this output a little different from V4 Exit- (V4 is 'Time:', V4RPP is 'Time-') so that xlibAdmin.v4:Display-Log-Files can tell them apart */
#ifdef WINNT
	UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Time- %g elapsed seconds, %g CPU seconds (%g user + %g kernel)\n"),wallseconds,(cpusecondsU+cpusecondsK),cpusecondsU,cpusecondsK) ; vout_UCText(VOUT_Status,0,tbuf) ;
	cpusecondsU += cpusecondsK ;
#else
	UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Time- %g elapsed seconds, %g CPU seconds\n"),wallseconds,cpusecondsU) ; vout_UCText(VOUT_Status,0,tbuf) ;
#endif
}

struct V4RH__NamedEntry *v4rh_neLookup(vrhm,eName)
  struct V4RH__Main *vrhm ;
  UCCHAR *eName ;
{ UCCHAR eNameUC[V4RH_Name_Max+1], *dfltVal ; INDEX hx,i ;
  static struct V4RH__NamedEntry vrneDflt ;

	dfltVal = NULL ;
	for(i=0;i<V4RH_Name_Max&&eName[i]!=UCEOS;i++)
	 { if (eName[i] == UClit(','))		/* Do we have 'name,default-value' construct ? */
	    { dfltVal = &eName[i+1] ; break ; } ;
	   eNameUC[i] = UCTOUPPER(eName[i]) ;
	 } ; eNameUC[i] = UCEOS ;
	VHASH32_FWD(hx,eNameUC,UCstrlen(eNameUC))
	for(;;hx++)
	 { hx = (hx & 0x7fffffff) % vrhm->vrneMax ;
	   if (vrhm->vrne[hx] == NULL) break ;
	   if (UCstrcmp(vrhm->vrne[hx]->eName,eNameUC) == 0) return(vrhm->vrne[hx]) ;
	 } ;
/*	Entry not found, return default (if we have it) */
	if (dfltVal == NULL) return(NULL) ;
	UCstrcpy(vrneDflt.eName,eNameUC) ; vrneDflt.eType = V4RH_ETYPE_Tag ;
	vrneDflt.eVal = dfltVal ;
	return(&vrneDflt) ;
} ;

struct V4RH__NamedEntry *v4rh_neUpdate(vrhm,eName,eType,eVal,updOper)
  struct V4RH__Main *vrhm ;
  UCCHAR *eName ;
  ETYPE eType ;
  UCCHAR *eVal ;
  ETYPE updOper ;
{ UCCHAR eNameUC[V4RH_Name_Max+1] ; INDEX hx ; LENMAX lenVal ;

	UCcnvupper(eNameUC,eName,UCsizeof(eNameUC)) ;
	VHASH32_FWD(hx,eNameUC,UCstrlen(eNameUC))
	for(;;hx++)
	 { hx = (hx & 0x7fffffff) % vrhm->vrneMax ;
	   if (vrhm->vrne[hx] == NULL) break ;
	   if (UCstrcmp(vrhm->vrne[hx]->eName,eNameUC) == 0) break ;
	 } ;
/*	Is this a new entry ? */
	if (vrhm->vrne[hx] == NULL)
	 { if (updOper == V4RH_UPDOPER_Delete) return(NULL) ;	/* Entry does not yet exist */
	   vrhm->vrne[hx] = (struct V4RH__NamedEntry *)v4rpp_AllocChunk(sizeof *vrhm->vrne[hx],FALSE) ;
	   UCstrcpy(vrhm->vrne[hx]->eName,eNameUC) ; vrhm->vrne[hx]->eType = eType ; vrhm->vrne[hx]->maxLenVal = 0 ; vrhm->vrne[hx]->eVal = NULL ;
	 } ;
	switch (updOper)
	 { default:			return(NULL) ;
	   case V4RH_UPDOPER_Update:
		if (vrhm->vrne[hx]->eVal != NULL) { ZUS(vrhm->vrne[hx]->eVal) ; } ;
/*		Just fall thru & append to empty string */
	   case V4RH_UPDOPER_Append:
		lenVal = UCstrlen(eVal) ;
		if (vrhm->vrne[hx]->eVal == NULL)
		 {  vrhm->vrne[hx]->eVal = v4rpp_AllocUC(lenVal+32) ; ZUS(vrhm->vrne[hx]->eVal) ;
		    vrhm->vrne[hx]->maxLenVal = lenVal + 32 ;
		 } else
		 { if (vrhm->vrne[hx]->maxLenVal < UCstrlen(vrhm->vrne[hx]->eVal) + lenVal + 1)
		    { UCCHAR *t = v4rpp_AllocUC(sizeof(UCCHAR)*(vrhm->vrne[hx]->maxLenVal+lenVal+32)) ;
		      UCstrcpy(t,vrhm->vrne[hx]->eVal) ; vrhm->vrne[hx]->eVal = t ;
		      vrhm->vrne[hx]->maxLenVal = vrhm->vrne[hx]->maxLenVal + lenVal + 32 ;
		    } ;
		 } ;
/*		If destination not empty then append EOS onto end before cat'ing new string */
		if (UCnotempty(vrhm->vrne[hx]->eVal)) UCstrcat(vrhm->vrne[hx]->eVal,UCEOLbts) ;
		UCstrcat(vrhm->vrne[hx]->eVal,eVal) ;
		if (wantTrace)
		 { if (UCstrlen(vrhm->vrne[hx]->eVal) < 255)
		    { v_Msg(NULL,vrhm->errMessage,"@*Updating %1U = %2U\n",vrhm->vrne[hx]->eName,vrhm->vrne[hx]->eVal) ;
		    } else { v_Msg(NULL,vrhm->errMessage,"@*Updating %1U = * %2d characters *\n",vrhm->vrne[hx]->eName,UCstrlen(vrhm->vrne[hx]->eVal)) ; } ;
		   vout_UCText(VOUT_Trace,0,vrhm->errMessage) ;
		 } ;
		break ;
	   case V4RH_UPDOPER_Delete:
		if (wantTrace) { v_Msg(NULL,vrhm->errMessage,"@*Deleting symbol %1U\n",vrhm->vrne[hx]->eName) ; vout_UCText(VOUT_Trace,0,vrhm->errMessage) ; } ;
		vrhm->vrne[hx] = NULL ; break ;
	 } ;
	return(vrhm->vrne[hx]) ;
} ;

LOGICAL v4rh_ReadMaskFile(vrhm,iniFile)
  struct V4RH__Main *vrhm ;
  UCCHAR *iniFile ;
{ struct V4LEX__TknCtrlBlk *tcb ;
  struct UC__File iFile ;
  UCCHAR eVal[UCFile_GetBufMaxChars], eName[V4RH_Name_Max+1] ; LENMAX eValLen ;
  UCCHAR *s, iBuf[UCReadLine_MaxLine] ;

	ZUS(eVal) ; eValLen = 0 ; ZUS(eName) ; tcb = vrhm->tcb ;
	for(;;)
	 {
	   if (!v4lex_ReadNextLine(tcb,NULL,NULL,NULL)) break ;
	   switch (tcb->ilvl[tcb->ilx].input_ptr[0])
	    { case UCEOS:
	      case UClit('/'):
	      case UClit('!'):
		continue ;			/* Ignore comment & blank lines */
	      default:


/*	   Look for next '##xxx##' construct */

		{ UCCHAR *t1, *t2, eName[V4RH_Name_Max+1] ; INDEX i ;
		  struct V4RH__NamedEntry *ne ;
		  ZUS(iBuf) ; s = tcb->ilvl[tcb->ilx].input_ptr ;
		  for(;;s=t2+2)
		   { t1 = UCstrstr(s,UClit("##")) ;
		     if (t1 == NULL) { UCstrcat(iBuf,s) ; break ; } ;
		     *t1 = UCEOS ; UCstrcat(iBuf,s) ;
		     t2 = UCstrstr(t1+2,UClit("##")) ;
		     if (t2 == NULL) { v_Msg(NULL,vrhm->errMessage,"@Improperly formed '##tag##' on line (%1d) of %2U",tcb->ilvl[tcb->ilx].UCFile->lineNum,iniFile) ; return(FALSE) ; } ;
		     for(i=0,t1=t1+2;t1!=t2&&i<V4RH_Name_Max;t1++,i++) { eName[i] = *t1 ; } ; eName[i] = UCEOS ;
		     ne = v4rh_neLookup(vrhm,eName) ;
		     if (ne == NULL)
		      { v_Msg(NULL,vrhm->errMessage,"@No such tag (%1U) on line (%2d) of %3U",eName,tcb->ilvl[tcb->ilx].UCFile->lineNum,iniFile) ; return(FALSE) ; } ;
		     UCstrcat(iBuf,ne->eVal) ;
		   } ; s = iBuf ;
		}


		if (UCstrncmpIC(UClit("SECTION:"),s,8) == 0)
		 { INDEX i ; LOGICAL ok ;
		   if (UCnotempty(eName))
		    { if (v4rh_neLookup(vrhm,eName) == NULL)
		       { v4rh_neUpdate(vrhm,eName,V4RH_ETYPE_Section,eVal,V4RH_UPDOPER_Update) ; }
		       else { printf("V4R- Duplicate Section name(%s) encountered, first one not overwritten\n",UCretASC(eName)) ; } ;
		      if (UCstrcmpIC(eName,UClit("SECTION:IMMEDIATE")) == 0) 
		       { ok = v4rh_XctSection(vrhm,UClit("IMMEDIATE")) ;
/*			 Now delete the entry so we can handle another IMMEDIATE if we get one */
			 v4rh_neUpdate(vrhm,eName,V4RH_ETYPE_Section,eVal,V4RH_UPDOPER_Delete) ;
			 if (!ok) return(FALSE) ;
		       } ;
		      ZUS(eVal) ; eValLen = 0 ;
		    } ;
/*		   Scan for end of name (name may include alphanumerics & '.') */
		   for(i=8;s[i]!=UCEOS && !vuc_IsWSpace(s[i]);i++) { } ;
		   if (i == 8) { v_Msg(NULL,vrhm->errMessage,"@%0F - No section name given on line %1d of %2U",tcb->ilvl[tcb->ilx].UCFile->lineNum,iniFile) ; return(FALSE) ; } ;
		   s[i] = UCEOS ; UCstrcpy(eName,s) ;
		   continue ;
		 } ;
		if (UCstrncmpIC(UClit("INCLUDE"),s,7) == 0)
		 { UCCHAR fileName[V_FileName_Max] ;
		   s += 7 ;
		   for(;vuc_IsWSpace(*s);s++) { } ;
		   tcb->ilvl[tcb->ilx].input_ptr = s ;				/* This is really bad but it works */
		   if (UCnotempty(tcb->ilvl[tcb->ilx].input_ptr))		/* Don't include nonexistant file - just skip */
		    { v_ParseFileName(tcb,fileName,NULL,UClit("ini"),NULL) ;
		      if (!v_UCFileOpen(&iFile,fileName,UCFile_Open_Read,TRUE,vrhm->errMessage,0)) return(FALSE) ;
		      iFile.wantEOL = FALSE ; 
		      v4lex_NestInput(vrhm->tcb,&iFile,fileName,V4LEX_InpMode_File) ;
		    } ;
		   continue ;
		 } ;
		if (UCstrncmpIC(UClit("V4R-Input-to-EOF"),s,16) == 0)
		 { v4rh_ReadV4RPass1(vrhm) ;
		   continue ;
		 } ;
/*		Not a new section - just fall thru and append */
		tcb->ilvl[tcb->ilx].input_ptr = s ;
	      case UClit('#'):
	      case UClit('<'):
	      case UClit(' '):
	      case UClit('\t'):
/*		Just append this line to current section value */
		if (UCempty(eName)) { v_Msg(NULL,vrhm->errMessage,"@%0F - No prior Section defined at line %1d of %2U",tcb->ilvl[tcb->ilx].UCFile->lineNum,iniFile) ; return(FALSE) ; } ;
		eValLen += (UCstrlen(tcb->ilvl[tcb->ilx].input_ptr)+1) ;
		if (eValLen >= UCsizeof(eVal))
		 { v_Msg(NULL,vrhm->errMessage,"@%0F Section (%1U) - Total size of entry (%4d) exceeds max (%5d)",eName,eValLen,UCsizeof(eVal)) ; return(FALSE) ; } ;
		UCstrcat(eVal,tcb->ilvl[tcb->ilx].input_ptr) ; UCstrcat(eVal,UCEOLbts) ;
		continue ;
	    } ;
	 } ;
/*	Hit EOF - write out last section BUT DON'T OVERWRITE AN EXISTING ONE WITH SAME NAME */
	if (UCnotempty(eName))
	 { if (v4rh_neLookup(vrhm,eName) == NULL)
	    { v4rh_neUpdate(vrhm,eName,V4RH_ETYPE_Section,eVal,V4RH_UPDOPER_Update) ; }
	    else { printf("V4R- Duplicate Section name(%s) encountered, first one not overwritten\n",UCretASC(eName)) ; } ;
	   ZUS(eVal) ; eValLen = 0 ;
	 } ;
	if (UCstrcmpIC(eName,UClit("SECTION:IMMEDIATE")) == 0) 
	 { LOGICAL ok ;
	   ok = v4rh_XctSection(vrhm,UClit("IMMEDIATE")) ;
/*	   Now delete the entry so we can handle another IMMEDIATE if we get one */
	   v4rh_neUpdate(vrhm,eName,V4RH_ETYPE_Section,eVal,V4RH_UPDOPER_Delete) ;
	   return(ok) ;
	 } ;
	return(TRUE) ;
}

#define V4RH_IfRep_Type_None -1			/* Nothing - skip section (for Repeat with a repeat of '0') */
#define V4RH_IfRep_Type_If 0			/* Not a repeat but an If */
#define V4RH_IfRep_Type_Row 1			/* Repeat for all rows */
#define V4RH_IfRep_Type_Cell 2			/* Repeat for all cells in row */
#define V4RH_IfRep_Type_Sheet 3			/* Repeat for all sheets */
#define V4RH_IfRep_Type_Title 4			/* Repeat for all titles in a sheet */
#define V4RH_IfRep_Type_Header 5		/* Repeat for all header (rows) in a sheet */
#define V4RH_IfRep_Type_Footer 6		/* Repeat for all footer (rows) in a sheet */

struct lcl__IfRepeatInfo {
  INDEX irX ;
  struct {
     LOGICAL okXct ;			/* If TRUE then OK to execute, if FALSE then don't */
     UCCHAR repType ;			/* Type of repeating section (V4RH_IfRep_Type_xxx) */
     UCCHAR *repBegin ;			/* Pointer to begin of repeat list of values, should be freed when leaving level */
     void *repPtr ;			/* Pointer depending on repType value */
     UCCHAR eNameFML[V4RH_Name_Max+1] ; /* If not empty then name of tag for repeat first/middle/last values */
     UCCHAR *eNameFVal ;		/* Value for first repeat value */
     UCCHAR *eNameMVal,*eNameLVal ;	/* Values for middle & last values */
   } level[V4RH_MAXIFS] ;
} ;


LOGICAL v4rh_XctSection(vrhm,secName)
  struct V4RH__Main *vrhm ;
  UCCHAR *secName ;
{ struct V4LEX__TknCtrlBlk tcb ;
  struct V4RH__NamedEntry *neSec ;
  struct lcl__IfRepeatInfo liri ;
  struct V4RH__Row *crow ;
  INDEX line ; UCCHAR *cp,*ep, secNameFull[V4RH_Name_Max+1] ;

	neSec = v4rh_neLookup(vrhm,secName) ;
	cp = (neSec == NULL ? secName : neSec->eVal) ;
	if (UCstrlen(cp) + 8 >= V4RH_Name_Max)
	 { v_Msg(NULL,vrhm->errMessage,"@Section name(%1U) -> (%2U), with 'Section:' prefix exceeds max allowed name length",secName,cp) ; return(FALSE) ; } ;
	UCstrcpy(secNameFull,UClit("Section:")) ; UCstrcat(secNameFull,cp) ;
	neSec = v4rh_neLookup(vrhm,secNameFull) ;
	if (neSec == NULL)
	 { v_Msg(NULL,vrhm->errMessage,"@Could not locate section (%1U) -> %2U",secName,secNameFull) ; return(FALSE) ; } ;
/*	Initialize local if/repeat control structure, set okXct to true for base level */
	memset(&liri,0,sizeof liri) ; liri.level[liri.irX].okXct = TRUE ;
/*	Go thru each line in this section & interpret and/or output and/or whatever */
	for(cp=neSec->eVal,ep=cp,line=1;ep!=NULL;line++,cp=ep+1)
	 { ep = UCstrchr(cp,EOLbt) ;
	   if (ep != NULL) *ep = UCEOS ;			/* Set terminating char to EOS - HAVE TO RESET!!! */
	   if (UCempty(cp)) continue ;
	   if (vuc_IsWSpace(cp[0]) || cp[0] == UClit('<') || cp[0] == UClit('#'))
	    { if (liri.level[liri.irX].okXct)			/* Only handle if conditions are true */
	       { if (!v4rh_XctSectionLine(vrhm,cp,secName,line)) return(FALSE) ;
	         vout_NLFileX(vrhm->outFileX) ;
	       } ;
	      if (ep != NULL) *ep = EOLbt ;			/* Reset terminating character */
	      continue ;
	    } ;
/*	   Look for section command: If, Else, End, Repeat */
	   v4lex_InitTCB(&tcb,V4LEX_TCBINIT_NoStdIn) ; v4lex_NestInput(&tcb,NULL,cp,V4LEX_InpMode_String) ;
	   v4lex_NextTkn(&tcb,0) ;
	   switch (v4im_GetUCStrToEnumVal(tcb.UCstring,0))
	    { default:
		if (UCstrlen(cp) < 2) continue ;	/* Sometimes have problems with extra end-of-line characters, don't generate error if it is probably one of those */
		v_Msg(NULL,vrhm->errMessage,"@Section (%1U) line (%2d) - invalid command: %3U",secNameFull,line,cp) ;
		return(FALSE) ;
	      case _Rename:
		if (vrhm->outFileX != UNUSED) vout_CloseFile(vrhm->outFileId,UNUSED,vrhm->errMessage) ;
		UCremove(vrhm->outFile) ;
		if (!UCmove(vrhm->inpFile,vrhm->outFile)) { v_Msg(NULL,vrhm->errMessage,"@Could not move %1U -> %2U - %3O",vrhm->inpFile,vrhm->outFile,errno) ; return(FALSE) ; } ;
		break ;
	      case _Set:
		if (!liri.level[liri.irX].okXct) break ;	/* Skip if we are not xcting this part */
		{ UCCHAR eName[V4RH_Name_Max+1] ;
		  v4lex_NextTkn(&tcb,0) ; if (tcb.type != V4LEX_TknType_Keyword) { v_Msg(NULL,vrhm->errMessage,"@Section (%1U) line (%2d) - Expecting tag name after 'SET'",secNameFull,line) ; return(FALSE) ; } ;
		  UCstrcpy(eName,tcb.UCkeyword) ;
		  for(;;)
		   { v4lex_NextTkn(&tcb,0) ;
		     if (tcb.opcode == V_OpCode_Dot)
		      { v4lex_NextTkn(&tcb,0) ;
		        if (UCstrlen(eName) + UCstrlen(tcb.UCkeyword) + 1 >= UCsizeof(eName)) { v_Msg(NULL,vrhm->errMessage,"@Section (%1U) line (%2d) - Tag name too long",secNameFull,line) ; return(FALSE) ; } ;
		        UCstrcat(eName,UClit(".")) ; UCstrcat(eName,tcb.UCkeyword) ; continue ;
		      } ;
		     break ;
		   } ;
		  switch(tcb.opcode)
		   { default:			v_Msg(NULL,vrhm->errMessage,"@Section (%1U) line (%2d) - Expecting '=' after tag name",secNameFull,line) ; return(FALSE) ;
		     case V_OpCode_Equal:	v4rh_neUpdate(vrhm,eName,V4RH_ETYPE_Tag,tcb.ilvl[tcb.ilx].input_ptr,V4RH_UPDOPER_Update) ; break ;
		     case V_OpCode_PlusEqual:	v4rh_neUpdate(vrhm,eName,V4RH_ETYPE_Tag,tcb.ilvl[tcb.ilx].input_ptr,V4RH_UPDOPER_Append) ; break ;
		   } ;
		}
		break ;
	      case _JS:
		if (!liri.level[liri.irX].okXct) break ;	/* Skip if we are not xcting this part */
		if (vrhm->scriptNum >= V4RH_SCRIPT_MAX)
		 { v_Msg(NULL,vrhm->errMessage,"@Section (%1U) line (%2d) - Too many(%3d) JS entries: %3U",secNameFull,line,cp,V4RH_SCRIPT_MAX) ; return(FALSE) ; } ;
		vrhm->script[vrhm->scriptNum] = v4rpp_AllocUC(UCstrlen(tcb.ilvl[tcb.ilx].input_ptr)) ;
		UCstrcpy(vrhm->script[vrhm->scriptNum],tcb.ilvl[tcb.ilx].input_ptr) ;
		vrhm->scriptNum++ ; break ; 
	      case _If:
		if (liri.irX+1 >= V4RH_MAXIFS) { v_Msg(NULL,vrhm->errMessage,"@Section (%1U) line (%2d) - too many nested 'ifs': %3U",secNameFull,line,cp) ; return(FALSE) ; } ;
		if (liri.level[liri.irX].okXct)		/* If we are already ignoring this chunk then don't bother to evaluate the If */
		 { LOGICAL ok ; liri.irX++ ; liri.level[liri.irX].okXct = v4rh_XctIfNest(vrhm,&tcb,&ok) ; if (!ok) return(FALSE) ; }
		 else { liri.irX++ ; liri.level[liri.irX].okXct = FALSE ; } ;
		  
		break ;
	      case _Echo:
		if (!liri.level[liri.irX].okXct)
		 { UCCHAR ebuf[2048] ; UCstrcpy(ebuf,UClit("*Echo: ")) ; UCstrcat(ebuf,tcb.ilvl[tcb.ilx].input_ptr) ;
		   vout_UCText(VOUT_Trace,0,ebuf) ;
		 }
		break ;
	      case _Else:
		if (liri.irX <= 0) { v_Msg(NULL,vrhm->errMessage,"@Section (%1U) line (%2d) - 'else' without prior 'if': %3U",secNameFull,line,cp) ; return(FALSE) ; } ;
		if (liri.level[liri.irX].repType != V4RH_IfRep_Type_If) { v_Msg(NULL,vrhm->errMessage,"@Section (%1U) line (%2d) - 'else' within 'repeat': %3U",secNameFull,line,cp) ; return(FALSE) ; } ;
		liri.level[liri.irX].okXct = liri.level[liri.irX - 1].okXct && !liri.level[liri.irX].okXct ;	/* Only flip to TRUE if all prior stack entries are also TRUE */
		break ;
	      case _End:
		if (liri.irX <= 0) { v_Msg(NULL,vrhm->errMessage,"@Section (%1U) line (%2d) - 'end' without prior 'if': %3U",secNameFull,line,cp) ; return(FALSE) ; } ;
#define SETFMLTAG(MVLOG) \
 if (UCnotempty(liri.level[liri.irX].eNameFML)) \
  { v4rh_neUpdate(vrhm,liri.level[liri.irX].eNameFML,V4RH_ETYPE_Tag,((MVLOG) ? liri.level[liri.irX].eNameMVal : liri.level[liri.irX].eNameLVal),V4RH_UPDOPER_Update) ; \
  } ; \
 if (ep != NULL) *ep = EOLbt ; \
 ep = liri.level[liri.irX].repBegin ; \
 goto endBigLoop ;
 
		switch(liri.level[liri.irX].repType)
		 { default:
		   case V4RH_IfRep_Type_If:
			liri.irX-- ; break ;
		   case V4RH_IfRep_Type_Sheet:
			if (vrhm->sheetCur->sheetNext == NULL) { liri.irX-- ; break ; } ;
			if (!v4rh_SetCurSheet(vrhm,vrhm->sheetCur->sheetNext)) return(FALSE) ;
/*			If we have tag to update then do it */
			SETFMLTAG(vrhm->sheetCur->sheetNext != NULL)
		   case V4RH_IfRep_Type_Header:
			if (vrhm->sheetCur->hdrCur == NULL) { liri.irX-- ; break ; } ;
			for (;;)
			 { if (vrhm->sheetCur->hdrCur->rowNext == NULL) { liri.irX-- ; break ; } ;
			   vrhm->sheetCur->hdrCur = vrhm->sheetCur->hdrCur->rowNext ;
			   vrhm->sheetCur->rowNum++ ; if (!v4rh_SetCurRow(vrhm,vrhm->sheetCur->hdrCur,V4RH_RowType_Header,NULL)) return(FALSE) ;
			   if (vrhm->sheetCur->hdrCur->skipRow) { vrhm->sheetCur->rowNum-- ; continue ; } ;
			   SETFMLTAG(vrhm->sheetCur->hdrCur->rowNext != NULL) ;
			 } ;
			break ;
		   case V4RH_IfRep_Type_Footer:
			if (vrhm->sheetCur->ftrCur == NULL) { liri.irX-- ; break ; } ;
			for (;;)
			 { if (vrhm->sheetCur->ftrCur->rowNext == NULL) { liri.irX-- ; break ; } ;
			   vrhm->sheetCur->ftrCur = vrhm->sheetCur->ftrCur->rowNext ;
			   vrhm->sheetCur->rowNum++ ; if (!v4rh_SetCurRow(vrhm,vrhm->sheetCur->ftrCur,V4RH_RowType_Footer,NULL)) return(FALSE) ;
			   if (vrhm->sheetCur->ftrCur->skipRow) { vrhm->sheetCur->rowNum-- ; continue ; } ;
			   SETFMLTAG(vrhm->sheetCur->ftrCur->rowNext != NULL) ;
			 } ;
			break ;
		   case V4RH_IfRep_Type_Row:
			if (vrhm->sheetCur->rowCur == NULL) { liri.irX-- ; break ; } ;
			if (vrhm->sheetCur->rowNum > 1)
			 { vout_UCTextFileX(vrhm->outFileX,0,UClit("</tr>")) ; vout_NLFileX(vrhm->outFileX) ; } ;
			for(;;)
			 { if (vrhm->sheetCur->rowCur == NULL ? TRUE : vrhm->sheetCur->rowCur->rowNext == NULL) { liri.irX-- ; break ; } ;
			   vrhm->sheetCur->rowCur = vrhm->sheetCur->rowCur->rowNext ;
			   vrhm->sheetCur->rowNum++ ; if (!v4rh_SetCurRow(vrhm,vrhm->sheetCur->rowCur,V4RH_RowType_Data,NULL)) return(FALSE) ;
			   if (vrhm->sheetCur->rowCur->skipRow) { vrhm->sheetCur->rowNum-- ; continue ; } ;
			   SETFMLTAG(vrhm->sheetCur->rowCur->rowNext != NULL)
			 } ;
			break ;
		   case V4RH_IfRep_Type_Cell:
			crow = (vrhm->sheetCur->ftrCur != NULL ? vrhm->sheetCur->ftrCur : (vrhm->sheetCur->rowCur == NULL ? vrhm->sheetCur->hdrCur : vrhm->sheetCur->rowCur)) ;
			if (crow == NULL ? TRUE : crow->skipRow) { liri.irX-- ; break ; } ;
			if (crow->cellCur == NULL ? TRUE : crow->cellCur->cellNext == NULL) { liri.irX-- ; break ; } ;
			crow->cellCur = crow->cellCur->cellNext ;
			crow->cellNum++ ;
			if (!v4rh_SetCurCell(vrhm,crow,crow->cellCur,NULL)) return(FALSE) ;
			SETFMLTAG(crow->cellCur->cellNext != NULL)
		   case V4RH_IfRep_Type_Title:
			vrhm->sheetCur->titleX++ ;
			if (vrhm->sheetCur->titleX >= vrhm->sheetCur->titleNum)
			 { liri.irX-- ; break ; } ;
			v4rh_SetCurTitle(vrhm) ;
			SETFMLTAG(vrhm->sheetCur->titleX+1 < vrhm->sheetCur->titleNum)
		 } ;
		break ;
	      case _Repeat:
		if (!liri.level[liri.irX].okXct)		/* If we are in conditional that is not being executed then tweak and ignore through the ##END */
		 { liri.irX++ ; liri.level[liri.irX].okXct = FALSE ;
		   break ;
		 } ;
/*		Set up a repeating subsection */
		liri.irX++ ; memset(&liri.level[liri.irX],0,sizeof liri.level[liri.irX]) ; liri.level[liri.irX].okXct = TRUE ;
		liri.level[liri.irX].repBegin = ep ;		/* Save 'code' to begin of this loop */
		v4lex_NextTkn(&tcb,0) ;
		switch (v4im_GetUCStrToEnumVal(tcb.UCkeyword,0))
		 { default:
		     v_Msg(NULL,vrhm->errMessage,"@Section (%1U) line (%2d) - invalid Repeat type: %3U",secNameFull,line,tcb.UCkeyword) ;
		     return(FALSE) ;
		   case _Source:
		     if (!v_UCFileOpen(&vrhm->iFile,vrhm->inpFile,UCFile_Open_Read,TRUE,vrhm->errMessage,0)) return(FALSE) ;
		     v4lex_InitTCB(vrhm->tcb,V4LEX_TCBINIT_NoStdIn) ; v4lex_NestInput(vrhm->tcb,&vrhm->iFile,vrhm->inpFile,V4LEX_InpMode_File) ;
		     for(;;)
		      { if (!v4lex_ReadNextLine(vrhm->tcb,NULL,NULL,NULL)) { break ; } ;
		        if (UCstrncmpIC(vrhm->tcb->ilvl[vrhm->tcb->ilx].input_ptr,UClit("<!DOCTYPE"),9) == 0) continue ;
/*			If we have '<xxxxx ...' or '</xxxxx>' where xxxxx is tag to be ignored, then ignore it! */
			if (vrhm->tcb->ilvl[vrhm->tcb->ilx].input_ptr[0] == UClit('<'))
			 { INDEX i = (vrhm->tcb->ilvl[vrhm->tcb->ilx].input_ptr[1] == UClit('/') ? 2 : 1) ;
			   if (UCstrncmpIC(&vrhm->tcb->ilvl[vrhm->tcb->ilx].input_ptr[i],vrhm->tagToIgnore,UCstrlen(vrhm->tagToIgnore)) == 0)
			    continue ;
			 } ;
			vout_UCTextFileX(vrhm->outFileX,0,vrhm->tcb->ilvl[vrhm->tcb->ilx].input_ptr) ;
			vout_NLFileX(vrhm->outFileX) ;
		      } ;
		     v_UCFileClose(&vrhm->iFile) ;
		     liri.level[liri.irX].okXct = FALSE ; break ;
		   case _Sheet:
		     v4rh_SetCurSheet(vrhm,vrhm->sheetFirst) ; liri.level[liri.irX].repType = V4RH_IfRep_Type_Sheet ;
		     break ;
		   case _Title:
		     if (vrhm->sheetCur->titleNum == 0)
		      { liri.level[liri.irX].repType = V4RH_IfRep_Type_If ; liri.level[liri.irX].okXct = FALSE ; liri.level[liri.irX].okXct = V4RH_IfRep_Type_None ; break ; } ;
		     vrhm->sheetCur->titleX = 0 ; liri.level[liri.irX].repType = V4RH_IfRep_Type_Title ;
		     v4rh_SetCurTitle(vrhm) ; liri.level[liri.irX].repType = V4RH_IfRep_Type_Title ;
		     break ;
		   case _Header:
		     vrhm->sheetCur->hdrCur = vrhm->sheetCur->hdrFirst ; vrhm->sheetCur->rowNum = 1 ;
		     if (vrhm->sheetCur->hdrCur == NULL) { liri.level[liri.irX].okXct = FALSE ; break ; } ;
		     if (!v4rh_SetCurRow(vrhm,vrhm->sheetCur->hdrCur,V4RH_RowType_Header,NULL)) return(FALSE) ;
		     liri.level[liri.irX].repType = V4RH_IfRep_Type_Header ;
		     break ;
		   case _Footer:
		     vrhm->sheetCur->ftrCur = vrhm->sheetCur->ftrFirst ;
		     if (vrhm->sheetCur->ftrCur == NULL) { liri.level[liri.irX].okXct = FALSE ; break ; } ;
		     if (!v4rh_SetCurRow(vrhm,vrhm->sheetCur->ftrCur,V4RH_RowType_Footer,NULL)) return(FALSE) ;
		     liri.level[liri.irX].repType = V4RH_IfRep_Type_Footer ;
		     break ;
		   case _RowAndData:
		     vrhm->sheetCur->rowCur = vrhm->sheetCur->rowFirst ; vrhm->sheetCur->rowNum = 1 ;
		     if (vrhm->sheetCur->rowCur == NULL) { liri.level[liri.irX].okXct = FALSE ; break ; } ;
{ struct V4RH__Row *row ; struct V4RH__Cell *cell ;
  UCCHAR bbuf[0x10000] ;
  for(row=vrhm->sheetCur->rowFirst;row!=NULL;row=row->rowNext)
   { ZUS(bbuf) ; vrhm->sheetCur->rowCur = row ; if (!v4rh_SetCurRow(vrhm,row,V4RH_RowType_Data,bbuf)) return(FALSE) ;
     if (row->skipRow) continue ;
     for(cell=row->cellFirst;cell!=NULL;cell=cell->cellNext)
      { row->cellCur = cell ; if (!v4rh_SetCurCell(vrhm,row,cell,bbuf)) return(FALSE) ;
      } ;
     UCstrcat(bbuf,UClit("</tr>")) ;
     vout_UCTextFileX(vrhm->outFileX,0,bbuf) ; vout_NLFileX(vrhm->outFileX) ;
   } ;
 liri.level[liri.irX].okXct = FALSE ; break ;
}
		   case _R:
		   case _Row:
		     vrhm->sheetCur->rowCur = vrhm->sheetCur->rowFirst ; vrhm->sheetCur->rowNum = 1 ;
		     if (vrhm->sheetCur->rowCur == NULL) { liri.level[liri.irX].okXct = FALSE ; break ; } ;
		     if (!v4rh_SetCurRow(vrhm,vrhm->sheetCur->rowCur,V4RH_RowType_Data,NULL)) return(FALSE) ;
		     liri.level[liri.irX].repType = V4RH_IfRep_Type_Row ;
		     break ;
		   case _C:
		   case _Cell:
		     crow = (vrhm->sheetCur->ftrCur != NULL ? vrhm->sheetCur->ftrCur : (vrhm->sheetCur->rowCur == NULL ? vrhm->sheetCur->hdrCur : vrhm->sheetCur->rowCur)) ;
		     if (crow == NULL) { liri.level[liri.irX].okXct = FALSE ; break ; } ;
		     crow->cellCur = crow->cellFirst ; crow->cellNum = 1 ;
		     if (crow->cellCur == NULL ? TRUE : crow->skipRow) { liri.level[liri.irX].okXct = FALSE ; break ; } ;
		     v4rh_SetCurCell(vrhm,crow,crow->cellCur,NULL) ;
		     liri.level[liri.irX].repType = V4RH_IfRep_Type_Cell ;
		     break ;
		 } ;
/*		Now parse possible: name=(first,middle,last) structure */
		v4lex_NextTkn(&tcb,V4LEX_Option_RetEOL) ;
		if (tcb.type != V4LEX_TknType_EOL)
		 { tcb.UCkeyword[V4RH_Name_Max] = UCEOS ; UCstrcpy(liri.level[liri.irX].eNameFML,tcb.UCkeyword) ;
		   v4lex_NextTkn(&tcb,0) ; if (tcb.opcode != V_OpCode_Equal) { };
		   v4lex_NextTkn(&tcb,0) ; if (tcb.opcode != V_OpCode_LParen) { };
		   v4lex_NextTkn(&tcb,0) ; if (!(tcb.opcode == V_OpCode_QString || tcb.opcode == V_OpCode_DQString)) { } ;
		   liri.level[liri.irX].eNameFVal = v4rpp_AllocUC(UCstrlen(tcb.UCstring)) ; UCstrcpy(liri.level[liri.irX].eNameFVal,tcb.UCstring) ;
/*		   Set the tag for the first value */
		   v4rh_neUpdate(vrhm,liri.level[liri.irX].eNameFML,V4RH_ETYPE_Tag,liri.level[liri.irX].eNameFVal,V4RH_UPDOPER_Update) ;
		   v4lex_NextTkn(&tcb,0) ; if (tcb.opcode != V_OpCode_Comma) { };
		   v4lex_NextTkn(&tcb,0) ; if (!(tcb.opcode == V_OpCode_QString || tcb.opcode == V_OpCode_DQString)) { } ;
		   liri.level[liri.irX].eNameMVal = v4rpp_AllocUC(UCstrlen(tcb.UCstring)) ; UCstrcpy(liri.level[liri.irX].eNameMVal,tcb.UCstring) ;
		   v4lex_NextTkn(&tcb,0) ; if (tcb.opcode != V_OpCode_Comma) { };
		   v4lex_NextTkn(&tcb,0) ; if (!(tcb.opcode == V_OpCode_QString || tcb.opcode == V_OpCode_DQString)) { } ;
		   liri.level[liri.irX].eNameLVal = v4rpp_AllocUC(UCstrlen(tcb.UCstring)) ; UCstrcpy(liri.level[liri.irX].eNameLVal,tcb.UCstring) ;
		 } ;
	    } ;
	   if (ep != NULL) *ep = EOLbt ;			/* Reset terminating character */
endBigLoop:
	   continue ;
	 } ;
	return(TRUE) ;
}

LOGICAL v4rh_SetCurSheet(vrhm,sheet)
  struct V4RH__Main *vrhm ;
  struct V4RH__Sheet *sheet ;
{ struct V4RH__NamedEntry *vrne ;
  UCCHAR tbuf[64] ;

#define DEFDBL(DBLFIELD) \
 UCsprintf(tbuf,UCsizeof(tbuf),UClit("%g"),sheet->DBLFIELD) ; \
 v4rh_neUpdate(vrhm,UClit(#DBLFIELD),V4RH_ETYPE_Tag,tbuf,V4RH_UPDOPER_Update) ;
 

	if (sheet->sheetNext == NULL)
	 { v4rh_neUpdate(vrhm,UClit("lastSheet"),V4RH_ETYPE_Tag,UClit("yes"),V4RH_UPDOPER_Update) ; } ;
	vrne = v4rh_neLookup(vrhm,UClit("sourceProgram")) ;
/*	If coming from V4/EchoS() and no colCap set then default to 1 */
	if (vrne == NULL ? FALSE : UCstrcmp(UClit("EchoS"),vrne->eVal) == 0)
	 { if (sheet->colCap == 0) sheet->colCap = 1 ; } ;
/*	Before we get into this, maybe convert initial rows into headers (EchoS() does not distinguish between the two) */
	if (sheet->hdrFirst == NULL && sheet->colCap > 0)
	 { struct V4RH__Row *row ; INDEX i ;
/*	   Position to the last header row */
	   for (i=1,row=sheet->rowFirst;i<sheet->colCap&&row->rowNext!=NULL;i++,row=row->rowNext) { } ;
/*	   Make first row the first header */
	   sheet->hdrFirst = sheet->rowFirst ;
/*	   Make the new first row the row after the last header, make the last header the last header */
	   sheet->rowFirst = row->rowNext ; row->rowNext = NULL ;
	 } ;
	vrhm->sheetCur = sheet ;
	DEFDBL(marginTop) DEFDBL(marginBottom) DEFDBL(marginLeft) DEFDBL(marginRight) DEFDBL(scale)
	if (sheet->gridLines != 0)
	 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("%d"),sheet->gridLines) ;
	   v4rh_neUpdate(vrhm,UClit("gridLines"),V4RH_ETYPE_Tag,tbuf,V4RH_UPDOPER_Update) ;
	 } ;
	v4rh_neUpdate(vrhm,UClit("orientation"),V4RH_ETYPE_Tag,(sheet->orientPage < 0 ? UClit("landscape") : (sheet->orientPage > 0 ? UClit("portrait") : UClit("undefined"))),V4RH_UPDOPER_Update) ;
	v4rh_neUpdate(vrhm,UClit("sheetName"),V4RH_ETYPE_Tag,sheet->sheetName,V4RH_UPDOPER_Update) ;
	UCsprintf(tbuf,UCsizeof(tbuf),UClit("%d"),sheet->titleNum) ;
	v4rh_neUpdate(vrhm,UClit("titleNum"),V4RH_ETYPE_Tag,tbuf,V4RH_UPDOPER_Update) ;
	v4rh_neUpdate(vrhm,UClit("haveFooter"),V4RH_ETYPE_Tag,(sheet->ftrFirst != NULL ? UClit("1") : UClit("0")),V4RH_UPDOPER_Update) ;
	return(TRUE) ;
}

LOGICAL v4rh_SetCurTitle(vrhm)
  struct V4RH__Main *vrhm ;
{ 
	v4rh_neUpdate(vrhm,UClit("titleLine"),V4RH_ETYPE_Tag,vrhm->sheetCur->ucTitles[vrhm->sheetCur->titleX],V4RH_UPDOPER_Update) ;
	return(TRUE) ;
}

LOGICAL v4rh_SetCurRow(vrhm,row,rowType,trBuf)
  struct V4RH__Main *vrhm ;
  struct V4RH__Row *row ;
  ETYPE rowType ;
  UCCHAR *trBuf ;
{
  UCCHAR tbuf[64],*cp,*ep,*arg, *td, lHdr[3], extraStyle[512] ; INDEX line ; LOGICAL ok ;
  UCCHAR *bgColor, *classId, *style, *eventInfo, *id, *ifCond, *pageBreak, *textColor ;
  
	row->rowType = rowType ;
	UCsprintf(tbuf,UCsizeof(tbuf),UClit("%d"),vrhm->sheetCur->rowNum) ;
	v4rh_neUpdate(vrhm,UClit("rowNum"),V4RH_ETYPE_Tag,tbuf,V4RH_UPDOPER_Update) ;
/*	If doing this the 'slow' way then get rid of any prior row tags */
	if (trBuf == NULL)
	 { DELTAG("backgroundColor") DELTAG("class") DELTAG("eventInfo") DELTAG("styleInfo") DELTAG("id") DELTAG("if") DELTAG("pageBreak") DELTAG("textColor") 
	 } else
	 { bgColor = NULL ; classId = NULL ; style = NULL ; eventInfo = NULL ; id = NULL ; ifCond = NULL ;  pageBreak = NULL ; textColor = NULL ;
	 } ;
/*	Loop thru all sheet information & set appropriate tag values */
	for(cp=row->rowInfo,ep=cp,line=row->lineNum+1;ep!=NULL;line++,cp=ep+1)
	 { ep = UCstrchr(cp,EOLbt) ; if (ep != NULL) *ep = UCEOS ;	/* Set terminating char to EOS - HAVE TO RESET!!! */
	   if (UCempty(cp)) continue ;
	   lHdr[0] = cp[0] ; lHdr[1] = cp[1] ; ; lHdr[2] = UCEOS ; arg = &cp[2] ;
#define DEFTAGORARG(ARGVAR,NAME) if (trBuf != NULL) { ARGVAR = arg ; } else { DEFTAG(NAME) ; } ; break ;
	   switch (v4im_GetUCStrToEnumVal(lHdr,0))
	    { default:		v_Msg(NULL,vrhm->errMessage,"@SetCurRow() - Invalid entry (%1U) at line %2d of input",lHdr,line) ; return(FALSE) ;
	      case _BC:		DEFTAGORARG(bgColor,"backgroundColor")
	      case _CL:		DEFTAGORARG(classId,"class")
	      case _CS:		DEFTAGORARG(style,"styleInfo")
	      case _ES:		return(TRUE) ;
	      case _EV:		DEFTAGORARG(eventInfo,"eventInfo") ; break ;
	      case _Id:		DEFTAGORARG(id,"id")
	      case _If:
		{ struct V4LEX__TknCtrlBlk tcb ;
		  v4lex_InitTCB(&tcb,V4LEX_TCBINIT_NoStdIn) ; v4lex_NestInput(&tcb,NULL,arg,V4LEX_InpMode_String) ;
		  row->skipRow = !v4rh_XctIfNest(vrhm,&tcb,&ok) ;
		  if (!ok) return(FALSE) ;
		  if (row->skipRow) return(TRUE) ;
		}
		break ;
	      case _LE:		row->level = UCstrtol(arg,&td,10) ;
				switch ((*td == UClit(':')) ? UCstrtol(td+1,&td,10) : UNUSED)
				 { default:	
				   case V4IM_BaA_CondAfter:	row->levelAtEnd = FALSE ; break ;
				   case V4IM_BaA_CondEnd1:
				   case V4IM_BaA_CondEnd2:
				   case V4IM_BaA_CondBefore:	row->levelAtEnd = TRUE ; break ;
				 } ; 
				break ;
	      case _PB:		vrhm->pageBreak = TRUE ; DEFTAGORARG(pageBreak,"pageBreak")
	      case _TC:		DEFTAGORARG(textColor,"textColor")
		break ;
	    } ;
	 } ;
	if (trBuf == NULL) return(TRUE) ;
/*	Build up tr (or thead) tag */
	switch (rowType)
	 { case V4RH_RowType_Header:	UCstrcpy(trBuf,UClit("<tr")) ; break ;
	   case V4RH_RowType_Data:	UCstrcpy(trBuf,UClit("<tr")) ; break ;
	   case V4RH_RowType_Footer:	UCstrcpy(trBuf,UClit("<tr")) ; break ;
	 } ;
#define TRVAL(ARGVAR,NAME) if (ARGVAR != NULL) { UCstrcat(trBuf,UClit(" ") UClit(NAME) UClit("='")) ; UCstrcat(trBuf,ARGVAR) ; UCstrcat(trBuf,UClit("'")) ; }
	TRVAL(classId,"class") TRVAL(style,"styleInfo") TRVAL(id,"id")
	ZUS(extraStyle)
	if (bgColor != NULL) { UCstrcat(extraStyle,UClit("background-color: ")) ; UCstrcat(extraStyle,bgColor) ; UCstrcat(extraStyle,UClit(";")) ; } ;
	if (textColor != NULL) { UCstrcat(extraStyle,UClit("color: ")) ; UCstrcat(extraStyle,textColor) ; UCstrcat(extraStyle,UClit(";")) ; } ;
	if (pageBreak != NULL)
	 { UCstrcat(extraStyle,(pageBreak[0] == UClit('A') ? UClit("page-break-after: always;") : UClit("page-break-before: always;"))) ; } ;
	if (UCnotempty(extraStyle)) { UCstrcat(trBuf,UClit(" style='")) ; UCstrcat(trBuf,extraStyle) ; UCstrcat(trBuf,UClit("'")) ; } ;
	if (eventInfo != NULL) { UCstrcat(trBuf,UClit(" ")) ; UCstrcat(trBuf,eventInfo) ; } ;
	UCstrcat(trBuf,UClit(">")) ;
	return(TRUE) ;
}

LOGICAL v4rh_SetCurCell(vrhm,row,cell,tdBuf)
  struct V4RH__Main *vrhm ;
  struct V4RH__Row *row ;
  struct V4RH__Cell *cell ;
  UCCHAR *tdBuf ;
{ struct V4RH__DimInfo *vdi ;
  UCCHAR tbuf[64],*cp,*ep,*arg,*tc,*mask, lHdr[3], td[0x10000],rgbColor[24] ; INDEX line ;
  UCCHAR *className, *idName, *styleInfo, *noteInfo, *eventInfo, *urlInfo, *target, *imgURL ;
  static INDEX idnCount = 0 ;
  int inum ; double dbl ; LOGICAL logical ; CALENDAR cal ; LENMAX colSpan ;
  
	className = NULL ; idName = NULL ; styleInfo = NULL ; noteInfo = NULL ; eventInfo = NULL ; urlInfo = NULL ; target = NULL ; imgURL = NULL ;
	UCsprintf(tbuf,UCsizeof(tbuf),UClit("%d"),row->cellNum) ;
	v4rh_neUpdate(vrhm,UClit("cellNum"),V4RH_ETYPE_Tag,tbuf,V4RH_UPDOPER_Update) ;
	vdi = (cell->dimNum >= 0 && cell->dimNum < V4RH_DIM_MAX ? vrhm->vdi[cell->dimNum] : NULL) ;
/*	If vdi undefined then default with generic alpha */
	if (vdi == NULL)
	 { if (++idnCount < 10) printf("[Invalid dimension number (%d) encountered in cell, defaulting to Alpha]\n",cell->dimNum) ;
	   vdi = vrhm->vdi[cell->dimNum=V4RH_DimNum_Alpha] ;
	 } ;
	mask = (vdi->maskX == UNUSED ? NULL : vrhm->vmi->mask[vdi->maskX].formatMask) ;
	colSpan = (vdi->vfi != NULL ? vdi->vfi->colSpan : 0) ;
/*	Loop thru all sheet information & set appropriate tag values */
	for(cp=cell->cellInfo,ep=cp,line=cell->lineNum+1;ep!=NULL;line++,cp=ep+1)
	 { ep = UCstrchr(cp,EOLbt) ;
	   if (UCempty(cp)) continue ;
	   if (ep != NULL) *ep = UCEOS ;			/* Set terminating char to EOS - HAVE TO RESET!!! */
	   lHdr[0] = cp[0] ; lHdr[1] = cp[1] ; ; lHdr[2] = UCEOS ; arg = &cp[2] ;
	   switch (v4im_GetUCStrToEnumVal(lHdr,0))
	    { default:		v_Msg(NULL,vrhm->errMessage,"@SetCurCell() - Invalid entry (%1U) at line %2d of input",lHdr,line) ; return(FALSE) ;
	      case _CS:		styleInfo = arg ; break ;
	      case _CL:		className = arg ; break ;
	      case _EV:		eventInfo = arg ; break ;
	      case _HT:
		{ INDEX eX ;
		  eX = (vrhm->writeHTMLX % V4RH_EMBED_HTML_MAX) ;
		  if (vrhm->embedHTML[eX] == NULL) vrhm->embedHTML[eX] = v4rpp_AllocUC(1024) ;
		  if (UCstrlen(arg) > 1023) arg[1023] = UCEOS ;
		  UCstrcpy(vrhm->embedHTML[eX],arg) ; vrhm->writeHTMLX++ ;
		  break ;
		}
	      case _Id:		idName = arg ; break ;
	      case _IM:
/*		We should have url<tab>localfile<tab>rows<tab>columns */
		{ UCCHAR *b = UCstrchr(arg,'\t') ; if (b != NULL) { *b = UCEOS ; } else { break ; } ;
		  if (UCnotempty(arg)) imgURL = arg ;
		}
		break ;
	      case _No:		noteInfo = arg ; break ;
	      case _NT:
	      case _PB:		vrhm->pageBreak = TRUE ; break ;
	      case _SP:		colSpan = v_ParseInt(arg,&tc,10) ; break ;
	      case _TA:		target = arg ; break ;
	      case _UR:		urlInfo = arg ;	break ;
	    } ;
	 } ;
/*	Construct the <td> (or <th>) for the cell */

	ZUS(td) ;
#define CAT(STR) UCstrcat(td,STR) ;
#define CATL(STR) UCstrcat(td,UClit(STR)) ;
	switch (row->rowType)
	 {
	   case V4RH_RowType_Header:	CATL("<th") ; break ;
	   case V4RH_RowType_Data:	CATL("<td") ; break ;
	   case V4RH_RowType_Footer:	CATL("<td") ; break ;
	 } ;
	
	if (idName != NULL) { CATL(" id ='") CAT(idName) CATL("'") } ;
	if (className != NULL) { CATL(" class='") CAT(className) CATL("'") }
	 else { INDEX cssX = (cell->useColGroup ? UNUSED : vrhm->vdi[cell->dimNum]->cssX) ;
		if (cssX != UNUSED)
		 { CATL(" class='") CAT(vrhm->vce->css[cssX].className) CATL("'") } ;
	      } ;
	if (styleInfo != NULL) { CATL(" style='") CAT(styleInfo) CATL("'") } ;
	if (noteInfo != NULL) { CATL(" title='") CAT(noteInfo) CATL("'") } ;
	if (eventInfo != NULL)
	 { CATL(" ") CAT(eventInfo) } ;
	if (colSpan != 0) { UCsprintf(tbuf,UCsizeof(tbuf),UClit(" colspan='%d'"),colSpan) ; CAT(tbuf) } ;
	CATL(">")
	if (urlInfo != NULL && !vrhm->forPrinting)
	 { v_ParseInt(urlInfo,&tc,10) ;
	   CATL("<a href='")
	   if (*tc == UClit(':'))
	    { CATL("javascript:v4r.url(") *tc = UCEOS ; CAT(urlInfo) *tc = UClit(':') ;
	      CATL(",\"") CAT(tc+1) CATL("\"")
	      CATL(",\"") ; if (target != NULL) { CAT(target) ; } ; CATL("\"") ;
	      CATL(")'>")
	    } else 
	    { CAT(urlInfo) CATL("'>")
	    } ;
	 } ;
/*	Now handle the actual cell value */
	if (cell->cellValue == NULL)
	 cell->cellValue = UClit("") ;	/* If no value then default to empty string */
/*	If we have an image for the cell, then it trumps any value we may also have */
	switch (imgURL != NULL ?  -1 : (cell->pntType == UNUSED ? vdi->v4PntType : cell->pntType))
	 { default:
		v_Msg(NULL,vrhm->errMessage,"@Invalid point type (%1d) cell (%2U)",vdi->v4PntType,cell->cellValue) ; return(FALSE) ;
	   case -1:			/* Here to handle image */
		UCstrcat(td,UClit("<img src='")) ; UCstrcat(td,imgURL) ; UCstrcat(td,UClit("'/>")) ;
		break ;
//	   case V4DPI_PntType_MIDASUDT:
//		inum = v_ParseInt(cell->cellValue,&tc,10) ;
//		cal = UDTtoCAL(inum) ;
//		v_FormatDate(&cal,V4DPI_PntType_Calendar,VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,mask,&td[UCstrlen(td)]) ;
//		break ;
	   case V4DPI_PntType_UDate:
	   case V4DPI_PntType_UDT:
	   case V4DPI_PntType_Calendar:
		cal = v_ParseDbl(cell->cellValue,&tc) ;
		v_FormatDate(&cal,V4DPI_PntType_Calendar,VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,mask,&td[UCstrlen(td)]) ;
		break ;
	   case V4DPI_PntType_UYear:
	   case V4DPI_PntType_Int:
	   case V4DPI_PntType_Delta:
		inum = v_ParseInt(cell->cellValue,&tc,10) ;
		v_FormatInt(inum,mask,&td[UCstrlen(td)],rgbColor) ;
		break ;
	   case V4DPI_PntType_Logical:
		logical = v_ParseLog(cell->cellValue,&tc) ;
		UCstrcat(td,(logical ? UClit("Yes") : UClit("No"))) ;
		break ;
	   case V4DPI_PntType_Real:
		dbl = v_ParseDbl(cell->cellValue,&tc) ;
		if (fabs(dbl) < vrhm->epsilon) dbl = 0 ;
		v_FormatDbl(dbl,mask,&td[UCstrlen(td)],rgbColor) ;
		break ;
	   case V4DPI_PntType_BinObj:
	   case V4DPI_PntType_UOM:
	   case V4DPI_PntType_UOMPer:
	   case V4DPI_PntType_UTime:
	   case V4DPI_PntType_UMonth:
	   case V4DPI_PntType_UQuarter:
	   case V4DPI_PntType_UPeriod:
	   case V4DPI_PntType_UWeek:
	   case V4DPI_PntType_List:
	   case V4DPI_PntType_Dict:
	   case V4DPI_PntType_XDict:
	   case V4DPI_PntType_Char:
	   case V4DPI_PntType_BigText:
	   case V4DPI_PntType_TeleNum:
	   case V4DPI_PntType_Shell:
	   case V4DPI_PntType_Isct:
	   case V4DPI_PntType_Country:
	   case V4DPI_PntType_GeoCoord:
	   case V4DPI_PntType_UCChar:
		{ UCCHAR *ub,*eb ; INDEX eX ;
		  eb = &td[UCstrlen(td)] ;
		  for(ub=cell->cellValue;*ub!=UCEOS;ub++)
		   { switch (*ub)
		      { default:		*(eb++) = *ub ; break ;
		        case UCEOLbt:		*eb = UCEOS ; UCstrcat(eb,UClit("<br/>")) ; eb += 5 ; break ;
/*			  Handling '&' is difficult - do we pass as-is or do we convert it to '&amp;' ? */
/*			  Current rule: if next character is '#' then assume we got valid '&#nnnn;' & let it go through as-is */
		        case UClit('&'):	if (*(ub + 1) == UClit('#')) { *(eb++) = *ub ; break ; } ;
						*eb = UCEOS ;  UCstrcat(eb,UClit("&amp;")) ; eb += 5 ; break ;
		        case UClit('<'):	*eb = UCEOS ;  UCstrcat(eb,UClit("&lt;")) ; eb += 4 ; break ;
		        case 27:		/* Escape */
/*			   Take the next 'embedded html' entry (if we have it) */
			   if (vrhm->readHTMLX >= vrhm->writeHTMLX) break ;
			   eX = (vrhm->readHTMLX % V4RH_EMBED_HTML_MAX) ;
			   *eb = UCEOS ; UCstrcat(eb,vrhm->embedHTML[eX]) ; eb += UCstrlen(vrhm->embedHTML[eX]) ;
			   vrhm->readHTMLX++ ;
		      } ;
		   } ; *eb = UCEOS ;
		}
		break ;
	 } ;
	if (urlInfo != NULL) { CATL("</a>") } ;
	if (!vrhm->minimizeHTML)
	 { switch (row->rowType)
	    {
	      case V4RH_RowType_Header:	CATL("</th>") ; break ;
	      case V4RH_RowType_Data:	CATL("</td>") ; break ;
	      case V4RH_RowType_Footer:	CATL("</td>") ; break ;
	    } ;
	 } ;
	
/*	Relink to current cell & format, etc. */
	if (tdBuf != NULL)
	 { UCstrcat(tdBuf,td) ;
	 } else { v4rh_neUpdate(vrhm,UClit("cellValue"),V4RH_ETYPE_Tag,td,V4RH_UPDOPER_Update) ; } ;
	return(TRUE) ;
}

/*	v4rh_ParseCellValue - Parses cell value */
/*	Forms: letter:value, dimNum:value, dimNum.pntType:value */
void v4rh_ParseCellValue(vrhm,cell,cellValue,lType)
  struct V4RH__Main *vrhm ;
  struct V4RH__Cell *cell ;
  UCCHAR *cellValue ;
  enum DictionaryEntries lType ;
{
  UCCHAR *tc,*cv ;
  
	if (UCempty(cellValue))
	 { cell->dimNum = V4RH_DimNum_Alpha ; cell->pntType = UNUSED ; return ; } ;
	cell->dimNum = v_ParseInt(cellValue,&tc,10) ; cv = tc + 1 ;
	if (*tc == UClit('.'))		/* If got '.' then what follows is cell-specific point type */
	 { cell->pntType = v_ParseInt(tc+1,&tc,10) ; cv = tc + 1 ;
	 } else { cell->pntType = UNUSED ; } ;
	if (*tc != UClit(':'))
	 { tc = UCstrchr(cellValue,'.') ;
	   if (tc != NULL)
	    { cell->pntType = v_ParseInt(tc+1,&tc,10) ; cv = tc + 1 ;
	    } else { cell->pntType = UNUSED ; tc = UCstrchr(cellValue,':') ; } ;
	   if (tc == NULL)		/* Does not have 'x:' prefix */
	    { cell->dimNum = V4RH_DimNum_Alpha ; cell->pntType = V4DPI_PntType_Char ;
	      cv = cellValue ;
	    } else
	    { switch (UCTOUPPER(cellValue[0]))
	       { default:
	         case UClit('A'):	cell->dimNum = V4RH_DimNum_Alpha ; cell->pntType = V4DPI_PntType_Char ; break ;
	         case UClit('D'):	cell->dimNum = V4RH_DimNum_Date ; cell->pntType = V4DPI_PntType_UDate ; break ;
	         case UClit('F'):	cell->dimNum = V4RH_DimNum_Float ; cell->pntType = V4DPI_PntType_Real ; break ;
	         case UClit('I'):	cell->dimNum = V4RH_DimNum_Int ; cell->pntType = V4DPI_PntType_Int ; break ;
	         case UClit('L'):	cell->dimNum = V4RH_DimNum_Logical ; cell->pntType = V4DPI_PntType_Logical ; break ;
	         case UClit('X'):	cell->dimNum = V4RH_DimNum_XL ; cell->pntType = V4DPI_PntType_SSVal ; break ;
	       } ; cv = &cellValue[2] ;
	    } ;
	 } ;
	if (lType == _EX)		/* Parsing Excel formula ? */
	 { cell->cellFormula = v4rpp_AllocUC(UCstrlen(cv)) ; UCstrcpy(cell->cellFormula,cv) ;
	 } else { cell->cellValue = v4rpp_AllocUC(UCstrlen(cv)) ; UCstrcpy(cell->cellValue,cv) ; } ;

	return ;
}

/*	Parses & Executes an "IF" statement, returns TRUE or FALSE (if (!ok) then error) */
LOGICAL v4rh_XctIfNest(vrhm,tcb,ok)
  struct V4RH__Main *vrhm ;
  struct V4LEX__TknCtrlBlk *tcb ;
  LOGICAL *ok ;
{ 
  struct V4RH__NamedEntry *vrne1,*vrne2,vrneDflt ;
  enum DictionaryEntries operSym ;
  UCCHAR compareOper[64] ;

	*ok = TRUE ;		/* Assume it is all going to work out */
/*	If in conditional and not executing then just push another FALSE onto IF stack */
/*	Parse expression: 'value rel value' where xxx */
	v4lex_NextTkn(tcb,0) ;
/*	Do we have a unary operator ? */
	switch (tcb->type == V4LEX_TknType_Keyword ? v4im_GetUCStrToEnumVal(tcb->UCkeyword,0) : UNUSED)
	 { default:
	     break ;		/* Not a unary operator */
	   case _Defined:
	     v4lex_NextTkn(tcb,0) ;
	     return(v4rh_neLookup(vrhm,tcb->UCkeyword) != NULL) ;
	   case _Undefined:
	     v4lex_NextTkn(tcb,0) ;
	     return(v4rh_neLookup(vrhm,tcb->UCkeyword) == NULL) ;
	 } ;
/*	Here for infix-binary operator */
	if (tcb->type != V4LEX_TknType_Keyword)
	 { v_Msg(NULL,vrhm->errMessage,"@Expecting keyword in IF(1)") ; *ok = FALSE ; return(FALSE) ; } ;
	vrne1 = v4rh_neLookup(vrhm,tcb->UCkeyword) ;	/* Lookup first operand */
/*	Check for ",default-value" */
	if (*tcb->ilvl[tcb->ilx].input_ptr != UClit(','))
	 { if (vrne1 == NULL) { v_Msg(NULL,vrhm->errMessage,"@Unknown tag (%1U) in IF",tcb->UCkeyword) ; *ok = FALSE ; return(FALSE) ; } ;
	 } else if (vrne1 != NULL)	/* Have default but don't need it */
	 { v4lex_NextTkn(tcb,0) ; v4lex_NextTkn(tcb,0) ;
	 } else				/* Have default - fake into vrne1->eVal */
	 { vrne1 = &vrneDflt ; UCstrcpy(vrne1->eName,tcb->UCkeyword) ;
	   v4lex_NextTkn(tcb,0) ; v4lex_NextTkn(tcb,0) ;
	   switch (tcb->type)
	    { case V4LEX_TknType_Integer:	if (tcb->decimal_places == 0) { UCsprintf(tcb->UCstring,25,UClit("%d"),tcb->integer) ; }
						 else { tcb->floating = tcb->integer / pow(10.0,tcb->decimal_places) ;
							UCsprintf(tcb->UCstring,25,UClit("%g"),tcb->floating) ;
						      } ;
						break ;
	      case V4LEX_TknType_Float:		UCsprintf(tcb->UCstring,25,UClit("%g"),tcb->floating) ; break ;
	    } ;
	   vrne1->eVal = v4mm_AllocUC(UCstrlen(tcb->UCstring)) ;
	   UCstrcpy(vrne1->eVal,tcb->UCstring) ;
	 } ;
	v4lex_NextTkn(tcb,0) ;
	if (tcb->type != V4LEX_TknType_Keyword)
	 { v_Msg(NULL,vrhm->errMessage,"@Expecting keyword in IF(2)") ; *ok = FALSE ; return(FALSE) ; } ;
	UCstrcpy(compareOper,tcb->UCkeyword) ; operSym = v4im_GetUCStrToEnumVal(tcb->UCkeyword,0) ;
	v4lex_NextTkn(tcb,0) ;
	switch (tcb->type)
	 { default:
		v_Msg(NULL,vrhm->errMessage,"@Invlaid value in IF(3)") ; *ok = FALSE ; return(FALSE) ;
	   case V4LEX_TknType_String:
		break ;
	   case V4LEX_TknType_Keyword:	
		if ((vrne2 = v4rh_neLookup(vrhm,tcb->UCkeyword)) == NULL)
		 { v_Msg(NULL,vrhm->errMessage,"@Unknown tag (%1U) in IF",tcb->UCkeyword) ; *ok = FALSE ; return(FALSE) ; } ;
		UCstrcpy(tcb->UCkeyword,vrne2->eVal) ;
		break ;
	   case V4LEX_TknType_Integer:
		UCsprintf(tcb->UCstring,25,UClit("%d"),tcb->integer) ; break ;
	   case V4LEX_TknType_Float:
		UCsprintf(tcb->UCstring,25,UClit("%g"),tcb->floating) ; break ;
	 } ;
#define RELCOMP(RELEXP) \
 { double d1,d2 ; UCCHAR *b ; \
   d1 = v_ParseDbl(vrne1->eVal,&b) ; if (*b != UCEOS) { v_Msg(NULL,vrhm->errMessage,"@Invalid numeric value (%1U=%2U) on LHS",vrne1->eName,vrne1->eVal) ; *ok = FALSE ; return(FALSE) ; } ; \
   d2 = v_ParseDbl(tcb->UCstring,&b) ; if (*b != UCEOS) { v_Msg(NULL,vrhm->errMessage,"@Invalid numeric value (%1U) on RHS",tcb->UCstring) ; *ok = FALSE ; return(FALSE) ; } ; \
   if (wantTrace) { v_Msg(NULL,vrhm->errMessage,"@*Comparing: %1U(%2U) %3U %4U -> %5d\n",vrne1->eName,vrne1->eVal,compareOper,tcb->UCstring,(RELEXP)) ; vout_UCText(VOUT_Trace,0,vrhm->errMessage) ; } ; \
   return(RELEXP) ; \
 }

	switch (operSym)
	 { default:			v_Msg(NULL,vrhm->errMessage,"@Unknown If operator: %1U",operSym) ; *ok = FALSE ; return(FALSE) ;
	   case _LT:	RELCOMP(d1<d2)
	   case _LE:	RELCOMP(d1<=d2)
	   case _EQ:	RELCOMP(d1==d2)
	   case _NE:	RELCOMP(d1!=d2)
	   case _GE:	RELCOMP(d1>=d2)
	   case _GT:	RELCOMP(d1>d2)
	   case _Div:
		{ int i1,i2 ; UCCHAR *b ;
		  i1 = v_ParseInt(vrne1->eVal,&b,10) ; if (*b != UCEOS) { v_Msg(NULL,vrhm->errMessage,"@Invalid integer value (%1U=%2U) on LHS of DIV",vrne1->eName,vrne1->eVal) ; *ok = FALSE ; return(FALSE) ; } ;
		  i2 = v_ParseInt(tcb->UCstring,&b,10) ; if (*b != UCEOS) { v_Msg(NULL,vrhm->errMessage,"@Invalid integer value (%1U) on RHS of DIV",tcb->UCstring) ; *ok = FALSE ; return(FALSE) ; } ;
		  return((i1 % i2) == 0) ;
		}
		break ;
	 } ;
/*	Should not get here */
	*ok = FALSE ;
	v_Msg(NULL,vrhm->errMessage,"@Failed in v4rh_XctIfNest() for unknown reasons") ;
	return(FALSE) ;
}


LOGICAL v4rh_XctSectionLine(vrhm,lXct,secName,line)
  struct V4RH__Main *vrhm ;
  UCCHAR *lXct ;
  UCCHAR *secName ; INDEX line ;
{
  struct V4RH__NamedEntry *vrne ;
  UCCHAR *p, *t1, *t2, eName[V4RH_Name_Max+1] ; INDEX i,eX ;

	for(p=lXct;*p!=UCEOS;p=t2+2)
	 {

	   t1 = UCstrpbrk(p,UClit("#\033")) ;
	   if (t1 == NULL) { vout_UCTextFileX(vrhm->outFileX,0,p) ; break ; } ;
	   if (*t1 == UClit('\033'))
	    { *t1 = UCEOS ; vout_UCTextFileX(vrhm->outFileX,0,p) ; *t1 = UClit('\033') ;
	      if (vrhm->readHTMLX < vrhm->writeHTMLX)
	       { eX = (vrhm->readHTMLX % V4RH_EMBED_HTML_MAX) ;
		 vout_UCTextFileX(vrhm->outFileX,0,vrhm->embedHTML[eX]) ;
		 vrhm->readHTMLX++ ; t2 = t1 - 1 ; continue ;
	       } ;
	    } ;
/*	   Look for next '##xxx##' construct */
	   t1 = UCstrstr(p,UClit("##")) ;
	   if (t1 == NULL) { vout_UCTextFileX(vrhm->outFileX,0,p) ; break ; } ;
	   *t1 = UCEOS ; vout_UCTextFileX(vrhm->outFileX,0,p) ; *t1 = UClit('#') ;
	   t2 = UCstrstr(t1+2,UClit("##")) ;
	   if (t2 == NULL) { v_Msg(NULL,vrhm->errMessage,"@Section (%1U) line (%2d) - No ending '##' on tag (%3U)",secName,line,t1) ; return(FALSE) ; } ;
	   for(i=0,t1=t1+2;t1!=t2;t1++,i++) { eName[i] = *t1 ; } ; eName[i] = UCEOS ;
/*	   If a section then call XctSection to handle */
	   if (UCstrncmpIC(eName,UClit("SECTION:"),8) == 0)
	    { if (!v4rh_XctSection(vrhm,&eName[8])) return(FALSE) ;
	    } else
	    { vrne = v4rh_neLookup(vrhm,eName) ;
	      if (vrne == NULL)
	       { v_Msg(NULL,vrhm->errMessage,"@Section (%1U) line (%2d) - tag (%3U) not defined",secName,line,eName) ; return(FALSE) ; } ;
	      switch (vrne->eType)
	       { 
		 case V4RH_ETYPE_Tag:
		   if (!v4rh_XctSectionLine(vrhm,vrne->eVal,secName,line)) return(FALSE) ;
		   break ;
		 case V4RH_ETYPE_Section:
		 case V4RH_ETYPE_GenCSS:
		   v4rh_GenerateCSS(vrhm) ; break ;
		 case V4RH_ETYPE_GenJS:
		   v4rh_GenerateJS(vrhm) ; break ;
		 case V4RH_ETYPE_GenColGroup:
		   v4rh_GenerateColGroups(vrhm,vrhm->sheetCur) ; break ;
		 case V4RH_ETYPE_GenHTMLInclude:
		   v4rh_GenerateHTMLInclude(vrhm) ; break ;
		 case V4RH_ETYPE_GenIncludeTop:
		   v4rh_GenerateIncludeTop(vrhm) ; break ;
		 case V4RH_ETYPE_GenIncludeBottom:
		   v4rh_GenerateIncludeBottom(vrhm) ; break ;
		 case V4RH_ETYPE_GenLinkInclude:
		   v4rh_GenerateLinkInclude(vrhm) ; break ;
	       } ;
	    } ;
	 } ;
	return(TRUE) ;
}

/*	R O U T I N E S   T O   R E A D   V 4 R   F I L E	*/


ETYPE v4rh_ReadV4RPass1(vrhm)
  struct V4RH__Main *vrhm ;
{
  struct V4LEX__TknCtrlBlk *tcb ;		/* Token control block for various inputs */
  struct V4RH__Cell *cell, *cellPrior ;
  struct V4RH__Row *hdr, *hdrPrior ;
  struct V4RH__Row *row, *rowPrior ;
  struct V4RH__Row *ftr, *ftrPrior ;
  struct V4RH__Row *crow ;
  struct V4RH__Sheet *sheet, *sheetPrior ;
  INDEX i ; LOGICAL ok ; UCCHAR *arg ; UCCHAR multiLineBuf[V4LEX_Tkn_InpLineMax] ;
  
	sheet = NULL ; sheetPrior = NULL ;
	row = NULL ; rowPrior = NULL ;
	hdr = NULL ; hdrPrior = NULL ;
	ftr = NULL ; ftrPrior = NULL ;
	cell = NULL ; cellPrior = NULL ;
	tcb = vrhm->tcb ; ok = TRUE ;
	vrhm->nextPass = FALSE ;
	for(;!vrhm->nextPass;)
	 { 
	 
	 
	   LOGICAL continued,multiLine,eof ; INDEX iLen ;
	   multiLine = FALSE ; eof = FALSE ;
	   for(continued=TRUE;continued;)
	    { 
	      if (vrhm->iLineLookAhead) { vrhm->iLineLookAhead = FALSE ; }
	       else { if (!v4lex_ReadNextLine(tcb,NULL,NULL,NULL)) { eof = TRUE ; break ; } ; } ;
	      iLen = UCstrlen(tcb->ilvl[tcb->ilx].input_ptr) ;
/*	      Do we have multi-line input ? */
	      continued = (tcb->ilvl[tcb->ilx].input_ptr[iLen-1] == UClit('\\')) ;
	      if (!continued && !multiLine) break ;
	      if (continued) tcb->ilvl[tcb->ilx].input_ptr[iLen-1] = UCEOS ;
	      if (!multiLine) { UCstrcpy(multiLineBuf,tcb->ilvl[tcb->ilx].input_ptr) ; multiLine = TRUE ; }
	       else { UCstrcat(multiLineBuf,UCEOLbts) ; UCstrcat(multiLineBuf,tcb->ilvl[tcb->ilx].input_ptr) ; } ;
	    } ; if (eof) break ;
	   if (multiLine)
	    tcb->ilvl[tcb->ilx].input_ptr = multiLineBuf ;
	   if (tcb->ilvl[tcb->ilx].input_ptr[0] == UClit('!') || tcb->ilvl[tcb->ilx].input_ptr[0] == UClit('/') || UCempty(tcb->ilvl[tcb->ilx].input_ptr)) continue ;
/*	   Better have a section */
	   lHdr[0] = tcb->ilvl[tcb->ilx].input_ptr[0] ; lHdr[1] = tcb->ilvl[tcb->ilx].input_ptr[1] ; ; lHdr[2] = UCEOS ;
	   if (v4im_GetUCStrToEnumVal(lHdr,0) != DE(BS))
	    { 
/*	      If we failed on first line & it starts with a '<' then assume we already have HTML */
	      if (tcb->ilvl[tcb->ilx].UCFile->lineNum == 1 && lHdr[0] == UClit('<'))
	       { UCCHAR eName[V4RH_Name_Max+1], *eb ;
		 if (UCstrncmpIC(tcb->ilvl[tcb->ilx].input_ptr,UClit("<!DOCTYPE"),9) == 0)
		  { for(;;)
		     { if (!v4lex_ReadNextLine(tcb,NULL,NULL,NULL)) { v_Msg(NULL,vrhm->errMessage,"@Unexpected EOF in input file") ; return(0) ; } ;
		       for(;*tcb->ilvl[tcb->ilx].input_ptr==UClit(' ');tcb->ilvl[tcb->ilx].input_ptr++) { } ;
		       if (UCnotempty(tcb->ilvl[tcb->ilx].input_ptr)) break ;
		     } ;
		  } ;
/*		 Create section name based on the first HTML tag we see */
	         UCstrcpy(eName,UClit("main.html.")) ;
	         eb = UCstrpbrk(tcb->ilvl[tcb->ilx].input_ptr,UClit(" >")) ;
	         if (eb != NULL)
	          { if (*eb == UClit('>')) { *eb = UCEOS ; eb = NULL ; }	/* If matched on '>' then we are at end of line */
	             else { *eb = UCEOS ; } ;
	          } ;
	         if (UCstrlen(tcb->ilvl[tcb->ilx].input_ptr) >= UCsizeof(eName)-UCstrlen(eName)-4)
	          tcb->ilvl[tcb->ilx].input_ptr[UCsizeof(eName)-UCstrlen(eName)-4] = UCEOS ;
	         UCstrcat(eName,&tcb->ilvl[tcb->ilx].input_ptr[1]) ;
	         UCstrcpy(vrhm->tagToIgnore,&tcb->ilvl[tcb->ilx].input_ptr[1]) ;	/* Save this tag name so we can ignore it on output pass 2 */
/*		 Parse anything else in tag as sequence of: sym=value */
		 if (eb != NULL)
		  { tcb->ilvl[tcb->ilx].input_ptr = eb + 1 ;
		    for(tcb->ilvl[tcb->ilx].input_ptr=eb+1;eb!=NULL;)
		     { UCCHAR eName[V4RH_Name_Max+1] ; ETYPE opcode ;
		       v4lex_NextTkn(tcb,0) ;
		       if (tcb->opcode == V_OpCode_Rangle) break ;
		       if (tcb->type != V4LEX_TknType_Keyword) { v_Msg(NULL,vrhm->errMessage,"@Expecting tag name at line (%1d) of input file",tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(0) ; } ;
		       UCstrcpy(eName,tcb->UCkeyword) ;
		       for(;;)
			{ v4lex_NextTkn(tcb,0) ;
			  if (tcb->opcode == V_OpCode_Dot)
			   { v4lex_NextTkn(tcb,0) ;
			     if (UCstrlen(eName) + UCstrlen(tcb->UCkeyword) + 1 >= UCsizeof(eName)) { v_Msg(NULL,vrhm->errMessage,"@Tag name too long at line %1d of input file",tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(0) ; } ;
			     UCstrcat(eName,UClit(".")) ; UCstrcat(eName,tcb->UCkeyword) ; continue ;
			   } ;
			  break ;
			} ;
		       opcode = tcb->opcode ;
		       v4lex_NextTkn(tcb,V4LEX_Option_HTML) ;		/* Set HTML option so '\' not treated as escape character */
		       switch (tcb->type)
			{ case V4LEX_TknType_Integer:
			   if (tcb->decimal_places == 0) { UCsprintf(tcb->UCstring,25,UClit("%d"),tcb->integer) ; }
			    else { tcb->floating = tcb->integer / pow(10.0,tcb->decimal_places) ;
				   UCsprintf(tcb->UCstring,25,UClit("%g"),tcb->floating) ;
				 } ;
			   break ;
			  case V4LEX_TknType_Float:
			   UCsprintf(tcb->UCstring,25,UClit("%g"),tcb->floating) ; break ;
			} ;
		       switch(opcode)
			{ default:			v_Msg(NULL,vrhm->errMessage,"@Expecting '=' after tag name at line (%1d) of input file",tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(0) ;
			  case V_OpCode_Equal:		v4rh_neUpdate(vrhm,eName,V4RH_ETYPE_Tag,tcb->UCstring,V4RH_UPDOPER_Update) ; break ;
			  case V_OpCode_PlusEqual:	v4rh_neUpdate(vrhm,eName,V4RH_ETYPE_Tag,tcb->UCstring,V4RH_UPDOPER_Append) ; break ;
			} ;
		     } ;
		  } ;
/*		 Before we leave thinking this is well formed HTML, spin through & make sure no errors appear */
		 for(ok=TRUE;;)
		  { if (!v4lex_ReadNextLine(tcb,NULL,NULL,NULL)) break ;
		    if (UCstrncmpIC(tcb->ilvl[tcb->ilx].input_ptr,UClit("BSError"),7) != 0) continue ;
		    ok = v4rh_ReadError(vrhm) ; break ;
		  } ; if (!ok) return(0) ;
/*		 If eName is main.html.priorV4R then need to pull up a prior V4R and scan it */
		 if (UCstrcmpIC(eName,UClit("main.html.priorV4R")) == 0)
		  return(2) ;
/*		 Set the name of 'main' to main.html.xxxx (in eName) UNLESS we got an error */
	         if (!vrhm->gotError)
	          v4rh_neUpdate(vrhm,UClit("main"),V4RH_ETYPE_Tag,eName,V4RH_UPDOPER_Update) ;
	         vrhm->v4rIsHTML = TRUE ;
	         return(1) ;
	       } ;
	      v_Msg(NULL,vrhm->errMessage,"@Expecting begin-section (BS) at line (%1d) of input file",tcb->ilvl[tcb->ilx].UCFile->lineNum) ;
	      vrhm->gotError = TRUE ; return(0) ;
	    } ;
	   vrhm->gotEOF = FALSE ;		/* Not at EOF (anymore) */
/*	   Get section name and then handle that section */
	   for(i=2;vuc_IsAlphaNum(tcb->ilvl[tcb->ilx].input_ptr[i]);i++) { lHdr[i-2] = tcb->ilvl[tcb->ilx].input_ptr[i] ; } ; lHdr[i-2] = UCEOS ;
	   for(;vuc_IsWSpace(tcb->ilvl[tcb->ilx].input_ptr[i]);i++) {} ;
	   arg = &tcb->ilvl[tcb->ilx].input_ptr[i] ;		/* Arg points to anything that might be after the BS section name */
	   switch (v4im_GetUCStrToEnumVal(lHdr,0))
	    { default:			v_Msg(NULL,vrhm->errMessage,"@Undefined BS entry(%1U) at line %2d of input",lHdr,tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(0) ;
	      case _C:
	      case _Cell:
		crow = (ftr != NULL ? ftr : (row == NULL ? hdr : row)) ;
		if (crow == NULL)
		 { v_Msg(0,vrhm->errMessage,"@Encountered Cell section without prior Header/Row at line %1d of input",tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(0) ; } ;
		if (cell != NULL) cellPrior = cell ;
		cell = v4rpp_AllocChunk(sizeof *cell,TRUE) ;
		if (crow->cellFirst == NULL) crow->cellFirst = cell ;
		cell->lineNum = tcb->ilvl[tcb->ilx].UCFile->lineNum ;
		if (cellPrior != NULL) cellPrior->cellNext = cell ;
		crow->cellCur = cell ;		/* Set for duration of below routine */
		sheet->rowCur = crow ;
		v4rh_ParseCellValue(vrhm,cell,arg,DE(Cell)) ;
		if (!v4rh_ReadToEndSection(vrhm,NULL,&cell->cellInfo)) return(0) ;
		sheet->rowCur = NULL ;
		crow->cellCur = NULL ;
		break ;
	      case _ColumnInfo:		ok = v4rh_ReadColumnInfo(vrhm) ; break ;
	      case _Format:		ok = v4rh_ReadFormat(vrhm) ; break ;
	      case _Error:		ok = v4rh_ReadError(vrhm) ; break ;
	      case _EOF:
/*		Got EOF - really EOF or just end of current sheet ? Set flag and keep going, if not yet at EOF will reset gotEOF above */
		vrhm->gotEOF = TRUE ;
/*		Drop through here to read any Meta info that might have been included after the EOF */
	      case _Meta:		ok = v4rh_ReadMeta(vrhm) ; break ;
	      case _Other:		ok = v4rh_ReadOther(vrhm) ; break ;
	      case _Sheet:
#define NEWSHEET(DEFAULT) \
 if (sheet != NULL) sheetPrior = sheet ; \
 sheet = v4rpp_AllocChunk(sizeof *sheet,TRUE) ; \
 if (sheetPrior != NULL) sheetPrior->sheetNext = sheet ; \
 if (vrhm->sheetFirst == NULL) vrhm->sheetFirst = sheet ; \
 vrhm->sheetNum++ ; \
 vrhm->sheetCur = sheet ; \
 if (vrhm->sheetNum > 1) \
  v4rh_neUpdate(vrhm,UClit("main"),V4RH_ETYPE_Tag,(vrhm->forPrinting ? UClit("main.v4rMSPrint") : UClit("main.v4rMS")),V4RH_UPDOPER_Update) ; \
 if (DEFAULT) UCstrcpy(sheet->sheetName,UClit("Sheet1")) ;

		NEWSHEET(FALSE) ;
		rowPrior = NULL ; row = NULL ; hdrPrior = NULL ; hdr = NULL ; ftr = NULL ; ftrPrior = NULL ;
		if (!v4rh_ReadToEndSection(vrhm,sheet->sheetName,&sheet->sheetInfo)) return(0) ;
		break ;
	      case _Header:
		if (hdr != NULL) hdrPrior = hdr ;
		hdr = v4rpp_AllocChunk(sizeof *hdr,TRUE) ;
		if (sheet == NULL)	/* No explicit sheet - have to create one */
		 { NEWSHEET(TRUE) ;
		 } ;
		if (sheet->hdrFirst == NULL) sheet->hdrFirst = hdr ;
		hdr->lineNum = tcb->ilvl[tcb->ilx].UCFile->lineNum ;
		sheet->hdrCur = hdr ;
		 if (!v4rh_ReadToEndSection(vrhm,NULL,&hdr->rowInfo)) return(0) ;
		sheet->hdrCur = NULL ;
		if (hdrPrior != NULL) hdrPrior->rowNext = hdr ;
		cellPrior = NULL ; cell = NULL ;
		break ;
	      case _Footer:
		if (ftr != NULL) ftrPrior = ftr ;
		ftr = v4rpp_AllocChunk(sizeof *ftr,TRUE) ;
		if (sheet == NULL)	/* No explicit sheet - have to create one */
		 { v_Msg(0,vrhm->errMessage,"@Encountered Footer section without prior Header/Row at line %1d of input",tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(0) ; } ;
		if (sheet->ftrFirst == NULL) sheet->ftrFirst = ftr ;
		ftr->lineNum = tcb->ilvl[tcb->ilx].UCFile->lineNum ;
		sheet->ftrCur = ftr ;
		 if (!v4rh_ReadToEndSection(vrhm,NULL,&ftr->rowInfo)) return(0) ;
		sheet->ftrCur = NULL ;
		if (ftrPrior != NULL) ftrPrior->rowNext = ftr ;
		cellPrior = NULL ; cell = NULL ;
		break ;
	      case _Recap:
	      case _R:
	      case _Row:
		if (row != NULL) rowPrior = row ;
		row = v4rpp_AllocChunk(sizeof *row,TRUE) ;
		if (sheet == NULL)	/* No explicit sheet - have to create one */
		 { NEWSHEET(TRUE) ;
		 } ;
		if (sheet->rowFirst == NULL) sheet->rowFirst = row ;
		row->lineNum = tcb->ilvl[tcb->ilx].UCFile->lineNum ;
		sheet->rowCur = row ;
		  if (!v4rh_ReadToEndSection(vrhm,NULL,&row->rowInfo)) return(0) ;
		sheet->rowCur = NULL ;
		if (rowPrior != NULL) rowPrior->rowNext = row ;
		cellPrior = NULL ; cell = NULL ;
		sheet->hdrCur = NULL ;		/* No longer in header */
		break ;
	      case _XLInfo:		ok = v4rh_ReadXLInfo(vrhm) ; break ;
		break ;
	    } ;
	   if (!ok) return(0) ;
	 } ;
	return(1) ;
}

LOGICAL v4rh_ReadToEndSection(vrhm,name,ptrBuf)
  struct V4RH__Main *vrhm ;
  UCCHAR *name ;
  UCCHAR **ptrBuf ;
{
  struct V4LEX__TknCtrlBlk *tcb ;		/* Token control block for various inputs */
  enum DictionaryEntries ltype ;
  UCCHAR secBuf[V4LEX_Tkn_InpLineMax],multiLineBuf[V4LEX_Tkn_InpLineMax] ; LENMAX sbLen ;
  LENMAX iLen ; LOGICAL ok,multiLine ; UCCHAR *arg,*tc ;
	
	ZUS(secBuf) ; sbLen = 0 ; tcb = vrhm->tcb ;
	for(ok=TRUE;ok;)
	 { LOGICAL continued ;
	   multiLine = FALSE ;
	   for(continued=TRUE;continued;)
	    { if (!v4lex_ReadNextLine(tcb,NULL,NULL,NULL)) { ok = FALSE ; break ; } ;
	      iLen = UCstrlen(tcb->ilvl[tcb->ilx].input_ptr) ;
/*	      Do we have multi-line input ? */
	      continued = (tcb->ilvl[tcb->ilx].input_ptr[iLen-1] == UClit('\\')) ;
	      if (!continued && !multiLine) break ;
	      if (continued) tcb->ilvl[tcb->ilx].input_ptr[iLen-1] = UCEOS ;
	      if (!multiLine) { UCstrcpy(multiLineBuf,tcb->ilvl[tcb->ilx].input_ptr) ; multiLine = TRUE ; }
	       else { UCstrcat(multiLineBuf,UCEOLbts) ; UCstrcat(multiLineBuf,tcb->ilvl[tcb->ilx].input_ptr) ; } ;
	    } ;
/*	   If we got multiple lines, then start with beginning. If not and hit EOF (!ok) then quit */
	   if (multiLine)
	    { tcb->ilvl[tcb->ilx].input_ptr = multiLineBuf ; }
	    else { if (!ok) break ; } ;
	   if (tcb->ilvl[tcb->ilx].input_ptr[0] == UClit('!') || tcb->ilvl[tcb->ilx].input_ptr[0] == UClit('/') || UCempty(tcb->ilvl[tcb->ilx].input_ptr)) continue ;
	   lHdr[0] = tcb->ilvl[tcb->ilx].input_ptr[0] ; lHdr[1] = tcb->ilvl[tcb->ilx].input_ptr[1] ; lHdr[2] = UCEOS ;
	   arg = &tcb->ilvl[tcb->ilx].input_ptr[2] ;
	   switch (ltype = v4im_GetUCStrToEnumVal(lHdr,0))
	    { default:
append:		if (sbLen + iLen + 5 >= UCsizeof(secBuf))
		 { v_Msg(NULL,vrhm->errMessage,"@Exceeded max size of section at line %1d of input",tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(FALSE) ; } ;
		UCstrcpy(&secBuf[sbLen],tcb->ilvl[tcb->ilx].input_ptr) ; sbLen += iLen ;
		UCstrcpy(&secBuf[sbLen],UCEOLbts) ; sbLen += 1 ;
		break ;
	      case _DL:		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->initialLevel = v_ParseInt(arg,&tc,10) ; } ; break ;
	      case _ES:		ok = FALSE ; break ;
	      case _BS:		ok = FALSE ; vrhm->iLineLookAhead = TRUE ; break ;
	      case _UB:		v4rh_SetURLBase(vrhm,arg) ; break ;
	      case _NA:
/*		If we have a 'NAme' then handle immediately */
		if (name == NULL) break ;
		tcb->ilvl[tcb->ilx].input_ptr[2+V4RH_Name_Max] = UCEOS ;
		UCstrcpy(name,arg) ;
		goto append ;
	      case _KE:
		{ struct V4RH__Row *row ;
		  row = (vrhm->sheetCur == NULL ? NULL : (vrhm->sheetCur->rowCur != NULL ? vrhm->sheetCur->rowCur : NULL)) ;
		  if (row == NULL)
		   { v_Msg(NULL,vrhm->errMessage,"@No current row for key value at line %1d of input",tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(FALSE) ; } ;
		  row->rowKey = v4rpp_AllocUC(UCstrlen(arg)) ; UCstrcpy(row->rowKey,arg) ;
		  break ;
		}
/*	      This group gets assigned to the current cell */
	      case _EX:
	      case _CV:
		{ struct V4RH__Row *row ; struct V4RH__Cell *cell ;
		  row = (vrhm->sheetCur == NULL ? NULL : (vrhm->sheetCur->rowCur != NULL ? vrhm->sheetCur->rowCur : vrhm->sheetCur->hdrCur)) ;
		  cell = (row == NULL ? NULL : row->cellCur) ;
		  if (cell == NULL)
		   { v_Msg(NULL,vrhm->errMessage,"@No current cell for cell value at line %1d of input",tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(FALSE) ; } ;
		  v4rh_ParseCellValue(vrhm,cell,arg,ltype) ;
		  break ;
		}
	      case _MV:
/*		Parse multiple values, first value goes into 'this' cell, subsequent go into new cells appended to this one */
		{ struct V4RH__Row *row ; struct V4RH__Cell *cell,*ncell ;
		  UCCHAR *cc,*ec ; INDEX cnt ;
		  row = (vrhm->sheetCur == NULL ? NULL : (vrhm->sheetCur->rowCur != NULL ? vrhm->sheetCur->rowCur : vrhm->sheetCur->hdrCur)) ;
		  cell = (row == NULL ? NULL : row->cellCur) ;
		  if (cell == NULL)
		   { v_Msg(NULL,vrhm->errMessage,"@No current cell for cell value at line %1d of input",tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(FALSE) ; } ;
		  for (ncell=cell,cnt=1,cc=arg,ec=arg;ec!=NULL;cc=ec+1,cnt++)
		   { ec = UCstrchr(cc,'\t') ; if (ec != NULL) *ec = UCEOS ;
		     if (cnt == 1)
		      { ncell->cellValue = cc ;
		      } else
		      { struct V4RH__Cell *cellNew ;
		        cellNew = v4rpp_AllocChunk(sizeof *cellNew,TRUE) ;
		        ncell->cellNext = cellNew ;
		        v4rh_ParseCellValue(vrhm,cell,cc,ltype) ;
		        ncell = cellNew ;
		      } ;
		   } ;
		} break ;
/*	      This group gets assigned to current sheet */
	      case _BM:		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->marginBottom = v_ParseDbl(arg,&tc) ; } ; break ;
	      case _CN:
		if (vrhm->sheetCur == NULL) break ;
		vrhm->sheetCur->colIds = v4rpp_AllocUC(UCstrlen(arg)) ;	UCstrcpy(vrhm->sheetCur->colIds,arg) ;
		break ;
	      case _LM:		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->marginLeft = v_ParseDbl(arg,&tc) ; } ; break ;
	      case _PL:		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->orientPage = (UCTOUPPER(arg[0]) == UClit('L') ? -1 : UCTOUPPER(arg[0] == UClit('P') ? 1 : 0)) ; } ; break ;
	      case _RM:		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->marginRight = v_ParseDbl(arg,&tc) ; } ; break ;
	      case _SC:		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->scale = v_ParseDbl(arg,&tc) ; } ; break ;
	      case _SE:		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->selectInfo = v4rpp_AllocUC(UCstrlen(arg)) ; UCstrcpy(vrhm->sheetCur->selectInfo,arg) ; } ; break ;
	      case _SG:		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->gridLines = v_ParseInt(arg,&tc,10) ; } ; break ;
	      case _TI:
		if (vrhm->sheetCur == NULL ? TRUE : vrhm->sheetCur->titleNum >= V4RH_Max_Titles) break ;
		vrhm->sheetCur->ucTitles[vrhm->sheetCur->titleNum] = v4rpp_AllocUC(UCstrlen(arg)) ;
		UCstrcpy(vrhm->sheetCur->ucTitles[vrhm->sheetCur->titleNum],arg) ;
		vrhm->sheetCur->titleNum++ ;
		break ;
	      case _TM:		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->marginTop = v_ParseDbl(arg,&tc) ; } ; break ;
	      case _CC:		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->colCap = v_ParseDbl(arg,&tc) ; } ; break ;
/*	      This group is global - it applies to entire report */
	    } ;
	 } ;
	*ptrBuf = v4rpp_AllocUC(sbLen) ; UCstrcpy(*ptrBuf,secBuf) ;
	return(TRUE) ;
}


LOGICAL v4rh_ReadColumnInfo(vrhm)
  struct V4RH__Main *vrhm ;
{

	for(;;)
	 {
	   switch (v4im_GetUCStrToEnumVal(lHdr,0))
	    { default:		v_Msg(NULL,vrhm->errMessage,"@ReadColumnInfo() - Invalid entry (%1U) at line %2d of input",lHdr,-123) ; return(FALSE) ;
	      case _ES:		return(TRUE) ;
	      case _BC:
	      case _CG:
	      case _CW:
	      case _TC:
		break ;
	    } ;
	   
	 } ;
	return(TRUE) ;
}

struct V4RH__DimInfo *v4rh_DefDimEntry(vrhm,dimNum,vfi,cssX,maskX,pntType)
  struct V4RH__Main *vrhm ;
  INDEX dimNum ;
  struct V4RH__FormatInfo *vfi ;
  INDEX cssX, maskX ;
  PNTTYPE pntType ;
{ struct V4RH__DimInfo *vdi ;

	if (vrhm->vdi[dimNum] == NULL)
	 { vdi = (vrhm->vdi[dimNum] = v4rpp_AllocChunk(sizeof *vdi,TRUE)) ;
	   vdi->dimNum = dimNum ; vdi->cssX = UNUSED ; vdi->maskX = UNUSED ;
	 } ;
	vdi = vrhm->vdi[dimNum] ;
	if (vfi != NULL) vdi->vfi = vfi ;
	vdi->cssX = cssX ; vdi->maskX = maskX ;
	vdi->v4PntType = pntType ;
	return(vdi) ;
}

LOGICAL v4rh_ReadFormat(vrhm)
  struct V4RH__Main *vrhm ;
{
  struct V4LEX__TknCtrlBlk *tcb ;		/* Token control block for various inputs */
  struct V4RH__FormatInfo *vfi ;
  UCCHAR *arg,*b,lHdr[4] ; LENMAX iLen ; LOGICAL ok ; INDEX dimNum,maskX,cssX ; PNTTYPE pntType ;
  
	tcb = vrhm->tcb ; dimNum = UNUSED ; maskX = UNUSED ; pntType = V4DPI_PntType_UCChar ;
	vfi = v4rpp_AllocChunk(sizeof *vfi,TRUE) ;
	for(ok=TRUE;ok;)
	 {
	   if (!v4lex_ReadNextLine(tcb,NULL,NULL,NULL)) break ;
	   iLen = UCstrlen(tcb->ilvl[tcb->ilx].input_ptr) ;
	   if (tcb->ilvl[tcb->ilx].input_ptr[0] == UClit('!') || tcb->ilvl[tcb->ilx].input_ptr[0] == UClit('/') || UCempty(tcb->ilvl[tcb->ilx].input_ptr)) continue ;
	   lHdr[0] = tcb->ilvl[tcb->ilx].input_ptr[0] ; lHdr[1] = tcb->ilvl[tcb->ilx].input_ptr[1] ; ; lHdr[2] = UCEOS ;
	   arg = &tcb->ilvl[tcb->ilx].input_ptr[2] ;
	   switch (v4im_GetUCStrToEnumVal(lHdr,0))
	    { default:		v_Msg(NULL,vrhm->errMessage,"@ReadFormat() - Invalid entry (%1U) at line %2d of input",lHdr,tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(FALSE) ;
	      case _BK:
		vfi->bgColor = v_ColorNameToRef(arg) ;
		if (vfi->bgColor == 0) { v_Msg(NULL,vrhm->errMessage,"@ReadFormat() - Invalid color (%1U) at line %2d of input",arg,tcb->ilvl[tcb->ilx].UCFile->lineNum) ;  return(FALSE) ; } ;
		break ;
	      case _BO:
		switch (v4im_GetUCStrToEnumVal(arg,0))
		 { default:
		   case _Dashed:	vfi->border = V4SS_Border_Dashed ; break ;
		   case _Dotted:	vfi->border = V4SS_Border_Dotted ; break ;
		   case _Double:	vfi->border = V4SS_Border_Double ; break ;
		   case _Hairline:	vfi->border = V4SS_Border_Hairline ; break ;
		   case _Medium:	vfi->border = V4SS_Border_Medium ; break ;
		   case _Thick:		vfi->border = V4SS_Border_Thick ; break ;
		   case _Thin:		vfi->border = V4SS_Border_Thin ; break ;
		 } ;
		break ;
	      case _BS:			ok = FALSE ; vrhm->iLineLookAhead = TRUE ; break ;
	      case _CA:
		switch (v4im_GetUCStrToEnumVal(arg,0))
		 { default:
		   case _Bold:		vfi->fontAttr |= V4SS_Font_Bold ; break ;
		   case _Italic:	vfi->fontAttr |= V4SS_Font_Italic ; break ;
		   case _Outline:	vfi->fontAttr |= V4SS_Font_Outline ; break ;
		   case _Shadow:	vfi->fontAttr |= V4SS_Font_Shadow ; break ;
		   case _Subscript:	vfi->fontAttr |= V4SS_Font_SubScript ; break ;
		   case _Superscript:	vfi->fontAttr |= V4SS_Font_SuperScript ; break ;
		   case _StrikeThrough:	vfi->fontAttr |= V4SS_Font_StrikeThru ; break ;
		 } ;
		break ;
	      case _CL:		arg[V4RH_Name_Max] = UCEOS ; UCstrcpy(vfi->htmlClass,arg) ; break ;
	      case _CS:		vfi->cssStyle = v4rpp_AllocUC(UCstrlen(arg)) ; UCstrcpy(vfi->cssStyle,arg) ; break ;
	      case _DI:		dimNum = v_ParseInt(arg,&b,10) ; break ;
	      case _ES:		return(TRUE) ;
	      case _FI:		break ;
	      case _FM:		maskX = v4rh_GetMaskIndex(vrhm,arg) ; break ;
	      case _FO:		vfi->fontName = v4rpp_AllocUC(UCstrlen(arg)) ; UCstrcpy(vfi->fontName,arg) ; break ;
	      case _FS:		vfi->fontSize = v_ParseInt(arg,&b,10) ; break ;
	      case _HA:
		switch (v4im_GetUCStrToEnumVal(arg,0))
		 { default:
		   case _Center:	vfi->horAlign = V4SS_Align_Center ; break ;
		   case _Fill:		vfi->horAlign = V4SS_Align_Fill ; break ;
		   case _Indent:
		   case _Indent1:	vfi->horAlign = V4SS_Align_Indent1 ; break ;
		   case _Indent2:	vfi->horAlign = V4SS_Align_Indent2 ; break ;
		   case _Indent3:	vfi->horAlign = V4SS_Align_Indent3 ; break ;
		   case _Justify:	vfi->horAlign = V4SS_Align_Justify ; break ;
		   case _Left:		vfi->horAlign = V4SS_Align_Left ; break ;
		   case _Right:		vfi->horAlign = V4SS_Align_Right ; break ;
		 } ;
		break ;
	      case _HT:
		{ INDEX eX ;
		  eX = (vrhm->writeHTMLX % V4RH_EMBED_HTML_MAX) ;
		  if (vrhm->embedHTML[eX] == NULL) vrhm->embedHTML[eX] = v4rpp_AllocUC(1024) ;
		  if (UCstrlen(arg) > 1023) arg[1023] = UCEOS ;
		  UCstrcpy(vrhm->embedHTML[eX],arg) ; vrhm->writeHTMLX++ ;
		  break ;
		}
	      case _Id:		arg[V4RH_Name_Max] = UCEOS ; UCstrcpy(vfi->htmlId,arg) ; break ;
	      case _LA:
		{ struct V4CI__CountryInfo *ci ;
		  UCCHAR *tc ; int i, UNCode = v_ParseInt(arg,&tc,10) ;
		  ci = gpi->ci ;
		  for(i=0;i<ci->Count;i++) { if (ci->Cntry[i].UNCode == UNCode) break ; } ;
		   if (i < ci->Count)
		    { v_setCurrentCountry(ci,i) ; } ;
		} break ;
	      case _PT:
		pntType = v_ParseInt(arg,&b,10) ;
		switch (pntType)
		 { case V4DPI_PntType_Int:
		   case V4DPI_PntType_Delta:
		   case V4DPI_PntType_Real:
		   case V4DPI_PntType_Fixed:
		      vfi->horAlign = V4SS_Align_Right ; break ;
		 } ;
		break ;
	      case _SP:		vfi->colSpan = v_ParseInt(arg,&b,10) ; break ;
	      case _TC:
		vfi->textColor = v_ColorNameToRef(arg) ;
		if (vfi->textColor == 0) { v_Msg(NULL,vrhm->errMessage,"@ReadFormat() - Invalid color (%1U) at line %2d of input",arg,tcb->ilvl[tcb->ilx].UCFile->lineNum) ;  return(FALSE) ; } ;
		break ;
	      case _UB:		v4rh_SetURLBase(vrhm,arg) ; break ;
	      case _UL:
		switch (v_ParseInt(arg,&b,10))
		 { default:
		   case 1:	vfi->fontAttr |= V4SS_Font_Underline ; break ;
		   case 2:	vfi->fontAttr |= V4SS_Font_UnderlineDbl ; break ;
		 } ;
		break ;
	      case _VA:
		switch (v4im_GetUCStrToEnumVal(arg,0))
		 { default:
		   case _Bottom:	vfi->vertAlign = V4SS_Align_Bottom ; break ;
		   case _Center:	vfi->vertAlign = 0 ; break ;
		   case _Top:		vfi->vertAlign = V4SS_Align_Top ; break ;
		 } ;
		break ;
	      case _WA:
		switch (v4im_GetUCStrToEnumVal(arg,0))
		 { default:
		   case _NoBreak:	vfi->fontAttr |= V4SS_Font_NoBreak ; break ;
		   case _SizeToFit:	vfi->fontAttr |= V4SS_Font_SizeToFit ; break ;
		   case _Wrap:		vfi->fontAttr |= V4SS_Font_Wrap ; break ;
		 } ;
		break ;
	      case _WI:		vfi->colWidth = v_ParseInt(arg,&b,10) ; break ;
	    } ;
	   
	 } ;
/*	Make sure we have some default formats set */
	if (maskX == UNUSED)
	 { switch (pntType)
	    { default:				break ;
	      case V4DPI_PntType_UDate:		{ UCCHAR *df ;
						  switch (vrhm->outFormat)
						   { case html:
						     case json:	df = gpi->ci->Cntry[gpi->ci->CurX].DateMask ; break ;
						     case xls:
						     case xlsx:	
						       switch (gpi->ci->Cntry[gpi->ci->CurX].ymdOrder[0])
						        { default:			df = UClit("dd-mmm-yy") ; break ;
							  case V4LEX_YMDOrder_YMD:	df = UClit("yyyy-mmm-dd") ; break ;
							} ;
						     break ;
						   } ; 
						  maskX = v4rh_GetMaskIndex(vrhm,df) ;
						  break ;
						}
	      case V4DPI_PntType_UDT:
//	      case V4DPI_PntType_MIDASUDT:
	      case V4DPI_PntType_Calendar:	{ UCCHAR *df ;
						  switch (vrhm->outFormat)
						   { case html:
						     case json:	df = gpi->ci->Cntry[gpi->ci->CurX].DateTimeMask ; break ;
						     case xls:
						     case xlsx:	
						       switch (gpi->ci->Cntry[gpi->ci->CurX].ymdOrder[0])
						        { default:			df = UClit("dd-mmm-yy h:mm") ; break ;
							  case V4LEX_YMDOrder_YMD:	df = UClit("yyyy-mmm-dd h:mm") ; break ;
							} ;
						     break ;
						   } ; 
						  maskX = v4rh_GetMaskIndex(vrhm,df) ;
						  break ;
						}
	    }  ;
	 } ;
	if (dimNum < 0 || dimNum >= V4RH_VDI_MAX) { v_Msg(NULL,vrhm->errMessage,"@Dimension number (%1U) must be between 0 and %2d at line %3d of input",b,V4RH_VDI_MAX,tcb->ilvl[tcb->ilx].UCFile->lineNum) ;  return(FALSE) ; } ;
	switch (vrhm->outFormat)
	 { case html:
	   case json:	cssX = v4rh_GetCSSIndex(vrhm,v4rh_MakeCSSFromFormat(vrhm,vfi)) ; break ;
	   case tab:
	   case csv:
	   case xls:
	   case xlsx:	cssX = UNUSED ; break ;
	 } ;
	v4rh_DefDimEntry(vrhm,dimNum,vfi,cssX,maskX,pntType) ;
	return(TRUE) ;
}

/*	v4rh_MakeCSSFromFormat - Makes cannonical CSS entry from formatting structure */
UCCHAR *v4rh_MakeCSSFromFormat(vrhm,vfi)
  struct V4RH__Main *vrhm ;
  struct V4RH__FormatInfo *vfi ;
{
  static UCCHAR css[1024] ; UCCHAR tbuf[64] ;

	if (vfi->cssStyle != NULL) return(vrhm,vfi->cssStyle) ;
	ZUS(css) ;
	if (vfi->fontName != NULL) { UCstrcat(css,UClit("font-family:")) ; UCstrcat(css,vfi->fontName) ; UCstrcat(css,UClit(";")) ; } ;
	if (vfi->bgColor != 0)
	  { UCsprintf(tbuf,UCsizeof(tbuf),UClit("background-color:#%06x;"),v_ColorRefToRGB(vfi->bgColor)) ; UCstrcat(css,tbuf) ; } ;
	if (vfi->textColor != 0)  { UCsprintf(tbuf,UCsizeof(tbuf),UClit("color:#%06x;"),v_ColorRefToRGB(vfi->textColor)) ; UCstrcat(css,tbuf) ; } ;
#define FA(ATTR,CSS) if ((vfi->fontAttr & V4SS_Font_ ## ATTR) != 0) UCstrcat(css,UClit(CSS)) ;
	FA(Bold,"font-weight:bold;") FA(Italic,"font-style:italic;") FA(StrikeThru,"text-decoration: line-through;")
	FA(Underline,"text-decoration:underline;")
	if (vfi->colWidth != 0) { UCsprintf(tbuf,UCsizeof(tbuf),UClit("width:%d;"),vfi->colWidth) ; UCstrcat(css,tbuf) ; } ;
	if (vfi->fontSize > 0) { UCsprintf(tbuf,UCsizeof(tbuf),UClit("font-size:%dpt;"),vfi->fontSize) ; UCstrcat(css,tbuf) ; }
	 else if (vfi->fontSize < 0)
		{ UCsprintf(tbuf,UCsizeof(tbuf),(vfi->fontSize < -100 ? UClit("font-size:larger;") : UClit("font-size:smaller;")),vfi->fontSize) ; UCstrcat(css,tbuf) ; } ;
	switch (vfi->horAlign)
	 {
	   case V4SS_Align_Indent1:	UCstrcat(css,UClit("text-align:left;padding-left:2em")) ; break ;
	   case V4SS_Align_Indent2:	UCstrcat(css,UClit("text-align:left;padding-left:4em")) ; break ;
	   case V4SS_Align_Indent3:	UCstrcat(css,UClit("text-align:left;padding-left:6em")) ; break ;
	   case V4SS_Align_Left:	UCstrcat(css,UClit("text-align:left;")) ; break ;
	   case V4SS_Align_Right:	UCstrcat(css,UClit("text-align:right;")) ; break ;
	   case V4SS_Align_Center:	UCstrcat(css,UClit("text-align:center;")) ; break ;
	   case V4SS_Align_Justify:	UCstrcat(css,UClit("text-align:left;")) ; break ;
	   case V4SS_Align_Fill:	UCstrcat(css,UClit("text-align:left;")) ; break ;
	 } ;
/*	Have css string built - see if we already have one just like it */
	return(css) ;
}

INDEX v4rh_GetCSSIndex(vrhm,css)
  struct V4RH__Main *vrhm ;
  UCCHAR *css ;
{ INDEX i ;
  struct V4RH__CSSEntries *vce ;

	if (vrhm->vce == NULL) { vrhm->vce = v4rpp_AllocChunk(sizeof *vce,TRUE) ; } ;
	vce = vrhm->vce ;
	for(i=0;i<vce->numCSS;i++) { if (UCstrcmp(css,vce->css[i].cssDef) == 0) break ; } ;
	if (i < vce->numCSS) return(i) ;
	if (vce->numCSS >= V4RH_CSS_Max) { v_Msg(NULL,vrhm->errMessage,"@Exceeded max (%1d) CSS entries",V4RH_CSS_Max) ; return(UNUSED) ; } ;
	vce->css[vce->numCSS].cssDef = v4rpp_AllocUC(UCstrlen(css)) ; UCstrcpy(vce->css[vce->numCSS].cssDef,css) ;
	vce->css[vce->numCSS].className = NULL ;
	return(vce->numCSS++) ;
}


INDEX v4rh_GetMaskIndex(vrhm,mask)
  struct V4RH__Main *vrhm ;
  UCCHAR *mask ;
{ INDEX i ;

/*	If we are creating Excel and mask begins with '%' (C format) then try to convert to something Excel will take */
	if (mask[0] == UClit('%') && (vrhm->outFormat == xls || vrhm->outFormat == xlsx))
	 { if (UCstrcmp(mask,UClit("%#.2f")) == 0) { mask = UClit("#,###.00") ; }
	    else { printf("V4W- Could not convert C format mask (%s) to Excel, defaulting to '###'\n",UCretASC(mask)) ;
		   mask = UClit("###") ;
		 } ;
	 } ;
	if (vrhm->vmi == NULL)
	 { vrhm->vmi = v4rpp_AllocChunk(sizeof *vrhm->vmi,FALSE) ; vrhm->vmi->numMask = 0 ; } ;
	for(i=0;i<vrhm->vmi->numMask;i++)
	 { if (UCstrcmp(mask,vrhm->vmi->mask[i].formatMask) == 0) return(i) ;
	 } ;
/*	Add new entry */
	if (vrhm->vmi->numMask >= V4RH_MASK_MAX) return(UNUSED) ;
	vrhm->vmi->mask[vrhm->vmi->numMask].formatMask = v4rpp_AllocUC(UCstrlen(mask)) ;
	UCstrcpy(vrhm->vmi->mask[vrhm->vmi->numMask].formatMask,mask) ;
	return(vrhm->vmi->numMask++) ;
}


LOGICAL v4rh_ReadError(vrhm)
  struct V4RH__Main *vrhm ;
{ struct V4LEX__TknCtrlBlk *tcb ;		/* Token control block for various inputs */
  UCCHAR *arg ; LOGICAL ok ;

	tcb = vrhm->tcb ;
	vrhm->nextPass = TRUE ;		/* This will force end of first pass */
	vrhm->gotEOF = TRUE ;		/* OK if no explicit EOF - we are done anyway */
	vrhm->gotError = TRUE ;
/*	Set "main" section to process the "main.errors" section */
	v4rh_neUpdate(vrhm,UClit("main"),V4RH_ETYPE_Tag,UClit("main.errors"),V4RH_UPDOPER_Update) ;
	for(;;)
	 {
	   if (!v4lex_ReadNextLine(tcb,NULL,NULL,NULL)) break ;
	   if (tcb->ilvl[tcb->ilx].input_ptr[0] == UClit('!') || tcb->ilvl[tcb->ilx].input_ptr[0] == UClit('/') || UCempty(tcb->ilvl[tcb->ilx].input_ptr)) continue ;
	   lHdr[0] = tcb->ilvl[tcb->ilx].input_ptr[0] ; lHdr[1] = tcb->ilvl[tcb->ilx].input_ptr[1] ; ; lHdr[2] = UCEOS ; arg = &tcb->ilvl[tcb->ilx].input_ptr[2] ;
	   ok = TRUE ;
	   switch (v4im_GetUCStrToEnumVal(lHdr,0))
	    { default:		v_Msg(NULL,vrhm->errMessage,"@ReadError() - Invalid entry (%1U) at line %2d of input",lHdr,tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(FALSE) ;
	      case _BS:		return(TRUE) ;
	      case _ES:		return(TRUE) ;
	      case _MQ:
/*		Put quotes around the message */
		{ UCCHAR qmsg[512] ; v_StringLit(arg,qmsg,UCsizeof(qmsg),UClit('"'),0) ; arg = qmsg ;
		  DEFTAG("errMessage")
		} ;
	      case _MS:		DEFTAG("errMessage")
	      case _TY:		DEFTAG("errType")
	      case _UR:		DEFTAG("errURL")
		break ;
	    } ;
	   if (!ok) return(FALSE) ;
	 } ;
	return(TRUE) ;
}

LOGICAL v4rh_ReadMeta(vrhm)
  struct V4RH__Main *vrhm ;
{ struct V4LEX__TknCtrlBlk *tcb ;
  UCCHAR *arg, *tc ; LOGICAL ok ;

	tcb = vrhm->tcb ;
	for(;;)
	 {
	   if (!v4lex_ReadNextLine(tcb,NULL,NULL,NULL)) break ;
	   if (tcb->ilvl[tcb->ilx].input_ptr[0] == UClit('!') || tcb->ilvl[tcb->ilx].input_ptr[0] == UClit('/') || UCempty(tcb->ilvl[tcb->ilx].input_ptr)) continue ;
	   lHdr[0] = tcb->ilvl[tcb->ilx].input_ptr[0] ; lHdr[1] = tcb->ilvl[tcb->ilx].input_ptr[1] ; ; lHdr[2] = UCEOS ; arg = &tcb->ilvl[tcb->ilx].input_ptr[2] ;
	   ok = TRUE ;
	   switch (v4im_GetUCStrToEnumVal(lHdr,0))
	    { default:		v_Msg(NULL,vrhm->errMessage,"@ReadMeta() - Invalid entry (%1U) at line %2d of input",lHdr,tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(FALSE) ;
	      case _BS:		vrhm->iLineLookAhead = TRUE ; return(TRUE) ;
	      case _EP:		vrhm->epsilon = v_ParseDbl(arg,&tc) ; break ;
	      case _ES:		return(TRUE) ;
	      case _HL:		vrhm->xlsxURL = v_ParseInt(arg,&tc,10) ; break ;	/* Better be valid V4RH_XLSXURL_xxx value */
	      case _HT:
		{ INDEX eX ;
		  eX = (vrhm->writeHTMLX % V4RH_EMBED_HTML_MAX) ;
		  if (vrhm->embedHTML[eX] == NULL) vrhm->embedHTML[eX] = v4rpp_AllocUC(1024) ;
		  if (UCstrlen(arg) > 1023) arg[1023] = UCEOS ;
		  UCstrcpy(vrhm->embedHTML[eX],arg) ; vrhm->writeHTMLX++ ;
		  break ;
		}
	      case _Id:		DEFTAG("id")
	      case _IN:		if (vrhm->incNum >= V4RH_INCLUDE_MAX)
				 { v_Msg(NULL,vrhm->errMessage,"@ReadMeta() - Too many HTML includes at %2d of input",tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(FALSE) ; } ;
				vrhm->incFileName[vrhm->incNum] = v4mm_AllocUC(UCstrlen(arg)) ; UCstrcpy(vrhm->incFileName[vrhm->incNum],arg) ;
				vrhm->incNum++ ; break ;
	      case _LI:		if (vrhm->linkNum >= V4RH_LINK_MAX)
				 { v_Msg(NULL,vrhm->errMessage,"@ReadMeta() - Too many LINKs at %2d of input",tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(FALSE) ; } ;
				vrhm->linkURL[vrhm->linkNum] = v4mm_AllocUC(UCstrlen(arg)) ; UCstrcpy(vrhm->linkURL[vrhm->linkNum],arg) ;
				vrhm->linkNum++ ; break ;
	      case _IB:
	        { LENMAX cur ;
	          if (vrhm->sheetCur == NULL)
	           { v_Msg(NULL,vrhm->errMessage,"@ReadMeta() - Trying to set IB/IT without current SHEET at %2d of input",tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(FALSE) ; } ;
	          if (vrhm->sheetCur->bottomMax == 0) { vrhm->sheetCur->bottomMax = V4TMBufMax ; vrhm->sheetCur->bottomStr = v4mm_AllocUC(vrhm->sheetCur->bottomMax) ; ZUS(vrhm->sheetCur->bottomStr) ; } ;
	          cur = UCstrlen(vrhm->sheetCur->bottomStr) ;
	          if (cur + UCstrlen(arg) >= vrhm->sheetCur->bottomMax) { vrhm->sheetCur->bottomMax *= 1.5 ; vrhm->sheetCur->bottomStr = (UCCHAR *)realloc(vrhm->sheetCur->bottomStr,vrhm->sheetCur->bottomMax*sizeof(UCCHAR)+1) ; } ;
	          UCstrcat(vrhm->sheetCur->bottomStr,arg) ; UCstrcat(vrhm->sheetCur->bottomStr,UClit("\r")) ;
	        } break ;
	      case _IT:
	        { LENMAX cur ;
	          if (vrhm->sheetCur == NULL)
	           { v_Msg(NULL,vrhm->errMessage,"@ReadMeta() - Trying to set IB/IT without current SHEET at %2d of input",tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(FALSE) ; } ;
	          if (vrhm->sheetCur->topMax == 0) { vrhm->sheetCur->topMax = V4TMBufMax ; vrhm->sheetCur->topStr = v4mm_AllocUC(vrhm->sheetCur->topMax) ; ZUS(vrhm->sheetCur->topStr) ; } ;
	          cur = UCstrlen(vrhm->sheetCur->topStr) ;
	          if (cur + UCstrlen(arg) >= vrhm->sheetCur->topMax) { vrhm->sheetCur->topMax *= 1.5 ; vrhm->sheetCur->topStr = (UCCHAR *)realloc(vrhm->sheetCur->topStr,vrhm->sheetCur->topMax*sizeof(UCCHAR)+1) ; } ;
	          UCstrcat(vrhm->sheetCur->topStr,arg) ; UCstrcat(vrhm->sheetCur->topStr,UClit("\r")) ;
	        } break ;
	      case _DF:		DEFTAG("dfltFormat")
	      case _JN:		DEFTAG("jobId")
	      case _ME:		DEFTAG("memoText")
	      case _PE:		DEFTAG("privileges")
	      case _RN:		DEFTAG("reportName")
	      case _SK:		DEFTAG("sessionKey2")
	      case _SN:		{ UCCHAR secName[V4RH_Name_Max+1] ; UCstrcpy(secName,UClit("main.")) ; UCstrcat(secName,arg) ;
				  ok = (v4rh_neUpdate(vrhm,UClit("main"),V4RH_ETYPE_Tag,secName,V4RH_UPDOPER_Update) != NULL) ;
				} break ;
	      case _SP:		DEFTAG("sourceProgram")
	      case _SR:		DEFTAG("serverURL")
	      case _SU:		DEFTAG("setup")
	      case _TV:
		{ UCCHAR eName[V4RH_Name_Max+1] ; INDEX i ; ETYPE op ;
		  for(i=0;vuc_IsContId(arg[i])&&i<V4RH_Name_Max;i++) { eName[i] = arg[i] ; } ; eName[i] = UCEOS ;
		  for(;vuc_IsWSpace(arg[i]);i++) { } ;
		  op = (arg[i] == UClit('+') ? i++,V4RH_UPDOPER_Append : V4RH_UPDOPER_Update) ;
		  if (arg[i] != UClit('=')) { v_Msg(NULL,vrhm->errMessage,"@Improperly formed tag assignment at line (%1d) of input",tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(FALSE) ; } ;
		  for(i++;vuc_IsWSpace(arg[i]);i++) { } ;
		  if (v4rh_neUpdate(vrhm,eName,V4RH_ETYPE_Tag,&arg[i],op) == NULL) return(FALSE) ;
		}
		break ;
	      case _UN:		DEFTAG("userName")
	      case _VE:		DEFTAG("version")
	      case _CK:		
		{ struct V4RH__NamedEntry *ne = v4rh_neLookup(vrhm,UClit("sessionKey")) ;
		  if (ne != NULL)
		   { if (UCstrstr(arg,ne->eVal) == NULL)
		      { v_Msg(NULL,vrhm->errMessage,"@SecFail() - Session key mis-match in V4R") ; return(FALSE) ; } ;
		   } ;
		}
		DEFTAG("cookieValues")
/*	      These may appear after BSEOF (from the old Echos()) and so we have to also account for them here */
	      case _BM:		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->marginBottom = v_ParseDbl(arg,&tc) ; } ; break ;
	      case _LM:		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->marginLeft = v_ParseDbl(arg,&tc) ; } ; break ;
	      case _PL:		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->orientPage = (UCTOUPPER(arg[0]) == UClit('L') ? -1 : UCTOUPPER(arg[0] == UClit('P') ? 1 : 0)) ; } ; break ;
	      case _RM:		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->marginRight = v_ParseDbl(arg,&tc) ; } ; break ;
	      case _SC:		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->scale = v_ParseDbl(arg,&tc) ; } ; break ;
	      case _SG:		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->gridLines = v_ParseInt(arg,&tc,10) ; } ; break ;
	      case _TI:
		if (vrhm->sheetCur == NULL ? TRUE : vrhm->sheetCur->titleNum >= V4RH_Max_Titles) break ;
		vrhm->sheetCur->ucTitles[vrhm->sheetCur->titleNum] = v4rpp_AllocUC(UCstrlen(arg)) ;
		UCstrcpy(vrhm->sheetCur->ucTitles[vrhm->sheetCur->titleNum],arg) ;
		vrhm->sheetCur->titleNum++ ;
		break ;
	      case _TM:		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->marginTop = v_ParseDbl(arg,&tc) ; } ; break ;
	      case _CC:		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->colCap = v_ParseDbl(arg,&tc) ; } ; break ;
	    } ;
	   if (!ok) return(FALSE) ;
	 } ;
	return(TRUE) ;
}

LOGICAL v4rh_ReadOther(vrhm)
  struct V4RH__Main *vrhm ;
{ struct V4LEX__TknCtrlBlk *tcb ;
  UCCHAR *arg, *tc ; LOGICAL ok ;

	tcb = vrhm->tcb ;
	for(;;)
	 {
	   if (!v4lex_ReadNextLine(tcb,NULL,NULL,NULL)) break ;
	   if (tcb->ilvl[tcb->ilx].input_ptr[0] == UClit('!') || tcb->ilvl[tcb->ilx].input_ptr[0] == UClit('/') || UCempty(tcb->ilvl[tcb->ilx].input_ptr)) continue ;
	   lHdr[0] = tcb->ilvl[tcb->ilx].input_ptr[0] ; lHdr[1] = tcb->ilvl[tcb->ilx].input_ptr[1] ; ; lHdr[2] = UCEOS ; arg = &tcb->ilvl[tcb->ilx].input_ptr[2] ;
	   ok = TRUE ;
	   switch (v4im_GetUCStrToEnumVal(lHdr,0))
	    { default:		v_Msg(NULL,vrhm->errMessage,"@ReadOther() - Invalid entry (%1U) at line %2d of input",lHdr,tcb->ilvl[tcb->ilx].UCFile->lineNum) ; return(FALSE) ;
	      case _BS:		vrhm->iLineLookAhead = TRUE ; return(TRUE) ;
	      case _ES:		return(TRUE) ;
	      case _CS:
		if (vrhm->cssNum >= V4RH_CSS_MAX) break ;
		vrhm->cssLine[vrhm->cssNum] = v4rpp_AllocUC(UCstrlen(arg)+UCstrlen(UClit("\n"))) ;
		UCstrcpy(vrhm->cssLine[vrhm->cssNum],arg) ; UCstrcat(vrhm->cssLine[vrhm->cssNum],UClit("\n")) ;
		vrhm->cssNum++ ;
		break ;
	      case _SG:
		if (vrhm->sheetCur != NULL) { vrhm->sheetCur->gridLines = v_ParseInt(arg,&tc,10) ; } ; break ;
	      case _HT:
		{ INDEX eX ;
		  eX = (vrhm->writeHTMLX % V4RH_EMBED_HTML_MAX) ;
		  if (vrhm->embedHTML[eX] == NULL) vrhm->embedHTML[eX] = v4rpp_AllocUC(1024) ;
		  if (UCstrlen(arg) > 1023) arg[1023] = UCEOS ;
		  UCstrcpy(vrhm->embedHTML[eX],arg) ; vrhm->writeHTMLX++ ;
		  break ;
		}
	      case _UB:		v4rh_SetURLBase(vrhm,&tcb->ilvl[tcb->ilx].input_ptr[2]) ; break ;
	    } ;
	   if (!ok) return(FALSE) ;
	 } ;
	return(TRUE) ;
}

LOGICAL v4rh_ReadXLInfo(vrhm)
  struct V4RH__Main *vrhm ;
{

	for(;;)
	 {
	   switch (v4im_GetUCStrToEnumVal(lHdr,0))
	    { default:		v_Msg(NULL,vrhm->errMessage,"@ReadXLInfo() - Invalid entry (%1U) at line %2d of input",lHdr,-200) ; return(FALSE) ;
	      case _AF:
	      case _ES:		return(TRUE) ;
	      case _FC:
	      case _FR:
		break ;
	    } ;
	   
	 } ;
	return(TRUE) ;
}


void v4rh_SetURLBase(vrhm,baseText)
  struct V4RH__Main *vrhm ;
  UCCHAR *baseText ;		/* format: num=urlbasestring */
{ UCCHAR *tc ;
  int bn ;
  
	bn = v_ParseInt(baseText,&tc,10) ;
	if (*tc != UClit('=')) { bn = 0 ; }
	 else { baseText = tc + 1 ; } ;
	if (bn < 0 || bn >= V4RH_URLBASE_MAX) bn = 0 ;
	vrhm->urlBases[bn] = v4rpp_AllocUC(UCstrlen(baseText)+128) ;
//	UCstrcpy(vrhm->urlBases[bn],baseText) ;
/*	Save base as single-quote delimited string */
	v_StringLit(baseText,vrhm->urlBases[bn],UCstrlen(baseText)+128,UClit('\''),UClit('\0')) ;
}

//-j 20150227-000043 -k "PTphg1u0AXvi6RvD9G6HWCybv1AH6jBo" -i swsv4rpp.ini -m -s http://192.168.120.146:2355/ c:\vic\sws\work\v420150227-000043.v4r

LOGICAL v4rh_GenerateJS(vrhm)
  struct V4RH__Main *vrhm ;
{
  struct V4RH__NamedEntry *vrne ;
  UCCHAR js[0x10000] ; INDEX i ;

	ZUS(js) ;
	for(i=0;i<V4RH_URLBASE_MAX;i++)
	 { if (vrhm->urlBases[i] != NULL) break ; } ;
	if (i < V4RH_URLBASE_MAX) 
/*	   Got at least one base - have to set up js array & function */
	 { UCstrcat(js,UClit("v4r.urlBases = new Array(")) ;
	   for (i=0;i<V4RH_URLBASE_MAX;i++)
	    { if (vrhm->urlBases[i] == NULL) { UCstrcat(js,UClit("''")) ; }
	       else { UCstrcat(js,vrhm->urlBases[i]) ; } ;
	      if (i < V4RH_URLBASE_MAX - 1) { UCstrcat(js,UClit(",")) ; } ;
	    } ;
	   UCstrcat(js,UClit(");")) ; vout_UCTextFileX(vrhm->outFileX,0,js) ; vout_NLFileX(vrhm->outFileX) ;
//	   vout_UCTextFileX(vrhm->outFileX,0,UClit("function url(baseNum,urlSuffix,target) {")) ; vout_NLFileX(vrhm->outFileX) ;
//	   vout_UCTextFileX(vrhm->outFileX,0,UClit(" switch(target) { ")) ;
//	   vout_UCTextFileX(vrhm->outFileX,0,UClit("  case '':	location.href = urlBases[baseNum] + urlSuffix ; break ;")) ; vout_NLFileX(vrhm->outFileX) ;
//	   vout_UCTextFileX(vrhm->outFileX,0,UClit("  case '_blank':	window.open().location.href = urlBases[baseNum] + urlSuffix ; break ;")) ; vout_NLFileX(vrhm->outFileX) ;
//	   vout_UCTextFileX(vrhm->outFileX,0,UClit("  case '_self':	window.self.location.href = urlBases[baseNum] + urlSuffix ; break ;")) ; vout_NLFileX(vrhm->outFileX) ;
//	   vout_UCTextFileX(vrhm->outFileX,0,UClit("  case '_parent':	window.parent.location.href = urlBases[baseNum] + urlSuffix ; break ;")) ; vout_NLFileX(vrhm->outFileX) ;
//	   vout_UCTextFileX(vrhm->outFileX,0,UClit("  case '_top':	window.top.location.href = urlBases[baseNum] + urlSuffix ; break ;")) ; vout_NLFileX(vrhm->outFileX) ;
//	   vout_UCTextFileX(vrhm->outFileX,0,UClit("  default:	window.frames[target].location.href = urlBases[baseNum] + urlSuffix ; break ;")) ; vout_NLFileX(vrhm->outFileX) ;
//	   vout_UCTextFileX(vrhm->outFileX,0,UClit("  } ;")) ;
//	   vout_UCTextFileX(vrhm->outFileX,0,UClit("}")) ; vout_NLFileX(vrhm->outFileX) ;
	 } ;
/*	Do we have any cookie values to set ? */
	vrne = v4rh_neLookup(vrhm,UClit("cookieValues")) ;
	if (vrne != NULL)
	 { UCCHAR *b1,*b2 ;
	   for(b1 = vrne->eVal;*b1!=UCEOS;b1=b2)
	    { b2 = UCstrpbrk(b1,UClit(";") UCEOLbts) ;				/* Break on ';' or return */
	      if (b2 == NULL) { b2 = UClit("") ; } else { *b2 = UCEOS ; b2++ ; } ;
	      UCstrcpy(js,UClit("document.cookie = '")) ; UCstrcat(js,b1) ; UCstrcat(js,UClit("';")) ;
	      vout_UCTextFileX(vrhm->outFileX,0,js) ; vout_NLFileX(vrhm->outFileX) ;
	    } ;
	 } ;
/*	Any javascript from v4r source ? */
	for(i=0;i<vrhm->scriptNum;i++)
	 { vout_UCTextFileX(vrhm->outFileX,0,vrhm->script[i]) ; vout_NLFileX(vrhm->outFileX) ; } ;
	return(TRUE) ;
}

/*	O N   D E M A N D   H T M L   I N C L U D E S	*/

LOGICAL v4rh_GenerateHTMLInclude(vrhm)
  struct V4RH__Main *vrhm ;
{ struct UC__File iFile ;
  UCCHAR tbuf[UCReadLine_MaxLine] ; INDEX i ;
  
	for(i=0;i<vrhm->incNum;i++)
	 { if (!v_UCFileOpen(&iFile,vrhm->incFileName[i],UCFile_Open_Read,TRUE,vrhm->errMessage,0))
	    { v_Msg(NULL,tbuf,"@*Could not access include file (%1U) - %2U",vrhm->incFileName[i],vrhm->errMessage) ; vout_UCText(VOUT_Status,0,tbuf) ; continue ; } ;
	   for(;;)
	    { if (v_UCReadLine(&iFile,UCRead_UC,tbuf,UCsizeof(tbuf),tbuf) < 0) break ;
	      vout_UCTextFileX(vrhm->outFileX,0,tbuf) ; vout_NLFileX(vrhm->outFileX) ;
	    } ;
	   v_UCFileClose(&iFile) ;
	 } ;
	return(TRUE) ;
}

LOGICAL v4rh_GenerateLinkInclude(vrhm)
  struct V4RH__Main *vrhm ;
{ UCCHAR tbuf[2048] ; INDEX i ;
  
	for(i=0;i<vrhm->linkNum;i++)
	 { if (vrhm->linkURL[i][0] == UClit('<'))		/* If we already have HTML then don't wrap it */
	    { vout_UCTextFileX(vrhm->outFileX,0,vrhm->linkURL[i]) ; vout_NLFileX(vrhm->outFileX) ; continue ; } ;
	   if (UCstrstr(vrhm->linkURL[i],UClit(".css")))
	    { UCsprintf(tbuf,UCsizeof(tbuf),UClit("<link rel='stylesheet' href='%s' type='text/css' media='all'></link>\n"),vrhm->linkURL[i]) ; vout_UCTextFileX(vrhm->outFileX,0,tbuf) ; continue ; } ;
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("<script type='text/javascript' src='%s'></script>\n"),vrhm->linkURL[i]) ; vout_UCTextFileX(vrhm->outFileX,0,tbuf) ;
	 } ;
	return(TRUE) ;
}

LOGICAL v4rh_GenerateIncludeTop(vrhm)
  struct V4RH__Main *vrhm ;
{ UCCHAR *b, *b1 ;
  
	if (vrhm->sheetCur->topStr == NULL) return(FALSE) ;
	for(b=vrhm->sheetCur->topStr;;b=b1+1)
	 { b1 = UCstrchr(b,'\r') ; if (b1 != NULL) *b1 = UCEOS ;
	   vout_UCTextFileX(vrhm->outFileX,0,b) ; vout_NLFileX(vrhm->outFileX) ;
	   if (b1 == NULL) break ;
	 } ;
	return(TRUE) ;
}

LOGICAL v4rh_GenerateIncludeBottom(vrhm)
  struct V4RH__Main *vrhm ;
{ UCCHAR *b, *b1 ;
  
	if (vrhm->sheetCur->bottomStr == NULL) return(FALSE) ;
	for(b=vrhm->sheetCur->bottomStr;;b=b1+1)
	 { b1 = UCstrchr(b,'\r') ; if (b1 != NULL) *b1 = UCEOS ;
	   vout_UCTextFileX(vrhm->outFileX,0,b) ; vout_NLFileX(vrhm->outFileX) ;
	   if (b1 == NULL) break ;
	 } ;
	return(TRUE) ;
}

/*	O N  D E M A N D   T A G   V A L U E   G E N E R A T I O N	*/


LOGICAL v4rh_GenerateCSS(vrhm)
  struct V4RH__Main *vrhm ;
{
  struct V4RH__CSSEntries *vce ;
  struct V4RH__NamedEntry *vrne ;
#define V4RH_GENCSS_MAXCSS 100
  struct {				/* ncs - list of needed-css-sections */
    COUNTER numCSS ;
    struct {
      UCCHAR cssName[V4RH_Name_Max+1] ;
     } css[V4RH_GENCSS_MAXCSS] ;
   } ncs ;
  UCCHAR cssBuf[1024] ; INDEX i ;

	for(i=0;i<vrhm->cssNum;i++)
	 { vout_UCTextFileX(vrhm->outFileX,0,vrhm->cssLine[i]) ; } ;
	vce = vrhm->vce ; if (vce == NULL) return(TRUE) ;	/* Nothing (else) to generate ? */
	for(i=0;i<vce->numCSS;i++)
	 { v_Msg(NULL,cssBuf,"@.%1U {%2U}",vce->css[i].className,vce->css[i].cssDef) ;
	   vout_UCTextFileX(vrhm->outFileX,0,cssBuf) ; vout_NLFileX(vrhm->outFileX) ;
	 } ;
/*	If tag 'cssSections' is defined then make a list of css entries to include (otherwise bring 'em all in) */
	vrne = v4rh_neLookup(vrhm,UClit("cssSections")) ;
	ncs.numCSS = 0 ;
	if (vrne != NULL)
	 { UCCHAR *b ; INDEX i ;
	   for(b = vrne->eVal,i=0;*b!=UCEOS;b++)		/* Convert list of css section names to structured entries in ncs.xxx */
	    { if (vuc_IsContId(*b)) { if (i < V4RH_Name_Max) ncs.css[ncs.numCSS].cssName[i++] = *b ; continue ; } ;
	      if (i > 0 && ncs.numCSS < V4RH_GENCSS_MAXCSS) { ncs.css[ncs.numCSS].cssName[i++] = UCEOS ; ncs.numCSS++ ; i = 0 ; } ;
	      continue ;	/* Ignore everything else */
	    } ;
	   if (i > 0)		/* Finish off last entry */
	    { ncs.css[ncs.numCSS].cssName[i++] = UCEOS ; ncs.numCSS++ ; } ;
	 } ;
/*	Do we have a 'Section:css' (or Section:css.xxx) ? If so then include here also */
	for(i=0;i<V4RH_VRNE_MAX;i++)
	 { UCCHAR *sb, *eb ; vrne = vrhm->vrne[i] ;
	   if (vrne == NULL) continue ;
	   if (UCstrncmpIC(vrne->eName,UClit("SECTION:CSS"),11) != 0) continue ;
	   if (!(vrne->eName[11] == UCEOS || vrne->eName[11] == UClit('.'))) continue ;
/*	   Does this section match any of the entries in ncs ? */
	   if (ncs.numCSS > 0)
	    { INDEX i ;
	      for(i=0;i<ncs.numCSS;i++) { if (UCstrcmpIC(&vrne->eName[12],ncs.css[i].cssName) == 0) break ; } ;
	      if (i >= ncs.numCSS) continue ;
	    } ;
	   for(sb=vrne->eVal;;sb=eb+1)
	    { eb = UCstrchr(sb,EOLbt) ; if (eb != NULL) *eb = UCEOS ;
	      vout_UCTextFileX(vrhm->outFileX,0,sb) ; vout_NLFileX(vrhm->outFileX) ;
	      if (eb == NULL) break ; *eb = UCEOLbt ;
	    } ;
	 } ;
	return(TRUE) ;
}

int v4rh_GenerateColGroups(vrhm,sheet)
  struct V4RH__Main *vrhm ;
  struct V4RH__Sheet *sheet ;
{ struct V4RH__Column *col ;
  struct V4RH__Row *row ;
  struct V4RH__Cell *cell ;
  UCCHAR cg[0x1000] ; INDEX i,iMax,cx ; COUNTER curMax,total ;

	
	for(cx=1,col=sheet->colFirst;col!=NULL;cx++,col=col->colNext)
	 { ZUS(cg) ;
/*	   See if any dimNum is in the majority, if so then use it */
	   for(i=0,curMax=0,total=0;i<V4RH_DIM_MAX;i++)
	    { if (col->dimNumCount[i] > curMax) { iMax = i ; curMax = col->dimNumCount[i] ; } ;
	      total += col->dimNumCount[i] ;
	    } ;
	   if (curMax >= total / 2)		/* Does this account for at least 1/2 ? */
	    { v_Msg(NULL,cg,"@<colgroup id='cg_%1d' class='%2U' span='1'></colgroup>\n",cx,vrhm->vce->css[vrhm->vdi[iMax]->cssX].className) ;
	      col->dimNumMax = iMax ;
	    } else
	    { v_Msg(NULL,cg,"@<colgroup id='cg_%1d span='1'></colgroup>\n",cx) ; col->dimNumMax = UNUSED ;
	    } ;
	   vout_UCTextFileX(vrhm->outFileX,0,cg) ;
	 } ;
/*	Now go through all cells and wipe out dimNum if same as column-group */
	for (sheet=vrhm->sheetFirst;sheet!=NULL;sheet=sheet->sheetNext)
	 { 
	   for(row=sheet->rowFirst;row!=NULL;row=row->rowNext)
	    { if (sheet->colFirst == NULL) sheet->colFirst = v4rpp_AllocChunk(sizeof *col,TRUE) ;
	      for(cell=row->cellFirst,col=sheet->colFirst;cell!=NULL;cell=cell->cellNext,col=col->colNext)
	       { if (col->dimNumMax == cell->dimNum)
		  cell->useColGroup = TRUE ;
	       } ;
	    } ;
	 } ;
	return(0) ;
}




/*	Here to generate Excel binary file from parsed report specs */

void v4rx_GenerateXLS(vrhm,toWWW)
  struct V4RH__Main *vrhm ;
{
  struct V4RH__NamedEntry *vrne ;
  struct V4RH__Sheet *sheet ; struct V4RH__Row *row ; struct V4RH__Cell *cell ; struct V4RH__Column *col ;
  struct ss__WorkSheet *ws ;

	if (vrhm->gotError) { printf("?Errors detected, cannot create xls\n") ; return ; } ;
/*	Open up the output file */
	vrhm->wb = wbNew(UCretASC(vrhm->outFile)) ;
/*	Generating HTTP Headers ? */
/*	If going back to WWW client then add some necessary HTTP headers */
	if (toWWW)
	 { fprintf(vrhm->wb->ole->fp,"Content-Type: application/octet-stream\n") ;
	   fprintf(vrhm->wb->ole->fp,"Expires: -1\n") ;
	   fprintf(vrhm->wb->ole->fp,"Cache-Control: max-age=1\n") ;
	   fprintf(vrhm->wb->ole->fp,"Content-Disposition: attachment; filename=\"%s\"\n",(UCstrlen(vrhm->outFile) == 0 ? "result.xls" : UCretASC(vrhm->outFile))) ;
	 } ;
	{ 
#if defined WINNT && defined V4UNICODE
	  struct _stat statbuf ;
#else
	  struct stat statbuf ;
#endif

	  UCstat(vrhm->inpFile,&statbuf) ; vrhm->wb->wsSize = statbuf.st_size * 2 ;
	}
	if (vrhm->wb->wsSize < 1000 || vrhm->wb->wsSize > 5000000) vrhm->wb->wsSize = WS_DataSize ;
	ws = wbAddWorkSheet(vrhm->wb,"Sheet1",vrhm->wb->wsSize) ;

/*	Set up some default formats */
	vrhm->dateformat = wbAddFormat(vrhm->wb) ; strcpy(vrhm->dateformat->mask,"dd-mmm-yy") ;
	vrhm->dtFormat = wbAddFormat(vrhm->wb) ; strcpy(vrhm->dtFormat->mask,"dd-mmm-yy hh:mm") ;
	if (vrhm->sheetFirst != NULL ? vrhm->sheetFirst->gridLines != 0 : FALSE)
	 { ws->GridLines = vrhm->sheetFirst->gridLines ;
	   vrhm->dateformat->top = (vrhm->dateformat->bottom = (vrhm->dateformat->left = (vrhm->dateformat->right = 0x7))) ;
	   vrhm->defaultformat = wbAddFormat(vrhm->wb) ;
	   vrhm->defaultformat->top = (vrhm->defaultformat->bottom = (vrhm->defaultformat->left = (vrhm->defaultformat->right = 0x7))) ;
	   vrhm->dtFormat->top = (vrhm->dtFormat->bottom = (vrhm->dtFormat->left = (vrhm->dtFormat->right = 0x7))) ;
	 } ;

/*	Iterate through entire report */
	for (sheet=vrhm->sheetFirst;sheet!=NULL;sheet=sheet->sheetNext)
	 { INDEX rowNum, colNum, tx ;


/*	   Create new sheet in workbook */
	   if (sheet == vrhm->sheetFirst)				/* Very begin? */
	    { strcpy(ws->name,UCretASC(sheet->sheetName)) ; }		/*  then update current ws name */
	    else { ws = wbAddWorkSheet(vrhm->wb,UCretASC(sheet->sheetName),vrhm->wb->wsSize) ;	/*  otherwise create a new sheet */
	         } ;
	   ws->LMargin = 0.0 ; ws->RMargin = 0.0 ;
	   strcpy(ws->header,"&C&12&\"Arial,Bold Italic\"") ;
	   for(tx=0;tx<V4RH_Max_Titles;tx++)
	    { if (sheet->ucTitles[tx] == NULL) break ;
	      if (tx > 0) strcat(ws->header,"\012") ;
	      v4rx_FixUpString(sheet->ucTitles[tx]) ;
	      strcat(ws->header,UCretASC(sheet->ucTitles[tx])) ;
	    } ;
	   vrne =  v4rh_neLookup(vrhm,UClit("jobId")) ;
	   sprintf(ws->footer,"&L%s&CPage &P&R",(vrne == NULL ? "&D" : UCretASC(vrne->eVal))) ;
/*	   First do the headers */
	   for(rowNum=0,row=sheet->hdrFirst;row!=NULL;row=row->rowNext,rowNum++)
	    { 
	      for(colNum=0,cell=row->cellFirst,col=sheet->colFirst;cell!=NULL;cell=cell->cellNext,col=col->colNext)
	       { 
		 colNum += v4rx_CreateCell(vrhm,rowNum,colNum,row,cell,col,ws) ;
	       } ;
	    } ;
	   if (rowNum > 0)		/* Set up repeating column headers */
	    { char tbuf[32] ; sprintf(tbuf,"%s!$1:$%d",ws->name,rowNum) ;
//veh090917 - For some reason, Excel 2007 will only allow this once otherwise it 'stops working'
	      if (ws->index == 1)
	       wsStoreDefinedName(ws,NULL,7,ws->index,tbuf,rowNum) ;
	    } ;

/*	   Now the data rows */
	   for(row=sheet->rowFirst;row!=NULL;row=row->rowNext,rowNum++)
	    { vrhm->pageBreak = FALSE ;				/* Set global break indicator to false, if set true by end of row then set in Excel */
	      v4rh_SetCurRow(vrhm,row,V4RH_RowType_Data,NULL) ;
	      if (row->skipRow) continue ;
	      for(colNum=0,cell=row->cellFirst,col=sheet->colFirst;cell!=NULL;cell=cell->cellNext,col=col->colNext)
	       { 
		 colNum += v4rx_CreateCell(vrhm,rowNum,colNum,row,cell,col,ws) ;
	       } ;
	      if (vrhm->pageBreak)
	       { if (ws->PBCount < WS_PBMax) ws->PBRows[ws->PBCount++] = rowNum ; } ;
	    } ;
/*	   And any footers */
	   for(row=sheet->ftrFirst;row!=NULL;row=row->rowNext,rowNum++)
	    { vrhm->pageBreak = FALSE ;				/* Set global break indicator to false, if set true by end of row then set in Excel */
	      v4rh_SetCurRow(vrhm,row,V4RH_RowType_Data,NULL) ;
	      if (row->skipRow) continue ;
	      for(colNum=0,cell=row->cellFirst,col=sheet->colFirst;cell!=NULL;cell=cell->cellNext,col=col->colNext)
	       { 
		 colNum += v4rx_CreateCell(vrhm,rowNum,colNum,row,cell,col,ws) ;
	       } ;
	      if (vrhm->pageBreak)
	       { if (ws->PBCount < WS_PBMax) ws->PBRows[ws->PBCount++] = rowNum ; } ;
	    } ;
	   for(colNum=0,col=sheet->colFirst;col!=NULL;col=col->colNext,colNum++)
	    { if (col->width == 0) continue ;
	      if (ws->colCount >= WS_COLMAX-1) continue ;	/* Don't exceed column max */
	      ws->ColInfo[ws->colCount].firstcol = (ws->ColInfo[ws->colCount].lastcol = colNum) ;
	      ws->ColInfo[ws->colCount].width = col->width ;
	      ws->colCount++ ;
	    } ;
/*	   Is page orientation defined? If not then make landscape if >= 10 columns */
	   if (sheet->orientPage == 0) { sheet->orientPage = (ws->colCount >= 10 ? -1 : 1) ; } ;
	   ws->Portrait = (sheet->orientPage > 0) ;
	   ws->FitToWidth = 1 ;			/* Try to fit one page across */
	 } ;
	wbClose(vrhm->wb) ;
}

struct ss__Format *v4rx_CreateXLFormat(vrhm,vfi,mask,multiLine)
  struct V4RH__Main *vrhm ;
  struct V4RH__FormatInfo *vfi ;
  UCCHAR *mask ;
  LOGICAL multiLine ;
{
  struct ss__Format tformat,*format ;
  INDEX i ;
 
	memset(&tformat,0,sizeof tformat) ; formatNew(0,&tformat) ;
	if (mask != NULL) { UCstrcpyToASC(tformat.mask,mask) ; } ;
	if (vfi->fontAttr & V4SS_Font_Bold) tformat.bold = 0x2bc ;
	if (vfi->fontAttr & V4SS_Font_Italic) tformat.italic = 1 ;
	if (vfi->fontAttr & V4SS_Font_StrikeThru) tformat.font_strikeout = 1 ;
	if (vfi->fontAttr & V4SS_Font_Wrap)
	  tformat.text_wrap = TRUE ;
	if (vfi->fontAttr & V4SS_Font_SizeToFit) tformat.size_to_fit = TRUE ;
	switch (vfi->horAlign)
	 { case V4SS_Align_Left:	tformat.text_h_align = (vfi->colSpan > 0 ? 20 : 1) ; break ;
	   case V4SS_Align_Right:	tformat.text_h_align = (vfi->colSpan > 0 ? 21 : 3) ; break ;
	   case V4SS_Align_Center:	tformat.text_h_align = (vfi->colSpan > 0 ? 6 : 2) ; break ;
	   case V4SS_Align_Justify:	tformat.text_h_align = 5 ; break ;
	   case V4SS_Align_Fill:	tformat.text_h_align = 4 ; break ;
	   case V4SS_Align_MultiColCenter:	tformat.text_h_align = 6 ; break ;
	   case V4SS_Align_MultiColLeft:	tformat.text_h_align = 20 ; break ;
	   case V4SS_Align_MultiColRight:	tformat.text_h_align = 21 ; break ;
	 } ;
	switch (vfi->vertAlign)
	 { case V4SS_Align_Center:	tformat.text_v_align = 1 ; break ;
	   case V4SS_Align_Top:		tformat.text_v_align = 0 ; break ;
	   case V4SS_Align_Bottom:	tformat.text_v_align = 2 ; break ;
	 } ;
	tformat.color = v_ColorRefToXL(vfi->textColor,FALSE) ;
	if (vfi->bgColor != 0)
	 { tformat.bg_color = (tformat.fg_color = v_ColorRefToXL(vfi->bgColor,FALSE)) ; tformat.pattern = 1 ; } ;
	if (vrhm->sheetCur->gridLines != 0)
	 { tformat.top = (tformat.bottom = (tformat.left = (tformat.right = 0x7))) ;
	 } ;
	if (multiLine)
	 tformat.text_wrap = TRUE ;	/* If text is multiline then have to set text wrapping */
/*	Fontsize < 0 means % increase/decrease - for now just tweak by 2pts */
	if (vfi->fontSize > 0) { tformat.size = vfi->fontSize ; }
	 else if (vfi->fontSize < 0)
		{ tformat.size += (vfi->fontSize < -100 ? 2 : -2) ; } ;
	for(i=0;i<vrhm->wb->formatCount;i++)
	 { tformat.xf_index = vrhm->wb->format[i]->xf_index ;
	   if (memcmp(vrhm->wb->format[i],&tformat,sizeof tformat) == 0)
	    { format = vrhm->wb->format[i] ; break ; } ;
	 } ;
/*	Have to make a new format */
	if (i >= vrhm->wb->formatCount) 
	 { int xfi ;
	   format = wbAddFormat(vrhm->wb) ; xfi = format->xf_index ;
	   memcpy(format,&tformat,sizeof tformat) ; format->xf_index = xfi ;
	 } ;
	return(format) ;
}

COUNTER v4rx_CreateCell(vrhm,rowNum,colNum,row,cell,col,ws)
  struct V4RH__Main *vrhm ;
  INDEX rowNum,colNum ;
  struct V4RH__Row *row ;
  struct V4RH__Cell *cell ;
  struct V4RH__Column *col ;
  struct ss__WorkSheet *ws ;
{
  struct V4RH__DimInfo *vdi ;
  struct V4RH__FormatInfo *vfi,lvfi ;
  struct ss__Format *format ;
  static INDEX idnCount = 0 ;
  static double zuluoffset = UNUSED ;
  UCCHAR *tc,*cp,*ep,lHdr[3],*arg ; CALENDAR cal ; int inum,i ; double dbl,maxdnum ; LOGICAL logical ; LENMAX colSpan,valLen ;

	vdi = (cell->dimNum >= 0 && cell->dimNum < V4RH_DIM_MAX ? vrhm->vdi[cell->dimNum] : NULL) ;
/*	If vdi undefined then default with generic alpha */
	if (vdi == NULL)
	 { if (++idnCount < 10) printf("[Invalid dimension number (%d) encountered in cell, defaulting to Alpha]\n",cell->dimNum) ;
	   vdi = vrhm->vdi[cell->dimNum=V4RH_DimNum_Alpha] ;
	 } ;
	vfi = vdi->vfi ;
/*	Parse (abbreviated) cell attributes */
	for(cp=cell->cellInfo,ep=cp;ep!=NULL;cp=ep+1)
	 { ep = UCstrchr(cp,EOLbt) ;
	   if (UCempty(cp)) continue ;
	   if (ep != NULL) *ep = UCEOS ;			/* Set terminating char to EOS - HAVE TO RESET!!! */
	   lHdr[0] = cp[0] ; lHdr[1] = cp[1] ; ; lHdr[2] = UCEOS ; arg = &cp[2] ;
	   switch (v4im_GetUCStrToEnumVal(lHdr,0))
	    { default:		continue ;
	      case _PB:		vrhm->pageBreak = TRUE ; break ;
	      case _SP:
		cell->colSpan = v_ParseInt(arg,&tc,10) ;
		lvfi = *vfi ; vfi = &lvfi ;	/* Make copy of format */
		vfi->colSpan = cell->colSpan ;
		if (vfi->horAlign == 0) vfi->horAlign = V4SS_Align_Center ;
		break ;
	    } ;
	 } ;

	format = v4rx_CreateXLFormat(vrhm,vfi,(vdi->maskX != UNUSED ? vrhm->vmi->mask[vdi->maskX].formatMask : NULL),(cell->cellValue == NULL ? FALSE : UCstrchr(cell->cellValue,'\r') != NULL)) ;
	if (cell->cellFormula != NULL)		/* If we have a formula then inject it */
	 { wsWriteFormula(ws,rowNum,colNum,UCretASC(cell->cellFormula),0.0,1,(format == NULL ? vrhm->defaultformat : format)) ;
	   if (col->width < 8) col->width = 8 ;
	   return(1) ;
	 } ;
	if (cell->cellValue == NULL)
	 { wsWriteString(ws,rowNum,colNum,"",(format == NULL ? vrhm->defaultformat : format)) ; return(1) ; } ;		/* If no value then default to empty cell */
	colSpan = (cell->colSpan > 0 ? cell->colSpan : (vdi->vfi != NULL ? vdi->vfi->colSpan : 0)) ;
	valLen = UCstrlen(cell->cellValue) ;
	switch (cell->pntType == UNUSED ? vdi->v4PntType : cell->pntType)
	 { default:
		v_Msg(NULL,vrhm->errMessage,"@Invalid point type (%1d) cell (%2U)",vdi->v4PntType,cell->cellValue) ; return(FALSE) ;
//	   case V4DPI_PntType_MIDASUDT:   
//		inum = v_ParseInt(cell->cellValue,&tc,10) ;
//		cal = UDTtoCAL(inum) ;
//		goto cal_entry ;
	   case V4DPI_PntType_UDate:
	   case V4DPI_PntType_UDT:
	   case V4DPI_PntType_Calendar:
		cal = v_ParseDbl(cell->cellValue,&tc) ;
//cal_entry:
		{ double frac,fixdate ; struct ss__Format *ssf ;
		  if (zuluoffset == (double)UNUSED) zuluoffset = -(double)mscu_minutes_west() / (24 * 60) ;
		  if (cal == VCal_NullDate) cal = -1 ;
/*		  If we have time portion then convert from Zulu, otherwise take as is */
		  frac = modf(cal,&fixdate) ;
		  if (frac != 0.0)
		   { cal += zuluoffset ; frac = modf(cal,&fixdate) ;  if (col->width < 15) col->width = 15 ; } ;
		  ssf = (cell->pntType == V4DPI_PntType_UDate || frac == 0.0 ? vrhm->dateformat : vrhm->dtFormat) ;
		  cal = cal - VCal_MJDOffset - 50084 + 35065 ;
		  if (cal <= 0) { wsWriteString(ws,rowNum,colNum,"",(format == NULL ? ssf : format)) ; }
		   else { wsWriteNumber(ws,rowNum,colNum,cal,(format == NULL ? ssf : format)) ; } ;
		}
		if (col->width < 10) col->width = 10 ;
		break ;
	   case V4DPI_PntType_UYear:
	   case V4DPI_PntType_Int:
	   case V4DPI_PntType_Delta:
		inum = v_ParseInt(cell->cellValue,&tc,10) ;
		wsWriteNumber(ws,rowNum,colNum,(double)inum,(format == NULL ? vrhm->defaultformat : format)) ;
		if (UCstrlen(cell->cellValue) > col->width) col->width = UCstrlen(cell->cellValue) ;
		if (col->width < valLen) col->width = valLen ;
		break ;
	   case V4DPI_PntType_Real:
		dbl = v_ParseDbl(cell->cellValue,&tc) ;
		if (fabs(dbl) < vrhm->epsilon) dbl = 0 ;
		wsWriteNumber(ws,rowNum,colNum,dbl,(format == NULL ? vrhm->defaultformat : format)) ;
/*		Determining the width is not easy - have to format each number and guess (can't rely on input length- '23.0000000000001' may format as '23.00') */
		{ UCCHAR mask[64],dst[128] ;
		  UCstrcpyAtoU(mask,(format == NULL ? vrhm->defaultformat->mask : format->mask)) ;
		  if (UCempty(mask))
		   { double intPart ; if (modf(dbl,&intPart) == 0.0) { UCstrcpy(mask,UClit("%.0f")) ; } else { UCstrcpy(mask,UClit("%g")) ; } ;
		   } ;
		  v_FormatDbl(dbl,mask,dst,NULL) ; valLen = UCstrlen(dst) ;
		}
		if (col->width < valLen) col->width = valLen ;
		break ;
	   case V4DPI_PntType_Logical:
		logical = v_ParseLog(cell->cellValue,&tc) ;
		wsWriteString(ws,rowNum,colNum,(logical ? "Yes" : "No"),(format == NULL ? vrhm->defaultformat : format)) ;
		if (col->width < 3) col->width = 3 ;
		break ;
	   case V4DPI_PntType_BinObj:
	   case V4DPI_PntType_UOM:
	   case V4DPI_PntType_UMonth:
	   case V4DPI_PntType_UQuarter:
	   case V4DPI_PntType_UPeriod:
	   case V4DPI_PntType_UWeek:
	   case V4DPI_PntType_UOMPer:
	   case V4DPI_PntType_UTime:
	   case V4DPI_PntType_List:
	   case V4DPI_PntType_Dict:
	   case V4DPI_PntType_XDict:
	   case V4DPI_PntType_Char:
	   case V4DPI_PntType_BigText:
	   case V4DPI_PntType_UCChar:
	   case V4DPI_PntType_TeleNum:
	   case V4DPI_PntType_Shell:
	   case V4DPI_PntType_Isct:
	   case V4DPI_PntType_Country:
	   case V4DPI_PntType_GeoCoord:
		if (UCstrlen(cell->cellValue) >= 255) cell->cellValue[255] = UCEOS ;			/* Make sure buffer does not exceed 255 */
/*		Have to check format for multi-column centered string - if so parse number of columns */
		if (colSpan != 0)
		 { INDEX i ;
		   v4rx_FixUpString(cell->cellValue) ;
		   wsWriteString(ws,rowNum,colNum,UCretASC(cell->cellValue),(format == NULL ? vrhm->defaultformat : format)) ;
		   colNum++ ;
		   for(i=1;i<colSpan;i++)		/* Fill out following columns with blank cells */
		    { wsWriteString(ws,rowNum,colNum++,"",(format == NULL ? vrhm->defaultformat : format)) ; } ;
		   break ;
		 } ;
/*		If any embedded HTML stuff or &xxx; then strip out */
		v4rx_FixUpString(cell->cellValue) ;
		wsWriteString(ws,rowNum,colNum,UCretASC(cell->cellValue),(format == NULL ? vrhm->defaultformat : format)) ;
		maxdnum = 0.0 ;
		for(i=0,dbl=0.0;cell->cellValue[i]!='\0';i++)
		 { if (cell->cellValue[i] >= 'a' && cell->cellValue[i] <= 'z')
		    { switch (cell->cellValue[i])
		       { default:
				dbl += 1.0 ; break ;
		         case 'i': case 'l': case 'j': case 't':
				dbl += 0.6 ; break ;
		       } ; continue ;
		    } ;
		   if (cell->cellValue[i] >= 'A' && cell->cellValue[i] <= 'Z')
		    { switch (cell->cellValue[i])
		       { default:
				dbl += 1.3 ; break ;
		         case 'I':
				dbl += 0.9 ; break ;
		       } ; continue ;
		    } ;
		   switch (cell->cellValue[i])
		    { default:
			dbl += 1 ; break ;
		      case 10:				/* New line - start count all over again */
			if (dbl > maxdnum) maxdnum = dbl ; dbl = 0.0 ; break ;
		      case '.': case ',': case ';':
			dbl += 0.8 ; break ;
		    } ;
		 } ; if (maxdnum > dbl) dbl = maxdnum ;
		inum = (int)(dbl + 0.50) ;
		if (format != NULL) { if (format->bold >= 0x2bc) inum += 2 ; } ;
/*		If spans multiple columns, divide inum by number of columns (so first column not unecessarily huge) */
		if (cell->colSpan > 0) inum /= cell->colSpan ;
		if (inum <= 0) inum = 1 ;
		if (col->width < inum) col->width = inum ;
		break ;
	      case V4DPI_PntType_SSVal:
		inum = v_ParseInt(cell->cellValue,&tc,10) ;
		switch(inum)
		 {
		   case V4SS_SSVal_Null:	inum = 0 ; break ;
		   case V4SS_SSVal_Div0:	inum = 0x07 ; break ;
		   case V4SS_SSVal_Value:	inum = 0x0f ; break ;
		   case V4SS_SSVal_Ref:		inum = 0x17 ; break ;
		   case V4SS_SSVal_Name:	inum = 0x1d ; break ;
		   case V4SS_SSVal_Num:		inum = 0x24 ; break ;
		   case V4SS_SSVal_NA:		inum = 0x2a ; break ;
		   case V4SS_SSVal_Empty:	inum = UNUSED ; break ;
		 } ;
		if (inum != UNUSED) wsWriteError(ws,rowNum,colNum,inum,(format == NULL ? vrhm->defaultformat : format)) ;
		break ;
	 } ;
	return(colSpan == 0 ? 1 : colSpan) ;
}

void v4rx_FixUpString(buf)
  UCCHAR *buf ;
{
  INDEX s, d ; UCCHAR term ;

/*	Get rid of any '\$', '<html>', and '&entity;' stuff */
	if (UCempty(buf)) return ;
	term = 0 ;
	for(s=0,d=0;buf[s]!=UCEOS;s++)
	 { if (buf[s] == UClit('\033')) continue ;		/* Ignore escapes */
	   if (buf[s] == EOLbt)
	    { buf[d++] = 10 ; continue ; } ;	/* Replace new-line with Excel new-line */
	   if (buf[s] == UClit('<') && vuc_IsContId(buf[s+1]))
	    { term = UClit('>') ; continue ; } ;/* Skip over any embedded HTML tags */
	   if (buf[s] == UClit('&'))		/* This may just be a lone '&' - check next few characters */
	    { INDEX i ;
	      for(i=1;i<10;i++)
	       { if (buf[s+i] == UClit(';'))
		  { UCCHAR ub = v4xml_LookupISONamedEntity(&buf[s]) ;
		    if (ub > 0 && ub < 0x7f)	/* If this is an ASCII character take it, otherwise skip the whole thing */
		     { buf[d++] = ub ; } ;
		    s += (i+1) ; continue ;	/* Got HTML entity ('&xxx;') - skip it */
		  } ;
	         if (!(vuc_IsContId(buf[s+i]) || (buf[s+i] == UClit('#')))) break ;
	       } ;
	    } ;
	   if (buf[s] == term) { term = 0 ; continue ; } ;
	   if (term != 0) continue ;
	   buf[d++] = buf[s] ;
	 } ;
	buf[d] = UCEOS ;
	return ;

#ifdef MOOO
/*	If any embedded HTML stuff then strip out */
/*	p1 = begin of string, p = ptr to '<', p2 = ptr to '>', tbuf holds "fixed" string up to p1 */
	for(tbuf[0]=UCEOS,p1=buf,fix=FALSE;(p=UCstrchr(p1,'<')) != NULL;)
	 { p2 = UCstrchr(p,'>') ; fix = TRUE ;
	   if (p2 == NULL || (p2 - p) > 15)
	    { UCCHAR save ; save = *(p+1) ; *(p+1) = UCEOS ; UCstrcat(tbuf,p1) ; *(p+1) = save ;
	      p1 = p + 1 ; continue ;
	    } ;
	   *p = UCEOS ; UCstrcat(tbuf,p1) ; 
	   p1 = p2 + 1 ; continue ; 
	 } ;
	if (fix) { UCstrcat(tbuf,p1) ; UCstrcpy(buf,tbuf) ; } ;
/*	Now sortof repeat above for "&xxx;" construct */
	for(tbuf[0]=UCEOS,p1=buf,fix=FALSE;(p=UCstrchr(p1,'&')) != NULL;)
	 { p2 = UCstrchr(p,';') ; fix = TRUE ;
	   if (p2 == NULL || (p2 - p) > 7)
	    { UCCHAR save ; save = *(p+1) ; *(p+1) = UCEOS ; UCstrcat(tbuf,p1) ; *(p+1) = save ;
	      p1 = p + 1 ; continue ;
	    } ;
	   *p = UCEOS ; UCstrcat(tbuf,p1) ; 
	   p1 = p2 + 1 ; continue ; 
	 } ;
	if (fix)
	 { UCstrcat(tbuf,p1) ; UCstrcpy(buf,tbuf) ; } ;
#endif
}

#include "v4rppxlsx.c"
#include "v4rppjson.c"
#include "v4rpptab.c"
//#include "v4rpptxt.c"

#ifdef MOOO
{meta:{status:xxx, msg:error-message},
  sheets:[ 
	   {sheetName:xxxx,
	    titles:[title1, title2, ...],
	    urlBases:[urlBase1, urlBase2, ...],
	    columnNames:[col1, col2],
	    colDefaults:[{v4PntType, css}, ...],		/* Defaults for each column: v4 point type, css formatting */
	    headings:[[heading 1], [heading 2]],
	    rows:[[row 1], [row 2], ...],
	    footers:[[footer 1], [footer 2]]
	   },...

	 ]
}
format for heading, row, and footer arrays-
there must be entry for each column, if column is empty then slot filled with null value
last element in array is additional-info array (or null if none)
[col1,col2,...coln, [ ]]
each element in additional-info array is object containing column:n object any number of the following
url:urlValue, spans:columns, v4DataType:n, css:string, textColor:xx, backgroundColor:xx, justify:{l,r,c}, bold:true, italic:true, underline:true


Be able to include format info (color, font, etc.)
URL links
Be able to indicate recap levels (exclude levels if resorting, dynamic drill-down)
Able to flag special rows (e.g. exceptions) that could be selected or excluded
Include non-tablular text (explanatory footnotes, help info)
#endif