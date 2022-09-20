/*	Issues to be fixed/tested
  Column widths too narrow on 2007
  size-to-fit
  fit on page (one page across, multiple down)
  page breaks - not working
  rows repeating top-of-page
  printing masks
  width

Done------------------------------------  
  Hyperlinks missing _Area & _sesKey
  Hyperlinks not showing with underline
  Multi-line headings showing on 1 line
  rows-repeat on print (does it work?)
  strike-through
  font-size
  grid
  meta-data: user, version, id, memo, name, privs
  ability to have descriptive data (like html title='xx')
  formulas

*/
/*	G E N E R A T E   X L S X   F I L E		*/

#define V4RPP_ExcelDflt_MarginLeft 0.5
#define V4RPP_ExcelDflt_MarginRight 0.5
#define V4RPP_ExcelDflt_MarginTop 0.75
#define V4RPP_ExcelDflt_MarginBottom 0.75
#define V4RPP_ExcelDflt_HdrMargin 0.3
#define V4RPP_ExcelDflt_FtrMargin 0.3

struct V4XLSX__XMLS {
  UCCHAR *xmlBase ;			/* Pointer to begin of current XML string */
  UCCHAR *xml ;				/* Pointer to [end of] current XML string */
  LENMAX maxChar ;			/* Maximum number of characters in string */
} ;

/*	M A I N   X L S X   S T R U C T U R E	*/

#define V4XLSX_SHEET_MAX 256
#define V4XLSX_MRUMAX 256
#define V4XLSX_PAGEBRK_MAX 512
struct V4XLSX__Main {
  struct V4XLSX__ssHash *ssh ;
  struct V4XLSX__ContentTypes *ctyp ;
  struct V4XLSX__4MAT_numFmts *vnnf ;
  struct V4XLSX__4MAT_fonts *vnf ;
  struct V4XLSX__4MAT_cellXfs *vcxfs ;		/* This is formatting associated with a cell */
  struct V4XLSX__4MAT_cellXfs *vcsxfs ;		/* This is formatting associated with a style */
  struct V4XLSX__4MAT_fills *vfl ;
  struct V4XLSX__4MAT_borders *vbor ;
  struct V4XLSX__4MAT_CellStyles *vstyle ;
  struct V4XLSX__4MAT_Drawing *vdrw ;
  struct V4XLSX__4MAT_Comment *vcom ;
  UCCHAR *cmpName, *mgrName ;			/* If not NULL then company/manager name */
//  UCCHAR *urlBase ;				/* If not NULL then hyperlink base URL */
  UCCHAR *urlSuffix ;				/* If not NULL then suffix info to be appended to all V4 URLs */
  COUNTER mruCount ;				/* Number of custom colors referenced */
  FLAGS32 mruList[V4XLSX_MRUMAX] ;		/* List of 32-bit hex color codes */
  struct UC__File UCFile ;			/* If saving secure URLs then output actual URLs to this file */
  COUNTER urlX ;				/* If saving secure URLs then keep track of URL count here */
  INDEX numSheets ;
  struct {
   struct V4XLSX__SheetXMLRels *lastSXR ;	/* Pointer to last allocated relationship for this sheet */
   struct V4XLSX__XMLS xmls ;
   struct V4XLSX__CellSpans *vcs ;		/* If not NULL then links to table of cell-spannings */
   struct V4XLSX__HyperLinks *vhl ;		/* If not NULL then links to hyperlinks */
   COUNTER numBrks ;				/* Number of page breaks */
   INDEX brkRow[V4XLSX_PAGEBRK_MAX] ;		/* Insert break after this row (0 indexed) */
   } sheet[V4XLSX_SHEET_MAX] ;
} ;

#define V4XLSX_Comment_Max 1024
struct V4XLSX__4MAT_Comment {
  COUNTER num ;
  struct {
    INDEX row,col,sheetX ;			/* Position of the comment */
    UCCHAR *str ;				/* The actual comment */
   } entry[V4XLSX_Comment_Max] ;
} ;

#define V4XLSX_Drawings_EntryMax 1024		/* Number of image entries (cells containing/anchoring an image) */
#define V4XLSX_Drawings_ImgMax 512		/* Number of images */
struct V4XLSX__4MAT_Drawing {
  COUNTER numEntries ;
  struct { 
    INDEX row,col,sheetX ;			/* row, column & sheet where drawing is anchored */
    INDEX rows,cols ;				/* Number of rows/columns image spans (default is within anchor cell only) */
    INDEX imgX ;				/* Index to image below (we may have mutliple references to same image) */
   } entry[V4XLSX_Drawings_EntryMax] ;
  COUNTER numImgs ;
  struct {
    INDEX rId ;					/* Id of _rels describing image */
    UCCHAR *path ;				/* path-name of the image */
   } img[V4XLSX_Drawings_ImgMax] ;
} ;

#define V4XLSX_CellStyles_Max 1024
struct V4XLSX__4MAT_CellStyles {
  COUNTER num ;
  struct {
    UCCHAR name[32] ;
    INDEX builtInId ;
    INDEX xfId ;				/* 0-based index into cellXfs entries */
   } entry[V4XLSX_CellStyles_Max] ;
} ;

INDEX v4rx_GetIndex_cellStyle(vrhm,name,builtInId,xfId)
  struct V4RH__Main *vrhm ;
  UCCHAR *name ;
  INDEX builtInId ;
  INDEX xfId ;
{ INDEX i ;
  struct V4XLSX__4MAT_CellStyles *vstyle ;

	vstyle = vrhm->xlsx->vstyle ;
/*	If first call then make sure we throw in a 'normal' style */
	if (vstyle->num == 0)
	 { UCstrcpy(vstyle->entry[0].name,UClit("Normal")) ; vstyle->entry[0].builtInId = 0 ; vstyle->entry[0].xfId = 0 ;
	   vstyle->num++ ;
	 } ;
	memset(&vstyle->entry[vstyle->num],0,sizeof vstyle->entry[vstyle->num]) ;
	UCstrcpy(vstyle->entry[vstyle->num].name,name) ; vstyle->entry[vstyle->num].builtInId = builtInId ; vstyle->entry[vstyle->num].xfId = xfId ;
	for(i=0;i<vstyle->num;i++)
	 { if (memcmp(&vstyle->entry[i],&vstyle->entry[vstyle->num],sizeof vstyle->entry[i]) == 0) break ; } ;
	if (i >= vstyle->num) vstyle->num++ ;
	return(i) ;
}


#define V4XLSX_HyperLink_Dflt 2048
struct V4XLSX__HyperLinks {
  COUNTER num ;
  COUNTER maxNum ;
  struct {
    INDEX row,col ;				/* Row-column to be linked */
    UCCHAR *url ;				/* The hyperlink URL */
   } entry[V4XLSX_HyperLink_Dflt] ;
} ;

#define V4XLSX_CellSpans_Default 4096
struct V4XLSX__CellSpans {
  COUNTER num ;					/* Number of cell spans below */
  COUNTER maxNum ;
  struct {
    INDEX row,col ;				/* Starting row-column spec */
    COUNTER span ;				/* Number of columns of spanning */
   } entry[V4XLSX_CellSpans_Default] ;
} ;

/* Append STR to end of XMLS structure string */
#define XMLSContents(XMLS) ((XMLS)->xmlBase)
#define XMLSInit(XMLS) \
 (XMLS)->maxChar = 0x1000 ; (XMLS)->xmlBase = v4mm_AllocUC((XMLS)->maxChar) ; (XMLS)->xml = (XMLS)->xmlBase ; ZUS((XMLS)->xmlBase) ;
#define XMLSClose(XMLS) v4mm_FreeChunk((XMLS)->xmlBase) ;
#define XMLSAppend(XMLS,STR) \
 { LENMAX l = UCstrlen(STR) ; \
   if ((((XMLS)->xml - (XMLS)->xmlBase) + l) >= (XMLS)->maxChar) \
    { INDEX e = ((XMLS)->xml - (XMLS)->xmlBase) ; \
      (XMLS)->maxChar += (l > 0x10000 ? (l+0x1000) : 0x10000) ; (XMLS)->xmlBase = (UCCHAR *)realloc((XMLS)->xmlBase,(XMLS)->maxChar*sizeof(UCCHAR)) ; \
      (XMLS)->xml = &(XMLS)->xmlBase[e] ; \
    } ; \
   UCstrcat((XMLS)->xml,STR) ; (XMLS)->xml = UCstrchr((XMLS)->xml,'\0') ; \
 }

#define V4XLSX_MAXnumFmts 128
struct V4XLSX__4MAT_numFmts {
  INDEX num ;				/* Number below */
  COUNTER fmtId[V4XLSX_MAXnumFmts+1] ;	/* Corresponding Ids for entry */
  struct {
    UCCHAR formatCode[64] ;
   } entry[V4XLSX_MAXnumFmts+1] ;
} ;

INDEX v4rx_GetIndex_numFmts(vrhm,formatCode)
  struct V4RH__Main *vrhm ;
  UCCHAR *formatCode ;
{ INDEX i ;
  struct V4XLSX__4MAT_numFmts *vnnf ;

	if (formatCode == NULL ? TRUE : UCempty(formatCode)) return(0) ;		/* No format given - return 0 for 'general' */
	vnnf = vrhm->xlsx->vnnf ;
	memset(&vnnf->entry[vnnf->num],0,sizeof vnnf->entry[vnnf->num]) ;
	if (formatCode != NULL) UCstrcpy(vnnf->entry[vnnf->num].formatCode,formatCode) ;
	for(i=0;i<vnnf->num;i++)
	 { if (memcmp(&vnnf->entry[i],&vnnf->entry[vnnf->num],sizeof vnnf->entry[i]) == 0) break ; } ;
	if (i >= vnnf->num)
	 { vnnf->fmtId[vnnf->num] = 164 + vnnf->num ;
	   vnnf->num++ ;
	 } ;
	return(vnnf->fmtId[i]) ;
	
}

#define V4XLSX_MAXfonts 128
struct V4XLSX__4MAT_fonts {
  INDEX num ;
  struct {
    COUNTER sz ;
    int color ;
    UCCHAR name[64] ;			/* Font family name */
    FLAGS32 fontAttr ;
   } entry[V4XLSX_MAXfonts+1] ;
} ;

INDEX v4rx_GetIndex_fonts(vrhm,sz,color,name,fontAttr)
  struct V4RH__Main *vrhm ;
  COUNTER sz ;
  int color ;
  UCCHAR *name ;
  FLAGS32 fontAttr ;
{ INDEX i ;
  struct V4XLSX__4MAT_fonts *vnf ;

	vnf = vrhm->xlsx->vnf ;
	if (sz == 0 && color == 0 && (name == NULL ? TRUE : UCempty(name)) && fontAttr == 0)
	 return(0) ;		//Return default if nothing specified
	memset(&vnf->entry[vnf->num],0,sizeof vnf->entry[vnf->num]) ;
	vnf->entry[vnf->num].sz = sz ; vnf->entry[vnf->num].color = color ; vnf->entry[vnf->num].fontAttr = fontAttr ;
	if(name != NULL) UCstrcpy(vnf->entry[vnf->num].name,name) ;
	for(i=0;i<vnf->num;i++)
	 { if (memcmp(&vnf->entry[i],&vnf->entry[vnf->num],sizeof vnf->entry[i]) == 0) break ; } ;
	if (i >= vnf->num) vnf->num++ ;
	return(i) ;
}

#define V4XLSX_MAXfills 128
#define V4XLSX_patternType_none 0
#define V4XLSX_patternType_solid 1
#define V4XLSX_patternType_gray125 2

static UCCHAR v4rx_Fills[][32] = { UClit("none"), UClit("solid"), UClit("gray125")  } ;
struct V4XLSX__4MAT_fills {
  INDEX num ;
  struct {
    ETYPE patternType ;
    int fgColor ;
    int bgColor ;
   } entry[V4XLSX_MAXfills+1] ;
} ;

INDEX v4rx_GetIndex_fills(vrhm,patternType,fgColor,bgColor)
  struct V4RH__Main *vrhm ;
  ETYPE patternType ;
  int fgColor ;
  int bgColor ;
{ INDEX i ;
  struct V4XLSX__4MAT_fills *vfl ;

	vfl = vrhm->xlsx->vfl ;
	memset(&vfl->entry[vfl->num],0,sizeof vfl->entry[vfl->num]) ;
	vfl->entry[vfl->num].patternType = patternType ;
	vfl->entry[vfl->num].fgColor = fgColor ;
	vfl->entry[vfl->num].bgColor = bgColor ;
	for(i=0;i<vfl->num;i++)
	 { if (memcmp(&vfl->entry[i],&vfl->entry[vfl->num],sizeof vfl->entry[i]) == 0) break ; } ;
	if (i >= vfl->num) vfl->num++ ;
	return(i) ;
}

#define V4XLSX_MAXborders 128
#define V4XLSX_borderType_none 0
#define V4XLSX_borderType_medium 1
static UCCHAR v4rx_Borders[][32] = { UClit("none"), UClit("medium") } ;
struct V4XLSX__4MAT_borders {
  INDEX num ;
  struct {
    ETYPE left,right,top,bottom ;
    ETYPE borderType ;
   } entry[V4XLSX_MAXborders+1] ;
} ;

INDEX v4rx_GetIndex_borders(vrhm,left,right,top,bottom,borderType)
  struct V4RH__Main *vrhm ;
  LOGICAL left,right,top,bottom ;
  ETYPE borderType ;
{ INDEX i ;
  struct V4XLSX__4MAT_borders *vbor ;

	vbor = vrhm->xlsx->vbor ;
	memset(&vbor->entry[vbor->num],0,sizeof vbor->entry[vbor->num]) ;
#define SETBOR(EL) vbor->entry[vbor->num].EL = EL
	SETBOR(left) ; SETBOR(right) ; SETBOR(top) ; SETBOR(bottom) ; SETBOR(borderType) ;
	for(i=0;i<vbor->num;i++)
	 { if (memcmp(&vbor->entry[i],&vbor->entry[vbor->num],sizeof vbor->entry[i]) == 0) break ; } ;
	if (i >= vbor->num) vbor->num++ ;
	return(i) ;
}


#define V4XLSX_MAXcellXfs 512		/* Max number of Xfs entries we can have */
struct V4XLSX__4MAT_cellXfs {
  COUNTER num ;				/* Number below */
  struct {
    INDEX numFmtId, fontId, fillId, borderId,xfId ;
    LOGICAL wrapLines ;			/* TRUE if cell text is to wrap multiple lines */
    FLAGS32 horAlign ;			/* If nonzero then see V4SS_Align_xxx */
   } entry[V4XLSX_MAXcellXfs+1] ;	/* Allocate extra because 'last' is always used as staging entry */
} ;

INDEX v4rx_GetIndex_cellStyleXfs(vrhm,mask,fontSize,textColor,fontName,fontAttr,horAlign,wrapLines)
  struct V4RH__Main *vrhm ;
  UCCHAR *mask ;
  LENMAX fontSize ;
  ETYPE textColor ;
  UCCHAR *fontName ;
  FLAGS32 fontAttr ;
  ETYPE horAlign ;
  LOGICAL wrapLines ;
{ INDEX i ;
  struct V4XLSX__4MAT_cellXfs *vcxfs ;

	vcxfs = vrhm->xlsx->vcsxfs ;
	memset(&vcxfs->entry[vcxfs->num],0,sizeof vcxfs->entry[vcxfs->num]) ;
	vcxfs->entry[vcxfs->num].numFmtId = v4rx_GetIndex_numFmts(vrhm,mask) ;
	vcxfs->entry[vcxfs->num].fontId = v4rx_GetIndex_fonts(vrhm,fontSize,textColor,fontName,fontAttr) ;
	vcxfs->entry[vcxfs->num].fillId = 0 ;
	vcxfs->entry[vcxfs->num].borderId = v4rx_GetIndex_borders(vrhm,0,0,0,0,0) ;
	vcxfs->entry[vcxfs->num].xfId = 0 ;
	vcxfs->entry[vcxfs->num].horAlign = horAlign ;
	vcxfs->entry[vcxfs->num].wrapLines = wrapLines ;
	for(i=0;i<vcxfs->num;i++)
	 { if (memcmp(&vcxfs->entry[i],&vcxfs->entry[vcxfs->num],sizeof vcxfs->entry[i]) == 0) break ; } ;
	if (i >= vcxfs->num) vcxfs->num++ ;
	return(i) ;
}

