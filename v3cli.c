/*	V44CLI.V3 - C Language Interface to V4

	Created 12/30/93 by Victor E. Hansen		*/

#include "v4defs.c"
#include <setjmp.h>
extern jmp_buf environment ;

UCCHAR *v4dpi_PointToString() ;

/*	v4cli_EvalIsct - Parses & Evaluates Isct, returns value as V4CLI__Value */
/*	Call: resultcode = v4cli_EvalIsct( ctx, isctstr , clival , trace )
	  where resultcode specifies result- V4CLI_RES_xxx,
		ctx is pointer to current context,
		isctstr is pointer to isct string,
		clival is pointer to V4CLI__Value which is updated with result,
		trace is tracing flag						*/

v4cli_EvalIsct(ctx,isctstr,clival,trace)
  struct V4C__Context *ctx ;
  char *isctstr ;
  union V4CLI__Value *clival ;
  int trace ;
{
  struct V4LEX__TknCtrlBlk tcb ;
  struct V4DPI__Point isct,tpt,*ipt ;
  int err ;

	if ((err = setjmp(environment)) != 0) return(err) ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,ASCretUC(isctstr),V4LEX_InpMode_String) ; v4dpi_PointParse(ctx,&isct,&tcb) ;
	ipt = v4dpi_IsctEval(&tpt,&isct,ctx,0,0,NULL) ;
	if (ipt == NULL) return(V4CLI_RES_Undef) ;
	return(v4cli_UpdateCLIVal(ctx,clival,ipt)) ;
}

/*	v4cli_UpdateCLIVal - Updates CLIVal with specified point */
/*	Call: resultcode = v4cli_UpdateCLIVal( ctx , clival , pnt )
	  where resultcode specifies datatype of result- V4CLI_RES_xxx,
	  ctx is current context,
	  clival is pointer to value structure,
	  pnt is V4 point used in update				*/

v4cli_UpdateCLIVal(ctx,clival,pnt)
  struct V4C__Context *ctx ;
  union V4CLI__Value *clival ;
  struct V4DPI__Point *pnt ;
{ struct V4DPI__Point_IntMix *pim ;
  struct V4DPI__Point_AlphaMix *pam ;
  struct V4DPI__Point *ipnt ;
  struct V4FFI__DataElSpec *des ;
  struct V4FFI__StructElSpec *ses ;
  char tb1[250], tb2[250], *tbuf, *ts ; int i,fx ;

