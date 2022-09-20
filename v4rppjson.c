/*	Here to generate JSON text file from parsed report specs */

#define OJ(STR) vout_UCTextFileX(vrhm->outFileX,0,STR) ;
#define OJL(STRLIT) vout_UCTextFileX(vrhm->outFileX,0,UClit(STRLIT)) ;

#define V4RJ_MAXCOLUMNS 1024
#define V4RJ_METABYTES 4096
#define INITMC(INDEX) if (meta[INDEX] == NULL) { meta[INDEX] = v4mm_AllocUC(V4RJ_METABYTES) ; } ; ZUS(meta[INDEX]) ;

LOGICAL v4j_CreateCell(vrhm,cell,cv,meta,colSpan)
  struct V4RH__Main *vrhm ;
  struct V4RH__Cell *cell ;
  UCCHAR *cv, *meta ;
  COUNTER *colSpan ;
{ struct V4RH__DimInfo *vdi ;
  UCCHAR tbuf[64],*cp,*ep,*arg,*tc,*mask, lHdr[3], td[0x10000],rgbColor[24] ; INDEX line ;
  UCCHAR *className, *idName, *styleInfo, *noteInfo, *eventInfo, *urlInfo, *target, *imgURL ;
  static INDEX idnCount = 0 ;
  int inum ; double dbl ; LOGICAL logical ; CALENDAR cal ; INDEX cssX ;

/*	Update cv with cell value, meta with other meta/format info */
	ZUS(cv) ZUS(meta)
  
	className = NULL ; idName = NULL ; styleInfo = NULL ; noteInfo = NULL ; eventInfo = NULL ; urlInfo = NULL ; target = NULL ; imgURL = NULL ;
	vdi = (cell->dimNum >= 0 && cell->dimNum < V4RH_DIM_MAX ? vrhm->vdi[cell->dimNum] : NULL) ;
/*	If vdi undefined then default with generic alpha */
	if (vdi == NULL)
	 { if (++idnCount < 10) printf("[Invalid dimension number (%d) encountered in cell, defaulting to Alpha]\n",cell->dimNum) ;
	   vdi = vrhm->vdi[cell->dimNum=V4RH_DimNum_Alpha] ;
	 } ;
	mask = (vdi->maskX == UNUSED ? NULL : vrhm->vmi->mask[vdi->maskX].formatMask) ;
	*colSpan = (vdi->vfi != NULL ? vdi->vfi->colSpan : 0) ;
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
	      case _SP:		*colSpan = v_ParseInt(arg,&tc,10) ; break ;
	      case _TA:		target = arg ; break ;
	      case _UR:		urlInfo = arg ;	break ;
	    } ;
	 } ;

	ZUS(td) ;
#undef CAT
#undef CATL
#define CAT(STR) UCstrcat(meta,STR) ;
#define CATL(STR) UCstrcat(meta,UClit(STR)) ;
	
	if (idName != NULL) { CATL("id:'") CAT(idName) CATL("',") } ;
	cssX = UNUSED ;
	if (className != NULL) { CATL("class:'") CAT(className) CATL("',") }
	 else { cssX = (cell->useColGroup ? UNUSED : vrhm->vdi[cell->dimNum]->cssX) ;
		if (cssX != UNUSED && cssX != vrhm->vce->muX && (cell->cellValue == NULL ? FALSE : UCnotempty(cell->cellValue)))
		 { UCCHAR cb[32] ; UCsprintf(cb,32,UClit("cx:%d,"),cssX) ; CAT(cb) ; } ;
	      } ;
	if (styleInfo != NULL) { CATL("style:'") CAT(styleInfo) CATL("',") } ;
	if (noteInfo != NULL) { CATL("title:'") CAT(noteInfo) CATL("',") } ;

