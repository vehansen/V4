/*	V44CLI.V3 - C Language Interface to V4

	Created 12/30/93 by Victor E. Hansen		*/

#include "v4defs.c"
#include <setjmp.h>

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

	if ((err = setjmp(ctx->pi->environment)) != 0) return(err) ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,ASCretUC(isctstr),V4LEX_InpMode_String) ; v4dpi_PointParse(ctx,&isct,&tcb,0) ;
	ipt = v4dpi_IsctEval(&tpt,&isct,ctx,NULL,NULL) ;
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
  char v4tbuf[500] ; UCCHAR v4utbuf[500] ;
  char tb2[250], *tbuf, *ts ; int i,fx ;

	switch (pnt->PntType)
	 { default: v4_error(0,0,"V3V4","UpdateArg","UNKPTTYPE","Unknown point type: %s",v4im_PTName(pnt->PntType)) ;
	   case V4DPI_PntType_Special:
		switch (pnt->Grouping)
		 { default: v4_error(0,0,"V3V4","UpdateArg","UNKSPCLPT","Unknown type for special point",pnt->Grouping) ;
		   case V4DPI_Grouping_All:	return(V4CLI_RES_Undef) ;
		   case V4DPI_Grouping_AllCnf:	return(V4CLI_RES_Undef) ;
		   case V4DPI_Grouping_Current:	return(V4CLI_RES_Undef) ;
		 } ; break ;
	   case V4DPI_PntType_List:
		memcpy(&clival->V4Pnt,pnt,pnt->Bytes) ; return(V4CLI_RES_List) ;
	   case V4DPI_PntType_Tree:
	   CASEofINT
		if (pnt->Grouping == V4DPI_Grouping_Single)
		 { clival->IntVal = pnt->Value.IntVal ; return(V4CLI_RES_Int) ; } ;
/*		Have a list or range or whatever - Push first value */
		pim = (struct V4DPI__Point_IntMix *)&pnt->Value ;
		clival->IntVal = pim->Entry[0].BeginInt ; return(V4CLI_RES_Int) ;
	   case V4DPI_PntType_Color:
		if (pnt->Grouping == V4DPI_Grouping_Single)
		 { strcpy(v4tbuf,UCretASC(v_ColorRefToName(pnt->Value.IntVal))) ; }
		 else { pim = (struct V4DPI__Point_IntMix *)&pnt->Value ;
			strcpy(v4tbuf,UCretASC(v_ColorRefToName(pim->Entry[0].BeginInt))) ;
		      } ;
		 break ;
	   case V4DPI_PntType_Country:
		if (pnt->Grouping == V4DPI_Grouping_Single)
		 { UCstrcpyToASC(v4tbuf,v_CountryRefToName(pnt->Value.IntVal)) ; }
		 else { pim = (struct V4DPI__Point_IntMix *)&pnt->Value ;
			UCstrcpyToASC(v4tbuf,v_CountryRefToName(pim->Entry[0].BeginInt)) ;
		      } ;
		 break ;
	   case V4DPI_PntType_XDict:
		if (pnt->Grouping == V4DPI_Grouping_Single)
		 { strcpy(v4tbuf,UCretASC(v4dpi_RevXDictEntryGet(ctx,pnt->Dim,pnt->Value.IntVal))) ; }
		 else { pim = (struct V4DPI__Point_IntMix *)&pnt->Value ;
			strcpy(v4tbuf,UCretASC(v4dpi_RevXDictEntryGet(ctx,pnt->Dim,pim->Entry[0].BeginInt))) ;
		      } ;
		 break ;
	   case V4DPI_PntType_Dict:
		if (pnt->Grouping == V4DPI_Grouping_Single)
		 { strcpy(v4tbuf,UCretASC(v4dpi_RevDictEntryGet(ctx,pnt->Value.IntVal))) ; }
		 else { pim = (struct V4DPI__Point_IntMix *)&pnt->Value ;
			strcpy(v4tbuf,UCretASC(v4dpi_RevDictEntryGet(ctx,pim->Entry[0].BeginInt))) ;
		      } ;
		 break ;
//	   case V4DPI_PntType_V3Mod:
//	   case V4DPI_PntType_V3ModRaw:
	   CASEofChar
		if (pnt->Grouping == V4DPI_Grouping_Single)
		 { if (pnt->Value.AlphaVal[0] < 255)
		    { strncpy(v4tbuf,&pnt->Value.AlphaVal[1],pnt->Value.AlphaVal[0]) ; v4tbuf[pnt->Value.AlphaVal[0]] = 0 ; }
		    else { strcpy(v4tbuf,&pnt->Value.AlphaVal[1]) ; } ;
		 } else { pam = (struct V4DPI__Point_AlphaMix *)&pnt->Value ; tbuf = (char *)&pam->Entry[pnt->Grouping].BeginIndex ;
			  strncpy(v4tbuf,tbuf+pam->Entry[0].BeginIndex+1,*(tbuf+pam->Entry[0].BeginIndex)) ;
		          v4tbuf[*(tbuf+pam->Entry[0].BeginIndex)] = 0 ;
			} ;
		break ;
	   case V4DPI_PntType_Real:
	   case V4DPI_PntType_Calendar:
		GETREAL(clival->DblVal,pnt) ; return(V4CLI_RES_Dbl) ;
	   case V4DPI_PntType_Isct:
		ZUS(v4utbuf) ; v4dpi_IsctToString(v4utbuf,pnt,ctx,0,ctx->pi->MaxPointOutput) ;
		UCstrcpyToASC(v4tbuf,v4utbuf) ; break ;
	   case V4DPI_PntType_GeoCoord:
	   case V4DPI_PntType_Complex:
	   case V4DPI_PntType_TeleNum:
	   case V4DPI_PntType_Int2:
	   case V4DPI_PntType_V4IS:
	   case V4DPI_PntType_XDB:
	   case V4DPI_PntType_Shell:
		ZUS(v4utbuf) ; v4dpi_PointToString(v4utbuf,(P *)&pnt->Value,ctx,0) ;
		UCstrcpyToASC(v4tbuf,v4utbuf) ; break ;
//	   case V4DPI_PntType_Foreign:
//		break ;
	   case V4DPI_PntType_FrgnDataEl:
		des = (struct V4FFI__DataElSpec *)&pnt->Value ;
		clival->Des = *des ; return(V4CLI_RES_Des) ;
	   case V4DPI_PntType_FrgnStructEl:
		ses = (struct V4FFI__StructElSpec *)&pnt->Value ;
		clival->Ses = *ses ; return(V4CLI_RES_Ses) ;
	 } ;
