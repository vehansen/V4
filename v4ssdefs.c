/*	V4SSDefs.c - Common Stuff for V4/SpreadSheet Xface	*/

/*      V4SS_Type_xxx - SpreadSheet Interface Codes */

#define V4SS_Type_EOF 0                 /* End of File (s/b 0 because "\0\0" forced to end of buffer in v4.xll) */
#define V4SS_Type_EOL 2                 /* End of Line/Row */
#define V4SS_Type_Int 3                 /* Integer */
#define V4SS_Type_Double 4              /* Double */
#define V4SS_Type_UDate 5               /* Date */
#define V4SS_Type_Alpha 6               /* Alpha - String */
#define V4SS_Type_UMonth 7              /* Universal month */
#define V4SS_Type_UQtr 8                /* Universal quarter */
#define V4SS_Type_UPeriod 9             /* Universal period */
#define V4SS_Type_UWeek 10              /* Universal week */
#define V4SS_Type_UndefVal 11           /* Undefined value */
#define V4SS_Type_Formula 12            /* Excel formula (=xxx) */
#define V4SS_Type_SSVal 13              /* Special spreadsheet value (see V4SS_SSVal_xxx below) */
#define V4SS_Type_Fixed 14              /* Fixed decimal (EchoS() passes as d.nnnnn where d is # decimals, nnnn is number */
#define V4SS_Type_FAlpha 15		/* Formula Alpha (first byte is FAlpha, second is Excel type, remainder per type */
#define V4SS_Type_Err 100               /* Error Message */
#define V4SS_Type_FormatControl 101     /* Format mode - embedded command */
#define V4SS_Type_FormatStyleName 102   /* Style name follows */
#define V4SS_Type_FormatFont 103        /* Font name */
#define V4SS_Type_FormatStyle 104       /* Style list follows (italic, bold, etc.) */
#define V4SS_Type_FormatFontSize 105    /* Font size */
#define V4SS_Type_FormatFormat 106      /* Format string follows (ex: "#,##0.00") */
#define V4SS_Type_PageBreak 107         /* Insert page break above current cell */
#define V4SS_Type_InsertRow 108         /* Insert row at current row (i.e. current becomes blank) */
#define V4SS_Type_SetColWidth 109       /* Set column with of current column */
#define V4SS_Type_Menu1 110             /* Begin definition of user menu 1 */
#define V4SS_Type_Menu2 111             /* Begin definition of user menu 2 */
#define V4SS_Type_Eval 112              /* Evaluate at current cell */
#define V4SS_Type_Justify 113           /* Justify cell contents */
#define V4SS_Type_Echo 114              /* Turn on screen echo */
#define V4SS_Type_NoEcho 115            /* Turn off screen echo */
#define V4SS_Type_Header 116            /* Set SS Header */
#define V4SS_Type_Footer 117            /* Set SS Footer */
#define V4SS_Type_LMargin 118           /* Left margin */
#define V4SS_Type_RMargin 119           /* Right margin */
#define V4SS_Type_TMargin 120           /* Top Margin */
#define V4SS_Type_BMargin 121           /* Bottom Margin */
#define V4SS_Type_Portrait 122          /* Portrait orientation */
#define V4SS_Type_Landscape 123         /* Landscape orientation */
#define V4SS_Type_Scale 124             /* Scaling (1-100%) */
#define V4SS_Type_Page 125              /* Page range to print */
#define V4SS_Type_SetPrintTitles1 126   /* Set Print Titles: Columns */
#define V4SS_Type_SetPrintTitles2 127   /* Set Print Titles: Rows */
#define V4SS_Type_SetColGap 128         /* Set column gap if snaking */
#define V4SS_Type_SetRowGap 129         /*  ditto for row gap */
#define V4SS_Type_SetRows 130           /* Number of rows if snaking */
#define V4SS_Type_SetColumns 131        /* Number of column (sets) if snaking */
#define V4SS_Type_AutoFit 132           /* Auto-fit all columns in current output */
#define V4SS_Type_Run 133		/* Run a macro */
#define V4SS_Type_XOffset 134		/* Adjust x-coord of entire result by this amount on input */
#define V4SS_Type_YOffset 135		/* Adjust y-coord of entire result by this amount on input */
#define V4SS_Type_UpdOK 136		/* Update went OK, reset cells in spreadsheet */
#define V4SS_Type_Sheet 137		/* Make the named worksheet active */
#define V4SS_Type_Row 138		/* Set new row (absolute or relative) */
#define V4SS_Type_Column 139		/* Set new columne (ditto) */
#define V4SS_Type_RowColor 140		/* Set background color of current row */
#define V4SS_Type_ColColor 141		/*  ditto for column */
#define V4SS_Type_CellColor 142		/*  ditto for column */
#define V4SS_Type_InsertObject 143	/* Insert external object into spreadsheet */
#define V4SS_Type_Note 144		/* Add note to current cell */
#define V4SS_Type_Message 145		/* Show message in current Excel workspace */
#define V4SS_Type_End 146		/* Show message in current Excel workspace */
#define V4SS_Type_PageBreakAfter 147	/* Insert page break below current cell */
#define V4SS_Type_Id 148		/* ID for lower left corner of page (via End::xxx) */
#define V4SS_Type_URLLink 149		/* URL link for following (next) value */
#define V4SS_Type_URLLinkX 150		/* Begin of URL link when > 255 bytes */
#define V4SS_Type_SSDim 151		/* Defines SSDim (index) for next value */
#define V4SS_Type_FormatSpec 152	/* Format specification associated with last SSDim (struct V4SS__FormatSpec) */
#define V4SS_Type_HTML 153		/* HTML code to be embedded within or following next column */
#define V4SS_Type_HTMLX 154		/* Begin of HTML when > 255 bytes */
#define V4SS_Type_URLBase 155		/* Defines URL base for future URLLinks */
#define V4SS_Type_URLBaseX 156		/* Defines URL base for future URLLinks (extension) */
#define V4SS_Type_Grid 157		/* Show grid lines */
#define V4SS_Type_FormatSpecNP 158	/* Format specification associated with next point (struct V4SS__FormatSpec) */
#define V4SS_Type_AutoHead 159		/* If TRUE then automatically format report headings */
#define V4SS_Type_GlobalFont 160	/* Global font name */
#define V4SS_Type_GlobalFontSize 161	/* Global font size */
#define V4SS_Type_HTTP_HTML 162		/* HTTP Headers for HTML output */
#define V4SS_Type_HTTP_Other 163	/* HTTP Headers for non HTML (e.g. Excel/BIFF) output */
#define V4SS_Type_FileName 164		/* FileName to be assigned to downloaded attachment */
#define V4SS_Type_FrameCnt 165		/* Number of frames to create in HTML */
#define V4SS_Type_URLAfterErr 166	/* URL to link to after xvrestohtml error popup window */
#define V4SS_Type_SetColWidthList 167	/* Set column width for all columns (argument is comma delimited list of widths, 0 = unchanged) */
#define V4SS_Type_ScriptInclude 168	/* Value is name of (Java)script file to include in vrestohtml output */
#define V4SS_Type_LongEntry 169		/* ** Long entry- head is 169,real-header,bytes>>8,bytes&0xff (NON STANDARD!!) ** */
#define V4SS_Type_Duplicate 170		/* Duplicate the html/excel file with given name (to save it permanently) */
#define V4SS_Type_HorRuleEvery 171	/* Output a horizontal rule every x rows/lines */
#define V4SS_Type_DHTMLEnable 172	/* Enable/Disable DHTML option */
#define V4SS_Type_BackgroundImage 173	/* Background Image Name */
#define V4SS_Type_BackgroundColor 174	/* Background Color */
#define V4SS_Type_Server 175		/* Address of CGI Server */
#define V4SS_Type_LogoImage 176		/* Logo Image for top of page */
#define V4SS_Type_LogoURL 177		/* URL to hyperlink to on logo */
#define V4SS_Type_Session 178		/* Session number/id */
#define V4SS_Type_DHTMLCode 179		/* HTML to inject to enable dynamic menus */
#define V4SS_Type_Alert 180		/* An alert message */
#define V4SS_Type_Warn 181		/* A warning message */
#define V4SS_Type_CSS 182		/* A CSS entry */
#define V4SS_Type_SSNotDim 183		/* Similar to Type_SSDim but used to pass Table/Row/Cell 'dimensions' */
#define V4SS_Type_Position 184		/* Position of division (value is 'height,width,left,top' positions) */
#define V4SS_Type_ExcelIni 185		/* Name of the V4-Excel "connection" so that V4R can be read into Excel & then use the "Expand" command */
#define V4SS_Type_Desc 186		/* Descriptive data (for saving result as a help example file) */
#define V4SS_Type_DescX 187		/* Begin of descriptive data when > 255 bytes */
#define V4SS_Type_URLBase2 188		/* See URLBase */
#define V4SS_Type_URLBase2X 189
#define V4SS_Type_URLLink2 190
#define V4SS_Type_URLLink2X 191
#define V4SS_Type_URLBase3 192		/* See URLBase */
#define V4SS_Type_URLBase3X 193
#define V4SS_Type_URLLink3 194
#define V4SS_Type_URLLink3X 195
#define V4SS_Type_URLBase4 196		/* See URLBase */
#define V4SS_Type_URLBase4X 197
#define V4SS_Type_URLLink4 198
#define V4SS_Type_URLLink4X 199
#define V4SS_Type_URLBase5 200		/* See URLBase */
#define V4SS_Type_URLBase5X 201
#define V4SS_Type_URLLink5 202
#define V4SS_Type_URLLink5X 203
#define V4SS_Type_Title 204
#define V4SS_Type_SourceJS 205		/* Javascript file to be referenced via <script src=xxx> */
#define V4SS_Type_SourceCSS 206		/* CSS file to be reference via <style src=xxx> */