/*	Create set of formatting attributes (similar to CSS but as javascript object elements) */
#define JCAT(STR) UCstrcat(jAttr,STR) ;
#define JCATL(STR) UCstrcat(jAttr,UClit(STR)) ;
#define HVALIGN(name) case V4SS_Align_##name: JCATL(#name) ; break ;
	if (vrhm->vdi[cell->dimNum]->vfi != NULL)
	 { UCCHAR jAttr[1024] ; UCstrcpy(jAttr,UClit("{")) ;
	   if (vrhm->vdi[cell->dimNum]->vfi->horAlign > 0)
	    { JCATL("hAlign:'")
	      switch(vrhm->vdi[cell->dimNum]->vfi->horAlign)
	       { default:		printf("?Unknown V4SS_Align_xxx code(%d) in horAlign\n",vrhm->vdi[cell->dimNum]->vfi->horAlign) ;
	         HVALIGN(Left) HVALIGN(Right) HVALIGN(Center) HVALIGN(Justify) HVALIGN(Fill) HVALIGN(MultiColCenter) HVALIGN(MultiColLeft) HVALIGN(MultiColRight) HVALIGN(Indent1) HVALIGN(Indent2) HVALIGN(Indent3)
	       } ;
	      JCATL("',")
	    } ;
	   if (vrhm->vdi[cell->dimNum]->vfi->vertAlign > 0)
	    { JCATL("vAlign:'")
	      switch(vrhm->vdi[cell->dimNum]->vfi->vertAlign)
	       { default:		printf("?Unknown V4SS_Align_xxx code(%d) in vertAlign\n",vrhm->vdi[cell->dimNum]->vfi->horAlign) ;
	         HVALIGN(Top) HVALIGN(Bottom)
	       } ;
	      JCATL("',")
	    } ;
#define FONT(ATTR) if (vrhm->vdi[cell->dimNum]->vfi->fontAttr & V4SS_Font_##ATTR) { JCATL(#ATTR) JCATL(":true,") ; } ;
	   if (vrhm->vdi[cell->dimNum]->vfi->fontAttr > 0)
	    { FONT(Italic) FONT(Underline) FONT(StrikeThru) FONT(Outline) FONT(Shadow) FONT(Wrap) FONT(SizeToFit) FONT(SubScript) FONT(SuperScript) FONT(NoBreak) FONT(Bold) FONT(UnderlineDbl) FONT(URL) } ;
	   if (vrhm->vdi[cell->dimNum]->vfi->bgColor != 0)
	    { JCATL("bgColor:'") JCAT(v_ColorRefToHTML(vrhm->vdi[cell->dimNum]->vfi->bgColor)) JCATL("',") } ;
	   if (vrhm->vdi[cell->dimNum]->vfi->textColor != 0)
	    { JCATL("textColor:'") JCAT(v_ColorRefToHTML(vrhm->vdi[cell->dimNum]->vfi->textColor)) JCATL("',") } ;
	   if (vrhm->vdi[cell->dimNum]->vfi->fontName == NULL ? FALSE : UCnotempty(vrhm->vdi[cell->dimNum]->vfi->fontName))
	    { JCATL("fontName:'") JCAT(vrhm->vdi[cell->dimNum]->vfi->fontName) ; JCATL("',") } ;
	   if (vrhm->vdi[cell->dimNum]->vfi->fontSize > 0)
	    { UCCHAR fs[32] ; UCsprintf(fs,UCsizeof(fs),UClit("fontSize:%d,"),vrhm->vdi[cell->dimNum]->vfi->fontSize) ; JCAT(fs)
#define BORDER(ATTR) if (vrhm->vdi[cell->dimNum]->vfi->border == V4SS_Border_##ATTR) { JCATL(#ATTR) JCATL(":true,") ; } ;
	      BORDER(None) BORDER(Thin) BORDER(Medium) BORDER(Dashed) BORDER(Dotted) BORDER(Thick) BORDER(Double) BORDER(Hairline)
	    } ;
	   if (jAttr[UCstrlen(jAttr)-1] == UClit(',')) { jAttr[UCstrlen(jAttr)-1] = UCEOS ; } ;
	   UCstrcat(jAttr,UClit("}")) ;
/*	   Now update css structure so we can use same cx:index element to get CSS and/or this info */
	   if (cssX != UNUSED)
	    { struct V4RH__CSSEntries *vce = vrhm->vce ;
	      if (vce->css[cssX].jsonAttr == NULL) vce->css[cssX].jsonAttr = v4mm_AllocUC(UCsizeof(jAttr)) ;
	      UCstrcpy(vce->css[cssX].jsonAttr,jAttr) ;
	    } else
	    { UCstrcpy(meta,jAttr) ;			/* Don't have CSS index? */
	    } ;
	 } ;

	if (eventInfo != NULL)
	 { CATL(" ") CAT(eventInfo) } ;
	if (*colSpan != 0) { UCsprintf(tbuf,UCsizeof(tbuf),UClit("colspan:%d,"),*colSpan) ; CAT(tbuf) } ;

	if (urlInfo != NULL)
	 { UCCHAR urlLit[1024] ;
	   v_ParseInt(urlInfo,&tc,10) ;
	   if (*tc == UClit(':'))
	    { UCCHAR urlLit[1024] ;
	      CATL("url:[") *tc = UCEOS ; CAT(urlInfo) *tc = UClit(':') ;
	      CATL(",") v_StringLit(tc+1,urlLit,UCsizeof(urlLit),UClit('\''),0) ; CAT(urlLit)
	      CATL(",'") ; if (target != NULL) { CAT(target) ; } ; CATL("'") ;
	      CATL("]")
	    } else 
	    { CATL("url:") v_StringLit(urlInfo,urlLit,UCsizeof(urlLit),UClit('\''),0) ; CAT(urlLit)
	    } ;
	   CATL(",")
	 } ;

	if (meta[UCstrlen(meta)-1] == UClit(',')) { meta[UCstrlen(meta)-1] = UCEOS ; } ;

