
/*	V3V4.C - V4 Interface to V3

	Created 4/21/91 by Victor E. Hansen			*/

#include <time.h>
#include "v3defs.c"
#include <setjmp.h>

char *stru_cvtcstr() ;
#define CS stru_cvtcstr(tmpstr,&inpstr)		/* Define this for multiple use below */
struct V4DPI__Point *v4dpi_IsctEval() ;
struct V4DPI__DimInfo *v4dpi_DimInfoGet() ;

UCCHAR *v4dpi_PointToString() ;

/*	Global Definitions for Parser		*/
extern struct db__parse_info *parse ;

/*	Global References for Execution		*/
extern struct db__process *process ;
extern struct db__psi *psi ;
struct V4C__Context *ctx = NULL ;

#define V3_V4_ITX_POINT V3_V4_OPOFFSET+1
#define V3_V4_AREACLOSE V3_V4_OPOFFSET+2
#define V3_V4_AREAOPEN V3_V4_OPOFFSET+3
#define V3_V4_BIND V3_V4_OPOFFSET+4
#define V3_V4_CONTEXTADD V3_V4_OPOFFSET+5
#define V3_V4_CONTEXTPOP V3_V4_OPOFFSET+6
#define V3_V4_CONTEXTPUSH V3_V4_OPOFFSET+7
#define V3_V4_DIMID V3_V4_OPOFFSET+8
#define V3_V4_DIMMAKE V3_V4_OPOFFSET+9
#define V3_V4_EVAL V3_V4_OPOFFSET+10
#define V3_V4_SHOWBUCKET V3_V4_OPOFFSET+11
#define V3_V4_SHOWCONTEXT V3_V4_OPOFFSET+12
#define V3_V4_XTI_POINT V3_V4_OPOFFSET+13
#define V3_V4_OPEN V3_V4_OPOFFSET+14
#define V3_V4_CLOSE V3_V4_OPOFFSET+15
#define V3_V4_GET V3_V4_OPOFFSET+16
#define V3_V4_PUT V3_V4_OPOFFSET+17
#define V3_V4_EVALPOINT V3_V4_OPOFFSET+18
#define V3_V4_POINTMAKE V3_V4_OPOFFSET+19
#define V3_V4_VALUEGET V3_V4_OPOFFSET+20
#define V3_V4_ISCTMAKE V3_V4_OPOFFSET+21
#define V3_V4_DATAELVAL V3_V4_OPOFFSET+22
#define V3_V4_BIGBUF_PUT V3_V4_OPOFFSET+23
#define V3_V4_BIGBUF_GET V3_V4_OPOFFSET+24
#define V3_V4_SCANFORDIM V3_V4_OPOFFSET+25
#define V3_V4_BINDPOINT V3_V4_OPOFFSET+26
#define V3_V4_POINTREMOVE V3_V4_OPOFFSET+27
#define V3_V4_POINTCOPY V3_V4_OPOFFSET+28
#define V3_V4_SETTRACE V3_V4_OPOFFSET+29
#define V3_V4_LISTTOARRAY V3_V4_OPOFFSET+30
#define V3_V4_REMOTEFILEREF V3_V4_OPOFFSET+31
#define V3_V4_DICTIONARYGET V3_V4_OPOFFSET+32
#define V3_V4_HANDLE V3_V4_OPOFFSET+33

/*	Windows Functions				*/

struct db__predef_table V3_V4_MODULES[] =
   {	{"ITX_POINT",30,NULLV,NULLV,UPD2,V3_V4_ITX_POINT,RES_INT,3},
	{"V4_AREACLOSE",30,NULLV,NULLV,NULLV,V3_V4_AREACLOSE,RES_INT,1},
	{"V4_AREAOPEN",30,NULLV,NULLV,NULLV,V3_V4_AREAOPEN,RES_INT,2},
	{"V4_BIGBUF_GET",30,NULLV,NULLV,UPD2,V3_V4_BIGBUF_GET,RES_INT,2},
	{"V4_BIGBUF_PUT",30,NULLV,NULLV,NULLV,V3_V4_BIGBUF_PUT,RES_INT,2},
	{"V4_BIND",30,NULLV,NULLV,NULLV,V3_V4_BIND,RES_INT,2},
	{"V4_BINDPOINT",30,NULLV,NULLV,NULLV,V3_V4_BINDPOINT,RES_NONE,2},
	{"V4_CLOSE",30,NULLV,NULLV,NULLV,V3_V4_CLOSE,RES_UNK,1},
	{"V4_CONTEXTADD",30,NULLV,NULLV,EOA,V3_V4_CONTEXTADD,RES_INT,2},
	{"V4_CONTEXTPOP",30,NULLV,NULLV,NULLV,V3_V4_CONTEXTPOP,RES_INT,1},
	{"V4_CONTEXTPUSH",30,NULLV,NULLV,NULLV,V3_V4_CONTEXTPUSH,RES_INT,1},
	{"V4_DATAELVAL",30,NULLV,NULLV,NULLV,V3_V4_DATAELVAL,RES_UNK,2},
	{"V4_DICTIONARYGET",30,NULLV,NULLV,NULLV,V3_V4_DICTIONARYGET,RES_INT,1},
	{"V4_DIMID",30,NULLV,NULLV,NULLV,V3_V4_DIMID,RES_INT,1},
	{"V4_DIMMAKE",30,NULLV,NULLV,NULLV,V3_V4_DIMMAKE,RES_INT,1},
	{"V4_EVAL",30,NULLV,NULLV,UPD1,V3_V4_EVAL,RES_INT,2},
	{"V4_EVALPOINT",30,NULLV,NULLV,NULLV,V3_V4_EVALPOINT,RES_INT,1},
	{"V4_GET",30,NULLV,NULLV,NULLV,V3_V4_GET,RES_UNK,1},
	{"V4_HANDLE",30,NULLV,NULLV,EOA,V3_V4_HANDLE,RES_UNK,1},
	{"V4_ISCTMAKE",30,NULLV,NULLV,NULLV,V3_V4_ISCTMAKE,RES_INT,2},
	{"V4_LISTTOARRAY",30,NULLV,NULLV,UPD2,V3_V4_LISTTOARRAY,RES_INT,3},
	{"V4_OPEN",30,NULLV,NULLV,NULLV,V3_V4_OPEN,RES_UNK,1},
	{"V4_POINTCOPY",30,NULLV,NULLV,UPD1,V3_V4_POINTCOPY,RES_UNK,2},
	{"V4_POINTMAKE",30,NULLV,NULLV,EOA,V3_V4_POINTMAKE,RES_INT,3},
	{"V4_POINTREMOVE",30,NULLV,NULLV,NULLV,V3_V4_POINTREMOVE,RES_INT,1},
	{"V4_PUT",30,NULLV,NULLV,NULLV,V3_V4_PUT,RES_UNK,1},
	{"V4_REMOTEFILEREF",30,NULLV,NULLV,NULLV,V3_V4_REMOTEFILEREF,RES_INT,4},
	{"V4_SCANFORDIM",30,NULLV,NULLV,NULLV,V3_V4_SCANFORDIM,RES_INT,7},
	{"V4_SETTRACE",30,NULLV,NULLV,NULLV,V3_V4_SETTRACE,RES_INT,1},
	{"V4_SHOWBUCKET",30,NULLV,NULLV,NULLV,V3_V4_SHOWBUCKET,RES_NONE,2},
	{"V4_SHOWCONTEXT",30,NULLV,NULLV,NULLV,V3_V4_SHOWCONTEXT,RES_NONE,2},
	{"V4_VALUEGET",30,NULLV,NULLV,UPD1,V3_V4_VALUEGET,RES_INT,2},
	{"XTI_POINT",30,NULLV,NULLV,UPD2,V3_V4_XTI_POINT,RES_INT,3}
   } ;

int PRS_V3V4_COUNT=33 ;		/* *** NOTE *** Must equal number of modules defined above */

/*	Main Execution Driver	*/

#define PTS_MAX 31
struct V4DPI__Point *pts[PTS_MAX] ;		/* Point to be used below */
int lupt ;					/* Last used point above */
#define ALLOCPTS(VAR) \
 if (pts[(++lupt)%PTS_MAX] == NULL) pts[lupt%PTS_MAX] = (P *)v4mm_AllocChunk(sizeof *pts[0],FALSE) ; VAR = pts[lupt%PTS_MAX] ;