	switch (pnt->PntType)
	 { default: v4_error(0,"V3V4","UpdateArg","UNKPTTYPE","Unknown point type: %s",v4im_PTName(pnt->PntType)) ;
	   case V4DPI_PntType_Special:
		switch (pnt->Grouping)
		 { default: v4_error(0,"V3V4","UpdateArg","UNKSPCLPT","Unknown type for special pnt",pnt->Grouping) ;
		   case V4DPI_Grouping_AllCnf:	return(V4CLI_RES_Undef) ;
		   case V4DPI_Grouping_All:	return(V4CLI_RES_Undef) ;
		   case V4DPI_Grouping_Current:	return(V4CLI_RES_Undef) ;
		   case V4DPI_Grouping_PCurrent: return(V4CLI_RES_Undef) ;
		   case V4DPI_Grouping_Binding:	return(V4CLI_RES_Undef) ;
		   case V4DPI_Grouping_List:	return(V4CLI_RES_Undef) ;
		 } ; break ;
	   case V4DPI_PntType_List:
		memcpy(&clival->V4Pnt,pnt,pnt->Bytes) ; return(V4CLI_RES_List) ;
	   case V4DPI_PntType_SegBitMap:
	   case V4DPI_PntType_Time:
	   case V4DPI_PntType_Logical:
	   case V4DPI_PntType_CodedRange:
	   case V4DPI_PntType_Int:
	   case V4DPI_PntType_Tree:
		if (pnt->Grouping == V4DPI_Grouping_Single)
		 { clival->IntVal = pnt->Value.IntVal ; return(V4CLI_RES_Int) ; } ;
/*		Have a list or range or whatever - Push first value */
		pim = (struct V4DPI__Point_IntMix *)&pnt->Value ;
		clival->IntVal = pim->Entry[0].BeginInt ; return(V4CLI_RES_Int) ;
	   case V4DPI_PntType_Color:
		if (pnt->Grouping == V4DPI_Grouping_Single)
		 { strcpy(&tb1,UCretASC(v_ColorRefToName(pnt->Value.IntVal))) ; }
		 else { pim = (struct V4DPI__Point_IntMix *)&pnt->Value ;
			strcpy(&tb1,UCretASC(v_ColorRefToName(pim->Entry[0].BeginInt))) ;
		      } ;
		 break ;
	   case V4DPI_PntType_Country:
		if (pnt->Grouping == V4DPI_Grouping_Single)
		 { UCstrcpyToASC(&tb1,v_CountryRefToName(pnt->Value.IntVal)) ; }
		 else { pim = (struct V4DPI__Point_IntMix *)&pnt->Value ;
			UCstrcpyToASC(&tb1,v_CountryRefToName(pim->Entry[0].BeginInt)) ;
		      } ;
		 break ;
	   case V4DPI_PntType_XDict:
		if (pnt->Grouping == V4DPI_Grouping_Single)
		 { strcpy(&tb1,UCretASC(v4dpi_RevXDictEntryGet(ctx,pnt->Dim,pnt->Value.IntVal))) ; }
		 else { pim = (struct V4DPI__Point_IntMix *)&pnt->Value ;
			strcpy(&tb1,UCretASC(v4dpi_RevXDictEntryGet(ctx,pnt->Dim,pim->Entry[0].BeginInt))) ;
		      } ;
		 break ;
	   case V4DPI_PntType_Dict:
		if (pnt->Grouping == V4DPI_Grouping_Single)
		 { strcpy(&tb1,v4dpi_RevDictEntryGet(ctx,pnt->Value.IntVal)) ; }
		 else { pim = (struct V4DPI__Point_IntMix *)&pnt->Value ;
			strcpy(&tb1,v4dpi_RevDictEntryGet(ctx,pim->Entry[0].BeginInt)) ;
		      } ;
		 break ;
//	   case V4DPI_PntType_V3Mod:
//	   case V4DPI_PntType_V3ModRaw:
	   CASEofChar
		if (pnt->Grouping == V4DPI_Grouping_Single)
		 { if (pnt->Value.AlphaVal[0] < 255)
		    { strncpy(&tb1,&pnt->Value.AlphaVal[1],pnt->Value.AlphaVal[0]) ; tb1[pnt->Value.AlphaVal[0]] = 0 ; }
		    else { strcpy(&tb1,pnt->Value.AlphaVal[1]) ; } ;
		 } else { pam = (struct V4DPI__Point_AlphaMix *)&pnt->Value ; tbuf = (char *)&pam->Entry[pnt->Grouping].BeginIndex ;
			  strncpy(&tb1,tbuf+pam->Entry[0].BeginIndex+1,*(tbuf+pam->Entry[0].BeginIndex)) ;
		          tb1[*(tbuf+pam->Entry[0].BeginIndex)] = 0 ;
			} ;
		break ;
	   case V4DPI_PntType_UTime:
	   case V4DPI_PntType_Real:
	   case V4DPI_PntType_Calendar:
		clival->DblVal = pnt->Value.RealVal ; return(V4CLI_RES_Dbl) ;
	   case V4DPI_PntType_Isct:
		tb1[0] = 0 ; v4dpi_IsctToString(&tb1,pnt,ctx,0,ctx->pi->MaxPointOutput) ;
		break ;
	   case V4DPI_PntType_CmpndIsct:
		tb1[0] = 0 ;					/* Make string 0 length */
		for((fx=0,i=0);i<pnt->Grouping;(fx+=ipnt->Bytes,i++))
		 { ipnt = (P *)&pnt->Value.AlphaVal[fx] ;/* Link up to (next) intersection */
		   if (ipnt->Dim != 0)
	 	    { strcat(tb1,v4dpi_RevDictEntryGet(ctx,ipnt->Dim)) ;
	   	      strcat(tb1,"=") ;
		    } ;
		   tb2[0] = 0 ; v4dpi_IsctToString(tb2,ipnt,ctx,0,ctx->pi->MaxPointOutput) ; strcat(tb1,tb2) ;
		   if (i < pnt->Grouping-1) strcat(tb1,",") ;
		 } ;
		break ;
	   case V4DPI_PntType_Shell:
		tb1[0] = 0 ; v4dpi_PointToString(&tb1,&pnt->Value,ctx,0) ;
		break ;
//	   case V4DPI_PntType_Foreign:
//		break ;
	   case V4DPI_PntType_FrgnDataEl:
		des = (struct V4FFI__DataElSpec *)&pnt->Value ;
		clival->Des = *des ; return(V4CLI_RES_Des) ;
	   case V4DPI_PntType_FrgnStructEl:
		ses = (struct V4FFI__StructElSpec *)&pnt->Value ;
		clival->Ses = *ses ; return(V4CLI_RES_Ses) ;
	 } ;
/*	If drop thru to here then returning string in tb1 */
	strcpy(clival->StrVal,tb1) ; return(V4CLI_RES_String) ;
}