/*	Now handle the actual cell value */
	if (cell->cellValue == NULL)
	 cell->cellValue = UClit("") ;	/* If no value then default to empty string */
/*	If we have an image for the cell, then it trumps any value we may also have */
	switch (imgURL != NULL ?  -1 : (cell->pntType == UNUSED ? vdi->v4PntType : cell->pntType))
	 { default:
		v_Msg(NULL,vrhm->errMessage,"@Invalid point type (%1d) cell (%2U)",vdi->v4PntType,cell->cellValue) ; return(FALSE) ;
	   case -1:			/* Here to handle image */
		UCstrcat(cv,UClit("<img src='")) ; UCstrcat(cv,imgURL) ; UCstrcat(cv,UClit("'/>")) ;
		break ;
//	   case V4DPI_PntType_MIDASUDT:
//		inum = v_ParseInt(cell->cellValue,&tc,10) ;
//		cal = UDTtoCAL(inum) ;
//		v_FormatDate(&cal,V4DPI_PntType_Calendar,VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,mask,cv) ;
//		break ;
	   case V4DPI_PntType_UDate:
	   case V4DPI_PntType_UDT:
	   case V4DPI_PntType_Calendar:
		cal = v_ParseDbl(cell->cellValue,&tc) ;
		v_FormatDate(&cal,V4DPI_PntType_Calendar,VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,mask,cv) ;
		break ;
	   case V4DPI_PntType_UYear:
	   case V4DPI_PntType_Int:
	   case V4DPI_PntType_Delta:
		inum = v_ParseInt(cell->cellValue,&tc,10) ;
		v_FormatInt(inum,mask,cv,rgbColor) ;
		break ;
	   case V4DPI_PntType_Logical:
//		logical = v_ParseInt(cell->cellValue,&tc,10) > 0 ;
		logical = v_ParseLog(cell->cellValue,&tc) ;
		UCstrcat(cv,(logical ? UClit("Yes") : UClit("No"))) ;
		break ;
	   case V4DPI_PntType_Real:
		dbl = v_ParseDbl(cell->cellValue,&tc) ;
		if (fabs(dbl) < vrhm->epsilon) dbl = 0 ;
		v_FormatDbl(dbl,mask,cv,rgbColor) ;
		break ;
	   case V4DPI_PntType_BinObj:
	   case V4DPI_PntType_UOM:
	   case V4DPI_PntType_UOMPer:
	   case V4DPI_PntType_UTime:
	   case V4DPI_PntType_UMonth:
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
		  eb = cv ;
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
	
	return(TRUE) ;
}