#define V4SS_URLBaseMax 5		/* Max number of URLBase/URL combinations */


#define V4SS_ServerInputHdr_Ini 'I'	/* Name of initialization file */
#define V4SS_ServerInputHdr_Row 'r'     /* Header byte for row */
#define V4SS_ServerInputHdr_RowCap 'R'  /* Header byte for row caption */
#define V4SS_ServerInputHdr_Col 'c'     /* Header byte for column */
#define V4SS_ServerInputHdr_ColCap 'C'  /* Header byte for column caption */
#define V4SS_ServerInputHdr_Ctx 'x'     /* Global context */
#define V4SS_ServerInputHdr_Val 'v'     /* Value */
#define V4SS_ServerInputHdr_Area 'a'    /* Area to load */
#define V4SS_ServerInputHdr_Isct 'i'    /* An intersection: [xxx] */
#define V4SS_ServerInputHdr_Update 'u'    /* A V4 interpreter command to evaluate */
#define V4SS_ServerInputHdr_ResFile 'f'	/* Name of the result file */
#define V4SS_ServerInputHdr_UpdHdr 'h'	/* Common update header info */
#define V4SS_ServerInputHdr_BCol '1'    /* A binding column spec */
#define V4SS_ServerInputHdr_BRow '2'    /* A binding row spec */
#define V4SS_ServerInputHdr_BVal '3'    /* A binding value (in current row) */
#define V4SS_ServerInputHdr_BCtx '4'    /* A binding context point*/
#define V4SS_ServerInputHdr_BIsct '5'	/* A Binding intersection */
#define V4SS_ServerInputHdr_BIVal '6'	/* Value to a binding intersection */
#define V4SS_ServerInputHdr_UserName 'n'/* Excel login user name */