struct V4C__Context *v4cli_InitContext()
{ struct V4C__Context *ctx ;

	   ctx = (struct V4C__Context *)v4mm_AllocChunk(sizeof *ctx,TRUE) ;
	   v4ctx_Initialize(ctx,NULL,NULL) ;			/* Initialize context */
	   return(ctx) ;
}

v4cli_SetupContext(ctx,logical,optlist)
  struct V4C__Context *ctx ;
  char *logical ;				/* Pointer to logical with list of context points */
  char *optlist ;				/* Optional list of other context points */
{
  struct V4LEX__TknCtrlBlk tcb ;
  struct V4DPI__Point tpt ;
  char buf[500] ;
  int err,pntcount ;

	if ((err = setjmp(environment)) != 0) return(err) ;
	if (logical == NULL) logical = "v4_setupcontext" ;	/* Supply default */
	pntcount = 0 ;
	if (v3_GetEnvValue(ASCretUC(logical),buf,UCsizeof(buf)))
	 { v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,buf,V4LEX_InpMode_String) ;
	   for(;;)
	    { v4dpi_PointParse(ctx,&tpt,&tcb) ; v4ctx_FrameAddDim(ctx,0,&tpt,0) ; pntcount ++ ;
	      v4lex_NextTkn(&tcb,V4LEX_Option_RetEOL) ;
	      if (tcb.type == V4LEX_TknType_EOL) break ;
	      v4lex_NextTkn(&tcb,V4LEX_Option_PushCur) ;
	    } ;
	 } ;
	if (optlist != NULL)
	 { v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,ASCretUC(optlist),V4LEX_InpMode_String) ;
	   for(;;)
	    { v4dpi_PointParse(ctx,&tpt,&tcb) ; v4ctx_FrameAddDim(ctx,0,&tpt,0) ; pntcount ++ ;
	      v4lex_NextTkn(&tcb,V4LEX_Option_RetEOL) ;
	      if (tcb.type == V4LEX_TknType_EOL) break ;
	      v4lex_NextTkn(&tcb,V4LEX_Option_PushCur) ;
	    } ;
	 } ;
	return(pntcount) ;				/* Return number of points added */
}

int v4cli_AreaRead(ctx,name)
  struct V4C__Context *ctx ;
  char *name ;
{
  struct V4IS__ParControlBlk pcb ;
  struct V4C__AreaHInfo ahi ;

	memset(&ahi,0,sizeof ahi) ;
	ahi.RelHNum = 0 ; ahi.ExtDictUpd = TRUE ; ahi.IntDictUpd = TRUE ; ahi.BindingsUpd = TRUE ;
	memset(&pcb,0,sizeof pcb) ;
	strcpy(pcb.V3name,"areau") ; strcpy(pcb.FileName,name) ;
	pcb.AccessMode = -1 ; pcb.OpenMode = V4IS_PCB_OM_Read ;
	v4is_Open(&pcb,NULL,NULL) ; if (v4ctx_AreaAdd(ctx,&pcb,&ahi,NULL) == UNUSED) return(FALSE) ;
	return(TRUE) ;
}

/*	M E N U    R O U T I N E S			*/

/*	v4gui_MakeMenuList - Builds up menu list, returns pointer to it */
/*	Call: pointer = v4gui_MakeMenuList( ctx , menuname , listpoint )
	  where pointer is pointer to V4GUI__Menu,
		ctx is pointer to current context,
		menuname is name of top-level menu entry,
		listpoint is pointer to list of menu items (menuname s/b "" if this is given) */

struct V4GUI__Menu *v4gui_MakeMenuList(ctx,menuname,listpoint)
  struct V4C__Context *ctx ;
  char *menuname ;
  struct V4DPI__Point *listpoint ;
{
  struct V4GUI__Menu *menu ;
  union V4CLI__Value listval,tval ;
  struct V4DPI__Point spnt ;
  struct V4L__ListPoint *lp ;
  struct V4LEX__TknCtrlBlk tcb ;
  struct V4DPI__Point isct,tpt,*ipt ;
  int err ; char buf[200] ;

