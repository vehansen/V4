/*	V4CTX.C - Context, List Modules

	Create 31-Mar-92 by Victor E. Hansen			*/

#ifndef NULLID
#include "v4defs.c"
#include <time.h>
#include <setjmp.h>
#endif

//struct V4MM__MemMgmtMaster *V4_GLOBAL_MMM_PTR ;

//extern char ASCTBUF1[], v4mbuf[] ;
GLOBALDIMSEXTERN
struct V4C__ProcessInfo *gpi  ;		/* Global process information */
extern struct V__UnicodeInfo *uci ;	/* Global structure of Unicode Info */
extern int opcodeDE[] ;
extern ETYPE traceGlobal ;

#define LOCKROOT lrx = v4mm_LockSomething(aid,V4MM_LockId_Root,V4MM_LockMode_Write,V4MM_LockIdType_Root,5,NULL,2)
#define RELROOT v4mm_LockRelease(aid,lrx,mmm->MyPid,aid,"RELROOTDPI") ; v4mm_TempLock(aid,0,FALSE)

/*	C O N T E X T   M O D U L E S					*/

/*	v4ctx_Initialize - Initialize Context Structures		*/
/*	Call: v4ctx_Initialize( ctx , pctx , mmm )
	  where ctx is pointer to context,
	  pctx is pointer to prior context (or NULL) if this is root,
	  mmm is memory management structure (NULL for default)		*/

void v4ctx_Initialize(ctx,pctx,mmm)
  struct V4C__Context *ctx ;
  struct V4C__Context *pctx ;
  struct V4MM__MemMgmtMaster *mmm ;
{ int rx ;
  struct V4IS__ParControlBlk pcb ;
  struct V4C__AreaHInfo ahi ;

	if (gpi == NULL) v_GetProcessInfo() ;
	memset(ctx,0,sizeof *ctx) ;
	ctx->pi = gpi ;					/* Link to owner process */
#ifdef V4ENABLEMULTITHREADS
	ctx->pctx = pctx ;				/* Link to prior context (if any) */
#endif
	v4ctx_FramePush(ctx,UClit("V4KERNEL")) ;	/* Init first frame */
	if (pctx == NULL)				/* If this is root context then create temp work Area */
	 { for(rx=0;rx<V4C_CtxRelH_Max;rx++) { gpi->RelH[rx].aid = UNUSED ; } ;
/*	   Create, Init, & link process specific (RelHNum=7) Area */
	   memset(&pcb,0,sizeof pcb) ; strcpy(pcb.V3name,"proca") ; pcb.BktSize = V4Area_DfltBktSize ;
	   pcb.DataMode = V4IS_PCB_DataMode_Auto ; pcb.AccessMode = -1 ; pcb.BktSize = V4TempArea_BktSize ;
	   pcb.DfltPutMode = V4IS_PCB_GP_Insert ; pcb.MinCmpBytes = 500 ; pcb.DfltDataMode = 500 ;
	   v_MakeOpenTmpFile(UClit("v4tw"),pcb.UCFileName,UCsizeof(pcb.UCFileName),NULL,ctx->ErrorMsgAux) ;
	   memset(&ahi,0,sizeof ahi) ;
	   ahi.BindingsUpd = TRUE ; ahi.IntDictUpd = TRUE ; ahi.ExtDictUpd = TRUE ;
	   pcb.OpenMode = V4IS_PCB_OM_NewTemp ; ahi.RelHNum = V4DPI_WorkRelHNum ; pcb.RelHNum = ahi.RelHNum ;
	   v4is_Open(&pcb,NULL,NULL) ;
	   if (v4ctx_AreaAdd(ctx,&pcb,&ahi,NULL) == UNUSED)
	    { v_Msg(ctx,NULL,"@*Error initializing context - %1U\n",ctx->ErrorMsgAux) ; vout_UCText(VOUT_Err,0,ctx->ErrorMsg) ; } ;
	 } ;
}

/*	v4ctx_Release - Release Context Structure			*/
/*	Call: v4ctx_Release( ctx )
	  where ctx is pointer to context				*/

void v4ctx_Release(ctx)
  struct V4C__Context *ctx ;
{ int i ;

	for(i=0;i<V4C_CtxDimHash_Max;i++) { if (ctx->DimHash[i].di != NULL) v4mm_FreeChunk(ctx->DimHash[i].di) ; } ;
	for(i=0;i<V4C_CtxVal_Max;i++) { if (ctx->CtxVal[i].nbl != NULL) v4mm_FreeChunk(ctx->CtxVal[i].nbl) ; } ;
	for(i=0;i<V4TRACE_MaxNestedrtStack;i++)
	 { if (ctx->rtStack[i].failText != NULL) v4mm_FreeChunk(ctx->rtStack[i].failText) ;
//	   if (ctx->IsctStack[i].sIsctPtr != NULL) v4mm_FreeChunk(ctx->IsctStack[i].sIsctPtr) ;
//	   if (ctx->IsctStack[i].IsctPrint != NULL) v4mm_FreeChunk(ctx->IsctStack[i].IsctPrint) ;
//	   if (ctx->IsctStack[i].FailReason != NULL) v4mm_FreeChunk(ctx->IsctStack[i].FailReason) ;
	 } ;
	for(i=0;i<V4TMBufMax;i++) { if (ctx->imArgBuf[i] != NULL) v4mm_FreeChunk(ctx->imArgBuf[i]) ; } ;
	v4mm_FreeChunk(ctx) ;
} ;

/*	v4ctx_InitTempAgg - Called when necessary to create temporary aggregate area	*/
/*	Call: aggx = v4ctx_InitTempAgg( ctx )
	  where aggx is internal aggregate id if ok, UNUSED if error (ctx->ErrorMsgAux)
		ctx is context								*/

int v4ctx_InitTempAgg(ctx)
  struct V4C__Context *ctx ;
{ struct V4IS__ParControlBlk pcb ;

	memset(&pcb,0,sizeof pcb) ; strcpy(pcb.V3name,"tagg") ; pcb.BktSize = V4AreaAgg_DfltBktSize ;
	pcb.DataMode = V4IS_PCB_DataMode_Auto ; pcb.AccessMode = -1 ;
	pcb.DfltPutMode = V4IS_PCB_GP_Insert | V4IS_PCB_NoDataCmp ; pcb.DfltDataMode = 500 ;
	v_MakeOpenTmpFile(UClit("v4ta"),pcb.UCFileName,UCsizeof(pcb.UCFileName),NULL,ctx->ErrorMsgAux) ;
	pcb.OpenMode = V4IS_PCB_OM_NewTemp ; pcb.AreaFlags |= V4IS_PCB_OF_NoError ;
	if (!v4is_Open(&pcb,NULL,ctx->ErrorMsgAux)) return(UNUSED) ;
	return(v4im_AggLoadArea(ctx,&pcb,NULL)) ;
}

/*	v4ctx_AreaAdd - Adds area to context				*/
/*	Call: uid = v4ctx_AreaAdd( ctx , areaid , filename , ahi , uidPnt)
	  where uid is unique id for this area (UNUSED if error),
		ctx is pointer to context,
		areaid is area id to add,
		filename is pointer to string file name,
		ahi is pointer to override heirarchy information,
		uidPnt is user assigned unique Id point			*/

int v4ctx_AreaAdd(ctx,pcb,ahi,uidPnt)
  struct V4C__Context *ctx ;
  struct V4IS__ParControlBlk *pcb ;
  struct V4C__AreaHInfo *ahi ;
  struct V4DPI__LittlePoint *uidPnt ;
{ struct V4IS__AreaCB *acb ;
  struct V4DPI__AreaBindInfo *abi,*abiptr ;
  struct V4MM__MemMgmtMaster *mmm ;
//  extern struct V4MM__MemMgmtMaster *V4_GLOBAL_MMM_PTR ;
  int ax ; AREAID aid ; INDEX areax ;

//	mmm = V4_GLOBAL_MMM_PTR ;
	FINDAREAINPROCESS(areax,pcb->AreaId)
	acb = v4mm_ACBPtr(pcb->AreaId) ;	/* Link up to control block for area */
	if (acb->ahi == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"CtxNotV4Area",pcb->UCFileName) ; return(UNUSED) ; } ;
//	 v4_error(V4E_CTXADDAREANOTV4,0,"V4DPI","AreaAdd","CTXADDAREANOTV4","Cannot add area (%s) to context- not a V4 area",pcb->FileName) ;
/*	If passed any overrides then have to go thru field by field, otherwise just copy */
#define VAL(EL) (ahi->EL != 0 ? ahi->EL : acb->ahi->EL)
#define SV(EL) gpi->RelH[ax].ahi.EL = VAL(EL) ;
	if (ahi == NULL) { gpi->RelH[ax = acb->ahi->RelHNum].ahi = *acb->ahi ; }
	 else { ax = VAL(RelHNum) ;
		if (ax < 0 || ax >= V4C_CtxRelH_Max)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CtxNotV4Area",pcb->UCFileName) ; return(UNUSED) ; } ;
//		 v4_error(V4E_CTXADDAREANOTV4,0,"V4DPI","AreaAdd","CTXADDAREANOTV4","Cannot add area (%s) to context- not a V4 area",pcb->FileName) ;
		SV(ExtDictUpd) SV(IntDictUpd) SV(BindingsUpd)
	      } ;
	if ((acb->AccessMode & (V4IS_PCB_AM_Insert | V4IS_PCB_AM_Update)) != (V4IS_PCB_AM_Insert | V4IS_PCB_AM_Update))
	 { gpi->RelH[ax].ahi.ExtDictUpd = FALSE ;	/* Area not open for updates! */
	   gpi->RelH[ax].ahi.IntDictUpd = FALSE ;
	   gpi->RelH[ax].ahi.BindingsUpd = FALSE ;
	 } ;
	if (ax < gpi->LowHNum) gpi->LowHNum = ax ; if (ax > gpi->HighHNum) gpi->HighHNum = ax ;
/*	Do we gots a primary or slave area here? */
//	if (gpi->RelH[ax].ahi.IsPrimary)
	if (TRUE)
	 { if (gpi->RelH[ax].aid != UNUSED)
	    { aid = gpi->RelH[ax].aid ;
	      v_Msg(ctx,ctx->ErrorMsgAux,"CtxDupHNum",aid,mmm->Areas[aid].UCFileName,ax) ; return(UNUSED) ;
//	      v4_error(V4E_AREADUPHNUM,0,"V4DPI","AreaAdd","AREADUPHNUM","Area (%d %s) already defined for this Heirarchy number (%d)",
//			aid,mmm->Areas[aid].FileName,ax) ;
	    } ;
	   gpi->RelH[ax].aid = pcb->AreaId ;
	 } else
	 { gpi->RelH[ax].SlaveAids[gpi->RelH[ax].SlaveCnt++] = pcb->AreaId ;
	 } ;
/*	Set optional Id for area */
	if (uidPnt == NULL ? FALSE : uidPnt->Bytes > 0)
	 { memcpy(&gpi->RelH[ax].areaPId,uidPnt,uidPnt->Bytes) ; }
	 else { ZPH(&gpi->RelH[ax].areaPId) ; } ;
	gpi->RelH[ax].AreaUId = (gpi->NextUId++) ;
/*	Now see if we can pull BindInfo record from this area  */
	abi = (struct V4DPI__AreaBindInfo *)v4mm_AllocChunk(sizeof *abi,FALSE) ;
	mmm->Areas[areax].abi = abi ;
	abi->kp.fld.KeyType = V4IS_KeyType_V4 ; abi->kp.fld.KeyMode = V4IS_KeyMode_Int ; abi->kp.fld.Bytes = V4IS_IntKey_Bytes ;
	abi->kp.fld.AuxVal = V4IS_SubType_BindInfo ; abi->key = 0 ;
	if (v4is_PositionKey(pcb->AreaId,(struct V4IS__Key *)&abi->kp,(struct V4IS__Key **)&abiptr,0,V4IS_PosDCKL) == V4RES_PosKey)	/* See if anything exists */
	 { memcpy(abi,abiptr,abiptr->Bytes) ;				/* Got something - copy it in */
	 } else { abi->DimCnt = 0 ; abi->Bytes = 0 ; } ;

	gpi->RelH[ax].pcb = (struct V4IS__ParControlBlk *)v4mm_AllocChunk(sizeof *pcb,FALSE) ;
	memcpy(gpi->RelH[ax].pcb,pcb,sizeof *pcb) ;			/* Keep local copy of pcb */
/*	If a new area and we have gpi->vcd then link it to the hnum */
	if (gpi->vcd != NULL)
	 { switch(pcb->OpenMode)
	    {
	      case V4IS_PCB_OM_New:
	      case V4IS_PCB_OM_NewIf:
	      case V4IS_PCB_OM_NewAppend:
	      case V4IS_PCB_OM_NewTemp:
	      case V4IS_PCB_OM_MustBeNew:	gpi->RelH[ax].vcd = gpi->vcd ; break ;
	    } ;
	 } ;
	return(gpi->RelH[ax].AreaUId) ;
}

/*	v4ctx_AreaClose - Removes area from context			*/
/*	Call: v4ctx_AreaClose( ctx , areaid )
	  where areaid is area to be removed (-1 for all)				*/

void v4ctx_AreaClose(ctx,areaid)
  struct V4C__Context *ctx ;
  int areaid ;
{ struct V4MM__MemMgmtMaster *mmm ;
//  extern struct V4MM__MemMgmtMaster *V4_GLOBAL_MMM_PTR ;
  int rx,ax ; UCCHAR msg[256] ;

	v4l_Purgevbe(ctx) ; v4l_Purgevbeg(ctx) ;			/* Write out any dirty list buffers */
//	if (gpi->MassUpdate)
//	 { v4dpi_BindListMake(NULL,NULL,NULL,ctx,NULL,NOWGTADJUST,0,DFLTRELH) ;		/* Maybe flush out last binding nbl */
//	 } ;
	v4dpi_AddDUPoint(ctx,NULL,NULL,0,V_DUPDICTACT_Ignore) ;	/* Flush out dimension point lists */
	v4dpi_DimUnique(ctx,NULL,NULL) ;				/* Flush out any {new} points */
	v4im_AggPutBlocked(ctx,NULL,DFLTAGGAREA) ;			/* Maybe flush out any BFAggregates */
	v4v4is_FlushStEl(ctx) ;						/* Maybe flush out any Structure/Element records */
	if (areaid == -1)
	 { v4dpi_FlushXDict(ctx) ;
	   v4tree_SaveTrees(ctx) ;
	   v4eval_SaveMacroDir(ctx) ;
	 } ;
//	mmm = V4_GLOBAL_MMM_PTR ;
	for(rx=0;rx<gpi->AreaAggCount;rx++)
	 { if (gpi->AreaAgg[rx].pcb == NULL) continue ;
	   if (areaid == -1 ? FALSE : gpi->AreaAgg[rx].pcb->AreaId != areaid) continue ;
	   FINDAREAINPROCESS(ax,gpi->AreaAgg[rx].pcb->AreaId) ;
	   if (ax == UNUSED) continue ;			/* Could not find area? */
	   if (gpi->AreaAgg[rx].vad != NULL)				/* Write out Aggregate dim/key info? */
	    { if ((gpi->AreaAgg[rx].pcb->AccessMode & (V4IS_PCB_AM_Insert | V4IS_PCB_AM_Update)) != 0)
	       { gpi->AreaAgg[rx].pcb->PutBufPtr = (char *)gpi->AreaAgg[rx].vad ;
		 gpi->AreaAgg[rx].pcb->PutBufLen = sizeof *gpi->AreaAgg[rx].vad ;
		 gpi->AreaAgg[rx].pcb->PutMode = V4IS_PCB_GP_Write ;
		 v4is_Put(gpi->AreaAgg[rx].pcb,NULL) ;			/* (Re)write the vad structure */
		 v4mm_FreeChunk(gpi->AreaAgg[rx].vad) ; gpi->AreaAgg[rx].vad = NULL ;
	       } ;
	    } ;
	   if (gpi->AreaAgg[rx].pcb->MaxBlockFillPC > 0 && gpi->AreaAgg[rx].pcb->MaxBlockFillPC < 80.00)
	    { v_Msg(ctx,msg,"@*Maximum fill %1d%% for aggregate %2U\n",DtoI(gpi->AreaAgg[rx].pcb->MaxBlockFillPC),gpi->AreaAgg[rx].pcb->UCFileName) ;
	      vout_UCText(VOUT_Trace,0,msg) ;
	    } ;
	   v4is_Close(gpi->AreaAgg[rx].pcb) ; v4mm_FreeChunk(gpi->AreaAgg[rx].pcb) ; gpi->AreaAgg[rx].pcb = NULL ;
	 } ;
	for(rx=gpi->HighHNum;rx>=gpi->LowHNum;rx--)
	 { if (gpi->RelH[rx].aid == UNUSED) continue ;
	   if (rx == V4DPI_WorkRelHNum) continue ;			/* Don't close off internal work area */
	   if (areaid == -1 ? TRUE : areaid == gpi->RelH[rx].aid)
	    { 
/*	      If area open for updates then write out compilation info */
	      if (gpi->vcd != NULL ? ((gpi->RelH[rx].pcb->AccessMode & (V4IS_PCB_AM_Insert | V4IS_PCB_AM_Update)) != 0) : FALSE)
	       { gpi->vcd->kp.fld.KeyType = V4IS_KeyType_V4 ; gpi->vcd->kp.fld.AuxVal = V4IS_SubType_CompileDir ; gpi->vcd->kp.fld.KeyMode = V4IS_KeyMode_Int ;
	         gpi->vcd->kp.fld.Bytes = 8 ; gpi->vcd->Key0 = 0 ;
//	         gpi->vcd->compUpdCal = VCal_MJDOffset + TIMEOFFSETDAY +(double)(time(NULL))/(double)VCAL_SecsInDay ;
//		 frac = modf(gpi->vcd->compUpdCal,&ipart) ; if (frac == 0.0) gpi->vcd->compUpdCal += VCAL_MidnightValue ;	/* Want some fractional value to indicate time */
	         setCALisNOW(gpi->vcd->compUpdCal)
	         gpi->vcd->verMajor = V4IS_MajorVersion ; gpi->vcd->verMinor = V4IS_MinorVersion ;
	         gpi->vcd->Bytes = (char *)&gpi->vcd->File[gpi->vcd->fileCount] - (char *)gpi->vcd ;
	         gpi->RelH[rx].pcb->PutBufPtr = (char *)gpi->vcd ; gpi->RelH[rx].pcb->PutBufLen = gpi->vcd->Bytes ; gpi->RelH[rx].pcb->PutMode = V4IS_PCB_GP_Write ;
		 v4is_Put(gpi->RelH[rx].pcb,NULL) ;			/* (Re)write the vcd structure */
	       } ;
	      v4mm_AreaFlush(gpi->RelH[rx].aid,TRUE) ;			/* Got matching area - flush it */
#ifdef WINNT
	      CloseHandle(gpi->mmm->Areas[gpi->RelH[rx].aid].hfile) ;
#else
	      fclose(gpi->mmm->Areas[gpi->RelH[rx].aid].FilePtr) ;
#endif
	      gpi->RelH[rx].aid = UNUSED ;				/* Flag as closed for possible re-open */
	      gpi->RelH[rx].AreaUId = UNUSED ;
	    } ;
	 } ;
}

///*	v4ctx_AreaCheckPoint - CheckPoints all buffers in areas for ctx		*/
///*	Call: v4ctx_AreaCheckPoint( ctx , areaid )
//	  where areaid is area to be check pointed (-1 for all)				*/
//
//void v4ctx_AreaCheckPoint(ctx,areaid)
//  struct V4C__Context *ctx ;
//  int areaid ;
//{ struct V4MM__MemMgmtMaster *mmm ;
//  struct V4DPI_XDictDimPoints xddp ;
////  extern struct V4MM__MemMgmtMaster *V4_GLOBAL_MMM_PTR ;
//  int rx,i,ax ;
//
//	v4l_Purgevbe(ctx) ; v4l_Purgevbe(ctx) ;				/* Write out any dirty list buffers */
////	if (gpi->MassUpdate)
////	 v4dpi_BindListMake(NULL,NULL,NULL,ctx,NULL,NOWGTADJUST,0,DFLTRELH) ;		/* Maybe flush out last binding nbl */
//	FINDAREAINPROCESS(ax,areaid)
//	if (ax == UNUSED) return ;			/* Could not find area? */
////	mmm = V4_GLOBAL_MMM_PTR ;
//	for(rx=gpi->HighHNum;rx>=gpi->LowHNum;rx--)			/* Find highest area allowing new entries */
//	 { if (gpi->RelH[rx].aid == UNUSED) continue ;
//	   if (areaid == -1 ? TRUE : areaid == gpi->RelH[rx].aid)
//	    { v4mm_AreaCheckPoint(gpi->RelH[rx].aid) ;			/* Got matching area */
//	    } ;
//	 } ;
//	if (gpi->xdr != NULL)
//	 { for(i=0;i<V4DPI_XDictRTCacheMax;i++)
//	    { if ((gpi->xdr->Cache[i].Flags & XDRDIRTY) == 0) continue ;
//	      xddp.kp.fld.AuxVal = V4IS_SubType_XDictDimPoints ; xddp.kp.fld.KeyType = V4IS_KeyType_V4 ; xddp.kp.fld.KeyMode = V4IS_KeyMode_Alpha ;
//	      xddp.DimXId = gpi->xdr->Cache[i].DimXId ; xddp.kp.fld.Bytes = (char *)&xddp.LastPoint - (char *)&xddp ;
//	      xddp.LastPoint = gpi->xdr->Cache[i].LastPoint ;
//#ifdef XDICTMU
//		gpi->xdr->pcb->GetMode = V4IS_PCB_GP_Write | V4IS_PCB_NoError ;
//		gpi->xdr->pcb->KeyPtr = (struct V4IS__Key *)&xddp ;
//		gpi->xdr->pcb->PutBufPtr = (BYTE *)&xddp ;
//		gpi->xdr->pcb->PutBufLen = sizeof xddp  ;
//		gpi->xdr->pcb->DataMode = V4IS_PCB_DataMode_Index ;
//		v4is_Put(gpi->xdr->pcb,gpi->ctx->ErrorMsgAux) ;
//#else
//	       if (gpi->xdr->Cache[i].Flags & XDRNEW)
//	        { v4is_Insert(gpi->xdr->aid,(struct V4IS__Key *)&xddp,&xddp,sizeof xddp,V4IS_PCB_DataMode_Index,0,0,FALSE,FALSE,0,NULL) ;
//	        } else
//	        { struct V4IS__AreaCB *acb = v4mm_ACBPtr(gpi->xdr->aid) ;
//		  v4is_PositionKey(gpi->xdr->aid,(struct V4IS__Key *)&xddp,NULL,0,V4IS_PosBoKL) ;
//		  v4is_Replace(gpi->xdr->aid,(struct V4IS__Key *)&xddp,&xddp,&xddp,sizeof xddp,V4IS_PCB_DataMode_Index,V4IS_DataCmp_None,acb->DataId,0) ;
//	        } ; gpi->xdr->Cache[i].Flags &= ~(XDRNEW+XDRDIRTY) ;
//#endif
//	    } ;
//	   v4mm_AreaCheckPoint(gpi->xdr->aid) ;
//	 } ;
//}

/*	v4ctx_FramePush - Pushes new frame onto context					*/
/*	Call: frameid = v4ctx_FramePush( ctx , name )
	  where frameid = new frame id,
		ctx is current context,
		name is string name of new context (if NULL then "." is used, see AddDim()/V4C_FrameId_Real)		*/

int v4ctx_FramePush(ctx,name)
  struct V4C__Context *ctx ;
  UCCHAR *name ;
{
	if (ctx->FrameCnt >= V4C_Frame_Max)
	 { UCCHAR tbuf[200] ; int i ; ZUS(tbuf) ;
	   for(i=ctx->FrameCnt-1;i>=0&&UCstrlen(tbuf) < (UCsizeof(tbuf) + V4C_FrameName_Max + 5);i--) { UCstrcat(tbuf,ctx->Frame[i].FrameName) ; UCstrcat(tbuf,UClit(",")) ; } ;
	   v_Msg(ctx,UCTBUF1,"CtxTooManyFrm",V4C_Frame_Max,tbuf) ; v4_UCerror(V4E_MAXCTXFRAMES,0,"","","",UCTBUF1) ;
	 } ;
	ctx->Frame[ctx->FrameCnt].FrameId = ctx->NextAvailFrameId++ ;
	if (name == NULL) { ctx->Frame[ctx->FrameCnt].FrameName[0] = UClit('.') ; }
	 else { UCstrncpy(ctx->Frame[ctx->FrameCnt].FrameName,name,V4C_FrameName_Max-1) ; } ;
	ctx->Frame[ctx->FrameCnt].PointCnt = 0 ;
//	ctx->Frame[ctx->FrameCnt].fhl = NULL ;
/*	Default output to Data Stream */
	if (ctx->FrameCnt == 0) { ctx->Frame[ctx->FrameCnt].DataStreamFileX = UNUSED ; }
	 else { ctx->Frame[ctx->FrameCnt].DataStreamFileX = ctx->Frame[ctx->FrameCnt - 1].DataStreamFileX ; } ;
	ctx->FrameCnt ++ ;
	return(ctx->Frame[ctx->FrameCnt-1].FrameId) ;
}

///*	v4ctx_FrameHan - Allocates handle & links to current frame	*/
///*	Call: han = v4ctx_FrameHan( ctx )
//	  where han is new handle,
//		ctx is context						*/
//
//int v4ctx_FrameHan(ctx)
//  struct V4C__Context *ctx ;
//{ struct V4IM__FrameHanList *fhl ;
//  int han ;
//
//	if((fhl=ctx->Frame[ctx->FrameCnt].fhl) == NULL)
//	 { ctx->Frame[ctx->FrameCnt].fhl = (struct V4IM__FrameHanList *)v4mm_AllocChunk(sizeof *fhl,TRUE) ;
//	   fhl = ctx->Frame[ctx->FrameCnt].fhl ;
//	 } ;
//	if (fhl->Count >= V4IM_FHL_Max)			/* If this one is full, link to new one */
//	 { fhl = (struct V4IM__FrameHanList *)v4mm_AllocChunk(sizeof *fhl,TRUE) ;
//	   fhl->fhl = ctx->Frame[ctx->FrameCnt].fhl ;
//	   ctx->Frame[ctx->FrameCnt].fhl = fhl ;
//	 } ;
//	han = (fhl->Handles[fhl->Count] = han_Make()) ;
//	fhl->Count++ ;
//	return(han) ;
//}

/*	v4ctx_FramePop - Pops one or more context frames		*/
/*	Call: ok = v4ctx_FramePop( ctx , frameid , name )
	  where ok is TRUE if OK FALSE if not (could not find),
		ctx is context,
		frameid is frame to pop-to (0 for current, -1 to use name),
		name is frame name to pop-to if frameid == -1		*/

LOGICAL v4ctx_FramePop(ctx,frameid,name)
  struct V4C__Context *ctx ;
  int frameid ;
  UCCHAR *name ;
{ 
//  struct V4IM__FrameHanList *fhl,*fhl1 ;
  int i,fx,px,cvx,dx,pvx,fid ;

	if (ctx->FrameCnt <= 1) return(FALSE) ;		/* Can't pop no more! */
/*	First figure out where to pop up to */
	if (frameid > 0)
	 { fid = frameid ;
	   if (ctx->Frame[ctx->FrameCnt-1].FrameId < fid)
	    return(FALSE) ;
//	    v4_error(V4E_INVPOPFRAMEID,0,"V4DPI","FramePop","INVPOPFRAMEID","Cannot FramePop to id (%d), top frame id is currently (%d)",
//			fid,ctx->Frame[ctx->FrameCnt-1].FrameId) ;
	 } else { if (frameid == 0) { fid = ctx->Frame[ctx->FrameCnt-1].FrameId ; }
		 else { for(fx=ctx->FrameCnt-1;fx>=0;fx--)
			 { if (UCstrcmpIC(ctx->Frame[fx].FrameName,name) == 0) break ; } ;
			if (fx < 0) return(FALSE) ;
			fid = ctx->Frame[fx].FrameId ;
//			 v4_error(V4E_UNKFRAMENAME,0,"V4DPI","FramePop","UNKFRAMENAME","No such frame with specified name (%s)",name) ;
		      } ;
	      } ;
/*	Now pop off until done */
	for(fx=ctx->FrameCnt-1;fx>=0;fx--)
	 { if (ctx->Frame[fx].FrameId < fid) break ;			/* All done ? */
	   if (ctx->Frame[fx].baf != NULL)				/* If got BaA then deallocate */
	    { v4mm_FreeChunk(ctx->Frame[fx].baf) ; ctx->Frame[fx].baf = NULL ; } ;
//	   for(fhl=ctx->Frame[fx].fhl;fhl!=NULL;)			/* Free up any link lists */
//	    { fhl1 = fhl->fhl ;
//	      for(i=0;i<fhl->Count;i++) { han_Close(fhl->Handles[i]) ; } ;
//	      v4mm_FreeChunk(fhl) ; fhl = fhl1 ;
//	    } ;	
	   for(px=0;px<ctx->Frame[fx].PointCnt;px++)
	    { dx = ctx->Frame[fx].FrameDimEntry[px].DimHashIndex ;	/* Index to linked dimension */
	      cvx = ctx->Frame[fx].FrameDimEntry[px].CtxValIndex2 ;	/* Index to point for this frame */
	      pvx = ctx->CtxVal[cvx].PriorCtxValIndex ;			/* Index to prior point for dim (-1 if none) */
	      ctx->DimHash[dx].CtxValIndex = pvx ;			/* Link to prior point */
	      ctx->CtxVal[cvx].FrameIndex = ctx->FirstFreeIndex ;	/* Free up this point */
	      ctx->FirstFreeIndex = cvx ;
	      if (ctx->CtxVal[cvx].HaveNBL)
	       { for(i=0;i<ctx->CtxVal[cvx].nblCnt;i++)
		  { if (ctx->FnblCnt < V4C_FreeNblListMax-1) ctx->fnbl[ctx->FnblCnt++] = ctx->CtxVal[cvx].nbl[i] ;
		    ctx->CtxVal[cvx].nbl[i] = NULL ;
		  } ;
		 ctx->CtxVal[cvx].nblCnt = 0 ;
		 if (pvx == -1 ? TRUE : !ctx->CtxVal[pvx].HaveNBL)	/* Had BindList but don't anymore ? */
		  { for(i=0;i<ctx->EvalListCnt;i++)
		     { if (ctx->EvalListDimIndexes[i] != dx) continue ;	/* Have to take out of list */
		       ctx->EvalListDimIndexes[i] = ctx->EvalListDimIndexes[--ctx->EvalListCnt] ;
		       break ;
		     } ;
		  } ;
	       } else
	       { if (pvx == -1 ? FALSE : ctx->CtxVal[pvx].HaveNBL)	/* Didn't have BindList but do now? */
	          { for(i=0;i<ctx->EvalListCnt;i++) { if (ctx->EvalListDimIndexes[i] == dx) break ; } ;
		    ctx->EvalListDimIndexes[i] = dx ; if (i >= ctx->EvalListCnt) ctx->EvalListCnt ++ ;
		  } ;
	       } ;
	    } ;
	   ctx->FrameCnt-- ;
	 } ;
	return(TRUE) ;		/* Return now current ID */
}


#define V4CTX_DimNBLCacheMax 101
#define V4CTX_DimNBLCacheBytes 256
struct V4CTX__DimNBLCache {
  int HasEntries ;			/* TRUE if any entries (set to FALSE after cache clear) */
  struct {
    int Dim, Key ;			/* Have to match on dimension & key */
    int aid ;
    struct V4DPI__BindList *nbl ;	/* Only want to cache if single record (all areas) */
   } Entry[V4CTX_DimNBLCacheMax] ;
} ;
struct V4CTX__DimNBLCache vdnc ;

void v4ctx_FrameCacheClear(ctx,shutdown)			/* Clears out above cache & loser cache in ctx */
  struct V4C__Context *ctx ;
  LOGICAL shutdown ;
{ int i ;

	if (ctx->LCacheDirty)			/* Only clear ctx cache if it's dirty */
	 { for(i=0;i<V4C_Cache_Max;i++) { ctx->Cache[i].lpt.Bytes = 0 ; } ;
	   ctx->LCacheDirty = FALSE ;
	 } ;
	if (!vdnc.HasEntries) return ;
	for(i=0;i<V4CTX_DimNBLCacheMax;i++) { vdnc.Entry[i].Dim = UNUSED ; } ;
	if (shutdown)
	 { for(i=0;i<V4CTX_DimNBLCacheMax;i++)
	    { if (vdnc.Entry[i].nbl != NULL) v4mm_FreeChunk(vdnc.Entry[i].nbl) ; } ;
	 } ;
	vdnc.HasEntries = FALSE ;
}

/*	v4ctx_FrameAddDim - Adds dimension point to context frame	*/
/*	Call: ok = v4ctx_FrameAddDim( ctx , frameid , point , dimid , basedim )
	  where ok is TRUE if OK, FALSE if error (reason in ctx->ErrorMsgAux),
		ctx is context,
		frameid is frame (0 for current, V4C_FrameId_Real for last "real"),
		point is "dim=value" to be added,
		dimid (if not 0 is dimension to be used to store point (for "dummy args" in iterations)
		basedimf (if not 0) is base dimension of which this dimension "isa", or V4C_CheckIsA to recursively add	*/
/*	NOTE: see definition of FASTCTXADD() macro and modify it if mods are made to this routine */

LOGICAL v4ctx_FrameAddDim(ctx,frameid,point,dimid,basedimf)
  struct V4C__Context *ctx ;
  int frameid ;
  struct V4DPI__Point *point ;
  int dimid,basedimf ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4DPI__BindList *nblrec ;
  struct V4DPI__BindListShell nbls,*nblsptr ;
  struct V4DPI__AreaBindInfo *abi ;
  struct V4DPI__Point lpnt,isaval,*tpt ;
  struct V4L__ListPoint *lp ;
  struct V4DPI__DimInfo *di ;
  static struct V4DPI__LittlePoint isapt ;
  int basedim,checkisa ; INDEX areax ;
  int i,fx,ex,hx,cx,px,opx,rel,aid,bx,chx,tryit,vdx,vdaid ; int iscur = FALSE ;
  INDEX lx,lx1 ;
  
#ifdef V4_BUILD_LMTFUNC_EXP_DATE
	if (gpi->Expired-- > 0 && gpi->Expired < 100)
	 return(TRUE) ;
#endif

/*	If point is a list & flagged ForceEval then recursively add each point to context */
	if (point->ForceEval)
	 { if (point->PntType == V4DPI_PntType_List)
	    { lp = ALIGNLP(&point->Value) ;
	      for (i=1;;i++)
	       { if (v4l_ListPoint_Value(ctx,lp,i,&lpnt) < 1) return(TRUE) ;
	         if (!v4ctx_FrameAddDim(ctx,frameid,&lpnt,0,basedimf)) return(FALSE) ;
	       } ;
	    } ;
	   point->ForceEval = FALSE ;
	 } ;
//VEH091115 - don't evaluate special points if using shell dimension
	if (point->PntType == V4DPI_PntType_Special && dimid == 0)
	 { switch (point->Grouping)
	    { default:				v_Msg(ctx,ctx->ErrorMsgAux,"CtxInvPnt",point) ; return(FALSE) ;
	      case V4DPI_Grouping_Current:
		DIMVAL(tpt,ctx,point->Dim) ;
		if (tpt == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"CtxNoCurVal2",point->Dim) ; return(FALSE) ; } ;
		point = tpt ;
	      case V4DPI_Grouping_Undefined:
	      case V4DPI_Grouping_All:
	      case V4DPI_Grouping_AllCnf:
	      case V4DPI_Grouping_None:
	      case V4DPI_Grouping_Sample:	break ;		/* Only allow undefine/all/none/sample in context */
	    } ;
	 } ;
