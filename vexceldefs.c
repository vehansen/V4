/*	vexceldefs.c - Definitions for Excel/BIFF Modules		*/

struct ss__Format *formatNew() ;
void wsInitialize() ;
void wsPageSetup() ;
void wsStoreDimensions() ;
void wsStoreWindow2() ;
void wsStoreSelection() ;
void wsStoreBOF() ;
void wsStoreEOF() ;
void wbStoreWorkbook() ;
void oleClose() ;
void wbStoreBOF() ;
void wbStoreWindow1() ;
void wbStore1904() ;
void wbStoreAllFonts() ;
void wbStoreAllNumFormats() ;
void wbStoreAllXFS() ;
void wbStoreAllStyles() ;
void wbStoreStyle() ;
void wbCalcSheetOffsets() ;
void wbStoreBoundSheet() ;
void wbStoreEOF() ;
void wbStoreNumFormat() ;
void oleInitialize() ;
void oleWritePPS() ;
int oleSetSize() ;
void oleWritePadding() ;
void oleWritePropertyStorage() ;
void oleWriteBigBlockDepot() ;
void oleWriteHeader() ;
void oleWrite() ;
void wsStoreColInfo() ;
int formatGetFontKey() ;
struct ss__OLE *oleNew() ;
struct ss__XF *formatGetXF() ;
struct ss__Font *formatGetFont() ;
struct ss__Format *wbAddFormat() ;

struct ss__WorkBook *wbNew(char *) ;
struct ss__WorkSheet *wbAddWorkSheet(struct ss__WorkBook *,char *,int) ;
struct ss__Format *wbMergeFormats(struct ss__WorkBook *,struct ss__Format *,struct ss__Format *) ;
void wsWriteString(struct ss__WorkSheet *,int,int,char *,struct ss__Format *) ;
void FixUpStrings(char *) ;
void wsWriteFooter(struct ss__WorkSheet *,char *) ;
void wsWriteHeader(struct ss__WorkSheet *,char *) ;


struct ss__biff {
  int ByteOrder, datasize, MaxSize ;
} ;

#define WB_WorkSheetMax 100
#define WB_DataSize 500000
#define WB_FormatMax 250
#define WB_FontMax 255
struct ss__WorkBook {
  char FileName[64], SheetName[64] ;
  char urlformat[64] ;
  char *parser ;
  int firstsheet,activesheet ;
  struct ss__OLE *ole ;
  struct ss__Format *tmpformat ;  
  int D1904, xf_index, fileclosed, biffsize ;
  int wsSize ;		/* Size (bytes) to allocate for each worksheet */
  int wsCount ;		/* Number of worksheets */
  struct ss__WorkSheet *ws[WB_WorkSheetMax] ;
  int formatCount ;
  struct ss__Format *format[WB_FormatMax] ;
/*
#define KHMax 255
  struct {
    int Count ;
    int Key[KHMax], Index[KHMax] ;
   } kh ;
*/
  int fontcount ;
  int nextfontindex ;
  struct {
    int Key ;
    int Index ;
    struct ss__Font *font ;
   } fe[WB_FontMax] ;

  int datasize ;
  char *data ;
} ;

#define WS_DataSize 500000		/* Initial size of sheet data buffer (reallocated as necessary) */
#define WS_COLMAX 512
#define WS_PBMax 100
struct ss__WorkSheet {
  struct ss__WorkBook *wb ;		/* Parent workbook */
  char name[100] ;
  int index ;
  int activesheet ;
  int firstsheet ;
  int Portrait ;
  int Scale ;
  int FitToWidth ;			/* Fit to width - how many pages */
  int FitToHeight ;			/* Fit to height - how many pages */
  int GridLines ;			/* If non-zero then show gridlines */
  int RefMode ;				/* UNUSED = not set (A1 assumed), 0 = A1 addressing, 1 = R1C1 mode */
  double LMargin, RMargin ;		/* Left/Right margins in inches */
  double TMargin, BMargin ;		/* Top/Bottom margins */
  char urlformat[100] ;
  char header[256], footer[246] ;
  int parser ;
  int using_tmpfile ;
//  FILE *fp ;
  int fileclosed ;
  int offset ;
  int xls_rowmax, xls_colmax, xls_strmax ;
  int dim_rowmin, dim_rowmax, dim_colmin, dim_colmax ;
  int colCount ;
  struct {
    int firstcol, lastcol, format, hidden ;
    double width ;
   } ColInfo[WS_COLMAX] ;
  struct {
    int firstrow, firstcol, lastrow, lastcol ;
   } Selection ;
  int PBCount ;				/* Number of page breaks below */
  int PBRows[WS_PBMax] ;		/* Rows containing page breaks */
  int datasize ;
  int datamax ;				/* Max bytes allowed below */
  char *data ;
} ;

struct ss__OLE {
  char FileName[255] ;
  FILE *fp ;
  int fileclosed ;
  int biff_only ;
  int size_allowed ;
  int biffsize ;
  int booksize ;
  int big_blocks ;
  int list_blocks ;
  int root_start ;
  int block_count ;
  int bytes_written ;
} ;

struct ss__Format {
  int xf_index ;
  int font_index ;
  char font[64] ;
  int size, bold, italic, color /* Font color */, underline, size_to_fit ;
  int font_strikeout, font_outline, font_shadow, font_script, font_family, font_charset ;
  char mask[64] ;			/* Format mask (converted to numeric num_format below) */
  int num_format ;
  int text_h_align, text_wrap, text_justlast, text_v_align, rotation ;
  int fg_color, bg_color ;
  int pattern ;
  int bottom, top, left, right ;
  int bottom_color, top_color, left_color, right_color ;
} ;

struct ss__Font {
  short record,length ;
  short dyHeight, grbit, icv, bls, sss ;
  char uls, bFamily, bCharSet, reserved, cch ;
  char rgch[64] ;
} ;

struct ss__XF {
  short record, length ;
  short ifnt, ifmt, style, align, icv, fill, border1, border2 ;
} ;

//#define P2(num) ((num & 0x00ff) << 8) + ((num & 0xff00) >> 8)
#define P2(num) num
//#define P4(num) (num >> 24) + ((num >> 8) & 0xff00) + ((num << 8) & 0xff0000) + (num <<24)
#define UP2(num) num
#define P4(num) num
#define PD(num) num
#define PN(num) (num >> 24) + ((num >> 8) & 0xff00) + ((num << 8) & 0xff0000) + (num <<24)

#define BIFF_VERSION 0x0500