/*	If drop thru to here then returning string in v4tbuf */
	strcpy(clival->StrVal,v4tbuf) ; return(V4CLI_RES_String) ;
}

struct V4C__Context *v4cli_InitContext()
{ struct V4C__Context *ctx ;

	   ctx = (struct V4C__Context *)v4mm_AllocChunk(sizeof *ctx,TRUE) ;
	   v4ctx_Initialize(ctx,NULL,NULL) ;			/* Initialize context */
	   return(ctx) ;
}

#ifdef NOTNEEDED
v4cli_SetupContext(ctx,logical,optlist)
  struct V4C__Context *ctx ;
  char *logical ;				/* Pointer to logical with list of context points */
  char *optlist ;				/* Optional list of other context points */
{
  struct V4LEX__TknCtrlBlk tcb ;
  struct V4DPI__Point tpt ;
  char buf[500] ;
  int err,pntcount ;

	if ((err = setjmp(ctx->pi->environment)) != 0) return(err) ;
	if (logical == NULL) logical = "v4_setupcontext" ;	/* Supply default */
	pntcount = 0 ;
	if (v3_GetEnvValue(ASCretUC(logical),buf,UCsizeof(buf)))
	 { v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,buf,V4LEX_InpMode_String) ;
	   for(;;)
	    { v4dpi_PointParse(ctx,&tpt,&tcb,0) ; v4ctx_FrameAddDim(ctx,0,&tpt,0,0) ; pntcount ++ ;
	      v4lex_NextTkn(&tcb,V4LEX_Option_RetEOL) ;
	      if (tcb.type == V4LEX_TknType_EOL) break ;
	      v4lex_NextTkn(&tcb,V4LEX_Option_PushCur) ;
	    } ;
	 } ;
	if (optlist != NULL)
	 { v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,ASCretUC(optlist),V4LEX_InpMode_String) ;
	   for(;;)
	    { v4dpi_PointParse(ctx,&tpt,&tcb,0) ; v4ctx_FrameAddDim(ctx,0,&tpt,0,0) ; pntcount ++ ;
	      v4lex_NextTkn(&tcb,V4LEX_Option_RetEOL) ;
	      if (tcb.type == V4LEX_TknType_EOL) break ;
	      v4lex_NextTkn(&tcb,V4LEX_Option_PushCur) ;
	    } ;
	 } ;
	return(pntcount) ;				/* Return number of points added */
}
#endif

v4cli_AreaRead(ctx,name)
  struct V4C__Context *ctx ;
  char *name ;
{
  struct V4IS__ParControlBlk pcb ;
  struct V4C__AreaHInfo ahi ;

	memset(&ahi,0,sizeof ahi) ;
	ahi.RelHNum = 0 ; ahi.ExtDictUpd = TRUE ; ahi.IntDictUpd = TRUE ; ahi.BindingsUpd = TRUE ;
	memset(&pcb,0,sizeof pcb) ;
	strcpy(pcb.V3name,"areau") ; UCstrcpy(pcb.UCFileName,ASCretUC(name)) ;
	pcb.AccessMode = -1 ; pcb.OpenMode = V4IS_PCB_OM_Read ;
	v4is_Open(&pcb,NULL,NULL) ;
	if (v4ctx_AreaAdd(ctx,&pcb,&ahi,NULL) == UNUSED) return(FALSE) ;
	return(TRUE) ;
}

v4cli_AggRead(ctx,name)
  struct V4C__Context *ctx ;
  char *name ;
{
  struct V4IS__ParControlBlk pcb ;

	memset(&pcb,0,sizeof pcb) ;
	strcpy(pcb.V3name,"areau") ; UCstrcpy(pcb.UCFileName,ASCretUC(name)) ;
	pcb.AccessMode = -1 ; pcb.OpenMode = V4IS_PCB_OM_Read ;
	v4is_Open(&pcb,NULL,NULL) ;
	return(v4im_AggLoadArea(ctx,&pcb,NULL)) ;
}