//071004	if (dimid == 0 ? point->PntType == V4DPI_PntType_Isct || point->Dim == 0 : FALSE)
//	 { di = (point->Dim > 0 ? (struct V4DPI__DimInfo *)v4dpi_DimInfoGet(ctx,point->Dim) : NULL) ;
//	   if (di != NULL ? di->PointType != V4DPI_PntType_Decomp : TRUE)
//	    { v_Msg(ctx,ctx->ErrorMsgAux,"CtxInvPnt",point) ; return(FALSE) ; } ;
//	 } ;
//VEH100607
//if (point->PntType == V4DPI_PntType_Isct)
// { v_Msg(ctx,ctx->ErrorMsgAux,"@*Isct into ctx? - %1P\n",point) ; vout_UCText(VOUT_Trace,0,ctx->ErrorMsgAux) ;
//   v4trace_ExamineState(ctx,NULL,V4_EXSTATE_All,VOUT_Trace) ;
//   return(FALSE) ;
// } ;
	ctx->ContextAddCount++ ;
	i = (dimid == 0 ? point->Dim : dimid) ;		/* Get dimension Id */
	hx = DIMIDHASH(i) ;				/* Hash to get dimension slot */
	for(;;)
	 { if (ctx->DimHash[hx].Dim == i) break ;
	   if (ctx->DimHash[hx].Dim == 0) break ;
	   hx++ ; hx = DIMIDHASH(hx) ;				/* Slot busy - try another */
	 } ;
	if (frameid == V4C_FrameId_NoOpt)		/* Here when new nbl on point already in context - just reset nlb */
	 { frameid = 0 ; fx = ctx->FrameCnt-1 ;
	   for(ex=0;ex<ctx->Frame[fx].PointCnt;ex++)		/* Look for hx in current frame */
	    { if (ctx->Frame[fx].FrameDimEntry[ex].DimHashIndex == hx) break ; } ;
	   px = ctx->DimHash[hx].CtxValIndex ;
	   if (ctx->CtxVal[px].HaveNBL)
	    { for(i=0;i<ctx->CtxVal[px].nblCnt;i++)
	       { if (ctx->FnblCnt < V4C_FreeNblListMax-1) ctx->fnbl[ctx->FnblCnt++] = ctx->CtxVal[px].nbl[i] ;
	         ctx->CtxVal[px].nbl[i] = NULL ;
	       } ; ctx->CtxVal[px].nblCnt = 0 ;
	    } ;
	   goto reset_nbl ;
	 } else { if (dimid == 0)				/* Maybe check to see if point already in ctx */
		 { 
		   cx = ctx->DimHash[hx].CtxValIndex ;
		   if (cx != UNUSED && frameid == 0)
		    { fx = ctx->CtxVal[cx].FrameIndex ;
		      if (ctx->Frame[fx].FrameId == ctx->Frame[ctx->FrameCnt-1].FrameId
				 && (ctx->DimHash[hx].di == NULL ? FALSE : ctx->DimHash[hx].di->BindList==0))
		       { memcpy(&ctx->CtxVal[cx].Point,point,point->Bytes) ;	/* Slam bad boy over current! */
		         ctx->ContextAddSlams++ ;
//			 ctx->DimHash[hx].LastDimUpd = ctx->ContextAddCount ;	/* Update last update id */
		         return(TRUE) ;
		       } ;
		    } ;
		   if (frameid == 0)			/* VEH081114 - Only want to do this if going into current frame, not if we are putting into V4C_FrameId_Real frame */
		    { 
		      DIMVAL(tpt,ctx,point->Dim) ;
		      if (tpt != NULL) { if (memcmp(tpt,point,point->Bytes) == 0) return(TRUE) ; } ;	/* Already in context - just return */
		    } ;
		 } ;
	      } ;
//	ctx->DimHash[hx].LastDimUpd = ctx->ContextAddCount ;	/* Update last update id */
	basedim = basedimf & V4C_FrameAdd_BaseMask ;
	if ((basedimf & V4C_CheckIsA) != 0)
	 { if (point->Dim > 0) { DIMINFO(di,ctx,point->Dim) ; } else { di = NULL ; } ;
	   checkisa = (di == NULL ? FALSE : (di->Flags & V4DPI_DimInfo_HasIsA) != 0) ;
	 } else { checkisa = FALSE ; } ;
	if (checkisa)					/* Are we doing IsA checks? */
	 { 
//	   ZPH(&lpnt) ; lpnt.PntType = V4DPI_PntType_Isct ; lpnt.Grouping = 2 ;
	   INITISCT(&lpnt) ; NOISCTVCD(&lpnt) ; lpnt.Grouping = 2 ;
	   memcpy(&lpnt.Value.Isct.pntBuf[0],&isapt,isapt.Bytes) ;		/* Build up isct- [Isa contextpt] to eval */
	   memcpy(&lpnt.Value.Isct.pntBuf[isapt.Bytes],point,point->Bytes) ;
	   lpnt.Bytes =+ (isapt.Bytes + point->Bytes) ;
	   if (v4dpi_IsctEval(&isaval,&lpnt,ctx,0,NULL,NULL) != NULL)
	    { if (V4C_TraceIsA & basedimf)
	       { 
//	         v4dpi_PointToString(ASCTBUF1,&lpnt,ctx,-1) ; printf("V4-Trace-IsAEval: %s\n",ASCTBUF1) ;
//	         v4dpi_PointToString(ASCTBUF1,&isaval,ctx,-1) ; printf("V4-Trace-IsAValue: %s\n",ASCTBUF1) ;
		 v_Msg(ctx,UCTBUF1,"@* IsAEval: %1P\n",&lpnt) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
		 v_Msg(ctx,UCTBUF1,"@* IsAVal: %1P\n",&isaval) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
	       } ;
	      if (isaval.Dim == Dim_Dim)			/* If result is Dim:xx then add point with new dim */
	       { i = isaval.Value.IntVal ; memcpy(&isaval,point,point->Bytes) ; isaval.Dim = i ; } ;
	      if (!v4ctx_FrameAddDim(ctx,frameid,&isaval,0,V4C_CheckIsA+(basedim == 0 ? point->Dim : basedim))) return(FALSE) ; ;
	    } else
	    { if (V4C_TraceIsA & basedimf)
	       { 
//	         v4dpi_PointToString(ASCTBUF1,&lpnt,ctx,-1) ; printf("V4-Trace-IsAFailed: %s\n",ASCTBUF1) ;
		 v_Msg(ctx,UCTBUF1,"@*IsaFailed: %1P\n",&lpnt) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
	       } ;
	    } ;
	 } ;
	switch (frameid)
	 { default:
			for(fx=ctx->FrameCnt-1;fx>=0;fx--) { if (ctx->Frame[fx].FrameId == frameid) break ; } ;
			if (fx < 0)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CtxNoFrame",frameid) ; return(FALSE) ;
//			   v4_error(V4E_INVFRAMEID,0,"V4DPI","FrameAddDim","INVFRAMEID","No such frame ID (%d)",frameid) ;
			 } ;
			break ;
	   case V4C_FrameId_Hold:
	   case 0:	fx = ctx->FrameCnt-1 ; break ;
	   case V4C_FrameId_Real:
			for(fx=ctx->FrameCnt-1;fx>=0;fx--) { if (ctx->Frame[fx].FrameName[0] != UClit('.')) break ; } ;
			if (fx < 0)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"CtxNoFrame",frameid) ; return(FALSE) ;
//			   v4_error(V4E_INVFRAMEID,0,"V4DPI","FrameAddDim","INVFRAMEID","No such frame ID (%d)",frameid) ;
			 } ;
			break ;
	 } ;
	if (ctx->Frame[fx].PointCnt >= V4C_FrameDimEntry_Max-1)
	 { v4ctx_ExamineCtx(ctx,TRUE,VOUT_Trace) ;
//	   v4dpi_PointToString(ASCTBUF1,point,ctx,-1) ; printf("\nAttempting to add point: %s\n",ASCTBUF1) ;
	   v_Msg(ctx,UCTBUF1,"@\nAttempting to add point: %1P\n",point) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
	   for(i=0;i<ctx->Frame[fx].PointCnt;i++)
	    { 
//	      v4dpi_PointToString(ASCTBUF1,&ctx->CtxVal[ctx->Frame[fx].FrameDimEntry[i].CtxValIndex2].Point,ctx,-1) ; printf("  Curpt #%d: %s\n",i+1,ASCTBUF1) ;
	      v_Msg(ctx,UCTBUF1,"@  Curpt #%1d: %2P\n",i+1,&ctx->CtxVal[ctx->Frame[fx].FrameDimEntry[i].CtxValIndex2].Point) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
	    } ;
	   v_Msg(ctx,ctx->ErrorMsgAux,"CtxTooManyPts",V4C_FrameDimEntry_Max,point,ctx->Frame[fx].FrameName) ; return(FALSE) ;
//	   v4_error(V4E_FRAMEMAXPNTS,0,"V4DPI","FrameAddDim","FRAMEMAXPNTS","Too many points (%d) in current frame (%s)",
//			V4C_FrameDimEntry_Max,ctx->Frame[fx].FrameName) ;
	 } ;
	if (ctx->FirstFreeIndex > 0)
	 { px = ctx->FirstFreeIndex ; ctx->FirstFreeIndex = ctx->CtxVal[px].FrameIndex ;
	 } else
	 { if (ctx->CtxValCnt >= V4C_CtxVal_Max-1)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"CtxMaxPnts",V4C_CtxVal_Max,point) ; return(FALSE) ; } ;
//	    v4_error(V4E_MAXCTXPNTS,0,"V4DPI","FrameAddDim","MAXCTXPNTS","Too many points (%d) declared in current context",V4C_CtxVal_Max) ;
	   px = (++ctx->CtxValCnt) ;
	 } ;
	ctx->CtxVal[px].PriorCtxValIndex = -1 ;				/* No prior point (yet) */
#ifdef V4_BUILD_SECURITY
	ctx->CtxVal[px].Hold = (frameid == V4C_FrameId_Hold) ;
#endif
	memcpy(&ctx->CtxVal[px].Point,point,point->Bytes) ;		/* Copy in point */
	ctx->CtxVal[px].UnDefined = (point->PntType == V4DPI_PntType_Special && point->Grouping == V4DPI_Grouping_Undefined) ;
	ctx->CtxVal[px].FrameIndex = fx ;				/* Link to frame */
	ctx->CtxVal[px].BaseDim = basedim ;				/* Save base dim if given */
	ex = (ctx->Frame[fx].PointCnt++) ;
	if (dimid == 0) dimid = point->Dim ;				/* Get hashing dimension */
	if (ctx->DimHash[hx].Dim == 0)					/* Is this a new dimension? */
	 { ctx->DimHash[hx].Dim = dimid ; iscur = TRUE ;
	   ctx->DimHash[hx].CtxValIndex = px ;
	   v4dpi_DimInfoGet(ctx,dimid) ;				/* Link up dimension info */
	   if (dimid != point->Dim) v4dpi_DimInfoGet(ctx,point->Dim) ;	/* Link up dimension info */
	 } else
	 { opx = ctx->DimHash[hx].CtxValIndex ;				/* opx = index to old point */
	   rel = (opx < 0 ? -1 : (ctx->CtxVal[opx].FrameIndex - fx)) ;	/* How does current frame point relate to this frame */
	   rel = (rel < 0 ? -1 : (rel > 0 ? 1 : 0)) ;
	   switch (rel)
	    { case -1:							/* Current frame supercedes old frame */
#ifdef V4_BUILD_SECURITY
		if (opx >= 0 ? ctx->CtxVal[opx].Hold : FALSE)		/* Point in prior frame on "hold" ? */
		 { ctx->Frame[fx].PointCnt-- ;				/* "back-out" this point */
		   ctx->CtxVal[px].FrameIndex = ctx->FirstFreeIndex ; ctx->FirstFreeIndex = px ;
		   return(TRUE) ;
		 } ;
#endif
		ctx->DimHash[hx].CtxValIndex = px ;			/* Not on hold- then blast in new point linkage */
		ctx->CtxVal[px].PriorCtxValIndex = opx ;		/*  & link new point to old one */
		iscur = TRUE ; break ;
	      case 0:							/* Overwriting current frame */
		for(ex=0;ex<ctx->Frame[fx].PointCnt;ex++)		/* Look for hx in current frame */
		 { if (ctx->Frame[fx].FrameDimEntry[ex].DimHashIndex == hx) break ; } ;
		ctx->CtxVal[opx].FrameIndex = ctx->FirstFreeIndex ; /* Free up old point */
	        if (ctx->CtxVal[opx].HaveNBL)
	         { for(i=0;i<ctx->CtxVal[opx].nblCnt;i++)
		    { if (ctx->FnblCnt < V4C_FreeNblListMax-1) ctx->fnbl[ctx->FnblCnt++] = ctx->CtxVal[opx].nbl[i] ;
		      ctx->CtxVal[opx].nbl[i] = NULL ;
		    } ; ctx->CtxVal[opx].nblCnt = 0 ;
		 } ;
		ctx->FirstFreeIndex = opx ;
		ctx->Frame[fx].PointCnt-- ;			/* Back out old ex from above */
		ctx->DimHash[hx].CtxValIndex = px ;		/*  then blast in new point linkage */
		ctx->CtxVal[px].PriorCtxValIndex = ctx->CtxVal[opx].PriorCtxValIndex ;
		iscur = TRUE ; break ;
	      case 1:							/* New point superceded by old */
//VEH020607 Don't allow this any more - causes too much grief
//VEH081114 Only allow if replacing current point with same value (i.e. update ContextL(x:y) with Context(x:y) - want to insert same point into REAL frame) */
		DIMVAL(tpt,ctx,point->Dim) ;					/* Get current value */
		if (tpt == NULL ? TRUE : memcmp(tpt,point,point->Bytes) != 0)	/* If not same value then error */
		 { v_Msg(ctx,ctx->ErrorMsgAux,"@Attempting to put point(%1P) into context frame which is superceded",point) ;
		   ctx->Frame[fx].PointCnt-- ;
		   ctx->CtxVal[px].FrameIndex = ctx->FirstFreeIndex ; ctx->FirstFreeIndex = px ;
		   return(FALSE) ;
		 } ;
//printf("fx=%d, PointCnt=%d\n",fx,ctx->Frame[fx].PointCnt) ;
//x x x x x this is where the problem is
//ctx->Frame[fx].PointCnt-- ;
//ctx->CtxVal[px].FrameIndex = ctx->FirstFreeIndex ; ctx->FirstFreeIndex = px ;
//return(TRUE) ;
		for(lx=opx;lx>=0;)
		 { if (ctx->CtxVal[lx].FrameIndex <= fx) break ;	/* Look for point which does not supercede */
		   lx1 = lx ; lx = ctx->CtxVal[lx].PriorCtxValIndex ;
		 } ;
		ctx->CtxVal[px].PriorCtxValIndex = ctx->CtxVal[lx1].PriorCtxValIndex ;
		ctx->CtxVal[lx1].PriorCtxValIndex = px ;
		if (ctx->CtxVal[lx].FrameIndex == fx)			/* If found frame in point list then replace */
		 { for(ex=0;ex<ctx->Frame[fx].PointCnt;ex++)		/* Look for hx in current frame */
		    { if (ctx->Frame[fx].FrameDimEntry[ex].DimHashIndex == hx) break ; } ;
		   ctx->CtxVal[lx].FrameIndex = ctx->FirstFreeIndex ; /* Free up old point */
		   ctx->FirstFreeIndex = lx ;
		   ctx->Frame[fx].PointCnt-- ;			/* Back out old ex from above */
		   ctx->DimHash[hx].CtxValIndex = px ;		/*  then blast in new point linkage */
		   ctx->CtxVal[px].PriorCtxValIndex = (lx < 0 ? -1 : ctx->CtxVal[lx].PriorCtxValIndex) ;
		 } ;
		iscur = FALSE ; break ;
	    } ;
	 } ;
reset_nbl:
	if (ctx->DimHash[hx].di == NULL ? FALSE : ctx->DimHash[hx].di->BindList) /* Look for bindlist for this point ? */
	 { tryit = TRUE ;
	   if (point->Bytes <= sizeof(struct V4DPI__LittlePoint))	/* Maybe look on loser list first - don't bother */
	    { chx = point->Dim + point->Value.IntVal ; chx = chx & V4C_Cache_Mask ;
	      if (memcmp(&ctx->Cache[chx].lpt,point,point->Bytes) == 0)
	       { ctx->ContextAddCache++ ; tryit = FALSE ; } ;	/* Got loser point - don't search below */
	    } ;
	 } else { tryit = FALSE ; } ;
	if (tryit)						/* Do we have to look for bindlist for this point ? */
	 { nbls.blsk.KeyType = V4IS_KeyType_Binding ;
	   nbls.blsk.AuxVal = ctx->DimHash[hx].Dim ;
	   vdx = UNUSED ;					/* If nonzero then maybe fill cache down below */
	   switch (point->PntType)
	    { default:
		vdx = (nbls.blsk.AuxVal + point->Value.IntVal) % V4CTX_DimNBLCacheMax ; if (vdx < 0) vdx = -vdx ;
		if (vdnc.Entry[vdx].Dim == nbls.blsk.AuxVal && vdnc.Entry[vdx].Key == point->Value.IntVal)
		 {
		   ctx->CtxVal[px].HaveNBL = TRUE ;			/* Got a binding for this point! */
		   ctx->CtxVal[px].AreaIds[ctx->CtxVal[px].nblCnt] = vdnc.Entry[vdx].aid ;	/* Save Area Id for use in IsctEval */
		   if (ctx->FnblCnt <= 0)
		    { ctx->CtxVal[px].nbl[ctx->CtxVal[px].nblCnt] = (struct V4DPI__BindList *)v4mm_AllocChunk(sizeof(struct V4DPI__BindList),FALSE) ;
		    } else
		    { ctx->CtxVal[px].nbl[ctx->CtxVal[px].nblCnt] = ctx->fnbl[--ctx->FnblCnt] ;
		    } ;
		   memcpy(ctx->CtxVal[px].nbl[ctx->CtxVal[px].nblCnt++],vdnc.Entry[vdx].nbl,vdnc.Entry[vdx].nbl->Bytes) ;
		   if (iscur)					/* If this is now a "current" point then add to list */
		    { for(i=0;i<ctx->EvalListCnt;i++) { if (ctx->EvalListDimIndexes[i] == hx) break ; } ;
		      ctx->EvalListDimIndexes[i] = hx ; if (i >= ctx->EvalListCnt) ctx->EvalListCnt ++ ;
		    } ;
		   ctx->CtxnblCacheHits++ ;
		   goto skip_cause_got_cache ;
		 } ; vdx = 0 ;					/* Didn't get cache hit, vdx will trigger cache load below */
	    	nbls.blsk.KeyVal.IntVal = point->Value.IntVal ;
	    	nbls.blsk.KeyMode = V4IS_KeyMode_Int ; nbls.blsk.Bytes = V4IS_IntKey_Bytes ;
	    	break ;
	      case V4DPI_PntType_UOM:
		memcpy(&nbls.blsk.KeyVal.Alpha,&point->Value.UOMVal,sizeof(struct V4DPI__Value_UOM)) ;
		nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = sizeof(union V4IS__KeyPrefix) + sizeof(struct V4DPI__Value_UOM) ;
		break ;
	      case V4DPI_PntType_UOMPer:
		memcpy(&nbls.blsk.KeyVal.Alpha,&point->Value.UOMPerVal,sizeof(struct V4DPI__Value_UOMPer)) ;
		nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = sizeof(union V4IS__KeyPrefix) + sizeof(struct V4DPI__Value_UOMPer) ;
		break ;
	      case V4DPI_PntType_UOMPUOM:
		memcpy(&nbls.blsk.KeyVal.Alpha,&point->Value.UOMPerVal,sizeof(struct V4DPI__Value_UOMPUOM)) ;
		nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = sizeof(union V4IS__KeyPrefix) + sizeof(struct V4DPI__Value_UOMPUOM) ;
		break ;
	      case V4DPI_PntType_Complex:
		memcpy(&nbls.blsk.KeyVal.Alpha,&point->Value.Complex,sizeof(struct V4DPI__Value_Complex)) ;
		nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = sizeof(union V4IS__KeyPrefix) + sizeof(struct V4DPI__Value_Complex) ;
		break ;
	      case V4DPI_PntType_GeoCoord:
		memcpy(&nbls.blsk.KeyVal.Alpha,&point->Value.GeoCoord,sizeof(struct V4DPI__Value_GeoCoord)) ;
		nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = sizeof(union V4IS__KeyPrefix) + sizeof(struct V4DPI__Value_GeoCoord) ;
		break ;
	      case V4DPI_PntType_BinObj:
		i = point->Bytes - V4DPI_PointHdr_Bytes ;		/* Number of bytes in object */
		if (i == 0 || i > V4IS_KeyBytes_Max-10) i =  V4IS_KeyBytes_Max-10 ;
		memcpy(&nbls.blsk.KeyVal.Alpha,&point->Value.AlphaVal,i) ;
		for(cx=i;cx<i+3;cx++) { nbls.blsk.KeyVal.Alpha[cx] = 0 ; } ;		/* Pad with nulls */
		i = ALIGN(i) ;
		nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = sizeof(union V4IS__KeyPrefix) + i ;
		break ;
	      CASEofChar
		i = point->Value.AlphaVal[0] ; if (i == 0 || i > V4IS_KeyBytes_Max-10) i =  V4IS_KeyBytes_Max-10 ;
		memcpy(&nbls.blsk.KeyVal.Alpha,&point->Value.AlphaVal[1],i) ;
		for(cx=i;cx<i+3;cx++) { nbls.blsk.KeyVal.Alpha[cx] = 0 ; } ;		/* Pad with nulls */
		i = ALIGN(i) ;
		nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = sizeof(union V4IS__KeyPrefix) + i ;
		break ;
	      case V4DPI_PntType_V4IS:
		memcpy(&nbls.blsk.KeyVal.Alpha,&point->Value,sizeof(struct V4DPI__PntV4IS)) ;
		nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = sizeof(struct V4DPI__PntV4IS) ;
		break ;
	      case V4DPI_PntType_AggRef:
		nbls.blsk.KeyVal.Int2Val[0] = point->Value.IntVal ;
		nbls.blsk.KeyVal.Int2Val[1] = point->Grouping ;
	    	nbls.blsk.KeyMode = V4IS_KeyMode_Int2 ; nbls.blsk.Bytes = V4IS_Int2Key_Bytes ;
		break ;
	      case V4DPI_PntType_TeleNum:		/* Only look at base number, ignore type & extension */
		nbls.blsk.KeyVal.Int2Val[0] = point->Value.Int2Val[0] ;
		nbls.blsk.KeyVal.Int2Val[1] = point->Value.Int2Val[1] ;
	    	nbls.blsk.KeyMode = V4IS_KeyMode_Int2 ; nbls.blsk.Bytes = V4IS_Int2Key_Bytes ;
		{ struct V4DPI__Value_Tele *tele = (struct V4DPI__Value_Tele *)&nbls.blsk.KeyVal.Int2Val[0] ;	/* If extended format then normalize back to old format */
		  if (tele->IntDialCode < 0) tele->IntDialCode = -tele->IntDialCode ;
		} ;
		break ;
	      case V4DPI_PntType_Int2:
	      case V4DPI_PntType_Fixed:
	      case V4DPI_PntType_Calendar:
	      case V4DPI_PntType_Real:
		nbls.blsk.KeyVal.Int2Val[0] = point->Value.Int2Val[0] ;
		nbls.blsk.KeyVal.Int2Val[1] = point->Value.Int2Val[1] ;
	    	nbls.blsk.KeyMode = V4IS_KeyMode_Int2 ; nbls.blsk.Bytes = V4IS_Int2Key_Bytes ;
		break ;
	    } ;
	   ctx->CtxVal[px].HaveNBL = FALSE ; ctx->CtxVal[px].nblCnt = 0 ; /* Assume no bindlist */
	   mmm = gpi->mmm ;
	   for(areax=0;areax<V4MM_Area_Max;areax++)			/* Search thru all areas with binds on this dimension */
	    { if (mmm->Areas[areax].abi == NULL) continue ;
	      abi = mmm->Areas[areax].abi ; aid = mmm->Areas[areax].AreaId ;
	      for(bx=0;bx<abi->DimCnt;bx++) { if (abi->DimsWithBind[bx] == ctx->DimHash[hx].Dim) break ; } ;
	      if (bx >= abi->DimCnt) continue ;			/* Nothing in this area */
	      ctx->ContextAddGetCount++ ;
#ifdef TRACEGRAB
printf("FrameAddDim %d %d\n",areax,mmm->Areas[areax].areaLock) ;
#endif
	      if (!GRABAREAMTLOCK(areax)) continue ;
	      i = (v4is_PositionKey(aid,(struct V4IS__Key *)&nbls.blsk,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) == V4RES_PosKey) ;	/* Check for binding */
	      if (!i)						/* Not found - can we try special for "dim:{all}" ? */
	       { if (ctx->DimHash[hx].di->AllValue != 0)	/*  only if allowed to! */
		  { nbls.blsk.KeyVal.IntVal = ctx->DimHash[hx].di->AllValue ;
		    nbls.blsk.KeyMode = V4IS_KeyMode_Int ; nbls.blsk.Bytes = V4IS_IntKey_Bytes ;
		    i = (v4is_PositionKey(aid,(struct V4IS__Key *)&nbls.blsk,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) == V4RES_PosKey) ;
		  } ;
	       } ;
	      if (i)							/* Did we get a binding record ? */
	       { 
	         ctx->CtxVal[px].HaveNBL = TRUE ;			/* Got a binding for this point! */
		 ctx->CtxVal[px].AreaIds[ctx->CtxVal[px].nblCnt] = aid ;	/* Save Area Id for use in IsctEval */
		 nblsptr = (struct V4DPI__BindListShell *)v4dpi_GetBindBuf(ctx,aid,ikde,TRUE,NULL) ;
		 if (nblsptr != NULL)				/* This should not ever be NULL but has been on occasion */
		  { nblrec = (struct V4DPI__BindList *)((char *)nblsptr + nbls.blsk.Bytes) ;
		    if (ctx->FnblCnt <= 0)
	             { ctx->CtxVal[px].nbl[ctx->CtxVal[px].nblCnt] = (struct V4DPI__BindList *)v4mm_AllocChunk(sizeof(struct V4DPI__BindList),FALSE) ;
	             } else
	             { ctx->CtxVal[px].nbl[ctx->CtxVal[px].nblCnt] = ctx->fnbl[--ctx->FnblCnt] ;
	             } ;
	            memcpy(ctx->CtxVal[px].nbl[ctx->CtxVal[px].nblCnt++],nblrec,nblrec->Bytes) ;/* Copy into local buffer */
	            if (iscur)					/* If this is now a "current" point then add to list */
	             { for(i=0;i<ctx->EvalListCnt;i++) { if (ctx->EvalListDimIndexes[i] == hx) break ; } ;
		       ctx->EvalListDimIndexes[i] = hx ; if (i >= ctx->EvalListCnt) ctx->EvalListCnt ++ ;
	             } ;
		    if (vdx != UNUSED) { vdx++ ; vdaid = aid ; } ;
		  } ;
	       } ;
	      FREEAREAMTLOCK(areax) ;
	     } ;
	  if (vdx == 1)						/* If got only 1 binding then maybe cache */
	   { if (nblrec->Bytes <= V4CTX_DimNBLCacheBytes)
	      { 
		vdx = (nbls.blsk.AuxVal + point->Value.IntVal) % V4CTX_DimNBLCacheMax ; if (vdx < 0) vdx = -vdx ;
		vdnc.Entry[vdx].aid = vdaid ; vdnc.Entry[vdx].Dim = nbls.blsk.AuxVal ;
		vdnc.Entry[vdx].Key = point->Value.IntVal ;
		if (vdnc.Entry[vdx].nbl == NULL) vdnc.Entry[vdx].nbl = (struct V4DPI__BindList *)v4mm_AllocChunk(V4CTX_DimNBLCacheBytes,FALSE) ;
		memcpy(vdnc.Entry[vdx].nbl,nblrec,nblrec->Bytes) ; ctx->CtxnblCachePuts++ ;
		vdnc.HasEntries = TRUE ;
	      } ;
	   } ;
	  if (!ctx->CtxVal[px].HaveNBL)				/* If no binding for this dim - add to loser list */
	   {
	     if (point->Bytes <= sizeof(struct V4DPI__LittlePoint))
	      { chx = point->Dim + point->Value.IntVal ; chx = chx & V4C_Cache_Mask ;
		memcpy(&ctx->Cache[chx].lpt,point,point->Bytes) ; ctx->LCacheDirty = TRUE ;
	      } ;
	   } ;
skip_cause_got_cache: ;
	 } ;
	ctx->Frame[fx].FrameDimEntry[ex].CtxValIndex2 = px ;		/* Link to point within this frame */
	ctx->Frame[fx].FrameDimEntry[ex].DimHashIndex = hx ;
	return(TRUE) ;
}

/*	v4ctx_FrameBaAInfo() - Returns pointer to current BaA structure (NULL if none) */

struct V4IM__BaA *v4ctx_FrameBaAInfo(ctx)
  struct V4C__Context *ctx ;
{ int i ;

	for(i=ctx->FrameCnt;i>=0;i--)
	 { if (ctx->Frame[i].baf != NULL) return(ctx->Frame[i].baf) ;
	 } ;
	return(NULL) ;				/* No BaA found - return NULL */
}

/*	v4ctx_FrameBaASet - Allocates new BaA structure & associates with current frame */
/*	Call: baf = v4ctx_FrameBaASet( ctx , nestRCV )
	  where baf is current/new baf pointer,
		ctx is current context,
		nestRCV is TRUE to see if already have nested baa, FALSE to allocate new one always,
		lvlMax is number of recapping levels,
		colMax is max number of columns to track,
		calcMax is max number of 'calc' elements to track */

struct V4IM__BaA *v4ctx_FrameBaASet(ctx,nestRCV,lvlMax,colMax,calcMax)
  struct V4C__Context *ctx ;
  LOGICAL nestRCV ;
  LENMAX lvlMax,colMax,calcMax ;
{ struct V4IM__BaA *baf ;
  INDEX i ;
#ifdef NEWBAFSTUFF
  LENMAX ccInfoBytes, ccTotalBytes ;
#endif

	if (nestRCV)
	 { if ((baf = v4ctx_FrameBaAInfo(ctx)) != NULL) return(baf) ;
	 } ;
	i = ctx->FrameCnt-1 ;			/* i = current frame index (s/b one less than current) */
	if (ctx->Frame[i].baf != NULL) return(ctx->Frame[i].baf) ;
	baf = (ctx->Frame[i].baf = (struct V4IM__BaA *)v4mm_AllocChunk(sizeof *ctx->Frame[i].baf,TRUE)) ;
#ifdef NEWBAFSTUFF
	baf->baaColMax = colMax ; baf->baaCalcMax = calcMax ; baf->baaLevelMax = lvlMax ;
	ccInfoBytes = sizeof(baf->ccInfo->colCalc[0]) * (baf->baaColMax + baf->baaCalcMax) ;
	ccTotalBytes = sizeof(baf->ccTot[0]->colCalc[0]) * (baf->baaColMax + baf->baaCalcMax) ;
	baf->ccInfo = (struct V4IM__BaAColCalcInfo *)v4mm_AllocChunk(ccInfoBytes + baf->baaLevelMax * ccTotalBytes,TRUE) ;
	baf->ccTot[0] = (struct V4IM__BaATotals*)((char *)baf->ccInfo + ccInfoBytes) ;
	for(i=1;i<baf->baaLevelMax;i++)
	 { baf->ccTot[i] = (struct V4IM__BaATotals *)(((char *)baf->ccTot[i-1]) + ccTotalBytes) ; } ;
	for(i=0;i<baf->baaColMax+baf->baaCalcMax;i++)
	 { baf->ccInfo->colCalc[i].dim = UNUSED ;  baf->ccInfo->colCalc[i].decimals = UNUSED ;  baf->ccInfo->colCalc[i].ref = UNUSED ;  baf->ccInfo->colCalc[i].index = UNUSED ;
	 } ;
#else
	for(j=0;j<V4IM_BaA_ColMax;j++)
	 { baf->Col[j].Dim = UNUSED ; baf->Col[j].Decimals = UNUSED ; baf->Col[j].Ref = UNUSED ;
	   baf->Col[j].Index = UNUSED ;
	 } ;
#endif
	return(ctx->Frame[i].baf) ;


}

/*	v4ctx_DimValue - Returns Point associated with dimension in current context */
/*	Call: point = v4ctx_DimValue( ctx , dimid , diUpd )
	  where point is pointer to point value (NULL if dimension not defined in context),
		ctx is context,
		dimid is dimension ID,
		diUpd if not NULL is updated to dimension info structure	*/