int v3_v4_handler(code)
 int code ;
{
  static struct V4IS__ParControlBlk *pcb,pcbbuf ;
  static int iscttrace ;
  V3STACK *base_ptr,*start_ptr ;
  struct V4LEX__TknCtrlBlk tcb ;
  struct V4DPI__Binding bindpt ;
  struct V4DPI__Point *ipt,*isct,*newpt,tpt,tpt2 ;
  struct V4DPI__Point *isctpt,*valpt ;
  struct V4DPI__Point_IntMix *pim ;
  struct V4DPI__Point_AlphaMix *pam ;
  struct V4DPI__BindPointVal *bpv ;
  struct V4FFI__DataElSpec *des ;
  struct V4C__AreaHInfo *ahi,ahibuf ;
  struct V4LEX__BigText *bt,*btx ;
  struct V4DPI__DimInfo *di ;
  struct V4L__ListPoint *lp ;
  struct str__ref inpstr,is1,is2 ; int is1len,is2len ; int *iptr ;
  union val__format tmp_val ; char *(*v3ptr) ;
  char tmpstr[500],ts1[250],ts2[250] ; int i,num1,num2,nv,dim,nextfree ;

/*	If first call then initialize context */
	if (ctx == NULL)
	 { v4mm_MemMgmtInit(NULL) ; ctx = (struct V4C__Context *)v4mm_AllocChunk(sizeof *ctx,FALSE) ;
	   v4ctx_Initialize(ctx,NULL,NULL) ; process->ctx = ctx ;
	 } ;
	switch(code)
	 { default:	return(FALSE) ;

	   case V3_V4_ITX_POINT:				/* num = itx_point( point , buffer , flags ) */
		itx_point(ctx) ; break ;
	   case V3_V4_AREACLOSE:				/* v4_AreaClose( areaid ) */
		v4ctx_AreaClose(ctx,xctu_popint()) ;
		break ;
	   case V3_V4_AREAOPEN:					/* areaid = v4_AreaOpen( filename/pcbunit , ahibuf/nil ) */
		stru_popstr(&inpstr) ;				/* Get pointer to ahi */
		if (inpstr.ptr != NULL) { ahi = (struct V4C__AreaHInfo *)inpstr.ptr ; }
		 else { ahi = &ahibuf ;				/* Didn't get one - set up defaults */
			ahi->RelHNum = 0 ; ahi->ExtDictUpd = TRUE ; ahi->IntDictUpd = TRUE ; ahi->BindingsUpd = TRUE ;
		      } ;
		stru_popstr(&inpstr) ;				/* Get pointer to I/O unit for area */
		if ( (i=stru_sslen(&inpstr)) == sizeof *pcb) { pcb = (struct V4IS__ParControlBlk *)inpstr.ptr ; }
		 else { pcb = &pcbbuf ;				/* Didn't get pointer - got file name for area */
			if (*inpstr.ptr == '+') { pcb->OpenMode = V4IS_PCB_OM_Update ; inpstr.ptr++ ; }
			 else pcb->OpenMode = V4IS_PCB_OM_Read ; /* If file name begins with "+" open for update else read-only */
			strcpy(pcb->V3name,"V4AREA") ;
			{ int j ;
			  for(j=0;j<i;j++) { pcb->UCFileName[j] = inpstr.ptr[j] ; } ;
			  pcb->UCFileName[j] = UClit('\0') ;
			}
//			strncpy(pcb->FileName,inpstr.ptr,i) ; pcb->FileName[i] = '\0' ;
			pcb->AccessMode = -1 ;
		      } ;
		if (pcb->OpenMode == V4IS_PCB_OM_New)			/* If creating area make sure some parameters are cool */
		 { if (pcb->BktSize < 8192) pcb->BktSize = 8192 ;
		   pcb->DfltDataMode = V4IS_PCB_DataMode_Data ;
		   if (pcb->AccessMode == 0) pcb->AccessMode = -1 ;
		   if (strlen(pcb->V3name) == 0) strcpy(pcb->V3name,"V4AREA") ;
		 } ;
		v4is_Open(pcb,NULL,NULL) ;
		if (v4ctx_AreaAdd(ctx,pcb,ahi,NULL) == UNUSED) pcb->AreaId = UNUSED ;
		PUSHINT(pcb->AreaId) ;
		break ;
	   case V3_V4_BIGBUF_GET:					/* len = v4_BigBuf_Get( intpoint , buffer ) */
		stru_popstr(&inpstr) ;
		bt = (struct V4LEX__BigText *)v4mm_AllocChunk(sizeof *bt,FALSE) ;
		bt->kp.fld.KeyType = V4IS_KeyType_V4 ; bt->kp.fld.KeyMode = V4IS_KeyMode_Int ;
		bt->kp.fld.Bytes = sizeof bt->kp + sizeof bt->Key ;
		bt->kp.fld.AuxVal = V4IS_SubType_BigText ; bt->Key = xctu_popint() ;
		if (v4is_PositionKey(ctx->LastBindValAid,(struct V4IS__Key *)bt,(struct V4IS__Key **)&btx,0,V4IS_PosDCKL) != V4RES_PosKey)
		 v3_error(V3E_NOREC,"V4IM","BIGBUFGET","NOREC","Could not locate/access BIGBUF record",(void *)0) ;
		v4is_GetData(ctx->LastBindValAid,bt,sizeof *bt,0) ;		/* Copy into bt */
		memcpy(inpstr.ptr,bt->BigBuf,bt->Bytes-bt->kp.fld.Bytes) ; v4mm_FreeChunk(bt) ;
		PUSHINT(bt->Bytes-bt->kp.fld.Bytes) ;			/* Return number of bytes read */
		break ;
	   case V3_V4_BIGBUF_PUT:					/* aid = v4_BigBuf_Put( intpoint , buffer ) */
		stru_popstr(&inpstr) ;
		bt = (struct V4LEX__BigText *)v4mm_AllocChunk(sizeof *bt,FALSE) ;
		memcpy(bt->BigBuf,inpstr.ptr,stru_sslen(&inpstr)) ;
/*		Set up key for this entry */
		bt->kp.fld.KeyType = V4IS_KeyType_V4 ; bt->kp.fld.KeyMode = V4IS_KeyMode_Int ;
		bt->kp.fld.Bytes = sizeof bt->kp + sizeof bt->Key ;
		bt->kp.fld.AuxVal = V4IS_SubType_BigText ; bt->Key = xctu_popint() ;
/*		Now write it all out */
		v4is_Insert(ctx->LastBindValAid,(struct V4IS__Key *)bt,bt,bt->Bytes,V4IS_PCB_DataMode_Data,V4IS_DataCmp_None,0,FALSE,FALSE,0,NULL) ;
		PUSHINT(ctx->LastBindValAid) ;
		break ;
	      case 11:	/* APPEND [intersection] value */
		break ;
	   case V3_V4_BIND:						/* areaid = v4_Bind( isctpt/string , valpt/string ) */
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;		/* Look at value point... */
		if (tmp_val.fld.type == VFT_POINTER)			/* Did we get a point or string? */
		 { ipt = (P *)xctu_popptr() ;		/* Got point - suck up pointer */
		 } else { stru_popstr(&inpstr) ;			/* Got string - convert to point */
			  if (stru_sslen(&inpstr) == sizeof *ipt) { ipt = (P *)inpstr.ptr ; }
			   else { ipt = &tpt ;					/* Allocate a new point */
			  	  v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,0,ASCretUC(CS),V4LEX_InpMode_String) ;
			  	  v4dpi_PointParse(ctx,ipt,&tcb,V4DPI_PointParse_LHS) ;
				} ;
			} ;
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;		/* Look at intersection... */
		if (tmp_val.fld.type == VFT_POINTER)			/* Did we get a point or string? */
		 { isct = (P *)xctu_popptr() ;	/* Got point - suck up pointer */
		 } else { stru_popstr(&inpstr) ;			/* Got string - convert to point */
			  if (stru_sslen(&inpstr) == sizeof *ipt) { isct = (P *)inpstr.ptr ; }
			   else { isct = &tpt2 ;				/* Allocate a new point */
			  	  v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,ASCretUC(CS),V4LEX_InpMode_String) ;
			  	  v4dpi_PointParse(ctx,isct,&tcb,0) ;
				} ;
			} ;
		if (!v4dpi_BindListMake(&bindpt,ipt,isct,ctx,&i,NOWGTADJUST,0,DFLTRELH)) i = -1 ;
		PUSHINT(i) ;						/* Return Area ID */
		break ;
	   case V3_V4_BINDPOINT:					/* point = v4_BindPoint( point , dimid ) */
		dim = xctu_popint() ;
		if ( (di = v4dpi_DimInfoGet(ctx,dim)) == NULL)
		 v3_error(V3E_INVDIM,"V4","POINTMAKE","INVDIM","Invalid dimension ID",(void *)dim) ;
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;		/* Look at value point... */
		if (tmp_val.fld.type == VFT_POINTER)			/* Did we get a point or string? */
		 { ipt = (P *)xctu_popptr() ;		/* Got point - suck up pointer */
		 } else { stru_popstr(&inpstr) ;			/* Got string - convert to point */
			  if (stru_sslen(&inpstr) == sizeof *ipt) { ipt = (P *)inpstr.ptr ; }
			   else v3_error(V3E_NOTPOINT,"V4","BINDPOINT","NOTPOINT","First argument must be a point",(void *)0) ;
			} ;
		ipt->PntType = di->PointType ; ipt->Dim = di->DimId ;
		bpv = (struct V4DPI__BindPointVal *)&ipt->Value ; *bpv = ctx->bpv ;/* Copy from context - hopefully got something */
		ipt->Bytes = V4DPI_PointHdr_Bytes + sizeof *bpv ;
		break ;
	   case V3_V4_CLOSE:						/* v4_Close( pcb ) */
		stru_popstr(&inpstr) ; v4is_Close((struct V4IS__ParControlBlk *)inpstr.ptr) ; break ;
	   case V3_V4_CONTEXTADD:					/* v4_ContextAdd( [framename/framenum] , valpt/string ) */
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_POINTER)			/* Did we get a point or string? */
		 { ipt = (P *)xctu_popptr() ;		/* Got point - suck up pointer */
		 } else { stru_popcstr(tmpstr) ;			/* Got string - convert to point */
			  ipt = &tpt ;					/* Allocate a new point */
			  v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,ASCretUC(tmpstr),V4LEX_InpMode_String) ;
			  v4dpi_PointParse(ctx,ipt,&tcb,0) ;
			} ;
		POPF(tmp_val.all) ;					/* Have context number/name ? */
		if (tmp_val.all != V3_FORMAT_EOA)
		 { PUSHF(tmp_val.all) ;
		   switch (tmp_val.fld.type)
		    { default:	v3_error(V3E_INVNAME,"V4","CONTEXTADD","INVNAME","Context frame id must be integer",0) ;
		      case VFT_BININT:
		      case VFT_BINLONG:
		      case VFT_BINWORD:		i = xctu_popint() ; break ;
		    } ;
		 } else i = 0 ;						/* No frame- default to current */
		v4ctx_FrameAddDim(ctx,i,ipt,0,0) ;
		break ;
	   case V3_V4_CONTEXTPOP:					/* v4_ContextPop( framename/framenum ) */
/*		Did we get a number or a string ? */
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
		switch (tmp_val.fld.type)
		 { case VFT_BININT:
		   case VFT_BINLONG:
		   case VFT_BINWORD:
			v4ctx_FramePop(ctx,xctu_popint(),NULL) ; break ;
		   case VFT_FIXSTRSK:
		   case VFT_FIXSTR:
		   case VFT_VARSTR:
			stru_popcstr(tmpstr) ; v4ctx_FramePop(ctx,-1,ASCretUC(tmpstr)) ; break ;
		 } ;
		break ;
	   case V3_V4_CONTEXTPUSH:					/* frameid = v4_ContextPush( framename ) */
		stru_popcstr(tmpstr) ;
		i = v4ctx_FramePush(ctx,tmpstr) ;
		PUSHINT(i) ;
		break ;
	   case V3_V4_DATAELVAL:					/* v3val = v4_DataElVal( recbuf , des/despt ) */
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
		switch (tmp_val.fld.type)
		 { case VFT_POINTER:
			ipt = (P *)xctu_popptr() ;	/* Get pointer to point */
			des = (struct V4FFI__DataElSpec *)&ipt->Value ; break ;
		   case VFT_FIXSTRSK:
		   case VFT_FIXSTR:
			stru_popstr(&inpstr) ;
			if (inpstr.format.fld.length == sizeof *des) { des = (struct V4FFI__DataElSpec *)inpstr.ptr ; }
			 else if (inpstr.format.fld.length == sizeof *ipt)
				{ ipt = (P *)inpstr.ptr ; des = (struct V4FFI__DataElSpec *)&ipt->Value ; }
			 else v3_error(V3E_INVDES,"V3V4","DATAELVAL","INVDES","Invalid des/v4point length",(void *)inpstr.format.fld.length) ;
			break ;
		 } ;
		stru_popstr(&inpstr) ; PUSHMP(inpstr.ptr+des->Offset) ;
		tmp_val.fld.length = des->Bytes ; tmp_val.fld.decimals = des->Decimals ; tmp_val.fld.mode = VFM_PTR ;
		tmp_val.fld.type = des->V3DT ; PUSHF(tmp_val.all) ;
		break ;
	   case V3_V4_DICTIONARYGET:					/* int = v4_DictionaryGet( entrystr ) */
		stru_popcstr(tmpstr) ;
		PUSHINT(v4dpi_DictEntryGet(ctx,0,ASCretUC(tmpstr),NULL,NULL)) ;
		break ;
	   case V3_V4_DIMID:						/* dimid = v4_DimId( point/dimname ) */