INDEX v4rx_GetIndex_cellXfs(vrhm,vfi,mask,xfId,isURL,wrapLines)
  struct V4RH__Main *vrhm ;
  struct V4RH__FormatInfo *vfi ;
  UCCHAR *mask ;
  INDEX xfId ;
  LOGICAL isURL ;
  LOGICAL wrapLines;
{ INDEX i ;
  struct V4XLSX__4MAT_cellXfs *vcxfs ;
  LOGICAL localUL ; ETYPE localColor ;

	vcxfs = vrhm->xlsx->vcxfs ;
	memset(&vcxfs->entry[vcxfs->num],0,sizeof vcxfs->entry[vcxfs->num]) ;
	vcxfs->entry[vcxfs->num].numFmtId = v4rx_GetIndex_numFmts(vrhm,mask) ;
	localColor = (isURL ? v_ColorNameToRef(UClit("blue")) : vfi->textColor) ;
	localUL = (isURL ? TRUE : ((vfi->fontAttr & V4SS_Font_Underline) !=0)) ;
	vcxfs->entry[vcxfs->num].fontId = v4rx_GetIndex_fonts(vrhm,vfi->fontSize,localColor,vfi->fontName,(vfi->fontAttr | (localUL ? V4SS_Font_URL : 0))) ;
	vcxfs->entry[vcxfs->num].fillId = v4rx_GetIndex_fills(vrhm,(vfi->bgColor != 0 ? V4XLSX_patternType_solid : V4XLSX_patternType_none),0,vfi->bgColor) ;
	vcxfs->entry[vcxfs->num].borderId = v4rx_GetIndex_borders(vrhm,0,0,0,0,0) ;
	vcxfs->entry[vcxfs->num].xfId = xfId ;
	vcxfs->entry[vcxfs->num].horAlign = vfi->horAlign ;
	vcxfs->entry[vcxfs->num].wrapLines = wrapLines ;
	for(i=0;i<vcxfs->num;i++)
	 { if (memcmp(&vcxfs->entry[i],&vcxfs->entry[vcxfs->num],sizeof vcxfs->entry[i]) == 0) break ; } ;
	if (i >= vcxfs->num) vcxfs->num++ ;
	return(i) ;
}

/*	v4rx_FixUpStr - Fixes string so that it can be embedded in XML		*/
/*	Call: eos = v4rx_FixUpStr( dst , src , isURL )
	  where eos is end of dst string (pointer to terminating UCEOS in dst),
		dst is updated with corrected string (better be long enough!),
		src is string to be fixed up,
		isURL is TRUE if string is URL, FALSE otherwise			*/ 

UCCHAR *v4rx_FixUpStr(dst,src,isURL)
  UCCHAR *dst, *src ;
  LOGICAL isURL ;
{ INDEX i,j ;

	ZUS(dst) ;
	for(i=0,j=0;src[i]!=UCEOS;i++)
	 { switch (src[i])
	    { default:		dst[j++] = src[i] ; break ;
	      case UClit('&'):	dst[j++] = UClit('&') ; dst[j++] = UClit('a') ; dst[j++] = UClit('m') ; dst[j++] = UClit('p') ; dst[j++] = UClit(';') ; break ;
	      case UClit('<'):	dst[j++] = UClit('&') ; dst[j++] = UClit('l') ; dst[j++] = UClit('t') ; dst[j++] = UClit(';') ; break ;
	      case UClit(' '):  dst[j++] = (isURL ? UClit('+') : UClit(' ')) ; break ;
	    } ;
	 } ;
	dst[j] = UCEOS ;
	return(&dst[j]) ;
}

/*	This is called to generate <xf> entries for <cellXfs> and <cellStyleXfs> sections */
void v4rx_Generate_Xf_Entries(xmls,vcxfs,isCellStyle)
  struct V4XLSX__XMLS *xmls ;
  struct V4XLSX__4MAT_cellXfs *vcxfs ;
  LOGICAL isCellStyle ;
{  INDEX i ; UCCHAR cbuf[512] ;
  
	for(i=0;i<vcxfs->num;i++)
	 { UCCHAR xfIdBuf[32],applyFill[24],applyFont[24] ;
	   if (isCellStyle) { ZUS(xfIdBuf) ; }		/* <cellStyleXfs> entries do not contain xfId attribute */
	    else { UCsprintf(xfIdBuf,UCsizeof(xfIdBuf),UClit(" xfId=\"%d\""),vcxfs->entry[i].xfId) ; } ;
	   if (vcxfs->entry[i].fillId != 0)
	    { UCstrcpy(applyFill,UClit(" applyFill=\"1\"")) ; } else ZUS(applyFill) ;
	   if (vcxfs->entry[i].fontId != 0 && FALSE) { UCstrcpy(applyFont,UClit(" applyFont=\"1\"")) ; } else ZUS(applyFont) ;
/*	   Do we have anything to put between <xf> & </xf>? If not then generate single <xf/> */
	   if (vcxfs->entry[i].horAlign == 0)
	    { 
	      UCsprintf(cbuf,UCsizeof(cbuf),UClit("<xf numFmtId=\"%d\" fontId=\"%d\" fillId=\"%d\" borderId=\"%d\"%s%s%s/>"),vcxfs->entry[i].numFmtId,
		vcxfs->entry[i].fontId,vcxfs->entry[i].fillId,vcxfs->entry[i].borderId,xfIdBuf,applyFill,applyFont) ;
	      XMLSAppend(xmls,cbuf) ;
	    } else
	    { UCsprintf(cbuf,UCsizeof(cbuf),UClit("<xf numFmtId=\"%d\" fontId=\"%d\" fillId=\"%d\" borderId=\"%d\"%s applyAlignment=\"1\"%s%s>"),vcxfs->entry[i].numFmtId,
		vcxfs->entry[i].fontId,vcxfs->entry[i].fillId,vcxfs->entry[i].borderId,xfIdBuf,applyFill,applyFont) ;
	      XMLSAppend(xmls,cbuf) ;
/*	      Want to make sure we add '<alignment wrapText="1"/>' if multiline text */
#define HORALIGN(TYPE,ATTR) (vcxfs->entry[i].horAlign == V4SS_Align_##TYPE) { XMLSAppend(xmls,UClit(" horizontal=\"") UClit(#ATTR) UClit("\"")) ; }
	      if (vcxfs->entry[i].horAlign != 0 || vcxfs->entry[i].wrapLines)
	       { XMLSAppend(xmls,UClit("<alignment ")) ;
		 if HORALIGN(Center,center)
		  else if HORALIGN(Fill,fill)
		  else if HORALIGN(Justify,justify)
		  else if HORALIGN(Left,left)
		  else if HORALIGN(Right,right) ;
		 if (vcxfs->entry[i].wrapLines) { XMLSAppend(xmls,UClit(" wrapText=\"1\"")) ; } ;
	         XMLSAppend(xmls,UClit("/>")) ;
	       } ;
#define VERALIGN(TYPE,ATTR) (vcxfs->entry[i].horAlign == V4SS_Align_##TYPE) { XMLSAppend(xmls,UClit("<alignment vertical=\"") UClit(#ATTR) UClit("\"/>")) ; }
	      if VERALIGN(Top,top)
	       else if VERALIGN(Bottom,bottom) ;
	      XMLSAppend(xmls,UClit("</xf>")) ;
	    }; 
	 } ;
}

/*	Returns 'ARGB' color with alpha always 0xFF */
FLAGS32 v4rx_RGBColor(vrhm,refColor)
  struct V4RH__Main *vrhm ;
  int refColor ;
{ FLAGS32 rgb ; INDEX i ;

	rgb = 0xFF000000 + v_ColorRefToRGB(refColor) ;
	for(i=0;i<vrhm->xlsx->mruCount;i++) { if (rgb == vrhm->xlsx->mruList[i]) break ; } ;
	if (i >= vrhm->xlsx->mruCount && vrhm->xlsx->mruCount < V4XLSX_MRUMAX)
	 { vrhm->xlsx->mruList[vrhm->xlsx->mruCount++] = rgb ; } ;
	return(rgb) ;
}

UCCHAR *v4rx_GenerateSTYLEXML(vrhm)
  struct V4RH__Main *vrhm ;
{ struct V4XLSX__XMLS xmls ;
  struct V4XLSX__4MAT_numFmts *vnnf ;
  struct V4XLSX__4MAT_fonts *vnf ;
  struct V4XLSX__4MAT_fills *vfl ;
  struct V4XLSX__4MAT_borders *vbor ;
  UCCHAR tbuf[1024] ; INDEX i ;
#define xmlStart UClit("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">")
//#define xmlEnd UClit("	<dxfs count=\"0\"/><tableStyles count=\"0\" defaultTableStyle=\"TableStyleMedium9\" defaultPivotStyle=\"PivotStyleLight16\"/></styleSheet>")
#define xmlEnd UClit("</styleSheet>")
	XMLSInit(&xmls) ;
	XMLSAppend(&xmls,xmlStart) ;
/*	Generate all numFmts */
	vnnf = vrhm->xlsx->vnnf ;
	UCsprintf(tbuf,UCsizeof(tbuf),UClit("<numFmts count=\"%d\">"),vnnf->num) ; XMLSAppend(&xmls,tbuf) ;
	for(i=0;i<vnnf->num;i++)
	 { UCCHAR nbuf[1024] ;
	   UCsprintf(nbuf,UCsizeof(nbuf),UClit("<numFmt numFmtId=\"%d\" formatCode=\"%s\"/>"),vnnf->fmtId[i],vnnf->entry[i].formatCode) ;
	   XMLSAppend(&xmls,nbuf) ;
	 } ; XMLSAppend(&xmls,UClit("</numFmts>")) ;
/*	Now the fonts */
#define FONT(FA) ((vnf->entry[i].fontAttr & V4SS_Font_##FA) != 0)

