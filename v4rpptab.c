/*	Here to generate tab text file from parsed report specs */

#define OT(STR) vout_UCTextFileX(vrhm->outFileX,0,STR) ;
#define OTL(STRLIT) vout_UCTextFileX(vrhm->outFileX,0,UClit(STRLIT)) ;


LOGICAL v4t_CreateCell(vrhm,cell,cv,meta,colSpan)
  struct V4RH__Main *vrhm ;
  struct V4RH__Cell *cell ;
  UCCHAR *cv, *meta ;
  COUNTER *colSpan ;
{ struct V4RH__DimInfo *vdi ;
  UCCHAR *tc,*mask ;
  static INDEX idnCount = 0 ;
  int inum ; double dbl ; LOGICAL logical ; CALENDAR cal ;

	vdi = (cell->dimNum >= 0 && cell->dimNum < V4RH_DIM_MAX ? vrhm->vdi[cell->dimNum] : NULL) ;
/*	If vdi undefined then default with generic alpha */
	if (vdi == NULL)
	 { if (++idnCount < 10) printf("[Invalid dimension number (%d) encountered in cell, defaulting to Alpha]\n",cell->dimNum) ;
	   vdi = vrhm->vdi[cell->dimNum=V4RH_DimNum_Alpha] ;
	 } ;
	mask = (vdi->maskX == UNUSED ? NULL : vrhm->vmi->mask[vdi->maskX].formatMask) ;
/*	Update cv with cell value */
	ZUS(cv)
	if (cell->cellValue == NULL)
	 cell->cellValue = UClit("") ;	/* If no value then default to empty string */
	switch (cell->pntType == UNUSED ? vdi->v4PntType : cell->pntType)
	 { default:
		v_Msg(NULL,vrhm->errMessage,"@Invalid point type (%1d) cell (%2U)",vdi->v4PntType,cell->cellValue) ; return(FALSE) ;
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
		v_FormatInt(inum,mask,cv,NULL) ;
		break ;
	   case V4DPI_PntType_Logical:
//		logical = v_ParseInt(cell->cellValue,&tc,10) > 0 ;
		logical = v_ParseLog(cell->cellValue,&tc) ;
		UCstrcat(cv,(logical ? UClit("Yes") : UClit("No"))) ;
		break ;
	   case V4DPI_PntType_Real:
		dbl = v_ParseDbl(cell->cellValue,&tc) ;
		if (fabs(dbl) < vrhm->epsilon) dbl = 0 ;
		v_FormatDbl(dbl,mask,cv,NULL) ;
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

void v4t_OutputSection(vrhm,sheet,firstRow,secName)
  struct V4RH__Main *vrhm ;
  struct V4RH__Sheet *sheet ;
  struct V4RH__Row *firstRow ;
  UCCHAR *secName ;
{
  struct V4RH__Row *row ; struct V4RH__Cell *cell ; struct V4RH__Column *col ;
  UCCHAR cv[0x10000], tb[0x10000], *meta[V4RJ_MAXCOLUMNS] ; COUNTER colSpan ;
  INDEX colX, i ;

	for(i=0;i<V4RJ_MAXCOLUMNS;i++) { meta[i] = NULL ; } ;
	for(row=firstRow;row!=NULL;row=row->rowNext)
	 { v4rh_SetCurRow(vrhm,row,V4RH_RowType_Data,NULL) ;
	   if (row->skipRow) continue ;
	   for(colX=0,cell=row->cellFirst,col=sheet->colFirst;cell!=NULL;cell=cell->cellNext,col=col->colNext,colX++)
	    { if (colX > 0) { if (vrhm->outFormat == csv) { OTL(",") ; } else { OTL("\t") ; } ; } ;
	      INITMC(colX)
	      v4t_CreateCell(vrhm,cell,cv,meta[colX],&colSpan) ;
	      if (vrhm->outFormat == csv)
	       { UCCHAR *b ; LOGICAL doQuote = FALSE ;
/*		 Have to check for embedded double quote/line-break/comma */
		 for(b=cv;*b!=UCEOS;b++)
		  { if (*b <= 26 || *b == UClit('"') || *b == UClit(','))
		     { doQuote = TRUE ; break ; } ;
		  } ;
		 if (!doQuote) { OT(cv) }
	          else { v_StringLit(cv,tb,UCsizeof(tb),UClit('"'),UClit('"')) ; OT(tb) ; } ;
	       } else
	       { OT(cv) ;
	       } ;
	    } ;
	   OTL("\n")
	   
	 }
}


LOGICAL v4t_GenerateTABCSV(vrhm)
  struct V4RH__Main *vrhm ;
{
  struct V4RH__Sheet *sheet ;

/*	Open up the output file */
	if ((vrhm->outFileId = vout_OpenStreamFile(NULL,vrhm->outFile,UClit("txt"),NULL,FALSE,vrhm->outEncoding,0,vrhm->errMessage)) == UNUSED) return(FALSE) ;
	vrhm->outFileX = vout_FileIdToFileX(vrhm->outFileId) ;

/*	Iterate through entire report */
	for (sheet=vrhm->sheetFirst;sheet!=NULL;sheet=sheet->sheetNext)
	 { 
	   v4t_OutputSection(vrhm,sheet,sheet->hdrFirst,UClit("headers")) ;
	   v4t_OutputSection(vrhm,sheet,sheet->ftrFirst,UClit("footers")) ;
	   v4t_OutputSection(vrhm,sheet,sheet->rowFirst,UClit("rows")) ;
	 } ;
	vout_CloseFile(vrhm->outFileId,UNUSED,vrhm->errMessage) ;
	return(TRUE) ;
}
