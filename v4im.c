/*	V4IM.C - Internal Module Routines for V4

	Created 31-Mar-92 by Victor E. Hansen				*/

#define V4IMMain
#include "v4imdefs.c"
extern struct V4C__ProcessInfo *gpi ;	/* Global process information */

DIMID Dim_Dim=0,Dim_Logical,Dim_List,Dim_Int,Dim_Int2,Dim_Alpha,Dim_Num,Dim_UCal,Dim_UDate,Dim_NId,Dim_UV4,Dim_UDT,Dim_UDelta,Dim_IntMod,Dim_Tag,Dim_UFix,Dim_UTree ;
DIMID Dim_UMonth,Dim_UTime,Dim_UYear,Dim_UWeek,Dim_UQuarter,Dim_UPeriod,Dim_Isct,Dim_UDim,Dim_UBLOB,Dim_UCountry,Dim_UCChar,Dim_UDB,Dim_UTele ;
DIMID Dim_UMacro,Dim_UTable,Dim_UArray ;

DIMID Dim_UDBArgs,Dim_UDBSpec ;

struct V4DPI__LittlePoint protoInt, protoDbl, protoDict, protoAlpha, protoLog, protoNone, protoQNone, protoUCChar, protoFix, protoSkip, protoEQ, protoEvalStream, protoTree, protoNull ;
#ifdef NEWQUOTE
DIMID Dim_UQuote ;
struct V4DPI__LittlePoint protoQuote ;
#endif
//struct V4DPI__LittlePoint protoForeign ;
struct V4DPI__LittlePoint Log_True,Log_False ;
extern ETYPE traceGlobal ;
V4DEBUG_SETUP
int eclCnt ;
double eclArray[1000] ;
extern struct V__UnicodeInfo *uci ;	/* Global structure of Unicode Info */

/*	NOTE: Returns FALSE if OK, TRUE if failed! */
LOGICAL v4im_InitConstVals(ctx)
 struct V4C__Context *ctx ;
{
  enum DictionaryEntries deval ;
#define ICV(NAME) Dim_##NAME = v4dpi_DimGet(ctx,UClit(#NAME),DIMREF_IRT) ;

	if (Dim_Dim != 0) return(FALSE) ;	/* Already done */
	ICV(Dim) if (Dim_Dim <= 0) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNoKernel") ; return(TRUE) ; } ;
	ICV(Logical) ICV(Num) ICV(NId) ICV(UV4) ICV(List) ICV(Int) ICV(Int2) ICV(Alpha)	ICV(UDelta) ICV(IntMod) ICV(Tag) ICV(Isct) ICV(UDim) ICV(UBLOB)	ICV(UMonth) ICV(UTime) ICV(UYear)
	ICV(UWeek) ICV(UQuarter) ICV(UPeriod) ICV(UCountry) ICV(UFix) ICV(UDB) ICV(UDB) ICV(UTable) ICV(UMacro) ICV(UArray) ICV(UCal) ICV(UDT) ICV(UDate) ICV(UTele) ICV(UTree)
#ifdef USE_NEW_XDB
	ICV(UDBArgs) ICV(UDBSpec)
#endif
	Dim_UCChar = v4dpi_DimGet(ctx,UClit("Unicode"),DIMREF_IRT) ;
#ifdef NEWQUOTE
	ICV(UQuote)
#endif
	
//	if (Dim_Dim != 0) return(FALSE) ;	/* Already done */
//	Dim_Dim = v4dpi_DimGet(ctx,UClit("Dim"),DIMREF_IRT) ;
//	if (Dim_Dim <= 0) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNoKernel") ; return(TRUE) ; } ;
//	Dim_Logical = v4dpi_DimGet(ctx,UClit("Logical"),DIMREF_IRT) ; Dim_Num = v4dpi_DimGet(ctx,UClit("Num"),DIMREF_IRT) ; Dim_NId = v4dpi_DimGet(ctx,UClit("NId"),DIMREF_IRT) ; Dim_UV4 = v4dpi_DimGet(ctx,UClit("UV4"),DIMREF_IRT) ;
//	Dim_List = v4dpi_DimGet(ctx,UClit("List"),DIMREF_IRT) ; Dim_Int = v4dpi_DimGet(ctx,UClit("Int"),DIMREF_IRT) ; Dim_Int2 = v4dpi_DimGet(ctx,UClit("Int2"),DIMREF_IRT) ; Dim_Alpha = v4dpi_DimGet(ctx,UClit("ALPHA"),DIMREF_IRT) ;
//	Dim_UDelta = v4dpi_DimGet(ctx,UClit("UDelta"),DIMREF_IRT) ; Dim_IntMod = v4dpi_DimGet(ctx,UClit("IntMod"),DIMREF_IRT) ; Dim_Tag = v4dpi_DimGet(ctx,UClit("Tag"),DIMREF_IRT) ;
//	Dim_Isct = v4dpi_DimGet(ctx,UClit("Isct"),DIMREF_IRT) ; Dim_UDim = v4dpi_DimGet(ctx,UClit("UDim"),DIMREF_IRT) ; Dim_UBLOB = v4dpi_DimGet(ctx,UClit("UBLOB"),DIMREF_IRT) ;
//	Dim_UMonth = v4dpi_DimGet(ctx,UClit("UMonth"),DIMREF_IRT) ; Dim_UTime = v4dpi_DimGet(ctx,UClit("UTime"),DIMREF_IRT) ; Dim_UYear = v4dpi_DimGet(ctx,UClit("UYear"),DIMREF_IRT) ; Dim_UWeek = v4dpi_DimGet(ctx,UClit("UWeek"),DIMREF_IRT) ;
//	Dim_UQuarter = v4dpi_DimGet(ctx,UClit("UQuarter"),DIMREF_IRT) ; Dim_UPeriod = v4dpi_DimGet(ctx,UClit("UPeriod"),DIMREF_IRT) ; Dim_UCountry = v4dpi_DimGet(ctx,UClit("UCountry"),DIMREF_IRT) ; Dim_UCChar = v4dpi_DimGet(ctx,UClit("Unicode"),DIMREF_IRT) ;
//	Dim_UFix = v4dpi_DimGet(ctx,UClit("UFix"),DIMREF_IRT) ; Dim_UDB = v4dpi_DimGet(ctx,UClit("UDB"),DIMREF_IRT) ; Dim_UDB = v4dpi_DimGet(ctx,UClit("UTele"),DIMREF_IRT) ;
//	Dim_UTable = v4dpi_DimGet(ctx,UClit("UTable"),DIMREF_IRT) ; Dim_UMacro = v4dpi_DimGet(ctx,UClit("UMacro"),DIMREF_IRT) ; Dim_UArray = v4dpi_DimGet(ctx,UClit("UArray"),DIMREF_IRT) ;
//#ifdef NEWQUOTE
//	Dim_UQuote = v4dpi_DimGet(ctx,UClit("UQuote"),DIMREF_IRT) ;
//#endif

//	memset(&Log_True,0,sizeof(Log_True)) ; Log_True.Dim = Dim_Logical ; Log_True.PntType = V4DPI_PntType_Logical ;
//	Log_True.Bytes = sizeof(Log_True) ; Log_True.Value.IntVal = TRUE ;
//	memset(&Log_False,0,sizeof(Log_False)) ; Log_False.Dim = Dim_Logical ; Log_False.PntType = V4DPI_PntType_Logical ;
//	Log_False.Bytes = sizeof(Log_False) ; Log_False.Value.IntVal = FALSE ;
//	Dim_UCal = v4dpi_DimGet(ctx,UClit("UCal"),DIMREF_IRT) ; Dim_UDT = v4dpi_DimGet(ctx,UClit("UDT"),DIMREF_IRT) ;
//	Dim_UDate = v4dpi_DimGet(ctx,UClit("UDate"),DIMREF_IRT) ;
	ZPH(&protoInt) ; protoInt.Dim = Dim_Int ; protoInt.PntType = V4DPI_PntType_Int ; protoInt.Bytes = V4PS_Int ;
	ZPH(&protoDict) ; protoDict.Dim = Dim_NId ; protoDict.PntType = V4DPI_PntType_Dict ; protoDict.Bytes = V4PS_Int ;
	ZPH(&protoLog) ; protoLog.Dim = Dim_Logical ; protoLog.PntType = V4DPI_PntType_Logical ; protoLog.Bytes = V4PS_Int ;
	ZPH(&protoDbl) ; protoDbl.Dim = Dim_Num ; protoDbl.PntType = V4DPI_PntType_Real ; protoDbl.Bytes = V4PS_Real ;
	ZPH(&protoFix) ; protoFix.Dim = Dim_UFix ; protoFix.PntType = V4DPI_PntType_Fixed ; protoFix.Bytes = V4PS_Fixed ;
	ZPH(&protoAlpha) ; protoAlpha.Dim = Dim_Alpha ; protoAlpha.PntType = V4DPI_PntType_Char ;
	ZPH(&protoUCChar) ; protoUCChar.Dim = Dim_Alpha ; protoUCChar.PntType = V4DPI_PntType_UCChar ;
//	ZPH(&protoForeign) ; protoForeign.Dim = Dim_Num ; protoForeign.PntType = V4DPI_PntType_Foreign ;
	ZPH(&protoNone) ; protoNone.Dim = Dim_UV4 ; protoNone.PntType = V4DPI_PntType_Dict ; protoNone.Bytes = V4PS_Int ;
	 protoNone.Value.IntVal = v4im_GetEnumToDictVal(ctx,deval=_None,Dim_NId) ;
	ZPH(&protoNull) ; protoNull.Dim = Dim_UV4 ; protoNull.PntType = V4DPI_PntType_Dict ; protoNull.Bytes = V4PS_Int ;
	 protoNull.Value.IntVal = v4im_GetEnumToDictVal(ctx,deval=_Null,Dim_NId) ;
	ZPH(&protoSkip) ; protoSkip.Dim = Dim_UV4 ; protoSkip.PntType = V4DPI_PntType_Dict ; protoSkip.Bytes = V4PS_Int ;
	 protoSkip.Value.IntVal = v4im_GetEnumToDictVal(ctx,deval=_Skip,Dim_NId) ;
	ZPH(&protoEQ) ; protoEQ.Dim = Dim_UV4 ; protoEQ.PntType = V4DPI_PntType_Dict ; protoEQ.Bytes = V4PS_Int ;
	 protoEQ.Value.IntVal = v4im_GetEnumToDictVal(ctx,deval=_EQ,Dim_NId) ;
	ZPH(&protoEvalStream) ; protoEvalStream.Dim = Dim_UV4 ; protoEvalStream.PntType = V4DPI_PntType_Dict ; protoEvalStream.Bytes = V4PS_Int ;
	 protoEvalStream.Value.IntVal = v4im_GetEnumToDictVal(ctx,deval=_EvalStream,Dim_NId) ;
	ZPH(&protoTree) ; protoTree.Dim = Dim_UTree ; protoTree.PntType = V4DPI_PntType_Tree ; protoTree.Bytes = V4PS_Tree ;
#ifdef NEWQUOTE
	ZPH(&protoQuote) ; protoQuote.Dim = Dim_UQuote ; protoQuote.PntType = V4DPI_PntType_Shell ; protoQuote.Bytes = V4DPI_PointHdr_Bytes ;
#endif
	protoQNone = protoNone ;
	QUOTE(&protoQNone) ;
	logPNTv(&Log_True,TRUE) ; logPNTv(&Log_False,FALSE) ;
	
	return(FALSE) ;
}



extern int vout_TotalBytes ;

struct V4SS__DimToFormatMap *vdfm=NULL ;

#define UC(DST) { int ucx ; for(ucx=0;DST[ucx]!=0;ucx++) DST[ucx] = toupper(DST[ucx]) ; }
#define RE1(ERRNUM) goto intmod_fail ;
#define ARGERR(ARGX) if (!ok) { ix = ARGX ; goto intmod_fail_arg ; }

/*	v4im_IntModHandler - Called from v4dpi_Eval to handle Internal Module evaluations	*/
/*	Call: point = v4im_IntModHandler( isct, respnt , ctx , evalmode )
	  where point is evaluated point,
		isct is intersection whose first point is IntMod,
		respnt if not NULL is ptr to buffer for result (** NO GUARENTEE OF BEING UPDATED- USED WHEN NEEDED**),
		ctx is current context,
		evalmode is V4DPI_EM_xxx - to control evaluation,
		trace is TRUE to enable tracing/debugging			*/

struct V4DPI__Point *v4im_IntModHandler(isct,respnt,ctx,evalmode)
 struct V4DPI__Point *isct,*respnt ;
 struct V4C__Context *ctx ;
 int evalmode ;
{
  struct V4DPI__DimInfo *di ;
  struct V4DPI__Point *Iipt,*tpt,*ipt,*cpt,*vpt ;
  struct V4DPI__Point isctbuf,ptbuf ;
  struct V4L__ListPoint *lp,*lp1 ;
  struct V4DPI__Point_IntMix *pim ;
  struct V4DPI__Point_RealMix *prm ;
  struct V4DPI__EvalList *el ;
  struct V4IM_DblArray *npv,*npv2 ;			/* Temp structure to hold cash payments for n periods */
  struct V4IM__BaA *baf ;
//  struct V4DPI__WormHoleIntInt *whii ;
//  struct V4IM__XMLNest xml ;
#ifdef WANTECHOF
  struct lcl__echof
   { char format[250] ; 	/* Format string */
     char *args[20] ;		/* Pointers/values of arguments */
     char strbuf[20][250] ;	/* Place holders for string arguments */
   } ; struct lcl__echof *le ;
#endif
int intmodx,argcnt,fail ;
#define V4DPI_IntModArgBuf_Max 8
  struct V4DPI__Point *argpnts[V4DPI_IntModArg_Max] ;
  struct V4DPI__Point argpntbufs[V4DPI_IntModArgBuf_Max] ;
  int argbufcnt=0 ;
//  FILE *fp ;
//  struct UC__File UCFile ;
  static int initialized ; static char minargs[V4IM_OpCode_LastOpcode] ;
  int bytes,num,dimid,type,curcall ; double dnum,dnum1,dnum2 ; B64INT b64,b642 ;
  int i,j,k,ix,pix,resdim,ok,rowdim ; char *bp ;
  VTRACE trace = traceGlobal ;


#ifdef V4_BUILD_RUNTIME_STATS
	int initalClock = clock() ;
#endif

//VEH060521 Pull up first point before loop & grab intmodx
	Iipt = ISCT1STPNT(isct) ; intmodx = Iipt->Value.IntVal ;
#ifdef NASTYBUG
	ctx->isctIntMod = isct ;		/* Save current intmod isct for possible trace */
#endif
	curcall = (++ctx->IsctEvalCount) ;
/*	Is this module restricted? If so then error (always allow Secure() - it checks within itself) */
#ifdef V4_BUILD_SECURITY
	if (gpi->lockModule[intmodx] ? intmodx != V4IM_OpCode_Secure : FALSE) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto intmod_fail ; } ;