	vnf = vrhm->xlsx->vnf ;
	UCsprintf(tbuf,UCsizeof(tbuf),UClit("<fonts count=\"%d\">"),vnf->num) ; XMLSAppend(&xmls,tbuf) ;
	for(i=0;i<vnf->num;i++)
	 { UCCHAR nbuf[1024],szBuf[32],colorBuf[128],familyBuf[32],attrBuf[64] ; ETYPE color ;
	   if (vnf->entry[i].sz != 0)
	    { /* If sz is < 0 then it's a %. Assuming base font size is 10 then just take 1/10 of the absolute value */
	      UCsprintf(szBuf,UCsizeof(szBuf),UClit("<sz val=\"%d\"/>"),(vnf->entry[i].sz > 0 ? vnf->entry[i].sz : (-vnf->entry[i].sz)/10)) ;
	    } else { ZUS(szBuf) ; } ;
	   if (FONT(URL)) { color = (vnf->entry[i].color == 0 ? v_ColorNameToRef(UClit("blue")) : vnf->entry[i].color) ; }
	    else { color = vnf->entry[i].color ; } ;
	   if (color != 0)
	    { UCsprintf(colorBuf,UCsizeof(colorBuf),UClit("<color rgb=\"%x\"/>"),v4rx_RGBColor(vrhm,vnf->entry[i].color)) ; } else { UCstrcpy(colorBuf,UClit("<color theme=\"1\"/>")) ; } ;
	   if (UCnotempty(vnf->entry[i].name)) { UCsprintf(familyBuf,UCsizeof(familyBuf),UClit("<name val=\"%s\"/>"),vnf->entry[i].name) ; } else { ZUS(familyBuf) ; } ;
	   ZUS(attrBuf) ;
	   if (FONT(Bold)) UCstrcat(attrBuf,UClit("<b/>")) ;
	   if (FONT(Italic)) UCstrcat(attrBuf,UClit("<i/>")) ;
	   if (FONT(Underline) || FONT(URL)) UCstrcat(attrBuf,UClit("<u/>")) ;
	   if (FONT(StrikeThru)) UCstrcat(attrBuf,UClit("<strike/>")) ;
	   UCsprintf(nbuf,UCsizeof(nbuf),UClit("<font>%s%s%s%s</font>"),szBuf,colorBuf,familyBuf,attrBuf) ;
	   XMLSAppend(&xmls,nbuf) ;
	 } ; XMLSAppend(&xmls,UClit("</fonts>")) ;
#undef FONT
/*	Now the fills */
	vfl = vrhm->xlsx->vfl ;
	UCsprintf(tbuf,UCsizeof(tbuf),UClit("<fills count=\"%d\">"),vfl->num) ; XMLSAppend(&xmls,tbuf) ;
	for(i=0;i<vfl->num;i++)
	 { UCCHAR fbuf[512],fgBuf[128],bgBuf[128] ;
/*	   Apparently background colors become <fill> 'fgColor' ???????????????????????????? VEH100709 */
	   if (vfl->entry[i].fgColor != 0)
	    { UCsprintf(fgBuf,UCsizeof(fgBuf),UClit("<fgColor rgb=\"%x\"/>"),v4rx_RGBColor(vrhm,vfl->entry[i].fgColor)) ; } else { ZUS(fgBuf) ; } ;
	   if (vfl->entry[i].bgColor != 0)
	    { UCsprintf(bgBuf,UCsizeof(bgBuf),UClit("<fgColor rgb=\"%x\"/>"),v4rx_RGBColor(vrhm,vfl->entry[i].bgColor)) ; } else { ZUS(bgBuf) ; } ;
	   if (UCempty(fgBuf) && UCempty(bgBuf))
	    { UCsprintf(fbuf,UCsizeof(fbuf),UClit("<fill><patternFill patternType=\"%s\"/></fill>"),v4rx_Fills[vfl->entry[i].patternType]) ;
	    } else
	    { UCsprintf(fbuf,UCsizeof(fbuf),UClit("<fill><patternFill patternType=\"%s\">%s%s</patternFill></fill>"),v4rx_Fills[vfl->entry[i].patternType],fgBuf,bgBuf) ;
	    } ;
	   XMLSAppend(&xmls,fbuf) ;
	 } ; XMLSAppend(&xmls,UClit("</fills>")) ;
/*	Any borders */
	vbor = vrhm->xlsx->vbor ;
	UCsprintf(tbuf,UCsizeof(tbuf),UClit("<borders count=\"%d\">"),vbor->num) ; XMLSAppend(&xmls,tbuf) ;
	for(i=0;i<vbor->num;i++)
	 { UCCHAR bbuf[512] ;
	   if (vbor->entry[i].borderType == V4XLSX_borderType_none)
	    { XMLSAppend(&xmls,UClit("<border><left/><right/><top/><bottom/><diagonal/></border>")) ;
	      continue ;
	    } ;
	   XMLSAppend(&xmls,UClit("<border>")) ;
#define DOBORDER(SIDE) \
 if (vbor->entry[i].SIDE != V4XLSX_borderType_none) \
 { UCCHAR bbuf[128] ; UCsprintf(bbuf,UCsizeof(bbuf),UClit("<") UClit(#SIDE) UClit(" style=\"%s\"><color indexed=\"%d\"/></") UClit(#SIDE) UClit(">"),v4rx_Borders[vbor->entry[i].borderType],vbor->entry[i].SIDE) ; \
   XMLSAppend(&xmls,bbuf) ; \
 } ;
	   DOBORDER(left) ; DOBORDER(right) ; DOBORDER(top) ; DOBORDER(bottom) ; 
	   XMLSAppend(&xmls,UClit("</border>")) ;
	   XMLSAppend(&xmls,bbuf) ;
	 } ; XMLSAppend(&xmls,UClit("</borders>")) ;

/*	Put it all together with cellStyleXfs & cellXfs */
	UCsprintf(tbuf,UCsizeof(tbuf),UClit("<cellStyleXfs count=\"%d\">"),vrhm->xlsx->vcsxfs->num) ; XMLSAppend(&xmls,tbuf) ;
	v4rx_Generate_Xf_Entries(&xmls,vrhm->xlsx->vcsxfs,TRUE) ;
	XMLSAppend(&xmls,UClit("</cellStyleXfs>")) ;

	UCsprintf(tbuf,UCsizeof(tbuf),UClit("<cellXfs count=\"%d\">"),vrhm->xlsx->vcxfs->num) ; XMLSAppend(&xmls,tbuf) ;
	v4rx_Generate_Xf_Entries(&xmls,vrhm->xlsx->vcxfs,FALSE) ;
	XMLSAppend(&xmls,UClit("</cellXfs>")) ;
	
/*	Any cell styles ? */
	if (vrhm->xlsx->vstyle->num > 0)
	 { struct V4XLSX__4MAT_CellStyles *vstyle = vrhm->xlsx->vstyle ; UCCHAR sbuf[512] ; INDEX i ;
	   UCsprintf(sbuf,UCsizeof(sbuf),UClit("<cellStyles count=\"%d\">"),vstyle->num) ; XMLSAppend(&xmls,sbuf) ;
	   for(i=0;i<vstyle->num;i++)
	    { UCsprintf(sbuf,UCsizeof(sbuf),UClit("<cellStyle name=\"%s\" xfId=\"%d\" builtinId=\"%d\"/>"),vstyle->entry[i].name,vstyle->entry[i].xfId,vstyle->entry[i].builtInId) ;
	      XMLSAppend(&xmls,sbuf) ;
	    } ;
	   XMLSAppend(&xmls,UClit("</cellStyles>")) ;
	 } ;
/*	Any custom colors reference ? */
/*	VEH120723 - Apparently Excel doesn't like (or need) this? Take it out */
	if (vrhm->xlsx->mruCount > 0 && FALSE)
	 { INDEX i ;
	   XMLSAppend(&xmls,UClit("<colors><mruColors>")) ;
	   for(i=0;i<vrhm->xlsx->mruCount;i++)
	    { UCCHAR cbuf[64] ;
	      UCsprintf(cbuf,UCsizeof(cbuf),UClit("<color rgb=\"%x\"/>"),vrhm->xlsx->mruList[i]) ;
	      XMLSAppend(&xmls,cbuf) ;
	    } ;
	   XMLSAppend(&xmls,UClit("</mruColors></colors>")) ;
	 } ;
	XMLSAppend(&xmls,xmlEnd) ;
	return(XMLSContents(&xmls)) ;
#undef xmlStart
#undef xmlEnd
}

/*	C O N T E N T   T Y P E S   T A B L E	*/
#define CTYP_MAX_ENTRIES 32	/* Max number of content entries */
struct V4XLSX__ContentTypes {
  COUNTER numEntries ;		/* Number of entries */
  struct {
     UCCHAR extName[16] ;	/* Extension name (empty if not an extension) */
     UCCHAR partName[128] ;	/* Part name (empty if not a part) */
     UCCHAR type[128] ;		/* Content-Type */
   } entry[CTYP_MAX_ENTRIES] ;
} ;

/*	S H A R E D   S T R I N G   T A B L E	*/

#define SSH_INITIAL_TABLE 750019
#define SSH_SIZE_IN_BYTES(SSH) (((char *)&(SSH)->entry[0] - (char *)(SSH)) + sizeof((SSH)->entry[0])*(SSH)->maxEntries)
struct V4XLSX__ssHash {
  INDEX maxEntries ;		/* Max number of slots below */
  INDEX numEntries ;		/* Number of entries currently in table */
  INDEX indexFirst ;		/* Index (into below) for first entry in this table */
  INDEX indexLast ;		/* Index (below) to last entry added */
  LENMAX totalChars ;		/* Count of total characters saved (to make allocation for XML easier later on) */
  struct {
    UCCHAR *str ;		/* String value of entry */
    INDEX indexNext ;		/* Index to next string */
    INDEX nth ;			/* This is the nth string inserted into table */
   } entry[SSH_INITIAL_TABLE] ;
} ;

struct V4XLSX__SheetXMLRels {
  struct V4XLSX__SheetXMLRels *priorSXR ;	/* Pointer to prior SXR */
  INDEX relId ;					/* This entry's Id */
  LOGICAL isExternal ;
  UCCHAR type[256] ;
  UCCHAR target[256] ;
} ;

INDEX v4rx_InsertXSR(vrhm,sheetId,type,target,isExternal)
  struct V4RH__Main *vrhm ;
  INDEX sheetId ;
  UCCHAR *type ;
  UCCHAR *target ;
  LOGICAL isExternal ;
{
  struct V4XLSX__SheetXMLRels *sxr ;

	sxr = v4mm_AllocChunk(sizeof *sxr,TRUE) ;
	UCstrcpy(sxr->type,type) ; UCstrcpy(sxr->target,target) ; sxr->isExternal = isExternal ;
	sxr->relId = (vrhm->xlsx->sheet[sheetId-1].lastSXR == NULL ? 1 : vrhm->xlsx->sheet[sheetId-1].lastSXR->relId+1) ;
	sxr->priorSXR = vrhm->xlsx->sheet[sheetId-1].lastSXR ;
	vrhm->xlsx->sheet[sheetId-1].lastSXR = sxr ;
	return(sxr->relId) ;
}

UCCHAR *v4rx_GenerateXSR(vrhm,sheetId)
  struct V4RH__Main *vrhm ;
  INDEX sheetId ;
{ UCCHAR *xml ;
  struct V4XLSX__SheetXMLRels *sxr ;

#define xmlStart UClit("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">")
#define xmlEnd UClit("</Relationships>")
	sxr = vrhm->xlsx->sheet[sheetId-1].lastSXR ;
	xml = v4mm_AllocUC(UCstrlen(xmlStart)+UCstrlen(xmlEnd)+512*(sxr == NULL ? 0 : sxr->relId+1)) ;
	UCstrcpy(xml,xmlStart) ;
	for(;sxr!=NULL;sxr=sxr->priorSXR)
	 { UCCHAR buf[512], mode[64] ;
	   UCstrcpy(mode,(sxr->isExternal ? UClit("TargetMode=\"External\"") : UClit(""))) ;
	   UCsprintf(buf,UCsizeof(buf),UClit("<Relationship Id=\"rId%d\" Type=\"%s\" Target=\"%s\" %s/>"),sxr->relId,sxr->type,sxr->target,mode) ;
	   UCstrcat(xml,buf) ;
	 } ;
	UCstrcat(xml,xmlEnd) ;
	return(xml) ;
#undef xmlStart
#undef xmlEnd
}

void v4xlsx_Initialize(vrhm)
  struct V4RH__Main *vrhm ;
{
	if (vrhm->xlsx != NULL) return ;	/* Already done ? */
	vrhm->xlsx = (struct V4XLSX__Main *)v4mm_AllocChunk(sizeof *vrhm->xlsx,TRUE) ;
	vrhm->xlsx->ctyp = (struct V4XLSX__ContentTypes *)v4mm_AllocChunk(sizeof *vrhm->xlsx->ctyp,TRUE) ;
	vrhm->xlsx->ssh = (struct V4XLSX__ssHash *)v4mm_AllocChunk(sizeof *vrhm->xlsx->ssh,TRUE) ;
/*	Shared strings */
	vrhm->xlsx->ssh->maxEntries = SSH_INITIAL_TABLE ; vrhm->xlsx->ssh->indexFirst = UNUSED ; vrhm->xlsx->ssh->indexLast = UNUSED ;
/*	Add content types we know we are going to need */
//#define V4XLSX_Include_PrinterSettings
#ifdef V4XLSX_Include_PrinterSettings
	v4rx_AddContentType(vrhm,UClit("bin"),NULL,UClit("application/vnd.openxmlformats-officedocument.spreadsheetml.printerSettings")) ;
#endif
	v4rx_AddContentType(vrhm,NULL,UClit("/xl/theme/theme1.xml"),UClit("application/vnd.openxmlformats-officedocument.theme+xml")) ;
	v4rx_AddContentType(vrhm,NULL,UClit("/xl/styles.xml"),UClit("application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml")) ;
	v4rx_AddContentType(vrhm,UClit("rels"),NULL,UClit("application/vnd.openxmlformats-package.relationships+xml")) ;
	v4rx_AddContentType(vrhm,UClit("xml"),NULL,UClit("application/xml")) ;
	v4rx_AddContentType(vrhm,NULL,UClit("/xl/workbook.xml"),UClit("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml")) ;
	v4rx_AddContentType(vrhm,NULL,UClit("/docProps/app.xml"),UClit("application/vnd.openxmlformats-officedocument.extended-properties+xml")) ;
#ifdef V4XLSX_Include_CalcChain
	v4rx_AddContentType(vrhm,NULL,UClit("/xl/calcChain.xml"),UClit("application/vnd.openxmlformats-officedocument.spreadsheetml.calcChain+xml")) ;
#endif
	v4rx_AddContentType(vrhm,NULL,UClit("/xl/sharedStrings.xml"),UClit("application/vnd.openxmlformats-officedocument.spreadsheetml.sharedStrings+xml")) ;
	v4rx_AddContentType(vrhm,NULL,UClit("/docProps/core.xml"),UClit("application/vnd.openxmlformats-package.core-properties+xml")) ;


	 vrhm->xlsx->vnnf = (struct V4XLSX__4MAT_numFmts *)v4mm_AllocChunk(sizeof *vrhm->xlsx->vnnf,FALSE) ; vrhm->xlsx->vnnf->num = 0 ;
/*	   Init with standard defaults */
	   v4rx_GetIndex_numFmts(vrhm,UClit("&quot;$&quot;#,##0.00")) ;
	   v4rx_GetIndex_numFmts(vrhm,UClit("[$-F800]dddd\\,\\ mmmm\\ dd\\,\\ yyyy")) ;
	vrhm->xlsx->vnf = (struct V4XLSX__4MAT_fonts *)v4mm_AllocChunk(sizeof *vrhm->xlsx->vnf,FALSE) ; vrhm->xlsx->vnf->num = 0 ;
	    v4rx_GetIndex_fonts(vrhm,11,0,UClit("Calibri"),0) ;
	vrhm->xlsx->vfl = (struct V4XLSX__4MAT_fills *)v4mm_AllocChunk(sizeof *vrhm->xlsx->vfl,FALSE) ; vrhm->xlsx->vfl->num = 0 ;
/*	   Init with standard entries */
	   v4rx_GetIndex_fills(vrhm,V4XLSX_patternType_none,0,0) ;
	   v4rx_GetIndex_fills(vrhm,V4XLSX_patternType_solid,0,0) ;
	   v4rx_GetIndex_fills(vrhm,V4XLSX_patternType_gray125,0,0) ;
	vrhm->xlsx->vbor = (struct V4XLSX__4MAT_borders *)v4mm_AllocChunk(sizeof *vrhm->xlsx->vbor,FALSE) ; vrhm->xlsx->vbor->num = 0 ;
	vrhm->xlsx->vstyle = (struct V4XLSX__4MAT_CellStyles *)v4mm_AllocChunk(sizeof *vrhm->xlsx->vstyle,FALSE) ; vrhm->xlsx->vstyle->num = 0 ;
	vrhm->xlsx->vcxfs = (struct V4XLSX__4MAT_cellXfs *)v4mm_AllocChunk(sizeof *vrhm->xlsx->vcxfs,FALSE) ; vrhm->xlsx->vcxfs->num = 0 ;
	   memset(&vrhm->xlsx->vcxfs->entry[0],0,sizeof vrhm->xlsx->vcxfs->entry[0]) ; vrhm->xlsx->vcxfs->num++ ;
	vrhm->xlsx->vcsxfs = (struct V4XLSX__4MAT_cellXfs *)v4mm_AllocChunk(sizeof *vrhm->xlsx->vcsxfs,FALSE) ; vrhm->xlsx->vcsxfs->num = 0 ;
	   memset(&vrhm->xlsx->vcsxfs->entry[0],0,sizeof vrhm->xlsx->vcsxfs->entry[0]) ; vrhm->xlsx->vcsxfs->num++ ;


}

/*	C O N T E N T   T Y P E S   T A B L E	*/

INDEX v4rx_AddContentType(vrhm,ext,part,type)
  struct V4RH__Main *vrhm ;
  UCCHAR *ext, *part, *type ;
{
  struct V4XLSX__ContentTypes *ctyp ;
  INDEX cx ;

	ctyp = vrhm->xlsx->ctyp ;
/*	See if we already have it, otherwise append it */
	for(cx=0;cx<ctyp->numEntries;cx++)
	 { if ((ext == NULL ? FALSE : UCstrcmp(ext,ctyp->entry[cx].extName) == 0) || (part == NULL ? FALSE : UCstrcmp(part,ctyp->entry[cx].partName) == 0))
	    return(cx) ;
	 } ;
	if (ctyp->numEntries >= CTYP_MAX_ENTRIES) return(UNUSED) ;
	if (ext != NULL) UCstrcpy(ctyp->entry[ctyp->numEntries].extName,ext) ;
	if (part != NULL) UCstrcpy(ctyp->entry[ctyp->numEntries].partName,part) ;
	UCstrcpy(ctyp->entry[ctyp->numEntries].type,type) ;
	return(ctyp->numEntries++) ;
}

UCCHAR *v4rx_GenerateContentType(vrhm)
  struct V4RH__Main *vrhm ;
{ struct V4XLSX__ContentTypes *ctyp ;
  UCCHAR *xml ; INDEX cx ;
  
	ctyp = vrhm->xlsx->ctyp ;
	xml = v4mm_AllocUC(512+256*ctyp->numEntries) ;
#define CTYP_HEAD UClit("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">")
#define CTYP_TAIL UClit("</Types>")
	UCstrcpy(xml,CTYP_HEAD) ;
	for(cx=0;cx<ctyp->numEntries;cx++)
	 { UCCHAR buf[512] ;
	   if (UCnotempty(ctyp->entry[cx].extName))
	    { UCsprintf(buf,UCsizeof(buf),UClit("<Default Extension=\"%s\" ContentType=\"%s\"/>"),ctyp->entry[cx].extName,ctyp->entry[cx].type) ;
	    } else if (UCnotempty(ctyp->entry[cx].partName))
	    { UCsprintf(buf,UCsizeof(buf),UClit("<Override PartName=\"%s\" ContentType=\"%s\"/>"),ctyp->entry[cx].partName,ctyp->entry[cx].type) ;
	    } ;
	   UCstrcat(xml,buf) ;
	 } ;
	UCstrcat(xml,CTYP_TAIL) ;
	return(xml) ;
}
/*	T O P   L E V E L   .R E L S	*/

UCCHAR *v4rx_GenerateRelsDotRels(vrhm)
  struct V4RH__Main *vrhm ;
{ UCCHAR *xml ;
#define rels UClit( \
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?> \
<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"> \
	<Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties\" Target=\"docProps/app.xml\"/> \
	<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" Target=\"docProps/core.xml\"/> \
	<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/> \
</Relationships>")
/*	Allocate xml just to be consistent with all the other v4rx_Generatexxx routines */
	xml = v4mm_AllocUC(UCstrlen(rels)) ; UCstrcpy(xml,rels) ;
	return(xml) ;
}

/*	A P P . X M L	*/

UCCHAR *v4rx_GenerateApp(vrhm)
  struct V4RH__Main *vrhm ;
{ INDEX sx ;
  UCCHAR aeBuf[4096],mgrName[512],cmpName[512] ;
  struct V4RH__Sheet *sheet ;
  struct V4XLSX__XMLS xmlsData ;
  
#define appStart UClit( \
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?> \
<Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\" xmlns:vt=\"http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes\"> \
	<Application>Microsoft Excel</Application> \
	<DocSecurity>0</DocSecurity> \
	<ScaleCrop>false</ScaleCrop> \
	<HeadingPairs> \
		<vt:vector size=\"2\" baseType=\"variant\"> \
			<vt:variant> \
				<vt:lpstr>Worksheets</vt:lpstr> \
			</vt:variant> \
			<vt:variant> \
				<vt:i4>%d</vt:i4> \
			</vt:variant> \
		</vt:vector> \
	</HeadingPairs> \
	<TitlesOfParts> \
		<vt:vector size=\"%d\" baseType=\"lpstr\">")

#define appEndMask UClit( \
"		</vt:vector> \
	</TitlesOfParts> \
	%s%s\
	<LinksUpToDate>false</LinksUpToDate> \
	<SharedDoc>false</SharedDoc> \
	<HyperlinksChanged>false</HyperlinksChanged> \
	<AppVersion>12.0000</AppVersion> \
</Properties>")

	XMLSInit(&xmlsData) ;
	{ COUNTER size = UCstrlen(appStart)+128 ; UCCHAR *xml = v4mm_AllocUC(size) ;
	  UCsprintf(xml,size,appStart,vrhm->sheetNum,vrhm->sheetNum) ;
	  XMLSAppend(&xmlsData,xml) ;
	  v4mm_FreeChunk(xml) ;
	}
	for (sx=1,sheet=vrhm->sheetFirst;sheet!=NULL;sheet=sheet->sheetNext,sx++)
	 { UCCHAR xmlS[128] ;
	   UCsprintf(xmlS,UCsizeof(xmlS),UClit("<vt:lpstr>%s</vt:lpstr>"),sheet->sheetName) ;
	   XMLSAppend(&xmlsData,xmlS) ;
	 } ;
#define PROP(FLD,TAG) if(vrhm->xlsx->FLD == NULL) { ZUS(FLD) ; } else { UCsprintf(FLD,UCsizeof(FLD),UClit("<") UClit(#TAG) UClit(">%s</") UClit(#TAG) UClit(">"),vrhm->xlsx->FLD) ; }
	PROP(mgrName,Manager) PROP(cmpName,Company)
	UCsprintf(aeBuf,UCsizeof(aeBuf),appEndMask,mgrName,cmpName) ;
	XMLSAppend(&xmlsData,aeBuf) ;
	return(XMLSContents(&xmlsData)) ;
}

/*	C O R E . X M L		*/

UCCHAR *v4rx_GenerateCore(vrhm)
  struct V4RH__Main *vrhm ;
{ UCCHAR dtbuf[128],buf[8192] ;
  CALENDAR cal ;
  struct V4XLSX__XMLS xmls ;
  struct V4RH__NamedEntry *vrne ;

#define coreMaskHead UClit( \
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?> \
<cp:coreProperties xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:dcterms=\"http://purl.org/dc/terms/\" xmlns:dcmitype=\"http://purl.org/dc/dcmitype/\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"> \
	<dc:creator>%s / v4rpp %d.%d</dc:creator> \
	<cp:lastModifiedBy>v4rpp</cp:lastModifiedBy> \
	<dcterms:created xsi:type=\"dcterms:W3CDTF\">%s</dcterms:created> \
	<dcterms:modified xsi:type=\"dcterms:W3CDTF\">%s</dcterms:modified>")
#define coreTail UClit("</cp:coreProperties>")

	XMLSInit(&xmls) ;
	
	setCALisNOW(cal) ;
	v_FormatDate(&cal,V4DPI_PntType_Calendar,VCAL_CalType_UseDeflt,UNUSED,UClit("yyyy-0m-0dT0h:0n:0s"),dtbuf) ; UCstrcat(dtbuf,UClit("Z")) ;
	vrne =  v4rh_neLookup(vrhm,UClit("userName")) ;
	UCsprintf(buf,UCsizeof(buf),coreMaskHead,(vrne==NULL ? UClit("") : vrne->eVal),V4RPP_Version_Major,V4RPP_Version_Minor,dtbuf,dtbuf) ; XMLSAppend(&xmls,buf) ;
	vrne =  v4rh_neLookup(vrhm,UClit("reportName")) ;
	UCsprintf(buf,UCsizeof(buf),UClit("<cp:keywords>func:%s "),(vrne == NULL ? UClit("xxx") : vrne->eVal)) ;
	vrne =  v4rh_neLookup(vrhm,UClit("setup")) ;
	if (vrne != NULL) { UCCHAR fusBuf[8192] ; v4rx_FixUpStr(fusBuf,vrne->eVal,FALSE) ; UCstrcat(buf,fusBuf) ; } ;
	UCstrcat(buf,UClit("</cp:keywords>")) ; XMLSAppend(&xmls,buf) ;
	vrne =  v4rh_neLookup(vrhm,UClit("memoText")) ;
	if (vrne != NULL)
	 { UCCHAR fusBuf[8192] ; v4rx_FixUpStr(fusBuf,vrne->eVal,FALSE) ; UCsprintf(buf,UCsizeof(buf),UClit("<dc:description>%s</dc:description>"),fusBuf) ; XMLSAppend(&xmls,buf) ; } ;
	XMLSAppend(&xmls,coreTail) ;	
	return(XMLSContents(&xmls)) ;
}