	if ((err = setjmp(environment)) != 0) return(err) ;
	menu = (struct V4GUI__Menu *)v4mm_AllocChunk(sizeof *menu,TRUE) ;
	if (strlen(menuname) > 0)
	 { v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,ASCretUC(menuname),V4LEX_InpMode_String) ; v4dpi_PointParse(ctx,&isct,&tcb) ;
	   ipt = v4dpi_IsctEval(&tpt,&isct,ctx,0,FALSE,NULL,NULL) ;		/* Eval to get MenuMaster */
	   v4ctx_FrameAddDim(ctx,0,ipt,0) ;
	   v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,0,UClit("[MenuBar MenuMaster*]"),V4LEX_InpMode_String) ;
	   v4dpi_PointParse(ctx,&isct,&tcb) ; ipt = v4dpi_IsctEval(&tpt,&isct,ctx,0,FALSE,NULL,NULL) ;
	   v4ctx_FrameAddDim(ctx,0,ipt,0) ;					/* Add MenuList to context */
	   v4cli_EvalIsct(ctx,"[MenuList MenuList*]",&listval,FALSE) ;
	   lp = ALIGNLP(&listval.V4Pnt.Value) ;
	 } else { lp = ALIGNLP(&listpoint->Value) ; } ;
	for(menu->Count=0;menu->Count < V4GUI_MenuEntry_Max;menu->Count++)
	 { if (v4l_ListPoint_Value(lp,menu->Count+1,&spnt) < 1) break ;		/* Get next menu entry in list */
	   v4ctx_FrameAddDim(ctx,0,&spnt,0) ;					/* Add point to current context */
	   v4cli_EvalIsct(ctx,"[Display MenuEntry*]",&tval,FALSE) ; strcpy(menu->Entry[menu->Count].Display,tval.StrVal) ;
	   if (v4cli_EvalIsct(ctx,"[Name MenuEntry*]",&tval,FALSE) == V4CLI_RES_String)
	    { strcpy(menu->Entry[menu->Count].Name,tval.StrVal) ; }
	    else { strcpy(menu->Entry[menu->Count].Name,menu->Entry[menu->Count].Display) ; } ;
	   v4cli_EvalIsct(ctx,"[Id MenuEntry*]",&tval,FALSE) ;
	   if ((menu->Entry[menu->Count].Id = tval.IntVal) != 0) continue ;
	   sprintf(buf,"[MenuList [%s MenuMaster*]]",menu->Entry[menu->Count].Name) ;	/* Get MenuList point for this entry */
	   if (v4cli_EvalIsct(ctx,buf,&tval,FALSE) != V4CLI_RES_List)			/* tval = list of menu entries */
	    v4_error(0,"V4GUI","MAKEMENULIST","NOTLIST","Isct (%s) did not evaluate to list",buf) ;
	   menu->Entry[menu->Count].Menu = v4gui_MakeMenuList(ctx,"",&tval.V4Pnt) ;
	 } ;
	return(menu) ;
}

/*	v4gui_AppendUserMenu - Attempts to append a user menu to end of menu(bar)	*/
/*	Call: logical = v4gui_AppendUserMenu( ctx, menu, name )
	  where logical is TRUE if menu appended,
		ctx is context,
		menu is pointer to menu,
		name is name of user menu to attempt to append				*/