#ifdef NOTNEEDED
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

	if ((err = setjmp(ctx->pi->environment)) != 0) return(err) ;
	menu = (struct V4GUI__Menu *)v4mm_AllocChunk(sizeof *menu,TRUE) ;
	if (strlen(menuname) > 0)
	 { v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,ASCretUC(menuname),V4LEX_InpMode_String) ; v4dpi_PointParse(ctx,&isct,&tcb,0) ;
	   ipt = v4dpi_IsctEval(&tpt,&isct,ctx,0,FALSE,0,NULL) ;		/* Eval to get MenuMaster */
	   v4ctx_FrameAddDim(ctx,0,ipt,0,0) ;
	   v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,UClit("[MenuBar MenuMaster*]"),V4LEX_InpMode_String) ;
	   v4dpi_PointParse(ctx,&isct,&tcb,0) ; ipt = v4dpi_IsctEval(&tpt,&isct,ctx,0,FALSE,0,NULL) ;
	   v4ctx_FrameAddDim(ctx,0,ipt,0,0) ;					/* Add MenuList to context */
	   v4cli_EvalIsct(ctx,"[MenuList MenuList*]",&listval,FALSE) ;
	   lp = ALIGNLP(&listval.V4Pnt.Value) ;
	 } else { lp = ALIGNLP(&listpoint->Value) ; } ;
	for(menu->Count=0;menu->Count < V4GUI_MenuEntry_Max;menu->Count++)
	 { if (v4l_ListPoint_Value(ctx,lp,menu->Count+1,&spnt) < 1) break ;		/* Get next menu entry in list */
	   v4ctx_FrameAddDim(ctx,0,&spnt,0,0) ;					/* Add point to current context */
	   v4cli_EvalIsct(ctx,"[Display MenuEntry*]",&tval,FALSE) ; strcpy(menu->Entry[menu->Count].Display,tval.StrVal) ;
	   if (v4cli_EvalIsct(ctx,"[Name MenuEntry*]",&tval,FALSE) == V4CLI_RES_String)
	    { strcpy(menu->Entry[menu->Count].Name,tval.StrVal) ; }
	    else { strcpy(menu->Entry[menu->Count].Name,menu->Entry[menu->Count].Display) ; } ;
	   v4cli_EvalIsct(ctx,"[Id MenuEntry*]",&tval,FALSE) ;
	   if ((menu->Entry[menu->Count].Id = tval.IntVal) != 0) continue ;
	   sprintf(buf,"[MenuList [%s MenuMaster*]]",menu->Entry[menu->Count].Name) ;	/* Get MenuList point for this entry */
	   if (v4cli_EvalIsct(ctx,buf,&tval,FALSE) != V4CLI_RES_List)			/* tval = list of menu entries */
	    v4_error(V4E_MENUNOTLIST,0,"V4GUI","MAKEMENULIST","MENUNOTLIST","Isct (%s) did not evaluate to list",buf) ;
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

	if ((err = setjmp(ctx->pi->environment)) != 0) return(err) ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,ASCretUC(name),V4LEX_InpMode_String) ; v4dpi_PointParse(ctx,&isct,&tcb,0) ;
	ipt = v4dpi_IsctEval(&tpt,&isct,ctx,0,FALSE,0,NULL) ;		/* Eval to get UserMenu point */
	if (ipt == NULL) return(FALSE) ;					/* Could not find ? */
	v4ctx_FrameAddDim(ctx,0,ipt,0,0) ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,UClit("[FunctionList UserMenu*]"),V4LEX_InpMode_String) ;
	v4dpi_PointParse(ctx,&isct,&tcb,0) ; ipt = v4dpi_IsctEval(&tpt,&isct,ctx,0,FALSE,0,NULL) ;
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
	 { if (v4l_ListPoint_Value(ctx,lp,menu->Count+1,&spnt) < 1) break ;		/* Get next menu/function entry in list */
	   v4ctx_FrameAddDim(ctx,0,&spnt,0,0) ;					/* Add point to current context */
	   v4cli_EvalIsct(ctx,"[Display Function*]",&tval,FALSE) ; strcpy(menu->Entry[menu->Count].Display,tval.StrVal) ;
	   if (v4cli_EvalIsct(ctx,"[Name Function*]",&tval,FALSE) == V4CLI_RES_String)
	    { strcpy(menu->Entry[menu->Count].Name,tval.StrVal) ; }
	    else { strcpy(menu->Entry[menu->Count].Name,menu->Entry[menu->Count].Display) ; } ;
	   v4cli_EvalIsct(ctx,"[Id Function*]",&tval,FALSE) ;
	   if ((menu->Entry[menu->Count].Id = tval.IntVal) != 0) continue ;
	   if (v4cli_EvalIsct(ctx,"[UserMenu Function*]",&tval,FALSE) != V4CLI_RES_String)	/* tval = list of menu entries */
	    v4_error(V4E_UMENUNOTLIST,0,"V4GUI","MAKEMENULIST","UMENUNOTLIST","Isct ([UserMenu Function*]) did not evaluate to string") ;
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

	if ((err = setjmp(ctx->pi->environment)) != 0) return(err) ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,ASCretUC(isctstr),V4LEX_InpMode_String) ; v4dpi_PointParse(ctx,&isct,&tcb,0) ;
	ipt = v4dpi_IsctEval(&tpt,&isct,ctx,NULL,FALSE,0,NULL) ;		/* Eval to get Window:xxx point */
	v4ctx_FrameAddDim(ctx,0,ipt,0,0) ;
	if (v4cli_EvalIsct(ctx,"[MenuMaster Window*]",&tval,FALSE) == V4CLI_RES_String)
	 { sprintf(buf,"[%s Dim:MenuMaster]",tval.StrVal) ; el.Menu = v4gui_MakeMenuList(ctx,buf,NULL) ;
	   v4gui_AppendUserMenu(ctx,el.Menu,"[UserMenu1 Window*]") ;
	   v4gui_AppendUserMenu(ctx,el.Menu,"[UserMenu2 Window*]") ;
	   v4gui_AppendUserMenu(ctx,el.Menu,"[UserMenu3 Window*]") ;
	 } else el.Menu = NULL ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,UClit("[ElList Window*]"),V4LEX_InpMode_String) ;
	v4dpi_PointParse(ctx,&isct,&tcb,0) ; ipt = v4dpi_IsctEval(&lpt,&isct,ctx,NULL,FALSE,0,NULL) ;
	lp = ALIGNLP(&lpt.Value) ;
	for(el.ElCount=0;el.ElCount < V4GUI_MenuEntry_Max;el.ElCount++)
	 { if (v4l_ListPoint_Value(ctx,lp,el.ElCount+1,&tpt) < 1) break ;		/* Get next el entry in list */
	   v4ctx_FrameAddDim(ctx,0,&tpt,0,0) ;					/* Add point to current context */
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

	if ((err = setjmp(ctx->pi->environment)) != 0) return(NULL) ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,ASCretUC(isctstr),V4LEX_InpMode_String) ; v4dpi_PointParse(ctx,&isct,&tcb,0) ;
	ipt = v4dpi_IsctEval(&tpt,&isct,ctx,NULL,FALSE,0,NULL) ;	/* Eval to get Table:xxx point */
	v4ctx_FrameAddDim(ctx,0,ipt,0,0) ;				/* Add to context */
	v4cli_EvalIsct(ctx,"[Help Table*]",&tval,FALSE) ; strcpy(table.TableHelp,tval.StrVal) ;
	v4cli_EvalIsct(ctx,"[Name Table*]",&tval,FALSE) ; strcpy(table.TableName,tval.StrVal) ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,UClit("[EntryList Table*]"),V4LEX_InpMode_String) ;
	v4dpi_PointParse(ctx,&isct,&tcb,0) ; ipt = v4dpi_IsctEval(&lpt,&isct,ctx,NULL,FALSE,0,NULL) ;
	lp = ALIGNLP(&lpt.Value) ;			/* Link up to table-entry list */
	for(table.Count=0;table.Count < V4GUI_TableEntry_Max;table.Count++)
	 { if (v4l_ListPoint_Value(ctx,lp,table.Count+1,&tpt) < 1) break ;		/* Get next table-entry in list */
	   v4ctx_FrameAddDim(ctx,0,&tpt,0,0) ;					/* Add point to current context */
	   memcpy(&table.Entry[table.Count].Point,&tpt,sizeof table.Entry[0].Point) ;
	   v4cli_EvalIsct(ctx,"[Display TableEntry*]",&tval,FALSE) ; strcpy(table.Entry[table.Count].DisplayText,tval.StrVal) ;
	   v4cli_EvalIsct(ctx,"[Name TableEntry*]",&tval,FALSE) ; strcpy(table.Entry[table.Count].Name,tval.StrVal) ;
	   v4cli_EvalIsct(ctx,"[Help TableEntry*]",&tval,FALSE) ; strcpy(table.Entry[table.Count].EntryHelp,tval.StrVal) ;
	   v4cli_EvalIsct(ctx,"[Internal TableEntry*]",&tval,FALSE) ; table.Entry[table.Count].Internal = tval.IntVal ;
	 } ;
	v4cli_EvalIsct(ctx,"[ChooseMult Table*]",&tval,FALSE) ; table.ChooseMult = tval.IntVal ;