/*		Did we get a number or a string ? */
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
		switch (tmp_val.fld.type)
		 { case VFT_BININT:
		   case VFT_BINLONG:
		   case VFT_BINWORD:
			ipt = (P *)xctu_popptr() ; i = ipt->Dim ; break ;
		   case VFT_FIXSTRSK:
		   case VFT_FIXSTR:
		   case VFT_VARSTR:
			stru_popcstr(tmpstr) ; i = v4dpi_DimGet(ctx,ASCretUC(tmpstr),DIMREF_IRT) ;
			break ;
		 } ;
		PUSHINT(i) ;
		break ;
	   case V3_V4_DIMMAKE:						/* dimid = v4_DimMake( diminfo ) */
		stru_popstr(&inpstr) ;					/* Get pointer to DimInfo structure */
		i = v4dpi_DimMake(ctx,(struct V4DPI__DimInfo *)inpstr.ptr) ;
		PUSHINT(i) ;						/* Return Dim ID just created */
		break ;
	   case V3_V4_EVAL:						/* logical = v4_Eval( dst , isctpt/string ) */
		ALLOCPTS(newpt)
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_POINTER)			/* Did we get a point or string? */
		 { isct = (P *)xctu_popptr() ;	/* Got point - suck up pointer */
		 } else
		 { stru_popstr(&inpstr) ;			/* Got string - convert to point */
		   if (stru_sslen(&inpstr) == sizeof *isct) { isct = (P *)inpstr.ptr ; }
		    else { isct = &tpt ;			/* Allocate a new point */
			   v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,ASCretUC(CS),V4LEX_InpMode_String) ; v4dpi_PointParse(ctx,isct,&tcb,0) ;
			 } ;
		 } ;
		ipt = v4dpi_IsctEval(newpt,isct,ctx,0,NULL,NULLV) ;
		if (ipt == NULL) { POPF(i) ; POPVI(i) ; PUSHFALSE ; break ; } ;
		i = v3v4_UpdateArg(ipt,ctx) ;				/* Got point - update argument and ... */
		if (i == 0)						/*  If UpdateArg fails then don't try to update */
		 { POPF(i) ; POPVI(i) ; PUSHFALSE ; break ; } ;
		v3_operations(V3_XCT_UPDATE+1) ;
		POPF(i) ; POPVI(i) ;					/* Pop off results of update */
		PUSHTRUE ;						/* If UpdateArg OK then call V3 UPDATE & return TRUE */
		break ;
	   case V3_V4_EVALPOINT:					/* valpt/0 = v4_EvalPoint( isctpt/string ) */
		ALLOCPTS(newpt)
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_POINTER)			/* Did we get a point or string? */
		 { isct = (P *)xctu_popptr() ;	/* Got point - suck up pointer */
		 } else
		 { stru_popstr(&inpstr) ;			/* Got string - convert to point */
		   if (stru_sslen(&inpstr) == sizeof *isct) { isct = (P *)inpstr.ptr ; }
		    else { isct = &tpt ;					/* Allocate a new point */
			   v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,ASCretUC(CS),V4LEX_InpMode_String) ;
			   v4dpi_PointParse(ctx,isct,&tcb,0) ;
			  } ;
		 } ;
		ipt = v4dpi_IsctEval(newpt,isct,ctx,0,NULL,NULLV) ;
		PUSHMP(ipt) ; PUSHF(V3_FORMAT_POINTER) ;
		break ;
	   case V3_V4_GET:						/* v4_Get( pcb ) */
		stru_popstr(&inpstr) ; v4is_Get((struct V4IS__ParControlBlk *)inpstr.ptr) ; break ;
//	   case V3_V4_HANDLE:
//		start_ptr = psi->stack_ptr ;
//		for (num1=0;;num1++)				/* See how many arguments */
//		 { POPF(tmp_val.all) ; POPVI(i) ; if (tmp_val.all == V3_FORMAT_EOA) break ; } ;
///*		num1 = number of arguments, base_ptr = current stack pointer */
//		base_ptr = psi->stack_ptr ;
///*		Get function mask/code (num2) */
//		psi->stack_ptr = base_ptr - (2*SIZEOFSTACKENTRY) ;
//		num2 = xctu_popint() ; psi->stack_ptr = start_ptr ;
//		if ((num2 & V3_FLAGS_IOF_CREATE) != 0)		/* Create new handle? */
//		 { psi->stack_ptr = base_ptr ; PUSHINT(han_Make()) ;
//		 } else if ((num2 & V3_FLAGS_IOPG_DELETE) != 0)	/* Close handle? */
//		 { num2 = xctu_popint() ;
//		   psi->stack_ptr = base_ptr ; PUSHINT(han_Close(num2)) ;
//		 } else if ((num2 & V3_FLAGS_HAN_INFO) != 0)	/* Set/Get Info */
//		 { if ((num2 & V3_FLAGS_IOF_PUT) != 0)
//		    { num1 = xctu_popint() ; num2 = xctu_popint() ;	/* num1 = info, num2 = handle */
//		      psi->stack_ptr = base_ptr ; PUSHINT(han_SetInfo(num2,num1)) ;
//		    } else
//		    { num2 = xctu_popint() ;
//		      psi->stack_ptr = base_ptr ; PUSHINT(han_GetInfo(num2)) ;
//		    } ;
//		 } else if ((num2 & V3_FLAGS_HAN_PARENT) != 0)
//		 { if ((num2 & V3_FLAGS_IOF_PUT) != 0)
//		    { num1 = xctu_popint() ; num2 = xctu_popint() ;	/* num1 = parent, num2 = handle */
//		      psi->stack_ptr = base_ptr ; PUSHINT(han_SetParent(num2,num1)) ;
//		    } else
//		    { num2 = xctu_popint() ;
//		      psi->stack_ptr = base_ptr ; PUSHINT(han_GetParent(num2)) ;
//		    } ;
//		 } else if ((num2 & V3_FLAGS_HAN_POINTER) != 0)
//		 { if ((num2 & V3_FLAGS_IOF_PUT) != 0)
//		    { iptr = (int *)xctu_popptr() ; num2 = xctu_popint() ;	/* num1 = pointer, num2 = handle */
//		      psi->stack_ptr = base_ptr ; PUSHINT(han_SetPointer(num2,0,iptr)) ;
//		    } else
//		    { num2 = xctu_popint() ;
//		      psi->stack_ptr = base_ptr ; PUSHMP(han_GetPointer(num2,0)) ; PUSHF(V3_FORMAT_POINTER) ;
//		    } ;
//		 } else if ((num2 & V3_FLAGS_IOF_UPDATE) != 0)
//		 { if ((num2 & V3_FLAGS_IOF_PUT) != 0)
//		    { num2 = xctu_popint() ;			/* num2 = handle */
//		      psi->stack_ptr = base_ptr ; PUSHINT(han_SetUpdate(num2)) ;
//		    } else
//		    { num2 = xctu_popint() ;
//		      psi->stack_ptr = base_ptr ; PUSHINT(han_IsUpdated(num2)) ;
//		    } ;
//		 } else if ((num2 & V3_FLAGS_HAN_TYPE) != 0)
//		 { if ((num2 & V3_FLAGS_IOF_PUT) != 0)
//		    { num1 = xctu_popint() ; num2 = xctu_popint() ;	/* num1 = type, num2 = handle */
//		      psi->stack_ptr = base_ptr ; PUSHINT(han_SetType(num2,num1)) ;
//		    } else
//		    { num2 = xctu_popint() ;
//		      psi->stack_ptr = base_ptr ; PUSHINT(han_GetType(num2)) ;
//		    } ;
//		 } else if ((num2 & V3_FLAGS_HAN_LINK) != 0)
//		 { if ((num2 & V3_FLAGS_IOF_DELETE) != 0)
//		    { num2 = xctu_popint() ;				/* num2 = handle */
//		      psi->stack_ptr = base_ptr ; PUSHINT(han_RemoveHandleFromList(num2)) ;
//		    } else if ((num2 & V3_FLAGS_HAN_BEFORE) != 0)
//		    { num2 = xctu_popint() ; num1 = xctu_popint() ;	/* num1 = listhandle, num2 = newhandle */
//		      psi->stack_ptr = base_ptr ; PUSHINT(han_AddHandleToList(num1,num2,-1)) ;
//		    } else if ((num2 & V3_FLAGS_HAN_AFTER) != 0)
//		    { num2 = xctu_popint() ; num1 = xctu_popint() ;	/* num1 = listhandle, num2 = newhandle */
//		      psi->stack_ptr = base_ptr ; PUSHINT(han_AddHandleToList(num1,num2,1)) ;
//		    } else if ((num2 & V3_FLAGS_HAN_ENDOFLIST) != 0)
//		    { num2 = xctu_popint() ; num1 = xctu_popint() ;	/* num1 = parenthandle, num2 = newhandle */
//		      psi->stack_ptr = base_ptr ; PUSHINT(han_AddHandleToList(num1,num2,0)) ;
//		    } else
//		    { psi->stack_ptr = base_ptr ; PUSHFALSE ;
//		    } ;
//		 } else if ((num2 & V3_FLAGS_HAN_FREEMEMORY) != 0)
//		 { num2 = xctu_popint() ;
//		   psi->stack_ptr = base_ptr ; PUSHINT(han_FreeOnClose(num2)) ;
//		 } else v3_error(0,"V4","HANDLE","HANDLEFUNC","Invalid v4_Handle function code",(void *)num2) ;
//		break ;
	   case V3_V4_ISCTMAKE:						/* point = v4_IsctMake( nil/0/point , point/string ) */
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;		/* Adding point - given as point or string ? */
		if (tmp_val.fld.type == VFT_POINTER)			/* Did we get a point or string? */
		 { ipt = (P *)xctu_popptr() ;		/* Got point - suck up pointer */
		 } else
		 { stru_popstr(&inpstr) ;			/* Got string - convert to point */
		   if (stru_sslen(&inpstr) == sizeof *ipt) { ipt = (P *)inpstr.ptr ; }
		    else { CS ;
			   if (strcmp(tmpstr,"fence") == 0 || strcmp(tmpstr,"FENCE") == 0 || strcmp(tmpstr,"Fence") == 0)
			    {
			    } else
			    { ipt = &tpt ;			/* Got real point - convert it */
			      v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,ASCretUC(tmpstr),V4LEX_InpMode_String) ;
			      v4dpi_PointParse(ctx,ipt,&tcb,0) ;
			    } ;
			 } ;
		 } ;
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;		/* Adding point - given as point or string ? */
		if (tmp_val.fld.type == VFT_POINTER) { isct = (P *)xctu_popptr() ; }
		 else { stru_popstr(&inpstr) ; isct = (P *)inpstr.ptr ; } ;
		if (isct == NULL)
		 { ALLOCPTS(isct) ; INITISCT(isct) ; NOISCTVCD(isct) ;
//		   isct->Bytes = V4DPI_PointHdr_Bytes ; isct->PntType = V4DPI_PntType_Isct ; isct->Grouping = 0 ;
//		   isct->LHSCnt = 0 ; isct->NestedIsct = FALSE ;
		 } else { if (isct->PntType != V4DPI_PntType_Isct)
			   v3_error(V3E_NOTISCT,"V4","ISCTMAKE","NOTISCT","First argument is not of type ISCT",(void *)isct->PntType) ;
			} ;
//		memcpy(&isct->Value.AlphaVal[isct->Bytes == V4DPI_PointHdr_Bytes ? 0 : isct->Bytes-V4DPI_PointHdr_Bytes],ipt,ipt->Bytes) ;
		memcpy(&isct->Value.Isct.pntBuf[isct->Bytes == V4DPI_PointHdr_Bytes ? 0 : isct->Bytes-V4DPI_PointHdr_Bytes],ipt,ipt->Bytes) ;