void v4j_OutputSection(vrhm,sheet,firstRow,secName)
  struct V4RH__Main *vrhm ;
  struct V4RH__Sheet *sheet ;
  struct V4RH__Row *firstRow ;
  UCCHAR *secName ;
{
  struct V4RH__Row *row ; struct V4RH__Cell *cell ; struct V4RH__Column *col ;
  UCCHAR cv[0x10000], *meta[V4RJ_MAXCOLUMNS] ; COUNTER colSpan ;
  UCCHAR tb[V4LEX_Tkn_InpLineMax] ; INDEX colX, i ;

	for(i=0;i<V4RJ_MAXCOLUMNS;i++) { meta[i] = NULL ; } ;
	OJL("\n	") OJ(secName) OJL(":[")
	for(row=firstRow;row!=NULL;row=row->rowNext)
	 { INDEX colLastMX ;
	   v4rh_SetCurRow(vrhm,row,V4RH_RowType_Data,NULL) ;
	   if (row->skipRow) continue ;
	   OJL("\n		{data:[")
	   for(colX=0,cell=row->cellFirst,col=sheet->colFirst;cell!=NULL;cell=cell->cellNext,col=col->colNext,colX++)
	    { if (colX > 0) { OJL(",") ; } ;
	      INITMC(colX)
	      v4j_CreateCell(vrhm,cell,cv,meta[colX],&colSpan) ;
	      v_StringLit(cv,tb,UCsizeof(tb),UClit('\''),0) ; OJ(tb) ;
	      
	      for(;colSpan>1;colSpan--)
	       { colX++ ; INITMC(colX) } ;
	    } ;
	   OJL("]")
	   if (row->rowKey != NULL)
	    { OJL(",rowKey:") v_StringLit(row->rowKey,tb,UCsizeof(tb),UClit('\''),0) ; OJ(tb) ; } ;
/*	   Did all the values, now the meta information */
	   for(colLastMX=UNUSED,i=0;i<colX;i++)		/* colLastMX = last column seen with meta info */
	    { if (UCnotempty(meta[i])) { colLastMX = i ; } ; } ;
	   if (colLastMX == UNUSED)				/* If all the meta entries are blank then don't bother with the array */
	    { 
	    } else
	    { OJL(",meta:[")
	      for(i=0;i<=colLastMX;i++)			/* If prior column was last with meta then don't bother with the rest */
	       { if (UCempty(meta[i]))			/* If no meta info in this column then just output null as placeholder */
		  { if (i > 0) { OJL(",null") } else { OJL("null") } ; }
		  else { if (i > 0) { OJL(",{") } else { OJL("{") ; } ; OJ(meta[i]) ; OJL("}") } ;
	       } ;
	      OJL("]")				/* This ends the meta-info array */
	    } ;
	   OJL("}")				/* This ends the row array */
	   if (row->rowNext) { OJL(",") } ;
	 } ; OJL("\n	    ],")		/* This ends the row:[] main array */
	for(i=0;i<V4RJ_MAXCOLUMNS;i++)
	 { if (meta[i] != NULL) v4mm_FreeChunk(meta[i]) ; } ;
}

void v4j_OutputMeta(vrhm)
  struct V4RH__Main *vrhm ;
{
  struct V4RH__NamedEntry *vrne ;
  INDEX i,maxi ;