#define THA(nid,el) sprintf(buf,"[%s WinParams*]",nid) ; v4cli_EvalIsct(ctx,buf,&tval,FALSE) ; strcpy(table.el,tval.StrVal) ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,UClit("[TableHelp Dim:WinParams]"),V4LEX_InpMode_String) ;
	v4dpi_PointParse(ctx,&isct,&tcb,0) ; ipt = v4dpi_IsctEval(&tpt,&isct,ctx,NULL,FALSE,0,NULL) ;
	v4ctx_FrameAddDim(ctx,0,ipt,0,0) ;		/* Add WinParams to context for more info */
	THA("OKText",OKText) THA("CancelText",CancelText) THA("AllText",AllText) THA("ClearText",ClearText)
#define THI(nid,el) sprintf(buf,"[%s WinParams*]",nid) ; v4cli_EvalIsct(ctx,buf,&tval,FALSE) ; table.el = tval.IntVal ;
	i = (char *)&table.Entry[table.Count] - (char *)&table ;
	tablep = (struct V4GUI__Table *)v4mm_AllocChunk(i,FALSE) ; memcpy(tablep,&table,i) ;
	return(tablep) ;
}
#endif

/*	E R R O R   H A N D L E R S			*/

v3_error(errnum,subsys,module,mnemonic,msg,optarg)
  int errnum ;
  char *subsys,*module,*mnemonic,*msg ;
  int optarg ;
{
	v4_error(errnum,NULL,subsys,module,mnemonic,msg,optarg) ;
}

struct V4ERR__Info v4errinfo ;