struct V4DPI__Point *v4ctx_DimValue(ctx,dimid,diUpd)
  struct V4C__Context *ctx ;
  struct V4DPI__DimInfo **diUpd ;
  int dimid ;
{
  struct V4DPI__Point isct,*pt ;
  struct V4DPI__DimInfo *di ;
#ifdef V4ENABLEMULTITHREADS
  struct V4C__Context *cctx ;
#endif
  static struct V4DPI__Point dimpt ;
  int hx,i ;

#ifdef V4ENABLEMULTITHREADS
	for(cctx=ctx;cctx!=NULL;cctx=cctx->pctx)
	 { hx = DIMIDHASH(dimid) ;					/* Hash to get dimension slot */
	   for(;;)
	    { if (cctx->DimHash[hx].Dim == dimid)
	       { i = cctx->DimHash[hx].CtxValIndex ;
	         if (i != -1)						/* Dimension currently undefined */
	          { if (cctx->CtxVal[i].UnDefined) return(NULL) ;
	            if (diUpd != NULL) *diUpd = cctx->DimHash[hx].di ;
	            return(&cctx->CtxVal[i].Point) ;
	          } ;
	         if (cctx->DimHash[hx].di->AutoCtxPnt.Bytes == 0) return(NULL) ; /* Dim not defined & no auto-cctx */
	         break ;
	       } ;
	      if (cctx->DimHash[hx].Dim == 0) break ;
	      hx++ ; hx = DIMIDHASH(hx) ;				/* Slot busy - try another */
	    } ;
	 } ;
#else
	hx = DIMIDHASH(dimid) ;						/* Hash to get dimension slot */
	for(;;)
	 { if (ctx->DimHash[hx].Dim == dimid)
	    { i = ctx->DimHash[hx].CtxValIndex ;
	      if (i != -1)						/* Dimension currently undefined */
	       { if (ctx->CtxVal[i].UnDefined) return(NULL) ;
	         if (diUpd != NULL) *diUpd = ctx->DimHash[hx].di ;
	         return(&ctx->CtxVal[i].Point) ;
	       } ;
	      if (ctx->DimHash[hx].di->AutoCtxPnt.Bytes == 0) return(NULL) ; /* Dim not defined & no auto-ctx */
	      break ;
	    } ;
	   if (ctx->DimHash[hx].Dim == 0) break ;
	   hx++ ; hx = DIMIDHASH(hx) ;					/* Slot busy - try another */
	 } ;
#endif
/*	Point not in context - maybe we can figure it out from AutoContext attribute */
	DIMINFO(di,ctx,dimid) ;
	if (di == NULL) return(NULL) ;
	if (di->AutoCtxPnt.Bytes == 0) return(NULL) ;		/* No AutoContext for this dimension */
/*	Set up point Dim=dim */
	ZPH(&dimpt) ; dimpt.Dim = Dim_Dim ; dimpt.Value.IntVal = dimid ;
	dimpt.Bytes = V4PS_Int ; dimpt.PntType = V4DPI_PntType_Dict ;
/*	Now build up intersection [auto-point Dim=dim] */
//	ZPH(&isct) ; isct.PntType = V4DPI_PntType_Isct ;
	INITISCT(&isct) ; NOISCTVCD(&isct) ;
	pt = ISCT1STPNT(&isct) ;
	 memcpy(pt,&di->AutoCtxPnt,di->AutoCtxPnt.Bytes) ;
	  ADVPNT(pt) ;
	memcpy(pt,&dimpt,dimpt.Bytes) ; ADVPNT(pt) ;
	isct.Grouping = 2 ; ISCTLEN(&isct,pt) ;
	if (traceGlobal & V4TRACE_AutoContext)
	 { 
//	   v4dpi_PointToString(UCTBUF1,&isct,ctx,-1) ; printf("V4-Trace-AutoContext: %s\n",tbuf) ;
	   v_Msg(ctx,UCTBUF1,"@*AutoContext: %1P\n",&isct) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
	 } ;
	if (v4dpi_IsctEval(&dimpt,&isct,ctx,0,NULL,NULL) == NULL)
	 { 
//	   if (traceGlobal & V4TRACE_AutoContext) printf("V4-Trace-AutoContext: IsctEval Failed\n") ;
	   if (traceGlobal & V4TRACE_AutoContext) { v_Msg(ctx,UCTBUF1,"@*AutoContext: IsctEval Failed\n") ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
	   return(NULL) ; /* Could not eval ? */
	 } ;
	if (dimpt.PntType == V4DPI_PntType_Special && dimpt.Grouping == V4DPI_Grouping_Undefined) return(NULL) ;
	dimpt.Dim = dimid ;				/* Got a result - force in wanted dimension */
	if (diUpd != NULL) *diUpd = NULL ;
	return(&dimpt) ;				/* Return pointer to it */
}

/*	v4ctx_DimPValue - Returns Point associated with dimension in prior context (i.e. one "current context" back) */
/*	Call: point = v4ctx_DimPValue( ctx , dimid )
	  where point is pointer to point value (NULL if dimension not defined in context),
		ctx is context,
		dimid is dimension ID					*/

struct V4DPI__Point *v4ctx_DimPValue(ctx,dimid)
  struct V4C__Context *ctx ;
  int dimid ;
{
  int hx,i,j ;

#ifdef V4ENABLEMULTITHREADS	
	hx = DIMIDHASH(dimid) ;						/* Hash to get dimension slot */
	for(;;)
	 { if (ctx->DimHash[hx].Dim == dimid)
	    { i = ctx->DimHash[hx].CtxValIndex ;
	      if (i != -1)
	       { j = ctx->CtxVal[i].PriorCtxValIndex ;			/* Look for prior frame */
		 if (j != -1)
	          { if (ctx->CtxVal[j].UnDefined) return(NULL) ;	/* Got in prior - is it explicitly undefined? */
		    return(&ctx->CtxVal[j].Point) ;			/* Return prior frame value */
		  } else
		  { if (ctx->CtxVal[i].UnDefined) return(NULL) ;	/* No prior frame - try to return in this frame */
		    if (ctx->pctx != NULL)				/* Do we have prior context? */
		     { struct V4DPI__Point *cpt ;
		       cpt = v4ctx_DimValue(ctx->pctx,dimid,NULL) ;	/*  yes - search it */
		       if (cpt != NULL) return(cpt) ;
		     } ;
		    return(&ctx->CtxVal[i].Point) ;
		  } ;
	       } ;
	      break ;
	    } ;
	   if (ctx->DimHash[hx].Dim == 0) break ;
	   hx++ ; hx = DIMIDHASH(hx) ;					/* Slot busy - try another */
	 } ;
/*	Point not in context. If prior context then try to return prior value from it */
	return(ctx->pctx == NULL ? NULL : v4ctx_DimPValue(ctx->pctx,dimid)) ;
#else
	hx = DIMIDHASH(dimid) ;						/* Hash to get dimension slot */
	for(;;)
	 { if (ctx->DimHash[hx].Dim == dimid)
	    { i = ctx->DimHash[hx].CtxValIndex ;
	      if (i != -1)
	       { j = ctx->CtxVal[i].PriorCtxValIndex ;			/* Look for prior frame */
		 if (j != -1)
	          { if (ctx->CtxVal[j].UnDefined) return(NULL) ;	/* Got in prior - is it explicitly undefined? */
		    return(&ctx->CtxVal[j].Point) ;			/* Return prior frame value */
		  } else
		  { if (ctx->CtxVal[i].UnDefined) return(NULL) ;	/* No prior frame - try to return in this frame */
		    return(&ctx->CtxVal[i].Point) ;
		  } ;
	       } ;
	      break ;
	    } ;
	   if (ctx->DimHash[hx].Dim == 0) break ;
	   hx++ ; hx = DIMIDHASH(hx) ;					/* Slot busy - try another */
	 } ;
/*	Point not in context */
	return(NULL) ;
#endif
}

/*	v4ctx_DimBaseDim - Returns dimension of base if this point added via an "isa" link */
/*	Call: dim = v4ctx_DimBaseDim( ctx , dimid , trace )
	  where dim is dimension of base, 0 if none,
		ctx is context,
		dimid is dimension ID,
		trace is TRUE to provide trace if doing auto-context lookup	*/

int v4ctx_DimBaseDim(ctx,dimid,trace)
  struct V4C__Context *ctx ;
  int dimid,trace ;
{
  int hx ;

	hx = DIMIDHASH(dimid) ;				/* Hash to get dimension slot */
	for(;;)
	 { if (ctx->DimHash[hx].Dim == dimid) break ;
	   if (ctx->DimHash[hx].Dim == 0) break ;
	   hx++ ; hx = DIMIDHASH(hx) ;			/* Slot busy - try another */
	 } ;
	if (ctx->DimHash[hx].Dim != 0)			/* No such dimension in context */
	 { if (ctx->DimHash[hx].CtxValIndex != -1)	/* If got current point - return BaseDim */
	    return(ctx->CtxVal[ctx->DimHash[hx].CtxValIndex].BaseDim) ;
	 } ;
	return(0) ;					/* Dim not in context - return 0 */
}

/*	v4ctx_DimHasBindInfo - Returns TRUE if point has binding info associated with it	*/
/*	Call: logical = v4ctx_DimHasBindInfo( ctx , dimid , trace )
	  where logical is returned value
		ctx is context,
		dimid is dimension ID,
		trace is TRUE to provide trace if doing auto-context lookup	*/

LOGICAL v4ctx_DimHasBindInfo(ctx,dimid,trace)
  struct V4C__Context *ctx ;
  int dimid,trace ;
{
  int hx ;

	hx = DIMIDHASH(dimid) ;				/* Hash to get dimension slot */
	for(;;)
	 { if (ctx->DimHash[hx].Dim == dimid) break ;
	   if (ctx->DimHash[hx].Dim == 0) break ;
	   hx ++ ; hx = DIMIDHASH(hx) ;					/* Slot busy - try another */
	 } ;
	if (ctx->DimHash[hx].Dim != 0)					/* No such dimension in context */
	 { if (ctx->DimHash[hx].CtxValIndex != -1)			/* Dimension currently undefined */
	    { return(ctx->CtxVal[ctx->DimHash[hx].CtxValIndex].nblCnt > 0) ; } ;
	 } ;
	return(FALSE) ;							/* Not in context */
}

/*	v4ctx_Resynch - Re-synchronizes context after binds/mass-update	*/
/*	Call: v4ctx_Resynch( ctx )					*/

void v4ctx_Resynch(ctx)
  struct V4C__Context *ctx ;
{ int i ;

	for(i=0;i<V4C_CtxDimHash_Max;i++)
	 { if (ctx->DimHash[i].Dim == 0) continue ;
	   if (ctx->DimHash[i].CtxValIndex == -1) continue ;	/* Dimension currently undefined */
	   v4ctx_HasHaveNBL(ctx,ctx->DimHash[i].Dim) ;		/* Synch up this dimension */
	 } ;
}

/*	v4ctx_HasHaveNBL - Tests HaveNBL flag for point in context	*/
/*	Call: value = v4ctx_HasHaveNBL( ctx , dimid )
	  where value is -1 (not in context), 0 (in context, HaveNBL not set), 1(in context, & set),
		ctx is current context,
		dimid is dimension to tweak				*/

int v4ctx_HasHaveNBL(ctx,dimid)
  struct V4C__Context *ctx ;
  int dimid ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4DPI__BindList *nblrec ;
  struct V4DPI__BindListShell nbls,*nblsptr ;
  struct V4DPI__AreaBindInfo *abi ;
  INDEX areax ; int bx,i ;
  int hx,px,cx,cnt ;

	hx = DIMIDHASH(dimid) ;						/* Hash to get dimension slot */
	for(;;)
	 { if (ctx->DimHash[hx].Dim == dimid) break ;
	   if (ctx->DimHash[hx].Dim == 0) break ;
	   hx++ ; hx = DIMIDHASH(hx) ;					/* Slot busy - try another */
	 } ;
	if (ctx->DimHash[hx].Dim == 0)					/* No such dimension in context */
	 return(-1) ;
	   for(px=ctx->DimHash[hx].CtxValIndex,cnt=0;px>=0;px=ctx->CtxVal[px].PriorCtxValIndex,cnt++)
	    { if (cnt > 100)
	       v4_error(0,0,"CTX","HHNBL","ENDLESSLOOP","Appear to be stuck hx=%d, initial px=%d, px=%d",
			hx,ctx->DimHash[hx].CtxValIndex,px) ;
	      if (ctx->DimHash[hx].di == NULL ? FALSE : ctx->DimHash[hx].di->BindList) /* Look for bindlist for this point ? */
	       { nbls.blsk.KeyType = V4IS_KeyType_Binding ;
		 nbls.blsk.AuxVal = ctx->DimHash[hx].Dim ;
		 switch (ctx->CtxVal[px].Point.PntType)
		  { default:
			nbls.blsk.KeyVal.IntVal = ctx->CtxVal[px].Point.Value.IntVal ;
			nbls.blsk.KeyMode = V4IS_KeyMode_Int ; nbls.blsk.Bytes = V4IS_IntKey_Bytes ;
			break ;
		    case V4DPI_PntType_UOM:
			memcpy(&nbls.blsk.KeyVal.Alpha,&ctx->CtxVal[px].Point.Value.UOMVal,sizeof(struct V4DPI__Value_UOM)) ;
			nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = sizeof(union V4IS__KeyPrefix) + sizeof(struct V4DPI__Value_UOM) ;
			break ;
		    case V4DPI_PntType_UOMPer:
			memcpy(&nbls.blsk.KeyVal.Alpha,&ctx->CtxVal[px].Point.Value.UOMPerVal,sizeof(struct V4DPI__Value_UOMPer)) ;
			nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = sizeof(union V4IS__KeyPrefix) + sizeof(struct V4DPI__Value_UOMPer) ;
			break ;
		    case V4DPI_PntType_UOMPUOM:
			memcpy(&nbls.blsk.KeyVal.Alpha,&ctx->CtxVal[px].Point.Value.UOMPerVal,sizeof(struct V4DPI__Value_UOMPUOM)) ;
			nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = sizeof(union V4IS__KeyPrefix) + sizeof(struct V4DPI__Value_UOMPUOM) ;
			break ;
		    case V4DPI_PntType_Complex:
			memcpy(&nbls.blsk.KeyVal.Alpha,&ctx->CtxVal[px].Point.Value.Complex,sizeof(struct V4DPI__Value_Complex)) ;
			nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = sizeof(union V4IS__KeyPrefix) + sizeof(struct V4DPI__Value_Complex) ;
			break ;
		    case V4DPI_PntType_GeoCoord:
			memcpy(&nbls.blsk.KeyVal.Alpha,&ctx->CtxVal[px].Point.Value.GeoCoord,sizeof(struct V4DPI__Value_GeoCoord)) ;
			nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = sizeof(union V4IS__KeyPrefix) + sizeof(struct V4DPI__Value_GeoCoord) ;
			break ;
		   case V4DPI_PntType_BinObj:
			i = ctx->CtxVal[px].Point.Bytes - V4DPI_PointHdr_Bytes ;		/* Number of bytes in object */
			if (i == 0 || i > V4IS_KeyBytes_Max-10) i =  V4IS_KeyBytes_Max-10 ;
			memcpy(&nbls.blsk.KeyVal.Alpha,&ctx->CtxVal[px].Point.Value.AlphaVal,i) ;
			for(cx=i;cx<i+3;cx++) { nbls.blsk.KeyVal.Alpha[cx] = 0 ; } ;		/* Pad with nulls */
			i = ALIGN(i) ;
			nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = sizeof(union V4IS__KeyPrefix) + i ;
			break ;
		    CASEofChar
			i = ctx->CtxVal[px].Point.Value.AlphaVal[0] ;
			if (i == 0 || i > V4IS_KeyBytes_Max-10) i =  V4IS_KeyBytes_Max-10 ;
			strncpy(nbls.blsk.KeyVal.Alpha,&ctx->CtxVal[px].Point.Value.AlphaVal[1],i) ;
			for(cx=i;cx<i+3;cx++) { nbls.blsk.KeyVal.Alpha[cx] = 0 ; } ;		/* Pad with nulls */
			i = ALIGN(i) ;
			nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = sizeof(union V4IS__KeyPrefix) + i ;
			break ;
		   case V4DPI_PntType_V4IS:
			memcpy(&nbls.blsk.KeyVal.Alpha,&ctx->CtxVal[px].Point.Value,sizeof(struct V4DPI__PntV4IS)) ;
			nbls.blsk.KeyMode = V4IS_KeyMode_Alpha ; nbls.blsk.Bytes = sizeof(struct V4DPI__PntV4IS) ;
			break ;
		    case V4DPI_PntType_TeleNum:
			nbls.blsk.KeyVal.Int2Val[0] = ctx->CtxVal[px].Point.Value.Int2Val[0] ;
			nbls.blsk.KeyVal.Int2Val[1] = ctx->CtxVal[px].Point.Value.Int2Val[1] ;
			nbls.blsk.KeyMode = V4IS_KeyMode_Int2 ; nbls.blsk.Bytes = V4IS_Int2Key_Bytes ;
			{ struct V4DPI__Value_Tele *tele = (struct V4DPI__Value_Tele *)&nbls.blsk.KeyVal.Int2Val[0] ;	/* If extended format then normalize back to old format */
			  if (tele->IntDialCode < 0) tele->IntDialCode = -tele->IntDialCode ;
			} ;
			break ;
		    case V4DPI_PntType_Int2:
		    case V4DPI_PntType_Fixed:
		    case V4DPI_PntType_Calendar:
		    case V4DPI_PntType_Real:
			nbls.blsk.KeyVal.Int2Val[0] = ctx->CtxVal[px].Point.Value.Int2Val[0] ;
			nbls.blsk.KeyVal.Int2Val[1] = ctx->CtxVal[px].Point.Value.Int2Val[1] ;
			nbls.blsk.KeyMode = V4IS_KeyMode_Int2 ; nbls.blsk.Bytes = V4IS_Int2Key_Bytes ;
			break ;
		  } ;
	         for(i=0;i<ctx->CtxVal[px].nblCnt;i++)
		  { if (ctx->FnblCnt < V4C_FreeNblListMax-1) ctx->fnbl[ctx->FnblCnt++] = ctx->CtxVal[px].nbl[i] ;
		    ctx->CtxVal[px].nbl[i] = NULL ;
		  } ;
		 ctx->CtxVal[px].HaveNBL = FALSE ; ctx->CtxVal[px].nblCnt = 0 ; /* Assume no bindlist */
		 mmm = gpi->mmm ;
		 for(areax=0;areax<V4MM_Area_Max;areax++)			/* Search thru all areas with binds on this dimension */
		  { if (mmm->Areas[areax].abi == NULL) continue ;
		    abi = mmm->Areas[areax].abi ;
		    for(bx=0;bx<abi->DimCnt;bx++) { if (abi->DimsWithBind[bx] == ctx->DimHash[hx].Dim) break ; } ;
		    if (bx >= abi->DimCnt) continue ;			/* Nothing in this area */
		    ctx->ContextAddGetCount++ ;
#ifdef TRACEGRAB
printf("HasHaveNBL %d %d\n",areax,mmm->Areas[areax].areaLock) ;
#endif
		    if (!GRABAREAMTLOCK(areax)) continue ;
		    i = (v4is_PositionKey(mmm->Areas[areax].AreaId,(struct V4IS__Key *)&nbls.blsk,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) == V4RES_PosKey) ;	/* Check for binding */
		    if (!i)						/* Not found - can we try special for "dim:{all}" ? */
		     { if (ctx->DimHash[hx].di->AllValue != 0)	/*  only if allowed to! */
			{ nbls.blsk.KeyVal.IntVal = ctx->DimHash[hx].di->AllValue ;
			  nbls.blsk.KeyMode = V4IS_KeyMode_Int ; nbls.blsk.Bytes = V4IS_IntKey_Bytes ;
			  i = (v4is_PositionKey(mmm->Areas[areax].AreaId,(struct V4IS__Key *)&nbls.blsk,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) == V4RES_PosKey) ;
		        } ;
	             } ;
		    if (i)							/* Did we get a binding record ? */
		     { ctx->CtxVal[px].HaveNBL = TRUE ;			/* Got a binding for this point! */
		       ctx->CtxVal[px].AreaIds[ctx->CtxVal[px].nblCnt] = mmm->Areas[areax].AreaId ;	/* Save Area Id for use in IsctEval */
		       nblsptr = (struct V4DPI__BindListShell *)v4dpi_GetBindBuf(ctx,mmm->Areas[areax].AreaId,ikde,TRUE,NULL) ;
		       nblrec = (struct V4DPI__BindList *)((char *)nblsptr + nblsptr->blsk.Bytes) ;
		       if (ctx->FnblCnt <= 0)
			{ ctx->CtxVal[px].nbl[ctx->CtxVal[px].nblCnt] = (struct V4DPI__BindList *)v4mm_AllocChunk(sizeof(struct V4DPI__BindList),FALSE) ;
			} else
			{ ctx->CtxVal[px].nbl[ctx->CtxVal[px].nblCnt] = ctx->fnbl[--ctx->FnblCnt] ;
			} ;
		       memcpy(ctx->CtxVal[px].nbl[ctx->CtxVal[px].nblCnt++],nblrec,nblrec->Bytes) ;/* Copy into local buffer */
		       if (TRUE /*iscur*/)				/* If this is now a "current" point then add to list */
			{ for(i=0;i<ctx->EvalListCnt;i++) { if (ctx->EvalListDimIndexes[i] == hx) break ; } ;
			  ctx->EvalListDimIndexes[i] = hx ; if (i >= ctx->EvalListCnt) ctx->EvalListCnt ++ ;
			} ;
		     } ;
		    FREEAREAMTLOCK(areax) ;
		  } ;
	       } ;
	    } ;
	return(-1) ;							/* Point not in context */
}

/*	v4ctx_ExamineCtx - Formats Current Context			*/
/*	Call: v4ctx_ExamineCtx( ctx , usePrefix , stream )
	  where dststr is updated to string,
		ctx is context to be formatted,
		usePrefix is TRUE for V4E- prefix,
		stream is output stream to use (ex: VOUT_Trace)		*/

void v4ctx_ExamineCtx(ctx,usePrefix,stream)
  struct V4C__Context *ctx ;
  LOGICAL usePrefix ;
  int stream ;
{
  struct V4DPI__DimInfo *di ;
  int hx,px,gap,i,j ; UCCHAR msg[2048],buf[1024],*b,spaces[V4DPI_DimInfo_DimNameMax+1] ;
  FRAMEID frameId ; INDEX savertStackFail ;
//  struct V4DPI__LittlePoint lPt ;
  struct {
    int dimCount ;
    int maxDimLen ;
    struct {
      struct V4DPI__Point *ctxVal ;
      int Dim ;
      UCCHAR dimNameUpper[V4DPI_DimInfo_DimNameMax+1] ;
     } Entry[V4C_CtxDimHash_Max+1] ;
   } lcse ;


	lcse.dimCount = 0 ; lcse.maxDimLen = 0 ;
	for(hx=0;hx<V4C_CtxDimHash_Max;hx++)
	 { if (ctx->DimHash[hx].Dim == 0) continue ;			/* Empty slot */
	   di = (struct V4DPI__DimInfo *)v4dpi_DimInfoGet(ctx,ctx->DimHash[hx].Dim) ;
	   if (di == NULL) { v_Msg(ctx,NULL,"@*Dimension in slot %1d appears to be trashed\n",hx) ; vout_UCText(stream,0,ctx->ErrorMsg) ; continue ; } ;
	   if ((px = ctx->DimHash[hx].CtxValIndex) < 0) continue ;
#ifdef V4_BUILD_SECURITY
	   if (di->rtFlags & V4DPI_rtDimInfo_Hidden) continue ;
#endif
	   lcse.Entry[lcse.dimCount].Dim = di->DimId ; lcse.Entry[lcse.dimCount].ctxVal = &ctx->CtxVal[px].Point ;
	   UCcnvupper(lcse.Entry[lcse.dimCount].dimNameUpper,di->DimName,UCsizeof(lcse.Entry[lcse.dimCount].dimNameUpper)) ; lcse.dimCount++ ;
	   if (UCstrlen(di->DimName) > lcse.maxDimLen) lcse.maxDimLen = UCstrlen(di->DimName) ;
	 } ;
/*	Now sort the dimension names */
	for(gap=lcse.dimCount/2;gap>0;gap=gap/2)
	 { for(i=gap;i<lcse.dimCount;i++)
	    { for(j=i-gap;j>=0;j-=gap)
	       { if (UCstrcmp(lcse.Entry[j].dimNameUpper,lcse.Entry[j+gap].dimNameUpper) <= 0) break ;
	         lcse.Entry[V4C_CtxDimHash_Max] = lcse.Entry[j] ;
		 lcse.Entry[j] = lcse.Entry[j+gap] ;
		 lcse.Entry[j+gap] = lcse.Entry[V4C_CtxDimHash_Max] ;
	       } ;
	    } ;
	 } ;

	if(lcse.dimCount <= 0)
	 { vout_UCText(stream,0,UClit("*Context (no points)\n")) ; return ; } ;

	if (usePrefix) vout_UCText(stream,0,UClit("*Context-\n")) ;
	frameId = UNUSED ;
///*	If we have any [UV4:displayerCTX ...] bindings then put UV4:displayerCTX into context */
//	dictPNTv((P *)&lPt,Dim_UV4,v4im_GetEnumToDictVal(ctx,DE(DisplayerCTX),UNUSED)) ;
//	if (lPt.Value.IntVal == 0) { frameId = UNUSED ; }
//	 else { frameId = v4ctx_FramePush(ctx,NULL) ; v4ctx_FrameAddDim(ctx,0,(P *)&lPt,0,0) ; } ;
	savertStackFail = ctx->rtStackFail ;
	for(i=0;i<lcse.dimCount;i++)
	 { 
	   v4dpi_PointToStringML(buf,lcse.Entry[i].ctxVal,ctx,(V4DPI_FormatOpt_ShowDimAlways|V4DPI_FormatOpt_AlphaQuote|V4DPI_FormatOpt_NoDisplayer|V4DPI_FormatOpt_DisplayerTrace),512) ;
	   b = UCstrchr(buf,':') ; if (b == NULL) { b = UClit("") ; } else { *b = UCEOS ; b++ ; } ;
	   j = lcse.maxDimLen - UCstrlen(buf) ;
	   if (j < 0) { spaces[0] = UClit(' ') ; spaces[1] = UCEOS ; }
	    else { spaces[j ] = UCEOS ; for(j--;j>=0;j--) spaces[j] = UClit(' ') ; } ;
	   v_Msg(ctx,msg,"@%1U    %2U%3U : %4U\n",(usePrefix ? UClit("*") : UClit("")),spaces,buf,b) ;
/*	   If gpi->ved defined then also save this info */
	   
	   vout_UCText(stream,0,msg) ;
	 } ;
	ctx->rtStackFail = savertStackFail ;
	if (frameId != UNUSED) v4ctx_FramePop(ctx,frameId,NULL) ;

}

/*	L I S T   M O D U L E S					*/

/*	v4l_ListPoint_Modify - Modifies a list */
/*	result = v4l_ListPoint_Modify( ctx, listp , action , point , relhnum )
	  where result is logical TRUE (but may not equal TRUE) if OK, FALSE if a problem
	  	ctx is current context,
		listp is pointer to list,
		action is action to be performed (insert, append, init),
		point is point to be merged into list,
		trace is trace flag(s),
		relhnum is relative H num (0 if not known)	*/


ETYPE v4l_ListPoint_Modify(ctx,listp,action,point,relhnum)
  struct V4C__Context *ctx ;
  struct V4L__ListPoint *listp ;
  int action ;
  struct V4DPI__Point *point ;
  int relhnum ;
{ struct V4L__ListPointIndex lpix ;
  struct V4L__ListPoint_Jacket *lpj ;
  struct V4DPI__Point tp,*ipt ;
  struct V4DPI__Point *lp1 ;
  struct V4L__ListHugeInt *vlh ;
  struct V4L__BlockEntry *vbe ;
  struct V4L__ListHugeGeneric *vlhg ;
  struct V4L__BlockEntryGeneric *vbeg ;
  struct V4L__ListIntArray *lia ;
  struct V4L__ListDblArray *lda ;
  struct V4L_MultPoints *vmp ;
  struct V4DPI__DimInfo *di ;
  int i,ok,dim,bytes,aid ;

	if (listp->ListType == V4L_ListType_Isct)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"LPModType",V4DPI_PntType_Isct) ; return(V4L_ModRes_Fail) ; } ;
	if (point->Bytes > sizeof tp)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"ListPtLen",point,point->Bytes,sizeof tp) ; return(V4L_ModRes_Fail) ; } ;
/*	If this is the second entry then see if we can perform some optimizations- space wise */
	if (FALSE && listp->Entries == 1)
	 { lp1 = (P *)&listp->Buffer[0] ;	/* Link up to first point */
	   if (TRUE && lp1->Dim == point->Dim)
	    { switch (lp1->PntType)
	       { case V4DPI_PntType_Int:
	         case V4DPI_PntType_CodedRange:
		 case V4DPI_PntType_IMArg:
//	         case V4DPI_PntType_Hashed:
		 case V4DPI_PntType_OSHandle:
		 case V4DPI_PntType_UDT:
		 case V4DPI_PntType_UDate:
		 case V4DPI_PntType_UMonth:
		 case V4DPI_PntType_UPeriod:
		 case V4DPI_PntType_UWeek:
		 case V4DPI_PntType_UQuarter:
		 case V4DPI_PntType_Logical:
		 case V4DPI_PntType_Delta:
		 case V4DPI_PntType_UYear:
		 case V4DPI_PntType_Time:
		 case V4DPI_PntType_SSVal:
		 case V4DPI_PntType_AggRef:
//		 case V4DPI_PntType_AggVal:
		 case V4DPI_PntType_Country:
		 case V4DPI_PntType_SegBitMap:
		 case V4DPI_PntType_ParsedJSON:
		 case V4DPI_PntType_Color:
		 case V4DPI_PntType_XDict:
		 case V4DPI_PntType_Dict:
		 case V4DPI_PntType_Tree:
			memcpy(listp->Buffer,&lp1->Value.IntVal,sizeof lp1->Value.IntVal) ;	/* Get first value */
			listp->BytesPerEntry = sizeof lp1->Value.IntVal ;
			listp->Dim = point->Dim ; listp->PntType = point->PntType ;
			listp->LPIOffset = 0 ; listp->FFBufx = listp->BytesPerEntry ;
			break ;
		 case V4DPI_PntType_UTime:
		 case V4DPI_PntType_Fixed:
		 case V4DPI_PntType_Calendar:
		 case V4DPI_PntType_Real:
			GETREAL(listp->Buffer,lp1) ;
//			memcpy(listp->Buffer,&lp1->Value.RealVal,sizeof lp1->Value.RealVal) ;	/* Get first value */
			listp->BytesPerEntry = sizeof lp1->Value.RealVal ;
			listp->Dim = point->Dim ; listp->PntType = point->PntType ;
			listp->LPIOffset = 0 ; listp->FFBufx = listp->BytesPerEntry ;
			break ;
	       } ;
	    } ;
	 } ;
	switch (action)
	 { default: v4_error(V4E_INVLISTACTCODE,0,"V4DPI","ListPoint_Modify","INVLISTACTCODE","Invalid action code (%d)",action) ;
	   case V4L_ListAction_Insert:			/* Insert at begin of list */
		if (listp->BytesPerEntry > 0)
		 { memmove(&listp->Buffer[listp->BytesPerEntry],listp->Buffer,(listp->Entries * listp->BytesPerEntry)) ;
		   memcpy(listp->Buffer,&point->Value,listp->BytesPerEntry) ;
		   listp->FFBufx += listp->BytesPerEntry ;
		   listp->Entries += 1 ;
		   listp->Bytes = ALIGN( (char *)&listp->Buffer[listp->FFBufx] - (char *)listp ) ;
		 } else
		 { memcpy(&lpix.Offset[1],&listp->Buffer[listp->LPIOffset],listp->Entries*(sizeof lpix.Offset[0])) ;
		   lpix.Offset[0] = listp->FFBufx ;	/* Record position of point at begin of list */
		   if (listp->Dim > 0)			/* Are we saving entire point or just value? */
		    { lpj = (struct V4L__ListPoint_Jacket *)&listp->Buffer[listp->FFBufx] ;	/* Link up to "jacket" */
		      lpj->Bytes = point->Bytes-V4DPI_PointHdr_Bytes ;
		      memcpy(lpj->Buffer,&point->Value,lpj->Bytes) ;
		      listp->FFBufx += ALIGN(sizeof lpj->Bytes + lpj->Bytes) ;
		    } else
		    { memcpy(&listp->Buffer[listp->FFBufx],point,point->Bytes) ;
		      listp->FFBufx += ALIGN(point->Bytes) ;
		    } ;
		   listp->Entries += 1 ; listp->LPIOffset = listp->FFBufx ;
		   memcpy(&listp->Buffer[listp->LPIOffset],&lpix,listp->Entries*(sizeof lpix.Offset[0])) ;
		   listp->Bytes =
		      ALIGN((char *)&listp->Buffer[listp->LPIOffset+listp->Entries*(sizeof lpix.Offset[0])] - (char *)listp) ;
		 } ;
		break ;
	   case V4L_ListAction_AppendMult:			/* Just loop thru points and add */
		vmp = (struct V4L_MultPoints *)point ;
		for(ipt=(P *)vmp->pointBuf,i=0;i<vmp->pointCount;i++)
		 { if (!v4l_ListPoint_Modify(ctx,listp,V4L_ListAction_Append,ipt,relhnum)) return(V4L_ModRes_Fail) ;
		   ADVPNT(ipt) ;
		 } ;
		return(V4L_ModRes_OK) ;
	   case V4L_ListAction_AppendUniqueMult:		/* Just loop thru points and add */
		vmp = (struct V4L_MultPoints *)point ;
		for(ipt=(P *)vmp->pointBuf,i=0;i<vmp->pointCount;i++)
		 { if (!v4l_ListPoint_Modify(ctx,listp,V4L_ListAction_AppendUnique,ipt,relhnum)) return(V4L_ModRes_Fail) ;
		   ADVPNT(ipt) ;
		 } ;
		return(V4L_ModRes_OK) ;
	   case V4L_ListAction_AppendUnique:
		i = SIZEofLIST(listp) ;				/* i = number of points in list */
		if (i > 0)
		 { for(i;i>=1;i--)
		    { v4l_ListPoint_Value(ctx,listp,i,&tp) ;	/* Iterate thru existing points */
		      if (memcmp(&tp,point,point->Bytes) == 0) break ;
		    } ;
		   if (i >= 1) return(V4L_ModRes_DupPnt) ;		/* If we got a match then return else fall thru & append */
		 } ;
	   case V4L_ListAction_Append:			/* Append at end of list */
append_again:
		if (listp->ListType == V4L_ListType_HugeInt)
		 { 
		   struct V4MM__MemMgmtMaster *mmm = gpi->mmm ;
		   vlh = (struct V4L__ListHugeInt *)&listp->Buffer[0] ;
		   if (vlh->Block[vlh->Blocks-1].ElCount <
			 ((listp->PntType == V4DPI_PntType_Real || listp->PntType == V4DPI_PntType_Int2 || listp->PntType == V4DPI_PntType_Calendar) ? V4L_BlockEntry_DblMax :V4L_BlockEntry_IntMax))	/* Get last block (unless full) */
		    { vbe = v4l_Getvbe(ctx,vlh->RelHNum,vlh->Block[vlh->Blocks-1].BlockValueNum) ; }
		    else { vbe = v4l_Getvbe(ctx,vlh->RelHNum,0) ;
			   if (vlh->Blocks >= V4L_ListHuge_BlockMax) { v_Msg(ctx,ctx->ErrorMsgAux,"LPModHIMax") ; return(V4L_ModRes_Fail) ; } ;
			   vlh->Block[vlh->Blocks].ElCount = 0 ; vlh->Block[vlh->Blocks].BlockValueNum = vbe->ValueNum ;
			   vlh->Blocks++ ;
			 } ;
if (vbe == NULL)
 { printf("? vbe is null?? vlh->Blocks=%d, ElCount=%d, BlockValueNum=%d\n",vlh->Blocks,vlh->Block[vlh->Blocks-1].ElCount,vlh->Block[vlh->Blocks-1].BlockValueNum) ;
   vbe = v4l_Getvbe(ctx,vlh->RelHNum,vlh->Block[vlh->Blocks-1].BlockValueNum) ;
   return(V4L_ModRes_Fail) ;
 } ;
		   if (listp->PntType == V4DPI_PntType_Real || listp->PntType == V4DPI_PntType_Int2 || listp->PntType == V4DPI_PntType_Calendar)
		    { lda = (struct V4L__ListDblArray *)&vbe->PointValBuf ;
		      if (point->Bytes == V4PS_Real)
		       { GETREAL(lda->PointDblVal[vbe->EntryCount++],point) ;
//		         memcpy(&lda->PointDblVal[vbe->EntryCount++],&point->Value.RealVal,sizeof(double)) ;
		       } else
		       { double tdbl = v4im_GetPointDbl(&ok,point,ctx) ; if (!ok) return(V4L_ModRes_Fail) ;
		         memcpy(&lda->PointDblVal[vbe->EntryCount++],&tdbl,sizeof(double)) ;
		       } ;
		    } else
		    { lia = (struct V4L__ListIntArray *)&vbe->PointValBuf ;
if (vbe->EntryCount >= V4L_BlockEntry_IntMax)
 { printf("vbe->EntryCount is %d, blocks=%d, ElCount=%d\n",vbe->EntryCount,vlh->Blocks,vlh->Block[vlh->Blocks-1].ElCount) ;
   return(V4L_ModRes_Fail) ;
 } ;
		      if (point->PntType == V4DPI_PntType_AggRef)
		       { if (point->Value.IntVal > 0xffffff)		/* Slimy code to embed grouping in IntVal */
		           { v_Msg(ctx,ctx->ErrorMsgAux,"LPModHIAggMax") ; return(V4L_ModRes_Fail) ; } ;
			 lia->PointIntVal[vbe->EntryCount++] = point->Value.IntVal | (point->Grouping << 24) ;
		       } else
		       { lia->PointIntVal[vbe->EntryCount++] = point->Value.IntVal ; } ;
		    } ;
		   vlh->TotalElCount++ ; vlh->Block[vlh->Blocks-1].ElCount++ ;

		   if (!mmm->Areas[aid = (vlh->RelHNum == 0 ? gpi->HighHNum : gpi->RelH[vlh->RelHNum].aid)].WriteAccess)
		    { v_Msg(ctx,ctx->ErrorMsgAux,"LPModAreaRO",mmm->Areas[aid].UCFileName) ; return(V4L_ModRes_Fail) ; } ;

		   v4l_Putvbe(ctx,vlh->RelHNum,vbe->ValueNum,FALSE) ;
		   listp->Entries ++ ;
		   break ;
		 } ;
		if (listp->ListType == V4L_ListType_HugeGeneric)
		 { vlhg = (struct V4L__ListHugeGeneric *)&listp->Buffer[0] ;
		   vbeg = v4l_Getvbeg(ctx,vlhg->RelHNum,vlhg->Block[vlhg->Blocks-1].BlockValueNum) ;
		   if (vbeg == NULL) return(V4L_ModRes_Fail) ;
		   for(ipt=(P*)&vbeg->PointValBuf,bytes=0,i=1;i<=vbeg->EntryCount;i++)	/* Position to end of vbeg */
		    { bytes += ipt->Bytes ; ADVPNT(ipt) ; } ;
		   if (bytes + point->Bytes >= V4L_BlockEntryGeneric_Max)	/* Have to allocate new vbeg? */
		    { vbeg = v4l_Getvbeg(ctx,vlhg->RelHNum,0) ;
		      if (vbeg == NULL) return(V4L_ModRes_Fail) ;
		      if (vlhg->Blocks >= V4L_ListHuge_BlockMax)
		       { v_Msg(ctx,ctx->ErrorMsgAux,"LPModHGMax") ; return(V4L_ModRes_Fail) ; } ;
		      vlhg->Block[vlhg->Blocks].ElCount = 0 ; vlhg->Block[vlhg->Blocks].BlockValueNum = vbeg->ValueNum ;
		      vlhg->Blocks++ ; ipt = (P *)&vbeg->PointValBuf ;
		    } ;
		   vbeg->EntryCount++ ; memcpy(ipt,point,point->Bytes) ;	/* Copy point into vbeg */
		   vlhg->TotalElCount++ ; vlhg->Block[vlhg->Blocks-1].ElCount++ ;
		   v4l_Putvbeg(ctx,vlhg->RelHNum,vbeg->ValueNum,FALSE) ;
		   listp->Entries ++ ;
		   break ;
		 } ;
		if (listp->BytesPerEntry > 0)
		 { if (listp->FFBufx + listp->BytesPerEntry >= V4L_ListPoint_BufMax)
		    {
		      if (relhnum == 0)
		       { for(relhnum=gpi->HighHNum;relhnum>=gpi->LowHNum;relhnum--)	/* Find highest area allowing new entries */
			  { if (gpi->RelH[relhnum].aid == UNUSED) continue ; if (relhnum == V4DPI_WorkRelHNum) continue ;
			    if (gpi->RelH[relhnum].ahi.BindingsUpd) break ;
			  } ;
			 if (relhnum < gpi->LowHNum && gpi->RelH[V4DPI_WorkRelHNum].aid != UNUSED)
			  relhnum = V4DPI_WorkRelHNum ;			/* Assign to work area as last resort */
		       } ;
		      vbe = v4l_Getvbe(ctx,relhnum,0) ;		/* Have to convert to huge-list format */
		      for(i=1;;i++)
		       { if (!v4l_ListPoint_Value(ctx,listp,i,&tp)) break ;	/* Append current (old format) list to huge format */
			 if (listp->PntType == V4DPI_PntType_Real || listp->PntType == V4DPI_PntType_Int2 || listp->PntType == V4DPI_PntType_Calendar)
			  { lda = (struct V4L__ListDblArray *)&vbe->PointValBuf ;
//			    memcpy(&lda->PointDblVal[vbe->EntryCount++],&tp.Value.RealVal,sizeof(double)) ;
			    if (point->Bytes == sizeof(double))
			     { GETREAL(lda->PointDblVal[vbe->EntryCount++],&tp) ;
//			       memcpy(&lda->PointDblVal[vbe->EntryCount++],&tp.Value.RealVal,sizeof(double)) ;
			     } else
			     { double tdbl = v4im_GetPointDbl(&ok,&tp,ctx) ; if (!ok) return(V4L_ModRes_Fail) ;
			       SETDBL(lda->PointDblVal[vbe->EntryCount++],tdbl) ;
			     } ;
			  } else
			  { lia = (struct V4L__ListIntArray *)&vbe->PointValBuf ;
			    lia->PointIntVal[vbe->EntryCount++] = tp.Value.IntVal ;
			  } ;
		       } ;
		      vlh = (struct V4L__ListHugeInt *)&listp->Buffer ;
		      if (relhnum == 0)				/* If no relhnum then grab from dimension */
		       { DIMINFO(di,ctx,listp->Dim) ;
		         relhnum = di->RelHNum ;
		       } ;
		      vlh->Bytes = sizeof(*vlh) ; vlh->Blocks = 1 ; vlh->RelHNum = relhnum ; vlh->TotalElCount = listp->Entries ;
		      vlh->Block[0].ElCount = vlh->TotalElCount ; vlh->Block[0].BlockValueNum = vbe->ValueNum ;
		      listp->ListType = V4L_ListType_HugeInt ;		/* Flip list type */
		      listp->FFBufx = sizeof(*vlh) ;
		      listp->Bytes = (char *)&listp->Buffer[listp->FFBufx] - (char *)listp ;
		      goto append_again ;				/* Try to append with new list type */
		    } ;
		   memcpy(&listp->Buffer[listp->FFBufx],&point->Value,listp->BytesPerEntry) ;
		   listp->FFBufx += listp->BytesPerEntry ;
		   listp->Entries += 1 ;
		   listp->Bytes = ALIGN( (char *)&listp->Buffer[listp->FFBufx] - (char *)listp ) ;
		 } else
		 { if (listp->Bytes + point->Bytes + 40 + (listp->Entries+1)*(sizeof lpix.Offset[0]) >= V4L_ListPoint_BufMax)	/* Will our cup runneth over? */
		    { switch (point->PntType)
		       { default:
		         case V4DPI_PntType_UOM:
			 case V4DPI_PntType_UOMPer:
			 case V4DPI_PntType_UOMPUOM:
			 case V4DPI_PntType_XDB:
							ok = FALSE ; break ;
			 CASEofINT
			 case V4DPI_PntType_AggRef:
//			 case V4DPI_PntType_AggVal:
			 case V4DPI_PntType_Color:
			 case V4DPI_PntType_Country:
			 case V4DPI_PntType_XDict:
			 case V4DPI_PntType_Dict:
			 case V4DPI_PntType_SSVal:
			 case V4DPI_PntType_Int2:
			 case V4DPI_PntType_Fixed:
			 case V4DPI_PntType_Calendar:
			 case V4DPI_PntType_Tree:
			 case V4DPI_PntType_Real:	ok = TRUE ; break ;
		       } ;
		      for(i=1;ok;i++)
		       { if (!v4l_ListPoint_Value(ctx,listp,i,&tp)) break ;
		         if (i == 1) { dim = tp.Dim ; }
			  else { if (dim != tp.Dim) { ok = FALSE ; break ; } ; } ;
			 switch (tp.PntType)
			  { default: ok = FALSE ;    break ;
		            CASEofINT
			    case V4DPI_PntType_AggRef:
//			    case V4DPI_PntType_AggVal:
			    case V4DPI_PntType_Country:
			    case V4DPI_PntType_Color:
			    case V4DPI_PntType_XDict:
			    case V4DPI_PntType_Dict:
			    case V4DPI_PntType_Int2:
			    case V4DPI_PntType_Tree:
			    case V4DPI_PntType_SSVal: break ;
			    case V4DPI_PntType_Fixed: break ;
			    case V4DPI_PntType_Calendar:
			    case V4DPI_PntType_Real: break ;
			  } ;
		       } ;
/*		      If ok still true then ok to flip to huge int otherwise to huge generic */
		      if (relhnum == 0)
		       { for(relhnum=gpi->HighHNum;relhnum>=gpi->LowHNum;relhnum--)	/* Find highest area allowing new entries */
		          { if (gpi->RelH[relhnum].aid == UNUSED) continue ; if (relhnum == V4DPI_WorkRelHNum) continue ;
			    if (gpi->RelH[relhnum].ahi.BindingsUpd) break ;
			  } ;
			 if (relhnum < gpi->LowHNum && gpi->RelH[V4DPI_WorkRelHNum].aid != UNUSED)
			  relhnum = V4DPI_WorkRelHNum ;			/* Assign to work area as last resort */
		       } ;
		      if (ok)
		      { 
		         vbe = v4l_Getvbe(ctx,relhnum,0) ;		/* Have to convert to huge-list format */
		         for(i=1;;i++)
		          { if (!v4l_ListPoint_Value(ctx,listp,i,&tp)) break ;	/* Append current (old format) list to huge format */
//			    if (point->PntType == V4DPI_PntType_Real || listp->PntType == V4DPI_PntType_Int2 || listp->PntType == V4DPI_PntType_Calendar)
			    if (tp.PntType == V4DPI_PntType_Real || tp.PntType == V4DPI_PntType_Int2 || tp.PntType == V4DPI_PntType_Calendar)
			     { lda = (struct V4L__ListDblArray *)&vbe->PointValBuf ;
//			       memcpy(&lda->PointDblVal[vbe->EntryCount++],&tp.Value.RealVal,sizeof(double)) ;
			       if (point->Bytes == V4PS_Real)
			        { GETREAL(lda->PointDblVal[vbe->EntryCount++],&tp) ;
//			          memcpy(&lda->PointDblVal[vbe->EntryCount++],&tp.Value.RealVal,sizeof(double)) ;
			        } else
			        { double tdbl = v4im_GetPointDbl(&ok,&tp,ctx) ; if (!ok) return(V4L_ModRes_Fail) ;
			          SETDBL(lda->PointDblVal[vbe->EntryCount++],tdbl) ;
			        } ;
			     } else
			     { lia = (struct V4L__ListIntArray *)&vbe->PointValBuf ;
			       if (tp.PntType == V4DPI_PntType_AggRef)
			        { if (tp.Value.IntVal > 0xffffff)		/* Slimy code to embed grouping in IntVal */
			           { v_Msg(ctx,ctx->ErrorMsgAux,"LPModHIAggMax") ; return(V4L_ModRes_Fail) ; } ;
				  lia->PointIntVal[vbe->EntryCount++] = tp.Value.IntVal | (tp.Grouping << 24) ;
			        } else
			        { lia->PointIntVal[vbe->EntryCount++] = tp.Value.IntVal ; } ;
			     } ;
		          } ;
		         vlh = (struct V4L__ListHugeInt *)&listp->Buffer ;
		         if (relhnum == 0)				/* If no relhnum then grab from dimension */
		          { DIMINFO(di,ctx,listp->Dim) ;
		            relhnum = di->RelHNum ;
		          } ;
		         vlh->Bytes = sizeof(*vlh) ; vlh->Blocks = 1 ; vlh->RelHNum = relhnum ; vlh->TotalElCount = listp->Entries ;
		         vlh->Block[0].ElCount = vlh->TotalElCount ; vlh->Block[0].BlockValueNum = vbe->ValueNum ;
		         listp->ListType = V4L_ListType_HugeInt ;		/* Flip list type */
		         listp->FFBufx = sizeof(*vlh) ;
		         listp->Bytes = (char *)&listp->Buffer[listp->FFBufx] - (char *)listp ;
			 listp->BytesPerEntry = (point->PntType == V4DPI_PntType_Real || point->PntType == V4DPI_PntType_Calendar ? sizeof(double) : sizeof lp1->Value.IntVal) ;
			 listp->Dim = point->Dim ; listp->PntType = point->PntType ;
		         goto append_again ;				/* Try to append with new list type */
		      } else						/* Here to convert to huge generic */
		      {
		         vbeg = v4l_Getvbeg(ctx,relhnum,0) ;		/* Have to convert to huge-list format */
			 if (vbeg == NULL) return(V4L_ModRes_Fail) ;
			 ipt = (P *)vbeg->PointValBuf ;
		         for(i=1;;i++)
		          { if (!v4l_ListPoint_Value(ctx,listp,i,&tp)) break ;	/* Append current (old format) list to huge format */
			    memcpy(ipt,&tp,tp.Bytes) ;
			    ADVPNT(ipt) ; vbeg->EntryCount++ ;
		          } ;
		         vlhg = (struct V4L__ListHugeGeneric *)&listp->Buffer ;
		         if (relhnum == 0)				/* If no relhnum then grab from dimension */
		          { DIMINFO(di,ctx,listp->Dim) ;
		            relhnum = di->RelHNum ;
		          } ;
		         vlhg->Bytes = sizeof(*vlhg) ; vlhg->Blocks = 1 ; vlhg->RelHNum = relhnum ; vlhg->TotalElCount = listp->Entries ;
		         vlhg->Block[0].ElCount = vlhg->TotalElCount ; vlhg->Block[0].BlockValueNum = vbeg->ValueNum ;
		         listp->ListType = V4L_ListType_HugeGeneric ;		/* Flip list type */
		         listp->FFBufx = sizeof(*vlhg) ;
		         listp->Bytes = (char *)&listp->Buffer[listp->FFBufx] - (char *)listp ;
			 listp->BytesPerEntry = (point->PntType == V4DPI_PntType_Real || point->PntType == V4DPI_PntType_Calendar ? sizeof(double) : sizeof lp1->Value.IntVal) ;
			 listp->Dim = point->Dim ; listp->PntType = point->PntType ;
		         goto append_again ;				/* Try to append with new list type */
		      } ;
		    } ;
		   memcpy(&lpix,&listp->Buffer[listp->LPIOffset],listp->Entries*(sizeof lpix.Offset[0])) ;
		   lpix.Offset[listp->Entries] = listp->FFBufx ;	/* Record position of point at end of list */
		   if (listp->Dim > 0)			/* Are we saving entire point or just value? */
		    { lpj = (struct V4L__ListPoint_Jacket *)&listp->Buffer[listp->FFBufx] ;	/* Link up to "jacket" */
		      lpj->Bytes = point->Bytes-V4DPI_PointHdr_Bytes ;
		      memcpy(lpj->Buffer,&point->Value,lpj->Bytes) ;
		      listp->FFBufx += ALIGN(sizeof lpj->Bytes + lpj->Bytes) ;
		    } else
		    { memcpy(&listp->Buffer[listp->FFBufx],point,point->Bytes) ;
		      listp->FFBufx += ALIGN(point->Bytes) ;
		    } ;
		   listp->Entries += 1 ; listp->LPIOffset = listp->FFBufx ;
		   listp->Bytes = ALIGN((char *)&listp->Buffer[listp->LPIOffset+listp->Entries*(sizeof lpix.Offset[0])] - (char *)listp) ;
		   if (listp->Bytes >= V4L_ListPoint_BufMax)
		    { 
//		      v4dpi_PointToString(ASCTBUF1,point,ctx,-1) ; printf("Entries=%d, Bytes=%d, Dim=%d, point=%s\n",listp->Entries,listp->Bytes,listp->Dim,ASCTBUF1) ;
		      v_Msg(ctx,UCTBUF1,"@*Entries=%1d, Bytes=%2d, Dim=%3D, point=%4P\n",listp->Entries,listp->Bytes,listp->Dim,point) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
		      for(i=1;i<=listp->Entries-1;i++)
		       { v4l_ListPoint_Value(ctx,listp,i,&tp) ;
//		         v4dpi_PointToString(ASCTBUF1,&tp,ctx,-1) ; printf("%d. %s\n",i,ASCTBUF1) ;
			 v_Msg(ctx,UCTBUF1,"@*      %1d. %2P\n",i,&tp) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
		       } ;
		      v_Msg(ctx,ctx->ErrorMsgAux,"LPModMaxSize") ; return(V4L_ModRes_Fail) ;
		    } ;
		   memcpy(&listp->Buffer[listp->LPIOffset],&lpix,listp->Entries*(sizeof lpix.Offset[0])) ;
		 } ;
		break ;
	 } ;
	return(V4L_ModRes_OK) ;		/* All must be well */
}