	vrne =  v4rh_neLookup(vrhm,UClit("jobId")) ; if (vrne != NULL) { OJL("\n jobId:'") OJ(vrne->eVal) OJL("',") } ;

	for(i=0,maxi=UNUSED;i<V4RH_URLBASE_MAX;i++)
	 { if (vrhm->urlBases[i] != NULL) maxi = i ; } ;
	if (maxi != UNUSED) 
/*	   Got at least one base - have to set up js array & function */
	 { OJL("\n urlBases:[") ;
	   for (i=0;i<=maxi;i++)
	    { if (i > 0) OJL(",")
	      if (vrhm->urlBases[i] == NULL) { OJL("null") ; }
	       else { OJ(vrhm->urlBases[i]) ; } ;
	    } ; OJL("],")
	 } ;
/*	Do we have any cookie values to set ? */
	vrne = v4rh_neLookup(vrhm,UClit("cookieValues")) ;
	if (vrne != NULL)
	 { UCCHAR *b1,*b2 ;
	   OJL("\n cookies:[")
	   for(b1 = vrne->eVal;*b1!=UCEOS;b1=b2)
	    { b2 = UCstrpbrk(b1,UClit(";") UCEOLbts) ;				/* Break on ';' or return */
	      if (b2 == NULL) { b2 = UClit("") ; } else { *b2 = UCEOS ; b2++ ; } ;
	      OJL("'") OJ(b1) OJL("'")
	    } ;
	   OJL("],")
	 } ;
}