#define V4SS_SSVal_Null 1		/* #NULL! */
#define V4SS_SSVal_Div0 2		/* #DIV/0 */
#define V4SS_SSVal_Value 3		/* #VALUE! */
#define V4SS_SSVal_Ref 4		/* #REF! */
#define V4SS_SSVal_Name 5		/* #NAME! */
#define V4SS_SSVal_Num 6		/* #NUM! */
#define V4SS_SSVal_NA 7			/* #N/A */
#define V4SS_SSVal_Empty 8		/* no value, blank cell */
#define V4SS_SSVal_Row 9		/* Entire Row */
#define V4SS_SSVal_Column 10		/* Entire Column */


#define V4SS_Font_Italic 0x1
#define V4SS_Font_Underline 0x2
#define V4SS_Font_StrikeThru 0x4
#define V4SS_Font_Outline 0x8
#define V4SS_Font_Shadow 0x10
#define V4SS_Font_Wrap 0x20
#define V4SS_Font_SizeToFit 0x40
#define V4SS_Font_SubScript 0x80
#define V4SS_Font_SuperScript 0x100
#define V4SS_Font_NoBreak 0x200
#define V4SS_Font_Bold 0x400
#define V4SS_Font_UnderlineDbl 0x800
#define V4SS_Font_URL 0x1000

