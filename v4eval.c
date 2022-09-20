/*	V4EVAL.C - Evaluator for V4

	Created 28-Apr-92 by Victor E. Hansen			*/

#include <time.h>
#include <setjmp.h>

#ifndef NULLID
//#include "v3defs.c"
#include "v4imdefs.c"
#define V3E_EOF 100580			/* End of file reached */
#endif

#ifdef INCSYS
#include <sys/stat.h>
#else
#include <sys/stat.h>
#endif

#ifdef HPUX
#include <sys/fcntl.h>
#endif

#ifdef ALPHAOSF
#include <sys/mode.h>
#endif

extern struct V4C__ProcessInfo *gpi ;	/* Global process information */
GLOBALDIMSEXTERN
extern struct V__UnicodeInfo *uci ;	/* Global structure of Unicode Info */
V4DEBUG_SETUP

#ifdef WINNT
LPTHREAD_START_ROUTINE v4eval_EscThread() ;
#endif
//extern struct V4MM__MemMgmtMaster *V4_GLOBAL_MMM_PTR ;		/* Point to master area structure */
#define LOCKROOT lrx = v4mm_LockSomething(aid,V4MM_LockId_Root,V4MM_LockMode_Write,V4MM_LockIdType_Root,5,NULL,6)
#define RELROOT v4mm_LockRelease(aid,lrx,mmm->MyPid,aid,"RELROOTDPI") ; v4mm_TempLock(aid,0,FALSE)

#define NOEOL 0			/* Do not allow End-of-Line as token */
#define EOLOK 1			/* EOL an acceptable token */


//static struct V4Eval__MacroCache *mc = NULL ;
ETYPE traceGlobal = 0 ;

/*	v4eval_Eval - Evaluates a text string		*/
/*	Call: res = v4eval_Eval( tcb , ctx , multeval , inittrace , traperrs , nested, withinpnt , respnt )
	  where res = see V4EVAL_Res_xxx,
		tcb is pointer to token control block (V4LEX__TknCtrlBLk)
		ctx is the current context,
		multeval is TRUE for multi-line evaluations, FALSE for single command,
		inittrace is initial trace setting,
		traperrs is TRUE to trap all V4 errors via setjmp (FALSE if somebody else is trapping!),
		nested is TRUE if nested call, FALSE if from top-level,
		withinpnt is TRUE if called from within point specification (ex: ^v"xxxxx"),
		respnt (if not NULL) may be updated with result point			*/


int v4eval_Eval(tcb,ctx,multeval,inittrace,traperrs,nested,withinpnt,respnt)
  struct V4LEX__TknCtrlBlk *tcb ;
  struct V4C__Context *ctx ;
  LOGICAL multeval,traperrs,nested,withinpnt ;
  int inittrace ;
  struct V4DPI__Point *respnt ;
{
#define READMAX 1024
  FILE *incfp ;
  jmp_buf traceback ;
  struct V4IS__ParControlBlk pcb ;
  struct V4DPI__RecogBlock *rcgb ;
  struct V4IS__ParControlBlk *dpcb ;
  struct V4IS__AreaCB *acb ;
  struct V4DPI__DimInfo di,*diminfo,*dix ;
  struct V4DPI__Point point,*dpt,*isct,*filter,*logisct = NULL ;
  struct V4DPI__Point valpt,isctpt,pisct,misct,xisct ;
  struct V4DPI__LittlePoint idpt ;
  struct V4DPI__Binding bindpt ;
  struct V4L__ListPoint *lp ;
  struct V4C__AreaHInfo ahi ; int aid ;
  struct V4V4IS__RuntimeList *rtl ;
  struct V4V4IS__StructEl *stel ;
  struct V4LEX__BigText *bt,*stbt ;
  struct V4LEX__Table *tvlt,*vlt = NULL ;
  struct V4LEX__TablePrsOpt *tpo,*dtpo ;
  struct V4LEX__Table_Tran *vtt ;
  struct V4DPI__UOMTable *uomt ;
  struct V4Eval__Loop *loop ;
  struct UC__File UCFile ;
  enum DictionaryEntries cx,kwx,kwx2 ;
#define LOOPALLOCMAX 10
  struct V4Eval__Loop *loopalloc[LOOPALLOCMAX] ;	/* Keep list of allocated loops - reuse */
  int loopalloccnt=0 ;
  int trace ;
  UCCHAR cname[50],mname[50],newmacro[50] ;
  static UCCHAR macrocall[255] ;		/* Want to be able to survive multiple calls! */
  static int MaxMacros = FALSE ;
  INDEX saveStackX ;
  UCCHAR macrobind[255],textbind[255] ;
  UCCHAR *ucb,*stptr ;
//  UCCHAR ucbuf[V4LEX_Tkn_ArgLineMax] ;
  char result[2500] ; UCCHAR filename[V_FileName_Max] ; UCCHAR onerror[V_FileName_Max] ; UCCHAR argbuf[V4LEX_Tkn_ArgLineMax-10] ;
  int dim,i,j,l,ok,num,areaidused,looper,cnt,gotfile,initial_ilx,framecnt,notflag,dfltext,more,filetype,assok,areaopen,retry ;
//  static int KernelCheck=FALSE ;
  UCCHAR b ;
  int voutRedirected,voutUCF ;

	trace = inittrace ; tpo = NULL ; INITTPO ; dtpo = tpo ; tpo = NULL ;
	ZUS(macrobind) ; bt = NULL ;	/* Nothing in macro-binding command buffer */
	ZUS(macrocall) ; ZUS(cname) ; ZUS(mname) ;
	ZUS(textbind) ;		/* ditto */
	if (traperrs)
	 { if (setjmp(gpi->environment) != 0)
	    {
	      if (voutRedirected) { vout_CloseFile(UNUSED,VOUT_Data,NULL) ; } ;
	      if (gpi->ErrCount > gpi->MaxAllowedErrs)
	       { vout_UCText(VOUT_Err,0,UClit("*Exceeded max allowed errors, Quitting\n")) ; exit(EXITABORT) ; } ;
	      for(;ctx->FrameCnt > framecnt && ctx->FrameCnt > 1;) { v4ctx_FramePop(ctx,0,NULL) ; } ;
	      if (!multeval) return(V4EVAL_Res_NestRetOK) ;
	      if (gpi->ErrNum == V3E_EOF)			/* If ^Z or (^D in UNIX) then quit */
	       { v4ctx_AreaClose(ctx,-1) ; return(V4EVAL_Res_ExitStats) ; } ;
	      dtpo->readLineOpts = V4LEX_ReadLine_FlushCmd ;
	      if (!v4lex_ReadNextLine(tcb,dtpo,NULL,NULL))
	       { if (tcb->type == V4LEX_TknType_Error) { UCstrcpy(ctx->ErrorMsgAux,tcb->UCstring) ; goto fail ; } ; } ;
	      if (vlt != NULL) vlt = NULL ;
	      ctx->rtStackX = saveStackX ;
	    } ;
	 } ;
	saveStackX = ctx->rtStackX ;
	voutRedirected = FALSE ; voutUCF = FALSE ;
	if (gpi->ErrCount > gpi->MaxAllowedErrs)
	 { vout_UCText(VOUT_Err,0,UClit("*Exceeded max allowed errors, Quitting\n")) ; exit(EXITABORT) ; } ;
	framecnt = ctx->FrameCnt ;
	gpi->ctx = ctx ;		/* Link current context to ctx in owner process */
	initial_ilx = tcb->ilx ;	/* Save initial ilx for possible repeats of nest */
	for(cnt=(multeval ? V4LIM_BiggestPositiveInt : 1);(tcb->ilx > initial_ilx) || (cnt>0 || (tcb->type != V4LEX_TknType_EOL));cnt--)
	 { 
//	   if (!nested)
//	    { ctx->NestedIsctEval = 0 ;
//	      ctx->NestedIntMod = 0 ;					/* Reset nesting maxes */
//	    } ;
//	   ctx->FailLevel = UNUSED ;					/* Reset any prior failures */
	   ctx->rtStackFail = UNUSED ;
	   ctx->rtStack[ctx->rtStackX].isctPtr = NULL ;
//	   cx=v4eval_NextKeyWord(comlist,tcb,NOEOL) ;			/* Get command */
	   cx = v4im_GetTCBtoEnumVal(ctx,tcb,NOEOL) ;
	   if (cx == -2)
	    { if (tcb->type == V4LEX_TknType_Error) { UCstrcpy(ctx->ErrorMsgAux,tcb->UCstring) ; }
	       else { v_Msg(ctx,ctx->ErrorMsgAux,"CmdTooLong",tcb->type) ; } ;
	      goto fail ;
	    } ;
	   gpi->LastV4CommandIndex = cx ;
	   if (cx == _End)						/* Got END - then check some stuff */
	    { if (vlt != NULL) { cx = _EndTable ; }			/* Transform into EndTable */
	       else if (tcb->appendto != NULL) { cx = _EndLog ; }		/* Transform into EndLog */
	       else if (tcb->ifx > 0) { cx = _EndIf ; }
	       else { v_Msg(ctx,ctx->ErrorMsgAux,"CmdEndWhat",tcb->type) ; goto fail ; } ;
	    } ;
	   if (tcb->ifx > 0)						/* In an if-then-else situation ? */
	    { i = (tcb->ifs[tcb->ifx-1].inif ? tcb->ifs[tcb->ifx-1].doif : tcb->ifs[tcb->ifx-1].doelse) ;
	      if (i) goto xct_command ;					/* Execute command otherwise drop down and check */
	      if (*tcb->ilvl[tcb->ilx].input_ptr == UClit(':') && *(tcb->ilvl[tcb->ilx].input_ptr + 1) == UClit(':'))
	       { cx = _NoCommand_ ; } ;					/* Got xxx:: - V4 Tag, not command */
	      switch(cx)
	       { case _If:
			if (*tcb->ilvl[tcb->ilx].input_ptr == UClit('('))	/* Got If(... -> V4 IntMod, not V4EVAL command */ 
			 { cx = UNUSED ; break ; } ;
			if ((*tcb->ilvl[tcb->ilx].input_ptr == UClit(' ')) && *(tcb->ilvl[tcb->ilx].input_ptr+1) == UClit('('))	/* Got If (... -> V4 IntMod, not V4EVAL command */ 
			 { cx = UNUSED ; break ; } ;
			if (tcb->ifx >= V4LEX_NestedIfThenElse - 1) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIfMaxNest",tcb->type) ; goto fail ; } ;
			tcb->ifx++ ; tcb->ifs[tcb->ifx-1].doif = FALSE ; tcb->ifs[tcb->ifx-1].doelse = FALSE ;
			if (!v4eval_SetIfLevel(ctx,tcb)) goto fail ; break ;
		 case _ElseIf: if (tcb->ifs[tcb->ifx-1].doelse) goto xct_command ;
								/* ElseIf drops thru if not checking the if */
		 case _Else:
			if (*tcb->ilvl[tcb->ilx].input_ptr == UClit('{'))
			 { cx = UNUSED ; break ; } ;
			if ((*tcb->ilvl[tcb->ilx].input_ptr == UClit(' ')) && *(tcb->ilvl[tcb->ilx].input_ptr+1) == UClit('{'))
			 { cx = UNUSED ; break ; } ;
			tcb->ifs[tcb->ifx-1].inif = FALSE ; if (!v4eval_CheckIfLevel(ctx,tcb)) goto fail ; break ;
		 case _EndIf:
			tcb->ifx-- ; if (!v4eval_CheckIfLevel(ctx,tcb)) goto fail ; break ;
		 case _EndLog: if (tcb->appendto != NULL) goto xct_command ;
	       } ;
	      for(;tcb->type!=V4LEX_TknType_EOL;)			/* Skip to end-of-statement/line */
	       { v4lex_NextTkn(tcb,V4LEX_Option_RetEOL+V4LEX_Option_ForceAsIs) ;
		 if (tcb->opcode == V_OpCode_Semi) break ;
	       } ;
	      continue ;
	    } ;
xct_command:
	   if (cx == _LBracket_)					/* Got "[" - fake a binding */
	    { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; gpi->LastV4CommandIndex = (cx = _Bind) ; goto bind_entry ;
	    } ;
	   tcb->UCkeyword[V4LEX_TCBMacro_NameMax] = UCEOS ; UCstrcpy(cname,tcb->UCkeyword) ;
	   if (cx < 0 && tcb->opcode == V_OpCode_Not)			/* If got "!" then probably embedded comment in table/macro save */
	     { if (tcb->ilvl[tcb->ilx].mode == V4LEX_InpMode_String)
	        { for(;;)
		   { v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
		     if (tcb->opcode == V_OpCode_Semi) { if (cnt == 1) cnt++ ; break ; } ;
		     if (tcb->type == V4LEX_TknType_EOL) break ;	/* Scan 'til "end-of-line" (or semi-colon) */
		   } ;
		} ; continue ;
	     } ;

/*	   If we think we got a V4 command (not BIND), check for "(" indicating a macro */
	   if (cx > 0 && !(cx==_Bind || cx==_Pattern || cx==_EBind || cx==_EqualSign_))
	    { v4lex_NextTkn(tcb,V4LEX_Option_RetEOL+V4LEX_Option_NextChar) ;
	      if (tcb->UCkeyword[0] == UClit('(')) cx = -1 ;	/* Got a macro */
	    } ;
	   switch (cx)
	    { default:
		if (UCstrcmp(tcb->UCkeyword,UClit(";")) == 0) break ;		/* Ignore extra ";"s */
		{ struct V4LEX__MacroEntry *vlme ;
		  struct V4LEX__MacroCallArgs *vlmca ;
		  v4lex_NextTkn(tcb,0) ;		/* Skip over the opening '(' after macro name */
		  vlme = v4eval_GetMacroEntry(ctx,cname,V4LEX_MACTYPE_Macro) ;
		  if (vlme == NULL)
		   { v_Msg(ctx,ctx->ErrorMsgAux,"MacroNotFnd",tcb->type,cname) ;
//v_Msg(ctx,NULL,"@*ptr = *%1U*\n",tcb->ilvl[tcb->ilx].input_ptr) ; vout_UCText(VOUT_Trace,0,ctx->ErrorMsg) ;
//v_Msg(ctx,NULL,"@*str = *%1U*\n",tcb->ilvl[tcb->ilx].input_str) ; vout_UCText(VOUT_Trace,0,ctx->ErrorMsg) ;
		     goto fail ;
		   } ;
		  vlmca = v4lex_ParseMacroCall(ctx,tcb,vlme) ;
		  if (vlmca == NULL) goto fail ;
		  v4lex_InvokeMacroCall(ctx,tcb,vlme,vlmca) ;
		}
		break ;
	      case _SemiColon_:
	      case _EndOfLine_:	/* End of line (i.e. blank) */	continue ;
	      case _LBraceSlash_:	/* Got "{/" - Create a new "macro" or text entry */
		if (v4eval_ParseMacroDef(ctx,tcb,trace) == NULL) goto fail ;
		break ;
	      case _Agg:		cx = _Aggregate ;	/* Fall thru */
	      case _BFAggregate:
	      case _FAggregate:
	      case _Aggregate:
		i = v4eval_AggParse(tcb,ctx,trace,NULL,(cx==_FAggregate || cx==_BFAggregate),cx==_BFAggregate) ;
		if (i <= 0)
		 { switch (i)
		    { case 0:		/* Warn */
			for(i=tcb->ilx;i>0;i--)
			 { if (tcb->ilvl[i].mode == V4LEX_InpMode_File || tcb->ilvl[i].mode == V4LEX_InpMode_CmpFile) break ; } ;
			if (tcb->ilx <= 1) goto fail ;		/* Not in "Include" file ? */
			v_Msg(ctx,UCTBUF1,"AggNestErr",ctx->ErrorMsg,ctx->ErrorMsgAux) ; vout_UCText(VOUT_Err,0,UCTBUF1) ;
			v_Msg(ctx,UCTBUF1,"@%0F - %0A\n") ; vout_UCText(VOUT_Err,0,UCTBUF1) ;
			gpi->ErrCount++ ; break ;
		      case -1:		/* Ignore */
			break ;
		      case -2:		/* Abort */
			goto fail ;
		    } ;
		 } ;
		break ;
	      case _CAggregate:
		if (!v4dpi_PointParse(ctx,&valpt,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
		if (!v4eval_ChkEOL(tcb)) goto fail ;
		if (v4eval_AggCAggPut(ctx,&valpt) == UNUSED)
		 goto fail ;
//		    v4_error(0,tcb,"V4","CAgg","NOPUTTOAGG","Could not write point to an Aggregate") ;
		break ;
	      case _Append:
	      case _Insert:
#ifdef APPENDINSERTCOMMANDS
		if (!v4dpi_PointParse(ctx,&isctpt,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
		if (!v4dpi_PointParse(ctx,&valpt,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
		isct = v4dpi_IsctEval(&xisct,&valpt,ctx,0,NULL,NULL) ;
		if (isct == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIsctFail",&valpt) ; goto fail ; } ;
		if (isct != &valpt) memcpy(&valpt,isct,isct->Bytes) ;
//		if (valpt.Grouping == V4DPI_Grouping_Current)		/* Pull value from context? {CONTEXT} */
//		 { DIMVAL(dpt,ctx,valpt.Dim) ;
//		   if (dpt == NULL)
//		    v4_error(V4E_PTNOTINCTX,tcb,"V4","AppIns","PTNOTINCTX","Could not find value point in current context") ;
//		   memcpy(&valpt,dpt,dpt->Bytes) ;
//		 } ;
		if (!v4eval_ChkEOL(tcb)) goto fail ;
//		if (gpi->MassUpdate)
		if (!v4dpi_BindListMake(NULL,NULL,NULL,ctx,NULL,0,0,,DFLTRELH))		/* Maybe flush out last binding nbl */
		 goto fail ;
//		 v4_error(0,tcb,"V4","INSERT","BINDERR",ctx->ErrorMsgAux) ;
		bind = (struct V4DPI__Binding *)v4dpi_IsctEval(&point,&isctpt,ctx,0,trace,&aid,NULL) ;
		if (bind == NULL)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdAppendErr",&isctpt) ; goto fail ; } ;
//		   v4_error(V4E_NOAPNDINSPT,tcb,"V4","Append","NOAPNDINSPT","No point found to APPEND/INSERT to") ;
		for(i=gpi->LowHNum;i<=gpi->HighHNum;i++)
		 { if (gpi->RelH[i].aid != aid) continue ;
		   if (!gpi->RelH[i].ahi.BindingsUpd) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdAppendNoUpd",gpi->RelH[i].pcb->FileName) ; goto fail ; } ;
		   break ;
		 } ;
		memcpy(&bindpt,bind,bind->Bytes) ;		/* Copy binding into temp buffer */
		bind = &bindpt ;
		dpt = (struct V4DPI__Point *)&bind->Buffer ;	/* Get pointer to list point */
		isct = (struct V4DPI__Point *)&bind->Buffer[bind->IsctIndex] ;
		memcpy(&isctpt,isct,isct->Bytes) ;		/* Copy intersection into temp */
		lp = ALIGNLP(&dpt->Value) ;	/* Link to list */
		for(l=0,i=gpi->LowHNum;i<=gpi->HighHNum;i++) { if (gpi->RelH[i].aid == aid) { l = i ; break ; } ; } ;
		if (!v4l_ListPoint_Modify(ctx,lp,(cx==11 ? V4L_ListAction_Append : V4L_ListAction_Insert),&valpt,0,l)) goto fail ;
		dpt->Bytes = ALIGN((char *)&dpt->Value.AlphaVal[lp->Bytes] - (char *)dpt) ;
		bind->IsctIndex = ALIGN(dpt->Bytes) ;		/* Get new offset for trailing intersection */
		memcpy(&bind->Buffer[bind->IsctIndex],&isctpt,isctpt.Bytes) ; /* Put intersection back */
		bind->Bytes = (char *)&bind->Buffer[bind->IsctIndex+isctpt.Bytes] - (char *)bind ;
/*		Rewrite the binding back from whence it came */
		v4is_PositionKey(aid,bind,NULL,0,V4IS_PosDCKL) ;	/* Reposition to get possibly changed dataid */
		v4is_Replace(aid,bind,bind,bind,bind->Bytes,V4IS_PCB_DataMode_Auto,V4IS_DataCmp_None,0,0) ;
		if (trace & V4TRACE_Lists)
		 { 
//		   v4dpi_PointToString(&result,dpt,ctx,-1) ;
		   v_Msg(ctx,ASCTBUF1,"@*List = %1P\n",dpt) ; vout_UCText(VOUT_Trace,0,ASCTBUF1) ;
		 } ;
		v4dpi_IsctEvalCacheClear(ctx,FALSE) ;				/* Clear out the IsctEval cache */
		break ;
#else
		v_Msg(ctx,ctx->ErrorMsgAux,"CmdAppendNLI") ; goto fail ;
#endif
	      case _Argument:
	      case _Arguments:
		b = UCEOS ;							/* Used to track if all start with same char */
		for(i=0;i<V4LEX_Tkn_ArgMax;i++)
		 { v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
		   if (tcb->type != V4LEX_TknType_Keyword) break ;
		   if (b == 0) { b = tcb->UCkeyword[0] ; }
		    else { if (b != tcb->UCkeyword[0]) b = TRUE ; } ;
		   tcb->UCkeyword[V4LEX_TCBMacro_NameMax] = UCEOS ;
		   UCstrcpy(tcb->ilvl[tcb->ilx].macarg[i].name,tcb->UCkeyword) ;	/* Just save in name list for this level */
		 } ;
		for(;i<V4LEX_Tkn_ArgMax;i++) { ZUS(tcb->ilvl[tcb->ilx].macarg[i].name) } ;
		tcb->ilvl[tcb->ilx].HaveMacroArgNames = b ;			/* Flag that we got names */
		break ;
	      case _Area:	/* AREA mode [fileame options] */
		if (gpi->RestrictionMap & V_Restrict_AreaCommand) { v_Msg(ctx,ctx->ErrorMsgAux,"RestrictCmd") ; goto fail ; } ;
		v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
		if (tcb->type == V4LEX_TknType_EOL) break ;		/* If no arguments then done! */
		if (tcb->opcode == V_OpCode_LBracket || tcb->opcode == V_OpCode_NCLBracket)	/* Got an intersection */
		 { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
		   i = v4dpi_PointParse(ctx,&isctpt,tcb,V4DPI_PointParse_RetFalse) ;	/* Parse the isct */
		   if (!i) goto fail ;
		   dpt = v4dpi_IsctEval(&valpt,&isctpt,ctx,0,NULL,NULL) ;
		   if (dpt == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIsctFail",&isctpt) ; goto fail ; } ;
/*		   If result is Logical:False or 0-length string then don't do anything */
		   if (dpt->PntType == V4DPI_PntType_Logical && dpt->Value.IntVal <= 0) break ;
		   v4im_GetPointUC(&i,UCTBUF1,V4TMBufMax,&valpt,ctx) ;
//		   if (UCstrlen(ucbuf) == 0) break ;
		   if (UCempty(UCTBUF1)) break ;
		   if (!i) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotStr",&isctpt,&valpt) ; goto fail ; } ;
		   v4lex_NestInput(tcb,NULL,UCTBUF1,V4LEX_InpMode_String) ;
		 } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
next_file:
		memset(&pcb,0,sizeof pcb) ; gotfile = TRUE ; ZUS(onerror) ; areaopen = 0 ;
		strcpy(pcb.V3name,"v4a") ; pcb.BktSize = V4Area_DfltBktSize ;
		pcb.DfltDataMode = V4IS_PCB_DataMode_Auto ; pcb.AccessMode = -1 ;
	   	pcb.DfltPutMode = V4IS_PCB_GP_Insert ; pcb.MinCmpBytes = 100 ;
		memset(&ahi,0,sizeof ahi) ; ZUS(filename) ;
		ahi.BindingsUpd = TRUE ; ahi.IntDictUpd = TRUE ; ahi.ExtDictUpd = TRUE ;
//		switch (v4eval_NextKeyWord(arealist,tcb,NOEOL))
		switch(v4im_GetTCBtoEnumVal(ctx,tcb,NOEOL))
		 { default:	v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,cx) ; goto fail ;
//				v4_error(V4E_INVAREAACCKW,tcb,"V4","Eval","INVAREAACCKW","Invalid AREA access keyword: %s",tcb->keyword) ;
		   case _Create:
			if (gpi->RestrictionMap & V_Restrict_AreaUpdate) { v_Msg(ctx,ctx->ErrorMsgAux,"RestrictCmd") ; goto fail ; } ;
			pcb.OpenMode = V4IS_PCB_OM_New ; ahi.RelHNum = V4DPI_dfltRelHNum ;
			break ;
		   case _Read:			pcb.OpenMode = V4IS_PCB_OM_Read ; break ;
		   case _ORead:			pcb.OpenMode = V4IS_PCB_OM_Read ; gotfile = FALSE ; break ;
		   case _Update:
			if (gpi->RestrictionMap & V_Restrict_AreaUpdate) { v_Msg(ctx,ctx->ErrorMsgAux,"RestrictCmd") ; goto fail ; } ;
			pcb.OpenMode = V4IS_PCB_OM_Update ; break ;
		   case _Close:
			v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
			if (tcb->type == V4LEX_TknType_EOL)	/* If just "Area Close" then close all areas */
			 { v4ctx_AreaClose(ctx,-1) ; goto end_area ; } ;
			v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
			pcb.OpenMode = UNUSED ; break ;
		   case _CreateIf:
			if (gpi->RestrictionMap & V_Restrict_AreaUpdate) { v_Msg(ctx,ctx->ErrorMsgAux,"RestrictCmd") ; goto fail ; } ;
			pcb.OpenMode = V4IS_PCB_OM_NewIf ; ahi.RelHNum = 5 ; break ;
		   case _Rename:
//			if (gpi->RestrictionMap & V_Restrict_AreaRename) { v_Msg(ctx,ctx->ErrorMsgAux,"RestrictCmd") ; goto fail ; } ;
			areaopen = 3 ; break ;
//			remove(argbuf) ;				/* Try to delete new file name */
//			if (rename(filename,argbuf) == -1)
//			 { sprintf(ASCTBUF1,"*Rename to area %s failed (%s)\n",argbuf,v_OSErrString(errno)) ; vout_Text(VOUT_Err,0,ASCTBUF1) ; } ;
//			goto end_area ;
		   case _Index:
			if (gpi->RestrictionMap & V_Restrict_AreaUpdate) { v_Msg(ctx,ctx->ErrorMsgAux,"RestrictCmd") ; goto fail ; } ;
			areaopen = 4 ; break ;
		   case _Rebuild:
			if (gpi->RestrictionMap & V_Restrict_AreaUpdate) { v_Msg(ctx,ctx->ErrorMsgAux,"RestrictCmd") ; goto fail ; } ;
			areaopen = 5 ; break ;
		   case _Reset:
			if (gpi->RestrictionMap & V_Restrict_AreaUpdate) { v_Msg(ctx,ctx->ErrorMsgAux,"RestrictCmd") ; goto area_fail ; } ;
			areaopen = 2 ; break ;

		 } ;
		if (v_UCGetEnvValue(UClit("v4_AreaPath"),(UCCHAR *)result,UCsizeof(result)))	/* If logical defined then preface file */
		 { ucb = UClit("v4_AreaPath:") ; } else { ucb = NULL ; } ;
		v_ParseFileName(tcb,pcb.UCFileName,ucb,UClit(".v4a"),&dfltext) ; dfltext = !dfltext ;
		if (pcb.OpenMode == V4IS_PCB_OM_NewIf)		/* See if we should create (want flag for later use */
		 { struct UC__File UCFile ;
		   if (v_UCFileOpen(&UCFile,pcb.UCFileName,UCFile_Open_Read,TRUE,ctx->ErrorMsgAux,0)) { v_UCFileClose(&UCFile) ; } else { pcb.OpenMode = V4IS_PCB_OM_New ; } ;
//		   if ((fp=fopen(v3_logical_decoder(pcb.FileName,TRUE),"r")) != NULL) { fclose(fp) ; } else { pcb.OpenMode = V4IS_PCB_OM_New ; } ;
		 } ;
		switch (areaopen)
		 {
		   case 5:		/* Area Rebuild - pick up target name */
		   case 3:		/* Area Rename - pick up target name */
		     v_ParseFileName(tcb,filename,NULL,UClit(".v4a"),NULL) ;
		     ucb = v_UCLogicalDecoder(filename,VLOGDECODE_Exists,0,ctx->ErrorMsgAux) ; if (ucb == NULL) goto fail ;
		     UCstrcpy(filename,ucb) ;
		     break ;

		 } ;
		if (!gotfile)					/* See if we should look for file */
		 { struct UC__File UCFile ;
		   if (v_UCFileOpen(&UCFile,pcb.UCFileName,UCFile_Open_Read,TRUE,ctx->ErrorMsgAux,0)) { gotfile = TRUE ; v_UCFileClose(&UCFile) ; } ;
//		   if ((fp=fopen(v3_logical_decoder(pcb.FileName,TRUE),"r")) != NULL) { gotfile = TRUE ; fclose(fp) ; } ;
		 } ;
		ZUS(newmacro) ; ZPH(&idpt) ; retry = 3 ;
/*		Parse any additional options */
		for(more=FALSE,looper=TRUE;looper;)
		 { 
//		   switch (v4eval_NextKeyWord(ahilist,tcb,EOLOK))
		   switch(v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
		    { default:	v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,cx) ; goto fail ;
//				v4_error(V4E_UNKAREAATTR,tcb,"V4","Eval","UNKAREAATTR","Unknown AREA attribute: %s",tcb->keyword) ;
		      case _Comma_:		looper = FALSE ; more = TRUE ; break ;
		      case _EndOfLine_:		looper = FALSE ; break ;
		      case _SemiColon_:		if (cnt == 1) cnt++ ; looper = FALSE ; break ;
		      case _Hierarchy:		v4lex_NextTkn(tcb,0) ; ahi.RelHNum = tcb->integer ;
						if (ahi.RelHNum < 0 || ahi.RelHNum > V4C_CtxRelH_Max || ahi.RelHNum == V4DPI_WorkRelHNum)
						 { v_Msg(ctx,ctx->ErrorMsgAux,"AreaHNum",UNUSED,ahi.RelHNum,0,V4C_CtxRelH_Max,V4DPI_WorkRelHNum) ; goto fail ; } ;
						break ;
		      case _ExtDictUpd:		ahi.ExtDictUpd = TRUE ; break ;
		      case _IntDictUpd:		ahi.IntDictUpd = TRUE ; break ;
		      case _BindingUpd:		ahi.BindingsUpd = TRUE ; break ;
		      case _NoExtDictUpd:	ahi.ExtDictUpd = FALSE ; break ;
		      case _NoIntDictUpd:	ahi.IntDictUpd = FALSE ; break ;
		      case _NoBindingUpd:	ahi.BindingsUpd = FALSE ; break ;
		      case _NewMacro:
				v4lex_NextTkn(tcb,0) ;
				tcb->UCkeyword[V4LEX_TCBMacro_NameMax] = UCEOS ; UCstrcpy(newmacro,tcb->UCkeyword) ; break ;
		      case _Id:
				if (!v4dpi_PointParse(ctx,&valpt,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
				if (valpt.Bytes > sizeof(struct V4DPI__LittlePoint)) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntTooBig",DE(Id)) ; goto fail ; } ;
				memcpy(&idpt,&valpt,valpt.Bytes) ; break ;
		      case _BucketSize:
				v4lex_NextTkn(tcb,0) ;
				if (tcb->type != V4LEX_TknType_Integer)
				 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdBktSize",tcb->type) ; goto fail ; } ;
//				 v4_error(V4E_INVBKTSIZENUM,tcb,"V4","Eval","INVBKTSIZENUM","Expecting an integer bucket size number") ;
				if (pcb.BktSize == V4AreaAgg_DfltBktSize)
				 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdBktSizeAgg") ; goto fail ; } ;
//				 v4_error(0,tcb,"V4","Eval","CHGBKTSIZE","Cannot specify bucket size for Aggregate area") ;
				pcb.BktSize = tcb->integer ;
				if (pcb.BktSize < 4*V4IS_BktIncrement || pcb.BktSize > V4DPI_Binding_BufMax)
				 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdBktSizeBnd",pcb.BktSize,V4IS_BktIncrement*4,V4DPI_Binding_BufMax) ; goto fail ; } ;
//				 v4_error(V4E_INVBKTSIZE,tcb,"V4","Eval","INVBKTSIZE","Bucket size (%d) must be between %d and %d",
//					pcb.BktSize,V4IS_BktIncrement*4,V4DPI_Binding_BufMax) ;
				break ;
		      case _Agg:
		      case _Aggregate:
				strcpy(pcb.V3name,"agg") ; pcb.BktSize = V4AreaAgg_DfltBktSize ;
			   	pcb.DfltPutMode |= V4IS_PCB_CmpOnOverload ;
				break ;
		      case _FastAdd:
				pcb.DfltPutMode = (pcb.DfltPutMode & 0xffff0000) + V4IS_PCB_GP_DataOnly ; break ;
		      case _Dictionary:
		      case _External:
				if (areaopen != 0) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdAreaExtInv",tcb->type) ; goto fail ; } ;
				areaopen = 1 ; break ;
#ifdef XDICTMU
		      case _Locks:
				pcb.LockMode = (pcb.OpenMode == V4IS_PCB_OM_Read ? V4IS_PCB_LM_Get : V4IS_PCB_LM_Update) ;
				break ;
#endif
		      case _Retry:
				v4lex_NextTkn(tcb,0) ;
				if (tcb->type != V4LEX_TknType_Integer) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;
				retry = tcb->integer ; break ;
		      case _OnError:
				v4lex_NextTkn(tcb,0) ;
				if (tcb->type == V4LEX_TknType_Keyword)
				 { switch (v4im_GetUCStrToEnumVal(tcb->UCkeyword,0))
				    { default:		break ;
				      case _Fail:
				      case _Exxit:	UCstrcpy(onerror,UClit("***FAIL***")) ; break ;
				      case _Ignore:	UCstrcpy(onerror,UClit("***IGNORE***")) ; break ;
				    } ;
//				   if (strcmp(tcb->keyword,"FAIL") == 0 || strcmp(tcb->keyword,"EXIT") == 0)
//				    { strcpy(onerror,"***FAIL***") ; break ; }
//				    else if (strcmp(tcb->keyword,"IGNORE") == 0) { strcpy(onerror,"***IGNORE***") ; break ; } ;
				 } ;
				v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
				v_ParseFileName(tcb,onerror,NULL,UClit(".v4"),NULL) ;
		      case _Scan:
				pcb.AreaFlags |= V4IS_PCB_OF_SeqScan ;
				break ;
		    } ;
		   if (looper)
		    { v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;		/* Grab next token */
		      if (tcb->opcode == V_OpCode_Comma) { looper = FALSE ; more = TRUE ; }
		       else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
		    } ;
		 } ;
		if (!gotfile)					/* If we don't got file then don't try to open */
		 { if (more) goto next_file ;
		   goto end_area ;
		 } ;
		if (pcb.OpenMode == UNUSED)			/* Want to close off area ? */
		 { UCCHAR tfile[V_FileName_Max] ;
		   ucb = v_UCLogicalDecoder(pcb.UCFileName,VLOGDECODE_Exists,0,ctx->ErrorMsgAux) ; if (ucb == NULL) goto area_fail ;
		   UCstrcpy(tfile,ucb) ;
		   for(i=0;i<gpi->AreaAggCount;i++)
	            { if (UCstrcmp(tfile,gpi->AreaAgg[i].pcb->UCFileName) != 0) continue ;
		      v4ctx_AreaClose(ctx,gpi->AreaAgg[i].pcb->AreaId) ;
		      break ;
		    } ;
		   if (i >= gpi->AreaAggCount)
		    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdAreaNotOpn",pcb.UCFileName) ; goto fail ; } ;
//		    v4_error(0,tcb,"V4","Eval","NOTISCT","Area CLOSE error - Aggregate(%s) not open ",pcb.FileName) ;
		   if (more) goto next_file ;
		   goto end_area ;
		 } ;
		switch (areaopen)
		 {
		   case 1:	/* External Dictionary */
		     if (gpi->xdr != NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdAreaExtOpen",gpi->xdr->pcb->UCFileName) ; goto area_fail ; } ;
		     pcb.AreaFlags |= V4IS_PCB_OF_NoError ;
		     for(i=0;;i++)
		      { if (!v4is_Open(&pcb,NULL,ctx->ErrorMsgAux))
			 { if (i >= retry) goto area_fail ;		/* If failed then maybe retry */
			   if (i > 0) { v_Msg(ctx,UCTBUF1,"AreaRetry",pcb.UCFileName,ctx->ErrorMsgAux,i+1,retry) ; vout_UCText(VOUT_Warn,0,UCTBUF1) ; } ;
			   HANGLOOSE(100) ; continue ;
			 } ;
			break ;
		      } ;
		     gpi->xdr = (struct V4DPI__XDictRuntime *)v4mm_AllocChunk(sizeof *gpi->xdr,TRUE) ;
		     gpi->xdr->pcb = (struct V4IS__ParControlBlk *)v4mm_AllocChunk(sizeof *gpi->xdr->pcb,FALSE) ;
		     memcpy(gpi->xdr->pcb,&pcb,sizeof pcb) ; gpi->xdr->aid = pcb.AreaId ;
#ifdef V4ENABLEMULTITHREADS
		     INITMTLOCK(gpi->xdr->mtLock) ;			/* Used for multi-threaded spin-lock */
#endif
		     break ;
		   case 2:	/* Reset */
		   { UCCHAR tfile[V_FileName_Max] ;
		     if (UCstrrchr(pcb.UCFileName,'.') == NULL) UCstrcat(pcb.UCFileName,UClit(".v4x")) ; 
		     ucb = v_UCLogicalDecoder(pcb.UCFileName,VLOGDECODE_Exists,0,ctx->ErrorMsgAux) ; if (ucb == NULL) goto area_fail ;
		     UCstrcpy(pcb.UCFileName,ucb) ; UCstrcpy(filename,pcb.UCFileName) ;
		     UCstrcpy(tfile,pcb.UCFileName) ; *(ucb=UCstrrchr(tfile,'.')) = UCEOS ; UCstrcat(tfile,UClit(".old")) ;
		     UCremove(v_UCLogicalDecoder(tfile,VLOGDECODE_NewFile,0,ctx->ErrorMsgAux)) ;
//		     remove(v3_logical_decoder(result,FALSE)) ;
		     if (!UCrename(pcb.UCFileName,tfile)) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdRename",pcb.UCFileName,tfile,errno) ; goto area_fail ; } ;
		     memset(&pcb,0,sizeof pcb) ; strcpy(pcb.V3name,"testu") ;
		     pcb.OpenMode = V4IS_PCB_OM_Read ; pcb.DfltGetMode = V4IS_PCB_GP_Next ;
		     UCstrcpy(pcb.UCFileName,tfile) ; v4is_Open(&pcb,NULL,NULL) ;
		     if (pcb.AreaId == V4IS_AreaId_SeqOnly)
		      { v_Msg(ctx,ctx->ErrorMsgAux,"CmdSeqOnly",pcb.UCFileName) ; goto area_fail ; } ;
		     dpcb = (struct V4IS__ParControlBlk *)v4mm_AllocChunk(sizeof *dpcb,TRUE) ;
		     strcpy(dpcb->V3name,"dstu") ; UCstrcpy(dpcb->UCFileName,filename) ;	/* filename = original first argument of Reset command */
		     dpcb->AccessMode = -1 ; dpcb->OpenMode = V4IS_PCB_OM_NewAppend ; dpcb->DfltPutMode = V4IS_PCB_GP_Append ;
		     acb = (struct V4IS__AreaCB *)v4mm_ACBPtr(pcb.AreaId) ; dpcb->BktSize = acb->RootInfo->BktSize ;
		     v4is_Open(dpcb,NULL,NULL) ;
		     COPYJMP(traceback,(gpi->environment)) ;
		     i = v4is_Reformat(&pcb,dpcb,NULL,FALSE,V4IS_Reformat_Silent|V4IS_Reformat_XDict|V4IS_Reformat_NoContPrompt) ;
		     COPYJMP((gpi->environment),traceback) ;
		     v4is_Close(&pcb) ; v4is_Close(dpcb) ;
		     if (i == UNUSED) { v_Msg(ctx,ctx->ErrorMsgAux,"V4ISRefErr",pcb.UCFileName,dpcb->UCFileName) ; goto area_fail ; } ;
		     goto end_area ;
		   }
		   case 3:	/* Rename/Move */
		     if ((ucb = v_UCLogicalDecoder(filename,VLOGDECODE_NewFile,0,ctx->ErrorMsgAux)) == NULL) goto area_fail ;
		     UCstrcpy(filename,ucb) ; 
		     for(;;)
		      {
		        if (UCremove(filename) != 0)
		         { if (retry > 0) { retry-- ; HANGLOOSE(500) ; continue ; } ;
		           v_Msg(ctx,ctx->ErrorMsgAux,"AreaRename",errno) ; goto area_fail ;
		         } ;
		        ucb = v_UCLogicalDecoder(pcb.UCFileName,VLOGDECODE_Exists,0,ctx->ErrorMsgAux) ; if (ucb == NULL) goto area_fail ;
		        UCstrcpy(pcb.UCFileName,ucb) ;
		        if (!UCmove(pcb.UCFileName,filename))
		         { v_Msg(ctx,ctx->ErrorMsgAux,"CmdAreaRename",pcb.UCFileName,filename,errno) ; goto area_fail ; } ;
		        break ;
		      } ;
		     goto end_area ;
		   case 4:	/* Index */
		     memset(&pcb,0,sizeof pcb) ; strcpy(pcb.V3name,"testu") ;
		     pcb.OpenMode = V4IS_PCB_OM_Update ; pcb.DfltGetMode = V4IS_PCB_GP_Next ;
		     v4is_Open(&pcb,NULL,NULL) ;
		     if (pcb.AreaId == V4IS_AreaId_SeqOnly)
		      { v_Msg(ctx,ctx->ErrorMsgAux,"CmdSeqOnly",filename) ; goto area_fail ; } ;
		     v4is_RebuildAreaIndex(&pcb) ;
		     goto end_area ;
		   case 5:	/* Rebuild */
		     pcb.OpenMode = V4IS_PCB_OM_Read ; pcb.DfltGetMode = V4IS_PCB_GP_Next ; pcb.AreaFlags |= V4IS_PCB_OF_NoError ;
		     if (!v4is_Open(&pcb,NULL,ctx->ErrorMsgAux)) goto area_fail ;
		     if (pcb.AreaId == V4IS_AreaId_SeqOnly) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdSeqOnly",filename) ; goto area_fail ; } ;
		     dpcb = (struct V4IS__ParControlBlk *)v4mm_AllocChunk(sizeof *dpcb,TRUE) ;
		     strcpy(dpcb->V3name,"dstu") ; UCstrcpy(dpcb->UCFileName,filename) ; dpcb->AreaFlags |= V4IS_PCB_OF_NoError ;
		     dpcb->AccessMode = -1 ; dpcb->OpenMode = V4IS_PCB_OM_NewAppend ; dpcb->DfltPutMode = V4IS_PCB_GP_Append ;
		     acb = (struct V4IS__AreaCB *)v4mm_ACBPtr(pcb.AreaId) ; dpcb->BktSize = acb->RootInfo->BktSize ;
		     if (!v4is_Open(dpcb,NULL,ctx->ErrorMsgAux)) goto area_fail ;
		     COPYJMP(traceback,(gpi->environment)) ;
		     i = v4is_Reformat(&pcb,dpcb,NULL,FALSE,V4IS_Reformat_NoContPrompt) ;
		     COPYJMP((gpi->environment),traceback) ;
		     v4is_Close(&pcb) ; v4is_Close(dpcb) ;
		     if (i == UNUSED) { v_Msg(ctx,ctx->ErrorMsgAux,"V4ISRefErr",pcb.UCFileName,dpcb->UCFileName) ; goto area_fail ; } ;
		     goto end_area ;
		   default:
		     pcb.RelHNum = ahi.RelHNum ; pcb.AreaFlags |= V4IS_PCB_OF_NoError ;
		     for(i=0;;i++)
		      { if (!v4is_Open(&pcb,NULL,ctx->ErrorMsgAux))
			 { if (i >= retry) goto area_fail ;		/* If failed then maybe retry */
			   if (i > 0) { v_Msg(ctx,UCTBUF1,"AreaRetry",pcb.UCFileName,ctx->ErrorMsgAux,i+1,retry) ; vout_UCText(VOUT_Warn,0,UCTBUF1) ; } ;
			   HANGLOOSE(100) ; continue ;
			 } ;
			break ;
		      } ;
		     if (pcb.AreaId == V4IS_AreaId_SeqOnly) { v_Msg(ctx,ctx->ErrorMsgAux,"V4ISInvFormat",pcb.UCFileName) ; goto area_fail ; } ;
		     if (strcmp(pcb.V3name,"agg") == 0)
		      { if (v4im_AggLoadArea(ctx,&pcb,&idpt) == UNUSED) goto area_fail ;
		        break ;
		      } ;
		     if (v4ctx_AreaAdd(ctx,&pcb,&ahi,&idpt) == UNUSED) goto area_fail ;
		     if (pcb.OpenMode == V4IS_PCB_OM_New && UCstrlen(newmacro))	/* If new file then maybe init with macro? */
		      { UCstrcat(newmacro,UClit("()\n")) ; v4lex_NestInput(tcb,NULL,newmacro,V4LEX_InpMode_String) ; } ;
		 } ;

		if (more) goto next_file ;
end_area:
		break ;

area_fail:
		if (UCempty(onerror)) goto fail ;	/* Fail unless we have an OnError Include file to execute */
		if (UCstrcmp(onerror,UClit("***FAIL***")) == 0)
		 { v_Msg(ctx,NULL,"*CmdAreaExit",filename) ; vout_UCText(VOUT_Trace,0,ctx->ErrorMsg) ;
		   return(V4EVAL_Res_ExitStats) ;
		 } ;
		if (UCstrcmp(onerror,UClit("***IGNORE***")) == 0)
		 { v_Msg(ctx,NULL,"*CmdAreaIgnore",filename) ; vout_UCText(VOUT_Trace,0,ctx->ErrorMsg) ;
		   break ;
		 } ;
		v_Msg(ctx,NULL,"*CmdAreaInclude",filename) ; vout_UCText(VOUT_Trace,0,ctx->ErrorMsg) ;
//		if ((incfp = fopen(v3_logical_decoder(UCretASC(filename),TRUE),"r")) == 0)
//		if (incfp == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNoInclude",errno,filename) ; goto fail ; } ;
		if (!v_UCFileOpen(&UCFile,filename,UCFile_Open_Read,TRUE,ctx->ErrorMsgAux,0)) goto fail ;
		v4lex_NestInput(tcb,&UCFile,filename,V4LEX_InpMode_File) ;
		break ;
	      case _DSI:			/* Same as 'Debug StepInto' */
		if (!v4eval_ChkEOL(tcb)) goto fail ;
		if (respnt != NULL)		/* Only meaningful if from within debugger, otherwise drop thru to 'Step' */
		 { return(V4EVAL_Res_Flag_ShowRes | V4EVAL_Res_BPStepInto) ;
		 } ;
	      case _DS:				/* Same as 'Debug Step' or 'Debug Source file' */
		if (!v4eval_ChkEOL(tcb)) { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; goto dbg_source_entry ; } ;
		if (respnt == NULL)		/* This means we are not called from breakpoint - have to fake one in */
		 { if (gpi->bpl == NULL) v4dbg_bplInit() ;
		   gpi->bpl->rtStackXStep = ctx->rtStackX ;
		 } ;
		return(V4EVAL_Res_Flag_ShowRes | V4EVAL_Res_BPStep) ;
	      case _DSN:				/* Same as 'Debug SNC' */
		if (!v4eval_ChkEOL(tcb)) goto fail ;
		if (respnt == NULL)		/* This means we are not called from breakpoint - have to fake one in */
		 { if (gpi->bpl == NULL) v4dbg_bplInit() ;
		   gpi->bpl->rtStackXStep = ctx->rtStackX ;
		 } ;
		return(V4EVAL_Res_Flag_ShowRes | V4EVAL_Res_BPStepNC) ;
	      case _DC:			/* Same as 'Debug Continue' */
		if (!v4eval_ChkEOL(tcb)) goto fail ;
		return(V4EVAL_Res_BPContinue) ;
	      case _DB:				/* Same as 'Debug Break line#' */
		goto dbg_break_entry ;
	      case _D:
	      case _Debug:
	      { struct V4DPI__Point ifPt, doPt, retPt ;
		struct V4DBG__BreakPoint *vbp ;
		int brkNum, brkAction, brkAfter, brkIntMod, flags ; LOGICAL autoStep ;
		switch(v4im_GetTCBtoEnumVal(ctx,tcb,NOEOL))
		 { default:	v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,cx) ; goto fail ;
dbg_break_entry:   case _Break:
			brkNum = UNUSED ; brkAction=0 ; brkAfter=UNUSED ; brkIntMod=UNUSED ; flags=V4DBG_bpFlag_Default ; autoStep=FALSE ;
			v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
			if (tcb->type == V4LEX_TknType_EOL)		/* List the current breakpoints */
			 { 
			   if (gpi->bpl == NULL ? TRUE : gpi->bpl->Count == 0) { v_Msg(ctx,UCTBUF2,"DBGNoBPs") ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;  break ; } ;
			   for(i=0;i<gpi->bpl->Count;i++)
			    { vout_UCText(VOUT_Trace,0,v4dbg_FormatVBP(ctx,&gpi->bpl->vbp[i],&gpi->bpl->vbp[i].vis,0,FALSE)) ; vout_NL(VOUT_Trace) ;
			    } ;
			   break ;
			 } ;
			if (tcb->type == V4LEX_TknType_Integer)
			 { brkNum = tcb->integer ;
			 } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
			ifPt.Bytes = 0 ; doPt.Bytes = 0 ;
			for(looper=TRUE;looper;)
			 { v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
			   switch (tcb->type)
			    { 
			      case V4LEX_TknType_EOL:
				looper = FALSE ; break ;
			      case V4LEX_TknType_Keyword:
				v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
				switch(v4im_GetTCBtoEnumVal(ctx,tcb,NOEOL))
				 { default:	v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,cx) ; goto fail ;
				   case _After:		v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ; if (tcb->type != V4LEX_TknType_Integer) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;
							brkAfter = tcb->integer ; break ;
				   case _AutoStep:	autoStep = TRUE ; break ;
				   case _Break:		flags |= V4DBG_bpFlag_Break ; break ;
				   case _Context:	flags |= V4DBG_bpFlag_Context ; break ;
				   case _Continue:	flags &= ~(V4DBG_bpFlag_Break) ; break ;
				   case _Delete:	brkAction=1 ; break ;
				   case _Do:		if (!v4dpi_PointParse(ctx,&doPt,tcb,V4DPI_PointParse_RetFalse)) goto fail ; break ;
				   case _Evaluations:	flags |= V4DBG_bpFlag_EvalCount ; break ;
				   case _Fail:		flags &= ~V4DBG_bpFlag_DoNotEval ; flags |= V4DBG_bpFlag_BreakOnFail ; break ;
				   case _If:		if (!v4dpi_PointParse(ctx,&ifPt,tcb,V4DPI_PointParse_RetFalse)) goto fail ; break ;
				   case _Location:	flags |= V4DBG_bpFlag_Loc ; break ;
				   case _Module:	v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ; if (tcb->type != V4LEX_TknType_Keyword) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdKWStr",tcb->type) ; goto fail ; } ;
							brkIntMod = v4im_Accept(tcb->UCkeyword) ; if (brkIntMod <= 0) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIAcpNoIntMod",tcb->UCkeyword) ; goto fail ; } ;
							break ;
				   case _Once:		flags |= V4DBG_bpFlag_OneTime ; break ;
				   case _Point:		flags |= V4DBG_bpFlag_Isct ; break ;
				   case _Results:	flags |= V4DBG_bpFlag_EvalRes ; break ;
				   case _Return:	if (!v4dpi_PointParse(ctx,&retPt,tcb,V4DPI_PointParse_RetFalse)) goto fail ; break ;
				   case _Silent:	flags |= ~(V4DBG_bpFlag_Context | V4DBG_bpFlag_Stack | V4DBG_bpFlag_Loc) ; break ;
				   case _Skip:		flags &= ~V4DBG_bpFlag_BreakOnFail ; flags |= V4DBG_bpFlag_DoNotEval ; break ;
				   case _Stack:		flags |= V4DBG_bpFlag_Stack ; break ;
				   case _Time:		flags |= (V4DBG_bpFlag_WallCPU | V4DBG_bpFlag_DeltaWallCPU) ; break ;
				 } ;
			    } ;
			 } ;
			switch (brkAction)
			 { case 0:			/* Add new breakpoint */
				if (gpi->bpl == NULL) v4dbg_bplInit() ;
				if (gpi->bpl->Count >= V4DBG_Breakpoint_Max) { v_Msg(ctx,ctx->ErrorMsgAux,"DBGBPMax",tcb->type,V4DBG_Breakpoint_Max) ; goto fail ; } ;
				vbp = &gpi->bpl->vbp[gpi->bpl->Count] ; INITBP(vbp) ;
				if (brkNum != UNUSED)
				 { if (gpi->bpl->visSource.iVal == 0) { v_Msg(ctx,ctx->ErrorMsgAux,"DBGNoSrcSel",tcb->type) ; goto fail ; } ;
				   vbp->vis = gpi->bpl->visSource ; vbp->vis.c.lineNumber = brkNum * 10 ;
				 } ;
				if (doPt.Bytes != 0) { vbp->evalPt = (P *)v4mm_AllocChunk(doPt.Bytes,FALSE) ; memcpy(vbp->evalPt,&doPt,doPt.Bytes) ; } ;
				if (ifPt.Bytes != 0) { vbp->condPt = (P *)v4mm_AllocChunk(ifPt.Bytes,FALSE) ; memcpy(vbp->condPt,&ifPt,ifPt.Bytes) ; } ;
				if (retPt.Bytes != 0) { vbp->resPt = (P *)v4mm_AllocChunk(retPt.Bytes,FALSE) ; memcpy(vbp->resPt,&retPt,retPt.Bytes) ; } ;
				if (brkIntMod != UNUSED) vbp->intmodX = brkIntMod ;
				vbp->bpFlags = flags ;
				if (brkAfter != UNUSED) { vbp->isctEvals = brkAfter ; vbp->bpFlags |= V4DBG_bpFlag_OneTime ; } ;
				if (autoStep)
				 { vbp->lineNumber = 0 ; vbp->rtStackX = ctx->rtStackX - 1 ; } ;
				vbp->bpId = ++gpi->bpl->luId ; gpi->bpl->Count++ ;
				v_Msg(ctx,UCTBUF2,"DBGBPAdd",v4dbg_FormatBP(vbp->bpId),V4DBG_Breakpoint_Max-gpi->bpl->Count) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
				break ;
			   case 1:			/* Remove breakpoint */
				if (brkNum == UNUSED) { v_Msg(ctx,ctx->ErrorMsgAux,"DBGBPDel",tcb->type) ; goto fail ; } ;
				if (gpi->bpl == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"DBGBPNotFnd",tcb->type,brkNum) ; goto fail ; } ;
				for(i=0;i<gpi->bpl->Count;i++) { if (gpi->bpl->vbp[i].bpId == brkNum) break ; } ;
				if (i >= gpi->bpl->Count) { v_Msg(ctx,ctx->ErrorMsgAux,"DBGBPNotFnd",tcb->type,brkNum) ; goto fail ; } ;
				for (;i<gpi->bpl->Count;i++) { gpi->bpl->vbp[i] = gpi->bpl->vbp[i+1] ; } ;
				gpi->bpl->Count-- ;
				v_Msg(ctx,UCTBUF2,"DBGBPRmv",brkNum,gpi->bpl->Count) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
				break ;
			 } ;
			break ;
		   case _Continue:
			if (!v4eval_ChkEOL(tcb)) goto fail ;
			return(V4EVAL_Res_BPContinue) ;
		   case _Source:
dbg_source_entry:	v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
			switch (tcb->type)
			 {
			   case V4LEX_TknType_EOL:
				{ struct V4LEX__CompileDir *vcd ; int relh,sfx=0 ; UCCHAR flag[4],nbuf[8] ;
				  for (relh=gpi->HighHNum;relh>=gpi->LowHNum;relh--)
				   { if (gpi->RelH[relh].aid == UNUSED) continue ;
				     if ((vcd = v4trace_LoadVCDforHNum(ctx,relh,FALSE)) == NULL) continue ;
				     for(i=0;i<vcd->fileCount;i++)
				      { sfx++ ;
				        UCstrcpy(flag,(gpi->bpl == NULL ? UClit("  ") : (gpi->bpl->visSource.c.HNum==relh && gpi->bpl->visSource.c.vcdIndex==i ? UClit(" *") : UClit("  ")))) ;
				        UCsprintf(nbuf,UCsizeof(nbuf),UClit("%2d"),sfx) ;
				        v_Msg(ctx,UCTBUF2,"@%1U%2U. %3U\n",flag,nbuf,vcd->File[i].fileName) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
				      } ;
				   } ;
				}
				break ;
			   case V4LEX_TknType_Integer:
				{ struct V4LEX__CompileDir *vcd ; int relh,sfx=0,matches=0 ; UCCHAR nbuf[8] ;
				  for (relh=gpi->HighHNum;matches==0 && relh>=gpi->LowHNum;relh--)
				   { if (gpi->RelH[relh].aid == UNUSED) continue ;
				     if ((vcd = v4trace_LoadVCDforHNum(ctx,relh,FALSE)) == NULL) continue ;
				     for(i=0;matches==0 && i<vcd->fileCount;i++)
				      { sfx++ ; 
					if (sfx != tcb->integer) continue ;
					if (gpi->bpl == NULL) v4dbg_bplInit() ;
				        if (vcd->File[i].spwHash64 != 0 && vcd->File[i].spwHash64 != gpi->spwHash64) break ;
					matches++ ; gpi->bpl->visSource.c.HNum = relh ; gpi->bpl->visSource.c.vcdIndex = i ;
				        UCsprintf(nbuf,UCsizeof(nbuf),UClit("%2d"),sfx) ;
				        v_Msg(ctx,UCTBUF2,"@ *%1U. %2U\n",nbuf,vcd->File[i].fileName) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
				      } ;
				   } ;
				  if (matches == 0) { v_Msg(ctx,ctx->ErrorMsgAux,"DBGNoSrcFileX",tcb->type,tcb->integer) ; goto fail ; } ;
				}
				break ;
			   case V4LEX_TknType_Keyword:
				{ struct V4LEX__CompileDir *vcd ; int relh,sfx=0,saverelh,savesfx,savefc,matches=0 ; UCCHAR savefn[V_FileName_Max],nbuf[8] ;
				  for (relh=gpi->HighHNum;relh>=gpi->LowHNum;relh--)
				   { if (gpi->RelH[relh].aid == UNUSED) continue ;
				     if ((vcd = v4trace_LoadVCDforHNum(ctx,relh,FALSE)) == NULL) continue ;
				     for(i=0;i<vcd->fileCount;i++)
				      { sfx++ ; 
				        if (vcd->File[i].spwHash64 != 0 && vcd->File[i].spwHash64 != gpi->spwHash64) continue ;
					if (vuc_StrStrIC(vcd->File[i].fileName,tcb->UCstring) == NULL) continue ;
					if (matches > 0) { UCsprintf(nbuf,UCsizeof(nbuf),UClit("%2d"),savesfx) ; v_Msg(ctx,UCTBUF2,"@%1U %2U\n",nbuf,savefn) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ; } ;
					matches++ ; saverelh = relh ; savesfx = sfx ; savefc = i ; UCstrcpy(savefn,vcd->File[i].fileName) ;
					
				      } ;
				   } ;
				  if (matches > 1)
				   { UCsprintf(nbuf,UCsizeof(nbuf),UClit("%2d"),savesfx) ;
				     v_Msg(ctx,UCTBUF2,"@%1U %2U\n",nbuf,savefn) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
				     v_Msg(ctx,ctx->ErrorMsgAux,"DBGMultSrcFile",tcb->type,tcb->UCstring) ; goto fail ;
				   } ;
				  if (matches == 0) { v_Msg(ctx,ctx->ErrorMsgAux,"DBGNoSrcFile",tcb->type,tcb->UCstring) ; goto fail ; } ;
				  if (gpi->bpl == NULL) v4dbg_bplInit() ;
				  gpi->bpl->visSource.c.HNum = saverelh ; gpi->bpl->visSource.c.vcdIndex = savefc ;
				  UCsprintf(nbuf,UCsizeof(nbuf),UClit("%2d"),savesfx) ;
				  v_Msg(ctx,UCTBUF2,"@ *%1U. %2U\n",nbuf,savefn) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
				}
				break ;
			 } ;
			break ;
		   case _Stack:
			if (!v4eval_ChkEOL(tcb)) goto fail ;
			for(i=ctx->rtStackX;i>0;i--)
			 { UCCHAR *srcFile ; int hnum = ctx->rtStack[i].vis.c.HNum ; struct V4LEX__CompileDir *vcd ;
/*			   Try to link up to vcd for this hnum */
			   if (ctx->rtStack[i].isctPtr == NULL) continue ;
			   if ((vcd = v4trace_LoadVCDforHNum(ctx,hnum,TRUE)) == NULL) { srcFile = NULL ; }
			    else { srcFile = (ctx->rtStack[i].vis.c.vcdIndex < vcd->fileCount ? vcd->File[ctx->rtStack[i].vis.c.vcdIndex].fileName : NULL) ; } ;
			   if (srcFile != NULL)
			    { if (vcd->File[ctx->rtStack[i].vis.c.vcdIndex].spwHash64 != 0 && vcd->File[ctx->rtStack[i].vis.c.vcdIndex].spwHash64 != gpi->spwHash64) srcFile = UClit("restricted file") ; } ;
			   v_Msg(ctx,UCTBUF2,"@%1d %2U:%3U: %4P\n",i,(srcFile == NULL ? UClit("") : srcFile),v4dbg_FormatLineNum(ctx->rtStack[i].vis.c.lineNumber),ctx->rtStack[i].isctPtr) ;
			   vout_UCText(VOUT_Trace,0,UCTBUF2) ;
			 } ;
			break ;
		   case _StepInto:
			if (!v4eval_ChkEOL(tcb)) goto fail ;
			if (respnt != NULL)		/* Only meaningful if from within debugger, otherwise drop thru to 'Step' */
			 { return(V4EVAL_Res_Flag_ShowRes | V4EVAL_Res_BPStepInto) ;
			 } ;
		   case _Step:
			if (!v4eval_ChkEOL(tcb)) goto fail ;
			if (respnt == NULL)		/* This means we are not called from breakpoint - have to fake one in */
			 { if (gpi->bpl == NULL) v4dbg_bplInit() ;
			   gpi->bpl->rtStackXStep = ctx->rtStackX ;
			 } ;
			return(V4EVAL_Res_Flag_ShowRes | V4EVAL_Res_BPStep) ;
		   case _SNC:
			if (!v4eval_ChkEOL(tcb)) goto fail ;
			if (respnt == NULL)		/* This means we are not called from breakpoint - have to fake one in */
			 { if (gpi->bpl == NULL) v4dbg_bplInit() ;
			   gpi->bpl->rtStackXStep = ctx->rtStackX ;
			 } ;
			return(V4EVAL_Res_Flag_ShowRes | V4EVAL_Res_BPStepNC) ;
		   case _Stop:
			if (!v4eval_ChkEOL(tcb)) goto fail ;
			return(V4EVAL_Res_StopXct) ;
		   case _Time:
			if (!v4eval_ChkEOL(tcb)) goto fail ;
			v_Msg(ctx,UCTBUF2,"DBGTimeCum",v_CPUTime(),v_ConnectTime()) ;
			vout_UCText(VOUT_Trace,0,UCTBUF2) ;
			break ;
		 } ;
		break ;
	      }
	      case _LogToIsct:	/* LOGTOISCT isct */
		if (CHECKV4INIT) goto fail ;
		if (!gpi->DoLogToIsct)
		 { for(;;)				/* Skip until end-of-command */
		    { v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ; if (tcb->type == V4LEX_TknType_EOL) break ; } ;
		   break ;
		 } ;
		if (tcb->appendto != NULL)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNoNest",cx) ; goto fail ; } ;
//		 v4_error(0,tcb,"V4","Eval","NESTLOG","Cannot nest LogToIsct commands") ;
		if (tcb->ifx >= V4LEX_NestedIfThenElse - 1) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIfMaxNest",tcb->type) ; goto fail ; } ;
		logisct = (P *)v4mm_AllocChunk(sizeof *logisct,FALSE) ;
		i = v4dpi_PointParse(ctx,logisct,tcb,V4DPI_PointParse_LHS|V4DPI_PointParse_RetFalse) ;	/* Parse the isct */
//{ UCCHAR vv[500] ; v_Msg(ctx,vv,"@*logisct=%1P\n",logisct) ; vout_UCText(VOUT_Trace,0,vv) ; }
		if (!i) goto fail ;
		if (logisct->PntType != V4DPI_PntType_Isct)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIsct1Arg",tcb->type,cx,V4DPI_PntType_Isct) ; goto fail ; } ;
//		 v4_error(0,tcb,"V4","Eval","NOTISCT","First argument to LogToIsct must be isct") ;
		tcb->appendto = (UCCHAR *)v4mm_AllocUC(UCsizeof(bt->BigBuf)) ;
		*tcb->appendto = UCEOS ; tcb->maxappend = UCsizeof(bt->BigBuf) ; tcb->bytesappend = 0 ;
		tcb->ifs[tcb->ifx].ifxID = ++tcb->LastifxID ;
		UCsprintf(tcb->ifs[tcb->ifx].name,UCsizeof(tcb->ifs[tcb->ifx].name),UClit("(LogToIsct line %d)"),tcb->ilvl[tcb->ilx].current_line) ;
		tcb->ifs[tcb->ifx].inif = TRUE ; tcb->ifs[tcb->ifx].doif = TRUE ; tcb->ifx ++ ;
		for(looper=TRUE;looper;)
		 { 
//		   switch (v4eval_NextKeyWord(loglist,tcb,EOLOK))
		   switch (v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
		    { default: v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,cx) ; goto fail ;
//				v4_error(0,tcb,"V4","Eval","INVLOGARG","Unknown LogToIsct keyword: %s",tcb->keyword) ;
		      case _EndOfLine_:		looper = FALSE ; break ;
		      case _SemiColon_:		looper = FALSE ; break ;
		      case _NoEval:		/* If no eval then set doif to FALSE */
			tcb->ifs[tcb->ifx-1].doif = FALSE ; break ;
		    } ;
		 } ;
		tcb->appendopt |= (V4LEX_AppendTo_LineTerm | V4LEX_AppendTo_LogToIsct) ; /* Set this to get EOLbt at end of each line */	
		break ;
	      case _EndLog:	/* ENDLOG */
		if (!v4eval_ChkEOL(tcb)) goto fail ;
		if (!gpi->DoLogToIsct) break ;
		if (tcb->ifx > 0) tcb->ifx -- ;				/* Pop off if level */
		if (tcb->appendto == NULL || logisct == NULL)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdLogIsctEnd",cx,(kwx=_LogToIsct)) ; goto fail ; } ;
//		 v4_error(0,tcb,"V4","Eval","NOLOG","EndLog may only be used to terminate a prior LogToIsct command") ;
		i = Dim_List ;
		dpt = &valpt ; INITLP(dpt,lp,i)		/* Init list to hold text lines */
//		ZPH(&misct) ; misct.Dim = Dim_Alpha ; misct.PntType = V4DPI_PntType_Char ;
		uccharPNT(&misct)
		*(tcb->appendto + UCstrlen(tcb->appendto) - 1) = UCEOS ;
		for(ucb=tcb->appendto;;)
		 { UCCHAR *mp = UCstrchr(ucb,EOLbt) ; if (mp == NULL) break ;	/* Don't do last line (ENDLOG!) */
		   *mp = UCEOS ;
		   UCstrcpy(&misct.Value.UCVal[1],ucb) ;
//		   CHARPNTBYTES1(&misct)
		   UCCHARPNTBYTES1(&misct)
//sss		   misct.Value.AlphaVal[0] = strlen(ucb) ;
//		   misct.Bytes = ALIGN(V4DPI_PointHdr_Bytes + misct.Value.AlphaVal[0] + 1) ;
		   if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&misct,0)) goto fail ;
		   ucb = mp + 1 ;
		 } ;
		v4mm_FreeChunk(tcb->appendto) ; tcb->appendto = NULL ; tcb->appendopt = 0 ;
		ENDLP(dpt,lp)
		if (!v4dpi_BindListMake(&bindpt,dpt,logisct,ctx,NULL,NOWGTADJUST,V4DPI_BindLM_WarnDupBind,DFLTRELH))
		 goto fail ;
//		 v4_error(0,tcb,"V4","Eval","BINDERR",ctx->ErrorMsgAux) ;
		v4mm_FreeChunk(logisct) ; logisct = NULL ;
		break ;
	      case _Loop:	/* LOOP */
		if (loopalloccnt < LOOPALLOCMAX)
		 { loopalloc[loopalloccnt] = (struct V4Eval__Loop *)v4mm_AllocChunk(sizeof *loop,FALSE) ; } ;
		loop = loopalloc[loopalloccnt%LOOPALLOCMAX] ; loopalloccnt++ ;
		loop->Self = loop ;
		v4lex_NextTkn(tcb,0) ;			/* Do we have n=values or #table-name# ? */
		if (tcb->opcode == V_OpCode_Pound)
		 { v4lex_NextTkn(tcb,0) ;
		   if (tcb->type != V4LEX_TknType_Keyword) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdTblName1",tcb->type) ; goto fail ; } ;
//		   for(tvlt=gpi->vlt;tvlt!=NULL;tvlt=tvlt->link)
//		    { if (strcmp(tvlt->Name,tcb->keyword) == 0) break ; } ;
		   tvlt = v4eval_GetTable(ctx,tcb->UCkeyword,NULL) ;
//		   if (tvlt == NULL) { v_Msg(NULL,tbuf,"TableNotDef1",tcb->keyword) ; v4_error(0,tcb,"V4","Eval","NOTDEFTBL",tbuf) ; } ;
		   if (tvlt == NULL) goto fail ;
		   tvlt->inclusionIndex = UNUSED ; v4lex_NextTkn(tcb,0) ;
		   if (tcb->opcode == V_OpCode_Colon)
		    { v4lex_NextTkn(tcb,0) ;
		      if (tcb->type != V4LEX_TknType_Integer || tcb->integer < 1 || tcb->integer > 32) {v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;
		      tvlt->inclusionIndex = tcb->integer ;
		      v4lex_NextTkn(tcb,0) ;
		    } else if (tcb->opcode != V_OpCode_Pound)  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdTblName1",tcb->type) ; goto fail ; } ;
		   UCstrcpy(loop->KWBuf,tvlt->Name) ; loop->LoopType = V4E_Loop_Table ; loop->vlt = tvlt ;
		   loop->Num = 0 ; loop->End = tvlt->Columns ;
		   tcb->poundparam['I'-'A'].numvalue = 0 ;		/* Set #i# to 1 */
		 } else
		 { if (tcb->type != V4LEX_TknType_Keyword || UCstrlen(tcb->UCkeyword) != 1)
		    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotAtoZ",tcb->type) ; goto fail ; } ;
//		    v4_error(0,tcb,"V4","Eval","NOTLETTER","Expecting letter from A - Z") ;
		   loop->Param = UCTOUPPER(tcb->UCkeyword[0]) - UClit('A') ;
		   if (loop->Param < 0 || loop->Param > 25)
		    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotAtoZ",tcb->type) ; goto fail ; } ;
//		    v4_error(0,tcb,"V4","Eval","NOTLETTER","Expecting letter from A - Z") ;
		   v4lex_NextTkn(tcb,0) ;
		   if (tcb->opcode != V_OpCode_Equal)
		    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotEqual",tcb->type) ; goto fail ; } ;
//		    v4_error(0,tcb,"V4","Eval","NOTEQUAL","Expecting an equal sign") ;
		   v4lex_NextTkn(tcb,0) ;
		   if (tcb->type != V4LEX_TknType_Integer)
		    {v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;
//		    v4_error(0,tcb,"V4","Eval","NOTINT","Expecting an integer value") ;
		   loop->Num = tcb->integer ;
		   v4lex_NextTkn(tcb,0) ;
		   if (tcb->opcode != V_OpCode_DotDot)
		    {v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotDotDot",tcb->type) ; goto fail ; } ;
//		    v4_error(0,tcb,"V4","Eval","NOTDOTDOT","Expecting \'..\' to indicate range of values") ;
		   v4lex_NextTkn(tcb,0) ;
		   if (tcb->type != V4LEX_TknType_Integer)
		    {v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;
//		    v4_error(0,tcb,"V4","Eval","NOTINT","Expecting an integer value") ;
		   loop->End = tcb->integer ;
		   loop->LoopType = V4E_Loop_Int ;
		 } ;
		ZUS(loop->V4Com) ;
		for(;;)
		 { v4lex_NextTkn(tcb,V4LEX_Option_ForceAsIs+V4LEX_Option_RetEOL) ;
		   if (tcb->opcode == V_OpCode_Semi) break ;
		   if (tcb->type == V4LEX_TknType_EOL) break ;
		   UCstrcat(loop->V4Com,tcb->UCstring) ;
		   if(vuc_IsWSpace(*tcb->ilvl[tcb->ilx].input_ptr))		/* Only append space if terminated with white space */
		    UCstrcat(loop->V4Com,UClit(" ")) ;
		 } ; UCstrcat(loop->V4Com,UClit(";")) ;
		UCsprintf(UCTBUF1,256,UClit("XXLoop %p\n"),loop->Self) ;
		v4lex_NestInput(tcb,NULL,UCTBUF1,V4LEX_InpMode_String) ;	/* Inject into tcb & continue */
		break ;
	      case _XXLoop:	/* XXLOOP */
		loop = NULL ; UCsscanf(tcb->ilvl[tcb->ilx].input_ptr,UClit(" %p"),&loop) ;
		if (loop == NULL ? TRUE : loop != loop->Self) break ;	/* Don't bother */
		more = FALSE ; v4lex_CloseOffLevel(tcb,0) ;		/* Pop off level with _Loop */
		switch (loop->LoopType)
		 {
		   case V4E_Loop_Int:
			if (loop->Num > loop->End)			/* End of loop- set param to 1 greater & quit */
			 { tcb->poundparam[loop->Param].numvalue = loop->Num ; break ; } ;
			tcb->poundparam[loop->Param].numvalue = loop->Num++ ;
			more = TRUE ; break ;
		   case V4E_Loop_Keywords:
		   case V4E_Loop_Table:
			for(;;loop->Num++)
			 { if (loop->Num >= loop->End)
			    { tcb->poundparam['I'-'A'].numvalue++ ; break ; } ;		/* All done */
			   if (loop->vlt->Col[loop->Num].ColumnType & (V4LEX_TableCT_Ignore|V4LEX_TableCT_AggKey|V4LEX_TableCT_Expression)) continue ;
//			   if (loop->vlt->Col[loop->Num].ColumnType & V4LEX_TableCT_AggKey) continue ;
			   if (loop->vlt->inclusionIndex != UNUSED && (loop->vlt->Col[loop->Num].inclusionMask & (1 << (loop->vlt->inclusionIndex - 1))) == 0) continue ;
			   if (loop->vlt->Col[loop->Num].Desc != NULL)
			    { v_Msg(ctx,tcb->poundparam['X'-'A'].value,"@\"%1U\"",loop->vlt->Col[loop->Num].Desc) ;
//			      strcpy(tcb->poundparam['X'-'A'].value,"\"") ;
//			      strcat(tcb->poundparam['X'-'A'].value,loop->vlt->Col[loop->Num].Desc) ;
//			      strcat(tcb->poundparam['X'-'A'].value,"\"") ;
			    } else { UCstrcpy(tcb->poundparam['X'-'A'].value,UClit("\"\"")) ; } ;
			   UCstrcpy(tcb->poundparam['C'-'A'].value,loop->vlt->Col[loop->Num].Name) ;
//			   sprintf(tcb->poundparam['Q'-'A'].value,"\"%s\"",UCretASC(loop->vlt->Col[loop->Num].Name)) ;
			   v_Msg(ctx,tcb->poundparam['Q'-'A'].value,"@\"%1U\"",loop->vlt->Col[loop->Num].Name) ;
			   UCstrcpy(tcb->poundparam['D'-'A'].value,loop->vlt->Col[loop->Num].di->DimName) ;
			   if (loop->vlt->Col[loop->Num].Missing == NULL) { UCstrcpy(tcb->poundparam['M'-'A'].value,UClit(" ")) ; }
			    else { v4dpi_PointToStringML(UCTBUF1,loop->vlt->Col[loop->Num].Missing,ctx,V4DPI_FormatOpt_Echo,V4DPI_AlphaVal_Max) ;
/*				   Note: Already did a length check when defined .Missing point in COLUMN command */
				   UCstrcpy(tcb->poundparam['M'-'A'].value,UClit("Error::")) ; UCstrcat(tcb->poundparam['M'-'A'].value,UCTBUF1) ;
				 } ;
			   if (loop->vlt->Col[loop->Num].PntType1 != UNUSED)
			    { UCstrcat(tcb->poundparam['D'-'A'].value,UClit(",")) ;
			      UCstrcat(tcb->poundparam['D'-'A'].value,loop->vlt->Col[loop->Num].di1->DimName) ;
			    } ;
			   tcb->poundparam['X'-'A'].numvalue = V4LEX_Tkn_PPNumUndef ;
			   tcb->poundparam['C'-'A'].numvalue = V4LEX_Tkn_PPNumUndef ;
			   tcb->poundparam['D'-'A'].numvalue = V4LEX_Tkn_PPNumUndef ;
			   tcb->poundparam['M'-'A'].numvalue = V4LEX_Tkn_PPNumUndef ;
			   tcb->poundparam['Q'-'A'].numvalue = V4LEX_Tkn_PPNumUndef ;
			   tcb->poundparam['I'-'A'].numvalue++ ;
			   loop->Num++ ; more = TRUE ; break ;
			 } ; break ;
		 } ;
		if (more)
		 { 
//		   sprintf(v4mbuf,"XXLoop %p\n",loop->Self) ;
//		   strcpy(ASCTBUF1,loop->V4Com) ; strcat(ASCTBUF1,v4mbuf) ;
//		   v_Msg(ctx,ucbuf,"@%1U XXLoop %2p\n",loop->V4Com,loop->Self) ;
		   UCstrcpy(UCTBUF1,loop->V4Com) ; UCsprintf(&UCTBUF1[UCstrlen(UCTBUF1)],100,UClit(" XXLoop %p\n"),loop->Self) ;
		   v4lex_NestInput(tcb,NULL,UCTBUF1,V4LEX_InpMode_String) ;	/* Inject into tcb & continue */
		 } ;
		tcb->need_input_line = TRUE ;
		break ;
	      case _EBind:	/* EBIND [intersection] value */
#ifdef WANTPATTERNMATCH
	      case _Pattern: /* PATTERN */
#endif
	      case _Bind:	/* BIND (+/- n) [intersection] value */
bind_entry:
		num = 0 ; v4lex_NextTkn(tcb,0) ;
		if (tcb->opcode == V_OpCode_LParen || tcb->opcode == V_OpCode_NCLParen)
		 { v4lex_NextTkn(tcb,0) ; i = 0 ;
		   if (tcb->opcode == V_OpCode_Plus) { i = 1 ; v4lex_NextTkn(tcb,0) ; } ;
		   if (tcb->opcode == V_OpCode_Minus) { i = -1 ; v4lex_NextTkn(tcb,0) ; } ;
		   if (tcb->type != V4LEX_TknType_Integer)
		    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;
		   switch (i)
		    { case -1:	num = -tcb->integer ; break ;
		      case 0:	num = tcb->integer + V4DPI_BindWgt_PrecOffset ; break ;
		      case 1:
			if (tcb->integer >= V4DPI_BindWgt_PrecOffset)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdBindAdj",tcb->type,V4DPI_BindWgt_PrecOffset) ; goto fail ; } ;
			num = tcb->integer ; break ;
		    } ;
		   v4lex_NextTkn(tcb,0) ;
		   if (tcb->opcode != V_OpCode_RParen)
		    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotRParen",tcb->type) ; goto fail ; } ;
		 } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
		i = v4dpi_PointParse(ctx,&isctpt,tcb,((V4DPI_PointParse_RetFalse|V4DPI_PointParse_LHS) | (cx==_Pattern ? V4DPI_PointParse_Pattern : 0))) ;
		if (!i) goto fail ;
/*		j = BindingMake flags */
		j = V4DPI_BindLM_WarnDupBind ;
#ifdef WANTPATTERNMATCH
		if (cx == 27) j |= V4DPI_BindLM_Pattern ;
#endif
		v4lex_NextTkn(tcb,0) ;
		switch (tcb->opcode)
		 { default:			v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; break ;
		   case V_OpCode_Equal:		break ;
		   case V_OpCode_LangleDash:
		   case V_OpCode_LangleEqual:	j &= ~V4DPI_BindLM_WarnDupBind ; break ; /* Turn off dup-warning */
		 } ;
		i = v4dpi_PointParse(ctx,&valpt,tcb,V4DPI_PointParse_MultiLine+V4DPI_PointParse_RetFalse) ;
		if (!i) goto fail ;
		if (!v4eval_ChkEOL(tcb))
		 goto fail ;
		if (cx == _EBind ? (valpt.PntType == V4DPI_PntType_Isct) : FALSE)
		 { dpt = v4dpi_IsctEval(&point,&valpt,ctx,0,NULL,NULL) ;
		   if (dpt == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIsctFail",&valpt) ; goto fail_full ; } ;
		 } else { dpt = &valpt ; } ;
		if (!v4dpi_BindListMake(&bindpt,dpt,&isctpt,ctx,&areaidused,num,j,DFLTRELH))
		 goto fail ;
		break ;
	      case _C:		/* CONTEXT ADD point */
		v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;	/* Grab next token */
		if (tcb->type == V4LEX_TknType_EOL)		/* End of line - do 'Context Examine' */
		 { v4ctx_ExamineCtx(ctx,FALSE,VOUT_Trace) ; break ;
		 } ;
		v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;	/* Push token back on & handle as 'Context Add xxx' */
		goto context_add ;
	      case _Context:	/* CONTEXT what */
		switch(v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
		 { default:	v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,cx) ; goto fail ;
		   case _Add:
context_add:		if (!v4dpi_PointParse(ctx,&isctpt,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
			if (!v4eval_ChkEOL(tcb)) goto fail ;
			dpt = v4dpi_IsctEval(&point,&isctpt,ctx,0,NULL,NULL) ;
			if (dpt == NULL)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIsctFail",&isctpt) ; goto fail_full ; } ;
			if (!v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,dpt,0,0)) goto fail ; break ;
		   case _EndOfLine_:
		   case _Examine:
			if (!v4eval_ChkEOL(tcb)) goto fail ;
			v4ctx_ExamineCtx(ctx,FALSE,VOUT_Trace) ; break ;
		   case _Pop:
			v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;		/* Grab next token */
			if (tcb->type == V4LEX_TknType_EOL)
			 { v4ctx_FramePop(ctx,0,NULL) ;
			 } else
			 { if (tcb->type == V4LEX_TknType_Keyword)
			    { if (!v4ctx_FramePop(ctx,UNUSED,tcb->UCkeyword)) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdCtxPopName",tcb->UCkeyword) ; goto fail ; } ;
			      if (!v4eval_ChkEOL(tcb)) goto fail ;
			    } else
			    { if (tcb->type != V4LEX_TknType_Integer)
			       { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;
			      if (!v4ctx_FramePop(ctx,tcb->integer,NULL)) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdCtxPopId",tcb->integer) ; goto fail ; } ;
			      if (!v4eval_ChkEOL(tcb)) goto fail ;
			    } ;
			 } ;
			break ;
		   case _PopAny:	/* CONTEXT POPANY list */
			for(;;)
			 { v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;		/* Grab next token */
			   if (tcb->type == V4LEX_TknType_Keyword)
			    { v4ctx_FramePop(ctx,UNUSED,tcb->UCkeyword) ;
			    } else
			    { if (tcb->type != V4LEX_TknType_Integer) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;
			      v4ctx_FramePop(ctx,tcb->integer,NULL) ;
			    } ;
			   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
			   if (tcb->type == V4LEX_TknType_EOL || tcb->opcode == V_OpCode_Semi) break ;
			   if (tcb->opcode != V_OpCode_Comma) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotEOL",tcb->type) ; goto fail ; } ;
			 } ;
			break ;
		   case _Push:	/* CONTEXT PUSH name */
		      { UCCHAR tname[V4LEX_Tkn_KeywordMax+1] ;
			v4lex_NextTkn(tcb,0) ; UCstrcpy(tname,tcb->UCkeyword) ;
			if (!v4eval_ChkEOL(tcb)) goto fail ;
			if (trace & V4TRACE_Context)
			 { v_Msg(ctx,UCTBUF1,"@*Ctx Frame: %1U = %2d\n",tname,v4ctx_FramePush(ctx,tname)) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
			 } else { v4ctx_FramePush(ctx,tname) ; } ;
			break ;
		      } ;
		   case _ADV:	/* CONTEXT ADV dim value - Adds point specified by dimension & value on remainder of line */
		      { UCCHAR *advBuf ; INDEX ilxSave = tcb->ilx ;
			v4lex_NextTkn(tcb,0) ;
			dix = v4dpi_DimInfoGet(ctx,v4dpi_DimGet(ctx,tcb->UCkeyword,DIMREF_LEX)) ;
			if (dix == NULL)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotDim",tcb->UCkeyword) ; goto adv_fail ; } ;
			if (*tcb->ilvl[tcb->ilx].input_ptr > 26)	/* Skip over next space (or colon!) */
			 tcb->ilvl[tcb->ilx].input_ptr ++ ;
			for(i=UCstrlen(tcb->ilvl[tcb->ilx].input_ptr)-1;;i--)
			 { if (tcb->ilvl[tcb->ilx].input_ptr[i] >= 26) break ; } ;
			tcb->ilvl[tcb->ilx].input_ptr[i+1] = UCEOS ;
			v4lex_NestInput(tcb,NULL,NULL,V4LEX_InpMode_Stage) ; advBuf = tcb->ilvl[tcb->ilx+1].input_str ;
			switch (dix->PointType)
			 { default:
				if (UCempty(tcb->ilvl[tcb->ilx].input_ptr))
				 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdADVNoVal",dix->DimId) ; goto adv_fail ; } ;
				v_Msg(ctx,advBuf,"@%1U:%2U",tcb->UCkeyword,tcb->ilvl[tcb->ilx].input_ptr) ;
				break ;
			   case V4DPI_PntType_Int:		/* If no value then convert to dim:none */
			   case V4DPI_PntType_CodedRange:
			   case V4DPI_PntType_Fixed:
			   case V4DPI_PntType_Real:
				if (UCempty(tcb->ilvl[tcb->ilx].input_ptr))
				 { dpt = &valpt ; ZPH(dpt) ; dpt->Dim = dix->DimId ; dpt->PntType = V4DPI_PntType_Special ;
				   dpt->Bytes = V4PS_SpecNone ; dpt->Grouping = V4DPI_Grouping_None ;
				   if (!v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,dpt,0,0)) goto adv_fail ;
				   continue ;
				 } ;
				if ((dix->Flags & V4DPI_DimInfo_ListOK) == 0)	/* If multiple values not allowed then get rid of any commas */
				 { struct V4CI__CountryInfo *ci = gpi->ci ;
				   if (ci->Cntry[ci->CurX].DigiDelim == UClit(','))	/* Only get rid of it if it is thousands delimiter */
				    { UCCHAR *b1,*b2 ; int sig ;
				      for(sig=FALSE,b1=tcb->ilvl[tcb->ilx].input_ptr,b2=tcb->ilvl[tcb->ilx].input_ptr;;b1++)
				       { if (!sig)
				          { if (*b1 == UClit(' ')) continue ;		/* Skip leading spaces */
					    if (*b1 == ci->Cntry[ci->CurX].CurrencySign[0]) continue ;		/* Skip leading currency indicator */
					  } ;
				         sig = TRUE ; *b2 = *b1 ; if ((ci->Cntry[ci->CurX].DigiDelim != UClit(',')) || (*b2 != UClit(','))) b2++ ;
				         if (*b2 == UCEOS) break ;
				       } ;
				    } ;
				 } ;
				v_Msg(ctx,advBuf,"@%1U:%2U",tcb->UCkeyword,tcb->ilvl[tcb->ilx].input_ptr) ;
				break ;
			   case V4DPI_PntType_Dict:
			   case V4DPI_PntType_XDict:
			   	if (UCempty(tcb->ilvl[tcb->ilx].input_ptr))
			   	 { v_Msg(ctx,advBuf,"@%1U:%2U",tcb->UCkeyword,UClit("#0")) ; break ; }
			   	if (tcb->ilvl[tcb->ilx].input_ptr[0] == UClit('#'))
			   	 { v_Msg(ctx,advBuf,"@%1U:%2U",tcb->UCkeyword,tcb->ilvl[tcb->ilx].input_ptr) ; break ; }
				if (UCstrcmp(tcb->ilvl[tcb->ilx].input_ptr,UClit("..")) == 0)
				 { v_Msg(ctx,advBuf,"@%1U:..",tcb->UCkeyword) ;
				   break ;	/* Don't want to fall thru & enclose ".." in quotes */
				 } ;
				UCstrcpy(advBuf,tcb->UCkeyword) ; UCstrcat(advBuf,UClit(":\"")) ;
				{ UCCHAR *s, *d ;
				  s = tcb->ilvl[tcb->ilx].input_ptr ; d = &advBuf[UCstrlen(advBuf)] ;
				  for(;;s++)
				   { if (*s == UClit(',')) { *(d++) = UClit('"') ; *(d++) = UClit(',') ; *(d++) = UClit('"') ; continue ; } ;
				     if (*s == UClit('"')) { *(d++) = UClit('\\') ; *(d++) = UClit('"') ; continue ; } ;
				     if (*s == UCEOS) break ;
				     *(d++) = *s ;
				   } ;
				  *(d++) = UClit('"') ; *(d++) = UCEOS ;
				}
				break ;
			   case V4DPI_PntType_Logical:
			   	if (UCempty(tcb->ilvl[tcb->ilx].input_ptr))
				 { v_Msg(ctx,advBuf,"@%1U:%2U",tcb->UCkeyword,UClit("0")) ; }
				 else { v_Msg(ctx,advBuf,"@%1U:%2U",tcb->UCkeyword,tcb->ilvl[tcb->ilx].input_ptr) ; } ;
				break ;
			   case V4DPI_PntType_Color:
			   case V4DPI_PntType_Country:
			   case V4DPI_PntType_Calendar:
			   case V4DPI_PntType_UPeriod:
			   case V4DPI_PntType_UQuarter:
			   case V4DPI_PntType_UYear:
			   case V4DPI_PntType_UMonth:
			   case V4DPI_PntType_UDate:
			   case V4DPI_PntType_UDT:
			   case V4DPI_PntType_UWeek:
			   case V4DPI_PntType_UTime:
			   case V4DPI_PntType_TeleNum:
//			   	if (UCstrlen(tcb->ilvl[tcb->ilx].input_ptr) == 0)
			   	if (UCempty(tcb->ilvl[tcb->ilx].input_ptr))
				 { v_Msg(ctx,advBuf,"@%1U:%2U",tcb->UCkeyword,UClit("none")) ; }
				 else { v_Msg(ctx,advBuf,"@%1U:%2U",tcb->UCkeyword,tcb->ilvl[tcb->ilx].input_ptr) ; } ;
				break ;
			   case V4DPI_PntType_UOM:
			      { UCCHAR tuom[256] ;
				for(i=0,j=0;j<sizeof tuom;i++)
				 { if (tcb->ilvl[tcb->ilx].input_ptr[i] == UClit(' ')) continue ;
				   tuom[j++] = tcb->ilvl[tcb->ilx].input_ptr[i] ;
				   if (tcb->ilvl[tcb->ilx].input_ptr[i] == UCEOS) break ;
				 } ;
				v_Msg(ctx,advBuf,"@%1U:%2U",tcb->UCkeyword,tuom) ;
				break ;
			      }
			   case V4DPI_PntType_BigText:
				if (dix->ds.Alpha.aFlags & V4DPI_DimInfoAlpha_ParseJSON && UCnotempty(tcb->ilvl[tcb->ilx].input_ptr))	/* If flag set AND there is more input then parse as JSON expression */
				 { DICTID dictId = vjson_ParseString(ctx,tcb,dix->DimName,0) ;
				   if (dictId == 0) { v_Msg(ctx,NULL,"ParseJSON",V4IM_OpCode_Context) ; goto adv_fail ; } ;
				   ZPH(&point) ; point.PntType = V4DPI_PntType_ParsedJSON ; point.Bytes = gpi->PointBytes[V4DPI_PntType_ParsedJSON] ; point.Dim = dix->DimId ;
				   if (!v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,&point,0,0)) goto adv_fail ;
				   goto context_done ;
				 } ;
adv_bigtext:			if (bt != NULL) { memset(bt,0,sizeof *bt) ; }
				 else { bt = (struct V4LEX__BigText *)v4mm_AllocChunk(sizeof *bt,FALSE) ; } ;
				num = UCstrlen(tcb->ilvl[tcb->ilx].input_ptr) ;
				if (num >= V4LEX_BigText_Max)
				 { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsExpLitMax",num,V4LEX_BigText_Max) ; goto adv_fail ; } ;
//				UCstrcpy(bt->BigBuf,tcb->ilvl[tcb->ilx].input_ptr) ;
				{ UCCHAR *dst=bt->BigBuf, *src=tcb->ilvl[tcb->ilx].input_ptr ;
				  for(;*src!=UCEOS;src++)
				   { switch (*src)
				      { default:		*(dst++) = *src ; break ;
					case UClit('<'):	*(dst++) = *src ;
								if ((dix->ds.Alpha.aFlags & (V4DPI_DimInfoAlpha_XMLOK)) != 0) break ;
								dst-- ; *(dst++) = UClit('&') ; *(dst++) = UClit('l') ; *(dst++) = UClit('t') ; *(dst++) = UClit(';') ;
								num += 3 ;
								break ;
//					case UClit('&'):	*(dst++) = *src ;
//								if ((dix->ds.Alpha.aFlags & V4DPI_DimInfoAlpha_XMLOK) != 0) break ;
//								dst-- ; *(dst++) = UClit('&') ; *(dst++) = UClit('a') ; *(dst++) = UClit('m') ; *(dst++) = UClit('p') ; *(dst++) = UClit(';') ;
//								break ;
					case UClit('\\'):
				          if ((dix->ds.Alpha.aFlags & V4DPI_DimInfoAlpha_AsIs) != 0)
					   { *(dst++) = *src ; break ; } ;
					  switch (*(++src))
				           { default:		*(dst++) = '\\' ; *(dst++) = *src ; break ;
				             case UClit('t'):	*(dst++) = UClit('\t') ; break ;
				             case UClit('r'):	*(dst++) = UClit('\r') ; break ;
				             case UClit('l'):	*(dst++) = 10 ; break ;
				             case UClit('n'):	*(dst++) = UCNL ; break ;
				             case UClit('"'):	*(dst++) = UClit('"') ; break ;
				             case UClit('\\'):	*(dst++) = UClit('\\') ; break ;
				           } ; break ;
				      } ;
				   } ; *dst = UCEOS ;
				}
				if (!v4dpi_SaveBigTextPoint(ctx,bt,num,&point,dix->DimId,TRUE)) goto adv_fail ;
				if (!v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,&point,0,0)) goto adv_fail ;
				tcb->need_input_line = TRUE ;			/* Force to new line */
				goto context_done ;
			   CASEofCharMBT
/*				If XML not set then have to check for any '<' and adjust if necessary */
				if ((dix->ds.Alpha.aFlags & V4DPI_DimInfoAlpha_XMLOK) == 0)
				 { UCCHAR *s = tcb->ilvl[tcb->ilx].input_ptr, *d = UCTBUF1 ;
				   for(;;s++)
				    { *(d++) = *s ;
				      if (*s == UCEOS) break ;
				      if (*s == UClit('<'))
				       { d-- ; *(d++) = UClit('&') ; *(d++) = UClit('l') ; *(d++) = UClit('t') ; *(d++) = UClit(';') ; } ;
//				       } else if (*s == UClit('&')) { d-- ; *(d++) = UClit('&') ; *(d++) = UClit('a') ; *(d++) = UClit('m') ; *(d++) = UClit('p') ; *(d++) = UClit(';') ; } ;
				    } ;
				   tcb->ilvl[tcb->ilx].input_ptr = UCTBUF1 ;
				 } ;
				if (dix->ds.Alpha.aFlags & V4DPI_DimInfoAlpha_IsFileName)	/* Take file as-is */
/*				    If embedded spaces & not already in quotes then enclose in quotes */
				 { if (UCstrchr(tcb->ilvl[tcb->ilx].input_ptr,' ') != NULL && *tcb->ilvl[tcb->ilx].input_ptr != UClit('"'))
				    { v_Msg(ctx,advBuf,"@%1U:\"%2U\"",tcb->UCkeyword,tcb->ilvl[tcb->ilx].input_ptr) ; }
				    else { v_Msg(ctx,advBuf,"@%1U:%2U",tcb->UCkeyword,tcb->ilvl[tcb->ilx].input_ptr) ; } ;
				 } else
				 { UCCHAR *p=tcb->ilvl[tcb->ilx].input_ptr ;
				   int i=0,j ;
				   v_Msg(ctx,advBuf,"@%1U:\"",tcb->UCkeyword) ; j=UCstrlen(advBuf) ;
				   if (*p == UClit('"'))
				    { UCstrcat(advBuf,p+1) ;	/* If already quoted then take as-is */
				    } else
				    { for(;p[i]!=UCEOS;i++)
				       { if (p[i] == UClit('"')) { advBuf[j++] = UClit('\\') ; } ;
				         advBuf[j++] = p[i] ;
				       } ; advBuf[j++] = '"' ; advBuf[j] = UCEOS ;
				    } ;
				 } ;
				if (UCstrlen(advBuf) >= (dix->PointType == V4DPI_PntType_UCChar ? V4DPI_UCVal_Max-1 : V4DPI_AlphaVal_Max-1))
				 goto adv_bigtext ;
				break ;
			   case V4DPI_PntType_List:
				if (*tcb->ilvl[tcb->ilx].input_ptr == UClit('('))
				 { v_Msg(ctx,advBuf,"@%1U:%2U",tcb->UCkeyword,tcb->ilvl[tcb->ilx].input_ptr) ; }
				 else { v_Msg(ctx,advBuf,"@%1U:(%2U)",tcb->UCkeyword,tcb->ilvl[tcb->ilx].input_ptr) ; } ;
				break ;
			 } ;

//			ttcb = v4mm_AllocChunk(sizeof *ttcb,FALSE) ; v4lex_InitTCB(ttcb,0) ; v4lex_NestInput(ttcb,NULL,ucbuf,V4LEX_InpMode_String) ;
//			tcb->need_input_line = TRUE ;			/* Force to new line */
//			if (!v4dpi_PointParse(ctx,&isctpt,ttcb,V4DPI_PointParse_RetFalse))
//			 { v4lex_FreeTCB(ttcb) ; v_Msg(ctx,UCTBUF1,"CmdCtxADV") ; if (UCempty(gpi->CtxADVEchoE)) UCstrcpy(ctx->ErrorMsgAux,UCTBUF1) ; goto adv_fail ; } ;
//			v4lex_FreeTCB(ttcb) ;

			v4lex_NestInput(tcb,NULL,NULL,V4LEX_InpMode_Commit) ; tcb->need_input_line = TRUE ;
			if (!v4dpi_PointParse(ctx,&isctpt,tcb,V4DPI_PointParse_RetFalse))
			 { v_Msg(ctx,UCTBUF1,"CmdCtxADV") ; if (UCempty(gpi->CtxADVEchoE)) UCstrcpy(ctx->ErrorMsgAux,UCTBUF1) ; goto adv_fail ; } ;

			dpt = v4dpi_IsctEval(&point,&isctpt,ctx,0,NULL,NULL) ;
			if (dpt == NULL)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIsctFail",&isctpt) ; goto adv_fail ; } ;
			if (!v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,dpt,0,0)) goto adv_fail ;
			tcb->ilx = ilxSave ;
			break ;

adv_fail:		/* Here on Context ADV fail - may generate EchoE for better WWW error handling */
			tcb->ilx = ilxSave ;
			if (UCnotempty(gpi->CtxADVEchoE))
			 { int filex ;
			   if ((i=vout_OpenStreamFile(NULL,gpi->CtxADVEchoE,NULL,NULL,FALSE,V4L_TextFileType_ASCII,0,ctx->ErrorMsgAux)) == UNUSED) goto fail ;
			   vout_BindStreamFile(i,VOUT_FileType_File,VOUT_Data,ctx->ErrorMsgAux) ;
			   filex=vout_StreamToFileX(VOUT_Data) ;
/*			   Generate EchoE type error UNLESS we are doing Ajax, if so then use the given pattern */
			   if (UCnotempty(gpi->patternAJAXErr))
			    { v_StringLit(ctx->ErrorMsgAux,UCTBUF2,V4TMBufMax,'"','\\') ;
#ifdef USEOLDAJAXSUB	
			      v_Msg(ctx,UCTBUF1,gpi->patternAJAXErr,UClit("error"),UCTBUF2) ; vout_UCTextFileX(filex,0,UCTBUF1) ;
#else
			      vjson_SubParamsInJSON(UCTBUF1,V4TMBufMax,gpi->patternAJAXErr,UCTBUF2,UClit("error")) ;
#endif

			    } else
			    { 
/*			      Dump out error message and linking URL (if we have it) */
			      v_Msg(ctx,UCTBUF1,"@BSError\nMQ%1U\nTYError\n",ctx->ErrorMsgAux) ; vout_UCTextFileX(filex,0,UCTBUF1) ;
			      if (UCnotempty(gpi->CtxADVURLonError))
			       { v_Msg(ctx,UCTBUF1,"@UR%1U\n",gpi->CtxADVURLonError) ; vout_UCTextFileX(filex,0,UCTBUF1) ; } ;
			    } ;
			   return(V4EVAL_Res_ExitStats) ;
			 } ;
			goto fail ;
		      }
		 } ;
context_done:
		break ;
	      case _Dimension:	/* DIMENSION name attributes */
	      case _Dim:
		memset(&di,0,sizeof di) ; di.DictType = V4DPI_DictType_Ext ;
		ZUS(argbuf) ;
		for(;;)
		 { v4lex_NextTkn(tcb,0) ;
		   if (UCstrlen(tcb->UCkeyword) >= UCsizeof(di.DimName))
		    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdDimName",tcb->UCkeyword,UCsizeof(di.DimName)-1) ; goto fail ; } ;
//		    v4_error(0,tcb,"V4","Eval","INVDIMNAME","Dimension name (%s) cannot exceed %d characters",tcb->keyword,sizeof(di.DimName)-1) ;
		   UCstrcpy(di.DimName,tcb->UCstring) ;	/* Get upper/lower case name */
		   v4lex_NextTkn(tcb,0) ;
		   if (tcb->opcode == V_OpCode_Star)	/* Got name* - see if defined in current context */
		    { if (gpi->vltCur != NULL)		/* If processing table then see if got a column spec */
		       { 
		         for(i=0;i<gpi->vltCur->Columns;i++) { if (UCstrcmpIC(gpi->vltCur->Col[i].Name,di.DimName) == 0) break ; } ;
			 if (i < gpi->vltCur->Columns)			/* Found it - copy current value into point */
			  { if (gpi->vltCur->Col[i].Cur == NULL || (gpi->vltCur->InXML ? (gpi->vltCur->Col[i].XMLFlags & V4LEX_Table_XML_Defined)==0 : FALSE))
			     { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColUndef",di.DimName) ; goto fail ;
//			       v4_error(0,tcb,"V4DPI","PointParse","COLUNDEF","Column (#%d %s) is not currently defined",i+1,di.DimName) ;
			     } ;
			     v4dpi_PointToString(UCTBUF1,gpi->vltCur->Col[i].Cur,ctx,V4DPI_FormatOpt_Echo) ;
			    if (UCstrlen(UCTBUF1) >= UCsizeof(di.DimName))
			     { v_Msg(ctx,ctx->ErrorMsgAux,"CmdDimName",UCTBUF1,UCsizeof(di.DimName)-1) ; goto fail ; } ;
//			     v4_error(0,tcb,"V4","Eval","INVDIMNAME","Dimension name (%s) cannot exceed %d characters",ASCTBUF1,sizeof(di.DimName)-1) ;
			    UCstrcpy(di.DimName,UCTBUF1) ;
			  } else
			  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColUndef",di.DimName) ; goto fail ;
//			    v4_error(0,tcb,"V4DPI","PointParse","COLUNDEF","Column (%s) is not currently defined",di.DimName) ;
			  } ;
		       } ;
		     v4lex_NextTkn(tcb,0) ;
		    } ;
		   UCstrcat(argbuf,di.DimName) ;
		   if (tcb->opcode == V_OpCode_Comma) { UCstrcat(argbuf,UClit(",")) ; continue ; } ;
		   v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; break ;
		 } ;

		v4lex_NextTkn(tcb,0) ;
		if (tcb->type != V4LEX_TknType_Keyword) { di.PointType = UNUSED ; }
		 else { di.PointType = v4im_PTId(tcb->UCkeyword) ; } ;
		if (di.PointType == UNUSED)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotPntType",tcb->type) ; goto fail ; } ;

//		 v4_error(V4E_INVDIMTYPE,tcb,"V4","Eval","INVDIMTYPE","Invalid DIMENSION TYPE argument: %s",tcb->keyword) ;
//if (di.PointType == V4DPI_PntType_WormHole)
// { v_Msg(ctx,UCTBUF1,"@!V4W- *** %0F: WORMHOLES are no longer supported - use Project() ***\n") ; vout_UCText(VOUT_Warn,0,UCTBUF1) ; } ;
		if (di.PointType == 9999)
		 { if (UCstrchr(argbuf,',') != NULL)
		    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdDimListLocal") ; goto fail ; } ;
//		    v4_error(0,tcb,"V4","Eval","INVMULTNAME","Cannot use list of dimension names with LOCAL") ;
		   diminfo = v4dpi_DimInfoGet(ctx,v4dpi_DimGet(ctx,di.DimName,DIMREF_DCL)) ;
		   if (diminfo == NULL)
		    { v_Msg(ctx,ctx->ErrorMsgAux,"TagNotDim2",di.DimName) ; goto fail ; } ;
//		    v4_error(0,tcb,"V4","Eval","INVMULTNAME","Not a dimension: %s",di.DimName) ;
		 } else { diminfo = &di ; } ;

		v4dpi_DimInitOnType(ctx,diminfo) ;	/* Init based on point type */
		
		for(looper=TRUE;looper;)
		 { 
		   switch (v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
//		   switch (v4eval_NextKeyWord(dimlist,tcb,EOLOK))
		    { default: v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,cx) ; goto fail ;
//				v4_error(V4E_INVDIMATTR,tcb,"V4","Eval","INVDIMATTR","Unknown DIMENSION attribute: %s",tcb->keyword) ;
		      case _EndOfLine_:
			looper = FALSE ; break ;
		      case _SemiColon_:
			if (cnt == 1) cnt++ ; looper = FALSE ; break ;
		      case _Acceptor:	/* ACCEPTOR module */
			diminfo->Flags |= V4DPI_DimInfo_Acceptor ; break ;
		      case _ADPoint:	/* ADPOINT point */
			 if (!v4dpi_PointParse(ctx,&point,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
			 if (point.Bytes > sizeof diminfo->ADPnt)
			  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntTooBig",(kwx=_ADPoint),&point) ; goto fail ; } ;
//			  v4_error(0,0,"V4EVAL","Dim","POINTTOOBIG","ADPoint is too big - limit to Int/Dict/(small alpha)") ;
			 v4dpi_PointToString(UCTBUF1,&point,ctx,V4DPI_FormatOpt_ShowDim) ;
			 if (UCstrlen(UCTBUF1) > UCsizeof(diminfo->ADPntStr)-1)
			  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntTooBig",(kwx=_ADPoint),&point) ; goto fail ; } ;
//			  v4_error(0,0,"V4EVAL","Dim","POINTTOOBIG","ADPoint is too big - limit to Int/Dict/(small alpha)") ;
			 UCstrcpy(diminfo->ADPntStr,UCTBUF1) ;
			 memcpy(&diminfo->ADPnt,&point,point.Bytes) ; break ;
		      case _All:
		      case _AllValue:	/* ALLVALUE n */
			v4lex_NextTkn(tcb,0) ;
			if (tcb->opcode == V_OpCode_Minus || tcb->opcode == V_OpCode_UMinus)
			 { v4lex_NextTkn(tcb,0) ; diminfo->AllValue = -tcb->integer ; } else { diminfo->AllValue = tcb->integer ; } ;
			break ;
		      case _AsIs:
			if (diminfo->PointType != V4DPI_PntType_BigText)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdArgNotAllow",(kwx=_Attributes),V4DPI_PntType_BigText) ; goto fail ; } ;
			diminfo->ds.Alpha.aFlags |= V4DPI_DimInfoAlpha_AsIs ; break ;
		      case _Attributes:	/* ATTRIBUTES xxx */
			if (diminfo->PointType != V4DPI_PntType_BigText)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdArgNotAllow",(kwx=_Attributes),V4DPI_PntType_BigText) ; goto fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","INVDIM","ATTRIBUTE argument only allowed with BIGTEXT point types") ;
			v4lex_NextTkn(tcb,0) ;
			switch(tcb->type)
			 { default: 
				v_Msg(ctx,ctx->ErrorMsgAux,"CmdKWStr",tcb->type) ; goto fail ;
//				v4_error(0,tcb,"V4","Eval","INVOUTFORMAT","Expecting an ATTRIBUTES string") ;
			   case V4LEX_TknType_Keyword:	UCstrcpy(tcb->UCstring,tcb->UCkeyword) ;
			   case V4LEX_TknType_String:	break ;
			 } ;
			if (UCstrlen(tcb->UCstring) >= UCsizeof(diminfo->Attributes))
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdAttrTooLong",(enum DictionaryEntries)_Attributes,UCstrlen(tcb->UCstring),UCsizeof(diminfo->Attributes)) ; goto fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","TOOLONG","ATTRIBUTE argument too long") ;
			UCstrcpy(diminfo->Attributes,tcb->UCstring) ; break ;
////		      case _AutoContext:	/* AUTOCONTEXT point */
////			 if (!v4dpi_PointParse(ctx,&point,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
////			 if (point.Bytes > sizeof diminfo->AutoCtxPnt)
////			  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntTooBig",(kwx=_AutoContext),&point) ; goto fail ; } ;
////			 memcpy(&diminfo->AutoCtxPnt,&point,point.Bytes) ; break ;
////		      case _AutoIsct:	/* AUTOISCT */
////			diminfo->Flags |= V4DPI_DimInfo_AutoIsct ; break ;
		      case _BaseDate:
			if (!v4dpi_PointParse(ctx,&pisct,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
			isct = v4dpi_IsctEval(&valpt,&pisct,ctx,0,NULL,NULL) ;
			if (isct == NULL ? TRUE : isct->PntType != V4DPI_PntType_UDate) { v_Msg(ctx,NULL,"CmdArgNotAllow",DE(BaseDate),V4DPI_PntType_UDate) ; goto fail ; } ;
			diminfo->ds.UWeek.baseUDate = isct->Value.IntVal ;
			break ;
		      case _BindEval:	/* BINDEVAL */
			diminfo->Flags |= V4DPI_DimInfo_BindEval ; break ;
		      case _Bind:
		      case _Binding:	/* BINDING num */
			v4lex_NextTkn(tcb,0) ; diminfo->BindList = tcb->integer ; break ;
		      case _Calendar:	/* Calendar xxx */
			if (diminfo->PointType != V4DPI_PntType_Calendar)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdArgNotAllow",(kwx=_Calendar),V4DPI_PntType_Calendar) ; goto fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","INVDIM","CALENDAR argument only allowed with CALENDAR point types") ;
//			diminfo->ds.Cal.CalendarType = v4eval_NextKeyWord(callist,tcb,NOEOL) ;
			switch (v4im_GetTCBtoEnumVal(ctx,tcb,NOEOL))
			 { default: ok = FALSE ; v_Msg(ctx,ctx->ErrorMsgAux,"CalInvTkn",tcb->type) ; break ;
			   case _Gregorian:	ok = VCAL_CalType_Gregorian ; break ;
			   case _Julian:	ok = VCAL_CalType_Julian ; break ;
			   case _Islamic:	ok = VCAL_CalType_Islamic ; break ;
			   case _ISO:		ok = VCAL_CalType_ISO ; break ;
			   case _Hebrew:	ok = VCAL_CalType_Hebrew ; break ;
			   case _Chinese:	ok = VCAL_CalType_Chinese ; break ;
			   case _Hindu:		ok = VCAL_CalType_Hindu ; break ;
			 } ; if (ok == FALSE) break ;
			diminfo->ds.Cal.CalendarType = ok ;
			break ;
		      case _NoCreate:
		      case _NoAutoCreate:	/* NOAUTOCREATE */
			diminfo->Flags |= V4DPI_DimInfo_NoAutoCreate ; break ;
		      case _NoPrefix:
			diminfo->Flags |= V4DPI_DimInfo_NoNamePrefix ; break ;
		      case _Decimal:
		      case _Decimals:	/* DECIMALS n */
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Integer)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;
//			 v4_error(V4E_NOTINTEGER,tcb,"V4","Eval","NOTINTEGER","Expecting integer value, not: %s",tcb->keyword) ;
			if (tcb->integer < 0 || tcb->integer > V4DPI_Fixed_MaxDecimals)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIntValue",tcb->type,(kwx=_Decimals),1,V4DPI_Fixed_MaxDecimals) ; goto fail ; } ;
//			 v4_error(V4E_NOTINTEGER,tcb,"V4","Eval","NOTINTEGER","Decimals must be 1-%d, not: %d",V4DPI_Fixed_MaxDecimals,tcb->integer) ;
			diminfo->Decimals = tcb->integer ;
			if (diminfo->OutFormat[0] == UCEOS)		/* No format specified (yet) ? */
			 UCsprintf(diminfo->OutFormat,UCsizeof(diminfo->OutFormat),UClit("%%#.%df"),diminfo->Decimals) ;
			break ;
		      case _Desc:
		      case _Description:	/* DESCRIPTION xxx */
			v4lex_NextTkn(tcb,0) ;
			if (UCstrlen(tcb->UCstring) >= (UCsizeof(diminfo->Desc)))
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdDimDesc",tcb->type,UCstrlen(tcb->UCstring),UCsizeof(diminfo->Desc)) ; goto fail ; } ;
			UCstrcpy(diminfo->Desc,tcb->UCstring) ;
			break ;
		      case _Displayer:	/* DISPLAYER */
			diminfo->Flags |= V4DPI_DimInfo_Displayer ; break ;
		      case _DisplayerTrace:	/* DISPLAYERCTX */
			diminfo->Flags |= V4DPI_DimInfo_DisplayerTrace ; break ;
		      case _DotDotToList:	/* DotDotToList */
			diminfo->Flags |= V4DPI_DimInfo_DotDotToList ; break ;
		      case _DotIndex:	/* DOTINDEX */
			diminfo->Flags |= V4DPI_DimInfo_DotIndex ; break ;
		      case _DotList:	/* DOTLIST */
			diminfo->Flags |= V4DPI_DimInfo_DotList ; break ;
		      case _NoDuplicate:
		      case _Set:	/* SET */		diminfo->Flags |= V4DPI_DimInfo_IsSet ; break ;
		      case _Entries:	/* ENTRIES dimid */
			v4lex_NextTkn(tcb,0) ;
			if (UCstrlen(tcb->UCkeyword) >= UCsizeof(di.DimName))
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdDimName",tcb->UCkeyword,UCsizeof(di.DimName)-1) ; goto fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","INVDIMNAME","Dimension name (%s) cannot exceed %d characters",tcb->keyword,sizeof(di.DimName)-1) ;
			di.ListDimId = v4dpi_DimGet(ctx,tcb->UCkeyword,DIMREF_REF) ;
			if (di.ListDimId == 0)
			  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotDim",tcb->UCkeyword) ; goto fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","INVDIMNAME","Dimension (%s) does not exist",tcb->keyword) ;
			break ;
		      case _FileName:	/* FILENAME */
			diminfo->ds.Alpha.aFlags |= V4DPI_DimInfoAlpha_IsFileName ; break ;
		      case _JSON:	/* JSON */
			diminfo->ds.Alpha.aFlags |= V4DPI_DimInfoAlpha_ParseJSON ; break ;
		      case _XML:	/* XML */
			diminfo->ds.Alpha.aFlags |= V4DPI_DimInfoAlpha_XMLOK ; break ;
		      case _Hex:
		      case _Hexadecimal:
			diminfo->ds.Int.IFormat = V4LEX_TablePT_Hexadecimal ; break ;
		      case _Format:
		      case _History:
			switch (diminfo->PointType)
			 { default:			v_Msg(ctx,ctx->ErrorMsgAux,"CmdArgNotAllow",DE(History),V4DPI_PntType_Calendar) ; goto fail ;
			   case V4DPI_PntType_Calendar:	diminfo->ds.Cal.calFlags |= VCAL_Flags_Historical ; break ;
			   case V4DPI_PntType_UDate:	diminfo->ds.UDate.calFlags |= VCAL_Flags_Historical ; break ;
			   case V4DPI_PntType_UDT:	diminfo->ds.UDT.calFlags |= VCAL_Flags_Historical ; break ;
			   case V4DPI_PntType_UMonth:	diminfo->ds.UMonth.calFlags |= VCAL_Flags_Historical ; break ;
			   case V4DPI_PntType_UYear:	diminfo->ds.UYear.calFlags |= VCAL_Flags_Historical ; break ;
			   case V4DPI_PntType_UPeriod:	diminfo->ds.UPeriod.calFlags |= VCAL_Flags_Historical ; break ;
			   case V4DPI_PntType_UWeek:	diminfo->ds.UWeek.calFlags |= VCAL_Flags_Historical ; break ;
			   case V4DPI_PntType_UQuarter:	diminfo->ds.UQuarter.calFlags |= VCAL_Flags_Historical ; break ;
			 } ;
			break ;
		      case _OutFormat:	/* OUTFORMAT xxx */
			v4lex_NextTkn(tcb,0) ;
			switch(tcb->type)
			 { default: v_Msg(ctx,ctx->ErrorMsgAux,"Cmd4MatStr",tcb->type,DE(OutFormat),UCsizeof(diminfo->OutFormat)) ; goto fail ;
//				    v4_error(0,tcb,"V4","Eval","INVOUTFORMAT","Expecting an OutFormat string") ;
			   case V4LEX_TknType_Keyword:	UCstrcpy(tcb->UCstring,tcb->UCkeyword) ;
			   case V4LEX_TknType_String:	break ;
			 } ;
			if (UCstrlen(tcb->UCstring) >= UCsizeof(diminfo->OutFormat))
			 { v_Msg(ctx,ctx->ErrorMsgAux,"Cmd4MatStr",tcb->type,DE(OutFormat),UCsizeof(diminfo->OutFormat)) ; goto fail ; } ;			 
//			   v4_error(0,tcb,"V4","Eval","TOOLONG","OutFormat argument too long") ;
			UCstrcpy(diminfo->OutFormat,tcb->UCstring) ; break ;
		      case _Intersection:
			diminfo->ds.List.lFlags |= V4DPI_DimInfoList_IsctModOK ; break ;
		      case _Periods:
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Integer) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;
			if (tcb->integer < 2 || tcb->integer > 100)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIntValue",tcb->type,(kwx=_Periods),1,100) ; goto fail ; } ;
			diminfo->ds.UPeriod.periodsPerYear = tcb->integer ;
			break ;
		      case _HasIsA:	/* HASISA */
			diminfo->Flags |= V4DPI_DimInfo_HasIsA ; break ;
		      case _Hierarchy:	/* HIERARCY n */	v4lex_NextTkn(tcb,0) ; diminfo->RelHNum = tcb->integer ; break ;
		      case _NoIC:
		      case _CaseSensitive:	/* CASESENSITIVE */
			diminfo->Flags |= V4DPI_DimInfo_CaseSensitive ; break ;
		      case _IsA:	/* ISA dim */
			v4lex_NextTkn(tcb,0) ;
			if (UCstrlen(tcb->UCkeyword) >= UCsizeof(di.DimName))
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdDimName",tcb->UCkeyword,UCsizeof(di.DimName)-1) ; goto fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","INVDIMNAME","Dimension name (%s) cannot exceed %d characters",tcb->keyword,sizeof(di.DimName)-1) ;
			 dix = v4dpi_DimInfoGet(ctx,v4dpi_DimGet(ctx,tcb->UCkeyword,DIMREF_REF)) ;
			 if (dix == NULL)
			  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotDim",tcb->UCkeyword) ; goto fail ; } ;
//			  v4_error(0,tcb,"V4","Eval","INVDIMName","Not a dimension: %s",tcb->keyword) ;
			 if (dix->PointType != diminfo->PointType)
			  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIsADimPT",DE(IsA),dix->DimId,dix->PointType,diminfo->PointType) ; goto fail ; } ;
//			  v4_error(0,tcb,"V4","Eval","NotSameType","Isa Dim:%s must be same point-type",dix->DimName) ;
			 diminfo->Flags |= dix->Flags ;
			UCstrcpy(diminfo->IsA,tcb->UCkeyword) ; break ;
		      case _Local:	/* LOCAL */
			UCstrcpy(UCTBUF1,argbuf) ;			/* Copy list of dimension names into ASCTBUF1 */
			ZUS(argbuf) ;
			for(ucb=UCTBUF1;*ucb!=UCEOS;)
			 { UCCHAR *bp,*bp1,tbuf[V4DPI_DimInfo_DimNameMax+10],unamebuf[V4DPI_DimInfo_DimNameMax+10] ;
			   bp = UCstrchr(ucb,',') ; if (bp != NULL) *bp = UCEOS ;
			   UCstrcpy(tbuf,ucb) ; bp1 = v4dpi_DimUniqueName(ctx,tbuf,unamebuf) ;
			   if (bp1 == NULL) goto fail ;
//			   if (bp1 == NULL) v4_error(0,tcb,"V4","Eval","INVDIMNAME","DIMENSION error - %s",UCretASC(ctx->ErrorMsgAux)) ;
			   if (UCstrlen(argbuf) > 0) UCstrcat(argbuf,UClit(",")) ; UCstrcat(argbuf,bp1) ;
			   if (gpi->dnb == NULL)
			    { gpi->dnb = (struct V4DPI__DimNameBlocks *)v4mm_AllocChunk(sizeof *gpi->dnb,FALSE) ;
			      gpi->dnb->BlockX = 0 ; gpi->dnb->Block[0].NameCount = 0 ;
			      if (withinpnt)			/* If within a point then bump up one level */
			       { gpi->dnb->BlockX = 1 ; gpi->dnb->Block[1].NameCount = 0 ; } ;
			    } ;
			   i = gpi->dnb->BlockX ;
			   if (gpi->dnb->Block[i].NameCount >= V4DPI_DimNameBlockNameMax)
			    { v_Msg(ctx,ctx->ErrorMsgAux,"DimMaxLocal",V4DPI_DimNameBlockNameMax,DE(Local)) ; goto fail ; } ;
//			    v4_error(0,tcb,"V4","Eval","INVDIMNAME","Cannot exceed more than (%d) Local Dimensions",V4DPI_DimNameBlockNameMax) ;
			   j = gpi->dnb->Block[i].NameCount++ ;
			   UCstrcpy(gpi->dnb->Names[j].IDimName,bp1) ; UCstrcpy(gpi->dnb->Names[j].DimName,ucb) ;
//			   for(bp1=gpi->dnb->Names[j].IDimName;*bp1!=UCEOS;bp1++) { *bp1 = UCtoupper(*bp1) ; } ;
//			   for(bp1=gpi->dnb->Names[j].DimName;*bp1!=UCEOS;bp1++) { *bp1 = UCtoupper(*bp1) ; } ;
			   if (bp == NULL) break ; ucb = bp + 1 ;
			 } ;
			break ;
		      case _MMDDYY:	/* MMDDYY */
			if (diminfo->PointType == V4DPI_PntType_UDate) { diminfo->ds.UDate.IFormat = V4LEX_TablePT_MMDDYY ; }
			 else if (diminfo->PointType == V4DPI_PntType_Calendar) { diminfo->ds.Cal.IFormat = V4LEX_TablePT_MMDDYY ; }
			 else if (diminfo->PointType == V4DPI_PntType_UDT) { diminfo->ds.UDT.IFormat = V4LEX_TablePT_MMDDYY ; }
			 else { v_Msg(ctx,ctx->ErrorMsgAux,"CmdArgNotAllow",(kwx=_MMDDYY),V4DPI_PntType_UDate) ; goto fail ; } ;
//			 else { v4_error(0,tcb,"V4","Eval","INVDIM","MMDDYY argument only allowed with UDate point types") ; } ;
			break ;
		      case _MMYY: /* MMYY */
			if (diminfo->PointType == V4DPI_PntType_UMonth) { diminfo->ds.UMonth.IFormat = V4LEX_TablePT_MMYY ; }
			 else { v_Msg(ctx,ctx->ErrorMsgAux,"CmdArgNotAllow",(kwx=_MMYY),V4DPI_PntType_UDate) ; goto fail ; } ;
//			 else { v4_error(0,tcb,"V4","Eval","INVDIM","MMYY argument only allowed with UMonth point types") ; } ;
			break ;
		      case _Multiple:	/* MULTIPLE */		diminfo->Flags |= V4DPI_DimInfo_ListOK ; break ;
		      case _None:	/* NONE value */
			v4lex_NextTkn(tcb,V4LEX_Option_NegLit) ;
			switch (diminfo->PointType)
			 { default:		v_Msg(ctx,ctx->ErrorMsgAux,"CmdArgNotNum",tcb->type) ; goto fail ; 
			   case V4DPI_PntType_Int:
				if (tcb->type != V4LEX_TknType_Integer) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotNum",tcb->type) ; goto fail ; } ;
				diminfo->ds.Int.NoneValue = tcb->integer ;
				break ;
			   case V4DPI_PntType_Real:
				if (tcb->type == V4LEX_TknType_Integer) { diminfo->ds.Real.NoneValue = (double)tcb->integer ; }
				 else if (tcb->type == V4LEX_TknType_Float) { diminfo->ds.Real.NoneValue = tcb->floating ; }
				 else { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotNum",tcb->type) ; goto fail ; } ;
				break ;
			 } ; 
			diminfo->Flags |= V4DPI_DimInfo_HaveNone ; break ;
		      case _Value:
		        diminfo->Flags &= ~(V4DPI_DimInfo_ValueTree|V4DPI_DimInfo_ValueList) ;		/* Clear all flags */
			switch (v4im_GetTCBtoEnumVal(ctx,tcb,NOEOL))
		         { default:	v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetEcho",DE(Dimension),DE(Value),tcb->UCkeyword) ; goto fail ;
			   case _None:	break ;
			   case _List:	diminfo->Flags |= V4DPI_DimInfo_ValueList ; break ;
			   case _Tree:	diminfo->Flags |= V4DPI_DimInfo_ValueTree ; break ;
			 } ;
			break ;
		      case _Normalize:	/* NORMALIZE */
			diminfo->Flags |= V4DPI_DimInfo_Normalize ; break ;
		      case _Overload:	/* OVERLOAD percentage */
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Integer)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;
//			 v4_error(V4E_NOTINTEGER,tcb,"V4","Eval","NOTINTEGER","Expecting integer value, not: %s",tcb->keyword) ;
			if (tcb->integer < 0 || tcb->integer > 100)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColOver",DE(Overload),tcb->integer) ; goto fail ; } ;
//			 v4_error(V4E_NOTINTEGER,tcb,"V4","Eval","NOTINTEGER","Overload must be 0 thru 100, not: %d",tcb->integer) ;
			diminfo->PCAggExtend = tcb->integer ;
			break ;
		      case _Point:
		      case _PointCreate:	/* POINTCREATE */
			switch (v4im_GetTCBtoEnumVal(ctx,tcb,NOEOL))
		         { default:	v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetEcho",DE(Dimension),DE(PointCreate),tcb->UCkeyword) ; goto fail ;
			   case _New:	diminfo->UniqueOK = (diminfo->PointType == V4DPI_PntType_AggRef ? V4DPI_DimInfo_UOkAgg : V4DPI_DimInfo_UOkNew) ; break ;
			   case _Point:	diminfo->UniqueOK = V4DPI_DimInfo_UOkPoint ; break ;
			 } ;
			break ;
		      case _Range:	/* RANGE */		diminfo->Flags |= V4DPI_DimInfo_RangeOK ; break ;
		      case _RDB:
		      case _RDBNumeric:	/* RDBNumeric */
			diminfo->Flags |= V4DPI_DimInfo_RDBNumeric ; break ;
		      case _Structure: /* Structure */
			diminfo->Flags |= V4DPI_DimInfo_Structure ; break ;
		      case _TimeZone:	/* TimeZone */
//			 v4_error(0,tcb,"V4","Eval","INVDIM","TIMEZONE argument only allowed with CALENDAR point types") ;
			v4lex_NextTkn(tcb,V4LEX_Option_NegLit) ;
			if (tcb->type != V4LEX_TknType_Integer || tcb->integer < -12 || tcb->integer > 12)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdTZErr",tcb->type,(kwx=_TimeZone)) ; goto fail ; } ;
//			 v4_error(V4E_NOTINTEGER,tcb,"V4","Eval","NOTINTEGER","Expecting integer value between -12..12, not: %s",tcb->keyword) ;
			if (diminfo->PointType == V4DPI_PntType_Calendar) { diminfo->ds.Cal.TimeZone = tcb->integer ; }
			 else if (diminfo->PointType == V4DPI_PntType_GeoCoord) { diminfo->ds.Geo.TimeZone = tcb->integer ; }
			 else { v_Msg(ctx,ctx->ErrorMsgAux,"CmdArgNotAllow",(kwx=_TimeZone),V4DPI_PntType_Calendar) ; goto fail ; } ;
			break ;
		      case _UOMId:	/* UOMID num */
			v4lex_NextTkn(tcb,0) ; diminfo->UOMRef = tcb->integer ; break ;
		      case _YMDOrder:
			for(i=0;i<VCAL_YMDOrderMax;i++)
			 { int ymdo ;
			   switch (v4im_GetTCBtoEnumVal(ctx,tcb,NOEOL))
			    { default:		v_Msg(ctx,ctx->ErrorMsgAux,"CalYMDOrder1",tcb->type) ; goto fail ;
			      case _DMY:	ymdo = V4LEX_YMDOrder_DMY ; break ;
			      case _DYM:	ymdo = V4LEX_YMDOrder_DYM ; break ;
			      case _MDY:	ymdo = V4LEX_YMDOrder_MDY ; break ;
			      case _MYD:	ymdo = V4LEX_YMDOrder_MYD ; break ;
			      case _YDM:	ymdo = V4LEX_YMDOrder_YDM ; break ;
			      case _YMD:	ymdo = V4LEX_YMDOrder_YMD ; break ;
			    } ;
			   switch (diminfo->PointType)
			    { default:				v_Msg(ctx,NULL,"DimInvOption2",tcb->type,DE(YMDOrder),diminfo->PointType) ; goto fail ;
			      case V4DPI_PntType_UDate:		diminfo->ds.UDate.YMDOrder[i] = ymdo ; break ;
			      case V4DPI_PntType_UDT:		diminfo->ds.UDT.YMDOrder[i] = ymdo ; break ;
			      case V4DPI_PntType_Calendar:	diminfo->ds.Cal.YMDOrder[i] = ymdo ; break ;
			      case V4DPI_PntType_GeoCoord:	diminfo->ds.Geo.YMDOrder[i] = ymdo ; break ;
			    } ;
			   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
			   if (tcb->opcode != V_OpCode_Comma) { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; break ; } ;
			 } ;
			break ;
//		      case _NonNumeric:	/* NONNUMERIC */
//			diminfo->Flags |= V4DPI_DimInfo_NonNumeric ; break ;
//		      case _Numeric:	/* NUMERIC */		diminfo->Flags |= V4DPI_DimInfo_NumericDict ; break ;
		      case _Unique:	/* UNIQUE */		diminfo->UniqueOK = V4DPI_DimInfo_UOkNew ; break ;
		      case _Internal: 	/* INTERNAL */		diminfo->DictType = V4DPI_DictType_Int ; break ;
		      case _External:	/* EXTERNAL */		diminfo->DictType = V4DPI_DictType_Ext ; break ;
		      case _Dimension:	/* DIMENSION */		diminfo->DictType = V4DPI_DictType_Dim ; break ;
		    } ;
		 } ;


//		v4_error(0,tcb,"V4","Eval","NODECIMALS","The \"Decimals nn\" entry must be included for dimensions of type FIXED") ;
		for(ucb=argbuf;diminfo==&di;)			/* Loop thru all dimension names */
		 { UCCHAR *mp = UCstrchr(ucb,',') ; if (mp != NULL) *mp = UCEOS ;
		   UCstrcpy(di.DimName,ucb) ;
		   if (v4dpi_DimMake(ctx,&di) <= 0) goto fail ;			/* Make dimension if LOCALLY not specified */
//		    v4_error(0,tcb,"V4","Eval","DIMERR","DIMENSION error: %s",UCretASC(ctx->ErrorMsgAux)) ;
		   if (mp == NULL) break ;
		   ucb = mp + 1 ;					/* Advance to next name */
		 } ;
		break ;
//	      case _Clipboard:	/* CLIPBOARD file */
//		v4lex_NextTkn(tcb,0) ; 		/* Get file name */
//		if (tcb->type == V4LEX_TknType_Keyword)
//		 { for(i=UCstrlen(tcb->UCkeyword)-1;i>=0;i--) { tcb->keyword[i] = tolower(tcb->keyword[i]) ; } ;
//		   strcat(tcb->string,".cbf") ;
//		 } ;
//		v4eval_FileToClipboard(tcb->string) ;
//		break ;
	      case _Dump:	/* DUMP filterpoint */
		if (CHECKV4INIT) goto fail ;
		if (!v4dpi_PointParse(ctx,&pisct,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
		switch (pisct.PntType)
		 { default:
		     v_Msg(ctx,ctx->ErrorMsgAux,"DumpInvArg",&pisct) ; goto fail ;
		   case V4DPI_PntType_Isct:	/* If isct then take dim from first point, use isct as filter */
		     filter = &pisct ; dpt = ISCT1STPNT(filter) ; dim = dpt->Dim ;
		     break ;
		   case V4DPI_PntType_Special:
		     if (pisct.Grouping == V4DPI_Grouping_All) { filter = NULL ; dim = pisct.Dim ; break ; } ;
		     v_Msg(ctx,ctx->ErrorMsgAux,"DumpInvArg",&pisct) ; goto fail ;
		   case V4DPI_PntType_Dict:	/* If just Dict point then convert to dim - no filter */
		     if (pisct.Dim == Dim_Dim) { dim = pisct.Value.IntVal ; }
		      else { dim = v4dpi_DimGet(ctx,v4dpi_RevDictEntryGet(ctx,pisct.Value.IntVal),DIMREF_LEX) ; } ;
		     if (dim <= 0) { v_Msg(ctx,ctx->ErrorMsgAux,"DimNotDim",&pisct) ; goto fail ; } ;
		     filter = NULL ; break ;
		 } ; 
		if (gpi->RestrictionMap & V_Restrict_DataStream)		/* If we have locked down the data stream then don't even bother looking for '> file' or '>> file' */
		 { tcb->need_input_line = TRUE ;
		 } else		
		 { if (tcb->type != V4LEX_TknType_EOL) v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
		   if (tcb->opcode == V_OpCode_RangleRangle)
			{ if (gpi->RestrictionMap & V_Restrict_redirection) { v_Msg(ctx,ctx->ErrorMsgAux,"RestrictCmd") ; goto fail ; } ;
			  voutRedirected = TRUE ; 
			  if ((i=vout_OpenStreamFile(NULL,NULL,NULL,tcb,TRUE,gpi->OutputCharSet,0,ctx->ErrorMsgAux)) == UNUSED) goto fail ;
			  vout_BindStreamFile(i,VOUT_FileType_File,VOUT_Trace,ctx->ErrorMsgAux) ;
			}
		    else if (tcb->opcode == V_OpCode_Rangle)
			{ if (gpi->RestrictionMap & V_Restrict_redirection) { v_Msg(ctx,ctx->ErrorMsgAux,"RestrictCmd") ; goto fail ; } ;
			  voutRedirected = TRUE ;
			  if ((i=vout_OpenStreamFile(NULL,NULL,NULL,tcb,FALSE,gpi->OutputCharSet,0,ctx->ErrorMsgAux)) == UNUSED) goto fail ;
			  vout_BindStreamFile(i,VOUT_FileType_File,VOUT_Trace,ctx->ErrorMsgAux) ;
			}
		    else { if (tcb->type != V4LEX_TknType_EOL) { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ; } ;
		   if (!v4eval_ChkEOL(tcb)) goto fail ;
		 } ;
		for(i=0;(i=v4eval_ScanAreaForBinds(ctx,dim,i,&isctpt,&valpt,filter))!=0;)
		 { 
		   v_Msg(ctx,UCTBUF2,"@*%1P = %2P\n",&isctpt,&valpt) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
		 } ;

		if (voutRedirected) vout_CloseFile(UNUSED,VOUT_Trace,NULL) ;
		break ;
	      case _Optimize:	/* OPTIMIZE */
	      case _Eval:	/* EVALUATE expression [ > or >> file] */
	      case _Evaluate:	/* EVALUATE expression [ > or >> file] */
		if (CHECKV4INIT) goto fail ;
//		if (!KernelCheck)
//		 { if (v4dpi_DimGet(ctx,"Dim") != 0) { KernelCheck = TRUE ; }
//		    else { v_Msg(ctx,ASCTBUF1,"CmdNoKernel") ; vout_Text(VOUT_Err,0,ASCTBUF1) ; vout_NL(VOUT_Err) ; v4lex_ReadNextLine(tcb,V4LEX_ReadLine_FlushCmd,NULL,NULL) ; break ; } ;
//		 } ;
		ZUS(gpi->SavedErrMsg) ;
		if (gpi->breakOnEval)
		 { jmp_buf traceback ; ETYPE eres ;
		   struct V4LEX__TknCtrlBlk *tcb = (struct V4LEX__TknCtrlBlk *)v4mm_AllocChunk(sizeof *tcb,FALSE) ; v4lex_InitTCB(tcb,0) ;
		   UCstrcpy(tcb->ilvl[0].prompt,UClit("V4DBG>")) ;
		   gpi->breakOnEval = FALSE ;			/* Only do this once */
		   COPYJMP(traceback,(gpi->environment)) ;
		   v_Msg(ctx,UCTBUF2,"DBGBreakEval") ; vout_UCText(VOUT_Status,0,UCTBUF2) ;
		   traceGlobal = trace ;
		   eres = v4eval_Eval(tcb,ctx,TRUE,trace,TRUE,FALSE,FALSE,NULL) ;
		   COPYJMP((gpi->environment),traceback) ; v4lex_FreeTCB(tcb) ;
		   switch (eres)
		    { default:				/* Default is to exit */
			return(V4EVAL_Res_ExitStats) ;
		      case V4EVAL_Res_BPContinue:	/* Continue from break point */
			trace = traceGlobal ;		/* If trace options changed then grab them for this level */
			break ;
		      case V4EVAL_Res_BPStep:		/* Continue with eval on 'next line' from break point */
		      case V4EVAL_Res_BPStepNC:		/* Continue with eval on 'next line' from break point */
		      case V4EVAL_Res_BPStepInto:	/* Continue with next eval from break point */
			break ;
		    } ;
		 } ;
		if (!v4dpi_PointParse(ctx,&isctpt,tcb,V4DPI_PointParse_RetFalse)) goto fail ; voutRedirected = FALSE ;
		if (gpi->RestrictionMap & V_Restrict_DataStream)		/* If we have locked down the data stream then don't even bother looking for '> file' or '>> file' */
		 { tcb->need_input_line = TRUE ;
		 } else		
		 { if (tcb->type != V4LEX_TknType_EOL) v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
		   if (tcb->opcode == V_OpCode_RangleRangle)
			{ if (gpi->RestrictionMap & V_Restrict_redirection) { v_Msg(ctx,ctx->ErrorMsgAux,"RestrictCmd") ; goto fail ; } ;
			  voutRedirected = TRUE ; 
			  if ((i=vout_OpenStreamFile(&protoEvalStream,NULL,NULL,tcb,TRUE,gpi->OutputCharSet,0,ctx->ErrorMsgAux)) == UNUSED) goto fail ;
//			   v4_error(0,tcb,"V4","Eval","NOOUT","Could not append output - %s",UCretASC(ctx->ErrorMsgAux)) ;
			  vout_BindStreamFile(i,VOUT_FileType_File,VOUT_Data,ctx->ErrorMsgAux) ;
			}
		    else if (tcb->opcode == V_OpCode_Rangle)
			{ if (gpi->RestrictionMap & V_Restrict_redirection) { v_Msg(ctx,ctx->ErrorMsgAux,"RestrictCmd") ; goto fail ; } ;
			  voutRedirected = TRUE ;
			  if ((i=vout_OpenStreamFile(&protoEvalStream,NULL,NULL,tcb,FALSE,gpi->OutputCharSet,0,ctx->ErrorMsgAux)) == UNUSED) goto fail ;
//			   v4_error(0,tcb,"V4","Eval","NOOUT","Could not create output - %s",UCretASC(ctx->ErrorMsgAux)) ;
			  vout_BindStreamFile(i,VOUT_FileType_File,VOUT_Data,ctx->ErrorMsgAux) ;
			}
		    else { if (tcb->type != V4LEX_TknType_EOL) { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ; } ;
		   if (!v4eval_ChkEOL(tcb)) goto fail ;
		 } ;

		if (cx == _Optimize)
		 { num = v4eval_OptPoint(ctx,&isctpt,&xisct,sizeof xisct,NULL) ;
		   if (trace & V4TRACE_Optimize)
		    { 
//		      v4dpi_PointToString(ASCTBUF1,&xisct,ctx,V4DPI_FormatOpt_ShowDim) ;
		      v_Msg(ctx,UCTBUF2,"@* Optimize => %1P\n",&xisct) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
		    } ;
		   dpt = v4dpi_IsctEval(&point,&xisct,ctx,0,NULL,NULL) ;
		 } else
		 { dpt = v4dpi_IsctEval(&point,&isctpt,ctx,0,NULL,NULL) ;
		 } ;

/*		Do we have to evaluate a 'Deferred' Do() ? */
		if (ctx->vdds != NULL && dpt != NULL)
		 { INDEX i ; FILEID saveFileId ;
		   saveFileId = gpi->fileIdDefer ; gpi->fileIdDefer = UNUSED ;	/* Clear this so deferred do is sent to normal stream output */
		   dpt = v4im_DoDo(ctx,&valpt,ctx->vdds->argcnt,ctx->vdds->argpnts,V4IM_OpCode_DeferDo) ;
		   for(i=1;i<ctx->vdds->argcnt;i++) { v4mm_FreeChunk(ctx->vdds->argpnts[i]) ; } ;
		   if (ctx->vdds->ctxcnt > 0) v4mm_FreeChunk(ctx->vdds->ctxPts) ;
		   v4mm_FreeChunk(ctx->vdds) ; ctx->vdds = NULL ;
		   gpi->fileIdDefer = saveFileId ;
		 } ;
/*		Have we been buffering AJAX output (or deferred Do()) ? If so then send it to VOUT_Data */
		if (gpi->fileIdDefer != UNUSED && !nested)	/* VEH110224 - Don't want to close off if here because of nested eval (via EvalCmd() module) */
		 { if (dpt == NULL)		/* Handle via different AJAX/JSON patterns depending on error or not */
		    { if (UCnotempty(gpi->patternAJAXErr))
		       { UCCHAR buf2[VOUT_FileType_Buffer_Max], buf[VOUT_FileType_Buffer_Max] ;
		         v_StringLit(ctx->ErrorMsg,buf2,VOUT_FileType_Buffer_Max,'"','\\') ;
#ifdef USEOLDAJAXSUB	
			 v_Msg(ctx,buf,gpi->patternAJAXErr,UClit("error"),buf2) ;
#else
			 vjson_SubParamsInJSON(buf,VOUT_FileType_Buffer_Max,gpi->patternAJAXErr,buf2,UClit("error")) ;
#endif
			 vout_UCTextFileX(vout_FileIdToFileX(gpi->fileIdDefer),0,buf) ;
		       } ;
		      vout_CloseDeferStream(gpi->fileIdDefer) ;
		    } else
		    { if (UCnotempty(gpi->patternAJAXJSON)) { vout_CloseAjaxStream(gpi->fileIdDefer) ; }
		       else { vout_CloseDeferStream(gpi->fileIdDefer) ; } ;
		    } ;
		   gpi->fileIdDefer = UNUSED ;
		 } ;
		if (voutRedirected) { vout_CloseFile(UNUSED,VOUT_Data,NULL) ; } ;
		if (dpt == NULL)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIsctFail",&isctpt) ;
		   ctx->rtStackX = 0 ;
		   ctx->rtStack[ctx->rtStackX].vis = isctpt.Value.Isct.vis ;		/* Plug in failure at level 0 (top level) */
		   ctx->rtStack[ctx->rtStackX].isctPtr = &isctpt ; 
		   if (ctx->rtStack[ctx->rtStackX].failText == NULL) ctx->rtStack[ctx->rtStackX].failText = v4mm_AllocUC(V4TMBufMax) ;
		   UCstrcpy(ctx->rtStack[ctx->rtStackX].failText,ctx->ErrorMsg) ;
//		   dpt = ISCT1STPNT(&isctpt) ; ctx->rtStack[ctx->rtStackX].intmodX = (dpt->PntType == V4DPI_PntType_IntMod ? dpt->Value.IntVal : UNUSED) ;
		   goto fail_full ;
		 } ;
//		   v4dpi_PointToString(ASCTBUF1,&isctpt,ctx,V4DPI_FormatOpt_ShowDim) ;
//		   v4_error(V4E_NOISCTEVALTOOUTPUT,tcb,"V4","Eval","NOISCTEVALTOOUTPUT","Evaluation failed") ;
		break ;
	      case _Structure:	/* STRUCTURE xxx */
	      case _Element:	/* ELEMENT */
		if (gpi->rtl == NULL)				/* Is this the first one? */
		 { gpi->rtl = (struct V4V4IS__RuntimeList *)v4mm_AllocChunk(sizeof(*gpi->rtl),TRUE) ; } ;
		rtl = gpi->rtl ;
		v4lex_NextTkn(tcb,0) ;
		if (tcb->type != V4LEX_TknType_Integer)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;
//		 v4_error(V4E_NOTINTEGER,tcb,"V4","Eval","NOTINTEGER","Expecting integer value, not: %s",tcb->keyword) ;
		for(i=0;i<rtl->Count;i++) { if (rtl->FileRefs[i] == tcb->integer) break ; } ;
		if (i >= rtl->Count)
		 { if (rtl->Count >= V4V4IS_RTList_Max)
		    v4_error(0,0,"V4V4IS","LoadFileRef","MAXREFS","Exceeded max (%d) number of runtime file refs",V4V4IS_RTList_Max) ;
		   i = rtl->Count ;					/* Add new structure/element list */
		   rtl->stel[i] = (struct V4V4IS__StructEl*)v4mm_AllocChunk(sizeof *rtl->stel[i],TRUE) ;
		   rtl->IsNew[i] = TRUE ; rtl->FileRefs[i] = tcb->integer ;
		   stel = rtl->stel[i] ; rtl->Count++ ;
		   stel->kp.fld.KeyMode = V4IS_KeyMode_Int ; stel->kp.fld.KeyType = V4IS_KeyType_V4 ;
		   stel->kp.fld.Bytes = 8 ; stel->kp.fld.AuxVal = V4IS_SubType_V4ISStEl ;
		   stel->FileRef = tcb->integer ;			/* Set up key */
		 } ; stel = rtl->stel[i] ;
		if (stel->Count >= V4V4IS_Field_Max)
		 v4_error(0,0,"V4EVAL","STELParse","MAXFIELD","Exceeded max(%d) fields in Structure/Element Ref #%d",
					V4V4IS_Field_Max,stel->FileRef) ;
#define SENUM(el) \
 v4lex_NextTkn(tcb,0) ; \
 if (tcb->type != V4LEX_TknType_Integer) \
  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;\
 stel->Field[stel->Count].el = tcb->integer ;
		SENUM(StructNum) SENUM(Index) v4lex_NextTkn(tcb,0) ; /* Skip Name */ SENUM(Bytes) SENUM(Offset)
		if (cx == _Structure)				/* finish off structure */
		 { stel->Field[stel->Count].V3DataType = UNUSED ; SENUM(Decimals)
		 } else					/* finishing off field */
		 { SENUM(V3DataType) SENUM(Decimals) } ;
		if (!v4eval_ChkEOL(tcb)) goto fail ;
		stel->Field[stel->Count].StructNum-- ; stel->Count++ ;
		break ;
	      case _Echo:	/* ECHO xxxx */
	      case _Error:	/* ERROR xxx */
		ZUS(ctx->ErrorMsg) ;
		for(looper=TRUE;looper;)
		 { v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;		/* Grab next token */
		   switch (tcb->type)
		    { case V4LEX_TknType_EOL:		looper = FALSE ; break ;
		      case V4LEX_TknType_Keyword:	/*strcat(ctx->ErrorMsg,tcb->keyword) ; break ; */
		      case V4LEX_TknType_String:	UCstrcat(ctx->ErrorMsg,tcb->UCstring) ; break ;
		      case V4LEX_TknType_Integer:	{ UCCHAR tbuf[64] ; UCsprintf(tbuf,UCsizeof(tbuf),UClit("%d"),tcb->integer) ; UCstrcat(ctx->ErrorMsg,tbuf) ; break ; }
		      case V4LEX_TknType_Punc:		if (tcb->opcode == V_OpCode_Semi) { looper = FALSE ; } else { UCstrcat(ctx->ErrorMsg,tcb->UCkeyword) ; } ; break ;
		    } ;
		 } ;
		 if (cx == _Echo) { v_Msg(ctx,UCTBUF2,"@*%1U\n",ctx->ErrorMsg) ; vout_UCText(VOUT_Status,0,UCTBUF2) ; }
		  else { v4_UCerror(V4E_USERERROR,tcb,"","","",ctx->ErrorMsg) ; } ;
		break ;
	      case _Exxit:	/* EXIT */
		if (bt != NULL) { v4mm_FreeChunk(bt) ; bt = NULL ; } ;
//		switch (v4eval_NextKeyWord(exitlist,tcb,EOLOK))
		switch (v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
		 { default:	v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,cx) ; goto fail ;
//				v4_error(V4E_INVSETATTR,tcb,"V4","Eval","INVSETATTR","Unknown EXIT keyword: %s",tcb->keyword) ;
		   case _EndOfLine_:
		   case _Statistics:	i = V4EVAL_Res_ExitStats ; break ;
		   case _File:		v4lex_CloseOffLevel(tcb,1) ;
					if (nested) {i = V4EVAL_Res_ExitNoStats ; break ; } ;
					continue ;
		   case _NoStatistics:	i = V4EVAL_Res_ExitNoStats ; break ;
		 } ;
		return(i) ;
	      case _ElseIf:	/* ELSEIF xxx */
		if (tcb->ifx <= 0)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotIf",tcb->type,DE(If),DE(Then),DE(Else)) ; goto fail ; } ;
//			v4_error(V4E_NOTINIF,tcb,"V4","Eval","NOTINIF","Not in IF/ELSE/END situation!") ;
		if (!v4eval_CheckIfLevel(ctx,tcb)) goto fail ;
		if (tcb->ifs[tcb->ifx-1].doif) { tcb->ifs[tcb->ifx-1].doif = FALSE ; tcb->ifs[tcb->ifx-1].inif = FALSE ; break ; }
		 else { if (! tcb->ifs[tcb->ifx-1].doelse) break ; } ;
							/* Fall thru to IF if still trying for "ELSE" portion */
	      case _If1:	/* IF1 xxx expression */
	      case _If:	/* IF xxx */
		if (tcb->ifx >= V4LEX_NestedIfThenElse - 1) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIfMaxNest",tcb->type) ; goto fail ; } ;
//040610	if (tcb->ifx == 0) tcb->ifxilx = tcb->ilx ;	/* Remember defining ilx level (for error checking) */
		if (cx == _If) { if (!v4eval_SetIfLevel(ctx,tcb)) goto fail ; } ;
		if (!tcb->ilvl[tcb->ilx].BoundMacroArgs) 
		 { if (!v4lex_BindArgs(tcb))			/* Bind arguments if necessary */
		    { UCstrcpy(ctx->ErrorMsgAux,tcb->UCstring) ; goto fail ; } ;
		 } ;
		notflag = FALSE ;
not_rentry:
//		switch (v4eval_NextKeyWord(iflist,tcb,NOEOL))
		switch (v4im_GetTCBtoEnumVal(ctx,tcb,NOEOL))
		 { default:	v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,cx) ; goto fail ;
//				v4_error(V4E_BADIFKW,tcb,"V4","Eval","BADIFKW","Invalid IF keyword: %s",tcb->keyword) ;
		   case _Defined:
			v4lex_NextTkn(tcb,V4LEX_Option_ForceAsIs) ; i = FALSE ;
			switch (tcb->type)
			 { case V4LEX_TknType_Keyword:
				if (UCstrlen(tcb->UCkeyword) == 1)		/* Got a letter - check to see if defined */
				 { if (tcb->UCkeyword[0] >= UClit('A') && tcb->UCkeyword[0] <= UClit('Z')) { i = tcb->UCkeyword[0] - UClit('A') ; }
				    else if (tcb->UCkeyword[0] >= UClit('a') && tcb->UCkeyword[0] <= UClit('z')) { i = tcb->UCkeyword[0] - UClit('a') ; }
				    else i = -1 ;
				   if (i >= 0) { i = (UCstrlen(tcb->poundparam[i].value) > 0 || tcb->poundparam[i].numvalue > 0) ; }
				    else i = FALSE ;
				 } else
		 		 { UCCHAR tname[V4LEX_Tkn_KeywordMax+1] ;
		 		   UCstrcpy(tname,tcb->UCkeyword) ; v4lex_NextTkn(tcb,V4LEX_Option_ForceAsIs) ;
				   if (tcb->opcode == V_OpCode_Star)	/* Got name* - see if defined in current context */
				    { if (gpi->vltCur != NULL)		/* If processing table then see if got a column spec */
				       { for(i=0;i<gpi->vltCur->Columns;i++) { if (UCstrcmpIC(gpi->vltCur->Col[i].Name,tname) == 0) break ; } ;
				         if (i < gpi->vltCur->Columns)				/* Found it - copy current value into point */
				          { if (gpi->vltCur->Col[i].Cur != NULL || (gpi->vltCur->InXML ? (gpi->vltCur->Col[i].XMLFlags & V4LEX_Table_XML_Defined)==0 : FALSE))
					     { i = !(gpi->vltCur->Col[i].Cur->PntType == V4DPI_PntType_Special) ; break ; } ;
					  } ;
				       } ;
				      if((i = v4dpi_DimGet(ctx,tname,DIMREF_LEX)) != 0) { DIMVAL(dpt,ctx,i) ; } else { dpt = NULL ; } ;
				      i = (dpt != NULL) ; break ;
				    } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
				   for(i=0;i<V4LEX_Tkn_ArgMax;i++)	/* Got a name - see if argument is defined */
		    		    { if (UCstrcmpIC(tcb->ilvl[tcb->ilx].macarg[i].name,tname) == 0) break ; } ;
				    if (i >= V4LEX_Tkn_ArgMax)
				     { v_Msg(ctx,ctx->ErrorMsgAux,"CmdMacroArg3",tcb->type,tname) ; goto fail ; } ;
//				     v4_error(V4E_NOSUCHARGNAME,tcb,"V4","Eval","NOSUCHARGNAME","No such argument name: %s",UCretASC(tname)) ;
				    i = tcb->ilvl[tcb->ilx].macarg[i].value != NULL ;
				 } ;
				break ;
			   case V4LEX_TknType_String:
				i = UCstrlen(tcb->UCstring) > 0 ; break ;
			   case V4LEX_TknType_Integer:
				if (tcb->integer < 1 || tcb->integer > 9) { i = FALSE ; break ; } ;
				i = (tcb->ilvl[tcb->ilx].macarg[tcb->integer].value != NULL) ;
				break ;
			 } ;
			break ;
		   case _Empty:
			v4lex_NextTkn(tcb,V4LEX_Option_ForceAsIs) ; i = FALSE ;
			switch (tcb->type)
			 { case V4LEX_TknType_Keyword:
				if (UCstrlen(tcb->UCkeyword) == 1)		/* Got a letter - check to see if defined */
				 { if (tcb->UCkeyword[0] >= UClit('A') && tcb->UCkeyword[0] <= UClit('Z')) { i = tcb->UCkeyword[0] - UClit('A') ; }
				    else if (tcb->UCkeyword[0] >= UClit('a') && tcb->UCkeyword[0] <= UClit('z')) { i = tcb->UCkeyword[0] - UClit('a') ; }
				    else i = -1 ;
//				   if (i >= 0) { i = (UCstrlen(tcb->poundparam[i].value) == 0) ; }
				   if (i >= 0) { i = UCempty(tcb->poundparam[i].value) ; }
				    else i = FALSE ;
				 } else
		 		 { for(j=0;j<V4LEX_Tkn_ArgMax;j++)	/* Got a name - see if argument is defined */
		    		    { if (UCstrcmpIC(tcb->ilvl[tcb->ilx].macarg[j].name,tcb->UCkeyword) == 0) break ; } ;
				    if (j >= V4LEX_Tkn_ArgMax) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdMacroArg3",tcb->type,tcb->UCkeyword) ; goto fail ; } ;
//				     v4_error(V4E_NOSUCHARGNAME,tcb,"V4","Eval","NOSUCHARGNAME","No such argument name: %s",UCretASC(tcb->UCkeyword)) ;
				    i = tcb->ilvl[tcb->ilx].macarg[j].value == NULL ;
				    if (!i)
				     { if (UCstrncmp(tcb->ilvl[tcb->ilx].macarg[j].value,UClit("<>"),2) == 0) { i = TRUE ; }
				        else if (UCstrncmp(tcb->ilvl[tcb->ilx].macarg[j].value,UClit(" <>"),3) == 0) { i = TRUE ; }
				     } ;
				 } ;
				break ;
			   case V4LEX_TknType_String:
//				i = UCstrlen(tcb->UCstring) == 0 ; break ;
				i = UCempty(tcb->UCstring) ; break ;
			   case V4LEX_TknType_Integer:
				if (tcb->integer < 1 || tcb->integer > 9) { i = FALSE ; break ; } ;
				i = (tcb->ilvl[tcb->ilx].macarg[tcb->integer].value == NULL) ;
				break ;
			 } ;
			break ;
		   case _Exists:	/* EXISTS file */
			v_ParseFileName(tcb,filename,NULL,NULL,&i) ;
			i = v4eval_GetFileUpdateDT(ctx,filename) ;
			i = (i > 0) ; break ;
		   case _Same:	/* SAME xxx xxx */
		      { UCCHAR tbuf[V4LEX_Tkn_StringMax] ;
			if (!v4eval_NextAsString(ctx,tcb,V4LEX_Option_ForceKW,FALSE)) goto fail ; UCstrcpy(tbuf,tcb->UCstring) ;
			if (!v4eval_NextAsString(ctx,tcb,V4LEX_Option_ForceKW,FALSE)) goto fail ; i = UCstrcmp(tbuf,tcb->UCstring) == 0 ;
			break ;
		      } ;
		   case _Numeric:	/* NUMERIC xxx */
			v4lex_NextTkn(tcb,V4LEX_Option_NegLit) ; i = (tcb->type == V4LEX_TknType_Integer) ;
			break ;
		   case _NoBind:	/* NOBIND [isct] */
			if (!v4dpi_PointParse(ctx,&isctpt,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
			dpt = v4dpi_IsctEval(&point,&isctpt,ctx,V4DPI_EM_NoIsctFail,NULL,NULL) ;
			i = (dpt == NULL) ;			/* "true" if no binding for this isct */
			break ;
		   case _Section:	/* SECTION name */
			v4lex_NextTkn(tcb,V4LEX_Option_ForceKW) ;
			if (tcb->type != V4LEX_TknType_Keyword)
	 		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotName",tcb->type,DE(Section)) ; goto fail ; } ;
//	 		 v4_error(V4E_NOTIFNAME,tcb,"V4","Eval","NOTIFNAME","Expecting a keyword section name") ;
			i = FALSE ;
			for(ucb=tcb->sections;*ucb!=0;)
			 { for(j=0;*ucb!=0 && *ucb!=',';j++) { UCTBUF1[j] = *(ucb++) ; } ;
			   if (*ucb == ',') ucb++ ; UCTBUF1[j] = UCEOS ;
			   if (UCstrcmpIC(UCTBUF1,tcb->UCkeyword) == 0) { i = TRUE ; break ; } ;
			 } ;
			if (!i && tcb->ilvl[tcb->ilx].lvlSections != NULL)		/* If no match then see if we have any level specific sections */
			 { for(ucb=tcb->ilvl[tcb->ilx].lvlSections;*ucb!=0;)
			    { for(j=0;*ucb!=0 && *ucb!=',';j++) { UCTBUF1[j] = *(ucb++) ; } ;
			      if (*ucb == ',') ucb++ ; UCTBUF1[j] = UCEOS ;
			      if (UCstrcmpIC(UCTBUF1,tcb->UCkeyword) == 0) { i = TRUE ; break ; } ;
			    } ;
			 } ;
			break ;
		   case _Dim:
		   case _Dimension:	/* DIMENSION dimname */
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Keyword)
	 		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdDimName1",tcb->type) ; goto fail ; } ;
//			 v4_error(V4E_NOTIFNAME,tcb,"V4","Eval","NOTDIMNAME","Expecting a Dimension name") ;
			i = v4dpi_DimGet(ctx,tcb->UCkeyword,DIMREF_LEX) ;
			if (i <= 0 || i > 0xffff) { i = FALSE ; } else { i = TRUE ; } ;
			break ;
		   case _ValidPoint:	/* VALID dimname value */
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Keyword)
	 		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdDimName1",tcb->type) ; goto fail ; } ;
//	 		 v4_error(V4E_NOTIFNAME,tcb,"V4","Eval","NOTDIMNAME","Expecting a Dimension name") ;
			i = v4dpi_DimGet(ctx,tcb->UCkeyword,DIMREF_LEX) ;
			if (i <= 0 || i > 0xffff)
	 		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdDimName1",tcb->type) ; goto fail ; } ;
//			 v4_error(V4E_NOTIFNAME,tcb,"V4","Eval","NOTDIMNAME","Expecting a Dimension name") ;
			diminfo = v4dpi_DimInfoGet(ctx,i) ;
			i = v4dpi_PointAccept(&valpt,diminfo,tcb,ctx,V4DPI_PointParse_RetFalse) ;
			break ;
		   case _Not:	/* NOT */
			notflag = !notflag ; goto not_rentry ;
		   case _MoreRecent:	/* MORERECENT file1 file2 */
			v_ParseFileName(tcb,filename,NULL,NULL,&i) ;
			i = v4eval_GetFileUpdateDT(ctx,filename) ;
			v_ParseFileName(tcb,filename,NULL,NULL,&i) ;
			i = (i > v4eval_GetFileUpdateDT(ctx,filename)) ;
			break ;
		   case _Errors:
			i = (gpi->ErrCount != 0) ; break ;
		   case _True:	/* TRUE */
			i = TRUE ; break ;
		   case _False:	/* FALSE */
			i = FALSE ; break ;
		   case _Point:	/* POINT v4point */
			if (!v4dpi_PointParse(ctx,&isctpt,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
			dpt = v4dpi_IsctEval(&point,&isctpt,ctx,V4DPI_EM_NoIsctFail,NULL,NULL) ;
//			i = (dpt == NULL ? FALSE : v4im_GetPointLog(&ok,dpt,ctx)) ; if (!ok) goto fail ;
			if (dpt == NULL) { i = FALSE ; }
			 else { i = v4im_GetPointLog(&ok,dpt,ctx) ; if (!ok) goto fail ; } ;
			break ;
		   case _Bind:	/* BIND v4point */
			if (!v4dpi_PointParse(ctx,&isctpt,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
			dpt = v4dpi_IsctEval(&point,&isctpt,ctx,V4DPI_EM_NoIsctFail,NULL,NULL) ;
			i = (dpt != NULL) ;			/* "true" if eval succeeded this isct */
			break ;
		   case _Macro:	/* MACRO v4point */
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Keyword)
	 		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotName",tcb->type,DE(Macro)) ; goto fail ; } ;
//	 		 v4_error(V4E_NOTIFNAME,tcb,"V4","Eval","NOTDIMNAME","Expecting a Macro name") ;
			i = (v4eval_GetMacroEntry(ctx,tcb->UCkeyword,V4LEX_MACTYPE_Macro) != NULL) ;
			break ;
		   case _Test:	/* TEST v4point */
			if (!v4dpi_PointParse(ctx,&isctpt,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
			i = v4im_GetPointLog(&ok,&isctpt,ctx) ; if (!ok) goto fail ;
		 } ;
		if (notflag) i = !i ;				/* Maybe compliment result (i) */
		if (cx == _If1)					/* Got "IF1" ? */
		 { if (i) continue ;				/* If "true" then pop up & treat remainder of line as command */
		   for(;;)					/*  else skip to end-of-statement/line */
		    { v4lex_NextTkn(tcb,V4LEX_Option_RetEOL+V4LEX_Option_ForceAsIs) ;
		      if (tcb->type == V4LEX_TknType_EOL) break ; if (tcb->opcode == V_OpCode_Semi) break ;
		    } ;
		   break ;					/* Else ignore */
		 } ;
		if (cx == _If) tcb->ifx++ ;			/* Don't update ifx if dropping thru from ELSEIF */
		tcb->ifs[tcb->ifx-1].inif = TRUE ;
		tcb->ifs[tcb->ifx-1].doif = (i ? tcb->ilx : FALSE) ;	/* Mark TRUE with ilx level for error checking */
		tcb->ifs[tcb->ifx-1].doelse = ((! i) ? tcb->ilx : FALSE) ; /* in vcommon- ReadNextLine */
	        break ;
	      case _Else:	/* ELSE */
		if (tcb->ifx <= 0)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotIf",tcb->type,DE(If),DE(Then),DE(Else)) ; goto fail ; } ;
//		   v4_error(V4E_NOTINIF,tcb,"V4","Eval","NOTINIF","Not in IF/ELSE/END situation!") ;
		if (tcb->ifs[tcb->ifx-1].inif) { tcb->ifs[tcb->ifx-1].inif = FALSE ; }
		 else { v_Msg(ctx,ctx->ErrorMsgAux,"CmdOneElse",tcb->type,DE(Else),DE(If)); goto fail ; } ;
//			v4_error(V4E_MULTIPLEELSE,tcb,"V4","Eval","MULTIPLEELSE","Can only have one ELSE per IF") ;
		if (!v4eval_CheckIfLevel(ctx,tcb)) goto fail ; break ;
	      case _EndIf:	/* ENDIF */
		if (tcb->ifx <= 0)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotIf",tcb->type,DE(If),DE(Then),DE(Else)) ; goto fail ; } ;
//		 v4_error(V4E_NOTINIF,tcb,"V4","Eval","NOTINIF","Not in IF/ELSE/END situation!") ;
		if (!v4eval_CheckIfLevel(ctx,tcb)) goto fail ; tcb->ifx-- ;
	        break ;
	      case _Data:	/* DATA v4pt [include options] */
		if (!v4dpi_PointParse(ctx,&isctpt,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
		dpt = v4dpi_IsctEval(&point,&isctpt,ctx,0,NULL,NULL) ;
		if (dpt == NULL)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdDataEvlFail",DE(Data),&isctpt) ; goto fail ; } ;
//		 v4_error(V4E_NOISCTEVALTOOUTPUT,tcb,"V4","Data","NOISCTEVALTOOUTPUT","Evaluation of DATA point failed") ;
//		TEMPFILE(filename,"v4data") ;		/* Create a temp file name */
//		incfp = fopen(filename,"w") ;		/* Open file for write */
		incfp = v_MakeOpenTmpFile(UClit("v4data"),filename,sizeof filename,"w",ctx->ErrorMsgAux) ;
		if (incfp == NULL) goto fail ;
//		 v4_error(V4E_INVINCFILE,tcb,"V4","Data","INVINCFILE","Could not create temp DATA file(%s) - Err(%s)",filename,v_OSErrString(errno)) ;
		switch (dpt->PntType)
		 { default:
//		      v4dpi_PointToString(ASCTBUF1,dpt,ctx,0) ; fprintf(incfp,"%s\n",ASCTBUF1) ;
		      v_Msg(ctx,UCTBUF1,"@%1Pn",dpt) ; fprintf(incfp,"%s\n",UCretASC(UCTBUF1)) ;
		      break ;
		   case V4DPI_PntType_BigText:
		     { UCCHAR *bp,*bp1 ;
		      bp=v4_BigTextCharValue(ctx,dpt) ;
		      if (bp == NULL)
		       { v_Msg(ctx,ctx->ErrorMsgAux,"ListInvBigText") ; goto fail ; } ;
//		       v4_error(0,tcb,"V4","Data","NOBIGTEXT","Data point cannot get BigText - %s",UCretASC(ctx->ErrorMsgAux)) ;
		      for(;;)
		       { bp1 = UCstrchr(bp,EOLbt) ;
		         if (bp1 == NULL) { fprintf(incfp,"%s\n",UCretASC(bp)) ; break ; } ;
			 *bp1 = UCEOS ; fprintf(incfp,"%s\n",UCretASC(bp)) ;
			 *bp1 = EOLbt ; bp = bp1 + 1 ;
		       } ;
		     }
		      break ;
		   case V4DPI_PntType_List:
		      lp = v4im_VerifyList(NULL,ctx,dpt,0) ;
		      for(i=1;v4l_ListPoint_Value(ctx,lp,i,&valpt)>0;i++)
		       { 
//		         v4dpi_PointToString(ASCTBUF1,&valpt,ctx,0) ; fprintf(incfp,"%s\n",ASCTBUF1) ;
			 v_Msg(ctx,UCTBUF1,"@%1Pn",&valpt) ; fprintf(incfp,"%s\n",UCretASC(UCTBUF1)) ;
		       } ;
		      break ;
		 } ;
		fclose(incfp) ;
		goto include_entry ;		
	      case _Recognize:	/* RECOGNIZE 'pattern' newstring */
		rcgb = v4mm_AllocChunk(sizeof *rcgb,FALSE) ; rcgb->nrcgb = NULL ; rcgb->Flags = 0 ;
		v4lex_NextTkn(tcb,0) ;
		if (tcb->type != V4LEX_TknType_String && tcb->type != V4LEX_TknType_Keyword)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdKWStr",tcb->type) ; goto fail ; } ;
//		 v4_error(0,tcb,"V4","Eval","INVREGEXP","Expecting RECOGNIZE pattern-string substitution-string") ;
//		UCstrcpyToASC(ASCTBUF1,tcb->UCstring) ;
		UCUTF16toUTF8(ASCTBUF1,V4DPI_AlphaVal_Max,tcb->UCstring,UCstrlen(tcb->UCstring)) ;
		v4lex_NextTkn(tcb,V4LEX_Option_PatternSub) ;
		if (tcb->type != V4LEX_TknType_String && tcb->type != V4LEX_TknType_Keyword)
		 { if (tcb->opcode == V_OpCode_BackQuote)
		    { i = v4dpi_PointParse(ctx,&isctpt,tcb,V4DPI_PointParse_RetFalse) ;	/* Parse the isct */
		      if (!i) goto fail ;
		      dpt = v4dpi_IsctEval(&valpt,&isctpt,ctx,0,NULL,NULL) ;
		      if (dpt == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIsctFail",&isctpt) ; goto fail_full ; } ;
		      v4im_GetPointUC(&i,tcb->UCstring,V4LEX_Tkn_StringMax,&valpt,ctx) ;
		    } else { v_Msg(ctx,ctx->ErrorMsgAux,"CmdKWStr",tcb->type) ; goto fail ; } ;
		 } ;
//		 v4_error(0,tcb,"V4","Eval","INVREGEXP","Expecting RECOGNIZE pattern-string substitution-string") ;
		if (UCstrlen(tcb->UCstring) >= UCsizeof(rcgb->subbuf)) tcb->UCstring[UCsizeof(rcgb->subbuf)] = UCEOS ;
		UCstrcpy(rcgb->subbuf,tcb->UCstring) ;
		
		for(looper=TRUE;looper;)
		 { 
		   switch (v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
//		   switch (v4eval_NextKeyWord(recoglist,tcb,EOLOK))
		    { default: v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,cx) ; goto fail ;
//				v4_error(0,tcb,"V4","Eval","INVRECOGNIZEATTR","Unknown RECOGNIZE attribute: %s",tcb->keyword) ;
		      case _EndOfLine_:		looper = FALSE ; break ;
		      case _SemiColon_:		if (cnt == 1) cnt++ ; looper = FALSE ; break ;
		      case _Trace:		rcgb->Flags |= V4DPI_RegBlock_Trace ; break ;
		      case _Evaluate:		rcgb->Flags |= V4DPI_RegBlock_Eval ; break ;
		      case _NoCase:		rcgb->Flags |= V4DPI_RegBlock_IgnCase ; break ;
		    } ;
		 } ;
		if (i=vregexp_RegComp(&rcgb->rpb,ASCTBUF1,(REG_EXTENDED | ((rcgb->Flags & V4DPI_RegBlock_IgnCase) ? REG_ICASE : 0))))
		 { vregexp_Error(i,&rcgb->rpb,ctx->ErrorMsgAux,sizeof ctx->ErrorMsgAux) ;
		   v_Msg(ctx,ctx->ErrorMsgAux,"RegExpPattern",ASCTBUF1) ; goto fail ;
//		   v4_error(0,tcb,"V4","Eval","INVREGEXP","Invalid regular expression: %s",UCretASC(ctx->ErrorMsgAux)) ;
		 } ;
		if (gpi->rcgb == NULL) { gpi->rcgb = rcgb ; }
		 else { struct V4DPI__RecogBlock *lrcgb ;
			for(lrcgb=gpi->rcgb;;lrcgb=lrcgb->nrcgb) { if (lrcgb->nrcgb == NULL) break ; } ;
			lrcgb->nrcgb = rcgb ;
		      } ;
		break ;
	      case _I:
	      case _Include:	/* INCLUDE file [MACRO name] */
		if (gpi->RestrictionMap & V_Restrict_FileRead) { v_Msg(ctx,ctx->ErrorMsgAux,"RestrictCmd") ; goto fail ; } ;
		v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
		if (tcb->opcode == V_OpCode_BackQuote)		/* Have to eval a point to get file name */
		 { i = v4dpi_PointParse(ctx,&isctpt,tcb,V4DPI_PointParse_RetFalse) ;	/* Parse the isct */
		   if (!i) goto fail ;
		   dpt = v4dpi_IsctEval(&valpt,&isctpt,ctx,0,NULL,NULL) ;
		   if (dpt == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIsctFail",&isctpt) ; goto fail_full ; } ;
		   v4im_GetPointUC(&i,UCTBUF1,V4TMBufMax,&valpt,ctx) ;
		   if (!i) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotStr",&isctpt,&valpt) ; goto fail ; } ;
		   if (UCstrchrV(UCTBUF1,UCEOLbt) != NULL)			/* Do we have multiple "lines" (probably from BigText?) */
		    { UCCHAR *b, *b1 ; INDEX fid,fx ;
//		      fp = v_MakeOpenTmpFile(UClit("v4ec"),filename,UCsizeof(filename),"w",ctx->ErrorMsgAux) ; if (fp == NULL) goto fail ;
//		      for(b=UCTBUF1;;b=b1+1)
//		       { b1 = UCstrchr(b,UCEOLbt) ; if (b1 != NULL) *b1 = UCEOS ;
//			 fprintf(fp,"%s\n",b) ; if (b1 == NULL) break ;
//		       } ; fclose(fp) ;
		      v_MakeOpenTmpFile(UClit("v4ec"),filename,UCsizeof(filename),NULL,ctx->ErrorMsgAux) ;
		      if ((fid=vout_OpenStreamFile(NULL,filename,UClit("tmp"),NULL,FALSE,V4L_TextFileType_UTF16,0,ctx->ErrorMsgAux)) == UNUSED) goto fail ;
		      fx = vout_FileIdToFileX(fid) ;
		      for(b=UCTBUF1;;b=b1+1)
		       { b1 = UCstrchrV(b,UCEOLbt) ; if (b1 != NULL) *b1 = UCEOS ;
		         vout_UCTextFileX(fx,0,b) ; vout_NLFileX(fx) ; if (b1 == NULL) break ;
		       } ;
		      if (!vout_CloseFile(fid,UNUSED,ctx->ErrorMsgAux)) goto fail ;
		      if (!v_UCFileOpen(&UCFile,filename,UCFile_Open_Read,TRUE,ctx->ErrorMsgAux,0)) goto fail ;
		      v4lex_NestInput(tcb,&UCFile,filename,V4LEX_InpMode_TempFile) ;
		     break ;
		    } else
		    { v4lex_NestInput(tcb,NULL,UCTBUF1,V4LEX_InpMode_String) ;
		    }
		 } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
		v_ParseFileName(tcb,filename,NULL,UClit(".v4"),&i) ;
include_entry:
		tpo = NULL ; i = FALSE ;
		if (UCstrcmpIC(filename,UClit("V4.v4")) == 0)
		 { i = TRUE ; UCstrcpy(filename,v_GetV4HomePath(UClit("v4kernel.v4i"))) ; } ;
		if (!v_UCFileOpen(&UCFile,filename,UCFile_Open_Read,TRUE,ctx->ErrorMsgAux,0))
		 { if (!i)		/* Appended .v4 extension? */
		    { UCstrcat(filename,UClit("b")) ;		/* Try for .v4b !*/
		      if (!v_UCFileOpen(&UCFile,filename,UCFile_Open_Read,TRUE,ctx->ErrorMsgAux,0))
		       { filename[UCstrlen(filename)-1] = UClit('c') ;		/* Look for .v4c extension (compressed) */
			 if (!v_UCFileOpen(&UCFile,filename,UCFile_Open_Read,TRUE,ctx->ErrorMsgAux,0))
			  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNoInclude",errno,filename) ; goto fail ; } ;
		       } ;
		    } else { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNoInclude",errno,filename) ; goto fail ; } ;
		 } ;
		{ struct stat statbuf ; fstat(UCFile.fd,&statbuf) ;		/* Get size of the file */
		  if (statbuf.st_size == 0)
		   { v_Msg(ctx,UCTBUF1,"FileEmpty",filename) ; vout_UCText(VOUT_Warn,0,UCTBUF1) ; } ;
		}
		filetype = V4LEX_InpMode_File ;
		ucb = UCstrrchr(filename,'.') ;				/* Look for extension */
		if (ucb != NULL)
		 { if (UCstrcmpIC(ucb+1,UClit("V4C")) == 0)
		    { filetype = V4LEX_InpMode_CmpFile ;		/* Reopen file as binary */
		      v_UCFileClose(&UCFile) ; v_UCFileOpen(&UCFile,filename,UCFile_Open_ReadBin,TRUE,ctx->ErrorMsgAux,0) ;
		    } ;
		 } ;
		i = V4LEX_TCBMacro_Tab ; tvlt = NULL ; filter = NULL ; j = FALSE ; stptr = NULL ;
		for(looper=TRUE;looper;)
		 { 
		   switch (v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
		    { default: v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,cx) ; goto fail ;
		      case _EndOfLine_:		looper=FALSE ; break ;
		      case _SemiColon_:		if (cnt == 1) cnt++ ; looper=FALSE ; break ;
		      case _Macro:		v4lex_NextTkn(tcb,0) ; if (tcb->type != V4LEX_TknType_Keyword) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotName",tcb->type,DE(Macro)) ; goto fail ; } ;
						if (UCstrlen(tcb->UCkeyword) >= UCsizeof(tpo->Macro)) { v_Msg(ctx,ctx->ErrorMsgAux,"TknMacName",tcb->UCkeyword,UCsizeof(tpo->Macro)) ; goto fail ; } ;
						INITTPO ; UCstrcpy(tpo->Macro,tcb->UCkeyword) ;
						v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;		 /* Look for ",macro" */
						if (tcb->opcode != V_OpCode_Comma) { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; break ; } ;
						v4lex_NextTkn(tcb,0) ; if (tcb->type != V4LEX_TknType_Keyword) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotName",tcb->type,DE(Macro)) ; goto fail ; } ;
						if (UCstrlen(tcb->UCkeyword) >= UCsizeof(tpo->Macro)) { v_Msg(ctx,ctx->ErrorMsgAux,"TknMacName",tcb->UCkeyword,UCsizeof(tpo->Macro)) ; goto fail ; } ;
						UCstrcpy(tpo->BgnMacro,tpo->Macro) ;	/* First macro was really the begin macro */
						UCstrcpy(tpo->Macro,tcb->UCkeyword) ;
						break ;
		      case _Goto:		j = TRUE ; break ;
		      case _Header:		v4lex_NextTkn(tcb,0) ; if (tcb->type != V4LEX_TknType_Integer) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;
						INITTPO ; tpo->HeaderLines = tcb->integer ; /* i |= V4LEX_TCBMacro_Header ; */ break ;
		      case _CSV:		INITTPO ; tpo->Delim = ',' ; /* i &= ~V4LEX_TCBMacro_Tab ; i |= V4LEX_TCBMacro_CSV ; */ break ;
		      case _Tab:		INITTPO ; tpo->Delim = '\t' ; /* i &= ~V4LEX_TCBMacro_CSV ; i |= V4LEX_TCBMacro_Tab ; */ break ;
		      case _Embedded:		INITTPO ; tpo->isEmbed = TRUE ; /* i |= V4LEX_TCBMacro_Auto ; */ break ;
		      case _Table:		v4lex_NextTkn(tcb,0) ; tvlt = v4eval_GetTable(ctx,tcb->UCkeyword,NULL) ;
						if (tvlt == NULL) goto fail ;
						if (tpo != NULL) { v_Msg(ctx,UCTBUF1,"CmdIncDflts",DE(Include),DE(Table),tcb->UCkeyword) ; vout_UCText(VOUT_Warn,0,UCTBUF1) ; } ;
						INITTPO ; *tpo = tvlt->tpoT ;		/* Copy static Table options into current one */
						tvlt->tpo = tpo ;			/* Also remember this in Table (we will have same info in tcb->ilvl[].tpo & tvlt->tpo) */
						break ;
		      case _XML:		i |= V4LEX_TCBMacro_XML ; break ;
		      case _Blanks:		INITTPO ; tpo->MinLength = 1 ; /* i |= V4LEX_TCBMacro_IgBlanks ; */ break ;
		      case _Continued:		INITTPO ; tpo->readLineOpts = V4LEX_TCBMacro_Continued ; /* i |= V4LEX_TCBMacro_Continued ; */ break ;
		      case _Filter:		if (!v4dpi_PointParse(ctx,&isctpt,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
						filter = &isctpt ; break ;
		      case _Com:
		      case _Comment:		INITTPO ; v4lex_NextTkn(tcb,0) ;
						if (UCstrlen(tcb->UCstring) >= UCsizeof(tpo->Comment)) v4_error(0,tcb,"V4","Eval","TOOLONG","Too many comment characters specified") ;
						UCstrcpy(tpo->Comment,tcb->UCstring) ;
						break ;
		      case _Delimiter:		INITTPO ; v4lex_NextTkn(tcb,0) ; tpo->Delim = tcb->UCstring[0] ; break ;
		      case _Minimum:		INITTPO ; v4lex_NextTkn(tcb,0) ;
						if (tcb->type != V4LEX_TknType_Integer) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;
						tpo->MinLength = tcb->integer ; break ;
		      case _Section:		stptr = v4mm_AllocUC(V4LEX_Tkn_ArgLineMax) ; ZUS(stptr) ;
						v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
						for (;;)
						 { if (tcb->type == V4LEX_TknType_Keyword) { UCstrcpy(tcb->UCstring,tcb->UCkeyword) ; }
						    else if (tcb->type != V4LEX_TknType_String){ v_Msg(ctx,ctx->ErrorMsgAux,"CmdKWStr",tcb->type) ; goto fail ; } ;
						   UCstrcat(stptr,tcb->UCstring) ;	/* Take remainder of line */
						   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
						   if (tcb->opcode != V_OpCode_Comma) { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; break ; } ;
						   UCstrcat(stptr,UClit(",")) ;
						   v4lex_NextTkn(tcb,0) ;
						 } ;
						break ;
		    } ;
		 } ;
/*		If 'Goto' specified then close off current level before nesting into Include file */
		if (j)
		 v4lex_CloseOffLevel(tcb,0) ;
		if (i & V4LEX_TCBMacro_XML)
		 { i = v4lex_ReadXMLFile(ctx,filename,&UCFile,V4LEX_ReadXML_ValueErr|V4LEX_ReadXML_LimitErr,&tcb->tcb_lines,&tcb->tcb_tokens) ;
		   if (i != V4LEX_ReadXML_OK)
		    goto fail ;
		   v_UCFileClose(&UCFile) ; break ;
		 } ;
		v4lex_NestInput(tcb,&UCFile,filename,(cx == _Data ? V4LEX_InpMode_TempFile : filetype)) ;
		tcb->ilvl[tcb->ilx].lvlSections = stptr ;
		INITTPO ; tcb->ilvl[tcb->ilx].tpo = tpo ;
		if (filter != NULL)					/* If got a filter then save it */
		 { tpo->filterPt = (P *)v4mm_AllocChunk(filter->Bytes,FALSE) ;
		   memcpy(tpo->filterPt,filter,filter->Bytes) ;
		 } ;
		if (tpo->Macro[0] != UCEOS || tvlt != NULL)
		 { 
		   tcb->ilvl[tcb->ilx].vlt = tvlt ; gpi->vltCur = tvlt ;	/* Link vlt to ctx (may change with TABLE columns) */
		 } ;
		break ;
	      case _Point:	/* POINT dim value */
		v4lex_NextTkn(tcb,0) ;
		diminfo = v4dpi_DimInfoGet(ctx,v4dpi_DimGet(ctx,tcb->UCkeyword,DIMREF_POINT)) ;
		if (diminfo == NULL)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntNotDim",tcb->type,tcb->UCkeyword) ; goto fail ;
		 } ;
		if ((diminfo->Flags & V4DPI_DimInfo_Dim) != 0)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntMakeDim",tcb->type,(cx=_Dimension),V4IM_OpCode_MakeP,Dim_Dim,Dim_Dim) ; goto fail ;
		 } ;
		for(looper=TRUE;looper;)
		 { v4lex_NextTkn(tcb,(diminfo->PointType==V4DPI_PntType_Int ? V4LEX_Option_NegLit : V4LEX_Option_ForceKW)+V4LEX_Option_RetEOL) ;
		   switch(tcb->type)
		    { default:
			v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntValue",tcb->type,tcb->UCstring) ; goto fail ;
		      case V4LEX_TknType_EOL:		looper = FALSE ; continue ;
		      case V4LEX_TknType_String:	UCstrncpy(tcb->UCkeyword,tcb->UCstring,UCsizeof(tcb->UCkeyword)) ; break ;
		      case V4LEX_TknType_Keyword:	break ;
		      case V4LEX_TknType_Integer:	UCsprintf(tcb->UCkeyword,UCsizeof(tcb->UCkeyword),UClit("%d"),tcb->integer) ; break ;
		      case V4LEX_TknType_Punc:
			if (tcb->opcode == V_OpCode_Semi) { looper = FALSE ; continue ; }
			 else if (tcb->opcode == V_OpCode_Comma) { continue ; }
			 else { v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntPunc",tcb->type) ; goto fail ;
			      } ;
		    } ;
		   switch (diminfo->PointType)
		    { default:
			v_Msg(ctx,ctx->ErrorMsgAux,"DimWrongType",diminfo->DimId,V4DPI_PntType_Dict) ; goto fail ;
		      case V4DPI_PntType_XDict:
			if (gpi->xdr == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"@No EXTERNAL area has been opened") ; goto fail ; } ;
			aid = gpi->RelH[XTRHNUMDIM(diminfo->DimId)].aid ;
			if (gpi->vltCur != NULL)					/* If processing table then see if got a column spec */
			 { UCCHAR tname[V4LEX_Tkn_KeywordMax+1] ;
			   UCstrcpy(tname,tcb->UCkeyword) ; v4lex_NextTkn(tcb,0) ;	/* Save and get rid of "*" following */
			   for(i=0;i<gpi->vltCur->Columns;i++)
			    { if (UCstrcmpIC(gpi->vltCur->Col[i].Name,tname) == 0) break ; } ;
			   if (i < gpi->vltCur->Columns)			/* Found it - copy current value into point */
			    { if (gpi->vltCur->Col[i].Cur == NULL || (gpi->vltCur->InXML ? (gpi->vltCur->Col[i].XMLFlags & V4LEX_Table_XML_Defined)==0 : FALSE))
			       { v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntColUnd",tcb->type,i+1,tname,gpi->vltCur->Name) ; goto fail ; } ;
			      num = gpi->vltCur->Col[i].Cur->Value.IntVal ;
			    } else
			    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColUndef",tname) ; goto fail ; } ;
			   if (diminfo->UniqueOK == V4DPI_DimInfo_UOkPoint)
			    { if (!v4dpi_AddDUPoint(ctx,diminfo,NULL,num,V_DUPDICTACT_Err)) goto fail ; } ;
			   break ;
			 } ;
			if ((i = v4dpi_XDictEntryGet(ctx,tcb->UCstring,diminfo,V4DPI_XDict_MustExist)) == 0)
			 { i = v4dpi_XDictEntryGet(ctx,tcb->UCstring,diminfo,0) ;
			   if (trace & V4TRACE_Points)
			    { v_Msg(ctx,UCTBUF2,"@*  Added point (%1U) = %2d\n",tcb->UCstring,i) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ; } ;
			   if (diminfo->UniqueOK == V4DPI_DimInfo_UOkPoint)
			    { if (!v4dpi_AddDUPoint(ctx,diminfo,NULL,i,V_DUPDICTACT_Err)) goto fail ; } ;
			 } else
			 { struct V4DPI__LittlePoint xpt ;
			   ZPH(&xpt) ; xpt.Dim = diminfo->DimId ; xpt.PntType = V4DPI_PntType_XDict ; xpt.Bytes = V4PS_Int ; xpt.Value.IntVal = i ;
			   switch (gpi->dupPointAction)
			    { case V_DUPDICTACT_Err:		v_Msg(ctx,ctx->ErrorMsgAux,"CmdDupPointE",V4DPI_PntType_XDict,&xpt) ; goto fail ;
			      case V_DUPDICTACT_Ignore:		break ;
			      case V_DUPDICTACT_Undef:
			      case V_DUPDICTACT_Warn:		v_Msg(ctx,UCTBUF2,"*CmdDupPointW",V4DPI_PntType_XDict,&xpt) ;
								vout_UCText(VOUT_Warn,0,UCTBUF2) ; vout_NL(VOUT_Warn) ; break ;
			    } ;
			 } ;
			break ;
		      case V4DPI_PntType_Dict:
			if (CHECKV4INIT) goto fail ;
			aid = gpi->RelH[XTRHNUMDIM(diminfo->DimId)].aid ;
			if (gpi->vltCur != NULL)					/* If processing table then see if got a column spec */
			 { UCCHAR tname[V4LEX_Tkn_KeywordMax+1] ;
			   UCstrcpy(tname,tcb->UCkeyword) ; v4lex_NextTkn(tcb,0) ;	/* Save and get rid of "*" following */
			   for(i=0;i<gpi->vltCur->Columns;i++)
			    { if (UCstrcmpIC(gpi->vltCur->Col[i].Name,tname) == 0) break ; } ;
			   if (i < gpi->vltCur->Columns)			/* Found it - copy current value into point */
			    { if (gpi->vltCur->Col[i].Cur == NULL || (gpi->vltCur->InXML ? (gpi->vltCur->Col[i].XMLFlags & V4LEX_Table_XML_Defined)==0 : FALSE))
			       { v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntColUnd",tcb->type,i+1,tname,gpi->vltCur->Name) ; goto fail ; } ;
			      num = gpi->vltCur->Col[i].Cur->Value.IntVal ;
			    } else
			    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColUndef",tname) ; goto fail ; } ;
			   if (diminfo->UniqueOK == V4DPI_DimInfo_UOkPoint)
			    { if (!v4dpi_AddDUPoint(ctx,diminfo,NULL,num,V_DUPDICTACT_Ignore)) goto fail ; } ;
			   break ;
			 } ;
/*			Want to make it a dictionary point on Dim_NId - this really doesn't matter any more */
			i = v4dpi_DictEntryGet(ctx,Dim_NId,tcb->UCstring,diminfo,&dim) ;
			if (trace & V4TRACE_Points)
			 { v_Msg(ctx,UCTBUF2,"@*Added point (%1U) = %2d\n",tcb->UCstring,i) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ; } ;
			if (diminfo->UniqueOK == V4DPI_DimInfo_UOkPoint)
			 { if (!v4dpi_AddDUPoint(ctx,diminfo,NULL,i,gpi->dupPointAction)) goto fail ; } ;
			break ;
		      case V4DPI_PntType_CodedRange:
		      case V4DPI_PntType_Int:
//printf("  in INT\n") ;
			if (tcb->type != V4LEX_TknType_Integer)
			 { if (gpi->vltCur != NULL && tcb->type == V4LEX_TknType_Keyword)	/* If processing table then see if got a column spec */
			    { UCCHAR tname[V4LEX_Tkn_KeywordMax+1] ;
			      UCstrcpy(tname,tcb->UCkeyword) ; v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
			      for(i=0;i<gpi->vltCur->Columns;i++)
			       { if (UCstrcmpIC(gpi->vltCur->Col[i].Name,tname) == 0) break ; } ;
			      if (i < gpi->vltCur->Columns)				/* Found it - copy current value into point */
			       { if (gpi->vltCur->Col[i].Cur == NULL || (gpi->vltCur->InXML ? (gpi->vltCur->Col[i].XMLFlags & V4LEX_Table_XML_Defined)==0 : FALSE))
				  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntColUnd",tcb->type,i+1,tname,gpi->vltCur->Name) ; goto fail ; } ;
				 tcb->integer = gpi->vltCur->Col[i].Cur->Value.IntVal ;
			       } else
			       { if (tcb->opcode != V_OpCode_Star)
			          { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColUndef",tname) ; goto fail ; } ;
				 dix = v4dpi_DimInfoGet(ctx,v4dpi_DimGet(ctx,tname,DIMREF_REF)) ;
				 if (dix == NULL)
				  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotDim",tname) ; goto fail ; } ;
				 DIMVAL(dpt,ctx,dix->DimId) ;
				 if (dpt == NULL)
				  { v_Msg(ctx,ctx->ErrorMsgAux,"IsctEvalCtx",dix->DimId) ; goto fail ; } ;
				 tcb->integer = dpt->Value.IntVal ;
			       } ;
			    } else
			    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ;
			    } ;
			 } ;
			if (diminfo->UniqueOK != V4DPI_DimInfo_UOkPoint)
			 { v_Msg(ctx,UCTBUF2,"@*Dimension (%1D) does not have \"PointCreate POINT\" attribute\n",diminfo->DimId) ;
			   vout_UCText(VOUT_Err,0,UCTBUF2) ;
			   break ;
			 } ;
			if (!v4dpi_AddDUPoint(ctx,diminfo,NULL,tcb->integer,gpi->dupPointAction)) goto fail ;
			break ;
		      case V4DPI_PntType_Real:
			if (!(tcb->type == V4LEX_TknType_Integer || tcb->type == V4LEX_TknType_Float))
			 { if (gpi->vltCur != NULL && tcb->type == V4LEX_TknType_Keyword)	/* If processing table then see if got a column spec */
			    { UCCHAR tname[V4LEX_Tkn_KeywordMax+1] ;
			      UCstrcpy(tname,tcb->UCkeyword) ; v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
			      for(i=0;i<gpi->vltCur->Columns;i++)
			       { if (UCstrcmpIC(gpi->vltCur->Col[i].Name,tname) == 0) break ; } ;
			      if (i < gpi->vltCur->Columns)				/* Found it - copy current value into point */
			       { if (gpi->vltCur->Col[i].Cur == NULL || (gpi->vltCur->InXML ? (gpi->vltCur->Col[i].XMLFlags & V4LEX_Table_XML_Defined)==0 : FALSE))
				  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntColUnd",tcb->type,i+1,tname,gpi->vltCur->Name) ; goto fail ; } ;
				 dpt = gpi->vltCur->Col[i].Cur ;
			       } else
			       { if (tcb->opcode != V_OpCode_Star)
			          { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColUndef",tname) ; goto fail ; } ;
				 dix = v4dpi_DimInfoGet(ctx,v4dpi_DimGet(ctx,tname,DIMREF_REF)) ;
				 if (dix == NULL)
				  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotDim",tname) ; goto fail ; } ;
				 DIMVAL(dpt,ctx,dix->DimId) ;
				 if (dpt == NULL)
				  { v_Msg(ctx,ctx->ErrorMsgAux,"IsctEvalCtx",dix->DimId) ; goto fail ; } ;
			       } ;
			    } else
			    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ;
			    } ;
			 } else
			 { dpt = &point ; dblPNTv(dpt,tcb->floating) ;
			 } ;
			if (diminfo->UniqueOK != V4DPI_DimInfo_UOkPoint)
			 { v_Msg(ctx,UCTBUF2,"@*Dimension (%1D) does not have \"PointCreate POINT\" attribute\n",diminfo->DimId) ;
			   vout_UCText(VOUT_Err,0,UCTBUF2) ;
			   break ;
			 } ;
			if (!v4dpi_AddDUPoint(ctx,diminfo,dpt,UNUSED,gpi->dupPointAction)) goto fail ;
			break ;
		    } ;
		 } ;
		break ;
	      case _EqualSign_:	/* = expression */
		if (CHECKV4INIT) goto fail ;
		ZUS(gpi->SavedErrMsg) ;
		if (!v4dpi_PointParse(ctx,&isctpt,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
		if (!v4eval_ChkEOL(tcb)) goto fail ;
		dpt = v4dpi_IsctEval(&point,&isctpt,ctx,0,NULL,NULL) ;
		if (dpt == NULL)
		 { gpi->ErrCount++ ; v4trace_ExamineState(ctx,&isctpt,V4_EXSTATE_All,VOUT_Err) ; }
		 else { UCCHAR result[V4CTX_MaxPointOutputMax],tbuf[V4CTX_MaxPointOutputMax] ;
			v_Msg(ctx,result,"@%1Q",dpt) ;
			for(i=0,j=0;i>=0&&j<V4CTX_MaxPointOutputMax-2;i++)
			 { if (vuc_IsPrint(result[i])) { tbuf[j++] = result[i] ; continue ; } ;
			   switch(result[i])
			    { default:		tbuf[j++] = UClit('.') ; break ;
			      case UCEOS:	tbuf[j++] = UCEOS ; i = -999 ; break ;
			      case UClit('\n'):	tbuf[j++] = UClit('\\') ; tbuf[j++] = UClit('n') ; break ;
			      case UClit('\r'):	tbuf[j++] = UClit('\\') ; tbuf[j++] = UClit('r') ; break ;
			      case UClit(' '):	tbuf[j++] = UClit(' ') ; break ;
			      case UClit('\t'):	tbuf[j++] = UClit('\\') ; tbuf[j++] = UClit('t') ; break ;
			    } ;
			 } ;
			UCstrcat(tbuf,UClit("\n")) ; vout_UCText(VOUT_Trace,0,tbuf) ;
		      } ;
		break ;
	      case _Bucket:	/* BUCKET n */
		v4lex_NextTkn(tcb,0) ;
		if (!v4eval_ChkEOL(tcb)) goto fail ;
		v4is_ExamineBkt(tcb->integer,gpi->RelH[gpi->HighHNum].aid) ;
		break ;
	      case _Table:	/* TABLE name options */
		if (gpi->SaveTables)
		 { stbt = (struct V4LEX__BigText *)v4mm_AllocChunk(sizeof *stbt,FALSE) ; ZUS(stbt->BigBuf) ;
		   if (tcb->appendto == NULL)
		    { tcb->appendto = stbt->BigBuf ; tcb->appendopt |= V4LEX_AppendTo_LineTerm ;
		      tcb->maxappend = sizeof stbt->BigBuf ; tcb->bytesappend = 0 ;
		      for(i=UCstrlen(tcb->ilvl[tcb->ilx].input_str)-1;i>0;i--)
		       { if (tcb->ilvl[tcb->ilx].input_str[i] > UClit(' ')) break ; tcb->ilvl[tcb->ilx].input_str[i] = UCEOS ; } ;
		      UCstrcat(tcb->appendto,tcb->ilvl[tcb->ilx].input_str) ;	/* Assume TABLE command in current string */
		      UCstrcat(tcb->appendto,UCEOLbts) ; stptr = NULL ;
		    } else
		    { stptr = tcb->appendto + UCstrlen(tcb->appendto) - UCstrlen(tcb->ilvl[tcb->ilx].input_str) - 1 ; /* Save pointer in appendto where current string starts */
		    } ;
		 } ;
		if (vlt != NULL)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdTblNest",(kwx=_Table)) ; goto table_fail ; } ;
		vlt = (struct V4LEX__Table *)v4mm_AllocChunk(sizeof *vlt,TRUE) ;
		v4lex_NextTkn(tcb,0) ;
		switch(tcb->type)
		 { default:	v_Msg(ctx,ctx->ErrorMsgAux,"CmdKWStr",tcb->type) ; goto table_fail ;
		   case V4LEX_TknType_Keyword:
		   case V4LEX_TknType_String:	break ;
		 } ;
		if (UCstrlen(tcb->UCstring) >= UCsizeof(vlt->Name))
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdTblName",tcb->type,(kwx=_Table),(sizeof vlt->tpoT.Macro)-1) ; goto table_fail ; } ;
		UCstrcpy(vlt->Name,tcb->UCstring) ;
		vlt->IsDefault = UCstrcmpIC(vlt->Name,UClit("DEFAULTS")) == 0 ;
		if (vlt->IsDefault && gpi->vlt != NULL)
		 { for(tvlt=gpi->vlt;tvlt!=NULL;tvlt=tvlt->link)
		    { if (tvlt->IsDefault) break ; } ;
		   if (tvlt != NULL) { v4mm_FreeChunk(vlt) ; vlt = tvlt ; } ;	/* Only want 1 defaults table */
		 } ;
		vlt->tpoT.Delim = '\t' ; vlt->tpoT.MinLength = UNUSED ;
		for(looper=TRUE;looper;)
		 { 
		   switch(kwx=v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))

		    { default: v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,cx) ; goto table_fail ;
		      case _EndOfLine_: /* EOL */	looper=FALSE ; break ;
		      case _SemiColon_: /* ; */		if (cnt == 1) cnt++ ; looper=FALSE ; break ;
		      case _Dim:
			for (;;)
			 { v4lex_NextTkn(tcb,0) ; if (tcb->type != V4LEX_TknType_Keyword) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotName",tcb->type,DE(Dim)) ; goto table_fail ; } ;
			   diminfo = v4dpi_DimInfoGet(ctx,v4dpi_DimGet(ctx,tcb->UCkeyword,DIMREF_REF)) ;
			   if (diminfo == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdDimName1",tcb->type) ; goto table_fail ; } ;
			   if (vlt->dimCount >= V4LEX_Table_DimMax) { v_Msg(ctx,ctx->ErrorMsgAux,"TableMaxDim",tcb->type,V4LEX_Table_DimMax) ; goto table_fail ; } ;
			   vlt->dimIds[vlt->dimCount++] = diminfo->DimId ;
			   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
			   if (tcb->opcode != V_OpCode_Comma)  { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; break ; } ;
			 } ;
			break ;
		      case _Macro: /* MACRO name [,macro] */
			v4lex_NextTkn(tcb,0) ; if (tcb->type != V4LEX_TknType_Keyword) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotName",tcb->type,DE(Macro)) ; goto table_fail ; } ;
			if (UCstrlen(tcb->UCkeyword) >= UCsizeof(vlt->tpoT.Macro)) { v_Msg(ctx,ctx->ErrorMsgAux,"TknMacName",tcb->UCkeyword,UCsizeof(vlt->tpoT.Macro)) ; goto table_fail ; } ;
			UCstrcpy(vlt->tpoT.Macro,tcb->UCkeyword) ;
			v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;		 /* Look for ",macro" */
			if (tcb->opcode != V_OpCode_Comma) { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; break ; } ;
			v4lex_NextTkn(tcb,0) ; if (tcb->type != V4LEX_TknType_Keyword) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotName",tcb->type,DE(Macro)) ; goto table_fail ; } ;
			if (UCstrlen(tcb->UCkeyword) >= UCsizeof(vlt->tpoT.Macro)) { v_Msg(ctx,ctx->ErrorMsgAux,"TknMacName",tcb->UCkeyword,UCsizeof(vlt->tpoT.Macro)) ; goto table_fail ; } ;
			UCstrcpy(vlt->tpoT.BgnMacro,vlt->tpoT.Macro) ;	/* First macro was really the begin macro */
			UCstrcpy(vlt->tpoT.Macro,tcb->UCkeyword) ;
			break ;
		      case _Header: /* HEADER num */
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Integer)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto table_fail ; } ;
			vlt->tpoT.HeaderLines = tcb->integer ; break ;
		      case _CSV: /* CSV */
			vlt->tpoT.Delim = ',' ; break ;
		      case _Tab: /* TAB */
			vlt->tpoT.Delim = '\t' ; break ;
		      case _Com: /* COMMENT "x" */
		      case _Comment: /* COMMENT "x" */
			v4lex_NextTkn(tcb,0) ;
			if (UCstrlen(tcb->UCstring) >= UCsizeof(vlt->tpoT.Comment))
			 v4_error(0,tcb,"V4","Eval","TOOLONG","Too many comment characters specified") ;
			UCstrcpy(vlt->tpoT.Comment,tcb->UCstring) ;
			break ;
		      case _Delimiter: /* DELIMITER "x" */
			v4lex_NextTkn(tcb,0) ;
			vlt->tpoT.Delim = tcb->UCstring[0] ;
			break ;
		      case _Prefix:
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Keyword || UCstrlen(tcb->UCkeyword) >= sizeof vlt->colNamePrefix)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCstring,DE(Prefix)) ; goto table_fail ; } ;
			UCstrcpy(vlt->colNamePrefix,tcb->UCkeyword) ;
			break ;
		      case _Suffix:
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Keyword || UCstrlen(tcb->UCkeyword) >= sizeof vlt->colNameSuffix)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCstring,DE(Suffix)) ; goto table_fail ; } ;
			UCstrcpy(vlt->colNameSuffix,tcb->UCkeyword) ;
			break ;
		      case _Other:
			if (vlt->otherList != NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"TableOptDef",tcb->type,DE(Other)) ; goto table_fail ; } ;
			if (!v4im_vltParseOther(ctx,vlt,tcb)) goto table_fail ;
			break ;
		      case _Minimum: /* MINIMUM characters */
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Integer)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto table_fail ; } ;
			vlt->tpoT.MinLength = tcb->integer ; break ;
		    } ;
		 } ;
		break ;
/*		If we fail in table then may have to clean up some stuff */
table_fail:
		if (gpi->SaveTables) { tcb->appendto = NULL ; } ;
		v4mm_FreeChunk(vlt) ; vlt = NULL ;
		gpi->vlt = NULL ;		/* VEH140107 - disable all tables - we don't know the state of anything if an error */
		goto fail ;
	      case _Col:
	      case _Column:	/* COLUMN name dimension attributes */
top_column:
		if (vlt == NULL)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInTbl",(kwx=_Column),(kwx2=_Table)) ; goto column_fail ; } ;
		if (vlt->Columns >= V4LEX_Table_ColMax-1)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColMax",tcb->type,V4LEX_Table_ColMax,vlt->Name) ; goto column_fail ; } ;
		if (!vlt->IsDefault)				/* Defaults table does not have column name */
		 { v4lex_NextTkn(tcb,V4LEX_Option_ForceKW) ;
		   if (tcb->type != V4LEX_TknType_Keyword)
		    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotName",tcb->type,DE(Column)) ; goto column_fail ; } ;
		   tcb->UCstring[UCsizeof(vlt->Col[vlt->Columns].Name)-1] = '\0' ; UCstrcpy(vlt->Col[vlt->Columns].Name,tcb->UCstring) ;
		 } ;
		v4lex_NextTkn(tcb,0) ;
		if (tcb->type != V4LEX_TknType_Keyword)		/* If dimension is "*" then use column name */
		 { if (tcb->opcode == V_OpCode_NCStar && !vlt->IsDefault)
		    { UCstrcpy(tcb->UCkeyword,vlt->Col[vlt->Columns].Name) ; }
	 	    else { v_Msg(ctx,ctx->ErrorMsgAux,"CmdDimName1",tcb->type) ; goto column_fail ; } ;
		 } ;

		diminfo = v4dpi_DimInfoGet(ctx,v4dpi_DimGet(ctx,tcb->UCkeyword,DIMREF_REF)) ;
		if (diminfo == NULL)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdDimName1",tcb->type) ; goto column_fail ; } ;
/*		Now that we have Column dimension see if name has prefix or suffix */
		if (UCnotempty(vlt->colNamePrefix) || UCnotempty(vlt->colNameSuffix))
		 { UCstrcpy(UCTBUF1,vlt->colNamePrefix) ; UCstrcat(UCTBUF1,vlt->Col[vlt->Columns].Name) ; UCstrcat(UCTBUF1,vlt->colNameSuffix) ;
		   if (UCstrlen(UCTBUF1) >= UCsizeof(vlt->Col[vlt->Columns].Name))
		    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdTblPrefix",UCTBUF1,UCstrlen(UCTBUF1),UCsizeof(vlt->Col[vlt->Columns].Name)-1) ; goto column_fail ; } ;
		   UCstrcpy(vlt->Col[vlt->Columns].Name,UCTBUF1) ;
		 } ;
		vlt->Col[vlt->Columns].PntTypeInfo = UNUSED ; vlt->Col[vlt->Columns].PntType1 = UNUSED ;
		vlt->Col[vlt->Columns].Decimals = UNUSED ; vlt->Col[vlt->Columns].Width = UNUSED ; vlt->Col[vlt->Columns].inclusionMask = UNUSED ;
		switch (diminfo->PointType)			/* Assume most columns can be quoted (except alpha, dictionary, external) */
		 { default:	vlt->Col[vlt->Columns].ColumnType = V4LEX_TableCT_Quoted ; break ;
		   case V4DPI_PntType_Dict:
		   case V4DPI_PntType_XDict:
		   case V4DPI_PntType_List:
		   CASEofChar
			vlt->Col[vlt->Columns].ColumnType = 0 ; break ;
		 } ;
		if (!vlt->IsDefault)				/* If this is NOT defaults, then look for one */
		 { for(tvlt=gpi->vlt;tvlt!=NULL;tvlt=tvlt->link)
		    { if (tvlt->IsDefault) break ; } ;
		   if (tvlt != NULL)				/* If not NULL, then found defaults */
		    { for(i=0;i<tvlt->Columns;i++)
		       { if (tvlt->Col[i].di == diminfo)	/* Matched dimensions? */
		          { UCCHAR tname[V4DPI_DimInfo_DimNameMax+1] ;
		            UCstrcpy(tname,vlt->Col[vlt->Columns].Name) ;
			    vlt->Col[vlt->Columns] = tvlt->Col[i] ; UCstrcpy(vlt->Col[vlt->Columns].Name,tname) ;
			    vlt->Col[vlt->Columns].ColumnType |= V4LEX_TableCT_Kernel ;
			    break ;
			  } ;
		       } ;
		    } ;
		 } ;
		vlt->Col[vlt->Columns].di = diminfo ; vlt->Col[vlt->Columns].PntType = diminfo->PointType ;
		vlt->Col[vlt->Columns].StartX = UNUSED ; vlt->Col[vlt->Columns].EndX = UNUSED ;
		more = FALSE ;
		for(looper=TRUE;looper && vlt!=NULL;)
		 { 
		   switch (kwx=v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
		    { default:	v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,cx) ; goto column_fail ;
		      case _EndOfLine_:		looper = FALSE ; break ;
		      case _Comma_:		more = TRUE ; looper=FALSE ; break ;
		      case _SemiColon_:		if (cnt == 1) cnt++ ; looper=FALSE ; break ;
		      case _Acceptor:	/* ACCEPTOR isct */
			if (!v4dpi_PointParse(ctx,&isctpt,tcb,V4DPI_PointParse_NoMult|V4DPI_PointParse_RetFalse)) goto column_fail ;
			if ((vtt = vlt->Col[vlt->Columns].vtt) == NULL)
			 vtt = (vlt->Col[vlt->Columns].vtt = (struct V4LEX__Table_Tran *)v4mm_AllocChunk(sizeof *vtt,TRUE)) ;
			vtt->acpt = (P *)v4mm_AllocChunk(isctpt.Bytes,FALSE) ; memcpy(vtt->acpt,&isctpt,isctpt.Bytes) ;
			break ;
		      case _AggKey:	 /* AGGKEY */	vlt->Col[vlt->Columns].ColumnType |= V4LEX_TableCT_AggKey ; break ;
		      case _Before:	/* BEFORE string */
			if ((vtt = vlt->Col[vlt->Columns].vtt) == NULL)
			 vtt = (vlt->Col[vlt->Columns].vtt = (struct V4LEX__Table_Tran *)v4mm_AllocChunk(sizeof *vtt,TRUE)) ;
			if (vlt->Col[vlt->Columns].ColumnType & V4LEX_TableCT_Kernel)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColKerAttr",(kwx=_Before)) ; goto column_fail ; } ;
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type == V4LEX_TknType_Keyword) { UCstrcpy(tcb->UCstring,tcb->UCkeyword) ; }
			 else if (tcb->type != V4LEX_TknType_String)
				{ v_Msg(ctx,ctx->ErrorMsgAux,"CmdKWStr",tcb->type) ; goto column_fail ; } ;
			if (vtt->UCsubstr != NULL)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColBAArg",kwx) ; goto column_fail ;
			 } ;
			vtt->UCsubstr = v4mm_AllocUC(UCstrlen(tcb->UCstring)+1) ; UCstrcpy(vtt->UCsubstr,tcb->UCstring) ;
			vtt->substrHow = V4LEX_TT_SubStr_Before ; break ;
		      case _After:	/* AFTER string */
			if ((vtt = vlt->Col[vlt->Columns].vtt) == NULL)
			 vtt = (vlt->Col[vlt->Columns].vtt = (struct V4LEX__Table_Tran *)v4mm_AllocChunk(sizeof *vtt,TRUE)) ;
			if (vlt->Col[vlt->Columns].ColumnType & V4LEX_TableCT_Kernel)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColKerAttr",(kwx=_After)) ; goto column_fail ; } ;
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type == V4LEX_TknType_Keyword) { UCstrcpy(tcb->UCstring,tcb->UCkeyword) ; }
			 else if (tcb->type != V4LEX_TknType_String)
				{ v_Msg(ctx,ctx->ErrorMsgAux,"CmdKWStr",tcb->type) ; goto column_fail ; } ;
			if (vtt->UCsubstr != NULL)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColBAArg",kwx) ; goto column_fail ;
			 } ;
			vtt->UCsubstr = v4mm_AllocUC(UCstrlen(tcb->UCstring)+1) ; UCstrcpy(vtt->UCsubstr,tcb->UCstring) ;
			vtt->substrHow = V4LEX_TT_SubStr_After ; break ;
		      case _Uppercase:	/* UPPERCASE */
			if ((vtt = vlt->Col[vlt->Columns].vtt) == NULL)
			 vtt = (vlt->Col[vlt->Columns].vtt = (struct V4LEX__Table_Tran *)v4mm_AllocChunk(sizeof *vtt,TRUE)) ;
			if (vlt->Col[vlt->Columns].ColumnType & V4LEX_TableCT_Kernel)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColKerAttr",(kwx=_Uppercase)) ; goto column_fail ; } ;
			vtt->Transforms |= V4LEX_TT_Tran_UpperCase ; break ;
		      case _Lowercase:	/* LOWERCASE */
			if ((vtt = vlt->Col[vlt->Columns].vtt) == NULL)
			 vtt = (vlt->Col[vlt->Columns].vtt = (struct V4LEX__Table_Tran *)v4mm_AllocChunk(sizeof *vtt,TRUE)) ;
			if (vlt->Col[vlt->Columns].ColumnType & V4LEX_TableCT_Kernel)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColKerAttr",(kwx=_Lowercase)) ; goto column_fail ; } ;
			vtt->Transforms |= V4LEX_TT_Tran_LowerCase ; break ;
		      case _Mask:
			vlt->Col[vlt->Columns].inclusionMask = 0 ;
			for(;;)
			 { v4lex_NextTkn(tcb,FALSE) ;
			   if (tcb->type != V4LEX_TknType_Integer) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColMask",tcb->type,DE(Mask)) ; goto column_fail ; } ;
			   if (tcb->integer < 1 || tcb->integer > 32 || tcb->decimal_places > 0)
			    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColMask",tcb->type,DE(Mask)) ; goto column_fail ; } ;
			   vlt->Col[vlt->Columns].inclusionMask |= (FLAGS32)(1<<(tcb->integer-1)) ;
			   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ; if (tcb->opcode != V_OpCode_Comma) break ;
			 } ;
			v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; break ;
		      case _Point: /* POINT pt */
			vlt->Col[vlt->Columns].ColumnType |= V4LEX_TableCT_Point ;
			vlt->Col[vlt->Columns].Cur = PTFROMHEAP ;
			if (!v4dpi_PointParse(ctx,vlt->Col[vlt->Columns].Cur,tcb,V4DPI_PointParse_NoMult|V4DPI_PointParse_RetFalse)) goto column_fail ;
//			if (vlt->Col[vlt->Columns].Cur->PntType != V4DPI_PntType_Char && vlt->Col[vlt->Columns].Cur->PntType != V4DPI_PntType_UCChar)
//			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdKWStr",tcb->type) ; goto column_fail ; } ;
			break ;
		      case _Expression:	/* Expression pt */
			vlt->Col[vlt->Columns].ColumnType |= V4LEX_TableCT_Expression ;
			vlt->Col[vlt->Columns].Cur = PTFROMHEAP ;
			if (!v4dpi_PointParse(ctx,vlt->Col[vlt->Columns].Cur,tcb,V4DPI_PointParse_NoMult|V4DPI_PointParse_RetFalse)) goto column_fail ;
			break ;
		      case _Missing:
			dpt = PTFROMHEAP ;
			if (!v4dpi_PointParse(ctx,dpt,tcb,V4DPI_PointParse_NoMult|V4DPI_PointParse_RetFalse)) goto column_fail ;
			v4dpi_PointToStringML(UCTBUF1,dpt,ctx,V4DPI_FormatOpt_Echo,V4DPI_AlphaVal_Max) ;
			if (UCstrlen(UCTBUF1) + 7 >= UCsizeof(tcb->poundparam['M'-'A'].value))
			{ v_Msg(ctx,ctx->ErrorMsgAux,"CmdColMissing",DE(Missing),UCstrlen(UCTBUF1) + 7,UCsizeof(tcb->poundparam['M'-'A'].value)) ; goto column_fail ; } ;
			vlt->Col[vlt->Columns].Missing = dpt ;
			break ;
		      case _Error: /* ERROR pt */
		      case _Default: /* DEFAULT pt */
			dpt = PTFROMHEAP ;
			if (!v4dpi_PointParse(ctx,dpt,tcb,V4DPI_PointParse_NoMult|V4DPI_PointParse_RetFalse)) goto column_fail ;
			if (dpt->PntType == V4DPI_PntType_Isct ? FALSE : dpt->PntType != vlt->Col[vlt->Columns].PntType)
			  { if (dpt->PntType != V4DPI_PntType_Special)
			     { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColDimDiff",vlt->Col[vlt->Columns].di->DimId,vlt->Col[vlt->Columns].di->PointType,(kwx=_Default),dpt->Dim,dpt->PntType) ; goto column_fail ; } ;
			  } ;

			switch(dpt->PntType)
			 { default:			i = dpt->Bytes - V4DPI_PointHdr_Bytes ; break ;
			   case V4DPI_PntType_UCChar:	i = UCCHARSTRLEN(dpt) ; break ;
			   CASEofCharmU			i = CHARSTRLEN(dpt) ; break ;
			 } ;
			if (i > V4PT_MaxCharLenIn0)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColDfltMax",kwx,i,V4PT_MaxCharLenIn0) ; goto column_fail ; } ;

//			j = vlt->Col[vlt->Columns].FixedLength ;
//			switch(dpt->PntType)
//			 { case V4DPI_PntType_BinObj:
//				i = dpt->Bytes - V4DPI_PointHdr_Bytes ;
//				if (j > 0 && i != j)
//				 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColLen",DE(Column),DE(Length),j,DE(Default),i) ; goto column_fail ; } ;
//				break ;
//			   case V4DPI_PntType_UCChar:
//				i = UCCHARSTRLEN(dpt) ;
//				if (j > 0 && i != j)
//				 { if (i < j)
//				    { memset(&dpt->Value.UCVal[i+1],UClit(' '),j-i) ;
//				      UCCHARPNTBYTES2(dpt,j) ;
//				    } else { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColLen",DE(Column),DE(Length),j,DE(Default),i) ; goto column_fail ; } ;
//				 } ;
//				break ;
//			   CASEofCharmU
//				i = CHARSTRLEN(dpt) ;
//				if (j > 0 && i != j)
//				 { if (i < j)
//				    { memset(&dpt->Value.AlphaVal[i+1],' ',j-i) ;
//				      CHARPNTBYTES2(dpt,j) ;
//				    } else { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColLen",DE(Column),DE(Length),j,DE(Default),i) ; goto column_fail ; } ;
//				 } ;
//				break ;
//			 } ;


			switch (kwx)
			 { case _Default:	vlt->Col[vlt->Columns].Dflt = dpt ; break ;
			   case _Error:		vlt->Col[vlt->Columns].Err = dpt ; break ;
			 } ;
			break ;
		      case _Desc:
		      case _Description: /* DESCRIPTION xxx */
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type == V4LEX_TknType_Keyword) { UCstrcpy(tcb->UCstring,tcb->UCkeyword) ; }
			 else if (tcb->type != V4LEX_TknType_String)
				{ v_Msg(ctx,ctx->ErrorMsgAux,"CmdKWStr",tcb->type) ; goto column_fail ; } ;
			vlt->Col[vlt->Columns].Desc = v4mm_AllocUC(UCstrlen(tcb->UCstring)) ;
			UCstrcpy(vlt->Col[vlt->Columns].Desc,tcb->UCstring) ;
			break ;
		      case _UOM: /* UOM nnn/xxx */
			if (gpi->uomt == NULL) v4dpi_UOMInitialize(ctx,UNUSED) ;	/* Attempt to init if necessary */
			uomt = gpi->uomt ;
			if ((vtt = vlt->Col[vlt->Columns].vtt) == NULL)
			 vtt = (vlt->Col[vlt->Columns].vtt = (struct V4LEX__Table_Tran *)v4mm_AllocChunk(sizeof *vtt,TRUE)) ;
			if (vlt->Col[vlt->Columns].ColumnType & V4LEX_TableCT_Kernel)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColKerAttr",(kwx=_UOM)) ; goto column_fail ; } ;
			v4lex_NextTkn(tcb,0) ;
			if (uomt == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdUOMUndef",tcb->type,tcb->UCstring) ; goto column_fail ; } ;
			switch (tcb->type)
			 { default:	v_Msg(ctx,ctx->ErrorMsgAux,"CmdUOMId",tcb->type,DE(UOM)) ; goto column_fail ;
			   case V4LEX_TknType_Keyword:	UCstrcpy(tcb->UCstring,tcb->UCkeyword) ;
			   case V4LEX_TknType_String:
				for(i=0;i<uomt->Count;i++) { if (UCstrcmpIC(uomt->Entry[i].uomd->Name,tcb->UCstring) == 0) break ; } ;
				if (i >= uomt->Count)
				 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdUOMUndef",tcb->type,tcb->UCstring) ; goto column_fail ; } ;
				break ;
			   case V4LEX_TknType_Integer:
				for(i=0;i<uomt->Count;i++) { if (uomt->Entry[i].Ref == tcb->integer) break ; } ;
				if (i >= uomt->Count)
				 { UCsprintf(tcb->UCstring,10,UClit("%d"),tcb->integer) ; v_Msg(ctx,ctx->ErrorMsgAux,"CmdUOMUndef",tcb->type,tcb->UCstring) ; goto column_fail ; } ;
				break ;
			 } ;
			vtt->uomd = uomt->Entry[i].uomd ;
			break ;
		      case _Skip: /* SKIP n */
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Integer)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto column_fail ; } ;
			vlt->Col[vlt->Columns].Decimals = tcb->integer ;
			vlt->Col[vlt->Columns].ColumnType |= (V4LEX_TableCT_Ignore | V4LEX_TableCT_Skip);
			break ;
		      case _Dim:
		      case _Dimension: /* DIMENSION name */
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Keyword)
	 		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdDimName1",tcb->type) ; goto column_fail ; } ;
			diminfo = v4dpi_DimInfoGet(ctx,v4dpi_DimGet(ctx,tcb->UCkeyword,DIMREF_REF)) ;
			if (diminfo == NULL)
	 		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdDimName1",tcb->type) ; goto column_fail ; } ;
			vlt->Col[vlt->Columns].di1 = diminfo ; vlt->Col[vlt->Columns].PntType1 = diminfo->PointType ;
			break ;
		      case _Start: /* START num */
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Integer)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto column_fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","INVCOLARG","Expecting an integer START number") ;
			vlt->Col[vlt->Columns].StartX = tcb->integer-1 ; vlt->tpoT.Delim = UNUSED ; break ;
		      case _End: /* END num */
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Integer)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto column_fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","INVCOLARG","Expecting an integer START number") ;
			vlt->Col[vlt->Columns].EndX = tcb->integer-1 ; vlt->tpoT.Delim = UNUSED ; break ;
		      case _FWidth: /* FWIDTH num */
		      case _Width: /* WIDTH num */
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Integer)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto column_fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","INVCOLARG","Expecting an integer START number") ;
			vlt->Col[vlt->Columns].Width = tcb->integer ; vlt->tpoT.Delim = UNUSED ;
			if (kwx == _FWidth) vlt->Col[vlt->Columns].FixedLength = tcb->integer ;
			break ;
		      case _Quoted: /* QUOTED */	vlt->Col[vlt->Columns].ColumnType |= V4LEX_TableCT_Quoted ; break ;
		      case _Ignore: /* IGNORE */	vlt->Col[vlt->Columns].ColumnType |= V4LEX_TableCT_Ignore ; break ;
		      case _Table: /* TABLE */		vlt->Col[vlt->Columns].ColumnType |= V4LEX_TableCT_TableName ; break ;
		      case _NPTable: /* NPTABLE */	vlt->Col[vlt->Columns].ColumnType |= V4LEX_TableCT_NPTableName ; break ;
		      case _Trim: /* TRIM */		vlt->Col[vlt->Columns].ColumnType |= V4LEX_TableCT_Trim ; break ;
		      case _Format: /* FORMAT xxx */
//			switch (v4eval_NextKeyWord(colarglist,tcb,0))
			switch (v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
			 { default:		v_Msg(ctx,ctx->ErrorMsgAux,"CmdColFormat",kwx,tcb->UCkeyword) ; goto column_fail ;
			   case _Default:	vlt->Col[vlt->Columns].PntTypeInfo = 0 ; break ;
			   case _Internal:	vlt->Col[vlt->Columns].PntTypeInfo = V4LEX_TablePT_Internal ; break ;
			   case _MMDDYY:	vlt->Col[vlt->Columns].PntTypeInfo = V4LEX_TablePT_MMDDYY ; break ;
			   case _YYMMDD:	vlt->Col[vlt->Columns].PntTypeInfo = V4LEX_TablePT_YYMMDD ; break ;
			   case _DDMMMYY:	vlt->Col[vlt->Columns].PntTypeInfo = V4LEX_TablePT_DDMMMYY ; break ;
			   case _DDMMYY:	vlt->Col[vlt->Columns].PntTypeInfo = V4LEX_TablePT_DDMMYY ; break ;
			   case _Hex4:		vlt->Col[vlt->Columns].PntTypeInfo = V4LEX_TablePT_Hex4 ; break ;
			   case _Hexadecimal:	vlt->Col[vlt->Columns].PntTypeInfo = V4LEX_TablePT_Hexadecimal ; break ;
			   case _Money:		vlt->Col[vlt->Columns].PntTypeInfo = V4LEX_TablePT_Money ; break ;
			   case _YYMM:		vlt->Col[vlt->Columns].PntTypeInfo = V4LEX_TablePT_YYMM ; break ;
			   case _MMYY:		vlt->Col[vlt->Columns].PntTypeInfo = V4LEX_TablePT_MMYY ; break ;
			   case _MMM:		vlt->Col[vlt->Columns].PntTypeInfo = V4LEX_TablePT_MMM ; break ;
			 } ;
			break ;
		      case _Prefix:	/* PREFIX "xxx" */
			if ((vtt = vlt->Col[vlt->Columns].vtt) == NULL)
			 vtt = (vlt->Col[vlt->Columns].vtt = (struct V4LEX__Table_Tran *)v4mm_AllocChunk(sizeof *vtt,TRUE)) ;
			if (vlt->Col[vlt->Columns].ColumnType & V4LEX_TableCT_Kernel)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColKerAttr",(kwx=_Prefix)) ; goto column_fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","NOTSTRING","Cannot modify PREFIX on COLUMN defaulted from Kernel") ;
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type == V4LEX_TknType_Keyword) { UCstrcpy(tcb->UCstring,tcb->UCkeyword) ; }
			 else if (tcb->type != V4LEX_TknType_String)
				{ v_Msg(ctx,ctx->ErrorMsgAux,"CmdKWStr",tcb->type) ; goto column_fail ; } ;
//				v4_error(0,tcb,"V4","Eval","NOTSTRING","Expecting a string") ;
			vtt->UCprefix = v4mm_AllocUC(UCstrlen(tcb->UCstring)+1) ; UCstrcpy(vtt->UCprefix,tcb->UCstring) ;
			if (diminfo->PointType == V4DPI_PntType_TeleNum)
			 { i = UCstrtol(vtt->UCprefix,&ucb,10) ;
			   if (i < 100 || i > 999 || *ucb != UCEOS)
			    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdTeleAC",tcb->type,DE(Prefix),i) ; goto column_fail ; } ;
//			     v4_error(0,tcb,"V4","Eval","NOTSTRING","Prefix for TELENUM point not a valid area code") ;
			 } ;
			break ;
		      case _Suffix:	/* SUFFIX "xxx" */
			if ((vtt = vlt->Col[vlt->Columns].vtt) == NULL)
			 vtt = (vlt->Col[vlt->Columns].vtt = (struct V4LEX__Table_Tran *)v4mm_AllocChunk(sizeof *vtt,TRUE)) ;
			if (vlt->Col[vlt->Columns].ColumnType & V4LEX_TableCT_Kernel)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColKerAttr",(kwx=_Suffix)) ; goto column_fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","NOTSTRING","Cannot modify SUFFIX on COLUMN defaulted from Kernel") ;
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type == V4LEX_TknType_Keyword) { UCstrcpy(tcb->UCstring,tcb->UCkeyword) ; }
			 else if (tcb->type != V4LEX_TknType_String)
				{ v_Msg(ctx,ctx->ErrorMsgAux,"CmdKWStr",tcb->type) ; goto column_fail ; } ;
//				v4_error(0,tcb,"V4","Eval","NOTSTRING","Expecting a string") ;
			vtt->UCsuffix = v4mm_AllocUC(UCstrlen(tcb->UCstring)+1) ; UCstrcpy(vtt->UCsuffix,tcb->UCstring) ;
			break ;
		      case _Offset:	/* OFFSET nn */
			if ((vtt = vlt->Col[vlt->Columns].vtt) == NULL)
			 vtt = (vlt->Col[vlt->Columns].vtt = (struct V4LEX__Table_Tran *)v4mm_AllocChunk(sizeof *vtt,TRUE)) ;
			if (vlt->Col[vlt->Columns].ColumnType & V4LEX_TableCT_Kernel)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColKerAttr",(kwx=_Offset)) ; goto column_fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","NOTSTRING","Cannot modify OFFSET on COLUMN defaulted from Kernel") ;
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Integer)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto column_fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","NOTNUM","Expecting an integer OFFSET") ;
			vtt->offset = tcb->integer ;
			break ;
		      case _Scale:	/* SCALE nn */
			if ((vtt = vlt->Col[vlt->Columns].vtt) == NULL)
			 vtt = (vlt->Col[vlt->Columns].vtt = (struct V4LEX__Table_Tran *)v4mm_AllocChunk(sizeof *vtt,TRUE)) ;
			if (vlt->Col[vlt->Columns].ColumnType & V4LEX_TableCT_Kernel)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColKerAttr",(kwx=_Scale)) ; goto column_fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","NOTSTRING","Cannot modify SCALE on COLUMN defaulted from Kernel") ;
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type == V4LEX_TknType_Integer ||tcb->type == V4LEX_TknType_LInteger)
			 { FIXEDtoDOUBLE(tcb->floating,tcb->Linteger,tcb->decimal_places) ; }
			 else if (tcb->type != V4LEX_TknType_Float)
				{ v_Msg(ctx,ctx->ErrorMsgAux,"CmdArgNotNum",tcb->type) ; goto column_fail ; } ;
//				v4_error(0,tcb,"V4","Eval","NOTSTRING","Expecting a number") ;
			vtt->scale = tcb->floating ;
			break ;
		      case _Substitution:	/* SUBSTITUTION new=old, new=old, ... */
			if ((vtt = vlt->Col[vlt->Columns].vtt) == NULL)
			 vtt = (vlt->Col[vlt->Columns].vtt = (struct V4LEX__Table_Tran *)v4mm_AllocChunk(sizeof *vtt,TRUE)) ;
			if (vlt->Col[vlt->Columns].ColumnType & V4LEX_TableCT_Kernel)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColKerAttr",(kwx=_Substitution)) ; goto column_fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","NOTSTRING","Cannot modify SUBSTITUTION on COLUMN defaulted from Kernel") ;
			if (vtt->vtts == NULL)
			 vtt->vtts = (struct V4LEX__Table_Tran_Sub *)v4mm_AllocChunk(sizeof *vtt->vtts,TRUE) ;
			for(;;)
			 { 
			   if (vtt->vtts->Count >= V4LEX_Table_Tran_SubMax)
			    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdSubErr",tcb->type,V4LEX_Table_Tran_SubMax,DE(Substitution)) ; goto column_fail ; } ;
//			    v4_error(0,tcb,"V4","Eval","TOOMNYSUB","Exceeded max number of SUBSTITUTION pairs") ;
			   v4lex_NextTkn(tcb,0) ;
			   if (tcb->type == V4LEX_TknType_Integer)
			    { vtt->vtts->Sub[vtt->vtts->Count].OutValue = tcb->integer ; }
			    else { if (v4im_GetUCStrToEnumVal(tcb->UCkeyword,0) == DE(Error))
//			           if (strcmp(tcb->keyword,"ERROR") == 0)
				    { vtt->vtts->Sub[vtt->vtts->Count].OutValue = V4LEX_Table_Tran_ErrVal ; }
				    else { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto column_fail ; } ;
//				    else { v4_error(0,tcb,"V4","Eval","INVCOLARG","Expecting an integer SUBSTITUTION new-value") ; } ;
				 } ;
			   v4lex_NextTkn(tcb,0) ;
			   if (tcb->opcode != V_OpCode_Equal)
			    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdSubSyn",tcb->type,DE(Substitution)) ; goto column_fail ; } ;
//			    v4_error(0,tcb,"V4","Eval","INVCOLARG","Expecting SUBSTITUTION new=old pairs") ;
			   v4lex_NextTkn(tcb,V4LEX_Option_NegLit) ;
			   if (tcb->type != V4LEX_TknType_Integer)
			    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto column_fail ; } ;
//			    v4_error(0,tcb,"V4","Eval","INVCOLARG","Expecting an integer SUBSTITUTION old-value") ;
			   vtt->vtts->Sub[vtt->vtts->Count].InValue = tcb->integer ;
			   vtt->vtts->Count++ ;
			   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
			   if (tcb->opcode == V_OpCode_Comma) continue ;
			   v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
			   break ;
			 } ;
			break ;
		      case _FixedLength: /* FIXEDLENGTH n */
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Integer)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto column_fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","INVCOLARG","Expecting an integer FixedLength number") ;
			if (tcb->integer >= V4DPI_UCVAL_MaxSafe)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColFixLen",tcb->type,V4DPI_UCVAL_MaxSafe,DE(FixedLength)) ; goto column_fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","INVCOLARG","FixedLength argument (%d) cannot exceed %d",
//					tcb->integer,V4DPI_AlphaVal_Max) ;
			vlt->Col[vlt->Columns].FixedLength = tcb->integer ; break ;
		      case _Decimals: /* DECIMALS n */
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Integer)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto column_fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","INVCOLARG","Expecting an integer DECIMALS number") ;
			vlt->Col[vlt->Columns].Decimals = tcb->integer ; break ;
		      case _BaseYear: /* BASEYEAR yyyy */
			v4lex_NextTkn(tcb,0) ;
			if (tcb->type != V4LEX_TknType_Integer)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto column_fail ; } ;
//			 v4_error(0,tcb,"V4","Eval","INVCOLARG","Expecting an integer BASEYEAR number") ;
			VCALADJYEAR(FALSE,tcb->integer) ; vlt->Col[vlt->Columns].Decimals = tcb->integer ; break ;
		    } ;
		 } ;
//		if (vlt->Col[vlt->Columns].ColumnType & V4LEX_TableCT_Ignore)	/* Should not IGNORE if also trying to use column as 'Table' or 'NPTable' */
//		 { if (vlt->Col[vlt->Columns].ColumnType & (V4LEX_TableCT_TableName | V4LEX_TableCT_NPTableName))
//		    { v_Msg(ctx,ctx->ErrorMsgAux,"TblColIgnore",DE(Ignore),DE(Table),DE(NPTable)) ; goto column_fail ; } ;
//		 } ;
		if (vlt->Col[vlt->Columns].FixedLength > 0)
		 { if (diminfo->PointType != V4DPI_PntType_Char && diminfo->PointType != V4DPI_PntType_UCChar && diminfo->PointType != V4DPI_PntType_BinObj)
		    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColFixPT",DE(FixedLength),V4DPI_PntType_Char,V4DPI_PntType_Char,V4DPI_PntType_BinObj) ; goto column_fail ; } ;
//		    v4_error(0,tcb,"V4","Eval","INVCOLARG","FixedLength attribute only allowed for Alpha/Binary dimensions") ;
		   if (vlt->Col[vlt->Columns].Err != NULL && vlt->Col[vlt->Columns].Err->PntType != V4DPI_PntType_Special)
		    { num = (vlt->Col[vlt->Columns].Err->PntType == V4DPI_PntType_UCChar ? UCCHARSTRLEN(vlt->Col[vlt->Columns].Err) : vlt->Col[vlt->Columns].Err->Value.AlphaVal[0]) ;
		      if (num > vlt->Col[vlt->Columns].FixedLength)
		       { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColFixLenMM",DE(Error),num,DE(FixedLength),vlt->Col[vlt->Columns].FixedLength) ; goto column_fail ; } ;
//		       v4_error(0,tcb,"V4","Eval","INVCOLARG","Length of Error point (%d) not same as FixedLength (%d)",
//					vlt->Col[vlt->Columns].Err->Value.AlphaVal[0],vlt->Col[vlt->Columns].FixedLength) ;
		    } ;
		   if (vlt->Col[vlt->Columns].Dflt != NULL && vlt->Col[vlt->Columns].Dflt->PntType != V4DPI_PntType_Special)
		    { switch(vlt->Col[vlt->Columns].Dflt->PntType)
		       { case V4DPI_PntType_BinObj:	num = vlt->Col[vlt->Columns].Dflt->Bytes - V4DPI_PointHdr_Bytes ; break ;
		         case V4DPI_PntType_UCChar:	num = UCCHARSTRLEN(vlt->Col[vlt->Columns].Dflt) ; break ;
		         default:			num = CHARSTRLEN(vlt->Col[vlt->Columns].Dflt) ; break ;
		       } ;
		      if (num > vlt->Col[vlt->Columns].FixedLength)
		       { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColFixLenMM",DE(Default),num,DE(FixedLength),vlt->Col[vlt->Columns].FixedLength) ; goto column_fail ; } ;
//		       v4_error(0,tcb,"V4","Eval","INVCOLARG","Length of Default point (%d) not same as FixedLength (%d)",
//					vlt->Col[vlt->Columns].Dflt->Value.AlphaVal[0],vlt->Col[vlt->Columns].FixedLength) ;
		    } ;
		 } ;
		if (vlt->Col[vlt->Columns].EndX != UNUSED && vlt->Col[vlt->Columns].Width != UNUSED)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColEndWidth") ; goto column_fail ; } ;
//		 v4_error(0,0,"V4","Eval","INVCOLARG","Cannot have both End & Width specified") ;

//VEH041105 - Take out this error - figure out columns based on Start at EndTable time
//		if (vlt->Col[vlt->Columns].StartX != UNUSED && vlt->Col[vlt->Columns].Width == UNUSED && vlt->Col[vlt->Columns].EndX == UNUSED)
//		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColStart",vlt->Col[vlt->Columns].StartX) ; goto column_fail ; } ;
//		 v4_error(0,0,"V4","Eval","NOENDCOL","START %d given, but no END or WIDTH specified",vlt->Col[vlt->Columns].StartX) ;
		if (vlt->Col[vlt->Columns].StartX == UNUSED && vlt->Col[vlt->Columns].Width != UNUSED)
		 vlt->Col[vlt->Columns].StartX =
			(vlt->Columns == 0 ? 0 : vlt->Col[vlt->Columns-1].StartX + vlt->Col[vlt->Columns-1].Width) ;
		if (vlt->Col[vlt->Columns].EndX != UNUSED)
		 vlt->Col[vlt->Columns].Width = vlt->Col[vlt->Columns].EndX - vlt->Col[vlt->Columns].StartX + 1 ;
		if (diminfo->PointType == V4DPI_PntType_Fixed)
		 { if (vlt->Col[vlt->Columns].Decimals == UNUSED) vlt->Col[vlt->Columns].Decimals = diminfo->Decimals ;
		   if (vlt->Col[vlt->Columns].Decimals != diminfo->Decimals)
		     { v_Msg(ctx,ctx->ErrorMsgAux,"CmdColDecimal",vlt->Col[vlt->Columns].Decimals,diminfo->DimId,diminfo->Decimals) ; goto column_fail ; } ;
//		     v4_error(0,tcb,"V4","Eval","INVDEC","Decimals (%d) not same as those for Dim %s (%d)",
//				vlt->Col[vlt->Columns].Decimals,diminfo->DimName,diminfo->Decimals) ;
		 } ;
		vlt->Columns ++ ;
		if (more) goto top_column ;
		break ;
/*		If we fail in column then may have to clean up some stuff */
column_fail:
		if (gpi->SaveTables) { tcb->appendto = NULL ; } ;
		v4mm_FreeChunk(vlt) ; vlt = NULL ;
		gpi->vlt = NULL ;		/* VEH140107 - disable all tables - we don't know the state of anything if an error */
		goto fail ;
	      case _EndTable:	/* ENDTABLE */
		if (CHECKV4INIT) goto fail ;
		if (vlt == NULL)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInTbl",(kwx=_EndTable),(kwx2=_Table)) ;
/*		   Before we fail maybe clean up pointers if saving table */
		   if (gpi->SaveTables)
		    { if (stptr != NULL) { stptr = NULL ; }
		       else { tcb->appendto = NULL ; tcb->appendopt = 0 ; tcb->maxappend = 0 ; tcb->bytesappend = 0 ; } ;
		    } ;
		   goto fail ;
		 } ;
//		 v4_error(0,tcb,"V4","Eval","NOTBLDEF","Not currently within TABLE definition") ;
		if (!v4eval_ChkEOL(tcb)) goto fail ;
		if (!v4im_vltEndTable(ctx,vlt,TRUE)) goto fail ;		/* Finish off any loose ends in the table */
		if (gpi->SaveTables)
		 { if (stptr == NULL)						/* If NULL then appendto points to stbt->BigBuf - reset it */
		    { tcb->appendto = NULL ; tcb->appendopt = 0 ; tcb->maxappend = 0 ; tcb->bytesappend = 0 ; }
		    else { UCstrcpy(stbt->BigBuf,stptr) ; stptr = NULL ; } ;	/* If not NULL then save in stbt->BigBuf */
		   v4eval_SaveMacroBodyOnly(ctx,vlt->Name,stbt->BigBuf,V4LEX_MACTYPE_Table,0) ;
		   v4mm_FreeChunk(stbt) ; stbt = NULL ;
		 } ;
		vlt = NULL ; break ;
#ifdef WANTRESTRICTCOMMAND
	      case _Restrict:	/* Restrict xxx */
		for(looper=TRUE;looper;)
		 { 
		   switch (v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
		    { default: v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,cx) ; goto fail ;
		      case _EndOfLine_:			looper = FALSE ; break ;
		      case _SemiColon_:			if (cnt == 1) cnt++ ; looper=FALSE ; break ;
		      case _Files:			gpi->RestrictionMap |= V_Restrict_FileRead ; break ;
		      case _OSFileListOf:		gpi->RestrictionMap |= V_Restrict_OSFileListOf ; break ;
		      case _OSFileInfo:			gpi->RestrictionMap |= V_Restrict_OSFileInfo ; break ;
		      case _ProjectArea:		gpi->RestrictionMap |= V_Restrict_ProjectArea ; break ;
		      case _AreaUpdate:			gpi->RestrictionMap |= V_Restrict_AreaUpdate ; break ;
		      case _AreaAll:			gpi->RestrictionMap |= V_Restrict_AreaCommand ; break ;
//		      case _AreaRename:			gpi->RestrictionMap |= V_Restrict_AreaRename ; break ;
		      case _Redirection:		gpi->RestrictionMap |= V_Restrict_redirection ; break ;
		      case _Quota:
			for(;looper;)
			 { 
			   switch (v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
			    { default: v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,cx) ; goto fail ;
			      case _EndOfLine_:		looper = FALSE ; break ;
			      case _SemiColon_:		if (cnt == 1) cnt++ ; looper=FALSE ; break ;
			      case _Echo:
				v4lex_NextTkn(tcb,0) ;
				if (tcb->type != V4LEX_TknType_Integer) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;
				gpi->QuotaData = tcb->integer ; gpi->RestrictionMap |= V_Restrict_QuotaEcho ; break ;
			      case _HTMLGet:
				v4lex_NextTkn(tcb,0) ;
				if (tcb->type != V4LEX_TknType_Integer) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;
				gpi->QuotaHTMLGet = tcb->integer ; gpi->RestrictionMap |= V_Restrict_QuotaHTML ; break ;
			      case _HTMLPut:
				v4lex_NextTkn(tcb,0) ;
				if (tcb->type != V4LEX_TknType_Integer) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;
				gpi->QuotaHTMLPut = tcb->integer ; gpi->RestrictionMap |= V_Restrict_QuotaHTML ; break ;
			    } ;
			   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
			   if(tcb->type == V4LEX_TknType_EOL || tcb->opcode == V_OpCode_Semi) { looper = FALSE ; break ; } ;
			   if (tcb->opcode == V_OpCode_Comma) continue ;
			   v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; break ;
			 } ;
			break ;
		      case _Test:		gpi->RestrictionMap |= V_Restrict_Test ; break ;
		     } ;
		 } ;
		break ;
#endif

	      case _Set:	/* SET xxx */
		for(looper=TRUE;looper;)
		 { INDEX rx ;
//		   switch (v4eval_NextKeyWord(setlist,tcb,EOLOK))
		   switch (v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
		    { default: v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,cx) ; goto fail ;
//				v4_error(V4E_INVSETATTR,tcb,"V4","Eval","INVSETATTR","Unknown SET attribute: %s",tcb->keyword) ;
		      case _EndOfLine_:		looper = FALSE ; break ;
		      case _SemiColon_:		if (cnt == 1) cnt++ ; looper=FALSE ; break ;
		      case _Aggregate:
		      case _Agg:
			if (!v4dpi_PointParse(ctx,&valpt,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
			for(rx=0;rx<gpi->AreaAggCount;rx++)
			 { if (gpi->AreaAgg[rx].aggPId.Bytes == 0) continue ; if (memcmp(&gpi->AreaAgg[rx].aggPId,&valpt,gpi->AreaAgg[rx].aggPId.Bytes) == 0) break ;
			 } ;
			if (rx >= gpi->AreaAggCount)		/* If did not find match look for 'NONE' to reset it */
			 { v4dpi_PointToString(tcb->UCstring,&valpt,ctx,V4DPI_FormatOpt_Echo) ;
			   if (v_IsNoneLiteral(tcb->UCstring)) { gpi->dfltAreaAggX = DFLTAGGAREA ; break ; } ;
			   v_Msg(ctx,ctx->ErrorMsgAux,"AreaNoId1",&valpt) ; goto fail ;
			 } ;
			if ((gpi->AreaAgg[rx].pcb->AccessMode & V4IS_PCB_AM_Insert) == 0) { v_Msg(ctx,ctx->ErrorMsgAux,"AggNotUpd",gpi->AreaAgg[rx].pcb->UCFileName) ; goto fail ; } ;
			gpi->dfltAreaAggX = rx ;
			break ;
		      case _AJAX:
			v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
			if (tcb->type != V4LEX_TknType_Keyword) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntValue",tcb->type,tcb->UCstring) ; goto fail ; } ;
			switch (v4im_GetUCStrToEnumVal(tcb->UCkeyword,0))
			 { default:		v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetEcho",DE(Set),DE(AJAX),tcb->UCkeyword) ; goto fail ;
			   case _Err:
			   case _Error:
			   case _Errors:
				v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
				if (tcb->type != V4LEX_TknType_String) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntValue",tcb->type,tcb->UCstring) ; goto fail ; } ;
				if (UCstrlen(tcb->UCstring) >= sizeof gpi->patternAJAXErr) tcb->UCstring[(sizeof gpi->patternAJAXErr) - 5] = UCEOS ;
				strcpy(gpi->patternAJAXErr,"@") ; strcat(gpi->patternAJAXErr,UCretASC(tcb->UCstring)) ;
				break ;
			   case _JSON:
				v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
				if (tcb->type != V4LEX_TknType_String) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntValue",tcb->type,tcb->UCstring) ; goto fail ; } ;
				if (gpi->fileIdDefer != UNUSED) { v_Msg(ctx,NULL,"DeferNoMult1",tcb->type) ; goto fail ; } ;
				if (UCstrlen(tcb->UCstring) >= sizeof gpi->patternAJAXJSON) tcb->UCstring[(sizeof gpi->patternAJAXJSON) - 5] = UCEOS ;
				strcpy(gpi->patternAJAXJSON,"@") ; strcat(gpi->patternAJAXJSON,UCretASC(tcb->UCstring)) ;
				gpi->fileIdDefer = vout_OpenStreamBuffer(NULL,ctx->ErrorMsgAux,TRUE) ;
				break ;
			 } ;
			break ;
		      case _BindContext:		/* Set BindContext [isct] */
			if (tcb->ilvl[tcb->ilx].bcPt == NULL) { tcb->ilvl[tcb->ilx].bcPt = (P *)v4mm_AllocChunk(sizeof(P),FALSE) ; } ;
			ZPH(tcb->ilvl[tcb->ilx].bcPt) ;
			if (!v4dpi_PointParse(ctx,tcb->ilvl[tcb->ilx].bcPt,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
			if (memcmp(tcb->ilvl[tcb->ilx].bcPt,&protoNone,V4PS_Int) == 0)		/* Set BindContext UV4:none = disable */
			 { ZPH(tcb->ilvl[tcb->ilx].bcPt) ; break ; } ;
			if (tcb->ilvl[tcb->ilx].bcPt->ForceEval)
			 { dpt = v4dpi_IsctEval(&isctpt,tcb->ilvl[tcb->ilx].bcPt,ctx,0,NULL,NULL) ;
			   if (dpt == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIsctFail",&valpt) ; goto fail ; } ;
			   memcpy(tcb->ilvl[tcb->ilx].bcPt,dpt,dpt->Bytes) ;
			 } ;
			if (tcb->ilvl[tcb->ilx].bcPt->PntType != V4DPI_PntType_Isct)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIsct1Arg",tcb->type,cx,V4DPI_PntType_Isct) ; goto fail ; } ;
			break ;
		      case _Echo:
			v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
			switch(tcb->type)
			 { default: v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntValue",tcb->type,tcb->UCstring) ; goto fail ;
			   case V4LEX_TknType_String:	tcb->UCstring[V4LEX_Tkn_KeywordMax] = UCEOS ; UCstrcpy(tcb->UCkeyword,tcb->UCstring) ;
			   case V4LEX_TknType_Keyword:
				switch (v4im_GetUCStrToEnumVal(tcb->UCkeyword,0))
				 { default:	v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetEcho",DE(Set),DE(Echo),tcb->UCkeyword) ; goto fail ;
				   case _Disable:	tcb->ilvl[tcb->ilx].echoDisable = TRUE ; break ;
				   case _Enable:	tcb->ilvl[tcb->ilx].echoDisable = FALSE ; break ;
				   case _On:		tcb->ilvl[tcb->ilx].echo = 1 ; break ;
				   case _Off:		tcb->ilvl[tcb->ilx].echo = 0 ; tcb->ilvl[tcb->ilx].echoHTML = FALSE ; break ;
				   case _All:		tcb->ilvl[tcb->ilx].echo = 9999 ; break ;
				   case _HTML:		tcb->ilvl[tcb->ilx].echo = 1 ; tcb->ilvl[tcb->ilx].echoHTML = TRUE ; break ;
				 } ;
//				if (strcmp(tcb->keyword,"ON") == 0) { tcb->ilvl[tcb->ilx].echo = 1 ; }
//				 else if (strcmp(tcb->keyword,"OFF") == 0) { tcb->ilvl[tcb->ilx].echo = 0 ; tcb->ilvl[tcb->ilx].echoHTML = FALSE ; }
//				 else if (strcmp(tcb->keyword,"ALL") == 0) { tcb->ilvl[tcb->ilx].echo = 9999 ; }
//				 else if (strcmp(tcb->keyword,"HTML") == 0) { tcb->ilvl[tcb->ilx].echo = 1 ; tcb->ilvl[tcb->ilx].echoHTML = TRUE ; }
//				 else v4_error(V4E_INVSETECHOKW,tcb,"V4","Eval","INVSETECHOKW","Invalid SET ECHO keyword: %s",tcb->keyword) ;
				 looper = FALSE ; break ;
			   case V4LEX_TknType_Integer:	tcb->ilvl[tcb->ilx].echo = tcb->integer ; break ;
			   case V4LEX_TknType_EOL:
				tcb->ilvl[tcb->ilx].echo = 1 ; looper = FALSE ; break ;
			   case V4LEX_TknType_Punc:
				if (tcb->opcode == V_OpCode_Semi) { tcb->ilvl[tcb->ilx].echo = 1 ; }
				 else { v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetEcho",DE(Set),DE(Echo),tcb->UCkeyword) ; goto fail ; } ;
				looper = FALSE ; break ;
			  } ;
			 break ;
		      case _NoEcho:	tcb->ilvl[tcb->ilx].echo = 0 ; tcb->ilvl[tcb->ilx].echoHTML = FALSE ; break ;
		      case _Duplicates:
				switch (v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
				 { default:			v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetEcho",DE(Set),DE(Duplicates),tcb->UCkeyword) ; goto fail ;
				   case _Err:
				   case _Error:			gpi->dupPointAction = V_DUPDICTACT_Err ; break ;
				   case _Ignore:		gpi->dupPointAction = V_DUPDICTACT_Ignore ; break ;
				   case _Warn:			gpi->dupPointAction = V_DUPDICTACT_Warn ; break ;
				 } ;
				break ;
		      case _Trace: /* TRACE */
		              { FLAGS32 setTrace ; ETYPE plusMinus ;
				setTrace = 0 ;
				v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;		/* Look for starting +/- */
				switch(tcb->opcode)
				 { default:			plusMinus = 0 ; v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; break ;
				   case V_OpCode_Plus:		plusMinus = 1 ; break ;
				   case V_OpCode_Minus:		plusMinus = -1 ; break ;
				 } ;
				for(;;)
				 {
				   switch (v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
				    { default:			v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetEcho",DE(Set),DE(Trace),tcb->UCkeyword) ; goto fail ;
				      case _Dimension:
				      case _Dimensions:		if (gpi->vldd == NULL) { gpi->vldd = v4mm_AllocChunk(sizeof *gpi->vldd,FALSE) ; gpi->vldd->count = 0 ; gpi->vldd->vlddPrior = NULL ; } ;
								break ;
				      case _EndOfLine_:		looper = FALSE ; break ;
				      case _Log:		setTrace |= V4TRACE_LogIscts ;
								v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;	/* Look for optional "starting eval #" */
								if (tcb->type == V4LEX_TknType_Integer) { gpi->StartTraceOutput = tcb->integer ; }
								 else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; gpi->StartTraceOutput = 0 ; } ;
								break ;
				      case _NBLBindings:	setTrace |= V4TRACE_BindList ; break ;
//				      case _Decomposition:	setTrace |= V4TRACE_Decomp ; break ;
				      case _Compares:		setTrace |= V4TRACE_PointCompare ; break ;
				      case _AddContext:		setTrace |= V4TRACE_ContextAdd ; break ;
				      case _AutoContext:	setTrace |= V4TRACE_AutoContext ; break ;
				      case _EvalPt:		setTrace |= V4TRACE_EvalPt ; break ;
				      case _EvalToContext:	setTrace |= V4TRACE_EvalToContext ; break ;
				      case _Tally:
				      case _Progress:		setTrace |= V4TRACE_Progress ;
								v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;	/* Look for optional "starting eval #" */
								if (tcb->type == V4LEX_TknType_Integer) { gpi->traceProgressMin = tcb->integer ; }
								 else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; gpi->traceProgressMin = 0 ; } ;
								break ;
				      case _TallyBind:		setTrace |= V4TRACE_TallyBind ; break ;
				      case _Bind:
				      case _Binding:		setTrace |= V4TRACE_Bindings ; break ;
				      case _Macro:		setTrace |= V4TRACE_Macros ; break ;
				      case _Lists:		setTrace |= V4TRACE_Lists ; break ;
				      case _Frame:		setTrace |= V4TRACE_Context ; break ;
				      case _Point:		setTrace |= V4TRACE_Points ; break ;
				      case _BindEval:		setTrace |= V4TRACE_BindEval ; break ;
				      case _Arith:		setTrace |= V4TRACE_Arith ; break ;
				      case _V4IS:		setTrace |= V4TRACE_V4IS ; break ;
				      case _IsctFail:		setTrace |= V4TRACE_IsctFail ; break ;
				      case _Optimize:		setTrace |= V4TRACE_Optimize ; break ;
				      case _All:		setTrace |= 0xffffffff ; gpi->StartTraceOutput = 0 ; break ;
				      case _None:
				      case _Off:		setTrace = 0 ;  gpi->traceProgressMin = 0 ; gpi->StartTraceOutput = 0 ; break ;
				      case _XDB:		setTrace |= V4TRACE_XDB ; break ;
				      case _ODBC:		setTrace |= V4TRACE_XDB ; break ;
				      case _Errors:		setTrace |= V4TRACE_Errors ; break ;
				      case _Recursion:		setTrace |= V4TRACE_Recursion ; break ;
				      case _TimeStamp:		setTrace |= V4TRACE_TimeStamp ; break ;
				    } ;
				   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
				   if(tcb->type == V4LEX_TknType_EOL || tcb->opcode == V_OpCode_Semi) { looper = FALSE ; break ; } ;
				   if (tcb->opcode == V_OpCode_Comma) continue ;
				   v_Msg(NULL,ctx->ErrorMsgAux,"CmdNotEOL",tcb->type) ; goto fail ;
				  } ;
				 switch (plusMinus)
				  { case -1:		trace &= (~setTrace) ; break ;
				    case 0:		trace = setTrace ; break ;
				    case 1:		trace |= setTrace ; break ;
				  } ;
				 traceGlobal = trace ;		/* Set this so other (higher) instances of Eval() can see if any changes made */
		              }
				 break ;
		      case _NoTrace:	trace = FALSE ; break ;
		      case _MacroBind:
				UCstrcpy(macrobind,tcb->ilvl[tcb->ilx].input_ptr) ; tcb->need_input_line = TRUE ;
				tcb->type = V4LEX_TknType_EOL ; ZUS(textbind) ; looper = FALSE ; break ;
		      case _MacroCall:
				UCstrcpy(macrocall,tcb->ilvl[tcb->ilx].input_ptr) ; tcb->need_input_line = TRUE ;
				tcb->type = V4LEX_TknType_EOL ; looper = FALSE ; break ;
		      case _MIDAS:
				switch (v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
				 { default:		v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetEcho",DE(Set),DE(MIDAS),tcb->UCkeyword) ; goto fail ;
				   case _Off:		gpi->inMIDAS = FALSE ; break ;
				   case _On:		gpi->inMIDAS = TRUE ; break ;
				 } ; break ;
		      case _TextBind:
				UCstrcpy(textbind,tcb->ilvl[tcb->ilx].input_ptr) ; tcb->need_input_line = TRUE ;
				tcb->type = V4LEX_TknType_EOL ; ZUS(macrobind) ; looper = FALSE ; break ;
		      case _Output:
				switch (v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
				 { default:		v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetEcho",DE(Set),DE(Output),tcb->UCkeyword) ; goto fail ;
				   case _ASCII:		gpi->OutputCharSet = V4L_TextFileType_ASCII ; break ;
				   case _UTF16:		gpi->OutputCharSet = V4L_TextFileType_UTF16 ; break ;
				   case _UTF16be:	gpi->OutputCharSet = V4L_TextFileType_UTF16be ; break ;
				   case _UTF16le:	gpi->OutputCharSet = V4L_TextFileType_UTF16le ; break ;
				   case _UTF8:		gpi->OutputCharSet = V4L_TextFileType_UTF8 ; break ;
				   case _UTF8nh:	gpi->OutputCharSet = V4L_TextFileType_UTF8nh ; break ;
				 } ; break ;
		      case _Parameter: /* Parameter let value */
				v4lex_NextTkn(tcb,0) ;
				if (UCstrlen(tcb->UCkeyword) != 1)
				 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetParam",DE(Set),DE(Parameter),tcb->UCkeyword) ; goto fail ; } ;
//				 v4_error(V4E_BADLETLEN,tcb,"V4","Eval","BADLETLEN","Expecting a single letter, not: %s",tcb->keyword) ;
				if (tcb->UCkeyword[0] >= UClit('a') && tcb->UCkeyword[0] <= UClit('z')) { i = tcb->UCkeyword[0] - UClit('a') ; }
				 else if (tcb->UCkeyword[0] >= UClit('A') && tcb->UCkeyword[0] <= UClit('Z')) { i = tcb->UCkeyword[0] - UClit('A') ; }
				 else  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetParam",DE(Set),DE(Parameter),tcb->UCkeyword) ; goto fail ; } ;
				if (tcb->poundparam[i].SetAtStartup)
				 { v_Msg(ctx,ctx->ErrorMsgAux,"*CmdSetParamSU",DE(Set),DE(Parameter),tcb->UCkeyword) ;
				   vout_UCText(VOUT_Warn,0,ctx->ErrorMsgAux) ;
				 } ;
				j = FALSE ; tcb->poundparam[i].SetAtStartup = FALSE ; assok = TRUE ;
set_again:			
//				v4lex_NextTkn(tcb,0) ;
				if (!v4eval_NextAsString(ctx,tcb,V4LEX_Option_ForceKW,assok)) goto fail ; assok = FALSE ;
				switch(tcb->type)
				 { case V4LEX_TknType_Keyword:
//					strcpy(tcb->string,tcb->keyword) ;
				   case V4LEX_TknType_String:
					l = UCstrlen(tcb->UCstring) ;
					if (j) l += UCstrlen(tcb->poundparam[i].value) ;
					if (l >= UCsizeof(tcb->poundparam[i].value))
					 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetParamLen",tcb->type,DE(Set),DE(Parameter),l,UCsizeof(tcb->poundparam[i].value)) ; goto fail ; } ;
//					 v4_error(V4E_PARAMTOOLONG,tcb,"V4","Eval","PARAMTOOLONG","Parameter value length (%d) cannot exceed %d bytes",
//						UCstrlen(tcb->ilvl[tcb->ilx].input_ptr),l-1) ;
					if (j)
					 { UCstrcat(tcb->poundparam[i].value,tcb->UCstring) ; }
					 else { UCstrcpy(tcb->poundparam[i].value,tcb->UCstring) ; } ;
					tcb->poundparam[i].numvalue = V4LEX_Tkn_PPNumUndef ;
					break ;
				   case V4LEX_TknType_Integer:
					if (j) { tcb->poundparam[i].numvalue += tcb->integer ; }
					 else { tcb->poundparam[i].numvalue = tcb->integer ; } ;
					ZS(tcb->poundparam[i].value) ;
					break ;
				   case V4LEX_TknType_Punc:
					if (tcb->opcode == V_OpCode_Equal)
					 { v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
					   if (tcb->opcode != V_OpCode_Percent) { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; goto set_again ; } ;
/*					   Got set parameter x=%... - look for logical name */
					   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
					   if (tcb->type != V4LEX_TknType_Keyword)
					    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetParamSyn",tcb->type,DE(Set),DE(Parameter)) ; goto fail ; } ;
					   if (!v_UCGetEnvValue(tcb->UCkeyword,tcb->poundparam[i].value,UCsizeof(tcb->poundparam[i].value)))
					    { v_Msg(NULL,ctx->ErrorMsgAux,"LogDecNoLogUC",tcb->UCkeyword,tcb->UCkeyword) ; goto fail ; } ;
					   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
					   if (tcb->opcode != V_OpCode_Percent) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetParamSyn",tcb->type,DE(Set),DE(Parameter)) ; goto fail ; } ;
					   break ;
					 }
					if (tcb->opcode == V_OpCode_PlusEqual)
					 { j = TRUE ; goto set_again ; } ;
					/* If not = or += then drop thru to error */
				   default:
					v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetParamSyn",tcb->type,DE(Set),DE(Parameter)) ; goto fail ;
//					v4_error(0,tcb,"V4","Eval","INVPPVALUE","Invalid parameter value type") ;
				 } ;
				v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
				if (tcb->opcode == V_OpCode_Plus) { j = TRUE ; goto set_again ; }
				 else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
				looper = FALSE ; break ;




		      case _QParameter: /* Parameter let value */
				v4lex_NextTkn(tcb,0) ;
				if (UCstrlen(tcb->UCkeyword) != 1)
				 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetParam",DE(Set),DE(Parameter),tcb->UCkeyword) ; goto fail ; } ;
				if (tcb->UCkeyword[0] >= UClit('a') && tcb->UCkeyword[0] <= UClit('z')) { i = tcb->UCkeyword[0] - UClit('a') ; }
				 else if (tcb->UCkeyword[0] >= UClit('A') && tcb->UCkeyword[0] <= UClit('Z')) { i = tcb->UCkeyword[0] - UClit('A') ; }
				 else  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetParam",DE(Set),DE(Parameter),tcb->UCkeyword) ; goto fail ; } ;
				if (tcb->poundparam[i].SetAtStartup)
				 { v_Msg(ctx,ctx->ErrorMsgAux,"*CmdSetParamSU",DE(Set),DE(Parameter),tcb->UCkeyword) ; vout_UCText(VOUT_Warn,0,ctx->ErrorMsgAux) ; } ;
				tcb->poundparam[i].SetAtStartup = FALSE ; tcb->poundparam[i].numvalue = V4LEX_Tkn_PPNumUndef ;
				if (!v4eval_NextAsString(ctx,tcb,V4LEX_Option_ForceKW,FALSE)) goto fail ;
				if (tcb->type == V4LEX_TknType_Keyword || tcb->type == V4LEX_TknType_String)
				 { v_StringLit(tcb->UCstring,tcb->poundparam[i].value,UCsizeof(tcb->poundparam[i].value),UClit('"'),UCEOS) ; }
				 else {	v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetParamSyn",tcb->type,DE(Set),DE(QParameter)) ; goto fail ; } ;
				v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
				v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
				looper = FALSE ; break ;





		      case _MaxAllowedErrors:
				v4lex_NextTkn(tcb,0) ;
				gpi->MaxAllowedErrs = tcb->integer ;
				break ;
//		      case _MassUpdates: /* MassUpdates [logical] */
//				(gpi->MassUpdate) = TRUE ;
//				break ;
		      case _PointMax:
				v4lex_NextTkn(tcb,0) ;
				if (tcb->type != V4LEX_TknType_Integer)
				 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; goto fail ; } ;
//				 v4_error(0,tcb,"V4","Eval","INVTLYMAX","Expecting an integer PointMax number") ;
				gpi->PointMax = tcb->integer ; break ;
		      case _EchoS:
//				switch (v4eval_NextKeyWord(echoslist,tcb,EOLOK))
				switch (v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
				 { default:		v_Msg(ctx,ctx->ErrorMsgAux,"CmdSetEcho",DE(Set),DE(EchoS),tcb->UCkeyword) ; goto fail ;
//					 v4_error(0,tcb,"V4","Eval","INVTLYMAX","Expecting keyword: TEXT or SPREADSHEET") ;
				   case _Spreadsheet:	i = TRUE ; break ;
				   case _Text:		i = FALSE ; break ;
				 } ; gpi->DoEchoS = i ; break ;
		      case _Argument:
				v4lex_NextTkn(tcb,V4LEX_Option_ForceAsIs) ;
				if (tcb->type == V4LEX_TknType_Keyword) { UCstrcpy(tcb->UCstring,tcb->UCkeyword) ; }
				 else if (tcb->type != V4LEX_TknType_String)
				 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdKWStr",tcb->type) ; goto fail ; } ;
				for(i=0;i<V4LEX_Tkn_ArgMax;i++)
				 { if (UCstrcmpIC(tcb->UCkeyword,tcb->ilvl[tcb->ilx].macarg[i].name) == 0) break ; } ;
//				if (mc == NULL || i >= V4LEX_Tkn_ArgMax)
				if (i >= V4LEX_Tkn_ArgMax)
				 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdMacroArg3",tcb->type,tcb->UCkeyword) ; goto fail ; } ;
				v4lex_NextTkn(tcb,V4LEX_Option_ForceAsIs) ;
//				if (mc->NextFree + UCstrlen(tcb->UCstring) > 1024) mc->NextFree = 0 ;
//				mc->SetArgs[mc->NextFree] = UCEOS ; UCstrcat(&mc->SetArgs[mc->NextFree],tcb->UCstring) ;
//				mc->NextFree += (UCstrlen(tcb->UCstring)+1) ;
				tcb->ilvl[tcb->ilx].macarg[i].value = v4mm_AllocUC(UCstrlen(tcb->UCstring)) ;
				UCstrcpy(tcb->ilvl[tcb->ilx].macarg[i].value,tcb->UCstring) ;
				break ;
		      case _MaxPointOutput:
				v4lex_NextTkn(tcb,0) ;
				if (tcb->type != V4LEX_TknType_Integer || tcb->integer < 50 || tcb->integer >= V4CTX_MaxPointOutputMax)
				 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdSMPORange",tcb->type,DE(Set),DE(MaxPointOutput),50,V4CTX_MaxPointOutputMax) ; goto fail ; } ;
//				 v4_error(0,tcb,"V4","Eval","INVTLYMAX","Expecting an integer MaxPointOutput number between 20 & 5000") ;
				gpi->MaxPointOutput = tcb->integer ; break ;
		      case _LogToIsct:
//				switch (v4eval_NextKeyWord(setlogtoisctlist,tcb,EOLOK))
				switch (v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
				 { default:	v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,cx) ; goto fail ;
				   case _Disable:	i = FALSE ; break ;
				   case _Enable:	i = TRUE ; break ;
				 } ; gpi->DoLogToIsct = i ; break ;
#ifdef CHECKCLASSICMODE
		     case _Classic:		gpi->ClassicMode = TRUE ; break ;
#endif
		     case _MacroSave:		gpi->SaveMacros = TRUE ; break ;
		     case _TableSave:		gpi->SaveTables = TRUE ; break ;
		     case _Errors:
		     case _ADV: /* ADV option */
//				switch (v4eval_NextKeyWord(setadvlist,tcb,EOLOK))
				switch (v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
				 { default:	v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,cx) ; goto fail ;
				   case _Output:
				   case _EchoE: /* EchoE/Output filename  */
					v_ParseFileName(tcb,gpi->CtxADVEchoE,NULL,UClit(".v4r"),NULL) ;
					break ;
				   case _None:		gpi->ErrCount = 0 ; break ;
				   case _URLOnError: /* URLonError xxx */
					v4lex_NextTkn(tcb,0) ;
					if (tcb->type != V4LEX_TknType_String)
					 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdKWStr",tcb->type) ; goto fail ; } ;
					if (UCstrlen(tcb->UCstring) >= UCsizeof(gpi->CtxADVURLonError))
					 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdTooLong",tcb->type) ; goto fail ; } ;
					UCstrcpy(gpi->CtxADVURLonError,tcb->UCstring) ;
					break ;
				   case _Compiling: /* COMPILING */
					gpi->Compiling = 999999999 ; break ;
				   case _Interpreting: /* INTERPRETING */
					gpi->Compiling = 0 ; break ;
				 } ;
				break ;
		     case _Logical: /* LOGICAL symbol="path" */
				for(;;)
				 { UCCHAR tsym[V4LEX_Tkn_KeywordMax+1] ;
				   v4lex_NextTkn(tcb,0) ;
				   if (tcb->type != V4LEX_TknType_Keyword)
				    { v_Msg(ctx,ctx->ErrorMsgAux,"CmdKWStr",tcb->type) ; goto fail ; } ;
				   UCstrcpy(tsym,tcb->UCkeyword) ;
				   v4lex_NextTkn(tcb,0) ; if (tcb->opcode == V_OpCode_Equal) v4lex_NextTkn(tcb,0) ;
				   if (tcb->type == V4LEX_TknType_String) { UCstrcpy(filename,tcb->UCstring) ; }
				    else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; v_ParseFileName(tcb,filename,NULL,NULL,&i) ; } ;
				   vlog_StoreSymValue(tsym,filename) ;
				   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
				   if (tcb->opcode == V_OpCode_Comma) continue ;
				   v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; break ;
				 } ;
				break ;
		     case _Section: /* SECTION [name,name,...] */
				ZUS(tcb->sections) ;
				v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ; if (tcb->type == V4LEX_TknType_EOL) { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; break ; } ;	/* If nothing given then reset */
				for (;;)
				 { if (tcb->type == V4LEX_TknType_Keyword) { UCstrcpy(tcb->UCstring,tcb->UCkeyword) ; }
				    else if (tcb->type != V4LEX_TknType_String){ v_Msg(ctx,ctx->ErrorMsgAux,"CmdKWStr",tcb->type) ; goto fail ; } ;
				   UCstrcat(tcb->sections,tcb->UCstring) ;	/* Take remainder of line */
				   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
				   if (tcb->opcode != V_OpCode_Comma) { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; break ; } ;
				   UCstrcat(tcb->sections,UClit(",")) ;
				   v4lex_NextTkn(tcb,0) ;
				 } ;
				break ;
		    } ;
		 } ;
		break ;
	    } ;
	   tcb->type = V4LEX_TknType_EOL ;			/* Force end-of-line */
	 } ;
	return(V4EVAL_Res_ExitNoStats) ;


fail:						/* Error message in ctx->ErrorMsgAux */
	gpi->Compiling = 1 ;			/* Set so we don't do error stack, context, areas, etc. */
	goto fail_continue ;
fail_full:
	gpi->Compiling = 0 ;
fail_continue:
	if (nested) return(V4EVAL_Res_NestRetErr) ;
	v_Msg(ctx,NULL,"CmdError") ;
	v4_UCerror(0,tcb,"","","",ctx->ErrorMsg) ;
	return(V4EVAL_Res_NestRetErr) ;
}

LOGICAL v4eval_ChkEOL(tcb)
  struct V4LEX__TknCtrlBlk *tcb ;
{
	if (tcb->type == V4LEX_TknType_EOL) return(TRUE) ;
	if (tcb->opcode == V_OpCode_Semi) return(TRUE) ;
	v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
	if (tcb->type == V4LEX_TknType_EOL) return(TRUE) ;
	if (tcb->opcode == V_OpCode_Semi) return(TRUE) ;
	v_Msg(NULL,gpi->ctx->ErrorMsgAux,"CmdNotEOL",tcb->type) ; return(FALSE) ;
//	v4_error(V4E_NOTEOL,tcb,"V4","ChkEOL","NOTEOL","Expecting End-of-Line") ;
}


/*	v4eval_NextAsString - Parses next token(s) and returns as string				*/
/*	Call: ok = v4eval_NextAsString( ctx , tcb , tknflags , assok )
	  where ok = TRUE if OK, FALSE if error (in ctx->ErrorMsgAux),
		assok = TRUE if allow "=" & "+=" to go through			*/
/*	`v4pt - parsed & evaled, xxx* = lookup in context/table, xxx - keyword, "xxx" - string */
/*	Result always in tcb->UCstring, if possible, tcb->type & other tcb->xxx elements updated also to reflect non-string results */

LOGICAL v4eval_NextAsString(ctx,tcb,tknflags,assok)
  struct V4C__Context *ctx ;
  struct V4LEX__TknCtrlBlk *tcb ;
  int tknflags ; LOGICAL assok ;
{ P valpt,point,*dpt ;
  int i ; UCCHAR ucbuf[UCsizeof(tcb->UCkeyword)] ; double d1 ;

	v4lex_NextTkn(tcb,tknflags) ;
	switch (tcb->type)
	 { default:
		v_Msg(ctx,ctx->ErrorMsgAux,"CmdString",tcb->type) ; return(FALSE) ;
//		 v4_error(0,tcb,"V4","NextStr","NOTSTR","Expecting a keyword, string, context reference or `v4point") ;
	   case V4LEX_TknType_Punc:
		switch (tcb->opcode)
		 { default:
		    v_Msg(ctx,ctx->ErrorMsgAux,"CmdString",tcb->type) ; return(FALSE) ;
//		    v4_error(0,tcb,"V4","NextStr","NOTSTR","Expecting a keyword, string, context reference or `v4point") ;
		   case V_OpCode_Equal:
		   case V_OpCode_PlusEqual:
		    if (assok) return(TRUE) ;
		    v_Msg(ctx,ctx->ErrorMsgAux,"CmdStringAss",tcb->type) ; return(FALSE) ;
//		    v4_error(0,tcb,"V4","NextStr","NOTSTR","Assignment operator (=, +=) not allowed here") ;
		   case V_OpCode_BackQuote:
/*		    Have V4 point to parse & evaluate */
		    if (!v4dpi_PointParse(ctx,&valpt,tcb,V4DPI_PointParse_RetFalse)) return(FALSE) ;
//		     v4_error(0,tcb,"V4","NextStr","INVPT","Point parse error: %s",UCretASC(ctx->ErrorMsgAux)) ;
		    dpt = v4dpi_IsctEval(&point,&valpt,ctx,0,NULL,NULL) ;
		    if (dpt == NULL)
		     { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIsctFail",&valpt) ; return(FALSE) ;
//		       v4dpi_PointToString(ASCTBUF1,&valpt,ctx,V4DPI_FormatOpt_ShowDim) ;
//		       v_Msg(ctx,ASCTBUF1,"@No evaluation for: %1P",&valpt) ;
//		       v4_error(V4E_NOISCTEVALTOOUTPUT,tcb,"V4","Eval","NOISCTEVALTOOUTPUT",UCretASC(ASCTBUF1)) ;
		     } ;
		    v4dpi_PointToString(tcb->UCstring,dpt,ctx,V4DPI_FormatOpt_Echo) ;
//		    UCstrcatToASC(tcb->string,tcb->UCstring) ;
		    switch (dpt->PntType)
		     { default:			tcb->type = V4LEX_TknType_String ; break ;
		       case V4DPI_PntType_Int:	tcb->type = V4LEX_TknType_Integer ; tcb->Linteger = (tcb->integer = dpt->Value.IntVal) ; break ;
		       case V4DPI_PntType_Real:	tcb->type = V4LEX_TknType_Float ; GETREAL(tcb->floating,dpt) ; break ;
		     } ;
		    break ;
		 } ;
		break ;
	   case V4LEX_TknType_String:	break ;
	   case V4LEX_TknType_Keyword:
		if (tcb->ilvl[tcb->ilx].input_ptr[0] != UClit('*'))	/* Not great coding but legal way does not work! */
		 { UCstrcpy(tcb->UCstring,tcb->UCkeyword) ; break ; } ;
		UCstrcpy(ucbuf,tcb->UCkeyword) ;
		v4lex_NextTkn(tcb,tknflags) ;	/* Pop "*" off of input */
		if (gpi->vltCur != NULL)		/* If processing table then see if got a column spec */
		 { for(i=0;i<gpi->vltCur->Columns;i++) { if (UCstrcmpIC(gpi->vltCur->Col[i].Name,ucbuf) == 0) break ; } ;
		   if (i < gpi->vltCur->Columns)				/* Found it - copy current value into point */
		    { if (gpi->vltCur->Col[i].Cur == NULL || (gpi->vltCur->InXML ? (gpi->vltCur->Col[i].XMLFlags & V4LEX_Table_XML_Defined)==0 : FALSE))
		       { v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntColUnd",tcb->type,0,ucbuf,gpi->vltCur->Name) ; return(FALSE) ; } ;
//		       v4_error(0,tcb,"V4","Eval","NOCOLDEF","Table (%s) Column (%s) not defined",UCretASC(gpi->vltCur->Name),UCretASC(ucbuf)) ;
		      dpt = gpi->vltCur->Col[i].Cur ;
		    } ;
		 } else
		 { if((i = v4dpi_DimGet(ctx,ucbuf,DIMREF_REF)) == 0) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotDim",ucbuf) ; return(FALSE) ; } ;
		   DIMVAL(dpt,ctx,i) ;
		   if (dpt == NULL)
		    { v_Msg(ctx,ctx->ErrorMsgAux,"CtxNoCurVal2",i) ; return(FALSE) ; } ;
//		     v4_error(0,tcb,"V4","Eval","NOCTX","Dimension (%s) not defined in context",UCretASC(ucbuf)) ;
		 } ;
		v4dpi_PointToString(tcb->UCstring,dpt,ctx,V4DPI_FormatOpt_Echo) ;
//		UCstrcatToASC(tcb->string,tcb->UCstring) ;
		switch (dpt->PntType)
		 { default:			tcb->type = V4LEX_TknType_String ; break ;
		   case V4DPI_PntType_Int:	tcb->type = V4LEX_TknType_Integer ; tcb->Linteger = (tcb->integer = dpt->Value.IntVal) ; break ;
		   case V4DPI_PntType_Real:	tcb->type = V4LEX_TknType_Float ; GETREAL(tcb->floating,dpt) ; break ;
		 } ;
		break ;
	   case V4LEX_TknType_Float:
	   case V4LEX_TknType_LInteger:
	   case V4LEX_TknType_Integer:
		if (tcb->type == V4LEX_TknType_Float) { d1 = tcb->floating ; }
		 else { FIXEDtoDOUBLE(d1,tcb->Linteger,tcb->decimal_places) ; } ;
//		sprintf(tcb->string,"%g",d1) ;
		UCsprintf(tcb->UCstring,30,UClit("%g"),d1) ; break ;
	 } ;
	return(TRUE) ;
}

/*	v4eval_SetIfLevel - Sets level name for current IF statement	*/
/*	Call: v4eval_SetIfLevel(ctx,tcb)					*/

int v4eval_SetIfLevel(ctx,tcb)
  struct V4C__Context *ctx ;
  struct V4LEX__TknCtrlBlk *tcb ;
{ int i ;
	for(i=tcb->ilx;i>0;i--)
	 { if (tcb->ilvl[i].mode == V4LEX_InpMode_File)
	    { v_Msg(ctx,tcb->ifs[tcb->ifx].name,"@%1U line %2d",tcb->ilvl[i].file_name,tcb->ilvl[i].current_line) ;
	      break ;
	    } ;
	 } ;
	if (i <= 0) { v_Msg(ctx,tcb->ifs[tcb->ifx].name,"@%1d",tcb->ifx+1) ; } ;/* Default if-level name */
	tcb->ifs[tcb->ifx].ifxID = ++tcb->LastifxID ;
	v4lex_NextTkn(tcb,0) ;
	if (tcb->type != V4LEX_TknType_Punc) { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; return(TRUE) ; } ;
	v4lex_NextTkn(tcb,V4LEX_Option_ForceKW) ;
	if (tcb->type != V4LEX_TknType_Keyword)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdLvlSlash",tcb->type) ; return(FALSE) ; } ;
//	 v4_error(V4E_NOTIFNAME,tcb,"V4","Eval","NOTIFNAME","Expecting a keyword level name after slash") ;
	tcb->UCkeyword[UCsizeof(tcb->ifs[tcb->ifx].name)-1] = UCEOS ;
	UCstrcpy(tcb->ifs[tcb->ifx].name,tcb->UCkeyword) ;		/* Save the name */
	return(TRUE) ;
}

/*	v4eval_CheckIfLevel - Checks else/end for proper if level name	*/
/*	Call: ok = v4eval_CheckIfLevel( tcb )				*/

LOGICAL v4eval_CheckIfLevel(ctx,tcb)
  struct V4C__Context *ctx ;
  struct V4LEX__TknCtrlBlk *tcb ;
{
	v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
	if (tcb->type != V4LEX_TknType_Punc) { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; return(TRUE) ; } ;
	if (tcb->opcode != V_OpCode_Slash && tcb->opcode != V_OpCode_Colon)
	 { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; return(TRUE) ; } ;	/* Only continue if ":" or "/" */
	v4lex_NextTkn(tcb,V4LEX_Option_ForceKW) ;
	if (tcb->type != V4LEX_TknType_Keyword) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdLvlSlash",tcb->type) ; return(FALSE) ; } ;
//	  v4_error(V4E_NOTIFNAME,tcb,"V4","Eval","NOTIFNAME","Expecting a keyword level name after slash") ;
	if (UCstrcmpIC(tcb->ifs[tcb->ifx].name,tcb->UCkeyword) != 0)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdLvlName",tcb->type,tcb->UCkeyword,tcb->ifs[tcb->ifx].name) ; return(FALSE) ; } ;
//	 v4_error(V4E_BADIFNAME,tcb,"V4","Eval","BADIFNAME","Current IF level (%s) does not match: %s",
//		UCretASC(tcb->ifs[tcb->ifx].name),UCretASC(tcb->UCkeyword)) ;
	return(TRUE) ;
}


/*	v4eval_GetFileUpdateDT - Returns DateTime file last updated/created	*/
/*	Call: dt = v4eval_GetFileUpdateDT(ctx,file)
	  where dt is date time (-1 if error),
		ctx is current context,
		file is filename as string					*/

int v4eval_GetFileUpdateDT(ctx,file)
  struct V4C__Context *ctx ;
  UCCHAR *file ;
{
#if defined WINNT && defined V4UNICODE
 struct _stat stat_buf ;
#else
 struct stat stat_buf ;
#endif
  UCCHAR *bp ;

	bp = v_UCLogicalDecoder(file,VLOGDECODE_Exists,0,ctx->ErrorMsgAux) ;
	if (bp == NULL) return(-1) ;
	if (UCstat(bp,&stat_buf) == -1) return(-1) ;
	return((int)stat_buf.st_mtime - (60*gpi->MinutesWest) - TIMEOFFSETSEC) ;
}

//#ifdef WINNT
//LPTHREAD_START_ROUTINE v4eval_EscThread(pi)
//  struct V4C__ProcessInfo *pi ;
//{ int sin,sout,len,r ; char buf[4] ;
//
//	if (pi->EscFlag == UNUSED) SuspendThread(pi->hEscThread) ;
//	for(;;)
//	 { sin = GetStdHandle(STD_INPUT_HANDLE) ; sout = GetStdHandle(STD_OUTPUT_HANDLE) ;
//	   if (pi->EscFlag == UNUSED) { SuspendThread(pi->hEscThread) ; continue ; } ;
//	   if (WaitForSingleObject(sin,INFINITE) == WAIT_FAILED)
//	    { CloseHandle(pi->hEscThread) ; pi->hEscThread = UNUSED ; return ; } ;
//	   if (pi->EscFlag == UNUSED) { SuspendThread(pi->hEscThread) ; continue ; } ;
//	   r = ReadFile(sin,buf,1,&len,NULL) ; buf[1] = '\0' ;
//	   if (buf[0] <= ' ') continue ;
//	   if (buf[0] >= 'a') buf[0] = 'A' + buf[0] - 'a' ;
//	   if (pi->EscFlag == UNUSED) { SuspendThread(pi->hEscThread) ; continue ; } ;
//	   switch(buf[0])
//	    { default:		termWriteFile1(sout,"\a",1,&len,NULL) ; continue ;
//	      case 'B':
//	      case 'C':
//	      case 'Q':
//	      case 'T':		pi->EscFlag = buf[0] ; break ;
//	      case 'H':
//	      case '?':		termWriteFile1(sout,"b - Bindings, c - Context, q - Quit, t - Trace\n",48,&len,NULL) ; break ;
//	    } ;
//	 } ;
//}
//#endif
//
/*	v4eval_GetTable - Attempts to find or load table from V4 areas			*/
/*	Call: vlt = v4eval_GetTable( ctx , tablename , ok )
	  where vlt is point to found table def or NULL if problem (ctx->ErrorMsgAux with message),
		ctx is context,
		tablename is table name,
		ok if defined is updated TRUE if OK or error is simply table not found	*/

struct V4LEX__Table *v4eval_GetTable(ctx,tablename,ok)
  struct V4C__Context *ctx ;
  UCCHAR *tablename ;
  LOGICAL *ok ;
{ struct V4LEX__Table *vlt ;
  struct V4LEX__TknCtrlBlk *tcb ;
  UCCHAR tnbuf[256] ;
  struct UC__File UCFile ; int fid,fx ;
  static int didDEFAULTS = FALSE ;
  int i,save ; UCCHAR *b,*b1 ;
  UCCHAR tbuf[V4LEX_BigText_Max] ;
	if (CHECKV4INIT) return(NULL) ;
	if (ok != NULL) *ok = TRUE ;			/* Assume we will be ok */
//	for(i=0;;i++) { tablename[i] = toupper(tname[i]) ; if (tname[i] == '\0') break ; } ;

	for(vlt=gpi->vlt;vlt!=NULL;vlt=vlt->link)
	 { if (UCstrcmpIC(vlt->Name,tablename) == 0) break ; } ;
	if (vlt != NULL) return(vlt) ;
	{ struct V4LEX__MacroBody *vlmb ;
	  struct V4LEX__MacroEntry *vlme = v4eval_GetMacroEntry(ctx,tablename,V4LEX_MACTYPE_Table) ;
	  if (vlme == NULL)
	   { v_Msg(ctx,ctx->ErrorMsgAux,"TableNotDef",tablename) ; return(NULL) ; } ;
	  vlmb = (struct V4LEX__MacroBody *)&vlme->macData[vlme->bodyOffset] ;
	  UCstrcpy(tbuf,vlmb->macBody) ;	/* TEMP CODE UNTIL WE GET RID OF STUFF BELOW */
	}
/*	Got one - before we parse make sure we have "Table DEFAULTS" */
	if (!didDEFAULTS)
	 { didDEFAULTS = TRUE ;			/* Only do (or try) this once */
	   v4eval_GetTable(ctx,UClit("DEFAULTS"),&i) ;	/* It may or may not work, at least we tried */
	   if (!i) return(NULL) ;
	 } ;
	v_MakeOpenTmpFile(UClit("v4td"),tnbuf,UCsizeof(tnbuf),NULL,ctx->ErrorMsg) ;
	if ((fid=vout_OpenStreamFile(NULL,tnbuf,UClit("tmp"),NULL,FALSE,V4L_TextFileType_UTF16,0,ctx->ErrorMsgAux)) == UNUSED) return(NULL) ;
	fx = vout_FileIdToFileX(fid) ;
	for(b=tbuf;;b=b1+1)
	 { b1 = UCstrchr(b,'\r') ; if (b1 != NULL) *b1 = UCEOS ;
	   vout_UCTextFileX(fx,0,b) ; vout_NLFileX(fx) ; if (b1 == NULL) break ;
	 } ;
	vout_UCTextFileX(fx,0,UClit("Exit NoStatistics\n")) ;
	if (!vout_CloseFile(fid,UNUSED,ctx->ErrorMsgAux)) return(NULL) ;
	if (!v_UCFileOpen(&UCFile,tnbuf,UCFile_Open_Read,TRUE,ctx->ErrorMsgAux,0)) return(NULL) ;
	tcb = v4mm_AllocChunk(sizeof *tcb,FALSE) ; v4lex_InitTCB(tcb,0) ; v4lex_NestInput(tcb,&UCFile,tnbuf,V4LEX_InpMode_TempFile) ;
	save = gpi->SaveTables ; gpi->SaveTables = FALSE ;	/* Turn this off so we don't loop forever */
	i = v4eval_Eval(tcb,ctx,TRUE,0,FALSE,TRUE,FALSE,NULL) ;	/* Parse the definition */
	if (tcb->ilx != 0) v4lex_CloseOffLevel(tcb,0) ;		/* If we did not close off the tempfile level then do it now */
	v4lex_FreeTCB(tcb) ;
	if ((i & V4EVAL_ResMask) == V4EVAL_Res_NestRetErr)							/* Got an error ? */
	 { UCstrcpy(UCTBUF1,ctx->ErrorMsgAux) ; v_Msg(ctx,ctx->ErrorMsgAux,"TableDefErr",tablename,UCTBUF1) ;
	   if (ok != NULL) *ok = FALSE ; return(NULL) ;
	 } ;
	gpi->SaveTables = save ;

	v_UCFileClose(&UCFile) ;

/*	See if we can now find the table */	
	for(vlt=gpi->vlt;vlt!=NULL;vlt=vlt->link)
	 { if (UCstrcmpIC(vlt->Name,tablename) == 0) break ; } ;
	return(vlt) ;		/* vlt will be NULL if we could not find table */

//no_load:
//	v_Msg(ctx,ctx->ErrorMsgAux,"TableNotDef",tablename) ;
//	if (ok != NULL) *ok = FALSE ;
//	return(NULL) ;
}

/*	B I N D I N G   M O D U L E S					*/

/*	v4dpi_BindListMake - Updates BindList from a new bind point, updates area via ctx */
/*	Call: ok = v4dpi_BindListMake( bindpt , valpt , isctarg , ctx , areaidptr , bindwgtadj , flags , relhX )
	  where ok is TRUE if all is well, FALSE if error (ctx->ErrorMsgAux)
	        bindpt is new binding,
		valpt is pointer to value point,
		isctarg is pointer to intersection (NULL to flush out current nbl),
		nblptr if non NULL is updated with nbl created,
		ctx is current context,
		areaidptr if non NULL is updated with area id used to store binding,
		bindwgtadj is optional initial setting for binding weight,
		flags is options (V4DPI_BindLM_xxx),
		relhX is index into RelH[] to write this binding (or DFLTRELH for auto-select)	*/

LOGICAL v4dpi_BindListMake(bindpt,valpt,isctarg,ctx,areaidptr,bindwgtadj,flags,relhX)
  struct V4DPI__Binding *bindpt ;
  struct V4DPI__Point *valpt ;
  struct V4DPI__Point *isctarg ;
  struct V4C__Context *ctx ;
  int *areaidptr ;
  int bindwgtadj,flags ;
  INDEX relhX ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__AreaCB *acb ;
  struct V4DPI__AreaBindInfo *abi,*abiptr ;
  struct V4DPI__DimInfo *di ;
  struct V4DPI__Point *isct ;
  struct V4DPI__Point *ipt,*vpt ;
  static struct V4DPI__LittlePoint NIdAll ;
  struct V4L__ListPoint *lp ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4DPI__Point *isctpt,*ipnt,*npnt,*cpnt,isctbuf,*dimpt,bepnt,beres,ftnisct ;
  struct V4DPI__BindList *nbl,*dataptr ;
  struct V4DPI__BindListShell nbls,*nblsptr ;
  struct V4DPI__BindList_Value nblv,*nblvptr ;
  struct V4DPI__BindList_Dim nbld,*nbldptr ;
  struct lcl__nbl
   { int gotnew,gotrec,aid ;
     struct V4DPI__BindListShell snbls ;
   } ;
  char dvpbuf[V4DPI_BindList_BufMax] ;
  int dimval,dimval2,dimvalpnttype ; int *lpti,chx ; DATAID dataId ;
  int lrx,addnew,px,ix,gotnew,i,j,k,fx,bytes,dvpbytes,olddimcnt,dimid,dimx,wgt,tfctr,nested,relhnum,bindmax,nblsBytes,flatten ;
  int beflag ; static int Dim_Value = UNUSED ; AREAID aid ; INDEX areax ;

	if (CHECKV4INIT) return(FALSE) ;
	if (NIdAll.Bytes != V4DPI_PointHdr_Bytes)		/* Set up point for NId.. */
	 { NIdAll.Dim = Dim_NId ; NIdAll.PntType = V4DPI_PntType_Special ;
	   NIdAll.Bytes = V4DPI_PointHdr_Bytes ; NIdAll.Grouping = V4DPI_Grouping_All ;
	 } ;
	if (isctarg == NULL) goto ret_ok ;
	if (ISQUOTED(isctarg)) isctarg = UNQUOTEPTR(isctarg) ;
	(gpi->BindingCount) ++ ;
/*	Lets check out the intersection for any nested intersections- maybe evaluate */
	if (isctarg->LHSCnt != 0)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"@Cannot bind with intersection containing \"|\"") ; goto ret_fail ; } ;
	for((ipnt=ISCT1STPNT(isctarg),nested=FALSE,ix=0);ix<isctarg->Grouping;(ix++,ADVPNT(ipnt)))
	 { switch (ipnt->PntType)
	    { default:						i = 0 ; break ;
	      case V4DPI_PntType_Isct:				i = 1 ; break ;
//	      case V4DPI_PntType_Context:			i = 2 ; break ;
	      case V4DPI_PntType_Special:
			switch (ipnt->Grouping)
			 { default:				i = 1 ; break ;
			   case V4DPI_Grouping_AllCnf:		i = 0 ; break ;
			   case V4DPI_Grouping_All:		i = 0 ;
								if (memcmp(ipnt,&NIdAll,NIdAll.Bytes) == 0)
								 { 
//								   v4dpi_PointToString(ASCTBUF1,valpt,ctx,-1) ;
								   v_Msg(ctx,UCTBUF2,"*BindNIdAll",&NIdAll,isctarg,valpt) ; vout_UCText(VOUT_Warn,0,UCTBUF2) ;
								 } ;
								break ;
			   case V4DPI_Grouping_Current:		i = 3 ; break ;
			   case V4DPI_Grouping_PCurrent:	i = 4 ; break ;
			   case V4DPI_Grouping_New:		i = 5 ; break ;
			 } ; break ;
	      case V4DPI_PntType_List:				i = (ipnt->ForceEval ? 6 : 0) ; break ;
	      case V4DPI_PntType_SymDef:			i = 1 ; break ;
	    } ;
	   if (i == 0)
	    { if (nested)
	       { memcpy(npnt,ipnt,ipnt->Bytes) ; ADVPNT(npnt) ;
	       } ;
	      continue ;
	    } ;
	   if (!nested)						/* Got nested intersection- if first time then set up */
	    { memcpy(&isctbuf,isctarg,(char *)ipnt - (char *)isctarg) ; /* Copy up to this point/intersection */
	      nested = TRUE ; npnt = (P *)((char *)&isctbuf + ( (char *)ipnt - (char *)isctarg )) ;
	    } ;
	   switch (i)
	    { case 1:	/* Isct - eval & sub in result */
	      case 5:	/* New - let isct module evaluate! */
		if (v4dpi_IsctEval(npnt,ipnt,ctx,0,NULL,NULL) == NULL)
	         { v_Msg(ctx,ctx->ErrorMsgAux,"BindEvalFail",ipnt,isctarg) ; goto ret_fail ;
	         } ;
		if (i == 5)					/* If dim+ then place into context also */
		 if (!v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,npnt,0,0)) goto ret_fail ;
		if (npnt->PntType == V4DPI_PntType_List && ipnt->ForceEval)
		 { memcpy(&bepnt,npnt,npnt->Bytes) ; cpnt = &bepnt ; goto list_expand ; } ;
		break ;
	      case 3:	/* Current - Get context value and sub in */
		DIMVAL(cpnt,ctx,ipnt->Dim) ;
		if (cpnt == NULL)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CtxNoCurVal2",ipnt->Dim) ; goto ret_fail ;
		 } ;
cur_entry:	if (cpnt->PntType == V4DPI_PntType_Shell)	/* Got a shell dimension? */
		 { if (!ISQUOTED(cpnt))				/*  and not quoted */
		    cpnt = (P *)&cpnt->Value ;			/*   then pull actual point, not shell */
		 } 
		if (cpnt->PntType == V4DPI_PntType_List && ipnt->ForceEval) goto list_expand ;
		cpnt->AltDim = ipnt->AltDim ;			/* Copy status of '~' prefix */
		memcpy(npnt,cpnt,cpnt->Bytes) ;			/* Copy context point into intersection */
		break ;
	      case 4:	/* PCurrent */
		DIMPVAL(cpnt,ctx,ipnt->Dim) ;
		if (cpnt == NULL)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CtxNoPriorVal2",UNUSED,ipnt->Dim) ; goto ret_fail ;
		 } ;
		goto cur_entry ;
	       case 6:	/* `list - Expand into multiple points */
		cpnt = ipnt ;
list_expand:



		lp = ALIGNLP(&cpnt->Value) ;
		for(j=1;;j++)
		 { if (v4l_ListPoint_Value(ctx,lp,j,npnt) <= 0) break ;
		   ADVPNT(npnt) ;
		   isctbuf.Grouping ++ ;
		 } ; ;
		isctbuf.Grouping -- ;


//		v_Msg(ctx,ASCTBUF1,"*BindDubiousPt",isctarg,cpnt,cpnt->PntType) ;
//		vout_Text(VOUT_Warn,0,ASCTBUF1) ; vout_NL(VOUT_Warn) ; break ;
		break ;
	    } ;
	   switch (npnt->PntType)
	    { default:				break ;
	      case V4DPI_PntType_Isct:
	      case V4DPI_PntType_BigIsct:
	      case V4DPI_PntType_PntIdx:
	      case V4DPI_PntType_MemPtr:
	      case V4DPI_PntType_BigText:
//	      case V4DPI_PntType_PIntMod:
//	      case V4DPI_PntType_Binding:
	      case V4DPI_PntType_OSHandle:
//	      case V4DPI_PntType_AggVal:
	      case V4DPI_PntType_QIsct:
	      case V4DPI_PntType_TagVal:
	      case V4DPI_PntType_V4IS:
	      case V4DPI_PntType_IMArg:
	      case V4DPI_PntType_MDArray:
	      case V4DPI_PntType_RegExpPattern:
	      case V4DPI_PntType_List:
		v_Msg(ctx,UCTBUF1,"*BindDubiousPt",isctarg,npnt,npnt->PntType) ;
		vout_UCText(VOUT_Warn,0,UCTBUF1) ; break ;
	    } ;
	   ADVPNT(npnt) ;
	 } ;
	if (nested) { ISCTLEN(&isctbuf,npnt) ;isctpt = (P *)&isctbuf ; }
	 else { isctpt = isctarg ; } ;
	vpt = valpt ;
	if (valpt->Grouping == V4DPI_Grouping_PCurrent)		/* Pull value from context? */
	 { DIMPVAL(vpt,ctx,valpt->Dim) ;
	   if (vpt == NULL)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"CtxNoPriorVal2",UNUSED,valpt->Dim) ; goto ret_fail ;
	    } ;
	 } else if (valpt->Grouping == V4DPI_Grouping_Current)		/* Pull value from context? */
	 { DIMVAL(vpt,ctx,valpt->Dim) ;
	   if (vpt == NULL)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"CtxNoCurVal3",valpt->Dim,valpt) ; goto ret_fail ;
	    } ;
	 } ;
	flatten = FALSE ;						/* Set TRUE if we attempt to flatten */
flatten_restart:							/* Here to start over again if we flattened the binding isct */
	fx = 0 ;
	memcpy(&bindpt->Buffer[fx],vpt,vpt->Bytes) ;			/* Copy value */
	fx += ALIGN(vpt->Bytes) ;
	bindpt->IsctIndex = fx ;					/* Remember where Isct starts */
	memcpy(&bindpt->Buffer[fx],isctpt,isctpt->Bytes) ;		/* & intersection */
	fx += ALIGN(isctpt->Bytes) ;
	bindpt->kp.fld.AuxVal = V4IS_SubType_Value ; bindpt->kp.fld.KeyType = V4IS_KeyType_V4 ;
	bindpt->kp.fld.KeyMode = V4IS_KeyMode_Int ; bindpt->kp.fld.Bytes = V4IS_IntKey_Bytes ;
	bindpt->Bytes = (char *)&bindpt->Buffer[fx] - (char *)bindpt ;	/* Determine length */
	bindpt->BindWgt = (bindwgtadj >= V4DPI_BindWgt_PrecOffset ? bindwgtadj - V4DPI_BindWgt_PrecOffset : 0) ;

/*	Look for dimension with potential in the binding (& also whether or not to flatten) */
	isct = (P *)&bindpt->Buffer[bindpt->IsctIndex] ;		/* Get pointer to intersection */
	tfctr = 0 ;							/* No time factor (yet) */
	wgt = bindwgtadj * 10 ;						/* Get initial weight */
	for(bindmax=0,dimx=(-1),i=0,ipt=ISCT1STPNT(isct);i<isct->Grouping;i++)
	 { DIMINFO(di,ctx,ipt->Dim) ;
	   if (!flatten && (di->rtFlags & V4DPI_rtDimInfo_Flatten) && ipt->PntType != V4DPI_PntType_Special)
	    { flatten = TRUE ;						/* Only want to try this once */
	      isct = (P *)&bindpt->Buffer[bindpt->IsctIndex] ;
	      if (v4dpi_FlattenIsct(ctx,isct,&ftnisct,TRUE)) { isctpt = &ftnisct ; goto flatten_restart ; } ;
/*	      If did not flatten then just drop thru as if nothing had happened */
	    } ;
	   if (di == NULL)
	    { 
//	      v4dpi_PointToString(ASCTBUF1,isct,ctx,-1) ;
	      v_Msg(ctx,UCTBUF2,"@*No dimension on i=%1d: %2P\n",i,isct) ;
	      vout_UCText(VOUT_Err,0,UCTBUF2) ; continue ;
	    } ;
	   if (di->BindList > bindmax)					/* Must also be regular, single point! */
	    { if (ipt->PntType == V4DPI_PntType_AggRef)
	       { dimx = i ; bindmax = di->BindList ; dimid = ipt->Dim ; beflag = di->Flags & V4DPI_DimInfo_BindEval ;
	         dimval = ipt->Value.IntVal ; dimval2 = ipt->Grouping ; dimvalpnttype = ipt->PntType ; dimpt = ipt ;
	       } else
	      if (ipt->Grouping == V4DPI_Grouping_Single && ipt->PntType != V4DPI_PntType_Special)
	       { dimx = i ; bindmax = di->BindList ; dimid = ipt->Dim ; beflag = di->Flags & V4DPI_DimInfo_BindEval ;
	         dimval = ipt->Value.IntVal ; dimval2 = ipt->Value.Int2Val[1] ; dimvalpnttype = ipt->PntType ; dimpt = ipt ;
	       } ;
	    } ;
	   if (di->PointType == V4DPI_PntType_Time)			/* If time (>700M) then convert to reduce size */
	    { tfctr = (ipt->Value.IntVal > 700000000 ? (ipt->Value.IntVal-700000000)/10 : ipt->Value.IntVal) ; } ;
	   switch (ipt->Grouping)
	    { default:
//071004	if (di->PointType == V4DPI_PntType_Decomp) { wgt += 3 ; }
//	    	 else { wgt += (ipt->Grouping == V4DPI_Grouping_Single ? 10 : 7) ; } ;
	    	wgt += (ipt->Grouping == V4DPI_Grouping_Single ? 10 : 7) ;
		break ;
	      case V4DPI_Grouping_AllCnf:
	      case V4DPI_Grouping_All:		wgt += 4 ; break ;
	      case V4DPI_Grouping_PCurrent:
	      case V4DPI_Grouping_Current:
		UCstrcpy(ctx->ErrorMsgAux,UClit("Cannot use \"{CONTEXT}\" in binding")) ; goto ret_fail ;
	    } ;
	   ADVPNT(ipt) ;		/* No find- advance to next */
	 } ;
	if (dimx < 0)						/* If no dimension with Binding Value, try AllValues */
	 { for((bindmax=0,dimx=(-1),i=0,ipt=ISCT1STPNT(isct));i<isct->Grouping;i++)
	    { DIMINFO(di,ctx,ipt->Dim) ;
	      if (di == NULL) continue ;
	      if (di->BindList > bindmax)
	       { if ((ipt->Grouping == V4DPI_Grouping_All || ipt->Grouping == V4DPI_Grouping_AllCnf) && ipt->PntType == V4DPI_PntType_Special && di->AllValue != 0)
	          { dimx = i ; bindmax = di->BindList ; dimid = ipt->Dim ; beflag = di->Flags & V4DPI_DimInfo_BindEval ;
	            dimval = di->AllValue ; dimval2 = 0 ; dimvalpnttype = ipt->PntType ; dimpt = ipt ;
	          } ;
	       } ;
	      ADVPNT(ipt) ;
	    } ;
	 } ;
	if (dimx < 0)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"BindNoDimBndPot",isctarg) ; goto ret_fail ;
	 } ;
	if (beflag && Dim_Value == UNUSED)
	 { Dim_Value = v4dpi_DimGet(ctx,UClit("VALUE"),DIMREF_IRT) ;
	   if (Dim_Value == 0)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"BindNoDimValue") ; goto ret_fail ;
	    } ;
	 } ;
	if (beflag && Dim_Value > 0)				/* Try to handle binding as an eval? (only if got Dim:Value!) */
	 { memcpy(&bepnt,isctpt,isctpt->Bytes) ;		/* Make copy of isct */
	   ipnt = (P *)( (char *)&bepnt + bepnt.Bytes ) ;	/* Get to end for Value:xxx point */
	   ZPH(ipnt) ; ipnt->PntType = V4DPI_PntType_Shell ;
	   ipnt->Dim = Dim_Value ; ipnt->Bytes = V4DPI_PointHdr_Bytes + vpt->Bytes ;
	   memcpy(&ipnt->Value,vpt,vpt->Bytes) ;
	   bepnt.Grouping++ ; bepnt.Bytes += ipnt->Bytes ;
	   if (traceGlobal & V4TRACE_BindEval)
	    { 
//	      v4dpi_PointToString(ASCTBUF1,&bepnt,ctx,-1) ;
	      v_Msg(ctx,UCTBUF2,"@*BindEval: %1P\n",&bepnt) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	    } ;
	   ipnt = v4dpi_IsctEval(&beres,&bepnt,ctx,0,NULL,NULL) ;
	   if (traceGlobal & V4TRACE_BindEval)
	    { v_Msg(ctx,UCTBUF2,"@*BindEvalRes: %1U\n",(ipnt == NULL ? "Failed" : "OK!")) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	    } ;
	   if (ipnt != NULL) goto ret_ok ;
/*	   If failed because primary binding not found then continue below, if because nested isct/mod failed then fail here */
	   if (ctx->FailProgress == V4FailProgress_NestEval)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"BindEvalNstFail",isct,valpt) ; goto ret_fail ; } ;
	 } ;
	if (bindpt->BindWgt == 0) bindpt->BindWgt = (tfctr == 0 ? wgt : (wgt<<24)+tfctr) ;
/*	Figure out area this binding belongs to */
	if (gpi->RelH[di->RelHNum].ahi.BindingsUpd && relhX == DFLTRELH)
	 { relhnum = di->RelHNum ; }				/* Default Hnum for Dim allows updates- OK */
	 else { for(relhnum=(relhX == DFLTRELH ? gpi->HighHNum : relhX);relhnum>=gpi->LowHNum;relhnum--)	/* Find highest area allowing new entries */
	 	 { if (gpi->RelH[relhnum].aid == UNUSED) continue ; if (relhnum == V4DPI_WorkRelHNum) continue ;
		   if (gpi->RelH[relhnum].ahi.BindingsUpd) break ;
		 } ;
		if (relhnum < gpi->LowHNum && gpi->RelH[V4DPI_WorkRelHNum].aid != UNUSED)
		 relhnum = V4DPI_WorkRelHNum ;			/* Assign to work area as last resort */
		if (relhnum < gpi->LowHNum)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"@Could not find any Area available for binding updates") ; goto ret_fail ; } ;
	      } ;
	aid = gpi->RelH[relhnum].aid ;
	if (aid == UNUSED)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"@Area selected for BIND is not currently open") ; goto ret_fail ; } ;
	FINDAREAINPROCESS(areax,aid)
	if (areaidptr != NULL) *areaidptr = aid ;			/* Return Area Id if requested by caller */
/*	Now that we got Area ID, maybe write out bind point value */
	if (vpt->Bytes > V4DPI_ShortPoint_Limit || vpt->Grouping != V4DPI_Grouping_Single)	/* Fit in short point ? */
	 { acb = v4mm_ACBPtr(aid) ;								/*  No - have to write it out */
#ifdef TRACEGRAB
printf("blmake %d\n",mmm->Areas[areax].areaLock) ;
#endif
	   GRABAREAMTLOCK(areax) ;
	   LOCKROOT ; bindpt->ValueId = acb->RootInfo->NextAvailValueNum++ ;	/* Assign value number */
	   RELROOT ;
	   v4is_Insert(aid,(struct V4IS__Key *)bindpt,bindpt,bindpt->Bytes,V4IS_PCB_DataMode_Auto,V4IS_DataCmp_None,0,FALSE,FALSE,0,NULL) ;
	   FREEAREAMTLOCK(areax) ;
	 } else bindpt->ValueId = -1 ;

/*	Maybe flush this point out of the "loser" cache */
	if (dimpt->Bytes <= sizeof(struct V4DPI__LittlePoint))
	 { lpti = (int *)dimpt ; chx = 0 ;
	   for(i=0;i<(dimpt->Bytes/(sizeof(int)));i++) { chx += *lpti ; lpti++ ; } ;
	   chx = chx % V4C_Cache_Max ; if (chx < 0) chx = -chx ;
	   ZPH(&ctx->Cache[chx].lpt) ;
	 } ;
/*	Try and pull any existing info for this dimension point */
	nbls.blsk.KeyType = V4IS_KeyType_Binding ; nbls.blsk.AuxVal = dimid ;
	switch (dimvalpnttype)
	 { default:
		nbls.blsk.KeyVal.IntVal = dimval ;
		nbls.blsk.KeyMode = V4IS_KeyMode_Int ; nbls.blsk.Bytes = V4IS_IntKey_Bytes ;
		break ;
	   case V4DPI_PntType_BinObj:
		i = dimpt->Bytes - V4DPI_PointHdr_Bytes ; if (i == 0 || i > V4IS_KeyBytes_Max-10) i =  V4IS_KeyBytes_Max-10 ;
		memcpy(&nbls.blsk.KeyVal.Alpha,&dimpt->Value.AlphaVal,i) ;
		for(j=i;j<i+3;j++) { nbls.blsk.KeyVal.Alpha[j] = 0 ; } ;		/* Pad with nulls */
		i = ALIGN(i) ;
		nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = sizeof(union V4IS__KeyPrefix) + i ;
		break ;
	   case V4DPI_PntType_Complex:
		memcpy(&nbls.blsk.KeyVal.Alpha,&dimpt->Value.Complex,sizeof dimpt->Value.Complex) ;
		nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = ALIGN(sizeof(union V4IS__KeyPrefix) + sizeof dimpt->Value.Complex) ;
		break ;
	   case V4DPI_PntType_GeoCoord:
		memcpy(&nbls.blsk.KeyVal.Alpha,&dimpt->Value.GeoCoord,sizeof dimpt->Value.GeoCoord) ;
		nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = ALIGN(sizeof(union V4IS__KeyPrefix) + sizeof dimpt->Value.GeoCoord) ;
		break ;
	   CASEofChar
		i = dimpt->Value.AlphaVal[0] ; if (i == 0 || i > V4IS_KeyBytes_Max-10) i =  V4IS_KeyBytes_Max-10 ;
		memcpy(&nbls.blsk.KeyVal.Alpha,&dimpt->Value.AlphaVal[1],i) ;
		for(j=i;j<i+3;j++) { nbls.blsk.KeyVal.Alpha[j] = 0 ; } ;		/* Pad with nulls */
		i = ALIGN(i) ;
		nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = sizeof(union V4IS__KeyPrefix) + i ;
		break ;
	   case V4DPI_PntType_Calendar:
	   case V4DPI_PntType_Real:
	   case V4DPI_PntType_TeleNum:
	   case V4DPI_PntType_Int2:
	   case V4DPI_PntType_AggRef:
//	   case V4DPI_PntType_XDB:
		nbls.blsk.KeyVal.Int2Val[0] = dimval ; nbls.blsk.KeyVal.Int2Val[1] = dimval2 ;
		nbls.blsk.KeyMode = V4IS_KeyMode_Int2 ; nbls.blsk.Bytes = V4IS_Int2Key_Bytes ;
		break ;
	 } ;
	nbl = (struct V4DPI__BindList *)((char *)&nbls + nbls.blsk.Bytes) ;	/* Put nbl in proper position */
	i = TRUE ;
	if (i)
	 { 
#ifdef TRACEGRAB
printf("blmake2 %d\n",mmm->Areas[areax].areaLock) ;
#endif
	   GRABAREAMTLOCK(areax) ;
	   if (v4is_PositionKey(aid,(struct V4IS__Key *)&nbls,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) == V4RES_PosKey)	/* See if anything exists */
	    { 
	      nblsptr = (struct V4DPI__BindListShell *)v4dpi_GetBindBuf(ctx,aid,ikde,FALSE,NULL) ;
	      dataptr = (struct V4DPI__BindList *)((char *)nblsptr + nblsptr->blsk.Bytes) ;
	      memcpy(nbl,dataptr,(char *)&dataptr->Buffer[dataptr->DimStart]-(char *)dataptr) ;/* Header & values to stand-alone */
	      nbldptr = (struct V4DPI__BindList_Dim *)&dataptr->Buffer[dataptr->DimStart] ;	/* Pointer to Dimension list */
	      memcpy(&nbld,nbldptr,dataptr->PointStart-dataptr->DimStart) ;/* Copy into stand-alone buffer */
	      dvpbytes = (char *)dataptr + dataptr->Bytes - (char *)&dataptr->Buffer[dataptr->PointStart] ;
	      memcpy(&dvpbuf,&dataptr->Buffer[dataptr->PointStart],dvpbytes) ; /* Copy dim=val points */
	      FREEAREAMTLOCK(areax) ;
	      gotnew = FALSE ; dataId = ikde->DataId ;
	    } else
	    { FREEAREAMTLOCK(areax) ;
	      nbl->Bytes = 0 ; nbl->ValueCnt = 0 ; nbl->ValueStart = 0 ; nbl->PointCnt = 0 ; nbl->DimStart = 0 ;
	      nbld.DimCnt = 0 ; dvpbytes = 0 ;
	      gotnew = TRUE ;
	    } ;
	 } ;
/*	Set up new nblv */
	if (bindpt->ValueId == -1)					/* Use short point ? */
	 { vpt = (P *)&bindpt->Buffer[0] ;		/* Yes - link up to big point */
	   nblv.sp.Dim = vpt->Dim ; nblv.sp.Bytes = vpt->Bytes ; nblv.sp.PntType = vpt->PntType ; nblv.sp.LHSCnt = vpt->LHSCnt ;
	   memcpy(&nblv.sp.Value,&vpt->Value,vpt->Bytes-V4DPI_PointHdr_Bytes) ;
	 } else { nblv.sp.Bytes = 0 ; nblv.sp.Value.IntVal = bindpt->ValueId ; } ;
	nblv.BindWgt = bindpt->BindWgt ; nblv.IndexCnt = 0 ; nblv.BindFlags = flags ;
	nblv.BindId = (++nbl->LUBindId) ;				/* Assign unique id */
/*	Loop thru all points in binding's intersection & add/match with existing */
	olddimcnt = nbld.DimCnt ;					/* Remember how many different dim points at start */
	for((px=0,ipt=ISCT1STPNT(isct));px<isct->Grouping;(px++,ADVPNT(ipt)))
	 {
	   for(i=0;i<nbld.DimCnt;i++)
	    { if (memcmp(&dvpbuf[nbld.DimEntryIndex[i]],ipt,ipt->Bytes) == 0) break ; } ;
	   if (nblv.IndexCnt >= V4DPI_IsctDimMax)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"@BindList exceeds max total dimensions (%1d): Bind %2P",V4DPI_IsctDimMax,isctarg) ; goto ret_fail ; } ;
	   if (i < nbld.DimCnt)
	    { nblv.DimPos[nblv.IndexCnt++] = i ;			/* Already have dim=val, just copy index */
	    } else
	    { 
	      if (nbld.DimCnt > 255)					/* DimPos is currently unsigned char ! */
	       { v_Msg(ctx,ctx->ErrorMsgAux,"@BindList(1) exceeds max DimCnt (%1d): Bind %2P",255,isctarg) ; goto ret_fail ; } ;
	      nblv.DimPos[nblv.IndexCnt++] = nbld.DimCnt ;		/* Have to add new one */
	      if (nbld.DimCnt >= V4DPI_BindList_DimMax)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"@BindList(2) exceeds max DimCnt (%1d): Bind %2P",V4DPI_BindList_DimMax,isctarg) ; goto ret_fail ; } ;
	      nbld.DimEntryIndex[nbld.DimCnt] = dvpbytes ;
	      nbld.DimCnt++ ;
	      memcpy(&dvpbuf[dvpbytes],ipt,ipt->Bytes) ;
	      dvpbytes += ipt->Bytes ; nbl->PointCnt ++ ;
	    } ;
	 } ;
	nblv.Bytes = ALIGN((char *)&nblv.DimPos[nblv.IndexCnt+1] - (char *)&nblv) ;
	nblvptr = (BLV *)&nbl->Buffer[0] ;	/* Get to first value */
	addnew = TRUE ;							/* Assume we are going to have to add new value */
	for(i=0;i<nbl->ValueCnt;(i++,nblvptr =(BLV *)((char *)nblvptr + nblvptr->Bytes)))
	 { if (nblv.BindWgt >= nblvptr->BindWgt)			/* Find position in list based on bind weight */
	    { if (olddimcnt == nbld.DimCnt && nblv.BindWgt == nblvptr->BindWgt)
/*		 Did not add any new dimensions, wgt is same, maybe got exact match - check it out */
	       { for(;i<nbl->ValueCnt && nblv.BindWgt==nblvptr->BindWgt;(i++,nblvptr =(BLV *)((char *)nblvptr + nblvptr->Bytes)))
		  { if (nblv.IndexCnt != nblvptr->IndexCnt) continue ;	/* Different number of dimensions */
		    addnew = FALSE ;					/* Maybe got the same (just maybe) */
		    for(j=0;j<nblv.IndexCnt;j++)
		     { for (k=0;k<nblv.IndexCnt;k++) { if (nblv.DimPos[j] == nblvptr->DimPos[k]) break ; } ;
		       if (k >= nblv.IndexCnt) { addnew = TRUE ; break ; } ; /* If no match then try next */
		     } ;
		    if (addnew) { continue ; } else goto at_proper_pos ;	/* If addnew then no match-keep looking */
		  } ;
	       } ;
	      break ;
	    } ;
	 } ;
/*	At proper position - update value or add new? */
at_proper_pos:
	if (addnew)
	 { memmove((char *)nblvptr+nblv.Bytes,nblvptr,(char *)&nbl->Buffer[nbl->DimStart]-(char*)nblvptr) ;/* Make hole */
	   memcpy(nblvptr,&nblv,nblv.Bytes) ;		/* and copy into correct spot */
	   nbl->ValueCnt ++ ;
	 } else
	 { memcpy(&nblvptr->sp,&nblv.sp,sizeof nblv.sp) ;		/* Just update value/value-id */
	 } ;
/*	All done - put everything back together into one nbl structure */
	fx = 0 ;
	for(i=0;i<nbl->ValueCnt;i++)
	 { nblvptr = (BLV *)&nbl->Buffer[fx] ;	/* Pointer to next nblv */
	   fx += nblvptr->Bytes ;					/* Advance to next one */
	 } ;
	nbl->DimStart = ALIGN(fx) ;
	bytes = (char *)&nbld.DimEntryIndex[nbld.DimCnt] - (char *)&nbld ;
	memcpy(&nbl->Buffer[fx],&nbld,bytes) ;				/* Copy Dimension list */
	fx += ALIGN(bytes) ;
	nbl->PointStart = fx ;
	if (fx + dvpbytes >= sizeof(nbl->Buffer))
	 { v4ctx_HasHaveNBL(ctx,dimid) ;		/* Maybe re-synch context point */
	   v4dpi_GetBindBuf(ctx,UNUSED,NULL,FALSE,NULL) ;	/* Flush the cache */
	   v4ctx_FrameCacheClear(ctx,FALSE) ;			/* Clear out the Ctx nbl cache */
	   v4dpi_IsctEvalCacheClear(ctx,FALSE) ;			/* Clear out the IsctEval cache */
	   v_Msg(ctx,ctx->ErrorMsgAux,"@BindList exceeds max size (%1d): Bind %2P",sizeof(nbl->Buffer),isctarg) ;
	   goto ret_fail ;
	 } ;
	memcpy(&nbl->Buffer[fx],&dvpbuf,dvpbytes) ;			/* Copy dim=value points */
	fx += ALIGN(dvpbytes) ;


//VEH041118 - nbl->Bytes should not be same as nbls, subtract out length of nbls key
//	nbl->Bytes = (char *)&nbl->Buffer[fx] - (char *)&nbls ;
	nblsBytes = ((char *)&nbl->Buffer[fx] - (char *)&nbls) ;
	nbl->Bytes = nblsBytes - nbls.blsk.Bytes ;



	acb = v4mm_ACBPtr(aid) ;
	if (nblsBytes >= acb->RootInfo->BktSize-100)
	 { v4dpi_ExamineBindList(nbl,ctx,aid) ;
	   v_Msg(ctx,ctx->ErrorMsgAux,"@BindList exceeds bucket size (%1d) for area",acb->RootInfo->BktSize) ;
	   goto ret_fail ;
	 } ;
/*	Now write out to current area */
#ifdef TRACEGRAB
printf("blmake3 %d\n",mmm->Areas[areax].areaLock) ;
#endif
	GRABAREAMTLOCK(areax) ;
	if (gotnew)
	 { v4is_Insert(aid,(struct V4IS__Key *)&nbls,&nbls,nblsBytes,V4IS_PCB_DataMode_Auto,V4IS_DataCmp_None,0,FALSE,FALSE,0,NULL) ;
	 } else
	 { v4is_Replace(aid,(struct V4IS__Key *)&nbls,&nbls,&nbls,nblsBytes,V4IS_PCB_DataMode_Auto,V4IS_DataCmp_None,dataId,0) ;
	 } ;
	FREEAREAMTLOCK(areax) ;
	if (traceGlobal & V4TRACE_Bindings)
	 { v4dpi_PointToStringML(UCTBUF1,valpt,ctx,V4DPI_FormatOpt_Dump,gpi->MaxPointOutput) ;
	   v_Msg(ctx,UCTBUF2,"@*Binding:(New=%1d, Key=%2s) - %3P - ",gotnew,v4is_FormatKey((struct V4IS__Key *)&nbls),isctpt) ;
	   UCstrcat(UCTBUF2,UCTBUF1) ;
	   vout_UCText(VOUT_Trace,0,UCTBUF2) ; vout_NL(VOUT_Trace) ;
	 } ;
/*	If the point used to save this nbl is on current context then update with new nbl */
	DIMVAL(ipt,ctx,nbls.blsk.AuxVal) ;
	if (ipt != NULL)
	 { if (ipt->Value.IntVal == nbls.blsk.KeyVal.IntVal)
/*	   NOT GOOD WAY, INCORRECT FOR NON-INT BINDINGS! (but probably close enough) */
	    { if (!v4ctx_FrameAddDim(ctx,V4C_FrameId_NoOpt,ipt,ipt->Dim,0)) goto ret_fail ; } ;
	 } ;
/*	Set up BindPoint Value in current context for possible later reference */
	ctx->bpv.Dim = nbls.blsk.AuxVal ; ctx->bpv.BindId = nbl->LUBindId ;
	ctx->bpv.AreaId = aid ; memcpy(&ctx->bpv.bpKey,&nbls.blsk,nbls.blsk.Bytes) ;
/*	Update Bind Info Record for this Area */
	if (mmm->Areas[areax].abi == NULL)
	 { abi = (struct V4DPI__AreaBindInfo *)v4mm_AllocChunk(sizeof *abi,FALSE) ;	/* First time ? */
	   mmm->Areas[areax].abi = abi ;
	   abi->kp.fld.KeyType = V4IS_KeyType_V4 ; abi->kp.fld.KeyMode = V4IS_KeyMode_Int ; abi->kp.fld.Bytes = V4IS_IntKey_Bytes ;
	   abi->kp.fld.AuxVal = V4IS_SubType_BindInfo ; abi->key = 0 ;
#ifdef TRACEGRAB
printf("blmake4 %d\n",mmm->Areas[areax].areaLock) ;
#endif
	   GRABAREAMTLOCK(areax) ;
	   if (v4is_PositionKey(aid,(struct V4IS__Key *)&abi->kp,(struct V4IS__Key **)&abiptr,0,V4IS_PosDCKL) == V4RES_PosKey)	/* See if anything exists */
	    { memcpy(abi,abiptr,abiptr->Bytes) ;		/* Got something - copy it in */
	    } else { abi->DimCnt = 0 ; abi->Bytes = 0 ; } ;
	   FREEAREAMTLOCK(areax) ;
	 } else { abi = mmm->Areas[areax].abi ; } ;
	for (i=0;i<abi->DimCnt;i++)				/* Look for this dimension in list */
	 { if (dimid == abi->DimsWithBind[i]) break ; } ;
	if (i >= abi->DimCnt)					/* Not found - have to add & update */
	 { abi->DimsWithBind[abi->DimCnt++] = dimid ;
	   abi->Bytes = ALIGN((char *)&abi->DimsWithBind[abi->DimCnt] - (char *)abi) ;
#ifdef TRACEGRAB
printf("blmake5 %d\n",mmm->Areas[areax].areaLock) ;
#endif
	   GRABAREAMTLOCK(areax) ;
	   if (abi->DimCnt == 1)				/* Are we updating existing or making new one? */
	    { v4is_Insert(aid,(struct V4IS__Key *)abi,abi,abi->Bytes,V4IS_PCB_DataMode_Index,V4IS_DataCmp_None,0,FALSE,FALSE,0,NULL) ;
	    } else { if (v4is_PositionKey(aid,(struct V4IS__Key *)abi,NULL,0,V4IS_PosDCKL) != V4RES_PosKey)
		      { v_Msg(ctx,ctx->ErrorMsgAux,"@Could not re-locate AreaBindInfo record") ; goto ret_fail ; } ;
		     v4is_Replace(aid,(struct V4IS__Key *)abi,abi,abi,abi->Bytes,V4IS_PCB_DataMode_Index,V4IS_DataCmp_None,0,0) ;
		   } ;
	   FREEAREAMTLOCK(areax) ;
	 } ;
	v4dpi_GetBindBuf(ctx,UNUSED,NULL,FALSE,NULL) ;	/* Flush the cache */
	v4ctx_HasHaveNBL(ctx,dimid) ;		/* Maybe re-synch context point */
//VEH160422 - Not sure what this does other than blow up with the '-1'. Think it is left over from days long gone
//	if ( ((gpi->BindingCount) % 1000) == 0)
//	 v4ctx_AreaCheckPoint(ctx,-1) ;
	v4ctx_FrameCacheClear(ctx,FALSE) ;			/* Clear out the Ctx nbl cache */
	v4dpi_IsctEvalCacheClear(ctx,FALSE) ;			/* Clear out the IsctEval cache */
	if (!addnew && (flags & V4DPI_BindLM_WarnDupBind))	/* Give warning if replaced a binding */
	 { v4dpi_PointToString(UCTBUF2,valpt,ctx,V4DPI_FormatOpt_Trace) ;
	   v_Msg(ctx,UCTBUF1,"*BindExists",isctarg,UCTBUF2) ; vout_UCText(VOUT_Warn,0,UCTBUF1) ;
	 } ;

ret_ok:
	return(TRUE) ;

ret_fail:
	return(FALSE) ;
}

/*	v4dpi_BindListRemove - Removes "bind point" from a binding list	*/
/*	Call: logical = v4dpi_BindListRemove( ctx , bpv )
	  where logical is TRUE if point has been removed, FALSE if not found,
		ctx is current context,
		bpv is pointer to V4DPI__BindPointVal referencing the point	*/

LOGICAL v4dpi_BindListRemove(ctx,bpv)
  struct V4C__Context *ctx ;
  struct V4DPI__BindPointVal *bpv ;
{
#ifdef IMPLEMENT_LATER
  struct V4IS__AreaCB *acb ;
  struct V4DPI__Point *ipt ;
  struct V4DPI__Point *isctpt,*ipnt,*cpnt ;
  struct V4DPI__Binding *bindp ;
  struct V4DPI__BindList *nbl,nblbuf,*dataptr ;
  struct V4DPI__BindList_Value *nblvptr ;
  struct V4DPI__BindList_Dim nbld,*nbldptr ;
  struct lcl__key				/* NOTE: Must match begin of V4DPI__Binding *** */
   { union V4IS__KeyPrefix kp ;
     int ValueId ;
   } bind ;
  char dvpbuf[V4DPI_BindList_BufMax] ;
  short dimuse[V4DPI_BindList_DimMax] ;
  int i,j,fx,bytes,dvpbytes,dataid ; char *endptr ;

	nbl = &nblbuf ; acb = v4mm_ACBPtr(bpv->AreaId) ;
/*	Try and pull any existing info for this dimension point */
	Check for DCLx !!!
	if (v4is_PositionKey(bpv->AreaId,&bpv->bpKey,&dataptr,0,V4IS_PosDCKL) != V4RES_PosKey) return(FALSE) ;	/* No record ? */
	dataid = acb->DataId ;
	memcpy(nbl,dataptr,(char *)&dataptr->Buffer[dataptr->DimStart]-(char *)dataptr) ;/* Header & values to stand-alone */
	nbldptr = (struct V4DPI__BindList_Dim *)&dataptr->Buffer[dataptr->DimStart] ;	/* Pointer to Dimension list */
	memcpy(&nbld,nbldptr,dataptr->PointStart-dataptr->DimStart) ;/* Copy into stand-alone buffer */
	dvpbytes = (char *)dataptr + dataptr->Bytes - (char *)&dataptr->Buffer[dataptr->PointStart] ;
	memcpy(&dvpbuf,&dataptr->Buffer[dataptr->PointStart],dvpbytes) ; /* Copy dim=val points */
	nblvptr = (BLV *)&nbl->Buffer[0] ;	/* Get to first value */
	endptr = (char *)&nbl->Buffer[dataptr->DimStart] ;		/* Pointer to end of values */
	for(i=0;i<V4DPI_BindList_DimMax;i++) dimuse[i] = 0 ;		/* Reset list- no reference */
/*	Loop thru all points in binding's intersection & add/match with existing */
	for(i=0;i<nbl->ValueCnt;i++)
	 { if (nblvptr->BindId == bpv->BindId)				/* Is this the one? */
	    { if (nblvptr->sp.Bytes == 0)				/* YES - do we have to delete value record ? */
	       { bind.kp.fld.AuxVal = V4IS_SubType_Value ; bind.kp.fld.KeyType = V4IS_KeyType_V4 ;
		 bind.kp.fld.KeyMode = V4IS_KeyMode_Int ; bind.kp.fld.Bytes = sizeof bind ;
		 bind.ValueId = nblvptr->sp.Value.IntVal ;
		 if (v4is_PositionKey(bpv->AreaId,&bind.kp,&bindp,0,V4IS_PosDCKL) == V4RES_PosKey)
		  { bytes = bindp->Bytes ; bindp->Bytes = 0 ;		/* Mark as deleted */
		    v4is_Replace(bpv->AreaId,bindp,bindp,bindp,bytes,V4IS_PCB_DataMode_Index,V4IS_DataCmp_None,0,0) ;
		  } ;
	       } ;
	      memcpy(nblvptr,(char *)nblvptr+nblvptr->Bytes,1+endptr-((char *)nblvptr+nblvptr->Bytes)) ;/* Get rid of the point */
	      nbl->ValueCnt-- ; i-- ;
	      continue ;
	    } ;
	   for(j=0;j<nblvptr->IndexCnt;j++)				/* Not the point - track Dim's used */
	    { dimuse[nblvptr->DimPos[j]]++ ; } ;
	   nblvptr = (BLV *)((char *)nblvptr + nblvptr->Bytes) ;
	 } ;
/*	Now trash any dimensions no longer in use */
	for(i=0;i<nbld.DimCnt;i++)
	 { if (dimuse[i] > 0) continue ;
	   for(j=i;j<nbld.DimCnt;j++) nbld.DimEntryIndex[j] = nbld.DimEntryIndex[j+1] ;
	   nbld.DimCnt-- ; i-- ;
	 } ;
/*	All done - put everything back together into one nbl structure */
	fx = 0 ;
	for(i=0;i<nbl->ValueCnt;i++)
	 { nblvptr = (BLV *)&nbl->Buffer[fx] ;	/* Pointer to next nblv */
	   fx += nblvptr->Bytes ;					/* Advance to next one */
	 } ;
	nbl->DimStart = ALIGN(fx) ;
	bytes = (char *)&nbld.DimEntryIndex[nbld.DimCnt] - (char *)&nbld ;
	memcpy(&nbl->Buffer[fx],&nbld,bytes) ;				/* Copy Dimension list */
	fx += ALIGN(bytes) ;
	nbl->PointStart = fx ;
	for(i=0;i<nbld.DimCnt;i++)
	 { ipt = (P *)&dvpbuf[nbld.DimEntryIndex[i]] ;
	   memcpy(&nbl->Buffer[fx],ipt,ipt->Bytes) ; fx += ipt->Bytes ;
	 } ;
	nbl->Bytes = (char *)&nbl->Buffer[fx] - (char *)nbl ;
	if (nbl->Bytes >= acb->RootInfo->BktSize-100)
	 { v4dpi_ExamineBindList(nbl,ctx,bpv->AreaId) ;
	   v4_error(V4E_BINDLISTMAXBKT,0,"V4DPI","BindListMake","BINDLISTMAXBKT","BindList exceeds bucket size (%d) for area",acb->RootInfo->BktSize) ;
	 } ;
/*	Now write out to current area */
	v4is_Replace(bpv->AreaId,nbl,nbl,nbl,nbl->Bytes,V4IS_PCB_DataMode_Index,V4IS_DataCmp_None,dataid,0) ;
#endif
	return(FALSE) ;
}

/*	v4dpi_ExamineBindList - Formats contents of bind list	*/
/*	Call: v4dpi_ExamineBindList( nbl , ctx , aid )
	  where nbl is pointer to V4DPI__BindList,
		ctx is current context,
		aid is area id from whence nbl came		*/

void v4dpi_ExamineBindList(nbl,ctx,aid)
  struct V4DPI__BindList *nbl ;
  struct V4C__Context *ctx ;
  int aid ;
{
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4DPI__Binding bind,*binding ;
  struct V4DPI__Point *ipt,xpnt ;
  struct V4DPI__BindList_Value *nblv ;
  struct V4DPI__BindList_Dim *nbld ;
  int i,j ; UCCHAR tb1[5000],tb2[5000],vtb[5000] ;

	nblv = (BLV *)&nbl->Buffer[nbl->ValueStart] ;
	nbld = (struct V4DPI__BindList_Dim *)&nbl->Buffer[nbl->DimStart] ;
	for(i=0;i<nbl->ValueCnt;i++)
	 { ZUS(tb1) ;
	   for(j=0;j<nblv->IndexCnt;j++)
	    { ipt = (P *)&nbl->Buffer[nbl->PointStart+nbld->DimEntryIndex[nblv->DimPos[j]]] ;
//	      v4dpi_PointToString(tb2,ipt,ctx,-1) ;
	      v_Msg(ctx,tb2,"@%1P",ipt) ;
	      UCstrcat(tb1,tb2) ; if (j < nblv->IndexCnt-1) UCstrcat(tb1,UClit(" ")) ;
	    } ;
	   if (nblv->sp.Bytes == 0)					/* Is value in the short point or in value record */
	    { bind.kp.fld.AuxVal = V4IS_SubType_Value ; bind.kp.fld.KeyType = V4IS_KeyType_V4 ;
	      bind.kp.fld.KeyMode = V4IS_KeyMode_Int ; bind.kp.fld.Bytes = V4IS_IntKey_Bytes ; bind.ValueId = nblv->sp.Value.IntVal ;
	      if (v4is_PositionKey(aid,(struct V4IS__Key *)&bind,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey)
	       v4_error(V4E_NOVALREC,0,"V4DPI","ExamineBindList","NOVALREC","Could not access value record") ;
	      binding = (struct V4DPI__Binding *)v4dpi_GetBindBuf(ctx,aid,ikde,FALSE,NULL) ;
	      UCsprintf(vtb,UCsizeof(vtb),UClit("#%d"),nblv->sp.Value.IntVal) ;
//	      v4dpi_PointToString(tb2,&binding->Buffer,ctx,-1) ;
	      v_Msg(ctx,tb2,"@%1P",&binding->Buffer) ;
	    } else								/* Value is in short point - link up */
	    { xpnt.Dim = nblv->sp.Dim ; xpnt.Bytes = nblv->sp.Bytes ; xpnt.PntType = nblv->sp.PntType ;
	      xpnt.Grouping = V4DPI_Grouping_Single ; xpnt.LHSCnt = nblv->sp.LHSCnt ; xpnt.NestedIsct = FALSE ;
	      memcpy(&xpnt.Value,&nblv->sp.Value,nblv->sp.Bytes-V4DPI_PointHdr_Bytes) ;
	      UCstrcpy(vtb,UClit("(local)")) ;
//	      v4dpi_PointToString(tb2,&xpnt,ctx,-1) ;
	      v_Msg(ctx,tb2,"@%1P",&xpnt) ;
	    } ;
	   v_Msg(ctx,UCTBUF2,"@*Bind [%1U] = %2U  (Pos=%3d, Value=%4U, Wgt=%5d, Bytes=%6d)\n",tb1,tb2,i+1,vtb,nblv->BindWgt,nblv->Bytes) ;
	   vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	   nblv = (BLV *)((char *)nblv + nblv->Bytes) ;	/* Advance to next one */
	 } ;
}

/*	v4dpi_GetBindBuf - Returns pointer to BindingBuf if in index or data bucket	*/
/*	NOTE: The area must be locked down before calling this routine in multi-thread environment (i.e. - this routine does not lock) */
/*	Call: ptr = v4dpi_GetBindBuf( ctx , aid , ikde , cache , updbytes )
	  where ptr is pointer to data,
		ctx is current context,
		aid is area id (UNUSED to flush cache, -2 to free cache memory),
		ikde is pointer to key entry,
		cache is TRUE to attempt cache lookup/save,
		updbytes if not NULL is updated with bytes			*/

char *v4dpi_GetBindBuf(ctx,aid,ikde,cache,updbytes)
  struct V4C__Context *ctx ;
  int aid ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  LOGICAL cache ;
  int *updbytes ;
{
  union V4MM__DataIdMask dim ;
  struct V4IS__Bkt *dbkt ;
  struct V4IS__DataBktHdr *dbh ;
  struct V4IS__DataBktEntryHdr *dbeh ;
  char *b ; int len,index,hx ;

	if (aid < 0)
	 { if (aid == UNUSED)
	    { if (ctx->gbbc != NULL) { for(hx=0;hx<GBBMax;hx++) { ctx->gbbc->Entry[hx].DataId = UNUSED ; } ; } ;
	    } else if (aid == -2) { if (ctx->gbbc != NULL) v4mm_FreeChunk(ctx->gbbc) ; } ;
	   return(NULL) ;
	 } ;
	if (ikde->EntryType == V4IS_EntryType_DataLen)		/* Data resides in index */
	 { if (updbytes != NULL) *updbytes = ikde->AuxVal-sizeof *ikde ;
	   return( (char *)ikde + sizeof(*ikde) ) ;
	 } ;
	if (cache)					/* Attemp to look up in cache? */
	 { if (ctx->gbbc == NULL) ctx->gbbc = (struct V4C__GBBCache *)v4mm_AllocChunk(sizeof *ctx->gbbc,TRUE) ;
	   hx = ikde->DataId * (aid + 1) ; hx = hx % GBBMax ; if (hx < 0) hx = -hx ;
	   if (ctx->gbbc->Entry[hx].DataId == ikde->DataId && ctx->gbbc->Entry[hx].aid == aid)
	    { if (updbytes != NULL) *updbytes = ctx->gbbc->Entry[hx].Bytes ;
	      return(ctx->gbbc->Entry[hx].buf) ;
	    } ;
	 } ;
	dim.dataid = ikde->DataId ;				/* Data in databucket - go get it */
	dbkt = v4mm_BktPtr(dim.fld.BktNum,aid,V4MM_LockIdType_Data) ;
	dbh = (struct V4IS__DataBktHdr *)dbkt ;			 /* Link to data bucket */
/*	Before we get too carried away, let's make sure we actually have a data bucket */
	switch (dbh->sbh.BktType)
	 { default: v4_error(V4E_INVBKTTYPE,0,"V4IS","Get","INVBKTTYPE","Unknown bucket type-1 (%d)",dbh->sbh.BktType) ;
	   case V4_BktHdrType_Data:
/*		Scan thru data bucket for wanted data entry */
		for(index=dbh->FirstEntryIndex;index<dbh->FreeEntryIndex;index+=ALIGN(dbeh->Bytes))
		 { dbeh = (struct V4IS__DataBktEntryHdr *)&dbkt->Buffer[index] ;
		   if (dbeh->Bytes == 0)
		    v4_error(V4E_INVDBEHLENIS0,0,"V4IS","Get","INVDBEHLENIS0","Area (%d %s) Bucket (%d) invalid dbeh->Bytes",
				aid,"?",dbh->sbh.BktNum) ;
		   if (dbeh->DataId != dim.dataid) continue ;
		   switch (dbeh->CmpMode)
		    { default:
			v4_error(0,0,"V4IS","Get","INVCMPMODE","Invalid compression mode") ;
		      case V4IS_DataCmp_None:
			len = dbeh->Bytes - sizeof(*dbeh) ; b = (char *)dbeh + sizeof(*dbeh) ;
			break ;
		      case V4IS_DataCmp_Mth1:
			if (ctx->cmpBuf == NULL) ctx->cmpBuf = (char *)v4mm_AllocChunk(V4IS_BktSize_Max,FALSE) ;
			len = data_expand(ctx->cmpBuf,(char *)dbeh + sizeof(*dbeh),dbeh->Bytes-sizeof *dbeh) ;
			b = ctx->cmpBuf ;
			break ;
		      case V4IS_DataCmp_Mth2:
			if (ctx->cmpBuf == NULL) ctx->cmpBuf = (char *)v4mm_AllocChunk(V4IS_BktSize_Max*2,FALSE) ;
//printf("dbeh->Bytes=%d, cmpBuf size=%d\n",dbeh->Bytes, V4IS_BktSize_Max * 2) ;
			lzrw1_decompress((char *)dbeh + sizeof(*dbeh),dbeh->Bytes-sizeof *dbeh,ctx->cmpBuf,&len,V4IS_BktSize_Max*2) ;
//printf("dbeh->Bytes=%d, cmpBuf size=%d, expanded len=%d\n",dbeh->Bytes, V4IS_BktSize_Max * 2,len) ;
			b = ctx->cmpBuf ;
			break ;
		    } ;
		   if (cache && len < GBB_BufMax)
		    { hx = ikde->DataId * (aid + 1) ; hx = hx % GBBMax ; if (hx < 0) hx = -hx ;
		      ctx->gbbc->Entry[hx].DataId = ikde->DataId ; ctx->gbbc->Entry[hx].aid = aid ;
		      memcpy(&ctx->gbbc->Entry[hx].buf,b,len) ; ctx->gbbc->Entry[hx].Bytes = len ;
		    } ;
		   if (updbytes != NULL) *updbytes = len ;
		   return(b) ;
		 } ;
		return(NULL) ;		/* Could not find ? */
	 } ;
}
/*	A G G R E G A T E S			*/

/*	AggHdr = AggShell + AggShell->Key.Bytes ;
	AggData = AggHdr + sizeof(Bytes) + sizeof(Count) + AggHdr->Count*sizeof(Indexes[0])
	if Count < 0 then have AggVHdr
	  key is V4IS_KeyType_V4, V4IS_KeyMode_Int, V4IS_SubType_AggVHdr, Bytes=12, Value.IntVal = dimension
*/

#define GrabALATable \
  { int cnt ; for(cnt=0;cnt<500;cnt++) { if (GETSPINLOCK(&ala->Interlock)) break ; HANGLOOSE(1) ; continue ; } ; if (cnt >= 1000) goto grab_err ; }
#define ReleaseALATable \
  ala->Interlock = UNUSED ;

/*	v4im_ALA_InitALA - Initializes ala structure & starts up producer thread			*/
/*	Call: logical = v4im_ALA_InitALA( ctx , ala , point )
	  where ala points to the initialized (maybe allocated) ala, NULL if error (ctx->ErrorMsgAux with error),
		ctx is current context,
		ala is pointer to look-ahead structure that is initialized or NULL to be allocated within,
		point is V4 point that is list to be enumerated (i.e. aggs to be obtained from this point */
		
struct V4IM__AggLookAhead *v4im_ALA_InitALA(ctx,ala,point)
  struct V4C__Context *ctx ;
  struct V4IM__AggLookAhead *ala ;
  P *point ;
{ void v4im_ALA_ProducerThread() ;
  struct V4L__ListPoint *lp ;
  int i ;

	lp = ALIGNLP(&point->Value) ;
	if (lp->ListType != V4L_ListType_AggRef) { v_Msg(ctx,ctx->ErrorMsgAux,"ListNotAgg",point) ; return(NULL) ; } ;
	if (ala == NULL)
	 { ala = (struct V4IM__AggLookAhead *)v4mm_AllocChunk(sizeof *ala,TRUE) ; ala->Flags |= V4IM_ALHFlags_Deallocate ; }
	 else { memset(ala,0,sizeof *ala) ; } ;
	ala->ctx = ctx ;			/* Save ctx here (limited amount of info can be passed to thread) */
	ala->Index1 = -1 ;			/* Set consumer index to 1 before first */
	ala->Index2 = -1 ;			/*  ditto for producer */
	ala->Interlock = UNUSED ;
	ala->BufCount = V4IM_AggLA_Max ;
	for(i=0;i<ala->BufCount;i++) ala->aggs[i] = v4mm_AllocChunk(sizeof ala->aggs[0],FALSE) ;
	ala->MinForWakeup = (int)(ala->BufCount * 0.1) ;
	ala->lar = (struct V4L__ListAggRef *)&lp->Buffer[0] ;
#ifdef WINNT
	ala->synchEvent1 = CreateEvent(NULL,TRUE,FALSE,NULL) ;
	ala->synchEvent2 = CreateEvent(NULL,TRUE,FALSE,NULL) ;
	ala->hThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)v4im_ALA_ProducerThread,ala,0,NULL) ;
#else
	v_Msg(ctx,ctx->ErrorMsgAux,"NYIonthisOS",0) ; return(NULL) ;
#endif
	ala->Status1 = V4IM_ALHStatus_Working ;
	gpi->ala = ala ;			/* Set this in so we pick it up when we try to get first agg value */
	return(ala) ;
}

/*	v4im_ALA_Cleanup - Frees all resources used by look-ahead			*/
/*	Call: v4im_ALA_Cleanup( ala )
	  where ala is pointer to look-ahead structure (ala itself is not freed)	*/

void v4im_ALA_Cleanup(ala)
  struct V4IM__AggLookAhead *ala ;
{
  int i ;

	for(i=0;i<V4IM_AggLA_Max;i++) { if (ala->aggs[i] != NULL) v4mm_FreeChunk(ala->aggs[i]) ; } ;
#ifdef WINNT
	CloseHandle(ala->synchEvent1) ; CloseHandle(ala->synchEvent2) ; CLOSE_THREAD(ala->hThread) ;
#endif
	if (ala->Flags & V4IM_ALHFlags_Deallocate) v4mm_FreeChunk(ala) ;
}

/*	v4im_ALA_ConsumeNextAggs - Grabs the next available aggs buffer (may have to wait for producer)	*/
/*	All communication between thread & caller via ala->xxx, caller may obtain status of thread via ala->Status2 & ala->ErrMsg */
/*	Call: aggs = v4im_ALA_ConsumeNextAggs( ala )
	  where aggs is next buffer (NULL if error, error in ala->ErrorMsg),
		ala is pointer to look-ahead structure							*/

struct V4IM__AggShell *v4im_ALA_ConsumeNextAggs(ala)
  struct V4IM__AggLookAhead *ala ;
{
  int i,res ;

	for(i=0;;i++)
	 { 
/*	   If look-ahead falls below minimum & producer is sleeping then wake it up */
#ifdef WINNT
	   if (ala->Index2 - ala->Index1 < ala->MinForWakeup)
	    { if (ala->Status2 == V4IM_ALHStatus_Wait) { SetEvent(ala->synchEvent2) ; } ;
	    } ;
#endif
	   if (ala->Index1 < ala->Index2)	/* Producer still ahead of consumer - grab next available one */
	    { GrabALATable ; ++ala->Index1 ; ReleaseALATable ;
//printf("consume %d, mod=%d\n",ala->Index1,ala->Index1 % ala->BufCount) ;
	      return(ala->aggs[ala->Index1 % ala->BufCount]) ;
	    } ;
/*	   Nothing to grab - see if we are done */
	   if (ala->Status2 == V4IM_ALHStatus_Done) { v_Msg(ala->ctx,ala->ErrorMsg,"ALANoMore",ala->lar->Dim) ; return(NULL) ; } ;
	   if (ala->Status2 == V4IM_ALHStatus_Err) return(NULL) ;
	   GrabALATable
	   ala->Status1 = V4IM_ALHStatus_Wait ;
#ifdef WINNT
	   if (ala->Status2 == V4IM_ALHStatus_Wait) SetEvent(ala->synchEvent2) ;
	   ReleaseALATable ;
//printf("Consumer sleeping\n") ;
	   res = WaitForSingleObject(ala->synchEvent1,5000) ;	/* Wait max of 5 seconds */
	   ResetEvent(ala->synchEvent1) ; ala->Wakeups1 ++ ;
	   if (res == WAIT_TIMEOUT) { v_Msg(ala->ctx,ala->ErrorMsg,"ALAWaitTimeout2",ala->lar->Dim) ; return(NULL) ; } ;
	   ala->Status1 = V4IM_ALHStatus_Working ;
//printf("Consumer awake\n") ;
#endif
	   } ;

grab_err:
	ala->Status1 = V4IM_ALHStatus_Err ;
	v_Msg(ala->ctx,ala->ErrorMsg,"ALAGrabLockFail",ala->lar->Dim) ;
	return(NULL) ;
}

/*	v4im_ALA_ProducerThread - Routine that is called as independent thread to prefetch agg buffers	*/
/*	All communication between thread & caller via ala->xxx, caller may obtain status of thread via ala->Status2 & ala->ErrMsg */
/*	Call: v4im_ALA_ProducerThread( ala )
	  where ala is pointer to look-ahead structure							*/

void v4im_ALA_ProducerThread(ala)
  struct V4IM__AggLookAhead *ala ;
{
  int res ;

	ala->Status2 = V4IM_ALHStatus_Working ;
	for(ala->larIndex=0;ala->larIndex<ala->lar->Count;ala->larIndex++)
	 { ala->biCurVal = 0 ;
	   for(;;)
	    { 
/*	      Figure out the next buffer to fill & whether or not we can do it */
	      GrabALATable
	      if (ala->Index2 - ala->Index1 >= ala->BufCount - 1) { ReleaseALATable ; goto wait ; } ;
//	      if (ala->Index1 == -1 ? FALSE : (ala->Index2+1) % ala->BufCount != ala->Index1 % ala->BufCount)
//	       { ReleaseALATable ; goto wait ; } ;
	      ReleaseALATable

//printf("Buf grab index=%d, status1=%d\n",i,ala->Status1) ;
	      if (!v4im_ALA_GrabNextBuffer(ala,ala->Index2+1)) break ;
	      GrabALATable
	      ala->Index2++ ;
#ifdef WINNT
	      if (ala->Status1 == V4IM_ALHStatus_Wait) { /* printf("Wakeup consumer\n") ; */ SetEvent(ala->synchEvent1) ; } ;
#endif
	      ReleaseALATable
	      continue ;

wait:	      GrabALATable
	      ala->Status2 = V4IM_ALHStatus_Wait ;
#ifdef WINNT
	      if (ala->Status1 == V4IM_ALHStatus_Wait) { /* printf("Wakeup consumer2\n") ; */ SetEvent(ala->synchEvent1) ; } ;
	      ReleaseALATable
//printf("Producer sleeping\n") ;
	      res = WaitForSingleObject(ala->synchEvent2,60000) ;	/* Wait max of 60 seconds */
	      if (res == WAIT_TIMEOUT)
	       { ala->Status2 = V4IM_ALHStatus_Err ; v_Msg(ala->ctx,ala->ErrorMsg,"ALAWaitTimeout",ala->lar->Dim) ; return ; } ;
	      ala->Status2 = V4IM_ALHStatus_Working ; ResetEvent(ala->synchEvent2) ; ala->Wakeups2 ++ ;
#endif
//printf("Producer awake\n") ;
	      continue ;

	    } ;
	 } ;
/*	If here then show is over */
	GrabALATable
	ala->Status2 = V4IM_ALHStatus_Done ;
#ifdef WINNT
	if (ala->Status1 == V4IM_ALHStatus_Wait) { /* printf("Wakeup consumer3\n") ; */ SetEvent(ala->synchEvent1) ; } ;
#endif
	ReleaseALATable
//printf("Consumer wakeups=%d, Producer Wakeups=%d\n",ala->Wakeups1,ala->Wakeups2) ;
	return ;
grab_err:
	ala->Status2 = V4IM_ALHStatus_Err ;
	v_Msg(ala->ctx,ala->ErrorMsg,"ALAGrabLockFail",ala->lar->Dim) ;
	return ;
}

/*	v4im_ALA_GrabNextBuffer - Reads next agg buffer into aggs structure	*/
/*	Call: v4im_ALA_GrabNextBuffer( ala , aggs )
	  where ala is pointer to look-ahead structure,
	  aggs is pointer to buffer to obtain info				*/

LOGICAL v4im_ALA_GrabNextBuffer(ala,grabcnt)
  struct V4IM__AggLookAhead *ala ;
  int grabcnt ;
{
  struct V4IS__ParControlBlk *pcb ;
  struct V4IM__AggShell *aggs ;
  int i ;

	i = ala->lar->AreaAggIndex[ala->larIndex] ;		/* i = index of current area within ctx */
	pcb = gpi->AreaAgg[i].pcb ;				/* pcb = aggregate pcb */

	i = grabcnt % ala->BufCount ;
	aggs = ala->aggs[i] ;

	aggs->Key.KeyType = V4IS_KeyType_AggShell ; aggs->Key.AuxVal = ala->lar->Dim ;
	aggs->Key.KeyMode = V4IS_KeyMode_Fixed ; aggs->Key.Bytes = V4IS_FixedKey_Bytes ;
	ala->biCurVal++ ; COPYB64(aggs->Key.KeyVal.B64,ala->biCurVal) ;
//printf("Get key = %d\n",aggs->Key.KeyVal.IntVal) ;

	pcb->GetMode = V4IS_PCB_GP_KeyedNext|V4IS_PCB_NoError ;
	pcb->GetBufPtr = (BYTE *)aggs ; pcb->GetBufLen = sizeof *aggs ;
	pcb->KeyPtr = &aggs->Key ;	   
	v4is_Get(pcb) ;
	COPYB64(ala->biCurVal,aggs->Key.KeyVal.B64) ;
//printf("  produce= %d, got key = %d, mod=%d\n",grabcnt,aggs->Key.KeyVal.IntVal,i) ;
	return(pcb->GetLen != UNUSED) ;
}




/*	v4im_AggLoadArea - Loads aggregate area into context		*/
/*	Call: uid = v4im_AggLoadArea( ctx, pcb , idPnt)
	  where uid is internal aggregate unique Id if all is well, UNUSED if problems,
		ctx is current context,
		pcb is pointer to open pcb for aggregate area,
		idPnt is optional unique id for aggregate		*/

int v4im_AggLoadArea(ctx,pcb,idPnt)
  struct V4C__Context *ctx ;
  struct V4IS__ParControlBlk *pcb ;
  struct V4DPI__LittlePoint *idPnt ;
{
  struct V4AGG__AggDims *vad ;
  int rx ;
#ifdef V4ENABLEMULTITHREADS		/* This module probably won't be used in MT environment... but just in case */
  static DCLSPINLOCK alaLock = UNUSED ;
#endif

	if (!GRABMTLOCK(alaLock))
	 { v_Msg(ctx,ctx->ErrorMsgAux,"MTCantLock",UClit("alaLock in v4im_AggLoadArea)")) ; return(UNUSED) ; } ;
	for(rx=0;rx<gpi->AreaAggCount;rx++)			/* Look for an empty slot */
	 { if (gpi->AreaAgg[rx].pcb == NULL) break ; } ;
	if (rx >= gpi->AreaAggCount)				/* None found, append to end if possible */
	 { if (gpi->AreaAggCount >= V4C_AreaAgg_Max)
	    { FREEMTLOCK(alaLock) ; v_Msg(ctx,ctx->ErrorMsgAux,"@Attempt to open too many (%1d) AGGREGATE areas",V4C_AreaAgg_Max) ; return(UNUSED) ; } ;
	   rx = gpi->AreaAggCount++ ;
	 } ;
	gpi->AreaAgg[rx].pcb = (struct V4IS__ParControlBlk *)v4mm_AllocChunk(sizeof *pcb,FALSE) ;
	memcpy(gpi->AreaAgg[rx].pcb,pcb,sizeof *pcb) ;
	vad = (struct V4AGG__AggDims *)v4mm_AllocChunk(sizeof *(gpi->AreaAgg[0].vad),FALSE) ;
	vad->kp.fld.Bytes = sizeof vad->kp + sizeof vad->KeyVal ;
	vad->kp.fld.KeyMode = V4IS_KeyMode_Int ; vad->kp.fld.KeyType = V4IS_KeyType_V4 ;
	vad->kp.fld.AuxVal = V4IS_SubType_AggDims ; vad->KeyVal = 0 ;	/* Format correct key for this structure */
	gpi->AreaAgg[rx].pcb->GetBufPtr = (BYTE *)vad ;
	gpi->AreaAgg[rx].pcb->GetBufLen = sizeof *vad ;
	gpi->AreaAgg[rx].pcb->GetMode = V4IS_PCB_GP_Keyed | V4IS_PCB_NoError ;
	gpi->AreaAgg[rx].pcb->KeyPtr = (struct V4IS__Key *)vad ;
	gpi->AreaAgg[rx].AggUId = gpi->NextUId++ ;			/* Assign unique Id */
	if (idPnt == NULL ? FALSE : idPnt->Bytes > 0)
	 { memcpy(&gpi->AreaAgg[rx].aggPId,idPnt,idPnt->Bytes) ; }
	 else { ZPH(&gpi->AreaAgg[rx].aggPId) ; } ;
	v4is_Get(gpi->AreaAgg[rx].pcb) ;
	if (gpi->AreaAgg[rx].pcb->GetLen == UNUSED)			/* Did we get record? */
	 { v4mm_FreeChunk(vad) ; vad = NULL ; } ;			/*  apparently not! */
	gpi->AreaAgg[rx].vad = vad ;
	FREEMTLOCK(alaLock) ;
	return(gpi->AreaAgg[rx].AggUId) ;
}

/*	v4im_AggInit - Makes a new aggregate & sets up key	*/
/*	Call: v4im_AggInit( aggs , aggh, aggd, pt )
	  where aggs is pointer to V4IM__AggShel,
		aggh is pointer to V4IM__AggHdr,
		aggd is pointer to V4IM_aggData,
		pt is a point to be used as basis for key		*/

void v4im_AggInit(aggs,aggh,aggd,pt)
  struct V4IM__AggShell *aggs ;
  struct V4IM__AggHdr *aggh ;
  struct V4IM__AggData *aggd ;
  struct V4DPI__Point *pt ;
{ int i,j ;

	if (aggh != NULL) { aggh->Bytes = 0 ; aggh->Count = 0 ; } ;	/* Init the header */
	aggs->Key.KeyType = V4IS_KeyType_AggShell ;
	if (pt == NULL) { aggs->Key.Bytes = V4IS_FixedKey_Bytes ; return ; } ;
	aggs->Key.AuxVal = pt->Dim ;
	switch (pt->PntType)
	 { default:
		aggs->Key.KeyVal.IntVal = pt->Value.IntVal ;
		aggs->Key.KeyMode = V4IS_KeyMode_Int ; aggs->Key.Bytes = V4IS_IntKey_Bytes ;
		break ;
	      case V4DPI_PntType_Fixed:
		COPYB64(aggs->Key.KeyVal.B64,pt->Value.FixVal) ;
		aggs->Key.KeyMode = V4IS_KeyMode_Fixed ; aggs->Key.Bytes = V4IS_FixedKey_Bytes ;
		break ;
	      case V4DPI_PntType_BinObj:
		i = pt->Bytes - V4DPI_PointHdr_Bytes ; if (i == 0 || i > V4IS_KeyBytes_Max-10) i =  V4IS_KeyBytes_Max-10 ;
		memcpy(&aggs->Key.KeyVal.Alpha,&pt->Value.AlphaVal[1],i) ;
		for(j=i;j<i+3;j++) { aggs->Key.KeyVal.Alpha[j] = 0 ; } ;		/* Pad with nulls */
		i = ALIGN(i) ;
		aggs->Key.KeyMode = V4IS_KeyMode_Alpha ; aggs->Key.Bytes = sizeof(union V4IS__KeyPrefix) + i ;
		break ;
	      CASEofChar
		i = pt->Value.AlphaVal[0] ; if (i == 0 || i > V4IS_KeyBytes_Max-10) i =  V4IS_KeyBytes_Max-10 ;
		strncpy(aggs->Key.KeyVal.Alpha,&pt->Value.AlphaVal[1],i) ;
		for(j=i;j<i+3;j++) { aggs->Key.KeyVal.Alpha[j] = 0 ; } ;		/* Pad with nulls */
		i = ALIGN(i) ;
		aggs->Key.KeyMode = V4IS_KeyMode_Alpha ; aggs->Key.Bytes = sizeof(union V4IS__KeyPrefix) + i ;
		break ;
	      case V4DPI_PntType_V4IS:
		memcpy(&aggs->Key.KeyVal,&pt->Value,sizeof(struct V4DPI__PntV4IS)) ;
		aggs->Key.KeyMode = V4IS_KeyMode_Alpha ; aggs->Key.Bytes = sizeof(struct V4DPI__PntV4IS) ;
		break ;
	   case V4DPI_PntType_Calendar:
	   case V4DPI_PntType_Real:
		aggs->Key.KeyVal.Int2Val[0] = pt->Value.Int2Val[0] ; aggs->Key.KeyVal.Int2Val[1] = pt->Value.Int2Val[1] ;
		aggs->Key.KeyMode = V4IS_KeyMode_Int2 ; aggs->Key.Bytes = V4IS_Int2Key_Bytes ;
		break ;
	   case V4DPI_PntType_TeleNum:
/*		NOTE: This is pretty bad, but there is a check at startup (in v_GetProcessInfo()) to verify that TeleNum & Int2 are the same size */
		aggs->Key.KeyVal.Int2Val[0] = pt->Value.Int2Val[0] ; aggs->Key.KeyVal.Int2Val[1] = pt->Value.Int2Val[1] ;
		aggs->Key.KeyMode = V4IS_KeyMode_Int2 ; aggs->Key.Bytes = V4IS_Int2Key_Bytes ;
		{ struct V4DPI__Value_Tele *tele = (struct V4DPI__Value_Tele *)&aggs->Key.KeyVal.Int2Val[0] ;
		  if (tele->IntDialCode < 0) tele->IntDialCode = -tele->IntDialCode ;	/* Normalize IntDialCode if necessary */
		} ;
		break ;
	   case V4DPI_PntType_Int2:
//	   case V4DPI_PntType_XDB:
		aggs->Key.KeyVal.Int2Val[0] = pt->Value.Int2Val[0] ; aggs->Key.KeyVal.Int2Val[1] = pt->Value.Int2Val[1] ;
		aggs->Key.KeyMode = V4IS_KeyMode_Int2 ; aggs->Key.Bytes = V4IS_Int2Key_Bytes ;
		break ;
	 } ;
}

/*	v4im_AggAppendVal - Appends a new value (point) to the aggregate */
/*	Call: res = v4im_AggAppendVal( ctx, aggs, aggh, aggd, pt, ptbuf )
	  where res = TRUE if OK, FALSE if error (error in ctx->ErrorMsgAux),
		ctx is current context,
	  	aggs is pointer to V4IM__AggShel,
		aggh is pointer to V4IM__AggHdr,
		aggd is pointer to V4IM__AggData,
		pt is a point to be appended to the aggregate,
		ptbuf is pointer to point to be used, if necessary, as working storage		*/

LOGICAL v4im_AggAppendVal(ctx,aggs,aggh,aggd,pt,ptbuf)
  struct V4C__Context *ctx ;
  struct V4IM__AggShell *aggs ;
  struct V4IM__AggHdr *aggh ;
  struct V4IM__AggData *aggd ;
  struct V4DPI__Point *pt,*ptbuf ;
{ P *tpt ;
  struct V4DPI__DimInfo *di ;
  int len,i ; char *b ;

	if (pt == NULL)					/* Point undefined? */
	 { aggh->Indexes[aggh->Count] = V4IM_AggIndex_Undef ; aggh->Count++ ;
	   return(FALSE) ;
	 } ;
	switch(pt->PntType)				/* Check out the point */
	 { default:
	 	break ;
	   case V4DPI_PntType_Special:
		if (pt->Grouping == V4DPI_Grouping_Current)
		 { DIMVAL(tpt,ctx,pt->Dim) ;
		 } else if (pt->Grouping == V4DPI_Grouping_PCurrent)
		 { DIMPVAL(tpt,ctx,pt->Dim) ;
		 } else { tpt = NULL ; } ;
		if (tpt == NULL)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"IsctEvalCtx",pt->Dim) ; return(FALSE) ;
//		   v4dpi_PointToString(ASCTBUF1,pt,ctx,-1) ;
//		   v4_error(0,0,"V4EVAL","ParseAgg","NOTINCTX","Point in Aggregate< ... %s ... > not in context",ASCTBUF1) ;
		 } ;
		pt = tpt ; break ;
//	   case V4DPI_PntType_V3PicMod:
	   case V4DPI_PntType_BigText:
//	   case V4DPI_PntType_PIntMod:
	   case V4DPI_PntType_IntMod:
//	   case V4DPI_PntType_Context:
	   case V4DPI_PntType_Isct:
		tpt = v4dpi_IsctEval(ptbuf,pt,ctx,0,NULL,NULL) ;
		if (tpt == NULL)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIsctFail",pt) ; return(FALSE) ;
//		   v4dpi_PointToString(ASCTBUF1,pt,ctx,-1) ;
//		   v4_error(0,0,"V4EVAL","ParseAgg","NOTINCTX","Point in Aggregate< ... %s ... > does not evaluate",ASCTBUF1) ;
		 } ;
		pt = tpt ;
		break ;
	   case V4DPI_PntType_UCChar:
/*		Check dimension's point type, if not UCChar then try to convert back to ASCII */
/*		(we need to do this because many V4 routines internall convert Char to UCChar (see Str()) */
		DIMINFO(di,ctx,pt->Dim) ;
		if (di->PointType != V4DPI_PntType_UCChar)
		 { memcpy(ptbuf,pt,V4DPI_PointHdr_Bytes) ;
		   for(i=0;i<UCCHARSTRLEN(pt);i++)
		    { ptbuf->Value.AlphaVal[1+i] = pt->Value.UCVal[1+i] ;
		      if (pt->Value.UCVal[1+i] > 0xff)
		       { v_Msg(ctx,ctx->ErrorMsgAux,"CmdUCtoASC",pt,i+1,pt->Value.UCVal[1+i]) ; return(FALSE) ; } ;
		    } ;
		   ptbuf->Value.AlphaVal[1+i] = '\0' ;
		   CHARPNTBYTES2(ptbuf,UCCHARSTRLEN(pt)) ; pt = ptbuf ;
		 } ;
		break ;
//	   case V4DPI_PntType_Pragma:
//	   case V4DPI_PntType_Binding:
	   case V4DPI_PntType_FrgnDataEl:
	   case V4DPI_PntType_FrgnStructEl:
	   case V4DPI_PntType_Shell:
//	   case V4DPI_PntType_V3Mod:
//	   case V4DPI_PntType_V3ModRaw:
		v_Msg(ctx,ctx->ErrorMsgAux,"AggValType",pt) ; return(FALSE) ;
//		v4_error(0,0,"V4IM","AggAppendVal","INVPNTTYPE","Invalid point type (%s) for Aggregate",v4im_PTName(pt->PntType)) ;
	 } ;
	aggh->Indexes[aggh->Count] = aggh->Bytes ;	/* Record the position */
	b = (char *)&aggd->Buffer[aggh->Bytes] ;	/* Get pointer to next free spot */
	if (pt->AltDim)					/* Got alternate dimension? */
	 { *b = pt->PntType ;				/*  then record point type in first byte */
	   b++ ; aggh->Bytes++ ;
	 } ;
	len = pt->Bytes - V4DPI_PointHdr_Bytes ;	/* Get length of data in point */
	if (aggh->Count > V4IM_AggValMax)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"AggMaxVals",V4IM_AggValMax) ; return(FALSE) ; } ;
//	   v4_error(0,0,"V4IM","AggAppendVal","TOOMANYVALS","Exceeded max number of values") ;
	if (aggh->Bytes + aggs->Key.Bytes + (aggh->Count+3)*(sizeof(aggh->Indexes[0])) >= V4IM_AggShell_BufMax)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"AggTooBig2",aggh->Bytes + aggs->Key.Bytes + (aggh->Count+3)*(sizeof(aggh->Indexes[0])),V4IM_AggShell_BufMax) ; return(FALSE) ; } ;
//	 v4_error(0,0,"V4IM","AggAppendVal","TOOBIG","Aggregate shell too big") ;
	memcpy(b,&pt->Value,len) ;			/* Copy data portion of the point */
	aggh->Count++ ; aggh->Bytes += len ;
	return(TRUE) ;
}

/*	v4im_AggBuild - Constructs full aggregate in shell from 3 components	*/
/*	Call: bytes = v4im_AggBuild( aggs , aggh, aggd, isVHdr, isBlocked, areax )
	  where bytes = total number of bytes in aggregate,
		aggs, aggh, aggd are shell, header, & data,
		isVHdr is TRUE if constructing a VHdr aggregate,
		isBlocked is TRUE if blocking aggregates,
		areax is aggregate area to use (DFLTAGGAREA for default)	*/

int v4im_AggBuild(ctx,aggs,aggh,aggd,isVHdr,isBlocked,areaxAgg)
  struct V4C__Context *ctx ;
  struct V4IM__AggShell *aggs ;
  struct V4IM__AggHdr *aggh ;
  struct V4IM__AggData *aggd ;
  LOGICAL isVHdr,isBlocked ;
  INDEX areaxAgg ;
{ struct V4IM__AggHdr *aggh1 ;
  struct V4IM__AggData *aggd1 ;
  struct V4IM__AggVData *aggvd ;
  struct V4DPI__DimInfo *di ;
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4C__ProcessInfo *pi ;
  struct V4IM__LocalVHdr *lvh ;
  struct V4IM__AggVHdr *avh,avh1 ;
  struct V4IM__AggBDataI *abdi ;				/* Don't really need, just size */
  int rx,hl,i,aid,relhnum,dim,aggusablesize ; INDEX areax ;

	dim = aggs->Key.AuxVal ;				/* Get the dim associated with this aggregate */
	if (isVHdr)
	 { pi = gpi ;
	   if (pi->lvh == NULL) pi->lvh = (struct V4IM__LocalVHdr *)v4mm_AllocChunk(sizeof *lvh,TRUE) ;
	   lvh = pi->lvh ;
	   aggvd = (struct V4IM__AggVData *)((char *)aggs + aggs->Key.Bytes) ;
	   aggvd->Count = (isBlocked ? V4IM_AggHdrType_BValue : V4IM_AggHdrType_Value ) ;
	   memcpy(aggvd->Buffer,aggd,aggh->Bytes) ;		/* Copy data right into buffer */
	   aggvd->Bytes = (char *)&aggvd->Buffer[aggh->Bytes] - (char *)aggs ;
	   avh = v4im_AggGetVHdr(ctx,dim) ;			/* See if we already have avh for this dimension */
	   if (avh == NULL)					/* No- then make it */
	    { memset(&avh1,0,sizeof avh1) ;
	      avh1.kp.fld.KeyMode = V4IS_KeyMode_Int ; avh1.kp.fld.KeyType = V4IS_KeyType_V4 ;
	      avh1.kp.fld.Bytes = sizeof avh1.kp + sizeof(int) ; avh1.kp.fld.AuxVal = V4IS_SubType_VHdr ;
	      avh1.Dim = dim ;
	      avh1.Type = (isBlocked ? V4IM_AggHdrType_BValue : V4IM_AggHdrType_Value ) ;
	      avh1.Count = aggh->Count ;
	      for(i=0;i<avh1.Count;i++) { avh1.Indexes[i] = aggh->Indexes[i] ; } ;
	      avh1.Bytes = (char *)&avh1.Indexes[avh1.Count] - (char *)&avh1 ;
	      avh1.BytesPerEntry = aggvd->Bytes - aggs->Key.Bytes - sizeof aggvd->Bytes - sizeof aggvd->Count + sizeof abdi->biKey ;


	      for(rx=areaxAgg;rx<gpi->AreaAggCount;rx++)		/* Look for update-able aggregate area */
	       { if (gpi->AreaAgg[rx].pcb == NULL) continue ;
	         if ((gpi->AreaAgg[rx].pcb->AccessMode & V4IS_PCB_AM_Insert) != 0) break ;	/* Aggregate not open for update? */
	       } ;
	      if (rx >= gpi->AreaAggCount)			/* No aggregates open for update? */
	       { aggusablesize = V4TempArea_UsableBytes ;	/*  then use bucket size for work area */
	       } else { aggusablesize = V4AreaAgg_UsableBytes ; } ; 


	      avh1.EntriesPerBuffer = (aggusablesize - aggs->Key.Bytes - sizeof aggvd->Bytes - sizeof aggvd->Count) / avh1.BytesPerEntry ;
	      DIMINFO(di,ctx,dim) ;				/* If here then look in V4 area(s) */
	      if (di == NULL)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"AggNoDim1") ; return(UNUSED) ;
//	         v4_error(0,0,"V4DPI","BFAgg","NODIM","Could not find dimension info (maybe BFAgg should be Agg?)") ;
	       } ;
	      if (di->PCAggExtend != 0)
	       { int save ; save = avh1.EntriesPerBuffer ;
	         avh1.EntriesPerBuffer = (int)(avh1.EntriesPerBuffer * (1.0 + ((double)di->PCAggExtend / 100))) ;
	         v_Msg(ctx,UCTBUF1,"@*Increased Entries on Dim:%1D from %2d to %3d\n",di->DimId,save,avh1.EntriesPerBuffer) ;
		 vout_UCText(VOUT_Status,0,UCTBUF1) ;
	       } ;
	      if (gpi->RelH[di->RelHNum].ahi.BindingsUpd)
	       { relhnum = di->RelHNum ; }			/* Default Hnum for Dim allows updates- OK */
	       else { for(relhnum=gpi->HighHNum;relhnum>=gpi->LowHNum;relhnum--)	/* Find highest area allowing new entries */
	 	       { if (gpi->RelH[relhnum].aid == UNUSED) continue ; if (relhnum == V4DPI_WorkRelHNum) continue ;
		         if (gpi->RelH[relhnum].ahi.BindingsUpd) break ;
		       } ;
		      if (relhnum < gpi->LowHNum && gpi->RelH[V4DPI_WorkRelHNum].aid != UNUSED)
		      relhnum = V4DPI_WorkRelHNum ;		/* Assign to work area as last resort */
		      if (relhnum < gpi->LowHNum)
		       { v_Msg(ctx,ctx->ErrorMsgAux,"CtxNoUpdArea1") ; return(UNUSED) ;
//		         v4_error(V4E_AREANOBIND,0,"V4DPI","BindListMake","AREANOBIND","Could not find any Area available for binding updates") ;
		       } ;
	            } ;
	      aid = gpi->RelH[relhnum].aid ;
	      FINDAREAINPROCESS(areax,aid) ;
	      GRABAREAMTLOCK(areax) ;
	      v4is_Insert(aid,(struct V4IS__Key *)&avh1,&avh1,avh1.Bytes,V4IS_PCB_DataMode_Data,V4IS_DataCmp_None,0,FALSE,FALSE,0,NULL) ;
	      FREEAREAMTLOCK(areax) ;
	      for(rx=areax;rx<gpi->AreaAggCount;rx++)		/* Write avh into aggregate as backup */
	       { if (gpi->AreaAgg[rx].pcb == NULL) continue ;
	         if ((gpi->AreaAgg[rx].pcb->AccessMode & V4IS_PCB_AM_Insert) == 0) continue ;	/* Aggregate not open for update? */
	         gpi->AreaAgg[rx].pcb->PutBufPtr = (BYTE *)&avh1 ; gpi->AreaAgg[rx].pcb->PutBufLen = avh1.Bytes ;
	         gpi->AreaAgg[rx].pcb->KeyPtr = (struct V4IS__Key *)&avh1 ;
	         FINDAREAINPROCESS(areax,gpi->AreaAgg[rx].pcb->AreaId) ;
	         GRABAREAMTLOCK(areax) ;
	         if (!v4is_Put(gpi->AreaAgg[rx].pcb,ctx->ErrorMsgAux)) { FREEAREAMTLOCK(areax) ; return(UNUSED) ; } ;
	         FREEAREAMTLOCK(areax) ;
		 break ;
	       } ;
	      avh = (struct V4IM__AggVHdr *)v4mm_AllocChunk(avh1.Bytes,FALSE) ;
	      memcpy(avh,&avh1,avh1.Bytes) ;
	      if (lvh->Count >= V4IM_LocalVHdrMax)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"AggVHdrMax",V4IM_LocalVHdrMax) ; return(UNUSED) ;
//	         v4_error(0,0,"V4IM","AggBuild","MAXLVH","Exceeded max in LocalVHdr structure") ;
	       } ;
	      lvh->Entry[lvh->Count].Dim = dim ; lvh->Entry[lvh->Count].avh = avh ;
	      lvh->Entry[lvh->Count].STREAM.aggs = NULL ; lvh->Entry[lvh->Count].biKeyLastPut = LONG_MIN ;
	      lvh->Entry[lvh->Count].STREAM.ala = NULL ; 
	      lvh->Count++ ;
	    } ;
	   return(aggvd->Bytes) ;
	 } ;
	aggh1 = (struct V4IM__AggHdr *)((char *)aggs + aggs->Key.Bytes) ;
	hl = ALIGN(((char *)&aggh->Indexes[0] - (char *)aggh) + aggh->Count*sizeof(aggh->Indexes[0]) ) ;/* Bytes in aggh */
	aggd1 = (struct V4IM__AggData *)((char *)aggh1 + hl) ;
	memcpy(aggh1,aggh,hl) ;
	memcpy(aggd1,aggd,aggh->Bytes) ;				/* aggh->Bytes = number of bytes in data (initially) */
	aggh1->Bytes =							/* aggh1->Bytes = total for agg */
	  (char *)&aggd1->Buffer[aggh->Bytes] - (char *)aggs ;
	return(aggh1->Bytes) ;
}


///*	v4eval_MakeP_AggVal - Called from MakeP() in v4im() to make Agg from arguments	*/
//
//struct V4DPI__Point *v4eval_MakeP_AggVal(ctx,respnt,argpnts,argcnt,intmodx,trace,di)
//  struct V4C__Context *ctx ;
//  P *respnt ;						/* Ptr for result */
//  INTMODX intmodx ;
//  P *argpnts[] ;
//  COUNTER argcnt ; VTRACE trace ;
//  struct V4DPI__DimInfo *di ;
//{ struct V4IM__AggShell aggs ;
//  struct V4IM__AggHdr aggh ;
//  struct V4IM__AggData aggd ;
//  struct V4DPI__Point xptbuf,ptbuf,*ipt ;
//  int i,bytes ;
//
//	ZPH(respnt) ; respnt->Dim = di->DimId ; respnt->PntType = di->PointType ;
///*	Construct AggVal from arguments: Dim:av pt1 pt2 ... pt2 */
//	v4im_AggInit(&aggs,&aggh,&aggd,NULL) ;		/* Init the agg buffers & key */
//	for(i=2;i<=argcnt;i++)
//	 { 
//	   if (argpnts[i] == NULL)				/* If no value then append null value */
//	    { if (!v4im_AggAppendVal(ctx,&aggs,&aggh,&aggd,NULL,&ptbuf)) return(FALSE) ;
//	      continue ;
//	    } ;
//	   if (!v4im_AggAppendVal(ctx,&aggs,&aggh,&aggd,argpnts[i],&xptbuf)) return(FALSE) ;
//	 } ;
//	bytes = v4im_AggBuild(ctx,&aggs,&aggh,&aggd,FALSE,FALSE,DFLTAGGAREA) ;	/* Put it all together */
//	if (bytes > V4DPI_AlphaVal_Max)
//	 { v_Msg(ctx,ctx->ErrorMsgAux,"AggTooBig",bytes,V4DPI_AlphaVal_Max) ; return(FALSE) ; } ;
////	 v4_error(0,0,"V4EVAL","ParseAgg","AGGTOOBIG","Aggregate too big to fit in point") ;
//	memcpy(&respnt->Value.AlphaVal,&aggs,bytes) ;
//	respnt->Bytes = bytes + V4DPI_PointHdr_Bytes ;
//	return(respnt) ;
//}

/*	v4im_AggPutBlocked - Appends aggs to internal buffer, writes out only when buffer full		*/
/*	Call: ok = v4im_AggPutBlocked( ctx, aid, aggs, areax )
	  where ctx is current context,
		aggs is pointer to AggShell with current record to be appended (NULL to flush all),
		areax is area index to write into (or DFLTAGGAREA for default)	*/

LOGICAL v4im_AggPutBlocked(ctx,aggs,areaxAgg)
  struct V4C__Context *ctx ;
  struct V4IM__AggShell *aggs ;
  INDEX areaxAgg ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IM__AggVData *aggvd ;
  struct V4C__ProcessInfo *pi ;
  struct V4IM__LocalVHdr *lvh ;
  struct V4IM__AggVHdr *avh ;
  struct V4IM__AggBDataI abdi ;
  int i,rx,dim ; INDEX areax ; LOGICAL ok ;

	pi = gpi ;
	if (pi->lvh == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"@Agg headers not initialized") ; return(FALSE) ; } ;
	lvh = pi->lvh ;
	if (aggs == NULL)					/* Flushing everything? */
	 { for(i=0;i<lvh->Count;i++)
	    { if (lvh->Entry[i].STREAM.aggs == NULL) continue ; if (!lvh->Entry[i].Dirty) continue ;
	      aggs = lvh->Entry[i].STREAM.aggs ;
	      avh = lvh->Entry[i].avh ;
	      aggvd = (struct V4IM__AggVData *)((char *)aggs + aggs->Key.Bytes) ;
	      aggvd->Bytes = 
	        (char *)&aggvd->Buffer[lvh->Entry[i].STREAM.OffsetToFirst + lvh->Entry[i].STREAM.CurrentRecNum*avh->BytesPerEntry] - (char *)aggs ;
	      for(rx=lvh->Entry[i].areaX;rx<gpi->AreaAggCount;rx++)			/* Look first in any AGGREGATE areas */
	       { if (gpi->AreaAgg[rx].pcb == NULL) continue ;
	         if ((gpi->AreaAgg[rx].pcb->AccessMode & V4IS_PCB_AM_Insert) == 0) continue ;	/* Aggregate not open for update? */
	         gpi->AreaAgg[rx].pcb->PutBufPtr = (BYTE *)aggs ; gpi->AreaAgg[rx].pcb->PutBufLen = aggvd->Bytes ;
	         gpi->AreaAgg[rx].pcb->KeyPtr = (struct V4IS__Key *)aggs ; gpi->AreaAgg[rx].pcb->PutMode = V4IS_PCB_GP_Write ;
		 if (!v4is_Put(gpi->AreaAgg[rx].pcb,ctx->ErrorMsgAux)) return(FALSE) ;
		 lvh->Entry[i].Dirty = FALSE ; gpi->AggPutWCount++ ;
		 break ;
	       } ;
	      if (rx >= gpi->AreaAggCount)
	       { struct V4DPI__DimInfo *di ;
	         int relhnum,aid ;
		 di = v4dpi_DimInfoGet(ctx,aggs->Key.AuxVal) ;
		 if (gpi->RelH[di->RelHNum].ahi.BindingsUpd)
		  { relhnum = di->RelHNum ; }				/* Default Hnum for Dim allows updates- OK */
		  else { for(relhnum=gpi->HighHNum;relhnum>=gpi->LowHNum;relhnum--)	/* Find highest area allowing new entries */
			  { if (gpi->RelH[relhnum].aid == UNUSED) continue ; if (relhnum == V4DPI_WorkRelHNum) continue ;
			    if (gpi->RelH[relhnum].ahi.BindingsUpd) break ;
			  } ;
			 if (relhnum < gpi->LowHNum && gpi->RelH[V4DPI_WorkRelHNum].aid != UNUSED)
			  relhnum = V4DPI_WorkRelHNum ;			/* Assign to work area as last resort */
			 if (relhnum < gpi->LowHNum)
			  { v_Msg(ctx,ctx->ErrorMsgAux,"CtxNoUpdArea1") ; return(FALSE) ; } ;
//			  v4_error(V4E_AREANOBIND,0,"V4DPI","BindListMake","AREANOBIND","Could not find any Area available for binding updates") ;
		       } ;
		 aid = gpi->RelH[relhnum].aid ;
		 gpi->AggPutWCount++ ;
		 FINDAREAINPROCESS(areax,aid) ;
		 GRABAREAMTLOCK(areax) ;
		 v4is_Insert(aid,(struct V4IS__Key *)aggs,aggs,aggvd->Bytes,V4IS_PCB_DataMode_Data,V4IS_DataCmp_None,0,FALSE,FALSE,0,NULL) ;
		 FREEAREAMTLOCK(areax) ;
	       } ;
	      v4mm_FreeChunk(lvh->Entry[i].STREAM.aggs) ; lvh->Entry[i].STREAM.aggs = NULL ; lvh->Entry[i].Dim = 0 ;
	    } ;
	   return(TRUE) ;
	 } ;
	dim = aggs->Key.AuxVal ;				/* Get the dim associated with this aggregate */
	avh = v4im_AggGetVHdr(ctx,dim) ;			/* See if we already have avh for this dimension */
	if (avh == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"@No Agg header found") ; return(FALSE) ; } ;
	for(i=0;i<lvh->Count;i++) { if (lvh->Entry[i].Dim == dim) break ; } ;
	aggvd = (struct V4IM__AggVData *)((char *)aggs + aggs->Key.Bytes) ;
	if (lvh->Entry[i].STREAM.aggs == NULL)				/* If first time the allocate */
	 { lvh->Entry[i].STREAM.aggs = (struct V4IM__AggShell *)v4mm_AllocChunk(sizeof *aggs,FALSE) ;
	   lvh->Entry[i].STREAM.CurrentRecNum = 0 ; lvh->Entry[i].STREAM.ala = NULL ;
	   lvh->Entry[i].STREAM.OffsetToFirst = (char *)&aggvd->Buffer - (char *)aggs ;
	 } ;
//	COPYB64(abdi.biKey,aggs->Key.KeyVal.B64) ;
	switch (aggs->Key.KeyMode)
	 { default:			v_Msg(ctx,ctx->ErrorMsgAux,"@Invalid key mode: %1d",aggs->Key.KeyMode) ; return(FALSE) ;
	   case V4IS_KeyMode_Int:
	   case V4IS_KeyMode_RevInt:	abdi.biKey = aggs->Key.KeyVal.IntVal ; break ;
	   case V4IS_KeyMode_DblFlt:
	   case V4IS_KeyMode_Int2:
	   case V4IS_KeyMode_Fixed:	COPYB64(abdi.biKey,aggs->Key.KeyVal.B64) ; break ;
	 } ;	  
	if (lvh->Entry[i].biKeyLastPut > abdi.biKey)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"AggOrder",DE(BFAggregate),lvh->Entry[i].biKeyLastPut,abdi.biKey) ;
	   lvh->Entry[i].biKeyLastPut = abdi.biKey ;
	   return(FALSE) ;
	 } ;
	if (lvh->Entry[i].areaX == DFLTAGGAREA) lvh->Entry[i].areaX = areaxAgg ;	/* Save area index so when we flush a buffer we write to the correct one */
	lvh->Entry[i].biKeyLastPut = abdi.biKey ;
	memcpy(&abdi.Buffer,&aggvd->Buffer,avh->BytesPerEntry) ;
	if (lvh->Entry[i].STREAM.CurrentRecNum == 0)			/* If first record then init aggs */
	 { memcpy(lvh->Entry[i].STREAM.aggs,aggs,aggs->Key.Bytes + sizeof aggvd->Bytes + sizeof aggvd->Count) ;
	 } ;
/*	From here on, aggvd now relative to Entry[].aggs! */
	aggvd = (struct V4IM__AggVData *)((char *)lvh->Entry[i].STREAM.aggs + aggs->Key.Bytes) ;
	if (lvh->Entry[i].STREAM.CurrentRecNum >= avh->EntriesPerBuffer)
	 { aggvd->Bytes = 
	     (char *)&aggvd->Buffer[lvh->Entry[i].STREAM.OffsetToFirst + lvh->Entry[i].STREAM.CurrentRecNum*avh->BytesPerEntry] - (char *)lvh->Entry[i].STREAM.aggs ;
	   for(rx=areaxAgg;rx<gpi->AreaAggCount;rx++)			/* Look first in any AGGREGATE areas */
	    { if (gpi->AreaAgg[rx].pcb == NULL) continue ;
	      if ((gpi->AreaAgg[rx].pcb->AccessMode & V4IS_PCB_AM_Insert) != 0) break ;	/* Aggregate not open for update? */
	    } ;
	   if (rx >= gpi->AreaAggCount)
	    { if (rx >= gpi->AreaAggCount)
	       { struct V4DPI__DimInfo *di ;
	         int relhnum,aid ;
		 di = v4dpi_DimInfoGet(ctx,aggs->Key.AuxVal) ;
		 if (gpi->RelH[di->RelHNum].ahi.BindingsUpd)
		  { relhnum = di->RelHNum ; }				/* Default Hnum for Dim allows updates- OK */
		  else { for(relhnum=gpi->HighHNum;relhnum>=gpi->LowHNum;relhnum--)	/* Find highest area allowing new entries */
			  { if (gpi->RelH[relhnum].aid == UNUSED) continue ; if (relhnum == V4DPI_WorkRelHNum) continue ;
			    if (gpi->RelH[relhnum].ahi.BindingsUpd) break ;
			  } ;
			 if (relhnum < gpi->LowHNum && gpi->RelH[V4DPI_WorkRelHNum].aid != UNUSED)
			  relhnum = V4DPI_WorkRelHNum ;			/* Assign to work area as last resort */
			 if (relhnum < gpi->LowHNum)
			  { v_Msg(ctx,ctx->ErrorMsgAux,"CtxNoUpdArea1") ; return(FALSE) ; } ;
//			  v4_error(V4E_AREANOBIND,0,"V4DPI","BindListMake","AREANOBIND","Could not find any Area available for binding updates") ;
		       } ;
		 aid = gpi->RelH[relhnum].aid ;
		 FINDAREAINPROCESS(areax,aid) ;
		 GRABAREAMTLOCK(areax) ;
		 ok = (v4is_Insert(aid,(struct V4IS__Key *)lvh->Entry[i].STREAM.aggs,lvh->Entry[i].STREAM.aggs,aggvd->Bytes,V4IS_PCB_DataMode_Data,V4IS_DataCmp_None,0,FALSE,FALSE,0,ctx->ErrorMsgAux) != -1) ;
		 FREEAREAMTLOCK(areax) ;
		 if (!ok) return(FALSE) ;
	       } ;
/*	      Old code below - should delete after a while (VEH 021022) */
//	      strcpy(ctx->ErrorMsgAux,"No Areas set for UPDATE") ; return(FALSE) ;
	    } else
	    { gpi->AreaAgg[rx].pcb->PutBufPtr = (BYTE *)lvh->Entry[i].STREAM.aggs ; gpi->AreaAgg[rx].pcb->PutBufLen = aggvd->Bytes ;
	      gpi->AreaAgg[rx].pcb->KeyPtr = (struct V4IS__Key *)lvh->Entry[i].STREAM.aggs ;
	      FINDAREAINPROCESS(areax,gpi->AreaAgg[rx].pcb->AreaId) ;
	      GRABAREAMTLOCK(areax) ;
	      ok = v4is_Put(gpi->AreaAgg[rx].pcb,ctx->ErrorMsgAux) ;
	      FREEAREAMTLOCK(areax) ;
	      if (!ok) return(FALSE) ;
	    } ;
	   lvh->Entry[i].STREAM.CurrentRecNum = 0 ;
	   gpi->AggPutWCount++ ;
	 } ;
	memcpy(&aggvd->Buffer[lvh->Entry[i].STREAM.OffsetToFirst + lvh->Entry[i].STREAM.CurrentRecNum*avh->BytesPerEntry],
		&abdi,avh->BytesPerEntry) ;
	COPYB64(lvh->Entry[i].STREAM.aggs->Key.KeyVal.B64,abdi.biKey) ;	/* Key to record always highest key */
	lvh->Entry[i].STREAM.CurrentRecNum ++ ; lvh->Entry[i].Dirty = TRUE ;
	return(TRUE) ;
}

/*	v4im_AggGetBlocked - Attempt to access record within block & return value point */
/*	Call: ptr = v4im_AggGetBlocked( ctx , lvh , keypt , index , aggsptr , lenptr )
	  where ptr is pointer to value (NULL if none),
		ctx is context,
		lvh is ptr to LocalVHdr info,
		keypt is (P *) to point we are seeking,
		index is the field value index within aggregate
		aggsptr if not NULL is updated to pointer to aggs
		lenptr if not NULL is updated with field length				*/

char *v4im_AggGetBlocked(ctx,lvh,keypt,index,aggsptr,lenptr)
  struct V4C__Context *ctx ;
  struct V4IM__LocalVHdr *lvh ;
  P *keypt ;
  int index, *lenptr ;
  struct V4IM__AggShell *(*aggsptr) ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IM__AggVHdr *avh ;
  struct V4IM__AggShell *aggs ;
  struct V4IM__AggBDataI *abdi ;
  struct V4IM__AggVData *aggvd ;
  struct V4IS__ParControlBlk *pcb ;
  struct V4IS__Key key ;
  struct V4DPI__LittlePoint fkpnt ;
  int ex,j,rx,dim ; char *b ;
  B64INT biKey ; double dbl ; INDEX areax ;

	dim = keypt->Dim ;				/* The dimension we be looking for */
	switch (keypt->PntType)
	 { default:			v_Msg(ctx,NULL,"AggUnsDT",keypt->PntType,DE(Aggregate),keypt) ; return(NULL) ;
	   case V4DPI_PntType_AggRef:
	   case V4DPI_PntType_XDict:
	   CASEofINT
		biKey = (B64INT)keypt->Value.IntVal ;
		break ;
	   case V4DPI_PntType_Real:
	   case V4DPI_PntType_UTime:
	   case V4DPI_PntType_Calendar:
		GETREAL(dbl,keypt) ; biKey = (B64INT)dbl ;
		break ;
	   case V4DPI_PntType_Fixed:
	   case V4DPI_PntType_Int2:
		COPYB64(biKey,keypt->Value.FixVal) ;		/* Assume no decimal point ? */
		break ;
	 } ;
/*	Construct a fixed key with biKey value but keypt->Dim dimension */
	fixPNTv(&fkpnt,biKey) ; fkpnt.Dim = keypt->Dim ;


	for(ex=0;ex<lvh->Count;ex++) { if (dim == lvh->Entry[ex].Dim) break ; } ;
	if (ex >= lvh->Count)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"AggNoDim",keypt) ; return(NULL) ; } ;	/* Could not find? */

	if (gpi->ala != NULL)			/* Maybe have prefetch setup for this agg ? */
	 { if (gpi->ala->lar->Dim == dim)		/*  and do the dimensions match ? */
	    { lvh->Entry[ex].STREAM.ala = gpi->ala ;	/* Got a match - copy ala into lvh & clear global setting in pi->ala */
	      gpi->ala = NULL ;
	    } ;
	 } ;

	avh = lvh->Entry[ex].avh ;
	if (index > avh->Count)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"AggBadFldIndex",index,avh->Count,keypt) ; return(NULL) ; } ;
	if (lvh->Entry[ex].Dirty)			/* If current entry dirty then best write it out before continuing */
	 { v4im_AggPutBlocked(ctx,NULL,DFLTAGGAREA) ; } ;
	aggs = lvh->Entry[ex].STREAM.aggs ;
	if (aggs == NULL)
	 { lvh->Entry[ex].STREAM.aggs = (struct V4IM__AggShell *)v4mm_AllocChunk(sizeof *aggs,FALSE) ; aggs = lvh->Entry[ex].STREAM.aggs ;
	   lvh->Entry[ex].STREAM.CurrentRecNum = V4LIM_BiggestPositiveInt ; lvh->Entry[ex].Dirty = FALSE ;
	 } ;
	if (aggsptr != NULL) *aggsptr = aggs ;
	if (lvh->Entry[ex].STREAM.AggIndex == keypt->Grouping && lvh->Entry[ex].STREAM.CurrentRecNum <= avh->EntriesPerBuffer)
	 { 
	   aggvd = (struct V4IM__AggVData *)((char *)aggs + aggs->Key.Bytes) ;
	   abdi = (struct V4IM__AggBDataI *)&aggvd->Buffer[lvh->Entry[ex].STREAM.OffsetToFirst + (lvh->Entry[ex].STREAM.CurrentRecNum-1)*avh->BytesPerEntry] ;
	   if (abdi->biKey == biKey)			/* Current record match the one we want? */
	    { b = (char *)abdi->Buffer + avh->Indexes[index-1] ;		/* Link up to proper data address */
	      if (lenptr != NULL)
	       *lenptr = (index >= avh->Count ? avh->BytesPerEntry : avh->Indexes[index]) - avh->Indexes[index-1] ;
	      gpi->AggValueRepeatCount++ ;
	      return(b) ;
	    } else
	    {
	      abdi = (struct V4IM__AggBDataI *)&aggvd->Buffer[lvh->Entry[ex].STREAM.OffsetToFirst + ((++lvh->Entry[ex].STREAM.CurrentRecNum)-1)*avh->BytesPerEntry] ;
	      if (abdi->biKey == biKey)			/* Current record match the one we want? */
	       { b = (char *)abdi->Buffer + avh->Indexes[index-1] ;		/* Link up to proper data address */
	         if (lenptr != NULL)
	          *lenptr = (index >= avh->Count ? avh->BytesPerEntry : avh->Indexes[index]) - avh->Indexes[index-1] ;
		 gpi->AggValueRepeatCount++ ;
	         return(b) ;
	       } ;
	    } ;
	 } ;
	
/*	If no look-ahead then do indexed v4is_Get to grab appropriate aggs */
	if (lvh->Entry[ex].STREAM.ala == NULL)
	 { rx = (keypt->PntType == V4DPI_PntType_AggRef ? keypt->Grouping : 0) ;	/* If key via agg list then we know where to look */
	   for(;rx<gpi->AreaAggCount;rx++)			/* Look first in any AGGREGATE areas */
	    { if (gpi->AreaAgg[rx].pcb == NULL) continue ;
	      pcb = gpi->AreaAgg[rx].pcb ;
	      FINDAREAINPROCESS(areax,pcb->AreaId) ;
	      GRABAREAMTLOCK(areax) ;
	      pcb->GetMode = V4IS_PCB_GP_KeyedNext|V4IS_PCB_NoError ;
	      pcb->GetBufPtr = (BYTE *)aggs ; pcb->GetBufLen = sizeof *aggs ;
	      v4im_AggInit((struct V4IM__AggShell *)&key,NULL,NULL,(P *)&fkpnt) ;	/* Set up the aggs key WITH THE FIXED KEY VALUE */
	      pcb->KeyPtr = &key ;	   
	      v4is_Get(pcb) ;
	      FREEAREAMTLOCK(areax) ;
	      if (pcb->GetLen == UNUSED) continue ;		/* Did not find here */
	      break ;
	    } ; if (rx >= gpi->AreaAggCount) return(NULL) ;	/* Did not find in any aggregates? */
	 } else
/*	   Look-ahead enabled - grab "next" block & pray it is the correct one */
	 { lvh->Entry[ex].STREAM.aggs = v4im_ALA_ConsumeNextAggs(lvh->Entry[ex].STREAM.ala) ;
	   if (lvh->Entry[ex].STREAM.aggs == NULL) { UCstrcpyAtoU(ctx->ErrorMsgAux,lvh->Entry[ex].STREAM.ala->ErrorMsg) ; return(NULL) ; } ;
	   aggs = lvh->Entry[ex].STREAM.aggs ;
	 } ;
	aggvd = (struct V4IM__AggVData *)((char *)aggs + aggs->Key.Bytes) ;
	lvh->Entry[ex].STREAM.OffsetToFirst = (char *)&aggvd->Buffer - (char *)aggs ;
	lvh->Entry[ex].STREAM.CurrentRecNum = 0 ; lvh->Entry[ex].Dirty = FALSE ;
	aggvd = (struct V4IM__AggVData *)((char *)aggs + aggs->Key.Bytes) ;
	for(j=0;j<avh->EntriesPerBuffer;j++)
	 {
	   abdi = (struct V4IM__AggBDataI *)&aggvd->Buffer[lvh->Entry[ex].STREAM.OffsetToFirst + j*avh->BytesPerEntry] ;
	   if (biKey < abdi->biKey)		/* Not going to find match! */
	    { lvh->Entry[ex].STREAM.CurrentRecNum = V4LIM_BiggestPositiveInt ; return(NULL) ; } ;
	   if (abdi->biKey == biKey)				/* Current record match the one we want? */
	    { b = (char *)abdi->Buffer + avh->Indexes[index-1] ;/* Link up to proper data address */
	      if (lenptr != NULL)
	       *lenptr = (index >= avh->Count ? avh->BytesPerEntry : avh->Indexes[index]) - avh->Indexes[index-1] ;
	      lvh->Entry[ex].STREAM.CurrentRecNum = j+1 ; lvh->Entry[ex].STREAM.AggIndex = keypt->Grouping ;
	      return(b) ;
	    } ;
	 } ;
	lvh->Entry[ex].STREAM.CurrentRecNum = V4LIM_BiggestPositiveInt ;		/* Set to big number to force re-Get() */
	v_Msg(ctx,ctx->ErrorMsgAux,"@Could not find Agg in areas") ;
	return(NULL) ;						/* Did not find? */
}

/*	v4im_AggGetValue - Gets aggs record & retrieves value from it */
/*	Call: pt = v4im_AggGetValue( ctx , keypt , dimpt , index , respt )
	  where pt is value pt,
		ctx is context,
		keypt is point used for key to aggs
		dimpt is Dim:xxx for resulting point,
		index is the field index to retrieve within the record,
		respt is pointer to result point buffer				*/

struct V4DPI__Point *v4im_AggGetValue(ctx,keypt,dimpt,index,respt)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *keypt,*dimpt,*respt ;
  int index ;
{ struct V4IM__AggShell *aggs ;
  struct V4IM__AggHdr *aggh ;
  struct V4IM__AggData *aggd ;
  struct V4IM__AggVData *aggvd ;
  struct V4IM__AggVHdr *avh ;
  struct V4IM__LocalVHdr *lvh ;
  struct V4C__ProcessInfo *pi ;
  struct V4IS__Key key ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4DPI__DimInfo *di ;
  struct V4DPI__Point_IntMix *pim ;
  struct V4DPI__Point *ipt ;
  int lastAggIndex ;							/* Aggregate file index of last aggregate */
  int i,hl,rx,blocked,arx,dimid,pt,vallen,lvhx ; unsigned char *b ;

	lastAggIndex = UNUSED ; lvhx = UNUSED ;
	gpi->AggValueCount++ ;
	if (keypt->PntType == V4DPI_PntType_AggRef)			/* If AggRef then save index to aggregate */
	 { 
	   if (keypt->Value.IntVal == UNUSED && keypt->Grouping == 0)	/* Got aggref:NONE - don't bother looking for it */
	    { v_Msg(ctx,ctx->ErrorMsgAux,"AggNoneVal",keypt) ; return(NULL) ; } ;
	   arx = keypt->Grouping ;
	 } else { arx = 0 ; } ;
	if (arx >= V4C_AreaAgg_Max)				/* Got text table entry? */
	 { arx -= V4C_AreaAgg_Max ;
	   if (gpi->TextAgg[arx].Count != keypt->Value.IntVal)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"@Reached EOF on Text Area") ; return(NULL) ; } ;	/* No longer have link - end of text file? */
	   if (index > gpi->TextAgg[arx].vlt->Columns)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"@Column(%1d) exceeds maximum(%2d) for Text area",index,gpi->TextAgg[arx].vlt->Columns) ; return(NULL) ; } ;
	   memcpy(respt,gpi->TextAgg[arx].vlt->Col[index-1].Cur,gpi->TextAgg[arx].vlt->Col[index-1].Cur->Bytes) ;
	   return(respt) ;
	 } ;
	blocked = UNUSED ;					/* Don't know if yes or no (yet) */
	pi = gpi ;
	ZUS(ctx->ErrorMsgAux) ;
test_again:
	if (pi->lvh != NULL)					/* See if aggregate via Blocked record */
	 { lvh = pi->lvh ;
	   for(lvhx=0;lvhx<lvh->Count;lvhx++)
	    { if (lvh->Entry[lvhx].Dim != keypt->Dim) continue ;
	      lastAggIndex = lvh->Entry[lvhx].STREAM.AggIndex ;
	      if (lvh->Entry[lvhx].avh->Type == V4IM_AggHdrType_BValue)
	       { blocked = TRUE ;
	         b = v4im_AggGetBlocked(ctx,lvh,keypt,index,NULL,&vallen) ;
		 if (b != NULL) goto update_respt ;		/* Got value pointed to by b */
	       } else { blocked = FALSE ; } ;
	      break ;
	    } ;							/* If here then drop thru and try normal */
	   if (keypt->PntType == V4DPI_PntType_AggRef && lvhx >= lvh->Count && blocked != UNUSED)
	    { static int dimIdLastWarn = UNUSED ;
	      if (dimIdLastWarn != keypt->Dim) { v_Msg(ctx,UCTBUF1,"AggNoHeader",keypt) ; vout_UCText(VOUT_Warn,0,UCTBUF1) ; dimIdLastWarn = keypt->Dim ; } ;
//	      vout_UCText(VOUT_Warn,0,UClit("** Attempt to access (B)FAggregate without any header info? **\n")) ;
//	      vout_UCText(VOUT_Warn,0,UClit("   (perhaps all info is in separate Aggregate area & none in main area)\n")) ;
	    } ;
	 } ;
	v4im_AggInit((struct V4IM__AggShell *)&key,NULL,NULL,keypt) ;			/* Set up the aggs key */
	if (lvhx == UNUSED ? FALSE : lvh->Entry[lvhx].STREAM.aggs != NULL)
	 { if (arx == lastAggIndex && memcmp(lvh->Entry[lvhx].STREAM.aggs,&key,key.Bytes) == 0)		/* Same as last? */
	    { aggs = lvh->Entry[lvhx].STREAM.aggs ; gpi->AggValueRepeatCount++ ; goto gotit ; } ;
	 } ;
/*	The code immediately below will not do well in multi-threaded environment UNLESS arx is "correct" and UNIQUE for each thread!! */
//xxxxxxxxxxxxxxxxxxxxxxxxxxxx
	for(rx=arx;rx<gpi->AreaAggCount;rx++)			/* Look first in any AGGREGATE areas */
	 { if (gpi->AreaAgg[rx].pcb == NULL) continue ;
	   if (v4is_PositionKey(gpi->AreaAgg[rx].pcb->AreaId,(struct V4IS__Key *)&key,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey) continue ;
	   aggs = (struct V4IM__AggShell *)v4dpi_GetBindBuf(ctx,gpi->AreaAgg[rx].pcb->AreaId,ikde,FALSE,NULL) ;
	   if (arx == 0) arx = rx ; break ;
	 } ;
	if (gpi->AreaAggCount > 0 && blocked == UNUSED)		/* May have aggregates - look for Blocked */
	 { 
	   if (v4im_AggGetVHdr(ctx,keypt->Dim) != NULL)		/* Try & load dim */
	    goto test_again ;					/*   and if successful then test for blocked */
	   if (keypt->PntType == V4DPI_PntType_AggRef && rx >= gpi->AreaAggCount)
	    { static int dimIdLastWarn = UNUSED ;
	      if (dimIdLastWarn != keypt->Dim) { v_Msg(ctx,UCTBUF1,"AggNoHeader",keypt) ; vout_UCText(VOUT_Warn,0,UCTBUF1) ; dimIdLastWarn = keypt->Dim ; } ;
//	      vout_UCText(VOUT_Warn,0,UClit("** Attempt to access (B)FAggregate without any header info? **\n")) ;
//	      vout_UCText(VOUT_Warn,0,UClit("   (perhaps all info is in separate Aggregate area & none in main area)\n")) ;
	    } ;
	 } ;
	
	if (rx >= gpi->AreaAggCount)
	 { for(rx=V4C_CtxRelH_Max-1;rx>=0;rx--)			/* Look for aggs */
	    { if (gpi->RelH[rx].aid < 0) continue ;
	      if (v4is_PositionKey(gpi->RelH[rx].aid,(struct V4IS__Key *)&key,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey) continue ;
	      aggs = (struct V4IM__AggShell *)v4dpi_GetBindBuf(ctx,gpi->RelH[rx].aid,ikde,FALSE,NULL) ; break ;
	    } ;
	   if (rx < 0)
	    { if (ctx->ErrorMsgAux[0] == UCEOS) { v_Msg(ctx,ctx->ErrorMsgAux,"@Could not find Agg(%1P) in areas",keypt) ; } ;
	      return(NULL) ;
	    } ;
	 } ;
gotit:
	aggh = (struct V4IM__AggHdr *)((char *)aggs + aggs->Key.Bytes) ;
//	if (aggs != lastagg && aggh->Bytes < LastAggBytes)
//	 { memcpy(lastagg,aggs,aggh->Bytes) ;				/* Save to maybe save time on next call */
	if (lvhx == UNUSED ? FALSE : lvhx < lvh->Count)
	 lvh->Entry[lvhx].STREAM.AggIndex = arx ;
//	   lastAggIndex = arx ;
//	 } ;
	if (aggh->Count < 0)						/* via FAggregate? */
	 { aggvd = (struct V4IM__AggVData *)aggh ;			/* Yes - link up */
	   avh = v4im_AggGetVHdr(ctx,keypt->Dim) ;
	   if (avh == NULL)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"@No headers in Agg") ;  return(NULL) ; } ; /* No VHdr ??? */
	   if (index > avh->Count)					/* Outside of range */
	    { v_Msg(ctx,ctx->ErrorMsgAux,"@Field index(%1d) exceeds maximum(%2d)",index,avh->Count) ;  return(NULL) ; } ;
	   b = (char *)aggvd->Buffer + avh->Indexes[index-1] ;		/* Link up to proper data address */
	   vallen = (index >= avh->Count ? avh->BytesPerEntry : avh->Indexes[index]) - avh->Indexes[index-1] ;
	 } else
	 { hl = ALIGN(((char *)&aggh->Indexes[0] - (char *)aggh) + aggh->Count*sizeof(aggh->Indexes[0]) ) ;/* Bytes in aggh */
//	   hl = ALIGN( (aggh->Count+2)*sizeof(aggh->Indexes[0]) ) ;	/* Bytes in aggh */
	   aggd = (struct V4IM__AggData *)((char *)aggh + hl) ;
	   if (index < 1 || index > aggh->Count)			/* Point undefined */
	    { v_Msg(ctx,ctx->ErrorMsgAux,"@Field index(%1d) exceeds maximum(%2d)",index,aggh->Count) ;  return(NULL) ; } ;
	   if (aggh->Indexes[index-1] == V4IM_AggIndex_Undef)		/* Index entry not defined */
	    { v_Msg(ctx,ctx->ErrorMsgAux,"@Field index(%1d) references undefined point",index) ;  return(NULL) ; } ;
	   b = (char *)&aggd->Buffer + aggh->Indexes[index-1] ;		/* Pointer to value point value */
/*	   vallen of last element is whatever is left over in aggd block after REF001 (see other REF001 above!) */
	   vallen = (index >= aggh->Count ? aggh->Bytes - hl - aggh->Indexes[aggh->Count-1] : aggh->Indexes[index] - aggh->Indexes[index-1]) ;
	 } ;
update_respt:
	if (dimpt->Grouping == V4DPI_Grouping_Single || dimpt->PntType == V4DPI_PntType_Special) /* Single dimension given? */
	 { if (dimpt->Dim == Dim_Dim) { dimid = dimpt->Value.IntVal ; }
	    else { dimid = dimpt->Dim ; } ;				/* Do we have Dim:xxx or DDD:xxx ? */
	 } else
	 { pim = (struct V4DPI__Point_IntMix *)&dimpt->Value ;		/* Have multiple - match on point type */
	   dimid = pim->Entry[0].BeginInt ;				/* Default to first in case no match below */
	   pt = (unsigned char)*b ; b++ ;				/* Pull point type from first byte of "value" */
	   for(i=0;i<dimpt->Grouping;i++)				/* Loop thru all dimensions given */
	    { di = v4dpi_DimInfoGet(ctx,pim->Entry[i].BeginInt) ;
	      if (pt == di->PointType)					/* Does it match pointtype in aggregate? */
	       { dimid = di->DimId ; break ; } ;
	    } ;
	 } ;
	ipt = v4im_AggUpdRes(ctx,respt,dimid,b,0,vallen,arx) ;		/* Call routine to create actual point */
	if (ipt == NULL && dimpt->Dim != Dim_Dim) ipt = dimpt ;		/* Take default if AggVal fails */
	return(ipt) ;
}

/*	v4im_AggDelete - Deletes an Agg Record			*/
/*	Call: ok = v4im_AggDelete( ctx , keypt )
	  where ok is TRUE if ok, FALSE otherwise,
		ctx is context,
		keypt is key to aggregate to be deleted		*/

LOGICAL v4im_AggDelete(ctx,keypt)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *keypt ;
{
  struct V4IM__AggShell *aggs ;
  struct V4IM__LocalVHdr *lvh ;
  struct V4IS__Key key ;
  struct V4C__ProcessInfo *pi ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  int blocked,i,rx,arx ;

	if (keypt->PntType == V4DPI_PntType_AggRef)		/* If AggRef then save index to aggregate */
	 { arx = keypt->Grouping ; } else { arx = 0 ; } ;
	if (arx >= V4C_AreaAgg_Max)				/* Got text table entry? */
	 { v_Msg(ctx,ctx->ErrorMsgAux,"@Cannot update value within a text aggregate") ; return(FALSE) ; } ;
//	v4_error(0,0,"V4IM","UPDVAL","TEXTAGG","Cannot update value within a text aggregate") ;
	blocked = UNUSED ;					/* Don't know if yes or no (yet) */
	pi = gpi ;
test_again:
	if (pi->lvh != NULL)					/* See if aggregate via Blocked record */
	 { lvh = pi->lvh ;
	   for(i=0;i<lvh->Count;i++)
	    { if (lvh->Entry[i].Dim != keypt->Dim) continue ;
	      if (lvh->Entry[i].avh->Type == V4IM_AggHdrType_BValue)
	       { blocked = TRUE ;
	         v4im_AggGetBlocked(ctx,lvh,keypt,0,&aggs,NULL) ;
	       } else { blocked = FALSE ; } ;
	      break ;
	    } ;							/* If here then drop thru and try normal */
	 } ;
	v4im_AggInit((struct V4IM__AggShell *)&key,NULL,NULL,keypt) ;			/* Set up the aggs key */
	for(rx=arx;rx<gpi->AreaAggCount;rx++)			/* Look first in any AGGREGATE areas */
	 { if (gpi->AreaAgg[rx].pcb == NULL) continue ;
	   if (v4is_PositionKey(gpi->AreaAgg[rx].pcb->AreaId,(struct V4IS__Key *)&key,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey) continue ;
	   aggs = (struct V4IM__AggShell *)v4dpi_GetBindBuf(ctx,gpi->AreaAgg[rx].pcb->AreaId,ikde,FALSE,NULL) ;
	   arx = rx ; break ;
	 } ;
	if (gpi->AreaAggCount > 0 && blocked == UNUSED)		/* May have aggregates - look for Blocked */
	 { if (v4im_AggGetVHdr(ctx,keypt->Dim) != NULL)		/* Try & load dim */
	    goto test_again ;					/*   and if successful then test for blocked */
	   if (keypt->PntType == V4DPI_PntType_AggRef && rx >= gpi->AreaAggCount)
	    { vout_UCText(VOUT_Warn,0,UClit("** Attempt to access (B)FAggregate without any header info? **\n")) ;
	      vout_UCText(VOUT_Warn,0,UClit("   (perhaps all info is in separate Aggregate area & none in main area)\n")) ;
	      v_Msg(ctx,ctx->ErrorMsgAux,"@Agg missing header info") ; return(FALSE) ;
	    } ;
	 } ;
	
	if (rx >= gpi->AreaAggCount)
	 { for(rx=V4C_CtxRelH_Max-1;rx>=0;rx--)			/* Look for aggs */
	    { if (gpi->RelH[rx].aid < 0) continue ;
	      if (v4is_PositionKey(gpi->RelH[rx].aid,(struct V4IS__Key *)&key,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey) continue ;
	      aggs = (struct V4IM__AggShell *)v4dpi_GetBindBuf(ctx,gpi->RelH[rx].aid,ikde,FALSE,NULL) ; break ;
	    } ;
	   if (rx < 0)						/* Could not find point? */
	    { v_Msg(ctx,ctx->ErrorMsgAux,"@Agg point not found") ; return(FALSE) ; } ;
	 } ;
	v_Msg(ctx,ctx->ErrorMsgAux,"@AggDel function not implemented") ;
	return(FALSE) ;						/* Missing code ??? */
}

/*	v4im_AggUpdValue - Updates single value in an aggregate record	*/
/*	Call: ok = v4im_AggUpdValue( ctx , keypt , dimpt , index , valpt )
	  where ok is TRUE if OK, false if failed (ctx->ErrorMsgAux has error),
		ctx is context,
		keypt is point used for key to aggs,
		dimpt is point describing dimension,
		index is index within record,
		valpt is is new value to be updated			*/

LOGICAL v4im_AggUpdValue(ctx,keypt,dimpt,index,valpt)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *keypt,*dimpt,*valpt ;
  int index ;
{ struct V4DPI__DimInfo *di ;
  struct V4IM__AggShell *aggs ;
  struct V4IM__AggShell aggsx ;
  struct V4IM__AggHdr *aggh ;
  struct V4IM__AggData *aggd ;
  struct V4IM__AggVData *aggvd ;
  struct V4IM__AggVHdr *avh ;
  struct V4IM__LocalVHdr *lvh ;
  struct V4DPI__Point_IntMix *pim ;
  struct V4C__ProcessInfo *pi ;
  struct V4IS__Key key ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  int aid,dataid ; double dnum ; char tbuf[512] ; UCCHAR ucbuf[512] ;
  int i,hl,rx,blocked,arx,dimid,pt,bytes,fldlen,tbuflen,ok ; unsigned char *b ;

//	if (keypt->PntType == V4DPI_PntType_AggVal)		/* Key is aggretate itself? */
//	 { aggs = (struct V4IM__AggShell *)&keypt->Value ; goto gotit ; } ;
	if (keypt->PntType == V4DPI_PntType_AggRef)		/* If AggRef then save index to aggregate */
	 { arx = keypt->Grouping ; } else { arx = 0 ; } ;
	if (arx >= V4C_AreaAgg_Max)				/* Got text table entry? */
	 { v_Msg(ctx,ctx->ErrorMsgAux,"@Cannot update value within a text aggregate") ; return(FALSE) ; } ;
	blocked = UNUSED ;					/* Don't know if yes or no (yet) */
	pi = gpi ;
test_again:
	if (pi->lvh != NULL)					/* See if aggregate via Blocked record */
	 { lvh = pi->lvh ;
	   for(i=0;i<lvh->Count;i++)
	    { if (lvh->Entry[i].Dim != keypt->Dim) continue ;
	      if (lvh->Entry[i].avh->Type == V4IM_AggHdrType_BValue)
	       { blocked = TRUE ; bytes = UNUSED ;
	         b = v4im_AggGetBlocked(ctx,lvh,keypt,index,&aggs,NULL) ;
		 if (b != NULL) goto update_valpt ;		/* Got value pointed to by b */
	       } else { blocked = FALSE ; } ;
	      break ;
	    } ;							/* If here then drop thru and try normal */
	   if (keypt->PntType == V4DPI_PntType_AggRef && i >= lvh->Count && blocked != UNUSED)
	    { vout_UCText(VOUT_Warn,0,UClit("** Attempt to access (B)FAggregate without any header info? **\n")) ;
	      vout_UCText(VOUT_Warn,0,UClit("   (perhaps all info is in separate Aggregate area & none in main area)\n")) ;
	    } ;
	 } ;
	v4im_AggInit((struct V4IM__AggShell *)&key,NULL,NULL,keypt) ;			/* Set up the aggs key */
	for(rx=arx;rx<gpi->AreaAggCount;rx++)			/* Look first in any AGGREGATE areas */
	 { if (gpi->AreaAgg[rx].pcb == NULL) continue ;
	   if (v4is_PositionKey(gpi->AreaAgg[rx].pcb->AreaId,(struct V4IS__Key *)&key,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey) continue ;
	   aggs = (struct V4IM__AggShell *)v4dpi_GetBindBuf(ctx,gpi->AreaAgg[rx].pcb->AreaId,ikde,FALSE,&bytes) ;
	   aid = gpi->AreaAgg[rx].pcb->AreaId ; dataid = ikde->DataId ;
	   arx = rx ; break ;
	 } ;
	if (gpi->AreaAggCount > 0 && blocked == UNUSED)		/* May have aggregates - look for Blocked */
	 { if (v4im_AggGetVHdr(ctx,keypt->Dim) != NULL)		/* Try & load dim */
	    goto test_again ;					/*   and if successful then test for blocked */
	   if (keypt->PntType == V4DPI_PntType_AggRef && rx >= gpi->AreaAggCount)
	    { vout_UCText(VOUT_Warn,0,UClit("** Attempt to access (B)FAggregate without any header info? **\n")) ;
	      vout_UCText(VOUT_Warn,0,UClit("   (perhaps all info is in separate Aggregate area & none in main area)\n")) ;
	    } ;
	 } ;
	
	if (rx >= gpi->AreaAggCount)
	 { for(rx=V4C_CtxRelH_Max-1;rx>=0;rx--)			/* Look for aggs */
	    { if (gpi->RelH[rx].aid < 0) continue ;
	      if (v4is_PositionKey(gpi->RelH[rx].aid,(struct V4IS__Key *)&key,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey) continue ;
	      aid = gpi->RelH[rx].aid ; dataid = ikde->DataId ;
	      aggs = (struct V4IM__AggShell *)v4dpi_GetBindBuf(ctx,gpi->RelH[rx].aid,ikde,FALSE,&bytes) ; break ;
	    } ;
	   if (rx < 0) { v_Msg(ctx,ctx->ErrorMsgAux,"@Could not find Agg point to Update") ; return(FALSE) ; } ; /* Could not find point? */
	 } ;
	memcpy(&aggsx,aggs,bytes) ; aggs = &aggsx ;	/* Copy aggs out of v4is internal buffers */
	aggh = (struct V4IM__AggHdr *)((char *)aggs + aggs->Key.Bytes) ;
	if (aggh->Count < 0)						/* via FAggregate? */
	 { aggvd = (struct V4IM__AggVData *)aggh ;			/* Yes - link up */
	   avh = v4im_AggGetVHdr(ctx,keypt->Dim) ;
	   if (avh == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"@Could not find Agg header") ; return(FALSE) ; } ; /* No VHdr ??? */
	   if (index > avh->Count)					/* Outside of range */
	    { v_Msg(ctx,ctx->ErrorMsgAux,"@Field index(%1d) exceeds that of Agg(%2d)",index,avh->Count) ; return(FALSE) ; } ;
	   b = (char *)aggvd->Buffer + avh->Indexes[index-1] ;		/* Link up to proper data address */
	 } else
	 { hl = ALIGN( (aggh->Count+2)*sizeof(aggh->Indexes[0]) ) ;	/* Bytes in aggh */
	   aggd = (struct V4IM__AggData *)((char *)aggh + hl) ;
	   if (index < 1 || index > aggh->Count)			/* Point undefined */
	    { v_Msg(ctx,ctx->ErrorMsgAux,"@Field index(%1d) exceeds that of Agg(%2d)",index,aggh->Count) ; return(FALSE) ; } ;
	   if (aggh->Indexes[index-1] == V4IM_AggIndex_Undef)		/* Index entry not defined */
	    { v_Msg(ctx,ctx->ErrorMsgAux,"@Field index(%1d) not defined in Agg",index) ; return(FALSE) ; } ;
	   b = (char *)&aggd->Buffer + aggh->Indexes[index-1] ;		/* Pointer to value point value */
	 } ;
update_valpt:
	if (dimpt->Grouping == 0)					/* Single dimension given? */
	 { dimid = dimpt->Value.IntVal ; pt = UNUSED ;
	 } else
	 { pim = (struct V4DPI__Point_IntMix *)&dimpt->Value ;		/* Have multiple - match on point type */
	   dimid = pim->Entry[0].BeginInt ;				/* Default to first in case no match below */
	   pt = (unsigned char)*b ; b++ ;				/* Pull point type from first byte of "value" */
	   for(i=0;i<dimpt->Grouping;i++)				/* Loop thru all dimensions given */
	    { di = v4dpi_DimInfoGet(ctx,pim->Entry[i].BeginInt) ;
	      if (pt == di->PointType)					/* Does it match pointtype in aggregate? */
	       { dimid = di->DimId ; break ; } ;
	    } ;
	 } ;
update_valpt1:
	DIMINFO(di,ctx,dimid) ;
	if (di == NULL)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"@AggUpd() point not a dimension") ; return(FALSE) ; } ;
//	 v4_error(0,0,"V4IM","AggUpd","NODIM","AggUpd() point not a dimension") ;
	ok = TRUE ;
	switch(di->PointType)
	 { default:
		v_Msg(ctx,ctx->ErrorMsgAux,"@Invalid point type (Dim:%1D %2Y) for AggUpd()",di->DimId,di->PointType) ; return(FALSE) ;
//		v4_error(0,0,"V4IM","UPDVAL","INVDATA","Invalid data type for AggUpd()") ;
	   CASEofINT
		i = v4im_GetPointInt(&ok,valpt,ctx) ; memcpy(b,&i,sizeof i) ;
		break ;
	   case V4DPI_PntType_Color:
		v4im_GetPointUC(&ok,ucbuf,sizeof ucbuf,valpt,ctx) ;
		i = v_ColorNameToRef(ucbuf) ;
		if (i == 0)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"DPIAcpNoColor",di->DimId,ucbuf) ; return(FALSE) ; } ;
//		 v4_error(0,0,"V4IM","UPDVAL","INVDICT","No such COLOR entry: %s",tbuf) ;
		memcpy(b,&i,sizeof i) ;
		break ;
	   case V4DPI_PntType_Country:
		v4im_GetPointUC(&ok,ucbuf,UCsizeof(ucbuf),valpt,ctx) ;
		i = v_CountryNameToRef(ucbuf) ;
		if (i == 0)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"DPIAcpNoCntry",di->DimId,ucbuf) ; return(FALSE) ; } ;
//		  v4_error(0,0,"V4IM","UPDVAL","INVDICT","No such COUNTRY entry: %s",tbuf) ;
		memcpy(b,&i,sizeof i) ;
		break ;
	   case V4DPI_PntType_XDict:
		v4im_GetPointUC(&ok,ucbuf,UCsizeof(ucbuf),valpt,ctx) ;
		i = v4dpi_XDictEntryGet(ctx,ucbuf,di,0) ;
		if (i <= 0)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"DPIAcpPntUndefX",V4DPI_PntType_XDict,di->DimId,ucbuf) ; return(FALSE) ;
//		   v4_error(0,0,"V4IM","UPDVAL","INVDICT","No such dictionary entry: %s",ucbuf) ;
		 } ;
		memcpy(b,&i,sizeof i) ;
		break ;
	   case V4DPI_PntType_Dict:
		v4im_GetPointUC(&ok,ucbuf,UCsizeof(ucbuf),valpt,ctx) ;
		i = v4dpi_DictEntryGet(ctx,UNUSED,ucbuf,NULL,NULL) ;
		if (i <= 0)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"DPIAcpPntUndef",di->DimId,ucbuf) ; return(FALSE) ; } ;
//		  v4_error(0,0,"V4IM","UPDVAL","INVDICT","No such dictionary entry: %s",ucbuf) ;
		memcpy(b,&i,sizeof i) ;
		break ;
	   case V4DPI_PntType_Int2:
		memcpy(b,&valpt->Value.Int2Val,sizeof valpt->Value.Int2Val) ; break ;
	   case V4DPI_PntType_TeleNum:
		memcpy(b,&valpt->Value.Tele,sizeof valpt->Value.Tele) ; break ;
	   case V4DPI_PntType_UTime:
	   case V4DPI_PntType_Calendar:
	   case V4DPI_PntType_Real:
		dnum = v4im_GetPointDbl(&ok,valpt,ctx) ;
		memcpy(b,&dnum,sizeof dnum) ; break ;
	   case V4DPI_PntType_BinObj:
		memcpy(b,&valpt->Value.AlphaVal,valpt->Bytes-V4DPI_PointHdr_Bytes) ;
		break ;
	   case V4DPI_PntType_RegExpPattern:
	   CASEofChar
		if (valpt->PntType != V4DPI_PntType_Char && pt != UNUSED)	/* Have split personality */
		 { 
		   for(i=0;i<dimpt->Grouping;i++)				/* Loop thru all dimensions given */
		    { di = v4dpi_DimInfoGet(ctx,pim->Entry[i].BeginInt) ;
		      if (di->PointType != V4DPI_PntType_Char)			/* Not equal to alpha? */
		       { dimid = di->DimId ; break ; } ;
		    } ;
		   goto update_valpt1 ;
		 } ;
		fldlen = (*b > V4PT_MaxCharLenIn0 ? strlen(b+1) : *b) ;
		v4im_GetPointChar(&ok,tbuf,sizeof tbuf,valpt,ctx) ; tbuflen = strlen(tbuf) ;
		for(i=0;i<fldlen;i++) { *(b+i+1) = (i >= tbuflen ? ' ' : tbuf[i]) ; } ;
		if (fldlen > V4PT_MaxCharLenIn0) { *(b+fldlen) = '\0' ; } else { *b = fldlen ; } ;
		break ;
	 } ;
	if (!ok) { UCstrcpy(UCTBUF1,ctx->ErrorMsgAux) ; v_Msg(ctx,ctx->ErrorMsgAux,"AggUpdVal",valpt,UCTBUF1) ; return(FALSE) ; } ;
	if (pt != UNUSED) *(b-1) = di->PointType ;	/* If have multiple possibilities then set to pointtype */
	if (bytes == UNUSED)				/* If UNUSED then agg in blocked record */
	 { if (!v4im_AggPutBlocked(ctx,aggs,DFLTAGGAREA)) return(FALSE) ;
	 } else						/* Agg not blocked - rewrite */
	 { acb = (struct V4IS__AreaCB *)v4mm_ACBPtr(aid) ;
	   if (!(acb->AccessMode & V4IS_PCB_AM_Update))
	    { v_Msg(ctx,ctx->ErrorMsgAux,"AggNotUpd",gpi->AreaAgg[rx].pcb->UCFileName) ; return(FALSE) ; } ;
//	    v4_error(V4E_AREANOUPD,0,"V4IS","Put","AREANOUPD","Update operation not permitted for this area") ;
	   v4is_Replace(aid,(struct V4IS__Key *)aggs,aggs,aggs,bytes,V4IS_PCB_DataMode_Index,V4IS_DataCmp_None,dataid,0) ;
	 } ;
	return(TRUE) ;
}

P *v4im_AggUpdRes(ctx,respt,dimid,valptr,avallen,vallen,aggrx)
  struct V4C__Context *ctx ;
  P *respt ;
  int dimid,avallen,vallen,aggrx ;
  void *valptr ;
{ struct V4DPI__DimInfo *di ;
  struct V4L__ListPoint *lp ;
  struct V4L__ListBitMap *lbm ;
  struct V4L__ListBMData1 *bm1 ;
  int i ;

	DIMINFO(di,ctx,dimid) ;
	if (di == NULL)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"@Call to UpdRes with point that is not a dimension (Dim:%1D)",dimid) ;
	   return(NULL) ;
	 } ;
	ZPH(respt) ;				/* Set up resulting point */
	respt->Dim = di->DimId ; respt->PntType = di->PointType ;
	switch(respt->PntType)
	 { default:
		v_Msg(ctx,ctx->ErrorMsgAux,"@AggUpdRes - Invalid point type(%1Y) for return value",respt->PntType) ; return(NULL) ;
	   case V4DPI_PntType_Color:
	   case V4DPI_PntType_Country:
	   case V4DPI_PntType_XDict:
	   case V4DPI_PntType_Dict:
	   case V4DPI_PntType_PntIdx:
	   case V4DPI_PntType_SSVal:
	   CASEofINT
//	   case V4DPI_PntType_Decomp:
		memcpy(&respt->Value.IntVal,valptr,(sizeof respt->Value.IntVal)) ;
		respt->Bytes = V4PS_Int ; break ;
	   case V4DPI_PntType_List:
		if (di->ListDimId != 0)					/* Converting to a list-bitmap? */
		 { INITLP(respt,lp,di->DimId) lp->ListType = V4L_ListType_BitMap ; lp->Dim = di->ListDimId ;
		   lbm = (struct V4L__ListBitMap *)&lp->Buffer ;
		   lp->Bytes = (char *)&lp->Buffer[0] - (char *)lp + sizeof *lbm ;
		   memset(lbm,0,sizeof *lbm) ; lbm->LastIndex = UNUSED ;
		   bm1 = (struct V4L__ListBMData1 *)valptr ; i = bm1->MaxBit ;
		   if (i < 0 || i > 10000000)
		    { v_Msg(ctx,ctx->ErrorMsgAux,"@AggUpdRes - Bitmap appears to be corrupted (maxbits=%1d)",i) ; return(NULL) ; } ;
//		   bm1 = (struct V4L__ListBMData1 *)v4l_BitMapAlloc(0,i) ;	/* Allocate for bitmap */
		   bm1 = (struct V4L__ListBMData1 *)v4mm_AllocChunk(BM1_StructBytes(i),FALSE) ;	/* Allocate for bitmap */
//		   COPYPTR(lbm->bm1,bm1) ; memcpy(bm1,valptr,sizeof(bm1->MaxBit) + 4*((i+31)/32)) ;	/* Copy bits into new buffer */
		   COPYPTR(lbm->bm1,bm1) ; memcpy(bm1,valptr,BM1_StructBytes(i)) ;	/* Copy bits into new buffer */
		   respt->Bytes = V4DPI_PointHdr_Bytes + lp->Bytes ; break ;
		 } ;
		lp = (struct V4L__ListPoint *)valptr ;
		memcpy(&respt->Value,valptr,lp->Bytes) ;
		respt->Bytes = V4DPI_PointHdr_Bytes + lp->Bytes ; break ;
	   case V4DPI_PntType_Int2:
	   case V4DPI_PntType_TeleNum:
		memcpy(&respt->Value.Tele,valptr,(sizeof respt->Value.Tele)) ;
		respt->Bytes = V4PS_Tele ; break ;
//	   case V4DPI_PntType_XDB:
//		memcpy(&respt->Value.Int2Val,valptr,(sizeof respt->Value.Int2Val)) ;
//		respt->Bytes = V4PS_Int2 ; break ;
	   case V4DPI_PntType_V4IS:
		memcpy(&respt->Value,valptr,(sizeof(struct V4DPI__PntV4IS))) ;
		respt->Bytes = V4DPI_PointHdr_Bytes + sizeof(struct V4DPI__PntV4IS) ; break ;
	   case V4DPI_PntType_UOM:
		memcpy(&respt->Value.UOMVal,valptr,(sizeof respt->Value.UOMVal)) ;
		respt->Bytes = V4PS_UOM ; break ;
	   case V4DPI_PntType_UOMPer:
		memcpy(&respt->Value.UOMPerVal,valptr,(sizeof respt->Value.UOMPerVal)) ;
		respt->Bytes = V4PS_UOMPer ; break ;
	   case V4DPI_PntType_UOMPUOM:
		memcpy(&respt->Value.UOMPUOMVal,valptr,(sizeof respt->Value.UOMPUOMVal)) ;
		respt->Bytes = V4PS_UOMPUOM ; break ;
	   case V4DPI_PntType_Complex:
		memcpy(&respt->Value.Complex,valptr,(sizeof respt->Value.Complex)) ;
		respt->Bytes = V4PS_Complex ; break ;
	   case V4DPI_PntType_GeoCoord:
		memcpy(&respt->Value.GeoCoord,valptr,(sizeof respt->Value.GeoCoord)) ;
		respt->Bytes = V4PS_GeoCoord ; break ;
	   case V4DPI_PntType_Fixed:
		memcpy(&respt->Value.FixVal,valptr,(sizeof respt->Value.RealVal)) ;
		respt->Bytes = V4PS_Fixed ; respt->LHSCnt = di->Decimals ; break ;
	   case V4DPI_PntType_UTime:
	   case V4DPI_PntType_Calendar:
	   case V4DPI_PntType_Real:
		memcpy(&respt->Value.RealVal,valptr,(sizeof respt->Value.RealVal)) ;
		respt->Bytes = V4PS_Real ; break ;
	   case V4DPI_PntType_BigText:
		memcpy(&respt->Value.AlphaVal,valptr,vallen) ;
		respt->PntType = V4DPI_PntType_UCChar ;	/* No longer a BigText point, but UCChar */
		respt->Bytes = V4DPI_PointHdr_Bytes + vallen ; break ;
	   case V4DPI_PntType_BinObj:
		memcpy(&respt->Value.AlphaVal,valptr,vallen) ;
		respt->Bytes = V4DPI_PointHdr_Bytes + vallen ; break ;
	   case V4DPI_PntType_UCChar:
		if (avallen >= V4DPI_UCVAL_MaxSafe)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"@AggUpdRes - Attempt to create alpha point of %1d bytes (exceeds max %2d bytes)",avallen,V4DPI_UCVAL_MaxSafe) ;
		   return(NULL) ;
		 } ;
		if (avallen == 0)
		 { i = ((UCCHAR *)valptr)[0] ;
		   memcpy(&respt->Value.UCVal,valptr,(i+1)*sizeof(UCCHAR)) ;
		   UCCHARPNTBYTES2(respt,i) ;
		 } else
		 { UCCHARPNTBYTES2(respt,avallen) ;
		   memcpy(&respt->Value.UCVal[1],valptr,avallen*sizeof(UCCHAR)) ;
		 }
		break ;
	   CASEofCharmU
		if (avallen >= V4DPI_AlphaVal_Max)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"@AggUpdRes - Attempt to create alpha point of %1d bytes (exceeds max %2d bytes)",avallen,V4DPI_AlphaVal_Max) ;
		   return(NULL) ;
		 } ;
		if (avallen == 0)
		 { i = (*(unsigned char *)valptr > V4PT_MaxCharLenIn0 ? strlen((char *)valptr) : *(unsigned char *)valptr) ;	/* Get length of the string */
		   memcpy(&respt->Value.AlphaVal,valptr,i+1) ;
		   CHARPNTBYTES2(respt,i)
		 } else							/* If len given then do not have leading bytes */
		 { CHARPNTBYTES2(respt,avallen)
		   memcpy(&respt->Value.AlphaVal[1],valptr,avallen) ;
		 }
		break ;
	   case V4DPI_PntType_AggRef:				/* If resulting point also aggref then form it properly */
		memcpy(&respt->Value.IntVal,valptr,(sizeof respt->Value.IntVal)) ;
		respt->Bytes = V4PS_Int ;
		respt->Grouping = aggrx ;
		break ;
	 } ;
	return(respt) ;
}

/*	v4im_AggPut - Writes out an Aggregate from list of points	*/
/*	Call: result = v4im_AggPut( ctx , keypt , argpnts , argcnt , startarg , intmodx , isVHDR, isBlocked, mustbenew, areaIndex )
	  where result is +n for aggregate index, -n/0 for areaid, -9999 for failure (ctx->ErrorMsgAux has error),
		ctx is current context,
		keypt is key point for aggregate,
		argpnts/argcnt are intmod arguments (only use arguments 2..argcnt),
		startarg is index of first argument to write to aggregate
		intmodx is opcode,
		isVHDR is TRUE if fixed record length (FAgg),
		isBlocked is TRUE if blocking (BFAgg),
		mustbenew is TRUE if insert must be new record, FALSE if don't care,
		areaIndex is index into gpi->AreaAgg[] to write agg (DFLTAGGAREA to auto-select)	*/

int v4im_AggPut(ctx,keypt,argpnts,argcnt,startarg,intmodx,isVHDR,isBlocked,mustbenew,areaIndex)
  struct V4C__Context *ctx ;
  P *keypt ;
  P *argpnts[] ;
  int argcnt,startarg,intmodx ;
  LOGICAL isVHDR,isBlocked,mustbenew ;
  INDEX areaIndex ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IM__AggShell aggs ;
  struct V4IM__AggHdr aggh ;
  struct V4IM__AggData aggd ;
  struct V4DPI__DimInfo *di ;
  struct V4LEX__Table *vlt ;
  struct V4DPI__Point *cpt,pnt ;
  int i,j,rx,relhnum,aid,bytes,ok ; UCCHAR TableName[V4LEX_TCBMacro_NameMax+1] ; INDEX areax ;

	gpi->AggPutCount++ ;
	if (isBlocked && isVHDR)
	 { struct V4DPI__LittlePoint fkpt ;	/* If BFAgg then force key point to be Fixed */
	   double dval ; B64INT fval ;
	   fixPNT(&fkpt) ; fkpt.Dim = keypt->Dim ;	/* Keep original dimension */
	   switch (keypt->PntType)
	    { default:				v_Msg(ctx,NULL,"AggUnsDT",keypt->PntType,DE(Aggregate),keypt) ; return(FALSE) ;
	      case V4DPI_PntType_AggRef:
	      case V4DPI_PntType_XDict:
	      CASEofINT				fval = (B64INT)(keypt->Value.IntVal) ; COPYB64(fkpt.Value.FixVal,fval) ; break ;
	      case V4DPI_PntType_Real:
	      case V4DPI_PntType_Calendar:	GETREAL(dval,keypt) ; fval = (B64INT)dval ; COPYB64(fkpt.Value.FixVal,fval) ; break ;
	      case V4DPI_PntType_Fixed:
	      case V4DPI_PntType_Int2:		COPYB64(fkpt.Value.FixVal,keypt->Value.FixVal) ; break ;
	      case V4DPI_PntType_TeleNum:
		COPYB64(fkpt.Value.FixVal,keypt->Value.FixVal) ;
		{ struct V4DPI__Value_Tele *tele = (struct V4DPI__Value_Tele *)&fkpt.Value.FixVal ;
		  if (tele->IntDialCode < 0) tele->IntDialCode = -tele->IntDialCode ;	/* Normalize IntDialCode if necessary */
		} ;
		break ;
	    } ;
	   v4im_AggInit(&aggs,&aggh,&aggd,(P *)&fkpt) ;
	 } else { v4im_AggInit(&aggs,&aggh,&aggd,keypt) ; } ;
//	v4im_AggInit(&aggs,&aggh,&aggd,keypt) ;			/* Init the agg buffers & key */
	for(i=startarg;i<=argcnt;i++)
	 { if (argpnts[i]->PntType != V4DPI_PntType_TagVal)
	    { if (!v4im_AggAppendVal(ctx,&aggs,&aggh,&aggd,argpnts[i],&pnt)) return(-9999) ;
	      continue ;
	    } ;
	   switch (v4im_CheckPtArgNew(ctx,argpnts[i],&cpt,NULL))
	    { default:	    		v_Msg(ctx,ctx->ErrorMsgAux,"@Tag(%1U) used improperly",v4im_LastTagName()) ; return(-9999) ;
	      case V4IM_Tag_Unk:	return(-9999) ;
	      case V4IM_Tag_Table:
		v4im_GetPointUC(&ok,TableName,V4LEX_TCBMacro_NameMax,cpt,ctx) ;
//		for(j=0;TableName[j]!='\0';j++) { TableName[j] = toupper(TableName[j]) ; } ;
//		for(vlt=gpi->vlt;vlt!=NULL;vlt=vlt->link) { if (strcmp(vlt->Name,TableName) == 0) break ; } ;
		if (!ok) { UCstrcpy(UCTBUF1,ctx->ErrorMsgAux) ; v_Msg(ctx,ctx->ErrorMsgAux,"AggUpdVal",cpt,UCTBUF1) ; return(-9999) ; } ;
		vlt = v4eval_GetTable(ctx,TableName,NULL) ;
		if (vlt == NULL) return(-9999) ;
		for(j=0;j<vlt->Columns;j++)
		 { if (vlt->Col[j].ColumnType & (V4LEX_TableCT_Ignore | V4LEX_TableCT_Expression)) continue ;
		   if (vlt->Col[j].ColumnType & V4LEX_TableCT_Point)	/* If this is a POINT entry then evaluate point to get value */
		    { struct V4LEX__TknCtrlBlk tcb ; P ppt,*ipt ;
/*		      Column 'points' are store as text, must parse & evaluate each time */
		      v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,vlt->Col[i].Cur,ctx) ;
		      v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,UCTBUF1,V4LEX_InpMode_String) ;
		      if (!v4dpi_PointParse(ctx,&ppt,&tcb,V4DPI_PointParse_RetFalse))	/* Parse leading point to get agg key */
		       { v_Msg(ctx,ctx->ErrorMsgAux,"AggKeyEvalErr") ; return(-9999) ; } ;
		      ipt = v4dpi_IsctEval(&pnt,&ppt,ctx,0,NULL,NULL) ;
		      if (ipt == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"AggKeyEvalErr",&ppt) ; return(FALSE) ; } ;
		      if (!v4im_AggAppendVal(ctx,&aggs,&aggh,&aggd,ipt,&pnt)) return(-9999) ;
		      continue ;
		    } ;
		   if (vlt->Col[j].Cur == NULL)
		    { v_Msg(ctx,ctx->ErrorMsgAux,"@Table (%1U) Column #%2d (%3U) not defined",vlt->Name,j+1,vlt->Col[j].Name) ; return(-9999) ; } ;
		   if (!v4im_AggAppendVal(ctx,&aggs,&aggh,&aggd,vlt->Col[j].Cur,&pnt)) return(-9999) ;
		 } ;

		break ;
	    } ;
	 } ;
	bytes = v4im_AggBuild(ctx,&aggs,&aggh,&aggd,isVHDR,isBlocked,DFLTAGGAREA) ;	/* Put it all together */
	if (bytes == UNUSED) return(-9999) ;
/*	Figure out area this agg belongs to */
	for(rx=areaIndex;rx<gpi->AreaAggCount;rx++)		/* Look first in any AGGREGATE areas */
	 { if (gpi->AreaAgg[rx].pcb == NULL) continue ;
	   if (isBlocked)					/* Are we Blocking/Buffering? */
	    { if (!v4im_AggPutBlocked(ctx,&aggs,rx)) return(-9999) ;
	    } else							/* Not blocked = dump out immediately */
	    { INDEX areax ; LOGICAL ok ;
	      if ((gpi->AreaAgg[rx].pcb->AccessMode & V4IS_PCB_AM_Insert) == 0) continue ;	/* Aggregate not open for update? */
	      FINDAREAINPROCESS(areax,gpi->AreaAgg[rx].pcb->AreaId) ;
	      GRABAREAMTLOCK(areax) ;
	      gpi->AreaAgg[rx].pcb->PutBufPtr = (BYTE *)&aggs ; gpi->AreaAgg[rx].pcb->PutBufLen = bytes ;
	      gpi->AreaAgg[rx].pcb->KeyPtr = (struct V4IS__Key *)&aggs ;
	      ok = v4is_Put(gpi->AreaAgg[rx].pcb,ctx->ErrorMsgAux) ;
	      FREEAREAMTLOCK(areax) ;
	      if (!ok) return(-9999) ;
	    } ;
	   return(rx+1) ;
	 } ;
	DIMINFO(di,ctx,keypt->Dim) ;				/* If here then look in V4 area(s) */
	if (di != NULL ? gpi->RelH[di->RelHNum].ahi.BindingsUpd : FALSE)
	 { relhnum = di->RelHNum ; }				/* Default Hnum for Dim allows updates- OK */
	 else { for(relhnum=gpi->HighHNum;relhnum>=gpi->LowHNum;relhnum--)	/* Find highest area allowing new entries */
	 	 { if (gpi->RelH[relhnum].aid == UNUSED) continue ; if (relhnum == V4DPI_WorkRelHNum) continue ;
		   if (gpi->RelH[relhnum].ahi.BindingsUpd) break ;
		 } ;
		if (relhnum < gpi->LowHNum && gpi->RelH[V4DPI_WorkRelHNum].aid != UNUSED)
		 relhnum = V4DPI_WorkRelHNum ;			/* Assign to work area as last resort */
		if (relhnum < gpi->LowHNum)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"AggNoArea") ; return(-9999) ; } ;
//		 v4_error(V4E_AREANOBIND,0,"V4DPI","BindListMake","AREANOBIND","Could not find any Area available for binding updates") ;
	      } ;
	aid = gpi->RelH[relhnum].aid ;
	gpi->AggPutWCount++ ;
//	v4is_Insert(aid,&aggs,&aggs,bytes,V4IS_PCB_DataMode_Data,V4IS_DataCmp_None,0,FALSE,FALSE,0,NULL) ;
	FINDAREAINPROCESS(areax,aid) ;
	GRABAREAMTLOCK(areax) ;
	if (v4is_Insert(aid,(struct V4IS__Key *)&aggs,&aggs,bytes,V4IS_PCB_DataMode_Data,V4IS_DataCmp_None,0,FALSE,FALSE,0,ctx->ErrorMsgAux) == -1)
	 { if (mustbenew) { FREEAREAMTLOCK(areax) ; return(-9999) ; } ;
	   if (v4is_PositionKey(aid,(struct V4IS__Key *)&aggs,NULL,0,V4IS_PosDCKL) != V4RES_PosKey) { FREEAREAMTLOCK(areax) ; return(-9999) ; } ;
	   v4is_Replace(aid,(struct V4IS__Key *)&aggs,&aggs,&aggs,bytes,V4IS_PCB_DataMode_Auto,V4IS_DataCmp_None,0,0) ;
	 }
	FREEAREAMTLOCK(areax) ;
	return(-aid) ;
}

/*	v4eval_AggCAggPut - Write out CAgg Record to current agg area	*/
/*	Call: index = v4eval_AggCAggPut( ctx , pt )
	  where index is area index (UNUSED if could not write & error in ctx->ErrorMsgAux),
		ctx is context,
		pt is point to be written				*/

int v4eval_AggCAggPut(ctx,pt)
  struct V4C__Context *ctx ;
  P *pt ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4AGG__Constant vac ;
  int rx,bytes ;
  struct V4DPI__DimInfo *di ;
  int relhnum,aid ; INDEX areax ; LOGICAL ok ;

	vac.kp.fld.KeyMode = V4IS_KeyMode_Int ; vac.kp.fld.KeyType = V4IS_KeyType_V4 ;
	vac.kp.fld.Bytes = 8 ; vac.kp.fld.AuxVal = V4IS_SubType_AggConstant ;
	vac.Dim = pt->Dim ;
	memcpy(&vac.dimpt,pt,pt->Bytes) ;
	bytes = (char *)&vac.dimpt + pt->Bytes - (char *)&vac ;
/*	Figure out area this agg belongs to */
	for(rx=0;rx<gpi->AreaAggCount;rx++)			/* Look first in any AGGREGATE areas */
	 { if (gpi->AreaAgg[rx].pcb == NULL) continue ;
	   if ((gpi->AreaAgg[rx].pcb->AccessMode & V4IS_PCB_AM_Insert) == 0) continue ;	/* Aggregate not open for update? */
	   FINDAREAINPROCESS(areax,gpi->AreaAgg[rx].pcb->AreaId) ;
	   GRABAREAMTLOCK(areax) ;
	   gpi->AreaAgg[rx].pcb->PutBufPtr = (BYTE *)&vac ; gpi->AreaAgg[rx].pcb->PutBufLen = bytes ;
	   gpi->AreaAgg[rx].pcb->KeyPtr = (struct V4IS__Key *)&vac ;
	   ok = v4is_Put(gpi->AreaAgg[rx].pcb,ctx->ErrorMsgAux) ;
	   FREEAREAMTLOCK(areax) ;
	   return(ok ? rx+1 : UNUSED) ;
	 } ;


/*	No aggregates available to write - write out to temp area */
	di = v4dpi_DimInfoGet(ctx,vac.Dim) ;
	if (gpi->RelH[di->RelHNum].ahi.BindingsUpd)
	 { relhnum = di->RelHNum ; }				/* Default Hnum for Dim allows updates- OK */
	 else { for(relhnum=gpi->HighHNum;relhnum>=gpi->LowHNum;relhnum--)	/* Find highest area allowing new entries */
		 { if (gpi->RelH[relhnum].aid == UNUSED) continue ; if (relhnum == V4DPI_WorkRelHNum) continue ;
		   if (gpi->RelH[relhnum].ahi.BindingsUpd) break ;
		 } ;
		if (relhnum < gpi->LowHNum && gpi->RelH[V4DPI_WorkRelHNum].aid != UNUSED)
		 relhnum = V4DPI_WorkRelHNum ;			/* Assign to work area as last resort */
		if (relhnum < gpi->LowHNum)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"AggNoArea") ; return(UNUSED) ; } ;
//		 v4_error(V4E_AREANOBIND,0,"V4DPI","BindListMake","AREANOBIND","Could not find any Area available for binding updates") ;
	      } ;
	aid = gpi->RelH[relhnum].aid ;
	gpi->AggPutWCount++ ;
	FINDAREAINPROCESS(areax,aid) ;
	GRABAREAMTLOCK(areax) ;
	v4is_Insert(aid,(struct V4IS__Key *)&vac,&vac,bytes,V4IS_PCB_DataMode_Data,V4IS_DataCmp_None,0,FALSE,FALSE,0,NULL) ;
	FREEAREAMTLOCK(areax) ;
	return(0) ;


//VEH040326	v_Msg(ctx,ctx->ErrorMsgAux,"AggNoArea") ;
//	return(UNUSED) ;					/* Could not write out? */
}

/*	v4eval_AggCAggGet - Get Point Associated with Dimension/Agg	*
/*	Call: pt = v4eval_AggCAggGet( ctx , aggpt , dim , trace )
	  where pt is pointer to value (NULL if error in ctx->ErrorMsgAux),
		ctx is context,
		aggpt is aggref point to use,
		dim is dimension,
		trace is trace flag					*/

P *v4eval_AggCAggGet(ctx,aggpt,dim,trace)
  struct V4C__Context *ctx ;
  P *aggpt ;
  int dim,trace ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4AGG__Constant vac,*vacp ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4DPI__DimInfo *di ;
#define LCAGGCMax 5
  struct lcl__CAggCache {
    int Overflow ;		/* Number of overflow conditions (for performance tracking) */
    int Count ;
    struct {
      int agx ;			/* Index into aggregates */
      int Dim ;			/* Dimension */
      P Valpt ;			/* Value point */
     } Entry[LCAGGCMax] ;
   } ;
  static struct lcl__CAggCache *lc=NULL ;
  int arx,rx,lx,relhnum,aid ; INDEX areax ;
#ifdef V4ENABLEMULTITHREADS
  static DCLSPINLOCK cagLock = UNUSEDSPINLOCKVAL ;
#endif
	if (aggpt->PntType == V4DPI_PntType_AggRef)
	 { arx = aggpt->Grouping ; } else { arx = 0 ; } ;
	GRABMTLOCK(cagLock) ;
	if (lc == NULL) lc = (struct lcl__CAggCache *)v4mm_AllocChunk(sizeof *lc,TRUE) ;
	for(lx=0;lx<lc->Count;lx++)
	 { if (lc->Entry[lx].agx == arx && lc->Entry[lx].Dim == dim)
	    { FREEMTLOCK(cagLock) ; return(&lc->Entry[lx].Valpt) ; } ;
	 } ;
	FREEMTLOCK(cagLock) ;
	vac.kp.fld.KeyMode = V4IS_KeyMode_Int ; vac.kp.fld.KeyType = V4IS_KeyType_V4 ;
	vac.kp.fld.Bytes = 8 ; vac.kp.fld.AuxVal = V4IS_SubType_AggConstant ;
	vac.Dim = dim ;
	for(rx=arx;rx<gpi->AreaAggCount;rx++)
	 { if (gpi->AreaAgg[rx].pcb == NULL) continue ;
	   FINDAREAINPROCESS(areax,gpi->AreaAgg[rx].pcb->AreaId) ;
	   GRABAREAMTLOCK(areax) ;
	   if (v4is_PositionKey(gpi->AreaAgg[rx].pcb->AreaId,(struct V4IS__Key *)&vac,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey)
	    { FREEAREAMTLOCK(areax) ; continue ; } ;
	   vacp = (struct V4AGG__Constant *)v4dpi_GetBindBuf(ctx,gpi->AreaAgg[rx].pcb->AreaId,ikde,FALSE,NULL) ;
	   arx = rx ; break ;
	 } ;
	if (rx >= gpi->AreaAggCount)			/* Could not find record */
	 { di = v4dpi_DimInfoGet(ctx,vac.Dim) ;		/* Search update-able areas */
	   if (gpi->RelH[di->RelHNum].ahi.BindingsUpd)
	    { relhnum = di->RelHNum ; }				/* Default Hnum for Dim allows updates- OK */
	    else { for(relhnum=gpi->HighHNum;relhnum>=gpi->LowHNum;relhnum--)	/* Find highest area allowing new entries */
		    { if (gpi->RelH[relhnum].aid == UNUSED) continue ; if (relhnum == V4DPI_WorkRelHNum) continue ;
		      if (gpi->RelH[relhnum].ahi.BindingsUpd) break ;
		    } ;
		   if (relhnum < gpi->LowHNum && gpi->RelH[V4DPI_WorkRelHNum].aid != UNUSED)
		    relhnum = V4DPI_WorkRelHNum ;			/* Assign to work area as last resort */
		   if (relhnum < gpi->LowHNum)
		    { v_Msg(ctx,ctx->ErrorMsgAux,"AggNoCAgg",dim) ; return(NULL) ; } ; 
//		     v4_error(V4E_AREANOBIND,0,"V4DPI","BindListMake","AREANOBIND","Could not find any Area available for binding updates") ;
	         } ;
	   aid = gpi->RelH[relhnum].aid ;
	   FINDAREAINPROCESS(areax,aid) ;
	   GRABAREAMTLOCK(areax) ;
	   if (v4is_PositionKey(aid,(struct V4IS__Key *)&vac,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey)
	    { FREEAREAMTLOCK(areax) ; v_Msg(ctx,ctx->ErrorMsgAux,"AggNoCAgg",dim) ; return(NULL) ; } ;
	   vacp = (struct V4AGG__Constant *)v4dpi_GetBindBuf(ctx,aid,ikde,FALSE,NULL) ;
	 } ;
/*	Stash point in local cache */
	GRABMTLOCK(cagLock) ;		/* areax MUST also be locked at this point! */
	for(lx=0;lx<lc->Count;lx++)
	 { if (lc->Entry[lx].Dim == dim) break ; } ;
	if (lx >= lc->Count)
	 { if (lc->Count >= LCAGGCMax)
	    { lx = 0 ; lc->Overflow++ ;
	      if (lc->Overflow == 1000) { vout_UCText(VOUT_Warn,0,UClit("[Warning: Multiple overflows in AggVal() constant cache!]\n")) ; } ;
	    } else { lx = (lc->Count++) ; } ;
	 } ;
	lc->Entry[lx].agx = arx ; lc->Entry[lx].Dim = dim ; memcpy(&lc->Entry[lx].Valpt,&vacp->dimpt,vacp->dimpt.Bytes) ;
	FREEAREAMTLOCK(areax) ; FREEMTLOCK(cagLock) ;
	return(&lc->Entry[lx].Valpt) ;
}

/*	v4im_AggGetVHdr - Returns pointer to VHdr for a dimension	*/
/*	Call: avh = v4im_AggGetVHdr( dim )
	  where avh is resulting VHdr, NULL if cannot locate,
	  ctx is current context,
	  dim is dim to look for					*/

struct V4IM__AggVHdr *v4im_AggGetVHdr(ctx,dim)
  struct V4C__Context *ctx ;
  int dim ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4C__ProcessInfo *pi ;
  struct V4IM__LocalVHdr *lvh ;
  struct V4IM__AggVHdr *avh,avh1,*avh2 ;
  int i,rx ; INDEX areax ;
  struct V4IS__IndexKeyDataEntry *ikde ;
#ifdef V4ENABLEMULTITHREADS
  static DCLSPINLOCK gvhLock = UNUSED ;
#endif


	if (!GRABMTLOCK(gvhLock)) { v_Msg(ctx,ctx->ErrorMsgAux,"MTCantLock",UClit("gvhLock")) ; return(NULL) ; } ;
	pi = gpi ;
	if (pi->lvh == NULL) pi->lvh = (struct V4IM__LocalVHdr *)v4mm_AllocChunk(sizeof *lvh,TRUE) ;
	lvh = pi->lvh ;
	for(i=0;i<lvh->Count;i++)			/* Look for dim in local structure */
	 { if (lvh->Entry[i].Dim != dim) continue ;
	   FREEMTLOCK(gvhLock) ; return(lvh->Entry[i].avh) ;
	 } ;
	avh1.kp.fld.KeyMode = V4IS_KeyMode_Int ; avh1.kp.fld.KeyType = V4IS_KeyType_V4 ;
	avh1.kp.fld.Bytes = sizeof avh1.kp + sizeof(int) ; avh1.kp.fld.AuxVal = V4IS_SubType_VHdr ;
	avh1.Dim = dim ;
	for(rx=V4C_CtxRelH_Max-1;rx>=0;rx--)			/* Look for aggs */
	 { if (gpi->RelH[rx].aid < 0) continue ;
	   FINDAREAINPROCESS(areax,gpi->RelH[rx].aid) ;
	   GRABAREAMTLOCK(areax) ;
	   if (v4is_PositionKey(gpi->RelH[rx].aid,(struct V4IS__Key *)&avh1,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey)
	    { FREEAREAMTLOCK(areax) ; continue ; } ;
	   avh2 = (struct V4IM__AggVHdr *)v4dpi_GetBindBuf(ctx,gpi->RelH[rx].aid,ikde,FALSE,NULL) ;
	   break ;
	 } ;
	if (rx < 0)
	 { for(rx=gpi->AreaAggCount;rx>=0;rx--)		/* Could not find in areas - try Agg areas */
	    { if (gpi->AreaAgg[rx].pcb == NULL) continue ;
	      FINDAREAINPROCESS(areax,gpi->AreaAgg[rx].pcb->AreaId) ;
	      GRABAREAMTLOCK(areax) ;
	      gpi->AreaAgg[rx].pcb->GetBufPtr = (BYTE *)&avh1 ; gpi->AreaAgg[rx].pcb->GetBufLen = sizeof(avh1) ;
	      gpi->AreaAgg[rx].pcb->GetMode = V4IS_PCB_GP_Keyed | V4IS_PCB_NoError ;
	      gpi->AreaAgg[rx].pcb->KeyPtr = (struct V4IS__Key *)&avh1 ; v4is_Get(gpi->AreaAgg[rx].pcb) ; avh2 = &avh1 ;
	      if (gpi->AreaAgg[rx].pcb->GetLen != UNUSED) break ;	/* Did we get record? */
	      FREEAREAMTLOCK(areax) ;
	    } ;
	 } ;
	if (rx < 0) { avh = NULL ; goto done ; } ;
	avh = (struct V4IM__AggVHdr *)v4mm_AllocChunk(avh2->Bytes,FALSE) ;
	memcpy(avh,avh2,avh2->Bytes) ;
	FREEAREAMTLOCK(areax) ;
	if (lvh->Count >= V4IM_LocalVHdrMax)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"AggVHdrMax",V4IM_LocalVHdrMax) ; avh = NULL ; goto done ; } ;
//	 v4_error(0,0,"V4IM","AggBuild","MAXLVH","Exceeded max in LocalVHdr structure") ;
	lvh->Entry[lvh->Count].Dim = dim ; lvh->Entry[lvh->Count].avh = avh ;
	lvh->Entry[lvh->Count].STREAM.aggs = NULL ; lvh->Entry[lvh->Count].biKeyLastPut = LONG_MIN ;
	lvh->Entry[lvh->Count].STREAM.ala = NULL ; lvh->Count++ ;

done:
	FREEMTLOCK(gvhLock) ;
	return(avh) ;
}

/*	v4eval_AggParse - Parses an Aggregate from tcb & writes out		*/
/*	Call: bytes = v4eval_AggParse ( tcb , ctx , trace , respt , isVHdr , isBlocked )
	  where bytes is number of bytes or 1 if OK, <= 0 if error (ctx->ErrorMsgAux updated with error, 0=warn, -1=ignore, -2=abort),
		tcb is token control block,
		ctx is context,
		trace is trace flag,
		respt is pointer to point to be updated with AggVal, NULL if writing out to aggregate area,
		isVHdr if Fixed format aggregate (FAGG, BFAGG),
		isBlocked if writing out blocked (BFAGG)			*/

int v4eval_AggParse(tcb,ctx,trace,respt,isVHdr,isBlocked)
  struct V4LEX__TknCtrlBlk *tcb ;
  struct V4C__Context *ctx ;
  int trace ;
  struct V4DPI__Point *respt ;
  LOGICAL isVHdr,isBlocked ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IM__AggShell aggs ;
  struct V4IM__AggHdr aggh ;
  struct V4IM__AggData aggd ;
  struct V4LEX__Table *vlt ;
  struct V4DPI__DimInfo *di ;
  struct V4DPI__Point xptbuf,ptbuf,isctbuf,*ipt ;
  int rx,dimid,relhnum,aid,bytes,i,ix ; UCCHAR tablename[64] ; LOGICAL loop, ok ; INDEX areax ;
  enum DictionaryEntries errret ;
  static LOGICAL warn=FALSE ;

/*	Parsing: ( pt val1 val2 ... valn ) */
	v4lex_NextTkn(tcb,0) ;
	if (tcb->opcode != V_OpCode_Langle)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"@Expecting a left paren") ; return(FALSE) ;
//	   v4_error(0,tcb,"V4EVAL","ParseAgg","NOTLPAREN","Expecting a left paren") ;
	 } ;
	if (respt == NULL)					/* Only if saving in area ... */
	 { v4lex_NextTkn(tcb,0) ;
	   if (tcb->opcode == V_OpCode_Pound)			/* Got #table-name# ? */
	    { v4lex_NextTkn(tcb,0) ;
	      if (tcb->type != V4LEX_TknType_Keyword)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"@Expecting #table-name#") ; return(FALSE) ; } ;
//	      for(vlt=gpi->vlt;vlt!=NULL;vlt=vlt->link)
//	       { if (strcmp(vlt->Name,tcb->keyword) == 0) break ; } ;
	      UCstrcpy(tablename,tcb->UCkeyword) ; vlt = v4eval_GetTable(ctx,tablename,NULL) ;
	      if (vlt == NULL) return(FALSE) ;

	      vlt->inclusionIndex = UNUSED ; v4lex_NextTkn(tcb,0) ;
	      if (tcb->opcode == V_OpCode_Colon)
	       { v4lex_NextTkn(tcb,0) ;
		 if (tcb->type != V4LEX_TknType_Integer || tcb->integer < 1 || tcb->integer > 32) {v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; return(FALSE) ; } ;
		 vlt->inclusionIndex = tcb->integer ;
	         v4lex_NextTkn(tcb,0) ;
	       } else if (tcb->opcode != V_OpCode_Pound)  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdTblName1",tcb->type) ; return(FALSE) ; } ;
//	      v4lex_NextTkn(tcb,0) ;
//	      if (tcb->opcode != V_OpCode_Pound) { v_Msg(ctx,ctx->ErrorMsgAux,"@Unknown TABLE name (%1U)",tcb->UCstring) ; return(FALSE) ; } ;
	      ZUS(UCTBUF1) ;
	      for(i=0;i<vlt->Columns;i++)			/* Append most columns in table into ASCTBUF1 */
	       { if ((vlt->Col[i].ColumnType & (V4LEX_TableCT_Ignore | V4LEX_TableCT_Expression)) != 0) continue ;	/* Don't if IGNORE set for column */
		 if (vlt->inclusionIndex != UNUSED && (vlt->Col[i].inclusionMask & (1 << (vlt->inclusionIndex - 1))) == 0) continue ;
		 if (isVHdr)
		  { switch (vlt->Col[i].di->PointType)
		     { default:		break ;
		       CASEofChar	if (vlt->Col[i].FixedLength == 0)  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdTblNotFix",DE(FAggregate),DE(BFAggregate),DE(FixedLength),vlt->Name,vlt->Col[i].Name) ; return(FALSE) ; } ;
		     } ;
		  } ;
		 if ((vlt->Col[i].ColumnType & V4LEX_TableCT_Point) != 0)
		  { 
//		    if (vlt->Col[i].Cur->PntType != V4DPI_PntType_Special && vlt->Col[i].Cur->PntType != V4DPI_PntType_Isct)
//		     { DIMINFO(di,ctx,vlt->Col[i].Cur->Dim) ;
//		       if (di != NULL) { UCstrcat(UCTBUF1,di->DimName) ; UCstrcat(UCTBUF1,UClit(":")) ; }
//		     } ;
//		    v4dpi_PointToString(UCTBUF2,vlt->Col[i].Cur,ctx,V4DPI_FormatOpt_Echo|V4DPI_FormatOpt_AlphaQuote) ;
//		    UCstrcat(UCTBUF1,UCTBUF2) ; UCstrcat(UCTBUF1,UClit(" ")) ;

		    v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,vlt->Col[i].Cur,ctx) ;
		    UCstrcat(UCTBUF1,UCTBUF2) ; UCstrcat(UCTBUF1,UClit(" ")) ;
		  } else { UCstrcat(UCTBUF1,vlt->Col[i].Name) ; UCstrcat(UCTBUF1,UClit("* ")) ; }
	       } ;
	      v4lex_NestInput(tcb,NULL,UCTBUF1,V4LEX_InpMode_String) ;	/* Inject into tcb & continue */
	    } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
	   if (!v4dpi_PointParse(ctx,&ptbuf,tcb,V4DPI_PointParse_RetFalse))	/* Parse leading point to get agg key */
	    { v_Msg(ctx,ctx->ErrorMsgAux,"AggKeyParseErr") ; return(FALSE) ; } ;
	   dimid = ptbuf.Dim ;
	   if (ptbuf.PntType == V4DPI_PntType_Isct || ptbuf.PntType == V4DPI_PntType_Special)
	    { ipt = v4dpi_IsctEval(&isctbuf,&ptbuf,ctx,0,NULL,NULL) ; dimid = (ipt == NULL ? 0 : ipt->Dim) ;
	    } else { ipt = &ptbuf ; } ;
	   if (ipt == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"AggKeyEvalErr",&ptbuf) ; return(FALSE) ; } ;
//	      v4dpi_PointToString(ASCTBUF1,&ptbuf,ctx,-1) ;
//	      sprintf(ctx->ErrorMsgAux,"Key point in Aggregate< %s ... > - could not eval",ASCTBUF1) ; return(FALSE) ;
//	    } ;
	 } else { ipt = NULL ; } ;
	if (isBlocked && isVHdr)
	 { struct V4DPI__LittlePoint fkpt ;	/* If BFAgg then force key point to be Fixed */
	   double dval ; B64INT fval ;
	   fixPNT(&fkpt) ; fkpt.Dim = ipt->Dim ;	/* Keep original dimension */
	   switch (ipt->PntType)
	    { default:				v_Msg(ctx,NULL,"AggUnsDT",ipt->PntType,DE(Aggregate),ipt) ; return(FALSE) ;
	      case V4DPI_PntType_AggRef:
	      case V4DPI_PntType_XDict:
	      CASEofINT				fval = (B64INT)(ipt->Value.IntVal) ; COPYB64(fkpt.Value.FixVal,fval) ; break ;
	      case V4DPI_PntType_Real:
	      case V4DPI_PntType_Calendar:	GETREAL(dval,ipt) ; fval = (B64INT)dval ; COPYB64(fkpt.Value.FixVal,fval) ; break ;
	      case V4DPI_PntType_Fixed:
	      case V4DPI_PntType_Int2:		COPYB64(fkpt.Value.FixVal,ipt->Value.FixVal) ; break ;
	      case V4DPI_PntType_TeleNum:
		COPYB64(fkpt.Value.FixVal,ipt->Value.FixVal) ;
		{ struct V4DPI__Value_Tele *tele = (struct V4DPI__Value_Tele *)&fkpt.Value.FixVal ;
		  if (tele->IntDialCode < 0) tele->IntDialCode = -tele->IntDialCode ;	/* Normalize IntDialCode if necessary */
		} ;
		break ;
	    } ;
	   v4im_AggInit(&aggs,&aggh,&aggd,(P *)&fkpt) ;
	 } else { v4im_AggInit(&aggs,&aggh,&aggd,ipt) ; } ;
	for(ix=1;;ix++)
	 { v4lex_NextTkn(tcb,V4LEX_Option_ForceKW) ;		/* Get next value? */
	   if (tcb->opcode == V_OpCode_Rangle) break ;		/* If right angle then all done */
	   if (tcb->opcode == V_OpCode_LangleRangle)
	    { if (!v4im_AggAppendVal(ctx,&aggs,&aggh,&aggd,NULL,&ptbuf)) return(FALSE) ;	/* If <> then append null value */
	      continue ;
	    } ;
	   if (tcb->opcode == V_OpCode_Pound)			/* Got #table-name# ? */
	    { v4lex_NextTkn(tcb,0) ;
	      if (tcb->type != V4LEX_TknType_Keyword) { v_Msg(ctx,ctx->ErrorMsgAux,"@Expecting #table-name#") ; return(FALSE) ; } ;
//	      for(vlt=gpi->vlt;vlt!=NULL;vlt=vlt->link)
//	       { if (strcmp(vlt->Name,tcb->keyword) == 0) break ; } ;
	      UCstrcpy(tablename,tcb->UCkeyword) ; vlt = v4eval_GetTable(ctx,tablename,NULL) ;
	      if (vlt == NULL) return(FALSE) ;

	      vlt->inclusionIndex = UNUSED ; v4lex_NextTkn(tcb,0) ;
	      if (tcb->opcode == V_OpCode_Colon)
	       { v4lex_NextTkn(tcb,0) ;
		 if (tcb->type != V4LEX_TknType_Integer || tcb->integer < 1 || tcb->integer > 32) {v_Msg(ctx,ctx->ErrorMsgAux,"CmdNotInt",tcb->type) ; return(FALSE) ; } ;
		 vlt->inclusionIndex = tcb->integer ;
		 v4lex_NextTkn(tcb,0) ;
	       } else if (tcb->opcode != V_OpCode_Pound)  { v_Msg(ctx,ctx->ErrorMsgAux,"CmdTblName1",tcb->type) ; return(FALSE) ; } ;


//	      v4lex_NextTkn(tcb,0) ;
//	      if (tcb->opcode != V_OpCode_Pound)
//	       { v_Msg(ctx,ctx->ErrorMsgAux,"CmdTblName1",tcb->type) ; return(FALSE) ; } ;
//	        v4_error(0,tcb,"V4","Eval","INVTBLNAME","Expecting #table-name#") ;
	      ZUS(UCTBUF1) ;
	      for(i=0;i<vlt->Columns;i++)			/* Append most columns in table into ASCTBUF1 */
	       { if ((vlt->Col[i].ColumnType & (V4LEX_TableCT_Ignore | V4LEX_TableCT_Expression)) != 0) continue ;	/* Don't if IGNORE set for column */
		 if (vlt->inclusionIndex != UNUSED && (vlt->Col[i].inclusionMask & (1 << (vlt->inclusionIndex - 1))) == 0) continue ;
		 if ((vlt->Col[i].ColumnType & V4LEX_TableCT_Point) != 0)
		  { 
/*		    Point value is store as string, just append it to Agg argument string to be parsed/evaluated later */
		    switch (vlt->Col[i].Cur->PntType)
		     { default:		v4dpi_PointToStringML(UCTBUF2,vlt->Col[i].Cur,ctx,V4DPI_FormatOpt_Dump,V4DPI_AlphaVal_Max) ; break ;
		       CASEofChar	v4im_GetPointUC(&ok,UCTBUF2,V4TMBufMax,vlt->Col[i].Cur,ctx) ; break ;
		     } ;
		    UCstrcat(UCTBUF1,UCTBUF2) ; UCstrcat(UCTBUF1,UClit(" ")) ;
		  } else { UCstrcat(UCTBUF1,vlt->Col[i].Name) ; UCstrcat(UCTBUF1,UClit("* ")) ; }
	       } ;
	      v4lex_NestInput(tcb,NULL,UCTBUF1,V4LEX_InpMode_String) ;	/* Inject into tcb & continue */
	      continue ;
	    } ;
	   v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
	   if (!v4dpi_PointParse(ctx,&ptbuf,tcb,V4DPI_PointParse_RetFalse))	/* Get value point */
	    { v_Msg(ctx,ctx->ErrorMsgAux,"AggValParseErr",ix+1,ix) ; return(FALSE) ; } ;
	   if (ptbuf.PntType == V4DPI_PntType_Dict && !warn)
	    { warn = TRUE ; v_Msg(ctx,UCTBUF2,"*AggValNId",tablename,ptbuf.PntType,&ptbuf) ; vout_UCText(VOUT_Warn,0,UCTBUF2) ; } ;
//	      v4dpi_PointToString(ASCTBUF1,&ptbuf,ctx,0) ;
//	      sprintf(v4mbuf,"[Warning - Constant point on NId dimension (%s) in aggregate, are you sure?]\n",ASCTBUF1) ;
//	      vout_Text(VOUT_Warn,0,v4mbuf) ; warn = TRUE ;
//	    } ;
	   if (!v4im_AggAppendVal(ctx,&aggs,&aggh,&aggd,&ptbuf,&xptbuf)) return(FALSE) ;
	 } ;

/*	Done parsing aggregate proper (<....>), now look for optional error handling */
	errret = _Warn ; rx = gpi->dfltAreaAggX ;
	for (loop=TRUE;loop;)
	 { switch (v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
	    { default: v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,DE(Agg)) ; goto fail ;
	      case _EndOfLine_:
	      case _SemiColon_:		loop = FALSE ; break ;		/* Ignore if end-of-line */
	      case _Error:
	      case _Errors:
		switch (v4im_GetTCBtoEnumVal(ctx,tcb,EOLOK))
		 { default: v_Msg(ctx,ctx->ErrorMsgAux,"CmdInvKW",tcb->UCkeyword,DE(Agg)) ; goto fail ;
		   case _Abort:		errret = _Abort ; break ;
		   case _Ignore:	errret = _Ignore ; break ;
		   case _Warn:		errret = _Warn ; break ;
		 } ;
		break ;
	      case _Out:
		if (!v4dpi_PointParse(ctx,&ptbuf,tcb,V4DPI_PointParse_RetFalse)) goto fail ;
		for(rx=0;rx<gpi->AreaAggCount;rx++)
		 { if (gpi->AreaAgg[rx].aggPId.Bytes == 0) continue ; if (memcmp(&gpi->AreaAgg[rx].aggPId,&ptbuf,gpi->AreaAgg[rx].aggPId.Bytes) == 0) break ;
		 } ; if (rx >= gpi->AreaAggCount) { v_Msg(ctx,ctx->ErrorMsgAux,"AreaNoId1",&ptbuf) ; errret = _Abort ; goto fail ; } ;
		if ((gpi->AreaAgg[rx].pcb->AccessMode & V4IS_PCB_AM_Insert) == 0) { v_Msg(ctx,ctx->ErrorMsgAux,"AggNotUpd",gpi->AreaAgg[rx].pcb->UCFileName) ; errret = _Abort ; goto fail ; } ;
	    } ;
	 } ;

	bytes = v4im_AggBuild(ctx,&aggs,&aggh,&aggd,isVHdr,isBlocked,rx) ;	/* Put it all together */
	if (bytes == UNUSED) goto fail ;

	if (respt != NULL)					/* Return as a point? */
	 { if (bytes > V4DPI_AlphaVal_Max)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"AggTooBig",bytes,V4DPI_AlphaVal_Max) ; goto fail ; } ;
	   memcpy(&respt->Value.AlphaVal,&aggs,bytes) ;
	   return(bytes) ;
	 } ;
/*	Figure out area this agg belongs to */
	gpi->AggPutCount++ ;
	for(;rx<gpi->AreaAggCount;rx++)			/* Look first in any AGGREGATE areas */
	 { if (gpi->AreaAgg[rx].pcb == NULL) continue ;
	   if ((gpi->AreaAgg[rx].pcb->AccessMode & V4IS_PCB_AM_Insert) == 0) continue ;	/* Aggregate not open for update? */
	   if (isBlocked)					/* Are we Blocking/Buffering? */
	    { if (!v4im_AggPutBlocked(ctx,&aggs,rx)) goto fail ;
	    } else							/* Not blocked = dump out immediately */
	    { FINDAREAINPROCESS(areax,gpi->AreaAgg[rx].pcb->AreaId) ;
	      GRABAREAMTLOCK(areax) ;
	      gpi->AreaAgg[rx].pcb->PutBufPtr = (BYTE *)&aggs ; gpi->AreaAgg[rx].pcb->PutBufLen = bytes ;
	      gpi->AreaAgg[rx].pcb->KeyPtr = (struct V4IS__Key *)&aggs ;
	      ok = v4is_Put(gpi->AreaAgg[rx].pcb,ctx->ErrorMsgAux) ;
	      FREEAREAMTLOCK(areax) ;
	      if (!ok) goto fail ;
	    } ;
	   return(TRUE) ;
	 } ;
	DIMINFO(di,ctx,dimid) ;					/* If here then look in V4 area(s) */
	if (gpi->RelH[di->RelHNum].ahi.BindingsUpd)
	 { relhnum = di->RelHNum ; }				/* Default Hnum for Dim allows updates- OK */
	 else { for(relhnum=gpi->HighHNum;relhnum>=gpi->LowHNum;relhnum--)	/* Find highest area allowing new entries */
	 	 { if (gpi->RelH[relhnum].aid == UNUSED) continue ; if (relhnum == V4DPI_WorkRelHNum) continue ;
		   if (gpi->RelH[relhnum].ahi.BindingsUpd) break ;
		 } ;
		if (relhnum < gpi->LowHNum && gpi->RelH[V4DPI_WorkRelHNum].aid != UNUSED)
		 relhnum = V4DPI_WorkRelHNum ;			/* Assign to work area as last resort */
		if (relhnum < gpi->LowHNum)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"@Could not find any Area available for binding updates") ; goto fail ; } ;
	      } ;
	aid = gpi->RelH[relhnum].aid ;
	if (isBlocked)					/* Are we Blocking/Buffering? */
	 { if (!v4im_AggPutBlocked(ctx,&aggs,rx)) goto fail ;
	 } else 
	 { gpi->AggPutWCount++ ;
	   FINDAREAINPROCESS(areax,aid) ;
	   GRABAREAMTLOCK(areax) ;
	   ok = v4is_Insert(aid,(struct V4IS__Key *)&aggs,&aggs,bytes,V4IS_PCB_DataMode_Data,V4IS_DataCmp_None,0,FALSE,FALSE,0,ctx->ErrorMsgAux) >= 0 ;
	   FREEAREAMTLOCK(areax) ;
	   if(!ok) goto fail ;
	 } ;

	return(TRUE) ;

fail:
	switch (errret)
	 { case _Abort:		return(-2) ;
	   case _Ignore:	return(-1) ;
	   default:
	   case _Warn:		return(0) ;
	 } ;
}

/*	v4trace_ExamineState - Dumps out current state on an error */

void v4trace_ExamineState(ctx,failpt,toShow,stream)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *failpt ;
  FLAGS32 toShow ;
  int stream ;
{ struct V4LEX__CompileDir *vcd ;
  int i ; INDEX hx,cx ; UCCHAR buf[V4TMBufMax] ;
  static int eslevel = 0 ;

	if (++eslevel != 1) return ;			/* Don't want to recurse - may never get out */
	if (toShow & V4_EXSTATE_Stack)
	 { if (ctx->rtStackFail != UNUSED)
	    { int rt ; UCCHAR yyy[V_FileName_Max+50], xxx[V4TMBufMax] ;
	      struct V4ERR__Dump *ved ;
	      ved = gpi->ved ;
/*	      Set up the gpi->ved structure in case we want to reference this error later via Error() module */
	      if (ved == NULL)
	       { ved = (gpi->ved = v4mm_AllocChunk(sizeof *gpi->ved,FALSE)) ;
	         ved->bSize = (ctx->rtStackFail * 500) ; ved->ucbuf = v4mm_AllocUC(ved->bSize) ;
	         ved->cSize = (10 * 500) ; ved->ctxbuf = v4mm_AllocChunk(ved->cSize,FALSE) ;
	       } ;
	      ved->bUsed = 0 ; ved->lx = 0 ; ved->cUsed = 0 ;
/*	      Save the context points */
	      for(hx=0,cx=0;hx<V4C_CtxDimHash_Max;hx++)
	       { INDEX px ; struct V4DPI__DimInfo *di ;
		 if (ctx->DimHash[hx].Dim == 0) continue ;			/* Empty slot */
		 di = (struct V4DPI__DimInfo *)v4dpi_DimInfoGet(ctx,ctx->DimHash[hx].Dim) ;
		 if (di == NULL) continue ;					/* Trashed entry ? */
		 if ((px = ctx->DimHash[hx].CtxValIndex) < 0) continue ;
#ifdef V4_BUILD_SECURITY
		 if (di->rtFlags & V4DPI_rtDimInfo_Hidden) continue ;
#endif
		 if (ctx->CtxVal[px].Point.Bytes + ved->cUsed > ved->cSize)
		  { ved->cSize *= 1.5 ; ved->ctxbuf = realloc(ved->ctxbuf,ved->cSize) ; } ;
		 ved->ctxpts[cx] = (P *)&ved->ctxbuf[ved->cUsed] ; memcpy(ved->ctxpts[cx],&ctx->CtxVal[px].Point,ctx->CtxVal[px].Point.Bytes) ;
		 gpi->ved->cUsed += ctx->CtxVal[px].Point.Bytes ;
		 cx ++ ;
	       } ; gpi->ved->ctxpts[cx] = NULL ;


	      for(rt=ctx->rtStackFail;rt>=0;rt--)
	       { UCCHAR *srcFile ; int need, hnum = ctx->rtStack[rt].vis.c.HNum ;
	         if ((vcd =v4trace_LoadVCDforHNum(ctx,hnum,TRUE)) != NULL)
	          { srcFile = (ctx->rtStack[rt].vis.c.vcdIndex < vcd->fileCount ? vcd->File[ctx->rtStack[rt].vis.c.vcdIndex].fileName : NULL) ;
	          } else { srcFile = NULL ; } ;
	         if (srcFile != NULL)
	          { if (vcd->File[ctx->rtStack[rt].vis.c.vcdIndex].spwHash64 != 0 && vcd->File[ctx->rtStack[rt].vis.c.vcdIndex].spwHash64 != gpi->spwHash64) srcFile = UClit("restricted-file") ; } ;
	         if (srcFile != NULL) { v_Msg(ctx,yyy,"@(%1U %2U)",srcFile,v4dbg_FormatLineNum(ctx->rtStack[rt].vis.c.lineNumber)) ; }
	          else { UCstrcpy(yyy,UClit("")) ; } ;
	         v_Msg(ctx,xxx,"@* %1d %2U: %3U\n",rt,yyy,(ctx->rtStack[rt].failText != NULL ? ctx->rtStack[rt].failText : UClit(""))) ;
	         vout_UCText(stream,0,xxx) ;
		 need = 5 + 5 + (srcFile != NULL ? UCstrlen(srcFile) : 0) + (ctx->rtStack[rt].failText != NULL ? UCstrlen(ctx->rtStack[rt].failText) : 0) ;
		 if (need + ved->bUsed > ved->bSize)
		  { ved->bSize *= 1.5 ; ved->ucbuf = realloc(ved->ucbuf,(ved->bSize * sizeof(UCCHAR))) ; } ;
		 ved->level[ved->lx].lineNumber = ctx->rtStack[rt].vis.c.lineNumber ;
		 if (srcFile == NULL) { ved->level[ved->lx].srcFile = NULL ; }
		  else { ved->level[ved->lx].srcFile = &ved->ucbuf[ved->bUsed] ; UCstrcpy(ved->level[ved->lx].srcFile,srcFile) ; ved->bUsed += (UCstrlen(srcFile) + 1) ; } ;
		 if (ctx->rtStack[rt].failText == NULL) { ved->level[ved->lx].errMsg = NULL ; }
		  else { ved->level[ved->lx].errMsg = &ved->ucbuf[ved->bUsed] ; UCstrcpy(ved->level[ved->lx].errMsg,ctx->rtStack[rt].failText) ; ved->bUsed += (UCstrlen(ctx->rtStack[rt].failText) + 1) ; } ;
		 ved->lx ++ ;
	       } ;
	    } else
	    { vout_UCText(stream,0,UClit("V4 Runtime stack-\n")) ;
			for(i=ctx->rtStackX;i>0;i--)
			 { UCCHAR *srcFile ; int hnum = ctx->rtStack[i].vis.c.HNum ;
/*			   Try to link up to vcd for this hnum */
			   if (ctx->rtStack[i].isctPtr == NULL) continue ;
			   if ((vcd = v4trace_LoadVCDforHNum(ctx,hnum,TRUE)) == NULL) continue ;		/* Problems if returns FALSE */
/*			   Link up to source file for this intersection */
			   srcFile = (ctx->rtStack[i].vis.c.vcdIndex < vcd->fileCount ? vcd->File[ctx->rtStack[i].vis.c.vcdIndex].fileName : NULL) ;
			   if (vcd->File[ctx->rtStack[i].vis.c.vcdIndex].spwHash64 != 0 && vcd->File[ctx->rtStack[i].vis.c.vcdIndex].spwHash64 != gpi->spwHash64)
			    { v_Msg(ctx,UCTBUF2,"@%1d restricted-file:%3U: restricted-source\n",i,v4dbg_FormatLineNum(ctx->rtStack[i].vis.c.lineNumber)) ;
			    } else
			    { v_Msg(ctx,UCTBUF2,"@%1d %2U:%3U: %4P\n",i,(srcFile == NULL ? UClit("") : srcFile),v4dbg_FormatLineNum(ctx->rtStack[i].vis.c.lineNumber),ctx->rtStack[i].isctPtr) ;
			    } ;
			   vout_UCText(stream,0,UCTBUF2) ;
			 } ;
	    } ;
	 } ;
	if (toShow & V4_EXSTATE_Context)
	 v4ctx_ExamineCtx(ctx,TRUE,stream) ;
	
	if (toShow & V4_EXSTATE_Areas)
	 { vout_UCText(stream,0,UClit("*Areas-\n")) ;
	   for(i=gpi->LowHNum;i<=gpi->HighHNum;i++)
	    { if (gpi->RelH[i].aid == UNUSED) continue ;
	      v_Msg(ctx,buf,"@*    HNum %1d - %2U\n",i,gpi->RelH[i].pcb->UCFileName) ; vout_UCText(stream,0,buf) ;
	    } ;
	   for(i=0;i<gpi->AreaAggCount;i++)
	    { if (gpi->AreaAgg[i].pcb == NULL) continue ;
	      v_Msg(ctx,buf,"@*    Agg %1d - %2U\n",i+1,gpi->AreaAgg[i].pcb->UCFileName) ; vout_UCText(stream,0,buf) ;
	    } ;
	 } ;
	--eslevel ;
	return ;
}

/*	v4trace_LoadVCDforHNum - Loads VCD for given hnum			*/
/*	Call: vcd  = v4trace_LoadVCDforHNum( ctx , hnum, workvcd )
	  where vcd = pointer to correct structure for hnum, NULL if not found or hnum invalid,
		ctx is context,
		hnum is hierarchy number,
		workvcd is TRUE to return gpi->vcd if hnum=V4DPI_WorkRelHNum, if FALSE then return only if gpi->vcd not already assigned to another hnum	*/

struct V4LEX__CompileDir *v4trace_LoadVCDforHNum(ctx,hnum,workvcd)
  struct V4C__Context *ctx ;
  int hnum ;
  LOGICAL workvcd ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4LEX__CompileDir vcd ;
  int min, relh ; INDEX areax ;

	if (hnum < gpi->LowHNum || hnum > gpi->HighHNum) return(NULL) ;
	if (gpi->RelH[hnum].vcd != NULL) return(gpi->RelH[hnum].vcd) ;
	if (hnum == V4DPI_WorkRelHNum)				/* If work area then put current vcd UNLESS current already in another (i.e. we are creating the other area at this time) */
	 { if (workvcd) return(gpi->vcd) ;
	   for(relh=gpi->LowHNum;relh<=gpi->HighHNum;relh++) { if (gpi->RelH[relh].vcd == gpi->vcd) return(NULL) ; } ;
	   return(gpi->vcd) ;
	 } ;

	if (gpi->RelH[hnum].aid == UNUSED) return(NULL) ;

	min = ((char *)&vcd.File[0] - (char *)&vcd) ; memset(&vcd,0,min) ; vcd.Bytes = min ;
	vcd.kp.fld.KeyType = V4IS_KeyType_V4 ; vcd.kp.fld.AuxVal = V4IS_SubType_CompileDir ; vcd.kp.fld.KeyMode = V4IS_KeyMode_Int ;
	vcd.kp.fld.Bytes = 8 ; vcd.Key0 = 0 ;
	FINDAREAINPROCESS(areax,gpi->RelH[hnum].pcb->AreaId) ;
	GRABAREAMTLOCK(areax) ;
	gpi->RelH[hnum].pcb->GetBufPtr = (BYTE *)&vcd ; gpi->RelH[hnum].pcb->GetBufLen = sizeof vcd ;
	gpi->RelH[hnum].pcb->KeyPtr = (struct V4IS__Key *)&vcd ; gpi->RelH[hnum].pcb->GetMode = V4IS_PCB_GP_Keyed|V4IS_PCB_NoError ;
	v4is_Get(gpi->RelH[hnum].pcb) ;
	FREEAREAMTLOCK(areax) ;

	gpi->RelH[hnum].vcd = v4mm_AllocChunk(vcd.Bytes,FALSE) ; memcpy(gpi->RelH[hnum].vcd,&vcd,vcd.Bytes) ;
	return(gpi->RelH[hnum].vcd) ;
}


/*	v4trace_RegisterError - Registers an error - Called via REGISTER_ERROR macro	*/
void v4trace_RegisterError(ctx)
  struct V4C__Context *ctx ;
{
	if (ctx->InIsctFail) return ;			/* Don't want to register nothin' if in IsctFail() */
	if (ctx->rtStackX < V4C_Frame_Max)
	 { if (ctx->rtStackX > ctx->rtStackFail) ctx->rtStackFail = ctx->rtStackX ;
	   if (ctx->rtStack[ctx->rtStackX].failText == NULL) { ctx->rtStack[ctx->rtStackX].failText = v4mm_AllocUC(V4TMBufMax) ; ZUS(ctx->rtStack[ctx->rtStackX].failText) ; } ;
	   UCstrcpy(ctx->rtStack[ctx->rtStackX].failText,ctx->ErrorMsg) ;
	 } ;
if (UCempty(ctx->ErrorMsg))
 { printf("??????? Called RegisterError with empty ErrorMsg?\n") ; return ; } ;
//	if (UCempty(gpi->SavedErrMsg))
//	 UCstrcpy(gpi->SavedErrMsg,ctx->ErrorMsg) ;	/* Save first error only */
	UCstrcpy(gpi->SavedErrMsg,ctx->ErrorMsg) ;	/* VEH110504 - Save last error - not sure which is better at this point */
	return ;
}


#ifdef PROMPTHANDLER
/*	v4trace_PromptHandler - Handles interactive trace		*/
/*	Call: pt = v4trace_PromptHandler( ctx , updpt , isctpt )
	  where pt is resulting point to return,
		ctx is context,
		updpt is point buffer to be updated if necessary,
		isctpt is offending isct that got us here		*/

struct kwlist1 phlist[] =  { { 1, "CONTEXT" }, { 2, "FAIL" }, { 3, "HELP" }, { 4, "POINT" }, { 5, "TRACE" }, { 6, "=" }, {-1, ""} } ;

P *v4trace_PromptHandler(ctx,updpt,isctpt)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *updpt,*isctpt ;
{
  jmp_buf traceback ;
  struct V4LEX__TknCtrlBlk tcb ;
  struct V4DPI__Point point,isctpnt,*dpt ;
  int cx,i ; char buf[100] ;

	v4dpi_PointToString(ASCTBUF1,isctpt,ctx,V4DPI_FormatOpt_ShowDim) ;
	sprintf(v4mbuf,"*V4 evaluation failure on: %s\n... entering V4Trace\n",ASCTBUF1) ;
	vout_Text(VOUT_Err,0,v4mbuf) ;
	v4lex_InitTCB(&tcb,0) ; UCstrcpy(tcb.ilvl[0].prompt,UClit("V4Trace>")) ;
	COPYJMP(traceback,(gpi->environment)) ;
	if (setjmp(gpi->environment) != 0) { tcb.need_input_line = TRUE ; } ;
	for(;;)
	 { cx=v4eval_NextKeyWord(phlist,&tcb,NOEOL) ;			/* Get command */
	   switch (cx)
	    { default:
		vout_Text(VOUT_Err,0,"*Unknown V4Trace command, try HELP for help\n") ; break ;
	      case 1:	/* CONTEXT */
		v4eval_ChkEOL(&tcb) ; v4ctx_ExamineCtx(ctx) ; break ;
	      case 2:	/* FAIL */
		COPYJMP((gpi->environment),traceback) ;
		return(NULL) ;
	      case 3:	/* HELP */
		vout_Text(VOUT_Debug,0,"\n\
CONTEXT		Output current context\n\
FAIL		Cause offending isct to fail\n\
POINT xxx	Return point xxx as result of offending isct\n\
TRACE		Output a trace of isct's that got you here\n") ;
		break ;
	      case 4:	/* POINT xxx */
		if !(v4dpi_PointParse(ctx,updpt,&tcb,V4DPI_PointParse_RetFalse)
		 { sprintf(ASCTBUF1,"* Invalid point specification: %s",ctx->ErrorMsgAux) ; continue ; } ;
		v4eval_ChkEOL(&tcb) ;
		COPYJMP((gpi->environment),traceback) ;
		return(updpt) ;
	      case 5:	/* TRACE */
		if (ctx->NestedIsctEval > V4TRACE_MaxNestedIsctEval) ctx->NestedIsctEval = V4TRACE_MaxNestedIsctEval ;
		for(i=ctx->NestedIsctEval-1;i>=0;i--)
		 { if (ctx->IsctStack[i].IsctPtr == NULL) continue ;
		   v4dpi_PointToString(ASCTBUF1,ctx->IsctStack[i].IsctPtr,ctx,V4DPI_FormatOpt_ShowDim) ;
		   sprintf(v4mbuf,"%2d. %s\n",i+1,ASCTBUF1) ; vout_Text(VOUT_Debug,0,v4mbuf) ;
		 } ;
		break ;
	      case 6:	/* = isct */
		if !(v4dpi_PointParse(ctx,&isctpnt,&tcb,V4DPI_PointParse_RetFalse)
		 { sprintf(ASCTBUF1,"* Invalid point specification: %s",ctx->ErrorMsgAux) ; continue ; } ;
		v4eval_ChkEOL(&tcb) ;			/* Verify EOL */
		dpt = v4dpi_IsctEval(&point,&isctpnt,ctx,0,NULL,NULL) ;
		if (dpt == NULL) { vout_Text(VOUT_Err,0,"*No value for lookup\n") ; }
		 else { v4dpi_PointToString(ASCTBUF1,dpt,ctx,-1) ;
			sprintf(v4mbuf,"   %s\n",ASCTBUF1) ; vout_Text(VOUT_Debug,0,v4mbuf) ;
		      } ;
		break ;
	    } ;
	 } ;
}
#endif

struct V4V4IS__StructEl *v4v4is_LoadFileRef(ctx,fileref)
  struct V4C__Context *ctx ;
  int fileref ;
{ struct V4V4IS__StructEl stel,*stel2 ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4V4IS__RuntimeList *rtl ;
  int rx ;

	stel.kp.fld.KeyMode = V4IS_KeyMode_Int ; stel.kp.fld.KeyType = V4IS_KeyType_V4 ;
	stel.kp.fld.Bytes = 8 ; stel.kp.fld.AuxVal = V4IS_SubType_V4ISStEl ;
	stel.FileRef = fileref ;				/* Set up key */
	for(rx=V4C_CtxRelH_Max-1;rx>=0;rx--)
	 { if (gpi->RelH[rx].aid < 0) continue ;
	   if (v4is_PositionKey(gpi->RelH[rx].aid,(struct V4IS__Key *)&stel,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey) continue ;
	   stel2 = (struct V4V4IS__StructEl *)v4dpi_GetBindBuf(ctx,gpi->RelH[rx].aid,ikde,FALSE,NULL) ; break ;
	 } ;
	if (rx < 0) return(NULL) ;				/* Could not find? */
	if (gpi->rtl == NULL)				/* Is this the first one? */
	 { gpi->rtl = (struct V4V4IS__RuntimeList *)v4mm_AllocChunk(sizeof(*gpi->rtl),TRUE) ; } ;
	rtl = gpi->rtl ;
	if (rtl->Count >= V4V4IS_RTList_Max)
	 v4_error(0,0,"V4V4IS","LoadFileRef","MAXREFS","Exceeded max (%d) number of runtime file refs",V4V4IS_RTList_Max) ;
	rtl->FileRefs[rtl->Count] = fileref ;
	rtl->stel[rtl->Count] = (struct V4V4IS__StructEl *)v4mm_AllocChunk(stel2->Bytes,FALSE) ;
	memcpy(rtl->stel[rtl->Count],stel2,stel2->Bytes) ;	/* Copy stel into in-core buffer */
	return(stel2) ;
}

void v4v4is_FlushStEl(ctx)
  struct V4C__Context *ctx ;
{ struct V4V4IS__StructEl *stel ;
  struct V4V4IS__RuntimeList *rtl ;
  int fx,rx,sx,relhnum,aid ;

	rtl = gpi->rtl ;
	if (rtl == NULL) return ;					/* Nothing to flush */
	for(rx=0;rx<rtl->Count;rx++)
	 { if (rtl->IsNew[rx] == FALSE) continue ;
	   for(relhnum=gpi->HighHNum;relhnum>=gpi->LowHNum;relhnum--)	/* Find highest area allowing new entries */
	    { if (gpi->RelH[relhnum].aid == UNUSED) continue ; if (relhnum == V4DPI_WorkRelHNum) continue ;
	      if (gpi->RelH[relhnum].ahi.BindingsUpd) break ;
	    } ;
	   if (relhnum < gpi->LowHNum && gpi->RelH[V4DPI_WorkRelHNum].aid != UNUSED)
	    relhnum = V4DPI_WorkRelHNum ;				/* Assign to work area as last resort */
	   if (relhnum < gpi->LowHNum)
	    v4_error(V4E_AREANOBIND,0,"V4DPI","FlushStEl","AREANOBIND","Could not find any Area available for StEl updates") ;
	   aid = gpi->RelH[relhnum].aid ; stel = rtl->stel[rx] ;
/*	   ****NOTE: THIS ONLY WORKS FOR AREAS WITH ONE LEVEL OF SUBSTRUCTURE**** */
	   for(fx=0;fx<stel->Count;fx++)				/* Go thru & find any elements within substruct */
	    { if (stel->Field[fx].StructNum == 0) continue ;		/* Ignore "top level" fields */
	      for(sx=0;sx<stel->Count;sx++)				/* Look for corresponding structure def */
	       { if (stel->Field[sx].V3DataType != UNUSED) continue ;	/* Not a structure def */
	         if (stel->Field[sx].StructNum != stel->Field[fx].StructNum) continue ;
		 stel->Field[fx].Offset += stel->Field[sx].Offset ;	/* Add offset of parent structure */
		 stel->Field[fx].SubBytes = stel->Field[sx].Bytes ;
	       } ;
	    } ;
	   stel->Bytes = (char *)&stel->Field[stel->Count].StructNum - (char *)stel ;
	   v4is_Insert(aid,(struct V4IS__Key *)stel,stel,stel->Bytes,V4IS_PCB_DataMode_Auto,V4IS_DataCmp_None,0,FALSE,FALSE,0,NULL) ;
	   rtl->IsNew[rx] = FALSE ;					/* Mark as updated/flushed! */
	 } ;
}

/*	S C A N N I N G   M O D U L E S				*/

//struct V4DPI__BindListShell *v4eval_ReadNextNBL(/* aid,posflag,kp */) ;
//struct V4IS__AreaCB *v4mm_ACBPtr( /* int */) ;

int v4eval_ScanAreaForBinds(ctx,dimid,handle,isct,valpt,pisct)
  struct V4C__Context *ctx ;
  int dimid,handle ;
  struct V4DPI__Point *isct,*valpt,*pisct ;
{ struct V4DPI__Point *pipt,*iipt ;
  int checkcontext = TRUE ;
  char list[100] ; int px,ix ;

	for(;;)
	 { handle = v4eval_ScanForDim(ctx,dimid,handle,isct,valpt) ;
	   if (handle == 0) return(0) ;					/* End of the line - no more */
	   memset(list,0,sizeof list) ;
	   if (pisct != NULL)						/* Do we have a pattern to match ? */
	    { for(px=0,pipt=ISCT1STPNT(pisct);px<pisct->Grouping;px++,ADVPNT(pipt))
	       { for(ix=0,iipt=ISCT1STPNT(isct);ix<isct->Grouping;ix++,ADVPNT(iipt))
		  { if (pipt->Dim != iipt->Dim)	continue ;		/* Dim's don't match */
		    if (pipt->Grouping == V4DPI_Grouping_All || pipt->Grouping == V4DPI_Grouping_AllCnf) /* Just match on dim */
		     break ;
		    if (pipt->Bytes != iipt->Bytes) continue ;
		    if (memcmp(&pipt->Value,&iipt->Value,iipt->Bytes-V4DPI_PointHdr_Bytes) == 0) break ;
		  } ;
		 if (ix >= isct->Grouping)				/* If did not pipt in any of the points in isct */
		  goto next_bind ;
		 list[ix] = TRUE ;					/* Mark this as a matched point */
	       } ;
	    } ;
	   if (checkcontext)
	    { for(ix=0,iipt=ISCT1STPNT(isct);ix<isct->Grouping;ix++,ADVPNT(iipt))
	       { if (list[ix]) continue ;				/* Don't check if matched via above match list */
		 DIMVAL(pipt,ctx,iipt->Dim) ;			/* pipt = corresponding point in context */
		 if (pipt == NULL) continue ;				/* Not in context - that's OK */
		 if (iipt->Grouping == V4DPI_Grouping_All || iipt->Grouping == V4DPI_Grouping_AllCnf)
		  { list[ix] = TRUE ; continue ; } ;
		 if (pipt->Bytes != iipt->Bytes) goto next_bind ;
		 if (memcmp(&pipt->Value,&iipt->Value,iipt->Bytes-V4DPI_PointHdr_Bytes) != 0) goto next_bind ;
		 list[ix] = TRUE ;
	       } ;
	    } ;
	   return(handle) ;
next_bind: continue ;
	 } ;
}

/*	v4eval_ScanForDim - Scans all areas in context for bindings containing a dimension	*/
/*	Call: rhandle = v4eval_ScanForDim( ctx , dimid , handle , isct , valpt )
	  where rhandle is handle for "next" call, 0 if no more,
		ctx is context,
		dimid is dimension to find,
		handle is handle from prior call (0 to get started),
		isct is pointer to area updated with binding intersection,
		valpt is pointer to area updated with value point			*/

int v4eval_ScanForDim(ctx,dimid,handle,isct,valpt)
  struct V4C__Context *ctx ;
  int dimid,handle ;
  struct V4DPI__Point *isct,*valpt ;
{ static struct V4DPI__BindListShell *snbls = NULL ;
  struct V4DPI__BindListShell *nbls ;
  struct V4DPI__BindList *nbl ;
  union sfd__handle
   { int handle ;
     struct { short rhx ; short nblx ; } fld ;
   } sfdh ;
  int posflag ;

	if (handle == 0)
	 { if (snbls != NULL) { v4mm_FreeChunk(snbls) ; snbls = NULL ; } ;
	   sfdh.fld.rhx = gpi->LowHNum ; sfdh.fld.nblx = 0 ; posflag = V4IS_PCB_GP_BOF ;
	 } else { sfdh.handle = handle ; posflag = V4IS_PCB_GP_Keyed ; } ;
	if (sfdh.fld.nblx > 0)					/* Maybe search current nbls */
	 { sfdh.fld.nblx = v4eval_NextDimInBind(ctx,gpi->RelH[sfdh.fld.rhx].aid,snbls,sfdh.fld.nblx,dimid,valpt,isct) ;
	   if (sfdh.fld.nblx != 0) return(sfdh.handle) ;
	 } ;

	for(;sfdh.fld.rhx<=gpi->HighHNum;sfdh.fld.rhx++)
	 { if (gpi->RelH[sfdh.fld.rhx].aid < 0) continue ;		/* Nothing in this slot */
	   for(;;)
	    {
	      nbls = v4eval_ReadNextNBL(ctx,gpi->RelH[sfdh.fld.rhx].aid,posflag,(union V4IS__KeyPrefix *)snbls) ;	/* Get next nbls bucket */
	      if (nbls == NULL) break ;					/* No more in this area - keep plugging */
	      sfdh.fld.nblx = v4eval_NextDimInBind(ctx,gpi->RelH[sfdh.fld.rhx].aid,nbls,0,dimid,valpt,isct) ;
	      if (sfdh.fld.nblx == 0) { posflag = V4IS_PCB_GP_Next ; continue ; } ;	/* Nothing in this one, look again */
	      if (snbls == NULL)
	       snbls = (struct V4DPI__BindListShell *)v4mm_AllocChunk(sizeof *snbls,FALSE) ;
	      nbl = (struct V4DPI__BindList *)((char *)nbls + nbls->blsk.Bytes) ;
	      memcpy(snbls,nbls,nbl->Bytes + nbls->blsk.Bytes) ;			/* Save nbls for next call */
	      return(sfdh.handle) ;
	    } ;
	  posflag = V4IS_PCB_GP_BOF  ;
	 } ;
	if (snbls != NULL) { v4mm_FreeChunk(snbls) ; snbls = NULL ; } ;
	return(0) ;							/* End of the line! */
}

/*	v4eval_ReadNextNBL - Reads area for next nbl record		*/
/*	Call: nblptr = v4eval_ReadNextNBL( ctx , aid , posflag , kp )
	  where nblptr is pointer to next nbl shell, NULL if end of area reached,
		ctx is context,
		aid is area id,
		posflag is one of V4IS_PCB_GP_xxx,
		kp is pointer to key if posflag = V4IS_PCB_GP_Keyed (for restarting at certain point)	*/

struct V4DPI__BindListShell *v4eval_ReadNextNBL(ctx,aid,posflag,kp)
  struct V4C__Context *ctx ;
  int aid,posflag ;
  union V4IS__KeyPrefix *kp ;
{ struct V4DPI__BindListShell *nbls ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__IndexBktHdr *ibh ;
  struct V4IS__Bkt *bkt ;
  struct V4IS__IndexBktKeyList *ibkl ;
  struct V4IS__IndexKeyDataEntry *ikde ;

	if (posflag == V4IS_PCB_GP_Keyed)			/* If got a key then position to use as starting point */
	 { v4is_PositionKey(aid,(struct V4IS__Key *)kp,NULL,0,V4IS_PosDCKL) ; posflag = V4IS_PCB_GP_Next ; } ;
	acb = v4mm_ACBPtr(aid) ;
	for(;;posflag=V4IS_PCB_GP_Next)
	 {
	   if (! v4is_PositionRel(aid,posflag,-1,NULL)) break ;
	   bkt = v4mm_BktPtr(acb->Lvl[acb->CurLvl].BktNum,aid,V4MM_LockIdType_Index) ;
	   ibh = (struct V4IS__IndexBktHdr *)bkt ;	/* Get the bucket */
	   ibkl = (struct V4IS__IndexBktKeyList *)&bkt->Buffer[ibh->KeyEntryTop] ;	/* Pointer to B header & Key indexes */
	   ikde = (struct V4IS__IndexKeyDataEntry *)&bkt->Buffer[ibkl->DataIndex[acb->CurPosKeyIndex]] ;
	   nbls = (struct V4DPI__BindListShell *)v4dpi_GetBindBuf(ctx,aid,ikde,FALSE,NULL) ;
	   if (nbls->blsk.KeyType != V4IS_KeyType_Binding) { IBHINUSE ; continue ; } ; /* Not correct type */
	   return(nbls) ;
	 } ;
	return(NULL) ;						/* Hit End-of-Area */
}

/*	v4eval_NextDimInBind - Finds next binding in bindlist containing specified dimension	*/
/*	Call: rnblvx = v4eval_NextDimInBind(ctx,aid,nbls,nblvx,dimid,vpt,isct)
	  where rnblvx is index for use in next call (0 if no more matches),
		ctx is context,
		aid is area we are looking at,
		nbls is pointer to bind list (struct V4DPI__BindListShell),
		nblvx is index to use for successive calls (0 for first),
		dimid is dimension id we are looking for,
		vpt is pointer to POINT to be updated with value point,
		isct is pointer to ISCT to be updated with full binding ISCT			*/

int v4eval_NextDimInBind(ctx,aid,nbls,nblvx,dimid,vpt,isct)
  struct V4C__Context *ctx ;
  int aid ;
  struct V4DPI__BindListShell *nbls ;
  int nblvx ;
  int dimid ;
  struct V4DPI__Point *vpt ;
  struct V4DPI__Point *isct ;
{
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4DPI__Binding bind,*binding ;
  struct V4DPI__BindList *nbl ;
  struct V4DPI__BindList_Value *nblv ;
  struct V4DPI__Point *ipt,*pt,xpnt ;
  struct V4DPI__BindList_Dim *nbld ;
  static char list[V4DPI_BindList_DimMax] ;
  int i,j,hit ;

	nbl = (struct V4DPI__BindList *)((char *)nbls + nbls->blsk.Bytes) ;
	nblv = (struct V4DPI__BindList_Value *)&nbl->Buffer[nbl->ValueStart] ;
	nbld = (struct V4DPI__BindList_Dim *)&nbl->Buffer[nbl->DimStart] ;
	if (nblvx == 0)						/* If first time, scan list of dims to see if anything */
	 { memset(list,FALSE,sizeof list) ;			/* Want all positions containing "dimid" */
	   for(i=0,hit=FALSE;i<nbld->DimCnt;i++)
	    { ipt = (struct V4DPI__Point *)&nbl->Buffer[nbl->PointStart+nbld->DimEntryIndex[i]] ;
	      if (ipt->Dim == dimid) { hit = TRUE ; list[i] = TRUE ; } ;
	    } ; if (!hit) return(0) ;				/* Nothing ? */
	 } ;
	for(i=0;i<nbl->ValueCnt;i++,nblv = (struct V4DPI__BindList_Value *)((char *)nblv + nblv->Bytes))
	 { if(i < nblvx) continue ;					/* Advance to proper position */
	   for(j=0;j<nblv->IndexCnt;j++) { if (list[nblv->DimPos[j]]) break ; } ;
	   if (j >= nblv->IndexCnt) continue ;			/* Got no matches */
	   if (nblv->sp.Bytes == 0)					/* Is value in the short point or in value record */
	    { bind.kp.fld.AuxVal = V4IS_SubType_Value ; bind.kp.fld.KeyType = V4IS_KeyType_V4 ;
	      bind.kp.fld.KeyMode = V4IS_KeyMode_Int ; bind.kp.fld.Bytes = V4IS_IntKey_Bytes ; bind.ValueId = nblv->sp.Value.IntVal ;
	      if (v4is_PositionKey(aid,(struct V4IS__Key *)&bind,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey)
	       v4_error(V4E_NOVALREC,0,"V4IM","NextDimInBind","NOVALREC","Could not access value record") ;
	      binding = (struct V4DPI__Binding *)v4dpi_GetBindBuf(ctx,aid,ikde,FALSE,NULL) ;
	      ipt = (struct V4DPI__Point *)&binding->Buffer ;
	    } else								/* Value is in short point - link up */
	    { ZPH(&xpnt) ;
	      xpnt.Dim = nblv->sp.Dim ; xpnt.Bytes = nblv->sp.Bytes ; xpnt.PntType = nblv->sp.PntType ;
	      xpnt.Grouping = V4DPI_Grouping_Single ; xpnt.LHSCnt = nblv->sp.LHSCnt ;
	      memcpy(&xpnt.Value,&nblv->sp.Value,nblv->sp.Bytes-V4DPI_PointHdr_Bytes) ;
	      ipt = &xpnt ;
	    } ;
	   if (vpt != NULL) memcpy(vpt,ipt,ipt->Bytes) ;		/* Return value to caller */
	   if (isct != NULL)
	    { INITISCT(isct) ; NOISCTVCD(isct) ; isct->Grouping = nblv->IndexCnt ;
	      for(j=0,pt=ISCT1STPNT(isct);j<nblv->IndexCnt;j++)
	       { ipt = (struct V4DPI__Point *)&nbl->Buffer[nbl->PointStart+nbld->DimEntryIndex[nblv->DimPos[j]]] ;
	         memcpy(pt,ipt,ipt->Bytes) ; ADVPNT(pt) ;
	       } ;
	      ISCTLEN(isct,pt) ;
	    } ;
	   return(i+1) ;						/* Return index for next call */
	 } ;
	return(0) ;							/* End of the line */
}

/*	v4eval_OptPoint - Optimizes references to iscts of form: x.y (replaces with first eval (e.g. AggVal())) */
/*	Call: bytes = v4eval_OptPoint( ctx , opt , respnt , maxbytes , odl )
	  where bytes is number of bytes used in respnt for optimization of opt point (-1 if error),
		ctx is current context,
		opt is pointer to isct to optimize,
		respnt is point to resulting point,
		maxbytes is maximum number of bytes in respnt,
		odl, if not NULL, is list of dimensions to OK to optimize (if NULL then all dims are OK) */

int v4eval_OptPoint(ctx,opt,respnt,maxbytes,odl)
  struct V4C__Context *ctx ;
  P *opt,*respnt ;
  int maxbytes ;
  struct V4EVAL_OptDimList *odl ;
{ P *tpt,*xpt,*respnt1,pt,rpt,listbuf ;
  struct V4L__ListPoint *lp,*lp1 ;
  struct V4DPI__TagVal *tv,*tv1 ;
  int b,ok,i ;
#define OB(BIT) respnt->BIT |= opt->BIT


	if (opt->PntType == V4DPI_PntType_TagVal)
	 { tv = (struct V4DPI__TagVal *)&opt->Value ;
	   CPH(respnt,opt) ;
	   tv1 = (struct V4DPI__TagVal *)&respnt->Value ; tv1->TagVal = tv->TagVal ;
	   if (opt->ForceEval)			/* Got tag? ? */
	    { b = (char *)&tv1->TagPt - (char *)respnt ;
	      return(b) ;
	    } ;
	   b = v4eval_OptPoint(ctx,&tv->TagPt,&tv1->TagPt,maxbytes-V4DPI_PointHdr_Bytes,odl) ;
	   if (b < 0) return(-1) ;
	   respnt->Bytes = (char *)&tv1->TagPt + b - (char *)respnt ;
//printf("****tag bytes=%d, tag=%d\n",respnt->Bytes,tv->TagVal) ;
	   return(respnt->Bytes) ;
	 } ;
	if (opt->PntType == V4DPI_PntType_List)
	 { INITLP(respnt,lp,opt->Dim) ;
	   lp1 = v4im_VerifyList(NULL,ctx,opt,0) ;
//v_Msg(ctx,NULL,"@Opt list: %1P",opt) ; printf("%s\n",ctx->ErrorMsg) ;
	   if (lp1 == NULL) { memcpy(respnt,opt,opt->Bytes) ; return(opt->Bytes) ; } ;
	   for(i=1;v4l_ListPoint_Value(ctx,lp1,i,&listbuf)>0;i++)
	    { v4eval_OptPoint(ctx,&listbuf,&rpt,sizeof rpt,odl) ;
//v_Msg(ctx,NULL,"@  %1P (%2i) -> %3P (%4i)",&listbuf,listbuf.Bytes,&rpt,rpt.Bytes) ; printf("%s\n",ctx->ErrorMsg) ;
	      if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&rpt,0)) return(-1) ;
	    } ;
	   ENDLP(respnt,lp) ;
//printf("  respnt->Bytes=%d, Dim=%d, PntType=%d\n",respnt->Bytes,respnt->Dim,respnt->PntType) ;
//printf("  lp->Bytes=%d, ListType=%d, BytesPerEntry=%d\n",lp->Bytes,lp->ListType,lp->BytesPerEntry) ;
//v_Msg(ctx,NULL,"@  result: %1P",respnt) ; printf("%s\n",ctx->ErrorMsg) ;
	   return(respnt->Bytes) ;
	 } ;
	if (ISQUOTED(opt))
	 { int res = v4eval_OptPoint(ctx,UNQUOTEPTR(opt),respnt,maxbytes,odl) ;
	   QUOTE(respnt) ;
	   return(res) ;
	 } ;
	if (opt->PntType != V4DPI_PntType_Isct)
	 { if (opt->Bytes >= maxbytes) return(-1) ;
	   memcpy(respnt,opt,opt->Bytes) ; return(opt->Bytes) ;
	 } ;
/*	Got an isct - ok to optimize only if form of x.y */
	ok = (opt->Grouping == 2) ;
	if (ok)
	 { tpt = ISCT1STPNT(opt) ;
	   ok = (tpt->PntType == V4DPI_PntType_Special && tpt->Grouping == V4DPI_Grouping_Current) ;
	   if (ok && odl != NULL)
	    { for(i=odl->Count;i>=0;i--) { if (tpt->Dim == odl->Dims[i]) break ; } ;
	      if (i < 0) ok = FALSE ;
	    } ;
	 } ;
	if (opt->ForceEval) ok = FALSE ;		/* If `[xxx] then don't optimize */
	if (ok)
	 { memcpy(&pt,opt,opt->Bytes) ;			/* Copy x.y into pt and convert x* to x:{sample} */
	   tpt = ISCT1STPNT(&pt) ; tpt->Grouping = V4DPI_Grouping_Sample ;
	   pt.Continued = FALSE ;			/* Don't want to continue */
	   xpt = v4dpi_IsctEval(&rpt,&pt,ctx,V4DPI_EM_EvalQuote+V4DPI_EM_NoNest+V4DPI_EM_NoIsctFail+V4DPI_EM_NoRegisterErr,NULL,NULL) ;
	   if (xpt == NULL) { memcpy(respnt,opt,opt->Bytes) ; return(opt->Bytes) ; }
	   if (xpt->PntType != V4DPI_PntType_Isct) { memcpy(respnt,opt,opt->Bytes) ; return(opt->Bytes) ; } ;
	   if (xpt->Bytes >= maxbytes) return(-1) ;
	   memcpy(respnt,xpt,xpt->Bytes) ;
//	   OB(Quoted) ;
	   OB(TraceEval) ; OB(ForceEval) ; OB(Continued) ;
	   if (opt->Continued)						/* Have to copy continued stuff? */
	    { for(tpt=ISCT1STPNT(opt),i=0;i<opt->Grouping;i++)		/* Position to end of "main" point */
	       { ADVPNT(tpt) ; } ;
	      b = (char *)tpt - (char *)opt ;				/* b = number of bytes used in main point */
	      b = opt->Bytes - b ;					/* b = number of bytes in continuation */
	      if (b >= maxbytes) return(-1) ;
	      memcpy((char *)respnt + xpt->Bytes,tpt,b) ;		/* Copy continuation point(s) */
	      maxbytes -= b ; respnt->Bytes += b ;			/* Update everything */
	    } ;
	   return(respnt->Bytes) ;					/* Return bytes of optimized point */
	 } ;
/*	Not OK to optimize - enumerate & optimize each component point */
	CPH(respnt,opt) ; respnt->Value.Isct.vis = opt->Value.Isct.vis ;
	respnt1 = ISCT1STPNT(respnt) ;
	for(tpt=ISCT1STPNT(opt),i=0;i<opt->Grouping;i++)
	 { b = v4eval_OptPoint(ctx,tpt,respnt1,maxbytes,odl) ;
	   if (b < 0) return(-1) ;
	   ADVPNT(respnt1) ; maxbytes -= b ;
	   ADVPNT(tpt) ;
	 } ;
	if (opt->Continued)				/* Have to copy continued stuff? */
	 { b = (char *)tpt - (char *)opt ;		/* b = number of bytes used */
	   b = opt->Bytes - b ;
	   if (b >= maxbytes) return(-1) ;
	   memcpy(respnt1,tpt,b) ; ADVPNT(respnt1) ; maxbytes -= b ;
	 } ;
	respnt->Bytes = (char *)respnt1 - (char *)respnt ;
	return(respnt->Bytes) ;
}


/*	N E W   M A C R O   R O U T I N E S	*/

DCLSPINLOCK macLock = UNUSEDSPINLOCKVAL ;			/* Spin lock element for macros */

/*	v4eval_ParseMacroDef - Parse macro definition & return pointer to macro-entry structure
	NOTE: Assumes tcb points to begin of stream of form: macroname(arg1, arg2, ...) macro-body ... }/macroname	*/

struct V4LEX__MacroEntry *v4eval_ParseMacroDef(ctx,tcb,trace)
  struct V4C__Context *ctx ;
  struct V4LEX__TknCtrlBlk *tcb ;		/* Token Control Block */
  VTRACE trace ;
{
  struct V4LEX__MacroDirectory *vlmd ;
  struct V4LEX__MacroEntry vlme, *vlmePtr ;
  struct V4LEX__MacroBody *vlmb ;
  UCCHAR *mb, *eomb ;
  COUNTER maxBody, charCount, len ; UCCHAR firstChar ; LOGICAL getArgs ;
#define TKN(flags) { v4lex_NextTkn(tcb,flags) ; if (tcb->type == V4LEX_TknType_Error) goto parse_err ; }

	if (gpi->vlmd == NULL)
	 { gpi->vlmd = v4eval_GetMacroDir(ctx) ;
	   if (gpi->vlmd == NULL)
	    { gpi->vlmd = (vlmd = (struct V4LEX__MacroDirectory *)v4mm_AllocChunk(sizeof *vlmd,FALSE)) ;
	      vlmd->isDirty = TRUE ; vlmd->isSorted = FALSE ; vlmd->maxEntries = V4LEX_MacDir_MaxInit ; vlmd->numEntries = 0 ;
	    } ;
	 } ;
	vlmd = gpi->vlmd ;
	vlme.argCount = 0 ; ZUS(vlme.macNameNC) ; vlme.udaArgX = UNUSED ; vlme.btArgX = UNUSED ; vlme.refCount = 0 ;
	firstChar = UCEOS ;
	TKN(0) if (tcb->type != V4LEX_TknType_Keyword) { v_Msg(NULL,ctx->ErrorMsgAux,"MacroName",tcb->type) ; goto fail ; } ;
	if (UCstrlen(tcb->UCkeyword) > V4LEX_Tkn_KeywordMax) { v_Msg(NULL,ctx->ErrorMsgAux,"MacroNameSize",tcb->type,tcb->UCkeyword) ; goto fail ; } ;
	UCstrcpy(vlme.macNameNC,tcb->UCkeyword) ;
/*	Remember that we are parsing macro so that if we hit EOF we can generate an error */
	tcb->cpmStartLine = tcb->ilvl[tcb->ilx].current_line ; UCstrcpy(tcb->cpmName,tcb->UCkeyword) ;
/*	If parsing V3/MIDAS style 'macro' definitions (no '(argument list)') then accept EOL here */
	if (gpi->inMIDAS)
	 { TKN(gpi->inMIDAS ? V4LEX_Option_RetEOL : 0) ;
	   if (tcb->opcode != V_OpCode_EOX)
	    { v_Msg(NULL,ctx->ErrorMsgAux,"MacroArgList",tcb->type) ; goto fail ; } ;
	   getArgs = FALSE ;
	 } else
	 { TKN(0) if (tcb->opcode != V_OpCode_LParen && tcb->opcode != V_OpCode_NCLParen) { v_Msg(NULL,ctx->ErrorMsgAux,"MacroArgList",tcb->type) ; goto fail ; } ;
	   getArgs = TRUE ;
	 } ;
	for(;getArgs;)
	 { TKN(0)
	   if (tcb->opcode == V_OpCode_RParen) break ;
	   if (vlme.argCount > 0 && tcb->opcode == V_OpCode_Comma) { TKN(0) ; } ;
	   if (tcb->opcode == V_OpCode_Star || tcb->opcode == V_OpCode_NCStar)
	    { if (vlme.udaArgX != UNUSED) { v_Msg(ctx,ctx->ErrorMsgAux,"MacroOnlyOnce",tcb->type) ; goto fail ; } ;
	      UCstrcpy(vlme.arg[vlme.argCount].argName,V4LEX_MACRO_UDALIST) ;	/* Add argument udaList (undefined-arguments-list) */
	      vlme.udaArgX = vlme.argCount ;
	      TKN(0) ; vlme.argCount++ ;
	      if (tcb->opcode == V_OpCode_RParen) break ;
	      continue ;
	    } ;
	   if (tcb->opcode == V_OpCode_DotDotDot)
	    { if (vlme.btArgX != UNUSED) { v_Msg(ctx,ctx->ErrorMsgAux,"MacroOnlyOnce",tcb->type) ; goto fail ; } ;
	      UCstrcpy(vlme.arg[vlme.argCount].argName,V4LEX_MACRO_BTARG) ;	/* Add argument btArg (big-text argument) */
	      vlme.btArgX = vlme.argCount ;
	      TKN(0) ; vlme.argCount++ ;
	      if (tcb->opcode == V_OpCode_RParen) break ;
	      continue ;
	    } ;
	   if (tcb->opcode != V_OpCode_Keyword) { v_Msg(NULL,ctx->ErrorMsgAux,"MacroArgName",tcb->type) ; goto fail ; } ;
/*	   Handle macro argument name */
	   if (UCstrlen(tcb->UCkeyword) > V4LEX_Tkn_KeywordMax) { v_Msg(NULL,ctx->ErrorMsgAux,"MacroNameSize",tcb->type,tcb->UCkeyword) ; goto fail ; } ;
	   UCstrcpy(vlme.arg[vlme.argCount].argName,tcb->UCkeyword) ;
	   if (vlme.argCount == 0) { firstChar = tcb->UCkeyword[0] ; }	/* Keep track if we have common first character */
	    else { if (firstChar != tcb->UCkeyword[0]) firstChar = UCEOS ; } ;
	   vlme.argCount++ ;
	 } ;
/*	Now parse remainder of macro body (subtract 1 because we have already allocated for one argument in structure definition) */
	vlme.bodyOffset = ALIGN((sizeof vlme.arg[0]) * (vlme.argCount - 1)) ;
	vlmb = (struct V4LEX__MacroBody *)&vlme.macData[vlme.bodyOffset] ;
	maxBody = (sizeof(vlme.macData) - vlme.bodyOffset - 16) / sizeof(UCCHAR) ;	/* Max number of characters in body */
	mb = vlmb->macBody ; ZUS(mb) ; eomb = NULL ;
	for(charCount=0;;)
	 {
	   v4lex_NextTkn(tcb,V4LEX_Option_ForceAsIsULC+V4LEX_Option_RetEOL) ;
	   if (tcb->type == V4LEX_TknType_EOL)
	    UCstrcpy(tcb->UCstring,UClit("\032")) ;
	   if (tcb->opcode == V_OpCode_EOF) { v_Msg(NULL,ctx->ErrorMsgAux,"MacroUnexpEOF",vlme.macNameNC) ; goto fail ; } ;
	   if (eomb != NULL)		/* Did we just see possible end-of-macro "}/..." */
	    { if (UCstrcmpIC(tcb->UCstring,vlme.macNameNC) == 0)
	       { mb = eomb ; break ; }
	       else { eomb = NULL ; }
	    } ;
	   if (tcb->opcode == V_OpCode_RBraceSlash) eomb = mb ;
	   charCount += (len = UCstrlen(tcb->UCstring)) ;
	   if (charCount >= maxBody) { v_Msg(NULL,ctx->ErrorMsgAux,"MacroTooBig",tcb->type,vlme.macNameNC,maxBody) ; goto fail ; } ;
	   UCstrcat(mb,tcb->UCstring) ; 
	   if(vuc_IsWSpace(*tcb->ilvl[tcb->ilx].input_ptr))		/* Only append space if terminated with white space */
	    { UCstrcat(mb,UClit(" ")) ; len++ ; charCount++ ; } ;
	   mb += len ;
	 } ;
	*mb = UCEOS ;
	vlme.bytes = ALIGN(((char *)mb - (char *)&vlme) + 1) ;
/*	Save the entry in real memory (not stack memory) */
	vlmePtr = (struct V4LEX__MacroEntry *)v4mm_AllocChunk(vlme.bytes,FALSE) ;
	memcpy(vlmePtr,&vlme,vlme.bytes) ;
/*	Update macro directory */
	if (vlmd->numEntries >= vlmd->maxEntries)		/* Exceeded max number of macros - have to expand directory */
	 { vlmd = (struct V4LEX__MacroDirectory *)realloc(vlmd,SIZEOFVLMD(vlmd,vlmd->maxEntries+50)) ;
	   gpi->vlmd = vlmd ; vlmd->maxEntries += 50 ;
	 } ;
	UCstrcpy(vlmd->macro[vlmd->numEntries].macNameNC,vlme.macNameNC) ;
	vlmd->macro[vlmd->numEntries].macType = (gpi->inMIDAS ? V4LEX_MACTYPE_MIDAS : V4LEX_MACTYPE_Macro) ;
	vlmd->macro[vlmd->numEntries].vlme = vlmePtr ;
/*	Now write it out to an area */
	if (TRUE)
	 {
	   vlmd->macro[vlmd->numEntries].segId = v4seg_PutSegments(ctx,vlmePtr,vlmePtr->bytes,FALSE,FALSE) ;
	   if (vlmd->macro[vlmd->numEntries].segId == UNUSED) goto fail ;
	 } ;
	vlmd->numEntries ++ ; vlmd->isDirty = TRUE ;
	if (trace & V4TRACE_Macros)
	 { v_Msg(ctx,UCTBUF1,"@*Saved macro %1U\n",vlme.macNameNC) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
/*	Remember that we are no longer parsing macro */
	tcb->cpmStartLine = 0 ; ZUS(tcb->cpmName) ;
	return(vlmePtr) ;
parse_err:
	v_Msg(NULL,ctx->ErrorMsgAux,"MacroPrsErr",tcb->type,vlme.macNameNC,tcb->UCstring) ; goto fail ;

fail:	
/*	Remember that we are no longer parsing macro */
	tcb->cpmStartLine = 0 ; ZUS(tcb->cpmName) ;
	return(NULL) ;
}
/*	v4eval_SaveMacroBodyOnly - Saves Macro Text Entry		*/

struct V4LEX__MacroEntry *v4eval_SaveMacroBodyOnly(ctx,macName,macroBody,macType,intmodx)
  struct V4C__Context *ctx ;
  UCCHAR *macName, *macroBody ;
  ETYPE macType ;
  INTMODX intmodx ;
{
  struct V4LEX__MacroDirectory *vlmd ;
  struct V4LEX__MacroEntry vlme, *vlmePtr ;
  struct V4LEX__MacroBody *vlmb ;
  COUNTER maxBody, len ;

	if (gpi->vlmd == NULL)
	 { gpi->vlmd = v4eval_GetMacroDir(ctx) ;
	   if (gpi->vlmd == NULL)
	    { gpi->vlmd = (vlmd = (struct V4LEX__MacroDirectory *)v4mm_AllocChunk(sizeof *vlmd,FALSE)) ;
	      vlmd->isDirty = TRUE ; vlmd->isSorted = FALSE ; vlmd->maxEntries = V4LEX_MacDir_MaxInit ; vlmd->numEntries = 0 ;
	    } ;
	 } ;
	vlmd = gpi->vlmd ;
/*	No arguments in this entry */
	vlme.argCount = 0 ; vlme.udaArgX = UNUSED ; vlme.btArgX = UNUSED ;
	UCcnvupper(vlme.macNameNC,macName,UCsizeof(vlme.macNameNC)) ;

	vlme.bodyOffset = 0 ;
	vlmb = (struct V4LEX__MacroBody *)&vlme.macData[vlme.bodyOffset] ;
	maxBody = (V4AreaAgg_UsableBytes - vlme.bodyOffset) / sizeof(UCCHAR) ;	/* Max number of characters in body */
	len = UCstrlen(macroBody) ;
	if (len > maxBody) { v_Msg(ctx,ctx->ErrorMsgAux,"MacroTooBig1",intmodx,macName,maxBody) ; return(NULL) ; } ;
	UCstrcpy(vlmb->macBody,macroBody) ;
	vlme.bytes = ALIGN(((char *)&vlmb->macBody[len+1] - (char *)&vlme) + 1) ;
/*	Save the entry in real memory (not stack memory) */
	vlmePtr = (struct V4LEX__MacroEntry *)v4mm_AllocChunk(vlme.bytes,FALSE) ;
	memcpy(vlmePtr,&vlme,vlme.bytes) ;
/*	Update macro directory */
	if (vlmd->numEntries >= vlmd->maxEntries)		/* Exceeded max number of macros - have to expand directory */
	if (vlmd->numEntries >= vlmd->maxEntries)		/* Exceeded max number of macros - have to expand directory */
	 { vlmd = (struct V4LEX__MacroDirectory *)realloc(vlmd,SIZEOFVLMD(vlmd,vlmd->maxEntries+50)) ;
	   gpi->vlmd = vlmd ; vlmd->maxEntries += 50 ;
	 } ;
	UCstrcpy(vlmd->macro[vlmd->numEntries].macNameNC,vlme.macNameNC) ;
	vlmd->macro[vlmd->numEntries].macType = macType ;
	vlmd->macro[vlmd->numEntries].vlme = vlmePtr ;
/*	Now write it out to an area */
	if (TRUE)
	 {
	   vlmd->macro[vlmd->numEntries].segId = v4seg_PutSegments(ctx,vlmePtr,vlmePtr->bytes,FALSE,FALSE) ;
	   if (vlmd->macro[vlmd->numEntries].segId == UNUSED) return(NULL) ;
	 } ;
	vlmd->numEntries ++ ; vlmd->isDirty = TRUE ;
	return(vlmePtr) ;
}

/*	v4lex_ParseMacroCall - Parses macro call - assumes tcb points to first argument and that macro ends with closing ')' */


struct V4LEX__MacroCallArgs *v4lex_ParseMacroCall(ctx,tcb,vlme)
  struct V4C__Context *ctx ;
  struct V4LEX__TknCtrlBlk *tcb ;		/* Token Control Block */
  struct V4LEX__MacroEntry *vlme ;
{
  struct V4LEX__MacroCallArgs *vlmca ;
  UCCHAR argName[V4LEX_Tkn_KeywordMax+5], argVal[4096], udaArg[256], udaListVal[V4LEX_Tkn_StringMax] ;
  INDEX ax, startingLine ; LOGICAL keepGoing,gotVal ;

#define SAVEARG(VALUE) { vlmca->arg[ax].argVX = vlmca->abIndex ; UCstrcpy(&vlmca->argBuf[vlmca->abIndex],VALUE) ; vlmca->abIndex += (UCstrlen(VALUE) + 1) ; }
#define GRABARG(VALUE) \
 if (UCstrlen(VALUE) + UCstrlen(argVal) >= sizeof argVal) { v_Msg(ctx,ctx->ErrorMsgAux,"MacroArgLen",tcb->type,UCstrlen(VALUE) + UCstrlen(argVal),sizeof argVal) ; return(NULL) ; } ; \
 UCstrcat(argVal,VALUE) ;

	if (tcb->ilvl[tcb->ilx].vlmca == NULL) tcb->ilvl[tcb->ilx].vlmca = (struct V4LEX__MacroCallArgs *)v4mm_AllocChunk(sizeof *vlmca,FALSE) ;
	vlmca = tcb->ilvl[tcb->ilx].vlmca ; vlmca->abIndex = 0 ;
	startingLine = tcb->ilvl[tcb->ilx].current_line ;
/*	Start parsing the arguments */
	for (ax=0;ax<vlme->argCount;ax++)
	 { vlmca->arg[ax].argVX = UNUSED ;
	   UCstrcpy(vlmca->arg[ax].argName,vlme->arg[ax].argName) ;
	   if (ax == 0) { vlmca->firstChar = vlme->arg[ax].argName[0] ; }
	    else { if (vlmca->firstChar != vlme->arg[ax].argName[0] && ax != vlme->udaArgX && ax != vlme->btArgX) vlmca->firstChar = UCEOS ; } ;
	 } ;
	vlmca->argCount = vlme->argCount ;
	ZUS(udaListVal) ;
	for(ax=0,keepGoing=TRUE;keepGoing;ax++)		/* ax = Current argument index (may be updated below) */
	 { TKN(V4LEX_Option_ForceAsIsULC+V4LEX_Option_ExpandArgs)
	   ZUS(udaArg) ; ZUS(argVal) ; gotVal = TRUE ;
/*	   We may have a number of things: argument name, argument value, or begin of < ... > sequence */
	   switch (tcb->opcode)
	    { default:
		v_Msg(ctx,ctx->ErrorMsgAux,"MacroCallSyn",tcb->type,7,vlme->macNameNC) ; return(NULL) ;
	      case V_OpCode_Langle:
	      case V_OpCode_LangleRangle:
	      case V_OpCode_LangleLangle:
	      case V_OpCode_Numeric:
	      case V_OpCode_DQString:
	      case V_OpCode_QString:		break ;
	      case V_OpCode_DotDotDot:
		if (vlme->btArgX == UNUSED) { v_Msg(ctx,ctx->ErrorMsgAux,"MacroCallSyn",tcb->type,1,vlme->macNameNC) ; return(NULL) ; } ;
//		tcb->need_input_line = TRUE ;
		{ P pnt ;
		  struct V4DPI__DimInfo di ;
/*		  Ignore remainder of current line if it is a comment, otherwise treat it as BigText parsing argument */
		  for(;vuc_IsWSpace(*tcb->ilvl[tcb->ilx].input_ptr);++tcb->ilvl[tcb->ilx].input_ptr) { } ;
		  if (tcb->ilvl[tcb->ilx].input_ptr[0] == UClit('/') && (tcb->ilvl[tcb->ilx].input_ptr[1] == UClit('*') || tcb->ilvl[tcb->ilx].input_ptr[1] == UClit('/')))
		   { tcb->ilvl[tcb->ilx].input_ptr = UClit("") ; } ;
		  UCsprintf(argVal,UCsizeof(argVal),UClit("<%s %s>\n"),vlme->macNameNC,tcb->ilvl[tcb->ilx].input_ptr) ;
		  tcb->ilvl[tcb->ilx].input_ptr = argVal ;
/*		  Set di to dummy BigText point type (this is ugly but obviates need for kernel BigText dimension */
		  memset(&di,0,sizeof di) ; di.PointType = V4DPI_PntType_BigText ; di.DimId = UNUSED ;
/*		  Parse the bigtext value then save as '#nnn' where nnn is internal bigtext key number OR as string literal (if small enough) */
		  if (!v4dpi_PointAccept(&pnt,&di,tcb,ctx,V4DPI_PointParse_RetFalse))
		   { UCstrcpy(UCTBUF1,ctx->ErrorMsgAux) ;
		     v_Msg(ctx,ctx->ErrorMsgAux,"MacroBTPrsErr",startingLine,UCTBUF1) ;
		     return(NULL) ;
		   } ;
		  if (pnt.Value.IntVal != -1) { UCsprintf(argVal,UCsizeof(argVal),UClit("#%d"),pnt.Value.IntVal) ; }
		   else { v_StringLit((UCCHAR *)&pnt.Value.Int2Val[1],argVal,UCsizeof(argVal)-5,UClit('"'),UCEOS) ;
			} ;
		  ax = vlme->btArgX ; SAVEARG(argVal) ;
		  tcb->opcode = V_OpCode_RParen ;		/* Set this to ')' so we think we are at the end of the macro call */
		  keepGoing = FALSE ; continue ;
		} ;
		break ;
	      case V_OpCode_RParen:
		if (ax == 0) keepGoing = FALSE ;
		break ;
	      case V_OpCode_Keyword:
/*		Don't know what this is until we look at next token */
		UCstrcpy(argName,tcb->UCstring) ;
		TKN(V4LEX_Option_ForceAsIsULC+V4LEX_Option_ExpandArgs)
		switch (tcb->opcode)
		 { default:
			v_Msg(ctx,ctx->ErrorMsgAux,"MacroCallSyn",tcb->type,2,vlme->macNameNC) ; return(NULL) ;
		   case V_OpCode_Equal:
			for(ax=0;ax<vlme->argCount;ax++) { if (UCstrcmpIC(argName,vlme->arg[ax].argName) == 0) break ; } ;
			if (ax >= vlme->argCount)
			 { if (vlmca->firstChar != UCEOS)		/* Argument have common first character, add that to argName and try again */
			    { LENMAX len = UCstrlen(argName) ;
			      for(;len>=0;len--) { argName[len+1] = argName[len] ; } ; argName[0] = vlmca->firstChar ;
			      for(ax=0;ax<vlme->argCount;ax++) { if (UCstrcmpIC(argName,vlme->arg[ax].argName) == 0) break ; } ;
			    } ;
			   if (ax >= vlme->argCount)
			    { if (vlme->udaArgX  == UNUSED) { v_Msg(ctx,ctx->ErrorMsgAux,"MacroArgNotFnd",tcb->type,argName,vlme->macNameNC) ; return(NULL) ; } ;
			      UCstrcpy(udaArg,(vlmca->firstChar != UCEOS ? &argName[1] : argName)) ;
			      ax = vlme->udaArgX ;
			    } ;
			 } ;
			TKN(V4LEX_Option_ForceAsIsULC+V4LEX_Option_ExpandArgs) ; break ;
		   case V_OpCode_RParen:
			SAVEARG(argName) ;
			keepGoing = FALSE ; continue ;
		   case V_OpCode_Dot:
		   case V_OpCode_Colon:
			if (vlme->udaArgX  == UNUSED) { v_Msg(ctx,ctx->ErrorMsgAux,"MacroArgNotFnd",tcb->type,argName,vlme->macNameNC) ; return(NULL) ; } ;
			ax = vlme->udaArgX ;
			UCstrcat(udaArg,UClit("(")) ; UCstrcat(udaArg,argName) ;
			for(;;)
			 {
			   if (tcb->opcode == V_OpCode_Dot)
			    { UCstrcat(udaArg,UClit(" ")) ; TKN(0) ; if (tcb->opcode != V_OpCode_Keyword) { v_Msg(ctx,ctx->ErrorMsgAux,"MacroCallSyn",tcb->type,3,vlme->macNameNC) ; return(NULL) ; } ;
			    } else if (tcb->opcode == V_OpCode_Colon)
			    { UCstrcat(udaArg,UClit(":")) ;
			      TKN(V4LEX_Option_ForceAsIsULC+V4LEX_Option_ExpandArgs) ;
			      UCstrcat(udaArg,tcb->UCstring) ;
			      TKN(0) ;
			      if (tcb->opcode == V_OpCode_Equal) break ;
			      if (tcb->opcode != V_OpCode_Dot) { v_Msg(ctx,ctx->ErrorMsgAux,"MacroCallSyn",tcb->type,4,vlme->macNameNC) ; return(NULL) ; } ;
			      UCstrcat(udaArg,UClit(" ")) ; TKN(0) ; if (tcb->opcode != V_OpCode_Keyword) break ;
			    } else
			    { v_Msg(ctx,ctx->ErrorMsgAux,"MacroCallSyn",tcb->type,5,vlme->macNameNC) ; return(NULL) ; } ;
			   UCstrcat(udaArg,tcb->UCkeyword) ;
			   TKN(0) ; if (tcb->opcode == V_OpCode_Equal) break ;
			 } ;
			UCstrcat(udaArg,UClit(")")) ; 
			TKN(V4LEX_Option_ForceAsIsULC+V4LEX_Option_ExpandArgs) ; break ;
		   case V_OpCode_Comma:
			SAVEARG(argName) ;
			continue ;		/* Continue with next argument */
		 } ;
		break ;
	    } ;
	   if (ax >= vlme->argCount) break ;
/*	   At this point we should have argument name (or at least ax set correctly)
	     and we should be ready to parse the value					*/
	   switch (tcb->opcode)
	    { default:
	      case V_OpCode_Keyword:
	      case V_OpCode_QString:
	      case V_OpCode_DQString:
		GRABARG(tcb->UCstring) ; break ;
	      case V_OpCode_LangleRangle:
		gotVal = FALSE ; break ;					/* <> is empty argument place holder */
	      case V_OpCode_Langle:
	      case V_OpCode_LangleLangle:
	      { UCCHAR fnStart[V_FileName_Max] ; INDEX lnStart,ilxStart ;	/* Remember where we are in case we don't ever see ">" or ">>" */
	        INDEX endOp = (tcb->opcode == V_OpCode_Langle ? V_OpCode_Rangle : V_OpCode_RangleRangle) ;	/* If we start with '<' then end with '>', if start with '<<' then end with '>>' */
		ilxStart = tcb->ilx ; lnStart = tcb->ilvl[tcb->ilx].current_line ; UCstrcpy(fnStart,tcb->ilvl[tcb->ilx].file_name) ; 
		TKN(V4LEX_Option_ForceAsIsULC+V4LEX_Option_ExpandArgs) ;
		for(;;)
		 { GRABARG(tcb->UCstring) ;
		   if(vuc_IsWSpace(*tcb->ilvl[tcb->ilx].input_ptr) || (*tcb->ilvl[tcb->ilx].input_ptr < 26))
		    { GRABARG(UClit(" ")) ; } ;	/* Only append space if terminated with white space or control character (ex: end-of-line) */
		   TKN(V4LEX_Option_ForceAsIsULC+V4LEX_Option_ExpandArgs) ;
		   if (tcb->ilx != ilxStart)
		    { v_Msg(ctx,ctx->ErrorMsgAux,"MacroLAngle",vlme->macNameNC,vlme->arg[ax].argName,lnStart,fnStart) ; return(NULL) ; } ;
		   if (tcb->opcode == endOp) break ;
		 } ;
	      }
		vlmca->abIndex++ ;				/* Regain the UCEOS */
		break ;
	    } ;
	   if (gotVal)
	    { if (ax != vlme->udaArgX) { SAVEARG(argVal) ; }
	       else { if (UCempty(udaListVal)) { UCstrcat(udaListVal,UClit("(")) ; } ;
		      UCstrcat(udaListVal,UClit("(")) ; UCstrcat(udaListVal,udaArg) ; UCstrcat(udaListVal,UClit(" ")) ; UCstrcat(udaListVal,argVal) ; UCstrcat(udaListVal,UClit(")")) ;
		    } ;
	    } ;
/*	   Next token best be "," or ")" */
	   TKN(0)
	   if (tcb->opcode == V_OpCode_Comma) continue ;
	   if (tcb->opcode == V_OpCode_RParen) break ;
	   v_Msg(ctx,ctx->ErrorMsgAux,"MacroCallSyn",tcb->type,6,vlme->macNameNC) ; return(NULL) ;
	 } ;
/*	If we got a udaListVal then save it */
	if (UCnotempty(udaListVal))
	 { ax = vlme->udaArgX ; UCstrcat(udaListVal,UClit(")")) ; SAVEARG(udaListVal) ; } ;
/*	If current opcode ")" then we all done ? */
	if (tcb->opcode == V_OpCode_RParen) return(vlmca) ;
	v_Msg(ctx,ctx->ErrorMsgAux,"MacroTooMnyArgs",tcb->type,vlme->argCount,vlme->macNameNC) ; return(NULL) ;
parse_err:
	v_Msg(NULL,ctx->ErrorMsgAux,"MacroPrsErr",tcb->type,vlme->macNameNC,tcb->UCstring) ; return(NULL) ;
}

LOGICAL v4lex_InvokeMacroCall(ctx,tcb,vlme,vlmca)
  struct V4C__Context *ctx ;
  struct V4LEX__TknCtrlBlk *tcb ;		/* Token Control Block */
  struct V4LEX__MacroEntry *vlme ;
  struct V4LEX__MacroCallArgs *vlmca ;
{
  struct V4LEX__MacroBody *vlmb ;
  INDEX ax ;

//if (UCstrcmp(vlme->macNameNC,UClit("HFE")) == 0)
//printf("moo\n") ;
	vlme->refCount++ ;			/* Increment call/reference counter for this macro */
	vlmb = (struct V4LEX__MacroBody *)&vlme->macData[vlme->bodyOffset] ;
	v4lex_NestInput(tcb,NULL,vlmb->macBody,V4LEX_InpMode_StringML) ;
	for (ax=0;ax<vlmca->argCount;ax++)
	 { UCstrcpy(tcb->ilvl[tcb->ilx].macarg[ax].name,vlmca->arg[ax].argName) ;
	   tcb->ilvl[tcb->ilx].macarg[ax].value = (vlmca->arg[ax].argVX == UNUSED ? NULL : &vlmca->argBuf[vlmca->arg[ax].argVX]) ;
	 } ;
	tcb->ilvl[tcb->ilx].BoundMacroArgs = TRUE ; tcb->ilvl[tcb->ilx].vlt = NULL ;
	tcb->ilvl[tcb->ilx].HaveMacroArgNames = (vlmca->firstChar == UCEOS ? TRUE : vlmca->firstChar) ;
	return(TRUE) ;
}

LOGICAL v4eval_SaveMacroDir(ctx)
  struct V4C__Context *ctx ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4LEX__MacDirBoot vlmdb ;
  INDEX rx, areax ; LENMAX bytes ;

	if (gpi->vlmd == NULL) return(TRUE) ;		/* Nothing to save */
	if (!gpi->vlmd->isDirty) return(TRUE) ;		/* Already saved */
	for(rx=gpi->HighHNum;rx>=gpi->LowHNum;rx--)
	 { if (rx == V4DPI_WorkRelHNum || gpi->RelH[rx].aid == UNUSED) continue ;
/*	   Is Area open for update? */
	   if ((gpi->RelH[rx].pcb->AccessMode & (V4IS_PCB_AM_Insert | V4IS_PCB_AM_Update)) != 0) break ;
	 } ;
/*	If rx too small then did not find update-able are, return FALSE */
	if (rx < gpi->LowHNum) return(FALSE) ;
	gpi->vlmd->isDirty = FALSE ;
	FINDAREAINPROCESS(areax,gpi->RelH[rx].aid) ;
	vlmdb.kp.fld.KeyType = V4IS_KeyType_V4 ; vlmdb.kp.fld.KeyMode = V4IS_KeyMode_Int ; vlmdb.kp.fld.AuxVal = V4IS_SubType_MacroBoot ;
	vlmdb.kp.fld.Bytes = V4IS_IntKey_Bytes ; vlmdb.mustBeZero = 0 ;
	bytes = SIZEOFVLMD(gpi->vlmd,gpi->vlmd->maxEntries) ;
	vlmdb.segId = v4seg_PutSegments(ctx,gpi->vlmd,bytes,FALSE,FALSE) ;
	GRABAREAMTLOCK(areax) ;
	gpi->RelH[rx].pcb->PutBufPtr = (BYTE *)&vlmdb ; gpi->RelH[rx].pcb->PutBufLen = sizeof vlmdb ;
	gpi->RelH[rx].pcb->KeyPtr = (struct V4IS__Key *)&vlmdb ;
	if (!v4is_Put(gpi->RelH[rx].pcb,ctx->ErrorMsgAux)) { FREEAREAMTLOCK(areax) ; return(FALSE) ; } ;
	FREEAREAMTLOCK(areax) ;
	return(TRUE) ;
}

struct V4LEX__MacroEntry *v4eval_GetMacroEntry(ctx,macName,macType)
  struct V4C__Context *ctx ;
  UCCHAR *macName ;
  ETYPE macType ;
{
  struct V4LEX__MacroDirectory *vlmd ;
  struct V4LEX__MacroEntry *vlme ;
  INDEX mx ; LENMAX bytes ; UCCHAR macNameNC[V4LEX_Tkn_KeywordMax+1] ;

	if (gpi->vlmd == NULL)
	 { gpi->vlmd = v4eval_GetMacroDir(ctx) ; if (gpi->vlmd == NULL) goto fail ;
	 } ; vlmd = gpi->vlmd ;
/*	Lookup name */
	UCcnvupper(macNameNC,macName,UCsizeof(macNameNC)) ;
	for(;!GETSPINLOCK(&macLock);) {} ;
	if (vlmd->isSorted)
	 { mx = UNUSED ;
	 } else
	 { for(mx=0;mx<vlmd->numEntries;mx++)
	    { if (UCstrcmp(vlmd->macro[mx].macNameNC,macNameNC) == 0 && vlmd->macro[mx].macType == macType) break ; } ;
	   if (mx >= vlmd->numEntries)
	    mx = UNUSED ;
	 } ;
	if (mx == UNUSED) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdMacroNIArea",macName) ; goto fail ; } ;
/*	Found entry, have we already loaded this macro ? */
	if (vlmd->macro[mx].vlme != NULL) goto success ;
/*	Not yet - grab it */
	vlme = (struct V4LEX__MacroEntry *)v4seg_GetSegments(ctx,vlmd->macro[mx].segId,&bytes,FALSE,UNUSED) ;
	if (vlme == NULL)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"SegNotFound",vlmd->macro[mx].segId) ; goto fail ; } ;
	vlmd->macro[mx].vlme = (struct V4LEX__MacroEntry *)v4mm_AllocChunk(vlme->bytes,FALSE) ;
	memcpy(vlmd->macro[mx].vlme,vlme,vlme->bytes) ;
success:
	RLSSPINLOCK(macLock) ;
	return(vlmd->macro[mx].vlme) ;
fail:
	RLSSPINLOCK(macLock) ; return(NULL) ;
}

struct V4LEX__MacroDirectory *v4eval_GetMacroDir(ctx)
  struct V4C__Context *ctx ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4LEX__MacDirBoot vlmdb ;
  struct V4LEX__MacroDirectory *vlmd ;
  BYTE *ptr ;
  INDEX rx, areax, ex ; LENMAX bytes ;

	if (gpi->vlmd != NULL) return(gpi->vlmd) ;	/* Already have it ? */

	for(;!GETSPINLOCK(&macLock);) {} ;
/*	First need to grab boot structure to get directory segment id */
	vlmd = NULL ;
	for(rx=gpi->LowHNum;rx<=gpi->HighHNum;rx++)
	 { if (rx == V4DPI_WorkRelHNum || gpi->RelH[rx].aid == UNUSED) continue ;
	   vlmdb.kp.fld.KeyType = V4IS_KeyType_V4 ; vlmdb.kp.fld.KeyMode = V4IS_KeyMode_Int ; vlmdb.kp.fld.AuxVal = V4IS_SubType_MacroBoot ;
	   vlmdb.kp.fld.Bytes = V4IS_IntKey_Bytes ; vlmdb.mustBeZero = 0 ;	/* Set up key field */
	   FINDAREAINPROCESS(areax,gpi->RelH[rx].aid)
	   if (!GRABAREAMTLOCK(areax)) continue ;
	   if (v4is_PositionKey(gpi->RelH[rx].aid,(struct V4IS__Key *)&vlmdb,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey)
	    { FREEAREAMTLOCK(areax) ; continue ; } ;
	   ptr = (BYTE *)v4dpi_GetBindBuf(ctx,gpi->RelH[rx].aid,ikde,FALSE,NULL) ;
	   if (ptr == NULL) goto fail ;
	   memcpy(&vlmdb,ptr,sizeof vlmdb) ;
	   FREEAREAMTLOCK(areax) ; break ;
	 } ; if (rx > gpi->HighHNum) goto fail ;		/* No macros previously defined */

	vlmd = (struct V4LEX__MacroDirectory *)v4seg_GetSegments(ctx,vlmdb.segId,&bytes,FALSE,gpi->RelH[rx].aid) ;
	gpi->vlmd = (struct V4LEX__MacroDirectory *)v4mm_AllocChunk(bytes,FALSE) ;
	memcpy(gpi->vlmd,vlmd,bytes) ; vlmd = gpi->vlmd ;

/*	Got the directory - clear any old memory pointers */
	for(ex=0;ex<vlmd->numEntries;ex++)
	 { vlmd->macro[ex].vlme = NULL ; } ;

	RLSSPINLOCK(macLock) ;
	return(vlmd) ;
fail:
	RLSSPINLOCK(macLock) ; return(NULL) ;
}