#ifdef ALTVARGS						/* Need funky header for SCO variable-arg handler */
void v4_error(va_alist)
  va_dcl
{ extern int *v3unitp ;
  struct V4LEX__TknCtrlBlk *tcb ;
  struct V4C__ProcessInfo *pi ;
  int errnum ; char *subsys,*module,*mne,*msg ;
  va_list args ; char *format ;
  char ebuf[250] ; int code ; UCCHAR v4ubuf[2500] ;
  char path[200],tmp[10000], *b ; int i,j,nx ;

/*	Do special formatting on error message */
	va_start(args) ; errnum = va_arg(args,int) ; tcb = va_arg(args,struct V4LEX__TknCtrlBlk *) ;
	subsys = va_arg(args,char *) ; module = va_arg(args,char *) ; mne = va_arg(args,char *) ;
	format = va_arg(args,char *) ; vsprintf(ebuf,format,args) ; va_end(args) ;
#else							/* All other UNIX */
void v4_error(errnum,tcb,subsys,module,mne,msg)
  int errnum ;
  struct V4LEX__TknCtrlBlk *tcb ;
  char *subsys,*module,*mne,*msg ;

{ extern int *v3unitp ;
  va_list ap ;
  struct V4C__ProcessInfo *pi ;
  char ebuf[2500] ; UCCHAR v4ubuf[2500] ;
  UCCHAR path[256] ; UCCHAR tmp[10000] ; int i,j ;

/*	Do special formatting on error message */
//	va_start(ap,mne) ; msg = va_arg(ap,char *) ; vsprintf(ebuf,msg,ap) ; va_end(ap) ;
	va_start(ap,msg) ; vsprintf(ebuf,msg,ap) ; va_end(ap) ;
#endif
	pi = v_GetProcessInfo() ;
//	pi->EscFlag = UNUSED ;				/* Disable/suspend escape-handler thread */
	pi->XMLIndent = UNUSED ;			/* Reset any indentation for XML */
	pi->NestMax = FALSE ;
#define MainTcbx (pi->MainTcb)
/*	Look for primary file & line number for main error message */
	if (tcb == NULL)
	 { ZUS(tmp) ;
	   for(i=(MainTcbx == NULL ? 0 : MainTcbx->ilx);i>0;i--)
	    { if (MainTcbx->ilvl[i].file_name[0] == 0) continue ;		/* No file associated with this level */
	      v_Msg(NULL,path,"@(%1U %2d)",vlex_FullPathNameOfFile(MainTcbx->ilvl[i].file_name,NULL,0),MainTcbx->ilvl[i].current_line) ;
	      if (UCstrlen(tmp) > 0) { UCstrcat(tmp,UClit(",")) ; } ;
	      UCstrcat(tmp,path) ;
	    } ;
	 } else
	 { ZUS(tmp) ;
	   for(i=tcb->ilx;i>0;i--)
	    { if (tcb->ilvl[i].file_name[0] == 0) continue ;		/* No file associated with this level */
	      v_Msg(NULL,path,"@(%1U %2d)",vlex_FullPathNameOfFile(tcb->ilvl[i].file_name,NULL,0),tcb->ilvl[i].current_line) ;
	      if (UCstrlen(tmp) > 0) { UCstrcat(tmp,UClit(",")) ; } ;
	      UCstrcat(tmp,path) ;
	    } ;
	 } ;

	if (pi->ErrEcho)
	 { if (strlen(subsys) == 0)
	    { v_Msg(NULL,v4ubuf,"@*%1U %2U\n",tmp,pi->ctx->ErrorMsg) ; }
	    else { v_Msg(NULL,v4ubuf,"@*%1s %2s.%3s.%4s -  %5s\n",tmp,subsys,module,mne,ebuf) ; } ;
	   vout_UCText(VOUT_Err,0,v4ubuf) ;
	 } ;
/*	Should we point to offending whatever ? */
	if (tcb < (struct V4LEX__TknCtrlBlk *)100) tcb = NULL ;
/*	Only update error count if not a warning or alert */
	if (!(errnum == V4E_Warn || errnum == V4E_Alert)) pi->ErrCount++ ;
	pi->ErrNum = errnum ;
	if (tcb != NULL)
	 {
	   UCstrcpy(tmp,tcb->ilvl[tcb->ilx].input_str) ;
/*	   Set up to make pointer */
	   j = (UCCHAR *)tcb->ilvl[tcb->ilx].input_ptr - (UCCHAR *)(&(tcb->ilvl[tcb->ilx].input_str)) ;
	   for (i=0;i<j-1;i++)
	    { if (tmp[i] > UClit(' ')) tmp[i] = UClit(' ') ; } ;
	   tmp[i] = UClit('^') ; tmp[++i] = UCEOS ;
	   if (pi->ErrEcho)
	    { v_Msg(NULL,v4ubuf,"@  %1U",tcb->ilvl[tcb->ilx].input_str) ; vout_UCText(VOUT_Err,0,v4ubuf) ;
	      if (tcb->ilvl[tcb->ilx].input_str[UCstrlen(tcb->ilvl[tcb->ilx].input_str)-1] > 15) vout_NL(VOUT_Err) ;
	      v_Msg(NULL,v4ubuf,"@  %1U\n",tmp) ; vout_UCText(VOUT_Err,0,v4ubuf) ;
	    } ;
	   if (strlen(subsys) > 0)		/* Only do this for old-style error messages */
	    { switch(tcb->type)
	       { default:				ZUS(v4ubuf) ; break ;
		   case V4LEX_TknType_RegExpStr:
		   case V4LEX_TknType_String:		v_Msg(NULL,v4ubuf,"@  Token-String : %1U\n",tcb->UCstring) ; break ;
		   case V4LEX_TknType_Keyword:		v_Msg(NULL,v4ubuf,"@  Token-Keyword: %1U\n",tcb->UCkeyword) ; break ;
		   case V4LEX_TknType_Punc:		v_Msg(NULL,v4ubuf,"@  Token-Punc: %1U\n",tcb->UCkeyword) ; break ;
		   case V4LEX_TknType_Integer:		v_Msg(NULL,v4ubuf,"@  Token-Integer: %1d(%2d)\n",tcb->integer,tcb->decimal_places) ; break ;
		   case V4LEX_TknType_LInteger:		UCsprintf(v4ubuf,255,UClit("  Token-LInteger: %I64d(%d)\n"),tcb->Linteger,tcb->decimal_places) ; break ;
		   case V4LEX_TknType_Float:		UCsprintf(v4ubuf,255,UClit("  Token-Float  : %G\n"),tcb->floating) ; break ;
		   case V4LEX_TknType_EOL:		UCstrcpy(v4ubuf,UClit("  Token-End-of-Line\n")) ; break ;
	       } ; if (pi->ErrEcho && UCstrlen(v4ubuf) > 0) vout_UCText(VOUT_Err,0,v4ubuf) ;
	    } ;
	 } ;
	if (pi->ctx != NULL && pi->ErrEcho && --pi->Compiling < 0)	/* If got a context & not compiling then try to dump out current state */
	 {  
	   v4trace_ExamineState(pi->ctx,NULL,V4_EXSTATE_All,VOUT_Err) ;
///*	   Try and output Isct stack information */
//	   vout_Text(VOUT_Err,0,"*V4 Call Stack Detail at time of Error-\n") ;
//	   for(i=pi->ctx->NestedIntMod-1;i>=0;i--)
//	    { v_Msg(pi->ctx,NULL,"@*    %1d. %2P\n",i+1,pi->ctx->IsctStack[i].IsctPtr) ; vout_UCText(VOUT_Err,0,pi->ctx->ErrorMsg) ;
//	    } ;
	   v_Msg(pi->ctx,NULL,"@*Last V4Eval command was: %1S\n",pi->LastV4CommandIndex) ; vout_UCText(VOUT_Err,0,pi->ctx->ErrorMsg) ;
	 } ;
/*	If we are running as batch job then abort immediately, otherwise longjmp to GKW */
	if(!v_IsAConsole(stdin))
	 { v4_ExitStats(pi->ctx,(tcb == NULL ? MainTcbx : tcb)) ;
	   v4mm_FreeResources(pi) ; v4mm_MemoryExit() ;
//	   if (gpi->doSummaryAtExit) v4im_DoSummaryOutput(pi->ctx,NULL,FALSE,NULL) ;
	   exit(EXITABORT) ;
	 } ;
//	strncpy(pi->ErrMsg,ebuf,(sizeof pi->ErrMsg)-1) ;
	UCstrcpyAtoU(pi->ErrMsg,ebuf) ;
	longjmp(pi->environment,1) ;
}


/*	Alternate v4_error when error message is UTF-16 (Unicode) (no further formatting) */
void v4_UCerror(errnum,tcb,subsys,module,mne,ucmsg)
  int errnum ;
  struct V4LEX__TknCtrlBlk *tcb ;
  char *subsys,*module,*mne ;
  UCCHAR *ucmsg ;
{ extern int *v3unitp ;
  struct V4C__ProcessInfo *pi ;
  UCCHAR tbuf[UCReadLine_MaxLine] ;
  UCCHAR path[200],tmp[10000] ; int i,j,stream ;

	pi = v_GetProcessInfo() ;
//	pi->EscFlag = UNUSED ;				/* Disable/suspend escape-handler thread */
	pi->XMLIndent = UNUSED ;			/* Reset any indentation for XML */
	pi->NestMax = FALSE ;
	
	switch(errnum)
	 { default:		stream = VOUT_Err ; break ;
	   case V4E_Warn:	stream = VOUT_Warn ; break ;
	   case V4E_Alert:	stream = VOUT_Status ; break ;
	 } ;

#define MainTcbx (pi->MainTcb)
/*	Look for primary file & line number for main error message */
	if (tcb == NULL)
	 { ZUS(tmp) ;
	   for(i=(MainTcbx == NULL ? 0 : MainTcbx->ilx);i>0;i--)
	    { if (MainTcbx->ilvl[i].file_name[0] == 0) continue ;		/* No file associated with this level */
	      v_Msg(NULL,path,"@(%1U %2d)",vlex_FullPathNameOfFile(MainTcbx->ilvl[i].file_name,NULL,0),MainTcbx->ilvl[i].current_line) ;
	      if (UCstrlen(tmp) > 0) { UCstrcat(tmp,UClit(",")) ; } ;
	      UCstrcat(tmp,path) ;
	    } ;
	 } else
	 { ZUS(tmp) ;
	   for(i=tcb->ilx;i>0;i--)
	    { if (tcb->ilvl[i].file_name[0] == 0) continue ;		/* No file associated with this level */
	      v_Msg(NULL,path,"@(%1U %2d)",vlex_FullPathNameOfFile(tcb->ilvl[i].file_name,NULL,0),tcb->ilvl[i].current_line) ;
	      if (UCstrlen(tmp) > 0) { UCstrcat(tmp,UClit(",")) ; } ;
	      UCstrcat(tmp,path) ;
	    } ;
	 } ;

	if (pi->ctx->rtStack[0].isctPtr != NULL) goto skip_tcb ;	/* If V4 runtime then don't output compilation junk */

	if (pi->ErrEcho)
	 { if (strlen(subsys) == 0)
	    { v_Msg(NULL,tbuf,"@*%1U %2U\n",tmp,pi->ctx->ErrorMsg) ; }
	    else { v_Msg(NULL,tbuf,"@*%1U %2s.%3s.%4s -  %5U\n",tmp,subsys,module,mne,ucmsg) ; } ;
	   vout_UCText(stream,0,tbuf) ;
	 } ;
/*	Should we point to offending whatever ? */
	if (tcb < (struct V4LEX__TknCtrlBlk *)100) tcb = NULL ;
/*	Only update error count if not a warning or alert */
	if (stream == VOUT_Err) pi->ErrCount++ ;
	pi->ErrNum = errnum ;
	if (tcb != NULL)
	 { if (UCstrlen(tcb->ilvl[tcb->ilx].input_str) >= UCsizeof(tmp)) { tcb->ilvl[tcb->ilx].input_str[UCsizeof(tmp) - 5] = UCEOS ; } ;
	   UCstrcpy(tmp,tcb->ilvl[tcb->ilx].input_str) ;
/*	   Set up to make pointer */
	   j = (UCCHAR *)tcb->ilvl[tcb->ilx].input_ptr - (UCCHAR *)(&(tcb->ilvl[tcb->ilx].input_str)) ;
	   for (i=0;i<j-1;i++)
	    { if (tmp[i] > UClit(' ')) tmp[i] = UClit(' ') ; } ;
	   tmp[i] = UClit('^') ; tmp[++i] = UCEOS ;
	   if (pi->ErrEcho)
	    { v_Msg(NULL,tbuf,"@  %1U",tcb->ilvl[tcb->ilx].input_str) ; vout_UCText(stream,0,tbuf) ;
	      if (tcb->ilvl[tcb->ilx].input_str[UCstrlen(tcb->ilvl[tcb->ilx].input_str)-1] > 15) vout_NL(stream) ;
	      v_Msg(NULL,tbuf,"@  %1U\n",tmp) ; vout_UCText(stream,0,tbuf) ;
	    } ;
	   if (strlen(subsys) > 0)		/* Only do this for old-style error messages */
	    { switch(tcb->type)
	       { default:				ZUS(tbuf) ; break ;
		   case V4LEX_TknType_RegExpStr:
		   case V4LEX_TknType_String:		v_Msg(NULL,tbuf,"@  Token-String : %1U\n",tcb->UCstring) ; break ;
		   case V4LEX_TknType_Keyword:		v_Msg(NULL,tbuf,"@  Token-Keyword: %1U\n",tcb->UCkeyword) ; break ;
		   case V4LEX_TknType_Punc:		v_Msg(NULL,tbuf,"@  Token-Punc: %1U\n",tcb->UCkeyword) ; break ;
		   case V4LEX_TknType_Integer:		UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Token-Integer: %d(%d)\n"),tcb->integer,tcb->decimal_places) ; break ;
		   case V4LEX_TknType_LInteger:		UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Token-LInteger: %I64d(%d)\n"),tcb->Linteger,tcb->decimal_places) ; break ;
		   case V4LEX_TknType_Float:		UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Token-Float  : %G\n"),tcb->floating) ; break ;
		   case V4LEX_TknType_EOL:		UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Token-End-of-Line\n")) ; break ;
	       } ; if (pi->ErrEcho && UCstrlen(tbuf) > 0) vout_UCText(stream,0,tbuf) ;
	    } ;
	 } ;

skip_tcb:
	if (pi->ctx != NULL && pi->ErrEcho && --pi->Compiling < 0)	/* If got a context & not compiling then try to dump out current state */
	 {  
	   v4trace_ExamineState(pi->ctx,NULL,V4_EXSTATE_All,VOUT_Err) ;
/*	   Try and output Isct stack information */
//	   vout_UCText(stream,0,UClit("*V4 Call Stack Detail at time of Error-\n")) ;
//	   for(i=pi->ctx->NestedIntMod-1;i>=0;i--)
//	    { v_Msg(pi->ctx,NULL,"@*    %1d. %2P\n",i+1,pi->ctx->IsctStack[i].IsctPtr) ; vout_UCText(stream,0,pi->ctx->ErrorMsg) ;
//	    } ;
	   v_Msg(pi->ctx,NULL,"@*Last V4Eval command was: %1S\n",pi->LastV4CommandIndex) ; vout_UCText(stream,0,pi->ctx->ErrorMsg) ;
	 } ;
/*	If we are running as batch job then abort immediately, otherwise longjmp to GKW */
	if(!v_IsAConsole(stdin))
	 { v4_ExitStats(pi->ctx,(tcb == NULL ? MainTcbx : tcb)) ;
	   v4mm_FreeResources(pi) ; v4mm_MemoryExit() ;
//	   if (gpi->doSummaryAtExit) v4im_DoSummaryOutput(pi->ctx,NULL,FALSE,NULL) ;
	   exit(EXITABORT) ;
	 } ;
//	strncpy(pi->ErrMsg,ebuf,(sizeof pi->ErrMsg)-1) ;
	UCstrcpy(pi->ErrMsg,ucmsg) ;
	longjmp(pi->environment,1) ;
}

#ifdef WINNT
  FILETIME ftKernel2,ftUser2 ;
#endif

void v4_ExitStats(ctx,tcb)
  struct V4C__Context *ctx ;
  struct V4LEX__TknCtrlBlk *tcb ;
{
  time_t time_of_day ; double wallseconds ;
  double cpusecondsU,cpusecondsK ;
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
	UCsprintf(tbuf,UCsizeof(tbuf),UClit("  Errs: %d\n"),(ctx->pi->ErrCount)) ; vout_UCText(VOUT_Status,0,tbuf) ;

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
}

//void _cputws(ustr)
//  UCCHAR *ustr ;
//{
//	_cputs(UCretASC(ustr)) ;
//}

v3_v4_EvalV3Mod()
{
	v4_error(V4E_V3MODUNS,0,"V4TEST","EvalV3MOD","V3MODUNS","V3Mod/V3ModRaw point types not supported outside of V3") ;
}

v3v4_v4im_Load_V3PicMod()
{
	v4_error(V4E_NOV3LOADUNS,0,"V4TEST","V3PicMod","NOV3LOADUNS","Load of V3 module not supported outside of V3") ;
}

v3v4_v4im_Unload_V3PicMod()
{
	v4_error(V4E_NOV3UNLOADUNS,0,"V4TEST","V3PicMod","NOV3UNLOADUNS","UnLoad of V3 module not supported outside of V3") ;
}

/*	D I S P L A Y   W I N D O W   M O D U L E S	*/

/*	v4gui_GetDisplaySetupInfo - Returns info on display window */
/*	Call: ok = v4gui_GetDisplaySetupInfo( displaypt , *displaysetup )
	  where ok is TRUE if displaysetup updated,
	  	displaypt is point denoting display,
	  	displaysetup is pointer to structure to be updated			*/

int v4gui_GetDisplaySetupInfo(displaypt,displaysetup)
  int displaypt ;
  struct V4GUI__DisplaySetup *displaysetup ;
{
	return(0) ;
}

/*	v4gui_EnumDisplayFieldPoints - Returns (one at a time) field points for a display window	*/
/*	Call: fieldpoint = v4gui_EnumDisplayFieldPoints( displaypt , index )
	  where fieldpoint is (int)fieldpt (0 if end of list),
	  	displaypt is point corresponding to display window,
	  	index is 0..n-1 for a window with n fields			*/

int v4gui_EnumDisplayFieldPoints(displaypt,index)
  int displaypt,index ;
{
	return(0) ;
}

/*	v4gui_GetDisplayPromptInfo - Returns info on prompt for a Display field	*/
/*	Call: ok = v4gui_GetDisplayPromptInfo( displaypt , fieldpt , widthpt , *promptinfo )
	  where ok is TRUE if promptinfo updated,
	  	displaypt is point denoting display,
	  	fieldpt is point denoting the field,
	  	widthpt is point denoting current operating width,
	  	promptinfo is pointer to structure to be updated with prompt info	*/

int v4gui_GetDisplayPromptInfo(displaypt,fieldpt,widthpt,promptinfo)
  int displaypt,fieldpt,widthpt ;
  struct V4GUI__PromptInfo *promptinfo ;
{
	return(0) ;
}

/*	v4gui_GetDisplayFieldSetupInfo - Returns info on field (edit control) for a display window */
/*	Call: ok = v4gui_GetDisplayFieldSetupInfo( displaypt , fieldpt , *fieldsetup )
	  where ok is TRUE if fieldsetup updated,
	  	displaypt is point denoting display,
	  	fieldpt is point denoting the field,
	  	fieldsetup is pointer to structure to be updated			*/

int v4gui_GetDisplayFieldSetupInfo(displaypt,fieldpt,fieldsetup)
  int displaypt,fieldpt ;
  struct V4GUI__FieldSetup *fieldsetup ;
{
	return(0) ;
}

/*	v4gui_ChangeDisplayPromptWidth - Attempts to change width of prompts for a display window */
/*	Call: newwidth = v4gui_ChangeDisplayPromptWidth( displaypt , currentwidth , change )
	  where newwidth is new width for prompts (same as currentwidth if no change possible),
	  	dislaypt is point denoting display,
	  	currentwidth is current width (call with 0 for initial setting),
	  	change is -1 to reduce width, +1 to increase width				*/

int v4gui_ChangeDisplayPromptWidth(displaypt,currentwidth,change)
  int displaypt,currentwidth,change ;
{
	return(0) ;
}

/*	v4gui_GetDisplayFieldDataText - Gets data for a display field as displayable text	*/
/*	Call: texttype = v4gui_GetDisplayFieldDataText( displaypt , fieldpt , datahandle , *datatext )
	  where texttype is type of text data returned (V4GUI_TextType_xxx),
	  	displaypt is pointing denoting display,
	  	fieldpt is point denoting field,
	  	datahandle is handle to record buffer,
	  	datatext is pointer to structure to be updated					*/

int v4gui_GetDisplayFieldDataText(displaypt,fieldpt,datahandle,datatext)
  int displaypt,fieldpt ;
  int datahandle ;
  struct V4GUI__DataText *datatext ;
{
	return(0) ;
}

/*	v4gui_ConvertUOM - Converts Unit-of-Measure value to another UOM	*/
/*	Call: newvalue = v4gui_ConvertUOM( uomvalue , newuom )
	  where newvalue is uomvalue converted to newuom,
	  	uomvalue is starting value,
	  	newuom is new unit of measure type				*/

double v4gui_ConvertUOM(uomvalue,newuom)
  UOMV uomvalue ;
  int newuom ;
{
	return(0.0) ;
} ;

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