//		if (v4dpi_IsctParseSetNested(ipt)) isct->NestedIsct = TRUE ;
		ISCTSETNESTED(isct,ipt) ;
		isct->Grouping ++ ; isct->Bytes += ALIGN(ipt->Bytes) ;
		if (ipt->PntType == V4DPI_PntType_Isct) isct->NestedIsct = TRUE ;
		PUSHMP(isct) ; PUSHF(V3_FORMAT_POINTER) ;
		break ;
	   case V3_V4_LISTTOARRAY:					/* count = v4_ListToArray( array , max , listpt ) */
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_POINTER) { ipt = (P *)xctu_popptr() ; }
		 else { stru_popstr(&inpstr) ; ipt = (P *)inpstr.ptr ; } ;
		lp = ALIGNLP(&ipt->Value) ;
		num1 = xctu_popint() ;					/* Get max in array */
		stru_popstr(&inpstr) ; iptr = (int *)inpstr.ptr ;	/* Get pointer to list */
		for(num2=1;v4l_ListPoint_Value(ctx,lp,num2,&tpt) > 0;num2++,iptr++)	/* Loop thru each point in list */
		 { if (num2 > num1) continue ;				/* Don't overflow array */
		   *iptr = tpt.Value.IntVal ;				/* Append integer value to list */
		 } ;
		PUSHINT(num2-1) ;					/* Return number of points in list */
		break ;
	   case V3_V4_OPEN:						/* v4_Open( pcb ) */
		stru_popstr(&inpstr) ;
		if (stru_sslen(&inpstr) != sizeof *pcb)
		 v3_error(V3E_INVPCB,"V4","OPEN","INVPCB","Invalid size for PCB argument",(void *)sizeof *pcb) ;
		v4is_Open((struct V4IS__ParControlBlk *)inpstr.ptr,NULL,NULL) ; break ;
	   case V3_V4_POINTCOPY:					/* dstpt = v4_PointCopy( dstpt , srcpt ) */
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_POINTER) { ipt = (P *)xctu_popptr() ; }
		 else { stru_popstr(&inpstr) ; ipt = (P *)inpstr.ptr ; } ;
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;
		if (tmp_val.fld.type == VFT_POINTER)
		 { POPVP(v3ptr,(char *(*))) ; newpt = (P *)*v3ptr ;	/* Allocate space if pointer is NULL */
		   if (newpt == NULL) { newpt = (P *)v4mm_AllocChunk(ipt->Bytes,FALSE) ; *v3ptr = (char *)newpt ; } ;
		 } else { stru_popstr(&inpstr) ; newpt = (P *)inpstr.ptr ; } ;
		memcpy(newpt,ipt,ipt->Bytes) ;
		PUSHINT(ipt->Bytes) ;					/* Return number of bytes */
		break ;
	   case V3_V4_POINTMAKE:					/* v4_PointMake( nil/0/point , dim , value [,value] ) */
		if ( *(psi->stack_ptr+(3*SIZEOFSTACKENTRY)) == V3_FORMAT_EOA ) { nv = 1 ; } /* Only one value */
		 else { nv = 2 ;					/* Better have two values */
			if (*(psi->stack_ptr+(4*SIZEOFSTACKENTRY)) != V3_FORMAT_EOA)
			 v3_error(V3E_ARGCNT,"V4","POINTMAKE","ARGCNT","v4_PointMake requires 3 or 4 arguments",0) ;
		      } ;
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ; is1.ptr = NULLV ;
		switch (tmp_val.fld.type)
		 { default:	v3_error(V3E_INVPOINTVAL,"V4","CONTEXTADD","INVPOINTVAL","Point value must be string or integer",0) ;
		   case VFT_BININT:
		   case VFT_BINLONG:
		   case VFT_BINWORD:
			num2 = xctu_popint() ; if (nv == 1) { num1 = num2 ; } else { num1 = xctu_popint() ; } ;
			break ;
		   case VFT_FIXSTRSK:
		   case VFT_FIXSTR:
		   case VFT_VARSTR:
			stru_popstr(&is2) ; is2len = stru_sslen(&is2) ;
			if (nv == 1) { is1 = is2 ; is1len = is2len ; is2.ptr = NULL ; }
			 else { stru_popstr(&is1) ; is1len = stru_sslen(&is1) ; } ;
			break ;
		 } ;
		dim = xctu_popint() ;					/* Pop off the dimension */
		ipt = (P *)xctu_popptr() ;		/* Pop off pointer to point */
		POPF(i) ; POPVI(i) ;					/* and EOA */
		if (ipt == NULL)					/* Are we creating new point ? */
		 { ALLOCPTS(ipt)
		   ipt->Dim = dim ;					/* Set up dimension */
		   ipt->Bytes = 0 ; ipt->Grouping = 0 ; ipt->LHSCnt = 0 ;
		   if ( (di = v4dpi_DimInfoGet(ctx,ipt->Dim)) == NULL)
		    v3_error(V3E_INVDIM,"V4","POINTMAKE","INVDIM","Invalid dimension ID",(void *)dim) ;
		   ipt->PntType = di->PointType ;
		 } else
		 { if (dim != 0 && dim != ipt->Dim)
		    v3_error(V3E_INCNSDIM,"V4","POINTMAKE","INCNSDIM","Dimension inconsistent with dimension in point",(void *)ipt->Dim) ;
		 } ;
		switch(ipt->PntType)
		 { case V4DPI_PntType_Special:
			v3_error(V3E_INVV4TYPE,"V4","MAKEPOINT","INVV4TYPE","Cannot use v4_MakePoint to make special {xxx} points",(void *)0) ;
		   case V4DPI_PntType_Color:
			if (is1.ptr != NULLV)				/* Given string - have to convert */
			 { di = v4dpi_DimInfoGet(ctx,ipt->Dim) ;
			   strncpy(tmpstr,is1.ptr,is1len) ; tmpstr[is1len] = 0 ;	/* Make ASCIZ string */
			   num1 = v_ColorNameToRef(tmpstr) ;
			   if (is2.ptr != NULLV)
			    { strncpy(tmpstr,is2.ptr,is2len) ; tmpstr[is2len] = 0 ;
			      num2 = v_ColorNameToRef(tmpstr) ;
			    } else num2 = num1 ;
			 } ;
			if (num1 == 0 || num2 == 0)
			 v3_error(V3E_INVDICT,"V4","MAKEPOINT","INVDICT","Invalid dictionary entry",0) ;
			break ;
		   case V4DPI_PntType_Country:
			if (is1.ptr != NULLV)				/* Given string - have to convert */
			 { di = v4dpi_DimInfoGet(ctx,ipt->Dim) ;
			   strncpy(tmpstr,is1.ptr,is1len) ; tmpstr[is1len] = 0 ;	/* Make ASCIZ string */
			   num1 = v_CountryNameToRef(tmpstr) ;
			   if (is2.ptr != NULLV)
			    { strncpy(tmpstr,is2.ptr,is2len) ; tmpstr[is2len] = 0 ;
			      num2 = v_CountryNameToRef(tmpstr) ;
			    } else num2 = num1 ;
			 } ;
			if (num1 == 0 || num2 == 0)
			 v3_error(V3E_INVDICT,"V4","MAKEPOINT","INVDICT","Invalid dictionary entry",0) ;
			break ;
		   case V4DPI_PntType_XDict:
			if (is1.ptr != NULLV)				/* Given string - have to convert */
			 { di = v4dpi_DimInfoGet(ctx,ipt->Dim) ;
			   strncpy(tmpstr,is1.ptr,is1len) ; tmpstr[is1len] = 0 ;	/* Make ASCIZ string */
			   num1 = v4dpi_XDictEntryGet(ctx,ASCretUC(tmpstr),di,0) ;
			   if (is2.ptr != NULLV)
			    { strncpy(tmpstr,is2.ptr,is2len) ; tmpstr[is2len] = 0 ;
			      num2 = v4dpi_XDictEntryGet(ctx,ASCretUC(tmpstr),di,0) ;
			    } else num2 = num1 ;
			 } ;
			if (num1 == 0 || num2 == 0)
			 v3_error(V3E_INVDICT,"V4","MAKEPOINT","INVDICT","Invalid dictionary entry",0) ;
			break ;
		   case V4DPI_PntType_Dict:
			if (is1.ptr != NULLV)				/* Given string - have to convert */
			 { di = v4dpi_DimInfoGet(ctx,ipt->Dim) ;
			   strncpy(tmpstr,is1.ptr,is1len) ; tmpstr[is1len] = 0 ;	/* Make ASCIZ string */
			   num1 = v4dpi_DictEntryGet(ctx,0,ASCretUC(tmpstr),di,NULL) ;
			   if (is2.ptr != NULLV)
			    { strncpy(tmpstr,is2.ptr,is2len) ; tmpstr[is2len] = 0 ;
			      num2 = v4dpi_DictEntryGet(ctx,0,ASCretUC(tmpstr),di,NULL) ;
			    } else num2 = num1 ;
			 } ;
			if (num1 == 0 || num2 == 0)
			 v3_error(V3E_INVDICT,"V4","MAKEPOINT","INVDICT","Invalid dictionary entry",0) ;
			break ;
		   case V4DPI_PntType_Tree:
		   CASEofINT
			pim = (struct V4DPI__Point_IntMix *)&ipt->Value ;
			pim->Entry[ipt->Grouping].BeginInt = num1 ;
			if (ipt->Bytes == 0 && num1 == num2)
			 { ipt->Grouping = V4DPI_Grouping_Single ;
			   ipt->Bytes = V4PS_Int ;
			 } else
			 { pim->Entry[ipt->Grouping++].EndInt = num2 ;
			   ipt->Bytes = V4DPI_PointHdr_Bytes + ipt->Grouping * sizeof pim->Entry[0] ;
			 } ;
			break ;
		   CASEofChar
			if (is1len > 255)
			 { ts1[0] = 0 ; strncpy(&ts1[1],is1.ptr,is1len) ; ts1[1+is1len] = 0 ; is1len += 2 ; is1.ptr = ts1 ;
			 } else
			 { ts1[0] = is1len++ ; strncpy(&ts1[1],is1.ptr,ts1[0]) ;
			   is1.ptr = ts1 ;						/* Convert to V4 string */
			   if (is2.ptr != NULL)
			    { ts2[0] = is2len++ ; strncpy(&ts2[1],is2.ptr,ts2[0]) ; is2.ptr = ts2 ; } ;
			 } ;
		   case V4DPI_PntType_Fixed:
		   case V4DPI_PntType_UOM:
		   case V4DPI_PntType_UOMPer:
		   case V4DPI_PntType_UOMPUOM:
		   case V4DPI_PntType_Real:
		   case V4DPI_PntType_Calendar:
		   case V4DPI_PntType_TeleNum:
		   case V4DPI_PntType_GeoCoord:
		   case V4DPI_PntType_Int2:
		   case V4DPI_PntType_XDB:
		   case V4DPI_PntType_V4IS:
		   case V4DPI_PntType_Complex:
		   case V4DPI_PntType_Isct:
		   case V4DPI_PntType_Shell:
		   case V4DPI_PntType_FrgnDataEl:
		   case V4DPI_PntType_FrgnStructEl:
//		   case V4DPI_PntType_Foreign:
			pam = (struct V4DPI__Point_AlphaMix *)&ipt->Value ;
			if (ipt->Bytes == 0) { nextfree = 0 ; }
			 else { nextfree = ipt->Bytes - V4DPI_PointHdr_Bytes - (ipt->Grouping * sizeof pam->Entry[0]) ;
				memcpy(tmpstr,&ipt->Value.AlphaVal[V4DPI_PointHdr_Bytes + (ipt->Grouping * sizeof pam->Entry[0])],
						nextfree) ;
			      } ;
			if (ipt->Bytes == 0 && nv == 1)				/* First point a single value ? */
			 { memcpy(&ipt->Value,is1.ptr,is1len) ;		/* Yes - then don't bother with PAM */
			   ipt->Bytes = ALIGN(V4DPI_PointHdr_Bytes + is1len) ;
			   ipt->Grouping = V4DPI_Grouping_Single ;
			 } else
			 { memcpy(&tmpstr[nextfree],is1.ptr,is1len) ;
			   pam->Entry[ipt->Grouping].BeginIndex = nextfree ; nextfree += ALIGN(is1len) ;
			   if (nv == 1) { pam->Entry[ipt->Grouping].EndIndex = 0 ; }
			    else { memcpy(&tmpstr[nextfree],is2.ptr,is2len) ;
				   pam->Entry[ipt->Grouping].EndIndex = nextfree ; nextfree += ALIGN(is2len) ;
				 } ;
			   ipt->Grouping ++ ;
			   memcpy(&pam->Entry[ipt->Grouping],&tmpstr,nextfree) ;	/* Put back together again */
			   ipt->Bytes = (char *)&pam->Entry[ipt->Grouping] + nextfree - (char *)ipt ;
			 } ;
			break ;
		 } ;
		PUSHMP(ipt) ; PUSHF(V3_FORMAT_POINTER) ;		/* Push pointer to the point */
		break ;
	   case V3_V4_POINTREMOVE:					/* logical = v4_PointRemove( bindpt ) */
		POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;		/* Look at value point... */
		if (tmp_val.fld.type == VFT_POINTER)			/* Did we get a point or string? */
		 { ipt = (P *)xctu_popptr() ;		/* Got point - suck up pointer */
		 } else { stru_popstr(&inpstr) ;			/* Got string - convert to point */
			  if (stru_sslen(&inpstr) == sizeof *ipt) { ipt = (P *)inpstr.ptr ; }
			   else { ipt = &tpt ;					/* Allocate a new point */
			  	  v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,ASCretUC(CS),V4LEX_InpMode_String) ;
			  	  v4dpi_PointParse(ctx,ipt,&tcb,0) ;
				} ;
			} ;
		PUSHINT(v4dpi_BindListRemove(ctx,(struct V4DPI__BindPointVal *)&ipt->Value)) ;
		break ;
	   case V3_V4_PUT:						/* v4_Put( pcb ) */
		stru_popstr(&inpstr) ; v4is_Put((struct V4IS__ParControlBlk *)inpstr.ptr,NULL) ; break ;
	   case V3_V4_SCANFORDIM:					/* handle = v4_ScanForDim(handle , dim  [,filterisct] ) */