/*	v4l_ListPoint_Value - Returns value of i'th list element	*/
/*	Call: result = v4l_ListPoint_Value( ctx,listp , index , point )
	  where result is integer result depending on index value
			if index = V4L_ListSize then returns list size (if possible, V4L_ListSize_Unk if not known (e.g. lazy list))
			if index >= 0 then returns 0 if end of list, 1 if OK, 2 if ok & point already in context
			result is LISTVALERR if an error (message in ctx->ErrorMsgAux),
	  	ctx is current context,
		listp is pointer to list,
		index is element number, 0 to return number of elements in list,
		point is pointer to point buffer for result,
		trace is trace flag					*/

int v4l_ListPoint_Value(ctx,listp,index,point)
  struct V4C__Context *ctx ;
  struct V4L__ListPoint *listp ;
  int index ;
  struct V4DPI__Point *point ;
{ struct V4L__ListPointIndex *lpix ;
  struct V4L__ListCmpndRange *lcr ;
  struct V4L__ListCmpndRangeDBL *lcrd ;
  struct V4L__ListPoint_Jacket *lpj ;
  struct V4L__ListIsct *lisct ;
  struct V4L__ListAggRef *lar ;
  struct V4L__ListTextTable *ltt ;
  struct V4L__ListToken *vllt ;
  struct V4L__ListXDBGet *log ;
  struct V4DPI__Point *pt,*tpt1,*tpt2,*ipt ;
  struct V4L__ListHugeInt *vlh ;
  struct V4L__ListHugeGeneric *vlhg ;
  struct V4L__BlockEntry *vbe ;
  struct V4L__BlockEntryGeneric *vbeg ;
  struct V4L__ListMultiAppend *vlma ;
  struct V4L__ListIntArray *lia ;
  struct V4L__ListDblArray *lda ;
  struct V4L__ListBigText *lbt ;
  struct V4L__ListV4IS *lv4 ;
  struct V4DPI__PntV4IS *pis ;
  struct V4IS__V4Areas *v4a ;
  struct V4IS__ParControlBlk *pcb ;
  struct V4L__ListLazy *vll ;
  struct V4L__ListBitMap *lbm ;
  struct V4L__ListBMData1 *bm1 ;
  struct V4L__ListMDArray *lmda ;
#ifdef WANTHTMLMODULE
  struct V4L__ListHTMLTable *lht ;
#endif
  struct V4L__ListFiles *vlf ;
  struct V4L__ListTextFile *ltf ;
  struct V4L__ListFilesInfo *lfi ;
  struct V4L__ListPoint *lp1 ;
  struct V4DPI__DimInfo *di ;
  static struct V4DPI__DimInfo *diXMLBegin,*diXMLEnd,*diNId ; static LOGICAL needdiInit=TRUE ;
  struct V4IM__MDArray *vmda ;
  struct V4LEX_ListXMLCtrl *lxc ;
 static double powers[] = { 1.0, 10.0, 100.0, 1000.0, 10000.0, 100000.0, 1000000.0, 10000000.0, 100000000.0 } ;
 int i,j,bx,num,max,*imax,ok,iswhile,flags ; LENMAX len ; short *smax ; double d1,d2,d3 ; UCCHAR msg[100], *b ;

#ifdef V4_BUILD_LMTFUNC_MAX_LIST
	if (index > V4_BUILD_LMTFUNC_MAX_LIST) return(FALSE) ;	/* Don't allow access to list elements beyond max */
#endif
	if (index == V4L_ListSize)				/* Just return number of elements in list */
	 { if (listp->Entries > 0) return(listp->Entries) ;
/*	   Don't have explicit count of entries - see if we can figure it out */
	   switch(listp->ListType)
	    { case V4L_ListType_BitMap:		return(v4l_BitMapCount(ctx,NULL,listp)) ;
	      case V4L_ListType_Point:		return(0) ;		/* There are no elements in list (yet) */
	      case V4L_ListType_JSON:		return(0) ;		/* Nothing in the BLOB */
	    } ;
	   return(V4L_ListSize_Unk) ;		/* Can't figure out (easily) the number of entries */
	 } ;
	switch (listp->ListType)
	 {
	   case V4L_ListType_Drawer:
		if (index > listp->Entries) return(0) ;
		{ struct V4L__ListDrawer *ldrw ;
		  ldrw = (struct V4L__ListDrawer *)&listp->Buffer[0] ;
		  if (ldrw->index > ldrw->drw->Points) return(LISTVALERR) ;
		  memcpy(point,ldrw->dPt,ldrw->dPt->Bytes) ;
		  ldrw->dPt = (P *)((char *)ldrw->dPt + ldrw->dPt->Bytes) ;
		  ldrw->index++ ;
		  return(1) ;
		}
	   case V4L_ListType_JSON:
		if (index > listp->Entries) return(0) ;
		{ P *ipt ; struct V4L__ListJSON *ljson ; P *argpnts[2] ; struct V4DPI__LittlePoint lpnt ; intPNTv((P*)&lpnt,index) ; argpnts[1] = (P *)&lpnt ;
		  ljson = (struct V4L__ListJSON *)&listp->Buffer[0] ;
		  ipt = vjson_Dereference(ctx,point,ljson->vjblob,&ljson->vjval,argpnts,1,1,0,ljson->dimAsName) ;
		  return(ipt != NULL ? 1 : LISTVALERR) ;
		} ;
	   case V4L_ListType_TextTable:
		ltt = (struct V4L__ListTextTable *)&listp->Buffer[0] ;
		if (++ltt->Count != index) return(0) ;			/* Can only return sequentially */
		for(;;)
		 { 
//		   if (fgets(ltt->line,V4LEX_Tkn_InpLineMax,ltt->fp) == NULL)	/* Error or EOF */
//		    { fclose(ltt->fp) ;
		   if (v_UCReadLine(&ltt->UCFile,UCRead_UC,ltt->line,V4LEX_Tkn_InpLineMax,ctx->ErrorMsgAux) < 0)
		    { v4l_ListClose(ctx,listp) ; return(0) ;
		    } ; ltt->ActCount ++ ;
		   { struct V4LEX__Table *zzz ; COPYPTR(zzz,ltt->vlt) ; 
		     if ((num=v4lex_ReadNextLine_Table(ltt->line,UNUSED,UCTBUF2,UCTBUF1,msg,zzz,NULL,0,NULL,NULL,ltt->ActCount)) >= 0) break ;
		   }
		   v_Msg(ctx,UCTBUF1,"@* (%1U) in %2U line %3d column %4d (%5U) field *[%6U]*\n",msg,ltt->FileName,
					ltt->ActCount,-num,UCTBUF1,UCTBUF2) ;
		   vout_UCText(VOUT_Err,0,UCTBUF1) ; gpi->ErrCount++ ;
		   continue ;
		 } ;
		ZPH(point) ; point->PntType = V4DPI_PntType_AggRef ;
		point->Bytes = V4PS_Int ;
/*		Note offset in Grouping is V4C_AreaAgg_Max so we can differentiate Aggs from TextAggs! */
		point->Grouping = ltt->TextAggIndex + V4C_AreaAgg_Max ;
		point->Dim = ltt->Dim ;
		point->Value.IntVal = ltt->Count ; gpi->TextAgg[ltt->TextAggIndex].Count = ltt->Count ;
		return(1) ;
	   case V4L_ListType_BigText:
		lbt = (struct V4L__ListBigText *)&listp->Buffer[0] ;
		if (*lbt->ptrText == UCEOS)			/* End of the BigBuf */
		 { v4l_ListClose(ctx,listp) ; return(0) ;
		 } ;
		b = UCstrchr(lbt->ptrText,EOLbt) ; if (b != NULL) *b = UCEOS ;
		len = (b == NULL ? UCstrlen(lbt->ptrText) : b - lbt->ptrText) ;
		if (len >= V4DPI_UCVAL_MaxSafe)	/* Have to convert to bigtext */
		 { struct V4LEX__BigText *bt ;
		   if (len >= V4LEX_BigText_Max)
		    { v_Msg(ctx,ctx->ErrorMsgAux,"@Result string (len=%1d) exceeds max length allowed(%2d)",b-lbt->ptrText,V4LEX_BigText_Max) ; 
		      return(LISTVALERR) ;
		    } ;
		   bt = (struct V4LEX__BigText *)v4mm_AllocChunk(sizeof *bt,FALSE) ;
		   UCstrcpy(bt->BigBuf,lbt->ptrText) ;
		   if (!v4dpi_SaveBigTextPoint(ctx,bt,UCstrlen(bt->BigBuf),point,Dim_Alpha,TRUE))
		    { v_Msg(ctx,ctx->ErrorMsgAux,"StrSaveBigText",0,V4DPI_PntType_BigText) ; return(LISTVALERR) ; } ;
		   v4mm_FreeChunk(bt) ;		
		 } else
		 { uccharPNTv(point,lbt->ptrText) ; } ;
		lbt->lineNum++ ; lbt->ptrText = (b != NULL ? b + 1 : UClit("")) ;	/* Set to character after end-of-line or to UCEOS if nothing else */
		return(1) ;
	   case V4L_ListType_JavaTokens:
	   case V4L_ListType_Token:
next_token:	vllt = (struct V4L__ListToken *)&listp->Buffer[0] ;
		flags = V4LEX_Option_RetEOL ;
		if (listp->ListType == V4L_ListType_JavaTokens)
		 { flags |= V4LEX_Option_Java ;
		   if (vllt->lastWasOper) flags |= V4LEX_Option_RegExp ;
		 } ;
		if (vllt->parseFlags & V4L_ListToken_Negative) flags |= V4LEX_Option_NegLit ;
		if (vllt->parseFlags & V4L_ListToken_Space) flags |= V4LEX_Option_Space ;
		v4lex_NextTkn(vllt->tcb,flags) ;
/*		Results in vllt->tcb - figure out how to return as point */
		switch (vllt->tcb->type)
		 { default:
		   case V4LEX_TknType_RegExpStr:
		   case V4LEX_TknType_String:
			vllt->lastWasOper = FALSE ;
			vllt->tcb->UCstring[V4DPI_UCVal_Max-1] = UCEOS ; uccharPNTv(point,vllt->tcb->UCstring) ; break ;
		   case V4LEX_TknType_Keyword:
			vllt->lastWasOper = FALSE ;
			DIMINFO(di,ctx,Dim_NId) ;
			if (vllt->parseFlags & V4L_ListToken_LowerCase) { UCSTRTOLOWER(vllt->tcb->UCstring) ; }
			 else if (vllt->parseFlags & V4L_ListToken_UpperCase) { UCSTRTOUPPER(vllt->tcb->UCstring) ; }
			dictPNTv(point,Dim_NId,v4dpi_DictEntryGet(ctx,Dim_NId,vllt->tcb->UCstring,di,NULL)) ; break ;
		   case V4LEX_TknType_Integer:
			vllt->lastWasOper = FALSE ;
			if (vllt->tcb->decimal_places == 0) { intPNTv(point,vllt->tcb->integer) ; break ; } ;
			d1 = vllt->tcb->integer / powers[vllt->tcb->decimal_places] ;
			dblPNTv(point,d1) ; break ;
		   case V4LEX_TknType_Float:
			vllt->lastWasOper = FALSE ;
			dblPNTv(point,vllt->tcb->floating) ; break ;
		   case V4LEX_TknType_EOL:
			vllt->lastWasOper = FALSE ;
			if ((vllt->parseFlags & V4L_ListToken_EndOfLine) == 0) goto next_token ;
			dictPNTv(point,Dim_UV4,v4im_GetEnumToDictVal(ctx,DE(EndOfLine),Dim_UV4)) ;
			break ;
		   case V4LEX_TknType_Punc:
			switch (vllt->tcb->opcode)
			 { default:
			     dictPNTv(point,Dim_UV4,v4im_GetEnumToDictVal(ctx,opcodeDE[vllt->tcb->opcode-1],Dim_UV4)) ;
			     break ;
			   case V_OpCode_EOF:		/* End of file/line - if pulling from list get next element, if end of list then really end of file */
			     if (vllt->listPt == NULL) goto at_end ;
			     if (!v4l_ListPoint_Value(ctx,(struct V4L__ListPoint *)&vllt->listPt->Value,index,point)) goto at_end ;
			     v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,point,ctx) ;
			     if (!ok) { v_Msg(ctx,NULL,"ModInvArg",0,1) ; goto at_end ; } ;
			     v4lex_NestInput(vllt->tcb,NULL,UCTBUF1,V4LEX_InpMode_String) ;
			     goto next_token ;
			 } ;
			if (listp->ListType == V4L_ListType_JavaTokens)
			 { switch (vllt->tcb->opcode)
			    { default:			vllt->lastWasOper = TRUE ; break ;
			      case V_OpCode_RParen:	vllt->lastWasOper = FALSE ; break ;
			    } ;
			 } ;
			break ;
		   case V4LEX_TknType_LInteger:
			vllt->lastWasOper = FALSE ;
			dblPNTv(point,vllt->tcb->Linteger) ; break ;
		   case V4LEX_TknType_Error:
			vllt->lastWasOper = FALSE ;
			UCstrcpy(ctx->ErrorMsgAux,vllt->tcb->UCstring) ;
			vllt->tcb->UCstring[V4DPI_UCVal_Max-1] = UCEOS ; uccharPNTv(point,vllt->tcb->UCstring) ;
			point->Dim = Dim_UV4 ; return(LISTVALERR) ;
		 } ;
	        return(1) ;
	   case V4L_ListType_HTMLTokens:
	   case V4L_ListType_XMLTokens:
#define XS(STATE) lxc->xmlState = XML_State_##STATE
#define XERR(ERRSRC,ERRCODE) { num = ERRSRC ; ok = ERRCODE ; goto hit_error ; }
		vllt = (struct V4L__ListToken *)&listp->Buffer[0] ;
		if (needdiInit)
		 { needdiInit = FALSE ; DIMINFO(diNId,ctx,v4dpi_DimGet(ctx,UClit("NId"),DIMREF_IRT)) ;
		   DIMINFO(diXMLBegin,ctx,v4dpi_DimGet(ctx,UClit("XMLBegin"),DIMREF_IRT)) ; DIMINFO(diXMLEnd,ctx,v4dpi_DimGet(ctx,UClit("XMLEnd"),DIMREF_IRT)) ;
		 } ;
		lxc = vllt->lxc ;
		flags = V4LEX_Option_RetEOL | (listp->ListType == V4L_ListType_HTMLTokens ? V4LEX_Option_HTML : V4LEX_Option_XML) ;
		for(;;)
		 { UCCHAR lname[V4LEX_Tkn_StringMax] ;
		   P tpnt ; int res ;
		   switch (lxc->xmlState)
		    { 
		      case XML_State_BOM:
			v4lex_NextTkn(vllt->tcb,flags) ;
			switch (vllt->tcb->opcode)
			 { default:
			   case V_OpCode_EOF:			goto hit_eof ;
			   case V_OpCode_EOX:			continue ;
			   case V_OpCode_Langle:		XS(StartLvl) ; continue ;
			   case V_OpCode_LangleBang:		XS(InCom) ; continue ;
			   case V_OpCode_LangleBangDashDash:	XS(InCom1) ; continue ;
			   case V_OpCode_LangleSlash:		XS(EndLvl) ; continue ;
			   case V_OpCode_LangleQuestion:	XS(InHdr) ; continue ;
			 } ;
		      case XML_State_StartLvl:
			v4lex_NextTkn(vllt->tcb,flags) ;
			if (vllt->tcb->opcode != V_OpCode_Keyword) XERR(10,XML_Error_InvName) ;
/*			Look for construction of form: <xx:levelname ... > */
			UCstrcpy(lname,vllt->tcb->UCstring) ; v4lex_NextTkn(vllt->tcb,flags) ;
			if (vllt->tcb->opcode == V_OpCode_Colon)
			 { v4lex_NextTkn(vllt->tcb,flags) ;
			   if (vllt->tcb->opcode != V_OpCode_Keyword) XERR(20,XML_Error_InvName) ;
			 } else { v4lex_NextTkn(vllt->tcb,V4LEX_Option_PushCur) ; UCstrcpy(vllt->tcb->UCstring,lname) ; } ;
			res = 0 ;
/*			If parsing HTML then see if current level is one of list of 'special' tags that don't need explicit </xxx> ending */
/*			Examples include: <meta...> <input...> and <br> */
			if (listp->ListType == V4L_ListType_HTMLTokens && lxc->nestLvl > 0)
			 { int hx ;
			   for(hx=0;hx<V4LI_HTMLMax;hx++)
			    { if (UCstrcmpIC(lxc->Lvl[lxc->nestLvl-1].lvlName,gpi->ci->li->LI[gpi->ci->li->CurX].htmlTag[hx]) == 0)
			       { lxc->nestLvl-- ; res |= V4L_AutoEndLvl ; break ; } ;
			    } ;
			 } ;
			if (lxc->nestLvl >= V4LEX_XML_LvlMax) XERR(30,XML_Error_TooMnyLvl) ;
			if (vllt->parseFlags & V4L_ListToken_LowerCase) { UCSTRTOLOWER(vllt->tcb->UCstring) ; }
			 else if (vllt->parseFlags & V4L_ListToken_UpperCase) { UCSTRTOUPPER(vllt->tcb->UCstring) ; }
			UCstrcpy(lxc->Lvl[lxc->nestLvl].lvlName,vllt->tcb->UCstring) ; lxc->nestLvl++ ;
			XS(InLvl) ; 
/*			Now return XMLBegin:xxx */
//			DIMINFO(di,ctx,v4dpi_DimGet(ctx,UClit("XMLBegin"))) ;
			dictPNTv(point,diXMLBegin->DimId,v4dpi_DictEntryGet(ctx,Dim_NId,lxc->Lvl[lxc->nestLvl-1].lvlName,diXMLBegin,NULL)) ;
			return(res | 1) ;
		      case XML_State_InCom1:					/* Got '<!--', read until '-->' */
			for(;lxc->xmlState==XML_State_InCom1;)
			 { v4lex_NextTkn(vllt->tcb,flags) ;
			   switch (vllt->tcb->opcode)
			    { default:				continue ;
			      case V_OpCode_DashDashRangle:	XS(BOM) ; break ;	/* Pretend we are at bom to look for '<' */
			      case V_OpCode_EOF:		goto hit_eof ;
			      case V_OpCode_EOX:		continue ;
			    } ;
		         } ;
		        continue ;
		      case XML_State_InCom:
			for(;lxc->xmlState==XML_State_InCom;)
			 { v4lex_NextTkn(vllt->tcb,flags) ;
			   switch (vllt->tcb->opcode)
			    { default:			continue ;
			      case V_OpCode_Rangle:	XS(BOM) ; break ;	/* Pretend we are at bom to look for '<' */
			      case V_OpCode_EOF:	goto hit_eof ;
			      case V_OpCode_EOX:	continue ;
			      case V_OpCode_NCLBracket:				/* Maybe we got <![CDATA[ xxx ?? */
			      case V_OpCode_LBracket:
				switch(v4im_GetTCBtoEnumVal(ctx,vllt->tcb,0))
				 { default:		XERR(40,XML_Error_Syntax) ;
				   case _If:		continue ;
				   case _EndIf:		continue ;
				   case _CDATA:		v4lex_NextTkn(vllt->tcb,flags) ;
							if (vllt->tcb->opcode != V_OpCode_LBracket) { XERR(50,XML_Error_Syntax) ; } ;
							XS(InCDATA) ; break ;
				 } ; break ;
			    } ;
			 } ;
			continue ;
		      case XML_State_InLvl:
			v4lex_NextTkn(vllt->tcb,flags|V4LEX_Option_ForceXMLKW) ;
			switch (vllt->tcb->opcode)
			 { default:			XERR(60,XML_Error_Syntax) ;
			   case V_OpCode_EOF:		goto hit_eof ;
			   case V_OpCode_EOX:		continue ;
			   case V_OpCode_Colon:
			   case V_OpCode_Keyword:
			     v4lex_NextTkn(vllt->tcb,flags) ;
			     if (vllt->tcb->opcode == V_OpCode_Colon)
			      { if (vllt->lxc->includeSchema)
			         { v4lex_NextTkn(vllt->tcb,flags) ;
			           if (vllt->tcb->opcode != V_OpCode_Keyword) XERR(70,XML_Error_InvName) ;
			         } else
			         { v4lex_NextTkn(vllt->tcb,flags) ;		/* Ignoring Schema info - skip tag:keyword=value */
			           if (vllt->tcb->opcode != V_OpCode_Keyword) XERR(80,XML_Error_InvName) ;
			           v4lex_NextTkn(vllt->tcb,flags) ;
			           if (vllt->tcb->opcode != V_OpCode_Equal)
			            XERR(90,XML_Error_Syntax) ;
			           v4lex_NextTkn(vllt->tcb,flags) ;
			           if (vllt->tcb->type != V4LEX_TknType_String && vllt->tcb->type != V4LEX_TknType_Keyword) XERR(100,XML_Error_Syntax) ;
			           continue ;
			         } ;
			      } else
			      { if (vllt->lxc->includeAttr)
			        { v4lex_NextTkn(vllt->tcb,V4LEX_Option_PushCur) ; }
			         else { LOGICAL loop ;
					if (vllt->tcb->opcode != V_OpCode_Equal)
					 XERR(110,XML_Error_Syntax) ;				/* Skip remaining '=value' */
					for (loop=TRUE;loop;)
					 { v4lex_NextTkn(vllt->tcb,flags|V4LEX_Option_ForceKW|V4LEX_Option_MLStringLit) ;
					   switch (vllt->tcb->type)
					    { default:			XERR(12,XML_Error_Syntax) ;
					      case V4LEX_TknType_String:
					      case V4LEX_TknType_Keyword:	loop = FALSE ; break ;
					      case V4LEX_TknType_MLString:
						if (!v4lex_ReadNextLine(vllt->tcb,vllt->tcb->ilvl[vllt->tcb->ilx].tpo,NULL,NULL))
						 { if (!v4l_XMLHandleEOF(ctx,vllt,point,traceGlobal)) XERR(112,XML_Error_Syntax) ;
						 }
						continue ;

//					      	XS(MLString1) ; continue ;
					    } ;
					 } ;
					if (vllt->tcb->type != V4LEX_TknType_String && vllt->tcb->type != V4LEX_TknType_Keyword)
					 XERR(12,XML_Error_Syntax) ;
					continue ;
				      } ;
			      } ;
			     if (vllt->parseFlags & V4L_ListToken_LowerCase) { UCSTRTOLOWER(vllt->tcb->UCstring) ; }
			      else if (vllt->parseFlags & V4L_ListToken_UpperCase) { UCSTRTOUPPER(vllt->tcb->UCstring) ; }
			     UCstrcpy(lxc->curKeyword,vllt->tcb->UCstring) ; XS(CheckAttVal) ; continue ;	
			   case V_OpCode_Rangle:	XS(LookForLitVal) ; continue ;
			   case V_OpCode_SlashRangle:
			     XS(BOM) ;
			     if (lxc->nestLvl > 0) lxc->nestLvl-- ;
/*			     Return XMLEnd:xxx (with last saved name) */
//			     DIMINFO(di,ctx,v4dpi_DimGet(ctx,UClit("XMLEnd"))) ;
			     dictPNTv(point,diXMLEnd->DimId,v4dpi_DictEntryGet(ctx,Dim_NId,lxc->Lvl[lxc->nestLvl].lvlName,diXMLEnd,NULL)) ;
			     return(1) ;
			 } ;
		      case XML_State_CheckAttVal:
			v4lex_NextTkn(vllt->tcb,flags) ;
			switch (vllt->tcb->opcode)
			 { default:			XERR(130,XML_Error_Syntax) ;
			   case V_OpCode_EOX:		continue ;
			   case V_OpCode_Equal:		XS(AttrValue) ; continue ;
			 } ;
		      case XML_State_AttrValue:
			v4lex_NextTkn(vllt->tcb,flags|V4LEX_Option_ForceXMLKW|V4LEX_Option_MLStringLit) ;
			switch (vllt->tcb->opcode)
			 { default:
			     XERR(140,XML_Error_Syntax) ;
			   case V_OpCode_EOF:		goto hit_eof ;
			   case V_OpCode_EOX:		continue ;
			   case V_OpCode_MLString:
			     { LOGICAL loop ; UCCHAR mlbuf[V4DPI_UCVal_Max] ; ZUS(mlbuf) ;
			       if (UCstrlen(vllt->tcb->UCstring) + UCstrlen(mlbuf) < UCsizeof(mlbuf)) UCstrcat(mlbuf,vllt->tcb->UCstring) ;
			       for(loop=TRUE;loop;)
			        { if (!v4lex_ReadNextLine(vllt->tcb,vllt->tcb->ilvl[vllt->tcb->ilx].tpo,NULL,NULL))
				   { if (!v4l_XMLHandleEOF(ctx,vllt,point,traceGlobal)) XERR(112,XML_Error_Syntax) ;
				   } ;
				  v4lex_NextTkn(vllt->tcb,flags|V4LEX_Option_ForceXMLKW|V4LEX_Option_MLStringLit) ;
			          if (UCstrlen(vllt->tcb->UCstring) + UCstrlen(mlbuf) < UCsizeof(mlbuf)) UCstrcat(mlbuf,vllt->tcb->UCstring) ;
				  if (vllt->tcb->opcode == V_OpCode_MLString)
				   continue ;
				  break ;
			        } ;
			       UCstrcpy(vllt->tcb->UCstring,mlbuf) ;
			     }
			     XS(InLvl) ;
/*			     Return list of form: (attr-name attr-value) (using last saved name) */
			     INITLP(point,lp1,Dim_List) ;
//			     DIMINFO(di,ctx,v4dpi_DimGet(ctx,UClit("NId"))) ;
			     dictPNTv(&tpnt,Dim_NId,v4dpi_DictEntryGet(ctx,Dim_NId,lxc->curKeyword,diNId,NULL)) ;
			     if (!v4l_ListPoint_Modify(ctx,lp1,V4L_ListAction_Append,&tpnt,0)) XERR(150,XML_Error_List) ;
			     uccharPNTv(&tpnt,vllt->tcb->UCstring) ;
			     if (!v4l_ListPoint_Modify(ctx,lp1,V4L_ListAction_Append,&tpnt,0)) XERR(160,XML_Error_List) ;
			     ENDLP(point,lp1) ;
			     return(1) ;
			   case V_OpCode_Keyword:	UCstrcpy(vllt->tcb->UCstring,vllt->tcb->UCkeyword) ; /* Fall thru and handle as if string */
			   case V_OpCode_QString:
			   case V_OpCode_DQString:
			     XS(InLvl) ;
/*			     Return list of form: (attr-name attr-value) (using last saved name) */
			     INITLP(point,lp1,Dim_List) ;
//			     DIMINFO(di,ctx,v4dpi_DimGet(ctx,UClit("NId"))) ;
			     dictPNTv(&tpnt,Dim_NId,v4dpi_DictEntryGet(ctx,Dim_NId,lxc->curKeyword,diNId,NULL)) ;
			     if (!v4l_ListPoint_Modify(ctx,lp1,V4L_ListAction_Append,&tpnt,0)) XERR(150,XML_Error_List) ;
			     uccharPNTv(&tpnt,vllt->tcb->UCstring) ;
			     if (!v4l_ListPoint_Modify(ctx,lp1,V4L_ListAction_Append,&tpnt,0)) XERR(160,XML_Error_List) ;
			     ENDLP(point,lp1) ;
			     return(1) ;
			 } ;
		      case XML_State_LookForLitVal:
			v4lex_NextTkn(vllt->tcb,flags|V4LEX_Option_XMLLitVal) ;
			switch (vllt->tcb->opcode)
			 { default:			XERR(170,XML_Error_Syntax) ;
			   case V_OpCode_EOF:		goto hit_eof ;
			   case V_OpCode_EOX:		continue ;
			   case V_OpCode_XMLLitPartial:
/*			     Return value of form XMLLit:xxxx */
//			     DIMINFO(di,ctx,v4dpi_DimGet(ctx,UClit("Alpha"))) ;
			     { LENMAX len = UCstrlen(vllt->tcb->UCstring) ;
			       if (len >= V4DPI_UCVAL_MaxSafe)		/* Save as string or BigText ? */
				{ vllt->tcb->UCstring[V4LEX_BigText_Max-1] = UCEOS ; v4dpi_SaveBigTextPoint2(ctx,vllt->tcb->UCstring,point,Dim_Alpha,TRUE) ;
				} else { uccharPNTvl(point,vllt->tcb->UCstring,len) ; } ;
			     }
//			     uccharPNTv(point,vllt->tcb->UCstring) ; point->Dim = Dim_Alpha ;
			     vllt->tcb->need_input_line = TRUE ;
			     return(1) ;
			   case V_OpCode_XMLLit:
			     XS(StartEndLvl) ;		/* Only way to get OpCode_XMLLit is when we hit '<' */
//			     if (UCstrlen(vllt->tcb->UCstring) == 0)	/* Don't return empty value, look for nested '<' */
			     if (UCempty(vllt->tcb->UCstring))		/* Don't return empty value, look for nested '<' */
			      continue ;
/*			     Return value of form XMLLit:xxxx */
//			     DIMINFO(di,ctx,v4dpi_DimGet(ctx,UClit("Alpha"))) ;
			     { LENMAX len = UCstrlen(vllt->tcb->UCstring) ;
			       if (len >= V4DPI_UCVAL_MaxSafe)		/* Save as string or BigText ? */
				{ vllt->tcb->UCstring[V4LEX_BigText_Max-1] = UCEOS ; v4dpi_SaveBigTextPoint2(ctx,vllt->tcb->UCstring,point,Dim_Alpha,TRUE) ;
				} else { uccharPNTvl(point,vllt->tcb->UCstring,len) ; } ;
			     }
//			     uccharPNTv(point,vllt->tcb->UCstring) ; point->Dim = Dim_Alpha ;
			     return(1) ;
			 } ;
		      case XML_State_InCDATA:
//veh081210 - Added loop here to look for 'eof' and then read next line from file so we return single cdata entry
//needs to be cleaned up (indented, etc) if it really works
	 { UCCHAR cdata[UCReadLine_MaxLine] ; LOGICAL cdl ;	
		ZUS(cdata) ;
		for(cdl=TRUE;cdl;)	
		 {
			v4lex_NextTkn(vllt->tcb,flags|V4LEX_Option_XMLCDATA) ;
			switch (vllt->tcb->opcode)
			 { default:			XERR(180,XML_Error_Syntax) ;
			   case V_OpCode_EOF:
			     if (!v4l_XMLHandleEOF(ctx,vllt,point,traceGlobal)) goto at_end ;
			     continue ;
			   case V_OpCode_EOX:		continue ;
			   case V_OpCode_XMLLitPartial:
			     if (vllt->tcb->literal_len + UCstrlen(cdata) + 5 < UCsizeof(cdata))
			      { if(UCnotempty(cdata)) UCstrcat(cdata,UClit(" ")) ; UCstrcat(cdata,vllt->tcb->UCstring) ; } ;
			     if (v4l_XMLHandleEOF(ctx,vllt,point,traceGlobal)) continue ;
			     { LENMAX len = UCstrlen(cdata) ;
			       if (len >= V4DPI_UCVAL_MaxSafe)		/* Save as string or BigText ? */
				{ cdata[V4LEX_BigText_Max-1] = UCEOS ; v4dpi_SaveBigTextPoint2(ctx,cdata,point,Dim_Alpha,TRUE) ;
				} else { uccharPNTvl(point,cdata,len) ; } ;
			     }
			     vllt->tcb->need_input_line = TRUE ;
			     return(1) ;



///*			     Return value of form XMLLit:xxxx */
//			     if (vllt->tcb->literal_len > V4DPI_UCVal_Max)
//			      vllt->tcb->UCstring[V4DPI_UCVal_Max-3] = UCEOS ;
//			     { LENMAX len = UCstrlen(vllt->tcb->UCstring) ;
//			       if (len >= V4DPI_UCVal_Max - 1)		/* Save as string or BigText ? */
//				{ vllt->tcb->UCstring[V4LEX_BigText_Max-1] = UCEOS ; v4dpi_SaveBigTextPoint2(ctx,vllt->tcb->UCstring,point,Dim_Alpha,TRUE) ;
//				} else { uccharPNTvl(point,vllt->tcb->UCstring,len) ; } ;
//			     }
//			     vllt->tcb->need_input_line = TRUE ;
//			     return(1) ;
			   case V_OpCode_XMLLit:
			     XS(StartEndLvl) ;		/* Only way to get OpCode_XMLLit is when we hit ']]>' */
			     if (vllt->tcb->literal_len + UCstrlen(cdata) + 5 < UCsizeof(cdata))
			      { if(UCnotempty(cdata)) UCstrcat(cdata,UClit(" ")) ; UCstrcat(cdata,vllt->tcb->UCstring) ; } ;
			     if (UCempty(cdata)) continue ;
			     if (vllt->tcb->literal_len > V4DPI_UCVal_Max)
			      vllt->tcb->UCstring[V4DPI_UCVal_Max-3] = UCEOS ;
			     { LENMAX len = UCstrlen(cdata) ;
			       if (len >= V4DPI_UCVAL_MaxSafe)		/* Save as string or BigText ? */
				{ cdata[V4LEX_BigText_Max-1] = UCEOS ; v4dpi_SaveBigTextPoint2(ctx,cdata,point,Dim_Alpha,TRUE) ;
				} else { uccharPNTvl(point,cdata,len) ; } ;
			     }
			     cdl = FALSE ; break ;
			     
//			     if (UCempty(vllt->tcb->UCstring))		/* Don't return empty value, look for nested '<' */
//			      continue ;
///*			     Return value of form XMLLit:xxxx */
//			     if (vllt->tcb->literal_len > V4DPI_UCVal_Max)
//			      vllt->tcb->UCstring[V4DPI_UCVal_Max-3] = UCEOS ;
//			     { LENMAX len = UCstrlen(vllt->tcb->UCstring) ;
//			       if (len >= V4DPI_UCVal_Max - 1)		/* Save as string or BigText ? */
//				{ vllt->tcb->UCstring[V4LEX_BigText_Max-1] = UCEOS ; v4dpi_SaveBigTextPoint2(ctx,vllt->tcb->UCstring,point,Dim_Alpha,TRUE) ;
//				} else { uccharPNTvl(point,vllt->tcb->UCstring,len) ; } ;
//			     }
//			     return(1) ;
			 } ;
		 } ;
	 }
		 return(1) ;
		      case XML_State_StartEndLvl:					/* Maybe starting literal value or <xxx> or </xxx> or whatever - look at next token & decide */
			v4lex_NextTkn(vllt->tcb,flags) ;
			switch(vllt->tcb->opcode)
			 { default:			XERR(190,XML_Error_Syntax) ;
			   case V_OpCode_Numeric:
			   case V_OpCode_Keyword:	v4lex_NextTkn(vllt->tcb,V4LEX_Option_PushCur) ; XS(LookForLitVal) ; continue ;
			   case V_OpCode_EOF:		goto hit_eof ;
			   case V_OpCode_EOX:		continue ;
			   case V_OpCode_LangleBang:	XS(InCom) ; continue ;
			   case V_OpCode_Langle:	XS(StartLvl) ; continue ;	/* Nesting down to new level */
			   case V_OpCode_LangleSlash:	XS(EndLvl) ; continue ;
			   case V_OpCode_LangleBangDashDash: XS(InCom1) ; continue ;
			 } ;
		      case XML_State_EndLvl:
			v4lex_NextTkn(vllt->tcb,flags) ;
			switch (vllt->tcb->opcode)
			 { default:			XERR(200,XML_Error_Syntax) ;
			   case V_OpCode_EOF:		goto hit_eof ;
			   case V_OpCode_EOX:		continue ;
			   case V_OpCode_Keyword:
/*			     Make sure UCkeyword matches current level, pop off, and return XMLEnd:xxx */
			     res = 0 ;
			     if (lxc->nestLvl <= 0) XERR(210,XML_Error_Syntax) ;
			     UCstrcpy(lname,vllt->tcb->UCkeyword) ; v4lex_NextTkn(vllt->tcb,flags) ;
			     if (vllt->tcb->opcode == V_OpCode_Colon)
			      { v4lex_NextTkn(vllt->tcb,flags) ;
			        if (vllt->tcb->opcode != V_OpCode_Keyword) XERR(220,XML_Error_InvName) ;
			      } else { v4lex_NextTkn(vllt->tcb,V4LEX_Option_PushCur) ; UCstrcpy(vllt->tcb->UCkeyword,lname) ; } ;
			     for(;lxc->nestLvl>=0;lxc->nestLvl--)
			      { if (UCstrcmpIC(vllt->tcb->UCkeyword,lxc->Lvl[lxc->nestLvl-1].lvlName) == 0) break ;
			        if (listp->ListType == V4L_ListType_HTMLTokens) 
				 { int hx ;
				   for(hx=0;hx<V4LI_HTMLMax;hx++)
				    { if (UCstrcmpIC(lxc->Lvl[lxc->nestLvl-1].lvlName,gpi->ci->li->LI[gpi->ci->li->CurX].htmlTag[hx]) == 0) break ;
				    } ; if (hx < V4LI_HTMLMax) { res |= V4L_AutoEndLvl ; continue ; } ;
				 } ;
			        XERR(230,XML_Error_NameMatch) ;
			      } ;
			     if (lxc->nestLvl > 0) lxc->nestLvl-- ;
//			     DIMINFO(di,ctx,v4dpi_DimGet(ctx,UClit("XMLEnd"))) ;
			     dictPNTv(point,diXMLEnd->DimId,v4dpi_DictEntryGet(ctx,Dim_NId,lxc->Lvl[lxc->nestLvl].lvlName,diXMLEnd,NULL)) ;
			     v4lex_NextTkn(vllt->tcb,flags) ;
			     if (vllt->tcb->opcode != V_OpCode_Rangle) XERR(240,XML_Error_Syntax) ;
			     XS(StartEndLvl) ; return(res | 1) ;
			 } ;
		      case XML_State_EOM:
			break ;
		      case XML_State_InHdr:
			for(;lxc->xmlState==XML_State_InHdr;)	/* Not handling '<? xxx ?>' as of yet */
			 { v4lex_NextTkn(vllt->tcb,flags) ;
			   switch (vllt->tcb->opcode)
			    { default:				continue ;
			      case V_OpCode_QuestionRangle:	XS(BOM) ; break ;	/* Pretend we are at bom to look for '<' */
			      case V_OpCode_EOF:		goto hit_eof ;
			      case V_OpCode_EOX:		continue ;
			    } ;
			 } ;
			continue ;
		    } ;
hit_eof:
/*		   Hit 'eof' - if source is list then grab next element & continue, otherwise we are done */
		   if (!v4l_XMLHandleEOF(ctx,vllt,point,traceGlobal)) goto at_end ;
		 } ;
hit_error:	if (vllt->listPt != NULL)
		 { v_Msg(ctx,msg,"ParseXMLSrcPos",vllt->listPtIndex,vllt->listPt) ; } else { ZUS(msg) ; } ;
		switch(ok)
		 { case XML_Error_InvName:	v_Msg(ctx,ctx->ErrorMsgAux,"ParseXMLErr1",msg) ; break ;
		   case XML_Error_TooMnyLvl:	v_Msg(ctx,ctx->ErrorMsgAux,"ParseXMLErr2",V4LEX_XML_LvlMax,msg) ; break ;
		   case XML_Error_Syntax:	v_Msg(ctx,ctx->ErrorMsgAux,"ParseXMLErr3",msg) ; break ;
		   case XML_Error_NameMatch:	v_Msg(ctx,ctx->ErrorMsgAux,"ParseXMLErr4",vllt->tcb->UCkeyword,lxc->Lvl[lxc->nestLvl-1].lvlName,msg) ; break ;
		   case XML_Error_List:		v_Msg(ctx,ctx->ErrorMsgAux,"ParseXMLErr5",msg) ; break ;
		 } ;
		return(LISTVALERR) ;

/*		At end of list - free up stuff and return 0 */
at_end:		v4l_ListClose(ctx,listp) ;
		return(0) ;