v4gui_AppendUserMenu(ctx,mainmenu,name)
  struct V4C__Context *ctx ;
  struct V4GUI__Menu *mainmenu ;
  char *name ;
{
  union V4CLI__Value listval,tval ;
  struct V4GUI__Menu *menu ;
  struct V4DPI__Point spnt ;
  struct V4L__ListPoint *lp ;
  struct V4LEX__TknCtrlBlk tcb ;
  struct V4DPI__Point isct,tpt,*ipt ;
  int err ;

	if ((err = setjmp(environment)) != 0) return(err) ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,0,ASCretUC(name),V4LEX_InpMode_String) ; v4dpi_PointParse(ctx,&isct,&tcb) ;
	ipt = v4dpi_IsctEval(&tpt,&isct,ctx,0,FALSE,NULL,NULL) ;		/* Eval to get UserMenu point */
	if (ipt == NULL) return(FALSE) ;					/* Could not find ? */
	v4ctx_FrameAddDim(ctx,0,ipt,0) ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,0,UClit("[FunctionList UserMenu*]"),V4LEX_InpMode_String) ;
	v4dpi_PointParse(ctx,&isct,&tcb) ; ipt = v4dpi_IsctEval(&tpt,&isct,ctx,0,FALSE,NULL,NULL) ;
	lp = ALIGNLP(&ipt->Value) ;
	menu = (struct V4GUI__Menu *)v4mm_AllocChunk(sizeof *menu,TRUE) ;	/* Allocate for menu */
	if (v4cli_EvalIsct(ctx,"[Display UserMenu*]",&tval,FALSE) != V4CLI_RES_String) return(FALSE) ;
	if (mainmenu != NULL)
	 { strcpy(mainmenu->Entry[mainmenu->Count].Name,tval.StrVal) ;		/* Copy menubar name */
	   strcpy(mainmenu->Entry[mainmenu->Count].Display,tval.StrVal) ;
	   mainmenu->Entry[mainmenu->Count].Id = 0 ;
	   mainmenu->Entry[mainmenu->Count].Menu = menu ;			/* Link to sub-menu */
	 } ;
	for(menu->Count=0;menu->Count < V4GUI_MenuEntry_Max;menu->Count++)	/* Now fill in details of menu */
	 { if (v4l_ListPoint_Value(lp,menu->Count+1,&spnt) < 1) break ;		/* Get next menu/function entry in list */
	   v4ctx_FrameAddDim(ctx,0,&spnt,0) ;					/* Add point to current context */
	   v4cli_EvalIsct(ctx,"[Display Function*]",&tval,FALSE) ; strcpy(menu->Entry[menu->Count].Display,tval.StrVal) ;
	   if (v4cli_EvalIsct(ctx,"[Name Function*]",&tval,FALSE) == V4CLI_RES_String)
	    { strcpy(menu->Entry[menu->Count].Name,tval.StrVal) ; }
	    else { strcpy(menu->Entry[menu->Count].Name,menu->Entry[menu->Count].Display) ; } ;
	   v4cli_EvalIsct(ctx,"[Id Function*]",&tval,FALSE) ;
	   if ((menu->Entry[menu->Count].Id = tval.IntVal) != 0) continue ;
	   if (v4cli_EvalIsct(ctx,"[UserMenu Function*]",&tval,FALSE) != V4CLI_RES_String)	/* tval = list of menu entries */
	    v4_error(0,"V4GUI","MAKEMENULIST","NOTLIST","Isct ([UserMenu Function*]) did not evaluate to string") ;
	   menu->Entry[menu->Count].Menu = v4gui_AppendUserMenu(ctx,NULL,&tval.StrVal) ;
	 } ;
	if (mainmenu != NULL) mainmenu->Count ++ ;
	return(menu) ;
}

/*	E L E M E N T   L I S T   M O D U L E S			*/

/*	v4gui_MakeElList - Builds up list of elements and returns pointer to it	*/
/*	Call: pointer = v4_MakeElList( ctx , isctstr )
	  where pointer is pointer to V4GUI_ElList structure,
		ctx is current context,
		isctstr is isct string to be evaluated for starting point	*/

