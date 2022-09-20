/*	VCOMMON.C - Common utilities for V3 & V4			*/

#include <time.h>
#include <stdlib.h>

/*	These are copied from v3defs.c - Including v3defs.c causes problems when compiling on Linux ?? */
/*  #include "v3defs.c"  */
#define V3E_EOF 100580			/* End of file reached */
#define V3E_NOLOGICAL 101550		/* npath */
#define V3E_NONESTEDLOGICAL 101650		/* ffn */

#define NEED_SOCKET_LIBS 1

#include "v4imdefs.c"
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef WINNT
#include <windows.h>
#include <conio.h>
#include <process.h>
#include <io.h>
#endif
#ifdef UNIX
#include <signal.h>
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>
 #if defined LINUX486 || defined RASPPI
  #include <sys/stat.h>
 #else
  #include <sys/mode.h>
 #endif
#include <sys/fcntl.h>
#endif

GLOBALDIMSEXTERN
V4DEBUG_SETUP
struct V__UnicodeInfo *uci = NULL ;	/* Global structure of Unicode Info */
struct V4C__ProcessInfo *gpi = NULL ;	/* Global process information */
struct v__ColorValues *vcv = NULL ;	/* Global color information */
int v4NutCracker = 0 ;			/* Debugging progress indicator for tough nuts to crack */
char v4NCBuf[512] ;			/* String buffer for more info */
extern ETYPE traceGlobal ;

/*	D A T A   C O M P R E S S I O N   R O U T I N E S		*/

static union {
  unsigned char all ;
  struct {
     unsigned type : 2 ;		/* Type of header byte */
     unsigned count : 6 ;		/* Number of bytes following */
   } fld ;
 } control ;

#define DC_TYPE_DATA 0			/* Data follows */
#define DC_TYPE_DATALONG 1		/* Data follows (add 63 to count) */
#define DC_TYPE_SPACES 2		/* Spaces follow */
#define DC_TYPE_NULLS 3			/* Nulls follow */

/*	data_compress - Compresses data into work buffer, returns bytes */

int data_compress(voptr,viptr,ilen,header_length,omax)
  void *voptr ;			/* Pointer to work buffer */
  void *viptr ;			/* Pointer to buf to be compressed */
  int ilen ;			/* Number of bytes in input buffer */
  int header_length ;		/* Number of bytes in header (don't compress) */
  int omax ;			/* Max number of bytes in output work buffer */
{ int i ;
  unsigned char *last_ptr,*end_ptr,*ptr ;
  unsigned char *initial_optr = voptr ;
  unsigned char *optr,*iptr ;
  
	optr = (unsigned char *)voptr ; iptr = (unsigned char *)viptr ;
	last_ptr = iptr + ilen - 1 ;	/* To last byte */
/*	First do the header */
	for(i = header_length;i>0;i--) { *optr++ = *iptr++ ; } ;
/*	Now scan remainder of buffer & compress */
	for(;iptr <= last_ptr;)
	 { switch (*iptr)
	    { case 0:	/* Check for string of null bytes */
		end_ptr = iptr + 62 ;		/* Last byte to look at */
		if (end_ptr > last_ptr) end_ptr = last_ptr ;
		for(ptr=iptr+1;ptr<=end_ptr && *ptr == 0;ptr++) ;
/*		Did we get enough to make worthwhile ? */
		if (ptr-iptr <= 2) goto got_data ;
		control.fld.type = DC_TYPE_NULLS ; control.fld.count = ptr-iptr ;
		*(optr++) = control.all ; iptr = ptr ;
		break ;
	      case ' ':	/* Check for string of spaces */
		end_ptr = iptr + 62 ;		/* Last byte to look at */
		if (end_ptr > last_ptr) end_ptr = last_ptr ;
		for(ptr=iptr+1;ptr<=end_ptr && *ptr == ' ';ptr++) ;
/*		Did we get enough to make worthwhile ? */
		if (ptr-iptr <= 2) goto got_data ;
		control.fld.type = DC_TYPE_SPACES ; control.fld.count = ptr-iptr ;
		*(optr++) = control.all ; iptr = ptr ;
		break ;
	      default:	/* All other characters */
got_data:	end_ptr = iptr + 123 ; if ( end_ptr >= last_ptr-2) end_ptr = last_ptr-2 ;
		for(ptr=iptr+1;ptr<=end_ptr;ptr++)
		 { if(*(ptr+1) == 0 && *(ptr+2) == 0) break ;
		   if(*(ptr+1) == ' ' && *(ptr+2) == ' ') break ;
		 } ;
		if (ptr >= last_ptr-2) ptr = last_ptr ;
/*		Append these bytes */
		i = ptr - iptr + 1 ;
		if (i > 63)
		 { control.fld.type = DC_TYPE_DATALONG ; control.fld.count = i-64 ; }
		 else { control.fld.type = DC_TYPE_DATA ; control.fld.count = i ; } ;
		*optr++ = control.all ;
		for(;i>0;i--) { *optr++ = *iptr++ ; } ;
	     } ;
	 } ;
/*	All done - return length */
	*optr++ = header_length ;	/* Make sure ends with header length */
	i = optr - initial_optr ;
	if (i > omax) v4_error(0,0,"VCOM","COMPRESS","BUFOVER","Compression buffer (%d) overrun to (%d) - BAD NEWS",omax,i) ;
	return(i) ;
}

/*	data_expand - Expands a compress buffer, returns final length */

int data_expand(voptr,viptr,ilen)
  void *voptr ;			/* Pointer to output buffer */
  void *viptr ;			/* Pointer to compressed input */
  int ilen ;			/* Length of compressed input buffer */
{ unsigned char *initial_optr = voptr ;
   unsigned char *last_ptr ;
   int i,count ;
  unsigned char *optr,*iptr ;
  
	optr = (unsigned char *)voptr ; iptr = (unsigned char *)viptr ;
/*	First do the header */
	last_ptr = iptr + ilen - 1 ;	/* Pointer to last byte in buffer */
	for(i= *last_ptr;i>0;i--) { *(optr++) = *(iptr++) ; } ;
/*	Now expand out */
	for(;iptr<last_ptr;)
	 { control.all = *iptr++ ;
	   switch (control.fld.type)
	    { case DC_TYPE_DATA:
		for(count = control.fld.count;count>0;count--)
		 { *optr++ = *iptr++ ; } ;
		break ;
	      case DC_TYPE_DATALONG:
		for(count = control.fld.count+64;count>0;count--)
		 { *optr++ = *iptr++ ; } ;
		break ;
	      case DC_TYPE_SPACES:
		for(count = control.fld.count;count>0;count--)
		 { *optr++ = ' ' ; } ;
		break ;
	      case DC_TYPE_NULLS:
		for(count = control.fld.count;count>0;count--)
		 { *optr++ = 0 ; } ;
		break ;
	     } ;
	 } ;
/*	All done - return new length */
	return(optr - initial_optr) ;
}

/*	T O K E N   G E N E R A T O R			*/

//#ifndef V4LEX_TknType_EOL
//#define NULL 0
//#endif

int opcodeDE[] =
   { DE(Ampersand),DE(Colon),DE(ColonColon),DE(ColonEqual),DE(Comma),DE(DashRangle),DE(Dot),DE(DotDot),DE(NCDot),DE(Equal),DE(EqualEqual),DE(LBrace),DE(LBraceSlash),
     DE(LBracket),DE(LParen),DE(LRBracket),DE(Langle),DE(LangleDash),DE(LangleEqual),DE(LangleLangle),DE(LangleRangle),DE(Line),DE(Minus),DE(MinusEqual),
     DE(Not),DE(NotEqual),DE(Plus),DE(PlusEqual),DE(Pound),DE(Question),DE(RBrace),DE(RBraceCross),DE(RBraceSlash),DE(RBracket),DE(RParen),DE(Rangle),
     DE(RangleEqual),DE(RangleRangle),DE(Semi),DE(Slash),DE(SlashEqual),DE(SlashSlash),DE(SlashStar),DE(Star),DE(NCStar),DE(Tilde),DE(AtSign),DE(UMinus),
     DE(Percent),DE(BackQuote),DE(BackSlash),DE(Possessive),DE(StarEqual),DE(StarStar),DE(NCLParen),DE(TildeEqual),DE(NCColon),DE(NCLBrace),DE(LineLine),
     DE(DotDotDot),DE(EqualRangle),DE(LangleSlash),DE(SlashRangle),DE(LangleQuestion),DE(QuestionRangle),DE(LangleBang),
     DE(LangleBangDashDash),DE(DashDashRangle),DE(NCLBracket),DE(ColonColonColon),DE(Space),DE(CommaComma),DE(DollarSign),DE(DollarLParen),DE(DolDolLParen)
   } ;



#define END_OF_LINE 0			/* End of line */
#define KEYWORD 1			/* A predefined/user symbol */
#define STRING_LIT 2			/* A string literal */
#define NUMERIC_LIT 3			/* A numeric literal */
#define RADIX_OVERRIDE 4		/* Override for numeric radix */

/*	Character sets for numeric literals		*/
#define INVALID_NUMERIC 0		/* For invalid characters */
#define END_OF_NUMBER 1
#define DIGIT 2
#define HEX_DIGIT 3
#define EXPONENT 4
#define DECIMAL_POINT 5
#define IGNOREIT 6

UCCHAR *v4lex_TknPtr(tcb,optype,optptr,ok)
  struct V4LEX__TknCtrlBlk *tcb ;	/* Token control block */
  int optype ;				/* Type of operation - V4LEX_TknOp_xxx */
  UCCHAR *optptr ;			/* Optional pointer */
  int *ok ;				/* Set to TRUE if all went well */
{ static UCCHAR tbuf[1024] ;
  UCCHAR *b1,*b2 ; int i ;
	switch (optype)
	 { default:			*ok = FALSE ; return(NULL) ;
	   case V4LEX_TknOp_CurrentPtr:
		*ok = TRUE ; return(tcb->have_lookahead ? tcb->prior_input_ptr : tcb->ilvl[tcb->ilx].input_ptr) ;
	   case V4LEX_TknOp_ThruCurrent:
		if (optptr == NULL) { *ok = FALSE ; return(NULL) ; } ;
		for(b1=optptr,b2=tbuf,i=0;*b1 != UCEOS && i<UCsizeof(tbuf) && b1 != tcb->ilvl[tcb->ilx].input_ptr;i++) { *(b2++) = (*b1++) ; } ;
		if (i >= UCsizeof(tbuf)) { *ok = FALSE ; return(NULL) ; } ;
		*b2 = UCEOS ; *ok = TRUE ; return(tbuf) ;
	 } ;
}

/*	type_of_token - 256 byte array describing each ASCII character for start of token */
char type_of_token[256] ;
char type_of_token_keyword[256] ;
UCCHAR *v4lex_NextTkn(tcb,tg_flags)
  struct V4LEX__TknCtrlBlk *tcb ;	/* Token control block */
  int tg_flags ;			/* Generator flags - If TRUE then set "lookahead" flag for next call */
{
   static unsigned char keyword_translation[256],numeric_array[256],need_type_setup=TRUE ;
   UCCHAR dps,have_dp,sc,has_minus,have_num ; UCCHAR *iptr,*optr ;
   UCCHAR *tkn_sym,byte,delimiter,*str_ptr,*uom,*uc ;
   static UCCHAR *rescan_ptr ; static int multi_line ;
   static UCCHAR mlDelimiter = UCEOS ;
   int i,j,have_exp,res,quotenext,b,uom_status ; int *ip ;
   double fres ; B64INT lres,lresa ;

/*	If first call then have to init type_of_token array	*/
	if (need_type_setup)
	 { need_type_setup = FALSE ;
	   for (i=0;i < 256;i++) type_of_token[i] = i ;
	   type_of_token[0] = (type_of_token[10] = (type_of_token[12] = (type_of_token[13] = END_OF_LINE))) ;
	   v4lex_TknArray_Setup(type_of_token,"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_",KEYWORD) ;
	   v4lex_TknArray_Setup(type_of_token,"\042'",STRING_LIT) ;
	   v4lex_TknArray_Setup(type_of_token,"0123456789",NUMERIC_LIT) ;
	   v4lex_TknArray_Setup(type_of_token,"^",RADIX_OVERRIDE) ;
/*	   Set up special array to force numbers into keywords */
	   memcpy(type_of_token_keyword,type_of_token,sizeof type_of_token) ;
	   v4lex_TknArray_Setup(type_of_token_keyword,"0123456789",KEYWORD) ;
/*	   Set up table for keyword */
	   for (i='A';i<='Z';i++)
	    { keyword_translation[i] = i ; keyword_translation[i-'A'+'a'] = i ; } ;
	   for (i='0';i<='9';i++)
	    { keyword_translation[i] = i ; } ;
	   keyword_translation['_'] = '_' ;
/*	   Set up for numeric literals */
	   v4lex_TknArray_Setup(numeric_array,"!|@#$%^&*)-_+=`~{]}\\;:,<>/?",END_OF_NUMBER) ;
	   for (i=0;i<=' ';i++) numeric_array[i] = END_OF_NUMBER ;
	   v4lex_TknArray_Setup(numeric_array,"0123456789",DIGIT) ;
	   v4lex_TknArray_Setup(numeric_array,"ABCDEFabcdef",HEX_DIGIT) ;
	   numeric_array['.'] = DECIMAL_POINT ; numeric_array['_'] = IGNOREIT ;
	 } ;
	if (tg_flags & V4LEX_Option_Excel)	/* Allow +-[]$ in keyword */
	 { keyword_translation['-'] = '-' ; keyword_translation['+'] = '+' ; keyword_translation['['] = '[' ;
	   keyword_translation[']'] = ']' ; keyword_translation['$'] = '$' ; keyword_translation[':'] = ':' ;
	   type_of_token['$'] = KEYWORD ;
	   need_type_setup = TRUE ;		/* Want to force reset next time thru */
	 } ;

/*	See if setting have_lookahead flag or if we got it */
	if (tg_flags & V4LEX_Option_PushCurHard)
	 { tcb->ilvl[tcb->ilx].input_ptr = tcb->prior_input_ptr ; return(rescan_ptr) ; } ;
	if (tg_flags & V4LEX_Option_PushCur)
	 { tcb->have_lookahead = TRUE ; return(rescan_ptr) ; } ;
/*	Reset look-ahead flag (if set to EOL then immediately return EOL (again)) */
	if (tcb->have_lookahead)
	 { if (tcb->lookahead_flags == tg_flags || multi_line)		/* If flags match then just return tcb "as is" */
	    { tcb->have_lookahead = 0 ; return(rescan_ptr) ; } ;
	   tcb->have_lookahead = 0 ; tcb->ilvl[tcb->ilx].input_ptr = tcb->prior_input_ptr ;
	 } ;
	tcb->lookahead_flags = tg_flags ;		/* Save for possible fast-lookahead processing */
	tcb->opcode = 0 ; tcb->prec = 0 ;		/* Reset these */
	multi_line = FALSE ;

top_of_module:
/*	If we need another input line then grab it	*/
	if (tcb->need_input_line)
	 { if (!v4lex_ReadNextLine(tcb,tcb->ilvl[tcb->ilx].tpo,NULL,NULL))
	    { if (tcb->type == V4LEX_TknType_Error) return(NULL) ;
	      tcb->opcode = V_OpCode_EOF ; tcb->type = V4LEX_TknType_Punc ; return(rescan_ptr) ;
	    } ;
	 } ;
	if (tcb->ilvl[tcb->ilx].input_ptr == NULL)
	 { v_Msg(NULL,tcb->UCstring,"TknUnExpEnd") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
/*	If we are continuing a multi-line string literal then set up some stuff & jump into string literal parsing */
	if ((tg_flags & V4LEX_Option_MLStringLit) != 0 && mlDelimiter != UCEOS)
	 { delimiter = mlDelimiter ;
	   tcb->opcode = (mlDelimiter == UClit('"') ? V_OpCode_DQString : V_OpCode_QString) ;
	   tcb->literal_len = 0 ; str_ptr = tcb->UCstring ;
	   goto mlstring_entry ;				/* Continuing with multi-line string literal */
	 } ;
/*	Check to see if we are in a comment */
	if (tcb->in_comment)
	 { if ((tcb->ilvl[tcb->ilx].input_ptr = UCstrchr(tcb->ilvl[tcb->ilx].input_ptr,'*')) == NULL)
	    { tcb->need_input_line = TRUE ; goto top_of_module ; } ;
/*	   Found a '*', see if last or next is a '/' */
	   if (*(tcb->ilvl[tcb->ilx].input_ptr - 1) == UClit('/'))
	    { v_Msg(NULL,tcb->UCstring,"TknNestCom") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
	   if (*(tcb->ilvl[tcb->ilx].input_ptr+1) != UClit('/')) { tcb->ilvl[tcb->ilx].input_ptr++ ; goto top_of_module ; } ;
/*	   End of comment - kick out of this little test */
	   tcb->ilvl[tcb->ilx].input_ptr += 2 ; tcb->in_comment = FALSE ;
	 } ;

/*	Return next character in input buffer ? */
	if (tg_flags & V4LEX_Option_NextChar)
	 { tcb->UCkeyword[0] = *tcb->ilvl[tcb->ilx].input_ptr ; tcb->UCkeyword[1] = UCEOS ;
	   rescan_ptr = tcb->ilvl[tcb->ilx].input_ptr ;
	   return(rescan_ptr) ;
	 } ;

/*	Set up some fields in token value format */
/*	Skip over tabs, spaces, whatever	*/
	tcb->prior_input_ptr = tcb->ilvl[tcb->ilx].input_ptr ;
	if(vuc_IsWSpace(*tcb->ilvl[tcb->ilx].input_ptr))
	 { for(tcb->ilvl[tcb->ilx].input_ptr++;vuc_IsWSpace(*tcb->ilvl[tcb->ilx].input_ptr);tcb->ilvl[tcb->ilx].input_ptr++) { } ;
	   if(tg_flags & V4LEX_Option_Space)					/* Treat white-space as a token ? */
	    { tcb->opcode = V_OpCode_Space ; tcb->type = V4LEX_TknType_Punc ;
	      return(rescan_ptr) ;
	    } ;
	 } ;
//	tcb->ilvl[tcb->ilx].input_ptr += UCstrspn(tcb->ilvl[tcb->ilx].input_ptr,UClit(" 	")) ;
	rescan_ptr = tcb->ilvl[tcb->ilx].input_ptr ;	/* Save pointer to begin of token */
	tcb->tcb_tokens ++ ;		/* Track total number of tokens */
	type_of_token['-'] = (tg_flags & V4LEX_Option_NegLit ? NUMERIC_LIT : '-') ;
	type_of_token['.'] = (((tg_flags & V4LEX_Option_NegLit) != 0) ? NUMERIC_LIT : '.') ;
	tcb->radix = tcb->default_radix ;

/*	Return next XML literal value ? */
	if (tg_flags & (V4LEX_Option_XMLLitVal | V4LEX_Option_XMLCDATA))
	 { UCCHAR ebuf[20],*uc1 ; LOGICAL atBOL = FALSE ;
	   uc = tcb->ilvl[tcb->ilx].input_ptr ;
	   for(b=0;b<V4LEX_Tkn_StringMax;b++,uc++)
	    { if (atBOL && vuc_IsWSpace(*uc)) continue ;
	      atBOL = FALSE ;
	      switch (*uc)
	       { default:		tcb->UCstring[b] = *uc ; break ;
	         case UCEOS:	
	         case UCNL:
		   if (!v4lex_ReadNextLine(tcb,tcb->ilvl[tcb->ilx].tpo,NULL,NULL))
		    { if (b > 0) { tcb->UCstring[b] = UCEOS ; tcb->type = V4LEX_TknType_String ; tcb->opcode = V_OpCode_XMLLitPartial ; }
		       else { tcb->opcode = V_OpCode_EOF ; tcb->type = V4LEX_TknType_Punc ; } ;
		      tcb->literal_len = b ; return(rescan_ptr) ;
		    } ;
		   atBOL = TRUE ; continue ;
	         case UClit(']'):	if ((tg_flags & V4LEX_Option_XMLCDATA) == 0) { tcb->UCstring[b] = *uc ; break ; } ;
					if (!(uc[1] == UClit(']') && uc[2] == UClit('>'))) { tcb->UCstring[b] = *uc ; break ; } ;
					tcb->UCstring[b] = UCEOS ; tcb->type = V4LEX_TknType_String ; tcb->opcode = V_OpCode_XMLLit ;
					tcb->literal_len = b ; tcb->ilvl[tcb->ilx].input_ptr = uc+3 ; return(rescan_ptr) ;
					
	         case UClit('<'):	if ((tg_flags & V4LEX_Option_XMLLitVal) == 0) { tcb->UCstring[b] = *uc ; break ; } ;
					tcb->UCstring[b] = UCEOS ; tcb->type = V4LEX_TknType_String ; tcb->opcode = V_OpCode_XMLLit ;
					tcb->literal_len = b ; tcb->ilvl[tcb->ilx].input_ptr = uc ; return(rescan_ptr) ;
	         case UClit('&'):
/*		   Don't decode Named-Entity if within CDATA, only if parsing regular XML */
		   if (tg_flags & V4LEX_Option_XMLCDATA) { tcb->UCstring[b] = *uc ; break ; } ;
		   if (*(uc+1) == UClit('#'))
		    { int cv = 0 ; uc += 2 ;
		      for (;vuc_IsDigit(*uc);uc++)
		       { cv *= 10 ; cv += uci->Entry[*uc].OtherValue ; } ;
		      tcb->UCstring[b] = cv ;
		    } else
		    { ebuf[j=0] = *uc ;
		      for (uc1=uc,j=1,uc++;j<UCsizeof(ebuf) && vuc_IsAlpha(*uc);) { ebuf[j++] = UCTOLOWER(*uc) ; uc++ ; } ; ebuf[j] = UCEOS ;
		      tcb->UCstring[b] = v4xml_LookupISONamedEntity(ebuf) ;
		      if (tcb->UCstring[b] == UCEOS) { uc = uc1 ; tcb->UCstring[b] = *uc ; } ;
		    } ;

//		   if (UCstrcmp(ebuf,UClit("LT")) == 0) { tcb->UCstring[b] = UClit('<') ; }
//		    else if (UCstrcmp(ebuf,UClit("GT")) == 0) { tcb->UCstring[b] = UClit('>') ; }
//		    else if (UCstrcmp(ebuf,UClit("AMP")) == 0) { tcb->UCstring[b] = UClit('&') ; }
//		    else if (UCstrcmp(ebuf,UClit("QUOT")) == 0) { tcb->UCstring[b] = UClit('\'') ; }
//		    else { tcb->UCstring[b] = UClit('?') ; } ;
	       } ;
	    } ;
	 } ;

///*	Skip over any white-characters */
//	if (tg_flags & V4LEX_Option_NextChar)
//	 { 
//	   for(iptr=tcb->ilvl[tcb->ilx].input_ptr;;iptr++)
//	    { if (*iptr == UClit(' ') || *iptr == UClit('\t')) continue ;
//	      tcb->UCkeyword[0] = *iptr ; tcb->UCkeyword[1] = UCEOS ;
//	      return(rescan_ptr) ;
//	    } ;
//	 } ;

/*	Return next chunk (i.e. up to white-space, comma, EOL, etc.) */
	if (tg_flags & (V4LEX_Option_NextChunk | V4LEX_Option_NextChunkExt))
	 { iptr=tcb->ilvl[tcb->ilx].input_ptr ;
	   if (*iptr == UClit('"') || *iptr == UClit('\''))		/* Got quotes - scan up to ending quote */
	    { UCCHAR end = *iptr ; iptr++ ;
	      for(optr=tcb->UCstring;;iptr++)
	       { if (*iptr == end) break ;
	         if (*iptr == UCEOS || *iptr == UCNL)
		  { v_Msg(NULL,tcb->UCstring,"TknEOLinStr") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
		 *(optr++) = *iptr ;
	       } ; iptr++ ;	/* Skip over ending quote */
	    } else if (*iptr == UClit('#'))			/* Got # - assume to be followed by integer "ref" number */
	    { optr = tcb->UCstring ; *(optr++) = *(iptr++) ;
	      for(;;iptr++)
	       { if ((*iptr >= UClit('0') && *iptr <= UClit('9')) || *iptr == UClit('-')) { *(optr++) = *iptr ; continue ; } else { break ; } ;
	       } ;
	    } else
	    { LOGICAL lpCnt=0 ;
	      for(optr=tcb->UCstring,res=TRUE;res;iptr++)
	       { 
//	         if ((*iptr >= UClit('A') && *iptr <= UClit('Z')) || (*iptr >= UClit('a') && *iptr <= UClit('z')) || (*iptr >= UClit('0') && *iptr <= UClit('9')))
	         if (vuc_IsContId(*iptr))
	          { *(optr++) = *iptr ; continue ; } ;
		 switch(*iptr)
		  { default:		if (*iptr == UClit(')') && lpCnt > 0) { lpCnt-- ; *(optr++) = *iptr ; continue ; } ;
					if (*iptr > 127) { *(optr++) = *iptr ; continue ; } ;	/* If not ASCII then assume part of chunk */
					iptr -- ; res = FALSE ; break ;
		    case UClit('.'):	if ((tg_flags & V4LEX_Option_NextChunkExt) == 0)	/* If NextChunk then don't include '..' if ChunkExt then keep as part of chunk */
					 { if (*(iptr+1) == UClit('.')) { iptr -- ; res = FALSE ; break ; } ; } ;
					*(optr++) = *iptr ; continue ;
		    case UClit('-'):
		    case UClit('+'):
		    case UClit('/'):
		    case UClit(':'):
		    case UClit('#'):
		    case UClit('_'):
		    case UClit('$'):	*(optr++) = *iptr ; continue ;
		    case UClit('('):	lpCnt++ ; *(optr++) = *iptr ; continue ;
		  } ;
	       } ;
	    } ;
	   tcb->ilvl[tcb->ilx].input_ptr = iptr ;
	   *optr = UCEOS ;
	   tcb->keyword_len = UCstrlen(tcb->UCstring) ; tcb->type = V4LEX_TknType_String ;
//  UCstrcpyToASC(tcb->string,tcb->UCstring) ;
	   return(rescan_ptr) ;
	 } ;
/*	Are we parsing a file name (then do it unless we appear to have string literal) */
//VEH040409 - Don't want to skip this if filespec enclosed in quotes
//	if ((tg_flags & V4LEX_Option_FileSpec) != 0 ? type_of_token[*tcb->ilvl[tcb->ilx].input_ptr] != STRING_LIT : FALSE)
	if ((tg_flags & V4LEX_Option_FileSpec) != 0)
	 { b = FALSE ;				/* If we have a "[" */
	   j=FALSE ;				/* If we have a quote */
	   iptr=tcb->ilvl[tcb->ilx].input_ptr ;
	   if (*iptr == UClit('"')) { iptr++ ; j = TRUE ; } ;	/* If starts with qoute then ignore it */
	   for(optr = tcb->UCstring,res=TRUE;res;iptr++)
	    { switch(*iptr)
	       { default:
		   *optr++ = *iptr ; break ;
	         case UClit('#'):					/* Expand out parameter? */
		   if (*(iptr+2) != UClit('#')) { *optr++ = *iptr ; break ; } ;
		   iptr++ ;
		   if (*iptr >= UClit('A') && *iptr <= UClit('Z')) { i = V_OpCode_MacArgA + *iptr - UClit('A') ; }
		    else if (*iptr >= UClit('a') && *iptr <= UClit('z')) { i = V_OpCode_MacArgA + *iptr - UClit('a') ; }
		    else { iptr-- ; *optr++ = *iptr ; break ; } ;
		   i -= V_OpCode_MacArgA ; iptr++ ;
		   if (tcb->poundparam[i].numvalue == V4LEX_Tkn_PPNumUndef && tcb->poundparam[i].value[0] == 0)
		    { v_Msg(NULL,tcb->UCstring,"TknParamUndef",i) ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
		   if (tcb->poundparam[i].numvalue != V4LEX_Tkn_PPNumUndef)
		    { UCsprintf(tcb->poundparam[i].value,15,UClit("%d"),tcb->poundparam[i].numvalue) ; } ;
		   *optr = UCEOS ; UCstrcat(optr,tcb->poundparam[i].value) ;
		   optr = optr + UCstrlen(optr) ;
		   break ;
		 case UClit('"'):
		   j = !j ; break ;		/* Track quoted strings, drop thru to include in filespec */
		 case UClit('['):
		   b = TRUE ; *optr++ = *iptr ; break ;
		 case UClit(']'):
		   if (!b) { iptr-- ; res = FALSE ; break ; } ;
		   b = FALSE ; *optr++ = *iptr ; break ;
	         case UClit(','):
		   if (j || b) { *optr++ = *iptr ; break ; }
		 case UClit(' '): case '\t': case UClit('('): case UClit(')'):
		   if (j)			/* Quit unless in string */
		    { *optr++ = *iptr ; break ; } ;
		 case 0: case UCNL:
		   iptr-- ; res = FALSE ; break ; 
	       } ;
	    } ;
	   tcb->ilvl[tcb->ilx].input_ptr = iptr ;
	   *optr = UCEOS ;
	   tcb->keyword_len = UCstrlen(tcb->UCstring) ; tcb->type = V4LEX_TknType_String ;
//  UCstrcpyToASC(tcb->string,tcb->UCstring) ;
	   return(rescan_ptr) ;
	 } ;
	if ((tg_flags & V4LEX_Option_WantUOM) != 0)
	 { 
	   goto numeric_lit_entry ;
	 } ;
/*	Check next character to see what type of token we have */
	byte = *tcb->ilvl[tcb->ilx].input_ptr ;
	if (byte > 127) { i = KEYWORD ; }
	 else if (byte != UClit('^') && (tg_flags & (V4LEX_Option_WantInt | V4LEX_Option_Hex))) { i = NUMERIC_LIT ; }
	 else if (tg_flags & V4LEX_Option_ForceKW) { i = type_of_token_keyword[byte] ; }
	 else { i = type_of_token[byte] ; } ;
//	switch (byte > 127 ? KEYWORD : (tg_flags & V4LEX_Option_ForceKW)!=0 ? type_of_token_keyword[byte] : type_of_token[byte])
	switch (i)
	 { case END_OF_LINE:
/*		Check for page marker (form-feed) */
		tcb->need_input_line = TRUE ;
		if (tg_flags & V4LEX_Option_RetEOL)		/* Return EOL as a token ? */
		 { if (tcb->ilvl[tcb->ilx].mode != V4LEX_InpMode_MacArg)
		    { tcb->type = V4LEX_TknType_EOL ; tcb->opcode = V_OpCode_EOX ; return(rescan_ptr) ; } ;
		 } ;
		goto top_of_module ;
	   case KEYWORD:
keyword_entry:
		tkn_sym = tcb->UCkeyword ; tcb->ilvl[tcb->ilx].input_ptr-- ; tcb->actual_keyword_len = 0 ;
/*		Accept "-" and "/" as part of keyword unless parsing XML /HTML*/
		if (tg_flags & V4LEX_Option_ForceKW)
		 { if (tg_flags & V4LEX_Option_XML)
		    {						/* Don't allow '-' or '/' if XML */
		    } else if (tg_flags & V4LEX_Option_HTML)
		    { keyword_translation['-'] = '-' ;		/* Allow '-' if HTML */
		      keyword_translation['.'] = '.' ;		/* Allow '.' if HTML */
		    } else					/* Otherwise allow both */
		    { keyword_translation['-'] = '-' ; keyword_translation['/'] = '/' ;
		    } ;
		 } ;
		/*		Loop thru all chars in keyword & normalize */
		for (i=0;i<V4LEX_Tkn_KeywordMax;i++)
		 { byte = *(++(tcb->ilvl[tcb->ilx].input_ptr)) ;
		   if (*(tkn_sym++) = (byte > 127 ? byte : keyword_translation[byte]))
		    { tcb->UCstring[tcb->actual_keyword_len++] = byte ; }
		    else { if (*(tcb->ilvl[tcb->ilx].input_ptr) == UClit('.'))			/* Have a "." but not ".." ? */
			    { if ((tg_flags & V4LEX_Option_AllowDot) && *(tcb->ilvl[tcb->ilx].input_ptr+1) != UClit('.'))
			       { *(tkn_sym-1) = UClit('.') ; tcb->UCstring[tcb->actual_keyword_len++] = UClit('.') ;
			         continue ;
			       } ;
			    } ;
			   if (tg_flags & (V4LEX_Option_ForceAsIs | V4LEX_Option_ForceAsIsULC)) goto end_of_keyword ;
			   if (*(tcb->ilvl[tcb->ilx].input_ptr) != UClit('#')) goto end_of_keyword ;
			   if (*(tcb->ilvl[tcb->ilx].input_ptr+2) != UClit('#')) goto end_of_keyword ;
			   b = *(tcb->ilvl[tcb->ilx].input_ptr+1) ;	/* Get x from #x# */
			   b = toupper(b) - UClit('A') ; if (b < 0 || b > 25) goto end_of_keyword ;
			   if (tcb->poundparam[b].numvalue != V4LEX_Tkn_PPNumUndef)
			    { UCsprintf(tcb->poundparam[b].value,15,UClit("%d"),tcb->poundparam[b].numvalue) ; } ;
			   tcb->ilvl[tcb->ilx].input_ptr += 2 ; tkn_sym-- ;
			   for(j=0;i<V4LEX_Tkn_KeywordMax;i++,j++)
			    { if((*(tkn_sym++) = keyword_translation[tcb->poundparam[b].value[j]]) == 0) break ;
			      tcb->UCstring[tcb->actual_keyword_len++] = tcb->poundparam[b].value[j] ; 
			    } ;
			 } ;
		} ;
/*		If here then keyword too long - scan to end but don't include in tcb->keyword */
		while (keyword_translation[*(++(tcb->ilvl[tcb->ilx].input_ptr))]) tcb->actual_keyword_len++ ;
end_of_keyword:
		tcb->UCstring[tcb->actual_keyword_len] = UCEOS ;
		keyword_translation['-'] = '\0' ;			/* Reset entry for dashes */
		keyword_translation['.'] = '\0' ;
		keyword_translation['/'] = '\0' ;
		keyword_translation['+'] = '\0' ; keyword_translation['['] = '\0' ;
		   keyword_translation[']'] = '\0' ; keyword_translation['$'] = '\0' ;
		tcb->keyword_len =
		   (tcb->actual_keyword_len > V4LEX_Tkn_KeywordMax ? V4LEX_Tkn_KeywordMax : tcb->actual_keyword_len) ;
		tcb->type = V4LEX_TknType_Keyword ; tcb->opcode = V_OpCode_Keyword ;
		if (tg_flags & (V4LEX_Option_ForceAsIs | V4LEX_Option_ForceAsIsULC))		/* If parsing for macro then copy to string */
		 { if (!(tg_flags & V4LEX_Option_ForceAsIsULC)) UCstrcpy(tcb->UCstring,tcb->UCkeyword) ;
//  UCstrcpyToASC(tcb->string,tcb->UCstring) ;
//  UCstrcpyToASC(tcb->keyword,tcb->UCkeyword) ;
		   return(rescan_ptr) ;
		 } ;
		if (tcb->ilvl[tcb->ilx].HaveMacroArgNames)	/* See if this is a macro argument name ? */
		 { LOGICAL gotArg ;
		   for(i=0;i<V4LEX_Tkn_ArgMax;i++)
		    { if (UCempty(tcb->ilvl[tcb->ilx].macarg[i].name)) { gotArg = FALSE ; break ; } ;
		      if (UCstrcmpIC(tcb->ilvl[tcb->ilx].macarg[i].name,tcb->UCkeyword) == 0) { gotArg = TRUE ; break ; } ;
		    } ;
		   if (gotArg)
	   	    { if (!tcb->ilvl[tcb->ilx].BoundMacroArgs) 
		       { if (!v4lex_BindArgs(tcb))		/* Bind arguments if necessary */
		          { tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
		       } ;
		      if (tcb->ilvl[tcb->ilx].macarg[i].value == NULL)
		       { v_Msg(NULL,tcb->UCstring,"TknMacArgMiss",i+1,tcb->UCkeyword) ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
//		       v4_error(V4E_MISSINGARG,tcb,"VCOM","LEX","MISSINGARG","Argument number %d (%s) missing from macro call",i+1,tcb->keyword) ;
/*		      If argument ends with '$' or '$$' then convert argument to string */
		      if (tcb->ilvl[tcb->ilx].input_ptr[0] == UClit('$'))
		       { if (tcb->ilvl[tcb->ilx].input_ptr[1] == UClit('$'))	/* '$$' - convert to macro argument, '$' - convert to string */
		          { LOGICAL needAB ;
		            tcb->ilvl[tcb->ilx].input_ptr += 2 ;
/*			    Try to see if we need to enclose macro argument in '<...>' */
			    if (tcb->ilvl[tcb->ilx].macarg[i].value[0] == UClit('"') || tcb->ilvl[tcb->ilx].macarg[i].value[0] == UClit('\''))
			     { needAB = FALSE ;		/* Quoted string does NOT need angle brackets */
			     } else
			     { UCCHAR *b ;
			       for(b=tcb->ilvl[tcb->ilx].macarg[i].value;*b!=UCEOS;b++)
			        { if (!vuc_IsAlphaNum(*b)) break ; } ;
			       needAB = (*b != UCEOS) ;
			     } ;
			    if (needAB)
			     { UCCHAR *ucb = v4mm_AllocUC(UCstrlen(tcb->ilvl[tcb->ilx].macarg[i].value) + 2) ;
			       UCstrcpy(ucb,UClit("<")) ; UCstrcat(ucb,tcb->ilvl[tcb->ilx].macarg[i].value) ; UCstrcat(ucb,UClit(">")) ;
			       v_StringLit(ucb,tcb->UCstring,UCsizeof(tcb->UCstring),UClit('"'),UCEOS) ;
			       v4mm_FreeChunk(ucb) ;
			     } else
			     { v_StringLit(tcb->ilvl[tcb->ilx].macarg[i].value,tcb->UCstring,UCsizeof(tcb->UCstring),UClit('"'),UCEOS) ;
			     } ;
		          } else 
		          { tcb->ilvl[tcb->ilx].input_ptr += 1 ;
			    v_StringLit(tcb->ilvl[tcb->ilx].macarg[i].value,tcb->UCstring,UCsizeof(tcb->UCstring),UClit('"'),UCEOS) ;
			  } ;
			 v4lex_NestInput(tcb,NULL,tcb->UCstring,V4LEX_InpMode_MacArg) ;
		       } else
		       { v4lex_NestInput(tcb,NULL,tcb->ilvl[tcb->ilx].macarg[i].value,V4LEX_InpMode_MacArg) ;
		       } ;
	   	      goto top_of_module ;			/* Push argument value & re-parse */
		    } ;
		 } ;
//  UCstrcpyToASC(tcb->keyword,tcb->UCkeyword) ;
		return(rescan_ptr) ;

	   case STRING_LIT:
string_entry:
		tcb->opcode = (*tcb->ilvl[tcb->ilx].input_ptr == UClit('"') ? V_OpCode_DQString : V_OpCode_QString) ;
		b = *(tcb->ilvl[tcb->ilx].input_ptr - 1) ;	/* Look for "possessive" */
		if (*tcb->ilvl[tcb->ilx].input_ptr == UClit('\'') && *(tcb->ilvl[tcb->ilx].input_ptr+1) == UClit('s') &&
		     ((b >= UClit('A') && b <= UClit('Z')) || (b >= UClit('a') && b <= UClit('z'))) )
		 { tcb->opcode = V_OpCode_Possessive ; tcb->prec = 18 ; UCstrcpy(tcb->UCkeyword,UClit("'s")) ;
		   tcb->ilvl[tcb->ilx].input_ptr += 2 ; goto type_punc ;
		 } ;
/*		Set up to scan to ending delimeter */
		tcb->literal_len = 0 ; str_ptr = tcb->UCstring ;
		if (tg_flags & (V4LEX_Option_ForceAsIs | V4LEX_Option_ForceAsIsULC))		/* If parsing for macro then capture leading quote */
		 { tcb->literal_len++ ; delimiter = (*(str_ptr++) = *(tcb->ilvl[tcb->ilx].input_ptr++)) ;
		   quotenext = FALSE ;
		   for (;;)
		    { if (quotenext ? FALSE : *(tcb->ilvl[tcb->ilx].input_ptr) == delimiter)
		       { if (*(++(tcb->ilvl[tcb->ilx].input_ptr)) != delimiter) break ;
/*			 Got double delimiter - append it */
		         tcb->literal_len++ ; *(str_ptr++) = delimiter ; tcb->literal_len++ ; *(str_ptr++) = delimiter ;
			 tcb->ilvl[tcb->ilx].input_ptr++ ; continue ;
		       } ;
		      if (quotenext) { quotenext = FALSE ; }
		       else { if (*tcb->ilvl[tcb->ilx].input_ptr == UClit('\\')) quotenext = TRUE ; } ;
		      if (++tcb->literal_len > V4LEX_Tkn_StringMax)
		        { UCstrncpy(tcb->UCkeyword,tcb->UCstring,30) ; v_Msg(NULL,tcb->UCstring,"TknStrTooLong",tcb->UCkeyword,V4LEX_Tkn_StringMax) ;
			  tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;
			} ;
		      *(str_ptr++) = (sc = *(tcb->ilvl[tcb->ilx].input_ptr++)) ;
/*		      Make sure we have not run off the end of the input line */
		      if (sc > 255) continue ;	/* Got Unicode character - continue */
		      if (type_of_token[sc] == END_OF_LINE)
/*		         End of line - check to see if '&' terminated */
		       { 
		         if (*(str_ptr-2) == UClit('&') || *(str_ptr-2) == UClit('\\'))
		          { v4lex_ReadNextLine(tcb,0,NULL,NULL) ; multi_line = TRUE ; str_ptr+=(-2) ; tcb->literal_len+=(-2) ; continue ; }
		          else if ( *(str_ptr-2) == UClit(' ') && (*(str_ptr-3) == UClit('&') || *(str_ptr-3) == UClit('\\')))
			        { v4lex_ReadNextLine(tcb,0,NULL,NULL) ; multi_line = TRUE ; str_ptr+=(-3) ; tcb->literal_len+=(-3) ; continue ; }
		          else { tcb->need_input_line = TRUE ; v_Msg(NULL,tcb->UCstring,"TknEOLinStr") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;
			       } ;
/*		         Got continued string literal - read in next line & continue */
		       } ;
		    } ;
		   tcb->literal_len++ ; *(str_ptr++) = delimiter ;
/*		   End of string literal - set up value format */
		   tcb->type = (tcb->radix == V4LEX_Radix_V4EvalStr ? V4LEX_TknType_VString : (tcb->radix == V4LEX_Radix_V4PointStr ? V4LEX_TknType_PString : V4LEX_TknType_String)) ;
		   *(str_ptr) = UCEOS ;	/* Terminate with null value */
//  UCstrcpyToASC(tcb->string,tcb->UCstring) ;
		   return(rescan_ptr) ;
		 } ;
		delimiter = *(tcb->ilvl[tcb->ilx].input_ptr++) ;
		mlDelimiter = UCEOS ;					/* Reset any prior multi-line delimiter */
mlstring_entry:
		for (;;)
		 { if (*(tcb->ilvl[tcb->ilx].input_ptr) == delimiter)
		    { if (*(++(tcb->ilvl[tcb->ilx].input_ptr)) != delimiter) break ;
/*		      Got double delimiter - append it */
		      tcb->literal_len++ ; *(str_ptr++) = delimiter ; tcb->ilvl[tcb->ilx].input_ptr++ ; continue ;
		    } ;
		   if (++tcb->literal_len > V4LEX_Tkn_StringMax)
		     { UCstrncpy(tcb->UCkeyword,tcb->UCstring,30) ; v_Msg(NULL,tcb->UCstring,"TknStrTooLong",tcb->UCkeyword,V4LEX_Tkn_StringMax) ;
		       tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;
		     } ;
/*		   Check for special "\x" characters */
		   if (*tcb->ilvl[tcb->ilx].input_ptr == UClit('\\') && (tg_flags & (V4LEX_Option_XML | V4LEX_Option_HTML | V4LEX_Option_RegExp)) == 0)
		    { if ((tg_flags & V4LEX_Option_JSON) != 0)
		       { 
//VEH140731 - I don't think we want this now that 'AsIs' dimension option added for JSON
//		         if (*(tcb->ilvl[tcb->ilx].input_ptr+1) == UClit('"') || *(tcb->ilvl[tcb->ilx].input_ptr+1) == UClit('\''))
//			  { tcb->ilvl[tcb->ilx].input_ptr += 2 ; *(str_ptr++) = UClit('\'') ;  continue ; } ;				/* Only handle escaped quotes */
//			 goto no_escape ;												/* All others - keep the backslash AND next character */
		       } ;
		      switch (*(++(tcb->ilvl[tcb->ilx].input_ptr)))
			{ default:
/*				Check for upper case letters */
				if (*tcb->ilvl[tcb->ilx].input_ptr >= UClit('A') && *tcb->ilvl[tcb->ilx].input_ptr <= UClit('Z'))
				 { *(str_ptr++) = *tcb->ilvl[tcb->ilx].input_ptr - UClit('A') + 1 ; break ; } ;
/*				Last chance - maybe a decimal number */
				if (*tcb->ilvl[tcb->ilx].input_ptr < UClit('0') || *tcb->ilvl[tcb->ilx].input_ptr > UClit('9'))
				 { tcb->UCkeyword[0] = *tcb->ilvl[tcb->ilx].input_ptr ; tcb->UCkeyword[1] = UCEOS ;
				   v_Msg(NULL,tcb->UCstring,"TknEscChar",tcb->UCkeyword) ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;
				 } ;
				for(*str_ptr = 0;;tcb->ilvl[tcb->ilx].input_ptr++)
				 { sc = *(tcb->ilvl[tcb->ilx].input_ptr) ;
				   if (sc < UClit('0') || sc > UClit('9')) break ; *str_ptr = *str_ptr * 10 + sc-UClit('0') ;
				 } ;
				if ((tg_flags & V4LEX_Option_PatternSub) && *str_ptr < 10)
				 { *str_ptr += 1 ; } ;		/* If got a pattern (single quote) then add one to \0 ... \9 */
				tcb->ilvl[tcb->ilx].input_ptr -- ; str_ptr ++ ; break ;
			  case UClit('0'):		/* If '\0nn' then nn is octal number */
				{ INDEX i ;
				  for(*str_ptr = 0,i=0;i<3;i++,tcb->ilvl[tcb->ilx].input_ptr++)
				   { sc = *(tcb->ilvl[tcb->ilx].input_ptr) ;
				     if (sc < UClit('0') || sc > UClit('7'))
				      { v_Msg(NULL,tcb->UCstring,"TknEscChar",tcb->UCkeyword) ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
				     *str_ptr = *str_ptr * 8 + sc-UClit('0') ;
				   } ;
				}
				tcb->ilvl[tcb->ilx].input_ptr -- ; str_ptr ++ ;
				break ;
			  case UClit('n'):
#ifdef WINNT
				*(str_ptr++) = 13 ; *(str_ptr++) = 10 ; ++tcb->literal_len ;
#else
 #ifdef UNIX
				*(str_ptr++) = 10 ;
 #else
				*(str_ptr++) = 13 ; *(str_ptr++) = 10 ; ++tcb->literal_len ;
 #endif
#endif
				break ;
			  case UClit('u'):
			  case UClit('+'):
/*				Parse 4 digit hex unicode value */
				for(lres=0,i=0;i<4;i++)
				 { sc = *(++tcb->ilvl[tcb->ilx].input_ptr) ;
				   if (sc >= UClit('0') && sc <= UClit('9')) { lres <<= 4 ; lres |= (sc - UClit('0')) ; }
				    else if (sc >= UClit('A') && sc <= UClit('F'))  { lres <<= 4 ; lres |= 10 + (sc - UClit('A')) ; }
				    else if (sc >= UClit('a') && sc <= UClit('f'))  { lres <<= 4 ; lres |= 10 + (sc - UClit('a')) ; }
				    else { v_Msg(NULL,tcb->UCstring,"TknUCChar",sc) ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
				 } ;
				*(str_ptr++) = lres ; break ;
			  case UClit('x'):
/*				Parse 2 digit hex unicode value */
				for(lres=0,i=0;i<2;i++)
				 { sc = *(++tcb->ilvl[tcb->ilx].input_ptr) ;
				   if (sc >= UClit('0') && sc <= UClit('9')) { lres <<= 4 ; lres |= (sc - UClit('0')) ; }
				    else if (sc >= UClit('A') && sc <= UClit('F'))  { lres <<= 4 ; lres |= 10 + (sc - UClit('A')) ; }
				    else if (sc >= UClit('a') && sc <= UClit('f'))  { lres <<= 4 ; lres |= 10 + (sc - UClit('a')) ; }
				    else { v_Msg(NULL,tcb->UCstring,"TknUCChar",sc) ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
				 } ;
				*(str_ptr++) = lres ; break ;
			  case UClit('$'): *(str_ptr++) = 27 ; break ;
			  case UClit('b'): *(str_ptr++) = 7 ; break ;
			  case UClit('\\'): *(str_ptr++) = UClit('\\') ; break ;
			  case UClit('/'): *(str_ptr++) = UClit('/') ; break ;
			  case UClit('<'): *(str_ptr++) = 8 ; break ;
			  case UClit('f'): *(str_ptr++) = 12 ; break ;
			  case UClit('t'): *(str_ptr++) = 9 ; break ;
			  case UClit('!'): *(str_ptr++) = UClit('!') ; break ;
			  case UClit('l'): *(str_ptr++) = 10 ; break ;
			  case UClit('r'): *(str_ptr++) = 13 ; break ;
			  case UClit('\''): *(str_ptr++) = UClit('\'') ; break ;
			  case UClit('"'): *(str_ptr++) = UClit('"') ; break ;
			  case UClit('\0'):
			  case UClit('\n'):
			  case UClit('\r'): v4lex_ReadNextLine(tcb,0,NULL,NULL) ; multi_line = TRUE ; continue ;

			} ;
		      ++(tcb->ilvl[tcb->ilx].input_ptr) ; continue ;
		    } ;
//no_escape:
		   *(str_ptr++) = (sc = *(tcb->ilvl[tcb->ilx].input_ptr++)) ;	/* Copy byte into tcb->UCstring */
		   if (sc > 255)
		    { tcb->opcode = V_OpCode_DQUString ; continue ; } ;
/*		   Make sure we have not run off the end of the input line */
//VEH160601 - Not sure why skipping this code if in JSON - can cause memory trap
//		   if (type_of_token[sc] == END_OF_LINE && (tg_flags & V4LEX_Option_JSON) == 0)
		   if (type_of_token[sc] == END_OF_LINE)
/*		      End of line - check to see if '&' terminated */
		    { if ((tg_flags & V4LEX_Option_JSON) != 0)
		       { v_Msg(NULL,tcb->UCstring,"TknEOLinStr") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
		      if (*(str_ptr-2) == '&')
		       { v4lex_ReadNextLine(tcb,0,NULL,NULL) ; multi_line = TRUE ; str_ptr+=(-2) ; tcb->literal_len+=(-2) ; continue ; }
		       else if ( *(str_ptr-2) == ' ' && *(str_ptr-3) == '&')
			     { v4lex_ReadNextLine(tcb,0,NULL,NULL) ; multi_line = TRUE ; str_ptr+=(-3) ; tcb->literal_len+=(-3) ; continue ; }
		       else { tcb->need_input_line = TRUE ;
			      if (tg_flags & V4LEX_Option_MLStringLit)
			       { *(str_ptr) = UCEOS ; mlDelimiter = delimiter ;		/* Save the delimiter for the next entry */
			         tcb->type = V4LEX_TknType_MLString ; tcb->opcode = V_OpCode_MLString ; return(rescan_ptr) ;
			       } else
			       { v_Msg(NULL,tcb->UCstring,"TknEOLinStr") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;
			       } ;
			    } ;
/*		      Got continued string literal - read in next line & continue */
		    } ;
		 } ;
/*		End of string literal - set up value format */
		tcb->type = (tcb->radix == V4LEX_Radix_V4EvalStr ? V4LEX_TknType_VString : (tcb->radix == V4LEX_Radix_V4PointStr ? V4LEX_TknType_PString : V4LEX_TknType_String)) ;
		if (delimiter == UClit('/')) { tcb->type = V4LEX_TknType_RegExpStr ; tcb->opcode = V_OpCode_RegExpStr ; } ;
		*(str_ptr) = UCEOS ;	/* Terminate with null value */
		mlDelimiter = UCEOS ;	/* Clear this out so we don't get confused on reentry */
//  UCstrcpyToASC(tcb->string,tcb->UCstring) ;
//  UCstrcpyToASC(tcb->keyword,tcb->UCkeyword) ;
		return(rescan_ptr) ;

	   case RADIX_OVERRIDE:
		uom = tcb->UCkeyword ;
		if (tg_flags & (V4LEX_Option_ForceAsIs | V4LEX_Option_ForceAsIsULC)) str_ptr = tcb->UCstring ;
		switch (*(++(tcb->ilvl[tcb->ilx].input_ptr)))
		 { 
		   default: v_Msg(NULL,tcb->UCstring,"TknRadix") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;

		   case UClit('a'):
		   case UClit('A'): tcb->ilvl[tcb->ilx].input_ptr++ ; v4lex_NextTkn(tcb,FALSE) ;
/*			     Best have a string literal */
			     if (tcb->type != V4LEX_TknType_String)
			      { v_Msg(NULL,tcb->UCstring,"TknAlphaLit") ;	tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;
			      } ;
			     lres = tcb->UCstring[0] ; has_minus = 0 ; dps = 0 ; have_exp = 0 ; goto numeric_literal_finish ;
		   case UClit('n'):
		   case UClit('N'): iptr = tcb->prior_input_ptr ;
			     tcb->ilvl[tcb->ilx].input_ptr++ ; v4lex_NextTkn(tcb,FALSE) ;
			     if (!(tcb->opcode == V_OpCode_LParen || tcb->opcode == V_OpCode_NCLParen))
			      { v_Msg(NULL,tcb->UCstring,"TknBitLit") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;
			      } ;
			     for(lres=0;;)
			      { v4lex_NextTkn(tcb,FALSE) ;
			        if (tcb->type != V4LEX_TknType_Integer) break ;
				if (tcb->integer < 1 || tcb->integer > 32 || tcb->decimal_places > 0)
				 { v_Msg(NULL,tcb->UCstring,"TknBitLit") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;
				 } ;
				lres |= (B64INT)(1<<(tcb->integer-1)) ;
				v4lex_NextTkn(tcb,FALSE) ; if (tcb->opcode != V_OpCode_Comma) break ;
			      } ;
			     if (tcb->opcode != V_OpCode_RParen)
			      { v_Msg(NULL,tcb->UCstring,"TknBitLit") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;
			      } ;
			     tcb->prior_input_ptr = iptr ;
			     have_num = TRUE ; has_minus = 0 ; dps = 0 ; have_exp = 0 ; goto numeric_literal_finish ;
		   case UClit('b'):
		   case UClit('B'): tcb->radix = 2 ; break ;
		   case UClit('d'):
		   case UClit('D'): tcb->radix = 10 ; break ;
		   case UClit('o'):
		   case UClit('O'): tcb->radix = 8 ; break ;
		   case UClit('x'):
		   case UClit('X'): tcb->radix = 16 ; break ;
		   case UClit('4'):


			uc = tcb->ilvl[tcb->ilx].input_ptr+1 ;
			ip = (int *)tcb->UCstring ; tcb->literal_len = 0 ;	/* Stage results here */
			for(i=0;*uc!=UCEOS;uc++)
			 { switch(*uc)
			    { default:
				tcb->UCkeyword[0] = *uc ; tcb->UCkeyword[1] = UCEOS ; v_Msg(NULL,tcb->UCstring,"TknHexLit",tcb->UCkeyword) ;
				tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;
			      case 10: case 13:
			      case UClit(';'):
			      case UClit(' '):
			      case UClit('	'): goto hex4_done ;
			      case UClit(','):
				*ip = i ; ip++ ; i = 0 ; tcb->literal_len += 4 ;
				break ;
			      case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'):
			      case UClit('5'): case UClit('6'): case UClit('7'): case UClit('8'): case UClit('9'):		i = (i<<4) + *uc - UClit('0') ; break ;
			      case UClit('a'): case UClit('b'): case UClit('c'): case UClit('d'): case UClit('e'): case UClit('f'):	i = (i<<4) + *uc - UClit('a') + 10 ; break ;
			      case UClit('A'): case UClit('B'): case UClit('C'): case UClit('D'): case UClit('E'): case UClit('F'):	i = (i<<4) + *uc - UClit('A') + 10 ; break ;
			    } ;
			 } ;
hex4_done:
			*ip = i ; ip++ ; tcb->literal_len += 4 ;
			tcb->type = V4LEX_TknType_BString ; tcb->ilvl[tcb->ilx].input_ptr = uc ;
			return(rescan_ptr) ;
		   case UClit('P'):
		   case UClit('p'):
			tcb->radix = V4LEX_Radix_V4PointStr ;		/* Tweak radix so string parser knows what's going on */
			if (*(tcb->ilvl[tcb->ilx].input_ptr+1) != UClit('"'))
			 { v_Msg(NULL,tcb->UCstring,"TknV4Over") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
			tcb->ilvl[tcb->ilx].input_ptr ++ ;
			goto string_entry ;
		   case UClit('V'):
		   case UClit('v'):
			tcb->radix = V4LEX_Radix_V4EvalStr ;		/* Tweak radix so string parser knows what's going on */
			if (*(tcb->ilvl[tcb->ilx].input_ptr+1) != UClit('"'))
			 { v_Msg(NULL,tcb->UCstring,"TknV4Over") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
			tcb->ilvl[tcb->ilx].input_ptr ++ ;
			goto string_entry ;
		 } ;
		if (tg_flags & (V4LEX_Option_ForceAsIs | V4LEX_Option_ForceAsIsULC))		/* If parsing for macro then capture all info */
		 { str_ptr = tcb->UCstring ; *(str_ptr++) = UClit('^') ; *(str_ptr++) = *tcb->ilvl[tcb->ilx].input_ptr ; } ;
		tcb->ilvl[tcb->ilx].input_ptr++ ; goto radix_entry ;

	   case NUMERIC_LIT:
numeric_lit_entry:
		if (tg_flags & (V4LEX_Option_ForceAsIs | V4LEX_Option_ForceAsIsULC)) str_ptr = tcb->UCstring ;
		if (tg_flags & V4LEX_Option_Hex) tcb->radix = 16 ;
/*		Assume default radix unless overridden (see above) */
radix_entry:
/*		Init all sorts of stuff */
		lres = 0 ;	/* Initial result */
		dps = 0 ;	/* Number of decimal places */
		have_dp = 0 ; 	/* Set to 1 on "." */
		fres = 0.0 ; 	/* Floating point temp result */
		have_exp = 0 ;	/* Set to +/- 1 on pos/neg exponent */
		has_minus = FALSE ;	/* Set to TRUE if leading minus sign */
		have_num = FALSE ;	/* Set to TRUE when we get any portion of number (i.e. not just "-" or radix stuff) */
		uom = tcb->UCkeyword ;
		uom_status = -1 ;	/* -1 = no uom, 0 = parsing uom, 1 = finished uom */
		iptr = tcb->ilvl[tcb->ilx].input_ptr ;	/* Save ptr to begin in case we want to rescan as different type of token */
		if (tg_flags & V4LEX_Option_WantDbl)
		 { UCCHAR *bp ;
		   tcb->floating = UCstrtod(tcb->ilvl[tcb->ilx].input_ptr,&bp) ;
		   tcb->type = V4LEX_TknType_Float ; tcb->ilvl[tcb->ilx].input_ptr = bp ;
		   tcb->opcode = V_OpCode_Numeric ;
/*		   We can also end number with '..' indicating a range. We need special check for this */
		   if (numeric_array[*bp] != END_OF_NUMBER)
		    { if (*bp == UClit('.'))
		       { if (*(bp-1) == UClit('.')) { tcb->ilvl[tcb->ilx].input_ptr-- ; return(rescan_ptr) ; } ;
		         if (*(bp+1) == UClit('.')) return(rescan_ptr) ;
		       } ;
		      v_Msg(NULL,tcb->UCstring,"DPIMNotFP") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
		   return(rescan_ptr) ;
		 } ;
		if (*tcb->ilvl[tcb->ilx].input_ptr == UClit('.') && (*(tcb->ilvl[tcb->ilx].input_ptr+1) < UClit('0') || *(tcb->ilvl[tcb->ilx].input_ptr+1) > UClit('9')))
		 goto dot_entry ;
		if (*tcb->ilvl[tcb->ilx].input_ptr == UClit('-'))
		 { tcb->ilvl[tcb->ilx].input_ptr++ ; has_minus = TRUE ; if (tg_flags & (V4LEX_Option_ForceAsIs | V4LEX_Option_ForceAsIsULC)) *(str_ptr++) = UClit('-') ;
		   if (*tcb->ilvl[tcb->ilx].input_ptr == UClit(' ')) { tcb->ilvl[tcb->ilx].input_ptr ++ ; } ;
		 } ;
/*		If literal is decimal then make UClit('E') for exponential */
		numeric_array[UClit('e')] = (numeric_array[UClit('E')] = (tcb->radix == 10 ? EXPONENT : HEX_DIGIT)) ;
/*		Loop thru all characters in numeric literal */
		for (;;)
		 { if (tg_flags & (V4LEX_Option_ForceAsIs | V4LEX_Option_ForceAsIsULC)) *(str_ptr++) = *tcb->ilvl[tcb->ilx].input_ptr ;
		   switch (numeric_array[sc = *tcb->ilvl[tcb->ilx].input_ptr++])
		    { case DIGIT:
/*			If parsing UOM and  have already gotten non-numeric (i.e. uom has been updated), then continue updating uom */
			if (uom_status == 0) { *(uom++) = sc ; break ; } ;
			if ((i = sc-UClit('0')) >= tcb->radix)
			  { v_Msg(NULL,tcb->UCstring,"TknDigitTooBig",i) ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;
			  } ;
			if (have_exp == 0) fres = fres*10.0 + (double)i ;
			dps += have_dp ; lresa = lres ; lres = (lres*tcb->radix)+i ;
			if (lres < lresa)
			 { if ((tg_flags & V4LEX_Option_JSON) != 0)
			    { tcb->ilvl[tcb->ilx].input_ptr = rescan_ptr ; tg_flags |= V4LEX_Option_WantDbl ; goto numeric_lit_entry ; } ;
			   v_Msg(NULL,tcb->UCstring,"TknNumLitBig") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;
			 } ;
			have_num = TRUE ; break ;
		      case HEX_DIGIT:
			if ((tg_flags & V4LEX_Option_WantUOM) != 0 && uom_status != 1)
			 { uom_status = 0 ;
			   *(uom++) = sc ; break ;
			 } ;
			if (sc >= UClit('a')) sc += -32 ;
			if ((i = sc+10-UClit('A')) >= tcb->radix)
			 { if ((tg_flags & (V4LEX_Option_ForceAsIs | V4LEX_Option_ForceAsIsULC)) == 0)
			    { if (!has_minus && ((tg_flags & (V4LEX_Option_WantInt | V4LEX_Option_WantDbl | V4LEX_Option_WantLInt)) == 0))	/* Treat as keyword only under specific conditions */
			       { tcb->ilvl[tcb->ilx].input_ptr = iptr ; goto keyword_entry ; } ;
			      tcb->UCkeyword[0] = sc ; tcb->UCkeyword[1] = UCEOS ; v_Msg(NULL,tcb->UCstring,"TknHexLit2",tcb->UCkeyword,tcb->radix) ;
			      tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;
			    } ;
			 } ;
			lres = (lres<<4)+i ; have_num = TRUE ; break ;
		      case DECIMAL_POINT:
			if (tcb->in_jsonRef)
			 { if (!have_num)
			    { tcb->UCkeyword[0] = *tcb->ilvl[tcb->ilx].input_ptr ; tcb->opcode = V_OpCode_Dot ; tcb->prec = 18 ; tcb->ilvl[tcb->ilx].input_ptr -- ; goto single_punc ; } ;
			   tcb->ilvl[tcb->ilx].input_ptr -- ; goto numeric_literal_finish ;
			 } ;
/*			Check for ".." */
			if (*tcb->ilvl[tcb->ilx].input_ptr == UClit('.'))
			 { tcb->ilvl[tcb->ilx].input_ptr-- ; goto numeric_literal_finish ; } ;
			if (have_dp)
			 { if (!has_minus && ((tg_flags & (V4LEX_Option_WantInt | V4LEX_Option_WantDbl | V4LEX_Option_WantLInt)) == 0))	/* Treat as keyword only under specific conditions */
			    { tcb->ilvl[tcb->ilx].input_ptr = iptr ;
			      keyword_translation['-'] = '-' ; keyword_translation['.'] = '.' ;	/* Assume keyword - allow '-' & '.' */
			      goto keyword_entry ;
			    } ;
			   v_Msg(NULL,tcb->UCstring,"TknOneDP") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;
			 } ;
			if (have_exp)
			 { if (!has_minus && ((tg_flags & (V4LEX_Option_WantInt | V4LEX_Option_WantDbl | V4LEX_Option_WantLInt)) == 0))	/* Treat as keyword only under specific conditions */
			    { tcb->ilvl[tcb->ilx].input_ptr = iptr ; goto keyword_entry ; } ;
			   v_Msg(NULL,tcb->UCstring,"TknExpDP") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;
			 } ;
			if (tcb->radix != 10)
			 { if (!has_minus && ((tg_flags & (V4LEX_Option_WantInt | V4LEX_Option_WantDbl | V4LEX_Option_WantLInt)) == 0))	/* Treat as keyword only under specific conditions */
			    { tcb->ilvl[tcb->ilx].input_ptr = iptr ; goto keyword_entry ; } ;
			   v_Msg(NULL,tcb->UCstring,"TknDPRadixLit") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;
			 } ;
			have_dp++ ; break ;
		      case EXPONENT:
			if ((tg_flags & V4LEX_Option_WantUOM) != 0) { *(uom++) = sc ; break ; } ;
			if (have_exp != 0)
			 { if (!has_minus && ((tg_flags & (V4LEX_Option_WantInt | V4LEX_Option_WantDbl | V4LEX_Option_WantLInt)) == 0))	/* Treat as keyword only under specific conditions */
			    { tcb->ilvl[tcb->ilx].input_ptr = iptr ; goto keyword_entry ; } ;
			   v_Msg(NULL,tcb->UCstring,"TknOneDP") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;
			 } ;
			if (tcb->radix != 10)
			 { if (!has_minus && ((tg_flags & (V4LEX_Option_WantInt | V4LEX_Option_WantDbl | V4LEX_Option_WantLInt)) == 0))	/* Treat as keyword only under specific conditions */
			    { tcb->ilvl[tcb->ilx].input_ptr = iptr ; goto keyword_entry ; } ;
			   v_Msg(NULL,tcb->UCstring,"TknFPRadix") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;
			 } ;

/*			Look for optional "+" or "-" */
			have_exp = 1 ; have_dp = 0 ;
			if (*(tcb->ilvl[tcb->ilx].input_ptr) == UClit('+')) {tcb->ilvl[tcb->ilx].input_ptr++ ; }
			 else {if (*(tcb->ilvl[tcb->ilx].input_ptr) == UClit('-'))
				{ tcb->ilvl[tcb->ilx].input_ptr++ ; have_exp = -1 ; } ;
			      } ;
			lres = 0 ; break ;
		      case END_OF_NUMBER:
			if ((tg_flags & V4LEX_Option_WantUOM) != 0)
			 { switch (sc)
			    { default:
				*(uom++) = sc ; break ;
			      case UClit(' '): case '\t': case UClit('}'): case UClit(']'): case UClit(')'): case UCEOS: case UClit('\n'): case UClit('\r'):
				tcb->ilvl[tcb->ilx].input_ptr-- ; goto numeric_literal_finish ;
			    } ; break ;
			 } ;
			tcb->ilvl[tcb->ilx].input_ptr-- ; goto numeric_literal_finish ;
		      case IGNOREIT: break ;
		      default:
			if ((tg_flags & V4LEX_Option_WantUOM) != 0 && uom_status != 1)
			 { if (sc == UClit(' '))		/* Got a space? then either starting or ending UOM */
			    { uom_status = (uom_status == -1 ? 0 : 1) ; break ; } ;
			   if (uom_status == -1) uom_status = 0 ;
			   if (uom_status == 1) break ;
			   *(uom++) = sc ;
			   break ;
			 } ;
			if (tg_flags & (V4LEX_Option_ForceAsIs | V4LEX_Option_ForceAsIsULC)) break ;
/* VEH030918		If got funky character assume token is really a keyword & reparse as such */
			tcb->ilvl[tcb->ilx].input_ptr = tcb->prior_input_ptr ;
			tcb->ilvl[tcb->ilx].input_ptr += UCstrspn(tcb->ilvl[tcb->ilx].input_ptr,UClit(" 	")) ;
			goto keyword_entry ;
//			tcb->type = V4LEX_TknType_Error ; strcpy(tcb->UCstring,"Invalid character in/terminating numeric literal") ; return(rescan_ptr) ;
//			strcpy(tcb->UCstring,"Invalid character in/terminating numeric literal") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;

		    } ;
		 } ;
/*		Here to finish off the literal */
numeric_literal_finish:
		if (tg_flags & (V4LEX_Option_ForceAsIs | V4LEX_Option_ForceAsIsULC)) { str_ptr -- ; *str_ptr = UCEOS ; } ;
		*uom = UCEOS ;
		tcb->opcode = V_OpCode_Numeric ;
		if (!have_num)		/* Got a minus but no number - return opcode for minus */
		 { if (has_minus)
		    { tcb->type = V4LEX_TknType_Punc ; tcb->opcode = V_OpCode_Minus ; return(rescan_ptr) ; } ;
		   tcb->type = V4LEX_TknType_Error ; v_Msg(NULL,tcb->UCstring,"TknNoNumLit") ; return(rescan_ptr) ;
		 } ;
		if (have_exp != 0)
		 { i = (have_exp*lres)-dps ;
		   if (i > 0) { for(;i>0;i--) fres = fres * (double)10.0 ; }
		    else { for(;i<0;i++) fres = fres / (double)10.0 ; } ;
		   tcb->floating = fres ;
/*		   tcb->floating = fres * pow((double)10.0,(double)i) ; ? Could not get this to work? */
		   if (has_minus) tcb->floating = -tcb->floating ;
		   tcb->literal_len = 8 ; tcb->type = V4LEX_TknType_Float ;
		 }
		 else
		 { 
		   if (dps > V_MAX_LITERAL_DECIMALS)
		    { tcb->floating = (has_minus ? -lres : lres) / pow(10.0,dps) ;
		      tcb->literal_len = 8 ; tcb->type = V4LEX_TknType_Float ;
		      return(rescan_ptr) ;
		    } ;
		   tcb->integer = (has_minus ? -lres : lres) ; tcb->decimal_places = dps ; tcb->Linteger = (has_minus ? -lres : lres) ;
		   if ((lres & (tcb->radix == 10 ? 0xffffffff80000000 : 0xffffffff00000000)) != 0)
		    { if ((tg_flags & V4LEX_Option_WantInt) != 0)
		       { v_Msg(NULL,tcb->UCstring,"TknNumLitBig") ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;

		      tcb->literal_len = sizeof tcb->Linteger ; tcb->type = V4LEX_TknType_LInteger ;
		    } else
		    { tcb->literal_len = sizeof tcb->integer ; tcb->type = V4LEX_TknType_Integer ;
		    } ;
		 } ;
		return(rescan_ptr) ;

#define PUNC1(PREC,OP) tcb->UCkeyword[0] = *tcb->ilvl[tcb->ilx].input_ptr ; tcb->opcode = OP ; tcb->prec = PREC ; goto single_punc
#define PUNC2(CHAR,PREC,OP) \
  if ((*(tcb->ilvl[tcb->ilx].input_ptr+1)) == CHAR)\
   {tcb->UCkeyword[0] = *tcb->ilvl[tcb->ilx].input_ptr ; tcb->UCkeyword[1] = *(tcb->ilvl[tcb->ilx].input_ptr+1) ; tcb->opcode = OP ; tcb->prec = PREC ; goto double_punc ; }
#define PUNC3(STR,PREC,OP) \
  if (UCstrncmp(tcb->ilvl[tcb->ilx].input_ptr,UClit(STR),3) == 0)\
   { UCstrcpy(tcb->UCkeyword,UClit(STR)) ; tcb->opcode = OP ; tcb->prec = PREC ; goto triple_punc ; }
#define PUNC4(STR,PREC,OP) \
  if (UCstrncmp(tcb->ilvl[tcb->ilx].input_ptr,UClit(STR),4) == 0)\
   { UCstrcpy(tcb->UCkeyword,UClit(STR)) ; tcb->opcode = OP ; tcb->prec = PREC ; goto quad_punc ; }

	   case UClit('!'):	PUNC2(UClit('='),24,V_OpCode_NotEqual) ; PUNC1(23,V_OpCode_Not) ;
	   case UClit('#'):	if (*(tcb->ilvl[tcb->ilx].input_ptr+1) >= UClit('0') && *(tcb->ilvl[tcb->ilx].input_ptr+1) <= UClit('9')
				&& *(tcb->ilvl[tcb->ilx].input_ptr+2) == UClit('#'))
				 { tcb->opcode = V_OpCode_MacArg0 + *(tcb->ilvl[tcb->ilx].input_ptr+1)-UClit('0') ; /* Got arg: #n# for n=0..9 */
				   tcb->UCkeyword[0] = (tcb->UCkeyword[2] = UClit('#')) ; tcb->UCkeyword[1] = *(tcb->ilvl[tcb->ilx].input_ptr+1) ;
				   tcb->UCkeyword[3] = 0 ; tcb->prec = 30 ; tcb->ilvl[tcb->ilx].input_ptr += 3 ; tcb->keyword_len = 3 ;
				   goto type_punc ;
		   		 } else if (*(tcb->ilvl[tcb->ilx].input_ptr+1) >= UClit('A') && *(tcb->ilvl[tcb->ilx].input_ptr+1) <= UClit('Z')
					&& *(tcb->ilvl[tcb->ilx].input_ptr+2) == UClit('#'))
				 { tcb->opcode = V_OpCode_MacArgA + *(tcb->ilvl[tcb->ilx].input_ptr+1)-UClit('A') ;
				   tcb->UCkeyword[0] = (tcb->UCkeyword[2] = UClit('#')) ; tcb->UCkeyword[1] = *(tcb->ilvl[tcb->ilx].input_ptr+1) ;
				   tcb->UCkeyword[3] = 0 ; tcb->prec = 30 ; tcb->ilvl[tcb->ilx].input_ptr += 3 ; tcb->keyword_len = 3 ;
				   goto type_punc ;
		   		 } else if (*(tcb->ilvl[tcb->ilx].input_ptr+1) >= UClit('A') && *(tcb->ilvl[tcb->ilx].input_ptr+1) <= UClit('Z')
					&& *(tcb->ilvl[tcb->ilx].input_ptr+2) == UClit('+') && *(tcb->ilvl[tcb->ilx].input_ptr+3) == UClit('#'))
				 { tcb->opcode = V_OpCode_IncMacArgA + *(tcb->ilvl[tcb->ilx].input_ptr+1)-UClit('A') ;
				   tcb->UCkeyword[0] = (tcb->UCkeyword[3] = UClit('#')) ; tcb->UCkeyword[2] = UClit('+') ;
				   tcb->UCkeyword[1] = *(tcb->ilvl[tcb->ilx].input_ptr+1) ;
				   tcb->UCkeyword[4] = 0 ; tcb->prec = 30 ; tcb->ilvl[tcb->ilx].input_ptr += 4 ; tcb->keyword_len = 4 ;
				   goto type_punc ;
		   		 } else if (*(tcb->ilvl[tcb->ilx].input_ptr+1) >= UClit('a') && *(tcb->ilvl[tcb->ilx].input_ptr+1) <= UClit('z')
					&& *(tcb->ilvl[tcb->ilx].input_ptr+2) == UClit('+') && *(tcb->ilvl[tcb->ilx].input_ptr+3) == UClit('#'))
				 { tcb->opcode = V_OpCode_IncMacArgA + *(tcb->ilvl[tcb->ilx].input_ptr+1)-UClit('a') ;
				   tcb->UCkeyword[0] = (tcb->UCkeyword[3] = UClit('#')) ; tcb->UCkeyword[2] = UClit('+') ;
				   tcb->UCkeyword[1] = *(tcb->ilvl[tcb->ilx].input_ptr+1) ;
				   tcb->UCkeyword[4] = 0 ; tcb->prec = 30 ; tcb->ilvl[tcb->ilx].input_ptr += 4 ; tcb->keyword_len = 4 ;
				   goto type_punc ;
		   		 } else if (*(tcb->ilvl[tcb->ilx].input_ptr+1) >= UClit('a') && *(tcb->ilvl[tcb->ilx].input_ptr+1) <= UClit('z')
					&& *(tcb->ilvl[tcb->ilx].input_ptr+2) == UClit('#'))
				 { tcb->opcode = V_OpCode_MacArgA + *(tcb->ilvl[tcb->ilx].input_ptr+1)-UClit('a') ;
				   tcb->UCkeyword[0] = (tcb->UCkeyword[2] = UClit('#')) ; tcb->UCkeyword[1] = *(tcb->ilvl[tcb->ilx].input_ptr+1) ;
				   tcb->UCkeyword[3] = 0 ; tcb->prec = 30 ; tcb->ilvl[tcb->ilx].input_ptr += 3 ; tcb->keyword_len = 3 ;
				   goto type_punc ;
				 } else { PUNC1(23,V_OpCode_Pound) ; } ;
	   case UClit('$'):	PUNC3("$$(",22,V_OpCode_DolDolLParen) ; PUNC2(UClit('('),22,V_OpCode_DollarLParen) ; PUNC1(22,V_OpCode_DollarSign) ;
	   case UClit('&'):	PUNC1(22,V_OpCode_Ampersand) ;
	   case UClit('|'):	PUNC2(UClit('|'),21,V_OpCode_LineLine) ; PUNC1(21,V_OpCode_Line) ;
	   case UClit('~'):	PUNC2(UClit('='),24,V_OpCode_TildeEqual) ;
				PUNC1(23,V_OpCode_Tilde) ;
	   case UClit('`'):	PUNC1(23,V_OpCode_BackQuote) ;
	   case UClit('?'):	PUNC2(UClit('>'),15,V_OpCode_QuestionRangle) ; PUNC1(15,V_OpCode_Question) ;
	   case UClit('@'):	PUNC1(23,V_OpCode_AtSign) ;
	   case UClit('('):	b = *(tcb->ilvl[tcb->ilx].input_ptr-1) ;	/* Prior character */
				if ( !(b>=UClit('A')&&b<=UClit('Z')) && !(b>=UClit('a')&&b<=UClit('z')) && !(b>=UClit('0')&&b<=UClit('9')))
				 { PUNC1(11,V_OpCode_NCLParen) ; } ;
				PUNC1(11,V_OpCode_LParen) ;
	   case UClit(')'):	PUNC1(12,V_OpCode_RParen) ;
	   case UClit('*'):	PUNC2(UClit('='),24,V_OpCode_StarEqual) ; PUNC2(UClit('*'),24,V_OpCode_StarStar)
				b = *(tcb->ilvl[tcb->ilx].input_ptr-1) ;	/* Prior character */
				if ( !(b>=UClit('A')&&b<=UClit('Z')) && !(b>=UClit('a')&&b<=UClit('z')) && !(b>=UClit('0')&&b<=UClit('9')) && (b!=UClit('#')))
				 { PUNC1(28,V_OpCode_NCStar) ; } ;
				PUNC1(28,V_OpCode_Star) ;
	   case UClit('+'):	PUNC2(UClit('='),15,V_OpCode_PlusEqual) ; PUNC1(27,V_OpCode_Plus) ;
	   case UClit(','):	PUNC2(UClit(','),15,V_OpCode_CommaComma) ; PUNC1(15,V_OpCode_Comma) ;
	   case UClit('-'):	PUNC2(UClit('='),15,V_OpCode_MinusEqual) ; PUNC2(UClit('>'),15,V_OpCode_DashRangle) ;
				PUNC3("-->",15,V_OpCode_DashDashRangle) ; PUNC1(27,V_OpCode_Minus) ;
	   case UClit('.'):	if (*(tcb->ilvl[tcb->ilx].input_ptr+1) >= UClit('0') && *(tcb->ilvl[tcb->ilx].input_ptr+1) <= UClit('9'))
	   			 { 
				   if (*(tcb->ilvl[tcb->ilx].input_ptr-1) <= UClit(' ') || *(tcb->ilvl[tcb->ilx].input_ptr-1) == UClit(',')
					|| *(tcb->ilvl[tcb->ilx].input_ptr-1) == UClit('*') || *(tcb->ilvl[tcb->ilx].input_ptr-1) == UClit('/')
					|| *(tcb->ilvl[tcb->ilx].input_ptr-1) == UClit('+') || *(tcb->ilvl[tcb->ilx].input_ptr-1) == UClit('-'))
				    goto numeric_lit_entry ;		/* Assume ".digit" is a numeric literal */
				 } ;
dot_entry:			PUNC3("...",15,V_OpCode_DotDotDot) ;
				PUNC2(UClit('.'),15,V_OpCode_DotDot) ;
				b = *(tcb->ilvl[tcb->ilx].input_ptr-1) ;	/* Prior character */
				if ( !(b>=UClit('A')&&b<=UClit('Z')) && !(b>=UClit('a')&&b<=UClit('z')) && !(b>=UClit('0')&&b<=UClit('9')) && b!=UClit(']') )
				 { PUNC1(18,V_OpCode_NCDot) ; } ;
				PUNC1(18,V_OpCode_Dot) ;
	   case UClit('/'):	PUNC2(UClit('*'),20,V_OpCode_SlashStar) ; PUNC2(UClit('='),15,V_OpCode_SlashEqual) ;
				PUNC2(UClit('/'),28,V_OpCode_SlashSlash) ; PUNC2(UClit('>'),15,V_OpCode_SlashRangle) ;
				if (tg_flags & V4LEX_Option_RegExp)
				 goto string_entry ;
				PUNC1(28,V_OpCode_Slash) ;
	   case UClit('\\'):	PUNC1(28,V_OpCode_BackSlash) ;
	   case UClit(':'):	PUNC3(":::",29,V_OpCode_ColonColonColon) ; PUNC2(UClit(':'),29,V_OpCode_ColonColon) ; PUNC2(UClit('='),15,V_OpCode_ColonEqual) ;
				b = *(tcb->ilvl[tcb->ilx].input_ptr-1) ;	/* Prior character */
				if ( !(b>=UClit('A')&&b<=UClit('Z')) && !(b>=UClit('a')&&b<=UClit('z')) && !(b>=UClit('0')&&b<=UClit('9')) )
				 { PUNC1(30,V_OpCode_NCColon) ; } ;
				PUNC1(30,V_OpCode_Colon) ;
	   case UClit(';'):   	PUNC1(12,V_OpCode_Semi) ;
	   case UClit('<'):	if ((tg_flags & (V4LEX_Option_XML | V4LEX_Option_HTML)) == 0) 
				 { PUNC2(UClit('>'),24,V_OpCode_LangleRangle) ; PUNC2(UClit('='),25,V_OpCode_LangleEqual) ; } ;
				PUNC2(UClit('-'),30,V_OpCode_LangleDash) ; PUNC2(UClit('/'),15,V_OpCode_LangleSlash) ; PUNC2(UClit('<'),15,V_OpCode_LangleLangle) ;
				PUNC2(UClit('?'),15,V_OpCode_LangleQuestion) ; PUNC4("<!--",15,V_OpCode_LangleBangDashDash) ;
				PUNC2(UClit('!'),15,V_OpCode_LangleBang) ; PUNC1(25,V_OpCode_Langle) ;
	   case UClit('>'):	if ((tg_flags & (V4LEX_Option_XML | V4LEX_Option_HTML)) == 0) { PUNC2(UClit('='),26,V_OpCode_RangleEqual) ; PUNC2(UClit('>'),10,V_OpCode_RangleRangle) ; } ;
				PUNC1(26,V_OpCode_Rangle) ;
	   case UClit('='):	PUNC2(UClit('='),24,V_OpCode_EqualEqual) ; PUNC2(UClit('>'),10,V_OpCode_EqualRangle) ; PUNC1(24,V_OpCode_Equal) ;
	   case UClit('['):	PUNC2(UClit(']'),11,V_OpCode_LRBracket) ;
				b = *(tcb->ilvl[tcb->ilx].input_ptr-1) ;	/* Prior character */
				if ( !(b>=UClit('A')&&b<=UClit('Z')) && !(b>=UClit('a')&&b<=UClit('z')) && !(b>=UClit('0')&&b<=UClit('9')) )
				 { PUNC1(18,V_OpCode_NCLBracket) ; } ;
				PUNC1(11,V_OpCode_LBracket) ;
	   case UClit(']'):	PUNC1(12,V_OpCode_RBracket) ;
	   case UClit('{'):	PUNC2(UClit('/'),15,V_OpCode_LBraceSlash) ;
				b = *(tcb->ilvl[tcb->ilx].input_ptr-1) ;	/* Prior character */
				if ( !(b>=UClit('A')&&b<=UClit('Z')) && !(b>=UClit('a')&&b<=UClit('z')) && !(b>=UClit('0')&&b<=UClit('9')) )
				 { PUNC1(15,V_OpCode_NCLBrace) ; } ;
				PUNC1(15,V_OpCode_LBrace) ;
	   case UClit('}'):	PUNC2(UClit('/'),15,V_OpCode_RBraceSlash) ; PUNC2(UClit('#'),15,V_OpCode_RBraceCross) ; PUNC1(15,V_OpCode_RBrace) ;
	   case UClit('%'):	PUNC1(28,V_OpCode_Percent) ;
/*	   Here on unknown VICTIM III character */
	   default:		PUNC1(0,0) ;
//VEH000330 Replaced error below with PUNC1(0,0) for UOM stuff
//	   	tcb->UCkeyword[0] = *(tcb->ilvl[tcb->ilx].input_ptr++) ; tcb->UCkeyword[1] = '\0' ;
//		v_Msg(NULL,tcb->UCstring,"TknNotStrLit",tcb->UCkeyword) ;
//	        tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ;

	 } ;
quad_punc:
	tcb->keyword_len = 4 ; tcb->ilvl[tcb->ilx].input_ptr += 4 ; goto type_punc ;
triple_punc:
	tcb->keyword_len = 3 ; tcb->ilvl[tcb->ilx].input_ptr += 3 ; goto type_punc ;
double_punc:
	tcb->keyword_len = 2 ; tcb->UCkeyword[2] = UCEOS ; tcb->ilvl[tcb->ilx].input_ptr+=2 ;	/* Advance pointer */
	goto type_punc ;
single_punc:
	tcb->keyword_len = 1 ; tcb->UCkeyword[1] = UCEOS ; tcb->ilvl[tcb->ilx].input_ptr++ ;		/* Advance (again) */
type_punc:
	if ((tg_flags & V4LEX_Option_ExpandArgs) ? tcb->opcode >= V_OpCode_MacArg0 && tcb->opcode <= V_OpCode_MacArg9 : FALSE)
	 { i = tcb->opcode-V_OpCode_MacArg0 - 1 ; if (i==(-1)) i = 9 ;
	   if (!tcb->ilvl[tcb->ilx].BoundMacroArgs)
	    { if (!v4lex_BindArgs(tcb))			/* Bind arguments if necessary */
	       { tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
	    } ;
	   if (tcb->ilvl[tcb->ilx].macarg[i].value == NULL)
	    { v_Msg(NULL,tcb->UCstring,"TknMacArgUndef",i+1) ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
	   v4lex_NestInput(tcb,NULL,tcb->ilvl[tcb->ilx].macarg[i].value,V4LEX_InpMode_MacArg) ;
	   goto top_of_module ;						/* Push argument value & re-parse */
	 } ;
	if (tg_flags & (V4LEX_Option_ForceAsIs | V4LEX_Option_ForceAsIsULC))
	 { UCstrcpy(tcb->UCstring,tcb->UCkeyword) ;
	 } else
	 { if (tcb->opcode >= V_OpCode_MacArg0 && tcb->opcode <= V_OpCode_MacArg9)
	    { i = tcb->opcode-V_OpCode_MacArg0 - 1 ; if (i==(-1)) i = 9 ;
	      if (!tcb->ilvl[tcb->ilx].BoundMacroArgs)
	       { if (!v4lex_BindArgs(tcb))				/* Bind arguments if necessary */
		  { tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
	       } ;
	      if (tcb->ilvl[tcb->ilx].macarg[i].value == NULL)
	       { v_Msg(NULL,tcb->UCstring,"TknMacArgUndef",i+1) ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
	      v4lex_NestInput(tcb,NULL,tcb->ilvl[tcb->ilx].macarg[i].value,V4LEX_InpMode_MacArg) ;
	      goto top_of_module ;						/* Push argument value & re-parse */
	    } ;
	   if (tcb->opcode >= V_OpCode_MacArgA && tcb->opcode <= V_OpCode_MacArgZ)
	    { i = tcb->opcode - V_OpCode_MacArgA ;
	      if (tcb->poundparam[i].numvalue == V4LEX_Tkn_PPNumUndef && tcb->poundparam[i].value[0] == 0)
	       { v_Msg(NULL,tcb->UCstring,"TknParamUndef",i) ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
	      if (tcb->poundparam[i].numvalue != V4LEX_Tkn_PPNumUndef)
	       { UCsprintf(tcb->poundparam[i].value,15,UClit("%d"),tcb->poundparam[i].numvalue) ; } ;
	      v4lex_NestInput(tcb,NULL,tcb->poundparam[i].value,V4LEX_InpMode_MacArg) ;
	      goto top_of_module ;
	    } ;
	   if (tcb->opcode >= V_OpCode_IncMacArgA && tcb->opcode <= V_OpCode_IncMacArgZ)
	    { i = tcb->opcode - V_OpCode_IncMacArgA ;
	      if (tcb->poundparam[i].numvalue == V4LEX_Tkn_PPNumUndef)
	       { v_Msg(NULL,tcb->UCstring,"TknParamNotSet",i) ; tcb->type = V4LEX_TknType_Error ; return(rescan_ptr) ; } ;
	      UCsprintf(tcb->poundparam[i].value,15,UClit("%d"),tcb->poundparam[i].numvalue++) ;
	      v4lex_NestInput(tcb,NULL,tcb->poundparam[i].value,V4LEX_InpMode_MacArg) ;
	      goto top_of_module ;
	    } ;
	 } ;
	tcb->type = V4LEX_TknType_Punc ;
/*	Check for start of slash-star comment */
//	if (UCstrcmp(tcb->UCkeyword,UClit("/*")) == 0)
	if (tcb->opcode == V_OpCode_SlashStar)
	 { tcb->in_comment = TRUE ; goto top_of_module ; } ;
/*	Check for Java(script) slash-slash comment til-end-of-line */
	if ((tg_flags & V4LEX_Option_Java) != 0 && tcb->opcode == V_OpCode_SlashSlash)
	 { tcb->need_input_line = TRUE ; goto top_of_module ; } ;

	return(rescan_ptr) ;
}


/*	v4lex_ReadNextLine - Read next line of input		*/
/*	Call: result = v4lex_ReadNextLine( tcb , options , search , comment )
	  where result is TRUE if got next line, FALSE if not,
		(tcb->type == V4LEX_TknType_EOL if end-of-file,
		 tcb->type == V4LEX_TknType_Error if error with msg in tcb->UCstring) */

LOGICAL v4lex_ReadNextLine(tcb,tpo,search,comment)
  struct V4LEX__TknCtrlBlk *tcb ;		/* Token Control Block */
  struct V4LEX__TablePrsOpt *tpo ;		/* Processing options */
  UCCHAR *search ;				/* If not NULL then read until get </search> */
  UCCHAR *comment ;				/* If not NULL then ignore any lines beginning with this string */
{ struct V_COMPRESS_IncludeInfo *cii ;
  struct VFC__LZRW1Hdr lhdr ;
  enum DictionaryEntries deval ;
//  static struct V4C__ProcessInfo *pi=NULL ;
  int len,i,bc ; UCCHAR *b ;
#ifdef WINNT
  FILETIME ftCreate,ftExit,ftKernel1,ftKernel2,ftUser2 ;
  static FILETIME ftUser1 ;
  SYSTEMTIME systime ;
#endif
  static time_t time_of_day ; time_t timer ; int wallseconds ;
  static clock_t cpu_start ; int cpuseconds ; static COUNTER oldbindcount ;
  UCCHAR mbuf[V4LEX_Tkn_InpLineMax+30],tbuf[V4LEX_Tkn_InpLineMax+30],lbuf[V4LEX_Tkn_InpLineMax+30],msg[100] ;
  char *bp ; UCCHAR *b1,*b2 ; UCCHAR *macroname ; int retcode,mode,searchdone,ok ;
	if (cpu_start == 0)			/* Init clocks on first pass */
	 { time(&time_of_day) ; cpu_start = clock() ;
#ifdef WINNT
	   GetProcessTimes(GetCurrentProcess(),&ftCreate,&ftExit,&ftKernel1,&ftUser1) ;
#endif
	 } ;

	for (;;)
	 { tcb->ilvl[tcb->ilx].current_line++ ; tcb->ilvl[tcb->ilx].total_lines++ ; tcb->tcb_lines ++ ;
	   if (tpo == NULL) tpo = tcb->ilvl[tcb->ilx].tpo ;
	   tcb->need_input_line = FALSE ; tcb->have_lookahead = FALSE ;
	   switch (mode=tcb->ilvl[tcb->ilx].mode)
	    { default: v4_error(V4E_INVTKNLVLMODE,tcb,"VCOM","ReadNextLine","INVTKNLVLMODE","Invalid token level (%d) mode (%d)",tcb->ilx,tcb->ilvl[tcb->ilx].mode) ;
	      case V4LEX_InpMode_Error:
		tcb->type = V4LEX_TknType_Error ; return(FALSE) ;
	      case V4LEX_InpMode_RetEOF:
		tcb->type = V4LEX_TknType_EOL ; return(FALSE) ;
	      case V4LEX_InpMode_Stdin:
/*		Should we be grabbing 'stdin' from websocket (i.e. user response from web client) ? */
		if (gpi->lIdDebug > 0)
		 { vout_UCText(VOUT_Trace,0,UClit("WebDebug>")) ;
		   UCstrcpy(tcb->ilvl[tcb->ilx].input_str,v4im_MessageNextMessageText()) ;
		   vout_UCText(VOUT_Trace,0,tcb->ilvl[tcb->ilx].input_str) ; vout_NL(VOUT_Trace) ;
		   break ;
		 } ;
//		if (pi == NULL) pi = v_GetProcessInfo() ;
		if (tcb->AutoExitEOF)
		 { v_Msg(NULL,tbuf,"*V4EOFExit") ; vout_UCText(VOUT_Status,0,tbuf) ;
		   gpi->ErrNum = V3E_EOF ;		/* Set this to fake a "^Z" (or "^D") */
		   longjmp(gpi->environment,1) ;		/* Exit to god-knows-where */
		 } ;
		if (tcb->ilvl[tcb->ilx].echoHTML)
		 { static int firstTime = TRUE ;
		   if (firstTime) { firstTime = FALSE ; } else { vout_Text(VOUT_Data,0,"</span>" TXTEOL) ; } ;
		   UCstrcpy(tbuf,UClit("<span class='v4prompt'>")) ; UCstrcat(tbuf,tcb->ilvl[tcb->ilx].prompt) ;
		   UCstrcat(tbuf,UClit("</span>") UClit("<span class='v4input'>")) ;
		 } else { UCstrcpy(tbuf,tcb->ilvl[tcb->ilx].prompt) ; } ;
		if (tcb->ilvl[tcb->ilx].input_str == NULL)		/* If no input str then must be all done */
		 { tcb->type = V4LEX_TknType_EOL ; return(FALSE) ; } ;
		if (v4lex_GetStdInput(tbuf,tcb->ilvl[tcb->ilx].input_str,V4LEX_Tkn_InpLineMax))
		 { if (tcb->ilvl[tcb->ilx].echoHTML) { vout_Text(VOUT_Data,0,"</span>" TXTEOL "<span class='v4output'>") ; } ;
		   break ;
		 } ;
/*		Hit eof on tty input - pop up level if possible */
		if (tcb->ilx <= 0)
		 { tcb->type = V4LEX_TknType_EOL ; return(FALSE) ; } ;
		v4lex_CloseOffLevel(tcb,0) ; return(TRUE) ;
	      case V4LEX_InpMode_CmpFile:
		cii = tcb->ilvl[tcb->ilx].cii ;
		for(;;)
		 { if (cii->curPtr == NULL)			/* Have to read in next chunk from compressed file */
		    { if ((len=fread(&lhdr,1,sizeof lhdr,tcb->ilvl[tcb->ilx].UCFile->fp)) < sizeof lhdr) goto end_of_file ;
		      if (lhdr.SrcBytes < 0 || lhdr.SrcBytes > 0xfffff || lhdr.CmpBytes < 0 || lhdr.CmpBytes > 0xfffff)
		       { FLIPLONG(lhdr.SrcBytes,lhdr.SrcBytes) ; FLIPLONG(lhdr.CmpBytes,lhdr.CmpBytes) ;
		       } ;
		      if (lhdr.SrcBytes < 0 || lhdr.SrcBytes > 0xfffff || lhdr.CmpBytes < 0 || lhdr.CmpBytes > 0xfffff)
		       { tcb->type = V4LEX_TknType_Error ; v_Msg(NULL,tcb->UCstring,"FileInvCmpForm",tcb->ilvl[tcb->ilx].file_name,1,0,0) ;
			 return(FALSE) ;
//			 v4_error(0,NULL,"VCOM","ReadNextLine","CMPERR","File(%s) not in valid compressed format",UCretASC(tcb->ilvl[tcb->ilx].file_name)) ;
		       } ;
		      if (cii->BufMax == 0)
		       { cii->BufMax = lhdr.SrcBytes + V_COMPRESS_MaxTextLine ;
		         cii->CmpBuf = v4mm_AllocChunk(cii->BufMax,FALSE) ; cii->Buf = v4mm_AllocChunk(cii->BufMax,FALSE) ;
		       } ;
		      len = fread(cii->CmpBuf,1,lhdr.CmpBytes,tcb->ilvl[tcb->ilx].UCFile->fp) ;
		      if (len < lhdr.CmpBytes)
		       { tcb->type = V4LEX_TknType_Error ; v_Msg(NULL,tcb->UCstring,"FileInvCmpForm",tcb->ilvl[tcb->ilx].file_name,2,len,lhdr.CmpBytes) ;
			 return(FALSE) ;
		       } ;
//		       v4_error(0,NULL,"VCOM","ReadNextLine","READERR","Error(%s) reading Include file(%s) (%d read, %d needed)",
//				v_OSErrString(errno),UCretASC(tcb->ilvl[tcb->ilx].file_name),len,lhdr.CmpBytes) ;
		      lzrw1_decompress(cii->CmpBuf,lhdr.CmpBytes,cii->Buf,&len,cii->BufMax) ;
		      if (len != lhdr.SrcBytes)
		       { tcb->type = V4LEX_TknType_Error ; v_Msg(NULL,tcb->UCstring,"FileInvCmpForm",tcb->ilvl[tcb->ilx].file_name,3,len,lhdr.SrcBytes) ;
			 return(FALSE) ;
		       } ;
//		       v4_error(0,NULL,"VCOM","ReadNextLine","READERR","Error reading Include file(%s) (expanded to %d, s/b %d bytes)",
//				UCretASC(tcb->ilvl[tcb->ilx].file_name),len,lhdr.SrcBytes) ;
		      cii->curPtr = cii->Buf ; cii->Buf[lhdr.SrcBytes] = UCEOS ;	/* Terminate buffer with null */
		    } ;
		   if (*cii->curPtr == UCEOS) { cii->curPtr = NULL ; continue ; } ;
		   bp = memchr(cii->curPtr,'\n',V4LEX_Tkn_InpLineMax) ;
		   if (bp == NULL)
		    { tcb->type = V4LEX_TknType_Error ; v_Msg(NULL,tcb->UCstring,"FileInvCmpForm",tcb->ilvl[tcb->ilx].file_name,4,0,0) ;
		      return(FALSE) ;
		    } ;
//		    v4_error(0,NULL,"VCOM","ReadNextLine","BADFILE","Compressed Include file (%s) appears malformed",UCretASC(tcb->ilvl[tcb->ilx].file_name)) ;
//		   strncpy(tcb->ilvl[tcb->ilx].input_str,ASCretUC(cii->curPtr),bp-cii->curPtr+1) ;
		   len = ((char *)bp - (char *)cii->curPtr+1) ;
		   for(i=0;i<len;i++) { tcb->ilvl[tcb->ilx].input_str[i] = cii->curPtr[i] ; } ;
		   if (tcb->ilvl[tcb->ilx].input_str[len-2] == 13)
		    { tcb->ilvl[tcb->ilx].input_str[len-2] = UCEOS ; }
		    else { tcb->ilvl[tcb->ilx].input_str[len] = UCEOS ; } ;
		   cii->curPtr = bp + 1 ;			/* Position to begin of next line */
		   goto read_file_entry ;
		 } ;
	      case V4LEX_InpMode_List:
		{ P point ;
		  ok = v4l_ListPoint_Value(gpi->ctx,tcb->ilvl[tcb->ilx].listp,++tcb->ilvl[tcb->ilx].listX,&point) ;
		  if (ok != 1 && ok != 2) return(FALSE) ;
		  tcb->ilvl[tcb->ilx].current_line++ ;
		  v4im_GetPointUC(&ok,tcb->ilvl[tcb->ilx].input_str,tcb->maxCharInpBuf,&point,gpi->ctx) ;
		  tcb->ilvl[tcb->ilx].input_ptr = tcb->ilvl[tcb->ilx].input_str ;
		}
		return(TRUE) ;
	      case V4LEX_InpMode_TempFile:
	      case V4LEX_InpMode_File:
//		if (fgets(tcb->ilvl[tcb->ilx].input_str,V4LEX_Tkn_InpLineMax,tcb->ilvl[tcb->ilx].file) == NULL) goto end_of_file ;
		if ((len = v_UCReadLine(tcb->ilvl[tcb->ilx].UCFile,UCRead_UC,tcb->ilvl[tcb->ilx].input_str,tcb->maxCharInpBuf,tcb->ilvl[tcb->ilx].input_str)) < 0) goto end_of_file ;
		for(;(tpo == NULL ? FALSE : (tpo->readLineOpts & V4LEX_TCBMacro_Continued) != 0);)
		 { int lenc ;
//		   len = UCstrlen(tcb->ilvl[tcb->ilx].input_str) ;
		   b = &tcb->ilvl[tcb->ilx].input_str[len-1] ;
//		   if (*b == UCNL || *b == UClit('\r')) { *b = UCEOS ; continue ; }	/* Get rid of trailing junk */
		   if (*b != UClit('\\')) break ;	/* Not to be continued */
//		   if (fgets(tbuf,V4LEX_Tkn_InpLineMax,tcb->ilvl[tcb->ilx].file) == NULL) ZS(tbuf) ;
		   if ((lenc = v_UCReadLine(tcb->ilvl[tcb->ilx].UCFile,UCRead_UC,tbuf,UCsizeof(tbuf),tcb->ilvl[tcb->ilx].input_str)) < 0) ZS(tbuf) ;
		   tcb->ilvl[tcb->ilx].current_line++ ;
		   if (lenc + len >= V4LEX_Tkn_InpLineMax)
		    { tcb->type = V4LEX_TknType_Error ; v_Msg(NULL,tcb->UCstring,"TknContLine",V4LEX_Tkn_InpLineMax) ;
		      return(FALSE) ;
		    } ;
//		    v4_error(0,NULL,"VCOM","ReadNextLine","BADFILE","Continuation at line(%d) in file(%s) exceeds line maximum(%d bytes)",
//				tcb->ilvl[tcb->ilx].current_line,UCretASC(tcb->ilvl[tcb->ilx].file_name),V4LEX_Tkn_InpLineMax) ;
		   *b = UCEOS ; UCstrcat(tcb->ilvl[tcb->ilx].input_str,tbuf) ; len += (lenc - 1) ;
		 } ;
read_file_entry:
		if (comment == NULL ? FALSE : UCstrncmp(tcb->ilvl[tcb->ilx].input_str,comment,UCstrlen(comment)) == 0) continue ;
		if ((tpo == NULL ? FALSE :(tpo->readLineOpts & V4LEX_ReadLine_FlushCmd) != 0))	/* Flush until "command" ? */
		 { b = tcb->ilvl[tcb->ilx].input_str ;		/*  then continue until line with letter */
		   if (!( (*b >= UClit('A') && *b <= UClit('Z')) || (*b >= UClit('a') && *b <= UClit('z')) )) continue ;
		 } ;
		if (tcb->ilvl[tcb->ilx].echo >= V4LEX_EchoEveryStartNum
			&& (tcb->ilvl[tcb->ilx].current_line % tcb->ilvl[tcb->ilx].echo) == 0)
		 { wallseconds = time(NULL) -  time_of_day ; cpuseconds = (clock() - cpu_start) / CLOCKS_PER_SEC ;
#ifdef WINNT
		   GetProcessTimes(GetCurrentProcess(),&ftCreate,&ftExit,&ftKernel2,&ftUser2) ;
		   FileTimeToSystemTime(&ftUser1,&systime) ;
		   cpu_start = systime.wMinute*60 + systime.wSecond + (systime.wMilliseconds+500)/1000 ;
		   FileTimeToSystemTime(&ftUser2,&systime) ;
		   cpuseconds = systime.wMinute*60 + systime.wSecond + (systime.wMilliseconds+500)/1000 ;
		   cpuseconds -= cpu_start ;
#endif
		   time(&timer) ;
		   bc = gpi->BindingCount ;
		   { UCCHAR sbuf[128] ;
		     UCsprintf(sbuf,UCsizeof(sbuf),UClit("\n%.19s - Line: %d, Elapsed: %d, Delta CPU: %d, Binds: %d (%d total)\n"),
				ASCretUC(ctime(&timer)),tcb->ilvl[tcb->ilx].current_line,wallseconds,cpuseconds,
				bc - oldbindcount,bc) ;
		     vout_UCText(VOUT_Trace,0,sbuf) ;
		     vout_UCText(VOUT_Trace,0,tcb->ilvl[tcb->ilx].input_str) ;
		   }
		   time_of_day = time(NULL) ; cpu_start = clock() ;
		   oldbindcount = gpi->BindingCount ;
#ifdef WINNT
		   ftUser1 = ftUser2 ;
#endif
		 } ;
filter_retry:
		if (tpo == NULL ? FALSE : tpo->filterPt != NULL)			/* If have filter then put line in ctx as Alpha* & call filter */
		 { struct V4C__Context *ctx ;
		   struct V4DPI__LittlePoint retry ;
		   struct V4DPI__Point apnt ; struct V4DPI__Point *ipt, respnt ;
		   ctx = gpi->ctx ;
		   CHECKV4INIT ;
		   ZPH(&apnt) ; apnt.Dim = Dim_Alpha ; apnt.PntType = V4DPI_PntType_UCChar ;
		   ZPH(&retry) ; retry.Dim = Dim_UV4 ; retry.PntType = V4DPI_PntType_Dict ;
//		   retry.Bytes = V4PS_Int ; retry.Value.IntVal = v4dpi_DictEntryGet(ctx,UNUSED,"Retry",NULL,NULL) ;
		   retry.Bytes = V4PS_Int ; retry.Value.IntVal = v4im_GetEnumToDictVal(ctx,deval=_Retry,UNUSED) ;
		   len = UCstrlen(tcb->ilvl[tcb->ilx].input_str) ;
		   if (len >= V4DPI_UCVAL_MaxSafe)
		    { tcb->type = V4LEX_TknType_Error ; v_Msg(NULL,tcb->UCstring,"TknFltrStr",len,V4DPI_UCVAL_MaxSafe) ;
		      return(FALSE) ;
		    } ;
//		    v4_error(0,NULL,"VCOM","ReadNextLine","LINETOOLONG","Length (%d) of line (%d) of file (%s) exceeds V4 Alpha point maximum (%d)",
//				len,tcb->ilvl[tcb->ilx].current_line,UCretASC(tcb->ilvl[tcb->ilx].file_name),V4DPI_AlphaVal_Max) ;
		   UCstrcpy(&apnt.Value.UCVal[1],tcb->ilvl[tcb->ilx].input_str) ;
		   UCCHARPNTBYTES1(&apnt)
//sss		   if (len < 255) { apnt.Value.AlphaVal[0] = len ; apnt.Bytes = ALIGN(V4DPI_PointHdr_Bytes + len + 1) ; }
//		    else { apnt.Value.AlphaVal[0] = 255 ; apnt.Bytes = ALIGN(V4DPI_PointHdr_Bytes + len + 2) ; } ;
		   v4ctx_FrameAddDim(ctx,0,&apnt,0,0) ;		/* Add current line to ctx as Dim:Alpha point */
		   ipt = v4dpi_IsctEval(&respnt,tpo->filterPt,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
		   if (ipt == NULL)
		    { UCstrcpy(ctx->ErrorMsgAux,ctx->ErrorMsg) ;
		      tcb->type = V4LEX_TknType_Error ; v_Msg(NULL,tcb->UCstring,"TknFltrIsct",tpo->filterPt) ;
		      return(FALSE) ;
		    } ;
//		    v4_error(0,NULL,"VCOM","ReadNextLine","FLTRERR","Filter processing failed on line (%d) file (%s)",
//				tcb->ilvl[tcb->ilx].current_line,UCretASC(tcb->ilvl[tcb->ilx].file_name)) ;
/*		   Look at returned value to see what to do */
		   switch (ipt->PntType)
		    { default:
			if (!v4im_GetPointLog(&ok,ipt,ctx)) continue ;	/* If TRUE not returned then skip this line */
			DIMVAL(ipt,ctx,apnt.Dim) ;				/* Get Alpha* and plug as current input line */
			v4im_GetPointUC(&len,tcb->ilvl[tcb->ilx].input_str,V4LEX_Tkn_InpLineMax,ipt,ctx) ;
			len = UCstrlen(tcb->ilvl[tcb->ilx].input_str) ;
			break ;
		      case V4DPI_PntType_Dict:					/* Check for UV4:Retry point */
			if (memcmp(ipt,&retry,retry.Bytes) == 0) goto filter_retry ;
		      case V4DPI_PntType_List:					/* A list - push multiple lines back onto input stream */
			{ struct V4DPI__Point listpnt ;
		          struct V4L__ListPoint *lp ; int i ;
			  UCCHAR mlbuf[V4LEX_Tkn_InpLineMax] ;
			  lp = ALIGNLP(&ipt->Value) ; ZUS(mlbuf) ;
			  for(i=1;;i++)
			   { if (v4l_ListPoint_Value(ctx,lp,i,&listpnt) <= 0) break ;
			     v4im_GetPointUC(&len,tcb->ilvl[tcb->ilx].input_str,V4LEX_Tkn_InpLineMax,&listpnt,ctx) ;
			     if (UCstrlen(mlbuf) + UCstrlen(tcb->ilvl[tcb->ilx].input_str) >= UCsizeof(mlbuf))
			      { tcb->type = V4LEX_TknType_Error ; v_Msg(NULL,tcb->UCstring,"TknFltrList",ipt) ;
			        return(FALSE) ;
			      } ;
//			     v4_error(0,NULL,"VCOM","ReadNextLine","FLTRERR","Filter processing failed on line (%d) file (%s) - resulting list of lines exceeds max allowed",
//					tcb->ilvl[tcb->ilx].current_line,UCretASC(tcb->ilvl[tcb->ilx].file_name)) ;
			     UCstrcat(mlbuf,tcb->ilvl[tcb->ilx].input_str) ; UCstrcat(mlbuf,UClit("\032")) ;
			   } ;
			  v4lex_NestInput(tcb,NULL,mlbuf,V4LEX_InpMode_StringML) ;
			  continue ;
			}
		      CASEofChar
//		      case V4DPI_PntType_BigText:
			v4im_GetPointUC(&len,tcb->ilvl[tcb->ilx].input_str,V4LEX_Tkn_InpLineMax,ipt,ctx) ;
			len = UCstrlen(tcb->ilvl[tcb->ilx].input_str) ;
			break ;
		    } ;
		 } ;
/*		If we are parsing a table then handle here (but don't try to parse if we are already parsing a table!) */
		if (tcb->ilvl[tcb->ilx].vlt != NULL && ((tpo->readLineOpts & V4LEX_ReadLine_NestTable) == 0))
		 { if (tcb->ilvl[tcb->ilx].current_line <= tpo->HeaderLines) continue ;
		   if (tpo->MinLength != UNUSED ? UCstrlen(tbuf) < tpo->MinLength : FALSE) continue ;
		   if (tpo->Comment[0] == UCEOS ? FALSE
				: UCstrchrV(tpo->Comment,tcb->ilvl[tcb->ilx].input_str[0]) != NULL)
		    continue ;					/* Skip if line begins with comment character */
		   b = tcb->ilvl[tcb->ilx].input_str + UCstrlen(tcb->ilvl[tcb->ilx].input_str) - 1 ;
		   if (*b == UCNL || *b == UClit('\r')) *b = UCEOS ;		/* Get rid of trailing junk */
		   if (tpo->MinLength != UNUSED)
		    { if (*tcb->ilvl[tcb->ilx].input_str == UCNL || *tcb->ilvl[tcb->ilx].input_str == UCEOS) continue ; } ;		/* Ignore blank lines ? */
		   if ((retcode=v4lex_ReadNextLine_Table(tcb->ilvl[tcb->ilx].input_str,UNUSED,mbuf,lbuf,msg,tcb->ilvl[tcb->ilx].vlt,&macroname,0,tcb,NULL,UNUSED)) < 0)
		    { v_Msg(NULL,tbuf,"@*(%1U) in %2U line %3d column %4d (%5U) field *[%6U]*\n",msg,tcb->ilvl[tcb->ilx].file_name,
					tcb->ilvl[tcb->ilx].current_line,-retcode,lbuf,mbuf) ;
		      vout_UCText(VOUT_Err,0,tbuf) ; gpi->ErrCount++ ;
		      continue ;
		    } ;
//		   if (tcb->ilvl[tcb->ilx].vlt->ctx->OptScanForDict) continue ;	/* Don't process macro - go onto next line */
		   if (macroname != NULL)
		    { UCstrcpy(tcb->ilvl[tcb->ilx].input_str,macroname) ; }
		    else { if (tpo->Macro[0] == UCEOS)
			    { v_Msg(NULL,tbuf,"*TblNoMacro",tcb->ilvl[tcb->ilx].file_name,tcb->ilvl[tcb->ilx].current_line) ;
			      vout_UCText(VOUT_Err,0,tbuf) ; gpi->ErrCount++ ;
			      continue ;
			    } ;
			   UCstrcpy(tcb->ilvl[tcb->ilx].input_str,(macroname != NULL ? macroname : tpo->Macro)) ;
			 } ;
		   UCstrcat(tcb->ilvl[tcb->ilx].input_str,UClit("()\n")) ;
		 } ;
		if (tpo == NULL ? FALSE : tpo->MinLength != UNUSED)
		 { if (*tcb->ilvl[tcb->ilx].input_str == UCNL || *tcb->ilvl[tcb->ilx].input_str == UCEOS) continue ; } ;		/* Ignore blank lines ? */
		break ;
/*		End of file on input - close off & pop up level */
end_of_file:
		if (tcb->ilvl[tcb->ilx].vlt != NULL) { tcb->ilvl[tcb->ilx].vlt->ctx->pi->vltCur = NULL ; } ;
		if (tcb->ifx > 0 ? (tcb->ifs[tcb->ifx-1].ifxID != tcb->ilvl[tcb->ilx].ifxID_on_entry) : FALSE)
		 { int saveifx = tcb->ifx ; tcb->ifx = 0 ;
		   if (tcb->appendto != NULL)		/* In midst of LogToIsct ? */
		    { v4mm_FreeChunk(tcb->appendto) ; tcb->appendto = NULL ; tcb->appendopt = 0 ; } ;
		   tcb->type = V4LEX_TknType_Error ; v_Msg(NULL,tcb->UCstring,"ReadNLEOSIf",tcb->ifs[saveifx-1].name) ;
		   return(FALSE) ;
		 } ;
		if (tcb->cpmStartLine != 0)
		 { v_Msg(NULL,tcb->UCstring,"CmdMacroEOF",tcb->cpmName,tcb->cpmStartLine) ;
		   v4lex_CloseOffLevel(tcb,0) ;
		   v4_UCerror(V3E_EOF,tcb,"TKN","Next","preEOF",tcb->UCstring) ;		/* This is a bad error- want to quit out of V4 ASAP */
//		    tcb->type = V4LEX_TknType_Error ; return(FALSE) ;
		 } ;
/*		Check to see if comment not finished or in wrong level */
		if (search != NULL)
	         { tcb->ilvl[tcb->ilx].input_ptr = tcb->ilvl[tcb->ilx].input_str ;
	           tcb->type = V4LEX_TknType_Error ; v_Msg(NULL,tcb->UCstring,"@Reached end of source file while searching for %1U",search) ; return(FALSE) ;
	         } ;
		v4lex_CloseOffLevel(tcb,0) ; tpo = NULL ;




		if (tcb->in_comment)
	         { tcb->in_comment = FALSE ; tcb->ilvl[tcb->ilx].input_ptr = tcb->ilvl[tcb->ilx].input_str ;
	           tcb->type = V4LEX_TknType_Error ; v_Msg(NULL,tcb->UCstring,"@Reached end of source file while in \'/* xxx */\' comment") ; return(FALSE) ;
	         } ;
		if (tcb->ilvl[tcb->ilx].input_ptr == NULL) continue ;
		return(TRUE) ;
	      case V4LEX_InpMode_StringML:
		tcb->ilvl[tcb->ilx].current_line = (tcb->ilvl[tcb->ilx].statement_start_line = 0) ;	/* These are meaningless within this source of input */
		if (tcb->ilvl[tcb->ilx].strptr == NULL ? TRUE : *tcb->ilvl[tcb->ilx].strptr == UCEOS)
		 { if (tcb->ifx > 0 ? (tcb->ifs[tcb->ifx-1].ifxID != tcb->ilvl[tcb->ilx].ifxID_on_entry) : FALSE)
		    { i = tcb->ifx - 1 ; tcb->ifx = 0 ;
		      tcb->type = V4LEX_TknType_Error ; v_Msg(NULL,tcb->UCstring,"@Reached end of macro while still in IF:%1U statement",tcb->ifs[i].name) ;
		      return(FALSE) ;
		    } ;
		   tcb->ilx-- ; tpo = NULL ;
		   continue ;
		 } ;
		for(;;)
		 { b1 = UCstrchr(tcb->ilvl[tcb->ilx].strptr,'\032') ;
		   if (b1 == NULL) { tcb->ilx -- ; tpo = NULL ; goto continue_main ; } ;		/* Multi-line not well formed - ignore it */
		   *b1 = UCEOS ; UCstrcpy(tcb->ilvl[tcb->ilx].input_str,tcb->ilvl[tcb->ilx].strptr) ;
		   tcb->ilvl[tcb->ilx].strptr = b1 + 1 ;
		   if (UCnotempty(tcb->ilvl[tcb->ilx].input_str)) break ;
		 } ;
		if (tcb->ilvl[tcb->ilx].vlt != NULL)			/* Need separate vlt handling to work with input_ptr, not input_str */
		 { 
		   if (tpo->Comment[0] == UCEOS ? FALSE
				: UCstrchrV(tpo->Comment,tcb->ilvl[tcb->ilx].input_ptr[0]) != NULL)
		    continue ;					/* Skip if line begins with comment character */
		   b = tcb->ilvl[tcb->ilx].input_ptr + UCstrlen(tcb->ilvl[tcb->ilx].input_ptr) - 1 ;
		   if (*b == UCNL || *b == UClit('\r')) *b = UCEOS ;		/* Get rid of trailing junk */
		   if (tpo->MinLength != UNUSED)
		    { if (*tcb->ilvl[tcb->ilx].input_ptr == '\n' || *tcb->ilvl[tcb->ilx].input_ptr == UCEOS) continue ; } ;		/* Ignore blank lines ? */
		   if ((retcode=v4lex_ReadNextLine_Table(tcb->ilvl[tcb->ilx].input_ptr,UNUSED,mbuf,lbuf,msg,tcb->ilvl[tcb->ilx].vlt,&macroname,0,tcb,NULL,UNUSED))<0)
		    { v_Msg(NULL,tbuf,"@*(%1U) in %2U line %3d column %4d (%5s) field *[%6U]*\n",msg,tcb->ilvl[tcb->ilx].file_name,
					tcb->ilvl[tcb->ilx].current_line,-retcode,lbuf,mbuf) ;
		      vout_UCText(VOUT_Err,0,tbuf) ; gpi->ErrCount++ ;
		      continue ;
		    } ;
//		   if (tcb->ilvl[tcb->ilx].vlt->ctx->OptScanForDict) continue ;	/* Don't process macro - go onto next line */
		   if (macroname != NULL)
		    { UCstrcpy(tcb->ilvl[tcb->ilx].arglist,macroname) ; }
		    else { if (tpo->Macro[0] == UCEOS)
			    { v_Msg(NULL,tbuf,"*TblNoMacro",tcb->ilvl[tcb->ilx].file_name,tcb->ilvl[tcb->ilx].current_line) ;
			      vout_UCText(VOUT_Err,0,tbuf) ; gpi->ErrCount++ ;
			      continue ;
			    } ;
			   UCstrcpy(tcb->ilvl[tcb->ilx].arglist,(macroname != NULL ? macroname : tpo->Macro)) ;
			 } ;
		   UCstrcat(tcb->ilvl[tcb->ilx].arglist,UClit("()\n")) ;
		   tcb->ilvl[tcb->ilx].input_ptr = tcb->ilvl[tcb->ilx].arglist ;
///* VERY BAD TEMP CODE AS INTERMEDIATE  (just delete these when convert done, line above will then be valid) */
//tcb->ilvl[tcb->ilx].input_ptr = (char *)v4mm_AllocChunk(UCstrlen(tcb->ilvl[tcb->ilx].arglist)+1,FALSE) ;
//UCstrcpyToASC(tcb->ilvl[tcb->ilx].input_ptr,tcb->ilvl[tcb->ilx].arglist) ;
		   return(TRUE) ;			/* Want to return immediately without changing pointers */
		 } ;
		break ;
	      case V4LEX_InpMode_MacArg:
	      case V4LEX_InpMode_String:
	      case V4LEX_InpMode_StringPtr:
		tcb->ilvl[tcb->ilx].current_line = (tcb->ilvl[tcb->ilx].statement_start_line = 0) ;	/* These are meaningless within this source of input */
		if (tpo == NULL ? FALSE : ((tpo->readLineOpts & V4LEX_ReadLine_FlushCmd) != 0))		/* Flush until "command" ? */
		 { v4lex_CloseOffLevel(tcb,0) ; tpo = NULL ; continue ; } ;		/*  then ignore this */
		if (tcb->ilvl[tcb->ilx].strptr == NULL)
		 { 
//printf("ifx=%d, ilx=%d, ifxID=%d, onentry=%d\n",tcb->ifx,tcb->ilx,tcb->ifs[tcb->ifx-1].ifxID,tcb->ilvl[tcb->ilx].ifxID_on_entry) ;
		   if (tcb->ifx > 0 ? (tcb->ifs[tcb->ifx-1].ifxID != tcb->ilvl[tcb->ilx].ifxID_on_entry) : FALSE)
		    { i = tcb->ifx - 1 ; tcb->ifx = 0 ;
		      tcb->type = V4LEX_TknType_Error ; v_Msg(NULL,tcb->UCstring,"@Reached end of macro while still in IF:%1U statement",tcb->ifs[i].name) ; return(FALSE) ;
		    } ;
//		   v4lex_CloseOffLevel(tcb,0) ;
		   tcb->ilx-- ;	/* CloseOffLevel also sets "NeedNewLine" which is bad!!! */
		   tpo = NULL ;
		   if (tcb->ilvl[tcb->ilx].input_ptr == NULL) continue ;
		   if (tcb->appendto != NULL && ((tcb->appendopt & V4LEX_AppendTo_LogToIsct) == 0)) continue ;	/* VEH051019 - if still in BigText then continue */
		   return(TRUE) ;
		 } ;
		tcb->ilvl[tcb->ilx].strptr = NULL ;
		break ;
	    } ;
	   tcb->ilvl[tcb->ilx].input_ptr = tcb->ilvl[tcb->ilx].input_str ;
	   len = UCstrlen(tcb->ilvl[tcb->ilx].input_str) ;
/*	   If input ends with CR/LF then tweak to standard '\n' (i.e. just LF) */
	   if (tcb->ilvl[tcb->ilx].input_str[len-1] == UClit('\012') && tcb->ilvl[tcb->ilx].input_str[len-2] == UClit('\015'))
	    { tcb->ilvl[tcb->ilx].input_str[len-2] = UCNL ; tcb->ilvl[tcb->ilx].input_str[len-1] = UCEOS ; } ;
	   if (search != NULL)					/* Look for search string */
	    { int i ;
	      searchdone = FALSE ;
	      for(i=0;i<=sizeof lbuf;i++) { lbuf[i] = UCTOUPPER(tcb->ilvl[tcb->ilx].input_str[i]) ; if (lbuf[i] == UCEOS) break ; } ;
	      if ((b1=UCstrstr(lbuf,search)) != NULL)	/* Did we find ending </xxx> ? If so then advance point past it */
	       { len=(b1-lbuf) ;
	         tcb->ilvl[tcb->ilx].input_str[len] = UCEOS ;
	         tcb->ilvl[tcb->ilx].input_ptr = &tcb->ilvl[tcb->ilx].input_str[len+UCstrlen(search)] ;
	         searchdone = TRUE ;
	       } ;
	    } ;
/*	   If AppendTo set & not expanding a macro then append if enough room */
	   if (tcb->appendto != NULL && !(mode == V4LEX_InpMode_String || mode == V4LEX_InpMode_MacArg))
	    { if (len + tcb->bytesappend >= tcb->maxappend-1)
	       { tcb->appendto[30] = UCEOS ; UCstrcpy(mbuf,tcb->appendto) ;
		 tcb->appendto = NULL ;
	         tcb->type = V4LEX_TknType_Error ; v_Msg(NULL,tcb->UCstring,"@Exceeded max bytes(%1d) in BigText entry (%2U...)",tcb->maxappend,mbuf) ; return(FALSE) ;
	       } ;
	      b1=tcb->ilvl[tcb->ilx].input_str ;
	      for(b2=b1+UCstrlen(b1)-1;*b2<='\r'&&b2!=b1-1;len--,b2--) { *b2 = UCEOS ; } ;
	      if ((tpo == NULL ? FALSE : (tpo->readLineOpts & V4LEX_ReadLine_MTLTrash) != 0))	/* Ignore blank lines */
	       { if (UCempty(b1))
	          { if (search != NULL && searchdone) return(TRUE) ;
		    continue ;
		  } ;
	       } ;
	      if (tpo != NULL)
	       { if ((tpo->readLineOpts & V4LEX_ReadLine_IndentNone) != 0)	/* Strip leading white space? */
		  { for(;;b1++) { if (*b1 != UClit(' ') && *b1 != UClit('\t')) break ; } ;
		  } else if ((tpo->readLineOpts & V4LEX_ReadLine_IndentBlank) != 0)
		     { for(;;b1++) { if (*b1 != UClit(' ') && *b1 != UClit('\t')) break ; } ;
		       if (UCstrlen(tcb->appendto) > 0) UCstrcat(tcb->appendto,UClit(" ")) ;
		  } else if ((tpo->readLineOpts & V4LEX_ReadLine_IndentTab) != 0)
		     { if (*b1 == UClit('\t')) b1++ ;
		  } ;
	       } ;
	      if ((tpo == NULL ? FALSE : ((tpo->readLineOpts & V4LEX_ReadLine_NLAsIs) != 0 && !searchdone)) || (tcb->appendopt & V4LEX_AppendTo_LineTerm))
	       { UCstrcat(b1,UCEOLbts) ; } ;				/* Do we want end-of-line ? */
	      if (tpo != NULL ? ((tpo->readLineOpts & V4LEX_ReadLine_Javascript) != 0) : FALSE)
	       { UCCHAR *src,*dst ;
		 for(;;b1++) { if (!vuc_IsWSpace(*b1)) break ; } ;	/* Get rid of leading white-space */
	         for(b2=b1;*b2!=UCEOS;b2++)
		  { if (*b2 == UClit('/') && *(b2+1) == UClit('/') && (b2 == b1 || vuc_IsWSpace(*(b2-1)))) *b2 = UCEOS ; } ;	/* Ignore after //comment */
		 for(src=b1+1,dst=b1+1;*src!=UCEOS;src++,dst++)
		  { if (*src == UClit(';')) { if (*(dst-1) == UClit(' ')) dst-- ; } ;
		    if (*src == UClit('{')) { if (*(dst-1) == UClit(' ')) dst-- ; } ;
		    if (*src == UClit('=')) { if (*(dst-1) == UClit(' ')) dst-- ;  if (*(src+1) == UClit(' ')) { src++ ; *dst = UClit('=') ; continue ; } ; } ;
		    if (src != dst) *dst = *src ;
		    if (*src == UCEOS) break ;
		  } ; *dst = UCEOS ;
		 b2 = &b1[UCstrlen(b1)-1] ;				/* b2 = last character in line */
		 for(;*b2<26 && b2!=b1;b2--) {} ;			/* Skip left over end-of-line */
		 if (*b2 == UClit(';') || *b2 == UClit(',') || *b2 == UClit('{'))
		  *(b2+1) = UCEOS ;					/* If ';' last character in line then continue next line with this one */
	       } ;
	      UCstrcat(tcb->appendto,b1) ;
	      tcb->bytesappend = UCstrlen(tcb->appendto) ;
	      if (search != NULL && searchdone) return(TRUE) ;
	    } ;
	   if (search != NULL) continue ;				/* Have not found what we want yet! */
	   if (mode != V4LEX_InpMode_MacArg)				/* If not macro arg then terminate with NL */
	    { if (tcb->ilvl[tcb->ilx].input_str[len-1] != UCNL && (tcb->ilvl[tcb->ilx].UCFile != NULL ? tcb->ilvl[tcb->ilx].UCFile->wantEOL : TRUE))
	       { tcb->ilvl[tcb->ilx].input_str[len] = UCNL ; tcb->ilvl[tcb->ilx].input_str[len+1] = 0 ; } ;
	    } ;
	   if (tcb->ilvl[tcb->ilx].echo > 0 && tcb->ilvl[tcb->ilx].echo < V4LEX_EchoEveryStartNum && !tcb->ilvl[tcb->ilx].echoDisable)
	    { 
	      if (tcb->ilvl[tcb->ilx].echoHTML)
	       { static int firstTime = TRUE ;
	         if (firstTime) { firstTime = FALSE ; } else { vout_UCText(VOUT_Data,0,UClit("</span>") UCTXTEOL) ; } ;
//	         ZUS(tbuf) ; UCstrcat(tbuf,tcb->ilvl[tcb->ilx].prompt) ; UCstrcat(tbuf,UClit("")) ;
	         v_Msg(NULL,tbuf,"@<span class='v4prompt'>%1d. </span><span class='v4input'>%2U</span>" TXTEOL "<span class='v4output'>",
			tcb->ilvl[tcb->ilx].current_line,tcb->ilvl[tcb->ilx].input_str) ;
		 vout_UCText(VOUT_Trace,0,tbuf) ;
	       } else
	       { v_Msg(NULL,tbuf,"@%1d. %2u",tcb->ilvl[tcb->ilx].current_line,tcb->ilvl[tcb->ilx].input_str) ;
	         len = UCstrlen(tbuf) ; vout_UCText(VOUT_Trace,len,tbuf) ;
	         if (tbuf[len-1] != UCNL) vout_NL(VOUT_Trace) ;
//	         if (UCstrlen(tbuf) >= UCsizeof(tbuf) - 1) vout_NL(VOUT_Trace) ;
	       } ;
	    } ;
/*	   Any line starting with '!' is comment EXCEPT '!{/name xxx' and '}/name' lines */
	   if (*tcb->ilvl[tcb->ilx].input_ptr == UClit('!') && (tpo == NULL ? TRUE : ((tpo->readLineOpts & V4LEX_ReadLine_Bang) == 0)))
	    { if (*(tcb->ilvl[tcb->ilx].input_ptr+1) != UClit('{') && *(tcb->ilvl[tcb->ilx].input_ptr+1) != UClit('}')) continue ;
	      tcb->ilvl[tcb->ilx].input_ptr ++ ;	/* Skip over the UClit('!') */
	    } ;
	   if (tcb->ilvl[tcb->ilx].statement_start_line == 0)
	    tcb->ilvl[tcb->ilx].statement_start_line = tcb->ilvl[tcb->ilx].current_line ;
	   for(b=tcb->ilvl[tcb->ilx].input_ptr,i=1;;b++)		/* Determine amount of indentation for line */
	    { if (*b == UClit(' ')) { i ++ ; continue ; } ;
	      if (*b == UClit('\t')) { i = ((i+7)&0xFFFF8) ; continue ; } ;
	      tcb->ilvl[tcb->ilx].indent_line = i-1 ; break ;		/* Not space/tab - done */
	    } ;
//vout_UCText(VOUT_Trace,0,tcb->ilvl[tcb->ilx].input_ptr) ; vout_NL(VOUT_Trace) ;
	   return(TRUE) ;

continue_main:
	   continue ;
	 } ;
}

/*	v4lex_ReadNextLine_Table - Here to perform TABLE processing on an input line	*/
/*	Call: res = v4lex_ReadNextLine_Table(istr , onlycol , errfld, labelfld , errmsg , vlt , macroname , nestcnt , tcb , filename , lineNumber )
	  where res = TRUE if OK, -column if error,
		istr is pointer to line to parse,
		onlycol, if not UNUSED, is the single column to parse & istr is assumed to be ptr to column data,
		errfld if not null is to be updated with error field,
		labelfld if not null is updated with error table & column names,
		errmsg is updated with error message
		vlt is pointer to table definition to use,
		macroname, if not NULL, is updated to macro pointer (or NULL) associated with table,
		nestcnt is how nested we are,
		tcb if not NULL is input stream and may also be used for certain warning messages
		filename if not NULL is name of source file (s/b if tcb != NULL)
		lineNumber if not UNUSED is line number withing source file (s/b UNUSED if tcb != NULL)						*/

int v4lex_ReadNextLine_Table(istr,onlycol,errfld,labelfld,errmsg,vlt,macroname,nestcnt,tcbE,filename,lineNumber)
  UCCHAR *istr ;
  int onlycol ;
  UCCHAR *errfld,*labelfld,*errmsg ;
  struct V4LEX__Table *vlt ;
  UCCHAR **macroname ;
  int nestcnt ;
  struct V4LEX__TknCtrlBlk *tcbE ;
  UCCHAR *filename ;
  int lineNumber ;
{ P *p,*tpt,acpt ;
  struct V4LEX__Table *tvlt ;
  static struct V4LEX__TknCtrlBlk *tcb=NULL ;
//  static struct V4C__ProcessInfo *pi=NULL ;
  struct V4CI__CountryInfo *ci ;
  struct V4DPI__DimInfo *di ;
  struct V4DPI_UOMDetails *uomd ;
  struct V4DPI__Point_IntMix *pim ;
  struct V4DPI__Point_RealMix *prm ;
  UCCHAR tbuf[V4LEX_Tkn_InpLineMax],ibuf[512],acbuf[128],*bp,*ep,*fp,*tp,*errp,*ub,*ulnb ;
  UCCHAR uom[32],name[256] ; char *ab,*lnb, *lastByte ;
  int relflag,saveint1,saveint2,startcx,endcx ; B64INT lint,lint2 ;
  int i,*ip,len,cx,sign,first,exp,inexp,bytes,yy,mm,hh,ss,quote,pt,skip,uomx,base,ok,ymdo ;
  double dnum,dp ; 
  static int ipowers[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000 } ;
#define ERR(msg) { if ((void *)msg != (void*)errmsg) UCstrcpy(errmsg,UClit(msg)) ; goto field_err ; } ;
#define ERR1(msg) { if ((void *)msg != (void *)errmsg) UCstrcpy(errmsg,msg) ; goto field_err ; } ;

//	if (pi == NULL) pi = v_GetProcessInfo() ;
	if (nestcnt > 10)
	 { v_Msg(NULL,errmsg,"@exceeded max allowed nested tables (use IGNORE instead of TABLE in Column def)") ;
	   if (errfld != NULL) UCstrcpy(errfld,UClit("")) ;
	   if (labelfld != NULL) UCstrcpy(labelfld,vlt->Name) ; return(-1) ;
	 } ;
//	OptScanForDict = vlt->ctx->OptScanForDict ;
	vlt->ctx->pi->vltCur = vlt ;			/* Link up to current vlt */
	if (macroname != NULL)			/* Update macro ? */
	 *macroname = (UCstrlen(vlt->tpo->Macro) > 0 ? vlt->tpo->Macro : NULL) ;
	ci = gpi->ci ;
//	ni = ci->Cntry[ci->CurX].NumInfo ;	/* ni[0] = currency, ni[1] = delim, ni[2] = decimal pt */
	bp = istr ;				/* Set current position (bp) to begin of line */
	len = UCstrlen(bp) ;
	if (*(bp+len-1) == UClit('\r') || *(bp+len-1) == UCNL) *(bp+len-1) = UCEOS ;
	if (onlycol == UNUSED) { startcx = 0 ; endcx = vlt->Columns ; }
	 else { startcx = onlycol ; endcx = onlycol + 1 ; } ;
	for(cx=startcx;cx<endcx;cx++)
	 { if (vlt->Col[cx].Cur == NULL || (vlt->InXML ? (vlt->Col[cx].XMLFlags & V4LEX_Table_XML_Defined)==0 : FALSE))
	    { if ((vlt->Col[cx].ColumnType & (V4LEX_TableCT_Ignore|V4LEX_TableCT_Point|V4LEX_TableCT_Expression)) == 0) vlt->Col[cx].Cur = PTFROMHEAP ; } ;
	   if (onlycol != UNUSED) { fp = bp ; goto got_column ; } ;
	   if (vlt->Col[cx].StartX == UNUSED)
	    { fp = bp ;				/* Pointer to begin of field */
/*	      If field starts with quote and we are comma delimited OR looking for quotes, then handle here */
//	      if (vlt->tpo->Delim == UClit(',') && ((vlt->Col[cx].ColumnType & V4LEX_TableCT_Quoted)==0))	/* If delimiter is comma then assume CSV */
	      if (vlt->tpo->Delim == UClit(',') && ((vlt->Col[cx].ColumnType & V4LEX_TableCT_Quoted) == 0))	/* If delimiter is comma then assume CSV */
	       { if (V4LEX_CSVStatus_OK != v4lex_NextCSVField(fp,tbuf,V4LEX_Tkn_InpLineMax,&bp,TRUE,tcbE))
		  { if (errmsg != NULL) v_Msg(NULL,errmsg,"@invalid CSV string around character %1d",bp-fp) ;
		    if (labelfld != NULL) v_Msg(NULL,labelfld,"@%1U.%2U",vlt->Name,vlt->Col[cx].Name) ;
		    if (errfld != NULL) UCstrcpy(errfld,UClit("")) ;
		    return(-(cx+1)) ;
	          } ;
	         fp = tbuf ; ep = NULL ;
	       }
	      else if (*fp == UClit('"') && ((vlt->Col[cx].ColumnType & V4LEX_TableCT_Quoted)==0))
	       { if (V4LEX_CSVStatus_OK != v4lex_NextTABField(fp,tbuf,V4LEX_Tkn_InpLineMax,&bp))
		  { if (errmsg != NULL) v_Msg(NULL,errmsg,"@invalid TAB string around character %1d",bp-fp) ;
		    if (labelfld != NULL) v_Msg(NULL,labelfld,"@%1U.%2U",vlt->Name,vlt->Col[cx].Name) ;
		    if (errfld != NULL) UCstrcpy(errfld,UClit("")) ;
		    return(-(cx+1)) ;
	          } ;
	         fp = tbuf ; ep = NULL ;
	       } else
	       { skip = ((vlt->Col[cx].ColumnType & V4LEX_TableCT_Skip) != 0 ? vlt->Col[cx].Decimals : 1) ;
	         ep = NULL ;
	         for(i=0;i<skip;i++)
	          { ep = UCstrchrV(bp,(UCCHAR)vlt->tpo->Delim) ;	/* Position to end of field */
		    if (ep != NULL) { *ep = UCEOS ; bp = ep + 1 ; }
		     else { bp = UClit("\0") ; } ;
		  } ;
	       } ;
//v_Msg(vlt->ctx,errmsg,"@**%1U=%2U**\n",vlt->Col[cx].Name,fp) ; vout_UCText(VOUT_Warn,0,errmsg) ;

	      if (vlt->Col[cx].ColumnType & V4LEX_TableCT_Trim)	/* Trim spaces ? */
	       { for(;*fp!=UCEOS;fp++) { if (*fp != UClit(' ')) break ; } ;
	         for(;ep!=NULL && ep!=fp;ep--) { if (*(ep - 1) != UClit(' ')) break ; } ;
		 *ep = UCEOS ;
	       } ;
	      if (*fp == UCEOS)			/* Empty field? */
	       { if (vlt->Col[cx].ColumnType & (V4LEX_TableCT_Ignore|V4LEX_TableCT_Point|V4LEX_TableCT_Expression)) continue ;
	         p = vlt->Col[cx].Dflt ;
	         if (p == NULL) p = vlt->Col[cx].Err ;
		 if (p == NULL)
		  { if (errmsg != NULL) v_Msg(NULL,errmsg,"@empty column") ;
		    if (labelfld != NULL) v_Msg(NULL,labelfld,"@%1U.%2U",vlt->Name,vlt->Col[cx].Name) ;
		    if (errfld != NULL) UCstrcpy(errfld,UClit("")) ;
		    return(-(cx+1)) ;
		  } ;
		 memcpy(vlt->Col[cx].Cur,p,p->Bytes) ;	/* Take this as the current point for this column */
		 if (vlt->Col[cx].FixedLength > 0)
		  { p = vlt->Col[cx].Cur ;
		    switch (p->PntType)
		     { default:		break ;
		       CASEofCharmU
			  len = CHARSTRLEN(p) ;
			  if (len > vlt->Col[cx].FixedLength && len <= V4PT_MaxCharLenIn0) p->Value.AlphaVal[0] = vlt->Col[cx].FixedLength ;
			  len = vlt->Col[cx].FixedLength ;
			  p->Bytes = ALIGN((char *)&p->Value.AlphaVal[len+1] - (char *)p) ;
			  break ;
		       case V4DPI_PntType_UCChar:
			len = UCCHARSTRLEN(p) ;
			if (len > vlt->Col[cx].FixedLength) p->Value.UCVal[0] = vlt->Col[cx].FixedLength ;
			len = vlt->Col[cx].FixedLength ;
			p->Bytes = ALIGN((char *)&p->Value.UCVal[bytes+1] - (char *)p) ;
			break ;
		     } ;
		    continue ;				/* Safe to continue with next column - would not have FixedLength on table name */
		  } ;


















		 if (vlt->Col[cx].ColumnType & (V4LEX_TableCT_TableName|V4LEX_TableCT_NPTableName))
		  { if ((vlt->Col[cx].ColumnType & V4LEX_TableCT_NPTableName) != 0) { ZUS(name) ; }
		     else { UCstrcpy(name,vlt->Col[cx].Name) ; } ;		/* Create table name from column name (maybe) & default value */
		    if (!(p->PntType == V4DPI_PntType_Char || p->PntType == V4DPI_PntType_UCChar))
		     { if (errmsg != NULL) v_Msg(NULL,errmsg,"@invalid default point for table column") ;
		       if (labelfld != NULL) v_Msg(NULL,labelfld,"@%1U.%2U",vlt->Name,vlt->Col[cx].Name) ;
		       if (errfld != NULL) UCstrcpy(errfld,UClit("")) ;
		       return(-(cx+1)) ;
		     } ;
		    v4im_GetPointUC(&ok,ibuf,UCsizeof(ibuf),p,vlt->ctx) ;
//		    UCstrncpy(ibuf,&p->Value.AlphaVal[1],p->Value.AlphaVal[0]) ; ibuf[p->Value.AlphaVal[0]] = UCEOS ;
//		    UCstrcpyAtoU(name,ibuf) ;
//		    for(i=0;name[i]!=UCEOS;i++) { name[i] = toupper(name[i]) ; } ;
		    for(tvlt=gpi->vlt;tvlt!=NULL;tvlt=tvlt->link)
		     { if (UCstrcmpIC(tvlt->Name,name) == 0) break ; } ;
		    if (tvlt == NULL)
		     { v_Msg(NULL,errmsg,"CmdTblName1",name) ;
		       if (labelfld != NULL) v_Msg(NULL,labelfld,"@%1U.%2U",vlt->Name,vlt->Col[cx].Name) ;
		       if (errfld != NULL) UCstrcpy(errfld,UClit("")) ;
		       return(-(cx+1)) ;
		     } ;
//		      v4_error(0,0,"V4","Eval","NOTABLE","Unknown TABLE name (%s)",UCretASC(name)) ;
		    return(v4lex_ReadNextLine_Table(istr,UNUSED,errfld,labelfld,errmsg,tvlt,macroname,nestcnt+1,tcbE,NULL,UNUSED)) ;
		  } ;
		 continue ;
	       } else
	       { if (fp != tbuf) { UCstrcpy(tbuf,fp) ; fp = tbuf ; } ;	/* fp == tbuf on CSV parsing */
	         if (*fp == UClit('"') && vlt->Col[cx].PntType != V4DPI_PntType_List)	/* Start with leading quote? */
		  { fp++ ;			/*  (not any more) */
		    tp = &tbuf[UCstrlen(tbuf)-1] ;
		    if (*tp == UClit('"')) *tp = UCEOS ;/* Ditto for last character in string */
		  } ;
		 if (ep != NULL) *ep = vlt->tpo->Delim ;
	       } ;
	    } else
	    { if (vlt->Col[cx].StartX <= len)
	       { bp = istr + vlt->Col[cx].StartX ;
	         UCstrncpy(tbuf,bp,vlt->Col[cx].Width) ; tbuf[vlt->Col[cx].Width] = UCEOS ;
	         fp = tbuf ;
	       } else
	       { fp = UClit("") ;			/* Past end of string - field is empty */
	       } ;
	    } ;
got_column: /* Entry if onlycol != UNUSED */
	   if (vlt->Col[cx].ColumnType & (V4LEX_TableCT_Ignore|V4LEX_TableCT_Point|V4LEX_TableCT_Expression)) continue ;
	   if (vlt->Col[cx].ColumnType & (V4LEX_TableCT_TableName|V4LEX_TableCT_NPTableName))
	    { if ((vlt->Col[cx].ColumnType & V4LEX_TableCT_NPTableName) != 0) { ZUS(name) ; }
	       else { UCstrcpy(name,vlt->Col[cx].Name) ; } ;		/* Create table name from column name (maybe) & default value */
	      UCstrcat(name,fp) ;
//	      for(i=0;name[i]!=UCEOS;i++) { name[i] = toupper(name[i]) ; } ;
	      for(tvlt=gpi->vlt;tvlt!=NULL;tvlt=tvlt->link)
	       { if (UCstrcmpIC(tvlt->Name,name) == 0) break ; } ;
	      if (tvlt == NULL)
	       { tvlt = v4eval_GetTable(vlt->ctx,name,NULL) ;		/* Try to get table from wherever */
	         if (tvlt == NULL)
	          { v_Msg(NULL,errmsg,"TableDefErr2",vlt->Name,vlt->Col[cx].Name,cx+1) ;
		    if (labelfld != NULL) v_Msg(NULL,labelfld,"@%1U.%2U",vlt->Name,vlt->Col[cx].Name) ;
		    if (errfld != NULL) UCstrcpy(errfld,name) ;
	            return(-(cx+1)) ;
	          } ;
//		  v4_error(0,0,"V4","Eval","NOTABLE","Error via %s.%s (col %d): %s",UCretASC(vlt->Name),UCretASC(vlt->Col[cx].Name),cx+1,UCretASC(vlt->ctx->ErrorMsgAux)) ;
	       } ;
	      tvlt->tpo = &tvlt->tpoT ;				/* Assign default tpo for this table */
	      return(v4lex_ReadNextLine_Table(istr,UNUSED,errfld,labelfld,errmsg,tvlt,macroname,nestcnt+1,tcbE,NULL,UNUSED)) ;
	    } ;
	   p = vlt->Col[cx].Cur ;		/* Current point pointer */
	   errp = fp ;				/* Save in case an error */
	   pt = vlt->Col[cx].PntType ;		/* Get point type for switch below */
	   di = vlt->Col[cx].di ;
try_next_dim:
	   if ((di->Flags & V4DPI_DimInfo_Acceptor) && (vlt->Col[cx].PntTypeInfo != V4LEX_TablePT_Internal))
	    { if (di->ADPnt.Bytes != 0)
	       { v_Msg(NULL,acbuf,"@[UV4:Acceptor %1U Alpha:\"%2U\"]",di->ADPntStr,fp) ;
	       } else
	       { v_Msg(NULL,acbuf,"@[UV4:Acceptor Dim:%1U Alpha:\"%2U\"]",(UCstrlen(di->IsA) > 0 ? di->IsA : di->DimName),fp) ;
	       } ;
	      if (tcb != NULL) { v4lex_ResetTCB(tcb) ; }
	       else { tcb = (struct V4LEX__TknCtrlBlk *)v4mm_AllocChunk(sizeof *tcb,FALSE) ; v4lex_InitTCB(tcb,0) ; }
	      v4lex_NestInput(tcb,NULL,acbuf,V4LEX_InpMode_String) ;
	      if (!v4dpi_PointParse(vlt->ctx,&acpt,tcb,V4DPI_PointParse_RetFalse))
	       { v_Msg(NULL,errmsg,"@Parse of AD point: %1U",vlt->ctx->ErrorMsgAux) ; goto field_err ; } ;
	      tpt = v4dpi_IsctEval(p,&acpt,vlt->ctx,0,NULL,NULL) ;
	      if (tpt == NULL)
	       { ERR("Eval of UV4:Acceptor failed") ; } ;
/*	      Check out the result - primarily if fixed length */
	      if (vlt->Col[cx].FixedLength > 0)
	       { switch (p->PntType)
	          { default:			ok = TRUE ;
		    CASEofCharmU		ok = (CHARSTRLEN(p) == vlt->Col[cx].FixedLength) ; break ;
		    case V4DPI_PntType_UCChar:	ok = (UCCHARSTRLEN(p) == vlt->Col[cx].FixedLength) ; break ;
		  } ;
		 if (!ok)
		  { ERR("Eval of UV4:Acceptor - result length does not match FIXEDLENGTH attribute on column") ; } ;
	       } ;
	      continue ;
	    } ;
	   ZPH(p) ; p->Dim = di->DimId ; p->PntType = pt ;
	   if (vlt->Col[cx].PntType1 != UNUSED) p->AltDim = TRUE ;

	   if (vlt->Col[cx].vtt != NULL)			/* Check for BEFORE/AFTER/PREFIX/SUFFIX string */
	    { static UCCHAR *trans=NULL ;
	      if (vlt->Col[cx].vtt->UCsubstr != NULL)
	       { UCCHAR *b ;
	         b = UCstrstr(fp,vlt->Col[cx].vtt->UCsubstr) ;
	         if (b != NULL)
	          { if (vlt->Col[cx].vtt->substrHow == V4LEX_TT_SubStr_Before)
		     { *b = UCEOS ;					/* Want everything before match - stop at match */
		     } else
		     { fp = b + UCstrlen(vlt->Col[cx].vtt->UCsubstr) ;	/* Want everything after match */
		     } ;
		    } ;
	       } ;
/*	      Check for any transforms such as prefix, etc. */
/*	      Don't apply prefix if telephone number - check later & use prefix if missing area code */
	      if (vlt->Col[cx].vtt->UCprefix != NULL && vlt->Col[cx].PntType != V4DPI_PntType_TeleNum)
	       { if (trans == NULL) trans = v4mm_AllocUC(V4LEX_Tkn_InpLineMax) ;
	         UCstrcpy(trans,vlt->Col[cx].vtt->UCprefix) ; UCstrcat(trans,fp) ; fp = trans ;
	       } ;
	      if (vlt->Col[cx].vtt->UCsuffix != NULL)
	       { if (trans == NULL) trans = v4mm_AllocUC(V4LEX_Tkn_InpLineMax) ;
	         if (fp == trans) { UCstrcat(trans,vlt->Col[cx].vtt->UCsuffix) ; }
		  else { UCstrcpy(trans,fp) ; UCstrcat(trans,vlt->Col[cx].vtt->UCsuffix) ; fp = trans ; } ;
	       } ;
	      if (vlt->Col[cx].vtt->Transforms & V4LEX_TT_Tran_UpperCase)
	       { UCCHAR *b ;
	         if (trans == NULL) trans = v4mm_AllocUC(V4LEX_Tkn_InpLineMax) ;
	         if (fp != trans) { UCstrcpy(trans,fp) ; fp = trans ; } ;
		 for(b=fp;*b!=UCEOS;b++) { *b = UCTOUPPER(*b) ; } ;
	       } ;
	      if (vlt->Col[cx].vtt->Transforms & V4LEX_TT_Tran_LowerCase)
	       { UCCHAR *b ;
	         if (trans == NULL) trans = v4mm_AllocUC(V4LEX_Tkn_InpLineMax) ;
	         if (fp != trans) { UCstrcpy(trans,fp) ; fp = trans ; } ;
		 for(b=fp;*b!=UCEOS;b++) { *b = UCTOLOWER(*b) ; } ;
	       } ;
	      if (vlt->Col[cx].vtt->acpt != NULL)		/* Call Acceptor routine for point? */
	       { 
//	         static int Dim_Alpha=UNUSED ; if (Dim_Alpha == UNUSED) Dim_Alpha = v4dpi_DimGet(vlt->ctx,UClit("ALPHA")) ;
//	         ZPH(&acpt) ; acpt.Dim = Dim_Alpha ; acpt.PntType = V4DPI_PntType_UCChar ;
//	         UCstrcpy(&acpt.Value.UCVal[1],fp) ; len = UCstrlen(fp) ;
//		 UCCHARPNTBYTES2(&acpt,len) ;
		 uccharPNTv(&acpt,fp) ;
//sss		 if (len < 255) { acpt.Value.AlphaVal[0] = len ; acpt.Bytes = ALIGN(V4DPI_PointHdr_Bytes + len + 1) ; }
//		  else { acpt.Value.AlphaVal[0] = 255 ; acpt.Bytes = ALIGN(V4DPI_PointHdr_Bytes + len + 2) ; } ;
		 v4ctx_FrameAddDim(vlt->ctx,0,&acpt,0,0) ; CLEARCACHE	/* Add input string to context as Dim:Alpha */
		tpt = v4dpi_IsctEval(p,vlt->Col[cx].vtt->acpt,vlt->ctx,(V4DPI_EM_NoIsctFail|V4DPI_EM_EvalQuote),NULL,NULL) ;
		if (tpt == NULL)
		 { if (errmsg != NULL) v_Msg(vlt->ctx,errmsg,"TableAcptErr",DE(Column),DE(Acceptor)) ;
		   v4trace_ExamineState(vlt->ctx,vlt->Col[cx].vtt->acpt,V4_EXSTATE_All,VOUT_Err) ;
		   goto field_err ;
		 } ;
/*		If resulting point does not match what we want then maybe fix */
		if (tpt->PntType != vlt->Col[cx].PntType)
		 { struct V4C__Context *ctx = vlt->ctx ;
		   switch (vlt->Col[cx].PntType)
		    { default:		v_Msg(NULL,errmsg,"TableAcptType",DE(Acceptor),tpt,tpt->PntType,vlt->Col[cx].PntType) ; goto field_err ;
		      CASEofCharmU
			v4im_GetPointChar(&ok,ASCTBUF1,V4DPI_AlphaVal_Max,tpt,ctx) ; bytes = strlen(ASCTBUF1) ;
			alphaPNTv(p,ASCTBUF1) ;
			if (vlt->Col[cx].FixedLength > 0)
			 { if (bytes > vlt->Col[cx].FixedLength && bytes <= V4PT_MaxCharLenIn0) p->Value.AlphaVal[0] = vlt->Col[cx].FixedLength ;
			   bytes = vlt->Col[cx].FixedLength ;
			   p->Bytes = ALIGN((char *)&p->Value.AlphaVal[bytes+1] - (char *)p) ;
			 } ;
			break ;
		      case V4DPI_PntType_UCChar:
			v4im_GetPointUC(&ok,UCTBUF1,V4DPI_UCVAL_MaxSafe,tpt,ctx) ; bytes = UCstrlen(UCTBUF1) ;
			uccharPNTv(p,UCTBUF1) ;
			if (vlt->Col[cx].FixedLength > 0)
			 { if (bytes > vlt->Col[cx].FixedLength) p->Value.UCVal[0] = vlt->Col[cx].FixedLength ;
			   bytes = vlt->Col[cx].FixedLength ;
			   p->Bytes = ALIGN((char *)&p->Value.UCVal[bytes+1] - (char *)p) ;
			 } ;
			break ;
			
		    } ;
		 } else
		 {
/*		   If column is fixed length then maybe adjust our point */
		   if (vlt->Col[cx].FixedLength > 0)
		    { switch (vlt->Col[cx].PntType)
		       { 
		         CASEofCharmU
			    bytes = CHARSTRLEN(p) ;
			    if (bytes > vlt->Col[cx].FixedLength && bytes <= V4PT_MaxCharLenIn0)
			     p->Value.AlphaVal[0] = vlt->Col[cx].FixedLength ;
			    bytes = vlt->Col[cx].FixedLength ; /* Maybe force out length of point */
			    p->Bytes = ALIGN((char *)&p->Value.AlphaVal[bytes+1] - (char *)p) ;
		         case V4DPI_PntType_UCChar:
			    bytes = UCCHARSTRLEN(p) ;
			    if (bytes > vlt->Col[cx].FixedLength)
			     p->Value.UCVal[0] = vlt->Col[cx].FixedLength ;
			    bytes = vlt->Col[cx].FixedLength ; /* Maybe force out length of point */
			    p->Bytes = ALIGN((char *)&p->Value.AlphaVal[bytes+1] - (char *)p) ;
		       } ;
		    } ;
		 } ;

		continue ;					/* Got it (in p), go to next field */
	       } ;
	    } ;

#define DN(var) var=0 ; for(;;fp++) { if (*fp != UClit(' ')) break ; } ; for(;;fp++) { if (*fp >= UClit('0') && *fp <= UClit('9')) { var*=10 ; var+= *fp - UClit('0') ; } else { break ; } ; } ;
	   switch(pt)				/* See if we can parse locally */
	    { default:
		ERR("[Unsupported TABLE dimension type]")
//	      case V4DPI_PntType_Context:
//		UCstrcpy(ibuf,UClit("[")) ; UCstrcat(ibuf,fp) ; UCstrcat(ibuf,UClit("]")) ;	/* Turn input into isct */
//		if (UCstrcmp(ibuf,UClit("[]")) == 0) { p->Bytes = V4DPI_PointHdr_Bytes ; break ; } ; /* Check for empty context */
//		if (tcb == NULL) tcb = (struct V4LEX__TknCtrlBlk *)v4mm_AllocChunk(sizeof *tcb,FALSE) ;
//		v4lex_InitTCB(tcb,0) ; v4lex_NestInput(tcb,NULL,ibuf,V4LEX_InpMode_String) ;
//		if (!v4dpi_PointParse(vlt->ctx,p,tcb,V4DPI_PointParse_RetFalse))
//		 { if (errmsg != NULL) v_Msg(NULL,errmsg,"@Point parse failed: %1U",vlt->ctx->ErrorMsgAux) ; goto field_err ; } ;
//
//		p->PntType = V4DPI_PntType_Context ;
//		break ;
	      case V4DPI_PntType_Isct:
		if (tcb != NULL) { v4lex_ResetTCB(tcb) ; }
		 else { tcb = (struct V4LEX__TknCtrlBlk *)v4mm_AllocChunk(sizeof *tcb,FALSE) ; v4lex_InitTCB(tcb,0) ; }
		v4lex_NestInput(tcb,NULL,fp,V4LEX_InpMode_String) ;
		if (!v4dpi_PointParse(vlt->ctx,&acpt,tcb,V4DPI_PointParse_RetFalse))
		 { if (errmsg != NULL) v_Msg(NULL,errmsg,"@Point parse failed: %1U",vlt->ctx->ErrorMsgAux) ; goto field_err ; } ;
		tpt = v4dpi_IsctEval(p,&acpt,vlt->ctx,(V4DPI_EM_NoIsctFail|V4DPI_EM_EvalQuote),NULL,NULL) ;
		if (tpt == NULL) { if (errmsg != NULL) v_Msg(NULL,errmsg,"@Isct point eval failed") ; goto field_err ; } ;
		if (tpt != p) memcpy(p,tpt,tpt->Bytes) ;
		break ;
	      case V4DPI_PntType_List:
		ZS(ibuf) ;
		if (*fp != UClit('('))	UCstrcat(ibuf,UClit("(")) ;					/* Make sure starts with "(" */
		UCstrcat(ibuf,fp) ; if (ibuf[UCstrlen(ibuf)-1] != UClit(')')) UCstrcat(ibuf,UClit(")")) ;	/* and ends with ')' */
		if (tcb != NULL) { v4lex_ResetTCB(tcb) ; }
		 else { tcb = (struct V4LEX__TknCtrlBlk *)v4mm_AllocChunk(sizeof *tcb,FALSE) ; v4lex_InitTCB(tcb,0) ; }
		v4lex_NestInput(tcb,NULL,ibuf,V4LEX_InpMode_String) ;
		if (!v4dpi_PointParse(vlt->ctx,p,tcb,V4DPI_PointParse_RetFalse))
		 { if (errmsg != NULL) v_Msg(NULL,errmsg,"@Point parse failed: %1U",vlt->ctx->ErrorMsgAux) ; goto field_err ; } ;
		p->Dim = di->DimId ;							/* Force dimension back */
		break ;
	      case V4DPI_PntType_CodedRange:
	      case V4DPI_PntType_Int:
		relflag = 0 ;
int_entry:	
//		if (OptScanForDict) { p->Bytes = V4PS_Int ; break ; } ;
		lint = 0 ; first = TRUE ; sign = FALSE ; dp = FALSE ;
		base = (vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Hexadecimal ? 16 : 10) ;
		for(;*fp!=UCEOS;fp++)
		 { switch(*fp)
		    { default:
			if (*fp == ci->Cntry[ci->CurX].DigiDelim) continue ;
			if (*fp == ci->Cntry[ci->CurX].RadixPnt) { if (dp != 0) ERR("Too many decimal points") ; first = FALSE ; dp = 1.0 ; continue ; } ;
			if (*fp == ci->Cntry[ci->CurX].CurrencySign[0] && first && vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Money) continue ;
		    	ERR("Invalid integer character") ;
		      case UClit('<'): relflag = 1 ; break ;
		      case UClit('>'): relflag = 2 ; break ;
		      case UClit('.'): if (base != 10) ERR("Decimals only allowed in base 10 literals") ;
				if (relflag > 0 || *(fp+1) != UClit('.'))
				 { if (dp) ERR("More than 1 decimal point") ; dp = TRUE ;
				   if (*(fp + 1) >= UClit('5')) lint++ ;	/* Maybe round up */
				   break ;
				 } ;
				relflag = 3 ; fp++ ; fp++ ;
				if (sign) lint = -lint ; saveint1 = lint ; if (saveint1 != lint) ERR("Number too large") ;
				goto int_entry ;
		      case UClit('A'): case UClit('B'): case UClit('C'): case UClit('D'): case UClit('E'): case UClit('F'):
		      case UClit('a'): case UClit('b'): case UClit('c'): case UClit('d'): case UClit('e'): case UClit('f'):
			if (base != 16) ERR("'A' thru 'E' valid only in hexadecimal") ;
			lint *= base ; lint += *fp - UClit('A') + 10 ;
			break ;
		      case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'): case UClit('5'): case UClit('6'):
		      case UClit('7'): case UClit('8'): case UClit('9'):
			if (dp) break ;		/* If to right of "." then ignore */
			first = FALSE ; lint *= base ; lint += *fp - UClit('0') ;
			break ;
		      case UClit('$'):	if (vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Money) continue ;
		      case UClit(' '): if (first || vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Money) continue ;
				if (*(fp+1) == UClit(' ') || *(fp+1) == UCEOS) continue ;
				ERR("Embedded spaces") ;
		      case UClit(','): continue ;	/* and commas */
		      case UClit('"'): if (first) continue ; if (*(fp+1) != UCEOS) ERR("Embedded quote") ;
		      case UClit('('): case UClit(')'):
			if (vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Money) { sign = TRUE ; continue ; } ;
			ERR("Parentheses to indicate negative only allowed for FORMAT MONEY columns")
		      case UClit('-'):
			if (vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Money) { sign = TRUE ; continue ; } ;
			if (*(fp+1) == UCEOS) { sign = TRUE ; continue ; } ;	/* Allow trailing sign */
			if (!first) ERR("Embedded minus sign") ; 
			first = FALSE ; sign = TRUE ; continue ;
		    } ;
		 } ;
		if (sign) lint = -lint ;
		if (vlt->Col[cx].vtt != NULL)
		 { lint += vlt->Col[cx].vtt->offset ;
		   if (vlt->Col[cx].vtt->scale != 0.0)
		    lint = lint * (B64INT)vlt->Col[cx].vtt->scale ;
		   if (vlt->Col[cx].vtt->vtts != NULL)
		    { int v ;
		      for(v=0;v<vlt->Col[cx].vtt->vtts->Count;v++)
		       { if (lint == vlt->Col[cx].vtt->vtts->Sub[v].InValue)
		          { if (vlt->Col[cx].vtt->vtts->Sub[v].OutValue == V4LEX_Table_Tran_ErrVal) ERR("Prohibited value") ;
			    lint = vlt->Col[cx].vtt->vtts->Sub[v].OutValue ; break ;
			  } ;
		       } ;
		    } ;
		 } ;
		p->Value.IntVal = lint ; if (p->Value.IntVal != lint) ERR("Number too large") ;
		p->Bytes = V4PS_Int ;
		if (relflag == 0) break ;
		pim = (struct V4DPI__Point_IntMix *)&p->Value.AlphaVal ;
		saveint2 = p->Value.IntVal ;
		switch (relflag)
		 { case 1:	p->Grouping = V4DPI_Grouping_LT ; break ;
		   case 2:	p->Grouping = V4DPI_Grouping_GT ; break ;
		   case 3:
			pim->Entry[0].BeginInt = saveint1 ; pim->Entry[0].EndInt = saveint2 ;
			p->Grouping = 1 ;
//			p->Bytes += sizeof p->Value.IntVal ;
			SETBYTESGRPINT(p) ;
			break ;
		 } ;
		break ;
	      case V4DPI_PntType_UOMPer:
//		if (OptScanForDict) { p->Bytes = V4PS_UOMPer ; break ; } ;
		dnum = 0.0 ; first = TRUE ; sign = FALSE ; dp = 0 ; uomx = 0 ;
		if (*fp == UClit('"')) fp++ ;
		for(;*fp!=UCEOS;fp++)
		 { switch(*fp)
		    { default:
			if (first) ERR("Invalid character") ;
			goto end_per1 ;
		      case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'): case UClit('5'): case UClit('6'):
		      case UClit('7'): case UClit('8'): case UClit('9'):
			first = FALSE ;
			dnum *= 10.0 ; dnum += (double)(*fp - UClit('0')) ; dp *= 10.0 ;
			break ;
		      case UClit(' '): if (first) { continue ; } else { goto end_per1 ; } ;
		      case UClit(','): continue ;	/* And commas */
		      case UClit('.'):
			if (dp != 0) ERR("More than one decimal point") ;
			first = FALSE ; dp = 1.0 ; continue ;
		      case UClit('-'):
			if (!first)		/* Error if not first unless last */
			 { if (*(fp+1) == UClit(' ') || *(fp+1) == UCEOS) { sign = TRUE ; continue ; } ;
			   ERR("Embedded minus sign") ; 
			 } ;
			first = FALSE ; sign = TRUE ; continue ;
		    } ;
		 } ;
end_per1:
		for(i=0;i<vlt->Col[cx].Decimals;i++) { dp = 1 ; dnum /= 10.0 ; } ;
		if (dp > 0.0) dnum /= dp ;
		if (sign) dnum = -dnum ;
		if (vlt->Col[cx].vtt != NULL)
		 { dnum += vlt->Col[cx].vtt->offset ;
		   if (vlt->Col[cx].vtt->scale != 0.0) dnum *= vlt->Col[cx].vtt->scale ;
		 } ;
		memcpy(&p->Value.UOMPerVal.Amount,&dnum,sizeof dnum) ;
/*		Now fall thru and parse the UOM portion */
	      case V4DPI_PntType_UOM:
//		if (OptScanForDict) { p->Bytes = V4PS_UOM ; break ; } ;
		dnum = 0.0 ; first = TRUE ; sign = FALSE ; dp = 0 ; uomx = 0 ;
		if (*fp == UClit('"')) fp++ ;
		for(;*fp!=UCEOS;fp++)
		 { switch(*fp)
		    { case UClit(':'):				/* UClit(':') ends number on internal format */
			if (vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Internal) { fp++ ; goto end_uom1 ; } ;
		      default:
			if (first)
			 { uom[uomx++] = *fp ;
			   if (uomx > UCsizeof(uomd->UEntry[0].PSStr)-1) ERR("Unit-of-measure spec too big") ;
			   continue ;
			 } ;
		    	if (uomx != 0) ERR("Cannot have both prefix & suffix") ;		/* Got prefix & suffix? */
			for(;*fp!=UCEOS;fp++)
			 { if (*fp == UClit(' ')) continue ;
			   uom[uomx++] = *fp ;
			   if (uomx > UCsizeof(uomd->UEntry[0].PSStr)-1) ERR("Unit-of-measure spec too big") ;
			 } ;
			fp-- ; continue ;
		      case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'): case UClit('5'): case UClit('6'):
		      case UClit('7'): case UClit('8'): case UClit('9'):
			first = FALSE ;
			dnum *= 10.0 ; dnum += (double)(*fp - UClit('0')) ; dp *= 10.0 ;
			break ;
		      case UClit(' '): continue ;	/* Ignore spaces */
		      case UClit(','): continue ;	/* And commas */
		      case UClit('.'):
			if (dp != 0) ERR("More than one decimal point") ;
			first = FALSE ; dp = 1.0 ; continue ;
		      case UClit('-'):
			if (!first)		/* Error if not first unless last */
			 { if (*(fp+1) == UClit(' ') || *(fp+1) == UCEOS) { sign = TRUE ; continue ; } ;
			   ERR("Embedded minus sign") ; 
			 } ;
			first = FALSE ; sign = TRUE ; continue ;
		    } ;
		 } ;

end_uom1:	for(i=0;i<vlt->Col[cx].Decimals;i++) { dp = 1 ; dnum /= 10.0 ; } ;
		if (dp > 0.0) dnum /= dp ;
		if (sign) dnum = -dnum ;
		if (pt == V4DPI_PntType_UOM && vlt->Col[cx].vtt != NULL)
		 { dnum += vlt->Col[cx].vtt->offset ;
		   if (vlt->Col[cx].vtt->scale != 0.0) dnum *= vlt->Col[cx].vtt->scale ;
		 } ;
		if (vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Internal)		/* Here to parse ref:index of UOM */
		 { p->Value.UOMVal.Ref = UCstrtol(fp,&tp,10) ;
		   if (*tp != UClit(':')) ERR("Internal UOM of form- num:ref:index")
		   fp = tp + 1 ; p ->Value.UOMVal.Index = UCstrtol(fp,&tp,10) ;
		   if (*tp == UClit(':')) { fp = tp + 1 ; p ->Value.UOMVal.Index += V4DPI_CaseCount_Mult * UCstrtol(fp,&tp,10) ; } ;
		   goto end_uom2 ;
		 } ;
		if (vlt->Col[cx].vtt == NULL)
		 ERR("No UOM tables defined via UOM() module") ;
		if ((uomd = vlt->Col[cx].vtt->uomd) == NULL) ERR("No UOM xxx specification on this column") ;
		if (dp != 0  ? (uomd->Attributes & V4DPI_UOMAttr_FractionOK) == 0 : FALSE)
		 ERR("Fractional units not allowed") ;
		p->Value.UOMVal.Ref = uomd->Ref ;
		uom[uomx] = UCEOS ;
		if (uomx == 0)
		 { if (uomd->DfltAcceptX == UNUSED) { p->Value.UOMVal.Index = UNUSED ; }
		    else { p->Value.UOMVal.Index = uomd->UEntry[uomd->DfltAcceptX].Index ;
			   if (uomd->CaseCount > 0 && uomd->UEntry[uomd->DfltAcceptX].Factor == 0)
			    { dnum *= uomd->CaseCount ; }
			    else { dnum = ((dnum + uomd->UEntry[uomd->DfltAcceptX].preOffset) * uomd->UEntry[uomd->DfltAcceptX].Factor) + uomd->UEntry[uomd->DfltAcceptX].postOffset ; } ;
			   if (uomd->CaseCount != 0) p->Value.UOMVal.Index += V4DPI_CaseCount_Mult * uomd->CaseCount ;
			 } ;
		 } else
		 { for(i=0;i<uomd->Count;i++) { if (UCstrcmp(uom,uomd->UEntry[i].PSStr) == 0) break ; } ;
		   if (i >= uomd->Count)
		    ERR("Unit-of-Measure spec not defined") ;
		   p->Value.UOMVal.Index = uomd->UEntry[i].Index ;
		   if (uomd->CaseCount > 0 && uomd->UEntry[i].Factor == 0)
		    { dnum *= uomd->CaseCount ; }
		    else { dnum = ((dnum + uomd->UEntry[i].preOffset) * uomd->UEntry[i].Factor) + uomd->UEntry[i].postOffset ; } ;
		   if (uomd->CaseCount != 0) p->Value.UOMVal.Index += V4DPI_CaseCount_Mult * uomd->CaseCount ;
		 } ;
end_uom2:	memcpy(&p->Value.UOMVal.Num,&dnum,sizeof(dnum)) ;
		p->Bytes = (pt == V4DPI_PntType_UOM ? V4PS_UOM : V4PS_UOMPer) ;
		break ;
	      case V4DPI_PntType_Fixed:
		relflag = 0 ;
//		if (OptScanForDict) { p->Bytes = V4PS_Fixed ; break ; } ;
		lint = 0 ; first = TRUE ; sign = FALSE ; dp = -1 ;
		for(;*fp!=UCEOS;fp++)
		 { switch(*fp)
		    { default:	ERR("Invalid character for number") ;
		      case UClit('<'): relflag = 1 ; break ;
		      case UClit('>'): relflag = 2 ; break ;
		      case UClit('.'): if (relflag > 0 || *(fp+1) != UClit('.'))
				 { if (dp != -1) ERR("More than one decimal point") ; dp = 0 ;
				   break ;
				 } ;
		      case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'): case UClit('5'): case UClit('6'):
		      case UClit('7'): case UClit('8'): case UClit('9'):
			first = FALSE ; if(dp >= 0) dp++ ;
			lint2 = lint ; lint *= 10 ; lint += *fp - UClit('0') ;
			if (lint < lint2) ERR("Number too large") ;
			break ;
		      case UClit('$'):	if (vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Money) continue ;
		      case UClit(' '): continue ;	/* Ignore spaces */
		      case UClit(','): continue ;	/* and commas */
		      case UClit('"'): continue ;	/* and quotes */
		      case UClit('-'):
			if (vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Money) { sign = TRUE ; continue ; } ;
			if (!first) ERR("Embedded minus sign") ; 
			first = FALSE ; sign = TRUE ; continue ;
		      case UClit('('): case UClit(')'):
			if (vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Money) { sign = TRUE ; continue ; } ;
			ERR("Parentheses to indicate negative only allowed for FORMAT MONEY columns")
		    } ;
		 } ;
		if (dp == -1) dp = 0 ;
		if (dp < vlt->Col[cx].Decimals)
		 { for(;dp<vlt->Col[cx].Decimals;dp++) lint *= 10 ;
		 } else if (dp > vlt->Col[cx].Decimals)
		   { for(;dp>vlt->Col[cx].Decimals+1;dp--) lint /= 10 ;
		     lint += 5 ; lint /= 10 ;
		   } ;
		if (sign) lint = -lint ;
		if (vlt->Col[cx].vtt != NULL)
		 { lint += vlt->Col[cx].vtt->offset ;
		   if (vlt->Col[cx].vtt->scale != 0.0) lint = lint * (B64INT)vlt->Col[cx].vtt->scale ;
		 } ;
		p->Bytes = V4PS_Fixed ; p->LHSCnt = vlt->Col[cx].Decimals ; memcpy(&p->Value.FixVal,&lint,sizeof lint) ;
		break ;
	      case V4DPI_PntType_Real:
//		if (OptScanForDict) { p->Bytes = V4PS_Real ; break ; } ;
		dnum = 0.0 ; inexp = FALSE ; first = TRUE ; sign = FALSE ; dp = 0 ;
		for(;*fp!=UCEOS;fp++)
		 { switch(*fp)
		    { default:
//			if (*fp == ni[1]) continue ;
//			if (*fp == ni[2]) { if (dp != 0) ERR("Too many decimal points") ; first = FALSE ; dp = 1.0 ; continue ; } ;
//			if (*fp == ni[0] && first && vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Money) continue ;
			if (*fp == ci->Cntry[ci->CurX].DigiDelim) continue ;
			if (*fp == ci->Cntry[ci->CurX].RadixPnt) { if (dp != 0) ERR("Too many decimal points") ; first = FALSE ; dp = 1.0 ; continue ; } ;
			if (*fp == ci->Cntry[ci->CurX].CurrencySign[0] && first && vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Money)
			 continue ;
		    	ERR("Invalid numeric character") ;
		      case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'): case UClit('5'): case UClit('6'):
		      case UClit('7'): case UClit('8'): case UClit('9'):
			first = FALSE ;
			if (inexp)
			 { if (dp != 0) ERR("More than one decimal point") ;
			   exp *= 10 ; exp += *fp - UClit('0') ;
			 } else
			 { dnum *= 10.0 ; dnum += (double)(*fp - UClit('0')) ; dp *= 10.0 ;
			 } ;
			break ;
//		      case UClit('$'):	if (vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Money) continue ;
		      case UClit('('): case UClit(')'):
			if (vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Money) { sign = TRUE ; continue ; } ;
			ERR("Parentheses to indicate negative only allowed for FORMAT MONEY columns")
		      case UClit(' '): continue ;	/* Ignore spaces */
//		      case UClit(','): continue ;	/* And commas */
		      case UClit('"'): continue ;	/*  and quotes */
		      case UClit('E'): case UClit('e'):
			if (inexp) ERR("More than one exponent") ;
			if (dp > 0.0) dnum /= dp ;
			if (sign) { dnum = -dnum ; sign = FALSE ; } ;
			first = FALSE ; inexp = TRUE ; exp = 0 ; dp = 0.0 ;
			if (*(fp+1) == UClit('+')) fp++ ;
			if (*(fp+1) == UClit('-')) { fp++ ; sign = TRUE ; } ;
			break ;
//		      case UClit('.'):
//			if (dp != 0) ERR("Too many decimal points") ;
//			first = FALSE ; dp = 1.0 ; continue ;
		      case UClit('+'):
			if (!first)		/* Error if not first unless last */
			 { ERR("Embedded plus sign") ; } ;
			first = FALSE ; continue ;
		      case UClit('-'):
			if (vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Money) { sign = TRUE ; continue ; } ;
			if (!first)		/* Error if not first unless last */
			 { if (*(fp+1) == UClit(' ') || *(fp+1) == UCEOS) { sign = TRUE ; continue ; } ;
			   ERR("Embedded minus sign") ; 
			 } ;
			first = FALSE ; sign = TRUE ; continue ;
		    } ;
		 } ;
		for(i=0;i<vlt->Col[cx].Decimals;i++) { dnum /= 10.0 ; } ;
		if (inexp)
		 { if (sign) { for(dp=1.0;exp>0;exp--) { dp /= 10.0 ; } ; }
		    else { for(dp=1.0;exp>0;exp--) { dp *= 10.0 ; } ; }
		   dnum = dnum * dp ;
		 } else
		 { if (dp > 0.0) dnum /= dp ;
		   if (sign) dnum = -dnum ;
		 } ;
		if (vlt->Col[cx].vtt != NULL)
		 { dnum += vlt->Col[cx].vtt->offset ;
		   if (vlt->Col[cx].vtt->scale != 0.0) dnum *= vlt->Col[cx].vtt->scale ;
		 } ;
		PUTREAL(p,dnum) ;
		p->Bytes = V4PS_Real ;
		break ;
	      case V4DPI_PntType_XDict:
	      case V4DPI_PntType_Dict:
		for(;*fp!=UCEOS;fp++) { if (*fp != UClit(' ')) break ; } ;		/* Get rid of leading spaces */
		tp = fp + UCstrlen(fp)-1 ;	/* Position to last byte */
		for(;tp!=fp;tp--) { if (!isspace(*tp)) break ; *tp = UCEOS ; } ;	/* Get rid of trailing spaces */
		if (*fp == UCEOS)
		 { tpt = vlt->Col[cx].Dflt ;
	           if (tpt == NULL) tpt = vlt->Col[cx].Err ;
		   if (tpt == NULL)
		    { if (errmsg != NULL) v_Msg(NULL,errmsg,"@empty column") ;
		      if (labelfld != NULL) v_Msg(NULL,labelfld,"@%1U.%2U",vlt->Name,vlt->Col[cx].Name) ;
		      if (errfld != NULL) UCstrcpy(errfld,UClit("")) ;
		      return(-(cx+1)) ;
		    } ;
		   memcpy(p,tpt,tpt->Bytes) ; break ;
		 } ;

		p->Value.IntVal =
		 (pt == V4DPI_PntType_Dict ? v4dpi_DictEntryGet(vlt->ctx,0,fp,di,NULL) : v4dpi_XDictEntryGet(vlt->ctx,fp,di,0)) ;
		p->Bytes = V4PS_Int ;
		break ;
	      case V4DPI_PntType_Color:
		for(;*fp!=UCEOS;fp++) { if (*fp != UClit(' ')) break ; } ;		/* Get rid of leading spaces */
		tp = fp + UCstrlen(fp)-1 ;	/* Position to last byte */
		for(;tp!=fp;tp--) { if (!isspace(*tp)) break ; *tp = UCEOS ; } ;	/* Get rid of trailing spaces */
		if (*fp == UCEOS)
		 { tpt = vlt->Col[cx].Dflt ;
	           if (tpt == NULL) tpt = vlt->Col[cx].Err ;
		   if (tpt == NULL)
		    { if (errmsg != NULL) v_Msg(NULL,errmsg,"@empty column") ;
		      if (labelfld != NULL) v_Msg(NULL,labelfld,"@%1U.%2U",vlt->Name,vlt->Col[cx].Name) ;
		      if (errfld != NULL) UCstrcpy(errfld,UClit("")) ;
		      return(-(cx+1)) ;
		    } ;
		   memcpy(p,tpt,tpt->Bytes) ; break ;
		 } ;
		p->Value.IntVal = v_ColorNameToRef(fp) ;
		if (p->Value.IntVal == 0) ERR("Invalid color specification") ;
		p->Bytes = V4PS_Int ;
		break ;
	      case V4DPI_PntType_Country:
		for(;*fp!=UCEOS;fp++) { if (*fp != UClit(' ')) break ; } ;		/* Get rid of leading spaces */
		tp = fp + UCstrlen(fp)-1 ;	/* Position to last byte */
		for(;tp!=fp;tp--) { if (!isspace(*tp)) break ; *tp = UCEOS ; } ;	/* Get rid of trailing spaces */
		if (*fp == UCEOS)
		 { tpt = vlt->Col[cx].Dflt ;
	           if (tpt == NULL) tpt = vlt->Col[cx].Err ;
		   if (tpt == NULL)
		    { if (errmsg != NULL) v_Msg(NULL,errmsg,"@empty column") ;
		      if (labelfld != NULL) v_Msg(NULL,labelfld,"@%1U.%2U",vlt->Name,vlt->Col[cx].Name) ;
		      if (errfld != NULL) UCstrcpy(errfld,UClit("")) ;
		      return(-(cx+1)) ;
		    } ;
		   memcpy(p,tpt,tpt->Bytes) ; break ;
		 } ;
		p->Value.IntVal = v_CountryNameToRef(fp) ;
		if (p->Value.IntVal == 0) ERR("Invalid country specification") ;
		p->Bytes = V4PS_Int ;
		break ;
	      case V4DPI_PntType_Logical:
//		if (OptScanForDict) { p->Bytes = V4PS_Int ; break ; } ;
		switch (*fp)
		 { case UClit('Y'): case UClit('y'): case UClit('1'): case UClit('T'): case UClit('t'):	sign = TRUE ; break ;
		   case UClit('N'): case UClit('n'): case UClit('0'): case UClit('F'): case UClit('f'):	sign = FALSE ; break ;
		   default:						ERR("Invalid Logical value") ;	
		 } ;
		if (vlt->Col[cx].vtt != NULL)
		 { if (vlt->Col[cx].vtt->scale < 0.0) sign = !sign ; } ;
		p->Value.IntVal = sign ;
		p->Bytes = V4PS_Int ;
		break ;
	      case V4DPI_PntType_GeoCoord:
		if (!v_ParseGeoCoord(vlt->ctx,p,di,fp,errmsg)) goto field_err ;
		break ;
	      case V4DPI_PntType_Complex:
		break ;
	      case V4DPI_PntType_TeleNum:
		if (!v_parseTeleNum(vlt->ctx,di,p,(vlt->Col[cx].vtt != NULL ? vlt->Col[cx].vtt->UCprefix : NULL),fp,errmsg)) goto field_err ;
		break ;
	      case V4DPI_PntType_BinObj:
//		if (OptScanForDict) { p->Bytes = V4PS_Int ; break ; } ;
//		tp = p->Value.AlphaVal ;
		bytes = 0 ;
		if (vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Hex4)
		 { ip = (int *)&ibuf ;			/* Stage results here */
		   for(i=0;*fp!=UCEOS;fp++)
		    { switch(*fp)
		       { default: ERR("Invalid hexadecimal number") ;
		         case UClit('"'): continue ;
		         case UClit(','):
				*ip = i ; ip++ ; i = 0 ; bytes += 4 ;
				if (bytes > 250) { ERR("Alpha/Bit pattern cannot exceed 254 bytes") ; } ;
				break ;
			 case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'):
			 case UClit('5'): case UClit('6'): case UClit('7'): case UClit('8'): case UClit('9'):		i = (i<<4) + *fp - UClit('0') ; break ;
			 case UClit('a'): case UClit('b'): case UClit('c'): case UClit('d'): case UClit('e'): case UClit('f'):	i = (i<<4) + *fp - UClit('a') + 10 ; break ;
			 case UClit('A'): case UClit('B'): case UClit('C'): case UClit('D'): case UClit('E'): case UClit('F'):	i = (i<<4) + *fp - UClit('A') + 10 ; break ;
		       } ;
		    } ;
		   *ip = i ; ip++ ; bytes += 4 ;
		   if (bytes >= V4DPI_AlphaVal_Max) { ERR("BINARY bit pattern exceeds max bytes allowed") } ;
		   if (bytes < vlt->Col[cx].FixedLength)		/* Maybe pad out to fixed length? */
		    { memset(ip,0,vlt->Col[cx].FixedLength-bytes) ; bytes = vlt->Col[cx].FixedLength ; }
		    else if (vlt->Col[cx].FixedLength > 0 && bytes > vlt->Col[cx].FixedLength) bytes = vlt->Col[cx].FixedLength ;
		   memcpy(&p->Value.AlphaVal,ibuf,bytes) ;
		   p->Bytes = bytes + V4DPI_PointHdr_Bytes ;
		   break ;
		 } ;
		ERR("BINARY must be Hex4 type")
	      CASEofCharmU
//		if (OptScanForDict) { p->Bytes = V4PS_Int ; break ; } ;
		if (*fp == UClit('"'))			/* Got a string enclosed in quotes? */
		 { quote = TRUE ; fp++ ; } else { quote = FALSE ; } ;
		lnb = (ab = &p->Value.AlphaVal[1]) ; bytes = 0 ;
		if (vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Hex4)
		 { ip = (int *)&ibuf ;			/* Stage results here */
		   for(i=0;*fp!=UCEOS;fp++)
		    { switch(*fp)
		       { default: ERR("Invalid hexadecimal number") ;
		         case UClit('"'): continue ;
		         case UClit(','):
				*ip = i ; ip++ ; i = 0 ; bytes += 4 ;
				if (bytes > 250) { ERR("Alpha/Bit pattern cannot exceed 254 bytes") ; } ;
				break ;
			 case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'):
			 case UClit('5'): case UClit('6'): case UClit('7'): case UClit('8'): case UClit('9'):		i = (i<<4) + *fp - UClit('0') ; break ;
			 case UClit('a'): case UClit('b'): case UClit('c'): case UClit('d'): case UClit('e'): case UClit('f'):	i = (i<<4) + *fp - UClit('a') + 10 ; break ;
			 case UClit('A'): case UClit('B'): case UClit('C'): case UClit('D'): case UClit('E'): case UClit('F'):	i = (i<<4) + *fp - UClit('A') + 10 ; break ;
		       } ;
		    } ;
		   *ip = i ; ip++ ; bytes += 4 ;
		   if (bytes >= 255) { ERR("Alpha/Bit pattern cannot exceed 254 bytes") } ;
		   if (bytes < vlt->Col[cx].FixedLength)		/* Maybe pad out to fixed length? */
		    { memset(ip,0,vlt->Col[cx].FixedLength-bytes) ; bytes = vlt->Col[cx].FixedLength ; }
		    else if (vlt->Col[cx].FixedLength > 0 && bytes > vlt->Col[cx].FixedLength) bytes = vlt->Col[cx].FixedLength ;
		   memcpy(&p->Value.AlphaVal[1],ibuf,bytes) ; p->Value.AlphaVal[0] = bytes ;
		   p->Bytes = ALIGN((char *)&p->Value.AlphaVal[bytes+1] - (char *)p) ;
		   break ;
		 } ;	/* End of Hex4 parse */
		lastByte = ab + V4DPI_AlphaVal_Max - 4 ;				/* Last byte we can safely write to */
		for(i=0;*fp!=UCEOS;i++,fp++)
		 { 
//		   if (ab >= lastByte)						/* Don't overwrite string */
//		    { ab++ ; if (*fp != UClit(' ')) lnb = ab ; continue ; } ;
		   if (ab >= lastByte) continue ;				/* Don't overwrite string */
		   if (*fp == UClit('"') && *(fp+1) == UClit('"'))
		    { if (quote) { *(ab++) = *fp ; fp++ ; continue ; } ;
		    } ;
		   if (*fp == UClit('\\'))
		    { fp++ ;
		      switch(*fp)
		       { default:	*(ab++) = *fp ; break ;
		         case UClit('t'):	*(ab++) = UClit('\t') ; break ;
			 case UClit('l'):	*(ab++) = 10 ; break ;
			 case UClit('n'):	*(ab++) = UCNL ; break ;
			 case UClit('r'):	*(ab++) = UClit('\r') ; break ;
			 case UClit('/'):	*(ab++) = UClit('/') ; break ;
			 case UClit('"'):	*(ab++) = UClit('"') ; break ;
			 case UClit('\''):	*(ab++) = UClit('\'') ; break ;
			 case UClit('\\'):	*(ab++) = UClit('\\') ; break ;
		       } ;
		    } else { *(ab++) = *fp ; } ;
		   if (*fp != UClit(' '))
		    lnb = ab ;			/* Track last non-blank byte */
		 } ;
		bytes = lnb - (char *)&p->Value.AlphaVal[1] ;
//		if (bytes < vlt->Col[cx].FixedLength)			/* Maybe pad out to fixed length? */
//		 { memset(ab,UClit(' '),vlt->Col[cx].FixedLength-bytes) ; } ;
		CHARPNTBYTES2(p,bytes)
//sss		if (bytes >= 255) { p->Value.AlphaVal[0] = 255 ; bytes++ ; p->Value.AlphaVal[bytes] = UCEOS ; }
//		 else { p->Value.AlphaVal[0] = bytes ; } ;
		if (vlt->Col[cx].FixedLength > 0)
		 { if (bytes > vlt->Col[cx].FixedLength)
		    { 
		      v_Msg(NULL,errmsg,"@*Line(%1d) of file(%2U) column (%3d %4U) length (%5d) exceeded fixed length (%6d)\n",
				(tcbE != NULL ? tcbE->ilvl[tcbE->ilx].current_line : lineNumber),(tcbE != NULL ? tcbE->ilvl[tcbE->ilx].file_name : (filename != NULL ? filename : UClit(""))),cx+1,vlt->Col[cx].Name,bytes,vlt->Col[cx].FixedLength) ;
		      vout_UCText(VOUT_Warn,0,errmsg) ;
		      bytes = vlt->Col[cx].FixedLength ;			/* Truncate string */
 		      CHARPNTBYTES2(p,bytes) ;
		    } else if (bytes < vlt->Col[cx].FixedLength)
		    { CHARPNTBYTES2(p,bytes) ;
/*		      BFAgg writes out based on point size, not character length! */
		      p->Bytes = ALIGN((char *)&p->Value.AlphaVal[vlt->Col[cx].FixedLength+1] - (char *)p) ;
		    } ;
		 } ;
 		break ;
	      case V4DPI_PntType_UCChar:
		if (*fp == UClit('"'))			/* Got a string enclosed in quotes? */
		 { quote = TRUE ; fp++ ; } else { quote = FALSE ; } ;
		ulnb = (ub = &p->Value.UCVal[1]) ; bytes = 0 ;
		for(;*fp!=UCEOS;fp++)
		 { if (*fp == UClit('"') && *(fp+1) == UClit('"'))
		    { if (quote) { *(ub++) = *fp ; fp++ ; continue ; } ;
		    } ;
		   if (*fp == UClit('\\'))
		    { fp++ ;
		      switch(*fp)
		       { default:		*(ub++) = *fp ; break ;
		         case UClit('t'):	*(ub++) = UClit('\t') ; break ;
			 case UClit('l'):	*(ub++) = UClit('\012') ; break ;
			 case UClit('n'):	*(ub++) = UCNL ; break ;
			 case UClit('r'):	*(ub++) = UClit('\r') ; break ;
			 case UClit('/'):	*(ub++) = UClit('/') ; break ;
			 case UClit('"'):	*(ub++) = UClit('"') ; break ;
			 case UClit('\''):	*(ub++) = UClit('\'') ; break ;
			 case UClit('\\'):	*(ub++) = UClit('\\') ; break ;
			 case UClit('+'):
			   { int ucval,i,sc ;
			     for(ucval=0,i=0;i<4;i++)
			      { sc = *(++fp) ;
				if (sc >= UClit('0') && sc <= UClit('9')) { ucval <<= 4 ; ucval |= (sc - UClit('0')) ; }
				 else if (sc >= UClit('A') && sc <= UClit('F'))  { ucval <<= 4 ; ucval |= 10 + (sc - UClit('A')) ; }
				 else if (sc >= UClit('a') && sc <= UClit('f'))  { ucval <<= 4 ; ucval |= 10 + (sc - UClit('a')) ; }
				 else ERR("Invalid character in UNICODE '\\+nnnn' literal")
			      } ;
			     *(ub++) = ucval ;
			   }
		       } ;
		    } else { *(ub++) = *fp ; } ;
		   if (*fp != UClit(' '))
		    ulnb = ub ;			/* Track last non-blank byte */
		 } ;
		bytes = ulnb - &p->Value.UCVal[1] ;
		UCCHARPNTBYTES2(p,bytes) ;
		if (vlt->Col[cx].FixedLength > 0)
		 { if (bytes > vlt->Col[cx].FixedLength && bytes <= V4PT_MaxCharLenIn0) UCPNTSETLEN(p,vlt->Col[cx].FixedLength) ;
		   bytes = vlt->Col[cx].FixedLength ; /* Maybe force out length of point */
		 } ;
/*		Force size of point to match bytes (not necessarily actual size of character string)	*/
/*		BFAgg writes out based on point size, not character length!				*/
		p->Bytes = ALIGN((char *)&p->Value.UCVal[bytes+1] - (char *)p) ;
		break ;
	      case V4DPI_PntType_UTime:
		if (vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Internal)
		 { dnum = UCstrtod(fp,&tp) ; if (*tp != UCEOS) ERR("Invalid character in internal representation") ;
		   if (dnum < 0) dnum = 0 ;
		   PUTREAL(p,dnum) ; p->Bytes = V4PS_UTime ;
		   break ;
		 } ;
//		if (OptScanForDict) { p->Bytes = V4PS_Int ; break ; } ;
		p->Value.IntVal = 0 ; relflag = 0 ; hh = (mm = (ss = UNUSED)) ;
		for(;*fp!=UCEOS;fp++)
		 { switch(*fp)
		    { default:	ERR("Invalid character for time") ;
		      case UClit('<'): relflag = 1 ; break ;
		      case UClit('>'): relflag = 2 ; break ;
		      case UClit('.'): if (relflag > 0 || *(fp+1) != UClit('.')) ERR("Invalid use of decimal point") ;
				relflag = 3 ; fp++ ;
				if (hh == UNUSED) { hh = p->Value.IntVal / 100 ; mm = p->Value.IntVal % 100 ; ss = 0 ; }
				 else if (mm == UNUSED) { mm = p->Value.IntVal ; ss = 0 ; }
				 else { ss = p->Value.IntVal ; }
				if (hh < 0 || hh > 23 || mm < 0 || mm > 59 || ss < 0 || ss > 59) ERR("Time component invalid") ;
				p->Value.IntVal = hh * 3600 + mm * 60 + ss ;
				saveint1 = p->Value.IntVal ;
				p->Value.IntVal = 0 ; hh = (mm = (ss = UNUSED)) ; break ;
		      case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'): case UClit('5'): case UClit('6'):
		      case UClit('7'): case UClit('8'): case UClit('9'):
			p->Value.IntVal *= 10 ; p->Value.IntVal += *fp - UClit('0') ; break ;
		      case UClit(':'):
			if (hh == UNUSED) { hh = p->Value.IntVal ; }
			 else if (mm == UNUSED) { mm = p->Value.IntVal ; }
			 else ERR("Too many parts to time") ;
			p->Value.IntVal = 0 ; break ;
		      case UClit(' '): continue ;	/* Ignore spaces */
		    } ;
		 } ;
		if (hh == UNUSED) { hh = p->Value.IntVal / 100 ; mm = p->Value.IntVal % 100 ; ss = 0 ; }
		 else if (mm == UNUSED) { mm = p->Value.IntVal ; ss = 0 ; }
		 else { ss = p->Value.IntVal ; }
		if (hh < 0 || hh > 23 || mm < 0 || mm > 59 || ss < 0 || ss > 59) ERR("Time component invalid") ;
		dnum = hh * 3600 + mm * 60 + ss ; PUTREAL(p,dnum) ;
		p->Bytes = V4PS_UTime ;
		if (relflag == 0) break ;
		prm = (struct V4DPI__Point_RealMix *)&p->Value.AlphaVal ;
		saveint2 = p->Value.IntVal ;
		switch (relflag)
		 { case 1:	p->Grouping = V4DPI_Grouping_LT ; break ;
		   case 2:	p->Grouping = V4DPI_Grouping_GT ; break ;
		   case 3:
			if (saveint1 > saveint2) ERR("First time must be <= to second") ;
			dnum = saveint1 ; SETDBL(prm->Entry[0].BeginReal,dnum) ; dnum = saveint2 ; SETDBL(prm->Entry[0].EndReal,dnum) ;
			p->Grouping = 1 ;
//			p->Bytes += sizeof dnum ;
			SETBYTESGRPDBL(p) ;
			break ;
		 } ;
		break ;
	      case V4DPI_PntType_UMonth:
		if (vlt->Col[cx].PntTypeInfo == V4LEX_TablePT_Internal) { relflag = 0 ; goto int_entry ; } ;
//		if (OptScanForDict) { p->Bytes = V4PS_Int ; break ; } ;
		relflag = UNUSED ; saveint2 = V4LIM_SmallestNegativeInt ;
		switch(*fp)
		 { case UClit('<'):	if (*(fp+1) != UClit('=')) { relflag = V4DPI_Grouping_LT ; }
				 else  { fp++ ; relflag = V4DPI_Grouping_LE ; }
				fp ++ ; break ;
		   case UClit('>'): if (*(fp+1) != UClit('=')) { relflag = V4DPI_Grouping_GT ; }
				 else  { fp++ ; relflag = V4DPI_Grouping_GE ; }
				fp ++ ; break ;
		 } ;

		if ((tp = UCstrchr(fp,'.')) != NULL)
		 { *tp = UCEOS ; saveint1 = v_ParseUMonth(gpi->ctx,fp,vlt->Col[cx].PntTypeInfo) ;
		   fp = tp + 1 ; if (*fp != UClit('.')) ERR("Invalid UMonth range") ;
		   fp ++ ; saveint2 = v_ParseUMonth(gpi->ctx,fp,vlt->Col[cx].PntTypeInfo) ;
		 } else
		 { saveint1 = v_ParseUMonth(gpi->ctx,fp,vlt->Col[cx].PntTypeInfo) ;
		 } ;
		if (saveint1 == VCAL_BadVal) ERR1(gpi->ctx->ErrorMsgAux) ;
		p->Bytes = V4PS_Int ;
		if (relflag == UNUSED) { p->Value.IntVal = saveint1 ; break ; } ;
		pim = (struct V4DPI__Point_IntMix *)&p->Value ;
		switch (relflag)
		 { default:	if (relflag != UNUSED) p->Grouping = relflag ; break ;
		   case 1:
			if (saveint1 > saveint2) ERR("First month must be <= to second") ;
			pim->Entry[0].BeginInt = saveint1 ; pim->Entry[0].EndInt = saveint2 ;
			p->Grouping = 1 ;
//			p->Bytes += sizeof p->Value.IntVal ;
			SETBYTESGRPINT(p) ;
			break ;
		 } ;
		break ;
	      case V4DPI_PntType_UWeek:
//		if (OptScanForDict) { p->Bytes = V4PS_Int ; break ; } ;
		p->Value.IntVal = 0 ;
		for(;*fp!=UCEOS;fp++)
		 { switch(*fp)
		    { default:	ERR("Invalid character for week") ;
		      case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'): case UClit('5'): case UClit('6'):
		      case UClit('7'): case UClit('8'): case UClit('9'):
			p->Value.IntVal *= 10 ; p->Value.IntVal += *fp - UClit('0') ; break ;
		      case UClit(' '): continue ;	/* Ignore spaces */
		    } ;
		 } ;
		if (vlt->Col[cx].PntTypeInfo != V4LEX_TablePT_Internal)
		 { yy = p->Value.IntVal / 100 ; VCALADJYEAR(((vlt->Col[cx].di->ds.UWeek.calFlags & VCAL_Flags_Historical) != 0),yy) ; mm = p->Value.IntVal % 100 ;
		   if (mm < 1 || mm > 52) ERR("Week must be 1 - 52") ;
		   p->Value.IntVal = (yy - VCAL_BaseYear) * 52 + mm - 1 ;
		 } ;
		p->Bytes = V4PS_Int ;
		break ;
	      case V4DPI_PntType_UQuarter:
//		if (OptScanForDict) { p->Bytes = V4PS_Int ; break ; } ;
		p->Value.IntVal = 0 ;
		for(;*fp!=UCEOS;fp++)
		 { switch(*fp)
		    { default:	ERR("Invalid character for quarter") ;
		      case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'): case UClit('5'): case UClit('6'):
		      case UClit('7'): case UClit('8'): case UClit('9'):
			p->Value.IntVal *= 10 ; p->Value.IntVal += *fp - UClit('0') ; break ;
		      case UClit(' '): continue ;	/* Ignore spaces */
		      case UClit('Q'): case UClit('q'): continue ;	/* Ignore Q's */
		    } ;
		 } ;
		if (vlt->Col[cx].PntTypeInfo != V4LEX_TablePT_Internal)
		 { yy = p->Value.IntVal / 100 ; VCALADJYEAR(((vlt->Col[cx].di->ds.UQuarter.calFlags & VCAL_Flags_Historical) != 0),yy) ; mm = p->Value.IntVal % 100 ;
		   if (mm < 1 || mm > 4) ERR("Quarter must be 1 - 4") ;
//		   p->Value.IntVal = (yy - VCAL_BaseYear) * 4 + mm - 1 ;
		   p->Value.IntVal = YYQQtoUQTR(yy,mm) ;
		 } ;
		if (vlt->Col[cx].vtt != NULL) p->Value.IntVal += vlt->Col[cx].vtt->offset ;
		p->Bytes = V4PS_Int ;
		break ;
	      case V4DPI_PntType_UPeriod:
//		if (OptScanForDict) { p->Bytes = V4PS_Int ; break ; } ;
		p->Value.IntVal = 0 ;
		for(;*fp!=UCEOS;fp++)
		 { switch(*fp)
		    { default:	ERR("Invalid character for period") ;
		      case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'): case UClit('5'): case UClit('6'):
		      case UClit('7'): case UClit('8'): case UClit('9'):
			p->Value.IntVal *= 10 ; p->Value.IntVal += *fp - UClit('0') ; break ;
		      case UClit(' '): continue ;	/* Ignore spaces */
		    } ;
		 } ;
//xxx read table di already set
		{ int ppy = di->ds.UPeriod.periodsPerYear ; if (ppy == 0) ppy = 13 ; 
		  if (vlt->Col[cx].PntTypeInfo != V4LEX_TablePT_Internal)
		   { yy = p->Value.IntVal / 100 ; VCALADJYEAR(((vlt->Col[cx].di->ds.UPeriod.calFlags & VCAL_Flags_Historical) != 0),yy) ; mm = p->Value.IntVal % 100 ;
		     if (mm < 1 || mm > ppy) ERR("Period must be 1 - 13") ;
		     p->Value.IntVal = (yy - VCAL_BaseYear) * ppy + mm - 1 ;
		   } else
		   { if (vlt->Col[cx].Decimals > 0) p->Value.IntVal += (vlt->Col[cx].Decimals - VCAL_BaseYear) * ppy ;
		     if (p->Value.IntVal > 4562 /* UPeriod:2200/13 */)
		      { if (p->Value.IntVal < 185890 || p->Value.IntVal > 220013 || (p->Value.IntVal % 100 < 90)) ERR("Invalid internal UPeriod value") ; } ;
		   } ;
		}
		p->Bytes = V4PS_Int ;
		break ;
	      case V4DPI_PntType_UYear:
//		if (OptScanForDict) { p->Bytes = V4PS_Int ; break ; } ;
		p->Value.IntVal = 0 ;
		for(;*fp!=UCEOS;fp++)
		 { switch(*fp)
		    { default:	ERR("Invalid character for year") ;
		      case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'): case UClit('5'): case UClit('6'):
		      case UClit('7'): case UClit('8'): case UClit('9'):
			p->Value.IntVal *= 10 ; p->Value.IntVal += *fp - UClit('0') ; break ;
		      case UClit(' '): continue ;	/* Ignore spaces */
		    } ;
		 } ;
		VCALADJYEAR(((vlt->Col[cx].di->ds.UYear.calFlags & VCAL_Flags_Historical) != 0),p->Value.IntVal) ; if (p->Value.IntVal < VCAL_UYearMin || p->Value.IntVal > VCAL_UYearMax) ERR("Invalid UYear") ;
		p->Bytes = V4PS_Int ;
		break ;
	      case V4DPI_PntType_UDT:
//		if (OptScanForDict) { p->Bytes = V4PS_Int ; break ; } ;
		ymdo = (di->ds.UDT.YMDOrder[0] == V4LEX_YMDOrder_None ? ci->Cntry[ci->CurX].ymdOrder[0] : di->ds.UDT.YMDOrder[0]) ;
		if ((p->Value.IntVal = v_ParseUDT(gpi->ctx,fp,vlt->Col[cx].PntTypeInfo,ymdo,UNUSED,NULL,((di->ds.UDT.calFlags & VCAL_Flags_Historical) == 0))) == VCAL_BadVal) ERR1(gpi->ctx->ErrorMsgAux) ;
		p->Bytes = V4PS_Int ;
		break ;
	      case V4DPI_PntType_Calendar:
		ymdo = (di->ds.Cal.YMDOrder[0] == V4LEX_YMDOrder_None ? ci->Cntry[ci->CurX].ymdOrder[0] : di->ds.Cal.YMDOrder[0]) ;
		dnum = v_ParseCalendar(gpi->ctx,fp,vlt->Col[cx].PntTypeInfo,ymdo,VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,((di->ds.Cal.calFlags & VCAL_Flags_Historical) == 0)) ;
		if (dnum == VCAL_BadVal) ERR("Invalid calendar entry")
		PUTREAL(p,dnum) ;
		p->Bytes = V4PS_Calendar ; break ;
	      case V4DPI_PntType_UDate:
		ymdo = (di->ds.UDate.YMDOrder[0] == V4LEX_YMDOrder_None ? ci->Cntry[ci->CurX].ymdOrder[0] : di->ds.UDate.YMDOrder[0]) ;
		if ((p->Value.IntVal = v_ParseUDate(gpi->ctx,fp,vlt->Col[cx].PntTypeInfo,ymdo,((di->ds.UDate.calFlags & VCAL_Flags_Historical) == 0))) == VCAL_BadVal) ERR1(gpi->ctx->ErrorMsgAux) ;
		p->Bytes = V4PS_Int ;
		break ;
	    } ;
	   continue ;
field_err: /* See if we have error point - if so use it otherwise return error */
	   if (pt == vlt->Col[cx].PntType)		/* Are we looking at first dimension in column spec? */
	    { if (vlt->Col[cx].PntType1 != UNUSED)
	       { pt = vlt->Col[cx].PntType1 ; di = vlt->Col[cx].di1 ; p->AltDim = TRUE ; goto try_next_dim ; } ;
	    } ;
	   p = vlt->Col[cx].Err ;
	   if (p == NULL)
	    { if (errfld != NULL) UCstrcpy(errfld,errp) ;
	      if (labelfld != NULL) v_Msg(NULL,labelfld,"@%1U.%2U",vlt->Name,vlt->Col[cx].Name) ;
	      return(-(cx+1)) ;
	    } ;
	   memcpy(vlt->Col[cx].Cur,p,p->Bytes) ;	/* Take this as the current point for this column */
	   continue ;
	 } ;
	return(1) ;
}

ETYPE v4lex_NextCSVField(src,dst,dstMax,newSrc,strict,tcb)
  UCCHAR *src, *dst, **newSrc ;
  LENMAX dstMax ;
  LOGICAL strict ;
  struct V4LEX__TknCtrlBlk *tcb ;
{
  INDEX i ;
  
/*	Per CSV specs, ignore leading spaces */
	for(;*src==UClit(' ');src++) { } ;
	if (*src != UClit('"') || (!strict))	/* If not strict interpretation then just go to next comma */
	 { for(i=0;*src != UClit(',') && *src != UCEOS && i < dstMax;i++,src++)
	    { dst[i] = *src ; } ;
	   if (newSrc != NULL) *newSrc = (*src == UCEOS ? src : src+1) ;
	   if (i >= dstMax)
	    return(V4LEX_CSVError_TooBig) ;
	   for(;i>0 && dst[i-1]==UClit(' ');i--) { } ;
	   dst[i] = UCEOS ; 
	   return(V4LEX_CSVStatus_OK) ;
	 } ;
/*	Here to handle CSV value enclosed in quotes */
	for(i=0,src++;i<dstMax;i++,src++)
	 { if (*src == UCEOS)
	    { if (tcb == NULL) break ;		/* Hit end of line while in quoted string? Then continue column with next line */
	      tcb->ilvl[tcb->ilx].tpo->readLineOpts |= V4LEX_ReadLine_NestTable ;
	      if (!v4lex_ReadNextLine(tcb,tcb->ilvl[tcb->ilx].tpo,NULL,NULL))
	       { tcb->ilvl[tcb->ilx].tpo->readLineOpts &= (~V4LEX_ReadLine_NestTable) ; return(V4LEX_CSVError_InvQuote) ; } ;
	      tcb->ilvl[tcb->ilx].tpo->readLineOpts &= (~V4LEX_ReadLine_NestTable) ;
	      src = tcb->ilvl[tcb->ilx].input_ptr - 1 ;
/*	      Subtract from line counters because this new 'line' is really part of the column */
	      tcb->ilvl[tcb->ilx].current_line -- ; tcb->ilvl[tcb->ilx].total_lines -- ;
	      dst[i] = UCEOLbt ; continue ;
	    } ;
	   if (*src != UClit('"')) { dst[i] = *src ; continue ; } ;
/*	   Got a quote - if followed by another then keep one & continue else end of field */
	   src++ ;
	   if (*src != UClit('"')) break ;
	   dst[i] = *src ;
	 } ;
	if (newSrc != NULL) *newSrc = (*src == UCEOS ? src : src+1) ;
	if (i >= dstMax)
	 return(V4LEX_CSVError_TooBig) ;
	if (*src == UCEOS && src[-1] != UClit('"'))
	 return(V4LEX_CSVError_UnexpEOS) ;
/*	Maybe skip over some spaces to next comma */
	for(;*src==UClit(' ');src++) { } ;
	if (newSrc != NULL) *newSrc = (*src == UCEOS ? src : src+1) ;
	if (*src != UClit(',') && *src != UCEOS)
	 return(V4LEX_CSVError_InvQuote) ;
	dst[i] = UCEOS ; 
	return(V4LEX_CSVStatus_OK) ;
}

ETYPE v4lex_NextTABField(src,dst,dstMax,newSrc)
  UCCHAR *src, *dst, **newSrc ;
  LENMAX dstMax ;
{
  INDEX i ;
  
	if (*src != UClit('"'))
	 { for(i=0;*src != UClit('\t') && *src != UCEOS && i < dstMax;i++,src++)
	    { dst[i] = *src ; } ;
	   if (newSrc != NULL) *newSrc = (*src == UCEOS ? src : src+1) ;
	   if (i >= dstMax) return(V4LEX_CSVError_TooBig) ;
	   for(;i>0 && dst[i-1]==UClit(' ');i--) { } ;
	   dst[i] = UCEOS ; 
	   return(V4LEX_CSVStatus_OK) ;
	 } ;
/*	Here to handle TAB value enclosed in quotes */
	for(i=0,src++;i<dstMax && *src!=UCEOS;i++,src++)
	 { if (*src == UClit('\\'))		/* Got backslash - look for common escape sequences and handle */
	    { switch(*(src+1))
	       { default:		dst[i] = *src ; continue ;		/* Don't recognize - just take backslash */
	         case UClit('t'):	
	         case UClit('"'):	src++ ; dst[i] = *src ; continue ;
	       } ;
	    } ;
	   if (*src != UClit('"')) { dst[i] = *src ; continue ; } ;
/*	   Got a quote - if followed by another then keep one & continue else end of field */
	   src++ ;
	   if (*src != UClit('"')) break ;
	   dst[i] = *src ;
	 } ;
	if (newSrc != NULL) *newSrc = (*src == UCEOS ? src : src+1) ;
	if (i >= dstMax) return(V4LEX_CSVError_TooBig) ;
	if (*src == UCEOS && src[-1] != UClit('"'))
	 return(V4LEX_CSVError_UnexpEOS) ;
/*	Maybe skip over some spaces to next comma */
	if (newSrc != NULL) *newSrc = (*src == UCEOS ? src : src+1) ;
	if (*src != UClit('\t') && *src != UCEOS)
	 return(V4LEX_CSVError_InvQuote) ;
	dst[i] = UCEOS ; 
	return(V4LEX_CSVStatus_OK) ;
}


#define XMLTableNestMax 50
struct lcl__XMLTableNest {
  UCCHAR UName[V4DPI_DimInfo_DimNameMax+2] ; /* If not null then name of current undefined name - scan to closing */
  int NumLevels ;			/* Number of nested levels */
  struct {
    struct V4LEX__Table *vlt ;		/* Point to table at this level */
    int ColumnX ;			/* Current Column in table (UNUSED if none) */
  } Level[XMLTableNestMax] ;
    
} ;

/*	ok = v4lex_ReadXMLFile - Reads & parses (via Tables) XML data file	*/
/*	Call: res = v4lex_ReadXMLFile( ctx , filename , UCFile , contmask , lcptr , ltptr )
	  where res is return indicator V4LEX_ReadXML_xxx (ctx->ErrorMsgAux contains msg),
		ctx is current context,
		filename is filename referenced by fp,
		UCFile is open file pointer to the XML file,
		contmask is mask of errors (V4LEX_ReadXML_xxx) to be allowed,
		lcptr & ltptr if not NULL are updated with line & token counts	*/

#define NB for(;;) { if (*(++b) > 10) break ; if (*b == UClit('\t')) break ; \
 if (lcptr!=NULL) (*lcptr)++ ; if (v_UCReadLine(UCFile,UCRead_UC,xmlibuf,UCsizeof(xmlibuf),ctx->ErrorMsgAux) < 0) goto xml_eof ; \
 linenum++ ; b = xmlibuf ; b-- ; } ;
#define PUSHB --b ;
#define SKIPWS for(;;) { NB ; if (*b == UClit(' ') || *b == UClit('\t')) continue ; break ; } ;
#define SKIPCOM for(;;) { NB ; if (*b != UClit('-')) continue ; if (*(b+1) == UClit('>')) break ; } ; NB ;

#define MAXVALTOSHOW 20
int v4lex_ReadXMLFile(ctx,filename,UCFile,contmask,lcptr,ltptr)
  struct V4C__Context *ctx ;
  UCCHAR *filename ;
  struct UC__File *UCFile ;
  int contmask ;
  int *lcptr,*ltptr ;
{ struct V4LEX__Table *vlt,*cvlt ;				/* Current vlt pointer via lxtn */
  struct V4LEX__TknCtrlBlk *tcb ;				/* Token Control Block */
  struct V4LEX__TableHash *lth ;
  struct lcl__XMLTableNest lxtn ;
  union {
    UCCHAR alpha[V4DPI_DimInfo_DimNameMax+2] ;
   } name ;							/* Name buffer */
  int jump1st[128],jump[128] ;
  UCCHAR *b,*tp ; UCCHAR xmlibuf[V4LEX_Tkn_StringMax+1],valuebuf[V4LEX_Tkn_StringMax+1] ;
  int i,j,term,linenum,termflag,namehash,hx,cdata,singleentry ; UCCHAR msg[512] ;

/*	Initialize jump arrays */
	for(i=0;i<128;i++) { jump1st[i] = (jump[i] = 0) ; } ;
	for(i=UClit('a');i<=UClit('z');i++) { jump1st[i] = (jump[i] = 1) ; } ;
	for(i=UClit('A');i<=UClit('Z');i++) { jump1st[i] = (jump[i] = 1) ; } ;
	for(i=UClit('0');i<=UClit('9');i++) { jump[i] = 1 ; } ;
	jump[UClit('_')] = 1 ; jump[UClit(' ')] = 2 ; jump[UClit('>')] = 2 ; jump[UClit('=')] = 2 ; jump[UClit(':')] = 2 ;

	b = xmlibuf ; ZUS(xmlibuf) ; linenum = 0 ; tcb = NULL ;
	memset(&lxtn,0,sizeof lxtn) ;
	for(;;)
	 { SKIPWS
	   if (*b != UClit('<'))
	    { v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file(%2U) - expecting UClit('<')",linenum,filename) ; return(V4LEX_ReadXML_StructureErr) ; } ;
	   termflag = FALSE ;
	   NB
	   if (*b == UClit('!')) { SKIPCOM ; continue ; } ;
	   if (*b == UClit('/')) { termflag = TRUE ; } else { PUSHB } ;
//	   memset(&name,0,sizeof name) ;
	   for(i=0;;i++)
	    { NB
	      switch((*b > (UCCHAR)127 ? 2 : (i== 0 ? jump1st[*b] : jump[*b])))
	       { default:
		   v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file (%2U) - character(%3d) not valid for name",linenum,filename,i+1) ;
		   return(V4LEX_ReadXML_StructureErr) ;
		 case 1:	if (i<V4DPI_DimInfo_DimNameMax) name.alpha[i] = UCTOUPPER(*b) ; continue ;
		 case 2:	break ;	
	       } ; break ;
	    } ; name.alpha[i] = UCEOS ;
//	   namehash = name.num[0] + name.num[1] + name.num[2] + name.num[3] + name.num[5] + name.num[6] + name.num[7] + name.num[8] ;
//	   namehash = v_Hash32((char *)name.alpha,UCsizeof(name.alpha)*(sizeof(UCCHAR)),1) ;
	   VHASH32_FWD(namehash,name.alpha,UCstrlen(name.alpha)) ;
	   if (namehash < 0) namehash = -namehash ; if (namehash == 0) namehash = 1 ;
//	   name.alpha[i >= V4DPI_DimInfo_DimNameMax ? V4DPI_DimInfo_DimNameMax : i] = UCEOS ;
	   if (i >= V4DPI_DimInfo_DimNameMax)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"@*readXMLfile fail - line(%1d) file(%2U) - name(%3U) exceeds max of %4d characters\n",
			linenum,filename,name.alpha,V4DPI_DimInfo_DimNameMax) ;
	      if ((contmask & V4LEX_ReadXML_LimitErr) == 0) { return(V4LEX_ReadXML_LimitErr) ; }
	       else { vout_UCText(VOUT_Warn,0,ctx->ErrorMsgAux) ; } ;
	    } ;
	   cvlt = (lxtn.NumLevels <= 0 ? NULL : lxtn.Level[lxtn.NumLevels-1].vlt) ;
term_entry:
	   if (termflag)						/* If </name> then has to end current level */
	    { 
	      if (lxtn.UName[0] != UCEOS)				/* Scanning to matching undefined name? */
	       { if (UCstrcmp(lxtn.UName,name.alpha) == 0) { ZUS(lxtn.UName) ; } ;
	         for(;*b!=UClit('<');) { NB ; } ; continue ;			/* Found it - continue with next tag */
	       } ;
	      if (lxtn.NumLevels > 0 ? lxtn.Level[lxtn.NumLevels-1].ColumnX != UNUSED : FALSE)
	       { if (UCstrcmpIC(name.alpha,cvlt->Col[lxtn.Level[lxtn.NumLevels-1].ColumnX].Name) != 0)
		  { v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file(%2U) - </(%3U)> does not match current item name(%4U)",
			linenum,filename,name.alpha,vlt->Col[lxtn.Level[lxtn.NumLevels-1].ColumnX].Name) ;
		    return(V4LEX_ReadXML_StructureErr) ;
		  } ;
		 lxtn.Level[lxtn.NumLevels-1].ColumnX = UNUSED ;	/* No current column in current table */
		 continue ;						/* Parse next entry - must now see UClit('<') */
	       } ;
	      if (UCstrcmpIC(name.alpha,(cvlt == NULL ? UClit("??") : cvlt->Name)) !=0)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file(%2U) - </(%3U)> does not match current level name(%4U)",
			linenum,filename,name.alpha,(cvlt==NULL?UClit("<no level defined>"):cvlt->Name)) ;
	         return(V4LEX_ReadXML_StructureErr) ;
	       } ;
	      for(i=0;i<cvlt->Columns;i++)				/* Maybe supply defaults to some columns */
	       { if ((cvlt->Col[i].XMLFlags & V4LEX_Table_XML_Defined) != 0) continue ;
	         if (cvlt->Col[i].Dflt == NULL) continue ;		/* No default to supply */
		 if ((vlt->Col[i].ColumnType & (V4LEX_TableCT_Ignore|V4LEX_TableCT_Point|V4LEX_TableCT_Expression)) != 0) continue ;
		 if (cvlt->Col[i].Cur == NULL) {  vlt->Col[i].Cur = PTFROMHEAP ; } ;
		 memcpy(cvlt->Col[i].Cur,cvlt->Col[i].Dflt,cvlt->Col[i].Dflt->Bytes) ;
		 cvlt->Col[i].XMLFlags |= V4LEX_Table_XML_Defined ;	/* It's defined now */
	       } ;
	      if (cvlt->tpoT.Macro[0] != UCEOS)			/* Call ending macro if we got it */
	       { int s1 ;
		 if (tcb != NULL) { v4lex_ResetTCB(tcb) ; }
		  else { tcb = (struct V4LEX__TknCtrlBlk *)v4mm_AllocChunk(sizeof *tcb,FALSE) ; v4lex_InitTCB(tcb,0) ; }
		 v4lex_NestInput(tcb,NULL,UClit("Exit"),V4LEX_InpMode_String) ;
	         v_Msg(ctx,msg,"@%1U()",cvlt->tpoT.Macro) ;			/* Format macro call command */
		 v4lex_NestInput(tcb,NULL,msg,V4LEX_InpMode_String) ;
//		 s1 = ctx->NestedIsctEval ; s2 = ctx->NestedIntMod ;
		 s1 = ctx->rtStackX ;
		 v4eval_Eval(tcb,ctx,FALSE,0,FALSE,TRUE,FALSE,NULL) ;		/* Evaluate the macro */
		 ctx->rtStackX = s1 ;
//		 ctx->NestedIsctEval = s1 ; ctx->NestedIntMod = s2 ;
	       } ;
	      lxtn.NumLevels -- ;
	      cvlt->InXML = FALSE ;					/* Table no longer part of current XML hierarchy */
	      if (*b == UClit(':'))						/* Got a :label ? */
	       { for(i=0;;i++)
	          { NB
	            switch((*b > (UCCHAR)127 ? 2 : (i== 0 ? jump1st[*b] : jump[*b])))
		     { default:
			v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file (%2U) - character(%3d) not valid for name",linenum,filename,i+1) ;
			return(V4LEX_ReadXML_StructureErr) ;
		       case 1:	if (i<V4DPI_DimInfo_DimNameMax) name.alpha[i] = UCTOUPPER(*b) ; continue ;
		       case 2:	break ;	
		     } ; break ;
	          } ;
/*	         Got label in name.alpha - ??? DON'T KNOW WHAT TO DO HERE !!! ??? */
	       } ;
	      if (*b != UClit('>'))
	       { v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file(%2U) - </(%3U)... - does not terminate with UClit('>')",
			linenum,filename,name.alpha) ;
	         return(V4LEX_ReadXML_StructureErr) ;
	       } ;
	      continue ;						/* Parse next entry - must now see UClit('<') */
	    } ;
	   if (lxtn.UName[0] != UCEOS)					/* Scanning to matching undefined name? */
	    { for(;*b!=UClit('<');) { NB ; } ; continue ;			/* Not found yet - continue with next tag */
	    } ;
	   if (cvlt != NULL)
	    { hx = namehash % V4LEX_HASH_NUM ; lth = cvlt->lth ;
	      for(;;)
	       { if (lth->Entry[hx].Hash == namehash)
	          { i = lth->Entry[hx].ColX ; if (UCstrcmpIC(name.alpha,cvlt->Col[i].Name) == 0) break ; } ;
	         if (lth->Entry[hx].Hash == 0)				/* Name not found */
	          { i = V4LEX_Table_ColMax + 1 ; break ; }
	         hx = (hx + 1) % V4LEX_HASH_NUM ;
	       } ;
	    } ;
//	   for(i=0;cvlt!=NULL && i<V4LEX_Table_ColMax;i++)		/* Look up in current table (if have one) */
//	    { 
//static int cnt2 ; 
//cnt2++ ; if (cnt2%1000 == 0) printf("cnt2= %d\n",cnt2) ;
//	      if (strcmp(name.alpha,cvlt->Col[i].Name) == 0) break ;
//	    } ;
	   if ((cvlt != NULL) & (i < V4LEX_Table_ColMax))		/* Got column name */
	    { lxtn.Level[lxtn.NumLevels-1].ColumnX = i ;
	    } else							/* See if we got a table name */
	    { for(vlt=gpi->vlt;vlt!=NULL;vlt=vlt->link)		/* Search thru all defined */
	       { if (UCstrcmpIC(name.alpha,vlt->Name) == 0) break ; } ;
	      if (vlt == cvlt && vlt != NULL)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file(%2U) - cannot nest levels(%3s)\n",
			linenum,filename,name.alpha) ;
	         if ((contmask & V4LEX_ReadXML_NestErr) == 0) { return(V4LEX_ReadXML_NestErr) ; }
		  else { vout_UCText(VOUT_Warn,0,ctx->ErrorMsgAux) ; } ;
	       } ;
	      if (vlt == NULL && cvlt != NULL)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file(%2U) - <%3U> not column in table(%4U) nor is it a valid table name\n",
			linenum,filename,name.alpha,cvlt->Name) ;
	         if ((contmask & V4LEX_ReadXML_UndefErr) == 0) { return(V4LEX_ReadXML_UndefErr) ; }
		  else { vout_UCText(VOUT_Warn,0,ctx->ErrorMsgAux) ; UCstrcpy(lxtn.UName,name.alpha) ;
			 for(;*b!=UClit('<');) { NB ; } ; continue ;
		       } ;
	       } ;
	      if (vlt == NULL)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file(%2U) - <%3U> is not a valid table name",
			linenum,filename,name.alpha) ;
	         if ((contmask & V4LEX_ReadXML_UndefErr) == 0) { return(V4LEX_ReadXML_UndefErr) ; }
		  else { vout_UCText(VOUT_Warn,0,ctx->ErrorMsgAux) ; vout_NL(VOUT_Warn) ; UCstrcpy(lxtn.UName,name.alpha) ;
			 for(;*b!=UClit('<');) { NB ; } ; continue ;
		       } ;
	       } ;
	      if (lxtn.NumLevels >= XMLTableNestMax)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file (%2U) - exceeded maximum of %3d nested levels",linenum,filename,XMLTableNestMax) ;
		 return(V4LEX_ReadXML_StructureErr) ;
	       } ;
	      lxtn.Level[lxtn.NumLevels].vlt = (cvlt = vlt) ;		/* Define new level */
	      lxtn.Level[lxtn.NumLevels].ColumnX = UNUSED ;		/* (no current column) */
	      lxtn.NumLevels++ ;
	      for(i=0;i<cvlt->Columns;i++) { cvlt->Col[i].XMLFlags = 0 ; } ;
	      cvlt->InXML = TRUE ;					/* Table part of current XML hierarchy */
	      if (cvlt->lth == NULL)					/* Hash table need to be built? */
	       { cvlt->lth = (lth = (struct V4LEX__TableHash *)v4mm_AllocChunk(sizeof *lth,TRUE)) ;
	         for(i=0;i<cvlt->Columns;i++)
		  { if (lth->Count >= V4LEX_HASH_NUM)
		     { v_Msg(ctx,ctx->ErrorMsgAux,"XMLHashMax",V4LEX_HASH_NUM) ; return(V4LEX_ReadXML_LimitErr) ; } ;
		    memset(&name,0,sizeof name) ;
		    for(j=0;;j++) { name.alpha[j] = UCTOUPPER(cvlt->Col[i].Name[j]) ; if (name.alpha[j] == UCEOS) break ; } ;
//		     UCstrcpy(name.alpha,cvlt->Col[i].Name) ;
//		    namehash = name.num[0] + name.num[1] + name.num[2] + name.num[3] + name.num[5] + name.num[6] + name.num[7] + name.num[8] ;
//		    namehash = v_Hash32((char *)name.alpha,UCsizeof(name.alpha)*(sizeof(UCCHAR)),1) ;
		    VHASH32_FWD(namehash,name.alpha,UCstrlen(name.alpha)) ;
		    if (namehash < 0) namehash = -namehash ; if (namehash == 0) namehash = 1 ;
		    hx = namehash % V4LEX_HASH_NUM ;
		    for(;;)
		     { if (lth->Entry[hx].Hash == 0) break ;
		       hx = (hx + 1) % V4LEX_HASH_NUM ;
		     } ;
		    lth->Entry[hx].Hash = namehash ; lth->Entry[hx].ColX = i ; lth->Count++ ;
		  } ;
	       } ;
	    } ;

	   if (*b == UClit(':'))						/* Got a :label ? */
	    { for(i=0;;i++)
	       { NB
	         switch(i== 0 ? jump1st[*b] : jump[*b])
		  { default:
			v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file (%2U) - character(%3d) not valid for name",linenum,filename,i+1) ;
			return(V4LEX_ReadXML_StructureErr) ;
		    case 1:	if (i<V4DPI_DimInfo_DimNameMax) name.alpha[i] = UCTOUPPER(*b) ; continue ;
		    case 2:	break ;	
		  } ; break ;
	       } ;
/*	      Got label in name.alpha - ??? DON'T KNOW WHAT TO DO HERE !!! ??? */
	    } ;

	   for(;*b==UClit(' ');)						/* Parse any attribtes */
	    {


	      for(i=0;;i++)		/* Parse attribute name */
	       { NB
	         switch((*b > (UCCHAR)127 ? 2 : (i== 0 ? jump1st[*b] : jump[*b])))
		  { default:
			v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file (%2U) - character(%3d) not valid for name",linenum,filename,i+1) ;
			return(V4LEX_ReadXML_StructureErr) ;
		    case 1:	if (i<V4DPI_DimInfo_DimNameMax) name.alpha[i] = UCTOUPPER(*b) ; continue ;
		    case 2:	break ;	
		  } ; break ;
	       } ;
	      name.alpha[i<V4DPI_DimInfo_DimNameMax ? i : V4DPI_DimInfo_DimNameMax-1] = UCEOS ;
	      if (i >= V4DPI_DimInfo_DimNameMax)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file(%2U) - name(%3s) exceeds max of %4d characters",
			linenum,filename,name.alpha,V4DPI_DimInfo_DimNameMax) ;
	         if ((contmask & V4LEX_ReadXML_LimitErr) == 0) { return(V4LEX_ReadXML_LimitErr) ; }
	          else { vout_UCText(VOUT_Warn,0,ctx->ErrorMsgAux) ; vout_NL(VOUT_Warn) ; } ;
	       } ;
	      if (*b != UClit('='))						/* Now get ="value" */
	       { v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file(%2U) - attribute(%3s) not followed with UClit('=')",
			linenum,filename,name.alpha) ;
	         return(V4LEX_ReadXML_StructureErr) ;
	       } ;
	      NB
	      if (*b != UClit('"') && *b != UClit('\''))
	       { v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file(%2U) - attribute(%3U) value must be enclosed in quotes",
			linenum,filename,name.alpha) ;
	         return(V4LEX_ReadXML_StructureErr) ;
	       } ;
	      term = *b ;
	      for(i=0,tp=valuebuf;i<V4LEX_Tkn_StringMax;i++)
	       { NB if (*b == term) break ;
	         if (*b == UClit('&'))						/* Look for possible escape sequence (e.g. &lt;) */
	          { if (*(b+1) == UClit('l') && *(b+2) == UClit('t') && *(b+3) == UClit(';'))
	             { *(tp++) = UClit('<') ; b += 3 ; continue ; } ;
	            if (*(b+1) == UClit('g') && *(b+2) == UClit('t') && *(b+3) == UClit(';'))
	             { *(tp++) = UClit('>') ; b += 3 ; continue ; } ;
	            if (*(b+1) == UClit('a') && *(b+2) == UClit('m') && *(b+3) == UClit('p') && *(b+4) == UClit(';'))
	             { *(tp++) = UClit('&') ; b += 4 ; continue ; } ;
	            if (*(b+1) == UClit('a') && *(b+2) == UClit('p') && *(b+3) == UClit('o')  && *(b+4) == UClit('s') && *(b+5) == UClit(';'))
	             { *(tp++) = UClit('\'') ; b += 5 ; continue ; } ;
	            if (*(b+1) == UClit('q') && *(b+2) == UClit('u') && *(b+3) == UClit('o')  && *(b+4) == UClit('t') && *(b+5) == UClit(';'))
	             { *(tp++) = UClit('"') ; b += 5 ; continue ; } ;
	          } ;
	         *(tp++) = *b ;
	       } ;
	      if (i >= V4LEX_Tkn_StringMax)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file(%2U) - attribute(%3s) value exceeds max of %4d characters",
			linenum,filename,name.alpha,V4LEX_Tkn_StringMax) ;
	         if ((contmask & V4LEX_ReadXML_LimitErr) == 0) { return(V4LEX_ReadXML_LimitErr) ; }
	          else { vout_UCText(VOUT_Warn,0,ctx->ErrorMsgAux) ; vout_NL(VOUT_Warn) ; } ;
	       } ;
	      NB							/* Should be another space or UClit('>') */
	      *tp = UCEOS ;
	      for(i=0;i<cvlt->Columns;i++)				/* Look up in current table */
	       { if (UCstrcmpIC(name.alpha,cvlt->Col[i].Name) == 0) break ;
	       } ;
	      
	      if (ltptr != NULL) *ltptr += 3 ;				/* <tkn1>tkn2</tkn3> = 3 ! */

	      if (i < cvlt->Columns)
	       { if (v4lex_ReadNextLine_Table(valuebuf,i,NULL,NULL,msg,cvlt,NULL,0,tcb,NULL,UNUSED) < 0)
		  { UCstrcpy(&valuebuf[MAXVALTOSHOW],UClit("...")) ;
		    v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file(%2U) - %3s=\"%4s\" - %5U",
			linenum,filename,name.alpha,valuebuf,msg) ;
		    if ((contmask & V4LEX_ReadXML_ValueErr) == 0) { return(V4LEX_ReadXML_ValueErr) ; }
		     else { vout_UCText(VOUT_Warn,0,ctx->ErrorMsgAux) ; vout_NL(VOUT_Warn) ; } ;
		  } ;
		 cvlt->Col[i].XMLFlags |= V4LEX_Table_XML_Defined ;	/* This field now defined! */
	       } else
	       { UCstrcpy(&valuebuf[MAXVALTOSHOW],UClit("...")) ;
	         v_Msg(ctx,msg,"@[readXMLfile -  line(%1d) file(%2U) - %3U=\"%4U\" - ignored]",linenum,filename,name.alpha,valuebuf) ;
		 vout_UCText(VOUT_Warn,0,msg) ; vout_NL(VOUT_Warn) ;
	       } ;

	    } ;
	   if (*b == UClit('/'))						/* Got <name/> ? */
	    { termflag = TRUE ; NB ;
	      if (*(b+1) != UClit('<'))					/* Must follow with new name */
	       { v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file(%2U) - expecting UClit('<') to follow name(%3s/>)",
			linenum,filename,name.alpha) ;
	         return(V4LEX_ReadXML_StructureErr) ;
	       } ;
	    } ;
	   if (*b != UClit('>'))
	    { v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file(%2U) - expecting UClit('>') to follow name(%3s)",
			linenum,filename,name.alpha) ;
	      return(V4LEX_ReadXML_StructureErr) ;
	    } ;
	   if (lxtn.Level[lxtn.NumLevels-1].ColumnX == UNUSED)		/* If just started new level, call begin macro */
	    { if (cvlt->tpoT.BgnMacro[0] != UCEOS)			/* Best make sure we have one! */
	       { int s1 ;
		 if (tcb != NULL) { v4lex_ResetTCB(tcb) ; }
		  else { tcb = (struct V4LEX__TknCtrlBlk *)v4mm_AllocChunk(sizeof *tcb,FALSE) ; v4lex_InitTCB(tcb,0) ; }
		 v4lex_NestInput(tcb,NULL,UClit("Exit"),V4LEX_InpMode_String) ;
	         v_Msg(ctx,msg,"@%1U()",cvlt->tpoT.BgnMacro) ;			/* Format macro call command */
		 v4lex_NestInput(tcb,NULL,msg,V4LEX_InpMode_String) ;
//		 s1 = ctx->NestedIsctEval ; s2 = ctx->NestedIntMod ;
		 s1 = ctx->rtStackX ;
		 v4eval_Eval(tcb,ctx,FALSE,0,FALSE,TRUE,FALSE,NULL) ;		/* Evaluate the macro */
		 ctx->rtStackX = s1 ;
//		 ctx->NestedIsctEval = s1 ; ctx->NestedIntMod = s2 ;
	       } ;
	    } ;
	   SKIPWS								/* Now could get value or begin of another <xxx> */
	   if (*b == UClit('<'))
	    { PUSHB ;
	      if (termflag) { goto term_entry ; } else { continue ; } ;	/* If had <name/> then follow with section termination */
	    } ;
	   PUSHB ; cdata = FALSE ;
	   for(i=0,tp=valuebuf;i<V4LEX_Tkn_StringMax;i++)
	    { NB
	      if (i == 0 && *b == UClit('<') && *(b+1) == UClit('!') && *(b+2) == UClit('[') && *(b+3) == UClit('C') && *(b+4) == UClit('D')
		    && *(b+5) == UClit('A') && *(b+6) == UClit('T') && *(b+7) == UClit('A') && *(b+8) == UClit('['))
	       { cdata = TRUE ; b += 8 ; NB ;				/* Got <!![CDATA[ - turn on flag */
	       } ;
	      if (cdata)
	       { if (*b == UClit(']') && *(b+1) == UClit(']') && *(b+2) == UClit(']') && *(b+4) == UClit('>'))
	          { cdata = FALSE ; b += 4 ; } ;
		 *(tp++) = *b ; continue ; 
	       } ;
	      if (*b == UClit('<')) { PUSHB ; break ; } ;
	      if (*b == UClit('&'))						/* Look for possible escape sequence (e.g. &lt;) */
	       { if (*(b+1) == UClit('l') && *(b+2) == UClit('t') && *(b+3) == UClit(';'))
	          { *(tp++) = UClit('<') ; b += 3 ; continue ; } ;
	         if (*(b+1) == UClit('g') && *(b+2) == UClit('t') && *(b+3) == UClit(';'))
	          { *(tp++) = UClit('>') ; b += 3 ; continue ; } ;
	         if (*(b+1) == UClit('a') && *(b+2) == UClit('m') && *(b+3) == UClit('p') && *(b+4) == UClit(';'))
	          { *(tp++) = UClit('&') ; b += 4 ; continue ; } ;
	         if (*(b+1) == UClit('a') && *(b+2) == UClit('p') && *(b+3) == UClit('o')  && *(b+4) == UClit('s') && *(b+5) == UClit(';'))
	          { *(tp++) = UClit('\'') ; b += 5 ; continue ; } ;
	         if (*(b+1) == UClit('q') && *(b+2) == UClit('u') && *(b+3) == UClit('o')  && *(b+4) == UClit('t') && *(b+5) == UClit(';'))
	          { *(tp++) = UClit('"') ; b += 5 ; continue ; } ;
	       } ;
	      *(tp++) = *b ;
	    } ;
	   if (i >= V4LEX_Tkn_StringMax)
	    { UCstrcpy(&valuebuf[MAXVALTOSHOW],UClit("...")) ;
	      v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file(%2U) - value(%3s) exceeds max of %4d characters",
			linenum,filename,valuebuf,V4LEX_Tkn_StringMax) ;
	      if ((contmask & V4LEX_ReadXML_LimitErr) == 0) { return(V4LEX_ReadXML_LimitErr) ; }
	       else { vout_UCText(VOUT_Warn,0,ctx->ErrorMsgAux) ; vout_NL(VOUT_Warn) ; } ;
	    } ;
	   *tp = UCEOS ;
/*	   If no current column BUT current table only has one row then default to it */
	   if (lxtn.Level[lxtn.NumLevels-1].ColumnX == UNUSED && lxtn.Level[lxtn.NumLevels-1].vlt->Columns == 1)
	    { lxtn.Level[lxtn.NumLevels-1].ColumnX = 0 ;		/* Default to first column */
	      singleentry = TRUE ;					/* Remember we got single entry */
	    } else { singleentry = FALSE ; } ;
	   if (lxtn.Level[lxtn.NumLevels-1].ColumnX == UNUSED)
	    { UCstrcpy(&valuebuf[MAXVALTOSHOW],UClit("...")) ;
	      v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file(%2U) - Cannot have value(%3s) associated with Table(%4U)",
			linenum,filename,valuebuf,cvlt->Name) ;
	      if ((contmask & V4LEX_ReadXML_ValueErr) == 0) { return(V4LEX_ReadXML_ValueErr) ; }
	       else { vout_UCText(VOUT_Warn,0,ctx->ErrorMsgAux) ; vout_NL(VOUT_Warn) ; } ;
	    } else
	    { if (ltptr != NULL) *ltptr += 3  ;
	      i = lxtn.Level[lxtn.NumLevels-1].ColumnX ;
	      if (v4lex_ReadNextLine_Table(valuebuf,i,NULL,NULL,msg,cvlt,NULL,0,tcb,NULL,UNUSED) < 0)
	       { UCstrcpy(&valuebuf[MAXVALTOSHOW],UClit("...")) ;
	         v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file(%2U) - <%3U>%4s<%5s> - %6U",
			linenum,filename,cvlt->Col[i].Name,valuebuf,cvlt->Col[i].Name,msg) ;
		 if ((contmask & V4LEX_ReadXML_ValueErr) == 0) { return(V4LEX_ReadXML_ValueErr) ; }
		  else { vout_UCText(VOUT_Warn,0,ctx->ErrorMsgAux) ; vout_NL(VOUT_Warn) ; } ;
	       } ;
	      cvlt->Col[i].XMLFlags |= V4LEX_Table_XML_Defined ;	/* This field now defined! */
	      if (singleentry)						/* If single entry, pretent column not defined */
	       lxtn.Level[lxtn.NumLevels-1].ColumnX = UNUSED ;
	    } ;
	 } ;
xml_eof:
	if (lxtn.NumLevels != 0)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"@readXMLfile fail - line(%1d) file(%2U) - Premature EOF reached",linenum,filename) ;
	   return(V4LEX_ReadXML_StructureErr) ;
	 } ;
	return(V4LEX_ReadXML_OK) ;					/* If here, all went well */
}


/*	v4lex_CloseOffLevel - Closes off current tcb input level (usually a file)	*/
/*	Call: result = v4lex_CloseOffLevel( tcb , options )
	  where result is TRUE if OK, FALSE if not,
		options are any of V4LEX_CLOSELEVEL_xxx					*/

LOGICAL v4lex_CloseOffLevel(tcb,options)
  struct V4LEX__TknCtrlBlk *tcb ;		/* Token Control Block */
  int options ;					/* Read options */
{ struct V_COMPRESS_IncludeInfo *cii ;
  int i,max ;

	tcb->need_input_line = TRUE ;
	max = (options != 0 ? 999 : 1) ;	/* This is lame- when more than 1 option should clean up */
	for (i=1;i<=max;i++)
	 { 
	   if (tcb->ilvl[tcb->ilx].lvlSections != NULL) { v4mm_FreeChunk(tcb->ilvl[tcb->ilx].lvlSections) ; tcb->ilvl[tcb->ilx].lvlSections = NULL ; } ;
/*	   Free up input string buffer for this level UNLESS it is ilvl[0] & ilvl[0] is stdin */
	   if (tcb->ilx == 0 && tcb->ilvl[tcb->ilx].mode == V4LEX_InpMode_Stdin ? FALSE : tcb->ilvl[tcb->ilx].input_str != NULL)
	    { v4mm_FreeChunk(tcb->ilvl[tcb->ilx].input_str) ; tcb->ilvl[tcb->ilx].input_str = NULL ; } ;
	   switch (tcb->ilvl[tcb->ilx].mode)
	    { default: v4_error(V4E_INVTKNLVLMODE,tcb,"VCOM","ReadNextLine","INVTKNLVLMODE",
			"Invalid token level (%d) mode (%d)",tcb->ilx,tcb->ilvl[tcb->ilx].mode) ;
	      case V4LEX_InpMode_Error:
		return(tcb->ilx == 0) ;
	      case V4LEX_InpMode_Stdin:
		if (tcb->ilx <= 0) return(FALSE) ;
		tcb->ilx-- ; return(TRUE) ;
	      case V4LEX_InpMode_TempFile:
		v_UCFileClose(tcb->ilvl[tcb->ilx].UCFile) ; v4mm_FreeChunk(tcb->ilvl[tcb->ilx].UCFile) ;
		UCremove(tcb->ilvl[tcb->ilx].file_name) ;
		v4dpi_LocalDimAddEndBlock(gpi->ctx,FALSE) ;
		tcb->ilx-- ; continue ;
	      case V4LEX_InpMode_CmpFile:
		if ((cii = tcb->ilvl[tcb->ilx].cii) != NULL)
		 { if (cii->CmpBuf != NULL) v4mm_FreeChunk(cii->CmpBuf) ;
		   if (cii->Buf != NULL) v4mm_FreeChunk(cii->Buf) ;
		   v4mm_FreeChunk(cii) ; tcb->ilvl[tcb->ilx].cii = NULL ;
		 } ; /* Fall thru to close off the file */
	      case V4LEX_InpMode_File:
		v_UCFileClose(tcb->ilvl[tcb->ilx].UCFile) ; v4mm_FreeChunk(tcb->ilvl[tcb->ilx].UCFile) ;
//		fclose(tcb->ilvl[tcb->ilx].file) ;
		tcb->ilx-- ;
		v4dpi_LocalDimAddEndBlock(gpi->ctx,FALSE) ;
		continue ;
	      case V4LEX_InpMode_List:
		v4l_ListClose(gpi->ctx,tcb->ilvl[tcb->ilx].listp) ;
		tcb->ilx-- ; continue ;
	      case V4LEX_InpMode_MacArg:
	      case V4LEX_InpMode_String:
		tcb->ilx-- ; continue ;
	    } ;
	 } ;
	return(TRUE) ;
}

/*	v4lex_GetStdInput - Prompt & read from stdin		*/

static unsigned short cp437ToUC[] =
 { 0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00d4, 0x00e0, 0x00e5, 0x00e7, 0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
   0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9, 0x00ff, 0x00d6, 0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x20a7, 0x0192,
   0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba, 0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb,
   0x2591, 0x2592, 0x2593, 0x2592, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510,
   0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567,
   0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b, 0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2590,
   0x03b1, 0x00df, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03c4, 0x03a6, 0x039a, 0x03a9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229,
   0x2261, 0x00b1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00f7, 0x2248, 0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0
 } ;
LOGICAL v4lex_GetStdInput(prompt,result,maxlen)
  UCCHAR *prompt ;		/* Prompt string */
  UCCHAR *result ;		/* Result string to be updated */
  int maxlen ;			/* Maximum input length */
//  int parse_flag ;		/* TRUE iff called from V3 parser otherwise from runtime */
{ int i ; UCCHAR *b ;
  struct lcl__PreInput		/* Structure holds "pre-input" strings */
   { int used ;			/* Number used as tty input */
     int saved ;		/* Number currently saved */
     struct {
       UCCHAR *text ;		/* Pointer to text */
      } entry[20] ;
   } ;
  static struct lcl__PreInput *lpi = NULL ;

	if (result == NULL)		/* Maybe save input ? */
	 { if (lpi == NULL) lpi = (struct lcl__PreInput *)v4mm_AllocChunk(sizeof *lpi,TRUE) ;
	   lpi->entry[lpi->saved].text = v4mm_AllocUC(UCstrlen(prompt)+1) ;
	   UCstrcpy(lpi->entry[lpi->saved].text,prompt) ; lpi->saved++ ;
	   return(TRUE) ;			/* Store for later use */
	 } ;
	if (lpi != NULL)		/* Don't go get input if we already have it */
	 { if (lpi->used < lpi->saved)
	    { UCCHAR sbuf[512] ;
	      v_Msg(NULL,sbuf,"@%1U%2U\n",prompt,lpi->entry[lpi->used].text) ;	/* "Echo" it */
	      vout_UCText(VOUT_Prompt,0,sbuf) ;
	      UCstrcpy(result,lpi->entry[lpi->used].text) ;
	      lpi->used++ ; return(TRUE) ;
	    } ;
	 } ;
//#ifdef VMSOS
//	return(vms_input_tty(prompt,result,maxlen,parse_flag)) ;
//#endif

	vout_UCText(VOUT_Prompt,0,prompt) ;
#if defined V4_BUILD_TELNET_XFACE && defined V4UNICODE
	{ char buf8[1024] ;
	  if (fgets(buf8,sizeof buf8,stdin) == NULL)	/* Read non-unicode and convert */
	   return(FALSE) ;
	  for(i=0;i<maxlen;i++) { result[i] = buf8[i] ; if (buf8[i] == '\0') break ; } ;
	}
#else
	if(UCfgets(result,maxlen,stdin) == NULL)
	 { /* if (parse_flag) exit(exit_value=1) ; */
	   return(FALSE) ;
	 } ;
#endif
	i = UCstrlen(result) ; if (result[i-1] == UCNL) result[i-1] = UCEOS ;	/* Strip terminating newline if there */
/*	Console input coming in via Codepage 437 (USA). Translate characters 128-255 to corresponding Unicode */
	if (isatty(fileno(stdin)))
	 { for(b=result;*b!=UCEOS;b++)
	    { if (*b >= 0x80 && *b <= 0xFF)
	       *b = cp437ToUC[*b - 0x80] ;
	    } ;
	 } else
	 { INDEX j ; char *tbuf = v4mm_AllocChunk(i+1,FALSE) ; for(j=0;j<=i;j++) { tbuf[j] = result[j] ; } ;
	   UCUTF8toUTF16(result,maxlen,tbuf,i) ;
	 } ;
for (i=0;i<15;i++) { printf("%d %c %d %x\n",i, result[i], result[i], result[i]) ; } ;
	return(TRUE) ;
}

/*	vlex_FullPathNameOfFile - Returns full system path name of a file	*/
/*	Call: path = vlex_FullPathNameOfFile( filename , dststr , dststrmax )
	  where path is pointer to full path name (dststr).
		filename is v4 file name (logicals OK),
		dststr & dststrmax are output string (if NULL/0 then internal buffer used	*/

UCCHAR *vlex_FullPathNameOfFile(filename,dststr,dststrmax)
  UCCHAR *filename ;
  UCCHAR *dststr ; int dststrmax ;
{ UCCHAR *b, errbuf[512] ;
  static UCCHAR lclfile[V_FileName_Max] ;

	if (dststr == NULL) { dststr = lclfile ; dststrmax = V_FileName_Max ; } ;

#ifdef WINNT
	if (UCGetFullPathName(v_UCLogicalDecoder(filename,VLOGDECODE_Exists,0,errbuf),dststrmax,dststr,&b) == 0)
	 UCstrncpy(dststr,filename,dststrmax-1) ;
#else
	UCstrncpy(dststr,v_UCLogicalDecoder(filename,VLOGDECODE_Exists,0,errbuf),dststrmax-1) ;
#endif
	return(dststr) ;
}

/*	v4lex_TknArray_Setup - Sets up token "jump table" arrays	*/

LOGICAL v4lex_TknArray_Setup(dst_array,chars_to_set,set_value)
  char dst_array[],chars_to_set[],set_value ;
{ unsigned char i ;
	for (i=0;chars_to_set[i] != '\0';i++)
	  dst_array[chars_to_set[i]] = set_value ;
	return(TRUE) ;
}

/*	v4lex_NestInput - Initializes a new parser input level */
void v4lex_NestInput(tcb,UCFile,strarg,mode)
  struct V4LEX__TknCtrlBlk *tcb ;	/* Token control block */
  struct UC__File *UCFile ;
  UCCHAR *strarg ;			/* String Arg, file name, prompt, or input string based on below */
  int mode ;				/* Input mode- V4LEX_InpMode_xxx */
{ 
  int ilx,i ;

	if (tcb->ilx >= V4LEX_Tkn_InpLvlMax - 1)
	 v4_error(V4E_MAXLEVELS,0,"VCOM","NestInput","MAXLEVELS","Exceeded max number of nested parser input levels") ;
	ilx = ++(tcb->ilx) ;
	if (mode == V4LEX_InpMode_Commit)		/* Commit to this nested input (everything has already been done) */
	 { tcb->ilvl[ilx].mode = V4LEX_InpMode_String ;
	   ZUS(tcb->ilvl[ilx].file_name) ; tcb->ilvl[ilx].strptr = tcb->ilvl[ilx].input_str ;
	   return ;
	 } ;
/*	Zero out this ilvl[] (but save a couple of things if already set) */
	{ void *input_str = tcb->ilvl[ilx].input_str, *bcPt = tcb->ilvl[ilx].bcPt ;
	  memset(&tcb->ilvl[ilx],0,sizeof tcb->ilvl[ilx]) ;
 	  tcb->ilvl[ilx].input_str = input_str ; tcb->ilvl[ilx].bcPt = bcPt ;
	}



	tcb->ilvl[ilx].mode = mode ;
	if (tcb->ilvl[ilx].input_str == NULL) tcb->ilvl[ilx].input_str = v4mm_AllocUC(tcb->maxCharInpBuf) ;
	tcb->ilvl[ilx].echo = (ilx > 0 ? tcb->ilvl[ilx-1].echo-1 : 0) ;	/* Take echo flag as prior-1 */
	if (tcb->ilvl[ilx].echo >= V4LEX_EchoEveryStartNum-1) tcb->ilvl[ilx].echo++ ;	/* If >= then don't decrement */
	tcb->ilvl[ilx].ifxID_on_entry = (tcb->ifx > 0 ? tcb->ifs[tcb->ifx-1].ifxID : 0) ;
	if (tcb->ilvl[ilx].bcPt != NULL) ZPH(tcb->ilvl[ilx].bcPt) ;	/* Make sure no BindContext for this level (yet) */
/*	Initially set vcdIndex to same as current (about to be prior) level */
	tcb->ilvl[ilx].vcdIndex = (ilx > 1 ? tcb->ilvl[ilx-1].vcdIndex : UNUSED) ;
	switch (mode)
	 { default:	v4_error(V4E_INVMODE,0,"VCOM","NestInput","INVMODE","Invalid mode") ;
	   case V4LEX_InpMode_Stdin:
		ZUS(tcb->ilvl[ilx].file_name) ; UCstrcpy(tcb->ilvl[ilx].prompt,strarg) ; break ;
	   case V4LEX_InpMode_CmpFile:
		tcb->ilvl[ilx].cii = v4mm_AllocChunk(sizeof *tcb->ilvl[ilx].cii,TRUE) ;
		/* Fall thru to _File, cii components allocated as needed */
	   case V4LEX_InpMode_TempFile:
	   case V4LEX_InpMode_File:
//		tcb->ilvl[ilx].file = file_ptr ;
		tcb->ilvl[ilx].UCFile = v4mm_AllocChunk(sizeof(struct UC__File),FALSE) ; *tcb->ilvl[ilx].UCFile = *UCFile ;
		UCstrcpy(tcb->ilvl[ilx].file_name,strarg) ; tcb->ilvl[ilx].UCFile->wantEOL = FALSE ;
		v4dpi_LocalDimAddEndBlock(gpi->ctx,TRUE) ;
/*		Add this file to process source file list */
		if (gpi->vcd == NULL) { gpi->vcd = v4mm_AllocChunk(sizeof *gpi->vcd,TRUE) ; } ;
		for(i=0;i<gpi->vcd->fileCount;i++) { if (UCstrcmp(strarg,gpi->vcd->File[i].fileName) == 0) break ; } ;
		if (i >= gpi->vcd->fileCount)
		 { UCCHAR *bp,errmsg[512] ; CALENDAR calDT ; int uDT,uDate,uTime ;
//#ifdef WINNT
//		   struct _stat stat_buf ;
//#else
//		   struct stat stat_buf ;
//#endif
#if defined WINNT && defined V4UNICODE
		   struct _stat stat_buf ;
#else
		   struct stat stat_buf ;
#endif
		   if (gpi->vcd->fileCount >= V4LEX__CompileDir_Max) break ;
		   bp = v_UCLogicalDecoder(strarg,VLOGDECODE_Exists,0,errmsg) ;
		   if (bp != NULL) 
		    { UCstrcpy(gpi->vcd->File[i].fileName,bp) ;
		      if (UCstat(bp,&stat_buf) == -1) { calDT = 0 ; }
		       else { uDT = stat_buf.st_mtime - (60*gpi->MinutesWest) - TIMEOFFSETSEC ;
//			      uDate = (uDT / VCAL_SecsInDay) + VCAL_UDTUDateOffset ;
			      uDate = UDTtoUD(uDT) ;
			      uTime = (uDT % VCAL_SecsInDay) ;
			      calDT = (double)(VCal_MJDOffset + uDate) + (uTime != 0 ? ((double)uTime / (double)VCAL_SecsInDay) : VCAL_MidnightValue) ;
			    } ;
		      gpi->vcd->File[i].fileUpdCal = calDT ; gpi->vcd->File[i].spwHash64 = gpi->spwHash64 ;
		    } else { UCstrcpy(gpi->vcd->File[i].fileName,strarg) ; gpi->vcd->File[i].fileUpdCal = 0 ; } ;
		   gpi->vcd->fileCount++ ;
		 } ;
		tcb->ilvl[ilx].vcdIndex = i ;
		break ;
	   case V4LEX_InpMode_RetEOF:
		break ;				/* This is just to indicate to ReadNextLine to return EOF when we get here (not prompt from stdin) */
	   case V4LEX_InpMode_QString:
		if (UCstrlen(strarg) >= tcb->maxCharInpBuf)
		 v4_error(V4E_INPSTRTOOBIG,tcb,"VCOM","NestInput","INPSTRTOOBIG","Nested input line length (%d) exceeds maximum allowed (%d)",
			UCstrlen(strarg),tcb->maxCharInpBuf) ;
		ZUS(tcb->ilvl[ilx].file_name) ; tcb->ilvl[ilx].strptr = tcb->ilvl[ilx].input_str ;
		UCstrcpy(tcb->ilvl[ilx].strptr,UClit("\"")) ; UCstrcat(tcb->ilvl[ilx].strptr,strarg) ; UCstrcat(tcb->ilvl[ilx].strptr,UClit("\"")) ;
		tcb->ilvl[ilx].mode = V4LEX_InpMode_String ; break ;
	   case V4LEX_InpMode_Stage:
		tcb->ilx-- ;			/* Just gets everything ready for nested (string) input */
		return ;
	   case V4LEX_InpMode_List:
		tcb->ilvl[ilx].listp = (struct V4L__ListPoint *)strarg ;
		break ;
	   case V4LEX_InpMode_MacArg:
	   case V4LEX_InpMode_StringML:
	   case V4LEX_InpMode_String:

//printf("ifx=%d, ilx=%d, onentry=%d (push: %s)\n",tcb->ifx,tcb->ilx,tcb->ilvl[tcb->ilx].ifxID_on_entry,(strarg==NULL ? "" : UCretASC(strarg))) ;
		if (UCstrlen(strarg) >= tcb->maxCharInpBuf)
		 v4_error(V4E_INPSTRTOOBIG,tcb,"VCOM","NestInput","INPSTRTOOBIG","Nested input line length (%d) exceeds maximum allowed (%d)",
			UCstrlen(strarg),tcb->maxCharInpBuf) ;
		ZUS(tcb->ilvl[ilx].file_name) ; tcb->ilvl[ilx].strptr = tcb->ilvl[ilx].input_str ;
/*		Copy prior vlt if doing StringML (we came from nested ReadNextLine where Filter is returning multiple lines) */
		if (mode == V4LEX_InpMode_StringML && ilx > 1) tcb->ilvl[ilx].vlt = tcb->ilvl[ilx-1].vlt ;
		UCstrcpy(tcb->ilvl[ilx].strptr,strarg) ; break ;
	   case V4LEX_InpMode_StringPtr:			/* USE WITH CARE - Trash tcb after we are done. Don't want input_str to be freed because we don't know what it points to */
		tcb->ilvl[ilx].input_str = strarg ;
		ZUS(tcb->ilvl[ilx].file_name) ; tcb->ilvl[ilx].strptr = tcb->ilvl[ilx].input_str ;
/*		Copy prior vlt if doing StringML (we came from nested ReadNextLine where Filter is returning multiple lines) */
		if (mode == V4LEX_InpMode_StringML && ilx > 1) tcb->ilvl[ilx].vlt = tcb->ilvl[ilx-1].vlt ;
		break ;
	 } ;
	tcb->need_input_line = TRUE ;		/* Force read from new level */
	return ;
}

/*	v4lex_BindArgs - Binds arguments to macarg.value based on position/name	*/
/*	Call: ok = v4lex_BindArgs(tcb)
	  where ok is TRUE if all is well, FALSE if error (tcb->string),
		tcb is current tcb (tcb->ilvl[tcb->ilx] is updated)		*/

LOGICAL v4lex_BindArgs(tcb)
  struct V4LEX__TknCtrlBlk *tcb ;	/* Token control block */
{ UCCHAR argbuf[V4LEX_Tkn_ArgLineMax+10],name[100],tname[100] ;
  int i,ax ; UCCHAR *c,*a,*ap ;

	for(i=0;i<V4LEX_Tkn_ArgMax;i++) { tcb->ilvl[tcb->ilx].macarg[i].value = 0 ; } ; /* Zap all arguments */
	if (tcb->ilvl[tcb->ilx].arglist[0] == UCEOS) return(TRUE) ;	/* Nothing to bind ? */
	ZUS(argbuf) ; a = argbuf ; ZUS(name) ; ax = 0 ;
	for(c=tcb->ilvl[tcb->ilx].arglist,ap = c;;)
	 { 
	   switch (*c)							/* Look at next character */
	    { default: *(a++) = *(c++) ; break ;			/* Nothing special - copy */
	      case UClit('\001'): ap = NULL ; c++ ; break ;				/* Got <> - no argument */
	      case UCEOS:
	      case UCNL:						/* Got comma - handle this argument */
		if (UCempty(name))					/* Did we get a name ? */
		 { tcb->ilvl[tcb->ilx].macarg[ax++].value = ap ;	/* Just save pointer to argument */
		   if (*c == 0) goto out_loop ;
		   a = argbuf ; ZUS(argbuf) ;				/* Reset argbuf */
		   ap = ++c ; break ;
		 } ;
		for(i=0;name[i]==UClit(' ');i++) { } ; if (i > 0) UCstrcpy(name,&name[i]) ;
		for(i=UCstrlen(name)-1;name[i]==UClit(' ');i--) { name[i] = UCEOS ; } ; /* Get rid of any trailing spaces */
		if (tcb->ilvl[tcb->ilx].HaveMacroArgNames > 1)		/* If >1 then have default argument prefix- check */
		 { if (name[0] != tcb->ilvl[tcb->ilx].HaveMacroArgNames)
		    { tname[0] = tcb->ilvl[tcb->ilx].HaveMacroArgNames ; tname[1]= UCEOS ; UCstrcat(tname,name) ; UCstrcpy(name,tname) ; } ;
		 } ;
		for(ax=0;ax<V4LEX_Tkn_ArgMax;ax++)			/* Search for the name */
		 { if (UCstrcmpIC(tcb->ilvl[tcb->ilx].macarg[ax].name,name) == 0) break ; } ;
		if (ax >= V4LEX_Tkn_ArgMax)
		 { if (tcb->ilvl[tcb->ilx].HaveMacroArgNames > 1)
		    { tname[0] = tcb->ilvl[tcb->ilx].HaveMacroArgNames ; tname[1] = UCEOS ; UCstrcat(tname,name) ; UCstrcpy(name,tname) ; } ;
		   for(ax=0;ax<V4LEX_Tkn_ArgMax;ax++)			/* Try sticking prefix on again */
		    { if (UCstrcmpIC(tcb->ilvl[tcb->ilx].macarg[ax].name,name) == 0) break ; } ;
		   if (ax >= V4LEX_Tkn_ArgMax)
		    { v_Msg(NULL,tcb->UCstring,"TknMacBadName",&name[1]) ; return(FALSE) ; } ;
//		      v4_error(V4E_NOARGNAMEINMAC,tcb,"VCOM","BindArgs","NOARGNAMEINMAC","No such argument name (%s) for this macro",name) ;
		 } ;
		tcb->ilvl[tcb->ilx].macarg[ax++].value = ap ;		/* Bind in proper slot */
		if (*c == UCEOS) goto out_loop ;
		ap = ++c ;						/* Next argument begins on next byte! */
		a = argbuf ; ZUS(argbuf) ;				/* Reset argbuf */
		break ;
	      case UClit('\002'):
	       *a = UCEOS ;
	       for(i=0;;i++)
	        { 
		  if (i >= UCsizeof(name))
		   { name[UCsizeof(name) - 2] = UCEOS ; v_Msg(NULL,tcb->UCstring,"TknMacName",name,UCsizeof(name)) ; return(FALSE) ;
		   } ;
		  name[i] = UCTOUPPER(argbuf[i]) ; if (argbuf[i] == UCEOS) break ;
		} ;
	       a = argbuf ; ap = ++c ; break ;	/* Got "=" - save name */
	    } ;
	 } ;
out_loop:
	tcb->ilvl[tcb->ilx].BoundMacroArgs = TRUE ;			/* Flag as done */
	return(TRUE) ;
}

/*	v4lex_InitTCB - Sets up for a new parse environment	*/
struct V4LEX__TknCtrlBlk *v4lex_InitTCB(tcb,flags)
  struct V4LEX__TknCtrlBlk *tcb ;	/* Token control block */
  int flags ;
{ INDEX i ;

	memset(tcb,0,sizeof *tcb) ;
/*	Set up some other stuff */
	tcb->ilx = 0 ; tcb->maxCharInpBuf = V4LEX_Tkn_InpStrMax ;
	tcb->ilvl[0].mode = ((flags & V4LEX_TCBINIT_NoStdIn) ? V4LEX_InpMode_Error : V4LEX_InpMode_Stdin) ;
//VEH090207 This should be set to null (see below)	tcb->ilvl[0].input_ptr = tcb->ilvl[0].input_str ;
	for(i=0;i<V4LEX_Tkn_InpLvlMax;i++)
	 { tcb->ilvl[i].input_str = NULL ; } ;
	tcb->ilvl[0].input_str = v4mm_AllocUC(V4LEX_Tkn_ArgLineMax) ; ZUS(tcb->ilvl[0].input_str) ;
	tcb->need_input_line = TRUE ; tcb->check_initialized = TRUE ;
	UCstrcpy(tcb->ilvl[0].prompt,UClit("V4>")) ;
	tcb->default_radix = 10 ; tcb->tcb_lines = (tcb->tcb_tokens = 0) ;
	for(i=0;i<V4LEX_Tkn_PParamMax;i++)
	 { tcb->poundparam[i].numvalue = V4LEX_Tkn_PPNumUndef ; } ;	/* Set up undefined value */
/*	And finally return pointer to environment */
	return(tcb) ;
}

void v4lex_ResetTCB(tcb)
  struct V4LEX__TknCtrlBlk *tcb ;	/* Token control block */
{ INDEX i ;

	tcb->ilx = 0 ;
	tcb->need_input_line = TRUE ; tcb->check_initialized = TRUE ;
	UCstrcpy(tcb->ilvl[0].prompt,UClit("V4>")) ;
	tcb->default_radix = 10 ; tcb->tcb_lines = (tcb->tcb_tokens = 0) ;
	for(i=0;i<V4LEX_Tkn_PParamMax;i++)
	 { tcb->poundparam[i].numvalue = V4LEX_Tkn_PPNumUndef ; } ;	/* Set up undefined value */
}

void v4lex_FreeTCB(tcb)
  struct V4LEX__TknCtrlBlk *tcb ;	/* Token control block */
{ INDEX i ;
	for(i=0;i<V4LEX_Tkn_InpLvlMax;i++)
	 { if(tcb->ilvl[i].input_str != NULL) v4mm_FreeChunk(tcb->ilvl[i].input_str) ; } ;
	v4mm_FreeChunk(tcb) ;
}

/*	v4lex_SetPrompt - Sets prompt for current level		*/
/*	Call: v4lex_SetPrompt( tcb , prompt)
	  where tcb is pointer to control block,
		prompt is string prompt				*/

void v4lex_SetPrompt(tcb,prompt)
  struct V4LEX__TknCtrlBlk *tcb ;
  UCCHAR *prompt ;
{
	UCstrcpy(tcb->ilvl[tcb->ilx].prompt,prompt) ;		/* Copy prompt */
	tcb->need_input_line = TRUE ;				/* Force new input line */
}

/*	R E V E R S E   P O L I S H   P A R S E R			*/

/*	v4lex_RevPolParse - Parse input stream into reverse polish	*/
/*	Call: result = v4lex_RevPolParse( rpp , tcb , flags , ctx)
	  where result is TRUE if OK, FALSE if error (in ctx->ErrorMsgAux)
		rpp is pointer to V4LEX__RevPolParse structure to be updated,
		tcb is token control block with input stream,
		flags are optional parsing flags- V4LEX_RPPFlag_xxx,
		ctx is current context					*/

LOGICAL v4lex_RevPolParse(rpp,tcb,flags,ctx)
  struct V4LEX__RevPolParse *rpp ;
  struct V4LEX__TknCtrlBlk *tcb ;
  int flags ;
  struct V4C__Context *ctx ;
{
  struct V4LEX__RevPolParse op ;				/* Temp for holding operators */
  P *ipt,isctpt ;
  struct V4LEX__TknCtrlBlk *ltcb ;
  int got_value,tknflags,ppflags,ptbegin,i,StartingLine ;

	rpp->Count = 0 ;
	op.Count = 0 ;						/* Reset & push "begin-of-expression" */
	op.Token[op.Count].Type = V4LEX_TknType_Punc ;
	op.Token[op.Count].Prec = 1 ;
	op.Token[op.Count++].OpCode = V_OpCode_BOX ;
	got_value = FALSE ;
	ppflags = ((flags & V4LEX_RPPFlag_BraceDelim) != 0 ? V4DPI_PointParse_InV4Exp : 0) ;
	if ((flags & V4LEX_RPPFlag_V4IMEval) != 0) ppflags |= V4DPI_PointParse_InV4Exp ;
	if ((flags & V4LEX_RPPFlag_NotADim) != 0) ppflags |= V4DPI_PointParse_NotADim ;
	tknflags = ((flags & V4LEX_RPPFlag_EOL) != 0 ? V4LEX_Option_RetEOL : 0) ;
	if ((flags & V4LEX_RPPFlag_Excel) != 0) tknflags |= V4LEX_Option_Excel ;
	StartingLine = tcb->ilvl[tcb->ilx].current_line ;
	for(;op.Token[op.Count-1].OpCode != V_OpCode_EOX;)
	 { v4lex_NextTkn(tcb,tknflags) ;
	   if (tcb->type == V4LEX_TknType_Error)
	    { UCstrcpy(ctx->ErrorMsgAux,tcb->UCstring) ; return(FALSE) ; } ;

/*	If have any recognition blocks then try pattern match now */
	if ((ppflags & V4DPI_PointParse_InV4Exp) && (tcb->opcode == V_OpCode_QString || tcb->opcode == V_OpCode_Keyword))
	 { struct V4DPI__RecogBlock *lrcgb ;
	   struct V4DPI__Point tp1,tp2 ;
	   UCCHAR tbuf[V4DPI_AlphaVal_Max+100] ;
	   regmatch_t matches[10];
	   int j,n ;
	   for(lrcgb=gpi->rcgb;lrcgb!=NULL;lrcgb=lrcgb->nrcgb)
	    { i = vregexp_RegExec(&lrcgb->rpb,UCretASC(tcb->UCstring),10,matches,0) ;
	      if (i) continue ;			/* This pattern did not match - try next */
	      for(j=0,i=0;;i++,j++)
	       { int len ;
	         tbuf[j] = lrcgb->subbuf[i] ; if (tbuf[j] == UCEOS) break ;
	         if (tbuf[j] > 10) continue ;	/* Got a substitution (\0 .. \9)+1 */
		 n = tbuf[j]-1 ; if (matches[n].rm_so == -1) continue ; 
		 len= matches[n].rm_eo - matches[n].rm_so ;
		 UCstrncpy(&tbuf[j],&tcb->UCstring[matches[n].rm_so],len) ;
		 j += (len-1) ;
	       } ;
	      if (lrcgb->Flags & V4DPI_RegBlock_Eval)
	       { struct V4LEX__TknCtrlBlk *ltcb ; ltcb = v4mm_AllocChunk(sizeof *ltcb,FALSE) ;
		 v4lex_InitTCB(ltcb,0) ; v4lex_NestInput(ltcb,NULL,tbuf,V4LEX_InpMode_String) ;
		 i = v4dpi_PointParse(ctx,&tp1,ltcb,V4DPI_PointParse_RetFalse) ; v4lex_FreeTCB(ltcb) ;
		 if (!i) return(FALSE) ;
		 if (v4dpi_IsctEval(&tp2,&tp1,ctx,0,NULL,NULL) == NULL)
		  { v_Msg(ctx,ctx->ErrorMsgAux,"ParseRecog1",tcb->UCstring,tbuf) ; return(FALSE) ; } ;
//		  PERR2("Recognition evaluation failed: %s => %s => failed",tcb->string,tbuf) ;
		  if (lrcgb->Flags & V4DPI_RegBlock_Trace)
		   { 
		     v_Msg(ctx,ctx->ErrorMsgAux,"@*%1U => %2U => %3P\n",tcb->UCstring,tbuf,&tp2) ; vout_UCText(VOUT_Trace,0,ctx->ErrorMsgAux) ;
//		     v4dpi_PointToString(ASCTBUF1,&tp2,ctx,-1) ;
//		     sprintf(v4mbuf,"*%s => %s => %s\n",tcb->string,tbuf,ASCTBUF1) ; vout_Text(VOUT_Trace,0,v4mbuf) ;
		   } ;
/*		 Convert to string to process within { ... } */
		 v4dpi_PointToString(tbuf,&tp2,ctx,V4DPI_FormatOpt_Echo) ;
		 v4lex_NestInput(tcb,NULL,tbuf,V4LEX_InpMode_String) ;
		 goto next_token ; ;
	       } ;
	      if (lrcgb->Flags & V4DPI_RegBlock_Trace)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"@*%1U => %2U\n",tcb->UCstring,tbuf) ; vout_UCText(VOUT_Trace,0,ctx->ErrorMsgAux) ; } ;
	      if (tbuf[0] != UCEOS) v4lex_NestInput(tcb,NULL,tbuf,V4LEX_InpMode_String) ;
	      goto next_token ;
	    } ;
	 } ;

   
	   if (tcb->ilvl[tcb->ilx].current_line > StartingLine && tcb->ilvl[tcb->ilx].mode == V4LEX_InpMode_File)
	    { StartingLine = tcb->ilvl[tcb->ilx].current_line ;
	      if ((ppflags & V4DPI_PointParse_InV4Exp) && tcb->ilvl[tcb->ilx].indent_line == 0)
	       { v_Msg(ctx,ctx->ErrorMsgAux,"RevPolContLine") ; return(FALSE) ; } ;
	    } ;
	   if (tcb->opcode == V_OpCode_RBrace && (flags & V4LEX_RPPFlag_BraceDelim) != 0)
	    tcb->opcode = V_OpCode_EOX ;			/* "}" ends the expression */
	   if (tcb->opcode == V_OpCode_RBraceSlash && (flags & V4LEX_RPPFlag_BraceDelim) != 0)
	    { tcb->opcode = V_OpCode_EOX ;			/* "}/" ends the expression */
	      tcb->ilvl[tcb->ilx].input_ptr-- ;			/* Very bad code to back up to the "/" to parse separately */
	    } ;
	   tknflags &= (~V4LEX_Option_ForceKW) ;
	   if (tcb->type == V4LEX_TknType_EOL)
	    { tcb->opcode = V_OpCode_EOX ; tcb->prec = 12 ; goto check_op ; } ;
	   ptbegin = FALSE ;
	   switch (tcb->opcode)
	    { default:			break ;
	      case V_OpCode_Langle:	ptbegin = !got_value ; break ;	/* Begin of point if we don't alread have value */
	      case V_OpCode_NCLBracket:
	      case V_OpCode_LBracket:
	      case V_OpCode_Dot:
	      case V_OpCode_NCDot:
	      case V_OpCode_AtSign:	ptbegin = TRUE ; break ;	/* Valid begin of points */
	      case V_OpCode_QString:
	      case V_OpCode_DQString:
	      case V_OpCode_Keyword:	break ;
	      case V_OpCode_RBrace:	v_Msg(ctx,ctx->ErrorMsgAux,"RevPolSyntax") ; return(FALSE) ;
	      case V_OpCode_DollarLParen:
	      case V_OpCode_DolDolLParen:
/*					Looks like we got '$(...' or '$$(...' - parse as point */
					rpp->Token[rpp->Count].ValPtr = (char *)v4mm_AllocChunk(sizeof(struct V4DPI__Point),FALSE) ;
					v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
					if (!v4dpi_PointParse(ctx,(P *)rpp->Token[rpp->Count].ValPtr,tcb,ppflags|V4DPI_PointParse_RetFalse))
					 return(FALSE) ;
					rpp->Token[rpp->Count].Type = V4LEX_TknType_Isct ; rpp->Token[rpp->Count].OpCode = V_Opcode_V4Point ;
					rpp->Count ++ ; if (rpp->Count >= V4LEX_RPP_TokenMax) { v_Msg(ctx,ctx->ErrorMsgAux,"RevPolTknMax") ; return(FALSE) ; } ;
					got_value = TRUE ;
					continue ;
	      case V_OpCode_DollarSign:	{ UCCHAR symName[V4LEX_Tkn_KeywordMax+1] ;
					   v4lex_NextTkn(tcb,0) ;		/* Get symbol after '$' */
					   if (tcb->opcode != V_OpCode_Keyword) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIMNotKW",0) ; return(FALSE) ; } ;
					   UCstrcpy(symName,tcb->UCkeyword) ; v4lex_NextTkn(tcb,0) ;
					   if (tcb->opcode == V_OpCode_ColonEqual) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIPrsSymAsgn") ; return(FALSE) ; } ;
					   v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
					   rpp->Token[rpp->Count].ValPtr = (char *)v4mm_AllocChunk(sizeof(struct V4DPI__Point),FALSE) ;
					   if (!v4dpi_PushLocalSym(ctx,symName,(P *)rpp->Token[rpp->Count].ValPtr,NULL,0)) return(FALSE) ;
					   rpp->Token[rpp->Count].Type = V4LEX_TknType_Point ; rpp->Token[rpp->Count].OpCode = V_Opcode_V4Point ;
					   rpp->Count ++ ; if (rpp->Count >= V4LEX_RPP_TokenMax) { v_Msg(ctx,ctx->ErrorMsgAux,"RevPolTknMax") ; return(FALSE) ; } ;
					   got_value = TRUE ;
					   goto next_token ;
					}

	    } ;
	   if (OPCODEISVALUE(tcb->opcode) || (tcb->opcode==V_OpCode_LParen || tcb->opcode == V_OpCode_NCLParen) || ptbegin)	/* Got a value */
	    { if (got_value)					/* Two values in a row - what to do? */
	       { if (flags & V4LEX_RPPFlag_ImplyAnd)
		  { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
		    tcb->opcode = V_OpCode_Ampersand ; tcb->prec = 22 ; tcb->lookahead_flags = -1 ;
		    tcb->type = V4LEX_TknType_Punc ; UCstrcpy(tcb->UCkeyword,UClit("&")) ; goto check_op ;
		  } else { if (ctx != NULL) v_Msg(ctx,ctx->ErrorMsgAux,"RevPol2Vals") ; return(FALSE) ; } ;
	       } ;
	    } ;
	   if (OPCODEISVALUE(tcb->opcode))			/* Got a value */
	    { switch (tcb->type)
	       { case V4LEX_TknType_Keyword:
			if (flags & V4LEX_RPPFlag_Excel && *tcb->ilvl[tcb->ilx].input_ptr == '(')
			 { op.Token[op.Count].Type = V4LEX_TknType_Function ;
			   UCstrcpy(op.Token[op.Count].AlphaVal,tcb->UCkeyword) ;
			   op.Token[op.Count].Prec = 30 ; op.Token[op.Count].OpCode = 999 ; op.Count ++ ;
			   got_value = TRUE ;
			   rpp->Token[rpp->Count].Type = V4LEX_TknType_EOA ;	/* Mark for arguments */
			   rpp->Token[rpp->Count].OpCode = V_Opcode_EOA ;
			   rpp->Count ++ ; if (rpp->Count >= V4LEX_RPP_TokenMax) { v_Msg(ctx,ctx->ErrorMsgAux,"RevPolTknMax") ; return(FALSE) ; } ;
			   got_value = FALSE ;
			   goto next_token ;					/* Push these immediately */
			 } ;
			if (flags & V4LEX_RPPFlag_V4IMEval)
			 { 
//			   if (ctx == NULL) v4_error(V4E_RPPNOCTX,tcb,"VCOM","RPP","RPPNOCTX","Cannot parse V4 Point - no context specified") ;
			   v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
			   rpp->Token[rpp->Count].ValPtr = (char *)v4mm_AllocChunk(sizeof(struct V4DPI__Point),FALSE) ;
			   if (!v4dpi_PointParse(ctx,(P *)rpp->Token[rpp->Count].ValPtr,tcb,ppflags|V4DPI_PointParse_RetFalse)) return(FALSE) ;
			   rpp->Token[rpp->Count].Type = V4LEX_TknType_Point ; rpp->Token[rpp->Count].OpCode = V_Opcode_V4Point ;
			   rpp->Count ++ ; if (rpp->Count >= V4LEX_RPP_TokenMax) { v_Msg(ctx,ctx->ErrorMsgAux,"RevPolTknMax") ; return(FALSE) ; } ;
			   got_value = TRUE ;
			   if (flags & V4LEX_RPPFlag_NoCTX) continue ;
			   v4lex_NextTkn(tcb,tknflags) ;
			   if (flags & V4LEX_RPPFlag_EvalLE)	/* Look for possible "*" */
			    { if (tcb->opcode == V_OpCode_Star)
			       { rpp->Token[rpp->Count-1].Type = V4LEX_TknType_PointWithStar ; continue ; } ;
			    } ;
			   if (tcb->opcode != V_OpCode_LineLine)	/* Look for possible "||" */
			    { v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ; continue ; } ;
			   rpp->Token[rpp->Count-1].Type = V4LEX_TknType_PointWithContext ;	/* Mark prior point! */
			   rpp->Token[rpp->Count].ValPtr = (char *)v4mm_AllocChunk(sizeof(struct V4DPI__Point),FALSE) ;
			   if (v4dpi_PointParse(ctx,(P *)rpp->Token[rpp->Count].ValPtr,tcb,ppflags|V4DPI_PointParse_RetFalse)) return(FALSE) ;
			   rpp->Token[rpp->Count].Type = V4LEX_TknType_Point ; rpp->Token[rpp->Count].OpCode = V_Opcode_V4Point ;
			   rpp->Count ++ ; if (rpp->Count >= V4LEX_RPP_TokenMax) { v_Msg(ctx,ctx->ErrorMsgAux,"RevPolTknMax") ; return(FALSE) ; } ;
			   continue ;
			 } ;
			UCstrcpy(rpp->Token[rpp->Count].AlphaVal,tcb->UCkeyword) ; break ;
		 case V4LEX_TknType_PString:
			ltcb = v4mm_AllocChunk(sizeof *ltcb,FALSE) ;
			v4lex_InitTCB(ltcb,0) ; v4lex_NestInput(ltcb,NULL,tcb->UCstring,V4LEX_InpMode_String) ;
			if (!v4dpi_PointParse(ctx,&isctpt,ltcb,ppflags|V4DPI_PointParse_RetFalse)) return(FALSE) ;
			rpp->Token[rpp->Count].ValPtr = (char *)v4mm_AllocChunk(sizeof(struct V4DPI__Point),FALSE) ;
			ipt = v4dpi_IsctEval((P *)rpp->Token[rpp->Count].ValPtr,&isctpt,ctx,0,NULL,NULL) ;
			if (ipt == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"PLitStrFail",tcb->UCstring) ; return(FALSE) ; } ;
			v4lex_FreeTCB(ltcb) ;
			rpp->Token[rpp->Count].Type = V4LEX_TknType_Point ; rpp->Token[rpp->Count].OpCode = V_Opcode_V4Point ;
			rpp->Count ++ ; if (rpp->Count >= V4LEX_RPP_TokenMax) { v_Msg(ctx,ctx->ErrorMsgAux,"RevPolTknMax") ; return(FALSE) ; } ;
			got_value = TRUE ;
			continue ;
		 case V4LEX_TknType_Punc:
			UCstrcpy(rpp->Token[rpp->Count].AlphaVal,tcb->UCkeyword) ; break ;
		 case V4LEX_TknType_String:
			UCstrcpy(rpp->Token[rpp->Count].AlphaVal,tcb->UCstring) ; break ;
		 case V4LEX_TknType_Integer:
			rpp->Token[rpp->Count].IntVal = tcb->integer ; rpp->Token[rpp->Count].dps = tcb->decimal_places ;
			break ;
		 case V4LEX_TknType_Float:
			rpp->Token[rpp->Count].Floating = tcb->floating ; break ;
		 case V4LEX_TknType_LInteger:
//			rpp->Token[rpp->Count].Floating = (double)tcb->Linteger ;
//			for(i=0;i<tcb->decimal_places;i++) rpp->Token[rpp->Count].Floating /= 10.0 ;
			FIXEDtoDOUBLE(rpp->Token[rpp->Count].Floating,tcb->Linteger,tcb->decimal_places) ;
			tcb->type = V4LEX_TknType_Float ; break ;
		 case V4LEX_TknType_EOL:
			tcb->opcode = V_OpCode_EOX ; tcb->prec = 12 ; goto check_op ;
		 default:
			v_Msg(ctx,ctx->ErrorMsgAux,"RevPolTknType",tcb->type) ; return(FALSE) ; 
	       } ;
 	      rpp->Token[rpp->Count].Type = tcb->type ; rpp->Token[rpp->Count].OpCode = V_Opcode_GenericValue ;
	      rpp->Count ++ ; if (rpp->Count >= V4LEX_RPP_TokenMax) { v_Msg(ctx,ctx->ErrorMsgAux,"RevPolTknMax") ; return(FALSE) ; } ;
	      got_value = TRUE ;
	      continue ;
	 } ;
	if (flags & V4LEX_RPPFlag_V4IMEval ? ptbegin : FALSE)		/* Got begin of point or ISCT ? */
	 { 
//	   if (ctx == NULL) v4_error(V4E_RPPNOCTX,tcb,"VCOM","RPP","RPPNOCTX","Cannot parse V4 Point - no context specified") ;
	   v4lex_NextTkn(tcb,V4LEX_Option_PushCur) ;
	   rpp->Token[rpp->Count].ValPtr = (char *)v4mm_AllocChunk(sizeof(struct V4DPI__Point),FALSE) ;
	   if (!v4dpi_PointParse(ctx,(P *)rpp->Token[rpp->Count].ValPtr,tcb,ppflags|V4DPI_PointParse_RetFalse))
	    return(FALSE) ;
	   rpp->Token[rpp->Count].Type = V4LEX_TknType_Isct ; rpp->Token[rpp->Count].OpCode = V_Opcode_V4Point ;
	   rpp->Count ++ ; if (rpp->Count >= V4LEX_RPP_TokenMax) { v_Msg(ctx,ctx->ErrorMsgAux,"RevPolTknMax") ; return(FALSE) ; } ;
	   got_value = TRUE ;
	   continue ;
	 } ;
/*	   Here to check out operators */
check_op:
	   switch (tcb->opcode)
	    { default:
		break ;
	      case V_OpCode_Colon:
		if (flags & V4LEX_RPPFlag_ColonKW) tknflags |= V4LEX_Option_ForceKW ;
		break ;
	      case V_OpCode_Minus:	if (!got_value) { tcb->opcode = V_OpCode_UMinus ; tcb->prec = 29 ; } ; break ;
	      case V_OpCode_NCLBrace:	tcb->opcode = V_OpCode_LBrace ; goto co1 ;
	      case V_OpCode_NCLParen:	tcb->opcode = V_OpCode_LParen ; goto co1 ;
	      case V_OpCode_LParen:
	      case V_OpCode_LBracket:
	      case V_OpCode_NCLBracket:
	      case V_OpCode_LangleLangle:
	      case V_OpCode_LBrace:
co1:		op.Token[op.Count].Type = tcb->type ; UCstrcpy(op.Token[op.Count].AlphaVal,tcb->UCkeyword) ;
		op.Token[op.Count].Prec = tcb->prec ; op.Token[op.Count].OpCode = tcb->opcode ; op.Count ++ ;
		if (flags & V4LEX_RPPFlag_PushParens)
		 { rpp->Token[rpp->Count] = op.Token[op.Count-1] ;
		   rpp->Count ++ ; if (rpp->Count >= V4LEX_RPP_TokenMax) { v_Msg(ctx,ctx->ErrorMsgAux,"RevPolTknMax") ; return(FALSE) ; } ;
		 } ;
		got_value = FALSE ;
		goto next_token ;					/* Push these immediately */
	      case V_OpCode_Semi:
		tcb->opcode = V_OpCode_EOX ; break ;
	    } ;
	   got_value = (tcb->opcode == V_OpCode_RParen) ;
	   for(;;)
	    { if (tcb->prec > op.Token[op.Count-1].Prec)
	       { 
	         switch (tcb->opcode)
		  { case V_OpCode_RParen:
			if (op.Token[op.Count-1].OpCode != V_OpCode_LParen)
			 { if (flags & V4LEX_RPPFlag_RParenBrkt) { tcb->opcode = V_OpCode_EOX ; break ; } ;
			   v_Msg(ctx,ctx->ErrorMsgAux,"RevPolParens") ; return(FALSE) ;
			 } ;
			if (flags & V4LEX_RPPFlag_PushParens)
			 { rpp->Token[rpp->Count].Prec = tcb->prec ; rpp->Token[rpp->Count].OpCode = tcb->opcode ;
			   rpp->Token[rpp->Count].Type = tcb->type ;
			   UCstrcpy(rpp->Token[rpp->Count].AlphaVal,tcb->UCkeyword) ;
			   rpp->Count ++ ; if (rpp->Count >= V4LEX_RPP_TokenMax) { v_Msg(ctx,ctx->ErrorMsgAux,"RevPolTknMax") ; return(FALSE) ; } ;
			 } ;
			op.Count -- ; goto next_token ;
		    case V_OpCode_RBracket:
			if (flags & V4LEX_RPPFlag_RParenBrkt)
			 { tcb->opcode = V_OpCode_EOX ; break ; } ;
		    case V_OpCode_RBrace:
			break ;
		  } ;
		 if (tcb->opcode == V_OpCode_EOX && op.Token[op.Count-1].OpCode != V_OpCode_BOX)
		  { v_Msg(ctx,ctx->ErrorMsgAux,"RevPolSyntax") ; return(FALSE) ; } ;
		 op.Token[op.Count].Prec = tcb->prec ; op.Token[op.Count].OpCode = tcb->opcode ;
		 op.Token[op.Count].Type = tcb->type ;
		 UCstrcpy(op.Token[op.Count].AlphaVal,tcb->UCkeyword) ;
		 op.Count ++ ;
		 goto next_token ;
	       } else
	       { op.Count -- ;
		 rpp->Token[rpp->Count] = op.Token[op.Count] ;
		 rpp->Count ++ ; if (rpp->Count >= V4LEX_RPP_TokenMax) { v_Msg(ctx,ctx->ErrorMsgAux,"RevPolTknMax") ; return(FALSE) ; } ;
	       } ;
	    } ;
next_token:
	   continue ;
	 } ;
	return(TRUE) ;
}

/*	R E V E R S E   P O L I S H   O P T I M I Z A T I O N   M O D U L E S	*/

#ifdef WANTREVPOLOPT

struct local__tree {				/* Tree for holding parse structure */
  short tx ;
  struct {
    short parent ;				/* Index to parent node, 0 if top level */
    short val1,val2 ;				/* Children, -n if node in tree, 0/+n if token[] index */
    short arguments,opcode,opx ;		/* Opcode for node, opx is index to token[] */
    short ismap,keynum ;			/* ismap TRUE if bitmap/key possibility, keynum = key number */
  } node[100] ;
 } ;

/*	v4lex_OptRevPosSimplify - Simplifies parse try by nesting as much as possible down left side (val1) of tree */
/*	Call: v4lex_OptRevPolSimplify( lt , nodex )
	  where lt is pointer to tree,
		nodex is starting position in tree			*/

void v4lex_OptRevPolSimplify(lt,nodex)
  struct local__tree *lt ;
  int nodex ;
{ int rc,rcln ;

	for(;lt->node[nodex].val2<0;)
	 { rc = -lt->node[nodex].val2 ;			/* Index to right child */
	   if(lt->node[nodex].opcode != lt->node[rc].opcode)	/* XXXXX check for non-commutative !!! */
	    break ;
	   rcln = lt->node[rc].val1 ;			/* Save right child, left node */
	   lt->node[rc].val1 = lt->node[nodex].val1 ;
	   if(lt->node[rc].val1 < 0) lt->node[-lt->node[rc].val1].parent = rc ;
	   lt->node[nodex].val1 = -rc ;			/* Link this to right child */
	   lt->node[nodex].val2 = lt->node[rc].val2 ;	/* Move child's right to current node */
	   lt->node[rc].val2 = rcln ;			/* Move child's left to right */
	 } ;
	if(lt->node[nodex].val2 < 0) v4lex_OptRevPolSimplify(lt,-lt->node[nodex].val2) ;
	if(lt->node[nodex].val1 < 0) v4lex_OptRevPolSimplify(lt,-lt->node[nodex].val1) ;
}

/*	v4lex_OptRevPolDropLeft - Attempts to drop all opcode nodes to bottom left of parse tree (attempt to aggregate map) */
/*	Call: v4lex_OptRevPolDropLeft( lt , nodex , opcode )
	  where lt is pointer to tree,
		nodex is starting position,
		opcode is opcode to attempt to drop & aggregate		*/

void v4lex_OptRevPolDropLeft(lt,nodex,opcode)
 struct local__tree *lt ;
 int nodex,opcode ;
{ int rc,rc1,nx,p ;

	for(;nodex <= 0 ? FALSE : lt->node[nodex].opcode == opcode && lt->node[nodex].val2 < 0;nodex=(-lt->node[nodex].val1))
	 { rc = -lt->node[nodex].val2 ;
	   if(!lt->node[rc].ismap) continue ;
	   for(nx=nodex;lt->node[nx].val1<0;)
	    { nx = -lt->node[nx].val1 ;
	      if(lt->node[nx].opcode != opcode)			/* Left side not correct opcode, best we can do is swap */
	       { if(lt->node[nx].ismap) break ;			/* Don't bother, nothing to gain */
		 p = lt->node[nx].parent ;			/* p is parent of nx */
		 lt->node[nodex].val2 = lt->node[p].val1 ; lt->node[p].val1 = -rc ;
		 break ;
	       } ;
	      rc1 = lt->node[nx].val2 ;
	      if(rc1 >= 0) continue ; rc1 = -rc1 ;
	      if(lt->node[rc1].ismap) continue ;
	      lt->node[rc1].parent = nodex ;
	      lt->node[rc].parent = nx ;
	      lt->node[nodex].val2 = -rc1 ;
	      lt->node[nx].val2 = -rc ;
	      break ;
	    } ;
	 } ;
}

/*	v4lex_OptRevPolMarkMap - Marks all nodes as "map" (node map iff both children are "map") */

LOGICAL v4lex_OptRevPolMarkMap(lt,nodex)
  struct local__tree *lt ;
  int nodex ;
{ int lhs,rhs ;

	if(lt->node[nodex].val1 >= 0 || lt->node[nodex].val2 >= 0) return(lt->node[nodex].ismap) ;
	lhs = v4lex_OptRevPolMarkMap(lt,-lt->node[nodex].val1) ;
	rhs = v4lex_OptRevPolMarkMap(lt,-lt->node[nodex].val2) ;
	lt->node[nodex].ismap = lhs && rhs ;
	return(lt->node[nodex].ismap) ;
}

/*	v4lex_OptRevPolCvtTree - Converts tree back to reverse polish notation */
/*	Call: v4lex_OptRevPolCvtTree( lt , drp , srp , nodex )
	  where lt is pointer to tree,
		drp is destination reverse polish structure,
		srp is source structure,
		nodex is starting node/position in tree			*/

void v4lex_OptRevPolCvtTree(lt,drp,srp,nodex)
  struct local__tree *lt ;
  struct V4LEX__RevPolParse *drp,*srp ;
  int nodex ;
{
	if(lt->node[nodex].val1 >= 0)
	 { drp->Token[drp->Count++] = srp->Token[lt->node[nodex].val1] ; }
	 else { v4lex_OptRevPolCvtTree(lt,drp,srp,-lt->node[nodex].val1) ; } ;
	if(lt->node[nodex].val2 >= 0)
	 { drp->Token[drp->Count++] = srp->Token[lt->node[nodex].val2] ; }
	 else { v4lex_OptRevPolCvtTree(lt,drp,srp,-lt->node[nodex].val2) ; } ;
	drp->Token[drp->Count++] = srp->Token[lt->node[nodex].opx] ;
}

/*	v4lex_Show - Debugging module to dump out tree from a starting point */

void v4lex_Show(lt,srp,nodex)
  struct local__tree *lt ;
  struct V4LEX__RevPolParse *srp ;
  int nodex ;
{ char tb1[100],tb2[100] ;

	if(lt->node[nodex].val1 < 0) { sprintf(tb1,"%d",-lt->node[nodex].val1) ; }
	 else { sprintf(tb1,"%s",UCretASC(srp->Token[lt->node[nodex].val1].AlphaVal)) ; } ;
	if(lt->node[nodex].val2 < 0) { sprintf(tb2,"%d",-lt->node[nodex].val2) ; }
	 else { sprintf(tb2,"%s",UCretASC(srp->Token[lt->node[nodex].val2].AlphaVal)) ; } ;
	printf("Node %d Op %d IsMap %d Val1=%s Val2=%s\n",nodex,lt->node[nodex].opcode,lt->node[nodex].ismap,tb1,tb2) ;
	if(lt->node[nodex].val1 < 0) v4lex_Show(lt,srp,-lt->node[nodex].val1) ;
	if(lt->node[nodex].val2 < 0) v4lex_Show(lt,srp,-lt->node[nodex].val2) ;
}

/*	v4lex_OptRevPol - Optimizes reverse polish expression		*/
/*	Call: v4lex_OptRevPol( drp , srp )
	  where drp is pointer to destination structure to be updated,
		srp is pointer to RP structure to be optimized		*/

void v4lex_OptRevPol(drp,srp)
  struct V4LEX__RevPolParse *drp,*srp ;
{ struct local__tree tree ;
  short valstk[100],vsx ;
  int i,p,pp,maxkey ; int debug=FALSE ;

	memset(&tree,0,sizeof tree) ;				/* Zap all in the tree */
	for(i=0,vsx=1,tree.tx=1;i<srp->Count;i++)		/* Loop thru RevPol to build up a binary expression tree */
	 { switch(srp->Token[i].Type)
	    { case V4LEX_TknType_Punc:
		tree.node[tree.tx].arguments = srp->Token[i].Arguments ;
		tree.node[tree.tx].ismap = srp->Token[i].IsMap ; tree.node[tree.tx].keynum = srp->Token[i].KeyNum ;
		switch(srp->Token[i].Arguments)
		 { case 1:
			tree.node[tree.tx].val1 = valstk[--vsx] ; tree.node[tree.tx].val2 = tree.node[tree.tx].val1 ;
			tree.node[tree.tx].opcode = srp->Token[i].OpCode ; tree.node[tree.tx].opx = i ;
			valstk[vsx++] = -tree.tx ;		/* Push - index of tree onto value stack */
/*			May link up this parent to sub-nodes */
			if(tree.node[tree.tx].val1 < 0) tree.node[-tree.node[tree.tx].val1].parent = tree.tx ;
			if(tree.node[tree.tx].val2 < 0) tree.node[-tree.node[tree.tx].val2].parent = tree.tx ;
			break ;
		   case 2:
			tree.node[tree.tx].val1 = valstk[--vsx] ; tree.node[tree.tx].val2 = valstk[--vsx] ;
			tree.node[tree.tx].opcode = srp->Token[i].OpCode ; tree.node[tree.tx].opx = i ;
			valstk[vsx++] = -tree.tx ;		/* Push - index of tree onto value stack */
/*			May link up this parent to sub-nodes */
			if(tree.node[tree.tx].val1 < 0) tree.node[-tree.node[tree.tx].val1].parent = tree.tx ;
			if(tree.node[tree.tx].val2 < 0) tree.node[-tree.node[tree.tx].val2].parent = tree.tx ;
			break ;
		 } ;
		tree.tx ++ ; break ;
	      case V4LEX_TknType_String:
	      case V4LEX_TknType_Keyword:
	      case V4LEX_TknType_Integer:
	      case V4LEX_TknType_Float:
		valstk[vsx++] = i ;		/* Just push index to value onto stack */
		break ;
	    } ;
	 } ;
/*	Finished building expression tree, top of valstk should be index to it */
	if (valstk[vsx-1] != -(tree.tx-1)) v4_error(V4E_INVSYNEXP,0,"VCOM","OptRevPol","INVSYNEXP","Invalid expression syntax") ;
/*	See if we can optimize on it */
	drp->MapCount = (drp->Count = 0) ;
/*	Do a check for keyed access- only permit optimization on 1 key or bitmaps, not both */
	for(i=1,maxkey=0;i<tree.tx;i++)
	 { if(!tree.node[i].ismap) continue ;			/* Not a key */
	   if(tree.node[i].keynum > maxkey) maxkey = tree.node[i].keynum ;
	 } ;
	if(maxkey > 0)						/* Got a key- then trash all other possible "maps" */
	 { for(i=1;i<tree.tx;i++)
	    { if(!tree.node[i].ismap) continue ;
	      if(tree.node[i].val1 < 0 || tree.node[i].val2 < 0) continue ;
	      if(tree.node[i].keynum < maxkey) tree.node[i].ismap = FALSE ;
	    } ;
	 } ;
	if(debug) { vout_Text(VOUT_Trace,0,"After RevPol\n") ; v4lex_Show(&tree,srp,tree.tx-1) ; } ;
	v4lex_OptRevPolSimplify(&tree,tree.tx-1) ;
	if(debug) { vout_Text(VOUT_Trace,0,"After Simplify\n") ; v4lex_Show(&tree,srp,tree.tx-1) ; } ;
	v4lex_OptRevPolMarkMap(&tree,tree.tx-1) ;
	v4lex_OptRevPolDropLeft(&tree,tree.tx-1,V_OpCode_Ampersand) ;
	if(debug) { vout_Text(VOUT_Trace,0,"After DropLeft\n") ; v4lex_Show(&tree,srp,tree.tx-1) ; } ;
	v4lex_OptRevPolMarkMap(&tree,tree.tx-1) ;
	for(i=(-(tree.tx-1));i<0;i=tree.node[i].val1)		/* Start scanning down LHS looking for ISMAP */
	 { i = -i ;						/* Convert to positive node index */
	   if(!tree.node[i].ismap)
	    { if(tree.node[i].opcode == V_OpCode_Ampersand) continue ; /* Drop down if a boolean AND */
	      break ;						/* Break out if not */
	    } ;
	   v4lex_OptRevPolCvtTree(&tree,drp,srp,i) ;		/* First generate bitmap/key portion */
	   drp->MapCount = drp->Count ;
	   if(tree.node[i].parent == 0) return ;		/* If at top of tree then whole expression is map! */
	   p = tree.node[i].parent ;
	   if(tree.node[p].parent == 0)				/* If parent is top of tree then just do RHS */
	    { v4lex_OptRevPolCvtTree(&tree,drp,srp,-tree.node[p].val2) ; return ; } ;
	   pp = tree.node[p].parent ;				/* Get parent's parent */
	   tree.node[pp].val1 = tree.node[p].val2 ;		/* Link parent's RHS to it's parent- chopping off MAP */
	   v4lex_OptRevPolCvtTree(&tree,drp,srp,pp) ;		/* Convert from this point */
	   return ;
	 } ;
	v4lex_OptRevPolCvtTree(&tree,drp,srp,tree.tx-1) ;
	return ;
}
#endif /* WANTREVPOLOPT */

/*	v_ipLookup - Does ip address lookup & socket initialization for various V4 modules				*/
/*	Call: ok = v_ipLookup( ctx , intmodx , urlAddress , portOrSvc, sockType , action , resInt , resUCCHAR , respnt )
	  where ok is TRUE if success, FALSE if failed (error in ctx->ErrorMsg),
		ctx is context,
		intmodx is module,
		urlAddress is address in variety of formats, empty string to reference current machine,
		portOrSvc is port number or service name, NULL for default,
		socketType is SOCK_STREAM, SOCK_DGRAM, ...
		action is action to perform - V_IPLookup_xxx,
		resInt is updated with integer result,
		resUCCHAR is updated with UCCHAR result,
		respnt if not NULL is updated with list of IP addresses with V_IPLookup_IPV4/V6 calls			*/

LOGICAL v_ipLookup(ctx,intmodx,urlAddress,portOrSvc,sockType,action,resInt,resUCCHAR,respnt)
  struct V4C__Context *ctx ;
  INTMODX intmodx ;
  UCCHAR *urlAddress ;
  UCCHAR *portOrSvc ;
  int sockType ;
  ETYPE action ;
  SOCKETNUM *resInt ;
  UCCHAR *resUCCHAR ;
  P *respnt ;
{
  struct addrinfo hints, *addrRes ;
  LOGICAL ok ; UCCHAR ucHost[NI_MAXHOST] ; SOCKETNUM socketNum ;

	addrRes = NULL ;
	SOCKETINIT
/*	If no URL then get our own address */
	if (UCempty(urlAddress))
	 { char szHost[NI_MAXHOST] ; gethostname(szHost,sizeof szHost) ; UCstrcpyAtoU(ucHost,szHost) ; urlAddress = ucHost ; } ;
	memset(&hints,0,sizeof hints) ;
	 hints.ai_family = AF_UNSPEC ; hints.ai_socktype = sockType ; hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
	UCgetaddrinfo(ok,urlAddress,portOrSvc,&hints,&addrRes) ;
	if (ok != 0)
	 { v_Msg(ctx,NULL,"SocketLookupErr",intmodx,"getaddrinfo()",urlAddress,(portOrSvc == NULL ? UClit("") : portOrSvc),NETERROR) ; goto fail ; } ;

	switch(action)
	 {
	   case V_IPLookup_IPV4:
	      { struct addrinfo *res ; UCCHAR ipBuf[128] ; struct V4L__ListPoint *lp ;
	        ZUS(resUCCHAR) ; if (respnt != NULL) { INITLP(respnt,lp,Dim_List) ; } ;
		for(res=addrRes;res!=NULL;res=res->ai_next)
		 { if (res->ai_family != AF_INET) continue ;
		   UCgetnameinfo(ok,res->ai_addr, res->ai_addrlen,ipBuf,NI_MAXHOST,NULL,0,NI_NUMERICHOST) ; 
		   if (ok != 0) { v_Msg(ctx,NULL,"SocketLookupErr",intmodx,"getnameinfo()",urlAddress,(portOrSvc == NULL ? UClit("") : portOrSvc),NETERROR) ; goto fail ; } ;
		   if (respnt != NULL)
		    { P ipnt ; uccharPNTv(&ipnt,ipBuf) ; if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&ipnt,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto fail ; } ; }
		    else { UCstrcat(resUCCHAR,ipBuf) ; break ; } ;		/* Just take the first one if not returning a list */
//		   goto success ;
		 } ;
		if (respnt != NULL) { ENDLP(respnt,lp) ; } ;
		goto success ;
	      }
	   case V_IPLookup_IPV6:
	      { struct addrinfo *res ; UCCHAR ipBuf[128] ; struct V4L__ListPoint *lp ;
	        ZUS(resUCCHAR) ; if (respnt != NULL) { INITLP(respnt,lp,Dim_List) ; } ;
		for(res=addrRes;res!=NULL;res=res->ai_next)
		 { switch (res->ai_family)
		    { default:		continue ;
		      case AF_INET:	continue ;
		      case AF_INET6:
			UCgetnameinfo(ok,res->ai_addr, res->ai_addrlen,ipBuf,NI_MAXHOST,NULL,0,NI_NUMERICHOST) ; 
			if (ok != 0) { v_Msg(ctx,NULL,"SocketLookupErr",intmodx,"getnameinfo()",urlAddress,(portOrSvc == NULL ? UClit("") : portOrSvc),NETERROR) ; goto fail ; } ;
			if (respnt != NULL)
			 { P ipnt ; uccharPNTv(&ipnt,ipBuf) ; if (!v4l_ListPoint_Modify(ctx,lp,V4L_ListAction_Append,&ipnt,0)) { v_Msg(ctx,NULL,"LPModFail",intmodx) ; goto fail ; } ; }
			 else { UCstrcat(resUCCHAR,ipBuf) ; break ; } ;		/* Just take the first one if not returning a list */
		    } ;
		 } ; goto success ;
	      }
	   case V_IPLookup_Connect:
		if ((socketNum = socket(addrRes->ai_family,addrRes->ai_socktype,addrRes->ai_protocol)) < 0)
		 { v_Msg(ctx,NULL,"SocketLookupErr",intmodx,"socket()",urlAddress,(portOrSvc == NULL ? UClit("") : portOrSvc),NETERROR) ; goto fail ; } ;
		if (connect(socketNum,addrRes->ai_addr,addrRes->ai_addrlen) < 0)
		 { if (!(NETERROR == EWOULDBLOCK || NETERROR == EINPROGRESS))
		    { v_Msg(ctx,NULL,"SocketLookupErr",intmodx,"connect()",urlAddress,(portOrSvc == NULL ? UClit("") : portOrSvc),NETERROR) ; goto fail ; } ;
		 } ;
		*resInt = socketNum ;
		goto success ;
	   case V_IPLookup_Listen:
		if ((socketNum = socket(addrRes->ai_family,addrRes->ai_socktype,addrRes->ai_protocol)) < 0)
		 { v_Msg(ctx,NULL,"SocketLookupErr",intmodx,"socket()",urlAddress,(portOrSvc == NULL ? UClit("") : portOrSvc),NETERROR) ; goto fail ; } ;
		if (listen(socketNum,10) < 0)
		 { v_Msg(ctx,NULL,"SocketLookupErr",intmodx,"listen()",urlAddress,(portOrSvc == NULL ? UClit("") : portOrSvc),NETERROR) ; goto fail ; } ;
		*resInt = socketNum ;
		goto success ;
	 } ;

success:
	if (addrRes != NULL) freeaddrinfo(addrRes);
	return(TRUE) ;

fail:
	if (addrRes != NULL) freeaddrinfo(addrRes);
	return(FALSE) ;

}

/*	P R I M E   N U M B E R   C A L C U L A T I O N		*/
/*	Call: prime = v_CvtToPrime( num )
	  where prime is prime number >= num,
		num is starting number for prime		*/

int v_CvtToPrime(num)
  int num ;
{ int i,sroot ;

	num = num | 1 ;				/* Make sure number is odd */
	sroot = (int)sqrt((double)num) + 1 ;
	for(;;)
	 { for(i=2;i<=sroot;i++)
	    { if ((num % i) == 0) break ; } ;
	   if (i > sroot) return(num) ;		/* Got a prime */
	   num += 2 ;				/* Not prime - advance to next odd number */
	   if (sroot * sroot < num) sroot++ ;
	 } ;
}

/*	S P A W N   A   S U B  -  P R O C E S S			*/

/*	v_AvailableProcessors - Returns the number of CPUs/processors available */
/*	Call: num = v_AvailableProcessors()					*/

int v_AvailableProcessors()
{ UCCHAR ubuf[128],*ubp ; int num ;
#ifdef WINNT
	if (!v_UCGetEnvValue(UClit("number_of_processors"),ubuf,UCsizeof(ubuf))) return(1) ;
	num = UCstrtol(ubuf,&ubp,10) ; return(*ubp == UCEOS ? num : 1) ;
#elif defined UNIX
	num = sysconf(_SC_NPROCESSORS_ONLN) ; return(num) ;	
#elif defined VMS
		return(-1) ;		
#else
		return(-1) ;		
#endif
}


/*	v_SpawnSort - Spawns a subprocess to xvsort							*/
/*	Call: res = v_SpawnSort( argbuf , waitflag , errbuf )
	  where res is TRUE/FALSE if OK, not OK,
		argbuf is ptr to string argument (minus the leading xvsort),
		waitflag is determines how long to wait for sort to finish (see V_SPAWNPROC_xxx),
		errbuf buffer for error message if problem						*/

#ifdef WINNT
v_SpawnSort(argbuf,waitflag,errbuf)
  UCCHAR *argbuf ;
  int waitflag ;
  UCCHAR *errbuf ;
{ DWORD res ; PROCESS_INFORMATION pi ;	/* Get process info from CreateProcess */
  UCCHAR cmdstr[512] ;
#ifdef V4UNICODE
  STARTUPINFOW si ;		/* Process startup info */
#else
  STARTUPINFO si ;		/* Process startup info */
#endif

	UCstrcpy(cmdstr,v_GetV4HomePath(UClit("xvsort.exe"))) ; UCstrcat(cmdstr,UClit(" ")) ; UCstrcat(cmdstr,argbuf) ;
	memset(&si,0,sizeof si) ; si.cb = sizeof si ;
	UCGetStartupInfo(&si) ;
	si.lpTitle = UClit("V4 Spawn") ;
	si.dwFlags |= STARTF_USESTDHANDLES ;		/* Want subprocess to use handles of this process */
	{ HANDLE hDupStdOutput ;
	  if (!DuplicateHandle(GetCurrentProcess(),GetStdHandle(STD_OUTPUT_HANDLE),GetCurrentProcess(),&hDupStdOutput,0,TRUE,DUPLICATE_SAME_ACCESS))
	   { printf("???DuplicateHandle() failed\n") ;
	   } ;
	  si.hStdOutput = hDupStdOutput ;
	} ;
	if (waitflag & V_SPAWNPROC_NewWindow) si.lpDesktop = UClit("") ;
	if (!UCCreateProcess(NULL,cmdstr,NULL,NULL,TRUE,CREATE_UNICODE_ENVIRONMENT,NULL,NULL,&si,&pi))
	 { if (errbuf != NULL) v_Msg(NULL,errbuf,"SpawnFail","CreateProcess",argbuf,GetLastError()) ;
	   return(FALSE) ;
	 } ;
	res = WaitForSingleObject(pi.hProcess,(waitflag == V_SPAWNWAIT_Infinite ? INFINITE : (1000 * SPAWNMAXWAITDEC(waitflag)))) ;
	if (res != WAIT_OBJECT_0)		/* If process did not terminate normally then force a termination */
	 { TerminateProcess(pi.hProcess,EXITABORT) ;
	 } ;
	CloseHandle(pi.hProcess) ; CloseHandle(pi.hThread) ;
	switch (res)
	 { default:		if (errbuf != NULL) v_Msg(NULL,errbuf,"SpawnFail","WaitForSingleObject",(argbuf == NULL ? UClit("cmd") : argbuf),GetLastError()) ;
	   case WAIT_TIMEOUT:	v_Msg(NULL,errbuf,"SpawnTO",(argbuf == NULL ? UClit("cmd") : argbuf),SPAWNMAXWAITDEC(waitflag)) ; return(FALSE) ;
	   case WAIT_OBJECT_0:	break ;
	 } ;
	return(TRUE) ;
}
#else
LOGICAL v_SpawnSort(argbuf,waitflag,errbuf)
  UCCHAR *argbuf ;
  int waitflag ;
  UCCHAR *errbuf ;
{ UCCHAR cmdstr[512] ;

#if defined LINUX486 || defined ALPHAOSF || defined RASPPI
	UCstrcpy(cmdstr,v_GetV4HomePath(UClit("xvsort"))) ;
#else
	v_Msg(ctx,errbuf,"NYIonthisOS",0) ; return(FALSE) ;
#endif
	UCstrcat(cmdstr,UClit(" ")) ; UCstrcat(cmdstr,argbuf) ;
//	return(v_SpawnProcess(NULL,cmdstr,waitflag,errbuf,NULL,NULL,UNUSED)) ;
	{ struct V__SpawnArgs sa ; memset(&sa,0,sizeof sa) ; sa.argBuf = cmdstr ; sa.errBuf = errbuf ; sa.fileId = UNUSED ;
	  if (waitflag == V_SPAWNWAIT_Infinite) { sa.waitFlags = V_SPAWNPROC_Wait ; }
	   else { sa.waitFlags = V_SPAWNPROC_Wait ; sa.waitSeconds = SPAWNMAXWAITDEC(waitflag) ; } ;
	  return(v_SpawnProcess(&sa)) ;
	}
}
#endif


#ifdef WINNT
struct lcl__SpawnProcThreadArg {
  int socknum ;
  DWORD idProcess ;
  HANDLE hProcess ;
} ;
#include <TLHELP32.H>
void v_SpawnSocketMonitor(spta)
  struct lcl__SpawnProcThreadArg *spta ;
{ fd_set sset ;
//  HANDLE hJob ; int byte ;
  int res ;

printf("**** in SpawnSocketMonitor\n") ;
	FD_ZERO(&sset) ; FD_SET(spta->socknum,&sset) ;
	if (select(FD_SETSIZE,&sset,NULL,NULL,NULL) == -1)	/* Wait for an event on this socket (most likely will not get one) */
	 { printf(" ?? Error (%d) in select() within v_SpawnSocketMonitor thread\n",errno) ;
	   return ;
	 } ;
/*	If we wake up then got a socket event, since we should not get a read event, we must have a connection failure */
#ifdef NOTAVAILABLEYET
	hJob = CreateJobObject(NULL,"temporary_job") ;
printf("  result of CreateJob = %p\n",hJob) ;
	res = AssignProcessToJobObject(hJob,spta->hProcess) ;
printf("  result of AssignProcess = %d\n",res) ;
	res = TerminateJobObject(hJob,0) ;
printf("  result of TerminateJob = %d\n",res) ;
	CloseHandle(hJob) ;
#endif
//#ifdef NOTAVAILABLEYET
	{ PROCESSENTRY32 lppr ; HANDLE hSnap ;
	  hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0) ;
	  Process32First(hSnap,&lppr) ;
printf("  ***** woke up to kill subs - %d\n",spta->idProcess) ;
	  for(;Process32Next(hSnap,&lppr);)
	   { 
	     if (lppr.th32ParentProcessID == spta->idProcess)
	      { HANDLE hSubProc ;
	        printf("Killing subprocess of target: %d %d %s\n",lppr.th32ProcessID,lppr.th32ParentProcessID,lppr.szExeFile) ;
	        if ((hSubProc = OpenProcess(PROCESS_ALL_ACCESS,FALSE,lppr.th32ParentProcessID)) == NULL)
	         { printf("?? Could not get handle for subprocess (%d)\n",GetLastError()) ; continue ; } ;
	        res = TerminateProcess(hSubProc,FALSE) ;
	        printf("  terminated %s, result=%d\n",lppr.szExeFile,res) ;
	        CloseHandle(hSubProc) ; continue ;
	      } ;
	     if (lppr.th32ProcessID == spta->idProcess) 
	      { 
	        printf(" will kill target subprocess: %d %d %s\n",lppr.th32ProcessID,lppr.th32ParentProcessID,lppr.szExeFile) ;
	        continue ;
	      } ;
	   } ;
	  res = TerminateProcess(spta->hProcess,FALSE) ;
	}
//#endif
	return ;
}
#endif

#ifdef WINNT
#define NEWSPAWN			/* Only on Windows */
#endif
#ifdef NEWSPAWN
struct lcl__SpawnInfo {
  LENMAX waitMS ;
  LENMAX bytesOut ;
  HANDLE idProcess, idThread ;
  LOGICAL didTimeout ;
  LOGICAL noWait ;			/* TRUE if v_spawnDoIt() called via thread (no waiting by parent process) */
  UCCHAR *exefile, *argbuf, *errbuf ;
//  INTMODX intmodx ;
  FILEID fileId ;
} ;
void v_spawnTimeOutWait(toi)		/* A thread to wait certain amount of time & then terminate process */
  struct lcl__SpawnInfo *toi ;		/* The calling process will terminate this thread if subprocess terminates within time limit */
{
	Sleep(toi->waitMS) ;
	TerminateProcess(toi->idProcess,EXITABORT) ;
	toi->didTimeout = TRUE ;
}

/*	v_spawnDoIt - Spawns subprocess, captures stdout/stderr output via pipe and appends to V4 'Progress' stream */
LOGICAL v_spawnDoIt(toi)
  struct lcl__SpawnInfo *toi ;		/* The calling process will terminate this thread if subprocess terminates within time limit */
{
  HANDLE hOutputReadTmp,hOutputRead,hOutputWrite,hErrorWrite;
  SECURITY_ATTRIBUTES sa ; PROCESS_INFORMATION pi ;
#ifdef V4UNICODE
  STARTUPINFOW si ;		/* Process startup info */
#else
  STARTUPINFO si ;		/* Process startup info */
#endif
  HANDLE idThread ;
  INDEX fx ; LOGICAL retVal ; LENMAX bytesOut ;
  
	ZUS(toi->errbuf) ;
//	intmodx = toi->intmodx ;	/* Needed for CREATE_THREAD macro to work */
	retVal = FALSE ;		/* Take pessimistic view - assume failure */
	sa.nLength= sizeof(SECURITY_ATTRIBUTES) ; sa.lpSecurityDescriptor = NULL ; sa.bInheritHandle = TRUE ;
	if (!CreatePipe(&hOutputReadTmp,&hOutputWrite,&sa,0))		/* Pipe to connect to subprocess */
         { v_Msg(NULL,toi->errbuf,"SpawnFail","CreatePipe",(toi->argbuf == NULL ? UClit("cmd") : toi->argbuf),GetLastError()) ; goto done ; } ;
/*	Create a duplicate of the output write handle for the std error write handle. This is necessary in case the child application closes one of its std output handles */
	if (!DuplicateHandle(GetCurrentProcess(),hOutputWrite,GetCurrentProcess(),&hErrorWrite,0,TRUE,DUPLICATE_SAME_ACCESS))
         { v_Msg(NULL,toi->errbuf,"SpawnFail","DuplicateHandle",(toi->argbuf == NULL ? UClit("cmd") : toi->argbuf),GetLastError()) ; goto done ; } ;
/*	Same thing for output handle */
	if (!DuplicateHandle(GetCurrentProcess(),hOutputReadTmp,GetCurrentProcess(),&hOutputRead,0,FALSE, DUPLICATE_SAME_ACCESS))
         { v_Msg(NULL,toi->errbuf,"SpawnFail","DuplicateHandle(2)",(toi->argbuf == NULL ? UClit("cmd") : toi->argbuf),GetLastError()) ; goto done ; } ;
/*	Close inheritable copies of the handles you do not want to be inherited */
	CloseHandle(hOutputReadTmp) ;
	memset(&pi,0,sizeof(PROCESS_INFORMATION)) ;
	memset(&si,0,sizeof(STARTUPINFO)) ; si.cb = sizeof(STARTUPINFO) ; si.dwFlags = STARTF_USESTDHANDLES ;
	si.hStdOutput = hOutputWrite ; si.hStdError = hErrorWrite ;	/* Reassign stdout & stderr */
	if (!UCCreateProcess(toi->exefile,(toi->argbuf == NULL ? UClit("cmd") : toi->argbuf),NULL,NULL,TRUE,CREATE_UNICODE_ENVIRONMENT,NULL,NULL,&si,&pi))
         { printf("  ?FAIL UCCreateProcess ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n") ; v_Msg(NULL,toi->errbuf,"SpawnFail","UCCreateProcess",(toi->argbuf == NULL ? UClit("cmd") : toi->argbuf),GetLastError()) ; goto done ; } ;
	toi->idProcess = pi.hProcess ; toi->idThread = pi.hThread ;
	if (toi->waitMS != V_SPAWNWAIT_Infinite)			/* If max time then spawn thread to wait and kill process if it has not already terminated */
	 { toi->didTimeout = FALSE ;
	   CREATE_THREAD(v_spawnTimeOutWait,idThread,toi,NULL,toi->errbuf)
	 } ;
	CloseHandle(hOutputWrite) ; CloseHandle(hErrorWrite) ;
/*	Determine output for this subprocess. If no fileId then use VOUT_Progress */
	fx = (toi->fileId == UNUSED ? vout_StreamToFileX(VOUT_Progress) : vout_FileIdToFileX(toi->fileId)) ;
	for(bytesOut=0;;)					/* suck all subprocess output from the pipe and send it to fx stream */
	 { CHAR lpBuffer[4192]; DWORD nBytesRead;
	   if (!ReadFile(hOutputRead,lpBuffer,sizeof(lpBuffer),&nBytesRead,NULL) || !nBytesRead)
	    { if (GetLastError() == ERROR_BROKEN_PIPE) { break ; } ;
	      v_Msg(NULL,toi->errbuf,"SpawnFail","ReadFile",(toi->argbuf == NULL ? UClit("cmd") : toi->argbuf),GetLastError()) ; goto done ;
	    } ;
	   bytesOut += nBytesRead ;
	   if (bytesOut > toi->bytesOut)
	    break ;
	   vout_TextFileX(fx,nBytesRead,lpBuffer) ;
//	   vout_Text(VOUT_Progress,nBytesRead,lpBuffer) ;
         } ;
	CloseHandle(hOutputRead) ; CloseHandle(pi.hProcess) ; CloseHandle(pi.hThread) ; 
	if (toi->waitMS != V_SPAWNWAIT_Infinite)
	 { CLOSE_THREAD(idThread) ; } ;
	if (toi->fileId != UNUSED)
	 vout_CloseFile(toi->fileId,UNUSED,toi->errbuf) ;
	if (toi->didTimeout)
	 { v_Msg(NULL,toi->errbuf,"SpawnTO",(toi->argbuf == NULL ? UClit("cmd") : toi->argbuf),(toi->waitMS/1000)) ; goto done ; } ;

	if (bytesOut <= toi->bytesOut) { retVal = TRUE ; }
	 else { v_Msg(NULL,toi->errbuf,"SpawnMaxOut",toi->bytesOut) ; retVal = FALSE ; } ;
done:	if (toi->noWait)			/* If called 'no waiting' then deallocate stuff */
	 { if (toi->argbuf != NULL) v4mm_FreeChunk(toi->argbuf) ; if (toi->exefile != NULL) v4mm_FreeChunk(toi->exefile) ; if (toi->errbuf != NULL) v4mm_FreeChunk(toi->errbuf) ;
	   v4mm_FreeChunk(toi) ;
	 } ;
fail:
	return(retVal) ;
}
#endif

/*	v_SpawnProcess - Spawns a subprocess, with/without argument, with/without waiting for return	*/
/*	Call: logical = v_SpawnProcess( sa )
	  where logical = TRUE if OK, false if not,
		sa = spawn argument block (struct V__SpawnArgs)						*/

///*	Call: res = v_SpawnProcess( exefile, argbuf , waitflag , errbuf, respnt , sockptr, fileId)
//	  where res is TRUE/FALSE if OK, not OK,
//		exefile is executable to run, if NULL then exe implied in argbuf,
//		argbuf is ptr to string argument, NULL if none,
//		waitflag low bits blocking value (waitflag & 0xf) and flags (see V_SPAWNPROC_xxx), high bits are max seconds to allow for completion (0=infinite),
//		errbuf is optional buffer for error message if problem, NULL if none,
//		respnt (if not NULL) is updated with resulting point (maybe process/thread info),
//		sockptr, if not NULL, is pointer to socket to monitor and if disconnect then terminate process immediately,
//		fileId, if not UNUSED, is fileId for subprocess output to be streamed to (and closed when subprocess terminates)	*/
//

//LOGICAL v_SpawnProcess(exefile, argbuf,waitflag,errbuf,respnt,sockptr,fileId)
//  UCCHAR *exefile ;
//  UCCHAR *argbuf ;
//  int waitflag ;
//  UCCHAR *errbuf ;
//  P *respnt ;
//  int *sockptr ;
//  FILEID fileId ;
LOGICAL v_SpawnProcess(sa)
  struct V__SpawnArgs *sa ;
{
#ifdef UNIX
  char tb[512],*bp1 ;
#endif
#ifdef VMSOS
  int res,resx ;
   struct vms__dsc {
     short int length ;		/* Length of value */
     short int desc ;		/* Descriptive info */
     char *pointer ;		/* Pointer to value */
    } strdsc ;
#endif
#ifdef WINNT
   UCCHAR *param,file[V_FileName_Max] ;
   int i ;
#endif
  LENMAX waitMS ;
#ifdef WINNT

/*	Get max mseconds to wait for completion from high 16 bits of waitflag */
//	waitMS = SPAWNMAXWAITDEC(waitflag) ;
	waitMS = sa->waitSeconds * 1000 ;
	if (waitMS != V_SPAWNWAIT_Infinite) waitMS *= 1000 ;	/* If actual number of seconds then convert to msecs */

	switch (sa->waitFlags & 0xf)
	 { default:				return(FALSE) ;
	   case V_SPAWNPROC_Wait:
#ifdef NEWSPAWN
		{ LOGICAL ok ;
		  struct lcl__SpawnInfo toi ;

		  memset(&toi,0,sizeof toi) ; toi.waitMS = waitMS ; toi.exefile = sa->exeFile ; toi.argbuf = sa->argBuf ; toi.errbuf = sa->errBuf ; toi.fileId = sa->fileId ;
		  toi.bytesOut = (sa->kilobytesOut == 0 ? V_SPAWN_MAX_KBOUT : sa->kilobytesOut) * 1000 ;
		  ok = v_spawnDoIt(&toi) ;
		  if (sa->respnt != NULL) { logPNTv(sa->respnt,ok) } ;
		  return(ok) ;
		}
#else
	        { DWORD threadExitCode,res ; PROCESS_INFORMATION pi ;	/* Get process info from CreateProcess */
	          HANDLE hThread ;
#ifdef V4UNICODE
		  STARTUPINFOW si ;		/* Process startup info */
#else
		  STARTUPINFO si ;		/* Process startup info */
#endif
		  memset(&si,0,sizeof si) ; si.cb = sizeof si ;
		  UCGetStartupInfo(&si) ;
		  si.lpTitle = UClit("V4 Spawn") ;
		  si.dwFlags |= STARTF_USESTDHANDLES ;		/* Want subprocess to use handles of this process */
		  { HANDLE hDupStdOutput ;
		    if (!DuplicateHandle(GetCurrentProcess(),GetStdHandle(STD_OUTPUT_HANDLE),GetCurrentProcess(),&hDupStdOutput,0,TRUE,DUPLICATE_SAME_ACCESS))
		     { printf("???DuplicateHandle() failed\n") ;
		     } ;
		    si.hStdOutput = hDupStdOutput ;
		  } ;
		  if (sa->waitFlags & V_SPAWNPROC_NewWindow) si.lpDesktop = UClit("") ;
		  if (!UCCreateProcess(sa->exeFile,(sa->argBuf == NULL ? UClit("cmd") : sa->argBuf),NULL,NULL,TRUE,CREATE_UNICODE_ENVIRONMENT,NULL,NULL,&si,&pi))
		   { if (sa->errBuf != NULL) v_Msg(NULL,sa->errBuf,"SpawnFail","CreateProcess",(sa->argBuf == NULL ? UClit("cmd") : sa->argBuf),GetLastError()) ;
		     return(FALSE) ;
		   } ;
		  if (sa->sockPtr != NULL)
		   { struct lcl__SpawnProcThreadArg spta ;
		     DWORD dwThreadId ;
		     spta.socknum = *sa->sockPtr ; spta.hProcess = pi.hProcess ; spta.idProcess = pi.dwProcessId ;
		     hThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)v_SpawnSocketMonitor,&spta,0,&dwThreadId) ;
		   } ;
		  res = WaitForSingleObject(pi.hProcess,(waitMS == V_SPAWNWAIT_Infinite ? INFINITE : waitMS)) ;
		  if (res != WAIT_OBJECT_0)		/* If process did not terminate normally then force a termination */
		   { TerminateProcess(pi.hProcess,EXITABORT) ;
		   } ;
		  CloseHandle(pi.hProcess) ; CloseHandle(pi.hThread) ;
		  switch (res)
		   { default:		if (sa->errBuf != NULL) v_Msg(NULL,sa->errBuf,"SpawnFail","WaitForSingleObject",(sa->argBuf == NULL ? UClit("cmd") : sa->argBuf),GetLastError()) ;
		     case WAIT_TIMEOUT:	v_Msg(NULL,sa->errBuf,"SpawnTO",(sa->argBuf == NULL ? UClit("cmd") : sa->argBuf),(waitMS/1000)) ; return(FALSE) ;
		     case WAIT_OBJECT_0:	break ;
		   } ;
		  if (sa->sockPtr != NULL)
		   { GetExitCodeThread(hThread,&threadExitCode) ;
		     if (threadExitCode == STILL_ACTIVE) { TerminateThread(hThread,0) ; }
		      else { if (sa->errBuf != NULL) v_Msg(NULL,sa->errBuf,"SpawnPremTerm",(sa->argBuf == NULL ? UClit("cmd") : sa->argBuf)) ;
			     return(FALSE) ;
			   } ;
		   } ;
		  if (sa->respnt != NULL)
		   { ZPH(sa->respnt) ; sa->respnt->Dim = Dim_Logical ; sa->respnt->PntType = V4DPI_PntType_Logical ; sa->respnt->Bytes = V4PS_Int ; sa->respnt->Value.IntVal = TRUE ; } ;
		}
#endif
		return(TRUE) ;
	   case V_SPAWNPROC_NoWait:
#ifdef NEWSPAWN
		{ 
		  struct lcl__SpawnInfo *toi ;
		  HANDLE idThread ;
		  INTMODX intmodx ;

		  intmodx = UNUSED ;	/* (needed for CREATE_THREAD() macro) */  
/*		  Since running asynch make copies of arguments because we don't know what will happen to existing references */
		  toi = v4mm_AllocChunk(sizeof *toi,TRUE) ; toi->waitMS = waitMS ; toi->fileId = sa->fileId ;
		  toi->bytesOut = (sa->kilobytesOut == 0 ? V_SPAWN_MAX_KBOUT : sa->kilobytesOut) * 1000 ;
		  if (sa->exeFile != NULL) { toi->exefile = v4mm_AllocUC(UCstrlen(sa->exeFile)) ; UCstrcpy(toi->exefile,sa->exeFile) ; } ; 
		  toi->errbuf = (sa->errBuf == NULL ? sa->errBuf : v4mm_AllocUC(1024)) ;
		  if (sa->argBuf != NULL) { toi->argbuf = v4mm_AllocUC(UCstrlen(sa->argBuf)) ; UCstrcpy(toi->argbuf,sa->argBuf) ; } ;
/*		  Just call v_spawnDoIt as thread and let it do its thing */
		  if (sa->fileId != UNUSED) vout_FileIdViaThread(sa->fileId,TRUE) ;
		  toi->noWait = TRUE ; CREATE_THREAD(v_spawnDoIt,idThread,toi,NULL,toi->errbuf)
/*		  Have to wait until subprocess is created so we can return process & thread handles */
		  if (sa->respnt != NULL)
		   { INDEX wx ;
		     for(wx=0;wx<1000;wx++)
		      { HANGLOOSE(5) ;
		        if (toi->idProcess != 0) break ;
		      } ;
		     ZPH(sa->respnt) ; sa->respnt->Dim = Dim_UV4 ; sa->respnt->PntType = V4DPI_PntType_OSHandle ; sa->respnt->Bytes = V4PS_Int2 ;
		     sa->respnt->Value.Int2Val[0] = v_OSH_CreateRef(V4OSH_Type_SubProc,V4OSH_Status_Active,toi->idProcess,toi->idThread,sa->errBuf) ;
		     sa->respnt->Value.Int2Val[1] = UNUSED ;
		   } ;
		  return(TRUE) ;
fail:		  return(FALSE) ;
		}
#else
		{ PROCESS_INFORMATION pi ;
#ifdef V4UNICODE
		  STARTUPINFOW si ;		/* Process startup info */
#else
		  STARTUPINFO si ;		/* Process startup info */
#endif
		  memset(&si,0,sizeof si) ; si.cb = sizeof si ;
		  UCGetStartupInfo(&si) ;
		  si.lpTitle = UClit("V4 Spawn") ;
		  if (sa->waitFlags & V_SPAWNPROC_NewWindow)
		   { /* si.lpDesktop = "" ; */
		     si.dwFlags |= STARTF_USESHOWWINDOW ;
		     si.wShowWindow |= SW_SHOWNORMAL ;
		   } ;
		  if (!UCCreateProcess(sa->exeFile,(sa->argBuf == NULL ? UClit("cmd") : sa->argBuf),NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))
		   { if (sa->errBuf != NULL) v_Msg(NULL,sa->errBuf,"SpawnFail","CreateProcess",(sa->argBuf == NULL ? UClit("cmd") : sa->argBuf),GetLastError()) ;
		     return(FALSE) ;
		   } ;
		  if (sa->respnt != NULL)
		   { ZPH(sa->respnt) ; sa->respnt->Dim = Dim_UV4 ; sa->respnt->PntType = V4DPI_PntType_OSHandle ; sa->respnt->Bytes = V4PS_Int2 ;
		     sa->respnt->Value.Int2Val[0] = v_OSH_CreateRef(V4OSH_Type_SubProc,V4OSH_Status_Active,pi.hProcess,pi.hThread,sa->errBuf) ;
		     sa->respnt->Value.Int2Val[1] = UNUSED ;
		   } ;
		}
		return(TRUE) ;
#endif
	   case V_SPAWNPROC_Application:
		{
#ifdef V4UNICODE
		  SHELLEXECUTEINFOW sxi ;
#else
		  SHELLEXECUTEINFO sxi ;
#endif
		  memset(&sxi,0,sizeof sxi) ; sxi.cbSize = sizeof sxi ;
		  sxi.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NO_CONSOLE ;
		  if (sa->waitFlags) sxi.fMask |= SEE_MASK_NOCLOSEPROCESS ;
		  sxi.lpVerb = UClit("open") ;
		  param = UCstrchr(sa->argBuf,' ') ;
		  if (param == NULL) { UCstrcpy(file,sa->argBuf) ; sxi.lpParameters = UClit("") ; }
		   else { sxi.lpParameters = param + 1 ;
			  UCstrncpy(file,sa->argBuf,(param-sa->argBuf)) ; file[param-sa->argBuf] = UCEOS ;
			} ;
/*		  Convert any '/'s in filename to backslashes */
		  for (i=0;file[i]!=UCEOS;i++) { if (file[i] == UClit('/')) file[i] = UClit('\\') ; } ;
		  sxi.lpFile = file ;
		  sxi.lpDirectory = UClit("") ; sxi.nShow = SW_NORMAL ;
		  if (!UCShellExecuteEx(&sxi))
		   { if (sa->errBuf != NULL) v_Msg(NULL,sa->errBuf,"SpawnFail","ShellExecuteEx",sa->argBuf,GetLastError()) ; return(FALSE) ; } ;
		  if (sa->respnt != NULL)
		   { ZPH(sa->respnt) ; sa->respnt->Dim = Dim_UV4 ; sa->respnt->PntType = V4DPI_PntType_OSHandle ; sa->respnt->Bytes = V4PS_Int ;
		     sa->respnt->Value.IntVal = v_OSH_CreateRef(V4OSH_Type_SubProc,V4OSH_Status_Active,sxi.hProcess,INVALID_HANDLE_VALUE,sa->errBuf) ;
		   } ;
		  return(TRUE) ;
		}
	 } ;
#endif
#ifdef VMSOS
	if (sa->argBuf != NULL)
	 { strdsc.pointer = UCretASC(sa->argBuf) ; strdsc.length = strlen(strdsc.pointer) ; strdsc.desc = 0 ;
	   res = LIB$SPAWN(&strdsc,0,0,0,0,0,&resx) ;
	 } else { res = LIB$SPAWN() ; } ;
	if ((res & 1) == 0)
	 { if (sa->errBuf != NULL) v_Msg(NULL,sa->errBuf,"LIB$SPAWN","ShellExecuteEx",(sa->argBuf == NULL ? "" : sa->argBuf),res) ;
	   return(FALSE) ;
	 } ;
#endif
#ifdef UNIX
	bp1 = getenv("SHELL") ;			/* Want to run same shell as parent */
	if (bp1 == NULL) bp1 = "/bin/sh" ;
	strcpy(tb,bp1) ;
	if (fork() == 0)
	 { if (execlp(tb,tb,(sa->argBuf != NULL ? "-c" : NULL),UCretASC(sa->argBuf),0) == -1)
	    { if (sa->errBuf != NULL) v_Msg(NULL,sa->errBuf,"execlp","ShellExecuteEx",(sa->argBuf == NULL ? "" : sa->argBuf),errno) ;
	      return(FALSE) ;
	    } ;
	 } else wait(NULL) ;
#endif
	return(TRUE) ;
}

/*	v_OSH_CreateRef - Creates a new OSH entry & returns unique ref to it		*/
/*	Call: ref = v_OSH_CreateRef( oshtype , oshstatus, info1 , info2 , errmsg )
	  where ref is assigned unique reference for the subprocess (UNUSED if error, errmsg updated),
		oshtype is type - V4OSH_Type_xxx,
		oshstatus is initial status - V4OSH_Status_xxx,
		info1 & info2 are OS dependant parameters,
		errormsg is updated with any errors					*/

int v_OSH_CreateRef(oshtype,oshstatus,info1,info2,errmsg)
  int oshtype ;
#ifdef WINNT
  HANDLE info1,info2 ;
#else
  int info1,info2 ;
#endif
  UCCHAR *errmsg ;
{ 
	if (gpi->osh == NULL) gpi->osh = (struct V4OSH__Table *)v4mm_AllocChunk(sizeof *gpi->osh,TRUE) ;
	if (gpi->osh->Count >= V4OSH_Max) { v_Msg(NULL,errmsg,"SpawnMaxProc1",V4OSH_Max) ; return(UNUSED) ; } ;
	gpi->osh->Entry[gpi->osh->Count].Ref = gpi->osh->NextAvailRef ;
	gpi->osh->Entry[gpi->osh->Count].Type = oshtype ;
	gpi->osh->Entry[gpi->osh->Count].Status = oshstatus ;
#ifdef WINNT
	gpi->osh->Entry[gpi->osh->Count].hProcess = info1 ; gpi->osh->Entry[gpi->osh->Count].hThread = info2 ;
#endif
	gpi->osh->Count++ ;
	return(gpi->osh->NextAvailRef++) ;
}

/*	v_OSH_IsProcThreadActive - Returns OS process/fork handle if subprocess/thread still active		*/
/*	Call: handle = v_OSH_IsProcThreadActive( ref )
	  where logical is TRUE if ref is still active,
		ref is unique process/thread ref assigned by v_OSH_CreateRef()	*/

LOGICAL v_OSH_IsProcThreadActive(ref)
  int ref ;
{ 
  int i ;

	if (gpi->osh == NULL) return(FALSE) ;		/* No active (never have been) */
	for(i=0;i<gpi->osh->Count;i++)
	 { if (ref == gpi->osh->Entry[i].Ref) break ; } ;
	if (i >= gpi->osh->Count) return(FALSE) ;
	if (gpi->osh->Entry[i].Type != V4OSH_Type_SubProc) return(FALSE) ;
	v_OSH_VerifyProcThread(gpi->osh,i) ;
	if (gpi->osh->Entry[i].Status != V4OSH_Status_Active) return(FALSE) ;
	return(TRUE) ;
}


LOGICAL v_OSH_VerifyProcThread(osh,refx)
  struct V4OSH__Table *osh ;
  int refx ;
{ 
  int ok, res ;

	if (refx < 0 || refx >= osh->Count) return(FALSE) ;
	if (gpi->osh->Entry[refx].Type != V4OSH_Type_SubProc) return(FALSE) ;
#ifdef WINNT
	ok = GetExitCodeProcess(osh->Entry[refx].hProcess,&res) ;
	if (ok == 0) { gpi->osh->Entry[refx].Status = V4OSH_Status_Unknown ; return(FALSE) ; } ;
	gpi->osh->Entry[refx].Status = (res == STILL_ACTIVE ? V4OSH_Status_Active : V4OSH_Status_Done) ;
	return(TRUE) ;
#endif

	return(FALSE) ;
}

/*	v_OSH_TypeDisplayer - Returns ASCIZ value of OSH ref type	*/
/*	Call: type = v_OSH_TypeDisplayer( ref )
	  where type is (char *) to displayer string,
		ref is OSH ref number					*/

UCCHAR *v_OSH_TypeDisplayer(ref)
  int ref ;
{ 
  static UCCHAR tbuf[24] ;
  int i ;

	if (gpi->osh == NULL) return(UClit("?")) ;		/* No active (never have been) */
	for(i=0;i<gpi->osh->Count;i++)
	 { if (ref == gpi->osh->Entry[i].Ref) break ; } ;
	if (i >= gpi->osh->Count) return(UClit("?")) ;
	switch (gpi->osh->Entry[i].Type)
	 { default:			UCsprintf(tbuf,UCsizeof(tbuf),UClit("?#%d?"),gpi->osh->Entry[i].Type) ; return(tbuf) ;
	   case V4OSH_Type_SubProc:	return(UClit("Subprocess")) ;
	   case V4OSH_Type_Thread:	return(UClit("Thread")) ;
	   case V4OSH_Type_Fiber:	return(UClit("Fiber")) ;
	 } ;
} ;

#ifdef WINNT
/*	v_OSH_WinNTProcHandleList - Updates list of active process handles	*/
/*	Call: count = v_OSH_WinNTProcHandleList( hlist )
	  where count is number updated (0 if none),
		hlist is HANDLE array to be updated with handles		*/

int v_OSH_WinNTProcHandleList(hlist)
  HANDLE hlist[] ;
{ 
  int i,count ;

	if (gpi->osh == NULL) return(0) ;		/* No active (never have been) */
	for(i=0,count=0;i<gpi->osh->Count;i++)
	 { if (!v_OSH_IsProcThreadActive(gpi->osh->Entry[i].Ref)) continue ;
	   hlist[count++] = gpi->osh->Entry[i].hProcess ;
	 } ;
	return(count) ;
}
#endif

#ifdef WINNT
LOGICAL v_IsAConsole(fp)
  FILE *fp ;
{
	return(_isatty(_fileno(fp))) ;
}
#else
LOGICAL v_IsAConsole(fp)
  FILE *fp ;
{
	return(isatty(fileno(fp))) ;
}
#endif
///*	v_IsBatchJob - Returns TRUE if running from batch job, otherwise FALSE */
//
//LOGICAL v_IsBatchJob()
//{
//#ifdef WINNT
///*	If stdin is from a disk then assume going to a batch job */
//	return(GetFileType(GetStdHandle(STD_INPUT_HANDLE)) == FILE_TYPE_DISK) ;
//#endif
//#ifdef POSIX
//	return(isatty(0) == FALSE) ; 		/* Assume batch job if stdin/stdout not a terminal */
//#endif
//}

/*	v_MakeOpenTmpFile - Creates & Opens (w+b) temp file				*/
/*	Call: file = v_MakeOpenTmpFile(prefix,filname,namemax,openmode,errmsg)
	  where file is stream pointer (NULL if error or openmode NULL),
		prefix is temp filename prefix,
		filename is updated with temp filename (empty string if error). If NULL then filename not returned,
		namemax is max bytes of filename,
		openmode is 1-4 character open mode string (ex: "w", "wb", etc.) or NULL if file not to be opened,
		errmsg is updated with error message (if error and errmsg not NULL)	*/
/*	Note: if filename is NULL and openmode is "wb" then tmpfile() is used and prefix is ignored */

struct vc__TmpFileList {
  struct vc__TmpFileList *next ;		/* Links to next one */
  UCCHAR *tmpfile ;				/* Pointer to temp file name */
} ;
struct vc__TmpFileList *vctfl = NULL ;		/* Temp file list - use in v4mm_FreeResources() to delete all temp files */

FILE *v_MakeOpenTmpFile(prefix,filename,namemax,openmode,errmsg)
  UCCHAR *prefix, *filename ;
  char *openmode ;
  UCCHAR *errmsg ;
  int namemax ;
{ FILE *fp ;
  static int callcnt = 0 ;
  struct vc__TmpFileList *tvctfl ;
  UCCHAR tmpfiledir[V_FileName_Max],tmpfilebuf[V_FileName_Max] ;
	
	callcnt++ ;					/* Track number of calls in case we have to auto-create our own temp file */
	
	if (filename == NULL && (openmode == NULL ? FALSE : strcmp(openmode,"wb") == 0))
	 { fp = tmpfile() ; if (fp != NULL) return(fp) ;
	   if (errmsg != NULL) v_Msg(NULL,errmsg,"TempFileFail",errno,UClit("tmpfile() call")) ;
	   return(NULL) ;
	 } ;

#ifdef WINNT
	if (!v_UCGetEnvValue(UClit("temp"),tmpfiledir,UCsizeof(tmpfiledir)))
	 { if (!v_UCGetEnvValue(UClit("tmp"),tmpfiledir,UCsizeof(tmpfiledir))) { UCstrcpy(tmpfiledir,UClit(".")) ; } ; } ;
	if (tmpfiledir[UCstrlen(tmpfiledir)-1] != UClit('\\')) UCstrcat(tmpfiledir,UClit("\\")) ;
#elif defined UNIX
	if (!v_UCGetEnvValue(UClit("temp"),tmpfiledir,UCsizeof(tmpfiledir)))
	 { if (!v_UCGetEnvValue(UClit("tmp"),tmpfiledir,UCsizeof(tmpfiledir))) { UCstrcpy(tmpfiledir,UClit(".")) ; } ; } ;
	if (tmpfiledir[UCstrlen(tmpfiledir)-1] != UClit('/')) UCstrcat(tmpfiledir,UClit("/")) ;
#else
	ZUS(tmpfiledir) ;
#endif

	v_Msg(NULL,tmpfilebuf,"@%1U%2U%3d_%4d_%5d.tmp",tmpfiledir,prefix,clock(),CURRENTPID,callcnt) ;

	if (filename != NULL)
	 { if(UCstrlen(tmpfilebuf) >= namemax)
	    { if (errmsg != NULL) v_Msg(NULL,errmsg,"TempFileFail2",UCstrlen(tmpfilebuf),namemax) ; return(NULL) ; } ;
	   UCstrcpy(filename,tmpfilebuf) ;
	 } ;
	if (openmode != NULL)
	 { 
	   fp = UCfopenAlt(tmpfilebuf,ASCretUC(openmode)) ;
	   if (fp == NULL) { if (errmsg != NULL) v_Msg(NULL,errmsg,"TempFileFail3",tmpfilebuf,errno) ; } ;
	 } ;
//	if (tmpfilebuf != NULL && tmpfilebuf != tmpfilebuf) free(tmpfilebuf) ;	/* If tmpfilebuf assigned via tmpfile() then free memory */
/*	Keep track of temp files in runtime list so we can delete when V4 exits */
	tvctfl = v4mm_AllocChunk(sizeof *tvctfl,FALSE) ; tvctfl->next = vctfl ; vctfl = tvctfl ;
	tvctfl->tmpfile = v4mm_AllocUC(UCstrlen(tmpfilebuf)) ; UCstrcpy(tvctfl->tmpfile,tmpfilebuf) ;
	return(fp) ;
}



UCCHAR *v_UCLogicalDecoder(cspec,flags,index,errmsg)
  UCCHAR *cspec ;
  int flags ;			/* see VLOGDECODE_xxx */
  int index ;			/* If > 0 then return index'th path in logical definitions */
  UCCHAR *errmsg ;		/* If not NULL then updated with any error messages */
{ static UCCHAR ffn[200],normspec[200] ;
   UCCHAR *b,*b1,*ns,*spec ;
   UCCHAR *comma,*colon ; UCCHAR firstffn[300],path[500],npath[500] ;
   int i,cnt ; FILE *tf ;

#ifdef VMSOS
	return(cspec) ;						/* VMS does all this for us */
#endif

#ifdef WINNT
	if ((flags & VLOGDECODE_KeepCase) == 0)
	 { UCcnvlower(normspec,cspec) ; }
	 else { UCstrcpy(normspec,cspec) ; }
	ns = UCstrstr(normspec,UClit("  .")) ;				/* Any spaces before extension? */
	if (ns != NULL)
	 { for(;*ns == UClit(' ');ns--) {} ;				/* Backup to last non-space */
	   for(i=TRUE,b1=path,b=normspec;;)
	    { if (!i && *b == UClit('.')) i = TRUE ;
	      if (i) *(b1++) = *b ; if (*b == '\0') break ;
	      if (b == ns) i = FALSE ; b++ ;
	    } ; UCstrcpy(normspec,path) ;
	 } ;
	spec = normspec ;
	if (*(spec+1) == UClit(':')) return(index > 1 ? NULL : spec) ;		/* Don't decode if single letter device: c:xxx */
#endif
#ifdef UNIX
	if ((flags & VLOGDECODE_KeepCase) == 0)
	 { UCcnvlower(normspec,cspec) ; }
	 else { UCstrcpy(normspec,cspec) ; }
	spec = normspec ;
#endif
	if (*spec == UClit('$')) spec++ ;				/* Get rid of possible leading $ */
	if ((b = UCstrrchr(spec,';')) != NULL) *b = '\0' ;	/* Strip of trailing ";xxx" - VMS generation numbers */
/*	First see if we have a device (xxx:file) */
	if ((colon = UCstrchr(spec,':')) == 0)
#ifdef UNIX
	 return(spec) ;	/* No device - just return argument */
#endif
#ifdef WINNT
	 return(index > 1 ? NULL : spec) ;	/* No device - just return argument */
#endif
/*	Got a colon - lets see if we can decode it */
	UCstrncpy(ffn,spec,colon-spec) ; ffn[colon-spec] = UCEOS ;	/* Copy up to the colon */
/*	Now zip-zop thru the environment to see we have an path */
	if (!v_UCGetEnvValue(ffn,path,UCsizeof(path)))
	 { v_Msg(NULL,errmsg,"LogDecNoLogUC",ffn,cspec) ; return(NULL) ; } ;
/*	Loop thru all directories specified */
	ZUS(firstffn) ;
	if (index > 0) flags |= VLOGDECODE_Exists ;		/* Set to force sequence through logical defs */
	for(cnt=1;;cnt++)
	 { if (UCempty(path))				/* If nothing in path, return current file */
	    { if (index > 0) return(NULL) ;		/* If looking for n'th then return null (end of list) */
	      if (firstffn[0] != UCEOS) UCstrcpy(ffn,firstffn) ;
	      return(ffn) ;
	    } ;
	   if ((comma = UCstrchr(path,',')) == 0) /* Look for "," in path */
	    {
	      if (path[UCstrlen(path)-1] == UClit(':'))		/* Got a nested logical ? */
	       { path[UCstrlen(path)-1] = 0 ;
	         if (!v_UCGetEnvValue(path,ffn,UCsizeof(ffn)))
	          { v_Msg(NULL,errmsg,"LogDecNestedUC",path,cspec) ; return(NULL) ; } ;
	       } else { UCstrcpy(ffn,path) ; } ;
#ifdef WINNT
	      if (ffn[UCstrlen(ffn)-1] != '\\') UCstrcat(ffn,UClit("\\")) ;
#endif
#ifdef UNIX
	      if (ffn[UCstrlen(ffn)-1] != UClit('/')) UCstrcat(ffn,UClit("/")) ;
#endif
	      UCstrcat(ffn,colon+1) ;	/* And remainder of file spec */
#ifdef WINNT
	      i = UCstrlen(ffn) ; if (ffn[i-1] == UClit('\\')) ffn[i-1] = UCEOS ;	/* Don't return with trailing "/" */
#endif
#ifdef UNIX
	      i = UCstrlen(ffn) ; if (ffn[i-1] == UClit('/')) ffn[i-1] = UCEOS ;	/* Don't return with trailing "/" */
#endif
	      return(index > cnt ? NULL : ffn) ;
	    } ;
	   UCstrncpy(ffn,path,comma-path) ; ffn[comma-path] = UCEOS ; UCstrcpy(path,&path[1+comma-path]) ;
	   if (ffn[UCstrlen(ffn)-1] == UClit(':'))		/* Got a nested logical ? */
	    { ffn[UCstrlen(ffn)-1] = UCEOS ;
	      if (!v_UCGetEnvValue(ffn,npath,UCsizeof(npath)))
	       { v_Msg(NULL,errmsg,"LogDecNestedUC",path,cspec) ; return(NULL) ; } ;
	      UCstrcpy(ffn,npath) ;
	    } ;
#ifdef WINNT
	   if (ffn[UCstrlen(ffn)-1] != UClit('\\')) UCstrcat(ffn,UClit("\\")) ;
#endif
#ifdef UNIX
	   if (ffn[UCstrlen(ffn)-1] != UClit('/')) UCstrcat(ffn,UClit("/")) ;
#endif
	   UCstrcat(ffn,colon+1) ;	/* And remainder of file spec */
/*	   Try and read this file to make sure it exists */
	   if (index > 0 && cnt == index) return(ffn) ;
	   if ((tf=UCfopen(ffn,"r")) != NULL) { fclose(tf) ; return(ffn) ; } ;
	   if (flags & VLOGDECODE_Exists)
	    { if (firstffn[0] == '\0') UCstrcpy(firstffn,ffn) ;	/* Save first path in case we don't find file */
	    } ;
	 } ;
}


LOGICAL v_UCGetEnvValue(varname,resbuf,resmax)
  UCCHAR *varname ;		/* Variable to look-up */
  UCCHAR *resbuf ;		/* Hold value */
  int resmax ;			/* Max bytes in value */
{
#ifdef WINNT
  extern UCCHAR **startup_envp ;
  UCCHAR *def,*dl ; UCCHAR *b ;
  int i,j ;
  UCCHAR ucname[256] ;
  static HKEY rkey ;

	if ((b = vlog_GetSymValue(varname)) != NULL)
	 { for(j=0;j<resmax-1&&b[j]!='\0';j++) { resbuf[j] = b[j] ; } ; resbuf[j] = UCEOS ;
	   return(TRUE) ;
	 } ;
	for(i=0;;i++)
	 { def = startup_envp[i] ;		/* def = pointer to next item in environment */
	   if (def == NULL) break ;		/* No def, not found, try registry below */
	   dl = UCstrchr(def,'=') ;
//printf("def=*%s*   dl=%p\n",def,dl) ;
//if (dl == NULL) continue ;
	   for(j=0;j<dl-def;j++) { ucname[j] = def[j] ; } ; ucname[dl-def] = UCEOS ;
	   if (UCstrcmpIC(varname,ucname) == 0)	/* Got a match */
	    { UCstrcpy(resbuf,(dl+1)) ;
	      return(TRUE) ;
	    } ;
	 } ;
//	if (rkey == 0)
//	 { RegOpenKeyEx(HKEY_CURRENT_USER,"Environment",NULL,KEY_EXECUTE,&rkey) ; } ;
//	for(i=0;;i++)
//	 { valstrsize = sizeof valstr ; valbufsize = sizeof valbuf ;
//	   if (RegEnumValue(rkey,i,valstr,&valstrsize,NULL,&type,valbuf,&valbufsize) != ERROR_SUCCESS) break ;
//	   for(j=0;;j++) { ucname[j] = valstr[j] ; if (valstr[j] == 0) break ; } ;
//	   if (UCstrcmpIC(ucname,varname) != 0) continue ;
//	   j = (resmax > valbufsize ? valbufsize : resmax-1) ;
//	   for(i=0;i<j;i++) { resbuf[i] = valbuf[i] ; } ; resbuf[i] = UCEOS ;
//	   return(TRUE) ;
//	 } ;
	return(FALSE) ;
#else
   extern char **startup_envp ;
   int i,j ; char *b,*def,*dl ; UCCHAR ucname[300] ;

	if ((b = vlog_GetSymValue(varname)) != NULL)
	 { for(j=0;j<resmax-1&&b[j]!='\0';j++) { resbuf[j] = b[j] ; } ; resbuf[j] = UCEOS ;
	   return(TRUE) ;
	 } ;

	for(i=0;;i++)
	 { def = startup_envp[i] ;		/* def = pointer to next item in environment */
	   if (def == 0) break ;		/* No def, not found, try registry below */
	   dl = (char *)strchr(def,'=') ; if (dl == NULL) continue ;
	   for(j=0;j<dl-def;j++) { ucname[j] = def[j] ; } ; ucname[dl-def] = UCEOS ;
	   if (UCstrcmpIC(varname,ucname) == 0)	/* Got a match */
	    { dl++ ; UCstrcpyAtoU(resbuf,dl) ;
	      return(TRUE) ;
	    } ;
	 } ;
	return(FALSE) ;
#endif
}

//LOGICAL v3_GetEnvValue(uvarname,resbuf,resmax)
//  UCCHAR *uvarname ;		/* Variable to look-up */
//  UCCHAR *resbuf ;		/* Hold value */
//  int resmax ;			/* Max bytes in value */
//{ 
//	return(v_UCGetEnvValue(uvarname,resbuf,resmax)) ;
//
//#ifdef WINNT
//  extern char **startup_envp ;
//  char *b,*def,*dl,varname[64] ; UCCHAR *ub ;
//  int i,j ;
//  int *type ; int valstrsize,valbufsize ;
//  UCCHAR valstr[500],valbuf[500], name[500],name1[500] ;
//  static HKEY rkey ;
//
//
//	if ((ub = vlog_GetSymValue(uvarname)) != NULL)
//	 { i = UCstrlen(ub) ; i = (i > resmax ? resmax-1 : i) ;
//	   UCstrncpy(resbuf,ub,i) ; *(resbuf+i) = UCEOS ;
//	   return(TRUE) ;
//	 } ;
//	UCstrcpyToASC(varname,uvarname) ;
//	for(i=0;;i++)
//	 { def = startup_envp[i] ;	/* def = pointer to next item in environment */
//	   if (def == 0) break ;	/* No def, not found, try registry below */
//	   dl = (char *)strchr(def,'=') ; strncpy(name,def,dl-def) ; name[dl-def] = 0 ;
//	   for(b=name;*b != 0;b++) { *b = tolower(*b) ; } ;
//	   if (strcmp(varname,name) == 0) /* Got a match */
//	    { UCstrcpyAtoU(resbuf,(dl+1)) ;		 /* Copy logical into path buffer - may be more than one directory */
//	      return(TRUE) ;
//	    } ;
//	 } ;
//	for(i=0;*(varname+i)!=0;i++) { name[i] = tolower(*(varname+i)) ; } ; name[i] = 0 ;
//	if (rkey == 0)
//	 { RegOpenKeyEx(HKEY_CURRENT_USER,"Environment",NULL,KEY_EXECUTE,&rkey) ; } ;
//	for(i=0;;i++)
//	 { valstrsize = sizeof valstr ; valbufsize = sizeof valbuf ;
//	   if (RegEnumValue(rkey,i,valstr,&valstrsize,NULL,&type,valbuf,&valbufsize) != ERROR_SUCCESS) break ;
//	   for(j=0;;j++) { name1[j] = tolower(valstr[j]) ; if (valstr[j] == 0) break ; } ;
//	   if (strcmp(name,name1) != 0) continue ;
//	   j = (resmax > valbufsize ? valbufsize : resmax-1) ;
//	   strncpy(resbuf,ASCretUC(valbuf),j) ; resbuf[j] = UCEOS ;
//	   return(TRUE) ;
//	 } ;
//	return(FALSE) ;
//#else
//   extern char **startup_envp ;
//   int i ; char *b,*def,*dl,name[300] ;
//
//	if ((b = vlog_GetSymValue(varname)) != NULL)
//	 { i = strlen(b) ; i = (i > resmax ? resmax-1 : i) ;
//	   strncpy(resbuf,b,i) ; *(resbuf+i) = 0 ;
//	   return(TRUE) ;
//	 } ;
//	for(i=0;;i++)
//	 { def = startup_envp[i] ;	/* def = pointer to next item in environment */
//	   if (def == 0) return(FALSE) ;
//	   dl = (char *)strchr(def,'=') ; strncpy(name,def,dl-def) ; name[dl-def] = 0 ;
//	   for(b=name;*b != 0;b++) { *b = tolower(*b) ; } ;
//	   if (strcmp(varname,name) == 0) break ;	/* Got a match */
//	 } ;
//	strcpy(resbuf,dl+1) ;		 /* Copy logical into path buffer - may be more than one directory */
//	return(TRUE) ;
//#endif
//}

#define lcl_SymVal_Max 50

struct lcl__SymVal
{ int count ;
  struct {
     UCCHAR Sym[32+1] ;
     UCCHAR Val[100+1] ;
   } Entry[lcl_SymVal_Max] ;
} ;
struct lcl__SymVal *lsv ;

/*	vlog_StoreSymValue - Stores Symbol/Value Pair for later use	*/
/*	Call: result = vlog_StoreSymValue( symstr , valstr )
	  where result is TRUE of ok, FALSE if failed,
		symstr is symbol,
		valstr is value for symbol				*/

LOGICAL vlog_StoreSymValue(symstr,valstr)
  UCCHAR *symstr,*valstr ;
{ int i ; UCCHAR buf[sizeof lsv->Entry[0].Sym], buf1[sizeof lsv->Entry[0].Val] ;

	if (lsv == NULL) { lsv = (struct lcl__SymVal *)v4mm_AllocChunk(sizeof *lsv,FALSE) ; lsv->count = 0 ; } ;
	if (lsv->count >= lcl_SymVal_Max) return(FALSE) ;
	if (UCstrlen(symstr)+1 > UCsizeof(lsv->Entry[0].Sym)) return(FALSE) ;
	if (UCstrlen(valstr)+1 > UCsizeof(lsv->Entry[0].Val)) return(FALSE) ;
	for(i=0;;i++) { buf[i] = UCTOUPPER(symstr[i]) ; if (buf[i] == UCEOS) break ; } ;
	for(i=0;;i++) { buf1[i] = (valstr[i] == UClit('#') ? UClit(' ') : valstr[i]) ; if (buf1[i] == UCEOS) break ; } ;
	UCstrcpy(lsv->Entry[lsv->count].Sym,buf) ; UCstrcpy(lsv->Entry[lsv->count].Val,buf1) ;
	lsv->count ++ ;
	return(TRUE) ;
}

/*	vlog_GetSymValue - Gets value associated with a symbol		*/
/*	Call: pointer = vlog_GetSymValue( symstr )
	  where pointer is pointer to value, NULL if symbol not found,
		symstr is symbol string					*/

UCCHAR *vlog_GetSymValue(symstr)
  UCCHAR *symstr ;
{ int i ; UCCHAR buf[sizeof lsv->Entry[0].Sym] ;

	if (lsv == NULL) return(NULL) ;		/* No symbols defined! */
	for(i=0;;i++) { buf[i] = UCTOUPPER(symstr[i]) ; if (buf[i] == UCEOS) break ; } ;
	for(i=0;i<lsv->count;i++) { if (UCstrcmp(buf,lsv->Entry[i].Sym) == 0) return(lsv->Entry[i].Val) ; } ;
	return(NULL) ;
}

/*	v_GetV4HomePath - Returns path to V4 Home Directory		*/
/*	Call: path = v_GetV4HomePath( file )
	  where path is string path name to file (includes file),
		file is string name of file				*/

UCCHAR *v_GetV4HomePath(file)
  UCCHAR *file ;
{ static UCCHAR xv4Path[V_FileName_Max], *b ;
  static UCCHAR respath[V_FileName_Max] ; LOGICAL havehome ; static LOGICAL didInit=FALSE ;


#ifdef VMSOS
	UCstrcpy(respath,UClit("v4_home:")) ; UCstrcat(respath,file) ;
	return(respath) ;			/* On VMS just return v4_home:file */
#endif

	if (didInit)				/* Windows has potential deadlock condition within GetModuleFileNameW - only call it once to avoid problems */
	 { UCstrcpy(respath,xv4Path) ; UCstrcat(respath,file) ;
	   return(respath) ;
	 } ;
	havehome = v_UCGetEnvValue(UClit("v4_home"),xv4Path,sizeof xv4Path) ;
#ifdef WINNT
	if (havehome)				/* If we have v4_home then decode, otherwise just return file name */
	 { UCstrcpy(respath,xv4Path) ; if (respath[UCstrlen(xv4Path)-1] != UClit('\\')) UCstrcat(respath,UClit("\\")) ;
	   UCstrcat(respath,file) ; return(respath) ;
	 } else
	 { ZUS(respath) ;			/* If we can get runtime directory of xv4 then assume that is home */
	   if (UCGetModuleFileName(NULL,xv4Path,UCsizeof(xv4Path)))
	    { b = UCstrrchr(xv4Path,'\\') ;		/* Search for right-most backslash */
	      if (b != NULL) { *(b+1) = '\0' ; UCstrcpy(respath,xv4Path) ; } ;
	    } ;
	   UCstrcat(respath,file) ;
	   didInit = TRUE ;
	   return(respath) ;
	 } ;
	
#else
	if (havehome)				/* If we have v4_home then decode, otherwise just return file name */
	 { UCstrcpy(respath,xv4Path) ; if (respath[UCstrlen(xv4Path)-1] != UClit('/')) UCstrcat(respath,UClit("/")) ; } else { ZUS(respath) ; } ;
	UCstrcat(respath,file) ;
/*	Convert to lower case (NOTE - since uci->xxx probably not defined use tolower macro (THEREFORE CANNOT USE NON-ASCII in V4 special file names) */
	{ int i ; for(i=0;respath[i]!=UClit('\0');i++) respath[i] = tolower(respath[i]) ; } ;
	return(respath) ;
#endif
	return(respath) ;
}

/*	v_OSErrString - Returns formatted error message based on OS error code */

char *v_OSErrString(errnum)
  int errnum ;
{ static char EMsgBuf[280] ;
  char tbuf[256] ; int l ;

#ifdef WINNT
	ZS(tbuf) ;
	l = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,NULL,errnum,0,tbuf,sizeof tbuf,NULL );
	if (l <= 0)
	 { sprintf(EMsgBuf,"%d:[? Windows FormatMessage() failed with error %d]",errnum,GetLastError()) ;
	 } ;
	l = strlen(tbuf) - 1 ; for(;l>=0&&tbuf[l]<=0x20;l--) { tbuf[l] = '\0' ; } ;
	sprintf(EMsgBuf,"%d:[%s]",errnum,tbuf) ;
	return(EMsgBuf) ;
#endif
#ifdef VMSOS
	mscu_vms_errmsg(errnum,tbuf) ;
	sprintf(EMsgBuf,"%d:[%s]",errnum,tbuf) ;
	return(EMsgBuf) ;
#endif
#ifdef UNIX
	sprintf(EMsgBuf,"%d:[%s]",errnum,strerror(errnum)) ;
	return(EMsgBuf) ;
#endif
	sprintf(EMsgBuf,"%d",errnum) ;
	return(EMsgBuf) ;
}


#define VOUT_StreamBufferMax 4096	/* Number of bytes in file stream */
#define VOUT_FileMax 50

struct vout__Streams {
  FILEID LUFileId ;			/* Last used unique file id */
//  LENMAX FileCnt ;			/* Number defined files */
  INDEX highX ;				/* Highest index of active entries below */
  struct {
    FILEID FileId ;			/* Unique file identifier */
 #ifdef WINNT
    HANDLE hFile ;
 #else
    int hFile ;				/* File handle */
#endif
    FILE *fp ;				/*  or file pointer */
    struct V4DPI__LittlePoint id ;	/* Point to identify this file */
    ETYPE FileType ;			/* Type of file (see VOUT_FileType_xxx) */
    ETYPE TextType ;			/* (sub)Type of text file (see V4L_TextFileType_xxx) */
    UCCHAR FileName[V_FileName_Max] ;	/* File name associated with entry */
    char *ptr ;				/* Point to end of buffer */
    char *Buffer ;			/* Buffer holding current data */
    UCCHAR *uptr ;			/* UNICODE pointer to end of uBuffer */
    UCCHAR *uBuffer ;			/* UNICODE buffer */
    struct V4DPI__Point *BPoint ;	/* An intersection point to be bound to BigText value on closing */
    struct V4LEX__BigText *bt ;
    struct V4RPT__RptInfo *rri ;	/* If not NULL then Rpt() bound to this output path */
    COUNTER BytesAlloc ;		/* Number of bytes allocated in buffer (initial = VOUT_StreamBufferMax) */
    COUNTER BytesRemain ;		/* Remaining bytes in the buffer */
    COUNTER LineCount ;			/* Number of lines output */
    COUNTER charCount ;			/* Number of characters output */
    LOGICAL isCommandFile ;		/* TRUE if this is an O/S command file (UNIX only!) */
    LOGICAL threadOpen ;		/* TRUE if this stream is open by a thread (try to wait for thread to finish before forcing a close) */
   } File[VOUT_FileMax] ;
  struct {
    int FileX ;				/* Index to appropriate file above */
   } Stream[VOUT_StreamMax] ;		/* Indexed by VOUT_xxx (e.g. VOUT_Data, VOUT_Progress) */
 } vS ;

#define vSInitNewFile(INDEX) memset(&vS.File[INDEX],0,sizeof vS.File[INDEX])

/*	O U T P U T   ( C O N S O L E   O R   O T H E R W I S E )	*/

/*	vout_FileName - returns filename associated with a stream or filex	*/
/*	Call: ptr = vout_FileName( sfx )
	  where ptr is ptr to UCCHAR stream name,
		sfx is stream index (e.g. VOUT_Data), or -filex			*/
UCCHAR *vout_FileName(sfx)
  int sfx ;
{ int filex ; enum DictionaryEntries deval ;

	filex = (sfx < 0 ? -sfx : vS.Stream[sfx].FileX) ;
	switch(vS.File[filex].FileType)
	 { 
	   case VOUT_FileType_StdOut:		return(v4im_GetEnumToUCVal(deval=_StdOut)) ;
	   case VOUT_FileType_Console:		return(v4im_GetEnumToUCVal(deval=_Console)) ;
	   case VOUT_FileType_None:		return(v4im_GetEnumToUCVal(deval=_Null)) ;
	   case VOUT_FileType_File:		return(vS.File[filex].FileName) ;
	   case VOUT_FileType_Socket:		return(v4im_GetEnumToUCVal(deval=_Socket)) ;
	   case VOUT_FileType_BigText:		return(v4im_GetEnumToUCVal(deval=_BigText)) ;
	   case VOUT_FileType_Buffer:		return(v4im_GetEnumToUCVal(deval=_Buffer)) ;
	   case VOUT_FileType_BufJSON:		return(v4im_GetEnumToUCVal(deval=_JSON)) ;
	 } ;
	return(v4im_GetEnumToUCVal(deval=_Unknown)) ;
}

/*	vout_Init - Inits VOUT_xxx structures & modules */

void vout_Init()
{ int sx ;

	memset(&vS,0,sizeof vS) ;
	vS.File[vS.highX].FileId = ++vS.LUFileId ;
	  UCstrcpy(vS.File[vS.highX].FileName,UClit("StdOut")) ; vS.File[vS.highX++].FileType = VOUT_FileType_StdOut ;
	vS.File[vS.highX].FileId = ++vS.LUFileId ;
	  UCstrcpy(vS.File[vS.highX].FileName,UClit("Console")) ;vS.File[vS.highX++].FileType = VOUT_FileType_Console ;
	vS.File[vS.highX].FileId = ++vS.LUFileId ;
	  UCstrcpy(vS.File[vS.highX].FileName,UClit("None")) ;vS.File[vS.highX++].FileType = VOUT_FileType_None ;
	for(sx=0;sx<VOUT_StreamMax;sx++)			/* Send all output to stdout as default */
	 { vS.Stream[sx].FileX = 0 ; } ;
}

/*	vout_PntIdToFileId - Looks up & returns unique FileId from PntId	*/
/*	Call: fileid = vout_PntIdToFileId( ctx , ptid )
	  where fileid is unique id, UNUSED if not found,
		ctx is context,
		ptid is pointer to point id					*/

FILEID vout_PntIdToFileId(ctx,ptid)
  struct V4C__Context *ctx ;
  struct V4DPI__LittlePoint *ptid ;
{
  INDEX i ;

	if (ptid->PntType == V4DPI_PntType_Dict)
	 { switch (v4im_GetDictToEnumVal(ctx,(P *)ptid))
	    { default:		break ;
	      case _StdOut:	return(vout_FileTypeToFileId(VOUT_FileType_StdOut)) ;
	      case _Console:	return(vout_FileTypeToFileId(VOUT_FileType_Console)) ;
	      case _None:	return(vout_FileTypeToFileId(VOUT_FileType_None)) ;
	    } ;
	 } ;
	for(i=0;i<vS.highX;i++) { if (memcmp(&vS.File[i].id,ptid,ptid->Bytes) == 0) return(vS.File[i].FileId) ; } ;
	return(UNUSED) ;
}

/*	vout_PntIdToFileX - Looks up & returns filex from PntId	*/
/*	Call: filex = vout_PntIdToFileX( ctx , ptid )
	  where filex is file index, UNUSED if not found,
		ctx is context,
		ptid is pointer to point id					*/

INDEX vout_PntIdToFileX(ctx,ptid)
  struct V4C__Context *ctx ;
  struct V4DPI__LittlePoint *ptid ;
{
  int i ;

	if (ptid->PntType == V4DPI_PntType_Dict)
	 { switch (v4im_GetDictToEnumVal(ctx,(P *)ptid))
	    { default:		break ;
	      case _StdOut:	return(vout_FileTypeToFileX(VOUT_FileType_StdOut)) ;
	      case _Console:	return(vout_FileTypeToFileX(VOUT_FileType_Console)) ;
	      case _None:	return(vout_FileTypeToFileX(VOUT_FileType_None)) ;
	    } ;
	 } ;
	for(i=0;i<vS.highX;i++) { if (memcmp(&vS.File[i].id,ptid,ptid->Bytes) == 0) return(i) ; } ;
	return(UNUSED) ;
}

/*	vout_FileIdToFileX - Looks up & returns filex from FileId	*/
/*	Call: filex = vout_FileIdToFileX( stream )
	  where filex is file index, UNUSED if not found,
		fileid is a file identifier				*/

INDEX vout_FileIdToFileX(fileid)
  FILEID fileid ;
{ INDEX fx ;

	for (fx=0;fx<vS.highX;fx++) { if (vS.File[fx].FileId == fileid) return(fx) ; } ;
	return(UNUSED) ;
}

/*	vout_StreamToFileX - Looks up & returns filex from stream	*/
/*	Call: filex = vout_StreamToFileX( stream )
	  where filex is file index, UNUSED if not found,
		stream is output stream (e.g. VOUT_Data)		*/

INDEX vout_StreamToFileX(stream)
  VSTREAM stream ;
{
	if (stream < 0) return(UNUSED) ;
	return(vS.Stream[stream].FileX) ;
}

/*	vout_FileTypeToFileId - Looks up & returns unique FileId currently associated with a FileType	*/
/*	Call: fileid = vout_FileTypeToFileId( filetype )
	  where fileid is unique id, UNUSED if not found, returns first id for VOUT_FileType_File
		filetype is file type									*/

FILEID vout_FileTypeToFileId(filetype)
  ETYPE filetype ;
{
  INDEX i ;

	for(i=0;i<vS.highX;i++) { if (vS.File[i].FileType == filetype) return(vS.File[i].FileId) ; } ;
	return(UNUSED) ;
}

/*	vout_FileTypeToFileX - Looks up & returns file index currently associated with a FileType	*/
/*	Call: fileX = vout_FileTypeToFileX( filetype )
	  where fileX is file index, UNUSED if not found, returns first id for VOUT_FileType_File
		filetype is file type									*/

int vout_FileTypeToFileX(filetype)
  int filetype ;
{
  int i ;

	for(i=0;i<vS.highX;i++) { if (vS.File[i].FileType == filetype) return(i) ; } ;
	return(UNUSED) ;
}

/*	vout_FileIdViaThread - Sets/Resets threadOpen attribute for file entry	*/
/*	Call: ok = vout_FileIdViaThread( fileId , isThread )
	  where ok is TRUE if done, FALSE if not (invalid fileId),
	  fileId is the file entry to be updated,
	  isThread is TRUE if this is being controlled by a thread		*/

LOGICAL vout_FileIdViaThread(fileId,isThread)
  FILEID fileId ;
  LOGICAL isThread ;
{ INDEX fx ;

	fx = vout_FileIdToFileX(fileId) ;
	if (fx == UNUSED) return(FALSE) ;
	vS.File[fx].threadOpen = isThread ;
	return(TRUE) ;
}

/*	vout_OpenStreamFile - Opens up (another) file for linkage to V4 output stream	*/
/*	Call: fileid = vout_OpenStreamFile( idpt , filename , dfltext , tcb , append , texttype, decoderFlags, errmsg )
	  where fileid is unique file identifier, UNUSED if problems
		idpt, if not NULL, is optional Id point associated with this stream,
		filename is string file name,
		dfltext is default extension (NULL if none),
		tcb, if not NULL, is token-control with next token being filename,
		append is TRUE to append to file, FALSE to create new,
		texttype is type of text file to create (see V4L_TextFileType_xxx),
		decodeFlags are optional v_UCLogicalDecoder() flags (0 for none),
		errmsg, if not NULL, updated with any problems				*/
		 
int vout_OpenStreamFile(idpt,filename,dfltext,tcb,append,texttype,decoderFlags,errmsg)
  struct V4DPI__LittlePoint *idpt ;
  UCCHAR *filename,*dfltext ;
  struct V4LEX__TknCtrlBlk *tcb ;
  int append,texttype,decoderFlags ;
  UCCHAR *errmsg ;
{ 
  UCCHAR tfilename[V_FileName_Max],fnbuf[V_FileName_Max],*dext,*ddev,*fns,*dfns ;
  INDEX fx ; COUNTER retry ;

	if (idpt != NULL)
	 { for(fx=0;fx<vS.highX;fx++)
	    { if (vS.File[fx].FileId <= 0 || memcmp(&vS.File[fx].id,idpt,idpt->Bytes) != 0) continue ;
printf("openstreamfile error, fx=%d, vS.File[fx].FileId=%d, idpt->Bytes=%d, vS.File[fx].id.Bytes=%d  \n",fx,vS.File[fx].FileId,idpt->Bytes,vS.File[fx].id.Bytes) ;
	      if (errmsg != NULL) v_Msg(NULL,errmsg,"OutputIdExists",idpt) ; return(UNUSED) ;
	    } ;
	 } ;
/*	Find lowest free slot */
	for(fx=0;fx<vS.highX;fx++) { if (vS.File[fx].FileId <= 0) break ; } ;
	if (fx >= VOUT_FileMax)
	 { if (errmsg != NULL) v_Msg(NULL,errmsg,"@Exceed maximum of %1d output files",VOUT_FileMax) ; return(UNUSED) ; } ;
	vSInitNewFile(fx) ;
	if (tcb == NULL && filename == NULL) { if (errmsg != NULL) v_Msg(NULL,errmsg,"@No tcb or filename specified") ; return(UNUSED) ; } ;
	if (tcb != NULL)
	 { dext = UClit(".com") ; ddev = NULL ;
#ifdef UNIX
	   dext = UClit(".ucf") ;
#endif
#ifdef WINNT
	   dext = UClit(".cmd") ; ddev = UClit(".\\") ;
#endif
	   v_ParseFileName(tcb,fnbuf,ddev,dext,NULL) ;
	   fns = fnbuf ;
	 } else
	 { fns = filename ;
	 } ;
	if ((dfns = v_UCLogicalDecoder(fns,decoderFlags|(append ? VLOGDECODE_Exists : VLOGDECODE_NewFile),0,errmsg)) == NULL) return(UNUSED) ;
	UCstrcpy(tfilename,dfns) ;
#ifdef UNIX
	vS.File[fx].isCommandFile = (UCstrstr(tfilename,UClit(".ucf")) != NULL) ;	/* If UNIX then test for command file */
#endif
	dext = UCstrstr(tfilename,UClit(".dat")) ;
	if (dext == NULL ? FALSE : *(dext+4) == UCEOS)
	 { if (errmsg != NULL) v_Msg(NULL,errmsg,"FileCreateDAT",tfilename) ; return(UNUSED) ; } ;
#ifdef WINNT
#define _RETRYMAX 5
	for(retry=1;retry<_RETRYMAX;retry++)
	 { if ((vS.File[fx].hFile = UCCreateFile(tfilename,GENERIC_WRITE,0,NULL,(append ? OPEN_ALWAYS : CREATE_ALWAYS),FILE_ATTRIBUTE_NORMAL,NULL)) == INVALID_HANDLE_VALUE)
	    { 
	      if ((vS.File[fx].hFile = UCCreateFile(tfilename,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL)) == INVALID_HANDLE_VALUE)
	       { COUNTER ranHang =  (COUNTER)(vRan64_RandomU64() & 0x3f) ;
	         HANGLOOSE(ranHang) ; continue ;		/* If errors, then sleep & try again (random file-lock error on create ?) */
	       } ;
	    } ;
	   if (append) SetFilePointer(vS.File[fx].hFile,0,NULL,FILE_END) ;	/* If append then position to EOF */
	   break ;
	 } ;
	if (retry >= _RETRYMAX) { if (errmsg != NULL) v_Msg(NULL,errmsg,"FileCreate",fns,GetLastError()) ; return(UNUSED) ; } ;
#undef _RETRYMAX
#else
	if ((vS.File[fx].fp = (append ? UCfopen(tfilename,"a") : UCfopen(tfilename,"w"))) == NULL)
	 { if (errmsg != NULL)  v_Msg(NULL,errmsg,"@Could not write/append to file %1U (%2O)",fns,errno) ; return(UNUSED) ; } ;
#endif
	if (idpt != NULL) { memcpy(&vS.File[fx].id,idpt,idpt->Bytes) ; } else { memset(&vS.File[fx].id,0,V4DPI_PointHdr_Bytes) ; } ;
	UCstrcpy(vS.File[fx].FileName,tfilename) ;
	vS.File[fx].FileType = VOUT_FileType_File ;
	vS.File[fx].BytesRemain = VOUT_StreamBufferMax ;
	vS.File[fx].BytesAlloc = VOUT_StreamBufferMax ;
	vS.File[fx].Buffer = (char *)v4mm_AllocChunk(VOUT_StreamBufferMax,FALSE) ;
	vS.File[fx].ptr = vS.File[fx].Buffer ;
	vS.File[fx].TextType = V4L_TextFileType_ASCII ;		/* Set to ASCII until we get past possible type headers directly below */
	vS.File[fx].charCount = 0 ;
//	vS.FileCnt ++ ; 
	if (fx >= vS.highX) vS.highX++ ;
	vS.File[fx].FileId = ++vS.LUFileId ;
	if (!append)			/* If creating new file then maybe preface with appropriate header */
	 { switch (texttype)
	    { case V4L_TextFileType_ASCII:	break ;		/* Don't need anything */
	      case V4L_TextFileType_UTF8nh:	break ;		/* Don't want anything */
	      case V4L_TextFileType_UTF16be:	vout_TextFileX(fx,2,"\xfe\xff") ; break ;
	      case V4L_TextFileType_UTF16le:	vout_TextFileX(fx,2,"\xff\xfe") ; break ;
	      case V4L_TextFileType_UTF8:	vout_TextFileX(fx,3,"\xef\xbb\xbf") ; break ;
	      case V4L_TextFileType_UTF32be:	vout_TextFileX(fx,4,"\0\0\xff\xff") ; break ;
	      case V4L_TextFileType_UTF32le:	vout_TextFileX(fx,4,"\xff\xff\0\0") ; break ;
	    } ;
	 } ;
	vS.File[fx].TextType = texttype ;	/* Set this at the end so the above headers don't get interpreted! */
	return(vS.File[fx].FileId) ;
}

/*	vout_OpenStreamBigText - Opens up (another) BigText area for linkage to V4 output stream	*/
/*	Call: ok = vout_OpenStreamBigText( idpt , errmsg )
/*	Call: fileid = vout_OpenStreamBigText( idpt , bindpt , errmsg )
	  where fileid is unique file identifier, UNUSED if problems,
		bindpt is point to be bound to bigtext on close,
		errmsg, if not NULL, updated with any problems				*/
		 
int vout_OpenStreamBigText(idpt,bindpt,errmsg)
  struct V4DPI__LittlePoint *idpt ;
  struct V4DPI__Point *bindpt ;
  UCCHAR *errmsg ;
{ 
  int fx ;

	if (idpt != NULL)
	 { for(fx=0;fx<vS.highX;fx++)
	    { if (vS.File[fx].FileId <= 0 || memcmp(&vS.File[fx].id,idpt,idpt->Bytes) != 0) continue ;
	      if (errmsg != NULL) v_Msg(NULL,errmsg,"OutputIdExists",idpt) ; return(UNUSED) ;
	    } ;
	 } ;
///	fx = vS.FileCnt ;
	for(fx=0;fx<vS.highX;fx++) { if (vS.File[fx].FileId <= 0) break ; } ;
	if (fx >= VOUT_FileMax)
	 { if (errmsg != NULL) v_Msg(NULL,errmsg,"@Exceed maximum of %1d output files",VOUT_FileMax) ; return(UNUSED) ; } ;
	vSInitNewFile(fx) ;
	if (idpt != NULL) { memcpy(&vS.File[fx].id,idpt,idpt->Bytes) ; } else { memset(&vS.File[fx].id,0,V4DPI_PointHdr_Bytes) ; } ;
	vS.File[fx].FileType = VOUT_FileType_BigText ;
	vS.File[fx].BytesRemain = V4LEX_BigText_Max ;
	vS.File[fx].BytesAlloc = V4LEX_BigText_Max ;
	vS.File[fx].Buffer = (char *)v4mm_AllocChunk(V4LEX_BigText_Max,FALSE) ;
	vS.File[fx].BPoint = (P *)v4mm_AllocChunk(bindpt->Bytes,FALSE) ; memcpy(vS.File[fx].BPoint,bindpt,bindpt->Bytes) ;
	vS.File[fx].ptr = vS.File[fx].Buffer ;
	vS.File[fx].charCount = 0 ;
	vS.File[fx].bt = (struct V4LEX__BigText *)v4mm_AllocChunk(sizeof *vS.File[fx].bt,FALSE) ;
	 vS.File[fx].bt->Bytes = 0 ; vS.File[fx].bt->BigBuf[0] = UCEOS ;
//	vS.FileCnt ++ ; 
	if (fx >= vS.highX) vS.highX++ ;
	vS.File[fx].FileId = ++vS.LUFileId ;
	return(vS.File[fx].FileId) ;
}

/*	vout_OpenStreamBuffer - Opens up (another) stream where all output is internally buffered (i.e. not output anywhere)	*/
/*	Call: ok = vout_OpenStreamBuffer( idpt , errmsg , isJSON)
/*	Call: fileid = vout_OpenStreamBuffer( idpt , errmsg )
	  where fileid is unique file identifier, UNUSED if problems,
		idpt is optional stream id (NULL if none),
		errmsg, if not NULL, updated with any problems,
		isJSON is TRUE if buffer JSON, FALSE for regular text								*/
		 
int vout_OpenStreamBuffer(idpt,errmsg,isJSON)
  struct V4DPI__LittlePoint *idpt ;
  UCCHAR *errmsg ;
  LOGICAL isJSON ;
{ 
  int fx ;

	if (idpt != NULL)
	 { for(fx=0;fx<vS.highX;fx++)
	    { if (vS.File[fx].FileId <= 0 || memcmp(&vS.File[fx].id,idpt,idpt->Bytes) != 0) continue ;
	      if (errmsg != NULL) v_Msg(NULL,errmsg,"OutputIdExists",idpt) ; return(UNUSED) ;
	    } ;
	 } ;
	for(fx=0;fx<vS.highX;fx++) { if (vS.File[fx].FileId <= 0) break ; } ;
	if (fx >= VOUT_FileMax)
	 { if (errmsg != NULL) v_Msg(NULL,errmsg,"@Exceed maximum of %1d output files",VOUT_FileMax) ; return(UNUSED) ; } ;
	vSInitNewFile(fx) ;
	if (idpt != NULL) { memcpy(&vS.File[fx].id,idpt,idpt->Bytes) ; } else { memset(&vS.File[fx].id,0,V4DPI_PointHdr_Bytes) ; } ;
	vS.File[fx].FileType = (isJSON ? VOUT_FileType_BufJSON : VOUT_FileType_Buffer) ;
	vS.File[fx].BytesRemain = VOUT_FileType_Buffer_Max ;
	vS.File[fx].BytesAlloc = VOUT_FileType_Buffer_Max ;
	vS.File[fx].uBuffer = v4mm_AllocUC(VOUT_FileType_Buffer_Max) ;
	vS.File[fx].uptr = vS.File[fx].uBuffer ;
	vS.File[fx].charCount = 0 ;
	*vS.File[fx].uptr = UCEOS ;
	if (fx >= vS.highX) vS.highX++ ;
	vS.File[fx].FileId = ++vS.LUFileId ;
	return(vS.File[fx].FileId) ;
}

COUNTER vout_GetStreamBuffer(fileid,stream,charCount,outbuf,errmsg)
  FILEID fileid ;
  VSTREAM stream ;
  COUNTER charCount ;
  UCCHAR *outbuf,*errmsg ;
{
  COUNTER maxChars,outChars,usedChars,lineLen ; INDEX fx ;
  UCCHAR *ip, *ep, *op ;

	if (fileid == UNUSED)
	 { if (stream == UNUSED) { if (errmsg != NULL) v_Msg(NULL,errmsg,"@No fileId or Stream given") ; return(UNUSED) ; }
	    else { fx = vS.Stream[stream].FileX ; } ;
	 } else
	 { for (fx=0;fx<vS.highX;fx++) { if (vS.File[fx].FileId == fileid) break ; } ;
	   if (fx >= vS.highX) { if (errmsg != NULL) v_Msg(NULL,errmsg,"@No such file with specified ID") ; return(FALSE) ; } ;
	 } ;
	maxChars = (charCount < 0 ? -charCount : charCount) ;
	outChars = 0 ; op = outbuf ; ZUS(op)
	ip = vS.File[fx].uBuffer ;
	for(;;)
	 { ep = UCstrstr(ip,UCTXTEOL) ;
	   if (ep == NULL) break ;
	   lineLen = (ep - ip) ;
	   if (lineLen + 1 <= maxChars - outChars)
	    { UCstrncpy(op,ip,lineLen) ; op += lineLen ; *(op++) = UCEOLbt ; outChars += (lineLen + 1) ;
	      ip += lineLen + 2 ;
	      continue ;
	    } ;
/*	   Current line will exceed maximum allowed */
	   if (charCount < 0) break ;		/* If breaking by line then quit now */
	   break ;
	 } ;
/*	Now collapse the output buffer to remove what we have returned */
	usedChars = (ip - vS.File[fx].uBuffer) ;
 	vS.File[fx].BytesRemain += usedChars ; vS.File[fx].uptr -= usedChars ;
	memcpy(vS.File[fx].uBuffer,ip,(vS.File[fx].BytesAlloc - vS.File[fx].BytesRemain)*sizeof(UCCHAR)) ;
	ZUS(op) ; ZUS(vS.File[fx].uptr) ;
	return(outChars) ;
}

/*	vout_GetOutputBuffer - Returns pointer to output buffer for stream	*/
/*	Call: uptr = vout_GetOutputBuffer( fileId, stream, errmsg )
	  where uptr is pointer to output buffer or NULL if error,
		fileId is UNUSED or fileId,
		stream is UNUSED or stream,
		errmsg is updated with error message on error			*/
UCCHAR *vout_GetOutputBuffer(fileId, stream, errmsg )
  FILEID fileId ;
  VSTREAM stream ;
  UCCHAR *errmsg ;
{ INDEX fx ;

	if (fileId == UNUSED)
	 { if (stream == UNUSED) { if (errmsg != NULL) v_Msg(NULL,errmsg,"@No fileId or Stream given") ; return(NULL) ; }
	    else { fx = vS.Stream[stream].FileX ; } ;
	 } else
	 { for (fx=0;fx<vS.highX;fx++) { if (vS.File[fx].FileId == fileId) break ; } ;
	   if (fx >= vS.highX) { if (errmsg != NULL) v_Msg(NULL,errmsg,"@No such file with specified ID") ; return(NULL) ; } ;
	 } ;
	if (vS.File[fx].uBuffer == NULL) v_Msg(NULL,errmsg,"@Stream does not have buffered output") ;
	return(vS.File[fx].uBuffer) ;
} ;


/*	vout_CountForFile - Returns Count of Characters/Lines Output to File/Stream	*/
/*	Call: count = vout_CountForFile( fileid , stream , errmsg )
	  where count is number of lines, UNUSED if error,
		fileid is identifying point for file,
		stream is output stream (if filedid & stream are UNUSED then close all open files),
		wantLines is TRUE for line count, FALSE for character count
		errmsg, if not NULL is updated with errors		*/

COUNTER vout_CountForFile(fileid,stream,wantLines,errmsg)
  FILEID fileid ;
  VSTREAM stream ;
  UCCHAR *errmsg ;
{ INDEX fx ;

	if (fileid == UNUSED)
	 { if (stream == UNUSED) { if (errmsg != NULL) v_Msg(NULL,errmsg,"@No fileId or Stream given") ; return(UNUSED) ; }
	    else { fx = vS.Stream[stream].FileX ; } ;
	 } else
	 { for (fx=0;fx<vS.highX;fx++) { if (vS.File[fx].FileId == fileid) break ; } ;
	   if (fx >= vS.highX) { if (errmsg != NULL) v_Msg(NULL,errmsg,"@No such file with specified ID") ; return(FALSE) ; } ;
	 } ;
	return(wantLines ? vS.File[fx].LineCount : vS.File[fx].charCount) ;
}

/*	vout_CloseFile - Close a file & delink any streams associated	*/
/*	Call: ok = vout_CloseFile( fileid , stream , errmsg )
	  where ok is TRUE if ok, FALSE if error,
		fileid is identifying point for file,
		stream is output stream (if filedid & stream are UNUSED then close all open files),
		errmsg, if not NULL is updated with errors		*/

LOGICAL vout_CloseFile(fileid,stream,errmsg)
  FILEID fileid ;
  VSTREAM stream ;
  UCCHAR *errmsg ;
{
  struct V4DPI__Point valpt ;
  struct V4DPI__Binding bindpt ;
#ifdef WINNT
  int iptr ;
#endif
  INDEX fx,fxstart,fxend,sx ; COUNTER hangCount ;

	hangCount = 0 ;
	if (fileid == UNUSED)
	 { if (stream == UNUSED) { fxstart = 0 ; fxend = vS.highX ; hangCount = 5 ; }
	    else { fx = vS.Stream[stream].FileX ; fxstart = fx ; fxend = fx+1 ; } ;
	 } else
	 { for (fx=0;fx<vS.highX;fx++) { if (vS.File[fx].FileId == fileid) break ; } ;
	   if (fx >= vS.highX) { if (errmsg != NULL) v_Msg(NULL,errmsg,"@No such file with specified ID") ; return(FALSE) ; } ;
	   fxstart = fx ; fxend = fx+1 ;
	 } ;
	 
	for(fx=fxstart;fx<fxend;fx++)
	 { 
	   if (vS.File[fx].FileId <= 0) continue ;
	   if (vS.File[fx].threadOpen && hangCount > 0)
	    { printf("V4P- Output stream still open by a thread, waiting for it to finish\n") ;
	      HANGLOOSE(500) ; fx-- ; hangCount-- ;
	      if (hangCount <= 0) printf("V4E- V4 process terminating with running threads\n") ;
	      continue ;
	    } ;
	   switch (vS.File[fx].FileType)
	    { 
	      case VOUT_FileType_Console:
	      case VOUT_FileType_None:
	      case VOUT_FileType_StdOut:	break ;
	      case VOUT_FileType_Buffer:
	      case VOUT_FileType_BufJSON:
		v4mm_FreeChunk(vS.File[fx].uBuffer) ; vS.File[fx].uBuffer = NULL ;
		break ;
	      case VOUT_FileType_BigText:
		if (vS.File[fx].BytesRemain <= 1 || vS.File[fx].bt == NULL)
		 { if (errmsg != NULL) v_Msg(NULL,errmsg,"OutputCloseBT",vS.File[fx].BPoint,V4LEX_BigText_Max) ; return(FALSE) ; } ;
		if (!v4dpi_SaveBigTextPoint(gpi->ctx,vS.File[fx].bt,vS.File[fx].bt->Bytes,&valpt,UNUSED,TRUE)) return(FALSE) ;
		if (!v4dpi_BindListMake(&bindpt,&valpt,vS.File[fx].BPoint,gpi->ctx,NULL,NOWGTADJUST,0,DFLTRELH)) return(FALSE) ;
		v4mm_FreeChunk(vS.File[fx].bt) ; v4mm_FreeChunk(vS.File[fx].Buffer) ; vS.File[fx].Buffer = NULL ;
		break ;
	      case VOUT_FileType_File:
#ifdef WINNT
		WriteFile(vS.File[fx].hFile,vS.File[fx].Buffer,vS.File[fx].BytesAlloc-vS.File[fx].BytesRemain,&iptr,NULL) ;
		CloseHandle(vS.File[fx].hFile) ;
#else
		fwrite(vS.File[fx].Buffer,vS.File[fx].BytesAlloc-vS.File[fx].BytesRemain,1,vS.File[fx].fp) ;
		fclose(vS.File[fx].fp) ;
#ifdef UNIX
		if (vS.File[fx].isCommandFile) chmod(vS.File[fx].FileName,S_IRWXU+S_IRWXG) ;	/* Maybe change ownership to permit execution of file */
#endif
#endif
		v4mm_FreeChunk(vS.File[fx].Buffer) ; vS.File[fx].Buffer = NULL ;
		break ;
	      case VOUT_FileType_Socket:	break ;
	    } ;
	   vS.File[fx].FileId = UNUSED ; vS.File[fx].FileType = UNUSED ;
/*	   Now find any streams linked to this and assign to file #0 (stdout) */
	   for(sx=0;sx<VOUT_StreamMax;sx++)
	    { if (vS.Stream[sx].FileX == fx) vS.Stream[sx].FileX = 0 ; } ;
	   if (fx == vS.highX-1) vS.highX-- ;
	 } ;
	return(TRUE) ;
}

/*	vout_BindStreamFile - Binds file to a V4 output stream		*/
/*	Call: ok = vout_BindStreamFile( fileid , filetype , stream , errmsg)
	  where ok is TRUE if OK, FALSE if problems,
		fileid is id to file,
		filetype is file type (see VOUT_FileType_xxx) (NOTE: fileid or filetype required) (UNUSED to unbind file/stream)
		stream is stream index (e.g. VOUT_Data),
		errmsg, if not NULL, is updated with any errors		*/

LOGICAL vout_BindStreamFile(fileid,filetype,stream,errmsg)
  FILEID fileid ;
  ETYPE filetype ;
  VSTREAM stream ;
  UCCHAR *errmsg ;
{ INDEX fx ;

	if (fileid == UNUSED)
	 { if (filetype == UNUSED) { return(FALSE) ; }
	    else { for (fx=0;fx<vS.highX;fx++) { if (vS.File[fx].FileType == filetype) break ; } ;
		   if (fx >= vS.highX) { if (errmsg != NULL) v_Msg(NULL,errmsg,"@No such file with specified FileType") ; return(FALSE) ; } ;
	         } ;
	 } else
	 { for (fx=0;fx<vS.highX;fx++) { if (vS.File[fx].FileId == fileid) break ; } ;
	   if (fx >= vS.highX) { if (errmsg != NULL) v_Msg(NULL,errmsg,"@No such file with specified ID") ; return(FALSE) ; } ;
	 } ;
	vS.Stream[stream].FileX = fx ;
	return(TRUE) ;
}

/*	vout_rriSet - Associates rri (Rpt()) with stream output */
void vout_rriSet(filex,rri)
  int filex ;
  struct V4RPT__RptInfo *rri ;
{
	if (filex == UNUSED) filex = vout_StreamToFileX(VOUT_Data) ;
	vS.File[filex].rri = rri ;
}

/*	vout_rriGet - Returns rri (Rpt()) associated with filex (NULL if none) */
struct V4RPT__RptInfo *vout_rriGet(filex)
  int filex ;
{
	if (filex == UNUSED) filex = vout_StreamToFileX(VOUT_Data) ;
	return(vS.File[filex].rri) ;
}

/*	vout_rriClose - Informs outputs that rri is closing, look for any associations and break if found */
void vout_rriClose(rri)
  struct V4RPT__RptInfo *rri ;
{ INDEX i ;

	for(i=0;i<VOUT_FileMax;i++)
	 { if (vS.File[i].rri == rri) vS.File[i].rri = NULL ;
	 } ;
}


void vout_CloseAjaxStream(fileid)
{ int filex ; UCCHAR *tbuf,*b,*e ;

	filex = vout_FileIdToFileX(fileid) ;
/*	Go through and convert all UCEOS characters to "," (except for last one) */
	for(b=vS.File[filex].uBuffer;*b!=UCEOS;b=e+1)
	 { e = UCstrchr(b,'\0') ;
	  if (*(e+1) != UCEOS) *e = UClit(',') ;
	 } ;
/*	Now create final AJAX output string */
	tbuf = v4mm_AllocUC(UCstrlen(vS.File[filex].uBuffer) + strlen(gpi->patternAJAXJSON) + 1000) ;
/*	If the output buffer is empty then the resulting AJAX return is probably going to fail. Have to inject something! */
//VEH101025 - Don't use v_Msg, it will fail if uBuffer contains very long string
//	v_Msg(NULL,tbuf,gpi->patternAJAXJSON,(UCempty(vS.File[filex].uBuffer) ? UClit("'noResult':0") : vS.File[filex].uBuffer)) ;
	
	
#ifdef USEOLDAJAXSUB	
	ib=gpi->patternAJAXJSON ;
	if (*ib == '@') ib++ ;		/* Skip over leading '@' */
	{ UCCHAR *ob ; char *ib ;
	  for(ob=tbuf;*ib!='\0';ib++)
	   { switch(*(ib))
	      { default:		*(ob++) = *ib ; break ;
	        case '%':		if (*(ib+1) == '1' && *(ib+2) == 'U') { for(b=(UCempty(vS.File[filex].uBuffer) ? UClit("\"noResult\":0") : vS.File[filex].uBuffer);*b!=UCEOS;b++) { *(ob++) = *b ; } ; ib+=2 ; }
					 else { printf("%%Invalid AJAXJSON pattern: '%s'\n",gpi->patternAJAXJSON) ; } ;
					break ;
	      } ;
	   } ;
	}
	*(ob++) = UCEOS ;
#else
	vjson_SubParamsInJSON(tbuf,UCstrlen(vS.File[filex].uBuffer) + strlen(gpi->patternAJAXJSON) + 1000,gpi->patternAJAXJSON,(UCempty(vS.File[filex].uBuffer) ? UClit("\"noResult\":0") : vS.File[filex].uBuffer),NULL) ;
#endif
	vout_UCText(VOUT_Data,0,tbuf) ;
	v4mm_FreeChunk(tbuf) ;
}

void vout_CloseDeferStream(fileid)
{ int filex ; UCCHAR *b,*e ;

	filex = vout_FileIdToFileX(fileid) ;
	for(b=vS.File[filex].uBuffer;*b!=UCEOS;b=e+1)
	 { e = UCstrchr(b,'\0') ; if (*(e+1) != UCEOS) *e = UCEOS ;
	   vout_UCText(VOUT_Data,0,b) ;
	 } ;
}

/*	vout_Text - Outputs text string to whatever			*/
/*	Call: ok = vout_Text( stream , bytes , buf )
	  where ok is TRUE if output ok,
		stream is VOUT_xxx - which stream to get output
		bytes is number of bytes, 0 if buf is ASCIZ,
		buf is output buffer					*/

LOGICAL vout_Text( stream, bytes, buf)
  int stream, bytes ;
  char *buf ;
{ 
  struct tm *stm ;
  time_t timer ;
  char timebuf[32] ;

	if (buf[0] == '*')
	 { switch(stream)
	    { case VOUT_Status:		buf++ ; vout_TextFileX(vS.Stream[stream].FileX,0,"V4S- ") ; break ;
	      case VOUT_Debug:		buf++ ; vout_TextFileX(vS.Stream[stream].FileX,0,"V4D- ") ; break ;
	      case VOUT_Err:		buf++ ; vout_TextFileX(vS.Stream[stream].FileX,0,"V4E- ") ; break ;
	      case VOUT_Trace:		buf++ ; vout_TextFileX(vS.Stream[stream].FileX,0,"V4T- ") ; break ;
	      case VOUT_Warn:		buf++ ; vout_TextFileX(vS.Stream[stream].FileX,0,"V4W- ") ; break ;
	      case VOUT_Progress:	buf++ ; vout_TextFileX(vS.Stream[stream].FileX,0,"V4P- ") ; break ;
	    } ;
	   if (traceGlobal & V4TRACE_TimeStamp)
	    { time(&timer) ; stm = localtime(&timer) ; sprintf(timebuf,"%02d:%02d:%02d %.3f %d: ",stm->tm_hour,stm->tm_min,stm->tm_sec,v_CPUTime(),(gpi->ctx ? gpi->ctx->IsctEvalCount : 0)) ;
	      vout_TextFileX(vS.Stream[stream].FileX,0,timebuf) ;
	    } ;
	 } ;
	vout_TextFileX(vS.Stream[stream].FileX,bytes,buf) ;
	return(TRUE) ;
}

/*	vout_UCText - Outputs text string to whatever			*/
/*	Call: ok = vout_UCText( stream , bytes , buf )
	  where ok is TRUE if output ok,
		stream is VOUT_xxx - which stream to get output
		bytes is number of bytes, 0 if buf is ASCIZ,
		buf is output buffer					*/

LOGICAL vout_UCText( stream, bytes, buf)
  int stream, bytes ;
  UCCHAR *buf ;
{ 
  struct tm *stm ;
  time_t timer ;
  UCCHAR timebuf[32] ;


	if (buf[0] == UClit('*'))
	 { switch(stream)
	    { case VOUT_Status:		buf++ ; vout_UCTextFileX(vS.Stream[stream].FileX,0,UClit("V4S- ")) ; break ;
	      case VOUT_Debug:		buf++ ; vout_UCTextFileX(vS.Stream[stream].FileX,0,UClit("V4D- ")) ; break ;
	      case VOUT_Err:		buf++ ; vout_UCTextFileX(vS.Stream[stream].FileX,0,UClit("V4E- ")) ; break ;
	      case VOUT_Trace:		buf++ ; vout_UCTextFileX(vS.Stream[stream].FileX,0,UClit("V4T- ")) ; break ;
	      case VOUT_Warn:		buf++ ; vout_UCTextFileX(vS.Stream[stream].FileX,0,UClit("V4W- ")) ; break ;
	      case VOUT_Progress:	buf++ ; vout_UCTextFileX(vS.Stream[stream].FileX,0,UClit("V4P- ")) ; break ;
	    } ;
	   if (traceGlobal & V4TRACE_TimeStamp)
	    { time(&timer) ; stm = localtime(&timer) ; UCsprintf(timebuf,UCsizeof(timebuf),UClit("%02d:%02d:%02d %.3f %d: "),stm->tm_hour,stm->tm_min,stm->tm_sec,v_CPUTime(),(gpi->ctx ? gpi->ctx->IsctEvalCount : 0)) ;
	      vout_UCTextFileX(vS.Stream[stream].FileX,0,timebuf) ;
	    } ;
	 } ;
	vout_UCTextFileX(vS.Stream[stream].FileX,bytes,buf) ;
	return(TRUE) ;
}




/*	vout_NL - Outputs a newline */

void vout_NL(stream)
 int stream ;
{
	vout_NLFileX(vS.Stream[stream].FileX) ;
}

int vout_TotalBytes = 0 ;	/* Count of total bytes output */

/*	vout_TextFileX - Outputs text string to specific file index (as opposed to stream)	*/
/*	Call: ok = vout_TextFileX( filex , bytes , buf )
	  where ok is TRUE if output ok,
		filex is file index (UNUSED for default Data stream),
		bytes is number of bytes, 0 if buf is ASCIZ,
		buf is output buffer							*/

LOGICAL vout_TextFileX( filex, bytes, buf)
  int filex, bytes ;
  char *buf ;
{ int bbytes,i ;
#ifdef WINNT
  int iptr ;
#endif

#ifdef UNIXMT		/* If UNIX (Linux) then pass thru UC handler (can't mix/match byte & non-byte output to same stream) */
	return(vout_UCTextFileX(filex,bytes,ASCretUC(buf))) ;
#endif
	if (bytes == 0) bytes = strlen(buf) ;
	if (filex == UNUSED) filex = vout_StreamToFileX(VOUT_Data) ;
	vout_TotalBytes += bytes ; vS.File[filex].charCount += bytes ;
	switch(vS.File[filex].FileType)
	 {
	   case VOUT_FileType_Console:
	   case VOUT_FileType_StdOut:
/*		If we are Web debugging then send concole output out via V4 Message() */
		if (gpi->portDebug != 0)
		 { P *sendPt=NULL ; INTMODX intmodx=0 ; int ttl=UNUSED ; UCCHAR msgId[32],*ackBuf,spawnCmdLine[32] ; LOGICAL wantsReply=FALSE ; CALENDAR delCalDT=0 ; VTRACE trace=0 ;
		   ZUS(msgId) ; ZUS(spawnCmdLine) ; ackBuf = ASCretUC(buf) ;
		   v4im_MessageSendQueue(gpi->ctx,0,gpi->hostDebug,gpi->portDebug,ttl,sendPt,msgId,wantsReply,ackBuf,0,spawnCmdLine,delCalDT,trace) ;
/*		   And then continue to send output to real stdOut stream */
		 } ;
#if defined WINNT && defined V4UNICODE && !(defined V4_BUILD_TELNET_XFACE)
	   	if (buf[bytes] == UCEOS) { UCcputs(ASCretUC(buf)) ; break ; } ;
	   	for(i=0;i<bytes;i++) { UCfputc(buf[i],stdout) ; } ; break ;
#else
		fwrite(buf,bytes,1,stdout) ; break ;
#endif
	   case VOUT_FileType_None:		break ;
	   case VOUT_FileType_BigText:
		if (bytes + vS.File[filex].bt->Bytes >= UCsizeof(vS.File[0].bt->BigBuf)) return(FALSE) ;
		for(i=0;i<bytes;i++)
		 { vS.File[filex].bt->BigBuf[vS.File[filex].bt->Bytes++] = buf[i] ;
/*		   If EOL then skip next '\n', also replace '\r' with bigtext EOL character */
		   if (buf[i] == '\r' && buf[i+1] == '\n') { i++ ; vS.File[filex].bt->BigBuf[vS.File[filex].bt->Bytes-1] = UCEOLbt ; } ;
		 } ;
		break ;
	   case VOUT_FileType_Buffer:
		vS.File[filex].BytesRemain -= bytes ;
		if (vS.File[filex].BytesRemain <= 0)		/* Have to allocate more space ? */
		 { vS.File[filex].BytesAlloc += VOUT_FileType_Buffer_Max ;
		   vS.File[filex].BytesRemain += VOUT_FileType_Buffer_Max ;
		   vS.File[filex].uBuffer = (UCCHAR *)realloc(vS.File[filex].uBuffer,(sizeof (UCCHAR))*vS.File[filex].BytesAlloc) ;
		   vS.File[filex].uptr = &vS.File[filex].uBuffer[vS.File[filex].BytesAlloc - (vS.File[filex].BytesRemain + (bytes + 1))] ;
		 } ;
		for(i=0;i<bytes;i++) { *(vS.File[filex].uptr++) = buf[i] ; } ;
		*vS.File[filex].uptr = UCEOS ;
		break ;
	   case VOUT_FileType_BufJSON:
		vS.File[filex].BytesRemain -= (bytes + 1) ;
		if (vS.File[filex].BytesRemain <= 0)		/* Have to allocate more space ? */
		 { vS.File[filex].BytesAlloc += VOUT_FileType_Buffer_Max ;
		   vS.File[filex].BytesRemain += VOUT_FileType_Buffer_Max ;
		   vS.File[filex].uBuffer = (UCCHAR *)realloc(vS.File[filex].uBuffer,(sizeof (UCCHAR))*vS.File[filex].BytesAlloc) ;
		   vS.File[filex].uptr = &vS.File[filex].uBuffer[vS.File[filex].BytesAlloc - (vS.File[filex].BytesRemain + (bytes + 1))] ;
		 } ;
		for(i=0;i<bytes;i++) { *(vS.File[filex].uptr++) = buf[i] ; } ;
		*(vS.File[filex].uptr++) = UCEOS ;			/* End with double UCEOS if JSON buffer */
		*vS.File[filex].uptr = UCEOS ;
		break ;
	   case VOUT_FileType_File:
		switch (vS.File[filex].TextType)			/* Figure out how many buffer bytes we need */
		 { default:
		   case V4L_TextFileType_ASCII:
		   case V4L_TextFileType_UTF8nh:
		   case V4L_TextFileType_UTF8:		bbytes = bytes ; break ;
		   case V4L_TextFileType_UTF16be:
		   case V4L_TextFileType_UTF16le:	bbytes = 2 * bytes ; break ;
		   case V4L_TextFileType_UTF32be:
		   case V4L_TextFileType_UTF32le:	bbytes = 4 * bytes ; break ;
		 } ;
		if (bbytes > vS.File[filex].BytesRemain)	/* Have to flush buffer? */
		 {
#ifdef WINNT
		   WriteFile(vS.File[filex].hFile,vS.File[filex].Buffer,vS.File[filex].BytesAlloc-vS.File[filex].BytesRemain,&iptr,NULL) ;
#else
		   fwrite(vS.File[filex].Buffer,vS.File[filex].BytesAlloc-vS.File[filex].BytesRemain,1,vS.File[filex].fp) ;
#endif
		   vS.File[filex].BytesRemain = vS.File[filex].BytesAlloc ; vS.File[filex].ptr = vS.File[filex].Buffer ;
		   if (bbytes >= vS.File[filex].BytesAlloc)		/* What we got is bigger than buffer? */
		    {
#ifdef WINNT
		      WriteFile(vS.File[filex].hFile,buf,bbytes,&iptr,NULL) ;
#else
		      fwrite(buf,bbytes,1,vS.File[filex].fp) ;
#endif
		      break ;
		    } ;
		 } ;
		switch (vS.File[filex].TextType)
		 { case V4L_TextFileType_ASCII:
		   case V4L_TextFileType_UTF8nh:
		   case V4L_TextFileType_UTF8:
			memcpy(vS.File[filex].ptr,buf,bytes) ; vS.File[filex].ptr += bytes ; vS.File[filex].BytesRemain -= bytes ;
			break ;
		   case V4L_TextFileType_UTF16be:
			for(i=0;i<bytes;i++) { *(vS.File[filex].ptr++) = 0 ; *(vS.File[filex].ptr++) = buf[i] ; } ;
			vS.File[filex].BytesRemain -= bbytes ; break ;
		   case V4L_TextFileType_UTF16le:
			for(i=0;i<bytes;i++) { *(vS.File[filex].ptr++) = buf[i] ; *(vS.File[filex].ptr++) = 0 ; } ;
			vS.File[filex].BytesRemain -= bbytes ; break ;
		   case V4L_TextFileType_UTF32be:
		   case V4L_TextFileType_UTF32le:
			break ;			/* Not yet (ever?) supported */
		 } ;
//		memcpy(vS.File[filex].ptr,buf,bytes) ; vS.File[filex].ptr += bytes ; vS.File[filex].BytesRemain -= bytes ;
		break ;
	   case VOUT_FileType_Socket:		break ;
	 } ;
	return(TRUE) ;
}

/*	vout_UCTextFileX - Outputs Unicode string to specific file index (as opposed to stream)	*/
/*	Call: ok = vout_UCTextFileX( filex , bytes , buf )
	  where ok is TRUE if output ok,
		filex is file index (UNUSED for default Data stream),
		bytes is number of bytes, 0 if buf is ASCIZ,
		buf is output buffer							*/

LOGICAL vout_UCTextFileX( filex, bytes, buf)
  int filex, bytes ;
  UCCHAR *buf ;
{ int bbytes,i ;
  static char *utf8 = NULL ; static LENMAX utf8Max = VOUT_FileType_Buffer_Max ;
  UCCHAR tbuf[UCReadLine_MaxLine] ;
#ifdef WINNT
  int iptr ;
  static int isAConsole = UNUSED ;
#endif
  
	if (filex == UNUSED) filex = vout_StreamToFileX(VOUT_Data) ;
	if (filex < 0 || filex >= VOUT_FileMax)
	 { printf("????? filex(%d) out of range - resetting to UNUSED\n",filex) ; filex = UNUSED ; } ;
	switch(vS.File[filex].FileType)
	 { default:
		printf("???????????Invalid FileType(%d) for filex(%d) - defaulting to console\n",vS.File[filex].FileType,filex) ;
	   case UNUSED:				/* Default to closed streams - console/stdout */
	   case VOUT_FileType_Console:
	   case VOUT_FileType_StdOut:
//*		If we are Web debugging then send concole output out via V4 Message() */
		if (gpi->portDebug != 0)
		 { P *sendPt=NULL ; INTMODX intmodx=0 ; int ttl=UNUSED ; UCCHAR msgId[32],spawnCmdLine[32] ; LOGICAL wantsReply=FALSE ; CALENDAR delCalDT=0 ; VTRACE trace=0 ;
		   UCCHAR ackBuf[VOUT_FileType_Buffer_Max] ;
		   ZUS(msgId) ; ZUS(spawnCmdLine) ;
		   UCstrcpy(ackBuf,buf) ;

/*		   After this continue with 'normal' stdOut output */
		   v4im_MessageSendQueue(gpi->ctx,0,gpi->hostDebug,gpi->portDebug,ttl,sendPt,msgId,wantsReply,ackBuf,0,spawnCmdLine,delCalDT,trace) ;
		 } ;
//#define _cputws(str) fputws(str,stdout)
#ifdef WINNT
	   	if (isAConsole == UNUSED) { isAConsole = v_IsAConsole(stdout) ; } ;
	   	if (!isAConsole)		/* If stdout is not console then don't force to "console" */
	   	 { if (bytes == 0) { UCfputs(buf,stdout) ; bytes = UCstrlen(buf) ; break ; } ;
		   for(i=0;i<bytes;i++) { fputwc(buf[i],stdout) ; } ;
		   break ;
	   	 } ;
#if defined V4_BUILD_TELNET_XFACE && defined V4UNICODE
	   	if (bytes == 0) bytes = UCstrlen(buf) ;
	   	for(i=0;i<bytes;i++) { fputc((char )buf[i],stdout) ; } ;
#else
	   	if (bytes == 0)
		 { int res = UCcputs(buf) ;
		   bytes = UCstrlen(buf) ;
/*		   If this fails then buffer too big (Windows problem?). Have to split & output in pieces */
		   if (res != 0)
		    { int bx ; UCCHAR sbuf[512] ;
		      for(bx=0;bx<bytes;bx+=(UCsizeof(sbuf)-1))
		       { UCstrncpy(sbuf,&buf[bx],UCsizeof(sbuf)-1) ; sbuf[UCsizeof(sbuf)-1] = UCEOS ; UCcputs(sbuf) ; } ;
		    } ;
		   break ;
		 } ;
	   	if (buf[bytes] == UCEOS) { UCcputs(buf) ; break ; } ;
	   	for(i=0;i<bytes;i++) { UCfputc(buf[i],stdout) ; } ;
#endif
#else
	   	if (bytes == 0) { UCfputs(buf,stdout) ; bytes = UCstrlen(buf) ; break ; } ;
	   	if (buf[bytes] == UCEOS) { UCfputs(buf,stdout) ; break ; } ;
	   	for(i=0;i<bytes;i++) { UCfputc(buf[i],stdout) ; } ;
#endif
	   	break ;
	   case VOUT_FileType_None:		break ;
	   case VOUT_FileType_BigText:
		if (bytes == 0) bytes = UCstrlen(buf) ;
		if (bytes == 0) break ;
		if (bytes + vS.File[filex].bt->Bytes >= UCsizeof(vS.File[filex].bt->BigBuf))
		 { v_Msg(gpi->ctx,gpi->ctx->ErrorMsgAux,"OutputCloseBT",vS.File[filex].BPoint,UCsizeof(vS.File[filex].bt->BigBuf)) ;
		   return(FALSE) ;
		 } ;
		for(i=0;i<bytes;i++)
		 { vS.File[filex].bt->BigBuf[vS.File[filex].bt->Bytes++] = buf[i] ;
/*		   If EOL then skip next '\n', also replace '\r' with bigtext EOL character */
		   if (buf[i] == '\r' && buf[i+1] == '\n') { i++ ; vS.File[filex].bt->BigBuf[vS.File[filex].bt->Bytes-1] = UCEOLbt ; } ;
		 } ;
		break ;
//		printf("??????????????????/ BigText output Unicode not supported\n") ; exit(EXITABORT) ;
//		if (bytes >= vS.File[filex].BytesRemain) return(FALSE) ;	/* Don't overflow buffer (will error on close) */
//		memcpy(vS.File[filex].ptr,buf,bytes) ; vS.File[filex].ptr += bytes ; vS.File[filex].BytesRemain -= bytes ;
//		break ;
	   case VOUT_FileType_Buffer:
		if (bytes == 0) bytes = UCstrlen(buf) ;
		if (bytes == 0) break ;
		vS.File[filex].BytesRemain -= bytes ;
		if (vS.File[filex].BytesRemain <= 0)		/* Have to allocate more space ? */
		 { vS.File[filex].BytesAlloc += (bytes > VOUT_FileType_Buffer_Max ? bytes : VOUT_FileType_Buffer_Max) ;
		   vS.File[filex].BytesRemain += (bytes > VOUT_FileType_Buffer_Max ? bytes : VOUT_FileType_Buffer_Max) ;
		   vS.File[filex].uBuffer = (UCCHAR *)realloc(vS.File[filex].uBuffer,(sizeof (UCCHAR))*vS.File[filex].BytesAlloc) ;
		   vS.File[filex].uptr = &vS.File[filex].uBuffer[vS.File[filex].charCount] ;
		 } ;
		for(i=0;i<bytes;i++) { *(vS.File[filex].uptr++) = buf[i] ; } ;
		*vS.File[filex].uptr = UCEOS ;
		break ;
	   case VOUT_FileType_BufJSON:
		if (bytes == 0) bytes = UCstrlen(buf) ;
		vS.File[filex].BytesRemain -= (bytes + 1) ;
		if (vS.File[filex].BytesRemain <= 0)		/* Have to allocate more space ? */
		 { vS.File[filex].BytesAlloc += VOUT_FileType_Buffer_Max ;
		   vS.File[filex].BytesRemain += VOUT_FileType_Buffer_Max ;
		   vS.File[filex].uBuffer = (UCCHAR *)realloc(vS.File[filex].uBuffer,(sizeof (UCCHAR))*vS.File[filex].BytesAlloc) ;
		   vS.File[filex].uptr = &vS.File[filex].uBuffer[vS.File[filex].BytesAlloc - (vS.File[filex].BytesRemain + (bytes + 1))] ;
		 } ;
		for(i=0;i<bytes;i++) { *(vS.File[filex].uptr++) = buf[i] ; } ;
		*(vS.File[filex].uptr++) = UCEOS ;		/* End with double UCEOS if JSON buffer */
		*vS.File[filex].uptr = UCEOS ;
		break ;
	   case VOUT_FileType_File:
		if (bytes == 0) bytes = UCstrlen(buf) ;
		if (bytes == 0) break ;
		switch (vS.File[filex].TextType)
		 { default:
		   case V4L_TextFileType_ASCII:
/*			The difference between ASCII & UTF8 is that we just take bytes as-is on ASCII (necessary for EchoS() handling) */
/*			If we converted to UTF8 then any bytes > 127 would get screwed up & xvrestobiff/html would fail */
			if (utf8 == NULL)
			 { if (bytes > utf8Max) utf8Max = bytes * 1.2 ;		/* Want to make sure our buffer is big enought to handle whatever is coming */
			   utf8 = (char *)v4mm_AllocChunk(utf8Max,FALSE) ;
			 } else
			 { if (bytes > utf8Max) utf8Max = bytes * 1.2 ;		/* Not big enough, make it bigger */
			   utf8 = realloc(utf8,utf8Max) ;
			 } ;
			for(i=0;i<bytes;i++) { utf8[i] = buf[i] ; } ;
			buf = (UCCHAR *)utf8 ; bbytes = bytes ;
			break ;
#ifdef V4UNICODE /* If not Unicode then take default path above and treat as if ASCII */
		   case V4L_TextFileType_UTF8nh:
		   case V4L_TextFileType_UTF8:
			if (utf8 == NULL)
			 { if (bytes > utf8Max) utf8Max = bytes * 1.2 ;		/* Want to make sure our buffer is big enought to handle whatever is coming */
			   utf8 = (char *)v4mm_AllocChunk(utf8Max,FALSE) ;
			 } else
			 { if (bytes > utf8Max) utf8Max = bytes * 1.2 ;		/* Not big enough, make it bigger */
			   utf8 = realloc(utf8,utf8Max) ;
			 } ;
#ifdef ISLITTLEENDIAN
			bbytes = UTF16LEToUTF8(utf8,utf8Max,buf,bytes) ;
#else
			bbytes = UTF16BEToUTF8(utf8,utf8Max,buf,bytes) ;
#endif
			buf = (UCCHAR *)utf8 ;
			if (bbytes < 0)
			 { v_Msg(NULL,tbuf,"@vout_UCTextFile error - could not convert UC to UTF8...\n") ; vout_UCText(VOUT_Warn,0,tbuf) ;
			   buf = (UCCHAR *)"?UC->UTF8 error?" ; bbytes = strlen(utf8) ;
			 } ;
			break ;
#endif
		   case V4L_TextFileType_UTF16be:
#ifdef ISLITTLEENDIAN
			for (i=0;i<bytes;i++) { tbuf[i] = (buf[i] > 8) | ((buf[i] & 0xFF) < 8) ; } ;
			buf = tbuf ;
#endif
		   	bbytes = 2 * bytes ; break ;
		   case V4L_TextFileType_UTF16le:
#ifndef ISLITTLEENDIAN
			for (i=0;i<bytes;i++) { tbuf[i] = (buf[i] > 8) | ((buf[i] & 0xFF) < 8) ; } ;
			buf = tbuf ;
#endif
		   	bbytes = 2 * bytes ; break ;
		   case V4L_TextFileType_UTF32be:
		   case V4L_TextFileType_UTF32le:	bbytes = 4 * bytes ; break ;
		 } ;
		if (bbytes > vS.File[filex].BytesRemain)	/* Have to flush buffer? */
		 {
#ifdef WINNT
		   WriteFile(vS.File[filex].hFile,vS.File[filex].Buffer,vS.File[filex].BytesAlloc-vS.File[filex].BytesRemain,&iptr,NULL) ;
#else
		   fwrite(vS.File[filex].Buffer,vS.File[filex].BytesAlloc-vS.File[filex].BytesRemain,1,vS.File[filex].fp) ;
#endif
		   vS.File[filex].BytesRemain = vS.File[filex].BytesAlloc ; vS.File[filex].ptr = vS.File[filex].Buffer ;
		   if (bbytes >= vS.File[filex].BytesAlloc)		/* What we got is bigger than buffer? */
		    {
#ifdef WINNT
		      WriteFile(vS.File[filex].hFile,buf,bbytes,&iptr,NULL) ;
#else
		      fwrite(buf,bbytes,1,vS.File[filex].fp) ;
#endif
		      break ;
		    } ;
		 } ;
		switch (vS.File[filex].TextType)
		 { case V4L_TextFileType_ASCII:
		   case V4L_TextFileType_UTF8nh:
		   case V4L_TextFileType_UTF8:
			memcpy(vS.File[filex].ptr,buf,bbytes) ;
			vS.File[filex].ptr += bbytes ; vS.File[filex].BytesRemain -= bbytes ;
			break ;
		   case V4L_TextFileType_UTF16be:
			for(i=0;i<bytes;i++) { *(vS.File[filex].ptr++) = buf[i] ; *(vS.File[filex].ptr++) = 0 ; } ;
			vS.File[filex].BytesRemain -= bbytes ; break ;
		   case V4L_TextFileType_UTF16le:
			for(i=0;i<bytes;i++) { *(vS.File[filex].ptr++) = buf[i] ; *(vS.File[filex].ptr++) = buf[i] >> 8 ; } ;
			vS.File[filex].BytesRemain -= bbytes ; break ;
		   case V4L_TextFileType_UTF32be:
		   case V4L_TextFileType_UTF32le:
			break ;			/* Not yet (ever?) supported */
		 } ;
//		memcpy(vS.File[filex].ptr,buf,bytes) ; vS.File[filex].ptr += bytes ; vS.File[filex].BytesRemain -= bytes ;
		break ;
	   case VOUT_FileType_Socket:		break ;
	 } ;
	vout_TotalBytes += bytes ; vS.File[filex].charCount += bytes ;
	return(TRUE) ;
}

/*	vout_UCTextFileXCont - Same as vout_UCTextFileX but 'continues' prior 'line'	*/
/*	 (i.e. does not separate prior line and current with UCEOS)			*/

LOGICAL vout_UCTextFileXCont(filex,bytes,buf)
  int filex, bytes ;
  UCCHAR *buf ;
{
	if (filex == UNUSED) filex = vout_StreamToFileX(VOUT_Data) ;
	switch(vS.File[filex].FileType)
	 {
	   default:	break ;			/* For most output types this does nothing special */
/*	   If buffered (ex: creating JSON output) then backup 1 character to eliminate UCEOS between 'lines' */
	   case VOUT_FileType_Buffer:
	   case VOUT_FileType_BufJSON:
	     if (*(vS.File[filex].uptr - 1) == UCEOS)
	      { vS.File[filex].BytesRemain += 1 ; vS.File[filex].uptr-- ; } ;
	     break ;
	 } ;
/*	Now really handle the output */
	if (bytes == UNUSED) return(TRUE) ;
	return(vout_UCTextFileX(filex,bytes,buf)) ;
}


#ifdef V4UNICODE
/*	 vout_NLFileX - Outputs a (UNICODE) newline to file pointed to by file index */

void vout_NLFileX(filex)
 int filex ;
{
	if (filex == UNUSED) filex = vout_StreamToFileX(VOUT_Data) ;
	vout_UCTextFileX(filex,0,UCTXTEOL) ;
	vS.File[filex].LineCount++ ;
}
#else
/*	vout_NLFileX - Outputs a (ASCII) newline to file pointed to by file index */

void vout_NLFileX(filex)
 int filex ;
{
	if (filex == UNUSED) filex = vout_StreamToFileX(VOUT_Data) ;

	vout_TextFileX(filex,0,TXTEOL) ;
	vS.File[filex].LineCount++ ;
}
#endif


/*	Call: v_ParseFileName(tcb , filebuf , dfltdev , dfltext , userext)
/*	  where tcb is token ctrl block,
		filebuf is updated with filename,
		dfltdev is default device name (NULL if none),
		dfltext is default extension (NULL if none),
		userext is updated (if not NULL) with T/F if user supplied extension	*/

void v_ParseFileName(tcb,filebuf,dfltdev,dfltext,userext)
  struct V4LEX__TknCtrlBlk *tcb ;
  UCCHAR *filebuf,*dfltdev,*dfltext ;
  int *userext ;
{ int gotext=FALSE,gotdev=FALSE,indir=FALSE ;
  UCCHAR *ip ;

	v4lex_NextTkn(tcb,V4LEX_Option_FileSpec) ;		/* Get file specification */
	for(ip=tcb->UCstring;*ip!='\0';ip++)
	 { switch(*ip)
	    { default:		break ;
#ifdef VMSOS
	      case UClit('['):		indir = TRUE ; break ;
	      case UClit(']'):		gotext = FALSE ; indir = FALSE ; break ;
#endif
#ifdef UNIX
	      case UClit('/'):		gotext = FALSE ;
#endif
#ifdef WINNT
	      case '\\':	gotext = FALSE ;
#endif
	      case UClit(':'):		gotdev = TRUE ; break ;
	      case UClit('.'):		if (!indir) gotext = TRUE ; break ;
	    } ;
	 } ;
	ZUS(filebuf) ;
	if (!gotdev && dfltdev != NULL) UCstrcat(filebuf,dfltdev) ;
	UCstrcat(filebuf,tcb->UCstring) ;
	if (!gotext && dfltext != NULL) UCstrcat(filebuf,dfltext) ;
	if (userext != NULL) *userext = gotext ;
}

/*	M E M O R Y   M A N A G E M E N T			*/

/*	v4mm_AllocChunk - Allocates a chunk of memory		*/
/*	Call: ptr = v4mm_AllocChunk( bytes , clearflag )
	  where ptr is pointer to memory chunk,
		bytes is number of bytes needed,
		clearflag is TRUE if all bytes s/b set to 0	*/

//#define MemDebug 1
struct V4MM__MemChain
{ struct V4MM__MemChain *Prior,*Next ;
  int Bytes ;
  char *Current ;
} ;
struct V4MM__MemChain *First=NULL,*Last=NULL ;

int totalmem,highwatermem ;

void *v4mm_AllocChunk(bytes,clearflag)
  int bytes,clearflag ;
{ 
#ifdef MemDebug
  struct V4MM__MemChain *Cur ; char **tail ;
#else
  char *p ; UCCHAR emsg[128] ;
#endif

#ifdef MemDebug
	Cur = (struct V4MM__MemChain *)malloc(ALIGN(bytes + sizeof(struct V4MM__MemChain)+sizeof tail)) ;
	if (clearflag) memset(Cur,0,ALIGN(bytes+sizeof(struct V4MM__MemChain)+sizeof tail)) ;
	if (First == NULL) First = Cur ;
	if (Last != NULL) Last->Next = Cur ;
	Cur->Prior = Last ; Cur->Next = NULL ;
	Last = Cur ;
	Cur->Bytes = bytes ; Cur->Current = (char *)Cur + sizeof(struct V4MM__MemChain) ;
	tail = Cur->Current + ALIGN(Cur->Bytes) ; *tail = Cur->Current ;
	totalmem += bytes ; if (totalmem > highwatermem) highwatermem = totalmem ;
	return(Cur->Current) ;
#else
if (bytes > 20000000 || bytes < 1)
 { v_Msg(NULL,emsg,"@*Allocating %1d bytes in v4mm_AllocChunk()\n",bytes) ; vout_UCText(VOUT_Warn,0,emsg) ;
   if (gpi != NULL ? gpi->ctx != NULL : FALSE)
    {
#ifdef NASTYBUG
      v_Msg(NULL,gpi->ctx->utbuf1,"@*Iscts = %1d, current intMod = %2U / %3P\n",gpi->ctx->IsctEvalCount,v4im_Display(0),gpi->ctx->isctIntMod) ;
#else
      v_Msg(NULL,gpi->ctx->utbuf1,"@*Iscts = %1d, current intMod = %2U\n",gpi->ctx->IsctEvalCount,v4im_Display(0)) ;
#endif
      vout_UCText(VOUT_Warn,0,gpi->ctx->utbuf1) ;
      if (bytes < 1) bytes = 1 ;
    } ;
 } ;

	p = malloc(bytes) ;
	if (p == NULL)
	 { v_Msg(NULL,emsg,"@*Could not malloc(%1d) - you are in a 'heap' of trouble!\n",bytes) ; vout_UCText(VOUT_Err,0,emsg) ;
	   v4_error(V4E_MEMALLOCFAIL,0,"VCOM","AllocChunk","MEMALLOCFAIL","Could not malloc requested memory") ;
	 } ;
	if (clearflag) memset(p,0,bytes) ;
	totalmem += bytes ; return(p) ;
#endif
}

UCCHAR *v4mm_AllocUC(chars)
  int chars ;
{ if (chars < 0 || chars > 0x100000)
   {  UCCHAR emsg[128] ; v_Msg(NULL,emsg,"@*Unreasonable request? v4mm_AllocUC(%1d)  *******************\n",chars) ; vout_UCText(VOUT_Warn,0,emsg) ;
      if (chars < 1) chars = 64 - 1  ;
   } ;
  return((UCCHAR *)v4mm_AllocChunk((chars+1)*sizeof(UCCHAR),FALSE)) ;
}

/*	v4mm_FreeChunk - Deallocates a chunk of memory		*/
/*	Call: result = v4mm_FreeChunk( memptr )
	  where result is >= 0 if ok,
		memptr is pointer to memory previously allocated */

void v4mm_FreeChunk(memptr)
  void *memptr ;
{
#ifdef MemDebug
  struct V4MM__MemChain *Cur ; char **tail ;
#endif

#ifdef MemDebug
	Cur = First ;			/* Loop thru chain looking for this memory element */
	for(;Cur!=NULL;)
	 { if (Cur->Current == memptr)
	    { if (Cur->Prior != NULL) Cur->Prior->Next = Cur->Next ;
	      if (Cur->Next != NULL) Cur->Next->Prior = Cur->Prior ;
	      if (First == Cur) First = Cur->Next ;
	      if (Last == Cur) Last = Cur->Prior ;
	      tail = Cur->Current + ALIGN(Cur->Bytes) ;
	      if(*tail != Cur->Current)
	       printf("Memory overrun p=%p, bytes=%d\n",memptr,Cur->Bytes) ;
	      totalmem -= Cur->Bytes ;
	      free(Cur) ;
	      break ;
	    } ;
	   Cur = Cur->Next ;
	 } ;
	if (Cur != NULL) return ;	/* All is still well */
	printf("Attempt to return memory not currently allocated\n") ;
	return ;
#else
	free(memptr) ;
	return ;
#endif
}

void v4mm_MemoryExit()
{ 
#ifdef MemDebug
  struct V4MM__MemChain *Cur ; char **tail ;
  int TotMem, Cnt ;
	Cur = First ; TotMem = 0 ;
	for(Cnt=1;Cur!=NULL;Cnt++)
	 { printf("Memory Leak = %p, %d bytes\n",Cur->Current,Cur->Bytes) ; TotMem += Cur->Bytes ;
	   tail = Cur->Current + ALIGN(Cur->Bytes) ;
	   if(*tail != Cur->Current)
	    printf("Memory overrun p=%p, bytes=%d\n",Cur->Current,Cur->Bytes) ;
	   Cur = Cur->Next ;
	 } ;
	if (Cnt != 0) printf("Total Leaks= %d, Total Bytes=%d\n",Cnt,TotMem) ;
	printf("High Water Memory Usage = %d bytes",highwatermem) ;
#endif
}

/*	Here to free (most) memory resources associated with process */
void v4mm_FreeResources(gpi)
  struct V4C__ProcessInfo *gpi ;
{ struct V4C__Context *ctx ;
  struct V4IM__LocalVHdr *lvh ;
  struct V4LEX__Table *vlt,*lvlt ;
  int i,j ;

	if (gpi->MainTcb != NULL) { v4mm_FreeChunk(gpi->MainTcb) ; gpi->MainTcb = NULL ; } ;
//	vout_SetOutToFile(VOUT_Data,NULL,NULL,NULL,NULL) ;	/* Maybe close of any output streams */
	vout_CloseFile(UNUSED,UNUSED,NULL) ;			/* Close off all/any output stream files */
	if (gpi->ctx != NULL)
	 { ctx = gpi->ctx ;
	   v4ctx_FrameCacheClear(ctx,TRUE) ; v4ctx_AreaClose(ctx,-1) ;
	   for(i=0;i<V4C_CtxVal_Max;i++)
	    { for(j=0;j<V4C_CtxVal_nblMax;j++) { if (ctx->CtxVal[i].nbl[j] != NULL) v4mm_FreeChunk(ctx->CtxVal[i].nbl[j]) ; } ; } ;
	   for(i=0;i<ctx->FnblCnt;i++) { v4mm_FreeChunk(ctx->fnbl[i]) ; } ;
	   for(i=0;i<V4C_CtxDimHash_Max;i++) { if (ctx->DimHash[i].di != NULL) v4mm_FreeChunk(ctx->DimHash[i].di) ; } ;
	   for(i=0;i<ctx->pi->AreaAggCount;i++) { if (ctx->pi->AreaAgg[i].vad != NULL) v4mm_FreeChunk(ctx->pi->AreaAgg[i].vad) ; } ;
	   v4dpi_IsctEvalCacheClear(ctx,TRUE) ; v4dpi_PntIdx_Shutdown() ; v4dpi_GetBindBuf(ctx,-2,NULL,FALSE,NULL) ;
	   v4mm_FreeChunk(ctx) ; gpi->ctx = NULL ;
	 } ;
	for (lvlt=NULL,vlt=gpi->vlt;vlt!=NULL;vlt=vlt->link) { if (vlt->IsDefault) { lvlt = vlt ; break ; } ; } ;
	if (lvlt != NULL)				/* Found DEFAULTS table? */
	 { for(i=0;i<lvlt->Columns;i++)
	    { for(vlt=gpi->vlt;vlt!=NULL;vlt=vlt->link)	/*  yes - then zap all defaults/errs in others */
	       { if (vlt->IsDefault) continue ;
		 for(j=0;j<vlt->Columns;j++)
		  { if (lvlt->Col[i].Dflt == vlt->Col[j].Dflt) vlt->Col[j].Dflt = NULL ;
		    if (lvlt->Col[i].Err == vlt->Col[j].Err) vlt->Col[j].Err = NULL ;
		  } ;
	       } ;
	    } ;
	 } ;
	for (vlt=gpi->vlt;vlt!= NULL;vlt=lvlt)
	 { for(i=0;i<vlt->Columns;i++)
	    { if (vlt->Col[i].Dflt != NULL) v4mm_FreeChunk(vlt->Col[i].Dflt) ;
	      if (vlt->Col[i].Err != NULL) v4mm_FreeChunk(vlt->Col[i].Err) ;
	      if (vlt->Col[i].Cur != NULL) v4mm_FreeChunk(vlt->Col[i].Cur) ;
	      if (vlt->Col[i].vtt != NULL && ((vlt->Col[i].ColumnType & V4LEX_TableCT_Kernel) == 0))
	       { if (vlt->Col[i].vtt->vtts != NULL) v4mm_FreeChunk(vlt->Col[i].vtt->vtts) ;
		 v4mm_FreeChunk(vlt->Col[i].vtt) ;
	       } ;
	    } ;
	   lvlt=vlt->link ; v4mm_FreeChunk(vlt) ;
	 } ;
	if (gpi->lvh != NULL)
	 { lvh = gpi->lvh ;
	   for(i=0;i<lvh->Count;i++)
	    { if (lvh->Entry[i].avh != NULL) v4mm_FreeChunk(lvh->Entry[i].avh) ;
	      for(j=0;j<V4IM_AggStreamMax;j++)
	       { if (lvh->Entry[i].Stream[j].aggs != NULL) v4mm_FreeChunk(lvh->Entry[i].Stream[j].aggs) ; } ;
	    } ;
	   v4mm_FreeChunk(lvh) ;
	 } ;
/*	Delete all known temp files (if still around) */
	for(;vctfl!=NULL;vctfl = vctfl->next) { UCremove(vctfl->tmpfile) ; } ;
/*	Shut down any/all remote database (mySQL) connections */
	if (gpi->xdb != NULL)
	 { INDEX cx ;
	   for(cx=0;cx<gpi->xdb->conCount;cx++)
	    { if (gpi->xdb->con[cx].xdbId == UNUSED) continue ;
	      v4xdb_FreeStuff(ctx,gpi->xdb->con[cx].xdbId,0) ;
	    } ;
	 } ;
	v4mm_MemMgmtShutdown(gpi->mmm) ;	/* Shut down main process mmm */
}

#ifdef UNIX
void sleepms(msecs)
  int msecs ;
{ struct itimerval set,old ;
  void sleepms_wakeup() ;

	memset(&set,0,sizeof set) ; set.it_value.tv_usec = msecs*5000 ;
	signal(SIGALRM,sleepms_wakeup) ;
	if (setitimer(ITIMER_REAL,&set,&old) == 0) pause() ;
}
void sleepms_wakeup()
{	return ;
}
#endif

#ifdef VMSOS
/*	V M S   R O U T I N E S			*/


/*	mscu_vms_errmsg - Converts VMS error code to ascii string */

mscu_vms_errmsg(vms_code,str)
  int vms_code ;			/* Standard vms error value */
  char *str ;				/* Updated with string equivalent */
 { struct {
      short int length,desc ;
      char *ptr ;
    } str_dsc ;				/* VMS string descriptor */
   short int res_len ;

	str_dsc.length = 250 ; str_dsc.desc = 0 ; str_dsc.ptr = str ;
	sys$getmsg(vms_code,&res_len,&str_dsc,1,0) ; *(str+res_len) = 0 ;
	return(str) ;
}
/*	vms_Sleepms - Sleeps for specified number of milliseconds       */
 
vms_Sleepms(msecs)
  int msecs ;
{ struct
   { int lh,rh ; } vmsdt ;
 
	vmsdt.lh = -msecs * 10000 ; vmsdt.rh = -1 ; sys$schdwk(0,0,&vmsdt,0) ;
	sys$hiber() ;
}
#endif

/*	H A N D L E   R O U T I N E S			*/


/*	han_PtrToMaster - Returns pointer to master entry	*/
/*	Call: ptr = han_PtrToMaster()				*/

struct han__Master *han_PtrToMaster()
{ static struct han__Master *hanMaster ;

	if (hanMaster == NULL) hanMaster = (struct han__Master *)v4mm_AllocChunk(sizeof(*hanMaster),TRUE) ;
	return(hanMaster) ;
}

/*	han_Make - Creates a new handle and returns Id		*/
/*	Call: id = han_Make()					*/

int han_Make()
{ struct han__Master *hm ;
  struct han__Group *hg ;
  struct han__Id *id ;
  int gx,ex ;

	hm = han_PtrToMaster() ;		/* Get master */
	for(gx=0;gx<hm->Count;gx++)	/* Search for free slot */
	 { if (hm->Group[gx] == NULL) break ;
	   hg = hm->Group[gx] ;
	   if (hg->Count < HANGroup_Max) break ;
	 } ;
	if (gx >= hm->Count)		/* Have to allocate a new group? */
	 { hg = (hm->Group[hm->Count] = (struct han__Group *)v4mm_AllocChunk(sizeof *hg,TRUE)) ;
	   for(ex=0;ex<HANGroup_Max;ex++) { hg->Entry[ex].HandleId = HAN_UNUSED ; } ;
	   gx = (hm->Count++) ;
	 } ;
	hg = hm->Group[gx] ;
	for (ex=0;ex<HANGroup_Max;ex++)
	 { if (hg->Entry[ex].HandleId == HAN_UNUSED) break ; } ;
	if (ex >= HANGroup_Max) return(HAN_UNUSED) ;	/* Could not find free entry? */
	memset(&hg->Entry[ex],0,sizeof hg->Entry[ex]) ;
	hg->Entry[ex].ParentHandleId = HAN_UNUSED ;
	hg->Entry[ex].PriorHandle = HAN_UNUSED ; hg->Entry[ex].NextHandle = HAN_UNUSED ;
	id = (struct han__Id *)&hg->Entry[ex].HandleId ;
	id->GroupX = gx ; id->EntryX = ex ;
	id->Id = (hm->NextId++) ;
	hg->Count++ ;
	return(hg->Entry[ex].HandleId) ;
}

///*	han_Close - Closes/Release a Handle & any children of parent	*/
///*	Call:	result = han_Close( handle )
//	  where result is TRUE if closed, FALSE if not (ex: bad handle)
//	  	handle is Id to be closed				*/
//
//int han_Close(handle)
//  int handle ;
//{ struct han__Master *hm ;
//  struct han__Group *hg ;
//  struct han__Id *id ;
//  int gx,ex ;
//
//	id = (struct han__Id *)&handle ;
//	hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
//	if (hg == NULL) return(FALSE) ;
//	if (hg->Entry[id->EntryX].HandleId != handle) return(FALSE) ;
//	if (hg->Entry[id->EntryX].Flags & HANFlag_HasChild)
//	 { for(gx=0;gx<255;gx++)		/* Look for all with parent of this guy */
//	    { if (hm->Group[gx] == NULL) continue ;
//	      for(ex=0;ex<HANGroup_Max;ex++)	/* Recursively close all children */
//	       { if (hm->Group[gx]->Entry[ex].ParentHandleId == handle)
//	          { han_Close(hm->Group[gx]->Entry[ex].HandleId) ;
//	            if (hm->Group[gx] == NULL)	/* Was this group deallocated? */
//	             break ;
//	          } ;
//	       } ;
//	    } ;
//	 } ;
//	hg->Entry[id->EntryX].HandleId = HAN_UNUSED ;
//	if ((hg->Entry[id->EntryX].Flags & HANFlag_FreeMem) != 0)
//	 { if (hg->Entry[id->EntryX].P1 != NULL) v4mm_FreeChunk(hg->Entry[id->EntryX].P1) ;
//	 } ;
//	hg->Count-- ;
//	if (hg->Count <= 0)			/* If no more in this group then deallocate it */
//	 { v4mm_FreeChunk(hg) ;
//	   hm->Count-- ;
//	   hm->Group[id->GroupX] = NULL ;
//	 } ;
//	return(TRUE) ;
//}

///*	han_FreeOnClose - Set handle to free allocated memory on handle close	*/
///*	Call: result = han_FreeOnClose( handle )
//	  where result is TRUE if OK, FALSE if can't (index is bad)
//		handle is handle to be marked					*/
//
//int han_FreeOnClose(handle)
//  int handle ;
//{ struct han__Master *hm ;
//  struct han__Group *hg ;
//  struct han__Id *id ;
//
//	id = (struct han__Id *)&handle ;
//	hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
//	if (hg == NULL) return(FALSE) ;
//	if (hg->Entry[id->EntryX].HandleId != handle) return(FALSE) ;
//	hg->Entry[id->EntryX].Flags |= HANFlag_FreeMem ;
//	return(TRUE) ;
//}
//
///*	han_SetPointer - Sets (data) pointer for a handle		*/
///*	Call: result = han_SetPointer( handle, index, pointer )
//	  where result is TRUE if OK, FALSE if can't (index is bad)
//		handle is handle to be updated,
//		index is 0/1 for different pointer slots,
//		pointer is actual pointer to be saved			*/
//
int han_SetPointer(handle,index,pointer)
  int handle,index ;
  void *pointer ;
{ struct han__Master *hm ;
  struct han__Group *hg ;
  struct han__Id *id ;

	id = (struct han__Id *)&handle ;
	hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
	if (hg == NULL) return(FALSE) ;
	if (hg->Entry[id->EntryX].HandleId != handle) return(FALSE) ;
	switch (index)
	 { default:	return(FALSE) ;
	   case 0: hg->Entry[id->EntryX].P1 = pointer ; break ;
	 } ;
	return(TRUE) ;
}

/*	hanGetPointer - Gets (data) pointer for a handle		*/
/*	Call: pointer = han_GetPointer( handle, index )
	  where pointer is requested pointer (NULL if none or bad call)
		handle is handle to be queried,
		index is 0/1 for different pointer slots,		*/

void *han_GetPointer(handle,index)
  int handle,index ;
{ struct han__Master *hm ;
  struct han__Group *hg ;
  struct han__Id *id ;

	id = (struct han__Id *)&handle ;
	hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
	if (hg == NULL) return(NULL) ;
	if (hg->Entry[id->EntryX].HandleId != handle) return(NULL) ;
	switch (index)
	 { default:	break ;
	   case 0:	return(hg->Entry[id->EntryX].P1) ;
	 } ;
	return(NULL) ;
}


///*	han_SetParent - Sets parent for a handle			*/
///*	Call: result = han_SetParent( handle, parenthandle )
//	  where result is TRUE if OK, FALSE if can't (handle is bad)
//		handle is handle to be updated,
//		parenthandle is handle of parent (must be valid)	*/
//
//int han_SetParent(handle,parenthandle)
//  int handle,parenthandle ;
//{ struct han__Master *hm ;
//  struct han__Group *hg ;
//  struct han__Id *id ;
//
//	id = (struct han__Id *)&parenthandle ;		/* First set HasChild flag on parent */
//	hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
//	if (hg == NULL) return(FALSE) ;
//	if (hg->Entry[id->EntryX].HandleId != parenthandle) return(FALSE) ;
//	hg->Entry[id->EntryX].Flags |= HANFlag_HasChild ;
//
//	id = (struct han__Id *)&handle ;
//	hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
//	if (hg == NULL) return(FALSE) ;
//	if (hg->Entry[id->EntryX].HandleId != handle) return(FALSE) ;
//	hg->Entry[id->EntryX].ParentHandleId = parenthandle ;
//	return(TRUE) ;
//}
//
///*	hanGetParent - Gets parent handle for a handle		*/
///*	Call: parenthandle = han_GetParent( handle )
//	  where parenthandle is handle of parent (HAN_UNUSED if bad call or no parent)
//		handle is handle to be queried			*/
//
//int han_GetParent(handle)
//  int handle ;
//{ struct han__Master *hm ;
//  struct han__Group *hg ;
//  struct han__Id *id ;
//
//	id = (struct han__Id *)&handle ;
//	hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
//	if (hg == NULL) return(HAN_UNUSED) ;
//	if (hg->Entry[id->EntryX].HandleId != handle) return(HAN_UNUSED) ;
//	return(hg->Entry[id->EntryX].ParentHandleId) ;
//}


///*	han_SetUpdate - Sets Update flag for handle			*/
///*	Call: result = han_SetUpdate( handle )
//	  where result is TRUE if OK, FALSE if can't (handle is bad)
//		handle is handle to be updated				*/
//
//int han_SetUpdate(handle)
//  int handle ;
//{ struct han__Master *hm ;
//  struct han__Group *hg ;
//  struct han__Id *id ;
//
//	id = (struct han__Id *)&handle ;		/* First set HasChild flag on parent */
//	hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
//	if (hg == NULL) return(FALSE) ;
//	if (hg->Entry[id->EntryX].HandleId != handle) return(FALSE) ;
//	hg->Entry[id->EntryX].Flags |= HANFlag_IsUpdated ;
//	return(TRUE) ;
//}

///*	han_IsUpdated - Returns Update status on a handle	*/
///*	Call: result = han_IsUpdated( handle )
//	  where result is TRUE if updated, FALSE if bad handle or not updated,
//		handle is handle to be queried			*/
//
//int han_IsUpdated(handle)
//  int handle ;
//{ struct han__Master *hm ;
//  struct han__Group *hg ;
//  struct han__Id *id ;
//
//	id = (struct han__Id *)&handle ;
//	hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
//	if (hg == NULL) return(HAN_UNUSED) ;
//	if (hg->Entry[id->EntryX].HandleId != handle) return(HAN_UNUSED) ;
//	return((hg->Entry[id->EntryX].Flags & HANFlag_IsUpdated) != 0) ;
//}

/*	han_SetInfo - Sets User Info word for handle			*/
/*	Call: result = han_SetInfo( handle , info )
	  where result is TRUE if OK, FALSE if can't (handle is bad)
		handle is handle to be updated,
		info is information to be saved				*/

int han_SetInfo(handle,info)
  int handle,info ;
{ struct han__Master *hm ;
  struct han__Group *hg ;
  struct han__Id *id ;

	id = (struct han__Id *)&handle ;		/* First set HasChild flag on parent */
	hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
	if (hg == NULL) return(FALSE) ;
	if (hg->Entry[id->EntryX].HandleId != handle) return(FALSE) ;
	hg->Entry[id->EntryX].Info = info ;
	return(TRUE) ;
}

/*	han_GetInfo - Returns info word associated with a handle	*/
/*	Call: info = han_GetInfo( handle )
	  where info is info-word for handle (FALSE if problem)		*/

int han_GetInfo(handle)
  int handle ;
{ struct han__Master *hm ;
  struct han__Group *hg ;
  struct han__Id *id ;

	id = (struct han__Id *)&handle ;
	hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
	if (hg == NULL) return(HAN_UNUSED) ;
	if (hg->Entry[id->EntryX].HandleId != handle) return(HAN_UNUSED) ;
	return(hg->Entry[id->EntryX].Info) ;
}

///*	han_SetType - Sets Type Info for Handle				*/
///*	Call: result = han_SetType( handle , type )
//	  where result is TRUE if OK, FALSE if can't (handle is bad)
//		handle is handle to be updated,
//		type is information to be saved				*/
//
//int han_SetType(handle,type)
//  int handle,type ;
//{ struct han__Master *hm ;
//  struct han__Group *hg ;
//  struct han__Id *id ;
//
//	id = (struct han__Id *)&handle ;		/* First set HasChild flag on parent */
//	hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
//	if (hg == NULL) return(FALSE) ;
//	if (hg->Entry[id->EntryX].HandleId != handle) return(FALSE) ;
//	hg->Entry[id->EntryX].Type = type ;
//	return(TRUE) ;
//}
//
///*	han_GetType - Returns type word associated with a handle	*/
///*	Call: type = han_GetType( handle )
//	  where type is type info for handle (FALSE if problem)		*/
//
//int han_GetType(handle)
//  int handle ;
//{ struct han__Master *hm ;
//  struct han__Group *hg ;
//  struct han__Id *id ;
//
//	id = (struct han__Id *)&handle ;
//	hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
//	if (hg == NULL) return(HAN_UNUSED) ;
//	if (hg->Entry[id->EntryX].HandleId != handle) return(HAN_UNUSED) ;
//	return(hg->Entry[id->EntryX].Type) ;
//}

///*	han_AddHandleToList - Adds handle to linked list of handles	*/
///*	Call: result = han_AddHandleToList( listhandle , newhandle , position )
//	  where result is TRUE if OK,
//	  	listhandle is existing handle in list,
//	  	newhandle is handle to be added to the list,
//	  	position is -1 for adding before, +1 to add after,
//	  	position is 0 to append to end of chain (listhandle is handle of parent for list)	*/
//
//LOGICAL han_AddHandleToList(listhandle,newhandle,position)
//  int listhandle,newhandle ;
//  int position ;
//{ struct han__Master *hm ;
//  struct han__Group *hg,*hgx ;
//  struct han__Id *id,*idx ;
//  int thandle,priorhandle,nexthandle ;
//
//	id = (struct han__Id *)&listhandle ;
//	hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
//	if (hg == NULL) return(FALSE) ;
//	if (hg->Entry[id->EntryX].HandleId != listhandle) return(FALSE) ;
//	if (position == 0)
//	 { position = 1 ;				/* Force an append below */
//	   if (hg->Entry[id->EntryX].PriorHandle != HAN_UNUSED)
//	    { thandle = hg->Entry[id->EntryX].PriorHandle ; }	/* PriorHandle on parent of list is last handle in list! */
//	    else { thandle = listhandle ; } ;
//	   hg->Entry[id->EntryX].PriorHandle = newhandle ;	/* Save newhandle as new end of list */
//	   listhandle = thandle ;
//	   id = (struct han__Id *)&listhandle ;			/* Reposition to possible new listhandle */
//	   hg = hm->Group[id->GroupX] ;
//	   if (hg == NULL) return(FALSE) ;
//	   if (hg->Entry[id->EntryX].HandleId != listhandle) return(FALSE) ;
//	 } ;
//
//	if (position < 0)
//	 { priorhandle = hg->Entry[id->EntryX].PriorHandle ;
//	   nexthandle = listhandle ;
//	   hg->Entry[id->EntryX].PriorHandle = newhandle ;
//	   idx = (struct han__Id *)&priorhandle ;
//	   hgx = hm->Group[id->GroupX] ;
//	   if (hgx == NULL) return(FALSE) ;
//	   if (hgx->Entry[idx->EntryX].HandleId != priorhandle) return(FALSE) ;
//	   hgx->Entry[idx->EntryX].NextHandle = newhandle ;
//	 } else if (position > 0)
//	 { priorhandle = listhandle ;
//	   nexthandle = hg->Entry[id->EntryX].NextHandle ;
//	   hg->Entry[id->EntryX].NextHandle = newhandle ;
//	   if (nexthandle != HAN_UNUSED)		/* Link next handle unless appending to end of list */
//	    { idx = (struct han__Id *)&nexthandle ;
//	      hgx = hm->Group[id->GroupX] ;
//	      if (hgx == NULL) return(FALSE) ;
//	      if (hgx->Entry[idx->EntryX].HandleId != nexthandle) return(FALSE) ;
//	      hgx->Entry[idx->EntryX].PriorHandle = newhandle ;
//	    } ;
//	 } ;
//	id = (struct han__Id *)&newhandle ;
//	hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
//	if (hg == NULL) return(FALSE) ;
//	if (hg->Entry[id->EntryX].HandleId != newhandle) return(FALSE) ;
//	hg->Entry[id->EntryX].PriorHandle = priorhandle ;
//	hg->Entry[id->EntryX].NextHandle = nexthandle ;
//	return(TRUE) ;
//}
//
///*	han_RemoveHandleFromList - Removes a handle from linked list	*/
///*	Call: result = han_RemoveHandleFromList( handle )
//	  where result is TRUE if OK (** NOTE ** handle is not closed!),
//	  handle is handle to be removed				*/
//
//LOGICAL han_RemoveHandleFromList(handle)
//  int handle ;
//{ struct han__Master *hm ;
//  struct han__Group *hg ;
//  struct han__Id *id ;
//  int priorhandle,nexthandle ;
//
//	id = (struct han__Id *)&handle ;
//	hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
//	if (hg == NULL) return(FALSE) ;
//	if (hg->Entry[id->EntryX].HandleId != handle) return(FALSE) ;
//	priorhandle = hg->Entry[id->EntryX].PriorHandle ;
//	nexthandle = hg->Entry[id->EntryX].NextHandle ;
//	if (priorhandle != HAN_UNUSED)
//	 { id = (struct han__Id *)&priorhandle ;
//	   hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
//	   if (hg == NULL) return(FALSE) ;
//	   if (hg->Entry[id->EntryX].HandleId != priorhandle) return(FALSE) ;
//	   hg->Entry[id->EntryX].NextHandle = nexthandle ;
//	 } else
//	if (nexthandle != HAN_UNUSED)
//	 { id = (struct han__Id *)&nexthandle ;
//	   hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
//	   if (hg == NULL) return(FALSE) ;
//	   if (hg->Entry[id->EntryX].HandleId != nexthandle) return(FALSE) ;
//	   hg->Entry[id->EntryX].PriorHandle = priorhandle ;
//	 } ;
//	return(TRUE) ;
//}

///*	han_GetNextHandle - Returns next handle in a list		*/
///*	Call: nexthandle = han_GetNextHandle( handle )
//	  where nexthandle is next in list (HAN_UNUSED at end of list,
//	  	handle is current position in list			*/
//
//int han_GetNextHandle(handle)
//  int handle ;
//{ struct han__Master *hm ;
//  struct han__Group *hg ;
//  struct han__Id *id ;
//
//	id = (struct han__Id *)&handle ;
//	hm = han_PtrToMaster() ; hg = hm->Group[id->GroupX] ;
//	if (hg == NULL) return(HAN_UNUSED) ;
//	if (hg->Entry[id->EntryX].HandleId != handle) return(HAN_UNUSED) ;
//	return(hg->Entry[id->EntryX].NextHandle) ;
//}


/*	v_GetProcessInfo - Gets and/or sets current context		*/
/*	Call: gpi = v_GetProcessInfo()
	  where gpi is pointer to current process			*/

#include <signal.h>

struct V4C__ProcessInfo *v_GetProcessInfo()
{ 
  struct UC__File UCFile ;
  int pt,ok,makebin ; UCCHAR errbuf[512],msgbuf[512],rtFileName[512] ;

	if (gpi != NULL) return(gpi) ;
	gpi = (struct V4C__ProcessInfo *)v4mm_AllocChunk(sizeof *gpi,TRUE) ;
#ifdef UNIX
	gettimeofday (&gpi->bgnTime,NULL) ;
#endif
	gpi->StartClock = clock() ; time(&gpi->StartTime) ;
#ifdef WINNT
	GetProcessTimes(GetCurrentProcess(),&gpi->ftCreate,&gpi->ftExit,&gpi->ftKernel1,&gpi->ftUser1) ;
#endif
#ifdef V4_BUILD_TELNET_XFACE
	setbuf(stdout,NULL) ;		/* Turn off output buffering if doing the telnet thing */
#endif

	gpi->MaxAllowedErrs = V4CTX_DfltMaxAllowedErrs ;
	gpi->sortWaitFlag = V_SPAWNSORT_DfltWait ;
	gpi->MinutesWest = mscu_minutes_west() ;
	gpi->curYear = mscu_udate_to_yyyymmdd(valUDATEisNOW) / 10000 ;
	gpi->XMLIndent = UNUSED ;
	gpi->DoEchoS = TRUE ;					/* Default to "Set EchoS SpreadSheet" */
//	gpi->EchoZ = FALSE ;
	gpi->ErrEcho = TRUE ;					/* Output error messages in v4_error() */
	gpi->DoLogToIsct = TRUE ;				/* Enable LogToIsct command */
	gpi->MaxPointOutput = V4CTX_MaxPointOutputInit ;
	gpi->OutputCharSet = V4L_TextFileType_ASCII ;		/* Default is to output ASCII character set */
	gpi->inputCharSet = V4L_TextFileType_UTF8 ;		/* Set default (as of 9/14/22) to UTF8 */
	INITMTLOCK(gpi->msgLock) ;
	vout_Init() ;
	gpi->dfltAreaAggX = DFLTAGGAREA ;			/* Auto-determine output aggregate area */
	gpi->mmm = v4mm_MemMgmtInit(NULL) ;			/* Allocate & init mmm */
	gpi->LowHNum = 99 ; gpi->HighHNum = 0 ; gpi->NextUId = 100 ;
	
	gpi->fileIdDefer = UNUSED ;

	gpi->rtOptions = (0 
#ifdef V4UNICODE
	| V_RtOpt_Unicode
#endif
#ifdef V4ENABLEMULTITHREADS
	| V_RtOpt_MultiThread
#endif
#ifdef V4_BUILD_LMTFUNC_EXP_DATE
	| V_RtOpt_Expires
#endif
#if defined V4_BUILD_LMTFUNC_MAX_LIST || defined V4_BUILD_LMTFUNC_MAX_AREA || defined V4_BUILD_LMTFUNC_MAX_CTX_PTS
	| V_RtOpt_Limited
#endif
#ifdef V4_BUILD_CS_FUNCTIONALITY
	| V_RtOpt_ClientServer 
#endif
#ifdef V4_BUILD_TELNET_XFACE
	| V_RtOpt_Telnet 
#endif
#ifdef V4_BUILD_SECURITY
	| V_RtOpt_advSecurity 
#endif
#if SIZEofPTR == 8
	| V_RtOpt_64Bit
#endif
	  ) ;
#define SetOptSt(bit,val) if (gpi->rtOptions & V_RtOpt_##bit) UCstrcat(gpi->rtOptString,UClit(val)) ;
	SetOptSt(Unicode,"UC ") ; SetOptSt(MultiThread,"MT ") ; SetOptSt(Expires,"EX ") ; SetOptSt(Limited,"LF ") ;
	SetOptSt(ClientServer,"CS ") ; SetOptSt(64Bit,"64 ") ; SetOptSt(Telnet,"TN ") ; SetOptSt(advSecurity,"AS ") ;
	gpi->rtOptString[UCstrlen(gpi->rtOptString)-1] = UCEOS ;	/* Get rid of trailing space */
	gpi->mct = NULL ; ok = TRUE ; makebin = FALSE ;
	UCsprintf(rtFileName,UCsizeof(rtFileName),UClit("%s-%d-%d-%d.bin"),V4RUNTIMEINFOFILEPREFIX,V4IS_MajorVersion,V4IS_MinorVersion,(int)(sizeof (void *) * 8)) ;
	if (TRUE)			/* Try to pull "precompiled" binary file */
	 { if (ok = v_UCFileOpen(&UCFile,v_GetV4HomePath(rtFileName),UCFile_Open_ReadBin,FALSE,errbuf,0))
	    { gpi->mct = (struct V4__MasterCustomTable *)v4mm_AllocChunk(V4_MCT_HeaderBytes,FALSE) ;
	      ok = (fread(gpi->mct,V4_MCT_HeaderBytes,1,UCFile.fp) > 0) ;
	      if (ok && gpi->mct->RevLevel == V4_MasterCustomTableRevLvl)
	       { gpi->mct = (struct V4__MasterCustomTable *)realloc(gpi->mct,gpi->mct->TotalBytes) ;
	         rewind(UCFile.fp) ; ok = (fread(gpi->mct,gpi->mct->TotalBytes,1,UCFile.fp) > 0) ;
	       } ;
	      v_UCFileClose(&UCFile) ;
	    } ;
	 } ;
	if (!ok)
	 { 
#ifdef _DEBUG
	   signal(SIGSEGV,SIG_DFL) ;
#endif
	   makebin = TRUE ;
	   if (gpi->mct != NULL) { v4mm_FreeChunk(gpi->mct) ; gpi->mct = NULL ; } ;
	   v_LoadMsgFile(gpi) ;
	   if (!v_LoadUnicodeData(errbuf))
	    { v_Msg(NULL,msgbuf,"@* Err accessing V4/Unicode data file: %1U\n",errbuf) ; vout_UCText(VOUT_Warn,0,msgbuf) ; } ;
	   uci = (struct V__UnicodeInfo *)&gpi->mct->Data[gpi->mct->UnicodeOffset] ;
	   v_LoadCountryInfo() ;					/* This needs uci info !! */
	   uci = (struct V__UnicodeInfo *)&gpi->mct->Data[gpi->mct->UnicodeOffset] ;
	   v_LoadColorTable() ;
	   uci = (struct V__UnicodeInfo *)&gpi->mct->Data[gpi->mct->UnicodeOffset] ;
	   v_HTMLEntityTableLoad() ;
	   v_LoadSiteIni(gpi,UClit("v4Site.ini"),0) ;
/*	   Now try to create new binary runtime file */
	   gpi->mct->TotalBytes = (((char *)(&gpi->mct->Data[gpi->mct->NextAvail]) - (char *)gpi->mct) + 0x1ff) & (~0x1ff) ;
	   if (ok = v_UCFileOpen(&UCFile,v_GetV4HomePath(rtFileName),UCFile_Open_WriteBin,FALSE,errbuf,0))
	    { ok = (fwrite(gpi->mct,gpi->mct->TotalBytes,1,UCFile.fp) > 0) ;
	      v_UCFileClose(&UCFile) ;
	    } ;
	 } ;

/*	Now link up all structures - they should not be moving around at this point */
	uci = (struct V__UnicodeInfo *)&gpi->mct->Data[gpi->mct->UnicodeOffset] ;
	gpi->mi = (struct V4MSG__MsgInfo *)&gpi->mct->Data[gpi->mct->MsgInfoOffset] ;
	gpi->ci = (struct V4CI__CountryInfo *)&gpi->mct->Data[gpi->mct->CountryInfoOffset] ;
	gpi->ci->li = (struct V4LI_LanguageInfo *)&gpi->mct->Data[gpi->mct->LanguageInfoOffset] ;
	gpi->ci->cli = (struct V4LI_CalendarInfo *)&gpi->mct->Data[gpi->mct->CalendarInfoOffset] ;
	vcv = (struct v__ColorValues *)&gpi->mct->Data[gpi->mct->ColorOffset] ;
	gpi->hent = (struct V4HTML_Entities *)&gpi->mct->Data[gpi->mct->HTMLEntityOffset] ;
	gpi->vise = (gpi->mct->iniValsOffset == UNUSED ? NULL : (struct vIni__SectionEntry *)&gpi->mct->Data[gpi->mct->iniValsOffset]) ;
	if (gpi->vise != NULL)					/* Decrypt the ini file values */
	 { INDEX i ; B64INT *l = (B64INT *)gpi->vise ;
	   for(i=1;i<=gpi->mct->iniValBytes/sizeof(B64INT);i++,l++) { *l ^= gpi->mct->iniValXORKey ; } ;
	 }
	

	if (makebin)
	 { if (ok) { v_Msg(NULL,errbuf,"V4BinFileOK",v_GetV4HomePath(rtFileName)) ; vout_UCText(VOUT_Status,0,errbuf) ; }
	    else { v_Msg(NULL,errbuf,"FileWriteErr",errno,v_GetV4HomePath(UClit("v4UnicodeInfo.bin"))) ; vout_UCText(VOUT_Warn,0,errbuf) ; vout_NL(VOUT_Warn) ; } ;
	 } ;
	
	for(pt=0;pt<V4DPI_PntType_Last;pt++)			/* Init all point sizes by point type */
	 { switch(pt)
	    { CASEofINT
	      case V4DPI_PntType_SSVal:
	      case V4DPI_PntType_PntIdx:
	      case V4DPI_PntType_AggRef:
	      case V4DPI_PntType_Special:
	      case V4DPI_PntType_MDArray:
	      case V4DPI_PntType_Color:
	      case V4DPI_PntType_Country:
	      case V4DPI_PntType_Dict:
	      case V4DPI_PntType_XDict:		gpi->PointBytes[pt] = V4PS_Int ; break ;
	      case V4DPI_PntType_Calendar:	gpi->PointBytes[pt] = V4PS_Calendar ; break ;
	      case V4DPI_PntType_Real:		gpi->PointBytes[pt] = V4PS_Real ; break ;
	      case V4DPI_PntType_FrgnDataEl:	gpi->PointBytes[pt] = sizeof(struct V4FFI__DataElSpec) + V4DPI_PointHdr_Bytes ; break ;
	      case V4DPI_PntType_FrgnStructEl:	gpi->PointBytes[pt] = sizeof(struct V4FFI__StructElSpec) + V4DPI_PointHdr_Bytes ; break ;
	      case V4DPI_PntType_MemPtr:	gpi->PointBytes[pt] = V4PS_MemPtr ; break ;
	      case V4DPI_PntType_ParsedJSON:	gpi->PointBytes[pt] = V4DPI_PointHdr_Bytes ; break ;
	      case V4DPI_PntType_Int2:		gpi->PointBytes[pt] = V4PS_Int2 ; break ;
	      case V4DPI_PntType_XDB:		gpi->PointBytes[pt] = V4PS_XDB ; break ;
	      case V4DPI_PntType_V4IS:		gpi->PointBytes[pt] = V4DPI_PointHdr_Bytes + sizeof(struct V4DPI__PntV4IS) ; break ;
//	      case V4DPI_PntType_WormHole:	gpi->PointBytes[pt] = V4PS_Int ; break ;
	      case V4DPI_PntType_Fixed:		gpi->PointBytes[pt] = V4PS_Fixed ; break ;
	      case V4DPI_PntType_TeleNum:	gpi->PointBytes[pt] = V4PS_Tele ; break ;
	      case V4DPI_PntType_UOM:		gpi->PointBytes[pt] = V4PS_UOM ; break ;
	      case V4DPI_PntType_UOMPer:	gpi->PointBytes[pt] = V4PS_UOMPer ; break ;
	      case V4DPI_PntType_UOMPUOM:	gpi->PointBytes[pt] = V4PS_UOMPUOM ; break ;
//	      case V4DPI_PntType_GeoCoord:	gpi->PointBytes[pt] = V4PS_GeoCoord ; break ;
	      case V4DPI_PntType_Complex:	gpi->PointBytes[pt] = V4PS_Complex ; break ;
	      case V4DPI_PntType_Tree:		gpi->PointBytes[pt] = V4PS_Tree ; break ;
	    } ;
	 } ;
//#ifdef _DEBUG
//	if (V4PS_Tele != V4PS_Int2) printf("*** WARNING - V4PS_Tele != V4PS_Int2 ***\n") ;
//#endif
#ifdef V4_BUILD_LMTFUNC_EXP_DATE
	gpi->Expired = (gpi->StartTime -(60*gpi->MinutesWest))/VCAL_SecsInDay ;
	if (gpi->Expired <= (V4_BUILD_LMTFUNC_EXP_DATE - TIMEOFFSETDAY)) { gpi->Expired = UNUSED ; }
	 else { gpi->Expired = (v_Hash32(&gpi->StartTime,sizeof gpi->StartTime,1) & 0xffff) ; } ;
#endif
	return(gpi) ;
}
/*	D A T E   M O D U L E S					*/
short int julian[12] =
 { 0,31,59,90,120,151,181,212,243,273,304,334 } ;
short int julian_leap[12] =
 { 0,31,60,91,121,152,182,213,244,274,305,335 } ;

/*	mscu_ymd_to_ud - Converts year+month+day to universal */

int mscu_ymd_to_ud(year,month,day)
 int year,month,day ;
{ int ud ;

	VCALADJYEAR(FALSE,year) ;
	ud = 365*year + 31*(month-1) + day - 678955 ;
	if (month <= 2)
	 { ud += (year-1)/4 ; }
	 else { ud += year/4 - (4*month+23)/10 ; } ;
	return(ud) ;
}
/*	vcal_UDateFromYMD - Converts year+month+day to universal */
/*	Returns VCAL_BadVal if date not valid, err updated with error */

int vcal_UDateFromYMD(isHistory,year,month,day,err)
 LOGICAL isHistory ;
 int year,month,day ; UCCHAR *err ;
{ int ud,rd,ok, xx ; LOGICAL isYY ;

	isYY = year < 1000 ;
	VCALADJYEAR(isHistory,year) ;
/*	Check for special -values */
	if (year == 1800 && month == 1) return(-day) ;
	if (!VerifyGregorianYMD(year,month,day,err)) return(VCAL_BadVal) ;
	rd = FixedFromGregorian(year,month,day) ;
	ud = vcal_CalToUDate((double)rd,VCAL_TimeZone_Local,&ok) ;
	if (!ok) { UCsprintf(err,50,UClit("Date specified %d-%02d-%02d too small or too big"),year,month,day) ; return(VCAL_BadVal) ; } ;
	xx = valUDATEisNOW ;
	if (isHistory && ud > valUDATEisNOW)
	 { if (isYY) { year -= 100 ; return(vcal_UDateFromYMD(isHistory,year,month,day,err)) ; } ;
	   UCstrcpy(err,UClit("Date is in the future")) ; return(VCAL_BadVal) ;
	 } ;
	return(ud) ;
}
/*	vcal_UDTFromYMD - Converts year+month+day+hour+minute+second to UTD */
/*	Returns VCAL_BadVal if date not valid, err updated with error */

int vcal_UDTFromYMD(year,month,day,hour,minute,second,err)
 int year,month,day ; UCCHAR *err ;
{ int udt,rd,ok ;

	VCALADJYEAR(FALSE,year) ;
	if (hour<0 || hour>23 || minute < 0 || minute > 59 || second < 0 || second > 59) { v_Msg(NULL,err,"ParseUHMS1",hour,minute,(double)second) ; return(VCAL_BadVal) ; } ;
	if (!VerifyGregorianYMD(year,month,day,err)) return(VCAL_BadVal) ;
	rd = FixedFromGregorian(year,month,day) ;
	udt = vcal_CalToUDate((double)rd,VCAL_TimeZone_Local,&ok) ;
	if (!ok || udt <= VCAL_UDTUDateOffset) { UCsprintf(err,50,UClit("Date specified %d-%02d-%02d too small or too big"),year,month,day) ; return(VCAL_BadVal) ; } ;
	udt = (udt - VCAL_UDTUDateOffset) * VCAL_SecsInDay + hour*60*60 + minute*60 + second ;
	return(udt) ;
}


/*	mscu_udate_to_yyyymmdd - Converts universal to yyyymmdd	*/
int mscu_udate_to_yyyymmdd(udate)
  int udate ;
{ int ud,year,month,day ;

	if (udate == VCAL_LOCALNOW)
	 { 
//	   if (gpi == NULL) { gpi = (struct V4C__ProcessInfo *)v_GetProcessInfo() ; } ;
//	   udate = TIMEOFFSETDAY + (time(NULL)-(60*gpi->MinutesWest))/VCAL_SecsInDay ;
	   udate = valUDATEisNOW ;
	 } ;
	if (udate <= 0 || udate > VCAL_UDateMax) return(0) ;
	year = ((udate+678955)*4)/1460 ;
	for (;;)
	 { ud = mscu_ymd_to_ud(year,1,1) ;
	   if (ud > udate) { year-- ; continue ; } ;
	   if (udate - ud < ((year % 4) == 0 ? 366 : 365)) break ;
	   year-- ;
	 } ;
/*	Fake it as julian */
	udate = year*1000 + udate-ud + 1 ;
	year = udate/1000 ; udate %= 1000 ;
	if ((year % 4) == 0)
	 { for (month=1;month<12;month++)
	    { if (udate <= julian_leap[month]) break ; } ;
	   day = udate-julian_leap[month-1] ;
	 } else
	 { for (month=1;month<12;month++)
	    { if (udate <= julian[month]) break ; } ;
	   day = udate-julian[month-1] ;
	 } ;
	return(year*10000+month*100+day) ;
}

/*	mscu_udate_to_yyyyddd - Converts universal to julian format	*/
int mscu_udate_to_yyyyddd(udate)
  int udate ;
{ int ud,year ;

	if (udate <= 0 || udate > VCAL_UDateMax) return(0) ;
	year = ((udate+678955)*4)/1460 ;
	for (;;)
	 { ud = mscu_ymd_to_ud(year,1,1) ;
	   if (ud > udate) { year-- ; continue ; } ;
	   if (udate - ud < ((year % 4) == 0 ? 366 : 365)) break ;
	   year-- ;
	 } ;
/*	Fake it as julian */
	udate = year*1000 + udate-ud + 1 ;
	return(udate) ;
}

/*	mscu_udate_to_uweek - Converts universal to week */

int mscu_udate_to_uweek(udate,baseDate)
  int udate,baseDate ;
{ int ymd,jan1,dow,basedate,wn ;

	if (baseDate > 0)
	 { if (udate < baseDate) return(VCAL_BadVal) ;
	   return(((udate - baseDate) / 7) + 1) ;		/* If we have base date then just calculate week off of it */
	 } ;
	ymd = mscu_udate_to_yyyymmdd(udate) ;
	jan1 = mscu_ymd_to_ud(ymd/10000,01,01) ;
	dow = ((jan1 + 1)%7) + 1 ;			/* Day of week for first day of year (1=Mon) */
	if (dow == 1) { basedate = jan1 ; }
	 else { basedate = jan1 + (8 - dow) ; } ;
	if (udate < basedate)				/* If current date before base date then UWeek is #1 of current year */
	 return(((ymd / 10000) - VCAL_BaseYear) * 52 + 00) ;
/*	Figure out week number from, if > 52 then cut back to 52 */
	wn = (udate - basedate) / 7 ;
	if (wn > 51) wn = 51 ;
	return(((ymd / 10000) - VCAL_BaseYear) * 52 + wn) ;
}

/*	mscu_uweek_to_udate - Converts UWeek to UDate */
int mscu_uweek_to_udate(uweek,baseDate)
  int uweek,baseDate ;
{ int jan1,dow,basedate ;

	if (uweek == 0) return(0) ;
	if (baseDate > 0)
	 return(baseDate + (uweek - 1) * 7) ;
	jan1 = mscu_ymd_to_ud(VCAL_BaseYear + uweek/52,01,01) ;
	dow = ((jan1 + 1)%7) + 1 ;
	if (dow <= 3)
	 { basedate = jan1 - dow + 1 ; }		/* Base date is last monday of prior year (or jan1 if it is a Monday) */
	 else { basedate = jan1 + (8 - dow) ; } ;	/* Base date is first Monday of year */
	return(basedate + 7 * (uweek % 52)) ;
}

/*	mscu_udate_to_ddmmmyy - Converts universal to dd-mmm-yy	*/
UCCHAR *mscu_udate_to_ddmmmyy(udate)
  int udate ;
{ 
  static UCCHAR rp[24] ;

	v_FormatDate(&udate,V4DPI_PntType_UDate,VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,NULL,rp) ;
	return(rp) ;
}

/*	mscu_udt_to_ddmmmyyhhmmss - Converts universal date-time to "dd-mmm-yy hh:mm:ss"	*/
UCCHAR *mscu_udt_to_ddmmmyyhhmmss(udt)
  int udt ;
{ static UCCHAR rp[24] ;

	v_FormatDate(&udt,V4DPI_PntType_UDT,VCAL_CalType_UseDeflt,VCAL_TimeZone_Local,NULL,rp) ;
	return(rp) ;
}

/*	mscu_udt_to_msdos - converts UDT to MS-DOS time-time */
int mscu_udt_to_msdos(udt)
  int udt ;
{ int yyyymmdd,ts,msdos ;

	if (udt <= 0) return(0) ;
	yyyymmdd = mscu_udate_to_yyyymmdd(UDTtoUD(udt))	;
	ts = (udt % VCAL_SecsInDay) ;
	
	msdos = (((((yyyymmdd / 10000) - 1980) << 9) | (((yyyymmdd % 10000) / 100) << 5) | (yyyymmdd % 100)) << 16)
			| ((ts / 3600) << 11) | ((ts % 3600) / 60) << 5 | ((ts % 60) / 2) ;

	return(msdos) ;
}

/*	mscu_msdos_to_udt - converts MS-DOS to UDT */
int mscu_msdos_to_udt(msdos)
  int msdos ;
{ int yy,mm,dd,hh,min,sec ; UCCHAR err[1000] ;

	if (msdos == 0) return(0) ;
	yy = 1980 + (msdos >> 25) ; mm = (msdos >> 21) & 0xf ; dd = (msdos >> 16) & 0x1f ;
	hh = (msdos >> 11) & 0x1f ; min = (msdos >> 5) & 0x3f ; sec = msdos & 0x1f ;


	return(vcal_UDTFromYMD(yy,mm,dd,hh,min,sec,err)) ;
/*
$day = $int & 31;
  $month = ($int & 480) >> 5;
  $year = 1980 + (($int & 65024) >> 9);
  if ($day >= 1 and $day <= 31 and $month >= 1 and $month <= 12 and $year <= 2040) {    return array($day, $month, $year);
  }}function ParseDostIME($int) {  $sec = 2 * ($int & 31);
  $min = ($int & 2016) >> 5;
  $hour = ($int & 63488) >> 11;
  if ($sec <= 60 and $min <= 59 and $hour <= 23) {    return array($hour, $min, $sec);
  }
*/


//	return(msdos) ;
}

/*	mscu_minutes_west - Returns number of minutes west of GMT we be at */
int mscu_minutes_west()
{
#ifdef WINNT
  TIME_ZONE_INFORMATION winnt_tzi ;
#endif
#ifdef UNIX
  struct tm *tm ; time_t clock ;
#endif
#ifdef VMSOS
  struct tm *tm ; time_t clock ;
#endif
  int mw ;
/*	Figure out where in the world we are (i.e. which time zone (or GRID as per JES)) */
#ifdef UNIX
	time(&clock) ;
	tm = gmtime(&clock) ; mw = tm->tm_hour*60 + tm->tm_min ;
	tm = localtime(&clock) ; mw -= tm->tm_hour*60 + tm->tm_min ;
	if (mw < 0) mw += 24*60 ;		/* Adjust if GMT past midnight */
#elif VMSOS
	time(&clock) ;
	tm = gmtime(&clock) ; mw = tm->tm_hour*60 + tm->tm_min ;
	tm = localtime(&clock) ; mw -= tm->tm_hour*60 + tm->tm_min ;
	if (mw < 0) mw += 24*60 ;		/* Adjust if GMT past midnight */
#elif WINNT
	if (GetTimeZoneInformation(&winnt_tzi) == TIME_ZONE_ID_DAYLIGHT)
	 { mw = (winnt_tzi.Bias - 60) ; }
	 else { mw = winnt_tzi.Bias ; } ;
#else
	mw = 0 ;
#endif
	return(mw) ;
}

/*	v_FormatHTTPDateTime - Returns string formatted to valid http time (ex: Fri, 30 Oct 1998 14:19:41 GMT) */
/*	Call: ftime = v_FormatHTTPDateTime( zulutime , offset )
	  where ftime = formatted time as ASCIZ string,
		zulutime is GMT time for be formatted, UNUSED if not to be used,
		offset is time offset, in seconds, from current time if zulutime is UNUSED	*/

char *v_FormatHTTPDateTime(zulutime,offset)
  time_t zulutime ;
  int offset ;
{ struct tm *tm ;
  time_t clock ;
  static char tbuf[64] ;

	if (zulutime == UNUSED)
	 { clock = time(NULL) + offset ; tm = gmtime(&clock) ; }
	 else { tm = gmtime(&zulutime) ; } ;
	strftime(tbuf,sizeof tbuf,"%a, %d %b %Y %H:%M:%S GMT",tm) ;
	return(tbuf) ;
}

/*	P E R F O R M A N C E   F U N C T I O N S		*/

/*	v_ConnectTime() - Returns connect time in seconds as double (returns -1 if any error) */
double v_ConnectTime()
{ double wallseconds ;

#ifdef WINNT
	{ FILETIME ftKernel2,ftUser2 ;
	  if (GetProcessTimes(GetCurrentProcess(),&gpi->ftCreate,&gpi->ftExit,&ftKernel2,&ftUser2))	/* Only do if call implemented (NT) */
	   { B64INT t1, t2 ;
	     GetSystemTimeAsFileTime(&gpi->ftExit) ;
	     memcpy(&t1,&gpi->ftCreate,sizeof t1) ; memcpy(&t2,&gpi->ftExit,sizeof t2) ;
	     wallseconds = (t2 - t1) / 10000000.0 ;
	   } else { wallseconds = -1 ; } ;
	}
#else
#ifdef UNIX
	{ struct timeval endTime ;
	  gettimeofday (&endTime,NULL) ;
	  wallseconds = ((endTime.tv_sec - gpi->bgnTime.tv_sec)* 1000.0 + (endTime.tv_usec - gpi->bgnTime.tv_usec)/1000.0) / 1000.0 ;
	}
#else
	wallseconds = time(NULL) -  gpi->StartTime ;
#endif
#endif
	return(wallseconds) ;
}

/*	v_CPUTime - Returns cumulative CPU used by V4 process (and its children) as double */

double v_CPUTime()
{ double cpuseconds ;
#ifdef WINNT
	{ FILETIME ftCreate,ftExit,ftKernel2,ftUser2 ;
	  if (GetProcessTimes(GetCurrentProcess(),&ftCreate,&ftExit,&ftKernel2,&ftUser2))
	   { B64INT t1,t2 ;
	     memcpy(&t1,&gpi->ftKernel1,sizeof t1) ; memcpy(&t2,&ftKernel2,sizeof t2) ; cpuseconds = (t2 - t1) ;
	     memcpy(&t1,&gpi->ftUser1,sizeof t1) ; memcpy(&t2,&ftUser2,sizeof t2) ; cpuseconds += (t2 - t1) ; cpuseconds /= 10000000.0 ;
	   } ;
	}
#else
	cpuseconds = ((clock() -  gpi->StartClock)*100.0) / (double)CLOCKS_PER_SEC ;
	cpuseconds /= 100.0 ;
#endif
	return(cpuseconds) ;
}

/*	R A N D O M   N U M B E R   G E N E R A T O R S		*/

//B64INT v_psuedoRandomNum64()
//{ UCCHAR random[48] ;
//#ifdef WINNT
//  FILETIME ftTime ;
//
//	HANGLOOSE(1) ;		/* Just in case we are being called over & over */
//	GetSystemTimeAsFileTime(&ftTime) ;
//	v_Msg(NULL,random,"@%1d%2d%3g%4d",ftTime.dwLowDateTime,gpi->ftKernel1.dwLowDateTime,v_CPUTime(),CURRENTPID) ;
//#endif
//#ifdef UNIX
//  struct timeval curTime ;
//
//	HANGLOOSE(1) ;		/* Just in case we are being called over & over */
//	gettimeofday (&curTime,NULL) ;
//	v_Msg(NULL,random,"@%1d%2d%3g%4d",curTime.tv_usec,gpi->bgnTime.tv_usec,v_CPUTime(),CURRENTPID) ;
//#endif
//	return(v_Hash64(random)) ;
//}

/* 
   A C-program for MT19937-64 (2004/9/29 version).
   Coded by Takuji Nishimura and Makoto Matsumoto.

   This is a 64-bit version of Mersenne Twister pseudorandom number
   generator.

   Copyright (C) 2004, Makoto Matsumoto and Takuji Nishimura, All rights reserved.
   (Extensively modified VEH 20091229)

   References:
   T. Nishimura, ``Tables of 64-bit Mersenne Twisters''
     ACM Transactions on Modeling and 
     Computer Simulation 10. (2000) 348--357.
   M. Matsumoto and T. Nishimura,
     ``Mersenne Twister: a 623-dimensionally equidistributed
       uniform pseudorandom number generator''
     ACM Transactions on Modeling and 
     Computer Simulation 8. (Jan. 1998) 3--30.

   Any feedback is very welcome.
   http://www.math.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove spaces)
*/

#define SVMAX 312
#define SVMAXHALF (SVMAX / 2)
#define MATRIX_A 0xB5026F5AA96619E9ULL
#define MS33BITS 0xFFFFFFFF80000000ULL		/* Most significant 33 bits */
#define LS31BITS 0x7FFFFFFFULL			/* Least significant 31 bits */

static UB64INT stateVector[SVMAX] ;		/* The array for the state vector */
static int svX = UNUSED ; 

/*	vRan64_Seed - Initializes stateVector[SVMAX] with a seed */
void vRan64_Seed(seed)
  UB64INT seed ;
{
    stateVector[0] = seed;
    for (svX=1;svX<SVMAX;svX++) 
     { stateVector[svX] =  (6364136223846793005ULL * (stateVector[svX-1] ^ (stateVector[svX-1] >> 62)) + svX) ; } ;
}

/*	vRan64_ArraySeed - Initialize by an array with array-length */
void vRan64_ArraySeed(seedArray,seedArrayLen)
  UB64INT seedArray[], seedArrayLen ;
{ INDEX i, j, k ;

	vRan64_Seed(19650218ULL);
	i = 1 ; j = 0 ;
	k = (SVMAX>seedArrayLen ? SVMAX : seedArrayLen);
	for (;k>0;k--)
	 { stateVector[i] = (stateVector[i] ^ ((stateVector[i-1] ^ (stateVector[i-1] >> 62)) * 3935559000370003845ULL)) + seedArray[j] + j ;
	   i++ ; j++ ;
	   if (i >= SVMAX) { stateVector[0] = stateVector[SVMAX-1] ; i = 1 ; } ;
	   if (j>=seedArrayLen) j=0;
	 } ;
	for (k=SVMAX-1;k>0; k--)
	 { stateVector[i] = (stateVector[i] ^ ((stateVector[i-1] ^ (stateVector[i-1] >> 62)) * 2862933555777941757ULL)) - i ;
	   i++ ;
	   if (i>=SVMAX) { stateVector[0] = stateVector[SVMAX-1] ; i = 1 ; }
	 } ;
	stateVector[0] = 1ULL << 63 ; /* MSB is 1; assuring non-zero initial array */ 
}

/*	vRan64_RandomU64 - Generates a random number on [0, 2^64-1]-interval */
UB64INT vRan64_RandomU64()
{
  INDEX i ; UB64INT ranRes ;
  static UB64INT mag01[2]={0ULL, MATRIX_A};

	if (svX == UNUSED)			/* If not seeded then do so now */ 
	 { 
#ifdef WINNT
	   B64INT sArray[6] ; COUNTER bits ; INDEX i ;
	   ULARGE_INTEGER avail,total,totfree ;
	   GetDiskFreeSpaceExA("c:\\",&avail,&total,&totfree) ;
	   sArray[0] = time(NULL) ; sArray[1] = clock() ; sArray[2] = CURRENTPID ;
	   sArray[3] = avail.QuadPart ; sArray[4] = total.QuadPart ; sArray[5] = totfree.QuadPart ;
	   bits = GetTickCount() & 0x3f ;		/* Grab low 6 bits of system uptime and use that to rotate (i.e. scramble) other entries */
	   for (i=0;i<bits;i++)
	    { sArray[0] = (sArray[0] < 0 ? (sArray[0] << 1) | 1 : (sArray[0] << 1)) ;
	      sArray[1] = (sArray[1] < 0 ? (sArray[1] << 1) | 1 : (sArray[1] << 1)) ;
	      sArray[2] = (sArray[2] < 0 ? (sArray[2] << 1) | 1 : (sArray[2] << 1)) ;
	      sArray[4] = (sArray[4] < 0 ? (sArray[4] << 1) | 1 : (sArray[4] << 1)) ;
	      sArray[5] = (sArray[5] < 0 ? (sArray[5] << 1) | 1 : (sArray[5] << 1)) ;
	    } ;
	   vRan64_ArraySeed(sArray,6) ;		/* If not seeded then do so now */ 
#else
	   UB64INT sArray[3] ; COUNTER bits ; INDEX i ;
	   sArray[0] = time(NULL) ; sArray[1] = clock() ; sArray[2] = CURRENTPID ;
	   bits = sArray[1] & 0x3f ;		/* Grab low 6 bits of CPU time and use that to rotate (i.e. scramble) other entries */
	   for (i=0;i<bits;i++)
	    { sArray[0] = (sArray[0] < 0 ? (sArray[0] << 1) | 1 : (sArray[0] << 1)) ;
	      sArray[1] = (sArray[1] < 0 ? (sArray[1] << 1) | 1 : (sArray[1] << 1)) ;
	      sArray[2] = (sArray[2] < 0 ? (sArray[2] << 1) | 1 : (sArray[2] << 1)) ;
	    } ;
	   vRan64_ArraySeed(sArray,3) ;		/* If not seeded then do so now */ 
#endif
	 } ;
	if (svX >= SVMAX)
	 {
	   for (i=0;i<SVMAX-SVMAXHALF;i++)
	    { ranRes = (stateVector[i]&MS33BITS)|(stateVector[i+1]&LS31BITS) ;
	      stateVector[i] = stateVector[i+SVMAXHALF] ^ (ranRes>>1) ^ mag01[(int)(ranRes&1ULL)] ;
	    } ;
	   for (;i<SVMAX-1;i++)
	    { ranRes = (stateVector[i]&MS33BITS)|(stateVector[i+1]&LS31BITS) ;
	      stateVector[i] = stateVector[i+(SVMAXHALF-SVMAX)] ^ (ranRes>>1) ^ mag01[(int)(ranRes&1ULL)] ;
	    } ;
	   ranRes = (stateVector[SVMAX-1]&MS33BITS)|(stateVector[0]&LS31BITS) ;
	   stateVector[SVMAX-1] = stateVector[SVMAXHALF-1] ^ (ranRes>>1) ^ mag01[(int)(ranRes&1ULL)] ;
	   svX = 0;
	 } ;
  
	ranRes = stateVector[svX++];
	ranRes ^= (ranRes >> 29) & 0x5555555555555555ULL ; ranRes ^= (ranRes << 17) & 0x71D67FFFEDA60000ULL ;
	ranRes ^= (ranRes << 37) & 0xFFF7EEE000000000ULL ; ranRes ^= (ranRes >> 43) ;
	return(ranRes);
}

/*	v4stat_ran3 - Generates a random number on [0, 2^63-1]-interval */
B64INT v4stat_ran3()
{
	return (B64INT)(vRan64_RandomU64() >> 1);
}

/*	vRan64_RandomDbl - Generates a random number on [0,1]-real-interval (0 <= result <= 1) */
double vRan64_RandomDbl()
{
	return (vRan64_RandomU64() >> 11) * (1.0/9007199254740991.0) ;

/*	Other possible result ranges-
	return (vRan64_RandomU64() >> 11) * (1.0/9007199254740992.0) ;		(0 <= result < 1)
	return ((vRan64_RandomU64() >> 12) + 0.5) * (1.0/4503599627370496.0) ;	(0 < result < 1)
*/
}

/*	C A L E N D A R   M O D U L E S						*/

static double zuluoffset = UNUSED ;

/*	vcal_CalToUDate - Converts Calendar to UDate		*/
int vcal_CalToUDate(cal,timezone,ok)
 double cal ;
 int timezone,*ok ;
{
  int udate ; double frac,intdbl ;

	if (zuluoffset == (double)UNUSED) zuluoffset = -(double)mscu_minutes_west() / (24 * 60) ;

	if (cal == VCal_NullDate) return(0) ;
/*	If we have time portion then convert from Zulu, otherwise take as is */
	frac = modf(cal, &intdbl) ;
	if (frac != 0.0) { cal += (timezone != VCAL_TimeZone_Local ? (double)timezone / 24.0 : (double)zuluoffset) ; } ;
	udate = (int)cal - VCal_MJDOffset ;
	if (udate > 0 && udate < VCAL_UDateMax) { if (ok != NULL) *ok = TRUE ; }
	 else { udate = 0 ; if (ok != NULL) *ok = FALSE ; } ;
	return(udate) ;
}

/*	vcal_CalToUDT - Converts Calendar to DateTime		*/
/*	Returns -1 if Cal is out-of-range for UDT point		*/

int vcal_CalToUDT(cal,timezone,havetime)
 double cal ;
 int timezone,*havetime ;
{
  int udt ; double frac,intdbl ;

	if (zuluoffset == (double)UNUSED) zuluoffset = -(double)mscu_minutes_west() / (24 * 60) ;
	*havetime = FALSE ;
	if (cal == VCal_NullDate) return(0) ;
	if (cal - VCal_MJDOffset <= 0 || cal - VCal_MJDOffset > VCAL_UDateMax) return(-1) ; 
/*	If we have time portion then convert from Zulu, otherwise take as is */
	frac = modf(cal, &intdbl) ;
	if (frac != 0.0) { cal += (timezone != VCAL_TimeZone_Local ? (double)timezone / 24.0 : (double)zuluoffset) ; *havetime = TRUE ; } ;
	frac = modf(cal, &intdbl) ;
	udt = (int)(cal - VCal_MJDOffset - VCAL_UDTUDateOffset) * VCAL_SecsInDay + (int)((frac * (24.0*60.0*60.0)) + 0.5) ;
	return(udt) ;
}

/*	F O R M A T T I N G   M O D U L E S

/*	v_FormatLInt - Formats long integer via Excel format string (e.g. "#,##0")
	Call: length = v_FormatLInt(num , decimals , format , dst)
	  where length is length of final string,
		num is source integer to be formated,
		decimals is implied decimals,
		format is format string,
		dst is pointer to destination (assumed max of < 1000 bytes)	*/

int v_FormatLInt(num,decimals,format,dst)
  B64INT num ; int decimals ; UCCHAR *format, *dst ;
{ struct V4CI__CountryInfo *ci ; UCCHAR dpnt ;
  struct V4LI_LanguageInfo *li ;
  UCCHAR *b,delim,*sign,*ordinal,stack[100] ; UCCHAR *pos4mat,*neg4mat,*zip4mat,prefix[64],suffix[64],tformat[256] ;
  int i,j,s,t,zeros,neg,base,word,dps,fraction,scale,curx,digits ;

	ci = gpi->ci ;
//	ni = ci->Cntry[ci->CurX].NumInfo ;			/* ni[0] = currency, ni[1] = delim, ni[2] = decimal pt */
/*	If no format OR it begins with '%' then use C formatting */
	if (format == NULL)
	 { if (UCnotempty(gpi->formatMasks[V4DPI_PntType_Fixed])) format = gpi->formatMasks[V4DPI_PntType_Fixed] ;
	 } ;
	if (format == NULL ? TRUE : format[0] == UClit('%'))
	 { UCsprintf(dst,100,(format == NULL ? UClit("%ld") : format),num) ; return(UCstrlen(dst)) ; } ;
	if (num < 0) { num = -num ; neg = TRUE ; } else { neg = FALSE ; } ;
	if (decimals == 0) { fraction = 0 ; scale = 0 ; }
	 else { scale = 10 ; for(i=1;i<decimals;i++) { scale *= 10 ; } ;
		fraction = num % scale ; num /= scale ;
	      } ;
/*	Parse format string */
	zip4mat = NULL ; neg4mat = NULL ; pos4mat = format ;
	digits = FALSE ; ZUS(prefix) ; ZUS(suffix) ;
	b = UCstrchr(format,';') ;
	if (b != NULL)
	 { UCstrcpy(tformat,format) ; format = tformat ; b = UCstrchr(format,';') ; pos4mat = format ;
	   *b = UCEOS ; if (*(b+1) != UCEOS) neg4mat = b + 1 ;
//	   b = UCstrchr(b+1,';') ; if (b != NULL) { *b = UCEOS ; if (*(b+1) != UCEOS) zip4mat = b + 1 ; } ;
	   b = UCstrchr(b+1,';') ; if (b != NULL) { *b = UCEOS ; if (*(b+1) != UCEOS) { zip4mat = b + 1 ; } else { zip4mat = UClit("") ; } } ;
	 } ;
	if (num == 0 && zip4mat != NULL) { UCstrcpy(dst,zip4mat) ; return(UCstrlen(zip4mat)) ; } ;
	format = ((neg && neg4mat != NULL) ? neg4mat : pos4mat) ;
	sign = (neg4mat == NULL ? UClit("-") : NULL) ;
	delim = UCEOS ; base = 10 ; ordinal = NULL ; word = 0 ; dps = UNUSED ; dpnt = UClit('.') ;
	for(i=0,zeros=0;format[i]!=UCEOS;i++)
	 { switch (format[i])
	    { default:			b = (digits ? &suffix[UCstrlen(suffix)] : &prefix[UCstrlen(prefix)]) ;
					*b = format[i] ; *(b+1) = UCEOS ; break ;
	      case UClit('#'):		digits = TRUE ; if (dps != UNUSED) dps++ ; break ;
	      case UClit(','):		if (ci->Cntry[ci->CurX].DigiDelim == UClit(',')) { dps = 0 ; dpnt = UClit(',') ; break ; } ;
					delim = UClit(',') ; break ;
	      case UClit('_'):		delim = ci->Cntry[ci->CurX].DigiDelim ; break ;
	      case UClit('.'):		if (ci->Cntry[ci->CurX].DigiDelim == UClit('.')) { delim = UClit('.') ; break ; } ;
					dpnt = UClit('.') ; dps = 0 ; break ;
	      case UClit('!'):		dps = 0 ; dpnt = ci->Cntry[ci->CurX].RadixPnt ; break ;
	      case UClit('0'):		digits = TRUE ; if (dps != UNUSED) { dps++ ; } else { zeros++ ; } ; break ;
	      case UClit(')'):
	      case UClit('('):		sign = UClit("(") ; break ;
	      case UClit('x'):		digits = TRUE ; if (neg) { neg = FALSE ; num = -num ; } ; zeros++ ; base = 16 ; break ;
	      case UClit('X'):		digits = TRUE ; if (neg) { neg = FALSE ; num = -num ; } ; zeros++ ; base = 17 ; break ;
	      case UClit('o'):		digits = TRUE ; if (neg) { neg = FALSE ; num = -num ; } ; zeros++ ; base = 8 ; break ;
	      case UClit('b'):		digits = TRUE ; if (neg) { neg = FALSE ; num = -num ; } ; zeros++ ; base = 2 ; break ;
	      case UClit('t'):
	      case UClit('h'):
//		; for(curx=0;curx<li->Count;curx++) { if (li->LI[curx].Language == ci->Cntry[ci->CurX].Language) break ; } ;
//		if (curx >= li->Count) curx = 0 ;
		li = ci->li ; curx = ci->li->CurX ;			/* VEH060412 - pull current language index */
		ordinal = li->LI[curx].OrdSfx[num % 10] ; break ;
	      case UClit('w'):		word = 1 ; break ;
	    } ;
	 } ;
	if (dps == 0)
	 { dps = decimals ;			/* Set to number of decimals of original number */
	   if (dps == 0) dps = UNUSED ;		/* If no decimals then deactivate decimal point */
	 } else if (dps > 0)
	 { if (dps > V4DPI_Fixed_MaxDecimals) dps = V4DPI_Fixed_MaxDecimals ;
	   if (dps < decimals)			/* Wants fewer decimals - have to round */
	    { for(i=10,j=1;j<decimals-dps;j++) i *= 10 ;
	      fraction += i / 2 ; fraction /= i ;
	    } else if (dps > decimals)		/* Wants more decimals - scale */
	    { for(i=1;i<dps-decimals;i++) fraction *= 10 ;
	    } ;
	 } else					/* Wants no decimals - round */
	 { if (fraction > 0 && fraction >= scale / 2) num ++ ;
	 } ;
	dst[s=0] = UCEOS ;
	if (word > 0)
	 { int inum=DtoI(num) ;
//	   for(curx=0;curx<li->Count;curx++) { if (li->LI[curx].Language == ci->Cntry[ci->CurX].Language) break ; } ;
//	   if (curx >= li->Count) curx = 0 ;
	   li = ci->li ; curx = ci->li->CurX ;			/* VEH060412 - pull current language index */
	   if (num == 0)  v_HundredsToText(inum,dst,li,curx) ;
	   if (num > 1000000000) { v_HundredsToText((inum/1000000000),dst,li,curx) ; UCstrcat(dst,UClit(" ")) ; UCstrcat(dst,li->LI[curx].Num000[3]) ; UCstrcat(dst,UClit(" ")) ; num %= 1000000000 ; } ;
	   if (num > 1000000) { v_HundredsToText((inum/1000000),dst,li,curx) ; UCstrcat(dst,UClit(" ")) ; UCstrcat(dst,li->LI[curx].Num000[2]) ; UCstrcat(dst,UClit(" ")) ; num %= 1000000 ; } ;
	   if (num > 1000) { v_HundredsToText((inum/1000),dst,li,curx) ; UCstrcat(dst,UClit(" ")) ; UCstrcat(dst,li->LI[curx].Num000[1]) ; UCstrcat(dst,UClit(" ")) ; num %= 1000 ; } ;
	   if (num > 0) v_HundredsToText(inum,dst,li,curx) ;
	   i = UCstrlen(dst) ; if (dst[i-1] == UClit(' ')) dst[i-1] = UCEOS ;
	   return(i) ;
	 } ;
	if (num == 0)
	 { if (ordinal != NULL && zeros == 0) zeros = 1 ;
//	   for(i=0;i<zeros;i++) { UCstrcat(dst,UClit("0")) ; } ;
//	   if (ordinal != NULL) { for(i=0;ordinal[i]!=UCEOS;i++) { dst[s++] = ordinal[i] ; } ; } ;
//	   return(s) ;
	 } ;
	switch (base)
	 { case 2:
		for(i=0,j=0;num != 0 && i<sizeof stack;)
		 { t = num & 0x1 ; num = (0x7fffffff & num >> 1) ; stack[i++] = UClit("01")[t] ;
		 } ;
		break ;
	   case 8:
		for(i=0,j=0;num != 0 && i<sizeof stack;)
		 { t = num & 0x7 ; num = (0x1fffffff & num >> 3) ; stack[i++] = UClit("0123456789abcdef")[t] ;
		 } ;
		break ;
	   case 10:
		for(i=0,j=0;num != 0 && i<sizeof stack;)
		 { t = num % base ; num /= base ; stack[i++] = (t <= 9 ? t + UClit('0') : t - 10 + UClit('A')) ;
		   j++ ;
		   if (delim != UCEOS && ci->curDDMap[j] && num > 0) { stack[i++] = delim ; } ;
		 } ;
		break ;
	   case 16:
		for(i=0,j=0;num != 0 && i<sizeof stack;)
		 { t = num & 0xf ; num = (0x0fffffff & num >> 4) ; stack[i++] = UClit("0123456789abcdef")[t] ;
		 } ;
		break ;
	   case 17:
		for(i=0,j=0;num != 0 && i<sizeof stack;)
		 { t = num & 0xf ; num = (0x0fffffff & num >> 4) ; stack[i++] = UClit("0123456789ABCDEF")[t] ;
		 } ;
		break ;
	 } ;
	for(j=0;prefix[j]!=UCEOS;j++) { dst[s++] = prefix[j] ; if (zeros) zeros++ ; } ;
	for(;s+i<zeros;) { dst[s++] = UClit('0') ; } ;
	if (neg && sign != NULL) dst[s++] = *sign ;
	for(;i>0;) { dst[s++] = stack[--i] ; } ;
	if (sign == NULL ? FALSE : *sign == UClit('(')) { dst[s++] = (neg ? UClit(')') : UClit(' ')) ; } ;
	if (ordinal != NULL) { for(i=0;ordinal[i]!=UCEOS;i++) { dst[s++] = ordinal[i] ; } ; } ;
	if (dps != UNUSED)
	 { dst[s++] = dpnt ;
	   for(i=0;fraction != 0 && i < sizeof stack;)
	    { t = fraction % base ; fraction /= base ; stack[i++] = (t <= 9 ? t + UClit('0') : t - 10 + UClit('A')) ; } ;
	   for(;dps>0;dps--) { dst[s++] = (i <= 0 ? UClit('0') : stack[--i]) ; } ;
	 } ;
	for(j=0;suffix[j]!=UCEOS;j++) { dst[s++] = suffix[j] ; } ;
	dst[s] = UCEOS ;
	return(s) ;
}
/*	v_FormatInt - Formats integer via Excel format string (e.g. "#,##0")
	Call: length = v_FormatInt(num , format , dst )
	  where length is the bytes in the formatted string (numbers take as many positions as necessary),
		num is source integer to be formated,
		format is format string,
		dst is pointer to destination (assumed max of < 1000 bytes),
		rgbColor, if not NULL, is updated with any format color information	*/

int v_FormatInt(num,format,dst,rgbColor)
  int num ; UCCHAR *format, *dst, *rgbColor ;
{ struct V4CI__CountryInfo *ci ; UCCHAR dpnt ;
  struct V4LI_LanguageInfo *li ;
  UCCHAR *b,delim,*sign,*ordinal,stack[100] ; UCCHAR *pos4mat,*neg4mat,*zip4mat,prefix[64],suffix[64] ;
  UCCHAR color[128],tformat[256] ;
  int i,j,s,t,zeros,neg,base,word,dps,curx,digits ;

	ci = gpi->ci ;
//	ni = ci->Cntry[ci->CurX].NumInfo ;			/* ni[0] = currency, ni[1] = delim, ni[2] = decimal pt */
/*	If no format OR it begins with '%' then use C formatting */
	if (format == NULL)
	 { if (UCnotempty(gpi->formatMasks[V4DPI_PntType_Int])) format = gpi->formatMasks[V4DPI_PntType_Int] ;
	 } ;
	if (format == NULL ? TRUE : format[0] == UClit('%'))
	 { UCsprintf(dst,100,(format == NULL ? UClit("%d") : format),num) ; return(UCstrlen(dst)) ; } ;
	if (num < 0)
	 { if (num == 0x80000000) return(v_FormatLInt((B64INT)num,0,format,dst)) ;
	   num = -num ; neg = TRUE ;
	 } else { neg = FALSE ; } ;
/*	Parse format string */
	zip4mat = NULL ; neg4mat = NULL ; pos4mat = format ;
	digits = FALSE ; ZUS(prefix) ; ZUS(suffix) ; ZUS(color) ;
	b = UCstrchr(format,';') ;
	if (b != NULL)
	 { UCstrcpy(tformat,format) ; format = tformat ; b = UCstrchr(format,';') ; pos4mat = format ;
	   *b = UCEOS ; if (*(b+1) != UCEOS) neg4mat = b + 1 ;
//	   b = UCstrchr(b+1,';') ; if (b != NULL) { *b = UCEOS ; if (*(b+1) != UCEOS) zip4mat = b + 1 ; } ;
	   b = UCstrchr(b+1,';') ; if (b != NULL) { *b = UCEOS ; if (*(b+1) != UCEOS) { zip4mat = b + 1 ; } else { zip4mat = UClit("") ; } } ;
	 } ;
	if (num == 0 && zip4mat != NULL) { UCstrcpy(dst,zip4mat) ; return(UCstrlen(zip4mat)) ; } ;
	format = ((neg && neg4mat != NULL) ? neg4mat : pos4mat) ;
	sign = (neg4mat == NULL ? UClit("-") : NULL) ;
	delim = UCEOS ; base = 10 ; ordinal = NULL ; word = 0 ; dps = UNUSED ; dpnt = UClit('.') ;
	for(i=0,zeros=0;format[i]!=UCEOS;i++)
	 { switch (format[i])
	    { default:			b = (digits ? &suffix[UCstrlen(suffix)] : &prefix[UCstrlen(prefix)]) ;
					*b = format[i] ; *(b+1) = UCEOS ;
					break ;
	      case UClit('#'):		digits = TRUE ; if (dps != UNUSED) dps++ ; break ;
	      case UClit(','):		if (ci->Cntry[ci->CurX].RadixPnt == UClit(',')) { dps = 0 ; dpnt = UClit(',') ; break ; } ;
					delim = UClit(',') ; break ;
	      case UClit('_'):		delim = ci->Cntry[ci->CurX].DigiDelim ; break ;
	      case UClit('.'):		if (ci->Cntry[ci->CurX].DigiDelim == UClit('.')) { delim = UClit('.') ; break ; } ;
					dpnt = UClit('.') ; dps = 0 ; break ;
	      case UClit('!'):		dps = 0 ; dpnt = ci->Cntry[ci->CurX].RadixPnt ; break ;
	      case UClit('0'):		digits = TRUE ; if (dps != UNUSED) { dps++ ; } else { zeros++ ; } ; break ;
	      case UClit(')'):
	      case UClit('('):		sign = UClit("(") ; break ;
	      case UClit('x'):		digits = TRUE ; if (neg) { neg = FALSE ; num = -num ; } ; zeros++ ; base = 16 ; break ;
	      case UClit('X'):		digits = TRUE ; if (neg) { neg = FALSE ; num = -num ; } ; zeros++ ; base = 17 ; break ;
	      case UClit('o'):		digits = TRUE ; if (neg) { neg = FALSE ; num = -num ; } ; zeros++ ; base = 8 ; break ;
	      case UClit('b'):		digits = TRUE ; if (neg) { neg = FALSE ; num = -num ; } ; zeros++ ; base = 2 ; break ;
	      case UClit('t'):
	      case UClit('h'):
//		for(curx=0;curx<li->Count;curx++) { if (li->LI[curx].Language == ci->Cntry[ci->CurX].Language) break ; } ;
//		if (curx >= li->Count) curx = 0 ;
		li = ci->li ; curx = ci->li->CurX ;			/* VEH060412 - pull current language index */
		ordinal = li->LI[curx].OrdSfx[num % 10] ; break ;
	      case UClit('w'):		word = 1 ; break ;
	      case UClit('['):		for(j=0,i++;j<UCsizeof(color)&&format[i]!=UClit(']');j++,i++) { color[j] = format[i] ; } ;
					color[j] = UCEOS ; break ;
	    } ;
	 } ;
	if (rgbColor != NULL)
	 { if (UCempty(color)) { ZUS(rgbColor) ; }
	    else { j = v_ColorNameToRef(color) ; v_Msg(NULL,rgbColor,"@#%1x",v_ColorRefToRGB(j)) ; } ;
	 } ;
	dst[s=0] = UCEOS ;
	if (word > 0)
	 { 
//	   for(curx=0;curx<li->Count;curx++) { if (li->LI[curx].Language == ci->Cntry[ci->CurX].Language) break ; } ;
//	   if (curx >= li->Count) curx = 0 ;
	   li = ci->li ; curx = ci->li->CurX ;			/* VEH060412 - pull current language index */
	   if (num == 0)  v_HundredsToText(num,dst,li,curx) ;
	   if (num > 1000000000) { v_HundredsToText(num/1000000000,dst,li,curx) ; UCstrcat(dst,UClit(" ")) ; UCstrcat(dst,li->LI[curx].Num000[3]) ; UCstrcat(dst,UClit(" ")) ; num %= 1000000000 ; } ;
	   if (num > 1000000) { v_HundredsToText(num/1000000,dst,li,curx) ; UCstrcat(dst,UClit(" ")) ; UCstrcat(dst,li->LI[curx].Num000[2]) ; UCstrcat(dst,UClit(" ")) ; num %= 1000000 ; } ;
	   if (num > 1000) { v_HundredsToText(num/1000,dst,li,curx) ; UCstrcat(dst,UClit(" ")) ; UCstrcat(dst,li->LI[curx].Num000[1]) ; UCstrcat(dst,UClit(" ")) ; num %= 1000 ; } ;
	   if (num > 0) v_HundredsToText(num,dst,li,curx) ;
	   i = UCstrlen(dst) ; if (dst[i-1] == UClit(' ')) dst[i-1] = UCEOS ;
	   return(i) ;
	 } ;
	if (num == 0)
	 { if (ordinal != NULL && zeros == 0) zeros = 1 ;
//	   for(i=0;i<zeros;i++) { UCstrcat(dst,UClit("0")) ; } ;
//	   if (ordinal != NULL) { for(i=0;ordinal[i]!=UCEOS;i++) { dst[s++] = ordinal[i] ; } ; } ;
//	   return(s) ;
	 } ;
	switch (base)
	 { case 2:
		for(i=0,j=0;num != 0 && i<sizeof stack;)
		 { t = num & 0x1 ; num = (0x7fffffff & num >> 1) ; stack[i++] = UClit("01")[t] ;
		 } ;
		break ;
	   case 8:
		for(i=0,j=0;num != 0 && i<sizeof stack;)
		 { t = num & 0x7 ; num = (0x1fffffff & num >> 3) ; stack[i++] = UClit("012345678")[t] ;
		 } ;
		break ;
	   case 10:
		for(i=0,j=0;num != 0 && i<sizeof stack;)
		 { t = num % base ; num /= base ; stack[i++] = (t <= 9 ? t + UClit('0') : t - 10 + UClit('A')) ;
		   j++ ;
		   if (delim != UCEOS && ci->curDDMap[j] && num > 0) { stack[i++] = delim ; } ;
		 } ;
		break ;
	   case 16:
		for(i=0,j=0;num != 0 && i<sizeof stack;)
		 { t = num & 0xf ; num = (0x0fffffff & num >> 4) ; stack[i++] = UClit("0123456789abcdef")[t] ;
		 } ;
		break ;
	   case 17:
		for(i=0,j=0;num != 0 && i<sizeof stack;)
		 { t = num & 0xf ; num = (0x0fffffff & num >> 4) ; stack[i++] = UClit("0123456789ABCDEF")[t] ;
		 } ;
		break ;
	 } ;
	for(j=0;prefix[j]!=UCEOS;j++) { dst[s++] = prefix[j] ; if (zeros) zeros++ ; } ;
	if (neg && sign != NULL) dst[s++] = *sign ;
	for(;s+i<zeros;) { dst[s++] = UClit('0') ; } ;
	for(;i>0;) { dst[s++] = stack[--i] ; } ;
	if (sign == NULL ? FALSE : *sign == UClit('(')) { dst[s++] = (neg ? UClit(')') : UClit(' ')) ; } ;
	if (ordinal != NULL) { for(i=0;ordinal[i]!=UCEOS;i++) { dst[s++] = ordinal[i] ; } ; } ;
	if (dps != UNUSED) { dst[s++] = dpnt ; for(;dps>0;dps--) { dst[s++] = UClit('0') ; } ; } ;
	for(j=0;suffix[j]!=UCEOS;j++) { dst[s++] = suffix[j] ; } ;
	dst[s] = UCEOS ;
	return(s) ;
}

LOGICAL v_HundredsToText(num,dst,li,curx)
  int num,curx ;
  UCCHAR *dst ;
  struct V4LI_LanguageInfo *li ;
{  int onum,space ;

	onum = num ; space = FALSE ;
	if (num < 0 || num > 999) return(FALSE) ;
	if (num == 0) { UCstrcat(dst,li->LI[curx].Num120[0]) ; return(TRUE) ; } ;
	if (num > 100) { UCstrcat(dst,li->LI[curx].Num120[num/100]) ; UCstrcat(dst,UClit(" ")) ; UCstrcat(dst,li->LI[curx].Num000[0]) ; num %= 100 ; space = TRUE ; } ;
	if (num > 19) { if (space) UCstrcat(dst,UClit(" ")) ; UCstrcat(dst,li->LI[curx].Tens2090[(num/10)-2]) ; num %= 10 ; space = TRUE ; } ;
	if (num > 0 && num <= 19) { if (space) UCstrcat(dst,UClit(" ")) ; UCstrcat(dst,li->LI[curx].Num120[num]) ; } ;
	return(TRUE) ;
}

/*	v_ParseInt - Routines to parse doubles/integers (ignore commas, spaces, dollar signs)	*/
/*	Call: int = v_ParseInt(ptr,term)
	  where int is returned integer (only valid if *term = UCEOS),
		ptr is pointer to string,
		term is (UCCHAR **) updated to pointer to first non-numeric character		*/

int v_ParseInt(ptr,term,base)
  UCCHAR *ptr ;
  UCCHAR **term ;
  int base ;
{ int result = 0 ; LOGICAL isNeg = FALSE ;

	for(;;ptr++)
	 { switch (*ptr)
	    { default:
		goto done ;
	      case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'):
	      case UClit('5'): case UClit('6'): case UClit('7'): case UClit('8'): case UClit('9'):
		if ((*ptr) - UClit('0') >= base)  goto done ;
		result = (result * base) + ((*ptr) - UClit('0')) ; break ;
	      case UClit('a'): case UClit('b'): case UClit('c'): case UClit('d'): case UClit('e'): case UClit('f'):
	      case UClit('A'): case UClit('B'): case UClit('C'): case UClit('D'): case UClit('E'): case UClit('F'):
		if ((UCTOUPPER(*ptr)) - UClit('A') + 10 >= base)  goto done ;
		result = (result * base) + ((*ptr) - UClit('0') + 10) ; break ;
	      case UClit('-'):
		if (isNeg) goto done ;
		isNeg = TRUE ; break ;
	      case UClit(','):
	      case UClit(' '):
	      case UClit('$'):
		break ;
	    } ;
	 } ;
done:
	*term = ptr ; return(isNeg ? -result : result) ;
}

/*	v_ParseLog - Routines to parse logical values	*/
/*	Call: log = v_ParseLog(ptr,term)
	  where logical is returned integer (TRUE/FALSE) (only valid if *term = UCEOS),
		ptr is pointer to string,
		term is (UCCHAR **) updated to pointer to first non-numeric character		*/

LOGICAL v_ParseLog(ptr,term)
  UCCHAR *ptr ;
  UCCHAR **term ;
{
  UCCHAR ucbuf[32] ; INDEX i ;

	i = v_ParseInt(ptr,term,10) ;
	if (**term == UCEOS) return(i > 0) ;	/* If we got an integer then anything > 0 is TRUE */
/*	Not an integer - check for language-specific keywords */
	for(i=0;i<UCsizeof(ucbuf);i++) { ucbuf[i] = UCTOUPPER(ptr[i]) ; if (ptr[i] == UCEOS) break ; } ;
	*term = &ptr[i] ;
	if (UCstrncmp(ucbuf,gpi->ci->li->LI[gpi->ci->Cntry[gpi->ci->CurX].Language].YesUC,i) == 0) return(TRUE) ;
	if (UCstrncmp(ucbuf,gpi->ci->li->LI[gpi->ci->Cntry[gpi->ci->CurX].Language].TrueUC,i) == 0) return(TRUE) ;
	if (UCstrncmp(ucbuf,gpi->ci->li->LI[gpi->ci->Cntry[gpi->ci->CurX].Language].NoUC,i) == 0) return(FALSE) ;
	if (UCstrncmp(ucbuf,gpi->ci->li->LI[gpi->ci->Cntry[gpi->ci->CurX].Language].FalseUC,i) == 0) return(FALSE) ;
/*	Don't know what it is, return FALSE (but don't advance term) */
	*term = ptr ; return(FALSE) ;
}


/*	v_FormatDate - Formats Date/Calendar/UDT,UMonth via Excel format string (e.g. "#,##0") */

#define FD_CompMax 50
#define FD_Month 1
#define FD_Year 2
#define FD_Day 3
#define FD_WeekDay 4
#define FD_Julian 5
#define FD_Punc 6
#define FD_Hour 7
#define FD_Minute 8
#define FD_Second 9
#define FD_Zone 10
#define FD_Period 11
#define FD_Quarter 12
#define FD_Hour12 13
#define FD_AMPM 14
#define FD_Week 15
#define FD_FracSecond 16

#define FD_CntryCode 11
#define FD_AreaCode 12
#define FD_Number 13
#define FD_Extension 14
#define FD_Type 15

#define FDE(TYPE) if (type != TYPE) fd.Type[fd.Count++] = TYPE ; break ;
#define FDUC(TYPE) if (type != TYPE) fd.Type[fd.Count++] = TYPE ; fd.UpperCase[fd.Count-1] = TRUE ; break ;

/*	v_FormatDate - Formats all of the Date-Time point types in V4 to UCCHAR string */
/*	Call: chars = v_FormatDate( vnum , pnttype , calendar , timezone , format, dst )
	  where chars = number of characters in formatted string,
		vnum is POINTER to date-time value,
		pnttype is V4DPI_PntType_xxx (NOTE: for UPeriod use periodsInYear<<16+pnttype - i.e. pnttype is masked via (pnttype&0xffff)),
		calendar is calendar (VCAL_CalType_xxx), use VCAL_CalType_UseDeflt for default,
		timezone is timezone to adjust to (+/- GMT) use VCAL_TimeZone_Local for system local timezone,
		format is UCCHAR format mask (NULL for default for pnttype),
		dst is where formatted string to be placed - MUST BE BIG ENOUGH		*/

int v_FormatDate(vnum,pnttype,calendar,timezone,format,dst)
  void *vnum ;
  int pnttype,calendar,timezone ; UCCHAR *format ; UCCHAR *dst ;
{ struct V4CI__CountryInfo *ci ;
  static double zuluoffset = UNUSED ;
  struct V4LI_CalendarInfo *cli ;
  struct V4Cal__Args cal ;
  UCCHAR *zip4mat, *ucb, tformat[256] ;
  int *num,i,j,type,dx,ok,curx,isISO ; UCCHAR tbuf[256] ;
  int y,m,d,d1,ww,hh,mm,ss,fixdate,qtr,period,specfss ; double *dp,dnum,frac,ddate,intdbl,fss ;
  struct {
    int Count ;			/* Number of components in date */
    int Type[FD_CompMax+1] ;	/* Type of component */
    int Length[FD_CompMax+1] ;
    int ZeroFill[FD_CompMax+1] ;
    int UpperCase[FD_CompMax+1] ;
   } fd ;

	num = (int *)vnum ;
	ci = gpi->ci ; cli = ci->cli ;
	if (calendar == VCAL_CalType_UseDeflt) calendar = ci->Cntry[ci->CurX].Calendar ;
	for(curx=0;curx<cli->Count;curx++) { if (cli->CLI[curx].Language == ci->li->CurX && cli->CLI[curx].Calendar == calendar) break ; } ;
	if (curx >= cli->Count) curx = 0 ;
	if (format == NULL)
	 { if (UCnotempty(gpi->formatMasks[pnttype&0xff])) format = gpi->formatMasks[pnttype&0xff] ;
	 } ;
	if (format == NULL)		/* If no format then pull from Country/Language Info structures */
	 { switch(pnttype&0xff)
	    { case V4DPI_PntType_Calendar:	dp = (double *)num ; dnum = *dp ;
						frac = modf(dnum,&intdbl) ;
						format = (frac != 0.0 ? ci->Cntry[ci->CurX].DateTimeMask : ci->Cntry[ci->CurX].DateMask) ; break ;
	      case V4DPI_PntType_UDate:		format = ci->Cntry[ci->CurX].DateMask ; break ;
	      case V4DPI_PntType_UMonth:	format = ci->Cntry[ci->CurX].MonthMask ; break ;
	      case V4DPI_PntType_UDT:		format = ci->Cntry[ci->CurX].DateTimeMask ; break ;
	      case V4DPI_PntType_UTime:		format = UClit("0h:0n:0s") ; break ;
	      case V4DPI_PntType_UPeriod:	format = UClit("yyyy/0p") ; break ;
	      case V4DPI_PntType_UQuarter:	format = UClit("yyyyQq") ; break ;
	      case V4DPI_PntType_UWeek:		format = UClit("yyyy0k") ; break ;
	      case V4DPI_PntType_UYear:		format = UClit("yyyy") ; break ;
	    } ;
	 } ;
/*	Parse format string */
	zip4mat = gpi->ci->li->LI[gpi->ci->li->CurX].None ;
	ucb = UCstrchr(format,';') ;
	if (ucb != NULL)
	 { UCstrcpy(tformat,format) ; format = tformat ; ucb = UCstrchr(format,';') ; *ucb = UCEOS ; zip4mat = ucb + 1 ; } ;
	memset(&fd,0,sizeof fd) ; specfss = FALSE ;
	for(i=0;format[i]!='\0';i++)
	 { type = (fd.Count == 0 ? UNUSED : fd.Type[fd.Count-1]) ;
	   if (fd.Count >= FD_CompMax) break ;
	   switch (format[i]) 
	    { default:	fd.Length[fd.Count] = format[i] ; fd.Type[fd.Count++] = FD_Punc ; continue ;
	      case '"':
		for(i++,ok=TRUE;ok;i++)
		 { switch(format[i])
		    { case UClit('"'):	i-- ; ok = FALSE ; break ;	
		      case UCEOS:	i-- ; ok = FALSE ; break ;
		      default:		if (fd.Count >= FD_CompMax) { ok = FALSE ; break ; } ;
					if (format[i] != '\\') { fd.Length[fd.Count] = format[i] ; }
					 else { switch (format[i+1])
						 { default:		fd.Length[fd.Count] = format[i+1] ; break ;
						   case UClit('n'):	fd.Length[fd.Count] = UCNL ; break ;
//						   case UClit('l'):	fd.Length[fd.Count] = UClit('\l') ; break ;
						 } ; i++ ;
					      } ;
					fd.Type[fd.Count++] = FD_Punc ; continue ;
		    } ;
		 } ; continue ;
	      case UClit('0'): fd.ZeroFill[fd.Count] = TRUE ; fd.Length[fd.Count]++ ;  continue ;	/* Set "next" field to zero fill */
	      case UClit('M'): FDUC(FD_Month)
	      case UClit('m'):	FDE(FD_Month)
	      case UClit('d'): FDE(FD_Day)
	      case UClit('y'): FDE(FD_Year)
	      case UClit('W'): FDUC(FD_WeekDay)
	      case UClit('w'): FDE(FD_WeekDay)
	      case UClit('j'): FDE(FD_Julian)
	      case UClit('H'): FDE(FD_Hour12)
	      case UClit('h'):	FDE(FD_Hour)
	      case UClit('S'):
	      case UClit('s'):	FDE(FD_Second)
	      case UClit('F'):
	      case UClit('f'):	specfss = TRUE ; FDE(FD_FracSecond)
	      case UClit('N'):
	      case UClit('n'):	FDE(FD_Minute)
	      case UClit('q'): FDE(FD_Quarter)
	      case UClit('p'): FDE(FD_Period)
	      case UClit('z'):	FDE(FD_Zone)
	      case UClit('Z'):	FDUC(FD_Zone)
	      case UClit('A'): FDUC(FD_AMPM)
	      case UClit('a'): FDE(FD_AMPM)
	      case UClit('k'): FDE(FD_Week)
	    } ; fd.Length[fd.Count-1]++ ;
	 } ;

	isISO = FALSE ; fss = 0.0 ;
	switch (pnttype&0xffff)
	 { default:	UCsprintf(dst,30,UClit("?%d-%d"),pnttype,*num) ; break ;
	   case V4DPI_PntType_UDate:
		if (*num == VCAL_UDate_None) { UCstrcpy(dst,zip4mat) ; return(UCstrlen(dst)) ; } ;
/*		VEH031218- If negative date then convert to Jan -n, 1800 for compat with other databases (e.g. SQL) */
		if (*num < 0)
		 { y = 1800 ; m = 1 ; d = - *num ; ww = 0 ; hh = 0 ; mm = 0 ; ss = 0 ; break ; } ;
		ddate = *num + VCal_MJDOffset ; num = (int *)&ddate ; goto cal_entry ;
//		i = mscu_udate_to_yyyymmdd(*num) ;
//		y = i / 10000 ; m = (i % 10000) / 100 ; d = i % 100 ;
//		hh = 0 ; mm = 0 ; ss = 0 ; break ;
	   case V4DPI_PntType_UMonth:
		if (calendar != VCAL_CalType_Gregorian)	/* UMonth must be Gregorian Calendar - cannot change to others */
		 { calendar = VCAL_CalType_Gregorian ;
		   for(curx=0;curx<cli->Count;curx++) { if (cli->CLI[curx].Language == ci->Cntry[ci->CurX].Language && cli->CLI[curx].Calendar == calendar) break ; } ;
		 } ;
		if (*num == VCAL_UMonth_None) { UCstrcpy(dst,zip4mat) ; return(UCstrlen(dst)) ; } ;
		if (*num <= 0)
		 { y = 1800 + (- *num - 1) / 12 ; m = ((- *num - 1) % 12) + 1 ; d = 0 ; ww = 0 ; hh = 0 ; mm = 0 ; ss = 0 ; break ; } ;
		y = (*num / 12) + VCAL_BaseYear ; m = (*num%12)+1 ; d = 1 ;
		fixdate = FixedFromGregorian(y,m,d) ;
		hh = 0 ; mm = 0 ; ss = 0 ; break ;
	   case V4DPI_PntType_Calendar:
cal_entry:
		if (zuluoffset == (double)UNUSED) zuluoffset = -(double)mscu_minutes_west() / (24 * 60) ;
		dp = (double *)num ; dnum = *dp ;
		if (dnum == VCal_NullDate) { UCstrcpy(dst,zip4mat) ; return(UCstrlen(dst)) ; } ;
/*		If we have time portion then convert from Zulu, otherwise take as is */
		frac = modf(dnum,&intdbl) ;
		if (frac != 0.0) { dnum += (timezone != VCAL_TimeZone_Local ? (double)timezone / 24.0 : zuluoffset) ; frac = modf(dnum,&intdbl) ; ; } ;
		fixdate = (int)dnum ;
		if (frac == 0.0) { hh = (mm = (ss = 0)) ; }
		 else { i = (int)((frac * VCAL_SecsInDay) + 0.5) ; hh = i / (60*60) ; mm = (i / 60) % 60 ; ss = i % 60 ; } ;
		if (hh == 24)				/* We are resolving to nearest second, if we rounded up to 24:00:00 then add 1 to date & make time 00:00:00 */
		 { fixdate++ ; hh = 0 ; mm = 0 ; ss = 0 ; } ;
		switch (calendar)
		 { default:
		   case UNUSED:
		   case VCAL_CalType_Gregorian:		GregorianFromFixed(fixdate,&cal) ; break ;
		   case VCAL_CalType_Julian:		JulianFromFixed(fixdate,&cal) ; break ;
		   case VCAL_CalType_Islamic:		IslamicFromFixed(fixdate,&cal) ; break ;
		   case VCAL_CalType_ISO:		isISO = TRUE ; ISOFromFixed(fixdate,&cal) ; cal.Month = cal.Week ; break ;
		   case VCAL_CalType_Hebrew:		HebrewFromFixed(fixdate,&cal) ; break ;
		   case VCAL_CalType_Chinese:
		   case VCAL_CalType_Hindu:		cal.Year = 0 ; cal.Month = 0 ; cal.Day = 0 ; break ;
		 } ;
		y = cal.Year ; m = cal.Month ; d = cal.Day ; ww = 0 ;
		qtr = (m / 4) + 1 ; period = m ;
		break ;
	   case V4DPI_PntType_UDT:
		if (zuluoffset == (double)UNUSED) zuluoffset = -(double)mscu_minutes_west() / (24 * 60) ;
		if (*num <= 0) { UCstrcpy(dst,zip4mat) ;  return(UCstrlen(dst)) ; } ;
		ddate = (double)((*num / VCAL_SecsInDay) + VCAL_UDTUDateOffset + VCal_MJDOffset) + (double)(*num % VCAL_SecsInDay) / ((double)VCAL_SecsInDay) ;
/*		Make timezone "local" because UDT by definition is local */
		num = (int *)&ddate ; timezone = 0 ; goto cal_entry ;
//		i = mscu_udate_to_yyyymmdd(*num/VCAL_SecsInDay+VCAL_UDTUDateOffset) ;
//		y = i / 10000 ; m = (i % 10000) / 100 ; d = i % 100 ;
//		i = *num % VCAL_SecsInDay ;
//		hh = i / (60 * 60) ; mm = (i / 60) % 60 ; ss = i % 60 ; break ;
	   case V4DPI_PntType_UYear:
		if (*num == 0) { UCstrcpy(dst,zip4mat) ; return(UCstrlen(dst)) ; } ;
		y = *num ; m = 0 ; d = 0 ; ww = 0 ; hh = 0 ; mm = 0 ; ss = 0 ; qtr = 0 ; period = 0 ;
		break ;
	   case V4DPI_PntType_UPeriod:
//xxx formatdate
		{ int ppy = (pnttype >> 16) ; if (ppy == 0) ppy = 13 ;	/* Default to 13 periods */
		  if (*num == 0) { UCstrcpy(dst,zip4mat) ; return(UCstrlen(dst)) ; } ;
		  i = *num ; m = 0 ; d = 0 ; ww = 0 ; hh = 0 ; mm = 0 ; ss = 0 ; qtr = 0 ;
		  if (i > 100000)				/* Got regular period or year end adjustment? */
		   { y = i / 100 ; period = (i % 100) ; }
		   else { y = (i / ppy) + VCAL_BaseYear ; period = (i % ppy) + 1 ; } ;
		}
		break ;
	   case V4DPI_PntType_UQuarter:
		if (*num == 0) { UCstrcpy(dst,zip4mat) ; return(UCstrlen(dst)) ; } ;
		i = *num ; m = 0 ; d = 0 ; ww = 0 ; hh = 0 ; mm = 0 ; ss = 0 ; period = 0 ;
//		y = (i / 4) + VCAL_BaseYear ; qtr = (i % 4) + 1 ;
		y = UQTRtoUYEAR(i) ; qtr = UQTRtoQTR(i) ;
		break ;
	   case V4DPI_PntType_UWeek:
		if (*num == 0) { UCstrcpy(dst,zip4mat) ; return(UCstrlen(dst)) ; } ;
		i = *num ; m = 0 ; d = 0 ; ww = 0 ; hh = 0 ; mm = 0 ; ss = 0 ; qtr = 0 ;
		ww = (i % 52) + 1 ; y = (i / 52) + VCAL_BaseYear ;
		break ;
	   case V4DPI_PntType_UTime:
		dp = (double *)num ; dnum = *dp ;
		if (dnum < 0) { UCstrcpy(dst,zip4mat) ;  return(UCstrlen(dst)) ; } ;
		fss = modf(dnum,&intdbl) ; i = intdbl ;
		if (!specfss && fss >= 0.5) i++ ;	/* If fraction >= 0.5 and not going to output fraction then round up to nearest second */
		y = 0 ; m = 0 ; d = 0 ; ww = 0 ;
		hh = i / 3600 ; mm = (i % 3600) / 60 ; ss = i % 60 ;
		ddate = UNUSED ; num = (int *)&ddate ; timezone = 0 ;
		break ;
	 } ;
	dx = 0 ;
	for (i=0;i<fd.Count;i++)
	 { switch(fd.Type[i])
	    { case FD_Month:
		if (isISO)
		 { if (fd.Length[i] > 3) { v_Msg(NULL,tbuf,"@%1U%2d",ci->cli->CLI[curx].LMonths[0],m) ; }
		    else if (fd.Length[i] == 3)
		    { v_Msg(NULL,tbuf,"@%1U%2d",ci->cli->CLI[curx].SMonths[0],m) ; }
		    else { UCsprintf(tbuf,UCsizeof(tbuf),(fd.ZeroFill[i] ? UClit("%0*d") : UClit("%*d")),fd.Length[i],m) ; } ;
		   for(j=0;tbuf[j]!=UCEOS;j++) { dst[dx++] = (fd.UpperCase[i] ? UCTOUPPER(tbuf[j]) : tbuf[j]) ; } ;
		 } else
		 { if (fd.Length[i] > 3) { UCstrcpy(tbuf,ci->cli->CLI[curx].LMonths[m-1]) ; }
		    else if (fd.Length[i] == 3)
		    { UCstrcpy(tbuf,ci->cli->CLI[curx].SMonths[m-1]) ; }
		    else { UCsprintf(tbuf,UCsizeof(tbuf),(fd.ZeroFill[i] ? UClit("%0*d") : UClit("%*d")),fd.Length[i],m) ; } ;
		   for(j=0;tbuf[j]!=UCEOS;j++) { dst[dx++] = (fd.UpperCase[i] ? UCTOUPPER(tbuf[j]) : tbuf[j]) ; } ;
		 } ;
		break ;
	      case FD_Year:
		if (fd.Length[i] <= 2) { y = y % 100 ; fd.Length[i] = 2 ; } ;
		UCsprintf(tbuf,UCsizeof(tbuf),UClit("%0*d"),fd.Length[i],y) ;
		for(j=0;tbuf[j]!='\0';j++) { dst[dx++] = tbuf[j] ; } ; break ;
	      case FD_Day:
		UCsprintf(tbuf,UCsizeof(tbuf),(fd.ZeroFill[i] ? UClit("%0*d") : UClit("%*d")),fd.Length[i],d) ;
		for(j=0;tbuf[j]!=UCEOS;j++) { dst[dx++] = tbuf[j] ; } ; break ;
	      case FD_WeekDay:
		if (fixdate < 0) { UCstrcpy(tbuf,UClit("???")) ; }
		 else { d1 = (fixdate % 7) ; if (d1 == 0) d1 = 7 ; d1-- ;
			if (fd.Length[i] > 3) { UCstrcpy(tbuf,ci->cli->CLI[curx].LDaysOfWeek[d1]) ; }
			 else if (fd.Length[i] == 3)
			 { UCstrcpy(tbuf,ci->cli->CLI[curx].SDaysOfWeek[d1]) ; }
			 else { UCsprintf(tbuf,UCsizeof(tbuf),(fd.ZeroFill[i] ? UClit("%0*d") : UClit("%*d")),fd.Length[i],d1+1) ; } ;
		      } ;
		for(j=0;tbuf[j]!='\0';j++) { dst[dx++] = (fd.UpperCase[i] ? UCTOUPPER(tbuf[j]) : tbuf[j]) ; } ;
		break ;
	      case FD_Julian:
		if (fixdate - VCal_MJDOffset < 0) { UCstrcpy(tbuf,UClit("???")) ; }
		 else { d = mscu_udate_to_yyyyddd(fixdate - VCal_MJDOffset) ; d %= 1000 ;
			UCsprintf(tbuf,UCsizeof(tbuf),(fd.ZeroFill[i] ? UClit("%0*d") : UClit("%*d")),fd.Length[i],d) ;
		      } ;
		for(j=0;tbuf[j]!='\0';j++) { dst[dx++] = tbuf[j] ; } ;
		break ;
	      case FD_Week:
		UCsprintf(tbuf,UCsizeof(tbuf),(fd.ZeroFill[i] ? UClit("%0*d") : UClit("%*d")),fd.Length[i],ww) ;
		for(j=0;tbuf[j]!='\0';j++) { dst[dx++] = tbuf[j] ; } ; break ;
	      case FD_Punc:
		dst[dx++] = fd.Length[i] ; break ;
	      case FD_Hour:
		if (fd.Type[i+1] == FD_Punc && fd.Type[i+2] == FD_Month && fd.Length[i+2] == 2)		/* If got hh:mm then convert to 0h:0n to format properly */
		 { fd.ZeroFill[i] = TRUE ; fd.Type[i+2] = FD_Minute ; fd.ZeroFill[i+2] = TRUE ; } ;
		UCsprintf(tbuf,UCsizeof(tbuf),(fd.ZeroFill[i] ? UClit("%0*d") : UClit("%*d")),fd.Length[i],hh) ;
		for(j=0;tbuf[j]!='\0';j++) { dst[dx++] = tbuf[j] ; } ; break ;
	      case FD_Hour12:
		if (fd.Type[i+1] == FD_Punc && fd.Type[i+2] == FD_Month && fd.Length[i+2] == 2)		/* If got hh:mm then convert to 0h:0n to format properly */
		 { fd.ZeroFill[i] = TRUE ; fd.Type[i+2] = FD_Minute ; fd.ZeroFill[i+2] = TRUE ; } ;
		UCsprintf(tbuf,UCsizeof(tbuf),(fd.ZeroFill[i] ? UClit("%0*d") : UClit("%*d")),fd.Length[i],(hh > 12 ? hh-12 : (hh == 0 ? 12 : hh))) ;
		for(j=0;tbuf[j]!='\0';j++) { dst[dx++] = tbuf[j] ; } ; break ;
	      case FD_Minute:
		UCsprintf(tbuf,UCsizeof(tbuf),(fd.ZeroFill[i] ? UClit("%0*d") : UClit("%*d")),fd.Length[i],mm) ;
		for(j=0;tbuf[j]!='\0';j++) { dst[dx++] = tbuf[j] ; } ; break ;
	      case FD_Second:
		UCsprintf(tbuf,UCsizeof(tbuf),(fd.ZeroFill[i] ? UClit("%0*d") : UClit("%*d")),fd.Length[i],ss) ;
		for(j=0;tbuf[j]!='\0';j++) { dst[dx++] = tbuf[j] ; } ; break ;
	      case FD_FracSecond:
		UCsprintf(tbuf,UCsizeof(tbuf),UClit("%g"),fss) ;
/*		Add +2 to skip over leading "0." in fractional second */
		for(j=0;tbuf[j+2]!='\0'&&j<fd.Length[i];j++) { dst[dx++] = tbuf[j+2] ; } ; break ;
	      case FD_AMPM:
		UCstrcpy(tbuf,(hh >= 12 ? (fd.UpperCase[i] ? UClit("PM") : UClit("pm")) : (fd.UpperCase[i] ? UClit("AM") : UClit("am")))) ;
		for(j=0;tbuf[j]!='\0';j++) { dst[dx++] = tbuf[j] ; } ; break ;
	      case FD_Period:
		UCsprintf(tbuf,UCsizeof(tbuf),(fd.ZeroFill[i] ? UClit("%0*d") : UClit("%*d")),fd.Length[i],period) ;
		for(j=0;tbuf[j]!='\0';j++) { dst[dx++] = tbuf[j] ; } ; break ;
	      case FD_Quarter:
		UCsprintf(tbuf,UCsizeof(tbuf),(fd.ZeroFill[i] ? UClit("%0*d") : UClit("%*d")),fd.Length[i],qtr) ;
		for(j=0;tbuf[j]!='\0';j++) { dst[dx++] = tbuf[j] ; } ; break ;
	      case FD_Zone:
		i = mscu_minutes_west() ;
		if (i >= 0) { UCsprintf(tbuf,UCsizeof(tbuf),UClit("-%02d%02d"),i/60,i%60) ; }
		 else { UCsprintf(tbuf,UCsizeof(tbuf),UClit("+%02d%02d"),i/60,i%60) ; } ;
//		time(&ct) ; strftime(tbuf,3,"%z",localtime(&ct)) ; 
		for(j=0;tbuf[j]!='\0';j++) { dst[dx++] = (fd.UpperCase[i] ? UCTOUPPER(tbuf[j]) : tbuf[j]) ; } ;
		break ;
	    } ;
	 } ;
	dst[dx++] = '\0' ;
	return(dx) ;
}


/*	S P H E R I C A L   G E O M E T R Y  &   L A T  /  L O N   P A R S I N G			*/

double distFactors[] = { GEODISTFACTORS } ;

/*	v_NewLatLonFromLLAD - Updates new plat/plon from initial lat/lon, azimuth and distance (meters) */

void v_NewLatLonFromLLAD(plat,plon,lat,lon, az, dst)
  double *plat, *plon ;
  double lat, lon, az, dst ;
  
 { double rlat, rlon, rdst, raz ;
	rlat = 90.0 - lat ; rlat = rlat * 2.0 * PI / 360.0 ;	/* rlat = radians */
	rlon = lon * 2.0 * PI / 360.0 ;				/* rlon = radians */
	rdst = dst / METERSinNAUTICALMILE ;
	rdst = (PI /(180*60)) * rdst ;				/* rdst = distance in radians */
	raz = MOD(az+180,360.0) * 2.0 * PI / 360.0 ;		/* raz = radians */
	*plat=asin(sin(rlat)*cos(rdst)+cos(rlat)*sin(rdst)*cos(raz)) ;
	if (cos(*plat) == 0) { *plon = rlon ; }
	 else { double x = rlon - asin(sin(raz) * sin(rdst) / cos(*plat)) + PI ; *plon = MOD(x, 2 * PI) - PI ; } ;
	*plat =  90 - (*plat * (360.0 / (2.0 * PI))) ;
	*plon = (*plon * (360.0 / (2.0 * PI))) ;
 } ;

/*	v_DistBetween2LL - Returns distance (in requested units or meters if UNUSED) between two Lat/Lon pairs	*/

double v_DistBetween2LL(lat1,lon1,lat2,lon2,uomindex)
  double lat1,lon1,lat2,lon2 ;
  int uomindex ;
{ double phi,theta,x1,y1,z1, x2,y2,z2, dist ;

	phi = 90.0 - lat1 ; phi = phi * 2.0 * PI / 360.0 ;
	theta = lon1 * 2.0 * PI / 360.0 ;
	x1 = MEANEARTHRADIUS * cos(theta) * sin(phi) ; y1 = MEANEARTHRADIUS * sin(theta) * sin(phi) ; z1 = MEANEARTHRADIUS * cos(phi) ;
	phi = 90.0 - lat2 ; phi = phi * 2.0 * PI / 360.0 ;
	theta = lon2 * 2.0 * PI / 360.0 ;
	x2 = MEANEARTHRADIUS * cos(theta) * sin(phi) ; y2 = MEANEARTHRADIUS * sin(theta) * sin(phi) ; z2 = MEANEARTHRADIUS * cos(phi) ;
	dist = sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2)) ;
	dist = 2.0 * MEANEARTHRADIUS * asin(dist / (2.0 * MEANEARTHRADIUS)) ;
	dist = dist * 1000 ;				/* dist in meters */
	if (uomindex != UNUSED) dist = dist / distFactors[uomindex] ;	/* dist in what units user wanted */
	return(dist) ;
}

int v_FormatGeoCoord(ctx,geo,format,dst,dstmax)
  struct V4C__Context *ctx ;
  struct V4DPI__Value_GeoCoord *geo ;
  UCCHAR *format ;
  UCCHAR *dst ; int dstmax ;
{
  struct V4DPI__Value_GCDSV *gcdsv ;
  UCCHAR tbuf[128],*tzp ; int dlen,i,cx ; double dist,deg,hgt ; CALENDAR cal ;
  
	switch (geo->GCType)
	 { default:
	   case V4DPI_GCType_None:
		UCstrcpy(dst,gpi->ci->li->LI[gpi->ci->li->CurX].None1UC) ;
		return(UCstrlen(dst)) ;
	   case V4DPI_GCType_LatLonMeter:
//		hgt = (geo->Coord3 == V4DPI_GeoCoordCoord3_Null ? 0.0 : (double)(geo->Coord3 / V4DPI_GeoCoordCoord3_Scale) - V4DPI_GeoCoordCoord3_Offset) ;
		hgt = GETGEOALT(geo) ;
		if (geo->TimeZone == VCAL_TimeZone_Local)
		 { if(hgt != 0) { UCsprintf(dst,dstmax,UClit("(%g %g %g)"),GETGEOLAT(geo),GETGEOLON(geo),hgt) ; }
		    else { UCsprintf(dst,dstmax,UClit("(%g %g)"),GETGEOLAT(geo),GETGEOLON(geo)) ; } ;
		 } else
		 { UCsprintf(dst,dstmax,UClit("(%g %g %g %d)"),GETGEOLAT(geo),GETGEOLON(geo),hgt,geo->TimeZone) ;
		 } ;
		return(UCstrlen(dst)) ;
	   case V4DPI_GCType_Distance:
		gcdsv = (struct V4DPI__Value_GCDSV *)geo ;
		cx = gpi->ci->li->CurX ;
		UCstrcpy(dst,UClit("(")) ; dlen = UCstrlen(dst) ;
		if (gcdsv->Coord1 != UNUSED)
		 { tzp = gpi->ci->li->LI[cx].tzLocal ;
		   if (geo->TimeZone != VCAL_TimeZone_Local)
		    { for(i=0;i<V4LI_TZMax;i++) { if (geo->TimeZone == gpi->ci->li->LI[cx].TZ[i].tzOffset) { tzp = gpi->ci->li->LI[cx].TZ[i].tzAbbr ; break ; } ; } ;
		    } ;
		   v_Msg(ctx,tbuf,"@%1U:%2g %3U:%4g %5U:%6U",
				gpi->ci->li->LI[cx].gcLatitude,GETGEOLAT(geo),gpi->ci->li->LI[cx].gcLongitude,GETGEOLON(geo),gpi->ci->li->LI[cx].gcTimeZone,tzp) ;
		   UCstrcat(dst,tbuf) ; dlen += UCstrlen(tbuf) ;
		 } ;
		if (geo->Coord3 != V4DPI_GeoCoordCoord3_Null)
		 { if (dst[1] != UCEOS) { UCstrcat(dst,UClit(" ")) ; dlen++ ; } ;
		   v_Msg(ctx,tbuf,"@%1U:%2d",gpi->ci->li->LI[cx].gcHeight,(int)DtoI((((double)geo->Coord3 / V4DPI_GeoCoordCoord3_Scale) - V4DPI_GeoCoordCoord3_Offset) / distFactors[gcdsv->hgtUOM])) ;
		   UCstrcat(dst,tbuf) ; dlen += UCstrlen(tbuf) ;
		   for (i=0;i<V4LI_DstMax;i++)
		    { if (gcdsv->hgtUOM != gpi->ci->li->LI[cx].Dst[i].dstIntVal) continue ;
		      UCstrcat(dst,gpi->ci->li->LI[cx].Dst[i].dstAbbr) ; dlen += UCstrlen(gpi->ci->li->LI[cx].Dst[i].dstAbbr) ;
		      break ;
		    } ;
		 } ;
		if (gcdsv->distInt != 0 || gcdsv->distFrac != 0)
		 { if (dst[1] != UCEOS) { UCstrcat(dst,UClit(" ")) ; dlen++ ; } ;
		   dist = (double)gcdsv->distInt + ((double)gcdsv->distFrac) / V4DPI_GeoCoordDist_Factor ;
		   v_Msg(ctx,tbuf,"@%1U:%2g",(gcdsv->speedUnit == V4DPI_GeoCoordSpeed_None ? gpi->ci->li->LI[cx].gcDistance : gpi->ci->li->LI[cx].gcSpeed),dist) ;
		   UCstrcat(dst,tbuf) ; dlen += UCstrlen(tbuf) ;
		   if (gcdsv->speedUnit == V4DPI_GeoCoordSpeed_None)
		    { for (i=0;i<V4LI_DstMax;i++)
		       { if (gcdsv->distUOM != gpi->ci->li->LI[cx].Dst[i].dstIntVal) continue ;
		         UCstrcat(dst,gpi->ci->li->LI[cx].Dst[i].dstAbbr) ; dlen += UCstrlen(gpi->ci->li->LI[cx].Dst[i].dstAbbr) ;
		         break ;
		       } ;
		    }
		    { for (i=0;i<V4LI_SpdMax;i++)
		       { if (gcdsv->distUOM * 100 + gcdsv->speedUnit != gpi->ci->li->LI[cx].Spd[i].spdIntVal) continue ;
		         UCstrcat(dst,gpi->ci->li->LI[cx].Spd[i].spdAbbr) ;
		         dlen += UCstrlen(gpi->ci->li->LI[cx].Spd[i].spdAbbr) ;
		         break ;
		       } ;
		    } ;
		 } ;
		if (gcdsv->bearingDeg1 != V4DPI_GeoCoord_NoBearing*V4DPI_GeoCoord_Factor)
		 { if (dst[1] != UCEOS) { UCstrcat(dst,UClit(" ")) ; dlen++ ; } ;
		   deg = (double)gcdsv->bearingDeg1 / V4DPI_GeoCoord_Factor ;
		   v_Msg(ctx,tbuf,"@%1U:%2g",gpi->ci->li->LI[cx].gcAzimuth,deg) ;
		   UCstrcat(dst,tbuf) ; dlen += UCstrlen(tbuf) ; 
		 } ;
		if (gcdsv->bearingDeg2 != V4DPI_GeoCoord_NoBearing*V4DPI_GeoCoord_Factor)
		 { if (dst[1] != UCEOS) { UCstrcat(dst,UClit(" ")) ; dlen++ ; } ;
		   deg = (double)gcdsv->bearingDeg2 / V4DPI_GeoCoord_Factor ;
		   v_Msg(ctx,tbuf,"@%1U:%2g",gpi->ci->li->LI[cx].gcElevation,deg) ;
		   UCstrcat(dst,tbuf) ; dlen += UCstrlen(tbuf) ; 
		 } ;
		SETDBL(cal,gcdsv->calDateTime) ;
		if (cal != VCal_NullDate)
		 { if (dst[1] != UCEOS) { UCstrcat(dst,UClit(" ")) ; dlen++ ; } ;
		   UCstrcat(dst,gpi->ci->li->LI[cx].gcDateTime) ; UCstrcat(dst,UClit(":")) ; dlen += (UCstrlen(gpi->ci->li->LI[cx].gcDateTime) + 1) ;
		   v_FormatDate(&cal,V4DPI_PntType_Calendar,VCAL_CalType_UseDeflt,gcdsv->TimeZone,NULL,tbuf) ;
		   UCstrcat(dst,tbuf) ; dlen += UCstrlen(tbuf) ; 
		 } ;
		UCstrcat(dst,UClit(")")) ; dlen++ ;
		return(dlen) ;
	 } ;
}

LOGICAL v_ParseGeoCoord(ctx,point,di,geobuf,errmsg)
  struct V4C__Context *ctx ;
  struct V4DPI__DimInfo *di ;
  struct V4DPI__Point *point ;
  UCCHAR *geobuf, *errmsg  ;
{
  struct V4DPI__Value_GCDSV *gcdsv ;
  double lat,lon,hgt,ms,az,el,dist ; int tz,duom,suom,huom ; CALENDAR cal ;
  UCCHAR *gb,*tb,tag[48],sbbuf[128] ; int cx,num,i ;
  
	ZPH(point) ; point->Dim = di->DimId ; point->PntType = di->PointType ;
/*	First check for NONE value or any special values */
	if (v_IsNoneLiteral(geobuf))
	 { point->Value.GeoCoord.GCType = V4DPI_GCType_None ; point->Bytes = V4PS_GeoCoord ;
	   point->Value.GeoCoord.Coord1 = 0 ; point->Value.GeoCoord.Coord2 = 0 ; point->Value.GeoCoord.Coord3 = V4DPI_GeoCoordCoord3_Null ;
	   point->Value.GeoCoord.TimeZone = 0 ;
	   return(TRUE) ;
	 } ;
	if (v4sxi_SpecialAcceptor(ctx,point,di,geobuf)) return(TRUE) ;

	gb = geobuf ; cx = gpi->ci->li->CurX ;
	if (*gb == UClit('('))			/* Ignore possible enclosing "(...)" */
	 {  gb++ ;
	    if (gb[UCstrlen(gb)-1] == UClit(')')) gb[UCstrlen(gb)-1] = UCEOS ;
	 } ;

	lat = 0.0 ; lon = 0.0 ; hgt = V4DPI_GeoCoordCoord3_Null ; tz = di->ds.Geo.TimeZone ; az = V4DPI_GeoCoord_NoBearing ; el = V4DPI_GeoCoord_NoBearing ; dist = 0.0 ;
	duom = 0 ; suom = 0 ; huom = 0 ; cal = VCal_NullDate ;

#define IWS for(;vuc_IsWSpace(*gb);gb++) {} ;	/* Macro to ignore white spaces */
#define COORD(LL,DPOS,DNEG) \
 LL = UCstrtod(gb,&tb) ; \
 if(*tb == UClit(':')) \
  { gb = tb + 1 ; ms = (double)UCstrtol(gb,&tb,10) ; \
    if (ms < 0 || ms >= 60) { v_Msg(ctx,errmsg,"DPIGeoMinSec",ms) ; return(FALSE) ; } ; \
    LL += (LL < 0 ? -ms : ms) / 60.0 ; \
    if (*tb == UClit(':')) \
     { gb = tb + 1 ; ms = UCstrtod(gb,&tb) ; \
       if (ms < 0 || ms > 60) { v_Msg(ctx,errmsg,"DPIGeoMinSec",ms) ; return(FALSE) ; } ; \
       LL += (LL < 0 ? -ms : ms) / (60*60) ; \
     } ; \
  } ; \
 if (UCTOLOWER(*tb) == UCTOLOWER(gpi->ci->li->LI[cx].DNEG[0])) { LL = -LL ;  gb = tb + 1 ; } \
  else if (UCTOLOWER(*tb) == UCTOLOWER(gpi->ci->li->LI[cx].DPOS[0])) { gb = tb + 1 ; } \
  else { gb = tb ; } ; \
 if (*gb != UClit(' ') && *gb != UCEOS) { v_Msg(ctx,errmsg,"DPIGeoSyn",1+tb-geobuf,geobuf) ; return(FALSE) ; } ;

	IWS
/*	Determine if we just have geocode or expanded version with tags */
	if (vuc_IsDigit(gb[0]) || vuc_IsDigit(gb[1]))
	 { tz = VCAL_TimeZone_Local ;
	   COORD(lat,gcNorth,gcSouth) IWS COORD(lon,gcEast,gcWest)
	   IWS
	   if (*gb != UCEOS && *gb != UClit(')'))
	    { hgt = UCstrtod(gb,&tb) ; if (!(*tb == UClit(' ') || *tb == UClit(')') || *tb == UCEOS)) { v_Msg(ctx,errmsg,"DPIGeoSyn",tb-geobuf,geobuf) ; return(FALSE) ; } ;
//	      hgt = (hgt + V4DPI_GeoCoordCoord3_Offset) * V4DPI_GeoCoordCoord3_Scale ;
	      gb = (*tb == UCEOS ? tb : tb + 1) ; IWS	    
	      if (*gb != UCEOS && *gb != UClit(')'))
	       { tz = UCstrtod(gb,&tb) ; if (!(*tb == UClit(' ') || *tb == UClit(')') || *tb == UCEOS)) { v_Msg(ctx,errmsg,"DPIGeoSyn",tb-geobuf,geobuf) ; return(FALSE) ; } ; } ;
	    } ;
	   if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0 || hgt < 0 || hgt > V4DPI_GeoCoordCoord3_Max
		|| (tz == VCAL_TimeZone_Local ? FALSE : tz < -12 || tz > 12))
	    { v_Msg(ctx,errmsg,"DPIGeoCoord",lat,lon,hgt,V4DPI_GeoCoordCoord3_Max,(tz == VCAL_TimeZone_Local ? 0 : tz)) ; return(FALSE) ; } ;
//	   point->Value.GeoCoord.Coord1 = DtoI(lat * V4DPI_GeoCoord_Factor) ;
//	   point->Value.GeoCoord.Coord2 = DtoI(lon * V4DPI_GeoCoord_Factor) ;
//	   point->Value.GeoCoord.Coord3 = hgt ; point->Value.GeoCoord.TimeZone = tz ;
	   SETGEOLAT(&point->Value.GeoCoord,lat) ;
	    SETGEOLON(&point->Value.GeoCoord,lon) ;
	     SETGEOALT(&point->Value.GeoCoord,hgt) ;
	      point->Value.GeoCoord.TimeZone = tz ;
	   point->Value.GeoCoord.GCType = V4DPI_GCType_LatLonMeter ; point->Bytes = V4PS_GeoCoord ;
	   return(TRUE) ;
	 } ;
/*	Here to parse expanded geocoordinate spec (i.e. with tags) */
	for(;;)
	 { IWS if (*gb == UCEOS || *gb == UClit(')')) break ;
	   tb = UCstrchr(gb,':') ; if (tb == NULL) { v_Msg(ctx,errmsg,"DPIGeoTag",1+gb-geobuf,geobuf) ; return(FALSE) ; } ;
	   num = tb - gb ; if (num >= UCsizeof(tag)) num = UCsizeof(tag) - 1 ;
	   UCstrncpy(tag,gb,num) ; tag[num] = UCEOS ; gb = tb + 1 ;
#define GEOTAG(NAME) UCstrcmpIC(gpi->ci->li->LI[cx].NAME,tag) == 0
	   if (GEOTAG(gcLatitude)) { COORD(lat,gcNorth,gcSouth) ; }
	    else if (GEOTAG(gcLongitude)) { COORD(lon,gcEast,gcWest) ; }
	    else if (GEOTAG(gcHeight))
		  { hgt = UCstrtod(gb,&tb) ; gb = tb ;
		    tb = UCstrchr(gb,' ') ; if (tb == NULL) tb = UCstrchr(gb,'\0') ;
		    num = tb - gb ; UCstrncpy(tag,gb,num) ; tag[num] = UCEOS ; gb = tb ;
		    for(i=0;i<V4LI_DstMax;i++) { if (UCstrcmpIC(tag,gpi->ci->li->LI[cx].Dst[i].dstAbbr) == 0) break ; } ;
		    if (i >= V4LI_DstMax)
		     { ZUS(sbbuf) ; for(i=0;i<V4LI_DstMax;i++) { if (UCstrlen(gpi->ci->li->LI[cx].Dst[i].dstAbbr) != 0) { UCstrcat(sbbuf,gpi->ci->li->LI[cx].Dst[i].dstAbbr) ; UCstrcat(sbbuf,UClit(",")) ; } ; } ;
		       sbbuf[UCstrlen(sbbuf) - 1] = UCEOS ; v_Msg(ctx,errmsg,"DPIGeoUOMBad",tag,sbbuf) ; return(FALSE) ;
		     } ;
		    huom = gpi->ci->li->LI[cx].Dst[i].dstIntVal ;
		    hgt = (hgt * distFactors[huom]) ;
		  }
	    else if (GEOTAG(gcTimeZone))
		  { if (cal != VCal_NullDate) { v_Msg(ctx,errmsg,"DPIGeoTZDT") ; return(FALSE) ; } ;
		    if (vuc_IsLetter(*gb))		/* Do we have mnemonic or numeric tz specification? */
		     { tb = UCstrchr(gb,' ') ; if (tb == NULL) tb = UCstrchr(gb,'\0') ;
		       num = tb - gb ; UCstrncpy(tag,gb,num) ; tag[num] = UCEOS ;
		       for(i=0;i<V4LI_TZMax;i++) { if (UCstrcmpIC(tag,gpi->ci->li->LI[cx].TZ[i].tzAbbr) == 0) break ; } ;
		       if (i >= V4LI_TZMax)
		        { ZUS(sbbuf) ; for(i=0;i<V4LI_TZMax;i++) { if (UCstrlen(gpi->ci->li->LI[cx].TZ[i].tzAbbr) != 0) { UCstrcat(sbbuf,gpi->ci->li->LI[cx].TZ[i].tzAbbr) ; UCstrcat(sbbuf,UClit(",")) ; } ; } ;
		          sbbuf[UCstrlen(sbbuf) - 1] = UCEOS ; v_Msg(ctx,errmsg,"DPIGeoTZBad",tag,sbbuf) ; return(FALSE) ;
		        } ;
		       tz = gpi->ci->li->LI[cx].TZ[i].tzOffset ;
		     } else { tz = UCstrtod(gb,&tb) ; } ;
		    gb = tb ;
		  }
	    else if (GEOTAG(gcDistance))
		  { dist = UCstrtod(gb,&tb) ; gb = tb ;
		    tb = UCstrchr(gb,' ') ; if (tb == NULL) tb = UCstrchr(gb,'\0') ;
		    num = tb - gb ; UCstrncpy(tag,gb,num) ; tag[num] = UCEOS ; gb = tb ;
		    for(i=0;i<V4LI_DstMax;i++) { if (UCstrcmpIC(tag,gpi->ci->li->LI[cx].Dst[i].dstAbbr) == 0) break ; } ;
		    if (i < V4LI_DstMax) { duom = gpi->ci->li->LI[cx].Dst[i].dstIntVal ; continue ; } ;
		    ZUS(sbbuf) ; for(i=0;i<V4LI_DstMax;i++) { if (UCstrlen(gpi->ci->li->LI[cx].Dst[i].dstAbbr) != 0) { UCstrcat(sbbuf,gpi->ci->li->LI[cx].Dst[i].dstAbbr) ; UCstrcat(sbbuf,UClit(",")) ; } ; } ;
		    sbbuf[UCstrlen(sbbuf) - 1] = UCEOS ; v_Msg(ctx,errmsg,"DPIGeoUOMBad",tag,sbbuf) ; return(FALSE) ;
		  }
	    else if (GEOTAG(gcSpeed))
		  { dist = UCstrtod(gb,&tb) ; gb = tb ;
		    tb = UCstrchr(gb,' ') ; if (tb == NULL) tb = UCstrchr(gb,'\0') ;
		    num = tb - gb ; UCstrncpy(tag,gb,num) ; tag[num] = UCEOS ; gb = tb ;
		    for(i=0;i<V4LI_SpdMax;i++) { if (UCstrcmpIC(tag,gpi->ci->li->LI[cx].Spd[i].spdAbbr) == 0) break ; } ;
		    if (i < V4LI_SpdMax) { duom = gpi->ci->li->LI[cx].Spd[i].spdIntVal / 100 ; suom = gpi->ci->li->LI[cx].Spd[i].spdIntVal % 100 ; continue ; } ;
		    ZUS(sbbuf) ; for(i=0;i<V4LI_DstMax;i++) { if (UCstrlen(gpi->ci->li->LI[cx].Dst[i].dstAbbr) != 0) { UCstrcat(sbbuf,gpi->ci->li->LI[cx].Dst[i].dstAbbr) ; UCstrcat(sbbuf,UClit(",")) ; } ; } ;
		    sbbuf[UCstrlen(sbbuf) - 1] = UCEOS ; v_Msg(ctx,errmsg,"DPIGeoUOMBad",tag,sbbuf) ; return(FALSE) ;
		  }
	    else if (GEOTAG(gcDateTime))
		  { YMDORDER *ymdo ; ;
		    tb = UCstrchr(gb,' ') ; if (tb == NULL) tb = UCstrchr(gb,'\0') ;
		    num = tb - gb ; UCstrncpy(tag,gb,num) ; tag[num] = UCEOS ; gb = tb ;
		    ymdo = (di->ds.Geo.YMDOrder[0] == V4LEX_YMDOrder_None ? gpi->ci->Cntry[cx].ymdOrder : &di->ds.Geo.YMDOrder[0]) ;
		    for(i=0;i<VCAL_YMDOrderMax && ymdo[i]!=V4LEX_YMDOrder_None;i++)
		     { cal = v_ParseCalendar(ctx,tag,di->ds.Geo.IFormat,ymdo[i],di->ds.Geo.CalendarType,tz,((di->ds.Cal.calFlags & VCAL_Flags_Historical) == 0)) ;
		       if (cal != VCAL_BadVal || di->ds.Geo.IFormat != V4LEX_TablePT_Default) break ;
		     } ;
		    if (cal == VCAL_BadVal) { UCstrcpy(errmsg,ctx->ErrorMsgAux) ; return(FALSE) ; } ;
		  }
	    else if (GEOTAG(gcAzimuth)) { az = UCstrtod(gb,&tb) ; gb = tb ; }
	    else if (GEOTAG(gcElevation)) { el = UCstrtod(gb,&tb) ; gb = tb ; }
	    else { ZUS(sbbuf) ;
#define ATAG(NAME) UCstrcat(sbbuf,gpi->ci->li->LI[cx].NAME) ; UCstrcat(sbbuf,UClit(",")) ;
		   ATAG(gcLatitude) ATAG(gcLongitude) ATAG(gcHeight) ATAG(gcTimeZone) ATAG(gcDistance) ATAG(gcSpeed) ATAG(gcDateTime) ATAG(gcAzimuth) ATAG(gcElevation)
		   sbbuf[UCstrlen(sbbuf) - 1] = UCEOS ; v_Msg(ctx,errmsg,"DPIGeoTagBad",tag,sbbuf) ; return(FALSE) ;
		 } ;
	 } ;
/*	Build up geo point from its components */
	if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0 || hgt < -V4DPI_GeoCoordCoord3_Offset || hgt > V4DPI_GeoCoordCoord3_Max
		|| (tz == VCAL_TimeZone_Local ? FALSE : tz < -12 || tz > 12))
	 { v_Msg(ctx,errmsg,"DPIGeoCoord",lat,lon,hgt,V4DPI_GeoCoordCoord3_Max,(tz == VCAL_TimeZone_Local ? 0 : tz)) ; return(FALSE) ; } ;
	if ((az == V4DPI_GeoCoord_NoBearing ? FALSE : (az < -180.0 || az > 180.0)) || (el == V4DPI_GeoCoord_NoBearing ? FALSE : (el < -90.0 || el > 90.0)))
	 { v_Msg(ctx,errmsg,"DPIGeoAzEl",az,el) ; return(FALSE) ; } ;
	gcdsv = (struct V4DPI__Value_GCDSV *)&point->Value ;
//	gcdsv->Coord1 = DtoI(lat * V4DPI_GeoCoord_Factor) ; gcdsv->Coord2 = DtoI(lon * V4DPI_GeoCoord_Factor) ;
//	gcdsv->Coord3 = hgt ; gcdsv->TimeZone = tz ; gcdsv->distUOM = duom ; gcdsv->speedUnit = suom ; gcdsv->hgtUOM = huom ;
//	frac = modf(dist,&ipart) ; gcdsv->distInt = DtoI(ipart) ; gcdsv->distFrac = DtoI(frac * V4DPI_GeoCoordDist_Factor) ;
//	gcdsv->bearingDeg1 = DtoI(az * V4DPI_GeoCoord_Factor) ; gcdsv->bearingDeg2 = DtoI(el * V4DPI_GeoCoord_Factor) ;
	SETGEOLAT(gcdsv,lat) ; SETGEOLON(gcdsv,lon) ; SETGEOALT(gcdsv,hgt) ;
	SETGEODSTSPD(gcdsv,dist) ;
	 SETGEOAZ(gcdsv,az) ;
	  SETGEOEL(gcdsv,el) ;
	SETDBL(gcdsv->calDateTime,cal) ;
	gcdsv->TimeZone = tz ; gcdsv->distUOM = duom ; gcdsv->speedUnit = suom ; gcdsv->hgtUOM = huom ;
	gcdsv->GCType = V4DPI_GCType_Distance ; point->Bytes = V4PS_GeoCoordGCDSV ; return(TRUE) ;
}

int v_FormatTele(ctx,tele,format,dst)
  struct V4C__Context *ctx ;
  struct V4DPI__Value_Tele *tele ;
  UCCHAR *format ;
  UCCHAR *dst ;
{ UCCHAR *b, *zip4mat, *noac4mat ;
  int i,j,type,totalnum,used,ok,dx ; UCCHAR tbuf[256],numberbuf[256] ;
  struct {
    int Count ;			/* Number of components in date */
    int Type[FD_CompMax+1] ;	/* Type of component */
    int Length[FD_CompMax+1] ;
    int ZeroFill[FD_CompMax+1] ;
    int UpperCase[FD_CompMax+1] ;
   } fd ;
  enum DictionaryEntries deval ;


/*	First see if we have special telenumber for formatting */
	if (tele->IntDialCode == V4DPI__Tele_IDCSpecial && ctx != NULL)
	 { struct V4DPI__LittlePoint lpt,lpt2 ;
	   struct V4DPI__Point isctbuf,tpnt,*ipt ;
	   CHECKV4INIT ;
	   lpt.Dim = Dim_UV4 ; lpt.PntType = V4DPI_PntType_Dict ; lpt.Bytes = V4PS_Int ;
//	   lpt.Value.IntVal = v4dpi_DictEntryGet(ctx,UNUSED,"DisplayerTele",NULL,NULL) ;
	   lpt.Value.IntVal = v4im_GetEnumToDictVal(ctx,deval=_DisplayerTele,UNUSED) ;
	   if (lpt.Value.IntVal <= 0) goto normal_format ;
	   ZPH(&lpt2) ; lpt2.PntType = V4DPI_PntType_Int2 ; lpt2.Dim = Dim_Int2 ; lpt2.Bytes = V4PS_Int2 ;
	   INITISCT(&isctbuf) ; NOISCTVCD(&isctbuf) ; isctbuf.Grouping = 2 ;
	   ipt = ISCT1STPNT(&isctbuf) ; memcpy(ipt,&lpt,lpt.Bytes) ; ADVPNT(ipt) ;
	   lpt2.Value.Int2Val[0] = tele->AreaCode ; lpt2.Value.Int2Val[1] = tele->Number ;
	   memcpy(ipt,&lpt2,lpt2.Bytes) ; ADVPNT(ipt) ;
	   ISCTLEN(&isctbuf,ipt) ;
	   ipt = v4dpi_IsctEval(&tpnt,&isctbuf,ctx,V4DPI_EM_EvalQuote,NULL,NULL) ;
	   if (ipt == NULL) goto normal_format ;
	   v4im_GetPointUC(&ok,dst,250,ipt,ctx) ;
	   return(UCstrlen(dst)) ;
	 } ;
/*	Parse format string normal;no-area-code;no-number */
normal_format:
	if (UCnotempty(gpi->formatMasks[V4DPI_PntType_TeleNum])) format = gpi->formatMasks[V4DPI_PntType_TeleNum] ;
	zip4mat = UClit("") ; noac4mat = NULL ;
	b = UCstrchr(format,';') ;
	if (b != NULL)
	 { *b = UCEOS ;
	   if (b[1] != UCEOS)
	    { noac4mat = b + 1 ;
	      b = UCstrchr(noac4mat,';') ;
	      if (b != NULL) { *b = UCEOS ; if (b[1] != UCEOS) zip4mat = b + 1 ; } ;
	    } ;
	 } ;
	if (b != NULL) { *b = UCEOS ; if (*(b+1) != UCEOS) zip4mat = b + 1 ; } ;
	if (tele->Number == 0 && tele->AreaCode == 0) { UCstrcpy(dst,zip4mat) ; return(UCstrlen(dst)) ; } ;
	if (tele->AreaCode == 0 && noac4mat != NULL) format = noac4mat ;
	memset(&fd,0,sizeof fd) ;
	totalnum = 0 ;						/* Total number of positions in "number" portion */
/*	If country code != 1 then make sure format starts with country code */
	if (tele->IntDialCode > 1 && format[0] != UClit('+'))
	 { fd.Length[fd.Count] = UClit('+') ; fd.Type[fd.Count++] = FD_Punc ; fd.Type[fd.Count++] = FD_CntryCode ; type = FD_CntryCode ; fd.Length[fd.Count] = UClit(' '); fd.Type[fd.Count++] = FD_Punc;
	} ;
	for(i=0;format[i]!=UCEOS;i++)
	 { type = (fd.Count == 0 ? UNUSED : fd.Type[fd.Count-1]) ;
	   if (fd.Count >= FD_CompMax) break ;
	   switch (format[i])
	    { default:	fd.Length[fd.Count] = format[i] ; fd.Type[fd.Count++] = FD_Punc ; continue ;
	      case UClit('"'):
		for(i++,ok=TRUE;ok;i++)
		 { switch(format[i])
		    { case UClit('"'):	i-- ; ok = FALSE ; break ;	
		      case UCEOS:	i-- ; ok = FALSE ; break ;
		      default:		if (fd.Count >= FD_CompMax) { ok = FALSE ; break ; } ;
					fd.Length[fd.Count] = format[i] ; fd.Type[fd.Count++] = FD_Punc ; continue ;
		    } ;
		 } ; continue ;
	      case UClit('a'):
	      case UClit('A'): FDE(FD_AreaCode)
	      case UClit('c'):
	      case UClit('C'): FDE(FD_CntryCode)
	      case UClit('n'):
	      case UClit('N'):	totalnum++ ; FDE(FD_Number)
	      case UClit('t'):
	      case UClit('T'):	FDE(FD_Type) ;
	      case UClit('x'):
	      case UClit('X'):	FDE(FD_Extension) ;
	    } ; fd.Length[fd.Count-1]++ ;
	 } ;

	dx = 0 ; used = 0 ; UCsprintf(numberbuf,UCsizeof(numberbuf),UClit("%*d"),totalnum,tele->Number) ;
memset(dst,0,30) ;
	for (i=0;i<fd.Count;i++)
	 { switch(fd.Type[i])
	    { case FD_AreaCode:
		UCsprintf(tbuf,UCsizeof(tbuf),UClit("%d"),tele->AreaCode) ;
		for(j=0;tbuf[j]!=UCEOS;j++) { dst[dx++] = tbuf[j] ; } ; break ;
		break ;
	      case FD_Punc:
		dst[dx++] = fd.Length[i] ; break ;
	      case FD_CntryCode:
		UCsprintf(tbuf,UCsizeof(tbuf),UClit("%d"),(tele->IntDialCode < 0 ? -tele->IntDialCode : tele->IntDialCode)) ;
		for(j=0;tbuf[j]!=UCEOS;j++) { dst[dx++] = tbuf[j] ; } ; break ;
	      case FD_Extension:
		if (tele->IntDialCode >= 0) break ;		/* Have extension/type only if Country Code is negative */
		if (tele->Extension > 0)		
		 { UCsprintf(tbuf,UCsizeof(tbuf),UClit("X%d"),tele->Extension) ;
		   for(j=0;tbuf[j]!=UCEOS;j++) { dst[dx++] = tbuf[j] ; } ;
		 } ;
		break ;
	      case FD_Type:
		if (tele->IntDialCode >= 0) break ;		/* Have extension/type only if Country Code is negative */
		{ UCCHAR *tt = NULL ;
		  switch (tele->Type)
		   { case V4DPI_TeleType_Cell:	tt = v4im_GetEnumToUCVal(_Cell) ; break ;
		     case V4DPI_TeleType_Fax:	tt = v4im_GetEnumToUCVal(_Fax) ; break ;
		     case V4DPI_TeleType_Home:	tt = v4im_GetEnumToUCVal(_Home) ; break ;
		     case V4DPI_TeleType_Work:	tt = v4im_GetEnumToUCVal(_Work) ; break ;
		   } ;
		  if (tt != NULL)
		   { for(j=0;tt[j]!=UCEOS;j++) { dst[dx++] = tt[j] ; } ;
		     dst[dx++] = UClit(':') ; dst[dx++] = UClit(' ') ; 
		   } ;
		} ; break ;
	      case FD_Number:
		UCstrncpy(tbuf,&numberbuf[used],fd.Length[i]) ;
		tbuf[fd.Length[i]] = UCEOS ; used += fd.Length[i] ;
		for(j=0;tbuf[j]!=UCEOS;j++) { if (tbuf[j] != ' ') dst[dx++] = tbuf[j] ; } ; break ;
	    } ;
	 } ;
	dst[dx++] = '\0' ;
	return(dx) ;
}

/*	v_FormatDbl - Formats double (real) via Excel format string (e.g. "#,##0") */
/*	NOTE: format string may be updated!!!! */

int v_FormatDbl(dnum,format,dst,rgbColor)
  double dnum ; UCCHAR *format, *dst, *rgbColor ;
{ UCCHAR *b,*b1,delim,*sign,stack[100],nsrc[64],prefix[64],suffix[64] ;
  UCCHAR color[128],tformat[256] ;
  int i,j,s,zeros,neg,dps,digits ; UCCHAR *pos4mat,*neg4mat,*zip4mat ;
  struct V4CI__CountryInfo *ci ; UCCHAR dpnt ;

	ci = gpi->ci ;
/*	If no format OR it begins with '%' then use C formatting */
	if (format == NULL)
	 { if (UCnotempty(gpi->formatMasks[V4DPI_PntType_Real])) format = gpi->formatMasks[V4DPI_PntType_Real] ;
	 } ;
	if (format == NULL ? TRUE : format[0] == UClit('%'))
	 { UCsprintf(dst,100,(format == NULL ? UClit("%.15g") : format),dnum) ; return(UCstrlen(dst)) ; } ;
	if (dnum < 0) { dnum = -dnum ; neg = TRUE ; } else { neg = FALSE ; } ;
/*	Parse format string */
	zip4mat = NULL ; neg4mat = NULL ; pos4mat = format ;
	digits = FALSE ; ZUS(prefix) ; ZUS(suffix) ; ZUS(color) ;
	b = UCstrchr(format,';') ;
	if (b != NULL)
	 { UCstrcpy(tformat,format) ; format = tformat ; b = UCstrchr(format,';') ; pos4mat = format ;
	   *b = UCEOS ; if (*(b+1) != UCEOS) neg4mat = b + 1 ;
	   b = UCstrchr(b+1,';') ; if (b != NULL) { *b = UCEOS ; if (*(b+1) != UCEOS) { zip4mat = b + 1 ; } else { zip4mat = UClit("") ; } } ;
	 } ;
	if (dnum == 0 && zip4mat != NULL) { UCstrcpy(dst,zip4mat) ; return(UCstrlen(zip4mat)) ; } ;
	format = ((neg && neg4mat != NULL) ? neg4mat : pos4mat) ;
	sign = (neg4mat == NULL ? UClit("-") : NULL) ;
	delim = UCEOS ; dps = UNUSED ;
	for(i=0,zeros=0;format[i]!=UCEOS;i++)
	 { switch (format[i])
	    { default:			b = (digits ? &suffix[UCstrlen(suffix)] : &prefix[UCstrlen(prefix)]) ;
					*b = format[i] ; *(b+1) = UCEOS ; break ;
	      case UClit('#'):		digits = TRUE ; break ;
	      case UClit('0'):		digits = TRUE ; if (dps == UNUSED) { zeros++ ; } else { dps++ ; } ; break ;
	      case UClit(','):		if (ci->Cntry[ci->CurX].RadixPnt == UClit(',')) { dps = 0 ; dpnt = UClit(',') ; break ; } ;
					delim = UClit(',') ; break ;
	      case UClit('_'):		delim = ci->Cntry[ci->CurX].DigiDelim ; break ;
	      case UClit('.'):		if (ci->Cntry[ci->CurX].DigiDelim == UClit('.')) { delim = UClit('.') ; break ; } ;
					dpnt = UClit('.') ; dps = 0 ; break ;
	      case UClit('!'):		dps = 0 ; dpnt = ci->Cntry[ci->CurX].RadixPnt ; break ;
	      case UClit('('):
	      case UClit(')'):		sign = UClit("(") ; break ;
	      case UClit('['):		for(j=0,i++;j<UCsizeof(color)&&format[i]!=UClit(']');j++,i++) { color[j] = format[i] ; } ;
					color[j] = UCEOS ; break ;
	    } ;
	 } ;
	UCsprintf(nsrc,UCsizeof(nsrc),UClit("%.*f"),(dps < 0 ? 0 : dps),dnum) ;
	if (rgbColor != NULL)
	 { if (UCempty(color)) { ZUS(rgbColor) ; }
	    else { j = v_ColorNameToRef(color) ; v_Msg(NULL,rgbColor,"@#%1x",v_ColorRefToRGB(j)) ; } ;
	 } ;
	dst[s=0] = UCEOS ;
	for(j=0;;j++) { dst[s++] = prefix[j] ; if(prefix[j] == UCEOS) break ; } ; s-- ;
	if (dnum == 0.0)
	 { for(i=0;i<zeros;i++) { UCstrcat(dst,UClit("0")) ; } ;
	   if (dps > 0)
	    { UCstrcat(dst,UClit(".0")) ; for(i=1;i<dps;i++) { UCstrcat(dst,UClit("0")) ; } ;
	    } ;
	   UCstrcat(dst,suffix) ;
	   return(UCstrlen(dst)) ;
	 } ;
	if (zeros) zeros += UCstrlen(prefix) ;
	for(b=nsrc;*b!=UCEOS;b++) { if (*b == UClit('.')) break ; } ; b1 = b+1 ;
	for(i=0,j=0;b!=nsrc && i<UCsizeof(stack);)
	 { stack[i++] = *(--b) ; j++ ;
	   if (delim != UCEOS && ci->curDDMap[j] && b != nsrc) { stack[i++] = delim ; } ;
	 } ;
	if (i == 1 && stack[0] == UClit('0')) i = 0 ;		/* If got single '0' in stack then clear it out */
	if (neg && sign != NULL) dst[s++] = *sign ;
	for(;s+i<zeros;) { dst[s++] = UClit('0') ; } ;
	for(;i>0;) { dst[s++] = stack[--i] ; } ;
	if (dps > 0)
	 { dst[s++] = dpnt ;
	   for(i=0;i<dps && *b1!=UCEOS;i++,b1++) { dst[s++] = *b1 ; } ;	/* Do decimals */
	   for(;i<dps;i++) { dst[s++] = UClit('0') ; } ;			/*   and any needed trailing 0's */
	 } ;
	if (sign == NULL ? FALSE : *sign == UClit('(')) { dst[s++] = (neg ? UClit(')') : UClit(' ')) ; } ;
	for(j=0;suffix[j]!='\0';j++) { dst[s++] = suffix[j] ; } ;
	dst[s] = '\0' ;
	return(s) ;
}

/*	v_ParseDbl - Routines to parse doubles/integers (ignore commas, spaces, dollar signs)	*/
/*	Call: double = v_ParseDbl(ptr,term)
	  where double is returned double (only valid if *term = UCEOS),
		ptr is pointer to string,
		term is (UCCHAR **) updated to pointer to first non-numeric character		*/

double v_ParseDbl(ptr,term)
  UCCHAR *ptr ;
  UCCHAR **term ;
{ double result = 0, dpFactor ; LOGICAL isNeg = FALSE, haveDP = FALSE ; int exp ; UCCHAR *lterm ;

	if (*ptr == UClit('(')) { isNeg = TRUE ; ptr++ ; } ;
	for(;;ptr++)
	 { switch (*ptr)
	    { default:
		goto done ;
	      case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'):
	      case UClit('5'): case UClit('6'): case UClit('7'): case UClit('8'): case UClit('9'):
		if (haveDP) { dpFactor /= 10.0 ; result += ((*ptr) - UClit('0')) * dpFactor ; }
		 else { result = (result * 10.0) + ((*ptr) - UClit('0')) ; } ;
		break ;
	      case UClit('E'):
	      case UClit('e'):
		exp = UCstrtol(ptr+1,&lterm,10) ; ptr = lterm - 1 ;
		result *= pow(10.0,exp) ; break ;
	      case UClit('.'):
		if (haveDP) goto done ;
		haveDP = TRUE ; dpFactor = 1.0 ; break ;
	      case UClit('-'):
		if (isNeg) goto done ;
		isNeg = TRUE ; break ;
	      case UClit(')'):
		if (!isNeg) goto done ; break ;
	      case UClit(','):
	      case UClit(' '):
	      case UClit('$'):
		break ;
	    } ;
	 } ;
done:
	*term = ptr ; return(isNeg ? -result : result) ;
}


#if defined VAXVMS || defined WINNT || defined LINUX486 || defined HPUX || defined RS6000AIX || defined RASPPI
double nint(fnum)
  double fnum ;
{
	if (fnum < 0) { fnum -= 0.5 ; } else { fnum += 0.5 ; } ;
	return(fnum) ;
}
#endif

#if defined LINUX486 || defined RASPPI
void v_Sleepms(ms)			/* Sleep for ms milliseconds */
  int ms ;
{ struct timeval tev ;

	tev.tv_sec = 0 ; tev.tv_usec = ms * 1000 ;
	select(1,NULL,NULL,NULL,&tev) ;
}
#endif

/*	v_ParseFormatSpecs - Parses different types of format specifications	*/
/*	  Call: ok = v_ParseFormatSpecs(ctx, vfs, type, spec)
	 where	ok is TRUE if parsed ok, FALSE if problems with error in ctx->ErrorMsgAux,
		ctx is context,
		vfs is format structure to update,
		type is type of formatting string (V4SS_FormatType_xxx),
		spec is pointer to ASCIZ format string to be parsed		*/

LOGICAL v_ParseFormatSpecs(ctx,vfs,type,spec)
  struct V4C__Context *ctx ;
  struct V4SS__FormatSpec *vfs ;
  int type ;UCCHAR *spec ;
{ int i, j, len ; UCCHAR *b ; UCCHAR buf[256] ;

	switch (type)
	 { default:
		v_Msg(ctx,ctx->ErrorMsgAux,"FormatSpec1",type) ; return(FALSE) ;
	     case V4SS_FormatType_Init:
		memset(vfs,0,sizeof *vfs) ; vfs->Length = vfs->VarString - (char *)vfs ;
		vfs->FontBold = V4SS_FontBold_Normal ;
		break ;
	     case V4SS_FormatType_Mask:
		i = vfs->Length - (vfs->VarString - (char *)vfs) ;
		len = UCUTF16toUTF8(&vfs->VarString[i],(sizeof *vfs)-vfs->Length-1,spec,UCstrlen(spec)) ;
		if (len < 0)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"FormatLen",sizeof *vfs) ; return(FALSE) ; } ;
		vfs->Length += (1+len) ; vfs->MaskX = i+1 ;
		break ;
	     case V4SS_FormatType_Style:
		for(i=0;spec[i]!=UCEOS;)
		 { for(j=0;j<UCsizeof(buf);)
		    { switch (spec[i])
		       { default:		buf[j++] = spec[i++] ; continue ;
		         case UCEOS:	break ;
		         case UClit(' '):	i++ ; continue ;
			 case UClit(','):	i++ ; break ;
		       } ; break ;
		    } ; buf[j] = UCEOS ; if (j == 0) continue ;
		   switch(v4im_GetUCStrToEnumVal(buf,0))
		    { default: 			v_Msg(ctx,ctx->ErrorMsgAux,"FormatSpec2",buf) ; return(FALSE) ;
		      case _Bold:		vfs->FontBold = V4SS_FontBold_Bold ; vfs->FontAttr |= V4SS_Font_Bold ; break ;
		      case _Center:		vfs->HAlign = V4SS_Align_Center ; break ;
		      case _CSelect:		vfs->HAlign = V4SS_Align_MultiColCenter ; break ;
		      case _CSpan:		vfs->HAlign = V4SS_Align_MultiColCenter ; break ;
		      case _Fill:		vfs->HAlign = V4SS_Align_Fill ; break ;
		      case _Fit:		vfs->HAlign = V4SS_Align_Fill ; break ;
		      case _Italic:		vfs->FontAttr |= V4SS_Font_Italic ; break ;
		      case _Indent:
		      case _Indent1:		vfs->HAlign = V4SS_Align_Indent1 ; break ;
		      case _Indent2:		vfs->HAlign = V4SS_Align_Indent2 ; break ;
		      case _Indent3:		vfs->HAlign = V4SS_Align_Indent3 ; break ;
		      case _Left:
		      case _LeftJustify:	vfs->HAlign = V4SS_Align_Left ; break ;
		      case _LSpan:		vfs->HAlign = V4SS_Align_MultiColLeft ; break ;
		      case _NoBreak:		vfs->FontAttr |= V4SS_Font_NoBreak ; break ;
		      case _Right:
		      case _RightJustify:	vfs->HAlign = V4SS_Align_Right ; break ;
		      case _RSpan:		vfs->HAlign = V4SS_Align_MultiColRight ; break ;
		      case _StrikeThrough:	vfs->FontAttr |= V4SS_Font_StrikeThru ; break ;
		      case _Underline:		vfs->FontAttr |= V4SS_Font_Underline ; break ;
		      case _VBottom:		vfs->VAlign = V4SS_Align_Bottom ; break ;
		      case _VCenter:		vfs->VAlign = V4SS_Align_Center ; break ;
		      case _VTop:		vfs->VAlign = V4SS_Align_Top ; break ;
		      case _Wrap:		vfs->FontAttr |= V4SS_Font_Wrap ; break ;
		    } ;
		 } ;
		break ;
	     case V4SS_FormatType_Color:
	     case V4SS_FormatType_Size:
		vfs->FontSize = UCstrtol(spec,&b,10) ;
		if (*b == UClit('%')) { vfs->FontSize = -vfs->FontSize ; }
		 else if (*b != UCEOS) { v_Msg(ctx,ctx->ErrorMsgAux,"FormatFont",spec) ; return(FALSE) ; } ;
		break ;
	     case V4SS_FormatType_CellColor:
		 vfs->FgColor = v_ColorNameToRef(spec) ;
		 if (vfs->FgColor == 0)
		  { v_Msg(ctx,ctx->ErrorMsgAux,"FormatColor",spec) ; return(FALSE) ; } ;
		 vfs->BgColor = vfs->FgColor ; vfs->FillPattern = 1 ;
		break ;
	     case V4SS_FormatType_TextColor:
		 vfs->FontColor = v_ColorNameToRef(spec) ;
		if (vfs->FontColor == 0)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"FormatColor",spec) ; return(FALSE) ; } ;
		break ;
	     case V4SS_FormatType_Font:
		i = vfs->Length - (vfs->VarString - (char *)vfs) ;
		len = UCUTF16toUTF8(&vfs->VarString[i],(sizeof *vfs)-vfs->Length-1,spec,UCstrlen(spec)) ;
		if (len < 0) { v_Msg(ctx,ctx->ErrorMsgAux,"FormatLen",sizeof *vfs) ; return(FALSE) ; } ;
		vfs->Length += (1+len) ; vfs->FontNameX = i+1 ;
		break ;
	     case V4SS_FormatType_CSSStyle:
		i = vfs->Length - (vfs->VarString - (char *)vfs) ;
		len = UCUTF16toUTF8(&vfs->VarString[i],(sizeof *vfs)-vfs->Length-1,spec,UCstrlen(spec)) ;
		if (len < 0) { v_Msg(ctx,ctx->ErrorMsgAux,"FormatLen",sizeof *vfs) ; return(FALSE) ; } ;
		vfs->Length += (1+len) ; vfs->CSSStyle = i+1 ;
		break ;
	     case V4SS_FormatType_CSSId:
		i = vfs->Length - (vfs->VarString - (char *)vfs) ;
		len = UCUTF16toUTF8(&vfs->VarString[i],(sizeof *vfs)-vfs->Length-1,spec,UCstrlen(spec)) ;
		if (len < 0) { v_Msg(ctx,ctx->ErrorMsgAux,"FormatLen",vfs->Length+len+1,sizeof *vfs) ; return(FALSE) ; } ;
		vfs->Length += (1+len) ; vfs->CSSId = i+1 ;
		break ;
	     case V4SS_FormatType_CSSClass:
		i = vfs->Length - (vfs->VarString - (char *)vfs) ;
		len = UCUTF16toUTF8(&vfs->VarString[i],(sizeof *vfs)-vfs->Length-1,spec,UCstrlen(spec)) ;
		if (len < 0) { v_Msg(ctx,ctx->ErrorMsgAux,"FormatLen",vfs->Length+len+1,sizeof *vfs) ; return(FALSE) ; } ;
		vfs->Length += (1+len) ; vfs->CSSClass = i+1 ;
		break ;
	     case V4SS_FormatType_URLLink:
		i = vfs->Length - (vfs->VarString - (char *)vfs) ;
		len = UCUTF16toUTF8(&vfs->VarString[i],(sizeof *vfs)-vfs->Length-1,spec,UCstrlen(spec)) ;
		if (len < 0) { v_Msg(ctx,ctx->ErrorMsgAux,"FormatLen",sizeof *vfs) ; return(FALSE) ; } ;
		vfs->Length += (1+len) ; vfs->URLLink = i+1 ;
		break ;

	 } ;
	return(TRUE) ;
}








/*	v_VFStoCSS - Converts format specs in vfs to Cascading Style Sheet specs */
/*	Call: ok = v_VFStoCSS( ctx , vfs , sbuf , sbufmax ,  globalfontsize )
	  where ok is TRUE if all went well, FALSE if error (in ctx->ErrorMsgAux),
		ctx is current context,
		vfs is V4SS__FormatSpec pointer,
		sbuf is text string to be updated,
		sbufmax is max bytes in sbuf					*/

void v_VFStoCSS(ctx,vfs,sbuf,globalfontsize)
  struct V4C__Context *ctx ;
  struct V4SS__FormatSpec *vfs ;
  UCCHAR *sbuf ;
{
  UCCHAR tb[512],tb2[128] ; int color ;

	ZUS(sbuf) ;
/*	Determine font color & bgcolor */
	color = (vfs->FgColor != 0 ? vfs->FgColor : 0) ;
	if (color != 0) { v_Msg(ctx,tb,"@background-color: %1U; ",v_ColorRefToHTML(color)) ; UCstrcat(sbuf,tb) ; } ;
	if (vfs->FontColor != 0) { v_Msg(ctx,tb,"@color: %1U; ",v_ColorRefToHTML(vfs->FontColor)) ; UCstrcat(sbuf,tb) ; } ;
/*	Now determine formatting characteristics */
	if (vfs->FontSize > 0) { UCsprintf(tb,UCsizeof(tb),UClit("font-size: %dpt; "),vfs->FontSize) ; UCstrcat(sbuf,tb) ; }
	 else if (vfs->FontSize < 0) { UCsprintf(tb,UCsizeof(tb),UClit("font-size: '%d%%';"),-vfs->FontSize) ; UCstrcat(sbuf,tb) ; } ;
	if (vfs->FontBold > V4SS_FontBold_Normal) UCstrcat(sbuf,UClit("font-weight: bold; ")) ;
	if (vfs->FontAttr & V4SS_Font_Italic) UCstrcat(sbuf,UClit("font-style: italic; ")) ;
	if (vfs->FontAttr & V4SS_Font_Shadow) UCstrcat(sbuf,UClit("text-shadow black: 3px 3px 2px; ")) ;
	if (vfs->FontAttr & V4SS_Font_SizeToFit) UCstrcat(sbuf,UClit("text-overflow: ellipsis ; ")) ;
	ZUS(tb2) ;
	if (vfs->FontAttr & V4SS_Font_StrikeThru) UCstrcat(tb2,UClit("line-through; ")) ;
	if (vfs->FontAttr & V4SS_Font_Underline) UCstrcat(tb2,UClit("underline; ")) ;
	if (UCstrlen(tb2) > 0) { UCstrcat(sbuf,UClit("text-decoration: ")) ; UCstrcat(sbuf,tb2) ; } ;
	if (vfs->FontULStyle != 0) UCstrcat(sbuf,UClit("text-decoration: underline; ")) ;
	switch (vfs->HAlign)
	 { case V4SS_Align_MultiColCenter:
	   case V4SS_Align_Center:		UCstrcat(sbuf,UClit("text-align: center;")) ; break ;
	   case V4SS_Align_MultiColLeft:
	   case V4SS_Align_Left:		UCstrcat(sbuf,UClit("text-align: left;")) ; break ;
	   case V4SS_Align_MultiColRight:
	   case V4SS_Align_Right:		UCstrcat(sbuf,UClit("text-align: right;")) ; break ;
	   case V4SS_Align_Justify:		UCstrcat(sbuf,UClit("text-align: justify;")) ; break ;
	 } ;
	if (vfs->FontNameX != 0)
	 { UCstrcpy(tb2,ASCretUC(&vfs->VarString[vfs->FontNameX-1])) ;
	   v_Msg(ctx,tb,"@ font-family: '%1U'; ",tb2) ; UCstrcat(sbuf,tb) ;
	 } ;
	if (vfs->CSSStyle != 0)
	 { UCstrcpy(tb2,ASCretUC(&vfs->VarString[vfs->CSSStyle-1])) ;
	   v_Msg(ctx,tb,"@ %1U ",tb2) ; UCstrcat(sbuf,tb) ;
	 } ;

}


/*	C O U N T R Y   I N F O R M A T I O N	*/

#define V4CI_CIInitial 250
#define V4CI_CIIncrement 25
#define V4CI_LIInitial 10
#define V4CI_LIIncrement 10
#define V4CI_CLIInitial 10
#define V4CI_CLIIncrement 10

void v_LoadCountryInfo()
{ 
  struct UC__File UCFile ;
  struct V4CI__CountryInfo *ci ;
  struct V4LI_LanguageInfo *li ;
  struct V4LI_CalendarInfo *cli ;
  int max,max2,line,i,j,cal,lan,offset,len ; UCCHAR tval[512] ; UCCHAR wmsg[128],*b,*e,*b1 ; UCCHAR linetype[20],ubuf[512],errmsg[512] ;
#define NEXT b = e + 1 ; e = UCstrchr(b,'\t') ; if (e == NULL) e = UCstrchr(b,'\n') ; if (e == NULL) goto badline ; *e = UCEOS ; if (*b == UClit('"')) { b++ ; b[UCstrlen(b)-1] = UCEOS ; } ;
#define NEXTC b = e + 1 ; e = UCstrchr(b,'\t') ; if (e == NULL) e = UCstrchr(b,'\n') ; if (e == NULL) break ; *e = UCEOS ; if (*b == UClit('"')) { b++ ; b[UCstrlen(b)-1] = UCEOS ; } ;
//#define ALPHA(el) if (strlen(b) >= sizeof ci->Cntry[0].el) goto badline ; strcpy(ci->Cntry[ci->Count].el,b) ;
#define ALPHA(el) if (UCstrlen(b) >= UCsizeof(ci->Cntry[0].el)) goto badline ; UCstrcpy(ci->Cntry[ci->Count].el,b) ;
#define NUM(el) ci->Cntry[ci->Count].el = UCstrtol(b,&b1,10) ; if (*b1 != UCEOS) goto badline ;

	if (!v_UCFileOpen(&UCFile,v_GetV4HomePath(UClit("v4countryinfo.v4i")),UCFile_Open_Read,FALSE,wmsg,0))
	 { v_Msg(NULL,ubuf,"@* Err accessing V4 country-info initialization file: %1U\n",wmsg) ;
	   vout_UCText(VOUT_Warn,0,ubuf) ;
	   ci = (struct V4CI__CountryInfo *)v4mm_AllocChunk(sizeof *ci,TRUE) ;
	   offset = v_mctAlloc(gpi,sizeof *ci,NULL) ; gpi->mct->CountryInfoOffset = offset ;
	   ci = (struct V4CI__CountryInfo *)&gpi->mct->Data[offset] ;
	   ci->CurX = 0 ; ci->Count = 1 ;
	   ci->Cntry[0].IntDialCode = 1 ; UCstrcpy(ci->Cntry[0].ITeleMask,UClit("+c-aaa-nnn-nnnn")) ; UCstrcpy(ci->Cntry[0].NTeleMask,UClit("aaa-nnn-nnnn")) ;
	   UCstrcpy(ci->Cntry[0].DateMask,UClit("0d-mmm-0y")) ; UCstrcpy(ci->Cntry[0].MonthMask,UClit("mmm-yy")) ;
	   UCstrcpy(ci->Cntry[0].DateTimeMask,UClit("dd-mmm-yy:hh:mm:ss")) ;
//	   UCstrcpy(ci->Cntry[0].NumInfo,UClit("$,.")) ;
	   memset(&ci->Cntry[0].ymdOrder,0,sizeof ci->Cntry[0].ymdOrder) ; ci->Cntry[0].ymdOrder[0] = V4LEX_YMDOrder_MDY ; ci->Cntry[0].ymdOrder[1] = V4LEX_YMDOrder_DMY ;
	   UCstrcpy(ci->Cntry[0].CurrencySign,UClit("$")) ; ci->Cntry[0].CurSgnPrefix = TRUE ;
	   ci->Cntry[0].DigiDelim = UClit(',') ;
	   ci->Cntry[0].RadixPnt = UClit('.') ;
	   memset(&ci->Cntry[0].DDList,0,sizeof ci->Cntry[0].DDList) ; ci->Cntry[0].DDList[0] = 3 ;
	   ci->li = (struct V4LI_LanguageInfo *)v4mm_AllocChunk(sizeof *ci->li,FALSE) ;
	   ci->cli = (struct V4LI_CalendarInfo *)v4mm_AllocChunk(sizeof *ci->cli,FALSE) ;
	   goto do_language ;
	 } ;
	ci = (struct V4CI__CountryInfo *)v4mm_AllocChunk(sizeof *ci + V4CI_CIInitial * sizeof ci->Cntry[0],FALSE) ;

/*	Here to load up country file */
	max = V4CI_CIInitial ; ci->Count = 0 ;
	for(line=1;;line++)
	 { e = tval - 1 ;
//	   if (fgets(tval,sizeof tval,fp) == NULL) break ;
	   if (v_UCReadLine(&UCFile,UCRead_UC,tval,UCsizeof(tval),(UCCHAR *)tval) < 0) break ;
	   if (!isdigit(tval[0])) continue ;	/* If not digit then assume comment line */
	   NEXT NUM(UNCode) NEXT ALPHA(UNAbbr) NEXT ALPHA(iso2) NEXT ALPHA(UNName)
	   NEXT NUM(IntDialCode) NEXT NUM(IDD) NEXT NUM(NDD)
	   NEXT ALPHA(ITeleMask) NEXT ALPHA(NTeleMask)
	   NEXT ALPHA(DateMask) NEXT ALPHA(MonthMask) NEXT ALPHA(DateTimeMask) NEXT NUM(Calendar)
	   NEXT NUM(Language)
//	   NEXT ALPHA(NumInfo)
/*	   Parse YMD order info */
	   NEXT memset(&ci->Cntry[ci->Count].ymdOrder,0,sizeof ci->Cntry[ci->Count].ymdOrder) ;
	   for(i=0,j=0;i>=0;i++)
	    { switch (b[i])
	       { default:		ubuf[j++] = b[i] ; break ;
	         case UClit(' '):	break ;
	         case UClit(','):	ubuf[j++] = UCEOS ; break ;
	         case UCEOS:		ubuf[j++] = UCEOS ; i = -999 ; break ;
	       } ;
	    } ; ubuf[j++] = UCEOS ;	/* End with 2 end-of-strings */
	   for(i=0,b=ubuf;i<VCAL_YMDOrderMax&&UCstrlen(b)>0;i++,(b+=(UCstrlen(b)+1)))
	    { int ymdo ;
	      switch (v4im_GetUCStrToEnumVal(b,3))
	       { default:	goto badline ;
		 case _DMY:	ymdo = V4LEX_YMDOrder_DMY ; break ;
		 case _DYM:	ymdo = V4LEX_YMDOrder_DYM ; break ;
		 case _MDY:	ymdo = V4LEX_YMDOrder_MDY ; break ;
		 case _MYD:	ymdo = V4LEX_YMDOrder_MYD ; break ;
		 case _YDM:	ymdo = V4LEX_YMDOrder_YDM ; break ;
		 case _YMD:	ymdo = V4LEX_YMDOrder_YMD ; break ;
	       } ; ci->Cntry[ci->Count].ymdOrder[i] = ymdo ;
	    } ;
	   NEXT ALPHA(CurrencySign) NEXT ci->Cntry[ci->Count].CurSgnPrefix = (b[0] == UClit('Y') || b[0] == UClit('y') || b[0] == UClit('T') || b[0] == UClit('t')) ;
	   NEXT ci->Cntry[ci->Count].DigiDelim = b[0] ;
	   NEXT memset(&ci->Cntry[ci->Count].DDList,0,sizeof ci->Cntry[ci->Count].DDList) ;
	   for (i=0,b1=b;i<sizeof ci->Cntry[ci->Count].DDList&&*b1!=UCEOS;i++,b1++)
	    { ci->Cntry[ci->Count].DDList[i] = UCstrtol(b1,&b1,10) ;
	      if (*b1 == UClit(',')) continue ; if (*b1 == UCEOS) break ; goto badline ;
	    } ;
	   NEXT ci->Cntry[ci->Count].RadixPnt = b[0] ;
	   if (line >= max)			/* Have to increase size */
	    { max += V4CI_CIIncrement ;
	      ci = (struct V4CI__CountryInfo *)realloc(ci,sizeof *ci + max * sizeof ci->Cntry[0]) ;
	    } ;
	   if (ci->Cntry[ci->Count].UNCode == 840) ci->CurX = ci->Count ;	/* Look for USA as default */
	   ci->Count++ ; continue ;
badline:   v_Msg(NULL,wmsg,"@*Invalid line loading (%1U) v4_home:v4CountryInfo.v4i(line %2d)\n",b,line) ; vout_UCText(VOUT_Warn,0,wmsg) ;
	   continue ;
	 } ;
/*	Now allocate space within mct, copy structure in, free temporary structure */
	i = sizeof *ci + (ci->Count * sizeof ci->Cntry[0]) ;
	offset = v_mctAlloc(gpi,i,NULL) ; gpi->mct->CountryInfoOffset = offset ; memcpy(&gpi->mct->Data[offset],ci,i) ;
	v4mm_FreeChunk(ci) ; ci = (struct V4CI__CountryInfo *)&gpi->mct->Data[offset] ;
	v_UCFileClose(&UCFile) ;

/*	Here to load up v4LanguageInfo.v4i		*/
#undef NEXT
#undef NEXTC
#undef ALPHA
#undef NUM		/* Undefine so we can redefine same thing but trapping to badline2 */
#define NEXT b = e + 1 ; e = UCstrchr(b,'\t') ; if (e == NULL) e = UCstrchr(b,'\n') ; if (e == NULL) goto badline2 ; *e = UCEOS ; if (*b == UClit('"')) { b++ ; b[UCstrlen(b)-1] = UCEOS ; } ;
#define NEXTC b = e + 1 ; e = UCstrchr(b,'\t') ; if (e == NULL) e = UCstrchr(b,'\n') ; if (e == NULL) break ; *e = UCEOS ; if (*b == UClit('"')) { b++ ; b[UCstrlen(b)-1] = UCEOS ; } ;
#define ALPHA(el) if (UCstrlen(b) >= UCsizeof(ci->Cntry[0].el)) goto badline2 ; UCstrcpy(ci->Cntry[ci->Count].el,b) ;
#define NUM(el) ci->Cntry[ci->Count].el = UCstrtol(b,&b1,10) ; if (*b1 != UCEOS) goto badline2 ;
do_language:
	if (!v_UCFileOpen(&UCFile,v_GetV4HomePath(UClit("v4languageinfo.v4i")),UCFile_Open_Read,FALSE,ubuf,0))
	 { 
	   v_Msg(NULL,errmsg,"@* Err accessing V4 language-info initialization file: %1U\n",ubuf) ;
	   vout_UCText(VOUT_Warn,0,errmsg) ;
	   offset = v_mctAlloc(gpi,sizeof *li,NULL) ; gpi->mct->LanguageInfoOffset = offset ;
//	   li = (ci->li = (struct V4LI_LanguageInfo *)&gpi->mct->Data[offset]) ;
//	   offset = v_mctAlloc(gpi,sizeof *cli,&li) ; gpi->mct->CalendarInfoOffset = offset ;
//	   cli = (ci->cli = (struct V4LI_CalendarInfo *)&gpi->mct->Data[offset]) ;

	   li = (struct V4LI_LanguageInfo *)v4mm_AllocChunk(sizeof *ci->li + V4CI_LIInitial * sizeof ci->li->LI[0],FALSE) ;
	   cli = (struct V4LI_CalendarInfo *)v4mm_AllocChunk(sizeof *ci->cli + V4CI_CLIInitial * sizeof ci->cli->CLI[0],FALSE) ;
	   max = V4CI_LIInitial ; li->Count = 0 ; max2 = V4CI_CLIInitial ; cli->Count = 0 ;

	   li->Count = 1 ; li->CurX = 0 ; li->LI[0].Language = 0 ; UCstrcpy(li->LI[0].LangId,UClit("USEnglish")) ;
#define L(f,n) UCstrcpy(li->LI[0].f,UClit(n)) ;
	   L(Yes,"Yes") L(No,"No") L(Yesterday,"Yesterday") L(Today,"Today") L(Tomorrow,"Tomorrow") L(Now,"Now") L(True,"True") L(False,"False") L(None,"None") L(None1UC,"NONE")
	   cli->Count = 1 ; cli->CurX = 0 ; cli->CLI[0].Language = 0 ; cli->CLI[0].Calendar = VCAL_CalType_Gregorian ;
#define SD(name) UCstrcpy(cli->CLI[0].SDaysOfWeek[i++],UClit(name)) ;
#define LD(name) UCstrcpy(cli->CLI[0].LDaysOfWeek[i++],UClit(name)) ;
	   i = 0 ; SD("Mon") SD("Tue") SD("Wed") SD("Thu") SD("Fri") SD("Sat") SD("Sun")
	   i = 0 ; LD("Monday") LD("Tuesday") LD("Wednesday") LD("Thursday") LD("Friday") LD("Saturday") LD("Sunday")
#define SM(name) UCstrcpy(cli->CLI[0].SMonths[i++],UClit(name)) ;
#define LM(name) UCstrcpy(cli->CLI[0].LMonths[i++],UClit(name)) ;
	   i = 0 ; SM("Jan") SM("Feb") SM("Mar") SM("Apr") SM("May") SM("Jun") SM("Jul") SM("Aug") SM("Sep") SM("Oct") SM("Nov") SM("Dec")
	   i = 0 ; LM("January") LM("February") LM("March") LM("April") LM("May") LM("June")
		   LM("July") LM("August") LM("September") LM("October") LM("November") LM("December")
	   ci = (struct V4CI__CountryInfo *)&gpi->mct->Data[gpi->mct->CountryInfoOffset] ;
	   ci->li = li ; ci->cli = cli ;
	   goto end_langSetup ;
	 } ;

	uci = (struct V__UnicodeInfo *)&gpi->mct->Data[gpi->mct->UnicodeOffset] ;
	li = (struct V4LI_LanguageInfo *)v4mm_AllocChunk(sizeof *ci->li + V4CI_LIInitial * sizeof ci->li->LI[0],FALSE) ;
	cli = (struct V4LI_CalendarInfo *)v4mm_AllocChunk(sizeof *ci->cli + V4CI_CLIInitial * sizeof ci->cli->CLI[0],FALSE) ;
/*	Here to load up language file */
	max = V4CI_LIInitial ; li->Count = 0 ; max2 = V4CI_CLIInitial ; cli->Count = 0 ;
#define NLI(el) NEXT UCstrncpy(li->LI[i].el,b,UCsizeof(li->LI[i].el)) ; li->LI[i].el[UCsizeof(li->LI[i].el)-1] = UCEOS ;
	for(line=1;;line++)
	 { e = tval - 1 ;
//	   if (fgets(tval,sizeof tval,fp) == NULL) break ;
	   if (v_UCReadLine(&UCFile,UCRead_UC,tval,sizeof tval,errmsg) < 0) break ;
	   len = UCstrlen(tval) ;
	   if (tval[len-1] == UCNL && tval[len-2] == UClit('\r'))
	    { tval[len-2] = UCNL ; tval[len-1] = UCEOS ; } ;
	   if (UCstrlen(tval) <= 2) continue ;
	   NEXT if (UCempty(b)) continue ;	/* If first column empty then assume comment */
		if (UCstrlen(b) >= UCsizeof(linetype)) goto badline2 ; UCstrcpy(linetype,b) ;
	   NEXT lan = UCstrtol(b,&b1,10) ; if (*b1 != UCEOS) goto badline2 ;
	   NEXT cal = UCstrtol(b,&b1,10) ; if (*b1 != UCEOS) goto badline2 ;
	   switch (v4im_GetUCStrToEnumVal(linetype,0))
	    { default:
		v_Msg(NULL,errmsg,"@*Invalid line prefix (%1U) in v4_home:v4LanguageInfo.v4i(line %2d)\n",linetype,line) ; vout_UCText(VOUT_Warn,0,errmsg) ; continue ;
	      case _SDOW:
		for(i=0;i<cli->Count;i++) { if (cli->CLI[i].Language == lan && cli->CLI[i].Calendar == cal) break ; } ;
		if (i >= cli->Count) { i = cli->Count ; cli->Count ++ ; cli->CLI[i].Language = lan ; cli->CLI[i].Calendar = cal ; } ;
		for(j=0;j<7;j++) { NEXT if (UCstrlen(b) >= UCsizeof(cli->CLI[i].SDaysOfWeek[j])) goto badline2 ; UCstrcpy(cli->CLI[i].SDaysOfWeek[j],b) ; } ;
		break ;
	      case _LDOW:
		for(i=0;i<cli->Count;i++) { if (cli->CLI[i].Language == lan && cli->CLI[i].Calendar == cal) break ; } ;
		if (i >= cli->Count) { i = cli->Count ; cli->Count ++ ; cli->CLI[i].Language = lan ; cli->CLI[i].Calendar = cal ; } ;
		for(j=0;j<7;j++) { NEXT if (UCstrlen(b) >= UCsizeof(cli->CLI[i].LDaysOfWeek[j])) goto badline2 ; UCstrcpy(cli->CLI[i].LDaysOfWeek[j],b) ; } ;
		break ;
	      case _SMOY:
		for(i=0;i<cli->Count;i++) { if (cli->CLI[i].Language == lan && cli->CLI[i].Calendar == cal) break ; } ;
		if (i >= cli->Count) { i = cli->Count ; cli->Count ++ ; cli->CLI[i].Language = lan ; cli->CLI[i].Calendar = cal ; } ;
		for(j=0;j<24;j++)
		 { NEXTC
		   if (UCstrlen(b) >= UCsizeof(cli->CLI[i].SMonths[j])) goto badline2 ;
		   UCstrcpy(cli->CLI[i].SMonths[j],b) ;
		 } ; break ;
	      case _LMOY:
		for(i=0;i<cli->Count;i++) { if (cli->CLI[i].Language == lan && cli->CLI[i].Calendar == cal) break ; } ;
		if (i >= cli->Count) { i = cli->Count ; cli->Count ++ ; cli->CLI[i].Language = lan ; cli->CLI[i].Calendar = cal ; } ;
		for(j=0;j<24;j++) { NEXTC if (UCstrlen(b) >= UCsizeof(cli->CLI[i].LMonths[j])) goto badline2 ; UCstrcpy(cli->CLI[i].LMonths[j],b) ; } ;
		break ;
	      case _CMask:
		for(i=0;i<cli->Count;i++) { if (cli->CLI[i].Language == lan && cli->CLI[i].Calendar == cal) break ; } ;
		if (i >= cli->Count) { i = cli->Count ; cli->Count ++ ; cli->CLI[i].Language = lan ; cli->CLI[i].Calendar = cal ; } ;
		NEXT if (UCstrlen(b) >= UCsizeof(cli->CLI[i].CalMask)) goto badline2 ; UCstrcpy(cli->CLI[i].CalMask,b) ;
		NEXT if (UCstrlen(b) >= UCsizeof(cli->CLI[i].CalTimeMask)) goto badline2 ; UCstrcpy(cli->CLI[i].CalTimeMask,b) ;
		break ;
	      case _RelTime:
		for(i=0;i<li->Count;i++) { if (li->LI[i].Language == lan) break ; } ;
		if (i >= li->Count) { i = li->Count ; li->LI[i].Language = lan ; li->Count ++ ; } ;
		NLI(Yesterday) NLI(Today) NLI(Tomorrow) NLI(Now) NLI(Hours) NLI(Minutes) NLI(Seconds)
		UCcnvupper(li->LI[i].YesterdayUC,li->LI[i].Yesterday,UCsizeof(li->LI[i].YesterdayUC)) ;
		UCcnvupper(li->LI[i].TodayUC,li->LI[i].Today,UCsizeof(li->LI[i].TodayUC)) ;
		UCcnvupper(li->LI[i].TomorrowUC,li->LI[i].Tomorrow,UCsizeof(li->LI[i].TomorrowUC)) ;
		UCcnvupper(li->LI[i].NowUC,li->LI[i].Now,UCsizeof(li->LI[i].NowUC)) ;
		UCcnvupper(li->LI[i].HoursUC,li->LI[i].Hours,UCsizeof(li->LI[i].HoursUC)) ;
		UCcnvupper(li->LI[i].MinutesUC,li->LI[i].Minutes,UCsizeof(li->LI[i].MinutesUC)) ;
		UCcnvupper(li->LI[i].SecondsUC,li->LI[i].Seconds,UCsizeof(li->LI[i].SecondsUC)) ;
		break ;
	      case _Distance:
		for(i=0;i<li->Count;i++) { if (li->LI[i].Language == lan) break ; } ;
		if (i >= li->Count) { i = li->Count ; li->LI[i].Language = lan ; li->Count ++ ; } ;
		for(j=0;j<V4LI_DstMax;)
		 { int tz,k ;
		   NEXTC
		   b1 = UCstrchr(b,'=') ; if (b1 == NULL) goto badline2 ;
		   tz = UCstrtol(b1+1,&b1,10) ;
		   if (!(*b1 == UClit('\t') || *b1 == UCEOS)) goto badline2 ;
		   b1 = UCstrchr(b,'=') ; *b1 = UCEOS ;
		   for(;j<V4LI_DstMax;)
		    { b1 = UCstrchr(b,',') ; if (b1 != NULL) *b1 = UCEOS ; if (UCstrlen(b) >= UCsizeof(li->LI[i].Dst[j].dstAbbr)) goto badline2 ;
		      for(k=0;;k++,b++) { li->LI[i].Dst[j].dstAbbr[k] = UCTOUPPER(*b) ; if (*b == UCEOS) break ; } ; li->LI[i].Dst[j].dstIntVal = tz ;
		      j++ ; if (b1 == NULL) break ; b = b1 + 1 ;
		    } ;
		 } ;
		for(;j<V4LI_DstMax;j++) { ZUS(li->LI[i].Dst[j].dstAbbr) ; li->LI[i].Dst[j].dstIntVal = UNUSED ; } ;
		break ;
	      case _Speed:
		for(i=0;i<li->Count;i++) { if (li->LI[i].Language == lan) break ; } ;
		if (i >= li->Count) { i = li->Count ; li->LI[i].Language = lan ; li->Count ++ ; } ;
		for(j=0;j<V4LI_SpdMax;)
		 { int tz,k ;
		   NEXTC
		   b1 = UCstrchr(b,'=') ; if (b1 == NULL) goto badline2 ;
		   tz = UCstrtol(b1+1,&b1,10) ;
		   if (!(*b1 == UClit('\t') || *b1 == UCEOS)) goto badline2 ;
		   b1 = UCstrchr(b,'=') ; *b1 = UCEOS ;
		   for(;j<V4LI_SpdMax;)
		    { b1 = UCstrchr(b,',') ; if (b1 != NULL) *b1 = UCEOS ; if (UCstrlen(b) >= UCsizeof(li->LI[i].Spd[j].spdAbbr)) goto badline2 ;
		      for(k=0;;k++,b++) { li->LI[i].Spd[j].spdAbbr[k] = UCTOUPPER(*b) ; if (*b == UCEOS) break ; } ; li->LI[i].Spd[j].spdIntVal = tz ;
		      j++ ; if (b1 == NULL) break ; b = b1 + 1 ;
		    } ;
		 } ;
		for(;j<V4LI_SpdMax;j++) { ZUS(li->LI[i].Spd[j].spdAbbr) ; li->LI[i].Spd[j].spdIntVal = UNUSED ; } ;
		break ;
      case _GeoLbl:
		for(i=0;i<li->Count;i++) { if (li->LI[i].Language == lan) break ; } ;
		if (i >= li->Count) { i = li->Count ; li->LI[i].Language = lan ; li->Count ++ ; } ;
#define NGC(FLD) NEXTC if(UCstrlen(b) >= UCsizeof(li->LI[i].FLD)) goto badline2 ; UCstrcpy(li->LI[i].FLD,b) ;
		NGC(gcLatitude) NGC(gcLongitude) NGC(gcHeight) NGC(gcTimeZone) NGC(gcDistance) NGC(gcSpeed)
		NGC(gcDateTime) NGC(gcAzimuth) NGC(gcElevation) NGC(gcNorth) NGC(gcSouth) NGC(gcEast) NGC(gcWest)
		break ;
      case _TZones:
		for(i=0;i<li->Count;i++) { if (li->LI[i].Language == lan) break ; } ;
		if (i >= li->Count) { i = li->Count ; li->LI[i].Language = lan ; li->Count ++ ; } ;
		NEXTC if (UCstrlen(b) >= UCsizeof(li->LI[i].tzLocal)) goto badline2 ; UCstrcpy(li->LI[i].tzLocal,b) ;
		for(j=0;j<V4LI_TZMax;)
		 { int tz,k ;
		   NEXTC
		   b1 = UCstrchr(b,'=') ; if (b1 == NULL) goto badline2 ;
		   tz = UCstrtol(b1+1,&b1,10) ;
		   if (!(*b1 == UClit('\t') || *b1 == UCEOS)) goto badline2 ;
		   b1 = UCstrchr(b,'=') ; *b1 = UCEOS ;
		   for(;j<V4LI_TZMax;)
		    { b1 = UCstrchr(b,',') ; if (b1 != NULL) *b1 = UCEOS ; if (UCstrlen(b) >= UCsizeof(li->LI[i].TZ[j].tzAbbr)) goto badline2 ;
		      for(k=0;;k++,b++) { li->LI[i].TZ[j].tzAbbr[k] = UCTOUPPER(*b) ; if (*b == UCEOS) break ; } ; li->LI[i].TZ[j].tzOffset = tz ;
		      j++ ; if (b1 == NULL) break ; b = b1 + 1 ;
		    } ;
		 } ; break ;
	      case _Logical:
		for(i=0;i<li->Count;i++) { if (li->LI[i].Language == lan) break ; } ;
		if (i >= li->Count) { i = li->Count ; li->LI[i].Language = lan ; li->Count ++ ; } ;
		NLI(Yes) NLI(No) NLI(True) NLI(False)
		UCcnvupper(li->LI[i].YesUC,li->LI[i].Yes,UCsizeof(li->LI[i].YesUC)) ; UCcnvupper(li->LI[i].NoUC,li->LI[i].No,UCsizeof(li->LI[i].NoUC)) ;
		UCcnvupper(li->LI[i].TrueUC,li->LI[i].True,UCsizeof(li->LI[i].TrueUC)) ; UCcnvupper(li->LI[i].FalseUC,li->LI[i].False,UCsizeof(li->LI[i].FalseUC)) ;
//		for(j=0;j<UCsizeof(li->LI[0].Yes);j++)
//		 { li->LI[i].YesUC[j] = UCtoupper(li->LI[i].Yes[j]) ; li->LI[i].NoUC[j] = UCtoupper(li->LI[i].No[j]) ;
//		   li->LI[i].TrueUC[j] = UCtoupper(li->LI[i].True[j]) ; li->LI[i].FalseUC[j] = UCtoupper(li->LI[i].False[j]) ;
//		 } ;
		break ;
	      case _NoValue:
		for(i=0;i<li->Count;i++) { if (li->LI[i].Language == lan) break ; } ;
		if (i >= li->Count) { i = li->Count ; li->LI[i].Language = lan ; li->Count ++ ; } ;
		NLI(None) NLI(None2UC) NLI(None3UC)
		UCcnvupper(li->LI[i].None1UC,li->LI[i].None,UCsizeof(li->LI[i].None1UC)) ; UCSTRTOUPPER(li->LI[i].None2UC) ; UCSTRTOUPPER(li->LI[i].None3UC) ;
//		for(j=0;j<UCsizeof(li->LI[0].None);j++)
//		 { li->LI[i].None1UC[j] = UCTOUPPER(li->LI[i].None[j]) ;
//		   li->LI[i].None2UC[j] = UCTOUPPER(li->LI[i].None2UC[j]) ;
//		   li->LI[i].None3UC[j] = UCTOUPPER(li->LI[i].None3UC[j]) ;
//		 } ;
		break ;
	      case _SNums:
		for(i=0;i<li->Count;i++) { if (li->LI[i].Language == lan) break ; } ;
		if (i >= li->Count) { i = li->Count ; li->LI[i].Language = lan ; li->Count ++ ; } ;
		for(j=0;j<20;j++) { NEXT if (UCstrlen(b) >= UCsizeof(li->LI[j].Num120[0])) goto badline2 ; UCstrcpy(li->LI[i].Num120[j],b) ; } ;
		break ;
	      case _Tens2090:
		for(i=0;i<li->Count;i++) { if (li->LI[i].Language == lan) break ; } ;
		if (i >= li->Count) { i = li->Count ; li->LI[i].Language = lan ; li->Count ++ ; } ;
		for(j=0;j<8;j++) { NEXT if (UCstrlen(b) >= UCsizeof(li->LI[j].Tens2090[0])) goto badline2 ; UCstrcpy(li->LI[i].Tens2090[j],b) ; } ;
		break ;
	      case _Num000:
		for(i=0;i<li->Count;i++) { if (li->LI[i].Language == lan) break ; } ;
		if (i >= li->Count) { i = li->Count ; li->LI[i].Language = lan ; li->Count ++ ; } ;
		for(j=0;j<8;j++) { NEXT if (UCstrlen(b) >= UCsizeof(li->LI[j].Num000[0])) goto badline2 ; UCstrcpy(li->LI[i].Num000[j],b) ; } ;
		break ;
	      case _OrdSfx:
		for(i=0;i<li->Count;i++) { if (li->LI[i].Language == lan) break ; } ;
		if (i >= li->Count) { i = li->Count ; li->LI[i].Language = lan ; li->Count ++ ; } ;
		for(j=0;j<10;j++) { NEXT if (UCstrlen(b) >= UCsizeof(li->LI[j].OrdSfx[0])) goto badline2 ; UCstrcpy(li->LI[i].OrdSfx[j],b) ; } ;
		break ;
	      case _HTML:
		for(i=0;i<li->Count;i++) { if (li->LI[i].Language == lan) break ; } ;
		if (i >= li->Count) { i = li->Count ; li->LI[i].Language = lan ; li->Count ++ ; } ;
		for(j=0;j<V4LI_HTMLMax;j++) { NEXTC if (UCstrlen(b) >= UCsizeof(li->LI[j].htmlTag[0])) goto badline2 ; UCstrcpy(li->LI[i].htmlTag[j],b) ; } ;
		break ;
	      case _LangId:
		for(i=0;i<li->Count;i++) { if (li->LI[i].Language == lan) break ; } ;
		if (i >= li->Count) { i = li->Count ; li->LI[i].Language = lan ; li->Count ++ ; } ;
		NEXT if (UCstrlen(b) >= UCsizeof(li->LI[j].LangId)) goto badline2 ; UCstrcpy(li->LI[i].LangId,b) ;
		break ;
	    } ;
	   if (li->Count >= max)			/* Have to increase size */
	    { max += V4CI_LIIncrement ; li = (struct V4LI_LanguageInfo *)realloc(li,sizeof *li + max * sizeof li->LI[0]) ; } ;
	   if (cli->Count >= max2)
	    { max2 += V4CI_CLIIncrement ; cli = (struct V4LI_CalendarInfo *)realloc(cli,sizeof *cli + max2 * sizeof cli->CLI[0]) ; } ;
	   continue ;
badline2:   v_Msg(NULL,wmsg,"@*Invalid line (%1U) loading v4_home:v4languageinfo.v4i(line %2d, e=%3d, b=%4d)\n",b,line,e-tval,b-tval) ; vout_UCText(VOUT_Warn,0,wmsg) ;
	   continue ;
	 } ;
//	fclose(fp) ;
	v_UCFileClose(&UCFile) ;
end_langSetup:
/*	Now copy li & cli into mct & free up local */
	offset = v_mctAlloc(gpi,sizeof *li + max * sizeof li->LI[0],NULL) ; gpi->mct->LanguageInfoOffset = offset ;
	memcpy(&gpi->mct->Data[offset],li,sizeof *li + max * sizeof li->LI[0]) ;
	v4mm_FreeChunk(li) ; li = (struct V4LI_LanguageInfo *)&gpi->mct->Data[offset] ;
	offset = v_mctAlloc(gpi,sizeof *cli + max2 * sizeof cli->CLI[0],&li) ; gpi->mct->CalendarInfoOffset = offset ;
	memcpy(&gpi->mct->Data[offset],cli,sizeof *cli + max2 * sizeof cli->CLI[0]) ;
	v4mm_FreeChunk(cli) ; cli = (struct V4LI_CalendarInfo *)&gpi->mct->Data[offset] ;
	ci = (struct V4CI__CountryInfo *)&gpi->mct->Data[gpi->mct->CountryInfoOffset] ;
	ci->li = li ; ci->cli = cli ;

	lan = ci->Cntry[ci->CurX].Language ; cal = ci->Cntry[ci->CurX].Calendar ;
	for(li->CurX=0;li->CurX<li->Count;li->CurX++) { if (li->LI[li->CurX].Language == lan) break ; } ;
	if (li->CurX >= li->Count) li->CurX = 0 ;
	for(cli->CurX=0;cli->CurX<cli->Count;cli->CurX++) { if (cli->CLI[cli->CurX].Language == lan && cli->CLI[cli->CurX].Calendar == cal) break ; } ;
	if (cli->CurX >= cli->Count) cli->CurX = 0 ;
	v_setCurrentCountry(ci,ci->CurX) ;
	return ;
}

/*	v_setCurrentCountry - Sets internal structures to country in ci->Cntry[curx] */

void v_setCurrentCountry(ci,curx)
  struct V4CI__CountryInfo *ci ;
  int curx ;
{ int i,j,k ;
	ci->CurX = curx ;				/* Set current country index within ci->Cntry[] */
	ci->li->CurX = ci->Cntry[curx].Language ;	/* Set current language as default from new current country */
	memset(&ci->curDDMap,0,sizeof ci->curDDMap) ;
	for(i=0,j=0,k=ci->Cntry[curx].DDList[0];i<sizeof ci->curDDMap;i++)
	 { if (i != k) continue ;			/* Insert delimiter in this position? */
	   ci->curDDMap[i] = TRUE ;			/*  yes */
	   if (ci->Cntry[curx].DDList[j+1] != 0) j++ ;	/* Get increment to next position */
	   k += ci->Cntry[curx].DDList[j] ;		/* Set k to next position */
	 } ;
}

int v_CountryNameToRef(name)
  UCCHAR *name ;
{ 
  int i ;

	for(i=0;i<gpi->ci->Count;i++)
	 { if (UCstrcmpIC(name,gpi->ci->Cntry[i].UNAbbr) == 0) return(gpi->ci->Cntry[i].UNCode) ; } ;
	return(0) ;
}

UCCHAR *v_CountryRefToName(cref)
  int cref ;
{ 
  int i ; static UCCHAR err[16] ;

	if (cref == UNUSED) return(gpi->ci->li->LI[gpi->ci->li->CurX].None) ;
	for(i=0;i<gpi->ci->Count;i++)
	 { if (cref == gpi->ci->Cntry[i].UNCode) { return(gpi->ci->Cntry[i].UNAbbr) ; } ; } ;
	UCsprintf(err,UCsizeof(err),UClit("?%d?"),cref) ; return(err) ;
}

LOGICAL v_IsNoneLiteral(lit)
  UCCHAR *lit ;
{ 

	if (vuc_StrCmpIC(lit,gpi->ci->li->LI[gpi->ci->li->CurX].None1UC,0) == 0) return(TRUE) ;
	if (vuc_StrCmpIC(lit,gpi->ci->li->LI[gpi->ci->li->CurX].None2UC,0) == 0) return(TRUE) ;
	if (vuc_StrCmpIC(lit,gpi->ci->li->LI[gpi->ci->li->CurX].None3UC,0) == 0) return(TRUE) ;
	return(FALSE) ;
}


/*	T E L E P H O N E   P A R S I N G			*/

LOGICAL v_parseTeleNum(ctx,di,respnt,acprefix,tnp,errbuf)
  struct V4C__Context *ctx ;
  struct V4DPI__DimInfo *di ;
  P *respnt ;
  UCCHAR *acprefix ;
  UCCHAR *tnp ;
  UCCHAR *errbuf ;
{
  int cc,ac,cdigits,coptdigits,optdigits,digits,cnt,num,ext,type ; UCCHAR *tmask,*tnpbase,*tp ; B64INT num1,num2 ;
#undef DN
#define DN(var) var=0 ; for(;;tnp++) { if (*tnp != UClit(' ')) break ; } ; for(;;tnp++) { if (*tnp >= UClit('0') && *tnp <= UClit('9')) { var*=10 ; var+= *tnp - '0' ; } else { break ; } ; } ;

	tnpbase = tnp ;
	tmask = gpi->ci->Cntry[gpi->ci->CurX].NTeleMask ;
	for(cdigits=0,coptdigits=0,optdigits=0,digits=0,cnt=0;*tmask!=UCEOS;tmask++)
	 { switch (*tmask)
	    { default:		continue ;
	      case UClit('N'):		optdigits++ ; break ;
	      case UClit('n'):		digits++ ; break ;
	      case UClit('a'):		cdigits++ ; break ;
	      case UClit('A'):		coptdigits++ ; break ;
	    } ; cnt++ ;
	 } ;
	if (cnt > 10) { v_Msg(ctx,errbuf,"TeleMaskTooBig",gpi->ci->Cntry[gpi->ci->CurX].NTeleMask) ; return(FALSE) ; } ;
	cc = UNUSED ; ac = UNUSED ; num = UNUSED ; type = UNUSED ; ext = UNUSED ;
/*	If number starts with alpha then assume it is the phone type (or NONE) */
	if (vuc_IsAlpha(*tnp))
	 { UCCHAR tbuf[16] ; INDEX i ;
	   if (v_IsNoneLiteral(tnp)) { ac = 0 ; num = 0 ; goto checktele ; } ;
	   if (v4im_MakePTeleAccept(ctx,di,respnt,tnp,NULL,NULL)) goto tele_done ;
	   for(i=0;i<UCsizeof(tbuf) && vuc_IsAlpha(*tnp);i++,tnp++) { tbuf[i] = *tnp ; } ;
	   tbuf[i] = UCEOS ;
	   if (*tnp != UClit(':')) { v_Msg(ctx,errbuf,"TeleType",tnpbase) ; return(FALSE) ; } ;
	   for(tnp++;*tnp == UClit(' ');tnp++) { } ;
	   switch (v4im_GetUCStrToEnumVal(tbuf,0))
	    { default:		v_Msg(ctx,errbuf,"TeleType",tnpbase) ; return(FALSE) ;
	      case _Cell:	type = V4DPI_TeleType_Cell ; break ;
	      case _Fax:	type = V4DPI_TeleType_Fax ; break ;
	      case _Home:	type = V4DPI_TeleType_Home ; break ;
	      case _Work:	type = V4DPI_TeleType_Work ; break ;
	    } ;
	 } ;
	if (*tnp == UClit('+'))
	 { tnp++ ; DN(cc) ; if (*tnp == UClit('-')) tnp++ ; } ;
	if (*tnp == UClit('('))
	 { tnp++ ; DN(ac) ;
	   if (*tnp != UClit(')')) { v_Msg(ctx,errbuf,"TeleAreaCode",tnpbase) ; return(FALSE) ; } ;
	   tnp++ ;
	 } ;
	for(num1=0,num2=0,cnt=0;*tnp!=UCEOS;tnp++)
	 { switch (*tnp)
	    { case UCEOS:		break ;
	      case UClit(' '):		if (ac == UNUSED && num1 > 0) { ac = num1 ; num1 = 0 ; } ; continue ;
	      case UClit('.'):
	      case UClit('-'):		if (ac == UNUSED && num1 > 0) { ac = num1 ; num1 = 0 ; } ; continue ;
	      default:
		v_Msg(ctx,errbuf,"TeleBadChar",tnp-tnpbase,tnpbase) ; return(FALSE) ;
	      case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'): case UClit('5'): case UClit('6'): case UClit('7'): case UClit('8'): case UClit('9'):
		num1 *= 10 ; num1 += *tnp - UClit('0') ; num2 *= 10 ; num2 += *tnp - UClit('0') ; cnt++ ;
		break ;
	      case UClit('x'): case UClit('X'):
		tnp++ ; DN(ext) ;
		if (*tnp != UCEOS) { v_Msg(ctx,errbuf,"TeleExtension",tnpbase) ; return(FALSE) ; } ;
		tnp-- ; break ;
	    } ;
	 } ;
	if (cnt == 0) { ac = 0 ; num = 0 ; goto checktele ; } ;		/* Nothing given - assume 'none' */
	if (ac == UNUSED)
	 { if (cnt > digits + optdigits)	/* Have more than max - must have area code also */
	    { if (optdigits != 0) { v_Msg(ctx,errbuf,"TeleAreaCodeDlm",tnpbase) ; return(FALSE) ; } ;
	      num = num1 % ipowers[digits+optdigits] ;
	      ac = num1 / ipowers[digits+optdigits] ;
	      if (cnt > digits+optdigits+cdigits+coptdigits)
	       { if (ac / ipowers[cdigits+coptdigits] != 1) { v_Msg(ctx,errbuf,"TeleMaxDigits",tnpbase) ; return(FALSE) ; } ;
	         ac = ac % ipowers[cdigits+coptdigits] ; cnt-- ;	/* ac starts with 1 - strip it off */
	       } ;
	      if (ac == 0 && acprefix != NULL)
	       { ac = UCstrtol(acprefix,&tp,10) ; }
	       else { cnt -= (digits+optdigits) ;
		      if (cnt < cdigits || cnt > cdigits + coptdigits) { v_Msg(ctx,errbuf,"TeleBadLen",tnpbase) ; return(FALSE) ; } ;
		    } ;
	      if ((ac < ipowers[cdigits-1] || ac >= ipowers[cdigits+coptdigits]) && ac != 0) { v_Msg(ctx,errbuf,"TeleAreaBad",tnpbase) ; return(FALSE) ; } ;
	      goto checktele ;
	    } ;
	   if (cnt < digits) { v_Msg(ctx,errbuf,"TeleShort",tnpbase) ; return(FALSE) ; } ;
	   num = num2 ; goto checktele ;
	 } ;
/*	ac = num up to first delim, num1 = number after delimiter, num2 = total number */
	if (num1 < ipowers[digits-1])		/* Don't have enough for number */
	 { if (cdigits > 0)  { v_Msg(ctx,errbuf,"TeleShort",tnpbase) ; return(FALSE) ; } ;
	   num = num2 ; ac = 0 ;
	 } else
	 { num = num1 ;
	   if (cdigits < 1) { v_Msg(ctx,errbuf,"TeleAreaBad",tnpbase) ; return(FALSE) ; } ;
	   if ((ac < ipowers[cdigits-1] || ac >= ipowers[cdigits+coptdigits]) && ac != 0) { v_Msg(ctx,errbuf,"TeleAreaBad",tnpbase) ; return(FALSE) ; } ;
	 } ;
checktele:
	ZPH(respnt) ; respnt->Bytes = V4PS_Tele ; respnt->PntType = V4DPI_PntType_TeleNum ; respnt->Dim = di->DimId ;
	if (ac == UNUSED || num == UNUSED || ac > 999 || ((num != 0) && (num > 99999999 || num < 1000000))) { v_Msg(ctx,errbuf,"TeleSyntax",tnp-tnpbase,tnpbase) ; return(FALSE) ; } ;
	respnt->Value.Tele.IntDialCode = (cc == UNUSED ? gpi->ci->Cntry[gpi->ci->CurX].IntDialCode : cc) ;
	respnt->Value.Tele.AreaCode = ac ; respnt->Value.Tele.Number = num ;
	if (ext != UNUSED || type != UNUSED)
	 { respnt->Value.Tele.IntDialCode = -respnt->Value.Tele.IntDialCode ;
	   if (ext == UNUSED) { respnt->Value.Tele.Extension = 0 ; }
	    else { respnt->Value.Tele.Extension = ext ;
		   if (ext != respnt->Value.Tele.Extension) { v_Msg(ctx,errbuf,"TeleSyntax",tnp-tnpbase,tnpbase) ; return(FALSE) ; } ;
		 } ;
	   respnt->Value.Tele.Type = (type == UNUSED ? 0 : type) ;
	 } ;
tele_done:

	return(TRUE) ;
}


/*	v_URLAddressLookup	- Attempts to resolve URL specification to IP address	*/
/*	Call: ip = v_URLAddressLookup( urladdress , errbuf )
	  where ip = ptr to string IP address, NULL if cannot resolve (NOTE: static location used & may be overwritten!),
		urladdress is URL specification, NULL for address of current host,
		errbuf is error message buffer	*/

UCCHAR *v_URLAddressLookup(urladdress,errbuf)
  UCCHAR *urladdress, *errbuf ;
{ 
#define LCMAX 20			/* Don't go overboard on caches */
  struct lcl__cache {
    int Count ;				/* Number below */
    struct {
      UCCHAR URL[64] ;
      UCCHAR IPAddress[16] ;
     } Entry[LCMAX] ;
   } ;
  static struct lcl__cache *lc = NULL ;
  static UCCHAR result[NI_MAXHOST] ;
#ifdef V4ENABLEMULTITHREADS
  static DCLSPINLOCK urlLock = UNUSEDSPINLOCKVAL ;
#endif
  struct addrinfo hints, *addrRes, *res;
  int ok ;

	SOCKETINIT
/*	If no URL then get our own address */
	if (urladdress == NULL)
	 { char szHost[NI_MAXHOST] ; gethostname(szHost,sizeof szHost) ; UCstrcpyAtoU(result,szHost) ; urladdress = result ; } ;
	if (UCempty(urladdress)) return(NULL) ;
	memset(&hints,0,sizeof hints) ;
	 hints.ai_family = AF_UNSPEC ; hints.ai_socktype = SOCK_STREAM ; hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
	UCgetaddrinfo(ok,urladdress,NULL,&hints,&addrRes) ;
	if (ok != 0)
	 { v_Msg(NULL,errbuf,"SystemCallFail2","getaddrinfo()",errno) ;
	   return(NULL) ;
	 } ;
	for(res=addrRes;res!=NULL;res=res->ai_next)
	 { switch (res->ai_family)
	    { default:		continue ;
	      case AF_INET6:	continue ;	/* Skip v6 for now */
	      case AF_INET:			/* Just take v4 */
		UCgetnameinfo(ok,res->ai_addr, res->ai_addrlen,result,NI_MAXHOST,NULL,0,NI_NUMERICHOST) ; 
		if (ok != 0) { v_Msg(NULL,errbuf,"SystemCallFail2","getnameinfo()",errno) ; return(NULL) ; } ;
		break ;
	    } ;
	   break ;
	 } ;
	freeaddrinfo(addrRes);
	return(result) ;
}

/*	F I L E   E X T E N S I O N   T O   H T T P   C O N T E N T - T Y P E	*/

/*	v_FileExtToContentType - Converts file extension to HTTP Content-Type via v4_home:V4HTMLContentType.v4i */
/*	Call: ok = v_FileExtToContentType( srcext , dsttype , dstmax , errbuf)
	  where ok is TRUE if match found, FALSE if error
		srcext is pointer to ASCIZ file extension,
		dsttype is updated with content-type string (if error or no extension match then "application/octet-stream" is returned,
		dstmax is max bytes in dsttype,
		errbuf is updated with any errors			*/

LOGICAL v_FileExtToContentType(srcext,dsttype,dstmax,errbuf)
  UCCHAR *srcext, *dsttype ; UCCHAR *errbuf ;
  int dstmax ;
{ 
//  FILE *fp ;
  struct UC__File UCFile ;
  UCCHAR ext[32],tval[512], *b, *b1 ;
  int j,line ;


	if (dstmax < 32) return(FALSE) ;	/* Just not big enough */
	UCstrcpy(dsttype,UClit("application/octet-stream")) ;
	if (srcext == NULL) return(TRUE) ;
	if (srcext[0] == UClit('.')) srcext++ ;	/* If extension has "." then skip it */
	for(j=0;j<UCsizeof(ext);j++) { ext[j] = UCTOLOWER(srcext[j]) ; if (srcext[j] == UCEOS) break ; } ;
	ext[(UCsizeof(ext))-2] = UCEOS ; UCstrcat(ext,UClit(",")) ;
	if (!v_UCFileOpen(&UCFile,v_GetV4HomePath(UClit("V4HTMLContentType.v4i")),UCFile_Open_Read,FALSE,errbuf,0))
	 { UCstrcpy(dsttype,UClit("application/octet-stream")) ;
	   return(FALSE) ;
	 } ;
	for(line=1;;line++)
	 { 
	   if (v_UCReadLine(&UCFile,UCRead_UC,tval,UCsizeof(tval),errbuf) < 0) { v_UCFileClose(&UCFile) ; break ; } ;
	   if (!vuc_IsAlpha(tval[0])) continue ;	/* If not alpha then assume comment line */
	   b = UCstrchr(tval,'\t') ; if (b == NULL) continue ;
	   for(j=0,b1=b+1;*b1>' ' && j < 100;b1++,j++) { *b1 = UCTOLOWER(*b1) ; } ; *b1 = UCEOS ;
	   UCstrcat(b,UClit(",")) ;
	   if (UCstrstr(b,ext) != NULL)	/* We appended "," to both extension & list - this is funky but OK */
	    { *b = UCEOS ; UCstrcpy(dsttype,tval) ; break ; } ;
	 } ;
	v_UCFileClose(&UCFile) ;
	return(TRUE) ;
}

UCCHAR v4xml_LookupISONamedEntity(entity)
  UCCHAR *entity ;
{ UCCHAR *b, tbuf[UCsizeof(gpi->hent->entry[0].alphaVal)] ;
  INDEX i ;
  
	b = entity ; if (*b == UClit('&')) b++ ;
	for(i=0;i<(UCsizeof(tbuf))-1 && b[i]!=UClit(';');i++) { tbuf[i] = UCTOLOWER(b[i]) ; } ;
	tbuf[i] = UCEOS ;
	if (tbuf[0] > 127) return(0) ;
	i = gpi->hent->startIndex[tbuf[0]] ; if (i == UNUSED) return(0) ;
	for(;i<gpi->hent->count;i++)
	 { if (UCstrcmp(tbuf,gpi->hent->entry[i].alphaVal) == 0)
	    return(gpi->hent->entry[i].numVal) ;
	 } ;
	return(0) ;
 
}

/*	H T M L   E N T I T Y   T A B L E	*/

/*	v_HTMLEntityTableLoad - Converts file extension to HTTP Content-Type via v4_home:V4HTMLContentType.v4i */
/*	Call: ptr = v_HTMLEntityTableLoad()
	  where ptr is (struct V4HTML_Entities *) to entity table		*/

struct V4HTML_Entities *v_HTMLEntityTableLoad()
{ 
  struct UC__File UCFile ;
  struct V4HTML_Entities *hent ;
  INDEX i,j,gap ; COUNTER line ; LENMAX num,offset ; UCCHAR *b,*term,errbuf[256],tval[512] ;

	if (gpi->hent != NULL) return(gpi->hent) ;	/* Only do this once */
/*	Allocate for size+1 so that we have an extra slot for sorting at the end of this routine */
	hent = v4mm_AllocChunk(sizeof *hent,FALSE) ;
	hent->count = 0 ; for(i=0;i<128;i++) { hent->startIndex[i] = UNUSED ; } ;
	if (!v_UCFileOpen(&UCFile,v_GetV4HomePath(UClit("V4HTMLEntities.v4i")),UCFile_Open_Read,FALSE,errbuf,0))
	 { v_Msg(NULL,tval,"@* Err accessing V4 html entities initialization file: %1U\n",errbuf) ;
	   vout_UCText(VOUT_Warn,0,tval) ;
	   return(hent) ;
	 } ;
	UCFile.wantEOL = FALSE ;
	for(line=1;;line++)
	 { 
	   if (v_UCReadLine(&UCFile,UCRead_UC,tval,UCsizeof(tval),errbuf) < 0) break ;
	   if (!vuc_IsAlpha(tval[0])) continue ;	/* If not alpha then assume comment line */
	   b = UCstrchr(tval,'\t') ; if (b == NULL) goto badline ; *b = UCEOS ;
	   for(j=0;tval[j]!=UCEOS;j++) { tval[j] = UCTOLOWER(tval[j]) ; } ;
	   num = UCstrtol(b+1,&term,10) ; if (*term != UClit('\t') && *term != UCEOS) goto badline ;
	   if (hent->count >= V4HTML_EntityMaxSize)
	    { v_Msg(NULL,errbuf,"@*Exceeded max (%1d) number of HTML entities (%2U line %3d)\n",V4HTML_EntityMaxSize,v_GetV4HomePath(UClit("V4HTMLEntities.v4i")),line) ;
	      vout_UCText(VOUT_Warn,0,errbuf) ; break ;
	    } ;
	   hent->entry[hent->count].numVal = num ; UCstrcpy(hent->entry[hent->count].alphaVal,tval) ;
	   hent->count++ ;
	   continue ;
badline:
	   v_Msg(NULL,errbuf,"@*(%1U line %2d) - invalid syntax\n",v_GetV4HomePath(UClit("V4HTMLEntities.v4i")),line) ;
	   vout_UCText(VOUT_Warn,0,errbuf) ;
	 } ;
	v_UCFileClose(&UCFile) ;
/*	Sort by entity name */
	for(gap=hent->count/2;gap>0;gap=gap/2)
	 { for(i=gap;i<hent->count;i++)
	    { for(j=i-gap;j>=0;j-=gap)
	       { if (UCstrcmp(hent->entry[j].alphaVal,hent->entry[j+gap].alphaVal) <= 0) break ;
	         hent->entry[hent->count] = hent->entry[j] ; hent->entry[j] = hent->entry[j+gap] ; hent->entry[j+gap] = hent->entry[hent->count] ;
	       } ;
	    } ;
	 } ;
/*	Sort by character value */
	for(i=0;i<hent->count;i++)
	 { hent->charVal[i] = hent->entry[i].numVal ; hent->entryX[i] = i ; } ;

	for(gap=hent->count/2;gap>0;gap=gap/2)
	 { for(i=gap;i<hent->count;i++)
	    { for(j=i-gap;j>=0;j-=gap)
	       { UCCHAR uct ; INDEX tx ;
	         if (hent->charVal[j] <= hent->charVal[j+gap]) break ;
	         uct = hent->charVal[j] ; hent->charVal[j] = hent->charVal[j+gap] ; hent->charVal[j+gap] = uct ;
	         tx = hent->entryX[j] ; hent->entryX[j] = hent->entryX[j+gap] ; hent->entryX[j+gap] = tx ;
	       } ;
	    } ;
	 } ;
/*	Allocate entity table within mct, copy, and free up local structure */
	offset = v_mctAlloc(gpi,sizeof *hent,NULL) ;
	gpi->mct->HTMLEntityOffset = offset ; memcpy(&gpi->mct->Data[offset],hent,sizeof *hent) ;
	v4mm_FreeChunk(hent) ; hent = (struct V4HTML_Entities *)&gpi->mct->Data[offset] ;
	for(i=0;i<hent->count;i++)
	 { if (hent->entry[i].alphaVal[0] > 127)
	    { v_Msg(NULL,errbuf,"@*HTML entity (%1U) does not begin with ASCII value (0-127)\n",hent->entry[i].alphaVal) ; vout_UCText(VOUT_Warn,0,errbuf) ; continue ; } ;
	   if (hent->startIndex[hent->entry[i].alphaVal[0]] == UNUSED) hent->startIndex[hent->entry[i].alphaVal[0]] = i ;
	 } ;
	return(hent) ;
}



/*	C O L O R   H A N D L E R S		*/

/*	A color REF is defined as 32 bit int. If positive then it is assumed to be 24 bit RGB color.
	If negative then assumed to be 0x80000000+ref where ref is HTMLColors[x].Id		*/

struct v__ColorValues {
  int Count ;			/* Number of colors below */
  struct {
   int Id ;			/* Internal Reference Number */
   UCCHAR Name[24] ;		/* Name of color */
   int RGBValue ;		/* Red-Green-Blue value */
   } Entry[1] ;			/* Number depends... dynamically allocatd at color-load time */
} ;
#define V_ColorValue_InitSize 100
#define V_ColorValue_IncSize 100

void v_LoadColorTable()
{ 
//  FILE *fp ;
  struct UC__File UCFile ;
  char tval[128],wmsg[512], *b,*e ; UCCHAR name[64],errbuf[256],errbuf1[256] ;
  int line, id, rgbvalue, offset, max ;

	if (vcv != NULL) return ;		/* If already loaded then return */
//	fp = fopen(v_GetV4HomePath("v4colors.v4i"),"r") ;
//	if (fp == NULL)				/* Could not get file? */
//	 { v_Msg(NULL,tval,"V4InitFile",errno,v_GetV4HomePath(UClit("v4colors.v4i"))) ; vout_Text(VOUT_Warn,0,errbuf) ; vout_NL(VOUT_Warn) ;
	if (!v_UCFileOpen(&UCFile,v_GetV4HomePath(UClit("v4colors.v4i")),UCFile_Open_Read,FALSE,errbuf,0))
	 { v_Msg(NULL,errbuf1,"@* Err accessing V4 color initialization file: %1U\n",errbuf) ;
	   vout_UCText(VOUT_Warn,0,errbuf1) ;
	   vcv = (struct v__ColorValues *)v4mm_AllocChunk(sizeof *vcv + 5 * sizeof vcv->Entry[0],FALSE) ;
	   vcv->Count = 0 ;
	   vcv->Entry[vcv->Count].Id = 1 ; UCstrcpy(vcv->Entry[vcv->Count].Name,UClit("White")) ; vcv->Entry[vcv->Count++].RGBValue = 0xFFFFFF ; 
	   vcv->Entry[vcv->Count].Id = 2 ; UCstrcpy(vcv->Entry[vcv->Count].Name,UClit("Black")) ; vcv->Entry[vcv->Count++].RGBValue = 0x0 ; 
	   vcv->Entry[vcv->Count].Id = 3 ; UCstrcpy(vcv->Entry[vcv->Count].Name,UClit("Red")) ; vcv->Entry[vcv->Count++].RGBValue = 0xFF0000 ; 
	   vcv->Entry[vcv->Count].Id = 4 ; UCstrcpy(vcv->Entry[vcv->Count].Name,UClit("Green")) ; vcv->Entry[vcv->Count++].RGBValue = 0x00FF00 ; 
	   vcv->Entry[vcv->Count].Id = 5 ; UCstrcpy(vcv->Entry[vcv->Count].Name,UClit("Blue")) ; vcv->Entry[vcv->Count++].RGBValue = 0x0000FF ; 
	   return ;
	 } ;
/*	Here to load up file */
	vcv = (struct v__ColorValues *)v4mm_AllocChunk(sizeof *vcv + V_ColorValue_InitSize * sizeof vcv->Entry[0],FALSE) ;
	max = V_ColorValue_InitSize ; vcv->Count = 0 ;
	for(line=1;;line++)
	 { 
//	   if (fgets(tval,sizeof tval,fp) == NULL) break ;
	   if (v_UCReadLine(&UCFile,UCRead_UTF8,tval,sizeof tval,errbuf) < 0) break ;
	   if (!isdigit(tval[0])) continue ;	/* If not digit then assume comment line */
	   e = strchr(tval,'\t') ; if (e == NULL) goto badline ;
	   id = strtol(tval,&b,10) ; if (*b != '\t') goto badline ;
	   b = e + 1 ; e = strchr(b,'\t') ; if (e == NULL) goto badline ; *e = '\0' ;
	   if (strlen(b) >= UCsizeof(vcv->Entry[0].Name)) goto badline ; UCUTF8toUTF16(name,sizeof name,b,strlen(b)) ;
	   *e = '\t' ; e = strchr(b,'\t') ; if (e == NULL) goto badline ;
	   b = e + 1 ; rgbvalue = strtol(b,&b,16) ;
	   if (*b != '\t' && *b != ' ' && *b != '\0' && *b != '\r' && *b !='\n') goto badline ;
	   if (line >= max)			/* Have to increase size */
	    { max += V_ColorValue_IncSize ;
	      vcv = (struct v__ColorValues *)realloc(vcv,sizeof *vcv + max * sizeof vcv->Entry[0]) ;
	    } ;
	   vcv->Entry[vcv->Count].Id = id ; UCstrcpy(vcv->Entry[vcv->Count].Name,name) ; vcv->Entry[vcv->Count++].RGBValue = rgbvalue ; 
	   continue ;
badline:   sprintf(wmsg,"*Invalid line loading v4_home:v4colors.v4i(line %d): %s",line,tval) ;
	   vout_Text(VOUT_Warn,0,wmsg) ;
	   continue ;
	 } ;
//	fclose(fp) ;
	v_UCFileClose(&UCFile) ;
/*	Allocate color table within mct, copy, and free up local structure */
	offset = v_mctAlloc(gpi,sizeof *vcv + max * sizeof vcv->Entry[0],NULL) ;
	gpi->mct->ColorOffset = offset ; memcpy(&gpi->mct->Data[offset],vcv,sizeof *vcv + max * sizeof vcv->Entry[0]) ;
	v4mm_FreeChunk(vcv) ; vcv = (struct v__ColorValues *)&gpi->mct->Data[offset] ;
} ;


struct v__XLColorValues
 { int Id ;			/* Excel color code (with default color picker) */
   int RGBValue ;
 } ;

struct v__XLColorValues XLColors[] =
 { { 1, 0x000000 }, { 2, 0xFFFFFF }, { 3, 0xFF0000 }, { 4, 0x00FF00 }, { 5, 0x0000FF }, { 6, 0xFFFF00 },{ 7, 0xFF00FF },
   { 8, 0x00FFFF }, { 9, 0x800000 }, { 10, 0x008000 }, { 11, 0x000080 }, { 12, 0x808000 }, { 13, 0x800080 }, { 14, 0x008080 },
   { 15, 0xC0C0C0 }, { 16, 0x808080 }, { 17, 0x9999FF }, { 18, 0x993366 }, { 19, 0xFFFFCC }, { 20, 0xCCFFFF }, { 21, 0x660066 },
   { 22, 0xFF8080 }, { 23, 0x0066CC }, { 24, 0xCCCCFF }, { 25, 0x000080 }, { 26, 0xFF00FF }, { 27, 0xFFFF00 }, { 28, 0x00FFFF },
   { 29, 0x800080 }, { 30, 0x800000 }, { 31, 0x008080 }, { 32, 0x0000FF }, { 33, 0x00CCFF }, { 34, 0xCCFFFF }, { 35, 0xCCFFCC },
   { 36, 0xFFFF99 }, { 37, 0x99CCFF }, { 38, 0xFF99CC }, { 39, 0xCC99FF }, { 40, 0xFFCC99 }, { 41, 0x3366FF }, { 42, 0x33CCCC },
   { 43, 0x99CC00 }, { 44, 0xFFCC00 }, { 45, 0xFF9900 }, { 46, 0xFF6600 }, { 47, 0x666699 }, { 48, 0x969696 }, { 49, 0x003366 },
   { 50, 0x339966 }, { 51, 0x003300 }, { 52, 0x333300 }, { 53, 0x993300 }, { 54, 0x993366 }, { 55, 0x333399 }, { 56, 0x333333 },
   { -1, -1 }
  } ;

#define COLORRefMask 0x80000000
#define COLORRGBMask 0x40000000

int v_ColorNumToRef(num)
  int num ;
{
	return(COLORRGBMask | (num & 0xffffff)) ;
}
/*	v_ColorRefRange - Updates arguments with begin/end color REF range values */
int v_ColorRefRange(begin,end)
  int *begin,*end ;
{ 
//  int i,b,e ;

//	v_LoadColorTable() ;
	if (begin != NULL) *begin = COLORRefMask | 1 ;
	if (end != NULL) *end = COLORRefMask | vcv->Count ;
	return(0) ;

//	b = COLORRefMask | 1 ;			/* Lowest color */
//	for(i=0;HTMLColors[i].Id>0;i++) { e = HTMLColors[i].Id ; } ;
//	e |= COLORRefMask ;
//	if (begin != NULL) *begin = b ;
//	if (end != NULL) *end = e ;
//	return(0) ;
}

/*	v_ColorRGBNameToRef - Converts text RGB specification to internal color reference number (0 if none) */
int v_ColorRGBNameToRef(name,num)
  UCCHAR *name ;
  int num ;
{ int i,rgb ; UCCHAR *b ;
	
	if (name == NULL) { rgb = num ; }
	 else { rgb = UCstrtol(name,&b,16) ; if (*b != UCEOS) return(0) ; } ;
	if (rgb < 0 || rgb > 0xffffff) return(0) ;
//	v_LoadColorTable() ;
	for(i=0;i<vcv->Count;i++)
	 { if (rgb == vcv->Entry[i].RGBValue) return(COLORRefMask|vcv->Entry[i].Id) ; } ;
	return(COLORRGBMask|rgb) ;		/* No match - just return RGB */


//	for(i=0;HTMLColors[i].Id>0;i++)	/* Attempt to match with known color */
//	 { if (rgb == HTMLColors[i].RGBValue) return(COLORRefMask|HTMLColors[i].Id) ; } ;
//	return(COLORRGBMask|rgb) ;		/* No match - just return RGB */
}

/*	v_ColorNameToRef - Converts text name to internal color reference number (0 if none) */
int v_ColorNameToRef(name)
  UCCHAR *name ;
{ int i,j,rgb ; UCCHAR ucname[64],*b ;
	
//	v_LoadColorTable() ;

	if (name[0] == UClit('#'))		/* Color specified as hex #rrggbb value? */
	 { rgb = UCstrtol(&name[1],&b,16) ; if (*b != UCEOS) return(0) ;
	   if (rgb < 0 || rgb > 0xffffff) return(0) ;
	   for(i=0;i<vcv->Count;i++)	/* Attempt to match with known color */
	    { if (rgb == vcv->Entry[i].RGBValue) return(COLORRefMask|vcv->Entry[i].Id) ; } ;
	   return(COLORRGBMask|rgb) ;		/* No match - just return RGB */
	 } ;
	for(i=0,j=0;;i++)
	 { if (name[i] == UClit(' ')) continue ; ucname[j++] = UCTOUPPER(name[i]) ; if (name[i] == '\0') break ;
	 } ;
	if ((b = UCstrstr(ucname,UClit("GREY"))) != NULL) { UCstrncpy(b,UClit("GRAY"),4) ; } ;	/* Replace GREY with GRAY */
	for(i=0;vcv->Entry[i].Id>0;i++)
	 { 
//	   for(j=0;;j++) { ucname2[j] = toupper(vcv->Entry[i].Name[j]) ; if (vcv->Entry[i].Name[j] == '\0') break ; } ;
	   if (UCstrcmpIC(ucname,vcv->Entry[i].Name) == 0)
	    return(COLORRefMask|vcv->Entry[i].Id) ;
	 } ;
	return(0) ;
}

/*	v_ColorRefToName - Converts ref back to text name */
UCCHAR *v_ColorRefToName(ref)
  int ref ;
{ 
  int i,id ; static UCCHAR hexstr[12] ;

	if (ref == UNUSED) return(gpi->ci->li->LI[gpi->ci->li->CurX].None) ;
	if (ref < 0)
	 { id = ref & 0xffffff ;
	   for(i=0;i<vcv->Count;i++) { if (vcv->Entry[i].Id == id) return(vcv->Entry[i].Name) ; } ;
	   return(UClit("?unknowncolor")) ;
	 } ;
	UCsprintf(hexstr,UCsizeof(hexstr),UClit("#%06x"),(0xffffff & ref)) ; return(hexstr) ;
}

/*	v_ColorRefRGB - Converts ref back to 24 bit RGB value */
int v_ColorRefToRGB(ref)
  int ref ;
{ int i,id ;

//	v_LoadColorTable() ;
	if (ref >= 0) return(0xffffff & ref) ;
	id = ref & 0xffffff ;
	for(i=0;i<vcv->Count;i++)
	 { if (vcv->Entry[i].Id == id)
	  return(vcv->Entry[i].RGBValue) ; } ;
	return(0) ;
}

/*	v_ColorRefToHTML - Returns RGB name (as hex string) from ref, (NULL if none) */
UCCHAR *v_ColorRefToHTML(ref)
  int ref ;
{ int i,id ; static UCCHAR hexstr[12] ;

//	v_LoadColorTable() ;
	if (ref >= 0) { UCsprintf(hexstr,UCsizeof(hexstr),UClit("#%06x"),(0xffffff & ref)) ; return(hexstr) ; } ;
	id = ref & 0xffffff ;
	for(i=0;i<vcv->Count;i++)
	 { if (vcv->Entry[i].Id == id) { UCsprintf(hexstr,UCsizeof(hexstr),UClit("#%06x"),vcv->Entry[i].RGBValue) ; return(hexstr) ; } ;
	 } ;
	return(NULL) ;
}


/*	v_ColorRefToId - Attempts to match RGB value with "known" HTML color (or closest one) */

#define SQ(value) ((value)*(value))

int v_ColorRefToId(ref)
  int ref ;
{ int i,r1,r2,g1,g2,b1,b2,dist,mindist,mini,rgb ;

//	v_LoadColorTable() ;
	if (ref < 0) return(ref & 0xffffff) ;
	rgb = ref & 0xffffff ;
	r1 = rgb >> 16 ; g1 = (rgb >> 8) & 0xff ; b1 = rgb & 0xff ;
	for(i=0,mindist=V4LIM_BiggestPositiveInt;i<vcv->Count;i++)
	 { r2 = vcv->Entry[i].RGBValue >> 16 ; g2 = (vcv->Entry[i].RGBValue >> 8) & 0xff ; b2 = vcv->Entry[i].RGBValue & 0xff ;
	   dist = SQ(r1-r2) + SQ(g1-g2) + SQ(b1-b2) ;
	   if (dist < mindist) { mindist = dist ; mini = i ; } ;
	 } ;
	return(vcv->Entry[i].Id) ;
}

/*	v_ColorRefToXL - Attempts to match ref value with Excel default Color-Picker Index */
/*	Call: v_ColorRefToXL( ref , src )
	  where ref is color reference,
		src is TRUE if calling from V4-Excel add, FALSE if from routine making BIFF directly	*/

int v_ColorRefToXL(ref,src)
  int ref ;
  int src ;
{ int i,rgb,r1,r2,g1,g2,b1,b2,dist,mindist,mini=0,id ;

//	v_LoadColorTable() ;
	if (ref == 0) return(0) ;
	if (ref >= 0) { rgb = (0xffffff & ref) ; }
	 else { id = (ref & 0xffffff) ;
		for(i=0;i<vcv->Count;i++) { if (vcv->Entry[i].Id == id) break ; } ;
		if (i >= vcv->Count) return(UNUSED) ;
		rgb = vcv->Entry[i].RGBValue ;
	      } ;
	r1 = rgb >> 16 ; g1 = (rgb >> 8) & 0xff ; b1 = rgb & 0xff ;
	for(i=0,mindist=V4LIM_BiggestPositiveInt;XLColors[i].Id>0;i++)
	 { r2 = XLColors[i].RGBValue >> 16 ; g2 = (XLColors[i].RGBValue >> 8) & 0xff ; b2 = XLColors[i].RGBValue & 0xff ;
	   dist = SQ(r1-r2) + SQ(g1-g2) + SQ(b1-b2) ;
	   if (dist < mindist) { mindist = dist ; mini = i ; } ;
	 } ;
	return(src ? XLColors[mini].Id : XLColors[mini].Id + 7) ;
}

/*	B a s e 6 4   R o u t i n e s				*/

/*	VUC_Encode64 - Encodes a string to BASE64			*/
/*	Call: len = VUC_Encode64( in , inlen , out , outmax , errmsg )
	  where len is length of string (also is NULL terminated), -1 if error ,
		in / inlen descscribe input string,
		out / outmax describe output string,
		errmsg if not NULL is updated with any error	*/

int VUC_Encode64(in,inlen,out,outmax,errmsg)
  UCCHAR *in, *out, *errmsg ;
  int inlen, outmax ;
{ UCCHAR oval;
  int olen;
  static UCCHAR basis_64[] =
	UClit("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????");

	olen = (inlen + 2) / 3 * 4;
	if (outmax < olen + 1) { if (errmsg != NULL) UCsprintf(errmsg,100,UClit("Required bytes (%d) less than buffer size (%d)"),olen+1,outmax) ; return(-1) ; } ;

	for(;inlen >= 3;)
	 { 
	   if (in[0] > 255 || in[1] > 255 || in[2] > 255) { v_Msg(NULL,errmsg,"@non-ASCII character in Encode64() routine: %1U",in) ; return(-1) ; } ;
	   *out++ = basis_64[in[0] >> 2] ;
	   *out++ = basis_64[((in[0] << 4) & 0x30) | (in[1] >> 4)] ;
	   *out++ = basis_64[((in[1] << 2) & 0x3c) | (in[2] >> 6)] ;
	   *out++ = basis_64[in[2] & 0x3f] ;
	   in += 3 ; inlen -= 3 ;
	 } ;
	if (inlen > 0)			/* Have to pad ? */
	 {
	   *out++ = basis_64[in[0] >> 2] ;
	   oval = (in[0] << 4) & 0x30 ;
	   if (inlen > 1) oval |= in[1] >> 4 ;
	   *out++ = basis_64[oval] ;
	   *out++ = (inlen < 2) ? UClit('=') : basis_64[(in[1] << 2) & 0x3c] ;
	   *out++ = UClit('=') ;
	 } ;
	*out = UCEOS ;			/* Terminate with null */

	return(olen) ;
}

/*	V_Encode64 - Same as above but for char, not UCCHAR */
int V_Encode64(in,inlen,out,outmax,errmsg)
  unsigned char *in, *out ;
  int inlen, outmax ;
  UCCHAR *errmsg ;
{ UCCHAR oval;
  int olen;
  static UCCHAR basis_64[] =
	UClit("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????");

	olen = (inlen + 2) / 3 * 4;
	if (outmax < olen + 1) { if (errmsg != NULL) UCsprintf(errmsg,100,UClit("Required bytes (%d) less than buffer size (%d)"),olen+1,outmax) ; return(-1) ; } ;

	for(;inlen >= 3;)
	 { 
	   *out++ = basis_64[in[0] >> 2] ;
	   *out++ = basis_64[((in[0] << 4) & 0x30) | (in[1] >> 4)] ;
	   *out++ = basis_64[((in[1] << 2) & 0x3c) | (in[2] >> 6)] ;
	   *out++ = basis_64[in[2] & 0x3f] ;
	   in += 3 ; inlen -= 3 ;
	 } ;
	if (inlen > 0)			/* Have to pad ? */
	 {
	   *out++ = basis_64[in[0] >> 2] ;
	   oval = (in[0] << 4) & 0x30 ;
	   if (inlen > 1) oval |= in[1] >> 4 ;
	   *out++ = basis_64[oval] ;
	   *out++ = (inlen < 2) ? UClit('=') : basis_64[(in[1] << 2) & 0x3c] ;
	   *out++ = UClit('=') ;
	 } ;
	*out = UCEOS ;			/* Terminate with null */

	return(olen) ;
}

/*	V_Decode64 - Decodes BASE64 string			*/

/*	Call: len = V_Decode64( in , inlen , out , outmax , errmsg )
	  where len is length of string (also is NULL terminated), -1 ,
		in / inlen descscribe input BASE64 string,
		out / outmax describe output string,
		errmsg if not NULL is updated with any error	*/

int V_Decode64(in,inlen,out,outmax,errmsg)
  UCCHAR *in, *out ; UCCHAR *errmsg ;
  int inlen, outmax ;
{

  static char index_64[128] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
	-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1 } ;
  unsigned len = 0,lup ;
  int c1, c2, c3, c4 ;
#define CHAR64(c) (((c) < 0 || (c) > 127) ? -1 : index_64[(c)])

	if (in[0] == UClit('+') && in[1] == UClit(' ')) in += 2;

	if (*in == '\0') { out[0] = UCEOS ; return(0) ; } ;
//	 { if (errmsg != NULL) UCstrcpy(errmsg,UClit("Input string empty?")) ; return(-1) ; } ;

	for (lup = 0; lup < inlen / 4; lup++)
	 {
	   c1 = in[0] ; if (CHAR64(c1) == -1) goto bad_char ;
	   c2 = in[1] ; if (CHAR64(c2) == -1) goto bad_char ;
	   c3 = in[2] ; if (c3 != UClit('=') && CHAR64(c3) == -1) goto bad_char ;
	   c4 = in[3] ; if (c4 != UClit('=') && CHAR64(c4) == -1) goto bad_char ;
	  in += 4 ;
	  *out++ = (CHAR64(c1) << 2) | (CHAR64(c2) >> 4) ; ++len ;
	  if (c3 != UClit('='))
	   {
	     *out++ = ((CHAR64(c2) << 4) & 0xf0) | (CHAR64(c3) >> 2) ; ++len ;
	     if (c4 != UClit('=')) { *out++ = ((CHAR64(c3) << 6) & 0xc0) | CHAR64(c4) ; ++len ; } ;
	   } ;
	 } ;

	*out = 0 ;
	return(len) ;

bad_char:
	if (errmsg != NULL) UCstrcpy(errmsg,UClit("Invalid BASE64 character in string")) ;
	return(-1) ;
}


/*	v_Hash32 - Returns 32 bit hash value of its string argument		*/
/*	Call: hash = v_Hash32( str , len , direction )
	  where hash is 32 bit hash generated,
		str is string to hash,
		len is length of string in bytes,
		direction is direction to scan, 1 = forward, 2 = backwards	*/
HASH32 vhash32_table[256] =
	{ 0,8620,49519,30058,34915,14349,3083,44490,44518,61256,25133,34040,54458,2265,3503,34714,
	  43984,504,25127,4380,27360,45008,38599,60977,55454,34532,6027,42855,27262,45953,59658,49951,
	  17200,3110,48239,21511,41460,49572,64948,23942,16189,64392,47360,49371,42697,4763,41394,57980,
	  17872,28600,50233,31308,15582,18016,23544,10912,31884,58828,59585,3969,59287,33064,33835,20908,
	  64660,32373,17442,5946,62112,4833,32814,25175,18158,59887,34717,30437,61668,3282,49906,50476,
	  54251,8215,1039,45118,56901,41257,48249,47540,65500,58233,15282,20075,23004,33637,38739,55442,
	  27006,55149,17649,27223,35212,30665,18822,11686,10074,37463,52586,2166,35025,32668,62610,49040,
	  36345,58375,40950,55183,10470,13942,46839,8547,5963,17995,196,27151,1761,46518,61466,15722,
	  11855,20810,58129,42733,9852,44652,25284,25409,32751,9668,38481,55415,38673,62613,36447,9709,
	  64441,26788,9294,37021,16523,32015,30410,62986,8259,13091,20922,41239,8304,42680,40739,52630,
	  16242,31223,25514,13320,1859,59092,27950,9307,62094,26890,8597,58041,6040,10629,4657,23942,
	  16584,8854,51324,29839,22906,29641,53014,61058,42706,14106,44537,59567,16392,56417,30884,33158,
	  39347,53579,49534,30293,62348,41467,28791,54047,45153,46019,64693,62548,55788,18960,35220,33713,
	  6778,27133,37795,57446,28838,47824,56968,46900,52476,46303,48609,1251,58066,34405,30364,4272,
	  46754,32043,43757,44698,13078,60072,56746,58328,35648,9122,29513,64838,14125,29230,20691,33728,
	  57770,28817,30640,52864,23930,13862,65478,10066,41319,40393,38,57,50682,47666,20917,27375 } ;

HASH32 v_Hash32(str,len,direction)
 char *str ;
 int len,direction ;
{ HASH32 hash ; INDEX i ;

	hash = len | str[0] ;
	switch (direction)
	 {
	   case 1:
		for(i=0;i<len;i++)
		 { hash = (hash < 0 ? ((hash << 1) | 1) : (hash << 1)) ; hash ^= vhash32_table[str[i]] ; } ;
		break ;
	   case 2:
		for(i=len-1;i>=0;i--)
		 { hash = (hash < 0 ? ((hash << 1) | 1) : (hash << 1)) ; hash ^= vhash32_table[str[i]] ; } ;
		break ;
	 } ;
	return(hash) ;
}

/*	v_Hash64 - Returns 64 bit hash value of its string argument		*/
/*
 * Fowler/Noll/Vo hash
 *
 * The basis of this hash algorithm was taken from an idea sent
 * as reviewer comments to the IEEE POSIX P1003.2 committee by:
 *
 *      Phong Vo (http://www.research.att.com/info/kpv/)
 *      Glenn Fowler (http://www.research.att.com/~gsf/)
 *
 * In a subsequent ballot round:
 *
 *      Landon Curt Noll (http://www.isthe.com/chongo/)
 *
 * improved on their algorithm.  Some people tried this hash
 * and found that it worked rather well.  In an EMail message
 * to Landon, they named it the ``Fowler/Noll/Vo'' or FNV hash.
 *
 * FNV hashes are designed to be fast while maintaining a low
 * collision rate. The FNV speed allows one to quickly hash lots
 * of data while maintaining a reasonable collision rate.  See:
 *
 *      http://www.isthe.com/chongo/tech/comp/fnv/index.html
 *
 * for more details as well as other forms of the FNV hash.
 *
 ***
 *
 * Please do not copyright this code.  This code is in the public domain.
*/

#define FNV1_64_INIT (UB64INT)0xcbf29ce484222325
#define FNV_64_PRIME (UB64INT)0x100000001b3

UB64INT v_Hash64(str)			/* Hashes UTF-16 Unicode string */
  UCCHAR *str ;
{ UB64INT hval ; char byte ;

	hval = FNV1_64_INIT ;
	for (;*str!=UCEOS;str++)
	 { byte = *str & 0xff ;
	   if (byte != 0)
	    { hval *= FNV_64_PRIME ;	/* multiply by the 64 bit FNV magic prime mod 2^64 */
	      hval ^= (UB64INT)byte ;	/* xor the bottom with the current octet */
	    } ;
	   byte = *str >> 8 ;
	   if (byte != 0)
	    { hval *= FNV_64_PRIME ;	/* multiply by the 64 bit FNV magic prime mod 2^64 */
	      hval ^= (UB64INT)byte ;	/* xor the bottom with the current octet */
	    } ;
	 } ;
	return(hval) ;
}


UB64INT v_Hash64b(bstr,len)		/* Hashes 8-bit string (binary or ASCII) */
  unsigned char *bstr ;
  int len ;
{ UB64INT hval ; int i ;

	hval = FNV1_64_INIT ;
	for (i=0;i<len;bstr++,i++)
	 { 
//	   if (*bstr != 0)
	   if (TRUE)
	    { hval *= FNV_64_PRIME ;	/* multiply by the 64 bit FNV magic prime mod 2^64 */
	      hval ^= (UB64INT)*bstr ;	/* xor the bottom with the current octet */
	    } ;
	 } ;
	return(hval) ;
}


/*	v_StringLit - Converts string to valid V3/V4 string literal format		*/
/*	Call: newlen = v_StringLit( src , dst , dstlen , startchar , escchar)
	  where newlen is new length of string (in dst) (UNUSED if problems),
		src is source string,
		dst is destination buffer,
		dstlen is max length of buffer (if negative then use abs(dstlen) and end string with dot-dot-dot,
		startchar is starting string literal character (e.g. "'" or "\""),
		escchar is the escape character ('\0' for system default,UNUSED for none, -2 to use HTML entity for embedded ending characters) */

int v_StringLit(src,dst,dstlen,startchar,escchar)
  UCCHAR *dst,*src ; UCCHAR startchar,escchar ; LENMAX dstlen ;
{ int s,d,dddot ; UCCHAR endchar ;

#define DDD UClit("...")

	d=0 ; dst[d++] = startchar ;
	if (dstlen < 0) { dstlen = -dstlen ; dddot = TRUE ; } else { dddot = FALSE ; } ;
	if (escchar == UCEOS) escchar = UClit('\\') ;
	switch (startchar)
	 { default:		endchar = startchar ; break ;
	   case UClit('<'):	endchar = UClit('>') ; break ;
	   case UClit('['):	endchar = UClit(']') ; break ;
	   case UClit('('):	endchar = UClit(')') ; break ;
	   case UClit('{'):	endchar = UClit('}') ; break ;
	 } ;
	if (escchar == UNUSED)
	 { for(s=0;d<dstlen-1;s++)
	    { switch(src[s])
	       { case UCEOS:	dst[d++] = endchar ; dst[d++] = UCEOS ; return(d-1) ;
	         default:	if (src[s] == endchar) { dst[d++] = endchar ; dst[d++] = src[s] ; break ; } ;
				dst[d++] = src[s] ; break ;
	       } ;
	    } ;
	 } else if (escchar == -2)
	 { int i ; UCCHAR entity[12] ; UCsprintf(entity,UCsizeof(entity),UClit("&#%.3d;"),endchar) ;
	   for(s=0;d<dstlen-1;s++)
	    { switch(src[s])
	       { case UCEOS:	dst[d++] = endchar ; dst[d++] = UCEOS ; return(d-1) ;
	         default:	if (src[s] == endchar) { for(i=0;i<6;i++) dst[d++] = entity[i] ; break ; } ;
				dst[d++] = src[s] ; break ;
	       } ;
	    } ;
	 } else if (escchar != UClit('\\'))
	 { for(s=0;d<dstlen-1;s++)
	    { switch(src[s])
	       { case UCEOS:	dst[d++] = endchar ; dst[d++] = UCEOS ; return(d-1) ;
	         default:	if (src[s] == endchar || src[s] == escchar) { dst[d++] = escchar ; dst[d++] = src[s] ; break ; } ;
				dst[d++] = src[s] ; break ;
	       } ;
	    } ;
	 } ;
/*	Here if escape character is backslash */
	for(s=0;d<dstlen-1;s++)
	 { switch (src[s])
	    { case UCEOS:	dst[d++] = endchar ; dst[d++] = UCEOS ; return(d-1) ;
	      case UClit('\r'):	dst[d++] = UClit('\\') ; dst[d++] = UClit('r') ; break ;
	      case UCNL:	dst[d++] = UClit('\\') ; dst[d++] = UClit('n') ; break ;
	      case UClit('\t'):	dst[d++] = UClit('\\') ; dst[d++] = UClit('t') ; break ;
	      case UClit('\\'): dst[d++] = UClit('\\') ;
	      default:		if (src[s] == endchar) { dst[d++] = UClit('\\') ; dst[d++] = src[s] ; break ; } ;
				if (vuc_IsPrint(src[s]) || src[s] > 127 || src[s] == UClit(' ')) { dst[d++] = src[s] ; continue ; } ;
				dst[d++] = UClit('\\') ; dst[d++] = (src[s] >> 6) + '0' ;
				dst[d++] = ((src[s] >> 3) & 7) + '0' ; dst[d++] = (src[s] & 7) + '0' ; break ;
	    } ;
	 } ;
/*	If here then string exceeds max - maybe terminate with DDD */
	if (!dddot) return(UNUSED) ;		/* No DDD - return UNUSED as flag that string too long */
	d = dstlen - (UCstrlen(DDD) + 2) ;
	for(s=0;DDD[s]!=UCEOS;s++) { dst[d++] = DDD[s] ; } ; 
	dst[d++] = endchar ; dst[d++] = UCEOS ; return(d-1) ;
}

/********************************************************************************************************/
/*	C A L E N D A R   R O U T I N E S			*/
/********************************************************************************************************/


/*	Day-of-Week Functions (some implemented as macros) */
int DayOfWeekFromFixed(date) { return(IMOD(date,7)) ; }
int KDayOnOrBefore(date,k) { return(date - DayOfWeekFromFixed(date - k)) ; }
int KDayOnOrAfter(date,k) { return(KDayOnOrBefore(date+6,k)) ; }
int KDayNearest(date,k) { return(KDayOnOrBefore(date+3,k)) ; }
int KDayBefore(date,k) { return(KDayOnOrBefore(date-1,k)) ; }
int KDayAfter(date,k) { return(KDayOnOrBefore(date+7,k)) ; }

int NthKDay(n,k,gyear,gmonth,gday)
  int n,k,gyear,gmonth,gday ;
{ int res ;

	if (n > 0)
	 { res = 7 * n + KDayBefore(FixedFromGregorian(gyear,gmonth,gday),k) ; }
	 else { res = 7 * n + KDayAfter(FixedFromGregorian(gyear,gmonth,gday),k) ; } ;
	return(res) ;
}

double LunarLongitude() ;
double SolarLongitude() ;


/*	G R E G O R I A N   C A L E N D A R		*/

/*	GregorianLeapYear - Returns TRUE if year is leap year */
int GregorianLeapYear(year)
  int year ;
{ int mod400 ;

	mod400 = IMOD(year,400) ;
	return(IMOD(year,4) == 0 && !(mod400 == 100 || mod400 == 200 || mod400 == 300)) ;
}

LOGICAL VerifyGregorianYMD(year,month,day,err)
  int year,month,day ;
  UCCHAR *err ;
{
	switch (month)
	 { default:
		v_Msg(NULL,err,"CalMonth",month) ; return(FALSE) ;
	   case 1: case 3: case 5: case 7: case 8: case 10: case 12:
		if (day < 1 || day > 31) { v_Msg(NULL,err,"CalDayBad",day,31,month) ; return(FALSE) ; } ;
		break ;
	   case 4: case 6: case 9: case 11:
		if (day < 1 || day > 30) { v_Msg(NULL,err,"CalDayBad",day,30,month) ; return(FALSE) ; } ;
		break ;
	   case 2:
		{ int lastday = (GregorianLeapYear(year) ? 29 : 28) ;
		  if (day < 1 || day > lastday) { v_Msg(NULL,err,"CalDayBadFeb",day,lastday,year,month) ; return(FALSE) ; } ;
		} ;
		break ;
	 } ;
	if (year < -10000 || year > 10000) { v_Msg(NULL,err,"@Invalid year(%1d)",year) ; return(FALSE) ; } ;
	return(TRUE) ;
}
/*	FixedFromGregorian - year,month,day => date */
int FixedFromGregorian(year,month,day)
  int year,month,day ;
{ int date ;

	date = GregorianEpoch - 1 + 365 * (year - 1) + IDIVFLOOR((year - 1),4) -
	     IDIVFLOOR((year - 1),100) + IDIVFLOOR((year - 1),400) + IDIVFLOOR((367 * month - 362),12) +
	     (month <= 2 ? 0 : (GregorianLeapYear(year) ? -1 : -2)) + day ;
	return(date) ;
}

int GregorianYearFromFixed(date)
  int date ;
{ int d0,n400,d1,n100,d2,n4,d3,n1,year ;

	d0 = date - GregorianEpoch ;
	n400 = IDIVFLOOR(d0,146097) ;
	d1 = IMOD(d0,146097) ;
	n100 = IDIVFLOOR(d1,36524) ;
	d2 = IMOD(d1,36524) ;
	n4 = IDIVFLOOR(d2,1461) ;
	d3 = IMOD(d2,1461) ;
	n1 = IDIVFLOOR(d3,365) ;
	year = 400 * n400 + 100 * n100 + 4 * n4 + n1 ;
	return(n100 == 4 || n1 == 4 ? year : year + 1) ;
}

LOGICAL GregorianFromFixed(date,cal)
  int date ;
  struct V4Cal__Args *cal ;
{ int tdate,priordays,correction ;

	cal->Year = GregorianYearFromFixed(date) ;
	priordays = date - FixedFromGregorian(cal->Year,1,1) ;
	tdate = FixedFromGregorian(cal->Year,3,1) ;
	correction = (date < tdate ? 0 : (GregorianLeapYear(cal->Year) ? 1 : 2)) ;
	cal->Month = IDIVFLOOR((12 * (priordays + correction) + 373),367) ;
	cal->Day = date - FixedFromGregorian(cal->Year,cal->Month,1) + 1 ;
	return(TRUE) ;
}


int JulianLeapYear(jyear)
  int jyear ;
{
	return(IMOD(jyear,4) == (jyear > 0 ? 0 : 3)) ;
}

#define JulianEpoch -1 /* FixedFromGregorian(0,12,30) */

int FixedFromJulian(year,month,day)
  int year,month,day ;
{
  int date,y ;

	y = (year < 0 ? year + 1 : year) ;
	date = JulianEpoch - 1 + 365 * (y - 1) + IDIVFLOOR((y - 1),4) +
		IDIVFLOOR((367 * month - 362),12) +
		(month <= 2 ? 0 : (JulianLeapYear(year) ? -1 : -2)) +
		day ;
	return(date) ;
}

LOGICAL JulianFromFixed(date,cal)
  int date ;
  struct V4Cal__Args *cal ;
{ int approx,priordays,tdate,correction ;

	approx = IDIVFLOOR((4 * (date - JulianEpoch) + 1464),1461) ;
	cal->Year = (approx <= 0 ? approx - 1 : approx) ;
	priordays = date - FixedFromJulian(cal->Year,1,1) ;
	tdate = FixedFromJulian(cal->Year,3,1) ;
	  correction = (date < tdate ? 0 : (JulianLeapYear(cal->Year) ? 1 : 2)) ;
	cal->Month = IDIVFLOOR((12 * (priordays + correction) + 373),367) ;
	cal->Day = date - FixedFromJulian(cal->Year,cal->Month,1) + 1 ;
	return(TRUE) ;
}


/*	I S O   C A L E N D A R		*/

LOGICAL VerifyISOYWD(year,week,day,err)
  int year,week,day ;
  UCCHAR *err ;
{

	if (day < 1 || day > 7) { v_Msg(NULL,err,"@Invalid day(%1d)",day) ; return(FALSE) ; } ;
	if (week < 1) { v_Msg(NULL,err,"@Invalid week(%1d)",week) ; return(FALSE) ; } ;
	if (week > 52)
	 { if (week > 53 || (FixedFromISO(year+1,1,1) - FixedFromISO(year,1,1)) < 365)
	    { v_Msg(NULL,err,"@Invalid week(%1d)",week) ; return(FALSE) ; } ;
	 } ;
	if (year < -10000 || year > 10000) { v_Msg(NULL,err,"@Invalid year(%1d)",year) ; return(FALSE) ; } ;
	return(TRUE) ;
}

int FixedFromISO(year,week,day)
  int year,week,day ;
{ int date ;

	date = NthKDay(week,0,year-1,12,28) + day ;
	return(date) ;
}

int AMOD(x,y) int x,y ; { int t ; t = IMOD(x,y) ; return(t == 0 ? y : t) ; }

LOGICAL ISOFromFixed(date,cal)
  int date ;
  struct V4Cal__Args *cal ;
{ int approx ;

	approx = GregorianYearFromFixed(date-3) ;
	cal->Year = (date >= FixedFromISO(approx+1,1,1) ? approx + 1 : approx) ;
	cal->Week = IDIVFLOOR((date - FixedFromISO(cal->Year,1,1)),7) + 1 ;
	cal->Day = AMOD(date,7) ;
	return(TRUE) ;
}

/*	I S L A M I C   C A L E N D A R		*/

#define IslamicEpoch 227015	/* FixedFromJulian(-622,7,16) */

int IslamicLeapYear(iyear)
  int iyear ;
{
	return(IMOD((14 + 11*iyear),30) < 11) ;
}

LOGICAL VerifyIslamicYMD(year,month,day,err)
  int year,month,day ;
  UCCHAR *err ;
{
	switch (month)
	 { default:
		v_Msg(NULL,err,"@Invalid month(%1d)",month) ; return(FALSE) ;
	   case 1: case 3: case 5: case 7: case 9: case 11:
		if (day < 1 || day > 30) { v_Msg(NULL,err,"@Invalid day(%1d) for month(%2d)",day,month) ; return(FALSE) ; } ;
		break ;
	   case 2: case 4: case 6: case 8: case 10:
		if (day < 1 || day > 29) { v_Msg(NULL,err,"@Invalid day(%1d) for month(%2d)",day,month) ; return(FALSE) ; } ;
		break ;
	   case 12:
		if (day < 1 || day > 30) { v_Msg(NULL,err,"@Invalid day(%1d) for month(%2d)",day,month) ; return(FALSE) ; } ;
		if (day == 30 && !IslamicLeapYear(year)) { v_Msg(NULL,err,"@Invalid day(%1d) for month(%2d)",day,month) ; return(FALSE) ; } ;
		break ;
	 } ;
	if (year < -10000 || year > 10000) { v_Msg(NULL,err,"@Invalid year(%1d)",year) ; return(FALSE) ; } ;
	return(TRUE) ;
}
int FixedFromIslamic(year,month,day)
  int year,month,day ;
{ int date ;

	date = day + 29 * (month - 1) + IDIVFLOOR((6 * month - 1),11) + (year - 1) * 354 +
		IDIVFLOOR((3 + 11 * year),30) + IslamicEpoch - 1 ;
	return(date) ;
}

LOGICAL IslamicFromFixed(date,cal)
  int date ;
  struct V4Cal__Args *cal ;
{ int priordays ;

	cal->Year = IDIVFLOOR((30 * (date - IslamicEpoch) + 10646),10631) ;
	priordays = date - FixedFromIslamic(cal->Year,1,1) ;
	cal->Month = IDIVFLOOR((11 * priordays + 330),325) ;
	cal->Day = date - FixedFromIslamic(cal->Year,cal->Month,1) + 1 ;
	return(TRUE) ;
}

/*	H E B R E W   C A L E N D A R			*/

#define HebrewEpoch -1373427	/* FixedFromJulian(-3761,10,7) */
#define HR(hour) ((double)hour / 24.0)
#define NISAN 1
#define TISHRI 7

int HebrewLeapYear(hyear)
  int hyear ;
{
	return(IMOD((7 * hyear + 1),19) < 7) ;
}

int LastMonthOfHebrewYear(hyear)
{
	return(HebrewLeapYear(hyear) ? 13 : 12) ;
}

double Molad(hmonth,hyear)
  int hmonth, hyear ;
{ double res,monthselapsed ;

	monthselapsed = hmonth  - TISHRI + IDIVFLOOR((235 * hyear - 234),19) ;
	res = (double)HebrewEpoch - 876.0/25920.0 + monthselapsed * (29.0 + HR(12) + 793.0/25920.0) ;
	return(res) ;
}

int HebrewCalendarElapsedDays(hyear)
  int hyear ;
{ int monthselapsed,day ; double partselapsed ;
//double day1 ;
	monthselapsed = IDIVFLOOR((235 * hyear - 234),19) ;
	partselapsed = 12084 + 13753.0 * monthselapsed ;
	day = 29 * monthselapsed + IDIVFLOOR(partselapsed,25920) ;

//day1 = Molad(TISHRI,hyear) - HebrewEpoch + 0.5 ;

	return(IMOD((3 * (day + 1)),7) < 3 ? day + 1 : day) ;
}

int HebrewNewYearDelay(hyear)
  int hyear ;
{ int ny0,ny1,ny2 ;

	ny0 = HebrewCalendarElapsedDays(hyear - 1) ;
	ny1 = HebrewCalendarElapsedDays(hyear) ;
	ny2 = HebrewCalendarElapsedDays(hyear + 1) ;
	return(ny2 - ny1 == 356 ? 2 : (ny1 - ny0 == 382 ? 1 : 0)) ;
}

int HebrewNewYear(hyear)
  int hyear ;
{
	return(HebrewEpoch + HebrewCalendarElapsedDays(hyear) + HebrewNewYearDelay(hyear)) ;
}

int LastDayOfHebrewMonth(hmonth,hyear)
  int hmonth,hyear ;
{
	if ((hmonth == 2 ||  hmonth == 4 || hmonth == 6 || hmonth == 10 || hmonth == 13)
		|| (hmonth == 12 && !HebrewLeapYear(hyear))
		|| (hmonth == 8 && !LongMarheshvan(hyear))
		|| (hmonth == 9 && ShortKislev(hyear))) return(29) ;
	return(30) ;
}

int LongMarheshvan(hyear)
  int hyear ;
{ int d ;

	d = DaysInHebrewYear(hyear) ;
	return(d == 355 || d == 385) ;
}

int ShortKislev(hyear)
  int hyear ;
{int d ;
	d = DaysInHebrewYear(hyear) ;
	return(d == 353 || d == 383) ;
}

int DaysInHebrewYear(hyear)
  int hyear ;
{
	return(HebrewNewYear(hyear+1) - HebrewNewYear(hyear)) ;
}

LOGICAL VerifyHebrewYMD(year,month,day,err)
  int year,month,day ;
  UCCHAR *err ;
{
	if (month < 1 || month > 13) { v_Msg(NULL,err,"@Invalid month(%1d)",month) ; return(FALSE) ; } ;
	switch (month)
	 { case 1: case 3: case 5: case 7: case 11:
		if (day < 1 || day > 30) { v_Msg(NULL,err,"@Invalid day(%1d) for month(%2d)",day,month) ; return(FALSE) ; } ;
		break ;
	   case 2: case 4: case 6: case 10:
		if (day < 1 || day > 29) { v_Msg(NULL,err,"@Invalid day(%1d) for month(%2d)",day,month) ; return(FALSE) ; } ;
		break ;
	   case 8: case 9:
		if (day < 1 || (day > 29 ? day > LastDayOfHebrewMonth(month,year) : FALSE))
		 { v_Msg(NULL,err,"@Invalid day(%1d) for month(%2d)",day,month) ; return(FALSE) ; } ;
		break ;
	   case 12:
		if (day < 1 || (day > 29 ? !HebrewLeapYear(year) : FALSE)) { v_Msg(NULL,err,"@Invalid day(%1d) for month(%2d)",day,month) ; return(FALSE) ; } ;
		break ;
	   case 13:
		if (!HebrewLeapYear(year)) { v_Msg(NULL,err,"@Month(%1d) only allowed in leap year",month) ; return(FALSE) ; } ;
		if (day < 1 || day > 29) { v_Msg(NULL,err,"@Invalid day(%1d) for month(%2d)",day,month) ; return(FALSE) ; } ;
		break ;
	 }
	return(TRUE) ;
}

int FixedFromHebrew(year,month,day)
  int year,month,day ;
{ int mend,sum,m ;

	sum = 0 ;
	if (month < TISHRI)
	 { mend = LastMonthOfHebrewYear(year) ;
	   for(m=TISHRI;m<=mend;m++) { sum += LastDayOfHebrewMonth(m,year) ;} ;
	   for(m=NISAN;m<month;m++) { sum += LastDayOfHebrewMonth(m,year) ; } ;
	 } else
	 { for(m=TISHRI;m<month;m++) { sum += LastDayOfHebrewMonth(m,year) ; } ;
	 } ;
	return(HebrewNewYear(year) + day - 1 + sum) ;
}

LOGICAL HebrewFromFixed(date,cal)
  int date ;
  struct V4Cal__Args *cal ;
{ int approx,y,start,m ;

//for(y=5760;y<=5770;y++) printf("%d = %d\n",y,DaysInHebrewYear(y)) ;
//printf("%d\n",HebrewCalendarElapsedDays(4682)) ;
//printf("%d\n",HebrewCalendarElapsedDays(4683)) ;
//printf("%d\n",HebrewCalendarElapsedDays(4684)) ;
	approx = IDIVFLOOR(((double)(date - HebrewEpoch)),(35975351.0/98496.0)) + 1 ;
	for(y=approx-1;;y++) { if (HebrewNewYear(y) > date) break ; } ;
	cal->Year = y - 1 ;
//printf("%d = %d\n",cal->Year,HebrewLeapYear(cal->Year)) ;
	start = (date < FixedFromHebrew(cal->Year,NISAN,1) ? TISHRI : NISAN) ;
	for(m=start;;m++) { if (date <= FixedFromHebrew(cal->Year,m,LastDayOfHebrewMonth(m,cal->Year))) break ; } ;
	cal->Month = m ;
	cal->Day = date - FixedFromHebrew(cal->Year,cal->Month,1) + 1 ;
	return(TRUE) ;
}

int Easter(gyear)
  int gyear ;
{ int century,shiftedepact,adjustedepact,paschalmoon ;

	century = IDIVFLOOR(gyear,100) + 1 ;
	shiftedepact = IMOD((14 + 11 * IMOD(gyear,19) - IDIVFLOOR((3 * century),4) + IDIVFLOOR((5 + 8 * century),25)),30) ;
	adjustedepact = ((shiftedepact == 0 || (shiftedepact == 1 && IMOD(gyear,19) < 10)) ? shiftedepact + 1 : shiftedepact) ;
	paschalmoon = FixedFromGregorian(gyear,4,19) - adjustedepact ;
	return(KDayAfter(paschalmoon,0)) ;
}

int YomKippur(gyear)
  int gyear ;
{ int hyear ;

	hyear = gyear - GregorianYearFromFixed(HebrewEpoch) + 1 ;
	return(FixedFromHebrew(hyear,TISHRI,10)) ;
}

int Passover(gyear)
  int gyear ;
{ int hyear ;

	hyear = gyear - GregorianYearFromFixed(HebrewEpoch) ;
	return(FixedFromHebrew(hyear,NISAN,15)) ;
}

#define DtoR (PI/180.0)		/* Degrees to radians */
#define DEG(x) ((x)*DtoR)	/* Degrees to radians */
#define DMS(degree,minute,second) (degree + minute/60.0 + second/3600.0)


double Poly(var,num,coefs)
  double var,coefs[] ;
  int num ;
{ double res,nth ; int i ;

	res = 0.0 ;
	for(i=0;i<num;i++)
	 { nth = (i == 0 ? 1.0 : nth * var) ;
	   res += nth * coefs[i] ;
	 } ;
	return(res) ;
}

double ArcTan2(x,quad)
  double x ; int quad ;
{ double alpha, result ;
	if (quad == 1 || quad == 4) { alpha = atan(x) / DtoR ; }
	 else { alpha = atan(x) / DtoR + 180.0 ; } ;
	result = MOD(alpha,360.0) ;
	return(result) ;
}


double Direction(locallat,locallong,focuslat,focuslong)
  double locallat,locallong,focuslat,focuslong ;
{ double denom,arg1 ; double x,y ;

	denom = cos(DEG(locallat)) * tan(DEG(focuslat)) - sin(DEG(locallat)) * cos(DEG(locallong-focuslong)) ;
	if (denom == 0.0) return(0) ;
	arg1 = sin(DEG(focuslong-locallong)) / denom ;
	x = ArcTan2(arg1,(denom < 0 ? 2 : 1)) ;
	y = MOD(x,360.0) ;
	return(y) ;
//	return(IMOD(ArcTan2(arg1,(denom < 0 ? 2 : 1)),360)) ;

}


/*	T I M E   &   A S T R O N O M Y				*/

/*	dt = date.time, lon = longitude in degrees */
double UniversalFromLocal(dt,lon)
  double dt,lon ;
{	return(dt - lon/360.0) ; }
double LocalFromUniversal(dt,lon)
  double dt,lon ;
{	return(dt + lon/360.0) ; }
double StandardFromUniversal(dt,tz)
  double dt ; int tz ;
{	return(dt + (1.0/24.0)*(double)tz) ; }
double UniversalFromStandard(dt,tz)
  double dt ; int tz ;
{	return(dt - (1.0/24.0)*(double)tz) ; }
double StandardFromLocal(dt,lon,tz)
  double dt,lon ; int tz ;
{	return(StandardFromUniversal(UniversalFromLocal(dt,lon),tz)) ; }
double LocalFromStandard(dt,lon,tz)
  double dt,lon ; int tz ;
{	return(LocalFromUniversal(UniversalFromStandard(dt,tz),lon)) ; }


double EphemerisCorrection(dt)
  double dt ;
{
  int year,Jan011900,Jul1,Jan011810,Jan01 ;
  double c,x ;
  static double p1[] = {-0.00002, 0.000297, 0.025184, -0.181133, 0.553040, -0.861938, 0.677066, -0.212591 } ;
  static double p2[] = {-0.000009, 0.003844, 0.083563, 0.865736, -4.867575, 15.845535, 31.332267, 38.291999, 28.316289, 11.636204, 2.043794 } ;

	year = GregorianYearFromFixed((int)floor(dt)) ;
	Jan011900 = FixedFromGregorian(1900,1,1) ;
	Jul1 = FixedFromGregorian(year,7,1) ;
	c = (1.0/36525.0) * (Jul1 - Jan011900) ;
	if (year >= 1988 && year <= 2019) return((year-1933.0)/(24.0*60.0*60.0)) ;
	if (year >= 1900 && year <= 1987) return(Poly(c,8,p1)) ;
	if (year >= 1800 && year <= 1899) return(Poly(c,11,p2)) ;
	if (year >= 1620 && year <= 1799)
	 return((196.58333 - 4.0675*(year - 1600) + 0.0219167*(year-1600)*(year-1600))/(24.0*60.0*60.0)) ;
	Jan011810 = FixedFromGregorian(1810,1,1) ;
	Jan01 = FixedFromGregorian(year,1,1) ;
	x = HR(12) + (Jan01 - Jan011810) ;
	return(((x*x)/41048480 - 15.0)/(24.0*60.0*60.0)) ;
}

double DynamicalFromUniversal(dt)
  double dt ;
{ return(dt + EphemerisCorrection(dt)) ; }

double UniversalFromDynamical(dt)
  double dt ;
{ double ec = EphemerisCorrection(dt) ;
  return(dt - ec) ;
}

double JulianCenturies(dt)
  double dt ;
{ double Jan2000 ;

	Jan2000 = 0.5 + (double)FixedFromGregorian(2000,1,1) ;
	return((1.0/36525.0) * (DynamicalFromUniversal(dt) - Jan2000)) ;
}

double Obliquity(dt)
  double dt ;
{ double res,c ;

	c = JulianCenturies(dt) ;
	res = DMS(23.0,26.0,21.448) + DMS(0,0,-48.8150)*c - DMS(0,0,0.00059)*c*c + DMS(0,0,0.001813)*c*c*c ;
	return(res) ;		/* Return result in Degrees */
}

double SiderealFromMoment(dt)
  double dt ;
{ double c,Jan2000,sfm ;

	Jan2000 = 0.5 + (double)FixedFromGregorian(2000,1,1) ;
	c = (1.0/36525) * (dt - Jan2000) ;
	sfm = MOD(280.46061837 + 36525 * 360.98564736629 * c + 0.000387933 * c * c - (1.0/38710000.0) * c * c * c,360.0) ;
	return(sfm) ;
}

double SolarLongitude(dt)
  double dt ;
{ double c,longitude,sum,res ; int i ;
  static double x[] =
   { 403406, 195207, 119433, 112392, 3891, 2819, 1721, 660, 350, 334, 314, 268, 242, 234, 158, 132, 129, 114,
     99, 93, 86, 78, 72, 68, 64, 46, 38, 37, 32, 29, 28, 27, 27,25, 24, 21, 21, 20, 18, 17, 14, 13, 13, 13, 12, 10, 10, 10, 10 } ;
  static double y[] =
   { 270.54861, 340.19128, 63.91854, 331.26220, 317.843, 86.631, 240.052, 310.26, 247.23, 260.87, 297.82, 343.14, 166.79, 81.53,
            3.50, 132.75, 182.95, 162.03, 29.8, 266.4, 249.2, 157.6, 257.8, 185.1, 69.9, 8.0, 197.1, 250.4, 65.3, 162.7, 341.5,
            291.6, 98.5, 146.7, 110.0, 5.2, 342.6, 230.9, 256.1, 45.3, 242.9, 115.2, 151.8, 285.3, 53.3, 126.6, 205.7, 85.9, 146.1 } ;
  static double z[] =
   { 0.9287892, 35999.1376958, 35999.4089666, 35998.7287385, 71998.20261, 71998.4403, 36000.35726, 71997.4812, 32964.4678,
            -19.4410, 445267.1117, 45036.8840, 3.1008, 22518.4434, -19.9739, 65928.9345, 9038.0293, 3034.7684, 33718.148, 3034.448,
            -2280.773, 29929.992, 31556.493, 149.588, 9037.750, 107997.405, -4444.176, 151.771, 67555.316, 31556.080, -4561.540,
            107996.706, 1221.655, 62894.167, 31437.369, 14578.298, -31931.757, 34777.243, 1221.999, 62894.511,
            -4442.039, 107997.909, 119.066, 16859.071, -4.578, 26895.292, -39.127, 12297.536, 90073.778 } ;

	c = JulianCenturies(dt) ;

	for(i=0,sum=0;i<49;i++) { sum += (double)x[i]*sin(DEG(MOD(y[i]+z[i]*c,360))) ; } ;
	longitude = 282.7771834 + 36000.76953744*c + 0.000005729577951308232 * sum ;
	res = longitude + Abberation(dt) + Nutation(dt) ;
	res = MOD(res,360.0) ;
	return(res) ;		/* Return result in degrees */
}
double SolarLongitudeAfter(dt,phi)
  double dt,phi ;
{
  double epsilon = 0.00001 ;
  double rate,tau,l,u,x ;

	rate = MeanTropicalYear/360.0 ;
	tau = dt + rate * MOD(phi - SolarLongitude(dt),360) ;
	l = (dt > tau-5 ? dt : tau-5) ;
	u = tau + 5 ;
	for(;;)
	 { x = (u + l) / 2 ;
	   if (u - l < epsilon) break ;
	   if (MOD(SolarLongitude(x) - phi,360) < 180.0)
	    { u = x ; } else { l = x ; } ;
	 } ;
	return(x) ;
}

double SolarLongitudeBefore(dt,phi)
  double dt,phi ;
{
	return(SolarLongitudeAfter(dt-MeanTropicalYear,phi)) ;
}

double Nutation(dt)
  double dt ;
{ double c,A,B,res ;

	c = JulianCenturies(dt) ;
	A = 124.90 - 1934.134*c + 0.002063*c*c ;
	B = 201.11 + 72001.5377*c + 0.00057*c*c ;
	res = -0.004778 * sin(DEG(MOD(A,360))) + -0.0003667*sin(DEG(MOD(B,360))) ;
	return(res) ;
}

double Abberation(dt)
  double dt ;
{ double c,res ;

	c = JulianCenturies(dt) ;
	res = 0.0000974 * cos(DEG(MOD(177.63 + 35999.01848*c,360))) - 0.005575 ;
	return(res) ;
}

double EquationOfTime(dt)
  double dt ;
{ double c,lon,anomaly,eccentricity,epsilon,y,equation ;

	c = JulianCenturies(dt) ;
	lon = 280.46645 + 36000.76983*c + 0.0003032*c*c ;
	anomaly = 357.52910 + 35999.05030*c - 0.0001559*c*c - 0.00000048*c*c*c ;
	eccentricity = 0.016708617 - 0.000042037*c - 0.0000001236*c*c ;
	epsilon = Obliquity(dt) ;
	y = tan(DEG(epsilon/2.0)) ;
	y *= y ;
	equation = 
	 (1.0/(2*PI)) *	(y * sin(DEG(MOD(2*lon,360))) + (-2.0 * eccentricity * sin(DEG(MOD(anomaly,360))))
			 + 4.0 * eccentricity * y * sin(DEG(MOD(anomaly,360)))
			 * cos(DEG(MOD(2*lon,360)))
			 + (-0.5 * y * y * sin(DEG(MOD(4.0*lon,360)))) +
			 (-1.25*eccentricity*eccentricity*sin(DEG(MOD(2.0*anomaly,360))))) ;
	return(SIGNUM(equation) * (ABS(equation) < 0.5 ? ABS(equation) : 0.5)) ;
}

double ApparentFromLocal(dt)
  double dt ;
{	return(dt + EquationOfTime(dt)) ; }

double LocalFromApparent(dt)
  double dt ;
{	return(dt - EquationOfTime(dt)) ; }

double MomentFromDepression(ok,approx,lat,lon,alpha)
  LOGICAL *ok ; double approx,alpha,lat,lon ;
{ double phi,t,delta,sineoffset,res ; int morning ;

	phi = DEG(lat) ;
	t = UniversalFromLocal(approx,lon) ;
	delta = asin(sin(DEG(Obliquity(t))) * sin(DEG(MOD(SolarLongitude(t),360)))) ;
	morning = MOD(approx,1) < 0.5 ;
	sineoffset = tan(phi) * tan(delta) + (sin(DEG(alpha))/(cos(delta)*cos(phi))) ;
	if (ABS(sineoffset) > 1.0) { *ok = FALSE ; return(0) ; } ;
	res = LocalFromApparent(floor(approx) + 0.5 + (morning ? -1 : 1) *
		(MOD(0.5 + (asin(sineoffset)/DtoR)/360,1) - 0.25)) ;
	*ok = TRUE ; return(res) ;
}

double Dawn(ok,date,lat,lon,tz,alpha)
  LOGICAL *ok ;
  int date,tz ; double lat,lon,alpha ;
{ double approx,result ;

	approx = MomentFromDepression(ok,(double)date + 0.25,lat,lon,alpha) ;
	result = MomentFromDepression(ok,(*ok ? approx : (double)date),lat,lon,alpha) ;
	if (!(*ok)) return(0.0) ;
	result = StandardFromLocal(result,lon,tz) ;
	return(result) ;
}

double Sunrise(ok,date,lat,lon,elevation,tz)
  LOGICAL *ok ;
  int date,elevation,tz ; double lat,lon ;
{ int h ; double R,dip,alpha,res ;

	h = (elevation < 0 ? 0 : elevation) ;
	R = 6.372E6 ;
	dip = acos(R/(R+h))/DtoR ;
	alpha = DMS(0,50,0) + dip ;
	res = Dawn(ok,date,lat,lon,tz,alpha) ;
	return(res) ;
}

double Dusk(ok,date,lat,lon,tz,alpha)
  LOGICAL *ok,date,tz ; double lat,lon,alpha ;
{ double approx,result ;

	approx = MomentFromDepression(ok,(double)date + 0.75,lat,lon,alpha) ;
	result = MomentFromDepression(ok,(*ok ? approx : (double)date+0.99),lat,lon,alpha) ;
	if (!(*ok)) return(0.0) ;
	result = StandardFromLocal(result,lon,tz) ;
	return(result) ;
}

double Sunset(ok,date,lat,lon,elevation,tz)
  LOGICAL *ok,date,elevation,tz ; double lat,lon ;
{ int h ; double R,dip,alpha,res ;

	h = (elevation < 0 ? 0 : elevation) ;
	R = 6.372E6 ;
	dip = acos(R/(R+h))/DtoR ;
	alpha = DMS(0,50,0) + dip ;
	res = Dusk(ok,date,lat,lon,tz,alpha) ;
	return(res) ;
}

#define MeanSynodicMonth 29.530588853

double NthNewMoon(n)
  int n ;
{
  int k,i ;
  double E,c,approx,correction,extra,additional,solaranomaly,lunaranomaly,moonargument,omega ;

  double vtilde[] = { -0.4072010, 0.1724110, 0.0160810, 0.0103910, 0.0073910, -0.0051410, 0.0020810, -0.0011110, -0.0005710, 0.0005610,
		     -0.0004210, 0.0004210, 0.0003810, -0.0002410, -0.0000710, 0.0000410, 0.0000410, 0.0000310, 0.0000310,
		     -0.0000310, 0.0000310, -0.0000210, -0.0000210, 0.0000210 } ;
  double wtilde[] = { 0, 1, 0, 0, 1, 1, 2, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ;
  double xtilde[] = { 0, 1, 0, 0, -1, 1, 2, 0, 0, 1, 0, 1, 1, -1, 2, 0, 3, 1, 0, 1, -1, -1, 1, 0 } ;
  double ytilde[] = { 1, 0, 2, 0, 1, 1, 0, 1, 1, 2, 3, 0, 0, 2, 1, 2, 0, 1, 2, 1, 1, 1, 3, 4 } ;
  double ztilde[] = { 0, 0, 0, 2, 0, 0, 0, -2, 2, 0, 0, 2, -2, 0, 0, -2, 0, -2, 2, 2, 2, -2, 0, 0 } ;

  double itilde[] = { 251.8810, 251.8310, 349.4210, 84.6610, 141.7410, 207.1410, 154.8410, 34.5210, 207.1910, 291.3410, 161.7210, 239.5610, 331.5510 } ;
  double jtilde[] = { 0.01632110, 26.64188610, 36.41247810, 18.20623910, 53.30377110, 2.45373210, 7.30686010, 27.26123910, 0.12182410, 1.84437910, 24.19815410, 25.51309910, 3.59251810 } ;
  double ltilde[] = { 0.00016510, 0.00016410, 0.00012610, 0.00011010, 0.00006210, 0.00006010, 0.00005610, 0.00004710, 0.00004210, 0.00004010, 0.00003710, 0.00003510, 0.00002310 } ;

	k = n - 24724 ;
	c = k / 1236.85 ;
	approx = 730125.59765 + MeanSynodicMonth * 1236.85 * c + 0.0001337 * c * c - 0.000000150 * c * c * c + 0.00000000073 * c * c * c * c ;
	E = 1.0 - 0.002516 * c - 0.0000074 * c * c ;
	solaranomaly = 2.5534 + 1236.85 * 29.10535669 * c - 0.0000218 * c * c - 0.00000011 * c * c * c ;
	lunaranomaly = 201.5643 + 385.8169352810 * 1236.85 * c + 0.0107438 * c * c - 0.00001239 * c * c * c - 0.000000058 * c * c * c * c ;
	moonargument = 160.7108 + 390.67050274 * 1236.85 * c - 0.0016341 * c * c - 0.00000227 * c * c * c + 0.000000011 * c * c * c * c ;
	omega = 124.7746 + (-1.56375580 * 1236.85) * c + 0.0020691 * c * c + 0.00000215 * c * c * c ;
	correction = -0.00017 * sin(DEG(MOD(omega,360))) ;
	for(i=0;i<24;i++)
	 { correction += vtilde[i] * pow(E,wtilde[i]) * sin(DEG(xtilde[i]*solaranomaly + ytilde[i]*lunaranomaly + ztilde[i]*moonargument)) ;
	 } ;
	extra = 0.000325 * sin(DEG(MOD(299.77 + 132.8475848 * c - 0.009173 * c * c,360))) ;
	additional = 0 ;
	for(i=0;i<13;i++)
	 { additional += ltilde[i] * sin(DEG(MOD(itilde[i] + jtilde[i] * k,360))) ; } ;
	return(UniversalFromDynamical(approx + correction + extra + additional)) ;
}

double LunarPhase(dt)
  double dt ;
{ double ll,sl,res ;

	ll = LunarLongitude(dt) ;
	sl = SolarLongitude(dt) ;
	res = MOD(ll-sl,360.0) ;
	return(res) ;
//	return(MOD((LunarLongitude(dt) - SolarLongitude(dt)),360.0)) ;
}

double NewMoonBefore(dt)
  double dt ;
{
  int n ;

	n = DtoI( (dt / MeanSynodicMonth) - LunarPhase(dt)/360) ;
	return(NthNewMoon(n)) ;
} ;
double NewMoonAfter(dt)
  double dt ;
{
  int n ;

	n = DtoI( (dt / MeanSynodicMonth) - LunarPhase(dt)/360) ;
	return(NthNewMoon(n+1)) ;
} ;

double LunarLongitude(dt)
  double dt ;
{ double c, res ;
  double meanmoon,elongation,solaranomaly,lunaranomaly,moonmode,correction,venus,jupiter,flatearth,E ;
  double p1[] = { 218.316459110, 481267.8813423610,  -.001326810, 1.0/538841,  -1.0/65194000 } ;
  double p2[] = { 297.850204210, 445267.111516810, -.0016310, 1.0/545868, -1.0/113065000 } ;
  double p3[] = { 357.529109210, 35999.050290910, -.000153610, 1.0/24490000 } ;
  double p4[] = { 134.963411410, 477198.867631310, 0.00899710, 1.0/69699, -1.0/14712000 } ;
  double p5[] = { 93.272099310, 483202.017527310, -.003402910, -1.0/3526000, 1.0/863310000 } ;
  int i ;
  double vtilde[] = { 6288774, 1274027, 658314, 213618, -185116, -114332, 58793, 57066, 53322, 45758, -40923, -34720, -30383, 15327, -12528, 10980, 10675, 10034, 8548, -7888, 
		      -6766, -5163, 4987, 4036, 3994, 3861, 3665, -2689, -2602, 2390, -2348, 2236, -2120, -2069, 2048, -1773, -1595, 1215, -1110, -892, -810, 759, -713, -700, 691, 
		      596, 549, 537, 520, -487, -399, -381, 351, -340, 330, 327, -323, 299, 294 } ;
  double wtilde[] = { 0, 2, 2, 0, 0, 0, 2, 2, 2, 2, 0, 1, 0, 2, 0, 0, 4, 0, 4, 2, 2, 1, 1, 2, 2, 4, 2, 0, 2, 2, 1, 2, 0, 0, 2, 2, 2, 4, 0, 3, 2, 4, 0, 2, 2, 2, 4, 0, 4, 1, 2, 0, 1, 3, 4, 2, 0, 1, 2 } ;
  double xtilde[] = { 0, 0, 0, 0, 1, 0, 0, -1, 0, -1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, -1, 0, 0, 0, 1, 0, -1, 0, -2, 1, 2, -2, 0, 0, -1, 0, 0, 1, -1, 2, 2, 1, -1, 0, 0, -1, 0, 1, 0, 1, 0, 0, -1, 2, 1, 0 } ;
  double ytilde[] = { 1, -1, 0, 2, 0, 0, -2, -1, 1, 0, -1, 0, 1, 0, 1, 1, -1, 3, -2, -1, 0, -1, 0, 1, 2, 0, -3, -2, -1, -2, 1, 0, 2, 0, -1, 1, 0, -1, 2, -1, 1, -2, -1, -1, -2, 0, 1, 4, 0, -2, 0, 2, 1, -2, -3, 2, 1, -1, 3 } ;
  double ztilde[] = { 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, -2, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, -2, 2, 0, 2, 0, 0, 0, 0, 0, 0, -2, 0, 0, 0, 0, -2, -2, 0, 0, 0, 0, 0, 0, 0 } ;


	c = JulianCenturies(dt) ;
	meanmoon = Poly(c,5,p1) ;
	elongation = Poly(c,5,p2) ;
	solaranomaly = Poly(c,4,p3) ;
	lunaranomaly = Poly(c,5,p4) ;
	moonmode = Poly(c,5,p5) ;
	E = 1 - 0.002516 * c - 0.0000074 * c * c ;
	correction = 0 ;
	for(i=0;i<59;i++)
	 { correction += vtilde[i] * pow(E,ABS(xtilde[i])) * sin(DEG(MOD(wtilde[i]*elongation + xtilde[i] * solaranomaly + ytilde[i] * lunaranomaly + ztilde[i] * moonmode,360))) ;
	 } ;
	correction *= 1.0/1000000.0 ;
	venus = (3958.0 / 1000000.0) * sin(DEG(MOD(119.75 + c * 131.849,360))) ;
	jupiter = (318.0 / 1000000.0) * sin(DEG(MOD(53.09 + c * 479264.29,360))) ;
	flatearth =  (1962.0 / 1000000.0) * sin(DEG(MOD(meanmoon - moonmode,360))) ;
	res = meanmoon + correction + venus + jupiter + flatearth + Nutation(dt) ;
	res = MOD(res,360) ;
	return(res) ;
}

double LunarPhaseBefore(dt,phi)
  double dt,phi ;
{
  double epsilon = 0.00001 ;
  double tau,l,u,x ;

	tau = dt - (MeanSynodicMonth / 360) * MOD(LunarPhase(dt) - phi,360.0) ;
	l = tau - 2 ;
	u = (dt < tau+2 ? dt : tau+2) ;
	for(;;)
	 { x = (u + l) / 2 ;
	   if (u - l < epsilon) break ;
	   if (MOD(LunarPhase(x) - phi,360.0) < 180)
	    { u = x ; } else { l = x ; } ;
	 } ;
	return(x) ;
}

double LunarPhaseAfter(dt,phi)
  double dt,phi ;
{
  double epsilon = 0.00001 ;
  double tau,l,u,x ;

	tau = dt + (MeanSynodicMonth / 360) * MOD(phi - LunarPhase(dt),360.0) ;
	l = (dt > tau-2 ? dt : tau-2) ;
	u = tau + 2 ;
	for(;;)
	 { x = (u + l) / 2 ;
	   if (u - l < epsilon) break ;
	   if (MOD(LunarPhase(x) - phi,360.0) < 180.0)
	    { u = x ; } else { l = x ; } ;
	 } ;

	return(x) ;
}

double LunarLatitude(dt)
  double dt ;
{ double c,longitude,elongation,solaranomaly,lunaranomaly,moonnode,E,latitude,venus,flatearth,extra ;
  double p1[] = { 218.316459110, 481267.8813423610, -.001326810, 1.0/538841, -1.0/65194000 } ;
  double p2[] = { 297.850204210, 445267.111516810, -.0016310, 1.0/545868, -1.0/113065000} ;
  double p3[] = { 357.529109210, 35999.050290910, -.000153610, 1.0/24490000 } ;
  double p4[] = { 134.963411410, 477198.867631310, 0.00899710, 1.0/69699, -1.0/14712000 } ;
  double p5[] = { 93.272099310, 483202.017527310, -.003402910, -1.0/3526000, 1.0/863310000 } ;

  int i ;
  double vtilde[] = { 5128122, 280602, 277693, 173237, 55413, 46271, 32573, 17198, 9266, 8822, 8216, 4324, 4200, -3359, 2463, 2211, 2065, -1870, 1828, -1794, -1749, -1565, -1491, -1475, 
		      -1410, -1344, -1335, 1107, 1021, 833, 777, 671, 607, 596, 491, -451, 439, 422, 421, -366, -351, 331, 315, 302, -283, -229, 223, 223, -220, -220, -185, 181, -177, 176, 166, -164, 132, -119, 115, 107 } ;
  double wtilde[] = { 0, 0, 0, 2, 2, 2, 2, 0, 2, 0, 2, 2, 2, 2, 2, 2, 2, 0, 4, 0, 0, 0, 1, 0, 0, 0, 1, 0, 4, 4, 0, 4, 2, 2, 2, 2, 0, 2, 2, 2, 2, 4, 2, 2, 0, 2, 1, 1, 0, 2, 1, 2, 0, 4, 4, 1, 4, 1, 4, 2 } ;
  double xtilde[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 1, -1, -1, -1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 1, 0, -1, -2, 0, 1, 1, 1, 1, 1, 0, -1, 1, 0, -1, 0, 0, 0, -1, -2 } ;
  double ytilde[] = { 0, 1, 1, 0, -1, -1, 0, 2, 1, 2, 0, -2, 1, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 1, 1, 0, 0, 3, 0, -1, 1, -2, 0, 2, 1, -2, 3, 2, -3, -1, 0, 0, 1, 0, 1, 1, 0, 0, -2, -1, 1, -2, 2, -2, -1, 1, 1, -2, 0, 0 } ;
  double ztilde[] = { 1, 1, -1, -1, 1, -1, 1, 1, -1, -1, -1, -1, 1, -1, 1, 1, -1, -1, -1, 1, 3, 1, 1, 1, -1, -1, -1, 1, -1, 1, -3, 1, -3, -1, -1, 1, -1, 1, -1, 1, 1, 1, 1, -1, 3, -1, -1, 1, -1, -1, 1, -1, 1, -1, -1, -1, -1, -1, -1, 1 } ;

	c = JulianCenturies(dt) ;
	longitude = Poly(c,5,p1) ;
	elongation = Poly(c,5,p2) ;
	solaranomaly = Poly(c,4,p3) ;
	lunaranomaly = Poly(c,5,p4) ;
	moonnode = Poly(c,5,p5) ;
	E = 1 - 0.002516 * c - 0.0000074 * c * c ;
	latitude = 0 ;
	for(i=0;i<60;i++)
	 { latitude += vtilde[i] * pow(E,ABS(xtilde[i])) * sin(DEG(MOD(wtilde[i]*elongation + xtilde[i]*solaranomaly + ytilde[i]*lunaranomaly + ztilde[i]*moonnode,360))) ;
	 } ; latitude *= (1.0/1000000.0) ;
	venus = (175.0/1000000.0) * (sin(DEG(MOD(119.75 + c * 131.849 + moonnode,360))) + sin(DEG(MOD(119.75 + c * 131.849 - moonnode,360)))) ;
	flatearth = (-2235.0/1000000) * sin(DEG(longitude)) + (127.0/1000000.0) * sin(DEG(MOD(longitude - lunaranomaly,360))) + (-115.0/1000000.0) * sin(DEG(MOD(longitude + lunaranomaly,360))) ;
	extra = (382.0 / 1000000.0) * sin(DEG(MOD(313.45 + c * 481266.484,360))) ;
	return(MOD(latitude + venus + flatearth + extra,360.0)) ;
}

double LunarAltitude(dt,locallat,locallong)
  double dt, locallat,locallong ;
{ double phi=locallat,psi=locallong, e, gamma, beta, alpha,delta,theta,H,altitude ;
  double var1,var2 ;

	e = SolarLongitude(dt) ;
	e = Obliquity(dt) ;
	gamma = LunarLongitude(dt) ;
	beta = LunarLatitude(dt) ;
	alpha = ArcTan2((sin(DEG(gamma)) * cos(DEG(MOD(e,360))) - tan(DEG(beta))*sin(DEG(MOD(e,360))))/cos(DEG(gamma)),(int)(gamma/90.0+1)) ;
	delta = asin(sin(DEG(beta))*cos(DEG(MOD(e,360))) + cos(DEG(beta))*sin(DEG(MOD(e,360)))*sin(DEG(gamma))) ;
	theta = SiderealFromMoment(dt) ;
	H = MOD(theta + psi - alpha,360) ;
	var1 = sin(DEG(phi)) * sin(delta);
	var2 =  cos(DEG(phi)) * cos(delta) * cos(DEG(H)) ;
	altitude = asin(var1+var2) ;
	altitude *= 180.0 /  PI ;
	return(MOD(altitude + 180,360) - 180) ;
}

/*	A L L O C A T I O N   W I T H I N   V 4   C U S T O M I Z A T I O N   T A B L E	*/
/*	v_mctAlloc - Allocates bytes within V4__MasterCustomTable, reallocs entire table if necessary		*/
/*	Call: offset - v_mctAlloc( gpi , bytes )
	  where offset is byte offset within V4__MasterCustomTable.Data[] of allocated buffer,
		gpi is process-information structure,
		bytes is number bytes requested,
		sptr (if not null) is pointer to pointer within Data[] to be reset if mct is reallocated	*/

int v_mctAlloc(gpi,bytes,sptr)
  struct V4C__ProcessInfo *gpi ;
  int bytes ;
  void **sptr ;
{ int res,inc,sptroffset ;

	if (gpi->mct == NULL)			/* First time? */
	 { gpi->mct = v4mm_AllocChunk(sizeof *gpi->mct,FALSE) ;
	   memset(gpi->mct,0,(char *)&gpi->mct->Data[0]-(char *)gpi->mct) ;
	   gpi->mct->Free = V4_MCT_InitialDataAlloc ; gpi->mct->NextAvail = 0 ;
	   gpi->mct->RevLevel = V4_MasterCustomTableRevLvl ;
	   gpi->mct->TotalBytes = sizeof *gpi->mct ;
	 } ;
	if (bytes > gpi->mct->Free)		/* Have to allocate larger structure ? */
	 { inc = (bytes > V4_MCT_ReallocBytes ? bytes + V4_MCT_ReallocBytes/2 : V4_MCT_ReallocBytes) ;
	   if (sptr != NULL) sptroffset = (char *)*sptr - (char *)gpi->mct->Data ;
	   gpi->mct = realloc(gpi->mct,gpi->mct->TotalBytes + inc) ;
	   gpi->mct->TotalBytes += inc ; gpi->mct->Free += inc ;
	   if (sptr != NULL) *sptr = &gpi->mct->Data[sptroffset] ;
	 } ;
	res = gpi->mct->NextAvail ;
	gpi->mct->NextAvail += bytes ; gpi->mct->Free -= bytes ;
	return(res) ;
} ;

/*	L O A D   V 4 S I T E . I N I   F I L E			*/

#define vIni_nvListMax 0x2000
#define VIni_ValueMax 255
struct vIni__LoadSection {
 UCCHAR secName[V4DPI_DictEntryVal_Max+1] ;	/* Section name (upper case) */
 struct vIni__LoadSection *vilsNext ;		/* Pointer to next one (NULL if end of chain) */
 LENMAX nvChars ;				/* Number of characters in nvList below */
 LENMAX nvMax ;					/* Max number of allocated characters */
 UCCHAR *nvList ;				/* String of name=value entries, <tab> delimited */
} ;
struct vIni__SectionEntry {
  INDEX secNext ;				/* Index/offset to next section, UNUSED if last one */
  UCCHAR secName[V4DPI_DictEntryVal_Max+1] ;
  UCCHAR nvList[1] ;				/* Tab delimited name=value list, size allocated as needed */
} ;
void v_LoadSiteIni(gpi,fileName,level)
  struct V4C__ProcessInfo *gpi ;
  UCCHAR *fileName ;
  INDEX level ;
{
  struct UC__File UCFile ;
  UCCHAR msgBuf[512],errbuf[512] ;
  static struct vIni__LoadSection *vils = NULL,*tvils = NULL ;
  COUNTER line ;

/*	If no file then just return */
	if (!v_UCFileOpen(&UCFile,v_GetV4HomePath(fileName),UCFile_Open_Read,FALSE,msgBuf,0))
	 { if (level > 0)						/* If included ini then output warning, if first one don't bother. NOTE: don't have message file yet - output explicit message */
	    { v_Msg(NULL,errbuf,"@* Err accessing ini file: %1U\n",fileName) ; vout_UCText(VOUT_Warn,0,errbuf) ; } ;
	   gpi->mct->iniValsOffset = UNUSED ;				/* Set this to UNUSED so it only has good value if we finish OK */
	   return ;
	 } ;
	UCFile.wantEOL = FALSE ;
	if (level == 0)
	 { vils = (struct vIni__LoadSection *)v4mm_AllocChunk(sizeof *vils,TRUE) ;
	   vils->nvMax = vIni_nvListMax ; vils->nvList = v4mm_AllocUC(vils->nvMax) ; ZUS(vils->nvList) ;
	   gpi->mct->iniValsOffset = UNUSED ;				/* Set this to UNUSED so it only has good value if we finish OK */
	 } ;
	for(line=1;;line++)
	 { UCCHAR *b ;
	   if (v_UCReadLine(&UCFile,UCRead_UC,msgBuf,sizeof msgBuf,msgBuf) < 0) break ;
	   if (msgBuf[0] < UClit(' ') || msgBuf[0] == UClit('!') || msgBuf[0] == UClit('#')) continue ;		/* Ignore blanks & comments */
/*	   Do we have a section [xxx], or name=value ? */
	   if (msgBuf[0] == UClit('['))
	    { UCCHAR secName[UCsizeof(vils->secName)] ;
	      b = UCstrchr(msgBuf,']') ; if (b != NULL) *b = UCEOS ;
	      if (UCstrlen(&msgBuf[1]) >= sizeof secName || b == NULL)
	       { v_Msg(NULL,errbuf,"@* Invalid Section name (or too long) at line %1d in %1U\n",line,fileName) ; vout_UCText(VOUT_Warn,0,errbuf) ; return ; } ;
	      UCcnvupper(secName,&msgBuf[1],UCsizeof(secName)) ;
	      if (vils->nvChars == 0) { UCstrcpy(vils->secName,secName) ; } ;		/* If first time then init core vils */
	      for(tvils=vils;tvils!=NULL;tvils=tvils->vilsNext)
	       { if (UCstrcmp(secName,tvils->secName) == 0) break ; } ;
	      if (tvils == NULL)
	       { tvils = (struct vIni__LoadSection *)v4mm_AllocChunk(sizeof *vils,TRUE) ;
		 tvils->nvMax = vIni_nvListMax ; tvils->nvList = v4mm_AllocUC(vils->nvMax) ; ZUS(tvils->nvList) ;
		 UCstrcpy(tvils->secName,secName) ;
		 if (vils == NULL) { vils = tvils ; }
		  else { tvils->vilsNext = vils ; vils = tvils ; } ;
	       } ;
	      continue ;
	    } ;
	   b = UCstrchr(msgBuf,'=') ;
	   if (b == NULL)				/* If no '=' then must have a command */
	    { b = UCstrchr(msgBuf,' ') ;
	      if (b == NULL)
	       { v_Msg(NULL,errbuf,"@* Invalid syntax (missing '=' or space) at line %1d in %2U\n",line,fileName) ; vout_UCText(VOUT_Warn,0,errbuf) ; return ; } ;
	      *b = UCEOS ;
	      if (UCstrcmpIC(msgBuf,UClit("INCLUDE")) == 0)
	       { v_LoadSiteIni(gpi,b+1,level+1) ; }
	       else { v_Msg(NULL,errbuf,"@* Invalid command at line %1d in %2U\n",line,fileName) ; vout_UCText(VOUT_Warn,0,errbuf) ; return ; } ;
	      continue ;
	    } ;
/*	   Have name=value - add to current section */
	   if (tvils == NULL) { v_Msg(NULL,errbuf,"@* Cannot define value without prior [section] at line %1d in %2U\n",line,fileName) ; vout_UCText(VOUT_Warn,0,errbuf) ; return ; } ;
	   if (UCstrlen(msgBuf) + tvils->nvChars + 2 >= tvils->nvMax)
	    { v_Msg(NULL,errbuf,"@* Exceeded max number of section values at line %1d in %2U\n",line,fileName) ; vout_UCText(VOUT_Warn,0,errbuf) ; return ; } ;
	   { UCCHAR *s = msgBuf, *d0 = &tvils->nvList[UCstrlen(tvils->nvList)], *d ;
	     d = d0 ; *(d++) = UCEOLbt ;				/* Start each line with UCEOLvt (will make it faster to search later on) */
	     for(;*s!=UClit('=');s++) { *(d++) = UCTOUPPER(*s) ; } ;	/* Convert everything before '=' to upper case */
	     for(;*s!=UCEOS;s++)
	      { if (*s != UClit('\\')) { *(d++) = *s ; continue ; } ;
	        switch(*(s++))
		 { default:		v_Msg(NULL,errbuf,"@* Invalid escape sequence ('\\x') at line %1d in %2U\n",line,fileName) ; vout_UCText(VOUT_Warn,0,errbuf) ; return ;
		   case UClit('t'):	*(d++) = UClit('\t') ; break ;
		   case UClit('l'):	*(d++) = 10 ; break ;
		   case UClit('n'):	*(d++) = UCNL ; break ;
		   case UClit('r'):	*(d++) = UClit('\r') ; break ;
		   case UClit('\\'):	*(d++) = UClit('\\') ; break ;
		 } ;
	      } ; *d = UCEOS ;
	     tvils->nvChars += (d - d0) ;
	   }
	 } ;
	v_UCFileClose(&UCFile) ;
	if (level > 0) return ;
/*	Leaving for good - consolidate sections into single structure & free up stuff */
	{ struct vIni__SectionEntry *vise, *tvise ;
	  COUNTER bytes = 0 ; INDEX offset ;
	  for(tvils=vils;tvils!=NULL;tvils=tvils->vilsNext)
	   { bytes += ALIGN(((sizeof *vise) + (UCstrlen(tvils->nvList) * sizeof(UCCHAR)))) ; } ;
	  bytes = ALIGN64(bytes) ;
	  offset = v_mctAlloc(gpi,bytes,NULL) ; gpi->mct->iniValsOffset = offset ; gpi->mct->iniValBytes = bytes ;
	  vise = (struct vIni__SectionEntry *)&gpi->mct->Data[offset] ; tvise = vise ;
	  for(tvils=vils;tvils!=NULL;)
	   { struct vIni__LoadSection *svils ;
	     UCstrcpy(tvise->secName,tvils->secName) ; UCstrcpy(tvise->nvList,tvils->nvList) ;
	     if (tvils->vilsNext == NULL)
	      { tvise->secNext = UNUSED ;		/* Mark last one */
	      } else
	      { tvise->secNext = ALIGN((sizeof *vise) + (UCstrlen(tvils->nvList) * sizeof(UCCHAR))) ;
		tvise = (struct vIni__SectionEntry *)(((char *)tvise) + tvise->secNext) ;
	      } ;
	     v4mm_FreeChunk(tvils->nvList) ; svils = tvils->vilsNext ; v4mm_FreeChunk(tvils) ; tvils = svils ;
	   } ;
	  gpi->mct->iniValXORKey = vRan64_RandomU64() ;		/* Perform mickey-mouse encryption of ini values */
	  { INDEX i ; B64INT *l = (B64INT *)vise ;
	    for(i=1;i<=bytes/sizeof(B64INT);i++,l++) { *l ^= gpi->mct->iniValXORKey ; } ;
	  }
	}
}

/*	vIni_Lookup - Returns value associated with "section.name" from pre-loaded v4site.ini file */
UCCHAR *vIni_Lookup(ctx,name)
  struct V4C__Context *ctx ;
  UCCHAR *name ;
{ struct vIni__SectionEntry *vise ;
  UCCHAR secName[V4DPI_DictEntryVal_Max+1], eName[V4DPI_DictEntryVal_Max+1+1] ;
  static UCCHAR value[VIni_ValueMax+1] ; INDEX dx,sx ;

	if (gpi->vise == NULL) return(NULL) ;		/* Nothing to look at? */
	for(sx=0,dx=0;dx<V4DPI_DictEntryVal_Max && name[sx]!=UClit('.');sx++)
	 { secName[dx++] = UCTOUPPER(name[sx]) ; } ; secName[dx] = UCEOS ;
	eName[0] = UCEOLbt ;				/* Start entry with UCEOLbt so we can do simple string search */
	for(dx=1,sx++;dx<V4DPI_DictEntryVal_Max+1 && name[sx]!=UCEOS;sx++)
	 { eName[dx++] = UCTOUPPER(name[sx]) ; } ; eName[dx] = UCEOS ;
	for(vise=gpi->vise;;)
	 { if (UCstrcmp(vise->secName,secName) == 0)
	    { UCCHAR *e = UCstrstr(vise->nvList,eName) ; if (e == NULL) return(NULL) ;
	      e = UCstrchr(e,'=') ; if (e == NULL) return(NULL) ;
	      for(dx=0;dx<VIni_ValueMax;dx++)
	       { value[dx] = *(++e) ; if (value[dx] == UCEOLbt || value[dx] == UCEOS) { value[dx] = UCEOS ; return(value) ; } ; } ;
	    } ;
	   if (vise->secNext == UNUSED) break ;
	   vise = (struct vIni__SectionEntry *)((char *)vise + vise->secNext) ;
	 } ;
	return(NULL) ;
}

/*	M E S S A G E   ( E R R O R )   R O U T I N E S		*/

struct V4MSG__MsgInfo *v_LoadMsgFile(gpi)
  struct V4C__ProcessInfo *gpi ;
{ struct V4MSG__MsgInfo *mi ;
//  FILE *fp ;
  struct UC__File UCFile ;
  char ml[256] ; int mlbfree ; char lastmne[sizeof mi->Entry[0].Mnemonic] ;
  int line,l,offset; char *p,*q ; UCCHAR *filename=NULL, errbuf[512],msgbuf[512] ;

//	mi = v4mm_AllocChunk(sizeof *mi,FALSE) ;
	offset = v_mctAlloc(gpi,sizeof *mi,NULL) ; mi = (struct V4MSG__MsgInfo *)&gpi->mct->Data[offset] ; gpi->mct->MsgInfoOffset = offset ;
	mi->Count = 0 ; mlbfree = 0 ; mi->InOrder = TRUE ; mi->LastMsgIndex = 0 ; memset(mi->OKtoThrow,0,V4MSG_MIMax) ; memset(mi->SetToThrow,0,V4MSG_MIMax) ;

	if (filename == NULL) filename = UClit("v4errormsgtable.v4i") ;
	if (!v_UCFileOpen(&UCFile,v_GetV4HomePath(filename),UCFile_Open_Read,FALSE,msgbuf,0))
	 { v_Msg(NULL,errbuf,"@* Err accessing V4 message initialization file: %1U\n",msgbuf) ;
	   vout_UCText(VOUT_Warn,0,errbuf) ; return(NULL) ;
	 } ;
	ZS(lastmne) ;
/*	Loop thru file & load up messages */
	for(line=1;mi->Count<V4MSG_MIMax;line++)
	 { 
//	   if (fgets(ml,sizeof ml,fp) == NULL) break ;
	   if (v_UCReadLine(&UCFile,UCRead_UTF8,ml,sizeof ml,errbuf) < 0) break ;
	   if (ml[0] < ' ' || ml[0] == '!') continue ;		/* Ignore blanks & comments */
	   p = strchr(ml,'\t') ;				/* Up to first tab is message id/mnemonic */
	   if (p == NULL) { printf("%d: Missing tab-delimited mnemonic in message file\n",line) ; continue ; } ;
	   *p = '\0' ; if (strlen(ml) >= sizeof mi->Entry[0].Mnemonic) { printf("%d: Tab-delimited mnemonic too big in message file\n",line) ; continue ; } ;
	   strcpy(mi->Entry[mi->Count].Mnemonic,ml) ;
	   if (strcmp(ml,lastmne) <= 0) { mi->InOrder = FALSE ; printf("%d: Mnemonics not in ascending lexicographic order in message file\n",line) ; } ;
/*	   Now position p to begin of message */
	   for(p=p+1;;p++) { if (*p > ' ' || *p == '\0') break ; } ;
	   q = strchr(p,'\n') ; if (q != NULL) *q = '\0' ;
	   l = strlen(p) ;
	   if (strcmp(&p[l-6],"\tTHROW") == 0)
	    { mi->OKtoThrow[mi->Count] = TRUE ; l -= 6 ; p[l] = '\0' ; } ;
	   l++ ;						/* Add one for trailing nul character */
//	   if (l > mlbfree)				/* Need more buffer space? */
//	    { mlb = v4mm_AllocChunk(mlbfree=5000,FALSE) ;
//	    } ;
//	   mi->Entry[mi->Count].Text = mlb ; strcpy(mlb,p) ; mlb += l ; mlbfree -= l ;
	   offset = v_mctAlloc(gpi,l,&mi) ;		/* Allocate space for this message */
	   mi->Entry[mi->Count].TextOffset = offset ; strcpy(&gpi->mct->Data[offset],p) ;
	   strcpy(lastmne,mi->Entry[mi->Count].Mnemonic) ; mi->Count++ ;
	 } ;
	if (mi->Count >= V4MSG_MIMax)
	 printf("%%V4W- V4 Internal Error Message Buffer Overflow (%d > %d)\n",mi->Count,V4MSG_MIMax) ;
//	fclose(fp) ;
	v_UCFileClose(&UCFile) ;
	return(mi) ;
}

#ifdef ALTVARGS
LOGICAL v_Msg(va_alist)
  va_dcl
{ va_list ap ;
  struct V4C__Context *ctx ;
  char *dst ;
  char *name ;
#else
LOGICAL v_Msg(ctx,dst,name)
  struct V4C__Context *ctx ;
  UCCHAR *dst ;
  char *name ;
//  int arg1,arg2 ;
{ va_list ap ;
#endif
  struct V4__MasterCustomTable *mct ;
  struct V4DPI__DimInfo *di ;
  struct V4MSG__MsgInfo *mi ;
  struct V4LEX__TknCtrlBlk  *tcb ;
  unsigned char *mask ; char maskbuf[64] ;
  int at,ax, argtype[10] ; int argint ; double argdbl ; char *argptr ; UCCHAR *arguc ; B64INT argbi ;
  int i,j,d,gotentry,ok,byte,u16,remain ; UCCHAR buf[VOUT_FileType_Buffer_Max],mbuf[VOUT_FileType_Buffer_Max] ;
#ifndef ISLITTLEENDIAN
  unsigned short s1,s2 ; unsigned char *b ;
#endif

#ifdef ALTVARGS
	va_start(ap) ;
	ctx = va_arg(ap,struct V4C__Context *) ; dst = va_arg(ap,UCCHAR *) ; name = va_arg(ap,char *) ;
#endif

/*	If ctx is null then pull from ProcessInfo, if dst also null then make it ErrorMsgAux
	If dst is null but ctx is not then make dst = ErrorMsg					*/
	if (ctx == NULL)
	 { if (gpi == NULL) gpi = v_GetProcessInfo() ;
	   ctx = gpi->ctx ;
//	   if (dst == NULL) dst = ctx->ErrorMsgAux ;
	 } ;
//	if (dst == NULL) dst = ctx->ErrorMsg ;
	
/*	If name starts with '*' then assume this is a status message (as opposed to V4 failure message) */
	if (name[0] == '*')
	 { name = &name[1] ;			/* Skip over the '*' (but don't check for ctx->disableFailures) */
	 } else if (name[0] == '@')		/* If starts with '@' then name is message (and probably a status message) */
	 { mask = &name[1] ; goto got_mask ;	/*   again - don't check for ctx->disableFailures */
	 } else
	 { if (ctx == NULL ? FALSE : ctx->disableFailures)	/* If falure processing disabled then return immediately */
	   { if (dst != NULL) { ZUS(dst) ; } ; return(FALSE) ; } ;
	 } ;
	
	mct = gpi->mct ;	mi = gpi->mi ;
	gotentry = FALSE ;
	if (mi != NULL)					/* Look for message */
	 { if (mi->InOrder)				/* If mnemonics in order then do binary search */
	    { int first,last ;
	      for(first=0,last=mi->Count-1;;)
	       { i = (last + first) / 2 ; j = strcmp(name,mi->Entry[i].Mnemonic) ;
		 if (j == 0) break ; if (j < 0) { last = i - 1 ; } else { first = i + 1 ; } ;
		 if (first > last) { i = mi->Count + 1 ; break ; } ;
	       } ;
	    } else					/* Have to do linear search (bummer) */
	    { for(i=0;i<mi->Count;i++)
	       { if (strcmp(name,mi->Entry[i].Mnemonic) == 0) break ; } ;
	    } ;
	   if (i >= mi->Count)				/* Did we find it? */
	    { sprintf(maskbuf,"Unknown MsgId: %s",(strlen(name) > 30 ? "too long.." : name)) ; mask = maskbuf ;
	    } else { mask = (char *)&mct->Data[mi->Entry[i].TextOffset] ; gotentry = TRUE ; mi->LastMsgIndex = i ; } ;
	 } else
	 { sprintf(maskbuf,"MsgId: %s",(strlen(name) > 30 ? "too long.." : name)) ; mask = maskbuf ;
	 } ;
got_mask:
	for(i=0;mask[i]!='\0';i++)	/* Spin thru mask to figure out types of each argument */
	 { if (mask[i] != '%') continue ;		/* Only interested in %nf on this pass */
	   if (mask[++i] == '%') continue ;
	   ax = mask[i] - '0' ;				/* ax = argument index (s/b 1-9) */
	   if (ax < 0 || ax > 9)
	    { v_Msg(ctx,dst,"@Invalid v_Msg(%1s) mask(%2s) argument index",name,mask) ; return(FALSE) ; } ;
	   switch (mask[++i])
	    { default:		v_Msg(ctx,dst,"@Invalid v_Msg mask(%1s) format code",mask) ; return(FALSE) ;
	      case 'W':
	      case 'd':
	      case 'x':
	      case 'i':		at = 0 ; break ;	/* 0 = int */
	      case 'g':
	      case 'f':		at = 1 ; break ;	/* 1 = double */
	      case 'm':		at = 2 ; break ;
	      case 's':		at = 2 ; break ;	/* 2 = char * (pointer) */
	      case 'S':		at = 0 ; break ;
	      case 'p':		at = 3 ; break ;
	      case 't':		at = 4 ; break ;	/* (tree) */
	      case 'B':		at = 4 ; break ;	/* 4 = B64INT */
	      case 'Q':	
	      case 'P':		at = 2 ; break ;
	      case 'N':		at = 2 ; break ;
	      case 'T':		at = 0 ; break ;
	      case 'D':		at = 0 ; break ;
	      case 'Y':		at = 0 ; break ;
	      case 'E':		at = 0 ; break ;
	      case 'M':		at = 0 ; break ;
	      case 'A':		at = 2 ; break ;
	      case 'F':		at = 2 ; break ;
	      case 'L':		at = 0 ; break ;
	      case 'U':		at = 3 ; break ;
	      case 'u':		at = 3 ; break ;
	      case 'O':		at = 0 ; break ;
	      case '#':		at = 0 ; break ;
	    } ; if (ax > 0) argtype[ax-1] = at ;
//printf("Setting argtype[%d]=%d\n",ax-1,at) ;
	 } ;
/*	Now spin thru message mask and format actual message into mbuf */
	for(i=0,d=0;mask[i]!='\0' && d < VOUT_FileType_Buffer_Max-50;i++)
	 { 
/*	   Check for escape ('\x') characters NOTE - ONLY INTERPRETING '\n' FOR NOW (VEH090110) */
	   if (mask[i] == '\\')
	    { switch (mask[++i])
	       { default:	mbuf[d++] = UClit('\\') ; mbuf[d++] = mask[i] ; break ;
	         case '\\':	mbuf[d++] = UClit('\\') ; break ;
	         case 'n':	mbuf[d++] = UCNL ; break ;
	       } ; continue ;
	    } ;
/*	   mask s/b UTF-8 - so if not a '%' see if it multiple mask[] bytes need to be expanded into single UTF-16 mbuf[] wide character */
	   if (mask[i] != '%')
	    { 
//	      mbuf[d++] = mask[i] ;
	      byte = mask[i] ;
	      if (byte < 0x80) { u16 = byte ; remain = 0 ; }
	       else if (byte < 0xE0) { u16 = byte & 0x1F ; remain = 1 ; }
	       else if (byte < 0xF0) { u16 = byte & 0x0F ; remain = 2 ; } 
	       else if (byte < 0xF8) { u16 = byte & 0x07 ; remain = 3 ; }
	       else { u16 = byte ; remain = 0 ;	} ;	/* This should really be an error - but try to output something */
	      for(;remain>0&&mask[i+1]!='\0';remain--)	/* Note: Not checking for invalid UTF-8 codes */
	       { u16 <<= 6 ;
	         u16 |= mask[i+1] & 0x3F ;
	         i++ ;
	       } ;
	      if (u16 < 0x10000)
	       {
#ifdef ISLITTLEENDIAN
                 mbuf[d++] = u16 ;
#else
	         mbuf[d++] = (u16 >> 8) | ((u16 & 0xFF) << 8) ;
#endif
	       } else if (u16 < 0x110000)
	       { u16 -= 0x10000 ;			/* This is converting to UTF-16, not necessarily UCCHAR ! */
#ifdef ISLITTLEENDIAN
		 mbuf[d++] = 0xD800 | (u16 >> 10) ;
		 mbuf[d++] = 0xDC00 | (u16 & 0x3FF) ;
#else
		 s1 = 0xD800 | (u16 >> 10) ;
		 b = (unsigned char *)mbuf[d++] ;
		 *b = s1 ; *(b+1) = s1 >> 8 ;
		 s2 = 0xDC00 | (u16 & 0x03FF) ;
		 b = (unsigned char *)mbuf[d++] ;
		 *b = s2 ; *(b+1) = s2 >> 8 ;
#endif
	       } else { mbuf[d++] = u16 ; }	/* Again - this is not a vaild character, but keep plugging anyway */
	      continue ;
	    } ;

	   if (mask[++i] == '%') { mbuf[d++] = UClit('%') ; continue ; } ;
	   ax = mask[i] - '0' ;
#ifdef ALTVARGS
	   va_start(ap) ;
	   va_arg(ap,struct V4C__Context *) ; va_arg(ap,char *) ; va_arg(ap,char *) ;
#else
	   va_start(ap,name) ;
//printf("ap = %p, &name=%p\n",ap,&name) ;
#endif
	   for(j=0;j<ax;j++)
	    { //printf("switch (%d)\n",argtype[j]) ;
	      switch (argtype[j])
	       { case 0:	argint = va_arg(ap,int) ; /* printf(" ap is now %p\n",ap) ; */ break ;
	         case 1:	argdbl = va_arg(ap,double) ; break ;
		 case 2:	argptr = va_arg(ap,char *) ; break ;
		 case 3:	arguc = va_arg(ap,UCCHAR *) ; break ;
		 case 4:	argbi = va_arg(ap,B64INT) ; break ;
	       } ;
//printf(" j=%d, argtype[j]=%d, argint=%p\n",j,argtype[j],argint) ;
	    } ;
	   va_end(ap) ;
	   switch (mask[++i])
	    { default:		v_Msg(ctx,mbuf,"@Invalid v_Msg mask(%1s) format code",mask) ; return(FALSE) ;
	      case 'W':		ZUS(buf) ; if (argint > 0) { buf[argint--] = UCEOS ; for(;argint>=0;argint--) { buf[argint] = UClit(' ') ; } ; } ; break ;
	      case 'd':
	      case 'i':		UCsprintf(buf,UCsizeof(buf),UClit("%d"),argint) ; break ;
	      case 'B':		UCsprintf(buf,UCsizeof(buf),UClit("%lld"),argbi) ; break ;
	      case 'g':
	      case 'f':		UCsprintf(buf,UCsizeof(buf),UClit("%g"),argdbl) ; break ;
	      case 'S':		UCstrcpy(buf,v4im_GetEnumToUCVal(argint)) ; break ;
	      case 'U':		UCstrncpy(buf,arguc,UCsizeof(buf)-1) ; buf[UCsizeof(buf)-1] = UCEOS ; break ;
	      case 'u':		UCstrncpy(buf,arguc,1000) ; buf[1000] = UCEOS ; break ;
	      case 's':		UCstrcpyAtoU(buf,argptr) ; break ;
	      case 'p':		UCstrncpy(buf,arguc,15) ; if (UCstrlen(buf) > 15) { buf[15] = UCEOS ; UCstrcat(buf,UClit("...")) ; } ; break ;
	      case 'P':		if (argptr == NULL) { UCstrcpy(buf,UClit("?undefined?")) ; break ; } ;
#ifdef V4_BUILD_SECURITY
				if(((P *)argptr)->PntType == V4DPI_PntType_Isct)
				 { int hnum ; LOGICAL okToShow=TRUE ;
				   union V4DPI__IsctSrc vis ; struct V4LEX__CompileDir *vcd ;
				   vis.iVal = ((P *)argptr)->Value.Isct.vis.iVal ; hnum = vis.c.HNum ;
				   if ((vcd = v4trace_LoadVCDforHNum(ctx,hnum,TRUE)) != NULL)
				    { if (vis.c.vcdIndex < vcd->fileCount)
				       { if (vcd->File[vis.c.vcdIndex].spwHash64 != 0 && vcd->File[vis.c.vcdIndex].spwHash64 != gpi->spwHash64) okToShow = FALSE ; } ;
				    } ;
				   if (okToShow) { v4dpi_PointToString(buf,(P *)argptr,ctx,V4DPI_FormatOpt_Trace) ; }
				    else { UCstrcpy(buf,UClit("restricted")) ; } ;
				 } else {  v4dpi_PointToString(buf,(P *)argptr,ctx,V4DPI_FormatOpt_Trace) ; } ;
#else
				v4dpi_PointToString(buf,(P *)argptr,ctx,V4DPI_FormatOpt_Trace) ;
#endif
				if (UCstrlen(buf) > 200) { buf[197] = UCEOS ; UCstrcat(buf,UClit("...")) ; } ;
				break ;
	      case 'm':		if (gpi->mi != NULL)					/* Look for message */
				 { int i,j ;
				   if (gpi->mi->InOrder)				/* If mnemonics in order then do binary search */
				    { int first,last ;
				      for(first=0,last=gpi->mi->Count-1;;)
				       { i = (last + first) / 2 ; j = strcmp(argptr,gpi->mi->Entry[i].Mnemonic) ;
					 if (j == 0) break ; if (j < 0) { last = i - 1 ; } else { first = i + 1 ; } ;
					 if (first > last) { i = gpi->mi->Count + 1 ; break ; } ;
				       } ;
				    } else					/* Have to do linear search (bummer) */
				    { for(i=0;i<gpi->mi->Count;i++) { if (strcmp(argptr,gpi->mi->Entry[i].Mnemonic) == 0) break ; } ; } ;
				   if (i >= gpi->mi->Count)				/* Did we find it? */
				    { UCstrcpyAtoU(buf,argptr) ;
				    } else { UCstrcpy(buf,ASCretUC((char *)&gpi->mct->Data[gpi->mi->Entry[i].TextOffset])) ; } ;
				 } else { UCstrcpyAtoU(buf,argptr) ; } ;
				break ;
	      case 'N':		{ UCCHAR *srcFile ; int hnum ;
				  union V4DPI__IsctSrc vis ; struct V4LEX__CompileDir *vcd ;
				  vis.iVal = ((P *)argptr)->Value.Isct.vis.iVal ; hnum = vis.c.HNum ;
				  if ((vcd = v4trace_LoadVCDforHNum(ctx,hnum,TRUE)) != NULL)
				   { srcFile = (vis.c.vcdIndex < vcd->fileCount ? vcd->File[vis.c.vcdIndex].fileName : NULL) ;
				     if (srcFile != NULL)
				      { if (vcd->File[vis.c.vcdIndex].spwHash64 != 0 && vcd->File[vis.c.vcdIndex].spwHash64 != gpi->spwHash64) srcFile = UClit("restricted-file") ; } ;
				   } else { srcFile = NULL ; } ;
				  if (srcFile == NULL) { ZUS(buf) ; }
				   else { UCstrcpy(buf,srcFile) ; UCstrcat(buf,UClit(" ")) ; UCstrcat(buf,v4dbg_FormatLineNum(vis.c.lineNumber)) ; }
				} ;
				break ;
	      case 'Q':		v4dpi_PointToString(buf,(P *)argptr,ctx,V4DPI_FormatOpt_Trace) ; break ;
	      case 'T':		UCstrcpy(buf,(ax == 0 ? v4im_LastTagName() : v4im_TagName(argint))) ; break ;
	      case 't':		UCsprintf(buf,UCsizeof(buf),UClit("%d-%d"),TREEID(argint),TREENODEID(argint)) ; break ;
	      case 'x':		UCsprintf(buf,UCsizeof(buf),UClit("%x"),argint) ; break ;
	      case 'D':		DIMINFO(di,ctx,argint) ;
				if (di != NULL)
				 { UCstrcpy(buf,di->DimName) ; }
				 else { UCsprintf(buf,UCsizeof(buf),UClit("Dim:?%d?"),argint) ; } ;
				break ;
	      case 'Y':		UCstrcpy(buf,v4im_PTName(argint)) ; break ;
	      case 'E':		if (argint == UNUSED) { i++ ; break ; } ;		/* If UNUSED then don't output standard intmod error prefix (but skip next 'space' in error message) */
				UCstrcpy(buf,UClit("[")) ; UCstrcat(buf,(name[0] == '@' ? UClit("@") : ASCretUC(name))) ;
				UCstrcat(buf,UClit("] ")) ; UCstrcat(buf,v4im_ModFailStr(argint)) ;
				break ;
	      case 'M':
#ifdef V4ENABLEMULTITHREADS
				if (ctx->pctx != NULL) { UCsprintf(buf,UCsizeof(buf),UClit("%s(thread %d)"),v4im_Display(argint),ctx->aggStream) ; }
				 else { UCstrcpy(buf,v4im_Display(argint)) ; } ;
#else

				UCstrcpy(buf,v4im_Display(argint)) ;
#endif
				break ;
	      case 'A':		UCstrncpy(buf,ctx->ErrorMsgAux,UCsizeof(buf) - 1) ; buf[UCsizeof(buf) - 1] = UCEOS ; break ;
	      case 'O':		UCstrcpyAtoU(buf,v_OSErrString(argint)) ; break ;
	      case '#':		UCsprintf(buf,UCsizeof(buf),UClit("#%c#"),'a'+argint) ; break ;
	      case 'L':
		tcb = gpi->MainTcb ;
		switch (argint)
		 {
		   case V4LEX_TknType_String:	tcb->UCstring[10] = '\0' ; UCstrcat(tcb->UCstring,UClit("...")) ;
						UCstrcpy(buf,UClit("(string:")) ; UCstrcat(buf,tcb->UCstring) ; UCstrcat(buf,UClit(")")) ; break ;
		   case V4LEX_TknType_Keyword:	UCstrcpy(buf,UClit("(keyword:")) ; UCstrcat(buf,tcb->UCkeyword) ; UCstrcat(buf,UClit(")")) ;  break ;
		   case V4LEX_TknType_Integer:	UCsprintf(buf,UCsizeof(buf),UClit("(integer:%d)"),tcb->integer) ; break ;
		   case V4LEX_TknType_Float:	UCsprintf(buf,UCsizeof(buf),UClit("(integer:%g)"),tcb->floating) ; break ;
		   case V4LEX_TknType_EOL:	UCstrcpy(buf,UClit("(end-of-line)")) ; break ;
		   case V4LEX_TknType_Punc:	UCstrcpy(buf,UClit("(punctuation:'")) ; UCstrcat(buf,tcb->UCkeyword) ; UCstrcat(buf,UClit("')")) ; break ;
		   case V4LEX_TknType_LInteger:	UCsprintf(buf,UCsizeof(buf),UClit("(long integer:%I64d)"),tcb->Linteger) ; break ;
		   case V4LEX_TknType_BString:	UCstrcpy(buf,UClit("(binary string)")) ; break ;
		   case V4LEX_TknType_PString:
		   case V4LEX_TknType_VString:	tcb->UCstring[10] = UCEOS ; UCstrcat(tcb->UCstring,UClit("...")) ;
						UCstrcpy(buf,UClit("(Vstring:")) ; UCstrcat(buf,tcb->UCstring) ; UCstrcat(buf,UClit(")")) ; break ;
		   case V4LEX_TknType_Error:	UCstrcpy(buf,UClit("(syntax error)")) ; break ;
		   case V4LEX_TknType_Isct:
		   case V4LEX_TknType_DES:
		   case V4LEX_TknType_Point:
		   case V4LEX_TknType_PointWithContext:
		   case V4LEX_TknType_Function:
		   case V4LEX_TknType_EOA:
		   case V4LEX_TknType_PointWithStar:	UCstrcpy(buf,UClit("(V4 element)")) ; break ;
		 } ;
		break ;
	      case 'F':		tcb = gpi->MainTcb ;
				if (tcb == NULL) { ZUS(buf) ; break ; } ;
				for(j=tcb->ilx,ok=FALSE;!ok && j>0;j--)		/* Scan up through token source stack for file */
				 { switch (tcb->ilvl[j].mode)
				    { default:	ZUS(buf) ; break ;
				      case V4LEX_InpMode_CmpFile:
				      case V4LEX_InpMode_TempFile:
				      case V4LEX_InpMode_File:
					UCstrcpy(buf,UClit("(")) ; UCstrcat(buf,vlex_FullPathNameOfFile(tcb->ilvl[j].file_name,NULL,0)) ;
					UCsprintf(&buf[UCstrlen(buf)],UCsizeof(buf),UClit(" %d)"),tcb->ilvl[j].current_line) ;
					ok = TRUE ; break ;
				    } ;
				 } ;
				break ;
	    } ;
	   for(j=0;buf[j]!=UCEOS && d<VOUT_FileType_Buffer_Max-50;j++) { mbuf[d++] = buf[j] ; } ;
	 } ;
	mbuf[d++] = UCEOS ;
/*	Now copy mbuf into dst */
	UCstrcpy((dst == NULL ? ctx->ErrorMsg : dst),mbuf) ;
	return(TRUE) ;
}

/*	vjson_SubParamsInJSON -  Takes 'Set Ajax JSON' and 'Set Ajax Errors' string and formats		*/
/*	  call vjson_SubParamsInJSON( out , outMax , in , results , eLevel )
	  where out is point to output buffer to receive formatted string,
		outMax is max number of characters in out,
		in is input string to be formatted,
		results is result string (AJAX results or error message),
		eLevel is error level (error, warn, ...), NULL if not specified				*/

void vjson_SubParamsInJSON(out,outMax,in,results,eLevel)
  UCCHAR *out ;
  LENMAX outMax ;
  char *in ;
  UCCHAR *results ;
  UCCHAR *eLevel ;
{ struct tm *stm ; time_t timer ;
  P *tpt ; int dimId ;
  INDEX ix,ox ; UCCHAR *b ; LOGICAL ok ;
  UCCHAR jobNum[64], ajaxFunc[64], curTime[32] ;

	if (in[0] == '@') in++ ;	/* If starts with '@' then skip (for compat with older versions) */
	ZUS(jobNum) ; ZUS(ajaxFunc) ; ZUS(curTime) ;
	time(&timer) ; stm = localtime(&timer) ; UCsprintf(curTime,UCsizeof(curTime),UClit("\"%d:%02d:%02d\""),stm->tm_hour,stm->tm_min,stm->tm_sec) ;
	dimId = v4dpi_DictEntryGet(gpi->ctx,0,UClit("jobId"),DIMDICT,NULL) ;
	if (dimId != 0) { DIMVAL(tpt,gpi->ctx,dimId) ; if (tpt != NULL) v4im_GetPointUC(&ok,jobNum,UCsizeof(jobNum),tpt,gpi->ctx) ; } ;
	dimId = v4dpi_DictEntryGet(gpi->ctx,0,UClit("ajaxMod"),DIMDICT,NULL) ;
	if (dimId != 0) { DIMVAL(tpt,gpi->ctx,dimId) ; if (tpt != NULL) v4im_GetPointUC(&ok,ajaxFunc,UCsizeof(jobNum),tpt,gpi->ctx) ; } ;
	
	for(ix=0,ox=0;in[ix]!=UCEOS;ix++)
	 { switch(in[ix])
	    { default:			if (ox < outMax-1) out[ox++] = in[ix] ; break ;
/*					%1U/%2U - old format, %e error, %r results, %j jobId, %f func/ajaxFunc, %t current time */
	      case '%':			if (in[ix+1] == '1' && in[ix+2] == 'U') { for(b=(eLevel == NULL ? results : eLevel);*b!=UCEOS;b++) { out[ox++] = *b ; } ; ix += 2 ; }
					 else if (in[ix+1] == '2' && in[ix+2] == 'U') { for(b=results;*b!=UCEOS;b++) { out[ox++] = *b ; } ; ix += 2 ; }
					 else if (in[ix+1] == 'e')  { out[ox++] = '"' ; for(b=eLevel;*b!=UCEOS;b++) { out[ox++] = *b ; } ; out[ox++] = '"' ; ix++ ; }
					 else if (in[ix+1] == 'r')  { for(b=results;*b!=UCEOS;b++) { out[ox++] = *b ; } ; ix++ ; }
					 else if (in[ix+1] == 'j')  { out[ox++] = '"' ; for(b=jobNum;*b!=UCEOS;b++) { out[ox++] = *b ; } ; out[ox++] = '"' ; ix++ ; }
					 else if (in[ix+1] == 'f')  { out[ox++] = '"' ; for(b=ajaxFunc;*b!=UCEOS;b++) { out[ox++] = *b ; } ; out[ox++] = '"' ; ix++ ; }
					 else if (in[ix+1] == 't')  { for(b=curTime;*b!=UCEOS;b++) { out[ox++] = *b ; } ; ix++ ; }
					 else { printf("%%Invalid AJAXJSON pattern: '%s'\n",gpi->patternAJAXJSON) ; } ;
					break ;
	    } ;
	 } ;
	out[ox++] = UCEOS ;
}

/*	v_ParseUPeriod - Parses UPeriod in various forms		*/
/*	Call: period = v_ParseUPeriod( ctx , input , di , errbuf )
	  where period is period value (VCAL_BadVal) if not valid,
		ctx is context,
		input is input string,
		di is DimInfo,
		errbuf is updated with error if bad value		*/

int v_ParseUPeriod(ctx,input,di,errbuf)
  struct V4C__Context *ctx ;
  UCCHAR *input ;
  struct V4DPI__DimInfo *di ;
  UCCHAR *errbuf ;
{ UCCHAR *delim ;
  int res,year,period,ppy ;

	if (input[0] == UClit('#'))				/* If #num then assume internal format */
	 { res = UCstrtol(input+1,&delim,10) ;
	   if (*delim == UCEOS) return(res) ;
	   v_Msg(ctx,errbuf,"ParseInternal") ; return(VCAL_BadVal) ;
	 } ;
	if (vuc_IsBgnId(input[0]))				/* Do we have a special keyword ? */
	 { if (v_IsNoneLiteral(input)) return(VCAL_UPeriod_None) ;
	   v_Msg(ctx,errbuf,"ParseUSyn",input,input) ; return(VCAL_BadVal) ;
	 } ;
/*	Parse year */
	year = UCstrtol(input,&delim,10) ;
/*	If delim is EOS & input is 4 digits then assume we got yypp */
	if (*delim == UCEOS && (UCstrlen(input) == 4 || UCstrlen(input) == 6))
	 { period = year % 100 ; year /= 100 ;
	 } else
	 { VCALADJYEAR(((di->ds.UPeriod.calFlags & VCAL_Flags_Historical) != 0),year) ;
	   if (!(*delim == UClit('/') || *delim == UClit('-') || *delim == UClit('.') || *delim == UClit('p') || *delim == UClit('P')))	/* Expecting year/period */
	    { v_Msg(ctx,errbuf,"ParseUSyn",input,delim) ; return(VCAL_BadVal) ; } ;
	   period = UCstrtol(delim+1,&delim,10) ;
	   if (*delim != UCEOS) { v_Msg(ctx,errbuf,"ParseUSyn",input,delim) ; return(VCAL_BadVal) ; } ;
	 } ;
	VCALADJYEAR(((di->ds.UPeriod.calFlags & VCAL_Flags_Historical) != 0),year) ;
	if (period >= 90)
	 { if (year >= VCAL_UYearMin && year <= VCAL_UYearMax) return(year * 100 + period) ;
	   v_Msg(ctx,errbuf,"ParseUPeriod",year,period,period,input) ;  return(VCAL_BadVal) ;
	 } ;
	ppy = (di->ds.UPeriod.periodsPerYear == 0 ? 13 : di->ds.UPeriod.periodsPerYear) ;
	if (year < VCAL_UYearMin || year > VCAL_UYearMax || period < 1 || period > ppy)
	 { v_Msg(ctx,errbuf,"ParseUPeriod",year,period,ppy,input) ;  return(VCAL_BadVal) ; } ;
	res = YYPPtoUPERIOD(year,period,ppy) ;
	if ((di->ds.UPeriod.calFlags & VCAL_Flags_Historical) != 0)
	 { year = UPERIODtoUYEAR(res,ppy) ;			/* See if it should be an historical-only period */
	   if (year > gpi->curYear) { v_Msg(ctx,errbuf,"DPIMNoFuture") ; return(VCAL_BadVal) ; } ;
	 } ;
	return(res) ;
}

/*	v_ParseCalendar - Parses text date to Calendar & returns as double value		*/
/*	Call: caldate = v_ParseCalendar( ctx , input , ymdOrder , type , caltype , timezone , futureOK )
	  where caldate is returned date (VCAL_BadVal if error (ctx->ErrorMsgAux updated with error)),
		ctx is context,
		input is input string (null terminated),
		ymdOrder is ordering of YMD (see V4LEX_YMDOrder_xxx),
		type is type of date specification - V4LEX_TablePT_xxx,
		caltype is calendar type (UNUSED for default),
		timezone is timezone to use as offset (VCAL_TimeZone_Local for local),
		futureOK is TRUE if future date-times OK, false otherwise			*/

double v_ParseCalendar(ctx,input,type,ymdOrder,caltype,timezone,futureOK)
  struct V4C__Context *ctx ;
  UCCHAR *input ;
  int type,ymdOrder ;
  int caltype,timezone ;
  LOGICAL futureOK ;
{ struct V4CI__CountryInfo *ci ;
  struct V4LI_LanguageInfo *li ; static int lix ;
  struct V4LI_CalendarInfo *cli ;
  int curx,i,yy,mm,dd,hr,min,sec ; UCCHAR *fp,*b,month[32],tzabbr[16] ; double d1,d2,cal,frac,ipart ;
  UCCHAR *p1,*p2,*p3,*p4, *yp, *mp, *dp ; UCCHAR p2ds,p3ds,p4ds ;
  static double zuluoffset = UNUSED ;

	if (UCempty(input)) return(VCal_NullDate) ;		/* VEH131017 - Empty string same as date:none */
	fp = input ;
	if (*fp == UClit('#')) { fp++ ; type = V4LEX_TablePT_Internal ; } ;
#undef DN
#define DN(var) var=0 ; for(;;fp++) { if (*fp != UClit(' ')) break ; } ; for(;;fp++) { if (*fp >= UClit('0') && *fp <= UClit('9')) { var*=10 ; var+= *fp - '0' ; } else { break ; } ; } ;
	switch(type)
	 { case V4LEX_TablePT_Internal:
		cal = UCstrtod(input,&fp) ; if (!(*fp == UCEOS || *fp == UClit(' '))) goto badterm ;
		return(cal) ;
	   case V4LEX_TablePT_YYMMDD:
		DN(i) if (!(*fp == UCEOS || *fp == UClit(' ') || *fp == UClit(':'))) goto badterm ;
		yy = i / 10000 ; mm = (i / 100) % 100 ; dd = i % 100 ;
		break ;
	   case V4LEX_TablePT_DDMMYY:
		DN(i) if (!(*fp == UCEOS || *fp == UClit(' ') || *fp == UClit(':'))) goto badterm ;
		dd = i / 10000 ; mm = (i / 100) % 100 ; yy = i % 100 ;
		break ;
	   case V4LEX_TablePT_MMDDYY:
		DN(i) if (!(*fp == UCEOS || *fp == UClit(' ') || *fp == UClit(':'))) goto badterm ;
		mm = i / 10000 ; dd = (i / 100) % 100 ; yy = i % 100 ;
		break ;
	   case V4LEX_TablePT_DDMMMYY:
		for(;*fp != UCEOS;fp++) { if (*fp != UClit(' ')) break ; } ;
		DN(dd) if (dd == 0) goto badterm ; if (*(fp++) != '-') { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUExpMinus",input) ; return((double)VCAL_BadVal) ; } ;
		for(i=0;i<(UCsizeof(month))-1;i++)
		 { if (*fp == UClit('-')) break ; if (*fp == UCEOS) break ; month[i] = UCTOUPPER(*fp) ; fp++ ;
		 } ; month[i] = UCEOS ;
		if (*(fp++) != UClit('-')) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUExpSl",input) ; return((double)VCAL_BadVal) ; } ;
		DN(yy) if (!(*fp == ('\0') || *fp == UClit(':') || *fp == UClit(' '))) goto badterm ;
		cli = gpi->ci->cli ; for(curx=0;curx<cli->Count;curx++) { if (cli->CLI[curx].Language == gpi->ci->Cntry[gpi->ci->CurX].Language && cli->CLI[curx].Calendar == VCAL_CalType_Gregorian) break ; } ;
		if (curx >= cli->Count) curx = 0 ;
		for (mm=1;mm<=12;mm++)
		 { 
		   if (UCstrcmpIC(cli->CLI[curx].SMonths[mm-1],month) == 0) goto m4_ok ;
//		   for(i=0;;i++)
//		    { if (toupper(cli->CLI[curx].SMonths[mm-1][i]) != month[i]) break ;
//		      if (month[i] == 0) goto m4_ok ;
//		    }
		 } ;
		v_Msg(ctx,ctx->ErrorMsgAux,"ParseUBadMonth",month,input) ; return((double)VCAL_BadVal) ;
m4_ok:		break ;
	   case V4LEX_TablePT_Default:
	   default:
/*		Parse first part - if entire date is one big number then split on V4LEX_YTDOrder_xxx, otherwise look to see what format might be */
		p1 = fp ; DN(i)
		if(i > 9999 && (*fp == UCEOS || *fp == UClit(':') || *fp == UClit(' ')))
		 { switch (ymdOrder)
		    {
		      case V4LEX_YMDOrder_YMD:	yy = i / 10000 ; mm = (i / 100) % 100 ; dd = i % 100 ; break ;
		      case V4LEX_YMDOrder_MDY:	mm = i / 10000 ; dd = (i / 100) % 100 ; yy = i % 100 ; break ;
		      case V4LEX_YMDOrder_MYD:	mm = i / 10000 ; yy = (i / 100) % 100 ; dd = i % 100 ; break ;
		      case V4LEX_YMDOrder_DMY:	dd = i / 10000 ; mm = (i / 100) % 100 ; yy = i % 100 ; break ;
		      case V4LEX_YMDOrder_DYM:	dd = i / 10000 ; yy = (i / 100) % 100 ; mm = i % 100 ; break ;
		      case V4LEX_YMDOrder_YDM:	yy = i / 10000 ; dd = (i / 100) % 100 ; mm = i % 100 ; break ;
		    } ; break ;
		 } ;
/*		Parse into components and decide what we got */
		p2 = UCstrpbrk(p1,UClit(" ,-/.")) ; if (p2 == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUSyn",input,p1) ; return(VCAL_BadVal) ; } ;
		p2ds = *p2 ; *p2 = UCEOS ; p2++ ; p3 = UCstrpbrk(p2,UClit(" ,-/.")) ; if (p3 == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUSyn",input,p2) ; return(VCAL_BadVal) ; } ;
		p3ds = *p3 ; *p3 = UCEOS ; p3++ ;
		p4 = UCstrpbrk(p3,UClit(" :")) ; if (p4 != NULL) { p4ds = *p4 ; *p4 = UCEOS ; } ;
		switch (ymdOrder)
		 {
		   case V4LEX_YMDOrder_YMD:	yp = p1 ; mp = p2 ; dp = p3 ; break ;
		   case V4LEX_YMDOrder_MDY:	mp = p1 ; dp = p2 ; yp = p3 ; break ;
		   case V4LEX_YMDOrder_MYD:	mp = p1 ; yp = p2 ; dp = p3 ; break ;
		   case V4LEX_YMDOrder_DMY:	dp = p1 ; mp = p2 ; yp = p3 ; break ;
		   case V4LEX_YMDOrder_DYM:	dp = p1 ; yp = p2 ; mp = p3 ; break ;
		   case V4LEX_YMDOrder_YDM:	yp = p1 ; dp = p2 ; mp = p3 ; break ;
		 } ;
		fp = yp ; DN(yy) ; if (*fp != UCEOS) { p2[-1]=p2ds ; p3[-1]=p3ds ; if (p4 != NULL) *p4 = p4ds ; v_Msg(ctx,ctx->ErrorMsgAux,"ParseUSyn",input,yp) ; return(VCAL_BadVal) ; } ;
		fp = dp ; DN(dd) ; if (*fp != UCEOS) { p2[-1]=p2ds ; p3[-1]=p3ds ; if (p4 != NULL) *p4 = p4ds ; v_Msg(ctx,ctx->ErrorMsgAux,"ParseUSyn",input,dp) ; return(VCAL_BadVal) ; } ;
		fp = mp ; DN(mm) ;
		if (*fp != UCEOS)
		 { 
		   cli = gpi->ci->cli ; for(curx=0;curx<cli->Count;curx++) { if (cli->CLI[curx].Language == gpi->ci->Cntry[gpi->ci->CurX].Language && cli->CLI[curx].Calendar == VCAL_CalType_Gregorian) break ; } ;
		   if (curx >= cli->Count) curx = 0 ;
		   for (mm=1;mm<=12;mm++) { if (UCstrcmpIC(cli->CLI[curx].SMonths[mm-1],mp) == 0) break ; } ;
		   if (mm > 12) { p2[-1]=p2ds ; p3[-1]=p3ds ; v_Msg(ctx,ctx->ErrorMsgAux,"ParseUBadMonth",mp,input) ; return(VCAL_BadVal) ; } ;
		 } ;
		if (p4 == NULL) { fp = UClit("") ; } else { *p4 = p4ds ; fp = p4 ; } ;
		p2[-1]=p2ds ; p3[-1]=p3ds ; break ;
//		for(;*fp != UCEOS;fp++) { if (*fp != UClit(' ')) break ; } ;
//		DN(mm) if (mm == 0) goto badterm ; if (*(fp++) != '/') { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUExpSl",input) ; return((double)VCAL_BadVal) ; } ;
//		DN(dd) if (*(fp++) != UClit('/')) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUExpSl",input) ; return((double)VCAL_BadVal) ; } ;
//		DN(yy) if (!(*fp == UCEOS || *fp == UClit(' ') || *fp == UClit(':'))) goto badterm ;
//		break ;
	 } ;
	
	ci = gpi->ci ;
	li = ci->li ; curx = ci->li->CurX ;			/* VEH060412 - pull current language index */
	if (caltype == VCAL_CalType_UseDeflt) caltype = ci->Cntry[ci->CurX].Calendar ;

	switch (caltype)
	 {
	   default:
	   case VCAL_CalType_Gregorian:
		if (!VerifyGregorianYMD(yy,mm,dd,ctx->ErrorMsgAux)) return((double)VCAL_BadVal) ;
		d1 = FixedFromGregorian(yy,mm,dd) ; break ;
	   case VCAL_CalType_Julian:
		if (!VerifyGregorianYMD(yy,mm,dd,ctx->ErrorMsgAux)) return((double)VCAL_BadVal) ;
		d1 = FixedFromJulian(yy,mm,dd) ; break ;
	   case VCAL_CalType_Islamic:
		if (!VerifyIslamicYMD(yy,mm,dd,ctx->ErrorMsgAux)) return((double)VCAL_BadVal) ;
		d1 = FixedFromIslamic(yy,mm,dd) ; break ;
	   case VCAL_CalType_ISO:
		if (!VerifyISOYWD(yy,mm,dd,ctx->ErrorMsgAux)) return((double)VCAL_BadVal) ;
		d1 = FixedFromISO(yy,mm,dd) ; break ;
	   case VCAL_CalType_Hebrew:
		if (!VerifyHebrewYMD(yy,mm,dd,ctx->ErrorMsgAux)) return((double)VCAL_BadVal) ;
		d1 = FixedFromHebrew(yy,mm,dd) ; break ;
	   case VCAL_CalType_Chinese:
	   case VCAL_CalType_Hindu:
		v_Msg(ctx,ctx->ErrorMsgAux,"ParseUNYI") ; return((double)VCAL_BadVal) ;
	 } ;

	if (*fp == UClit(':') || *fp == UClit(' '))
	 { 
	   fp++ ; DN(hr) ; if (*fp != UClit(':')) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUExpColon",input) ; return((double)VCAL_BadVal) ; } ;
	   fp++ ; DN(min) ;// if (!(*fp == UClit(':') || *fp == UCEOS || *fp == UClit(' ') || *fp == UClit('-') || *fp == UClit('+'))) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUExpColon",input) ; return((double)VCAL_BadVal) ; } ;
	   if (*fp != UClit(':')) { sec = 0 ; } else { fp++ ; DN(sec) ; } ;
	   switch (*fp)			/* Got Time Zone spec ? */
	    { 
	      case '\0':
		break ;
	      case ' ':
	      case ':':
		fp++ ;			/* Advance over punctuation & continue */
	      default:
tz_lookup:
		for(i=0;i<sizeof tzabbr;i++,fp++) { tzabbr[i] = UCTOUPPER(*fp) ; if (*fp == '\0') break ; } ;
		tzabbr[(UCsizeof(tzabbr))-1] = '\0' ;
		for (i=0;i<V4LI_TZMax && li->LI[lix].TZ[i].tzAbbr[0]!='\0';i++)
		 { if (UCstrcmpIC(li->LI[lix].TZ[i].tzAbbr,tzabbr) == 0) break ; } ;
		if (i >= V4LI_TZMax || li->LI[lix].TZ[i].tzAbbr[0] == '\0')
		 { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUTZBad",input) ; return(VCAL_BadVal) ; } ;
		timezone = li->LI[lix].TZ[i].tzOffset ;
		break ;
	      case '+':
		timezone = UCstrtol(fp+1,&b,10) ; if (timezone < 0 || timezone > 12 || *b != '\0') goto badterm ;
		fp = b ; break ;
	      case '-':
		timezone = UCstrtol(fp,&b,10) ;			/* Got -hours or -xxx ? */
		if (b == fp) { fp++ ; goto tz_lookup ; } ;	/* Got -xxx - do lookup */
		fp = b ; if (timezone < -12 || timezone > 0 || *b != UCEOS) goto badterm ;
		break ;
	    } ;
	   if (*fp != UCEOS) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUTerm",input,"Calendar") ; return((double)VCAL_BadVal) ; } ;
	   if (hr < 0 || hr > 23 || min < 0 || min > 59 || sec < 0 || sec > 59)
	    { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUHMS",input) ; return((double)VCAL_BadVal) ; } ;
	   if (zuluoffset == (double)UNUSED) zuluoffset = -(double)mscu_minutes_west() / (24 * 60) ;
	   d2 = (double)(hr * (60*60) + min * 60 + sec) / (double)VCAL_SecsInDay ;
	   d2 -= (timezone != VCAL_TimeZone_Local ? (double)timezone / 24.0 : zuluoffset) ;
	   frac = modf(d2,&ipart) ;
	   if (frac == 0.0) d2 += VCAL_MidnightValue ;	/* Want some fractional value to indicate time */
	   d1 += d2 ;
	 } ;

	if (!futureOK)
	 { CALENDAR tcal ; setCALisNOW(tcal) ;
	   if (d1 > tcal) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIFuture") ; return(VCAL_BadVal) ; } ;
	 } ;
	return(d1) ;

badterm:
	if (v_IsNoneLiteral(input)) return(VCal_NullDate) ;
	v_Msg(ctx,ctx->ErrorMsgAux,"ParseUTerm",input,"Calendar2") ; return((double)VCAL_BadVal) ;
}

/*	v_ParseUDate - Parses text date to UDate & returns as value		*/
/*	Call: udate = v_ParseUDate(ctx,input,type,ymdOrder,futureOK)
	  where udate is returned udate (VCAL_BadVal if error (ctx->ErrorMsgAux updated with error)),
		ctx is context,
		input is input string (null terminated) ** NOTE: ASSUME THIS STRING MAY BE MODIFIED **,
		type is type of date specification - V4LEX_TablePT_xxx,
		ymdOrder is ordering of year-month-day - V4LEX_YMDOrder_xxx,
		futureOK is TRUE if future dates allowd, FALSE otherwise	*/

#define OKUDTERM (*fp == UCEOS || *fp == UClit('.') || *fp == UClit('+') || *fp == UClit('-'))

int v_ParseUDate(ctx,input,type,ymdOrder,futureOK)
  struct V4C__Context *ctx ;
  UCCHAR *input ;
  int type,ymdOrder ;
  LOGICAL futureOK ;
{
  struct V4LI_CalendarInfo *cli ;
  int curx,i,yy,mm,dd,sign,udate ; UCCHAR *fp,month[32] ;
  UCCHAR *p1,*p2,*p3, *yp, *mp, *dp, *endd ; UCCHAR p2ds,p3ds ;

	if (UCempty(input)) return(VCAL_UDate_None) ;		/* VEH131017 - Empty string same as date:none */
/*	fp = input with leading/trailing spaces trimmed away */
	fp = input ;
	for(;*fp != UCEOS;fp++) { if (*fp != UClit(' ')) break ; } ;
	for(i=UCstrlen(fp)-1;i>=0;i--) { if (fp[i] != UClit(' ')) break ; fp[i] = UCEOS ; } ;
	if (vuc_IsAlpha(*input))
	 { PNTTYPE pntType ;
	   if (v_IsNoneLiteral(fp)) return(VCAL_UDate_None) ;
	   if (v_ParseRelativeUDate(ctx,fp,&udate,0,futureOK,&pntType,NULL) != NULL)
	    { if (pntType == V4DPI_PntType_UDate) return(udate) ;
	      v_Msg(ctx,ctx->ErrorMsgAux,"ParseDateRelDT",input,pntType,V4DPI_PntType_UDate) ;	      
	      return(VCAL_BadVal) ;
	    } ;
	 } ;
	
#undef DN
#define DN(var) var=0 ; for(;;fp++) { if (*fp != ' ') break ; } ; for(;;fp++) { if (*fp >= UClit('0') && *fp <= UClit('9')) { var*=10 ; var+= *fp - '0' ; } else { break ; } ; } ;

	switch (*fp)
	 { case UClit('#'):	fp++ ; type = V4LEX_TablePT_Internal ; break ;
	   case UClit('+'):	fp++ ; DN(i) if (*fp != '\0') goto badterm ;
//				return(TIMEOFFSETDAY + (time(NULL)-(60*gpi->MinutesWest))/VCAL_SecsInDay + i) ;
				if (!futureOK) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIFuture") ; return(VCAL_BadVal) ; } ;
				return(valUDATEisNOW + i) ;
	   case UClit('-'):	fp++ ; DN(i) if (*fp != '\0') goto badterm ;
//				return(TIMEOFFSETDAY + (time(NULL)-(60*gpi->MinutesWest))/VCAL_SecsInDay - i) ;
				return(valUDATEisNOW - i) ;
	 } ;
	
	switch(type)
	 { case V4LEX_TablePT_Internal:
		if (*fp == UClit('-')) { sign = TRUE ; fp++ ; } else { sign= FALSE ; } ;
		DN(i) if (!OKUDTERM) goto badterm ; endd = fp ;
		if (i > VCAL_UDateMax || i < -100)
		 { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUDateInt",-100,i,VCAL_UDateMax) ; return(VCAL_BadVal) ; } ;
		return(sign ? -i : i) ;
	   case V4LEX_TablePT_YYMMDD:
		DN(i) if (!OKUDTERM) goto badterm ; endd = fp ;
		yy = i / 10000 ; mm = (i / 100) % 100 ; dd = i % 100 ;
		break ;
	   case V4LEX_TablePT_DDMMYY:
		DN(i) if (!OKUDTERM) goto badterm ; endd = fp ;
		dd = i / 10000 ; mm = (i / 100) % 100 ; yy = i % 100 ;
		break ;
	   case V4LEX_TablePT_MMDDYY:
		DN(i) if (!OKUDTERM) goto badterm ; endd = fp ;
		mm = i / 10000 ; dd = (i / 100) % 100 ; yy = i % 100 ;
		break ;
	   case V4LEX_TablePT_DDMMMYY:
		DN(dd) if (dd == 0) goto badterm ; if (*(fp++) != UClit('-')) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUExpMinus",input) ; return(VCAL_BadVal) ; } ;
		for(i=0;i<(UCsizeof(month))-1;i++)
		 { 
//		   if (*fp == UClit('-')) break ; if (!OKUDTERM) break ; month[i] = UCtoupper(*fp++) ;
		   if (!vuc_IsLetter(*fp)) break ; month[i] = *fp++ ;
		 } ; month[i] = UCEOS ;
		if (*(fp++) != UClit('-')) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUExpSl",input) ; return(VCAL_BadVal) ; } ;
		DN(yy) if (!OKUDTERM) goto badterm ; endd = fp ;
		cli = gpi->ci->cli ; for(curx=0;curx<cli->Count;curx++) { if (cli->CLI[curx].Language == gpi->ci->Cntry[gpi->ci->CurX].Language && cli->CLI[curx].Calendar == VCAL_CalType_Gregorian) break ; } ;
		if (curx >= cli->Count) curx = 0 ;
		for (mm=1;mm<=12;mm++)
		 { if (UCstrcmpIC(cli->CLI[curx].SMonths[mm-1],month) == 0) goto m4_ok ;
//		   for(i=0;;i++)
//		    { if (toupper(cli->CLI[curx].SMonths[mm-1][i]) != month[i]) break ;
//		      if (month[i] == 0) goto m4_ok ;
//		    }
		 } ;
		v_Msg(ctx,ctx->ErrorMsgAux,"ParseUBadMonth",month,input) ; return(VCAL_BadVal) ;
m4_ok:		break ;
	   case V4LEX_TablePT_Default:
	   default:
/*		Parse first part - if entire date is one big number then split on V4LEX_YTDOrder_xxx, otherwise look to see what format might be */
		p1 = fp ; DN(i)
		if(OKUDTERM && i > 10000)		/* Got a number - determine YMD order UNLESS > 18500000 then assume yyyymmdd */
		 { if (i > 18500000) ymdOrder = V4LEX_YMDOrder_YMD ;
		   switch (ymdOrder)
		    {
		      case V4LEX_YMDOrder_YMD:	yy = i / 10000 ; mm = (i / 100) % 100 ; dd = i % 100 ; break ;
		      case V4LEX_YMDOrder_MDY:	mm = i / 10000 ; dd = (i / 100) % 100 ; yy = i % 100 ; break ;
		      case V4LEX_YMDOrder_MYD:	mm = i / 10000 ; yy = (i / 100) % 100 ; dd = i % 100 ; break ;
		      case V4LEX_YMDOrder_DMY:	dd = i / 10000 ; mm = (i / 100) % 100 ; yy = i % 100 ; break ;
		      case V4LEX_YMDOrder_DYM:	dd = i / 10000 ; yy = (i / 100) % 100 ; mm = i % 100 ; break ;
		      case V4LEX_YMDOrder_YDM:	yy = i / 10000 ; dd = (i / 100) % 100 ; mm = i % 100 ; break ;
		    } ;
		   endd = fp ; break ;
		 } ;
/*		Parse into components and decide what we got */
		p2 = UCstrpbrk(p1,UClit(" ,-/.")) ; if (p2 == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUSyn",input,p1) ; return(VCAL_BadVal) ; } ;
		p2ds = *p2 ; *p2 = UCEOS ; p2++ ; p3 = UCstrpbrk(p2,UClit(" ,-/.")) ; if (p3 == NULL) { p2[-1] = p2ds ; v_Msg(ctx,ctx->ErrorMsgAux,"ParseUSyn",input,p2) ; return(VCAL_BadVal) ; } ;
		p3ds = *p3 ; *p3 = UCEOS ; p3++ ;
		fp = p1 ; DN(i) ;
		if (i > 1000)				/* If p1 > 1000 then most likely a year - assume y-m-d */
		 ymdOrder = V4LEX_YMDOrder_YMD ;
		switch (ymdOrder)
		 {
		   case V4LEX_YMDOrder_YMD:	yp = p1 ; mp = p2 ; dp = p3 ; break ;
		   case V4LEX_YMDOrder_MDY:	mp = p1 ; dp = p2 ; yp = p3 ; break ;
		   case V4LEX_YMDOrder_MYD:	mp = p1 ; yp = p2 ; dp = p3 ; break ;
		   case V4LEX_YMDOrder_DMY:	dp = p1 ; mp = p2 ; yp = p3 ; break ;
		   case V4LEX_YMDOrder_DYM:	dp = p1 ; yp = p2 ; mp = p3 ; break ;
		   case V4LEX_YMDOrder_YDM:	yp = p1 ; dp = p2 ; mp = p3 ; break ;
		 } ;
		fp = yp ; DN(yy) ; if (!OKUDTERM) { p2[-1]=p2ds ; p3[-1]=p3ds ; v_Msg(ctx,ctx->ErrorMsgAux,"ParseUSyn",input,yp) ; return(VCAL_BadVal) ; } ;
		 if (yp == p3) endd = fp ;	/* This is to keep track of terminating character */
		fp = dp ; DN(dd) ; if (!OKUDTERM) { p2[-1]=p2ds ; p3[-1]=p3ds ; v_Msg(ctx,ctx->ErrorMsgAux,"ParseUSyn",input,dp) ; return(VCAL_BadVal) ; } ;
		 if (dp == p3) endd = fp ;
		fp = mp ; DN(mm) ;
		 if (mp == p3) endd = fp ;
		if (!OKUDTERM)
		 { 
		   cli = gpi->ci->cli ; for(curx=0;curx<cli->Count;curx++) { if (cli->CLI[curx].Language == gpi->ci->Cntry[gpi->ci->CurX].Language && cli->CLI[curx].Calendar == VCAL_CalType_Gregorian) break ; } ;
		   if (curx >= cli->Count) curx = 0 ;
		   for (mm=1;mm<=12;mm++) { if (UCstrcmpIC(cli->CLI[curx].SMonths[mm-1],mp) == 0) break ; } ;
		   if (mm > 12) { p2[-1]=p2ds ; p3[-1]=p3ds ; v_Msg(ctx,ctx->ErrorMsgAux,"ParseUBadMonth",mp,input) ; return(VCAL_BadVal) ; } ;
		 } ;
		p2[-1]=p2ds ; p3[-1]=p3ds ; break ;
		
//		DN(mm) if (mm == 0) goto badterm ; if (*(fp++) != UClit('/')) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUExpSl",input) ; return(VCAL_BadVal) ; } ;
//		DN(dd) if (*(fp++) != UClit('/')) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUExpSl",input) ; return(VCAL_BadVal) ; } ;
//		DN(yy) if (!(*fp == UCEOS || *fp == UClit(' '))) goto badterm ;
//		break ;
	 } ;
	
	udate = vcal_UDateFromYMD(!futureOK,yy,mm,dd,ctx->ErrorMsgAux) ;
/*	endd points to terminating date character - if not UCEOS then parse relative date off of the one we have so far */
	if (*endd != UCEOS)
	 { PNTTYPE pntType ;
	   if (v_CalCalc(ctx,endd,udate,V4DPI_PntType_UDate,&udate,&pntType,FALSE) == NULL) return(VCAL_BadVal) ;
	 } ;
	if (!futureOK && udate != VCAL_BadVal)
	 { if (udate > valUDATEisNOW)
	    {v_Msg(ctx,ctx->ErrorMsgAux,"DPIFuture") ; return(VCAL_BadVal) ; } ;
	 } ;
	return(udate) ;
badterm:
//	if (v_IsNoneLiteral(input)) return(0) ;
	v_Msg(ctx,ctx->ErrorMsgAux,"ParseUTerm",input,"UDate") ; return(VCAL_BadVal) ;
}

/*	v_CalCalc - Performs 'relative' date calculations				*/
/*	Call: ptr = v_CalCalc( ctx , relExp , dateVal , pntType , newVal , newPntType, okDifPT)
	  where ptr is pointer within relExp where parsing stopped, NULL if error,
		ctx is context,
		relExp is UCCHAR expression to parse & eval,
		dateValOrig is current date value,
		pntTypeOrig is current point type (UDATE, UMONTH, UYEAR),
		newVal is pointer to integer to be updated with new value,
		newPntType is new point type,
		okDifPT is TRUE if ok for differing pntType, FALSE if must be same	*/

UCCHAR *v_CalCalc(ctx,relExp,dateValOrig,pntTypeOrig,newVal,newPntType,okDifPT)
  struct V4C__Context *ctx ;
  UCCHAR *relExp ;
  int dateValOrig ;
  PNTTYPE pntTypeOrig ;
  int *newVal ;
  PNTTYPE *newPntType ;
{ struct V4CI__CountryInfo *ci ;
  struct V4LI_CalendarInfo *cli ;
  PNTTYPE pntType ; int dateVal ; ETYPE delim,calendar ; INDEX curx ; COUNTER loop ; UCCHAR *ep ;
  int intTkn ; UCCHAR ucTkn[256] ; LOGICAL isNum, isHistory ;

#define CCTOKEN \
 { UCCHAR *ep1 ; INDEX i ; \
   intTkn = UCstrtol(ep,&ep1,10) ; \
   if (ep != ep1) { ep = ep1 ; isNum = TRUE ; } \
    else { for(i=0;i<UCsizeof(ucTkn)&&vuc_IsAlphaNum(ep[i]);i++) { ucTkn[i] = ep[i] ; } ; \
	   ep = &ep[i] ; ucTkn[i] = UCEOS ; isNum = FALSE ; \
	 } ; \
 }

	isHistory = FALSE ;
/*	Link up to calendar information */
	ci = gpi->ci ; cli = ci->cli ; calendar = ci->Cntry[ci->CurX].Calendar ;
	for(curx=0;curx<cli->Count;curx++) { if (cli->CLI[curx].Language == ci->li->CurX && cli->CLI[curx].Calendar == calendar) break ; } ;
	if (curx >= cli->Count) curx = 0 ;
	ep = relExp ; pntType = pntTypeOrig ; dateVal = dateValOrig ;
	for(loop=0;;loop++)
	 {
	   delim = *ep ; ep++ ;
	   switch (delim)
	    { default:
		if (loop <= 0) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseDateRel",1,relExp) ; return(NULL) ; } ;
		if (!okDifPT) { if (pntType != pntTypeOrig) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseDateRelDT",relExp,pntType,pntTypeOrig) ; return(NULL) ; } ; } ;
		*newVal = dateVal ; *newPntType = pntType ;
		return(ep - 1) ;
	      case UClit('.'):
	      case UClit('+'):
	      case UClit('-'):
		CCTOKEN ;
		if (UCstrlen(ucTkn) == 0 && !isNum) return(NULL) ;	/* If no token after delimiter (ex: have "..xxxx") then error */
		break ;
	    } ;
	   switch (pntType)
	    { default:		v_Msg(ctx,ctx->ErrorMsgAux,"ParseDateRel",2,relExp) ; return(NULL) ;
	      case V4DPI_PntType_UDate:
		if (isNum)
		 { switch (delim)
		    { case UClit('.'):	dateVal += intTkn ; break ;
		      case UClit('+'):	dateVal += intTkn ; break ;
		      case UClit('-'):	dateVal -= intTkn ; break ;
		    } ;  break ;
		 } ;
		switch (v4im_GetUCStrToEnumVal(ucTkn,0))
		 { default:
/*		    Before we quit as error, see if UCkeyword is one of the days of the week */
		    { INDEX i,t,dow ; ETYPE mode ;
		      for(i=0;i<7;i++)
		       { if (UCstrcmpIC(ci->cli->CLI[curx].SDaysOfWeek[i],ucTkn) == 0) break ; } ;
/*		      Don't have day-of-week, see if we have month of year */
		      if (i >= 7)
		       { int mm,yyyy, yyyymmdd = mscu_udate_to_yyyymmdd(dateVal) ;
		         yyyy = yyyymmdd / 10000 ; mm = (yyyymmdd % 10000) / 100 ;
			 if ((dateVal = v_CalCalcMonth(yyyy,mm,delim,ucTkn,ci,curx)) == VCAL_BadVal)
		          { v_Msg(ctx,ctx->ErrorMsgAux,"ParseDateRelKW",3,ucTkn,relExp) ; return(NULL) ; } ;
		         pntType = V4DPI_PntType_UMonth ; break ;
		       } ;
/*		      Have day-of-week (i) */
		      switch (delim)
		       { case UClit('.'):	dow = i + 1 ; mode = 0 ; break ;	/* Want nearest */
			 case UClit('+'):	dow = i + 1 ; mode = 1 ; break ;	/* Want > date */
			 case UClit('-'):	dow = -(i + 1) ; mode = -1 ; break ;	/* Want <= date */
		       } ;
		      if (dow < 0 || mode != 0)
		       { if (dow < 0) { dow = -dow ; dateVal -= 6 ; } ;
			 t = ((dateVal+1)%7)+1 ;			/* t = current day of week */
			 dow -= t ; if (dow < 0) dow += 7 ;
			 if (dow == 0 && mode == 1) dow += 7 ;		/* Want day > current date (not >=) */
		       } else
		       { t = ((dateVal+1)%7)+1 ; t = dow - t ; if (t < 0) t += 7 ;
			 if (t <= 3) { dow = t ; }
			 else { dateVal -= 6 ; t = ((dateVal+1)%7)+1 ; dow -= t ; if (dow < 0) dow += 7 ; } ;
		       } ;
		      dateVal += dow ;
		      break ;
		    } ; 
		    break ;
		   case DE(First):
		    { int yyyymmdd = mscu_udate_to_yyyymmdd(dateVal) ;
		      dateVal = vcal_UDateFromYMD(isHistory,yyyymmdd/10000,(yyyymmdd % 10000)/100,1,ctx->ErrorMsgAux) ;
		      if (dateVal == VCAL_BadVal) return(NULL) ;
		    } ;
		    break ;
		   case DE(Half):
		    { int dd, yyyymmdd = mscu_udate_to_yyyymmdd(dateVal) ;
		      dd = yyyymmdd % 100 ;
		      dateVal = vcal_UDateFromYMD(isHistory,yyyymmdd/10000,(yyyymmdd % 10000)/100,(dd >= 15 ? 15 : 1),ctx->ErrorMsgAux) ;
		      if (dateVal == VCAL_BadVal) return(NULL) ;
		    } ;
		    break ;
		   case DE(Last):
		    { int mm,yyyy, yyyymmdd = mscu_udate_to_yyyymmdd(dateVal) ;
		      yyyy = yyyymmdd / 10000 ; mm = (yyyymmdd % 10000) / 100 ;
		      mm += 1 ; if (mm > 12) { mm = 1 ; yyyy++ ; } ;
		      dateVal = vcal_UDateFromYMD(isHistory,yyyy,mm,1,ctx->ErrorMsgAux) ;
		      if (dateVal == VCAL_BadVal) return(NULL) ;
		      dateVal -= 1 ;
		    } ;
		    break ;
		   case DE(Month):
		    UDtoUMONTH(dateVal,dateVal) ; pntType = V4DPI_PntType_UMonth ; break ;
		   case DE(Qtr):
		   case DE(Quarter):
		    { int mm,yyyy,qtr, yyyymmdd = mscu_udate_to_yyyymmdd(dateVal) ;
		      yyyy = yyyymmdd / 10000 ; mm = (yyyymmdd % 10000) / 100 ; qtr = (mm + 2) /3 ;
		      dateVal = YYQQtoUQTR(yyyy,qtr) ; pntType = V4DPI_PntType_UQuarter ;
		    } ; break ;
		   case DE(Year):
		    dateVal = mscu_udate_to_yyyymmdd(dateVal) / 10000 ; pntType = V4DPI_PntType_UYear ; break ;
		 } ;
		break ;
	      case V4DPI_PntType_UMonth:
		if (isNum)
		 { switch (delim)
		    { case UClit('.'):
			dateVal = vcal_UDateFromYMD(isHistory,UMONTHtoUYEAR(dateVal),UMONTHtoMONTH(dateVal),intTkn,ctx->ErrorMsgAux) ;
			if (dateVal == VCAL_BadVal) return(NULL) ;
			pntType = V4DPI_PntType_UDate ; break ;
		      case UClit('+'):	dateVal += intTkn ; break ;
		      case UClit('-'):	dateVal -= intTkn ; break ;
		    } ;
		   break ;
		 } ;
		switch (v4im_GetUCStrToEnumVal(ucTkn,0))
		 { default:
			{ int y,m ;
			  y = UMONTHtoUYEAR(dateVal) ; m = UMONTHtoMONTH(dateVal) ;
			  if ((dateVal = v_CalCalcMonth(y,m,delim,ucTkn,ci,curx)) == VCAL_BadVal)
		           { v_Msg(ctx,ctx->ErrorMsgAux,"ParseDateRelKW",5,ucTkn,relExp) ; return(NULL) ; } ;
			}
			break ;
		     case _Current:
			{ INDEX y,m,d ;
			  y = UMONTHtoUYEAR(dateVal) ; m = UMONTHtoMONTH(dateVal) ; d = mscu_udate_to_yyyymmdd(valUDATEisNOW)%100 ;
			  for(;;d--)
			   { dateVal = vcal_UDateFromYMD(isHistory,y,m,d,ctx->ErrorMsgAux) ;
			     if (dateVal != VCAL_BadVal) break ;
			     if (d <= 28) return(NULL) ;
			   } ;
			}
			pntType = V4DPI_PntType_UDate ; break ;
		     case _First:
			dateVal = vcal_UDateFromYMD(isHistory,UMONTHtoUYEAR(dateVal),UMONTHtoMONTH(dateVal),1,ctx->ErrorMsgAux) ;
			if (dateVal == VCAL_BadVal) return(NULL) ;
			pntType = V4DPI_PntType_UDate ; break ;
		     case _Half:
			{ int y,m ;
			  y = UMONTHtoUYEAR(dateVal) ; m = UMONTHtoMONTH(dateVal) ;
			  dateVal = YYMMtoUMONTH(y,(m >= 7 ? 7 : 1)) ;
			}
			break ;
		     case _Last:
			{ int y,m ;
			  y = UMONTHtoUYEAR(dateVal) ; m = UMONTHtoMONTH(dateVal) ;
			  m += 1 ; if (m > 12) { m = 1 ; y += 1 ; } ;
			  dateVal = vcal_UDateFromYMD(isHistory,y,m,1,ctx->ErrorMsgAux) ;
			  if (dateVal == VCAL_BadVal) return(NULL) ;
			  dateVal -= 1 ; pntType = V4DPI_PntType_UDate ;
			}
			break ;
		   case DE(Quarter):
		   case DE(Qtr):
			{ int y,m,q ;
			  y = UMONTHtoUYEAR(dateVal) ; m = UMONTHtoMONTH(dateVal) ; q = (m + 2) /3 ;
			  dateVal = YYQQtoUQTR(y,q) ; pntType = V4DPI_PntType_UQuarter ;
			} ; break ;
		     case _Year:
			dateVal = UMONTHtoUYEAR(dateVal) ; pntType = V4DPI_PntType_UYear ; break ;
		 } ;
		break ;
	      case V4DPI_PntType_UQuarter:
		if (isNum)
		 { switch (delim)
		    { case UClit('.'):
			if (intTkn < 1 || intTkn > 4) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUDateInt",1,intTkn,4) ; return(NULL) ; } ;
			dateVal = YYQQtoUQTR(UQTRtoUYEAR(dateVal),intTkn) ; break ;
		      case UClit('+'):	dateVal += intTkn ; break ;
		      case UClit('-'):	dateVal -= intTkn ; break ;
		    } ;
		   break ;
		 } ;
		switch (v4im_GetUCStrToEnumVal(ucTkn,0))
		 { default:	v_Msg(ctx,ctx->ErrorMsgAux,"ParseDateRelKW",5,ucTkn,relExp) ; return(NULL) ;
		     case _First:
			{ dateVal = YYMMtoUMONTH(UQTRtoUYEAR(dateVal),(UQTRtoQTR(dateVal)*3) - 2) ; pntType = V4DPI_PntType_UMonth ;
			} ; break ;
		     case _Last:
			{ dateVal = YYMMtoUMONTH(UQTRtoUYEAR(dateVal),(UQTRtoQTR(dateVal)*3)) ; pntType = V4DPI_PntType_UMonth ;
			} ; break ;
		     case _Year:
			dateVal = UQTRtoUYEAR(dateVal) ; pntType = V4DPI_PntType_UYear ; break ;
		 } ;
		break ;
	      case V4DPI_PntType_UYear:
		if (isNum)
		 { switch (delim)
		    { case UClit('.'):
			if (intTkn < 1 || intTkn > 12) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUDate",UMONTHtoUYEAR(dateVal),intTkn,0) ; return(NULL) ; } ;
			dateVal = YYMMtoUMONTH(dateVal,intTkn) ;
			pntType = V4DPI_PntType_UMonth ; break ;
		      case UClit('+'):	dateVal += intTkn ; break ;
		      case UClit('-'):	dateVal -= intTkn ; break ;
		    } ;
		   break ;
		 } ;
		switch (v4im_GetUCStrToEnumVal(ucTkn,0))
		 { default:	v_Msg(ctx,ctx->ErrorMsgAux,"ParseDateRelKW",7,ucTkn,relExp) ; return(NULL) ;
		     case _Current:
			dateVal = YYMMtoUMONTH(dateVal,(mscu_udate_to_yyyymmdd(valUDATEisNOW) % 10000)/100) ;
			pntType = V4DPI_PntType_UMonth ; break ;
		     case _First:
			dateVal = YYMMtoUMONTH(dateVal,1) ;
			pntType = V4DPI_PntType_UMonth ; break ;
		     case _Last:
			dateVal = YYMMtoUMONTH(dateVal,12) ;
			pntType = V4DPI_PntType_UMonth ; break ;
		     case _Q1:
			dateVal = YYQQtoUQTR(dateVal,1) ; pntType = V4DPI_PntType_UQuarter ; break ;
		     case _Q2:
			dateVal = YYQQtoUQTR(dateVal,2) ; pntType = V4DPI_PntType_UQuarter ; break ;
		     case _Q3:
			dateVal = YYQQtoUQTR(dateVal,3) ; pntType = V4DPI_PntType_UQuarter ; break ;
		     case _Q4:
			dateVal = YYQQtoUQTR(dateVal,4) ; pntType = V4DPI_PntType_UQuarter ; break ;
		 } ;
		break ;
	    } ;
	 } ;

}

int v_CalCalcMonth(yyyy,mm,delim,tkn,ci,curx)
  int yyyy,mm ;			/* Current month number */
  UCCHAR delim ;
  UCCHAR *tkn ;
  struct V4CI__CountryInfo *ci ;
  INDEX curx ;
{ int i, gf, gb ;

	for(i=0;i<24;i++)
	 { if (UCstrcmpIC(ci->cli->CLI[curx].SMonths[i],tkn) == 0) break ; } ;
	if (i >= 24) return(VCAL_BadVal) ;
	i++ ;
	switch (delim)
	 { case UClit('.'):	/* Want nearest */
	 	if (mm == i) break ;
		if (mm < i)
		 { gf = i - mm ; gb = mm - (i - 12) ; if (gb < gf) yyyy-- ;
		 } else
		 { gf = 12 - mm + i ; gb = mm - i ; if (gf < gb) yyyy++ ;
		 } ;
		break ;
	   case UClit('+'):	/* Want > date */
	   	if (mm >= i) yyyy++ ; break ;
	   case UClit('-'):	/* Want <= date */
		if (mm < i) yyyy-- ; break ;
	 } ;
	return(YYMMtoUMONTH(yyyy,i)) ;
}


/*	v_ParseRelativeUDate - Checks for relative dates ("YESTERDAY", "TODAY", "TOMORROW")				*/
/*	Call: updptr = v_ParseRelativeUDate( ctx , input , resdate , minabbrew , futureOK , pntType , vreld )
	  where updptr is input point updated to first byte past relative portion, NULL if no match,
		ctx is context,
		input is pointer to text to check,
		resdate is pointer to (int)udate to be updated with appropriate UDate (only if updptr is nonNULL),
		minabbrev is minimum number of characters allowed for abbreviations (0 or UNUSED for exact match),
		futureOK is TRUE if future dates OK, false otherwise,
		vreld, if not NULL, is updated with VRELD_xxx indicating initial keyword match				*/

UCCHAR *v_ParseRelativeUDate(ctx,input,resdate,minabbrev,futureOK,pntType,vreld)
  struct V4C__Context *ctx ;
  UCCHAR *input ;
  int *resdate ;
  int minabbrev ;
  LOGICAL futureOK ;
  PNTTYPE *pntType ;
  ETYPE *vreld ;
{ 
  int i,ilen ; UCCHAR ucbuf[64] ;

	for(i=0;i<UCsizeof(ucbuf);i++) { ucbuf[i] = UCTOUPPER(input[i]) ; if (!vuc_IsAlpha(ucbuf[i])) break ; } ; ucbuf[i] = '\0' ;
	ilen = UCstrlen(ucbuf) ;
	*pntType = V4DPI_PntType_UDate ;	/* Assume we are going to return UDate (may be altered below by call to v_CalCalc()) */
	if (minabbrev > 0)
	 { if (ilen < minabbrev) return(NULL) ;	/* Input not long enough to match */
	   if (UCstrncmpIC(ucbuf,gpi->ci->li->LI[gpi->ci->li->CurX].Yesterday,ilen) == 0)
	    { 
//	      *resdate = TIMEOFFSETDAY + (time(NULL)-(60*gpi->MinutesWest))/VCAL_SecsInDay - 1 ;
	      *resdate = valUDATEisNOW - 1 ;
	      if (vreld != NULL) *vreld = VRELD_Yesterday ;
	      goto relexp ;
	    } ;
	   if ((i=UCstrncmpIC(ucbuf,gpi->ci->li->LI[gpi->ci->li->CurX].Today,ilen)) == 0)
	    { 
//	      *resdate = TIMEOFFSETDAY + (time(NULL)-(60*gpi->MinutesWest))/VCAL_SecsInDay ;
	      *resdate = valUDATEisNOW ;
	      if (vreld != NULL) *vreld = VRELD_Today ;
	      goto relexp ;
	    } ;
	   if ((i=UCstrncmpIC(ucbuf,gpi->ci->li->LI[gpi->ci->li->CurX].NowUC,ilen)) == 0)
	    { *resdate = valUDATEisNOW ;
	      if (vreld != NULL) *vreld = VRELD_Current ;
	      goto relexp ;
	    } ;
	   if (UCstrncmpIC(ucbuf,gpi->ci->li->LI[gpi->ci->li->CurX].Tomorrow,ilen) == 0)
	    { 
	      if (!futureOK) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIFuture") ; return(NULL) ; } ;
//	      *resdate = TIMEOFFSETDAY + (time(NULL)-(60*gpi->MinutesWest))/VCAL_SecsInDay + 1 ;
	      *resdate = valUDATEisNOW + 1 ;
	      if (vreld != NULL) *vreld = VRELD_Tomorrow ;
	      goto relexp ;
	    } ;
	 }
/*	Must have exact match */
	if (UCstrcmpIC(ucbuf,gpi->ci->li->LI[gpi->ci->li->CurX].Yesterday) == 0)
	 { 
//	   *resdate = TIMEOFFSETDAY + (time(NULL)-(60*gpi->MinutesWest))/VCAL_SecsInDay - 1 ;
	   *resdate = valUDATEisNOW - 1 ;
	   if (vreld != NULL) *vreld = VRELD_Yesterday ;
	   goto relexp ;
	 } ;
	if (UCstrcmpIC(ucbuf,gpi->ci->li->LI[gpi->ci->li->CurX].Today) == 0)
	 { 
//	   *resdate = TIMEOFFSETDAY + (time(NULL)-(60*gpi->MinutesWest))/VCAL_SecsInDay ;
	   *resdate = valUDATEisNOW ;
	   if (vreld != NULL) *vreld = VRELD_Today ;
	   goto relexp ;
	 } ;
	if ((i=UCstrcmpIC(ucbuf,gpi->ci->li->LI[gpi->ci->li->CurX].NowUC)) == 0)
	 { *resdate = valUDATEisNOW ;
	   if (vreld != NULL) *vreld = VRELD_Current ;
	   goto relexp ;
	 } ;
	if (UCstrcmpIC(ucbuf,gpi->ci->li->LI[gpi->ci->li->CurX].Tomorrow) == 0)
	 { 
	   if (!futureOK) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIFuture") ; return(NULL) ; } ;
//	   *resdate = TIMEOFFSETDAY + (time(NULL)-(60*gpi->MinutesWest))/VCAL_SecsInDay + 1 ;
	   *resdate = valUDATEisNOW + 1 ;
	   if (vreld != NULL) *vreld = VRELD_Tomorrow ;
	   goto relexp ;
	 } ;
	return(NULL) ;			/* Nothing matched - return failure */
relexp:
	if (input[ilen] == UClit('.') || input[ilen] == UClit('+') || input[ilen] == UClit('-'))
	 { if (vreld != NULL ? *vreld == VRELD_Current : FALSE) *vreld = VRELD_CurrentPlus ;
	   return(v_CalCalc(ctx,&input[ilen],*resdate,V4DPI_PntType_UDate,resdate,pntType,TRUE)) ;
	 } ;
	return(&input[ilen]) ;
}

/*	v_ParseUDT - Parses text date to Date-Time & returns as value		*/
/*	Call: udate = v_ParseUDT(ctx,input,ytdOrder,type,minabbr,relflag,futureOK)
	  where udt is returned udate (VCAL_BadVal if error (ctx->ErrorMsgAux updated with error)),
		ctx is context,
		input is input string (null terminated),
		ymdOrder is YMD ordering (see V4LEX_YMDOrder_xxx),
		type is type of date specification - V4LEX_TablePT_xxx,
		minabbr is minimum number of characters to match on relative date (today,tomorrow,yesterday), 0 for exact match, UNUSED to disable,
		relflag, if not NULL, is updated with 0=no relative, 1=relative only, 2=relative with time,
		futureOK is TRUE if future dates OK, false otherwise		*/

int v_ParseUDT(ctx,input,type,ymdOrder,minabbr,relflag,futureOK)
  struct V4C__Context *ctx ;
  UCCHAR *input ;
  int type,ymdOrder,minabbr,*relflag ;
  LOGICAL futureOK ;
{ 
  struct V4LI_CalendarInfo *cli ;
  int curx,i,yy,mm,dd,date,num,udt,ut ; UCCHAR *fp,month[32],ucbuf[UCsizeof(gpi->ci->li->LI[0].Hours)] ;
  UCCHAR *p1,*p2,*p3,*p4, *yp, *mp, *dp ; UCCHAR p2ds,p3ds,p4ds ; ETYPE vreld ;

	if (UCempty(input)) return(VCAL_UDT_None) ;		/* VEH131017 - Empty string same as date:none */
	vreld = UNUSED ;
	if (minabbr != UNUSED && vuc_IsAlpha(*input))
	 { PNTTYPE pntType ;
	   if ((fp = v_ParseRelativeUDate(ctx,input,&date,minabbr,futureOK,&pntType,&vreld)) != NULL)
	    { if (pntType == V4DPI_PntType_UDate)
	       { if (relflag != NULL)
	          { switch (vreld)
	             { default:
			  *relflag = (*fp == UCEOS ? 1 : 2) ;	/* Got relative set flag if we also have time or not */
			  break ;
	               case VRELD_Current:
			  *relflag = 0 ;
			  return(valUDTisNOW) ;
	               case VRELD_CurrentPlus:
			  *relflag = 0 ;			/* If 'CURRENT' then that is specific date-time, don't do relative, fill in current time */
			  return(UDtoUDTcurrentTime(date)) ;
	             } ;
		  } ;
		 goto grab_time ;
	       } ;
	    } ;
	 } ;
	fp = input ; if (relflag != NULL) *relflag = 0 ;	/* No relative date */
	if (v_IsNoneLiteral(fp)) return(VCAL_UDT_None) ;
#undef DN
#define DN(var) var=0 ; for(;;fp++) { if (*fp != UClit(' ')) break ; } ; for(;;fp++) { if (*fp >= UClit('0') && *fp <= UClit('9')) { var*=10 ; var+= *fp - '0' ; } else { break ; } ; } ;
	switch (*fp)
	 { default:		break ;
//Handling of 'current' is now in above code
//				if (vuc_IsDigit(*fp)) break ;		/* If a digit then don't test for "now" */
//				for(i=0;i<sizeof ucbuf;i++) { ucbuf[i] = UCtoupper(fp[i]) ; if (ucbuf[i] == UCEOS) break ; } ;
//				if (UCstrcmp(ucbuf,gpi->ci->li->LI[gpi->ci->li->CurX].NowUC) == 0)
//				 return(valUDTisNOW) ;
				break ;
	   case UClit('#'):	fp++ ; type = V4LEX_TablePT_Internal ; break ;
	   case UClit('+'):	fp++ ; DN(num)
				if (*fp == UCEOS || *fp == UClit(' ') || *fp == UClit(':')) 
				 { date = valUDATEisNOW + num ; goto grab_time ; } ;
				if (!futureOK) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIFuture") ; return(VCAL_BadVal) ; } ;
//				udt = time(NULL) - (60*gpi->MinutesWest) - TIMEOFFSETSEC ;
				udt = valUDTisNOW ;
				for(i=0;i<UCsizeof(ucbuf);i++) { ucbuf[i] = toupper(fp[i]) ; if (ucbuf[i] == '\0') break ; } ;
				if (UCstrncmp(ucbuf,gpi->ci->li->LI[gpi->ci->li->CurX].HoursUC,i) == 0) { return(udt + num * 60 * 60) ; }
				 else if (UCstrncmp(ucbuf,gpi->ci->li->LI[gpi->ci->li->CurX].MinutesUC,i) == 0) { return(udt + num * 60) ; }
				 else if (UCstrncmp(ucbuf,gpi->ci->li->LI[gpi->ci->li->CurX].SecondsUC,i) == 0) { return(udt + num) ; }
				 else goto badterm ;
	   case UClit('-'):	fp++ ; DN(num)
				if (*fp == UCEOS || *fp == UClit(' ') || *fp == UClit(':')) 
				 { date = valUDATEisNOW - num ; goto grab_time ; } ;
				udt = valUDTisNOW ;
				for(i=0;i<UCsizeof(ucbuf);i++) { ucbuf[i] = toupper(fp[i]) ; if (ucbuf[i] == '\0') break ; } ;
				if (UCstrncmp(ucbuf,gpi->ci->li->LI[gpi->ci->li->CurX].HoursUC,i) == 0) { return(udt - num * 60 * 60) ; }
				 else if (UCstrncmp(ucbuf,gpi->ci->li->LI[gpi->ci->li->CurX].MinutesUC,i) == 0) { return(udt - num * 60) ; }
				 else if (UCstrncmp(ucbuf,gpi->ci->li->LI[gpi->ci->li->CurX].SecondsUC,i) == 0) { return(udt - num) ; }
				 else goto badterm ;
	 } ;
	switch(type)
	 { case V4LEX_TablePT_Internal:
		DN(i) if (!(*fp == UCEOS || *fp == UClit(' '))) goto badterm ;
		return(i) ;
	   case V4LEX_TablePT_YYMMDD:
		DN(i) if (!(*fp == UCEOS || *fp == UClit(':') || *fp == UClit(' ') || OKUDTERM)) goto badterm ;
		yy = i / 10000 ; mm = (i / 100) % 100 ; dd = i % 100 ;
		break ;
	   case V4LEX_TablePT_DDMMYY:
		DN(i) if (!(*fp == UCEOS || *fp == UClit(':') || *fp == UClit(' ') || OKUDTERM)) goto badterm ;
		dd = i / 10000 ; mm = (i / 100) % 100 ; yy = i % 100 ;
		break ;
	   case V4LEX_TablePT_MMDDYY:
		DN(i) if (!*fp == UCEOS || (*fp == UClit(':') || *fp == UClit(' ') || OKUDTERM)) goto badterm ;
		mm = i / 10000 ; dd = (i / 100) % 100 ; yy = i % 100 ;
		break ;
	   case V4LEX_TablePT_DDMMMYY:
		for(;*fp != UCEOS;fp++) { if (*fp != UClit(' ')) break ; } ;
		DN(dd) if (dd == 0) goto badterm ; if (*(fp++) != UClit('-')) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUExpMinus",input) ; return(VCAL_BadVal) ; } ;
		for(i=0;i<(UCsizeof(month))-1;i++)
		 { if (*fp == UClit('-')) break ; if (*fp == UCEOS) break ; month[i] = *fp++ ;
		 } ; month[i] = '\0' ;
		if (*(fp++) != UClit('-')) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUExpMinus",input) ; return(VCAL_BadVal) ; } ;
		DN(yy) if (!(*fp == UCEOS || *fp == UClit(':') || *fp == UClit(' ') || OKUDTERM)) goto badterm ;
		cli = gpi->ci->cli ; for(curx=0;curx<cli->Count;curx++) { if (cli->CLI[curx].Language == gpi->ci->Cntry[gpi->ci->CurX].Language && cli->CLI[curx].Calendar == VCAL_CalType_Gregorian) break ; } ;
		if (curx >= cli->Count) curx = 0 ;
		for (mm=1;mm<=12;mm++)
		 { if (UCstrcmpIC(cli->CLI[curx].SMonths[mm-1],month) == 0) goto m1_ok ;
//		   for(i=0;;i++)
//		    { if (toupper(cli->CLI[curx].SMonths[mm-1][i]) != month[i]) break ;
//		      if (month[i] == 0) goto m1_ok ;
//		    }
		 } ;
m1_ok:
		break ;
	   default:
/*		Parse first part - if entire date is one big number then split on V4LEX_YTDOrder_xxx, otherwise look to see what format might be */
		p1 = fp ; DN(i)
		if(*fp == UCEOS || *fp == UClit(':') || *fp == UClit(' '))
		 { switch (ymdOrder)
		    {
		      case V4LEX_YMDOrder_YMD:	yy = i / 10000 ; mm = (i / 100) % 100 ; dd = i % 100 ; break ;
		      case V4LEX_YMDOrder_MDY:	mm = i / 10000 ; dd = (i / 100) % 100 ; yy = i % 100 ; break ;
		      case V4LEX_YMDOrder_MYD:	mm = i / 10000 ; yy = (i / 100) % 100 ; dd = i % 100 ; break ;
		      case V4LEX_YMDOrder_DMY:	dd = i / 10000 ; mm = (i / 100) % 100 ; yy = i % 100 ; break ;
		      case V4LEX_YMDOrder_DYM:	dd = i / 10000 ; yy = (i / 100) % 100 ; mm = i % 100 ; break ;
		      case V4LEX_YMDOrder_YDM:	yy = i / 10000 ; dd = (i / 100) % 100 ; mm = i % 100 ; break ;
		    } ; break ;
		 } ;
/*		Parse into components and decide what we got */
		p2 = UCstrpbrk(p1,UClit(" ,-/.")) ; if (p2 == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUSyn",input,p1) ; return(VCAL_BadVal) ; } ;
		p2ds = *p2 ; *p2 = UCEOS ; p2++ ; p3 = UCstrpbrk(p2,UClit(" ,-/.")) ; if (p3 == NULL) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUSyn",input,p2) ; return(VCAL_BadVal) ; } ;
		p3ds = *p3 ; *p3 = UCEOS ; p3++ ;
		p4 = UCstrpbrk(p3,UClit(" :.+-Tt ")) ; if (p4 != NULL) { p4ds = *p4 ; *p4 = UCEOS ; } ;
		switch (ymdOrder)
		 {
		   case V4LEX_YMDOrder_YMD:	yp = p1 ; mp = p2 ; dp = p3 ; break ;
		   case V4LEX_YMDOrder_MDY:	mp = p1 ; dp = p2 ; yp = p3 ; break ;
		   case V4LEX_YMDOrder_MYD:	mp = p1 ; yp = p2 ; dp = p3 ; break ;
		   case V4LEX_YMDOrder_DMY:	dp = p1 ; mp = p2 ; yp = p3 ; break ;
		   case V4LEX_YMDOrder_DYM:	dp = p1 ; yp = p2 ; mp = p3 ; break ;
		   case V4LEX_YMDOrder_YDM:	yp = p1 ; dp = p2 ; mp = p3 ; break ;
		 } ;
		fp = yp ; DN(yy) ; if (*fp != UCEOS) { p2[-1]=p2ds ; p3[-1]=p3ds ; if (p4 != NULL) *p4 = p4ds ; v_Msg(ctx,ctx->ErrorMsgAux,"ParseUSyn",input,yp) ; return(VCAL_BadVal) ; } ;
		fp = dp ; DN(dd) ; if (*fp != UCEOS) { p2[-1]=p2ds ; p3[-1]=p3ds ; if (p4 != NULL) *p4 = p4ds ; v_Msg(ctx,ctx->ErrorMsgAux,"ParseUSyn",input,dp) ; return(VCAL_BadVal) ; } ;
		fp = mp ; DN(mm) ;
		if (*fp != UCEOS)
		 { 
		   cli = gpi->ci->cli ; for(curx=0;curx<cli->Count;curx++) { if (cli->CLI[curx].Language == gpi->ci->Cntry[gpi->ci->CurX].Language && cli->CLI[curx].Calendar == VCAL_CalType_Gregorian) break ; } ;
		   if (curx >= cli->Count) curx = 0 ;
		   for (mm=1;mm<=12;mm++) { if (UCstrcmpIC(cli->CLI[curx].SMonths[mm-1],mp) == 0) break ; } ;
		   if (mm > 12) { p2[-1]=p2ds ; p3[-1]=p3ds ; if (p4 != NULL) *p4 = p4ds ; v_Msg(ctx,ctx->ErrorMsgAux,"ParseUBadMonth",mp,input) ; return(VCAL_BadVal) ; } ;
		 } ;
		if (p4 == NULL) { fp = UClit("") ; } else { *p4 = p4ds ; fp = p4 ; } ;
		p2[-1]=p2ds ; p3[-1]=p3ds ;break ;
//		for(;*fp != UCEOS;fp++) { if (*fp != UClit(' ')) break ; } ;
//		DN(mm) if (mm == 0) goto badterm ; if (*(fp++) != UClit('/')) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUExpSl",input) ; return(VCAL_BadVal) ; } ;
//		DN(dd) if (*(fp++) != UClit('/')) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUExpSl",input) ; return(VCAL_BadVal) ; } ;
//		DN(yy) if (!(*fp == UCEOS || *fp == UClit(':') || *fp == UClit(' '))) goto badterm ;
//		break ;
	 } ;
//TEMP CODE FOR YC CONVERSION - VEH160513
if (yy < 100)
 { if (yy < 50) { yy += 2000 ; } else { yy += 1900 ; } ; } ;
	
	if ((date = vcal_UDateFromYMD(!futureOK,yy,mm,dd,ctx->ErrorMsgAux)) < 0) return(VCAL_BadVal) ;
/*	date contains date, now grab time portion */
grab_time:
/*	See if we have relative date specification */
	if (OKUDTERM && *fp != UCEOS)
	 { UCCHAR *tp ; PNTTYPE newPntType ;
	   tp = UCstrpbrk(fp,UClit(" :")) ; if (tp != NULL) *tp = UCEOS ;
	   if (v_CalCalc(ctx,fp,date,V4DPI_PntType_UDate,&date,&newPntType,FALSE) == NULL) return(VCAL_BadVal) ;
	   if (tp != NULL) { *tp = UClit(' ') ; fp = tp ; } else { fp = UClit("") ; } ;
	   if (relflag != NULL) *relflag = 0 ;		/* No longer have 'relative' setting (if set at top of this routine) */
	 } ;
	if (*fp == UClit(' ') || *fp == UClit(':') || *fp == UClit(' ') || *fp == UClit('T') || *fp == UClit('t'))
	 { double d ;
	   fp++ ; 
	   d = v_ParseUTime(ctx,fp) ; ut = DtoI(d) ;
	   if (ut == VCAL_BadVal) return(VCAL_BadVal) ;
	 } else { ut = 0 ; } ;

	if (date - VCAL_UDTUDateOffset < 0 || date > VCAL_UDTMax) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUDate",yy,mm,dd) ; return(VCAL_BadVal) ; } ;
	num = ((date - VCAL_UDTUDateOffset)*VCAL_SecsInDay) + ut ;
	if (!futureOK)
	 { if (num > valUDTisNOW) { v_Msg(ctx,ctx->ErrorMsgAux,"DPIFuture") ; return(VCAL_BadVal) ; } ;
	 } ;
	return(num) ;
badterm:
	if (v_IsNoneLiteral(fp)) return(0) ;
	v_Msg(ctx,ctx->ErrorMsgAux,"ParseUTerm",input,"UDate2") ; return(VCAL_BadVal) ;
}


double v_ParseUTime(ctx,input)
  struct V4C__Context *ctx ;
  UCCHAR *input ;
{
  int num,hh,min,ss,res,decimals,ampm ;
  UCCHAR *fp ;
  
	if (UCempty(input)) return(VCAL_UTime_None) ;		/* VEH131017 - Empty string same as date:none */
	fp = input ;
	num = 0 ; hh = (min = (ss = UNUSED)) ; decimals = UNUSED ; ampm = 0 ;
	for(;;fp++) { if (*fp == UClit(' ') || *fp == UClit(':')) continue ; break ; } ;
	if (UCempty(fp))
	 { hh = (min = (ss = 0)) ; goto retval ; } ;
	if (UCstrlen(fp) < 4) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUTime1",input) ; return(VCAL_BadVal) ; } ;
	for(;*fp!=UCEOS;fp++)
	 { switch(*fp)
	    { default:
	    	goto badterm ;
	      case ('a'): case UClit('p'):
	      case UClit('A'): case UClit('P'):
		if (fp[1] == UClit('m') || fp[1] == UClit('M'))
		 { ampm = (*fp == UClit('P') || *fp == UClit('p') ? 2 : 1) ;
		   fp++ ;
		 } else { goto badterm ; } ;
		break ;
	      case UClit('Z'): case UClit('z'):
		if (*(fp+1) != UCEOS) goto badterm ;
		break ;
	      case UClit('.'):		/* If got "." then see if after seconds - if so then ignore */
		if (min == UNUSED || decimals != UNUSED) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUDecimal",input) ; return(VCAL_BadVal) ; } ;
		ss = num ; num = 0 ; decimals = 0 ; break ;
	      case UClit('0'): case UClit('1'): case UClit('2'): case UClit('3'): case UClit('4'): case UClit('5'): case UClit('6'):
	      case UClit('7'): case UClit('8'): case UClit('9'):
		num *= 10 ; num += *fp - '0' ; if (decimals != UNUSED) decimals++ ; break ;
	      case UClit(':'):
		if (hh == UNUSED) { hh = num ; }
		 else if (min == UNUSED) { min = num ; }
		 else { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUTime",input) ; return(VCAL_BadVal) ; } ;
		num = 0 ; break ;
	      case UClit(' '): continue ;	/* Ignore spaces */
	    } ;
	 } ;
	if (hh == UNUSED) { hh = num / 100 ; min = num % 100 ; ss = 0 ; }
	 else if (min == UNUSED) { min = num ; ss = 0 ; }
	 else if (ss == UNUSED) { ss = num ; } ;
	if (hh < 0 || hh > (ampm > 0 ? 12 : 23) || min < 0 || min > 59 || ss < 0 || ss > 59)
	 { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUHMS",input) ; return(VCAL_BadVal) ; } ;
	switch (ampm)
	 { case 1:	hh = (hh == 12 ? 0 : hh) ; break ;
	   case 2:	hh = (hh == 12 ? hh : hh + 12) ; break ;
	 } ;

retval:	res = hh*3600 + min*60 + ss ;
	if (decimals == UNUSED) return((double)res) ;
	return((double)res + ((double)num / powers[decimals])) ;
badterm:
	if (v_IsNoneLiteral(fp)) return(VCAL_UTime_None) ;
	v_Msg(ctx,ctx->ErrorMsgAux,"ParseUTerm",input,"UDate2") ; return((double)VCAL_BadVal) ;
} ;


/*	v_ParseUMonth - Parses text date to UMonth & returns as value		*/
/*	Call: umonth = v_ParseUMonth(ctx,input,type)
	  where umonth is returned umonth (VCAL_BadVal if error (ctx->ErrorMsgAux updated with error)),
		ctx is context,
		input is input string (null terminated),
		type is type of date specification - V4LEX_TablePT_xxx		*/

int v_ParseUMonth(ctx,input,type)
  struct V4C__Context *ctx ;
  UCCHAR *input ;
  int type ;
{
  struct V4LI_CalendarInfo *cli ;
  int curx,i,yy,mm,sign,umonth ; UCCHAR *fp, month[24] ;

	if (UCempty(input)) return(VCAL_UMonth_None) ;		/* VEH131017 - Empty string same as date:none */
	fp = input ;
	if (*fp == UClit('#')) { fp++ ; type = V4LEX_TablePT_Internal ; } ;
#undef DN
#define DN(var) var=0 ; for(;;fp++) { if (*fp != UClit(' ')) break ; } ; for(;;fp++) { if (*fp >= UClit('0') && *fp <= UClit('9')) { var*=10 ; var+= *fp - UClit('0') ; } else { break ; } ; } ;
	switch(type)
	 { 
	   case V4LEX_TablePT_Internal:
		if (*fp == UClit('-')) { sign = TRUE ; fp++ ; } else { sign= FALSE ; } ;
		DN(i) if (!OKUDTERM) goto badterm ;
		return(sign ? -i : i) ;
	   case V4LEX_TablePT_YYMM:
		DN(i)
		if (*fp == UCEOS) { yy = i / 100 ; mm = i % 100 ; break ; } ;
		if (*fp == UClit('/')) { yy = i ; fp ++ ; DN(mm) ; } ;
		if (!OKUDTERM) goto badterm ;
		break ;
	   case V4LEX_TablePT_MMYY:
		DN(i)
		if (*fp == UCEOS)
		 { if (i > 9999)			/* Did we get 2 or 4 digit year ? */
		    { yy = i % 10000 ; mm = i / 10000 ; } else { yy = i % 100 ; mm = i / 100 ; } ;
		   break ;
		 } ;
		if (*fp == UClit('/')) { mm = i ; fp ++ ; DN(yy) ; } ;
		if (!OKUDTERM) goto badterm ;
		break ;
	   default:
	   case V4LEX_TablePT_MMM:
/*		Could have yy-mmm or mmm-yy - determine by alpha */
		yy = UNUSED ;
		if (*fp >= UClit('0') && *fp <= UClit('9'))
		 { DN(yy) if (*fp != UClit('-')) goto badterm ; fp++ ; } ;
		for(i=0;i<(UCsizeof(month))-1;i++)
		 { if (!vuc_IsAlpha(*fp)) break ; month[i] = *fp++ ; } ; month[i] = UClit('\0' ;)
		cli = gpi->ci->cli ; for(curx=0;curx<cli->Count;curx++) { if (cli->CLI[curx].Language == gpi->ci->Cntry[gpi->ci->CurX].Language && cli->CLI[curx].Calendar == VCAL_CalType_Gregorian) break ; } ;
		if (curx >= cli->Count) curx = 0 ;
		for (mm=1;mm<=12;mm++)
		 { 
		   if (UCstrcmpIC(cli->CLI[curx].SMonths[mm-1],month) == 0) goto m2_ok ;
//		   for(i=0;;i++)
//		    { if (toupper(cli->CLI[curx].SMonths[mm-1][i]) != month[i]) break ;
//		      if (month[i] == 0) goto m2_ok ;
//		    } ;
		 } ;
m2_ok:		if (mm > 12)
		 { if (v_IsNoneLiteral(fp)) return(0) ;
		   v_Msg(ctx,ctx->ErrorMsgAux,"ParseUMonth1",input) ; return(VCAL_BadVal) ;
		 } ;
		if (yy == UNUSED)
		 { if (*fp != UClit('-')) goto badterm ;
		   fp++ ; DN(yy) ; if (!OKUDTERM) goto badterm ;
		 } ;
	 } ;

	VCALADJYEAR(FALSE,yy) ;
	if (mm < 1 || mm > 12 || yy < VCAL_UYearMin || yy > VCAL_UYearMax) { v_Msg(ctx,ctx->ErrorMsgAux,"ParseUMonth",yy,mm,input) ; return(VCAL_BadVal) ; } ;
//	return((yy - VCAL_BaseYear) * 12 + mm - 1) ;
	umonth = YYMMtoUMONTH(yy,mm) ;
	if (*fp != UCEOS)
	 { PNTTYPE pntType ;
	   if (v_CalCalc(ctx,fp,umonth,V4DPI_PntType_UMonth,&umonth,&pntType,FALSE) == NULL) return(VCAL_BadVal) ;
	 } ;
	return(umonth) ;
badterm:
	if (v_IsNoneLiteral(fp)) return(0) ;
	v_Msg(ctx,ctx->ErrorMsgAux,"ParseUTerm",input,"UMonth") ; return(VCAL_BadVal) ;
}

/*	v_Soundex - Converts string to Soundex code			*/
/*	Call: res = v_Soundex( src , dest )
	  where res is integer soundex code (always <= 2^16),
		src is source string,
		dest is updated with 4 character result (if not NULL)	*/

int v_Soundex(src,dest)
  UCCHAR *src, *dest ;
{ UCCHAR tbuf[200],t1buf[200] ;
  int i,j,sxnum = 0, digit, firstdigit, priordigit = UNUSED  ;
  
//	if (UCstrlen(src) > 150 || UCstrlen(src) == 0)	/* If too long or short then return error */
	if (UCstrlen(src) > 150 || UCempty(src))	/* If too long or short then return error */
	 { if (dest != NULL) UCstrcpy(dest,UClit("?000")) ; return(UNUSED) ; } ;
/*	Convert St -> Saint */
	if (UCTOUPPER(src[0]) == UClit('S') && UCTOUPPER(src[1]) == UClit('T') && (src[2] == UClit('.') || src[2] == UClit(' ')	))
	 { UCstrcpy(t1buf,UClit("SAINT")) ; UCstrcat(t1buf,&src[3]) ; }
	 else { UCstrcpy(t1buf,src) ; } ;
/*	Get rid of any non-alpha characters */
	for(i=0,j=0;t1buf[i]!=UCEOS;i++)
	 { if (UCTOUPPER(t1buf[i]) >= UClit('A') && UCTOUPPER(t1buf[i]) <= UClit('Z')) tbuf[j++] = UCTOUPPER(t1buf[i]) ; } ;
	tbuf[j] = UCEOS ;
	for(i=0;tbuf[i]!=UCEOS;i++)
	 { switch(tbuf[i])
	    { default:									digit = UNUSED ; break ;
	      case UClit('B'): case UClit('F'): case UClit('P'): case UClit('V'):	digit = 1 ; break ;
	      case UClit('C'): case UClit('G'): case UClit('J'): case UClit('K'):
	      case UClit('Q'): case UClit('S'): case UClit('X'): case UClit('Z'):	digit = 2 ; break ;
	      case UClit('D'): case UClit('T'):						digit = 3 ; break ;
	      case UClit('L'):								digit = 4 ; break ;
	      case UClit('M'): case UClit('N'):						digit = 5 ; break ;
	      case UClit('R'):								digit = 6 ; break ;
	    } ;
	   if (i == 0) { firstdigit = digit ; continue ; } ;
	   if (digit == UNUSED)
	    { if (tbuf[i] == UClit('W') || tbuf[i] == UClit('H')) continue ;
	      priordigit = digit ; continue ;
	    } ;
	   if (i == 1 && digit == firstdigit) continue ;
	   if (digit == priordigit) continue ;
	   if (sxnum > 100) continue ;	/* No more than 3 digits */
	   sxnum = sxnum * 10 + digit ; priordigit = digit ;
	 } ;
	if (sxnum < 10) { sxnum *= 100 ; } else if (sxnum < 100) { sxnum *= 10 ; } ;
	if (dest != NULL) UCsprintf(dest,20,UClit("%c%03d"),tbuf[0],sxnum) ;
	return((tbuf[0] - UClit('A') + 1) * 1000 + sxnum) ;
}


int v_Soundex2(src,dest)
  UCCHAR *src, *dest ;
{ UCCHAR tbuf[200],t1buf[200] ;
  int i,j ; UCCHAR let1, let1Num ;
  
//	if (UCstrlen(src) > 150 || UCstrlen(src) == 0)	/* If too long or short then return error */
	if (UCstrlen(src) > 150 || UCempty(src))	/* If too long or short then return error */
	 { if (dest != NULL) UCstrcpy(dest,UClit("?000")) ; return(UNUSED) ; } ;
/*	Convert St -> Saint */
	if (UCTOUPPER(src[0]) == UClit('S') && UCTOUPPER(src[1]) == UClit('T') && (src[2] == UClit('.') || src[2] == UClit(' ')	))
	 { UCstrcpy(t1buf,UClit("SAINT")) ; UCstrcat(t1buf,&src[3]) ; }
	 else { UCstrcpy(t1buf,src) ; } ;
/*	Get rid of any non-alpha characters */
	for(i=0,j=0;t1buf[i]!=UCEOS;i++)
	 { if (UCTOUPPER(t1buf[i]) >= UClit('A') && UCTOUPPER(t1buf[i]) <= UClit('Z')) tbuf[j++] = UCTOUPPER(t1buf[i]) ; } ;
	tbuf[j] = UCEOS ;
	let1 = tbuf[0] ;
	for(i=0;tbuf[i]!=UCEOS;i++)
	 { switch(tbuf[i])
	    { default:									tbuf[i] = 255 ; break ;
	      case UClit('B'): case UClit('F'): case UClit('P'): case UClit('V'):	tbuf[i] = 1 ; break ;
	      case UClit('C'): case UClit('G'): case UClit('J'): case UClit('K'):
	      case UClit('Q'): case UClit('S'): case UClit('X'): case UClit('Z'):	tbuf[i] = 2 ; break ;
	      case UClit('D'): case UClit('T'):						tbuf[i] = 3 ; break ;
	      case UClit('L'):								tbuf[i] = 4 ; break ;
	      case UClit('M'): case UClit('N'):						tbuf[i] = 5 ; break ;
	      case UClit('R'):								tbuf[i] = 6 ; break ;
	    } ;
	 } ;
	let1Num = tbuf[0] ;
	for(i=0,j=0;tbuf[j]!=UCEOS;j++)
	 { if (tbuf[j] == 255) continue ;
	   tbuf[i++] = tbuf[j] ;
	 } ; tbuf[i] = UCEOS ;
	for(i=0,j=1;tbuf[j]!=UCEOS;j++)
	 { if (tbuf[i] != tbuf[j]) i++ ;
	   tbuf[i] = tbuf[j] ;
	 } ; tbuf[++i] = UCEOS ;
	if (let1Num == tbuf[0])
	 { for(i=0,j=1;tbuf[j]!=UCEOS;j++)
	    { tbuf[i++] = tbuf[j] ; } ;
	   tbuf[i] = UCEOS ;
	 } ;

	dest[0] = let1 ;
	for(i=1,j=0;tbuf[j]!=UCEOS;j++)
	 { dest[i++] = tbuf[j] + UClit('0') ;
	 } ;
	for(;i<4;i++) { dest[i] = UClit('0') ; } ;
	dest[i] = UCEOS ;
	return(1) ;
}
// Eval EchoD(Str("antidisistablishment" Soundex::Alpha) Str("ridgeway" Soundex::Alpha) Str("goncharenko" Soundex::Alpha) Str("tamney" Soundex::Alpha) Str("kreisher" Soundex::Alpha))

/*	M D 5   F I L E   C H E C K S U M		*/


#define GET_UINT32(n,b,i)                       \
{                                               \
	(n) = ( (UINT32) (b)[(i)    ]       )       \
         | ( (UINT32) (b)[(i) + 1] <<  8 )       \
         | ( (UINT32) (b)[(i) + 2] << 16 )       \
         | ( (UINT32) (b)[(i) + 3] << 24 );      \
}

#define PUT_UINT32(n,b,i)                       \
{                                               \
	(b)[(i)    ] = (UINT8) ( (n)       );       \
	(b)[(i) + 1] = (UINT8) ( (n) >>  8 );       \
	(b)[(i) + 2] = (UINT8) ( (n) >> 16 );       \
	(b)[(i) + 3] = (UINT8) ( (n) >> 24 );       \
}

/*	v_MD5Start - Setups up md5context for new go-around */
void v_MD5Start( VMD5__Context *md5cx )
{
	md5cx->total[0] = 0; md5cx->total[1] = 0;
	md5cx->state[0] = 0x67452301; md5cx->state[1] = 0xEFCDAB89; md5cx->state[2] = 0x98BADCFE; md5cx->state[3] = 0x10325476;
}

/*	v_MD5Process - Process next string chunk */
void v_MD5Process( VMD5__Context *md5cx, UINT8 data[64] )
{
	UINT32 X[16], A, B, C, D;

	GET_UINT32( X[0],  data,  0 );
	GET_UINT32( X[1],  data,  4 );
	GET_UINT32( X[2],  data,  8 );
	GET_UINT32( X[3],  data, 12 );
	GET_UINT32( X[4],  data, 16 );
	GET_UINT32( X[5],  data, 20 );
	GET_UINT32( X[6],  data, 24 );
	GET_UINT32( X[7],  data, 28 );
	GET_UINT32( X[8],  data, 32 );
	GET_UINT32( X[9],  data, 36 );
	GET_UINT32( X[10], data, 40 );
	GET_UINT32( X[11], data, 44 );
	GET_UINT32( X[12], data, 48 );
	GET_UINT32( X[13], data, 52 );
	GET_UINT32( X[14], data, 56 );
	GET_UINT32( X[15], data, 60 );

#define S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))
#define P(a,b,c,d,k,s,t) { a += F(b,c,d) + X[k] + t; a = S(a,s) + b; }

	A = md5cx->state[0]; B = md5cx->state[1]; C = md5cx->state[2]; D = md5cx->state[3];

#define F(x,y,z) (z ^ (x & (y ^ z)))

	P( A, B, C, D,  0,  7, 0xD76AA478 );
	P( D, A, B, C,  1, 12, 0xE8C7B756 );
	P( C, D, A, B,  2, 17, 0x242070DB );
	P( B, C, D, A,  3, 22, 0xC1BDCEEE );
	P( A, B, C, D,  4,  7, 0xF57C0FAF );
	P( D, A, B, C,  5, 12, 0x4787C62A );
	P( C, D, A, B,  6, 17, 0xA8304613 );
	P( B, C, D, A,  7, 22, 0xFD469501 );
	P( A, B, C, D,  8,  7, 0x698098D8 );
	P( D, A, B, C,  9, 12, 0x8B44F7AF );
	P( C, D, A, B, 10, 17, 0xFFFF5BB1 );
	P( B, C, D, A, 11, 22, 0x895CD7BE );
	P( A, B, C, D, 12,  7, 0x6B901122 );
	P( D, A, B, C, 13, 12, 0xFD987193 );
	P( C, D, A, B, 14, 17, 0xA679438E );
	P( B, C, D, A, 15, 22, 0x49B40821 );

#undef F
#define F(x,y,z) (y ^ (z & (x ^ y)))

	P( A, B, C, D,  1,  5, 0xF61E2562 );
	P( D, A, B, C,  6,  9, 0xC040B340 );
	P( C, D, A, B, 11, 14, 0x265E5A51 );
	P( B, C, D, A,  0, 20, 0xE9B6C7AA );
	P( A, B, C, D,  5,  5, 0xD62F105D );
	P( D, A, B, C, 10,  9, 0x02441453 );
	P( C, D, A, B, 15, 14, 0xD8A1E681 );
	P( B, C, D, A,  4, 20, 0xE7D3FBC8 );
	P( A, B, C, D,  9,  5, 0x21E1CDE6 );
	P( D, A, B, C, 14,  9, 0xC33707D6 );
	P( C, D, A, B,  3, 14, 0xF4D50D87 );
	P( B, C, D, A,  8, 20, 0x455A14ED );
	P( A, B, C, D, 13,  5, 0xA9E3E905 );
	P( D, A, B, C,  2,  9, 0xFCEFA3F8 );
	P( C, D, A, B,  7, 14, 0x676F02D9 );
	P( B, C, D, A, 12, 20, 0x8D2A4C8A );

#undef F
#define F(x,y,z) (x ^ y ^ z)

	P( A, B, C, D,  5,  4, 0xFFFA3942 );
	P( D, A, B, C,  8, 11, 0x8771F681 );
	P( C, D, A, B, 11, 16, 0x6D9D6122 );
	P( B, C, D, A, 14, 23, 0xFDE5380C );
	P( A, B, C, D,  1,  4, 0xA4BEEA44 );
	P( D, A, B, C,  4, 11, 0x4BDECFA9 );
	P( C, D, A, B,  7, 16, 0xF6BB4B60 );
	P( B, C, D, A, 10, 23, 0xBEBFBC70 );
	P( A, B, C, D, 13,  4, 0x289B7EC6 );
	P( D, A, B, C,  0, 11, 0xEAA127FA );
	P( C, D, A, B,  3, 16, 0xD4EF3085 );
	P( B, C, D, A,  6, 23, 0x04881D05 );
	P( A, B, C, D,  9,  4, 0xD9D4D039 );
	P( D, A, B, C, 12, 11, 0xE6DB99E5 );
	P( C, D, A, B, 15, 16, 0x1FA27CF8 );
	P( B, C, D, A,  2, 23, 0xC4AC5665 );

#undef F
#define F(x,y,z) (y ^ (x | ~z))

	P( A, B, C, D,  0,  6, 0xF4292244 );
	P( D, A, B, C,  7, 10, 0x432AFF97 );
	P( C, D, A, B, 14, 15, 0xAB9423A7 );
	P( B, C, D, A,  5, 21, 0xFC93A039 );
	P( A, B, C, D, 12,  6, 0x655B59C3 );
	P( D, A, B, C,  3, 10, 0x8F0CCC92 );
	P( C, D, A, B, 10, 15, 0xFFEFF47D );
	P( B, C, D, A,  1, 21, 0x85845DD1 );
	P( A, B, C, D,  8,  6, 0x6FA87E4F );
	P( D, A, B, C, 15, 10, 0xFE2CE6E0 );
	P( C, D, A, B,  6, 15, 0xA3014314 );
	P( B, C, D, A, 13, 21, 0x4E0811A1 );
	P( A, B, C, D,  4,  6, 0xF7537E82 );
	P( D, A, B, C, 11, 10, 0xBD3AF235 );
	P( C, D, A, B,  2, 15, 0x2AD7D2BB );
	P( B, C, D, A,  9, 21, 0xEB86D391 );

#undef F

	md5cx->state[0] += A; md5cx->state[1] += B; md5cx->state[2] += C; md5cx->state[3] += D;
}

void v_MD5Update( VMD5__Context *md5cx, UINT8 *input, UINT32 length )
{ UINT32 left, fill;

	if( ! length ) return;

	left = md5cx->total[0] & 0x3F;
	fill = 64 - left;

	md5cx->total[0] += length; md5cx->total[0] &= 0xFFFFFFFF;

	if( md5cx->total[0] < length ) md5cx->total[1]++;

	if( left && length >= fill )
	 { memcpy( (void *) (md5cx->buffer + left), (void *) input, fill );
	   v_MD5Process( md5cx, md5cx->buffer );
	   length -= fill; input  += fill;
	   left = 0;
 	} ;

	while( length >= 64 )
	 { v_MD5Process( md5cx, input );
	   length -= 64; input  += 64;
	 }

	if( length ) { memcpy( (void *) (md5cx->buffer + left),(void *) input, length ); }
}

static UINT8 md5_padding[64] =
{
 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void v_MD5Finish( VMD5__Context *md5cx, UINT8 digest[16] )
{
	UINT32 last, padn;
	UINT32 high, low;
	UINT8 msglen[8];

	high = ( md5cx->total[0] >> 29 )| ( md5cx->total[1] <<  3 );
	low  = ( md5cx->total[0] <<  3 );

	PUT_UINT32( low,  msglen, 0 ); PUT_UINT32( high, msglen, 4 );

	last = md5cx->total[0] & 0x3F;
	padn = ( last < 56 ) ? ( 56 - last ) : ( 120 - last );

	v_MD5Update( md5cx, md5_padding, padn );
	v_MD5Update( md5cx, msglen, 8 );

	PUT_UINT32( md5cx->state[0], digest,  0 );
	PUT_UINT32( md5cx->state[1], digest,  4 );
	PUT_UINT32( md5cx->state[2], digest,  8 );
	PUT_UINT32( md5cx->state[3], digest, 12 );
}

/*
 * those are the standard RFC 1321 test vectors
 */

static char *msg[] = 
{
	"",
	"a",
	"abc",
	"message digest",
	"abcdefghijklmnopqrstuvwxyz",
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
	"12345678901234567890123456789012345678901234567890123456789012345678901234567890"
};

static char *val[] =
{
	"d41d8cd98f00b204e9800998ecf8427e",
	"0cc175b9c0f1b6a831c399e269772661",
	"900150983cd24fb0d6963f7d28e17f72",
	"f96b697d7cb7938d525a2f31aaf161d0",
	"c3fcd3d76192e4007dfb496cca67e13b",
	"d174ab98d277d9f5a5611c2c9f419d9f",
	"57edf4a22be3c955ac49da2e2107b67a"
};

char *v_MD5ChecksumFile( ctx, filename, strbuf, strbuflen)
  struct V4C__Context *ctx ;
  UCCHAR *filename ;
  char *strbuf ; int strbuflen ;
{
//  FILE *fp;
  struct UC__File UCFile ;
  int i, j;
  static unsigned char output[33];
  VMD5__Context md5cx;
  unsigned char buf[1024];
  unsigned char md5sum[16];

/*	If no filename then just run "self-test" */
	if ((filename != NULL ? UCstrlen(filename) == 0 : FALSE) && strbuflen == 0)
	 { printf( "\n MD5 Validation Tests:\n\n" );
	   for( i = 0; i < 7; i++ )
	    { printf( " Test %d ", i + 1 );
	      v_MD5Start( &md5cx ) ; v_MD5Update( &md5cx, (UINT8 *) msg[i], strlen( msg[i] ) ); v_MD5Finish( &md5cx, md5sum );
	      for( j = 0; j < 16; j++ ) { sprintf( output + j * 2, "%02x", md5sum[j] ); } ;
	      if( memcmp( output, val[i], 32 ) != 0)
	       { printf( "failed!\n" );
	       } else { printf( "passed.\n" ); } ;
	    } ;
	   v_Msg(ctx,ctx->ErrorMsgAux,"OSFileNoFile") ; return(NULL) ;
	} ;

/*	Here to run checksum on a string */
	if (strbuf != NULL)
	 { v_MD5Start( &md5cx ) ; v_MD5Update(&md5cx, strbuf, strbuflen) ; v_MD5Finish( &md5cx, md5sum ) ;
	   for( j = 0; j < 16; j++ ) { sprintf( output + j * 2, "%02x", md5sum[j] ); } ;
	   return(output);
	 } ;

/*	Here to run check sum on file */
	if (!v_UCFileOpen(&UCFile,filename,UCFile_Open_ReadBin,TRUE,ctx->ErrorMsgAux,0)) return(NULL) ;
//	if( ! ( fp = fopen( filename, "rb" ) ) )
//	 { v_Msg(ctx,ctx->ErrorMsgAux,"UtilFileRead",filename,errno) ; return(NULL) ; } ;

	v_MD5Start( &md5cx );
	while((i = fread( buf, 1, sizeof( buf ), UCFile.fp )) > 0 )
	 { v_MD5Update( &md5cx, buf, i ) ; } ;
	v_MD5Finish( &md5cx, md5sum );

	v_UCFileClose(&UCFile) ;

	for( j = 0; j < 16; j++ ) { sprintf( output + j * 2, "%02x", md5sum[j] ); } ;
	return(output);
}


/*	U N I C O D E   S T U F F	*/

//#include <wchar.h>
//#include <io.h>

#define U2028 0x2028		/* Unicode line separator */

LOGICAL v_UCFileOpen(UCFile,filename,mode,uselog,errmsg,intmodx)
  struct UC__File *UCFile ;
  UCCHAR *filename ;
  int mode ;
  int uselog ;
  UCCHAR *errmsg ;
  INTMODX intmodx ;
{
  UCCHAR *fn ; unsigned char hdr[12] ;
  struct stat statbuf ;

	if (filename != NULL)
	 { if (uselog)
	    { fn = v_UCLogicalDecoder(filename,(mode == UCFile_Open_Write ? VLOGDECODE_NewFile : VLOGDECODE_Exists),0,errmsg) ; if (fn == NULL) return(FALSE) ;
	    } else { fn = filename ; } ;
//printf("FileOpen(%s) -> *%s*\n",UCretASC(filename),UCretASC(fn)) ;
	   switch (UCFile->filemode=mode)
	    { default:			UCsprintf(errmsg,50,UClit("Invalid UCFileOpen mode (%d)"),mode) ; return(FALSE) ;
	      case UCFile_Open_Read:
		UCFile->fp = (UCempty(fn) ? stdin : UCfopen(fn,"r")) ;
		if (UCFile->fp == NULL) { v_Msg(NULL,errmsg,"FileError1",fn,errno) ; return(FALSE) ; } ;
		fread(hdr,1,4,UCFile->fp) ;			/* Read first four bytes */
		UCFile->filetype = UNUSED ;
		if (hdr[0] == 0xfe && hdr[1] == 0xff) { UCFile->filetype = V4L_TextFileType_UTF16be ; }
		 else if (hdr[0] == 0xff && hdr[1] == 0xfe) { UCFile->filetype = V4L_TextFileType_UTF16le ; }
		 else if (hdr[0] == 0xef && hdr[1] == 0xbb && hdr[2] == 0xbf) { UCFile->filetype = V4L_TextFileType_UTF8 ; }
		 else if (hdr[0] == 0xff && hdr[1] == 0xff && hdr[2] == 0x0 && hdr[3] == 0x0) { UCFile->filetype = V4L_TextFileType_UTF32le ; }
		 else if (hdr[0] == 0x0 && hdr[1] == 0x0 && hdr[2] == 0xff && hdr[3] == 0xff) { UCFile->filetype = V4L_TextFileType_UTF32be ; } ;
		switch (UCFile->filetype)
		 { case UNUSED:
		   case V4L_TextFileType_ASCII:
			rewind(UCFile->fp) ;		/* unread the 4-byte prefix  */
			break ;
		   case V4L_TextFileType_UTF8:
			rewind(UCFile->fp) ;		/* unread the 4-byte prefix  */
			fread(hdr,1,3,UCFile->fp) ;	/* and re-read the 3-byte prefix */
			break ;
		   case V4L_TextFileType_UTF16be:
		   case V4L_TextFileType_UTF16le:
//			rewind(UCFile->fp) ;		/* unread the 4-byte prefix  */
//			_setmode(UCFile->fd,_O_BINARY) ;
			UCFile->fp = UCfreopen(fn,"rb",UCFile->fp) ;	/* Reopen as binary */
			fread(hdr,1,2,UCFile->fp) ;	/* and re-read the 2-byte prefix */
			break ;
		   case V4L_TextFileType_UTF32be:
		   case V4L_TextFileType_UTF32le:
			v_Msg(NULL,errmsg,"FileErrorU32",intmodx,filename) ; return(FALSE) ;
		 } ;
		if (UCFile->filetype == UNUSED) UCFile->filetype = gpi->inputCharSet ;		/* If not set then grab default from global process info */

		break ;
	      case UCFile_Open_ReadBin:
		UCFile->fp = UCfopen(fn,"rb") ;
		if (UCFile->fp == NULL) { v_Msg(NULL,errmsg,"FileError1",fn,errno) ; return(FALSE) ; } ;
		break ;
	      case UCFile_Open_WriteBin:
		UCFile->fp = UCfopen(fn,"wb") ;
		if (UCFile->fp == NULL) { v_Msg(NULL,errmsg,"FileError1",fn,errno) ; return(FALSE) ; } ;
		break ;
	      case UCFile_Open_UpdateBin:
		UCFile->fp = UCfopen(fn,"rb+") ;
		if (UCFile->fp == NULL) { v_Msg(NULL,errmsg,"FileError1",fn,errno) ; return(FALSE) ; } ;
		break ;
	      case UCFile_Open_Write:
	      case UCFile_Open_Append:
		UCFile->fp = (mode == UCFile_Open_Write ? UCfopen(fn,"w") : UCfopen(fn,"a")) ;
		if (UCFile->fp == NULL) { v_Msg(NULL,errmsg,"FileError2a",fn,errno) ; return(FALSE) ; } ;
		UCFile->filetype = UNUSED ;
		break ;
	    } ;
	   UCFile->wtbuf = NULL ; UCFile->tbufmax = 0 ; UCFile->cbuf = NULL ; UCFile->wantEOL = TRUE ; UCFile->hitEOF = FALSE ;
	   UCFile->fd = fileno(UCFile->fp) ;  UCFile->lineNum = 0 ;
	   fstat(UCFile->fd,&statbuf) ;		/* Get size of the file */
/*	   If st_size < 0 then most likely a huge file - just take the max */
	   UCFile->bufSize = (statbuf.st_size < 0 || statbuf.st_size > 50 * UCFile_GetBufMaxChars ? UCFile_GetBufMaxChars
				 : (statbuf.st_size < UCFile_GetBufDfltChars ? statbuf.st_size : UCFile_GetBufDfltChars)) ;
	   if (UCFile->bufSize == 0) UCFile->bufSize = 1 ;
	   return(TRUE) ;
	 } ;
/*	Here if no filename - not yet handling this yet */
	v_Msg(NULL,errmsg,"@Called v_UCFileOpen() with no filename?") ; return(FALSE) ;
}

int v_UCFileClose(UCFile)
  struct UC__File *UCFile ;
{
	if (UCFile->fp != NULL) { fclose(UCFile->fp) ; UCFile->fp = NULL ; } ;
	if (UCFile->wtbuf != NULL) { v4mm_FreeChunk(UCFile->wtbuf) ; UCFile->wtbuf = NULL ; } ;
	if (UCFile->cbuf != NULL) { v4mm_FreeChunk(UCFile->cbuf) ; UCFile->cbuf = NULL ; } ;
	return(1) ;
}

/*	v_UCReadLine - Reads next line in UCFile				*/
/*	Call: chars = v_UCReadLine( UCFile , mode , buf , charsize , errmsg)
	  where chars = Number of characters read (>= 0 if OK, -1 if EOF, -2 if other error),
		UCFile is pointer to UCFile,
		mode is read mode (see UCRead_xxx),
		buf is pointer to 8/16/32 buffer for result,
		charsize is target buffer (charsize is number of characters - NOT BYTES),
		errmsg is updated with any error message			*/

int v_UCReadLine(UCFile,mode,buf,charsize,errmsg)
  struct UC__File *UCFile ;
  int mode ;
  void *buf ;
  int charsize ;
  UCCHAR *errmsg ;
{
  int ok,len ;
  static unsigned char *abuf=NULL ; static int abufmax=0 ;
  UCCHAR *ub ;

	if (UCFile->hitEOF) return(-1) ;
	UCFile->lineNum++ ;
	switch (mode)
	 {
	   case UCRead_ASCII:
	     switch (UCFile->filetype)
	      {
	        case V4L_TextFileType_ASCII:
		case V4L_TextFileType_UTF8nh:
		case V4L_TextFileType_UTF8:
//Work in progress!
		  len = v_Read8BitStreamTo8Bit(buf,charsize-1,UCFile->fp,UCFile->wantEOL,&UCFile->lineTerm) ;
		  if (len == -1) UCFile->hitEOF = TRUE ;
		  return(len) ;
//		  if (fgets(buf,charsize,UCFile->fp) == NULL) return(-1) ;
//		  return(1) ;
		case V4L_TextFileType_UTF16be:
		case V4L_TextFileType_UTF16le:
		  return(-1) ;
	      } ; break ;
	   case UCRead_UTF8:
	     switch (UCFile->filetype)
	      {
	        case V4L_TextFileType_ASCII:
		case V4L_TextFileType_UTF8nh:
		case V4L_TextFileType_UTF8:
//Work in progress!
		  len = v_Read8BitStreamTo8Bit(buf,charsize-1,UCFile->fp,UCFile->wantEOL,&UCFile->lineTerm) ;
		  if (len == -1) UCFile->hitEOF = TRUE ;
		  return(len) ;
//		  if (fgets(buf,charsize,UCFile->fp) == NULL) return(-1) ; return(1) ;
		case V4L_TextFileType_UTF16be:
		case V4L_TextFileType_UTF16le:
		  if (UCFile->wtbuf == NULL)
		   { UCFile->wtbuf = v4mm_AllocChunk(UCReadLine_MaxLine*sizeof(wchar_t),FALSE) ;	/* NOTE: THIS MUST BE WCHAR_T EVEN IF NOT UNICODE */
		     UCFile->tbufmax = UCReadLine_MaxLine ;
		   } ;
//		  if (fgetws(UCFile->tbuf,UCFile->tbufmax,UCFile->fp) == NULL) return(-1) ;
//		  len = wcslen((UCCHAR *)UCFile->tbuf) ;
//	try fgetwc(UCFile->tbuf)

		  for(len=0;len<UCFile->tbufmax;len++)
		   { if ((UCFile->wtbuf[len] = fgetwc(UCFile->fp)) == WEOF) { if (len == 0) return(-1) ; break ; } ;
		     if (UCFile->filetype != V4L_TextFileType_UTF16)	/* If endian of file different from system then flip bytes */
		      UCFile->wtbuf[len] = (UCFile->wtbuf[len] << 8) | (UCFile->wtbuf[len] >> 8) ;
		     if (UCFile->wtbuf[len] == U2028) UCFile->wtbuf[len] = '\n' ;
		     if (UCFile->wtbuf[len] == '\n') break ;
		   } ; len++ ;

#ifdef ISLITTLEENDIAN /* Remember - we flipped bytes above if necessary to get local endian */
		  ok = UTF16LEToUTF8(buf,charsize,UCFile->wtbuf,len) ;
#else
		  ok = UTF16BEToUTF8(buf,charsize,UCFile->wtbuf,len) ;
#endif
		  if (ok < 0) { v_Msg(NULL,errmsg,(ok == -2 ? "UCUTF8InvCode" : "UCBufOvrFlow")) ; return(-2) ; } ;
		  ((char *)buf)[ok] = '\0' ;
		  return(1) ;
	      } ; break ;
	   case UCRead_UTF16be:
	     switch (UCFile->filetype)
	      {
	        case V4L_TextFileType_ASCII:
		  len = v_ReadASCStreamtoUC(ub=(UCCHAR *)buf,charsize-1,UCFile) ;
		  if (len == -1) UCFile->hitEOF = TRUE ;
		  return(len) ;
		case V4L_TextFileType_UTF8nh:
		case V4L_TextFileType_UTF8:
		case V4L_TextFileType_UTF16be:
		case V4L_TextFileType_UTF16le:
		  return(-1) ;
	      } ; break ;
	   case UCRead_UTF16le:
	     switch (UCFile->filetype)
	      {
		case V4L_TextFileType_UTF8nh:
		case V4L_TextFileType_UTF8:
#ifdef V4UNICODE /* If not UNICODE then drop thru to ASCII */
		  if (abufmax < charsize)
		   { abufmax = (charsize < V4LEX_Tkn_InpLineMax ? V4LEX_Tkn_InpLineMax : charsize) ;
		     if (abuf != NULL) v4mm_FreeChunk(abuf) ;
		     abuf = (char *)v4mm_AllocChunk(abufmax,FALSE) ;
		   } ;
//Work in progress!
		  len = v_Read8BitStreamTo8Bit(abuf,V4LEX_Tkn_InpLineMax,UCFile->fp,UCFile->wantEOL,&UCFile->lineTerm) ;
		  if (len < 0) { UCFile->hitEOF = TRUE ; return(-1) ; } ;
		  len = UTF8ToUTF16LE((UCCHAR *)buf,charsize*2,abuf,len) ;
		  return(len) ;
#endif
	        case V4L_TextFileType_ASCII:
//Work in progress!
		  len = v_ReadASCStreamtoUC(ub=(UCCHAR *)buf,charsize-1,UCFile) ;
		  if (len == -1) UCFile->hitEOF = TRUE ;
		  return(len) ;
		case V4L_TextFileType_UTF16be:
//Work in progress!
		  len = v_Read16BitStreamto16Bit(ub=(UCCHAR *)buf,charsize-1,UCFile->fp,UCFile->wantEOL,&UCFile->lineTerm) ;
		  if (len == -1) UCFile->hitEOF = TRUE ;
		  return(len) ;
		case V4L_TextFileType_UTF16le:
//Work in progress!
		  len = v_Read16BitStreamto16Bit(ub=(UCCHAR *)buf,charsize-1,UCFile->fp,UCFile->wantEOL,&UCFile->lineTerm) ;
		  if (len == -1) UCFile->hitEOF = TRUE ;
		  return(len) ;
	      } ; break ;
	 } ;
	return(-1) ;	/* Should not get here */
} ;


#define UCtoUTF8(UC,UTF8BUF,BYTES) \
 if (UC < 0x80) { *(UTF8BUF++) = UC ; BYTES = 1 ; } \
  else if (UC < 0x800) { *(UTF8BUF++) = (0xC0 | UC>>6) ; *(UTF8BUF++) = (0x80 | UC & 0x3F) ; BYTES = 2 ; } \
  else if (UC < 0x10000) { *(UTF8BUF++) = (0xE0 | UC >> 12) ; *(UTF8BUF++) = (0x80 | UC>>6 & 0x3F) ; *(UTF8BUF++) = (0x80 | UC & 0x3F) ; BYTES = 3 ; } \
  else if (UC < 0x200000) { *(UTF8BUF++) = (0xF0 | UC>>18) ; *(UTF8BUF++) = (0x80 | UC>>12 & 0x3F) ; *(UTF8BUF++) = (0x80 | UC>>6 & 0x3F) ; *(UTF8BUF++) = (0x80 | UC & 0x3F) ; BYTES = 4 ; } \
  else { BYTES = 0 ; } ;

int v_Read8BitStreamTo8Bit(ascbuf,max,fp,wantEOL,term)
 char *ascbuf ;
 int max ;
 FILE *fp ;
 LOGICAL wantEOL ;
 int *term ;
{
  int i,nc,nnc ;
  
	for(i=0;i<max;i++)
	 { nc = getc(fp) ;
	   switch (nc)
	    { default:		ascbuf[i] = nc ; break ;
	      case EOF:		*term = UCFile_Term_EOF ; if (i > 0) { goto hiteol ; } else return(-1) ;
	      case UClit('\n'):	*term = UCFile_Term_NL ; goto hiteol ;	
	      case UClit('\r'):	nnc = getc(fp) ; if (nnc == UClit('\n')) { *term = UCFile_Term_CRLF ; goto hiteol ; }
				ungetc(nnc,fp) ; *term = UCFile_Term_CR ; goto hiteol ;
	      case UClit('\f'):	*term = UCFile_Term_FF ; goto hiteol ;
	      case UClit('\v'):	*term = UCFile_Term_VT ; goto hiteol ;
	    } ;
	 } ;
/*	If here then maxed out the line - return what we have */
	ungetc(ascbuf[max-1],fp) ; ascbuf[max-1] = UCEOS ;
	return(max) ;
hiteol:
	if (wantEOL) ascbuf[i++] = UCEOL ; ascbuf[i] = UCEOS ;
	return(i) ;
}

int v_ReadASCStreamtoUC(ucbuf,max,UCFile)
  UCCHAR *ucbuf ;
  int max ;
  struct UC__File *UCFile ;
{
  int i,nc,nnc,nr ;

	if (UCFile->cbuf == NULL)
	 { UCFile->cbuf = (char *)v4mm_AllocChunk(UCFile->bufSize+1,FALSE) ;
	   UCFile->cbptr = UCFile->cbuf ; UCFile->cbuf[0] = '\0' ;	/* Initialize to empty buffer */
	 } ;
	
	for(i=0;i<max;)
	 { nc = *(UCFile->cbptr++) ;
	   switch (nc)
	    { default:		ucbuf[i++] = nc ; break ;
	      case '\n':	UCFile->lineTerm = UCFile_Term_NL ; goto hiteol ;	
	      case '\r':	nnc = *(UCFile->cbptr++) ;
				if (nnc == '\0')	/* Just our luck - end-of-buffer, read next chunk */
				 { nr = fread(UCFile->cbuf,1,UCFile->bufSize,UCFile->fp) ;
				   if (nr <= 0)
				    { UCFile->lineTerm = UCFile_Term_EOF ; if (i == 0) { UCFile->hitEOF = TRUE ; return(-1) ; } ;
				      UCFile->cbuf[0] = '\0' ; UCFile->cbptr = UCFile->cbuf ; goto hiteol ;
				    } ;
				   UCFile->cbuf[nr] = '\0' ; UCFile->cbptr = UCFile->cbuf ;
				   nnc = *(UCFile->cbptr++) ;
				 } ;
				if (nnc == '\n') { UCFile->lineTerm = UCFile_Term_CRLF ; goto hiteol ; }
				UCFile->cbptr-- ; UCFile->lineTerm = UCFile_Term_CR ; goto hiteol ;
	      case '\f':	UCFile->lineTerm = UCFile_Term_FF ; goto hiteol ;
	      case '\v':	UCFile->lineTerm = UCFile_Term_VT ; goto hiteol ;
	      case '\0':	nr = fread(UCFile->cbuf,1,UCFile->bufSize,UCFile->fp) ;
				if (nr <= 0)
				 { if (feof(UCFile->fp) == 0)		/* If not at EOF then output errno as warning */
				    printf("? Unexpected fread() error (%d) v_ReadASCStreamtoUC()\n",errno) ;
				   UCFile->lineTerm = UCFile_Term_EOF ; if (i == 0) { UCFile->hitEOF = TRUE ; return(-1) ; } ;
				   UCFile->cbuf[0] = '\0' ; UCFile->cbptr = UCFile->cbuf ; goto hiteol ;
				 } ;
				UCFile->cbuf[nr] = '\0' ; UCFile->cbptr = UCFile->cbuf ;
				break ;
	    } ;
	 } ;
/*	If here then maxed out the line - return what we have */
	UCFile->cbptr-- ; ucbuf[max-1] = UCEOS ;
	return(max) ;
hiteol:
	if (UCFile->wantEOL) ucbuf[i++] = UCEOL ; ucbuf[i] = UCEOS ;
	return(i) ;
}

int v_Read16BitStreamto16Bit(buf16,max,fp,wantEOL,term)
 UCCHAR *buf16 ;
 int max ;
 FILE *fp ;
 LOGICAL wantEOL ;
 int *term ;
{
  int i,nc,nnc ;
  
	for(i=0;i<max;i++)
	 { nc = getwc(fp) ;
	   switch (nc)
	    { default:		buf16[i] = nc ; break ;
	      case WEOF:	*term = UCFile_Term_EOF ; if (i > 0) { goto hiteol ; } else return(-1) ;
	      case UClit('\n'):	*term = UCFile_Term_NL ; goto hiteol ;	
	      case UClit('\r'):	nnc = getwc(fp) ; if (nnc == UClit('\n')) { *term = UCFile_Term_CRLF ; goto hiteol ; }
				ungetwc(nnc,fp) ; *term = UCFile_Term_CR ; goto hiteol ;
	      case UClit('\f'):	*term = UCFile_Term_FF ; goto hiteol ;
	      case UClit('\v'):	*term = UCFile_Term_VT ; goto hiteol ;
	    } ;
	 } ;
/*	If here then maxed out the line - return what we have */
	ungetwc(buf16[max-1],fp) ; buf16[max-1] = UCEOS ;
	return(max) ;
hiteol:
	if (wantEOL) buf16[i++] = UCEOL ; buf16[i] = UCEOS ;
	return(i) ;
}



#ifndef ISLITTLEENDIAN
#define BIG_ENDIAN 1
#endif

/**
 * UTF16LEToUTF8:
 * @out:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @out
 * @inb:  a pointer to an array of UTF-16LE passwd as a byte array
 * @inlenb:  the length of @in in UTF-16LE chars
 *
 * Take a block of UTF-16LE ushorts in and try to convert it to an UTF-8
 * block of chars out. This function assume the endian properity
 * is the same between the native type of this machine and the
 * inputed one.
 *
 * Returns the number of byte written, or -1 by lack of space, or -2
 *     if the transcoding fails (for *in is not valid utf16 string)
 *     The value of *inlen after return is the number of octets consumed
 *     as the return value is positive, else unpredictiable.
 */
int UTF16LEToUTF8(unsigned char* out, int outlen,wchar_t * in, int inlen)
{
    unsigned char* outstart= out;
    unsigned char* outend= out+outlen;
//    unsigned short* in = (unsigned short*) inb;
    wchar_t *inend;
    unsigned int c, d ;
    int bits;
#ifdef BIG_ENDIAN
    unsigned char *tmp ;
#endif

//    if ((*inlenb % 2) == 1) (*inlenb)--;
//    inlen = *inlenb / 2;
    inend= in + inlen;
    while (in < inend) {
#ifdef BIG_ENDIAN
	tmp = (unsigned char *) in;
	c = *tmp++;
	c = c | (((unsigned int)*tmp) << 8);
	in++;
#else /* BIG_ENDIAN */
        c= *in++;
#endif /* BIG_ENDIAN */
        if ((c & 0xFC00) == 0xD800) {    /* surrogates */
            if (in >= inend) {           /* (in > inend) shouldn't happens */
//                (*inlenb) -= 2;
                break;
            }
#ifdef BIG_ENDIAN
            tmp = (unsigned char *) in;
            d = *tmp++;
	    d = d | (((unsigned int)*tmp) << 8);
	    in++;
#else /* BIG_ENDIAN */
            d = *in++;
#endif /* BIG_ENDIAN */
            if ((d & 0xFC00) == 0xDC00) {
                c &= 0x03FF;
                c <<= 10;
                c |= d & 0x03FF;
                c += 0x10000;
            }
            else
	        return(-2);
        }

	/* assertion: c is a single UTF-4 value */
        if (out >= outend)
	    return(-1);
        if      (c <    0x80) {  *out++=  c;                bits= -6; }
        else if (c <   0x800) {  *out++= ((c >>  6) & 0x1F) | 0xC0;  bits=  0; }
        else if (c < 0x10000) {  *out++= ((c >> 12) & 0x0F) | 0xE0;  bits=  6; }
        else                  {  *out++= ((c >> 18) & 0x07) | 0xF0;  bits= 12; }
 
        for ( ; bits >= 0; bits-= 6) {
            if (out >= outend)
	        return(-1);
            *out++= ((c >> bits) & 0x3F) | 0x80;
        }
    }
    if (out >= outend) return(-1) ;
    *out = '\0' ;
    return(out-outstart);
}

/**
 * UTF8ToUTF16LE:
 * @outb:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @outb
 * @in:  a pointer to an array of UTF-8 chars
 * @inlen:  the length of @in
 *
 * Take a block of UTF-8 chars in and try to convert it to an UTF-16LE
 * block of chars out.
 * TODO: UTF8ToUTF16LE need a fallback mechanism ...
 *
 * Returns the number of characters written, or -1 by lack of space, or -2
 *     if the transcoding failed. 
 */
int UTF8ToUTF16LE(wchar_t * out, int outlen,unsigned char* in, int inlen)
{
//    unsigned short* out = (unsigned short*) outb;
    wchar_t *outstart= out;
    wchar_t *outend;
    const unsigned char *inend= in+inlen;
    unsigned int c, d, trailing;
#ifdef BIG_ENDIAN
    unsigned char *tmp;
    unsigned short tmp1, tmp2;
#endif /* BIG_ENDIAN */

    outend = out + outlen;
    while (in < inend) {
      d= *in++;
      if      (d < 0x80)  { c= d; trailing= 0; }
      else if (d < 0xC0)
          return(-2);    /* trailing byte in leading position */
      else if (d < 0xE0)  { c= d & 0x1F; trailing= 1; }
      else if (d < 0xF0)  { c= d & 0x0F; trailing= 2; }
      else if (d < 0xF8)  { c= d & 0x07; trailing= 3; }
      else
          return(-2);    /* no chance for this in UTF-16 */

      if (inend - in < trailing) {
//          *inlen -= (inend - in);
          break;
      } 

      for ( ; trailing; trailing--) {
          if ((in >= inend) || (((d= *in++) & 0xC0) != 0x80))
	      return(-1);
          c <<= 6;
          c |= d & 0x3F;
      }

      /* assertion: c is a single UTF-4 value */
        if (c < 0x10000) {
            if (out >= outend)
	        return(-1);
#ifdef BIG_ENDIAN
            tmp = (unsigned char *) out;
            *tmp = c ;
            *(tmp + 1) = c >> 8 ;
            out++;
#else /* BIG_ENDIAN */
            *out++ = c;
#endif /* BIG_ENDIAN */
        }
        else if (c < 0x110000) {
            if (out+1 >= outend)
	        return(-1);
            c -= 0x10000;
#ifdef BIG_ENDIAN
            tmp1 = 0xD800 | (c >> 10);
            tmp = (unsigned char *) out;
            *tmp = tmp1;
            *(tmp + 1) = tmp1 >> 8;
            out++;

            tmp2 = 0xDC00 | (c & 0x03FF);
            tmp = (unsigned char *) out;
            *tmp  = tmp2;
            *(tmp + 1) = tmp2 >> 8;
            out++;
#else /* BIG_ENDIAN */
            *out++ = 0xD800 | (c >> 10);
            *out++ = 0xDC00 | (c & 0x03FF);
#endif /* BIG_ENDIAN */
        }
        else
	    return(-1);
    }
    if (out >= outend) return(-1) ;
    *out = UCEOS ;
    return(((char *)out-(char *)outstart)/sizeof(UCCHAR));
}

/**
 * UTF16BEToUTF8:
 * @out:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @out
 * @inb:  a pointer to an array of UTF-16 passwd as a byte array
 * @inlenb:  the length of @in in UTF-16 chars
 *
 * Take a block of UTF-16 ushorts in and try to convert it to an UTF-8
 * block of chars out. This function assume the endian properity
 * is the same between the native type of this machine and the
 * inputed one.
 *
 * Returns the number of byte written, or -1 by lack of space, or -2
 *     if the transcoding fails (for *in is not valid utf16 string)
 * The value of *inlen after return is the number of octets consumed
 *     as the return value is positive, else unpredictiable.
 */
int UTF16BEToUTF8(unsigned char* out, int outlen,wchar_t *in, int inlen)
{
    unsigned char* outstart= out;
    unsigned char* outend= out+outlen;
//    unsigned short* in = (unsigned short*) inb;
    wchar_t *inend;
    unsigned int c, d;
#ifdef BIG_ENDIAN
#else /* BIG_ENDIAN */
    unsigned char *tmp;
#endif /* BIG_ENDIAN */    
    int bits;

//    if ((*inlenb % 2) == 1) (*inlenb)--;
//    inlen = *inlenb / 2;
    inend= in + inlen;
    while (in < inend) {
#ifdef BIG_ENDIAN    
        c= *in++;
#else
        tmp = (unsigned char *) in;
	c = *tmp++;
	c = c << 8;
	c = c | (unsigned int) *tmp;
	in++;
#endif	
        if ((c & 0xFC00) == 0xD800) {    /* surrogates */
	    if (in >= inend) {           /* (in > inend) shouldn't happens */
//	        (*inlenb) -= 2;
		break;
	    }

#ifdef BIG_ENDIAN
            d= *in++;
#else
            tmp = (unsigned char *) in;
	    d = *tmp++;
	    d = d << 8;
	    d = d | (unsigned int) *tmp;
	    in++;
#endif	    
            if ((d & 0xFC00) == 0xDC00) {
                c &= 0x03FF;
                c <<= 10;
                c |= d & 0x03FF;
                c += 0x10000;
            }
            else 
	        return(-2);
        }

	/* assertion: c is a single UTF-4 value */
        if (out >= outend) 
	    return(-1);
        if      (c <    0x80) {  *out++=  c;                bits= -6; }
        else if (c <   0x800) {  *out++= ((c >>  6) & 0x1F) | 0xC0;  bits=  0; }
        else if (c < 0x10000) {  *out++= ((c >> 12) & 0x0F) | 0xE0;  bits=  6; }
        else                  {  *out++= ((c >> 18) & 0x07) | 0xF0;  bits= 12; }
 
        for ( ; bits >= 0; bits-= 6) {
            if (out >= outend) 
	        return(-1);
            *out++= ((c >> bits) & 0x3F) | 0x80;
        }
    }
    if (out >= outend) return(-1) ;
    *out = '\0' ;
    return(((char *)out-(char *)outstart)/sizeof(UCCHAR));
}

/**
 * UTF8ToUTF16BE:
 * @outb:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @outb
 * @in:  a pointer to an array of UTF-8 chars
 * @inlen:  the length of @in
 *
 * Take a block of UTF-8 chars in and try to convert it to an UTF-16BE
 * block of chars out.
 * TODO: UTF8ToUTF16BE need a fallback mechanism ...
 *
 * Returns the number of byte written, or -1 by lack of space, or -2
 *     if the transcoding failed. 
 */
int UTF8ToUTF16BE(wchar_t* out, int outlen, unsigned char* in, int inlen)
{
//    unsigned short* out = (unsigned short*) outb;
    wchar_t *outstart= out;
    wchar_t *outend;
    const unsigned char* inend= in+inlen;
    unsigned int c, d, trailing;
#ifdef BIG_ENDIAN
#else
    unsigned char *tmp;
    unsigned short tmp1, tmp2;
#endif /* BIG_ENDIAN */    

    outend = out + outlen;
    while (in < inend) {
      d= *in++;
      if      (d < 0x80)  { c= d; trailing= 0; }
      else if (d < 0xC0)
          return(-2);    /* trailing byte in leading position */
      else if (d < 0xE0)  { c= d & 0x1F; trailing= 1; }
      else if (d < 0xF0)  { c= d & 0x0F; trailing= 2; }
      else if (d < 0xF8)  { c= d & 0x07; trailing= 3; }
      else
          return(-2);    /* no chance for this in UTF-16 */

      if (inend - in < trailing) {
//          *inlen -= (inend - in);
          break;
      } 

      for ( ; trailing; trailing--) {
          if ((in >= inend) || (((d= *in++) & 0xC0) != 0x80))  return(-1);
          c <<= 6;
          c |= d & 0x3F;
      }

      /* assertion: c is a single UTF-4 value */
        if (c < 0x10000) {
            if (out >= outend)  return(-1);
#ifdef BIG_ENDIAN
            *out++ = c;
#else
            tmp = (unsigned char *) out;
            *tmp = c >> 8;
            *(tmp + 1) = c;
            out++;
#endif /* BIG_ENDIAN */
        }
        else if (c < 0x110000) {
            if (out+1 >= outend)  return(-1);
            c -= 0x10000;
#ifdef BIG_ENDIAN
            *out++ = 0xD800 | (c >> 10);
            *out++ = 0xDC00 | (c & 0x03FF);
#else
            tmp1 = 0xD800 | (c >> 10);
            tmp = (unsigned char *) out;
            *tmp = tmp1 >> 8;
            *(tmp + 1) = tmp1;
            out++;

            tmp2 = 0xDC00 | (c & 0x03FF);
            tmp = (unsigned char *) out;
            *tmp = tmp2 >> 8;
            *(tmp + 1) = tmp2;
            out++;
#endif
        }
        else  return(-1);
    }
    if (out >= outend) return(-1) ;
    *out = UCEOS ;
    return(((char *)out-(char *)outstart)/sizeof(UCCHAR));
}

/*	Temp routine to convert UC to ASC and return ASC */

char *v_ConvertUCtoASC(ucstr)
  UCCHAR *ucstr ;
{
  static char abuf[5][32000+1] ;	/* Yes, I know this is really bad - but it is just a bridge function */
  static int cnt=0 ;
  int i ; char *b ;
	
	b = abuf[(cnt++) % 5] ;
	for(i=0;i<32000;i++) { b[i] = ucstr[i] ; if (b[i] == '\0') return(b) ; } ;
	b[i] = UCEOS ; return(b) ;
} ;

UCCHAR *v_ConvertASCtoUC(str)
  char *str ;
{
  static UCCHAR ucbuf[5][32000] ;	/* Yes, I know this is really bad - but it is just a bridge function */
  static int cnt=0 ;
  int i ; UCCHAR *ucb ;

	ucb = ucbuf[(cnt++) % 5] ;
	for(i=0;;i++) { ucb[i] = str[i] ; if (ucb[i] == '\0') return(ucb) ; }
} ;

#ifdef WANTVUCASSUBROUTINES
#define VUC(attr) \
LOGICAL vuc_##attr(ucchar) \
  UCCHAR ucchar ;\
{	return(uci->Entry[ucchar].attr) ; }

	VUC(IsAlpha)
	VUC(IsLetter)
	VUC(IsUpper)
	VUC(IsLower)
	VUC(IsDigit)
	VUC(IsWSpace)
	VUC(IsPunc)
	VUC(IsBgnId)
	VUC(IsContId)
	VUC(IsMath)
#endif

int vuc_StrCmpIC(ucs1,ucs2,numchar)
  UCCHAR *ucs1,*ucs2 ;
  int numchar ;
{ int i1,i2, c1,c2, if1, if2, maxif ; int hx ;
  UCCHAR *cf1,*cf2 ;

	if (numchar == 0) numchar = 999999999 ;
	for(i1=0,i2=0;ucs1[i1]!=UCEOS;i1++,i2++)
	 { if (--numchar < 0) return(0) ;	/* We matched enough - return 0 */
	   if (ucs1[i1] == ucs2[i2]) continue ;
	   c1 = ucs1[i1] ; c2 = ucs2[i2] ;
	   if (c1 <= 0x7f && c2 <= 0x7f)	/* Both characters ASCII */
	    { if (uci->asciiLC[c1] == uci->asciiLC[c2]) { continue ; } else { goto fail ; } ;
	    } ;
	   if (uci->Entry[c1].IsFolded || uci->Entry[c2].IsFolded)
	    { if (uci->Entry[c1].IsFolded) { UIFOLDHASH(c1) ; cf1 = uci->fold[hx].foldchars ; if1 = uci->fold[hx].chars ; }
	       else { cf1 = &ucs1[i1] ; if1 = 0 ; } ;
	      if (uci->Entry[c2].IsFolded) { UIFOLDHASH(c2) ; cf2 = uci->fold[hx].foldchars ; if2 = uci->fold[hx].chars ; }
	       else { cf2 = &ucs2[i2] ; if2 = 0 ; } ;
	      maxif = (if1 > if2 ? if1 : if2) ;
	      if (UCstrncmp(cf1,cf2,maxif) != 0) goto fail ;
/*	      Got a match on folded characters - advance i1 & i2 properly */
	      if (if1 == 0) i1 += (maxif - 1) ; if (if2 == 0) i2 += (maxif - 1) ;
	    } ;
	   if ((uci->Entry[c1].IsUpper ? uci->Entry[c1].OtherValue : c1) ==
	       (uci->Entry[c2].IsUpper ? uci->Entry[c2].OtherValue : c2)) continue ;
	   break ;				/* Characters are not equal - quit */
	 } ;
	return(numchar == 0 ? 0 : ucs1[i1]-ucs2[i2]) ;
fail:	return(ucs1[i1]-ucs2[i2]) ;
}

UCCHAR *vuc_StrStrIC(ucs1,ucs2)
  UCCHAR *ucs1,*ucs2 ;
{ int l1,l2,i ;

	l1 = UCstrlen(ucs1) ; l2 = UCstrlen(ucs2) ;
	for(i=0;i<=l1-l2;i++) { if (vuc_StrCmpIC(ucs2,&ucs1[i],l2) == 0) return(&ucs1[i]) ; } ;
	return(NULL) ;

}

/*	Compares to see if two strings are same	*/
LOGICAL vuc_Cmp2PtsStr(pt1,pt2)
  struct V4DPI__Point *pt1, *pt2 ;
{ int l1, l2, i ;
	switch (pt1->PntType)
	 { default:				return(FALSE) ;
	   case V4DPI_PntType_UCChar:
	     switch (pt2->PntType)
	      { default:			return(FALSE) ;
	        case V4DPI_PntType_UCChar:	/* UCChar vs UCChar */
		  return(UCstrncmp(pt1->Value.UCVal,pt2->Value.UCVal,UCCHARSTRLEN(pt1)+1) == 0) ;
	        CASEofCharmU			/* UCChar vs Char */
		  l1 = UCCHARSTRLEN(pt1) ; l2 = CHARSTRLEN(pt2) ; if (l1 != l2) return(FALSE) ;
	          for(i=1;i<=l1;i++) { if (pt1->Value.UCVal[i] != pt2->Value.AlphaVal[i]) return(FALSE) ; } ;
	          return(TRUE) ;
	      } ;
	   CASEofCharmU
	     switch (pt2->PntType)
	      { default:			return(FALSE) ;
	        case V4DPI_PntType_UCChar:	/* Char vs UCChar */
		  l1 = CHARSTRLEN(pt1) ; l2 = UCCHARSTRLEN(pt2) ; if (l1 != l2) return(FALSE) ;
	          for(i=1;i<=l1;i++) { if (pt1->Value.AlphaVal[i] != pt2->Value.UCVal[i]) return(FALSE) ; } ;
	          return(TRUE) ;
	        CASEofCharmU			/* Char vs Char */
		  l1 = CHARSTRLEN(pt1) ; l2 = CHARSTRLEN(pt2) ; if (l1 != l2) return(FALSE) ;
		  return(strncmp(pt1->Value.AlphaVal,pt2->Value.AlphaVal,l1) == 0) ;
	      } ;
	 } ;
}

//#include <signal.h>


/*	v_LoadUnicodeData - Loads uci->xxx structure with Unicode information							*/
/*	Call: ok = v_LoadUnicodeData( errmsg)
	  where ok = FALSE if problem (in errmsg), TRUE if ok,
		errmsg is pointer to UCCHAR buffer for any error messages							*/

LOGICAL v_LoadUnicodeData(errmsg)
  UCCHAR *errmsg ;
{ 
//  struct V4C__ProcessInfo *gpi=NULL ;
  struct UC__File UCFile ;
  char buf[512],*b,*d,*nd ;
#ifdef V4UNICODE
#define UCMAXCASEFOLD 10
  UCCHAR cf[UCMAXCASEFOLD] ;
  int hits,hx ; char *d1 ;
#endif 
  int line,i,value,start,end,ok,endval,ucisize, offset ; UCCHAR wmsg[512] ;

//signal(SIGSEGV,SIG_DFL) ;

	endval = 0xffff ;
	ucisize = (((char *)&uci->Entry[0])-(char *)uci) + (endval + 1)*sizeof(uci->Entry[0]) ;
//	uci = (struct V__UnicodeInfo *)v4mm_AllocChunk(ucisize,TRUE) ;
//	gpi = v_GetProcessInfo() ;
	offset = v_mctAlloc(gpi,ucisize,NULL) ; gpi->mct->UnicodeOffset = offset ;
	uci = (struct V__UnicodeInfo *)&gpi->mct->Data[offset] ; memset(uci,0,ucisize) ;


/*	If here then have to read Unicode text files */
	if (!v_UCFileOpen(&UCFile,v_GetV4HomePath(UClit("UnicodeData.v4i")),UCFile_Open_Read,FALSE,wmsg,0))
	 { v_Msg(NULL,errmsg,"@* Err accessing UnicodeData file: %1U",wmsg) ; return(FALSE) ; } ;
	for(line=1;;line++)
	 { if (v_UCReadLine(&UCFile,UCRead_UTF8,buf,sizeof(buf),wmsg) < 0) break ;
#define UT(field) uci->Entry[value].field = TRUE ;
	   ok = TRUE ;
	   for(b=buf,i=0;i<15;b=d+1,i++)
	    { d = strchr(b,';') ; if (d != NULL) *d = '\0' ;
	      switch (i)
	       { case 0:	ok = TRUE ; value = strtol(b,&nd,16) ; if (*nd != '\0') { ok = FALSE ; } ; break ;
	         case 1:	continue ;	/* Unicode character name */
	         case 2:	if (b[0] == 'L') { UT(IsLetter) ; }
				 else if (strcmp(b,"Nd") == 0) { UT(IsDigit) ; }
				 else if (b[0] == 'P') { UT(IsPunc) ; }
				 else if (b[0] == 'Z') { UT(IsWSpace) ; } ;
				if (b[0] == 'L' || b[0] == 'N' || b[0] == 'P' || b[0] == 'S') { UT(IsPrint) ; } ;
				continue ;
		 case 3:			/* Cannonical combining class */
		 case 4:			/* Bidi class */
		 case 5:	continue ;	/* Decomposition */
		 case 6:	if (strlen(b) > 0) uci->Entry[value].OtherValue = strtol(b,&nd,10) ; break ;	/* Digit map */
		 case 7:			/* If digit, but not decimal then this has value */
		 case 8:	continue ;	/* If special number (such as fraction) then value is here */
		 case 9:			/* Bidi_Mirrored */
		 case 10:			/* Old name */
		 case 11:	continue ;	/* ISO comment */
		 case 12:	if (strlen(b) > 0) uci->Entry[value].OtherValue = strtol(b,&nd,16) ; break ;	/* Upper case map */
		 case 13:	if (strlen(b) > 0) uci->Entry[value].OtherValue = strtol(b,&nd,16) ; break ;	/* Lower case map */
		 case 14:	continue ;	/* Title case map */
		 case 15:	ok = FALSE ;break ;	/* s/b end of line */
	       } ;
	      if (!ok) break ; if (i == 0 && value > endval) break ;
	    } ;
	 } ;
	v_UCFileClose(&UCFile) ;
/*	Go through the properties & pull up stuff like white-space, quotes, etc. */
	if (!v_UCFileOpen(&UCFile,v_GetV4HomePath(UClit("UnicodePropList.v4i")),UCFile_Open_Read,FALSE,wmsg,0))
	 { v_Msg(NULL,errmsg,"@* Err accessing UnicodePropList file: %1U",wmsg) ; return(FALSE) ; } ;
	for(line=1;;line++)
	 { int c1,c2 ;
	   if (v_UCReadLine(&UCFile,UCRead_UTF8,buf,sizeof(buf),errmsg) < 0) break ;
	   c1 = strtol(buf,&nd,16) ; if (c1 <= 0) continue ;
	   c2 = (*nd == '.' ? strtol(nd+2,&nd,16) : c1) ;	/* Do we have a range ? */
/*	   We currently only handle 16 bit */
	   if (c2 > 0xffff) continue ;
	   if (strstr(buf,"White_Space") != NULL) { for(value=c1;value<=c2;value++) { UT(IsWSpace) ; } ; }
	    else if (strstr(buf,"Other_Math") != NULL)
	     { for(value=c1;value<=c2;value++) { UT(IsMath) ; } ; }
	    else if (strstr(buf,"Other_Alphabetic") != NULL)
	     { for(value=c1;value<=c2;value++) { UT(IsAlpha) ; } ; }
	    else if (strstr(buf,"Other_LowerCase") != NULL)
	     { for(value=c1;value<=c2;value++) { UT(IsLower) ; } ; } ;
	 } ;
	v_UCFileClose(&UCFile) ;


/*	Now go thru derived properties to pick up some extra stuff */
	if (!v_UCFileOpen(&UCFile,v_GetV4HomePath(UClit("UnicodeDerivedCoreProp.v4i")),UCFile_Open_Read,FALSE,wmsg,0))
	 { v_Msg(NULL,errmsg,"@* Err accessing UnicodeDerivedCoreProp file: %1U",wmsg) ; return(FALSE) ; } ;
	for(line=1;;line++)
	 { if (v_UCReadLine(&UCFile,UCRead_UTF8,buf,sizeof(buf),errmsg) < 0) break ;
#define UTM(field) for(value=start;value<=end;value++) uci->Entry[value].field = TRUE ;
	   for(b=buf,ok=TRUE,i=0;ok&&i<=1;b=d+1,i++)
	    { d = strchr(b,';') ; if (d != NULL) *d = '\0' ;
	      for(;*b==' ';b++) { } ;		/* Skip leading spaces */
	      switch (i)
	       { case 0:	ok = TRUE ;
				start = strtol(b,&nd,16) ;
				if (*nd == '.') { end = strtol(nd+2,&nd,16) ; if (end > endval) end = endval ;}
				 else if (*nd == ' ') { end = start ; }
				 else { ok = FALSE ; } ;
				if (start > endval) ok = FALSE ;
				break ;
	         case 1:	if (strncmp("ID_Start",b,8) == 0) { UTM(IsBgnId) ; }
				 else if (strncmp("ID_Continue",b,11) == 0) { UTM(IsContId) ; }
				 else if (strncmp("Math",b,4) == 0)
				  { UTM(IsMath) ; }
				 else if (strncmp("Alphabetic",b,10) == 0) { UTM(IsAlpha) ; }
				 else if (strncmp("Lowercase",b,9) == 0)
				  { UTM(IsLower) ; }
				 else if (strncmp("Uppercase",b,9) == 0)
				  { UTM(IsUpper) ; } ;
				break ;
	       } ;
	    } ;
	 } ;
	v_UCFileClose(&UCFile) ;

/*	Now go thru case folding file for case-insensitive comparisons */
#ifdef V4UNICODE
	if (!v_UCFileOpen(&UCFile,v_GetV4HomePath(UClit("UnicodeCaseFolding.v4i")),UCFile_Open_Read,FALSE,wmsg,0))
	 { v_Msg(NULL,errmsg,"@* Err accessing UnicodeCaseFolding file: %1U",wmsg) ; return(FALSE) ; } ;
	for(line=1;;line++)
	 { if (v_UCReadLine(&UCFile,UCRead_UTF8,buf,sizeof(buf),errmsg) < 0) break ;
	   if (buf[0] == '#') continue ;
	   for(b=buf,ok=TRUE,i=0;ok&&i<=2;b=d+1,i++)
	    { d = strchr(b,';') ; if (d != NULL) *d = '\0' ;
	      for(;*b==' ';b++) { } ;		/* Skip leading spaces */
	      switch (i)
	       { case 0:	ok = TRUE ;
				value = strtol(b,&nd,16) ;
				if (*nd != '\0' || value > 0xffff) { ok = FALSE ; break ; } ;
				continue ;
	         case 1:	if (b[0] == 'T') { ok = FALSE ; break ; } ;	/* Skip special case for I, dotted I (Turkic languages only) */
				continue ;
	         case 2:	for(i=0;i<UCMAXCASEFOLD;b=d1+1,i++)
				 { d1 = strchr(b,' ') ; if (d1 != NULL) *d1 = '\0' ;
				   cf[i] = strtol(b,&nd,16) ; if (*nd != '\0') { ok = FALSE ; break ; } ;
				   if (d1 == NULL) break ;
				 } ; if (!ok) break ;
				/* If got only 1 value and same as value's othervalue & value is upper case then don't really need it */
				i++ ;
				if (i == 1 && cf[0] == uci->Entry[value].OtherValue && uci->Entry[value].IsUpper) { ok = FALSE ; break ; } ;
				if (i > UI_FOLDCHARMAX) i = UI_FOLDCHARMAX ;
				for(hits=0,hx=UIHASH1(value);uci->foldcount < UI_MAXFOLD-100;hx=UIHASH2(hx))
				 { if (uci->fold[hx].ucval != 0) { hits++ ; continue ; } ;
				   uci->fold[hx].ucval = value ; uci->fold[hx].chars = i ; uci->Entry[value].IsFolded = TRUE ;
				   for(;i>=0;i--) { uci->fold[hx].foldchars[i] = cf[i] ; } ;
				   uci->foldcount ++ ;
				   break ;
				 } ;
				if (uci->foldcount >= UI_MAXFOLD-100) printf("??? Warning - exceeded max number of case folding entries ???\n") ;
				break ;
	       } ;
	    } ;
	 } ;
	v_UCFileClose(&UCFile) ;
#endif  /* V4UNICODE */
/*	Create a couple of arrays for fast ASCII character handling */
	for(i=0;i<128;i++) { uci->asciiLC[i] = tolower(i) ; uci->asciiUC[i] = toupper(i) ; } ;
	return(TRUE) ;
} ;


/******************************************************************************/
/*                         Start of LZRW1.C                                   */
/******************************************************************************

THE LZRW1 ALGORITHM
===================
Author : Ross N. Williams.
Date   : 31-Mar-1991.

1. I typed the following code in from my paper "An Extremely Fast Data
Compression Algorithm", Data Compression Conference, Utah, 7-11 April,
1991. The  fact that this  code works indicates  that the code  in the
paper is OK.

2. This file has been copied into a test harness and works.

3. Some users running old C compilers may wish to insert blanks around
the "="  symbols of  assignments so  as to  avoid expressions  such as
"a=*b;" being interpreted as "a=a*b;"

4. This code is public domain.

5. Warning:  This code  is non-deterministic insofar  as it  may yield
different  compressed representations  of the  same file  on different
runs. (However, it will always decompress correctly to the original).

6. If you use this code in anger (e.g. in a product) drop me a note at
ross@spam.ua.oz.au and I will put you  on a mailing list which will be
invoked if anyone finds a bug in this code.

7.   The  internet   newsgroup  comp.compression   might  also   carry
information on this algorithm from time to time.

******************************************************************************/

#define FLAG_BYTES    4     /* Number of bytes used by copy flag. */
#define FLAG_COMPRESS 0     /* Signals that compression occurred. */
#define FLAG_COPY     1     /* Signals that a copyover occurred.  */

/******************************************************************************/

void lzrw1_compress(p_src_first,src_len,p_dst_first,p_dst_len)
/* Input  : Specify input block using p_src_first and src_len.          */
/* Input  : Point p_dst_first to the start of the output zone (OZ).     */
/* Input  : Point p_dst_len to a ULONG to receive the output length.    */
/* Input  : Input block and output zone must not overlap.               */
/* Output : Length of output block written to *p_dst_len.               */
/* Output : Output block in Mem[p_dst_first..p_dst_first+*p_dst_len-1]. */
/* Output : May write in OZ=Mem[p_dst_first..p_dst_first+src_len+256-1].*/
/* Output : Upon completion guaranteed *p_dst_len<=src_len+FLAG_BYTES.  */
UBYTE *p_src_first,*p_dst_first; ULONG src_len,*p_dst_len;
#define PS *p++!=*s++  /* Body of inner unrolled matching loop.         */
#define ITEMMAX 16     /* Maximum number of bytes in an expanded item.  */
{UBYTE *p_src=p_src_first,*p_dst=p_dst_first;
 UBYTE *p_src_post=p_src_first+src_len,*p_dst_post=p_dst_first+src_len;
 UBYTE *p_src_max1=p_src_post-ITEMMAX,*p_src_max16=p_src_post-16*ITEMMAX;
 UBYTE *hash[4096],*p_control; UWORD control=0,control_bits=0;
 *p_dst=FLAG_COMPRESS; p_dst+=FLAG_BYTES; p_control=p_dst; p_dst+=2;
 memset(&hash[0],0,4096*sizeof(hash[0])) ;
 while (TRUE)
   {UBYTE *p,*s; UWORD unroll=16,len,index; ULONG offset;
    if (p_dst>p_dst_post) goto overrun;
    if (p_src>p_src_max16)
      {unroll=1;
       if (p_src>p_src_max1)
         {if (p_src==p_src_post) break; goto literal;}}
    begin_unrolled_loop:
       index=((40543*((((p_src[0]<<4)^p_src[1])<<4)^p_src[2]))>>4) & 0xFFF;
       p=hash[index]; hash[index]=s=p_src; offset=s-p;
       if (offset>4095 || p<p_src_first || offset==0 || PS || PS || PS)
         {literal: *p_dst++=*p_src++; control>>=1; control_bits++;}
       else
         {PS || PS || PS || PS || PS || PS || PS ||
          PS || PS || PS || PS || PS || PS || s++; len=s-p_src-1;
          *p_dst++=((offset&0xF00)>>4)+(len-1); *p_dst++=offset&0xFF;
          p_src+=len; control=(control>>1)|0x8000; control_bits++;}
    if (--unroll) goto begin_unrolled_loop;
    if (control_bits==16)
      {*p_control=control&0xFF; *(p_control+1)=control>>8;
       p_control=p_dst; p_dst+=2; control=control_bits=0;}
   }
 control>>=16-control_bits;
 *p_control++=control&0xFF; *p_control++=control>>8;
 if (p_control==p_dst) p_dst-=2;
 *p_dst_len=p_dst-p_dst_first;
 return;
 overrun: memcpy(p_dst_first+FLAG_BYTES,p_src_first,src_len);
          *p_dst_first=FLAG_COPY; *p_dst_len=src_len+FLAG_BYTES;
}

/******************************************************************************/

void lzrw1_decompress(p_src_first,src_len,p_dst_first,p_dst_len,max_dst_len)
/* Input  : Specify input block using p_src_first and src_len.          */
/* Input  : Point p_dst_first to the start of the output zone.          */
/* Input  : Point p_dst_len to a ULONG to receive the output length.    */
/* Input  : Input block and output zone must not overlap. User knows    */
/* Input  : upperbound on output block length from earlier compression. */
/* Input  : In any case, maximum expansion possible is eight times.     */
/* Input  : max_dst_len is max size allowed to write			*/
/* Output : Length of output block written to *p_dst_len.               */
/* Output : Output block in Mem[p_dst_first..p_dst_first+*p_dst_len-1]. */
/* Output : Writes only  in Mem[p_dst_first..p_dst_first+*p_dst_len-1]. */
UBYTE *p_src_first, *p_dst_first; ULONG src_len, *p_dst_len, max_dst_len;
{UWORD controlbits=0, control;
 int dlen ;
 UBYTE *p_src=p_src_first+FLAG_BYTES, *p_dst=p_dst_first,
       *p_src_post=p_src_first+src_len;

 if (*p_src_first==FLAG_COPY)		/* Not compressed? then just copy into dst */
  { dlen = src_len-FLAG_BYTES ; if (dlen > max_dst_len) dlen = max_dst_len ;
    memcpy(p_dst_first,p_src_first+FLAG_BYTES,dlen) ;
    *p_dst_len=src_len-FLAG_BYTES ;
    return ;
  } ;
 dlen = 0 ;
 while (p_src!=p_src_post)
   {if (controlbits==0) { control=*p_src++ ; control|=(*p_src++)<<8 ; controlbits=16 ; } ;
    if (control&1)
     { UWORD offset,len; UBYTE *p;
       offset = (*p_src & 0xF0) << 4 ; len = 1 + (*p_src++ & 0xF) ;
       offset += *p_src++ & 0xFF ; p = p_dst - offset ;
       dlen += len ; if (dlen > max_dst_len) { *p_dst_len = V4LIM_BiggestPositiveInt ; return ; } ;
       while (len--) *p_dst++ = *p++ ;
     } else
     { dlen ++ ; if (dlen > max_dst_len) { *p_dst_len = V4LIM_BiggestPositiveInt ; return ; } ;
       *p_dst++ = *p_src++ ;
     } ;
    control >>= 1 ; controlbits-- ;
   } ;
 *p_dst_len = p_dst - p_dst_first ;

}

/******************************************************************************/
/*                          End of LZRW1.C                                    */
/******************************************************************************/

/* ********************************************************************************************************** */
/*	D I C T I O N A R Y   M O D U L E S
/* ********************************************************************************************************** */

/*	Set up structure (deltbl[]) containing referenced Dict points { enum-value, textvalue } */
#define DICTIONARYENTRY(sym,value) { _##sym , UClit(value) },
struct delist { enum DictionaryEntries deval ; UCCHAR tval[V4DPI_DictEntryVal_Max+1] ; } ;
struct delist deltbl[] = { ALLDICTENTRIES } ;
#define SIZEOFDELTBL ((sizeof deltbl) / (sizeof deltbl[0]))

#define DICTENTCACHEMAX 67
struct lcl__decache {
  int intval[DICTENTCACHEMAX] ;
  enum DictionaryEntries deval[DICTENTCACHEMAX] ;
  UCCHAR *uctval[SIZEOFDELTBL] ;			/* If not NULL then points to corresponding uppercase version of deltbl[x].tval */
 } ; static struct lcl__decache *lde ;
#ifdef V4ENABLEMULTITHREADS
  static DCLSPINLOCK ldeLock = UNUSEDSPINLOCKVAL ;
#endif

/*	v4im_GetDictToEnumVal - Converts V4 point to corresponding DictEntry enum point	*/
/*	Call: enum = v4im_GetDictToEnumVal( ctx , point )
	  where enum is enum value (-1 if not found),
		ctx is context,
		point is point to map to enum value					*/

enum DictionaryEntries v4im_GetDictToEnumVal(ctx,point)
 struct V4C__Context *ctx ;
 P *point ;
{
  int i,j,hx,ok,first,last ; UCCHAR tsym[V4DPI_DictEntryVal_Max+1] ;
 
	GRABMTLOCK(ldeLock) ;
	if (lde == NULL) lde = (struct lcl__decache *)v4mm_AllocChunk(sizeof *lde,TRUE) ;
 /*	First, let's see if we have already come across this value */
	if (point->PntType == V4DPI_PntType_Dict)
	 { hx = (point->Value.IntVal % DICTENTCACHEMAX) & 0x7fffffff ;
	   if (lde->intval[hx] == point->Value.IntVal) { FREEMTLOCK(ldeLock) ; return(lde->deval[hx]) ; } ;	/* Got it! */
	 } ;
/*	Don't have it (or had it & lost it)  - do binary search of deltbl looking for it */
	v4im_GetPointUC(&ok,tsym,UCsizeof(tsym),point,ctx) ; if (!ok) { FREEMTLOCK(ldeLock) ; return(UNUSED) ; } ;
	UCSTRTOUPPER(tsym) ;			/* Convert to upper case */

	for(first=0,last=SIZEOFDELTBL-1;;)
	 { i = (last + first) / 2 ;
	   if (lde->uctval[i] == NULL)
	    { lde->uctval[i] = v4mm_AllocUC(V4DPI_DictEntryVal_Max) ;
	      for(j=0;;j++) { if ((lde->uctval[i][j] = UCTOUPPER(deltbl[i].tval[j])) == '\0') break ; } ;
	    } ;
	   j = UCstrcmp(tsym,lde->uctval[i]) ;
	   if (j == 0) break ; if (j < 0) { last = i - 1 ; } else { first = i + 1 ; } ;
	   if (first > last) { i = SIZEOFDELTBL+1 ; break ; } ;
	 } ;
	if (i >= SIZEOFDELTBL) { FREEMTLOCK(ldeLock) ; return(-1) ; } ;	/* Not in the table */
/*	If point is Dict entry then cache result */
	if (point->PntType == V4DPI_PntType_Dict)
	 { hx = (point->Value.IntVal % DICTENTCACHEMAX) & 0x7fffffff ;
	   lde->intval[hx] = point->Value.IntVal ; lde->deval[hx] = deltbl[i].deval ;
	 } ;
	FREEMTLOCK(ldeLock) ; return(deltbl[i].deval) ;
}

/*	v4im_GetUCStrToEnumVal - Converts Unicode string to corresponding DictEntry enum point	*/
/*	Call: enum = v4im_GetUCStrToEnumVal( ucstr , minmatch )
	  where enum is enum value (-1 if not found),
		ucstr is Unicode string,
		minmatch is minimum number characters to match (0 for exact)			*/

enum DictionaryEntries v4im_GetUCStrToEnumVal(ucstr,minmatch)
 UCCHAR *ucstr ;
 int minmatch ;
{ 
  int i,j,cnt,first,last ; UCCHAR tsym[V4DPI_DictEntryVal_Max+1] ;
 
	if (UCstrlen(ucstr) >= UCsizeof(tsym)) return(-1) ;
	GRABMTLOCKnoCTX(ldeLock) ; 
	if (lde == NULL) lde = (struct lcl__decache *)v4mm_AllocChunk(sizeof *lde,TRUE) ;
	UCcnvupper(tsym,ucstr,UCsizeof(tsym)) ;			/* Convert to upper case */

#ifdef _DEBUG
{ static int first=TRUE;
  if (first)
   { for(i=0;i<SIZEOFDELTBL-1;i++)
      { if (UCstrcmpIC(deltbl[i].tval,deltbl[i+1].tval) >= 0) printf("??????Dictionary entries not in order - problem with %s\n",UCretASC(deltbl[i].tval)) ; } ;
   } ; first = FALSE ;
}
#endif

	for(first=0,last=SIZEOFDELTBL-1;;)
	 { i = (last + first) / 2 ;
	   if (lde->uctval[i] == NULL)
	    { lde->uctval[i] = v4mm_AllocUC(V4DPI_DictEntryVal_Max) ;
	      for(j=0;;j++) { if ((lde->uctval[i][j] = UCTOUPPER(deltbl[i].tval[j])) == '\0') break ; } ;
	    } ;
	   j = (minmatch > 0 ? UCstrncmp(tsym,lde->uctval[i],minmatch) : UCstrcmp(tsym,lde->uctval[i])) ;
	   if (j == 0)							/* We got a match - make sure not ambiguous */
	    { if(minmatch == 0) break ;					/* Got exact match */
/*	      Need to position to begin of chain of possible ambiguous matches - see if exact otherwise it's ambiguous */
	      for (;i>0;i--)
	       { if (lde->uctval[i-1] == NULL)
		  { lde->uctval[i-1] = v4mm_AllocUC(V4DPI_DictEntryVal_Max) ;
		    for(j=0;;j++) { if ((lde->uctval[i-1][j] = UCTOUPPER(deltbl[i-1].tval[j])) == '\0') break ; } ;
		  } ;
	         if (UCstrncmp(tsym,lde->uctval[i-1],minmatch) != 0) break ;
	       } ;
	      if (UCstrcmp(tsym,lde->uctval[i]) == 0) break ;		/* It's exact - OK */
	      for(cnt=1;i<SIZEOFDELTBL;cnt++,i++)
	       { if (lde->uctval[i+1] == NULL)
		  { lde->uctval[i+1] = v4mm_AllocUC(V4DPI_DictEntryVal_Max) ;
		    for(j=0;;j++) { if ((lde->uctval[i+1][j] = UCTOUPPER(deltbl[i+1].tval[j])) == '\0') break ; } ;
		  } ;
	         if (i < SIZEOFDELTBL - 1 ? (UCstrncmp(tsym,lde->uctval[i+1],minmatch) != 0) : TRUE) break ;
	       } ;
	      if (cnt == 1) break ;					/* Not ambiguous - OK */
	    } ;
	   if (j < 0) { last = i - 1 ; } else { first = i + 1 ; } ;
	   if (first > last) { i = SIZEOFDELTBL+1 ; break ; } ;
	 } ;
	FREEMTLOCK(ldeLock) ; 
	if (i >= SIZEOFDELTBL) return(-1) ;	/* Not in the table */
	return(deltbl[i].deval) ;
}

/*	v4im_GetTCBtoEnumVal - Grabs next tcb token & converts to master Enum value */
/*	Call: enum = v4im_GetTCBtoEnumVal( ctx , tcb , eolflag )
	  where enum is enum value (-1 if not found, -2 if error),
		ctx is context,
		tcb is current token-control-block,
		eolflag is TRUE to allow for End-of-Line, FALSE to grab next line if currently at EOL */

enum DictionaryEntries v4im_GetTCBtoEnumVal(ctx,tcb,eolflag)
  struct V4C__Context *ctx ;
  struct V4LEX__TknCtrlBlk *tcb ;
  LOGICAL eolflag ;
{
	v4lex_NextTkn(tcb,(eolflag  ? V4LEX_Option_RetEOL : 0)) ; /* Grab next token */
	if (tcb->type == V4LEX_TknType_Error) return(-2) ;
//UCprintf(UClit("***%s* *%s*\n"),tcb->UCkeyword,tcb->ilvl[tcb->ilx].input_ptr) ;
	if (tcb->type == V4LEX_TknType_EOL) return(_EndOfLine_) ;
	switch (tcb->opcode)
	 {
	   case V_OpCode_EOF:		exit(EXITOK) ;
	   case V_OpCode_Comma:		return(_Comma_) ;
	   case V_OpCode_NCLBracket:	return(_LBracket_) ;
	   case V_OpCode_LBracket:	return(_LBracket_) ;
	   case V_OpCode_Equal:		return(_EqualSign_) ;
	   case V_OpCode_LBraceSlash:	return(_LBraceSlash_) ;
	   case V_OpCode_Semi:		return(_SemiColon_) ;
	 } ;
	if (tcb->actual_keyword_len >= UCsizeof(tcb->UCkeyword)) return(-2) ;
	return(v4im_GetUCStrToEnumVal(tcb->UCkeyword,tcb->keyword_len)) ;
}


/*	v4im_GetEnumToDictVal - Converts enum value to V4 dictionary point internal value	*/
/*	Call: intval = v4im_GetEnumToDictVal( ctx , deval , dimid )
	  where intval is V4 internal dictionary value,
		ctx is context,
		deval is enum value,
		dimid is dimension resulting point will belong to (ex: Dim_NId, Dim_UV4) or UNUSED to lookup but not auto-create	*/

int v4im_GetEnumToDictVal(ctx,deval,dimid)
  struct V4C__Context *ctx ;
  enum DictionaryEntries deval ;
  int dimid ;
{ struct V4DPI__DimInfo *di ;
  int intval ;

	if (dimid == UNUSED) { intval = v4dpi_DictEntryGet(ctx,UNUSED,deltbl[deval].tval,NULL,NULL) ; }
	 else { DIMINFO(di,ctx,dimid) ;	intval = v4dpi_DictEntryGet(ctx,dimid,deltbl[deval].tval,di,NULL) ; }
	return(intval) ;
}

/*	v4im_GetEnumToUCVal - Converts enum value to UC text value	*/
/*	Call: ucstr = v4im_GetEnumToUCVal( deval )
	  where ucstr is pointer to Unicode string,
		deval is enum value					*/

UCCHAR *v4im_GetEnumToUCVal(deval)
  enum DictionaryEntries deval ;
{
	return(deltbl[deval].tval) ;
}

/*	v_EncodeDecode - Encodes / Decodes string based on key					*/
/*	Call: len = v_EncodeDecode(dataBuf , dataLen , dataMax , edMethod , doEncode , keyBuf , keyLen , errMsg )
	  where len is length of encrypted string (may be greater than dataLen if padding is necessary), UNUSED if error (in errMsg),
		dataBuf is pointer to buffer to be encoded (IN PLACE!),
		dataLen is length of data,
		dataMax is maximum size of dataBuf in case have to pad with nulls (DES encrypts in 8 byte increments),
		edMethod is type of coding, see V4_EDMETHOD_xxx,
		doEncode is TRUE to encode, FALSE to decode,
		keyBuf is pointer to key,
		keyLen is pointer to key,
		errMsg is updated with error in event of failure				*/

LENMAX v_EncodeDecode(dataBuf,dataLen,dataMax,edMethod,doEncode,keyBuf,keyLen,errMsg)
  BYTE *dataBuf ; LENMAX dataLen,dataMax ;
  ETYPE edMethod ;
  LOGICAL doEncode ;
  BYTE *keyBuf ; LENMAX keyLen ;
  UCCHAR *errMsg ;
{
  INDEX i ;
	if (dataLen == 0) return (0) ;		/* Not much to do */

	switch (edMethod)
	 { default:
	   case V4_EDMETHOD_DES:
	      { LENMAX numBlocks ; struct VED__desKeySched ks ; struct VED__desKeyBlock tkey ;
		if (keyLen <= sizeof (struct VED__desKeyBlock))
		 { memset(&tkey,0,sizeof tkey) ; memcpy(&tkey,keyBuf,keyLen) ; }
		 else { v_Msg(NULL,errMsg,"EnDeDESKey",keyLen,sizeof (struct VED__desKeyBlock)) ; goto fail ; } ;
		numBlocks = dataLen / 8 ;
		if ((numBlocks * 8) != dataLen)			/*  Need whole number of blocks   */
		 { if (dataMax < numBlocks * 8) { v_Msg(NULL,errMsg,"EnDeDataMax",dataMax,numBlocks * 8) ; goto fail ; } ;
		   for(i=dataLen;i<numBlocks * 8;i++) { dataBuf[i] = '\0' ; } ;
		   dataLen = (numBlocks + 1) * 8 ;
		 } ;
		vdes_SetKey((struct VED__desKeyBlock *) keyBuf, (struct VED__desKeySched *) &ks) ;
		for (i=0;i<dataLen;i+= 8)
		 { vdes_ECBEncrypt ((struct VED__desKeyBlock *) (&dataBuf[i]),(struct VED__desKeyBlock *) (&dataBuf[i]),(struct VED__desKeySched *) &ks, doEncode) ; } ;
	      }
	      break ;
	   case V4_EDMETHOD_XOR:
	      for(i=0;i<dataLen;i++)
	       { dataBuf[i] ^= keyBuf[i%keyLen] ; } ;		/* Just XOR with 'next' byte in key */
	      break ;
	 }
	return (dataLen) ;                      /*  So far so good                   */
fail:
	return(UNUSED) ;
}

#define DESCTOL(c,l) \
 (l  = ((UWORD) (*((c)++))), l |= ((UWORD) (*((c)++))) <<  8, l |= ((UWORD) (*((c)++))) << 16, l |= ((UWORD) (*((c)++))) << 24)
#define DESLTOC(l,c) \
 (*((c)++) = (BYTE) (((l)) & 0xff), *((c)++) = (BYTE) (((l) >> 8) & 0xff), *((c)++) = (BYTE) (((l) >>16) & 0xff), *((c)++) = (BYTE) (((l) >>24) & 0xff))

#define DESENCRYPT(L,R,S) \
 u = (R^s [S]) ; \
 t =  R^s [S + 1] ; \
 t = ((t >> 4) + (t << 28)) ;           \
 L ^=	vdes_SPTransData[1][(U16INT) (t    ) & 0x3f]| \
	vdes_SPTransData[3][(U16INT) (t>> 8) & 0x3f]| \
	vdes_SPTransData[5][(U16INT) (t>>16) & 0x3f]| \
	vdes_SPTransData[7][(U16INT) (t>>24) & 0x3f]| \
	vdes_SPTransData[0][(U16INT) (u    ) & 0x3f]| \
	vdes_SPTransData[2][(U16INT) (u>> 8) & 0x3f]| \
	vdes_SPTransData[4][(U16INT) (u>>16) & 0x3f]| \
	vdes_SPTransData[6][(U16INT) (u>>24) & 0x3f];

#define DESPERMUTE(a,b,t,n,m) ((t)  = ((((a) >> (n)) ^ (b)) & (m)),(b) ^= (t), (a) ^= ((t) << (n)))
#define DESHPERMUTE(a,t,n,m) ((t) = ((((a) << (16 - (n))) ^ (a)) & (m)), (a) = (a) ^ (t) ^ (t >> (16 - (n))))

UWORD vdes_SPTransData[8][64] = {
  {	0x00820200L, 0x00020000L, 0x80800000L, 0x80820200L,
	0x00800000L, 0x80020200L, 0x80020000L, 0x80800000L,
	0x80020200L, 0x00820200L, 0x00820000L, 0x80000200L,
	0x80800200L, 0x00800000L, 0x00000000L, 0x80020000L,
	0x00020000L, 0x80000000L, 0x00800200L, 0x00020200L,
	0x80820200L, 0x00820000L, 0x80000200L, 0x00800200L,
	0x80000000L, 0x00000200L, 0x00020200L, 0x80820000L,
	0x00000200L, 0x80800200L, 0x80820000L, 0x00000000L,
	0x00000000L, 0x80820200L, 0x00800200L, 0x80020000L,
	0x00820200L, 0x00020000L, 0x80000200L, 0x00800200L,
	0x80820000L, 0x00000200L, 0x00020200L, 0x80800000L,
	0x80020200L, 0x80000000L, 0x80800000L, 0x00820000L,
	0x80820200L, 0x00020200L, 0x00820000L, 0x80800200L,
	0x00800000L, 0x80000200L, 0x80020000L, 0x00000000L,
	0x00020000L, 0x00800000L, 0x80800200L, 0x00820200L,
	0x80000000L, 0x80820000L, 0x00000200L, 0x80020200L },

  {	0x10042004L, 0x00000000L, 0x00042000L, 0x10040000L,
	0x10000004L, 0x00002004L, 0x10002000L, 0x00042000L,
	0x00002000L, 0x10040004L, 0x00000004L, 0x10002000L,
	0x00040004L, 0x10042000L, 0x10040000L, 0x00000004L,
	0x00040000L, 0x10002004L, 0x10040004L, 0x00002000L,
	0x00042004L, 0x10000000L, 0x00000000L, 0x00040004L,
	0x10002004L, 0x00042004L, 0x10042000L, 0x10000004L,
	0x10000000L, 0x00040000L, 0x00002004L, 0x10042004L,
	0x00040004L, 0x10042000L, 0x10002000L, 0x00042004L,
	0x10042004L, 0x00040004L, 0x10000004L, 0x00000000L,
	0x10000000L, 0x00002004L, 0x00040000L, 0x10040004L,
	0x00002000L, 0x10000000L, 0x00042004L, 0x10002004L,
	0x10042000L, 0x00002000L, 0x00000000L, 0x10000004L,
	0x00000004L, 0x10042004L, 0x00042000L, 0x10040000L,
	0x10040004L, 0x00040000L, 0x00002004L, 0x10002000L,
	0x10002004L, 0x00000004L, 0x10040000L, 0x00042000L },

  {	0x41000000L, 0x01010040L, 0x00000040L, 0x41000040L,
	0x40010000L, 0x01000000L, 0x41000040L, 0x00010040L,
	0x01000040L, 0x00010000L, 0x01010000L, 0x40000000L,
	0x41010040L, 0x40000040L, 0x40000000L, 0x41010000L,
	0x00000000L, 0x40010000L, 0x01010040L, 0x00000040L,
	0x40000040L, 0x41010040L, 0x00010000L, 0x41000000L,
	0x41010000L, 0x01000040L, 0x40010040L, 0x01010000L,
	0x00010040L, 0x00000000L, 0x01000000L, 0x40010040L,
	0x01010040L, 0x00000040L, 0x40000000L, 0x00010000L,
	0x40000040L, 0x40010000L, 0x01010000L, 0x41000040L,
	0x00000000L, 0x01010040L, 0x00010040L, 0x41010000L,
	0x40010000L, 0x01000000L, 0x41010040L, 0x40000000L,
	0x40010040L, 0x41000000L, 0x01000000L, 0x41010040L,
	0x00010000L, 0x01000040L, 0x41000040L, 0x00010040L,
	0x01000040L, 0x00000000L, 0x41010000L, 0x40000040L,
	0x41000000L, 0x40010040L, 0x00000040L, 0x01010000L },

  {	0x00100402L, 0x04000400L, 0x00000002L, 0x04100402L,
	0x00000000L, 0x04100000L, 0x04000402L, 0x00100002L,
	0x04100400L, 0x04000002L, 0x04000000L, 0x00000402L,
	0x04000002L, 0x00100402L, 0x00100000L, 0x04000000L,
	0x04100002L, 0x00100400L, 0x00000400L, 0x00000002L,
	0x00100400L, 0x04000402L, 0x04100000L, 0x00000400L,
	0x00000402L, 0x00000000L, 0x00100002L, 0x04100400L,
	0x04000400L, 0x04100002L, 0x04100402L, 0x00100000L,
	0x04100002L, 0x00000402L, 0x00100000L, 0x04000002L,
	0x00100400L, 0x04000400L, 0x00000002L, 0x04100000L,
	0x04000402L, 0x00000000L, 0x00000400L, 0x00100002L,
	0x00000000L, 0x04100002L, 0x04100400L, 0x00000400L,
	0x04000000L, 0x04100402L, 0x00100402L, 0x00100000L,
	0x04100402L, 0x00000002L, 0x04000400L, 0x00100402L,
	0x00100002L, 0x00100400L, 0x04100000L, 0x04000402L,
	0x00000402L, 0x04000000L, 0x04000002L, 0x04100400L },

  {	0x02000000L, 0x00004000L, 0x00000100L, 0x02004108L,
	0x02004008L, 0x02000100L, 0x00004108L, 0x02004000L,
	0x00004000L, 0x00000008L, 0x02000008L, 0x00004100L,
	0x02000108L, 0x02004008L, 0x02004100L, 0x00000000L,
	0x00004100L, 0x02000000L, 0x00004008L, 0x00000108L,
	0x02000100L, 0x00004108L, 0x00000000L, 0x02000008L,
	0x00000008L, 0x02000108L, 0x02004108L, 0x00004008L,
	0x02004000L, 0x00000100L, 0x00000108L, 0x02004100L,
	0x02004100L, 0x02000108L, 0x00004008L, 0x02004000L,
	0x00004000L, 0x00000008L, 0x02000008L, 0x02000100L,
	0x02000000L, 0x00004100L, 0x02004108L, 0x00000000L,
	0x00004108L, 0x02000000L, 0x00000100L, 0x00004008L,
	0x02000108L, 0x00000100L, 0x00000000L, 0x02004108L,
	0x02004008L, 0x02004100L, 0x00000108L, 0x00004000L,
	0x00004100L, 0x02004008L, 0x02000100L, 0x00000108L,
	0x00000008L, 0x00004108L, 0x02004000L, 0x02000008L },

  {	0x20000010L, 0x00080010L, 0x00000000L, 0x20080800L,
	0x00080010L, 0x00000800L, 0x20000810L, 0x00080000L,
	0x00000810L, 0x20080810L, 0x00080800L, 0x20000000L,
	0x20000800L, 0x20000010L, 0x20080000L, 0x00080810L,
	0x00080000L, 0x20000810L, 0x20080010L, 0x00000000L,
	0x00000800L, 0x00000010L, 0x20080800L, 0x20080010L,
	0x20080810L, 0x20080000L, 0x20000000L, 0x00000810L,
	0x00000010L, 0x00080800L, 0x00080810L, 0x20000800L,
	0x00000810L, 0x20000000L, 0x20000800L, 0x00080810L,
	0x20080800L, 0x00080010L, 0x00000000L, 0x20000800L,
	0x20000000L, 0x00000800L, 0x20080010L, 0x00080000L,
	0x00080010L, 0x20080810L, 0x00080800L, 0x00000010L,
	0x20080810L, 0x00080800L, 0x00080000L, 0x20000810L,
	0x20000010L, 0x20080000L, 0x00080810L, 0x00000000L,
	0x00000800L, 0x20000010L, 0x20000810L, 0x20080800L,
	0x20080000L, 0x00000810L, 0x00000010L, 0x20080010L },

  {	0x00001000L, 0x00000080L, 0x00400080L, 0x00400001L,
	0x00401081L, 0x00001001L, 0x00001080L, 0x00000000L,
	0x00400000L, 0x00400081L, 0x00000081L, 0x00401000L,
	0x00000001L, 0x00401080L, 0x00401000L, 0x00000081L,
	0x00400081L, 0x00001000L, 0x00001001L, 0x00401081L,
	0x00000000L, 0x00400080L, 0x00400001L, 0x00001080L,
	0x00401001L, 0x00001081L, 0x00401080L, 0x00000001L,
	0x00001081L, 0x00401001L, 0x00000080L, 0x00400000L,
	0x00001081L, 0x00401000L, 0x00401001L, 0x00000081L,
	0x00001000L, 0x00000080L, 0x00400000L, 0x00401001L,
	0x00400081L, 0x00001081L, 0x00001080L, 0x00000000L,
	0x00000080L, 0x00400001L, 0x00000001L, 0x00400080L,
	0x00000000L, 0x00400081L, 0x00400080L, 0x00001080L,
	0x00000081L, 0x00001000L, 0x00401081L, 0x00400000L,
	0x00401080L, 0x00000001L, 0x00001001L, 0x00401081L,
	0x00400001L, 0x00401080L, 0x00401000L, 0x00001001L },

  {	0x08200020L, 0x08208000L, 0x00008020L, 0x00000000L,
	0x08008000L, 0x00200020L, 0x08200000L, 0x08208020L,
	0x00000020L, 0x08000000L, 0x00208000L, 0x00008020L,
	0x00208020L, 0x08008020L, 0x08000020L, 0x08200000L,
	0x00008000L, 0x00208020L, 0x00200020L, 0x08008000L,
	0x08208020L, 0x08000020L, 0x00000000L, 0x00208000L,
	0x08000000L, 0x00200000L, 0x08008020L, 0x08200020L,
	0x00200000L, 0x00008000L, 0x08208000L, 0x00000020L,
	0x00200000L, 0x00008000L, 0x08000020L, 0x08208020L,
	0x00008020L, 0x08000000L, 0x00000000L, 0x00208000L,
	0x08200020L, 0x08008020L, 0x08008000L, 0x00200020L,
	0x08208000L, 0x00000020L, 0x00200020L, 0x08008000L,
	0x08208020L, 0x00200000L, 0x08200000L, 0x08000020L,
	0x00208000L, 0x00008020L, 0x08008020L, 0x08200000L,
	0x00000020L, 0x08208000L, 0x00208020L, 0x00000000L,
	0x08000000L, 0x08200020L, 0x00008000L, 0x00208020L } };

UWORD des_skb [8][64] = {
  {	0x00000000L, 0x00000010L, 0x20000000L, 0x20000010L,
	0x00010000L, 0x00010010L, 0x20010000L, 0x20010010L,
	0x00000800L, 0x00000810L, 0x20000800L, 0x20000810L,
	0x00010800L, 0x00010810L, 0x20010800L, 0x20010810L,
	0x00000020L, 0x00000030L, 0x20000020L, 0x20000030L,
	0x00010020L, 0x00010030L, 0x20010020L, 0x20010030L,
	0x00000820L, 0x00000830L, 0x20000820L, 0x20000830L,
	0x00010820L, 0x00010830L, 0x20010820L, 0x20010830L,
	0x00080000L, 0x00080010L, 0x20080000L, 0x20080010L,
	0x00090000L, 0x00090010L, 0x20090000L, 0x20090010L,
	0x00080800L, 0x00080810L, 0x20080800L, 0x20080810L,
	0x00090800L, 0x00090810L, 0x20090800L, 0x20090810L,
	0x00080020L, 0x00080030L, 0x20080020L, 0x20080030L,
	0x00090020L, 0x00090030L, 0x20090020L, 0x20090030L,
	0x00080820L, 0x00080830L, 0x20080820L, 0x20080830L,
	0x00090820L, 0x00090830L, 0x20090820L, 0x20090830L },

  {	0x00000000L, 0x02000000L, 0x00002000L, 0x02002000L,
	0x00200000L, 0x02200000L, 0x00202000L, 0x02202000L,
	0x00000004L, 0x02000004L, 0x00002004L, 0x02002004L,
	0x00200004L, 0x02200004L, 0x00202004L, 0x02202004L,
	0x00000400L, 0x02000400L, 0x00002400L, 0x02002400L,
	0x00200400L, 0x02200400L, 0x00202400L, 0x02202400L,
	0x00000404L, 0x02000404L, 0x00002404L, 0x02002404L,
	0x00200404L, 0x02200404L, 0x00202404L, 0x02202404L,
	0x10000000L, 0x12000000L, 0x10002000L, 0x12002000L,
	0x10200000L, 0x12200000L, 0x10202000L, 0x12202000L,
	0x10000004L, 0x12000004L, 0x10002004L, 0x12002004L,
	0x10200004L, 0x12200004L, 0x10202004L, 0x12202004L,
	0x10000400L, 0x12000400L, 0x10002400L, 0x12002400L,
	0x10200400L, 0x12200400L, 0x10202400L, 0x12202400L,
	0x10000404L, 0x12000404L, 0x10002404L, 0x12002404L,
	0x10200404L, 0x12200404L, 0x10202404L, 0x12202404L },

  {	0x00000000L, 0x00000001L, 0x00040000L, 0x00040001L,
	0x01000000L, 0x01000001L, 0x01040000L, 0x01040001L,
	0x00000002L, 0x00000003L, 0x00040002L, 0x00040003L,
	0x01000002L, 0x01000003L, 0x01040002L, 0x01040003L,
	0x00000200L, 0x00000201L, 0x00040200L, 0x00040201L,
	0x01000200L, 0x01000201L, 0x01040200L, 0x01040201L,
	0x00000202L, 0x00000203L, 0x00040202L, 0x00040203L,
	0x01000202L, 0x01000203L, 0x01040202L, 0x01040203L,
	0x08000000L, 0x08000001L, 0x08040000L, 0x08040001L,
	0x09000000L, 0x09000001L, 0x09040000L, 0x09040001L,
	0x08000002L, 0x08000003L, 0x08040002L, 0x08040003L,
	0x09000002L, 0x09000003L, 0x09040002L, 0x09040003L,
	0x08000200L, 0x08000201L, 0x08040200L, 0x08040201L,
	0x09000200L, 0x09000201L, 0x09040200L, 0x09040201L,
	0x08000202L, 0x08000203L, 0x08040202L, 0x08040203L,
	0x09000202L, 0x09000203L, 0x09040202L, 0x09040203L },

  {	0x00000000L, 0x00100000L, 0x00000100L, 0x00100100L,
	0x00000008L, 0x00100008L, 0x00000108L, 0x00100108L,
	0x00001000L, 0x00101000L, 0x00001100L, 0x00101100L,
	0x00001008L, 0x00101008L, 0x00001108L, 0x00101108L,
	0x04000000L, 0x04100000L, 0x04000100L, 0x04100100L,
	0x04000008L, 0x04100008L, 0x04000108L, 0x04100108L,
	0x04001000L, 0x04101000L, 0x04001100L, 0x04101100L,
	0x04001008L, 0x04101008L, 0x04001108L, 0x04101108L,
	0x00020000L, 0x00120000L, 0x00020100L, 0x00120100L,
	0x00020008L, 0x00120008L, 0x00020108L, 0x00120108L,
	0x00021000L, 0x00121000L, 0x00021100L, 0x00121100L,
	0x00021008L, 0x00121008L, 0x00021108L, 0x00121108L,
	0x04020000L, 0x04120000L, 0x04020100L, 0x04120100L,
	0x04020008L, 0x04120008L, 0x04020108L, 0x04120108L,
	0x04021000L, 0x04121000L, 0x04021100L, 0x04121100L,
	0x04021008L, 0x04121008L, 0x04021108L, 0x04121108L },

  {	0x00000000L, 0x10000000L, 0x00010000L, 0x10010000L,
	0x00000004L, 0x10000004L, 0x00010004L, 0x10010004L,
	0x20000000L, 0x30000000L, 0x20010000L, 0x30010000L,
	0x20000004L, 0x30000004L, 0x20010004L, 0x30010004L,
	0x00100000L, 0x10100000L, 0x00110000L, 0x10110000L,
	0x00100004L, 0x10100004L, 0x00110004L, 0x10110004L,
	0x20100000L, 0x30100000L, 0x20110000L, 0x30110000L,
	0x20100004L, 0x30100004L, 0x20110004L, 0x30110004L,
	0x00001000L, 0x10001000L, 0x00011000L, 0x10011000L,
	0x00001004L, 0x10001004L, 0x00011004L, 0x10011004L,
	0x20001000L, 0x30001000L, 0x20011000L, 0x30011000L,
	0x20001004L, 0x30001004L, 0x20011004L, 0x30011004L,
	0x00101000L, 0x10101000L, 0x00111000L, 0x10111000L,
	0x00101004L, 0x10101004L, 0x00111004L, 0x10111004L,
	0x20101000L, 0x30101000L, 0x20111000L, 0x30111000L,
	0x20101004L, 0x30101004L, 0x20111004L, 0x30111004L },

  {	0x00000000L, 0x08000000L, 0x00000008L, 0x08000008L,
	0x00000400L, 0x08000400L, 0x00000408L, 0x08000408L,
	0x00020000L, 0x08020000L, 0x00020008L, 0x08020008L,
	0x00020400L, 0x08020400L, 0x00020408L, 0x08020408L,
	0x00000001L, 0x08000001L, 0x00000009L, 0x08000009L,
	0x00000401L, 0x08000401L, 0x00000409L, 0x08000409L,
	0x00020001L, 0x08020001L, 0x00020009L, 0x08020009L,
	0x00020401L, 0x08020401L, 0x00020409L, 0x08020409L,
	0x02000000L, 0x0A000000L, 0x02000008L, 0x0A000008L,
	0x02000400L, 0x0A000400L, 0x02000408L, 0x0A000408L,
	0x02020000L, 0x0A020000L, 0x02020008L, 0x0A020008L,
	0x02020400L, 0x0A020400L, 0x02020408L, 0x0A020408L,
	0x02000001L, 0x0A000001L, 0x02000009L, 0x0A000009L,
	0x02000401L, 0x0A000401L, 0x02000409L, 0x0A000409L,
	0x02020001L, 0x0A020001L, 0x02020009L, 0x0A020009L,
	0x02020401L, 0x0A020401L, 0x02020409L, 0x0A020409L },

  {	0x00000000L, 0x00000100L, 0x00080000L, 0x00080100L,
	0x01000000L, 0x01000100L, 0x01080000L, 0x01080100L,
	0x00000010L, 0x00000110L, 0x00080010L, 0x00080110L,
	0x01000010L, 0x01000110L, 0x01080010L, 0x01080110L,
	0x00200000L, 0x00200100L, 0x00280000L, 0x00280100L,
	0x01200000L, 0x01200100L, 0x01280000L, 0x01280100L,
	0x00200010L, 0x00200110L, 0x00280010L, 0x00280110L,
	0x01200010L, 0x01200110L, 0x01280010L, 0x01280110L,
	0x00000200L, 0x00000300L, 0x00080200L, 0x00080300L,
	0x01000200L, 0x01000300L, 0x01080200L, 0x01080300L,
	0x00000210L, 0x00000310L, 0x00080210L, 0x00080310L,
	0x01000210L, 0x01000310L, 0x01080210L, 0x01080310L,
	0x00200200L, 0x00200300L, 0x00280200L, 0x00280300L,
	0x01200200L, 0x01200300L, 0x01280200L, 0x01280300L,
	0x00200210L, 0x00200310L, 0x00280210L, 0x00280310L,
	0x01200210L, 0x01200310L, 0x01280210L, 0x01280310L },

  {	0x00000000L, 0x04000000L, 0x00040000L, 0x04040000L,
	0x00000002L, 0x04000002L, 0x00040002L, 0x04040002L,
	0x00002000L, 0x04002000L, 0x00042000L, 0x04042000L,
	0x00002002L, 0x04002002L, 0x00042002L, 0x04042002L,
	0x00000020L, 0x04000020L, 0x00040020L, 0x04040020L,
	0x00000022L, 0x04000022L, 0x00040022L, 0x04040022L,
	0x00002020L, 0x04002020L, 0x00042020L, 0x04042020L,
	0x00002022L, 0x04002022L, 0x00042022L, 0x04042022L,
	0x00000800L, 0x04000800L, 0x00040800L, 0x04040800L,
	0x00000802L, 0x04000802L, 0x00040802L, 0x04040802L,
	0x00002800L, 0x04002800L, 0x00042800L, 0x04042800L,
	0x00002802L, 0x04002802L, 0x00042802L, 0x04042802L,
	0x00000820L, 0x04000820L, 0x00040820L, 0x04040820L,
	0x00000822L, 0x04000822L, 0x00040822L, 0x04040822L,
	0x00002820L, 0x04002820L, 0x00042820L, 0x04042820L,
	0x00002822L, 0x04002822L, 0x00042822L, 0x04042822L } };



int vdes_ECBEncrypt (struct VED__desKeyBlock *input, struct VED__desKeyBlock *output,
                 struct VED__desKeySched *ks, int encrypt)
{
  static UWORD l0, l1, ll[2] ;
  BYTE *in, *out;

	in  = (BYTE *)input ; out = (BYTE *)output ;
	DESCTOL(in,l0) ; DESCTOL(in,l1) ;
	ll[0] = l0 ; ll[1] = l1;
	vdes_Encrypt((UWORD *) ll, (UWORD *) ll, ks, encrypt) ;
	l0 = ll[0] ; l1 = ll[1] ;
	DESLTOC(l0, out) ; DESLTOC(l1, out) ;
	return (0) ;
}


int vdes_Encrypt (UWORD *input, UWORD *output, struct VED__desKeySched *ks, int encrypt)
{
  static UWORD l, r, *s, t, u ;
  int i ;

	l = input [0] ; r = input [1] ;

	DESPERMUTE(r,l,t,4 ,0x0f0f0f0fL) ;
	DESPERMUTE(l,r,t,16,0x0000ffffL) ;
	DESPERMUTE(r,l,t,2 ,0x33333333L) ;
	DESPERMUTE(l,r,t,8 ,0x00ff00ffL) ;
	DESPERMUTE(r,l,t,1 ,0x55555555L) ;

	t = (r << 1)|(r >> 31) ;
	r = (l << 1)|(l >> 31) ;
	l = t;

	l &= 0xffffffffL ; r &= 0xffffffffL;

	s = (UWORD *) ks;

	if (encrypt)
	 { for (i = 0; i < 32; i += 4) { DESENCRYPT (l, r, i + 0) ; DESENCRYPT (r, l, i + 2) ; } ;
	 } else
	 { for (i = 30; i > 0; i -= 4)
	    { DESENCRYPT (l, r, i - 0) ; DESENCRYPT (r, l, i - 2) ; } ;
	 } ;
	l = (l >> 1)|(l << 31) ; r = (r >> 1)|(r << 31) ;

	l &= 0xffffffffL ; r &= 0xffffffffL ;

	DESPERMUTE(r,l,t,1 ,0x55555555L) ; DESPERMUTE(l,r,t,8 ,0x00ff00ffL) ;
	DESPERMUTE(r,l,t,2 ,0x33333333L) ; DESPERMUTE(l,r,t,16,0x0000ffffL) ;
	DESPERMUTE(r,l,t,4 ,0x0f0f0f0fL) ;
	output[0] = l ;	output[1] = r ;

	return(0) ;
}

int vdes_SetKey (struct VED__desKeyBlock *key, struct VED__desKeySched *schedule)
{
  static UWORD c, d, t, s, *k ;
  static BYTE *in ;
  INDEX i ;
  static char shifts2 [16] = {0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0} ;

	k  = (UWORD *)schedule ;
	in = (BYTE *)key ;

	DESCTOL(in, c) ; DESCTOL(in, d) ;

	DESPERMUTE(d,c,t ,4,0x0f0f0f0fL) ;
	DESHPERMUTE(c,t,-2,0xcccc0000L) ;
	DESHPERMUTE(d,t,-2,0xcccc0000L) ;
	DESPERMUTE(d,c,t ,1,0x55555555L) ;
	DESPERMUTE(c,d,t ,8,0x00ff00ffL) ;
	DESPERMUTE(d,c,t ,1,0x55555555L) ;
	d =    (((d & 0x000000ffL) << 16)| (d & 0x0000ff00L)      |
	        ((d & 0x00ff0000L) >> 16)|((c & 0xf0000000L) >> 4)) ;
	c &= 0x0fffffffL;

	for (i=0;i<16;i++)
	  {
	    if (shifts2 [i])
	      { c = ((c >> 2) | (c << 26)) ; d = ((d >> 2) | (d << 26)) ; }
	    else { c = ((c >> 1) | (c << 27)) ; d = ((d >> 1) | (d << 27)) ; } ;
	    c &= 0x0fffffffL ; d &= 0x0fffffffL ;
	    s = des_skb [0] [(U16INT) ((c)        & 0x3f                      )] |
	        des_skb [1] [(U16INT) (((c >>  6) & 0x03) | ((c >>  7) & 0x3c))] |
	        des_skb [2] [(U16INT) (((c >> 13) & 0x0f) | ((c >> 14) & 0x30))] |
	        des_skb [3] [(U16INT) (((c >> 20) & 0x01) | ((c >> 21) & 0x06)   |
	                                                  ((c >> 22) & 0x38))];
	    t = des_skb [4] [(U16INT) (((d)       & 0x3f)                     )] |
	        des_skb [5] [(U16INT) (((d >>  7) & 0x03) | ((d >>  8) & 0x3c))] |
	        des_skb [6] [(U16INT) (((d >> 15) & 0x3f)                     )] |
	        des_skb [7] [(U16INT) (((d >> 21) & 0x0f) | ((d >> 22) & 0x30))];
	    *(k++) = ((t << 16) | (s & 0x0000ffffL)) & 0xffffffffL;
	    s = ((s >> 16) | (t & 0xffff0000L)) ;
	    s =  (s << 4)  | (s >> 28) ;
	    *(k++) = s & 0xffffffffL;
	  } ;
	return(0) ;
}

static UWORD crcTable [] = {
	0x00000000L, 0x77073096L, 0xEE0E612CL, 0x990951BAL,
	0x076DC419L, 0x706AF48FL, 0xE963A535L, 0x9E6495A3L,
	0x0EDB8832L, 0x79DCB8A4L, 0xE0D5E91EL, 0x97D2D988L,
	0x09B64C2BL, 0x7EB17CBDL, 0xE7B82D07L, 0x90BF1D91L,
	0x1DB71064L, 0x6AB020F2L, 0xF3B97148L, 0x84BE41DEL,
	0x1ADAD47DL, 0x6DDDE4EBL, 0xF4D4B551L, 0x83D385C7L,
	0x136C9856L, 0x646BA8C0L, 0xFD62F97AL, 0x8A65C9ECL,
	0x14015C4FL, 0x63066CD9L, 0xFA0F3D63L, 0x8D080DF5L,
	0x3B6E20C8L, 0x4C69105EL, 0xD56041E4L, 0xA2677172L,
	0x3C03E4D1L, 0x4B04D447L, 0xD20D85FDL, 0xA50AB56BL,
	0x35B5A8FAL, 0x42B2986CL, 0xDBBBC9D6L, 0xACBCF940L,
	0x32D86CE3L, 0x45DF5C75L, 0xDCD60DCFL, 0xABD13D59L,
	0x26D930ACL, 0x51DE003AL, 0xC8D75180L, 0xBFD06116L,
	0x21B4F4B5L, 0x56B3C423L, 0xCFBA9599L, 0xB8BDA50FL,
	0x2802B89EL, 0x5F058808L, 0xC60CD9B2L, 0xB10BE924L,
	0x2F6F7C87L, 0x58684C11L, 0xC1611DABL, 0xB6662D3DL,
	0x76DC4190L, 0x01DB7106L, 0x98D220BCL, 0xEFD5102AL,
	0x71B18589L, 0x06B6B51FL, 0x9FBFE4A5L, 0xE8B8D433L,
	0x7807C9A2L, 0x0F00F934L, 0x9609A88EL, 0xE10E9818L,
	0x7F6A0DBBL, 0x086D3D2DL, 0x91646C97L, 0xE6635C01L,
	0x6B6B51F4L, 0x1C6C6162L, 0x856530D8L, 0xF262004EL,
	0x6C0695EDL, 0x1B01A57BL, 0x8208F4C1L, 0xF50FC457L,
	0x65B0D9C6L, 0x12B7E950L, 0x8BBEB8EAL, 0xFCB9887CL,
	0x62DD1DDFL, 0x15DA2D49L, 0x8CD37CF3L, 0xFBD44C65L,
	0x4DB26158L, 0x3AB551CEL, 0xA3BC0074L, 0xD4BB30E2L,
	0x4ADFA541L, 0x3DD895D7L, 0xA4D1C46DL, 0xD3D6F4FBL,
	0x4369E96AL, 0x346ED9FCL, 0xAD678846L, 0xDA60B8D0L,
	0x44042D73L, 0x33031DE5L, 0xAA0A4C5FL, 0xDD0D7CC9L,
	0x5005713CL, 0x270241AAL, 0xBE0B1010L, 0xC90C2086L,
	0x5768B525L, 0x206F85B3L, 0xB966D409L, 0xCE61E49FL,
	0x5EDEF90EL, 0x29D9C998L, 0xB0D09822L, 0xC7D7A8B4L,
	0x59B33D17L, 0x2EB40D81L, 0xB7BD5C3BL, 0xC0BA6CADL,
	0xEDB88320L, 0x9ABFB3B6L, 0x03B6E20CL, 0x74B1D29AL,
	0xEAD54739L, 0x9DD277AFL, 0x04DB2615L, 0x73DC1683L,
	0xE3630B12L, 0x94643B84L, 0x0D6D6A3EL, 0x7A6A5AA8L,
	0xE40ECF0BL, 0x9309FF9DL, 0x0A00AE27L, 0x7D079EB1L,
	0xF00F9344L, 0x8708A3D2L, 0x1E01F268L, 0x6906C2FEL,
	0xF762575DL, 0x806567CBL, 0x196C3671L, 0x6E6B06E7L,
	0xFED41B76L, 0x89D32BE0L, 0x10DA7A5AL, 0x67DD4ACCL,
	0xF9B9DF6FL, 0x8EBEEFF9L, 0x17B7BE43L, 0x60B08ED5L,
	0xD6D6A3E8L, 0xA1D1937EL, 0x38D8C2C4L, 0x4FDFF252L,
	0xD1BB67F1L, 0xA6BC5767L, 0x3FB506DDL, 0x48B2364BL,
	0xD80D2BDAL, 0xAF0A1B4CL, 0x36034AF6L, 0x41047A60L,
	0xDF60EFC3L, 0xA867DF55L, 0x316E8EEFL, 0x4669BE79L,
	0xCB61B38CL, 0xBC66831AL, 0x256FD2A0L, 0x5268E236L,
	0xCC0C7795L, 0xBB0B4703L, 0x220216B9L, 0x5505262FL,
	0xC5BA3BBEL, 0xB2BD0B28L, 0x2BB45A92L, 0x5CB36A04L,
	0xC2D7FFA7L, 0xB5D0CF31L, 0x2CD99E8BL, 0x5BDEAE1DL,
	0x9B64C2B0L, 0xEC63F226L, 0x756AA39CL, 0x026D930AL,
	0x9C0906A9L, 0xEB0E363FL, 0x72076785L, 0x05005713L,
	0x95BF4A82L, 0xE2B87A14L, 0x7BB12BAEL, 0x0CB61B38L,
	0x92D28E9BL, 0xE5D5BE0DL, 0x7CDCEFB7L, 0x0BDBDF21L,
	0x86D3D2D4L, 0xF1D4E242L, 0x68DDB3F8L, 0x1FDA836EL,
	0x81BE16CDL, 0xF6B9265BL, 0x6FB077E1L, 0x18B74777L,
	0x88085AE6L, 0xFF0F6A70L, 0x66063BCAL, 0x11010B5CL,
	0x8F659EFFL, 0xF862AE69L, 0x616BFFD3L, 0x166CCF45L,
	0xA00AE278L, 0xD70DD2EEL, 0x4E048354L, 0x3903B3C2L,
	0xA7672661L, 0xD06016F7L, 0x4969474DL, 0x3E6E77DBL,
	0xAED16A4AL, 0xD9D65ADCL, 0x40DF0B66L, 0x37D83BF0L,
	0xA9BCAE53L, 0xDEBB9EC5L, 0x47B2CF7FL, 0x30B5FFE9L,
	0xBDBDF21CL, 0xCABAC28AL, 0x53B39330L, 0x24B4A3A6L,
	0xBAD03605L, 0xCDD70693L, 0x54DE5729L, 0x23D967BFL,
	0xB3667A2EL, 0xC4614AB8L, 0x5D681B02L, 0x2A6F2B94L,
	0xB40BBE37L, 0xC30C8EA1L, 0x5A05DF1BL, 0x2D02EF8DL
};

UWORD vcrc_Calculate(dataBuf,dataLen)
  BYTE *dataBuf ;
  LENMAX dataLen ;
{
  LENMAX i ;
  U16INT curShort ;
  UWORD crcRes ;

	crcRes = 0xFFFFFFFFL;
	for (i=0;i<dataLen;i++)
	 { curShort = dataBuf[i];
	   curShort = curShort ^ (U16INT)(crcRes & 255) ;
	   crcRes = (crcRes >> 8) ^ crcTable[curShort];
	 } ;
	return (crcRes ^ 0xFFFFFFFFL) ;
}

/*	Z I P   R O U T I N E S					*/

/*	vzip_InitZipFile - Initialize a new zip file		*/
/*	Call: vzm = vzip_InitZipFile( filename , errbuf )
	  where vzm = newly allocated zip 'master' to be used in subsequent calls (NULL if error),
		filename is output file,
		errbuf is updated with error message		*/

struct VZIP__Master *vzip_InitZipFile(filename,errbuf)
  UCCHAR *filename ;
  UCCHAR *errbuf ;
{
  struct VZIP__Master *vzm ;

	vzm = v4mm_AllocChunk(sizeof *vzm,TRUE) ;
/*	Make sure we can create the file */
	UCstrcpy(vzm->filename,filename) ;
	if (!v_UCFileOpen(&vzm->UCFile,vzm->filename,UCFile_Open_WriteBin,TRUE,errbuf,UNUSED)) return(NULL) ;
	return(vzm) ;
}

/*	vzip_AppendFile - Appends (another) file to specified vzm (zip master)		*/
/*	Call bytes = vzip_AppendFile( vzm , filename , bytes , contents , comments , isText , updateUDT )
	  where bytes is number of bytes of compresses file or UNUSED if error,
		vzm is zip master,
		filename is name of file to appear in zip archive. NOTE FOR WINDOWS: strips off leading device (ex: 'c:') and converts backslashes to slashes,
		bytes is number of bytes to append,
		contents is the contents of the file
		comments is any ASCII comments to be included (NULL if none),
		isText is TRUE if contents is text, FALSE if binary,
		UDT of file last-modify (0 for none)					*/

size_t vzip_AppendFile(vzm,filename,bytes,contents,comments,isText,udt)
  struct VZIP__Master *vzm ;
  UCCHAR *filename ;
  LENMAX bytes ;
  BYTE *contents ;
  char *comments ;
  LOGICAL isText ;
  UDTVAL udt ;
{
  struct VZIP__Entry *vze ;
  ETYPE compression ; LENMAX vzeBytes ; FLAGS32 bits ; char fnBuf[V_FileName_Max] ;

/*	Convert file name to ASCII, normalize if Windows */
#ifdef WINNT
	{ INDEX s,d ;
	  for(s=0,d=0;filename[s]!=UCEOS;s++)
	   { if (filename[s] == '\\') { fnBuf[d++] = '/' ; }
	      else if (filename[s] == ':') { d = 0 ; }
	      else { fnBuf[d++] = filename[s] ; } ;
	   } ; fnBuf[d] = '\0' ;
	}
#else
	UCstrcpyToASC(fnBuf,filename) ;
#endif

/*	Figure out size of this vze */
	vzeBytes = sizeof *vze + strlen(fnBuf)+1 + (comments == NULL ? 0 : strlen(comments)+1) ;
/*	Allocate new vze & insert/append in proper place */
	vze = v4mm_AllocChunk(vzeBytes,TRUE) ;
	if (vzm->vzeFirst == NULL)
	 { vzm->vzeFirst = vze ;
	 } else
	 { struct VZIP__Entry *tvze ;
	   for(tvze=vzm->vzeFirst;;tvze=tvze->vzeNext)
	    { if (tvze->vzeNext == NULL) { tvze->vzeNext = vze ; break ; } ;
	    } ;
	 } ;
	vze->contents = contents ; vze->bytes = bytes ; compression = 0 ; bits = 0 ;
#define ZIPCOMPRESS
#ifdef ZIPCOMPRESS
	{ char *dstBuf ; size_t dstLenMax,dstLen ; UCCHAR errBuf[256] ;
	  dstLenMax = bytes * 1.2 ;
	  dstBuf = v4mm_AllocChunk(dstLenMax,FALSE) ;
	  dstLen = v_zipCompress(contents,bytes,dstBuf,dstLenMax,6,errBuf) ;
//	  bits = 8 ;
	  bits = 0 ;			/* 'unzip -t' accepts 8 or 0, Excel likes it at 0 so let's leave it at that */
	  compression = 8 ;		/* VEH101203 - don't understand why 6 in call above but 8 here, but it works */
	  vze->contents = dstBuf ; vze->bytes = dstLen ;
	}
#endif


/*	Fill in the vze->vcd fields for this entry */
	vze->vcd.vcdSignature = 0x02014b50 ;
	vze->vcd.versionMade = 0 ;
	vze->vcd.versionMin = 0x0A ;
	vze->vcd.bits = bits ;
	vze->vcd.compression = compression ;
	{ int msdt = mscu_udt_to_msdos(udt) ;
	  vze->vcd.fileModTime = (msdt & 0xffff) ;
	  vze->vcd.fileModDate = (msdt >> 16) ;
	}
	vze->vcd.crc32 = vcrc_Calculate(contents,bytes) ;
	vze->vcd.bytesCompressed = vze->bytes ;
	vze->vcd.bytesUncompressed = bytes ;
	vze->vcd.bytesfilename = strlen(fnBuf) ;
	vze->vcd.bytesComments = 0 ;
	vze->vcd.diskNum = 0 ;
	vze->vcd.fileAttr = (isText ? 1 : 0) ;
	vze->vcd.fileAttrExternal = 0 ;
	vze->vcd.offsetFile = UNUSED ;		/* This will be filled in on close of zip file */
	strcpy(vze->vcd.filename,fnBuf) ;
//	vze->vcd.extra
//	vze->vcd.comments
	return(vze->bytes) ;
}

/*	vzip_CloseZipFile - Closes off current zip file				*/
/*	Call: ok = vzip_CloseZipFile( vzm , errbuf )
	  where ok is TRUE if all is well,
		vzm is current zip master,
		errbuf is updated with error message if any problems		*/
		
LOGICAL vzip_CloseZipFile(vzm,errbuf)
  struct VZIP__Master *vzm ;
  UCCHAR *errbuf ;
{
  struct VZIP__Entry *vze,*tvze ;
  struct VZIP__EndCentralDir vec ;
  struct VZIP__zipFileHeader *vzf ;
  LENMAX vecBytes ;
  
/*	Loop thru each file & write out file header & [compressed] contents */
	vzf = (struct VZIP__zipFileHeader *)v4mm_AllocChunk(sizeof *vzf + V_FileName_Max + 1 + VZIP_MaxBytesExtra,FALSE) ;
	for(vze=vzm->vzeFirst;vze!=NULL;vze=vze->vzeNext)
	 { 
	   LENMAX vzfBytes = 0 ;
/*	   Set up file header from central directory file header */
	   memset(vzf,0,sizeof vzf) ; vzf->hdrSignature = 0x04034b50 ;
#define CC(_field) vzf->_field = vze->vcd._field ;
	   CC(versionMin) CC(bits) CC(compression) CC(fileModTime) CC(fileModDate) CC(crc32)
	   CC(bytesCompressed) CC(bytesUncompressed) CC(bytesfilename) CC(bytesExtra)
	   strcpy(vzf->filename,vze->vcd.filename) ; vzfBytes += vzf->bytesfilename ;
	   if (vzf->bytesExtra > 0 && vzf->bytesExtra < VZIP_MaxBytesExtra)
	    { memcpy(&vzf->filename[vzf->bytesfilename],&vze->vcd.filename[vzf->bytesfilename],vzf->bytesExtra) ;
	    } ;
	   vzfBytes = (char *)&vzf->filename[vzf->bytesfilename] - (char *)vzf ;
/*	   Now fill in missing pieces in vze->vcd */
	   vze->vcd.offsetFile = vzm->offsetFile ;
/*	   Write out the header & contents */
	   fwrite(vzf,vzfBytes,1,vzm->UCFile.fp) ;
	   fwrite(vze->contents,vze->bytes,1,vzm->UCFile.fp) ;
	   vzm->offsetFile += vzfBytes + vze->bytes ;
	 } ;
/*	Now write out then central directory entries & fill in end-central-dir structure */
	memset(&vec,0,sizeof vec) ; vec.ecdSignature = 0x06054b50 ;
	vec.diskNum = 0 ; vec.diskNumCentralDir = 0 ; vec.bytesComment = 0 ;
	vec.offsetCentralDir = vzm->offsetFile ;
	for(vze=vzm->vzeFirst;vze!=NULL;vze=vze->vzeNext)
	 { LENMAX vcdBytes ;
	   vcdBytes = sizeof vze->vcd + vze->vcd.bytesfilename + vze->vcd.bytesExtra + vze->vcd.bytesComments ;
	   vec.numCDRThisDisk++ ; vec.totalCDR++ ;
	   vec.bytesCentralDir += vcdBytes ;
	   fwrite(&vze->vcd,vcdBytes,1,vzm->UCFile.fp) ;
	 } ;
/*	End finally the end entry */
	vecBytes = (char *)&vec.comment[vec.bytesComment] - (char *)&vec ;
	fwrite(&vec,vecBytes,1,vzm->UCFile.fp) ;
	v_UCFileClose(&vzm->UCFile) ;
/*	Now free up everything */
	for(vze=vzm->vzeFirst;vze!=NULL;vze=tvze)
	 { tvze = vze->vzeNext ;
	   v4mm_FreeChunk(vze) ;
	 } ;
	v4mm_FreeChunk(vzm) ; v4mm_FreeChunk(vzf) ;
	return(TRUE) ;
}

#ifdef ZIPCOMPRESS
#include "v4zip.c"
#endif

/*	Dummy up routines if we are including vcommon in something other than V4 (ex: vsort) */
#ifdef INCLUDE_VCOMMON
void v4ctx_AreaClose(struct V4C__Context *ctx,int areaid) {}
void v4dpi_IsctEvalCacheClear(struct V4C__Context *ctx,LOGICAL shutdown) {}
void v4ctx_FrameCacheClear(struct V4C__Context *ctx,LOGICAL shutdown) {}
void v4mm_MemMgmtShutdown(struct V4MM__MemMgmtMaster *mmm) {}
char *v4dpi_GetBindBuf(struct V4C__Context *ctx,int aid,struct V4IS__IndexKeyDataEntry  *ikde,LOGICAL cache,int *updbytes) { return(NULL) ; }
void v4dpi_PntIdx_Shutdown() {}
int v4dpi_XDictEntryGet(struct V4C__Context *ctx,UCCHAR *entry,struct V4DPI__DimInfo *di,int flg) { return(UNUSED) ;}
struct V4DPI__Point *v4dpi_IsctEval(struct V4DPI__Point *pt,struct V4DPI__Point *isct,struct V4C__Context *ctx,int int2,int *int3,struct V4DPI__EvalList *el) { return(NULL) ; }
LOGICAL v4dpi_PointParse(struct V4C__Context *ctx,struct V4DPI__Point *pnt,struct V4LEX__TknCtrlBlk *tkn,int flags) { return(FALSE) ; }
int v4eval_Eval(struct V4LEX__TknCtrlBlk *tcb,struct V4C__Context *ctx,LOGICAL log1,int int1,LOGICAL log2,LOGICAL log3,LOGICAL log4,struct V4DPI__Point *respnt) { return(UNUSED) ; }
//struct V4DPI__Point *v4dpi_PntIdx_CvtIdxPtr() {}
//v4dpi_PntIdx_AllocPnt() {}
int v4dpi_DictEntryGet(struct V4C__Context *ctx,int int1,UCCHAR *entry,struct V4DPI__DimInfo *di,int *int2) { return(UNUSED) ; }
void v4_error(int errnum,struct V4LEX__TknCtrlBlk *tcb,char *subsys,char *module,char *mne,char *msg,...) {}
UCCHAR *v4im_Display(int intmodx) { return(NULL) ; }
UCCHAR *v4im_ModFailStr(int intmodx) { return(NULL) ; }
UCCHAR *v4im_PTName(int int1) { return(NULL) ; }
struct V4DPI__DimInfo *v4dpi_DimInfoGet(struct V4C__Context *ctx,int dimid) { return(NULL) ; }
UCCHAR *v4im_TagName(int int1) { return(NULL) ; }
UCCHAR *v4im_LastTagName() { return(NULL) ; }
UCCHAR *v4dpi_PointToString(UCCHAR *dst,struct V4DPI__Point *pnt,struct V4C__Context *ctx,int flags) { return(NULL) ; }
struct V4DPI__Point *v4ctx_DimValue(struct V4C__Context *ctx,int int1,struct V4DPI__DimInfo **di) { return(NULL) ; }
int v4im_GetPointLog(LOGICAL *ok,struct V4DPI__Point *pnt,struct V4C__Context *ctx) { return(UNUSED) ; }
int v4im_GetPointChar(LOGICAL *ok,char *dst,int max,struct V4DPI__Point *p,struct V4C__Context *ctx) { return(UNUSED) ; }
struct V4LEX__Table *v4eval_GetTable(struct V4C__Context *ctx,UCCHAR *tbl,LOGICAL *log) { return(NULL) ; }
int v4l_ListPoint_Value(struct V4C__Context *ctx,struct V4L__ListPoint *lp,int int1,struct V4DPI__Point *pnt) { return(UNUSED) ; }
LOGICAL v4ctx_FrameAddDim(struct V4C__Context *ctx,int int1,struct V4DPI__Point *pnt,int int2,int int3) { return(FALSE) ; }
int v4dpi_DimGet(struct V4C__Context *ctx,UCCHAR *name, int dimid) { return(UNUSED) ; }
LOGICAL v4im_MakePTeleAccept(struct V4C__Context *ctx,struct V4DPI__DimInfo *di,struct V4DPI__Point *pnt,UCCHAR *buf,struct V4DPI__Point *pnt2,struct V4DPI__Point *pnt3) { return(FALSE) ; }
LOGICAL v4dpi_LocalDimAddEndBlock(struct V4C__Context *ctx,LOGICAL log) { return(FALSE) ; }
LOGICAL v4dpi_BindListMake(struct V4DPI__Binding *bind,struct V4DPI__Point *pnt,struct V4DPI__Point *pnt2,struct V4C__Context *ctx,int *int1,int int2,int int3, INDEX relhx) { return(FALSE) ; }
LOGICAL v4dpi_SaveBigTextPoint(struct V4C__Context *ctx,struct V4LEX__BigText *bt,int int1,struct V4DPI__Point *pnt,int int2,LOGICAL log1) { return(FALSE) ; }
int vregexp_RegExec (regex_t *rt,char *buf,size_t size,regmatch_t match[],int int1) { return(UNUSED) ; }
int Dim_UV4, Dim_Alpha, Dim_Dim, Dim_Logical, Dim_Int2, Dim_List ;
LOGICAL v4im_InitConstVals(struct V4C__Context *ctx)  { return(TRUE) ; }
int v4im_GetPointUC(LOGICAL *ok,UCCHAR *buf,int int1,struct V4DPI__Point *pnt,struct V4C__Context *ctx) { return(UNUSED) ; }
LOGICAL v4sxi_SpecialAcceptor(struct V4C__Context * ctx,struct V4DPI__Point *pnt,struct V4DPI__DimInfo *di,UCCHAR *buf) { return(FALSE) ; }
//char ASCTBUF1[V4TMBufMax] ;
struct V4LEX__CompileDir *v4trace_LoadVCDforHNum(struct V4C__Context *ctx,int hnum, LOGICAL force) { return(NULL) ;}
UCCHAR *v4dbg_FormatLineNum(int vis) { return(NULL) ;}
void v4trace_ExamineState(struct V4C__Context *ctx,struct V4DPI__Point *failpt,FLAGS32 flag, int stream) { }
void v4_UCerror(int errnum,struct V4LEX__TknCtrlBlk *tcb,char *subsys,char *module,char *mne,UCCHAR *ucmsg) { }
struct V4MM__MemMgmtMaster *v4mm_MemMgmtInit(struct V4MM__MemMgmtMaster *mmm) { return(NULL) ; }
LOGICAL v4im_MTGrabLock(struct V4C__Context *ctx,DCLSPINLOCK *lock) { return(FALSE) ; }
struct V4DPI__LittlePoint protoInt, protoDbl, protoDict, protoAlpha, protoLog, protoNone, protoQNone, protoUCChar, protoFix, protoNull ;
void intPNTvMod(struct V4DPI__Point * respnt,int val) { return ; } ;
ETYPE traceGlobal ;
LOGICAL v4dpi_PushLocalSym(struct V4C__Context *ctx,UCCHAR *s,struct V4DPI__Point *pt,struct V4DPI__Point *pt2,ETYPE type) { return(TRUE) ; } ;
ETYPE v4l_ListPoint_Modify(struct V4C__Context *ctx,struct V4L__ListPoint *lp,int i,struct V4DPI__Point *point,int j) { return(TRUE) ; } ;
UCCHAR *v4im_MessageNextMessageText() { return(UClit("")) ; } ;
LOGICAL v4im_MessageSendQueue(struct V4C__Context *ctx,int intmodx,UCCHAR *host,int port,int ttl ,struct V4DPI__Point *sendPt,UCCHAR *msgId,LOGICAL wantsReply,UCCHAR *ackBuf,int ackBufsize,UCCHAR *spawnCmdLine,CALENDAR delCalDT,int trace) { return(FALSE) ; } ;
LOGICAL v4l_ListClose(struct V4C__Context *ctx,struct V4L__ListPoint *lp) { return(TRUE) ; } ;
void v4xdb_FreeStuff(struct V4C__Context *ctx,COUNTER cnt,FLAGS32 flags) {} ;
#endif