#define V4SS_FontBold_Normal 128
#define V4SS_FontBold_Bold 150

#define V4SS_ULStyle_Single 1
#define V4SS_ULStyle_Double 2
#define V4SS_ULStyle_SAcctng 3
#define V4SS_ULStyle_DAcctng 4

#define V4SS_Align_Left 1
#define V4SS_Align_Right 2
#define V4SS_Align_Center 3
#define V4SS_Align_Justify 4
#define V4SS_Align_Fill 5
#define V4SS_Align_MultiColCenter 6
#define V4SS_Align_Top 7
#define V4SS_Align_Bottom 8
#define V4SS_Align_MultiColLeft 9
#define V4SS_Align_MultiColRight 10
#define V4SS_Align_Indent1 11		/* Indent '1' level */
#define V4SS_Align_Indent2 12
#define V4SS_Align_Indent3 13

#define V4SS_Border_None 0
#define V4SS_Border_Thin 1
#define V4SS_Border_Medium 2
#define V4SS_Border_Dashed 3
#define V4SS_Border_Dotted 4
#define V4SS_Border_Thick 5
#define V4SS_Border_Double 6
#define V4SS_Border_Hairline 7

struct V4SS__FormatSpec {
  unsigned char Length ;		/* Length of this structure (VarString at end is variable) */
  unsigned short FontAttr ;		/* Font Attributes- see V4SS_Font_xxx */
  char FontSize ;			/* Font Point Size (> = absolute size), (<0 = -(size) %) */
  unsigned int FontColor ;
  unsigned int FgColor,BgColor ;	/* Foreground/background colors */
  unsigned int LBorderColor ;
  unsigned int RBorderColor ;
  unsigned int TBorderColor ;
  unsigned int BBorderColor ;
  unsigned char FontULStyle ;		/* see V4SS_ULStyle_xxx */
  unsigned char FontBold ;		/* Boldness value (1 = not bold, 128 = normal, 255 = very bold) */
  unsigned short FontFamily ;
  unsigned char PredefineFormat ;	/* Predefined format mask (if applicable) */
  unsigned char VAlign ;		/* Vertical Alignment - see V4SS_Align_xxx */
  unsigned char HAlign ;		/* Horizontal Alignment - see V4SS_Align_xxx */
  unsigned char Orientation ;
  unsigned char FillPattern ;
  unsigned char Width ;			/* Width of entry (for columns) 0=unused, 1-100 is %, 101-255 is pixels+100 */
  unsigned char CSSStyle ;		/* Index+1 into VarString for CSS style text */
  unsigned char CSSId ;			/* Index+1 into VarString for CSS Id */
  unsigned char CSSClass ;		/* Index+1 into VarString for CSS Class */
  unsigned char URLLink ;		/* Index+1 into VarString for URL link */

  unsigned char TBA1, TBA2, TBA3 ;	/* To Be Announced - hold for future */
  unsigned char pntType ;		/* V4 point type (if we want separate specs by point type) */
  unsigned char FontNameX ;		/* Index+1 into VarString below to ASCIZ font name */
  unsigned char MaskX ;			/* Index+1 into Format mask (ex: "#,##0.00") */
  char VarString[175] ;
} ;