#define AP(VAR) { POPF(tmp_val.all) ; PUSHF(tmp_val.all) ;\
		  if (tmp_val.fld.type == VFT_POINTER) { ipt = (P *)xctu_popptr() ; }\
		   else { stru_popstr(&inpstr) ; VAR = (P *)inpstr.ptr ;\
			  if (stru_sslen(&inpstr) != sizeof *VAR)\
			   v3_error(V3E_INVPT,"V4","SCANFORDIM","INVPT","Argument not point/pointer",0) ;\
			 } ;\
		}
		AP(valpt) AP(isctpt) AP(isct) dim = xctu_popint() ; i = xctu_popint() ;
		PUSHINT(v4eval_ScanAreaForBinds(ctx,dim,i,isctpt,valpt,isct)) ;
		break ;
	   case V3_V4_REMOTEFILEREF:					/* v4_RemoteFileRef( ref , "host" , port , options ) */
#ifdef V4_BUILD_CS_FUNCTIONALITY
		num1 = xctu_popint() ; num2 = xctu_popint() ; stru_popcstr(ts1) ; i = xctu_popint() ;
		PUSHINT(v4is_RemoteFileRef(i,ts1,num2,num1)) ; break ;
#else
		v3_error(V3E_INVPT,"V4","REMOTEFILREF","NOTIMP","RemoteFileRef not available in this build of V3",0) ;
#endif
	   case V3_V4_SETTRACE:						/* oldval = v4_SetTrace( newval ) */
		i = iscttrace ;						/* Save old value */
		iscttrace = xctu_popint() ;				/* Get new value */
		PUSHINT(i) ; break ;
	   case V3_V4_SHOWBUCKET:					/* v4_ShowBucket( areaid , bucket ) */
		v4is_ExamineBkt(xctu_popint(),xctu_popint()) ;
		break ;
	   case V3_V4_SHOWCONTEXT:
		v4ctx_ExamineCtx(ctx,FALSE,VOUT_Trace) ;
		break ;
	   case	V3_V4_VALUEGET:						/* logical = v4_ValueGet( dst , point ) */
		ipt = (P *)xctu_popptr() ;
		if (ipt == NULL) { POPF(i) ; POPVI(i) ; PUSHFALSE ; break ; } ;
		i = v3v4_UpdateArg(ipt,ctx) ;				/* Got point - update argument and ... */
		if (i == 0)						/*  If UpdateArg fails then don't try to update */
		 { POPF(i) ; POPVI(i) ; PUSHFALSE ; break ; } ;
		v3_operations(V3_XCT_UPDATE+1) ;
		POPF(i) ; POPVI(i) ;					/* Pop off results of update */
		PUSHTRUE ;						/* If UpdateArg OK then call V3 UPDATE & return TRUE */
		break ;
	   case V3_V4_XTI_POINT:					/* num = xti_point( string , point , flags ) */
		xti_point(ctx) ;
		break ;
	 } ;
	return(TRUE) ;
 }

int v3v4_UpdateArg(ipt,ctx)
  struct V4DPI__Point *ipt ;
  struct V4C__Context *ctx ;
{ struct V4DPI__Point_IntMix *pim ;
  struct V4DPI__Point_AlphaMix *pam ;
  struct V4DPI__Point *ipnt ;
  struct V4FFI__DataElSpec *des ;
  struct V4FFI__StructElSpec *ses ;
  static union val__v3num v3n ;
  union val__format format ;
  char tb1[V4DPI_AlphaVal_Max+250], tb2[V4DPI_AlphaVal_Max+250], *tbuf, *ts ; int i,fx ;

	switch (ipt->PntType)
	 { default: v4_error(0,0,"V3V4","UpdateArg","UNKPTTYPE","Unknown point type: %s",v4im_PTName(ipt->PntType)) ;
	   case V4DPI_PntType_Special:
		switch (ipt->Grouping)
		 { default: v4_error(0,0,"V3V4","UpdateArg","UNKSPCLPT","Unknown type for special point",ipt->Grouping) ;
		   case V4DPI_Grouping_All:	return(FALSE) ;
		   case V4DPI_Grouping_AllCnf:	return(FALSE) ;
		   case V4DPI_Grouping_Current:	return(FALSE) ;
		   case V4DPI_Grouping_PCurrent:	return(FALSE) ;
		 } ; break ;
	   case V4DPI_PntType_Tree:
	   CASEofINT
		if (ipt->Grouping == V4DPI_Grouping_Single)
		 { PUSHINT(ipt->Value.IntVal) ; return(TRUE) ; } ;
/*		Have a list or range or whatever - Push first value */
		pim = (struct V4DPI__Point_IntMix *)&ipt->Value ; PUSHINT(pim->Entry[0].BeginInt) ;
		return(TRUE) ;
	   case V4DPI_PntType_Color:
		if (ipt->Grouping == V4DPI_Grouping_Single)
		 { strcpy(tb1,UCretASC(v_ColorRefToName(ipt->Value.IntVal))) ; }
		 else { pim = (struct V4DPI__Point_IntMix *)&ipt->Value ;
			strcpy(tb1,UCretASC(v_ColorRefToName(pim->Entry[0].BeginInt))) ;
		      } ;
		 break ;
	   case V4DPI_PntType_Country:
//		if (ipt->Grouping == V4DPI_Grouping_Single)
//		 { UCstrcpyToASC(tb1,v_CountryRefToName(ipt->Value.IntVal)) ; }
//		 else { pim = (struct V4DPI__Point_IntMix *)&ipt->Value ;
//			UCstrcpyToASC(tb1,v_CountryRefToName(pim->Entry[0].BeginInt)) ;
//		      } ;
		 break ;
	   case V4DPI_PntType_XDict:
		if (ipt->Grouping == V4DPI_Grouping_Single)
		 { strcpy(tb1,UCretASC(v4dpi_RevXDictEntryGet(ctx,ipt->Dim,ipt->Value.IntVal))) ; }
		 else { pim = (struct V4DPI__Point_IntMix *)&ipt->Value ;
			strcpy(tb1,UCretASC(v4dpi_RevXDictEntryGet(ctx,ipt->Dim,pim->Entry[0].BeginInt))) ;
		      } ;
		 break ;
	   case V4DPI_PntType_Dict:
		if (ipt->Grouping == V4DPI_Grouping_Single)
		 { strcpy(tb1,v4dpi_RevDictEntryGet(ctx,ipt->Value.IntVal)) ; }
		 else { pim = (struct V4DPI__Point_IntMix *)&ipt->Value ;
			strcpy(tb1,v4dpi_RevDictEntryGet(ctx,pim->Entry[0].BeginInt)) ;
		      } ;
		 break ;
//	   case V4DPI_PntType_V3Mod:
//	   case V4DPI_PntType_V3ModRaw:
	   CASEofChar
		if (ipt->Grouping == V4DPI_Grouping_Single)
		 { if (ipt->Value.AlphaVal[0] < 255)
		    { strncpy(tb1,&ipt->Value.AlphaVal[1],ipt->Value.AlphaVal[0]) ; tb1[ipt->Value.AlphaVal[0]] = 0 ; }
		    else { strcpy(tb1,&ipt->Value.AlphaVal[1]) ; } ;
		 } else { pam = (struct V4DPI__Point_AlphaMix *)&ipt->Value ; tbuf = (char *)&pam->Entry[ipt->Grouping].BeginIndex ;
			  strncpy(tb1,tbuf+pam->Entry[0].BeginIndex+1,*(tbuf+pam->Entry[0].BeginIndex)) ;
		          tb1[*(tbuf+pam->Entry[0].BeginIndex)] = 0 ;
			} ;
		break ;
	   case V4DPI_PntType_Real:
	   case V4DPI_PntType_Calendar:
		v3n.fp.marker = V3NUM_FPMARK ; GETREAL(v3n.fp.dbl,ipt) ;
		PUSHMP(&v3n) ; PUSHF(V3_FORMAT_V3NUM) ;
		return(TRUE) ;
	   case V4DPI_PntType_Isct:
		tb1[0] = 0 ; v4dpi_IsctToString(tb1,ipt,ctx,0,ctx->pi->MaxPointOutput) ;
		break ;
	   case V4DPI_PntType_Complex:
	   case V4DPI_PntType_GeoCoord:
	   case V4DPI_PntType_TeleNum:
	   case V4DPI_PntType_Int2:
	   case V4DPI_PntType_XDB:
	   case V4DPI_PntType_V4IS:
	   case V4DPI_PntType_Shell:
		tb1[0] = 0 ; v4dpi_PointToString(tb1,(struct V4DPI__Point *)&ipt->Value,ctx,0) ;
		break ;
//	   case V4DPI_PntType_Foreign:
//		break ;
	   case V4DPI_PntType_FrgnDataEl:
		des = (struct V4FFI__DataElSpec *)&ipt->Value ;
/*
		sprintf(tb1,"(%d,%d,%s,%s,%d,%d,%d,%d)",
			(int)des->FileRef,(int)des->Element,des->Name,des->DimName,(int)des->Owner,
			(int)des->Offset,(int)des->Bytes,(int)des->Decimals) ;
*/
		ts = (char *)stru_alloc_bytes(sizeof *des) ; memcpy(ts,des,sizeof *des) ;
		PUSHMP(ts) ; format.all = V3_FORMAT_FIXSTR ; format.fld.length = sizeof *des ; PUSHF(format.all) ;
		return(TRUE) ;
	   case V4DPI_PntType_FrgnStructEl:
		ses = (struct V4FFI__StructElSpec *)&ipt->Value ;
/*
		sprintf(tb1,"(%d,%d,%d,%d,%d)",
			(int)ses->FileRef,(int)ses->StructNum,(int)ses->Element,(int)ses->Bytes,(int)ses->Occurs) ;
*/
		ts = (char *)stru_alloc_bytes(sizeof *ses) ; memcpy(ts,ses,sizeof *ses) ;
		PUSHMP(ts) ; format.all = V3_FORMAT_FIXSTR ; format.fld.length = sizeof *ses ; PUSHF(format.all) ;
		return(TRUE) ;
	 } ;
	ts = (char *)stru_alloc_bytes(strlen(tb1)) ; strcpy(ts,tb1) ;		/* Copy string into semi-perm buffer */
	PUSHMP(ts) ;
	format.all = V3_FORMAT_FIXSTR ; format.fld.length = strlen(tb1) ; PUSHF(format.all) ;
	return(TRUE) ;
}