struct V4GUI__ElList *v4gui_MakeElList(ctx,isctstr)
  struct V4C__Context *ctx ;
  char *isctstr ;
{ struct V4GUI__ElList el,*elptr ;		/* Local structure to be built up */
  struct V4DPI__Point *ipt,tpt,isct,lpt ;
  struct V4L__ListPoint *lp ;
  struct V4LEX__TknCtrlBlk tcb ;
  union V4CLI__Value tval ;
  int err,i ; char buf[250] ;

	if ((err = setjmp(environment)) != 0) return(err) ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,0,isctstr,V4LEX_InpMode_String) ; v4dpi_PointParse(ctx,&isct,&tcb) ;
	ipt = v4dpi_IsctEval(&tpt,&isct,ctx,0,FALSE,NULL,NULL) ;		/* Eval to get Window:xxx point */
	v4ctx_FrameAddDim(ctx,0,ipt,0) ;
	if (v4cli_EvalIsct(ctx,"[MenuMaster Window*]",&tval,FALSE) == V4CLI_RES_String)
	 { sprintf(buf,"[%s Dim:MenuMaster]",tval.StrVal) ; el.Menu = v4gui_MakeMenuList(ctx,buf,NULL) ;
	   v4gui_AppendUserMenu(ctx,el.Menu,"[UserMenu1 Window*]") ;
	   v4gui_AppendUserMenu(ctx,el.Menu,"[UserMenu2 Window*]") ;
	   v4gui_AppendUserMenu(ctx,el.Menu,"[UserMenu3 Window*]") ;
	 } else el.Menu = NULL ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,0,"[ElList Window*]",V4LEX_InpMode_String) ;
	v4dpi_PointParse(ctx,&isct,&tcb) ; ipt = v4dpi_IsctEval(&lpt,&isct,ctx,NULL,FALSE,0,NULL) ;
	lp = ALIGNLP(&lpt.Value) ;
	for(el.ElCount=0;el.ElCount < V4GUI_MenuEntry_Max;el.ElCount++)
	 { if (v4l_ListPoint_Value(lp,el.ElCount+1,&tpt) < 1) break ;		/* Get next el entry in list */
	   v4ctx_FrameAddDim(ctx,0,&tpt,0) ;					/* Add point to current context */
	   memcpy(&el.Entry[el.ElCount].ElPoint,&tpt,sizeof el.Entry[0].ElPoint) ;
	   v4cli_EvalIsct(ctx,"[Prompt El*]",&tval,FALSE) ; strcpy(el.Entry[el.ElCount].Prompt,tval.StrVal) ;
	   v4cli_EvalIsct(ctx,"[ElName El*]",&tval,FALSE) ; strcpy(el.Entry[el.ElCount].ElName,tval.StrVal) ;
	   if (v4cli_EvalIsct(ctx,"[Label El*]",&tval,FALSE) == V4CLI_RES_String)
	    { strcpy(el.Entry[el.ElCount].Label,tval.StrVal) ; }
	    else { strcpy(el.Entry[el.ElCount].Label,el.Entry[el.ElCount].ElName) ; } ;
	 } ;
	i = (char *)&el.Entry[el.ElCount+1] - (char *)&el ;	/* All done, allocate just what we need */
	elptr = (struct V4GUI__ElList *)v4mm_AllocChunk(i,FALSE) ;
	memcpy(elptr,&el,i) ; return(elptr) ;
}

/*	v4gui_MakeTableHelp - Sets up a Table Help Structure		*/
/*	Call: pointer = v4gui_MakeTableHelp( ctx , isctstr )
	  where pointer is pointer to newly created V4GUI__Table structure,
		ctx is current context,
		isctstr is string to be evaluated for top level Table point	*/

struct V4GUI__Table *v4gui_MakeTableHelp(ctx,isctstr)
  struct V4C__Context *ctx ;
  char *isctstr ;
{ struct V4GUI__Table table,*tablep ;		/* Local structure to be built up */
  struct V4DPI__Point *ipt,tpt,isct,lpt ;
  struct V4L__ListPoint *lp ;
  struct V4LEX__TknCtrlBlk tcb ;
  union V4CLI__Value tval ;
  int err,i ; char buf[250] ;

	if ((err = setjmp(environment)) != 0) return(NULL) ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,0,isctstr,V4LEX_InpMode_String) ; v4dpi_PointParse(ctx,&isct,&tcb) ;
	ipt = v4dpi_IsctEval(&tpt,&isct,ctx,NULL,FALSE,0,NULL) ;	/* Eval to get Table:xxx point */
	v4ctx_FrameAddDim(ctx,0,ipt,0) ;				/* Add to context */
	v4cli_EvalIsct(ctx,"[Help Table*]",&tval,FALSE) ; strcpy(table.TableHelp,tval.StrVal) ;
	v4cli_EvalIsct(ctx,"[Name Table*]",&tval,FALSE) ; strcpy(table.TableName,tval.StrVal) ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,0,"[EntryList Table*]",V4LEX_InpMode_String) ;
	v4dpi_PointParse(ctx,&isct,&tcb) ; ipt = v4dpi_IsctEval(&lpt,&isct,ctx,NULL,FALSE,0,NULL) ;
	lp = ALIGNLP(&lpt.Value) ;			/* Link up to table-entry list */
	for(table.Count=0;table.Count < V4GUI_TableEntry_Max;table.Count++)
	 { if (v4l_ListPoint_Value(lp,table.Count+1,&tpt) < 1) break ;		/* Get next table-entry in list */
	   v4ctx_FrameAddDim(ctx,0,&tpt,0) ;					/* Add point to current context */
	   memcpy(&table.Entry[table.Count].Point,&tpt,sizeof table.Entry[0].Point) ;
	   v4cli_EvalIsct(ctx,"[Display TableEntry*]",&tval,FALSE) ; strcpy(table.Entry[table.Count].DisplayText,tval.StrVal) ;
	   v4cli_EvalIsct(ctx,"[Name TableEntry*]",&tval,FALSE) ; strcpy(table.Entry[table.Count].Name,tval.StrVal) ;
	   v4cli_EvalIsct(ctx,"[Help TableEntry*]",&tval,FALSE) ; strcpy(table.Entry[table.Count].EntryHelp,tval.StrVal) ;
	   v4cli_EvalIsct(ctx,"[Internal TableEntry*]",&tval,FALSE) ; table.Entry[table.Count].Internal = tval.IntVal ;
	 } ;
	v4cli_EvalIsct(ctx,"[ChooseMult Table*]",&tval,FALSE) ; table.ChooseMult = tval.IntVal ;