/*	S H A R E D   S T R I N G   T A B L E	*/

/*	v4rx_sshInsert - Returns index into sharedString table of argument (appends to end if new) */
INDEX v4rx_sshInsert(vrhm,str)
  struct V4RH__Main *vrhm ;
  UCCHAR *str ;
{ INDEX shx,shxFirst ; LENMAX len ; UCCHAR fixed[0x10000] ;
  struct V4XLSX__ssHash *ssh ;

	ssh = vrhm->xlsx->ssh ;
/*	Maybe fix up the string (make it XML OK) */
	v4rx_FixUpStr(fixed,str,FALSE) ;
/*	Hash the source string and see if in the table */
	len = UCstrlen(fixed) ;
	VHASH32_FWD(shx,fixed,len) ; shxFirst = (shx = (0x7fffffff & shx) % ssh->maxEntries) ;
	for(;ssh->entry[shx].str!=NULL;)
	 { if (UCstrcmp(ssh->entry[shx].str,fixed) == 0)
	    return(ssh->entry[shx].nth) ;
	   shx++ ; if (shx >= ssh->maxEntries) shx = 0 ;
	 } ;
/*	String not in table, have to insert, if table approaching max then increase it */
	if (ssh->numEntries > (ssh->maxEntries * 8) / 10)
	 { INDEX oldMax = ssh->maxEntries ; ssh->maxEntries = v_CvtToPrime(ssh->maxEntries * 1.5) ;
	   vrhm->xlsx->ssh = (ssh = (struct V4XLSX__ssHash *)realloc(ssh,SSH_SIZE_IN_BYTES(ssh))) ;
	   for(;oldMax<ssh->maxEntries;oldMax++)		/* Clear out newly allocated space */
	    { memset(&ssh->entry[oldMax],0,sizeof ssh->entry[oldMax]) ; } ;
/*	   Yes, since maxEntries has changed, the hashing is all different. We may end up with duplicates but who cares? */
	 } ;
	for(shx=shxFirst;;shx++)
	 { if (shx >= ssh->maxEntries) shx = 0 ;
	   if (ssh->entry[shx].str == NULL) break ;
	 } ;
/*	shx = correct slot, insert the string */
	ssh->entry[shx].str = v4mm_AllocUC(len) ;
	UCstrcpy(ssh->entry[shx].str,fixed) ;
	if (ssh->indexLast == UNUSED)
	 { ssh->indexFirst = (ssh->indexLast = shx) ;		/* First entry */
	 } else
	 { ssh->entry[ssh->indexLast].indexNext = shx ;		/* Link last entry to this one */
	   ssh->indexLast = shx ;
	 } ;
	ssh->totalChars += len ;
	ssh->entry[shx].nth = ssh->numEntries ;
	return(ssh->numEntries++) ;				/* Index is 0 based */
}

/*	v4rx_GenerateSSH - Generates sharedStrings.xml buffer */
UCCHAR *v4rx_GenerateSSH(vrhm)
  struct V4RH__Main *vrhm ;
{
  struct V4XLSX__ssHash *ssh ;
  UCCHAR *xml,*eos ;
  INDEX shx ; COUNTER num ;

	ssh = vrhm->xlsx->ssh ;
#define sshXMLHeader UClit("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><sst xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" count=\"%d\" uniqueCount=\"%d\">")
/*	Allocate xml buffer (based on number of entries & totals characters */
	xml = v4mm_AllocUC(UCstrlen(sshXMLHeader)+16*ssh->numEntries+ssh->totalChars+1024) ;
	UCsprintf(xml,1000,sshXMLHeader,ssh->numEntries,ssh->numEntries) ;
	eos = UCstrchr(xml,'\0') ;		/* Points to first free */
	for(num=1,shx=ssh->indexFirst;num<=ssh->numEntries;shx=ssh->entry[shx].indexNext,num++)
	 { UCstrcat(eos,UClit("<si><t>")) ; UCstrcat(eos,ssh->entry[shx].str) ; UCstrcat(eos,UClit("</t></si>")) ;
	   eos = UCstrchr(eos,'\0') ;
	 } ;
	UCstrcat(eos,UClit("</sst>")) ;
	return(xml) ;
}

/*	W O R K B O O K . X M L		*/

/*	v4rx_wbXMLGenerate - Generates workbook.xml buffer */
UCCHAR *v4rx_GenerateWB(vrhm)
  struct V4RH__Main *vrhm ;
{ struct V4RH__Sheet *sheet ;
  INDEX sx ; LOGICAL haveHdr ;
  struct V4XLSX__XMLS xmlsData ;
#define xmlStart UClit(\
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?> \
<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"> \
	<fileVersion appName=\"xl\" lastEdited=\"4\" lowestEdited=\"4\" rupBuild=\"4506\"/> \
	<workbookPr defaultThemeVersion=\"124226\"/> \
	<bookViews> \
		<workbookView xWindow=\"480\" yWindow=\"75\" windowWidth=\"29715\" windowHeight=\"19215\"/> \
	</bookViews>")

#define xmlEnd UClit("<calcPr calcId=\"125725\"/></workbook>")

	XMLSInit(&xmlsData) ;
	XMLSAppend(&xmlsData,xmlStart) ;
	haveHdr = FALSE ;
	XMLSAppend(&xmlsData,UClit("<sheets>")) ;
	for (sx=1,sheet=vrhm->sheetFirst;sheet!=NULL;sheet=sheet->sheetNext,sx++)
	 { UCCHAR xmlS[512] ;
	   UCsprintf(xmlS,UCsizeof(xmlS),UClit("<sheet name=\"%s\" sheetId=\"%d\" r:id=\"rId%d\"/>"),sheet->sheetName,sx,sx) ;
	   XMLSAppend(&xmlsData,xmlS) ;
	   if (sheet->hdrNum > 0) haveHdr = TRUE ;
	 } ;
	XMLSAppend(&xmlsData,UClit("</sheets>")) ;
/*	If we have column headings then set up defined name to define */
	if (haveHdr)
	 { XMLSAppend(&xmlsData,UClit("<definedNames>")) ;
	   for (sx=1,sheet=vrhm->sheetFirst;sheet!=NULL;sheet=sheet->sheetNext,sx++)
	    { UCCHAR xmlS[512] ;
	      if (sheet->hdrNum <= 0) continue ;
	      UCsprintf(xmlS,UCsizeof(xmlS),UClit("<definedName name=\"_xlnm.Print_Titles\" localSheetId=\"%d\">'%s'!$1:$%d</definedName>"),sx-1,sheet->sheetName,sheet->hdrNum) ;
	      XMLSAppend(&xmlsData,xmlS) ;
	    } ;
	   XMLSAppend(&xmlsData,UClit("</definedNames>")) ;
	 } ;

	XMLSAppend(&xmlsData,xmlEnd) ;
	return(XMLSContents(&xmlsData)) ;
#undef xmlStart
#undef xmlEnd
}

/*	W O R K B O O K . X M L . R E L S	*/

UCCHAR *v4rx_GenerateWBRels(vrhm)
  struct V4RH__Main *vrhm ;
{ struct V4RH__Sheet *sheet ;
  UCCHAR *xml ; INDEX sx ;

#define xmlStart UClit(\
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?> \
<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"> \
	<Relationship Id=\"rId507\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/calcChain\" Target=\"calcChain.xml\"/> \
	<Relationship Id=\"rId506\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/sharedStrings\" Target=\"sharedStrings.xml\"/> \
	<Relationship Id=\"rId505\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/> \
	<Relationship Id=\"rId504\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme\" Target=\"theme/theme1.xml\"/>")
#define xmlEnd UClit("</Relationships>")

	xml = v4mm_AllocUC(UCstrlen(xmlStart)+UCstrlen(xmlEnd)+vrhm->sheetNum*256) ;
	UCstrcpy(xml,xmlStart) ;
	for (sx=1,sheet=vrhm->sheetFirst;sheet!=NULL;sheet=sheet->sheetNext,sx++)
	 { UCCHAR xmlS[512] ;
	   UCsprintf(xmlS,UCsizeof(xmlS),UClit("<Relationship Id=\"rId%d\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet%d.xml\"/>"),sx,sx) ;
	   UCstrcat(xml,xmlS) ;
	 } ;
	UCstrcat(xml,xmlEnd) ;
	return(xml) ;
#undef xmlStart
#undef xmlEnd
}


/*	v4rx_ExcelRowColSpec - converts row & column into single Excel address string (ex: [0,0] -> 'A1') */
UCCHAR *v4rx_ExcelRowColSpec(rowNum,colNum)
 INDEX rowNum,colNum ;
{
  static UCCHAR rcsBufs[5][32] ; static INDEX rcsX=0 ;
  UCCHAR *rcs,stack[10] ; INDEX i,d ;
	
	rcs = rcsBufs[rcsX] ; rcsX = ((rcsX+1) % 5) ;
	for(i=0;;i++)
	 { stack[i] = UClit("ABCDEFGHIJKLMNOPQRSTUVWXYZ")[colNum % 26] ;
	   colNum /= 26 ; if (colNum <= 0) break ;
	   colNum-- ;
	 } ;
	for(d=0;i>=0;i--) { rcs[d++] = stack[i] ; } ;
	for(rowNum++,i=0;;i++)
	 { stack[i] = UClit("0123456789")[rowNum % 10] ;
	   rowNum /= 10 ; if (rowNum <= 0) break ;
	 } ;
	for(;i>=0;i--) { rcs[d++] = stack[i] ; } ;
	rcs[d++] = UCEOS ;
	return(rcs) ;
}

/*	v4rx_ConvertR1C1toA1 - scans formula & converts any R1C1 references to A1 format */
UCCHAR *v4rx_ConvertR1C1toA1(srcStr,rowNum,colNum)
  UCCHAR *srcStr ;
  INDEX rowNum,colNum ;
{
  static UCCHAR dstBuf[512] ;
  UCCHAR *s,*d,*tc,*a1 ; INDEX rr, rc ;

	s = srcStr ; d = dstBuf ;
	for(;*s!=UCEOS;)
	 {
/*	   Have to scan for potential references */
	   if (*s != UClit('R')) { *(d++) = *(s++) ; continue ; } ;
/*	   Got one if next character is 'C' or '[' */
	   switch (*(s+1))
	    { default:		s++ ; continue ;		/* Does not appear to be R1C1 reference */
	      case UClit('C'):
		s++ ; rr = 0 ; break ;
	      case UClit('['):
		rr = v_ParseInt(s+2,&tc,10) ;			/* Parse relative number after '[' */
		if (*tc != UClit(']')) return(NULL) ;		/* Did not end with ']' - error */
		s = tc + 1 ; break ;
	    } ;
/*	   Now parse the 'C' portion */
	   if (*s != UClit('C')) return(NULL) ;
	   s++ ;
	   if (*s != UClit('['))
	    { if (vuc_IsContId(*s)) return(NULL) ; rc = 0 ;
	    } else
	    { rc = v_ParseInt(s+1,&tc,10) ;			/* Parse relative number after '[' */
	      if (*tc != UClit(']')) return(NULL) ;		/* Did not end with ']' - error */
	      s = tc + 1 ;
	    } ;
/*	   Have rr & rc - convert to actual A1 reference based on rowNum & colNum */
	   a1 = v4rx_ExcelRowColSpec(rowNum+rr,colNum+rc) ;
	   for(;*a1!=UCEOS;) { *(d++) = *(a1++) ; } ;
	 } ;
/*	All done - end result with UCEOS & return it */
	*(d++) = UCEOS ;
	return(dstBuf) ;
}