/*	v3_v4_EvalV3Mod - Sets up stack to invoke V3 Module from V4 Intersection	*/

struct V4DPI__Point *v3_v4_EvalV3Mod(isct,ctx,priorisct)
 struct V4DPI__Point *isct ;
 struct V4C__Context *ctx ;
 struct V4DPI__Point *priorisct ;
{ struct V4DPI__Point *ipt,*bpnt,tpnt ;
  struct V4DPI__DimInfo *di ;
  union val__format format ;
  struct str__ref is1 ; int is1len ;
  int ix,dim,offset,bx ;

	PUSHVI(0) ; PUSHF(V3_FORMAT_EOA) ;			/* Start with EOA on stack */
	for((offset=0,ix=0);ix<isct->Grouping;ix++)	/* Push all argument (including module name) onto stack */
	 { ipt = (P *)&isct->Value.AlphaVal[offset] ; offset += ipt->Bytes ;
	   if (ix == 1) { dim = ipt->Value.IntVal ; }		/* (skip second argument which is resulting point) */
	    else { if (ipt->PntType == V4DPI_PntType_Isct)	/* Is one of the arguments an Isct ? */
		    { ipt = v4dpi_IsctEval(&tpnt,ipt,ctx,FALSE,NULL,NULLV) ;
		      if (ipt == NULL) v3_error(V3E_NOEVAL,"V4","EVALV3MOD","NOEVAL","Could not evaluate Isct argument to V3 mod",(void *)(ix+1)) ;
		    } ;
		   v3v4_UpdateArg(ipt,ctx) ;
		 } ;
	 } ;
	v3_operations(V3_XCT_MOD_APPLY) ;			/* Let V3 do all the work */
	psi->interrupt_index = 1 ;				/* Fake in an interrupt */
	xctu_main() ;						/* Have V3 execute the module & hopefully get back here on return */
	ALLOCPTS(ipt)
	ipt->Dim = dim ; ipt->Grouping = V4DPI_Grouping_Single ; ipt->LHSCnt = 0 ; ipt->NestedIsct = 0 ;
	if (ipt->Dim == 0)					/* Does not return anything ? */
	 { ipt->Dim = 1 ; ipt->PntType = V4DPI_PntType_Int ; ipt->Bytes = V4PS_Int ;
	   ipt->Value.IntVal = 0 ;
	   return(ipt) ;
	 } ;
	di = v4dpi_DimInfoGet(ctx,ipt->Dim) ; ipt->PntType = di->PointType ;
/*	Now see what is on stack & convert to a point */
	POPF(format.all) ; PUSHF(format.all) ;
	switch (format.fld.type)
	 { default:	v3_error(V3E_INVV3TOV4DT,"V4","V3MOD","INVV3TOV4DT","Invalid resulting datatype",(void *)format.fld.type) ;
	   case VFT_BININT:
	   case VFT_BINLONG:
	   case VFT_BINWORD:
		ipt->Value.IntVal = xctu_popint() ; ipt->Bytes = V4PS_Int ;
		break ;
	   case VFT_FIXSTRSK:
	   case VFT_FIXSTR:
	   case VFT_VARSTR:
		stru_popstr(&is1) ; is1len = stru_sslen(&is1) ;
		switch (ipt->PntType)
		 { default:
			memcpy(&ipt->Value.AlphaVal,is1.ptr,is1len) ; ipt->Bytes = V4DPI_PointHdr_Bytes + ALIGN(is1len) ;
			break ;
//		   case V4DPI_PntType_V3Mod:
//		   case V4DPI_PntType_V3ModRaw:
		   CASEofChar
			if (is1len > 255)
			 { ipt->Value.AlphaVal[0] = 255 ; strncpy(&ipt->Value.AlphaVal[1],is1.ptr,is1len) ;
			   ipt->Value.AlphaVal[1+is1len] = 0 ;
			 } else
			 { ipt->Value.AlphaVal[0] = is1len ; strncpy(&ipt->Value.AlphaVal[1],is1.ptr,is1len) ;
			   ipt->Bytes = V4DPI_PointHdr_Bytes + ALIGN(is1len+1) ;
			 } ;
			break ;
		 } ;
		break ;
	 } ;
	return(ipt) ;					/* Return pointer to new point */
}

#ifdef ALTVARGS						/* Need funky header for SCO variable-arg handler */
void v4_error(va_alist)
  va_dcl
{ extern struct iou__unit *v3unitp ;
  struct V4LEX__TknCtrlBlk *tcb ;
  char *subsys,*module,*mne,*msg ; int errnum ;
  va_list args ; char *format ;
  char ebuf[250] ; int code ;

/*	Do special formatting on error message */
	va_start(args) ; errnum = va_arg(args,int) ; tcb = va_arg(args,struct V4LEX__TknCtrlBlk *) ;
	subsys = va_arg(args,char *) ; module = va_arg(args,char *) ; mne = va_arg(args,char *) ;
	format = va_arg(args,char *) ; vsprintf(ebuf,format,args) ; va_end(args) ;
#else							/* All other UNIX */
void v4_error(errnum,tcb,subsys,module,mne,msg)
  int errnum ;
  struct V4LEX__TknCtrlBlk *tcb ;
  char *subsys,*module,*mne,*msg ;

{ extern struct iou__unit *v3unitp ;
  va_list ap ;
  char ebuf[250] ;

/*	Do special formatting on error message */
//	va_start(ap,mne) ; msg = va_arg(ap,char *) ; vsprintf(ebuf,msg,ap) ; va_end(ap) ;
	va_start(ap,msg) ; vsprintf(ebuf,msg,ap) ; va_end(ap) ;
#endif
	if (process == NULL) { printf("? Startup error(%d): %s\n",errnum,ebuf) ; exit(EXIT_FAILURE) ; } ;

//VEH051121 - Always invoke error cleanup if v3unitp defined
	if (v3unitp != NULL) v4is_ErrorCleanup((struct V4IS__ParControlBlk *)v3unitp->file_ptr) ;
//	switch(errnum)
//	 { case 0: case 1: case 2: case 3: case 4:
//		if (v3unitp != NULL) v4is_ErrorCleanup(v3unitp->file_ptr) ;	/* Clean up whatever due to nonfatal V4IS error */
//		break ;
//	   default:
//		v4is_ErrorCleanup(NULL) ; break ;
//	 } ;

	sprintf(process->v4_error,"(%s:%s) - %s: %s\n\n",subsys,mne,module,ebuf) ;
/*	Maybe intercept error (once!) */
	if (process->have_V4intercept) { process->have_V4intercept = FALSE ; longjmp(process->error_intercept,1) ; } ;
	if (v3unitp == NULL) errnum = -1 ;		/* Don't call IO error handler if no unit */
	switch(errnum)
	 { default: v3_error(errnum,subsys,module,mne,ebuf,tcb) ; break ;
	   case V4E_LOCKOUTERR: case V4E_PUTTOOMANYLVLS: case V4E_PUTAPND: case V4E_MAXAREALEN: case V4E_INVPUTMODE:
	   case V4E_AREANODELETE: case V4E_SENDERR: case V4E_RECVERR: case V4E_RECNOTLOCKED: case V4E_NORECTODEL: case V4E_AREANOTHASH1:
	   case V4E_AREANOTHASH2: case V4E_PUTIDXMLTKEY: case V4E_UPDATELINK: case V4E_AREANOUPD: case V4E_AREANOPUT:
	   case V4E_AREANOWRT: case V4E_INVIKDE: case V4E_SNBINDEXLINK: case V4E_DIRTREEBAD: case V4E_LINKNOTINDEX: case V4E_FIRSTNOTSAMEASPARENT:
	   case V4E_FIRSTNOTSAMEASPARENT1: case V4E_EXCEEDLVLMAX: case V4E_INVPOSFLAG: case V4E_NOCURPOS: case V4E_BADKEYINLINK:
	   case V4E_NOPOSPAR: case V4E_INVKEYMODE: case V4E_SEQIOERR: case V4E_SEQRECTOOBIG: case V4E_INVAREAID:
	   case V4E_AREANOGET: case V4E_INVGETMODE: case V4E_INVHASHGETMODE: case V4E_INVFORHASH: case V4E_DATAIDBAD: case V4E_INVBKTTYPE:
	   case V4E_INVDBEHLENIS0: case V4E_BADDDATAID: case V4E_INVDBEHLEN: case V4E_AREANOKEY:
	   case V4E_DILMAX: case V4E_RECEXCEEDBKT: case V4E_BADPOSKEYRES: case V4E_BADPRIMEKEYTYPE:
	   case V4E_BKTTOOSMALL: case V4E_NOCURRECREP: case V4E_AREANOFFI: case V4E_CHNGPRIMONREP:
	   case V4E_NOKEYPOSTODEL: case V4E_INVDATALEN: case V4E_INVCOMPMODE: case V4E_NOFINDKEY: case V4E_NOKEYWITHDATAID:
	   case V4E_KEYNUMTOOBIG: case V4E_KEYNOTINBUF: case V4E_FILEREFNOTINAREA: case V4E_INVOPNMODE: case V4E_MUSTBENEW: 
	   case V4E_INVRECBYTES: case V4E_NOFILECREATE: case V4E_INVOCBTYPE: case V4E_NOKEYINFO: case V4E_NOFILEACCESS: case V4E_INVSEQHDR: 
	   case V4E_OPENTOOMANYLVLS: case V4E_STASHFAILKEY: case V4E_MAKEINVBKTTYPE: case V4E_KEYPREFIXSEQERR:
	   case V4E_INVIKDETYPE: case V4E_BKTNOTDATA: case V4E_BADV4AREA: case V4E_RMSOPEN: case V4E_RMSCONNECT:
	   case V4E_RMSCLOSE: case V4E_RMSPUT: case V4E_RMSGET: case V4E_SHMDTERR:
		v3_error(V3E_IOERR,"IO",module,"IOERR",ebuf,v3unitp) ; break ;
	   case V4E_HITEOF: case V4E_POSPASTEOF: case V4E_EOF:
		v3_error(V3E_EOF,"IO","GET","EOF","End of file reached",v3unitp) ; break ;
	   case V4E_RECCURLOCK: case V4E_LOCKED: 
		v3_error(V3E_LOCKED,"IO","GET","LOCKED",ebuf,v3unitp) ; break ;
	   case V4E_RECTOOBIG: 
		v3_error(V3E_RECTOOBIG,"IO","GET","RECTOOBIG",ebuf,v3unitp) ; break ;
	   case V4E_INSRECEXISTS:
		v3_error(V3E_EXISTS,"IO","PUT","EXISTS",ebuf,v3unitp) ; break ;
	 } ;
}

/*	P O I N T   P A R S I N G   M O D U L E S		*/

/*	xti_point - Converts text to point			*/

int xti_point(ctx)
  struct V4C__Context *ctx ;
{ struct str__ref pntstr,inpstr ;	/* String ref to be edited */
  union val__format format ;
  union flag__ref flags ;	/* Editing flags */
  struct num__ref anum ;		/* Describes destination number */
  struct V4LEX__TknCtrlBlk tcb ;
  struct V4DPI__Point *valpt ;
  char buf[300] ; int len ; int *(*mp_ptr) ;

/*	Get the arguments		*/
	flags.all = xctu_popint() ;
	POPF(format.all) ;
	if (format.fld.type == VFT_FIXSTR || format.fld.type == VFT_FIXSTRSK)
	 { PUSHF(format.all) ; stru_popstr(&pntstr) ;
	   if (stru_sslen(&pntstr) != sizeof *valpt) v3_error(V3E_INVPNTLEN,"XTI","POINT","INVPNTLEN","Invalid length for point",0) ;
	 } else
	 { if (format.fld.type != VFT_POINTER) v3_error(V3E_ARGNOTPTR,"XTI","POINT","ARGNOTPTR","Second argument must be pointer/V4Point",0) ;
	   POPVP(mp_ptr,(int *(*))) ;		/* Pop pointer to pointer value (in case we are going to allocate) */
	   PUSHMP(mp_ptr) ; PUSHF(format.all) ;
	   pntstr.ptr = (char *)xctu_popptr() ;
	 } ;
	stru_popstr(&inpstr) ;
/*	Figure out total length */
	len = (flags.fld.byte3 == 0 ? stru_sslen(&inpstr) : flags.fld.byte3) ;
	strncpy(buf,inpstr.ptr,len) ; buf[len] = 0 ;		/* Convert to C string */
	if (pntstr.ptr == NULL)				/* Have to allocate a new point ? */
	 { ALLOCPTS(valpt) *mp_ptr = (int *)valpt ;
	 } else { valpt = (P *)pntstr.ptr ; } ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,ASCretUC(buf),V4LEX_InpMode_String) ;
	v4dpi_PointParse(ctx,valpt,&tcb,0) ;		/* Parse the point */
/*	Update result & return with length */
	return(len) ;
}