#define V4SS__DimToFormat_Max 250
#define V4SS__DimOffsetForV4RPP 10	/* Offset to add to all 'dimension ids' from V4 -> V4RPP (to skip over predefined ones) */

struct V4SS__DimToFormatMap {
  int Count ;				/* Number defined below */
  int RevLevel ;			/* Revision level (incremented with each update) */
  int xmitRevLevel ;			/* Rev level of last transmission (where applicable) */
  char GlobalFont[128] ;		/* Global Font Name */
  int GlobalFontSize ;
  DIMID dimIdList[V4SS__DimToFormat_Max] ;	/* List of dimension id's */
  struct V4SS__FormatSpec *vfs[V4SS__DimToFormat_Max] ; /* Pointer to format structure for dimension */
} ;

#define V4SS_FormatType_Init 0		/* Initialize vfs structure (spec argument ignored) */
#define V4SS_FormatType_Mask 1		/* String is formatting mask (ex: "#,###.00") */
#define V4SS_FormatType_Style 2		/* Formatting styles (ex: bold, underline, ... ) */
#define V4SS_FormatType_Color 3
#define V4SS_FormatType_Size 4		/* Font size */
#define V4SS_FormatType_Font 5		/* Font Name */
#define V4SS_FormatType_CellColor 6	/* Cell Color */
#define V4SS_FormatType_TextColor 7	/* Text Color */
#define V4SS_FormatType_CSSStyle 8	/* CSS Style text */
#define V4SS_FormatType_CSSId 9		/* CSS Id name */
#define V4SS_FormatType_CSSClass 10	/* CSS Class name */
#define V4SS_FormatType_URLLink 11	/* URL link/javascript-call */

#define V4SS_DimId_Table -10		/* Special DimId for Table */
#define V4SS_DimId_Row -11		/* For Rows */
#define V4SS_DimId_Cell -12		/* (you get the idea) */
#define V4SS_DimId_HdrRow -13
#define V4SS_DimId_HdrCell -14
#define V4SS_DimId_TopOfPage -15

/*	N E W   D E F S		*/

#define V4RH_Entry_Max ((CCOUNT)32000)		/* Max length of V4R entry */
#define V4RH_Name_Max ((CCOUNT)63)		/* Max length of name (tag/section) */
#define V4RH_MAXIFS 32				/* Max number of nested ##IF statements */

#define V4RH_ETYPE_Tag		1		/* Tag entry in 'symbol' table */
#define V4RH_ETYPE_Section	2		/* Section entry */
#define V4RH_ETYPE_GenCSS	3		/* On demand - generate CSS for report */
#define V4RH_ETYPE_GenJS	4		/* On demand - generate javascript for report */
#define V4RH_ETYPE_GenColGroup	5		/* On demand - generate javascript for report */
#define V4RH_ETYPE_GenHTMLInclude 6		/* On demand - insert any 'INCLUDE::xxx.xxx' files */
#define V4RH_ETYPE_GenIncludeBottom 7		/* On demand - insert any 'include at bottom' text */
#define V4RH_ETYPE_GenIncludeTop 8		/* On demand - insert any 'include at top' text */
#define V4RH_ETYPE_GenLinkInclude 9		/* On demand - insert all JS/CSS linked libraries */

#define V4R_Type_BOF		1		/* If first two bytes are not [2][BOF] then not V4R file */
#define V4R_Type_EOF		2		/* End of file, no entries should follow */
#define V4R_Type_V4Pnt		3		/* Data is V4 point converted to ASCII [type=V4R_Type_V4Pnt][point-type][ascii equiv]... */
#define V4R_Type_V4PntWithDim	4		/* Data is V4 point & dimension converted to ASCII [type=V4R_Type_V4Pnt][point-type][dimId low8][dimId high8][ascii equiv]... */
#define V4R_Type_SpecVal	5		/* Special (Excel) Value, see V4R_SpecVal_Null */
#define V4R_Type_XLFormula	6		/* Excel formula */
#define V4R_Type_XLInfo		7		/* Excel margin [len][XLInfo][see V4R_XL_xxx][data] ... */
#define V4R_Type_XLAutoFit	8		/* Auto-fit contents */
#define V4R_Type_FormatSpec	9		/* Dimension format information [len][DimFormat][dimId low8][dimId high8][format info].... */
#define V4R_Type_ColWidth	10		/* Width of next column */
#define V4R_Type_MultColWidth	11		/* Width of multiple columns [len][MultColWidth][colCount][width0 low8][width0 high8]... */
#define V4R_Type_PageBreak	12		/* Insert page break [len][PageBreak][1=before this, 2=after this] */
#define V4R_Type_RowColor	13
#define V4R_Type_ColColor	14
#define V4R_Type_PageInfo	15		/* Page setup information [len][PageInfo][see V4R_Page_xxx][data] ... */