COUNTER v4rx_CreateCellNEW(vrhm,xml,sheetX,rowNum,colNum,row,cell,col,height)
  struct V4RH__Main *vrhm ;
  UCCHAR *xml ;
  INDEX sheetX ;
  INDEX rowNum,colNum ;
  struct V4RH__Row *row ;
  struct V4RH__Cell *cell ;
  struct V4RH__Column *col ;
  COUNTER *height ;
{
  struct V4RH__DimInfo *vdi ;
  struct V4RH__FormatInfo *vfi,lvfi ;
  UCCHAR *tc,*cp,*ep,lHdr[3],*arg, *urlInfo, *imgPath ; CALENDAR cal ; int inum,i ; double dbl,maxdnum ; LOGICAL logical,wrapLines ; LENMAX colSpan,valLen ;
  UCCHAR tbuf[512],sEq[32],tEq[32],urlCat[1024] ; INDEX xfId ; INDEX imgRows,imgCols ;
  static INDEX idnCount = 0, invCount = 0 ;

	if (cell->cellValue == NULL)
	 cell->cellValue = UClit("") ;
	vdi = (cell->dimNum >= 0 && cell->dimNum < V4RH_DIM_MAX ? vrhm->vdi[cell->dimNum] : NULL) ;
/*	If vdi undefined then default with generic alpha */
	if (vdi == NULL)
	 { if (++idnCount < 10) printf("[Invalid dimension number (%d) encountered in cell, defaulting to Alpha]\n",cell->dimNum) ;
	   vdi = vrhm->vdi[cell->dimNum=V4RH_DimNum_Alpha] ;
	 } ;
	vfi = vdi->vfi ; urlInfo = NULL ; imgPath = NULL ; imgRows = 1 ; imgCols = 1 ;
/*	Parse (abbreviated) cell attributes */
	for(cp=cell->cellInfo,ep=cp;ep!=NULL;cp=ep+1)
	 { ep = UCstrchr(cp,EOLbt) ;
	   if (UCempty(cp)) continue ;
	   if (ep != NULL) *ep = UCEOS ;			/* Set terminating char to EOS - HAVE TO RESET!!! */
	   lHdr[0] = cp[0] ; lHdr[1] = cp[1] ; ; lHdr[2] = UCEOS ; arg = &cp[2] ;
	   switch (v4im_GetUCStrToEnumVal(lHdr,0))
	    { default:		continue ;
	      case _IM:
/*		We should have url<tab>localfile<tab>rows<tab>columns */
		{ UCCHAR *b = UCstrchr(arg,'\t') ; if (b != NULL) { *b = UCEOS ; } else { break ; } ;
		  arg = b+1 ; b = UCstrchr(arg,'\t') ; if (b != NULL) *b = UCEOS ;
		  imgPath = arg ;
		  if (b == NULL) break ;
		  arg = b+1 ; b = UCstrchr(arg,'\t') ; if (b != NULL) *b = UCEOS ; imgRows = v_ParseInt(arg,&tc,10) ;
		  if (b != NULL) { arg = b+1 ; imgCols = v_ParseInt(arg,&tc,10) ; } ;
		}
		break ;
	      case _No:
/*		Just keep track of comment for now */
		{ struct V4XLSX__4MAT_Comment *vcom ;
		  if (vrhm->xlsx->vcom == NULL) { vrhm->xlsx->vcom = (struct V4XLSX__4MAT_Comment *)v4mm_AllocChunk(sizeof *vcom,FALSE) ; vrhm->xlsx->vcom->num = 0 ; } ;
		  vcom = vrhm->xlsx->vcom ;
		  if (vcom->num >= V4XLSX_Comment_Max)
		   { printf("[Exceeded maximum of %d cell comments]\n",V4XLSX_Comment_Max) ; break ; } ;
		  vcom->entry[vcom->num].row = rowNum ; vcom->entry[vcom->num].col = colNum ; vcom->entry[vcom->num].sheetX = sheetX ;
		  vcom->entry[vcom->num].str = v4mm_AllocUC(UCstrlen(arg)) ; UCstrcpy(vcom->entry[vcom->num].str,arg) ;
		  vcom->num++ ;
		}
		break ;
	      case _PB:
/*		brkRow[n] = 0 indexed row to break AFTER, if PBA then save rowNum+1 else if PBB then rowNum */
		if (vrhm->xlsx->sheet[sheetX-1].numBrks <= V4XLSX_PAGEBRK_MAX)
		 { INDEX brkRow = rowNum + (arg[0] == UClit('A') ? 1 : 0) ;
		   vrhm->xlsx->sheet[sheetX-1].brkRow[vrhm->xlsx->sheet[sheetX-1].numBrks] = brkRow ;
		   vrhm->xlsx->sheet[sheetX-1].numBrks++ ;
		 } ;
		break ;
	      case _SP:
		cell->colSpan = v_ParseInt(arg,&tc,10) ;
		lvfi = *vfi ; vfi = &lvfi ;	/* Make copy of format */
		vfi->colSpan = cell->colSpan ;
		if (vfi->horAlign == 0) vfi->horAlign = V4SS_Align_Center ;
		break ;
	      case _UR:
		{ UCCHAR *tc ; INDEX urlBX = v_ParseInt(arg,&tc,10) ;
		  if (*tc == UClit(':'))
		   { if (vrhm->urlBases[urlBX] == NULL) break ;
		     UCstrcpy(urlCat,vrhm->urlBases[urlBX]) ; UCstrcat(urlCat,tc+1) ; urlInfo = urlCat ;
		   } else { urlInfo = arg ; } ;
		}
		switch (vrhm->xlsxURL)
		 { default:
		   case V4RH_XLSXURL_none:
			urlInfo = NULL ; break ;
		   case	V4RH_XLSXURL_exOnly:
			if (UCstrstr(urlInfo,UClit("_V4=")) == NULL && UCstrstr(urlInfo,UClit("_v4=")) == NULL) break ;
			urlInfo = NULL ; break ;	/* Appears to be V4 link - clear it */
		   case V4RH_XLSXURL_userKey:
		   case	V4RH_XLSXURL_secure:
/*			We are going to use this link, it may be converted later on for secure links, but we are going to use it */
/*			If this is to V4, then get the server address (everything before the '?') and save once as spreadsheet URL base */
			{ struct V4RH__NamedEntry *vrne = v4rh_neLookup(vrhm,UClit("sessionKey")) ;
			  if (UCstrchr(urlInfo,'?') != NULL && vrhm->xlsx->urlSuffix == NULL)
			   { 
			     vrhm->xlsx->urlSuffix = v4mm_AllocUC(256) ;		/* This will hold the constant '_Area=1&sesKey=xxxxx' */
/*			     Have to include _sesKey & _Area for links to actuall work */
/*			     Have to generate a new session key if V4RH_XLSXURL_secure */
			     UCstrcpy(vrhm->xlsx->urlSuffix,UClit("&_Area=1&_sesKey=")) ;
			     if (vrhm->xlsxURL == V4RH_XLSXURL_secure || vrne == NULL)
			      { LENMAX eLen ; INDEX i,j ; UCCHAR sesKey[V4XLIB_sesKey_Length+1] ; UCCHAR fnBuf[V_FileName_Max] ;
				eLen = UCstrlen(V4XLIB_sesKey_Elements) ;
				for(i=0;i<V4XLIB_sesKey_Length;i++)
				 { j = (int)floor(vRan64_RandomDbl() * eLen) ; if (j == eLen) j-- ;
				   sesKey[i] = V4XLIB_sesKey_Elements[j] ;
				 } ; sesKey[i] = UCEOS ;
				UCstrcat(vrhm->xlsx->urlSuffix,sesKey) ;	/* Add in new session key */
/*				Link up to special xlib routine to handle all this */
				UCstrcat(vrhm->xlsx->urlSuffix,UClit("&_V4=Process&_Func=xlsxDrilldown")) ;
/*				Open new txt file to hold URL information */
				UCsprintf(fnBuf,UCsizeof(fnBuf),UClit("%sv4rlink-%s.txt"),(vrhm->dirXLSXLclURLs == NULL ? UClit("") : vrhm->dirXLSXLclURLs),sesKey) ;
				if (!v_UCFileOpen(&vrhm->xlsx->UCFile,fnBuf,UCFile_Open_Write,TRUE,vrhm->errMessage,UNUSED)) return(UNUSED) ;
/*				Init the header for this file */
				for(i=1;i<=V4XLIB_XLSX_URLHdr_Lines;i++)
				 { switch (i)
				    { case 1:		/* Expiration Date-Time */
					{ CALENDAR cal ; UCCHAR calBuf[64] ;
					  setCALisNOW(cal) ; cal += (vrhm->xlsxLifeInDays <= 0 ? 7 : vrhm->xlsxLifeInDays) ;
					  v_FormatDate(&cal,V4DPI_PntType_Calendar,VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,NULL,calBuf) ;
					  fprintf(vrhm->xlsx->UCFile.fp,"%s\n",UCretASC(calBuf)) ;
					}
					break ;
				      case 2:		/* Privileges */
					{ struct V4RH__NamedEntry *vrne = v4rh_neLookup(vrhm,UClit("privileges")) ;
					  fprintf(vrhm->xlsx->UCFile.fp,"%s\n",(vrne != NULL ? UCretASC(vrne->eVal) : "")) ;
					  break ;
					}
				      default:
					fprintf(vrhm->xlsx->UCFile.fp,"\n") ;	/* Unused - blank line for now */
					break ;
				    } ;
				 } ;
			      } else
			      { UCstrcat(vrhm->xlsx->urlSuffix,vrne->eVal) ;
			      } ;
			   } ;
			 } ;
			break ;
		 } ;
		break ;
	    } ;
	 } ;

/*	If we have a picture/image associated with the cell */
	if (imgPath != NULL)
	 { struct V4XLSX__4MAT_Drawing *vdrw ;
/*	   Is this the first image ? */
	   if (vrhm->xlsx->vdrw == NULL)
	    { vrhm->xlsx->vdrw = (struct V4XLSX__4MAT_Drawing *)v4mm_AllocChunk(sizeof *vrhm->xlsx->vdrw,FALSE) ; vrhm->xlsx->vdrw->numEntries = 0 ; vrhm->xlsx->vdrw->numImgs = 0 ;
	    } ;
	   vdrw = vrhm->xlsx->vdrw ;
/*	   Have we already hit this image ? */
	   for(i=0;i<vdrw->numImgs;i++)
	    { if (UCstrcmp(imgPath,vdrw->img[i].path)== 0) break ; } ;
	   if (i >= vdrw->numImgs)
	    { UCCHAR xmlPath[128] ;
	      if (vdrw->numImgs >= V4XLSX_Drawings_ImgMax)
	       { v_Msg(NULL,vrhm->errMessage,"@Exceeded maximum (%1d) number of embedded images/drawings",V4XLSX_Drawings_ImgMax) ; return(UNUSED) ; } ;
	      vdrw->img[vdrw->numImgs].path = v4mm_AllocUC(UCstrlen(imgPath)) ; UCstrcpy(vdrw->img[vdrw->numImgs].path,imgPath) ;
/*	      Add resource linkage */
	      UCsprintf(xmlPath,UCsizeof(xmlPath),UClit("../drawings/drawing%d.xml"),vdrw->numImgs+1) ;
	      vdrw->img[vdrw->numImgs].rId = v4rx_InsertXSR(vrhm,sheetX,UClit("http://schemas.openxmlformats.org/officeDocument/2006/relationships/drawing"),xmlPath,FALSE) ;
	      i = vdrw->numImgs ; vdrw->numImgs++ ;
	    } ;
	   if (vdrw->numEntries >= V4XLSX_Drawings_EntryMax) { v_Msg(NULL,vrhm->errMessage,"@Exceeded maximum (%1d) number of embedded images/drawings entries",V4XLSX_Drawings_EntryMax) ; return(UNUSED) ; } ;
	   vdrw->entry[vdrw->numEntries].row = rowNum ; vdrw->entry[vdrw->numEntries].col = colNum ; vdrw->entry[vdrw->numEntries].sheetX = sheetX ;
	   vdrw->entry[vdrw->numEntries].rows = imgRows ; vdrw->entry[vdrw->numEntries].cols = imgCols ;
	   vdrw->entry[vdrw->numEntries].imgX = i ;
	   vdrw->numEntries++ ;
	 } ;

/*	Cell formatting is a little convoluted -
	  a cell has a format (0 based index into <cellXfs>...) via "s=index" attribute
	  a format (<cellXfs> <xf> may link to a <cellStyle> <xf> via 0-based xfId=index attribute
	  a style (<cellStyle><cellStyle>... just defines a name, it links to formatting via xfId attribute into <cellStyleXfs><xf>... entries
*/

/*	If URL then make sure we have defined the HyperLink style  */
/*	VEH100710 - This does not appear to do anything, have to tweak hyperlink formatting somewhere else (look for 'tweak') */
	if (urlInfo != NULL)
	 {
/*	   Set up default style for hyperlinks */
	   xfId = v4rx_GetIndex_cellStyleXfs(vrhm,NULL,11,v_ColorNameToRef(UClit("blue")),UClit("Calibri"),0,0,FALSE) ;
	   v4rx_GetIndex_cellStyle(vrhm,UClit("Hyperlink"),8,xfId) ;
	 } else
	 { xfId = 0 ;
	 } ;
/*	xfId now is link to correct style entry for this cell */


//if cell is string & has embedded newline then need <alignment wrapText="1"/> in its xfs
	wrapLines = (UCstrchr(cell->cellValue,EOLbt) != NULL) || ((vfi->fontAttr & V4SS_Font_Wrap) != 0) ;

/*	Build up the xml string for this cell */
//	xfId = 0 ;			/* Link up to default <cellStyleXfs> entry for now */
	xfId = v4rx_GetIndex_cellXfs(vrhm,vfi,(vdi->maskX != UNUSED ? vrhm->vmi->mask[vdi->maskX].formatMask : NULL),xfId,(urlInfo!=NULL),wrapLines) ;
/*	If anything other than default (0) then put link in cell to the entry */
	if (xfId > 0) { UCsprintf(sEq,UCsizeof(sEq),UClit(" s=\"%d\""),xfId) ; } else { ZUS(sEq) ; } ;


/*	If a string then set 't="s"' */
	switch (cell->pntType == UNUSED ? vdi->v4PntType : cell->pntType)
	 { default:
		if(cell->cellFormula != NULL) { ZUS(tEq) ; }
		 else { UCstrcpy(tEq,UClit(" t=\"s\"")) ; } ;
		break ;
//	   case V4DPI_PntType_MIDASUDT:
	   case V4DPI_PntType_UDate:
	   case V4DPI_PntType_UDT:
	   case V4DPI_PntType_Calendar:
	   case V4DPI_PntType_Int:
	   case V4DPI_PntType_Delta:
	   case V4DPI_PntType_Real:
		ZUS(tEq) ; break ;
	 } ;
	 
	UCsprintf(tbuf,UCsizeof(tbuf),UClit("<c r=\"%s\"%s%s>"),v4rx_ExcelRowColSpec(rowNum,colNum),sEq,tEq) ;
	UCstrcat(xml,tbuf) ;
/*	If we have a formula then append <f>....</f> */
	if (cell->cellFormula != NULL)
	 { UCCHAR *r1c1 = v4rx_ConvertR1C1toA1(cell->cellFormula,rowNum,colNum) ;
/*	   If we had any problems converting to A1 format then just drop out of this and handle regularly */
	   if (r1c1 != NULL)
	    { UCstrcat(xml,UClit("<f>")) ; UCstrcat(xml,r1c1) ; UCstrcat(xml,UClit("</f></c>")) ;
	      if (col->width < 8) col->width = 8 ;
	      return(1) ;
	    } ;
	 } ;
/*	If column spanning then save in special table for later use */
	colSpan = (cell->colSpan > 0 ? cell->colSpan : (vdi->vfi != NULL ? vdi->vfi->colSpan : 0)) ;
	if (colSpan != 0)
	 { struct V4XLSX__CellSpans *vcs ;
	   if (vrhm->xlsx->sheet[sheetX-1].vcs != NULL) { vcs = vrhm->xlsx->sheet[sheetX-1].vcs ; }
	    else { vcs = (vrhm->xlsx->sheet[sheetX-1].vcs = (struct V4XLSX__CellSpans *)v4mm_AllocChunk(sizeof(struct V4XLSX__CellSpans),FALSE)) ; vcs->num = 0 ; vcs->maxNum = V4XLSX_CellSpans_Default ; } ;
	   if (vcs->num >= vcs->maxNum)
	    { vcs->maxNum += V4XLSX_CellSpans_Default ;
	      vcs = realloc(vcs,((char *)&vcs->entry[vcs->maxNum])-(char *)vcs) ;
	      vrhm->xlsx->sheet[sheetX-1].vcs = vcs ;
//	      v_Msg(NULL,vrhm->errMessage,"@Exceeded max (%1d) spanning entries",V4XLSX_CellSpans_Max) ; return(UNUSED) ;
	    } ;
	   vcs->entry[vcs->num].row = rowNum ; vcs->entry[vcs->num].col = colNum ; vcs->entry[vcs->num].span = colSpan ;
	   vcs->num++ ;
	 } ;
/*	If a URL then save in special table for later use */
	if (urlInfo != NULL)
	 { struct V4XLSX__HyperLinks *vhl ;
	   if (vrhm->xlsx->sheet[sheetX-1].vhl != NULL) { vhl = vrhm->xlsx->sheet[sheetX-1].vhl ; }
	    else { vhl = (vrhm->xlsx->sheet[sheetX-1].vhl = (struct V4XLSX__HyperLinks *)v4mm_AllocChunk(sizeof(struct V4XLSX__HyperLinks),FALSE)) ; vhl->num = 0 ; vhl->maxNum = V4XLSX_HyperLink_Dflt ; } ;
	   if (vhl->num >= vhl->maxNum)
	    { vhl->maxNum *= 1.5 ; vhl = realloc(vhl,((char *)&vhl->entry[vhl->maxNum])-(char *)vhl) ; 
	      vrhm->xlsx->sheet[sheetX-1].vhl = vhl ;
	    } ;
	   vhl->entry[vhl->num].row = rowNum ; vhl->entry[vhl->num].col = colNum ;
//	   v4rx_FixUpStr(urlFixed,urlInfo,TRUE) ;
	   vhl->entry[vhl->num].url = v4mm_AllocUC(UCstrlen(urlInfo)) ; UCstrcpy(vhl->entry[vhl->num].url,urlInfo) ;
	   vhl->num++ ;
	 } ;

/*	And the value */
	UCstrcat(xml,UClit("<v>")) ;
	valLen = UCstrlen(cell->cellValue) ;
	if (valLen == 0)
	 { if (vdi->v4PntType == V4DPI_PntType_Int || vdi->v4PntType == V4DPI_PntType_Real)
	    { cell->cellValue = UClit("0") ; valLen = 1 ; }
	    else if (vdi->v4PntType == V4DPI_PntType_UDate || vdi->v4PntType == V4DPI_PntType_UDT || vdi->v4PntType == V4DPI_PntType_Calendar)
		  { cell->cellValue = UClit("-9999999") ; valLen = UCstrlen(cell->cellValue) ; } ;
	 } ;
	switch (cell->pntType == UNUSED ? (UCempty(cell->cellValue) ? V4DPI_PntType_Char : vdi->v4PntType) : cell->pntType)
	 { default:
		v_Msg(NULL,vrhm->errMessage,"@Invalid point type (%1d) cell (%2U)",vdi->v4PntType,cell->cellValue) ; return(UNUSED) ;
//	   case V4DPI_PntType_MIDASUDT:
//		inum = v_ParseInt(cell->cellValue,&tc,10) ;
//		if (*tc != UCEOS)
//		 { if (++invCount < 25) 
//		    v_Msg(NULL,vrhm->errMessage,"@*Invalid value (%1U) for point type (#%2d) at row(%3d) column(%4d)... replacing with 'none'\n",cell->cellValue,(cell->pntType == UNUSED ? vdi->v4PntType : cell->pntType),rowNum,colNum) ; vout_UCText(VOUT_Warn,0,vrhm->errMessage) ;
//		   cell->cellValue = UClit("0") ;
//		 } ; 
///*		Convert to date+fractional-time, in Excel 1/1/1900 = 1 so 15020 is offset to make that happen */
//		if (inum > 0)
//		 { UCCHAR cbuf[32] ;
//		   cal = (UDTtoUD(inum) - 15019) + ((double)UDTtoSeconds(inum) / (24.0 * 60.0 * 60.0)) ;
//		   UCsprintf(cbuf,UCsizeof(cbuf),V4DPI_PntFormat_Real,cal) ; UCstrcat(xml,cbuf) ;
//		   if (col->width < 15) col->width = 15 ;
//		 } ;
////		cal = UDTtoCAL(inum) - (double)(gpi->MinutesWest / (24.0 * 60.0)) ;
////		cal -= 693594 ;
////		if (cal > 0)
////		 { UCCHAR cbuf[32] ; UCsprintf(cbuf,UCsizeof(cbuf),UClit("%g"),cal) ; UCstrcat(xml,cbuf) ; } ;
////		if (col->width < 15) col->width = 15 ;
//		break ;
	   case V4DPI_PntType_UDate:
	   case V4DPI_PntType_UDT:
	   case V4DPI_PntType_Calendar:
		cal = v_ParseDbl(cell->cellValue,&tc) ;
		if (*tc != UCEOS)
		 { if (++invCount < 25) 
		    v_Msg(NULL,vrhm->errMessage,"@*Invalid value (%1U) for point type (#%2d) at row(%3d) column(%4d)... replacing with 'none'\n",cell->cellValue,(cell->pntType == UNUSED ? vdi->v4PntType : cell->pntType),rowNum,colNum) ; vout_UCText(VOUT_Warn,0,vrhm->errMessage) ;
		   cell->cellValue = UClit("0") ;
		 } ; 
/*		If we have a time component then adjust for local time */
		if (UCstrchr(cell->cellValue,'.') != NULL)
		 { int width ;
		   switch (gpi->ci->Cntry[gpi->ci->CurX].ymdOrder[0])
		    { default:			width = 15 ; break ;
		      case V4LEX_YMDOrder_YMD:	width = 17 ; break ;
		    } ;
		   cal -= (double)(gpi->MinutesWest / (24.0 * 60.0)) ; if (col->width < width) col->width = width ;
		 } else
		 { if (col->width < 10) col->width = 10 ; } ;
		cal = cal - VCal_MJDOffset - 50084 + 35065 ;
		if (cal > 0)
		 { UCCHAR cbuf[32] ; UCsprintf(cbuf,UCsizeof(cbuf),V4DPI_PntFormat_Real,cal) ;
		   UCstrcat(xml,cbuf) ;
		 } ;
		break ;
	   case V4DPI_PntType_UYear:
	   case V4DPI_PntType_Int:
	   case V4DPI_PntType_Delta:
		{ int ival ; UCCHAR *d, ibuf[32] ;
		  ival = v_ParseInt(cell->cellValue,&d,10) ;
		  if (*d != UCEOS)
		   { if (++invCount < 25) 
		      v_Msg(NULL,vrhm->errMessage,"@*Invalid value (%1U) for point type (#%2d) at row(%3d) column(%4d)... replacing with '0'\n",cell->cellValue,(cell->pntType == UNUSED ? vdi->v4PntType : cell->pntType),rowNum,colNum) ; vout_UCText(VOUT_Warn,0,vrhm->errMessage) ;
		     cell->cellValue = UClit("0") ;
		   } ; 
//	          UCstrcat(xml,cell->cellValue) ;
		  UCsprintf(ibuf,100,UClit("%d"),ival) ; UCstrcat(xml,ibuf) ;
		  if (UCstrlen(cell->cellValue) > col->width) col->width = UCstrlen(cell->cellValue) ;
		  if (col->width < valLen) col->width = valLen ;
		}
		break ;
	   case V4DPI_PntType_Real:
		dbl = v_ParseDbl(cell->cellValue,&tc) ;
		if (*tc != UCEOS)
		 { if (++invCount < 25) 
		    v_Msg(NULL,vrhm->errMessage,"@*Invalid value (%1U) for point type (#%2d) at row(%3d) column(%4d)... replacing with '0'\n",cell->cellValue,(cell->pntType == UNUSED ? vdi->v4PntType : cell->pntType),rowNum,colNum) ; vout_UCText(VOUT_Warn,0,vrhm->errMessage) ;
		   cell->cellValue = UClit("0") ;
		 } ; 
		if (fabs(dbl) < vrhm->epsilon)
		 { UCstrcat(xml,UClit("0")) ; }
		 else { UCCHAR dbuf[32] ;
//		        UCstrcat(xml,cell->cellValue) ;
			UCsprintf(dbuf,100,V4DPI_PntFormat_Real,dbl) ; UCstrcat(xml,dbuf) ;
		       } ;
/*		Determining the width is not easy - have to format each number and guess (can't rely on input length- '23.0000000000001' may format as '23.00') */
		{ UCCHAR mask[64],dst[128] ;
		  UCstrcpy(mask,(vdi->maskX != UNUSED ? vrhm->vmi->mask[vdi->maskX].formatMask : UClit(""))) ; if (UCempty(mask)) UCstrcpy(mask,UClit("%g")) ;
		  v_FormatDbl(dbl,mask,dst,NULL) ; valLen = UCstrlen(dst) ;
		}
		if (col->width < valLen) col->width = valLen ;
		break ;
	   case V4DPI_PntType_Logical:
		{ UCCHAR lbuf[32] ;
		  logical = v_ParseLog(cell->cellValue,&tc) ;
		  UCsprintf(lbuf,UCsizeof(lbuf),UClit("%d"),v4rx_sshInsert(vrhm,(logical ? UClit("Yes") : UClit("No")))) ;
		  UCstrcat(xml,lbuf) ;
		  if (col->width < 3) col->width = 3 ;
		}
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
/*		If newlines in cell then count number of lines and adjust height */
		{ COUNTER lines ; UCCHAR *p ;
		  for(lines=1,p=cell->cellValue;;lines++,p++)
		   { p = UCstrchr(p,'\r') ;
		     if (p == NULL) break ;
		   } ;
		  if (lines > 1)
		   { if (lines > *height) *height = lines ; } ;
		}
/*		Have to check format for multi-column centered string - if so parse number of columns */
		if (colSpan != 0)
		 { UCCHAR buf[4096] ;
		   v4rx_FixUpString(cell->cellValue) ;
		   UCsprintf(buf,UCsizeof(buf),UClit("%d"),v4rx_sshInsert(vrhm,cell->cellValue)) ;
		   UCstrcat(xml,buf) ;
		   colNum++ ;
//		   for(i=1;i<colSpan;i++)		/* Fill out following columns with blank cells */
//		    { wsWriteString(ws,rowNum,colNum++,"",(format == NULL ? vrhm->defaultformat : format)) ; } ;
		   break ;
		 } ;
/*		If any embedded HTML stuff or &xxx; then strip out */
//		v4rx_FixUpString(cell->cellValue) ;
		{ UCCHAR buf[4096] ;
		  v4rx_FixUpString(cell->cellValue) ;
		  UCsprintf(buf,UCsizeof(buf),UClit("%d"),v4rx_sshInsert(vrhm,cell->cellValue)) ;
		  UCstrcat(xml,buf) ;
		}
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
//		if (format != NULL) { if (format->bold >= 0x2bc) inum += 2 ; } ;
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
//		if (inum != UNUSED) wsWriteError(ws,rowNum,colNum,inum,(format == NULL ? vrhm->defaultformat : format)) ;
		break ;
	 } ;

	UCstrcat(xml,UClit("</v>")) ;
/*	Close it off & return */
	UCstrcat(xml,UClit("</c>")) ;

	return(colSpan == 0 ? 1 : colSpan) ;
}