LOGICAL v4j_GenerateJSON(vrhm,toWWW)
  struct V4RH__Main *vrhm ;
  LOGICAL toWWW ;
{
  struct V4RH__Sheet *sheet ; struct V4RH__Row *row ; struct V4RH__Cell *cell ; struct V4RH__Column *col ;
  UCCHAR tb[V4LEX_Tkn_InpLineMax] ; INDEX colX, i ;

/*	Open up the output file */
	if ((vrhm->outFileId = vout_OpenStreamFile(NULL,vrhm->outFile,UClit("ajax"),NULL,FALSE,vrhm->outEncoding,0,vrhm->errMessage)) == UNUSED) return(FALSE) ;
	vrhm->outFileX = vout_FileIdToFileX(vrhm->outFileId) ;
/*	Generating HTTP Headers ? */
/*	If going back to WWW client then add some necessary HTTP headers */
	if (toWWW)
	 { vout_UCTextFileX(vrhm->outFileX,0,UClit("Content-Type: application/x-javascript\n") UClit("Expires: -1\n") UClit("Cache-Control: max-age=1\n") UClit("\n")) ;
	 } ;

/*	Dump out JSON 'header' */
	if (vrhm->gotError)
	 { struct V4RH__NamedEntry *vrne ; UCCHAR qmsg[512] ;
	   vrne =  v4rh_neLookup(vrhm,UClit("errMessage")) ;
	   OJL("{meta:{status:'error', msg:")
	   if (vrne != NULL) { OJ(vrne->eVal) }
	    else { v_StringLit(vrhm->errMessage,qmsg,UCsizeof(qmsg),UClit('\''),0) ; OJ(qmsg) } ;
	   OJL("  }")
	   vrne =  v4rh_neLookup(vrhm,UClit("jobId")) ; if (vrne != NULL) { OJL("\n ,jobId:'") OJ(vrne->eVal) OJL("'") } ;
	   OJL("  \n}")
	   vout_CloseFile(vrhm->outFileId,UNUSED,vrhm->errMessage) ;
	   return(TRUE) ;
	 } ;
	OJL("{meta:{status:'ok'},")
	v4j_OutputMeta(vrhm) ;
	OJL("\n sheets:[") ;

/*	Iterate through entire report */
	for (sheet=vrhm->sheetFirst;sheet!=NULL;sheet=sheet->sheetNext)
	 { INDEX tx ;
	   struct V4RH__CSSEntries *vce = vrhm->vce ; COUNTER maxVal ;
/*	   Start new sheet object */
	   UCstrcpy(tb,UClit("\n  {sheetName:'")) ; UCstrcat(tb,sheet->sheetName) ; UCstrcat(tb,UClit("',")) ; OJ(tb) ;
/*	   Go through sheet and count how many times each CSS entry is referenced */
	   for (i=0,vce->muX=0;i<vce->numCSS;i++) { vce->css[i].refCount = 0 ; } ;
	   for(row=sheet->rowFirst;row!=NULL;row=row->rowNext)
	    { if (row->skipRow) continue ;
	      for(colX=0,cell=row->cellFirst,col=sheet->colFirst;cell!=NULL;cell=cell->cellNext,col=col->colNext)
	       { INDEX cssX = (cell->useColGroup ? UNUSED : vrhm->vdi[cell->dimNum]->cssX) ;
		 if (cell->cellValue == NULL ? FALSE : UCnotempty(cell->cellValue)) vrhm->vce->css[cssX].refCount++ ;
	       } ;
	    } ;
/*	   Get index for most used CSS entry */
	   for (i=0,maxVal=0;i<vce->numCSS;i++)
	    { if (vce->css[i].refCount > maxVal) { maxVal = vce->css[i].refCount ; vrhm->vce->muX = i ; } ; } ;
	   UCsprintf(tb,UCsizeof(tb),UClit("\n	defaultCSS:%d,"),vrhm->vce->muX) ; OJ(tb) ;
/*	   Dump out all titles */
	   OJL("\n	titles:[")
	   for(tx=0;tx<V4RH_Max_Titles;tx++)
	    { if (sheet->ucTitles[tx] == NULL) break ;
	      if (tx > 0) OJL(",") ;
	      v_StringLit(sheet->ucTitles[tx],tb,UCsizeof(tb),UClit('\''),0) ; vout_UCTextFileX(vrhm->outFileX,0,tb) ;
	    } ; OJL("],")
/*	   Any row selection info ? */
	   if (sheet->selectInfo != NULL)
	    { OJL("\n	selectInfo:") ; v_StringLit(sheet->selectInfo,tb,UCsizeof(tb),UClit('\''),0) ; vout_UCTextFileX(vrhm->outFileX,0,tb) ; OJL(",") ;
	    } ;
/*	   If we have column Ids for this sheet then output them */
	   if (sheet->colIds != NULL)
	    { UCCHAR *s,*e ;
	      OJL("\n	colIds:[")
	      for(s=sheet->colIds;;s=e+1)
	       { e = UCstrchr(s,'|') ; if (e != NULL) *e = UCEOS ;
	         if (UCnotempty(s)) { OJL("'") OJ(s) OJL("'") }
		  else { OJL("null") } ;
		 if (e == NULL) break ;
		 OJL(",")
	       } ; OJL("\n	   ],")
	    } ;
/*	   If we have row keys then output them */
	   { LOGICAL haveIds = FALSE ; COUNTER r ;
	     for(row=sheet->rowFirst;row!=NULL;row=row->rowNext) { if (row->rowKey != NULL) { haveIds = TRUE ; break ; } ; } ;
	     if (haveIds)
	      { OJL("\n	rowKeys:[")
		for(r=1,row=sheet->rowFirst;row!=NULL;row=row->rowNext,r++)
		 { if (row->rowKey == NULL) { OJL("null") }
		    else { v_StringLit(row->rowKey,tb,UCsizeof(tb),UClit('\''),0) ; OJ(tb) } ;
		   if (row->rowNext)
		    { OJL(",") if ((r % 20) == 0) { OJL("\n		") } ; } ;
		 } ; OJL("\n	   ],")
	      } ;
	   }
	   v4j_OutputSection(vrhm,sheet,sheet->hdrFirst,UClit("headers")) ;
	   v4j_OutputSection(vrhm,sheet,sheet->ftrFirst,UClit("footers")) ;
	   v4j_OutputSection(vrhm,sheet,sheet->rowFirst,UClit("rows")) ;

/*	   If we havel reporting level information then output it now */
	   { UCCHAR ilb[32] ; char isActive[V4IM_BaA_LevelMax], levelAtEnd[V4IM_BaA_LevelMax] ; INDEX highest, rowNum, hx ;
	     memset(isActive,FALSE,V4IM_BaA_LevelMax) ; highest = -1 ;
	     for(rowNum=0,row=sheet->rowFirst;row!=NULL;row=row->rowNext,rowNum++)
	      { if (row->level <= 0 || row->level >= V4IM_BaA_LevelMax) continue ;
	        isActive[row->level] = TRUE ; if (row->level > highest) highest = row->level ;
		levelAtEnd[row->level] = row->levelAtEnd ;
	      } ;
	     if (highest >= 0)
	      { LOGICAL first ; UCCHAR hbuf[32] ;
	        OJL("\n	hierarchy:[null,")
		for(hx=1;hx<=highest;hx++)
		 { OJL("\n	  [")
		   first = TRUE ;
		   for(rowNum=0,row=sheet->rowFirst;row!=NULL;row=row->rowNext,rowNum++)
		    { if (row->level != hx) continue ;
		      UCsprintf(hbuf,UCsizeof(hbuf),(first ? UClit("%d") : UClit(",%d")),rowNum) ; OJ(hbuf)
		      first = FALSE ;
		    } ;
		   OJL("]") if (hx < highest) { OJL(",") } ;
		 } ;
		OJL("\n	  ],")
	      } ;
	     OJL("\n	levelAtEnd:[")
	     for(hx=1;hx<=highest;hx++) { if (levelAtEnd[hx]) { OJL("true") } else { OJL("false") } ; if (hx < highest) { OJL(",") } ; } ;
	     OJL("],")
	     UCsprintf(ilb,UCsizeof(ilb),UClit("\n	initialLevel:%d"),sheet->initialLevel) ; OJ(ilb) ;
	   }
/*	   Finish off sheet object */
	   OJL("\n    }") ; if (sheet->sheetNext != NULL) { OJL(",") } ;
	 } ;
	OJL("\n   ],")		/* End of sheet array */
/*	Dump out CSS definitions */
	if (vrhm->vce != NULL)
	 { struct V4RH__CSSEntries *vce ;		/* Pointer to CSS entries */
	   vce = vrhm->vce ;
	   OJL("\n css:[")
	   for(i=0;i<vce->numCSS;i++)
	    { if (i > 0) OJL(",")
	      OJL("['") OJ(vce->css[i].className) OJL("',")
	      v_StringLit(vce->css[i].cssDef,tb,UCsizeof(tb),UClit('\''),0) ; OJ(tb) ; OJL("]")
	    } ;
	   OJL("],")
	   OJL("\n jAttr:[")
	   for(i=0;i<vce->numCSS;i++)
	    { if (i > 0) OJL(",")
	      if (vce->css[i].jsonAttr == NULL) { OJL("null") ; }
	       else { UCCHAR *b = vce->css[i].jsonAttr ;
/*		      Below is kind of ugly but seems to convert everything to camel-case */
		      for (;*b!=UCEOS;b++) { if (*b == UClit('[') || *b == UClit('\'') || *b == UClit(',')) *(b+1) = UCTOLOWER(*(b+1)) ; } ;
		      OJ(vce->css[i].jsonAttr) ;
		    } ;
	    } ;
	   OJL("]")
	 } ;
/*	Close off the JSON string and then the file */
	OJL("\n}") ;
	vout_CloseFile(vrhm->outFileId,UNUSED,vrhm->errMessage) ;
	return(TRUE) ;
}