	   case V4L_ListType_AggRef:
		lar = (struct V4L__ListAggRef *)&listp->Buffer[0] ;
		for(bx=0,num=index;bx<lar->Count;bx++)
		 { if (num > (lar->Agg[bx].EndPt - lar->Agg[bx].BeginPt + 1))
		    { num -= (lar->Agg[bx].EndPt - lar->Agg[bx].BeginPt + 1) ; continue ; } ;
//v_Msg(ctx,UCTBUF1,"@--lazy bx=%1d, lar->Count=%2d, num=%3d, areaAggX=%4d\n",bx,lar->Count,num,lar->AreaAggIndex[bx]) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
//if (lar->Count > 8 && lar->AreaAggIndex[8] < 1 || lar->AreaAggIndex[8] > 40)
//printf("??????????????? aggIndex[8] = %d, ctx->IsctEvalCount=%d\n",lar->AreaAggIndex[8],ctx->IsctEvalCount) ;
		   ZPH(point) ; point->PntType = V4DPI_PntType_AggRef ;
		   point->Bytes = V4PS_Int ;
		   point->Grouping = lar->AreaAggIndex[bx] ;		/* Plug proper index to Agg Area */
		   point->Dim = lar->Dim ;
		   point->Value.IntVal = lar->Agg[bx].BeginPt + num - 1 ;	/* Get Agg key value */
		   return(1) ;
		 } ;
		return(0) ;							/* If here could not find point? */
	   case V4L_ListType_Isct:
		lisct = (struct V4L__ListIsct *)&listp->Buffer[0] ;		/* Link to isct, return until undefined */
		pt = (P *)v4dpi_PntIdx_CvtIdxPtr(lisct->pix) ;
		if (v4dpi_IsctEval(point,pt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) == NULL) return(0) ;
		listp->Entries ++ ; return(1) ;
	   case V4L_ListType_Lazy:
		vll = (struct V4L__ListLazy *)&listp->Buffer[0] ;		/* Link to vll */
		pt = (P *)v4im_DoEnumCL(ctx,point,NULL,0,30 /* V4IM_OpCode_EnumCL */,traceGlobal,vll->lli,&ok) ;
		if (!ok) { UCstrcpy(ctx->ErrorMsgAux,ctx->ErrorMsg) ; return(LISTVALERR) ; } ;
		if (pt != point && pt != NULL) memcpy(point,pt,pt->Bytes) ;
		if (pt != NULL) return(1) ;
/*		End of list - deallocate everything */
		v4l_ListClose(ctx,listp) ;
		return(0) ;
	   case V4L_ListType_V4IS:
		lv4 = (struct V4L__ListV4IS *)&listp->Buffer[0] ;
		v4a = gpi->v4a ;					/* Pull v4a from process info */
		if (v4a == NULL) return(0) ;				/* No V4IS areas? */
		for(i=0;i<v4a->Count;i++) { if (lv4->Handle == v4a->Area[i].Handle) break ; } ;
		if (i >= v4a->Count)	return(0) ;			/* Could not find area? */
		if (lv4->Number != UNUSED)				/* See if exceeded max number of records */
		 { if (v4a->Area[i].Count >= lv4->Number) return(0) ; } ;
		if (lv4->Ifpix != UNUSED)
		 { tpt1 = (struct V4DPI__Point *)v4dpi_PntIdx_CvtIdxPtr(lv4->Ifpix) ; iswhile = FALSE ; }
		 else if (lv4->Whilepix != UNUSED)
		 { tpt1 = (struct V4DPI__Point *)v4dpi_PntIdx_CvtIdxPtr(lv4->Whilepix) ; iswhile = TRUE ; }
		 else { tpt1 = NULL ; iswhile = FALSE ; } ;
		tpt2 = (lv4->Subpix != UNUSED ? (struct V4DPI__Point *)v4dpi_PntIdx_CvtIdxPtr(lv4->Subpix) : NULL) ;
		for(;;)
		 { if (lv4->CountOffset != UNUSED && v4a->Area[i].DataId != UNUSED)	/* Looping thru sub-structure? */
		    { if (lv4->CountBytes == 2) { smax = (short *)v4a->Area[i].RecBuf + lv4->CountOffset ; max = *smax ; }
		       else { imax = (int *)v4a->Area[i].RecBuf + lv4->CountOffset ; max = *imax ; } ;
		      if (v4a->Area[i].SubIndex >= max) goto next_rec ;	/* Over maximum substructure count for this record */
		      ZPH(point) ; point->PntType = V4DPI_PntType_V4IS ; point->Dim = listp->Dim ;
		      point->Bytes = V4DPI_PointHdr_Bytes + sizeof(*pis) ;
		      pis = (struct V4DPI__PntV4IS *)&point->Value ;	/* Link up to V4IS info */
		      pis->Handle = v4a->Area[i].Handle ; pis->DataId = v4a->Area[i].DataId ;
		      pis->SubIndex = ++v4a->Area[i].SubIndex ;
		      if (tpt2 != NULL)					/* Have a record to test? */
		       { if (!v4ctx_FrameAddDim(ctx,0,point,0,0)) return(0) ;		/* Add new V4IS point to current context */
			 CLEARCACHE
			 pt = v4dpi_IsctEval(point,tpt2,ctx,0,NULL,NULL) ;
			 if (pt == NULL) continue ;			/* Record no good - try next */
			 if (pt->Value.IntVal <= 0) continue ;		/* FALSE - ditto, continue with next point */
		       } ;
		      v4a->Area[i].Count++ ; return(1) ;
		    } ;
next_rec:	   pcb = v4a->Area[i].pcb ;				/* Have to get another record */
		   if (index == 1 && lv4->Firstpix != UNUSED)
		    { pt = v4dpi_IsctEval(point,(struct V4DPI__Point *)v4dpi_PntIdx_CvtIdxPtr(lv4->Firstpix),ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		      if (pt == NULL) return(0) ;
		    } else
		    { switch (lv4->KeyNum)
		       { case 0:
			   pcb->GetMode = V4IS_PCB_GP_DataOnly | (V4IS_PCB_NoLock|V4IS_PCB_IgnoreLock|V4IS_PCB_NoError) ; break ;
		         default:
			   pcb->GetMode = V4IS_PCB_GP_Next | (V4IS_PCB_NoLock|V4IS_PCB_IgnoreLock|V4IS_PCB_NoError) ; break ;
		       } ;
		      v4is_Get(pcb) ;
		      if (pcb->GetLen == UNUSED) return(0) ;	/* End of the line - no more */
		    } ;
		   v4a->Area[i].DataId = pcb->DataId ;		/* Save current dataid */
		   ZPH(point) ; point->PntType = V4DPI_PntType_V4IS ; point->Dim = listp->Dim ;
		   point->Bytes = V4DPI_PointHdr_Bytes + sizeof(*pis) ;
		   pis = (struct V4DPI__PntV4IS *)&point->Value ;	/* Link up to V4IS info */
		   pis->Handle = v4a->Area[i].Handle ; pis->DataId = v4a->Area[i].DataId ; pis->SubIndex = UNUSED ;
		   if (tpt1 != NULL)					/* Have a record to test? */
		    { if (!v4ctx_FrameAddDim(ctx,0,point,0,0)) return(0) ;		/* Add new V4IS point to current context */
		      CLEARCACHE
		      pt = v4dpi_IsctEval(point,tpt1,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		     if (pt == NULL ? TRUE : pt->Value.IntVal <= 0)	/* Record no good? */
		      { v4a->Area[i].DataId = UNUSED ; if (iswhile) { return(0) ; } else { continue ; } ; } ;
		    } ;
		   v4a->Area[i].SubIndex = 0 ;
		   if (lv4->CountOffset == UNUSED) break ;
		   continue ;					/* Go grab substructure */
		 } ;
		v4a->Area[i].Count++ ; return(1) ;
	   case V4L_ListType_XDBGet:
		{ INDEX sx ;
		  log = (struct V4L__ListXDBGet *)&listp->Buffer[0] ;
		  ZPH(point) ;
		  
		  switch (XDBGETDBACCESS(log->xdbId))
		   {
#ifdef WANTODBC
		     case XDBODBC:	sx = v4odbc_Fetch(ctx,log->xdbId,V4ODBC_FetchNext,&ok,0,point) ; break ;
#endif
#ifdef WANTV4IS
		     case XDBV4IS:	sx = v4midas_Fetch(ctx,log->xdbId,V4ODBC_FetchNext,&ok,0,point) ; break ;
#endif
#ifdef WANTMYSQL
		     case XDBMYSQL:	sx = v4mysql_Fetch(ctx,log->xdbId,V4ODBC_FetchNext,&ok,0,point) ; break ;
#endif
#ifdef WANTORACLE
		     case XDBORACLE:	sx = v4oracle_Fetch(ctx,log->xdbId,V4ODBC_FetchNext,&ok,0,point) ; break ;
#endif
		   } ;


		  if (!ok)
		   return(LISTVALERR) ;
		  if (sx < 0) return(0) ;
		  if (point->Bytes == 0 || memcmp(point,&protoNone,V4PS_Int) == 0)	/* Did we update point via fetchPnt evaluation ? */
		   { ZPH(point) ;
/*		     If no target dimension then return XDB point, otherwise return point on target dim */
		     point->PntType = V4DPI_PntType_XDB ; point->Dim = listp->Dim ;	/* No - then update with this row */
		     point->Bytes = V4PS_XDB ;
//		     point->Value.XDB.rowNum = gpi->xdb->stmts[sx].curRow ;
		     point->Value.XDB.recId = gpi->xdb->stmts[sx].curRecId ; point->Value.XDB.xdbId = log->xdbId ;
		     if (gpi->xdb->stmts[sx].targetDimId != UNUSED)
		      { 
/*			Update target dimension with first record id */
		        DIMINFO(di,ctx,gpi->xdb->stmts[sx].targetDimId) ; di->XDB.lastRecId = gpi->xdb->stmts[sx].curRecId ;
//printf("gpi=%p, gpi->xdb=%p, sx=%d, vxssi=%p, lasRecId=%d (%p)\n",gpi,gpi->xdb,sx,gpi->xdb->stmts[sx].vxssi,gpi->xdb->stmts[sx].vxssi->lastRecId,&gpi->xdb->stmts[sx].vxssi->lastRecId) ;
		        gpi->xdb->stmts[sx].vxssi->lastRecId = gpi->xdb->stmts[sx].curRecId ;
		        intPNTv(point,gpi->xdb->stmts[sx].curRecId) ; point->Dim = gpi->xdb->stmts[sx].targetDimId ;
		      } ;
		   } ;
		}									/* If we did then ODBC point already in context */
		return(1) ;
	   case V4L_ListType_HugeInt:
		if (index > listp->Entries) return(0) ;	/* Can't return what we don't got! */
		vlh = (struct V4L__ListHugeInt *)&listp->Buffer[0] ;
		for(bx=0,num=index;bx<vlh->Blocks;bx++)
		 { if ((num -= vlh->Block[bx].ElCount) <= 0) break ; } ;
		num += vlh->Block[bx].ElCount ;		/* Get correct block */
		vbe = v4l_Getvbe(ctx,vlh->RelHNum,vlh->Block[bx].BlockValueNum) ;
		ZPH(point) ; point->Dim = listp->Dim ; point->PntType = listp->PntType  ;
		if (listp->PntType == V4DPI_PntType_Real || listp->PntType == V4DPI_PntType_Int2 || listp->PntType == V4DPI_PntType_Calendar)
		 { lda = (struct V4L__ListDblArray *)&vbe->PointValBuf ;
		   PUTREAL(point,lda->PointDblVal[num-1]) ;
//		   memcpy(&point->Value.RealVal,&lda->PointDblVal[num-1],sizeof(double)) ;
		   point->Bytes = V4PS_Real ;
		 } else
		 { lia = (struct V4L__ListIntArray *)&vbe->PointValBuf ;
		   point->Value.IntVal = lia->PointIntVal[num-1] ;
		   if (point->PntType == V4DPI_PntType_AggRef)		/* Have to break grouping out of IntVal */
		    { point->Grouping = point->Value.IntVal >> 24 ; point->Value.IntVal &= 0xffffff ;
		    } ;
		   point->Bytes = V4PS_Int ;
		 } ;
		return(1) ;
	   case V4L_ListType_HugeGeneric:
		if (index > listp->Entries) return(0) ;	/* Can't return what we don't got! */
		vlhg = (struct V4L__ListHugeGeneric *)&listp->Buffer[0] ;
		for(bx=0,num=index;bx<vlhg->Blocks;bx++)
		 { if ((num -= vlhg->Block[bx].ElCount) <= 0) break ; } ;
		num += vlhg->Block[bx].ElCount ;		/* Get correct block */
		vbeg = v4l_Getvbeg(ctx,vlhg->RelHNum,vlhg->Block[bx].BlockValueNum) ;
//VEH040320 - This is bad - have to return error in a better way then "end of list"
		if (vbeg == NULL) return(FALSE) ;
		for(tpt1=(P *)&vbeg->PointValBuf,i=1;i<num;i++)
		 { ADVPNT(tpt1) ; } ;	/* Advance to correct point in vbeg */
		memcpy(point,tpt1,tpt1->Bytes) ;
		return(1) ;
	   case V4L_ListType_CmpndRange:
		if (index > listp->Entries) return(0) ;	/* Can't return what we don't got! */
		lcr = (struct V4L__ListCmpndRange *)&listp->Buffer[0] ;
		for(num=index,bx=0;bx<lcr->Count;bx++)		/* Loop thru all entries until we get to correct one */
		 { num -= ((lcr->Cmpnd[bx].End - lcr->Cmpnd[bx].Begin)/lcr->Cmpnd[bx].Increment + 1) ;	/* Is the the compound we want ? */
		   if (num > 0) continue ;					/*  Not yet! */
		   ZPH(point) ; point->Dim = listp->Dim ; point->PntType = listp->PntType  ;
		   if (lcr->Cmpnd[bx].Increment != 1)
		    { num += ((lcr->Cmpnd[bx].End - lcr->Cmpnd[bx].Begin)/lcr->Cmpnd[bx].Increment + 1) ;
		      point->Value.IntVal = lcr->Cmpnd[bx].Begin + (num-1)*lcr->Cmpnd[bx].Increment ;
		    } else { point->Value.IntVal = lcr->Cmpnd[bx].End + num ; } ;		/* This should be correct? */
		   point->Bytes = V4PS_Int ;
		  return(1) ;
		 } ;
	   case V4L_ListType_CmpndRangeDBL:
		if (index > listp->Entries) return(0) ;	/* Can't return what we don't got! */
		lcrd = (struct V4L__ListCmpndRangeDBL *)&listp->Buffer[0] ;
		for(num=index,bx=0;bx<lcrd->Count;bx++)		/* Loop thru all entries until we get to correct one */
		 { memcpy(&d1,&lcrd->Cmpnd[bx].Begin,sizeof d1) ; memcpy(&d2,&lcrd->Cmpnd[bx].End,sizeof d1) ;
		   memcpy(&d3,&lcrd->Cmpnd[bx].Increment,sizeof d1) ;
		   num -= DtoI((d2 - d1)/d3 + 1) ;	/* Is the the compound we want ? */
		   if (num > 0) continue ;					/*  Not yet! */
		   ZPH(point) ; point->Dim = listp->Dim ; point->PntType = listp->PntType  ;
		   if (d3 != 1.0)
		    { num += DtoI((d2 - d1)/d3 + 1) ; d1 = d1 + (num-1)*d3 ;
		    } else { d1 = d2 + num ; } ;		/* This should be correct? */
		   PUTREAL(point,d1) ;
//		   memcpy(&point->Value.RealVal,&d1,sizeof d1) ;
		   point->Bytes = V4PS_Real ;
		   return(1) ;
		 } ;
	   case V4L_ListType_BitMap:
		lbm = (struct V4L__ListBitMap *)&listp->Buffer[0] ;
		COPYPTR(bm1,lbm->bm1) ;
		if (index != lbm->LastIndex+1)			/* If continuing where we left off then can optimize */
		 { i = 0 ; j = i/32 ; num = bm1->Seg32[j] ;	/* i = bit, j = corresponding 32 bit word */
		   bx = 0 ;					/* bx = number of bits found */
		   for(;i<=bm1->MaxBit;)
		    { for(;num!=0;) { i++ ; if (num & 1) break ; num = num >> 1 ; } ;
		      if (num == 0) { i = (++j)*32 ; num = bm1->Seg32[j] ; continue ; } ;
		      if (++bx >= index)			/* Found bit, is it the one we want? */
		       { if (i > bm1->MaxBit) return(0) ;
		         ZPH(point) ; point->Dim = listp->Dim ; point->PntType = listp->PntType  ;
			 point->Bytes = V4PS_Int ;
			 point->Value.IntVal = i ;
			 lbm->LastBit = i ; lbm->LastIndex = index ; return(1) ;
		       } ;
		      num = num >> 1 ;					/* Not yet - continue ; */
		    } ;
		   return(0) ;					/* Ran out of bits */
		 } else
		 { i = lbm->LastBit ; j = i/32 ;		/* i = bit, j = corresponding 32 bit word */
		   num = bm1->Seg32[j] ; num = num >> (i % 32) ;
		   bx = index-1 ;				/* bx = number of bits found */
		   for(;i<=bm1->MaxBit;)
		    { for(;num!=0;) { i++ ; if (num & 1) break ; num = num >> 1 ; } ;
		      if (num == 0) { i = (++j)*32 ; num = bm1->Seg32[j] ; continue ; } ;
		      if (++bx >= index)			/* Found bit, is it the one we want? */
		       { if (i > bm1->MaxBit) return(0) ;
		         ZPH(point) ; point->Dim = listp->Dim ; point->PntType = listp->PntType  ;
			 point->Bytes = V4PS_Int ;
			 point->Value.IntVal = i ;
			 lbm->LastBit = i ; lbm->LastIndex = index ; return(1) ;
		       } ;					/* Not yet - continue ; */
		      num = num >> 1 ;					/* Not yet - continue ; */
		    } ;
		   return(0) ;					/* Ran out of bits */
		 } ;
	   case V4L_ListType_MultiAppend:
		if (index > listp->Entries) return(0) ;		/* Can't return what we don't got! */
		vlma = (struct V4L__ListMultiAppend *)&listp->Buffer[0] ;
		for(num=index,bx=0;bx<vlma->Count;bx++)		/* Loop thru all entries until we get to correct one */
		 { if (num - vlma->Multi[bx].Entries > 0) { num -= vlma->Multi[bx].Entries ; continue ; } ;
		   { struct V4L__ListPoint *xxx ; COPYPTR(xxx,vlma->Multi[bx].lp) ;
		     return(v4l_ListPoint_Value(ctx,xxx,num,point)) ;
		   }
		 } ;
		return(0) ;
	   case V4L_ListType_MDArray:
		if (index > listp->Entries) return(0) ;		/* Can't return what we don't got! */
		lmda = (struct V4L__ListMDArray *)&listp->Buffer[0] ;
		vmda = (struct V4IM__MDArray *)han_GetPointer(lmda->han_MDArray,0) ;
		bx = 0 ; index -= 1 ;
		for(i=1;i<vmda->MDDimCount;i++)
		 { for(num=1,j=i;j<vmda->MDDimCount;j++) { num *= (lmda->MDDim[j].End - lmda->MDDim[j].Start + 1)  ; } ;
		   bx += ((index / num) + lmda->MDDim[i-1].Start) * vmda->MDDim[i-1].Offset ; index = (index % num) ;
		 } ;
		bx += (index + lmda->MDDim[i-1].Start) * vmda->MDDim[i-1].Offset ;
		ZPH(point) ; point->Dim = vmda->Dim ; point->PntType = vmda->PntType ;
		point->Bytes = gpi->PointBytes[vmda->PntType] ; memcpy(&point->Value,bx+vmda->Data,vmda->ElBytes) ;
		return(1) ;
#ifdef WANTHTMLMODULE
	   case V4L_ListType_HTMLTable:
		if (index > listp->Entries) return(0) ;		/* Can't return what we don't got! */
		lht = (struct V4L__ListHTMLTable *)&listp->Buffer[0] ;
		if (lht->NumRows != UNUSED)
		 { if (!v4html_PositionRowColumn(lht->vhp,index,UNUSED,NULL)) return(0) ;
		 } else
		 { if (!v4html_PositionRowColumn(lht->vhp,UNUSED,index,NULL)) return(0) ;
		 } ;
		ZPH(point) ; point->Dim = Dim_Int ; point->PntType = V4DPI_PntType_Int ;
		point->Bytes = V4PS_Int ; point->Value.IntVal = index ;
		return(1) ;
#endif
	   case V4L_ListType_TextFile:
		ltf = (struct V4L__ListTextFile *)&listp->Buffer[0] ;
		for(;;)
		 { if (ltf->fltrindex != UNUSED && ltf->fltrres != NULL)	/* Got a list of values - return next one */
		    { lp1 = (struct V4L__ListPoint *)&ltf->fltrres->Value ;
		      if (v4l_ListPoint_Value(ctx,lp1,++ltf->fltrindex,point) <= 0) { ltf->fltrindex = UNUSED ; continue ; } ;
		      return(1) ;
		    } ;
/*		   See if we have previously read very long line (length > size of UC point). If not read line otherwise grab next chunk */
		   if (ltf->line == NULL)
		    { ltf->lineLen = v_UCReadLine(&ltf->UCFile,UCRead_UC,ltf->lineBuf,ltf->lineBufSize,ctx->ErrorMsgAux) ;
//if (ltf->lineLen > 32000) ltf->lineBuf[ltf->lineLen=32000] = 0 ;
		      if (ltf->lineLen < 0)
		       { goto close_textfile ; } ;
		      ltf->lineCount++ ; ltf->line = ltf->lineBuf ;
		    } ;
/*		   If we want line as-fast-as-possible & within max-length then just return pointer to it */
		   if (index == V4L_NextElAsTempPt && ltf->lineLen < V4TMBufMax)
		    { ZPH(point) ; point->PntType = V4DPI_PntType_MemPtr ; memcpy(&point->Value.MemPtr,&ltf->line,SIZEofPTR) ;
		      ltf->line = NULL ; ltf->lineLen = 0 ;
		      return(1) ;
		    } ;
/*		   Is line too long for point? try BigText, if still too big then break it apart */
		   if (ltf->lineLen >= V4DPI_UCVAL_MaxSafe)
		    { if (ltf->lineLen < V4LEX_BigText_Max)
		       { v4dpi_SaveBigTextPoint2(ctx,ltf->line,point,ltf->Dim,TRUE) ;
		         ltf->line = NULL ; ltf->lineLen = 0 ;
		       } else
		       { for(i=V4DPI_UCVal_Max-2;i>0;i--)		/* Look for a white-space to break on */
		          { if (vuc_IsWSpace(ltf->line[i])) break ;
			    if (ltf->line[i] == UClit('<'))
			     { break ; } ;
		          } ;
			   if (i <= 0) i = V4DPI_UCVAL_MaxSafe ;
//		         UCstrncpy(&point->Value.UCVal[1],ltf->line,i) ; point->Value.UCVal[i+1] = UCEOS ; point->Value.UCVal[0] = i ; uccharPNT(point) ;
		         uccharPNT(point) ; UCstrncpy(&point->Value.UCVal[1],ltf->line,i) ; UCCHARPNTBYTES2(point,i) ;
		         ltf->line += i ; ltf->lineLen -= i ;
		       } ;
		    } else
		    { uccharPNTv(point,ltf->line) ; ltf->line = NULL ; ltf->lineLen = 0 ;
		    } ;
		   point->Dim = ltf->Dim ;
		   if (ltf->fltr != NULL)		/* Got a filter for this list? */
		    { if (!v4ctx_FrameAddDim(ctx,0,point,0,0)) { v_Msg(ctx,NULL,"CtxAddFail",0) ; return(-1) ; } ;
		      ipt = v4dpi_IsctEval(ltf->fltrres,ltf->fltr,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		      if (ipt == NULL) { v_Msg(ctx,NULL,"FilterIsctFail",0,ltf->fltr) ; return(-1) ; } ;
		      if (memcmp(ipt,&protoNone,protoNone.Bytes) == 0)		/* If filter returns UV4:none then force immediate EOF */
		       goto close_textfile ;
		      switch(ipt->PntType)
		       { case V4DPI_PntType_Logical:
			   if (ipt->Value.IntVal > 0) { return(1) ; } else { continue ; } ;
			 CASEofChar
			   memcpy(point,ipt,ipt->Bytes) ; point->Dim = ltf->Dim ; return(1) ;
			 case V4DPI_PntType_List:
			   ltf->fltrindex = 0 ; continue ;	/* Retry with list stored in ltf->fltrres */
		       } ;
		    } ;
		   return(1) ;
		 } ;
close_textfile:
		v4l_ListClose(ctx,listp) ;
		return(0) ;
	   case V4L_ListType_Files:
		vlf = (struct V4L__ListFiles *)&listp->Buffer[0] ;
		for(;;)
		 { for(lfi=vlf->lfi;;lfi=lfi->nestlfi) { if (lfi->nestlfi == NULL) break ; } ;
		   if (!v_NextFile(lfi)) { v4l_ListClose(ctx,listp) ; return(0) ; } ;
		   for(lfi=vlf->lfi;;lfi=lfi->nestlfi) { if (lfi->nestlfi == NULL) break ; } ;
		   if (vlf->Bytespt.Dim != 0 && lfi->FileType != V4_ListFilesType_Directory)
		    { struct V4DPI__LittlePoint lfpt ;
		      ZPH(&lfpt) ; lfpt.Dim = Dim_Int ; lfpt.Bytes = V4PS_Int ;
		      lfpt.PntType = V4DPI_PntType_Int ; lfpt.Value.IntVal = lfi->FileBytes ;
		      if (!v4im_DoIn(ctx,(P *)&lfpt,(P *)&vlf->Bytespt,1,1)) continue ;
		    } ;
		   if (vlf->Updatept.Dim != 0 && lfi->FileType != V4_ListFilesType_Directory)
		    { struct V4DPI__LittlePoint lfpt ;
		      ZPH(&lfpt) ; lfpt.Dim = Dim_UDate ; lfpt.Bytes = V4PS_Int ;
		      lfpt.PntType = V4DPI_PntType_UDT ; lfpt.Value.IntVal = lfi->UpdateDT ;
		      if (!v4im_DoIn(ctx,(P *)&lfpt,(P *)&vlf->Updatept,1,1)) continue ;
		    } ;
		   break ;
		 } ;
		uccharPNTv(point,lfi->FileName) ; point->Dim = vlf->Dim ;
		return(1) ;
	 } ;
	if (index > listp->Entries) return(0) ;	/* Can't return what we don't got! */
	if (listp->BytesPerEntry > 0)		/* Have to index into value, build up components of point */
	 { INDEX len = listp->BytesPerEntry ;
if (len >= sizeof(point->Value))
 { v_Msg(ctx,NULL,"@*Bad listp->BytesPerEntry (%1d)\n",listp->BytesPerEntry) ; vout_UCText(VOUT_Err,0,ctx->ErrorMsg) ; len = sizeof(point->Value) - 100 ; } ;
	   ZPH(point) ; point->Dim = listp->Dim ; point->PntType = listp->PntType  ;
	   point->Bytes = V4DPI_PointHdr_Bytes + len ;
	   memcpy(&point->Value,&listp->Buffer[(index-1)*len],len) ;
	   return(1) ;			/* That was relatively painless */
	 } ;
/*	Have to find value in index list, may have entire point or just part of it */
	lpix = (struct V4L__ListPointIndex *)&listp->Buffer[listp->LPIOffset] ;
	bx = lpix->Offset[index-1] ;
	if (listp->Dim > 0)
	 { ZPH(point) ; point->Dim = listp->Dim ; point->PntType = listp->PntType  ;
	   lpj = (struct V4L__ListPoint_Jacket *)&listp->Buffer[bx] ; point->Bytes = V4DPI_PointHdr_Bytes + lpj->Bytes ;
if (lpj->Bytes >= sizeof(point->Value))
 { v_Msg(ctx,NULL,"@*Bad lpj->Bytes (%1d)\n",lpj->Bytes) ; vout_UCText(VOUT_Err,0,ctx->ErrorMsg) ; } ;
	   memcpy(&point->Value,lpj->Buffer,lpj->Bytes) ;
	   return(1) ;
	 } ;
/*	Stored entire point - copy & return */
	pt = (P *)&listp->Buffer[bx] ;		/* Link up to the point */
if (pt->Bytes > sizeof(*point))
 { v_Msg(ctx,NULL,"@*Bad pt->Bytes (%1d)\n",pt->Bytes) ; vout_UCText(VOUT_Err,0,ctx->ErrorMsg) ; } ;
	memcpy(point,pt,pt->Bytes) ;		/* & copy over */
	return(1) ;
}

/*	v4l_ListClose - Closes/Releases Resources associated with a list	*/
/*	Call: ok = v4l_ListClose(ctx,listp)
	  where ok is TRUE if list is closed OK (or already closed off),
	  	ctx is current context,
		listp is pointer to list					*/

LOGICAL v4l_ListClose(ctx,listp)
  struct V4C__Context *ctx ;
  struct V4L__ListPoint *listp ;
{
	switch (listp->ListType)
	 {
	   case V4L_ListType_TextTable:
		{ struct V4L__ListTextTable *ltt = (struct V4L__ListTextTable *)&listp->Buffer[0] ;
		  if(ltt->line != NULL)
		   { v_UCFileClose(&ltt->UCFile) ; 
		     v4mm_FreeChunk(ltt->line) ; ltt->line = NULL ; ltt->Count = V4LIM_SmallestNegativeInt ;
		     gpi->TextAgg[ltt->TextAggIndex].line = NULL ; gpi->TextAgg[ltt->TextAggIndex].vlt = NULL ;
		   } ;
		}
		return(TRUE) ;
	   case V4L_ListType_BigText:
		{ struct V4L__ListBigText *lbt = (struct V4L__ListBigText *)&listp->Buffer[0] ;
		  if (lbt->btBuf != NULL)
		   { v4mm_FreeChunk(lbt->btBuf) ; lbt->btBuf = NULL ; } ;
		}
		return(TRUE) ;
	   case V4L_ListType_JavaTokens:
		return(TRUE) ;
	   case V4L_ListType_Token:
		{ struct V4L__ListToken *vllt = (struct V4L__ListToken *)&listp->Buffer[0] ;
		  if (vllt->tcb != NULL) { v4lex_FreeTCB(vllt->tcb) ; vllt->tcb = NULL ; } ;
	        }
		return(TRUE) ;
	   case V4L_ListType_HTMLTokens:
	   case V4L_ListType_XMLTokens:
		{ struct V4L__ListToken *vllt = (struct V4L__ListToken *)&listp->Buffer[0] ;
		  if (vllt->tcb != NULL)
		   { v4lex_FreeTCB(vllt->tcb) ; vllt->tcb = NULL ; } ;
		}
		return(TRUE) ;
	   case V4L_ListType_AggRef:
		{ //struct V4L__ListAggRef *lar = (struct V4L__ListAggRef *)&listp->Buffer[0] ;
		}
		return(TRUE) ;
	   case V4L_ListType_Isct:
		return(TRUE) ;
	   case V4L_ListType_Lazy:
		{ struct V4L__ListLazy *vll = (struct V4L__ListLazy *)&listp->Buffer[0] ;
		  struct V4L__ListLazyInfo *lli,*tlli ; INDEX i ;
		  for(lli=vll->lli;lli!=NULL;)
		   { for(i=0;i<lli->aggCount;i++) { if (lli->Area[i].areaName != NULL) { printf("---lazy free #%d uid=%d\n",i,lli->Area[i].AggUId) ; v4mm_FreeChunk(lli->Area[i].areaName) ; } ; } ;
		     for(i=0;i<lli->actCount;i++) { if (lli->Act[i].pt != NULL) v4mm_FreeChunk(lli->Act[i].pt) ; } ;
		     tlli = lli->lliNext ; v4mm_FreeChunk(lli) ; lli = tlli ;
		   } ; vll->lli = NULL ;
		}
		return(TRUE) ;
	   case V4L_ListType_V4IS:
		{ //struct V4L__ListV4IS *lv4 = (struct V4L__ListV4IS *)&listp->Buffer[0] ;
		}
		return(TRUE) ;
	   case V4L_ListType_XDBGet:
		{ struct V4L__ListXDBGet *log = (struct V4L__ListXDBGet *)&listp->Buffer[0] ; LOGICAL ok ;
		  switch (XDBGETDBACCESS(log->xdbId))
		   {
#ifdef WANTODBC
		     case XDBODBC:	v4odbc_Fetch(ctx,log->xdbId,V4ODBC_FetchEOF,&ok,UNUSED,NULL) ; break ;
#endif
#ifdef WANTV4IS
		     case XDBV4IS:	v4midas_Fetch(ctx,log->xdbId,V4ODBC_FetchEOF,&ok,UNUSED,NULL) ; break ;
#endif
#ifdef WANTMYSQL
		     case XDBMYSQL:	v4mysql_Fetch(ctx,log->xdbId,V4ODBC_FetchEOF,&ok,UNUSED,NULL) ; break ;
#endif
#ifdef WANTORACLE
		     case XDBORACLE:	v4oracle_Fetch(ctx,log->xdbId,V4ODBC_FetchEOF,&ok,UNUSED,NULL) ; break ;
#endif
		   } ;
		}
		return(TRUE) ;
	   case V4L_ListType_HugeInt:
		return(TRUE) ;
	   case V4L_ListType_HugeGeneric:
		return(TRUE) ;
	   case V4L_ListType_CmpndRange:
		return(TRUE) ;
	   case V4L_ListType_CmpndRangeDBL:
		return(TRUE) ;
	   case V4L_ListType_BitMap:
		{ //struct V4L__ListBitMap *lbm = (struct V4L__ListBitMap *)&listp->Buffer[0] ;
		}
		return(TRUE) ;
	   case V4L_ListType_MultiAppend:
		return(TRUE) ;
	   case V4L_ListType_MDArray:
		return(TRUE) ;
#ifdef WANTHTMLMODULE
	   case V4L_ListType_HTMLTable:
		{ struct V4L__ListHTMLTable *lht = (struct V4L__ListHTMLTable *)&listp->Buffer[0] ;
		}
		return(TRUE) ;
#endif
	   case V4L_ListType_TextFile:
		{ struct V4L__ListTextFile *ltf = (struct V4L__ListTextFile *)&listp->Buffer[0] ;
		  if (ltf->lineBuf != NULL)
		   { v_UCFileClose(&ltf->UCFile) ;
		     if (ltf->fltr != NULL) v4mm_FreeChunk(ltf->fltr) ; ltf->fltr = NULL ;
		     v4mm_FreeChunk(ltf->lineBuf) ; ltf->lineBuf = NULL ;
		     if (ltf->fltrres != NULL) v4mm_FreeChunk(ltf->fltrres) ; ltf->fltrres = NULL ;
		   } ;
		}
		return(TRUE) ;
	   case V4L_ListType_Files:
		{ struct V4L__ListFiles *vlf = (struct V4L__ListFiles *)&listp->Buffer[0] ;
		  if (vlf->lfi != NULL)
		   { v4mm_FreeChunk(vlf->lfi) ; vlf->lfi = NULL ; } ;
		}
		return(TRUE) ;
	 } ;
	return(FALSE) ;
}

LOGICAL v4l_XMLHandleEOF(ctx,vllt,point,trace)
  struct V4C__Context *ctx ;
  struct V4L__ListToken *vllt ;
  P *point ;
  VTRACE trace ;
{
  struct V4L__ListPoint *lp ;
  UCCHAR *bp ; LOGICAL ok ;

	if (vllt->listPt == NULL)
	 return(FALSE) ;
	++vllt->listPtIndex ;
	lp = (struct V4L__ListPoint *)&vllt->listPt->Value ;
	if (lp->ListType == V4L_ListType_TextFile)	/* If we are reading from text file then play games to optimize */
	 { 
	   if (!v4l_ListPoint_Value(ctx,(struct V4L__ListPoint *)&vllt->listPt->Value,V4L_NextElAsTempPt,point))
	    return(FALSE) ;
	   if (point->PntType == V4DPI_PntType_MemPtr)
	    { memcpy(&bp,&point->Value.MemPtr,SIZEofPTR) ; ok = TRUE ;
	    } else
	    { v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,point,ctx) ; bp = UCTBUF1 ;
	    } ;
	 } else
	 { if (!v4l_ListPoint_Value(ctx,(struct V4L__ListPoint *)&vllt->listPt->Value,vllt->listPtIndex,point))
	    return(FALSE) ;
	   v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,point,ctx) ; bp = UCTBUF1 ;
	 } ;
	if (!ok)
	 { v_Msg(ctx,NULL,"ModInvArg",0,1) ; return(FALSE) ; } ;
	v4lex_NestInput(vllt->tcb,NULL,bp,V4LEX_InpMode_String) ;
	return(TRUE) ;
}