#define WSPreface UClit(\
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?> \
<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"> \
	%s \
	<dimension ref=\"%s:%s\"/> \
	<sheetViews> \
		<sheetView tabSelected=\"1\" workbookViewId=\"0\"> \
			<selection activeCell=\"A1\" sqref=\"A1\"/> \
		</sheetView> \
	</sheetViews> \
	<sheetFormatPr defaultRowHeight=\"15\"/>")


#define ZIPCOMPONENT(XMLBUF,FILENAME,FREEDONE) \
{ char *utf8 ; UCCHAR *xml = XMLBUF ; LENMAX utf8Bytes ; LENMAX len = UCstrlen(xml) ; \
  utf8Bytes = (len * 1.5) ; utf8 = (char *)v4mm_AllocChunk(utf8Bytes,FALSE) ; \
  len = UCUTF16toUTF8(utf8,utf8Bytes,xml,len) ; \
  vzip_AppendFile(vzm,FILENAME,len,utf8,NULL,TRUE,0) ; \
  if (FREEDONE) v4mm_FreeChunk(xml) ; \
}

LOGICAL v4rx_GenerateXLSX(vrhm,toWWW)
  struct V4RH__Main *vrhm ;
  LOGICAL toWWW ;
{
  struct V4RH__NamedEntry *vrneId,*vrneName ;
  struct V4RH__Sheet *sheet ; struct V4RH__Row *row ; struct V4RH__Cell *cell ; struct V4RH__Column *col ;
  INDEX sheetX ;

	if (vrhm->gotError) { printf("?Errors detected, cannot create xlsx\n") ; return(FALSE) ; } ;
	v4xlsx_Initialize(vrhm) ;

/*	Iterate through entire report */
	for (sheetX=1,sheet=vrhm->sheetFirst;sheet!=NULL;sheet=sheet->sheetNext,sheetX++)
	 { INDEX rowNum, colNum, maxCol, tx, dataRows ;
	   struct V4XLSX__XMLS *xmls,xmlsData ;
	   UCCHAR tbuf[8192] ;
	   
/*	   Add the sheet to [Content_Type].xml */
	   { UCCHAR wsBuf[128] ; UCsprintf(wsBuf,UCsizeof(wsBuf),UClit("/xl/worksheets/sheet%d.xml"),sheetX) ;
	     v4rx_AddContentType(vrhm,NULL,wsBuf,UClit("application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml")) ;
	   }

/*	   Init expandable XML buffer */
	   XMLSInit(&xmlsData) ;
	   maxCol = 0 ;			/* Track highest column */
/*	   First do the headers */
	   for(rowNum=0,row=sheet->hdrFirst;row!=NULL;row=row->rowNext,rowNum++)
	    { struct V4XLSX__XMLS xmlsRow ; UCCHAR rbuf[128] ; int height=UNUSED ;
	      XMLSInit(&xmlsRow) ;
	      for(colNum=0,cell=row->cellFirst,col=sheet->colFirst;cell!=NULL;cell=cell->cellNext,col=col->colNext)
	       { COUNTER colInc ; UCCHAR xml[1024] ; ZUS(xml) ;
		 colInc = v4rx_CreateCellNEW(vrhm,xml,sheetX,rowNum,colNum,row,cell,col,&height) ;
		 if (colInc == UNUSED) return(FALSE) ;
		 colNum += colInc ;
		 XMLSAppend(&xmlsRow,xml) ;
		 if (colNum > maxCol) maxCol = colNum ;
	       } ;
	      if (colNum == 0)
	       continue ;	/* Empty row - don't output anything */
	      if (height != UNUSED)
	       { UCsprintf(rbuf,UCsizeof(rbuf),UClit("<row r=\"%d\" spans=\"%d:%d\" ht=\"%d\" customHeight=\"1\">"),rowNum+1,1,colNum,(height*15)) ;
	       } else
	       { UCsprintf(rbuf,UCsizeof(rbuf),UClit("<row r=\"%d\" spans=\"%d:%d\">"),rowNum+1,1,colNum) ; } ;
	      XMLSAppend(&xmlsData,rbuf) ;
	      XMLSAppend(&xmlsData,XMLSContents(&xmlsRow)) ;
	      XMLSAppend(&xmlsData,UClit("</row>")) ; XMLSAppend(&xmlsData,UCTXTEOL) ;
	    } ;
	   sheet->hdrNum = rowNum ;		/* Remember how many header rows we have */

/*	   Now the data rows */
	   for(dataRows=0,row=sheet->rowFirst;row!=NULL;row=row->rowNext,rowNum++,dataRows++)
	    { struct V4XLSX__XMLS xmlsRow ; UCCHAR rbuf[128] ; UCCHAR lHdr[3],*cp,*ep,*arg ; COUNTER height=UNUSED ;

/*	      Parse any special row information (ex: page breaks) */
	      for(cp=row->rowInfo,ep=cp;ep!=NULL;cp=ep+1)
	       { ep = UCstrchr(cp,EOLbt) ; if (ep != NULL) *ep = UCEOS ;	/* Set terminating char to EOS - HAVE TO RESET!!! */
		 if (UCempty(cp)) continue ;
		 lHdr[0] = cp[0] ; lHdr[1] = cp[1] ; ; lHdr[2] = UCEOS ; arg = &cp[2] ;
		 switch (v4im_GetUCStrToEnumVal(lHdr,0))
		  { default:	break ;
		    case _PB:
/*			brkRow[n] = 0 indexed row to break AFTER, if PBA then save rowNum+1 else if PBB then rowNum */
			if (vrhm->xlsx->sheet[sheetX-1].numBrks <= V4XLSX_PAGEBRK_MAX)
			 { INDEX brkRow = rowNum + (arg[0] == UClit('A') ? 1 : 0) ;
			   vrhm->xlsx->sheet[sheetX-1].brkRow[vrhm->xlsx->sheet[sheetX-1].numBrks] = brkRow ;
			   vrhm->xlsx->sheet[sheetX-1].numBrks++ ;
			 } ;
			break ;
		  } ;
	       } ;

	      XMLSInit(&xmlsRow) ;
	      vrhm->pageBreak = FALSE ;				/* Set global break indicator to false, if set true by end of row then set in Excel */
	      if (row->skipRow) continue ;
	      for(colNum=0,cell=row->cellFirst,col=sheet->colFirst;cell!=NULL;cell=cell->cellNext,col=col->colNext)
	       { COUNTER colInc ; UCCHAR xml[4096] ; ZUS(xml) ; 
		 colInc = v4rx_CreateCellNEW(vrhm,xml,sheetX,rowNum,colNum,row,cell,col,&height) ;
		 if (colInc == UNUSED) return(FALSE) ;
		 colNum += colInc ;
		 XMLSAppend(&xmlsRow,xml) ;
		 if (colNum > maxCol) maxCol = colNum ;
	       } ;
	      if (colNum == 0)
	       continue ;	/* Empty row - don't output anything */
	      UCsprintf(rbuf,UCsizeof(rbuf),UClit("<row r=\"%d\" spans=\"%d:%d\">"),rowNum+1,1,colNum) ;
	      XMLSAppend(&xmlsData,rbuf) ;
	      XMLSAppend(&xmlsData,XMLSContents(&xmlsRow)) ;
	      XMLSAppend(&xmlsData,UClit("</row>")) ; XMLSAppend(&xmlsData,UCTXTEOL) ;
	    } ;

/*	   Now any footers */
	   for(row=sheet->ftrFirst;row!=NULL;row=row->rowNext,rowNum++)
	    { struct V4XLSX__XMLS xmlsRow ; UCCHAR rbuf[128] ; COUNTER height=UNUSED ;
	      XMLSInit(&xmlsRow) ;
	      for(colNum=0,cell=row->cellFirst,col=sheet->colFirst;cell!=NULL;cell=cell->cellNext,col=col->colNext)
	       { COUNTER colInc,height=UNUSED ; UCCHAR xml[1024] ; ZUS(xml) ;
		 colInc = v4rx_CreateCellNEW(vrhm,xml,sheetX,rowNum,colNum,row,cell,col,&height) ;
		 if (colInc == UNUSED) return(FALSE) ;
		 colNum += colInc ;
		 XMLSAppend(&xmlsRow,xml) ;
		 if (colNum > maxCol) maxCol = colNum ;
	       } ;
	      if (colNum == 0)
	       continue ;	/* Empty row - don't output anything */
	      UCsprintf(rbuf,UCsizeof(rbuf),UClit("<row r=\"%d\" spans=\"%d:%d\">"),rowNum+1,1,colNum) ;
	      XMLSAppend(&xmlsData,rbuf) ;
	      XMLSAppend(&xmlsData,XMLSContents(&xmlsRow)) ;
	      XMLSAppend(&xmlsData,UClit("</row>")) ; XMLSAppend(&xmlsData,UCTXTEOL) ;
	    } ;


/*	   Have all the components for the worksheet, put it all together */
	   xmls = &vrhm->xlsx->sheet[vrhm->xlsx->numSheets++].xmls ; XMLSInit(xmls) ;
/*	   Inject preface, parameters are upper left & lower right corners of spreadsheet */
	   UCsprintf(tbuf,UCsizeof(tbuf),WSPreface,(sheet->scale == -1 ? UClit("<sheetPr><pageSetUpPr fitToPage=\"1\"/></sheetPr>") : UClit("")),v4rx_ExcelRowColSpec(0,0),v4rx_ExcelRowColSpec(rowNum-1,maxCol-1)) ;
	   XMLSAppend(xmls,tbuf)
	    
/*	   Dump out column specifications */
	   for(colNum=1,col=sheet->colFirst;col!=NULL;col=col->colNext,colNum++)
	    { if (col->width != 0) break ; } ;
/*	   Only do this if we have at least one column with non-default width */
	   if (col != NULL)
	    { XMLSAppend(xmls,UClit("<cols>")) ;
	      for(colNum=1,col=sheet->colFirst;col!=NULL;col=col->colNext,colNum++)
	       { if (col->width == 0) continue ;
//	         UCsprintf(tbuf,UCsizeof(tbuf),UClit("<col min=\"%d\" max=\"%d\" width=\"%d\" customWidth=\"1\"/>"),colNum,colNum,col->width) ;
	         UCsprintf(tbuf,UCsizeof(tbuf),UClit("<col min=\"%d\" max=\"%d\" width=\"%d\" bestFit=\"1\"/>"),colNum,colNum,col->width+1) ;
	         XMLSAppend(xmls,tbuf) ;
/*
other possible attributes here are
style='n'
hidden='1' column is hidden
bestFit='1' - set to best fit size

Column width measured as the number of characters of the maximum digit width of the
numbers 0, 1, 2, ..., 9 as rendered in the normal style's font. There are 4 pixels of margin
padding (two on each side), plus 1 pixel padding for the gridlines.
width = Truncate([{Number of Characters} * {Maximum Digit Width} + {5 pixel
padding}]/{Maximum Digit Width}*256)/256
Using the Calibri font as an example, the maximum digit width of 11 point font size is 7
pixels (at 96 dpi). In fact, each digit is the same width for this font. Therefore if the cell
width is 8 characters wide, the value of this attribute shall be
Truncate([8*7+5]/7*256)/256 = 8.7109375.
To translate the value of width in the file into the column width value at runtime
SpreadsheetML Reference Material - Worksheets
1941
Attributes Description
(expressed in terms of pixels), use this calculation:
=Truncate(((256 * {width} + Truncate(128/{Maximum Digit Width}))/256)*{Maximum
Digit Width})
Using the same example as above, the calculation would be
Truncate(((256*8.7109375+Truncate(128/7))/256)*7) = 61 pixels
To translate from pixels to character width, use this calculation:
=Truncate(({pixels}-5)/{Maximum Digit Width} * 100+0.5)/100
Using the example above, the calculation would be Truncate((61-5)/7*100+0.5)/100 = 8
characters.
Note: when wide borders are applied, part of the left/right border shall overlap with the
2 pixel padding on each side. Wide borders do not affect the width calculation of the
column.
Note: When the sheet is in the mode to view formulas instead of values, the pixel width
of the column is doubled.
The possible values for this attribute are defined by the XML Schema double datatype.
*/


	       } ;
	    } ;
	   XMLSAppend(xmls,UClit("</cols>")) ;


/*	   Now the sheet data */
	   XMLSAppend(xmls,UClit("<sheetData>")) ;
	   XMLSAppend(xmls,XMLSContents(&xmlsData)) ; XMLSClose(&xmlsData) ;
	   XMLSAppend(xmls,UClit("</sheetData>")) ;

/*	   Any cells to be merged? */
	   if (vrhm->xlsx->sheet[sheetX-1].vcs != NULL)
	    { INDEX i ; struct V4XLSX__CellSpans *vcs = vrhm->xlsx->sheet[sheetX-1].vcs ;
	      UCsprintf(tbuf,UCsizeof(tbuf),UClit("<mergeCells count=\"%d\">"),vcs->num) ; XMLSAppend(xmls,tbuf) ;
	      for(i=0;i<vcs->num;i++)
	       { UCsprintf(tbuf,UCsizeof(tbuf),UClit("<mergeCell ref=\"%s:%s\"/>"),v4rx_ExcelRowColSpec(vcs->entry[i].row,vcs->entry[i].col),v4rx_ExcelRowColSpec(vcs->entry[i].row,vcs->entry[i].col+vcs->entry[i].span-1)) ;
	         XMLSAppend(xmls,tbuf) ;
	       } ;
	      XMLSAppend(xmls,UClit("</mergeCells>")) ;
	    } ;

/*	   Any embedded cell comments in this sheet ? */
	   if (vrhm->xlsx->vcom != NULL)
	    { struct V4XLSX__4MAT_Comment *vcom = vrhm->xlsx->vcom ; INDEX i ; COUNTER num ;
	      for(num=0,i=0;i<vcom->num;i++)				/* Count the number on this sheet */
	       { if (vcom->entry[i].sheetX == sheetX) num++ ; } ;
	      if (num > 0)
	       { UCCHAR dvBuf[128] ;
	         UCsprintf(dvBuf,UCsizeof(dvBuf),UClit("<dataValidations count=\"%d\">"),num) ;
	         XMLSAppend(xmls,dvBuf) ;
	       } ;
	      for(i=0;i<vcom->num;i++)
	       { UCCHAR dvBuf[1024], *t ;
	         if (vcom->entry[i].sheetX != sheetX) continue ;
/*		 If we have an embedded tab then comment broken into title<tab>comment otherwise just have comment */
		 t = UCstrchr(vcom->entry[i].str,'\t') ;
		 if (t == NULL)
		  { UCsprintf(dvBuf,UCsizeof(dvBuf),UClit("<dataValidation allowBlank=\"1\" showInputMessage=\"1\" showErrorMessage=\"1\" prompt=\"%s\" sqref=\"%s\"/>"),
			vcom->entry[i].str,v4rx_ExcelRowColSpec(vcom->entry[i].row,vcom->entry[i].col)) ;
		  } else
		  { *t = UCEOS ; t++ ;
		    UCsprintf(dvBuf,UCsizeof(dvBuf),UClit("<dataValidation allowBlank=\"1\" showInputMessage=\"1\" showErrorMessage=\"1\" promptTitle=\"%s\" prompt=\"%s\" sqref=\"%s\"/>"),
			vcom->entry[i].str,t,v4rx_ExcelRowColSpec(vcom->entry[i].row,vcom->entry[i].col)) ;
		  } ;
	         XMLSAppend(xmls,dvBuf) ;
	       } ;
	      if (num > 0) { XMLSAppend(xmls,UClit("</dataValidations>")) ; } ;
	    } ;

/*	   Any hyperlinks ? */
	   if (vrhm->xlsx->sheet[sheetX-1].vhl != NULL)
	    { INDEX i,rId ; struct V4XLSX__HyperLinks *vhl = vrhm->xlsx->sheet[sheetX-1].vhl ;
	      XMLSAppend(xmls,UClit("<hyperlinks>")) ;
	      for(i=0;i<vhl->num;i++)
	       { LOGICAL extURL ; UCCHAR url[2048],urlFixed[2048],urlDisplay[2048] ;
/*		 Should we insert actual URL or use linkage to URL file saved on server (a lot more secure) */
		 extURL = (UCstrstr(vhl->entry[i].url,UClit("_V4=")) == NULL) && (UCstrstr(vhl->entry[i].url,UClit("_v4=")) == NULL) ;
		 switch (extURL ? UNUSED : vrhm->xlsxURL)
		  { default:
		    case V4RH_XLSXURL_none:
			break ; /* (should not get here) */
		    case V4RH_XLSXURL_exOnly:
			break ; /* (should not get here, extURL will be TRUE and so switch value is UNUSED) */
		    case V4RH_XLSXURL_userKey:
/*			Want to use user's existing session key */
			UCstrcpy(url,vhl->entry[i].url) ; UCstrcat(url,vrhm->xlsx->urlSuffix) ;
			UCstrcpy(urlDisplay,UClit("NON SECURE")) ;
			break ;
		    case V4RH_XLSXURL_secure:
			{ UCCHAR *qm = UCstrchr(vhl->entry[i].url,'?') ;
/*			  Have to save URL info in local (to server) text file, just use index as argument */
			  fprintf(vrhm->xlsx->UCFile.fp,"%s\n",UCretASC(qm == NULL ? vhl->entry[i].url : qm+1)) ;
			  vrhm->xlsx->urlX++ ; *(qm+1) = UCEOS ;
/*			  The urlSuffix is going to be first thing after '?', strip off possible leading '&' */
			  if (vrhm->xlsx->urlSuffix[0] == UClit('&')) vrhm->xlsx->urlSuffix++ ;
/*			  Create new URL - everything before '?' then special arguments for secure linkage */
			  UCsprintf(url,UCsizeof(url),UClit("%s%s&urlX=%d"),vhl->entry[i].url,vrhm->xlsx->urlSuffix,vrhm->xlsx->urlX) ;
			}
			UCsprintf(urlDisplay,UCsizeof(urlDisplay),UClit("secure #%d"),vrhm->xlsx->urlX) ;
			break ;
		    case UNUSED:
			UCstrcpy(url,vhl->entry[i].url) ;
//			UCstrcpy(urlDisplay,url) ;		/* Display full URL */
			v4rx_FixUpStr(urlDisplay,url,TRUE) ;
			break ; /* (external URL link - don't manipulate it at all) */
		  } ;
	         v4rx_FixUpStr(urlFixed,url,TRUE) ;
	         rId = v4rx_InsertXSR(vrhm,sheetX,UClit("http://schemas.openxmlformats.org/officeDocument/2006/relationships/hyperlink"),urlFixed,TRUE) ;
	         UCsprintf(tbuf,UCsizeof(tbuf),UClit("<hyperlink ref=\"%s\" r:id=\"rId%d\" tooltip=\"%s\"/>"),v4rx_ExcelRowColSpec(vhl->entry[i].row,vhl->entry[i].col),rId,urlDisplay) ;
	         XMLSAppend(xmls,tbuf) ;
	       } ;
	      XMLSAppend(xmls,UClit("</hyperlinks>")) ;
	    } ;
	   if (sheet->gridLines != 0)
	    { XMLSAppend(xmls,UClit("<printOptions gridLines=\"1\"/>")) ;
	    } ;
	   
#define MARGIN(NAME) (sheet->margin##NAME == 0 ? V4RPP_ExcelDflt_Margin##NAME : sheet->margin##NAME)
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("<pageMargins left=\"%g\" right=\"%g\" top=\"%g\" bottom=\"%g\" header=\"%g\" footer=\"%g\"/>"),
		MARGIN(Left),MARGIN(Right),MARGIN(Top),MARGIN(Bottom),V4RPP_ExcelDflt_HdrMargin,V4RPP_ExcelDflt_FtrMargin) ;
	   XMLSAppend(xmls,tbuf) ;
	   
/*	   Do page setup, have to link this sheet to proper printer settings via sheet specific rels xmls */

	   switch (sheet->orientPage)
	    { case -1:	XMLSAppend(xmls,UClit("<pageSetup orientation=\"landscape\"")) ; break ;
	      case 1:	XMLSAppend(xmls,UClit("<pageSetup orientation=\"portrait\"")) ; break ;
	      case 0:	XMLSAppend(xmls,(maxCol >= 10 ? UClit("<pageSetup orientation=\"landscape\"") : UClit("<pageSetup orientation=\"portrait\""))) ; break ;
	    } ;
/*	   Are we scaling this worksheet? */
	   if (sheet->scale > 0)
	    { UCCHAR sBuf[128] ;
	      UCsprintf(sBuf,UCsizeof(sBuf),UClit(" scale=\"%g\""),sheet->scale) ; XMLSAppend(xmls,sBuf) ;
/*		     Try to print across one page, multiple pages down */
//	    } else { XMLSAppend(xmls,UClit(" fitToWidth=\"1\" fitToHeight=\"100\"")) ; } ;
	    } else { XMLSAppend(xmls,UClit(" fitToHeight=\"999\"")) ; } ;

#ifdef V4XLSX_Include_PrinterSettings
	   { INDEX rId = v4rx_InsertXSR(vrhm,sheetX,UClit("http://schemas.openxmlformats.org/officeDocument/2006/relationships/printerSettings"),UClit("../printerSettings/printerSettings1.bin"),FALSE) ;
	     UCsprintf(tbuf,UCsizeof(tbuf),UClit(" r:id=\"rId%d\"/>"),rId) ; XMLSAppend(xmls,tbuf) ;
	   }
#else
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("/>")) ; XMLSAppend(xmls,tbuf) ;
#endif