/*	itx_point - Converts point to text */

int itx_point(ctx)
  struct V4C__Context *ctx ;
{ union flag__ref flags ;
  struct str__ref pntstr,xstr ; int xlen ;
  struct V4DPI__Point *ipt ;
  union val__format format ;
  char str[250],*lineptr ; int strl,i ;

/*	Pick up arguments */
	flags.all = xctu_popint() ; stru_popstr(&xstr) ;
/*	See if we got a report line */
	if (*xstr.ptr == -1 && flags.fld.byte3 > 0)
	 { lineptr = xstr.ptr ;
	   switch (rptu_colpos(&xstr,&xlen,flags.fld.byte3,V3_COLTYP_INT))
	    { case 0:	break ;	/* Fall thru for normal report */
	      case 1:	flags.all &= ~(V3_FLAGS_EDT_COMMAS | V3_FLAGS_EDT_FLOATING_DOLLAR | V3_FLAGS_EDT_RIGHT_HAND_SIGN
					| V3_FLAGS_EDT_ASTERISK_FILL | V3_FLAGS_EDT_PAREN_IF_NEG | V3_FLAGS_EDT_CENTER) ;
			break ;
	    } ;
	 }
	 else { if ( xstr.format.fld.type == VFT_VARSTR)
		 { flags.all |= V3_FLAGS_EDT_CONCAT ; xlen = (flags.fld.byte3 > 0 ? flags.fld.byte3 : 0) ; }
		 else { xlen = stru_sslen(&xstr) ; if (flags.fld.byte3 > 0 && flags.fld.byte3 < xlen) xlen = flags.fld.byte3 ; } ;
		lineptr = NULLV ;
	      } ;
	POPF(format.all) ; PUSHF(format.all) ;
	if (format.fld.type == VFT_FIXSTR || format.fld.type == VFT_FIXSTRSK)
	 { stru_popstr(&pntstr) ; ipt = (P *)pntstr.ptr ;
	   if (stru_sslen(&pntstr) != sizeof *ipt) v3_error(V3E_INVPNTLEN,"ITX","POINT","INVPNTLEN","Invalid length for point",0) ;
	 } else
	 { if (format.fld.type != VFT_POINTER) v3_error(V3E_ARGNOTPTR,"ITX","POINT","ARGNOTPTR","Second argument must be pointer/V4Point",0) ;
	   ipt = (P *)xctu_popptr() ;
	 } ;
	v4dpi_PointToString(str,ipt,ctx,V4DPI_FormatOpt_ShowDim) ; strl = strlen(str) ;
	if (flags.fld.mask & V3_FLAGS_EDT_CONCAT)
	 { if (xstr.format.fld.type != VFT_VARSTR)
	    v3_error(V3E_NOTVARSTR,"XTI","ALPHA","NOTVARSTR","Can only \'/concat/\' into STRING",(void *)xstr.format.fld.type) ;
	   xlen = strlen(xstr.ptr) ;
	   if (strl + xlen >= xstr.format.fld.length)
	    v3_error(V3E_TOOLONG,"XTI","ALPHA","TOOLONG","String to long to \'/concat/\'",(void *)strl) ;
	   strncat(xstr.ptr,str,strl) ; *(xstr.ptr+xlen+strl) = '\0' ;
	   return(xlen+strl) ;
	 } ;
/*	See if we are to right justify */
	if (strl > xlen) strl = xlen ;
	if (flags.fld.mask & V3_FLAGS_EDT_RIGHT_JUSTIFY)
	 { if (flags.fld.mask & V3_FLAGS_EDT_NOPAD) { xstr.ptr += (xlen-strl) ; }
	    else
	    { for (i=0;i<xlen-strl;i++) { *(xstr.ptr++) = ' ' ; } ;
	    } ;
	   memcpy(xstr.ptr,str,strl) ;
	   return(xlen) ;
	 } ;
/*	How about centering ? */
	if (flags.fld.mask & V3_FLAGS_EDT_CENTER)
	 { if (flags.fld.mask & V3_FLAGS_EDT_NOPAD)
	    { xstr.ptr += (xlen-strl)/2 ; }
	    else
	    { for (i=0;i<(xlen-strl)/2;i++) { *(xstr.ptr++) = ' ' ; } ; } ;
	   memcpy(xstr.ptr,str,strl) ; xstr.ptr += strl ;
/*	   Maybe pad out the rest */
	   if ((flags.fld.mask & V3_FLAGS_EDT_NOPAD) == 0)
	    { for (i=0;i<xlen-((xlen-strl)/2+strl);i++) *(xstr.ptr++) = ' ' ;
	    } ;
	   return(xlen) ;
	 } ;
/*	If here then left justify */
	memcpy(xstr.ptr,str,strl) ;
	if ((flags.fld.mask & V3_FLAGS_EDT_NOPAD) == 0)
	 { xstr.ptr += strl ;
	   for (i=0;i<xlen-strl;i++) *(xstr.ptr++) = ' ' ;
	 } ;
	return(xlen) ;
}

/*	L I N K S   T O   V 3   F O R   V 4 I M			*/

/*	v3v4_GetV4ctx - Returns pointer to global V4 ctx	*/

struct V4C__Context *v3v4_GetV4ctx()
{ extern struct V4C__ProcessInfo *gpi  ;		/* Global process information */

	if (ctx != NULL) return(ctx) ;
	v_GetProcessInfo() ;
	v4mm_MemMgmtInit(NULL) ;
	ctx = (struct V4C__Context *)v4mm_AllocChunk(sizeof *ctx,TRUE) ;
	v4ctx_Initialize(ctx,NULL,NULL) ;
	process->ctx = ctx ; gpi->ctx = ctx ;
	return(ctx) ;
}

/*	v3v4_v4im_Load_V3PicMod - Handles load of V3 module	*/
/*	Call: v3v4_v4im_Load_V3PicMod( modid , aid )
	  where modid is module id number (used to form V4IS key),
	  aid is area id where module can be found		*/

void v3v4_v4im_Load_V3PicMod(modid,aid)
  int modid,aid ;
{
  struct db__module_entry *mep,*mepx ;
  struct lcl__module_entry_key			/* BAD - should be in v3defs or somewhere common !!! */
   { union V4IS__KeyPrefix kp ;
     int Key ;
   } mek ;

	mek.kp.fld.KeyType = V4IS_KeyType_V4 ; mek.kp.fld.KeyMode = V4IS_KeyMode_Int ; mek.kp.fld.Bytes = V4IS_IntKey_Bytes ;
	mek.kp.fld.AuxVal = V4IS_SubType_V3PicMod ; mek.Key = modid ;
/*	Position to the module- BIG ASSUMPTION- ctx->LastBindValAid is aid containing module !! */
	if (v4is_PositionKey(aid,(struct V4IS__Key *)&mek,(struct V4IS__Key **)&mepx,0,V4IS_PosDCKL) != V4RES_PosKey)
	 v3_error(V3E_NOMODINV4,"V4IM","LOADV3PICMOD","NOMODINV4","Could not load module from V4 database",0) ;
	v4is_GetData(aid,&mep,0,V4IS_PCB_AllocBuf) ;		/* Allocate buffer & copy into *mep */
	mep->package_id = V3_PICMOD_PACKAGE_ID ;		/* Flag as special package */
	pcku_load_module(mep) ;					/* Link into V3 Environment */
}

/*	v3v4_v4im_Unload_V3PicMod - Unloads a V3 Module		*/
/*	Call: v3v4_v4im_Unload_V3PicMod( modname )
	  where modname is pointer to module name string	*/

void v3v4_v4im_Unload_V3PicMod(modname)
  char *modname ;
{
	return ;
}

/*	V R P P   E X E C U T I O N   M O D U L E S		*/

/*	v4pp_Execute - Executes rpp expression contained in vrx		*/
/*	Call: v4pp_Execute( ctx , vrx )
	  where ctx is current V4 context,
		vrx is pointer to expression block			*/

void vrpp_Execute(ctx,vrx)
  struct V4C__Context *ctx ;
  struct VRPP__Xct *vrx ;
{
  union val__format vf ;
  extern struct db__psi *psi ;		/* Points to current psi */
  struct V4DPI__Point *ipt,*isct,tpt ;
  int *intp ; short *shortp ;
  int ix,fx ; int i ; char *valp ;