/*	v4l_ListPartition - Partition an AggRef list into 'parts' more-or-less equal subsets	*/
/*	Call: ok = v4l_ListPartition( ctx , listp , lptrn , parts , intmodx )
	  where ok is TRUE if all went well, FALSE if error (in ErrorMsg),
		ctx is context,
		listp is point representing list to be partitioned,
		lptrn is (struct V4L__Partitions *) and updated with partitioning results,
		parts is number of parts we want,
		intmodx is calling module							*/

LOGICAL v4l_ListPartition(ctx,lp,lprtn,parts,intmodx)
  struct V4C__Context *ctx ;
  struct V4L__ListPoint *lp ;
  struct V4L__Partitions *lprtn ;
  int parts,intmodx ;
{ struct V4L__ListAggRef *lar,*nlar ;
  int i,tcnt,pcnt,lx,px,pmin,mpx,startx,cursize ;
  int partindex[V4L_AreaAgg_Max] ;
  int partsize[V4L_Partition_Max] ;
  
	memcpy(&lprtn->lpbuf,lp,lp->Bytes) ;
	lprtn->Count = 0 ;
	if (parts > V4L_Partition_Max) parts = V4L_Partition_Max ; memset(&partsize,0,sizeof partsize) ;
	switch (lp->ListType)
	 { default:	v_Msg(ctx,NULL,"ListPartUnavail",intmodx,lp->ListType) ; return(FALSE) ;
	   case V4L_ListType_AggRef:
		lar = (struct V4L__ListAggRef *)&lp->Buffer[0] ;
		nlar = (struct V4L__ListAggRef *)&lprtn->lpbuf.Buffer[0] ;
		memset(&partindex,0,sizeof partindex) ;
/*		Get total number of points in this "list" */
		for(i=0,tcnt=0;i<lar->Count;i++) { tcnt += (lar->Agg[i].EndPt - lar->Agg[i].BeginPt + 1) ; } ;
		pcnt = tcnt / parts ;				/* pcnt = theoretical number of points per segment */
		for(px=1;px<=parts;px++)			/* First pass - allocate parts up to pcnt (but not over) */
		 { for(lx=0,cursize=0;lx<lar->Count;lx++)
		    { if (partindex[lx] != 0) continue ;	/* If this aggregate already assigned then skip */
		      if (cursize == 0 || cursize + (lar->Agg[lx].EndPt - lar->Agg[lx].BeginPt + 1) < pcnt)
		       { cursize += (lar->Agg[lx].EndPt - lar->Agg[lx].BeginPt + 1) ; partindex[lx] = px ; continue ; } ;
		    } ; partsize[px-1] = cursize ;
		 } ;
//printf("First Pass\n") ;
//for(px=0;px<parts;px++) printf("%d. size=%d\n",px+1,partsize[px]) ; ;
/*		First pass done, any agg segments still unassigned then dump into partition with smallest number of points */
		for(lx=0;lx<lar->Count;lx++)
		 { if (partindex[lx] != 0) continue ;
		   for(mpx=0,px=1,pmin=V4LIM_BiggestPositiveInt;px<=parts;px++)
		    { if (partsize[px-1] < pmin) { pmin = partsize[px-1] ; mpx = px ; } ; } ;
		   partindex[lx] = mpx ; partsize[mpx-1] += (lar->Agg[lx].EndPt - lar->Agg[lx].BeginPt + 1) ;
//printf("Adding %d to part %d making it now %d\n",(lar->Agg[lx].EndPt - lar->Agg[lx].BeginPt + 1),mpx,partsize[mpx-1]) ;
		 } ;
/*		Now create a new ListPoint with Aggs arranged by partition */
		nlar->Count = 0 ; startx = 1 ;
		for(px=1;px<=parts;px++)
		 { lprtn->Seg[px-1].Start = startx ; lprtn->Seg[px-1].aggIndexCnt = 0 ;
		   for(lx=0;lx<lar->Count;lx++)
		    { if (partindex[lx] != px) continue ;
		      lprtn->Count = px ;
		      nlar->AreaAggIndex[nlar->Count] = lar->AreaAggIndex[lx] ; nlar->Agg[nlar->Count] = lar->Agg[lx] ;
		      startx += (lar->Agg[lx].EndPt - lar->Agg[lx].BeginPt + 1) ;
/*		      Need to keep track of Agg indexes so that we can (maybe) reassign mmm */
		      lprtn->Seg[px-1].aggIndexes[lprtn->Seg[px-1].aggIndexCnt++] = lar->AreaAggIndex[lx] ;
		      nlar->Count++ ;
		    } ;
		   lprtn->Seg[px-1].End = startx - 1 ;
		 } ;
//printf("Final\n") ;
//for(lx=0,cursize=0;lx<nlar->Count;lx++)
// { cursize += (nlar->Agg[lx].EndPt - nlar->Agg[lx].BeginPt + 1) ;
//   printf("Area: %s, size=%d, total=%d\n",UCretASC(gpi->AreaAgg[nlar->AreaAggIndex[lx]].pcb->UCFileName),(nlar->Agg[lx].EndPt - nlar->Agg[lx].BeginPt + 1),cursize) ;
// } ;
//for(px=0;px<lprtn->Count;px++) printf("%d. %d..%d (%d)\n",px+1,lprtn->Seg[px].Start,lprtn->Seg[px].End,lprtn->Seg[px].End-lprtn->Seg[px].Start+1) ;
		break ;


	 } ;
	return(TRUE) ;
}

/*	v4l_BitMapAlloc - Allocates buffer for a bitmap			*/
/*	Call: ptr = v4l_BitMapAlloc( type , bits )
	  where ptr is pointer to bitmap buffer,
		type is bitmap type (currently unused s/b 0),
		bits is number of bits we need				*/

struct V4L__ListBMData1 *v4l_BitMapAlloc(type,bits)
  int type,bits ;
{ 
#define BM_MAPALLOC_MAX 500		/* Allocate 500 bitmaps & then start cleaning up */
 static int maxcnt = 0 ;
 static struct V4L__ListBMData1 *mapptr[BM_MAPALLOC_MAX],*newptr ;

#ifdef V4ENABLEMULTITHREADS
 static DCLSPINLOCK bmLock = UNUSEDSPINLOCKVAL ;

	GRABMTLOCKnoCTX(bmLock) ;
#endif
	if (maxcnt > BM_MAPALLOC_MAX) v4mm_FreeChunk(mapptr[maxcnt % BM_MAPALLOC_MAX]) ;
	newptr = (struct V4L__ListBMData1 *)v4mm_AllocChunk(BM1_StructBytes(bits),FALSE) ;
	mapptr[(maxcnt++) % BM_MAPALLOC_MAX] = newptr ;

#ifdef V4ENABLEMULTITHREADS
	FREEMTLOCK(bmLock) ;
#endif
	return(newptr) ;
}

/*	v4l_BM1And - Logically and's two bitmaps			*/
/*	Call: bm1 = v4l_BM1And( s1bm1 , s2bm1 )
	  where bm1 is resulting bitmap,
		s1bm1,s2bm1 are two source bitmaps			*/

struct V4L__ListBMData1 *v4l_BM1And(s1bm1,s2bm1)
  struct V4L__ListBMData1 *s1bm1,*s2bm1 ;
{ struct V4L__ListBMData1 *bm1 ;
  int mb,i,maxi ;

	mb = (s1bm1->MaxBit<=s2bm1->MaxBit?s1bm1->MaxBit:s2bm1->MaxBit) ;
	bm1 = v4l_BitMapAlloc(0,BM1_CalcMaxBit(mb)) ; bm1->MaxBit = BM1_CalcMaxBit(mb) ;		/* Allocate number of bits */
//	maxi = (mb+31) / 32 ;
	maxi = BM1_MapWord32(mb) ;
	for(i=0;i<maxi;i++) { bm1->Seg32[i] = s1bm1->Seg32[i] & s2bm1->Seg32[i] ; } ;
	return(bm1) ;
}

/*	v4l_BM1Or - Logically or's two bitmaps			*/
/*	Call: bm1 = v4l_BM1And( s1bm1 , s2bm1 )
	  where bm1 is resulting bitmap,
		s1bm1,s2bm1 are two source bitmaps			*/

struct V4L__ListBMData1 *v4l_BM1Or(s1bm1,s2bm1)
  struct V4L__ListBMData1 *s1bm1,*s2bm1 ;
{ struct V4L__ListBMData1 *bm1 ;
  int mb,i,maxi,max1,max2 ;

	mb = (s1bm1->MaxBit>s2bm1->MaxBit?s1bm1->MaxBit:s2bm1->MaxBit) ;
	bm1 = v4l_BitMapAlloc(0,BM1_CalcMaxBit(mb)) ; bm1->MaxBit = BM1_CalcMaxBit(mb) ;		/* Allocate number of bits */
//	maxi = (mb+31) / 32 ;
	maxi = BM1_MapWord32(mb) ;
	max1 = BM1_MapWord32(s1bm1->MaxBit) ; max2 = BM1_MapWord32(s2bm1->MaxBit) ;
	for(i=0;i<maxi;i++) { bm1->Seg32[i] = (i>=max1? 0:s1bm1->Seg32[i]) | (i>=max2? 0:s2bm1->Seg32[i]) ; } ;
	return(bm1) ;
}

/*	v4l_BM1Minus - Subtracts second map from first			*/
/*	Call: bm1 = v4l_BM1Minus( s1bm1 , s2bm1 )
	  where bm1 is resulting bitmap,
		s1bm1,s2bm1 are two source bitmaps			*/

struct V4L__ListBMData1 *v4l_BM1Minus(s1bm1,s2bm1)
  struct V4L__ListBMData1 *s1bm1,*s2bm1 ;
{ struct V4L__ListBMData1 *bm1 ;
  int mb,i,maxi ;

	mb = (s1bm1->MaxBit>s2bm1->MaxBit?s1bm1->MaxBit:s2bm1->MaxBit) ;
	bm1 = v4l_BitMapAlloc(0,BM1_CalcMaxBit(mb)) ; bm1->MaxBit = BM1_CalcMaxBit(mb) ;		/* Allocate number of bits */
//	maxi = (mb+31) / 32 ;
	maxi = BM1_MapWord32(mb) ;
	for(i=0;i<maxi;i++)
	 { bm1->Seg32[i] = (i>=s1bm1->MaxBit/32? 0:s1bm1->Seg32[i]) & ~(i>=s2bm1->MaxBit/32? 0:s2bm1->Seg32[i]) ; } ;
	return(bm1) ;
}

/*	v4l_BM1Not - Logically complements a bitmap			*/
/*	Call: bm1 = v4l_BM1And( s1bm1 )
	  where bm1 is resulting bitmap,
		s1bm1 is source bitmap					*/

struct V4L__ListBMData1 *v4l_BM1Not(s1bm1)
  struct V4L__ListBMData1 *s1bm1 ;
{ struct V4L__ListBMData1 *bm1 ;
  int mb,i,maxi ;

	mb = s1bm1->MaxBit ;
	bm1 = v4l_BitMapAlloc(0,BM1_CalcMaxBit(mb)) ; bm1->MaxBit = BM1_CalcMaxBit(mb) ;		/* Allocate number of bits */
//	maxi = (mb+31) / 32 ;
	maxi = BM1_MapWord32(mb) ;
	for(i=0;i<maxi;i++) { bm1->Seg32[i] = ~s1bm1->Seg32[i] ; } ;
	return(bm1) ;
}

///*	v4l_BM1Test - Tests to see if bit is set in map			*/
///*	Call: result = v4l_BM1And( bit , s1bm1 )
//	  where result is TRUE/FALSE
//		bit is the bit to test for,
//		s1bm1 is source bitmap					*/
//
//struct V4L__ListBMData1 *v4l_BM1Test(bit,s1bm1)
//  int bit ;
//  struct V4L__ListBMData1 *s1bm1 ;
//{ 
//
//	bit-- ;
//	if (bit < 0 || bit >= s1bm1->MaxBit) return(FALSE) ;		/* Bit out of range */
//	return(BM1_IsBitSet(s1bm1,bit)) ;
////	return((s1bm1->Seg32[bit/32] >> (bit % 32)) & 1) ;
//}

/*	v4l_BitMapAnd - Logically and's two bitmap points		*/
/*	Call: v4l_BitMapAnd( ctx, intmodx , respt , bmpt1 , bmpt2 )
	  where ctx is current context,
		intmodx is intmod index from parent,
		respt is point to be updated,
		bmpt1 & bmpt2 are two bitmap points			*/

LOGICAL v4l_BitMapAnd(ctx,intmodx,respt,bmpt1,bmpt2)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  P *respt,*bmpt1,*bmpt2 ;
{ struct V4L__ListBMData1 *bm1,*bm2 ;
  struct V4L__ListBitMap *lbm1,*lbm2,*rlbm ;
  struct V4L__ListPoint *lp1,*lp2,*rlp ;
  struct V4IM__SetPoint *vsp ;
  int k ;

	lp1 = v4im_VerifyList(NULL,ctx,bmpt1,intmodx) ; lp2 = (lp1 != NULL ? v4im_VerifyList(NULL,ctx,bmpt2,intmodx) : NULL) ;
	if (lp1 == NULL || lp2 == NULL) return(FALSE) ;
	if (lp1->ListType == V4L_ListType_BitMap && lp2->ListType == V4L_ListType_BitMap)
	 { lbm1 = (struct V4L__ListBitMap *)&lp1->Buffer[0] ; lbm2 = (struct V4L__ListBitMap *)&lp2->Buffer[0] ;
	   COPYPTR(bm1,lbm1->bm1) ; COPYPTR(bm2,lbm2->bm1) ;
//VEH050503 - Have to set up respt as bit-map
	   ZPH(respt) ; respt->Dim = bmpt1->Dim ; respt->PntType = V4DPI_PntType_List ;
	   rlp = ALIGNLP(&respt->Value) ; rlbm = (struct V4L__ListBitMap *)&rlp->Buffer[0] ; rlbm->AllocMode = V4L_LBM_AllocTemp ;
	   rlp->ListType = V4L_ListType_BitMap ; rlp->Bytes = ((char *)rlbm - (char *)rlp) + sizeof *rlbm ;
	   respt->Bytes = V4DPI_PointHdr_Bytes + rlp->Bytes ;
	   COPYPTR1(rlbm->bm1,v4l_BM1And(bm1,bm2)) ;		/* And two results together */
//	   memcpy(respt,bmpt1,bmpt1->Bytes) ;		/* Set up result like first argument */
//	   rlp = ALIGNLP(&respt->Value) ; rlbm = (struct V4L__ListBitMap *)&rlp->Buffer[0] ;
////	   rlp = ALIGNLP(respt) ; rlbm = (struct V4L__ListBitMap *)&rlp->Buffer[0] ;
//	   COPYPTR1(rlbm->bm1,v4l_BM1And(bm1,bm2)) ;		/* And two results together */
	   return(TRUE) ;
	 } ;
/*	Don't have two bitmaps - have to do this the hard way */
	vsp = (struct V4IM__SetPoint *)v4mm_AllocChunk(sizeof *vsp,TRUE) ;
	if (!v4im_SetPointLoad(ctx,vsp,lp1,bmpt1->Dim)) return(FALSE) ;
	if (!v4im_SetPointLoad(ctx,vsp,lp2,bmpt2->Dim)) return(FALSE) ;
	for(k=0;k<V4IM_SetPointMapMax;k++) { vsp->List[0].Map[k] &= vsp->List[vsp->ListCount-1].Map[k] ; } ;
	memset(&vsp->List[vsp->ListCount].Map,0,sizeof vsp->List[vsp->ListCount].Map) ;
	if (v4im_SetPointMakeList(ctx,respt,vsp,0,TRUE) == NULL) return(FALSE) ;
	v4mm_FreeChunk(vsp) ;
	return(TRUE) ;
}

/*	v4l_BitMapOr - Logically or's two bitmap points		*/
/*	Call: v4l_BitMapOr( ctx, intmodx , respt , bmpt1 , bmpt2 )
	  where ctx is current context,
		intmodx is intmod index from parent,
		trace is trace flag,
		respt is point to be updated,
		bmpt1 & bmpt2 are two bitmap points			*/

LOGICAL v4l_BitMapOr(ctx,intmodx,respt,bmpt1,bmpt2)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  P *respt,*bmpt1,*bmpt2 ;
{ struct V4L__ListBMData1 *bm1,*bm2 ;
  struct V4L__ListBitMap *lbm1,*lbm2,*rlbm ;
  struct V4L__ListPoint *lp1,*lp2,*rlp ;
  struct V4IM__SetPoint *vsp ;
  int k ;

	lp1 = v4im_VerifyList(NULL,ctx,bmpt1,intmodx) ;  lp2 = (lp1 != NULL ? v4im_VerifyList(NULL,ctx,bmpt2,intmodx) : NULL) ;
	if (lp1 == NULL || lp2 == NULL) return(FALSE) ;
	if (lp1->ListType == V4L_ListType_BitMap && lp2->ListType == V4L_ListType_BitMap)
	 { lbm1 = (struct V4L__ListBitMap *)&lp1->Buffer[0] ; lbm2 = (struct V4L__ListBitMap *)&lp2->Buffer[0] ;
	   COPYPTR(bm1,lbm1->bm1) ; COPYPTR(bm2,lbm2->bm1) ;
//VEH050503 - Have to set up respt as bit-map
	   ZPH(respt) ; respt->Dim = bmpt1->Dim ; respt->PntType = V4DPI_PntType_List ;
	   rlp = ALIGNLP(&respt->Value) ; rlbm = (struct V4L__ListBitMap *)&rlp->Buffer[0] ; rlbm->AllocMode = V4L_LBM_AllocTemp ;
	   rlp->ListType = V4L_ListType_BitMap ; rlp->Bytes = ((char *)rlbm - (char *)rlp) + sizeof *rlbm ;
	   respt->Bytes = V4DPI_PointHdr_Bytes + rlp->Bytes ;
	   COPYPTR1(rlbm->bm1,v4l_BM1Or(bm1,bm2)) ;		/* And two results together */
//	   memcpy(respt,bmpt1,bmpt1->Bytes) ;		/* Set up result like first argument */
//	   rlp = ALIGNLP(&respt->Value) ; rlbm = (struct V4L__ListBitMap *)&rlp->Buffer[0] ;
//	   COPYPTR1(rlbm->bm1,v4l_BM1Or(bm1,bm2)) ;		/* And two results together */
	   return(TRUE) ;
	 } ;
/*	Don't have two bitmaps - have to do this the hard way */
	vsp = (struct V4IM__SetPoint *)v4mm_AllocChunk(sizeof *vsp,TRUE) ;
	if (!v4im_SetPointLoad(ctx,vsp,lp1,bmpt1->Dim)) return(FALSE) ;
	if (!v4im_SetPointLoad(ctx,vsp,lp2,bmpt2->Dim)) return(FALSE) ;
	for(k=0;k<V4IM_SetPointMapMax;k++) { vsp->List[0].Map[k] |= vsp->List[vsp->ListCount-1].Map[k] ; } ;
	memset(&vsp->List[vsp->ListCount].Map,0,sizeof vsp->List[vsp->ListCount].Map) ;
	if (v4im_SetPointMakeList(ctx,respt,vsp,0,TRUE) == NULL) return(FALSE) ;
	v4mm_FreeChunk(vsp) ;
	return(TRUE) ;
}

/*	v4l_BitMapMinus - Logically subtracts second bitmap from first	*/
/*	Call: v4l_BitMapMinus( ctx, intmodx , respt , bmpt1 , bmpt2 )
	  where ctx is current context,
		intmodx is intmod index from parent,
		respt is point to be updated,
		bmpt1 & bmpt2 are two bitmap points			*/

LOGICAL v4l_BitMapMinus(ctx,intmodx,respt,bmpt1,bmpt2)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  P *respt,*bmpt1,*bmpt2 ;
{ struct V4L__ListBMData1 *bm1,*bm2 ;
  struct V4L__ListBitMap *lbm1,*lbm2,*rlbm ;
  struct V4L__ListPoint *lp1,*lp2,*rlp ;
  struct V4IM__SetPoint *vsp ;
  int k ;

	lp1 = v4im_VerifyList(NULL,ctx,bmpt1,intmodx) ;  lp2 = (lp1 != NULL ? v4im_VerifyList(NULL,ctx,bmpt2,intmodx) : NULL) ;
	if (lp1 == NULL || lp2 == NULL) return(FALSE) ;
	if (lp1->ListType == V4L_ListType_BitMap && lp2->ListType == V4L_ListType_BitMap)
	 { lbm1 = (struct V4L__ListBitMap *)&lp1->Buffer[0] ; lbm2 = (struct V4L__ListBitMap *)&lp2->Buffer[0] ;
	   COPYPTR(bm1,lbm1->bm1) ; COPYPTR(bm2,lbm2->bm1) ;

//VEH050503 - Have to set up respt as bit-map
	   ZPH(respt) ; respt->Dim = bmpt1->Dim ; respt->PntType = V4DPI_PntType_List ;
	   rlp = ALIGNLP(&respt->Value) ; rlbm = (struct V4L__ListBitMap *)&rlp->Buffer[0] ; rlbm->AllocMode = V4L_LBM_AllocTemp ;
	   rlp->ListType = V4L_ListType_BitMap ; rlp->Bytes = ((char *)rlbm - (char *)rlp) + sizeof *rlbm ;
	   respt->Bytes = V4DPI_PointHdr_Bytes + rlp->Bytes ;
	   COPYPTR1(rlbm->bm1,v4l_BM1Minus(bm1,bm2)) ;		/* Subtract second from first */
	   
	   
//	   memcpy(respt,bmpt1,bmpt1->Bytes) ;		/* Set up result like first argument */
//	   rlp = ALIGNLP(&respt->Value) ; rlbm = (struct V4L__ListBitMap *)&rlp->Buffer[0] ;
//	   COPYPTR1(rlbm->bm1,v4l_BM1Minus(bm1,bm2)) ;		/* Subtract second from first */

	   return(TRUE) ;
	 } ;
/*	Don't have two bitmaps - have to do this the hard way */
	vsp = (struct V4IM__SetPoint *)v4mm_AllocChunk(sizeof *vsp,TRUE) ;
	if (!v4im_SetPointLoad(ctx,vsp,lp1,bmpt1->Dim)) return(FALSE) ;
	if (!v4im_SetPointLoad(ctx,vsp,lp2,bmpt2->Dim)) return(FALSE) ;
	for(k=0;k<V4IM_SetPointMapMax;k++) { vsp->List[0].Map[k] &= ~vsp->List[vsp->ListCount-1].Map[k] ; } ;
	memset(&vsp->List[vsp->ListCount].Map,0,sizeof vsp->List[vsp->ListCount].Map) ;
	if (v4im_SetPointMakeList(ctx,respt,vsp,0,TRUE) == NULL) return(FALSE) ;
	v4mm_FreeChunk(vsp) ;
	return(TRUE) ;
}