/*	   Handle title lines (headers) & footers */
	   XMLSAppend(xmls,UClit("<headerFooter>")) ;
	   ZUS(tbuf);
	   for(tx=0;tx<V4RH_Max_Titles;tx++)
	    { if (sheet->ucTitles[tx] == NULL) break ;
	      if (tx == 0) UCstrcat(tbuf,UClit("&amp;C")) ;	/* &C = center this next string (&L & &R are for left & right) */
	      if (tx > 0) UCstrcat(tbuf,UCTXTEOL) ;
	      v4rx_FixUpStr(&tbuf[UCstrlen(tbuf)],sheet->ucTitles[tx],FALSE) ;
	    } ;
	   if (UCnotempty(tbuf)) { XMLSAppend(xmls,UClit("<oddHeader>")) ; XMLSAppend(xmls,tbuf) ; XMLSAppend(xmls,UClit("</oddHeader>")) ; } ;
	   vrneId =  v4rh_neLookup(vrhm,UClit("jobId")) ;
	   vrneName =  v4rh_neLookup(vrhm,UClit("reportName")) ;
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("<oddFooter>&amp;L%s %s[%d]&amp;CPage &amp;P&amp;R</oddFooter>"),
		(vrneId == NULL ? UClit("&amp;D") : vrneId->eVal),(vrneName == NULL ? UClit("") : vrneName->eVal),dataRows) ;
	   XMLSAppend(xmls,tbuf) ;
	   XMLSAppend(xmls,UClit("</headerFooter>")) ;
/*	   Insert any page breaks ? */
	   if (vrhm->xlsx->sheet[sheetX-1].numBrks > 0)
	    { UCCHAR bbuf[128] ; INDEX i ;
	      UCsprintf(bbuf,UCsizeof(bbuf),UClit("<rowBreaks count=\"%d\" manualBreakCount=\"%d\">"),vrhm->xlsx->sheet[sheetX-1].numBrks,vrhm->xlsx->sheet[sheetX-1].numBrks) ;
	      XMLSAppend(xmls,bbuf) ;
	      for(i=0;i<vrhm->xlsx->sheet[sheetX-1].numBrks;i++)
	       { UCsprintf(bbuf,UCsizeof(bbuf),UClit("<brk id=\"%d\" man=\"1\" max=\"%d\"/>"),vrhm->xlsx->sheet[sheetX-1].brkRow[i],16383) ;
	         XMLSAppend(xmls,bbuf) ;
	       } ;
	      XMLSAppend(xmls,UClit("</rowBreaks>")) ;
	    } ;
/*	   Did we come across any images to include in this sheet ? */
	   if (vrhm->xlsx->vdrw != NULL)
	    { struct V4XLSX__4MAT_Drawing *vdrw = vrhm->xlsx->vdrw ; INDEX i ;
	      for(i=0;i<vdrw->numEntries;i++)
	       { if (vdrw->entry[i].sheetX != sheetX) continue ;
/*		 Create link in worksheet to corresponding drawingX.xml which lists cells containing images */
	         UCsprintf(tbuf,UCsizeof(tbuf),UClit("<drawing r:id=\"rId%d\"/>"),sheetX) ; XMLSAppend(xmls,tbuf) ;
	         break ;			/* Only want one per worksheet */
	       } ;
	    } ;
#ifdef FULLCOMMENTS
/*	   Any embedded cell comments in this sheet ? */
	   if (vrhm->xlsx->vcom != NULL)
	    { struct V4XLSX__4MAT_Comment *vcom = vrhm->xlsx->vcom ; INDEX i,rId ; UCCHAR cBuf[64],fBuf[64] ;
	      for(i=0;i<vcom->num;i++)
	       { if (vcom->entry[i].sheetX != sheetX) continue ;
	         UCsprintf(fBuf,UCsizeof(fBuf),UClit("../drawings/vmlDrawing%d.vml"),sheetX) ;
	         rId = v4rx_InsertXSR(vrhm,sheetX,UClit("http://schemas.openxmlformats.org/officeDocument/2006/relationships/vmlDrawing"),fBuf,FALSE) ;
	         UCsprintf(cBuf,UCsizeof(cBuf),UClit("<legacyDrawing r:id=\"rId%d\"/>"),rId) ;
	         XMLSAppend(xmls,cBuf) ;
	         break ;			/* Only want one of these per sheet */
	       } ;
	    } ;
#endif
	   XMLSAppend(xmls,UClit("</worksheet>")) ;

	 } ;
/*	Have all the bits & pieces, now generate the resulting zip file */
	{ struct VZIP__Master *vzm ;
	  INDEX i ;

/*	  Make sure we have xlsx extension (not xls) and init zip file */
	  if (UCstrcmp(&vrhm->outFile[UCstrlen(vrhm->outFile)-3],UClit("xls")) == 0) { UCstrcat(vrhm->outFile,UClit("x")) ; } ;
	  vzm = vzip_InitZipFile(vrhm->outFile,vrhm->errMessage) ; if (vzm == NULL) return(FALSE) ;


/*	  Any images to include ? (Do these first because they impact other zip components) */

#define imgMask UClit( \
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?> \
<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"> \
	<Relationship Id=\"rId%d\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\" Target=\"../media/image%d.jpeg\"/> \
</Relationships>")
#define drawingXMLMaskHead UClit( \
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?> \
<xdr:wsDr xmlns:xdr=\"http://schemas.openxmlformats.org/drawingml/2006/spreadsheetDrawing\" xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\">")
#define drawingXMLMaskEntry UClit( \
"	<xdr:twoCellAnchor editAs=\"oneCell\"> \
		<xdr:from> \
			<xdr:col>%d</xdr:col> \
			<xdr:colOff>0</xdr:colOff> \
			<xdr:row>%d</xdr:row> \
			<xdr:rowOff>0</xdr:rowOff> \
		</xdr:from> \
		<xdr:to> \
			<xdr:col>%d</xdr:col> \
			<xdr:colOff>0</xdr:colOff> \
			<xdr:row>%d</xdr:row> \
			<xdr:rowOff>0</xdr:rowOff> \
		</xdr:to> \
		<xdr:pic> \
			<xdr:nvPicPr> \
				<xdr:cNvPr id=\"%d\" name=\"Picture %d\" descr=\"%s\"/> \
				<xdr:cNvPicPr> \
					<a:picLocks noChangeAspect=\"1\"/> \
				</xdr:cNvPicPr> \
			</xdr:nvPicPr> \
			<xdr:blipFill> \
				<a:blip xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" r:embed=\"rId%d\" cstate=\"print\"/> \
				<a:stretch> \
					<a:fillRect/> \
				</a:stretch> \
			</xdr:blipFill> \
			<xdr:spPr> \
				<a:prstGeom prst=\"rect\"> \
					<a:avLst/> \
				</a:prstGeom> \
			</xdr:spPr> \
		</xdr:pic> \
		<xdr:clientData/> \
	</xdr:twoCellAnchor>")
