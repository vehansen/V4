#define V4RT_MAXCHARINLINE 1024
#define V4RT_MAXLINESINPAGE 512
#define V4RT_MAXCOLUMNS 256
#define V4RT_TABSIZE 8

  enum rowType { title, heading, detail, footer } ;
struct V4RT__TxtReportInfo {
  COUNTER pages ;
  INDEX header ;
  INDEX detail ;
  enum rowType lines[V4RT_MAXLINESINPAGE] ;
  COUNTER columns ;
  struct {
    INDEX start, end ;		/* Start & end positions for this column */
   } cols[V4RT_MAXCOLUMNS] ;
} ;

LOGICAL v4rt_ScopeOutTxtReport(vrhm)
  struct V4RH__Main *vrhm ;
{
  enum rowType curLine ;
  LOGICAL topOfPage ; COUNTER line, totalLines ;
  COUNTER numSpace[V4RT_MAXCHARINLINE], numNumeric[V4RT_MAXCHARINLINE], numAlpha[V4RT_MAXCHARINLINE], numUL[V4RT_MAXCHARINLINE] ;
  struct V4RT__TxtReportInfo vtr ;

	memset(&vtr,0,sizeof vtr) ;
	memset(&numSpace,0,sizeof numSpace) ; memset(&numNumeric,0,sizeof numNumeric) ; memset(&numAlpha,0,sizeof numAlpha) ; memset(&numUL,0,sizeof numUL) ;
	if (!v_UCFileOpen(&vrhm->iFile,vrhm->inpFile,UCFile_Open_Read,TRUE,vrhm->errMessage,0)) return(FALSE) ;
	topOfPage = TRUE ; curLine = title ;
	line = 0 ; vtr.pages = 0 ; totalLines = 0 ;
	for(line=1;;line++)
	 { UCCHAR line[V4RT_MAXCHARINLINE] ; INDEX col,i ; COUNTER numULsInLine ;
	   if (v_UCReadLine(&vrhm->iFile,UCRead_UC,line,UCsizeof(line),vrhm->errMessage) < 0) break ;
	   totalLines++ ;
	   if (topOfPage && vtr.pages == 0)
	    { if (vrhm->iFile.lineTerm == UCFile_Term_CR)
	       { curLine = heading ; if (vtr.header == 0) vtr.header = curLine ; } ;
	      if (curLine == title && UCempty(line))
	       { curLine = heading ; continue ; } ;
	      if (curLine == heading && UCempty(line))
	       { curLine = detail ; if (vtr.detail == 0) vtr.detail = curLine + 1 ; continue ; } ;
	    } ;
	   if (curLine < V4RT_MAXLINESINPAGE) vtr.lines[curLine] = curLine ;
/*	   numULsInLine = number of underlines in current line. If below minimum then don't count them as underlines in header */
	   for(i=0,numULsInLine=0;line[i]!=UCEOS;i++) { if (line[i] == UClit('_')) numULsInLine++ ; } ;
/*	   Count the number of spaces, etc. that occur in each column of text report */
	   for(col=0,i=0;line[i]!=UCEOS;i++,col++)
	    { if (line[i] == '\t')
	       { INDEX tab = (((col / V4RT_TABSIZE) + 1) * V4RT_TABSIZE)  ;	/* tab = next tab stop */
	         for(;col < tab;col++) numSpace[col]++ ;
	       } else if (vuc_IsWSpace(line[i])) { numSpace[col]++ ;
	       } else if (vuc_IsDigit(line[i])) { numNumeric[col]++ ;
	       } else { switch(line[i])
			 { default:		numAlpha[col]++ ; break ;
			   case UClit('_'):	if (numULsInLine > 10) numUL[col]++ ; break ;
			   case UClit('-'):	if (vuc_IsDigit(line[i+1]) || (i > 0 ? vuc_IsDigit(line[i-1]) : FALSE)) { numNumeric[col]++ ; } else { numAlpha[col]++ ; } ; break ;
			   case UClit('.'):
			   case UClit(','):	if (vuc_IsDigit(line[i+1])) { numNumeric[col]++ ; } else { numAlpha[col]++ ; } ; break ;
			 } ;
		      } ;
	    } ;
	   switch(vrhm->iFile.lineTerm)
	    { default:			break ;
	      case  UCFile_Term_NL:	curLine++ ; break ;
	      case  UCFile_Term_CR:
	      case  UCFile_Term_CRLF:	curLine++ ; break ;
	      case  UCFile_Term_FF:	topOfPage = TRUE ; vtr.pages++ ; curLine = 0 ; break ;
	      case  UCFile_Term_VT:
	      case  UCFile_Term_EOB:	break ;
	    } ;
	 } ;
/*	Gone through file - figure out where columns are */
	printf("pages=%d, heading = %d, detail = %d\n",vtr.pages,vtr.header,vtr.detail) ;
/*	If we got underlines then use them on the assumption that the column headings were underlined */
	{ COUNTER uls = 0 ; INDEX i ;
	  for(i=0;i<V4RT_MAXCHARINLINE;i++) { uls += numUL[i] ; } ;
	  if (uls > 20 * vtr.pages)
	   { COUNTER minUL ;
	     minUL = vtr.pages ;				/* minUL is minimum number to consider significant (could have spurious underline in data) */
	     for(i=0;i<V4RT_MAXCHARINLINE;i++)
	      { if (numUL[i] < minUL) continue ;		/* Look for begin of first column, ignore possible false positives */
	        vtr.cols[vtr.columns].start = i ;
		for(;i<V4RT_MAXCHARINLINE;i++) { if (numUL[i] < minUL) break ; } ;
		vtr.cols[vtr.columns].end = i-1 ;
		vtr.columns++ ;
	      } ;
	   } else
	   { INDEX col,i ;
	   } ;
	}
	printf("Found %d columns\n",vtr.columns) ;
	{ INDEX i ;
	  for(i=0;i<vtr.columns;i++) { printf(" #%d %d..%d\n",i,vtr.cols[i].start,vtr.cols[i].end) ; } ;
	} ;

	return(TRUE) ;
}