#define V4R_Type_UTF16		128		/* UTF-16 string */
#define V4R_Type_UTF16WithDim	129		/* UTF-16 string [len][UTF16WithDim][dimId low8][dimId high8][data]... */
#define V4R_Type_Err		130		/* [len][Err][error-type (err, alert, warn)[Error message]... */
#define V4R_Type_ErrURL		131		/* URL to link to after error message alert */
#define V4R_Type_URLBase	132		/* URL Base [len][URLBase][base index][data]... */
#define V4R_Type_URL		133		/* URL [len][URL][base index][data]... */
#define V4R_Type_Sheet		134		/* Set (next) sheet name */
#define V4R_Type_VInfo		135		/* Report version/meta-data [len][VInfo][V4R_VInfo_xxx][data] */
#define V4R_Type_CSSText	136
#define V4R_Type_JSText		137
#define V4R_Type_Note		138		/* Note/text to be attached to next value */
#define V4R_Type_BeginTag	139		/* Start of tagged section  (standard tags: Title, Heading, Footer) */
#define V4R_Type_EndTag		140		/* End of tagged section */
#define V4R_Type_TaggedVal	141		/* A tagged value of form: tag=value.... or tag+=value (concats to end of current tag value) */
						/* Tags are defined in pass 1, and redefined in pass2 (therefore any that are updated are handled properly) */
#define V4R_Type_Section	142		/* Defines Section 'subname' (ex: topOfPage.linkMenu) */



#define V4R_XL_AutoFit		1
#define V4R_XL_FreezeCols	2		/* Number of columns to freeze */
#define V4R_XL_FreezeRows	3		/* Number of rows to freeze */

#define V4R_Page_LMargin	1
#define V4R_Page_RMargin	2
#define V4R_Page_TMargin	3
#define V4R_Page_BMargin	4
#define V4R_Page_Orientation	5
#define V4R_Page_Scale		6
#define V4R_Page_ShowGrid	7		/* Grid type 



#define V4SS_Type_Fixed 14              /* Fixed decimal (EchoS() passes as d.nnnnn where d is # decimals, nnnn is number */
#define V4SS_Type_FAlpha 15		/* Formula Alpha (first byte is FAlpha, second is Excel type, remainder per type */
#define V4SS_Type_HTML 153		/* HTML code to be embedded within or following next column */
#define V4SS_Type_HTMLX 154		/* Begin of HTML when > 255 bytes */
#define V4SS_Type_FormatSpecNP 158	/* Format specification associated with next point (struct V4SS__FormatSpec) */
#define V4SS_Type_SSNotDim 183		/* Similar to Type_SSDim but used to pass Table/Row/Cell 'dimensions' */


#define V4R_SpecVal_Null 1		/* #NULL! */
#define V4R_SpecVal_Div0 2		/* #DIV/0 */
#define V4R_SpecVal_Value 3		/* #VALUE! */
#define V4R_SpecVal_Ref 4		/* #REF! */
#define V4R_SpecVal_Name 5		/* #NAME! */
#define V4R_SpecVal_Num 6		/* #NUM! */
#define V4R_SpecVal_NA 7		/* #N/A */
#define V4R_SpecVal_Empty 8		/* no value, blank cell */
#define V4R_SpecVal_Row 9		/* Entire Row */
#define V4R_SpecVal_Column 10		/* Entire Column */
