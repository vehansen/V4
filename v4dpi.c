/*	V4DPI.C - V4 Dimension, Point, & Intersection Modules

	Create 31-Mar-92 by Victor E. Hansen			*/

#ifndef NULLID
#include "v4defs.c"
#include "v4imdefs.c"
#include <time.h>
#include <setjmp.h>
#include <sys/types.h>
#endif

#ifdef WINNT
#include <process.h>
#endif
#ifdef UNIX
#include <unistd.h>
#endif

GLOBALDIMSEXTERN
extern struct V4C__ProcessInfo *gpi ;	/* Global process information */
extern ETYPE traceGlobal ;
V4DEBUG_SETUP

#define LOCKROOT lrx = v4mm_LockSomething(aid,V4MM_LockId_Root,V4MM_LockMode_Write,V4MM_LockIdType_Root,5,NULL,3)
#define RELROOT v4mm_LockRelease(aid,lrx,mmm->MyPid,aid,"RELROOTDPI") ; v4mm_TempLock(aid,0,FALSE)

extern struct V__UnicodeInfo *uci ;	/* Global structure of Unicode Info */
extern int opcodeDE[] ;
extern struct V4DPI__LittlePoint CondEvalRet ;

/*	intPNTvMod - Functional version of intPNTv() macro - to get around macro expansion issues when second argument is NULL */
void intPNTvMod(respnt,intVal)
  struct V4DPI__Point *respnt ;
  int intVal ;
{
	if (respnt == NULL) return ;
	intPNTv(respnt,intVal) ;
}


/*	D I M E N S I O N   &	N I D	D I M E N S I O N   M O D U L E S	*/

/*	v4dpi_DimInitOnType - Inits diminfo structure with point-type dependant defaults */

void v4dpi_DimInitOnType(ctx,diminfo)
  struct V4C__Context *ctx ;
  struct V4DPI__DimInfo *diminfo ;
{
	switch (diminfo->PointType)
	 { 
	   case V4DPI_PntType_Calendar:	diminfo->ds.Cal.CalendarType = VCAL_CalType_Gregorian ; diminfo->ds.Cal.TimeZone = VCAL_TimeZone_Local ; break ;
	   case V4DPI_PntType_AggRef:	diminfo->UniqueOK = V4DPI_DimInfo_UOkAgg ; break ;
	   case V4DPI_PntType_BigText:	diminfo->UniqueOK = V4DPI_DimInfo_UOkInt ; break ;
	 } ;
}


/*	v4dpi_DimMake - Makes a new dimension and enters into area		*/
/*	Call: dimnum = v4dpi_DimMake( ctx , diminfo )
	  where dimnum is new dimension number (0 if already error or UNUSED if dimension already exists),
		ctx is current context,
		diminfo is pointer to dimension info structure- V4DPI__DimInfo	*/

int v4dpi_DimMake(ctx,diminfo)
  struct V4C__Context *ctx ;
  struct V4DPI__DimInfo *diminfo ;
{ struct V4DPI__DimInfo *dix ;
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__AreaCB *acb ;
  struct V4DPI__DimUnique du ;
  struct V4DPI__DUList dul ;
  int rx,bytes ; AREAID aid ; INDEX areax ;


/*	Make sure semantics of diminfo are OK */
	switch (diminfo->UniqueOK)
	 { default: break ;
	   case V4DPI_DimInfo_UOkNew:
		switch (diminfo->PointType)
		 { default: v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntCreType",diminfo->PointType,DE(PointCreate),DE(New)) ; return(0) ;
		   case V4DPI_PntType_CodedRange:
		   case V4DPI_PntType_Int:
		   case V4DPI_PntType_AggRef:
		   case V4DPI_PntType_BigText:
		   case V4DPI_PntType_Time:	break ;
		 } ; break ;
	   case V4DPI_DimInfo_UOkPoint:
		switch (diminfo->PointType)
		 { default:	v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntCreType",diminfo->PointType,DE(PointCreate),DE(Point)) ; return(0) ;
		   case V4DPI_PntType_CodedRange:
		   case V4DPI_PntType_Real:
		   case V4DPI_PntType_Int:
		   case V4DPI_PntType_XDict:
		   case V4DPI_PntType_Dict:
		   case V4DPI_PntType_Time:	break ;
		 } ; break ;
	 } ;
/*	If dimension is LIST with target dimension (via ENTRIES) then set "PointCreate NEW" */
	if (diminfo->PointType == V4DPI_PntType_List && diminfo->ListDimId > 0)
	 diminfo->UniqueOK = V4DPI_DimInfo_UOkNew ;
	if (diminfo->Flags & V4DPI_DimInfo_RangeOK)
	 { switch (diminfo->PointType)
	    { default:	v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntRange",diminfo->PointType) ; return(0) ;
	      case V4DPI_PntType_Fixed:
	      case V4DPI_PntType_UOM:
	      case V4DPI_PntType_UOMPer:
	      case V4DPI_PntType_Real:
	      case V4DPI_PntType_Calendar:
	      case V4DPI_PntType_UTime:
	      CASEofINT
	      CASEofChar
	      case V4DPI_PntType_XDict:
	      case V4DPI_PntType_Dict:
		break ;
	    } ;
	 } ;
	if (diminfo->Flags & V4DPI_DimInfo_ListOK)
	 { switch (diminfo->PointType)
	    { default:	v_Msg(ctx,ctx->ErrorMsgAux,"CmdPntMultiple",diminfo->PointType) ; return(0) ;
	      CASEofINT CASEofChar
	      case V4DPI_PntType_Real: case V4DPI_PntType_Fixed: case V4DPI_PntType_SSVal: case V4DPI_PntType_Tree:
	      case V4DPI_PntType_FrgnDataEl: case V4DPI_PntType_Dict: case V4DPI_PntType_XDict:
	      case V4DPI_PntType_Shell: case V4DPI_PntType_UOM: case V4DPI_PntType_UOMPer: case V4DPI_PntType_UOMPUOM: case V4DPI_PntType_Country:
	      case V4DPI_PntType_FrgnStructEl: case V4DPI_PntType_Color: case V4DPI_PntType_Calendar: case V4DPI_PntType_UTime:
		break ;
	    } ;
	 } ;

	diminfo->DimId = v4dpi_DictEntryGet(ctx,0,diminfo->DimName,DIMDICT,NULL) ;	/* Dictionary entry already out there ? */
	if (diminfo->DimId != 0)
	 { if ((dix = v4dpi_DimInfoGet(ctx,diminfo->DimId)) != NULL)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"DPIDimExists",dix->DimId) ; return(UNUSED) ; } ;
	 } ;
	if (UCstrcmpIC(diminfo->DimName,UClit("DIM")) == 0)
	 { diminfo->Flags |= V4DPI_DimInfo_Dim ;	/* Flag DIM dimension! */
	   diminfo->DictType = V4DPI_DictType_Dim ;
	 } ;
/*	Try and assign a RHnum (rx) to this dimension */
	if (diminfo->RelHNum != 0) { rx = diminfo->RelHNum ; }
	 else { for(rx=gpi->HighHNum;rx>=gpi->LowHNum;rx--)			/* Find highest area allowing new entries */
		 { if (rx == V4DPI_WorkRelHNum) continue ;			/* Don't default to work area! */
		   if (gpi->RelH[rx].aid == UNUSED) continue ; if (gpi->RelH[rx].ahi.ExtDictUpd) break ;
		 } ;
		if (rx < gpi->LowHNum && gpi->RelH[V4DPI_WorkRelHNum].aid != UNUSED) /* If all else fails - assign to process work */
		 rx = V4DPI_WorkRelHNum ;
		if (rx < gpi->LowHNum) { v_Msg(ctx,ctx->ErrorMsgAux,"CtxNoUpdArea1") ; return(0) ; } ;
		diminfo->RelHNum = rx ;
	      } ;
	aid = gpi->RelH[rx].aid ; acb = v4mm_ACBPtrRE(aid) ;
	if (acb == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"DimNoHier",diminfo->DimName,diminfo->RelHNum) ; return(0) ; } ;
	if (diminfo->DimId == 0)
	 diminfo->DimId = v4dpi_DictEntryPut(ctx,aid,diminfo->DimName,V4DPI_DictType_Dim,0,diminfo) ;
	if (diminfo->DimId == 0) return(0) ;
	diminfo->kp.fld.AuxVal = V4IS_SubType_DimInfo ; diminfo->kp.fld.KeyType = V4IS_KeyType_V4 ;
	diminfo->kp.fld.KeyMode = V4IS_KeyMode_Int ; diminfo->kp.fld.Bytes = sizeof diminfo->kp + sizeof diminfo->DimId ;
/*	Figure out how much to save- don't save auto-context point if none specified */
	bytes = sizeof *diminfo ;
	FINDAREAINPROCESS(areax,aid)
#ifdef TRACEGRAB
printf("Dimmake %d\n",mmm->Areas[areax].areaLock) ;
#endif
	GRABAREAMTLOCK(areax) ;
	if (v4is_Insert(aid,(struct V4IS__Key *)diminfo,diminfo,bytes,V4IS_PCB_DataMode_Auto,0,0,FALSE,FALSE,0,ctx->ErrorMsgAux) == -1) return(0) ;
	FREEAREAMTLOCK(areax) ;
	switch (diminfo->UniqueOK)
	 {
	   case V4DPI_DimInfo_UOkInt:
	   case V4DPI_DimInfo_UOkNew:					/* If allowed to use {New} then... */
		du.kp.fld.KeyType = V4IS_KeyType_V4 ; du.kp.fld.KeyMode = V4IS_KeyMode_Int ;
		du.kp.fld.AuxVal = V4IS_SubType_DimUnique ; du.kp.fld.Bytes = sizeof du.kp + sizeof du.DimId ;
		du.DimId = diminfo->DimId ;	/* Set up key field */
		du.Revision = 0 ;		/* Start with Revision 0 */
		du.NextAvailPntNum = (diminfo->UniqueOK == V4DPI_DimInfo_UOkInt ? diminfo->DimId << 20 : 0) + 1 ;
/*		Write out initialized record so v4dpi_DimUnique will work later on in this area */
		GRABAREAMTLOCK(areax) ;
		if (v4is_Insert(aid,(struct V4IS__Key *)&du,&du,sizeof du,V4IS_PCB_DataMode_Index,0,0,FALSE,FALSE,0,ctx->ErrorMsgAux) == -1) { FREEAREAMTLOCK(areax) ; return(0) ; } ;
		FREEAREAMTLOCK(areax) ;
		break ;
	   case V4DPI_DimInfo_UOkPoint: 				/* If allowed to use POINT then ... */
		memset(&dul,0,sizeof dul) ;
		dul.kp.fld.KeyType = V4IS_KeyType_V4 ; dul.kp.fld.KeyMode = V4IS_KeyMode_Int ;
		dul.kp.fld.AuxVal = V4IS_SubType_DUList ; dul.kp.fld.Bytes = sizeof dul.kp + sizeof dul.DimId ;
		dul.DimId = diminfo->DimId ;	/* Set up key field */
		dul.lp.Bytes = 0 ; dul.lp.ListType = V4L_ListType_Point ; dul.lp.Entries = 0 ; dul.lp.BytesPerEntry = 0 ;
		dul.lp.Dim = 0 ; dul.lp.PntType = 0 ; dul.lp.LPIOffset = 0 ; dul.lp.FFBufx = 0 ;
		GRABAREAMTLOCK(areax) ;
		if (v4is_Insert(aid,(struct V4IS__Key *)&dul,&dul,sizeof dul,V4IS_PCB_DataMode_Auto,0,0,FALSE,FALSE,0,ctx->ErrorMsgAux) == -1) { FREEAREAMTLOCK(areax) ; return(0) ; } ;
		FREEAREAMTLOCK(areax) ;
		break ;
	 } ;


/*	All done making dimension, maybe add to vldd */
	if (gpi->MainTcb == NULL || gpi->vldd == NULL) return(diminfo->DimId) ;		/* Don't appear to be compiling */
	{ struct V4LEX__TknCtrlBlk *tcb = gpi->MainTcb ; P v4pt ; INDEX ilx,ilxSave ;
	  for(ilx=tcb->ilx;ilx > 0 && tcb->ilvl[ilx].mode != V4LEX_InpMode_File;ilx--) { } ;
	  if (ilx <= 0) return(diminfo->DimId) ;					/* Only want files (not macro expansions) */
	  if (gpi->vldd->count >= DIMDIR_Max)
	   { struct V4LEX__DIMDIR *tvldd = v4mm_AllocChunk(sizeof *tvldd,FALSE) ; tvldd->vlddPrior = gpi->vldd ; gpi->vldd = tvldd ; gpi->vldd->count = 0 ; } ;
	  ilxSave = tcb->ilx ; tcb->ilx = ilx ;				/* This is really ugly, but I want to use ISCTVCD macro, so deal with it */
	  ISCTVCD(&v4pt) ; gpi->vldd->entry[gpi->vldd->count].dimId = diminfo->DimId ; gpi->vldd->entry[gpi->vldd->count].vis = v4pt.Value.Isct.vis ;
	  gpi->vldd->entry[gpi->vldd->count].usage = DIMREF_DCL ;
	  gpi->vldd->count++ ;
	  tcb->ilx = ilxSave ;
	}
	return(diminfo->DimId) ;					/* Return new ID */
}

/*	v4dpi_DimUniqueName - Returns unique dimension name from base name component	*/
/*	Call: uname = v4dpi_DimUniqueName( ctx , namebase , unamebuf )
	  where uname is unique name (NULL if no unique name possible),
		ctx is pointer to current context,
		name is character string base name (digits added to end to make unique),
		unamebuf is updated with unique name					*/

UCCHAR *v4dpi_DimUniqueName(ctx,namebase,unamebuf)
  struct V4C__Context *ctx ;
  UCCHAR *namebase, *unamebuf ;
{ int i ;

	for(i=1;i<100000;i++)
	 { v_Msg(ctx,unamebuf,"@%1U_L%2d",namebase,i) ;
	   if (UCstrlen(unamebuf) > V4DPI_DimInfo_DimNameMax)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"DPIDimUnqLong",namebase) ; return(NULL) ; } ;
	   if (v4dpi_DimGet(ctx,unamebuf,DIMREF_IRT) == 0) return(unamebuf) ;
	 } ;
	v_Msg(ctx,ctx->ErrorMsgAux,"DPIDimNoUnq",namebase) ;
	return(NULL) ;
} ;

/*	v4dpi_LocalDimAddEndBlock - Adds/Ends Block for Nested Local Dimension Names	*/
/*	Call: v4dpi_LocalDimAddEndBlock( ctx , flag )
	  where ok is TRUE if success, FALSE if failure,
		ctx is point to current context,
		flag is TRUE to add new block, FALSE to remove				*/

LOGICAL v4dpi_LocalDimAddEndBlock(ctx,flag)
  struct V4C__Context *ctx ;
  LOGICAL flag ;
{
	if (ctx == NULL) return(TRUE) ;				/* No context? */
	if (gpi->dnb == NULL) return(TRUE) ;			/* No local dimensions */
	if (flag)
	 { if (gpi->dnb->BlockX >= V4DPI_DimNameBlockMax-1)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"DPIDimNameBlk",V4DPI_DimNameBlockMax) ; return(FALSE) ; } ;
	   gpi->dnb->Block[gpi->dnb->BlockX+1].NameCount = gpi->dnb->Block[gpi->dnb->BlockX].NameCount ;
	   gpi->dnb->BlockX++ ;
	   return(TRUE) ;
	 } ;
/*	Here to remove current block */
	if (gpi->dnb->BlockX <= 0)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"DPIDimBlkBase") ; return(FALSE) ; } ;
	gpi->dnb->BlockX-- ;
	return(TRUE) ;
}

/*	v4dpi_DimGet - Returns dimension number for a dimension name		*/
/*	Call: dimnum = v4dpi_DimGet( ctx , name , reftype )
	  where dimnum is dimension number (0 if name is not a dimension),
		ctx is pointer to current context,
		name is character string name of dimension,
		reftype is how dimension is reference (DIMREF_xxx)		*/

DIMID v4dpi_DimGet(ctx,name,reftype)
  struct V4C__Context *ctx ;
  UCCHAR *name ;
  ETYPE reftype ;
{
  struct V4DPI__DimInfo *di ;
  P v4pt,ipnt,*ipt ;
  enum DictionaryEntries deval ;
#define LCMAX 73
#define LCTHRASH1 67
  struct lcl__Cache {
   int HashDiv ;		/* Used to generate hash - starts with LCLMAX */
   int Count ;
   struct {
    struct V4DPI__DimInfo *di ;
    int Dim ;
    int Thrash ;
    UCCHAR DimName[V4DPI_DimInfo_DimNameMax+1] ;
    } Entry[LCMAX] ;
   } ; static struct lcl__Cache *lc = NULL ;
  int hx,i ; DIMID dimId ;
  union {
    UCCHAR abuf[V4DPI_DimInfo_DimNameMax+1] ;
   } t ;
#ifdef V4ENABLEMULTITHREADS
  static DCLSPINLOCK dgLock = UNUSED ;
#endif
  struct V4LEX__TknCtrlBlk *tcb ;

	if (!GRABMTLOCK(dgLock)) { v_Msg(ctx,ctx->ErrorMsgAux,"MTCantLock",UClit("dgLock")) ; return(0) ; } ;
	if (lc == NULL) { lc = (struct lcl__Cache *)v4mm_AllocChunk(sizeof *lc,TRUE) ; lc->HashDiv = LCMAX ; } ;
	memset(&t,0,sizeof(t)) ;
	UCcnvupper(t.abuf,name,UCsizeof(t.abuf)) ;
	if (gpi->dnb != NULL)					/* If we got local names then lookup before we leap */
	 { for(i=gpi->dnb->Block[gpi->dnb->BlockX].NameCount-1;i>=0;i--)
	    { if (UCstrcmpIC(t.abuf,gpi->dnb->Names[i].DimName) != 0) continue ;
	      name = gpi->dnb->Names[i].IDimName ;		/* Found it - replace with real name */
	      UCstrcpy(t.abuf,name) ;
	      break ;
	    } ;
	 } ;
	VHASH32_FWD(hx,t.abuf,UCstrlen(t.abuf)) ;
	hx = hx % lc->HashDiv ; if (hx < 0) hx = -hx ;
	if (UCstrcmp(lc->Entry[hx].DimName,t.abuf) == 0)
	 { FREEMTLOCK(dgLock) ;
#ifdef V4_BUILD_SECURITY
	   if (lc->Entry[hx].di->rtFlags & V4DPI_rtDimInfo_Hidden) return(0) ;
#endif
	   if (gpi->vldd == NULL) { return(lc->Entry[hx].Dim) ; } else { dimId = lc->Entry[hx].Dim ; goto vldd_entry ; } ;
	 } ;
	dimId = v4dpi_DictEntryGet(ctx,0,name,DIMDICT,NULL) ;
	if (dimId != 0) ;
	 { DIMINFO(di,ctx,dimId) ;
	   if (di != NULL)			/* Got a dictionary entry, make sure it's a dim */
	    { 
#ifdef V4_BUILD_SECURITY
	      if (di->rtFlags & V4DPI_rtDimInfo_Hidden) return(0) ;
#endif
	      UCstrcpy(lc->Entry[hx].DimName,t.abuf) ; lc->Entry[hx].Dim = dimId ; lc->Entry[hx].di = di ;
	      lc->Entry[hx].Thrash++ ;
	      if (lc->Entry[hx].Thrash > 100)				/* If too much thrashing - try & change hash function */
	       { lc->HashDiv = (lc->HashDiv == LCMAX ? LCTHRASH1 : LCMAX) ;
		 lc->Entry[hx].Thrash = 0 ;
	       } ;
	      FREEMTLOCK(dgLock) ;
	      if (gpi->vldd == NULL) { return(dimId) ; } else { goto vldd_entry ; } ;
	    } ;
	 } ;
	if (UCstrcmp(name,UClit("UV4")) == 0) { FREEMTLOCK(dgLock) ; return(0) ; } ;
	INITISCT(&v4pt) ; NOISCTVCD(&v4pt) ; v4pt.Grouping = 2 ;				/* Construct [UV4:DimSynonym xxx] intersection */
	ipt = ISCT1STPNT(&v4pt) ; dictPNTv(ipt,Dim_UV4,v4im_GetEnumToDictVal(ctx,deval=_DimSynonym,UNUSED)) ; v4pt.Bytes += ipt->Bytes ;
	if (ipt->Dim == 0 || ipt->Value.IntVal == 0) { FREEMTLOCK(dgLock) ; return(0) ; } ;
	ADVPNT(ipt) ; dictPNTv(ipt,Dim_NId,v4dpi_DictEntryGet(ctx,UNUSED,name,NULL,NULL)) ; v4pt.Bytes += ipt->Bytes ;
	if (ipt->Dim == 0 || ipt->Value.IntVal == 0) { FREEMTLOCK(dgLock) ; return(0) ; } ;
	ipt = v4dpi_IsctEval(&ipnt,&v4pt,ctx,V4DPI_EM_NoIsctFail|V4DPI_EM_FailOK,NULL,NULL) ;
	FREEMTLOCK(dgLock) ;
	if (ipt == NULL) return(0) ;				/* No synonym for this dimension */
	DIMINFO(di,ctx,ipt->Value.IntVal) ;
	if (di == NULL) return(0) ;
#ifdef V4_BUILD_SECURITY
	if (di->rtFlags & V4DPI_rtDimInfo_Hidden) return(0) ;
#endif
	if (gpi->vldd == NULL) { return(ipt->Value.IntVal) ; } else { dimId = ipt->Value.IntVal ; goto vldd_entry ; } ;

vldd_entry:
	if (gpi->MainTcb == NULL) return(dimId) ;		/* Don't appear to be compiling */
	tcb = gpi->MainTcb ; 
	{ INDEX ilx ;
	  for(ilx=tcb->ilx;ilx>=0;ilx--)
	   { if (tcb->ilvl[ilx].mode == V4LEX_InpMode_File) break ; } ;
	  if (ilx < 0) return(dimId) ;				/* Could not find any source code */
	  if (gpi->vldd->count >= DIMDIR_Max)
	   { struct V4LEX__DIMDIR *tvldd = v4mm_AllocChunk(sizeof *tvldd,FALSE) ; tvldd->vlddPrior = gpi->vldd ; gpi->vldd = tvldd ; gpi->vldd->count = 0 ; } ;
	  ISCTVCDX(&v4pt,ilx) ;
	}
	gpi->vldd->entry[gpi->vldd->count].dimId = dimId ; gpi->vldd->entry[gpi->vldd->count].vis = v4pt.Value.Isct.vis ;
	gpi->vldd->entry[gpi->vldd->count].usage = 3 ;
	gpi->vldd->count++ ;
	return(dimId) ;
}

/*	v4dpi_DimInfoGet - Returns pointer to dimension info structure for a dimension */
/*	Call: diminfo = v4dpi_DimInfoGet( ctx , dimid )
	  where diminfo is pointer to V4DPI__DimInfo,
		ctx is current context,
		dimid is dimension ID					*/

struct V4DPI__DimInfo *v4dpi_DimInfoGet(ctx,dimid)
  struct V4C__Context *ctx ;
  int dimid ;
{ struct V4DPI__DimInfo di,*di1 ;
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  int hx,rx,bytes ; INDEX areax ;

/*	See if we already have info stored in context dimension hash table */
	hx = DIMIDHASH(dimid) ; 					/* Hash to get dimension slot */
	for(;;)
	 { if (ctx->DimHash[hx].Dim == dimid)
	    { if (ctx->DimHash[hx].di != NULL) return(ctx->DimHash[hx].di) ;	/* Found in hash - return pointer */
	      goto load_dim ;
	    } ;
	   if (ctx->DimHash[hx].Dim == 0) break ;
	   hx++ ; hx = DIMIDHASH(hx) ;					/* Slot busy - try another */
	 } ;
	if (++ctx->DimHashCnt >= V4C_CtxDimHash_Max-1)			/* Have to init to load new active dimension */
	 v4_error(0,0,"V4DPI","DimInfo","MAXACTDIM","Exceeded max number of active dimensions (%d)",V4C_CtxDimHash_Max) ;
	ctx->DimHash[hx].Dim = dimid ;
	ctx->DimHash[hx].di = NULL ;
	ctx->DimHash[hx].CtxValIndex = -1 ;				/* Not linked to any point (yet) */
load_dim:
	di.kp.fld.AuxVal = V4IS_SubType_DimInfo ; di.kp.fld.KeyType = V4IS_KeyType_V4 ;
	di.kp.fld.KeyMode = V4IS_KeyMode_Int ; di.kp.fld.Bytes = sizeof di.kp + sizeof di.DimId ;
	di.DimId = dimid ;
	if ( (rx = XTRHNUMDIM(dimid)) == 0) rx = gpi->LowHNum ;		/* Try to get HNum from dimid */
	if (rx < 0 || rx > gpi->HighHNum || gpi->RelH[rx].aid == UNUSED)
	 { ctx->DimHash[hx].Dim = 0 ; if (ctx->DimHash[hx].di == NULL) ctx->DimHashCnt-- ; return(NULL) ; } ;
	for(;rx<=gpi->HighHNum;rx++)
	 { if (gpi->RelH[rx].aid == UNUSED) continue ;
	   FINDAREAINPROCESS(areax,gpi->RelH[rx].aid)
#ifdef TRACEGRAB
printf("diminfoget %d %d\n",areax,mmm->Areas[areax].areaLock) ;
#endif
	   if (!GRABAREAMTLOCK(areax)) continue ;
	   if (v4is_PositionKey(gpi->RelH[rx].aid,(struct V4IS__Key *)&di,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) == V4RES_PosKey) break ;
	   FREEAREAMTLOCK(areax) ;
	 } ;
	if (rx > gpi->HighHNum)
	 { FREEAREAMTLOCK(areax) ; ctx->DimHash[hx].Dim = 0 ; if (ctx->DimHash[hx].di == NULL) ctx->DimHashCnt-- ; return(NULL) ; } ;
	di1 = (struct V4DPI__DimInfo *)v4dpi_GetBindBuf(ctx,gpi->RelH[rx].aid,ikde,FALSE,&bytes) ;
	ctx->DimHash[hx].di = (struct V4DPI__DimInfo *)v4mm_AllocChunk(sizeof di,FALSE) ;	/* Save info in dimension hash */
	memcpy(ctx->DimHash[hx].di,di1,bytes) ;		/* VEH050715 - Just copy # bytes from GetBindBuf */
							/* problem: (sizeof *di) is bigger than bytes for new versions, occasional memory faults */
	FREEAREAMTLOCK(areax) ;
	ctx->DimHash[hx].di->rtFlags = 0 ;				/* Clear all runtime flags */
/*	Maybe tweak this dimension's RelH info based on current context */
	di1 = ctx->DimHash[hx].di ;
	for(rx=gpi->HighHNum;rx>=gpi->LowHNum;rx--)			/* Find highest area allowing new entries */
	 { if (gpi->RelH[rx].aid == UNUSED) continue ; if (rx == V4DPI_WorkRelHNum) continue ;
	   if (gpi->RelH[rx].ahi.ExtDictUpd) break ;
	 } ;
	di1->RelHNum = (rx < gpi->LowHNum ? 0 : rx) ;
	di1->vcr = NULL ;
	return(ctx->DimHash[hx].di) ;					/* Finally return pointer */
}

/*	v4dpi_DimShellDimId - Returns DimId of point referring to a shell dimension	*/
/*	Call: dimid = v4dpi_DimShellDimId( ok , ctx , pnt )
	  where dimid is DimId of shell point,
		ok updated with TRUE/FALSE (ctx->ErrorMsgAux updated with error),
		ctx is current context,
		pnt is point from which shell dimension is to be extracted		*/

int v4dpi_DimShellDimId(ok,ctx,pnt)
  LOGICAL *ok ;
  struct V4C__Context *ctx ;
  struct V4DPI__Point *pnt ;
{ struct V4DPI__DimInfo *di ;
  int dim ;

	*ok = FALSE ;
	if (pnt == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"IsctEvalNull") ; return(UNUSED) ; } ;
	if (pnt->Dim == Dim_Dim)
	 { DIMINFO(di,ctx,pnt->Value.IntVal) ;
	   if (di->PointType != V4DPI_PntType_Shell) { v_Msg(ctx,ctx->ErrorMsgAux,"DimWrongType",di->DimId,V4DPI_PntType_Shell) ; return(UNUSED) ; } ;
	   *ok = TRUE ; return(pnt->Value.IntVal) ;
	 } ;
/*	Point not a dimension - if it is NId then try to convert to dimension */
	if (pnt->Dim == Dim_NId)
	 { dim = v4dpi_DimGet(ctx,v4dpi_RevDictEntryGet(ctx,pnt->Value.IntVal),DIMREF_IRT) ;
	   if (dim > 0)
	    { DIMINFO(di,ctx,dim) ;
	      if (di->PointType != V4DPI_PntType_Shell) { v_Msg(ctx,ctx->ErrorMsgAux,"DimWrongType",di->DimId,V4DPI_PntType_Shell) ; return(UNUSED) ; } ;
	      *ok = TRUE ; return(dim) ;
	    } ;
	 } ;
	v_Msg(ctx,ctx->ErrorMsgAux,"DimNotDim",pnt) ; return(UNUSED) ;
}

/*	v4dpi_DimIndexDimId - Returns DimId of point referring to an index dimension		*/
/*	Call: dimid = v4dpi_DimIndexDimId( ok , ctx , pnt , startVal )
	  where dimid is DimId of index point,
		ok updated with TRUE/FALSE (ctx->ErrorMsgAux updated with error),
		ctx is current context,
		pnt is point from which shell dimension is to be extracted,
		startVal, if not NULL, is updated with starting index value (default is 1)	*/

int v4dpi_DimIndexDimId(ok,ctx,pnt,startVal)
  LOGICAL *ok ;
  struct V4C__Context *ctx ;
  struct V4DPI__Point *pnt ;
  INDEX *startVal ;
{ struct V4DPI__DimInfo *di ;
  struct V4DPI__Point tpnt ;
  int dim ;

	*ok = FALSE ;
	if (pnt == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"IsctEvalNull") ; return(UNUSED) ; } ;
	if (pnt->Dim == Dim_Dim)
	 { DIMINFO(di,ctx,pnt->Value.IntVal) ;
	   if (!(di->PointType == V4DPI_PntType_Int || di->PointType == V4DPI_PntType_Real)) { v_Msg(ctx,ctx->ErrorMsgAux,"DimWrongType",di->DimId,V4DPI_PntType_Int) ; return(UNUSED) ; } ;
	   *ok = TRUE ; if (startVal != NULL) *startVal = 1 ;
	   return(pnt->Value.IntVal) ;
	 } ;
/*	Point not a dimension - if it is NId then try to convert to dimension */
	if (pnt->Dim == Dim_NId)
	 { dim = v4dpi_DimGet(ctx,v4dpi_RevDictEntryGet(ctx,pnt->Value.IntVal),DIMREF_IRT) ;
	   if (dim > 0)
	    { DIMINFO(di,ctx,dim) ;
	      if (!(di->PointType == V4DPI_PntType_Int || di->PointType == V4DPI_PntType_Real)) { v_Msg(ctx,ctx->ErrorMsgAux,"DimWrongType",di->DimId,V4DPI_PntType_Int) ; return(UNUSED) ; } ;
	      *ok = TRUE ;  if (startVal != NULL) *startVal = 1 ;
	      return(dim) ;
	    } ;
	 } ;
	if (pnt->PntType == V4DPI_PntType_Special || pnt->PntType == V4DPI_PntType_Isct)
	 { P *ipt = v4dpi_IsctEval(&tpnt,pnt,ctx,V4DPI_EM_NoIsctFail,NULL,NULL) ;
	   if (ipt == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdIsctFail",pnt) ; return(UNUSED) ; } ;
	   pnt = ipt ;
	 } ;
	switch (pnt->PntType)
	 { CASEofINT	*ok = TRUE ; if (startVal != NULL) *startVal = pnt->Value.IntVal ; return(pnt->Dim) ;
	 } ;
	v_Msg(ctx,ctx->ErrorMsgAux,"DimNotDim",pnt) ; return(UNUSED) ;
}

/*	v4dpi_PointToDimId - Converts point to dimension id						*/
/*	Call: dimid = v4dpi_PointToDimId( ctx , point , di )
	  where dimid is corresponding dimension, UNUSED if failed (ctx->ErrorMsgAux with error),
		ctx is context,
		point is point to convert to dimension
		di, if not NULL, updated to point to dimension info					*/

DIMID v4dpi_PointToDimId(ctx,point,di,intmodx,argx)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *point ;
  struct V4DPI__DimInfo **di ;
  INTMODX intmodx ;
  INDEX argx ;
{
  DIMID dimid ;

	if (point == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"IsctEvalNull") ; return(UNUSED) ; } ;
	if (point->Dim == Dim_Dim) { dimid = point->Value.IntVal ; }
	 else { switch (point->PntType)
		 { default:
			v4dpi_PointToString(UCTBUF1,point,ctx,V4DPI_FormatOpt_Echo) ;
			dimid = v4dpi_DimGet(ctx,UCTBUF1,DIMREF_IRT) ; break ;
		   case V4DPI_PntType_Dict:
			dimid = v4dpi_DimGet(ctx,v4dpi_RevDictEntryGet(ctx,point->Value.IntVal),DIMREF_IRT) ; break ;
		 } ;
	      } ;
	if (dimid <= 0)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"ModArgNotDimCvt",intmodx,argx,point) ; return(UNUSED) ; } ;
	if (di != NULL) { DIMINFO(*di,ctx,dimid) ;	 } ;
	return(dimid) ;
}

#define LCL_MAX 100
  struct lcl__Cache {				/* Local cache to hold points with MassUpdate or from read-only areas */
    int Count ;
    int DimId[LCL_MAX] ;
    int Dirty[LCL_MAX] ;			/* TRUE if entry for dimension has been updated - need to flush */
    int Revision[LCL_MAX] ;			/* Highest revision of this dimension */
    int PntNum[LCL_MAX] ;
    int FirstNum[LCL_MAX] ;			/* VEH051107 - First number to use when creating list of points (so we can reset dimension via V4(dim Remove?)) */
   } ;
static struct lcl__Cache *dulc = NULL ;

/*	v4dpi_DimUnqToCache - Finds dimension in dulc cache, if not there looks in areas & inserts	*/
/*	Call: index = v4dpi_DimUnqToCache( ctx , di ),
	  where index is index in dulc cache, UNUSED if not found anywhere,
		ctx is context,
		di is diminfo of dimension in question							*/

int v4dpi_DimUnqToCache(ctx,di)
  struct V4C__Context *ctx ;
  struct V4DPI__DimInfo *di ;
{ 
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4DPI__DimUnique du,duhr,*dux ;
  int i,lowrx,hghrx,rx ; INDEX areax ;

	if (dulc != NULL)				/* Check to see if already in cache */
	 { for(i=0;i<dulc->Count;i++) { if (dulc->DimId[i] == di->DimId) return(i) ; } ; } ;
	lowrx = gpi->LowHNum ; hghrx = gpi->HighHNum ;	/* Look thru all available areas */
	duhr.Revision = UNUSED ;
	for(rx=lowrx;rx<=hghrx;rx++)
	 { if (gpi->RelH[rx].aid == UNUSED) continue ;
	   du.kp.fld.KeyType = V4IS_KeyType_V4 ; du.kp.fld.KeyMode = V4IS_KeyMode_Int ; du.kp.fld.AuxVal = V4IS_SubType_DimUnique ;
	   du.kp.fld.Bytes = sizeof du.kp + sizeof du.DimId ; du.DimId = di->DimId ;	/* Set up key field */
	   FINDAREAINPROCESS(areax,gpi->RelH[rx].aid)
#ifdef TRACEGRAB
printf("Unqtoca %d\n",mmm->Areas[areax].areaLock) ;
#endif
	   if (!GRABAREAMTLOCK(areax)) continue ;
	   if (v4is_PositionKey(gpi->RelH[rx].aid,(struct V4IS__Key *)&du,(struct V4IS__Key **)&dux,0,V4IS_PosDCKL) != V4RES_PosKey)
	    { FREEAREAMTLOCK(areax) ; continue ; } ;
	   if (dux->Revision > 1)
	    { if (duhr.Revision > 0) { FREEAREAMTLOCK(areax) ; continue ; } ;	/* veh040423 - Temp code for cross-over */
	      duhr.Revision = 0 ; duhr.NextAvailPntNum = dux->Revision ;
	    } else
	    { if (dux->Revision > duhr.Revision) duhr = *dux ;	/* Track highest revision */
	    } ;
	   FREEAREAMTLOCK(areax) ;
	 } ;
	if (duhr.Revision == UNUSED)				/* Could not find anywhere? */
	 { FREEAREAMTLOCK(areax) ; return(UNUSED) ; } ;
	if (di->PointType == V4DPI_PntType_Isct)			 /* If this is Isct (i.e. BigIsct) then don't pick up revision */
	 { FREEAREAMTLOCK(areax) ; return(UNUSED) ; } ; /*   return UNUSED to force new (random) point value */
	if (dulc == NULL) dulc = (struct lcl__Cache *)v4mm_AllocChunk(sizeof *dulc,TRUE) ;
	if (dulc->Count < LCL_MAX)					/* Force into local cache */
	 { dulc->DimId[dulc->Count] = di->DimId ; dulc->PntNum[dulc->Count] = duhr.NextAvailPntNum ; dulc->Dirty[dulc->Count] = FALSE ;
	   dulc->Revision[dulc->Count] = duhr.Revision ; dulc->FirstNum[dulc->Count] = 1 ; dulc->Count++ ;
	 } ;
	FREEAREAMTLOCK(areax) ;
	return(dulc->Count-1) ;
}

#define LCL_UPMAX 100		/* Max number of cache entries below */
struct lcl__DUPCache {
 int Count ;
 struct {
  struct V4DPI__DUList *dul ;
  int DimId ;			/* Dimension of entry */
  int Dirty ;			/* TRUE if entry has been updated (for end-of-process flush) */
  int Revision ;		/* Revision number (updated if written to another area) */
 } Entry[LCL_UPMAX] ;
} ;
static struct lcl__DUPCache *duplc = NULL ;
#ifdef V4ENABLEMULTITHREADS
  static DCLSPINLOCK dupLock = UNUSEDSPINLOCKVAL ;
#endif

int v4dpi_DimUnqRmvFromCache(ctx,di)	/* Remove di->DimId from duplc cache - returns number of points removed */
  struct V4C__Context *ctx ;
  struct V4DPI__DimInfo *di ;
{ int i,cnt,num ;
//  int trace=0 ;	/* To make SizeOfList macro work */

	GRABMTLOCK(dupLock) ;
	if (dulc != NULL)
	 { for(i=0;i<dulc->Count;i++)
	    { if (dulc->DimId[i] != di->DimId) continue ;
	      cnt = dulc->PntNum[i] - dulc->FirstNum[i] ;
	      dulc->FirstNum[i] = dulc->PntNum[i] ;		/* Effectively remove all points from dimension */
	      FREEMTLOCK(dupLock) ; return(cnt) ;
	    } ;
	 } ;
	
	if (duplc == NULL) { FREEMTLOCK(dupLock) ; return(0) ; } ;
	cnt = 0 ;
	for(i=0;i<duplc->Count;i++)
	 { if (duplc->Entry[i].DimId != di->DimId) continue ;
	   num = SIZEofLIST(&duplc->Entry[i].dul->lp) ;
	   if (num != V4L_ListSize_Unk) cnt += num ;
	   duplc->Entry[i] = duplc->Entry[--duplc->Count] ;
	 }
	FREEMTLOCK(dupLock) ; return(cnt) ;
}


int v4dpi_DimUnqPntToCache(ctx,di)
  struct V4C__Context *ctx ;
  struct V4DPI__DimInfo *di ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4DPI__DUList dul,*dulx,dulhr ;
  int i,lowrx,hghrx,rx ; INDEX areax ;

/*	First search for entry in cache */
	GRABMTLOCK(dupLock) ; 
	if (duplc != NULL)
	 { for(i=0;i<duplc->Count;i++)
	    { if (duplc->Entry[i].DimId == di->DimId) { FREEMTLOCK(dupLock) ; return(i) ; } ; } ;
	 } ;
	FREEMTLOCK(dupLock) ;
	lowrx = gpi->LowHNum ; hghrx = gpi->HighHNum ;		/* Look thru all available areas */
	dulhr.Revision = UNUSED ;
	for(rx=lowrx;rx<=hghrx;rx++)
	 { if (gpi->RelH[rx].aid == UNUSED) continue ;
	   dul.kp.fld.KeyType = V4IS_KeyType_V4 ; dul.kp.fld.KeyMode = V4IS_KeyMode_Int ;
	   dul.kp.fld.AuxVal = V4IS_SubType_DUList ;
	   dul.kp.fld.Bytes = sizeof dul.kp + sizeof dul.DimId ; dul.DimId = di->DimId ; /* Set up key field */
	   FINDAREAINPROCESS(areax,gpi->RelH[rx].aid)
#ifdef TRACEGRAB
printf("dimunqpnttoca %d\n",mmm->Areas[areax].areaLock) ;
#endif
	   GRABAREAMTLOCK(areax) ;
	   if (v4is_PositionKey(gpi->RelH[rx].aid,(struct V4IS__Key *)&dul,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey) { FREEAREAMTLOCK(areax) ; continue ; } ;
	   dulx = (struct V4DPI__DUList *)v4dpi_GetBindBuf(ctx,gpi->RelH[rx].aid,ikde,FALSE,NULL) ;

//veh040426 temp code for cross-over to new structure
//	   if (dulx->Revision > 3)	/* Got old structure - shift it */
//	    { struct V4DPI__DUList *dulz ; dulz = (struct V4DPI__DUList *)v4mm_AllocChunk(sizeof *dulz,FALSE) ;
//	      memcpy(&dulz->lp,&dulx->Revision,(dulx->Revision & 0xffff)) ; dulz->Revision = 0 ;
//	      dulz->DimId = dulx->DimId ; dulz->kp = dulx->kp ;
//	      dulx = dulz ;	/* Not very good code, but temp so who cares */
//	    } ;
	   if (dulx->Revision > dulhr.Revision) dulhr = *dulx ;	/* Save if higher revision */
	   FREEAREAMTLOCK(areax) ;
	 } ;
	if (dulhr.Revision == UNUSED) return(UNUSED) ;		/* Did not find anything? */
	GRABMTLOCK(dupLock) ; 
	if (duplc == NULL) duplc = (struct lcl__DUPCache *)v4mm_AllocChunk(sizeof *duplc,TRUE) ;
	if (duplc->Count >= LCL_UPMAX) { FREEMTLOCK(dupLock) ; return(UNUSED) ; } ;		/* Too many entries */
	duplc->Entry[duplc->Count].DimId = di->DimId ; duplc->Entry[duplc->Count].Dirty = FALSE ;
	duplc->Entry[duplc->Count].Revision = dulhr.Revision ;
	duplc->Entry[duplc->Count].dul = (struct V4DPI__DUList *)v4mm_AllocChunk(sizeof dulhr,FALSE) ;
	memcpy(duplc->Entry[duplc->Count].dul,&dulhr,sizeof dulhr) ;
	FREEMTLOCK(dupLock) ; return(duplc->Count++) ;
}

/*	v4dpi_AddDUPoint - Adds point to dimension's list (called from POINT command in V4Eval) */
/*	Call: ok = v4dpi_AddDUPoint( ctx , di , point, pointval , dupact)
	  where ok is TRUE if all went well, FALSE if error (in ctx->ErrorMsgAux),
		ctx is current context,
		di is dimension info (if NULL then flush all),
		point is pointer to point-value to be used, if NULL use pointval (int),
		pointval is integer value of point being added to list,
		dupact is action to take on duplicate points (see V_DUPDICTACT_xxx)		*/

LOGICAL v4dpi_AddDUPoint(ctx,di,point,pointval,dupact)
  struct V4C__Context *ctx ;
  struct V4DPI__DimInfo *di ;
  struct V4DPI__Point *point ;
  int pointval ;
  ETYPE dupact ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4DPI__DUList *dul ;
  struct V4DPI__LittlePoint pt ;
  int rx,lowrx,hghrx,i,cx, res ; INDEX areax ;

	if (di == NULL)						/* Flush out all updated lists? */
	 { if (duplc == NULL) return(TRUE) ;				/*  but nothing to do */
	   lowrx = gpi->LowHNum ; hghrx = gpi->HighHNum ;
	   for(i=0;i<duplc->Count;i++)
	    { if (!duplc->Entry[i].Dirty) continue ;
	      dul = duplc->Entry[i].dul ;
	      for(rx=hghrx;rx>=lowrx;rx--)			/* Scan looking for first updatable area with wanted info */
	       { if (gpi->RelH[rx].aid == UNUSED) continue ;
	         FINDAREAINPROCESS(areax,gpi->RelH[rx].aid)
#ifdef TRACEGRAB
printf("adddupoint %d\n",mmm->Areas[areax].areaLock) ;
#endif
	         GRABAREAMTLOCK(areax) ;
	         if (v4is_PositionKey(gpi->RelH[rx].aid,(struct V4IS__Key *)duplc->Entry[i].dul,NULL,0,V4IS_PosDCKL) != V4RES_PosKey) { FREEAREAMTLOCK(areax) ; continue ; } ;
		 if (!gpi->RelH[rx].ahi.IntDictUpd) { FREEAREAMTLOCK(areax) ; continue ; } ;	/* Updates not allowed in this area? */
	         v4is_Replace(gpi->RelH[rx].aid,(struct V4IS__Key *)dul,dul,dul,sizeof *dul,V4IS_PCB_DataMode_Auto,V4IS_DataCmp_None,0,0) ;
		 FREEAREAMTLOCK(areax) ;
		 break ;
	       } ;
	     if (rx < lowrx)					/* Could not write out? */
	      { for(rx=lowrx;rx<=hghrx;rx++)			/* Want to find first area (not temp) that is writable */
		 { if (gpi->RelH[rx].aid == UNUSED) continue ;
		   if (!gpi->RelH[rx].ahi.IntDictUpd) continue ;
		   duplc->Entry[i].dul->Revision++ ;		/* Update revision number */
		   FINDAREAINPROCESS(areax,gpi->RelH[rx].aid)
#ifdef TRACEGRAB
printf("adddupoint2 %d\n",mmm->Areas[areax].areaLock) ;
#endif
		   GRABAREAMTLOCK(areax) ;
		   if (v4is_Insert(gpi->RelH[rx].aid,(struct V4IS__Key *)dul,dul,sizeof *dul,V4IS_PCB_DataMode_Auto,0,0,FALSE,FALSE,0,ctx->ErrorMsgAux) == -1)
		     { FREEAREAMTLOCK(areax) ; v_Msg(ctx,NULL,"DPIDimUnqFlush",di->DimId) ; return(FALSE) ; } ;
		   FREEAREAMTLOCK(areax) ;
		 } ;
	      } ;
	    } ;
	   v4mm_FreeChunk(duplc) ; duplc = NULL ;
	   return(TRUE) ;
	 } ;

/*	Here to add point to dimension, create temp point to be added to dul list */
	if (point == NULL)
	 { point = (P *)&pt ;
	   ZPH(&pt) ; pt.PntType = di->PointType ; pt.Bytes = V4PS_Int ;
	   pt.Dim = di->DimId ; pt.Value.IntVal = pointval ;	/* Set up point with proper value */
	 } ;

/*	See if stuff is in cache */
	cx = v4dpi_DimUnqPntToCache(ctx,di) ;
	GRABMTLOCK(dupLock) ; 
	if (cx == UNUSED)					/* Not in cache - append it */
	 { if (duplc == NULL) duplc = (struct lcl__DUPCache *)v4mm_AllocChunk(sizeof *duplc,TRUE) ;
	   if (duplc->Count >= LCL_UPMAX)			/* Too many entries */
	    { v_Msg(ctx,ctx->ErrorMsgAux,"DPIDimUnqCMax",LCL_UPMAX,di->DimId) ; { FREEMTLOCK(dupLock) ; return(UNUSED) ; } ; } ;
	   cx = duplc->Count++ ;
	   duplc->Entry[cx].DimId = di->DimId ; duplc->Entry[cx].Revision = 0 ;
	   duplc->Entry[cx].dul = (dul = (struct V4DPI__DUList *)v4mm_AllocChunk(sizeof *dul,FALSE)) ;
	   dul->kp.fld.KeyType = V4IS_KeyType_V4 ; dul->kp.fld.KeyMode = V4IS_KeyMode_Int ;
	   dul->kp.fld.AuxVal = V4IS_SubType_DUList ; dul->kp.fld.Bytes = sizeof dul->kp + sizeof dul->DimId ;
	   dul->DimId = di->DimId ;
	   memset(&dul->lp,0,V4L_ListPointHdr_Bytes) ; dul->lp.ListType = V4L_ListType_Point ;
	 } ;

/*	Now just append point to the dul->lp list */
	dul = duplc->Entry[cx].dul ;
	switch (dupact)
	 { case V_DUPDICTACT_Warn:
		res = v4l_ListPoint_Modify(ctx,&dul->lp,V4L_ListAction_AppendUnique,point,0) ;
		if (!res)
		 { v_Msg(ctx,UCTBUF2,"*CmdDupPointW2",di->PointType,point) ;
		   vout_UCText(VOUT_Warn,0,UCTBUF2) ; vout_NL(VOUT_Warn) ;
		 } ;
		if (res == V4L_ModRes_DupPnt)
		 { v_Msg(ctx,UCTBUF2,"*CmdDupPointW",di->PointType,point) ;
		   vout_UCText(VOUT_Warn,0,UCTBUF2) ; vout_NL(VOUT_Warn) ;
		 } ;
		break ;
	   case V_DUPDICTACT_Err:
		res = v4l_ListPoint_Modify(ctx,&dul->lp,V4L_ListAction_AppendUnique,point,0) ;	if (!res) goto fail ;
		if (res == V4L_ModRes_DupPnt)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"CmdDupPointE",di->PointType,point) ; goto fail ; } ;
		break ;
	   case V_DUPDICTACT_Undef:
	   case V_DUPDICTACT_Ignore:
//20150120 - still blows up with this commented
		res = v4l_ListPoint_Modify(ctx,&dul->lp,V4L_ListAction_Append,point,0) ;
		if (!res)
		 { v_Msg(ctx,UCTBUF2,"*CmdDupPointW2",di->PointType,point) ;
		   vout_UCText(VOUT_Warn,0,UCTBUF2) ; vout_NL(VOUT_Warn) ;
		 } ;
		break ;
	 } ;
	duplc->Entry[cx].Dirty = TRUE ;				/* Mark cache entry as dirty */

	FREEMTLOCK(dupLock) ; return(TRUE) ;

fail:	FREEMTLOCK(dupLock) ; return(FALSE) ;
}

/*	v4dpi_DimUnique - Returns next unique point number for a dimension	*/
/*	Call: unique = v4dpi_DimUnique( ctx , di , perm )
	  where unique is next unique number (UNUSED if error),
		ctx is current context,
		di is dimension info ptr (NULL to purge, returns TRUE if OK, FALSE if error in ctx->ErrorMsg),
		perm, if not NULL, is updated TRUE/FALSE depending on if dimunique value stored permanently */

int v4dpi_DimUnique(ctx,di,isperm)
  struct V4C__Context *ctx ;
  struct V4DPI__DimInfo *di ;
  LOGICAL *isperm ;
{ struct V4DPI__DimUnique du,*dux ;
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__AreaCB *acb ;
  struct V4AGG__AggDims *vad ;
  int aid,ax,dx,lx,rx,hghrx,lowrx,unique,i,aggid ; INDEX areax ;

	if (isperm != NULL) *isperm = TRUE ;			/* Assume permanent for now */
	if (di == NULL)						/* Here to flush out cache */
	 { if (dulc == NULL) return(TRUE) ;			/* Nothing to do! */
	   for(i=0;i<dulc->Count;i++)
	    { if (!dulc->Dirty[i]) continue ;
	      du.kp.fld.KeyType = V4IS_KeyType_V4 ; du.kp.fld.KeyMode = V4IS_KeyMode_Int ;
	      du.kp.fld.AuxVal = V4IS_SubType_DimUnique ;
	      du.kp.fld.Bytes = sizeof du.kp + sizeof du.DimId ; du.DimId = dulc->DimId[i] ;
	      lowrx = gpi->LowHNum ; hghrx = gpi->HighHNum ;
	      for(rx=hghrx;rx>=lowrx;rx--)				/* Scan looking for first updatable area with wanted info */
	       { if (gpi->RelH[rx].aid == UNUSED) continue ;
		 if (!gpi->RelH[rx].ahi.IntDictUpd) continue ;		/* Updates not allowed in this area? */
		 if (v4is_PositionKey((aid=gpi->RelH[rx].aid),(struct V4IS__Key *)&du,(struct V4IS__Key **)&dux,0,V4IS_PosDCKL) == V4RES_PosKey) break ;
	       } ;
	      if (rx < lowrx)					/* Could not write where we got it - try another update-able area */
	       { for(rx=lowrx;rx<=hghrx;rx++)				/* Scan looking for first updatable area with wanted info */
		  { if (gpi->RelH[rx].aid == UNUSED) continue ;
		    if (!gpi->RelH[rx].ahi.IntDictUpd) continue ;	/* Updates not allowed in this area? */
		    du.NextAvailPntNum = dulc->PntNum[i] ;		/* Set next available point */
		    du.Revision = dulc->Revision[i] + 1 ;		/* Up the revision number! */
		    FINDAREAINPROCESS(areax,gpi->RelH[rx].aid) ;
#ifdef TRACEGRAB
printf("dimunique %d %d\n",rx,mmm->Areas[areax].areaLock) ;
#endif
		    GRABAREAMTLOCK(areax) ;
		    if (v4is_Insert(gpi->RelH[rx].aid,(struct V4IS__Key *)&du,&du,sizeof du,V4IS_PCB_DataMode_Index,0,0,FALSE,FALSE,0,ctx->ErrorMsgAux) == -1)
		     { FREEAREAMTLOCK(areax) ; v_Msg(ctx,ctx->ErrorMsgAux,"DPIDimUnqFlush",du.DimId) ; return(UNUSED) ; } ;
		    FREEAREAMTLOCK(areax) ;
		    break ;
		  } ;
/*		 If here & rx past limit then sux for this dimension - can't write it out */
		 if (rx > hghrx)
		  { v_Msg(ctx,UCTBUF1,"*DPIDimROArea",du.DimId) ; vout_UCText(VOUT_Warn,0,UCTBUF1) ; } ;
	         continue ;						/* Already gave warning below! */
	       } ;
	      acb = v4mm_ACBPtr(aid) ;
	      lx = v4mm_LockSomething(aid,acb->DataId,V4MM_LockMode_Write,V4MM_LockIdType_Record,5,NULL,4) ;
	      memcpy(&du,dux,sizeof du) ;				/* Copy record into local buffer */
	      FINDAREAINPROCESS(areax,aid) ;
#ifdef TRACEGRAB
printf("dimunique2 %d %d\n",rx,mmm->Areas[areax].areaLock) ;
#endif
	      GRABAREAMTLOCK(areax) ;
	      du.NextAvailPntNum = dulc->PntNum[i] ;			/* Set next available point */
	      du.Revision = dulc->Revision[i] ;
	      v4is_Replace(aid,(struct V4IS__Key *)&du,&du,&du,sizeof du,V4IS_PCB_DataMode_Index,V4IS_DataCmp_None,acb->DataId,0) ;
	      { struct V4MM__MemMgmtMaster *mmm = gpi->mmm ;
	        v4mm_LockRelease(aid,lx,mmm->MyPid,aid,"DimUnique") ;	/* Release the lock */
	      }
	      FREEAREAMTLOCK(areax) ;
	    } ;
	   return(TRUE) ;
	 } ;

	switch (di->UniqueOK == 0 ? V4DPI_DimInfo_UOkInt : di->UniqueOK)
	 { default:
		v_Msg(ctx,ctx->ErrorMsgAux,"DPIDimUnqType",di->DimId) ; return(UNUSED) ;
	   case V4DPI_DimInfo_UOkAgg:
		for(ax=0;ax<gpi->AreaAggCount;ax++)
		 { if (gpi->AreaAgg[ax].pcb == NULL) continue ;
		   if ((gpi->AreaAgg[ax].pcb->AccessMode & (V4IS_PCB_AM_Insert | V4IS_PCB_AM_Update)) != 0) break ;
		 } ;
		if (ax >= gpi->AreaAggCount)
		 { aggid = v4ctx_InitTempAgg(ctx) ; if (aggid == UNUSED) return(UNUSED) ;
		   for(ax=0;ax<gpi->AreaAggCount;ax++)
		    { if (aggid == gpi->AreaAgg[ax].AggUId) break ; } ;
		   if (ax >= gpi->AreaAggCount) return(UNUSED) ;
		 } ;
		if (gpi->AreaAgg[ax].vad == NULL)		         /* If first time for area then create new structure */
		 { gpi->AreaAgg[ax].vad = (struct V4AGG__AggDims *)v4mm_AllocChunk(sizeof *(gpi->AreaAgg[ax].vad),TRUE) ;
		   vad = gpi->AreaAgg[ax].vad ;
		   vad->kp.fld.Bytes = sizeof vad->kp + sizeof vad->KeyVal ;
		   vad->kp.fld.KeyMode = V4IS_KeyMode_Int ; vad->kp.fld.KeyType = V4IS_KeyType_V4 ;
		   vad->kp.fld.AuxVal = V4IS_SubType_AggDims ; vad->KeyVal = 0 ;     /* Format correct key for this structure */
		 } ;
		vad = gpi->AreaAgg[ax].vad ;
		for(dx=0;dx<vad->Count;dx++) { if (vad->Dims[dx] == di->DimId) break ; } ;
		if (dx >= vad->Count)				  /* New dimension in area? */
		 { if (vad->Count >= V4AGG_AggDims_Max)
		    { v_Msg(ctx,ctx->ErrorMsgAux,"DPIDimUnqAggMax",di->DimId,gpi->AreaAgg[ax].pcb->UCFileName) ; return(UNUSED) ; } ;
		   vad->Dims[vad->Count] = di->DimId ; vad->Entry[vad->Count].BeginPt = 0 ;
		   dx = vad->Count++ ;
		 } ;
		++vad->Entry[dx].EndPt ;
		if (vad->Entry[dx].BeginPt == 0) vad->Entry[dx].BeginPt = vad->Entry[dx].EndPt ;
		return(vad->Entry[dx].EndPt) ;
	   case V4DPI_DimInfo_UOkNew:
	   case V4DPI_DimInfo_UOkInt:
		i = v4dpi_DimUnqToCache(ctx,di) ;			/* Look for dim in cache (load if necessary) */
		if (i == UNUSED)					/* Didn't get anything ? */
		 { 
		   if (di->PointType == V4DPI_PntType_Isct)		/* If this is Isct (for BigIsct entries) */
		    { struct { int walltime ; int cputime ; } tunique ; 
		      tunique.walltime = time(NULL) ; tunique.cputime = clock() * CURRENTPID ;
		      VHASH32_FWDb(unique,&tunique,sizeof tunique) ;
		      
		    } else { unique = (di->DimId << 20) + 1 ; } ;
		   if (dulc == NULL) dulc = (struct lcl__Cache *)v4mm_AllocChunk(sizeof *dulc,TRUE) ;
		   if (dulc->Count < LCL_MAX)				/* Place into local cache */
		    { dulc->DimId[dulc->Count] = di->DimId ; dulc->Dirty[dulc->Count] = TRUE ;
		      dulc->PntNum[dulc->Count] = unique + 1 ;		/* Hold next available number */
		      dulc->Revision[dulc->Count] = 0 ; dulc->Count++ ;
		    } ;
		 } else
		 { unique = (dulc->PntNum[i]++) ; dulc->Dirty[i] = TRUE ;
		 } ;
		return(unique) ;
	 } ;
}

/*	v4dpi_DimUniqueToList - Returns list of all points in Dimension 	*/
/*	Call: point = v4dpi_DimUniqueToList( respnt , ctx , dimid , listdim , srcpt, dotdot )
	  where point is pointer to point, respnt is buffer to use for point,
		ctx is current context,
		dimid is dimension id in question,
		listdim is dimension of resulting list point (updated in respnt),
		srcpt is point where dimid comes from,
		dotdot is TRUE if evaluating dim.., FALSE otherwise		*/

struct V4DPI__Point *v4dpi_DimUniqueToList(respnt,ctx,dimid,listdim,srcpt,dotdot)
  struct V4DPI__Point *respnt ;
  struct V4C__Context *ctx ;
  int dimid,listdim ; LOGICAL dotdot ;
  P *srcpt ;
{ 
  struct V4DPI__DimInfo *di ;
  struct V4DPI__Point *ipt,v4pt ;
  struct V4L__ListPoint *lp ;
  struct V4L__ListCmpndRange *lcr ;
  struct V4AGG__AggDims *vad ;
  struct V4L__ListAggRef *lar ;
  struct V4DPI__DictList *vdl ;				/* For enumerating thru all dictionary entries */
  struct V4DPI__LittlePoint litpt ;
#define nestDimsMax 8
  static int nestDims[nestDimsMax], nestDimsCnt=0 ;	/* VEH060912 - kind of a kludge to prevent stack overflow on nested calls with same dimension */
  enum DictionaryEntries deval ;
  int i,rx,vx,begin,end ; static int warn = FALSE ;

	DIMINFO(di,ctx,dimid) ;
try_again:
	if (di == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIUnqNotDim") ;  return(NULL) ; } ;
	if (dotdot && (di->Flags & V4DPI_DimInfo_DotDotToList))
	 { INITISCT(&v4pt) ; NOISCTVCD(&v4pt) ; v4pt.Grouping = 2 ;		/* Construct [Dim:xxx UV4:DotDotToList] intersection */
	   ipt = ISCT1STPNT(&v4pt) ; dictPNTv(ipt,Dim_UV4,v4im_GetEnumToDictVal(ctx,deval=_DotDotToList,UNUSED)) ; v4pt.Bytes += ipt->Bytes ;
	   ADVPNT(ipt) ; dictPNTv(ipt,Dim_Dim,di->DimId) ; v4pt.Bytes += ipt->Bytes ;
	   if (di->ADPnt.Bytes != 0)				/* Tack on AD-Point if we got it */
	    { ADVPNT(ipt) ;
	      memcpy(ipt,&di->ADPnt,di->ADPnt.Bytes) ; v4pt.Grouping++ ; v4pt.Bytes += ipt->Bytes ;
	    } ;
/*	   Only perform this evaluation if we are NOT already performing it (i.e. don't want to nest because we recurse into oblivion) */
	   for(i=0;i<nestDimsMax;i++) { if (nestDims[i] == di->DimId) break ; } ;
	   if (i >= nestDimsMax)
	    { i = (nestDimsCnt % nestDimsMax) ; nestDimsCnt++ ;
	      nestDims[i] = di->DimId ;				/* Record dimension to prevent nesting */
	      ipt = v4dpi_IsctEval(respnt,&v4pt,ctx,V4DPI_EM_NoIsctFail,NULL,NULL) ;
	      nestDims[i] = 0 ;					/* Reset dimension slot */
	      if (ipt != NULL) { ipt->Dim = dimid ; return(ipt) ; } ;
	    } ;
	 } ;
	switch (di->UniqueOK)
	 { default:
		switch (di->PointType)
//tried below & it did not work for normal points - trying to get it to work with new ODBC stuff
//		switch (srcpt->PntType)				/* VEH070617 - Go off of point, not dimension (point type may not be same as dimension */
		 { default:
			if (UCnotempty(di->IsA))		/* If has an IsA then link and try again */
			 { i = v4dpi_DimGet(ctx,di->IsA,DIMREF_IRT) ; DIMINFO(di,ctx,i) ; goto try_again ; } ;
			v_Msg(ctx,ctx->ErrorMsgAux,"DPIUnqNoImpPts",di->DimId) ;
			return(NULL) ;
		   case V4DPI_PntType_Logical:
			INITLP(respnt,lp,listdim) lp->Dim = di->DimId ;
			lp->ListType = V4L_ListType_CmpndRange ; lp->PntType = di->PointType ;
			lcr = (struct V4L__ListCmpndRange *)&lp->Buffer[0] ; lcr->Entries = 0 ; lcr->Count = 0 ;
			lcr->Cmpnd[lcr->Count].Begin = FALSE ; lcr->Cmpnd[lcr->Count].End = TRUE ;
			lcr->Cmpnd[lcr->Count++].Increment = 1 ; lcr->Entries = 2 ;
			lcr->Bytes = (char *)&lcr->Cmpnd[lcr->Count] - (char *)lcr ;
			lp->Entries = lcr->Entries ; lp->Bytes = (char *)&lp->Buffer[ALIGN(lcr->Bytes)] - (char *)lp ;
			ENDLP(respnt,lp) return(respnt) ;
		   case V4DPI_PntType_Color:
			INITLP(respnt,lp,listdim) lp->Dim = di->DimId ;
			lp->ListType = V4L_ListType_CmpndRange ; lp->PntType = di->PointType ;
			lcr = (struct V4L__ListCmpndRange *)&lp->Buffer[0] ; lcr->Entries = 0 ; lcr->Count = 0 ;
			v_ColorRefRange(&begin,&end) ; lcr->Cmpnd[lcr->Count].Begin = begin ;
			lcr->Cmpnd[lcr->Count].End = end ;
			lcr->Cmpnd[lcr->Count].Increment = 1 ; lcr->Entries = end - begin + 1 ;
			lcr->Bytes = (char *)&lcr->Cmpnd[++lcr->Count] - (char *)lcr ;
			lp->Entries = lcr->Entries ; lp->Bytes = (char *)&lp->Buffer[ALIGN(lcr->Bytes)] - (char *)lp ;
			ENDLP(respnt,lp) return(respnt) ;
		   case V4DPI_PntType_Country:
			INITLP(respnt,lp,listdim)
			ipt = &v4pt ; ZPH(ipt) ; ipt->PntType = V4DPI_PntType_Country ; ipt->Dim = di->DimId ; ipt->Bytes = V4PS_Int ;
			for(i=0;i<gpi->ci->Count;i++)
			 { ipt->Value.IntVal = gpi->ci->Cntry[i].UNCode ;
			   v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,ipt,0) ;
			 } ;
			ENDLP(respnt,lp) return(respnt) ;
		   case V4DPI_PntType_XDict:
			INITLP(respnt,lp,listdim) lp->Dim = di->DimId ;
			lp->ListType = V4L_ListType_CmpndRange ; lp->PntType = di->PointType ;
			lcr = (struct V4L__ListCmpndRange *)&lp->Buffer[0] ; lcr->Entries = 0 ; lcr->Count = 0 ;
			v4dpi_RevXDictEntryGet(ctx,dimid,1) ;		/* Force dimension into cache */
			for(i=0;gpi->xdr!=NULL && i<V4DPI_XDictRTCacheMax;i++)
			 { if (gpi->xdr->Cache[i].DimId != di->DimId) continue ;
			   lcr->Cmpnd[lcr->Count].Begin = 1 ; lcr->Cmpnd[lcr->Count].End = gpi->xdr->Cache[i].LastPoint ;
			   lcr->Cmpnd[lcr->Count].Increment = 1 ; lcr->Entries = lcr->Cmpnd[lcr->Count].End ;
			   lcr->Bytes = (char *)&lcr->Cmpnd[++lcr->Count] - (char *)lcr ;
			   lp->Entries = lcr->Entries ; lp->Bytes = (char *)&lp->Buffer[ALIGN(lcr->Bytes)] - (char *)lp ;
			   ENDLP(respnt,lp) return(respnt) ;
			 } ;
			v_Msg(ctx,ctx->ErrorMsgAux,"DPIUnqNoExtDict",di->DimId) ; return(NULL) ;
		   case V4DPI_PntType_Tag:
			INITLP(respnt,lp,listdim) lp->Dim = di->DimId ;
			lp->ListType = V4L_ListType_CmpndRange ; lp->PntType = di->PointType ;
			lcr = (struct V4L__ListCmpndRange *)&lp->Buffer[0] ; lcr->Entries = 0 ; lcr->Count = 0 ;
			v4im_TagRange(&begin,&end) ; lcr->Cmpnd[lcr->Count].Begin = begin ;
			lcr->Cmpnd[lcr->Count].End = end ;
			lcr->Cmpnd[lcr->Count].Increment = 1 ; lcr->Entries = end - begin + 1 ;
			lcr->Bytes = (char *)&lcr->Cmpnd[++lcr->Count] - (char *)lcr ;
			lp->Entries = lcr->Entries ; lp->Bytes = (char *)&lp->Buffer[ALIGN(lcr->Bytes)] - (char *)lp ;
			ENDLP(respnt,lp) return(respnt) ;
		   case V4DPI_PntType_IntMod:
			INITLP(respnt,lp,listdim) lp->Dim = di->DimId ;
			lp->ListType = V4L_ListType_CmpndRange ; lp->PntType = di->PointType ;
			lcr = (struct V4L__ListCmpndRange *)&lp->Buffer[0] ; lcr->Entries = 0 ; lcr->Count = 0 ;
			v4im_IntModRange(&begin,&end) ; lcr->Cmpnd[lcr->Count].Begin = begin ;
			lcr->Cmpnd[lcr->Count].End = end ;
			lcr->Cmpnd[lcr->Count].Increment = 1 ; lcr->Entries = end - begin + 1 ;
			lcr->Bytes = (char *)&lcr->Cmpnd[++lcr->Count] - (char *)lcr ;
			lp->Entries = lcr->Entries ; lp->Bytes = (char *)&lp->Buffer[ALIGN(lcr->Bytes)] - (char *)lp ;
			ENDLP(respnt,lp) return(respnt) ;
		   case V4DPI_PntType_Dict:
			INITLP(respnt,lp,listdim)
			ipt = &v4pt ; ZPH(ipt) ; ipt->PntType = V4DPI_PntType_Char ; ipt->Dim = Dim_Alpha ;
			if (di->DimId == Dim_Dim)
			 { vdl = v4mm_AllocChunk(sizeof *vdl,FALSE) ; v4dpi_DictEntryEnum(ctx,UNUSED,vdl,V4IS_SubType_DimDict) ;
			   ipt = (P *)&litpt ; ZPH(ipt) ; ipt->Dim = di->DimId ;
			   ipt->PntType = V4DPI_PntType_Dict ; ipt->Bytes = V4PS_Int ;
			   for(i=0;i<vdl->Count && i<V4DPI_DictListMax;i++)
			    { ipt->Value.IntVal = vdl->Entry[i].DictId ;
			      DIMINFO(di,ctx,ipt->Value.IntVal) ;
#ifdef V4_BUILD_SECURITY
			      if (di == NULL ? TRUE : (di->rtFlags & V4DPI_rtDimInfo_Hidden)) continue ;
#endif
			      v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,ipt,0) ;
			    } ;
			   ENDLP(respnt,lp) v4mm_FreeChunk(vdl) ;
			   return(respnt) ;
			 } else if (di->DimId == Dim_NId)
			 { vdl = v4mm_AllocChunk(sizeof *vdl,FALSE) ; v4dpi_DictEntryEnum(ctx,UNUSED,vdl,V4IS_SubType_ExtDict) ;
			   ipt = (P *)&litpt ; ZPH(ipt) ; ipt->Dim = di->DimId ;
			   ipt->PntType = V4DPI_PntType_Dict ; ipt->Bytes = V4PS_Int ;
			   for(i=0;i<vdl->Count && i<V4DPI_DictListMax;i++)
			    { ipt->Value.IntVal = vdl->Entry[i].DictId ;
			      v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,ipt,0) ;
			    } ;
			   ENDLP(respnt,lp) v4mm_FreeChunk(vdl) ;
			   return(respnt) ;
			 } ;
			v_Msg(ctx,ctx->ErrorMsgAux,"DPIUnqNoPts",di->DimId) ;	return(NULL) ;
		 } ;
	   case V4DPI_DimInfo_UOkInt:
	   case V4DPI_DimInfo_UOkNew:
		ipt = respnt ; ZPH(ipt) ;
		lp = ALIGNLP(&ipt->Value) ; memset(lp,0,V4L_ListPointHdr_Bytes) ;	/* Link up list structure */
		lp->ListType = V4L_ListType_CmpndRange ; lp->PntType = di->PointType ; lp->Dim = di->DimId ;
		lcr = (struct V4L__ListCmpndRange *)&lp->Buffer[0] ; lcr->Entries = 0 ; lcr->Count = 0 ;
		ipt->PntType = V4DPI_PntType_List ; ipt->Dim = listdim ;
		i = v4dpi_DimUnqToCache(ctx,di) ;			/* Find the point in cache */
		if (i == UNUSED) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIUnqNoPts",di->DimId) ; return(NULL) ; } ;
		if (di->UniqueOK == V4DPI_DimInfo_UOkNew) { begin = dulc->FirstNum[i] ; }
		 else { begin = (di->DimId << 20) + 1 ; } ;
		end = dulc->PntNum[i] ;
		lcr->Cmpnd[lcr->Count].Begin = begin ; lcr->Cmpnd[lcr->Count].End = end - 1 ;
		lcr->Cmpnd[lcr->Count++].Increment = 1 ;
		lcr->Entries += end - begin ;
		if (lcr->Entries <= 0) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIUnqNoPts",di->DimId) ; return(NULL) ; } ;
		lcr->Bytes = (char *)&lcr->Cmpnd[lcr->Count] - (char *)lcr ;
		lp->Entries = lcr->Entries ; lp->Bytes = (char *)&lp->Buffer[ALIGN(lcr->Bytes)] - (char *)lp ;
		ipt->Bytes = (char *)&ipt->Value.AlphaVal[lp->Bytes] - (char *)ipt ;
		if (ipt->Bytes < V4DPI_PointHdr_Bytes + ((char *)&lp->Buffer[0] - (char *)lp))
		 ipt->Bytes = V4DPI_PointHdr_Bytes + ((char *)&lp->Buffer[0] - (char *)lp) ;	/* Make sure min size */
		return(ipt) ;
	   case V4DPI_DimInfo_UOkPoint:
#ifdef BEFOREMULTIAREA
		lowrx = gpi->LowHNum ; hghrx = gpi->HighHNum ;		/* Look thru all available areas */
		for(rx=lowrx;rx<=hghrx;rx++)
		 { if (gpi->RelH[rx].aid == UNUSED) continue ;
		   dul.kp.fld.KeyType = V4IS_KeyType_V4 ; dul.kp.fld.KeyMode = V4IS_KeyMode_Int ;
		   dul.kp.fld.AuxVal = V4IS_SubType_DUList ;
		   dul.kp.fld.Bytes = sizeof dul.kp + sizeof dul.DimId ; dul.DimId = dimid ;	/* Set up key field */
		   if (v4is_PositionKey(gpi->RelH[rx].aid,&dul,&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey) continue ;
		   dulx = (struct V4DPI__DUList *)v4dpi_GetBindBuf(ctx,gpi->RelH[rx].aid,ikde,FALSE,NULL) ;
		   ipt = respnt ; ZPH(ipt) ; ipt->PntType = V4DPI_PntType_List ; ipt->Dim = listdim ;
		   memcpy(&ipt->Value,&dulx->lp,sizeof dulx->lp) ;
		   ipt->Bytes = ((char *)&ipt->Value+sizeof(*dulx)) - (char *)ipt ;
		   return(ipt) ;
		 } ;
		v_Msg(ctx,ctx->ErrorMsgAux,"DPIUnqNoPts",di->DimId) ; return(NULL) ;
#endif

		i = v4dpi_DimUnqPntToCache(ctx,di) ;
		GRABMTLOCK(dupLock) ; 
		if (i == UNUSED ? TRUE : duplc->Entry[i].dul->lp.Entries == 0)
		 { FREEMTLOCK(dupLock) ; v_Msg(ctx,ctx->ErrorMsgAux,"DPIUnqNoPts",di->DimId) ; return(NULL) ; } ;
		ZPH(respnt) ; respnt->PntType = V4DPI_PntType_List ; respnt->Dim = listdim ;
		memcpy(&respnt->Value,&duplc->Entry[i].dul->lp,sizeof duplc->Entry[i].dul->lp) ;
		respnt->Bytes = ((char *)&respnt->Value+sizeof duplc->Entry[i].dul->lp) - (char *)respnt ;
		FREEMTLOCK(dupLock) ; return(respnt) ;
		



	   case V4DPI_DimInfo_UOkAgg:
		v4im_AggPutBlocked(ctx,NULL,DFLTAGGAREA) ;			/* Flush out all partial BFAggs */
		ipt = respnt ; ZPH(ipt) ;
		lp = ALIGNLP(&ipt->Value) ; memset(lp,0,V4L_ListPointHdr_Bytes) ;	/* Link up list structure */
		lp->ListType = V4L_ListType_AggRef ; lp->PntType = di->PointType ; lp->Dim = di->DimId ;
		lar = (struct V4L__ListAggRef *)&lp->Buffer[0] ;
		lar->Dim = dimid ; lar->Count = 0 ;
		ipt->PntType = V4DPI_PntType_List ; ipt->Dim = listdim ;
		for(rx=0;rx<gpi->AreaAggCount;rx++)			/* Loop thru all loaded aggs */
		 { if (gpi->AreaAgg[rx].pcb == NULL) continue ;
		   vad = gpi->AreaAgg[rx].vad ; if (vad == NULL) continue ;
		   for(vx=0;vx<vad->Count;vx++)
		    { if (vad->Dims[vx] != dimid) continue ;		/* Look for wanted dimension */
		      if ((char *)&lar->Agg[lar->Count+1] - (char *)lp > V4DPI_AlphaVal_Max - 20)
		       { v_Msg(ctx,ctx->ErrorMsgAux,"DPIUnqTooBig",lar->Count+1) ; return(NULL) ; } ;
		      lar->AreaAggIndex[lar->Count] = rx ;
/*		      If begin point greater than end point then don't count this (VEH050906 - reset of aggregate dimension) */
		      if (vad->Entry[vx].BeginPt > vad->Entry[vx].EndPt) continue ;
		      lar->Agg[lar->Count].BeginPt = vad->Entry[vx].BeginPt ; lar->Agg[lar->Count].EndPt = vad->Entry[vx].EndPt ;
		      lp->Entries += (lar->Agg[lar->Count].EndPt - lar->Agg[lar->Count].BeginPt + 1) ;
//printf("--lazy construct entries=%d, vx=%d/%d, areaindex=%d, begin=%d, end=%d\n",lp->Entries,vx,vad->Count,rx,lar->Agg[lar->Count].BeginPt,lar->Agg[lar->Count].EndPt) ;
		      lar->Count++ ;
		      break ;
		    } ;
		 } ;
		if (lar->Count == 0)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"DPIUnqNoPts",di->DimId) ; return(NULL) ; } ;
		lp->Bytes = (char *)&lar->Agg[lar->Count] - (char *)lp ;
		ipt->Bytes = (char *)&ipt->Value.AlphaVal[lp->Bytes] - (char *)ipt ;
		if (ipt->Bytes < V4DPI_PointHdr_Bytes + ((char *)&lp->Buffer[0] - (char *)lp))
		 ipt->Bytes = V4DPI_PointHdr_Bytes + ((char *)&lp->Buffer[0] - (char *)lp) ;	/* Make sure min size */
		return(ipt) ;
	 } ;
	return(NULL) ;
}

/*	v4dpi_DimUniqueNumPoints - Returns number of Points Associated with Dimension 	*/
/*	Call: number = v4dpi_DimUniqueNumPoints( ctx , dimid )
	  where number is number of points, UNUSED if cannot determine (ctx->ErrorMsgAux updated),
		ctx is current context,
		dimid is dimension id in question					*/

int v4dpi_DimUniqueNumPoints(ctx,dimid)
  struct V4C__Context *ctx ;
  int dimid ;
{ 
  struct V4DPI__DimInfo *di ;
  struct V4L__ListPoint *lp ;
  struct V4AGG__AggDims *vad ;
  struct V4DPI__DictList *vdl ;				/* For enumerating thru all dictionary entries */
  struct V4DPI__Point *tpt,spnt,qpnt ;
  int i,rx,vx,begin,end,num ; static int warn = FALSE ;
//  int trace=0 ;	/* To make SizeOfList macro work */

	DIMINFO(di,ctx,dimid) ;
try_again:
	if (di == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIUnqNotDim") ;  return(UNUSED) ; } ;
	switch (di->UniqueOK)
	 { default:
	   switch (di->PointType)
	    { default:
		if (UCnotempty(di->IsA))		/* If has an IsA then link and try again */
		 { i = v4dpi_DimGet(ctx,di->IsA,DIMREF_IRT) ; DIMINFO(di,ctx,i) ; goto try_again ; } ;
		tpt = v4dpi_DimUniqueToList(&spnt,ctx,dimid,Dim_List,NULL,TRUE) ;
		if (tpt != NULL)			/* Try to convert to list (maybe thru DotDotToList) */
		 { lp = (struct V4L__ListPoint *)v4im_VerifyList(&qpnt,ctx,tpt,0) ;
		   num = SIZEofLIST(lp) ;
		   if (num > 0) return(num) ;
		 } ;
		v_Msg(ctx,ctx->ErrorMsgAux,"DPIUnqNoImpPts",di->DimId) ; return(UNUSED) ;
	      case V4DPI_PntType_Logical:
		return(2) ;
	      case V4DPI_PntType_Color:
		v_ColorRefRange(&begin,&end) ; return(end-begin+1) ;
	      case V4DPI_PntType_Country:
		return(gpi->ci->Count) ;
	      case V4DPI_PntType_XDict:
		v4dpi_RevXDictEntryGet(ctx,dimid,1) ;		/* Force dimension into cache */
		for(i=0;i<V4DPI_XDictRTCacheMax;i++)
		 { if (gpi->xdr->Cache[i].DimId != di->DimId) continue ;
		   return(gpi->xdr->Cache[i].LastPoint) ;
		 } ;
		v_Msg(ctx,ctx->ErrorMsgAux,"DPIUnqNoExtDict",di->DimId) ;
		return(UNUSED) ;
	     case V4DPI_PntType_Tag:
		v4im_TagRange(&begin,&end) ; return(end-begin+1) ;
	     case V4DPI_PntType_IntMod:
		v4im_IntModRange(&begin,&end) ; return(end-begin+1) ;
	     case V4DPI_PntType_Dict:
		 if (di->DimId == Dim_Tag)
		 { for(num=0,i=1,end=10;end>0;i++)
		    { if (v4im_TagName(i)[1] == UClit('?')) { end-- ; continue ; } ;
		      num++ ;
		    } ; return(num) ;
		 } else if (di->DimId == Dim_Dim)
		 { vdl = v4mm_AllocChunk(sizeof *vdl,FALSE) ; v4dpi_DictEntryEnum(ctx,UNUSED,vdl,V4IS_SubType_DimDict) ;
		   num = vdl->Count ; v4mm_FreeChunk(vdl) ;
		   return(num) ;
		 } else if (di->DimId == Dim_NId)
		 { vdl = v4mm_AllocChunk(sizeof *vdl,FALSE) ; v4dpi_DictEntryEnum(ctx,UNUSED,vdl,V4IS_SubType_ExtDict) ;
		   num = vdl->Count ; v4mm_FreeChunk(vdl) ;
		   return(num) ;
		 } ;
		v_Msg(ctx,ctx->ErrorMsgAux,"DPIUnqNoPts",di->DimId) ;
		return(UNUSED) ;
	    } ;
	   case V4DPI_DimInfo_UOkInt:
	   case V4DPI_DimInfo_UOkNew:
		i = v4dpi_DimUnqToCache(ctx,di) ;
		if (i == UNUSED) return(0) ;
		if (di->UniqueOK == V4DPI_DimInfo_UOkNew) { begin = dulc->FirstNum[i] ; }
		 else { begin = (di->DimId << 20) + 1 ; } ;
		end = dulc->PntNum[i] ;
		return(end - begin) ;
	   case V4DPI_DimInfo_UOkPoint:
#ifdef BEFOREMULTIAREA
		lowrx = gpi->LowHNum ; hghrx = gpi->HighHNum ;		/* Look thru all available areas */
		for(rx=lowrx;rx<=hghrx;rx++)
		 { if (gpi->RelH[rx].aid == UNUSED) continue ;
		   dul.kp.fld.KeyType = V4IS_KeyType_V4 ; dul.kp.fld.KeyMode = V4IS_KeyMode_Int ;
		   dul.kp.fld.AuxVal = V4IS_SubType_DUList ;
		   dul.kp.fld.Bytes = sizeof dul.kp + sizeof dul.DimId ; dul.DimId = dimid ;	/* Set up key field */
		   if (v4is_PositionKey(gpi->RelH[rx].aid,&dul,&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey) continue ;
		   dulx = (struct V4DPI__DUList *)v4dpi_GetBindBuf(ctx,gpi->RelH[rx].aid,ikde,FALSE,NULL) ;
		   return(dulx->lp.Entries) ;
		 } ;
		v_Msg(ctx,ctx->ErrorMsgAux,"DPIUnqNoImpPts",di->DimId) ; return(UNUSED) ;
#endif
		GRABMTLOCK(dupLock) ; i = v4dpi_DimUnqPntToCache(ctx,di) ;
		if (i == UNUSED ? TRUE : duplc->Entry[i].dul->lp.Entries == 0)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"DPIUnqNoImpPts",di->DimId) ; FREEMTLOCK(dupLock) ; return(UNUSED) ; } ;
		FREEMTLOCK(dupLock) ; return(duplc->Entry[i].dul->lp.Entries) ;
	   case V4DPI_DimInfo_UOkAgg:
		v4im_AggPutBlocked(ctx,NULL,DFLTAGGAREA) ;			/* Flush out all partial BFAggs */
		for(num=0,rx=0;rx<gpi->AreaAggCount;rx++)			/* Loop thru all loaded aggs */
		 { if (gpi->AreaAgg[rx].pcb == NULL) continue ;
		   vad = gpi->AreaAgg[rx].vad ; if (vad == NULL) continue ;
		   for(vx=0;vx<vad->Count;vx++)
		    { if (vad->Dims[vx] != dimid) continue ;		/* Look for wanted dimension */
		      num += (vad->Entry[vx].EndPt - vad->Entry[vx].BeginPt) + 1 ;
		    } ;
		 } ;
		return(num) ;
	 } ;
	return(UNUSED) ;
}

/*	E X T E R N A L   D I C T I O N A R Y   M O D U L E S				*/

/*	v4dpi_FlushXDict - Flush out any cached entries, close area */
/*	Call: v4dpi_FlushXDict( ctx )							*/

void v4dpi_FlushXDict(ctx)
  struct V4C__Context *ctx ;
{ struct V4DPI_XDictDimPoints xddp ;
  int i ;

	if (gpi->xdr == NULL) return ;			/* Nothing to do */
/*	Note - if External dictionary locking is enabled then nothing should be flagged here */
	for(i=0;i<V4DPI_XDictRTCacheMax;i++)
	 { if ((gpi->xdr->Cache[i].Flags & XDRDIRTY) == 0) continue ;
	   xddp.kp.fld.AuxVal = V4IS_SubType_XDictDimPoints ; xddp.kp.fld.KeyType = V4IS_KeyType_V4 ; xddp.kp.fld.KeyMode = V4IS_KeyMode_Alpha ;
	   xddp.DimXId = gpi->xdr->Cache[i].DimXId ; xddp.kp.fld.Bytes = (char *)&xddp.LastPoint - (char *)&xddp ;
	   xddp.LastPoint = gpi->xdr->Cache[i].LastPoint ;
#ifdef XDICTMU
	   gpi->xdr->pcb->PutMode = V4IS_PCB_GP_Write | V4IS_PCB_NoError ;
	   gpi->xdr->pcb->KeyPtr = (struct V4IS__Key *)&xddp ;
	   gpi->xdr->pcb->PutBufPtr = (BYTE *)&xddp ;
	   gpi->xdr->pcb->PutBufLen = sizeof xddp  ;
	   gpi->xdr->pcb->DataMode = V4IS_PCB_DataMode_Index ;
	   v4is_Put(gpi->xdr->pcb,gpi->ctx->ErrorMsgAux) ;
#else
	   if (gpi->xdr->Cache[i].Flags & XDRNEW)
	    { v4is_Insert(gpi->xdr->aid,(struct V4IS__Key *)&xddp,&xddp,sizeof xddp,V4IS_PCB_DataMode_Index,0,0,FALSE,FALSE,0,NULL) ;
	    } else
	    { int r ;
	      r = v4is_PositionKey(gpi->xdr->aid,(struct V4IS__Key *)&xddp,NULL,0,V4IS_PosDCKL) ;
	      v4is_Replace(gpi->xdr->aid,(struct V4IS__Key *)&xddp,&xddp,&xddp,sizeof xddp,V4IS_PCB_DataMode_Index,V4IS_DataCmp_None,0,0) ;
	    } ;
#endif
	 } ;
	v4is_Close(gpi->xdr->pcb) ; v4mm_FreeChunk(gpi->xdr->pcb) ; gpi->xdr->pcb = NULL ;
	if (gpi->xdr->xdrc != NULL) { v4mm_FreeChunk(gpi->xdr->xdrc) ; gpi->xdr->xdrc = NULL ; } ;
	if (gpi->xdr->xdc != NULL) { v4mm_FreeChunk(gpi->xdr->xdc) ; gpi->xdr->xdc = NULL ; } ;
	v4mm_FreeChunk(gpi->xdr) ; gpi->xdr = NULL ;
}


/*	v4dpi_XDictDimGet - Returns XDict Entry for Dimension Name	*/
/*	Note: This plays a game to force Dim:Dim into cache with dimid=-1 - not real cool & burned me (VEH030617)
	until I figured out problem - must plug gpi->xdr->Cache[cx].DimId with dimid before calling this module or else
	run risk of cx here being same as cx in calling module	*/

int v4dpi_XDictDimGet(ctx,entry)
  struct V4C__Context *ctx ;
  UCCHAR *entry ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4DPI__DimInfo di ;
  struct V4DPI_XDictDimPoints xddp ;
  int cx,dimid ; INDEX areax ;

	dimid = -1 ;					/* Look for (or make if necessary) entry for dimensions */
	for(cx=(dimid & 0x7fffffff)%V4DPI_XDictRTCacheMax;;cx=(cx+1)%V4DPI_XDictRTCacheMax)
	 { if (gpi->xdr->Cache[cx].DimId == dimid) break ;
	   if (gpi->xdr->Cache[cx].DimId != 0) continue ;
	   if (gpi->xdr->Count != 0)
	    v4_error(0,0,"V4DPI","XDictGetDim","MALFORM","External dictionary cache is malformed (%d %s)",gpi->xdr->Count,entry) ;
	   GRABMTLOCK(gpi->xdr->mtLock) ;
	   gpi->xdr->Count++ ;
	   gpi->xdr->Cache[cx].DimId = dimid ; gpi->xdr->Cache[cx].Flags = 0 ; gpi->xdr->Cache[cx].LastPoint = 0 ;
	   gpi->xdr->Cache[cx].DimXId = -1 ;
	   xddp.kp.fld.AuxVal = V4IS_SubType_XDictDimPoints ; xddp.kp.fld.KeyType = V4IS_KeyType_V4 ; xddp.kp.fld.KeyMode = V4IS_KeyMode_Alpha ;
	   xddp.DimXId = gpi->xdr->Cache[cx].DimXId ; xddp.kp.fld.Bytes = (char *)&xddp.LastPoint - (char *)&xddp ;
	   FINDAREAINPROCESS(areax,gpi->xdr->aid) ;
#ifdef TRACEGRAB
printf("xdictDimGet %d\n",mmm->Areas[areax].areaLock) ;
#endif
	   GRABAREAMTLOCK(areax) ;
#ifdef XDICTMU
	   gpi->xdr->pcb->GetMode = (V4IS_PCB_GP_Keyed | V4IS_PCB_NoLock | V4IS_PCB_IgnoreLock | V4IS_PCB_NoError | V4IS_PCB_LockWait) ;
	   gpi->xdr->pcb->KeyPtr = (struct V4IS__Key *)&xddp ;
	   gpi->xdr->pcb->GetBufPtr = (BYTE *)&xddp ;
	   gpi->xdr->pcb->GetBufLen = sizeof xddp ;
	   v4is_Get(gpi->xdr->pcb) ;
	   if (gpi->xdr->pcb->GetLen == UNUSED)
	    { gpi->xdr->Cache[cx].Flags |= XDRNEW ;
	    } else
	    { gpi->xdr->Cache[cx].LastPoint = xddp.LastPoint ; 
	    } ;
#else
	   { struct V4DPI_XDictDimPoints *xddp1 ;
	     if (v4is_PositionKey(gpi->xdr->aid,(struct V4IS__Key *)&xddp,(struct V4IS__Key **)&xddp1,0,V4IS_PosDCKL) == V4RES_PosKey)
	      { gpi->xdr->Cache[cx].LastPoint = xddp1->LastPoint ; } else { gpi->xdr->Cache[cx].Flags |= XDRNEW ; } ;
	   }
#endif
	   FREEAREAMTLOCK(areax) ;
	   FREEMTLOCK(gpi->xdr->mtLock) ;
	   break ;
	 } ;
	memset(&di,0,sizeof di) ; UCstrcpy(di.DimName,UClit("DIM")) ; di.DimId = -1 ;
	return(v4dpi_XDictEntryGet(ctx,entry,&di,0)) ;
}

/*	v4dpi_XDictEntryGet - Converts string to internal dictionary entry number */
/*	Call: dictnum = v4dpi_XDictEntryGet( ctx, entry , di , flags )
	  where dictnum is number (0 if not found),
		ctx is pointer to current context,
		entry is dictionary entry string,
		di is pointer to dimension info,
		flags are various lookup flags- V4DPI_XDict_xxx */

int v4dpi_XDictEntryGet(ctx,entry,di,flags)
  struct V4C__Context *ctx ;
  UCCHAR *entry ;
  struct V4DPI__DimInfo *di ;
  int flags ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4DPI__XDictEntry xde ;
  struct V4DPI_XDictDimPoints xddp ;
  int i,numeric,elen,first,cx,dimid,hash,hx,casesensitive ; int *dictid ;
  UCCHAR *ep,*bp,*lastnb ; INDEX areax ; LENMAX bytesInKey ;

	if (gpi->xdr == NULL) return(0) ;
	numeric = di->Flags & V4DPI_DimInfo_Normalize ;
	dimid = di->DimId ;
	for(cx=(dimid & 0x7fffffff)%V4DPI_XDictRTCacheMax;;cx=(cx+1)%V4DPI_XDictRTCacheMax)
	 { if (gpi->xdr->Cache[cx].DimId == dimid) break ;
	   if (gpi->xdr->Cache[cx].DimId != 0) continue ;
	   if (gpi->xdr->Count >= V4DPI_XDictRTCacheMax - 10)
	    v4_error(0,0,"V4DPI","XDictGet","TOOMNYXDICT","Exceeded max number of concurrent XDict entries") ;
	   gpi->xdr->Cache[cx].DimId = dimid ; gpi->xdr->Cache[cx].Flags = 0 ; gpi->xdr->Cache[cx].LastPoint = 0 ;
	   gpi->xdr->Cache[cx].DimXId = v4dpi_XDictDimGet(ctx,di->DimName) ;
	   if (gpi->xdr->Cache[cx].DimXId < 1)			/* Don't have this dimension in the dictionary */
	    { gpi->xdr->Cache[cx].DimId = 0 ; return(0) ; } ;
	   GRABMTLOCK(gpi->xdr->mtLock) ;
	   gpi->xdr->Count++ ;
	   xddp.kp.fld.AuxVal = V4IS_SubType_XDictDimPoints ; xddp.kp.fld.KeyType = V4IS_KeyType_V4 ; xddp.kp.fld.KeyMode = V4IS_KeyMode_Alpha ;
	   xddp.DimXId = gpi->xdr->Cache[cx].DimXId ; xddp.kp.fld.Bytes = (char *)&xddp.LastPoint - (char *)&xddp ;
	   FINDAREAINPROCESS(areax,gpi->xdr->aid) ;
#ifdef TRACEGRAB
printf("xdictentryget %d\n",mmm->Areas[areax].areaLock) ;
#endif
	   GRABAREAMTLOCK(areax) ;
#ifdef XDICTMU
	   gpi->xdr->pcb->GetMode = (V4IS_PCB_GP_Keyed | V4IS_PCB_NoLock | V4IS_PCB_IgnoreLock | V4IS_PCB_NoError | V4IS_PCB_LockWait) ;
	   gpi->xdr->pcb->KeyPtr = (struct V4IS__Key *)&xddp ;
	   gpi->xdr->pcb->GetBufPtr = (BYTE *)&xddp ;
	   gpi->xdr->pcb->GetBufLen = sizeof xddp ;
	   v4is_Get(gpi->xdr->pcb) ;
	   if (gpi->xdr->pcb->GetLen == UNUSED)
	    { gpi->xdr->Cache[cx].Flags |= XDRNEW ;
	    } else
	    { gpi->xdr->Cache[cx].LastPoint = xddp.LastPoint ; 
	    } ;
#else
	   if (v4is_PositionKey(gpi->xdr->aid,(struct V4IS__Key *)&xddp,(struct V4IS__Key **)&xddp1,0,V4IS_PosDCKL) == V4RES_PosKey)
	    { gpi->xdr->Cache[cx].LastPoint = xddp1->LastPoint ; } else { gpi->xdr->Cache[cx].Flags |= XDRNEW ; } ;
#endif
	   FREEAREAMTLOCK(areax) ;
	   FREEMTLOCK(gpi->xdr->mtLock) ;
	   break ;
	 } ;
/*	Construct key & see if we got in specified area */
	xde.kp.fld.AuxVal = V4IS_SubType_IntXDict ; xde.kp.fld.KeyType = V4IS_KeyType_V4 ; xde.kp.fld.KeyMode = V4IS_KeyMode_Alpha ;
	xde.DimXId = ((flags & V4DPI_XDict_DimDim) != 0 ? UNUSED : gpi->xdr->Cache[cx].DimXId) ;
/*	Now move entry (convert to upper case) as the key */
	if (*entry == UCEOS) entry = DICTENTRYMT ;		/* Check for empty string & replace with special character */
	if (numeric) { for(;*entry==UClit('0');) { entry++ ; } ; if (*entry==UCEOS) entry = UClit("0") ; } ;
	casesensitive = di->Flags & V4DPI_DimInfo_CaseSensitive ;
	hash = di->DimId << 15 ;			/* Init hash value */
	for(bp=xde.EntryVal+1,first=TRUE,i=0,ep=entry,lastnb=bp;i<V4DPI_XDictEntryVal_Max && *ep != UCEOS;i++,ep++) /* Normalize entry & copy into xde */
	 { if (*ep == ' ')
	    { if (first) continue ;
	      *bp = (casesensitive ? *ep : UCTOUPPER(*ep)) ; hash += (*bp * (i+1)) ; bp++ ;
	    } else { first = FALSE ; lastnb = bp ; *bp = (casesensitive ? *ep : UCTOUPPER(*ep)) ; hash += (*bp * (i+1)) ; bp++ ; } ;
	 } ;
	lastnb++ ; bytesInKey = ALIGN((char *)lastnb - (char *)&xde) ;
	*(lastnb++) = UCEOS ; *(lastnb++) = UCEOS ; *(lastnb++) = UCEOS ;
	elen = UCstrlen(&xde.EntryVal[1]) ; xde.EntryVal[0] = elen ;
	gpi->XDictLookups++ ;
	if (gpi->xdr->xdc != NULL)			/* Do we have a cache? */
	 { hx = (hash & 0x7fffffff) % gpi->xdr->xdc->HashDiv ;
	   if (gpi->xdr->xdc->Cache[hx].DimId == di->DimId && UCstrcmp(gpi->xdr->xdc->Cache[hx].Name,xde.EntryVal) == 0)
	    { gpi->XDictCacheHits++ ; return(gpi->xdr->xdc->Cache[hx].DictId) ; } ;
	 } ;
	if (*ep != UCEOS)				/* Entry too long? */
	 { v_Msg(ctx,UCTBUF2,"*DPIDictTrunc",entry,&xde.EntryVal[1]) ; vout_UCText(VOUT_Warn,0,UCTBUF2) ; } ;
	xde.kp.fld.Bytes = bytesInKey ;			/* Store size of key */
/*	See if got the entry */
	GRABMTLOCK(gpi->xdr->mtLock) ;
	FINDAREAINPROCESS(areax,gpi->xdr->aid) ;
#ifdef TRACEGRAB
printf("xdictentryget 2 %d\n",mmm->Areas[areax].areaLock) ;
#endif
	GRABAREAMTLOCK(areax) ;
#ifdef XDICTMU
	gpi->xdr->pcb->GetMode = (V4IS_PCB_GP_Keyed | V4IS_PCB_NoLock | V4IS_PCB_IgnoreLock | V4IS_PCB_NoError | V4IS_PCB_LockWait) ;
	gpi->xdr->pcb->KeyPtr = (struct V4IS__Key *)&xde ;
	gpi->xdr->pcb->GetBufPtr = (BYTE *)&xde ;
	gpi->xdr->pcb->GetBufLen = sizeof xde ;
	v4is_Get(gpi->xdr->pcb) ;
	if (gpi->xdr->pcb->GetLen == UNUSED)
	 { FREEAREAMTLOCK(areax) ; FREEMTLOCK(gpi->xdr->mtLock) ;
	   if ((di->Flags & V4DPI_DimInfo_NoAutoCreate) || (flags & V4DPI_XDict_MustExist)) return(0) ;	/* Not allowed to auto-create */
	   return(v4dpi_XDictEntryPut(gpi->xdr->aid,entry,di,((flags & V4DPI_XDict_DimDim)!=0 ? V4DPI_XDict_DimDim : 0))) ;
	 } ;
	dictid = (int *)((char *)&xde + bytesInKey) ;	/* Integer align pointer to dictionary entry */
#else
	if (v4is_PositionKey(gpi->xdr->aid,(struct V4IS__Key *)&xde,(struct V4IS__Key **)&xde1,0,V4IS_PosDCKL) != V4RES_PosKey)
	 { FREEAREAMTLOCK(areax) ; FREEMTLOCK(gpi->xdr->mtLock) ;
	   if ((di->Flags & V4DPI_DimInfo_NoAutoCreate) || (flags & V4DPI_XDict_MustExist)) return(0) ;	/* Not allowed to auto-create */
	   return(v4dpi_XDictEntryPut(gpi->xdr->aid,entry,di,((flags & V4DPI_XDict_DimDim)!=0 ? V4DPI_XDict_DimDim : 0))) ;
	 } ;
	dictid = (int *)((char *)xde1 + bytesInKey) ;	/* Integer align pointer to dictionary entry */
#endif
	if (++gpi->xdr->LookupCount > V4DPI_XDictCacheTrigger)
	 { if (gpi->xdr->xdc == NULL)
	    { gpi->xdr->xdc = (struct V4DPI__XDictCache *)v4mm_AllocChunk(sizeof *gpi->xdr->xdc,TRUE) ;
	      gpi->xdr->xdc->HashDiv = V4DPI_XDictCacheMax ;
	    } ;
	   hx = (hash & 0x7fffffff) % gpi->xdr->xdc->HashDiv ;
	   if (gpi->xdr->xdc->Cache[hx].DimId != 0) gpi->xdr->xdc->Cache[hx].Thrash++ ;
	   gpi->xdr->xdc->Cache[hx].DimId = di->DimId ; gpi->xdr->xdc->Cache[hx].DictId = *dictid ;
	   UCstrcpy(gpi->xdr->xdc->Cache[hx].Name,xde.EntryVal) ;
	   if (gpi->xdr->xdc->Cache[hx].Thrash > 100)
	    { /* printf("Flipping XDict cache factor - too much thrashing! (hx=%d, %s:%s)\n",hx,di->DimName,entry) ; */
	      gpi->xdr->xdc->HashDiv = (gpi->xdr->xdc->HashDiv == V4DPI_XDictCacheMax ? V4DPI_XDictCacheMaxThrash : V4DPI_XDictCacheMax) ; 
	      for(i=0;i<V4DPI_XDictCacheMax;i++) { gpi->xdr->xdc->Cache[i].Thrash = 0 ; } ;
	    } ;
	 } ;
	FREEAREAMTLOCK(areax) ;
	FREEMTLOCK(gpi->xdr->mtLock) ; return(*dictid) ;
}

/*	v4dpi_XDictEntryPut - Adds new string to internal dictionary, returns entry number	*/
/*	Call: dictnum = v4dpi_XDictEntryPut( aid , entry , di, 0 )
	  where dictnum is new number (NULLID if already exists),
		aid is area id,
		entry is string,
		di is pointer to V4DPI__DimInfo structure,
		flags are optional flags - V4DPI_XDict_xxx					*/

int v4dpi_XDictEntryPut(aid,entry,di,flags)
  int aid ;
  UCCHAR *entry ;
  struct V4DPI__DimInfo *di ;
  int flags ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4C__Context *ctx ;
  struct V4DPI__XDictEntry xde ;
  struct V4DPI__XDictRuntime *xdr ;
  struct V4DPI__RevXDictEntry xrde ;
  struct V4DPI_XDictDimPoints xddp ;
  int i,cx,elen,first,numeric,dimid,casesensitive ; int *dictid ;
  UCCHAR *bp, *ep, *lastnb, emsg[256] ; INDEX areax ; LENMAX bytesInKey ;

	xdr = gpi->xdr ;
	if (xdr == NULL) return(0) ;
	if ((xdr->pcb->AccessMode & (V4IS_PCB_AM_Insert | V4IS_PCB_AM_Update)) == 0)
	 return(0) ;					/* External Dictionary not open for updates */
	FINDAREAINPROCESS(areax,xdr->aid) ;
#ifdef TRACEGRAB
printf("xdictentryput %d\n",mmm->Areas[areax].areaLock) ;
#endif
	ctx = gpi->ctx ;
	dimid = di->DimId ;
	for(cx=(dimid & 0x7fffffff)%V4DPI_XDictRTCacheMax;;cx=(cx+1)%V4DPI_XDictRTCacheMax)
	 { 
/*	   We have empty slot (where this dim should go) or we have a cache hit */
	   if (xdr->Cache[cx].DimId == dimid)
	    { 
#ifdef XDICTMU
	      if (gpi->xdr->pcb->LockMode == 0) break ;		/* Only accept 'hit' if we are not locking, otherwise have to re-get (and lock) record to see if any changes */
#else
	      break ;						/* Found the dimension in cache, use the cache value */
#endif
	    } else
	    { if (xdr->Cache[cx].DimId != 0) continue ;
	    } ;
	   if (xdr->Count >= V4DPI_XDictRTCacheMax - 10)
	    v4_error(0,0,"V4DPI","XDictGet","TOOMNYXDICT","Exceeded max number of concurrent XDict entries") ;
	   xdr->Cache[cx].DimId = dimid ; xdr->Cache[cx].Flags = 0 ; xdr->Cache[cx].LastPoint = 0 ;
	   xdr->Cache[cx].DimXId = v4dpi_XDictDimGet(gpi->ctx,di->DimName) ;
	   if (xdr->Cache[cx].DimXId < 1)			/* Don't have this dimension in the dictionary */
	    { xdr->Cache[cx].DimId = 0 ; FREEAREAMTLOCK(areax) ; return(0) ; } ;
	   if (xdr->Cache[cx].DimId == 0) xdr->Count++ ;	/* Only update the count if its the first time */
	   xddp.kp.fld.AuxVal = V4IS_SubType_XDictDimPoints ; xddp.kp.fld.KeyType = V4IS_KeyType_V4 ; xddp.kp.fld.KeyMode = V4IS_KeyMode_Alpha ;
	   xddp.DimXId = xdr->Cache[cx].DimXId ; xddp.kp.fld.Bytes = (char *)&xddp.LastPoint - (char *)&xddp ;
	   GRABAREAMTLOCK(areax) ;
#ifdef XDICTMU
	   gpi->xdr->pcb->GetMode = (V4IS_PCB_GP_Keyed | V4IS_PCB_NoError | V4IS_PCB_LockWait) ;
	   gpi->xdr->pcb->KeyPtr = (struct V4IS__Key *)&xddp ;
	   gpi->xdr->pcb->GetBufPtr = (BYTE *)&xddp ;
	   gpi->xdr->pcb->GetBufLen = sizeof xddp ;
	   v4is_Get(gpi->xdr->pcb) ;
	   if (gpi->xdr->pcb->GetLen != UNUSED)
	    { xdr->Cache[cx].LastPoint = xddp.LastPoint ; } else { xdr->Cache[cx].Flags |= XDRNEW ; } ;
#else
	   if (v4is_PositionKey(xdr->aid,(struct V4IS__Key *)&xddp,(struct V4IS__Key **)&xddp1,0,V4IS_PosDCKL) == V4RES_PosKey)
	    { xdr->Cache[cx].LastPoint = xddp1->LastPoint ; } else { xdr->Cache[cx].Flags |= XDRNEW ; } ;
#endif
	   FREEAREAMTLOCK(areax) ; 
	   break ;
	 } ;
/*	At this point xdr->Cache[cx].LastPoint has value of last xdict point for this dimension, or (xdr->Cache[cx].Flags | XDRNEW) is TRUE if first time for this dimension */
	numeric = di->Flags & V4DPI_DimInfo_Normalize ;
	casesensitive = di->Flags & V4DPI_DimInfo_CaseSensitive ;
	xde.kp.fld.AuxVal = V4IS_SubType_IntXDict ; xde.kp.fld.KeyType = V4IS_KeyType_V4 ; xde.kp.fld.KeyMode = V4IS_KeyMode_Alpha ;
	xde.DimXId = ((flags & V4DPI_XDict_DimDim)==0 ? xdr->Cache[cx].DimXId : UNUSED);
/*	Now move entry (convert to upper case) as the key */
	if (*entry == UCEOS) entry = DICTENTRYMT ;		/* Check for empty string & replace with special character */
	if (numeric)						/* Maybe strip leading 0's */
	 { for(;*entry==UClit('0');) { entry++ ; } ;
	   if (*entry==UCEOS) entry = UClit("0") ; 	/* All zeros? then make single 0 */
	 } ;
	for(bp=xde.EntryVal+1,first=TRUE,i=0,ep=entry,lastnb=bp;i<V4DPI_XDictEntryVal_Max && *ep != UCEOS;i++,ep++) /* Normalize entry & copy into xde */
	 { if (*ep == ' ')
	    { if (first) continue ;
	      *(bp++) = (casesensitive ? *ep : UCTOUPPER(*ep)) ;
	    } else
	    { first = FALSE ; lastnb = bp ;		/* Track last non-blank byte */
	      *(bp++) = (casesensitive ? *ep : UCTOUPPER(*ep)) ;
	    } ;
	 } ;
	lastnb++ ; bytesInKey = ALIGN((char *)lastnb - (char *)&xde) ;
	*(lastnb++) = UCEOS ; *(lastnb++) = UCEOS ; *(lastnb++) = UCEOS ;
	elen = UCstrlen(&xde.EntryVal[1]) ; xde.EntryVal[0] = elen ;
	if (*ep != UCEOS)		/* Entry too long? */
	 { v_Msg(NULL,emsg,"*DPIDictTrunc",entry,&xde.EntryVal[1]) ; vout_UCText(VOUT_Warn,0,emsg) ; } ;
	dictid = (int *)((char *)&xde + bytesInKey) ;					/* Integer align pointer to dictionary entry */
	xde.kp.fld.Bytes = ALIGN((char *)dictid - (char *)&xde) ;	/* Store size of key */
/*	See if got the entry */
	GRABAREAMTLOCK(areax) ;
#ifdef XDICTMU
	gpi->xdr->pcb->GetMode = (V4IS_PCB_GP_Keyed | V4IS_PCB_NoLock | V4IS_PCB_IgnoreLock | V4IS_PCB_NoError  | V4IS_PCB_LockWait) ;
	gpi->xdr->pcb->KeyPtr = (struct V4IS__Key *)&xde ;
	gpi->xdr->pcb->GetBufPtr = (BYTE *)&xde ;
	gpi->xdr->pcb->GetBufLen = sizeof xde ;
	v4is_Get(gpi->xdr->pcb) ;
	if (gpi->xdr->pcb->GetLen != UNUSED)
	 { dictid = (int *)((char *)&xde + bytesInKey) ;
	   FREEAREAMTLOCK(areax) ; return(*dictid) ;
	 } ;
#else
	if (v4is_PositionKey(aid,(struct V4IS__Key *)&xde,(struct V4IS__Key **)&xde1,0,V4IS_PosDCKL) == V4RES_PosKey)
	 { dictid = (int *)((char *)xde1 + bytesInKey) ;
	   FREEAREAMTLOCK(areax) ; return(*dictid) ;
	 } ;
#endif
/*	Entry does not exist - define it & corresponding Reverse entry */
	*dictid = ++xdr->Cache[cx].LastPoint ; xdr->Cache[cx].Flags |= XDRDIRTY ;

#ifdef XDICTMU
	if (gpi->xdr->pcb->LockMode != 0)	/* If we are doing multi-user updates then have to update 'last used point' record for this xdict dimension */
	 {
	   xddp.kp.fld.AuxVal = V4IS_SubType_XDictDimPoints ; xddp.kp.fld.KeyType = V4IS_KeyType_V4 ; xddp.kp.fld.KeyMode = V4IS_KeyMode_Alpha ;
	   xddp.DimXId = xdr->Cache[cx].DimXId ; xddp.kp.fld.Bytes = (char *)&xddp.LastPoint - (char *)&xddp ;
/*	   First grab it again (to lock) */
	   gpi->xdr->pcb->GetMode = (V4IS_PCB_GP_Keyed | V4IS_PCB_NoError | V4IS_PCB_LockWait) ;
	   gpi->xdr->pcb->KeyPtr = (struct V4IS__Key *)&xddp ;
	   gpi->xdr->pcb->GetBufPtr = (BYTE *)&xddp ;
	   gpi->xdr->pcb->GetBufLen = sizeof xddp ;
	   v4is_Get(gpi->xdr->pcb) ;
/*	   Now update it */
	   xddp.LastPoint = xdr->Cache[cx].LastPoint ;
	   xdr->Cache[cx].Flags &= ~(XDRDIRTY) ;		/* Clear cache flag because we are updating right now */
	   gpi->xdr->pcb->PutMode = (V4IS_PCB_GP_Write | V4IS_PCB_NoError) ;
	   gpi->xdr->pcb->KeyPtr = (struct V4IS__Key *)&xddp ;
	   gpi->xdr->pcb->PutBufPtr = (BYTE *)&xddp ;
	   gpi->xdr->pcb->PutBufLen = sizeof xddp  ;
	   gpi->xdr->pcb->DataMode = V4IS_PCB_DataMode_Index ;
	   if (!v4is_Put(gpi->xdr->pcb,gpi->ctx->ErrorMsgAux))
	    { v_Msg(gpi->ctx,NULL,"*DPIDictIOErr",1,di->DimId,entry) ; vout_UCText(VOUT_Warn,0,gpi->ctx->ErrorMsg) ;
	    } ;
	 } ;
#endif


	xrde.kp.fld.AuxVal = V4IS_SubType_RevXDict ; xrde.kp.fld.KeyType = V4IS_KeyType_V4 ; xrde.kp.fld.KeyMode = V4IS_KeyMode_Int2 ;

	for(bp=xrde.EntryVal+1,first=TRUE,i=0,ep=entry,lastnb=bp;i<V4DPI_XDictEntryVal_Max && *ep != UCEOS;i++,ep++) /* Normalize entry & copy into xde */
	 { if (*ep == UClit(' ')) { if (first) continue ; *(bp++) = *ep ;
	    } else		/* Track last non-blank byte */
	    { first = FALSE ; lastnb = bp ; *(bp++) =  *ep  ; } ;
	 } ; *(++lastnb) = UCEOS ; *(++lastnb) = UCEOS ; *(++lastnb) = UCEOS ; xrde.EntryVal[0] = elen ;
	xrde.EntryId = *dictid ;
	xrde.DimXId = xdr->Cache[cx].DimXId ;
	xrde.kp.fld.Bytes = (char *)&xrde.EntryVal - (char *)&xrde ;
	elen = ALIGN((char *)&xrde.EntryVal[xde.EntryVal[0]+1] - (char *)&xrde) ;
#ifdef XDICTMU
/*	These are new records so we shouldn't have to worry about locking them first */
	gpi->xdr->pcb->PutMode = (V4IS_PCB_GP_Insert | V4IS_PCB_NoError) ;
	gpi->xdr->pcb->KeyPtr = (struct V4IS__Key *)&xrde ;
	gpi->xdr->pcb->PutBufPtr = (BYTE *)&xrde ;
	gpi->xdr->pcb->PutBufLen = elen  ;
	gpi->xdr->pcb->DataMode = V4IS_PCB_DataMode_Index ;
	if (!v4is_Put(gpi->xdr->pcb,gpi->ctx->ErrorMsgAux))
	 { v_Msg(gpi->ctx,NULL,"*DPIDictIOErr",2,di->DimId,entry) ; vout_UCText(VOUT_Warn,0,gpi->ctx->ErrorMsg) ;
	 } ;
	elen = (char *)dictid + sizeof *dictid - (char *)&xde ;
	gpi->xdr->pcb->PutMode = (V4IS_PCB_GP_Insert | V4IS_PCB_NoError) ;
	gpi->xdr->pcb->KeyPtr = (struct V4IS__Key *)&xde ;
	gpi->xdr->pcb->PutBufPtr = (BYTE *)&xde ;
	gpi->xdr->pcb->PutBufLen = elen  ;
	gpi->xdr->pcb->DataMode = V4IS_PCB_DataMode_Index ;
	if (!v4is_Put(gpi->xdr->pcb,gpi->ctx->ErrorMsgAux))
	 { v_Msg(gpi->ctx,NULL,"*DPIDictIOErr",3,di->DimId,entry) ; vout_UCText(VOUT_Warn,0,gpi->ctx->ErrorMsg) ;
	 } ;
#else
	if (v4is_Insert(aid,(struct V4IS__Key *)&xrde,&xrde,elen,V4IS_PCB_DataMode_Index,0,0,FALSE,FALSE,0,gpi->ctx->ErrorMsgAux) == UNUSED)
	 { v_Msg(gpi->ctx,NULL,"*DPIDictIOErr",4,di->DimId,entry) ; vout_UCText(VOUT_Warn,0,gpi->ctx->ErrorMsg) ;
	   v4is_Replace(aid,(struct V4IS__Key *)&xrde,&xrde,&xrde,elen,V4IS_PCB_DataMode_Index,0,0,0) ;
	 } ;
	elen = (char *)dictid + sizeof *dictid - (char *)&xde ;
	if (v4is_Insert(aid,(struct V4IS__Key *)&xde,&xde,elen,V4IS_PCB_DataMode_Index,0,0,FALSE,FALSE,0,gpi->ctx->ErrorMsgAux) == UNUSED)
	 { v_Msg(gpi->ctx,NULL,"*DPIDictIOErr",5,di->DimId,entry) ; vout_UCText(VOUT_Warn,0,gpi->ctx->ErrorMsg) ;
	   v4is_Replace(aid,(struct V4IS__Key *)&xde,&xde,&xde,elen,V4IS_PCB_DataMode_Index,0,0,0) ;
	 } ;
#endif
	FREEAREAMTLOCK(areax) ;
	return(*dictid) ;
}


/*	v4dpi_RevXDictEntryGet - Gets string associated with a DictId	*/
/*	Call: string = v4dpi_RevXDictEntryGet( ctx, dimid , dictid )
	  where string is pointer to string (character) value associated with dictid,
		ctx is pointer to current context,
		dimid is dimension index,
		dictid is the dictionary entry				*/

UCCHAR *v4dpi_RevXDictEntryGet(ctx,dimid,dictid)
  struct V4C__Context *ctx ;
  int dimid,dictid ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4DPI__RevXDictEntry xrde ;
  struct V4DPI_XDictDimPoints xddp ;
  struct V4DPI__DimInfo *di ;
  int cx,xid ; INDEX areax ;

	if (dictid < 0)					/* Got a dim:#xxx entry? */
	 { dictid &= 0x7fffffff ;			/* Get rid of sign bit */
	   UCsprintf(ctx->rxdName,UCsizeof(ctx->rxdName),UClit("%x"),dictid) ;		/* Convert to hex display */
	   return(ctx->rxdName) ;
	 } ;
	if (gpi->xdr == NULL) return(UClit("?noextdictopen")) ;
	if (gpi->xdr->xdrc != NULL)				/* Try the cache? */
	 { gpi->xdr->xdrc->Attempts++ ;
	   cx = (((dimid << 15) + dictid) & 0x7fffffff) % V4DPI_XDictRevCacheMax ;
	   if (gpi->xdr->xdrc->Cache[cx].DimId == dimid && gpi->xdr->xdrc->Cache[cx].DictId == dictid)
	    { gpi->xdr->xdrc->Hits++ ; return(gpi->xdr->xdrc->Cache[cx].StrVal) ; } ;
	 } ;
	FINDAREAINPROCESS(areax,gpi->xdr->aid) ;
#ifdef TRACEGRAB
printf("revxdictentryget %d\n",mmm->Areas[areax].areaLock) ;
#endif
	for(cx=dimid%V4DPI_XDictRTCacheMax;;cx=(cx+1)%V4DPI_XDictRTCacheMax)
	 { if (gpi->xdr->Cache[cx].DimId == dimid) break ;
	   if (gpi->xdr->Cache[cx].DimId != 0) continue ;
	   if (gpi->xdr->Count >= V4DPI_XDictRTCacheMax - 10)
	    v4_error(0,0,"V4DPI","XDictGet","TOOMNYXDICT","Exceeded max number of concurrent XDict entries") ;
/*	   Have to get mapping from DimId -> DimXId & Last Used Point */
	   di = v4dpi_DimInfoGet(ctx,dimid) ;
	   xid = v4dpi_XDictDimGet(ctx,di->DimName) ;
	   GRABMTLOCK(gpi->xdr->mtLock) ;
	   if (gpi->xdr->Cache[cx].DimId != 0) { FREEMTLOCK(gpi->xdr->mtLock) ; continue ; } ;	/* Make sure slot still free */
	    gpi->xdr->Cache[cx].DimId = di->DimId ; gpi->xdr->Cache[cx].DimXId = xid ;
	   if (gpi->xdr->Cache[cx].DimXId < 1)			/* Don't have this dimension in the dictionary */
	    { gpi->xdr->Cache[cx].DimId = 0 ; FREEMTLOCK(gpi->xdr->mtLock) ; UCsprintf(ctx->rxdName,UCsizeof(ctx->rxdName),UClit("#%x"),dictid) ; return(ctx->rxdName) ; } ;
	   gpi->xdr->Count++ ;
	   gpi->xdr->Cache[cx].Flags = 0 ; gpi->xdr->Cache[cx].LastPoint = 0 ;
	   xddp.kp.fld.AuxVal = V4IS_SubType_XDictDimPoints ; xddp.kp.fld.KeyType = V4IS_KeyType_V4 ; xddp.kp.fld.KeyMode = V4IS_KeyMode_Alpha ;
	   xddp.DimXId = gpi->xdr->Cache[cx].DimXId ; xddp.kp.fld.Bytes = (char *)&xddp.LastPoint - (char *)&xddp ;
	   GRABAREAMTLOCK(areax) ;
#ifdef XDICTMU
	   gpi->xdr->pcb->GetMode = (V4IS_PCB_GP_Keyed | V4IS_PCB_NoLock | V4IS_PCB_IgnoreLock | V4IS_PCB_NoError | V4IS_PCB_LockWait) ;
	   gpi->xdr->pcb->KeyPtr = (struct V4IS__Key *)&xddp ;
	   gpi->xdr->pcb->GetBufPtr = (BYTE *)&xddp ;
	   gpi->xdr->pcb->GetBufLen = sizeof xddp ;
	   v4is_Get(gpi->xdr->pcb) ;
	   if (gpi->xdr->pcb->GetLen != UNUSED)
	    { gpi->xdr->Cache[cx].LastPoint = xddp.LastPoint ; } else { gpi->xdr->Cache[cx].Flags |= XDRNEW ; } ;
#else
	   if (v4is_PositionKey(gpi->xdr->aid,(struct V4IS__Key *)&xddp,(struct V4IS__Key **)&xddp1,0,V4IS_PosDCKL) == V4RES_PosKey)
	    { gpi->xdr->Cache[cx].LastPoint = xddp1->LastPoint ; } else { gpi->xdr->Cache[cx].Flags |= XDRNEW ; } ;
#endif
	   FREEMTLOCK(gpi->xdr->mtLock) ;
	   break ;
	 } ;
	xrde.kp.fld.AuxVal = V4IS_SubType_RevXDict ; xrde.kp.fld.KeyType = V4IS_KeyType_V4 ; xrde.kp.fld.KeyMode = V4IS_KeyMode_Int2 ;
	xrde.EntryId = dictid ;	xrde.DimXId = gpi->xdr->Cache[cx].DimXId ;
	xrde.kp.fld.Bytes = (char *)&xrde.EntryVal - (char *)&xrde ;
	GRABMTLOCK(gpi->xdr->mtLock) ;
	if (gpi->xdr->xdrc == NULL) gpi->xdr->xdrc =
	 (struct V4DPI__XDictRevCache *)v4mm_AllocChunk(sizeof *gpi->xdr->xdrc,TRUE) ;
#ifdef XDICTMU
	gpi->xdr->pcb->GetMode = (V4IS_PCB_GP_Keyed | V4IS_PCB_NoLock | V4IS_PCB_IgnoreLock | V4IS_PCB_NoError | V4IS_PCB_LockWait) ;
	gpi->xdr->pcb->KeyPtr = (struct V4IS__Key *)&xrde ;
	gpi->xdr->pcb->GetBufPtr = (BYTE *)&xrde ;
	gpi->xdr->pcb->GetBufLen = sizeof xrde ;
	v4is_Get(gpi->xdr->pcb) ;
	if (gpi->xdr->pcb->GetLen == UNUSED)
	 { FREEAREAMTLOCK(areax) ; FREEMTLOCK(gpi->xdr->mtLock) ; UCsprintf(ctx->rxdName,UCsizeof(ctx->rxdName),UClit("#%x"),dictid) ; return(ctx->rxdName) ; } ;
	UCstrncpy(ctx->rxdName,&xrde.EntryVal[1],xrde.EntryVal[0]) ; ctx->rxdName[xrde.EntryVal[0]] = 0 ;
#else
	if (v4is_PositionKey(gpi->xdr->aid,(struct V4IS__Key *)&xrde,(struct V4IS__Key **)&xrde1,0,V4IS_PosDCKL) != V4RES_PosKey)
	 { FREEAREAMTLOCK(areax) ; FREEMTLOCK(gpi->xdr->mtLock) ; UCsprintf(ctx->rxdName,UCsizeof(ctx->rxdName),UClit("#%x"),dictid) ; return(ctx->rxdName) ; } ;
	UCstrncpy(ctx->rxdName,&xrde1->EntryVal[1],xrde1->EntryVal[0]) ; ctx->rxdName[xrde1->EntryVal[0]] = 0 ;
#endif
	if (UCstrcmp(ctx->rxdName,DICTENTRYMT) == 0) { ZUS(ctx->rxdName) ; } ;
	cx = (((dimid << 15) + dictid) & 0x7fffffff) % V4DPI_XDictRevCacheMax ;
	gpi->xdr->xdrc->Cache[cx].DimId = dimid  ; gpi->xdr->xdrc->Cache[cx].DictId = dictid ;
	UCstrcpy(gpi->xdr->xdrc->Cache[cx].StrVal,ctx->rxdName) ;
	FREEAREAMTLOCK(areax) ; FREEMTLOCK(gpi->xdr->mtLock) ;
	return(ctx->rxdName) ;
}

/*	D I C T I O N A R Y   M O D U L E S				*/
#ifdef V4ENABLEMULTITHREADS
  static DCLSPINLOCK degLock = UNUSEDSPINLOCKVAL ;
#endif

/*	v4dpi_DictEntryGet - Converts string to internal dictionary entry number */
/*	Call: dictnum = v4dpi_DictEntryGet( ctx, createdimid , entry , diminfo , dimupd )
	  where dictnum is number (0 if not found),
		ctx is pointer to current context,
		createdimid is dimid to use to auto-create (0 for NId, UNUSED for no auto-create),
		** NOTE ** if createdimid is specified then it overrides NOCREATE flag associated with diminfo 
		entry is dictionary entry string,
		diminfo is pointer to dimension info, NULL if none, DIMDICT if looking for dimension,
		dimupd, if not NULL, is updated with preferred dimension (0 if none) */

int v4dpi_DictEntryGet(ctx,createdimid,entry,diminfo,dimupd)
  struct V4C__Context *ctx ;
  UCCHAR *entry ;
  struct V4DPI__DimInfo *diminfo ;
  int createdimid,*dimupd ;
{ struct V4DPI__DictEntry de,*de1 ;
  struct V4DPI__DimInfo *di ;
  struct V4DPI__DictEntryDim *ded ;
  struct V4MM__MemMgmtMaster *mmm ;
#define LHMAX 211
#define LHTHRASH1 197 
  struct lcl__Cache {
    int hash ;
    int HashDiv ;
    struct {
      int EntryVal ;		/* Value of Dict Entry */
      int DimId ;
      int Thrash ;		/* Dimension, if any, associated with entry */
      UCCHAR Name[V4DPI_DictEntryVal_Max+1] ;
     } Entry[LHMAX] ;
   } ; static struct lcl__Cache *lc=NULL ;
  int i,rx,auxval,lowrx,hghrx,dicttype,numeric,isdim,elen,casesensitive ; int *dictid,idReturn ;
  INDEX areax ;
  UCCHAR tbuf[V4DPI_DictEntryVal_Max*2], lbuf[V4DPI_DictEntryVal_Max+1] ;

	if (!GRABMTLOCK(degLock)) return(0) ;
	if (lc == NULL) { lc = (struct lcl__Cache *)v4mm_AllocChunk(sizeof *lc,TRUE) ; lc->HashDiv = LHMAX ; } ;
/*	What kind of info do we have in diminfo ? */
	numeric = FALSE ; isdim = FALSE ; casesensitive = FALSE ;
	if ((void *)diminfo == (void *)DIMDICT)
	 { lowrx = gpi->LowHNum ; hghrx = gpi->HighHNum ; isdim = TRUE ; auxval = V4IS_SubType_DimDict ; dicttype = V4DPI_DictType_Ext ; }
	 else if (diminfo == NULL)
		{ lowrx = gpi->LowHNum ; hghrx = gpi->HighHNum ; auxval = V4IS_SubType_ExtDict ; dicttype = V4DPI_DictType_Ext ; }
	 else { di = (struct V4DPI__DimInfo *)diminfo ; dicttype = di->DictType ;
		switch (di->DictType)
		 { default:
		   case V4DPI_DictType_Int: lowrx = XTRHNUMDICT(di->DimId) ; hghrx = lowrx ; auxval = V4IS_SubType_IntDict ; break ;
		   case V4DPI_DictType_Ext: lowrx = gpi->LowHNum ; hghrx = gpi->HighHNum ; auxval = V4IS_SubType_ExtDict ; break ;
		   case V4DPI_DictType_Dim: isdim = TRUE ; lowrx = gpi->LowHNum ; hghrx = gpi->HighHNum ; auxval = V4IS_SubType_DimDict ; break ;
		 } ;
		numeric = di->Flags & V4DPI_DimInfo_Normalize ;
		casesensitive = di->Flags & V4DPI_DimInfo_CaseSensitive ;
		if (di->Flags & V4DPI_DimInfo_Dim)	/* Set flag if this is DIM dimension */
		 { isdim = TRUE ; auxval = V4IS_SubType_DimDict ; } ;
	      } ;
/*	Construct key & see if we got in specified area */
	de.kp.fld.AuxVal = auxval ; de.kp.fld.KeyType = V4IS_KeyType_V4 ; de.kp.fld.KeyMode = V4IS_KeyMode_Alpha ;
/*	Now move entry (convert to upper case) as the key */
	for(;*entry==UClit(' ');) { entry++ ; } ;		/* String leading junk */
	if (*entry == UCEOS) entry = DICTENTRYMT ;		/* Check for empty string & replace with special character */
	elen = UCstrlen(entry) ;
	if (elen > V4DPI_DictEntryVal_Max)		/* Entry too long? */
	 { UCstrncpy(lbuf,entry,V4DPI_DictEntryVal_Max) ; lbuf[V4DPI_DictEntryVal_Max] = UCEOS ;
	   v_Msg(NULL,UCTBUF2,"*DPIDictTrunc",entry,lbuf) ; vout_UCText(VOUT_Warn,0,UCTBUF2) ;
	   entry = lbuf ; elen = V4DPI_DictEntryVal_Max ;
	 } ;
	for(i=UCstrlen(entry)-1;i > 0 && *(entry+i)==' ';) { i-- ; } ;
	if (i < elen-1) { UCstrcpy(tbuf,entry) ; tbuf[i+1] = UCEOS ; entry = tbuf ; } ;
	if (numeric)					/* Maybe strip leading 0's */
	 { for(;*entry==UClit('0');) { entry++ ; } ;
	   if (*entry==UCEOS) entry = UClit("0") ;	/* All zeros? then make single 0 */
	 } ;
	for(i=0,lc->hash=1;i<V4DPI_DictEntryVal_Max && *(entry+i) != 0;i++)
	 { lc->hash *= (de.EntryVal[i+1] = (casesensitive ? entry[i] : UCTOUPPER(entry[i]) )) << i ; } ;
	de.EntryVal[0] = i ; i++ ;			/* Store number of bytes in first byte */
	de.EntryVal[i] = 0 ; de.EntryVal[i+1] = 0 ; de.EntryVal[i+2] = 0 ;	/* Make sure word padded with nulls */
	lc->hash %= lc->HashDiv ; if (lc->hash < 0) lc->hash = -lc->hash ;
	if (!isdim)					/* Don't check cache for Dim entries (have separate dim cache) */
	 { if (UCstrcmp(lc->Entry[lc->hash].Name,&de.EntryVal[1]) == 0)		/* Got match in local cache? */
	    { if (dimupd != NULL) *dimupd = lc->Entry[lc->hash].DimId ;
	      FREEMTLOCK(degLock) ; return(lc->Entry[lc->hash].EntryVal) ;
	    } ;
	 } ;
	de.kp.fld.Bytes = ALIGN(sizeof de.kp + (i * sizeof entry[0])) ;	/* Store size of key */
/*	See if got the entry */
	for(rx=lowrx;rx<=hghrx;rx++)
	 { if (gpi->RelH[rx].aid == UNUSED) continue ;
	   FINDAREAINPROCESS(areax,gpi->RelH[rx].aid) ;
	   GRABAREAMTLOCK(areax) ;
	   if (v4is_PositionKey(gpi->RelH[rx].aid,(struct V4IS__Key *)&de,(struct V4IS__Key **)&de1,0,V4IS_PosDCKL) == V4RES_PosKey) break ;
	   FREEAREAMTLOCK(areax) ;
	 } ;
	if (rx > hghrx) 				/* Did we find anything ? */
	 { FREEAREAMTLOCK(areax) ;
	   if ((void *)diminfo == (void *)DIMDICT || diminfo == NULL) { FREEMTLOCK(degLock) ; return(0) ; } ;			/* Don't auto-create dimensions */
	   di = (struct V4DPI__DimInfo *)diminfo ;
	   if (di->Flags & V4DPI_DimInfo_NoAutoCreate && createdimid <= 0) { FREEMTLOCK(degLock) ; return(0) ; } ;	/* Not allowed to auto-create */
	   for(rx=hghrx;rx>=lowrx;rx--)
	    { if (gpi->RelH[rx].aid == UNUSED) continue ; if (rx == V4DPI_WorkRelHNum) continue ;
	      if (dicttype == V4DPI_DictType_Int ? gpi->RelH[rx].ahi.IntDictUpd : gpi->RelH[rx].ahi.IntDictUpd) break ;
	    } ;
	   if (rx < gpi->LowHNum && gpi->RelH[V4DPI_WorkRelHNum].aid != UNUSED) rx = V4DPI_WorkRelHNum ;
	   if (rx < gpi->LowHNum) { FREEMTLOCK(degLock) ; return(0) ; } ;		/* Can't create it */
	   if (dimupd != NULL) *dimupd = 0 ;
	   /* printf("[AutoCreating dictionary entry: %s]\n",entry) ; */
	   if (createdimid >= 0)
	    { int newid = v4dpi_DictEntryPut(ctx,gpi->RelH[rx].aid,entry,dicttype,createdimid,di) ;
	      FREEMTLOCK(degLock) ;
	      return(newid) ;
	    } else { FREEMTLOCK(degLock) ; return(0) ; } ;
	 } ;
	dictid = (int *)(de1->EntryVal + ALIGN(i)) ;		/* Position to integer ID immediately following */
	ded = (struct V4DPI__DictEntryDim *)dictid ;
	if (!isdim)						/* Don't check cache for Dim entries (have separate dim cache) */
	 { if (++lc->Entry[lc->hash].Thrash > 100)
	    { lc->Entry[lc->hash].Thrash = 0 ;
	      lc->HashDiv = (lc->HashDiv == LHMAX ? LHTHRASH1 : LHMAX) ;
	    } ;
	   UCstrcpy(lc->Entry[lc->hash].Name,&de.EntryVal[1]) ;	/* Copy name into cache */
	 } ;
	if (ded->Marker == 0)					/* Got preferred dimension ? */
	 { if (dimupd != NULL) *dimupd = ded->DimId ;
	   if (!isdim) { lc->Entry[lc->hash].EntryVal = ded->EntryId ; lc->Entry[lc->hash].DimId = ded->DimId ; } ;
	   idReturn = ded->EntryId ;
	   FREEAREAMTLOCK(areax) ; FREEMTLOCK(degLock) ; return(idReturn) ;
	 } else
	 { if (dimupd != NULL) *dimupd = 0 ;
	   if (!isdim) { lc->Entry[lc->hash].EntryVal = *dictid ; lc->Entry[lc->hash].DimId = 0 ; } ;
	   idReturn = *dictid ;
	   FREEAREAMTLOCK(areax) ; FREEMTLOCK(degLock) ; return(idReturn) ;
	 } ;
}

/*	v4dpi_DictEntryEnum - Enumerates thru all dictionary entries in context */
/*	Call: num = v4dpi_DictEntryEnum( ctx, dimid, reslist , dictype )
	  where num is number of entries found,
		ctx is pointer to current context,
		dimid is dimension for only those with dimid as preferred, (UNUSED for all),
		reslist is pointer to V4DPI__DictList for results,
		dictype is type of dictionary entry wanted (ex: V4IS_SubType_DimDict)		*/

int v4dpi_DictEntryEnum(ctx,dimid,reslist,dictype)
  struct V4C__Context *ctx ;
  int dimid,dictype ;
  struct V4DPI__DictList *reslist ;
{ struct V4DPI__DictEntry de,*de1 ;
  struct V4DPI__DictEntryDim *ded ;
  struct V4MM__MemMgmtMaster *mmm ;
  int rx,len,*dictid ; INDEX areax ;

	reslist->Count = 0 ;
	for(rx=gpi->LowHNum;rx<=gpi->HighHNum;rx++)
	 { if (gpi->RelH[rx].aid == UNUSED) continue ;
	   memset(&de,0,sizeof de) ;
	   FINDAREAINPROCESS(areax,gpi->RelH[rx].aid) ;
	   GRABAREAMTLOCK(areax) ;
	   v4is_PositionKey(gpi->RelH[rx].aid,(struct V4IS__Key *)&de,(struct V4IS__Key **)&de1,0,V4IS_PosDCKL) ;
	   for(;v4is_PositionRel(gpi->RelH[rx].aid,V4IS_PosNext,-1,(struct V4IS__Key **)&de1);)
	    { if (de1->kp.fld.KeyType != V4IS_KeyType_V4 || de1->kp.fld.KeyMode != V4IS_KeyMode_Alpha) continue ;
	      if (de1->kp.fld.AuxVal != dictype) continue ;
	      len = de1->EntryVal[0]+1 ;
	      dictid = (int *)((char *)&de1->EntryVal + (ALIGN(len)*sizeof(UCCHAR))) ;	/* Position to integer ID immediately following */
	      ded = (struct V4DPI__DictEntryDim *)dictid ;
	      if (ded->Marker == 0)
	       { reslist->Entry[reslist->Count].DimId = ded->DimId ;
		 reslist->Entry[reslist->Count].DictId = ded->EntryId ;
	       } else
	       { reslist->Entry[reslist->Count].DimId = UNUSED ;
		 reslist->Entry[reslist->Count].DictId = *dictid ;
	       } ; reslist->Count++ ;
	      if (reslist->Count >= V4DPI_DictListMax)
	       { FREEAREAMTLOCK(areax) ; v4_error(0,0,"V4DPI","DictEntryEnum","TOOMANY","Exceeded max of %d entries",V4DPI_DictListMax) ; } ;
	    } ;
	   FREEAREAMTLOCK(areax) ;
	 } ;
	return(reslist->Count) ;
}


/*	v4dpi_DictEntryPut - Adds new string to internal dictionary, returns entry number */
/*	NOTE: This module assumes degLock is already set on entry (i.e. it does not fuss with this lock) */
/*	Call: dictnum = v4dpi_DictEntryPut(ctx , aid , entry , dicttype , dimid, di )
	  where dictnum is new number (0 if error in ctx->ErrorMsgAux),
		ctx is current context,
		aid is area id,
		entry is string,
		dicttype is dictionary type- V4DPI_DictType_xxx,
		dimid is id of preferred dimension, 0 if none,
		di is pointer to V4DPI__DimInfo structure				*/

int v4dpi_DictEntryPut(ctx,aid,entry,dicttype,dimid,di)
  struct V4C__Context *ctx ;
  int aid ;
  UCCHAR *entry ;
  int dicttype,dimid ;
  struct V4DPI__DimInfo *di ;
{ struct V4DPI__DictEntry de ;
  struct V4DPI__RevDictEntry rde ;
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__AreaCB *acb ;
  struct V4DPI__DictEntryDim *ded ;
  int lrx,i,size,numeric,casesensitive ; int *dictid,idReturn ; INDEX areax ;

	numeric = di->Flags & V4DPI_DimInfo_Normalize ;
	casesensitive = di->Flags & V4DPI_DimInfo_CaseSensitive ;
	de.kp.fld.KeyType = V4IS_KeyType_V4 ; de.kp.fld.KeyMode = V4IS_KeyMode_Alpha ;
	switch (dicttype)
	 { default:			v_Msg(ctx,ctx->ErrorMsgAux,"InvDictType",dicttype) ; return(0) ;
	   case V4DPI_DictType_Int:	de.kp.fld.AuxVal = V4IS_SubType_IntDict ; break ;
	   case V4DPI_DictType_Ext:	de.kp.fld.AuxVal = V4IS_SubType_ExtDict ; break ;
	   case V4DPI_DictType_Dim:	de.kp.fld.AuxVal = V4IS_SubType_DimDict ;
					casesensitive = FALSE ;		/* Don't want entry for the dimension to be case sensitive */
					break ;
	 } ;
/*	Now move entry (convert to upper case) as the key */
	if (numeric)					/* Maybe strip leading 0's */
	 { for(;*entry=='0';) { entry++ ; } ;
	   if (*entry==0) entry = UClit("0") ; 		/* All zeros? then make single 0 */
	 } ;
	for(i=0;i<V4DPI_DictEntryVal_Max && *(entry+i) != 0;i++) { de.EntryVal[i+1] = (casesensitive ? entry[i] : UCTOUPPER(entry[i])) ; } ;
	de.EntryVal[0] = i ; i++ ;			/* Store number of characters in first byte */
	de.EntryVal[i] = 0 ; de.EntryVal[i+1] = 0 ; de.EntryVal[i+2] = 0 ;	/* Make sure word padded with nulls */
	de.kp.fld.Bytes = ALIGN(sizeof de.kp + (i * sizeof entry[0])) ;	/* Store size of key */
/*	See if got the entry */
	FINDAREAINPROCESS(areax,aid) ;
	GRABAREAMTLOCK(areax) ;
	if (v4is_PositionKey(aid,(struct V4IS__Key *)&de,NULL,0,V4IS_PosDCKL) == V4RES_PosKey)
	 { FREEAREAMTLOCK(areax) ; v_Msg(ctx,ctx->ErrorMsgAux,"DictEntryExists2",entry) ; return(0) ; } ;
/*	Entry does not exist - define it & corresponding Reverse entry */
	FREEAREAMTLOCK(areax) ;
	acb = v4mm_ACBPtr(aid) ;
	dictid = (int *)(de.EntryVal + ALIGN(i)) ;		/* Position to integer ID immediately following */
	ded = (struct V4DPI__DictEntryDim *)dictid ;
	LOCKROOT ;
	switch (dicttype)
	 { default:			v_Msg(ctx,ctx->ErrorMsgAux,"InvDictType",dicttype) ; return(0) ;
	   case V4DPI_DictType_Int:	*dictid = (acb->RootInfo->NextAvailIntDictNum)++ ; break ;
	   case V4DPI_DictType_Ext:	*dictid = (acb->RootInfo->NextAvailExtDictNum)++ ; break ;
	   case V4DPI_DictType_Dim:
		{ static struct V4DPI__DimInfo *diDim = NULL ;
		  if (gpi->xdr == NULL) { *dictid = (acb->RootInfo->NextAvailDimDictNum)++ ; break ; } ;
/*		  Have XDict active - try to assign dictid via XDict entry on Dim dimension */
		  if (di->Flags & V4DPI_DimInfo_Dim)				/* Booting up: Defining Dim:Dim */
		   { if (di->DimId == 0) di->DimId = 1 ;
		     *dictid = v4dpi_XDictEntryPut(gpi->xdr->aid,entry,di,0) ;
		     if (*dictid != 1)						/* Inconsistant something! */
		      { v_Msg(ctx,ctx->ErrorMsgAux,"InvXDictDim",*dictid) ; return(0) ; } ;
		     break ;
		   } else if (diDim == NULL)
			   { int dimid ; dimid = v4dpi_DimGet(ctx,UClit("DIM"),DIMREF_IRT) ;
			     if (dimid == 0) { v_Msg(ctx,ctx->ErrorMsgAux,"DimDimNotDef") ; return(0) ; } ;
			     DIMINFO(diDim,ctx,dimid) ;
			   } ;
		  *dictid = v4dpi_XDictEntryPut(gpi->xdr->aid,entry,diDim,0) ;
		  if (*dictid == 0)						/* If XDict not open for update */
		   *dictid = (acb->RootInfo->NextAvailDimDictNum)++ ;
		  break ;
		} ;
	 } ;
	RELROOT ;
	rde.kp.fld.AuxVal = V4IS_SubType_RevDict ; rde.kp.fld.KeyType = V4IS_KeyType_V4 ; rde.kp.fld.KeyMode = V4IS_KeyMode_Int ;
//	memcpy(&rde.EntryVal,&de.EntryVal,de.EntryVal[0]+1) ; Replace with upper/lower case entry (below)
	rde.EntryVal[0] = de.EntryVal[0] ; UCstrncpy(&rde.EntryVal[1],entry,de.EntryVal[0]) ;
	rde.EntryId = *dictid ;
	rde.kp.fld.Bytes = sizeof rde.kp + sizeof rde.EntryId ;
	size = (char *)dictid + sizeof *dictid - (char *)&de ;
	GRABAREAMTLOCK(areax) ;
	if (v4is_Insert(aid,(struct V4IS__Key *)&rde,&rde,size,V4IS_PCB_DataMode_Index,0,0,FALSE,FALSE,0,ctx->ErrorMsgAux) == -1)
	 { v_Msg(ctx,NULL,"*DictEntExists",entry) ; vout_UCText(VOUT_Warn,0,ctx->ErrorMsg) ;
	 } ;
	if (dimid != 0)
	 { ded->EntryId = *dictid ; ded->Marker = 0 ; ded->DimId = dimid ;
	   size = (char *)ded + sizeof(*ded) - (char *)&de ;
	   v4is_Insert(aid,(struct V4IS__Key *)&de,&de,size,V4IS_PCB_DataMode_Index,0,0,FALSE,FALSE,0,NULL) ;
	   idReturn = ded->EntryId ;
	   FREEAREAMTLOCK(areax) ;
	   return(idReturn) ;
	 } else
	 { if (v4is_Insert(aid,(struct V4IS__Key *)&de,&de,size,V4IS_PCB_DataMode_Index,0,0,FALSE,FALSE,0,ctx->ErrorMsgAux) == -1)
	    { v_Msg(ctx,NULL,"*DictEntExists",entry) ; vout_UCText(VOUT_Warn,0,ctx->ErrorMsg) ;
	    } ;
	   idReturn = *dictid ;
	   FREEAREAMTLOCK(areax) ;
	   return(idReturn) ;
	 } ;
}

/*	v4dpi_RevDictEntryGet - Gets string associated with a DictId	*/
/*	Call: string = v4dpi_RevDictEntryGet( ctx , dictid )
	  where string is pointer to string (character) value associated with dictid,
		ctx is pointer to current context,
		dictid is the dictionary entry (aid built into dictid)	*/

UCCHAR *v4dpi_RevDictEntryGet(ctx,dictid)
  struct V4C__Context *ctx ;
  int dictid ;
{ struct V4DPI__RevDictEntry rde,*rde1 ;
  struct V4MM__MemMgmtMaster *mmm ;
  int rx ; INDEX areax ;

/*	Have to figure out where to look */
	if (dictid < MAXDIMNUM) 		/* Is this a dimension entry ? */
	 { rx = XTRHNUMDIM(dictid) ; }
	 else { rx = XTRHNUMDICT(dictid) ;	/* If external then HNUM embedded in entry */
		if (rx == 0) rx = gpi->LowHNum ;
	      } ;
	rde.kp.fld.AuxVal = V4IS_SubType_RevDict ; rde.kp.fld.KeyType = V4IS_KeyType_V4 ; rde.kp.fld.KeyMode = V4IS_KeyMode_Int ;
	rde.EntryId = dictid ; rde.kp.fld.Bytes = sizeof rde.kp + sizeof rde.EntryId ;
	if (rx < 0 || rx >= V4C_CtxRelH_Max ? TRUE : gpi->RelH[rx].aid < 0)
	 { UCsprintf(ctx->rdName,UCsizeof(ctx->rdName),UClit("?#%x"),dictid) ; return(ctx->rdName) ; } ;
	FINDAREAINPROCESS(areax,gpi->RelH[rx].aid) ;
	GRABAREAMTLOCK(areax) ;
	if (v4is_PositionKey(gpi->RelH[rx].aid,(struct V4IS__Key *)&rde,(struct V4IS__Key **)&rde1,0,V4IS_PosDCKL) != V4RES_PosKey)
	 { FREEAREAMTLOCK(areax) ; UCsprintf(ctx->rdName,sizeof ctx->rdName,UClit("?#%x"),dictid) ; return(ctx->rdName) ; } ;
	UCstrncpy(ctx->rdName,&rde1->EntryVal[1],rde1->EntryVal[0]) ; ctx->rdName[rde1->EntryVal[0]] = 0 ;
	if (UCstrcmp(ctx->rdName,DICTENTRYMT) == 0) { ZUS(ctx->rdName) ; } ;
	FREEAREAMTLOCK(areax) ;
	return(ctx->rdName) ;
}

/*	P O I N T   M O D U L E S					*/

static UCCHAR ucdimbuf[100] ;		/* Holds Unicode version */

/*	v4dpi_PointParse - Parses string and creates a point		*/
/*	Call: result = v4dpi_PointParse( ctx , point , tcb , flags )
	  where result is result of the parse,
		ctx is pointer to current context,
		point is pointer to V4DPI__Point and is updated with point info,
		tcb is pointer to token control block,
		flags are special parsing flags: V4DPI_PointParse_xxx	*/

LOGICAL v4dpi_PointParse(ctx,point,tcb,flags)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *point ;
  struct V4LEX__TknCtrlBlk *tcb ;
  int flags ;
{ struct V4DPI__Point tp1,tp2,*cpp,*cpt ;
  struct V4DPI__Point isctp ;				/* Point to parse intersection */
  struct V4DPI__Point_IntMix *pim,*pim1 ;
  struct V4DPI__Point_UOMMix *pum ;
  struct V4DPI__Point_UOMPerMix *pupm ;
  struct V4DPI__Point_UOMPUOMMix *pupum ;
  struct V4DPI__Point_RealMix *prm ;
  struct V4DPI__Point_FixMix *pfm ;
  struct V4DPI__Point_AlphaMix *pam ;
  struct V4DPI__TagVal *tv ;				/* Structure of a tagged value */
  struct V4DPI__DimInfo *di ;
  regmatch_t matches[10];
  int inrange,gotcompound,i,j,n,dimid,quoted,forceeval,gotrel,trace,minus ; double d1,d2 ; B64INT b641,b642 ;
  unsigned char tbuf[V4DPI_AlphaVal_Max+100] ; int nextfree ;		/* Used to build up alpha point values */
  UCCHAR errmsg[256], *ucb, ucbuf[V4DPI_AlphaVal_Max+100] ;
  static UCCHAR dimbuf1[100] ; static int errcnt=0 ;
#define TKN(flags) { v4lex_NextTkn(tcb,flags) ; if (tcb->type == V4LEX_TknType_Error) PERR("Syntax error") ; }
#define AUXERR { ucb = ctx->ErrorMsgAux ; goto PointParseErr ; }
#define PERR(MSG) { ucb = UClit(MSG) ; goto PointParseErr ; }
#define PERR1a(MSG,ARG1) { ucb = errmsg ; v_Msg(ctx,ucb,MSG,ARG1) ; goto PointParseErr ; }
#define PERR2(MSG,ARG1,ARG2) { ucb = errmsg ; v_Msg(ctx,ucb,MSG,ARG1,ARG2) ; goto PointParseErr ; }
#define PERR4(MSG,ARG1,ARG2,ARG3,ARG4) { ucb = errmsg ; v_Msg(ctx,ucb,MSG,ARG1,ARG2,ARG3,ARG4) ; goto PointParseErr ; }

/*	General format is dimension=point,point..point (if no dimension then point assumed to be NId) */
	if (CHECKV4INIT) AUXERR ;
	ZPH(point) ;				/* Clear out the point */
	di = NULL ;				/* No dimension yet */
	nextfree = 0 ;						/* Index to next free byte in point->AlphaVal */
	gotcompound = FALSE ;					/* TRUE if hit xxx,yyy or xxx..yyy */
	inrange = FALSE ;
	gotrel = FALSE ;					/* No relational operator yet (dim:>value) */
	trace = 0 ;
	pim = (struct V4DPI__Point_IntMix *)&point->Value.AlphaVal ;	/* Link IntMix in case we get a range of integers */
	pam = (struct V4DPI__Point_AlphaMix *)&point->Value.AlphaVal ;
	prm = (struct V4DPI__Point_RealMix *)&point->Value.AlphaVal ;
	pfm = (struct V4DPI__Point_FixMix *)&point->Value.AlphaVal ;
	pum = (struct V4DPI__Point_UOMMix *)&point->Value.AlphaVal ;
	pupm = (struct V4DPI__Point_UOMPerMix *)&point->Value.AlphaVal ;
	pupum = (struct V4DPI__Point_UOMPUOMMix *)&point->Value.AlphaVal ;
	cpp = &tp1 ; ZPH(cpp) ;					/* Start building into temp point #1 */
	for(gpi->ppHNum=gpi->HighHNum;gpi->ppHNum>=gpi->LowHNum;gpi->ppHNum--)	/* Find highest area allowing new entries */
	 { if (gpi->RelH[gpi->ppHNum].aid == UNUSED) continue ; if (gpi->ppHNum == V4DPI_WorkRelHNum) continue ;
	   if (gpi->RelH[gpi->ppHNum].ahi.BindingsUpd) break ;
	 } ;
/*	If parsing .xxx then don't allow multiple values for xxx (VEH 040610) */
	if ((flags & V4DPI_PointParse_NestedDot) != 0) flags |= V4DPI_PointParse_NoMult ;
/*	Parse: [dim:]value[..value][,value][...] or [intersection]	*/
next_token:
	TKN(0) ; if (tcb->type == V4LEX_TknType_Error) PERR("Syntax error") ;
/*	If have any recognition blocks then try pattern match now */
	if (tcb->opcode == V_OpCode_QString || tcb->opcode == V_OpCode_Keyword)
	 { struct V4DPI__RecogBlock *lrcgb ;
	   for(lrcgb=gpi->rcgb;lrcgb!=NULL;lrcgb=lrcgb->nrcgb)
	    { i = vregexp_RegExec(&lrcgb->rpb,UCretASC(tcb->opcode == V_OpCode_QString ? tcb->UCstring : tcb->UCkeyword),10,matches,0) ;
	      if (i) continue ;			/* This pattern did not match - try next */
	      for(j=0,i=0;;i++,j++)
	       { int len ;
	         ucbuf[j] = lrcgb->subbuf[i] ; if (ucbuf[j] == UCEOS) break ;
	         if (ucbuf[j] > 10) continue ;	/* Got a substitution (\0 .. \9)+1 */
		 n = ucbuf[j]-1 ; if (matches[n].rm_so == -1) continue ; 
		 len= matches[n].rm_eo - matches[n].rm_so ;
		 UCstrncpy(&ucbuf[j],&tcb->UCstring[matches[n].rm_so],len) ;
		 j += (len-1) ;
	       } ;
	      if (lrcgb->Flags & V4DPI_RegBlock_Eval)
	       { struct V4LEX__TknCtrlBlk *ltcb ; ltcb = (struct V4LEX__TknCtrlBlk *)v4mm_AllocChunk(sizeof *ltcb,FALSE) ;
		 v4lex_InitTCB(ltcb,0) ; v4lex_NestInput(ltcb,NULL,ucbuf,V4LEX_InpMode_String) ;
		 i = v4dpi_PointParse(ctx,&tp1,ltcb,V4DPI_PointParse_RetFalse) ; v4lex_FreeTCB(ltcb) ;
		 if (!i) { ucb = ctx->ErrorMsgAux ; goto PointParseErr ; } ;
		 if (v4dpi_IsctEval(point,&tp1,ctx,0,NULL,NULL) == NULL)
		  { v_Msg(ctx,ctx->ErrorMsgAux,"ParseRecog1",tcb->UCstring,ucbuf) ; ucb = ctx->ErrorMsgAux ; goto PointParseErr ; } ;
		  if (lrcgb->Flags & V4DPI_RegBlock_Trace)
		   { 
		     v_Msg(ctx,UCTBUF2,"@*%1U => %2U => %3P\n",tcb->UCstring,ucbuf,point) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
		   } ;
		 return(TRUE) ;
	       } ;
	      if (lrcgb->Flags & V4DPI_RegBlock_Trace)
	       { v_Msg(ctx,UCTBUF2,"@*%1U => %2U\n",tcb->UCstring,ucbuf) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ; } ;
	      if (ucbuf[0] != UCEOS) v4lex_NestInput(tcb,NULL,ucbuf,V4LEX_InpMode_String) ;
	      goto next_token ;
	    } ;
	 } ;
	if (tcb->type == V4LEX_TknType_VString)			/* Look for ^v"xxxx" to immediately evaluate */
	 { struct V4LEX__TknCtrlBlk *ltcb ; ltcb = v4mm_AllocChunk(sizeof *ltcb,FALSE) ;
	   v4lex_InitTCB(ltcb,0) ; v4lex_NestInput(ltcb,NULL,tcb->UCstring,V4LEX_InpMode_String) ;
	   v4eval_Eval(ltcb,ctx,FALSE,trace,FALSE,TRUE,TRUE,NULL) ;
	   v4lex_FreeTCB(ltcb) ;
	   goto next_token ;
	 } ;
#ifdef NEWQUOTE
	if (tcb->opcode == V_OpCode_AtSign)
	 { if (!v4dpi_PointParse(ctx,point,tcb,flags)) return(FALSE) ;
	   QUOTE(point) ;
	   return(TRUE) ;
	 } ;
#else
	 { QUOTE(point) ; v4lex_NextTkn(tcb,0) ; } ;	/* "@" flags point as being "quoted" */
#endif
	if (tcb->opcode == V_OpCode_BackQuote)
	 { point->ForceEval = TRUE ; v4lex_NextTkn(tcb,0) ; } ; /* "`" flags point as being forced-evaluation */
	if (tcb->opcode == V_OpCode_BackQuote)
	 { QUOTE(point) ; v4lex_NextTkn(tcb,0) ; } ;	/* Got "``" - set ForceEval & Quoted to indicate fail on undefined */
	if (tcb->type == V4LEX_TknType_PString)			/* Look for ^p"xxxx" to immediately evaluate */
	 { LOGICAL quoted, forceeval ;
//	   int quoted=point->Quoted,forceeval=point->ForceEval ;
	   struct V4LEX__TknCtrlBlk *ltcb ; ltcb = v4mm_AllocChunk(sizeof *ltcb,FALSE) ;
	   quoted = ISQUOTED(point) ; forceeval = point->ForceEval ;
	   v4lex_InitTCB(ltcb,0) ; v4lex_NestInput(ltcb,NULL,tcb->UCstring,V4LEX_InpMode_String) ;
	   i = v4dpi_PointParse(ctx,&isctp,ltcb,flags) ;
	   v4lex_FreeTCB(ltcb) ; if (!i) return(i) ;
	   cpt = v4dpi_IsctEval(point,&isctp,ctx,0,NULL,NULL) ;
	   if (cpt == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"PLitStrFail",tcb->UCstring) ; return(FALSE) ; } ;
	   if (cpt != point) memcpy(point,cpt,cpt->Bytes) ;
//	   point->Quoted = quoted ; point->ForceEval = forceeval ;
#ifndef NEWQUOTE
	   if (quoted) { QUOTE(point) ; } else { UNQUOTE(point) ; } ;
#endif
	   point->ForceEval = forceeval ;
	   return(i) ;
	 } ;
	if (tcb->type == V4LEX_TknType_Error) PERR("Syntax error") ;
	if (tcb->type == V4LEX_TknType_Float ||
	    ((tcb->type == V4LEX_TknType_Integer ||tcb->type == V4LEX_TknType_LInteger) && tcb->decimal_places > 0))
	 { if ((tp1.Dim = Dim_Num) == 0)  PERR2("CmdDimMissing",tcb->type,V4DPI_PntType_Real) ;
	   DIMINFO(di,ctx,tp1.Dim) ; tp1.PntType = di->PointType ;
	   if (tcb->type == V4LEX_TknType_Float) { d1 = tcb->floating ; }
	    else FIXEDtoDOUBLE(d1,tcb->Linteger,tcb->decimal_places) ;
	   PUTREAL(&tp1,d1) ; CPH(point,&tp1) ; goto check_continue ;
	 } ;
	if (tcb->type == V4LEX_TknType_Integer) 		/* If we got "nnn" then assume Int:nnn */
	 { if ((tp1.Dim = Dim_Int) == 0) PERR2("CmdDimMissing",tcb->type,V4DPI_PntType_Int) ;
	   DIMINFO(di,ctx,tp1.Dim) ; tp1.PntType = di->PointType ;
	   tp1.Value.IntVal = tcb->integer ; CPH(point,&tp1) ; goto check_continue ;
	 } ;
	if (tcb->type == V4LEX_TknType_LInteger) PERR("Integer literal too big- use FLOAT or FIXED") ;
	if (tcb->opcode == V_OpCode_Minus || tcb->opcode == V_OpCode_Plus)			/* If got "-", assume UDelta:-nn" */
	 { ETYPE op = tcb->opcode ;		/* Save this for later */
	   TKN(0) ;
	   switch(tcb->type)
	    { default:				PERR("Expecting numeric literal to follow '-'")
	      case V4LEX_TknType_Integer:
		if (tcb->decimal_places != 0)
		 { if ((point->Dim = Dim_Num) == 0) PERR2("CmdDimMissing",tcb->type,V4DPI_PntType_Real) ;
		   FIXEDtoDOUBLE(d1,tcb->integer,tcb->decimal_places) ;
		   if (op == V_OpCode_Minus) d1 = -d1 ;
		   dblPNTv(point,d1) ; goto endofpoint ;
		 } ;
		if ((point->Dim = Dim_UDelta) == 0) PERR2("CmdDimMissing",tcb->type,V4DPI_PntType_Delta) ;
		DIMINFO(di,ctx,point->Dim) ; point->PntType = di->PointType ;
		point->Value.IntVal = (op == V_OpCode_Plus ? tcb->integer : -tcb->integer) ;
		goto endofpoint ;
	      case V4LEX_TknType_LInteger:
		if (tcb->decimal_places != 0)
		 { if ((point->Dim = Dim_Num) == 0) PERR2("CmdDimMissing",tcb->type,V4DPI_PntType_Real) ;
		   FIXEDtoDOUBLE(d1,tcb->Linteger,tcb->decimal_places) ;
		   if (op == V_OpCode_Minus) d1 = -d1 ;
		 } else { d1 = -tcb->Linteger ; } ;
		dblPNTv(point,d1) ; goto endofpoint ;
	      case V4LEX_TknType_Float:
		d1 = (op == V_OpCode_Plus ? tcb->floating : -tcb->floating) ;
		dblPNTv(point,d1) ; goto endofpoint ;
	    } ;
	 } ;
	if (tcb->type == V4LEX_TknType_String)			/* If we got "xxx" then assume Alpha:"xxx" */
	 { if ((point->Dim = Dim_Alpha) == 0) PERR2("CmdDimMissing",tcb->type,V4DPI_PntType_Char) ;
	   if (tcb->opcode == V_OpCode_DQUString)		/* If string contains Unicode then force Unicode point (but Dim:Alpha) */
	    { if (UCstrlen(tcb->UCstring) >= V4DPI_UCVal_Max)
	       { v_Msg(ctx,NULL,"*ParseStrLenWrn",UCstrlen(tcb->UCstring),V4DPI_UCVal_Max-1,V4DPI_PntType_UCChar) ;
	         tcb->UCstring[V4DPI_UCVal_Max-1] = UCEOS ;
		 vout_UCText(VOUT_Warn,0,ctx->ErrorMsg) ;
	       } ;
	      uccharPNTv(point,tcb->UCstring) ;
	    }
//	    else { UCUTF16toUTF8(&point->Value.AlphaVal[1],V4DPI_AlphaVal_Max,tcb->UCstring,UCstrlen(tcb->UCstring)) ; alphaPNT(point) ; CHARPNTBYTES1(point) ; } ;
	    else { if (tcb->literal_len >= V4DPI_AlphaVal_Max) PERR1a("CmdTooLong",tcb->type) ;
		   UCstrcpyToASC(&point->Value.AlphaVal[1],tcb->UCstring) ; alphaPNT(point) ; CHARPNTBYTES2(point,tcb->literal_len) ;
		 } ;
/*	   This may not be an alpha point - see if next token is "*" or ":" - if so then quoted dimension */
	   if (UCstrlen(tcb->UCstring) <= V4DPI_DimInfo_DimNameMax)
	    { UCstrcpy(ucbuf,tcb->UCstring) ;
	      TKN(V4LEX_Option_RetEOL) ;
	      if (tcb->opcode == V_OpCode_Star || tcb->opcode == V_OpCode_Colon || tcb->opcode == V_OpCode_ColonEqual)
	       { UCstrcpy(ucdimbuf,ucbuf) ; goto skip_dim1 ; } ;
	      v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
	    } ;
	   goto return_point ;
	 } ;
	if (tcb->opcode == V_OpCode_LParen || tcb->opcode == V_OpCode_NCLParen)		/* If we got "(" then assume List:"( ..." */
	 { if ((point->Dim = Dim_List) == 0) PERR2("CmdDimMissing",tcb->type,V4DPI_PntType_List) ;
	   DIMINFO(di,ctx,point->Dim) ; point->PntType = di->PointType ;
	   v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;		/* Push "(" back onto token stack */
	   goto getpointval ;
	 } ;
	if (tcb->opcode == V_OpCode_NCColon)			/* If we got ":" then pull dim from last parsed */
	 { tcb->opcode = V_OpCode_Colon ; goto skip_dim1 ;
	 } ;
	if (tcb->opcode == V_OpCode_NCDot)			/* If we got " ." (space dot) then pull dim from last parsed */
	 { 
	   UCstrcpy(ucdimbuf,dimbuf1) ; tcb->opcode = V_OpCode_Dot ; /* Make opcode a regular dot so we think we got dim.point VEH040513 */
	   goto skip_dim1 ;
	 } ;
	if (tcb->opcode == V_OpCode_LBrace || tcb->opcode == V_OpCode_NCLBrace) /* Got "{" - parse expression } */
	 { LOGICAL gotcc ;
	   quoted = ISQUOTED(point) ; forceeval = point->ForceEval ;
	   TKN(0) ;
	   if (tcb->opcode == V_OpCode_Question) { trace = TRUE ; }
	    else { trace = FALSE ; v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
	   if (!v4dpi_PointParseExp(ctx,point,tcb,(flags | V4DPI_PointParse_MultiLine)))
	    { ucb = errmsg ; v_Msg(ctx,ucb,"DPIPrsBraceExp") ; goto PointParseErr ; } ;
#ifndef NEWQUOTE
	   if (quoted) { QUOTE(point) ; } else { UNQUOTE(point) ; } ;
#endif
	   point->ForceEval = forceeval ; point->TraceEval |= trace ;
	   for(gotcc=FALSE;;)
	    { TKN(V4LEX_Option_RetEOL) ;			/* Look to see if end of Isct or if we got a "," */
	      if (tcb->opcode == V_OpCode_CommaComma)
	       { cpt = (P *)((char *)point + point->Bytes) ; if (!v4dpi_PointParse(ctx,cpt,tcb,flags)) AUXERR ;
	         gotcc = TRUE ; continue ;
	       } ;
	      if (tcb->opcode == V_OpCode_Comma)
	       { cpt = (P *)((char *)point + point->Bytes) ;
		 if (!v4dpi_PointParse(ctx,cpt,tcb,flags)) AUXERR ;
		 if (!gotcc)							/* If got prior ',,' then don't bother saving this */
		  { point->Bytes += cpt->Bytes ; point->Continued = TRUE ; } ;	/* Point continues with another Isct */
		 continue ;
	       } ;
	      if (tcb->opcode == V_OpCode_Slash && ((flags & (V4DPI_PointParse_InColonEq|V4DPI_PointParse_InV4Exp)) == 0))
	       { if (!v4dpi_ParsePointSlashPoint(ctx,point,tcb,flags)) AUXERR ;
	       } else TKN(V4LEX_Option_PushCur) ;
	     break ;
	    } ;
	   goto endofpoint ;
	 } ;
/*	Figure if we got Nid, Dim, or intersection */
	if (tcb->opcode == V_OpCode_LBracket || tcb->opcode == V_OpCode_NCLBracket || tcb->opcode == V_OpCode_LRBracket)	/* Got an intersection */
	 { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
	   if (!v4dpi_IsctParse(&isctp,tcb,ctx,NULL,V_OpCode_RBracket,flags)) AUXERR ;	/* Parse simple/single [...] */
	   quoted = ISQUOTED(point) ; forceeval = point->ForceEval ;
	   memcpy(point,&isctp,isctp.Bytes) ;			/* Copy result of intersection back into whatever */
#ifndef NEWQUOTE
	   if (quoted) { QUOTE(point) ; } else { UNQUOTE(point) ; } ;
#endif
	   point->ForceEval = forceeval ;
	   goto endofpoint ;
	 } ;
#ifdef WANTIMCACHE
	if (tcb->opcode == V_OpCode_Langle && (flags & V4DPI_PointParse_NestedDot) == 0)
	 { tp1.Value.IntVal = V4IM_OpCode_Cache ;
	   tp1.Bytes = (char *)&point->Value.AlphaVal[SIZEofINT] - (char *)point ;
	   if ((tp1.Dim = Dim_IntMod) == 0) PERR2("CmdDimMissing",tcb->type,V4DPI_PntType_IntMod) ;
	   DIMINFO(di,ctx,tp1.Dim) ; tp1.PntType = di->PointType ;
	   if (!v4dpi_IsctParse(&isctp,tcb,ctx,&tp1,V_OpCode_Rangle,flags)) AUXERR ;
	   quoted = point->Quoted ; forceeval = point->ForceEval ;
	   memcpy(point,&isctp,isctp.Bytes) ; point->Quoted = quoted ; point->ForceEval = forceeval ;
	   cpp = ISCT1STPNT(point) ;				/* Scan contents for [-], if so then this is PIntMod */
	   for(n=0;n<point->Grouping;n++)
	    { if (n == 1 && cpp->PntType == V4DPI_PntType_Isct) /* Got an isct within Cache() */
	       cpp->Quoted = TRUE ;
	      ADVPNT(cpp) ;
	    } ;
	   goto endofpoint ;
	 } ;
#endif
	if (tcb->opcode == V_OpCode_DollarLParen || tcb->opcode == V_OpCode_DolDolLParen)
	 { quoted = ISQUOTED(point) ; forceeval = point->ForceEval ;
	   if (!v4dpi_ParseJSONSyntax(ctx,point,tcb,flags,tcb->opcode == V_OpCode_DolDolLParen)) AUXERR ;
//	   point->Quoted = quoted ;
	   point->ForceEval = forceeval ;
	   return(TRUE) ;
	 } ;
	if (tcb->opcode == V_OpCode_DollarSign)
	 { UCCHAR symName[V4LEX_Tkn_KeywordMax+1] ;
	   v4lex_NextTkn(tcb,0) ; if (tcb->type != V4LEX_TknType_Keyword) { PERR1a("DPIMNotKW",0) ; } ;
	   UCstrcpy(symName,tcb->UCstring) ;
	   v4lex_NextTkn(tcb,0) ;
	   if (tcb->opcode == V_OpCode_ColonEqual || tcb->opcode == V_OpCode_PlusEqual || tcb->opcode == V_OpCode_MinusEqual)
	    { int op = (tcb->opcode == V_OpCode_ColonEqual ? 0 : (tcb->opcode == V_OpCode_PlusEqual ? 1 : 2)) ;
	      if (!v4dpi_PointParse(ctx,&tp2,tcb,flags&~V4DPI_PointParse_InColonEq)) AUXERR ;	
	      if (!v4dpi_PushLocalSym(ctx,symName,point,&tp2,op)) AUXERR ;
	    } else
	    { quoted = ISQUOTED(point) ; forceeval = point->ForceEval ;
	      if (tcb->opcode == V_OpCode_LBracket)	/* Do we have $array[subscripts] ? */
	       { if (!v4dpi_ParseArray(point,tcb,ctx,symName,TRUE)) { AUXERR ; } ;
	       } else
	       { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
		 if (!v4dpi_PushLocalSym(ctx,symName,point,NULL,0)) AUXERR ;
		 point->ForceEval = forceeval ;
	       } ;
	      v4lex_NextTkn(tcb,0) ;
	      if (tcb->opcode == V_OpCode_Slash && ((flags & (V4DPI_PointParse_InColonEq|V4DPI_PointParse_InV4Exp)) == 0))
	       { if (!v4dpi_ParsePointSlashPoint(ctx,point,tcb,flags)) AUXERR ; }
	       else v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
	    } ;
	   return(TRUE) ;
	 } ;
	if (tcb->type == V4LEX_TknType_Error) PERR("Syntax error") ;
	if (tcb->type != V4LEX_TknType_Keyword)
	 { 
	   PERR("Not a valid begin-of-point character") ;
	 } ;
	UCstrcpy(ucdimbuf,tcb->UCstring) ;			/* Assume we got dim=value */

	TKN(V4LEX_Option_RetEOL) ;
skip_dim1:
 	if ((tcb->opcode == V_OpCode_LParen || tcb->opcode == V_OpCode_LBrace) && (flags & V4DPI_PointParse_NestedDot) == 0)
	 { LOGICAL isIf ;
	   if ((tp1.Value.IntVal = v4im_Accept(ucdimbuf)) <= 0)	/* Got: xxx( ... - assume calling an "IntMod" */
	    { if (tcb->opcode == V_OpCode_LBrace) goto not_intmod ;	/* Brace begins { exp } point spec */
	      PERR1a("DPIAcpNoIntMod",ucdimbuf) ;
	    } ;
	   tp1.Bytes = (char *)&point->Value.AlphaVal[SIZEofINT] - (char *)point ;
	   if ((tp1.Dim = Dim_IntMod) == 0)
	    { if (errcnt++ > 2) { v_Msg(ctx,UCTBUF1,"CmdNoKernelF") ; vout_UCText(VOUT_Err,0,UCTBUF1) ; exit(EXITABORT) ; } ;
	       PERR2("CmdDimMissing",tcb->type,V4DPI_PntType_IntMod) ;
	    } ;
	   DIMINFO(di,ctx,tp1.Dim) ; tp1.PntType = di->PointType ;
	   isIf = (tp1.Value.IntVal == V4IM_OpCode_If) ;
	   if (!v4dpi_LocalDimAddEndBlock(ctx,TRUE)) AUXERR ;		/* Start new name block */
	   if (!v4dpi_IsctParse(&isctp,tcb,ctx,&tp1,(tcb->opcode == V_OpCode_LParen ? V_OpCode_RParen : V_OpCode_RBrace),flags))
	    { v4dpi_LocalDimAddEndBlock(ctx,FALSE) ; AUXERR ; } ;
	   v4dpi_LocalDimAddEndBlock(ctx,FALSE) ;
	   quoted = ISQUOTED(point) ; forceeval = point->ForceEval ;
	   memcpy(point,&isctp,isctp.Bytes) ;
#ifndef NEWQUOTE
	   if (quoted) { QUOTE(point) ; } else { UNQUOTE(point) ; } ;
#endif
	   point->ForceEval = forceeval ;
	   cpp = ISCT1STPNT(point) ;				/* Scan contents for [-], if so then this is PIntMod */
	   for(n=0;n<point->Grouping;n++)
	    { 
//v_Msg(ctx,UCTBUF1,"@%1d %2P\n",n,cpp) ; vout_UCText(VOUT_Warn,0,UCTBUF1) ;
/*	      Special check here for If() - make sure all args after 1st are one of these points: Then::, Else:: or ElseIf:: */
	      if (n > 1 && isIf == V4IM_OpCode_If && (cpp->PntType != V4DPI_PntType_TagVal || !(cpp->Value.IntVal == V4IM_Tag_Then || cpp->Value.IntVal == V4IM_Tag_Else || cpp->Value.IntVal == V4IM_Tag_ElseIf)))
	       { PERR2("DPIPrsIfThen",n+1,cpp) ; } ;
	      ADVPNT(cpp) ;
	    } ;
	   goto endofpoint ;
	 } ;
	if (tcb->opcode == V_OpCode_LBracket)			/* Got 'xxx[ ...' - convert to Array([xxx] ...) */
	 { UCstrcpy(ucbuf,ucdimbuf) ;				/* ucdimbuf is static - copy into dynamic so it doesn't get trashed in the call */
	   if (!v4dpi_ParseArray(point,tcb,ctx,ucbuf,FALSE)) { AUXERR ; } ;
	   goto endofpoint ;
	 } ;
	if (tcb->opcode == V_OpCode_NCLParen)			/* Got 'xxx<space>( ...' - output warning if we think it may be intmod reference */
	 { if (v4im_Accept(ucdimbuf) > 0)
	    { v_Msg(ctx,UCTBUF1,"@*%0F Detected possible internal module (%1U) followed by non-contiguous open parenthesis; remove space(s) for module reference\n",ucdimbuf) ; vout_UCText(VOUT_Warn,0,UCTBUF1) ;
	    } ;
	 } ;
not_intmod:
/*	i is TRUE if got star, if in { ... } then star has to be smack up against dim name (if spaces then 2 * is mult()) */
	if (tcb->opcode == V_OpCode_Star)			/* Got: xxx* - convert to xxx:{*} */
	 { if (gpi->vltCur != NULL)				/* If processing table then see if got a column spec */
	    { 
	      for(i=0;i<gpi->vltCur->Columns;i++)
	       { if (UCstrcmpIC(gpi->vltCur->Col[i].Name,ucdimbuf) == 0) break ; } ;
	      if (i < gpi->vltCur->Columns)				/* Found it - copy current value into point */
	       { if (gpi->vltCur->Col[i].Cur == NULL || (gpi->vltCur->InXML ? (gpi->vltCur->Col[i].XMLFlags & V4LEX_Table_XML_Defined)==0 : FALSE))
		  PERR4("CmdPntColUnd",tcb->type,i+1,ucdimbuf,gpi->vltCur->Name) ;
		 memcpy(point,gpi->vltCur->Col[i].Cur,gpi->vltCur->Col[i].Cur->Bytes) ;
		 goto return_point ;
	       } ;
	    } ;
	   if ((point->Dim = v4dpi_DimGet(ctx,ucdimbuf,DIMREF_REF)) == 0)
	    { ucb = errmsg ; v_Msg(ctx,errmsg,"DPIPrsNotDim",ucdimbuf) ; goto PointParseErr ; } ;
	   point->Grouping = V4DPI_Grouping_Current ; point->PntType = V4DPI_PntType_Special ;
	   point->Bytes = V4DPI_PointHdr_Bytes ;
	   TKN(V4LEX_Option_RetEOL) ;
	   if (tcb->opcode == V_OpCode_CommaComma)
	    { cpt = (P *)((char *)point + point->Bytes) ;
	      if (!v4dpi_PointParse(ctx,cpt,tcb,flags))
	       { ucb = errmsg ; UCstrcpy(ucb,ctx->ErrorMsgAux) ; tcb->type = UNUSED ; goto PointParseErr ;} ;
	    }
	    else if (tcb->opcode == V_OpCode_Comma)
	    { point->Continued = TRUE ;			/* Point continues with another Isct */
	      cpt = (P *)((char *)point + point->Bytes) ;
	      if (!v4dpi_PointParse(ctx,cpt,tcb,flags))
	       { ucb = errmsg ; UCstrcpy(ucb,ctx->ErrorMsgAux) ; tcb->type = UNUSED ; goto PointParseErr ;} ;
	      point->Bytes += cpt->Bytes ;
	    } else if (tcb->opcode == V_OpCode_Slash && ((flags & (V4DPI_PointParse_InColonEq|V4DPI_PointParse_InV4Exp)) == 0))
	    { if (!v4dpi_ParsePointSlashPoint(ctx,point,tcb,flags)) AUXERR ;
	    } else v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
	   goto endofpoint ;
	 } ;
	if (tcb->opcode == V_OpCode_DashRangle && (flags & V4DPI_PointParse_NestedDot) == 0)
	 { if (!v4dpi_ParsePointDashRangle(ctx,point,tcb,flags)) AUXERR ; goto endofpoint ; } ;
	if (tcb->opcode == V_OpCode_StarStar)				/* Got dim** */
	 { if ((point->Dim = v4dpi_DimGet(ctx,ucdimbuf,DIMREF_REF)) == 0) PERR1a("CmdNotDim",ucdimbuf) ;
	    point->Grouping = V4DPI_Grouping_PCurrent ; point->PntType = V4DPI_PntType_Special ;
	    goto endofpoint ;
	 } ;
	if (tcb->opcode == V_OpCode_Possessive) 			/* Got 's - convert to dot or dotdot */
	 { tcb->opcode = ((flags & V4DPI_PointParse_LHS) ? V_OpCode_DotDot : V_OpCode_Dot) ; } ;
/*	Don't want to do this if parsing $(x.y.z) */
	if (((flags & V4DPI_PointParse_InJSON) == 0) && (tcb->opcode == V_OpCode_Dot /* || tcb->opcode == V_OpCode_NCDot */) && (flags & V4DPI_PointParse_NestedDot) == 0)	/* Got xxx.yyy - convert to [xxx* yyy] */
	 {
	   ZPH(&tp1) ;
	   if ((tp1.Dim = v4dpi_DimGet(ctx,ucdimbuf,DIMREF_REF)) == 0)  PERR1a("CmdNotDim",ucdimbuf) ;
	   UCstrcpy(dimbuf1,ucdimbuf) ;				/* Save dimension for possible ".xxx" later on */
	   tp1.PntType = V4DPI_PntType_Special ;
	   tp1.Grouping = V4DPI_Grouping_Current ;		/* Convert dim name to point: dim* */
	   tp1.Bytes = V4DPI_PointHdr_Bytes ;
	   if (*tcb->ilvl[tcb->ilx].input_ptr == UClit('?'))		/* Have '?' immediately after the dot ? */
	    { trace = TRUE ; tcb->ilvl[tcb->ilx].input_ptr++ ;
	    } else { trace = FALSE ; } ;
	   if(!v4dpi_PointParse(ctx,&tp2,tcb,flags | V4DPI_PointParse_NestedDot)) AUXERR ;	/* Parse point after "." */
	   quoted = ISQUOTED(point) ; forceeval = point->ForceEval ; ZPH(point) ; nextfree = 0 ;

	   INITISCT(point) ; ISCTVCD(point) ;
	   
/*	   See if got list.point - expand to List(list* Nth::point) IFF ok & point <> Dictionary or XDict */
	   DIMINFO(di,ctx,tp1.Dim) ;				/* Get dimension info */
	   if (di->PointType == V4DPI_PntType_List && ((di->Flags & V4DPI_DimInfo_DotIndex) != 0)
		&& (tp2.Dim == Dim_Dim || !(tp2.PntType == V4DPI_PntType_XDict)))	/* Got a list - then handle special */
	    {							/* Convert to List(tp1 Nth::tp2) */
	      cpt = ISCT1STPNT(point) ;
	      ZPH(cpt) ; cpt->PntType = V4DPI_PntType_IntMod ; cpt->Bytes = V4PS_Int ; cpt->Dim = Dim_IntMod ;
	      cpt->Value.IntVal = V4IM_OpCode_List ;
	      point->Bytes += cpt->Bytes ; ADVPNT(cpt) ;
	      memcpy(cpt,&tp1,tp1.Bytes) ;			/* Copy the list* point */
	      point->Bytes += tp1.Bytes ; ADVPNT(cpt) ;
	      ZPH(&tp1) ; tp1.Dim = Dim_Tag ; tp1.PntType = V4DPI_PntType_TagVal ;
	      tv = (struct V4DPI__TagVal *)&tp1.Value ;
	      tv->TagVal = v4im_TagValue(tp2.PntType == V4DPI_PntType_Dict && tp2.Dim != Dim_Dim ? UClit("Key") : UClit("Nth")) ; ; memcpy(&tv->TagPt,&tp2,tp2.Bytes) ;
	      tp1.Bytes = (char *)&tv->TagPt + tp2.Bytes - (char *)&tp1 ;
	      memcpy(cpt,&tp1,tp1.Bytes) ; point->Bytes += tp1.Bytes ;
	      point->Grouping = 3 ;
#ifndef NEWQUOTE
	      if (quoted) { QUOTE(point) ; } else { UNQUOTE(point) ; } ;
#endif
	      point->ForceEval = forceeval ;
	      goto endofpoint ;
	    } else if (di->PointType == V4DPI_PntType_List && ((di->Flags & V4DPI_DimInfo_DotList) != 0) && tp2.PntType != V4DPI_PntType_Int)
	    { struct V4L__ListPoint *lp ;
	      
	      cpt = ISCT1STPNT(point) ; dictPNTv(cpt,Dim_UV4,v4im_GetEnumToDictVal(ctx,(enum DictionaryEntries)_Eval,Dim_UV4)) ;
	      point->Bytes += cpt->Bytes ; ADVPNT(cpt) ;
/*	      Now make a list of all following points until no longer have anything left */
	      INITLP(cpt,lp,di->DimId) ;
/*	      First point after dot already parsed in tp2 - append it */
	      if (tp2.PntType != V4DPI_PntType_Dict) PERR("Invalid DotList syntax") ;
	      if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp2,0)) AUXERR ;
	      for(;;)
	       { v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
	         if (tcb->opcode == V_OpCode_LBracket)
	          { P *dlpt,*tpt ;
	            struct V4L__ListPoint *lpp ;
	            if(!v4dpi_PointParse(ctx,&tp1,tcb,flags)) AUXERR ;
	            v4lex_NextTkn(tcb,0) ;
/*		    If just a single point enclosed in brackets then append as another point in main list */
	            if (tcb->opcode == V_OpCode_RBracket) { if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp1,0)) AUXERR ; continue ; } ;
/*		    Have something more complex (ex: [point = value]) - parse it as isct of form: [UV4:DotListSubScript dim:(arguments)] */
		    dlpt = &isctp ; INITISCT(dlpt) ; ISCTVCD(dlpt) ; dlpt->Grouping = 2 ;
		    tpt = ISCT1STPNT(dlpt) ;
/*		    Create first argument - UV4:ParseRel */
		    dictPNTv(tpt,Dim_UV4,v4im_GetEnumToDictVal(ctx,DE(ParseRelation),Dim_UV4)) ; ;
		    dlpt->Bytes += tpt->Bytes ; ADVPNT(tpt) ;
/*		    Now second element - a list of everything within the brackets */
		    INITLP(tpt,lpp,di->DimId) ;
/*		    First is the point we just parsed */
		    if (tp1.PntType == V4DPI_PntType_Isct || tp1.PntType == V4DPI_PntType_Special) { QUOTE(&tp1) ; } ;
		    if (!v4l_ListPoint_Modify(ctx,lpp,V4L_ListAction_Append,&tp1,0)) AUXERR ;
	            for (;;)
	             { if (tcb->opcode < V_Opcode_Starting || tcb->opcode > V_Opcode_Ending) PERR("Invalid DotList syntax") ;
/*			  Next is the opcode */
			  dictPNTv(&tp1,Dim_UV4,v4im_GetEnumToDictVal(ctx,opcodeDE[tcb->opcode-1],Dim_UV4)) ;
			  if (!v4l_ListPoint_Modify(ctx,lpp,V4L_ListAction_Append,&tp1,0)) AUXERR ;
/*			  There should be another argument */
			  if(!v4dpi_PointParse(ctx,&tp1,tcb,flags)) AUXERR ;
			  if (tp1.PntType == V4DPI_PntType_Isct || tp1.PntType == V4DPI_PntType_Special) { QUOTE(&tp1) ; } ;
			  if (!v4l_ListPoint_Modify(ctx,lpp,V4L_ListAction_Append,&tp1,0)) AUXERR ;
/*			  And finally the closing bracket */
			  v4lex_NextTkn(tcb,0) ; if (tcb->opcode == V_OpCode_RBracket) break ;
			}
		    ENDLP(tpt,lpp) ; dlpt->Bytes += tpt->Bytes ;
/*		    Now evaluate it and if successful append result in main list */
		    tpt = v4dpi_IsctEval(&tp1,dlpt,ctx,0,NULL,NULL) ;
		    if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,(tpt == NULL ? dlpt : tpt),0)) AUXERR ;
		    continue ;
	          } ;
	         if (tcb->opcode != V_OpCode_Dot) { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; break ; } ;
	         v4lex_NextTkn(tcb,0) ;
	         if (tcb->type == V4LEX_TknType_Keyword)
	          { ZPH(&tp1) ; tp1.Dim = Dim_NId ; tp1.PntType = V4DPI_PntType_Dict ; tp1.Bytes = V4PS_Int ;
	            tp1.Value.IntVal = v4dpi_DictEntryGet(ctx,0,tcb->UCstring,di,NULL) ;
		    if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp1,0)) AUXERR ;
	          } else { PERR("Invalid DotList syntax") ; } ;
	       } ;
	      ENDLP(cpt,lp) ; point->Bytes += cpt->Bytes ; point->Grouping = 2 ;
#ifndef NEWQUOTE
	      if (quoted) { QUOTE(point) ; } else { UNQUOTE(point) ; } ;
#endif
	      point->ForceEval = forceeval ;
/*	      Look for possible ,xxx */
	      v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
	      if (tcb->opcode == V_OpCode_CommaComma)
	       { if(!v4dpi_PointParse(ctx,&tp1,tcb,flags)) AUXERR ;
	       } else if (tcb->opcode == V_OpCode_Comma)
	       { point->Continued = TRUE ;			/* Point continues with another Isct */
		 cpt = point ; ADVPNT(cpt) ;			/* Figure out where next Isct to begin */
		 if(!v4dpi_PointParse(ctx,cpt,tcb,flags)) AUXERR ;
		 point->Bytes += cpt->Bytes ;
	       } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
	      goto endofpoint ;
	    } else
	    {
	      memcpy(&point->Value.Isct.pntBuf[nextfree],&tp1,tp1.Bytes) ;	/* Append first point to ISCT */
	      ISCTSETNESTED(point,&tp1) ;
	      nextfree += tp1.Bytes ; point->Grouping ++ ;
	      memcpy(&point->Value.Isct.pntBuf[nextfree],&tp2,tp2.Bytes) ;	/* Then append second point */
	      ISCTSETNESTED(point,&tp2) ;
	      nextfree += tp2.Bytes ; point->Grouping ++ ;
	      point->Bytes = (char *)&point->Value.Isct.pntBuf[nextfree] - (char *)point ;
	    } ;
	   for(;;)
	    { TKN(V4LEX_Option_RetEOL) ;
	      if (tcb->opcode == V_OpCode_Dot)			/* Got another "." ? */
	       { memcpy(&tp1,point,point->Bytes) ;
		 if(!v4dpi_PointParse(ctx,&tp2,tcb,flags|V4DPI_PointParse_NestedDot)) AUXERR ;	/* Parse point after "." */
		 INITISCT(point) ; ISCTVCD(point) ; nextfree = 0 ;
		 memcpy(&point->Value.Isct.pntBuf[nextfree],&tp1,tp1.Bytes) ;	/* Append first point to ISCT */
		 point->NestedIsct = TRUE ; nextfree += tp1.Bytes ; point->Grouping ++ ;
		 memcpy(&point->Value.Isct.pntBuf[nextfree],&tp2,tp2.Bytes) ;	/* Then append second point */
		 nextfree += tp2.Bytes ; point->Grouping ++ ;
		 point->Bytes = (char *)&point->Value.Isct.pntBuf[nextfree] - (char *)point ;
		 continue ;
	       } ;
	      if (tcb->opcode == V_OpCode_DashRangle)
	       { if (!v4dpi_ParsePointDashRangle(ctx,point,tcb,flags)) AUXERR ; break ; } ;
	      if (tcb->opcode == V_OpCode_CommaComma)
	       { cpt = (P *)&point->Value.Isct.pntBuf[nextfree] ;	/* Figure out where next Isct to begin (but not going to save it) */
		 if(!v4dpi_PointParse(ctx,cpt,tcb,flags)) AUXERR ;
		 continue ;
	       } ;
	      if (tcb->opcode == V_OpCode_Comma)
	       { point->Continued = TRUE ;			/* Point continues with another Isct */
		 cpt = (P *)&point->Value.Isct.pntBuf[nextfree] ;	/* Figure out where next Isct to begin */
		 if(!v4dpi_PointParse(ctx,cpt,tcb,flags)) AUXERR ;
		 point->Bytes += cpt->Bytes ; nextfree += cpt->Bytes ;
		 continue ;
	       } ;
	      if (tcb->opcode == V_OpCode_Not)		/* Did we get a "!" */
	       { point->CondEval = TRUE ; 
		 break ;		 
	       } ;
	      if (tcb->opcode == V_OpCode_Slash && ((flags & (V4DPI_PointParse_InColonEq|V4DPI_PointParse_InV4Exp)) == 0))
	       { if (!v4dpi_ParsePointSlashPoint(ctx,point,tcb,flags)) AUXERR ;
	       } else v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
	     break ;
	    } ;
#ifndef NEWQUOTE
	   if (quoted) { QUOTE(point) ; } else { UNQUOTE(point) ; } ;
#endif
	   point->ForceEval = forceeval ; point->TraceEval = trace ;
	   goto endofpoint ;
	 } ;
	if (tcb->opcode == V_OpCode_ColonEqual) 		/* Got: dim:=point */
	 { int gotlog = FALSE ; int tf = FALSE ; P *ip ;
	   if ((dimid = v4dpi_DimGet(ctx,ucdimbuf,DIMREF_REF)) == 0)
	    { if (UCstrcmp(ucdimbuf,UClit("TRUE")) == 0) { gotlog = TRUE ; tf = TRUE ; }
	       else if (UCstrcmp(ucdimbuf,UClit("FALSE")) == 0) { gotlog = TRUE ; tf = FALSE ; }
	       else { dimid = v4dpi_DictEntryGet(ctx,0,ucdimbuf,DIMDICT,NULL) ;
		      if (dimid <= 0 || dimid > 0xffff) PERR1a("CmdNotDim",ucdimbuf) ;
		    } ;
	    } ;
	   quoted = ISQUOTED(point) ; forceeval = point->ForceEval ; trace = point->TraceEval ;
	   INITISCT(point) ; ISCTVCD(point) ; point->Grouping = 3 ; ip = ISCT1STPNT(point) ;
	   ZPH(ip) ; ip->PntType = V4DPI_PntType_IntMod ; ip->Dim = Dim_IntMod ; ip->Value.IntVal = V4IM_OpCode_Project ; ip->Bytes = V4PS_Int ;
	   point->Bytes += ip->Bytes ;
	   ADVPNT(ip) ; 
	   if (gotlog) { logPNTv(ip,tf) ; } else { dictPNTv(ip,Dim_Dim,dimid) ; } ;
	   point->Bytes += ip->Bytes ; ADVPNT(ip) ;
	   if(!v4dpi_PointParse(ctx,ip,tcb,flags|V4DPI_PointParse_InColonEq)) AUXERR ;
	   point->Bytes += ip->Bytes ;
	   TKN(V4LEX_Option_RetEOL) ;
	   if (tcb->opcode == V_OpCode_Slash && ((flags & (V4DPI_PointParse_InColonEq|V4DPI_PointParse_InV4Exp)) == 0))
	    { if (!v4dpi_ParsePointSlashPoint(ctx,point,tcb,flags)) AUXERR ;
	    } else v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
#ifndef NEWQUOTE
	   if (quoted) { QUOTE(point) ; } else { UNQUOTE(point) ; } ;
#endif
	   point->ForceEval = forceeval ; point->TraceEval = trace ;
	   goto endofpoint ;
	 } ;
	if (tcb->opcode == V_OpCode_ColonColon || tcb->opcode == V_OpCode_ColonColonColon)
	 { TAGVAL tn = v4im_TagValue(ucdimbuf) ;			/* Got dim::pt - convert "dim" to tag number */
	   if (tn <= 0) PERR1a("CmdNotTag",ucdimbuf) ;
	   if (tcb->opcode == V_OpCode_ColonColonColon) tn |= V4DPI_TagFlag_Colon3 ;
	   point->Dim = Dim_Tag ; point->PntType = V4DPI_PntType_TagVal ;
	   DIMINFO(di,ctx,point->Dim) ;
	   if(!v4dpi_PointParse(ctx,&tp1,tcb,flags)) AUXERR ;		/* Parse tagged point */
	   tv = (struct V4DPI__TagVal *)&point->Value ;
	   tv->TagVal = tn ; memcpy(&tv->TagPt,&tp1,tp1.Bytes) ;
	   point->Bytes = (char *)&tv->TagPt + tp1.Bytes - (char *)point ;
	   goto endofpoint ;
	 } ;
	if (tcb->opcode == V_OpCode_DotDot)			/* Got: xxx.. - convert to xxx:{all} */
	 { if ((point->Dim = v4dpi_DimGet(ctx,ucdimbuf,DIMREF_REF)) == 0)  PERR1a("CmdNotDim",ucdimbuf) ;
	    point->Grouping = V4DPI_Grouping_All ; point->PntType = V4DPI_PntType_Special ;
	    goto endofpoint ;
	 } ;
	if (tcb->opcode == V_OpCode_DotDotDot)			/* Got: xxx... - convert to xxx:{all} */
	 { if ((flags & V4DPI_PointParse_LHS) == 0) { ucb = errmsg ; v_Msg(ctx,ucb,"DPIPrsNotLHS") ; goto PointParseErr ; } ;
	   if ((point->Dim = v4dpi_DimGet(ctx,ucdimbuf,DIMREF_REF)) == 0)  PERR1a("CmdNotDim",ucdimbuf) ;
	    point->Grouping = V4DPI_Grouping_AllCnf ; point->PntType = V4DPI_PntType_Special ;
	    goto endofpoint ;
	 } ;
	if (tcb->opcode == V_OpCode_Plus && (flags & (V4DPI_PointParse_NestedDot|V4DPI_PointParse_InV4Exp)) == 0)
	 { point->Dim = v4dpi_DimGet(ctx,ucdimbuf,DIMREF_REF) ;			/* Got: xxx+ - Convert to new reference */
	   di = (point->Dim == 0 ? NULL : v4dpi_DimInfoGet(ctx,point->Dim)) ;
	   if (di == NULL)  PERR1a("CmdNotDim",ucdimbuf) ;
	   if (!di->UniqueOK) PERR1a("DPIDimNoNew",di->DimId) ;
	   point->Grouping = V4DPI_Grouping_New ; point->PntType = V4DPI_PntType_Special ;
	   goto endofpoint ;
	 } ;
	if (tcb->opcode == V_OpCode_Tilde && (flags & (V4DPI_PointParse_NestedDot|V4DPI_PointParse_InV4Exp)) == 0)
	 { point->Dim = v4dpi_DimGet(ctx,ucdimbuf,DIMREF_REF) ;			/* Got: xxx~ - Convert to new reference */
	   di = (point->Dim == 0 ? NULL : v4dpi_DimInfoGet(ctx,point->Dim)) ;
	   if (di == NULL)  PERR1a("CmdNotDim",ucdimbuf) ;
	   point->Grouping = V4DPI_Grouping_Undefined ; point->PntType = V4DPI_PntType_Special ;
	   goto endofpoint ;
	 } ;
	if (tcb->opcode == V_OpCode_Question)
	 { point->Dim = v4dpi_DimGet(ctx,ucdimbuf,DIMREF_REF) ;
	   di = (point->Dim == 0 ? NULL : v4dpi_DimInfoGet(ctx,point->Dim)) ;
	    { TAGVAL tn = v4im_TagValue(ucdimbuf) ;			/* Got dim::pt - convert "dim" to tag number */
	      if (tn <= 0) PERR1a("CmdNotTag",ucdimbuf) ;
	      point->Dim = Dim_Tag ; point->PntType = V4DPI_PntType_TagVal ;
	      point->ForceEval = TRUE ; 				/* Flag point as "?" */
	      DIMINFO(di,ctx,point->Dim) ;
	      tv = (struct V4DPI__TagVal *)&point->Value ; tv->TagVal = tn ;
	      point->Bytes = (char *)&tv->TagPt - (char *)point ;
	      goto endofpoint ;
	    } ;
	 } ;
	if (tcb->type != V4LEX_TknType_Punc ? TRUE : tcb->opcode != V_OpCode_Colon)
	 { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;				/* Did not get "=", parse as NId */
	   if ((i=UCstrcmp(ucdimbuf,UClit("TRUE"))) == 0 || UCstrcmp(ucdimbuf,UClit("FALSE")) == 0)
	    { if ((point->Dim = Dim_Logical) == 0)
	       { if (errcnt++ > 2) { v_Msg(ctx,UCTBUF1,"CmdNoKernelF") ; vout_UCText(VOUT_Err,0,UCTBUF1) ; exit(EXITABORT) ; } ;
	         PERR2("CmdDimMissing",tcb->type,V4DPI_PntType_Logical) ;
	       } ;
	      point->Value.IntVal = (i==0) ; point->PntType = V4DPI_PntType_Logical ; point->Bytes = V4PS_Int ;
	      goto endofpoint ;
	    } ;
	   if (UCstrcmp(ucdimbuf,UClit("FAIL")) == 0)		/* Convert to NOp(0) */
	    { quoted = ISQUOTED(point) ; INITISCT(point) ; ISCTVCD(point) ; point->Grouping = 2 ; nextfree = 0 ;
	      cpt = ISCT1STPNT(point) ; ZPH(cpt) ; cpt->PntType = V4DPI_PntType_IntMod ; cpt->Bytes = V4PS_Int ;
	      cpt->Dim = Dim_IntMod ;  cpt->Value.IntVal = V4IM_OpCode_NOp ; point->Bytes += cpt->Bytes ;
	      ADVPNT(cpt) ; intPNTv(cpt,0) ; point->Bytes += cpt->Bytes ;
#ifndef NEWQUOTE
	      if (quoted) { QUOTE(point) ; } else { UNQUOTE(point) ; } ;
#endif
	      goto endofpoint ;
	    } ;
	   if ((point->Dim = Dim_NId) == 0)
	    { if (errcnt++ > 2) { v_Msg(ctx,UCTBUF1,"CmdNoKernelF") ; vout_UCText(VOUT_Err,0,UCTBUF1) ; exit(EXITABORT) ; } ;
	       PERR2("CmdDimMissing",tcb->type,V4DPI_PntType_Dict) ;
	    } ;
	   DIMINFO(di,ctx,point->Dim) ;
	   if ((point->Value.IntVal = v4dpi_DictEntryGet(ctx,0,ucdimbuf,di,&dimid)) == 0)  PERR2("CmdDictNotFnd",tcb->type,ucdimbuf) ;
#ifdef CHECKCLASSICMODE
	   if (dimid != 0 && dimid != di->DimId)		/* Got a preferred dimension that is not Dim:NId ? */
	    { if (gpi->ClassicMode)
	       { point->Dim = dimid ; DIMINFO(di,ctx,dimid) ;	/* Yes */
	       } else
	       { v_Msg(ctx,UCTBUF1,"ParseAutoDim",di->DimId,ucdimbuf,dimid) ; vout_UCText(VOUT_Warn,0,UCTBUF1) ;
	       } ;
	    } ;
#endif
	   point->PntType = di->PointType ; point->Bytes = V4PS_Int ;
	   goto endofpoint ;
	 } ;
/*	Got "dim:", now parse one or more points */
	if ((point->Dim = v4dpi_DimGet(ctx,ucdimbuf,DIMREF_REF)) == 0)
	 { struct V4L__ListPoint *lp ; struct V4DPI__DimInfo *di ; enum DictionaryEntries deval ;
	   if ((flags & V4DPI_PointParse_NotADim) == 0)
	    { PERR1a("CmdNotDim",ucdimbuf) ; } ;
/*	   Convert to a list (name rel value) */
	   INITLP(point,lp,Dim_List) ;
	   DIMINFO(di,ctx,Dim_NId) ; dictPNTv(&tp1,Dim_NId,v4dpi_DictEntryGet(ctx,0,ucdimbuf,di,NULL)) ;
	   if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp1,0)) return(FALSE) ;



	   deval = DE(EQ) ;				/* Starting with assumption of '=' relation */
	   TKN(0) ;
	   if (tcb->type == V4LEX_TknType_Punc)
	    { switch (tcb->opcode)
	       { default:			v_Msg(ctx,ctx->ErrorMsgAux,"DPIInvRel",tcb->type) ; return(FALSE) ;
		 case V_OpCode_Rangle:		deval = DE(GT) ; break ;
		 case V_OpCode_RangleEqual:	deval = DE(GE) ; break ;
		 case V_OpCode_Langle:		deval = DE(LT) ; break ;
		 case V_OpCode_LangleDash:	/* '<-' parsed as single token, have to play games here */
		    TKN(V4LEX_Option_PushCurHard) ;	/* Reset token pointer & then skip over the '<' */
		    for(;*tcb->ilvl[tcb->ilx].input_ptr!=UClit('<');tcb->ilvl[tcb->ilx].input_ptr++) { } ;
		    deval = DE(LT) ;
		    break ;
		 case V_OpCode_LangleEqual:	deval = DE(LE) ; break ;
		 case V_OpCode_LangleRangle:	deval = DE(NE) ; break ;
	       } ;
	    } else { TKN(V4LEX_Option_PushCurHard) ; } ;
	   TKN(V4LEX_Option_NextChunkExt) ;
	   dictPNTv(&tp1,Dim_UV4,v4im_GetEnumToDictVal(ctx,deval,Dim_UV4)) ;
	   if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp1,0)) return(FALSE) ;
	   uccharPNTv(&tp1,tcb->UCstring) ; if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp1,0)) return(FALSE) ;
	   ENDLP(point,lp) ;
	   return(TRUE) ;







#ifdef MOOOOOOOO
	   TKN(V4LEX_Option_NextChunk) ;
	   switch (tcb->type)
	    { default:
	      case V4LEX_TknType_Error:
	      case V4LEX_TknType_String:
	      case V4LEX_TknType_Keyword:
		dictPNTv(&tp1,Dim_UV4,v4im_GetEnumToDictVal(ctx,DE(EQ),Dim_UV4)) ;
		if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp1,0)) return(FALSE) ;
		uccharPNTv(&tp1,tcb->UCstring) ; if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp1,0)) return(FALSE) ;
		break ;
	      case V4LEX_TknType_Integer:
		dictPNTv(&tp1,Dim_UV4,v4im_GetEnumToDictVal(ctx,DE(EQ),Dim_UV4)) ;
		if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp1,0)) return(FALSE) ;
/*		Repush token & parse as Int:nnn (to properly handle various permutations such as Int:num..num) */
		TKN(V4LEX_Option_PushCurHard) ; v4lex_NestInput(tcb,NULL,UClit("Int:"),V4LEX_InpMode_String) ;
		v4dpi_PointParse(ctx,&tp1,tcb,flags) ; if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp1,0)) return(FALSE) ;
		break ;
	      case V4LEX_TknType_Float:
		dictPNTv(&tp1,Dim_UV4,v4im_GetEnumToDictVal(ctx,DE(EQ),Dim_UV4)) ;
		if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp1,0)) return(FALSE) ;
		TKN(V4LEX_Option_PushCur) ; v4lex_NestInput(tcb,NULL,UClit("Num:"),V4LEX_InpMode_String) ;
		v4dpi_PointParse(ctx,&tp1,tcb,flags) ; if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp1,0)) return(FALSE) ;
		break ;
	      case V4LEX_TknType_LInteger:
		dictPNTv(&tp1,Dim_UV4,v4im_GetEnumToDictVal(ctx,DE(EQ),Dim_UV4)) ;
		if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp1,0)) return(FALSE) ;
		TKN(V4LEX_Option_PushCur) ; v4lex_NestInput(tcb,NULL,UClit("UFix:"),V4LEX_InpMode_String) ;
		v4dpi_PointParse(ctx,&tp1,tcb,flags) ; if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp1,0)) return(FALSE) ;
		break ;
	      case V4LEX_TknType_Punc:
		{ enum DictionaryEntries deval ; LOGICAL minus=FALSE ;
		  switch (tcb->opcode)
		   { default:				v_Msg(ctx,ctx->ErrorMsgAux,"DPIInvRel",tcb->type) ; return(FALSE) ;
		     case V_OpCode_Rangle:		deval = DE(GT) ; break ;
		     case V_OpCode_RangleEqual:		deval = DE(GE) ; break ;
		     case V_OpCode_Langle:		deval = DE(LT) ; break ;
		     case V_OpCode_LangleDash:			/* '<-' parsed as single token, have to play games here */
			deval = DE(LT) ;
			switch(point->PntType)
			 { default:	PERR("Invalid use of minus/dash ('-')") ;
			   case V4DPI_PntType_Int:
			   case V4DPI_PntType_Real:
			   case V4DPI_PntType_Fixed:
			   case V4DPI_PntType_CodedRange:	minus = TRUE ; break ;
			 } ;
			break ;
		     case V_OpCode_LangleEqual:		deval = DE(LE) ; break ;
		     case V_OpCode_LangleRangle:	deval = DE(NE) ; break ;
		   } ;
		  dictPNTv(&tp1,Dim_UV4,v4im_GetEnumToDictVal(ctx,deval,Dim_UV4)) ;
		  if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp1,0)) return(FALSE) ;
		  TKN(0) ;
		   switch (tcb->type)
		    { default:				v_Msg(ctx,ctx->ErrorMsgAux,"DPIAlphaNum",tcb->type) ; return(FALSE) ;
		      case V4LEX_TknType_Error:
		      case V4LEX_TknType_String:
		      case V4LEX_TknType_Keyword:
			uccharPNTv(&tp1,tcb->UCstring) ; if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp1,0)) return(FALSE) ;
			break ;
		      case V4LEX_TknType_Integer:
//			TKN(V4LEX_Option_PushCur) ;
			intPNTv(&tp1,(minus ? -tcb->integer : tcb->integer)) ; if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp1,0)) return(FALSE) ;
			break ;
		      case V4LEX_TknType_Float:
		        if (minus) tcb->floating = -tcb->floating ;
			dblPNTv(&tp1,tcb->floating) ; if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp1,0)) return(FALSE) ;
			break ;
		      case V4LEX_TknType_LInteger:
		        if (minus) tcb->Linteger = -tcb->Linteger ;
			fixPNTv(&tp1,tcb->Linteger) ; if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&tp1,0)) return(FALSE) ;
			break ;
		    } ;
		}
		break ;
	    }
	   ENDLP(point,lp) ;
	   return(TRUE) ;
#endif
	 } else
	 { DIMINFO(di,ctx,point->Dim) ; 		/* Have real dimension - Link up to dimension info */
	 } ;
	point->PntType = di->PointType ;
getpointval:
	for(;;)
	 { minus = FALSE ;
	   switch(point->PntType)
	    { case V4DPI_PntType_Shell: 	break ; 	/* Can't have relationals if dim is a shell */
	      case V4DPI_PntType_BigText:	break ;
	      case V4DPI_PntType_Int2:		break ;
	      default:
	        if ((di->ds.Alpha.aFlags & V4DPI_DimInfoAlpha_IsFileName) != 0 && point->PntType == V4DPI_PntType_Char) goto no_rel ;	/* If a file name then can't be this stuff */
		TKN(V4LEX_Option_RetEOL|V4LEX_Option_ForceKW) ;
		switch (tcb->opcode)				/* Look for relational operator */
		 { default:	v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; goto no_rel ; /* Not a relational */
		   case V_OpCode_Rangle:	gotrel = V4DPI_Grouping_GT ; break ;
		   case V_OpCode_RangleEqual:	gotrel = V4DPI_Grouping_GE ; break ;
		   case V_OpCode_Langle:	gotrel = V4DPI_Grouping_LT ; break ;
		   case V_OpCode_LangleDash:			/* '<-' parsed as single token, have to play games here */
			gotrel = V4DPI_Grouping_LT ;
			switch(point->PntType)
			 { default:	PERR("Invalid use of minus/dash ('-')") ;
			   case V4DPI_PntType_Int:
			   case V4DPI_PntType_Real:
			   case V4DPI_PntType_Fixed:
			   case V4DPI_PntType_CodedRange:	minus = TRUE ; break ;
			 } ;
			break ;
		   case V_OpCode_LangleEqual:	gotrel = V4DPI_Grouping_LE ; break ;
		   case V_OpCode_LangleRangle:	gotrel = V4DPI_Grouping_NE ; break ;
		 } ;
//veh090317		if ((di->Flags & (V4DPI_DimInfo_RangeOK | V4DPI_DimInfo_ListOK)) == 0) PERR("Range of values not allowed for this dimension") ;
	   } ;
no_rel:
/*	   Get the first point (and maybe the last) */
	   if (!v4dpi_PointAccept(&tp1,di,tcb,ctx,(flags | (minus ? V4LEX_Option_MakeNeg : 0))))
	    { ucb = errmsg ; ctx->ErrorMsgAux[UCsizeof(errmsg)- 2] = UCEOS ; UCstrcpy(ucb,ctx->ErrorMsgAux) ; tcb->type = UNUSED ; goto PointParseErr ;} ;
/*	   Point different type - probably through special acceptor - take as is */
	   if (tp1.PntType != di->PointType && ((di->Flags & (V4DPI_DimInfo_Acceptor | V4DPI_DimInfo_ValueList | V4DPI_DimInfo_ValueTree)) || tp1.PntType == V4DPI_PntType_SegBitMap))
	    { memcpy(point,&tp1,tp1.Bytes) ; goto return_point ; } ;
/*	   If di->PointType is Alpha and tp1.PntType is UCChar then return tp1 as point (veh201012) */
	   if ((di->PointType == V4DPI_PntType_Char || di->PointType == V4DPI_PntType_UCChar) && (tp1.PntType == V4DPI_PntType_UCChar || tp1.PntType == V4DPI_PntType_BigText))
	    { memcpy(point,&tp1,tp1.Bytes) ; goto return_point ; } ;
	   if (tp1.PntType == V4DPI_PntType_Special) point->PntType = V4DPI_PntType_Special ;
/*	   If we are followed by ".." then get next point */
check_continue:
	   TKN(V4LEX_Option_RetEOL) ;
	   if (tcb->opcode == V_OpCode_DotDot)
	    { //if (tp1.Grouping > 0)     PERR("Cannot have range of points when first is, itself, a range") ;
	      if (tp1.Grouping > 0) tp1.Grouping = 0 ;
	      if (!v4dpi_PointAccept(&tp2,di,tcb,ctx,flags))	/* Get second point in range */
	       { ucb = errmsg ; UCstrcpy(ucb,ctx->ErrorMsgAux) ; tcb->type = UNUSED ; goto PointParseErr ;} ;
	      inrange = TRUE ; gotcompound = TRUE ;
	      if (gotrel)     PERR("Range of values not allowed after relational operator") ;
//veh090317	      if ((di->Flags & (V4DPI_DimInfo_RangeOK | V4DPI_DimInfo_ListOK)) == 0)     PERR("Range of values not allowed for this dimension") ;
	    } else
	    { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; 		/* Push back token */
	    } ;
	   switch(point->PntType)
	    { case V4DPI_PntType_Special:
			if (point->Grouping != V4DPI_Grouping_Single || inrange) PERR("Cannot have range or list with \"{xxx}\" point value") ;
			point->Grouping = tp1.Grouping ;
			break ;
	      case V4DPI_PntType_MemPtr:
		memcpy(&point->Value.MemPtr,&tp1.Value.MemPtr,sizeof(tp1.Value.MemPtr)) ; break ;
	      case V4DPI_PntType_UOM:
		point->LHSCnt = tp1.LHSCnt ;
		if (point->Grouping >= V4DPI_PointUOMMix_Max) PERR("Too many points specified") ;
		memcpy(&pum->Entry[point->Grouping].BeginUOM,&tp1.Value.UOMVal,sizeof(struct V4DPI__Value_UOM)) ;
		memcpy(&pum->Entry[point->Grouping++].EndUOM,(inrange ? &tp2.Value.UOMVal : &tp1.Value.UOMVal),sizeof(struct V4DPI__Value_UOM)) ;
		if (inrange)
		 { memcpy(&d1,&tp1.Value.UOMVal.Num,sizeof(d1)) ; memcpy(&d2,&tp2.Value.UOMVal.Num,sizeof(d2)) ;
		   if (d2 < d1) { ucb = errmsg ; v_Msg(ctx,ucb,"DPIPrsPtRange",d1,d2) ; goto PointParseErr ; } ;
		 } ;
		break ;
	      case V4DPI_PntType_UOMPer:
		point->LHSCnt = tp1.LHSCnt ;
		if (point->Grouping >= V4DPI_PointUOMPerMix_Max) PERR("Too many points specified") ;
		memcpy(&pupm->Entry[point->Grouping].BeginUOMPer,&tp1.Value.UOMPerVal,sizeof(struct V4DPI__Value_UOMPer)) ;
		memcpy(&pupm->Entry[point->Grouping++].EndUOMPer,(inrange ? &tp2.Value.UOMPerVal : &tp1.Value.UOMPerVal),sizeof(struct V4DPI__Value_UOMPer)) ;
		if (inrange)
		 { memcpy(&d1,&tp1.Value.UOMPerVal.Num,sizeof(d1)) ; memcpy(&d2,&tp2.Value.UOMPerVal.Num,sizeof(d2)) ;
		   if (d2 < d1)
		     { ucb = errmsg ; v_Msg(ctx,ucb,"DPIPrsPtRange",d1,d2) ; goto PointParseErr ; } ;
		 } ;
		break ;
	      case V4DPI_PntType_UOMPUOM:
		point->LHSCnt = tp1.LHSCnt ;
		if (point->Grouping >= V4DPI_PointUOMPUOMMix_Max) PERR("Too many points specified") ;
		memcpy(&pupum->Entry[point->Grouping].BeginUOMPUOM,&tp1.Value.UOMPUOMVal,sizeof(struct V4DPI__Value_UOMPUOM)) ;
		memcpy(&pupum->Entry[point->Grouping++].EndUOMPUOM,(inrange ? &tp2.Value.UOMPUOMVal : &tp1.Value.UOMPUOMVal),sizeof(struct V4DPI__Value_UOMPUOM)) ;
		break ;
	      case V4DPI_PntType_Fixed:
		point->LHSCnt = tp1.LHSCnt ;
		if (point->Grouping >= V4DPI_PointFixMix_Max) PERR("Too many points specified") ;
		memcpy(&pfm->Entry[point->Grouping].BeginFix,&tp1.Value.FixVal,sizeof(B64INT)) ;
		memcpy(&pfm->Entry[point->Grouping++].EndFix,(inrange ? &tp2.Value.FixVal : &tp1.Value.FixVal),sizeof(B64INT)) ;
		if (inrange)
		 { memcpy(&b641,&tp1.Value.FixVal,sizeof(B64INT)) ; memcpy(&b642,&tp2.Value.FixVal,sizeof(B64INT)) ;
		   if (b642 < b641) { ucb = errmsg ; v_Msg(ctx,ucb,"DPIPrsPtRange",(double)b641,(double)b642) ; goto PointParseErr ; } ;
		 } ;
		break ;
	      case V4DPI_PntType_Calendar:
	      case V4DPI_PntType_UTime:
	      case V4DPI_PntType_Real:
		if (point->Grouping >= V4DPI_PointRealMix_Max) PERR("Too many points specified") ;
		GETREAL(prm->Entry[point->Grouping].BeginReal,&tp1) ;
//		memcpy(&prm->Entry[point->Grouping].BeginReal,&tp1.Value.RealVal,SIZEofDOUBLE) ;
		memcpy(&prm->Entry[point->Grouping++].EndReal,(inrange ? &tp2.Value.RealVal : &tp1.Value.RealVal),SIZEofDOUBLE) ;
		if (inrange)
		 { GETREAL(d1,&tp1) ; GETREAL(d2,&tp2) ;
		   if (d2 < d1)
		    { ucb = errmsg ; v_Msg(ctx,ucb,"DPIPrsPtRange",d1,d2) ; goto PointParseErr ; } ;
		 } ;
		break ;
	      case V4DPI_PntType_TeleNum:
		point->Value.Tele = tp1.Value.Tele ; break ;
	      case V4DPI_PntType_Int2:
		point->Value.Int2Val[0] = tp1.Value.Int2Val[0] ; point->Value.Int2Val[1] = tp1.Value.Int2Val[1] ; break ;
	      case V4DPI_PntType_XDB:
		point->Value.XDB = tp1.Value.XDB ; break ;
	      case V4DPI_PntType_V4IS:
		point->Value = tp1.Value ; break ;
	      CASEofINT
	      case V4DPI_PntType_IntMod:
	      case V4DPI_PntType_SSVal:
	      case V4DPI_PntType_PntIdx:
		if (point->Grouping >= V4DPI_PointIntMix_Max) PERR("Too many points specified") ;
		if (tp1.Grouping > 0)
		 { pim1  = (struct V4DPI__Point_IntMix *)&tp1.Value ; gotcompound = TRUE ;
		   for(i=0;i<tp1.Grouping;i++) { pim->Entry[point->Grouping++] = pim1->Entry[i] ; } ;
		 } else
		 { pim->Entry[point->Grouping].BeginInt = tp1.Value.IntVal ;
		   pim->Entry[point->Grouping++].EndInt = (inrange ? tp2.Value.IntVal : tp1.Value.IntVal) ;
		   if (inrange ? (tp2.Value.IntVal < tp1.Value.IntVal) : FALSE)
		    { ucb = errmsg ; v_Msg(ctx,ucb,"DPIPrsPtRange",(double)tp1.Value.IntVal,(double)tp2.Value.IntVal) ; goto PointParseErr ; } ;
		 } ;
			break ;
	      case V4DPI_PntType_Tree:
	      case V4DPI_PntType_Color:
	      case V4DPI_PntType_Country:
	      case V4DPI_PntType_XDict:
	      case V4DPI_PntType_Dict:
	      case V4DPI_PntType_Tag:
		if (point->Grouping >= V4DPI_PointIntMix_Max) PERR("Too many points specified") ;
		pim->Entry[point->Grouping].BeginInt = tp1.Value.IntVal ;
		pim->Entry[point->Grouping++].EndInt = (inrange ? tp2.Value.IntVal : tp1.Value.IntVal) ;
			break ;
	      case V4DPI_PntType_AggRef:
			point->Value.IntVal = tp1.Value.IntVal ; point->Grouping = tp1.Grouping ;
			break ;
	      case V4DPI_PntType_List:
		   memcpy(&point->Value,&tp1.Value,tp1.Bytes-V4DPI_PointHdr_Bytes) ;	/* Move list into final point */
		   point->Bytes = tp1.Bytes ;
		   TKN(V4LEX_Option_RetEOL) ;
		   if (tcb->opcode == V_OpCode_Slash && ((flags & (V4DPI_PointParse_InColonEq|V4DPI_PointParse_InV4Exp)) == 0))
		    { if (!v4dpi_ParsePointSlashPoint(ctx,point,tcb,flags)) AUXERR ;
		    } else v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
		   goto return_point ;
	      CASEofChar
	      case V4DPI_PntType_Complex:
	      case V4DPI_PntType_BinObj:
	      case V4DPI_PntType_Isct:
	      case V4DPI_PntType_GeoCoord:
	      case V4DPI_PntType_Shell:
	      case V4DPI_PntType_FrgnDataEl:
	      case V4DPI_PntType_FrgnStructEl:
	      case V4DPI_PntType_TagVal:
//	      case V4DPI_PntType_BigText:
	      case V4DPI_PntType_RegExpPattern:
			tp1.Bytes -= V4DPI_PointHdr_Bytes ; tp2.Bytes -= V4DPI_PointHdr_Bytes ;
			if (tp1.Bytes + nextfree > V4DPI_AlphaVal_Max)
			 { ucb = errmsg ; v_Msg(ctx,ucb,"DPIPrsPtTooBig",nextfree,tp1.Bytes,V4DPI_AlphaVal_Max) ; goto PointParseErr ; } ;
			memcpy(&tbuf[nextfree],tp1.Value.AlphaVal,tp1.Bytes) ;
/*			Save index - if Unicode then divide by 2 because subscripting UCCHAR is increments of 2 bytes */
			pam->Entry[point->Grouping].BeginIndex = (point->PntType == V4DPI_PntType_UCChar ? nextfree / sizeof(UCCHAR) : nextfree) ;
			nextfree += ALIGN(tp1.Bytes) ;
			if (!inrange) { pam->Entry[point->Grouping].EndIndex = 0 ; }
			 else { memcpy(&tbuf[nextfree],tp2.Value.AlphaVal,tp2.Bytes) ;
				pam->Entry[point->Grouping].EndIndex = (point->PntType == V4DPI_PntType_UCChar ? nextfree / sizeof(UCCHAR) : nextfree) ;
				nextfree += ALIGN(tp2.Bytes) ;
			      } ;
			point->Grouping ++ ;
			break ;
	    } ;
	   TKN(V4LEX_Option_RetEOL) ;
	   if ((tcb->opcode == V_OpCode_Comma) && ((flags & V4DPI_PointParse_NoMult) == 0))
	    { gotcompound = TRUE ; inrange = FALSE ;
	      if (gotrel)  PERR("List of points not allowed with relational operator") ;
	      if ((di->Flags & V4DPI_DimInfo_ListOK) == 0)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"DPINoList",di->DimId) ; AUXERR
	       }
	      continue ;
	    } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; break ; } ;
	 } ;
/*	Get here at end of point - finalize point */
endofpoint:
	switch (point->PntType)
	 { default: PERR1a("@Unknown point type (%1U)",v4im_PTName(point->PntType)) ;
	   case V4DPI_PntType_Special:
		if (point->Bytes < V4DPI_PointHdr_Bytes) point->Bytes = V4DPI_PointHdr_Bytes ;
		break ;
	   CASEofINT
	   case V4DPI_PntType_Tree:
	   case V4DPI_PntType_Color:
	   case V4DPI_PntType_Country:
	   case V4DPI_PntType_XDict:
	   case V4DPI_PntType_Dict:
	   case V4DPI_PntType_SSVal:
	   case V4DPI_PntType_PntIdx:
	   case V4DPI_PntType_IntMod:
	   case V4DPI_PntType_Tag:
		if (gotcompound)
		 { 
//		   point->Bytes = (char *)&pim->Entry[point->Grouping].BeginInt - (char *)point ;
		   SETBYTESGRPINT(point) ;
		 } else
		 { point->Grouping = V4DPI_Grouping_Single ;
//		   point->Bytes = (char *) &point->Value.IntVal + sizeof point->Value.IntVal - (char *)point ;
		   point->Bytes = V4PS_Int ;
		 } ;
		break ;
	   case V4DPI_PntType_AggRef:
		point->Bytes = (char *)&point->Value.IntVal + sizeof point->Value.IntVal - (char *)point ;
		break ;
	   case V4DPI_PntType_V4IS:
		point->Bytes = V4DPI_PointHdr_Bytes + sizeof(struct V4DPI__PntV4IS) ; break ;
	   case V4DPI_PntType_TeleNum:
		point->Bytes = V4PS_Tele ; break ;
	   case V4DPI_PntType_Int2:
		point->Bytes = V4PS_Int2 ; break ;
	   case V4DPI_PntType_XDB:
		point->Bytes = V4PS_XDB ; break ;
	   case V4DPI_PntType_MemPtr:
		point->Bytes = V4DPI_PointHdr_Bytes + sizeof point->Value.MemPtr ; break ;
	   case V4DPI_PntType_UOM:
		if (gotcompound)
		 { point->Bytes = (char *)&pum->Entry[point->Grouping].BeginUOM - (char *)point ;
		 } else
		 { point->Grouping = V4DPI_Grouping_Single ;
		   point->Bytes = V4PS_UOM ;
		 } ;
		break ;
	   case V4DPI_PntType_UOMPer:
		if (gotcompound)
		 { point->Bytes = (char *)&pupm->Entry[point->Grouping].BeginUOMPer - (char *)point ;
		 } else
		 { point->Grouping = V4DPI_Grouping_Single ;
		   point->Bytes = V4PS_UOMPer ;
		 } ;
		break ;
	   case V4DPI_PntType_UOMPUOM:
		if (gotcompound)
		 { point->Bytes = (char *)&pupum->Entry[point->Grouping].BeginUOMPUOM - (char *)point ;
		 } else
		 { point->Grouping = V4DPI_Grouping_Single ;
		   point->Bytes = V4PS_UOMPUOM ;
		 } ;
		break ;
	   case V4DPI_PntType_Fixed:
		if (gotcompound)
		 { point->Bytes = (char *)&pfm->Entry[point->Grouping].BeginFix - (char *)point ;
		 } else
		 { point->Grouping = V4DPI_Grouping_Single ;
		   point->Bytes = V4PS_Real ;
		 } ;
		break ;
	   case V4DPI_PntType_UTime:
	   case V4DPI_PntType_Calendar:
	   case V4DPI_PntType_Real:
		if (gotcompound)
		 { 
//		   point->Bytes = (char *)&prm->Entry[point->Grouping].BeginReal - (char *)point ;
		   SETBYTESGRPDBL(point) ;
		 } else
		 { point->Grouping = V4DPI_Grouping_Single ;
		   point->Bytes = V4PS_Real ;
		 } ;
		break ;
	   case V4DPI_PntType_SymDef:
	   case V4DPI_PntType_TagVal:
	   case V4DPI_PntType_BigIsct:
	   case V4DPI_PntType_Isct:
		break ;
	   case V4DPI_PntType_List:
	   case V4DPI_PntType_GeoCoord:
	   case V4DPI_PntType_Complex:
	   case V4DPI_PntType_FrgnDataEl:
	   case V4DPI_PntType_FrgnStructEl:
	   case V4DPI_PntType_Shell:
	   case V4DPI_PntType_BinObj:
	   case V4DPI_PntType_RegExpPattern:
//	   case V4DPI_PntType_BigText:
	   CASEofChar
/*		Copy string in tbuf into point, add 3 extra bytes to ensure 32bit word is padded with 0's */
		if (gotcompound)
		 { memcpy(&pam->Entry[point->Grouping].BeginIndex,tbuf,nextfree+3) ;	/* Copy actual bytes in strings */
		   point->Bytes = (char *)&pam->Entry[point->Grouping].BeginIndex + nextfree - (char *)point ;
		 } else
		 { memcpy(&point->Value.AlphaVal,tbuf,nextfree+3) ;
		   point->Grouping = V4DPI_Grouping_Single ;
		   point->Bytes = (char *)&point->Value.AlphaVal[nextfree] - (char *)point ;
		 } ;
		break ;
	 } ;
	point->Bytes = ALIGN(point->Bytes) ;
	if (gotrel) point->Grouping = gotrel ;
return_point:
	if ((flags & V4DPI_PointParse_InV4Exp) == 0)		/* If not in {xxx} then check for "*" */
	 { TKN(V4LEX_Option_RetEOL) ;
	   if (tcb->opcode == V_OpCode_Star || tcb->opcode == V_OpCode_NCStar)	/* Got "*" - convert to Context(point) */
	    { P *ip ;
	      quoted = ISQUOTED(point) ; forceeval = point->ForceEval ;
#ifndef NEWQUOTE
	      UNQUOTE(point) ;
#endif
	      point->ForceEval = FALSE ; memcpy(&tp2,point,point->Bytes) ;
	      INITISCT(point) ;  ISCTVCD(point) ;point->Grouping = 2 ; ip = ISCT1STPNT(point) ;
	      ZPH(ip) ; ip->PntType = V4DPI_PntType_IntMod ; ip->Dim = Dim_IntMod ; ip->Value.IntVal = V4IM_OpCode_Context ; ip->Bytes = V4PS_Int ;
	      point->Bytes += ip->Bytes ; ADVPNT(ip) ;
	      memcpy(ip,&tp2,tp2.Bytes) ; point->Bytes += tp2.Bytes ;
#ifndef NEWQUOTE
	      if (quoted) { QUOTE(point) ; } else { UNQUOTE(point) ; } ;
#endif
	      forceeval = point->ForceEval = forceeval ;
	      TKN(V4LEX_Option_RetEOL) ;			/* Look for ",anotherpoint" */
	      if (tcb->opcode == V_OpCode_CommaComma)
	       { ADVPNT(ip) ;
	         if (!v4dpi_PointParse(ctx,ip,tcb,flags)) { ucb = errmsg ; UCstrcpy(ucb,ctx->ErrorMsgAux) ; tcb->type = UNUSED ; goto PointParseErr ;} ;
	       } else if (tcb->opcode == V_OpCode_Comma)
	       { point->Continued = TRUE ;			/* Point continues with another Isct */
	         ADVPNT(ip) ;
	         if (!v4dpi_PointParse(ctx,ip,tcb,flags))
		  { ucb = errmsg ; UCstrcpy(ucb,ctx->ErrorMsgAux) ; tcb->type = UNUSED ; goto PointParseErr ;} ;
	         point->Bytes += ip->Bytes ;
	       } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
	    } else
	    { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;		/* Don't have "*" - push back on */
	    } ;
	 } ;
	TKN(V4LEX_Option_RetEOL) ;

	if (tcb->opcode == V_OpCode_ColonEqual) 		/* Got: point1:=point2 */
	 { P *ip ;
	   quoted = ISQUOTED(point) ;
#ifndef NEWQUOTE
	   UNQUOTE(point) ;
#endif
	   memcpy(&tp2,point,point->Bytes) ;			/* Save point1 */
	   INITISCT(point) ; ISCTVCD(point) ; point->Grouping = 3 ;
#ifndef NEWQUOTE
	   if (quoted) { QUOTE(point) ; } else { UNQUOTE(point) ; } ;
#endif
	   ip = ISCT1STPNT(point) ; ZPH(ip) ; ip->PntType = V4DPI_PntType_IntMod ; ip->Dim = Dim_IntMod ; ip->Bytes = V4PS_Int ;
	   ip->Value.IntVal = V4IM_OpCode_Project ; point->Bytes += ip->Bytes ;
	   ADVPNT(ip) ; memcpy(ip,&tp2,tp2.Bytes) ; point->Bytes += tp2.Bytes ;
	   ADVPNT(ip) ;
	   if(!v4dpi_PointParse(ctx,ip,tcb,flags|V4DPI_PointParse_InColonEq)) AUXERR ;
	   point->Bytes += ip->Bytes ;
	   TKN(V4LEX_Option_RetEOL) ;
	   if (tcb->opcode == V_OpCode_Slash && ((flags & (V4DPI_PointParse_InColonEq|V4DPI_PointParse_InV4Exp)) == 0))
	    { if (!v4dpi_ParsePointSlashPoint(ctx,point,tcb,flags)) AUXERR ;
	    } else v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
/*	   Now about point->dim ? */
	 } else if (tcb->opcode == V_OpCode_DashRangle && (flags & V4DPI_PointParse_NestedDot) == 0)
	 { if (!v4dpi_ParsePointDashRangle(ctx,point,tcb,flags)) AUXERR ;
/*	   How about point,point ? (but look out for point.point,point - handle ,point from calling routine!) */
	 } else if (tcb->opcode == V_OpCode_Comma && ((flags & (V4DPI_PointParse_NestedDot|V4DPI_PointParse_NoMult)) == 0))
	 { point->Continued = TRUE ;				/* Point continues with another Isct */
	   cpt = (P *)((char *)point + point->Bytes) ;		/* Figure out where next point to begin */
	   if(!v4dpi_PointParse(ctx,cpt,tcb,flags&~V4DPI_PointParse_InColonEq)) AUXERR ;
	   point->Bytes += cpt->Bytes ; 			/* Include bytes of next point */
	 } else if (tcb->opcode == V_OpCode_CommaComma && ((flags & (V4DPI_PointParse_NestedDot|V4DPI_PointParse_NoMult)) == 0))
	 { cpt = (P *)((char *)point + point->Bytes) ;		/* Figure out where next point to begin */
	   if(!v4dpi_PointParse(ctx,cpt,tcb,flags&~V4DPI_PointParse_InColonEq)) AUXERR ;
	 } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;

	return(TRUE) ;
/*	Here to handle point syntax errors (ucb = ptr to error message) */
PointParseErr:
	if (tcb->type == V4LEX_TknType_Error && UCstrlen(tcb->UCstring) > 0)
	 { if (ucb == NULL) { ucb = errmsg ; UCstrcpy(ucb,tcb->UCstring) ; }
	    else { v_Msg(ctx,errmsg,"@%1U (%2U)",ucb,tcb->UCstring) ; ucb = errmsg ; } ;
	   ZUS(tcb->UCstring) ;	/* Set to NULL so we don't nest these errors via nested parse calls */
	 } ;
	if (ucb != ctx->ErrorMsgAux)
	 { if (ucb == NULL) { UCstrcpy(ctx->ErrorMsgAux,UClit("(error not set???)")) ; }
	    else { UCstrcpy(ctx->ErrorMsgAux,ucb) ; } ;
	 } ;
	if (flags & V4DPI_PointParse_RetFalse) return(FALSE) ;
	v4_UCerror(0,tcb,"","","",ctx->ErrorMsgAux) ;
	return(FALSE) ;
}




LOGICAL v4dpi_ParsePointSlashPoint(ctx,point,tcb,flags)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *point ;
  struct V4LEX__TknCtrlBlk *tcb ;
  int flags ;
{ P tp2, *ip ;
  int forceeval = FALSE ;

	if (point->ForceEval) { forceeval = TRUE ; point->ForceEval = FALSE ; } ;
	memcpy(&tp2,point,point->Bytes) ;		/* Copy initial isct into temp */
	QUOTE(&tp2) ;					/* Make first point quoted */
	INITISCT(point) ; ISCTVCD(point) ; point->Grouping = 3 ; point->ForceEval = forceeval ;
	ip = ISCT1STPNT(point) ;
	ZPH(ip) ; ip->PntType = V4DPI_PntType_IntMod ; ip->Dim = Dim_IntMod ;
	ip->Value.IntVal = V4IM_OpCode_EnumCL ; ip->Bytes = V4PS_Int ; point->Bytes += ip->Bytes ;
	ADVPNT(ip) ; if(!v4dpi_PointParse(ctx,ip,tcb,flags)) return(FALSE) ; /* Parse point after slash */
	point->Bytes += ip->Bytes ;
	ADVPNT(ip) ; memcpy(ip,&tp2,tp2.Bytes) ; point->Bytes += tp2.Bytes ;
	return(TRUE) ;
}

/*	v4dpi_ParsePointDashRangle - parses construct of form point->dim	*/
/*	Call: ok = v4dpi_ParsePointDashRangle
	      assumes point holds current point, tcb positioned to "->"
	      updates point with resulting projection				*/

LOGICAL v4dpi_ParsePointDashRangle(ctx,point,tcb,flags)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *point ;
  struct V4LEX__TknCtrlBlk *tcb ;
  int flags ;
{ P newpt,tp1,tp2,*cpt,*ip ;

	INITISCT(&newpt) ; ISCTVCD(&newpt) ; newpt.Grouping = 3 ; ip = ISCT1STPNT(&newpt) ;
/*	Construct Project() point */
	ZPH(ip) ; ip->PntType = V4DPI_PntType_IntMod ; ip->Dim = Dim_IntMod ;
	ip->Value.IntVal = V4IM_OpCode_Project ; ip->Bytes = V4PS_Int ;
	newpt.Bytes += ip->Bytes ;
/*	Grab next token which best be a dimension */
	v4lex_NextTkn(tcb,V4LEX_Option_ForceKW) ;
	if (tcb->type != V4LEX_TknType_Keyword)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"DPIUnqNotDim") ; return(FALSE) ; } ;
	ADVPNT(ip) ; dictPNTv(ip,Dim_Dim,v4dpi_DimGet(ctx,tcb->UCkeyword,DIMREF_REF)) ;
	if (ip->Value.IntVal <= 0) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIUnqNotDim") ; return(FALSE) ; } ;
	newpt.Bytes += ip->Bytes ;
/*	Now copy original point */
	ADVPNT(ip) ; memcpy(ip,point,point->Bytes) ; newpt.Bytes += point->Bytes ;
/*	And finally copy back over original point */
	memcpy(point,&newpt,newpt.Bytes) ;
/*	Now check for following "." or another "->" */
	for(;;)
	 { 
	   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
	   if (tcb->opcode == V_OpCode_Dot)
	    { memcpy(&tp1,point,point->Bytes) ;
	      if(!v4dpi_PointParse(ctx,&tp2,tcb,V4DPI_PointParse_NestedDot)) return(FALSE) ;
	      INITISCT(point) ; ISCTVCD(point) ; ip = ISCT1STPNT(point) ;
	      memcpy(ip,&tp1,tp1.Bytes) ;	/* Append first point to ISCT */
	      point->NestedIsct = TRUE ; point->Bytes += tp1.Bytes ; point->Grouping ++ ;
	      ADVPNT(ip) ; memcpy(ip,&tp2,tp2.Bytes) ;	/* Then append second point */
	      point->Bytes += tp2.Bytes ; point->Grouping ++ ;
	      continue ;
	    }
	    else if (tcb->opcode == V_OpCode_DashRangle)
	    { if (!v4dpi_ParsePointDashRangle(ctx,point,tcb,flags)) return(FALSE) ;
	      continue ;
	    }
	    else if (tcb->opcode == V_OpCode_Comma)
	    { point->Continued = TRUE ;			/* Point continues with another Isct */
	      cpt = (P *)((char *)point + point->Bytes) ;
	      if (!v4dpi_PointParse(ctx,cpt,tcb,flags)) return(FALSE) ;
	      point->Bytes += cpt->Bytes ;
	      break ;
	    } else if (tcb->opcode == V_OpCode_CommaComma)
	    { cpt = (P *)((char *)point + point->Bytes) ;
	      if (!v4dpi_PointParse(ctx,cpt,tcb,flags)) return(FALSE) ;
	      break ;
	    } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; break ; } ;
	 } ;
	return(TRUE) ;
}


LOGICAL v4dpi_PointParseExp(ctx,point,tcb,flags)		/* Parse { exp } & convert to intmods */
  struct V4C__Context *ctx ;
  struct V4DPI__Point *point ;
  struct V4LEX__TknCtrlBlk *tcb ;
  int flags ;
{ struct V4DPI__Point *apt,*tpt,*vpt,impt,*cpp ;
  struct V4LEX__RevPolParse rpp ;
  int SCount ;				/* Stack of V4 points */
#define V4DPI_PPE_SMAX 20
  struct V4DPI__Point Spt[V4DPI_PPE_SMAX] ;
  int imx,rx,n,i,operands,opt1,opt2,rppf,len,totlen ;

	rppf = V4LEX_RPPFlag_V4IMEval|V4LEX_RPPFlag_BraceDelim|V4LEX_RPPFlag_NoCTX ;
/*	If not allowing multi-line points then don't allow multi-line {...} */
	if ((flags & V4DPI_PointParse_MultiLine) == 0) rppf |= V4LEX_RPPFlag_EOL ;
	if (!v4lex_RevPolParse(&rpp,tcb,rppf,ctx)) return(FALSE) ;
	if (rpp.Count <= 0) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsBraceExp1") ; return(FALSE) ; } ;

#ifdef TRACERPPRESULTS
for(rx=0;rx<rpp.Count;rx++)
 { if (OPCODEISOPER(rpp.Token[rx].OpCode))
    { printf("%d - Opcode(%d)\n",rx,rpp.Token[rx].OpCode) ; continue ; } ;
   switch(rpp.Token[rx].Type)
    { case V4LEX_TknType_String:
	printf("%d - String(%s)\n",rx,UCretASC(rpp.Token[rx].AlphaVal)) ; break ;
      case V4LEX_TknType_Keyword:
      case V4LEX_TknType_Integer:
	printf("%d - KW/Int(%d)\n",rx,rpp.Token[rx].IntVal) ; break ;
      case V4LEX_TknType_Float:
	printf("%d - Float(%g)\n",rx,rpp.Token[rx].Floating) ; break ;
      case V4LEX_TknType_EOL:
      case V4LEX_TknType_Punc:
      case V4LEX_TknType_DES:
      case V4LEX_TknType_Isct:
      case V4LEX_TknType_Point:
	v_Msg(ctx,v4xtbuf,"@%1d - Point(%2P)\n",rx,rpp.Token[rx].ValPtr) ; vout_UCText(VOUT_Trace,0,ASCTBUF1) ; break ;
    } ;
} ;
#endif

	SCount = 0 ;				/* Init stack index */
	for(rx=0;rx<rpp.Count;rx++)
	 { if (OPCODEISOPER(rpp.Token[rx].OpCode))	/* Got an operator? */
	    { operands = 2 ;
	      opt1 = FALSE ;			/* If true then try to pull operands into one intmod (e.g. And/Or/Plus) */
	      opt2 = FALSE ;			/* If true then quote all isct arguments */
	      switch (rpp.Token[rx].OpCode)
	       { default:

		   if (rpp.Token[rx].OpCode >= V_OpCode_MacArgA && rpp.Token[rx].OpCode <= V_OpCode_MacArgZ)
		    { i = rpp.Token[rx].OpCode - V_OpCode_MacArgA ;
		      if (tcb->poundparam[i].numvalue == V4LEX_Tkn_PPNumUndef)
		       { v_Msg(ctx,ctx->ErrorMsgAux,"TknParamNotSet",i) ; return(FALSE) ; } ;
		      rpp.Token[rx].Type = V4LEX_TknType_Integer ;
		      rpp.Token[rx].OpCode = 0 ; rpp.Token[rx].IntVal = tcb->poundparam[i].numvalue ;
		      rx-- ; continue ;			/* If we got #x# then fake in numeric value & continue */
		    } ;
		   v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsExpInvOp",rpp.Token[rx].OpCode) ; return(FALSE) ;
		 case V_OpCode_Plus:		opt1 = TRUE ; imx = V4IM_OpCode_Plus ; break ;
		 case V_OpCode_Minus:		imx = V4IM_OpCode_Minus ; break ;
		 case V_OpCode_UMinus:		operands = 1 ; imx = V4IM_OpCode_Minus ; break ;
		 case V_OpCode_NCStar:
		 case V_OpCode_Star:		imx = V4IM_OpCode_Mult ; break ;
		 case V_OpCode_Slash:		imx = V4IM_OpCode_Div ; break ;
		 case V_OpCode_SlashSlash:	imx = V4IM_OpCode_DDiv ; break ;
		 case V_OpCode_Equal:		imx = V4IM_OpCode_EQ ; break ;
		 case V_OpCode_Langle:		imx = V4IM_OpCode_LT ; break ;
		 case V_OpCode_LangleEqual:	imx = V4IM_OpCode_LE ; break ;
		 case V_OpCode_TildeEqual:	imx = V4IM_OpCode_NIn ; break ;
		 case V_OpCode_EqualEqual:	imx = V4IM_OpCode_In ; break ;
		 case V_OpCode_LangleRangle:	imx = V4IM_OpCode_NEk ; break ;
		 case V_OpCode_Rangle:		imx = V4IM_OpCode_GT ; break ;
		 case V_OpCode_RangleEqual:	imx = V4IM_OpCode_GE ; break ;
		 case V_OpCode_Tilde:		operands = 1 ; imx = V4IM_OpCode_Not ; break ;
		 case V_OpCode_Ampersand:	opt1 = TRUE ; opt2 = TRUE ; imx = V4IM_OpCode_And ; break ;
		 case V_OpCode_Line:		opt1 = TRUE ; opt2 = TRUE ; imx = V4IM_OpCode_Or ; break ;
		 case V_OpCode_Percent: 	imx = V4IM_OpCode_Percent ; break ;
	       } ;
	      INITISCT(&impt) ; ISCTVCD(&impt) ; impt.Grouping = 1 + operands ;
	      tpt = ISCT1STPNT(&impt) ; ZPH(tpt) ; tpt->Bytes = V4PS_Int ; tpt->Value.IntVal = imx ; tpt->Dim = Dim_IntMod ; tpt->PntType = V4DPI_PntType_IntMod ;
	      ISCTSETNESTED(&impt,tpt) ;
	      impt.Bytes += tpt->Bytes ; ADVPNT(tpt) ;
	      if (SCount - operands < 0)			/* Make sure we have enough operands */
	       { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsExpNEVal") ; return(FALSE) ; } ;
	      totlen = 0 ;
	      for(i=0;i<operands;i++)
	       { 
	         
		 if (opt1)
		  {
		    apt = &Spt[SCount-operands+i] ;		/* apt = ptr to argument */
		    if (apt->PntType == V4DPI_PntType_Isct)
		     { n = apt->Grouping - 1 ;			/* n = number of arguments (exclude first) */
		       apt = ISCT1STPNT(apt) ;			/* apt = ptr to first point in isct argument */
		       if (apt->PntType == V4DPI_PntType_IntMod && apt->Value.IntVal == imx)
		        { for(;n>0;n--)				/* Append all "real arguments */
			   { ADVPNT(apt) ;			/* Advance to 2nd point (1st arg of intmod) */
			     if (opt2 && apt->PntType == V4DPI_PntType_Isct) { QUOTE(apt) ; } ;
			      if ((totlen += apt->Bytes) > V4DPI_AlphaVal_Max - 50)
			       { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsExpSize",&Spt[SCount-operands+i]) ; return(FALSE) ; } ;
			      memcpy(tpt,apt,apt->Bytes) ;			/* Copy argument over */
			      ADVPNT(tpt) ;
			      impt.Grouping++ ;
			    } ; impt.Grouping += (n - 1) ;	/* Increment final number of arguments */
			  continue ;
			} ;
		     } ;
		  } ;
		 apt = &Spt[SCount-operands+i] ;
		 if (opt2 && apt->PntType == V4DPI_PntType_Isct) { QUOTE(apt) ; } ;
		 memcpy(tpt,apt,apt->Bytes) ;
	         ISCTSETNESTED(&impt,tpt) ;
	         impt.Bytes += tpt->Bytes ; ADVPNT(tpt) ;
	       } ;

	      SCount -= operands ;				/* Pop args off of stack */
	      ISCTLEN(&impt,tpt) ;
	      tpt = &Spt[SCount++] ;			/* Push intmod back onto stack */
	      memcpy(tpt,&impt,impt.Bytes) ;
	      for(n=0,cpp=ISCT1STPNT(tpt);n<tpt->Grouping;n++)
	       { 
		 ADVPNT(cpp) ;
	       } ;
	      continue ;
	    } ;
	   if (rpp.Token[rx].dps > 0 && rpp.Token[rx].Type == V4LEX_TknType_Integer)
	    { rpp.Token[rx].Type = V4LEX_TknType_Float ;
	      rpp.Token[rx].Floating = rpp.Token[rx].IntVal ;
	      for(n=0;n<rpp.Token[rx].dps;n++) { rpp.Token[rx].Floating /= 10.0 ; } ;
	    } ;
	   switch(rpp.Token[rx].Type)
	    { case V4LEX_TknType_String:
		tpt = &Spt[SCount++] ;
		len = UCstrlen(rpp.Token[rx].AlphaVal) ;
		if (len >= V4DPI_UCVal_Max)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsExpLitMax",len,V4DPI_UCVal_Max) ; return(FALSE) ; } ;
		uccharPNTv(tpt,rpp.Token[rx].AlphaVal) ;
		break ;
	      case V4LEX_TknType_Keyword:
	      case V4LEX_TknType_Integer:
		tpt = &Spt[SCount++] ;		/* tpt = top of point stack */
		intPNTv(tpt,rpp.Token[rx].IntVal) ; break ;
	      case V4LEX_TknType_Float:
		tpt = &Spt[SCount++] ;		/* tpt = top of point stack */
		dblPNT(tpt) ; PUTREAL(tpt,rpp.Token[rx].Floating) ; break ;
	      case V4LEX_TknType_EOL:
	      case V4LEX_TknType_Punc:
	      case V4LEX_TknType_DES:
	      case V4LEX_TknType_Isct:
	      case V4LEX_TknType_Point:
		tpt = &Spt[SCount++] ;		/* tpt = top of point stack */
		vpt = (P *)rpp.Token[rx].ValPtr ; memcpy(tpt,vpt,vpt->Bytes) ; v4mm_FreeChunk(rpp.Token[rx].ValPtr) ;
		break ;
	    } ;
	 } ;
	memcpy(point,&Spt[SCount-1],Spt[SCount-1].Bytes) ;	/* Return final result */
	return(TRUE) ;
}

LOGICAL v4dpi_ParseJSONSyntax(ctx,point,tcb,flags,isDim)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *point ;
  struct V4LEX__TknCtrlBlk *tcb ;
  FLAGS32 flags ;
  LOGICAL isDim ;
{ P *ipt, *pt, tpnt ;
 
/*	Syntax: $(point.point. ...) - but have already parse '$' and '('	*/
/*	Going to convert to Array(point point ...)				*/
	INITISCT(point) ; NOISCTVCD(point) ; point->Grouping = 1 ;		/* Construct JSONRef/JSONRefDim() intersection */
	pt = &tpnt ; ZPH(pt) ; pt->PntType = V4DPI_PntType_IntMod ; pt->Bytes = V4PS_Int ; pt->Value.IntVal = (isDim ? V4IM_OpCode_JSONRefDim : V4IM_OpCode_JSONRef) ; pt->Dim = Dim_IntMod ;
	ipt = ISCT1STPNT(point) ; memcpy(ipt,pt,pt->Bytes) ; ADVPNT(ipt) ; point->Bytes += pt->Bytes ;
	tcb->in_jsonRef = TRUE ; v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
	if (tcb->opcode == V_OpCode_Question) { point->TraceEval = TRUE ; }
	 else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
	for(;;)
	 { if (point->Grouping >= V4DPI_IsctDimMax) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsIsctMaxPt",V4DPI_IsctDimMax) ; goto fail ; } ;
	   if (!v4dpi_PointParse(ctx,&tpnt,tcb,flags|V4DPI_PointParse_RetFalse|V4DPI_PointParse_InJSON)) goto fail ;
	   if (point->Bytes + tpnt.Bytes >  V4DPI_AlphaVal_Max - 20) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsIsctBigMx",point->Bytes,tpnt.Bytes,point->Bytes + tpnt.Bytes, V4DPI_AlphaVal_Max - 20) ; goto fail ; } ;
	   memcpy(ipt,&tpnt,tpnt.Bytes) ; ADVPNT(ipt) ; point->Grouping++ ; point->Bytes += tpnt.Bytes ; 
	   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ; if (tcb->type == V4LEX_TknType_Error) { UCstrcpy(ctx->ErrorMsgAux,tcb->UCstring) ; goto fail ; } ;
/*	   Last chance - may have parsed '.nnnn' and returned a decimal number. If so then handle */
	   if (tcb->type == V4LEX_TknType_Integer && tcb->decimal_places > 0)
	    { intPNTv(&tpnt,tcb->integer) ; memcpy(ipt,&tpnt,tpnt.Bytes) ; ADVPNT(ipt) ; point->Grouping++ ; point->Bytes += tpnt.Bytes ; 
	      v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ; if (tcb->type == V4LEX_TknType_Error) { UCstrcpy(ctx->ErrorMsgAux,tcb->UCstring) ; goto fail ; } ;
	    } ;
	   if (tcb->opcode == V_OpCode_Dot || tcb->opcode == V_OpCode_NCDot) continue ;
	   if (tcb->opcode == V_OpCode_RParen) break ;
/*	   If got an '=' then parse one more VALUE point */
	   if (tcb->opcode == V_OpCode_Equal)
	    { memcpy(ipt,&protoEQ,protoEQ.Bytes) ; ADVPNT(ipt) ; point->Grouping++ ; point->Bytes += protoEQ.Bytes ; 
	      if (!v4dpi_PointParse(ctx,&tpnt,tcb,flags|V4DPI_PointParse_RetFalse|V4DPI_PointParse_InJSON)) goto fail ;
	      memcpy(ipt,&tpnt,tpnt.Bytes) ; ADVPNT(ipt) ; point->Grouping++ ; point->Bytes += tpnt.Bytes ;
	      v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
	      if (tcb->opcode == V_OpCode_RParen) break ;
	      v_Msg(ctx,ctx->ErrorMsgAux,"JSONDotParen",((int)(tcb->ilvl[tcb->ilx].input_ptr - tcb->ilvl[tcb->ilx].input_str))) ; goto fail ;
	    } ;
	   v_Msg(ctx,ctx->ErrorMsgAux,"JSONDotParen",((int)(tcb->ilvl[tcb->ilx].input_ptr - tcb->ilvl[tcb->ilx].input_str))) ; goto fail ;
	 } ;
	
	v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
	if (tcb->opcode == V_OpCode_CommaComma)
	 { if (!v4dpi_PointParse(ctx,&tpnt,tcb,flags)) goto fail ;
	 } else if (tcb->opcode == V_OpCode_Comma)
	 { if (!v4dpi_PointParse(ctx,&tpnt,tcb,flags)) goto fail ;
	   if (point->Bytes + tpnt.Bytes >  V4DPI_AlphaVal_Max - 20) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsIsctBigMx",point->Bytes,tpnt.Bytes,point->Bytes + tpnt.Bytes, V4DPI_AlphaVal_Max - 20) ; goto fail ; } ;
	   memcpy(ipt,&tpnt,tpnt.Bytes) ; ADVPNT(ipt) ; point->Bytes += tpnt.Bytes ; point->Continued = TRUE ;
	 } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
	v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
	if (tcb->opcode == V_OpCode_Slash && ((flags & (V4DPI_PointParse_InColonEq|V4DPI_PointParse_InV4Exp)) == 0))
	 { if (!v4dpi_ParsePointSlashPoint(ctx,point,tcb,flags)) goto fail ;
	 } else v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
	tcb->in_jsonRef = FALSE ; return(TRUE) ;

fail:	tcb->in_jsonRef = FALSE ; return(FALSE) ;
}



/*	v4dpi_IsctParse - Parses an intersection			*/
/*	Call: ok = v4dpi_IsctParse( point , tcb , ctx , startpt , termop , flags )
	  where ok is result of parse, TRUE if ok, FALSE if problem (in ErrorMsgAux),
		point is updated with intersection,
		tcb is pointer to token control block (next token must be starting "["),
		ctx is area id,
		startpt if not NULL is starting point for Isct (terminating with ")", not "]",
		termop is terminating opcode (usually V_Opcode_RBracket or V_Opcode_RParen),
		flags are parsing options/flags: V4DPI_PointParse_xxx	*/

LOGICAL v4dpi_IsctParse(point,tcb,ctx,startpt,termop,flags)
  struct V4DPI__Point *point ;
  struct V4LEX__TknCtrlBlk *tcb ;
  struct V4C__Context *ctx ;
  struct V4DPI__Point *startpt ;
  int termop,flags ;
{ struct V4DPI__Point tp1,tp2,*cpt ;
  struct V4MM__MemMgmtMaster *mmm ;
  struct V4DPI__BigIsct bibuf ;
#define MAXPNTS 150
  DIMID dimIdList[MAXPNTS] ;
  LOGICAL gotTilde, gotBar ;
  int i,nextfree,StartingLine,rx,maxsize,inexp,lflags ; INDEX areax ; INDEX localSymLevel ;

	inexp = ((flags & V4DPI_PointParse_InV4Exp) != 0) ;
	lflags = flags & (~V4DPI_PointParse_InV4Exp) ;
/*	General format is dimension=point,point..point (if no dimension then point assumed to be NId) */
	INITISCT(point) ; ISCTVCD(point) ;
	nextfree = 0 ;

/*	Include any BindContext points first */
	for(i=tcb->ilx;i>=0;i--)
	 { INDEX px ;
	   if (tcb->ilvl[i].bcPt == NULL) continue ;
	   if ( tcb->ilvl[i].bcPt->Grouping == 0) continue ;
	   cpt = ISCT1STPNT(tcb->ilvl[i].bcPt) ;
	   for(px=0;px<tcb->ilvl[i].bcPt->Grouping;px++)
	    { memcpy(&point->Value.Isct.pntBuf[nextfree],cpt,cpt->Bytes) ;
	      ISCTSETNESTED(point,cpt) ;  nextfree += cpt->Bytes ; point->Grouping++ ; ADVPNT(cpt) ;
	    } ;
	 } ;

	gotBar = FALSE ; gotTilde = FALSE ;
	maxsize = V4DPI_AlphaVal_Max - 20 ;		/* Set initial max isct size to fit in regular point (minus a little room in case part of a tag!) */
	bibuf.DimUID = UNUSED ; 			/* No BigIsct yet */
	localSymLevel = v4dpi_NewLevelLocalSym() ;	/* Initialize local symbols */
	if (startpt == NULL)
	 { v4lex_NextTkn(tcb,0) ; if (tcb->type == V4LEX_TknType_Error) { UCstrcpy(ctx->ErrorMsgAux,tcb->UCstring) ; goto fail ; } ;
	   if (!(tcb->opcode == V_OpCode_LBracket || tcb->opcode == V_OpCode_NCLBracket))
	    { if (tcb->opcode == V_OpCode_LRBracket) goto endofisct ;
	      v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsIsctBgn") ; goto fail ;
	    } ;
	 } else
	 { memcpy(&point->Value.Isct.pntBuf[nextfree],startpt,startpt->Bytes) ;
	   ISCTSETNESTED(point,startpt) ;
	   nextfree += startpt->Bytes ; point->Grouping ++ ;
	 } ;
	v4lex_NextTkn(tcb,0) ; if (tcb->type == V4LEX_TknType_Error) { UCstrcpy(ctx->ErrorMsgAux,tcb->UCstring) ; goto fail ; } ;	/* Check for "?" indicating TraceEval flag */
	if (tcb->opcode == V_OpCode_Question)
	 { point->TraceEval = TRUE ; }
	 else { point->TraceEval = FALSE ; v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
/*	Only set the starting line if we are actually reading lines */
	switch (tcb->ilvl[tcb->ilx].mode)
	 { default:
		{ INDEX i ; StartingLine = 0 ;
/*		  Not reading 'line' from line oriented input, grab line from last input with lines */
		  for(i=tcb->ilx;i>0&&StartingLine==0;i--)
		   { switch (tcb->ilvl[i].mode)
		      {	case V4LEX_InpMode_File: case V4LEX_InpMode_TempFile: case V4LEX_InpMode_Stdin:	case V4LEX_InpMode_CmpFile: //case V4LEX_InpMode_StringML:
			   StartingLine = tcb->ilvl[i].current_line ;  break ;
		      } ;
		   } ;
		}
		break ;
	    case V4LEX_InpMode_File: case V4LEX_InpMode_TempFile: case V4LEX_InpMode_Stdin:	case V4LEX_InpMode_CmpFile:case V4LEX_InpMode_StringML:
		StartingLine = tcb->ilvl[tcb->ilx].current_line ;
		break ;
	 } ;
	for(;;)
	 {
	   v4lex_NextTkn(tcb,V4LEX_Option_NegLit) ; if (tcb->type == V4LEX_TknType_Error) { UCstrcpy(ctx->ErrorMsgAux,tcb->UCstring) ; goto fail ; } ;
	   if (tcb->type == V4LEX_TknType_Punc)
	    { switch(tcb->opcode)
	       { case V_OpCode_Line:
			if (gotBar) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsOneBar") ; goto fail ; } ;
			point->LHSCnt = point->Grouping ; gotBar = TRUE ;
			continue ;
		 case V_OpCode_Tilde:
			if (startpt != NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsTildeMod") ; goto fail ; } ;
			if (gotTilde) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsIsctTilde") ; goto fail ; } ;
			gotTilde = TRUE ; continue ;
	       } ;
	      switch (termop)			/* Do special check to all closing brace to close lots of iscts/lists */
	       { case V_OpCode_RBracket:
			if (tcb->opcode == termop) goto endofisct ; break ;
		 case V_OpCode_Rangle:
			if (tcb->opcode == termop) goto endofisct ; break ;
		 case V_OpCode_RParen:
			if (tcb->opcode == termop) goto endofisct ;
			if (tcb->opcode == V_OpCode_RBrace) { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; goto endofisct ; } ;
			break ;
		 case V_OpCode_RBrace:
			if (tcb->opcode == termop) goto endofisct ;
			break ;
	       } ;
	    } ;
	   if (tcb->ilvl[tcb->ilx].current_line < StartingLine &&
		 !(tcb->ilvl[tcb->ilx].mode == V4LEX_InpMode_MacArg || tcb->ilvl[tcb->ilx].mode == V4LEX_InpMode_String || tcb->ilvl[tcb->ilx].mode == V4LEX_InpMode_StringML))
	    { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsNewFile") ; goto fail ; } ;
	   if (tcb->ilvl[tcb->ilx].current_line > StartingLine)
	    { StartingLine = tcb->ilvl[tcb->ilx].current_line ;
	      if (tcb->ilvl[tcb->ilx].indent_line == 0)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsNewLine") ; goto fail ; } ;
	    } ;
/*	   Nothing special - parse as point! */
	   v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
	   if(!v4dpi_PointParse(ctx,&tp1,tcb,lflags&~V4DPI_PointParse_InColonEq)) goto fail ;	/* Not part of intersection syntax - parse point */
	   if (gotTilde) { gotTilde = FALSE ; tp1.AltDim = TRUE ; } ;	/* Set flag in point */
	   tp1.Bytes = ALIGN(tp1.Bytes) ;				/* Align the point */
	   if (tp1.Bytes + nextfree > maxsize)				/* If exceeded max point the make BigIsct */
	    { point = v4dpi_MakeBigIsct(ctx,&bibuf,point,nextfree,&tp1,&maxsize) ;
	      if (point == NULL) goto fail ;
	    } ;
	   memcpy(&point->Value.Isct.pntBuf[nextfree],&tp1,tp1.Bytes) ;
	   ISCTSETNESTED(point,&tp1) ;
	   nextfree += tp1.Bytes ;
	   if (point->Grouping >= V4DPI_IsctDimMax && !(startpt == NULL ? FALSE : startpt->PntType == V4DPI_PntType_IntMod))
	    { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsIsctMaxPt",V4DPI_IsctDimMax) ; goto fail ; } ;
	   for(i=0;i<point->Grouping&&((flags&V4DPI_PointParse_Pattern)==0);i++)
	    { if (tp1.Dim != dimIdList[i] || tp1.Dim == 0 || tp1.PntType == V4DPI_PntType_Isct) continue ;
	      if (startpt == NULL ? FALSE : startpt->PntType == V4DPI_PntType_IntMod) break ;	/* IntMods can have dup dimensions */
	      v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsIsctDupDm",i+1,point->Grouping+1,&tp1) ; goto fail ;
	    } ;
	   dimIdList[point->Grouping] = (tp1.PntType == V4DPI_PntType_Isct ? 0 : tp1.Dim) ;
	   point->Grouping ++ ; 			/* Update point count in intersection */
	 } ;
/*	All done intersection - maybe update pointer & return OK */
endofisct:
	if (gotTilde) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsIsctTilde") ; goto fail ; } ;
	nextfree = ALIGN(nextfree) ;
	point->Bytes = (char *)&point->Value.Isct.pntBuf[nextfree] - (char *)point ;
/*	Disabled this cache mode as of 6/1/97: point->Dim = (cacheok ? ++gpi->CacheIsctLastId : 0) ; */
	v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ; if (tcb->type == V4LEX_TknType_Error) { UCstrcpy(ctx->ErrorMsgAux,tcb->UCstring) ; tcb->type = UNUSED ; goto fail ; } ;/* Look to see if end of Isct or if we got a "," */
	if (tcb->opcode == V_OpCode_Not)		/* Did we get a "!" */
	 { if (termop != V_OpCode_RBracket) { v_Msg(ctx,ctx->ErrorMsgAux,"DPICondIsct") ; goto fail ; } ;
	   point->CondEval = TRUE ; 
	   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ; if (tcb->type == V4LEX_TknType_Error) { UCstrcpy(ctx->ErrorMsgAux,tcb->UCstring) ; goto fail ; } ;/* Look to see if end of Isct or if we got a "," */
	 } ;
	if (tcb->opcode == V_OpCode_Comma)
	 {
	   if(!v4dpi_PointParse(ctx,&tp2,tcb,flags&~V4DPI_PointParse_InColonEq)) goto fail ;
	   if (nextfree + tp2.Bytes >= maxsize)
	    { point = v4dpi_MakeBigIsct(ctx,&bibuf,point,nextfree,&tp2,&maxsize) ;
	      if (point == NULL)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsIsctCont",tp2.Bytes,nextfree,maxsize) ; goto fail ; } ;
	    } ;
	   cpt = (P *)&point->Value.Isct.pntBuf[nextfree] ;/* Figure out where next Isct to begin */
	   memcpy(cpt,&tp2,tp2.Bytes) ;
	   point->Continued = TRUE ;			/* Point continues with another Isct */
	   point->Bytes += cpt->Bytes ; 		/* Include bytes of next point */
	   nextfree += cpt->Bytes ;
	   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ; if (tcb->type == V4LEX_TknType_Error) { UCstrcpy(ctx->ErrorMsgAux,tcb->UCstring) ; goto fail ; } ;/* Look to see if end of Isct or if we got a "," */
	 } ;
	if (tcb->opcode == V_OpCode_CommaComma)		/* Same as ',' but don't save next point (useful if you want to quickly disable error trapping for debugging purposes) */
	 {
	   if(!v4dpi_PointParse(ctx,&tp2,tcb,flags&~V4DPI_PointParse_InColonEq)) goto fail ;
	   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ; if (tcb->type == V4LEX_TknType_Error) { UCstrcpy(ctx->ErrorMsgAux,tcb->UCstring) ; goto fail ; } ;/* Look to see if end of Isct or if we got a "," */
	 } ;
/*	If NOT parsing { ... } then look for slash */
/*	Converting "[isct] / pt" => "EnumCL(pt @[isct])" */
	if (tcb->opcode == V_OpCode_Slash && !(inexp || (lflags & V4DPI_PointParse_InColonEq)))
	 { P *ip ;
	   memcpy(&tp2,point,point->Bytes) ;		/* Copy initial isct into temp */
	   QUOTE(&tp2) ;				/* Make first point quoted */
	   ZPH(&tp1) ; tp1.PntType = V4DPI_PntType_IntMod ; tp1.Dim = Dim_IntMod ;
	   tp1.Value.IntVal = V4IM_OpCode_EnumCL ;
	   tp1.Bytes = V4PS_Int ;
	   INITISCT(point) ; ISCTVCD(point) ; point->Grouping = 3 ; ip = ISCT1STPNT(point) ;
	  
	   memcpy(ip,&tp1,tp1.Bytes) ; point->Bytes += ip->Bytes ;
	   ADVPNT(ip) ; if(!v4dpi_PointParse(ctx,ip,tcb,lflags&~V4DPI_PointParse_InColonEq)) goto fail ;	/* Parse point after slash */
	   point->Bytes += ip->Bytes ;
	   ADVPNT(ip) ; memcpy(ip,&tp2,tp2.Bytes) ; point->Bytes += tp2.Bytes ;
	 }
	if (tcb->type != V4LEX_TknType_Error) { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; }
	 else { UCstrcpy(ctx->ErrorMsgAux,tcb->UCstring) ; goto fail ; } ;
	if (bibuf.DimUID != UNUSED)			/* Got a BigIsct - write it out */
	 { for(rx=gpi->HighHNum;rx>=gpi->LowHNum;rx--)	/* Find highest area allowing new entries */
	    { if (rx == V4DPI_WorkRelHNum) continue ;			/* Don't default to work area! */
	      if (gpi->RelH[rx].aid == UNUSED) continue ; if (gpi->RelH[rx].ahi.ExtDictUpd) break ;
	    } ;
	   if (rx < gpi->LowHNum && gpi->RelH[V4DPI_WorkRelHNum].aid != UNUSED) /* If all else fails - assign to process work */
	    rx = V4DPI_WorkRelHNum ;
	   if (rx < gpi->LowHNum)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"CtxNoUpdArea1") ; goto fail ; } ;
	   bibuf.Bytes = (char *)&point->Value.Isct.pntBuf[nextfree] - (char *)&bibuf ;
	   FINDAREAINPROCESS(areax,gpi->RelH[rx].aid) ;
	   GRABAREAMTLOCK(areax) ;
	   v4is_Insert(gpi->RelH[rx].aid,(struct V4IS__Key *)&bibuf,&bibuf,bibuf.Bytes,V4IS_PCB_DataMode_Auto,0,0,FALSE,FALSE,0,NULL) ;
	   FREEAREAMTLOCK(areax) ;
	 } ;
	v4dpi_PopLocalSyms(localSymLevel) ;
	return(TRUE) ;

fail:
	if (localSymLevel != UNUSED) v4dpi_PopLocalSyms(localSymLevel) ;
	return(FALSE) ;
}

#define V4DPI_SYM_VALUE_MAX 500
#define V4DPI_SYM_VALUE_LEVEL_MAX 50
struct lcl__symValues {
  COUNTER count ;				/* Number below */
  COUNTER lastLevelId ;				/* Last assigned level id/number */
  COUNTER lastSymId ;				/* Last assigned symbol id/number */
  INDEX nLevel ;				/* Nested level index */
  COUNTER levelSymId[V4DPI_SYM_VALUE_LEVEL_MAX] ; /* SymId for each level */
  INDEX hNum ;					/* Hierarchy number to be used with symbol Ids to guarantee uniqueness */
  struct {
   UCCHAR symName[V4LEX_Tkn_KeywordMax+1] ;	/* Symbol name */
   COUNTER levelId ;				/* Level id for this entry */
   SYMID symId ;				/* Symbol Id */
   } entry[V4DPI_SYM_VALUE_MAX] ;
} ;

static struct lcl__symValues *lsv = NULL ;

INDEX v4dpi_NewLevelLocalSym()
{ INDEX i ;

/*	Set up lsv structure if necessary */
	if (lsv == NULL)
	 { lsv = (struct lcl__symValues *)v4mm_AllocChunk(sizeof *lsv,FALSE) ;
	   lsv->count = 0 ; lsv->lastLevelId = 0 ; lsv->lastSymId = 0 ; lsv->nLevel = 0 ;
/*	   Go through open areas to find the one open for update - use its HNUM */
	   for(i=gpi->LowHNum;i<=gpi->HighHNum;i++)
	    { if (gpi->RelH[i].aid == UNUSED) continue ;
	      if ((gpi->RelH[i].pcb->AccessMode & V4IS_PCB_AM_Update) != 0) break ;
	    } ; lsv->hNum = (i > gpi->HighHNum ? 0 : i) ;
	 } ;
	if (lsv->nLevel >= V4DPI_SYM_VALUE_LEVEL_MAX)
	 return(UNUSED) ;
	lsv->nLevel++ ;
	lsv->levelSymId[lsv->nLevel] = ++lsv->lastLevelId ;
	return(lsv->nLevel) ;
}

/*	v4dpi_PushLocalSym - Gets (and updates) $symbol						*/
/*	Call: logical = v4dpi_PushLocalSym( ctx , symName , respnt , valPnt , lsOp )
	  where ctx is context,
		symName is $xxx name,
		respnt is updated with the value of the symbol,
		valPnt, if not NULL is the new value,
		lsOp is 'opcode': 0=assign, 1=add to , 2=subtract from				*/

LOGICAL v4dpi_PushLocalSym(ctx,symName,respnt,valPnt,lsOp)
  struct V4C__Context *ctx ;
  UCCHAR *symName ;
  P *respnt, *valPnt ;
  ETYPE lsOp ;
{ INDEX i ; INDEX levelId ;


	if (lsv == NULL)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"DPISymNoLevel",symName) ; return(FALSE) ; } ;
	levelId = lsv->levelSymId[lsv->nLevel] ;
/*	Look for the symbol */
	for(i=0;i<lsv->count;i++) { if (UCstrcmpIC(symName,lsv->entry[i].symName) == 0) break ; } ;
/*	Are we defining new symbol (valPnt != NULL) OR looking one up? */
	if (valPnt == NULL)
	 { if (i >= lsv->count)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"DPISymNotFound",symName) ; return(FALSE) ; } ;
	   symdefPNTv(respnt,lsv->entry[i].symId,symName,NULL,0) ;
	 } else
	 { if (i < lsv->count)
	    { symdefPNTv(respnt,lsv->entry[i].symId,symName,valPnt,lsOp) ;
	      return(TRUE) ;
//	      v_Msg(ctx,ctx->ErrorMsgAux,"DPISymExists",symName) ; return(FALSE) ;
	    } ;
	   if (lsOp != 0)	/* Can't += or -= to an undefined $xxx */
	    { v_Msg(ctx,ctx->ErrorMsgAux,"DPISymNotFound",symName) ; return(FALSE) ; } ;
	   if (lsv->count >= V4DPI_SYM_VALUE_MAX) { v_Msg(ctx,ctx->ErrorMsgAux,"DPISymMax",symName,V4DPI_SYM_VALUE_MAX) ; return(FALSE) ; } ;
	   UCstrcpy(lsv->entry[lsv->count].symName,symName) ;
	   lsv->entry[lsv->count].levelId = levelId ;
	   lsv->lastSymId++ ; lsv->entry[lsv->count].symId = ADDHNUMDICT(lsv->hNum,lsv->lastSymId) ;
	   symdefPNTv(respnt,lsv->entry[lsv->count].symId,symName,valPnt,0) ;
	   lsv->count++ ;
	 } ;
	return(TRUE) ;
}

void v4dpi_PopLocalSyms(localSymLevel)
  INDEX localSymLevel ;
{ INDEX i ; COUNTER levelId ;

	if (localSymLevel <= 0 || lsv == NULL) return ;
	levelId = lsv->levelSymId[localSymLevel] ;
	lsv->nLevel = localSymLevel-1 ;
	for(i=0;i<lsv->count;i++)
	 { if (lsv->entry[i].levelId == levelId)
	    { lsv->count = i ; break ; } ;
	 } ;
}

/*	v4dpi_MakeBigIsct - Makes a B I G  intersection !			*/
/*	Call: newpt = v4dpi_MakeBigIsct( ctx , bibuf , pnt )
	  where newpt is pointer to new point, NULL if problems,
		ctx is context,
		bibuf is pointer to big-intersection buffer (struct V4DPI__BigIsct),
		oldpnt is pointer to current isct point (that needs to be made bigger),
		nextfree is index to next free slot in oldpnt,
		newpnt is pointer to point to be added (NOTE: point not added, its Bytes is used for checking),
		maxsize is pointer to int & updated with new max size		*/

P *v4dpi_MakeBigIsct(ctx,bibuf,oldpnt,nextfree,newpnt,maxsize)
  struct V4C__Context *ctx ;
  struct V4DPI__BigIsct *bibuf ;
  P *oldpnt, *newpnt ;
  int nextfree, *maxsize ;
{ struct V4DPI__DimInfo *di ;
  union V4DPI__IsctSrc vis ;
  
	if (newpnt->Bytes + nextfree >= V4DPI_BigIsctBufMax)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsIsctBigMx",newpnt->Bytes,nextfree,newpnt->Bytes+nextfree,V4DPI_BigIsctBufMax) ; return(NULL) ; } ;
	bibuf->kp.fld.KeyType = V4IS_KeyType_V4 ; bibuf->kp.fld.KeyMode = V4IS_KeyMode_Int ;
	bibuf->kp.fld.AuxVal = V4IS_SubType_BigIsct ; bibuf->kp.fld.Bytes = V4IS_IntKey_Bytes ;
	DIMINFO(di,ctx,Dim_Isct) ;
	if (di == NULL)
	 { vout_UCText(VOUT_Warn,0,UClit("*No ISCT dimension defined for BigIsct, defaulting to Dim:INT\n")) ;
		 DIMINFO(di,ctx,Dim_Int) ;
	 } ;
	di->UniqueOK = V4DPI_DimInfo_UOkInt ;			/* Force this so DimUnique() does not fail */
	bibuf->DimUID = v4dpi_DimUnique(ctx,di,NULL) ;
	if (bibuf->DimUID == UNUSED) return(NULL) ;
	memcpy(&bibuf->biPnt,oldpnt,sizeof *oldpnt) ;		/* Copy oldpnt (so far) into BigIsct */
	vis = oldpnt->Value.Isct.vis ;
	ZPH(oldpnt) ; oldpnt->PntType = V4DPI_PntType_BigIsct ;	/* Redefine oldpnt as BigIsct link */
	oldpnt->Bytes = V4PS_BigIsct ;
	oldpnt->Value.BigIsct.vis = vis ;
	oldpnt->Value.BigIsct.DimUID = bibuf->DimUID ;
	oldpnt = (struct V4DPI__Point *)&bibuf->biPnt ;		/* Now link up to oldpnt in BigIsct */
	*maxsize = V4DPI_BigIsctBufMax ;			/*  & up the max buffer size */
	return(oldpnt) ;
}

/*	v4dpi_ParseArray - Parses an intersection					*/
/*	Call: ok = v4dpi_ParseArray( point , tcb , ctx , arrayName , isDollarSym )
	  where ok is result of parse, TRUE if ok, FALSE if problem (in ErrorMsgAux),
		point is point to be updated,
		tcb is token control block,
		ctx is context
		arrayName is UCCHAR array name
		isDollarSym is TRUE if arrayName is '$arrayName', FALSE otherwise	*/
LOGICAL v4dpi_ParseArray(point,tcb,ctx,arrayName,isDollarSym)
  struct V4DPI__Point *point ;
  struct V4LEX__TknCtrlBlk *tcb ;
  struct V4C__Context *ctx ;
  UCCHAR *arrayName ;
  LOGICAL isDollarSym ;
{ struct V4DPI__DimInfo *di ;
  P apnt,tpnt,*pt,*ipt ;
  COUNTER cnt ; UCCHAR dclbuf[V4TMBufMax] ; LOGICAL ok ;
/*	Want to convert arrayName[ . . . ] -> Array([arrayName] . . . )
	OR
	convert arrayName[Dim:xxx . . .] -> BindQE([arrayName] Array(Dim:xxx . . . ) */
	if (!v4dpi_PointParse(ctx,&apnt,tcb,V4DPI_PointParse_RetFalse)) return(FALSE) ;
	if (apnt.Dim == Dim_Dim)			/* Which form do we have?) */
	 goto dclArray ;
/*	Here just to reference an array: Array([apnt] . . .) */
	INITISCT(point) ; NOISCTVCD(point) ; point->Grouping = 2 ;		/* Construct Array() intersection */
	pt = &tpnt ; ZPH(pt) ; pt->PntType = V4DPI_PntType_IntMod ; pt->Bytes = V4PS_Int ; pt->Value.IntVal = V4IM_OpCode_Array ; pt->Dim = Dim_IntMod ;
	ipt = ISCT1STPNT(point) ; memcpy(ipt,pt,pt->Bytes) ; ADVPNT(ipt) ;
/*	If we got $xxxx then parse as such, otherwise construct isct containing arrayName as point on NId dimension */
	if (isDollarSym)
	 { v4dpi_PushLocalSym(ctx,arrayName,ipt,NULL,0) ; }
	 else { INITISCT(&tpnt) ; NOISCTVCD(&tpnt) ; tpnt.Grouping = 1 ;
		pt = ISCT1STPNT(&tpnt) ; DIMINFO(di,ctx,Dim_NId) ; dictPNTv(pt,Dim_NId,v4dpi_DictEntryGet(ctx,0,arrayName,di,NULL)) ;
		ADVPNT(pt) ; ISCTLEN(&tpnt,pt) ; memcpy(ipt,&tpnt,tpnt.Bytes) ;
	      } ;
/*	Pick up the remaining arguments (up to closing bracket) */
	for(cnt=0;cnt<V4DPI_MDArray_Max+1;cnt++)				/* Allow +1 because last argument may be Value::xxx */
	 { ADVPNT(ipt) ; memcpy(ipt,&apnt,apnt.Bytes) ;	point->Grouping++ ;
	   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ; if (tcb->opcode == V_OpCode_RBracket) break ;
	   v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
	   if (!v4dpi_PointParse(ctx,&apnt,tcb,V4DPI_PointParse_RetFalse)) return(FALSE) ;
	 } ;
	 if (cnt >= V4DPI_MDArray_Max+1)					/* Allow +1 for last argument as value */
	  { v_Msg(ctx,ctx->ErrorMsgAux,"ArrayMaxDims",tcb->type) ; return(FALSE) ; } ;
	ADVPNT(ipt) ; ISCTLEN(point,ipt) ;
 //v_Msg(ctx,ctx->ErrorMsg,"@*Syntax sugar: %1P\n",point) ; vout_UCText(VOUT_Trace,0,ctx->ErrorMsg) ;
	return(TRUE) ;
/*	Here to declare a new array - most easily by creating string and then parsing it */
dclArray:
	v_Msg(ctx,dclbuf,"@BindQE([%1U] Array(%2P ",arrayName,&apnt) ;
	for(cnt=0;cnt<V4DPI_MDArray_Max+1;cnt++)				/* Allow +1 because last argument may be Value::xxx */
	 { v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ; if (tcb->opcode == V_OpCode_RBracket) break ;
	   v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
	   if (!v4dpi_PointParse(ctx,&apnt,tcb,V4DPI_PointParse_RetFalse)) return(FALSE) ;
	   UCstrcat(dclbuf,UClit(" ")) ; v4dpi_PointToStringML(&dclbuf[UCstrlen(dclbuf)],&apnt,ctx,V4DPI_FormatOpt_Dump,500) ;
//	   v_Msg(ctx,&dclbuf[UCstrlen(dclbuf)],"@ %1P",&apnt) ;
	 } ;
	 if (cnt >= V4DPI_MDArray_Max+1)					/* Allow +1 for last argument as value */
	  { v_Msg(ctx,ctx->ErrorMsgAux,"ArrayMaxDims",tcb->type) ; return(FALSE) ; } ;
	UCstrcat(dclbuf,UClit("))\n")) ;
	v4lex_NestInput(tcb,NULL,dclbuf,V4LEX_InpMode_String) ;
	ok = v4dpi_PointParse(ctx,point,tcb,V4DPI_PointParse_RetFalse) ;
	return(ok) ;
}

/*	v4dpi_PointAccept - Accepts (Converts) Single Point Value	*/
/*	Call: result = v4dpi_PointAccept( point , diminfo , tcb , ctx, flags )
	  where result is the result code,
		point is pointer to hold results of "acceptance",
		diminfo is pointer to dimension info- V4DPI__DimInfo,
		tcb is pointer to token control block,
		ctx is pointer to current context,
		flags are parsing flags (V4DPI_PointParse_xxx)		*/

LOGICAL v4dpi_PointAccept(point,diminfo,tcb,ctx,flags)
  struct V4DPI__Point *point ;
  struct V4DPI__DimInfo *diminfo ;
  struct V4C__Context *ctx ;
  int flags ;
  struct V4LEX__TknCtrlBlk *tcb ;
{ struct V4DPI__Point *npt,isctbuf ;
  struct V4FFI__DataElSpec *des ;
  struct V4FFI__StructElSpec *ses ;
  static struct V4LEX__TknCtrlBlk *tcbx = NULL ;
  struct V4LEX__BigText *bt ;
  struct V4L__ListPoint *lp ;
  struct V4LEX__TablePrsOpt *tpo ;
  struct V4DPI__UOMTable *uomt ;
  struct V4DPI_UOMDetails *uomd ;
  static struct V4CI__CountryInfo *ci = NULL ;
  regex_t *repattern ;
  YMDORDER *ymdo ;
#define ESTRLEN 2048
  UCCHAR estr[ESTRLEN],*TknStart,arg[250] ;
  UCCHAR comment[16] ;
  int savebytesappend,savemaxappend ; UCCHAR *saveappendto ;
  int i,brace,y,m,ok,num ; B64INT b64,b64a ; double d1 ;
  UCCHAR *ucb,ucbuf[2048], *bp ; char *emne ;
#define SYNERR(MSG) { ucb = UClit(MSG) ; goto PointAcceptErr ; }
#define SYNERRMSG(ERRMNE) { emne = ERRMNE ; goto PointAcceptErrMne ; }

	TknStart = v4lex_TknPtr(tcb,V4LEX_TknOp_CurrentPtr,NULL,&i) ;	/* Save current start of token */
	if ((diminfo->ds.Alpha.aFlags & V4DPI_DimInfoAlpha_IsFileName) != 0 && diminfo->PointType == V4DPI_PntType_Char) goto parse_value ;
	v4lex_NextTkn(tcb,V4LEX_Option_RetEOL+V4LEX_Option_ForceKW) ;
	if (tcb->type == V4LEX_TknType_Error) { UCstrcpy(ctx->ErrorMsgAux,tcb->UCstring) ; return(FALSE) ; } ;
	ZPH(point) ;
	if (tcb->opcode == V_OpCode_DotDot)			/* Got: xxx:.. - convert to xxx:{all} */
	 { point->Grouping = V4DPI_Grouping_All ; point->PntType = V4DPI_PntType_Special ; point->Bytes = V4DPI_PointHdr_Bytes ;
	   point->Dim = diminfo->DimId ; return(TRUE) ;
	 } ;
	if (diminfo->Flags & V4DPI_DimInfo_Acceptor)
	 { if (tcb->opcode != V_OpCode_Pound)			/* If followed by # then don't do acceptor logic */
	    { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;		/* Push current token back onto input stream */
	      v4lex_NextTkn(tcb,V4LEX_Option_NextChunk) ;	/* Grab next chunk as the token - have to assume acceptor working with larger chunk (ex: dim:100.23 -> "100.23" not "100") */
/*	      Create intersection of form: [UV4:Acceptor xxx Alpha:string-to-parse] */
	      if (diminfo->ADPnt.Bytes != 0)
	       { v_Msg(ctx,ucbuf,"@[%1D:%2S %3U %4D:\"%5U\"]",Dim_UV4,DE(Acceptor),diminfo->ADPntStr,Dim_Alpha,tcb->UCstring) ;
	       } else
	       { v_Msg(ctx,ucbuf,"@[%1D:%2S %3D:%4U %5D:\"%6U\"]",
			Dim_UV4,DE(Acceptor),Dim_Dim,(UCstrlen(diminfo->IsA) > 0 ? diminfo->IsA : diminfo->DimName),Dim_Alpha,(tcb->type == V4LEX_TknType_Keyword ? tcb->UCkeyword : tcb->UCstring)) ;
	       } ;
	      if (tcbx != NULL) { v4lex_ResetTCB(tcbx) ; }
	       else { tcbx = (struct V4LEX__TknCtrlBlk *)v4mm_AllocChunk(sizeof *tcbx,FALSE) ; v4lex_InitTCB(tcbx,0) ; } ;
	      v4lex_NestInput(tcbx,NULL,ucbuf,V4LEX_InpMode_String) ;
	      if(!v4dpi_PointParse(ctx,&isctbuf,tcbx,0)) return(FALSE) ;
	      npt = v4dpi_IsctEval(point,&isctbuf,ctx,0,NULL,NULL) ;
	      UCstrcpy(ucdimbuf,diminfo->DimName) ;	/* Reset dimbuf just in case */
	      if (npt != NULL) return(TRUE) ;
	      ucb = estr ; v_Msg(ctx,estr,"DPIAcpAcceptor",ucbuf,ctx->ErrorMsg) ; goto PointAcceptErr ;
	    } else
	    { v4lex_NextTkn(tcb,V4LEX_Option_ForceKW) ;		/* Have #xxx - get rid of "#" & advance to xxx, fall thru */
	      if (tcb->type == V4LEX_TknType_Error) { UCstrcpy(ctx->ErrorMsgAux,tcb->UCstring) ; return(FALSE) ; } ;
	    } ;
	 } ;
/*	Look for some special values before point-type specific parsing */
	if (tcb->opcode == V_OpCode_LBrace || tcb->opcode == V_OpCode_NCLBrace)
	 { v4lex_NextTkn(tcb,0) ;
	   if (tcb->type == V4LEX_TknType_Error) { UCstrcpy(ctx->ErrorMsgAux,tcb->UCstring) ; return(FALSE) ; } ;
	   point->Bytes = V4DPI_PointHdr_Bytes ; point->Dim = diminfo->DimId ;
	   if (tcb->opcode == V_OpCode_Star || tcb->opcode == V_OpCode_NCStar)
	    { point->Grouping = V4DPI_Grouping_Current ; point->PntType = V4DPI_PntType_Special ;
	    } else if (tcb->opcode == V_OpCode_StarStar)
		    { point->Grouping = V4DPI_Grouping_PCurrent ; point->PntType = V4DPI_PntType_Special ;
	    } else {
		     switch (v4im_GetUCStrToEnumVal(tcb->UCkeyword,0))
		      { default:	SYNERR("Unknown \"{xxx}\" type") ;
		        case _All:	point->Grouping = V4DPI_Grouping_All ; point->PntType = V4DPI_PntType_Special ; break ;
		        case _Context:	point->Grouping = V4DPI_Grouping_Current ; point->PntType = V4DPI_PntType_Special ; break ;
		        case _New:	
			  if (!diminfo->UniqueOK) SYNERR("{NEW} not allowed for this dimension") ;
			  point->PntType = diminfo->PointType ;
			  if (point->PntType == V4DPI_PntType_AggRef)	/* If an AggRef then pick first updateable Agg */
			   { for(i=0;i<gpi->AreaAggCount;i++)
			      { if (gpi->AreaAgg[i].pcb == NULL) continue ;
			        if ((gpi->AreaAgg[i].pcb->AccessMode & (V4IS_PCB_AM_Insert | V4IS_PCB_AM_Update)) != 0) break ;
			       } ;
			     if (i >= gpi->AreaAggCount) i = 0 ;
			      point->Grouping = i ;
			   } else { point->Grouping = V4DPI_Grouping_Single ; } ;
			  point->Value.IntVal = v4dpi_DimUnique(ctx,diminfo,NULL) ;
			  if (point->Value.IntVal == UNUSED) { ucb = estr ; UCstrcpy(ucb,ctx->ErrorMsgAux) ; goto PointAcceptErr ; } ;
			  point->Bytes = V4PS_Int ; break ;
		        case _Current:
		        case _Now:	
			  switch(diminfo->PointType)
			   { default:	SYNERRMSG("DPIMNotTime") ;
			      case V4DPI_PntType_Time:
				point->Grouping = V4DPI_Grouping_Single ; point->PntType = diminfo->PointType ;
				point->Value.IntVal = time(0) ; break ; 		/* Get current time! */
			      case V4DPI_PntType_UDate: case V4DPI_PntType_UMonth: case V4DPI_PntType_UDT: case V4DPI_PntType_UYear:
			      case V4DPI_PntType_UWeek: case V4DPI_PntType_UQuarter: case V4DPI_PntType_UPeriod: case V4DPI_PntType_UTime:
			      case V4DPI_PntType_Calendar:
				point->Grouping = V4DPI_Grouping_Now ; point->PntType = V4DPI_PntType_Special ;
				break ;
			   } ;
			  point->Bytes = V4PS_Int ; break ;
		        case _Undefined: point->Grouping = V4DPI_Grouping_Undefined ; point->PntType = V4DPI_PntType_Special ; break ;
		        case _Sample:	point->Grouping = V4DPI_Grouping_Sample ; point->PntType = V4DPI_PntType_Special ; break ;
		      } ;
		   } ;
	   v4lex_NextTkn(tcb,0) ;
	   if (tcb->opcode != V_OpCode_RBrace) SYNERR("Missing ending \"}\" in special symbol") ;
	   return(TRUE) ;
	 } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
parse_value:
	point->PntType = diminfo->PointType ;
	switch (diminfo->PointType)
	 { default: SYNERR("Unknown/invalid point type") ;
	   case V4DPI_PntType_V4IS:
		v4lex_NextTkn(tcb,0) ; if (!v_IsNoneLiteral(tcb->UCkeyword)) { SYNERR("Cannot explicitly specify V4IS point") ; } ;
		point->Value.V4IS.Handle = UNUSED ; point->Value.V4IS.DataId = UNUSED ; point->Bytes = V4PS_V4IS ;
		break ;
	   case V4DPI_PntType_XDB:
		v4lex_NextTkn(tcb,0) ; if (!v_IsNoneLiteral(tcb->UCkeyword)) { SYNERR("Cannot explicitly specify XDB point") ; } ;
		point->Bytes = V4PS_XDB ; point->Value.XDB.xdbId = 0 ;
		break ;
	   case V4DPI_PntType_Complex:
		ok = FALSE ; v4lex_NextTkn(tcb,V4LEX_Option_NegLit+V4LEX_Option_WantDbl) ;
		if (tcb->opcode == V_OpCode_LParen || tcb->opcode == V_OpCode_NCLParen)
		 { ok = TRUE ; v4lex_NextTkn(tcb,V4LEX_Option_NegLit+V4LEX_Option_WantDbl) ; } ;
		if (tcb->type == V4LEX_TknType_Integer ||tcb->type == V4LEX_TknType_LInteger)
		 { FIXEDtoDOUBLE(tcb->floating,tcb->Linteger,tcb->decimal_places) ;
		 } else if (tcb->type != V4LEX_TknType_Float) SYNERRMSG("DPIMNotFP") ;
		memcpy(&point->Value.Complex.r,&tcb->floating,sizeof(tcb->floating)) ;
		v4lex_NextTkn(tcb,V4LEX_Option_NegLit+V4LEX_Option_WantDbl) ;
		if (tcb->opcode == V_OpCode_Comma) v4lex_NextTkn(tcb,V4LEX_Option_NegLit+V4LEX_Option_WantDbl) ;
		if (tcb->type == V4LEX_TknType_Integer ||tcb->type == V4LEX_TknType_LInteger)
		 { FIXEDtoDOUBLE(tcb->floating,tcb->Linteger,tcb->decimal_places) ;
		 } else if (tcb->type != V4LEX_TknType_Float) SYNERRMSG("DPIMNotFP") ;
		memcpy(&point->Value.Complex.i,&tcb->floating,sizeof(tcb->floating)) ;
		if (ok)						/* Check for ending paren ? */
		 { v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
		   if (tcb->opcode != V_OpCode_RParen) SYNERR("Expecting closing \")\" on complex number") ;
		 } ;
		point->Bytes = V4PS_Complex ;
		return(TRUE) ;
	   case V4DPI_PntType_GeoCoord:			/*  */
		v4lex_NextTkn(tcb,0) ;
		if (tcb->type == V4LEX_TknType_Keyword || tcb->type == V4LEX_TknType_String)
		 { if (!v_ParseGeoCoord(ctx,point,diminfo,tcb->UCstring,estr))
		    { ucb = estr ; goto PointAcceptErr ; } ;
		   return(TRUE) ;
		 } ;
		ucb = UCstrchr(tcb->ilvl[tcb->ilx].input_ptr,')') ;
		if (tcb->opcode != V_OpCode_NCLParen || ucb == NULL) SYNERR("Expecting coordinate to begin with \"(\" and end with \")\"") ;
		num = ucb - tcb->ilvl[tcb->ilx].input_ptr ;
		UCstrncpy(ucbuf,tcb->ilvl[tcb->ilx].input_ptr,num) ; ucbuf[num] = UCEOS ;
		tcb->ilvl[tcb->ilx].input_ptr = ucb + 1 ;
		if (!v_ParseGeoCoord(ctx,point,diminfo,ucbuf,estr))
		 { ucb = estr ; goto PointAcceptErr ; } ;
		return(TRUE) ;
	   case V4DPI_PntType_TeleNum:
		v4lex_NextTkn(tcb,V4LEX_Option_NextChunk|V4LEX_Option_RetEOL) ;
		if (!v_parseTeleNum(ctx,diminfo,point,NULL,tcb->UCstring,ucbuf)) { ucb = ucbuf ; goto PointAcceptErr ; } ;
		break ;
	   case V4DPI_PntType_Int2:
		v4lex_NextTkn(tcb,0) ;
		if (tcb->opcode != V_OpCode_Langle && tcb->opcode != V_OpCode_NCLParen)
		 { if (v_IsNoneLiteral(tcb->UCkeyword))
		    { point->PntType = V4DPI_PntType_Special ; point->Grouping = V4DPI_Grouping_None ;
		      point->Bytes = V4PS_SpecNone ; return(TRUE) ;
		    } ;
		   SYNERRMSG("DPIMBadInt2") ;
		 }
		v4lex_NextTkn(tcb,V4LEX_Option_NegLit) ;
		if (tcb->type != V4LEX_TknType_Integer) SYNERRMSG("DPIMNotInt") ;
		point->Value.Int2Val[0] = tcb->integer ;
		v4lex_NextTkn(tcb,V4LEX_Option_NegLit) ;
		if (tcb->type != V4LEX_TknType_Integer) SYNERRMSG("DPIMNotInt") ;
		point->Value.Int2Val[1] = tcb->integer ;
		if (diminfo->Flags & V4DPI_DimInfo_Normalize)
		 { if (point->Value.Int2Val[0] > point->Value.Int2Val[1])
		    { int t ; t = point->Value.Int2Val[0] ; point->Value.Int2Val[0] = point->Value.Int2Val[1] ; point->Value.Int2Val[1] = t ;
		    } ;
		 } ;
		point->Bytes = V4PS_Int2 ;
		v4lex_NextTkn(tcb,0) ;
		if (tcb->opcode != V_OpCode_Rangle && tcb->opcode != V_OpCode_RParen) SYNERRMSG("DPIMBadInt2") ;
		break ;
	   case V4DPI_PntType_UWeek:
		v4lex_NextTkn(tcb,0) ;
		if (tcb->opcode == V_OpCode_Pound)		/* If dim:#num then assume number in internal format */
		 { v4lex_NextTkn(tcb,V4LEX_Option_NegLit) ;
		   if (tcb->type != V4LEX_TknType_Integer) SYNERRMSG("DPIMNotInt") ;
		   point->Value.IntVal = tcb->integer ;
		 } else if (tcb->type == V4LEX_TknType_Keyword)
		 { if (!v_IsNoneLiteral(tcb->UCkeyword)) SYNERRMSG("DPIMNotInt") ;
		   point->Value.IntVal = 0 ;
		 } else
		 { if (tcb->type != V4LEX_TknType_Integer)
		    { if (v4sxi_SpecialAcceptor(ctx,point,diminfo,tcb->UCstring)) break ;
		      SYNERRMSG("DPIMNotInt") ;
		    } ;
		   if (diminfo->ds.UWeek.baseUDate != VCAL_UDate_None)
		    { point->Value.IntVal = tcb->integer ;
		    } else
		    { y = tcb->integer/100 ; VCALADJYEAR(((diminfo->ds.UWeek.calFlags & VCAL_Flags_Historical) != 0),y) ; m = tcb->integer%100 ;
		      if (y < VCAL_UYearMin || y > VCAL_UYearMax) SYNERRMSG("DPIMBadYear") ;
		      if (m < 1 || m > 52) SYNERR("Invalid week") ;
		      point->Value.IntVal = YYWWtoUWEEK(y,m) ;
		    } ;
		 } ;
		if (diminfo->ds.UWeek.baseUDate == VCAL_UDate_None && (diminfo->ds.UWeek.calFlags & VCAL_Flags_Historical) != 0)
		 { 
		   y = UWEEKtoUYEAR(point->Value.IntVal) ;
		   if (y > gpi->curYear) SYNERRMSG("DPIMNoFuture") ;
		 } ;
		point->Bytes = V4PS_Int ;
		break ;
	   case V4DPI_PntType_UPeriod:
		v4lex_NextTkn(tcb,V4LEX_Option_NextChunk|V4LEX_Option_RetEOL) ;
		point->Value.IntVal = v_ParseUPeriod(ctx,tcb->UCstring,diminfo,UCTBUF1) ;
		if (point->Value.IntVal == VCAL_BadVal)		/* Enough already: Not valid - see if matches special values */
		 { if (v4sxi_SpecialAcceptor(ctx,point,diminfo,tcb->UCstring)) break ;
		   ucb = estr ; UCstrcpy(ucb,UCTBUF1) ; goto PointAcceptErr ;
		 } ;
		point->Bytes = V4PS_Int ;
		break ;
	   case V4DPI_PntType_UYear:
		point->Bytes = V4PS_Int ;
		v4lex_NextTkn(tcb,0) ;
		if (tcb->type != V4LEX_TknType_Integer)
		 { if (tcb->type == V4LEX_TknType_Keyword || tcb->type == V4LEX_TknType_String)
		    { if (!v_IsNoneLiteral(tcb->UCstring))
		       { if (v4sxi_SpecialAcceptor(ctx,point,diminfo,tcb->UCstring)) break ;
			 if ((i=UCstrcmpIC(tcb->UCstring,gpi->ci->li->LI[gpi->ci->li->CurX].NowUC)) == 0)
			  { tcb->integer = valUYEARisNOW ;
			    v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;			/* See if we have a continuation with './+/- relative expression */
			    if (tcb->opcode == V_OpCode_Plus || tcb->opcode == V_OpCode_Minus || tcb->opcode == V_OpCode_Dot)
			     { PNTTYPE pntType ;
			       v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; v4lex_NextTkn(tcb,V4LEX_Option_NextChunk|V4LEX_Option_RetEOL) ;
			       if (v_CalCalc(ctx,tcb->UCstring,tcb->integer,V4DPI_PntType_UYear,&tcb->integer,&pntType,FALSE) == NULL) { UCstrcpy(ucbuf,ctx->ErrorMsgAux) ; ucb = ucbuf ; goto PointAcceptErr ; } ;
			     } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
			    goto check_uyear ;
			  } ;
/*			 Before complaining, see if it is a string that is a year */
		         tcb->integer = UCstrtol(tcb->UCstring,&bp,10) ; if (*bp == UCEOS) goto check_uyear ;
			 SYNERRMSG("DPIMNotInt") ;
		       } ;
		      point->Value.IntVal = 0 ; return(TRUE) ;
		    } else if (tcb->opcode == V_OpCode_Pound)		/* If dim:#num then assume number in internal format */
		       { v4lex_NextTkn(tcb,V4LEX_Option_NegLit) ;
			 if (tcb->type != V4LEX_TknType_Integer) SYNERRMSG("DPIMNotInt") ;
			 point->Value.IntVal = tcb->integer ; return(TRUE) ;
		       }
		    else SYNERRMSG("DPIMNotInt") ;
		 } ;
check_uyear:	y = tcb->integer ;  VCALADJYEAR(((diminfo->ds.UYear.calFlags & VCAL_Flags_Historical) != 0),y) ; if (y < VCAL_UYearMin || y > VCAL_UYearMax) SYNERRMSG("DPIMBadYear") ;
		if ((diminfo->ds.UYear.calFlags & VCAL_Flags_Historical) != 0)
		 { if (y > gpi->curYear) SYNERRMSG("DPIMNoFuture") ;
		 } ;
		point->Value.IntVal = y ;
		break ;
	   case V4DPI_PntType_UQuarter:
		v4lex_NextTkn(tcb,V4LEX_Option_ForceKW) ;
		if (tcb->opcode == V_OpCode_Pound)		/* If dim:#num then assume number in internal format */
		 { v4lex_NextTkn(tcb,V4LEX_Option_NegLit) ;
		   if (tcb->type != V4LEX_TknType_Integer) SYNERRMSG("DPIMNotInt") ;
		   point->Value.IntVal = tcb->integer ;
		 } else if (tcb->type == V4LEX_TknType_Keyword || tcb->type == V4LEX_TknType_String)
		 { if (v_IsNoneLiteral(tcb->UCstring)) { point->Value.IntVal = 0 ; return(TRUE) ; } ;
		   if (v4sxi_SpecialAcceptor(ctx,point,diminfo,tcb->UCstring)) break ;
		   point->Value.IntVal = 0 ;
		   ucb = tcb->UCstring ; y = 0 ; m = UNUSED ;
		   for(;*ucb!=UCEOS;ucb++)
		    { switch(*ucb)
		       { case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'):
			 case UClit('5'): case UClit('6'): case UClit('7'): case UClit('8'): case UClit('9'):
			   if (m == UNUSED) { y = y * 10 + (*ucb - UClit('0')) ; }
			    else { m = m * 10 + (*ucb - UClit('0')) ; } ;
			    break ;
			 case UClit('Q'): case UClit('q'):
			   if (m != UNUSED) SYNERR("Invalid Quarter format") ;
			   m = 0 ; break ;
		       } ;
		    } ;
		   if (m == UNUSED) { m = y % 100 ; y /= 100 ; } ;
		   VCALADJYEAR(((diminfo->ds.UQuarter.calFlags & VCAL_Flags_Historical) != 0),y) ; if (y < VCAL_UYearMin || y > VCAL_UYearMax) SYNERRMSG("DPIMBadYear") ;
		   if (m < 1 || m > 4) SYNERR("Invalid quarter") ;
		   point->Value.IntVal = YYQQtoUQTR(y,m) ;
		   if ((diminfo->ds.UQuarter.calFlags & VCAL_Flags_Historical) != 0)
		    { int yyyymmdd = mscu_udate_to_yyyymmdd(valUDATEisNOW), qtr = YYYYMMDDtoUQTR(yyyymmdd) ;
		      if (point->Value.IntVal > qtr) SYNERRMSG("DPIMNoFuture") ;
		    } ;
		 } ;
		point->Bytes = V4PS_Int ;
		break ;
	   case V4DPI_PntType_UMonth:
		v4lex_NextTkn(tcb,V4LEX_Option_NextChunk|V4LEX_Option_RetEOL) ;
		if (tcb->keyword_len == 0) SYNERRMSG("DPIMNoValue") ;
		if (UCstrchr(tcb->UCstring,'/') != NULL) { i = V4LEX_TablePT_MMYY ; }
		 else if (UCstrchr(tcb->UCstring,'-') != NULL) { i = V4LEX_TablePT_MMM ; }
		 else { i = (diminfo->ds.UMonth.IFormat != 0 ? diminfo->ds.UMonth.IFormat : V4LEX_TablePT_YYMM) ; } ;
		point->Value.IntVal = v_ParseUMonth(ctx,tcb->UCstring,i) ;
		if (point->Value.IntVal == VCAL_BadVal)
		 { UCCHAR *rb, saveb ;
		   if (v4sxi_SpecialAcceptor(ctx,point,diminfo,tcb->UCstring)) break ;
/*		   Loook for 'current' plus anything after it */
		   rb = UCstrpbrk(tcb->UCstring,UClit(".+-")) ; if (rb != NULL) { saveb = *rb ; *rb = UCEOS ; } ;
		   if ((i=UCstrcmpIC(tcb->UCstring,gpi->ci->li->LI[gpi->ci->li->CurX].NowUC)) == 0)
		    { UDtoUMONTH(point->Value.IntVal,valUDATEisNOW) ;
		      if (rb != NULL)
		       { PNTTYPE pntType ;
		         *rb = saveb ;
			 if (v_CalCalc(ctx,rb,point->Value.IntVal,V4DPI_PntType_UMonth,&point->Value.IntVal,&pntType,FALSE) == NULL) { UCstrcpy(ucbuf,ctx->ErrorMsgAux) ; ucb = ucbuf ; goto PointAcceptErr ; } ;
		       } ;
		      goto check_umonth ;
		    } ;
		   ucb = estr ; UCstrcpy(ucb,ctx->ErrorMsgAux) ; goto PointAcceptErr ;
		 } ;
check_umonth:	if ((diminfo->ds.UMonth.calFlags & VCAL_Flags_Historical) != 0)
		 { int yyyymmdd = mscu_udate_to_yyyymmdd(valUDATEisNOW), um = YYMMtoUMONTH(yyyymmdd/10000,((yyyymmdd/100)%100)) ;
		   if (point->Value.IntVal > um) SYNERRMSG("DPIMNoFuture") ;
		 } ;
		point->Bytes = V4PS_Int ;
		break ;
	   case V4DPI_PntType_UDT:
		v4lex_NextTkn(tcb,V4LEX_Option_NextChunk|V4LEX_Option_RetEOL) ;
		if (tcb->keyword_len == 0) SYNERRMSG("DPIMNoValue") ;
		ymdo = (diminfo->ds.UDT.YMDOrder[0] == V4LEX_YMDOrder_None ? gpi->ci->Cntry[gpi->ci->CurX].ymdOrder : &diminfo->ds.UDT.YMDOrder[0]) ;
		for(i=0;i<VCAL_YMDOrderMax && ymdo[i]!=V4LEX_YMDOrder_None;i++)
		 { point->Value.IntVal = v_ParseUDT(ctx,tcb->UCstring,diminfo->ds.UDT.IFormat,ymdo[i],3,&num,((diminfo->ds.UDT.calFlags & VCAL_Flags_Historical) == 0)) ;
		   if (point->Value.IntVal != VCAL_BadVal || diminfo->ds.UDT.IFormat != V4LEX_TablePT_Default) break ;
		 } ;
		if (point->Value.IntVal == VCAL_BadVal)
		 { if (v4sxi_SpecialAcceptor(ctx,point,diminfo,tcb->UCstring)) break ;
		   ucb = estr ; UCstrcpy(ucb,ctx->ErrorMsgAux) ; goto PointAcceptErr ;
		 } ;
		if (num == 1)				/* If = 1 then got relative date & NO time (ex: TODAY) */
		 { point->Grouping = 1 ;		/* Return range of times - from midnight to 23:59:59 */
		   point->Value.Int2Val[1] = point->Value.Int2Val[0] + VCAL_SecsInDay - 1 ;
		   point->Bytes = V4PS_Int2 ; break ;
		 } ;
		point->Bytes = V4PS_Int ;
		break ;
	   case V4DPI_PntType_Calendar:
		v4lex_NextTkn(tcb,V4LEX_Option_NextChunk|V4LEX_Option_RetEOL) ;
		if (tcb->keyword_len == 0) SYNERRMSG("DPIMNoValue") ;
		ymdo = (diminfo->ds.Cal.YMDOrder[0] == V4LEX_YMDOrder_None ? gpi->ci->Cntry[gpi->ci->CurX].ymdOrder : &diminfo->ds.Cal.YMDOrder[0]) ;
		for(i=0;i<VCAL_YMDOrderMax && ymdo[i]!=V4LEX_YMDOrder_None;i++)
		 { d1 = v_ParseCalendar(ctx,tcb->UCstring,diminfo->ds.Cal.IFormat,ymdo[i],diminfo->ds.Cal.CalendarType,diminfo->ds.Cal.TimeZone,((diminfo->ds.Cal.calFlags & VCAL_Flags_Historical) == 0)) ;
		   if (d1 != VCAL_BadVal || diminfo->ds.Cal.IFormat != V4LEX_TablePT_Default) break ;
		 } ;
		if (d1 == VCAL_BadVal)
		 { PNTTYPE pntType ;
		   if (v_ParseRelativeUDate(ctx,tcb->UCstring,&i,3,((diminfo->ds.Cal.calFlags & VCAL_Flags_Historical) == 0),&pntType,NULL) == NULL)
		    { ucb = estr ; UCstrcpy(ucb,ctx->ErrorMsgAux) ; goto PointAcceptErr ; } ;
		   if (pntType != V4DPI_PntType_UDate)
		    { v_Msg(ctx,estr,"ParseDateRelDT",tcb->UCstring,pntType,V4DPI_PntType_UDate) ; goto PointAcceptErr ; } ;
		   d1 = VCal_MJDOffset + i ;
		 } ;
		point->Bytes = V4PS_Calendar ; PUTREAL(point,d1) ;
		break ;
	   case V4DPI_PntType_UDate:
		v4lex_NextTkn(tcb,V4LEX_Option_NextChunk|V4LEX_Option_RetEOL) ;
		if (tcb->type == V4LEX_TknType_Error || tcb->type == V4LEX_TknType_EOL || tcb->opcode == V_OpCode_EOF)
		 { tcb->keyword_len = 0 ; ZUS(tcb->UCstring) ; v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
//		if (tcb->keyword_len == 0) SYNERRMSG("DPIMNoValue") ;
		ymdo = (diminfo->ds.UDate.YMDOrder[0] == V4LEX_YMDOrder_None ? gpi->ci->Cntry[gpi->ci->CurX].ymdOrder : &diminfo->ds.UDate.YMDOrder[0]) ;
		for(i=0;i<VCAL_YMDOrderMax && ymdo[i]!=V4LEX_YMDOrder_None;i++)
		 { point->Value.IntVal = v_ParseUDate(ctx,tcb->UCstring,diminfo->ds.UDT.IFormat,ymdo[i],((diminfo->ds.UDate.calFlags & VCAL_Flags_Historical) == 0)) ;
		   if (point->Value.IntVal != VCAL_BadVal || diminfo->ds.UDate.IFormat != V4LEX_TablePT_Default) break ;
		   break ;
		 } ;
		if (point->Value.IntVal == VCAL_BadVal)		/* Enough already: Not valid - see if matches special values */
		 { if (v4sxi_SpecialAcceptor(ctx,point,diminfo,tcb->UCstring)) break ;
//		   if (v_ParseRelativeUDate(ctx,tcb->UCstring,&point->Value.IntVal,3,((diminfo->ds.UDate.calFlags & VCAL_Flags_Historical) == 0)) == NULL)
//		    { ucb = estr ; UCstrcpy(ucb,ctx->ErrorMsgAux) ; goto PointAcceptErr ; }
		   ucb = estr ; UCstrcpy(ucb,ctx->ErrorMsgAux) ; goto PointAcceptErr ;
		 } ;
		point->Bytes = V4PS_Int ;
		break ;
	   case V4DPI_PntType_BigText:			/* Parse bigtext: <hdr>text...</hdr> */
		tpo = NULL ; INITTPO ;
/*		Do a quick look-ahead to see if we have btdim:#internal-number or btdim:"xxxxx" */
		if (tcb->opcode != V_OpCode_EOX)	/* Don't bother if at end of line */
		 { v4lex_NextTkn(tcb,V4LEX_Option_NextChar) ;
		   if (tcb->UCkeyword[0] == UClit('#'))
		    { v4lex_NextTkn(tcb,0) ; v4lex_NextTkn(tcb,0) ;
		      if (tcb->type != V4LEX_TknType_Integer) SYNERRMSG("DPIMNotInt") ;
		      point->Bytes = V4PS_Int ; point->Value.IntVal = tcb->integer ;
		      break ;
		    } ;
		   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
		   if (tcb->type == V4LEX_TknType_String)	/* If got quoted string then do embedded BigText */
		    { if (!v4dpi_SaveBigTextPoint2(ctx ,tcb->UCstring,point,diminfo->DimId,TRUE))
		       { ucb = estr ; UCstrcpy(ucb,ctx->ErrorMsgAux) ; goto PointAcceptErr ; } ;
		      break ;
		    } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
		 } ;
		ZUS(comment) ; ZUS(arg)
		tpo->readLineOpts = V4LEX_ReadLine_NLAsIs | V4LEX_ReadLine_MTLAsIs | V4LEX_ReadLine_IndentAsIs ;




/*		If we have attributes for this dimension then use them */
		if (UCnotempty(diminfo->Attributes))
		 { UCstrcpy(ucbuf,diminfo->Attributes) ;
		   v_Msg(ctx,UCTBUF1,"@%1U>",ucbuf) ;			/* It is assumed we already got the leading '<' if we call v4dpi_ParseBigTextAttributes so we don't need to prepend it */
		   if (tcbx != NULL) { v4lex_ResetTCB(tcbx) ; }
		    else { tcbx = (struct V4LEX__TknCtrlBlk *)v4mm_AllocChunk(sizeof *tcbx,FALSE) ; v4lex_InitTCB(tcbx,0) ; } ;
		   v4lex_NestInput(tcbx,NULL,UCTBUF1,V4LEX_InpMode_String) ;
		   if (!v4dpi_ParseBigTextAttributes(ctx,tcbx,tpo,point,comment,arg,ucbuf)) { ucb = ucbuf ; goto PointAcceptErr ; } ;
//		   v4mm_FreeChunk(tcbx) ; tcbx = NULL ;
		 } ;
		if (tcb->opcode != V_OpCode_EOX)
		 v4lex_NextTkn(tcb,0) ;					/* Do we have a '<' ? */
		if (tcb->opcode == V_OpCode_Langle)
		 { if (!v4dpi_ParseBigTextAttributes(ctx,tcb,tpo,point,comment,arg,ucbuf))  { ucb = ucbuf ; goto PointAcceptErr ; } ;
		 } else
		 { if (UCempty(diminfo->Attributes)) SYNERR("Expecting bigtext \"<hdr>\"") ;		/* No? then error unless we got defaults above */
		   v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
		 } ;



//		v4lex_NextTkn(tcb,0) ; if (tcb->opcode != V_OpCode_Langle) SYNERR("Expecting bigtext \"<hdr>\"") ;
//		v4lex_NextTkn(tcb,0) ; if (tcb->type != V4LEX_TknType_Keyword) SYNERR("Expecting BigText \"<hdr>\"") ;
//		v_Msg(ctx,arg,"@</%1U>",tcb->UCkeyword) ;	/* Save the keyword */
//		for(ok=TRUE;ok;)				/* Parse any embedded attributes until ">" */
//		 { 
//		   switch (v4im_GetTCBtoEnumVal(ctx,tcb,0))
//		    { default:
//			if (tcb->opcode == V_OpCode_Rangle) { ok = FALSE ; continue ; } ;
//			SYNERR("Invalid BigText attribute keyword") ;
//		      case _NewLine:		i = 1 ; break ;
//		      case _Comment:		i = 2 ; break ;
//		      case _EmptyLine:		i = 3 ; break ;
//		      case _Indent:		i = 4 ; break ;
//		      case _Dim:
//		      case _Dimension:		i = 5 ; break ;
//		      case _Javascript:		tpo->readLineOpts |= V4LEX_ReadLine_Javascript ; continue ;
//		    } ;
//		   v4lex_NextTkn(tcb,0) ;
//		   if (tcb->opcode != V_OpCode_Equal) SYNERR("Expecting \"=\" after BigText keyword") ;
//		   v4lex_NextTkn(tcb,0) ;
//		   if (tcb->type == V4LEX_TknType_String)
//		    { if (UCstrlen(tcb->UCstring) >= UCsizeof(comment)) SYNERR("BigText comment string too long") ;
//		      UCstrcpy(tcb->UCkeyword,tcb->UCstring) ;
//		    } else { if (tcb->type != V4LEX_TknType_Keyword) SYNERR("Expecting BigText value") ;
//			   } ;
//		   switch (i)
//		    { case 1:	/* NEWLINE */
//			tpo->readLineOpts = (tpo->readLineOpts & ~V4LEX_ReadLine_NLAsIs) ;		/* Turn off default */
//			switch (v4im_GetUCStrToEnumVal(tcb->UCkeyword,0))
//			 { default:	SYNERR("Invalid BigText NewLine=xxx value") ;
//			   case _None:	tpo->readLineOpts |= V4LEX_ReadLine_NLNone ; break ;
//			   case _Input:	tpo->readLineOpts |= V4LEX_ReadLine_NLAsIs ; break ;
//			   case _NL:	tpo->readLineOpts |= V4LEX_ReadLine_NLNL ; break ;
//			 }
//			break ;
//		      case 2:	/* COMMENT */
//			UCstrcpy(comment,tcb->UCkeyword) ; comp = comment ;
//			break ;
//		      case 3:	/* EMPTYLINE */
//			tpo->readLineOpts = (tpo->readLineOpts & ~V4LEX_ReadLine_MTLAsIs) ;		/* Turn off default */
//			switch (v4im_GetUCStrToEnumVal(tcb->UCkeyword,0))
//			 { default:	SYNERR("Invalid BigText EmptyLine=xxx value") ;
//			   case _Keep:	tpo->readLineOpts |= V4LEX_ReadLine_MTLAsIs ; break ;
//			   case _Ignore:tpo->readLineOpts |= V4LEX_ReadLine_MTLTrash ; break ;
//			 }
//			break ;
//		      case 4:	/* INDENT */
//			tpo->readLineOpts = (tpo->readLineOpts & ~V4LEX_ReadLine_IndentAsIs) ;		/* Turn off default */
//			switch (v4im_GetUCStrToEnumVal(tcb->UCkeyword,0))
//			 { default:	SYNERR("Invalid BigText Indent=xxx value") ;
//			   case _Ignore:tpo->readLineOpts |= V4LEX_ReadLine_IndentNone ; break ;
//			   case _Input:	tpo->readLineOpts |= V4LEX_ReadLine_IndentAsIs ; break ;
//			   case _Blank:	tpo->readLineOpts |= V4LEX_ReadLine_IndentBlank ; break ;
//			   case _Tab:	tpo->readLineOpts |= V4LEX_ReadLine_IndentTab ; break ;
//			 }
//			break ;
//		      case 5:	/* DIMENSION */
//			i = v4dpi_DimGet(ctx,tcb->UCkeyword,DIMREF_REF) ;
//			dix = (i == 0 ? NULL : v4dpi_DimInfoGet(ctx,point->Dim)) ;
//			if (dix == NULL) SYNERR("BigText Dimension attribute - not a dimension") ;
//			if (dix->PointType != V4DPI_PntType_BigText)
//			 SYNERR("BigText Dimension - not type BigText") ;
//			point->Dim = i ; break ;
//		    } ;
//		 } ;



		bt = (struct V4LEX__BigText *)v4mm_AllocChunk(sizeof *bt,FALSE) ;
		saveappendto = tcb->appendto ; savemaxappend = tcb->maxappend ; savebytesappend = tcb->bytesappend ;
		tcb->appendto = bt->BigBuf ; *tcb->appendto = UCEOS ; tcb->maxappend = UCsizeof(bt->BigBuf) ; tcb->bytesappend = 0 ;
		if (!v4lex_ReadNextLine(tcb,tpo,arg,(UCempty(comment) ? NULL : comment)))
		 { ucb = estr ;
		   if (tcb->type == V4LEX_TknType_Error) { UCstrcpy(ucb,UClit("")) ; goto PointAcceptErr ; } ;
		   v_Msg(ctx,ucb,"ReadUnexEOF") ; goto PointAcceptErr ;
		 } ;

		if (!v4dpi_SaveBigTextPoint(ctx,bt,tcb->bytesappend,point,diminfo->DimId,TRUE))
		 { ucb = estr ; UCstrcpy(ucb,ctx->ErrorMsgAux) ; goto PointAcceptErr ; } ;


		if (saveappendto != NULL)		/* If within LogToIsct then append this to prior */
		 { if (savebytesappend + tcb->bytesappend >= savemaxappend)
		    SYNERR("BigText size + prior LogToIsct text exceeds max allowed") ;
		   UCstrcat(saveappendto,tcb->appendto) ; tcb->maxappend = savemaxappend ; tcb->bytesappend += savebytesappend ;
		 } ;
		tcb->appendto = saveappendto ; v4mm_FreeChunk(bt) ;		
		break ;
	   case V4DPI_PntType_PntIdx:
	   case V4DPI_PntType_OSHandle:
	   case V4DPI_PntType_CodedRange:
	   case V4DPI_PntType_Int:
		if (diminfo->Flags & (V4DPI_DimInfo_ValueList | V4DPI_DimInfo_ValueTree))
		 { ETYPE res = v4dpi_PointAcceptTreeOrList(point,diminfo,tcb,ctx,flags) ;
		   if (res == -1) return(FALSE) ;
		   if (res == 1) break ;		/* If 0 then did not parse - continue with normal parsing */
		 } ;
/*		Allow integer to be enclosed in quotes */
		v4lex_NextTkn(tcb,V4LEX_Option_NextChar) ;
		if (tcb->UCkeyword[0] == UClit('"'))
		 { v4lex_NextTkn(tcb,0) ;
		 } else { v4lex_NextTkn(tcb,V4LEX_Option_NegLit|V4LEX_Option_WantInt|(diminfo->ds.Int.IFormat == V4LEX_TablePT_Hexadecimal ? V4LEX_Option_Hex : 0)) ; } ;
		if (tcb->type == V4LEX_TknType_String)
		 { tcb->integer = UCstrtol(tcb->UCstring,&bp,10) ; 
		   if (*bp != UCEOS)
		    { SYNERRMSG("DPIMNotInt") ; } ;
		 } else if (tcb->type != V4LEX_TknType_Integer || tcb->decimal_places > 0)
		 { if (v4sxi_SpecialAcceptor(ctx,point,diminfo,tcb->UCkeyword)) break ;
		   if (v_IsNoneLiteral(tcb->UCkeyword))
		    { point->PntType = V4DPI_PntType_Special ; point->Grouping = V4DPI_Grouping_None ;
		      point->Bytes = V4PS_SpecNone ; point->Dim = diminfo->DimId ; return(TRUE) ;
		    } ;
		   SYNERRMSG("DPIMNotInt") ;
		 } ;
		point->Bytes = V4PS_Int ;
		point->Value.IntVal = ((flags & V4LEX_Option_MakeNeg) != 0 ? -tcb->integer : tcb->integer) ;
		break ;
	   case V4DPI_PntType_IMArg:
		v4lex_NextTkn(tcb,0) ;
		if (tcb->type != V4LEX_TknType_Integer || tcb->decimal_places > 0) SYNERRMSG("DPIMNotInt") ;
		point->Value.IntVal = tcb->integer ;
		point->Bytes = V4PS_Int ;
		break ;
	   case V4DPI_PntType_Delta:
		v4lex_NextTkn(tcb,0) ;
		if (tcb->opcode == V_OpCode_Plus) { i = 1 ; }
		 else if (tcb->opcode == V_OpCode_Minus) { i = -1 ; }
		 else SYNERR("Expecting \"+\" or \"-\" for delta value") ;
		v4lex_NextTkn(tcb,0) ;
		if (tcb->type != V4LEX_TknType_Integer || tcb->decimal_places > 0) SYNERRMSG("DPIMNotInt") ;
		point->Value.IntVal = i * tcb->integer ;
		point->Bytes = V4PS_Int ;
		break ;
	   case V4DPI_PntType_AggRef:
		if (diminfo->Flags & (V4DPI_DimInfo_ValueList | V4DPI_DimInfo_ValueTree))
		 { ETYPE res = v4dpi_PointAcceptTreeOrList(point,diminfo,tcb,ctx,flags) ;
		   if (res == -1) return(FALSE) ;
		   if (res == 1) break ;		/* If 0 then did not parse - continue with normal parsing */
		 } ;
		v4lex_NextTkn(tcb,0) ;
		if (tcb->type != V4LEX_TknType_Integer)
		 { if (v_IsNoneLiteral(tcb->UCkeyword))
		    { point->Value.IntVal = UNUSED ; point->Grouping = 0 ; point->Bytes = V4PS_Int ; return(TRUE) ; } ;
		   SYNERRMSG("DPIMNotInt") ;
		 } ;
		if (tcb->integer < 1 || tcb->integer > gpi->AreaAggCount)
		 { ucb = estr ; v_Msg(ctx,ucb,"DPIAcpAggRef",gpi->AreaAggCount) ; goto PointAcceptErr ; } ;
		point->Grouping = tcb->integer-1 ;
		v4lex_NextTkn(tcb,0) ;
		if (tcb->opcode != V_OpCode_Colon) SYNERR("AggRef reference must be dim:agg:key - missing colon") ;
		v4lex_NextTkn(tcb,0) ;
		if (tcb->type != V4LEX_TknType_Integer) SYNERRMSG("DPIMNotInt") ;
		point->Value.IntVal = tcb->integer ;
		point->Bytes = V4PS_Int ;
		break ;
	   case V4DPI_PntType_UTime:
		v4lex_NextTkn(tcb,V4LEX_Option_NextChunk|V4LEX_Option_RetEOL) ;
		if (tcb->keyword_len == 0) SYNERRMSG("DPIMNoValue") ;
		d1 = v_ParseUTime(ctx,tcb->UCstring) ;
		if (d1 == (double)VCAL_BadVal)
		 { if (v4sxi_SpecialAcceptor(ctx,point,diminfo,tcb->UCstring)) break ;
		   ucb = estr ; UCstrcpy(ucb,ctx->ErrorMsgAux) ; goto PointAcceptErr ;
		 } ;
		PUTREAL(point,d1) ; point->Bytes = V4PS_UTime ;
		break ;
	   case V4DPI_PntType_Time:
	   case V4DPI_PntType_MemPtr:
		v4lex_NextTkn(tcb,0) ;
		if (tcb->type != V4LEX_TknType_Integer)
		 { if (v_IsNoneLiteral(tcb->UCkeyword)) { tcb->integer = UNUSED ; }
		    else { SYNERRMSG("DPIMNotInt") ; } ;
		 } ;
		memcpy(&point->Value.MemPtr,&tcb->integer,sizeof(tcb->integer)) ;
		point->Bytes = V4DPI_PointHdr_Bytes + sizeof point->Value.MemPtr ;
		break ;
	   case V4DPI_PntType_UOM:
		v4lex_NextTkn(tcb,V4LEX_Option_NegLit+V4LEX_Option_WantUOM) ;
		point->Bytes = V4PS_UOM ;
		if (tcb->type == V4LEX_TknType_Integer ||tcb->type == V4LEX_TknType_LInteger)
		   FIXEDtoDOUBLE(tcb->floating,tcb->Linteger,tcb->decimal_places)
		 else if (tcb->type != V4LEX_TknType_Float)
		 { if (v_IsNoneLiteral(tcb->UCkeyword))
		    { point->Value.UOMVal.Ref = UNUSED ; point->Value.UOMVal.Index = 0 ; point->Value.UOMVal.Num = 0.0 ; return(TRUE) ; } ;
		   SYNERRMSG("DPIMNotFP") ;
		 } ;
		if (gpi->uomt == NULL)
		 { if (!v4dpi_UOMInitialize(ctx,UNUSED)) { SYNERRMSG("DPIUOMInit") ; } ; } ;
		uomt = gpi->uomt ;
		if (uomt == NULL)
		 { uomt = (gpi->uomt = v4mm_AllocChunk(sizeof *uomt,FALSE)) ; uomt->Count = 0 ;
		 } ;
		if (diminfo->UOMRef == UOM_GENERIC_REF)
		 { for(i=0;i<uomt->Count;i++) { if (uomt->Entry[i].Ref == UOM_GENERIC_REF) break ; } ;
		   if (i >= uomt->Count)
/*		      Set up dummy UOM for those we know nothing about */
		    { uomt->Entry[uomt->Count].Ref = 0 ; uomd = (uomt->Entry[uomt->Count].uomd = v4mm_AllocChunk(sizeof *uomt->Entry[0].uomd,TRUE)) ;
		      uomt->Count++ ;
		      UCstrcpy(uomd->Name,UClit("GENERIC_UOM")) ; uomd->Ref = UOM_GENERIC_REF ;
		      uomd->Attributes |= V4DPI_UOMAttr_DisplayPSfix ; uomd->DfltDispX = UNUSED ;
		    } ;
		   uomd = uomt->Entry[i].uomd ;
		   for(i=0;i<uomd->Count;i++) { if (UCstrcmp(tcb->UCkeyword,uomd->UEntry[i].PSStr) == 0) break ; } ;
		   if (i >= uomd->Count)
		    { uomd->UEntry[uomd->Count].Factor = 1.0 ; uomd->UEntry[uomd->Count].preOffset = 0 ; uomd->UEntry[uomd->Count].postOffset = 0 ; uomd->UEntry[uomd->Count].Index = uomd->Count+1 ; uomd->UEntry[uomd->Count].Prefix = FALSE ;
		      if (UCstrlen(tcb->UCkeyword) >= UCsizeof(uomd->UEntry[0].PSStr) - 1) tcb->UCkeyword[UCsizeof(uomd->UEntry[0].PSStr) - 1] = UCEOS ;
		      UCstrcpy(uomd->UEntry[uomd->Count].PSStr,tcb->UCkeyword) ; uomd->Count ++ ;
		    } ;
		 } ;
		if (uomt == NULL) SYNERR("No UOM information recorded in V4 process") ;
		for(i=0;i<=uomt->Count;i++) { if (uomt->Entry[i].Ref == diminfo->UOMRef) break ; } ;
		if (i >= uomt->Count)
		 { if (!v4dpi_UOMInitialize(ctx,diminfo->UOMRef)) { SYNERRMSG("DPIUOMInit") ; } ;
		   for(i=0;i<=uomt->Count;i++) { if (uomt->Entry[i].Ref == diminfo->UOMRef) break ; } ;
		 } ;
		if (i >= uomt->Count)
		 { ucb = estr ; v_Msg(ctx,ucb,"DPIAcpUOM",diminfo->UOMRef) ; goto PointAcceptErr ; } ;
		uomd = uomt->Entry[i].uomd ;
		if (tcb->decimal_places != 0 ? (uomd->Attributes & V4DPI_UOMAttr_FractionOK) == 0 : FALSE)
		 { ucb = estr ; v_Msg(ctx,ucb,"DPIAcpFrctUOM",diminfo->UOMRef) ; goto PointAcceptErr ; } ;
		point->Value.UOMVal.Ref = diminfo->UOMRef ;
		if (UCempty(tcb->UCkeyword))
		 { if (uomd->DfltAcceptX == UNUSED) { point->Value.UOMVal.Index = UNUSED ; }
		    else { point->Value.UOMVal.Index = uomd->UEntry[uomd->DfltAcceptX].Index ;
			   if (uomd->CaseCount > 0 && uomd->UEntry[uomd->DfltAcceptX].Factor == 0)
			    { tcb->floating *= uomd->CaseCount ; }
//			    else { tcb->floating *= uomd->UEntry[uomd->DfltAcceptX].Factor ; } ;
			    else { tcb->floating = ((tcb->floating + uomd->UEntry[uomd->DfltAcceptX].preOffset) * uomd->UEntry[uomd->DfltAcceptX].Factor) + uomd->UEntry[uomd->DfltAcceptX].postOffset ; } ;
			   if (uomd->CaseCount != 0) point->Value.UOMVal.Index += V4DPI_CaseCount_Mult * uomd->CaseCount ;
			 } ;
		 } else
		 { for(i=0;i<uomd->Count;i++) { if (UCstrcmp(tcb->UCkeyword,uomd->UEntry[i].PSStr) == 0) break ; } ;
		   if (i >= uomd->Count)
		    { ucb = estr ; v_Msg(ctx,ucb,"DPIAcpUOMPS",tcb->UCkeyword,diminfo->UOMRef) ; goto PointAcceptErr ; } ;
		   point->Value.UOMVal.Index = uomd->UEntry[i].Index ;
		   if (uomd->CaseCount > 0 && uomd->UEntry[i].Factor == 0)
		    { tcb->floating *= uomd->CaseCount ; }
		    else { tcb->floating = ((tcb->floating + uomd->UEntry[i].preOffset) * uomd->UEntry[i].Factor) + uomd->UEntry[i].postOffset ; } ;
		   if (uomd->CaseCount != 0) point->Value.UOMVal.Index += V4DPI_CaseCount_Mult * uomd->CaseCount ;
		 } ;
		memcpy(&point->Value.UOMVal.Num,&tcb->floating,sizeof(tcb->floating)) ;
		return(TRUE) ;
	   case V4DPI_PntType_UOMPer:
		v4lex_NextTkn(tcb,V4LEX_Option_NegLit+V4LEX_Option_WantDbl) ;
		if (tcb->type == V4LEX_TknType_Integer ||tcb->type == V4LEX_TknType_LInteger)
		   FIXEDtoDOUBLE(tcb->floating,tcb->Linteger,tcb->decimal_places)
		 else if (tcb->type != V4LEX_TknType_Float)
		 { if (v_IsNoneLiteral(tcb->UCkeyword))
		    { point->Value.UOMPerVal.Ref = UNUSED ; point->Value.UOMPerVal.Index = 0 ; point->Value.UOMPerVal.Num = 0.0 ; point->Value.UOMPerVal.Amount = 0.0 ; return(TRUE) ; } ;
		   SYNERRMSG("DPIMNotFP") ;
		 } ;
		ok = tcb->floating == 0.0 ;
		memcpy(&point->Value.UOMPerVal.Amount,&tcb->floating,sizeof(tcb->floating)) ;
		v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
		if (tcb->opcode != V_OpCode_Slash)		/* Allow "0" as 0/0xx" */
		 { if(ok)
		    { memset(&point->Value.UOMPerVal,0,sizeof point->Value.UOMPerVal) ;
		      v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; goto end_uomper ;
		     } ;
		   SYNERR("Expecting numeric amount to be followed by \"/uom\" for UOMPer") ;
		 } ;
		v4lex_NextTkn(tcb,V4LEX_Option_NegLit+V4LEX_Option_WantUOM) ;
		if (tcb->type == V4LEX_TknType_Integer ||tcb->type == V4LEX_TknType_LInteger)
		 FIXEDtoDOUBLE(tcb->floating,tcb->Linteger,tcb->decimal_places)
		 else if (tcb->type != V4LEX_TknType_Float) SYNERRMSG("DPIMNotFP") ;
		point->Bytes = V4PS_UOM ;
		if (diminfo->UOMRef == 0)			/* If no link to UOM, then do best we can */
		 { point->Value.UOMPerVal.Ref = 0 ; point->Value.UOMPerVal.Index = 0 ;
		   memcpy(&point->Value.UOMPerVal.Num,&tcb->floating,sizeof(tcb->floating)) ;
		   return(TRUE) ;
		 } ;
		if (gpi->uomt == NULL)
		 { if (!v4dpi_UOMInitialize(ctx,UNUSED)) { SYNERRMSG("DPIUOMInit") ; } ; } ;
		uomt = gpi->uomt ;
		if (uomt == NULL) SYNERR("No UOM information recorded in V4 process") ;
		for(i=0;i<=uomt->Count;i++) { if (uomt->Entry[i].Ref == diminfo->UOMRef) break ; } ;
		if (i >= uomt->Count)
		 { if (!v4dpi_UOMInitialize(ctx,diminfo->UOMRef)) { SYNERRMSG("DPIUOMInit") ; } ;
		   for(i=0;i<=uomt->Count;i++) { if (uomt->Entry[i].Ref == diminfo->UOMRef) break ; } ;
		 } ;
		if (i >= uomt->Count) SYNERR("No UOM information recorded in V4 process for Id::%d") ;
		uomd = uomt->Entry[i].uomd ;
		if (tcb->decimal_places != 0 ? (uomd->Attributes & V4DPI_UOMAttr_FractionOK) == 0 : FALSE)
		 { ucb = estr ; v_Msg(ctx,ucb,"DPIAcpFrctUOM",diminfo->UOMRef) ; goto PointAcceptErr ; } ;
		point->Value.UOMPerVal.Ref = diminfo->UOMRef ;
		if (UCempty(tcb->UCkeyword))
		 { if (uomd->DfltAcceptX == UNUSED) { point->Value.UOMPerVal.Index = UNUSED ; }
		    else { point->Value.UOMPerVal.Index = uomd->UEntry[uomd->DfltAcceptX].Index ;
			   if (uomd->CaseCount > 0 &&uomd->UEntry[uomd->DfltAcceptX].Factor  == 0)
			    { tcb->floating *= uomd->CaseCount ; }
			    else { tcb->floating = ((tcb->floating + uomd->UEntry[uomd->DfltAcceptX].preOffset) * uomd->UEntry[uomd->DfltAcceptX].Factor) + uomd->UEntry[uomd->DfltAcceptX].postOffset ; } ;
			   if (uomd->CaseCount != 0) point->Value.UOMVal.Index += V4DPI_CaseCount_Mult * uomd->CaseCount ;
			 } ;
		 } else
		 { for(i=0;i<uomd->Count;i++) { if (UCstrcmp(tcb->UCkeyword,uomd->UEntry[i].PSStr) == 0) break ; } ;
		   if (i >= uomd->Count)
		    { ucb = estr ; v_Msg(ctx,ucb,"DPIAcpUOMPS",tcb->UCkeyword,diminfo->UOMRef) ; goto PointAcceptErr ; } ;
		   point->Value.UOMPerVal.Index = uomd->UEntry[i].Index ;
		   if (uomd->CaseCount > 0 && uomd->UEntry[i].Factor == 0)
		    { tcb->floating *= uomd->CaseCount ; }
		    else { tcb->floating = ((tcb->floating + uomd->UEntry[i].preOffset) * uomd->UEntry[i].Factor) + uomd->UEntry[i].postOffset ; } ;
		   if (uomd->CaseCount != 0) point->Value.UOMVal.Index += V4DPI_CaseCount_Mult * uomd->CaseCount ;
		 } ;
		memcpy(&point->Value.UOMPerVal.Num,&tcb->floating,sizeof(tcb->floating)) ;
end_uomper:	point->Bytes = V4PS_UOMPer ;
		return(TRUE) ;
	   case V4DPI_PntType_UOMPUOM:
		v4lex_NextTkn(tcb,0) ;
		if (v_IsNoneLiteral(tcb->UCkeyword))
		 { point->Value.UOMPUOMVal.UOM.Ref = UNUSED ; point->Value.UOMPUOMVal.UOM.Index = 0 ; point->Value.UOMPUOMVal.UOM.Num = 0.0 ;
		   point->Value.UOMPUOMVal.PUOM.Ref = UNUSED ; point->Value.UOMPUOMVal.PUOM.Index = 0 ; point->Value.UOMPUOMVal.PUOM.Num = 0.0 ;
		   return(TRUE) ;
		 } ;
		SYNERR("'UOM[per]UOM' values may only be created via MakeP() module") ;
	   case V4DPI_PntType_Fixed:
		v4lex_NextTkn(tcb,V4LEX_Option_NegLit+V4LEX_Option_WantLInt) ;
		if (tcb->type == V4LEX_TknType_Integer ||tcb->type == V4LEX_TknType_LInteger)
		 { b64 = tcb->Linteger ;
		   if (tcb->decimal_places != diminfo->Decimals)
		    { i = diminfo->Decimals - tcb->decimal_places ;
		      if (i < 0)
		       { ucb = estr ; v_Msg(ctx,ucb,"DPIAcpDPs",diminfo->Decimals) ; goto PointAcceptErr ; } ;
		      for(;i>0;i--)
		       { b64a = b64 ; b64 *= 10 ;
		         if (b64a > 0 ? (b64a > b64) : (b64a < b64)) { ucb = estr ; v_Msg(ctx,ucb,"DPIAcpNumTooBig",diminfo->Decimals) ; goto PointAcceptErr ; } ;
		       } ;
		    } ;
		 } else if (tcb->type == V4LEX_TknType_String)
		 { b64 = UCstrtoll(tcb->UCstring,&bp,10) ; if (*bp != UCEOS) { SYNERRMSG("DPIMNotFP") ; } ;
		 } else
		 { if (tcb->type != V4LEX_TknType_Float) SYNERR("Expecting fixed value") ;
		   for(i=0;i<diminfo->Decimals;i++) { tcb->floating *= 10.0 ; } ; b64 = (B64INT)tcb->floating ;
		 } ;
		if ((flags & V4LEX_Option_MakeNeg) != 0) b64 = -b64 ;
		memcpy(&point->Value.FixVal,&b64,sizeof(b64)) ;
		point->Bytes = V4DPI_PointHdr_Bytes + sizeof b64 ; point->LHSCnt = diminfo->Decimals ;
		return(TRUE) ;
	   case V4DPI_PntType_Real:
		v4lex_NextTkn(tcb,V4LEX_Option_NegLit+V4LEX_Option_WantDbl) ;
		if (tcb->type == V4LEX_TknType_Integer ||tcb->type == V4LEX_TknType_LInteger)
		   FIXEDtoDOUBLE(tcb->floating,tcb->Linteger,tcb->decimal_places)
		 else if (tcb->type == V4LEX_TknType_String)
		 { tcb->floating = UCstrtod(tcb->UCstring,&bp) ; if (*bp != UCEOS) { SYNERRMSG("DPIMNotFP") ; } ;
		 } else if (tcb->type != V4LEX_TknType_Float)
		 { if (v4sxi_SpecialAcceptor(ctx,point,diminfo,tcb->UCkeyword)) break ;
		   if (v_IsNoneLiteral(tcb->UCkeyword))
		    { point->PntType = V4DPI_PntType_Special ; point->Grouping = V4DPI_Grouping_None ;
		      point->Bytes = V4PS_SpecNone ; return(TRUE) ;
		    } ;
		   SYNERRMSG("DPIMNotFP") ;
		 } ;
		if ((flags & V4LEX_Option_MakeNeg) != 0) tcb->floating = -tcb->floating ;
		PUTREAL(point,tcb->floating) ;
		point->Bytes = V4PS_Real ;
		return(TRUE) ;
	   case V4DPI_PntType_Logical:
		v4lex_NextTkn(tcb,0) ;
		switch (tcb->type)
		 { default:	SYNERR("Expecting integer or string value") ;
		   case V4LEX_TknType_Integer: point->Value.IntVal = tcb->integer ; break ;
		   case V4LEX_TknType_Keyword: UCstrcpy(tcb->UCstring,tcb->UCkeyword) ;	/* Copy to string & drop thru */
		   case V4LEX_TknType_String:
//			for(i=0;;i++) { ucbuf[i] = toupper(tcb->UCstring[i]) ; if (tcb->UCstring[i] == UCEOS) break ; } ;
//			if (ci == NULL) { ci = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ci ; } ;
//			if (UCstrncmp(ucbuf,ci->li->LI[ci->Cntry[ci->CurX].Language].YesUC,i) == 0) { point->Value.IntVal = TRUE ; }
//			 else if (UCstrncmp(ucbuf,ci->li->LI[ci->Cntry[ci->CurX].Language].TrueUC,i) == 0) { point->Value.IntVal = TRUE ; }
//			 else if (UCstrncmp(ucbuf,ci->li->LI[ci->Cntry[ci->CurX].Language].NoUC,i) == 0) { point->Value.IntVal = FALSE ; }
//			 else if (UCstrncmp(ucbuf,ci->li->LI[ci->Cntry[ci->CurX].Language].FalseUC,i) == 0) { point->Value.IntVal = FALSE ; }
//			 else { ucb = arg ; v_Msg(ctx,ucb,"DPILogVal",V4DPI_PntType_Logical,ci->li->LI[ci->Cntry[ci->CurX].Language].Yes,ci->li->LI[ci->Cntry[ci->CurX].Language].True,ci->li->LI[ci->Cntry[ci->CurX].Language].No,ci->li->LI[ci->Cntry[ci->CurX].Language].False) ; goto PointAcceptErr ; } ;
			if (ci == NULL) { ci = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ci ; } ;
			 point->Value.IntVal = v_ParseLog(tcb->UCstring,&ucb) ;
			 if (*ucb != UCEOS) { ucb = arg ; v_Msg(ctx,ucb,"DPILogVal",V4DPI_PntType_Logical,ci->li->LI[ci->Cntry[ci->CurX].Language].Yes,ci->li->LI[ci->Cntry[ci->CurX].Language].True,ci->li->LI[ci->Cntry[ci->CurX].Language].No,ci->li->LI[ci->Cntry[ci->CurX].Language].False) ; goto PointAcceptErr ; } ;
		 } ;
		point->Bytes = V4PS_Int ;
		break ;
	   case V4DPI_PntType_BinObj:
		v4lex_NextTkn(tcb,V4LEX_Option_ForceKW) ;
		if (tcb->type != V4LEX_TknType_BString) SYNERR("Expecting encoded binary string") ;
/*		Binary string - convert from wchar_t bytes to char bytes (not very clean but it's the way it works) */
		for(i=0;i<tcb->literal_len;i++) { point->Value.AlphaVal[i] = tcb->UCstring[i] ; } ;
		point->Bytes = V4DPI_PointHdr_Bytes + ALIGN(tcb->literal_len) ;
		break ;
	   case V4DPI_PntType_RegExpPattern:
		v4lex_NextTkn(tcb,V4LEX_Option_ForceKW) ;
		if (tcb->type == V4LEX_TknType_String)
		 { strcpy(&point->Value.AlphaVal[1],UCretASC(tcb->UCstring)) ; num = UCstrlen(tcb->UCstring) ;
		 } else
		 { if (tcb->type != V4LEX_TknType_Keyword) SYNERRMSG("DPIMNotKW") ;
		   strcpy(&point->Value.AlphaVal[1],UCretASC(tcb->UCkeyword)) ;
		   num = UCstrlen(tcb->UCkeyword) ;
		 } ;
		CHARPNTBYTES2(point,num)
		repattern = (regex_t *)&point->Value.AlphaVal[point->Bytes-V4DPI_PointHdr_Bytes] ;
		if (i=vregexp_RegComp(repattern,UCretASC(tcb->UCstring),REG_EXTENDED))
		 { vregexp_Error(i,repattern,estr,sizeof estr) ;
		   ucb = estr ; goto PointAcceptErr ;
		 } ;
		point->Bytes += sizeof *repattern ;
		break ;		
	   CASEofCharmU
		if (diminfo->ds.Alpha.aFlags & V4DPI_DimInfoAlpha_IsFileName)
		 { v_ParseFileName(tcb,arg,NULL,NULL,&i) ;
		   UCstrcpy(tcb->UCstring,arg) ; tcb->type = V4LEX_TknType_String ;
		 } else { v4lex_NextTkn(tcb,V4LEX_Option_ForceKW) ; } ;
		if (tcb->type == V4LEX_TknType_Error) SYNERR("Missing string value for alpha dimension") ;
		if (tcb->type == V4LEX_TknType_String)
		 { if (diminfo->Flags & V4DPI_DimInfo_Normalize)
		    { UCSTRTOUPPER(tcb->UCstring) ;
		    } ;
		   num = UCstrlen(tcb->UCstring) ;
//		   if (num > V4DPI_AlphaVal_Max - 10)
//		    SYNERR("Alpha string too long") ;
//		   num = UCUTF16toUTF8(&point->Value.AlphaVal[1],V4DPI_AlphaVal_Max,tcb->UCstring,num) ;
		   if (num <= V4DPI_AlphaVal_Max - 10)
		    { LOGICAL ok = TRUE;
		      for (i = 0; tcb->UCstring[i] != UCEOS; i++) { if (tcb->UCstring[i] > 0xff) { ok = FALSE; break; }; };
		      if (!ok) goto ucchar_entry ;
		      alphaPNTvl(point, UCretASC(tcb->UCstring), num) ;
		    } else goto ucchar_entry ;
		 } else	if (tcb->type == V4LEX_TknType_BString)
		 { memcpy(&point->Value.AlphaVal[1],tcb->UCstring,tcb->literal_len) ;
		   num = tcb->literal_len ;
		 } else
		 { if (tcb->type != V4LEX_TknType_Keyword) SYNERRMSG("DPIMNotKW") ;
		   if (diminfo->Flags & V4DPI_DimInfo_Normalize) { num = UCUTF16toUTF8(&point->Value.AlphaVal[1],V4DPI_AlphaVal_Max,tcb->UCkeyword,UCstrlen(tcb->UCkeyword)) ; }
		    else { num = UCUTF16toUTF8(&point->Value.AlphaVal[1],V4DPI_AlphaVal_Max,tcb->UCstring,UCstrlen(tcb->UCstring)) ; } ;
		 } ;
		if (num < 0) SYNERR("String length exceeds maximum allowed") ;
		CHARPNTBYTES2(point,num)
		break ;
	   case V4DPI_PntType_UCChar:
		if (diminfo->ds.Alpha.aFlags & V4DPI_DimInfoAlpha_IsFileName)
		 { v_ParseFileName(tcb,tcb->UCstring,NULL,NULL,&i) ; tcb->type = V4LEX_TknType_String ;
		 } else { v4lex_NextTkn(tcb,V4LEX_Option_ForceKW) ; } ;
ucchar_entry:
		if (tcb->type == V4LEX_TknType_String)
		 { if (diminfo->Flags & V4DPI_DimInfo_Normalize) { UCSTRTOUPPER(tcb->UCstring) ; } ;
//		   UCstrcpy(&point->Value.UCVal[1],tcb->UCstring) ;
		   num = UCstrlen(tcb->UCstring) ;
		   if (!v4dpi_SaveBigTextPoint2(ctx, tcb->UCstring, point, diminfo->DimId, TRUE))
		    { ucb = estr ; UCstrcpy(ucb, ctx->ErrorMsgAux) ; goto PointAcceptErr ;  } ;
		 } else
		 { if (tcb->type != V4LEX_TknType_Keyword) SYNERRMSG("DPIMNotKW") ;
		   UCstrcpy(&point->Value.UCVal[1],((diminfo->Flags & V4DPI_DimInfo_Normalize) ? tcb->UCkeyword : tcb->UCstring)) ;
		   num = UCstrlen(tcb->UCkeyword) ;
		   UCCHARPNTBYTES2(point,num) ;
		 } ;
		break ;
	   case V4DPI_PntType_IntMod:
		v4lex_NextTkn(tcb,0) ;
		if (tcb->opcode == V_OpCode_Pound)			/* Ignore leading # */
		 { int begin,end ;
		   v4lex_NextTkn(tcb,0) ;
		   if (tcb->type != V4LEX_TknType_Integer) SYNERRMSG("DPIMNotInt") ;
		   v4im_IntModRange(&begin,&end) ;
		   if (tcb->integer < begin || tcb->integer > end) SYNERR("Invalid IntMod point number") ;
		   point->Value.IntVal = tcb->integer ;
		 } else
		 { if (tcb->type != V4LEX_TknType_Keyword) SYNERRMSG("DPIMNotKW") ;
		   if ((point->Value.IntVal = v4im_Accept(tcb->UCkeyword)) <= 0)
		    { ucb = estr ; v_Msg(ctx,ucb,"DPIAcpNoIntMod",tcb->UCkeyword) ; goto PointAcceptErr ; } ;
		 } ;
		point->Bytes = V4PS_Int ; break ;
	   case V4DPI_PntType_SSVal:
		v4lex_NextTkn(tcb,0) ;
		if (tcb->type != V4LEX_TknType_Keyword) SYNERRMSG("DPIMNotKW") ;
		switch (v4im_GetUCStrToEnumVal(tcb->UCkeyword,0))
		 { default:	SYNERR("Not a valid SSVAL keyword") ;
		   case _Null:	point->Value.IntVal = V4SS_SSVal_Null ; break ;
		   case _Div0:	point->Value.IntVal = V4SS_SSVal_Div0 ; break ;
		   case _Value:	point->Value.IntVal = V4SS_SSVal_Value ; break ;
		   case _Ref:	point->Value.IntVal = V4SS_SSVal_Ref ; break ;
		   case _Name:	point->Value.IntVal = V4SS_SSVal_Name ; break ;
		   case _Num:	point->Value.IntVal = V4SS_SSVal_Num ; break ;
		   case _NA:	point->Value.IntVal = V4SS_SSVal_NA ; break ;
		   case _Empty:	point->Value.IntVal = V4SS_SSVal_Empty ; break ;
		   case _Row:	point->Value.IntVal = V4SS_SSVal_Row ; break ;
		   case _Col: point->Value.IntVal = V4SS_SSVal_Column ; break ;
		   case _Column: point->Value.IntVal = V4SS_SSVal_Column ; break ;
		 } ;
		point->Bytes = (char *)&point->Value.AlphaVal[SIZEofINT] - (char *)point ;
		break ;
	   case V4DPI_PntType_Color:
		v4lex_NextTkn(tcb,V4LEX_Option_ForceKW) ;
		if (tcb->opcode == V_OpCode_Pound)
		 { v4lex_NextTkn(tcb,V4LEX_Option_ForceKW) ;
		   if (tcb->type != V4LEX_TknType_Keyword && tcb->type != V4LEX_TknType_String)
		    SYNERR("Expecting COLOR specification/name") ;
		   point->Value.IntVal = v_ColorRGBNameToRef((tcb->type == V4LEX_TknType_Keyword ? tcb->UCkeyword : tcb->UCstring),0) ;
		 } else 
		 { if (tcb->type != V4LEX_TknType_Keyword && tcb->type != V4LEX_TknType_String)
		    SYNERR("Expecting COLOR specification/name") ;
		   point->Value.IntVal = v_ColorNameToRef(tcb->type == V4LEX_TknType_Keyword ? tcb->UCkeyword : tcb->UCstring) ;
		 } ;
		point->Bytes = V4PS_Int ;
		if (point->Value.IntVal == 0)
		 { if (v_IsNoneLiteral(tcb->UCstring)) { point->Value.IntVal = UNUSED ; return(TRUE) ; } ;
		   if (v4sxi_SpecialAcceptor(ctx,point,diminfo,tcb->UCkeyword)) break ;
		   v_Msg(ctx,estr,"DPIAcpNoColor",diminfo->DimId,tcb->UCstring) ; ucb = estr ; goto PointAcceptErr ;
		 } ;
		break ;
	   case V4DPI_PntType_Tree:
		v4lex_NextTkn(tcb,0) ;
		if (tcb->type == V4LEX_TknType_Integer ||tcb->type == V4LEX_TknType_LInteger)
		 { NODEID node ; TREEID id ;
		   if (tcb->decimal_places != 0) SYNERR("Tree value must be integer value") ;
		   node = tcb->Linteger ;
		   v4lex_NextTkn(tcb,V4LEX_Option_RetEOL) ;
		   if (tcb->opcode == V_OpCode_Minus)
		    { id = node ; v4lex_NextTkn(tcb,0) ;
		      if (tcb->type != V4LEX_TknType_Integer || tcb->decimal_places != 0) SYNERR("Invalid TREE value syntax") ;
		      node = tcb->integer ;
		    } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
		   point->Value.TreeNodeVal = TREENODE(id,node) ;
		 } else if (v_IsNoneLiteral(tcb->UCkeyword))
		 { point->Value.TreeNodeVal = -1 ;
		 } else SYNERR("Invalid TREE value") ; ;
		point->Bytes = V4PS_Tree ;
		break ;
	   case V4DPI_PntType_Country:
		v4lex_NextTkn(tcb,V4LEX_Option_ForceKW) ;
		if (tcb->opcode == V_OpCode_Pound)
		 { v4lex_NextTkn(tcb,0) ;
		   if (tcb->type != V4LEX_TknType_Integer) SYNERR("Expecting COUNTRY ref number") ;
		   point->Value.IntVal = tcb->integer ;
		 } else 
		 { if (tcb->type != V4LEX_TknType_Keyword && tcb->type != V4LEX_TknType_String)
		    SYNERR("Expecting COUNTRY specification/name") ;
		   point->Value.IntVal = v_CountryNameToRef(tcb->type == V4LEX_TknType_Keyword ? tcb->UCkeyword : tcb->UCstring) ;
		 } ;
		if (point->Value.IntVal == 0)
		 { if (v_IsNoneLiteral(tcb->UCstring)) { point->Value.IntVal = UNUSED ; return(TRUE) ; } ;
		   v_Msg(ctx,estr,"DPIAcpNoCntry",diminfo->DimId,tcb->UCstring) ; ucb = estr ; goto PointAcceptErr ; } ;
		point->Bytes = V4PS_Int ;
		break ;
	   case V4DPI_PntType_XDict:
		if (gpi->xdr == NULL) SYNERRMSG("DPIMNoExt") ;
		v4lex_NextTkn(tcb,V4LEX_Option_ForceKW+V4LEX_Option_AllowDot) ;
		if (tcb->opcode == V_OpCode_Pound)			/* If prefaced with "#" then hash value */
		 { v4lex_NextTkn(tcb,V4LEX_Option_ForceKW+V4LEX_Option_AllowDot) ;
		   point->Value.IntVal = UCstrtol(tcb->UCkeyword,&ucb,10) ;
		   if (*ucb != UCEOS)					/* If string does not parse as integer then hash it */
		    { point->Value.IntVal = 0 ;
		      for(i=1,ucb=tcb->UCkeyword;*ucb!=0;ucb++,i++)
		       { point->Value.IntVal += ( (99999 - (i*11))* *ucb ) ; } ;
		      point->Value.IntVal |= 0x80000000 ;			/* Force negative */
		    } ;
		   point->Bytes = V4PS_Int ; break ;
		 } ;
		if (tcb->type != V4LEX_TknType_Keyword && tcb->type != V4LEX_TknType_String)
		 SYNERRMSG("DPIMNotKW") ;
		if ((point->Value.IntVal =
			v4dpi_XDictEntryGet(ctx,tcb->UCstring,diminfo,0)) == 0)
		 { v_Msg(ctx,estr,"DPIAcpPntUndefX",V4DPI_PntType_XDict,diminfo->DimId,tcb->UCstring) ; ucb = estr ; goto PointAcceptErr ; } ;
		point->Bytes = V4PS_Int ;
		break ;
	   case V4DPI_PntType_Tag:
		v4lex_NextTkn(tcb,V4LEX_Option_ForceKW) ;
		if (tcb->opcode == V_OpCode_Pound)			/* Ignore leading # */
		 { int begin,end ;
		   v4lex_NextTkn(tcb,0) ;
		   if (tcb->type != V4LEX_TknType_Integer) SYNERRMSG("DPIMNotInt") ;
		   v4im_TagRange(&begin,&end) ;
		   if (tcb->integer < begin || tcb->integer > end) SYNERR("Invalid tag point number") ;
		   point->Value.IntVal = tcb->integer ;
		 } else
		 { if (tcb->type != V4LEX_TknType_Keyword && tcb->type != V4LEX_TknType_String)
		    SYNERRMSG("DPIMNotKW") ;
		   if ((point->Value.IntVal = v4im_TagValue(tcb->UCstring)) <= 0)
		    { v_Msg(ctx,estr,"DPIAcpPntUndef",diminfo->DimId,tcb->UCstring) ; ucb = estr ; goto PointAcceptErr ; } ;
		 } ;
		point->Bytes = V4PS_Int ;
		break ;
	   case V4DPI_PntType_Dict:
		v4lex_NextTkn(tcb,V4LEX_Option_ForceKW+V4LEX_Option_AllowDot) ;
		if (tcb->opcode == V_OpCode_Pound)			/* Ignore leading # */
		 v4lex_NextTkn(tcb,V4LEX_Option_ForceKW+V4LEX_Option_AllowDot) ;
		if (tcb->type != V4LEX_TknType_Keyword)
		 { if (tcb->type == V4LEX_TknType_String) { UCstrncpy(tcb->UCkeyword,tcb->UCstring,(UCsizeof(tcb->UCkeyword))-1) ; }
		    else { SYNERRMSG("DPIMNotKW") ; } ;
		 } ;


		if ((diminfo->Flags & V4DPI_DimInfo_Dim) != 0)
		 { 
//		   if (strcmp(tcb->keyword,"DEFAULT") == 0) strcpy(tcb->keyword,"NUM") ;
		   point->Value.IntVal = v4dpi_DimGet(ctx,tcb->UCkeyword,DIMREF_DIMDIM) ;	/* Here is where Dim:xxx dimension xxx is referenced */
		   if (point->Value.IntVal == 0)
		    { v_Msg(ctx,estr,"DPIAcpPntUndef",diminfo->DimId,tcb->UCstring) ; ucb = estr ; goto PointAcceptErr ; } ;
		 } else
		 { if ((point->Value.IntVal =
			v4dpi_DictEntryGet(ctx,0,tcb->UCstring,diminfo,NULL)) == 0)
		    { v_Msg(ctx,estr,"DPIAcpPntUndef",diminfo->DimId,tcb->UCstring) ; ucb = estr ; goto PointAcceptErr ;
		    } ;
		 } ;
		point->Bytes = V4PS_Int ;
		break ;
	   case V4DPI_PntType_List:
		v4lex_NextTkn(tcb,0) ;
		if (tcb->opcode == V_OpCode_LBrace || tcb->opcode == V_OpCode_NCLBrace) { brace = TRUE ; }
		 else if (tcb->opcode == V_OpCode_LParen || tcb->opcode == V_OpCode_NCLParen) { brace = FALSE ; }
		 else SYNERR("List must begin with \"(\"") ;

		if (diminfo->ListDimId > 0)		/* Look to see if we should make a bitmap */
		 { int maxpoints ;
		   struct V4L__ListBMData1 *vlbmd ; int vlbmdbytes ;
		   maxpoints = v4dpi_DimUniqueNumPoints(ctx,diminfo->ListDimId) ;
		   if (maxpoints == UNUSED) goto no_bitmap ;
		   vlbmdbytes = sizeof *vlbmd + ((maxpoints + 31) / 32) * 4 ;
		   vlbmd = (struct V4L__ListBMData1 *)v4mm_AllocChunk(vlbmdbytes,TRUE) ;
		   vlbmd->MaxBit = BM1_CalcMaxBit(maxpoints) ;
		   for(;;)
		    { v4lex_NextTkn(tcb,0) ;
		      if (tcb->opcode == V_OpCode_RParen) break ;
		      if (tcb->opcode == V_OpCode_RBrace)		/* Look for either closing brace or paren */
		       { if (!brace) v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
		         break ;
		       } ;
		      v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
		      if(!v4dpi_PointParse(ctx,&isctbuf,tcb,flags)) return(FALSE) ;		/* Parse point into temp buffer */
		      if (isctbuf.Dim != diminfo->ListDimId || isctbuf.Value.IntVal < 0 || isctbuf.Value.IntVal > maxpoints)
		       { v_Msg(ctx,estr,"DPIBMListEntry",&isctbuf,diminfo->ListDimId,maxpoints) ; ucb = estr ; goto PointAcceptErr ; } ;
		      BM1_SetBit(vlbmd,isctbuf.Value.IntVal) ;
		    } ;
		   point->PntType = V4DPI_PntType_SegBitMap ; point->Bytes = V4PS_Int ; point->Dim = diminfo->DimId ;
		   point->Value.IntVal = v4seg_PutSegments(ctx,vlbmd,vlbmdbytes,TRUE,FALSE) ;
		   return(TRUE) ;
		 } ;

no_bitmap:
		lp = ALIGNLP(&point->Value) ; memset(lp,0,V4L_ListPointHdr_Bytes) ;	/* Link up list structure */
		lp->ListType = V4L_ListType_Point ;
		for(;;)
		 { v4lex_NextTkn(tcb,0) ;
		   if (tcb->opcode == V_OpCode_RParen) break ;
		   if (tcb->opcode == V_OpCode_RBrace)		/* Look for either closing brace or paren */
		    { if (!brace) v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
		      break ;
		    } ;
		   v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
		   if(!v4dpi_PointParse(ctx,&isctbuf,tcb,flags)) return(FALSE) ;		/* Parse point into temp buffer */
		   if (isctbuf.PntType == V4DPI_PntType_Isct)
		    { if ((diminfo->ds.List.lFlags & V4DPI_DimInfoList_IsctModOK) == 0) SYNERR("Intersections / Internal Modules cannot be in this list") ;
		    } ;
		   if (!v4l_ListPoint_Modify(ctx,lp,((diminfo->Flags & V4DPI_DimInfo_IsSet) ? V4L_ListAction_AppendUnique : V4L_ListAction_Append),&isctbuf,0)) return(FALSE) ;
		 } ;
		point->Bytes = (char *)&point->Value.AlphaVal[lp->Bytes] - (char *)point ;
		if (point->Bytes < V4DPI_PointHdr_Bytes + ((char *)&lp->Buffer[0] - (char *)lp))
		 point->Bytes = V4DPI_PointHdr_Bytes + ((char *)&lp->Buffer[0] - (char *)lp) ;	/* Make sure min size */

		return(TRUE) ;
	   case V4DPI_PntType_Isct:
		return(TRUE) ;
	   case V4DPI_PntType_FrgnDataEl:
		v4lex_NextTkn(tcb,0) ;
		if (tcb->opcode != V_OpCode_NCLParen) SYNERR("Foreign field must be enclosed within \"()\"") ;
		des = (struct V4FFI__DataElSpec *)&point->Value ;
#define PN(FLD) v4lex_NextTkn(tcb,0) ; if (tcb->type!=V4LEX_TknType_Integer) SYNERR("Bad DES") ; ; des->FLD = tcb->integer ;
#define PC v4lex_NextTkn(tcb,0) ; if (tcb->opcode != V_OpCode_Comma) SYNERR("Expecting comma") ;
		PN(FileRef) ; PC ; PN(Element) ; PC ;
		v4lex_NextTkn(tcb,0) ; UCstrcpy(des->Name,tcb->UCkeyword) ; PC ;
		v4lex_NextTkn(tcb,0) ; UCstrcpy(des->DimName,tcb->UCkeyword) ; PC ;
		PN(V3DT) ; PC ; PN(KeyNum) ; PC ; PN(Owner) ; PC ; PN(Offset) ; PC ; PN(Bytes) ; PC ; PN(Decimals) ;
		v4lex_NextTkn(tcb,0) ;
		if (tcb->opcode != V_OpCode_RParen) SYNERR("Foreign field must be enclosed within \"()\"") ;
		point->Bytes = sizeof *des + V4DPI_PointHdr_Bytes ;
		break ;
	   case V4DPI_PntType_FrgnStructEl:
		v4lex_NextTkn(tcb,0) ;
		if (tcb->opcode != V_OpCode_NCLParen) SYNERR("Structure element must be enclosed within \"()\"") ;
#define PN1(FLD) v4lex_NextTkn(tcb,0) ; if (tcb->type!=V4LEX_TknType_Integer) SYNERR("Bad SES") ; ; ses->FLD = tcb->integer ;
		ses = (struct V4FFI__StructElSpec *)&point->Value ;
		PN1(FileRef) ; PC ; PN1(StructNum) ; PC ; PN1(Element) ; PC ; PN1(Bytes) ; PC ; PN1(Occurs) ; PC ;
		v4lex_NextTkn(tcb,0) ;
		if (tcb->type == V4LEX_TknType_Keyword) { strcpy(ses->CountField,UCretASC(tcb->UCkeyword)) ; }
		 else { ses->CountField[0] = 0 ; v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
		PC ; PN1(Offset) ; v4lex_NextTkn(tcb,0) ;
		if (tcb->opcode != V_OpCode_RParen) SYNERR("Structure element must be enclosed within \"()\"") ;
		point->Bytes = sizeof *ses + V4DPI_PointHdr_Bytes ;
		break ;
	   case V4DPI_PntType_Shell:
		npt = (P *)&point->Value ; if(!v4dpi_PointParse(ctx,npt,tcb,flags)) return(FALSE) ;
		point->Bytes = (char *)&point->Value.AlphaVal[npt->Bytes] - (char *)point ;
		break ;
	 } ;
	return(TRUE) ;
/*	Here to handle point syntax errors (b = ptr to error message) */
PointAcceptErr:
	if (tcb->type == V4LEX_TknType_Error)
	 { v_Msg(ctx,arg,"@%1U (%2U)",ucb,tcb->UCstring) ; ucb = arg ; } ;
PointAcceptErr1:
	if (v4dpi_PointAccept1(point,diminfo,tcb,ctx,flags,TknStart))
	 return(TRUE) ;							/* Can we parse via [UV4:AcceptFail] ? */
	bp = v4lex_TknPtr(tcb,V4LEX_TknOp_ThruCurrent,TknStart,&ok) ; if (!ok) bp = UClit("...") ;
	if (diminfo->DimId == UNUSED) { UCstrcpy(ctx->ErrorMsgAux,ucb) ; }
	 else { v_Msg(ctx,ctx->ErrorMsgAux,"@%1D:\"%2U\" - %3U",diminfo->DimId,bp,ucb) ; } ;
	if (flags & V4DPI_PointParse_RetFalse) return(FALSE) ;
	v4_UCerror(0,tcb,"V4DPI","PointParse","SYNERR",ctx->ErrorMsgAux) ;
	return(FALSE) ;
/*	Error - emne = pointer to (char *) error message mnemonic */
PointAcceptErrMne:
	if (tcb->type == V4LEX_TknType_Error)
	 { v_Msg(ctx,arg,"@%1m (%2U)",emne,tcb->UCstring) ; ucb = arg ; goto PointAcceptErr1 ; } ;
	if (v4dpi_PointAccept1(point,diminfo,tcb,ctx,flags,TknStart))
	 return(TRUE) ;							/* Can we parse via [UV4:AcceptFail] ? */
	bp = v4lex_TknPtr(tcb,V4LEX_TknOp_ThruCurrent,TknStart,&ok) ; if (!ok) bp = UClit("...") ;
	if (diminfo->DimId == UNUSED) { v_Msg(ctx,ctx->ErrorMsgAux,"@%1m",emne) ; }
	 else { v_Msg(ctx,ctx->ErrorMsgAux,"@%1D:\"%2U\" - %3m",diminfo->DimId,bp,emne) ; } ;
	if (flags & V4DPI_PointParse_RetFalse) return(FALSE) ;
	v4_UCerror(0,tcb,"V4DPI","PointParse","SYNERR",ctx->ErrorMsgAux) ;
	return(FALSE) ;
}

/*	v4dpi_PointAccept1 - Attempts to "accept" currently erroneous point value via Isct	*/
/*	Call: result = v4dpi_PointAccept1( point , diminfo , tcb , ctx, flags , tknstart )
	  where result is the result code,
		point is pointer to hold results of "acceptance",
		diminfo is pointer to dimension info- V4DPI__DimInfo,
		tcb is pointer to token control block,
		ctx is pointer to current context,
		flags are parsing flags (V4DPI_PointParse_xxx)
		tknstart is pointer to begin of token						*/

LOGICAL v4dpi_PointAccept1(point,diminfo,tcb,ctx,flags,tknstart)
  struct V4DPI__Point *point ;
  struct V4DPI__DimInfo *diminfo ;
  struct V4C__Context *ctx ;
  int flags ;
  UCCHAR *tknstart ;
  struct V4LEX__TknCtrlBlk *tcb ;
{ 
  P *ipt,*ip,v4pt,apt ;
  enum DictionaryEntries deval ;
  int framepush,ok ; UCCHAR *tkn ;

	INITISCT(&v4pt) ; ISCTVCD(&v4pt) ; v4pt.Grouping = 2 ;				/* Construct [Dim:xxx UV4:AcceptorFail Alpha:yyy] intersection */
	ip = ISCT1STPNT(&v4pt) ; dictPNTv(ip,Dim_UV4,v4im_GetEnumToDictVal(ctx,deval=_AcceptorFail,UNUSED)) ;
	if (ip->Dim == 0 || ip->Value.IntVal == 0) return(FALSE) ;
	v4pt.Bytes += ip->Bytes ; ADVPNT(ip) ;
	if (diminfo->ADPnt.Bytes != 0)
	 { memcpy(ip,&diminfo->ADPnt,diminfo->ADPnt.Bytes) ; } else { dictPNTv(ip,Dim_Dim,diminfo->DimId) ; } ;
	v4pt.Bytes += ip->Bytes ; ADVPNT(ip) ; 
	framepush = v4ctx_FramePush(ctx,NULL) ;
	tkn = v4lex_TknPtr(tcb,V4LEX_TknOp_ThruCurrent,tknstart,&ok) ;
	if (!ok) return(FALSE) ;				/* Could not get token ??? */
	uccharPNTv(&apt,tkn) ;
	if (!v4ctx_FrameAddDim(ctx,0,&apt,0,0)) return(FALSE) ;			/* Add Alpha:token-value point to context */
	ipt = v4dpi_IsctEval(point,&v4pt,ctx,V4DPI_EM_NoIsctFail,NULL,NULL) ;
	if (!v4ctx_FramePop(ctx,framepush,NULL)) return(FALSE) ;
	if (ipt == NULL) return(FALSE) ;
	memcpy(point,ipt,ipt->Bytes) ; point->Dim = diminfo->DimId ;
	return(TRUE) ;
}

/*	v4dpi_PointAcceptTreeOrList - Parses (maybe) a database:(tree-list expression	*/
/*	res = v4dpi_PointAcceptTreeOrList( point , di , tcb , ctx , flags )
	  where res = -1 (error in parse), 0 (nothing parsed), 1 (parsed OK),
		point is updated with resulting point,
		di is V4DPI__DimInfo *,
		tcb is the tcb,
		ctx is the context,
		flags are parsing flags							*/

ETYPE v4dpi_PointAcceptTreeOrList(point,di,tcb,ctx,flags)
  struct V4DPI__Point *point ;
  struct V4DPI__DimInfo *di ;
  struct V4LEX__TknCtrlBlk *tcb ;
  struct V4C__Context *ctx ;
  FLAGS32 flags ;
{ P *uvpt,*npt, isctbuf ;
  struct V4L__ListPoint *lp ;

	v4lex_NextTkn(tcb,0) ;
	if (tcb->opcode == V_OpCode_LBracket || tcb->opcode == V_OpCode_NCLBracket)		/* Want to convert dim:[xxx] -> [UV4:Value dim (xxx)] */
	 { INITISCT(point) ; ISCTVCD(point) ; point->Grouping = 3 ;
	   npt = ISCT1STPNT(point) ; dictPNTv(npt,Dim_UV4,v4im_GetEnumToDictVal(ctx,DE(ValueTree),Dim_UV4)) ; uvpt = npt ;
	   if (npt->Dim == 0 || npt->Value.IntVal == 0) return(-1) ;
	   point->Bytes += npt->Bytes ; ADVPNT(npt) ;
	   dictPNTv(npt,Dim_Dim,di->DimId) ; point->Bytes += npt->Bytes ; ADVPNT(npt) ;
/*	   If we have a '?' then set appropriate debug flag */
	   v4lex_NextTkn(tcb,0) ;
	   if (tcb->opcode == V_OpCode_Question)
	    { point->TraceEval |= TRUE ; } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
/*	   Parse a list up until closing bracket */
	   if (di->Flags & V4DPI_DimInfo_ValueList)		/* Want to get list or tree ? */
	    { INITLP(&isctbuf,lp,Dim_List) ;
	      for(;;)
	       { P pnt ;
	         v4lex_NextTkn(tcb,0) ; if (tcb->opcode == V_OpCode_RBracket) break ;
		 v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
		 if(!v4dpi_PointParse(ctx,&pnt,tcb,flags|V4DPI_PointParse_NotADim)) return(-1) ;		/* Parse point into temp buffer */
		 if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&pnt,0)) return(-1) ;
	       } ; ENDLP(&isctbuf,lp) ;
	      if (isctbuf.Bytes + point->Bytes >= sizeof *point)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsIsctBigMx",isctbuf.Bytes,point->Bytes,isctbuf.Bytes + point->Bytes,sizeof *point) ; return(-1) ; } ;
	      memcpy(npt,&isctbuf,isctbuf.Bytes) ; point->Bytes += npt->Bytes ; 
	      dictPNTv(uvpt,Dim_UV4,v4im_GetEnumToDictVal(ctx,DE(ValueList),Dim_UV4)) ;	/* Flip first point to UV4:ValueList */
	    } else
	    { struct V4LEX__RevPolParse rpp ;
	      struct V4Tree__Node *node ;
	      if (!v4lex_RevPolParse(&rpp,tcb,V4LEX_RPPFlag_ImplyAnd|V4LEX_RPPFlag_RParenBrkt|V4LEX_RPPFlag_V4IMEval|V4LEX_RPPFlag_NotADim,ctx)) return(-1) ;
/*	      If we got `isct or `intmod (token[0] = point, token[1] = backquote) then flip to single token point with ForceEval set */
	      if (rpp.Count == 2 && rpp.Token[1].OpCode == V_OpCode_BackQuote && rpp.Token[0].Type == V4LEX_TknType_Point)
	       { ((P *)rpp.Token[0].ValPtr)->ForceEval = TRUE ; rpp.Count = 1 ;
	       } ;
	      switch(rpp.Count)
	       { case 0:
		   intPNTv(npt,UNUSED) ; break ;		/* Nothing to convert? - Return UNUSED as 'node' of non-existant tree */
		 case 1:					/* Do we have an isct or intmod ? */
		   if (rpp.Token[0].Type == V4LEX_TknType_Point ? (((P *)rpp.Token[0].ValPtr)->PntType == V4DPI_PntType_Isct && ((P *)rpp.Token[0].ValPtr)->ForceEval) : FALSE)
		    { memcpy(npt,rpp.Token[0].ValPtr,((P *)rpp.Token[0].ValPtr)->Bytes) ; point->NestedIsct = TRUE ;
		      dictPNTv(uvpt,Dim_UV4,v4im_GetEnumToDictVal(ctx,DE(ValueList),Dim_UV4)) ;	/* Flip first point to UV4:ValueList */
		      break ;
		    } ;
		 /* If not an isct/intmod then fall thru */
		 default:
		   node = v4im_RPPtoTree(ctx,&rpp,TRUE) ; if (node == NULL) return(-1) ;
		   treePNTv(npt,node->Id) ; break ;
	       } ;
	      point->Bytes += npt->Bytes ;
	    } ;
//v_Msg(ctx,NULL,"@valueList -> %1P\n",point) ; vout_UCText(VOUT_Trace,0,ctx->ErrorMsg) ;
	   return(1) ;
	 } 
	else if (tcb->opcode == V_OpCode_LParen || tcb->opcode == V_OpCode_NCLParen)		/* Want to convert dim:(xxx) -> [UV4:Values dim (xxx)] */
	 { struct V4DPI__Point_IntMix *pim ; COUNTER cnt ;
	   P lpnt ; struct V4L__ListPoint *lp ;
	   INITISCT(point) ; ISCTVCD(point) ; point->Grouping = 3 ;
	   npt = ISCT1STPNT(point) ; dictPNTv(npt,Dim_UV4,v4im_GetEnumToDictVal(ctx,DE(ValuesTree),Dim_UV4)) ; uvpt = npt ;
	   if (npt->Dim == 0 || npt->Value.IntVal == 0) return(-1) ;
	   point->Bytes += npt->Bytes ; ADVPNT(npt) ;
/*	   If we have a '?' then set appropriate debug flag */
	   v4lex_NextTkn(tcb,0) ;
	   if (tcb->opcode == V_OpCode_Question)
	    { point->TraceEval |= TRUE ; } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
/*	   Check for optional +dim[+dim...] */
	   dictPNTv(npt,Dim_Dim,di->DimId) ;		/* npt = main dimension */
	   pim = (struct V4DPI__Point_IntMix *)&npt->Value ;
	   for(cnt=0;cnt<10;cnt++)
	    { v4lex_NextTkn(tcb,0) ;
	      if (tcb->opcode != V_OpCode_Plus) { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; break ; } ;
	      v4lex_NextTkn(tcb,0) ; if (tcb->opcode != V_OpCode_Keyword) { v_Msg(ctx,ctx->ErrorMsgAux,"CmdDimName1",tcb->type) ; return(-1) ; } ;
	      if (npt->Grouping == V4DPI_Grouping_Single)
	       { pim->Entry[npt->Grouping].BeginInt = (pim->Entry[npt->Grouping].EndInt = di->DimId) ; npt->Grouping++ ;
	       } ;
	      pim->Entry[npt->Grouping].BeginInt = (pim->Entry[npt->Grouping].EndInt = v4dpi_DimGet(ctx,tcb->UCkeyword,DIMREF_REF)) ;
	      if (pim->Entry[npt->Grouping].BeginInt == 0) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsNotDim",tcb->UCkeyword) ; return(-1) ; } ;
	      npt->Grouping++ ; SETBYTESGRPINT(npt) ;
	    } ;
	   point->Bytes += npt->Bytes ; ADVPNT(npt) ;
/*	   Look for optional field list enclosed in <...> */
#ifdef USE_NEW_XDB
	   INITLP(&lpnt,lp,Dim_UDBArgs)
#else
	   INITLP(&lpnt,lp,Dim_List)
#endif
	   v4lex_NextTkn(tcb,0) ;
	   if (tcb->opcode == V_OpCode_Langle)
	    { struct V4DPI__DimInfo *diNId ;
	      DIMINFO(diNId,ctx,Dim_NId) ;
	      for(;;)
	       { struct V4DPI__LittlePoint elPnt ;
	         v4lex_NextTkn(tcb,0) ; if (tcb->opcode == V_OpCode_Rangle) break ;
	         if (tcb->opcode != V_OpCode_Keyword) { v_Msg(ctx,ctx->ErrorMsgAux,"TableOptName",tcb->type,DE(Element)) ; return(-1) ; } ;
	         dictPNTv((P *)&elPnt,Dim_NId,v4dpi_DictEntryGet(ctx,0,tcb->UCkeyword,diNId,NULL)) ;
	         v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,(P *)&elPnt,0) ;
	       } ;
	    } else if (tcb->opcode == V_OpCode_LangleRangle) { /* just ignore <> - treat as empty list */
	    } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; } ;
	   ENDLP(&lpnt,lp) ;
	   if (lpnt.Bytes > (sizeof lpnt) / 2)			/* Make sure list of fields not too big */
	    { v_Msg(ctx,ctx->ErrorMsgAux,"ListPtLen",&lpnt,lpnt.Bytes,(sizeof lpnt) / 2) ; return(-1) ; } ;
	   if (di->Flags & V4DPI_DimInfo_ValueList)		/* Want to get list or tree ? */
	    { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
	      if(!v4dpi_PointParse(ctx,&isctbuf,tcb,flags|V4DPI_PointParse_NotADim))
	       return(-1) ;
	      if (isctbuf.Bytes + point->Bytes >= sizeof *point)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsIsctBigMx",isctbuf.Bytes,point->Bytes,isctbuf.Bytes + point->Bytes,sizeof *point) ; return(-1) ; } ;
	      memcpy(npt,&isctbuf,isctbuf.Bytes) ; point->Bytes += npt->Bytes ;
	      dictPNTv(uvpt,Dim_UV4,v4im_GetEnumToDictVal(ctx,DE(ValuesList),Dim_UV4)) ;	/* Flip first point to UV4:ValuesList */
	    } else
	    { struct V4LEX__RevPolParse rpp ;
	      struct V4Tree__Node *node ;
	      if (!v4lex_RevPolParse(&rpp,tcb,V4LEX_RPPFlag_ImplyAnd|V4LEX_RPPFlag_RParenBrkt|V4LEX_RPPFlag_V4IMEval|V4LEX_RPPFlag_NotADim,ctx))
	       return(-1) ;
/*	      If we got `isct or `intmod (token[0] = point, token[1] = backquote) then flip to single token point with ForceEval set */
	      if (rpp.Count == 2 && rpp.Token[1].OpCode == V_OpCode_BackQuote && rpp.Token[0].Type == V4LEX_TknType_Point)
	       { ((P *)rpp.Token[0].ValPtr)->ForceEval = TRUE ; rpp.Count = 1 ;
	       } ;
	      switch(rpp.Count)
	       { case 0:
		   treePNTv(npt,TREE_NONE) ; break ;		/* Nothing to convert? - Return UNUSED as 'node' of non-existant tree */
		 case 1:					/* Do we have an isct or intmod ? */
		   if (rpp.Token[0].Type == V4LEX_TknType_Point ? (((P *)rpp.Token[0].ValPtr)->PntType == V4DPI_PntType_Isct && ((P *)rpp.Token[0].ValPtr)->ForceEval) : FALSE)
		    { memcpy(npt,rpp.Token[0].ValPtr,((P *)rpp.Token[0].ValPtr)->Bytes) ; point->NestedIsct = TRUE ;
		      dictPNTv(uvpt,Dim_UV4,v4im_GetEnumToDictVal(ctx,DE(ValuesList),Dim_UV4)) ;	/* Flip first point to UV4:ValueList */
		      break ;
		    } ;
		 /* If not an isct/intmod then fall thru */
		 default:
		   node = v4im_RPPtoTree(ctx,&rpp,TRUE) ; if (node == NULL) return(-1) ;
		   treePNTv(npt,node->Id) ; break ;
	       } ;
	      point->Bytes += npt->Bytes ;
/*	      Add in the element list but set LHSCnt so that the matching binding does not need an explicit list */
	      ADVPNT(npt) ; memcpy(npt,&lpnt,lpnt.Bytes) ; point->Bytes += npt->Bytes ;
	      point->LHSCnt = point->Grouping ; point->Grouping++ ;
	    } ;
	   return(1) ;
	 } else { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; return(0) ; } ;
}

LOGICAL v4dpi_ParseBigTextAttributes(ctx,tcb,tpo,point,comment,xmlEnd,errBuf)
  struct V4C__Context *ctx ;
  struct V4LEX__TknCtrlBlk *tcb ;
  struct V4LEX__TablePrsOpt *tpo ;
  struct V4DPI__Point *point ;
  UCCHAR *comment ;
  UCCHAR *xmlEnd ;
  UCCHAR *errBuf ;
{
  enum DictionaryEntries btAtt ;
  LOGICAL ok ;
#define BTSYNERR(MSG) { UCstrcpy(errBuf,UClit(MSG)) ; return(FALSE) ; }

	v4lex_NextTkn(tcb,0) ; if (tcb->type != V4LEX_TknType_Keyword) BTSYNERR("Expecting BigText \"<hdr>\"") ;
	v_Msg(ctx,xmlEnd,"@</%1U>",tcb->UCkeyword) ;	/* Save the keyword */
	for(ok=TRUE;ok;)		/* Parse any embedded attributes until ">" */
	 { 
	   switch (btAtt=v4im_GetTCBtoEnumVal(ctx,tcb,0))
	    { default:
		if (tcb->opcode == V_OpCode_Rangle) { ok = FALSE ; continue ; } ;
		BTSYNERR("Invalid BigText attribute keyword") ;
	      case _NewLine:
	      case _Comment:
	      case _EmptyLine:
	      case _Indent:
	      case _Dimension:
	      case _Dim:	break ;
	      case _Javascript:	tpo->readLineOpts |= V4LEX_ReadLine_Javascript ; continue ;
	    } ;
	   v4lex_NextTkn(tcb,0) ;
	   if (tcb->opcode != V_OpCode_Equal) BTSYNERR("Expecting \"=\" after BigText keyword") ;
	   v4lex_NextTkn(tcb,0) ;
	   if (tcb->type == V4LEX_TknType_String)
	    { if (UCstrlen(tcb->UCstring) >= UCsizeof(comment)) BTSYNERR("BigText comment string too long") ;
	      UCstrcpy(tcb->UCkeyword,tcb->UCstring) ;
	    } else { if (tcb->type != V4LEX_TknType_Keyword) BTSYNERR("Expecting BigText value") ; } ;
	   switch (btAtt)
	    { case _NewLine:	/* NEWLINE */
		tpo->readLineOpts = (tpo->readLineOpts & ~V4LEX_ReadLine_NLAsIs) ;	/* Turn off default */
		switch (v4im_GetUCStrToEnumVal(tcb->UCkeyword,0))
		 { default:	BTSYNERR("Invalid BigText NewLine=xxx value") ;
		   case _None:	tpo->readLineOpts |= V4LEX_ReadLine_NLNone ; break ;
		   case _Input:	tpo->readLineOpts |= V4LEX_ReadLine_NLAsIs ; break ;
		   case _NL:	tpo->readLineOpts |= V4LEX_ReadLine_NLNL ; break ;
		 }
		break ;
	      case _Comment:
		UCstrcpy(comment,tcb->UCkeyword) ;
		break ;
	      case _EmptyLine:
		tpo->readLineOpts = (tpo->readLineOpts & ~V4LEX_ReadLine_MTLAsIs) ;	/* Turn off default */
		switch (v4im_GetUCStrToEnumVal(tcb->UCkeyword,0))
		 { default:	BTSYNERR("Invalid BigText EmptyLine=xxx value") ;
		   case _Keep:	tpo->readLineOpts |= V4LEX_ReadLine_MTLAsIs ; break ;
		   case _Ignore:tpo->readLineOpts |= V4LEX_ReadLine_MTLTrash ; break ;
		 }
		break ;
	      case _Indent:
		tpo->readLineOpts = (tpo->readLineOpts & ~V4LEX_ReadLine_IndentAsIs) ;	/* Turn off default */
		switch (v4im_GetUCStrToEnumVal(tcb->UCkeyword,0))
		 { default:	BTSYNERR("Invalid BigText Indent=xxx value") ;
		   case _Ignore:tpo->readLineOpts |= V4LEX_ReadLine_IndentNone ; break ;
		   case _Input:	tpo->readLineOpts |= V4LEX_ReadLine_IndentAsIs ; break ;
		   case _Blank:	tpo->readLineOpts |= V4LEX_ReadLine_IndentBlank ; break ;
		   case _Tab:	tpo->readLineOpts |= V4LEX_ReadLine_IndentTab ; break ;
		 }
		break ;
	      case _Dim:
		{ struct V4DPI__DimInfo *dix ; DIMID dimId = v4dpi_DimGet(ctx,tcb->UCkeyword,DIMREF_REF) ;
		  dix = (dimId == 0 ? NULL : v4dpi_DimInfoGet(ctx,point->Dim)) ;
		  if (dix == NULL) BTSYNERR("BigText Dimension attribute - not a dimension") ;
		  if (dix->PointType != V4DPI_PntType_BigText)
		   BTSYNERR("BigText Dimension - not type BigText") ;
		  point->Dim = dimId ; break ;
		}
	    } ;
	 } ;
	return(TRUE) ;
}

/*	v4dpi_SaveBigTextPoint - Creates a BigText Point from V4LEX__BigText Structure */
/*	Call: ok = v4dpi_SaveBigTextPoint( ctx , bt , bytes , point , dimid, lclOK )
	  where ok is TRUE if all went OK, FALSE if problems (ctx->ErrorMsgAux with error),
		ctx is context,
		bt is V4LEX_BigText with text portion filled in,
		bytes is number of text bytes,
		point is pointer to point to be updated,
		dimid is dimension to use (UNUSED for default),
		lclOK is TRUE if ok to save short bigtext points within point itself	*/

LOGICAL v4dpi_SaveBigTextPoint(ctx,bt,bytes,point,dimid,lclOK)
  struct V4C__Context *ctx ;
  struct V4LEX__BigText *bt ;
  int bytes ;
  struct V4DPI__Point *point ;
  int dimid ;
  LOGICAL lclOK ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4DPI__DimInfo *di ;
  int i,aid,bigtextdim,perm ; INDEX areax ;

/*	If we don't have a dimension then use (create) StrBigText dimension */
	if (dimid != UNUSED) { DIMINFO(di,ctx,dimid) ; }
	 else { if ((bigtextdim = v4dpi_DimGet(ctx,UClit("StrBigText"),DIMREF_IRT)) == 0)
	         { struct V4DPI__DimInfo diminfo ; memset(&diminfo,0,sizeof diminfo) ;
	           UCstrcpy(diminfo.DimName,UClit("StrBigText")) ; UCstrcpy(diminfo.Desc,UClit("Auto-created BigText dimension")) ;
	           diminfo.PointType = V4DPI_PntType_BigText ; diminfo.UniqueOK = V4DPI_DimInfo_UOkInt ;
		   diminfo.DictType = V4DPI_DictType_Ext ;
	           bigtextdim = v4dpi_DimMake(ctx,&diminfo) ;
	         } ;
		DIMINFO(di,ctx,bigtextdim) ;
	      } ;
	ZPH(point) ; point->Dim = di->DimId ; point->PntType = V4DPI_PntType_BigText ;
	if (lclOK && bytes < V4DPI_UCVAL_MaxSafe)	/* If string will fit in UCChar point then do it */
	 { point->Value.Int2Val[0] = UNUSED ;
	   bt->BigBuf[bytes] = UCEOS ; UCstrcpy((UCCHAR *)&point->Value.Int2Val[1],bt->BigBuf) ;
	   point->Bytes = V4DPI_PointHdr_Bytes + ALIGN(sizeof point->Value.Int2Val[0] + (sizeof(UCCHAR) * (bytes + 1))) ;
//{ static int tot=0 ;
//  tot += point->Bytes ;
//  v_Msg(ctx,UCTBUF1,"@%0F --small BigText-- %1d bytes, total %2d\n",point->Bytes,tot) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
//}
	   return(TRUE) ;
	 } ;


//	bt->BigBuf[bytes] = UCEOS ;			/* Make sure ends with NULL */
//	bt->Bytes = ALIGN((char *)&bt->BigBuf[bytes+5] - (char *)bt) ;
//	point->Value.IntVal = v4seg_PutSegments(ctx,bt,bt->Bytes,FALSE,FALSE) ;
//	if (point->Value.IntVal == UNUSED) return(FALSE) ;
//	point->Bytes = V4PS_Int ; return(TRUE) ;
//xxxxxx



	bt->kp.fld.KeyType = V4IS_KeyType_V4 ; bt->kp.fld.KeyMode = V4IS_KeyMode_Int ;
	bt->kp.fld.Bytes = V4IS_IntKey_Bytes ; bt->kp.fld.AuxVal = V4IS_SubType_BigText ;
	bt->Key = v4dpi_DimUnique(ctx,di,&perm) ;
//	if (!perm) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIDimROAreaBT",V4DPI_PntType_BigText,di->DimId) ; vout_UCText(VOUT_Warn,0,ctx->ErrorMsgAux) ; vout_NL(VOUT_Warn) ; } ;
	if (bt->Key == UNUSED) return(FALSE) ;
	bt->BigBuf[bytes] = UCEOS ;			/* Make sure ends with NULL */
	bt->Bytes = ALIGN((char *)&bt->BigBuf[bytes+5] - (char *)bt) ;
	point->Value.IntVal = bt->Key ; point->Bytes = V4PS_Int ;
	for(i=gpi->HighHNum;i>=gpi->LowHNum;i--)	/* Find highest area allowing new entries */
	 { if (gpi->RelH[i].aid == UNUSED) continue ; if (i == V4DPI_WorkRelHNum) continue ;
	   if (gpi->RelH[i].ahi.ExtDictUpd) break ;
	 } ; if (i < gpi->LowHNum) i = V4DPI_WorkRelHNum ;
	aid = gpi->RelH[i].aid ;
//{ static int tot=0 ;
//  tot += bt->Bytes ;
//  v_Msg(ctx,UCTBUF1,"@%0F --BigText-- %1d bytes, total %2d\n",bt->Bytes,tot) ;
//  if (bt->Bytes > 10000) vout_UCText(VOUT_Trace,0,UCTBUF1) ;
//}
	FINDAREAINPROCESS(areax,aid) ;
	GRABAREAMTLOCK(areax) ;
	if (v4is_Insert(aid,(struct V4IS__Key *)bt,bt,bt->Bytes,V4IS_PCB_DataMode_Data,V4IS_DataCmp_None,0,FALSE,FALSE,0,ctx->ErrorMsgAux) == -1)
	 { FREEAREAMTLOCK(areax) ; return(FALSE) ; } ;
	FREEAREAMTLOCK(areax) ;
	return(TRUE) ;
}

/*	v4dpi_SaveBigTextPoint2 - Creates a BigText Point from character string */
/*	Call: ok = v4dpi_SaveBigTextPoint2( ctx , srcstr , point , dimid, lclOK )
	  where ok is TRUE if all went OK, FALSE if problems (ctx->ErrorMsgAux with error),
		ctx is context,
		srcstr is null terminated string,
		point is pointer to point to be updated,
		dimid is dimension to use (UNUSED for default),
		lclOK is TRUE if OK to save short text within point		*/

LOGICAL v4dpi_SaveBigTextPoint2(ctx,srcstr,point,dimid,lclOK)
  struct V4C__Context *ctx ;
  UCCHAR *srcstr ;
  struct V4DPI__Point *point ;
  int dimid ;
  LOGICAL lclOK ;
{ struct V4LEX__BigText bt ; int sslen ;

	sslen = UCstrlen(srcstr) ;
	if (sslen >= UCsizeof(bt.BigBuf))
	 { v_Msg(ctx,ctx->ErrorMsgAux,"BigTextMaxLen",UCstrlen(srcstr),sizeof bt.BigBuf,V4DPI_PntType_BigText) ; return(FALSE) ; } ;
	UCstrcpy(bt.BigBuf,srcstr) ;
	return(v4dpi_SaveBigTextPoint(ctx,&bt,sslen,point,dimid,lclOK)) ;

}


/*	v4_BigTextCharValue - Returns point to bigtext string, NULL if failure	*/
/*	Call: string = v4_BigTextCharValue( ctx , point )
	  where string is pointer to ASCIZ string (NULL if error, ctx->ErrorMsgAux with message),
	  ctx is context,
	  point is BigText point to retrieve					*/
		
UCCHAR *v4_BigTextCharValue(ctx,point)
  struct V4C__Context *ctx ;
  P *point ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4LEX__BigText *btx ;
  int btkey ;					/* Key to bigext entry */
  int rx,i ; INDEX areax ;

	btkey = point->Value.IntVal ;		/* Grab key to BigText entry */
	if (btkey == UNUSED)			/* No BigText entry - grab string after IntVal */
	 return((UCCHAR *)&point->Value.Int2Val[1]) ;
	if (ctx->bigtextCur != NULL ? ctx->bigtextCur->Key == btkey : FALSE) return(ctx->bigtextCur->BigBuf) ;
	i = (ctx->bigtextBufCnt++)%V4DPI_BigTextBuf_Max ;
	if (ctx->bigtextBufs[i] == NULL) ctx->bigtextBufs[i] = (struct V4LEX__BigText *)v4mm_AllocChunk(sizeof *ctx->bigtextCur,FALSE) ;
	ctx->bigtextCur = ctx->bigtextBufs[i] ;
	ctx->bigtextCur->kp.fld.KeyType = V4IS_KeyType_V4 ; ctx->bigtextCur->kp.fld.KeyMode = V4IS_KeyMode_Int ;
	ctx->bigtextCur->kp.fld.Bytes = V4IS_IntKey_Bytes ; ctx->bigtextCur->kp.fld.AuxVal = V4IS_SubType_BigText ;
	ctx->bigtextCur->Key = btkey ;



//	btx = v4seg_GetSegments(ctx,point->Value.IntVal,NULL,FALSE,UNUSED) ;
//	if (btx == NULL) return(NULL) ;
//	memcpy(ctx->bigtextCur,btx,btx->Bytes) ;
//	return(ctx->bigtextCur->BigBuf) ;
	





	if ( (rx = XTRHNUMDICT(btkey)) == 0) rx = gpi->HighHNum ;	/* Try to get HNum from key */
//	if (rx < 0 || rx > gpi->HighHNum || gpi->RelH[rx].aid == UNUSED)
//	 { strcpy(ctx->ErrorMsgAux,"Invalid BIGTEXT key") ; return(NULL) ; } ;
	FINDAREAINPROCESS(areax,gpi->RelH[rx].aid) ;
	GRABAREAMTLOCK(areax) ;
	if ((rx < 0 || rx > gpi->HighHNum || gpi->RelH[rx].aid == UNUSED) ? TRUE 
		: (v4is_PositionKey(gpi->RelH[rx].aid,(struct V4IS__Key *)ctx->bigtextCur,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey))
	 { for(rx=gpi->HighHNum;rx>=gpi->LowHNum;rx--)
	    { if (gpi->RelH[rx].aid == UNUSED) continue ; if (rx == V4DPI_WorkRelHNum) continue ;
	      if (v4is_PositionKey(gpi->RelH[rx].aid,(struct V4IS__Key *)ctx->bigtextCur,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) == V4RES_PosKey) break ;
	    } ;
	   if (rx < gpi->LowHNum)
	    { rx = V4DPI_WorkRelHNum ;
	      if (v4is_PositionKey(gpi->RelH[rx].aid,(struct V4IS__Key *)ctx->bigtextCur,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey)
	       { FREEAREAMTLOCK(areax) ; v_Msg(ctx,ctx->ErrorMsgAux,"BigTextNoVal1",btkey) ; ctx->bigtextCur->Key = UNUSED ; return(NULL) ; } ;
	    } ;
	 } ;
	btx = (struct V4LEX__BigText *)v4dpi_GetBindBuf(ctx,gpi->RelH[rx].aid,ikde,FALSE,NULL) ;
	memcpy(ctx->bigtextCur,btx,btx->Bytes) ;
	FREEAREAMTLOCK(areax) ;
	return(ctx->bigtextCur->BigBuf) ;
} ;



#define ShowCtxDimMax 100		/* Keep track of dimensions we have shown values for */
int ShowCtxCnt=UNUSED ;
int ShowCtxDims[ShowCtxDimMax] ;


/*	v4dpi_PointToString - Formats a point into a string		*/
/*	Call: str = v4dpi_PointToString( dststr , point , ctx , format )
	  where str is ptr to character string (i.e. dststr),
		dststr is pointer to destination string,
		point is pointer to point to be formatted,
		ctx is pointer to current context,
		format is a bitmap of format options- V4DPI_FormatOpt_xxx	*/

UCCHAR *v4dpi_PointToString(dststr,point,ctx,format)
  UCCHAR *dststr ;
  struct V4DPI__Point *point ;
  struct V4C__Context *ctx ;
  int format ;
{ 
	return(v4dpi_PointToStringML(dststr,point,ctx,format,gpi->MaxPointOutput)) ;
}

/*	v4dpi_PointToStringML - Formats a point into a string (with max output length)		*/
/*	Call: str = v4dpi_PointToStringML( dststr , point , ctx , format )
	  where str is ptr to character string (i.e. dststr),
		dststr is pointer to destination string,
		point is pointer to point to be formatted,
		ctx is pointer to current context,
		format is a bitmap of format options- V4DPI_FormatOpt_xxx,
		maxlen is maximum length output may be						*/

UCCHAR *v4dpi_PointToStringML(dststr,point,ctx,format,maxlen)
  UCCHAR *dststr ;
  struct V4DPI__Point *point ;
  struct V4C__Context *ctx ;
  int format,maxlen ;
{ struct V4MM__MemMgmtMaster *mmm ;
  struct V4DPI__Point_IntMix *pim ;
  struct V4DPI__Point_RealMix *prm ;
  struct V4DPI__Point_FixMix *pfm ;
  struct V4DPI__Point_AlphaMix *pam ;
  struct V4DPI__Point_UOMMix *pum ;
  struct V4DPI__Point_UOMPerMix *pupm ;
  struct V4DPI__Point *ipnt ;
  struct V4DPI__Point tpnt,dpnt ;
  struct V4DPI__BigIsct *bibuf,*bibp ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4L__ListPoint *lp ;
  struct V4L__ListCmpndRange *lcr ;
  struct V4L__ListCmpndRangeDBL *lcrd ;
  struct V4L__ListAggRef *lar ;
  struct V4L__ListMultiAppend *vlma ;
  struct V4L__ListIsct *lisct ;
  struct V4L__ListMDArray *lmda ;
#ifdef WANTHTMLMODULE
  struct V4L__ListHTMLTable *lht ;
#endif
  struct V4L__ListFiles *lf ;
  struct V4L__ListTextFile *ltf ;
  struct V4IM__Drawer *drw ;
  struct V4FFI__DataElSpec *des ;
  struct V4FFI__StructElSpec *ses ;
  struct V4DPI__DimInfo *di,*di1 ;
  struct V4DPI__PntV4IS *pis ;
  struct V4IS__V4Areas *v4a ;
  struct V4DPI__TagVal *tv ;				/* Structure of a tagged value */
  struct V4DPI__UOMTable *uomt ;
  struct V4DPI_UOMDetails *uomd ;
  struct V4IM__MDArray *vmda ;
  int ShowCtxOwner = FALSE ;				/* If TRUE then this instance "owns" list & should reset on exit */
  UCCHAR *ups ; double ufctr,preOffset,postOffset ; int uprefix ; char *bp ; INDEX areax ;
  UCCHAR *ubp, tb1[5000], tb2[5000], *tbuf, *ls ; int i,j,fx,nq,len,showdim ; double d1,d2 ; B64INT b64,b641,b642 ;

	if (CHECKV4INIT) { UCstrcpy(dststr,ctx->ErrorMsgAux) ; return(dststr) ; } ;
	ZUS(dststr) ;
	if (point == NULL) { UCstrcat(dststr,UClit("?nullpoint?")) ; return(dststr) ; } ;
	if (format == 0) format = V4DPI_FormatOpt_Default ;
	if (format == -1) format = V4DPI_FormatOpt_Trace ;
	switch(point->PntType)
	 { default:			showdim = (format & V4DPI_FormatOpt_Echo) == 0 ; break ; /* Default is to show dimension (dim:...) unless in Echox() */
	   case V4DPI_PntType_Isct:
	   case V4DPI_PntType_QIsct:
	   case V4DPI_PntType_BigIsct:
	   case V4DPI_PntType_TagVal:	showdim = FALSE ; break ;
	 } ;
	if (format & V4DPI_FormatOpt_Point) showdim = TRUE ;		/* Always show dimension if doing 'point' formatting */
	if (ISQUOTED(point)) UCstrcat(dststr,UClit("@")) ; if (point->ForceEval) UCstrcat(dststr,UClit("`")) ;
/*	If point is NOT an intersection then preface with "dim:" */
	di = v4dpi_DimInfoGet(ctx,((point->PntType == V4DPI_PntType_Isct || point->PntType == V4DPI_PntType_BigIsct || point->PntType == V4DPI_PntType_QIsct || point->PntType == V4DPI_PntType_SymDef) ? Dim_Isct : point->Dim)) ;
	if (di == NULL)
	 { v_Msg(ctx,tb1,"@?dim=%1d, type=%2d, group=%3d, bytes=%4d?",point->Dim,point->PntType,point->Grouping,point->Bytes) ;
	   UCstrcat(dststr,tb1) ; return(dststr) ;
	 } else
	 { 
#ifdef V4_BUILD_SECURITY
	   if (di->rtFlags & V4DPI_rtDimInfo_Hidden)
	    { UCstrcpy(dststr,UClit("UV4:hidden")) ; return(dststr) ; } ;
#endif
	 } ;
	format |= V4DPI_FormatOpt_ListInParens ;
	if (showdim || (format & (V4DPI_FormatOpt_ShowDimAlways|V4DPI_FormatOpt_ShowDim)))
	 { UCCHAR *tbuf ;
	   tbuf = (di == NULL ? UClit("???") : di->DimName) ;
	   if (format & V4DPI_FormatOpt_ShowBytes)
	    { UCstrcat(dststr,tbuf) ; v_Msg(ctx,tb1,"@(%1d)",point->Bytes) ; UCstrcat(dststr,tb1) ; UCstrcat(dststr,UClit(":")) ;
	    } else
	    { 
	      i = (di->Flags & V4DPI_DimInfo_NoNamePrefix) != 0 ; if (i) format &= ~V4DPI_FormatOpt_ShowDim ;
	      if ((format & V4DPI_FormatOpt_ShowDimAlways) || point->PntType == V4DPI_PntType_Special || (!i))	/* If Dim:List/Alpha then don't preface with List: */
	       { UCstrcat(dststr,tbuf) ; UCstrcat(dststr,UClit(":")) ; } ;
	    } ;
	 } ;
	if ((point->PntType != V4DPI_PntType_Special)		/* If a displayer function then tweak here & drop thru */
		&& (di == NULL ? FALSE : ((di->Flags & V4DPI_DimInfo_DisplayerTrace) && (format & V4DPI_FormatOpt_DisplayerTrace)) || ((di->Flags & V4DPI_DimInfo_Displayer) && ((format & V4DPI_FormatOpt_NoDisplayer) == 0))))
	 { INDEX savertStackFail ;



	   switch (point->Grouping)
	    { default:
		switch (point->PntType)
		 { default:			break ;
		   CASEofINT
		   case V4DPI_PntType_Tree:
		   case V4DPI_PntType_Color:
		   case V4DPI_PntType_Country:
		   case V4DPI_PntType_Dict:
		   case V4DPI_PntType_XDict:
		     memcpy(&tpnt,point,V4DPI_PointHdr_Bytes) ; tpnt.Grouping = V4DPI_Grouping_Single ;
		     pim = (struct V4DPI__Point_IntMix *)&point->Value ;
		     for(i=0;i<point->Grouping;i++)
		      { tpnt.Value.IntVal = pim->Entry[i].BeginInt ;
			v4dpi_PointToString(tb1,&tpnt,ctx,V4DPI_FormatOpt_Echo) ; UCstrcat(dststr,tb1) ;
		        if (pim->Entry[i].BeginInt != pim->Entry[i].EndInt)
		         { tpnt.Value.IntVal = pim->Entry[i].EndInt ;
			   v4dpi_PointToString(tb1,&tpnt,ctx,V4DPI_FormatOpt_Echo) ; UCstrcat(dststr,UClit("..")) ; UCstrcat(dststr,tb1) ;
			 } ;
			if (i < point->Grouping - 1) UCstrcat(dststr,UClit(",")) ;
		      } ;
		     return(dststr) ;
		   case V4DPI_PntType_Isct:	goto no_displayer ;		/* Iscts cannot go through a displayer */
		 } ;
	      case V4DPI_Grouping_Single: break ;
	      case V4DPI_Grouping_GT:	UCstrcat(dststr,UClit(">")) ; break ;
	      case V4DPI_Grouping_GE:	UCstrcat(dststr,UClit(">=")) ; break ;
	      case V4DPI_Grouping_LT:	UCstrcat(dststr,UClit("<")) ; break ;
	      case V4DPI_Grouping_LE:	UCstrcat(dststr,UClit("<=")) ; break ;
	      case V4DPI_Grouping_NE:	UCstrcat(dststr,UClit("<>")) ; break ;
	    } ;

	   INITISCT(&tpnt) ; NOISCTVCD(&tpnt) ; tpnt.Grouping = 2 ;			/* Construct [UV4:Displayer point] intersection */
	   ipnt = ISCT1STPNT(&tpnt) ; dictPNTv(ipnt,Dim_UV4,v4im_GetEnumToDictVal(ctx,(((format & V4DPI_FormatOpt_DisplayerTrace) == 0) ? DE(Displayer) : DE(DisplayerTrace)),UNUSED)) ;
	   if (ipnt->Dim == 0 || ipnt->Value.IntVal == 0)
	    { if (format & V4DPI_FormatOpt_DisplayerTrace) goto no_displayer ;
	      v_Msg(ctx,tb1,"*DPIDspNoDisplr",di->DimId) ; vout_UCText(VOUT_Warn,0,tb1) ; goto no_displayer ;
	    } ;
	   ADVPNT(ipnt) ;
	   if (di->ADPnt.Bytes != 0)
	    { memcpy(ipnt,&di->ADPnt,di->ADPnt.Bytes) ; tpnt.Grouping ++ ; ADVPNT(ipnt) ;
	      memcpy(ipnt,point,point->Bytes) ; if (ipnt->PntType == V4DPI_PntType_Int) ipnt->Dim = Dim_Int ;
	    } else
	    { memcpy(ipnt,point,point->Bytes) ;  if (UCnotempty(di->IsA)) ipnt->Dim = v4dpi_DimGet(ctx,di->IsA,DIMREF_IRT) ;
	    } ;
	   ipnt->Grouping = V4DPI_Grouping_Single ;
	   ADVPNT(ipnt) ; ISCTLEN(&tpnt,ipnt) ;
	   savertStackFail = ctx->rtStackFail ;
	   ipnt = v4dpi_IsctEval(&dpnt,&tpnt,ctx,V4DPI_EM_NoIsctFail|V4DPI_EM_NoRegisterErr|V4DPI_EM_NoTrace,NULL,NULL) ;
	   ctx->rtStackFail = savertStackFail ;
	   if (ipnt != NULL) { point = ipnt ; }			/* If eval'd then use this point for display */
	    else { if (format & V4DPI_FormatOpt_DisplayerTrace) goto no_displayer ;
		   v_Msg(ctx,tb1,"*DPIDspDspFail",di->DimId,&tpnt,ctx->ErrorMsg) ; vout_UCText(VOUT_Warn,0,tb1) ;
		 } ;
	 } ;
no_displayer:
	if (point->Grouping > 150)
	 { int grp = point->Grouping ; point->Grouping = 10 ;
	   v_Msg(ctx,tb1,"*DPIDspGrpBig",point,grp,10) ; vout_UCText(VOUT_Warn,0,tb1) ;
	 } ;
	switch (point->PntType)
	 { default:
		UCsprintf(tb1,UCsizeof(tb1),UClit("?PntType=%s Bytes=%d Format=%x"),v4im_PTName(point->PntType),point->Bytes,format) ; UCstrcat(dststr,tb1) ; break ; 
//		v4_error(V4E_UNKPNTTYPE,0,"V4DPI","PointToString","UNKPNTTYPE","Unknown point type (%d)",point->PntType) ;
	   case V4DPI_PntType_Special:
		switch (point->Grouping)
		 { default:			UCsprintf(tb1,UCsizeof(tb1),UClit("{?%d?}"),point->Grouping) ; UCstrcat(dststr,tb1) ; break ;
		   case V4DPI_Grouping_All:
			len = UCstrlen(dststr) ; if (len > 0 ? dststr[len-1] == UClit(':') : FALSE) { len-- ; dststr[len] = UCEOS ; } ; 
			UCstrcat(dststr,UClit("..")) ; break ;
		   case V4DPI_Grouping_AllCnf:
			len = UCstrlen(dststr) ; if (len > 0 ? dststr[len-1] == UClit(':') : FALSE) { len-- ; dststr[len] = UCEOS ; } ; 
			UCstrcat(dststr,di->DimName) ; UCstrcat(dststr,UClit("...")) ; break ;
		   case V4DPI_Grouping_Current:
			ZUS(dststr) ;			/* Start all over - make sure we get dimension name */
			if (ISQUOTED(point)) UCstrcat(dststr,UClit("@")) ; if (point->ForceEval) UCstrcat(dststr,UClit("`")) ;
			UCstrcat(dststr,di->DimName) ; UCstrcat(dststr,UClit("*")) ;
			if (format & V4DPI_FormatOpt_ShowCtxVal)
			 { for(i=0;i<ShowCtxCnt;i++) { if (ShowCtxDims[i] == point->Dim) break ; } ;
			   if (i >= ShowCtxCnt)					/* Only show dimension once */
			    { if (ShowCtxCnt == UNUSED)
			       { ShowCtxOwner = TRUE ; ShowCtxCnt = 0 ; } ;	/* We "own" count & must reset on return */
			      if (ShowCtxCnt < ShowCtxDimMax) { ShowCtxDims[ShowCtxCnt++] = point->Dim ; } ;
			      DIMVAL(ipnt,ctx,point->Dim) ;
			      if (ipnt == NULL) { UCstrcat(dststr,UClit("<undefined>")) ; }
			       else { v4dpi_PointToString(tb1,ipnt,ctx,format|V4DPI_FormatOpt_DisplayerTrace) ; UCstrcat(dststr,UClit("<")) ;
				      if (UCstrlen(tb1) > 250) { tb1[250] = UCEOS ; UCstrcat(tb1,UClit("...")) ; } ;
				      UCstrcat(dststr,tb1) ; UCstrcat(dststr,UClit(">")) ;
				    } ;
			    } ;
			 } ;
			break ;
		   case V4DPI_Grouping_PCurrent:
			if (*(dststr+UCstrlen(dststr)-1) == UClit(':')) *(dststr+UCstrlen(dststr)-1) = 0 ;
			UCstrcat(dststr,UClit("**")) ;
			if (format & V4DPI_FormatOpt_ShowCtxVal)
			 { DIMPVAL(ipnt,ctx,point->Dim) ;
			   if (ipnt == NULL) { UCstrcat(dststr,UClit("<undefined>")) ; }
			    else { v4dpi_PointToString(tb1,ipnt,ctx,format|V4DPI_FormatOpt_DisplayerTrace) ; UCstrcat(dststr,UClit("<")) ;
				   if (UCstrlen(tb1) > 250) { tb1[250] = UCEOS ; UCstrcat(tb1,UClit("...")) ; } ;
				   UCstrcat(dststr,tb1) ; UCstrcat(dststr,UClit(">")) ;
				 } ;
			 } ;
			break ;
		   case V4DPI_Grouping_Now:		UCstrcat(dststr,UClit("{NOW}")) ; break ;
		   case V4DPI_Grouping_New:		UCstrcat(dststr,UClit("{NEW}")) ; break ;
//		   case V4DPI_Grouping_LastBind:	UCstrcat(dststr,UClit("{LASTBIND}")) ; break ;
		   case V4DPI_Grouping_Undefined:	UCstrcat(dststr,UClit("{UNDEFINED}")) ; break ;
		   case V4DPI_Grouping_Sample:		UCstrcat(dststr,UClit("{SAMPLE}")) ; break ;
//		   case V4DPI_Grouping_DecompLHS:	UCstrcat(dststr,UClit("?l")) ; break ;
//		   case V4DPI_Grouping_DecompRHS:	UCstrcat(dststr,UClit("?r")) ; break ;
		   case V4DPI_Grouping_None:
			if ((di->Flags & V4DPI_DimInfo_HaveNone) != 0 && (format & V4DPI_FormatOpt_Echo) != 0)
			 { switch (di->PointType)
			    { case V4DPI_PntType_Int:	UCsprintf(tb1,UCsizeof(tb1),UClit("%d"),di->ds.Int.NoneValue) ; break ;
			      case V4DPI_PntType_Real:	UCsprintf(tb1,UCsizeof(tb1),UClit("%g"),di->ds.Real.NoneValue) ; break ;
			    } ;
			   UCstrcat(dststr,tb1) ; break ;
			 } ;
			UCstrcat(dststr,gpi->ci->li->LI[gpi->ci->li->CurX].None) ;
			break ;
		 } ;
		if (point->Continued)
		 { v4dpi_PointToString(tb1,(P *)&point->Value,ctx,format) ;
		   UCstrcat(dststr,UClit(",")) ; UCstrcat(dststr,tb1) ;
		 } ;
		break ;
	   case V4DPI_PntType_UTime:
		switch (point->Grouping)
		 { default: goto no_relUTime ;
		   case V4DPI_Grouping_Single: break ;
		   case V4DPI_Grouping_GT:	UCstrcat(dststr,UClit(">")) ; break ;
		   case V4DPI_Grouping_GE:	UCstrcat(dststr,UClit(">=")) ; break ;
		   case V4DPI_Grouping_LT:	UCstrcat(dststr,UClit("<")) ; break ;
		   case V4DPI_Grouping_LE:	UCstrcat(dststr,UClit("<=")) ; break ;
		   case V4DPI_Grouping_NE:	UCstrcat(dststr,UClit("<>")) ; break ;
		 } ;
		if (point->Value.IntVal == UNUSED) { UCstrcat(dststr,gpi->ci->li->LI[gpi->ci->li->CurX].None) ; break ; } ;
		GETREAL(d1,point) ;
		v_FormatDate(&d1,V4DPI_PntType_UTime,VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,NULL,tb1) ; UCstrcat(dststr,tb1) ;
		break ;
/*		Have a list or range or whatever - do it */
no_relUTime:	prm = (struct V4DPI__Point_RealMix *)&point->Value ;
		for(i=0;i<point->Grouping;i++)
		 { memcpy(&d1,&prm->Entry[i].BeginReal,SIZEofDOUBLE) ; memcpy(&d2,&prm->Entry[i].EndReal,SIZEofDOUBLE) ;
		   if (d1 == d2)
		    { if (d1 == UNUSED) { UCstrcatToASC(dststr,gpi->ci->li->LI[gpi->ci->li->CurX].None) ; }
		       else { v_FormatDate(&d1,V4DPI_PntType_UTime,VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,NULL,tb1) ; UCstrcat(dststr,tb1) ; }
		    } else
		    { v_FormatDate(&d1,V4DPI_PntType_UTime,VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,NULL,tb1) ; UCstrcat(dststr,tb1) ;
		      UCstrcat(dststr,UClit("..")) ;
		      v_FormatDate(&d2,V4DPI_PntType_UTime,VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,NULL,tb1) ; UCstrcat(dststr,tb1) ;
		    } ;
		   if (i < point->Grouping-1) UCstrcat(dststr,UClit(",")) ;
		 } ;
		 break ;
	   case V4DPI_PntType_Time:
		if (point->Grouping == V4DPI_Grouping_Single)
		 { if (point->Value.IntVal == UNUSED) { UCstrcat(dststr,gpi->ci->li->LI[gpi->ci->li->CurX].None) ; break ; } ;
		   if (point->Value.IntVal < 700000000)
		    { UCsprintf(tb1,UCsizeof(tb1),UClit("%d"),point->Value.IntVal) ; UCstrcat(dststr,tb1) ;
		    } else 
		    { tbuf = UCasctime(localtime((time_t *)&point->Value.IntVal)) ; *(tbuf+UCstrlen(tbuf)-1) = 0 ;
		      UCstrcat(dststr,UClit("\"")) ; UCstrcat(dststr,tbuf) ; UCstrcat(dststr,UClit("\"")) ;
		    } ;
		   break ;
		 } ;
/*		Have a list or range or whatever - do it */
		pim = (struct V4DPI__Point_IntMix *)&point->Value ;
		for(i=0;i<point->Grouping;i++)
		 { if (pim->Entry[i].BeginInt == pim->Entry[i].EndInt)
		    { if (point->Value.IntVal == UNUSED) { UCstrcat(dststr,gpi->ci->li->LI[gpi->ci->li->CurX].None) ; }
		       else { if (pim->Entry[i].BeginInt < 700000000)
			       { UCsprintf(tb1,UCsizeof(tb1),UClit("%d"),pim->Entry[i].BeginInt) ; UCstrcat(dststr,tb1) ; }
			       else { tbuf = UCasctime(localtime((time_t *)&pim->Entry[i].BeginInt)) ; *(tbuf+UCstrlen(tbuf)-1) = 0 ;
				      UCstrcat(dststr,UClit("\"")) ; UCstrcat(dststr,tbuf) ; UCstrcat(dststr,UClit("\"")) ;
				    } ;
			    } ;
		    } else
		    { if (pim->Entry[i].BeginInt < 700000000)
		       { UCsprintf(tb1,UCsizeof(tb1),UClit("%d"),pim->Entry[i].BeginInt) ; UCstrcat(dststr,tb1) ; UCstrcat(dststr,UClit("..")) ;
			 UCsprintf(tb1,UCsizeof(tb1),UClit("%d"),pim->Entry[i].EndInt) ; UCstrcat(dststr,tb1) ;
		       } else
		       { tbuf = UCasctime(localtime((time_t *)&pim->Entry[i].BeginInt)) ; *(tbuf+UCstrlen(tbuf)-1) = 0 ;
			 UCstrcat(dststr,UClit("\"")) ; UCstrcat(dststr,tbuf) ; UCstrcat(dststr,UClit("\"..")) ;
			 tbuf = UCasctime(localtime((time_t *)&pim->Entry[i].EndInt)) ; *(tbuf+UCstrlen(tbuf)-1) = 0 ;
			 UCstrcat(dststr,UClit("\"")) ; UCstrcat(dststr,tbuf) ; UCstrcat(dststr,UClit("\"")) ;
		       } ;
		    } ;
		   if (i < point->Grouping-1) UCstrcat(dststr,UClit(",")) ;
		 } ;
		 break ;
	   case V4DPI_PntType_Logical:
		if (di->OutFormat[0] == UClit('%'))		/* If got formatting then check it out */
		 { i = UCstrlen(di->OutFormat) ;
		   switch (di->OutFormat[i-1])
		    { default:			UCstrcpy(tb1,(point->Value.IntVal ? UClit("Yes") : UClit("No"))) ; break ;
		      case UClit('s'):		UCsprintf(tb1,UCsizeof(tb1),di->OutFormat,(point->Value.IntVal ? UClit("Yes") : UClit("No"))) ; break ;
		      case UClit('d'):
		      case UClit('f'):
		      case UClit('g'):
		      case UClit('x'):		UCsprintf(tb1,UCsizeof(tb1),di->OutFormat,(point->Value.IntVal ? 1 : 0)) ; break ;
		    } ; UCstrcat(dststr,tb1) ;
		 } else { UCstrcat(dststr,(point->Value.IntVal ? UClit("Yes") : UClit("No"))) ; } ;
		break ;
	   case V4DPI_PntType_V4IS:
		pis = (struct V4DPI__PntV4IS *)&point->Value ;	/* Link up to V4IS info */
		if (pis->Handle == UNUSED && pis->DataId == UNUSED) { UCstrcat(dststr,gpi->ci->li->LI[gpi->ci->li->CurX].None) ; break ; } ;
		if ((v4a = gpi->v4a) == NULL) { UCstrcat(dststr,UClit("?V4IS?")) ; break ; } ;
		for(i=0;i<v4a->Count;i++) { if (pis->Handle == v4a->Area[i].Handle) break ; } ;
		if (i >= v4a->Count) { UCstrcat(dststr,UClit("?V4IS?")) ; break ; } ;
		UCstrcat(dststr,UClit("V4IS(")) ; UCstrcat(dststr,v4a->Area[i].pcb->UCFileName) ;
		if (pis->DataId != UNUSED) { UCsprintf(tb1,UCsizeof(tb1),UClit(",%x"),pis->DataId) ; UCstrcat(dststr,tb1) ; } ;
		if (pis->SubIndex != UNUSED) { UCsprintf(tb1,UCsizeof(tb1),UClit(",%d"),pis->SubIndex) ; UCstrcat(dststr,tb1) ; } ;
		UCstrcat(dststr,UClit(")")) ; break ;
	   case V4DPI_PntType_XDB:
		if (point->Value.XDB.xdbId == 0) { UCstrcat(dststr,gpi->ci->li->LI[gpi->ci->li->CurX].None) ; break ; } ;
		if (XDBISCON(point->Value.XDB.xdbId)) { v_Msg(ctx,dststr,"@XDB.con.%1d",XDBIDPART(point->Value.XDB.xdbId)) ; }
		 else if (point->Value.XDB.xdbId > 0) { v_Msg(ctx,dststr,"@XDB.stmt.%1d.%2d",XDBIDPART(point->Value.XDB.xdbId),point->Value.XDB.recId) ; } ;
		break ;
	   case V4DPI_PntType_Complex:
		memcpy(&d1,&point->Value.Complex.r,sizeof d1) ; memcpy(&d2,&point->Value.Complex.i,sizeof d2) ;
		UCsprintf(tb1,UCsizeof(tb1),UClit("(%g %g)"),d1,d2) ; UCstrcat(dststr,tb1) ;
		break ;
	   case V4DPI_PntType_GeoCoord:
		v_FormatGeoCoord(ctx,&point->Value.GeoCoord,NULL,dststr,maxlen) ;
		break ;
	   case V4DPI_PntType_TeleNum:
		if (gpi->ci->Cntry[gpi->ci->CurX].IntDialCode == point->Value.Tele.IntDialCode)
		 { v_FormatTele(ctx,&point->Value.Tele,gpi->ci->Cntry[gpi->ci->CurX].NTeleMask,tb1) ; }
		 else { v_FormatTele(ctx,&point->Value.Tele,gpi->ci->Cntry[gpi->ci->CurX].ITeleMask,tb1) ; }
		UCstrcat(dststr,tb1) ; break ;
	   case V4DPI_PntType_Drawer:
		drw = v4im_GetDrawerPtr(point->Dim,point->Value.IntVal) ;
		if (drw == NULL) { UCsprintf(tb1,UCsizeof(tb1),UClit("Drawer(id=%d unknown items)"),point->Value.IntVal) ; }
		 else { UCsprintf(tb1,UCsizeof(tb1),UClit("Drawer(id=%d items=%d)"),point->Value.IntVal,drw->Points) ; } ;
		UCstrcat(dststr,tb1) ; break ;
	   case V4DPI_PntType_SymDef:
		UCstrcat(dststr,UClit("$")) ;
		UCstrcat(dststr,v4dpi_RevDictEntryGet(ctx,point->Value.SymDef.dictId)) ;
		if (point->Bytes > V4DPI_SymDefBase_Bytes)
		 { UCstrcat(dststr,(point->Value.SymDef.lsOp == 0 ? UClit(":=") : (point->Value.SymDef.lsOp == 1 ? UClit("+=") : UClit("-=")))) ;
		   v4dpi_PointToStringML(tb1,(P *)&point->Value.SymDef.pntBuf,ctx,V4DPI_FormatOpt_Echo,UCsizeof(tb1)) ; UCstrcat(dststr,tb1) ;
		 } ;
		break ;
	   case V4DPI_PntType_Int2:
		UCsprintf(tb1,UCsizeof(tb1),UClit("<%d %d>"),point->Value.Int2Val[0],point->Value.Int2Val[1]) ; UCstrcat(dststr,tb1) ; break ;
	   case V4DPI_PntType_UWeek:
		if (di->ds.UWeek.baseUDate != VCAL_UDate_None)
		 { if (point->Value.IntVal > 0) goto int_entry ;	/* If we have base date then treat as if integer */
		   if (point->Value.IntVal < 0) { UCstrcpy(dststr,UClit("undefined")) ; break ; } ;
		 } ;
	   case V4DPI_PntType_UQuarter:
	   case V4DPI_PntType_UPeriod:
	   case V4DPI_PntType_UDate:
	   case V4DPI_PntType_UYear:
	   case V4DPI_PntType_UMonth:
		switch (point->Grouping)
		 { default: goto no_relUPeriod ;
		   case V4DPI_Grouping_Single: break ;
		   case V4DPI_Grouping_GT:	UCstrcat(dststr,UClit(">")) ; break ;
		   case V4DPI_Grouping_GE:	UCstrcat(dststr,UClit(">=")) ; break ;
		   case V4DPI_Grouping_LT:	UCstrcat(dststr,UClit("<")) ; break ;
		   case V4DPI_Grouping_LE:	UCstrcat(dststr,UClit("<=")) ; break ;
		   case V4DPI_Grouping_NE:	UCstrcat(dststr,UClit("<>")) ; break ;
		 } ;
		v_FormatDate(&point->Value.IntVal,((di->ds.UPeriod.periodsPerYear<<16) + point->PntType),VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,NULL,tb1) ; UCstrcat(dststr,tb1) ;
		break ;
no_relUPeriod:
		pim = (struct V4DPI__Point_IntMix *)&point->Value ;
		for(i=0;i<point->Grouping;i++)
		 { if (pim->Entry[i].BeginInt == pim->Entry[i].EndInt)
//xxx have to determine type of period (di->xxx)
		    { v_FormatDate(&pim->Entry[i].BeginInt,((di->ds.UPeriod.periodsPerYear<<16) + point->PntType),VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,NULL,tb1) ;
		    } else
		    { v_FormatDate(&pim->Entry[i].BeginInt,((di->ds.UPeriod.periodsPerYear<<16) + point->PntType),VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,NULL,tb1) ; UCstrcat(tb1,UClit("..")) ;
		      v_FormatDate(&pim->Entry[i].EndInt,((di->ds.UPeriod.periodsPerYear<<16) + point->PntType),VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,NULL,tb2) ; UCstrcat(tb1,tb2) ;
		    } ;
		   UCstrcat(dststr,tb1) ; if (i < point->Grouping-1) UCstrcat(dststr,UClit(",")) ;
		 } ;
		 break ;
	   case V4DPI_PntType_Calendar:
		switch (point->Grouping)
		 { default: goto no_relCalendar ;
		   case V4DPI_Grouping_Single: break ;
		   case V4DPI_Grouping_GT:	UCstrcat(dststr,UClit(">")) ; break ;
		   case V4DPI_Grouping_GE:	UCstrcat(dststr,UClit(">=")) ; break ;
		   case V4DPI_Grouping_LT:	UCstrcat(dststr,UClit("<")) ; break ;
		   case V4DPI_Grouping_LE:	UCstrcat(dststr,UClit("<=")) ; break ;
		   case V4DPI_Grouping_NE:	UCstrcat(dststr,UClit("<>")) ; break ;
		 } ;
		GETREAL(d1,point) ;
		v_FormatDate(&d1,V4DPI_PntType_Calendar,di->ds.Cal.CalendarType,di->ds.Cal.TimeZone,NULL,tb1) ;
		UCstrcat(dststr,tb1) ;
		break ;
no_relCalendar:
		prm = (struct V4DPI__Point_RealMix *)&point->Value ;
		for(i=0;i<point->Grouping;i++)
		 { memcpy(&d1,&prm->Entry[i].BeginReal,SIZEofDOUBLE) ; memcpy(&d2,&prm->Entry[i].EndReal,SIZEofDOUBLE) ;
		   if (d1 == d2)
		    { v_FormatDate(&d1,V4DPI_PntType_Calendar,di->ds.Cal.CalendarType,di->ds.Cal.TimeZone,NULL,tb1) ;
		    } else
		    { v_FormatDate(&d1,V4DPI_PntType_Calendar,di->ds.Cal.CalendarType,di->ds.Cal.TimeZone,NULL,tb1) ;
		      UCstrcat(dststr,tb1) ; UCstrcat(dststr,UClit("..")) ;
		      v_FormatDate(&d2,V4DPI_PntType_Calendar,di->ds.Cal.CalendarType,di->ds.Cal.TimeZone,NULL,tb1) ;
		    } ;
		   UCstrcat(dststr,tb1) ; if (i < point->Grouping-1) UCstrcat(dststr,UClit(",")) ;
		 } ;
		 break ;
	   case V4DPI_PntType_UDT:
		switch (point->Grouping)
		 { default: goto no_relUDT ;
		   case V4DPI_Grouping_Single:	break ;
		   case V4DPI_Grouping_GT:	UCstrcat(dststr,UClit(">")) ; break ;
		   case V4DPI_Grouping_GE:	UCstrcat(dststr,UClit(">=")) ; break ;
		   case V4DPI_Grouping_LT:	UCstrcat(dststr,UClit("<")) ; break ;
		   case V4DPI_Grouping_LE:	UCstrcat(dststr,UClit("<=")) ; break ;
		   case V4DPI_Grouping_NE:	UCstrcat(dststr,UClit("<>")) ; break ;
		 } ;
		if (point->Value.IntVal == 0) { UCstrcat(dststr,gpi->ci->li->LI[gpi->ci->li->CurX].None) ; }
		 else { UCstrcat(dststr,mscu_udt_to_ddmmmyyhhmmss(point->Value.IntVal)) ; } ;
		break ;
no_relUDT:
		pim = (struct V4DPI__Point_IntMix *)&point->Value ;
		for(i=0;i<point->Grouping;i++)
		 { if (pim->Entry[i].BeginInt == 0) { UCstrcatToASC(dststr,gpi->ci->li->LI[gpi->ci->li->CurX].None) ; }
		    else { UCstrcat(dststr,mscu_udt_to_ddmmmyyhhmmss(pim->Entry[i].BeginInt)) ; } ;
		   if (pim->Entry[i].BeginInt != pim->Entry[i].EndInt)
		    { UCstrcat(dststr,UClit("..")) ; UCstrcat(dststr,mscu_udt_to_ddmmmyyhhmmss(pim->Entry[i].EndInt)) ; } ;
		   if (i < point->Grouping-1) UCstrcat(dststr,UClit(",")) ;
		 } ;
		 break ;
	   case V4DPI_PntType_AggRef:
		if (point->Value.IntVal == UNUSED) { UCstrcpy(tb1,gpi->ci->li->LI[gpi->ci->li->CurX].None) ; }
		 else { UCsprintf(tb1,UCsizeof(tb1),UClit("%d:%d"),point->Grouping+1,point->Value.IntVal) ; } ;
		UCstrcat(dststr,tb1) ; break ;
	   case V4DPI_PntType_Tag:
		UCstrcat(dststr,v4im_TagName(point->Value.IntVal)) ; break ;
	   case V4DPI_PntType_TagVal:
		tv = (struct V4DPI__TagVal *)&point->Value ;
		if (point->ForceEval)				/* Got tag with question? */
		 { UCstrcpy(tb1,v4im_TagName(tv->TagVal)) ; UCstrcat(tb1,UClit("?")) ;
		 } else
		 { v_Msg(ctx,tb1,"@%1T::%2P",tv->TagVal,&tv->TagPt) ;
		 } ;
		UCstrcpy(dststr,tb1) ; break ;
	   case V4DPI_PntType_PntIdx:
		UCsprintf(tb1,UCsizeof(tb1),UClit("%d->"),point->Value.IntVal) ; UCstrcat(dststr,tb1) ;
		ipnt =	v4dpi_PntIdx_CvtIdxPtr(point->Value.IntVal) ;	/* Get pointer to real point & display it! */
		v4dpi_PointToString(dststr+UCstrlen(dststr),ipnt,ctx,format) ;
		break ;
	   case V4DPI_PntType_BigText:
		tbuf = v4_BigTextCharValue(ctx,point) ; nq = FALSE ;
		if (!(format & V4DPI_FormatOpt_ShowDim)) nq = FALSE ;
		if (format & (V4DPI_FormatOpt_AlphaQuote | V4DPI_FormatOpt_Point)) nq = TRUE ;
		if (tbuf == NULL)
		 { v_Msg(ctx,tb1,"@?BigText:%1U?",ctx->ErrorMsgAux) ; UCstrcat(dststr,tb1) ; }
		 else { if (nq)
			 { v_StringLit(tbuf,&dststr[UCstrlen(dststr)],-maxlen,'"',0) ; }
			 else { len = UCstrlen(tbuf) ;
				 if (len <= maxlen-2) { UCstrcat(dststr,tbuf) ; }
				  else { UCstrncat(dststr,tbuf,maxlen-5) ; UCstrcat(dststr,UClit("...")) ; } ;
			      } ;
		      } ;
		break ;
	   case V4DPI_PntType_OSHandle:
		v_Msg(ctx,tb1,"@OS(%1U %2d)",v_OSH_TypeDisplayer(point->Value.IntVal),point->Value.IntVal) ;
		UCstrcat(dststr,tb1) ; break ;
	   case V4DPI_PntType_Delta:
		if (point->Value.IntVal >= 0) { UCstrcat(dststr,UClit("+")) ; } ;
		UCsprintf(tb1,UCsizeof(tb1),UClit("%d"),point->Value.IntVal) ; UCstrcat(dststr,tb1) ;
		break ;
	   case V4DPI_PntType_IMArg:
		UCsprintf(tb1,UCsizeof(tb1),UClit("%d"),point->Value.IntVal) ; UCstrcat(dststr,tb1) ;
		break ;
	   case V4DPI_PntType_CodedRange:
	   case V4DPI_PntType_Int:
int_entry:
		switch (point->Grouping)
		 { default: goto no_relInt ;
		   case V4DPI_Grouping_Single: break ;
		   case V4DPI_Grouping_GT:	UCstrcat(dststr,UClit(">")) ; break ;
		   case V4DPI_Grouping_GE:	UCstrcat(dststr,UClit(">=")) ; break ;
		   case V4DPI_Grouping_LT:	UCstrcat(dststr,UClit("<")) ; break ;
		   case V4DPI_Grouping_LE:	UCstrcat(dststr,UClit("<=")) ; break ;
		   case V4DPI_Grouping_NE:	UCstrcat(dststr,UClit("<>")) ; break ;
		 } ;
		if (UCnotempty(gpi->formatMasks[V4DPI_PntType_Int]))
		 { v_FormatInt(point->Value.IntVal,NULL,tb1,NULL) ;
		 } else { UCsprintf(tb1,UCsizeof(tb1),(di->OutFormat[0] == UClit('%') ? di->OutFormat : UClit("%d")),point->Value.IntVal) ; } ;
		UCstrcat(dststr,tb1) ;
		break ;
no_relInt:
/*		Have a list or range or whatever - do it */
		pim = (struct V4DPI__Point_IntMix *)&point->Value ;
		for(i=0;i<point->Grouping;i++)
		 { if (pim->Entry[i].BeginInt == pim->Entry[i].EndInt)
		    { UCsprintf(tb1,UCsizeof(tb1),UClit("%d"),pim->Entry[i].BeginInt) ; }
		    else { UCsizeof(tb1),UCsprintf(tb1,UCsizeof(tb1),UClit("%d..%d"),pim->Entry[i].BeginInt,pim->Entry[i].EndInt) ; } ;
		   UCstrcat(dststr,tb1) ; if (i < point->Grouping-1) UCstrcat(dststr,UClit(",")) ;
		 } ;
		 break ;
	   case V4DPI_PntType_MemPtr:
		memcpy(&tbuf,&point->Value.MemPtr,sizeof(tbuf)) ;
		UCsprintf(tb1,UCsizeof(tb1),UClit("^X%p"),tbuf) ; UCstrcat(dststr,tb1) ;
		break ;
	   case V4DPI_PntType_ParsedJSON:
		UCsprintf(tb1,UCsizeof(tb1),UClit("*parsed JSON*")) ; UCstrcat(dststr,tb1) ;
		break ;
	   case V4DPI_PntType_UOM:
		if (gpi->uomt == NULL) v4dpi_UOMInitialize(ctx,UNUSED) ;	/* Attempt to init if necessary */
		uomt = gpi->uomt ; ups = NULL ; ufctr = 1.0 ; preOffset = 0 ; postOffset = 0 ; uprefix = FALSE ;
		if (uomt != NULL)
		 { for(i=0;i<=uomt->Count;i++) { if (uomt->Entry[i].Ref == point->Value.UOMVal.Ref) break ; } ;
		   if (i >= uomt->Count)
		    { v4dpi_UOMInitialize(ctx,point->Value.UOMVal.Ref) ;
		      for(i=0;i<=uomt->Count;i++) { if (uomt->Entry[i].Ref == point->Value.UOMVal.Ref) break ; } ;
		    } ;
		   if (i >= uomt->Count)
		    { ups = UClit("") ; uomd = NULL ;
		    } else 
		    { uomd = uomt->Entry[i].uomd ;
		      if (uomd->DfltDispX == UNUSED)		/* Have to pull display from uom point */
		       { if (point->Value.UOMVal.Index < 0)	/* VEH050601 Slimy code to hack MIDAS/CDF master pack */
		          { for(i=0;i<uomd->Count;i++) { if (uomd->UEntry[i].Index < 0) break ; } ;	/* Look for Index less than 0 */
			    if (i < uomd->Count) { ups = uomd->UEntry[i].PSStr ; uprefix = uomd->UEntry[i].Prefix ; }
			     else { ups = UClit("mp") ; } ;		/* If can't find an entry then default to "mp" */
			    ufctr = -point->Value.UOMVal.Index ; 
			  }
			  else
		          { for(i=0;i<uomd->Count;i++) { if (uomd->UEntry[i].Index == point->Value.UOMVal.Index) break ; } ;
			    if (i < uomd->Count && point->Value.UOMVal.Index >= V4DPI_CaseCount_Mult)
			     { for(i=0;i<uomd->Count;i++) { if (uomd->UEntry[i].Index == 0) break ; } ;
			     } ;
		            if (i < uomd->Count)
			     { ups = uomd->UEntry[i].PSStr ; ufctr = uomd->UEntry[i].Factor ; preOffset = uomd->UEntry[i].preOffset ; postOffset = uomd->UEntry[i].postOffset ; uprefix = uomd->UEntry[i].Prefix ; } ;
		          }
		       } else
		       { ups = uomd->UEntry[uomd->DfltDispX].PSStr ; ufctr = uomd->UEntry[uomd->DfltDispX].Factor ; preOffset = uomd->UEntry[uomd->DfltDispX].preOffset ; postOffset = uomd->UEntry[uomd->DfltDispX].postOffset ;
		         uprefix = uomd->UEntry[uomd->DfltDispX].Prefix ;
		       } ;
		      if (ufctr == 0.0) { ufctr = (double)(point->Value.UOMVal.Index / V4DPI_CaseCount_Mult) ; } ;
		      if (ufctr == 0.0) { ufctr = 1.0 ; } ;
		    } ;
		 } ;
		switch (point->Grouping)
		 { default: goto no_relUOM ;
		   case V4DPI_Grouping_Single: break ;
		   case V4DPI_Grouping_GT:	UCstrcat(dststr,UClit(">")) ; break ;
		   case V4DPI_Grouping_GE:	UCstrcat(dststr,UClit(">=")) ; break ;
		   case V4DPI_Grouping_LT:	UCstrcat(dststr,UClit("<")) ; break ;
		   case V4DPI_Grouping_LE:	UCstrcat(dststr,UClit("<=")) ; break ;
		   case V4DPI_Grouping_NE:	UCstrcat(dststr,UClit("<>")) ; break ;
		 } ;
		if (point->Value.UOMVal.Ref == UNUSED) { UCstrcat(dststr,gpi->ci->li->LI[gpi->ci->li->CurX].None) ; break ; } ;
		memcpy(&d1,&point->Value.UOMVal.Num,sizeof d1) ;	/* Copy to get around possible alignment probs */
//		d1 /= ufctr ;
		d1 = ((d1 - postOffset) / ufctr) - preOffset ;
		if (uomd == NULL ? FALSE : uomd->Rounding != 9999)
		 { if (uomd->Rounding >= 0)
		   d1 = (double)(DtoI(d1 * powers[uomd->Rounding])) / powers[uomd->Rounding] ;
		 } ;
		if (uomd != NULL && ups != NULL) { if (uprefix && (uomd->Attributes & V4DPI_UOMAttr_DisplayPSfix)) UCstrcat(dststr,ups) ; } ;
		UCsprintf(tb1,UCsizeof(tb1),(di->OutFormat[0] == UClit('%') ? di->OutFormat : V4DPI_PntFormat_Real),d1) ; UCstrcat(dststr,tb1) ;
		if (uomd != NULL && ups != NULL) { if ((!uprefix) && (uomd->Attributes & V4DPI_UOMAttr_DisplayPSfix)) { UCstrcat(dststr,UClit(" ")) ; UCstrcat(dststr,ups) ; } ; } ;
		break ;
no_relUOM:
/*		Have a list or range or whatever - do it */
		if (uprefix) { if (ups) UCstrcat(dststr,ups) ; } ;
		pum = (struct V4DPI__Point_UOMMix *)&point->Value ;
		for(i=0;i<point->Grouping;i++)
		 { memcpy(&d1,&pum->Entry[i].BeginUOM.Num,sizeof(d1)) ; memcpy(&d2,&pum->Entry[i].EndUOM.Num,sizeof(d2)) ;
		   if (d1 == d2)
		    { 
//		      d1 /= ufctr ;
		      d1 = ((d1 - postOffset) / ufctr) - preOffset ;
		      if (uomd == NULL ? FALSE : uomd->Rounding != 9999)
		       { if (uomd->Rounding >= 0)
		          d1 = (double)(DtoI(d1 * powers[uomd->Rounding])) / powers[uomd->Rounding] ;
		       } ;
		      UCsprintf(tb1,UCsizeof(tb1),(di->OutFormat[0] == UClit('%') ? di->OutFormat : V4DPI_PntFormat_Real),d1) ;
		    } else
		    { 
		      d1 /= ufctr ; d2 /= ufctr ;
		      d1 = ((d1 - postOffset) / ufctr) - preOffset ; d2 = ((d2 - postOffset) / ufctr) - preOffset ;
		      if (uomd == NULL ? FALSE : uomd->Rounding != 9999)
		       { if (uomd->Rounding >= 0)
		          { d1 = (double)(DtoI(d1 * powers[uomd->Rounding])) / powers[uomd->Rounding] ;
		            d2 = (double)(DtoI(d2 * powers[uomd->Rounding])) / powers[uomd->Rounding] ;
			  } ;
		       } ;
		      UCsprintf(tb1,UCsizeof(tb1),UClit("%.15g..%.15g"),d1,d2) ;
		    } ;
		   UCstrcat(dststr,tb1) ; if (i < point->Grouping-1) UCstrcat(dststr,UClit(",")) ;
		 } ;
		if (!uprefix) { if (ups) { UCstrcat(dststr,UClit(" ") ) ; UCstrcat(dststr,ups) ; } ; } ;
		break ;
	   case V4DPI_PntType_UOMPUOM:
		if (point->Value.UOMPUOMVal.UOM.Ref == UNUSED && point->Value.UOMPUOMVal.PUOM.Ref == UNUSED)
		 { UCstrcatToASC(dststr,gpi->ci->li->LI[gpi->ci->li->CurX].None) ; break ; } ;
		ZPH(&tpnt) ; tpnt.Dim = point->Dim ; tpnt.PntType = V4DPI_PntType_UOM ; tpnt.Bytes = V4PS_UOM ;
		tpnt.Value.UOMVal = point->Value.UOMPUOMVal.UOM ;
		v4dpi_PointToStringML(tb1,&tpnt,ctx,format,maxlen) ;	/* Format first UOM */
		UCstrcat(dststr,UClit("(")) ; UCstrcat(dststr,tb1) ; UCstrcat(dststr,UClit("/")) ;
		tpnt.Value.UOMVal = point->Value.UOMPUOMVal.PUOM ;
		v4dpi_PointToStringML(tb1,&tpnt,ctx,format,maxlen) ;	/* Format second UOM */
		UCstrcat(dststr,tb1) ; UCstrcat(dststr,UClit(")")) ;
		break ;
	   case V4DPI_PntType_UOMPer:
		if (gpi->uomt == NULL) v4dpi_UOMInitialize(ctx,UNUSED) ;	/* Attempt to init if necessary */
		uomt = gpi->uomt ; ups = NULL ; ufctr = 1.0 ; preOffset = 0 ; postOffset = 0 ; uprefix = FALSE ;
		if (uomt != NULL)
		 { for(i=0;i<=uomt->Count;i++) { if (uomt->Entry[i].Ref == point->Value.UOMPerVal.Ref) break ; } ;
		   if (i >= uomt->Count)
		    { v4dpi_UOMInitialize(ctx,point->Value.UOMVal.Ref) ;
		      for(i=0;i<=uomt->Count;i++) { if (uomt->Entry[i].Ref == point->Value.UOMVal.Ref) break ; } ;
		    } ;
		   if (i >= uomt->Count)
		    { ufctr = 1.0 ; uprefix = FALSE ; ups = UClit("") ;
		    } else 
		    { uomd = uomt->Entry[i].uomd ;
		      if (uomd->DfltDispX == UNUSED)		/* Have to pull display from uom point */
		       { if (point->Value.UOMVal.Index < 0)	/* VEH050601 Slimy code to hack MIDAS/CDF master pack */
		          { for(i=0;i<uomd->Count;i++) { if (uomd->UEntry[i].Index < 0) break ; } ;	/* Look for Index less than 0 */
			    if (i < uomd->Count) { ups = uomd->UEntry[i].PSStr ; uprefix = uomd->UEntry[i].Prefix ; }
			     else { ups = UClit("mp") ; } ;		/* If can't find an entry then default to "mp" */
			    ufctr = -point->Value.UOMVal.Index ; 
			  } else
		          { for(i=0;i<uomd->Count;i++) { if (point->Value.UOMVal.Index == uomd->UEntry[i].Index) break ; } ;	/* Look for Index less than 0 */
			    if (i < uomd->Count) { ups = uomd->UEntry[i].PSStr ; uprefix = uomd->UEntry[i].Prefix ; }
			  } ;
		       } else
		       { ups = uomd->UEntry[uomd->DfltDispX].PSStr ; ufctr = uomd->UEntry[uomd->DfltDispX].Factor ; preOffset = uomd->UEntry[uomd->DfltDispX].preOffset ; postOffset = uomd->UEntry[uomd->DfltDispX].postOffset ;
		         uprefix = uomd->UEntry[uomd->DfltDispX].Prefix ;
		       } ;
		      if (ufctr == 0.0) { ufctr = (double)(point->Value.UOMVal.Index / V4DPI_CaseCount_Mult) ; } ;
		      if (ufctr == 0.0) { ufctr = 1.0 ; } ;
		    } ;
		 } ;
		switch (point->Grouping)
		 { default: goto no_relUOMPer ;
		   case V4DPI_Grouping_Single: break ;
		   case V4DPI_Grouping_GT:	UCstrcat(dststr,UClit(">")) ; break ;
		   case V4DPI_Grouping_GE:	UCstrcat(dststr,UClit(">=")) ; break ;
		   case V4DPI_Grouping_LT:	UCstrcat(dststr,UClit("<")) ; break ;
		   case V4DPI_Grouping_LE:	UCstrcat(dststr,UClit("<=")) ; break ;
		   case V4DPI_Grouping_NE:	UCstrcat(dststr,UClit("<>")) ; break ;
		 } ;
		if (point->Value.UOMPerVal.Ref == UNUSED) { UCstrcat(dststr,gpi->ci->li->LI[gpi->ci->li->CurX].None) ; break ; } ;
		memcpy(&d1,&point->Value.UOMPerVal.Amount,sizeof d1) ;	/* Copy to get around possible alignment probs */
		UCsprintf(tb1,UCsizeof(tb1),(di->OutFormat[0] == UClit('%') ? di->OutFormat : V4DPI_PntFormat_Real),d1) ; UCstrcat(dststr,tb1) ; UCstrcat(dststr,UClit("/")) ;
		if (uprefix) { if (ups) UCstrcat(dststr,ups) ; } ;
		memcpy(&d1,&point->Value.UOMPerVal.Num,sizeof d1) ;	/* Copy to get around possible alignment probs */
//		UCsprintf(tb1,UCsizeof(tb1),(di->OutFormat[0] == UClit('%') ? di->OutFormat : V4DPI_PntFormat_Real),d1/ufctr) ; UCstrcat(dststr,tb1) ;
		UCsprintf(tb1,UCsizeof(tb1),(di->OutFormat[0] == UClit('%') ? di->OutFormat : V4DPI_PntFormat_Real),((d1 - postOffset)/ufctr) - preOffset) ; UCstrcat(dststr,tb1) ;
		if (!uprefix) { if (ups) UCstrcat(dststr,ups) ; } ;
		break ;
no_relUOMPer:
/*		Have a list or range or whatever - do it */
		pupm = (struct V4DPI__Point_UOMPerMix *)&point->Value ;
		for(i=0;i<point->Grouping;i++)
		 { memcpy(&d1,&pupm->Entry[i].BeginUOMPer.Num,sizeof(d1)) ; memcpy(&d2,&pupm->Entry[i].EndUOMPer.Num,sizeof(d2)) ;
		   if (d1 == d2)
		    { UCsprintf(tb1,UCsizeof(tb1),(di->OutFormat[0] == UClit('%') ? di->OutFormat : V4DPI_PntFormat_Real),d1) ; }
		    else { UCsprintf(tb1,UCsizeof(tb1),UClit("%.15g..%.15g"),d1,d2) ; } ;
		   UCstrcat(dststr,tb1) ; if (i < point->Grouping-1) UCstrcat(dststr,UClit(",")) ;
		 } ;
		UCstrcat(dststr,UClit("/")) ;
		if (uprefix) { if (ups) UCstrcat(dststr,ups) ; } ;
		memcpy(&d1,&point->Value.UOMPerVal.Num,sizeof d1) ;	/* Copy to get around possible alignment probs */
//		UCsprintf(tb1,UCsizeof(tb1),(di->OutFormat[0] == UClit('%') ? di->OutFormat : V4DPI_PntFormat_Real),d1/ufctr) ; UCstrcat(dststr,tb1) ;
		UCsprintf(tb1,UCsizeof(tb1),(di->OutFormat[0] == UClit('%') ? di->OutFormat : V4DPI_PntFormat_Real),((d1 - postOffset)/ufctr) - preOffset) ; UCstrcat(dststr,tb1) ;
		if (!uprefix) { if (ups) UCstrcat(dststr,ups) ; } ;
		break ;
	   case V4DPI_PntType_Fixed:
		switch (point->Grouping)
		 { default: goto no_relFix ;
		   case V4DPI_Grouping_Single: break ;
		   case V4DPI_Grouping_GT:	UCstrcat(dststr,UClit(">")) ; break ;
		   case V4DPI_Grouping_GE:	UCstrcat(dststr,UClit(">=")) ; break ;
		   case V4DPI_Grouping_LT:	UCstrcat(dststr,UClit("<")) ; break ;
		   case V4DPI_Grouping_LE:	UCstrcat(dststr,UClit("<=")) ; break ;
		   case V4DPI_Grouping_NE:	UCstrcat(dststr,UClit("<>")) ; break ;
		 } ;
		memcpy(&b64,&point->Value.FixVal,sizeof b64) ;	/* Copy to get around possible alignment probs */
		v_FormatLInt(b64,point->LHSCnt,UClit("0!"),tb1) ; UCstrcat(dststr,tb1) ;
		break ;
no_relFix:
/*		Have a list or range or whatever - do it */
		pfm = (struct V4DPI__Point_FixMix *)&point->Value ;
		for(i=0;i<point->Grouping;i++)
		 { memcpy(&b641,&pfm->Entry[i].BeginFix,sizeof(B64INT)) ; memcpy(&b642,&pfm->Entry[i].EndFix,sizeof(B64INT)) ;
		   if (b641 == b642)
		    { v_FormatLInt(b641,point->LHSCnt,UClit("!"),tb1) ;
		    }
		    else { v_FormatLInt(b641,point->LHSCnt,UClit("!"),tb1) ; UCstrcat(dststr,tb1) ; UCstrcat(dststr,UClit("..")) ;
			   v_FormatLInt(b642,point->LHSCnt,UClit("!"),tb1) ;
			 } ;
		   UCstrcat(dststr,tb1) ; if (i < point->Grouping-1) UCstrcat(dststr,UClit(",")) ;
		 } ;
		break ;
	   case V4DPI_PntType_Real:
		switch (point->Grouping)
		 { default: goto no_relReal ;
		   case V4DPI_Grouping_Single: break ;
		   case V4DPI_Grouping_GT:	UCstrcat(dststr,UClit(">")) ; break ;
		   case V4DPI_Grouping_GE:	UCstrcat(dststr,UClit(">=")) ; break ;
		   case V4DPI_Grouping_LT:	UCstrcat(dststr,UClit("<")) ; break ;
		   case V4DPI_Grouping_LE:	UCstrcat(dststr,UClit("<=")) ; break ;
		   case V4DPI_Grouping_NE:	UCstrcat(dststr,UClit("<>")) ; break ;
		 } ;
		GETREAL(d1,point) ;	/* Copy to get around possible alignment probs */
		if (di->Decimals == 0) { UCsprintf(tb1,UCsizeof(tb1),(di->OutFormat[0] == UClit('%') ? di->OutFormat : V4DPI_PntFormat_Real),d1) ; }
		 else { UCsprintf(tb1,UCsizeof(tb1),UClit("%.*f"),di->Decimals,d1) ; } ;
		UCstrcat(dststr,tb1) ;
		break ;
no_relReal:
/*		Have a list or range or whatever - do it */
		prm = (struct V4DPI__Point_RealMix *)&point->Value ;
		for(i=0;i<point->Grouping;i++)
		 { memcpy(&d1,&prm->Entry[i].BeginReal,SIZEofDOUBLE) ; memcpy(&d2,&prm->Entry[i].EndReal,SIZEofDOUBLE) ;
		   if (d1 == d2)
		    { if (di->Decimals == 0) { UCsprintf(tb1,UCsizeof(tb1),(di->OutFormat[0] == UClit('%') ? di->OutFormat : V4DPI_PntFormat_Real),d1) ; }
		       else { UCsprintf(tb1,UCsizeof(tb1),UClit("%.*f"),di->Decimals,d1) ; } ;
		    } else
		    { if (di->Decimals == 0) { UCsprintf(tb1,UCsizeof(tb1),(di->OutFormat[0] == UClit('%') ? di->OutFormat : V4DPI_PntFormat_Real),d1) ; UCsprintf(tb2,UCsizeof(tb1),V4DPI_PntFormat_Real,d2) ; }
		       else { UCsprintf(tb1,UCsizeof(tb1),UClit("%.*f"),di->Decimals,d1) ; UCsprintf(tb2,UCsizeof(tb1),UClit("%.*f"),di->Decimals,d2) ; } ;
		      UCstrcat(tb1,UClit("..")) ; UCstrcat(tb1,tb2) ;
		    } ;
		   UCstrcat(dststr,tb1) ; if (i < point->Grouping-1) UCstrcat(dststr,UClit(",")) ;
		 } ;
		break ;
	   case V4DPI_PntType_SSVal:
		if (point->Bytes > V4PS_Int) { UCstrcat(dststr,UClit("*format*")) ; break ; } ;
		switch (point->Value.IntVal)
		 { default:			UCstrcpy(tb1,UClit("??")) ; break ;
		    case V4SS_SSVal_Null:	UCstrcpy(tb1,UClit("NULL")) ; break ;
		    case V4SS_SSVal_Div0:	UCstrcpy(tb1,UClit("Div0")) ; break ;
		    case V4SS_SSVal_Value:	UCstrcpy(tb1,UClit("Value")) ; break ;
		    case V4SS_SSVal_Ref:	UCstrcpy(tb1,UClit("Ref")) ; break ;
		    case V4SS_SSVal_Name:	UCstrcpy(tb1,UClit("Name")) ; break ;
		    case V4SS_SSVal_Num:	UCstrcpy(tb1,UClit("Num")) ; break ;
		    case V4SS_SSVal_NA: 	UCstrcpy(tb1,UClit("NA")) ; break ;
		    case V4SS_SSVal_Empty:	UCstrcpy(tb1,UClit("Empty")) ; break ;
		    case V4SS_SSVal_Row:	UCstrcpy(tb1,UClit("Row")) ; break ;
		    case V4SS_SSVal_Column:	UCstrcpy(tb1,UClit("Column")) ; break ;
		 } ;
		UCstrcat(dststr,tb1) ; break ;
	   case V4DPI_PntType_Color:
		if (point->Grouping == V4DPI_Grouping_Single)
		 { UCstrcat(dststr,v_ColorRefToName(point->Value.IntVal)) ;
		   break ;
		 } ;
/*		Have a list or range or whatever - do it */
		switch (point->Grouping)
		 { default: goto no_relColor ;
		   case V4DPI_Grouping_Single:	break ;
		   case V4DPI_Grouping_GT:	UCstrcat(dststr,UClit(">")) ; break ;
		   case V4DPI_Grouping_GE:	UCstrcat(dststr,UClit(">=")) ; break ;
		   case V4DPI_Grouping_LT:	UCstrcat(dststr,UClit("<")) ; break ;
		   case V4DPI_Grouping_LE:	UCstrcat(dststr,UClit("<=")) ; break ;
		   case V4DPI_Grouping_NE:	UCstrcat(dststr,UClit("<>")) ; break ;
		 } ;
		UCstrcat(dststr,v_ColorRefToName(point->Value.IntVal)) ;
		break ;
no_relColor:
		pim = (struct V4DPI__Point_IntMix *)&point->Value ;
		for(i=0;i<point->Grouping;i++)
		 { if (pim->Entry[i].BeginInt == pim->Entry[i].EndInt)
		    { UCstrcpy(tb1,v_ColorRefToName(pim->Entry[i].BeginInt)) ; }
		    else { UCstrcpy(tb2,v_ColorRefToName(pim->Entry[i].BeginInt)) ;
			   v_Msg(ctx,tb1,"@%1U..%2U",tb2,v_ColorRefToName(pim->Entry[i].EndInt)) ;
			 } ;
		   UCstrcat(dststr,tb1) ; if (i < point->Grouping-1) UCstrcat(dststr,UClit(",")) ;
		 } ;
		 break ;
	   case V4DPI_PntType_Tree:
		UCsprintf(tb1,UCsizeof(tb1),UClit("%d-%d"),TREEID(point->Value.TreeNodeVal),TREENODEID(point->Value.TreeNodeVal)) ;
		UCstrcat(dststr,tb1) ; break ;
	   case V4DPI_PntType_Country:
		if (point->Grouping == V4DPI_Grouping_Single)
		 { UCstrcat(dststr,v_CountryRefToName(point->Value.IntVal)) ;
		   break ;
		 } ;
/*		Have a list or range or whatever - do it */
		switch (point->Grouping)
		 { default: goto no_relCountry ;
		   case V4DPI_Grouping_Single:	break ;
		   case V4DPI_Grouping_GT:	UCstrcat(dststr,UClit(">")) ; break ;
		   case V4DPI_Grouping_GE:	UCstrcat(dststr,UClit(">=")) ; break ;
		   case V4DPI_Grouping_LT:	UCstrcat(dststr,UClit("<")) ; break ;
		   case V4DPI_Grouping_LE:	UCstrcat(dststr,UClit("<=")) ; break ;
		   case V4DPI_Grouping_NE:	UCstrcat(dststr,UClit("<>")) ; break ;
		 } ;
		UCstrcat(dststr,v_CountryRefToName(point->Value.IntVal)) ;
		break ;
no_relCountry:
		pim = (struct V4DPI__Point_IntMix *)&point->Value ;
		for(i=0;i<point->Grouping;i++)
		 { if (pim->Entry[i].BeginInt == pim->Entry[i].EndInt)
		    { UCstrcpy(tb1,v_CountryRefToName(pim->Entry[i].BeginInt)) ; }
		    else { UCstrcpy(tb2,v_CountryRefToName(pim->Entry[i].BeginInt)) ;
			   v_Msg(ctx,tb1,"@%1U..%2U",tb2,v_CountryRefToName(pim->Entry[i].EndInt)) ;
			 } ;
		   UCstrcat(dststr,tb1) ; if (i < point->Grouping-1) UCstrcat(dststr,UClit(",")) ;
		 } ;
		 break ;
	   case V4DPI_PntType_XDict:
		if (point->Grouping == V4DPI_Grouping_Single)
		 { UCstrcat(dststr,v4dpi_RevXDictEntryGet(ctx,point->Dim,point->Value.IntVal)) ;
		   break ;
		 } ;
/*		Have a list or range or whatever - do it */
		switch (point->Grouping)
		 { default: goto no_relXDict ;
		   case V4DPI_Grouping_Single:	break ;
		   case V4DPI_Grouping_GT:	UCstrcat(dststr,UClit(">")) ; break ;
		   case V4DPI_Grouping_GE:	UCstrcat(dststr,UClit(">=")) ; break ;
		   case V4DPI_Grouping_LT:	UCstrcat(dststr,UClit("<")) ; break ;
		   case V4DPI_Grouping_LE:	UCstrcat(dststr,UClit("<=")) ; break ;
		   case V4DPI_Grouping_NE:	UCstrcat(dststr,UClit("<>")) ; break ;
		 } ;
		UCstrcat(dststr,v4dpi_RevXDictEntryGet(ctx,point->Dim,point->Value.IntVal)) ;
		break ;
no_relXDict:
		pim = (struct V4DPI__Point_IntMix *)&point->Value ;
		for(i=0;i<point->Grouping;i++)
		 { if (pim->Entry[i].BeginInt == pim->Entry[i].EndInt)
		    { UCstrcpy(tb1,v4dpi_RevXDictEntryGet(ctx,point->Dim,pim->Entry[i].BeginInt)) ; }
		    else { UCstrcpy(tb2,v4dpi_RevXDictEntryGet(ctx,point->Dim,pim->Entry[i].BeginInt)) ;
			   v_Msg(ctx,tb1,"@%1U..%2U",tb2,v4dpi_RevXDictEntryGet(ctx,point->Dim,pim->Entry[i].EndInt)) ;
			 } ;
		   UCstrcat(dststr,tb1) ; if (i < point->Grouping-1) UCstrcat(dststr,UClit(",")) ;
		 } ;
		 break ;
	   case V4DPI_PntType_Dict:
		if (point->Grouping == V4DPI_Grouping_Single)
		 { UCCHAR *b ;
		   if (UCstrcmp(dststr,UClit("NID:")) == 0) ZUS(dststr) ;
/*		   Try to decode, if fails then try for External dimension entry */
		   b = v4dpi_RevDictEntryGet(ctx,point->Value.IntVal) ;
		   if (b[0] == UClit('?') && gpi->xdr != NULL) b = v4dpi_RevXDictEntryGet(ctx,point->Dim,point->Value.IntVal) ;
		   UCstrcat(dststr,b) ;
		   break ;
		 } ;
/*		Have a list or range or whatever - do it */
		switch (point->Grouping)
		 { default: goto no_relDict ;
		   case V4DPI_Grouping_Single:
			if (UCstrcmp(dststr,UClit("NID:")) == 0) ZUS(dststr) ; break ;
		   case V4DPI_Grouping_GT:	UCstrcat(dststr,UClit(">")) ; break ;
		   case V4DPI_Grouping_GE:	UCstrcat(dststr,UClit(">=")) ; break ;
		   case V4DPI_Grouping_LT:	UCstrcat(dststr,UClit("<")) ; break ;
		   case V4DPI_Grouping_LE:	UCstrcat(dststr,UClit("<=")) ; break ;
		   case V4DPI_Grouping_NE:	UCstrcat(dststr,UClit("<>")) ; break ;
		 } ;
		UCstrcat(dststr,v4dpi_RevDictEntryGet(ctx,point->Value.IntVal)) ;
		break ;
no_relDict:
		pim = (struct V4DPI__Point_IntMix *)&point->Value ;
		for(i=0;i<point->Grouping;i++)
		 { if (pim->Entry[i].BeginInt == pim->Entry[i].EndInt)
		    { UCstrcpy(tb1,v4dpi_RevDictEntryGet(ctx,pim->Entry[i].BeginInt)) ;
		      if (tb1[0] == UClit('?') && gpi->xdr != NULL) UCstrcpy(tb1,v4dpi_RevXDictEntryGet(ctx,point->Dim,pim->Entry[i].BeginInt)) ;
		    }
		    else { UCstrcpy(tb2,v4dpi_RevDictEntryGet(ctx,pim->Entry[i].BeginInt)) ;
			   v_Msg(ctx,tb1,"@%1U..%2U",tb2,v4dpi_RevDictEntryGet(ctx,pim->Entry[i].EndInt)) ;
			 } ;
		   UCstrcat(dststr,tb1) ; if (i < point->Grouping-1) UCstrcat(dststr,UClit(",")) ;
		 } ;
		 break ;
//	   case V4DPI_PntType_PIntMod:
	   case V4DPI_PntType_IntMod:
		UCstrcpy(tb1,v4im_Display(point->Value.IntVal)) ; UCstrcat(dststr,tb1) ;
		break ;
	   case V4DPI_PntType_BinObj:
		UCsprintf(tb1,UCsizeof(tb1),UClit("(%d byte object)"),point->Bytes-V4DPI_PointHdr_Bytes) ; UCstrcat(dststr,tb1) ;
		break ;
	   case V4DPI_PntType_UCChar:
		if (point->Grouping == V4DPI_Grouping_Single)
		 { len = UCCHARSTRLEN(point) ; if (len > UCsizeof(tb1)) len = UCsizeof(tb1) - 100 ;
//		   len = point->Value.UCVal[0] ;
		   for(i=1,j=0,nq=FALSE;i<=len;i++)
		    { switch (point->Value.UCVal[i])
		       { default:	tb1[j++] = point->Value.UCVal[i] ;
					if (point->Value.UCVal[i] >= UClit('a') && point->Value.UCVal[i] <= UClit('z')) nq = TRUE ;
					break ;
			 case UClit('\t'):
			 case UClit('.'):
			 case UClit(','):
			 case UClit(' '):	nq = TRUE ; tb1[j++] = point->Value.UCVal[i] ; break ;
			 case UClit('"'):	tb1[j++] = point->Value.UCVal[i] ; break ;
		       } ;
		    } ; tb1[j] = 0 ;
		   if (!(format & V4DPI_FormatOpt_ShowDim)) nq = FALSE ;
		   if (format & (V4DPI_FormatOpt_AlphaQuote | V4DPI_FormatOpt_Point)) nq = TRUE ;
		   if (nq) { v_StringLit(tb1,&dststr[UCstrlen(dststr)],-maxlen,'"',0) ; }
		    else { if (UCstrlen(dststr) + len < maxlen) { UCstrcat(dststr,tb1) ; }
			    else { UCstrncat(dststr,tb1,maxlen-(UCstrlen(dststr) + len + 1)) ; dststr[maxlen-1] = UCEOS ; } ;
			 } ;
		   break ;
		 } ;
		if (!(format & V4DPI_FormatOpt_ShowDim)) nq = FALSE ;
		if (format & (V4DPI_FormatOpt_AlphaQuote | V4DPI_FormatOpt_Point)) nq = TRUE ;
		pam = (struct V4DPI__Point_AlphaMix *)&point->Value ; ubp = (UCCHAR *)&pam->Entry[point->Grouping].BeginIndex ;
		for(i=0;i<point->Grouping;i++)
		 { 
		   if (pam->Entry[i].EndIndex == 0)
		    { UCstrncpy(tb1,ubp+pam->Entry[i].BeginIndex+1,*(ubp+pam->Entry[i].BeginIndex)) ; tb1[*(ubp+pam->Entry[i].BeginIndex)] = UCEOS ;
		      if (nq) { v_StringLit(tb1,&dststr[UCstrlen(dststr)],-maxlen,UClit('"'),0) ; }
		       else { UCstrcat(dststr,tb1) ; } ;
		    } else
		    { UCstrncpy(tb1,ubp+pam->Entry[i].BeginIndex+1,*(ubp+pam->Entry[i].BeginIndex)) ; tb1[*(ubp+pam->Entry[i].BeginIndex)] = UCEOS ;
		      if (nq) { v_StringLit(tb1,&dststr[UCstrlen(dststr)],-maxlen,UClit('"'),0) ; } else { UCstrcat(dststr,tb1) ; } ;
		      UCstrcat(dststr,UClit("..")) ;
		      UCstrncpy(tb1,ubp+pam->Entry[i].EndIndex+1,*(ubp+pam->Entry[i].EndIndex)) ; tb1[*(ubp+pam->Entry[i].EndIndex)] = UCEOS ;
		      if (nq) { v_StringLit(tb1,&dststr[UCstrlen(dststr)],-maxlen,UClit('"'),0) ; } else {  UCstrcat(dststr,tb1) ; } ;
		    } ; if (i < point->Grouping-1) UCstrcat(dststr,UClit(",")) ;
		 } ;
		break ;
	   case V4DPI_PntType_RegExpPattern:
	   CASEofCharmU
		switch (point->Grouping)
		 { default:			goto alpha_mult ;
		   case V4DPI_Grouping_Single:	break ;
		   case V4DPI_Grouping_GT:	UCstrcat(dststr,UClit(">")) ; break ;
		   case V4DPI_Grouping_GE:	UCstrcat(dststr,UClit(">=")) ; break ;
		   case V4DPI_Grouping_LT:	UCstrcat(dststr,UClit("<")) ; break ;
		   case V4DPI_Grouping_LE:	UCstrcat(dststr,UClit("<=")) ; break ;
		   case V4DPI_Grouping_NE:	UCstrcat(dststr,UClit("<>")) ; break ;
		 } ;
		if (TRUE)
		 { len = CHARSTRLEN(point) ;
		   for(i=1,j=0,nq=FALSE;i<=len;i++)		/* Handle both types of strings- count & asciz */
		    { switch (point->Value.AlphaVal[i])
		       { default:	tb1[j++] = point->Value.AlphaVal[i] ;
					if (point->Value.AlphaVal[i] >= 'a' && point->Value.AlphaVal[i] <= 'z') nq = TRUE ;
					break ;
			 case UClit('\t'):
			 case UClit('.'):
			 case UClit(','):
			 case UClit(' '):	nq = TRUE ; tb1[j++] = point->Value.AlphaVal[i] ; break ;
			 case UClit('"'):	/* tb1[j++] = '\\' ; */ tb1[j++] = point->Value.AlphaVal[i] ; break ;
		       } ;
		    } ; tb1[j] = 0 ;
		   if (!(format & V4DPI_FormatOpt_ShowDim)) nq = FALSE ;
		   if (format & (V4DPI_FormatOpt_AlphaQuote | V4DPI_FormatOpt_Point)) nq = TRUE ;
		   if (nq) { v_StringLit(tb1,&dststr[UCstrlen(dststr)],-maxlen,'"',0) ; }
		    else { UCstrcat(dststr,tb1) ; } ;
		   break ;
		 } ;
alpha_mult:
		if (!(format & V4DPI_FormatOpt_ShowDim)) nq = FALSE ;
		if (format & (V4DPI_FormatOpt_AlphaQuote | V4DPI_FormatOpt_Point)) nq = TRUE ;
		pam = (struct V4DPI__Point_AlphaMix *)&point->Value ; bp = (char *)&pam->Entry[point->Grouping].BeginIndex ;
		for(i=0;i<point->Grouping;i++)
		 { char abuf[V4DPI_AlphaVal_Max*2] ;
		   if (pam->Entry[i].EndIndex == 0)
		    { strncpy(abuf,bp+pam->Entry[i].BeginIndex+1,*(bp+pam->Entry[i].BeginIndex)) ; abuf[*(bp+pam->Entry[i].BeginIndex)] = 0 ;
		      UCstrcpyAtoU(tb1,abuf) ;
		      if (nq) { v_StringLit(tb1,&dststr[UCstrlen(dststr)],-maxlen,UClit('"'),0) ; }
		       else { UCstrcat(dststr,tb1) ; } ;
		    } else
		    { strncpy(abuf,bp+pam->Entry[i].BeginIndex+1,*(bp+pam->Entry[i].BeginIndex)) ; abuf[*(bp+pam->Entry[i].BeginIndex)] = 0 ;
		      UCstrcpyAtoU(tb1,abuf) ;
		      if (nq) { v_StringLit(tb1,&dststr[UCstrlen(dststr)],-maxlen,UClit('"'),0) ; } else { UCstrcat(dststr,tb1) ; } ;
		      UCstrcat(dststr,UClit("..")) ;
		      strncpy(abuf,bp+pam->Entry[i].EndIndex+1,*(bp+pam->Entry[i].EndIndex)) ; abuf[*(bp+pam->Entry[i].EndIndex)] = 0 ;
		      UCstrcpyAtoU(tb1,abuf) ;
		      if (nq) { v_StringLit(tb1,&dststr[UCstrlen(dststr)],-maxlen,UClit('"'),0) ; } else {  UCstrcat(dststr,tb1) ; } ;
		    } ; if (i < point->Grouping-1) UCstrcat(dststr,UClit(",")) ;
		 } ;
		break ;
	   case V4DPI_PntType_SegBitMap:
		lp = (struct V4L__ListPoint *)v4im_VerifyList(&tpnt,ctx,point,0) ;
		goto list_entry ;		
	   case V4DPI_PntType_List:
		lp = ALIGNLP(&point->Value) ;
list_entry:
		ls = (((format & V4DPI_FormatOpt_ListNL) == 0) ? UClit(" ") : UClit("\n")) ;
		switch (lp->ListType)
		 { default: v4_error(V4E_INVLISTTYPE,0,"V4DPI","PointtoString","INVLISTTYPE","Unknown list type (%d)",lp->ListType) ;
		   case V4L_ListType_Point:
			if (format & V4DPI_FormatOpt_ListInParens) UCstrcat(dststr,UClit("(")) ;
			for(fx=1;v4l_ListPoint_Value(ctx,lp,fx,&tpnt) > 0;fx++)
			 { 
			   if (fx > 1) UCstrcat(dststr,ls) ;
			   v4dpi_PointToStringML(tb1,&tpnt,ctx,format,UCsizeof(tb1)) ;
			   if (UCstrlen(tb1) + UCstrlen(dststr) > maxlen)
			    { UCstrcat(dststr,UClit(" ... ")) ; break ; } ;
			   UCstrcat(dststr,tb1) ;
			 } ; if (format & V4DPI_FormatOpt_ListInParens) UCstrcat(dststr,UClit(")")) ;
			break ;
		   case V4L_ListType_CmpndRange:
			if (format & V4DPI_FormatOpt_ListInParens) UCstrcat(dststr,UClit("(")) ;
			lcr = (struct V4L__ListCmpndRange *)&lp->Buffer[0] ;
			di1 = v4dpi_DimInfoGet(ctx,lp->Dim) ; UCstrcat(dststr,di1->DimName) ; UCstrcat(dststr,UClit(":")) ;
			for(fx=0;;fx++)
			 { if (lcr->Cmpnd[fx].Increment != 1)
			    { UCsprintf(tb1,UCsizeof(tb1),UClit("%d..%d..%d"),lcr->Cmpnd[fx].Begin,lcr->Cmpnd[fx].End,lcr->Cmpnd[fx].Increment) ; }
			    else if (lcr->Cmpnd[fx].Begin == lcr->Cmpnd[fx].End)
			    { UCsprintf(tb1,UCsizeof(tb1),UClit("%d"),lcr->Cmpnd[fx].Begin) ; }
			    else { UCsprintf(tb1,UCsizeof(tb1),UClit("%d..%d"),lcr->Cmpnd[fx].Begin,lcr->Cmpnd[fx].End) ; } ;
			   UCstrcat(dststr,tb1) ;
			   if (fx+1<lcr->Count) { UCstrcat(dststr,ls) ; } else break ;
			 } ;
			if (format & V4DPI_FormatOpt_ListInParens) UCstrcat(dststr,UClit(")")) ; break ;
		   case V4L_ListType_CmpndRangeDBL:
			if (format & V4DPI_FormatOpt_ListInParens) UCstrcat(dststr,UClit("(")) ;
			di1 = v4dpi_DimInfoGet(ctx,lp->Dim) ; UCstrcat(dststr,di1->DimName) ; UCstrcat(dststr,UClit(":")) ;
			lcrd = (struct V4L__ListCmpndRangeDBL *)&lp->Buffer[0] ;
			for(fx=0;;fx++)
			 { memcpy(&d1,&lcrd->Cmpnd[fx].Begin,sizeof d1) ; memcpy(&d2,&lcrd->Cmpnd[fx].End,sizeof d2) ;
			   if(d1 == d2)
			    { UCsprintf(tb1,UCsizeof(tb1),UClit("%.15g "),d1) ; }
			    else { UCsprintf(tb1,UCsizeof(tb1),UClit("%.15g..%.15g "),d1,d2) ; } ;
			   UCstrcat(dststr,tb1) ;
			   memcpy(&d1,&lcrd->Cmpnd[fx].Increment,sizeof d1) ;
			   if (d1 != 1)
			    { *(dststr+UCstrlen(dststr)-1) = UCEOS ; UCsprintf(tb1,UCsizeof(tb1),UClit("..%.15g"),d1) ; UCstrcat(dststr,tb1) ; } ;
			   if (fx+1<lcrd->Count) { UCstrcat(dststr,ls) ; } else break ;
			 } ;
			if (format & V4DPI_FormatOpt_ListInParens) UCstrcat(dststr,UClit(")")) ; break ;
		   case V4L_ListType_HugeInt:
		   case V4L_ListType_HugeGeneric:
			if (format & V4DPI_FormatOpt_ListInParens) UCstrcat(dststr,UClit("(")) ;
			for(fx=1;v4l_ListPoint_Value(ctx,lp,fx,&tpnt) > 0 && fx <= 20;fx++)
			 { if (fx > 1) UCstrcat(dststr,UClit(" ")) ;
			   v4dpi_PointToString(tb1,&tpnt,ctx,format) ;
			   if (UCstrlen(tb1) + UCstrlen(dststr) > maxlen)
			    { UCstrcat(dststr,UClit(" ... ")) ; break ; } ;
			   UCstrcat(dststr,tb1) ;
			 } ;
			if (lp->Entries > 20) { v_Msg(ctx,tb1,"DPIDspBigList",lp->Entries-20) ; UCstrcat(dststr,tb1) ; } ;
			if (format & V4DPI_FormatOpt_ListInParens) UCstrcat(dststr,UClit(")")) ;
			break ;
		   case V4L_ListType_Drawer:
			v_Msg(ctx,tb1,"@(%1S %2d)",DE(Drawer),lp->Entries) ; UCstrcat(dststr,tb1) ; break ;
		   case V4L_ListType_JSON:
			{ struct V4L__ListJSON *ljson = (struct V4L__ListJSON *)&lp->Buffer ;
			  v_Msg(ctx,tb1,(ljson->vjval.jvType == VJSON_Type_Object ? "@JSON object" : "@JSON array")) ; UCstrcat(dststr,tb1) ;
			} break ;
		   case V4L_ListType_Isct:
			if (format & V4DPI_FormatOpt_ListInParens) UCstrcat(dststr,UClit("(")) ;
			lisct = (struct V4L__ListIsct *)&lp->Buffer[0] ;
			ipnt = (P *)v4dpi_PntIdx_CvtIdxPtr(lisct->pix) ;
			v4dpi_PointToString(tb1,ipnt,ctx,format) ; UCstrcat(dststr,tb1) ;
			if (format & V4DPI_FormatOpt_ListInParens) UCstrcat(dststr,UClit(" )")) ;
			break ;
		   case V4L_ListType_XDBGet:
			v_Msg(ctx,tb1,"@(%1S)",DE(XDB)) ; UCstrcat(dststr,tb1) ; break ;
		   case V4L_ListType_V4IS:
			v_Msg(ctx,tb1,"@(%1S)",DE(V4IS)) ; UCstrcat(dststr,tb1) ; break ;
		   case V4L_ListType_Lazy:
			v_Msg(ctx,tb1,"@(%1S)",DE(LazyList)) ; UCstrcat(dststr,tb1) ; break ;
		   case V4L_ListType_BitMap:
			di = v4dpi_DimInfoGet(ctx,lp->Dim) ;
			v_Msg(ctx,tb1,"@(%1S %2D:%3D, %4d %5S)",DE(BitMap),Dim_Dim,di->DimId,v4l_BitMapCount(ctx,NULL,lp),DE(Entries)) ;
			UCstrcat(dststr,tb1) ; break ;
		   case V4L_ListType_TextTable:
			v_Msg(ctx,tb1,"@(%1S)",DE(TextTable)) ; UCstrcat(dststr,tb1) ; break ;
		   case V4L_ListType_Token:
			v_Msg(ctx,tb1,"@(%1S)",DE(Token)) ; UCstrcat(dststr,tb1) ; break ;
		   case V4L_ListType_XMLTokens:
			v_Msg(ctx,tb1,"@(%1S %2S)",DE(XML),DE(Token)) ; UCstrcat(dststr,tb1) ; break ;
		   case V4L_ListType_HTMLTokens:
			v_Msg(ctx,tb1,"@(%1S %2S)",DE(HTML),DE(Token)) ; UCstrcat(dststr,tb1) ; break ;
		   case V4L_ListType_JavaTokens:
			v_Msg(ctx,tb1,"@(%1S %2S)",DE(Java),DE(Token)) ; UCstrcat(dststr,tb1) ; break ;
		   case V4L_ListType_BigText:			
			v_Msg(ctx,tb1,"@(%1S %2S)",DE(BigText),DE(Line)) ; UCstrcat(dststr,tb1) ; break ;
		   case V4L_ListType_MultiAppend:
			vlma = (struct V4L__ListMultiAppend *)&lp->Buffer ;
			v_Msg(ctx,tb1,"@(%1d %2S %3d %4S)",vlma->Count,DE(Parts),vlma->Entries,DE(Entries)) ; UCstrcat(dststr,tb1) ; break ;
		   case V4L_ListType_MDArray:
			lmda = (struct V4L__ListMDArray *)&lp->Buffer ;
			v_Msg(ctx,tb1,"@(%1S %2d %3S, %4d %5S)",DE(Array),lmda->MDDimCount,DE(Dimensions),lp->Entries,DE(Entries)) ; UCstrcat(dststr,tb1) ; break ;
#ifdef WANTHTMLMODULE
		   case V4L_ListType_HTMLTable:
			lht = (struct V4L__ListHTMLTable *)&lp->Buffer ;
			if (lht->NumRows != UNUSED) { UCsprintf(tb1,UCsizeof(tb1),UClit("(HTMLTable Rows, %d entries)"),lht->NumRows) ; }
			 else { UCsprintf(tb1,UCsizeof(tb1),UClit("(HTMLTable Columns, %d entries)"),lht->NumColumns) ; } ;
			UCstrcat(dststr,tb1) ; break ;
#endif
		   case V4L_ListType_Files:
			lf = (struct V4L__ListFiles *)&lp->Buffer ;
			v_Msg(ctx,tb1,"ListTypeFile",lf->FileSpec) ;
			UCstrcat(dststr,tb1) ; break ;
		   case V4L_ListType_TextFile:
			ltf = (struct V4L__ListTextFile *)&lp->Buffer ;
			v_Msg(ctx,tb1,"ListTypeFileCon",ltf->FileSpec,ltf->lineCount) ;
			UCstrcat(dststr,tb1) ; break ;
		   case V4L_ListType_AggRef:
			if (format & V4DPI_FormatOpt_ListInParens) UCstrcat(dststr,UClit("(")) ;
			di1 = v4dpi_DimInfoGet(ctx,lp->Dim) ;
			UCstrcat(dststr,di1->DimName) ; UCstrcat(dststr,UClit(":")) ;
			lar = (struct V4L__ListAggRef *)&lp->Buffer[0] ;
			for(i=0;i<lar->Count;i++)
			 { v_Msg(ctx,tb1,"@(%1U)%2d..%3d",gpi->AreaAgg[lar->AreaAggIndex[i]].pcb->UCFileName,
						lar->Agg[i].BeginPt,lar->Agg[i].EndPt) ;
			   UCstrcat(dststr,tb1) ;
			   if (i < lar->Count-1) UCstrcat(dststr,UClit(",")) ;
			 } ;
			if (format & V4DPI_FormatOpt_ListInParens) UCstrcat(dststr,UClit(")")) ; break ;
		 } ;
		break ;
	   case V4DPI_PntType_BigIsct:
		if (ctx->rtStackX >= 0)
		 { if (ctx->rtStack[ctx->rtStackX].biBuf == NULL)
		    { ctx->rtStack[ctx->rtStackX].biBuf = (struct V4DPI__BigIsct *)v4mm_AllocChunk(sizeof *bibuf,FALSE) ; } ;
		   bibuf = ctx->rtStack[ctx->rtStackX].biBuf ;
		 } else { bibuf = (struct V4DPI__BigIsct *)v4mm_AllocChunk(sizeof *bibuf,FALSE) ; } ;
		for(i=gpi->LowHNum;i<=gpi->HighHNum;i++)
		 { if (gpi->RelH[i].aid == UNUSED) continue ;
		   bibuf->kp.fld.KeyType = V4IS_KeyType_V4 ; bibuf->kp.fld.KeyMode = V4IS_KeyMode_Int ;
		   bibuf->kp.fld.AuxVal = V4IS_SubType_BigIsct ; bibuf->DimUID = point->Value.BigIsct.DimUID ;
		   bibuf->kp.fld.Bytes = V4IS_IntKey_Bytes ;
		   FINDAREAINPROCESS(areax,gpi->RelH[i].aid) ;
		   GRABAREAMTLOCK(areax) ;
		   if (v4is_PositionKey(gpi->RelH[i].aid,(struct V4IS__Key *)bibuf,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey)
		    { FREEAREAMTLOCK(areax) ; continue ; } ;
		   bibp = (struct V4DPI__BigIsct *)v4dpi_GetBindBuf(ctx,gpi->RelH[i].aid,ikde,FALSE,NULL) ;
		   memcpy(bibuf,bibp,bibp->Bytes) ;
		   FREEAREAMTLOCK(areax) ;
		   point = (struct V4DPI__Point *)&bibuf->biPnt ; /* Link to big point */
		   v4dpi_IsctToString(dststr,point,ctx,(((format == V4DPI_FormatOpt_Dump) || ((format & V4DPI_FormatOpt_Echo) != 0)) ? format : (format|V4DPI_FormatOpt_Trace)),maxlen) ;
		   if (ctx->rtStackX < 0) v4mm_FreeChunk(bibuf) ;
		   break ;
		 } ;
//		v4_error(0,0,"V4DPI","EvalIsct","NOBIGISCT","Could not find BigIsct #%d",bibuf->DimUID) ;
		if (i > gpi->HighHNum)
		 { UCsprintf(tb1,UCsizeof(tb1),UClit("[?BigIsct #%d]"),bibuf->DimUID) ; UCstrcat(dststr,tb1) ; } ;
		break ;
	   case V4DPI_PntType_Isct:
	   case V4DPI_PntType_QIsct:
		v4dpi_IsctToString(dststr,point,ctx,(((format == V4DPI_FormatOpt_Dump) || ((format & V4DPI_FormatOpt_Echo) != 0)) ? format : (format|V4DPI_FormatOpt_Trace)),maxlen) ;
		break ;
	   case V4DPI_PntType_MDArray:
		vmda = (struct V4IM__MDArray *)han_GetPointer(point->Value.IntVal,0) ;
		UCsprintf(tb1,UCsizeof(tb1),UClit("(Array[%d dimensions]=%d elements)"),vmda->MDDimCount,vmda->Elements) ;
		UCstrcat(dststr,tb1) ;
		break ;
	   case V4DPI_PntType_Shell:
		v4dpi_PointToStringML(tb1,(P *)&point->Value,ctx,format,maxlen) ;
		UCstrcat(dststr,tb1) ;
		break ;
	   case V4DPI_PntType_FrgnDataEl:
		des = (struct V4FFI__DataElSpec *)&point->Value ;
		UCsprintf(tb1,UCsizeof(tb1),UClit("(%d,%d,%s,%s,%d,%d,%d,%d,%d,%d)"),
			(int)des->FileRef,(int)des->Element,des->Name,des->DimName,des->V3DT,des->KeyNum,(int)des->Owner,
			(int)des->Offset,(int)des->Bytes,(int)des->Decimals) ;
		UCstrcat(dststr,tb1) ;
		break ;
	   case V4DPI_PntType_FrgnStructEl:
		ses = (struct V4FFI__StructElSpec *)&point->Value ;
		v_Msg(ctx,tb1,"@(%1d,%2d,%3d,%4d,%5d,%6s,%7d)",
			(int)ses->FileRef,(int)ses->StructNum,(int)ses->Element,(int)ses->Bytes,(int)ses->Occurs,
			ses->CountField,(int)ses->Offset) ;
		UCstrcat(dststr,tb1) ;
		break ;
	 } ;
	if (ShowCtxOwner) ShowCtxCnt = UNUSED ;
	return(dststr) ;
}

/*	v4dpi_IsctToString - Formats an intersection point into a string		*/
/*	Call: result = v4dpi_IsctToString( dststr , point , ctx , format , maxlen )
	  where result is result code,
		dststr is pointer to destination string,
		point is pointer to point to be formatted,
		ctx is pointer to current context,
		format is bitmap of formatting options,
		maxlen is maximum length allowed for output				*/

void v4dpi_IsctToString(dststr,point,ctx,format,maxlen)
  UCCHAR *dststr ;
  struct V4DPI__Point *point ;
  struct V4C__Context *ctx ;
  int format,maxlen ;
{ struct V4DPI__Point *ipnt,*cpt ;
  struct V4DPI__DimInfo *di ;
  int px,intmod,i,curlen ; UCCHAR tb1[5000] ;
  int ShowCtxOwner = FALSE ;

	curlen = 0 ;
	if (ShowCtxCnt == UNUSED) { ShowCtxOwner = TRUE ; ShowCtxCnt = 0 ; } ;
	ipnt = ISCT1STPNT(point) ;		/* Set intersection point to first point */
//	if (point->Quoted) strcat(dststr,"@") ; if (point->ForceEval) strcat(dststr,"`") ; moved to PointToString veh010607
	if (point->Grouping == 0)		/* Got empty intersection ? */
	 { UCstrcat(dststr,UClit("[ ]")) ; ipnt = ISCT1STPNT(point) ; goto isct_end ; } ;
	if (ipnt->Dim == Dim_IntMod)
	 { intmod = TRUE ;
	   v4dpi_PointToString(tb1,ipnt,ctx,V4DPI_FormatOpt_Echo) ;
	   UCstrcat(dststr,tb1) ; UCstrcat(dststr,UClit("(")) ; curlen += UCstrlen(tb1) + 1 ;
	   if (point->TraceEval) UCstrcat(dststr,UClit("? ")) ;
	   px = 1 ; ipnt = (P *)((char *)ipnt + ipnt->Bytes) ;		/* Advance to next point */
	 } else
	 { px = 0 ; intmod = FALSE ;
	   if (point->Grouping == 2 && ipnt->Grouping == V4DPI_Grouping_Current)
	    { DIMINFO(di,ctx,ipnt->Dim) ;		/* Got [x* y] - format as x.y */
	      UCstrcat(dststr,di->DimName) ; curlen += UCstrlen(di->DimName) ;
	      if (format & V4DPI_FormatOpt_ShowCtxVal)
	       { for(i=0;i<ShowCtxCnt;i++) { if (ShowCtxDims[i] == ipnt->Dim) break ; } ;
		 if (i >= ShowCtxCnt)					/* Only show dimension once */
	          { if (ShowCtxCnt < ShowCtxDimMax) { ShowCtxDims[ShowCtxCnt++] = ipnt->Dim ; } ;
		    DIMVAL(cpt,ctx,ipnt->Dim) ;
		    if (cpt == NULL) { UCstrcat(dststr,UClit("<undefined>")) ; }
		     else { v4dpi_PointToString(tb1,cpt,ctx,format|V4DPI_FormatOpt_DisplayerTrace) ; UCstrcat(dststr,UClit("<")) ;
			    if (UCstrlen(tb1) > 250) { tb1[250] = UCEOS ; UCstrcat(tb1,UClit("...")) ; } ;
			    UCstrcat(dststr,tb1) ; UCstrcat(dststr,UClit(">")) ; curlen += UCstrlen(tb1) + 1 ;
		          } ;
		  } ;
	       } ;
	      UCstrcat(dststr,UClit(".")) ;
	      ADVPNT(ipnt) ;
	      v4dpi_PointToString(tb1,ipnt,ctx,(ipnt->Dim == Dim_NId ? 0 : V4DPI_FormatOpt_ShowDim)) ; UCstrcat(dststr,tb1) ; curlen += UCstrlen(tb1) + 1 ;
	      ipnt = (P *)((char *)ipnt + ipnt->Bytes) ;	/* Advance to next point */
	      goto isct_end ;
	    } ;
	   UCstrcat(dststr,UClit("[")) ; curlen += 1 ;
	   if (point->TraceEval) { UCstrcat(dststr,UClit("? ")) ; curlen += 1 ; } ;
	   if (format & V4DPI_FormatOpt_ShowBytes)
	    { UCsprintf(tb1,UCsizeof(tb1),UClit("(%d) "),point->Bytes) ; UCstrcat(dststr,tb1) ; curlen += UCstrlen(tb1) ; } ;
	 } ;
	for(;px<point->Grouping;px++)
	 { 
	   if (curlen > maxlen)
	    { UCstrcat(dststr,UClit(" ... ")) ; break ; } ;		/* Don't want to go bonkers on long output */
	   if (ipnt->AltDim) UCstrcat(dststr,UClit("~")) ;
	   v4dpi_PointToStringML(tb1,ipnt,ctx,format,maxlen) ; /* Format this point */
	   UCstrcat(dststr,tb1) ; curlen += UCstrlen(tb1) + 1 ;
	   if (px < point->Grouping-1) UCstrcat(dststr,UClit(" ")) ;	/* Delimit with a space */
	   if (px == point->LHSCnt-1) UCstrcat(dststr,UClit("| ")) ;
	   ipnt = (P *)((char *)ipnt + ipnt->Bytes) ;		/* Advance to next point */
	 } ;
	UCstrcat(dststr,(intmod ? UClit(")") : UClit("]"))) ;	/* Terminate isct or intmod */
	curlen += 1 ;
isct_end:
	if (point->Continued)					/* Got another Isct ? */
	 { UCstrcat(dststr,UClit(",")) ;
	   if (curlen > maxlen) { UCstrcat(dststr,UClit("...")) ; }
	    else { v4dpi_PointToStringML(tb1,ipnt,ctx,format,maxlen-curlen) ; UCstrcat(dststr,tb1) ; } ;
	 } ;

	if (ShowCtxOwner) ShowCtxCnt = UNUSED ;
	return ;
}

#define PNTIDXMAX 512				/* MUST BE 2^n matching mask below! */
#define PNTIDXMASK 0x1FF 			/* Mask for above */
#define PNTIDXSHIFT 9				/* Shift mask for above */
struct lcl__PointMaster 			/* Local structure to track temp points */
{ int count ;
  int mlruidx ; 				/* Last used idx */
  int nextlru ; 				/* Next lru (last lru + 1) */
  struct {
    int pntidx ;				/* Point Index allocated (also used for LRU reallocation) */
    int lruidx ;
    struct V4DPI__Point *pt ;
    char *auxptr ;				/* Extra pointer for special what-ever */
  } entry[PNTIDXMAX] ;
  int scount ;					/* Number in use below */
  struct {
    int scope ; 				/* Scope of the point (= NestedIntModCount) */
    struct V4DPI__Point *pt ;
   } sentry[PNTIDXMAX] ;
} lpm ;

P *v4dpi_ScopePnt_Alloc(scope)
  int scope ;
{ int i ;

	for(i=0;i<lpm.scount;i++) { if (lpm.sentry[i].scope == 0) break ; } ;
	if (i < lpm.scount)			/* Reuse a slot */
	 { lpm.sentry[i].scope = scope ;
	   return(lpm.sentry[i].pt) ;
	 } ;
	if (lpm.scount >= PNTIDXMAX)
	 v4_error(0,0,"V4DPI","PntIdxAlloc","MAXPNTS","Attempt to allocate too many scoped points") ;
	lpm.sentry[lpm.scount].pt = (P *)v4mm_AllocChunk(sizeof *lpm.sentry[0].pt,FALSE) ;
	lpm.sentry[lpm.scount].scope = scope ;
	return(lpm.sentry[lpm.scount++].pt) ;
}

void v4dpi_ScopePnt_Free(scope)
  int scope ;
{ int i ;

	for(i=0;i<lpm.scount;i++)
	 { if (lpm.sentry[i].scope >= scope) { lpm.sentry[i].scope = 0 ; } ;	/* Flag slot as unused */
	 } ;
}

/*	v4dpi_PntIdx_AllocPnt - Allocates a point from internal buffers 	*/
/*	Call: index = v4dpi_PntIdx_AllocPnt( scope )
	  where index is integer index returned,
		pntidxarray[index] = pointer to the new point buffer		*/

int v4dpi_PntIdx_AllocPnt()
{ 
  int i,lru,lrux ;

	if (lpm.count < PNTIDXMAX)
	 { lrux = lpm.count ; lpm.entry[lrux].pt = (P *)v4mm_AllocChunk(sizeof(P),FALSE) ;
	 }
	 else { for(lru=V4LIM_BiggestPositiveInt,i=0;i<PNTIDXMAX;i++) 	/* Have to find least recently used */
		 { if (lpm.entry[i].lruidx == lpm.nextlru)	/* Found next one? */
		    { lrux = i ; break ; } ;
		   if (lru > lpm.entry[i].lruidx) { lru = lpm.entry[i].lruidx ; lrux = i ; } ;
		 } ;
		lpm.nextlru = lpm.entry[lrux].lruidx + 2 ;	/* Set nextlru to next "theoretical" lru */
	      } ;
	lpm.entry[lrux].pntidx = ((lpm.count++)<<PNTIDXSHIFT) | lrux ;
	lpm.entry[lrux].lruidx = lpm.mlruidx++ ;
	return(lpm.entry[lrux].pntidx) ;
}

/*	v4dpi_PntIdx_Shutdown - Frees up all points	*/
void v4dpi_PntIdx_Shutdown()
{ int i ;

	for(i=0;i<PNTIDXMAX;i++) { if (lpm.entry[i].pt != NULL) v4mm_FreeChunk(lpm.entry[i].pt) ; } ;
	for(i=0;i<PNTIDXMAX;i++) { if (lpm.sentry[i].pt != NULL) v4mm_FreeChunk(lpm.sentry[i].pt) ; } ;
}


/*	v4dpi_PntIdx_CvtIdxPtr - Converts index to pointer			*/
/*	Call: pointer = v4dpi_PntIdx_CvtIdxPtr( index )
	  where pointer is pointer to internal point buffer,
		index is index returned by above module 			*/

struct V4DPI__Point *v4dpi_PntIdx_CvtIdxPtr(index)
  int index ;
{
	if (lpm.entry[index&PNTIDXMASK].pntidx != index)
	 v4_error(V4E_PNTIDXNOTVALID,0,"V4DPI","PntIdx_AllocAux","PNTIDXNOTVALID","Index (%d) not valid (slot has been re-used?)",index) ;
	lpm.entry[index&PNTIDXMASK].lruidx = lpm.mlruidx++ ;
	return(lpm.entry[index&PNTIDXMASK].pt) ;
}

/*	v4dpi_UOMInitialize - Attempt to initialize UOM tables			*/
/*	Call: ok = v4dpi_UOMInitialize( table )
	  where ok is TRUE if all is well, FALSE if problems,
		table is UNUSED to initialize all, or >= 0 to init a table	*/

LOGICAL v4dpi_UOMInitialize(ctx,table)
  struct V4C__Context *ctx ;
  int table ;
{ P *ipt,tpt,v4pt ;
  enum DictionaryEntries deval ;
  UCCHAR errbuf[128] ; UCCHAR saveErr[V4TMBufMax] ;

	if (gpi->uomt != NULL && ctx->rtStackFail != UNUSED) return(TRUE) ;	/* If have been here before and in error unwind then don't do anything else */
	if (gpi->uomt == NULL) { gpi->uomt = (struct V4DPI__UOMTable *)v4mm_AllocChunk(sizeof(*gpi->uomt),TRUE) ; } ;
	INITISCT(&v4pt) ; NOISCTVCD(&v4pt) ; v4pt.Grouping = 1 ;				/* Construct [UV4:UOMInitialize] intersection */
	ipt = ISCT1STPNT(&v4pt) ; dictPNTv(ipt,Dim_UV4,v4im_GetEnumToDictVal(ctx,deval=_UOMInitialize,UNUSED)) ;
	if (ipt->Dim == 0 || ipt->Value.IntVal == 0) return(TRUE) ;
	v4pt.Bytes += ipt->Bytes ;
	if (table != UNUSED)					/* Maybe add in Int:table */
	 { ADVPNT(ipt) ; v4pt.Grouping++ ; intPNTv(ipt,table) ; v4pt.Bytes += ipt->Bytes ; } ;
	UCstrcpy(saveErr,ctx->ErrorMsg) ;			/* Save error msg buf (IsctEval clears it) */
	ipt = v4dpi_IsctEval(&tpt,&v4pt,ctx,V4DPI_EM_NoIsctFail,NULL,NULL) ;
	UCstrcpy(ctx->ErrorMsg,saveErr) ;
	if (ipt != NULL) return(TRUE) ;
/*	UOM init failed - dump out some trace stuff */
	v_Msg(ctx,errbuf,"*UOMInitFail") ; vout_UCText(VOUT_Err,0,errbuf) ;
	v4trace_ExamineState(ctx,&v4pt,V4_EXSTATE_All,VOUT_Err) ;
	return(FALSE) ;
}

/*	v4dpi_UOMAttemptCoerce - Attempt to Coerce from one UOM to Another	*/
/*	Call: ok = v4dpi_UOMAttemptCoerce( ctx, trace, UOMRef, UOMIndex, UOMPt, resdbl )
	  where ok is TRUE if successful, FALSE otherwise,
		ctx, trace are same old,
		UOMRef is target UOM Ref,
		UOMIndex is target UOM Index (UNUSED if not important),
		UOMPt is point containing UOM to be coerced,
		resdbl is pointer to resulting double to accept value		*/

LOGICAL v4dpi_UOMAttemptCoerce(ctx,trace,UOMRef,UOMIndex,UOMPt,resdbl)
  struct V4C__Context *ctx ;
  int trace,UOMRef,UOMIndex ;
  P *UOMPt ;
  double *resdbl ; 
{ P *ipt,tpt,v4pt ; int ok ;
  enum DictionaryEntries deval ;

/*	Build up: [UV4:UOMCoerce UOMRef:n UOMIndex:n UOMPt:xxx]			*/
	INITISCT(&v4pt) ; NOISCTVCD(&v4pt) ; v4pt.Grouping = 4 ;
	ipt = ISCT1STPNT(&v4pt) ; dictPNTv(ipt,Dim_UV4,v4im_GetEnumToDictVal(ctx,deval=_UOMCoerce,UNUSED)) ;
	ADVPNT(ipt) ; intPNTv(ipt,UOMRef) ; if((ipt->Dim = v4dpi_DimGet(ctx,UClit("UOMRef"),DIMREF_IRT)) == 0) return(FALSE) ;

	ADVPNT(ipt) ; intPNTv(ipt,UOMIndex) ; if((ipt->Dim = v4dpi_DimGet(ctx,UClit("UOMIndex"),DIMREF_IRT)) == 0) return(FALSE) ;

	ADVPNT(ipt) ; ZPH(ipt) ; ipt->PntType = V4DPI_PntType_Shell ; ipt->Bytes = V4DPI_PointHdr_Bytes + UOMPt->Bytes ;
	if((ipt->Dim = v4dpi_DimGet(ctx,UClit("UOMPt"),DIMREF_IRT)) == 0) return(FALSE) ;
	memcpy(&ipt->Value,UOMPt,UOMPt->Bytes) ; 

	ADVPNT(ipt) ; ISCTLEN(&v4pt,ipt) ;
	ipt = v4dpi_IsctEval(&tpt,&v4pt,ctx,V4DPI_EM_NoIsctFail,NULL,NULL) ;
	if (ipt == NULL) return(FALSE) ;
	*resdbl = v4im_GetPointDbl(&ok,ipt,ctx) ;
	return(ok) ;
}

/*	D E B U G G I N G   M O D U L E S				*/

UCCHAR *v4dbg_FormatLineNum(linenum)
  int linenum ;
{
  static UCCHAR lnBuf[20] ;

	if ((linenum % 10) == 0) { UCsprintf(lnBuf,UCsizeof(lnBuf),UClit("%d"),linenum/10) ; }
	 else { UCsprintf(lnBuf,UCsizeof(lnBuf),UClit("%d.%d"),linenum/10,(linenum % 10)) ; } ;
	return(lnBuf) ;
}

UCCHAR *v4dbg_FormatBP(bpnum)
  int bpnum ;
{
  static UCCHAR bpBuf[20] ;

	if (bpnum == 0) { ZUS(bpBuf) ; }
	 else { UCsprintf(bpBuf,UCsizeof(bpBuf),UClit("%d"),bpnum) ; } ;
	return(bpBuf) ;
}

UCCHAR *v4dbg_FormatVBP(ctx,vbp,vis,curcall,shortForm)
  struct V4C__Context *ctx ;
 struct V4DBG__BreakPoint *vbp ;
 union V4DPI__IsctSrc *vis ;
 int curcall ;
 LOGICAL shortForm ;
{ 
  static UCCHAR bpBuf[1024],tbuf[V_FileName_Max+32] ;

	ZUS(bpBuf) ;
	if (vbp->bpId != 0) { UCstrcat(bpBuf,UClit("#")) ; UCstrcat(bpBuf,v4dbg_FormatBP(vbp->bpId)) ; UCstrcat(bpBuf,UClit(" ")) ; } ;
	if (curcall != 0 && (vbp->bpFlags & V4DBG_bpFlag_EvalCount)) { UCsprintf(tbuf,UCsizeof(tbuf),UClit("(%d) "),curcall) ; UCstrcat(bpBuf,tbuf) ; } ;
	if (vbp->intmodX != 0 && !shortForm) { UCstrcat(bpBuf,v4im_GetEnumToUCVal(DE(Module))) ; UCstrcat(bpBuf,UClit(" ")) ; UCstrcat(bpBuf,v4im_Display(vbp->intmodX)) ; UCstrcat(bpBuf,UClit(" ")) ; } ;
	if (vbp->isctEvals > 0 && !shortForm) { v_Msg(NULL,tbuf,"@%1S %2d ",DE(After),vbp->isctEvals) ; UCstrcat(bpBuf,tbuf) ; } ;
	if (vbp->condPt != NULL && !shortForm) { v_Msg(NULL,tbuf,"@%1S %2P ",DE(If),vbp->condPt) ; UCstrcat(bpBuf,tbuf) ; } ;
	if (vbp->evalPt != NULL && !shortForm) { v_Msg(NULL,tbuf,"@%1S %2P ",DE(Do),vbp->evalPt) ; UCstrcat(bpBuf,tbuf) ; } ;
	if (vbp->resPt != NULL && !shortForm) { v_Msg(NULL,tbuf,"@%1S %2P ",DE(Return),vbp->resPt) ; UCstrcat(bpBuf,tbuf) ; } ;
#define SFOPT(FLAG,NAME) if ((vbp->bpFlags & V4DBG_bpFlag_##FLAG) && !shortForm) { UCstrcat(bpBuf,v4im_GetEnumToUCVal(DE(NAME))) ; UCstrcat(bpBuf,UClit(" ")) ; } ;
#define nSFOPT(FLAG,NAME) if ((vbp->bpFlags & V4DBG_bpFlag_##FLAG) == 0 && !shortForm) { UCstrcat(bpBuf,v4im_GetEnumToUCVal(DE(NAME))) ; UCstrcat(bpBuf,UClit(" ")) ; } ;
	SFOPT(Break,Break) nSFOPT(Break,Continue) SFOPT(Context,Context) SFOPT(BreakOnFail,Fail) SFOPT(Loc,Location) SFOPT(OneTime,Once) SFOPT(EvalRes,Results)
	SFOPT(Isct,Point) SFOPT(DoNotEval,Skip) SFOPT(Stack,Stack) SFOPT(WallCPU,Time)
	if ((vbp->bpFlags & V4DBG_bpFlag_Break) && !shortForm) { UCstrcat(bpBuf,v4im_GetEnumToUCVal(DE(Break))) ; UCstrcat(bpBuf,UClit(" ")) ; } ;
	if ((vis == NULL ? FALSE : vis->iVal != 0) && (vbp->bpFlags & V4DBG_bpFlag_Loc))
	 { struct V4LEX__CompileDir *vcd ;
	   vcd = v4trace_LoadVCDforHNum(ctx,vis->c.HNum,TRUE) ;
	   if (vcd != NULL)
	    { UCCHAR *fn = (vcd->File[vis->c.vcdIndex].spwHash64 != 0 && vcd->File[vis->c.vcdIndex].spwHash64 != gpi->spwHash64 ? UClit("restricted-file") : vcd->File[vis->c.vcdIndex].fileName) ;
	      v_Msg(ctx,tbuf,"@%1U:%2U ",fn,v4dbg_FormatLineNum(vis->c.lineNumber)) ; UCstrcat(bpBuf,tbuf) ;
	    } else
	    { UCstrcat(bpBuf,v4dbg_FormatLineNum(vis->c.lineNumber)) ; UCstrcat(bpBuf,UClit(" ")) ; } ;
	 } ;
	if (vbp->bpFlags & V4DBG_bpFlag_Disable) UCstrcat(bpBuf,UClit("*")) ;
	return(bpBuf) ;
}

void v4dbg_bplInit()
{

	gpi->bpl = (struct V4DBG__BreakPointList *)v4mm_AllocChunk(sizeof *gpi->bpl,TRUE) ;
	gpi->bpl->rtStackXStep = UNUSED ;
}

/*	v4dbg_BreakPoint - Handles breakpoint logic given that breakpoint should occur		*/
/*	Call: res = v4dbg_BreakPoint( ctx , vbp , isct , curcall , didEval , respnt )
	  where res is result code (see V4EVAL_Res_xxx),
		ctx is current context,
		vbp is pointer to breakpoint structure,
		isct is intersection that we stopped on,
		curcall is current number of evaluations,
		didEval is TRUE if isct has already been evaluated (ex: we only want to break on failure),
		respnt is point to be updated with any result					*/

int v4dbg_BreakPoint(ctx,vbp,isct,curcall,didEval,respnt)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *isct,*respnt ;
  struct V4DBG__BreakPoint *vbp ;
  int curcall ;
  LOGICAL didEval ;
{ jmp_buf traceback ; int savertStackX, savertStackFail,loop ;
  struct V4LEX__TknCtrlBlk *tcb ;
  int res ;

	if (vbp->bpFlags & V4DBG_bpFlag_FromV4)
	 { v_Msg(ctx,UCTBUF2,"DBGV4Break",V4IM_OpCode_V4,-V4IM_Tag_Break) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ; } ;
	if ((vbp->bpFlags & V4DBG_bpFlag_Isct))
	 { v_Msg(ctx,UCTBUF2,"DBGBreakLoc",v4dbg_FormatVBP(ctx,vbp,&isct->Value.Isct.vis,curcall,TRUE),isct) ;  }
	 else { v_Msg(ctx,UCTBUF2,"DBGBreakLoc2",v4dbg_FormatVBP(ctx,vbp,&isct->Value.Isct.vis,curcall,TRUE)) ; } ;
	vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	if (vbp->bpFlags & (V4DBG_bpFlag_WallCPU | V4DBG_bpFlag_DeltaWallCPU))
	 { static double lastConnect=0, lastCPU=0 ; double connect=v_ConnectTime(), cpu=v_CPUTime() ;
	   if ((vbp->bpFlags & (V4DBG_bpFlag_WallCPU | V4DBG_bpFlag_DeltaWallCPU)) == (V4DBG_bpFlag_WallCPU | V4DBG_bpFlag_DeltaWallCPU))
	    { v_Msg(ctx,UCTBUF2,"DBGTimeCumInc",cpu-lastCPU,cpu,connect-lastConnect,connect) ; }
	    else if (vbp->bpFlags & V4DBG_bpFlag_WallCPU)
	     { v_Msg(ctx,UCTBUF2,"DBGTimeCum",cpu,connect) ; }
	    else { v_Msg(ctx,UCTBUF2,"DBGTimeInc",cpu-lastCPU,connect-lastConnect) ; }
	   vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	   lastCPU = cpu ; lastConnect = connect ;
	 } ;
	if (didEval && respnt == NULL && (vbp->bpFlags & V4DBG_bpFlag_Loc))
	 { UCCHAR *msg ; msg = ctx->rtStack[ctx->rtStackFail].failText ; if (msg == NULL) msg = ctx->ErrorMsg ;
	   v_Msg(ctx,UCTBUF2,"DBGBreakOnFail",msg) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	 } ;
	if (vbp->bpFlags & V4DBG_bpFlag_Context)
	 { v4trace_ExamineState(ctx,NULL,V4_EXSTATE_Context,VOUT_Status) ; } ;
	if (vbp->bpFlags & V4DBG_bpFlag_Stack)
	 { v4trace_ExamineState(ctx,NULL,V4_EXSTATE_Stack,VOUT_Status) ; } ;
	if(!v_IsAConsole(stdin))
	 { v_Msg(ctx,UCTBUF1,"DBGBreakIgn1") ; vout_UCText(VOUT_Warn,0,UCTBUF1) ; return(V4EVAL_Res_BPContinue) ; } ;
	tcb = (struct V4LEX__TknCtrlBlk *)v4mm_AllocChunk(sizeof *tcb,FALSE) ; v4lex_InitTCB(tcb,0) ; UCstrcpy(tcb->ilvl[0].prompt,UClit("V4DBG>")) ;
	COPYJMP(traceback,(gpi->environment)) ; savertStackX = ctx->rtStackX ; savertStackFail = ctx->rtStackFail ; ctx->rtStackX++ ;
	loop = (vbp->bpFlags & V4DBG_bpFlag_Break) != 0 ;
	res = 0 ;
	for(;loop;)
	 { res = v4eval_Eval(tcb,ctx,TRUE,0,TRUE,FALSE,FALSE,respnt) ;
	   switch (res & V4EVAL_ResMask)
	    { default:		v_Msg(ctx,UCTBUF2,"DBGInvExit",DE(Debug),DE(Stop)) ; vout_UCText(VOUT_Warn,0,UCTBUF2) ; break ;
	      case V4EVAL_Res_StopXct:
	      case V4EVAL_Res_BPContinue:
	      case V4EVAL_Res_BPStep:
	      case V4EVAL_Res_BPStepInto:
	      case V4EVAL_Res_BPStepNC:
		 loop = FALSE ; break ;
	    } ;
	 } ;
	ctx->rtStackX = savertStackX ; ctx->rtStackFail = savertStackFail ;
	COPYJMP((gpi->environment),traceback) ; v4lex_FreeTCB(tcb) ;
	if ((res & V4EVAL_ResMask) == V4EVAL_Res_StopXct)
	 { v_Msg(ctx,UCTBUF2,"DBGStopXct") ; v4_UCerror(V4E_Warn,0,"V4W","User","Stop",UCTBUF2) ; } ;

	return(res) ;
}



/*	E V A L U A T I O N   M O D U L E S				*/


void v4dpi_IsctEvalCacheClear(ctx,shutdown)			/* Clear out the IsctEval cache */
  struct V4C__Context *ctx ;
  LOGICAL shutdown ;
{ int i ;

	for(i=0;i<V4_IsctEval_CacheMax;i++) { ctx->viec.Entry[i].KeyVal = UNUSED ; } ;
	if (shutdown)
	 { for(i=0;i<V4_IsctEval_CacheMax;i++)
	    { if(ctx->viec.Entry[i].bind != NULL) v4mm_FreeChunk(ctx->viec.Entry[i].bind) ; } ;
	 } ;
}


struct V4DPI__LittlePoint CondEvalRet ;		/* Global UV4:CondEvalFail point */

/*	v4dpi_IsctEval - Evaluates an Intersection			*/
/*	Call: result = v4dpi_IsctEval( respnt , isct , ctx , evalmode , aidptr , elptr )
	  where result is the result code,
		respnt is pointer to buffer to accept evaluated point (i.e. the result),
		isct is pointer to the intersection,
		ctx is pointer to context to be used,
		evalmode- V4DPI_EM_xxx - how to evaluate (usually 0)
		trace is TRUE to have search trace printed on standard output
		aidptr if non-NULL is updated with aid & result is pointer to V4DPI__Binding (for list operations)
		elptr is non-NULL is pointer to V4DPI__EvalList for continued searches (e.g. for Partials)	*/

struct V4DPI__Point *v4dpi_IsctEval(respnt,isctptr,ctx,evalmode,aidptr,elptr)
  struct V4DPI__Point *respnt ;
  struct V4DPI__Point *isctptr ;
  struct V4C__Context *ctx ;
  int evalmode,*aidptr ;
  struct V4DPI__EvalList *elptr ;
{
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4DPI__DimInfo *di ;
  struct V4DPI__EvalList elbuf,*el ;
  struct V4DPI__BindList_Value *nblv ;
  struct V4DPI__BigIsct *bibuf,*bibp ;
  struct V4DPI__Point isctbuf,iterbuf ;
  struct V4DPI__Point *tpnt,*ipnt,*bpnt,*spnt,*isct ;
  struct V4DPI__LittlePoint lpnt ;
  static int didInit = FALSE ; 
  struct V4L__ListPoint *lp ;
  enum DictionaryEntries deval ;
  int fromctx[V4DPI_IsctDimMax] ;
  int i,j,cvx,hx,ix,dx,framepush,musteval,curcall,traceEval ; double d1 ;
//  int failintmodx=UNUSED ;
  P *failisct=NULL ; VTRACE trace ;
  
#define CHECKRECURSIVE 1
#ifdef CHECKRECURSIVE
  int crHX = UNUSED ;
#define LCL_RHT_Max 10007
struct lcl_RecursionHashTable {
  int iValHash[LCL_RHT_Max] ;
} ;
#define CHECKRECURSERETURN if (crHX != UNUSED) { lrht->iValHash[crHX] = 0 ; crHX = UNUSED ; } ;
static struct lcl_RecursionHashTable *lrht = NULL ;
	if (traceGlobal & V4TRACE_Recursion)
	 { if (lrht == NULL) lrht = (struct lcl_RecursionHashTable *)v4mm_AllocChunk(sizeof *lrht,TRUE) ;
	   if (isctptr->Value.Isct.vis.iVal != 0)
	    { crHX = ((isctptr->Value.Isct.vis.iVal & 0x7fffffff) % LCL_RHT_Max) ;
	      if (lrht->iValHash[crHX] == isctptr->Value.Isct.vis.iVal)	/* Got match - then we are recursing in */
/*	         Set an immediate breakpoint (will be caught below) */
	       { if (gpi->bpl == NULL) v4dbg_bplInit() ;
	         INITBP(&gpi->bpl->vbp[gpi->bpl->Count]) ;
	         gpi->bpl->vbp[gpi->bpl->Count].bpFlags = (V4DBG_bpFlag_Default | V4DBG_bpFlag_OneTime) ;	/* Just set up unconditional one-time breakpoint */
	         gpi->bpl->vbp[gpi->bpl->Count].bpId = ++gpi->bpl->luId ; gpi->bpl->Count++ ;
	         v_Msg(ctx,UCTBUF2,"DBGRecursion") ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	       } else { lrht->iValHash[crHX] = isctptr->Value.Isct.vis.iVal ; } ;
	    } ;
	 } ;
#else
#define CHECKRECURSERETURN
#endif

	framepush = FALSE ;						/* Set TRUE if we push a new context frame */
	musteval = FALSE ;
	curcall = (++ctx->IsctEvalCount) ;
	trace = traceGlobal ;
	traceEval = (trace & V4TRACE_LogIscts) != 0 && (ctx->IsctEvalCount > gpi->StartTraceOutput) && ((evalmode & V4DPI_EM_NoTrace) == 0) ;
	if (!didInit)
	 { didInit = TRUE ;
	   ZPH(&CondEvalRet) ; CondEvalRet.PntType = V4DPI_PntType_Dict ; CondEvalRet.Dim = Dim_UV4 ; CondEvalRet.Bytes = V4PS_Int ;
	   DIMINFO(di,ctx,CondEvalRet.Dim) ;
	   CondEvalRet.Value.IntVal = v4im_GetEnumToDictVal(ctx,deval=_CondEvalFail,Dim_UV4) ;
	 } ;
	if (isctptr == NULL) { v_Msg(ctx,NULL,"IsctEvalNull") ; ++ctx->IsctEvalCount ; goto iscteval_fail ; } ;

#ifdef NASTYBUG
	ctx->isct = isctptr ;
#endif
	
/*	Should we look for and possibly handle a breakpoint ? */
	if (gpi->bpl == NULL ? FALSE : (curcall == gpi->bpl->curcallSkip ? FALSE : (isctptr->PntType == V4DPI_PntType_Isct || isctptr->PntType == V4DPI_PntType_BigIsct)))
	 { int didEval = FALSE ;			/* If this is true then dbgrespnt has result, return it immediately */
	   struct V4DPI__Point *dbgrespnt = NULL ;
	   if(gpi->bpl->rtStackXStep != UNUSED ? ctx->rtStackX <= gpi->bpl->rtStackXStep : FALSE)
	    { struct V4DBG__BreakPoint vbp ; int res ; 
	      INITBP(&vbp) vbp.bpFlags = (V4DBG_bpFlag_Default | V4DBG_bpFlag_Isct);
	      res = v4dbg_BreakPoint(ctx,&vbp,isctptr,curcall,didEval,respnt) ;
	      gpi->bpl->curcallSkip = curcall + 1 ;		/* To ensure that we don't check any breakpoints on this evaluation */
	      gpi->bpl->rtStackXStep = UNUSED ;			/* Turn off step */
	      if ((res & V4EVAL_ResMask) == V4EVAL_Res_BPStepInto) gpi->bpl->rtStackXStep = ctx->rtStackX + 1 ;
/*	      Do we want to see results of this evaluation, if so then evaluate it now */
	      if (res & V4EVAL_Res_Flag_ShowRes || TRUE)
	       { dbgrespnt = v4dpi_IsctEval(respnt,isctptr,ctx,evalmode,aidptr,elptr) ; didEval = TRUE ;
		 if (dbgrespnt == NULL) { v_Msg(ctx,UCTBUF2,"DBGResFail",curcall) ; }
	          else { v_Msg(ctx,UCTBUF2,"DBGResOK",curcall,dbgrespnt) ; } ;
	         vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	       } ;
/*	      Did we resume with STEP command ? */
	      if ((res & V4EVAL_ResMask) == V4EVAL_Res_BPStep || (res & V4EVAL_ResMask) == V4EVAL_Res_BPStepNC || (res & V4EVAL_ResMask) == V4EVAL_Res_BPStepInto)
	       { gpi->bpl->rtStackXStep = ((res & V4EVAL_ResMask) == V4EVAL_Res_BPStep ? ctx->rtStackX : ctx->rtStackX+1)  ;
	       } ;
	    } else 
	    { int res=0 ;					/* Result from calling v4eval_Eval() */
	      int showBP=TRUE ;				/* Set to FALSE after displaying breakpoint (so we don't show twice) */
	      for(ix=0;ix<gpi->bpl->Count;ix++)
	       { struct V4DBG__BreakPoint vbp ;
	         if (gpi->bpl->vbp[ix].bpFlags & V4DBG_bpFlag_Disable) continue ;		/* Skip if this is disabled */
	         if ((gpi->bpl->vbp[ix].vis.iVal != 0) && (isctptr->Value.Isct.vis.iVal != gpi->bpl->vbp[ix].vis.iVal)) continue ;
/*	         Got a match - check to see if a conditional */
	         if (gpi->bpl->vbp[ix].condPt != NULL)
	          { tpnt = v4dpi_IsctEval(respnt,gpi->bpl->vbp[ix].condPt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	            if (tpnt == NULL) { v_Msg(ctx,UCTBUF2,"DBGEvalFail",V4IM_Tag_If,gpi->bpl->vbp[ix].condPt) ; vout_UCText(VOUT_Warn,0,UCTBUF2) ; continue ; } ;
	            i = v4im_GetPointLog(&j,tpnt,ctx) ;
	            if (!j) { v_Msg(ctx,UCTBUF2,"DBGEvalNotLog",V4IM_Tag_If,gpi->bpl->vbp[ix].condPt,V4DPI_PntType_Logical,tpnt) ; vout_UCText(VOUT_Warn,0,UCTBUF2) ; continue ; } ;
	            if (!i) continue ;
	          } ;
/*	         Break after certain number of evaluations ? */
	         if (gpi->bpl->vbp[ix].isctEvals != 0)
	          { if (curcall <= (gpi->bpl->vbp[ix].isctEvals < 0 ? -gpi->bpl->vbp[ix].isctEvals : gpi->bpl->vbp[ix].isctEvals)) continue ; } ;
/*		 Are we looking to break on next line of V4 (an auto-stepping breaking) */
		 if (gpi->bpl->vbp[ix].rtStackX != 0)
		  { if (ctx->rtStackX > gpi->bpl->vbp[ix].rtStackX) continue ;		/* Nested too deep */
		    if (ctx->rtStackX < gpi->bpl->vbp[ix].rtStackX)			/* We have exitted from this level, clear the breakpoint */
		     { int i ;
		       for(i=ix+1;i<gpi->bpl->Count;i++) { gpi->bpl->vbp[i-1] = gpi->bpl->vbp[i] ; } ;
		       gpi->bpl->Count-- ; ix-- ;
		       continue ;
		     } ;
		    if (isctptr->Value.Isct.vis.c.lineNumber == gpi->bpl->vbp[ix].lineNumber) continue ;	/* Has to be different line number */
		    gpi->bpl->vbp[ix].lineNumber = isctptr->Value.Isct.vis.c.lineNumber ;
		  } ;
/*	         Looking for a particular intmod ? */
	         if (gpi->bpl->vbp[ix].intmodX != 0)
	          { if (isctptr->PntType == V4DPI_PntType_Isct)
	             { if (ISCT1STPNT(isctptr)->Value.IntVal != gpi->bpl->vbp[ix].intmodX || ISCT1STPNT(isctptr)->PntType != V4DPI_PntType_IntMod) continue ; }
/*		    This is a little more involved if BigIsct - have to grab it */
		     else if (isctptr->PntType == V4DPI_PntType_BigIsct)
		     { P *cpnt = NULL ; bibuf = (struct V4DPI__BigIsct *)&isctbuf ;	/* Not very clean, but fast */
		       for(hx=gpi->LowHNum;hx<=gpi->HighHNum;hx++)
		        { if (gpi->RelH[hx].aid == UNUSED) continue ;
		          bibuf->kp.fld.KeyType = V4IS_KeyType_V4 ; bibuf->kp.fld.KeyMode = V4IS_KeyMode_Int ; bibuf->kp.fld.AuxVal = V4IS_SubType_BigIsct ;
		          bibuf->DimUID = isctptr->Value.BigIsct.DimUID ; bibuf->kp.fld.Bytes = V4IS_IntKey_Bytes ;
		          if (v4is_PositionKey(gpi->RelH[hx].aid,(struct V4IS__Key *)bibuf,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey) continue ;
		          bibp = (struct V4DPI__BigIsct *)v4dpi_GetBindBuf(ctx,gpi->RelH[hx].aid,ikde,FALSE,NULL) ;
		          cpnt = (struct V4DPI__Point *)&bibp->biPnt ;
		          cpnt = ISCT1STPNT(cpnt) ;
		          break ;
		        } ;
		       if (cpnt == NULL) continue ;
		       if (cpnt->Value.IntVal != gpi->bpl->vbp[ix].intmodX || cpnt->PntType != V4DPI_PntType_IntMod) continue ;
		     } else { continue ; } ;
/*		    If here then got match on this intmod, set isctEvals so we don't break on this if we continue with a 'Step' */
		    gpi->bpl->vbp[ix].isctEvals = -(curcall+1) ;		/* Why negative? so it does not display in v4dbg_FormatVBP! */
	          } ;
/*	         Only want to break on fail? then have to evaluate now */
	         if (gpi->bpl->vbp[ix].bpFlags & V4DBG_bpFlag_BreakOnFail)
	          { 
	            if (!didEval) 
	             { gpi->bpl->vbp[ix].bpFlags |= V4DBG_bpFlag_Disable ;
	               dbgrespnt = v4dpi_IsctEval(respnt,isctptr,ctx,evalmode,aidptr,elptr) ;
	               gpi->bpl->vbp[ix].bpFlags &= ~V4DBG_bpFlag_Disable ;
	               didEval = TRUE ;
	             } ;
	            if (dbgrespnt != NULL) continue ;
	          } ;
		 vbp = gpi->bpl->vbp[ix] ;		/* Make copy in case we are going to delete it */
/*	         If this is one-time then delete it */
	         if (gpi->bpl->vbp[ix].bpFlags & V4DBG_bpFlag_OneTime)
	          { int i ;
	            for(i=ix+1;i<gpi->bpl->Count;i++) { gpi->bpl->vbp[i-1] = gpi->bpl->vbp[i] ; } ;
	            gpi->bpl->Count-- ; ix-- ;
	          } ;
/*	         Looks good - break now */
	         if (vbp.evalPt != NULL)
	          { tpnt = v4dpi_IsctEval(respnt,vbp.evalPt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	            if (tpnt == NULL) { v_Msg(ctx,UCTBUF2,"DBGEvalFail",V4IM_Tag_Do,vbp.condPt) ; vout_UCText(VOUT_Warn,0,UCTBUF2) ; continue ; } ;
	          } else if (showBP)
	          { res = v4dbg_BreakPoint(ctx,&vbp,isctptr,curcall,didEval,respnt) ;
	            showBP = FALSE ;		/* Only handle break once per instruction */
	          } ;
/*	         Do we want to see results of this evaluation, if so then evaluate it now */
	         if ((res & V4EVAL_Res_Flag_ShowRes) || (vbp.bpFlags & V4DBG_bpFlag_EvalRes))
	          { if (!didEval)			/* Have we evaluated it? */
	             { 
	               gpi->bpl->vbp[ix].bpFlags |= V4DBG_bpFlag_Disable ;
/*		       If we got here via 'Debug SNC' then set NoContinue flag for this evaluation */
	               dbgrespnt = v4dpi_IsctEval(respnt,isctptr,ctx,((res & V4EVAL_ResMask) == V4EVAL_Res_BPStepNC ? evalmode|V4DPI_EM_NoContinue : evalmode),aidptr,elptr) ; didEval = TRUE ;
	               gpi->bpl->vbp[ix].bpFlags &= ~V4DBG_bpFlag_Disable ;
	             } ;
	            if (dbgrespnt == NULL) { v_Msg(ctx,UCTBUF2,"DBGResFail",curcall) ; }
	             else { v_Msg(ctx,UCTBUF2,"DBGResOK",curcall,dbgrespnt) ; } ;
	            vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	          } ;
/*	         Did we resume with STEP command ? */
	         if ((res & V4EVAL_ResMask) == V4EVAL_Res_BPStep || (res & V4EVAL_ResMask) == V4EVAL_Res_BPStepNC || (res & V4EVAL_ResMask) == V4EVAL_Res_BPStepInto)
	          { gpi->bpl->rtStackXStep = ((res & V4EVAL_ResMask) == V4EVAL_Res_BPStep ? ctx->rtStackX : ctx->rtStackX+1)  ;
	            break ;			/* Break out immediately (don't check for any more breakpoints */
	          } ;
/*	         Do we want to return a specific value (whether or not we have already evaluated the current isct) */
	         if (vbp.resPt != NULL)
	          { isctptr = vbp.resPt ;	/* Set isctptr so we evaluate it properly if necessary */
	          } ;
	       } ;
	    } ;
	   if (didEval) { CHECKRECURSERETURN return(dbgrespnt) ; } ;
	 } ;

	ctx->FailProgress = V4FailProgress_Unk ;
/*	Bump run-time stack index (NOTE: If we hit max then continue with error, V4 structures allocated for slightly more than V4TRACE_MaxNestedrtStack nestings) */
	ctx->rtStackX++ ;
	ctx->rtStack[ctx->rtStackX].isctPtr = isctptr ;
	if (isctptr->PntType == V4DPI_PntType_Isct || isctptr->PntType == V4DPI_PntType_BigIsct)
	 { ctx->rtStack[ctx->rtStackX].vis = isctptr->Value.Isct.vis ; }
	 else { memset(&ctx->rtStack[ctx->rtStackX].vis,0,sizeof ctx->rtStack[ctx->rtStackX].vis) ; } ;
	if (ctx->rtStackX == V4TRACE_WarnNestedrtStack)
	 { static int didIt = FALSE ;
	   if (!didIt) { v_Msg(ctx,UCTBUF1,"WarnStackNest") ; vout_UCText(VOUT_Warn,0,UCTBUF1) ; didIt = TRUE ; } ;
	 } ;
	if (ctx->rtStackX >= V4TRACE_MaxNestedrtStack && !gpi->NestMax)
	 { static int Once=FALSE ;
	   if (!Once)
	    { v_Msg(ctx,UCTBUF1,"*IsctEvalMaxNest",isctptr) ; vout_UCText(VOUT_Err,0,UCTBUF1) ; Once = TRUE ; } ;
	   ipnt = v4dpi_MaxNesting(ctx,respnt,isctptr,&i) ;
	   if (i) { ctx->rtStackX -- ; CHECKRECURSERETURN return(ipnt) ; } ;
	   ctx->rtStack[ctx->rtStackX-1].isctPtr = isctptr ;
	   UCstrcpy(ctx->ErrorMsg,UClit("Exceeded max number of nested Isct's")) ; goto iscteval_fail ;
	 } ;
	ctx->rtStackFail = UNUSED ;
	ZUS(ctx->ErrorMsg) ;
	if (traceEval)
	 { v_Msg(ctx,UCTBUF2,"@*%1d: %2P\n",curcall,isctptr) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ; } ;
	el = NULL ;
/*	If already have a saved error then set evalmode flag to remember (VEH050517) */
	if (UCnotempty(gpi->SavedErrMsg))
	 evalmode |= V4DPI_EM_SavedErrSet ;
//eval_pointtype:
	isct = isctptr ;
	switch(isctptr->PntType)
	 { default:
		ctx->rtStackX -- ;
		if (elptr != NULL ? elptr->SkipCnt > 1 : FALSE)
		 { CHECKRECURSERETURN return(NULL) ; } ;		/* Cannot "skip" when evaluating something other than Isct */
		memcpy(respnt,isctptr,isctptr->Bytes) ; return(respnt) ;
	   case V4DPI_PntType_Special:
		if (elptr != NULL ? elptr->SkipCnt > 1 : FALSE)
		 { CHECKRECURSERETURN return(NULL) ; } ;			/* Cannot "skip" when evaluating something other than Isct */
		switch(isctptr->Grouping)
		 { default:
			v_Msg(ctx,NULL,"IsctEvalSpcPt",isctptr) ; goto iscteval_fail ;
		   case V4DPI_Grouping_Now:
			di = v4dpi_DimInfoGet(ctx,isctptr->Dim) ;
			ZPH(respnt) ; respnt->PntType = di->PointType ; respnt->Bytes = V4PS_Int ; respnt->Dim = di->DimId ;
			switch(di->PointType)
			 { default:
				v_Msg(ctx,NULL,"IsctEvalNoNow",isctptr) ; goto iscteval_fail ;
			   case V4DPI_PntType_UWeek:
//				i = time(NULL)-(60*gpi->MinutesWest) ; i = TIMEOFFSETDAY + i/VCAL_SecsInDay ;
				respnt->Value.IntVal = mscu_udate_to_uweek(valUDATEisNOW,di->ds.UWeek.baseUDate) ;
				if (respnt->Value.IntVal == VCAL_BadVal) { v_Msg(ctx,NULL,"IsctEvalNoNow",isctptr) ; goto iscteval_fail ; } ;
				break ;
			   case V4DPI_PntType_Calendar:
//				d1 = VCal_MJDOffset + TIMEOFFSETDAY +(double)(time(NULL))/(double)VCAL_SecsInDay ;
//				frac = modf(d1,&ipart) ; if (frac == 0.0) d1 += VCAL_MidnightValue ;	/* Want some fractional value to indicate time */
//				PUTREAL(respnt,d1) ; respnt->Bytes = V4PS_Calendar ;
				setCALisNOW(d1) ; PUTREAL(respnt,d1) ; respnt->Bytes = V4PS_Calendar ;
				break ;
			   case V4DPI_PntType_UDate:
//				respnt->Value.IntVal = time(NULL)-(60*gpi->MinutesWest) ;
//				respnt->Value.IntVal = TIMEOFFSETDAY + respnt->Value.IntVal/VCAL_SecsInDay ;
				respnt->Value.IntVal = valUDATEisNOW ;
				break ;
			   case V4DPI_PntType_UTime:
//				i = time(NULL)-(60*gpi->MinutesWest) ; d1 = i % VCAL_SecsInDay ;
				setUTIMEisNOW(d1) ;
				PUTREAL(respnt,d1) ; respnt->Bytes = V4PS_UTime ; break ;			
			   case V4DPI_PntType_UYear:
//				i = mscu_udate_to_yyyymmdd(VCAL_LOCALNOW) ;
//				respnt->Value.IntVal = i / 10000 ; break ;
				respnt->Value.IntVal = valUYEARisNOW ; break ;
			   case V4DPI_PntType_UMonth:
//				i = mscu_udate_to_yyyymmdd(VCAL_LOCALNOW) ;
//				respnt->Value.IntVal = (i/10000 - VCAL_BaseYear)*12 + ((i/100) % 100)-1 ; break ;
				UDtoUMONTH(respnt->Value.IntVal,valUDATEisNOW) ; break ;
			   case V4DPI_PntType_UQuarter:
				i = mscu_udate_to_yyyymmdd(VCAL_LOCALNOW) ;
//				respnt->Value.IntVal = (i/10000 - VCAL_BaseYear)*4 + (((i/100) % 100)-1) / 3 ; break ;
				respnt->Value.IntVal = YYYYMMDDtoUQTR(i) ; break ;
			   case V4DPI_PntType_UDT:
				respnt->Value.IntVal = valUDTisNOW ;
				break ;			
			 } ;
			ctx->rtStackX -- ;
			CHECKRECURSERETURN return(respnt) ;
		   case V4DPI_Grouping_None:
		   case V4DPI_Grouping_Undefined:
		   case V4DPI_Grouping_Sample:
		   case V4DPI_Grouping_All:
		   case V4DPI_Grouping_AllCnf:
			ctx->rtStackX -- ;
			memcpy(respnt,isctptr,isctptr->Bytes) ; CHECKRECURSERETURN return(respnt) ;
		   case V4DPI_Grouping_New:				/* Have to create a new AggRef point! */
			di = v4dpi_DimInfoGet(ctx,isctptr->Dim) ;
			ZPH(respnt) ; respnt->Dim = di->DimId ; respnt->PntType = di->PointType ;
			if (respnt->PntType == V4DPI_PntType_AggRef)	/* If an AggRef then pick first updateable Agg */
			 { for(i=0;i<gpi->AreaAggCount;i++)
			    { if (gpi->AreaAgg[i].pcb == NULL) continue ;
			      if ((gpi->AreaAgg[i].pcb->AccessMode & (V4IS_PCB_AM_Insert | V4IS_PCB_AM_Update)) != 0) break ;
			    } ;
			   if (i >= gpi->AreaAggCount) i = 0 ;
			   respnt->Grouping = i ;
			 } else { respnt->Grouping = V4DPI_Grouping_Single ; } ;
			respnt->Value.IntVal = v4dpi_DimUnique(ctx,di,NULL) ; respnt->Bytes = V4PS_Int ;
			if (respnt->Value.IntVal == UNUSED)
			 { v_Msg(ctx,NULL,"IsctEvalNoNew") ; goto iscteval_fail ; } ;
			ctx->rtStackX -- ;
			CHECKRECURSERETURN return(respnt) ;
		   case V4DPI_Grouping_PCurrent:
			DIMPVAL(bpnt,ctx,isctptr->Dim) ;
			if (bpnt != NULL)
			 { memcpy(respnt,bpnt,bpnt->Bytes) ;
			   ctx->rtStackX -- ;
			   CHECKRECURSERETURN return(bpnt) ;
			 } ;
			v_Msg(ctx,NULL,"IsctEvalPCtx",isctptr->Dim) ; goto iscteval_fail ;
		   case V4DPI_Grouping_Current:
			DIMVAL(bpnt,ctx,isctptr->Dim) ;
			if (bpnt == NULL && isctptr->Continued && ((evalmode & V4DPI_EM_NoContinue) == 0))		/* Have ,continue-point ? */
			 { bpnt = v4dpi_IsctEval(respnt,(P *)&isctptr->Value,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
			 } ;
			if (bpnt != NULL)
			 { 
			   if (bpnt != respnt) memcpy(respnt,bpnt,bpnt->Bytes) ;
			   ctx->rtStackX -- ;
			   CHECKRECURSERETURN return(respnt) ;
			 } ;
			v_Msg(ctx,NULL,"IsctEvalCtx",isctptr->Dim) ; goto iscteval_fail ;
		 } ;
#ifdef NEWQUOTE
	   case V4DPI_PntType_Shell:
		if (ISQUOTED(isctptr))
		 { UNQUOTECOPY(respnt,isctptr) ;
		    ctx->rtStackX -- ;
		   CHECKRECURSERETURN return(respnt) ;
		 } ;
//		if ((evalmode & V4DPI_EM_EvalQuote) == 0 || isctptr->Dim != Dim_UQuote)		/* Not in EvalQuote mode? then just return point */
//		 { ctx->rtStackX -- ;
//		   if (elptr != NULL ? elptr->SkipCnt > 1 : FALSE)
//		    { CHECKRECURSERETURN return(NULL) ; } ;			/* Cannot "skip" when evaluating something other than Isct */
//		   memcpy(respnt,isctptr,isctptr->Bytes) ; CHECKRECURSERETURN return(respnt) ;
//		 } ;
//		isctptr = UNQUOTEPTR(isctptr) ;				/* Grab the 'point' and continue on */
//		if (isctptr->Dim == Dim_UQuote)				/* Only strip off one level of quoting */
//		 { ctx->rtStackX -- ;
//		   if (elptr != NULL ? elptr->SkipCnt > 1 : FALSE)
//		    { CHECKRECURSERETURN return(NULL) ; } ;			/* Cannot "skip" when evaluating something other than Isct */
//		   memcpy(respnt,isctptr,isctptr->Bytes) ; CHECKRECURSERETURN return(respnt) ;
//		 } ;
//		goto eval_pointtype ;
#endif
	   case V4DPI_PntType_BigIsct:
		ipnt = isctptr ; goto got_result ;			/* Have to expand BigIsct! */
	   case V4DPI_PntType_QIsct:
	   case V4DPI_PntType_Isct:
		if (ISQUOTED(isctptr))					/* If point is quoted then just return as its own value */
		 { if ((evalmode & V4DPI_EM_EvalQuote) == 0)		/* NOTE: with new quoting, we should never have quoted Isct or QIsct */
		    { 
		      ctx->rtStackX -- ;
		      UNQUOTECOPY(respnt,isctptr) ; CHECKRECURSERETURN return(respnt) ;
		    } else
		    { isctptr = UNQUOTEPTR(isctptr) ; } ;		/* Grab unquoted portion */
		 } ;
		break ;
	   case V4DPI_PntType_SymDef:
{
#define V4DPI_SYMDEFVAL_MAX 7001
struct lcl__SymDefValues {
  COUNTER numValues ;
  struct {
    SYMID symId ;
    struct V4DPI__Point *valPt ;
  } entry[V4DPI_SYMDEFVAL_MAX] ;
} ;
static struct lcl__SymDefValues *lsdv ;
INDEX hx ;
		
		if (lsdv == NULL) lsdv = (struct lcl__SymDefValues *)v4mm_AllocChunk(sizeof *lsdv,TRUE) ;
		for(hx=(isctptr->Value.SymDef.symId % V4DPI_SYMDEFVAL_MAX);;hx=((hx+1) % V4DPI_SYMDEFVAL_MAX))
		 { if (lsdv->entry[hx].symId == 0 || lsdv->entry[hx].symId == isctptr->Value.SymDef.symId) break ;
//printf("----------------------------------- hash conflict in $xxxxxx %d -> %d\n",hx,((hx+1)%V4DPI_SYMDEFVAL_MAX)) ;
		 } ;
//v_Msg(ctx,UCTBUF2,"@>>>>$%1U -> hx:%2d\n",v4dpi_RevDictEntryGet(ctx,isctptr->Value.SymDef.dictId),hx) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;

		if (isctptr->Bytes > V4DPI_SymDefBase_Bytes)		/* Are we setting value? */
		 { ipnt = v4dpi_IsctEval(respnt,(P *)&isctptr->Value.SymDef.pntBuf,ctx,0,NULL,NULL) ;
		   if (ipnt == NULL) { v_Msg(ctx,NULL,"IsctEvalSymVal",isctptr) ; goto iscteval_fail ; } ;
/*		   At this point ipnt/respnt is new symbol's replacement value. If lsOp is non-zero then adjust replacement value based on lsOp */
		   if (isctptr->Value.SymDef.lsOp != 0)
		    { if (lsdv->entry[hx].symId != isctptr->Value.SymDef.symId)
		       { v_Msg(ctx,NULL,"IsctEvalSymNDef",v4dpi_RevDictEntryGet(ctx,isctptr->Value.SymDef.dictId)) ; goto iscteval_fail ; } ;
/*		      lpnt = symbols current value */
		      if (!(lsdv->entry[hx].valPt->PntType == V4DPI_PntType_Int || lsdv->entry[hx].valPt->PntType == V4DPI_PntType_Real))
		       { v_Msg(ctx,NULL,"DolSymNotNum",isctptr,lsdv->entry[hx].valPt->PntType) ; goto iscteval_fail ; } ;
		      memcpy(&lpnt,lsdv->entry[hx].valPt,lsdv->entry[hx].valPt->Bytes) ;
		      if (ipnt->PntType == V4DPI_PntType_Real || lpnt.PntType == V4DPI_PntType_Real)
		       { LOGICAL ok ; double n1 = v4im_GetPointDbl(&ok,ipnt,ctx), n2 = v4im_GetPointDbl(&ok,(P *)&lpnt,ctx) ;
		         n1 = (isctptr->Value.SymDef.lsOp == 1 ? n1+n2 : n2-n1) ;
			 dblPNTv(ipnt,n1) ;
		       } else
		       { LOGICAL ok ; int n1 = v4im_GetPointInt(&ok,ipnt,ctx), n2 = v4im_GetPointInt(&ok,(P *)&lpnt,ctx) ;
		         n1 = (isctptr->Value.SymDef.lsOp == 1 ? n1+n2 : n2-n1) ;
			 intPNTv(ipnt,n1) ;
		       } ;
		    } ;
		   if (lsdv->entry[hx].valPt == NULL)
		    { if (lsdv->numValues > (COUNTER)(V4DPI_SYMDEFVAL_MAX * 0.9))
		       { v_Msg(ctx,NULL,"IsctEvalSymMax") ; goto iscteval_fail ; } ;
		      lsdv->numValues++ ;
		      lsdv->entry[hx].valPt = (P *)v4mm_AllocChunk(sizeof(P),FALSE) ;
		    } ;
		   memcpy(lsdv->entry[hx].valPt,ipnt,ipnt->Bytes) ; if (ipnt != respnt) memcpy(respnt,ipnt,ipnt->Bytes) ;
		   lsdv->entry[hx].symId = isctptr->Value.SymDef.symId ;
		 } else
		 { if (lsdv->entry[hx].symId != isctptr->Value.SymDef.symId)
		    { v_Msg(ctx,NULL,"IsctEvalSymNDef",v4dpi_RevDictEntryGet(ctx,isctptr->Value.SymDef.dictId)) ; goto iscteval_fail ; } ;
		   memcpy(respnt,lsdv->entry[hx].valPt,lsdv->entry[hx].valPt->Bytes) ;
		 } ;
		ctx->rtStackX -- ; CHECKRECURSERETURN return(respnt) ;
}
		break ;
	 } ;
	if (elptr != NULL)						/* Are we doing a continued search? */
	 { if (elptr->Init)
	    { el = elptr ; el->autoRmvDimCnt = 0 ; isct = isctptr ;
	      memset(fromctx,TRUE,V4DPI_IsctDimMax*sizeof(fromctx[0])) ; /* Set all from-context slots to TRUE */
	      goto continue_continue ;
	    } ;
	   el = elptr ;
	 } else
	 { el = &elbuf ;
	   el->SkipCnt = 0 ; el->ReturnCnt = 0x7fff ;			/* Don't skip any, return all possible */
	 } ;
	el->AreaCnt = 0 ; el->Init = FALSE ; el->autoRmvDimCnt = 0 ;	/* Not yet initialized */

	bpnt = ISCT1STPNT(isctptr) ; 	/* Get first point */
	switch (bpnt->PntType)
	 { default:
		memset(fromctx,FALSE,V4DPI_IsctDimMax*sizeof(fromctx[0])) ;	/* Set all slots to false */
		break ;	
	   case V4DPI_PntType_IntMod:
	   	musteval = 2 ;							/* We gonna have to eval this intersection */
	   	break ;								/* (don't have to set fromctx because not going to reference it) */
	 } ;
/*	Check out intersection for any nested points (or {BINDING} points) - if so then expand */
	if ((!isctptr->NestedIsct) || (musteval == 2)) {  }
	 else { INDEX fx = 0 ;
		isct = &isctbuf ;					/* Have to expand */
		INITISCT2(isct,isctptr) ;
		isct->Grouping = 0 ;
		for((ipnt=ISCT1STPNT(isctptr),ix=0);ix<isctptr->Grouping;(ix++,ADVPNT(ipnt)))
		 { P *cpnt = ipnt ;				/* cpnt = to be copied into intersection (start with original) */
		   if (cpnt->PntType == V4DPI_PntType_BigIsct)
		    { if (ctx->rtStack[ctx->rtStackX].biBuf == NULL)
		       { ctx->rtStack[ctx->rtStackX].biBuf = (struct V4DPI__BigIsct *)v4mm_AllocChunk(sizeof *bibuf,FALSE) ; } ;
		      bibuf = ctx->rtStack[ctx->rtStackX].biBuf ;
		      for(hx=gpi->LowHNum;hx<=gpi->HighHNum;hx++)
		       { if (gpi->RelH[hx].aid == UNUSED) continue ;
			 bibuf->kp.fld.KeyType = V4IS_KeyType_V4 ; bibuf->kp.fld.KeyMode = V4IS_KeyMode_Int ;
			 bibuf->kp.fld.AuxVal = V4IS_SubType_BigIsct ; bibuf->DimUID = ipnt->Value.BigIsct.DimUID ;
			 bibuf->kp.fld.Bytes = V4IS_IntKey_Bytes ;
			 if (v4is_PositionKey(gpi->RelH[hx].aid,(struct V4IS__Key *)bibuf,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey) continue ;
			 bibp = (struct V4DPI__BigIsct *)v4dpi_GetBindBuf(ctx,gpi->RelH[hx].aid,ikde,FALSE,NULL) ;
			 memcpy(bibuf,bibp,bibp->Bytes) ;
			 cpnt = (struct V4DPI__Point *)&bibuf->biPnt ; /* Link to big point */
			 goto bi_entry2 ;
		       } ;
		      v4_error(0,0,"V4DPI","EvalIsct","NOBIGISCT","Could not find BigIsct #%d",bibuf->DimUID) ;
		    } ;
		   if (cpnt->PntType == V4DPI_PntType_Isct || cpnt->PntType == V4DPI_PntType_SymDef)
		    { 
bi_entry2:	      i = cpnt->ForceEval ;					/* Save this flag */
		      if (v4dpi_IsctEval(respnt,cpnt,ctx,V4DPI_EM_Normal,NULL,NULL) == NULL)
		       { if (traceEval)
			 { v_Msg(ctx,UCTBUF2,"@*ArgEvalFail.%1d: %2P\n",curcall,cpnt) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ; } ;
			 if (!isctptr->Continued)
			  { UCsprintf(ctx->ErrorMsg,sizeof ctx->ErrorMsg,UClit("Point #%d failed"),ix+1) ;
			    if ((evalmode & V4DPI_EM_NoRegisterErr) == 0) { REGISTER_ERROR(0) ; } ;
			  } ;
			 ctx->rtStackX -- ;
			 tpnt = ((isctptr->Continued && ((evalmode & V4DPI_EM_NoContinue) == 0)) ?
			  v4dpi_IsctContEval(V4DPI_ContFrom_NestedArgEval,ix+1,respnt,isctptr,ctx,V4DPI_EM_Normal,aidptr,elptr) :
			   v4dpi_IsctEvalFail(respnt,cpnt,ctx,evalmode,NULL,NULL)) ;
			 if (tpnt != NULL && (evalmode & V4DPI_EM_SavedErrSet) == 0) ZUS(gpi->SavedErrMsg) ;	/* If recovered from error then reset saved msg (VEH050517) */
			 CHECKRECURSERETURN return(tpnt) ;
		       } ;
		      if (respnt->PntType == V4DPI_PntType_List && i)		/* Got list and ForceEval (`) then expand args */
		       { lp = ALIGNLP(&respnt->Value) ;
			 for(j=1;;j++)
			  { cpnt = (P *)&isct->Value.Isct.pntBuf[fx] ;
			    if (v4l_ListPoint_Value(ctx,lp,j,cpnt) <= 0) break ;
			    fx += cpnt->Bytes ; isct->Grouping ++ ;
			  } ; ;
			 continue ;
		       } ;
		      cpnt = respnt ;				/* current now is result of intersection! */
		    } ;
		   if (cpnt->PntType == V4DPI_PntType_Special)
		    { switch(cpnt->Grouping)
		       { default:
				v_Msg(ctx,NULL,"IsctEvalCmpPt",ix+1,cpnt) ; goto iscteval_fail ;
		         case V4DPI_Grouping_Undefined:
			 case V4DPI_Grouping_Sample:
			 case V4DPI_Grouping_All:
			 case V4DPI_Grouping_AllCnf:
			 case V4DPI_Grouping_None:
				break ;
			 case V4DPI_Grouping_Now:
				di = v4dpi_DimInfoGet(ctx,cpnt->Dim) ;
				cpnt = respnt ; ZPH(cpnt) ; cpnt->PntType = di->PointType ; cpnt->Bytes = V4PS_Int ; cpnt->Dim = di->DimId ;
				switch(di->PointType)
				 { default:
					v_Msg(ctx,NULL,"IsctEvalNoNow",isctptr) ; goto iscteval_fail ;
				   case V4DPI_PntType_UWeek:
//					i = time(NULL)-(60*gpi->MinutesWest) ; i = TIMEOFFSETDAY + i/VCAL_SecsInDay ;
					cpnt->Value.IntVal = mscu_udate_to_uweek(valUDATEisNOW,di->ds.UWeek.baseUDate) ;
					if (cpnt->Value.IntVal == VCAL_BadVal) { v_Msg(ctx,NULL,"IsctEvalNoNow",isctptr) ; goto iscteval_fail ; } ;
					break ;
				   case V4DPI_PntType_Calendar:
//					d1 = VCal_MJDOffset + TIMEOFFSETDAY +(double)(time(NULL))/(double)VCAL_SecsInDay ;
//					frac = modf(d1,&ipart) ; if (frac == 0.0) d1 += VCAL_MidnightValue ;	/* Want some fractional value to indicate time */
//					PUTREAL(cpnt,d1) ; cpnt->Bytes = V4PS_Calendar ;
					setCALisNOW(d1) ; PUTREAL(cpnt,d1) ; cpnt->Bytes = V4PS_Calendar ;
					break ;
				   case V4DPI_PntType_UDate:
//					cpnt->Value.IntVal = time(NULL)-(60*gpi->MinutesWest) ;
//					cpnt->Value.IntVal = TIMEOFFSETDAY + cpnt->Value.IntVal/VCAL_SecsInDay ;
					cpnt->Value.IntVal = valUDATEisNOW ;
					break ;
				   case V4DPI_PntType_UTime:
//					i = time(NULL)-(60*gpi->MinutesWest) ; d1 = i % VCAL_SecsInDay ;
//					memcpy(&cpnt->Value.IntVal,&d1,sizeof d1) ; break ;			
					setUTIMEisNOW(d1) ; PUTREAL(cpnt,d1) ; break ;
				   case V4DPI_PntType_UYear:
					i = mscu_udate_to_yyyymmdd(VCAL_LOCALNOW) ;
					cpnt->Value.IntVal = i / 10000 ; break ;
				   case V4DPI_PntType_UMonth:
//					i = mscu_udate_to_yyyymmdd(VCAL_LOCALNOW) ;
//					cpnt->Value.IntVal = (i/10000 - VCAL_BaseYear)*12 + ((i/100) % 100)-1 ; break ;
					UDtoUMONTH(cpnt->Value.IntVal,valUDATEisNOW) ; break ;
				   case V4DPI_PntType_UQuarter:
					i = mscu_udate_to_yyyymmdd(VCAL_LOCALNOW) ;
//					cpnt->Value.IntVal = (i/10000 - VCAL_BaseYear)*4 + (((i/100) % 100)-1) / 3 ; break ;
					cpnt->Value.IntVal = YYYYMMDDtoUQTR(i) ; break ;
				   case V4DPI_PntType_UDT:
					cpnt->Value.IntVal = valUDTisNOW ;
					break ;			
				 } ;
				break ;
			 case V4DPI_Grouping_Current:
			 case V4DPI_Grouping_PCurrent:
			      { LOGICAL inCTX ;
				if (cpnt->Grouping == V4DPI_Grouping_Current) { DIMVAL(bpnt,ctx,cpnt->Dim) ; }
				 else { DIMPVAL(bpnt,ctx,cpnt->Dim) ; } ;
				inCTX = (bpnt != NULL) ;
				if (!inCTX)			/* Point not in context, do we have dim*,xxxx ? if so evaluation xxxx */
				 { if (cpnt->Continued && ((evalmode & V4DPI_EM_NoContinue) == 0))
				    { //ctx->rtStackX -- ; VEH150209 - don't think this s/b here. When it goes negative all hell breaks loose.
				      if (isctptr->TraceEval) { v_Msg(ctx,UCTBUF2,"*TraceIsctCtx",isctptr,cpnt->Dim) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ; } ;
/*				      Can't call IsctContEval because this is not an Isct (confuses meaning of Grouping for special points) */
				      bpnt = v4dpi_IsctEval(respnt,(P *)&cpnt->Value,ctx,evalmode,aidptr,elptr) ;
				    } else if (isctptr->Continued && ((evalmode & V4DPI_EM_NoContinue) == 0))
				    { ctx->rtStackX -- ;	/* Don't have dim*,xxx - but parent isct is continued, try to evaluate that */
				      if (isctptr->TraceEval)
				       { v_Msg(ctx,UCTBUF2,"*TraceIsctCtx",isctptr,cpnt->Dim) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ; } ;
				      CHECKRECURSERETURN return(v4dpi_IsctContEval(V4DPI_ContFrom_EvalCurVal,cpnt->Dim,respnt,isctptr,ctx,V4DPI_EM_Normal,aidptr,elptr)) ;
				    } else { v_Msg(ctx,NULL,"IsctEvalCmpPt1",ix+1,cpnt->Dim) ; goto iscteval_fail ;} ;
				   if (bpnt == NULL) { v_Msg(ctx,NULL,"IsctEvalCmpPt1",ix+1,cpnt->Dim) ; goto iscteval_fail ;} ;
				 } ;
				if (bpnt->PntType == V4DPI_PntType_Shell)
				 { bpnt = (P *)&bpnt->Value ; } 	/* If a shell dimension then grab actual point */
/*					Remember so we don't re-add to context below (only if dim*, not dim**) */
				 else { if (cpnt->Dim == bpnt->Dim && inCTX) fromctx[ix] = (cpnt->Grouping == V4DPI_Grouping_Current) ; } ;
				if (bpnt->PntType == V4DPI_PntType_List && cpnt->ForceEval)	/* Got list and ForceEval (`) then expand args */
				 { lp = ALIGNLP(&bpnt->Value) ;
				   for(j=1;;j++)
				    { cpnt = (P *)&isct->Value.Isct.pntBuf[fx] ;
				      if (v4l_ListPoint_Value(ctx,lp,j,cpnt) <= 0) break ;
				      fx += cpnt->Bytes ; isct->Grouping ++ ;
				    } ; ;
				   fromctx[ix] = FALSE ; continue ;
			         } ;
				cpnt = bpnt ;				/* Current now point from context */
			      }
				break ;
//			 case V4DPI_Grouping_PCurrent:
//				DIMPVAL(bpnt,ctx,cpnt->Dim) ;
//				if (bpnt == NULL)
//				 { 
//				   if (isctptr->Continued && ((evalmode & V4DPI_EM_NoContinue) == 0))
//				    { ctx->rtStackX -- ;
////				      CHECKRECURSERETURN return(v4dpi_IsctContEval(V4DPI_ContFrom_EvalPCurVal,cpnt->Dim,respnt,isctptr,ctx,V4DPI_EM_Normal,aidptr,elptr)) ;
///*				      Can't call IsctContEval because this is not an Isct (confuses meaning of Grouping for special points) */
//				      bpnt = v4dpi_IsctEval(respnt,(P *)&cpnt->Value,ctx,evalmode,aidptr,elptr) ;
//				    } ;
//				   if (bpnt == NULL) { v_Msg(ctx,NULL,"IsctEvalPCtx",cpnt->Dim) ; goto iscteval_fail ; } ;
//				 } ;
//				goto current_entry ;
		       } ;
		    } ;
		   if (cpnt->PntType == V4DPI_PntType_Isct)			/* If this point is itself an isct... */
		    { bpnt = cpnt ;						/*  then append each point to current */
		      for((cpnt=ISCT1STPNT(bpnt),i=0);i<bpnt->Grouping;(i++,ADVPNT(cpnt)))
		       { memcpy(&isct->Value.Isct.pntBuf[fx],cpnt,cpnt->Bytes) ;
			 fx += cpnt->Bytes ; isct->Grouping ++ ;
		       } ;
		      fromctx[ix] = FALSE ;					/* Make sure we don't think this was pulled from ctx */
		      continue ;
		    } ;
		   memcpy(&isct->Value.Isct.pntBuf[fx],cpnt,cpnt->Bytes) ; 	/* Copy "current" into intersection */
		   fx += cpnt->Bytes ; isct->Grouping ++ ;
		 } ;
		if (isctptr->Continued) 			/* Ditto for ",[]" */
		 { memcpy(&isct->Value.Isct.pntBuf[fx],ipnt,ipnt->Bytes) ;
		   fx += ipnt->Bytes ; ADVPNT(ipnt) ;
		 } ;
		isct->Bytes = (char *)&isct->Value.Isct.pntBuf[fx] - (char *)isct ;
		if (traceEval)
		 { 
		   v_Msg(ctx,UCTBUF2,"@*SrchIsct.%1d:%2P\n",curcall,isct) ;vout_UCText(VOUT_Trace,0,UCTBUF2) ;
		 } ;
	      } ;
	if (isctptr->TraceEval)
	 { 
	   v_Msg(ctx,UCTBUF2,"@*Eval.%1d:%2P (%3N)\n",curcall,isct,isct) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	 } ;
/*	If first point is IntMod or V3Mod then evaluate directly */
	if (musteval) { ipnt = isct ; goto got_result ; } ;	/* Immediately evaluate */
	evalmode &= (~V4DPI_EM_EvalQuote) ;

	if (!framepush) { framepush = v4ctx_FramePush(ctx,NULL) ; } ;
/*	Now loop thru intersection & add in any points which have potential */
	for((ipnt=ISCT1STPNT(isct),ix=0);ix<isct->Grouping;(ix++,ADVPNT(ipnt)))
	 { if (fromctx[ix]) continue ;					/* If pulled via {Context} then don't bother to add! */
	   if (trace & V4TRACE_ContextAdd)
	    { 
	      v_Msg(ctx,UCTBUF2,"@*CtxAdd.%1d:%2P\n",curcall,ipnt) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	    } ;
	   if (trace & V4TRACE_IsAEval) 				/* If evaling IsA then don't try again! */
	    { if (!v4ctx_FrameAddDim(ctx,0,ipnt,0,0)) goto isctctx_fail ; }
	    else { if (!v4ctx_FrameAddDim(ctx,0,ipnt,0,V4C_CheckIsA+V4C_TraceIsA)) goto isctctx_fail ; } ;
	 } ;
	for(dx=0;dx<ctx->EvalListCnt;dx++)				/* First link to all dimensions in context with potential */
	 { hx = ctx->EvalListDimIndexes[dx] ;				/* Ret hash index into context */
	   cvx = ctx->DimHash[hx].CtxValIndex ;
	   if (cvx == -1) continue ;					/* No point currently */
	   for(i=0;i<ctx->CtxVal[cvx].nblCnt;i++)
	    { if (el->AreaCnt >= V4DPI_EvalList_AreaMax)
	       { for(dx=0;dx<ctx->EvalListCnt;dx++)
	          { hx = ctx->EvalListDimIndexes[dx] ; cvx = ctx->DimHash[hx].CtxValIndex ; if (cvx == -1) continue ;
	            v_Msg(ctx,UCTBUF1,"@  Dim:%1D\n",ctx->DimHash[hx].Dim) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
	          } ;
	         v4_error(V4E_MAXLISTAREA,0,"V4DPI","IsctEval","MAXLISTAREA","Exceeded max number(%d) of context points with bind value areas",V4DPI_EvalList_AreaMax) ;
	       } ;
	      el->Area[el->AreaCnt].AreaId = ctx->CtxVal[cvx].AreaIds[i] ;
	      el->Area[el->AreaCnt++].nbl = ctx->CtxVal[cvx].nbl[i] ;
	    } ;
	   continue ;
	 } ;
/*	Now go see if we can match on something */
continue_continue:
	switch (v4dpi_IsctEvalDoit(el,isct,ctx,evalmode))
	 { case -1:	/* Error - return failure immediately */
		goto iscteval_fail ;
	   case 0:	/* Could not do-it */
		if (isctptr->CondEval) { ipnt = (P *)&CondEvalRet ; goto got_result ; } ;
		if (traceEval)
		 { v_Msg(ctx,UCTBUF2,"@*EvalFail.%1d: %2P\n",curcall,isct) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ; } ;
		if (evalmode & V4DPI_EM_FailOK) goto iscteval_fail ;
		ipnt = NULL ;
		if (isctptr->Continued && ((evalmode & V4DPI_EM_NoContinue) == 0))
		 { if (framepush) { if (!v4ctx_FramePop(ctx,framepush,NULL)) goto isctctx_fail ; framepush = 0 ; } ;
		   ipnt = v4dpi_IsctContEval(V4DPI_ContFrom_NoBinding,0,respnt,isctptr,ctx,evalmode,aidptr,elptr) ;
		 } ;
		if (ipnt == NULL) ipnt = v4dpi_IsctEvalFail(respnt,isct,ctx,evalmode,aidptr,elptr) ;
		if (framepush) { if (!v4ctx_FramePop(ctx,framepush,NULL)) goto isctctx_fail ; framepush = 0 ; } ;
		if (ipnt == NULL && isctptr->Grouping == 1)
		 { tpnt = ISCT1STPNT(isctptr) ;
		   if (tpnt->PntType == V4DPI_PntType_Isct)
		    { v_Msg(ctx,UCTBUF2,"*IsctEvalDblEvl",isctptr) ; vout_UCText(VOUT_Warn,0,UCTBUF2) ; } ;
		 } ;
		if (ipnt == NULL)
		 { if (isctptr->Grouping == 1)
		    { tpnt = ISCT1STPNT(isctptr) ;
		      if (tpnt->PntType == V4DPI_PntType_Isct)
		       { v_Msg(ctx,NULL,"*IsctEvalDblEvl1",isctptr) ; }
		       else { v_Msg(ctx,NULL,"IsctEvalNoBind",isctptr) ; ; } ;
		    }
		    else { v_Msg(ctx,NULL,"IsctEvalNoBind",isctptr) ; } ;
		   ctx->FailProgress = V4FailProgress_NoBind ; goto iscteval_fail ;
		 } ;
		if (ipnt != NULL && (evalmode & V4DPI_EM_SavedErrSet) == 0) ZUS(gpi->SavedErrMsg) ;	/* If recovered from error then reset saved msg (VEH050517) */
		ctx->rtStackX -- ; CHECKRECURSERETURN return(ipnt) ;
	   case 1:	/* Got nblv */
		nblv = el->Resnblv ; break ;
	   case 2:	/* Got pattern */
		ipnt = el->ResPattern ; goto got_result ;
	   case 3:	/* Got resulting point */
		ipnt = el->ResPattern ; goto got_result ;
	 } ;
	if (el->SkipCnt > 0)
	 { el->SkipCnt-- ; goto continue_continue ; } ;
	if ((el->ReturnCnt--) <= 0)
	 { UCstrcpy(ctx->ErrorMsg,UClit("Exceeded max requested number")) ; goto iscteval_fail ;
	 } ;
	ctx->LastBindValAid = el->Area[el->AreaMatch].AreaId ;	/* Save this for possible use by V3 or whatever */
	if (nblv->sp.Bytes == 0)					/* Is value in the short point or in value record */
	 { struct V4DPI__Binding bind,*binding=NULL ;
	   bind.kp.fld.AuxVal = V4IS_SubType_Value ; bind.kp.fld.KeyType = V4IS_KeyType_V4 ;
	   bind.kp.fld.KeyMode = V4IS_KeyMode_Int ; bind.kp.fld.Bytes = V4IS_IntKey_Bytes ; bind.ValueId = nblv->sp.Value.IntVal ;
	   ctx->IsctEvalValueGetCount++ ;
	   hx = ((el->Area[el->AreaMatch].AreaId+5) * bind.ValueId) % V4_IsctEval_CacheMax ; if (hx < 0) hx = -hx ;
	   if (ctx->viec.Entry[hx].AreaId == el->Area[el->AreaMatch].AreaId && ctx->viec.Entry[hx].KeyVal == bind.ValueId)
	    { binding = ctx->viec.Entry[hx].bind ; ctx->IsctEvalValueCache++ ;	/* Got bind value in cache (good job!) */
	    } else
	    { struct V4MM__MemMgmtMaster *mmm ; INDEX areax ;
	      FINDAREAINPROCESS(areax,el->Area[el->AreaMatch].AreaId) ;
	      GRABAREAMTLOCK(areax) ;
	      if (v4is_PositionKey(el->Area[el->AreaMatch].AreaId,(struct V4IS__Key *)&bind,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey)
	       v4_error(V4E_NOVALREC,0,"V4DPI","IsctEval","NOVALREC","Could not access value record") ;
	      binding = (struct V4DPI__Binding *)v4dpi_GetBindBuf(ctx,el->Area[el->AreaMatch].AreaId,ikde,FALSE,NULL) ;
	      if (binding->Bytes <= V4_IsctEval_CacheBytes)		/* If bind not too big then blow into local cache */
	       { ctx->viec.Entry[hx].AreaId = el->Area[el->AreaMatch].AreaId ; ctx->viec.Entry[hx].KeyVal = bind.ValueId ;
		 if (ctx->viec.Entry[hx].bind == NULL)
		  ctx->viec.Entry[hx].bind = (struct V4DPI__Binding *)v4mm_AllocChunk(V4_IsctEval_CacheBytes,FALSE) ;
		 memcpy(ctx->viec.Entry[hx].bind,binding,binding->Bytes) ;
	       } ;
	      FREEAREAMTLOCK(areax) ;
	    } ;
	   if (aidptr != NULL)						/* Maybe just return entire binding as result */
	    { if (framepush) { if (!v4ctx_FramePop(ctx,framepush,NULL)) goto isctctx_fail ; } ;
	      *aidptr = el->Area[el->AreaMatch].AreaId ;		/* Update with aid */
//	      ctx->NestedIsctEval-- ;
	      ctx->rtStackX -- ;
	      CHECKRECURSERETURN return((P *)binding) ;			/* (Probably to append/insert into list) */
	    } ;
	   ipnt = (P *)&binding->Buffer[0] ;
	   if (traceEval)
	    { memcpy(&iterbuf,ipnt,ipnt->Bytes) ; ipnt = &iterbuf ;	/* Save because PointToString may alter io buffers! */
	      v_Msg(ctx,UCTBUF2,"@*IsctVal.%1d:area.value=%2d.%3d- %4P\n",curcall,el->Area[el->AreaMatch].AreaId,bind.ValueId,ipnt) ;
	      vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	    } ;
	 } else 							/* Value is in short point - link up */
	 { 
	   ipnt = respnt ;						/* Expand short to regular point */
	   ZPH(ipnt) ; ipnt->Dim = nblv->sp.Dim ; ipnt->Bytes = nblv->sp.Bytes ; ipnt->PntType = nblv->sp.PntType ;
	   ipnt->LHSCnt = nblv->sp.LHSCnt ;
	   memcpy(&ipnt->Value,&nblv->sp.Value,nblv->sp.Bytes-V4DPI_PointHdr_Bytes) ;
	   if (isctptr->TraceEval && ipnt->PntType != V4DPI_PntType_BigIsct)
	    { memcpy(&iterbuf,ipnt,ipnt->Bytes) ; ipnt = &iterbuf ;	/* Save because PointToString may alter io buffers! */
	      v_Msg(ctx,UCTBUF2,"@*IsctVal.%1d:short pt- %2P\n",curcall,ipnt) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	    } ;
	 } ;
got_result:
	switch ((evalmode & V4DPI_EM_NoNest)!=0 ? UNUSED : ipnt->PntType)
	 { 
	   case V4DPI_PntType_BigIsct:			/* If a "big" isct then read from area & drop down to _Isct: */
		if (ctx->rtStack[ctx->rtStackX].biBuf == NULL)
		 { ctx->rtStack[ctx->rtStackX].biBuf = (struct V4DPI__BigIsct *)v4mm_AllocChunk(sizeof *bibuf,FALSE) ; } ;
		bibuf = ctx->rtStack[ctx->rtStackX].biBuf ;
//		bibuf = (struct V4DPI__BigIsct *)v4mm_AllocChunk(sizeof *bibuf,FALSE) ;
		for(hx=gpi->LowHNum;hx<=gpi->HighHNum;hx++)
		 { if (gpi->RelH[hx].aid == UNUSED) continue ;
		   bibuf->kp.fld.KeyType = V4IS_KeyType_V4 ; bibuf->kp.fld.KeyMode = V4IS_KeyMode_Int ;
		   bibuf->kp.fld.AuxVal = V4IS_SubType_BigIsct ; bibuf->DimUID = ipnt->Value.BigIsct.DimUID ;
		   bibuf->kp.fld.Bytes = V4IS_IntKey_Bytes ;
		   if (v4is_PositionKey(gpi->RelH[hx].aid,(struct V4IS__Key *)bibuf,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey) continue ;
		   bibp = (struct V4DPI__BigIsct *)v4dpi_GetBindBuf(ctx,gpi->RelH[hx].aid,ikde,FALSE,NULL) ;
		   memcpy(bibuf,bibp,bibp->Bytes) ;
		   ipnt = (struct V4DPI__Point *)&bibuf->biPnt ; /* Link to big point */
		   goto bi_entry ;
		 } ;
		v4_error(0,0,"V4DPI","EvalIsct","NOBIGISCT","Could not find BigIsct #%d",bibuf->DimUID) ;
	   case V4DPI_PntType_QIsct:
	   case V4DPI_PntType_Isct:			/* If point we just got is itself an intersection then recursively evaluate */
/*		If we were called with Isct/IntMod (i.e. this isct not result of isct lookup) then don't copy */
		if (!(musteval == 2 || ipnt->Bytes > sizeof iterbuf))
		 { memcpy(&iterbuf,ipnt,ipnt->Bytes) ;	/* Save point (may be in V4IS buffer!) */
		   ipnt = &iterbuf ;
		 } ;
		bibuf = NULL ;
bi_entry:
		bpnt = ISCT1STPNT(ipnt) ;		/* Do quick check on first point in intersection */
		if ((evalmode & V4DPI_EM_NestUV4Fail) != 0)
		 { evalmode &= ~V4DPI_EM_NestUV4Fail ;	/* Only handle this flag once */
		   ZPH(&lpnt) ; lpnt.Dim = Dim_UV4 ; lpnt.PntType = V4DPI_PntType_Special ;
		   lpnt.Bytes = V4DPI_PointHdr_Bytes ; lpnt.Grouping = V4DPI_Grouping_Undefined ;
		   if (!v4ctx_FrameAddDim(ctx,0,(P *)&lpnt,0,0)) goto isctctx_fail ;
		 } ;
		switch (bpnt->PntType)
		 { default:						/* Nothing special - drop below & recurse */
			if (traceEval)
			 { v_Msg(ctx,UCTBUF2,"@*ReEval.%1d:%2P\n",curcall,ipnt) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ; } ;

			ipnt = v4dpi_IsctEval(respnt,ipnt,ctx,evalmode,NULL,NULL) ;
			if (ipnt == NULL)
			 { if (isctptr->Continued && ((evalmode & V4DPI_EM_NoContinue) == 0)) break ;		/* If got ",continued" then drop thru & handle */
			   failisct = &iterbuf ; goto iscteval_fail ;
			 } ;
			break ;
//		   case V4DPI_PntType_V3ModRaw: break ;
		   case V4DPI_PntType_IntMod:
/*			If musteval is TRUE then we are from intmod, don't push it onto stack again 'cause it's already there (VEH061210) */
			if (!musteval) { ++ctx->rtStackX ; ctx->rtStack[ctx->rtStackX].vis = ipnt->Value.Isct.vis ; ctx->rtStack[ctx->rtStackX].isctPtr = ipnt ; } ;
			spnt = v4im_IntModHandler(ipnt,respnt,ctx,evalmode) ;
#ifdef NASTYBUG
			ctx->isctIntMod = NULL ;
#endif
			if (!musteval)
			 { --ctx->rtStackX ; } ;
/*			If spnt still NULL then fall through and try isctptr->Continued, etc. */
//			if (bibuf != NULL) v4mm_FreeChunk(bibuf) ;
			ipnt = spnt ; break ;
		} ;
	 } ;
	if (framepush) { if (!v4ctx_FramePop(ctx,framepush,NULL)) goto isctctx_fail ; framepush = 0 ; } ;
/*	If point has ForceEval attribute & we were given V4DPI_EM_EvalQuote in evalmode then nest-evaluate the result */
	if ((ipnt == NULL ? FALSE : ipnt->ForceEval) && (evalmode & V4DPI_EM_EvalQuote) != 0)
	 { if (ipnt == respnt) { memcpy(&iterbuf,ipnt,ipnt->Bytes) ; ipnt = &iterbuf ; } ;
	   ipnt = v4dpi_IsctEval(respnt,ipnt,ctx,evalmode,aidptr,elptr) ;
	 } ;
	if (ipnt == NULL)
	 { if (traceEval)
	    { UCsprintf(UCTBUF1,V4TMBufMax,UClit("*EvalFAIL.%d\n"),curcall) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
	   if (isctptr->Continued && ((evalmode & V4DPI_EM_NoContinue) == 0)) ipnt = v4dpi_IsctContEval(V4DPI_ContFrom_2ndIsctEval,0,respnt,isctptr,ctx,V4DPI_EM_Normal,aidptr,elptr) ;
	   if (evalmode & V4DPI_EM_FailOK) goto iscteval_fail ;
	   if (ipnt == NULL)
	    { UCCHAR saveErrorMsg[V4TMBufMax] ; UCstrcpy(saveErrorMsg,ctx->ErrorMsg) ;	/* Have to save error because IsctEvalFail, if it fails, will corrupt it (VEH050511) */
	      ipnt = v4dpi_IsctEvalFail(respnt,isct,ctx,evalmode,aidptr,elptr) ;
	      if (ipnt == NULL) UCstrcpy(ctx->ErrorMsg,saveErrorMsg) ;
	    } ;
	   if (ipnt == NULL) goto iscteval_fail ;
	   if ((evalmode & V4DPI_EM_SavedErrSet) == 0) ZUS(gpi->SavedErrMsg) ;		/* If recovered from error then reset saved msg (VEH050517) */
	   ctx->rtStackX -- ;
	   memcpy(respnt,ipnt,ipnt->Bytes) ; CHECKRECURSERETURN return(respnt) ;
	 } ;
	if (ipnt->Dim == 0)
	 { if (ipnt->PntType == V4DPI_PntType_Isct)		/* Return isct as the value! */
	    { memcpy(respnt,ipnt,ipnt->Bytes) ;
	      ctx->rtStackX -- ;
#ifndef NEWQUOTE
	      UNQUOTE(respnt) ;
#endif
	      CHECKRECURSERETURN return(respnt) ;
	    } ;
	   if (ipnt->PntType == V4DPI_PntType_QIsct)		/* Return isct as the value! */
	    { memcpy(respnt,ipnt,ipnt->Bytes) ; ipnt = respnt ;
	      ipnt->PntType = V4DPI_PntType_Isct ;		/* Convert to regular ISCT point (but keep quoted) */
	      ctx->rtStackX -- ;
	      CHECKRECURSERETURN return(ipnt) ;
	    } ;
	   if (traceEval)
	    { UCsprintf(UCTBUF1,V4TMBufMax,UClit("V4-Trace_EvalFAIL.%d - Bad resulting point!\n"),curcall) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; } ;
	   if (isctptr->Continued && ((evalmode & V4DPI_EM_NoContinue) == 0))
	    { /* ctx->NestedIsctEval-- ; */ ctx->rtStackX -- ; CHECKRECURSERETURN return(v4dpi_IsctContEval(V4DPI_ContFrom_NoDim,0,respnt,isctptr,ctx,V4DPI_EM_Normal,aidptr,elptr)) ; }
	    else goto iscteval_fail ;
	 } ;
	if (traceEval || isctptr->TraceEval)
	 { v_Msg(ctx,UCTBUF2,"@*EvalRes.%1d:%2P\n",curcall,ipnt) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ; } ;
	ctx->rtStackX -- ;
	for(i=0;(el == NULL ? FALSE : i < el->autoRmvDimCnt);i++)
	 { ZPH(&lpnt) ; lpnt.Dim = el->autoRmvDimList[i] ; lpnt.PntType = V4DPI_PntType_Special ; lpnt.Grouping = V4DPI_Grouping_Undefined ; lpnt.Bytes = V4DPI_PointHdr_Bytes ;
	   if (!v4ctx_FrameAddDim(ctx,V4C_FrameId_Real,(P *)&lpnt,0,0)) goto isctctx_fail ;
	 } ;
	if (ipnt != respnt) memcpy(respnt,ipnt,ipnt->Bytes) ; CHECKRECURSERETURN return(respnt) ;

/*	Here on evaluation failure */
iscteval_fail:
	if (framepush) v4ctx_FramePop(ctx,framepush,NULL) ;		/* Maybe pop off temp frame for this intersection */
	if (traceEval || isctptr->TraceEval)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"@*EvalFail.%1d:%2P [%3U]\n",curcall,isctptr,ctx->ErrorMsg) ; vout_UCText(VOUT_Trace,0,ctx->ErrorMsgAux) ; } ;
	if (evalmode & V4DPI_EM_FailOK)
	 { ctx->rtStackX -- ; CHECKRECURSERETURN return(NULL) ; } ;
	if (failisct != NULL)
	 { ctx->FailProgress = V4FailProgress_NestEval ; v_Msg(ctx,NULL,"IsctEvalNstFail",isctptr,failisct) ; } ;
	if ((evalmode & V4DPI_EM_NoRegisterErr) == 0) { REGISTER_ERROR(0) ; } ;
	ctx->rtStackX -- ; CHECKRECURSERETURN return(NULL) ;

isctctx_fail:
	v_Msg(ctx,NULL,"IsctEvalCtxErr",isctptr) ; goto iscteval_fail ;

}

/*	v4dpi_IsctEvalFail - Last ditch routine to attempt evaluation of isct	*/

struct V4DPI__Point *v4dpi_IsctEvalFail(point,isctptr,ctx,evalmode,aidptr,elptr)
  struct V4DPI__Point *point ;
  struct V4DPI__Point *isctptr ;
  struct V4C__Context *ctx ;
  int evalmode,*aidptr ;
  struct V4DPI__EvalList *elptr ;
{ P *ipt,*tpt,v4pt ;
  enum DictionaryEntries deval ;
  int ix,framepush ; char tbi[512] ;
  int savertStackFail,savertStackX ;

	if (evalmode & V4DPI_EM_NoIsctFail) return(NULL) ;	/* Don't want to mess with this */
	ipt=ISCT1STPNT(isctptr) ;				/* Go thru all points & put into context */
	if (ipt->PntType == V4DPI_PntType_IntMod) return(NULL) ;/* Evaling an IntMod - can't really do last ditch */
	framepush = v4ctx_FramePush(ctx,NULL) ;
	savertStackFail = ctx->rtStackFail ; savertStackX = ctx->rtStackX ;
	ctx->rtStackX = (ctx->rtStackFail > ctx->rtStackX ? ctx->rtStackFail+1 : ctx->rtStackX+1) ;
	ctx->InIsctFail = TRUE ;				/* Set this so other routines know where we be */
	for(ix=0;ix<isctptr->Grouping;ix++,ADVPNT(ipt))
	 { if (ipt->PntType != V4DPI_PntType_Isct) { v4ctx_FrameAddDim(ctx,0,ipt,0,0) ; continue ; } ;
	   tpt = v4dpi_IsctEval(&v4pt,ipt,ctx,V4DPI_EM_NoIsctFail,aidptr,elptr) ;
	   if (tpt == NULL) { ipt = NULL ; goto done ; } ;
	   v4ctx_FrameAddDim(ctx,0,tpt,0,0) ;			/* If arg is isct then eval it */
	 } ;
	INITISCT(&v4pt) ; v4pt.Value.Isct.vis = isctptr->Value.Isct.vis ; v4pt.Grouping = 1 ;			/* Construct [UV4:IsctFail] intersection */
	v4pt.Value.Isct.vis = isctptr->Value.Isct.vis ;	/* Copy "source" from offending isct */
	ipt = ISCT1STPNT(&v4pt) ; dictPNTv(ipt,Dim_UV4,v4im_GetEnumToDictVal(ctx,deval=_IsctFail,UNUSED)) ; v4pt.Bytes += ipt->Bytes ; 
	if (ipt->Dim == 0 || ipt->Value.IntVal == 0)
	 { v4ctx_FramePop(ctx,framepush,NULL) ; ipt = NULL ; goto done ; } ;
	if (traceGlobal & V4TRACE_IsctFail)
	 { v_Msg(ctx,UCTBUF2,"@*Isct: Eval of [UV4:IsctFail] for \"%1P\"\n",tbi) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	 } ;
	ipt = v4dpi_IsctEval(point,&v4pt,ctx,(V4DPI_EM_NoRegisterErr | V4DPI_EM_NoIsctFail | V4DPI_EM_NestUV4Fail),aidptr,elptr) ;
	if (traceGlobal & V4TRACE_IsctFail)
	 { 
	   if (ipt == NULL)
	    { v_Msg(ctx,UCTBUF2,"@*Isct: [UV4:IsctFail] of \"%1P\" -> *Undefined [%2U]*",isctptr,ctx->ErrorMsg) ;
	     } else
	     { 
	       v_Msg(ctx,UCTBUF2,"@*Isct: [UV4:IsctFail] of \"%1P\" -> %2P",tbi,ipt) ;
	     } ;
	   vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	 } ;
	if (!v4ctx_FramePop(ctx,framepush,NULL)) { v_Msg(ctx,NULL,"IsctEvalCtxErr",isctptr) ; return(NULL) ; } ;
done:
	ctx->rtStackFail = savertStackFail ; ctx->rtStackX = savertStackX ;
	ctx->InIsctFail = FALSE ;				/* Turn off this flag */
	return(ipt) ;
}

/*	v4dpi_MaxNesting - Called when evaluator (IsctEval or IntMod) nestes too deeply	*/
/*	Call: point = v4dpi_MaxNesting( ctx , respnt , isct , dideval )
	  where point is returned result (NULL if nothing to return),
		ctx is context,
		respnt is pointer to resulting point buffer,
		isct is offending intersection,
		dideval is *int updated to TRUE if isct eval'ed, FALSE if none		*/

struct V4DPI__Point *v4dpi_MaxNesting(ctx,respnt,isct,dideval)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *respnt ;
  struct V4DPI__Point *isct ;
  LOGICAL *dideval ;
{ P *ipt,v4pt ;
  enum DictionaryEntries deval ;
  int framepush ;

	*dideval = FALSE ;
	framepush = v4ctx_FramePush(ctx,NULL) ;

	INITISCT(&v4pt) ; v4pt.Value.Isct.vis = isct->Value.Isct.vis ; v4pt.Grouping = 2 ;				/* Construct [UV4:MaxNest UDim:isct] intersection */
	ipt = ISCT1STPNT(&v4pt) ; dictPNTv(ipt,Dim_UV4,v4im_GetEnumToDictVal(ctx,deval=_MaxNest,UNUSED)) ;
	if (ipt->Dim == 0 || ipt->Value.IntVal == 0) { v4ctx_FramePop(ctx,framepush,NULL) ; return(NULL) ; } ;

	ADVPNT(ipt) ; ZPH(ipt) ; ipt->PntType = V4DPI_PntType_Shell ; ipt->Dim = Dim_UDim ;
	if (ipt->Dim == 0) { v4ctx_FramePop(ctx,framepush,NULL) ; return(NULL) ; } ;
	memcpy(&ipt->Value,isct,isct->Bytes) ; ipt->Bytes = V4DPI_PointHdr_Bytes + isct->Bytes ;

	ADVPNT(ipt) ; ISCTLEN(&v4pt,ipt) ;
	gpi->NestMax = TRUE ;
	ipt = v4dpi_IsctEval(respnt,&v4pt,ctx,V4DPI_EM_NoIsctFail,NULL,NULL) ;
	if (!v4ctx_FramePop(ctx,framepush,NULL)) return(NULL) ;
	if (gpi->NestMax)			/* If this has been reset via V4(NoError?) then return OK */
	 { gpi->NestMax = FALSE ; return(NULL) ; }
	*dideval = TRUE ;			/* NestMax routine called V4(NoError?) - all is well! */
	return(ipt) ;
}

/*	v4dpi_IsctContEval - Continue IsctEval with next isct in [xxx],[xxx],... list	*/
/*	Call: same as v4dpi_IsctEval - just figures out next isct and recurses back	*/

struct V4DPI__Point *v4dpi_IsctContEval(srccode,srccodeaux,point,isctptr,ctx,evalmode,aidptr,elptr)
  int srccode,srccodeaux ;
  struct V4DPI__Point *point ;
  struct V4DPI__Point *isctptr ;
  struct V4C__Context *ctx ;
  int *aidptr ;
  struct V4DPI__EvalList *elptr ;
{ struct V4DPI__Point *ipnt ;
  int ix ; int savertStackX ;

/*	Have to position to next isct after current one */
	for((ipnt=ISCT1STPNT(isctptr),ix=0);ix<isctptr->Grouping;(ix++,ADVPNT(ipnt))) { } ;
	if ((traceGlobal & V4TRACE_Errors) != 0 || ipnt->TraceEval)
	 { 
	   switch (srccode)
	    { 
	      case V4DPI_ContFrom_NestedArgEval:	v_Msg(ctx,UCTBUF2,"TraceRegErrNest",srccodeaux) ; break ;
	      case V4DPI_ContFrom_EvalCurVal:		v_Msg(ctx,UCTBUF2,"TraceRegErrDim",srccodeaux) ; break ;
	      case V4DPI_ContFrom_EvalPCurVal:		v_Msg(ctx,UCTBUF2,"TraceRegErrPDim",srccodeaux) ; break ;
	      case V4DPI_ContFrom_IntModFail:		v_Msg(ctx,UCTBUF2,"TraceRegErrIMod",srccodeaux) ; break ;
	      case V4DPI_ContFrom_2ndIsctEval:
	      case V4DPI_ContFrom_NoDim:
	      case V4DPI_ContFrom_NoBinding:		v_Msg(ctx,UCTBUF2,"TraceRegDfltMsg",ctx->ErrorMsg) ; break ;
	    } ;
	   v_Msg(ctx,UCTBUF1,"*TraceRegErrCont",UCTBUF2) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
	   v4trace_ExamineState(ctx,isctptr,V4_EXSTATE_All,VOUT_Trace) ;
	 } ;
	savertStackX = ctx->rtStackX ;
	ctx->rtStackX = (ctx->rtStackFail > ctx->rtStackX ? ctx->rtStackFail+1 : ctx->rtStackX+1) ;
	ipnt = v4dpi_IsctEval(point,ipnt,ctx,evalmode,aidptr,elptr) ;
	ctx->rtStackX = savertStackX ;
	if (ipnt != NULL && ipnt != point) { memcpy(point,ipnt,ipnt->Bytes) ; ipnt = point ; } ;
	return(ipnt) ;
}


int v4dpi_IsctEvalDoit(el,isct,ctx,evalmode)
 struct V4DPI__EvalList *el ;
 struct V4DPI__Point *isct ;
 struct V4C__Context *ctx ;
 int evalmode ;
{ struct V4DPI__BindList *nbl ;
  struct V4DPI__BindList_Value *nblv ;
  struct V4DPI__BindList_Dim *nbld ;
  struct V4DPI__Point *ipnt,*dpnt,*tpnt ;
  struct V4DPI__Point_IntMix *pim ;
  struct V4DPI__Point_AlphaMix *pam ;
  struct V4DPI__Point_RealMix *prm ;
  struct V4DPI__DimInfo *di ;
  int oldax,topax,topwgt,lhcnt ; double dnum,d1,d2 ; B64INT b641,b642 ;
  int i,ax,ix,dx,dex ; char *cbuf,*tbuf ; UCCHAR *ucbuf,*utbuf ;

/*	Make sure el is set up ok */
	if (!el->Init)
	 { for(ax=0;ax<el->AreaCnt;ax++)
	    { el->Area[ax].nblv = (BLV *)&el->Area[ax].nbl->Buffer[el->Area[ax].nbl->ValueStart] ;
	      el->Area[ax].nbld = (struct V4DPI__BindList_Dim *)&el->Area[ax].nbl->Buffer[el->Area[ax].nbl->DimStart] ;
	      memset(&el->Area[ax].DimTest,0,el->Area[ax].nbl->PointCnt) ;
	      el->Area[ax].Valx = 0 ; el->Area[ax].CurWgt = el->Area[ax].nblv->BindWgt ;
	    } ; el->Init = TRUE ;
	 } ;
	oldax = -1 ;
	for(;;)
	 {
	   topwgt = 0 ; topax = 0 ;
	   for(ax=0;ax<el->AreaCnt;ax++)
	    { if (el->Area[ax].CurWgt >= 0)
	       { if (el->Area[ax].CurWgt > topwgt) { topax = ax ; topwgt = el->Area[ax].CurWgt ; } ;
		 continue ;
	       } ;
	      if (el->Area[ax].Valx >= (int)(el->Area[ax].nbl->ValueCnt)-1)
	       { el->Area[ax].CurWgt = 0 ; continue ; } ;		/* End of value points for this area */
	      el->Area[ax].nblv = (BLV *)((char *)el->Area[ax].nblv + el->Area[ax].nblv->Bytes) ;
	      el->Area[ax].CurWgt = el->Area[ax].nblv->BindWgt ; el->Area[ax].Valx ++ ;
	      if (el->Area[ax].CurWgt > topwgt) { topax = ax ; topwgt = el->Area[ax].CurWgt ; } ;
	    } ;
	   if (topwgt == 0) return(0) ; 			/* No points match ! */
	   nbl = el->Area[topax].nbl ; nbld = el->Area[topax].nbld ; nblv = el->Area[topax].nblv ;
	   el->AreaMatch = topax ;					/* Assume this is the one which will match */
	   if ((oldax != topax) && (traceGlobal & V4TRACE_BindList))
	    { oldax = topax ;
	      UCsprintf(UCTBUF1,V4TMBufMax,UClit("*TopNBL: Wgt=%d, AX=%d, AreaCnt=%d\n"),topwgt,topax,el->AreaCnt) ;
	      vout_UCText(VOUT_Trace,0,UCTBUF1) ;
	      v4dpi_ExamineBindList(nbl,ctx,el->Area[topax].AreaId) ;
	    } ;
	   lhcnt = 0 ;							/* Reset number of matches on LHS of "|" */
	   memset(&el->MatchedDims,0,V4DPI_IsctDimMax) ;
	   if (ctx->bpm != NULL)
	    { ctx->bpm->PntType = V4DPI_PntType_Isct ; ctx->bpm->Bytes = 0 ; ctx->bpm->Grouping = 0 ; } ;	/* Track matched binding point ? */
#ifdef WANTPATTERNMATCH
	   if (nblv->BindFlags & V4DPI_BindLM_Pattern)			/* Is this a pattern? */
	    { tpnt = v4dpi_MatchPattern(isct,nbl,nblv,ctx,el->Area[topax].AreaId,traceGlobal) ;
	      if (tpnt == NULL) goto no_match ;
	      if (tpnt->Grouping > 1)					/* Have an isct to be eval'ed ? */
	       { el->ResPattern = tpnt ; return(2) ;			/* Return new isct to be eval'ed */
	       } else { el->ResPattern = ISCT1STPNT(tpnt) ; return(3) ; } ;	/* Return resulting point */
	    } ;
#endif
	   if (traceGlobal & V4TRACE_PointCompare)
	    { UCstrcpy(UCTBUF1,UClit("*Testing: [ ")) ;
	      for(dx=0;dx<nblv->IndexCnt;dx++)
	       { dex = nblv->DimPos[dx] ;					/* Get index into dimension point */
		 dpnt = (P *)((char *)&nbl->Buffer[nbl->PointStart + nbld->DimEntryIndex[dex]]) ;
		 v4dpi_PointToString(UCTBUF2,dpnt,ctx,V4DPI_FormatOpt_Trace) ; UCstrcat(UCTBUF1,UCTBUF2) ; UCstrcat(UCTBUF1,UClit(" ")) ;
	       } ;
	      UCstrcat(UCTBUF1,UClit("]\n")) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
	      v4ctx_ExamineCtx(ctx,FALSE,VOUT_Trace) ;
	    } ;
	   el->autoRmvDimCnt = 0 ;					/* Reset possible list of dimensions to auto-remove from ctx */
	   for(dx=0;dx<nblv->IndexCnt;dx++)
	    { dex = nblv->DimPos[dx] ;					/* Get index into dimension point */
	      if (el->Area[topax].DimTest[dex] < 0)			/* If < 0 then point already failed match */
	       { el->Area[topax].CurWgt = -1 ;
		 if (traceGlobal & V4TRACE_PointCompare)
		  { dpnt = (P *)((char *)&nbl->Buffer[nbl->PointStart + nbld->DimEntryIndex[dex]]) ;
//		    v4dpi_PointToString(tb1,dpnt,ctx,-1) ;
		    v_Msg(ctx,UCTBUF1,"@*  Point #%1d (%2P) fails via prior match\n",dx+1,dpnt) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
		  } ;
		 break ;						/* Changed to break?? veh/9-Oct-96 */
		 continue ;
	       } ;
	      dpnt = (P *)((char *)&nbl->Buffer[nbl->PointStart + nbld->DimEntryIndex[dex]]) ;
	      if (dpnt->AltDim) el->autoRmvDimList[el->autoRmvDimCnt++] = dpnt->Dim ;
	      if (ctx->bpm != NULL)					/* If tracking matched point, then track! */
	       { INDEX i ; P *tpt = ISCT1STPNT(ctx->bpm) ;
	         for(i=0;i<ctx->bpm->Grouping;i++) { ADVPNT(tpt) ; } ;
	         memcpy(tpt,dpnt,dpnt->Bytes) ;
	         ADVPNT(tpt) ; ISCTLEN(ctx->bpm,tpt) ; 
		 ctx->bpm->Grouping ++ ;
	       } ;
/*	      Check context for match on point (all points from isct have been blasted into context!) */
	      DIMVAL(ipnt,ctx,dpnt->Dim) ;
	      if (ipnt == NULL)
	       { if (dpnt->PntType == V4DPI_PntType_Time) goto got_match ; /* No time in context- assume OK! */
		 if (dpnt->PntType == V4DPI_PntType_Special && dpnt->Grouping == V4DPI_Grouping_Undefined)
		  goto got_match ;					/* Wanted to match on "no point" */
		 if (evalmode & V4DPI_EM_NoCTXisAll)
		  goto got_match ;					/* Not in context -> assume a match! */
		 el->Area[topax].CurWgt = -1 ;				/* This point don't hack it - try again */
		 if (traceGlobal & V4TRACE_PointCompare)
		  { di = v4dpi_DimInfoGet(ctx,dpnt->Dim) ;
		    v_Msg(ctx,UCTBUF1,"@*  Compare: %1D* not in context - Isct FAILS match\n",di->DimId) ;
		    vout_UCText(VOUT_Trace,0,UCTBUF1) ;
		  } ;
		 break ;						/* Dimension not in context - you roose */
	       } ;
	      for((tpnt=ISCT1STPNT(isct),ix=0);ix<isct->Grouping;(ix++,ADVPNT(tpnt)))
	       { if (dpnt->Dim != tpnt->Dim) continue ; 		/* Do dimensions match ? */
		 el->MatchedDims[ix] = (dpnt->Grouping == V4DPI_Grouping_Single ? 1 : 2) ;	/* Remember position matched */
		 if (isct->LHSCnt == 0 ? TRUE : ix < isct->LHSCnt) lhcnt ++ ;	/* Got match on LHS of "|" */
		 goto check_points ;
	       } ;
	      if( i = v4ctx_DimBaseDim(ctx,dpnt->Dim,traceGlobal) )		/* Maybe matching via IsA dim - find base dim & try */
	       { for((tpnt=ISCT1STPNT(isct),ix=0);ix<isct->Grouping;(ix++,ADVPNT(tpnt)))
		  { if (i != tpnt->Dim) continue ;			/* Do dimensions match ? */
		    el->MatchedDims[ix] = (dpnt->Grouping == V4DPI_Grouping_Single ? 1 : 2) ;	/* Remember position matched */
		    if (isct->LHSCnt == 0 ? TRUE : ix < isct->LHSCnt) lhcnt ++ ;	/* Got match on LHS of "|" */
		    goto check_points ;
		  } ;
	       } ;
/*	      See if dpnt (dimension point) in some way matches with intersection/context (ipnt) point */
check_points:
	      if (el->Area[topax].DimTest[dex] > 0)		/* Already tested this dim point against context- OK */
	       {
		 if (traceGlobal & V4TRACE_PointCompare)
		  { dpnt = (P *)((char *)&nbl->Buffer[nbl->PointStart + nbld->DimEntryIndex[dex]]) ;
		    v_Msg(ctx,UCTBUF1,"@*  Point #%1d (%2P) OK via prior match\n",dx+1,dpnt) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
		  } ;
		 goto got_match ;
	       } ;
/*	      If intersection point is special then check it out */
	      if (ipnt->PntType == V4DPI_PntType_Special)
	       { switch (ipnt->Grouping)
		  { default: v_Msg(ctx,NULL,"@Unknown special point type in intersection (%1P)",ipnt) ; return(-1) ;
		    case V4DPI_Grouping_All:		goto got_match ;
		    case V4DPI_Grouping_AllCnf:		goto got_match ;	/* VEH050521 - Changed from break to "goto got_match" */
		    case V4DPI_Grouping_Sample: 	break ;
		    case V4DPI_Grouping_Undefined:	break ;
		    case V4DPI_Grouping_None:		break ;
		    case V4DPI_Grouping_PCurrent:
			DIMPVAL(ipnt,ctx,dpnt->Dim) ;
			if (ipnt == NULL) goto no_match ; break ;
		    case V4DPI_Grouping_Current:
			DIMVAL(ipnt,ctx,dpnt->Dim) ;
			if (ipnt == NULL) goto no_match ; break ;
		  } ;
	       } ;
match_start:
	      switch (dpnt->PntType)
	       { default: 
			v_Msg(ctx,NULL,"IsctEvalPntType",dpnt,isct) ; return(-1) ;
		 case V4DPI_PntType_Special:
			switch (dpnt->Grouping)
			 { default: v_Msg(ctx,NULL,"@Unsupported special point type in binding (%1P)",dpnt) ; return(-1) ;
			   case V4DPI_Grouping_AllCnf:
			   case V4DPI_Grouping_All:	goto got_match ;	/* Matches all points! */
			   case V4DPI_Grouping_Sample:
				if (ipnt->PntType == V4DPI_PntType_Special && ipnt->Grouping == V4DPI_Grouping_Sample) goto got_match ;
				goto no_match ;
			   case V4DPI_Grouping_None:
				if (ipnt->PntType == V4DPI_PntType_Special && ipnt->Grouping == V4DPI_Grouping_None) goto got_match ;
				goto no_match ;
			   case V4DPI_Grouping_Undefined:
				goto no_match ; 			/* If here then have context point, not UNDEFINED! */
			 } ; break ;
		 case V4DPI_PntType_Shell:	/* A shell, just grab contained points & try again! */
			dpnt = (P *)&dpnt->Value ; ipnt = (P *)&ipnt->Value ;
			goto match_start ;
		 case V4DPI_PntType_Complex:
			if (dpnt->Grouping == V4DPI_Grouping_Single)
			 { if (memcmp(&dpnt->Value.Complex,&ipnt->Value.Complex,sizeof(ipnt->Value.Complex))==0) goto got_match ; } ;
			break ;
		 case V4DPI_PntType_GeoCoord:
			if (dpnt->Grouping == V4DPI_Grouping_Single)
			 { if (memcmp(&dpnt->Value.GeoCoord,&ipnt->Value.GeoCoord,sizeof(ipnt->Value.GeoCoord))==0) goto got_match ; } ;
			break ;
		 case V4DPI_PntType_MemPtr:
			if (dpnt->Grouping == V4DPI_Grouping_Single)
			 { if (memcmp(&dpnt->Value.MemPtr,&ipnt->Value.MemPtr,sizeof(ipnt->Value.MemPtr))==0) goto got_match ; } ;
			break ;
		 case V4DPI_PntType_UOM:
			if (dpnt->Value.UOMVal.Ref != ipnt->Value.UOMVal.Ref) break ;
			switch (dpnt->Grouping)
			 { case V4DPI_Grouping_Single:
				if (memcmp(&dpnt->Value.UOMVal.Num,&ipnt->Value.UOMVal.Num,sizeof(ipnt->Value.UOMVal.Num))==0) goto got_match ; break ;
			   case V4DPI_Grouping_GT:
				memcpy(&d1,&ipnt->Value.UOMVal.Num,sizeof(double)) ; memcpy(&d2,&dpnt->Value.UOMVal.Num,sizeof(double)) ;
				if (d1 > d2) goto got_match ; break ;
			   case V4DPI_Grouping_GE:
				memcpy(&d1,&ipnt->Value.UOMVal.Num,sizeof(double)) ; memcpy(&d2,&dpnt->Value.UOMVal.Num,sizeof(double)) ;
				if (d1 >= d2) goto got_match ; break ;
			   case V4DPI_Grouping_LT:
				memcpy(&d1,&ipnt->Value.UOMVal.Num,sizeof(double)) ; memcpy(&d2,&dpnt->Value.UOMVal.Num,sizeof(double)) ;
				if (d1 < d2) goto got_match ; break ;
			   case V4DPI_Grouping_LE:
				memcpy(&d1,&ipnt->Value.UOMVal.Num,sizeof(double)) ; memcpy(&d2,&dpnt->Value.UOMVal.Num,sizeof(double)) ;
				if (d1 <= d2) goto got_match ; break ;
			   case V4DPI_Grouping_NE:
				if (memcmp(&dpnt->Value.UOMVal.Num,&ipnt->Value.UOMVal.Num,sizeof(ipnt->Value.UOMVal.Num))!=0) goto got_match ; break ;
			 } ;
			break ;
		 case V4DPI_PntType_UOMPer:
			if (dpnt->Value.UOMPerVal.Ref != ipnt->Value.UOMPerVal.Ref) break ;
			if (memcmp(&dpnt->Value.UOMPerVal.Amount,&ipnt->Value.UOMPerVal.Amount,sizeof(ipnt->Value.UOMPerVal.Amount))!=0) break ;
			switch (dpnt->Grouping)
			 { case V4DPI_Grouping_Single:
				if (memcmp(&dpnt->Value.UOMPerVal.Amount,&ipnt->Value.UOMPerVal.Amount,sizeof(ipnt->Value.UOMPerVal.Amount))==0) goto got_match ; break ;
			   case V4DPI_Grouping_GT:
				memcpy(&d1,&ipnt->Value.UOMPerVal.Amount,sizeof(double)) ; memcpy(&d2,&dpnt->Value.UOMPerVal.Amount,sizeof(double)) ;
				if (d1 > d2) goto got_match ; break ;
			   case V4DPI_Grouping_GE:
				memcpy(&d1,&ipnt->Value.UOMPerVal.Amount,sizeof(double)) ; memcpy(&d2,&dpnt->Value.UOMPerVal.Amount,sizeof(double)) ;
				if (d1 >= d2) goto got_match ; break ;
			   case V4DPI_Grouping_LT:
				memcpy(&d1,&ipnt->Value.UOMPerVal.Amount,sizeof(double)) ; memcpy(&d2,&dpnt->Value.UOMPerVal.Amount,sizeof(double)) ;
				if (d1 < d2) goto got_match ; break ;
			   case V4DPI_Grouping_LE:
				memcpy(&d1,&ipnt->Value.UOMPerVal.Amount,sizeof(double)) ; memcpy(&d2,&dpnt->Value.UOMPerVal.Amount,sizeof(double)) ;
				if (d1 <= d2) goto got_match ; break ;
			   case V4DPI_Grouping_NE:
				if (memcmp(&dpnt->Value.UOMPerVal.Amount,&ipnt->Value.UOMPerVal.Amount,sizeof(ipnt->Value.UOMPerVal.Amount))!=0) goto got_match ; break ;
			 } ;
			break ;
		 case V4DPI_PntType_Fixed:
			switch (dpnt->Grouping)
			 { case V4DPI_Grouping_Single:
				if (memcmp(&dpnt->Value.FixVal,&ipnt->Value.FixVal,sizeof(ipnt->Value.FixVal))==0) goto got_match ; break ;
			   case V4DPI_Grouping_GT:
				memcpy(&b641,&ipnt->Value.FixVal,sizeof(double)) ; memcpy(&b642,&dpnt->Value.FixVal,sizeof(double)) ;
				if (b641 > b642) goto got_match ; break ;
			   case V4DPI_Grouping_GE:
				memcpy(&b641,&ipnt->Value.FixVal,sizeof(double)) ; memcpy(&b642,&dpnt->Value.FixVal,sizeof(double)) ;
				if (b641 >= b642) goto got_match ; break ;
			   case V4DPI_Grouping_LT:
				memcpy(&b641,&ipnt->Value.FixVal,sizeof(double)) ; memcpy(&b642,&dpnt->Value.FixVal,sizeof(double)) ;
				if (b641 < b642) goto got_match ; break ;
			   case V4DPI_Grouping_LE:
				memcpy(&b641,&ipnt->Value.FixVal,sizeof(double)) ; memcpy(&b642,&dpnt->Value.FixVal,sizeof(double)) ;
				if (b641 <= b642) goto got_match ; break ;
			   case V4DPI_Grouping_NE:
				if (memcmp(&dpnt->Value.FixVal,&ipnt->Value.FixVal,sizeof(ipnt->Value.FixVal))!=0) goto got_match ; break ;
			 } ;
			break ;
		 case V4DPI_PntType_UTime:
		 case V4DPI_PntType_Calendar:
		 case V4DPI_PntType_Real:
			switch (dpnt->Grouping)
			 { default:				/* Have a list or range or whatever - do it */
				prm = (struct V4DPI__Point_RealMix *)&dpnt->Value ;
				for(i=0;i<dpnt->Grouping;i++)
				 { memcpy(&d1,&prm->Entry[i].BeginReal,SIZEofDOUBLE) ; memcpy(&d2,&prm->Entry[i].EndReal,SIZEofDOUBLE) ;
				   GETREAL(dnum,ipnt) ;
				   if (dnum >= d1 && dnum <= d2) goto got_match ;
				 } ; break ;
			   case V4DPI_Grouping_Single:
				if (memcmp(&dpnt->Value.RealVal,&ipnt->Value.RealVal,sizeof(ipnt->Value.RealVal))==0) goto got_match ; break ;
			   case V4DPI_Grouping_GT:
				GETREAL(d1,ipnt) ; GETREAL(d2,dpnt) ;
				if (d1 > d2) goto got_match ; break ;
			   case V4DPI_Grouping_GE:
				GETREAL(d1,ipnt) ; GETREAL(d2,dpnt) ;
				if (d1 >= d2) goto got_match ; break ;
			   case V4DPI_Grouping_LT:
				GETREAL(d1,ipnt) ; GETREAL(d2,dpnt) ;
				if (d1 < d2) goto got_match ; break ;
			   case V4DPI_Grouping_LE:
				GETREAL(d1,ipnt) ; GETREAL(d2,dpnt) ;
				if (d1 <= d2) goto got_match ; break ;
			   case V4DPI_Grouping_NE:
				if (memcmp(&dpnt->Value.RealVal,&ipnt->Value.RealVal,sizeof(ipnt->Value.RealVal))!=0) goto got_match ; break ;
			 } ;
			break ;
		 case V4DPI_PntType_AggRef:
			if (ipnt->Value.IntVal == dpnt->Value.IntVal ? ipnt->Grouping == dpnt->Grouping : FALSE) goto got_match ;
			break ;
		 CASEofINT
		 case V4DPI_PntType_BigText:
		 case V4DPI_PntType_IntMod:
		 case V4DPI_PntType_PntIdx:
		 case V4DPI_PntType_SSVal:
		 case V4DPI_PntType_Color:
		 case V4DPI_PntType_Country:
		 case V4DPI_PntType_Tree:
		 case V4DPI_PntType_XDict:
		 case V4DPI_PntType_Dict:
			 switch (dpnt->Grouping)
			 { default:				/* Have a list or range or whatever - do it */
				pim = (struct V4DPI__Point_IntMix *)&dpnt->Value ;
				for(i=0;i<dpnt->Grouping;i++)
				 { if (ipnt->Value.IntVal >= pim->Entry[i].BeginInt && ipnt->Value.IntVal <= pim->Entry[i].EndInt)
				    goto got_match ;
				 } ; break ;
			   case V4DPI_Grouping_Single:
				if (dpnt->PntType == V4DPI_PntType_Time)
				 { if (dpnt->Value.IntVal <= ipnt->Value.IntVal) goto got_match ; }
				 else { if (ipnt->Value.IntVal == dpnt->Value.IntVal) goto got_match ; } ;
				break ;
			   case V4DPI_Grouping_GT:
				if (ipnt->Value.IntVal > dpnt->Value.IntVal) goto got_match ; break ;
			   case V4DPI_Grouping_GE:
				if (ipnt->Value.IntVal >= dpnt->Value.IntVal) goto got_match ; break ;
			   case V4DPI_Grouping_LT:
				if (ipnt->Value.IntVal < dpnt->Value.IntVal) goto got_match ; break ;
			   case V4DPI_Grouping_LE:
				if (ipnt->Value.IntVal <= dpnt->Value.IntVal) goto got_match ; break ;
			   case V4DPI_Grouping_NE:
				if (ipnt->Value.IntVal != dpnt->Value.IntVal) goto got_match ; break ;
			 } ;
			break ;
		 case V4DPI_PntType_V4IS:
			if (memcmp(&dpnt->Value,&ipnt->Value,sizeof(struct V4DPI__PntV4IS)) == 0) goto got_match ;
			break ;
		 case V4DPI_PntType_TeleNum:
			if (dpnt->Value.Tele.AreaCode != ipnt->Value.Tele.AreaCode) break ;
			if (dpnt->Value.Tele.Number != ipnt->Value.Tele.Number) break ;
			if ((dpnt->Value.Tele.IntDialCode < 0 ? -dpnt->Value.Tele.IntDialCode : dpnt->Value.Tele.IntDialCode) != (ipnt->Value.Tele.IntDialCode < 0 ? -ipnt->Value.Tele.IntDialCode : ipnt->Value.Tele.IntDialCode)) break ;
/*			Telephone - only compare basic number, not type & extension */
			goto got_match ;
		 case V4DPI_PntType_Int2:
			if (memcmp(&dpnt->Value.Int2Val,&ipnt->Value.Int2Val,sizeof(ipnt->Value.Int2Val)) == 0) goto got_match ;
			break ;
		 case V4DPI_PntType_XDB:
			if (memcmp(&dpnt->Value.XDB,&ipnt->Value.XDB,sizeof(ipnt->Value.XDB)) == 0) goto got_match ;
			break ;
		 case V4DPI_PntType_BinObj:
			if (memcmp(&ipnt->Value.AlphaVal[0],&dpnt->Value.AlphaVal[0],dpnt->Bytes-V4DPI_PointHdr_Bytes) ==0)
			 goto got_match ;
			break ;
		  case V4DPI_PntType_FrgnDataEl:
		  case V4DPI_PntType_FrgnStructEl:
		  case V4DPI_PntType_UCChar:
			if (dpnt->Grouping == V4DPI_Grouping_Single)
			 { if (ipnt->PntType == V4DPI_PntType_Char)
			    { int i ;
			      for (i=0;;i++) { if (ipnt->Value.AlphaVal[i] != dpnt->Value.UCVal[i] || ipnt->Value.AlphaVal[i] == '\0') break ; } ;
			      if (ipnt->Value.AlphaVal[i] == dpnt->Value.UCVal[i])
			       goto got_match ;
			    } else	/* The compare below will be OK even though [0] is the length of the string */
			    { if (UCCHARSTRLEN(ipnt) == UCCHARSTRLEN(dpnt) ? UCstrncmp(ipnt->Value.UCVal,dpnt->Value.UCVal,UCCHARSTRLEN(dpnt)+1) == 0 : FALSE)
			       goto got_match ;
			    } ;
			   break ;
			 } ;
			pam = (struct V4DPI__Point_AlphaMix *)&dpnt->Value ;
			ucbuf = &ipnt->Value.UCVal[1] ; utbuf = (UCCHAR *)&pam->Entry[dpnt->Grouping].BeginIndex ;
			for(i=0;i<dpnt->Grouping;i++)
			 { if (pam->Entry[i].EndIndex == 0)
			    { if (UCstrncmp(ucbuf,utbuf+pam->Entry[i].BeginIndex+1,*(utbuf+pam->Entry[i].BeginIndex)) == 0)
			       goto got_match ;
			    } else
			    { if (UCstrncmp(ucbuf,utbuf+pam->Entry[i].BeginIndex+1,*(utbuf+pam->Entry[i].BeginIndex)) < 0)
			       break ;
			      if (UCstrncmp(ucbuf,utbuf+pam->Entry[i].EndIndex+1,*(utbuf+pam->Entry[i].EndIndex)) <= 0)
			       goto got_match ;
			    } ;
			 } ;
			break ;
		  CASEofCharmU
			if (dpnt->Grouping == V4DPI_Grouping_Single)
			 { if (ipnt->PntType == V4DPI_PntType_UCChar)
			    { int i ;
			      for (i=0;;i++) { if (ipnt->Value.UCVal[i] != dpnt->Value.AlphaVal[i] || ipnt->Value.UCVal[i] == UCEOS) break ; } ;
			      if (ipnt->Value.AlphaVal[i] == dpnt->Value.UCVal[i]) goto got_match ;
			    } else
			    { if (ipnt->Value.AlphaVal[0] == dpnt->Value.AlphaVal[0] && strncmp(&ipnt->Value.AlphaVal[1],&dpnt->Value.AlphaVal[1],dpnt->Value.AlphaVal[0]) == 0) goto got_match ;
			    } ;
			   break ;
			 } ;
			pam = (struct V4DPI__Point_AlphaMix *)&dpnt->Value ;
			cbuf = (char *)&ipnt->Value.AlphaVal[1] ; tbuf = (char *)&pam->Entry[dpnt->Grouping].BeginIndex ;
			for(i=0;i<dpnt->Grouping;i++)
			 { if (pam->Entry[i].EndIndex == 0)
			    { if (strncmp(cbuf,tbuf+pam->Entry[i].BeginIndex+1,*(tbuf+pam->Entry[i].BeginIndex)) == 0)
			       goto got_match ;
			    } else
			    { if (strncmp(cbuf,tbuf+pam->Entry[i].BeginIndex+1,*(tbuf+pam->Entry[i].BeginIndex)) < 0)
			       break ;
			      if (strncmp(cbuf,tbuf+pam->Entry[i].EndIndex+1,*(tbuf+pam->Entry[i].EndIndex)) <= 0)
			       goto got_match ;
			    } ;
			 } ;
			break ;
	       } ;
no_match:
	      el->Area[topax].CurWgt = -1 ;		/* This point don't hack it - try again */
	      el->Area[topax].DimTest[dex] = -1 ;	/* Flag this dim point as a loser */
	      if (traceGlobal & V4TRACE_PointCompare)
	       { 
		 v_Msg(ctx,UCTBUF2,"@*  Compare: Ctx(%1P) <> Isct #%2d(%3P)\n",ipnt,dx+1,dpnt) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	       } ;
	      break ;					/* Changed to break?? veh/9-Oct-96 */
	      continue ;
got_match:
	      el->Area[topax].DimTest[dex] = 1 ;	/* Flag this dim point as a winner */
	      if (traceGlobal & V4TRACE_PointCompare)
	       { 
		 v_Msg(ctx,UCTBUF2,"@*  Compare: Ctx(%1P) = Isct #%2d(%3P)\n",ipnt,dx+1,dpnt) ; vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	       } ;
	      continue ;
	    } ;
	   if (el->Area[topax].CurWgt < 0)
	    {
	      if (traceGlobal & V4TRACE_PointCompare)
	       vout_UCText(VOUT_Trace,0,UClit("*Results for isct: NOMATCH\n")) ;
	      continue ;	/* This point didn't make it */
	    } ;
/*	   If here then matched all points in binding (looks good almost) */
	   el->Area[topax].CurWgt = -1 ;		/* Mark this as "used" */
	   if (lhcnt < (isct->LHSCnt > 0 ? isct->LHSCnt : isct->Grouping))
	    { 
	      if (traceGlobal & V4TRACE_PointCompare)
	       { UCsprintf(UCTBUF2,V4TMBufMax,UClit("* Results for isct(lhcnt=%d, LHSCnt=%d, Group=%d): Did not match all points before \"|\" - NOMATCH\n"),
				lhcnt,isct->LHSCnt,isct->Grouping) ;
		 vout_UCText(VOUT_Trace,0,UCTBUF2) ;
	       } ;
	      continue ;	/* Didn't match all before "|" in isct */
	    } ;
	   if (traceGlobal & V4TRACE_PointCompare)
	    vout_UCText(VOUT_Trace,0,UClit("*Results for isct: MATCH\n")) ;
	   el->Resnblv = nblv ;
	   if (nblv->sp.Dim == 0 && (nblv->sp.PntType == V4DPI_PntType_Isct || nblv->sp.PntType == V4DPI_PntType_BigIsct))
	    nblv->sp.Dim = Dim_Isct ;			/* VEH120921 - If called from EvalBL() then may return isct - it does not have dimension so give it one to prevent IsctEval() from returning error */
	   el->bindWgt = topwgt ;			/* VEH121015 - return new field - binding weight - so we can randomize selected values when eval'ing [] */
	   return(1) ;
	 } ;
}

#ifdef WANTPATTERNMATCH
/*	v4dpi_MatchPattern - Attempts to match pattern binding in isct		*/
/*	Call: pt = v4dpi_MatchPattern( isct , nbl , nblv , ctx , aid , trace )
	  where pt is resulting isct point or NULL if no match,
		isct is isct to scan for pattern,
		nbl/nblv is current binding being checked,
		ctx is current context,
		aid is where binding (and its value) be at,
		trace is trace flag						*/

P *v4dpi_MatchPattern(isct,nbl,nblv,ctx,aid,trace)
  struct V4DPI__Point *isct ;
  struct V4DPI__BindList *nbl ;
  struct V4DPI__BindList_Value *nblv ;
  struct V4C__Context *ctx ;
  int aid,trace ;
{
  struct V4DPI__Binding bind,*binding ;
  struct V4IS__IndexKeyDataEntry *ikde ;
  struct V4DPI__Point *ipt,*bpt,*cpt,*saveipt1,*saveipt,*rpt,xpnt ;
  struct V4DPI__Point *cpts[50] ; int ccnt,dim ; UCCHAR dimname[50] ;
  struct V4DPI__BindList_Dim *nbld ;
  int i,j,r,max ; int frameid = -1 ;

	nbld = (struct V4DPI__BindList_Dim *)&nbl->Buffer[nbl->DimStart] ;
	max = isct->Grouping - nblv->IndexCnt ;
	for(i=0,ipt=ISCT1STPNT(isct);i<=max;i++,ADVPNT(ipt))
	 { saveipt = ipt ; ccnt = 0 ;
	   for(j=0;j<nblv->IndexCnt;j++)		/* Loop thru each point in binding */
	    { bpt = (P *)&nbl->Buffer[nbl->PointStart+nbld->DimEntryIndex[nblv->DimPos[j]]] ;
	      saveipt1 = ipt ;
//	      if (bpt->PntType == V4DPI_PntType_Special && bpt->Grouping == V4DPI_Grouping_DecompLHS)
//	       { cpts[ccnt++] = ipt ;			/* Decomp (dim?) matches all */
//	       } else
	       { if (ipt->Dim != bpt->Dim)		/* No match - see if we can match via IsA link */
		  { DIMVAL(cpt,ctx,bpt->Dim) ;
		    if (cpt == NULL) break ;		/* Apparently not in context */
		    if (v4ctx_DimBaseDim(ctx,cpt->Dim,trace) != ipt->Dim)
		     break ;				/* Not linked via IsA */
//v4dpi_PointToString(ASCTBUF1,cpt,ctx,-1) ; printf("V4Trace-Pattern: IsA -> %s\n",ASCTBUF1) ;
		    ipt = cpt ; 			/* Link via IsA! */
		  } ;
		 if (bpt->PntType == V4DPI_PntType_Special && (bpt->Grouping == V4DPI_Grouping_All || bpt->Grouping == V4DPI_Grouping_AllCnf))	/* Got dim.. ? */
		  { cpts[ccnt++] = ipt ;		/* Save point in isct matching dim.. */
		  } else if (memcmp(ipt,bpt,bpt->Bytes) != 0) break ;
	       } ;
	      ipt = (P *)((char *)saveipt1 + saveipt1->Bytes) ; /* Advance to next point in isct */
	    } ;
	   if (j >= nblv->IndexCnt)			/* Match at current position? */
	    { saveipt = ipt ; break ; } ;		/* saveipt = point after pattern in isct */
	   ipt = saveipt ; saveipt = NULL ;		/* No match, try starting with next point in isct */
	 } ;
	if (saveipt == NULL) return(NULL) ;		/* Could not find pattern in isct */
	for(j=0;j<ccnt;j++)				/* Add points to context via Pn dimension points */
	 { UCsprintf(dimname,UCsizeof(dimname),UClit("P%d"),j+1) ; dim = v4dpi_DimGet(ctx,dimname,DIMREF_IRT) ;
	   if (frameid <= 0) frameid = v4ctx_FramePush(ctx,NULL) ;			/* Start new context frame */
	   if (cpts[j]->PntType == V4DPI_PntType_List) QUOTE(cpts[j]) ;			/* Want list on context, not its contents */
	   if (!v4ctx_FrameAddDim(ctx,0,cpts[j],dim,0)) return(NULL) ;
//v4dpi_PointToString(ASCTBUF1,cpts[j],ctx,-1) ; printf("V4Trace-Pattern: %s = %s\n",dimname,ASCTBUF1) ;
	 } ;
	 rpt = v4dpi_PntIdx_CvtIdxPtr(v4dpi_PntIdx_AllocPnt()) ;	/* Set up resulting point */
	 INITISCT(rpt) ; NOISCTVCD(rpt) ;
	 for(r=0,ipt=ISCT1STPNT(isct);r<i;r++) 			/* Copy all points to begin of match */
	  { memcpy(&rpt->Value.Isct.pntBuf[rpt->Bytes],ipt,ipt->Bytes) ;
	    rpt->Bytes += ipt->Bytes ; ADVPNT(ipt) ; rpt->Grouping++ ;
	  } ;
//v4dpi_PointToString(ASCTBUF1,rpt,ctx,-1) ; printf("V4Trace-Pattern: Begin result: %s\n",ASCTBUF1) ;
	if (nblv->sp.Bytes == 0)					/* Is value in the short point or in value record */
	 { bind.kp.fld.AuxVal = V4IS_SubType_Value ; bind.kp.fld.KeyType = V4IS_KeyType_V4 ;
	   bind.kp.fld.KeyMode = V4IS_KeyMode_Int ; bind.kp.fld.Bytes = V4IS_IntKey_Bytes ; bind.ValueId = nblv->sp.Value.IntVal ;
	   if (v4is_PositionKey(aid,(struct V4IS__Key *)&bind,(struct V4IS__Key **)&ikde,0,V4IS_PosDCKLx) != V4RES_PosKey)
	    v4_error(V4E_NOVALREC,0,"V4DPI","ExamineBindList","NOVALREC","Could not access value record") ;
	   binding = (struct V4DPI__Binding *)v4dpi_GetBindBuf(ctx,aid,ikde,FALSE,NULL) ;
	   ipt = (P *)&binding->Buffer ;
	 } else 							/* Value is in short point - link up */
	 { ZPH(&xpnt) ;
	   xpnt.Dim = nblv->sp.Dim ; xpnt.Bytes = nblv->sp.Bytes ; xpnt.PntType = nblv->sp.PntType ;
	   xpnt.LHSCnt = nblv->sp.LHSCnt ;
	   memcpy(&xpnt.Value,&nblv->sp.Value,nblv->sp.Bytes-V4DPI_PointHdr_Bytes) ;
	   ipt = &xpnt ;
	 } ;
//v4dpi_PointToString(ASCTBUF1,ipt,ctx,-1) ; printf("V4Trace-Pattern: Result of match: %s\n",ASCTBUF1) ;
	if (ipt->PntType == V4DPI_PntType_Isct) 	/* If result is isct then eval it */
	 { ipt = v4dpi_IsctEval(&xpnt,ipt,ctx,NULL,NULL) ;
	   if (frameid > 0) { if (!v4ctx_FramePop(ctx,frameid,NULL)) ipt = NULL ; frameid = -1 ; } ;
	   if (ipt == NULL) return(NULL) ;			/* If isct does not eval, no match */
//v4dpi_PointToString(ASCTBUF1,ipt,ctx,-1) ; printf("V4Trace-Pattern: Result of isct eval: %s\n",ASCTBUF1) ;
	 } ;
	if (frameid > 0) { if (!v4ctx_FramePop(ctx,frameid,NULL)) return(NULL) ; frameid = -1 ; } ;
	memcpy(&rpt->Value.AlphaVal[rpt->Bytes],ipt,ipt->Bytes) ; rpt->Bytes += ipt->Bytes ; rpt->Grouping++ ;
/*	Now add in all points in isct after that which matched pattern */
	for(ipt=saveipt;rpt->Grouping<=isct->Grouping-nblv->IndexCnt;)
	 { memcpy(&rpt->Value.AlphaVal[rpt->Bytes],ipt,ipt->Bytes) ;
	   rpt->Bytes += ipt->Bytes ; rpt->Grouping++ ; ADVPNT(ipt) ;
	 } ;
	rpt->Bytes += V4DPI_PointHdr_Bytes ;		/* Finish off result point */
	if ((trace & V4TRACE_LogIscts) != 0 && ctx->IsctEvalCount > gpi->StartTraceOutput)
	 { 
//	   v4dpi_PointToString(ASCTBUF1,rpt,ctx,-1) ; printf("V4Trace-Pattern: New pattern isct: %s\n",ASCTBUF1) ; } ;
	   v_Msg(ctx,UCTBUF1,"@*Pattern: New pattern isct: %1P\n",rpt) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ;
	 } ;
	return(rpt) ;
}
#endif

/*	v4dpi_FlattenIsct - Attempts to flatten points within an intersection		*/
/*	Call: ok = v4dpi_FlattenIsct( ctx , srcisct , dstisct )
	  where ok = TRUE if srcisct is flattened,
		srcisct is source intersection,
		dstisct is updated with flattened result (may be same as srcisct),
		makebinding is TRUE to also create relating binding			*/
/*	NOTE: It is assumed that at least one point in intersection is flagged via V4DPI_rtDimInfo_Flatten */	

LOGICAL v4dpi_FlattenIsct(ctx,srcisct,dstisct,makebinding)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *srcisct,*dstisct ;
  int makebinding ;
{
  UB64INT v_Hash64() ;
  struct V4DPI__Binding binding ;
  struct V4DPI__Flatten *ftn ;
  P *ipt,*rpt,*vpt,*fpt,isctbuf,valbuf ;
  int i,j,hits,ok ; char *vp ; UB64INT ub64 ;

	for(ftn=gpi->ftn;ftn!=NULL;ftn=ftn->nftn)	/* Iterate through all structures (most likely only 1 or 2) */
	 { 
/*	   Reset some run-time arrays/structures in current ftn */
	   hits=0 ; memset(ftn->Matched,0,V4DPI_IsctDimMax * (sizeof ftn->Matched[0])) ;
/*	   See if srcisct contains ALL dimensions in this structure */
	   ipt=ISCT1STPNT(srcisct) ;
	   for(i=0;i<srcisct->Grouping;i++,ADVPNT(ipt))
	    { for(j=0;j<ftn->Count;j++)
	       { if (ftn->Entry[j].DimId == ipt->Dim) { hits++ ; ftn->Matched[i] = TRUE ; ftn->Entry[j].spt = ipt ; break ; } ; } ;
	    } ;
/*	   Did we get enough hits to signify a match ? */
	   if (hits < ftn->Count) continue ;		/* Did not match on enough dims - try next ftn */
	   break ;
	 } ;
/*	If here and ftn == NULL then did not get any matches */
	if (ftn == NULL) return(FALSE) ;
/*	Got a match - ftn points to correct structure, ftn->Entry[].spt are pointers to points to be flattened */
/*	First build up flattened point and then copy remainder of un-flatten points in srcisct */
	INITISCT(&isctbuf) ; isctbuf.Value.Isct.vis = srcisct->Value.Isct.vis ;
	rpt = ISCT1STPNT(&isctbuf) ;			/* rpt = position of flatten point in new isct */
	ZPH(rpt) ; rpt->Dim = ftn->TargetDimId ; rpt->PntType = ftn->TargetPntType ; rpt->Bytes = V4DPI_PointHdr_Bytes ;
	vp = (char *)&rpt->Value ;			/* vp = point to next value position in target */
	for(i=0;i<ftn->Count;i++)
	 { ipt = ftn->Entry[i].spt ;
/*	   Copy/transform value into flatten-point */
	   switch (ftn->Entry[i].Transform)
	    { case V4DPI_FTran_Copy:
	      case V4DPI_FTran_Hash64:
	      case V4DPI_FTran_Hash32:
		memcpy(vp,&ipt->Value,ftn->Entry[i].Bytes) ;
		break ;
	      case V4DPI_FTran_AlphaToHash64:
		v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,ipt,ctx) ; ub64 = (UB64INT)v_Hash64(UCTBUF1) ;
		memcpy(vp,&ub64,ftn->Entry[i].Bytes) ; break ;
	    } ;
	   rpt->Bytes += ftn->Entry[i].Bytes ; vp += ftn->Entry[i].Bytes ;
	 } ;
	switch (ftn->Entry[0].Transform)
	 { case V4DPI_FTran_Hash64:	ub64 = v_Hash64b((char *)&rpt->Value,rpt->Bytes-V4DPI_PointHdr_Bytes) ; rpt->Bytes = V4PS_Int2 ; memcpy(&rpt->Value.Int2Val,&ub64,sizeof ub64) ; break ;
	   case V4DPI_FTran_Hash32:	i = v_Hash32((char *)&rpt->Value,rpt->Bytes-V4DPI_PointHdr_Bytes,1) ; rpt->Bytes = V4PS_Int ; rpt->Value.IntVal = i ; break ;
	 } ;
	isctbuf.Grouping++ ;

/*	Now copy remainder of points into new isct */
	ipt=ISCT1STPNT(srcisct) ;
	for(i=0;i<srcisct->Grouping;i++,ADVPNT(ipt))
	 { if (ftn->Matched[i]) continue ;
	   ADVPNT(rpt) ;		/* rpt = position for next point in isct */
	   memcpy(rpt,ipt,ipt->Bytes) ; isctbuf.Grouping++ ;
	 } ;						/* rpt = position of last copied point */
/*	Figure out size of new isct & copy into dstisct */
	isctbuf.Bytes = (char *)rpt + rpt->Bytes - (char *)&isctbuf ;
//v_Msg(ctx,NULL,"@Flattened %1P -> %2P (%3d bytes)\n",srcisct,&isctbuf,isctbuf.Bytes) ; printf("%s",ctx->ErrorMsg) ;
	memcpy(dstisct,&isctbuf,isctbuf.Bytes) ;

/*	Should we create a binding of form [nid dim.. dim.. ...] = [nid Flatten(dim* dim* ...)] to access flattened isct */
	if (!(makebinding && dstisct->Grouping == 2 && rpt->Dim == Dim_NId)) return(TRUE) ;
	for(i=0;i<ftn->NIdCount;i++) { if (rpt->Value.IntVal == ftn->SeenNIds[i]) break ; } ;
	if (i < ftn->NIdCount) return(TRUE) ;		/* Already have a binding for this NId - don't duplicate */
	ftn->SeenNIds[ftn->NIdCount++] = rpt->Value.IntVal ;

/*	Yes - construct isct & value then bind together */
	ipt = &isctbuf ; INITISCT(ipt) ; NOISCTVCD(ipt) ;
	ipt = ISCT1STPNT(ipt) ; memcpy(ipt,rpt,rpt->Bytes) ; isctbuf.Bytes += rpt->Bytes ; isctbuf.Grouping++ ;
	rpt = ipt ;
	for(i=0;i<ftn->Count;i++)
	 { ADVPNT(ipt) ; ZPH(ipt) ; ipt->PntType = V4DPI_PntType_Special ; ipt->Dim = ftn->Entry[i].DimId ;
	   ipt->Grouping = V4DPI_Grouping_All ; ipt->Bytes = V4DPI_PointHdr_Bytes ;
	   isctbuf.Bytes += ipt->Bytes ; isctbuf.Grouping++ ;
	 } ;

	vpt = &valbuf ; INITISCT(vpt) ; NOISCTVCD(vpt) ; vpt->Grouping = 2 ; vpt->Bytes = V4DPI_PointHdr_Bytes ; vpt->NestedIsct = TRUE ;
	ipt = ISCT1STPNT(vpt) ; memcpy(ipt,rpt,rpt->Bytes) ; valbuf.Bytes += rpt->Bytes ; vpt->Bytes += rpt->Bytes ;
	
	fpt = (P *)((char *)ipt + ipt->Bytes) ; INITISCT(fpt) ; NOISCTVCD(fpt) ; fpt->Grouping = ftn->Count + 1 ;
	ipt = ISCT1STPNT(fpt) ;
	ZPH(ipt) ; ipt->PntType = V4DPI_PntType_IntMod ; ipt->Bytes = V4PS_Int ; ipt->Value.IntVal = V4IM_OpCode_Flatten ; ipt->Dim = Dim_IntMod ;
	  fpt->Bytes += ipt->Bytes ;
	for(i=0;i<ftn->Count;i++)
	 { ADVPNT(ipt) ;
	   ZPH(ipt) ; ipt->PntType = V4DPI_PntType_Special ; ipt->Dim = ftn->Entry[i].DimId ; ipt->Bytes = V4DPI_PointHdr_Bytes ;
	   ipt->Grouping = V4DPI_Grouping_Current ; fpt->Bytes += ipt->Bytes ;
	 } ;

	vpt->Bytes += fpt->Bytes ;			/* Finally update top-level value point with size of second Flatten() point */
	if (!v4dpi_BindListMake(&binding,vpt,&isctbuf,ctx,NULL,NOWGTADJUST,0,DFLTRELH)) return(FALSE) ;

	return(TRUE) ;
}



/*	v4dpi_AcceptValue - Updates V4 point with value			*/
/*	NOTE: if pval is string & does not end in EOS then pval buffer will be temporarily UPDATED */
/*	Call: pt = v4dpi_AcceptValue( ctx , respnt , di , vdType , pval , lenval )
	  where pt is resulting V4 point (NULL if error, msg in ctx->ErrorMsgAux),
		ctx is current context,
		respnt is point to be updated,
		di is dim info,
		vdType is format of data (see VDTYPE_xxx),
		pval is pointer to the value,
		lenval is length (characters BUT SEE types BELOW for exceptions) of value (may be 0 if size implied by vdType)	*/

P *v4dpi_AcceptValue(ctx,respnt,di,vdType,pval,lenval)
  struct V4C__Context *ctx ;
  P *respnt ;
  struct V4DPI__DimInfo *di ;
  ETYPE vdType ;
  void *pval ; LENMAX lenval ;
{ 
  static struct V4CI__CountryInfo *ci = NULL ;
  YMDORDER *ymdo ;
#ifdef WANTODBC
  DATE_STRUCT *ds ;
  TIME_STRUCT *ts ;
  TIMESTAMP_STRUCT *tss ;
  SQL_NUMERIC_STRUCT *sns ;
#endif
#ifdef WANTMYSQL
  MYSQL_TIME *mts ;
#endif
#ifdef WANTORACLE
  OCI_Resultset *rs ;
  OCI_Date *ots ;
  INDEX sx, cx ;
#endif

  short sval ; int ival ; double dval ; float fval ; LOGICAL sign ; B64INT lval ;
  INDEX i ; 
  UCCHAR utbuf1[V4TMBufMax], *ucpterm, *ucp ; char *bp ;
  UCCHAR *ucpValTerm,ucValTerm ; char *pValTerm,cValTerm ;
  
	ucpValTerm = NULL ; pValTerm = NULL ; cValTerm = '\0' ; ucValTerm = UClit('\0') ;
/*	***** All paths must end at label 'retOK(1)' or 'fail' ***** */
	switch (vdType)
	 { default:
		v_Msg(ctx,ctx->ErrorMsgAux,"XDBVDType",vdType,di->DimId) ; return(NULL) ;
	   case VDTYPE_b1SInt:
		ival = *(char *)pval ; goto b4SInt_entry ;
	   case VDTYPE_b2SInt:
		memcpy(&sval,pval,sizeof(short)) ; ival = sval ; goto b4SInt_entry ;
	   case VDTYPE_b4SInt:
		memcpy(&ival,pval,sizeof (int)) ;
b4SInt_entry:	switch (di->PointType)
		 { default:			v_Msg(ctx,ctx->ErrorMsgAux,"XDBUnSupCnv",vdType,di->DimId,di->PointType) ; goto fail ;
		   case V4DPI_PntType_Dict:
		   case V4DPI_PntType_XDict:	pval = utbuf1 ; UCsprintf(pval,20,UClit("%d"),ival) ; lenval = UCstrlen(utbuf1) ; goto UCChar_entry ;
		   case V4DPI_PntType_Int:	intPNTv(respnt,ival) ; break ;
		   case V4DPI_PntType_Real:	dval = (double)ival ; dblPNTv(respnt,dval) ; break ;
		   case V4DPI_PntType_Fixed:
		     lval = ival ;
		     fixPNTv(respnt,lval) ; break ;
		   case V4DPI_PntType_Char:	sprintf(&respnt->Value.AlphaVal[1],"%d",ival) ; alphaPNT(respnt) ; CHARPNTBYTES1(respnt) ; break ;
		   case V4DPI_PntType_BigText:
		   case V4DPI_PntType_UCChar:	UCsprintf(&respnt->Value.UCVal[1],40,UClit("%d"),ival) ; uccharPNT(respnt) ; UCCHARPNTBYTES1(respnt) ; break ;
		   case V4DPI_PntType_Logical:	logPNTv(respnt,(ival != 0)) ; break ;
		   case V4DPI_PntType_TeleNum:
		     UCsprintf(utbuf1,40,UClit("%d"),ival) ;
		     if (!v_parseTeleNum(ctx,di,respnt,NULL,utbuf1,ctx->ErrorMsg))
		      { v_Msg(ctx,ctx->ErrorMsgAux,"CnvBadVal",utbuf1,di->DimId,di->PointType,ctx->ErrorMsg) ;  goto fail ; } ;
		     break ;
		   case V4DPI_PntType_UDate:
		     UCsprintf(utbuf1,50,UClit("%d"),ival) ; ucp = utbuf1 ;	/* Convert to string because it will be a lot easier to parse with existing stuff */
		     ymdo = (di->ds.UDate.YMDOrder[0] == V4LEX_YMDOrder_None ? gpi->ci->Cntry[gpi->ci->CurX].ymdOrder : &di->ds.UDate.YMDOrder[0]) ;
		     for(i=0;i<VCAL_YMDOrderMax && ymdo[i]!=V4LEX_YMDOrder_None;i++)
		      { respnt->Value.IntVal = v_ParseUDate(ctx,ucp,di->ds.UDT.IFormat,ymdo[i],((di->ds.UDate.calFlags & VCAL_Flags_Historical) == 0)) ;
		        if (respnt->Value.IntVal != VCAL_BadVal || di->ds.UDate.IFormat != V4LEX_TablePT_Default) break ;
		      } ;
		     if (respnt->Value.IntVal == VCAL_BadVal)		/* Enough already: Not valid - see if matches special values */
		      { 
		        if (!v4sxi_SpecialAcceptor(ctx,respnt,di,ucp)) goto fail ;
		      } ;
		     respnt->Bytes = V4PS_Int ; respnt->PntType = di->PointType ; break ;
		   case V4DPI_PntType_UYear:	VCALADJYEAR(((di->ds.UYear.calFlags & VCAL_Flags_Historical) != 0),ival) ; intPNTv(respnt,ival) ; respnt->PntType = di->PointType ; break ;
		   case V4DPI_PntType_UTime:
		     { COUNTER hh,mm,ss ;
		       hh = ival / 10000 ; mm = (ival % 10000) / 100 ; ss = (ival % 100) ;
		       dval = (double)(hh * 3600 + mm * 60 + ss) ;
		       dblPNTv(respnt,dval) ; respnt->PntType = di->PointType ;
		     }
		     break ;
		   case V4DPI_PntType_UMonth:
/*		     ival better be of form yyyymm - convert to internal UMonth format */
		     { int yy,mm ;
		       yy = ival / 100 ; VCALADJYEAR(((di->ds.UWeek.calFlags & VCAL_Flags_Historical) != 0),yy) ; mm = ival % 100 ;
		       if (yy < VCAL_UYearMin || yy > VCAL_UYearMax || mm < 1 || mm > 12)
		        { v_Msg(ctx,ctx->ErrorMsgAux,"CnvBadWeek",yy,mm,di->DimId,di->PointType) ; goto fail ; } ;
//		       intPNTv(respnt,(yy - VCAL_BaseYear) * 52 + ww - 1) ;
		       intPNTv(respnt,YYMMtoUMONTH(yy,mm)) ;
		       if ((di->ds.UWeek.calFlags & VCAL_Flags_Historical) != 0)
		        { if (yy > gpi->curYear) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvFuture",respnt,di->DimId) ; goto fail ; } ;
		        } ;
		     } respnt->PntType = di->PointType ;
		     break ;
		   case V4DPI_PntType_UQuarter:
/*		     ival better be of form yyyyq - convert to internal UMonth format */
		     { int yy,qq ;
		       yy = ival / 10 ; VCALADJYEAR(((di->ds.UWeek.calFlags & VCAL_Flags_Historical) != 0),yy) ; qq = ival % 10 ;
		       if (yy < VCAL_UYearMin || yy > VCAL_UYearMax || qq < 1 || qq > 4)
		        { v_Msg(ctx,ctx->ErrorMsgAux,"CnvBadWeek",yy,qq,di->DimId,di->PointType) ; goto fail ; } ;
//		       intPNTv(respnt,(yy - VCAL_BaseYear) * 52 + ww - 1) ;
		       intPNTv(respnt,YYQQtoUQTR(yy,qq)) ;
		       if ((di->ds.UWeek.calFlags & VCAL_Flags_Historical) != 0)
		        { if (yy > gpi->curYear) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvFuture",respnt,di->DimId) ; goto fail ; } ;
		        } ;
		     } respnt->PntType = di->PointType ;
		     break ;
		   case V4DPI_PntType_UPeriod:
/*		     ival better be of form yyyypp - convert to internal UPeriod format */
		     { int yy,pp,ppy ;
		       yy = ival / 100 ; pp = ival % 100 ; ppy = (di->ds.UPeriod.periodsPerYear == 0 ? 13 : di->ds.UPeriod.periodsPerYear) ;
		       intPNTv(respnt,YYPPtoUPERIOD(yy,pp,ppy)) ;
		       if ((di->ds.UPeriod.calFlags & VCAL_Flags_Historical) != 0)
		        { if (yy > gpi->curYear) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvFuture",respnt,di->DimId) ; goto fail ; } ;
		        } ;
		     } respnt->PntType = di->PointType ;
		     break ;
		   case V4DPI_PntType_UWeek:
/*		     ival better be of form yyyyww - convert to internal UWeek format */
		     { int yy,ww ;
		       yy = ival / 100 ; VCALADJYEAR(((di->ds.UWeek.calFlags & VCAL_Flags_Historical) != 0),yy) ; ww = ival % 100 ;
		       if (yy < VCAL_UYearMin || yy > VCAL_UYearMax || ww < 1 || ww > 52)
		        { v_Msg(ctx,ctx->ErrorMsgAux,"CnvBadWeek",yy,ww,di->DimId,di->PointType) ; goto fail ; } ;
//		       intPNTv(respnt,(yy - VCAL_BaseYear) * 52 + ww - 1) ;
		       intPNTv(respnt,YYWWtoUWEEK(yy,ww)) ;
		       if ((di->ds.UWeek.calFlags & VCAL_Flags_Historical) != 0)
		        { if (yy > gpi->curYear) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvFuture",respnt,di->DimId) ; goto fail ; } ;
		        } ;
		     } respnt->PntType = di->PointType ;
		     break ;
		 } ;
		goto retOK1 ;
	   case VDTYPE_b8SInt:
		memcpy(&lval,pval,sizeof(lval)) ;
		switch (di->PointType)
		 { default:			v_Msg(ctx,ctx->ErrorMsgAux,"XDBUnSupCnv",vdType,di->DimId,di->PointType) ; goto fail ;
		   case V4DPI_PntType_Int:
		     if (lval < (B64INT)V4LIM_SmallestNegativeInt || lval > (B64INT)V4LIM_BiggestPositiveInt) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvOverflow1",(double)lval,di->DimId,di->PointType) ; goto fail ; } ;
		     ival = (int)lval ;
		     intPNTv(respnt,ival) ; break ;
		   case V4DPI_PntType_Fixed:
		     fixPNTv(respnt,lval) ; break ;
		   case V4DPI_PntType_Real:	dval = lval ; dblPNTv(respnt,dval) ; break ;
		   case V4DPI_PntType_Char:	sprintf(&respnt->Value.AlphaVal[1],"%lld",lval) ; alphaPNT(respnt) ; CHARPNTBYTES1(respnt) ; break ;
		   case V4DPI_PntType_BigText:
		   case V4DPI_PntType_UCChar:	UCsprintf(&respnt->Value.UCVal[1],40,UClit("%lld"),lval) ; uccharPNT(respnt) ; UCCHARPNTBYTES1(respnt) ; break ;
		   case V4DPI_PntType_Logical:	logPNTv(respnt,(lval != 0)) ; break ;
		   case V4DPI_PntType_Dict:
		   case V4DPI_PntType_XDict:	pval = utbuf1 ; UCsprintf(pval,20,UClit("%lld"),lval) ; lenval = UCstrlen(utbuf1) ; goto UCChar_entry ;
		 } ;
		goto retOK1 ;
	   case VDTYPE_b4FP:
		memcpy(&fval,pval,sizeof(fval)) ; dval = fval ; goto b8FP_entry ;
	   case VDTYPE_b8FP:
		memcpy(&dval,pval,sizeof(dval)) ;
b8FP_entry:	switch (di->PointType)
		 { default:			v_Msg(ctx,ctx->ErrorMsgAux,"XDBUnSupCnv",vdType,di->DimId,di->PointType) ; goto fail ;
		   case V4DPI_PntType_Int:
		     if (dval < V4LIM_SmallestNegativeInt || dval > V4LIM_BiggestPositiveInt) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvOverflow1",dval,di->DimId,di->PointType) ; goto fail ; } ;
		     intPNTv(respnt,DtoI(dval)) ; break ;
		   case V4DPI_PntType_Fixed:
		     if (dval < V4LIM_SmallestNegativeLong || dval > V4LIM_BiggestPositiveLong) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvOverflow1",dval,di->DimId,di->PointType) ; goto fail ; } ;
		     lval = dval ;
		     fixPNTv(respnt,lval) ; break ;
		   case V4DPI_PntType_Real:	dblPNTv(respnt,dval) ; break ;
		   case V4DPI_PntType_Char:	sprintf(&respnt->Value.AlphaVal[1],"%g",dval) ; alphaPNT(respnt) ; CHARPNTBYTES1(respnt) ; break ;
		   case V4DPI_PntType_BigText:
		   case V4DPI_PntType_UCChar:	UCsprintf(&respnt->Value.UCVal[1],40,UClit("%g"),dval) ; uccharPNT(respnt) ; UCCHARPNTBYTES1(respnt) ; break ;
		   case V4DPI_PntType_Logical:	logPNTv(respnt,(dval != 0)) ; break ;
		   case V4DPI_PntType_UTime:
		     ival = dval ; goto b4SInt_entry;
		   case V4DPI_PntType_TeleNum:
		     UCsprintf(utbuf1,40,UClit("%g"),dval) ;
		     if (!v_parseTeleNum(ctx,di,respnt,NULL,utbuf1,ctx->ErrorMsg))
		      { v_Msg(ctx,ctx->ErrorMsgAux,"CnvBadVal",utbuf1,di->DimId,di->PointType,ctx->ErrorMsg) ;  goto fail ; } ;
		     break ;
		 } ;
		goto retOK1 ;
	   case VDTYPE_NEWDEC:			/* These appear to come over as decimal character strings */
	   case VDTYPE_Char:
char_entry:
		bp = (char *)pval ;
		if (bp[lenval] != '\0') { pValTerm = &bp[lenval] ; cValTerm = bp[lenval] ; bp[lenval] = '\0' ;} ;
		if ((di->Flags & V4DPI_DimInfo_Acceptor) != 0)
		 { if (v4dpi_ValSpclAcptr(ctx,di,bp,NULL,respnt)) goto retOK ; } ;
		switch (di->PointType)
		 { default:
/*		     Default action is to convert to UC and drop into that code */
		     UCstrcpyAtoU(utbuf1,bp) ; pval = utbuf1 ; lenval = UCstrlen(utbuf1) ; goto UCChar_entry ;
		   case V4DPI_PntType_Char:
		     if (lenval >= V4DPI_AlphaVal_Max) { UCstrcpyAtoU(utbuf1,bp) ; pval = utbuf1 ; lenval = UCstrlen(utbuf1) ; goto UCChar_entry ; } ;
		     alphaPNTvl(respnt,bp,lenval) ; goto retOK1 ;
		 } ;
		goto retOK ;
	   case VDTYPE_UCChar:
		lenval /= sizeof(UCCHAR) ;		/* Convert lenval to characters */
UCChar_entry:	ucp = (UCCHAR *)pval ;
		if (lenval > V4LEX_BigText_Max)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"BigTextMaxLen",V4LEX_BigText_Max,di->PointType) ; goto fail ; } ;
		if (ucp[lenval] != UCEOS) { ucpValTerm = &ucp[lenval] ; ucValTerm = ucp[lenval] ; ucp[lenval] = UCEOS ;} ;
		if ((di->Flags & V4DPI_DimInfo_Acceptor) != 0)
		 { if (v4dpi_ValSpclAcptr(ctx,di,NULL,ucp,respnt)) goto retOK ; } ;
		switch (di->PointType)
		 { default:			v_Msg(ctx,ctx->ErrorMsgAux,"XDBUnSupCnv",vdType,di->DimId,di->PointType) ; goto fail ;
		   case V4DPI_PntType_Int:
		     ival = UCstrtol(ucp,&ucpterm,10) ; if (!(*ucpterm == UCEOS || *ucpterm == UClit('.'))) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvBadNum",ucp,di->DimId,di->PointType) ; goto fail ; } ;
		     intPNTv(respnt,ival) ; goto retOK1 ;
		   case V4DPI_PntType_Real:
//		     dval = UCstrtod(ucp,&ucpterm) ; if (*ucpterm != UCEOS) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvBadNum",ucp,di->DimId,di->PointType) ; goto fail ; } ;
		     dval = v_ParseDbl(ucp,&ucpterm) ; if (*ucpterm != UCEOS) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvBadNum",ucp,di->DimId,di->PointType) ; goto fail ; } ;
		     dblPNTv(respnt,dval) ; goto retOK1 ;
		   case V4DPI_PntType_Fixed:
		     for(lval=0,sign=FALSE,i=0;ucp[i]!=UCEOS;i++)
		      { switch(ucp[i])
		         { default:		v_Msg(ctx,ctx->ErrorMsgAux,"CnvBadNum",ucp,di->DimId,di->PointType) ; goto fail ;
		           case UClit('-'):
			     if (sign) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvBadNum",ucp,di->DimId,di->PointType) ; goto fail ; } ;
			     sign = TRUE ; continue ;
		           case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'):
		           case UClit('5'): case UClit('6'): case UClit('7'): case UClit('8'): case UClit('9'):
			     lval *= 10 ; lval += (ucp[i] - UClit('0')) ; break ;
			 } ;
		      } ;
		     if (lval < 0) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvOverflow",ucp,di->DimId,di->PointType) ; goto fail ; } ;
		     if (sign) lval = -lval ; fixPNTv(respnt,lval) ;
		     break ;
		   case V4DPI_PntType_Country:
		     intPNTv(respnt,v_CountryNameToRef(ucp)) ; respnt->PntType = V4DPI_PntType_Country ;
		     if (respnt->Value.IntVal == 0)
		      { if (v_IsNoneLiteral(ucp)) { respnt->Value.IntVal = UNUSED ; }
			 else {  v_Msg(ctx,ctx->ErrorMsgAux,"DPIAcpNoCntry",di->DimId,ucp) ; goto fail ; } ;
		      } ;
		     goto retOK1 ;
		   case V4DPI_PntType_UQuarter:
		   case V4DPI_PntType_UYear:	/* Convert these to integer and then handle as integer */
		   case V4DPI_PntType_UWeek:
		     ival = UCstrtol(ucp,&ucpterm,10) ; if (*ucpterm != UCEOS) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvBadNum",ucp,di->DimId,di->PointType) ; goto fail ; } ;
		     goto b4SInt_entry ;
		   case V4DPI_PntType_UMonth:
		     if (UCstrchr(ucp,'/') != NULL) { i = V4LEX_TablePT_MMYY ; }
		      else if (UCstrchr(ucp,'-') != NULL) { i = V4LEX_TablePT_MMM ; }
		      else { i = (di->ds.UMonth.IFormat != 0 ? di->ds.UMonth.IFormat : V4LEX_TablePT_YYMM) ; } ;
		     respnt->Value.IntVal = v_ParseUMonth(ctx,ucp,i) ;
		     if (respnt->Value.IntVal == VCAL_BadVal)
		      { if (!v4sxi_SpecialAcceptor(ctx,respnt,di,ucp)) goto fail ;
		      } ;
		     intPNT(respnt) ; respnt->PntType = di->PointType ;
		     break ;
		   case V4DPI_PntType_UDate:
		     ymdo = (di->ds.UDate.YMDOrder[0] == V4LEX_YMDOrder_None ? gpi->ci->Cntry[gpi->ci->CurX].ymdOrder : &di->ds.UDate.YMDOrder[0]) ;
		     for(i=0;i<VCAL_YMDOrderMax && ymdo[i]!=V4LEX_YMDOrder_None;i++)
		      { respnt->Value.IntVal = v_ParseUDate(ctx,ucp,di->ds.UDT.IFormat,ymdo[i],((di->ds.UDate.calFlags & VCAL_Flags_Historical) == 0)) ;
		        if (respnt->Value.IntVal != VCAL_BadVal || di->ds.UDate.IFormat != V4LEX_TablePT_Default) break ;
		      } ;
		     if (respnt->Value.IntVal == VCAL_BadVal)		/* Enough already: Not valid - see if matches special values */
		      { 
		        if (!v4sxi_SpecialAcceptor(ctx,respnt,di,ucp)) goto fail ;
//		        if (!v4sxi_SpecialAcceptor(ctx,respnt,di,ucp))
//		         { if (v_ParseRelativeUDate(ctx,ucp,&respnt->Value.IntVal,3,((di->ds.UDate.calFlags & VCAL_Flags_Historical) == 0)) == NULL) goto fail ; } ;
		      } ;
		     respnt->Bytes = V4PS_Int ; respnt->PntType = di->PointType ; break ;
		   case V4DPI_PntType_UPeriod:
		     respnt->Value.IntVal = v_ParseUPeriod(ctx,ucp,di,ctx->ErrorMsgAux) ;
		     if (respnt->Value.IntVal == VCAL_BadVal) goto fail ;
		     intPNT(respnt) ; respnt->PntType = di->PointType ;
		     break ;
		   case V4DPI_PntType_Calendar:
		     ymdo = (di->ds.Cal.YMDOrder[0] == V4LEX_YMDOrder_None ? gpi->ci->Cntry[gpi->ci->CurX].ymdOrder : &di->ds.Cal.YMDOrder[0]) ;
		     for(i=0;i<VCAL_YMDOrderMax && ymdo[i]!=V4LEX_YMDOrder_None;i++)
		      { dval = v_ParseCalendar(ctx,ucp,di->ds.Cal.IFormat,ymdo[i],di->ds.Cal.CalendarType,di->ds.Cal.TimeZone,((di->ds.Cal.calFlags & VCAL_Flags_Historical) == 0)) ;
		        if (dval != VCAL_BadVal || di->ds.Cal.IFormat != V4LEX_TablePT_Default) break ;
		      } ;
		     if (dval == VCAL_BadVal)
		      { PNTTYPE pntType ;
		        if (v_ParseRelativeUDate(ctx,ucp,&i,3,((di->ds.Cal.calFlags & VCAL_Flags_Historical) == 0),&pntType,NULL) == NULL) goto fail ;
			if (pntType != V4DPI_PntType_UDate)
			 { v_Msg(ctx,ctx->ErrorMsgAux,"ParseDateRelDT",ucp,pntType,V4DPI_PntType_UDate) ; goto fail ; } ;
		        dval = VCal_MJDOffset + i ;
		      } ;
		     dblPNTv(respnt,dval) ; respnt->PntType = di->PointType ;
		     break ;
		   case V4DPI_PntType_UTime:
		     { COUNTER hh,mm,ss=0 ; double ff=0 ;
		       if (v_IsNoneLiteral(ucp)) { hh = 0 ; mm = 0 ; }
			else { hh = UCstrtol(ucp,&ucpterm,10) ; 
			       if (*ucpterm != UClit(':')) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvBadVal",ucp,di->DimId,di->PointType,UClit("syntax")) ; goto fail ; } ;
			       mm = UCstrtol(ucpterm+1,&ucpterm,10) ;
			       if (*ucpterm == UClit(':'))
				{ ss = UCstrtol(ucpterm+1,&ucpterm,10) ;
				  if (*ucpterm == '.') { ff = UCstrtod(ucpterm,&ucpterm) ; } ;
				} ;
			       if (*ucpterm != '\0') { v_Msg(ctx,ctx->ErrorMsgAux,"CnvBadVal",ucp,di->DimId,di->PointType,UClit("syntax")) ; goto fail ; } ;
			     } ;
		       dval = (double)(hh * 3600 + mm * 60 + ss) + ff ;
		       dblPNTv(respnt,dval) ; respnt->PntType = di->PointType ;
		     } break ;
		   case V4DPI_PntType_UDT:
		     ymdo = (di->ds.UDT.YMDOrder[0] == V4LEX_YMDOrder_None ? gpi->ci->Cntry[gpi->ci->CurX].ymdOrder : &di->ds.UDT.YMDOrder[0]) ;
		     for(i=0;i<VCAL_YMDOrderMax && ymdo[i]!=V4LEX_YMDOrder_None;i++)
		      { respnt->Value.IntVal = v_ParseUDT(ctx,ucp,di->ds.UDT.IFormat,ymdo[i],3,&ival,((di->ds.UDT.calFlags & VCAL_Flags_Historical) == 0)) ;
		        if (respnt->Value.IntVal != VCAL_BadVal || di->ds.UDT.IFormat != V4LEX_TablePT_Default) break ;
		      } ;
		     if (respnt->Value.IntVal == VCAL_BadVal)
		      { if (!v4sxi_SpecialAcceptor(ctx,respnt,di,ucp)) goto fail ; ;
		      } ;
		     if (ival == 1)				/* If = 1 then got relative date & NO time (ex: TODAY) */
		      { respnt->Grouping = 1 ;		/* Return range of times - from midnight to 23:59:59 */
		        respnt->Value.Int2Val[1] = respnt->Value.Int2Val[0] + VCAL_SecsInDay - 1 ;
		        respnt->Bytes = V4PS_Int2 ; respnt->PntType = di->PointType ; break ;
		      } ;
		     respnt->Bytes = V4PS_Int ; respnt->PntType = di->PointType ; break ;
		   case V4DPI_PntType_Logical:
		     UCcnvupper(utbuf1,ucp,UCsizeof(utbuf1)) ;
		     if (ci == NULL) { ci = ((struct V4C__ProcessInfo *)v_GetProcessInfo())->ci ; } ;
		     respnt->Value.IntVal = v_ParseLog(utbuf1,&ucpterm) ;
		     if (*ucpterm != UCEOS) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvBadLog",V4DPI_PntType_Logical,ci->li->LI[ci->Cntry[ci->CurX].Language].Yes,ci->li->LI[ci->Cntry[ci->CurX].Language].True,ci->li->LI[ci->Cntry[ci->CurX].Language].No,ci->li->LI[ci->Cntry[ci->CurX].Language].False) ; goto fail ; } ;
		     logPNT(respnt) ; break ;
		   case V4DPI_PntType_Dict:
		   case V4DPI_PntType_XDict:
		     dictPNT(respnt,di->DimId) ; respnt->PntType = di->PointType ;
		     { UCCHAR *de, *delim = UCstrchr(ucp,',') ; INDEX dlen ;
		       struct V4DPI__Point_IntMix *pim ;
		       if (delim == NULL)
		        { respnt->Value.IntVal = (di->PointType == V4DPI_PntType_Dict ? v4dpi_DictEntryGet(ctx,di->DimId,ucp,di,NULL) : v4dpi_XDictEntryGet(ctx,ucp,di,0)) ;
			  if (respnt->Value.IntVal < 0) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvNoDict",ucp,di->DimId,di->PointType) ; goto fail ; } ;
			} else
			{ pim = (struct V4DPI__Point_IntMix *)&respnt->Value ;
			  for(de=ucp;;de=delim+1,delim=UCstrchr(de,','))
			   { UCCHAR dEntry[V4DPI_XDictEntryVal_Max+1] ;
			     dlen = (delim == NULL ? UCstrlen(de) : delim - de) ; if (dlen > V4DPI_XDictEntryVal_Max) dlen = V4DPI_XDictEntryVal_Max ;
			     UCstrncpy(dEntry,de,dlen) ; dEntry[dlen] = UCEOS ;
			     pim->Entry[respnt->Grouping].BeginInt = (di->PointType == V4DPI_PntType_Dict ? v4dpi_DictEntryGet(ctx,di->DimId,dEntry,di,NULL) : v4dpi_XDictEntryGet(ctx,dEntry,di,0)) ;
			     if (pim->Entry[respnt->Grouping].BeginInt < 0) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvNoDict",dEntry,di->DimId,di->PointType) ; goto fail ; } ;
			     pim->Entry[respnt->Grouping].EndInt = pim->Entry[respnt->Grouping].BeginInt ;
			     respnt->Grouping++ ;
			     if (delim == NULL) break ;
			   } ;
			  respnt->Bytes = (char *)&pim->Entry[respnt->Grouping].BeginInt - (char *)respnt ;
			} ;
		     }
		     goto retOK ;
		   case V4DPI_PntType_Char:
		     /* If too big for Alpha OR contains any Unicode then drop through to UCChar and write out as BigText */
		     if (lenval < V4DPI_AlphaVal_Max)
		      { LOGICAL ok = TRUE ;
		        for(i=0;ucp[i]!=UCEOS;i++) { if (ucp[i] > 0xff) { ok = FALSE ; break ; } ; } ;
		        if (ok)
			 { alphaPNTvl(respnt,UCretASC(ucp),lenval) ; goto retOK1 ; } ;
		      } ;
		   case V4DPI_PntType_BigText:
		   case V4DPI_PntType_UCChar:
		     if (lenval >= V4DPI_UCVAL_MaxSafe)
		      { if (!v4dpi_SaveBigTextPoint2(ctx,ucp,respnt,di->DimId,TRUE)) goto fail ;
		        break ;
		      } ;
		     uccharPNTvl(respnt,ucp,lenval) ; goto retOK1 ;
		   case V4DPI_PntType_TeleNum:
		     if (!v_parseTeleNum(ctx,di,respnt,NULL,ucp,ctx->ErrorMsg))
		      { v_Msg(ctx,ctx->ErrorMsgAux,"CnvBadVal",ucp,di->DimId,di->PointType,ctx->ErrorMsg) ;  goto fail ; } ;
		     break ;
		   case V4DPI_PntType_GeoCoord:
		     if (!v_ParseGeoCoord(ctx,respnt,di,ucp,ctx->ErrorMsg))
		      { v_Msg(ctx,ctx->ErrorMsgAux,"CnvBadVal",ucp,di->DimId,di->PointType,ctx->ErrorMsg) ;  goto fail ; } ;
		     break ;
		 } ;
		goto retOK1 ;
	   case VDTYPE_UTF8:
		lenval = UCUTF8toUTF16(utbuf1,V4TMBufMax,pval,lenval) ;
		pval = utbuf1 ; goto UCChar_entry ;
#ifdef WANTODBC
	   case VDTYPE_ODBC_DS:
		ds = (DATE_STRUCT *)pval ;
		switch (di->PointType)
		 { default:			v_Msg(ctx,ctx->ErrorMsgAux,"XDBUnSupCnv",vdType,di->DimId,di->PointType) ; goto fail ;
		   case V4DPI_PntType_UDate:
		     respnt->Value.IntVal = (ds->year == 0 && ds->month == 0 && ds->day == 0 ? VCAL_UDate_None : vcal_UDateFromYMD((di->ds.UDate.calFlags & VCAL_Flags_Historical),(ds->year < 1900 ? ds->year+1900 : ds->year),ds->month,ds->day,ctx->ErrorMsgAux)) ;
		     if (respnt->Value.IntVal == VCAL_BadVal) goto fail ;
		     intPNT(respnt)
		     goto chk_udate ;
		   case V4DPI_PntType_Calendar:
		     dval = FixedFromGregorian(ds->year,ds->month,ds->day) ;
		     dblPNTv(respnt,dval) ;
		     goto chk_cal ;
		   case V4DPI_PntType_UYear:
		     ival = ds->year ; VCALADJYEAR(((di->ds.Cal.calFlags & VCAL_Flags_Historical) != 0),ival) ; intPNTv(respnt,ival) ;
		     goto chk_uyear ;
		   case V4DPI_PntType_UMonth:
//		     intPNTv(respnt,(ds->year - VCAL_BaseYear) * 12 + (ds->month - 1)) ;
		     intPNTv(respnt,YYMMtoUMONTH(ds->year,ds->month)) ;
		     goto chk_umonth ;
		} ;
		goto retOK1 ;
	   case VDTYPE_ODBC_TS:
		ts = (TIME_STRUCT *)pval ;
		switch (di->PointType)
		 { default:			v_Msg(ctx,ctx->ErrorMsgAux,"XDBUnSupCnv",vdType,di->DimId,di->PointType) ; goto fail ;
		   case V4DPI_PntType_UTime:
		     dval = ((ts->hour*60)+ts->minute)*60+ts->second ; dblPNTv(respnt,dval) ; respnt->PntType = di->PointType ;
		     break ;
		 } ;
		goto retOK1 ;
	   case VDTYPE_ODBC_TSS:
		tss = (TIMESTAMP_STRUCT *)pval ;
		switch (di->PointType)
		 { default:			v_Msg(ctx,ctx->ErrorMsgAux,"XDBUnSupCnv",vdType,di->DimId,di->PointType) ; goto fail ;
		   case V4DPI_PntType_UDate:
		     respnt->Value.IntVal = vcal_UDateFromYMD((di->ds.UDate.calFlags & VCAL_Flags_Historical),(tss->year < 1900 ? tss->year+1900 : tss->year),tss->month,tss->day,ctx->ErrorMsgAux) ;
		     if (respnt->Value.IntVal == VCAL_BadVal) goto fail ;
		     intPNT(respnt)
		     goto chk_udate ;
		   case V4DPI_PntType_Calendar:
		     dval = FixedFromGregorian(tss->year,tss->month,tss->day) + (double)(((tss->hour*60)+tss->minute)*60+tss->second) / (double)VCAL_SecsInDay ;
		     dblPNTv(respnt,dval) ;
		     goto chk_cal ;
		   case V4DPI_PntType_UDT:
		     intPNTv(respnt,vcal_UDTFromYMD(tss->year,tss->month,tss->day,tss->hour,tss->minute,tss->second,ctx->ErrorMsgAux)) ;
		     if (respnt->Value.IntVal == VCAL_BadVal) goto fail ;
		     goto chk_udt ;
		   case V4DPI_PntType_UTime:
		     dval = ((tss->hour*60)+tss->minute)*60+tss->second ; dblPNTv(respnt,dval) ; respnt->PntType = di->PointType ;
		     break ;
		   case V4DPI_PntType_UMonth:
//		     intPNTv(respnt,(tss->year - VCAL_BaseYear) * 12 + (tss->month - 1)) ;
		     intPNTv(respnt,YYMMtoUMONTH(tss->year,tss->month)) ;
		     goto chk_umonth ;
		   case V4DPI_PntType_UYear:
		     ival = tss->year ; VCALADJYEAR(FALSE,ival) ; intPNTv(respnt,ival) ; goto chk_uyear ;
		   case V4DPI_PntType_Char:
		   case V4DPI_PntType_UCChar:
		     ival = vcal_UDateFromYMD(FALSE,tss->year,tss->month,tss->day,ctx->ErrorMsgAux) ;
		     if (ival == VCAL_BadVal) goto fail ;
		     if (((tss->hour*60)+tss->minute)*60+tss->second == 0)
		      { ival = vcal_UDateFromYMD(FALSE,tss->year,tss->month,tss->day,ctx->ErrorMsgAux) ;
		        if (ival == VCAL_BadVal) goto fail ;
		        v_FormatDate(&ival,V4DPI_PntType_UDate,VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,NULL,&respnt->Value.UCVal[1]) ;
		      } else
		      { ival = vcal_UDTFromYMD(tss->year,tss->month,tss->day,tss->hour,tss->minute,tss->second,ctx->ErrorMsgAux) ;
		        if (ival == VCAL_BadVal) goto fail ;
		        UCstrcpy(&respnt->Value.UCVal[1],mscu_udt_to_ddmmmyyhhmmss(ival)) ;
		      } ;
		     uccharPNT(respnt) ; break ;
		 } ;
		goto retOK1 ;

	   case VDTYPE_ODBC_SNS:
		sns = (SQL_NUMERIC_STRUCT *)pval ;
		for(i=0,lval = 0;i<SQL_MAX_NUMERIC_LEN;i++)
		 { if (sns->val[i] == 0) continue ;
		   if (i >= sizeof(B64INT)) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvOverflow",UClit("SQL_NUMERIC_STRUCT"),di->DimId,di->PointType) ; goto fail ; } ;
		   lval |= ((B64INT)sns->val[i] << (i*8)) ;
		 } ;
		dval = (double)lval / powers[sns->scale] ; if (sns->sign == 0) { dval = -dval ; lval = -lval ; }
		if (di->PointType != V4DPI_PntType_Fixed) goto b8FP_entry ;
		fixPNTv(respnt,lval) ;
		goto retOK1 ;
#endif
#ifdef WANTMYSQL
	   case VDTYPE_MYSQL_TS:
		mts = (MYSQL_TIME *)pval ;
		switch (di->PointType)
		 { default:			v_Msg(ctx,ctx->ErrorMsgAux,"XDBUnSupCnv",vdType,di->DimId,di->PointType) ; goto fail ;
		   case V4DPI_PntType_UDate:
		     respnt->Value.IntVal = vcal_UDateFromYMD((di->ds.UDate.calFlags & VCAL_Flags_Historical),(mts->year < 1900 ? mts->year+1900 : mts->year),mts->month,mts->day,ctx->ErrorMsgAux) ;
		     if (respnt->Value.IntVal == VCAL_BadVal)
		      goto fail ;
		     intPNT(respnt)
		     goto chk_udate ;
		   case V4DPI_PntType_Calendar:
		     dval = FixedFromGregorian(mts->year,mts->month,mts->day) + (double)(((mts->hour*60)+mts->minute)*60+mts->second) / (double)VCAL_SecsInDay ;
		     dblPNTv(respnt,dval) ; respnt->PntType = di->PointType ;
		     break ;
		   case V4DPI_PntType_UDT:
		     intPNTv(respnt,vcal_UDTFromYMD(mts->year,mts->month,mts->day,mts->hour,mts->minute,mts->second,ctx->ErrorMsgAux)) ;
		     if (respnt->Value.IntVal == VCAL_BadVal) goto fail ;
		     goto chk_udt ;
		   case V4DPI_PntType_UTime:
		     dval = ((mts->hour*60)+mts->minute)*60+mts->second ; dblPNTv(respnt,dval) ; respnt->PntType = di->PointType ;
		     break ;
		   case V4DPI_PntType_UMonth:
//		     intPNTv(respnt,(mts->year - VCAL_BaseYear) * 12 + (mts->month - 1)) ; goto chk_umonth ;
		     intPNTv(respnt,YYMMtoUMONTH(mts->year,mts->month)) ; goto chk_umonth ;
		   case V4DPI_PntType_UYear:
		     ival = mts->year ; VCALADJYEAR(FALSE,ival) ; intPNTv(respnt,ival) ; goto chk_uyear ;
		   case V4DPI_PntType_Char:
		   case V4DPI_PntType_UCChar:
		     if (((mts->hour*60)+mts->minute)*60+mts->second == 0)
		      { ival = vcal_UDateFromYMD(FALSE,mts->year,mts->month,mts->day,ctx->ErrorMsgAux) ;
		        if (ival == VCAL_BadVal) goto fail ;
		        v_FormatDate(&ival,V4DPI_PntType_UDate,VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,NULL,&respnt->Value.UCVal[1]) ;
		      } else
		      { 
		        ival = vcal_UDTFromYMD(mts->year,mts->month,mts->day,mts->hour,mts->minute,mts->second,ctx->ErrorMsgAux) ;
		        if (ival == VCAL_BadVal) goto fail ;
		        UCstrcpy(&respnt->Value.UCVal[1],mscu_udt_to_ddmmmyyhhmmss(ival)) ;
		      } ;
		     uccharPNT(respnt) ; break ;
		 } ;
		goto retOK1 ;
#endif
#ifdef WANTORACLE
	   case VDTYPE_ORACLE_NUMERIC:
		sx = lenval >> 16 ; cx = lenval & 0xffff ; rs = (OCI_Resultset *)pval ;
		switch (di->PointType)
		 { default:			v_Msg(ctx,ctx->ErrorMsgAux,"XDBUnSupCnv",vdType,di->DimId,di->PointType) ; goto fail ;
		   case V4DPI_PntType_Int:	intPNTv(respnt,OCI_GetInt(rs,cx)) ; break ;
		   case V4DPI_PntType_Real:	dval = OCI_GetDouble(rs,cx) ; dblPNTv(respnt,dval) ; break ;
		   case V4DPI_PntType_Fixed:	lval = OCI_GetBigInt(rs,cx) ; fixPNTv(respnt,lval) ; break ;
		   case V4DPI_PntType_Dict:
		   case V4DPI_PntType_XDict:	
		   case V4DPI_PntType_Char:	
		   case V4DPI_PntType_UCChar:	
		   case V4DPI_PntType_Logical:	
		   case V4DPI_PntType_TeleNum:
		   case V4DPI_PntType_UDate:
		   case V4DPI_PntType_UYear:
		   case V4DPI_PntType_UTime:
		   case V4DPI_PntType_UMonth:
		   case V4DPI_PntType_UQuarter:
		   case V4DPI_PntType_UWeek:	ival = OCI_GetInt(rs,cx) ; goto b4SInt_entry ;
		 } ;
		goto retOK1 ;
	   case VDTYPE_ORACLE_TEXT:
		sx = lenval >> 16 ; cx = lenval & 0xffff ; rs = (OCI_Resultset *)pval ;
		pval = (BYTE *)OCI_GetString(rs,cx) ; lenval = strlen(pval) ; goto char_entry ;
	   case VDTYPE_ORACLE_TS:
		sx = lenval >> 16 ; cx = lenval & 0xffff ; rs = (OCI_Resultset *)pval ;
		ots = OCI_GetDate(rs,cx) ;
		switch (di->PointType)
		 { default:			v_Msg(ctx,ctx->ErrorMsgAux,"XDBUnSupCnv",vdType,di->DimId,di->PointType) ; goto fail ;
		   case V4DPI_PntType_UDate:
		     { int y,m,d ; OCI_DateGetDate(ots,&y,&m,&d) ;
		       respnt->Value.IntVal = vcal_UDateFromYMD((di->ds.UDate.calFlags & VCAL_Flags_Historical),y,m,d,ctx->ErrorMsgAux) ;
		       if (respnt->Value.IntVal == VCAL_BadVal) goto fail ;
		       intPNT(respnt)
		       goto chk_udate ;
		     }
		   case V4DPI_PntType_Calendar:
		     { int y,m,d,h,n,s ; OCI_DateGetDateTime(ots,&y,&m,&d,&h,&n,&s) ;
		       dval = FixedFromGregorian(y,m,d) + (double)(((h*60)+n)*60+s) / (double)VCAL_SecsInDay ;
		       dblPNTv(respnt,dval) ; respnt->PntType = di->PointType ;
		       break ;
		     }
		   case V4DPI_PntType_UDT:
		     { int y,m,d,h,n,s ; OCI_DateGetDateTime(ots,&y,&m,&d,&h,&n,&s) ;
		       intPNTv(respnt,vcal_UDTFromYMD(y,m,d,h,n,s,ctx->ErrorMsgAux)) ;
		       if (respnt->Value.IntVal == VCAL_BadVal) goto fail ;
		       goto chk_udt ;
		     }
		   case V4DPI_PntType_UTime:
		     { int y,m,d,h,n,s ; OCI_DateGetDateTime(ots,&y,&m,&d,&h,&n,&s) ;
		       dval = ((h*60)+n)*60+s ; dblPNTv(respnt,dval) ; respnt->PntType = di->PointType ;
		       break ;
		     }
		   case V4DPI_PntType_UMonth:
		     { int y,m,d ; OCI_DateGetDate(ots,&y,&m,&d) ;
		       intPNTv(respnt,YYMMtoUMONTH(y,m)) ; goto chk_umonth ;
		     }
		   case V4DPI_PntType_UYear:
		     { int y,m,d ; OCI_DateGetDate(ots,&y,&m,&d) ;
		       ival = y ; VCALADJYEAR(FALSE,ival) ; intPNTv(respnt,ival) ; goto chk_uyear ;
		     }
		   case V4DPI_PntType_Char:
		   case V4DPI_PntType_UCChar:
		     { int y,m,d,h,n,s ; OCI_DateGetDateTime(ots,&y,&m,&d,&h,&n,&s) ;
		       if (((h*60)+n)*60+s == 0)
		        { ival = vcal_UDateFromYMD(FALSE,y,m,d,ctx->ErrorMsgAux) ;
		          if (ival == VCAL_BadVal) goto fail ;
		          v_FormatDate(&ival,V4DPI_PntType_UDate,VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,NULL,&respnt->Value.UCVal[1]) ;
		        } else
		        { ival = vcal_UDTFromYMD(y,m,d,h,n,s,ctx->ErrorMsgAux) ;
		          if (ival == VCAL_BadVal) goto fail ;
		          UCstrcpy(&respnt->Value.UCVal[1],mscu_udt_to_ddmmmyyhhmmss(ival)) ;
		        } ;
		       uccharPNT(respnt) ; break ;
		     }
		 } ;
		goto retOK1 ;
#endif
#ifdef WANTV4IS
/*	N O T E - None of these fields are EOS terminated - have to do it explicitly !!! */
	   case VDTYPE_MIDAS_Log:
		bp = (char *)pval ;
		if (bp[lenval] != '\0') { pValTerm = &bp[lenval] ; cValTerm = bp[lenval] ; bp[lenval] = '\0' ; } ;
		bp = (char *)pval ;
		switch (di->PointType)
		 { default:			v_Msg(ctx,ctx->ErrorMsgAux,"XDBUnSupCnv",vdType,di->DimId,di->PointType) ; goto fail ;
		   case V4DPI_PntType_Logical:	ival = strtol(bp,&pValTerm,10) ; logPNTv(respnt,(ival != 0)) ; break ;
		 } ;
		goto retOK1 ;
//	   case VDTYPE_MIDAS_UDate:
////		bp = strchr((char *)pval,' ') ; if (bp != NULL) *bp = '\0' ;
//		bp = (char *)pval ;
//		if (bp[lenval] != '\0') { pValTerm = &bp[lenval] ; cValTerm = bp[lenval] ; bp[lenval] = '\0' ; } ;
//		{ char *ep = strchr((char *)pval,' ') ;
//		  if (ep != NULL)
//		   { if (pValTerm != NULL) *pValTerm = cValTerm ;	/* Reset prior end-of-string */
//		     pValTerm = ep ; cValTerm = *ep ;
//		     *ep = '\0' ;
//		   } ;
//		}
//		switch (di->PointType)
//		 { default:			v_Msg(ctx,ctx->ErrorMsgAux,"XDBUnSupCnv",vdType,di->DimId,di->PointType) ; goto fail ;
//		   case V4DPI_PntType_UDate:	UCstrcpyAtoU(utbuf1,bp) ; ucp = utbuf1 ;
//						if (UCempty(ucp)) { respnt->Value.IntVal = 0 ; }
//						 else { respnt->Value.IntVal = v_ParseUDate(ctx,ucp,V4LEX_TablePT_DDMMMYY,V4LEX_YMDOrder_DMY,((di->ds.UDate.calFlags & VCAL_Flags_Historical) == 0)) ; } ;
//						if (respnt->Value.IntVal == VCAL_BadVal) goto fail ;
//						ZPH(respnt) ; respnt->Dim = Dim_UDate ; respnt->PntType = V4DPI_PntType_UDate ; respnt->Bytes = V4PS_Int ; break ;
//		 } ;
//		goto retOK1 ;
	   case VDTYPE_MIDAS_UDT:
		bp = (char *)pval ;
		if (bp[lenval] != '\0') { pValTerm = &bp[lenval] ; cValTerm = bp[lenval] ; bp[lenval] = '\0' ; } ;
		switch (di->PointType)
		 { default:			v_Msg(ctx,ctx->ErrorMsgAux,"XDBUnSupCnv",vdType,di->DimId,di->PointType) ; goto fail ;
		   case V4DPI_PntType_UDT:	UCstrcpyAtoU(utbuf1,bp) ; ucp = utbuf1 ;
						if (UCempty(ucp)) { respnt->Value.IntVal = 0 ; }
						 else { respnt->Value.IntVal = v_ParseUDT(ctx,ucp,V4LEX_TablePT_DDMMMYY,V4LEX_YMDOrder_DMY,UNUSED,NULL,((di->ds.UDT.calFlags & VCAL_Flags_Historical) == 0)) ; } ;
						if (respnt->Value.IntVal == VCAL_BadVal) goto fail ;
						ZPH(respnt) ; respnt->Dim = Dim_UDT ; respnt->PntType = V4DPI_PntType_UDT ; respnt->Bytes = V4PS_Int ; break ;
		 } ;
		goto retOK1 ;
//	   case VDTYPE_MIDAS_Zip:
//		bp = (char *)pval ;
//		if (bp[lenval] != '\0') { pValTerm = &bp[lenval] ; cValTerm = bp[lenval] ; bp[lenval] = '\0' ; } ;
//		{ int zip, zip5, zip4 ; char *term ;
//		  zip = strtol(bp,&term,10) ; zip5 = (zip > 99999 ? zip / 10000 : zip) ; zip4 = (zip > 99999 ? zip % 10000 : 0) ;
//		  switch (di->PointType)
//		   { default:			v_Msg(ctx,ctx->ErrorMsgAux,"XDBUnSupCnv",vdType,di->DimId,di->PointType) ; goto fail ;
//		      CASEofChar
//			if (zip4 == 0) { UCsprintf(&respnt->Value.UCVal[1],40,UClit("%05d"),zip5) ; }
//			 else { UCsprintf(&respnt->Value.UCVal[1],40,UClit("%05d-%04d"),zip5,zip4) ; } ;
//			uccharPNT(respnt) ; UCCHARPNTBYTES1(respnt) ; break ;
//		      case V4DPI_PntType_Int:
//			intPNTv(respnt,(zip4 == 0 ? zip5 : zip)) ;
//			break ;
//		   } ;
//		 } ;
//		goto retOK1 ;
	   case VDTYPE_MIDAS_Char:
	   case VDTYPE_MIDAS_Int:
	   case VDTYPE_MIDAS_Real:		goto char_entry ;
#endif
	 } ;

retOK1:	respnt->Dim = di->DimId ;
retOK:	if (ucpValTerm != NULL) { *ucpValTerm = ucValTerm ; }
	 else if (pValTerm != NULL) { *pValTerm = cValTerm ; } ;
	return(respnt) ;

fail:	if (ucpValTerm != NULL) { *ucpValTerm = ucValTerm ; }
	 else if (pValTerm != NULL) { *pValTerm = cValTerm ; } ;
	return(NULL) ;
/*	Here to perform common date-range checks */
chk_udate:
	if ((di->ds.UDate.calFlags & VCAL_Flags_Historical) != 0)
	 { if (respnt->Value.IntVal > valUDATEisNOW) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvFuture",respnt,di->DimId) ; goto fail ; } ; } ;
	respnt->PntType = di->PointType ; goto retOK1 ;

#ifdef WANTODBC
chk_cal:
	if ((di->ds.Cal.calFlags & VCAL_Flags_Historical) != 0)
	 { if (dval > UDtoCAL(valUDATEisNOW)) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvFuture",respnt,di->DimId) ; goto fail ; } ; } ;
	respnt->PntType = di->PointType ; goto retOK1 ;
#endif

chk_uyear:
	if ((di->ds.UYear.calFlags & VCAL_Flags_Historical) != 0)
	 { if (respnt->Value.IntVal > gpi->curYear) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvFuture",respnt,di->DimId) ; goto fail ; } ; } ;
	respnt->PntType = di->PointType ; goto retOK1 ;

chk_umonth:
	if ((di->ds.UMonth.calFlags & VCAL_Flags_Historical) != 0)
	 { if (respnt->Value.IntVal > UDtoUDT(valUDATEisNOW)) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvFuture",respnt,di->DimId) ; goto fail ; } ; } ;
	respnt->PntType = di->PointType ; goto retOK1 ;

chk_udt:
	if ((di->ds.UDT.calFlags & VCAL_Flags_Historical) != 0)
	 { if (respnt->Value.IntVal > UDtoUDT(valUDATEisNOW)) { v_Msg(ctx,ctx->ErrorMsgAux,"CnvFuture",respnt,di->DimId) ; goto fail ; } ; } ;
	respnt->PntType = di->PointType ; goto retOK1 ;
}

/*	v4dpi_ValSpclAcptr - Calls special dimension acceptor with given value, returns parsed point	*/
/*	Call: point = v4dpi_ValSpclAcptr( ctx , di , aval , ucval , respnt )
	  where point is returned parsed point (NULL if error in ctx->ErrorMsgAux),
		ctx is context,
		di is dimension info,
		aval is value to parse (ASCII) - one of these should be NULL,
		ucval is value to parse (Unicode) - one of these should be NULL,
		respnt - point to be updated								*/

LOGICAL v4dpi_ValSpclAcptr(ctx,di,aval,ucval,respnt)
  struct V4C__Context *ctx ;
  struct V4DPI__DimInfo *di ;
  char *aval ; UCCHAR *ucval ;
  P *respnt ;
{
  struct V4LEX__TknCtrlBlk *tcbx ;
  P isctbuf,*ipt ;
  UCCHAR arg[512],valbuf[512] ;

	if (aval != NULL)		/* Do we have ASCII or Unicode ? */
	 { if (strlen(aval) > 400) return(FALSE) ;
	   v_StringLit(ASCretUC(aval),valbuf,UCsizeof(arg),'"','\\') ;
	 } else
	 { if (UCstrlen(ucval) > 400) return(FALSE) ;
	   v_StringLit(ucval,valbuf,UCsizeof(arg),'"','\\') ;
	 } ;
	if (di->ADPnt.Bytes != 0)
	 { v_Msg(ctx,arg,"@[UV4:Acceptor %1U Alpha:%2U]",di->ADPntStr,valbuf) ; }
	 else { v_Msg(ctx,arg,"@[UV4:Acceptor Dim:%1U Alpha:%2U]",(UCstrlen(di->IsA) > 0 ? di->IsA : di->DimName),valbuf) ; } ;
	tcbx = v4mm_AllocChunk(sizeof *tcbx,FALSE) ; v4lex_InitTCB(tcbx,0) ; v4lex_NestInput(tcbx,NULL,arg,V4LEX_InpMode_String) ;
	if(!v4dpi_PointParse(ctx,&isctbuf,tcbx,V4DPI_PointParse_RetFalse)) { v4lex_FreeTCB(tcbx) ; return(FALSE) ; } ;
	v4lex_FreeTCB(tcbx) ;
	ipt = v4dpi_IsctEval(respnt,&isctbuf,ctx,0,NULL,NULL) ;
	if (ipt == NULL) return(FALSE) ;
/*	If result is same point type then make sure it's the correct dimension, otherwise take as-is */
	if (ipt->PntType == di->PointType) ipt->Dim = di->DimId ;
	return(TRUE) ;
}

#define TKNCHAR vjson_FormatErrorEnvironment(vjpi)
UCCHAR *vjson_FormatErrorEnvironment(vjpi)
  struct VJSON__ParseInfo *vjpi ;
{ static UCCHAR env[128] ; INDEX i,cx,dx,x ;
  x = (vjpi->tcb->ilvl[vjpi->tcb->ilx].input_ptr == NULL ? -1 : vjpi->tcb->ilvl[vjpi->tcb->ilx].input_ptr - vjpi->tcb->ilvl[vjpi->tcb->ilx].input_str) ;
  if (x >= 0) { UCsprintf(env,UCsizeof(env),UClit("around character %d: "),x) ; }
   else { return(UClit("at unknown location but probably end of string")) ; } ;
  for(i=0,cx=(x > 50 ? x - 50 : 1),dx=UCstrlen(env);i<100 && vjpi->tcb->ilvl[vjpi->tcb->ilx].input_str[cx]!=UCEOS;i++)
   { if (cx != x) { env[dx++] = vjpi->tcb->ilvl[vjpi->tcb->ilx].input_str[cx++] ; continue ; } ;
     env[dx++] = UClit('<') ; env[dx++] = vjpi->tcb->ilvl[vjpi->tcb->ilx].input_str[cx++] ; env[dx++] = UClit('>') ;
   } ;
  env[dx++] = UCEOS ;
  if (vjpi->tcb->ilvl[vjpi->tcb->ilx].input_str[cx] == UCEOS) { UCstrcat(env,UClit("*EOS*")) ; } ;
  return(env) ;
} 

/*	vjson_ParseString - Parses JSON string into VJSON__Blob and links to current process	*/
/*	Call: dictId = vjson_ParseString( ctx , tcb , tlName, parseFlags )
	  where	dictId is non-zero if OK, 0 if problem (in ctx->ErrorMsgAux),
		ctx is context,
		tcb is token control block with source,
		tlName is top-level name used only if one not specified in source,
		parseFlags are various parsing flags (see VJON_Parse_xxx)			*/

DICTID vjson_ParseString(ctx,tcb,tlName,parseFlags)
  struct V4C__Context *ctx ;
  struct V4LEX__TknCtrlBlk *tcb ;
  UCCHAR *tlName ;
  FLAGS32 parseFlags ;
{ struct VJSON__ParseInfo vjpiBuf, *vjpi ;
  static COUNTER unnamedCount ;

#define NTKN(flags,loc) { v4lex_NextTkn(vjpi->tcb,((flags)|V4LEX_Option_JSON)) ; if (vjpi->tcb->type == V4LEX_TknType_Error) { v_Msg(vjpi->ctx,vjpi->ctx->ErrorMsgAux,"JSONSyntax",loc,TKNCHAR) ; goto fail ; } ; }
#define TKNPUSH { v4lex_NextTkn(vjpi->tcb,V4LEX_Option_PushCur) ; }

/*	Allocate (setup) initial structures */
	vjpi = &vjpiBuf ;
	vjpi->vjblob = (struct VJSON__Blob *)v4mm_AllocChunk(VJSON_Blob_Max,FALSE) ;
	vjpi->vjblob->bMax = VJSON_Blob_Max ; vjpi->vjblob->bRemain = VJSON_Blob_Max - (sizeof *vjpi->vjblob - 1) ; vjpi->vjblob->bIndex = 0 ; vjpi->vjblob->vjblobNext = NULL ;
	DIMINFO(vjpi->di,ctx,Dim_NId) ;
	vjpi->ctx = ctx ; vjpi->tcb = tcb ;

/*	The JSON string must start with a name, if not default to UNNAMEDnnn */
	NTKN(0,101)

	if (tcb->type == V4LEX_TknType_Keyword || tcb->type == V4LEX_TknType_String)
	 { vjpi->vjblob->dictId = v4dpi_DictEntryGet(vjpi->ctx,Dim_NId,vjpi->tcb->UCstring,vjpi->di,NULL) ; NTKN(0,102)
	   if (tcb->opcode == V_OpCode_EOF)
	    { v_Msg(vjpi->ctx,ctx->ErrorMsgAux,"JSONHitEOF",TKNCHAR) ; goto fail ; } ;
	   if (!(tcb->opcode == V_OpCode_Colon || tcb->opcode == V_OpCode_NCColon))
	    { v_Msg(vjpi->ctx,ctx->ErrorMsgAux,"JSONNotColon",TKNCHAR) ; goto fail ; } ;
	 } else
	 { UCCHAR unnamed[V4DPI_DictEntryVal_Max+1] ;
	   if (tlName == NULL) UCsprintf(unnamed,UCsizeof(unnamed),UClit("!!unnamed-%03d"),++unnamedCount) ;
	    vjpi->vjblob->dictId = v4dpi_DictEntryGet(vjpi->ctx,Dim_NId,(tlName == NULL ? unnamed : tlName),vjpi->di,NULL) ;
	    TKNPUSH
	 } ;
	{ LOGICAL ok ; vjpi->vjblob->topVal = vjson_GetValue(vjpi,&ok,parseFlags) ; if (!ok) goto fail ; }
//	vjson_SerializeBlob(ctx,vjpi->vjblob,UCTBUF1,50) ; vout_UCText(VOUT_Trace,0,UCTBUF1) ; vout_NL(VOUT_Trace) ; 
/*	Add this blob to chain associated with process so we can reference later on */
	if (gpi->vjblob != NULL) vjpi->vjblob->vjblobNext = gpi->vjblob ;
	gpi->vjblob = vjpi->vjblob ;
	return(vjpi->vjblob->dictId) ;

fail:	v4mm_FreeChunk(vjpi->vjblob) ;
	return(0) ;
}

/*	vjson_GetValue - recursive routine to parse string into current blob	*/
struct VJSON__Value vjson_GetValue(vjpi,ok,parseFlags)
  struct VJSON__ParseInfo *vjpi ;
  LOGICAL *ok,parseFlags ;
{
  struct VJSON__Value vjval ;

//{ char foo[20] ;
//  strncpy(foo,UCretASC(vjpi->tcb->ilvl[vjpi->tcb->ilx].input_ptr),15) ; foo[15] = 0 ; printf("GetValue: %s\n",foo) ;
//}

	*ok = TRUE ; vjval.wIndex = 0 ;
	NTKN(V4LEX_Option_NegLit,103)
	switch(vjpi->tcb->opcode)
	 { default:
		v_Msg(vjpi->ctx,vjpi->ctx->ErrorMsgAux,"JSONSyntax",2,TKNCHAR) ; goto fail ;
	   case V_OpCode_EOF:
		v_Msg(vjpi->ctx,vjpi->ctx->ErrorMsgAux,"JSONHitEOF",TKNCHAR) ; goto fail ;
	   case V_OpCode_NCLBrace:
	   case V_OpCode_LBrace:
		{ struct VJSON__Value_Object vjo ;
		  for(vjo.count=1;;vjo.count++)
		   { if (vjo.count > VJSON_Value_ArrayMax) { v_Msg(vjpi->ctx,vjpi->ctx->ErrorMsgAux,"JSONArrayTooBig",VJSON_Value_ObjectMax,TKNCHAR) ; goto fail ; } ;
		     NTKN(0,104)
		     if (vjo.count == 1 && vjpi->tcb->opcode == V_OpCode_RBrace) { vjo.count = 0 ; break ; } ;	/* Check for empty object */
		     switch (vjpi->tcb->type)
		      { default:
		        case V4LEX_TknType_Keyword:
			case V4LEX_TknType_String:
			  vjo.objEntry[vjo.count-1].dictId = v4dpi_DictEntryGet(vjpi->ctx,Dim_NId,vjpi->tcb->UCstring,vjpi->di,NULL) ; break ;
		      } ;
		     NTKN(0,105)
		     if (!(vjpi->tcb->opcode == V_OpCode_Colon || vjpi->tcb->opcode == V_OpCode_NCColon))
		      { v_Msg(vjpi->ctx,vjpi->ctx->ErrorMsgAux,"JSONSyntax",3,TKNCHAR) ; goto fail ; } ;
		     vjo.objEntry[vjo.count-1].jsonVal = vjson_GetValue(vjpi,ok,parseFlags) ; if (!*ok) return(vjval) ;
		     NTKN(0,106)
		     switch (vjpi->tcb->opcode)
		      { default:		v_Msg(vjpi->ctx,vjpi->ctx->ErrorMsgAux,"JSONSyntax",4,TKNCHAR) ; goto fail ;
		        case V_OpCode_EOF:	v_Msg(vjpi->ctx,vjpi->ctx->ErrorMsgAux,"JSONHitEOF",TKNCHAR) ; goto fail ;
		        case V_OpCode_CommaComma:
						if((parseFlags & VJSON_Parse_RelaxComma) == 0) { v_Msg(vjpi->ctx,vjpi->ctx->ErrorMsgAux,"JSONSyntax",6,TKNCHAR) ; goto fail ; } ;
		        case V_OpCode_Comma:	if((parseFlags & VJSON_Parse_RelaxComma) != 0)		/* If TRUE then ignore extra commas in JSON */
						 { for(;;) { NTKN(0,107) ; if (!(vjpi->tcb->opcode == V_OpCode_Comma || vjpi->tcb->opcode == V_OpCode_CommaComma)) break ; } ;
						   if (vjpi->tcb->opcode == V_OpCode_RBrace) break ;
						   v4lex_NextTkn(vjpi->tcb,V4LEX_Option_PushCur) ; continue ;
						 } else { continue ; } ;
			case V_OpCode_RBrace:	break ;
		      } ;
		     break ;
		   } ;
		 vjval.jvType = VJSON_Type_Object ; vjval.wIndex = vjson_StashValue(vjpi,&vjo,(char *)&vjo.objEntry[vjo.count] - (char *)&vjo) ;
		}
		break ;
	   case V_OpCode_LRBracket:
		{ struct VJSON__Value_Array vja ;
		  vja.count = 0 ; vjval.jvType = VJSON_Type_Array ; vjval.wIndex = vjson_StashValue(vjpi,&vja,(char *)&vja.arrayVal[vja.count] - (char *)&vja) ;
		  break ;
		}
	   case V_OpCode_NCLBracket:
	   case V_OpCode_LBracket:
		{ struct VJSON__Value_Array vja ;
/*		  Check for "]" - empty array */
		  NTKN(0,1080)
		  if (vjpi->tcb->opcode != V_OpCode_RBracket) { v4lex_NextTkn(vjpi->tcb,V4LEX_Option_PushCur) ; }
		   else { vja.count = 0 ; vjval.jvType = VJSON_Type_Array ; vjval.wIndex = vjson_StashValue(vjpi,&vja,(char *)&vja.arrayVal[vja.count] - (char *)&vja) ;
			  break ;
			} ;
		  for(vja.count=1;;vja.count++)
		   { if (vja.count > VJSON_Value_ArrayMax) { v_Msg(vjpi->ctx,vjpi->ctx->ErrorMsgAux,"JSONArrayTooBig",vjpi->tcb->type,VJSON_Value_ArrayMax) ; goto fail ; } ;
		     vja.arrayVal[vja.count-1] = vjson_GetValue(vjpi,ok,parseFlags) ; if (!*ok) return(vjval) ;	
		     NTKN(0,108)
		     switch (vjpi->tcb->opcode)
		      { default:		v_Msg(vjpi->ctx,vjpi->ctx->ErrorMsgAux,"JSONSyntax",5,TKNCHAR) ; goto fail ;
		        case V_OpCode_CommaComma:
						if((parseFlags & VJSON_Parse_RelaxComma) == 0) { v_Msg(vjpi->ctx,vjpi->ctx->ErrorMsgAux,"JSONSyntax",6,TKNCHAR) ; goto fail ; } ;
		        case V_OpCode_Comma:	if((parseFlags & VJSON_Parse_RelaxComma) != 0)				/* If TRUE then ignore extra commas in JSON */
						 { for(;;) { NTKN(0,109) ; if (!(vjpi->tcb->opcode == V_OpCode_Comma || vjpi->tcb->opcode == V_OpCode_CommaComma)) break ; } ;
						   if (vjpi->tcb->opcode == V_OpCode_RBracket) break ;
						   v4lex_NextTkn(vjpi->tcb,V4LEX_Option_PushCur) ; continue ;
						 } else { continue ; } ;
			case V_OpCode_RBracket:	break ;
		      } ;
		     break ;
		   } ;
		 vjval.jvType = VJSON_Type_Array ; vjval.wIndex = vjson_StashValue(vjpi,&vja,(char *)&vja.arrayVal[vja.count] - (char *)&vja) ;
		}
		break ;
	   case V_OpCode_Keyword:
		{ if (UCstrcmp(vjpi->tcb->UCstring,UClit("true")) == 0) { vjval.jvType = VJSON_Type_Logical ; vjval.wIndex = TRUE ; }
		   else if (UCstrcmp(vjpi->tcb->UCstring,UClit("false")) == 0) { vjval.jvType = VJSON_Type_Logical ; vjval.wIndex = FALSE ; }
		   else if (UCstrcmp(vjpi->tcb->UCstring,UClit("null")) == 0) { vjval.jvType = VJSON_Type_Null ; vjval.wIndex = FALSE ; }
		   else { v_Msg(vjpi->ctx,vjpi->ctx->ErrorMsgAux,"JSONToken",vjpi->tcb->UCstring,TKNCHAR) ; goto fail ; } ;
		}
		break ;
	   case V_OpCode_QString:	
	   case V_OpCode_DQString:
		{ struct VJSON__Value_String vjs ;
		  UCstrcpy(vjs.strVal,vjpi->tcb->UCstring) ;
		  vjval.jvType = VJSON_Type_String ; vjval.wIndex = vjson_StashValue(vjpi,&vjs,((char *)&vjs.strVal[UCstrlen(vjs.strVal)] - (char *)&vjs.strVal)+1) ;
		} break ;
	   case V_OpCode_DQUString:
		{ struct VJSON__Value_String vjs ;
		  UCstrcpy(vjs.strVal,vjpi->tcb->UCstring) ;
		  vjval.jvType = VJSON_Type_String ; vjval.wIndex = vjson_StashValue(vjpi,&vjs,((char *)&vjs.strVal[UCstrlen(vjs.strVal)] - (char *)&vjs.strVal)+1) ;
		} break ;
	   case V_OpCode_Numeric:
		if (*vjpi->tcb->prior_input_ptr == UClit('0') && (parseFlags & VJSON_Parse_LeadingZero) != 0)
		 { struct VJSON__Value_String vjs ;		/* If numeric starts with leading '0' then force to keyword so we keep any leading 0's (for zipcodes, passwords, etc.) */
		   TKNPUSH ; NTKN(V4LEX_Option_ForceKW,110)
		   UCstrcpy(vjs.strVal,vjpi->tcb->UCstring) ;
		   vjval.jvType = VJSON_Type_String ; vjval.wIndex = vjson_StashValue(vjpi,&vjs,((char *)&vjs.strVal[UCstrlen(vjs.strVal)] - (char *)&vjs.strVal)+1) ;
		   break ;
		 } ;
		if (vjpi->tcb->type == V4LEX_TknType_Float || vjpi->tcb->type == V4LEX_TknType_LInteger || vjpi->tcb->decimal_places > 0)
		 { struct VJSON__Value_Double vjd ;
		   if (vjpi->tcb->type == V4LEX_TknType_Float) { vjd.dblVal = vjpi->tcb->floating ; } else { FIXEDtoDOUBLE(vjd.dblVal,vjpi->tcb->Linteger,vjpi->tcb->decimal_places) ; } ;
		   vjval.jvType = VJSON_Type_Double ; vjval.wIndex = vjson_StashValue(vjpi,&vjd,sizeof vjd) ;
		 } else
		 { struct VJSON__Value_Int vji ;
		   vji.intVal = vjpi->tcb->integer ;
		   vjval.jvType = VJSON_Type_Int ; vjval.wIndex = vjson_StashValue(vjpi,&vji,sizeof vji) ;
		 } ;
		break ;
	 } ;
	return(vjval) ;

fail:	*ok = FALSE ; return(vjval) ;
}

/*	vjson_StashValue - Stores value-bytes in current blob as described by vjpi */
INDEX vjson_StashValue(vjpi,value,bytes)
  struct VJSON__ParseInfo *vjpi ;
  void *value ;
  LENMAX bytes ;
{ INDEX x ;
	bytes = ALIGN(bytes) ;
	if (bytes >= vjpi->vjblob->bRemain)
	 { LENMAX inc = ((bytes / vjpi->vjblob->bMax) + 1) * vjpi->vjblob->bMax ;
	   vjpi->vjblob->bMax += inc ; vjpi->vjblob->bRemain += inc ;
	   vjpi->vjblob = realloc(vjpi->vjblob,vjpi->vjblob->bMax) ;
	 } ;
	x = vjpi->vjblob->bIndex ; vjpi->vjblob->bIndex += bytes ; vjpi->vjblob->bRemain -= bytes ;
	memcpy(&vjpi->vjblob->blob[x],value,bytes) ;
	return(x) ;
}

/*	vjson_serializeValue - Takes JSON blob value and converts to printable string		*/
/*	Call: len = vjson_SerializeValue( ctx, vjblob, vjval, dst, dstBytes )
	  where	len is length of the resulting string, UNUSED if problem (dst not big enough),
		vjblob is pointer to JSON blob,
		vjval is pointer to specific value in blob to be serialized,
		dst is destination UCCHAR buffer,
		dstBytes is size of the buffer						*/

LENMAX vjson_SerializeValue(ctx,vjblob,vjval,dst,dstBytes)
  struct V4C__Context *ctx ;
  struct VJSON__Blob *vjblob ;
  struct VJSON__Value *vjval ;
  UCCHAR *dst ;
  LENMAX dstBytes ;
{
	switch (vjval->jvType)
	 { default:
	   case VJSON_Type_Int:
	     { struct VJSON__Value_Int *vji ; vji = (struct VJSON__Value_Int *)&vjblob->blob[vjval->wIndex] ; if (dstBytes < 32) return(UNUSED) ;  UCsprintf(dst,dstBytes,UClit("%d"),vji->intVal) ; return(UCstrlen(dst)) ;
	     }
	   case VJSON_Type_Double:
	     { struct VJSON__Value_Double *vjd ; UCCHAR *b,*sig ;
	       vjd = (struct VJSON__Value_Double *)&vjblob->blob[vjval->wIndex] ; if (dstBytes < 32) return(UNUSED) ; 
	       UCsprintf(dst,dstBytes,UClit("%f"),vjd->dblVal) ;
/*	       Maybe trim off trailing 0's */
	       for(b=dst,sig=NULL;*b!=UCEOS;b++)
	        { if (*b == UClit('.')) sig = b ;
		  if (sig != NULL && *b != UClit('0')) sig = b ;
		} ; if (sig != NULL) *(sig+1) = UCEOS ;
	       return(UCstrlen(dst)) ;
	     }
	   case VJSON_Type_String:
	     { struct VJSON__Value_String *vjs ; vjs = (struct VJSON__Value_String *)&vjblob->blob[vjval->wIndex] ;
//	       len = UCstrlen(vjs->strVal) ; if (dstBytes <= len) return(UNUSED) ;  UCstrcat(dst,vjs->strVal) ; return(len) ;
	       return(v_StringLit(vjs->strVal,dst,dstBytes,'"','\\')) ;
	     }
	   case VJSON_Type_Logical:
	     if (dstBytes <= 5) return(UNUSED) ;
	     UCstrcpy(dst,(vjval->wIndex == TRUE ? UClit("true") : UClit("false"))) ; 
	     return(UCstrlen(dst)) ;
	   case VJSON_Type_Null:
	     if (dstBytes <= 5) return(UNUSED) ;
	     UCstrcpy(dst,UClit("null")) ; 
	     return(UCstrlen(dst)) ;
	   case VJSON_Type_Array:
	     { struct VJSON__Value_Array *vja ; LENMAX len=0, tlen ; INDEX i ;
	       vja = (struct VJSON__Value_Array *)&vjblob->blob[vjval->wIndex] ; if (dstBytes < 10) return(UNUSED) ;
	       UCstrcat(dst,UClit("[")) ; len++ ;
	       for (i=0;i<vja->count;i++)
	        { if (len+5 > dstBytes) return(UNUSED) ; if (i > 0) { UCstrcat(&dst[len],UClit(",")) ; len++ ; } ;
		  tlen = vjson_SerializeValue(ctx,vjblob,&vja->arrayVal[i],&dst[len],dstBytes-len) ;
		  if (tlen == UNUSED) return(UNUSED) ;
		  len += tlen ;
		} ;
	       if (len+5 >= dstBytes) return(UNUSED) ;
	       UCstrcat(&dst[len],UClit("]")) ; len++ ;
	       return(len) ;
	     } break ;
	   case VJSON_Type_Object:
	     { struct VJSON__Value_Object *vjo ; LENMAX len=0, tlen ; INDEX i ;
	       vjo = (struct VJSON__Value_Object *)&vjblob->blob[vjval->wIndex] ; if (dstBytes < 10) return(UNUSED) ;
	       UCstrcat(dst,UClit("{")) ; len++ ;
	       for (i=0;i<vjo->count;i++)
	        { if (len+5+V4DPI_DictEntryVal_Max > dstBytes) return(UNUSED) ; if (i > 0) { UCstrcat(&dst[len],UClit(",")) ; } ;
		  UCstrcat(&dst[len],UClit("\"")) ; UCstrcat(&dst[len],v4dpi_RevDictEntryGet(ctx,vjo->objEntry[i].dictId)) ; UCstrcat(&dst[len],UClit("\":")) ; len += UCstrlen(&dst[len]) ;
		  tlen = vjson_SerializeValue(ctx,vjblob,&vjo->objEntry[i].jsonVal,&dst[len],dstBytes-len) ;
		  if (tlen == UNUSED) return(UNUSED) ;
		  len += tlen ;
		} ;
	       if (len+5 >= dstBytes) return(UNUSED) ;
	       UCstrcat(&dst[len],UClit("}")) ; len++ ;
	       return(len) ;
	     } break ;
	 } ;
}

/*	vjson_serializeBlob - Takes JSON blob value and converts to printable string		*/
/*	Call: len = vjson_SerializeValue( ctx, vjblob, dst, dstBytes )
	  where	len is positive length if all is well, UNUSED if problems (usually dst too small),
		vjblob is pointer to JSON blob,
		dst is destination UCCHAR buffer,
		dstBytes is size of the buffer						*/

LENMAX vjson_SerializeBlob(ctx,vjblob,dst,dstBytes)
  struct V4C__Context *ctx ;
  struct VJSON__Blob *vjblob ;
  UCCHAR *dst ;
  LENMAX dstBytes ;
{ LENMAX len ;

	if (dstBytes < V4DPI_DictEntryVal_Max + 10) return(UNUSED) ;
/*	Start with top level name followed with a colon */
	UCstrcpy(dst,UClit("\"")) ; UCstrcpy(dst,v4dpi_RevDictEntryGet(ctx,vjblob->dictId)) ; UCstrcat(dst,UClit("\":")) ;
	len = UCstrlen(dst) ;
	return(vjson_SerializeValue(ctx,vjblob,&vjblob->topVal,&dst[len],dstBytes-len)) ;
}

struct VJSON__Blob *vjson_LocateBlob(ctx,refPt,vjval,objName)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *refPt ;
  struct VJSON__Value *vjval ;
  DICTID *objName ;
{ struct V4DPI__DimInfo *di ;
  struct V4L__ListPoint *lp ;
  struct V4L__ListJSON *ljson ;
  struct VJSON__Blob *vjblob ;
  DICTID dictId ; LOGICAL ok ;

	switch (refPt->PntType)
	 { default:
	   CASEofCharmU
	   case V4DPI_PntType_UCChar:
		v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,refPt,ctx) ; DIMINFO(di,ctx,Dim_NId) ;
		dictId = v4dpi_DictEntryGet(ctx,Dim_NId,UCTBUF1,di,NULL) ;
	   case V4DPI_PntType_Int:		/* If integer then propably Dim:Int value of unnamed blob */
	   case V4DPI_PntType_Dict:
		dictId = refPt->Value.IntVal ; break ;
	   case V4DPI_PntType_List:
		lp = (struct V4L__ListPoint *)&refPt->Value ; if (lp->ListType != V4L_ListType_JSON) return(NULL) ;
		ljson = (struct V4L__ListJSON *)&lp->Buffer ;
		*vjval = ljson->vjval ; *objName = ljson->objName ;
		return(ljson->vjblob) ;
	 } ;
	for (vjblob=gpi->vjblob;vjblob!=NULL;vjblob=vjblob->vjblobNext)
	 { if (vjblob->dictId == dictId) break ; } ;
	return(vjblob) ;
}

LOGICAL vjson_DeleteBlob(ctx,refPt)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *refPt ;
{ struct V4DPI__DimInfo *di ;
  struct VJSON__Blob *vjblob, *vjblobPrior ;
  DICTID dictId ; LOGICAL ok ;

	switch (refPt->PntType)
	 { default:
	   CASEofCharmU
	   case V4DPI_PntType_UCChar:
		v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,refPt,ctx) ; DIMINFO(di,ctx,Dim_NId) ;
		dictId = v4dpi_DictEntryGet(ctx,Dim_NId,UCTBUF1,di,NULL) ;
	   case V4DPI_PntType_Int:		/* If integer then propably Dim:Int value of unnamed blob */
	   case V4DPI_PntType_Dict:
		dictId = refPt->Value.IntVal ; break ;
	 } ;
	for (vjblobPrior=NULL,vjblob=gpi->vjblob;vjblob!=NULL;vjblobPrior=vjblob,vjblob=vjblob->vjblobNext)
	 { if (vjblob->dictId != dictId) continue ;
	   if (vjblobPrior == NULL) { gpi->vjblob = vjblob->vjblobNext ; }
	    else { vjblobPrior->vjblobNext = vjblob->vjblobNext ; } ;
	   v4mm_FreeChunk(vjblob) ;
	   return(TRUE) ;
	 } ;
	return(FALSE) ;
}


P *vjson_Dereference(ctx,respnt,vjblob,vjval,argpnts,argcnt,argStart,intmodx,dimAsName)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *respnt ;
  struct VJSON__Blob *vjblob ;
  struct VJSON__Value *vjval ;
  struct V4DPI__Point *argpnts[] ;
  COUNTER argcnt ;
  INDEX argStart ;
  INTMODX intmodx ;
  LOGICAL dimAsName ;
{ struct V4DPI__DimInfo *diRes = NULL ;
  struct V4DPI__DimInfo *di ; 
  static DICTID dictElementNames = UNUSED, dictLength, dictSerialize, dictTypeOf, dictUndefined, dictNameOf ;
  INDEX ix ; LOGICAL ok,doUpdate ; DICTID lastName ;

/*	Return UV4:undefined if last argument is '.typeOf' otherwise return NULL */
#define RETUNDEFINED \
 if(argpnts[argcnt]->PntType == V4DPI_PntType_Dict && argpnts[argcnt]->Value.IntVal == dictTypeOf) \
  { vjval->jvType = VJSON_Type_Undefined ; return(vjson_TypeOf(ctx,respnt,vjval)) ; } else { return(NULL) ; } ;

	ZPH(respnt) ; DIMINFO(di,ctx,Dim_NId) ; lastName = UNUSED ; doUpdate = FALSE ;
	if (dictElementNames == UNUSED)
	 { dictElementNames = v4dpi_DictEntryGet(ctx,Dim_NId,UClit("elementNames"),di,NULL) ; dictLength = v4dpi_DictEntryGet(ctx,Dim_NId,UClit("length"),di,NULL) ;
	   dictSerialize = v4dpi_DictEntryGet(ctx,Dim_NId,UClit("serialize"),di,NULL) ; dictTypeOf = v4dpi_DictEntryGet(ctx,Dim_NId,UClit("typeof"),di,NULL) ;
	   dictUndefined = v4dpi_DictEntryGet(ctx,Dim_NId,UClit("undefined"),di,NULL) ; dictNameOf = v4dpi_DictEntryGet(ctx,Dim_NId,UClit("nameof"),di,NULL) ;
	 } ;
	if (argStart <= 0) argStart = 1 ;
	if (vjblob != NULL && vjval->jvType == VJSON_Type_None) vjval = &vjblob->topVal ;
	for(ix=argStart,ok=TRUE;ok&&ix<=argcnt;ix++)
	 {
/*	   If don't yet have blob then first argument better be name of known blob */
	   if (vjblob == NULL)
	    { static struct VJSON__Value vjval1 ; vjval1.jvType = VJSON_Type_None ;
	      vjblob = vjson_LocateBlob(ctx,argpnts[ix],&vjval1,&lastName) ;
	      if (vjblob == NULL) { v_Msg(ctx,NULL,"JSONBlobNotFnd",intmodx,argpnts[ix]) ; return(NULL) ; } ;
	      vjval = (vjval1.jvType == VJSON_Type_None ? &vjblob->topVal : &vjval1) ;		/* If we got a vjval in lookup then use it otherwise take initial value of blob */
	      continue ;
	    } ;
	   switch (vjval->jvType)
	    { default:
	      case VJSON_Type_Int:
	      case VJSON_Type_Double:
	      case VJSON_Type_String:
	      case VJSON_Type_Logical:
	      case VJSON_Type_Null:
		if (argpnts[ix]->PntType == V4DPI_PntType_Dict && argpnts[ix]->Value.IntVal == dictTypeOf)
		 { return(vjson_TypeOf(ctx,respnt,vjval)) ;
		 } else if (argpnts[ix]->PntType == V4DPI_PntType_Dict && argpnts[ix]->Value.IntVal == dictNameOf)
		 { if (lastName == UNUSED) { v_Msg(ctx,NULL,"JSONNoName",intmodx) ; return(NULL) ; } ;
		   dictPNTv(respnt,Dim_NId,lastName) ; return(respnt) ;
		 } ;
/*		If we dereferenced and have not run out of arguments then we got an error. UNLESS next argument is UV4:EQ in which case we are updating */
		if (ix <= argcnt && memcmp(argpnts[ix],&protoEQ,protoEQ.Bytes) == 0)
		 { doUpdate = TRUE ; break ; } ;
		v_Msg(ctx,NULL,"JSONDeref",intmodx,vjson_TypeOf(ctx,respnt,vjval),argpnts[ix]) ; RETUNDEFINED ;
	      case VJSON_Type_Array:
		{ struct VJSON__Value_Array *vja ; INDEX i ;
		  vja = (struct VJSON__Value_Array *)&vjblob->blob[vjval->wIndex] ;
		  if (argpnts[ix]->PntType == V4DPI_PntType_Dict)
		   { DICTID dictId ;
		     dictId = argpnts[ix]->Value.IntVal ;
/*		     Do we have any special element references ? */
		     if (dictId == dictLength)
		      { intPNTv(respnt,vja->count) ; return(respnt) ;
		      } else if (dictId == dictSerialize)
		      { struct V4LEX__BigText *bt ; LENMAX len ;
			ZUS(UCTBUF1) ; len =  vjson_SerializeValue(ctx,vjblob,vjval,UCTBUF1,V4TMBufMax) ;
			if (len >= V4DPI_UCVal_Max - 5)
			 { if (len >= V4LEX_BigText_Max) { v_Msg(ctx,NULL,"@%1E - Result string (len=%2d) exceeds max length allowed(%3d)",intmodx,len,V4LEX_BigText_Max) ; return(NULL) ; } ;
			   bt = (struct V4LEX__BigText *)v4mm_AllocChunk(sizeof *bt,FALSE) ; UCstrcpy(bt->BigBuf,UCTBUF1) ;
			   if (!v4dpi_SaveBigTextPoint(ctx,bt,UCstrlen(bt->BigBuf),respnt,Dim_Alpha,TRUE))
			    { v_Msg(ctx,NULL,"StrSaveBigText",intmodx,V4DPI_PntType_BigText) ; return(NULL) ; } ;
			   v4mm_FreeChunk(bt) ;		
			 } else { uccharPNTv(respnt,UCTBUF1) ; } ;
			return(respnt) ;
		      } else if (dictId == dictTypeOf)
		      { return(vjson_TypeOf(ctx,respnt,vjval)) ;
		      } else if (dictId == dictNameOf)
		      { if (lastName == UNUSED) { v_Msg(ctx,NULL,"JSONNoName",intmodx) ; return(NULL) ; } ;
			dictPNTv(respnt,Dim_NId,lastName) ; return(respnt) ;
		      } ;
		   } ;
		  i = v4im_GetPointInt(&ok,argpnts[ix],ctx) ;
		  if (!ok || i < 1 || i > vja->count) { v_Msg(ctx,NULL,"JSONBadIndex",intmodx,argpnts[ix],vja->count) ; RETUNDEFINED ; } ;
		  vjval = &vja->arrayVal[i-1] ;
		  lastName = UNUSED ;
		  break ;
		}
	      case VJSON_Type_Object:
		{ struct VJSON__Value_Object *vjo ; INDEX i ; DICTID dictId ;
		  vjo = (struct VJSON__Value_Object *)&vjblob->blob[vjval->wIndex] ;
/*		  Element within object can be referenced by integer index or name */
		  switch (argpnts[ix]->PntType)
		   { default:
			v_Msg(ctx,NULL,"JSONRefArg",intmodx,argpnts[ix],argpnts[ix]->PntType) ; return(NULL) ;
		     case V4DPI_PntType_Int:
			i = v4im_GetPointInt(&ok,argpnts[ix],ctx) ;
			if (!ok || i < 1 || i > vjo->count) { v_Msg(ctx,NULL,"JSONBadIndex",intmodx,argpnts[ix],vjo->count) ; RETUNDEFINED ; } ;
			lastName = vjo->objEntry[i-1].dictId ;
			if (dimAsName && ix == argcnt)
			 { DIMID dim = v4dpi_DimGet(ctx,v4dpi_RevDictEntryGet(ctx,vjo->objEntry[i-1].dictId),0) ;
			   if (dim > 0) { DIMINFO(diRes,ctx,dim) ; } ;
			 } ;
			vjval = &vjo->objEntry[i-1].jsonVal ; break ;
		     CASEofCharmU
		     case V4DPI_PntType_UCChar:
		     case V4DPI_PntType_Dict:
		     case V4DPI_PntType_XDict:
			if (argpnts[ix]->PntType == V4DPI_PntType_UCChar || argpnts[ix]->PntType == V4DPI_PntType_Char || argpnts[ix]->PntType == V4DPI_PntType_XDict)
			 { v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,argpnts[ix],ctx) ;
			   dictId = v4dpi_DictEntryGet(ctx,Dim_NId,UCTBUF1,di,NULL) ;
			 } else { dictId = argpnts[ix]->Value.IntVal ; } ;
			for (i=0;i<vjo->count;i++)
			 { if (vjo->objEntry[i].dictId == dictId) break ; } ;
			if (i >= vjo->count)
			 { 
/*			   Do we have any special element references ? */
			   if (dictId == dictElementNames)
			    { struct V4L__ListPoint *lp ; struct VJSON__Value_Object *vjo = (struct VJSON__Value_Object *)&vjblob->blob[vjval->wIndex] ;
			      INITLP(respnt,lp,Dim_List) ;
			      for (i=0;i<vjo->count;i++)
			       { struct V4DPI__LittlePoint p ; LOGICAL ok ;
				 dictPNTv((P *)&p,Dim_NId,vjo->objEntry[i].dictId) ;
				 ok = v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_AppendUnique,(P *)&p,0) ; if (!ok) return(NULL) ;
			       } ;
			      ENDLP(respnt,lp) ; return(respnt) ;
			    } else if (dictId == dictLength)
			    { intPNTv(respnt,vjo->count) ; return(respnt) ;
			    } else if (dictId == dictSerialize)
			    { struct V4LEX__BigText *bt ; LENMAX len ;
			      ZUS(UCTBUF1) ; len =  vjson_SerializeValue(ctx,vjblob,vjval,UCTBUF1,V4TMBufMax) ;
			      if (len == UNUSED) { v_Msg(ctx,NULL,"JSONTooBig",intmodx) ; return(NULL) ; } ;
			      if (len >= V4DPI_UCVAL_MaxSafe)
			       { if (len >= V4LEX_BigText_Max) { v_Msg(ctx,NULL,"@%1E - Result string (len=%2d) exceeds max length allowed(%3d)",intmodx,len,V4LEX_BigText_Max) ; return(NULL) ; } ;
				 bt = (struct V4LEX__BigText *)v4mm_AllocChunk(sizeof *bt,FALSE) ; UCstrcpy(bt->BigBuf,UCTBUF1) ;
				 if (!v4dpi_SaveBigTextPoint(ctx,bt,UCstrlen(bt->BigBuf),respnt,Dim_Alpha,TRUE))
				  { v_Msg(ctx,NULL,"StrSaveBigText",intmodx,V4DPI_PntType_BigText) ; return(NULL) ; } ;
				 v4mm_FreeChunk(bt) ;		
			       } else { uccharPNTv(respnt,UCTBUF1) ; } ;
			      return(respnt) ;
			    } else if (dictId == dictTypeOf)
			    { return(vjson_TypeOf(ctx,respnt,vjval)) ;
			    } else if (dictId == dictNameOf)
			    { if (lastName == UNUSED) { v_Msg(ctx,NULL,"JSONNoName",intmodx) ; return(NULL) ; } ;
			      dictPNTv(respnt,Dim_NId,lastName) ; return(respnt) ;
			    } else { v_Msg(ctx,NULL,"JSONNameNF",intmodx,argpnts[ix],vjo->count) ; RETUNDEFINED ; } ;
			 } ;
			if (dimAsName && ix == argcnt)
			 { DIMID dim = v4dpi_DimGet(ctx,v4dpi_RevDictEntryGet(ctx,dictId),0) ;
			   if (dim > 0) { DIMINFO(diRes,ctx,dim) ; } ;
			 } ;
			lastName = vjo->objEntry[i].dictId ;
			vjval = &vjo->objEntry[i].jsonVal ; break ;
		   } ;
		}
	   } ;
	  if (doUpdate) break ;
	 } ;
/*	If doing an update then handle here */
	if (doUpdate)
	 { P *vpt = argpnts[argcnt] ;		/* Last point is the value point */
	   switch(vpt->PntType)
	    { default:	      			v4im_GetPointUC(&ok,UCTBUF1,V4TMBufMax,vpt,ctx) ;
						vjson_ReplaceValue(&vjblob,vjval,VJSON_Type_String,UCTBUF1,sizeof(UCCHAR) * (1 + UCstrlen(UCTBUF1))) ; break ;
	      case V4DPI_PntType_Int:
	      case V4DPI_PntType_Delta:		vjson_ReplaceValue(&vjblob,vjval,VJSON_Type_Int,&vpt->Value.IntVal,sizeof(vpt->Value.IntVal)) ; break ;
	      case V4DPI_PntType_Real:		vjson_ReplaceValue(&vjblob,vjval,VJSON_Type_Double,&vpt->Value.RealVal,sizeof(vpt->Value.RealVal)) ; break ;
	      case V4DPI_PntType_Logical:	ok = v4im_GetPointLog(&ok,vpt,ctx) ; vjson_ReplaceValue(&vjblob,vjval,VJSON_Type_Logical,NULL,ok) ; break ;
	    }
	   return(argpnts[argcnt]) ;
	 } ;

	switch (vjval->jvType)
	 { default:
	   case VJSON_Type_Int:
	      { struct VJSON__Value_Int *vji ; vji = (struct VJSON__Value_Int *)&vjblob->blob[vjval->wIndex] ;
	        if (diRes == NULL) { intPNTv(respnt,vji->intVal) ; return(respnt) ; }
		 else { P *rpt = v4dpi_AcceptValue(ctx,respnt,diRes,VDTYPE_b4SInt,&vji->intVal,sizeof vji->intVal) ;
			if (rpt != NULL) return(rpt) ;
			v_Msg(ctx,NULL,"ModFailed2",intmodx) ; return(NULL) ;
		      } ;
	      }
	   case VJSON_Type_Logical:
	      { if (diRes == NULL) { logPNTv(respnt,(vjval->wIndex == TRUE)) ; return(respnt) ; }
		 else { LOGICAL lval = (vjval->wIndex == TRUE) ;
			P *rpt = v4dpi_AcceptValue(ctx,respnt,diRes,VDTYPE_b4SInt,&lval,sizeof lval) ;
			if (rpt != NULL) return(rpt) ;
			v_Msg(ctx,NULL,"ModFailed2",intmodx) ; return(NULL) ;
		      } ;
	      }
	   case VJSON_Type_Null:
	      { if (diRes == NULL) { dictPNTv(respnt,Dim_UV4,v4im_GetEnumToDictVal(ctx,DE(Null),Dim_NId)) ; return(respnt) ; } ;
	        v_Msg(ctx,NULL,"JSONNull",intmodx,diRes->DimId) ; return(NULL) ;
	      }
	   case VJSON_Type_Double:
	      { struct VJSON__Value_Double *vjd ; vjd = (struct VJSON__Value_Double *)&vjblob->blob[vjval->wIndex] ;
		if (diRes == NULL) { dblPNTv(respnt,vjd->dblVal) ; return(respnt) ; }
		 else { P *rpt = v4dpi_AcceptValue(ctx,respnt,diRes,VDTYPE_b8FP,&vjd->dblVal,sizeof vjd->dblVal) ;
			if (rpt != NULL) return(rpt) ;
			v_Msg(ctx,NULL,"ModFailed2",intmodx) ; return(NULL) ;
		      } ;
	      }
	   case VJSON_Type_String:
	      { struct VJSON__Value_String *vjs ; vjs = (struct VJSON__Value_String *)&vjblob->blob[vjval->wIndex] ;
/*		If we are calling AcceptValue then pass the number of bytes, NOT the number of characters */
	        if (diRes == NULL)
		 { LENMAX len = UCstrlen(vjs->strVal) ; struct V4LEX__BigText *bt ;
		   if (len >= V4DPI_UCVAL_MaxSafe)
		    { if (len >= V4LEX_BigText_Max) { v_Msg(ctx,NULL,"ModStrTooLong",intmodx,len,V4LEX_BigText_Max) ; return(NULL) ; } ;
		      bt = (struct V4LEX__BigText *)v4mm_AllocChunk(sizeof *bt,FALSE) ;
		      UCstrcpy(bt->BigBuf,vjs->strVal) ;
		      if (!v4dpi_SaveBigTextPoint(ctx,bt,len,respnt,Dim_Alpha,TRUE))
		       { v_Msg(ctx,NULL,"StrSaveBigText",intmodx,V4DPI_PntType_BigText) ; return(NULL) ; } ;
		      v4mm_FreeChunk(bt) ;
		      return(respnt) ;	
		    } else		/* Store as Alpha point */	
		    { uccharPNTvl(respnt,vjs->strVal,len) ; return(respnt) ;
		    } ;
		 } else
		 { P *ipt ;
/*		   veh150219 - If special patterns then convert to special V4 points */
		   if (UCstrcmp(vjs->strVal,UClit("..")) == 0)
		    { ipt = respnt ; ZPH(ipt) ; ipt->Dim = diRes->DimId ; ipt->Bytes = V4DPI_PointHdr_Bytes ; ipt->Grouping = V4DPI_Grouping_All ; ipt->PntType = V4DPI_PntType_Special ; }
		    else if (UCstrcmp(vjs->strVal,UClit("*")) == 0)
		    { ipt = respnt ; ZPH(ipt) ; ipt->Dim = diRes->DimId ; ipt->Bytes = V4DPI_PointHdr_Bytes ; ipt->Grouping = V4DPI_Grouping_Current ; ipt->PntType = V4DPI_PntType_Special ; }
		    else { ipt = v4dpi_AcceptValue(ctx,respnt,diRes,VDTYPE_UCChar,&vjs->strVal,(sizeof(UCCHAR) * UCstrlen(vjs->strVal))) ; } ;
/*		   If simple accept failed then maybe we have compound point (a..b or a,b,c etc.) - do it the hard way with an EvalPt() */
		   if (ipt == NULL && UCstrlen(vjs->strVal) < 2000)
		    { struct V4LEX__TknCtrlBlk *tcb ; UCCHAR ptbuf[2048] ;
		      UCstrcpy(ptbuf,diRes->DimName) ; UCstrcat(ptbuf,UClit(":")) ; UCstrcat(ptbuf,vjs->strVal) ;
		      tcb = v4mm_AllocChunk(sizeof *tcb,FALSE) ; v4lex_InitTCB(tcb,V4LEX_TCBINIT_NoStdIn) ; v4lex_NestInput(tcb,NULL,ptbuf,V4LEX_InpMode_String) ;
		      ok = v4dpi_PointParse(ctx,respnt,tcb,V4DPI_PointParse_RetFalse) ;
		      v4lex_FreeTCB(tcb) ;
		      ipt = (ok ? respnt : NULL) ;
		    } ;
		   if (ipt == NULL) { v_Msg(ctx,NULL,"JSONDimVal",intmodx,diRes->DimId) ; return(NULL) ; } ;
		   return(ipt) ;
		 } ;
	      }
	   case VJSON_Type_Array:
	   case VJSON_Type_Object:
	      { struct V4L__ListPoint *lp ;
		struct V4L__ListJSON *ljson ; struct VJSON__Value_Object *vjo ; struct VJSON__Value_Array *vja ;
		INITLP(respnt,lp,Dim_List) ; lp->ListType = V4L_ListType_JSON ;
		ljson = (struct V4L__ListJSON *)&lp->Buffer ; ljson->vjblob = vjblob ; ljson->vjval = *vjval ; ljson->dimAsName = dimAsName ; ljson->objName = lastName ;
		lp->Bytes = (char *)&lp->Buffer - (char *)lp + sizeof *ljson ;
		vja = (struct VJSON__Value_Array *)&vjblob->blob[vjval->wIndex] ; vjo = (struct VJSON__Value_Object *)&vjblob->blob[vjval->wIndex] ;
		lp->Entries = (vjval->jvType == VJSON_Type_Array ? vja->count : vjo->count) ;
		ENDLP(respnt,lp) ; return(respnt) ;
	     }
	 } ;
}

/*	vjson_ReplaceValue - Adds new value to vjblob and returns index to it	*/
/*	Call: = logical = vjson_ReplaceValue(vjblobPtr,vjval,jvType,value,bytes)
	  where logical is TRUE if vjblob was reallocated to new position, FALSE if not,
		vjblobPtr is pointer to pointer to vjblob,
		vjval is pointer to vjval to be updated,
		jvType is jvType of new value,
		value & bytes is pointer to value and length of new value	*/

LOGICAL vjson_ReplaceValue(vjblobPtr,vjval,jvType,value,bytes)
  struct VJSON__Blob **vjblobPtr ;
  struct VJSON__Value *vjval ;
  ETYPE jvType ;
  void *value ;
  LENMAX bytes ;
{ LENMAX curLen,newLen ; struct VJSON__Blob *vjblob ;
	
	vjblob = *vjblobPtr ;
/*	Can we update 'in place' ? */
	switch (vjval->jvType)
	 { case VJSON_Type_None:	curLen = 0 ; break ;
	   case VJSON_Type_Int:		curLen = sizeof(int) ; break ;
	   case VJSON_Type_Double:	curLen = sizeof(double) ; break ;
	   case VJSON_Type_String:	{ struct VJSON__Value_String *vjs ; vjs = (struct VJSON__Value_String *)&vjblob->blob[vjval->wIndex] ;
					  curLen = sizeof(UCCHAR) * UCstrlen(vjs->strVal) ;
					  break ;
					}
	   case VJSON_Type_Array:	curLen = 0 ; break ;
	   case VJSON_Type_Object:	curLen = 0 ; break ;
	   case VJSON_Type_Logical:	curLen = 0 ; break ;
	   case VJSON_Type_Null:	curLen = 0 ; break ;
	 } ;
	switch (jvType)
	 { case VJSON_Type_None:	newLen = 0 ; break ;
	   case VJSON_Type_Int:		newLen = sizeof(int) ; break ;
	   case VJSON_Type_Double:	newLen = sizeof(double) ; break ;
	   case VJSON_Type_String:	newLen = bytes ; break ;
	   case VJSON_Type_Array:	newLen = 0 ; break ;
	   case VJSON_Type_Object:	newLen = 0 ; break ;
	   case VJSON_Type_Logical:	newLen = 0 ; break ;
	   case VJSON_Type_Null:	newLen = 0 ; break ;
	 } ;
/*	If new length greater than old then going to have to allocate for a new value */
	if (newLen > curLen)
	 { INDEX bytesNeeded ;
	   bytesNeeded = (newLen > curLen ? ALIGN(newLen) : 0) ;
	   if (bytesNeeded >= vjblob->bRemain)
	    { LENMAX inc = bytesNeeded + 0x100 ;
	      vjblob->bMax += inc ; vjblob->bRemain += inc ;
	      vjblob = realloc(vjblob,vjblob->bMax) ;
	    } ;
/*	   Update wIndex to index of new value */
	   vjval->wIndex = vjblob->bIndex ; vjblob->bIndex += bytesNeeded ; vjblob->bRemain -= bytesNeeded ;
	 } ;
	vjval->jvType = jvType ;
	switch (jvType)
	 { case VJSON_Type_None:
	   case VJSON_Type_Null:	vjval->wIndex = 0 ; break ;
	   case VJSON_Type_Int:
	   case VJSON_Type_Double:	memcpy(&vjblob->blob[vjval->wIndex],value,bytes) ; break ;
	   case VJSON_Type_String:	memcpy(&vjblob->blob[vjval->wIndex],value,bytes) ; vjblob->blob[vjval->wIndex+bytes] = UCEOS ; break ;
	   case VJSON_Type_Array:
	   case VJSON_Type_Object:	break ;		/* Not yet implemented */
	   case VJSON_Type_Logical:	vjval->wIndex = bytes ;	/* NOTE: logicals passed as: bytes = FALSE or TRUE */
	 } ;
	return(vjblob != *vjblobPtr) ;
}


P *vjson_TypeOf(ctx,respnt,vjval)
  struct V4C__Context *ctx ;
  struct V4DPI__Point *respnt ;
  struct VJSON__Value *vjval ;
{ struct V4DPI__DimInfo *di ; UCCHAR *tstr ;

	switch (vjval->jvType)
	 { default:			tstr = UClit("unknown") ; break ;
	   case VJSON_Type_Int:		tstr = UClit("integer") ; break ;
	   case VJSON_Type_Double:	tstr = UClit("double") ; break ;
	   case VJSON_Type_String:	tstr = UClit("string") ; break ;
	   case VJSON_Type_Logical:	tstr = UClit("logical") ; break ;
	   case VJSON_Type_Null:	tstr = UClit("null") ; break ;
	   case VJSON_Type_Array:	tstr = UClit("array") ; break ;
	   case VJSON_Type_Object:	tstr = UClit("object") ; break ;
	   case VJSON_Type_Undefined:	tstr = UClit("undefined") ; break ;
	 } ;
	DIMINFO(di,ctx,Dim_NId) ; dictPNTv(respnt,Dim_UV4,v4dpi_DictEntryGet(ctx,Dim_NId,tstr,di,NULL)) ;
	return(respnt) ;
}