#endif
	if (ISQUOTED(isct) && (evalmode & V4DPI_EM_EvalQuote) == 0) return(isct) ; /* Quoted? then return as is */
	if (!initialized)
	 { CHECKV4INIT ;
	   initialized = TRUE ; v4im_InitStuff(minargs) ;
	 } ;
	if (respnt == NULL)				/* If no spot for result then allocate one */
	 { vout_UCText(VOUT_Trace,0,UClit("*respnt arg to intmod is NULL\n")) ;
	   respnt = v4dpi_PntIdx_CvtIdxPtr(v4dpi_PntIdx_AllocPnt()) ;
	 } ;
	argcnt = 0 ;
	for(ix=1;ix<isct->Grouping;ix++)	/* Put all arguments into argpnts array */
	 { ADVPNT(Iipt) ; ipt = Iipt ;
	   if (argcnt >= V4DPI_IntModArg_Max) { v_Msg(ctx,NULL,"MaxNumModArgs",intmodx,V4DPI_IntModArg_Max) ; goto intmod_iarg_fail ; } ;
	   argpnts[++argcnt] = ipt ;			/* Save argument */
	   switch (ipt->ForceEval ? 0 : intmodx)
	    {
//	      case V4IM_OpCode_TQ:
	      case V4IM_OpCode_TEQ:
	      case V4IM_OpCode_BindQQ:
	      case V4IM_OpCode_Do:
	      case V4IM_OpCode_Try:
	      case V4IM_OpCode_MakeLC:
	      case V4IM_OpCode_EchoD:
	      case V4IM_OpCode_db:
	      case V4IM_OpCode_EchoS:
	      case V4IM_OpCode_EchoT:
	      case V4IM_OpCode_Rpt:
		continue ;				/* All evaluations via handler below! */
//	      case V4IM_OpCode_CacheX:
	      case V4IM_OpCode_NDefQ:
	      case V4IM_OpCode_DefQ:
	      case V4IM_OpCode_BindQE:
		if (ix == 1) continue ; 		/* Skip (arg #1) */
		break ;
	      case V4IM_OpCode_BindEQ:
		if (ix == 2) continue ;
		break ;
	      case V4IM_OpCode_Dim:			/* If single argument then quote it */
		if (isct->Grouping == 1) continue ;	/* (argcnt is not yet set - look to isct->Grouping as best guess for argcnt) */
	    } ;
/*	   If an argument is an intersection then evaluate, UNLESS we are getting IsctVal sequences! */
eval_arg:
	   switch(ipt->PntType)
	    { default:
		continue ;
	      case V4DPI_PntType_IMArg:
		if (ipt->Value.IntVal > argcnt)
		 { v_Msg(ctx,NULL,"ModRefAfterArg",intmodx,ipt->Value.IntVal,argcnt) ; goto intmod_iarg_fail ;
		 } ;
		ipt = argpnts[ipt->Value.IntVal] ;	/* Just reference an argument we already have */
		break ;
	      case V4DPI_PntType_List:
		if (!ipt->ForceEval) break ;		/* If `(list) then expand out arguments */
		lp = ALIGNLP(&ipt->Value) ;
		for(j=1;;j++)
		 { if (argbufcnt >= V4DPI_IntModArgBuf_Max)
		    { pix = v4dpi_PntIdx_AllocPnt() ; cpt = v4dpi_PntIdx_CvtIdxPtr(pix) ;
		    } else { cpt = &argpntbufs[argbufcnt++] ; } ;
		   if (v4l_ListPoint_Value(ctx,lp,j,cpt) <= 0) break ;
		   if (argcnt >= V4DPI_IntModArg_Max)
		    { v_Msg(ctx,NULL,"MaxNumModArgsXL",intmodx,ix,argcnt,V4DPI_IntModArg_Max) ; goto intmod_iarg_fail ; } ;
		   argpnts[argcnt++] = cpt ;
		 } ; argcnt-- ;
		continue ;
	      case V4DPI_PntType_QIsct: 		/* Copy and flip to Isct */
		if (argbufcnt >= V4DPI_IntModArgBuf_Max)
		 { pix = v4dpi_PntIdx_AllocPnt() ; cpt = v4dpi_PntIdx_CvtIdxPtr(pix) ;
		 } else { cpt = &argpntbufs[argbufcnt++] ; } ;	/* Allocate temp point area */
		memcpy(cpt,ipt,ipt->Bytes) ; cpt->PntType = V4DPI_PntType_Isct ;
		argpnts[argcnt++] = cpt ;
		break ;
#ifdef NEWQUOTE
	      case V4DPI_PntType_Shell:
		if (!ISQUOTED(ipt)) continue ;
		if (memcmp(ipt,&protoQNone,protoQNone.Bytes) == 0)	/* VEH100606 - Don't unquote @UV4:none since we have several checks throught for it (should do something about this) */
		 continue ;
		ipt = UNQUOTEPTR(ipt) ;
		break ;					/* Only unravel one quote at a time */
#endif
	      case V4DPI_PntType_SymDef:
	      case V4DPI_PntType_BigIsct:
	      case V4DPI_PntType_Isct:
		i = ipt->ForceEval ;			/* Save this flag */
		if (ISQUOTED(ipt))
		 { if (!i) break ;			/* Don't eval if quoted! */
		   fail = TRUE ;			/* set fail-on-undefined */
		 } else { fail = FALSE ; } ;
		if (argbufcnt >= V4DPI_IntModArgBuf_Max)
		 { pix = v4dpi_PntIdx_AllocPnt() ; cpt = v4dpi_PntIdx_CvtIdxPtr(pix) ;
		 } else { cpt = &argpntbufs[argbufcnt++] ; } ;	/* Allocate temp point area */
		tpt = ipt ;				/* Save in case we fail */
//		ipt = v4dpi_IsctEval(cpt,ipt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
//		ctx->NestedIsctEval++ ;			/* Increment this so if a failure we get decent error stack VEH060809 */
		FASTISCTEVAL(ipt,ipt,cpt) ;
//		ctx->NestedIsctEval-- ;			/*  and back again */
		if (ipt == NULL)
		 { v_Msg(ctx,NULL,"ModArgEval",intmodx,ix,tpt) ; goto intmod_iarg_fail ;
		 } ;
/*		If result was End::xxx or Continue::xxx then immediately return it! (veh070124) */
/*		(the Do() intmod evaluates its argument one at a time so we will never be here if this is a Do()) */
		if (ipt->PntType == V4DPI_PntType_TagVal)
		 { struct V4DPI__TagVal *tv ;
		   tv = (struct V4DPI__TagVal *)&ipt->Value ;
		   if ((tv->TagVal & V4DPI_TagFlag_MaskOut) == V4IM_Tag_End || (tv->TagVal & V4DPI_TagFlag_MaskOut) == V4IM_Tag_Continue)
		    { memcpy(respnt,ipt,ipt->Bytes) ; return(respnt) ; } ;
		 } ;
		if (i)						/* Got ForceEval? */
		 { switch(ipt->PntType)
		    {
		      case V4DPI_PntType_Special:
		      case V4DPI_PntType_Isct:
			memcpy(&ptbuf,ipt,ipt->Bytes) ;
			ipt = v4dpi_IsctEval(cpt,&ptbuf,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
			if (ipt == NULL)
			 { v_Msg(ctx,NULL,"ModArgNestEval",intmodx,ix,&ptbuf) ; goto intmod_iarg_fail ; } ;
			break ;
		      case V4DPI_PntType_List:			/* If list then expand arguments */
			lp = ALIGNLP(&ipt->Value) ;
			for(j=1;;j++)
			 { if (argbufcnt >= V4DPI_IntModArgBuf_Max)
			    { pix = v4dpi_PntIdx_AllocPnt() ; cpt = v4dpi_PntIdx_CvtIdxPtr(pix) ;
			    } else { cpt = &argpntbufs[argbufcnt++] ; } ;
			   if (v4l_ListPoint_Value(ctx,lp,j,cpt) <= 0) break ;
			   if (argcnt >= V4DPI_IntModArg_Max)
			    { v_Msg(ctx,NULL,"MaxNumModArgsXL",intmodx,ix,argcnt,V4DPI_IntModArg_Max) ; goto intmod_iarg_fail ; } ;
			   argpnts[argcnt++] = cpt ;
			 } ; argcnt-- ;
			continue ;
		    } ;
		 } ;
		break ;
	      case V4DPI_PntType_Special:
		switch(ipt->Grouping)
		 { default:
			v_Msg(ctx,NULL,"ModArgNestEvalS",intmodx,ix,ipt) ; goto intmod_iarg_fail ;
		   case V4DPI_Grouping_None:
		   case V4DPI_Grouping_Sample:
		   case V4DPI_Grouping_Undefined:
			break ;
		   case V4DPI_Grouping_Now:
		   case V4DPI_Grouping_New:
			if (argbufcnt >= V4DPI_IntModArgBuf_Max)
			 { pix = v4dpi_PntIdx_AllocPnt() ; cpt = v4dpi_PntIdx_CvtIdxPtr(pix) ;
			 } else { cpt = &argpntbufs[argbufcnt++] ; } ;	/* Allocate temp point area */
			i = ipt->Dim ; ipt = v4dpi_IsctEval(cpt,ipt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
			if (ipt == NULL)
			 { v_Msg(ctx,NULL,"ModArgNewEval",intmodx,ix,ipt) ; goto intmod_iarg_fail ; } ;
			break ;
		   case V4DPI_Grouping_All:			/* If got dim.. then expand to list, UNLESS in In()! */
			{ LOGICAL doit ;
			  if (ISQUOTED(ipt)) { UNQUOTE(ipt) ; doit = FALSE ; }
			   else if (ipt->ForceEval) { ipt->ForceEval = FALSE ; doit = TRUE ; }
			   else { doit = (intmodx != V4IM_OpCode_In && intmodx != V4IM_OpCode_Format && intmodx != V4IM_OpCode_Project) ; } ;
			  if (doit)
			   { if (argbufcnt >= V4DPI_IntModArgBuf_Max)
			      { pix = v4dpi_PntIdx_AllocPnt() ; cpt = v4dpi_PntIdx_CvtIdxPtr(pix) ;
			      } else { cpt = &argpntbufs[argbufcnt++] ; } ;	/* Allocate temp point area */
			     tpt = ipt ; ipt = v4dpi_DimUniqueToList(cpt,ctx,ipt->Dim,Dim_List,ipt,TRUE) ;
			     if (ipt == NULL)
			      { if (tpt->Continued)		/* Have ,continue-point ? */
			         { ipt = v4dpi_IsctEval(cpt,(P *)&tpt->Value,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
				   if (ipt != NULL) break ;
			         } ;
			        v_Msg(ctx,NULL,"ModArgAllEval",intmodx,ix,tpt->Dim) ; goto intmod_iarg_fail ;
			      } ;
			   } ;
			}
//			if ((ipt->Quoted ? (ipt->Quoted=FALSE)
//				: (ipt->ForceEval ? (ipt->ForceEval=FALSE,TRUE)
//				    : (intmodx != V4IM_OpCode_In && intmodx != V4IM_OpCode_Format && intmodx != V4IM_OpCode_Project) )))
//			 { if (argbufcnt >= V4DPI_IntModArgBuf_Max)
//			    { pix = v4dpi_PntIdx_AllocPnt() ; cpt = v4dpi_PntIdx_CvtIdxPtr(pix) ;
//			    } else { cpt = &argpntbufs[argbufcnt++] ; } ;	/* Allocate temp point area */
//			   tpt = ipt ; ipt = v4dpi_DimUniqueToList(cpt,ctx,ipt->Dim,Dim_List,ipt,TRUE) ;
//			   if (ipt == NULL)
//			    { if (tpt->Continued)		/* Have ,continue-point ? */
//			       { ipt = v4dpi_IsctEval(cpt,(P *)&tpt->Value,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
//				 if (ipt != NULL) break ;
//			       } ;
//			      v_Msg(ctx,NULL,"ModArgAllEval",intmodx,ix,tpt->Dim) ; goto intmod_iarg_fail ;
//			    } ;
//			 } ;
			break ;
		   case V4DPI_Grouping_PCurrent:
			if (ISQUOTED(ipt)) break ;
			tpt = ipt ; i = ipt->Dim ; DIMPVAL(ipt,ctx,i) ;
			if (ipt == NULL)
			 { v_Msg(ctx,NULL,"ModArgNewEval",intmodx,ix,tpt) ; goto intmod_iarg_fail ;
			 } ;
			goto current_entry ;
		   case V4DPI_Grouping_Current:
			if (ISQUOTED(ipt)) break ;
			tpt = ipt ; DIMVAL(ipt,ctx,ipt->Dim) ;
			if (ipt == NULL)
			 { if (tpt->Continued)		/* Have ,continue-point ? */
			    { if (argbufcnt >= V4DPI_IntModArgBuf_Max)
			       { pix = v4dpi_PntIdx_AllocPnt() ; cpt = v4dpi_PntIdx_CvtIdxPtr(pix) ;
			       } else { cpt = &argpntbufs[argbufcnt++] ; } ;	/* Allocate temp point area */
			      ipt = v4dpi_IsctEval(cpt,(P *)&tpt->Value,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
			      if (ipt != NULL) break ;
			    } ;
			   v_Msg(ctx,NULL,"ModArgNewEval",intmodx,ix,tpt) ; goto intmod_iarg_fail ;
			 } ;
current_entry:		if (ipt->PntType == V4DPI_PntType_Shell)
			 { ipt = (P *)&ipt->Value ;
			   if (tpt->ForceEval)					/* VEH070625 - Handle forced evaluate shell value */
			    { if (ipt->PntType == V4DPI_PntType_Isct || ipt->PntType == V4DPI_PntType_Special)
			       goto eval_arg ;
			    } ;
			  } ;
			if (ipt->PntType == V4DPI_PntType_Isct)
			 { if (ISQUOTED(ipt)) { break ; } else { goto eval_arg ; } ;
			 } ;
			if (ipt->PntType == V4DPI_PntType_List && tpt->ForceEval)
			 { lp = ALIGNLP(&ipt->Value) ;
			   for(j=1;;j++)
			    { if (argbufcnt >= V4DPI_IntModArgBuf_Max)
			       { pix = v4dpi_PntIdx_AllocPnt() ; cpt = v4dpi_PntIdx_CvtIdxPtr(pix) ;
			       } else { cpt = &argpntbufs[argbufcnt++] ; } ;
			      if (v4l_ListPoint_Value(ctx,lp,j,cpt) <= 0) break ;
			      if (argcnt >= V4DPI_IntModArg_Max)
			       { v_Msg(ctx,NULL,"MaxNumModArgsXL",intmodx,ix,argcnt,V4DPI_IntModArg_Max) ; goto intmod_iarg_fail ; } ;
			      argpnts[argcnt++] = cpt ;
			    } ; argcnt-- ;
			   continue ;
			 } ;
			break ;
		 } ;
	    } ;
	   argpnts[argcnt] = ipt ;				/* Save argument */
	 } ;
	if (argcnt < minargs[intmodx-1])
	 { v_Msg(ctx,NULL,"ModArgMissing",intmodx,minargs[intmodx-1],argcnt) ; goto intmod_fail ; } ;

#ifdef V4_BUILD_RUNTIME_STATS
	gpi->V4DtlModCalls[intmodx] ++ ; gpi->V4ArgsInMod[intmodx] += argcnt ;
	gpi->V4IntModTotalByArg[argcnt >= 32 ? 31 : argcnt] ++ ;
#endif

	if (isct->TraceEval || ((trace & V4TRACE_LogIscts) != 0 && ctx->IsctEvalCount > gpi->StartTraceOutput))
	 { 
	   
//	   v_Msg(ctx,UCTBUF2,"*TraceIMEntry",intmodx) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	   for(bytes=0,ix=1;ix<=argcnt;ix++)
	    { if (ctx->imArgBuf[ix] == NULL) ctx->imArgBuf[ix] = v4mm_AllocUC(V4TMBufMax) ;
	      v4dpi_PointToString(ctx->imArgBuf[ix],argpnts[ix],ctx,V4DPI_FormatOpt_Trace|V4DPI_FormatOpt_DisplayerTrace) ;
	      bytes += UCstrlen(ctx->imArgBuf[ix]) ;
	    } ;
/*	   If all arguments add up to alot of characters then display on separate lines, otherwise show all args on single line */
	  if (bytes > 255)
	   { if (v_Msg(ctx,UCTBUF1,"TraceModEntry",curcall,intmodx,argcnt)) vout_UCText(VOUT_Trace,0,UCTBUF1) ;
	     for(ix=1;ix<=argcnt;ix++)
	      { if (v_Msg(ctx,UCTBUF1,"TraceModArg",curcall,ix,ctx->imArgBuf[ix])) vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
	   } else
	   { ZUS(UCTBUF2) ;
	     for(ix=1;ix<=argcnt;ix++) { v_Msg(ctx,&UCTBUF2[UCstrlen(UCTBUF2)],"@%1d:%2U ",ix,ctx->imArgBuf[ix]) ; } ;
	     if (v_Msg(ctx,UCTBUF1,"TraceModArgShrt",curcall,intmodx,UCTBUF2)) vout_UCText(VOUT_Trace,0,UCTBUF1) ;
	   } ;
	 } ;
	switch(intmodx)
	 { default:
		v_Msg(ctx,NULL,"ModInvOpCode",intmodx) ; goto intmod_fail ;
	   case V4IM_OpCode_Bits:			/* Bits( mask tag ) */
		ipt = (P* )v4imu_Bits(ctx,respnt,intmodx,argpnts,argcnt) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Echo:			/* [IntMod=xxx pointtoecho] */
	   case V4IM_OpCode_EchoP:			/* [IntMod=xxx pointtoecho] */
		ipt = v4im_DoEcho(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;

	   case V4IM_OpCode_EchoW:
	   case V4IM_OpCode_EchoA:
	   case V4IM_OpCode_EchoE:
		ipt = v4im_DoEchoE(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_EchoD:			/* [IntMod=xxx pointtoecho] */
		if (gpi->RestrictionMap & V_Restrict_QuotaEcho)
		 { if (vout_TotalBytes > gpi->QuotaData) { v_Msg(ctx,NULL,"RestrictQuotaO",intmodx,gpi->QuotaData) ; goto intmod_fail ; } ;
		 } ;
/*		If we are doing timestamps then start with "*" to force one out */
		if (traceGlobal & V4TRACE_TimeStamp) { vout_UCText(VOUT_Debug,0,UClit("*")) ; } ;

		for(ix=1;ix<=argcnt;ix++)
		 { 
		   ipt = v4dpi_IsctEval(&ptbuf,argpnts[ix],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (ipt == NULL)
		    { v4dpi_PointToStringML(UCTBUF2,argpnts[ix],ctx,V4DPI_FormatOpt_Echo,9999) ;
		      UCTBUF2[10] = UCEOS ; v_Msg(ctx,UCTBUF1,"@?%1d:%2U",ix,UCTBUF2) ;
		    } else { v4dpi_PointToStringML(UCTBUF1,ipt,ctx,V4DPI_FormatOpt_Trace,9999) ; } ;
		   vout_UCText(VOUT_Debug,0,UCTBUF1) ; if (ix < argcnt) vout_UCText(VOUT_Debug,0,UClit(" ")) ;
		 } ;
		vout_NL(VOUT_Debug) ;
		ipt = (P *)&protoNone ;			/*  & return as value */
		break ;
	   case V4IM_OpCode_EchoT:
EchoT_Entry:	ipt = v4im_DoEchoT(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_EchoN:			/* EchoN - Echo "null" */
		baf = v4ctx_FrameBaAInfo(ctx) ; 	/* Doing totals? */
		if (baf == NULL) { ipt = (P *)&Log_False ; break ; } ; /* no - not much point in this then! */
		for(num=0,ix=1;ix<=argcnt;ix++)
		 { ipt = argpnts[ix] ;
		   if (ipt->PntType != V4DPI_PntType_List) { lp = NULL ; }
		    else { lp = ALIGNLP(&ipt->Value) ; } ;
		   for(j=1;;j++)
		    { if (lp != NULL)
		       { if (v4l_ListPoint_Value(ctx,lp,j,&isctbuf) < 1) break ;
			 ipt = &isctbuf ;
		       } ;
		      ++num ;			/* Update to current column */
		      v4im_BaAIncrement(ctx,baf,ipt,num,UNUSED) ;
		      if (lp == NULL) break ;
		    } ;
		 } ;
		ipt = (P *)&Log_True ;
		break ;
	   case V4IM_OpCode_SSDim:			/* SSDim( dim [list] ) */
		ipt = v4im_DoSSDim(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
#ifdef WANTSSSTUFF
	   case V4IM_OpCode_SSFormat:
		{ struct V4SS__FormatSpec *vfs ;
		  ZPH(respnt) ; respnt->Dim = Dim_UV4 ; respnt->PntType = V4DPI_PntType_SSVal ;
		  vfs = (struct V4SS__FormatSpec *)&respnt->Value ; respnt->Bytes = V4DPI_PointHdr_Bytes + sizeof *vfs ;
		  v_ParseFormatSpecs(ctx,vfs,V4SS_FormatType_Init,NULL) ;
		  for(i=1;i<=argcnt;i++)			/* Step thru the remaining arguments */
		    { ipt = argpnts[i] ;
		      switch (v4im_CheckPtArgNew(ctx,argpnts[i],&cpt,&isctbuf))
		       { default:		v_Msg(ctx,NULL,"TagBadUse",intmodx) ; RE1(0) ;
			 case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; RE1(0) ;
			 case V4IM_Tag_Dim:	goto next_dim ;
			 case V4IM_Tag_Font:	
			   v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; if (!ok) break ;
			   ok = v_ParseFormatSpecs(ctx,vfs,V4SS_FormatType_Font,tb) ; break ;
			 case V4IM_Tag_StyleName:
			 case V4IM_Tag_Style:
			   v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; if (!ok) break ;
			   ok = v_ParseFormatSpecs(ctx,vfs,V4SS_FormatType_Style,tb) ; break ;
			 case V4IM_Tag_Format:
			   v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; if (!ok) break ;
			   ok = v_ParseFormatSpecs(ctx,vfs,V4SS_FormatType_Mask,tb) ; break ;
			 case V4IM_Tag_FontSize:
			   v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; if (!ok) break ;
			   ok = v_ParseFormatSpecs(ctx,vfs,V4SS_FormatType_Size,tb) ; break ;
			 case V4IM_Tag_CellColor:
			   v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; if (!ok) break ;
			   ok = v_ParseFormatSpecs(ctx,vfs,V4SS_FormatType_CellColor,tb) ; break ;
			 case V4IM_Tag_TextColor:
			   v4im_GetPointUC(&ok,tb,UCsizeof(tb),cpt,ctx) ; if (!ok) break ;
			   ok = v_ParseFormatSpecs(ctx,vfs,V4SS_FormatType_TextColor,tb) ; break ;
			 case V4IM_Tag_PageBreak:
			   break ;
		       } ;
		      if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i) ; goto intmod_fail ; } ;
		    } ;
		 } ;
		ipt = respnt ; break ;
	   case V4IM_OpCode_SSVal:
		bp = &respnt->Value.AlphaVal[1] ; *bp = '\0' ; strcpy(bp,"=If(V4SSUpd,\"\",") ;
		cpt = argpnts[1] ; resdim = cpt->Dim ;
		switch(cpt->PntType)
		 { default:
			v4dpi_PointToString(tb,cpt,ctx,V4DPI_FormatOpt_Echo) ;
			strcat(bp,UCretASC(tb)) ; break ;
		   case V4DPI_PntType_Calendar:
		   case V4DPI_PntType_UDate:
			if (cpt->Value.IntVal == 0)		/* Do we have a date? */
			 { strcat(bp,"\"\"") ; }
			 else { v4dpi_PointToString(tb,cpt,ctx,V4DPI_FormatOpt_Echo) ;
				strcat(bp,"Value(\"") ; strcat(bp,UCretASC(tb)) ; strcat(bp,"\")") ; } ;
			break ;
		   case V4DPI_PntType_Color:
		   case V4DPI_PntType_Country:
		   case V4DPI_PntType_XDict:
		   case V4DPI_PntType_Dict:
		   CASEofChar
			v4im_GetPointChar(&ok,(char *)tb,sizeof tb,cpt,ctx) ; ARGERR(1) ;
			strcat(bp,"\"") ; strcat(bp,(char *)tb) ; strcat(bp,"\"") ; break ;
		 } ;
		goto ss_finish ;
	   case V4IM_OpCode_SSULC:
		rowdim = UNUSED ; coldim = UNUSED ;
		goto ssexp_entry ;
	   case V4IM_OpCode_SSCol:
		goto ssexp_entry ;
	   case V4IM_OpCode_SSRow:
		goto ssexp_entry ;
	   case V4IM_OpCode_SSExp:			/* SSExp( args Cell::value ) */
ssexp_entry:	cpt = NULL ; bp = &respnt->Value.AlphaVal[1] ; *bp = '\0' ;
		ok = intmodx == V4IM_OpCode_SSExp ;			/* Set flag for possible leading "+" (only if Expand) */
		strcpy(bp,(intmodx == V4IM_OpCode_SSExp ? "=If(V4SSExpand,\"" : "=If(V4SSUpd,\"")) ;
		for(ix=1,k=TRUE;k&&ix<=argcnt;ix++)
		 { ipt = argpnts[ix] ;
		   if (ipt->PntType == V4DPI_PntType_TagVal)
		    { switch (v4im_CheckPtArgNew(ctx,ipt,&tpt,&ptbuf))
		      { default:		v_Msg(ctx,NULL,"TagBadUse",intmodx) ; RE1(0) ;
			case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; RE1(0) ;
			 case V4IM_Tag_Cell:
				if (tpt->PntType != V4DPI_PntType_Isct) 	/* Quoted? */
				 { resdim = tpt->Dim ; cpt = &isctbuf ; memcpy(&isctbuf,tpt,tpt->Bytes) ; continue ;
				 } else
				 { cpt = v4dpi_IsctEval(&isctbuf,tpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
				   if (cpt == NULL) { ipt = NULL ; goto intmod_end1 ; } ;
				   resdim = cpt->Dim ; continue ;
				 } ;
			 case V4IM_Tag_Rows:	rowdim = v4im_GetPointInt(&k,tpt,ctx) ; continue ;
			 case V4IM_Tag_Columns: coldim = v4im_GetPointInt(&k,tpt,ctx) ; continue ;
		       } ;
		    } ;
		   v4dpi_PointToString(tb,argpnts[ix],ctx,(ipt->Dim == Dim_Alpha ? 0 :V4DPI_FormatOpt_ShowDim)) ;
		   UCstrcpyToASC(ASCTBUF1,tb) ;
		   if (ok)						/* First one? */
		    { switch(ASCTBUF1[0])
		       { default:	strcat(bp,"+") ; break ;	/* Default to "+" */
			 case '+': case '-': case '*': break ;		/* If explicit then leave it alone */
		       } ; ok = FALSE ;
		    } ;
		   if (ipt->PntType != V4DPI_PntType_List)
		    { if (strchr(ASCTBUF1,' ') != NULL && (bp1=strchr(ASCTBUF1,':')) != NULL)/* Got an embedded space? */
		       { *bp1 = '\0' ;					/* Copy dimension name */
			 strcat(ASCTBUF1,":\"") ; strcat(ASCTBUF1,bp1+1) ; strcat(ASCTBUF1,"\"") ;
		       } ;
		    } ;
		   bp1=(char *)&bp[strlen(bp)] ;
		   for(i=0;;i++)
		    { if (ASCTBUF1[i] == '"') *(bp1++) = '"' ;		/* If got a quote then preface with another quote */
		      if (ASCTBUF1[i] == '\\') *(bp1++) = '\\' ;		/* If got a "\" then double also */
		      *(bp1++) = ASCTBUF1[i] ;
		      if (ASCTBUF1[i] == '\0') break ;
		    } ;
		   strcat(bp," ") ;
		 } ;
		if (!k) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ix-1) ; goto intmod_fail ;
		 } ;
		if (intmodx == V4IM_OpCode_SSULC)
		 { if(rowdim == UNUSED || coldim == UNUSED)
		    { v_Msg(ctx,NULL,"@%1E Missing Rows::xxx and/or Columns::xxx arguments",intmodx) ; RE1(0) ; } ;
		   sprintf(ASCTBUF1,"UV4SSUpd:%d ",rowdim*10000+coldim) ; strcat(bp,ASCTBUF1) ;
		 } ;
		if (bp[strlen(bp)-1] == ' ') bp[strlen(bp)-1] = '\0' ;	/* Get rid of trailing space */
		strcat(bp,"\",") ;
		if (cpt == NULL)					/* No Cell::xxx value? */
		 { strcat(bp,"\"\"") ;				/*  insert empty string */
		 } else
		 { switch(cpt->PntType)
		    { default:
			v4dpi_PointToString(tb,cpt,ctx,V4DPI_FormatOpt_Echo) ;
			strcat(bp,UCretASC(tb)) ; break ;
		      case V4DPI_PntType_Calendar:
		      case V4DPI_PntType_UDate:
			v4dpi_PointToString(tb,cpt,ctx,V4DPI_FormatOpt_Echo) ;
			strcat(bp,"Value(") ; strcat(bp,UCretASC(tb)) ; strcat(bp,")") ; break ;
		      case V4DPI_PntType_List:
		      case V4DPI_PntType_UOM:
		      case V4DPI_PntType_UOMPer:
		      case V4DPI_PntType_UOMPUOM:
			v4dpi_PointToString(tb,cpt,ctx,V4DPI_FormatOpt_Echo) ;
			strcat(bp,"\"") ; strcat(bp,UCretASC(tb)) ; strcat(bp,"\"") ; break ;
		      case V4DPI_PntType_Color:
		      case V4DPI_PntType_Country:
		      case V4DPI_PntType_XDict:
		      case V4DPI_PntType_Dict:
		      CASEofChar
			v4im_GetPointChar(&ok,(char *)tb,sizeof tb,cpt,ctx) ; ARGERR(ix) ;
			strcat(bp,"\"") ; strcat(bp,(char *)tb) ; strcat(bp,"\"") ; break ;
		    } ;
		 } ;
ss_finish:	/* cpt points to cell value, or NULL if none */
		strcat(bp,")") ;
		ZPH(respnt) ; respnt->Dim = resdim ;
		if (cpt == NULL) { respnt->PntType = V4DPI_PntType_Char ; }
		 else { respnt->PntType = V4DPI_PntType_FChar ;  respnt->LHSCnt = cpt->PntType ; }
		i = strlen(bp) ;
		CHARPNTBYTES2(respnt,i)
		ipt = respnt ;
		break ;
#endif
	   case V4IM_OpCode_Rpt:
		if (gpi->RestrictionMap & V_Restrict_QuotaEcho)
		 { if (vout_TotalBytes > gpi->QuotaData) { v_Msg(ctx,NULL,"RestrictQuotaO",intmodx,gpi->QuotaData) ; goto intmod_fail ; } ;
		 } ;
		ipt = v4im_DoRpt(ctx,respnt,argpnts,argcnt,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_EchoS:
		if (!gpi->DoEchoS) goto EchoT_Entry ;
		if (gpi->RestrictionMap & V_Restrict_QuotaEcho)
		 { if (vout_TotalBytes > gpi->QuotaData) { v_Msg(ctx,NULL,"RestrictQuotaO",intmodx,gpi->QuotaData) ; goto intmod_fail ; } ;
		 } ;
		ipt = v4im_DoEchoS(ctx,respnt,argpnts,argcnt,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_EvalCmd:			/* [IntMod=xxx Alpha=evalstring] */
//		if (gpi->RestrictionMap & V_Restrict_EvalCmd) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto intmod_fail ; } ;
		ipt =  v4im_DoEvalCmd(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_EvalPt:
		ipt =  v4im_DoEvalPt(ctx,respnt,argpnts,argcnt,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_ListSize:		/* [IntMod=xxx List=list dim=xxx] */
		ipt = argpnts[1] ;			/* First best be list */
		lp = VERIFYLIST(&ptbuf,ctx,ipt,intmodx) ;
		if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList",intmodx,1) ; goto intmod_fail ; } ;
		ipt = respnt ;
//		intPNTv(ipt,v4l_ListPoint_Value(ctx,lp,V4L_ListSize,NULL)) ;
		intPNTv(ipt,SIZEofLIST(lp)) ;
		break ;
	   case V4IM_OpCode_Len:
		ipt = argpnts[1] ;
len_entry:
		if (ipt->Grouping != V4DPI_Grouping_Single)
		 { lp = VERIFYLIST(&ptbuf,ctx,ipt,intmodx) ; if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList",intmodx,1) ; goto intmod_fail ; } ;
		   intPNTv(respnt,SIZEofLIST(lp)) ;
		   ipt = respnt ; break ;
		 } ;
		switch(ipt->PntType)
		 { default:
			v_Msg(ctx,NULL,"ModArgPntType",intmodx,1,ipt,ipt->PntType) ; goto intmod_fail ;
		   case V4DPI_PntType_Tree:
			intPNTv(respnt,-1) ; break ;
		   case V4DPI_PntType_Shell:
			ipt = (P *)&ipt->Value ;
			goto len_entry ;
		   case V4DPI_PntType_TeleNum:
		   case V4DPI_PntType_UOM:
		   case V4DPI_PntType_UOMPer:
		   case V4DPI_PntType_UOMPUOM:
		   case V4DPI_PntType_GeoCoord:
		   case V4DPI_PntType_Complex:
		   case V4DPI_PntType_Country:
		   case V4DPI_PntType_Dict:
		   case V4DPI_PntType_XDict:
		   case V4DPI_PntType_Color:
		   CASEofINT
		   CASEofDBL
			v4dpi_PointToStringML(UCTBUF2,ipt,ctx,V4DPI_FormatOpt_Echo,9999) ;
			intPNTv(respnt,UCstrlen(UCTBUF2)) ; break ;
		   CASEofCharmU
			intPNTv(respnt,CHARSTRLEN(ipt)) ; break ;
			break ;
		   case V4DPI_PntType_UCChar:
			intPNTv(respnt,UCCHARSTRLEN(ipt)) ; break ;
			break ;
		   case V4DPI_PntType_BigText:
			{ UCCHAR *b ;
			  b = v4_BigTextCharValue(ctx,ipt) ;
			  intPNTv(respnt,UCstrlen(b)) ; break ;
			}
		   case V4DPI_PntType_List:
			lp = VERIFYLIST(&ptbuf,ctx,ipt,intmodx) ; if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList",intmodx,1) ; goto intmod_fail ; } ;
			intPNTv(respnt,SIZEofLIST(lp)) ;
			break ;
		   case V4DPI_PntType_Drawer:
			{ struct V4IM__Drawer *drw ;
			  drw = v4im_GetDrawerPtr(ipt->Dim,ipt->Value.IntVal) ;
			  intPNTv(respnt,drw->Points) ;
			}
			break ;
		 } ;
		ipt = respnt ; break ;
	   case V4IM_OpCode_Locale:
		ipt = v4im_DoLocale(ctx,respnt,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_DTInfo:				/* DTInfo( udate what ) */
		ipt = v4im_DoDTInfo(ctx,respnt,intmodx,argpnts,argcnt) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
//	   case V4IM_OpCode_Field_V4IS:
#ifdef WANTFIELD_V4IS
		ipt = argpnts[1] ;				/* Link to record */
		memcpy(&recptr,&ipt->Value.MemPtr,sizeof(recptr)) ;
		ipt = argpnts[2] ;				/* Data element */
		des = (struct V4FFI__DataElSpec *)&ipt->Value ;
		DIMINFO(di,ctx,v4dpi_DimGet(ctx,des->DimName)) ;
		if (di == NULL)
		 { sprintf(ctx->ErrorMsg,"%s Dimension (%s) associated with field (%d.%d) is not defined",
				v4im_ModFailStr(intmodx),des->DimName,des->FileRef,des->Element) ; RE1(0) ;
		 } ;
		ipt = respnt ;
		ipt->Dim = di->DimId ;				/* Build up return value */
		ipt->PntType = di->PointType ;
		ipt->Grouping = 0 ; ipt->LHSCnt = 0 ; ipt->NestedIsct = 0 ;
		switch (ipt->PntType)
		 { default:	sprintf(ctx->ErrorMsg,"%s Point (%s) cannot return value for data field",v4im_ModFailStr(intmodx),v4im_PTName(ipt->PntType)) ; RE1(0) ;
		   CASEofINT
			shortptr = (short *)(intptr = (int *)(recptr + des->Offset)) ;
			ipt->Value.IntVal = (des->Bytes == SIZEofINT ? *intptr : *shortptr) ;
			ipt->Bytes = V4PS_Int ;
			break ;
		   case V4DPI_PntType_BinObj:
			memcpy(&ipt->Value.AlphaVal,recptr+des->Offset,des->Bytes) ;
			ipt->Bytes = V4DPI_PointHdr_Bytes + des->Bytes ;
			break ;
		   CASEofChar
			memcpy(&ipt->Value.AlphaVal[1],recptr+des->Offset,des->Bytes) ;
			CHARPNTBYTES2(ipt,des->Bytes)
			break ;
		 } ;
#endif
		break ;
//	   case V4IM_OpCode_S1bcField_V4IS:
#ifdef WANTFIELD_V4IS
		ipt = argpnts[1] ;				/* Link to record */
		memcpy(&recptr,&ipt->Value.MemPtr,sizeof(recptr)) ;
		ipt = argpnts[2] ;				/* Data element */
		des = (struct V4FFI__DataElSpec *)&ipt->Value ;
		DIMINFO(di,ctx,v4dpi_DimGet(ctx,des->DimName)) ;
		if (di == NULL)
		 v4_error(V4E_DIMNOTDEF,0,"V4IM","Field_V4IS","DIMNOTDEF","Dimension (%s) associated with field (%d.%d) is not defined",
				des->DimName,des->FileRef,des->Element) ;
		bytes = des->Bytes ; offset = des->Offset ;	/* Save for later */
		sprintf(ASCTBUF1,"[FileRef=%d Structure=%d]",des->FileRef,des->Owner) ;
		INITTCB ; v4lex_NestInput(tcb,NULL,ASCTBUF1,V4LEX_InpMode_String) ;
		if (!v4dpi_PointParse(ctx,&ptbuf,tcb,V4DPI_PointParse_RetFalse))
		 v4_error(V4E_ISCTEVALFSFAIL,0,"V4IM","IntModS1bx","ISCTEVALFSFAIL","Invalid point: %s",ctx->ErrorMsgAux) ;
		if ((ipt = v4dpi_IsctEval(&isctbuf,&ptbuf,ctx,0,NULL,NULL)) == NULL)
		 v4_error(V4E_ISCTEVALFSFAIL,0,"V4IM","IntModS1bx","ISCTEVALFSFAIL","Could not eval [file= structure=] for field") ;
		ses = (struct V4FFI__StructElSpec *)&ipt->Value ;
		structoffset = ses->Offset ; structbytes = ses->Bytes ;
		sprintf(ASCTBUF1,"[DataEl FileRef=%d Field=%s]",ses->FileRef,ses->CountField) ;
		INITTCB ; v4lex_NestInput(tcb,NULL,ASCTBUF1,V4LEX_InpMode_String) ;
		if (!v4dpi_PointParse(ctx,&ptbuf,tcb,V4DPI_PointParse_RetFalse))
		 v4_error(V4E_ISCTEVALFSFAIL,0,"V4IM","IntModS1bx","ISCTEVALFSFAIL","Invalid point: %s",ctx->ErrorMsgAux) ;
		if ((ipt = v4dpi_IsctEval(&isctbuf,&ptbuf,ctx,0,NULL,NULL)) == NULL)
		 v4_error(V4E_CNTFIELDFAIL,0,"V4IM","IntModS1bx","CNTFIELDFAIL","Could not eval to find count field for this substructure element") ;
		des = (struct V4FFI__DataElSpec *)&ipt->Value ; /* Get des for count field */
		shortptr = (short *)(intptr = (int *)(recptr + des->Offset)) ;
		maxcnt = (des->Bytes == SIZEofINT ? *intptr : *shortptr) ; /* Get max number of occurs in substructure */
		ipt = argpnts[3] ;				/* Field to search */
		des = (struct V4FFI__DataElSpec *)&ipt->Value ;
		ipt = argpnts[4] ;				/* Value point to find */
		ix = ipt->Value.IntVal ;
		for(ix=0;ix<maxcnt;ix++)
		 { intptr = (int *)(recptr + (structoffset + structbytes * ix)	/* Adjust to proper structure */
				+ des->Offset) ;				/* Plus field within substructure */
		   shortptr = (short *)intptr ;
		   if (ipt->Value.IntVal == (des->Bytes == SIZEofINT ? *intptr : *shortptr)) break ;
		 } ;
		if (ix >= maxcnt) v4_error(V4E_SUBDIMFAIL,0,"V4IM","S1bc","SUBDIMFAIL","Could not locate sub-dimension in current record") ;
		recptr = recptr + (structoffset + structbytes * ix) ;	/* Adjust to proper entry in structure */
		ptbuf.Dim = di->DimId ; 			/* Build up return value */
		ptbuf.PntType = di->PointType ;
		ptbuf.Grouping = 0 ; ptbuf.LHSCnt = 0 ; ptbuf.NestedIsct = 0 ;
		switch (ptbuf.PntType)
		 { default:	v4_error(V4E_PNTVALFAIL,0,"V4IM","Field_V4IS","PNTVALFAIL","Cannot return value for data field") ;
		   CASEofINT
			shortptr = (short *)(intptr = (int *)(recptr + offset)) ;
			ptbuf.Value.IntVal = (bytes == SIZEofINT ? *intptr : *shortptr) ;
			ptbuf.Bytes = V4PS_Int ;
			break ;
		   case V4DPI_PntType_BinObj:
			memcpy(&ipt->Value.AlphaVal,recptr+des->Offset,des->Bytes) ;
			ipt->Bytes = V4DPI_PointHdr_Bytes + des->Bytes ;
			break ;
		   CASEofChar
			memcpy(&ptbuf.Value.AlphaVal[1],recptr+offset,bytes) ;
			CHARPNTBYTES2(&ptbuf,bytes)
			break ;
		 } ;
		ipt = respnt ; memcpy(ipt,&ptbuf,ptbuf.Bytes) ;
#endif
		break ;
//	   case V4IM_OpCode_S1bxField_V4IS:
#ifdef WANTFIELD_V4IS
		ipt = argpnts[1] ;				/* Link to record */
		memcpy(&recptr,&ipt->Value.MemPtr,sizeof(recptr)) ;
		ipt = argpnts[2] ;				/* Data element */
		des = (struct V4FFI__DataElSpec *)&ipt->Value ;
		sprintf(ASCTBUF1,"[FileRef=%d Structure=%d]",des->FileRef,des->Owner) ;
		INITTCB ; v4lex_NestInput(tcb,NULL,ASCTBUF1,V4LEX_InpMode_String) ;
		if (!v4dpi_PointParse(ctx,&ptbuf,tcb,V4DPI_PointParse_RetFalse))
		 v4_error(V4E_ISCTEVALFSFAIL,0,"V4IM","IntModS1bx","ISCTEVALFSFAIL","Invalid point: %s",ctx->ErrorMsgAux) ;
		if ((ipt = v4dpi_IsctEval(&isctbuf,&ptbuf,ctx,0,NULL,NULL)) == NULL)
		 v4_error(V4E_ISCTEVALFSFAIL,0,"V4IM","IntModS1bx","ISCTEVALFSFAIL","Could not eval [file= structure=] for field") ;
		ses = (struct V4FFI__StructElSpec *)&ipt->Value ;
		ipt = argpnts[3] ;				/* Index (integer) */
		ix = ipt->Value.IntVal ;
		recptr = recptr + (ses->Offset + (ses->Bytes * (ix-1))) ;	/* Adjust to proper entry in structure */
		DIMINFO(di,ctx,v4dpi_DimGet(ctx,des->DimName)) ;
		if (di == NULL)
		 v4_error(V4E_DIMNOTDEF,0,"V4IM","Field_V4IS","DIMNOTDEF","Dimension (%s) associated with field (%d.%d) is not defined",
				des->DimName,des->FileRef,des->Element) ;
		ptbuf.Dim = di->DimId ; 			/* Build up return value */
		ptbuf.PntType = di->PointType ;
		ptbuf.Grouping = 0 ; ptbuf.LHSCnt = 0 ; ptbuf.NestedIsct = 0 ;
		switch (ptbuf.PntType)
		 { default:	v4_error(V4E_PNTRETVAL,0,"V4IM","Field_V4IS","PNTRETVAL","Cannot return value for data field") ;
		   CASEofINT
			shortptr = (short *)(intptr = (int *)(recptr + des->Offset)) ;
			ptbuf.Value.IntVal = (des->Bytes == SIZEofINT ? *intptr : *shortptr) ;
			ptbuf.Bytes = V4PS_Int ;
			break ;
		   case V4DPI_PntType_BinObj:
			memcpy(&ipt->Value.AlphaVal,recptr+des->Offset,des->Bytes) ;
			ipt->Bytes = V4DPI_PointHdr_Bytes + des->Bytes ;
			break ;
		   CASEofChar
			memcpy(&ptbuf.Value.AlphaVal[1],recptr+des->Offset,des->Bytes) ;
			CHARPNTBYTES2(&ptbuf,des->Bytes)
			break ;
		 } ;
		ipt = respnt ; memcpy(ipt,&ptbuf,ptbuf.Bytes) ;
#endif
		break ;
	   case V4IM_OpCode_DeferDo:
		v_Msg(ctx,NULL,"DeferDoCall",intmodx,V4IM_OpCode_Do) ; RE1(0) ;
	   case V4IM_OpCode_Do:
		ipt = v4im_DoDo(ctx,respnt,argcnt,argpnts,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
#ifdef WANTIMCACHE
	   case V4IM_OpCode_Cache:
		ipt = v4im_IMCache(ctx,respnt,argcnt,argpnts,intmodx,trace) ;
		break ;
#endif
//	   case V4IM_OpCode_CacheX:
//		i = v4im_GetPointInt(&ok,argpnts[2],ctx) ; ARGERR(2) ;
//		ipt = v4im_Cache(ctx,respnt,argpnts[1],i,trace) ;
//		break ;
	   case V4IM_OpCode_Context:
		for (i=1;i<=argcnt;i++)
		 { ipt = argpnts[i] ;				/* Get argument */
		   if (ipt->PntType == V4DPI_PntType_Isct)	/* Can't put isct into context - one last chance to evaluate it */
		    { ipt = v4dpi_IsctEval(respnt,argpnts[i],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		      if (ipt == NULL) { v_Msg(ctx,NULL,"ModArgEval",intmodx,i,argpnts[i]) ; RE1(0) ; } ;
		    } ;
		   if (!v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,ipt,0,0))	/* Add point to current context */
		    { v_Msg(ctx,NULL,"CtxTryContextL",intmodx) ; RE1(0) ; } ;
		 } ;
		ipt = respnt ;
		if (argcnt == 0) { memcpy(ipt,&protoNone,V4PS_Int) ; } else { memcpy(ipt,argpnts[argcnt],argpnts[argcnt]->Bytes) ; } ;
		break ;
	   case V4IM_OpCode_ContextL:
		for (i=1;i<=argcnt;i++)
		 { ipt = argpnts[i] ;				/* Get argument */
		   if (ipt->PntType == V4DPI_PntType_Isct)	/* Can't put isct into context - one last chance to evaluate it */
		    { ipt = v4dpi_IsctEval(respnt,argpnts[i],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		      if (ipt == NULL) { v_Msg(ctx,NULL,"ModArgEval",intmodx,i,argpnts[i]) ; RE1(0) ; } ;
		    } ;
		   if (!v4ctx_FrameAddDim(ctx,0,ipt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; RE1(0) ; } ; /* Add point to current context */
		 } ;
		ipt = respnt ;
		if (argcnt == 0) { memcpy(ipt,&protoNone,V4PS_Int) ; } else { memcpy(ipt,argpnts[argcnt],argpnts[argcnt]->Bytes) ; } ;
		break ;
	   case V4IM_OpCode_PRUpd:
		if (argpnts[1]->PntType != V4DPI_PntType_PntIdx)
		 { v_Msg(ctx,NULL,"ModArgPntType2",intmodx,1,argpnts[1]->PntType,V4DPI_PntType_PntIdx) ; RE1(0) ; } ;
		ipt = v4dpi_PntIdx_CvtIdxPtr(argpnts[1]->Value.IntVal) ;
		memcpy(ipt,argpnts[2],argpnts[2]->Bytes) ;
		memcpy(respnt,argpnts[1],argpnts[1]->Bytes) ; ipt = respnt ;
		break ;
	   case V4IM_OpCode_Geo:
		ipt = v4im_DoGeo(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Plus:			/* [IntMod=Plus Int=a Int=b ]	*/
		if (argcnt == 0)			/* No arguments - return Int:0 */
		 { ipt = respnt ; intPNTv(respnt,0) ; break ; } ;
plus_top:	switch (argpnts[1]->PntType)
		 { case V4DPI_PntType_UOM:
uom_plus_entry:		for(ix=1,dnum=0,i=UNUSED,j=1;ix<=argcnt;ix++)
			 { ipt = argpnts[ix] ;
			   if (ipt->PntType != V4DPI_PntType_UOM)
			    { k = v4im_GetPointInt(&ok,ipt,ctx) ; ARGERR(ix) ; if (k == 0) continue ;
			      v_Msg(ctx,NULL,"ModArgNotSame",intmodx,V4DPI_PntType_UOM,1,argpnts[1],ix,argpnts[ix]) ; RE1(0) ;
			    } ;
			   memcpy(&dnum1,&ipt->Value.UOMVal.Num,sizeof(dnum)) ;
			   if (fabs(dnum1) <= gpi->DblEpsilonZero) continue ;		/* VEH101216 - If close to 0 then ignore */
			   if (i == UNUSED) { j = ix ; i = ipt->Value.UOMVal.Ref ; } ;	/* Save first Ref - all must be identical */
			   if (ipt->Value.UOMVal.Ref != i)
			    { if (!v4dpi_UOMAttemptCoerce(ctx,trace,i,UNUSED,ipt,&dnum1))
			       { v_Msg(ctx,NULL,"ModArgNotSame",intmodx,V4DPI_PntType_UOM,1,argpnts[1],ix,argpnts[ix]) ; RE1(0) ; } ;
			    } ;
			   dnum += dnum1 ;
			 } ;
			ipt = respnt ; memcpy(ipt,argpnts[j],V4PS_UOM) ;	/* Copy header from first UOM */
			memcpy(&ipt->Value.UOMVal.Num,&dnum,sizeof(dnum)) ;
			goto end_plus ;
		   case V4DPI_PntType_PntIdx:
			argpnts[1] = v4dpi_PntIdx_CvtIdxPtr(argpnts[1]->Value.IntVal) ; goto plus_top ;
		   case V4DPI_PntType_Complex:
			dnum1 = dnum2 = 0.0 ;
			for(ix=1;ix<=argcnt;ix++)
			 { ipt = argpnts[ix] ;
			   if (ipt->PntType != V4DPI_PntType_Complex)
			    { i = v4im_GetPointInt(&ok,ipt,ctx) ; ARGERR(ix) ; if (i == 0) continue ;
			      v_Msg(ctx,NULL,"ModArgNotSame",intmodx,V4DPI_PntType_Complex,1,argpnts[1],ix,argpnts[ix]) ; RE1(0) ;
			    } ;
			   memcpy(&dnum,&ipt->Value.Complex.r,sizeof(dnum)) ; dnum1 += dnum ;
			   memcpy(&dnum,&ipt->Value.Complex.i,sizeof(dnum)) ; dnum2 += dnum ;
			 } ;
			ipt = respnt ; memcpy(ipt,argpnts[1],V4PS_Complex) ;
			memcpy(&ipt->Value.Complex.r,&dnum1,sizeof(dnum1)) ; memcpy(&ipt->Value.Complex.i,&dnum2,sizeof(dnum2)) ;
			goto end_plus ;
		 } ;
		for(ix=1,k=0,dnum=0,b64=0,num=UNUSED;ix<=argcnt;ix++)
		 { ipt = argpnts[ix] ;
		   switch (ipt->PntType)
		    { default:	
			v_Msg(ctx,NULL,"ModInvArgNumLog",intmodx,ix,ipt) ; RE1(0) ;
		      case V4DPI_PntType_Special:
			dnum += v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(ix) ; break ;
		      case V4DPI_PntType_Fixed: 	/* If got fixed then track separately - add together below */
			if (num == UNUSED) num = ipt->LHSCnt ;
			for(;num<ipt->LHSCnt;num++) { b64 *= 10 ; } ;	/* If new point has more places then scale & adjust num */
			b64 += v4im_GetPointFixed(&ok,ipt,ctx,num) ; break ;
		      CASEofINT
		      case V4DPI_PntType_UTime:
		      case V4DPI_PntType_Calendar:
		      case V4DPI_PntType_Real:		dnum += v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(ix) ; break ;
		      case V4DPI_PntType_UOM:
			if (ix != argcnt || dnum != 0.0)
			 goto uom_plus_entry ;
//			 { v_Msg(ctx,NULL,"ModArgNotSame",intmodx,V4DPI_PntType_UOM,1,argpnts[1],ix,argpnts[ix]) ; RE1(0) ; } ;
			ipt = respnt ; memcpy(ipt,argpnts[ix],argpnts[ix]->Bytes) ;
			goto end_plus ;
		      case V4DPI_PntType_PntIdx:
			argpnts[ix] = v4dpi_PntIdx_CvtIdxPtr(argpnts[ix]->Value.IntVal) ; goto plus_top ;
		      case V4DPI_PntType_Complex:
			if (ix != argcnt || dnum != 0.0)
			 { v_Msg(ctx,NULL,"ModArgNotSame",intmodx,V4DPI_PntType_Complex,1,argpnts[1],ix,argpnts[ix]) ; RE1(0) ; } ;
			ipt = respnt ; memcpy(ipt,argpnts[ix],argpnts[ix]->Bytes) ;
			goto end_plus ;
		      case V4DPI_PntType_UOMPer:	k = ix ; dnum += v4im_GetPointDbl(&ok,ipt,ctx) ; if (!ok) goto intmod_fail ; break ;
		    } ;
		 } ;
		if (k != 0)				/* Got UOMPer */
		 { ipt = respnt ; memcpy(ipt,argpnts[k],argpnts[k]->Bytes) ;
		   memcpy(&ipt->Value.UOMPerVal.Amount,&dnum,sizeof dnum) ;
		   break ;
		 } ;
		if(ipt->Dim == Dim_Int) 		/* Result = dim of last argument UNLESS Dim:Int, then scan back */
		 { for(ix=argcnt-1;ix>=1;ix--)
		    { ipt = argpnts[ix] ; if (ipt->Dim != Dim_Int) break ;} ;
		 } ;
		if (ipt->PntType == V4DPI_PntType_Fixed)
		 { if (dnum != 0)
		    { dnum += (dnum < 0 ? -vRound[num] : vRound[num]) ;
		      dnum = dnum * powers[num] ;
		      b64 += (B64INT)dnum ;
		    } ;
		   SCALEFIX(b64,num,ipt->LHSCnt)
		   memcpy(respnt,ipt,V4DPI_PointHdr_Bytes) ; ipt = respnt ;
		   memcpy(&respnt->Value.FixVal,&b64,sizeof b64) ;
		 } else
		 { if (b64 != 0) { dnum += (double)b64 / powers[num] ; } ;
		   memcpy(respnt,ipt,V4DPI_PointHdr_Bytes) ; ipt = respnt ; v4im_SetPointValue(ctx,ipt,dnum) ;
		 } ;
end_plus:
		break ;
	   case V4IM_OpCode_Minus:
minus_top:	if (argcnt == 1)			/* Single argument - negate */
		 { memcpy(respnt,argpnts[1],argpnts[1]->Bytes) ;
		   switch (respnt->PntType)
		    { default:
			v_Msg(ctx,NULL,"ModInvArgNumLog",intmodx,1,respnt) ; RE1(0) ;
		      case V4DPI_PntType_Logical:
			respnt->Value.IntVal = respnt->Value.IntVal <= 0 ; break ;
		      case V4DPI_PntType_CodedRange:
		      case V4DPI_PntType_Int:
		      case V4DPI_PntType_Delta:
			respnt->Value.IntVal = -respnt->Value.IntVal ; break ;
		      case V4DPI_PntType_PntIdx:
			argpnts[1] = v4dpi_PntIdx_CvtIdxPtr(argpnts[1]->Value.IntVal) ; goto minus_top ;
		      case V4DPI_PntType_Real:
			GETREAL(dnum1,respnt) ; dnum1 = -dnum1 ;
			PUTREAL(respnt,dnum1) ; break ;
		      case V4DPI_PntType_UOM:
			SETDBL(dnum1,respnt->Value.UOMVal.Num) ; dnum1 = -dnum1 ;
			SETDBL(respnt->Value.UOMVal.Num,dnum1) ; break ;
		      case V4DPI_PntType_UOMPer:
			SETDBL(dnum1,respnt->Value.UOMPerVal.Amount) ; dnum1 = -dnum1 ;
			SETDBL(respnt->Value.UOMPerVal.Amount,dnum1) ; break ;
			break ;
		      case V4DPI_PntType_Complex:		/* Return the conjugate */
			SETDBL(dnum,respnt->Value.Complex.i) ; dnum = -dnum ;
			SETDBL(respnt->Value.Complex.i,dnum) ; break ;
			break ;
		      case V4DPI_PntType_Fixed:
			memcpy(&b64,&ipt->Value.RealVal,sizeof b64) ; b64 = -b64 ;
			memcpy(&ipt->Value.RealVal,&b64,sizeof b64) ; break ;
		    } ; ipt = respnt ; break ;
		 } ;
		switch (argpnts[1]->PntType)
		 { case V4DPI_PntType_UOM:
			for(ix=1;ix<=argcnt;ix++)
			 { ipt = argpnts[ix] ;
			   memcpy(&dnum1,&ipt->Value.UOMVal.Num,sizeof(dnum)) ;
			   if (ix == 1) { i = ipt->Value.UOMVal.Ref ; dnum = dnum1 ; continue ; } ;	/* Save first Ref - all must be identical */
			   if (ipt->PntType != V4DPI_PntType_UOM)
			    { k = v4im_GetPointInt(&ok,ipt,ctx) ; ARGERR(ix) ; if (k == 0) continue ;
			      v_Msg(ctx,NULL,"ModArgNotSame",intmodx,V4DPI_PntType_UOM,1,argpnts[1],ix,argpnts[ix]) ; RE1(0) ;
			    } ;
			   if (ipt->Value.UOMVal.Ref != i && dnum1 != 0.0)
			    { if (!v4dpi_UOMAttemptCoerce(ctx,trace,i,UNUSED,ipt,&dnum1))
			       { v_Msg(ctx,NULL,"UOMAllSame",ix,1,argpnts[1],ix,argpnts[ix]) ; RE1(0) ; } ;
			    } ;
			   dnum -= dnum1 ;
			 } ;
			ipt = respnt ; memcpy(ipt,argpnts[1],V4PS_UOM) ;
			memcpy(&ipt->Value.UOMVal.Num,&dnum,sizeof(dnum)) ;
			goto end_minus ;
		   case V4DPI_PntType_Complex:
			for(ix=1;ix<=argcnt;ix++)
			 { ipt = argpnts[ix] ;
			   if (ipt->PntType != V4DPI_PntType_Complex)
			    { i = v4im_GetPointInt(&ok,ipt,ctx) ; ARGERR(ix) ; if (i == 0) continue ;
			      v_Msg(ctx,NULL,"ModArgNotSame",intmodx,V4DPI_PntType_Complex,1,argpnts[1],ix,argpnts[ix]) ; RE1(0) ;
			    } ;
			   memcpy(&dnum,&ipt->Value.Complex.r,sizeof(dnum)) ;
			   if (ix == 1) { dnum1 = dnum ; } else { dnum1 -= dnum ; } ;
			   memcpy(&dnum,&ipt->Value.Complex.i,sizeof(dnum)) ;
			   if (ix == 1) { dnum2 = dnum ; } else { dnum2 -= dnum ; } ;
			 } ;
			ipt = respnt ; memcpy(ipt,argpnts[1],V4PS_Complex) ;
			memcpy(&ipt->Value.Complex.r,&dnum1,sizeof(dnum1)) ; memcpy(&ipt->Value.Complex.i,&dnum2,sizeof(dnum2)) ;
			goto end_minus ;
		 } ;
		for(ix=1,k=0,dnum=0,b64=0,num=UNUSED;ix<=argcnt;ix++)
		 { ipt = argpnts[ix] ;
		   switch (ipt->PntType)
		    { default:
			v_Msg(ctx,NULL,"ModInvArgNumLog",intmodx,ix,ipt) ; RE1(0) ;
		      case V4DPI_PntType_Special:
			dnum += v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(ix) ; break ;
		      CASEofINT
			if (ix == 1) { dnum = (double)ipt->Value.IntVal ; }
			 else { dnum -= (double)ipt->Value.IntVal ; } ;
			break ;
		      case V4DPI_PntType_UTime:
		      case V4DPI_PntType_Calendar:
		      case V4DPI_PntType_Real:
			if (ix == 1) { GETREAL(dnum,ipt) ; }
			 else { dnum -= v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(ix) ; } ;
			break ;
		      case V4DPI_PntType_Fixed: 	/* If got fixed then track separately - add together below */
			if (num == UNUSED) num = ipt->LHSCnt ;
			for(;num<ipt->LHSCnt;num++) { b64 *= 10 ; } ;
			if (ix == 1) { b64 = v4im_GetPointFixed(&ok,ipt,ctx,num) ; }
			 else { b64 -= v4im_GetPointFixed(&ok,ipt,ctx,num) ; } ;
			break ;
		      case V4DPI_PntType_PntIdx:
			argpnts[ix] = v4dpi_PntIdx_CvtIdxPtr(argpnts[ix]->Value.IntVal) ; goto minus_top ;
		      case V4DPI_PntType_UOM:		/* If here then ix != 1 (would have caught above!) */
			if (ix != argcnt || dnum != 0.0) { v_Msg(ctx,NULL,"ModArgNotSame",intmodx,V4DPI_PntType_UOM,1,argpnts[1],ix,argpnts[ix]) ; RE1(0) ; } ;
			ipt = respnt ; memcpy(ipt,argpnts[ix],argpnts[ix]->Bytes) ;
			memcpy(&dnum1,&ipt->Value.UOMVal.Num,sizeof dnum1) ; dnum1 = -dnum1 ;
			memcpy(&ipt->Value.UOMVal.Num,&dnum1,sizeof dnum1) ; goto end_minus ;
		      case V4DPI_PntType_UOMPer:
			if (ix == 1) { dnum = v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(ix) ; }
			 else { dnum -= v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(ix) ; } ;
			k = ix ; break ;
		    } ;
		 } ;
		if (k != 0)				/* Got UOMPer */
		 { ipt = respnt ; memcpy(ipt,argpnts[k],argpnts[k]->Bytes) ;
		   memcpy(&ipt->Value.UOMPerVal.Amount,&dnum,sizeof dnum) ;
		   break ;
		 } ;
		if (ipt->Dim == Dim_Int && argcnt > 1) ipt = argpnts[argcnt-1] ;	/* Result = dim of last arg UNLESS Int */
		if (ipt->PntType == V4DPI_PntType_Fixed)
		 { if (dnum != 0)
		    { dnum += (dnum < 0 ? -vRound[num] : vRound[num]) ;
		      dnum = dnum * powers[num] ;
		      b64 += (B64INT)dnum ;
		    } ;
		   SCALEFIX(b64,num,ipt->LHSCnt)
		   memcpy(respnt,ipt,V4DPI_PointHdr_Bytes) ; ipt = respnt ;
		   memcpy(&respnt->Value.FixVal,&b64,sizeof b64) ;
		 } else
		 { if (b64 != 0) { dnum += (double)b64 / powers[num] ; } ;
		   if (fabs(dnum) <= gpi->DblEpsilonZero) dnum = 0.0 ;
		   memcpy(respnt,ipt,V4DPI_PointHdr_Bytes) ; ipt = respnt ; v4im_SetPointValue(ctx,ipt,dnum) ;
		 } ;
		break ;
end_minus:
		break ;
	   case V4IM_OpCode_Mult:			/* [IntMod=Mult Int=a Int=b ]	*/
mult_top:	switch(argpnts[1]->PntType)
		 { default:			i = 1 ; break ; 	/* R egular */
		   case V4DPI_PntType_UOM:	i = 2 ; break ; 	/* U OM */
		   case V4DPI_PntType_UOMPer:	i = 3 ; break ; 	/* P er */
		   case V4DPI_PntType_Complex:	i = 4 ; break ; 	/* C complex */
		   case V4DPI_PntType_UOMPUOM:	i = 5 ; break ;		/* V = UOM per UOM */
		   case V4DPI_PntType_PntIdx:
						argpnts[1] = v4dpi_PntIdx_CvtIdxPtr(argpnts[1]->Value.IntVal) ; goto mult_top ;
		 } ;
		switch(argpnts[2]->PntType)
		 { default:			j = 1 ; break ; 	/* R */
		   case V4DPI_PntType_UOM:	j = 2 ; break ; 	/* U */
		   case V4DPI_PntType_UOMPer:	j = 3 ; break ; 	/* P */
		   case V4DPI_PntType_Complex:	j = 4 ; break ; 	/* C complex */
		   case V4DPI_PntType_UOMPUOM:	j = 5 ; break ;		/* V = UOM per UOM */
		   case V4DPI_PntType_PntIdx:
						argpnts[2] = v4dpi_PntIdx_CvtIdxPtr(argpnts[2]->Value.IntVal) ; goto mult_top ;
		 } ;
		ipt = respnt ;
		switch(i*10 + j)
		 { default:
			v_Msg(ctx,NULL,"ModArgCombo",intmodx,argpnts[1]->PntType,argpnts[1],argpnts[2]->PntType,argpnts[2]) ; RE1(0) ;
		   case 11: /* RR */
			tpt = (argpnts[2]->PntType != V4DPI_PntType_Int ? argpnts[2] : argpnts[1]) ;
			memcpy(ipt,tpt,tpt->Bytes) ;
			dnum = v4im_GetPointDbl(&ok,argpnts[2],ctx)*v4im_GetPointDbl(&ok,argpnts[1],ctx) ; ARGERR(1) ;
			if (fabs(dnum) <= gpi->DblEpsilonZero) dnum = 0.0 ;
			ok = TRUE ; goto percent_opts ;
		   case 12: /* RU */
			memcpy(ipt,argpnts[2],argpnts[2]->Bytes) ; memcpy(&dnum,&argpnts[2]->Value.UOMVal.Num,sizeof dnum) ;
			dnum *= v4im_GetPointDbl(&ok,argpnts[1],ctx) ; ARGERR(1) ;
			memcpy(&ipt->Value.UOMVal.Num,&dnum,sizeof dnum) ;
			break ;
		   case 13: /* RP */
			memcpy(ipt,argpnts[2],argpnts[2]->Bytes) ; memcpy(&dnum,&argpnts[2]->Value.UOMPerVal.Amount,sizeof dnum) ;
			dnum *= v4im_GetPointDbl(&ok,argpnts[1],ctx) ; ARGERR(1) ;
			memcpy(&ipt->Value.UOMPerVal.Amount,&dnum,sizeof dnum) ;
			break ;
		   case 14: /* RC */
			dnum = v4im_GetPointDbl(&ok,argpnts[1],ctx) ; ARGERR(1) ;
			memcpy(ipt,argpnts[2],argpnts[2]->Bytes) ;
			memcpy(&dnum1,&argpnts[2]->Value.Complex.r,sizeof dnum) ;
			memcpy(&dnum2,&argpnts[2]->Value.Complex.i,sizeof dnum) ;
			dnum1 *= dnum ; memcpy(&ipt->Value.Complex.r,&dnum1,sizeof dnum) ;
			dnum2 *= dnum ; memcpy(&ipt->Value.Complex.i,&dnum2,sizeof dnum) ;
			break ;
		   case 21: /* UR */
			memcpy(ipt,argpnts[1],argpnts[1]->Bytes) ; memcpy(&dnum,&argpnts[1]->Value.UOMVal.Num,sizeof dnum) ;
			dnum *= v4im_GetPointDbl(&ok,argpnts[2],ctx) ; ARGERR(2) ;
			memcpy(&ipt->Value.UOMVal.Num,&dnum,sizeof dnum) ;
			break ;
		   case 22: /* UU */
			v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_UOM,argpnts[1],V4DPI_PntType_UOM,argpnts[2]) ; RE1(0) ;
		   case 23: /* UP */
			memcpy(&dnum,&argpnts[1]->Value.UOMVal.Num,sizeof dnum) ;
			memcpy(&dnum1,&argpnts[2]->Value.UOMPerVal.Num,sizeof dnum) ;
			memcpy(&dnum2,&argpnts[2]->Value.UOMPerVal.Amount,sizeof dnum) ;
			dnum = (dnum1 == 0.0 ? 0 : dnum * dnum2 / dnum1) ;
			ZPH(ipt) ; ipt->PntType = V4DPI_PntType_Real ; ipt->Dim = Dim_Num ; ipt->Bytes = V4PS_Real ;
			PUTREAL(ipt,dnum) ; break ;
		   case 24: /* UC */
		   case 42: /* CU */
			v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_UOM,argpnts[1],V4DPI_PntType_Complex,argpnts[2]) ; RE1(0) ;
//			v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_UOM,V4DPI_PntType_Complex) ; RE1(0) ;
		   case 25: /* UV */
			memcpy(ipt,argpnts[1],V4DPI_PointHdr_Bytes) ;
			  ipt->Dim = argpnts[2]->Value.UOMPUOMVal.puomDim ;	/* Result takes on dimension PUOM "numerator" */
			ipt->Value.UOMVal = argpnts[2]->Value.UOMPUOMVal.UOM ;	/* but unit-of-measure of top portion of second argument */
			memcpy(&dnum,&argpnts[1]->Value.UOMVal.Num,sizeof dnum) ;
			memcpy(&dnum1,&argpnts[2]->Value.UOMPUOMVal.UOM.Num,sizeof dnum) ;
			memcpy(&dnum2,&argpnts[2]->Value.UOMPUOMVal.PUOM.Num,sizeof dnum) ;
			dnum = (dnum2 == 0.0 ? 0 : dnum * dnum1 / dnum2) ;
			memcpy(&ipt->Value.UOMVal.Num,&dnum,sizeof dnum) ; break ;
		   case 31: /* PR */
			memcpy(ipt,argpnts[1],argpnts[1]->Bytes) ; memcpy(&dnum,&argpnts[1]->Value.UOMPerVal.Amount,sizeof dnum) ;
			dnum *= v4im_GetPointDbl(&ok,argpnts[2],ctx) ; ARGERR(2) ;
			memcpy(&ipt->Value.UOMPerVal.Amount,&dnum,sizeof dnum) ;
			break ;
		   case 32: /* PU */
			memcpy(&dnum,&argpnts[2]->Value.UOMVal.Num,sizeof dnum) ;
			memcpy(&dnum1,&argpnts[1]->Value.UOMPerVal.Num,sizeof dnum) ;
			memcpy(&dnum2,&argpnts[1]->Value.UOMPerVal.Amount,sizeof dnum) ;
			dnum = (dnum1 == 0.0 ? 0 : dnum * dnum2 / dnum1) ;
			ZPH(ipt) ; ipt->PntType = V4DPI_PntType_Real ; ipt->Dim = Dim_Num ; ipt->Bytes = V4PS_Real ;
			PUTREAL(ipt,dnum) ; break ;
		   case 33: /* PP */
			v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_UOMPer,argpnts[1],V4DPI_PntType_UOMPer,argpnts[2]) ; RE1(0) ;
//			v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_UOMPer,V4DPI_PntType_UOMPer) ; RE1(0) ;
		   case 34: /* PC */
		   case 43: /* CP */
			v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_UOMPer,argpnts[1],V4DPI_PntType_Complex,argpnts[2]) ; RE1(0) ;
//			v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_UOMPer,V4DPI_PntType_Complex) ; RE1(0) ;
		   case 41: /* CR */
			dnum = v4im_GetPointDbl(&ok,argpnts[2],ctx) ; ARGERR(2) ;
			memcpy(ipt,argpnts[2],argpnts[2]->Bytes) ;
			memcpy(&dnum1,&argpnts[1]->Value.Complex.r,sizeof dnum) ;
			memcpy(&dnum2,&argpnts[1]->Value.Complex.i,sizeof dnum) ;
			dnum1 *= dnum ; memcpy(&ipt->Value.Complex.r,&dnum1,sizeof dnum) ;
			dnum2 *= dnum ; memcpy(&ipt->Value.Complex.i,&dnum2,sizeof dnum) ;
			break ;
		   case 44: /* CC */
			memcpy(ipt,argpnts[2],argpnts[2]->Bytes) ;
			{ double r1,r2,i1,i2 ;
			  memcpy(&r1,&argpnts[1]->Value.Complex.r,sizeof r1) ;
			  memcpy(&i1,&argpnts[1]->Value.Complex.i,sizeof r1) ;
			  memcpy(&r2,&argpnts[2]->Value.Complex.r,sizeof r1) ;
			  memcpy(&i2,&argpnts[2]->Value.Complex.i,sizeof r1) ;
			  dnum=r1*r2-i1*i2 ; memcpy(&ipt->Value.Complex.r,&dnum,sizeof dnum) ;
			  dnum=i1*r2+r1*i2 ; memcpy(&ipt->Value.Complex.i,&dnum,sizeof dnum) ;
			}
			break ;
		   case 52: /* VU */
			memcpy(ipt,argpnts[2],V4DPI_PointHdr_Bytes) ;
			  ipt->Dim = argpnts[1]->Value.UOMPUOMVal.puomDim ;	/* Result takes on dimension PUOM "numerator" */
			ipt->Value.UOMVal = argpnts[1]->Value.UOMPUOMVal.UOM ;	/* but unit-of-measure of top portion of first argument */
			memcpy(&dnum,&argpnts[2]->Value.UOMVal.Num,sizeof dnum) ;
			memcpy(&dnum1,&argpnts[1]->Value.UOMPUOMVal.UOM.Num,sizeof dnum) ;
			memcpy(&dnum2,&argpnts[1]->Value.UOMPUOMVal.PUOM.Num,sizeof dnum) ;
			dnum = (dnum2 == 0.0 ? 0 : dnum * dnum1 / dnum2) ;
			memcpy(&ipt->Value.UOMVal.Num,&dnum,sizeof dnum) ; break ;
		 } ;
		break ;
	   case V4IM_OpCode_Div:			/* [IntMod=Div Int=a Int=b ]	*/
		if (argpnts[1]->Grouping != V4DPI_Grouping_Single)
		 { v_Msg(ctx,NULL,"MathSngPoint",intmodx,argpnts[1]) ; RE1(0) ; } ;
div_top:	switch(argpnts[1]->PntType)
		 { default:			i = 1 ; break ; 	/* R egular */
		   case V4DPI_PntType_UOM:	i = 2 ; break ; 	/* U OM */
		   case V4DPI_PntType_UOMPer:	i = 3 ; break ; 	/* P er */
		   CASEofINT			i = 4 ; break ; 	/* I nt */
		   case V4DPI_PntType_Complex:	i = 5 ; break ; 	/* C complex */
		   case V4DPI_PntType_PntIdx:
						argpnts[1] = v4dpi_PntIdx_CvtIdxPtr(argpnts[1]->Value.IntVal) ; goto div_top ;
		 } ;
		switch(argpnts[2]->PntType)
		 { default:			j = 1 ; break ; 	/* R */
		   case V4DPI_PntType_UOM:	j = 2 ; break ; 	/* U */
		   case V4DPI_PntType_UOMPer:	j = 3 ; break ; 	/* P */
		   CASEofINT			j = 4 ; break ; 	/* I */
		   case V4DPI_PntType_Complex:	j = 5 ; break ; 	/* C complex */
		   case V4DPI_PntType_PntIdx:
						argpnts[2] = v4dpi_PntIdx_CvtIdxPtr(argpnts[2]->Value.IntVal) ; goto div_top ;
		 } ;
		ipt = respnt ; ok = TRUE ;
		switch(i*10 + j)
		 { case 12: /* RU */
			if (v4im_GetPointDbl(&ok,argpnts[1],ctx) != 0.0)
			 { v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_Real,argpnts[1],V4DPI_PntType_UOM,argpnts[2]) ; RE1(0) ; } ;
			if (!ok) goto div_fail ;
			memcpy(ipt,argpnts[2],argpnts[2]->Bytes) ; dnum = 0 ; memcpy(&ipt->Value.UOMVal.Num,&dnum,sizeof dnum) ;
			break ;
		   case 13: /* RP */
			 v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_Real,argpnts[1],V4DPI_PntType_UOMPer,argpnts[2]) ; RE1(0) ;
		   case 11: /* RR */
		   case 14: /* RI */
		   case 41: /* IR */
			dnum1 = v4im_GetPointDbl(&ok,argpnts[1],ctx) ; if (ok) dnum2 = v4im_GetPointDbl(&ok,argpnts[2],ctx) ; if (!ok) goto div_fail ;
			if (dnum2 == 0.0)
			 { if (trace & V4TRACE_Arith)
			    { v_Msg(ctx,UCTBUF1,"*TraceDivBy0",intmodx) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
//			      vout_UCText(VOUT_Trace,0,UClit("*Arith: Divide by zero\n")) ;
			      dnum = 0.0 ; ok = FALSE ;
			    } else { dnum = 0.0 ; ok = FALSE ; } ;
			 } else { dnum = dnum1/dnum2 ; } ;
			if (fabs(dnum) <= gpi->DblEpsilonZero) dnum = 0.0 ;
//			ZPH(ipt) ; ipt->PntType = V4DPI_PntType_Real ; ipt->Dim = Dim_Num ; ipt->Bytes = V4PS_Real ;
			dblPNT(ipt) ; goto percent_opts ;
		   case 15: /* RC */
		   case 45: /* IC */
			 v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_Real,argpnts[1],V4DPI_PntType_Complex,argpnts[2]) ; RE1(0) ;
		   case 21: /* UR */
		   case 24: /* UI */
			memcpy(ipt,argpnts[1],argpnts[1]->Bytes) ; memcpy(&dnum,&argpnts[1]->Value.UOMVal.Num,sizeof dnum) ;
			dnum /= v4im_GetPointDbl(&ok,argpnts[2],ctx) ; if (!ok) goto div_fail ;
			if (fabs(dnum) <= gpi->DblEpsilonZero) dnum = 0.0 ;
			memcpy(&ipt->Value.UOMVal.Num,&dnum,sizeof dnum) ;
			break ;
		   case 22: /* UU */
			memcpy(&dnum1,&argpnts[1]->Value.UOMVal.Num,sizeof dnum1) ;
			memcpy(&dnum2,&argpnts[2]->Value.UOMVal.Num,sizeof dnum2) ;
			if (dnum2 == 0.0)
			 { if (trace & V4TRACE_Arith)
			    { v_Msg(ctx,UCTBUF1,"*TraceDivBy0",intmodx) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
			      dnum = 0.0 ; ok = FALSE ;
			    } else { dnum = 0.0 ; ok = FALSE ; } ;
			 } else { dnum = dnum1/dnum2 ; } ;
			if (fabs(dnum) <= gpi->DblEpsilonZero) dnum = 0.0 ;
//			ZPH(ipt) ; ipt->PntType = V4DPI_PntType_Real ; ipt->Dim = Dim_Num ; ipt->Bytes = V4PS_Real ;
			dblPNT(ipt) ; goto percent_opts ;
		   case 23: /* UP */
			 v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_UOM,argpnts[1],V4DPI_PntType_UOMPer,argpnts[2]) ; RE1(0) ;
		   case 25: /* UC */
			 v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_UOM,argpnts[1],V4DPI_PntType_Complex,argpnts[2]) ; RE1(0) ;
		   case 31: /* PR */
		   case 34: /* PI */
			memcpy(ipt,argpnts[1],argpnts[1]->Bytes) ; memcpy(&dnum1,&argpnts[1]->Value.UOMPerVal.Amount,sizeof dnum) ;
			dnum2 = v4im_GetPointDbl(&ok,argpnts[2],ctx) ; if (!ok) goto div_fail ;
			if (dnum2 == 0.0)
			 { if (trace & V4TRACE_Arith)
			    { v_Msg(ctx,UCTBUF1,"*TraceDivBy0",intmodx) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
			      dnum = 0.0 ; ok = FALSE ;
			    } else { dnum = 0.0 ; ok = FALSE ; } ;
			 } else { dnum = dnum1/dnum2 ; } ;
			if (fabs(dnum) <= gpi->DblEpsilonZero) dnum = 0.0 ;
			memcpy(&ipt->Value.UOMPerVal.Amount,&dnum,sizeof dnum) ;
			break ;
		   case 32: /* PU */
			 v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_UOMPer,argpnts[1],V4DPI_PntType_UOM,argpnts[2]) ; RE1(0) ;
		   case 33: /* PP */
			 v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_UOMPer,argpnts[1],V4DPI_PntType_UOMPer,argpnts[2]) ; RE1(0) ;
		   case 35: /* PC */
			 v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_UOMPer,argpnts[1],V4DPI_PntType_Complex,argpnts[2]) ; RE1(0) ;
		   case 42: /* IU */
			if (argpnts[1]->Value.IntVal != 0)
			 { v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_Int,argpnts[1],V4DPI_PntType_UOM,argpnts[2]) ; RE1(0) ; } ;
			memcpy(ipt,argpnts[2],argpnts[2]->Bytes) ; dnum = 0 ; memcpy(&ipt->Value.UOMVal.Num,&dnum,sizeof dnum) ;
			break ;
		   case 43: /* IP */
			 v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_Int,argpnts[1],V4DPI_PntType_UOMPer,argpnts[2]) ; RE1(0) ;
		   case 44: /* II */
			tpt = (argpnts[2]->PntType == V4DPI_PntType_Int ? argpnts[1] : argpnts[2]) ;
			memcpy(ipt,tpt,tpt->Bytes) ;
			if (argpnts[2]->Value.IntVal == 0.0)
			 { if (trace & V4TRACE_Arith)
			    { v_Msg(ctx,UCTBUF1,"*TraceDivBy0",intmodx) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
			      dnum = 0.0 ; ok = FALSE ;
			    } else { dnum = 0.0 ; ok = FALSE ;} ;
			 } else { dnum = argpnts[1]->Value.IntVal/argpnts[2]->Value.IntVal ; } ;
			if (fabs(dnum) <= gpi->DblEpsilonZero) dnum = 0.0 ;
			goto percent_opts ;
		   case 51: /* CR */
		   case 54: /* CI */
			dnum = v4im_GetPointDbl(&ok,argpnts[2],ctx) ; if (!ok) goto div_fail ;
			memcpy(ipt,argpnts[1],argpnts[2]->Bytes) ;
			memcpy(&dnum1,&argpnts[1]->Value.Complex.r,sizeof dnum) ;
			memcpy(&dnum2,&argpnts[1]->Value.Complex.i,sizeof dnum) ;
			dnum1 /= dnum ; memcpy(&ipt->Value.Complex.r,&dnum1,sizeof dnum) ;
			dnum2 /= dnum ; memcpy(&ipt->Value.Complex.i,&dnum2,sizeof dnum) ;
			break ;
		   case 52: /* CU */
			v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_Complex,argpnts[1],V4DPI_PntType_UOM,argpnts[2]) ; RE1(0) ;
		   case 53: /* CP */
			v_Msg(ctx,NULL,"ModArgCombo",intmodx,V4DPI_PntType_Complex,argpnts[1],V4DPI_PntType_UOMPer,argpnts[2]) ; RE1(0) ;
		   case 55: /* CC */
			memcpy(ipt,argpnts[2],argpnts[2]->Bytes) ;
			{ double r,den,r0,r1,r2,i0,i1,i2 ;
			  memcpy(&r1,&argpnts[1]->Value.Complex.r,sizeof r1) ;
			  memcpy(&i1,&argpnts[1]->Value.Complex.i,sizeof r1) ;
			  memcpy(&r2,&argpnts[2]->Value.Complex.r,sizeof r1) ;
			  memcpy(&i2,&argpnts[2]->Value.Complex.i,sizeof r1) ;
			  if (fabs(r2) >= fabs(i2))
			   { r = i2 / r2 ; den = r2 + r * i2 ;
			     r0 = (r1 + r * i1)/den ; i0 = (i1 - r * r1) / den ;
			   } else
			   { r = r2 / i2 ; den = i2 + r * r2 ;
			     r0 = (r1 * r + i1) / den ; i0 = (i1 * r - r1) / den ;
			   } ;
			  memcpy(&ipt->Value.Complex.r,&r0,sizeof dnum) ;
			  memcpy(&ipt->Value.Complex.i,&i0,sizeof dnum) ;
			}
			break ;
		 } ;
		break ;
div_fail:	v_Msg(ctx,NULL,"ModInvArgNotNum",intmodx) ; RE1(0) ;
	   case V4IM_OpCode_DDiv:
//		ZPH(respnt) ; respnt->PntType = V4DPI_PntType_Foreign ; ipt = respnt ;
		ipt = respnt ; dblPNT(ipt) ;
		dnum = v4im_GetPointDbl(&ok,argpnts[1],ctx) ; if (ok) dnum1 = v4im_GetPointDbl(&ok,argpnts[2],ctx) ;
		if (!ok)
		 { v_Msg(ctx,NULL,"ModInvArgNotNum",intmodx,1) ; RE1(0) ; } ;
		if (dnum1 == 0.0)
		 { if (trace & V4TRACE_Arith)
		    { v_Msg(ctx,UCTBUF1,"*TraceDivBy0",intmodx) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
		      dnum = 0.0 ; ok = FALSE ; goto percent_opts ;
		    } else { dnum = 0.0 ; ok = FALSE ; goto percent_opts ; } ;
		 } else { dnum = dnum/dnum1 ; } ;
		ok = TRUE ; goto percent_opts ;
	   case V4IM_OpCode_Num:
		ipt = v4im_DoNum(ctx,respnt,argpnts,argcnt,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Counter:
		ipt = v4im_DoCounter(ctx,respnt,argpnts,argcnt,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Error:
		ipt = v4im_DoError(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Fail:
		ipt = v4im_DoFail(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
//		ZUS(UCTBUF2)
//		for(ix=1;ix<=argcnt;ix++)
//		 { v4im_GetPointUC(&ok,UCTBUF1,V4DPI_AlphaVal_Max,argpnts[ix],ctx) ;
//		   if (!ok) v4dpi_PointToString(UCTBUF1,argpnts[ix],ctx,V4DPI_FormatOpt_Trace) ;	/* If can't format nicely then format as V4 point */
//		   if (UCstrlen(UCTBUF2) + UCstrlen(UCTBUF1) < 500) UCstrcat(UCTBUF2,UCTBUF1) ;
//		 } ;
//		v_Msg(ctx,NULL,(argcnt == 0 ? "FailByUser2" : "FailByUser"),intmodx,UCTBUF2) ; REGISTER_ERROR(0) ;
//		UCstrcpy(gpi->SavedErrMsg,ctx->ErrorMsg) ;
//		ipt = NULL ; RETURNFAILURE ;
	   case V4IM_OpCode_Sqrt:
		switch (argpnts[1]->PntType)
		 { default:
			dnum = sqrt(v4im_GetPointDbl(&ok,argpnts[1],ctx)) ;
			if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
			ipt = respnt ; ZPH(ipt) ; ipt->PntType = V4DPI_PntType_Real ; ipt->Dim = Dim_Num ;
			ipt->Bytes = V4PS_Real ; PUTREAL(ipt,dnum) ;
			break ;
		   case V4DPI_PntType_Complex:
			{ double r,i,x,y,w ;
			  memcpy(&r,&ipt->Value.Complex.r,sizeof(dnum)) ;
			  memcpy(&i,&ipt->Value.Complex.i,sizeof(dnum)) ;
			  if (r == 0.0 && i == 0.0) { dnum1 = 0.0 ; dnum2 = 0.0 ; }
			   else { x = fabs(r) ; y = fabs(i) ;
				  if (x > y) { w = sqrt(x)*sqrt(0.5*(1.0+sqrt(1.0+(y/x)*(y/x)))) ; }
				   else { w = sqrt(y)*sqrt(0.5*((x/y)+sqrt(1.0+(x/y)*(x/y)))) ; } ;
				  if (r >= 0.0) { dnum1 = w ; dnum2 = i/(2.0*w) ; }
				   else { dnum2 = (i >= 0.0 ? w : -w) ; dnum1 = i / (2.0 * dnum2) ; } ;
				} ;
			}
			ipt = respnt ; memcpy(ipt,argpnts[1],V4PS_Complex) ;
			memcpy(&ipt->Value.Complex.r,&dnum1,sizeof(dnum1)) ; memcpy(&ipt->Value.Complex.i,&dnum2,sizeof(dnum2)) ;
			break ;
		 } ;
		break ;
	   case V4IM_OpCode_Percent:			/* Percent( num1 num2 ) */
//		ipt = respnt ; ZPH(ipt) ;  ipt->PntType = V4DPI_PntType_Foreign ;
		ipt = respnt ; dblPNT(ipt) ;
		dnum = v4im_GetPointDbl(&ok,argpnts[1],ctx) ; if (ok) dnum1 = v4im_GetPointDbl(&ok,argpnts[2],ctx) ;
		if (!ok) { v_Msg(ctx,NULL,"ModInvArgNotNum",intmodx) ; RE1(0) ; } ;
		if (dnum1 == 0.0) { dnum = 0.0 ; } else { dnum = 100.00 * (dnum/dnum1) ; } ;
		ok = TRUE ;
percent_opts:
		for(i=3;i<=argcnt;i++)			/* Maybe look for options? */
		 { switch (v4im_CheckPtArgNew(ctx,argpnts[i],&cpt,NULL))
		    { default:		v_Msg(ctx,NULL,"TagBadUse",intmodx) ; RE1(0) ;
		      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; RE1(0) ;
		      case V4IM_Tag_Error:
			if (ok) break ; 		/* Only look at if an error */
			memcpy(respnt,cpt,cpt->Bytes) ; ipt = NULL ; break ;
		      case V4IM_Tag_Round:
			j = v4im_GetPointInt(&ok,cpt,ctx) ; ARGERR(i) ;
			k = DtoI(dnum * powers[j]) ; dnum = (double)k / powers[j] ;			
			break ;
		    } ;
		 } ;
		if (ipt == NULL) { ipt = respnt ; }
		 else { v4im_SetPointValue(ctx,ipt,dnum) ; } ;		/* ipt set to NULL if Error::xxx */
		break ;
	   case V4IM_OpCode_PCChg1:			/* PCChg1( num1 num2 ) */
//		ipt = respnt ; ZPH(ipt) ;  ipt->PntType = V4DPI_PntType_Foreign ;
		ipt = respnt ; dblPNT(ipt) ;
		dnum = v4im_GetPointDbl(&ok,argpnts[1],ctx) ; if (ok) dnum1 = v4im_GetPointDbl(&ok,argpnts[2],ctx) ;
		if (!ok) { v_Msg(ctx,NULL,"ModInvArgNotNum",intmodx) ; RE1(0) ; } ;
		if (dnum == 0.0) { ok = FALSE ; goto percent_opts ; }
		 else { dnum = 100.00 * ((dnum1-dnum)/(dnum<0 ? -dnum : dnum)) ; } ;
		ok = TRUE ; goto percent_opts ;
	   case V4IM_OpCode_PCChg2:			/* PCChg2( num1 num2 ) */
//		ipt = respnt ; ZPH(ipt) ;  ipt->PntType = V4DPI_PntType_Foreign ;
		ipt = respnt ; dblPNT(ipt) ;
		dnum = v4im_GetPointDbl(&ok,argpnts[1],ctx) ; if (ok) dnum1 = v4im_GetPointDbl(&ok,argpnts[2],ctx) ;
		if (!ok) { v_Msg(ctx,NULL,"ModInvArgNotNum",intmodx) ; RE1(0) ; } ;
		if (dnum1 == 0.0) { dnum = 0.0 ; ok = FALSE ; goto percent_opts ; }
		 else { dnum = 100.00 * ((dnum1-dnum)/(dnum1 < 0 ? -dnum1 : dnum1)) ; } ;
		ok = TRUE ; goto percent_opts ;
	   case V4IM_OpCode_Self:			/* Identity( point ) */
		ipt = respnt ;
		memcpy(ipt,argpnts[1],argpnts[1]->Bytes) ;
/*		If we got a second argument & it is a point on Dim:Dim then update result to that dimension */
		if (argcnt > 1 ? (argpnts[2]->Dim == Dim_Dim) : FALSE) ipt->Dim = argpnts[2]->Value.IntVal ;
		break ; 				/* (doesn't get much easier than this) */
	   case V4IM_OpCode_MthMod:			/* MthMod( n1 n2 ) */
		i = v4im_GetPointInt(&ok,argpnts[1],ctx) ; if (ok) j = v4im_GetPointInt(&ok,argpnts[2],ctx) ;
		if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		ipt = respnt ;
		if (j == 0) { v_Msg(ctx,NULL,"MthModZero",intmodx,argpnts[2]) ; RE1(0) ; } ;
		intPNTv(ipt,(i % j)) ;
		break ;
	   case V4IM_OpCode_MakeQIsct:			/* MakeQIsct( pt pt ... ) */
		ipt = v4im_DoMakeI(ctx,respnt,argcnt,argpnts,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_MakeIsct:			/* MakeIsct( pt pt ... ) */
		ipt = v4im_DoMakeI(ctx,respnt,argcnt,argpnts,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_MakeT:					/* MakeT( IntMod:xxx pt ... ) */
		ipt = v4im_DoMakeT(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_MakeIn:		/* MakeIn(dim point [tags]) */
		ipt = v4im_DoMakeIn(ctx,respnt,argcnt,argpnts,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Dim:
		ipt = v4im_DoDim(ctx,respnt,argcnt,argpnts,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Message:
		ipt = v4im_DoMessage(ctx,respnt,argcnt,argpnts,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Eval:
		ipt = v4im_DoEval(ctx,respnt,argcnt,argpnts,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
//	   case V4IM_OpCode_Summary:
//		ipt = v4im_DoSummary(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
//		if (ipt == NULL) goto intmod_end1 ;
//		break ;
	   case V4IM_OpCode_IsctVals:			/* IsctVals( @[isct] int ) */
		ok = TRUE ; num = (argcnt > 1 ? v4im_GetPointInt(&ok,argpnts[2],ctx) : UNUSED) ; ARGERR(2) ;
		if (num == 0)				/* If (arg #2)is -1 then return NoNest value */
		 { ipt = v4dpi_IsctEval(respnt,argpnts[1],ctx,V4DPI_EM_EvalQuote+V4DPI_EM_NoNest,NULL,NULL) ;
		   break ;
		 } ;
		el = (struct V4DPI__EvalList *)v4mm_AllocChunk(sizeof *el,TRUE) ;	/* Allocate bytes for multiple searches */
		el->Init = FALSE ; el->SkipCnt = 0 ; el->ReturnCnt = 0x7fff ;
		if (argcnt > 1)
		 { el->SkipCnt = num - 1 ;
		   ipt = argpnts[1] ;
		   ipt = v4dpi_IsctEval(&isctbuf,ipt,ctx,V4DPI_EM_EvalQuote,NULL,el) ;
		   break ;
		 } ;
		INITLP(respnt,lp1,Dim_List)
		for(i=0;;i++)
		 { ipt = argpnts[1] ;
		   el->Init = FALSE ; el->SkipCnt = i ; el->ReturnCnt = 0x7fff ;
		   ipt = v4dpi_IsctEval(&isctbuf,ipt,ctx,V4DPI_EM_EvalQuote,NULL,el) ;
		   if (ipt == NULL) break ;				/* All done - quit with what we got */
		   if (!v4l_ListPoint_Modify(ctx,lp1,V4L_ListAction_Append,ipt,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto intmod_fail ; } ;
		 } ;
		ENDLP(respnt,lp1) ipt = respnt ;
		break ;
	   case V4IM_OpCode_Remove_Point:		/* [IntMod=Remove_Point point ] */
		ipt = argpnts[1] ;
		switch(ipt->PntType)
		 { default:	v4_error(V4E_RMVPNTTYPE,0,"V4IM","Eval","RMVPNTTYPE","Cannot Remove point of this type") ;
//		   case V4DPI_PntType_Binding:
//			if (!v4dpi_BindListRemove(ctx,(struct V4DPI__BindPointVal *)&ipt->Value)) ipt = NULL ;
//			break ;
		 } ;
		break ;
	   case V4IM_OpCode_And:			/* And( arg1, arg2, ... ) */
		if (argcnt == 0) { logPNTv(respnt,TRUE) ; return(respnt) ; } ;
		if (argpnts[1]->PntType == V4DPI_PntType_List && argcnt == 2)		/* If argument is list then handle bit-wize */
		 { if (!v4l_BitMapAnd(ctx,intmodx,respnt,argpnts[1],argpnts[2]))
		    { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		   ipt = respnt ; break ;
		 } ;
		for (ok=TRUE,i=TRUE,j=1;ok && i && j<=argcnt;j++)
		 { ipt = argpnts[j] ;
		   ipt = v4dpi_IsctEval(&isctbuf,ipt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (ipt == NULL) { v_Msg(ctx,NULL,"ModArgEval",intmodx,j,argpnts[j]) ; RE1(0) ; } ;
		   if (ipt->PntType == V4DPI_PntType_List)
		    { lp = v4im_VerifyList(&ptbuf,ctx,ipt,intmodx) ;
		      for(k=1;i && v4l_ListPoint_Value(ctx,lp,k,&isctbuf) > 0;k++) { i = (i && PNTlogVAL(&ok,&isctbuf,ctx)) ; } ;
		    } else { i = (i && PNTlogVAL(&ok,ipt,ctx)) ; } ;
		 } ;
		if (!ok)
		 { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		ipt = (P *)(i ? &Log_True : &Log_False) ;
		break ;
	   case V4IM_OpCode_Or: 		/* [IntMod=Or arg1 arg2 ...] */
		if (argcnt == 0) { logPNTv(respnt,FALSE) ; return(respnt) ; } ;
		if (argpnts[1]->PntType == V4DPI_PntType_List && argcnt == 2)		/* If argument is list then handle bit-wize */
		 { if (!v4l_BitMapOr(ctx,intmodx,respnt,argpnts[1],argpnts[2]))
		    { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		   ipt = respnt ; break ;
		 } ;
		for (ok=TRUE,i=FALSE,j=1;ok && !i && j<=argcnt;j++)
		 { ipt = argpnts[j] ;
		   ipt = v4dpi_IsctEval(&isctbuf,ipt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (ipt == NULL) { v_Msg(ctx,NULL,"ModArgEval",intmodx,j,argpnts[j]) ; RE1(0) ; } ;
		   if (ipt->PntType == V4DPI_PntType_List)
		    { lp = v4im_VerifyList(&ptbuf,ctx,ipt,intmodx) ;
		      for(k=1;!i && v4l_ListPoint_Value(ctx,lp,k,&isctbuf) > 0;k++) { i = (i || v4im_GetPointLog(&ok,&isctbuf,ctx)) ; } ;
		    } else
		    { if (PNTlogVAL(&ok,ipt,ctx)) { i = TRUE ; break ; } ; } ;
		 } ;
		if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		ipt = (P *)(i ? &Log_True : &Log_False) ;
		break ;
	   case V4IM_OpCode_Not:			/* [IntMod=Not arg1 resultpt] */
		if (argpnts[1]->PntType == V4DPI_PntType_List)		/* If argument is list then handle bit-wize */
		 { if (!v4l_BitMapNot(ctx,intmodx,respnt,argpnts[1]))
		    { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		   ipt = respnt ; break ;
		 } ;
		ipt = argpnts[1] ; i = !(PNTlogVAL(&ok,ipt,ctx)) ; /* Do logical AND */
		if (!ok)
		 { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		ipt = (P *)(i ? &Log_True : &Log_False) ;
		break ;
	   case V4IM_OpCode_NOp:			/* NOp( logical ) */
		ipt = (P *)&Log_True ;
		if (argcnt > 0) 			/* If have arg & is FALSE then NOp return undefined! */
		 { if (argpnts[1]->Value.IntVal < 1) ipt = NULL ;
/*		   If argument is PI then force access violation (for testing trapping, etc.) */
		   if (argpnts[1]->Value.IntVal == 314159) { bp = NULL ; *bp = 'x' ; } ;
		 } ;
		if (ipt == NULL) { v_Msg(ctx,NULL,"NOpFail",intmodx,argpnts[1]) ; RE1(0) ; } ;
		break ;
	   case V4IM_OpCode_Unicode:
		ipt = v4im_DoUnicode(ctx,respnt,argcnt,argpnts,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_UnQuote:				/* Evaluate - remove quote on 1st argument */
		if (ISQUOTED(argpnts[1]))
		 { UNQUOTECOPY(respnt,argpnts[1]) ; }
		 else { memcpy(respnt,argpnts[1],argpnts[1]->Bytes) ; } ;
		return(respnt) ;
	   case V4IM_OpCode_Quote:				/* Quote - just return 1st argument */
		memcpy(respnt,argpnts[1],argpnts[1]->Bytes) ;
		QUOTE(respnt) ;
		return(respnt) ;
	   case V4IM_OpCode_TEQ:				/* TEQ(value eq result eq result ...) */
		cpt = v4dpi_IsctEval(&isctbuf,argpnts[1],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		if (cpt == NULL)
		 { v_Msg(ctx,NULL,"ModArgEval",intmodx,1,argpnts[1]) ; RE1(0) ; } ;
		if (cpt->PntType == V4DPI_PntType_Shell) cpt = (P *)&cpt->Value ;
		switch(cpt->PntType)
		 { default:
			v_Msg(ctx,NULL,"ModArgPntType",intmodx,1,cpt,cpt->PntType) ; RE1(0) ;
		   CASEofINT
		   case V4DPI_PntType_Calendar:
		   case V4DPI_PntType_Real:
		   case V4DPI_PntType_Color:
		   case V4DPI_PntType_Country:
		   case V4DPI_PntType_XDict:
		   case V4DPI_PntType_Dict:
			ok = TRUE ; dnum = (cpt->PntType == V4DPI_PntType_Real ? v4im_GetPointDbl(&ok,cpt,ctx) : cpt->Value.IntVal) ; ARGERR(1) ;
			for(i=2;i<=argcnt;i+=2)
			 { 
//v_Msg(ctx,NULL,"@%1d / %2d %3P %4P\n",i,argcnt,cpt,argpnts[i]) ; vout_UCText(VOUT_Trace,0,ctx->ErrorMsg) ;
			   if (i == argcnt) { i -= 1 ; break ; } ;	/* If last argument then return, all others false */ 
			   ipt = v4dpi_IsctEval(respnt,argpnts[i],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
			   if (ipt == NULL) { v_Msg(ctx,NULL,"ModArgEval",intmodx,i,argpnts[i]) ; RE1(0) ; } ;
			   switch (ipt->PntType)
			    { default:
			       switch (ipt->Grouping)
				{ default:
				  pim = (struct V4DPI__Point_IntMix *)&ipt->Value ; k = FALSE ;
				  for(j=0;j<ipt->Grouping;j++)
				   { if (dnum >= pim->Entry[j].BeginInt && dnum <= pim->Entry[j].EndInt)
				      { k = TRUE ; break ; } ;
				   } ; break ;
				  case V4DPI_Grouping_Single:	k = (dnum == ipt->Value.IntVal) ; break ;
				  case V4DPI_Grouping_GT:	k = (dnum > ipt->Value.IntVal) ; break ;
				  case V4DPI_Grouping_GE:	k = (dnum >= ipt->Value.IntVal) ; break ;
				  case V4DPI_Grouping_LT:	k = (dnum < ipt->Value.IntVal) ; break ;
				  case V4DPI_Grouping_LE:	k = (dnum <= ipt->Value.IntVal) ; break ;
				  case V4DPI_Grouping_NE:	k = (dnum != ipt->Value.IntVal) ; break ;
				  case V4DPI_Grouping_All:	k = (ipt->Dim == cpt->Dim) ; break ;
				} ; break ;
			      case V4DPI_PntType_Calendar:
			      case V4DPI_PntType_Real:
			       switch (ipt->Grouping)
				{ default:
				  prm = (struct V4DPI__Point_RealMix *)&ipt->Value ; k = FALSE ;
				  for(j=0;j<ipt->Grouping;j++)
				   { memcpy(&dnum1,&prm->Entry[j].BeginReal,sizeof dnum1) ;
				     memcpy(&dnum2,&prm->Entry[j].EndReal,sizeof dnum2) ;
				     if (dnum >= dnum1 && dnum <= dnum2) { k = TRUE ; break ; } ;
				   } ; break ;
				  case V4DPI_Grouping_Single:	GETREAL(dnum1,ipt) ; k = (dnum == dnum1) ; break ;
				  case V4DPI_Grouping_GT:	GETREAL(dnum1,ipt) ; k = (dnum > dnum1) ; break ;
				  case V4DPI_Grouping_GE:	GETREAL(dnum1,ipt) ; k = (dnum >= dnum1) ; break ;
				  case V4DPI_Grouping_LT:	GETREAL(dnum1,ipt) ; k = (dnum < dnum1) ; break ;
				  case V4DPI_Grouping_LE:	GETREAL(dnum1,ipt) ; k = (dnum <= dnum1) ; break ;
				  case V4DPI_Grouping_NE:	GETREAL(dnum1,ipt) ; k = (dnum != dnum1) ; break ;
				  case V4DPI_Grouping_All:	k = (ipt->Dim == cpt->Dim) ; break ;
				} ; break ;
			    } ;
			   if (k) break ;
			  } ;
			break ;
		 } ;
		if (i > argcnt) 				/* Nothing matched, no "default" */
		 { v_Msg(ctx,NULL,"TEQNoResult",intmodx,argpnts[1]) ; RE1(0) ; } ;
		ipt = v4dpi_IsctEval(respnt,argpnts[i+1],ctx,0,NULL,NULL) ;
		break ;
	   case V4IM_OpCode_If: 			/* If(logical Then::xxx Else/ElseIf::xxx) */
	   case V4IM_OpCode_Ifnot:
		k = PNTlogVAL(&j,argpnts[1],ctx) ; if (intmodx == V4IM_OpCode_Ifnot) k = !k ;
		if (!j) { v_Msg(ctx,NULL,"ModInvArg",intmodx,1) ; RE1(0) ;
		 } ;
then_else:
/*		Common entry to handle then elseif else logic */
/*		k = true/false based on first argument */
		if (argpnts[2]->PntType != V4DPI_PntType_TagVal)	/* Don't have Then::xxx & Else::xxx */
		 { 
//		   switch (argpnts[2]->PntType == V4DPI_PntType_Dict ? v4im_GetDictToEnumVal(ctx,argpnts[2]) : -1)
//		    { case _Then:
//		      case _Else:
//		      case _ElseIf:	v_Msg(ctx,NULL,"ThenElse",intmodx,2) ; RE1(0) ;
//		    } ;
		   if (k) { ipt = argpnts[2] ; }
		    else { if (argcnt > 2) { ipt = argpnts[3] ; } else { ipt = (P *)&protoNone ; break ; } ;
			 } ;
		   ipt = v4dpi_IsctEval(respnt,ipt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   break ;
		 } ;
//		ipt = (k ? &Log_True : &Log_False) ;
		ipt = (P *)&protoNone ;
		for(i=2;i<=argcnt;i++)
		 { switch (v4im_CheckPtArgNew(ctx,argpnts[i],&tpt,NULL))
		    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; RE1(0) ;
		      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; RE1(0) ;
		      case V4IM_Tag_Then:
			if (k)
			 { ipt = v4dpi_IsctEval(respnt,tpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
			   if (ipt == NULL)
			    { v_Msg(ctx,NULL,"IfResFail",intmodx,V4IM_Tag_Then) ; RE1(0) ; } ;
			   goto intmod_end ;
			 } ;
			break ;
		      case V4IM_Tag_Else:
			if (!k)
			 { ipt = v4dpi_IsctEval(respnt,tpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
			   if (ipt == NULL)
			    { v_Msg(ctx,NULL,"IfResFail",intmodx,V4IM_Tag_Else) ; RE1(0) ; } ;
			   goto intmod_end ;
			 } ;
			break ;
		      case V4IM_Tag_ElseIf:
			k = v4im_GetPointLog(&j,tpt,ctx) ;
			if (!j) { v_Msg(ctx,NULL,"ModInvTagVal",intmodx,V4IM_Tag_ElseIf,i) ; RE1(0) ; } ;
			break ;
		    } ;
		 } ;
		break ;
	   case V4IM_OpCode_NDefQ:
	   case V4IM_OpCode_DefQ:
		goto def_entry ;
	   case V4IM_OpCode_Def:			/* [IntMod=IfDefined isct [iftrue] iffalse] */
def_entry:	ipt = argpnts[1] ; ok = ISQUOTED(ipt) ;	/* Is (arg #1) quoted? */
		if (ipt->PntType == V4DPI_PntType_List)
		 { lp = v4im_VerifyList(&ptbuf,ctx,ipt = argpnts[1],intmodx) ;
		 } else { lp = NULL ; } ;
		k = TRUE ;					/* Assume TRUE for now */
		for(i=1;;i++)
		 { if (lp == NULL && i>1) break ;		/* No list - go thru once */
		   if ( lp != NULL) { if (v4l_ListPoint_Value(ctx,lp,i,&isctbuf) <= 0) break ; ipt = &isctbuf ; } ;
		   switch (ipt->PntType)
		    { default:
			v_Msg(ctx,NULL,"ModArgQIsctDim",intmodx,i) ; RE1(0) ;
		      case V4DPI_PntType_Special:
			if (ipt->Grouping == V4DPI_Grouping_Current)
			 { DIMVAL(ipt,ctx,ipt->Dim) ;
			   k = (ipt != NULL) ;
			 } else if (ipt->Grouping == V4DPI_Grouping_PCurrent)
			 { DIMPVAL(ipt,ctx,ipt->Dim) ; k = (ipt != NULL) ; }
			 else { v_Msg(ctx,NULL,"ModArgQIsctDim",intmodx,i) ; RE1(0) ; } ;
			break ;
		      case V4DPI_PntType_Isct:
			/* Don't want TRACE- will generate incorrect traceback if undefined! */
			/* VEH040924 - Turn off V4TRACE_Errors - may generate too many traces */
			ctx->disableFailures = TRUE ;
			ipt = v4dpi_IsctEval(respnt,ipt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
			ctx->disableFailures = FALSE ;		/* Don't want to keep this on very long */
			if((evalmode & V4DPI_EM_SavedErrSet) == 0) ZUS(gpi->SavedErrMsg) ;	/* If an error then reset saved msg (VEH050517) */
			if (ipt == NULL || ok ? FALSE : ipt->PntType == V4DPI_PntType_Isct) { i-- ; continue ; } ;	/* Keep evaluating if isct */
			k = (ipt != NULL) ;		/* Set k to TRUE if result is defined */
			break ;
		    } ;
		   if (!k) break ;
		 } ;
/*		If no args then assume we got: DefQ(pt Then::True Else::False)
		  if one arg then	     : DefQ(pt Then::pt Else::arg)
		  if two args then	     : DefQ(pt Then::arg1 Else::arg2)
		  if one tag then	     : DefQ(pt Then::pt Else::fail) (overlay with Then/Else supplied)
		  if two tags then	     : DefQ(pt Then::xxx Else::yyy)		*/
		if (intmodx == V4IM_OpCode_NDefQ) k = !k ;		/* If NDefQ() then compliment result */
		if (argcnt < 2 ? TRUE : argpnts[2]->PntType != V4DPI_PntType_TagVal)	/* Don't have Then::xxx & Else::xxx */
		 { switch (argcnt)
		    { default:	ipt = (k ? argpnts[2] : argpnts[3]) ;		/* Have defined/notdefine values */
				ipt = v4dpi_IsctEval(respnt,ipt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
				break ;
		      case 2:	if (k) break ;					/* Pick 2nd if 1st undefined */
				ipt = v4dpi_IsctEval(respnt,argpnts[2],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
				break ;
		      case 1:	ipt = (P *)(k ? &Log_True : &Log_False) ; break ;	/* Only one arg - return T/F */
		    } ;
		   if (ipt == NULL) { v_Msg(ctx,NULL,"NDefQ1stArg",intmodx,argpnts[1]) ; RE1(0) ; } ;
		   break ;
		 } ;
		goto then_else ;						/* Use common then/elseif/else handler */
	   case V4IM_OpCode_nMT:
	   case V4IM_OpCode_MT:
		if (argcnt == 0) { logPNTv(respnt,intmodx==V4IM_OpCode_MT) ; return(respnt) ; } ;
		ipt = argpnts[1] ;
mt_entry:
//		if (ipt->PntType == V4DPI_PntType_Special && ipt->Grouping == V4DPI_Grouping_None)
//		 { logPNTv(respnt,intmodx==V4IM_OpCode_MT) ; return(respnt) ; } ;
		switch(ipt->PntType)			/* Check for special points first */
		 { case V4DPI_PntType_Isct:
			k = (ipt->Grouping == 0) ; goto mt_then_else ;
		   case V4DPI_PntType_Special:
			if (ipt->Grouping == V4DPI_Grouping_None)
			 { k = TRUE ; goto mt_then_else ; } ;
			v_Msg(ctx,NULL,"ModArgPntType",intmodx,1,ipt,ipt->PntType) ; goto intmod_fail ;
		   case V4DPI_PntType_Shell:
			ipt = (P *)&ipt->Value ;
			goto mt_entry ;
		 } ;
/*		Here to check for real values */
		if (ipt->Grouping != V4DPI_Grouping_Single)
		 { k = FALSE ; goto mt_then_else ; } ;
		switch(ipt->PntType)
		 { default:
			v_Msg(ctx,NULL,"ModArgPntType",intmodx,1,ipt,ipt->PntType) ; goto intmod_fail ;
		   case V4DPI_PntType_Dict:
//			k = (memcmp(ipt,&protoNone,protoNone.Bytes) == 0) ; break ;
			if (memcmp(ipt,&protoNone,protoNone.Bytes) == 0) { k = TRUE ; }			/* UV4:none is empty */
			 else { k = UCempty(v4dpi_RevDictEntryGet(ctx,ipt->Value.IntVal)) ; } ;		/* If printable version is empty then result is TRUE */
			break ;
		   case V4DPI_PntType_XDict:
			k = UCempty(v4dpi_RevXDictEntryGet(ctx,ipt->Dim,ipt->Value.IntVal)) ;		/* If printable version is empty then result is TRUE */
			break ;
		   CASEofINT
		   case V4DPI_PntType_Color:
			k = (ipt->Value.IntVal == 0) ; break ;
		   CASEofDBL
			GETREAL(dnum,ipt) ; k = (dnum == 0.0) ; break ;
		   CASEofCharmU
			if (CHARISEMPTY(ipt)) { k = TRUE ; break ; } ;
			k = TRUE ;
			for(i=1;i<=CHARSTRLEN(ipt);i++)
			 { if (!vuc_IsWSpace(ipt->Value.AlphaVal[i])) { k = FALSE ; break ; } ;
			 } ;
			break ;
		   case V4DPI_PntType_UCChar:
			if (UCCHARISEMPTY(ipt)) { k = TRUE ; break ; }
			k = TRUE ;
			for(i=1;i<=UCCHARSTRLEN(ipt);i++)
			 { if (!vuc_IsWSpace(ipt->Value.UCVal[i])) { k = FALSE ; break ; } ;
			 } ;
			break ;
		   case V4DPI_PntType_BigText:
			{ UCCHAR *b ; k = TRUE ;
			  b = v4_BigTextCharValue(ctx,ipt) ;
			  for(i=0;b[i]!=UCEOS;i++)
			   { if (!vuc_IsWSpace(ipt->Value.UCVal[i])) { k = FALSE ; break ; } ;
			   } ;
			}
			break ;
		   case V4DPI_PntType_List:
			lp = VERIFYLIST(&ptbuf,ctx,ipt,intmodx) ; if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList",intmodx,1) ; goto intmod_fail ; } ;
			k = (SIZEofLIST(lp) == 0) ;
			break ;
		   case V4DPI_PntType_TeleNum:
			k = (ipt->Value.Tele.IntDialCode <= 1 && ipt->Value.Tele.AreaCode == 0 && ipt->Value.Tele.Number == 0) ;
			break ;
		   case V4DPI_PntType_UOM:
			memcpy(&dnum,&ipt->Value.UOMVal.Num,sizeof dnum) ; k = (dnum == 0) ; break ;
		   case V4DPI_PntType_UOMPer:
			memcpy(&dnum,&ipt->Value.UOMPerVal.Num,sizeof dnum) ; k = (dnum == 0) ; break ;
		   case V4DPI_PntType_UOMPUOM:
			memcpy(&dnum,&ipt->Value.UOMPUOMVal.UOM.Num,sizeof dnum) ; k = (dnum == 0) ; break ;
		   case V4DPI_PntType_GeoCoord:
			k = (ipt->Value.GeoCoord.Coord1 == 0 && ipt->Value.GeoCoord.Coord2 == 0 && ipt->Value.GeoCoord.Coord3 == 0 && ipt->Value.GeoCoord.TimeZone == 0) ;
			break ;
		   case V4DPI_PntType_Complex:
			k = (ipt->Value.Complex.r == 0 && ipt->Value.Complex.i == 0) ; break ;
//		   case V4DPI_PntType_MDArray:
		   case V4DPI_PntType_Tree:
			k = FALSE ; break ;	/* How to decide if tree is empty?? */
		   case V4DPI_PntType_Country:
			k = (ipt->Value.IntVal == UNUSED) ; break ;
		   case V4DPI_PntType_Drawer:
			{ struct V4IM__Drawer *drw ;
			  drw = v4im_GetDrawerPtr(ipt->Dim,ipt->Value.IntVal) ;
			  k = (drw->Points == 0) ;
			}
			break ;
		   
		 } ;
mt_then_else:
		if (intmodx == V4IM_OpCode_nMT) k = !k ;
		if (argcnt > 1)	goto then_else ;
		logPNTv(respnt,k) ; return(respnt) ;
	   case V4IM_OpCode_OSExt:			/* OSExt( args ) */
		ipt = v4im_DoOSExt(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_OSInfo:			/* OSExt( args ) */
		ipt = v4im_DoOSInfo(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_OSFile:			/* OSFile( file, args ) */
		ipt = v4im_DoOSFile(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_ReAllocate:
		ipt = v4im_DoReAllocate(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_DMCluster:
		ipt = v4im_DoDMCluster(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_GraphConnect:
		ipt = v4im_DoGraphConnect(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Output:
		if (gpi->RestrictionMap & V_Restrict_redirection) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto intmod_fail ; } ;
		ipt = v4im_DoOutput(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_GuiAlert:
//		if (gpi->RestrictionMap & V_Restrict_Gui) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto intmod_fail ; } ;
		ipt = v4im_DoGuiAlert(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_GuiMsgBox:
//		if (gpi->RestrictionMap & V_Restrict_Gui) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto intmod_fail ; } ;
		ipt = v4im_DoGuiMsgBox(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
#ifdef WANTHTMLMODULE
	   case V4IM_OpCode_HTML:			/* HTML( args ) */
		ipt = v4im_DoHTML(ctx,respnt,argcnt,argpnts,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
#endif
	   case V4IM_OpCode_HTTP:			/* HTML( args ) */
#if defined HAVECURL || defined HAVEWININET
		ipt = v4im_DoHTTP(ctx,respnt,argcnt,argpnts,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
#else
		v_Msg(ctx,NULL,"NYIonthisOS",intmodx) ; RE1(0) ;
#endif
	   case V4IM_OpCode_FTP:			/* FTP( args ) */
#if defined HAVECURL || defined HAVEWININET
		ipt = v4im_DoFTP(ctx,respnt,argcnt,argpnts,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
#else
		v_Msg(ctx,NULL,"NYIonthisOS",intmodx) ; RE1(0) ;
#endif
	   case V4IM_OpCode_SendMail:			/* SendMail( args ) */
#if defined HAVECURL
		ipt = v4im_DoSendMail(ctx,respnt,argcnt,argpnts,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
#else
		v_Msg(ctx,NULL,"NYIonthisOS",intmodx) ; RE1(0) ;
#endif
	   case V4IM_OpCode_Names:			/* Names( args ) */
		ipt = v4im_DoNames(ctx,respnt,argcnt,argpnts,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Drawer:			/* Drawer( args ) */
		ipt = v4im_DoDrawer(ctx,respnt,argcnt,argpnts,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
#ifdef WANTHTMLMODULE
	   case V4IM_OpCode_XMLRPC:			/* XMLRPC( args ) */
		ipt = v4im_DoXMLRPC(ctx,respnt,argcnt,argpnts,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
#endif
	   case V4IM_OpCode_Socket:			/* Socket( args ) */
		ipt = v4im_DoSocket(ctx,respnt,argcnt,argpnts,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_UOM:
		ipt = v4im_DoUOM(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_nStr:
		ipt = v4im_DoStr(ctx,respnt,argpnts,argcnt,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		if (ipt->PntType == V4DPI_PntType_Logical || ipt->PntType == V4DPI_PntType_Int)
		 logPNTv(ipt,(ipt->Value.IntVal <= 0)) ;
		break ;
	   case V4IM_OpCode_Str:
		ipt = v4im_DoStr(ctx,respnt,argpnts,argcnt,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_NGram:
		ipt = v4im_DoNGram(ctx,respnt,argpnts,argcnt,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_JSON:
		ipt = v4im_DoJSON(ctx,respnt,argpnts,argcnt,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_JSONRef:
		ipt = v4im_DoJSONRef(ctx,respnt,argpnts,argcnt,intmodx,FALSE) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_JSONRefDim:
		ipt = v4im_DoJSONRef(ctx,respnt,argpnts,argcnt,intmodx,TRUE) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Lock:
		ipt = v4im_DoLock(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Parse:
		ipt = v4im_DoParse(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		break ;
	   case V4IM_OpCode_Spawn:
//		if (gpi->RestrictionMap & V_Restrict_Spawn) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto intmod_fail ; } ;
		ipt =  v4im_DoSpawn(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_EnumX:
		goto enum_entry ;
	   case V4IM_OpCode_Enum:
enum_entry:
//		ipt = v4im_DoEnum(ctx,respnt,argpnts,argcnt,intmodx,trace,intmodx == V4IM_OpCode_EnumX) ;
		ipt = v4im_DoEnum(ctx,respnt,argpnts,argcnt,intmodx,trace,TRUE) ;	//veh070215 - Force to extended mode
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_EnumCL:					/* EnumCL( list , [isct] , optResDim ) */
		ipt = v4im_DoEnumCL(ctx,respnt,argpnts,argcnt,intmodx,trace,NULL,NULL) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Try:					/* Try( isct isct ... Catch::exceptname isct ...) */
		ipt = v4im_DoTry(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Throw:					/* Throw( exceptname contextpt contextpt ...) */
		ipt = v4im_DoThrow(ctx,respnt,argpnts,argcnt,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Pack1616:					/* Pack1616( left16, right16, resultdim ) */
		ipt = respnt ;
		intPNTv(ipt,(argpnts[1]->Value.IntVal & 0xFFFF) * 0x10000 + (argpnts[2]->Value.IntVal & 0xFFFF)) ;
		break ;
	   case V4IM_OpCode_Sum:
		cpt = respnt ;
		dimid = UNUSED ; j = UNUSED ; b64 = 0 ; dnum = 0 ; num = 0 ;
		if (argcnt > 1)
		 { if (argpnts[2]->Value.IntVal < 0 || argpnts[2]->Value.IntVal > 0xffff) { di = NULL ; }
		    else { DIMINFO(di,ctx,argpnts[2]->Value.IntVal) ; } ;
		   if (di == NULL) { v_Msg(ctx,NULL,"ModArgNotDim",intmodx,2,argpnts[2]) ; RE1(0) ; } ;
		   dimid = di->DimId ; type = di->PointType ; bytes = gpi->PointBytes[type] ;
		 } ;
		lp = v4im_VerifyList(&ptbuf,ctx,argpnts[1],intmodx) ;
		if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList",intmodx,1) ; goto intmod_fail ; } ;
		for(i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
		 { if (isctbuf.PntType == V4DPI_PntType_Isct)	/* If an ISCT then evaluate it */
		    { ipt = v4dpi_IsctEval(cpt,&isctbuf,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ; }
		    else { ipt = &isctbuf ; } ;
		   if (ipt == NULL) { v_Msg(ctx,NULL,"ModArgEval",intmodx,i,&isctbuf) ; RE1(0) ; } ;
		   if (dimid == UNUSED) { dimid = ipt->Dim ; type = ipt->PntType ; } ;
		   switch(ipt->PntType)
		    { default:
			if (j != UNUSED)
			 { if (v4im_GetPointDbl(&ok,ipt,ctx) == 0.0) { if (ok) break ; } ;
			   v_Msg(ctx,NULL,"SumUOMScaler",intmodx,i) ; RE1(0) ;
			 } ;
			dnum += v4im_GetPointDbl(&ok,ipt,ctx) ;
//if (i <= eclCnt)
//printf("eclArray[%d]=%g, Sum[%d]=%g, Total=%g %s\n",i,eclArray[i-1],i,v4im_GetPointDbl(&ok,ipt,ctx),dnum,(eclArray[i-1] == v4im_GetPointDbl(&ok,ipt,ctx) ? "" : "***")) ;
//printf("%d\\%.15g\\%.15g\n",i,v4im_GetPointDbl(&ok,ipt,ctx),dnum) ;
			if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i) ; RE1(0) ; } ;
			break ;
		      case V4DPI_PntType_Fixed:
			if (j != UNUSED) { v_Msg(ctx,NULL,"SumUOMScaler",intmodx,i) ; RE1(0) ; } ;
			for(;num<ipt->LHSCnt;num++) { b64 *= 10 ; } ;
			b64 += v4im_GetPointFixed(&ok,ipt,ctx,num) ;
			break ;
		      case V4DPI_PntType_UOM:
			if (j == UNUSED)
			 { j = ipt->Value.UOMVal.Ref ; k = ipt->Value.UOMVal.Index ; rowdim = ipt->Dim ;
			 } else
			 { if (j != ipt->Value.UOMVal.Ref)
			    { memcpy(&dnum1,&ipt->Value.UOMVal.Num,sizeof dnum1) ;
			      if (dnum1 == 0.0) continue ;		/* May be different UOM but 0 is always 0, let it go */
			      if (!v4dpi_UOMAttemptCoerce(ctx,trace,j,UNUSED,ipt,&dnum1))
			       { v_Msg(ctx,NULL,"UOMAllSameList",intmodx,1,i,argpnts[1]) ; RE1(0) ; } ;
			      dnum += dnum1 ; continue ;
			    } ;
			   if (ipt->Value.UOMVal.Index < k) k = ipt->Value.UOMVal.Index ;
			 } ;
			memcpy(&dnum1,&ipt->Value.UOMVal.Num,sizeof dnum1) ;
			dnum += dnum1 ; break ;
		    } ;
		 } ;

		if (j != UNUSED)				/* Returning UOM ? */
		 { ipt = respnt ; ZPH(ipt) ; ipt->Dim = rowdim ; ipt->PntType = V4DPI_PntType_UOM ;
		   ipt->Bytes = V4PS_UOM ; ipt->Value.UOMVal.Ref = j ; ipt->Value.UOMVal.Index = k ;
		   memcpy(&ipt->Value.UOMVal.Num,&dnum,sizeof dnum) ;
		   break ;
		 } ;
		ZPH(cpt) ;
		if (type == V4DPI_PntType_Fixed)
		 { if (dnum != 0)
		    { dnum += (dnum < 0 ? -vRound[num] : vRound[num]) ;
		      dnum = dnum * powers[num] ;
		      b64 += (B64INT)dnum ;
		    } ;
		   if (argcnt > 1) { SCALEFIX(b64,num,di->Decimals) ; num = di->Decimals ; } ;
		   memcpy(&cpt->Value.FixVal,&b64,sizeof b64) ;
		   cpt->LHSCnt = num ; cpt->Dim = dimid ; cpt->PntType = type ; cpt->Bytes = V4PS_Fixed ;
		 } else
		 { if (b64 != 0) { dnum += (double)b64 / powers[num] ; } ;
		   if (argcnt > 1) { cpt->PntType = type ; cpt->Dim = dimid ; cpt->Bytes = bytes ;}
		    else if (dnum == DtoI(dnum)) { intPNTv(cpt,DtoI(dnum)) ; }
		    else { dblPNTv(cpt,dnum) ; } ;
//printf("dnum\\\\%.15g\n",dnum) ;
//		   v4im_SetPointValue(ctx,cpt,dnum) ;
//v_Msg(ctx,NULL,"@cpt\\\\%1P\n",cpt) ; vout_UCText(VOUT_Trace,0,ctx->ErrorMsg) ;
		 } ;
		ipt = cpt ;
		break ;
	   case V4IM_OpCode_Average:
		dnum = 0.0 ; num = 0 ; j = UNUSED ;
		for(i=1;i<=argcnt;i++)
		 { ipt = argpnts[i] ;
		   if (ipt->PntType == V4DPI_PntType_List)
		    { lp = ALIGNLP(&ipt->Value) ;
		      for(i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
		       { ipt = &isctbuf ;
			 if (j == UNUSED) { j = ipt->PntType ; resdim = ipt->Dim ; k = ipt->PntType ; } ;
			 if (resdim != ipt->Dim) { resdim = UNUSED ; } ;
			 num++ ; dnum += v4im_GetPointDbl(&ok,ipt,ctx) ; if (!ok) break ;
		       } ;
		    } else
		    { num ++ ; dnum += v4im_GetPointDbl(&ok,ipt,ctx) ;
		      if (j == UNUSED) { j = ipt->PntType ; resdim = ipt->Dim ; } ;
		      if (resdim != ipt->Dim) { resdim = UNUSED ; } ;
		    } ;
		   if (!ok) break ;
		 } ;
		if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		ipt = respnt ; ZPH(ipt) ;
//		if (resdim == Dim_Int) resdim = UNUSED ;
//		if (resdim != UNUSED) { ipt->Dim = resdim ; ipt->PntType = k ; ipt->Bytes = V4PS_Real ; }
//		 else { ipt->PntType = xxV4DPI_PntType_Foreign ; } ;
//		v4im_SetPointValue(ctx,ipt,(num == 0 ? 0.0 : dnum/(double)num)) ;
		dnum = (num == 0 ? 0.0 : dnum/(double)num) ; dblPNTv(ipt,dnum) ;
		break ;
	   case V4IM_OpCode_Min:					/* [IntMod:Min list ] */
		if (argpnts[1]->PntType == V4DPI_PntType_List)
		 { ipt = v4stat_Arg1List(ctx,respnt,v4stat_Min,intmodx,argpnts,argcnt,trace) ;
		   if (ipt == NULL) goto intmod_end1 ;
		 } else
		 { dnum = DBL_MAX ; ipt = NULL ; 
		   for(i=1;i<=argcnt;i++)
		    { cpt = argpnts[i] ; dnum1 = v4im_GetPointDbl(&ok,cpt,ctx) ; if (!ok) break ;
		      if (dnum1 < dnum) { dnum = dnum1 ; ipt = respnt ; memcpy(respnt,cpt,cpt->Bytes) ; } ;
		    } ;
		   if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		 } ;
		break ;
	   case V4IM_OpCode_Max:					/* [IntMod:Max list ] */
		if (argpnts[1]->PntType == V4DPI_PntType_List)
		 { ipt = v4stat_Arg1List(ctx,respnt,v4stat_Max,intmodx,argpnts,argcnt,trace) ;
		   if (ipt == NULL) goto intmod_end1 ;
		 } else
		 { dnum = -DBL_MAX ; ipt = NULL ;
		   for(i=1;i<=argcnt;i++)
		    { cpt = argpnts[i] ; dnum1 = v4im_GetPointDbl(&ok,cpt,ctx) ; if (!ok) break ;
		      if (dnum1 > dnum) { dnum = dnum1 ; ipt = respnt ; memcpy(respnt,cpt,cpt->Bytes) ; } ;
		    } ;
		   if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		 } ;
		break ;
	   case V4IM_OpCode_Optimize:
		ipt = v4im_DoOptimize(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_ChemOpt:
//		ipt = v4im_DoChemOpt(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Abs:
		ipt = respnt ; memcpy(ipt,argpnts[1],argpnts[1]->Bytes) ;
		switch(ipt->PntType)
		 { default:	v_Msg(ctx,NULL,"ModArgPntType",intmodx,1,ipt,ipt->PntType) ; RE1(0) ;
		   CASEofINT
			if (ipt->Value.IntVal < 0) ipt->Value.IntVal = -ipt->Value.IntVal ; break ;
		   case V4DPI_PntType_UOM:
			memcpy(&dnum,&ipt->Value.UOMVal.Num,sizeof dnum) ;
			if (dnum < 0) { dnum = -dnum ; memcpy(&ipt->Value.UOMVal.Num,&dnum,sizeof dnum) ; } ;
			break ;
		   case V4DPI_PntType_UOMPer:
			memcpy(&dnum,&ipt->Value.UOMPerVal.Amount,sizeof dnum) ;
			if (dnum < 0) { dnum = -dnum ; memcpy(&ipt->Value.UOMPerVal.Amount,&dnum,sizeof dnum) ; } ;
			break ;
		   case V4DPI_PntType_Fixed:
			memcpy(&b64,&ipt->Value.RealVal,sizeof b64) ;
			if (b64 < 0) { b64 = -b64 ; memcpy(&ipt->Value.RealVal,&b64,sizeof b64) ; } ;
			break ;
		   case V4DPI_PntType_Real:
			GETREAL(dnum,ipt) ;
			if (dnum < 0) { dnum = -dnum ; PUTREAL(ipt,dnum) ; } ;
			break ;
		   case V4DPI_PntType_Complex:
			dnum = v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(1) ;
			ipt = respnt ; dblPNTv(ipt,dnum) ;
//			ZPH(ipt) ; ipt->PntType = V4DPI_PntType_Real ; ipt->Dim = Dim_Num ;
//			ipt->Bytes = V4PS_Real ; memcpy(&ipt->Value.RealVal,&dnum,sizeof dnum) ;
			break ;

		 } ; break ;
	   case V4IM_OpCode_StatRan:			/* StatRan() */
		ipt = v4stat_DoStatRan(ctx,respnt,argpnts,argcnt,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_BindEQ:			/* BindEQ( [isct] value ) */
	   case V4IM_OpCode_BindQQ:			/* BindQQ( [isct] value ) */
	   case V4IM_OpCode_BindQE:			/* BindQ( [isct] value ) */
	   case V4IM_OpCode_BindEE:			/* Bind( [isct] value ) */

#ifdef OLDBINDQE
		ipt = argpnts[1] ;
#ifdef NEWQUOTE
		if (ISQUOTED(ipt))
		 { ipt = UNQUOTEPTR(ipt) ; } ;	/* VEH100604 - Allow BindEE(MakeQI(xxxx) xxx) to work (should be BindEE(MakeI(xxx) xxx)) */
#endif
		if (ipt->PntType != V4DPI_PntType_Isct && ipt->PntType != V4DPI_PntType_QIsct) { v_Msg(ctx,NULL,"ModArgQIsct",intmodx,1) ; RE1(0) ; } ;
		bindingBuf = (struct V4DPI__Binding *)v4mm_AllocChunk(sizeof *bindingBuf,FALSE) ;
//v_Msg(ctx,NULL,"@*%1M(%2P %3P)\n",intmodx,argpnts[1],argpnts[2]) ; vout_UCText(VOUT_Trace,0,ctx->ErrorMsg) ;
		if (!v4dpi_BindListMake(bindingBuf,argpnts[2],ipt,ctx,NULL,NOWGTADJUST,0,DFLTRELH))
		 { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		v4mm_FreeChunk(bindingBuf) ;
		ipt = respnt ; memcpy(ipt,argpnts[2],argpnts[2]->Bytes) ;
		break ;
#endif

	      { COUNTER aCnt = 0, wgt = NOWGTADJUST ; INDEX relH = DFLTRELH ;
		static struct V4DPI__Binding *bindingBuf = NULL ;

		ipt = NULL ; vpt = NULL ;
		for(ix=1,ok=TRUE;ok&&ix<=argcnt;ix++)
		 { if (argpnts[ix]->PntType != V4DPI_PntType_TagVal)
		    { switch (++aCnt)
		       { default:
			   v_Msg(ctx,NULL,"BindTMArgs",intmodx,2) ; RE1(0) ;
		         case 1:
			   ipt = argpnts[aCnt] ;
#ifdef NEWQUOTE
			   if (ISQUOTED(ipt))
			    { ipt = UNQUOTEPTR(ipt) ; } ;	/* VEH100604 - Allow BindEE(MakeQI(xxxx) xxx) to work (should be BindEE(MakeI(xxx) xxx)) */
#endif
			   if (ipt->PntType != V4DPI_PntType_Isct && ipt->PntType != V4DPI_PntType_QIsct)
			    { v_Msg(ctx,NULL,"ModArgQIsct",intmodx,1) ; RE1(0) ; } ;
			   break ;
		         case 2:
			   vpt = argpnts[aCnt] ; break ;
		       } ;
		      continue ;
		    } ;
		   switch(v4im_CheckPtArgNew(ctx,argpnts[ix],&cpt,&ptbuf))
		    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; RE1(0) ;
		      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; RE1(0) ;
		      case V4IM_Tag_Out:
			for(relH=gpi->LowHNum;relH<=gpi->HighHNum;relH++)
			 { if (gpi->RelH[relH].areaPId.Bytes == 0) continue ;
			   if (memcmp(cpt,&gpi->RelH[relH].areaPId,gpi->RelH[relH].areaPId.Bytes) == 0) break ;
			 } ; if (relH > gpi->HighHNum) { v_Msg(ctx,NULL,"AreaNoId",intmodx,V4IM_Tag_Id,cpt) ; RE1(0) ; } ;
			if (!gpi->RelH[relH].ahi.BindingsUpd) { v_Msg(ctx,NULL,"BindNoUpd",gpi->RelH[relH].pcb->UCFileName) ; RE1(0) ; } ;
			break ;
		      case V4IM_Tag_Weight:
			if (cpt->PntType == V4DPI_PntType_Delta)
			 { wgt = cpt->Value.IntVal ;
			   if (wgt >= V4DPI_BindWgt_PrecOffset) { v_Msg(ctx,NULL,"CmdBindAdj1",intmodx,V4IM_Tag_Weight,argpnts[ix],V4DPI_BindWgt_PrecOffset) ; RE1(0) ; } ;
			 } else { wgt = v4im_GetPointInt(&ok,cpt,ctx) ; if (!ok) break ; if (wgt > 0) wgt += V4DPI_BindWgt_PrecOffset ; } ;
			break ;
		    } ;
		 } ;
		if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ix-1) ; RE1(0) ; } ;
		if (vpt == NULL) { v_Msg(ctx,NULL,"BindNEArgs",intmodx,2) ; RE1(0) ; } ;
		if (bindingBuf == NULL) bindingBuf = (struct V4DPI__Binding *)v4mm_AllocChunk(sizeof *bindingBuf,FALSE) ;
		ok = v4dpi_BindListMake(bindingBuf,vpt,ipt,ctx,NULL,wgt,0,relH) ;
		if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
	      }
		ipt = respnt ; memcpy(ipt,vpt,vpt->Bytes) ;
		break ;

	   case V4IM_OpCode_Flatten:
		ipt = v4im_DoFlatten(ctx,respnt,argpnts,argcnt,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
//	   case V4IM_OpCode_Make_ListProduct:		/* Make_ListProduct( resultdim , list1, ... listn ) */
#ifdef MAKELISTPRODUCT
		ipt = argpnts[1] ;				/* Get resulting dimension */
		ISVALIDDIM(ipt->Value.IntVal,1,"ListProduct()") ;
		DIMINFO(di,ctx,ipt->Value.IntVal) ;
		if (di == NULL) { v_Msg(ctx,NULL,"ModArgNotDim",intmodx,1,ipt) ; RE1(0) ; } ;
		if (di->PointType != V4DPI_PntType_List)
		 { sprintf(ctx->ErrorMsg,"%s Dimension %s is not type %s",v4im_ModFailStr(intmodx),di->DimName,"List") ; RE1(0) ; } ;
		cpt = respnt ;
		ZPH(cpt) ;
		lp = ALIGNLP(&cpt->Value) ;	/* Link up list structure */
		memset(lp,0,(char *)&lp->Buffer - (char *)lp) ;
		lp->ListType = V4L_ListType_Point ;
		cpt->PntType = di->PointType ; cpt->Dim = di->DimId ;
		for(ix=1;ix<=argcnt;ix++) { indexes[ix] = 1 ; } ;	/* Set all interation indexes to 1 */
		for(;;)
		 { lp1 = ALIGNLP(&isctbuf.Value) ;	/* Set up list for next "product" */
		   memset(lp1,0,(char *)&lp1->Buffer - (char *)lp1) ;
		   lp1->ListType = V4L_ListType_Point ; lp1->Dim = lp->Dim ;
		   ZPH(&isctbuf) ;
		   for(ix=2;ix<=argcnt;ix++)
		    { bpnt = argpnts[ix] ;				/* Get list point */
		      lp2 = ALIGNLP(&bpnt->Value) ;	/* Link up to list */
		      if (v4l_ListPoint_Value(ctx,lp2,indexes[ix],&ptbuf) == 0) break ;	/* Get a value or end of list? */
		      if (!v4l_ListPoint_Modify(ctx,lp1,V4L_ListAction_Append,&ptbuf,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto intmod_fail ; } ;
		    } ;
		   if (ix <= argcnt)					/* If terminated early then have to bump indexes */
		    { if (ix == 2) goto end_product ;
		      indexes[ix-1] ++ ; for(;ix<=argcnt;ix++) { indexes[ix] = 1 ; } ;
		    } else
		    { indexes[argcnt] ++ ;				/* Got good product, append to master point */
		      isctbuf.Bytes = (char *)&isctbuf.Value.AlphaVal[lp1->Bytes] - (char *)&isctbuf ;
		      isctbuf.PntType = di->PointType ; isctbuf.Dim = di->DimId ;
		      if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&isctbuf,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto intmod_fail ; } ;
		    } ;
		   if (lp->Bytes > V4DPI_AlphaVal_Max)
		    { sprintf(ctx->ErrorMsg,"%s Exceeded max list length in ListProduct",v4im_ModFailStr(intmodx)) ; RE1(0) ; } ;
		 } ;
end_product:
		ipt = respnt ;
		ipt->Bytes = (char *)&ipt->Value.AlphaVal[lp->Bytes] - (char *)ipt ;
#endif
		break ;
#ifdef WANTSET
	   case V4IM_OpCode_Set:				/* List( ... ) */
		ipt = v4im_DoSet(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
#endif
	   case V4IM_OpCode_ListNE:				/* ListNE( ... ) */
	   case V4IM_OpCode_List:				/* List( ... ) */
		ipt = v4im_DoList(ctx,respnt,argpnts,argcnt,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_MakePm:				/* MakeP( Dim:dim value ) */
		ipt = v4im_DoMakePm(ctx,respnt,argcnt,argpnts,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_MakeP:				/* MakeP( Dim:dim value ) */
		ipt = v4im_DoMakeP(ctx,respnt,argcnt,argpnts,intmodx,&i) ;
		if (ipt == NULL && i) goto Coerce_entry ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Project:				/* Project( args ) */
		ipt = v4im_DoProject(ctx,respnt,argpnts,argcnt,intmodx,&i) ;
		if (!i) 					/* If i = TRUE then go into Coerce() */
		 { if (ipt == NULL) goto intmod_end1 ;
		   break ;
		 } ;
		goto Coerce_entry ;
	   case V4IM_OpCode_Coerce:				/* Coerce( Dim:dim value ) */
Coerce_entry:
		DIMINFO(di,ctx,argpnts[1]->Value.IntVal) ;
		if (di == NULL) { v_Msg(ctx,NULL,"ModArgNotDim",intmodx,1,argpnts[1]) ; RE1(0) ; } ;
		ipt = argpnts[2] ;
		if ((di->Flags & V4DPI_DimInfo_Dim) != 0) /* Making point on Dim (i.e. another dimension?) */
		 { memcpy(respnt,argpnts[1],argpnts[1]->Bytes) ; 
		   respnt->Value.IntVal = (ipt->Dim == 0 && ipt->PntType == V4DPI_PntType_Isct ? Dim_Isct : ipt->Dim) ;
		   DIMINFO(di,ctx,(ipt->Dim == 0 ? Dim_Isct : ipt->Dim)) ;
#ifdef V4_BUILD_SECURITY
		   if (di == NULL ? TRUE : (di->rtFlags & V4DPI_rtDimInfo_Hidden))
		    { v_Msg(ctx,NULL,"DimNotDim1",intmodx,ipt) ; RE1(0) ; } ;
#endif
		   ipt = respnt ; break ;
		 }
		if (ipt->PntType != V4DPI_PntType_Special && ipt->Grouping != V4DPI_Grouping_Single)
		 { switch (di->PointType)
		    { case V4DPI_PntType_List:
		      case V4DPI_PntType_AggRef:
		      case V4DPI_PntType_Shell:
		      case V4DPI_PntType_UDT:
		      CASEofChar			goto coerce_branch ;
		    } ;
		   if (di->PointType != ipt->PntType && ipt->PntType != V4DPI_PntType_Special)
		    { v_Msg(ctx,NULL,"ProjectMP",intmodx,ipt,di->DimId) ; RE1(0) ; } ;
		   memcpy(respnt,ipt,ipt->Bytes) ;  respnt->Dim = di->DimId ; ipt = respnt ; break ;
		 } ;
/*		If dim has Acceptor & source is a string then blow through acceptor logic */
		if ((ipt->PntType == V4DPI_PntType_Char || ipt->PntType == V4DPI_PntType_UCChar) && (di->Flags & V4DPI_DimInfo_Acceptor))
		 { v4im_GetPointUC(&ok,UCTBUF1,V4DPI_AlphaVal_Max,ipt,ctx) ;
		   if (di->ADPnt.Bytes != 0)
		    { v_Msg(ctx,UCTBUF2,"@[UV4:Acceptor %1U Alpha:\"%2U\"]",di->ADPntStr,UCTBUF1) ;
		    } else
		    { v_Msg(ctx,UCTBUF2,"@[UV4:Acceptor Dim:%1U Alpha:\"%2U\"]",(UCstrlen(di->IsA) > 0 ? di->IsA : di->DimName),UCTBUF1) ;
		    } ;
		   ipt = v4im_CoerceStringToPoint(ctx,respnt,intmodx,UCTBUF2,di) ;
		   break ;
		 } ;
/*		di = ptr to dimension of 1st argument, ipt = 2nd argument */
coerce_branch:
		if (v4im_DoCoerce(ctx,respnt,di,argpnts[1],ipt)) { ipt = respnt ; break ; } ;
		goto intmod_fail ;
	   case V4IM_OpCode_MakeQL:
	   case V4IM_OpCode_MakeL:				/* MakeL( pt pt ... ) */
		INITLP(respnt,lp1,Dim_List) ;
		for(i=1;i<=argcnt;i++)
		 { ipt = argpnts[i] ;
		   if (memcmp(ipt,&protoNone,V4PS_Int) == 0) continue ;	/* Don't format UuserOptStringnone point */
		   if (memcmp(ipt,&protoQNone,V4PS_Int) == 0)
		    { UNQUOTE(ipt) ; } ;
		   if (!v4l_ListPoint_Modify(ctx,lp1,V4L_ListAction_Append,ipt,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto intmod_fail ; } ;
		 } ;
		ENDLP(respnt,lp1) ;
		if (intmodx == V4IM_OpCode_MakeQL) { QUOTE(respnt) ; } ;
		ipt = respnt ;
		break ;
	   case V4IM_OpCode_MakeLC:				/* MakeLC( condition point condition point ... ) */
		INITLP(respnt,lp1,Dim_List) ;
		for(i=1;i<=argcnt;i+=2)
		 { k = v4im_GetPointLog(&j,argpnts[i],ctx) ;
		   if (!j) { v_Msg(ctx,NULL,"ModInvArg",intmodx,i) ; goto intmod_fail ; } ;
		   if (!k) continue ;
		   ipt = v4dpi_IsctEval(&ptbuf,argpnts[i+1],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (ipt == NULL) { v_Msg(ctx,NULL,"ModArgEval",intmodx,i+1,argpnts[i+1]) ; RE1(0) ; } ;
		   if (!v4l_ListPoint_Modify(ctx,lp1,V4L_ListAction_Append,ipt,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto intmod_fail ; } ;
		 } ;
		ENDLP(respnt,lp1) ; ipt = respnt ;
		break ;
	   case V4IM_OpCode_Sort:				/* Sort( list, [optionalorder], @[isct], ... ) */
		ipt = v4im_DoSort(ctx,respnt,argpnts,argcnt,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_AggPut:				/* AggPut( key point ... ) */
	      {	int mustbenew = TRUE ; int keyix,ax ;
		ipt = respnt ; intPNT(ipt) ;
		i = j = k = FALSE ; vpt = NULL ; ok = TRUE ; ax = DFLTAGGAREA ; keyix = 0 ;
		for(ix=1;ok&&ix<=argcnt;ix++)	/* Loop thru any/all begining tagged values */
		 { if (argpnts[ix]->PntType != V4DPI_PntType_TagVal) break ;
		   switch(v4im_CheckPtArgNew(ctx,argpnts[ix],&cpt,&ptbuf))
		    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; RE1(0) ;
		      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; RE1(0) ;
		      case V4IM_Tag_Agg:	vpt = cpt ; keyix = ix ; break ;
		      case -V4IM_Tag_Create:	k = TRUE ; break ;
		      case V4IM_Tag_FAgg:	vpt = cpt ; i = TRUE ; keyix = ix ; break ;
		      case V4IM_Tag_BFAgg:	vpt = cpt ; i = TRUE ; keyix = ix ; j = TRUE ; break ;
		      case V4IM_Tag_New:	mustbenew = v4im_GetPointLog(&ok,cpt,ctx) ; break ;
		      case V4IM_Tag_Out:
			for(ax=0;ax<gpi->AreaAggCount;ax++)
			 { if (gpi->AreaAgg[ax].aggPId.Bytes == 0) continue ; if (memcmp(&gpi->AreaAgg[ax].aggPId,cpt,gpi->AreaAgg[ax].aggPId.Bytes) == 0) break ;
			 } ; if (ax >= gpi->AreaAggCount) { v_Msg(ctx,NULL,"AreaNoId",intmodx,V4IM_Tag_Id,cpt) ; goto intmod_fail ; } ;
			if ((gpi->AreaAgg[ax].pcb->AccessMode & V4IS_PCB_AM_Insert) == 0) { v_Msg(ctx,NULL,"AggNotUpd1",intmodx,gpi->AreaAgg[ax].pcb->UCFileName) ; goto intmod_fail ; } ;
		    } ;
		 } ;
		if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,ix-1) ; RE1(0) ; } ;
		if (vpt == NULL) { v_Msg(ctx,NULL,"AggNoKey",intmodx) ; RE1(0) ; } ;
		if (vpt->Dim == Dim_Int)	/* If first point's value is on Dim:Int then want key to be int'th argument */
		 { if (vpt->Value.IntVal < 1 || vpt->Value.IntVal > (argcnt - ix + 1))
		    { v_Msg(ctx,NULL,"AggPutArg",intmodx,argpnts[keyix],Dim_Int,vpt->Value.IntVal,argcnt-ix+1) ; RE1(0) ; } ;
		   ipt = v4dpi_IsctEval(&isctbuf,argpnts[ix + vpt->Value.IntVal - 1],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (ipt == NULL) { v_Msg(ctx,NULL,"ModArgEval2",intmodx,argpnts[ix + vpt->Value.IntVal - 1]) ; RE1(0) ; } ;
		   vpt = ipt ;
		 } else if (vpt->PntType == V4DPI_PntType_Special && vpt->Grouping == V4DPI_Grouping_New)
		 { DIMINFO(di,ctx,vpt->Dim) ;
		   if (!di->UniqueOK) { v_Msg(ctx,NULL,"DimNoNew",intmodx,vpt->Dim) ; RE1(0) ; } ;
		   vpt = respnt ; ZPH(vpt) ; vpt->PntType = di->PointType ; vpt->Bytes = V4PS_Int ; vpt->Dim = di->DimId ;
		   vpt->Value.IntVal = v4dpi_DimUnique(ctx,di,NULL) ;
		 } ;
		if (k)				/* Auto-create the point ? */
		 { DIMINFO(di,ctx,vpt->Dim) ;
		   if (di == NULL ? TRUE : di->UniqueOK != V4DPI_DimInfo_UOkPoint) { v_Msg(ctx,NULL,"DimNoNew",intmodx,vpt->Dim) ; RE1(0) ; } ;
		   if (!v4dpi_AddDUPoint(ctx,di,vpt,UNUSED,V_DUPDICTACT_Err)) { v_Msg(ctx,NULL,"AggPutPutErr",intmodx,vpt) ; RE1(0) ; } ;
		 } ;
		i = v4im_AggPut(ctx,vpt,argpnts,argcnt,ix,intmodx,i,j,mustbenew,ax) ;
		ipt = respnt ; intPNTv(ipt,i) ;
		if (ipt->Value.IntVal == -9999)
		 { v_Msg(ctx,NULL,"AggPutPutErr",intmodx,vpt) ; RE1(0) ; } ;
	      }
		break ;
	   case V4IM_OpCode_AggUpd:				/* AggUpd( key dim index value ) */
		i = v4im_GetPointInt(&ok,argpnts[3],ctx) ; ARGERR(3) ;
		if (v4im_AggUpdValue(ctx,argpnts[1],argpnts[2],i,argpnts[4]))
		 { ipt = (P *)&Log_True ; break ; } ;
		v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ;
	   case V4IM_OpCode_AggDel:				/* AggUpd( key ) */
		if (v4im_AggDelete(ctx,argpnts[1]))
		 { ipt = (P *)&Log_True ; break ; } ;
		v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ;
	   case V4IM_OpCode_AggVal:				/* AggVal( key dim index [tagged..] ) */
		if (argcnt == 2)
		 { ipt = v4eval_AggCAggGet(ctx,argpnts[1],argpnts[2]->Value.IntVal,trace) ;
		   if (ipt != NULL) break ;
		   v_Msg(ctx,NULL,"AggValCAgg",intmodx) ; RE1(0) ;
		 } ;
		ipt = respnt ;
		ipt = v4im_AggGetValue(ctx,argpnts[1],argpnts[2],argpnts[3]->Value.IntVal,ipt) ;
		if (ipt == NULL)
		 { for(i=4;i<=argcnt;i++)	/* Look for Error tag to supply default value */
		    { switch (v4im_CheckPtArgNew(ctx,argpnts[i],&cpt,NULL))
		       { default:		break ;
		         case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; RE1(0) ;
		         case V4IM_Tag_QIsct:
		         case V4IM_Tag_Error:	ipt = v4dpi_IsctEval(respnt,cpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ; break ;
		       } ;
		    } ;
		   if (ipt == NULL) { v_Msg(ctx,NULL,"AggValGetErr",intmodx) ; RE1(0) ; } ;
		   if (argcnt == 4) break ;
		 } ;
		if (argcnt == 3) break ;			/* No additional arguments */
/*		Set up to return logical off of integer AggVal (in j) */
		for(i=4,ok=TRUE;ok && i<=argcnt;i++)
		 { 
		   switch (v4im_CheckPtArgNew(ctx,argpnts[i],&cpt,&isctbuf))
		    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; RE1(0) ;
		      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; RE1(0) ;
		      case V4IM_Tag_Nth:
			j = v4im_GetPointInt(&ok,ipt,ctx) ; ARGERR(1) ; k = v4im_GetPointInt(&ok,cpt,ctx) ; ARGERR(i) ;
			logPNTv(ipt,((j & (1 << (k-1))) != 0)) ; break ;
		      case V4IM_Tag_QIsct:
		      case V4IM_Tag_Error:
			break ;		/* Has already been handled or not necessary */
		      case V4IM_Tag_LT:
			dnum1 = v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(1) ; dnum2 = v4im_GetPointDbl(&ok,cpt,ctx) ; ARGERR(i) ;
			logPNTv(ipt,dnum1 < dnum2) ; break ;
		      case V4IM_Tag_LE:
			dnum1 = v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(1) ; dnum2 = v4im_GetPointDbl(&ok,cpt,ctx) ; ARGERR(i) ;
			logPNTv(respnt,dnum1 <= dnum2) ; break ;
		      case V4IM_Tag_EQ:
			dnum1 = v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(1) ; dnum2 = v4im_GetPointDbl(&ok,cpt,ctx) ; ARGERR(i) ;
			logPNTv(respnt,dnum1 == dnum2) ; break ;
		      case V4IM_Tag_GE:
			dnum1 = v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(1) ; dnum2 = v4im_GetPointDbl(&ok,cpt,ctx) ; ARGERR(i) ;
			logPNTv(respnt,dnum1 >= dnum2) ; break ;
		      case V4IM_Tag_GT:
			dnum1 = v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(1) ; dnum2 = v4im_GetPointDbl(&ok,cpt,ctx) ; ARGERR(i) ;
			logPNTv(respnt,dnum1 > dnum2) ; break ;
		      case V4IM_Tag_NE:
			dnum1 = v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(1) ; dnum2 = v4im_GetPointDbl(&ok,cpt,ctx) ; ARGERR(i) ;
			logPNTv(respnt,dnum1 != dnum2) ; break ;
		    } ;
		 } ;
		if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,2) ; RE1(0) ; } ;
		break ;
	   case V4IM_OpCode_NIn:		/* NIn( point1, point2 ) */
		i = v4im_DoIn(ctx,argpnts[1],argpnts[2],intmodx,2) ;
		if (i != -999) i = !i ;
		goto relcomp_end ;
	   case V4IM_OpCode_In: 		/* In( point1, point2 ) */
		i = v4im_DoIn(ctx,argpnts[1],argpnts[2],intmodx,2) ;
relcomp_end:	/* Common ending for relational IntMods: i = TRUE or FALSE */
		if (i == -999)
		 { RE1(0) ;
		 } ;
		if (argcnt == 2)		/* 2 argument mode - return TRUE/FALSE */
		 { ipt = (i ? (P *)&Log_True : (P *)&Log_False) ; break ; } ;

/*		Check for old style - no tags, just 3/4 arguments */
		if (argpnts[3]->PntType != V4DPI_PntType_TagVal)
		 { 
static int warning=0; UCCHAR wBuf[512] ;
if ((++warning) < 5) { v_Msg(ctx,wBuf,"@*Using old style arguments for %1M, replace with THEN/ELSE tags - %2N\n",intmodx,isct) ; vout_UCText(VOUT_Warn,0,wBuf) ; } ;
		   if (!i && argcnt <= 3)
		    { ipt = (P *)&Log_False ; break ; } ;	/* 3 argument mode - return FALSE if not true */
		   ipt = argpnts[i ? 3 : 4] ;		/* Have (at least) 4 arguments */
		   if (ipt->PntType == V4DPI_PntType_Isct) 	/* If an ISCT then evaluate it */
		   ipt = v4dpi_IsctEval(&ptbuf,ipt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (ipt == NULL) { v_Msg(ctx,NULL,"ModArgEval",intmodx,(i ? 3 : 4),argpnts[i ? 3 : 4]) ; RE1(0) ; } ;
		   memcpy(respnt,ipt,ipt->Bytes) ; ipt = respnt ;
		   break ;
		 } ;

		ipt = (P *)&protoNone ;
		for(ok=TRUE,ix=3;ix<=argcnt;ix++)
		 { switch (k=v4im_CheckPtArgNew(ctx,argpnts[ix],&tpt,NULL))
		    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; RE1(0) ;
		      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; RE1(0) ;
		      case V4IM_Tag_Else:
		      case V4IM_Tag_Then:
			if (k == V4IM_Tag_Then ? i : !i)
			 { ipt = v4dpi_IsctEval(respnt,tpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
			   if (ipt == NULL) { v_Msg(ctx,NULL,"IfResFail",intmodx,V4IM_Tag_Then) ; RE1(0) ; } ;
			   goto intmod_end ;
			 } ;
			break ;
		      case V4IM_Tag_ElseIf:
			i = v4im_GetPointLog(&ok,tpt,ctx) ;
			if (!ok) { v_Msg(ctx,NULL,"ModInvTagVal",intmodx,V4IM_Tag_ElseIf,i) ; RE1(0) ; } ;
			break ;
		    } ;
		 } ;
		break ;
	   case V4IM_OpCode_EQk:
		ipt = argpnts[1] ;
		switch (ipt->PntType)
		 { 
		   case V4DPI_PntType_Fixed:
			switch (argpnts[2]->PntType)
			 { default:		goto eqk_real ; 	/* If both args not Int/Fixed then fall to double compare below */
			   case V4DPI_PntType_Fixed:
				FIXNORMALIZE(b64,b642,ipt,argpnts[2]) ; i = (b64 == b642) ; goto relcomp_end ;
			   CASEofINT
				memcpy(&b64,&ipt->Value.FixVal,sizeof b64) ;
				b642 = argpnts[2]->Value.IntVal ; SCALEFIX(b642,0,ipt->LHSCnt) ; /* Scale to same as 1st argument */
				i = (b64 == b642) ; goto relcomp_end ;
			 } ;
		   case V4DPI_PntType_Tree:
			i = (ipt->Value.TreeNodeVal = argpnts[2]->Value.TreeNodeVal) ; break ;
		   CASEofINT
			switch (argpnts[2]->PntType)
			 { default:		goto eqk_real ; 	/* If both args not Int/Fixed then fall to double compare below */
			   case V4DPI_PntType_Fixed:
				memcpy(&b642,&argpnts[2]->Value.FixVal,sizeof b64) ;
				b64 = ipt->Value.IntVal ; SCALEFIX(b64,0,argpnts[2]->LHSCnt) ; /* Scale to same as 1st argument */
				i = (b64 == b642) ; goto relcomp_end ;
			   CASEofINT
				i = (ipt->Value.IntVal == argpnts[2]->Value.IntVal) ; goto relcomp_end ;
			 } ;
		   case V4DPI_PntType_SSVal:
			i = (ipt->Value.IntVal == v4im_GetPointInt(&ok,argpnts[2],ctx)) ;  ARGERR(2) ; break ;
		   case V4DPI_PntType_Color:
		   case V4DPI_PntType_Country:
		   case V4DPI_PntType_XDict:
		   case V4DPI_PntType_Dict:
		   CASEofChar
			v4im_GetPointUC(&ok,UCTBUF1,V4DPI_AlphaVal_Max,ipt,ctx) ; ARGERR(1) ;
			v4im_GetPointUC(&ok,UCTBUF2,V4DPI_AlphaVal_Max,argpnts[2],ctx) ; ARGERR(2) ;
			i = (UCstrcmp(UCTBUF1,UCTBUF2) == 0)  ; break ;
		   case V4DPI_PntType_Complex:
			i = memcmp(&argpnts[1]->Value.Complex,&argpnts[2]->Value.Complex,sizeof(struct V4DPI__Value_Complex)) == 0 ;
			break ;
		   case V4DPI_PntType_UOM:
		   case V4DPI_PntType_UOMPer:
		   case V4DPI_PntType_Calendar:
		   case V4DPI_PntType_UTime:
		   case V4DPI_PntType_Real:
eqk_real:		i = (fabs(v4im_GetPointDbl(&ok,ipt,ctx) - v4im_GetPointDbl(&ok,argpnts[2],ctx)) <= gpi->DblEpsilon) ;  ARGERR(1) ; break ;
		   default:
			i = (memcmp(argpnts[2],ipt,ipt->Bytes) == 0) ; break ;
		 } ;
		goto relcomp_end ;
	   case V4IM_OpCode_EQ: 		/* EQ( point1 , point2 , resdim ) */
		ipt = argpnts[1] ;
		i = (ipt->Dim == argpnts[2]->Dim) ;	/* EQ means dims have to match */
		if (!i) goto relcomp_end ;
		switch (ipt->Grouping == 0 && argpnts[2]->Grouping == 0 ? ipt->PntType : -999)
		 { CASEofINT
		   case V4DPI_PntType_SSVal:
		   case V4DPI_PntType_Color:
		   case V4DPI_PntType_Country:
		   case V4DPI_PntType_XDict:
		   case V4DPI_PntType_Dict:
			i = (ipt->Value.IntVal == argpnts[2]->Value.IntVal) ; break ;
		   case V4DPI_PntType_Tree:
			i = (ipt->Value.TreeNodeVal == argpnts[2]->Value.TreeNodeVal) ; break ;
		   case V4DPI_PntType_UOM:
//			GETREAL(dnum1,ipt) ; GETREAL(dnum2,argpnts[2]) ;
//			i = (memcmp(&ipt->Value.UOMVal.Num,&argpnts[2]->Value.UOMVal.Num,sizeof ipt->Value.UOMVal.Num) == 0) ;
//VEH101216 - Use epsilon calculation for UOM equality
			memcpy(&dnum1,&ipt->Value.UOMVal.Num,sizeof dnum1) ; memcpy(&dnum2,&argpnts[2]->Value.UOMVal.Num,sizeof dnum2) ;
			i = (fabs(dnum1 - dnum2) <= gpi->DblEpsilon) ;
			if (i)
			 { if (ipt->Value.UOMVal.Ref != argpnts[2]->Value.UOMVal.Ref) i = FALSE ;
			 } ;
			break ;
		   case V4DPI_PntType_UOMPer:
			i = (memcmp(&ipt->Value.UOMPerVal.Amount,&argpnts[2]->Value.UOMPerVal.Amount,sizeof ipt->Value.UOMPerVal.Amount) == 0) ;
			if (i)
			 { if (ipt->Value.UOMPerVal.Ref != argpnts[2]->Value.UOMPerVal.Ref) i = FALSE ;
			   if (i)
			    { i = (memcmp(&ipt->Value.UOMPerVal.Num,&argpnts[2]->Value.UOMPerVal.Num,sizeof ipt->Value.UOMPerVal.Num) == 0) ;
			    } ;
			 } ;
			break ;
		   case V4DPI_PntType_Fixed:		/* If same dimension then same scaling - just compare */
			i = (memcmp(&ipt->Value.FixVal,&argpnts[2]->Value.FixVal,sizeof(B64INT)) == 0) ; break ;
		   case V4DPI_PntType_Calendar:
		   case V4DPI_PntType_UTime:
		   case V4DPI_PntType_Real:
			GETREAL(dnum1,ipt) ; GETREAL(dnum2,argpnts[2]) ;
			i = (fabs(dnum1 - dnum2) <= gpi->DblEpsilon) ; break ;
		   case V4DPI_PntType_BinObj:
			i = (memcmp(argpnts[2]->Value.AlphaVal,ipt->Value.AlphaVal,ipt->Bytes+V4DPI_PointHdr_Bytes) == 0) ; break ;
		   CASEofChar
			v4im_GetPointUC(&ok,UCTBUF1,V4DPI_AlphaVal_Max,ipt,ctx) ; ARGERR(1)
			v4im_GetPointUC(&ok,UCTBUF2,V4DPI_AlphaVal_Max,argpnts[2],ctx) ; ARGERR(2)
			i = UCstrcmp(UCTBUF1,UCTBUF2) == 0 ; break ;
		   default:
			i = (memcmp(argpnts[2],ipt,ipt->Bytes) == 0) ; break ;
		 } ;
		goto relcomp_end ;
	   case V4IM_OpCode_NE: 		/* NE( point1 , point2 , resdim ) */
		goto nek_entry ;
	   case V4IM_OpCode_NEk:
nek_entry:	ipt = argpnts[1] ;
		switch (ipt->Grouping == V4DPI_Grouping_Single && argpnts[2]->Grouping == V4DPI_Grouping_Single ? ipt->PntType : UNUSED)
		 { CASEofINT
		   case V4DPI_PntType_Fixed:
		   case V4DPI_PntType_UTime:
		   case V4DPI_PntType_Real:
		   case V4DPI_PntType_Calendar:
		   case V4DPI_PntType_UOM:
		   case V4DPI_PntType_UOMPer:
			i = (fabs(v4im_GetPointDbl(&ok,ipt,ctx) - v4im_GetPointDbl(&ok,argpnts[2],ctx)) > gpi->DblEpsilon) ;
			if (!ok) i = TRUE ;		/* If GetPointDbl() failed then points not equal */
			break ;
		   case V4DPI_PntType_Tree:
			i = (ipt->Value.TreeNodeVal != argpnts[2]->Value.TreeNodeVal) ; break ;
		   case V4DPI_PntType_Complex:
			i = memcmp(&argpnts[1]->Value.Complex,&argpnts[2]->Value.Complex,sizeof(struct V4DPI__Value_Complex)) != 0 ;
			break ;
		   case V4DPI_PntType_SSVal:
			i = (ipt->Value.IntVal != argpnts[2]->Value.IntVal) ; break ;
		   case V4DPI_PntType_Color:
		   case V4DPI_PntType_Country:
		   case V4DPI_PntType_XDict:			/* For XDict- have to compare string values */
		   case V4DPI_PntType_Dict:
		   CASEofChar
			v4im_GetPointUC(&ok,UCTBUF1,V4DPI_AlphaVal_Max,ipt,ctx) ; ARGERR(1) ;
			v4im_GetPointUC(&ok,UCTBUF2,V4DPI_AlphaVal_Max,argpnts[2],ctx) ; ARGERR(2) ;
			i = UCstrcmp(UCTBUF1,UCTBUF2) != 0 ; break ;
		   default:
			i = (memcmp(argpnts[2],ipt,ipt->Bytes) != 0) ; break ;
		 } ;
		if (!i && intmodx == V4IM_OpCode_NE) i = (ipt->Dim != argpnts[2]->Dim) ;	/* See EQ/EQk above */
		goto relcomp_end ;
	   case V4IM_OpCode_GT: 		/* GT( point1 , point2 , resdim ) */
		ipt = argpnts[1] ;
		switch (ipt->PntType)
		 { 
		   case V4DPI_PntType_Fixed:
			switch (argpnts[2]->PntType)
			 { default:		goto gt_real ;		/* If both args not Int/Fixed then fall to double compare below */
			   case V4DPI_PntType_Fixed:
				FIXNORMALIZE(b64,b642,ipt,argpnts[2]) ; i = (b64 > b642) ; goto relcomp_end ;
			   CASEofINT
				memcpy(&b64,&ipt->Value.FixVal,sizeof b64) ;
				b642 = argpnts[2]->Value.IntVal ; SCALEFIX(b642,0,ipt->LHSCnt) ; /* Scale to same as 1st argument */
				i = (b64 > b642) ; goto relcomp_end ;
			 } ;
		   CASEofINTmT
			switch (argpnts[2]->PntType)
			 { default:		goto gt_real ;		/* If both args not Int/Fixed then fall to double compare below */
			   case V4DPI_PntType_Fixed:
				memcpy(&b642,&argpnts[2]->Value.FixVal,sizeof b64) ;
				b64 = ipt->Value.IntVal ; SCALEFIX(b64,0,argpnts[2]->LHSCnt) ; /* Scale to same as 1st argument */
				i = (b64 > b642) ; goto relcomp_end ;
			   CASEofINT
				i = (ipt->Value.IntVal > argpnts[2]->Value.IntVal) ; goto relcomp_end ;
			 } ;
		   case V4DPI_PntType_Color:
		   case V4DPI_PntType_Country:
		   case V4DPI_PntType_XDict:
		   case V4DPI_PntType_Dict:
		   CASEofChar
			v4im_GetPointUC(&ok,UCTBUF1,V4DPI_AlphaVal_Max,ipt,ctx) ; ARGERR(1) ;
			v4im_GetPointUC(&ok,UCTBUF2,V4DPI_AlphaVal_Max,argpnts[2],ctx) ; ARGERR(2) ;
			i = (UCstrcmp(UCTBUF1,UCTBUF2) > 0)  ; break ;
		   case V4DPI_PntType_UOM:
		   case V4DPI_PntType_UOMPer:
		   case V4DPI_PntType_Complex:
		   case V4DPI_PntType_UTime:
		   case V4DPI_PntType_Real:
gt_real:		i = (v4im_GetPointDbl(&ok,ipt,ctx) - gpi->DblEpsilon > v4im_GetPointDbl(&ok,argpnts[2],ctx)) ; ARGERR(1) ; break ;
		   case V4DPI_PntType_Calendar:
		   case V4DPI_PntType_UDT:
		   case V4DPI_PntType_UDate:
		   case V4DPI_PntType_UWeek:
		   case V4DPI_PntType_UMonth:
		   case V4DPI_PntType_UPeriod:
		   case V4DPI_PntType_UQuarter:
		   case V4DPI_PntType_UYear:
			i = v4imu_CompareTime(ctx,intmodx,ipt,argpnts[2],&ok) ;
			if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,2) ; RE1(0) ; } ;
			break ;
		   default:
			v_Msg(ctx,NULL,"ModNoDefined2",intmodx,ipt,ipt->PntType) ; RE1(0) ;
		 } ;
		goto relcomp_end ;
	   case V4IM_OpCode_LT: 		/* LT( point1 , point2 , resdim ) */
		ipt = argpnts[1] ;
		switch (ipt->PntType)
		 { 
		   case V4DPI_PntType_Fixed:
			switch (argpnts[2]->PntType)
			 { default:		goto lt_real ;		/* If both args not Int/Fixed then fall to double compare below */
			   case V4DPI_PntType_Fixed:
				FIXNORMALIZE(b64,b642,ipt,argpnts[2]) ; i = (b64 < b642) ; goto relcomp_end ;
			   CASEofINT
				memcpy(&b64,&ipt->Value.FixVal,sizeof b64) ;
				b642 = argpnts[2]->Value.IntVal ; SCALEFIX(b642,0,ipt->LHSCnt) ; /* Scale to same as 1st argument */
				i = (b64 < b642) ; goto relcomp_end ;
			 } ;
		   CASEofINTmT
			switch (argpnts[2]->PntType)
			 { default:		goto lt_real ;		/* If both args not Int/Fixed then fall to double compare below */
			   case V4DPI_PntType_Fixed:
				memcpy(&b642,&argpnts[2]->Value.FixVal,sizeof b64) ;
				b64 = ipt->Value.IntVal ; SCALEFIX(b64,0,argpnts[2]->LHSCnt) ; /* Scale to same as 1st argument */
				i = (b64 < b642) ; goto relcomp_end ;
			   CASEofINT
				i = (ipt->Value.IntVal < argpnts[2]->Value.IntVal) ; goto relcomp_end ;
			 } ;
		   case V4DPI_PntType_UOM:
		   case V4DPI_PntType_UOMPer:
		   case V4DPI_PntType_UTime:
		   case V4DPI_PntType_Real:
		   case V4DPI_PntType_Complex:
lt_real:		i = (v4im_GetPointDbl(&ok,ipt,ctx) + gpi->DblEpsilon < v4im_GetPointDbl(&ok,argpnts[2],ctx)) ; ARGERR(2) ; break ;
		   case V4DPI_PntType_Calendar:
		   case V4DPI_PntType_UDT:
		   case V4DPI_PntType_UDate:
		   case V4DPI_PntType_UWeek:
		   case V4DPI_PntType_UMonth:
		   case V4DPI_PntType_UPeriod:
		   case V4DPI_PntType_UQuarter:
		   case V4DPI_PntType_UYear:
			i = v4imu_CompareTime(ctx,intmodx,ipt,argpnts[2],&ok) ;
			if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,2) ; RE1(0) ; } ;
			break ;
		   case V4DPI_PntType_Color:
		   case V4DPI_PntType_Country:
		   case V4DPI_PntType_XDict:			/* For XDict- have to compare string values */
		   case V4DPI_PntType_Dict:
		   CASEofChar
			v4im_GetPointUC(&ok,UCTBUF1,V4DPI_AlphaVal_Max,ipt,ctx) ; ARGERR(1) ;
			v4im_GetPointUC(&ok,UCTBUF2,V4DPI_AlphaVal_Max,argpnts[2],ctx) ; ARGERR(2) ;
			i = UCstrcmp(UCTBUF1,UCTBUF2) < 0 ; break ;
		   default:
			v_Msg(ctx,NULL,"ModNoDefined2",intmodx,ipt,ipt->PntType) ; RE1(0) ;
		 } ;
		goto relcomp_end ;
	   case V4IM_OpCode_GE: 		/* GE( point1 , point2 , resdim ) */
		ipt = argpnts[1] ;
		switch (ipt->PntType)
		 { 
		   case V4DPI_PntType_Fixed:
			switch (argpnts[2]->PntType)
			 { default:		goto ge_real ;		/* If both args not Int/Fixed then fall to double compare below */
			   case V4DPI_PntType_Fixed:
				FIXNORMALIZE(b64,b642,ipt,argpnts[2]) ; i = (b64 >= b642) ; goto relcomp_end ;
			   CASEofINT
				memcpy(&b64,&ipt->Value.FixVal,sizeof b64) ;
				b642 = argpnts[2]->Value.IntVal ; SCALEFIX(b642,0,ipt->LHSCnt) ; /* Scale to same as 1st argument */
				i = (b64 >= b642) ; goto relcomp_end ;
			 } ;
		   CASEofINTmT
			switch (argpnts[2]->PntType)
			 { default:		goto ge_real ;		/* If both args not Int/Fixed then fall to double compare below */
			   case V4DPI_PntType_Fixed:
				memcpy(&b642,&argpnts[2]->Value.FixVal,sizeof b64) ;
				b64 = ipt->Value.IntVal ; SCALEFIX(b64,0,argpnts[2]->LHSCnt) ; /* Scale to same as 1st argument */
				i = (b64 >= b642) ; goto relcomp_end ;
			   CASEofINT
				i = (ipt->Value.IntVal >= argpnts[2]->Value.IntVal) ; goto relcomp_end ;
			 } ;
		   case V4DPI_PntType_UOM:
		   case V4DPI_PntType_UOMPer:
		   case V4DPI_PntType_UTime:
		   case V4DPI_PntType_Real:
		   case V4DPI_PntType_Complex:
ge_real:		i = (v4im_GetPointDbl(&ok,ipt,ctx) + gpi->DblEpsilon >= v4im_GetPointDbl(&ok,argpnts[2],ctx)) ; ARGERR(2) ; break ;
		   case V4DPI_PntType_Calendar:
		   case V4DPI_PntType_UDT:
		   case V4DPI_PntType_UDate:
		   case V4DPI_PntType_UWeek:
		   case V4DPI_PntType_UMonth:
		   case V4DPI_PntType_UPeriod:
		   case V4DPI_PntType_UQuarter:
		   case V4DPI_PntType_UYear:
			i = v4imu_CompareTime(ctx,intmodx,ipt,argpnts[2],&ok) ;
			if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,2) ; RE1(0) ; } ;
			break ;
		   case V4DPI_PntType_Color:
		   case V4DPI_PntType_Country:
		   case V4DPI_PntType_XDict:			/* For XDict- have to compare string values */
		   case V4DPI_PntType_Dict:
		   CASEofChar
			v4im_GetPointUC(&ok,UCTBUF1,V4DPI_AlphaVal_Max,ipt,ctx) ; ARGERR(1) ;
			v4im_GetPointUC(&ok,UCTBUF2,V4DPI_AlphaVal_Max,argpnts[2],ctx) ; ARGERR(2) ;
			i = UCstrcmp(UCTBUF1,UCTBUF2) >= 0 ; break ;
		   default:
			v_Msg(ctx,NULL,"ModNoDefined2",intmodx,ipt,ipt->PntType) ; RE1(0) ;
		 } ;
		goto relcomp_end ;
	   case V4IM_OpCode_LE: 		/* LE( point1 , point2 , resdim ) */
		ipt = argpnts[1] ;
		switch (ipt->PntType)
		 { 
		   case V4DPI_PntType_Fixed:
			switch (argpnts[2]->PntType)
			 { default:		goto le_real ;		/* If both args not Int/Fixed then fall to double compare below */
			   case V4DPI_PntType_Fixed:
				FIXNORMALIZE(b64,b642,ipt,argpnts[2]) ; i = (b64 <= b642) ; goto relcomp_end ;
			   CASEofINT
				memcpy(&b64,&ipt->Value.FixVal,sizeof b64) ;
				b642 = argpnts[2]->Value.IntVal ; SCALEFIX(b642,0,ipt->LHSCnt) ; /* Scale to same as 1st argument */
				i = (b64 <= b642) ; goto relcomp_end ;
			 } ;
		   CASEofINTmT
			switch (argpnts[2]->PntType)
			 { default:		goto le_real ;		/* If both args not Int/Fixed then fall to double compare below */
			   case V4DPI_PntType_Fixed:
				memcpy(&b642,&argpnts[2]->Value.FixVal,sizeof b64) ;
				b64 = ipt->Value.IntVal ; SCALEFIX(b64,0,argpnts[2]->LHSCnt) ; /* Scale to same as 1st argument */
				i = (b64 <= b642) ; goto relcomp_end ;
			   CASEofINT
				i = (ipt->Value.IntVal <= argpnts[2]->Value.IntVal) ; goto relcomp_end ;
			 } ;
		   case V4DPI_PntType_UOM:
		   case V4DPI_PntType_UOMPer:
		   case V4DPI_PntType_UTime:
		   case V4DPI_PntType_Real:
		   case V4DPI_PntType_Complex:
le_real:		i = (v4im_GetPointDbl(&ok,ipt,ctx) - gpi->DblEpsilon <= v4im_GetPointDbl(&ok,argpnts[2],ctx)) ; ARGERR(2) ; break ;
		   case V4DPI_PntType_Calendar:
		   case V4DPI_PntType_UDT:
		   case V4DPI_PntType_UDate:
		   case V4DPI_PntType_UWeek:
		   case V4DPI_PntType_UMonth:
		   case V4DPI_PntType_UPeriod:
		   case V4DPI_PntType_UQuarter:
		   case V4DPI_PntType_UYear:
			i = v4imu_CompareTime(ctx,intmodx,ipt,argpnts[2],&ok) ;
			if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,2) ; RE1(0) ; } ;
			break ;
		   case V4DPI_PntType_Color:
		   case V4DPI_PntType_Country:
		   case V4DPI_PntType_XDict:			/* For XDict- have to compare string values */
		   case V4DPI_PntType_Dict:
		   CASEofChar
			v4im_GetPointUC(&ok,UCTBUF1,V4DPI_AlphaVal_Max,ipt,ctx) ; ARGERR(1) ;
			v4im_GetPointUC(&ok,UCTBUF2,V4DPI_AlphaVal_Max,argpnts[2],ctx) ; ARGERR(2) ;
			i = UCstrcmp(UCTBUF1,UCTBUF2) <= 0 ; break ;
		   default:
			v_Msg(ctx,NULL,"ModNoDefined2",intmodx,ipt,ipt->PntType) ; RE1(0) ;
		 } ;
		goto relcomp_end ;
	   case V4IM_OpCode_Is1:
		k = (argpnts[1]->Grouping == V4DPI_Grouping_Single) ;
		if (argcnt == 1)		/* 1 argument mode - return TRUE/FALSE */
		 { ipt = (k ? (P *)&Log_True : (P *)&Log_False) ; break ; } ;
		goto then_else ;
	   case V4IM_OpCode_IsAll:
		k = (argpnts[1]->Grouping == V4DPI_Grouping_All) ;
		if (argcnt == 1)		/* 1 argument mode - return TRUE/FALSE */
		 { ipt = (k ? (P *)&Log_True : (P *)&Log_False) ; break ; } ;
		goto then_else ;
	   case V4IM_OpCode_LEG:
		ipt = argpnts[1] ;
		switch (ipt->PntType)
		 { CASEofINTmT
		   case V4DPI_PntType_Fixed:
		   case V4DPI_PntType_UOM:
		   case V4DPI_PntType_UOMPer:
		   case V4DPI_PntType_UTime:
		   case V4DPI_PntType_Real:
		   case V4DPI_PntType_Complex:
			dnum = (v4im_GetPointDbl(&ok,ipt,ctx) - v4im_GetPointDbl(&ok,argpnts[2],ctx)) ; ARGERR(2) ;
			if (fabs(dnum) <= gpi->DblEpsilon) dnum = 0.0 ;
			break ;
		   case V4DPI_PntType_Calendar:
		   case V4DPI_PntType_UDT:
		   case V4DPI_PntType_UDate:
		   case V4DPI_PntType_UWeek:
		   case V4DPI_PntType_UMonth:
		   case V4DPI_PntType_UPeriod:
		   case V4DPI_PntType_UQuarter:
		   case V4DPI_PntType_UYear:
			i = v4imu_CompareTime(ctx,intmodx,ipt,argpnts[2],&ok) ;
			if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,2) ; RE1(0) ; } ;
			break ;
		   default:
			v_Msg(ctx,NULL,"ModNoDefined2",intmodx,ipt,ipt->PntType) ; RE1(0) ;
		 } ;
		if (dnum < 0) { ipt = argpnts[3] ; }
		 else if (dnum == 0) { ipt = argpnts[4] ; }
		 else { ipt = argpnts[5] ; } ;

		cpt = respnt ;
		ipt = v4dpi_IsctEval(cpt,ipt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		break ;
	   case V4IM_OpCode_nSame:
	   case V4IM_OpCode_Same:
		{ double d1, d2, epsilon ; LOGICAL haveTag ;
		  d1 = v4im_GetPointDbl(&ok,argpnts[1],ctx) ; if (!ok) { ix = 1 ; goto intmod_fail_arg ; } ;
		  d2 = v4im_GetPointDbl(&ok,argpnts[2],ctx) ; if (!ok) { ix = 2 ; goto intmod_fail_arg ; } ;
/*		  Do we have delta (arg #3 unless arg 3 is tag */
		  if (argcnt == 2) { epsilon = 0.005 ; haveTag = FALSE ; }
		   else if (argpnts[3]->PntType == V4DPI_PntType_TagVal) { epsilon = 0.005 ; i = 3 ; haveTag = TRUE ; }
		   else { epsilon = v4im_GetPointDbl(&ok,argpnts[3],ctx) ; if (!ok) { ix = 3 ; goto intmod_fail_arg ; } ; i = 4 ; haveTag = argcnt > 3 ; }
		  k = fabs(d1 - d2) < epsilon ;
		  if (intmodx == V4IM_OpCode_nSame) k = !k ;
		  if (!haveTag) { logPNTv(respnt,k) ; ipt = respnt ; break ; } ;
		}
		ipt = (P *)&protoNone ;
		for(;i<=argcnt;i++)
		 { switch (v4im_CheckPtArgNew(ctx,argpnts[i],&tpt,NULL))
		    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; RE1(0) ;
		      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; RE1(0) ;
		      case V4IM_Tag_Then:
			if (k)
			 { ipt = v4dpi_IsctEval(respnt,tpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
			   if (ipt == NULL)
			    { v_Msg(ctx,NULL,"IfResFail",intmodx,V4IM_Tag_Then) ; RE1(0) ; } ;
			   goto intmod_end ;
			 } ;
			break ;
		      case V4IM_Tag_Else:
			if (!k)
			 { ipt = v4dpi_IsctEval(respnt,tpt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
			   if (ipt == NULL)
			    { v_Msg(ctx,NULL,"IfResFail",intmodx,V4IM_Tag_Else) ; RE1(0) ; } ;
			   goto intmod_end ;
			 } ;
			break ;
		      case V4IM_Tag_ElseIf:
			k = v4im_GetPointLog(&j,tpt,ctx) ;
			if (!j) { v_Msg(ctx,NULL,"ModInvTagVal",intmodx,V4IM_Tag_ElseIf,i) ; RE1(0) ; } ;
			break ;
		    } ;
		 } ; goto intmod_end ;
	   case V4IM_OpCode_EvalArithExp:		/* EvalArithExp( "text expression" [result-dim] ) */
		ipt = v4im_DoEvalAE(ctx,respnt,argpnts,argcnt,intmodx,trace,isct) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_EvalLE:			/* EvalLE( "text expression" ) */
		ipt = v4im_DoEvalLE(ctx,respnt,argpnts,argcnt,intmodx,trace,isct) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Secure:
#ifdef V4_BUILD_SECURITY
		ipt = v4im_DoSecure(ctx,respnt,argpnts,argcnt,intmodx,trace,isct) ;
		if (ipt == NULL) goto intmod_end1 ;
#else
		v_Msg(ctx,NULL,"V4NotThisBuild2",intmodx) ; goto intmod_fail ;
#endif
		break ;
	   case V4IM_OpCode_Area:
		ipt = v4im_DoArea(ctx,respnt,argpnts,argcnt,intmodx,trace,isct) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Tally:				/* Tally( list , (value list) (by list) ) */
		ipt = v4im_DoTally(ctx,respnt,argpnts,argcnt,intmodx,trace,isct) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_TallyM:				/* Tally( Area::xxx Bind:xxx ) */
#ifdef DISABLETALLYM
		v_Msg(ctx,NULL,"TallyMNotAvail",intmodx) ; goto intmod_fail ;
#else
		if (gpi->RestrictionMap & V_Restrict_TallyM) { v_Msg(ctx,NULL,"RestrictMod",intmodx) ; goto intmod_fail ; } ;
		ipt = v4im_DoTallyM(ctx,respnt,argpnts,argcnt,intmodx,trace,isct) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
#endif
	   case V4IM_OpCode_Transform:				/* Transform( pt First::xxx Last::xxx ... ) */
		ipt = v4im_DoTransform(ctx,respnt,intmodx,argcnt,argpnts) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Format:					/* string = Format( num options ) */
		ipt = v4im_DoFormat(ctx,respnt,argcnt,argpnts,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_EvalBL:					/* list = EvalBL( @[isct[ ) */
		ipt = v4im_DoEvalBL(ctx,respnt,argcnt,argpnts,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_FinCvtDec:
#define GPDA(res,arg) res = v4im_GetPointDbl(&ok,argpnts[arg],ctx) ; if (!ok) { ix = arg ; goto intmod_fail_arg ; } ;
#define GPIA(res,arg) res = v4im_GetPointInt(&ok,argpnts[arg],ctx) ; if (!ok) { ix = arg ; goto intmod_fail_arg ; } ;
		ipt = respnt ; GPDA(dnum1,1) ; GPDA(dnum2,2) ; dnum = v4fin_CvtDec(dnum1,dnum2) ; dblPNTv(ipt,dnum) ;
		break ;
	   case V4IM_OpCode_FinCvtFrac:
		ipt = respnt ; GPDA(dnum1,1) ; GPDA(dnum2,2) ; dnum = v4fin_CvtFrac(dnum1,dnum2) ; dblPNTv(ipt,dnum) ;
		break ;
	   case V4IM_OpCode_FinIntRate:
		ipt = v4fin_IntRate(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_FinNPV:
		lp = v4im_VerifyList(NULL,ctx,ipt = argpnts[2],intmodx) ;
		if (lp == NULL)  { v_Msg(ctx,NULL,"ModInvArgList",intmodx,2) ; goto intmod_fail ; } ;
		if (argcnt > 2 && argpnts[3]->PntType != V4DPI_PntType_Isct)
		 { v_Msg(ctx,NULL,"ModArgQIsct",intmodx,3) ; RE1(0) ; } ;
		npv = v_InitDblArray(0) ;
		for(npv->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
		 { if (argcnt == 2)
		    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&isctbuf,ctx) ; ARGERR(i) ; continue ;
		    } ;
		   if (!v4ctx_FrameAddDim(ctx,0,&isctbuf,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; RE1(0) ; } ;
		   CLEARCACHE
		   ipt = v4dpi_IsctEval(&ptbuf,argpnts[3],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (ipt == NULL) { v4mm_FreeChunk(npv) ; v_Msg(ctx,NULL,"ModArgEval",intmodx,3,argpnts[3]) ; RE1(0) ; } ;
		   if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		   npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(2) ;
		 } ;
		ipt = respnt ;
		GPDA(dnum1,1) ; dblPNTi(ipt,v4fin_NPV(dnum1,npv->Count,npv->Pay)) ;
		v4mm_FreeChunk(npv) ;
		break ;
	   case V4IM_OpCode_FinPrice:
		ipt = respnt ;
		ipt = respnt ;
		{ double dnum3, dnum4, dnum5; int inum6, inum7 ;
		  GPIA(i,1) ; GPIA(j,2) ; GPDA(dnum3,3) ; GPDA(dnum4,4) ; GPDA(dnum5,5) ; GPIA(inum6,6) ; GPIA(inum7,7) ;
		  dblPNTi(ipt,v4fin_Price(i,j,dnum3,dnum4,dnum5,inum6,inum7)) ;
		}
		break ;
	   case V4IM_OpCode_FinIRR:
		ipt = v4fin_IRR(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_FinTVM:				/* FinTMV( args ... ) */
		ipt = v4fin_TVM(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_FinFVSched: 			/* FinFVSched(pv ratelist) */
		lp = v4im_VerifyList(NULL,ctx,ipt = argpnts[2],intmodx) ;
		if (lp == NULL)
		 { v_Msg(ctx,NULL,"ModInvArgList",intmodx,2) ; goto intmod_fail ; } ;
		if (argcnt > 2 && argpnts[3]->PntType != V4DPI_PntType_Isct)
		 { v_Msg(ctx,NULL,"ModArgQIsct",intmodx,3) ; RE1(0) ; } ;
		npv = v_InitDblArray(0) ;
		for(npv->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
		 { if (argcnt == 2)
		    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&isctbuf,ctx) ; ARGERR(i) ; continue ;
		    } ;
		   if (!v4ctx_FrameAddDim(ctx,0,&isctbuf,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; RE1(0) ; } ;
		   CLEARCACHE
		   ipt = v4dpi_IsctEval(&ptbuf,argpnts[3],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (ipt == NULL)
		    { v4mm_FreeChunk(npv) ; v_Msg(ctx,NULL,"ModArgEval",intmodx,3,argpnts[3]) ; RE1(0) ; } ;
		   npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(2) ;
		 } ;
		ipt = respnt ;
		GPDA(dnum1,1) ; dblPNTi(ipt,v4fin_FVSched(dnum1,npv->Count,npv->Pay)) ;
		v4mm_FreeChunk(npv) ;
		break ;
	   case V4IM_OpCode_FinCoupon:			/* FinCoupon( arg arg ... ) */
		ipt = v4fin_Coupon(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_FinDep:				/* FinDep( arg arg ... ) */
		ipt = v4fin_Dep(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ; 	
	   case V4IM_OpCode_FinSecDisc:
		ipt = v4fin_FinSecDisc(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_FinSecDur:
		ipt = v4fin_FinSecDur(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_FinSecInt:
		ipt = v4fin_FinSecInt(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_FinSecPrice:
		ipt = v4fin_FinSecPrice(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_FinSecRcvd:
		ipt = v4fin_FinSecRcvd(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_FinSecYield:
		ipt = v4fin_FinSecYield(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_FinTBill:
		ipt = v4fin_TBill(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ; 	
	   case V4IM_OpCode_FinDays360: 	
		ipt = respnt ;
		GPIA(i,1) ; GPIA(j,2) ; intPNTv(ipt,v4fin_Days360(i,j)) ;
		break ;
	   case V4IM_OpCode_FinDisc:		
		ipt = respnt ; 
		{ double dnum3,dnum4 ;
		  GPIA(i,1) ; GPIA(j,2) ; GPDA(dnum3,3) ; GPDA(dnum4,4) ; if (argcnt > 4) { GPIA(k,3) ; } else { k = 0 ; } ;
		  dnum = v4fin_Disc(i,j,dnum3,dnum4,k,&ok) ;
		}
		if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		dblPNTv(ipt,dnum) ; break ;
	   case V4IM_OpCode_StatAdjCosSim: 				/* StatAdjCosSim( list isct ) */
		ipt = v4stat_Arg3List(ctx,respnt,v4stat_StatAdjCosSim,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatAvgDev: 				/* StatAvgDev( list isct ) */
		ipt = v4stat_Arg1List(ctx,respnt,v4stat_AvgDev,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatAvg:				/* StatAvg( list isct ) */
		if (argcnt == 1)
		 { dnum = 0 ; lp = v4im_VerifyList(&ptbuf,ctx,argpnts[1],intmodx) ;
		   if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList",intmodx,1) ; ; goto intmod_fail ; } ;
		   for(i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)
		    { dnum += v4im_GetPointDbl(&ok,&isctbuf,ctx) ; ARGERR(i) ; } ;
		   if (i == 1) { v_Msg(ctx,NULL,"ModArgMand",intmodx) ; goto intmod_fail ; } ;
		  dnum = dnum / (double)(i - 1) ;
		  ipt = respnt ; dblPNTv(ipt,dnum) ;
		 } ;
		ipt = v4stat_Arg1List(ctx,respnt,v4stat_Avg,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatChiTest:			/* StatChiTest( constraints list list ) */
		lp = v4im_VerifyList(&ptbuf,ctx,ipt = argpnts[2],intmodx) ;
		if (lp == NULL)
		 { v_Msg(ctx,NULL,"ModInvArgList",intmodx,2) ; goto intmod_fail ; } ;
		npv = v_InitDblArray(0) ;
		for(npv->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
		 { if (TRUE)
		    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&isctbuf,ctx) ; ARGERR(i) ; continue ;
		    } ;
		 } ;
		lp = v4im_VerifyList(&ptbuf,ctx,ipt = argpnts[3],intmodx) ;
		if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList",intmodx,3) ; goto intmod_fail ; } ;
		npv2 = v_InitDblArray(0) ;
		for(npv2->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
		 { if (TRUE)
		    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		      npv2->Pay[npv2->Count++] = v4im_GetPointDbl(&ok,&isctbuf,ctx) ; ARGERR(i) ; continue ;
		    } ;
		 } ;
		ipt = respnt ;
		GPDA(dnum1,1) ; dblPNTi(ipt,v4stat_ChiTest(dnum1,npv->Count,npv->Pay,npv2->Count,npv2->Pay)) ;
		v4mm_FreeChunk(npv) ; v4mm_FreeChunk(npv2) ;
		break ;
	   case V4IM_OpCode_StatConfidence:			/* StatConfidence( alpha stddev size ) */
		ipt = respnt ;
		{ double dnum3 ;
		  GPDA(dnum1,1) ; GPDA(dnum2,2) ; GPDA(dnum3, 3) ;
		  dblPNTi(ipt,v4stat_Confidence(dnum1,dnum2,dnum3,&ok)) ;
		}
		if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		break ;
	   case V4IM_OpCode_StatCorrel: 			/* StatCorrel( list list ) */
		ipt = v4stat_Arg2List(ctx,respnt,v4stat_Correl,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatCovar:				/* StatCovar( list list ) */
		ipt = v4stat_Arg2List(ctx,respnt,v4stat_Covar,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatCritBinom:			/* StatCritBinom( trials probs alpha ) */
		ipt = respnt ;
		{ double dnum3 ;
		  GPDA(dnum1,1) ; GPDA(dnum2,2) ; GPDA(dnum3, 3) ;
		  dblPNTi(ipt,v4stat_CritBinom(dnum1,dnum2,dnum3,&ok)) ;
		}
		if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		break ;
	   case V4IM_OpCode_StatDevSq:				/* StatDevSq( list isct ) */
		lp = v4im_VerifyList(NULL,ctx,ipt = argpnts[1],intmodx) ;
		if (lp == NULL)
		 { v_Msg(ctx,NULL,"ModInvArgList",intmodx,1) ; ; goto intmod_fail ; } ;
		if (argcnt > 1 && argpnts[2]->PntType != V4DPI_PntType_Isct)
		 { v_Msg(ctx,NULL,"ModArgQIsct",intmodx,3) ; RE1(0) ; } ;
		npv = v_InitDblArray(0) ;
		for(npv->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
		 { if (argcnt == 1)
		    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&isctbuf,ctx) ; ARGERR(i) ; continue ;
		    } ;
		   if (!v4ctx_FrameAddDim(ctx,0,&isctbuf,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; RE1(0) ; } ;
		   CLEARCACHE
		   ipt = v4dpi_IsctEval(&ptbuf,argpnts[2],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (ipt == NULL) { v4mm_FreeChunk(npv) ; v_Msg(ctx,NULL,"ModArgEval",intmodx,2,argpnts[2]) ; RE1(0) ; } ;
		   if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		   npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(1) ;
		 } ;
		ipt = respnt ;
		dblPNTi(ipt,v4stat_DevSq(npv->Count,npv->Pay,&ok)) ;
		if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		v4mm_FreeChunk(npv) ;
		break ;
	   case V4IM_OpCode_StatDist:				/* StatDist( tag? arg arg ... ) */
		ipt = v4stat_Dist(ctx,respnt,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatDistInv:			/* StatDistInv( tag? arg arg ... ) */
		ipt = v4stat_DistInv(ctx,respnt,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatErrF:				/* StatErrF( upper ) */
		ipt = respnt ; 
		GPDA(dnum1,1) ; dblPNTi(ipt,v4stat_ErrF(dnum1)) ;
		break ;
	   case V4IM_OpCode_StatErrFC:				/* StatErrFC( upper ) */
		ipt = respnt ; 
		GPDA(dnum1,1) ; dblPNTi(ipt,v4stat_ErrFC(dnum1)) ;
		break ;
	   case V4IM_OpCode_StatFisher: 			/* StatFisher( x ) */
		ipt = respnt ;
		memcpy(ipt,argpnts[1],argpnts[1]->Bytes) ;
		GPDA(dnum1,1) ; dnum = v4stat_Fisher(dnum1) ; dblPNTv(ipt,dnum) ;
		break ;
	   case V4IM_OpCode_StatFisherInv:			/* StatFisherInv( y ) */
		ipt = respnt ;
		memcpy(ipt,argpnts[1],argpnts[1]->Bytes) ;
		GPDA(dnum1,1) ; dnum = v4stat_FisherInv(dnum1) ; dblPNTv(ipt,dnum) ;
		break ;
	   case V4IM_OpCode_StatForecast:			/* StatForecast( x listx listy ) */
		lp = v4im_VerifyList(&ptbuf,ctx,ipt = argpnts[2],intmodx) ;
		if (lp == NULL)
		 { v_Msg(ctx,NULL,"ModInvArgList",intmodx,2) ; goto intmod_fail ; } ;
		npv = v_InitDblArray(0) ;
		for(npv->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
		 { if (TRUE)
		    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&isctbuf,ctx) ; ARGERR(i) ; continue ;
		    } ;
		 } ;
		lp = v4im_VerifyList(&ptbuf,ctx,ipt = argpnts[3],intmodx) ;
		if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList",intmodx,3) ; goto intmod_fail ; } ;
		npv2 = v_InitDblArray(0) ;
		for(npv2->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
		 { if (TRUE)
		    { if (npv2->Count >= npv2->Max) npv2 = v_EnlargeDblArray(npv2,0) ;
		      npv2->Pay[npv2->Count++] = v4im_GetPointDbl(&ok,&isctbuf,ctx) ; ARGERR(i) ; continue ;
		    } ;
		 } ;
		ipt = respnt ;
		GPDA(dnum1,1) ; dblPNTi(ipt,v4stat_Forecast(dnum1,npv->Count,npv->Pay,npv2->Count,npv2->Pay)) ;
		v4mm_FreeChunk(npv) ; v4mm_FreeChunk(npv2) ;
		break ;
	   case V4IM_OpCode_StatFrequency:			/* StatFrequency( pt list ) */
		lp = v4im_VerifyList(NULL,ctx,ipt = argpnts[2],intmodx) ;
		if (lp == NULL)
		 { v_Msg(ctx,NULL,"ModInvArgList",intmodx,2) ; goto intmod_fail ; } ;
		if (argcnt > 2 && argpnts[3]->PntType != V4DPI_PntType_Isct)
		 { v_Msg(ctx,NULL,"ModArgQIsct",intmodx,3) ; RE1(0) ; } ;
		npv = v_InitDblArray(0) ;
		for(npv->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
		 { if (argcnt == 2)
		    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&isctbuf,ctx) ; ARGERR(i) ; continue ;
		    } ;
		   if (!v4ctx_FrameAddDim(ctx,0,&isctbuf,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; RE1(0) ; } ;
		   CLEARCACHE
		   ipt = v4dpi_IsctEval(&ptbuf,argpnts[2],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (ipt == NULL) { v_Msg(ctx,NULL,"ModArgEval",intmodx,2,argpnts[2]) ; RE1(0) ; } ;
		   if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		   npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(i) ;
		 } ;
		ipt = respnt ; 
		GPDA(dnum1,1) ; dblPNTi(ipt,v4stat_Frequency(dnum1,npv->Count,npv->Pay)) ;
		v4mm_FreeChunk(npv) ;
		break ;
	   case V4IM_OpCode_StatFTest:				/* StatFTest( list list ) */
		ipt = v4stat_Arg2List(ctx,respnt,v4stat_FTest,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatGammaLn:			/* StatGammaLn( x ) */
		ipt = respnt ;
		GPDA(dnum1,1) ; dblPNTi(ipt,v4stat_GammaLn(dnum1)) ;
		break ;
	   case V4IM_OpCode_StatGeoMean:			/* StatGeoMean( list isct ) */
		ipt = v4stat_Arg1List(ctx,respnt,v4stat_GeoMean,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatHarMean:			/* StatHarMean( list isct ) */
		ipt = v4stat_Arg1List(ctx,respnt,v4stat_HarMean,intmodx,argpnts,argcnt,trace) ;
		break ;
	   case V4IM_OpCode_StatKurtosis:			/* StatKurtosis( list isct ) */
		ipt = v4stat_Arg1List(ctx,respnt,v4stat_Kurtosis,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatLinFit: 			/* StatLinFit( Y::pt X::pt X::pt... Slope::xx ... ) */
		ipt = v4stat_Fit(ctx,respnt,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatMax:				/* StatMax( list isct ) */
		ipt = v4stat_Arg1List(ctx,respnt,v4stat_Max,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatMedian: 			/* StatMedian( list isct ) */
		ipt = v4stat_Arg1List(ctx,respnt,v4stat_Median,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatMin:				/* StatMin( list isct ) */
		ipt = v4stat_Arg1List(ctx,respnt,v4stat_Min,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatMode:					/* StatMode( list isct ) */
		ipt = v4stat_Arg1List(ctx,respnt,v4stat_Mode,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatPearson:				/* StatPearson( list list ) */
		ipt = v4stat_Arg2List(ctx,respnt,v4stat_Pearson,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatPercentile:				/* Percentile( k list isct ) */
		lp = v4im_VerifyList(NULL,ctx,ipt = argpnts[2],intmodx) ;
		if (lp == NULL)
		 { v_Msg(ctx,NULL,"ModInvArgList",intmodx,2) ; goto intmod_fail ; } ;
		if (argcnt > 2 && argpnts[3]->PntType != V4DPI_PntType_Isct)
		 { v_Msg(ctx,NULL,"ModArgQIsct",intmodx,3) ; RE1(0) ; } ;
		npv = v_InitDblArray(0) ;
		for(npv->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
		 { if (argcnt == 2)
		    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&isctbuf,ctx) ; ARGERR(i) ; continue ;
		    } ;
		   if (!v4ctx_FrameAddDim(ctx,0,&isctbuf,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; RE1(0) ; } ;
		   CLEARCACHE
		   ipt = v4dpi_IsctEval(&ptbuf,argpnts[2],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (ipt == NULL)
		    { v4mm_FreeChunk(npv) ; v_Msg(ctx,NULL,"ModArgEval",intmodx,2,argpnts[2]) ; RE1(0) ; } ;
		   if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		   npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(2) ;
		 } ;
		ipt = respnt ;
		GPDA(dnum1,1) ; dblPNTi(ipt,v4stat_Percentile(dnum1,npv->Count,npv->Pay)) ;
		v4mm_FreeChunk(npv) ;
		break ;
	   case V4IM_OpCode_StatPCRank: 				/* StatPCRank( x list isct ) */
		lp = v4im_VerifyList(NULL,ctx,ipt = argpnts[2],intmodx) ;
		if (lp == NULL)
		 { v_Msg(ctx,NULL,"ModInvArgList",intmodx,2) ; goto intmod_fail ; } ;
		if (argcnt > 2 && argpnts[3]->PntType != V4DPI_PntType_Isct)
		 { v_Msg(ctx,NULL,"ModArgQIsct",intmodx,3) ; RE1(0) ; } ;
		npv = v_InitDblArray(0) ;
		for(npv->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
		 { if (argcnt == 2)
		    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&isctbuf,ctx) ; ARGERR(i) ; continue ;
		    } ;
		   if (!v4ctx_FrameAddDim(ctx,0,&isctbuf,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; RE1(0) ; } ;
		   CLEARCACHE
		   ipt = v4dpi_IsctEval(&ptbuf,argpnts[2],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (ipt == NULL)
		    { v4mm_FreeChunk(npv) ; v_Msg(ctx,NULL,"ModArgEval",intmodx,2,argpnts[2]) ; RE1(0) ; } ;
		   if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		   npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(2) ;
		 } ;
		ipt = respnt ; 
		GPDA(dnum1,1) ; dblPNTi(ipt,v4stat_PercentRank(dnum1,npv->Count,npv->Pay)) ;
		v4mm_FreeChunk(npv) ;
		break ;
	   case V4IM_OpCode_StatPermute:				/* StatPermute( number chosen ) */
		ipt = respnt ; 
		GPDA(dnum1,1) ; GPDA(dnum2,2) ; dblPNTi(ipt,v4stat_Permute(dnum1,dnum2)) ;
		break ;
	   case V4IM_OpCode_StatProb:
		ipt = respnt ; 
		GPDA(dnum1,1) ; GPDA(dnum2,2) ; dblPNTi(ipt,v4stat_Prob(dnum1,dnum2)) ;
		break ;
	   case V4IM_OpCode_StatProduct:
		ipt = v4stat_Arg1List(ctx,respnt,v4stat_Product,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatQuartile:				/* StatQuartile( quartile list isct ) */
		lp = v4im_VerifyList(NULL,ctx,ipt = argpnts[2],intmodx) ;
		if (lp == NULL)
		 { v_Msg(ctx,NULL,"ModInvArgList",intmodx,2) ; goto intmod_fail ; } ;
		if (argcnt > 2 && argpnts[3]->PntType != V4DPI_PntType_Isct)
		 { v_Msg(ctx,NULL,"ModArgQIsct",intmodx,3) ; RE1(0) ; } ;
		npv = v_InitDblArray(0) ;
		for(npv->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
		 { if (argcnt == 2)
		    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&isctbuf,ctx) ; ARGERR(i) ; continue ;
		    } ;
		   if (!v4ctx_FrameAddDim(ctx,0,&isctbuf,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; RE1(0) ; } ;
		   CLEARCACHE
		   ipt = v4dpi_IsctEval(&ptbuf,argpnts[2],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (ipt == NULL) { v4mm_FreeChunk(npv) ; v_Msg(ctx,NULL,"ModArgEval",intmodx,2,argpnts[2]) ; RE1(0) ; } ;
		   if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		   npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(2) ;
		 } ;
		ipt = respnt ; 
		GPIA(i,1) ; dblPNTi(ipt,v4stat_Quartile(i,npv->Count,npv->Pay,&ok)) ;
		v4mm_FreeChunk(npv) ;
		if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		break ;
	   case V4IM_OpCode_StatRank:					/* StatRank( x start list isct ) */
		lp = v4im_VerifyList(NULL,ctx,ipt = argpnts[3],intmodx) ;
		if (lp == NULL) { v_Msg(ctx,NULL,"ModInvArgList",intmodx,3) ; goto intmod_fail ; } ;
		if (argcnt > 3 && argpnts[4]->PntType != V4DPI_PntType_Isct)
		 { v_Msg(ctx,NULL,"ModArgQIsct",intmodx,3) ; RE1(0) ; } ;
		npv = v_InitDblArray(0) ;
		for(npv->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
		 { if (argcnt == 3)
		    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&isctbuf,ctx) ; ARGERR(i) ; continue ;
		    } ;
		   if (!v4ctx_FrameAddDim(ctx,0,&isctbuf,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; RE1(0) ; } ;
		   CLEARCACHE
		   ipt = v4dpi_IsctEval(&ptbuf,argpnts[4],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (ipt == NULL)
		    { v_Msg(ctx,NULL,"ModArgEval",intmodx,4,argpnts[4]) ; RE1(0) ; } ;
		   if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		   npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(2) ;
		 } ;
		ipt = respnt ; 
		GPIA(i,1) ; GPIA(j,2) ; dblPNTi(ipt,v4stat_Rank(npv->Count,npv->Pay,i,j)) ;
		v4mm_FreeChunk(npv) ;
		break ;
	   case V4IM_OpCode_StatRSQ:				/* StatRSQ( list list/isct isct ) */
		ipt = v4stat_Arg2List(ctx,respnt,v4stat_RSQ,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatScale:
		ipt = v4im_DoStatScale(ctx,respnt,argcnt,argpnts,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatSkew:				/* StatSkew( list isct ) */
		ipt = v4stat_Arg1List(ctx,respnt,v4stat_Skew,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatSlope:				/* StatSlope( list1 list2 ) */
		ipt = v4stat_Arg2List(ctx,respnt,v4stat_Slope,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatStandardize:				/* StatStandardize( x mean stddev ) */
		ipt = respnt ;
		{ double dnum3 ;
		  GPDA(dnum1,1) ; GPDA(dnum2,2) ; GPDA(dnum3, 3) ;
		  dblPNTi(ipt,v4stat_Standardize(dnum1,dnum2,dnum3)) ;
		}
		break ;
	   case V4IM_OpCode_StatStdDev: 				/* StdDev( list isct ) */
		ipt = v4stat_Arg1List(ctx,respnt,v4stat_StdDev,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatStdDevP:					/* StdDev( list isct ) */
		ipt = v4stat_Arg1List(ctx,respnt,v4stat_StdDevP,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatSumSq:				/* StatSumSq( list isct ) */
		ipt = v4stat_Arg1List(ctx,respnt,v4stat_SumSq,intmodx,argpnts,argcnt,trace) ;
		break ;
	   case V4IM_OpCode_StatStdErrYX:				/* StatStdErrYX( listy listx ) */
		ipt = v4stat_Arg2List(ctx,respnt,v4stat_StdErrYX,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatSumX2mY2:				/* StatX2mY2( list list ) */
		ipt = v4stat_Arg2List(ctx,respnt,v4stat_SumX2mY2,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatSumX2pY2:				/* StatX2pY2( list list ) */
		ipt = v4stat_Arg2List(ctx,respnt,v4stat_SumX2pY2,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatSumXmY2:				/* StatXmY2( list list ) */
		ipt = v4stat_Arg2List(ctx,respnt,v4stat_SumXmY2,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatTrimMean:			/* StatTrimMean( percent list isct ) */
		lp = v4im_VerifyList(NULL,ctx,ipt = argpnts[2],intmodx) ;
		if (lp == NULL)
		 { v_Msg(ctx,NULL,"ModInvArgList",intmodx,2) ; goto intmod_fail ; } ;
		if (argcnt > 2 && argpnts[3]->PntType != V4DPI_PntType_Isct)
		 { v_Msg(ctx,NULL,"ModArgQIsct",intmodx,3) ; RE1(0) ; } ;
		npv = v_InitDblArray(0) ;
		for(ok=TRUE,npv->Count=0,i=1;ok&&v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
		 { if (argcnt == 2)
		    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&isctbuf,ctx) ; continue ;
		    } ;
		   if (!v4ctx_FrameAddDim(ctx,0,&isctbuf,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; RE1(0) ; } ;
		   CLEARCACHE
		   ipt = v4dpi_IsctEval(&ptbuf,argpnts[2],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (ipt == NULL)
		    { v4mm_FreeChunk(npv) ; v_Msg(ctx,NULL,"ModArgEval",intmodx,2,argpnts[2]) ; RE1(0) ; } ;
		   if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		   npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ;
		 } ;
		if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		ipt = respnt ; 
		GPDA(dnum1,1) ; dblPNTi(ipt,v4stat_TrimMean(npv->Count,npv->Pay,dnum1,&ok)) ;
		v4mm_FreeChunk(npv) ;
		if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		break ;
	   case V4IM_OpCode_StatTTest:				/* StatTTest( list list tails type ) */
		lp = v4im_VerifyList(&ptbuf,ctx,ipt = argpnts[1],intmodx) ;
		if (lp == NULL)
		 { v_Msg(ctx,NULL,"ModInvArgList",intmodx,1) ; ; goto intmod_fail ; } ;
		npv = v_InitDblArray(0) ;
		for(npv->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
		 { if (TRUE)
		    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&isctbuf,ctx) ; ARGERR(i) ; continue ;
		    } ;
		 } ;
		lp = v4im_VerifyList(&ptbuf,ctx,ipt = argpnts[2],intmodx) ;
		if (lp == NULL)
		 { v_Msg(ctx,NULL,"ModInvArgList",intmodx,2) ; goto intmod_fail ; } ;
		npv2 = v_InitDblArray(0) ;
		for(npv2->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
		 { if (TRUE)
		    { if (npv2->Count >= npv2->Max) npv2 = v_EnlargeDblArray(npv2,0) ;
		      npv2->Pay[npv2->Count++] = v4im_GetPointDbl(&ok,&isctbuf,ctx) ; ARGERR(i) ; continue ; } ;
		 } ;
		ipt = respnt ; 
		GPIA(i,3) ; GPIA(j,4) ; dblPNTi(ipt,v4stat_TTest(npv->Count,npv->Pay,npv2->Count,npv2->Pay,i,j)) ;
		v4mm_FreeChunk(npv) ; v4mm_FreeChunk(npv2) ;
		break ;
	   case V4IM_OpCode_StatVar:				/* StatVar( list isct ) */
		ipt = v4stat_Arg1List(ctx,respnt,v4stat_Var,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatVarP:				/* StatVarP( list isct ) */
		ipt = v4stat_Arg1List(ctx,respnt,v4stat_VarP,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_StatZTest:				/* StatZTest( x sigma list isct ) */
		lp = v4im_VerifyList(NULL,ctx,ipt = argpnts[3],intmodx) ;
		if (lp == NULL)
		 { v_Msg(ctx,NULL,"ModInvArgList",intmodx,3) ; goto intmod_fail ; } ;
		if (argcnt > 3 && argpnts[4]->PntType != V4DPI_PntType_Isct)
		 { v_Msg(ctx,NULL,"ModArgQIsct",intmodx,3) ; RE1(0) ; } ;
		npv = v_InitDblArray(0) ;
		for(npv->Count=0,i=1;v4l_ListPoint_Value(ctx,lp,i,&isctbuf) > 0;i++)	/* Loop thru each point in list */
		 { if (argcnt == 3)
		    { if (npv->Count >= npv->Max) npv = v_EnlargeDblArray(npv,0) ;
		      npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,&isctbuf,ctx) ; ARGERR(i) ; continue ;
		    } ;
		   if (!v4ctx_FrameAddDim(ctx,0,&isctbuf,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; RE1(0) ; } ;
		   CLEARCACHE
		   ipt = v4dpi_IsctEval(&ptbuf,argpnts[4],ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (ipt == NULL)
		    { v4mm_FreeChunk(npv) ; v_Msg(ctx,NULL,"ModArgEval",intmodx,4,argpnts[4]) ; RE1(0) ; } ;
		   npv->Pay[npv->Count++] = v4im_GetPointDbl(&ok,ipt,ctx) ; ARGERR(3) ;
		 } ;
		ipt = respnt ; 
		GPDA(dnum1,1) ; GPDA(dnum2,2) ;
		dblPNTi(ipt,v4stat_ZTest(npv->Count,npv->Pay,dnum1,dnum2,&ok)) ;
		if (!ok) { v_Msg(ctx,NULL,"ModFailed2",intmodx) ; RE1(0) ; } ;
		v4mm_FreeChunk(npv) ;
		break ;
	   case V4IM_OpCode_dbConnect:
		if (gpi->xdb == NULL)			/* First call? */
		 {
#ifdef WANTORACLE
		  static int didOCIInit = FALSE ;
		  if (!didOCIInit)
		   { extern void v4oracle_ErrorHandler() ;
		     if (!OCI_Initialize(v4oracle_ErrorHandler, NULL,OCI_ENV_DEFAULT))
		      { v_Msg(ctx,NULL,"ORACLEINIT",intmodx) ; RE1(0) ; } ;
		     didOCIInit = TRUE ;
		   } ;
#endif
		   gpi->xdb = (struct V4XDB__Master *)v4mm_AllocChunk(sizeof *gpi->xdb,TRUE) ;
		   gpi->xdb->xdbLock = UNUSEDSPINLOCKVAL ;
		 } ;
		ZUS(gpi->xdbLastError) ;
		ipt = v4db_DodbConnect(ctx,respnt,intmodx,argcnt,argpnts,trace) ;
		if (ipt == NULL) goto odbc_fail ;
		break ;
	   case V4IM_OpCode_Dbg:
		ipt = v4im_DoDbg(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL)
		 goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_dbGet:
		ZUS(gpi->xdbLastError) ;
		ipt = v4im_DodbGet(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL)
		 goto odbc_fail ;
		break ;
	   case V4IM_OpCode_db:
		ipt = v4im_Dodb(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
//		if (ipt == NULL) goto odbc_fail ;		//VEH110628 - don't want to do this on fail - error message handled in v4im_Dodb()
		break ;
	   case V4IM_OpCode_ODBCXct:
		ZUS(gpi->xdbLastError) ;
		ipt = v4im_DodbXct(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL)
		 goto odbc_fail ;
		break ;
	   case V4IM_OpCode_dbVal:				/* ODBCVal( rpt [dim] column ) */
		ZUS(gpi->xdbLastError) ;
	      if (gpi->xdb == NULL) { v_Msg(ctx,NULL,"XDBNoCon",intmodx) ; RE1(0) ; } ;
	      {	INDEX sx,cx ; ETYPE vdType ; P *xdbPt=NULL ; BYTE *data ; XDBRECID recId = UNUSED ; XDBID xdbId = UNUSED ; struct V4XDB__SaveStmtInfo *vxssi ;
	        if (argpnts[1]->PntType == V4DPI_PntType_XDB) { xdbPt = argpnts[1] ; }
		 else { if (argpnts[1]->Dim == Dim_Dim)		/* If first argument is Dim:xxx then grab xxx* */
			 { DIMVAL(xdbPt,ctx,argpnts[1]->Value.IntVal) ;
			   if (xdbPt == NULL) { v_Msg(ctx,NULL,"XDBDimBad",intmodx,1,argpnts[1]) ; ipt = NULL ; goto odbc_fail ; } ;
			 } else					/* Assume this is the name of a statement */
			 { INDEX sx ;
			   for(sx=0;sx<gpi->xdb->stmtCount;sx++)
			    { if (gpi->xdb->stmts[sx].xdbId != UNUSED && memcmp(&gpi->xdb->stmts[sx].idPnt,argpnts[1],argpnts[1]->Bytes) == 0) break ; } ;
			   if (sx < gpi->xdb->stmtCount)		/* If we matched on Id then take it */
			    { for(cx=0;cx<gpi->xdb->conCount;cx++)	/* Get corresponding connection */
			       { if (gpi->xdb->con[cx].xdbId == gpi->xdb->stmts[sx].hxdbId) break ; } ;
			      recId = gpi->xdb->stmts[sx].curRecId ;
			      xdbId = gpi->xdb->stmts[sx].xdbId ;
			    } ;
			 } ;
		      } ;
		if (xdbPt != NULL) { recId = xdbPt->Value.XDB.recId ; xdbId = xdbPt->Value.XDB.xdbId ; }
		 else if (recId == UNUSED) { recId = argpnts[1]->Value.IntVal ;		/* This better be a recId */
			
		      } ;
		if (argcnt > 2)
		 { if (argpnts[2]->Dim == Dim_Dim)		/* Is 2nd argument a regular point on point on Dim:Dim? */
		    { DIMINFO(di,ctx,argpnts[2]->Value.IntVal) ;
		    } else { DIMINFO(di,ctx,argpnts[2]->Dim) ; }
		   if (di == NULL) { v_Msg(ctx,NULL,"ModArgNotDim",intmodx,2,argpnts[2]) ; ipt = NULL ; goto odbc_fail ; } ;
		   cx = v4im_GetPointInt(&ok,argpnts[3],ctx) ;
		   if (!ok) { v_Msg(ctx,NULL,"ModInvArg",intmodx,3) ; ipt = NULL ; goto odbc_fail ; } ;
		 } else
		 { di = NULL ; cx = v4im_GetPointInt(&ok,argpnts[2],ctx) ;
		   if (!ok) {  v_Msg(ctx,NULL,"ModInvArg",intmodx,2) ; ipt = NULL ; goto odbc_fail ; } ;
		 } ;

/*		Convert column # from 1..n to 0..(n-1) for standard C offsetting */
		cx-- ;
/*		Now have to get address, vdtype, length of data element */
		if (xdbId != UNUSED) { for(sx=0;sx<gpi->xdb->stmtCount;sx++) { if (gpi->xdb->stmts[sx].xdbId == xdbId) break ; } ; }
/*			Don't have xdbId - see if this point matches a current statment */
		 else { for(sx=0;sx<gpi->xdb->stmtCount;sx++)
			 { if (gpi->xdb->stmts[sx].xdbId == UNUSED) continue ;
			   if (gpi->xdb->stmts[sx].curRecId != recId) continue ;
			   if (gpi->xdb->stmts[sx].targetDimId == argpnts[1]->Dim) break ;
			 } ;
		      } ;
		if (sx < gpi->xdb->stmtCount ? recId == gpi->xdb->stmts[sx].curRecId : FALSE)
/*		    Want current row, sx = statement */
		 { vdType = gpi->xdb->stmts[sx].col[cx].vdType ;
		   if (cx >= gpi->xdb->stmts[sx].colCount)
		    { v_Msg(ctx,NULL,"XDBMaxCol",intmodx,cx+1,gpi->xdb->stmts[sx].colCount) ; ipt = NULL ; goto odbc_fail ; } ;
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
			if (OCI_IsNull(gpi->xdb->stmts[sx].rs,cx+1))	/* Oracle is not zero based */
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
		 } else
/*		   Want row that is not 'current' row - pull from 'history' */
		 { unsigned short *colOff ;
		   struct V4XDB_SavedRow *vxsr ;
//xxx problem here - don't do this if valCol != UNUSED
/*		   Have to find vxssi - if we have xdbId then it's easy, otherwise try record number range, if that fails then take first one (most recent) that has identical dimId */
		   for(vxssi=gpi->xdb->vxssi;vxssi!=NULL;vxssi=vxssi->vxssiNext)
		    { if (vxssi->xdbId == xdbId) break ;
		      if (vxssi->valCol == UNUSED && argpnts[1]->Dim == vxssi->dimId && recId >= vxssi->firstRecId && recId <= vxssi->lastRecId) break ;
		    } ;
		   if (vxssi == NULL)
		    { for(vxssi=gpi->xdb->vxssi;vxssi!=NULL;vxssi=vxssi->vxssiNext)
		       { if (vxssi->dimId == argpnts[1]->Dim) break ; } ;
		      if (vxssi == NULL)
		       { v_Msg(ctx,NULL,"XDBNoSave",intmodx,recId) ; ipt = NULL ; goto odbc_fail ; } ;
		    } ;
		   if (cx > vxssi->colCount)
		    { v_Msg(ctx,NULL,"XDBMaxCol",intmodx,cx,vxssi->colCount) ; ipt = NULL ; goto odbc_fail ; } ;
		   vdType = vxssi->col[cx].vdType ;
		   vxsr = v4xdb_GetXDBRow(ctx,gpi->xdb,vxssi,recId) ;
		   if (vxsr == NULL)
		    { v_Msg(ctx,NULL,"XDBNoSave",intmodx,recId) ; ipt = NULL ; goto odbc_fail ; } ;
		   colOff = (unsigned short *)&vxsr->data ;
		   data = &vxsr->data[colOff[cx]] ;
		   bytes = (colOff[cx] == 0 ? -1 : colOff[cx+1] - colOff[cx]) ;
/*		   Figure out the length of this column (need special checks for null values) */
		   if (colOff[cx] == 0) { bytes = -1 ; }
		    else if (colOff[cx+1] != 0) { bytes = colOff[cx+1] - colOff[cx] ; }
		    else { INDEX ox = cx + 2 ;
			   for(;;ox++) { if (colOff[ox] != 0) break ; } ;
			   bytes = colOff[ox] - colOff[cx] ;
			 } ;
		 } ;
		if (di == NULL) { DIMINFO(di,ctx,v4xdb_VDTYPEtoDim(vdType)) ; } ;
		if (bytes < 0)				/* Null value - do we have default (second argument not a dimension) */
		 { if (di != NULL ? argpnts[2]->Dim != Dim_Dim : FALSE) { memcpy(respnt,argpnts[2],argpnts[2]->Bytes) ; ipt = respnt ; break ; } ;
		   v_Msg(ctx,NULL,"XDBNullVal",intmodx,cx+1,di->DimId) ; ipt = NULL ; goto odbc_fail ;
		 } ;
/*		Now convert xdb value to corresponding V4 point */
 		ipt = v4dpi_AcceptValue(ctx,respnt,di,vdType,data,bytes) ;
		if (ipt == NULL)
		 { v_Msg(ctx,NULL,"XDBCnvValue",intmodx,cx+1,di->DimId,di->PointType) ;
		   goto odbc_fail ;
		 } ;
		break ;

	       }
                if (ipt == NULL)
                 goto odbc_fail ;
		break ;
	   case V4IM_OpCode_dbFree:
	    { XDBID xdbId = 0 ; LOGICAL wantAll = FALSE ; INDEX sx ;
		if (gpi->xdb == NULL) { v_Msg(ctx,NULL,"XDBNoCon",intmodx) ; RE1(0) ; } ;
		for(i=1;i<=argcnt;i++)
		 { if (argpnts[i]->PntType == V4DPI_PntType_XDB)
		    { xdbId = argpnts[i]->Value.XDB.xdbId ; continue ; } ;
		   switch (v4im_CheckPtArgNew(ctx,argpnts[i],&ipt,&ptbuf))
		    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; RE1(0) ;
		      case V4IM_Tag_Unk:	v_Msg(ctx,NULL,"TagUnknown",intmodx) ; RE1(0) ;
		      case -V4IM_Tag_All:	wantAll = TRUE ; break ;
		      case V4IM_Tag_Dim:	for(pix=0;pix<gpi->xdb->conCount;pix++)
						 { if (gpi->xdb->con[pix].DimId == ipt->Value.IntVal && gpi->xdb->con[pix].xdbId != UNUSED) break ; } ;
						if (pix >= gpi->xdb->conCount)
						 { DIMINFO(di,ctx,ipt->Value.IntVal) ;		/* Can't find - if this is ODBC dim then already free! */
						   if (di->PointType == V4DPI_PntType_XDB) goto odbcfree_end ;
						   v_Msg(ctx,NULL,"XDBDimBad",intmodx,i,ipt) ; RE1(0) ;
						 } ;
						xdbId = gpi->xdb->con[pix].xdbId ;
						break ;
		      case V4IM_Tag_Connection:	for(pix=0;pix<gpi->xdb->conCount;pix++)
						 { if (gpi->xdb->con[pix].xdbId != UNUSED && memcmp(&gpi->xdb->con[pix].idPnt,ipt,ipt->Bytes) == 0) break ; } ;
						if (pix >= gpi->xdb->conCount) { v_Msg(ctx,NULL,"XDBDimBad",intmodx,i,ipt) ; RE1(0) ; } ;
						xdbId = gpi->xdb->con[pix].xdbId ;
						break ;
		      case V4IM_Tag_Statement:	for(sx=0;sx<gpi->xdb->stmtCount;sx++)
						 { if (gpi->xdb->stmts[sx].xdbId != UNUSED && memcmp(&gpi->xdb->stmts[sx].idPnt,ipt,ipt->Bytes) == 0) break ; } ;
						if (sx >= gpi->xdb->stmtCount) { v_Msg(ctx,NULL,"XDBDimBad",intmodx,i,cpt) ; RE1(0) ; } ;
						xdbId = gpi->xdb->stmts[sx].xdbId ;
			break ;
		    } ;
		 } ;
		if (wantAll)
		 { INDEX cx ;
		   for(cx=0;cx<=gpi->xdb->conCount;cx++) { if (gpi->xdb->con[cx].xdbId != UNUSED) v4xdb_FreeStuff(ctx,gpi->xdb->con[cx].xdbId,0) ; } ;
		 } else { v4xdb_FreeStuff(ctx,xdbId,0) ; } ;
odbcfree_end:	ipt = (P *)&Log_True ;
	    }
		break ;
	   case V4IM_OpCode_dbInfo:		/* ODBCInfo( spt col ) */
#if defined WANTODBC || defined WANTMYSQL || defined WANTORACLE
		ZUS(gpi->xdbLastError) ;
		ipt = v4db_Info(ctx,respnt,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto odbc_fail ;
		break ;
#else
		v_Msg(ctx,NULL,"NYIonthisOS",intmodx) ; RE1(0) ;
#endif
	   case V4IM_OpCode_ODBCError:
		ipt = v4db_Error(ctx,respnt,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto odbc_fail ;
		break ;
odbc_fail:
		if (gpi->xdb != NULL && UCempty(gpi->xdbLastError))
		 { 
//		   i = UCstrlen(ctx->ErrorMsg) ;
//		   if (i >= V4DPI_AlphaVal_Max) { i = V4DPI_AlphaVal_Max - 1 ; ctx->ErrorMsg[i] = UCEOS ; } ;
		   ctx->ErrorMsg[UCsizeof(gpi->xdbLastError)-1] = UCEOS ; UCstrcpy(gpi->xdbLastError,ctx->ErrorMsg) ;
		 } ;
		goto intmod_fail ;
	   case V4IM_OpCode_RCVi:
		ipt = v4im_DoRCV(ctx,respnt,intmodx,argpnts,argcnt,Dim_Int) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_RCV:			/* RCV( arguments ) */
		ipt = v4im_DoRCV(ctx,respnt,intmodx,argpnts,argcnt,UNUSED) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_V4ISCon:
		ipt = v4im_V4ISCon(ctx,respnt,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_V4ISVal:
		ipt = v4im_V4ISVal(ctx,respnt,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_V4ISOp:
		ipt = v4im_V4ISOp(ctx,respnt,intmodx,argpnts,argcnt,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
#ifdef WANTWHLINK
	   case V4IM_OpCode_WHLink:
		resdim=argpnts[1]->Value.IntVal ;
		ISVALIDDIM(resdim,1,"WHLink()") ;
		DIMINFO(di,ctx,resdim) ;
		if (di == NULL) { v_Msg(ctx,NULL,"ModArgNotDim",intmodx,1,argpnts[1]) ; RE1(0) ; } ;
		if (di->PointType != V4DPI_PntType_WormHole)
		 { v_Msg(ctx,NULL,"@%1E (arg #1) (Dim:%2D) is not type WormHole",intmodx,di->DimId) ; RE1(0) ; } ;
		j = UNUSED ; vpt = NULL ; dpt = NULL ; spt = NULL ; tpt = NULL ; k = 0 ;
		for(num=0,i=2;i<=argcnt;i++) { if (argpnts[i]->PntType != V4DPI_PntType_TagVal) num++ ; } ;
		for(i=2;i<=argcnt;i++)			/* Step thru the remaining arguments */
		 { ipt = argpnts[i] ;
		   if (ipt->PntType != V4DPI_PntType_TagVal)	/* If not tag then either src or dest */
		    { if (num == 1) { vpt = ipt ; }		/* If only 1 (non-tagged) argument after first then assume default */
		       else { if (spt == NULL) { spt = ipt ; } else { dpt = ipt ; } ; } ;
		      continue ;
		    } ;
		   switch (v4im_CheckPtArgNew(ctx,argpnts[i],&cpt,NULL))
		    { default:			v_Msg(ctx,NULL,"TagBadUse",intmodx) ; RE1(0) ;
		      case V4IM_Tag_Unk:		v_Msg(ctx,NULL,"TagUnknown",intmodx) ; RE1(0) ;
		      case V4IM_Tag_Values:	j = v4im_GetPointInt(&ok,cpt,ctx) ; ARGERR(i) ; break ;
		      case V4IM_Tag_Error:	vpt = cpt ; break ;
		      case V4IM_Tag_Point:	tpt = cpt ; break ;
		      case -V4IM_Tag_Hold:	k = V4C_FrameId_Real ; break ;
		    } ;
		 } ;
		if (spt != NULL && dpt == NULL)
		 { v_Msg(ctx,NULL,"@%1E missing destination point(s)",intmodx) ; RE1(0) ; } ;
		DIMVAL(ipt,ctx,resdim) ;
/*		** NOTE ** - Initxxx() modules will call different handlers if necessary! ** */
		if (ipt == NULL)	/* Is dimension on context? */
		 { ZPH(respnt) ; ipt = respnt ; 				/* No - then make new WH point & add to context */
		   respnt->Dim = resdim ; respnt->PntType = V4DPI_PntType_WormHole ;
		   respnt->Bytes = V4DPI_PointHdr_Bytes + sizeof(respnt->Value.MemPtr) ;
		   respnt->Value.IntVal = v4wh_InitWH_IntInt(ctx,UNUSED,NULL,j,vpt,tpt) ;
		   if (!v4ctx_FrameAddDim(ctx,k,respnt,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",intmodx) ; RE1(0) ; } ;
		 } else
		 { if (dpt == NULL) { memcpy(respnt,ipt,ipt->Bytes) ; }
		    else { memcpy(respnt,dpt,dpt->Bytes) ; } ;
		 } ;
		if (spt != NULL)
		 { whii = (struct V4DPI__WormHoleIntInt *)han_GetPointer(ipt->Value.IntVal,0) ;
		   switch (whii->WHType)
		    { case V4DPI_WHType_IntInt:
			v4wh_PutWH_IntInt(ctx,whii,spt->Value.IntVal,dpt->Value.IntVal) ; break ;
		      case V4DPI_WHType_IntDbl:
			memcpy(&dnum,&dpt->Value.RealVal,sizeof dnum) ;
			v4wh_PutWH_IntDbl(ctx,(struct V4DPI__WormHoleIntDbl *)whii,spt->Value.IntVal,dnum) ; break ;
		      case V4DPI_WHType_IntAgg:
			v4wh_PutWH_IntAgg(ctx,(struct V4DPI__WormHoleIntAgg *)whii,spt->Value.IntVal,dpt->Value.IntVal,dpt->Grouping) ; break ;
		    } ;
		 } ;
		ipt = respnt ;
		break ;
#endif
	   case V4IM_OpCode_Macro:
		ipt = v4im_DoMacro(ctx,respnt,argpnts,argcnt,intmodx,trace,isct) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Table:
		ipt = v4im_DoTable(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_V4:
		ipt = v4im_DoV4(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Trig:
		ipt = v4im_DoTrig(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Log:
		ipt = v4im_DoLog(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Array:
		ipt = v4im_DoArray(ctx,respnt,argpnts,argcnt,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Timer:
		ipt = v4im_DoTimer(ctx,respnt,argpnts,argcnt,intmodx,trace) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Tree:
		ipt = v4im_DoTree(ctx,respnt,argpnts,argcnt,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
	   case V4IM_OpCode_Zip:
		ipt = v4im_DoZip(ctx,respnt,argpnts,argcnt,intmodx) ;
		if (ipt == NULL) goto intmod_end1 ;
		break ;
/*End of BIG Switch */
	 } ;
#ifdef V4_BUILD_RUNTIME_STATS
	gpi->V4TicksInMod[intmodx] += (clock() - initalClock) ;
#endif
intmod_end:			/* HERE ONLY IF SUCCESS, result in ipt */
	if (isct->TraceEval || ((trace & V4TRACE_LogIscts) != 0 && ctx->IsctEvalCount > gpi->StartTraceOutput))
	 { if (v_Msg(ctx,UCTBUF1,"TraceMod",curcall,intmodx,ipt)) vout_UCText(VOUT_Trace,0,UCTBUF1) ;
	 } ;
	goto intmod_end2 ;
/*	If here & ipt = NULL then failure but have already done REGISTER_ERROR */
intmod_end1:			/* HERE ONLY ON FAILURE, ipt = NULL */
	if (ipt == NULL && isct->Continued && ((evalmode & V4DPI_EM_NoContinue) == 0))
	 { ipt = v4dpi_IsctContEval(V4DPI_ContFrom_IntModFail,intmodx,respnt,isct,ctx,V4DPI_EM_Normal,NULL,NULL) ;
	   if (ipt != NULL) goto intmod_end2 ;
	   if (!ctx->disableFailures)
	    { 
	      v_Msg(ctx,NULL,"IsctEvalModFail",isct,intmodx) ;	/* Plug in proper error message (VEH050511) */
	    } ;
	   if (isct->TraceEval || ((trace & V4TRACE_LogIscts) != 0 && ctx->IsctEvalCount > gpi->StartTraceOutput))
	    { if (v_Msg(ctx,UCTBUF1,"TraceModF",curcall,intmodx)) vout_UCText(VOUT_Trace,0,UCTBUF1) ;
	    } ;
	 } ;
intmod_end2:
	if (ctx->HaveScopedPts[ctx->rtStackX])	/* Did we allocate a scope point */
	 { ctx->HaveScopedPts[ctx->rtStackX] = FALSE ;
	   v4dpi_ScopePnt_Free(ctx->rtStackX) ;
	 } ;
//	--ctx->NestedIntMod ;				/* Decrement counter */
	if (ipt == NULL) RETURNFAILURE ;
	if (ipt != respnt)				/* Make sure respnt updated with result VEH060810 */
	 memcpy(respnt,ipt,ipt->Bytes) ;
	return(ipt) ;					/* Return pointer to new point */
/*	Get here on IntMod failure - Need to do a REGISTER_ERROR */
intmod_fail:
	if (isct->TraceEval || ((trace & V4TRACE_LogIscts) != 0 && ctx->IsctEvalCount > gpi->StartTraceOutput))
	 { if (v_Msg(ctx,UCTBUF1,"TraceModF",curcall,intmodx)) vout_UCText(VOUT_Trace,0,UCTBUF1) ;
	 } ;
//VEH060806 - If intmod failed but we have continued isct then try it now
intmod_fail1:
	if (isct->Continued && ((evalmode & V4DPI_EM_NoContinue) == 0))
	 { ipt = v4dpi_IsctContEval(V4DPI_ContFrom_IntModFail,intmodx,respnt,isct,ctx,V4DPI_EM_Normal,NULL,NULL) ;
	   if (ipt != NULL) goto intmod_end1 ;
	   if (!ctx->disableFailures)
	    { 
	      v_Msg(ctx,NULL,"IsctEvalModFail",isct,intmodx) ;	/* Plug in proper error message (VEH050511) */
	    } ;
	 } ;
//END060806
/*	Temporarily up NestedIsctEval and save IsctPtr so we get decent error trace (VEH050511) */
//	ctx->NestedIsctEval++ ;
//	ctx->IsctStack[ctx->NestedIsctEval-1].IsctPtr = isct ;
//if (isct->Bytes > 2 * (sizeof *isct))
// { ctx->IsctStack[ctx->NestedIsctEval-1].IsctPtr = NULL ; } ;

//	ctx->IsctStack[ctx->NestedIsctEval-1].FailCode = UNUSED ;
	REGISTER_ERROR(0) ;
//	ctx->NestedIsctEval-- ;
	ipt = NULL ; goto intmod_end2 ;

intmod_fail_arg:			/* HERE WHEN ARGUMENT FAILS WITHIN INMOD CODE */
	v_Msg(ctx,NULL,"ModInvArg",intmodx,ix) ; goto intmod_fail ;

intmod_iarg_fail:			/* HERE WHEN INITIAL INMOD ARGUMENT FAILS */
	if (isct->TraceEval || ((trace & V4TRACE_LogIscts) != 0 && ctx->IsctEvalCount > gpi->StartTraceOutput))
	 { if (v_Msg(ctx,UCTBUF1,"TraceModFailArg",curcall,intmodx,ix)) vout_UCText(VOUT_Trace,0,UCTBUF1) ;
	 } ;
	goto intmod_fail1 ;
}