	for(ix=0;ix<vrx->EntryCount;ix++)
	 {
	   switch (vrx->Entry[ix].ValueType)
	    { default:
	      case VRPP_ValType_LiteralInt:
		vf.all = V3_FORMAT_INTEGER ; vf.fld.decimals = vrx->Entry[ix].Aux1 ;
		PUSHVI(vrx->Entry[ix].Val.IntVal) ; PUSHF(vf.all) ;
		break ;
	      case VRPP_ValType_LiteralDbl:
		vf.all = V3_FORMAT_FLOAT ; vf.fld.decimals = vrx->Entry[ix].Aux1 ;
		PUSHMP(&vrx->Entry[ix].Val.FloatVal) ; PUSHF(vf.all) ;
		break ;
	      case VRPP_ValType_LiteralAlpha:
		vf.all = V3_FORMAT_FIXSTR ; vf.fld.length = vrx->Entry[ix].Aux1 ;
		PUSHMP(vrx->Entry[ix].Val.ValPtr) ; PUSHF(vf.all) ;
		break ;
	      case VRPP_ValType_Isct:
		isct = (P *)vrx->Entry[ix].Val.ValPtr ;
		ipt = (P *)v4dpi_IsctEval(&tpt,isct,ctx,FALSE,NULL,NULLV) ;
		i = v3v4_UpdateArg(ipt,ctx) ;				/* Got point - update argument and ... */
		break ;
	      case VRPP_ValType_Field:
		for(fx=0;fx<vrx->FileCount;fx++)
		 { if (vrx->Entry[ix].Val.DES.FileRef == vrx->File[fx].FileRef) break ; } ;
		if (fx >= vrx->FileCount)
		 v3_error(V3E_NOFILEREF,"V3","RPPXCT","NOFILEREF","Could not locate FileRef in file table",(void *)vrx->Entry[ix].Val.DES.FileRef) ;
		valp = vrx->File[fx].BufferPtr + vrx->Entry[ix].Val.DES.Offset ;
		switch (vrx->Entry[ix].Val.DES.V3DT)
		 { default:
		   case VFT_BININT:
			vf.all = V3_FORMAT_INTEGER ; vf.fld.decimals = vrx->Entry[ix].Val.DES.Decimals ;
			intp = (int *)valp ; PUSHVI(*intp) ; PUSHF(vf.all) ;
			break ;
		   case VFT_BINWORD:
			vf.all = V3_FORMAT_INTEGER ; vf.fld.decimals = vrx->Entry[ix].Val.DES.Decimals ;
			shortp = (short *)valp ; PUSHVI(*shortp) ; PUSHF(vf.all) ;
			break ;
		   case VFT_FLOAT:
			vf.all = V3_FORMAT_FLOAT ; vf.fld.decimals = vrx->Entry[ix].Val.DES.Decimals ;
			PUSHMP(valp) ; PUSHF(vf.all) ;
			break ;
		   case VFT_FIXSTRSK:
		   case VFT_FIXSTR:
			vf.all = V3_FORMAT_FIXSTR ; vf.fld.length = vrx->Entry[ix].Val.DES.Bytes ;
			PUSHMP(valp) ; PUSHF(vf.all) ;
			break ;
		 } ;
		break ;
	      case VRPP_ValType_V3Mod:
		PUSHMP(vrx->Entry[ix].Val.ValPtr) ; PUSHF(V3_FORMAT_POINTER) ;
		PUSHVI(0) ; PUSHF(V3_FORMAT_EOA) ;
		break ;
	      case V4PP_ValType_ModCall:
		v3_operations(V3_XCT_MOD_APPLY) ;			/* Let V3 do all the work */
		psi->interrupt_index = 1 ;				/* Fake in an interrupt */
		xctu_main() ;					/* Have V3 execute the module & hopefully get back here on return */
		break ;
	      case VRPP_ValType_V3Op:
		v3_operations(vrx->Entry[ix].Val.IntVal) ;
		break ;
	      case VRPP_ValType_PushEOA:
	   	PUSHVI(0) ; PUSHF(V3_FORMAT_EOA) ; break ;
	    } ;
	 } ;
}


void v4_UCerror(int errnum,struct V4LEX__TknCtrlBlk *tcb,char *subsys,char *module,char *mne,UCCHAR *msg)
 { v4_error(errnum,tcb,subsys,module,mne,UCretASC(msg)) ;
 }


void v4_ExitStats(ctx,tcb)
  struct V4C__Context *ctx ;
  struct V4LEX__TknCtrlBlk *tcb ;
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

	time(&time_of_day) ; UCsprintf(tbuf,UCsizeof(tbuf),UClit("V4 Processing Summary - %.19s\n"),ASCretUC(ctime(&time_of_day))) ; vout_UCText(VOUT_Status,0,tbuf) ;
	wallseconds = v_ConnectTime() ;
#ifdef WINNT
	if (GetProcessTimes(GetCurrentProcess(),&ctx->pi->ftCreate,&ctx->pi->ftExit,&ftKernel2,&ftUser2))	/* Only do if call implemented (NT) */
	 { B64INT t1,t2 ; memcpy(&t1,&ctx->pi->ftUser1,sizeof t1) ; memcpy(&t2,&ftUser2,sizeof t2) ;
	   cpusecondsU = (t2 - t1) / 10000000.0 ;
	   memcpy(&t1,&ctx->pi->ftKernel1,sizeof t1) ; memcpy(&t2,&ftKernel2,sizeof t2) ;
	   cpusecondsK = (t2 - t1) / 10000000.0 ;
	 } ;
#else
	cpusecondsU = v_CPUTime() ; cpusecondsK = 0.0 ;
#endif
#ifdef WINNT
	UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Time: %g elapsed seconds, %g CPU seconds (%g user + %g kernel)\n"),wallseconds,(cpusecondsU+cpusecondsK),cpusecondsU,cpusecondsK) ; vout_UCText(VOUT_Status,0,tbuf) ;
	cpusecondsU += cpusecondsK ;
#else
	UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Time: %g elapsed seconds, %g CPU seconds\n"),wallseconds,cpusecondsU) ; vout_UCText(VOUT_Status,0,tbuf) ;
#endif
	UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Scan: %d lines, %d tokens\n"),tcb->tcb_lines,tcb->tcb_tokens) ; vout_UCText(VOUT_Status,0,tbuf) ;
	if (ctx->pi->BindingCount != 0)
	 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Bind: %d added\n"),(ctx->pi->BindingCount)) ; vout_UCText(VOUT_Status,0,tbuf) ; } ;
	if (ctx->IsctEvalCount != 0)
	 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Isct: %d eval\'s, %d value I/O\'s, %d%% cache hits\n"),ctx->IsctEvalCount,ctx->IsctEvalValueGetCount,
			(ctx->IsctEvalValueGetCount == 0 ? 0 : DtoI(((double)ctx->IsctEvalValueCache*100.00)/(double)ctx->IsctEvalValueGetCount))) ;
		 vout_UCText(VOUT_Status,0,tbuf) ;
	 } ;
	if (ctx->ContextAddCount != 0)
	 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("   Ctx: %d adds, %d I/O, %d Cache\'s, %d Slams\n"),ctx->ContextAddCount,ctx->ContextAddGetCount,
			ctx->ContextAddCache,ctx->ContextAddSlams) ; vout_UCText(VOUT_Status,0,tbuf) ;
	 } ;
	if (ctx->pi->CacheCalls > 0)
	 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Cache:%d calls, %d%% hits\n"),ctx->pi->CacheCalls,DtoI((double)(ctx->pi->CacheHits*100.0)/(double)ctx->pi->CacheCalls)) ;  vout_UCText(VOUT_Status,0,tbuf) ; } ;
	if (ctx->CtxnblCachePuts > 0)
	 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Cnbl: %d insertions, %d hits\n"),ctx->CtxnblCachePuts,ctx->CtxnblCacheHits) ; vout_UCText(VOUT_Status,0,tbuf) ; } ;
	if (ctx->pi->XDictLookups > 0)
	 { UCsprintf(tbuf,UCsizeof(tbuf),UClit(" XDict: %d XDict lookups, %d%% cache hits\n"),ctx->pi->XDictLookups,
			DtoI((double)(ctx->pi->XDictCacheHits*100)/(double)ctx->pi->XDictLookups)) ;
	   vout_UCText(VOUT_Status,0,tbuf) ;
	 } ;
	if (ctx->pi->AggPutCount != 0)
	 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  WAgg: %d logical puts, %d IOs\n"),ctx->pi->AggPutCount,ctx->pi->AggPutWCount) ;
		 vout_UCText(VOUT_Status,0,tbuf) ;
	 } ;
	if (ctx->pi->AggValueCount > 0)
	 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  RAgg: %d calls, %d repeats (%d %%)\n"),ctx->pi->AggValueCount,ctx->pi->AggValueRepeatCount,intPC(ctx->pi->AggValueRepeatCount,ctx->pi->AggValueCount)) ;
		 vout_UCText(VOUT_Status,0,tbuf) ;
	 } ;
	if (ctx->pi->mtLockConflicts > 0)
	 { UCsprintf(tbuf,UCsizeof(tbuf),UClit(" MThrd: %d interlock conflicts\n"),ctx->pi->mtLockConflicts) ;
		 vout_UCText(VOUT_Status,0,tbuf) ;
	 } ;
	if (cpusecondsU == 0) cpusecondsU = 1 ; if (wallseconds == 0) wallseconds = 1 ;
	if (ctx->IsctEvalCount != 0)
	 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Wall: %d evals/sec, %d context-adds/second\n"),DtoI(ctx->IsctEvalCount/wallseconds),DtoI(ctx->ContextAddCount/wallseconds)) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("   CPU: %d evals/cpusec, %d context-adds/cpusec\n"),DtoI(ctx->IsctEvalCount/cpusecondsU),DtoI(ctx->ContextAddCount/cpusecondsU)) ; vout_UCText(VOUT_Status,0,tbuf) ;
	 } ;
	if (ctx->pi->ErrCount > 0) { UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Errs: %d\n"),(ctx->pi->ErrCount)) ; vout_UCText(VOUT_Status,0,tbuf) ; } ;

#ifdef V4_BUILD_RUNTIME_STATS
	vout_UCText(VOUT_Status,0,UClit("Begin of detailed runtime statistics\n")) ;
	vout_UCText(VOUT_Status,0,,UClit("Args  Number of Calls\n")) ;
	for(i=0;i<31;i++)
	 { if (ctx->pi->V4IntModTotalByArg[i] == 0) continue ;
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit(,"%4d  %10d\n"),i,ctx->pi->V4IntModTotalByArg[i]) ; vout_UCText(VOUT_Status,0,tbuf) ;
	 } ;
	if (ctx->pi->V4IntModTotalByArg[31] != 0)
	 { UCsprintf(tbuf,UCsizeof(tbuf),UClit(," >30  %10d\n"),i,ctx->pi->V4IntModTotalByArg[31]) ; vout_UCText(VOUT_Status,0,tbuf) ; } ;
	vout_UCText(VOUT_Status,0,UClit("    Calls  Module\n")) ;
	for(;;)
	 { for(max = 0,i=1;i<=V4BUILD_OpCodeMax;i++)
	    { if (ctx->pi->V4DtlModCalls[i] > max) { max = ctx->pi->V4DtlModCalls[i] ; maxi = i ; } ; } ;
	   if (max == 0) break ;
	   UCsprintf(tbuf,UCsizeof(tbuf),UClit("%9d  %s\n"),ctx->pi->V4DtlModCalls[maxi],v4im_Display(maxi)) ; vout_UCText(VOUT_Status,0,tbuf) ;
	   ctx->pi->V4DtlModCalls[maxi] = 0 ;
	 } ;
#endif
} ;