#define THA(nid,el) sprintf(buf,"[%s WinParams*]",nid) ; v4cli_EvalIsct(ctx,buf,&tval,FALSE) ; strcpy(table.el,tval.StrVal) ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,0,"[TableHelp Dim:WinParams]",V4LEX_InpMode_String) ;
	v4dpi_PointParse(ctx,&isct,&tcb) ; ipt = v4dpi_IsctEval(&tpt,&isct,ctx,NULL,FALSE,0,NULL) ;
	v4ctx_FrameAddDim(ctx,0,ipt,0) ;		/* Add WinParams to context for more info */
	THA("OKText",OKText) THA("CancelText",CancelText) THA("AllText",AllText) THA("ClearText",ClearText)
#define THI(nid,el) sprintf(buf,"[%s WinParams*]",nid) ; v4cli_EvalIsct(ctx,buf,&tval,FALSE) ; table.el = tval.IntVal ;
	THI("TitleHeight",TitleHeight) THI("ButtonHeight",ButtonHeight) THI("VerticalGap",VerticalGap)
	THI("HorizontalGap",HorizontalGap) THI("MinListWidth",MinListWidth) THI("MinEditWidth",MinEditWidth)
	THI("MinListHeight",MinListHeight)
	i = (char *)&table.Entry[table.Count] - (char *)&table ;
	tablep = (struct V4GUI__Table *)v4mm_AllocChunk(i,FALSE) ; memcpy(tablep,&table,i) ;
	return(tablep) ;
}

/*	E R R O R   H A N D L E R S			*/

v3_error(subsys,module,mnemonic,msg,optarg)
  char *subsys,*module,*mnemonic,*msg ;
  int optarg ;
{
	v4_error(0,subsys,module,mnemonic,msg,optarg) ;
}

struct V4ERR__Info v4errinfo ;