/*	v4l_BitMapNot - Logically conmpliments bitmap point		*/
/*	Call: v4l_BitMapNot( ctx, intmodx , respt , bmpt1 )
	  where ctx is current context,
		intmodx is intmod index from parent,
		respt is point to be updated,
		bmpt1 is bitmap point					*/

LOGICAL v4l_BitMapNot(ctx,intmodx,respt,bmpt1)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  P *respt,*bmpt1 ;
{ struct V4L__ListBMData1 *bm1 ;
  struct V4L__ListBitMap *lbm1,*rlbm ;
  struct V4L__ListPoint *lp1,*rlp ;

	lp1 = v4im_VerifyList(NULL,ctx,bmpt1,intmodx) ;
	if (lp1 == NULL) return(FALSE) ;
	if (lp1->ListType == V4L_ListType_BitMap)
	 { lbm1 = (struct V4L__ListBitMap *)&lp1->Buffer[0] ;
	   COPYPTR(bm1,lbm1->bm1) ;
//VEH050503 - Have to set up respt as bit-map
	   ZPH(respt) ; respt->Dim = bmpt1->Dim ; respt->PntType = V4DPI_PntType_List ;
	   rlp = ALIGNLP(&respt->Value) ; rlbm = (struct V4L__ListBitMap *)&rlp->Buffer[0] ; rlbm->AllocMode = V4L_LBM_AllocTemp ;
	   rlp->ListType = V4L_ListType_BitMap ; rlp->Bytes = ((char *)rlbm - (char *)rlp) + sizeof *rlbm ;
	   respt->Bytes = V4DPI_PointHdr_Bytes + rlp->Bytes ;
	   COPYPTR1(rlbm->bm1,v4l_BM1Not(bm1)) ;		/* And two results together */
//	   memcpy(respt,bmpt1,bmpt1->Bytes) ;		/* Set up result like first argument */
//	   rlp = ALIGNLP(&respt->Value) ; rlbm = (struct V4L__ListBitMap *)&rlp->Buffer[0] ;
//	   COPYPTR1(rlbm->bm1,v4l_BM1Not(bm1)) ;		/* And two results together */
	   return(TRUE) ;
	 } ;
	v_Msg(ctx,ctx->ErrorMsgAux,"@Bitmap negation not supported on this point(%1P)",bmpt1) ;
	return(FALSE) ;
}

/*	v4l_BitMapCount - Counts number of bits in bitmap		*/
/*	Call: count = v4l_BitMapCount( ctx, bmpt1 , lp )
	  where count is number of bits (UNUSED if error),
		ctx is current context,
		bmpt1 is bitmap point,
		or lp is list pointer to bitmap (if not NULL)		*/

int v4l_BitMapCount(ctx,bmpt1,lp)
  struct V4C__Context *ctx ;
  struct V4L__ListPoint *lp ;
  P *bmpt1 ;
{ struct V4L__ListBMData1 *bm1 ;
  struct V4L__ListBitMap *lbm1 ;
  struct V4L__ListPoint *lp1 ;
  int k,count ;

	lp1 = (lp != NULL ? lp : v4im_VerifyList(NULL,ctx,bmpt1,0)) ;
	if (lp1 == NULL) return(UNUSED) ;
	if (lp1->ListType != V4L_ListType_BitMap)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"@ListType (%1d) not bitmap",lp1->ListType) ; return(UNUSED) ; } ;
	lbm1 = (struct V4L__ListBitMap *)&lp1->Buffer[0] ;
	COPYPTR(bm1,lbm1->bm1) ;
	BM1_NumBitsSet(bm1,count) ;
	return(count);
}

/*	v4l_BitMapTest - Tests to see if bit set in map			*/
/*	Call: logical = v4l_BitMapOr( ctx, intmodx , bit , bmpt1 )
	  where logical is TRUE if bit set, FALSE if not set
		ctx is current context,
		intmodx is intmod index from parent,
		bit is bit to be tested
		bmpt1 is bitmap point					*/

LOGICAL v4l_BitMapTest(ctx,intmodx,bit,bmpt1)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  INDEX bit ;
  P *bmpt1 ;
{ struct V4L__ListBMData1 *bm1 ;
  struct V4L__ListBitMap *lbm1 ;
  struct V4L__ListPoint *lp1 ;

	lp1 = v4im_VerifyList(NULL,ctx,bmpt1,intmodx) ;
	if (lp1 == NULL) return(FALSE) ;
	if (lp1->ListType == V4L_ListType_BitMap)
	 { lbm1 = (struct V4L__ListBitMap *)&lp1->Buffer[0] ;
	   COPYPTR(bm1,lbm1->bm1) ;
	   return(BM1_IsBitSet(bm1,bit)) ;
//	   return(v4l_BM1Test(bit,bm1)) ;
	 } else
	 { return(FALSE) ;
	 } ;
}

/*	v4l_Getvbe - Gets a list block entry				*/
/*	Call: vbe = v4l_Getvbe( ctx, relhnum , blocknum )
	  where vbe is pointer to V4L__BlockEntry we want,
	  	ctx is current context,
	  	relhnum is RelHNum under consideration,
	  	blocknum is block number we want (0 to create new)	*/

#define LVM 5
struct lcl__vbe				/* Temp cache of vbe's */
{ int Count ;
  int LastMRU ;
  struct {
    struct V4L__BlockEntry *vbe ;
    int relhnum,blocknum,dirty ;
    int MRU,newrec ;
   } Entry[LVM] ;
} *lvbe = NULL ;

void v4l_Purgevbe(ctx)
  struct V4C__Context *ctx ;
{ int i ;

	if (lvbe == NULL) return ;		/* Nothing to purge out */
	for(i=0;i<LVM;i++)
	 { if (lvbe->Entry[i].dirty)
	    v4l_Putvbe(ctx,lvbe->Entry[i].relhnum,lvbe->Entry[i].blocknum,TRUE) ;	/* Write out old block before re-using */
	 } ;
}

struct V4L__BlockEntry *v4l_Getvbe(ctx,relhnum,blocknum)
  struct V4C__Context *ctx ;
  int relhnum,blocknum ;
{ 
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4L__BlockEntry *vbe,*vbeptr ;
  INDEX i,lrx ; INDEX low,lowx ; AREAID aid ; INDEX areax ;

	if (lvbe == NULL) lvbe = (struct lcl__vbe *)v4mm_AllocChunk(sizeof *lvbe,TRUE) ;
	for(i=0;i<LVM;i++)
	 { if (lvbe->Entry[i].relhnum == relhnum && lvbe->Entry[i].blocknum == blocknum)
	    { lvbe->Entry[i].MRU = (++lvbe->LastMRU) ; return(lvbe->Entry[i].vbe) ; } ;
	 } ;
	if (relhnum == 0) { aid = gpi->RelH[gpi->HighHNum].aid ; }
	 else { aid = gpi->RelH[relhnum].aid ; } ;
	FINDAREAINPROCESS(areax,aid)
	for(lowx=0,i=0,low=V4LIM_BiggestPositiveInt;i<LVM;i++)
	 { if (lvbe->Entry[i].MRU < low) { lowx = i ; low = lvbe->Entry[i].MRU ; } ; } ;
	if (lvbe->Entry[lowx].dirty)
	 v4l_Putvbe(ctx,lvbe->Entry[lowx].relhnum,lvbe->Entry[lowx].blocknum,TRUE) ;	/* Write out old block before re-using */
	if (lvbe->Entry[lowx].vbe == NULL) lvbe->Entry[lowx].vbe = (struct V4L__BlockEntry *)v4mm_AllocChunk(sizeof *vbe,FALSE) ;
	vbe = lvbe->Entry[lowx].vbe ;
	lvbe->Entry[lowx].relhnum = relhnum ; lvbe->Entry[lowx].blocknum = blocknum ; lvbe->Entry[lowx].dirty = FALSE ;
	lvbe->Entry[lowx].MRU = (++lvbe->LastMRU) ;
	vbe->kp.fld.AuxVal = V4IS_SubType_Value ; vbe->kp.fld.KeyType = V4IS_KeyType_V4 ;
	vbe->kp.fld.KeyMode = V4IS_KeyMode_Int ; vbe->kp.fld.Bytes = V4IS_IntKey_Bytes ;
	if (blocknum == 0)			/* Creating a new block? */
	 { 
	   vbe->EntryCount = 0 ; acb = v4mm_ACBPtr(aid) ;
#ifdef TRACEGRAB
printf("getvbe %d\n",mmm->Areas[areax].areaLock) ;
#endif
	   LOCKROOT ; if (!GRABAREAMTLOCK(areax)) return(NULL) ;
	   vbe->ValueNum = HNUMDICTMASK & (acb->RootInfo->NextAvailValueNum++) ;	/* Assign value number */
	   RELROOT ; FREEAREAMTLOCK(areax) ;
	   lvbe->Entry[lowx].blocknum = vbe->ValueNum ; lvbe->Entry[lowx].newrec = TRUE ;
	 } else
	 { vbe->ValueNum = blocknum ;
#ifdef TRACEGRAB
printf("getvbe2 %d %d\n",areax,mmm->Areas[areax].areaLock) ;
#endif
	   if (!GRABAREAMTLOCK(areax)) return(NULL) ;
	   if (v4is_PositionKey(aid,(struct V4IS__Key *)&vbe->kp,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) == V4RES_PosKey)	/* See if anything exists */
	    { vbeptr = (struct V4L__BlockEntry *)v4dpi_GetBindBuf(ctx,aid,ikde,FALSE,NULL) ;
	      memcpy(vbe,vbeptr,sizeof *vbe) ;		/* Got something - copy it in */
	      lvbe->Entry[lowx].newrec = FALSE ;
	      FREEAREAMTLOCK(areax) ;
	    } else { FREEAREAMTLOCK(areax) ; return(NULL) ; } ;
	 } ;
	return(vbe) ;
}

LOGICAL v4l_Putvbe(ctx,relhnum,blocknum,forceflag)
  struct V4C__Context *ctx ;
  int relhnum,blocknum,forceflag ;
{ 
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__AreaCB *acb ;
  struct V4L__BlockEntry *vbe ;
  int i ; AREAID aid ; INDEX areax ;

	if (relhnum == 0) { aid = gpi->RelH[gpi->HighHNum].aid ; }
	 else { aid = gpi->RelH[relhnum].aid ; } ;
	FINDAREAINPROCESS(areax,aid)
	for(i=0;i<LVM;i++)
	 { if (lvbe->Entry[i].relhnum == relhnum && lvbe->Entry[i].blocknum == blocknum) break ; } ;
	if (i >= LVM) return(FALSE) ;					/* Could not find entry? */
	if (!forceflag) { lvbe->Entry[i].dirty = TRUE ; return(FALSE) ; } ;	/* Just mark as "dirty" */
	vbe = lvbe->Entry[i].vbe ;
	acb = v4mm_ACBPtr(aid) ;
//	mmm = V4_GLOBAL_MMM_PTR ;
#ifdef TRACEGRAB
printf("putvbe %d\n",mmm->Areas[areax].areaLock) ;
#endif
	if (!GRABAREAMTLOCK(areax)) return(FALSE) ;
	if (lvbe->Entry[i].newrec)
	 { v4is_Insert(aid,(struct V4IS__Key *)vbe,vbe,sizeof *vbe,V4IS_PCB_DataMode_Auto,V4IS_DataCmp_None,0,FALSE,FALSE,0,NULL) ;
	 } else
	 { v4is_PositionKey(aid,(struct V4IS__Key *)vbe,NULL,0,V4IS_PosDCKL) ;	/* Reposition to get possibly changed dataid */
	   v4is_Replace(aid,(struct V4IS__Key *)vbe,vbe,vbe,sizeof *vbe,V4IS_PCB_DataMode_Auto,V4IS_DataCmp_None,0,0) ;
	 } ;
	FREEAREAMTLOCK(areax) ;
	lvbe->Entry[i].newrec = FALSE ; lvbe->Entry[i].dirty = FALSE ;
	return(TRUE) ;
}

/*	v4l_Getvbeg - Gets a list block entry for generic block		*/
/*	NOTE: This code is very similar to above but for generic point lists */
/*	Call: vbe = v4l_Getvbe( ctx, relhnum , blocknum )
	  where vbe is pointer to V4L__BlockEntry we want (NULL if error in ctx->ErrorMsgAux),
	  	ctx is current context,
	  	relhnum is RelHNum under consideration,
	  	blocknum is block number we want (0 to create new)	*/

#define LVM 5
struct lcl__vbeg				/* Temp cache of vbe's */
{ int Count ;
  int LastMRU ;
  struct {
    struct V4L__BlockEntry *vbeg ;
    int relhnum,blocknum,dirty ;
    int MRU,newrec ;
   } Entry[LVM] ;
} *lvbeg = NULL ;

void v4l_Purgevbeg(ctx)
  struct V4C__Context *ctx ;
{ int i ;

	if (lvbeg == NULL) return ;		/* Nothing to purge out */
	for(i=0;i<LVM;i++)
	 { if (lvbeg->Entry[i].dirty)
	    v4l_Putvbeg(ctx,lvbeg->Entry[i].relhnum,lvbeg->Entry[i].blocknum,TRUE) ;	/* Write out old block before re-using */
	 } ;
}

struct V4L__BlockEntryGeneric *v4l_Getvbeg(ctx,relhnum,blocknum)
  struct V4C__Context *ctx ;
  int relhnum,blocknum ;
{ 
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__AreaCB *acb ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4L__BlockEntryGeneric *vbeg,*vbegptr ;
  INDEX i,lrx,areax,low,lowx ; AREAID aid ;

	if (lvbeg == NULL) lvbeg = (struct lcl__vbeg *)v4mm_AllocChunk(sizeof *lvbeg,TRUE) ;
	for(i=0;i<LVM;i++)
	 { if (lvbeg->Entry[i].relhnum == relhnum && lvbeg->Entry[i].blocknum == blocknum)
	    { lvbeg->Entry[i].MRU = (++lvbeg->LastMRU) ; return((struct V4L__BlockEntryGeneric *)lvbeg->Entry[i].vbeg) ; } ;
	 } ;
	if (relhnum == 0) { aid = gpi->RelH[gpi->HighHNum].aid ; }
	 else { aid = gpi->RelH[relhnum].aid ; } ;
	FINDAREAINPROCESS(areax,aid)
	for(lowx=0,i=0,low=V4LIM_BiggestPositiveInt;i<LVM;i++)
	 { if (lvbeg->Entry[i].MRU < low) { lowx = i ; low = lvbeg->Entry[i].MRU ; } ; } ;
	if (lvbeg->Entry[lowx].dirty)
	 v4l_Putvbeg(ctx,lvbeg->Entry[lowx].relhnum,lvbeg->Entry[lowx].blocknum,TRUE) ;	/* Write out old block before re-using */
	if (lvbeg->Entry[lowx].vbeg == NULL) lvbeg->Entry[lowx].vbeg = (struct V4L__BlockEntry *)v4mm_AllocChunk(sizeof *vbeg,FALSE) ;
	vbeg = (struct V4L__BlockEntryGeneric *)lvbeg->Entry[lowx].vbeg ;
	lvbeg->Entry[lowx].relhnum = relhnum ; lvbeg->Entry[lowx].blocknum = blocknum ; lvbeg->Entry[lowx].dirty = FALSE ;
	lvbeg->Entry[lowx].MRU = (++lvbeg->LastMRU) ;
	vbeg->kp.fld.AuxVal = V4IS_SubType_Value ; vbeg->kp.fld.KeyType = V4IS_KeyType_V4 ;
	vbeg->kp.fld.KeyMode = V4IS_KeyMode_Int ; vbeg->kp.fld.Bytes = V4IS_IntKey_Bytes ;
	if (blocknum == 0)			/* Creating a new block? */
	 { 
	   vbeg->EntryCount = 0 ; acb = v4mm_ACBPtr(aid) ;
#ifdef TRACEGRAB
printf("getvbg %d\n",mmm->Areas[areax].areaLock) ;
#endif
	   LOCKROOT ; GRABAREAMTLOCK(areax) ;
	   vbeg->ValueNum = HNUMDICTMASK & (acb->RootInfo->NextAvailValueNum++) ;	/* Assign value number */
	   RELROOT ; FREEAREAMTLOCK(areax) ;
	   lvbeg->Entry[lowx].blocknum = vbeg->ValueNum ; lvbeg->Entry[lowx].newrec = TRUE ;
	   return(vbeg) ;
	 } ;
/*	Loook for the entry in aid */
	vbeg->ValueNum = blocknum ;
#ifdef TRACEGRAB
printf("getvbg2 %d\n",mmm->Areas[areax].areaLock) ;
#endif
	if (!GRABAREAMTLOCK(areax)) return(NULL) ;
	if (v4is_PositionKey(aid,(struct V4IS__Key *)&vbeg->kp,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey)	/* See if anything exists */
	 { 
	   FREEAREAMTLOCK(areax) ;
	   for(areax=0;areax<V4MM_Area_Max;areax++)			/* Search thru all areas with binds on this dimension */
	    { if (mmm->Areas[areax].abi == NULL) continue ;
	      aid = mmm->Areas[areax].AreaId ;
	      if (!GRABAREAMTLOCK(areax)) continue ;
	      if (v4is_PositionKey(aid,(struct V4IS__Key *)&vbeg->kp,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) == V4RES_PosKey) break ;	/* See if anything exists */
	      FREEAREAMTLOCK(areax) ;
	    } ;
	   if (areax >=V4MM_Area_Max)
	    { 
//	      sprintf(ASCTBUF1,"*Could not find vbeg block #%d\n",blocknum) ; vout_Text(VOUT_Trace,0,ASCTBUF1) ;
	      v_Msg(ctx,ctx->ErrorMsgAux,"ListVBEG",blocknum,areax) ; return(NULL) ;
	    } ;
	 } ;
	vbegptr = (struct V4L__BlockEntryGeneric *)v4dpi_GetBindBuf(ctx,aid,ikde,FALSE,NULL) ;
	memcpy(vbeg,vbegptr,sizeof *vbeg) ;		/* Got something - copy it in */
	FREEAREAMTLOCK(areax) ;
	lvbeg->Entry[lowx].newrec = FALSE ;
	return(vbeg) ;
}

void v4l_Putvbeg(ctx,relhnum,blocknum,forceflag)
  struct V4C__Context *ctx ;
  int relhnum,blocknum,forceflag ;
{ 
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__AreaCB *acb ;
  struct V4L__BlockEntryGeneric *vbeg ;
  int i ; AREAID aid ; INDEX areax ;

	if (relhnum == 0) { aid = gpi->RelH[gpi->HighHNum].aid ; }
	 else { aid = gpi->RelH[relhnum].aid ; } ;
	FINDAREAINPROCESS(areax,aid)
	for(i=0;i<LVM;i++)
	 { if (lvbeg->Entry[i].relhnum == relhnum && lvbeg->Entry[i].blocknum == blocknum) break ; } ;
	if (i >= LVM) return ;					/* Could not find entry? */
	if (!forceflag) { lvbeg->Entry[i].dirty = TRUE ; return ; } ;	/* Just mark as "dirty" */
	vbeg = (struct V4L__BlockEntryGeneric *)lvbeg->Entry[i].vbeg ;
	acb = v4mm_ACBPtr(aid) ;
//	mmm = V4_GLOBAL_MMM_PTR ;
#ifdef TRACEGRAB
printf("putvbeg %d\n",mmm->Areas[areax].areaLock) ;
#endif
	GRABAREAMTLOCK(areax) ;
	if (lvbeg->Entry[i].newrec)
	 { v4is_Insert(aid,(struct V4IS__Key *)vbeg,vbeg,sizeof *vbeg,V4IS_PCB_DataMode_Auto,V4IS_DataCmp_None,0,FALSE,FALSE,0,NULL) ;
	 } else
	 { v4is_PositionKey(aid,(struct V4IS__Key *)vbeg,NULL,0,V4IS_PosDCKL) ;	/* Reposition to get possibly changed dataid */
	   v4is_Replace(aid,(struct V4IS__Key *)vbeg,vbeg,vbeg,sizeof *vbeg,V4IS_PCB_DataMode_Auto,V4IS_DataCmp_None,0,0) ;
	 } ;
	FREEAREAMTLOCK(areax) ;
	lvbeg->Entry[i].newrec = FALSE ; lvbeg->Entry[i].dirty = FALSE ;
}

/*	L I S T  /  B I T M A P   M O D U L E S			*/

#define V4Seg_MaxSegBytes V4Area_UsableBytes		/* Max bytes in a V4 segment */
#define V4Seg_MaxSegments 1000				/* Max number of segments in a data record */
#define V4Seg_CacheEntryStart 1009			/* Starting Segment Cache Size */

struct V4Seg__Directory	{		/* Segment directory record format */
  union V4IS__KeyPrefix kp ;		/* Bytes = V4IS_IntKey_Bytes / KeyMode = V4IS_KeyMode_Int / KeyType = V4IS_KeyType_V4Segments / AuxVal = 0 */
  V4MSEGID PointVal ;
  int TotalBytes ;			/* Total bytes in data */
  int SegCount ;			/* Number of segments below */
  struct {
   int Key ;				/* Key to this segment (same format as dkey above, new value) */
   int Bytes ;				/* Number of bytes in this segment */
  } Segment[V4Seg_MaxSegments] ;
} ;

struct V4Seg__LocalCache {		/* Format of in-core segment cache */
  int Count ;				/* Number of active entries below */
  int Max ;				/* Max number of entries */
  struct {
   int Bytes ;				/* Length of data */
   int Key ;				/* Key value to the starting segment */
   char *Ptr ;				/* Pointer to the completed (fully formed) data */
  } Entry[V4Seg_CacheEntryStart] ;	/* Initial size (will grow as needed) */
} ;

static struct V4Seg__LocalCache *vslc=NULL ;

struct V4Seg__OneSegment {		/* Format of a single segment */
  union V4IS__KeyPrefix kp ;
  V4MSEGID PointVal ;
  char Buf[V4Seg_MaxSegBytes] ;
} ;


/*	v4seg_PutSegments - Writes out large data into one or more segments	*/
/*	Call: segId = v4seg_PutSegments( ctx , dataptr , databytes , cachectl , copyctl )
	  where segId is returned segment number (UNUSED if fail, ctx->ErrorMsgAux has error),
		ctx is current context,
		dataptr/databytes point to the data,
		cachectl is TRUE to cache data (in addition to writing out),
		copyctl is TRUE to make copy of data for cache, FALSE to use dataptr without copying (this option ignored if cachectl is FALSE)		*/

V4MSEGID v4seg_PutSegments(ctx,dataptr,databytes,cachectl,copyctl)
  struct V4C__Context *ctx ;
  void *dataptr ; int databytes ;
  LOGICAL cachectl, copyctl ;
{ struct V4IS__AreaCB *acb ;
  struct V4Seg__Directory sdir ;
  struct V4Seg__OneSegment vsos ;
  V4MSEGID segId ;
  int bytes,offset,remain,rx,aid,hash,hx ;

#define SEGIDINDEXMASK 0x0FFFFFFF
#define SEGIDwHNUM(hnum,SEGID) ((hnum<<28)|(SEGID&SEGIDINDEXMASK))

/*	Determine aid to write to */
	for(rx=gpi->HighHNum;rx>=gpi->LowHNum;rx--)			/* Find highest area allowing new entries */
	 { if (rx == V4DPI_WorkRelHNum) continue ;			/* Don't default to work area! */
	   if (gpi->RelH[rx].aid == UNUSED) continue ; if (gpi->RelH[rx].ahi.ExtDictUpd) break ;
	 } ;
	if (rx < gpi->LowHNum && gpi->RelH[V4DPI_WorkRelHNum].aid != UNUSED) /* If all else fails - assign to process work */
	 rx = V4DPI_WorkRelHNum ;
	if (rx < gpi->LowHNum) { v_Msg(ctx,ctx->ErrorMsgAux,"CtxNoUpdArea1") ; return(UNUSED) ; } ;
	aid = gpi->RelH[rx].aid ; acb = v4mm_ACBPtr(aid) ;	/* Link up to control block for area */


	if (databytes <= V4Seg_MaxSegBytes)		/* Can we write out data as single segment? */
	 { vsos.kp.fld.Bytes = V4IS_IntKey_Bytes ; vsos.kp.fld.KeyMode = V4IS_KeyMode_Int ; vsos.kp.fld.KeyType = V4IS_KeyType_V4Segments ;
	   vsos.kp.fld.AuxVal = 0 ;
	   acb->RootInfo->lastSegNum++ ;	/* Get new point value */
	   vsos.PointVal = SEGIDwHNUM(rx,acb->RootInfo->lastSegNum) ;	/* Get new point value */
	   memcpy(vsos.Buf,dataptr,databytes) ;
//xxxxxxxxxxxxxxxxx Needs lock
	   if (v4is_Insert(aid,(struct V4IS__Key *)&vsos,&vsos,V4IS_IntKey_Bytes+databytes,V4IS_PCB_DataMode_Data,0,0,FALSE,FALSE,0,ctx->ErrorMsg) == UNUSED)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"SegPutErr",ctx->ErrorMsg,vsos.PointVal) ; return(UNUSED) ; } ;
	   segId = vsos.PointVal ; goto maybe_cache ;
	 } ;
/*	If here then have to break big data up into segments */
	sdir.kp.fld.Bytes = V4IS_IntKey_Bytes ; sdir.kp.fld.KeyMode = V4IS_KeyMode_Int ; sdir.kp.fld.KeyType = V4IS_KeyType_V4Segments ;
	sdir.kp.fld.AuxVal = 0 ;
	acb->RootInfo->lastSegNum++ ;
	sdir.PointVal = -SEGIDwHNUM(rx,acb->RootInfo->lastSegNum) ;	/* Get new point value (negative = directory) */
	sdir.TotalBytes = databytes ; sdir.SegCount = 0 ;
	vsos.kp.fld.Bytes = V4IS_IntKey_Bytes ; vsos.kp.fld.KeyMode = V4IS_KeyMode_Int ;
	vsos.kp.fld.KeyType = V4IS_KeyType_V4Segments ; vsos.kp.fld.AuxVal = 0 ;
/*	Write out each segment */
	for(remain=databytes,offset=0;remain>0;remain-=V4Seg_MaxSegBytes,offset+=V4Seg_MaxSegBytes)
	 { 
	   acb->RootInfo->lastSegNum++ ;
	   vsos.PointVal = SEGIDwHNUM(rx,acb->RootInfo->lastSegNum) ;	/* Get new point value for this segment */
	   bytes = (remain > V4Seg_MaxSegBytes ? V4Seg_MaxSegBytes : remain) ;
	   memcpy(vsos.Buf,(char *)dataptr+offset,bytes) ;
	   if (v4is_Insert(aid,(struct V4IS__Key *)&vsos,&vsos,V4IS_IntKey_Bytes+bytes,V4IS_PCB_DataMode_Data,0,0,FALSE,FALSE,0,ctx->ErrorMsg) == UNUSED)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"SegPutErr",ctx->ErrorMsg,vsos.PointVal) ; return(UNUSED) ; } ;
	   sdir.Segment[sdir.SegCount].Key = vsos.PointVal ; sdir.Segment[sdir.SegCount].Bytes = bytes ;
	   sdir.SegCount ++ ;
	 } ;
/*	And finally the segment header */
	bytes = (char *)&sdir.Segment[sdir.SegCount].Key - (char *)&sdir ;
	if (v4is_Insert(aid,(struct V4IS__Key *)&sdir,&sdir,bytes,V4IS_PCB_DataMode_Data,0,0,FALSE,FALSE,0,ctx->ErrorMsg) == UNUSED)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"SegPutErr",ctx->ErrorMsg,sdir.PointVal) ; return(UNUSED) ; } ;
	segId = sdir.PointVal ; goto maybe_cache ;


maybe_cache:
	if (!cachectl) return(segId) ;
	if (vslc == NULL)
	 { vslc = (struct V4Seg__LocalCache *)v4mm_AllocChunk(sizeof *vslc,TRUE) ; vslc->Max = V4Seg_CacheEntryStart ; } ;
	if (vslc->Count >= (vslc->Max * 0.9))	/* Are we filling up the cache ? */
	 { int i,max ;
	   struct V4Seg__LocalCache *vslc1 ;
	   max = v_CvtToPrime((int)(vslc->Max * 1.5)) ;
	   vslc1 = v4mm_AllocChunk((char *)&vslc->Entry[max] - (char *)vslc,TRUE) ;
	   vslc1->Max = max ; vslc1->Count = vslc->Count ;
	   for(i=0;i<vslc->Max;i++)		/* Re-hash into new space */
	    { if (vslc->Entry[i].Key == 0) continue ;
	      hash = vslc->Entry[i].Key & 0x7fffffff ;
	      for(;;hash++) { hx = (hash % vslc1->Max) ; if (vslc1->Entry[hx].Key == 0) break ; } ;
	      vslc1->Entry[hx] = vslc->Entry[i] ;
	    } ;
	   v4mm_FreeChunk(vslc) ; vslc = vslc1 ;
	 } ;
	hash = (segId & 0x7fffffff) ;
	for(;;hash++) { hx = (hash % vslc->Max) ; if (vslc->Entry[hx].Key == 0) break ; } ;
	vslc->Entry[hx].Bytes = databytes ; vslc->Count ++ ;
	vslc->Entry[hx].Key = segId ;
	if (copyctl)
	 { vslc->Entry[hx].Ptr = (char *)v4mm_AllocChunk(databytes,FALSE) ; memcpy(vslc->Entry[hx].Ptr,dataptr,databytes) ;
	 } else
	 { vslc->Entry[hx].Ptr = dataptr ;
	 } ;
	return(segId) ;

}

/*	v4seg_GetSegments - Retrieves large data from one or more segments	*/
/*	Call: data = v4seg_GetSegments( ctx , segId , &databytes , cachectl , segAId )
	  where data is pointer to large data segment (NULL if failed, ctx->ErrorMsgAux has message),
		ctx is current context,
		segId is value for data (neg -> record is struct V4Seg__Directory, pos -> record is data),
		databytes, if not NULL, is pointer to *int that is updated with bytes in data,
		cachectl is cache control - TRUE to store data in cache, FALSE to return ptr to temp buffer,
		segAId, if not UNUSED, if Area Id to retrieve from		*/

void *v4seg_GetSegments(ctx,segId,databytes,cachectl,segAId)
  struct V4C__Context *ctx ;
  V4MSEGID segId ;
  int *databytes ;
  int cachectl ;
  AREAID segAId ;
{
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4Seg__Directory sdir ;
  static char *lclbuf ; static int lclbytes=0 ; static int lastaid=UNUSED ;
  struct V4Seg__OneSegment vsos, *vsosp ; char *data ;
  int bytes,ok,rx,hash,hx,aid,i ; char *ptr ;

/*	If caching then see if entry is in local cache */
	if (cachectl)
	 { if (vslc == NULL)
	    { vslc = (struct V4Seg__LocalCache *)v4mm_AllocChunk(sizeof *vslc,TRUE) ; vslc->Max = V4Seg_CacheEntryStart ; } ;
	   hash = segId & 0x7fffffff ;
	   for(;;hash++)
	    { hx = (hash % vslc->Max) ;
	      if (vslc->Entry[hx].Key == segId)
	       { if (databytes != NULL) *databytes = vslc->Entry[hx].Bytes ;
	         return(vslc->Entry[hx].Ptr) ;
	       } ;
	      if (vslc->Entry[hx].Key == 0) break ;	/* Empty slot - not gonna find it in cache */
	    } ;
	 } ;


/*	First look for the initial segment, if we have tried before then start with lastaid */
	vsos.kp.fld.Bytes = V4IS_IntKey_Bytes ; vsos.kp.fld.KeyMode = V4IS_KeyMode_Int ; vsos.kp.fld.KeyType = V4IS_KeyType_V4Segments ;
	vsos.kp.fld.AuxVal = 0 ; vsos.PointVal = segId ;
	if (segAId != UNUSED) lastaid = segAId ;
	ok = FALSE ;
	if (lastaid != UNUSED)
	 { 
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//	   GRABMTLOCK(gpi->Areas[lastaid].areaLock) ;
	   ok = (v4is_PositionKey(lastaid,(struct V4IS__Key *)&vsos,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) == V4RES_PosKey) ;
//	   FREEMTLOCK(gpi->Areas[lastaid].areaLock) ;
	 } ;
	if (lastaid == UNUSED || (!ok))
	 { for(rx=gpi->HighHNum;rx>=gpi->LowHNum;rx--)				/* Scan looking for first updatable area with wanted info */
	    { if (gpi->RelH[rx].aid == UNUSED) continue ;
	      aid = gpi->RelH[rx].aid ;
//	      if (!GRABMTLOCK(gpi->Areas[aid].areaLock)) continue ;
	      if (v4is_PositionKey(aid,(struct V4IS__Key *)&vsos,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) == V4RES_PosKey) break ;
//	      FREEMTLOCK(gpi->Areas[aid].areaLock) ;
	    } ;
	   if (rx < gpi->LowHNum) { v_Msg(ctx,ctx->ErrorMsgAux,"SegNotFound",segId) ; return(NULL) ; } ;
//	   FREEMTLOCK(gpi->Areas[aid].areaLock) ;
	   lastaid = aid ;
	 } ;
//	GRABMTLOCK(gpi->Areas[lastaid].areaLock) ;
	vsosp = (struct V4Seg__OneSegment *)v4dpi_GetBindBuf(ctx,lastaid,ikde,FALSE,&bytes) ;
//	FREEMTLOCK(gpi->Areas[lastaid].areaLock) ;

/*	If key > 0 then data contains a single segment record */
	if (segId > 0)
	 { bytes -= V4IS_IntKey_Bytes ; data = vsosp->Buf ;
	   if (lclbytes == 0) { lclbuf = (char *)v4mm_AllocChunk(lclbytes = bytes,FALSE) ; }
	    else if (bytes > lclbytes) { v4mm_FreeChunk(lclbuf) ; lclbuf = (char *)v4mm_AllocChunk(lclbytes = bytes,FALSE) ; } ;
	   memcpy(lclbuf,data,bytes) ;
	   goto try_cache ;
	 } ;

/*	Have to pull segment directory & construct data from multiple segments */
/*	data points to the directory */
	if (bytes > sizeof sdir) { v_Msg(ctx,ctx->ErrorMsgAux,"SegDirBad",segId,bytes,sizeof sdir) ; return(NULL) ; } ;
	memcpy(&sdir,vsosp,bytes) ;
	if (lclbytes == 0) { lclbuf = (char *)v4mm_AllocChunk(lclbytes = sdir.TotalBytes,FALSE) ; }
	 else if (sdir.TotalBytes > lclbytes) { v4mm_FreeChunk(lclbuf) ; lclbuf = (char *)v4mm_AllocChunk(lclbytes = sdir.TotalBytes,FALSE) ; } ;
	for(i=0,ptr=lclbuf;i<sdir.SegCount;i++)
	 { vsos.PointVal = sdir.Segment[i].Key ;
//	   if (!GRABMTLOCK(gpi->Areas[lastaid].areaLock)) return(NULL) ;
	   if (!(v4is_PositionKey(lastaid,(struct V4IS__Key *)&vsos,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) == V4RES_PosKey))
	    { 
//	      FREEMTLOCK(gpi->Areas[lastaid].areaLock) ;
	      v_Msg(ctx,ctx->ErrorMsgAux,"SegNotFound2",vsos.PointVal,lastaid) ; return(NULL) ;
	    } ;
	   vsosp = (struct V4Seg__OneSegment *)v4dpi_GetBindBuf(ctx,lastaid,ikde,FALSE,&bytes) ;
	   data = vsosp->Buf ; bytes -= V4IS_IntKey_Bytes ;
	   if (bytes != sdir.Segment[i].Bytes)
	    { 
//	      FREEMTLOCK(gpi->Areas[lastaid].areaLock) ;
	      v_Msg(ctx,ctx->ErrorMsgAux,"SegDirLenBad",segId,i,sdir.Segment[i].Bytes,bytes) ; return(NULL) ;
	    } ;
	   memcpy(ptr,data,bytes) ; ptr += bytes ;
//	   FREEMTLOCK(gpi->Areas[lastaid].areaLock) ;
	 } ; bytes = sdir.TotalBytes ;
/*	See if we are to cache the result */
try_cache:

	if (databytes != NULL) *databytes = bytes ;
	if (!cachectl) return(lclbuf) ;		/* Just return temp pointer to data */


	if (vslc->Count >= (vslc->Max * 0.9))	/* Are we filling up the cache ? */
	 { int i,max ;
	   struct V4Seg__LocalCache *vslc1 ;
	   max = v_CvtToPrime((int)(vslc->Max * 1.5)) ;
	   vslc1 = v4mm_AllocChunk((char *)&vslc->Entry[max] - (char *)vslc,TRUE) ;
	   vslc1->Max = max ; vslc1->Count = vslc->Count ;
	   for(i=0;i<vslc->Max;i++)		/* Re-hash into new space */
	    { if (vslc->Entry[i].Key == 0) continue ;
	      hash = vslc->Entry[i].Key & 0x7fffffff ;
	      for(;;hash++) { hx = (hash % vslc1->Max) ; if (vslc1->Entry[hx].Key == 0) break ; } ;
	      vslc1->Entry[hx] = vslc->Entry[i] ;
	    } ;
	   v4mm_FreeChunk(vslc) ; vslc = vslc1 ;
	 } ;
	hash = segId & 0x7fffffff ;
	for(;;hash++) { hx = (hash % vslc->Max) ; if (vslc->Entry[hx].Key == 0) break ; } ;
	vslc->Entry[hx].Bytes = bytes ; vslc->Count ++ ;
	vslc->Entry[hx].Key = segId ;
	vslc->Entry[hx].Ptr = (char *)v4mm_AllocChunk(bytes,FALSE) ; memcpy(vslc->Entry[hx].Ptr,lclbuf,bytes) ;
	return(vslc->Entry[hx].Ptr) ;
}