#define drawingXMLMaskTail UClit("</xdr:wsDr>")
	  if(vrhm->xlsx->vdrw != NULL)
	   { struct V4XLSX__4MAT_Drawing *vdrw = vrhm->xlsx->vdrw ;
	     struct stat statbuf ; struct UC__File UCFile ;
	     BYTE *pbuf ;
	     v4rx_AddContentType(vrhm,UClit("jpeg"),NULL,UClit("image/jpeg")) ;
	     for(i=0;i<vdrw->numImgs;i++)
	      { UCCHAR imgBuf[4096],fnBuf[512] ; COUNTER cnt ;
	        UCsprintf(imgBuf,UCsizeof(imgBuf),imgMask,vdrw->img[i].rId,vdrw->img[i].rId) ; UCsprintf(fnBuf,UCsizeof(fnBuf),UClit("xl/drawings/_rels/drawing%d.xml.rels"),i+1) ;
	        ZIPCOMPONENT(imgBuf,fnBuf,FALSE) ;
	        if (!v_UCFileOpen(&UCFile,vdrw->img[i].path,UCFile_Open_ReadBin,TRUE,vrhm->errMessage,0)) { return(FALSE) ; } ;
	        fstat(UCFile.fd,&statbuf) ;		/* Get size of the file */
	        pbuf = v4mm_AllocChunk(statbuf.st_size,FALSE) ;
	        cnt = fread(pbuf,1,statbuf.st_size,UCFile.fp) ;
	        if (cnt != statbuf.st_size)
	         { v_Msg(NULL,vrhm->errMessage,"@Error (%1O) reading file (%2U) - read only %3d bytes of %4d",errno,vdrw->img[i].path,cnt,statbuf.st_size) ;
	           return(FALSE) ;
	         } ;
	        v_UCFileClose(&UCFile) ;
	        UCsprintf(fnBuf,UCsizeof(fnBuf),UClit("xl/media/image%d.jpeg"),i+1) ;
	        vzip_AppendFile(vzm,fnBuf,statbuf.st_size,pbuf,NULL,FALSE,0) ;
	      } ;
/*	     Loop through each sheet, then through drawing entries looking for entries on this sheet */
	     for(i=0;i<vrhm->xlsx->numSheets;i++)
	      { UCCHAR fnBuf[128] ; INDEX sheetX ; struct V4XLSX__XMLS xmlsDrw ; LOGICAL startedXML=FALSE ;
	        sheetX = i+1 ;
	        for(i=0;i<vdrw->numEntries;i++)
	         { UCCHAR dBuf[2048] ;
	           if (vdrw->entry[i].sheetX != sheetX) continue ;
	           if (!startedXML)
	            { XMLSInit(&xmlsDrw) ;
	              XMLSAppend(&xmlsDrw,drawingXMLMaskHead) ;
	              startedXML = TRUE ;
	            } ;
/*		   Append this cell to the xmlsDrw buffer */
		   UCsprintf(dBuf,UCsizeof(dBuf),drawingXMLMaskEntry,vdrw->entry[i].col,vdrw->entry[i].row,vdrw->entry[i].col+vdrw->entry[i].cols,vdrw->entry[i].row+vdrw->entry[i].rows,
			i+2,vdrw->img[vdrw->entry[i].imgX].rId,vdrw->img[vdrw->entry[i].imgX].path,vdrw->img[vdrw->entry[i].imgX].rId) ;
		   XMLSAppend(&xmlsDrw,dBuf) ; XMLSAppend(&xmlsDrw,UCTXTEOL) ;
	         } ;
	        if (startedXML)
		 {
/*		   Got all images/drawings for this sheet - output xml file to zip */
		   XMLSAppend(&xmlsDrw,drawingXMLMaskTail) ;			/* Finish off the xml */
/*		   Have to be careful here - contenttype needs leading slash, zip entry does not get leading slash */
		   UCsprintf(fnBuf,UCsizeof(fnBuf),UClit("/xl/drawings/drawing%d.xml"),sheetX) ;
	           v4rx_AddContentType(vrhm,NULL,fnBuf,UClit("application/vnd.openxmlformats-officedocument.drawing+xml")) ;
		   UCsprintf(fnBuf,UCsizeof(fnBuf),UClit("xl/drawings/drawing%d.xml"),sheetX) ;
		   ZIPCOMPONENT(XMLSContents(&xmlsDrw),fnBuf,FALSE) ;
		   XMLSClose(&xmlsDrw) ;
		 } ;
	      } ;
	   } ;

#ifdef FULLCOMMENTS
#define comXMLMaskHead UClit("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><comments xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\"><commentList>")
#define comXMLMaskEntry UClit("<comment ref=\"%s\"><text><r><rPr><sz val=\"9\"/><color indexed=\"81\"/><rFont val=\"Tahoma\"/><family val=\"2\"/></rPr><t xml:space=\"preserve\">%s</t></r></text></comment>")
#define comXMLMaskTail UClit("</commentList></comments>")
/*	  If comments then create file for each worksheet containing comments */
	  if (vrhm->xlsx->vcom != NULL)
	   { INDEX i ; struct V4XLSX__4MAT_Comment *vcom = vrhm->xlsx->vcom ;
	     v4rx_AddContentType(vrhm,UClit("vml"),NULL,UClit("application/vnd.openxmlformats-officedocument.vmlDrawing")) ;
	     for(i=0;i<vrhm->xlsx->numSheets;i++)
	      { UCCHAR fnBuf[128] ; INDEX sheetX,dx ; struct V4XLSX__XMLS xmlsCom ; LOGICAL startedXML=FALSE ;
	        sheetX = i + 1 ;
	        for(dx=0;dx<vcom->num;dx++)
	         { UCCHAR cBuf[1024] ;
	           if (vcom->entry[dx].sheetX != sheetX) continue ;
		   if (!startedXML)
	            { XMLSInit(&xmlsCom) ;
	              XMLSAppend(&xmlsCom,comXMLMaskHead) ;
	              startedXML = TRUE ;
	            } ;
	           UCsprintf(cBuf,UCsizeof(cBuf),comXMLMaskEntry,v4rx_ExcelRowColSpec(vcom->entry[dx].row,vcom->entry[dx].col),vcom->entry[dx].str) ;
	           XMLSAppend(&xmlsCom,cBuf) ;
	         } ;
	        if (startedXML)
	         { XMLSAppend(&xmlsCom,comXMLMaskTail) ;
		   UCsprintf(fnBuf,UCsizeof(fnBuf),UClit("/xl/comments%d.xml"),sheetX) ;
	           v4rx_AddContentType(vrhm,NULL,fnBuf,UClit("application/vnd.openxmlformats-officedocument.spreadsheetml.comments+xml")) ;
		   UCsprintf(fnBuf,UCsizeof(fnBuf),UClit("xl/drawings/vmlDrawing%d.vml"),sheetX) ;
		   ZIPCOMPONENT(XMLSContents(&xmlsCom),fnBuf,FALSE) ;
	           XMLSClose(&xmlsCom) ;
	         } ;
	      } ;
	   } ;
#endif
/*
<xml xmlns:v=\"urn:schemas-microsoft-com:vml\"
 xmlns:o=\"urn:schemas-microsoft-com:office:office\"
 xmlns:x=\"urn:schemas-microsoft-com:office:excel\">
 <o:shapelayout v:ext=\"edit\">
  <o:idmap v:ext=\"edit\" data=\"1\"/>
 </o:shapelayout><v:shapetype id=\"_x0000_t202\" coordsize=\"21600,21600\" o:spt=\"202\"
  path=\"m,l,21600r21600,l21600,xe\">
  <v:stroke joinstyle=\"miter\"/>
  <v:path gradientshapeok=\"t\" o:connecttype=\"rect\"/>
 </v:shapetype><v:shape id=\"_x0000_s1025\" type=\"#_x0000_t202\" style='position:absolute;
  margin-left:81.75pt;margin-top:22.5pt;width:108pt;height:59.25pt;z-index:1;
  visibility:hidden' fillcolor=\"#ffffe1\" o:insetmode=\"auto\">
  <v:fill color2=\"#ffffe1\"/>
  <v:shadow on=\"t\" color=\"black\" obscured=\"t\"/>
  <v:path o:connecttype=\"none\"/>
  <v:textbox style='mso-direction-alt:auto'>
   <div style='text-align:left'></div>
  </v:textbox>
  <x:ClientData ObjectType=\"Note\">
   <x:MoveWithCells/>
   <x:SizeWithCells/>
   <x:Anchor>
    1, 15, 0, 30, 2, 95, 4, 9</x:Anchor>
   <x:AutoFill>False</x:AutoFill>
   <x:Row>1</x:Row>
   <x:Column>0</x:Column>
  </x:ClientData>
 </v:shape></xml>
 
 
 This is with 2 comments
 
 <xml xmlns:v=\"urn:schemas-microsoft-com:vml\"
 xmlns:o=\"urn:schemas-microsoft-com:office:office\"
 xmlns:x=\"urn:schemas-microsoft-com:office:excel\">
 <o:shapelayout v:ext=\"edit\">
  <o:idmap v:ext=\"edit\" data=\"1\"/>
 </o:shapelayout><v:shapetype id=\"_x0000_t202\" coordsize=\"21600,21600\" o:spt=\"202\"
  path=\"m,l,21600r21600,l21600,xe\">
  <v:stroke joinstyle=\"miter\"/>
  <v:path gradientshapeok=\"t\" o:connecttype=\"rect\"/>
 </v:shapetype><v:shape id=\"_x0000_s1025\" type=\"#_x0000_t202\" style='position:absolute;
  margin-left:81.75pt;margin-top:22.5pt;width:108pt;height:59.25pt;z-index:1;
  visibility:hidden' fillcolor=\"#ffffe1\" o:insetmode=\"auto\">
  <v:fill color2=\"#ffffe1\"/>
  <v:shadow on=\"t\" color=\"black\" obscured=\"t\"/>
  <v:path o:connecttype=\"none\"/>
  <v:textbox style='mso-direction-alt:auto'>
   <div style='text-align:left'></div>
  </v:textbox>
  <x:ClientData ObjectType=\"Note\">
   <x:MoveWithCells/>
   <x:SizeWithCells/>
   <x:Anchor>
    1, 15, 0, 30, 2, 95, 4, 9</x:Anchor>
   <x:AutoFill>False</x:AutoFill>
   <x:Row>1</x:Row>
   <x:Column>0</x:Column>
  </x:ClientData>
 </v:shape><v:shape id=\"_x0000_s1026\" type=\"#_x0000_t202\" style='position:absolute;
  margin-left:323.25pt;margin-top:37.5pt;width:108pt;height:59.25pt;z-index:2;
  visibility:visible' fillcolor=\"#ffffe1\" o:insetmode=\"auto\">
  <v:fill color2=\"#ffffe1\"/>
  <v:shadow on=\"t\" color=\"black\" obscured=\"t\"/>
  <v:path o:connecttype=\"none\"/>
  <v:textbox style='mso-direction-alt:auto'>
   <div style='text-align:left'></div>
  </v:textbox>
  <x:ClientData ObjectType=\"Note\">
   <x:MoveWithCells/>
   <x:SizeWithCells/>
   <x:Anchor>
    4, 15, 1, 10, 5, 95, 5, 9</x:Anchor>
   <x:AutoFill>False</x:AutoFill>
   <x:Row>2</x:Row>
   <x:Column>3</x:Column>
  </x:ClientData>
 </v:shape></xml>

*/


/*	  Generate & append to zip file */
	  ZIPCOMPONENT(v4rx_GenerateContentType(vrhm),UClit("[Content_Types].xml"),TRUE) ;
	  ZIPCOMPONENT(v4rx_GenerateRelsDotRels(vrhm),UClit("_rels/.rels"),TRUE) ;
	  ZIPCOMPONENT(v4rx_GenerateApp(vrhm),UClit("docProps/app.xml"),TRUE) ;
	  ZIPCOMPONENT(v4rx_GenerateCore(vrhm),UClit("docProps/core.xml"),TRUE) ;
	  ZIPCOMPONENT(v4rx_GenerateSSH(vrhm),UClit("xl/sharedStrings.xml"),TRUE) ;
	  ZIPCOMPONENT(v4rx_GenerateSTYLEXML(vrhm),UClit("xl/styles.xml"),TRUE) ;
	  ZIPCOMPONENT(v4rx_GenerateWB(vrhm),UClit("xl/workbook.xml"),TRUE) ;
	  ZIPCOMPONENT(v4rx_GenerateWBRels(vrhm),UClit("xl/_rels/workbook.xml.rels"),TRUE) ;
/*	  For theme.xml - Just read in standard and output as-is */
	  { struct stat statbuf ; struct UC__File UCFile ;
	    BYTE *pbuf ;
	    if (!v_UCFileOpen(&UCFile,v_GetV4HomePath(UClit("xlsxTheme1XML.v4i")),UCFile_Open_ReadBin,TRUE,vrhm->errMessage,0)) { return(FALSE) ; } ;
	    fstat(UCFile.fd,&statbuf) ;		/* Get size of the file */
	    pbuf = v4mm_AllocChunk(statbuf.st_size,FALSE) ;
	    fread(pbuf,1,statbuf.st_size,UCFile.fp) ;
	    v_UCFileClose(&UCFile) ;
	    vzip_AppendFile(vzm,UClit("xl/theme/theme1.xml"),statbuf.st_size,pbuf,NULL,TRUE,0) ;
	  }
#ifdef V4XLSX_Include_PrinterSettings
//http://msdn.microsoft.com/en-us/library/dd183565(VS.85).aspx - DEVMODE structure
/*	  For printersettings.xml - Just read in standard and output as-is */
	  { struct stat statbuf ; struct UC__File UCFile ; DEVMODE *dm ;
	    BYTE *pbuf ;
	    if (!v_UCFileOpen(&UCFile,v_GetV4HomePath(UClit("printerSettings.bin")),UCFile_Open_ReadBin,TRUE,vrhm->errMessage,0)) { return(FALSE) ; } ;
	    fstat(UCFile.fd,&statbuf) ;		/* Get size of the file */
	    pbuf = v4mm_AllocChunk(statbuf.st_size,FALSE) ;
	    fread(pbuf,1,statbuf.st_size,UCFile.fp) ;
	    v_UCFileClose(&UCFile) ;
	    dm = (DEVMODE *)pbuf ;
	    vzip_AppendFile(vzm,UClit("xl/theme/printerSettings1.bin"),statbuf.st_size,pbuf,NULL,FALSE,0) ;
	  }
#endif
	  for(i=0;i<vrhm->xlsx->numSheets;i++)
	   { struct V4XLSX__XMLS xmlsRel ;
	     struct V4XLSX__SheetXMLRels *sxr ;
	     UCCHAR tbuf[512] ;
	     UCsprintf(tbuf,UCsizeof(tbuf),UClit("xl/worksheets/sheet%d.xml"),i+1) ;
	     ZIPCOMPONENT(XMLSContents(&vrhm->xlsx->sheet[i].xmls),tbuf,FALSE) ;
/*	     Now link any sheet-specific relationships for this spreadsheet */
	     XMLSInit(&xmlsRel) ;
	     XMLSAppend(&xmlsRel,UClit("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">")) ;
	     for(sxr=vrhm->xlsx->sheet[i].lastSXR;sxr!=NULL;sxr=sxr->priorSXR)
	      { UCCHAR rbuf[1024] ; UCCHAR tmod[64] ;
	        if (sxr->isExternal) { UCstrcpy(tmod,UClit(" TargetMode=\"External\"")) ; } else { ZUS(tmod) ; } ;
	        UCsprintf(rbuf,UCsizeof(rbuf),UClit("<Relationship Id=\"rId%d\" Type=\"%s\" Target=\"%s\"%s/>"),sxr->relId,sxr->type,sxr->target,tmod) ;
	        XMLSAppend(&xmlsRel,rbuf) ;
	      } ;
	     XMLSAppend(&xmlsRel,UClit("</Relationships>")) ;
	     UCsprintf(tbuf,UCsizeof(tbuf),UClit("xl/worksheets/_rels/sheet%d.xml.rels"),i+1) ;
	     ZIPCOMPONENT(XMLSContents(&xmlsRel),tbuf,FALSE) ;
	     XMLSClose(&xmlsRel) ;
	   } ;


	  if (!vzip_CloseZipFile(vzm,vrhm->errMessage)) return(FALSE) ;
	}  
/*	If we created local URL file then close it off */
	if (vrhm->xlsx->urlX > 0) v_UCFileClose(&vrhm->xlsx->UCFile) ;
	return(TRUE) ;

}

/*

01/01/2000 12:00 PM, converts to 36526.5

0 = 'General';
1 = '0';
2 = '0.00';
3 = '#,##0';
4 = '#,##0.00';

9 = '0%';
10 = '0.00%';
11 = '0.00E+00';
12 = '# ?/?';
13 = '# ??/??';
14 = 'mm-dd-yy';
15 = 'd-mmm-yy';
16 = 'd-mmm';
17 = 'mmm-yy';
18 = 'h:mm AM/PM';
19 = 'h:mm:ss AM/PM';
20 = 'h:mm';
21 = 'h:mm:ss';
22 = 'm/d/yy h:mm';

37 = '#,##0 ;(#,##0)';
38 = '#,##0 ;[Red](#,##0)';
39 = '#,##0.00;(#,##0.00)';
40 = '#,##0.00;[Red](#,##0.00)';

44 = '_("$"* #,##0.00_);_("$"* \(#,##0.00\);_("$"* "-"??_);_(@_)';
45 = 'mm:ss';
46 = '[h]:mm:ss';
47 = 'mmss.0';
48 = '##0.0E+0';
49 = '@';

27 = '[$-404]e/m/d';
30 = 'm/d/yy';
36 = '[$-404]e/m/d';
50 = '[$-404]e/m/d';
57 = '[$-404]e/m/d';

59 = 't0';
60 = 't0.00';
61 = 't#,##0';
62 = 't#,##0.00';
67 = 't0%';
68 = 't0.00%';
69 = 't# ?/?';
70 = 't# ??/??';
*/