#ifdef ALTVARGS						/* Need funky header for SCO variable-arg handler */
v4_error(va_alist)
  va_dcl
{ extern int *v3unitp ;
  struct V4LEX__TknCtrlBlk *tcb ;
  char *subsys,*module,*mne,*msg ;
  va_list args ; char *format ;
  char ebuf[250] ; int code ;
  char tmp[10000] ; int i,j,nx ;

/*	Do special formatting on error message */
	va_start(args) ; tcb = va_arg(args,struct V4LEX__TknCtrlBlk *) ;
	subsys = va_arg(args,char *) ; module = va_arg(args,char *) ; mne = va_arg(args,char *) ;
	format = va_arg(args,char *) ; vsprintf(ebuf,format,args) ; va_end(args) ;
#else							/* All other UNIX */
v4_error(tcb,subsys,module,mne,msg)
  struct V4LEX__TknCtrlBlk *tcb ;
  char *subsys,*module,*mne,*msg ;

{ extern int *v3unitp ;
  va_list ap ;
  char ebuf[250] ; int code ;
  char tmp[10000] ; int i,j,nx ;

/*	Do special formatting on error message */
	va_start(ap,msg) ; msg = va_arg(ap,char *) ; vsprintf(ebuf,msg,ap) ; va_end(ap) ;
#endif
/*	Copy into global error block */
	strcpy(v4errinfo.subsys,subsys) ; strcpy(v4errinfo.module,module) ; strcpy(v4errinfo.mne,mne) ;
	strcpy(v4errinfo.msg,ebuf) ; v4errinfo.tcb = tcb ; v4errinfo.code = 0 ;
/*	Should we point to offending whatever ? */
	i = (int)tcb ; if (i < 100) tcb = NULL ;
	if (tcb != NULL)
	 { if (tcb->ilvl[tcb->ilx].last_page_printed == 0)
	    { tcb->ilvl[tcb->ilx].last_page_printed = 1 ;		/* Print out filename */
	      if (tcb->ilvl[tcb->ilx].file_name[0] != 0)
	       fprintf(stdout,"Error in file: %s\n",tcb->ilvl[tcb->ilx].file_name) ;
	    } ;
	   strcpy(tmp,tcb->ilvl[tcb->ilx].input_str) ;
/*	   Set up to make pointer */
	   j = (char *)tcb->ilvl[tcb->ilx].input_ptr - (char *)(&(tcb->ilvl[tcb->ilx].input_str)) ;
	   for (i=0;i<j-1;i++)
	    { if (tmp[i] > ' ') tmp[i] = ' ' ; } ;
	   tmp[i] = '^' ; tmp[++i] = NULL ;
	   fprintf(stdout,"%d.	%s",tcb->ilvl[tcb->ilx].current_line,tcb->ilvl[tcb->ilx].input_str) ;
	   if (tcb->ilvl[tcb->ilx].input_str[strlen(tcb->ilvl[tcb->ilx].input_str)-1] > 15) fprintf(stdout,"\n") ;
	   fprintf(stdout,"	%s\n",tmp) ;
	   switch(tcb->type)
	    { default: printf("Unknown token type?\n") ;
	      case V4LEX_TknType_RegExpStr:
	      case V4LEX_TknType_BString:
	      case V4LEX_TknType_VString:
	      case V4LEX_TknType_PString:
	      case V4LEX_TknType_String:	printf("Token-String : %s\n",UCretASC(tcb->UCstring)) ; break ;
	      case V4LEX_TknType_Keyword:	printf("Token-Keyword: %s\n",UCretASC(tcb->UCkeyword)) ; break ;
	      case V4LEX_TknType_Punc:		printf("Token-Punc: %s\n",UCretASC(tcb->UCkeyword)) ; break ;
	      case V4LEX_TknType_Integer:	printf("Token-Integer: %d(%d)\n",tcb->integer,tcb->decimal_places) ; break ;
	      case V4LEX_TknType_Float:		printf("Token-Float  : %G\n",tcb->floating) ; break ;
	      case V4LEX_TknType_EOL:		printf("Token-End-of-Line\n") ; break ;
	    } ;
	 } ;
	printf("?Err (%s:%s) - %s\n",module,mne,ebuf) ;
	longjmp(environment,1) ;
}

v3_v4_EvalV3Mod()
{
	v4_error(0,"V4TEST","EvalV3MOD","NOV3MOD","V3Mod/V3ModRaw point types not supported outside of V3") ;
}

v3v4_v4im_Load_V3PicMod()
{
	v4_error(0,"V4TEST","V3PicMod","NOV3MOD","Load of V3 module not supported outside of V3") ;
}

v3v4_v4im_Unload_V3PicMod()
{
	v4_error(0,"V4TEST","V3PicMod","NOV3MODUNL","UnLoad of V3 module not supported outside of V3") ;
}

/*	W I N D O W S   N T   S P E C I F I C	*/

#ifdef WINNT

/*	v4gui_MenuHandle - Returns handle to menu from V4GUI__Menu pointer	*/
/*	Call: handle = v4gui_MenuHandle( menu )
	  where handle is menu handle for Windows,
	  	menu is pointer to V4 internal menu structure			*/

HMENU v4gui_MenuHandle(menu)
  struct V4GUI__Menu *menu ;
{ HMENU hmenu ;
  int i ;
	hmenu = CreateMenu() ;
	for(i=0;i<menu->Count;i++)
	 { if (menu->Entry[i].Id != 0)
	    { AppendMenu(hmenu,MF_STRING,menu->Entry[i].Id,menu->Entry[i].Display) ;
	    } else
	    { AppendMenu(hmenu,MF_POPUP+MF_STRING,(int)v4gui_MenuHandle(menu->Entry[i].Menu),menu->Entry[i].Display) ;
	    } ;
	 } ;
	return(hmenu) ;
}
#endif