/*	P R O J E C T I O N   M O D U L E S			*/

#define LCL_PROJ_MAX 1001		/* Max number of dim-dim cache entries */
struct lcl__Projection {
  int Count ;
  struct {
    unsigned short SrcDim ;
    unsigned short TargetDim ;
    struct V4DPI__ProjectionInfo *prj ;	/* Projection info for dim-dim pair, if NULL then we don't have any info */
   } Cache[LCL_PROJ_MAX] ;
 } ;

/*	v4prj_RemoveProjectionInfo - Removes projection info between two dimensions (ONLY if cached projection) */
/*	Call: logical = v4prj_RemoveProjectionInfo( ctx , srcdim , targetdim , intmodx )
	  where logical is TRUE if projection removed, FALSE if problem (ctx->ErrorMsg updated with problem),
		ctx is context,
		srcdim, targetdim are two dimensions,
		intmodx is calling internal module index			*/

int v4prj_RemoveProjectionInfo(ctx,srcdim,targetdim,intmodx)
  struct V4C__Context *ctx ;
  int srcdim,targetdim ;
  INTMODX intmodx ;
{ struct lcl__Projection *lprc ;
  struct V4DPI__ProjectionInfo *prj ;
  int hash ;

	if (gpi->lprc == NULL) { v_Msg(ctx,NULL,"ProjectUndef",intmodx,targetdim,srcdim) ; return(FALSE) ; } ;
	lprc = gpi->lprc ;
	hash = (((srcdim << 16) + targetdim) & 0x7fffffff) % LCL_PROJ_MAX ;
	for(;;)
	 { if (lprc->Cache[hash].SrcDim == srcdim && lprc->Cache[hash].TargetDim == targetdim) break ;
	   if (lprc->Cache[hash].SrcDim != 0) { hash = (hash+1) % LCL_PROJ_MAX ; continue ; } ;
	   v_Msg(ctx,NULL,"ProjectUndef",intmodx,targetdim,srcdim) ; return(FALSE) ;
	 } ;
/*	hash = index into lprc->Cache of the projection */
	prj = lprc->Cache[hash].prj ;
	if (prj == NULL) { v_Msg(ctx,NULL,"ProjectUndef",intmodx,targetdim,srcdim) ; return(FALSE) ; } ;
	if (prj->Permanent) { v_Msg(ctx,NULL,"ProjectRmvPerm",intmodx,targetdim,srcdim) ; return(FALSE) ; } ;
//	if (prj->hanWhii != UNUSED) han_Close(prj->hanWhii) ;	/* Free memory allocated to cached hash space */
	if (prj->ptrWH != NULL) v4mm_FreeChunk(prj->ptrWH) ;
	lprc->Cache[hash].SrcDim = 0 ; lprc->Cache[hash].TargetDim = 0 ; lprc->Cache[hash].prj = NULL ;
	v4mm_FreeChunk(prj) ;
	return(TRUE) ;
}


DCLSPINLOCK prjGlblLock = UNUSED ;

/*	v4prj_GetProjectionInfo - Returns T/F if have projection info, optional update of pointer to it */
/*	Call: logical = v4prj_GetProjectionInfo( ctx , srcdim , targetdim , prjp )
	  where logical is TRUE if have entry (may return NULL if undefined)
		ctx is context,
		srcdim, targetdim are the two dimensions,
		prjp is pointer to prj pointer and if not NULL is updated (when we have it) with ptr to projection info */

int v4prj_GetProjectionInfo(ctx,srcdim,targetdim,prjp)
  struct V4C__Context *ctx ;
  int srcdim,targetdim ;
  struct V4DPI__ProjectionInfo **prjp ;
{ struct lcl__Projection *lprc ;
  int hash ;

	if (gpi->lprc == NULL)
	 {
	   if (!GRABMTLOCK(prjGlblLock)) return(FALSE) ;
	   gpi->lprc = (struct lcl__Projection *)v4mm_AllocChunk(sizeof *lprc,TRUE) ;
	   FREEMTLOCK(prjGlblLock) ;
	 } ;
	lprc = gpi->lprc ;
	hash = (((srcdim << 16) + targetdim) & 0x7fffffff) % LCL_PROJ_MAX ;
	for(;;)
	 { if (lprc->Cache[hash].SrcDim == srcdim && lprc->Cache[hash].TargetDim == targetdim)
	    { if (prjp != NULL) *prjp = lprc->Cache[hash].prj ; return(TRUE) ; } ;
	   if (lprc->Cache[hash].SrcDim != 0) { hash = (hash+1) % LCL_PROJ_MAX ; continue ; } ;
	   if (prjp != NULL) *prjp = NULL ; return(FALSE) ;
	 } ;
}

/*	v4prj_CacheProjectionInfo - Saves Projection Info in process cache (or NULL if none for dim-dim pair) */
/*	Call: v4prj_CacheProjectionInfo( ctx , srcdim , targetdim , prjp )
	  where 
		ctx is context,
		srcdim, targetdim are the two dimensions,
		prj is pointer to ProjectInfo structure or NULL if we want to remember that we have "looked" but found none */

void v4prj_CacheProjectionInfo(ctx,srcdim,targetdim,prj)
  struct V4C__Context *ctx ;
  int srcdim,targetdim ;
  struct V4DPI__ProjectionInfo *prj ;
{ struct lcl__Projection *lprc ;
  int hash ;

	GRABMTLOCK(prjGlblLock) ;
	if (gpi->lprc == NULL) gpi->lprc = (struct lcl__Projection *)v4mm_AllocChunk(sizeof *lprc,TRUE) ;
	lprc = gpi->lprc ;
	if (lprc->Count >= LCL_PROJ_MAX - 1)
	 v4_error(0,0,"V4DPI","CachePInfo","PICACHEFULL","Projection cache is full with %d entries",lprc->Count) ;
	hash = (((srcdim << 16) + targetdim) & 0x7fffffff) % LCL_PROJ_MAX ;
	for(;;)
	 { if (lprc->Cache[hash].SrcDim == srcdim && lprc->Cache[hash].TargetDim == targetdim)
	    { lprc->Cache[hash].prj = prj ; break ; } ;		/* Update existing entry */
	   if (lprc->Cache[hash].SrcDim != 0) { hash = (hash+1) % LCL_PROJ_MAX ; continue ; } ;
	   lprc->Cache[hash].SrcDim = srcdim ; lprc->Cache[hash].TargetDim = targetdim ;
	   lprc->Cache[hash].prj = prj ; break ;
	 } ;
	FREEMTLOCK(prjGlblLock) ;
	return ;
}

/*	W O R M   H O L E   M O D U L E S			*/

/*	v4wh_InitWH_IntInt - Initializes new WormHole structure */
/*	Call: ptr = v4wh_InitWH_IntInt(ctx, whii, maximum, failpt, respt )
	  where ptr is pointer to maybe newly allocated whii,
		ctx is context,
		whii is whii to init (NULL to allocate),
		maximum is initial maximum number of entries (UNUSED to ignore)
		failpt if not NULL is failure point,
		respt is representative resulting point			*/

void *v4wh_InitWH_IntInt(ctx,whii,maximum,failpt,respt)
  struct V4C__Context *ctx ;
  struct V4DPI__WormHoleIntInt *whii ;
  int maximum ;
  P *failpt,*respt ;
{
  int i,hdr,bytes,pt=UNUSED ;

	if (respt != NULL) { pt = respt->PntType ; }
	 else if (failpt != NULL) { pt = failpt->PntType ; } ;
	if (pt == V4DPI_PntType_Real || pt == V4DPI_PntType_Calendar || pt == V4DPI_PntType_Int2)			/* Mapping from int to double */
	 return(v4wh_InitWH_IntDbl(ctx,(struct V4DPI__WormHoleIntDbl *)whii,maximum,failpt,respt)) ;
	if (pt == V4DPI_PntType_AggRef)				/* Mapping from int to AggRef */
	 return(v4wh_InitWH_IntAgg(ctx,(struct V4DPI__WormHoleIntAgg *)whii,maximum,failpt,respt)) ;
	if (pt == V4DPI_PntType_UOMPer)				/* Mapping from int to UOMPer */
	 return(v4wh_InitWH_IntUOMPer(ctx,(struct V4DPI__WormHoleIntUOMPer *)whii,maximum,failpt)) ;
	if (pt == V4DPI_PntType_GeoCoord)			/* Mapping from int to GeoCoordinate */
	 return(v4wh_InitWH_IntGeo(ctx,(struct V4DPI__WormHoleIntGeo *)whii,maximum,failpt)) ;
	if (whii == NULL)
	 { hdr = (char *)&whii->Entry[0] - (char *)whii ;
	   if (maximum == UNUSED) maximum = V4DPI_WHIntInt_Max ;
	   bytes = hdr + maximum * sizeof(whii->Entry[0]) ;
	   whii = (struct V4DPI__WormHoleIntInt *)v4mm_AllocChunk(bytes,FALSE) ;
	   whii->WHType = V4DPI_WHType_IntInt ;
	   whii->Maximum = maximum ; whii->Count = 0 ;
	   if (failpt != NULL) { memcpy(&whii->Fail,failpt,failpt->Bytes) ; }
	    else { ZPH(&whii->Fail) ; } ;
	   if (respt != NULL) { memcpy(&whii->rPt,respt,respt->Bytes) ; }	/* If no representative, try to use Fail pt */
	    else if (failpt != NULL) { memcpy(&whii->rPt,failpt,failpt->Bytes) ; }
	    else { ZPH(&whii->rPt) ; } ;
//	   if (han == UNUSED) han = v4ctx_FrameHan(ctx) ;		/* Allocate a handle */
//	   whii->Han = han ;
//	   han_SetPointer(han,0,whii) ; han_FreeOnClose(han) ;
	 } ;
	for(i=0;i<whii->Maximum;i++) { whii->Entry[i].Src = WH_EMPTY ; } ;
	return(whii) ;
}

/*	v4wh_PutWH_IntInt - Adds new entry to WH (may have to redefine if too many entries */
/*	Call: ptr = v4wh_PutWH_IntInt( ctx, whii, srcval, destval )
	  where ptr is pointer to possibly new wormhole structure,
		ctx is context,
		whii is current wormhole structure,
		srcval,destval are source & dest values		*/

struct V4DPI__WormHoleIntInt *v4wh_PutWH_IntInt(ctx,whii,srcval,destval)
  struct V4C__Context *ctx ;
  struct V4DPI__WormHoleIntInt *whii ;
  int srcval,destval ;
{ struct V4DPI__WormHoleIntInt *whiix ;
  int newmax,i,j,hdr,bytes ;

	if (whii->Count >= 0.8 * whii->Maximum)			/* Reaching limit? */
	 { newmax = v_CvtToPrime((int)(whii->Maximum * 1.5)) ;	/* Increase maximum */
//printf("Newmax=%d %d\n",newmax,whii->Maximum) ;
	   hdr = (char *)&whii->Entry[0] - (char *)whii ;
	   bytes = hdr + newmax * sizeof(whii->Entry[0]) ;
	   whiix = (struct V4DPI__WormHoleIntInt *)v4mm_AllocChunk(bytes,FALSE) ;
	   memcpy(whiix,whii,hdr) ;			/* Copy over header */
	   whiix->Maximum = newmax ; whiix->Count = 0 ; v4wh_InitWH_IntInt(ctx,whiix,UNUSED,&whii->Fail,(P *)&whii->rPt) ;
	   for(i=0;i<whii->Maximum;i++)
	    { if (whii->Entry[i].Src == WH_EMPTY) continue ;
	      v4wh_PutWH_IntInt(ctx,whiix,whii->Entry[i].Src,whii->Entry[i].Dest) ;
	    } ;
	   v4mm_FreeChunk(whii) ; whii = whiix ;	/* Make new chunk the current one */
	   han_SetPointer(whii->Han,0,whii) ;		/* Link to new structure */
	 } ;

	i = srcval % whii->Maximum ; if (i < 0) i = -i ;
//if ((whii->Count % 250) == 0)
//{ int v,empty ;
//  for(v=0,empty=0;v<whii->Maximum;v++)
//   { if (whii->Entry[v].Src == WH_EMPTY) empty++ ; } ;
//  printf("Count=%d Empty=%d Sum=%d\n",whii->Count,empty,whii->Count+empty) ;
//} ;
	for(j=whii->Maximum;;)
	 { if (whii->Entry[i].Src == WH_EMPTY) break ;
	   if (whii->Entry[i].Src == srcval) { whii->Entry[i].Dest = destval ; return(whii) ; } ;
	   j /= 2 ; if (j < 1) j = 1 ;
	   i = (i+j) % whii->Maximum ;
	 } ;
	whii->Entry[i].Src = srcval ;
	whii->Entry[i].Dest = destval ;
	whii->Count++ ;
	return(whii) ;					/* Return current whii */
}

/*	v4wh_InitWH_IntDbl - Initializes new WormHole structure */
/*	Call: ptr = v4wh_InitWH_IntDbl(ctx, whid, maximum, failpt, respt )
	  where ptr is pointer to maybe newly allocated whid,
		ctx is context,
		whid is whid to init (NULL to allocate),
		maximum is initial maximum number of entries (UNUSED to ignore)
		failpt if not NULL is failure point,
		respt is representative resulting point			*/

void *v4wh_InitWH_IntDbl(ctx,whid,maximum,failpt,respt)
  struct V4C__Context *ctx ;
  struct V4DPI__WormHoleIntDbl *whid ;
  int maximum ;
  P *failpt,*respt ;
{
  int i,hdr,bytes ;

	if (whid == NULL)
	 { hdr = (char *)&whid->Entry[0] - (char *)whid ;
	   if (maximum == UNUSED) maximum = V4DPI_WHIntInt_Max ;
	   bytes = hdr + maximum * sizeof(whid->Entry[0]) ;
	   whid = (struct V4DPI__WormHoleIntDbl *)v4mm_AllocChunk(bytes,FALSE) ;
	   whid->WHType = V4DPI_WHType_IntDbl ;
	   whid->Maximum = maximum ; whid->Count = 0 ;
	   if (failpt != NULL) { memcpy(&whid->Fail,failpt,failpt->Bytes) ; }
	    else { ZPH(&whid->Fail) ; } ;
	   if (respt != NULL) { memcpy(&whid->rPt,respt,respt->Bytes) ; }	/* If no representative, try to use Fail pt */
	    else if (failpt != NULL) { memcpy(&whid->rPt,failpt,failpt->Bytes) ; }
	    else { ZPH(&whid->rPt) ; } ;
//	   if (han == UNUSED) han = v4ctx_FrameHan(ctx) ;		/* Allocate a handle */
//	   whid->Han = han ;
//	   han_SetPointer(han,0,whid) ; han_FreeOnClose(han) ;
	 } ;
	for(i=0;i<whid->Maximum;i++) { whid->Entry[i].Src = WH_EMPTY ; } ;
	return(whid) ;
}

/*	v4wh_InitWH_IntAgg - Initializes new WormHole structure */
/*	Call: ptr = v4wh_InitWH_IntAgg(ctx, whid, maximum, failpt, respt )
	  where ptr is pointer to maybe newly allocated whid,
		ctx is context,
		whid is whid to init (NULL to allocate),
		maximum is initial maximum number of entries (UNUSED to ignore)
		failpt if not NULL is failure point,
		respt is representative resulting point			*/

void *v4wh_InitWH_IntAgg(ctx,whid,maximum,failpt,respt)
  struct V4C__Context *ctx ;
  struct V4DPI__WormHoleIntAgg *whid ;
  int maximum ;
  P *failpt,*respt ;
{
  int i,hdr,bytes ;

	if (whid == NULL)
	 { hdr = (char *)&whid->Entry[0] - (char *)whid ;
	   if (maximum == UNUSED) maximum = V4DPI_WHIntInt_Max ;
	   bytes = hdr + maximum * sizeof(whid->Entry[0]) ;
	   whid = (struct V4DPI__WormHoleIntAgg *)v4mm_AllocChunk(bytes,FALSE) ;
	   whid->WHType = V4DPI_WHType_IntAgg ;
	   whid->Maximum = maximum ; whid->Count = 0 ;
	   if (failpt != NULL) { memcpy(&whid->Fail,failpt,failpt->Bytes) ; }
	    else { ZPH(&whid->Fail) ; } ;
	   if (respt != NULL) { memcpy(&whid->rPt,respt,respt->Bytes) ; }	/* If no representative, try to use Fail pt */
	    else if (failpt != NULL) { memcpy(&whid->rPt,failpt,failpt->Bytes) ; }
	    else { ZPH(&whid->rPt) ; } ;
//	   if (han == UNUSED) han = v4ctx_FrameHan(ctx) ;		/* Allocate a handle */
//	   whid->Han = han ;
//	   han_SetPointer(han,0,whid) ; han_FreeOnClose(han) ;
	 } ;
	for(i=0;i<whid->Maximum;i++) { whid->Entry[i].Src = WH_EMPTY ; } ;
	return(whid) ;
}

/*	v4wh_InitWH_IntUOMPer - Initializes new WormHole structure */
/*	Call: ptr = v4wh_InitWH_IntUOMPer(ctx, whid, maximum, failpt )
	  where ptr is pointer to maybe newly allocated whid,
		ctx is context,
		whid is whid to init (NULL to allocate),
		maximum is initial maximum number of entries (UNUSED to ignore)
		failpt if not NULL is failure point			*/

void *v4wh_InitWH_IntUOMPer(ctx,whid,maximum,failpt)
  struct V4C__Context *ctx ;
  struct V4DPI__WormHoleIntUOMPer *whid ;
  int maximum ;
  P *failpt ;
{
  int i,hdr,bytes ;

	if (whid == NULL)
	 { hdr = (char *)&whid->Entry[0] - (char *)whid ;
	   if (maximum == UNUSED) maximum = V4DPI_WHIntInt_Max ;
	   bytes = hdr + maximum * sizeof(whid->Entry[0]) ;
	   whid = (struct V4DPI__WormHoleIntUOMPer *)v4mm_AllocChunk(bytes,FALSE) ;
	   whid->WHType = V4DPI_WHType_IntUOMPer ;
	   whid->Maximum = maximum ; whid->Count = 0 ;
	   if (failpt != NULL) { memcpy(&whid->Fail,failpt,failpt->Bytes) ; }
	    else { ZPH(&whid->Fail) ; } ;
//	   if (respt != NULL) { memcpy(&whid->rPt,respt,respt->Bytes) ; }	/* If no representative, try to use Fail pt */
//	    else if (failpt != NULL) { memcpy(&whid->rPt,failpt,failpt->Bytes) ; }
//	    else { ZPH(&whid->rPt) ; } ;
//	   if (han == UNUSED) han = v4ctx_FrameHan(ctx) ;		/* Allocate a handle */
//	   whid->Han = han ;
//	   han_SetPointer(han,0,whid) ; han_FreeOnClose(han) ;
	 } ;
	for(i=0;i<whid->Maximum;i++) { whid->Entry[i].Src = WH_EMPTY ; } ;
	return(whid) ;
}

/*	v4wh_InitWH_IntGeo - Initializes new WormHole structure */
/*	Call: ptr = v4wh_InitWH_IntGeo(ctx, whid, maximum, failpt )
	  where ptr is pointer to maybe newly allocated whid,
		ctx is context,
		whid is whid to init (NULL to allocate),
		maximum is initial maximum number of entries (UNUSED to ignore)
		failpt if not NULL is failure point			*/

void *v4wh_InitWH_IntGeo(ctx,whid,maximum,failpt)
  struct V4C__Context *ctx ;
  struct V4DPI__WormHoleIntGeo *whid ;
  int maximum ;
  P *failpt ;
{
  int i,hdr,bytes ;

	if (whid == NULL)
	 { hdr = (char *)&whid->Entry[0] - (char *)whid ;
	   if (maximum == UNUSED) maximum = V4DPI_WHIntInt_Max ;
	   bytes = hdr + maximum * sizeof(whid->Entry[0]) ;
	   whid = (struct V4DPI__WormHoleIntGeo *)v4mm_AllocChunk(bytes,FALSE) ;
	   whid->WHType = V4DPI_WHType_IntGeo ;
	   whid->Maximum = maximum ; whid->Count = 0 ;
	   if (failpt != NULL) { memcpy(&whid->Fail,failpt,failpt->Bytes) ; }
	    else { ZPH(&whid->Fail) ; } ;
//	   if (respt != NULL) { memcpy(&whid->rPt,respt,respt->Bytes) ; }	/* If no representative, try to use Fail pt */
//	    else if (failpt != NULL) { memcpy(&whid->rPt,failpt,failpt->Bytes) ; }
//	    else { ZPH(&whid->rPt) ; } ;
//	   if (han == UNUSED) han = v4ctx_FrameHan(ctx) ;		/* Allocate a handle */
//	   whid->Han = han ;
//	   han_SetPointer(han,0,whid) ; han_FreeOnClose(han) ;
	 } ;
	for(i=0;i<whid->Maximum;i++) { whid->Entry[i].Src = WH_EMPTY ; } ;
	return(whid) ;
}

/*	v4wh_PutWH_IntDbl - Adds new entry to WH (may have to redefine if too many entries */
/*	Call: ptr = v4wh_PutWH_IntDbl( ctx, whid, srcval, destval )
	  where ptr is pointer to possibly new wormhole structure,
		ctx is context,
		whid is current wormhole structure,
		srcval,destval are source & dest values		*/

struct V4DPI__WormHoleIntDbl *v4wh_PutWH_IntDbl(ctx,whid,srcval,destval)
  struct V4C__Context *ctx ;
  struct V4DPI__WormHoleIntDbl *whid ;
  int srcval ;
  double destval ;
{ struct V4DPI__WormHoleIntDbl *whidx ;
  int newmax,i,j,hdr,bytes ;

	if (whid->Count >= 0.8 * whid->Maximum)			/* Reaching limit? */
	 { newmax = v_CvtToPrime((int)(whid->Maximum * 1.5)) ;	/* Increase maximum */
	   hdr = (char *)&whid->Entry[0] - (char *)whid ;
	   bytes = hdr + newmax * sizeof(whid->Entry[0]) ;
	   whidx = (struct V4DPI__WormHoleIntDbl *)v4mm_AllocChunk(bytes,FALSE) ;
	   memcpy(whidx,whid,hdr) ;			/* Copy over header */
	   whidx->Maximum = newmax ; whidx->Count = 0 ; v4wh_InitWH_IntDbl(ctx,whidx,UNUSED,&whid->Fail,(P *)&whid->rPt) ;
	   for(i=0;i<whid->Maximum;i++)
	    { if (whid->Entry[i].Src == WH_EMPTY) continue ;
	      v4wh_PutWH_IntDbl(ctx,whidx,whid->Entry[i].Src,whid->Entry[i].Dest) ;
	    } ;
	   v4mm_FreeChunk(whid) ; whid = whidx ;	/* Make new chunk the current one */
	   han_SetPointer(whid->Han,0,whid) ;		/* Link to new structure */
	 } ;

	i = srcval % whid->Maximum ; if (i < 0) i = -i ;
	for(j=whid->Maximum;;)
	 { if (whid->Entry[i].Src == WH_EMPTY) break ;
	   if (whid->Entry[i].Src == srcval) { whid->Entry[i].Dest = destval ; return(whid) ; } ;
	   j /= 2 ; if (j < 1) j = 1 ;
	   i = (i+j) % whid->Maximum ;
	 } ;
	whid->Entry[i].Src = srcval ;
	whid->Entry[i].Dest = destval ;
	whid->Count++ ;
	return(whid) ;					/* Return current whid */
}

/*	v4wh_PutWH_IntAgg - Adds new entry to WH (may have to redefine if too many entries */
/*	Call: ptr = v4wh_PutWH_IntAgg( ctx, whid, srcval, destval )
	  where ptr is pointer to possibly new wormhole structure,
		ctx is context,
		whid is current wormhole structure,
		srcval,keyval,keyindex are source & dest values		*/

struct V4DPI__WormHoleIntAgg *v4wh_PutWH_IntAgg(ctx,whia,srcval,keyval,keyindex)
  struct V4C__Context *ctx ;
  struct V4DPI__WormHoleIntAgg *whia ;
  int srcval ;
  int keyval,keyindex ;
{ struct V4DPI__WormHoleIntAgg *whiax ;
  int newmax,i,j,hdr,bytes ;

	if (whia->Count >= 0.8 * whia->Maximum)			/* Reaching limit? */
	 { newmax = v_CvtToPrime((int)(whia->Maximum * 1.5)) ;	/* Increase maximum */
	   hdr = (char *)&whia->Entry[0] - (char *)whia ;
	   bytes = hdr + newmax * sizeof(whia->Entry[0]) ;
	   whiax = (struct V4DPI__WormHoleIntAgg *)v4mm_AllocChunk(bytes,FALSE) ;
	   memcpy(whiax,whia,hdr) ;			/* Copy over header */
	   whiax->Maximum = newmax ; whiax->Count = 0 ; v4wh_InitWH_IntAgg(ctx,whiax,UNUSED,&whia->Fail,(P *)&whia->rPt) ;
	   for(i=0;i<whia->Maximum;i++)
	    { if (whia->Entry[i].Src == WH_EMPTY) continue ;
	      v4wh_PutWH_IntAgg(ctx,whiax,whia->Entry[i].Src,whia->Entry[i].AggKeyValue,whia->Entry[i].AggIndex) ;
	    } ;
	   v4mm_FreeChunk(whia) ; whia = whiax ;	/* Make new chunk the current one */
	   han_SetPointer(whia->Han,0,whia) ;		/* Link to new structure */
	 } ;

	i = srcval % whia->Maximum ; if (i < 0) i = -i ;
	for(j=whia->Maximum;;)
	 { if (whia->Entry[i].Src == WH_EMPTY) break ;
	   if (whia->Entry[i].Src == srcval)
	    {whia->Entry[i].AggIndex = keyindex ; whia->Entry[i].AggKeyValue = keyval ; return(whia) ; } ;
	   j /= 2 ; if (j < 1) j = 1 ;
	   i = (i+j) % whia->Maximum ;
	 } ;
	whia->Entry[i].Src = srcval ;
	whia->Entry[i].AggIndex = keyindex ;
	whia->Entry[i].AggKeyValue = keyval ;
	whia->Count++ ;
	return(whia) ;					/* Return current whia */
}

/*	v4wh_PutWH_IntUOMPer - Adds new entry to WH (may have to redefine if too many entries */
/*	Call: ptr = v4wh_PutWH_IntUOMPer( ctx, whid, srcval, uomper )
	  where ptr is pointer to possibly new wormhole structure,
		ctx is context,
		whid is current wormhole structure,
		srcval,uomper are source & dest values		*/

struct V4DPI__WormHoleIntUOMPer *v4wh_PutWH_IntUOMPer(ctx,whia,srcval,uomper)
  struct V4C__Context *ctx ;
  struct V4DPI__WormHoleIntUOMPer *whia ;
  int srcval ;
  struct V4DPI__Value_UOMPer *uomper ;
{ struct V4DPI__WormHoleIntUOMPer *whiax ;
  int newmax,i,j,hdr,bytes ;

	if (whia->Count >= 0.8 * whia->Maximum)			/* Reaching limit? */
	 { newmax = v_CvtToPrime((int)(whia->Maximum * 1.5)) ;	/* Increase maximum */
	   hdr = (char *)&whia->Entry[0] - (char *)whia ;
	   bytes = hdr + newmax * sizeof(whia->Entry[0]) ;
	   whiax = (struct V4DPI__WormHoleIntUOMPer *)v4mm_AllocChunk(bytes,FALSE) ;
	   memcpy(whiax,whia,hdr) ;			/* Copy over header */
	   whiax->Maximum = newmax ; whiax->Count = 0 ;
	   v4wh_InitWH_IntUOMPer(ctx,whiax,UNUSED,&whia->Fail) ;
	   for(i=0;i<whia->Maximum;i++)
	    { if (whia->Entry[i].Src == WH_EMPTY) continue ;
	      v4wh_PutWH_IntUOMPer(ctx,whiax,whia->Entry[i].Src,&whia->Entry[i].UOMPer) ;
	    } ;
	   v4mm_FreeChunk(whia) ; whia = whiax ;	/* Make new chunk the current one */
	   han_SetPointer(whia->Han,0,whia) ;		/* Link to new structure */
	 } ;

	i = srcval % whia->Maximum ; if (i < 0) i = -i ;
	for(j=whia->Maximum;;)
	 { if (whia->Entry[i].Src == WH_EMPTY) break ;
	   if (whia->Entry[i].Src == srcval)
	    { whia->Entry[i].UOMPer = *uomper ; return(whia) ; } ;
	   j /= 2 ; if (j < 1) j = 1 ;
	   i = (i+j) % whia->Maximum ;
	 } ;
	whia->Entry[i].Src = srcval ;
	whia->Entry[i].UOMPer = *uomper ;
	whia->Count++ ;
	return(whia) ;					/* Return current whia */
}

/*	v4wh_PutWH_IntGeo - Adds new entry to WH (may have to redefine if too many entries */
/*	Call: ptr = v4wh_PutWH_IntGeo( ctx, whid, srcval, geo )
	  where ptr is pointer to possibly new wormhole structure,
		ctx is context,
		whid is current wormhole structure,
		srcval,keyval,keyindex are source & dest values		*/

struct V4DPI__WormHoleIntGeo *v4wh_PutWH_IntGeo(ctx,whia,srcval,geo)
  struct V4C__Context *ctx ;
  struct V4DPI__WormHoleIntGeo *whia ;
  int srcval ;
  struct V4DPI__Value_GeoCoord *geo ;
{ struct V4DPI__WormHoleIntGeo *whiax ;
  int newmax,i,j,hdr,bytes ;

	if (whia->Count >= 0.8 * whia->Maximum)			/* Reaching limit? */
	 { newmax = v_CvtToPrime((int)(whia->Maximum * 1.5)) ;	/* Increase maximum */
	   hdr = (char *)&whia->Entry[0] - (char *)whia ;
	   bytes = hdr + newmax * sizeof(whia->Entry[0]) ;
	   whiax = (struct V4DPI__WormHoleIntGeo *)v4mm_AllocChunk(bytes,FALSE) ;
	   memcpy(whiax,whia,hdr) ;			/* Copy over header */
	   whiax->Maximum = newmax ; whiax->Count = 0 ;
	   v4wh_InitWH_IntGeo(ctx,whiax,UNUSED,&whia->Fail) ;
	   for(i=0;i<whia->Maximum;i++)
	    { if (whia->Entry[i].Src == WH_EMPTY) continue ;
	      v4wh_PutWH_IntGeo(ctx,whiax,whia->Entry[i].Src,&whia->Entry[i].Geo) ;
	    } ;
	   v4mm_FreeChunk(whia) ; whia = whiax ;	/* Make new chunk the current one */
	   han_SetPointer(whia->Han,0,whia) ;		/* Link to new structure */
	 } ;

	i = srcval % whia->Maximum ; if (i < 0) i = -i ;
	for(j=whia->Maximum;;)
	 { if (whia->Entry[i].Src == WH_EMPTY) break ;
	   if (whia->Entry[i].Src == srcval)
	    { whia->Entry[i].Geo = *geo ; return(whia) ; } ;
	   j /= 2 ; if (j < 1) j = 1 ;
	   i = (i+j) % whia->Maximum ;
	 } ;
	whia->Entry[i].Src = srcval ;
	whia->Entry[i].Geo = *geo ;
	whia->Count++ ;
	return(whia) ;					/* Return current whia */
}

/*	v4wh_InitWH_LIntLInt - Initializes new WormHole structure */
/*	Call: ptr = v4wh_InitWH_LIntLInt(ctx, whll, maximum, failpt, respt )
	  where ptr is pointer to maybe newly allocated whll,
		ctx is context,
		whll is whll to init (NULL to allocate),
		maximum is initial maximum number of entries (UNUSED to ignore)
		failpt if not NULL is failure point,
		respt is representative resulting point			*/

void *v4wh_InitWH_LIntLInt(ctx,whll,maximum,failpt,respt)
  struct V4C__Context *ctx ;
  struct V4DPI__WormHoleLIntLInt *whll ;
  int maximum ;
  P *failpt,*respt ;
{
  int i,hdr,bytes,pt=UNUSED ;

	if (respt != NULL) { pt = respt->PntType ; }
	 else if (failpt != NULL) { pt = failpt->PntType ; } ;
	if (whll == NULL)
	 { hdr = (char *)&whll->Entry[0] - (char *)whll ;
	   if (maximum == UNUSED) maximum = V4DPI_WHIntInt_Max ;
	   bytes = hdr + maximum * sizeof(whll->Entry[0]) ;
	   whll = (struct V4DPI__WormHoleLIntLInt *)v4mm_AllocChunk(bytes,FALSE) ;
	   whll->WHType = V4DPI_WHType_LIntLInt ;
	   whll->Maximum = maximum ; whll->Count = 0 ;
	   if (failpt != NULL) { memcpy(&whll->Fail,failpt,failpt->Bytes) ; }
	    else { ZPH(&whll->Fail) ; } ;
	   if (respt != NULL) { memcpy(&whll->rPt,respt,respt->Bytes) ; }	/* If no representative, try to use Fail pt */
	    else if (failpt != NULL) { memcpy(&whll->rPt,failpt,failpt->Bytes) ; }
	    else { ZPH(&whll->rPt) ; } ;
//	   if (han == UNUSED) han = v4ctx_FrameHan(ctx) ;		/* Allocate a handle */
//	   whll->Han = han ;
//	   han_SetPointer(han,0,whll) ; han_FreeOnClose(han) ;
	 } ;
	for(i=0;i<whll->Maximum;i++) { whll->Entry[i].Src = WH_EMPTY ; } ;
	return(whll) ;
}

/*	v4wh_PutWH_LIntLInt - Adds new entry to WH (may have to redefine if too many entries */
/*	Call: ptr = v4wh_PutWH_LIntLInt( ctx, whll, srcval, destval )
	  where ptr is pointer to possibly new wormhole structure,
		ctx is context,
		whll is current wormhole structure,
		srcval,destval are source & dest values		*/

struct V4DPI__WormHoleLIntLInt *v4wh_PutWH_LIntLInt(ctx,whll,srcval,destval)
  struct V4C__Context *ctx ;
  struct V4DPI__WormHoleLIntLInt *whll ;
  B64INT srcval,destval ;
{ struct V4DPI__WormHoleLIntLInt *whllx ;
  int newmax,i,j,hdr,bytes ;

	if (whll->Count >= 0.8 * whll->Maximum)			/* Reaching limit? */
	 { newmax = v_CvtToPrime((int)(whll->Maximum * 1.5)) ;	/* Increase maximum */
	   hdr = (char *)&whll->Entry[0] - (char *)whll ;
	   bytes = hdr + newmax * sizeof(whll->Entry[0]) ;
	   whllx = (struct V4DPI__WormHoleLIntLInt *)v4mm_AllocChunk(bytes,FALSE) ;
	   memcpy(whllx,whll,hdr) ;			/* Copy over header */
	   whllx->Maximum = newmax ; whllx->Count = 0 ; v4wh_InitWH_LIntLInt(ctx,whllx,UNUSED,&whll->Fail,(P *)&whll->rPt) ;
	   for(i=0;i<whll->Maximum;i++)
	    { if (whll->Entry[i].Src == WH_EMPTY) continue ;
	      v4wh_PutWH_LIntLInt(ctx,whllx,whll->Entry[i].Src,whll->Entry[i].Dest) ;
	    } ;
	   v4mm_FreeChunk(whll) ; whll = whllx ;	/* Make new chunk the current one */
	   han_SetPointer(whll->Han,0,whll) ;		/* Link to new structure */
	 } ;

	i = (int)(srcval % whll->Maximum) ; if (i < 0) i = -i ;
	for(j=whll->Maximum;;)
	 { if (whll->Entry[i].Src == WH_EMPTY) break ;
	   if (whll->Entry[i].Src == srcval) { whll->Entry[i].Dest = destval ; return(whll) ; } ;
	   j /= 2 ; if (j < 1) j = 1 ;
	   i = (i+j) % whll->Maximum ;
	 } ;
	whll->Entry[i].Src = srcval ;
	whll->Entry[i].Dest = destval ;
	whll->Count++ ;
	return(whll) ;					/* Return current whll */
}
