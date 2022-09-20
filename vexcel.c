#include <stdio.h>
//#include <file.h>
#include <string.h>
#include <stdlib.h>
#include "v4defs.c"

//#define UNITTEST 1
void wbAppend() ;

#include "vexceldefs.c"

int v_ParseRowCol(struct ss__WorkSheet *,char *,int,int,int*,int*,int*,int*) ;
  ws ;
  char *spec ;
  int row,col,*prow,*pcol,*prrow,*prcol ;


void wsAppend(ws,data,bytes)
  struct ss__WorkSheet *ws ;
  char *data ; int bytes ;
{
  struct xl__ls {
    short type,length ;
   } *ls ;
  int tot ;

/*	If we are out of room then reallocate more */
	if (ws->datasize + bytes > ws->datamax)
	 { 
	   ws->datamax *= 1.5 ; ws->data = realloc(ws->data,ws->datamax) ;
//	   printf("Exceeded max ws size of %d bytes\n",ws->datamax) ; exit(EXITABORT) ;
	 } ;
	ls = (struct xl__ls *)data ;
	if (ls->length != bytes - 4)
	 printf("something not right\n") ;
	memcpy(ws->data+ws->datasize,data,bytes) ; ws->datasize += bytes ;
	ls = (struct xl__ls *)ws->data ; tot = 0 ;
//	for(;tot<ws->datasize;)
//	 { tot += ls->length + 4 ; ls = (char *)ls + ls->length + 4 ;
//	 } ;
//	if (tot != ws->datasize)
//	 printf("tot = %d, ws->bytes = %d\n",tot,ws->datasize) ;
}

void wsPrepend(ws,data,bytes)
  struct ss__WorkSheet *ws ;
  char *data ; int bytes ;
{

	memmove(ws->data+bytes,ws->data,ws->datasize) ; memcpy(ws->data,data,bytes) ; ws->datasize += bytes ;
}

void wsStoreBOF(ws,type)
  struct ss__WorkSheet *ws ;
  int type ;
{
#pragma pack(1)
  struct lcl__BOF {
    short record,length ;
    short version,type,build,year ;
   } lBOF ;
#pragma pack()

	lBOF.record = P2(0x0809) ; lBOF.length = P2(0x0008) ;
	lBOF.version = P2(BIFF_VERSION) ; lBOF.type = P2(type) ;
	lBOF.build = P2(0x096c) ; lBOF.year = P2(0x07C9) ;
	wsPrepend(ws,&lBOF,sizeof lBOF) ;
//	memcpy(ws->data+ws->datasize,&lBOF,sizeof lBOF) ; ws->datasize += sizeof lBOF ;
/*
    my $self    = shift;
    my $record  = 0x0809;        # Record identifier
    my $length  = 0x0008;        # Number of bytes to follow

    my $version = $BIFF_version;
    my $type    = $_[0];

    # According to the SDK $build and $year should be set to zero.
    # However, this throws a warning in Excel 5. So, use these
    # magic numbers.
    my $build   = 0x096C;
    my $year    = 0x07C9;

    my $header  = pack("vv",   $record, $length);
    my $data    = pack("vvvv", $version, $type, $build, $year);

    $self->_prepend($header, $data);
*/	
}

void wsStoreEOF(ws)
  struct ss__WorkSheet *ws ;
{
  struct lcl__EOF {
    short record,length ;
   } lEOF ;

	lEOF.record = P2(0x000A) ; lEOF.length = P2(0x0000) ;
	wsAppend(ws,&lEOF,sizeof lEOF) ;
//	memcpy(ws->data+ws->datasize,&lEOF,sizeof lEOF) ; ws->datasize += sizeof lEOF ;

/*
    my $self      = shift;
    my $record    = 0x000A; # Record identifier
    my $length    = 0x0000; # Number of bytes to follow

    my $header    = pack("vv", $record, $length);

    $self->_append($header);
*/
}



struct ss__WorkSheet *wsNew(name,index,activesheet,firstsheet,urlformat,parser,maxbytes)
 char *name, *urlformat ;
 int index, activesheet, firstsheet, parser, maxbytes ;
{ struct ss__WorkSheet *ws ;

	ws = (struct ss__WorkSheet *)v4mm_AllocChunk(sizeof *ws,TRUE) ;
	if (maxbytes < 0 || maxbytes > 1000000) maxbytes = WS_DataSize ;
	ws->datamax = (maxbytes <= WS_DataSize ? WS_DataSize : maxbytes) ;
	ws->data = v4mm_AllocChunk(ws->datamax,FALSE) ;
	strcpy(ws->name,name) ; strcpy(ws->urlformat,urlformat) ;
	ws->index = index ; ws->activesheet = activesheet ; ws->firstsheet = firstsheet ;

	ws->using_tmpfile = TRUE ;
	ws->xls_rowmax = 65536 ; ws->xls_colmax = 256 ; ws->xls_strmax = 255 ;
	ws->dim_rowmin = ws->xls_rowmax + 1 ; ws->dim_colmin = ws->xls_colmax + 1 ;
	ws->LMargin = -1 ; ws->RMargin = -1 ; ws->TMargin = -1 ; ws->BMargin = -1 ;
	wsInitialize(ws) ;
	return(ws) ;
/*
    my $class               = shift;
    my $self                = Spreadsheet::WriteExcel::BIFFwriter->new();
    my $rowmax              = 65536; # 16384 in Excel 5
    my $colmax              = 256;
    my $strmax              = 255;

    $self->{ name}          = $_[0];
    $self->{_index}         = $_[1];
    $self->{_activesheet}   = $_[2];
    $self->{_firstsheet}    = $_[3];
    $self->{_url_format}    = $_[4];
    $self->{_parser}        = $_[5];

    $self->{_using_tmpfile} = 1;
    $self->{_filehandle}    = "";
    $self->{_fileclosed}    = 0;
    $self->{_offset}        = 0;
    $self->{_xls_rowmax}    = $rowmax;
    $self->{_xls_colmax}    = $colmax;
    $self->{_xls_strmax}    = $strmax;
    $self->{_dim_rowmin}    = $rowmax +1;
    $self->{_dim_rowmax}    = 0;
    $self->{_dim_colmin}    = $colmax +1;
    $self->{_dim_colmax}    = 0;
    $self->{_colinfo}       = [];
    $self->{_selection}     = [0, 0];


    bless $self, $class;
    $self->_initialize();
    return $self;
*/
}

void wsInitialize(ws)
  struct ss__WorkSheet *ws ;
{
//	ws->fp = tmpfile() ;
	ws->using_tmpfile = TRUE ;
	ws->RefMode = UNUSED ;
/*
# _initialize()
#
# Open a tmp file to store the majority of the Worksheet data. If this fails,
# for example due to write permissions, store the data in memory. This can be
# slow for large files.
#
sub _initialize {

    my $self    = shift;

    # Open tmp file for storing Worksheet data
    my $fh = IO::File->new_tmpfile();

    if (defined $fh) {
        # binmode file whether platform requires it or not
        binmode($fh);

        # Store filehandle
        $self->{_filehandle} = $fh;
    }
    else {
        # If new_tmpfile() fails store data in memory
        $self->{_using_tmpfile} = 0;
    }
*/
}


wsClose(ws)
  struct ss__WorkSheet *ws ;
{ int i,length ;
  struct lcl__PageBreaks {
    short record, length ;
    short cbrk ;
    short rgrw[WS_PBMax] ;
   } pb ;
  struct lcl__Grid {
    short record, length ;
    short PrintGrid ;
   } grid ;

	wsPageSetup(ws) ;
	wsStoreDimensions(ws) ;
	for(i=0;i<ws->colCount;i++)
	 { wsStoreColInfo(ws,ws->ColInfo[i].firstcol,ws->ColInfo[i].lastcol,ws->ColInfo[i].width,
		ws->ColInfo[i].format,ws->ColInfo[i].hidden) ;
	 } ;
	wsStoreBOF(ws,0x0010) ;
	wsStoreWindow2(ws) ;
	wsStoreSelection(ws,ws->Selection.firstrow,ws->Selection.firstcol,ws->Selection.lastrow,ws->Selection.lastcol) ;
	if (strlen(ws->header)) { wsWriteHeader(ws,ws->header) ; } ;
	if (strlen(ws->footer)) { wsWriteFooter(ws,ws->footer) ; } ;
	switch (ws->GridLines)
	 { case 1:	grid.record = P2(0x2B) ; grid.length = P2(sizeof grid - 4) ; grid.PrintGrid = 1 ;
			wsAppend(ws,&grid,sizeof grid) ; break ;
	   case 2:	/* This is handled by setting all formats to include hairline borders! */
			break ;
	 } ;
	if (ws->PBCount > 0)
	 { pb.record = P2(27) ;
	   for (i=0;i<ws->PBCount;i++) { pb.rgrw[i] = P2(ws->PBRows[i]) ; } ;
	   length = (char *)&pb.rgrw[ws->PBCount] - (char *)&pb - 4 ; pb.length = P2(length) ; pb.cbrk = P2(ws->PBCount) ;
	   wsAppend(ws,&pb,length + 4) ;
	 } ;
	wsStoreEOF(ws) ;
/*
sub _close {

    my $self = shift;

    # Prepend in reverse order!!
    $self->_store_dimensions();


    # Prepend the COLINFO records if they exist
    if (@{$self->{_colinfo}}){
        while (@{$self->{_colinfo}}) {
            my $arrayref = pop @{$self->{_colinfo}};
            $self->_store_colinfo(@$arrayref);
        }
        $self->_store_defcol();
    }

    # Prepend in reverse order!!
    $self->_store_bof(0x0010);

    # Append
    $self->_store_window2();
    $self->_store_selection(@{$self->{_selection}});
    $self->_store_eof();
*/

}

wsActivate(ws)
  struct ss__WorkSheet *ws ;
{

	ws->activesheet = ws->index ;
/*
# Set this worksheet as the selected worksheet, i.e. the worksheet
# with its tab highlighted.
#
sub activate {

    my $self = shift;

    ${$self->{_activesheet}} = $self->{_index};
}
*/
}


wsFirstsheet(ws)
  struct ss__WorkSheet *ws ;
{
	ws->firstsheet = ws->index ;
/*
# set_first_sheet()
#
# Set this worksheet as the first visible sheet. This is necessary
# when there are a large number of worksheets and the activated
# worksheet is not visible on the screen.
#
sub set_first_sheet {

    my $self = shift;

    ${$self->{_firstsheet}} = $self->{_index};
}
*/
}


wsXF(ws)
  struct ss__WorkSheet *ws ;
{
	if (ws != NULL) return(0) ;
	return(0x0f) ;
/*
# _XF()
#
# Returns an index to the XF record in the workbook
#
sub _XF {

    my $self = shift;

    if (ref($self)) {
        return $self->get_xf_index();
    }
    else {
        return 0x0F;
    }
}
*/
}

void wsPageSetup(ws)
  struct ss__WorkSheet *ws ;
{
#pragma pack(1)
  struct lcl__PageSetup {
    short record, length ;
    short iPaperSize, iScale, iPageStart, iFitWidth, iFitHeight, igrbit, iRes, iVRes ;
    double numHdr, numFtr ;
    short iCopies ;
   } ps ;
  struct lcl__WSBool {
    short record, length ;
    unsigned short grbit ;
   } wsb ;
  struct lcl__LMargin {
    short record, length ;
    double num ;
   } lm ;
  struct lcl__RMargin {
    short record, length ;
    double num ;
   } rm ;
  struct lcl__TMargin {
    short record, length ;
    double num ;
   } tm ;
  struct lcl__BMargin {
    short record, length ;
    double num ;
   } bm ;
#pragma pack()

	memset(&ps,0,sizeof ps) ;
	ps.record = P2(0xA1) ; ps.length = sizeof ps - 4 ;
	if (ws->Scale == 0 && ws->FitToWidth == 0 && ws->FitToHeight == 0) ws->Scale = 100 ;
	ps.iScale = P2(ws->Scale) ; ps.iFitWidth = P2(ws->FitToWidth) ; ps.iFitHeight = P2(ws->FitToHeight) ;
	ps.igrbit = (ws->Portrait ? 0x02 : 0) ;
	wsAppend(ws,&ps,sizeof ps) ;
	if (ws->FitToWidth || ws->FitToHeight)		/* If either of these set then must set additional flag */
	 { wsb.record = P2(0x0081) ; wsb.length = P2(sizeof wsb - 4) ;
	   wsb.grbit = P2(0x0100) ;
	   wsAppend(ws,&wsb,sizeof wsb) ;
	 } ;
	if (ws->LMargin >= 0)
	 { lm.record = P2(0x26) ; lm.length = sizeof lm - 4 ; memcpy(&lm.num,&ws->LMargin,sizeof(double)) ;
	   wsAppend(ws,&lm,sizeof lm) ;
	 } ;
	if (ws->RMargin >= 0)
	 { rm.record = P2(0x27) ; rm.length = sizeof rm - 4 ; memcpy(&rm.num,&ws->LMargin,sizeof(double)) ;
	   wsAppend(ws,&rm,sizeof rm) ;
	 } ;
	if (ws->TMargin >= 0)
	 { tm.record = P2(40) ; tm.length = sizeof tm - 4 ; memcpy(&tm.num,&ws->TMargin,sizeof(double)) ;
	   wsAppend(ws,&tm,sizeof tm) ;
	 } ;
	if (ws->BMargin >= 0)
	 { bm.record = P2(41) ; bm.length = sizeof bm - 4 ; memcpy(&bm.num,&ws->BMargin,sizeof(double)) ;
	   wsAppend(ws,&bm,sizeof bm) ;
	 } ;

}

void wsStoreDefinedName(ws,name,pdnx,lclglbl,definition,rows)	/* Enters a defined name */
  struct ss__WorkSheet *ws ;
  char *name ;		/* The name to define (NULL if using pdnx) */
  int pdnx ;		/* Predefined name index if name is NULL */
  int lclglbl ;		/* 0 for global name, worksheet index for local */
  char *definition ;	/* The definition for the symbol */
  int rows ;		/* Temp parameter - number of rows to repeat at top of page */
			/* Note - this routine reverse-engineered, protocol not completely understood therefore just
			   hacking in what I need to keep things going  VEH040114 */
{
#pragma pack(1)
  struct lcl__ExternCount {
    short record, length ;
    short cxals ;
   } ec ;
  struct lcl__ExternSheet {
    short record, length ;
    char cch ;
    char unknown ;
    short rgch[100] ;
   } es ;
  struct lcl__DefinedName {
    short record, length ;
    short grbit ;
    char chKey, cch ;
    short cce, ixals, itab ;
    char cchCustMenu, cchDescription, cchHelptopic, cchStatustext ;
    char var[1000] ;
  } dn ;
#pragma pack()

	ec.record = P2(0x16) ; ec.length = 2 ; ec.cxals = 1 ;
	wbAppend(ws->wb,&ec,sizeof ec) ;

	es.record = P2(0x17) ;
	es.rgch[1] = 3 ; 
//	es.cch = strlen("Sheet1") ; es.unknown = 3 ;
//	strcpy(es.rgch,"Sheet1") ;
	es.cch = strlen(ws->name) ; es.unknown = 3 ;
	strcpy((char *)es.rgch,ws->name) ;
	es.length = 2 + es.cch ;
	wbAppend(ws->wb,&es,es.length+4) ;

	dn.record = P2(0x18) ;
	dn.grbit = 0 ; dn.chKey = 0 ; 
	dn.ixals = lclglbl ; dn.itab = dn.ixals ;
	dn.cchCustMenu = 0 ; dn.cchDescription = 0 ; dn.cchHelptopic = 0 ; dn.cchStatustext = 0 ;

	if (name == NULL)
	 { dn.cch = P2(01) ; dn.var[0] = pdnx ;
//VEH040114 - this code needs to be able to parse Sheet1!$1:$n
//	   dn.cce = v_ParseExcelFormula(ws,definition,&dn.var[1],0,0) ;
	   dn.grbit |= 0x20 ;
//	   dn.length = P2((&dn.var[dn.cce+1] - (char *)&dn) - 4) ;

	dn.length = 0x24 ; dn.cce = 0x15 ;	/* This code is part of hack reverse-engineering VEH040114 */
	memcpy(&dn.var[1],"\x3b\xff\xff\0\0\0\0\0\0\1\0\0\0\0\0\0\0\4\0\0\xff",21) ;
	if (rows > 0) dn.var[18] = rows - 1 ;

	 } ;

	wbAppend(ws->wb,&dn,dn.length + 4) ;
}  

void wsStoreDimensions(ws)
  struct ss__WorkSheet *ws ;
{
#pragma pack(1)
  struct lcl__StoreDimensions {
    short record, length ;
    short rowmin, rowmax, colmin, colmax, reserved ;
   } sd ;
#pragma pack()

	sd.record = P2(0x0000) ; sd.length = P2(0x000a) ;
	sd.rowmin = P2(ws->dim_rowmin) ; sd.rowmax = P2(ws->dim_rowmax) ;
	sd.colmin = P2(ws->dim_colmin) ; sd.colmax = P2(ws->dim_colmax) ;
	sd.reserved = P2(0x0000) ;
	wsPrepend(ws,&sd,sizeof sd) ;
//	memmove(ws->data+sizeof sd,ws->data,ws->datasize) ; memcpy(ws->data,&sd,sizeof sd) ; ws->datasize += sizeof sd ;
/*
# _store_dimensions()
#
# Writes Excel DIMENSIONS to define the area in which there is data.
#
sub _store_dimensions {

    my $self      = shift;
    my $record    = 0x0000;               # Record identifier
    my $length    = 0x000A;               # Number of bytes to follow
    my $row_min   = $self->{_dim_rowmin}; # First row
    my $row_max   = $self->{_dim_rowmax}; # Last row plus 1
    my $col_min   = $self->{_dim_colmin}; # First column
    my $col_max   = $self->{_dim_colmax}; # Last column plus 1
    my $reserved  = 0x0000;               # Reserved by Excel

    my $header    = pack("vv",    $record, $length);
    my $data      = pack("vvvvv", $row_min, $row_max,
                                  $col_min, $col_max, $reserved);
    $self->_prepend($header, $data);
}
*/
}

void wsStoreWindow2(ws)
  struct ss__WorkSheet *ws ;
{

#pragma pack(1)
  struct lcl__W2 {
    short record, length ;
    short grbit, rwTop, colLeft ;
    int rgbHdr ;
   } w2 ;
  struct lcl__Pane {
    short record, length ;
    short x, y, rwTop, colLeft, pnnAct ;
   } pane ;
#pragma pack()

	w2.record = P2(0x023e) ; w2.length = P2(0x000a) ;
	w2.grbit = P2(0x00b6) ; w2.rwTop = P2(0x0000) ; w2.colLeft = P2(0x0000) ;
	w2.rgbHdr = P4(0x00000000) ;
	if (ws->activesheet == ws->index) w2.grbit = P2(0x06b6) ;
//w2.grbit = P2(0x00BE) ; /* To freeze panes */
	wsAppend(ws,&w2,sizeof w2) ;

	pane.record = P2(0x41) ; pane.length = P2((sizeof pane) - 4) ;
	pane.x = P2(0) ;
	pane.y = P2(1) ;
	pane.rwTop = P2(2) ;
	pane.colLeft = P2(0) ;	/* Leftmost column visible in right pane */
	pane.pnnAct = P2(0) ;	/* Lower right is active */
//	wsAppend(ws,&pane,sizeof pane) ;

//	memcpy(ws->data+ws->datasize,&w2,sizeof w2) ; ws->datasize += sizeof ws ;
/*
# Write BIFF record Window2.
#
sub _store_window2 {

    my $self    = shift;
    my $record  = 0x023E;     # Record identifier
    my $length  = 0x000A;     # Number of bytes to follow

    my $grbit   = 0x00B6;     # Option flags
    my $rwTop   = 0x0000;     # Top row visible in window
    my $colLeft = 0x0000;     # Leftmost column visible in window
    my $rgbHdr  = 0x00000000; # Row/column heading and gridline color

    if (${$self->{_activesheet}} == $self->{_index}) {
        $grbit = 0x06B6;
    }

    my $header  = pack("vv",   $record, $length);
    my $data    = pack("vvvV", $grbit, $rwTop, $colLeft, $rgbHdr);

    $self->_append($header, $data);
*/
}

wsStoreDefcol(ws)
  struct ss__WorkSheet *ws ;
{
  struct lcl__Defcol {
    short record, length ;
    short colwidth ;
   } dc ;

	dc.record = P2(0x0055) ; dc.length = P2(0x0002) ;
	dc.colwidth = P2(0x0008) ;
	wsPrepend(ws,&dc,sizeof dc) ;
//	memmove(ws->data+sizeof dc,ws->data,ws->datasize) ; memcpy(ws->data,&dc,sizeof dc) ; ws->datasize += sizeof dc ;
/*
#
# Write BIFF record DEFCOLWIDTH if COLINFO records are in use.
#
sub _store_defcol {

    my $self     = shift;
    my $record   = 0x0055;      # Record identifier
    my $length   = 0x0002;      # Number of bytes to follow

    my $colwidth = 0x0008;      # Default column width

    my $header   = pack("vv", $record, $length);
    my $data     = pack("v",  $colwidth);

    $self->_prepend($header, $data);
*/
}


void wsStoreColInfo(ws,firstcol,lastcol,width,format,hidden)
  struct ss__WorkSheet *ws ;
  int firstcol,lastcol,format,hidden ;
  double width ;
{ double twidth ;
#pragma pack(1)
  struct lcl__StoreColinfo {
    short record,length ;
    short colFirst,colLast,coldx,ixfe,grbit ;
    unsigned char reserved ;
   } ci ;
#pragma pack()

	ci.record = P2(0x007d) ; ci.length = P2(0x000b) ;
	ci.colFirst =  P2(firstcol) ; ci.colLast = P2(lastcol) ;
	twidth = (width > 0 ? width : 8.43) ; twidth += 0.72 ;
	ci.coldx = (short)(twidth * 256) ;
	ci.ixfe = format ;
	ci.grbit = P2(hidden) ; ci.reserved = '\0' ;
	wsPrepend(ws,&ci,sizeof ci) ;
//	memmove(ws->data+sizeof ci,ws->data,ws->datasize) ; memcpy(ws->data,&ci,sizeof ci) ; ws->datasize += sizeof ci ;
/*
# _store_colinfo($firstcol, $lastcol, $width, $format, $hidden)
#
# Write BIFF record COLINFO to define column widths
#
# Note: The SDK says the record length is 0x0B but Excel writes a 0x0C
# length record.
#
sub _store_colinfo {

    my $self     = shift;
    my $record   = 0x007D;          # Record identifier
    my $length   = 0x000B;          # Number of bytes to follow

    my $colFirst = $_[0] || 0;      # First formatted column
    my $colLast  = $_[1] || 0;      # Last formatted column
    my $coldx    = $_[2] || 8.43;   # Col width, 8.43 is Excel default

    $coldx       += 0.72;           # Fudge. Excel subtracts 0.72 !?
    $coldx       *= 256;            # Convert to units of 1/256 of a char


    my $ixfe     = _XF($_[3]);      # XF
    my $grbit    = $_[4] || 0;      # Option flags
    my $reserved = 0x00;            # Reserved

    my $header   = pack("vv",     $record, $length);
    my $data     = pack("vvvvvC", $colFirst, $colLast, $coldx,
                                  $ixfe, $grbit, $reserved);

    $self->_prepend($header, $data);
*/
}


void wsStoreSelection(ws,firstrow,firstcol,lastrow,lastcol)
  struct ss__WorkSheet *ws ;
  int firstrow,firstcol,lastrow,lastcol ;
{
#pragma pack(1)
  struct lcl__StoreSelection {
    short record,length ;
    char pnn ;
    short rwAct,colAct,irefAct,cref,rwFirst,rwLast ;
    unsigned char colFirst,colLast ;
   } ss ;
#pragma pack()
  short ts ; unsigned char tc ;

	ss.record = P2(0x001d) ; ss.length = P2(0x000f) ;
	ss.pnn = P2(3) ; ss.rwFirst = (ss.rwAct = P2(firstrow)) ; ss.colFirst = (ss.colAct = P2(firstcol)) ;
	ss.rwLast = P2(lastrow) ; ss.colLast = P2(lastcol) ;
	ss.irefAct = P2(0) ; ss.cref = P2(1) ;
	if (ss.rwFirst > ss.rwLast) { ts = ss.rwFirst ; ss.rwFirst = ss.rwLast ; ss.rwLast = ts ; } ;
	if (ss.colFirst > ss.colLast) { tc = ss.colFirst ; ss.colFirst = ss.colLast ; ss.colLast = tc ; } ;
	wsAppend(ws,&ss,sizeof ss) ;
//	memcpy(ws->data+ws->datasize,&ss,sizeof ss) ; ws->datasize += sizeof ss ;
/*
###############################################################################
#
# _store_selection($first_row, $first_col, $last_row, $last_col)
#
# Write BIFF record SELECTION.
#
sub _store_selection {

    my $self     = shift;
    my $record   = 0x001D;                  # Record identifier
    my $length   = 0x000F;                  # Number of bytes to follow

    my $pnn      = 3;                       # Pane position
    my $rwAct    = $_[0];                   # Active row
    my $colAct   = $_[1];                   # Active column
    my $irefAct  = 0;                       # Active cell ref
    my $cref     = 1;                       # Number of refs

    my $rwFirst  = $_[0];                   # First row in reference
    my $colFirst = $_[1];                   # First col in reference
    my $rwLast   = $_[2] || $rwFirst;       # Last row in reference
    my $colLast  = $_[3] || $colFirst;      # Last col in reference

    # Swap last row/col for first row/col as necessary
    if ($rwFirst > $rwLast) {
        ($rwFirst, $rwLast) = ($rwLast, $rwFirst);
    }

    if ($colFirst > $colLast) {
        ($colFirst, $colLast) = ($colLast, $colFirst);
    }


    my $header   = pack("vv",           $record, $length);
    my $data     = pack("CvvvvvvCC",    $pnn, $rwAct, $colAct,
                                        $irefAct, $cref,
                                        $rwFirst, $rwLast,
                                        $colFirst, $colLast);

    $self->_append($header, $data);
*/
}

/*
###############################################################################
#
# write ($row, $col, $token, $format)
#
# Parse $token call appropriate write method. $row and $column are zero
# indexed. $format is optional.
#
# Returns: return value of called subroutine
#
sub write {

    my $self  = shift;
    my $token = $_[2];

    # Match number
    if ($token =~ /^([+-]?)(?=\d|\.\d)\d*(\.\d*)?([Ee]([+-]?\d+))?$/) {
        return $self->write_number(@_);
    }
    # Match http or ftp URL
    elsif ($token =~ m|[fh]tt?p://|) {
        return $self->write_url(@_);
    }
    # Match formula
    elsif ($token =~ /^=/) {
        return $self->write_formula(@_);
    }
    # Match blank
    elsif ($token eq '') {
        splice @_, 2, 1; # remove the empty string from the parameter list
        return $self->write_blank(@_);
    }
    # Default: match string
    else {
        return $self->write_string(@_);
    }
}
*/

int v_ParseExcelFormula(ws,formula,resbuf,row,col)	/* Parses formula, returns # bytes in resbuf */
  struct ss__WorkSheet *ws ;
  char *formula ;			/* ASCIZ formula to parse */
  char *resbuf ;			/* Where result is to go */
  int row,col ;				/* Current row & column for relative references */
{ struct V4LEX__TknCtrlBlk tcb ;
  struct V4LEX__RevPolParse rpp ;
#pragma pack(1)
  struct lcl__CellRef {
    unsigned char ptgRef ;		/* 0x24 - Cell reference */
    unsigned short grbitRw ;		/* Bits & row */
    unsigned char col ;			/* Column */
   } *lc ;
  struct lcl__AreaRef {
    unsigned char ptgArea ;		/* 0x25 - Area reference */
    unsigned short grbitRwFirst ;
    unsigned short grbitRwLast ;
    unsigned char colFirst, colLast ;
   } *la ;
  struct lcl__Function {
    unsigned char ptgFuncVar ;
    unsigned char cargs ;
    unsigned short iftab ;
   } *lfnc ;
#pragma pack()
  char *p ;
  int flags,rx,operands,ptg,n,lrow,lcol,isrowrel,iscolrel,argcnt,ix ; short sint ;

	flags = V4LEX_RPPFlag_EOL | V4LEX_RPPFlag_Excel ;
	v4lex_InitTCB(&tcb,0) ; v4lex_NestInput(&tcb,NULL,ASCretUC(formula),V4LEX_InpMode_String) ;
	if (!v4lex_RevPolParse(&rpp,&tcb,flags,NULL))
	 return(UNUSED) ;
	p = resbuf ;

	for(rx=0;rx<rpp.Count;rx++)
	 { 

	   if (OPCODEISOPER(rpp.Token[rx].OpCode))	/* Got an operator? */
	    { operands = 2 ;
	      switch (rpp.Token[rx].OpCode)
	       { default:
			printf("? Error in Excel formula: %s\n",formula) ; exit(EXITABORT) ;
		 case V_OpCode_Plus:		ptg = 0x03 ; break ;
		 case V_OpCode_Minus:		ptg = 0x04 ; break ;
		 case V_OpCode_UMinus:		ptg = 0x13 ; operands = 1 ; break ;
		 case V_OpCode_NCStar:
		 case V_OpCode_Star:		ptg = 0x05 ; break ;
		 case V_OpCode_Slash:		ptg = 0x06 ; break ;
		 case V_OpCode_Equal:		ptg = 0x0b ; break ;
		 case V_OpCode_Langle:		ptg = 0x09 ; break ;
		 case V_OpCode_LangleEqual:	ptg = 0x0a ; break ;
		 case V_OpCode_LangleRangle:	ptg = 0x0e ; break ;
		 case V_OpCode_Rangle:		ptg = 0x0d ; break ;
		 case V_OpCode_RangleEqual:	ptg = 0x0c ; break ;
		 case V_OpCode_NCColon:
		 case V_OpCode_Colon:	 	ptg = -1 ; break ;
		 case 999:			ptg = -2 ; break ;
	       } ;
	      switch (ptg)
	       { default:	*(p++) = ptg ; break ;
//		 case -1:	/* Here to combine two cell specs into range */
//			la.ptgArea = 0x25 ;
//			lc = (struct lcl__CelRef *)lastp2 ;	/* Get first cell specification */
//			la.grbitRwFirst = lc->grbitRw ; la.colFirst = lc->col ;
//			lc = (struct lcl__CelRef *)lastp ;	/* Get second cell specification */
//			la.grbitRwLast = lc->grbitRw ; la.colLast = lc->col ;
//			memcpy(lastp2,&la,sizeof la) ; p = lastp2 + sizeof la ;
//			argcnt-- ;				/* Merging two arguments into 1 */
//			break ;
		 case -2:	/* Here to form function reference */
			lfnc = (struct lcl__Function *)p ;
			lfnc->cargs = argcnt ; lfnc->ptgFuncVar = 0x22 ;
			if (UCstrcmp(rpp.Token[rx].AlphaVal,UClit("SUM")) == 0) { lfnc->iftab = 4 ; }
			 else { } ;
			p += sizeof *lfnc ;
			 break ;
	       } ;
	      continue ;
	    } ;
	   if (rpp.Token[rx].dps > 0 && rpp.Token[rx].Type == V4LEX_TknType_Integer)
	    { rpp.Token[rx].Type = V4LEX_TknType_Float ;
	      rpp.Token[rx].Floating = rpp.Token[rx].IntVal ;
	      for(n=0;n<rpp.Token[rx].dps;n++) { rpp.Token[rx].Floating /= 10.0 ; } ;
	    } ;
	   switch(rpp.Token[rx].Type)
	    { case V4LEX_TknType_EOA:		/* Start counting function arguments */
		argcnt = 0 ; break ;		/* NOTE: this will not work on nested functions! */
	      case V4LEX_TknType_String:
	      case V4LEX_TknType_Keyword:	/* Parse cell specification (ex: A3) */
		if ((ix = v_ParseRowCol(ws,UCretASC(rpp.Token[rx].AlphaVal),row,col,&lrow,&lcol,&isrowrel,&iscolrel)) == UNUSED)
		 { } ;
		if (rpp.Token[rx].AlphaVal[ix] == UClit(':'))	/* Have an area specification- cell:cell) ? */
		 { 
		   argcnt++ ; la = (struct lcl__AreaRef *)p ; p += sizeof *la ;
		   la->ptgArea = 0x25 ;
		   la->grbitRwFirst = (lrow == UNUSED ? 0 : lrow) ; la->colFirst = (lcol == UNUSED ? 0 : lcol) ;
		   if (isrowrel) la->grbitRwFirst |= 0x8000 ; if (iscolrel) la->grbitRwFirst |= 0x4000 ;
		   v_ParseRowCol(ws,UCretASC(&rpp.Token[rx].AlphaVal[ix+1]),row,col,&lrow,&lcol,&isrowrel,&iscolrel) ;
		   la->grbitRwLast = (lrow == UNUSED ? 0 : lrow) ; la->colLast = (lcol == UNUSED ? 0xFF : lcol) ;
		   if (isrowrel) la->grbitRwLast |= 0x8000 ; if (iscolrel) la->grbitRwLast |= 0x4000 ;
		 } else					/* Single cell */
		 { argcnt++ ; lc = (struct lcl__CellRef *)p ;
		   if (lcol == UNUSED) lcol = 0 ; if (lrow == UNUSED) lrow = 0 ;
		   lc->ptgRef = 0x44 /* ptgRefV */ ; lc->grbitRw = /*0xc000 + */ lrow ; lc->col = lcol ; p += sizeof *lc ;
//		   if (isrowrel) lc->grbitRw |= 0x80 ; if (iscolrel) lc->grbitRw |= 0x40 ;
		 } ;
		break ;
	      case V4LEX_TknType_Integer:
		argcnt++ ; *(p++) = 0x1e ; sint = rpp.Token[rx].IntVal ; memcpy(p,&sint,sizeof(int)) ; p += sizeof sint ;
		break ;
	      case V4LEX_TknType_Float:
		argcnt++ ; *(p++) = 0x1f ; memcpy(p,&rpp.Token[rx].Floating,sizeof(double)) ; p += sizeof(double) ; break ;
	      case V4LEX_TknType_EOL:
		break ;
	    } ;
	 } ;
	return(p - resbuf) ;
}
int v_ParseRowCol(ws,spec,row,col,prow,pcol,prrow,prcol)
  struct ss__WorkSheet *ws ;
  char *spec ;
  int row,col,*prow,*pcol,*prrow,*prcol ;
{ int i, l, t, sign ;

	*prow = UNUSED ; *pcol = UNUSED ; *prrow = TRUE ; *prcol = TRUE ;
	for(i=0;;i++)
	 { l = toupper(spec[i]) ;
	   if (l == '$' && i == 0) { *prcol = FALSE ; continue ; } ;
	   if (l < 'A' || l > 'Z') break ;
	   if (*pcol == UNUSED) *pcol = 0 ;
	   *pcol = (*pcol * 26) + (l - 'A') ;
	 } ;
/*	Check for RC specification */
	if (*pcol == 444 || l == '[' || l == 'C' || l == '+' || l == '-') goto rc_spec ;
	if (spec[i] == '$')
	 { i++ ; *prrow = FALSE ; }
	 else { if (*pcol == UNUSED && *prcol == FALSE) *prrow = FALSE ; } ;
	for(;;i++)
	 { l = spec[i] ; if (l < '0' || l > '9') break ;
	   if (*prow == UNUSED) *prow = 0 ;
	   *prow = (*prow * 10) + (l - '0') ;
	 } ; (*prow)-- ;
	return(i) ;
rc_spec:
	ws->RefMode = 0 ;			/* Set RefMode to R1C1 */
	*prow = 0 ; *pcol = 0 ; *prrow = FALSE ; *prcol = FALSE ;
	sign = FALSE ; t = 0 ;
	for(i=1;;i++)
	 { if (spec[i] == '[' && i == 1) { *prrow = TRUE ; continue ; } ;
	   if (spec[i] == '-') { sign = TRUE ; continue ; } ;
	   if (spec[i] == '+') { continue ; } ;
	   if (spec[i] < '0' || spec[i] > '9') break ;
	   t = (t * 10) + spec[i] - '0' ;
	 } ;
	*prow = row + (sign ? -t : t) ;
	if (spec[i] == ']') i++ ;
	if (toupper(spec[i++]) != 'C') return(UNUSED) ;
//	if (i == 2) *prrow = TRUE ;		/* If got "RC..." then row is relative to current */
	sign = FALSE ; t = 0 ;
	for(;;i++)
	 { if (spec[i] == '[') { *prcol = TRUE ; continue ; } ;
	   if (spec[i] == '-') { sign = TRUE ; continue ; } ;
	   if (spec[i] == '+') { continue ; } ;
	   if (spec[i] < '0' || spec[i] > '9') break ;
	   t = (t * 10) + spec[i] - '0' ;
	 } ;
	*pcol = col + (sign ? -t : t) ;
	if (spec[i] == ']') i++ ;
	return(i) ;
}

void wsWriteFormula(ws,row,col,str,result,calconload,format)
  struct ss__WorkSheet *ws ;
  int row,col,calconload ;
  double result ;
  char *str ;
  struct ss__Format *format ;
{ 
#pragma pack(1)
  struct lcl__Formula {
    short record, length ;
    short rw, col, ixfe ;
    double num ;
    short grbit ;
    int chn ;
    short cce ;
    char rgce[256] ;
   } lf ;
//  struct lcl__RefMode {
//    short record, length ;
//    short fRefA1 ;
//   } rm ;
#pragma pack()
  int bytes ;
  int refmode ;

	refmode = ws->RefMode ;				/* Save current refmode */
	bytes = v_ParseExcelFormula(ws,str,&lf.rgce,row,col) ;	/* Parse formula into lf.rgce */
//VEH041026 - Don't want to do this???
//	if (refmode != ws->RefMode)			/* Has RefMode changed ? */
//	 { rm.record = P2(0x0F) ; rm.length = P2(2) ; rm.fRefA1 = ws->RefMode ;
//	   wsAppend(ws,&rm,sizeof rm) ;
//	 } ;
	if (bytes == UNUSED)
	 { wsWriteString(ws,row,col,str,format) ; return ; } ;
	lf.record = P2(0x0406) ;
	lf.rw = P2(row) ; lf.col = P2(col) ; lf.ixfe = (format == NULL ? 0 : format->xf_index) ;
	memcpy(&lf.num,&result,sizeof result) ; lf.grbit = P2(0x01) ;
	lf.chn = P4(0x0) ; lf.cce = bytes ;
	lf.length = P2(22 + lf.cce) ;
	wsAppend(ws,&lf,lf.length + 4) ;

}


wsWriteBoolean(ws,row,col,logical,format)
  struct ss__WorkSheet *ws ;
  int row,col ;
  struct ss__Format *format ;
  int logical ;
{
#pragma pack(1)
  struct lcl__WriteBoolErr {
    short record,length ;
    short row,col,xf ;
    unsigned char bBoolErr, fError ;
   } wbe ;
#pragma pack()

	wbe.record = P2(0x0205) ; wbe.length = 8 ;
	wbe.row = P2(row) ; wbe.col = P2(col) ; wbe.xf = (format == NULL ? 0 : format->xf_index) ;
	wbe.fError = P2(0x00) ; wbe.bBoolErr = P2(logical) ;
	wsAppend(ws,&wbe,sizeof wbe) ;
}

wsWriteError(ws,row,col,errcode,format)
  struct ss__WorkSheet *ws ;
  int row,col ;
  struct ss__Format *format ;
  int errcode ;
{
#pragma pack(1)
  struct lcl__WriteBoolErr {
    short record,length ;
    short row,col,xf ;
    unsigned char bBoolErr, fError ;
   } wbe ;
#pragma pack()

	wbe.record = P2(0x0205) ; wbe.length = 8 ;
	wbe.row = P2(row) ; wbe.col = P2(col) ; wbe.xf = (format == NULL ? 0 : format->xf_index) ;
	wbe.fError = P2(0x01) ; wbe.bBoolErr = P2(errcode) ;
	wsAppend(ws,&wbe,sizeof wbe) ;
}

void wsWriteFooter(ws,string)
  struct ss__WorkSheet *ws ;
  char *string ;
{ int length ;
  struct lcl__Footer {
   short record,length ;
   unsigned char cch ;
   char rgch[256] ;
  } lf ;

	length = strlen(string) ; lf.cch = P2(length) ; strcpy(lf.rgch,string) ;
	lf.record = P2(0x15) ; lf.length = P2(1 + length) ;
	wsAppend(ws,&lf,length+1+4) ;
}

void wsWriteHeader(ws,string)
  struct ss__WorkSheet *ws ;
  char *string ;
{ int length ;
  struct lcl__Header {
   short record,length ;
   unsigned char cch ;
   char rgch[256] ;
  } lh ;

	length = strlen(string) ; lh.cch = P2(length) ; strcpy(lh.rgch,string) ;
	lh.record = P2(0x14) ; lh.length = P2(1 + length) ;
	wsAppend(ws,&lh,length+1+4) ;
}

wsWriteNumber(ws,row,col,num,format)
  struct ss__WorkSheet *ws ;
  int row,col ;
  struct ss__Format *format ;
  double num ;
{
#pragma pack(1)
  struct lcl__WriteNumber {
    short record,length ;
    short row,col,xf ;
    double num ;
   } wn ;
#pragma pack()

	wn.record = P2(0x0203) ; wn.length = P2(0x000e) ;
	wn.row = P2(row) ; wn.col = P2(col) ; wn.xf = (format == NULL ? 0 : format->xf_index) ;
	wn.num = PD(num) ;
	if (row < ws->dim_rowmin) ws->dim_rowmin = row ;
	if (row > ws->dim_rowmax) ws->dim_rowmax = row ;
	if (col < ws->dim_colmin) ws->dim_colmin = col ;
	if (col > ws->dim_colmax) ws->dim_colmax = col ;
	wsAppend(ws,&wn,sizeof wn) ;
//	memcpy(ws->data+ws->datasize,&wn,sizeof wn) ; ws->datasize += sizeof wn ;
	return(0) ;
/*
# write_number($row, $col, $num, $format)
#
# Write a double to the specified row and column (zero indexed).
# An integer can be written as a double. Excel will display an
# integer. $format is optional.
#
# Returns  0 : normal termination
#         -1 : insufficient number of arguments
#         -2 : row or column out of range
#
sub write_number {

    my $self      = shift;
    if (@_ < 3) { return -1 }

    my $record    = 0x0203;                 # Record identifier
    my $length    = 0x000E;                 # Number of bytes to follow

    my $row       = $_[0];                  # Zero indexed row
    my $col       = $_[1];                  # Zero indexed column
    my $num       = $_[2];
    my $xf        = _XF($_[3]);             # The cell format

    if ($row >= $self->{_xls_rowmax}) { return -2 }
    if ($col >= $self->{_xls_colmax}) { return -2 }
    if ($row <  $self->{_dim_rowmin}) { $self->{_dim_rowmin} = $row }
    if ($row >  $self->{_dim_rowmax}) { $self->{_dim_rowmax} = $row }
    if ($col <  $self->{_dim_colmin}) { $self->{_dim_colmin} = $col }
    if ($col >  $self->{_dim_colmax}) { $self->{_dim_colmax} = $col }

    my $header    = pack("vv",  $record, $length);
    my $data      = pack("vvv", $row, $col, $xf);
    my $xl_double = pack("d",   $num);

    if ($self->{_byte_order}) { $xl_double = reverse $xl_double }

    $self->_append($header, $data, $xl_double);

    return 0;
*/
}


wsWriteHLink(ws,row,col,string)
  struct ss__WorkSheet *ws ;
  int row,col ;
  char *string ;
{ int length ;
  static struct ss__Format *twformat=NULL ;
  struct lcl__HLink {
    short record,length ;
    short rwFirst,rwLast,colFirst,colLast ;
    char str[512] ;
   } hl ;

	length = strlen(string) ;
	hl.record = P2(0x0204) ; hl.length = P2(0x0008 + length) ;
	hl.rwLast = (hl.rwFirst = P2(row)) ; hl.colLast = (hl.colFirst = P2(col)) ;
	strcpy(hl.str,string) ;
	wsAppend(ws,&hl,0x0008 + length + 4) ;
}


void wsWriteString(ws,row,col,string,format)
  struct ss__WorkSheet *ws ;
  int row,col ;
  char *string ;
  struct ss__Format *format ;
{ int length ;
  static struct ss__Format *twformat=NULL ;
  struct lcl__WriteString {
    short record,length ;
    short row,col,xf,strlen ;
    char str[256] ;
   } lws ;
  struct lcl__BlankCell {
    short record,length ;
    short row,col ;
    short xf ;
   } bc ;

	length = strlen(string) ; lws.strlen = P2(length) ;
	if (length > 255) length = 255 ;		/* VEH130109 - don't allow to go over the max */
	if (length == 0)
	 { bc.record = P2(0x0201) ; bc.length = P2(6) ;
	   bc.row = P2(row) ; bc.col = P2(col) ;
	   bc.xf =  (format == NULL ? 0 : format->xf_index) ;
	   wsAppend(ws,&bc,sizeof bc) ;
	   return ;
	 } ;
	lws.record = P2(0x0204) ; lws.length = P2(0x0008 + length) ;
	lws.row = P2(row) ; lws.col = P2(col) ;
	lws.xf =  (format == NULL ? 0 : format->xf_index) ;
	if (row < ws->dim_rowmin) ws->dim_rowmin = row ;
	if (row > ws->dim_rowmax) ws->dim_rowmax = row ;
	if (col < ws->dim_colmin) ws->dim_colmin = col ;
	if (col > ws->dim_colmax) ws->dim_colmax = col ;
	strncpy(lws.str,string,length) ; lws.str[length] = '\0' ;
//	for (i=0;;i++)				/* Copy string - check for newline */
//	 { lws.str[i] = string[i] ; if (string[i] == '\0') break ;
//	   if (string[i] != 0x0a) continue ;	/* Got new line? - have to force format to "word_wrap" */
//	   if (format != NULL)
//	    { format->text_wrap = TRUE ; continue ; } ;
//	   if (twformat == NULL) twformat = wbAddFormat(ws->wb) ;
//	   twformat->text_wrap = TRUE ; lws.xf = twformat->xf_index ;
//	 } ;
	wsAppend(ws,&lws,0x0008 + length + 4) ;
//	memcpy(ws->data+ws->datasize,&lws,sizeof lws) ; ws->datasize += sizeof lws ;
//	memcpy(ws->data+ws->datasize,string,lws.strlen) ; ws->datasize += lws.strlen ;

/*
# write_string ($row, $col, $string, $format)
#
# Write a string to the specified row and column (zero indexed).
# NOTE: there is an Excel 5 defined limit of 255 characters.
# $format is optional.
# Returns  0 : normal termination
#         -1 : insufficient number of arguments
#         -2 : row or column out of range
#         -3 : long string truncated to 255 chars
#
sub write_string {

    my $self      = shift;
    if (@_ < 3) { return -1 }

    my $record    = 0x0204;                 # Record identifier
    my $length    = 0x0008 + length($_[2]); # Bytes to follow

    my $row       = $_[0];                  # Zero indexed row
    my $col       = $_[1];                  # Zero indexed column
    my $strlen    = length($_[2]);
    my $str       = $_[2];
    my $xf        = _XF($_[3]);             # The cell format

    my $str_error = 0;

    if ($row >= $self->{_xls_rowmax}) { return -2 }
    if ($col >= $self->{_xls_colmax}) { return -2 }
    if ($row <  $self->{_dim_rowmin}) { $self->{_dim_rowmin} = $row }
    if ($row >  $self->{_dim_rowmax}) { $self->{_dim_rowmax} = $row }
    if ($col <  $self->{_dim_colmin}) { $self->{_dim_colmin} = $col }
    if ($col >  $self->{_dim_colmax}) { $self->{_dim_colmax} = $col }

    if ($strlen > $self->{_xls_strmax}) { # LABEL must be < 255 chars
        $str       = substr($str, 0, $self->{_xls_strmax});
        $length    = 0x0008 + $self->{_xls_strmax};
        $strlen    = $self->{_xls_strmax};
        $str_error = -3;
    }

    my $header    = pack("vv",   $record, $length);
    my $data      = pack("vvvv", $row, $col, $xf, $strlen);

    $self->_append($header, $data, $str);

    return $str_error;
*/
}

wsWriteBlank(ws,row,col,format)
  struct ss__WorkSheet *ws ;
  int row,col,format ;
{
  struct lcl__WriteBlank {
    short record,length ;
    short row,col,xf ;
   } wb ;

	wb.record = P2(0x0201) ; wb.length = P2(0x0006) ;
	if (row < ws->dim_rowmin) ws->dim_rowmin = row ;
	if (row > ws->dim_rowmax) ws->dim_rowmax = row ;
	if (col < ws->dim_colmin) ws->dim_colmin = col ;
	if (col > ws->dim_colmax) ws->dim_colmax = col ;

/*
###############################################################################
#
# write_blank($row, $col, $format)
#
# Write a blank cell to the specified row and column (zero indexed).
# A blank cell is used to specify formatting without adding a string
# or a number. $format is optional.
#
# Returns  0 : normal termination
#         -1 : insufficient number of arguments
#         -2 : row or column out of range
#
sub write_blank {

    my $self      = shift;
    if (@_ < 2) { return -1 }

    my $record    = 0x0201;                 # Record identifier
    my $length    = 0x0006;                 # Number of bytes to follow

    my $row       = $_[0];                  # Zero indexed row
    my $col       = $_[1];                  # Zero indexed column
    my $xf        = _XF($_[2]);             # The cell format

    if ($row >= $self->{_xls_rowmax}) { return -2 }
    if ($col >= $self->{_xls_colmax}) { return -2 }
    if ($row <  $self->{_dim_rowmin}) { $self->{_dim_rowmin} = $row }
    if ($row >  $self->{_dim_rowmax}) { $self->{_dim_rowmax} = $row }
    if ($col <  $self->{_dim_colmin}) { $self->{_dim_colmin} = $col }
    if ($col >  $self->{_dim_colmax}) { $self->{_dim_colmax} = $col }

    my $header    = pack("vv",  $record, $length);
    my $data      = pack("vvv", $row, $col, $xf);

    $self->_append($header, $data);

    return 0;
*/
}

//------------- N E E D   W O R K -----------------------
wsWriteURL(ws,row,col,url,string,format)
  struct ss__WorkSheet *ws ;
  int row,col,format ;
  char *url,*string ;
{

//	needcodehere ;
/*
###############################################################################
#
# write_url($row, $col, $url, $string, $format )
#
# Write a hyperlink. This is comprised of two elements: the visible label and
# the invisible link. The visible label is the same as the link unless an
# alternative string is specified. The label is written using the
# write_string() method. Therefore the 255 characters string limit applies.
# $string and $format are optional.
#
# Returns  0 : normal termination
#         -1 : insufficient number of arguments
#         -2 : row or column out of range
#         -3 : long string truncated to 255 chars
#
sub write_url {
    my $self      = shift;
    if (@_ < 3) { return -1 }                    # Check the number of args

    my $record  = 0x01B8;                        # Record identifier
    my $length  = 0x0034 + 2*(1+length($_[2]));  # Bytes to follow

    my $row     = $_[0];                         # Zero indexed row
    my $col     = $_[1];                         # Zero indexed column
    my $url     = $_[2];                         # URL string
    my $str     = $_[3] || $_[2];                # Alternative label
    my $xf      = $_[4] || $self->{_url_format}; # The cell format


    # Write the visible label using the write_string() method.
    my $str_error = $self->write_string($row, $col, $str, $xf);
    return $str_error if $str_error == -2;


    # Pack the header data
    my $header  = pack("vv",   $record, $length);
    my $data    = pack("vvvv", $row, $row, $col, $col);


    # Pack the undocumented part of the hyperlink stream, 40 bytes.
    my $unknown = "D0C9EA79F9BACE118C8200AA004BA90B02000000";
    $unknown   .= "03000000E0C9EA79F9BACE118C8200AA004BA90B";
    my $stream  = pack("H*", $unknown);


    # Convert URL to a null terminated wchar string
    $url        = join("\0", split('', $url));
    $url        = $url . "\0\0\0";


    # Pack the length of the URL
    my $url_len = pack("V", length($url));


    # Write the packed data
    $self->_append($header, $data);
    $self->_append($stream);
    $self->_append($url_len);
    $self->_append($url);

    return $str_error;
*/
}


wsSetRow(ws,row,height,xf)
  struct ss__WorkSheet *ws ;
  int row,height ;
  struct ss__Format *xf ;
{
  struct lcl__SetRow {
    short record, length ;
    short rw, colMic, colMac, miyRw, irwMac, reserved, grbit, ixfe ;
   } sr ;

	memset(&sr,0,sizeof sr) ;
	sr.record = P2(0x0208) ; sr.length = P2(0x0010) ;
	sr.rw = P2(row) ; sr.grbit = P2(0x01c0) ;
	sr.ixfe = xf->xf_index ;
	if (height == UNUSED) { sr.miyRw = P2(0xff) ; }
	 else { sr.miyRw = P2(height * 20) ; } ;
	wsAppend(ws,&sr,sizeof sr) ;
//	memcpy(ws->data+ws->datasize,&sr,sizeof sr) ; ws->datasize += sizeof sr ;
/*
# set_row($row, $height, $XF)
#
# This method is used to set the height and XF format for a row.
# Writes the  BIFF record ROW.
#
sub set_row {

    my $self        = shift;
    my $record      = 0x0208;               # Record identifier
    my $length      = 0x0010;               # Number of bytes to follow

    my $rw          = $_[0];                # Row Number
    my $colMic      = 0x0000;               # First defined column
    my $colMac      = 0x0000;               # Last defined column
    my $miyRw;                              # Row height
    my $irwMac      = 0x0000;               # Used by Excel to optimise loading
    my $reserved    = 0x0000;               # Reserved
    my $grbit       = 0x01C0;               # Option flags. (monkey) see $1 do
    my $ixfe        = _XF($_[2]);           # XF index

    # Use set_row($row, undef, $XF) to set XF without setting height
    if (defined ($_[1])) {
        $miyRw = $_[1] *20;
    }
    else {
        $miyRw = 0xff;
    }

    my $header   = pack("vv",       $record, $length);
    my $data     = pack("vvvvvvvv", $rw, $colMic, $colMac, $miyRw,
                                    $irwMac,$reserved, $grbit, $ixfe);

    $self->_append($header, $data);
*/
}


/*	wbMergeFormats - Merges attributes of new format2 into saved format2 & returns possible new format */

struct ss__Format *wbMergeFormats(wb,format1,format2)
  struct ss__WorkBook *wb ;
  struct ss__Format *format1 ;
  struct ss__Format *format2 ;
{
  struct ss__Format nformat ;
  struct ss__Format *fp ;
  int xfi,i ;

	if (format2 == NULL) return(format1) ;		/* If no new format then just use first one */
	if (format1 == NULL) { memcpy(&nformat,format2,sizeof nformat) ; }
	 else { memcpy(&nformat,format1,sizeof nformat) ;
		if (strlen(format2->mask) > 0) strcpy(nformat.mask,format2->mask) ;
		if (format2->bold != 0) nformat.bold = format2->bold ;
		nformat.italic |= format2->italic ;
		nformat.font_strikeout |= format2->font_strikeout ;
		nformat.text_wrap |= format2->text_wrap ;
		nformat.size_to_fit |= format2->size_to_fit ;
		if (format2->text_h_align != 0) nformat.text_h_align = format2->text_h_align ;
		if (format2->text_v_align != 0) nformat.text_v_align = format2->text_v_align ;
		if (format2->fg_color != 0) nformat.fg_color = format2->fg_color ;
		if (format2->bg_color != 0) nformat.bg_color = format2->bg_color ;
		if (format2->pattern != 0) nformat.pattern = format2->pattern ;
		if (format2->color != 0) nformat.color = format2->color ;
		if (format2->size != 0) nformat.size = format2->size ;
	      } ;
/*	See if we already have this format */
	for(i=0;i<wb->formatCount;i++)
	 { nformat.xf_index = wb->format[i]->xf_index ; if (memcmp(wb->format[i],&nformat,sizeof nformat) == 0) return(wb->format[i]) ; } ;
/*	Have to make a new format */
	fp = wbAddFormat(wb) ; xfi = fp->xf_index ;
	memcpy(fp,&nformat,sizeof nformat) ; fp->xf_index = xfi ;
	return(fp) ;
} ;

struct ss__WorkBook *wbNew(filename)
  char *filename ;
{ struct ss__WorkBook *wb ;

	wb = (struct ss__WorkBook *)v4mm_AllocChunk(sizeof *wb,TRUE) ;
	wb->data = v4mm_AllocChunk(WB_DataSize,FALSE) ;
	wb->tmpformat = wbAddFormat(wb) ;
	wb->ole = oleNew(filename) ;
	wb->D1904 = 0 ;
	wb->xf_index = 16 ;
	strcpy(wb->SheetName,"Sheet") ;
	wb->nextfontindex = 6 ;

	return(wb) ;
/*
# new()
#
# Constructor. Creates a new Workbook object from a BIFFwriter object.
#
sub new {

    my $class       = shift;
    my $filename    = $_[0] || '';
    my $self        = Spreadsheet::WriteExcel::BIFFwriter->new();
    my $ole_writer  = Spreadsheet::WriteExcel::OLEwriter->new($filename);
    #my $tmp_sheet   = Spreadsheet::WriteExcel::Worksheet->new('', 0, 0);
    my $tmp_format  = Spreadsheet::WriteExcel::Format->new();

    my $byte_order  = $self->{_byte_order};
    my $parser      = Spreadsheet::WriteExcel::Formula->new($byte_order);

    $self->{_OLEwriter}         = $ole_writer;
    $self->{_parser}            = $parser;
    $self->{_1904}              = 0;
    $self->{_activesheet}       = 0;
    $self->{_firstsheet}        = 0;
    $self->{_xf_index}          = 16; # 15 style XF's and 1 cell XF.
    $self->{_fileclosed}        = 0;
    $self->{_biffsize}          = 0;
    $self->{_sheetname}         = "Sheet";
    #$self->{_tmp_worksheet}     = $tmp_sheet;
    $self->{_tmp_format}        = $tmp_format;
    $self->{_url_format}        = '';
    $self->{_worksheets}        = [];
    $self->{_formats}           = [];

    bless $self, $class;

    # Add the default format for hyperlinks
    my $url_format = $self->addformat();
    $url_format->set_color('blue');
    $url_format->set_underline(1);
    $self->{_url_format} = $url_format;

    $self->_tmpfile_warning();

    return $self;
*/
}

void wbPrepend(wb,data,bytes)
  struct ss__WorkBook *wb ;
  char *data ; int bytes ;
{

	memmove(wb->data+bytes,wb->data,wb->datasize) ; memcpy(wb->data,data,bytes) ; wb->datasize += bytes ;
}

void wbAppend(wb,data,bytes)
  struct ss__WorkBook *wb ;
  char *data ; int bytes ;
{
  struct lcl__ls {
    short type,length ;
   } *ls ;
  int tot ;
	ls = (struct lcl__ls *)data ;
	if (ls->length != bytes - 4)
	 printf("something not right\n") ;
	if (wb->datasize + bytes > WB_DataSize)
	 { printf("Exceeded max wb size\n") ; exit(EXITABORT) ; } ;
	memcpy(wb->data+wb->datasize,data,bytes) ; wb->datasize += bytes ;
	ls = (struct lcl__ls *)wb->data ; tot = 0 ;
	for(;tot<wb->datasize;)
	 { tot += ls->length + 4 ; ls = (struct lcl__ls *)((char *)ls + ls->length + 4) ;
	 } ;
	if (tot != wb->datasize)
	 printf("tot = %d, wb->bytes = %d\n",tot,wb->datasize) ;
}

void wbClose(wb)
  struct ss__WorkBook *wb ;
{

	if (wb->fileclosed) return ;
	wbStoreWorkbook(wb) ;
	oleClose(wb->ole) ;
	wb->fileclosed = TRUE ;
/*
# close()
#
# Calls finalization methods and explicitly close the OLEwriter file
# handle.
#
sub close {

    my $self = shift;

    return if $self->{_fileclosed}; # Prevent calling close() twice

    $self->_store_workbook();
    $self->{_OLEwriter}->close();
    $self->{_fileclosed} = 1;
*/
}



void wbStoreBOF(wb,type)
  struct ss__WorkBook *wb ;
  int type ;
{
  struct lcl__BOF {
    short record,length ;
    short version,type,build,year ;
   } lBOF ;

	lBOF.record = P2(0x0809) ; lBOF.length = P2(0x0008) ;
	lBOF.version = P2(BIFF_VERSION) ; lBOF.type = P2(type) ;
	lBOF.build = P2(0x096c) ; lBOF.year = P2(0x07C9) ;
	wbPrepend(wb,&lBOF,sizeof lBOF) ;
//	memcpy(wb->data+wb->datasize,&lBOF,sizeof lBOF) ; wb->datasize += sizeof lBOF ;
}

void wbStoreEOF(wb)
  struct ss__WorkBook *wb ;
{
  struct lcl__EOF {
    short record,length ;
   } lEOF ;

	lEOF.record = P2(0x000A) ; lEOF.length = P2(0x0000) ;
	wbAppend(wb,&lEOF,sizeof lEOF) ;
//	memcpy(wb->data+wb->datasize,&lEOF,sizeof lEOF) ; wb->datasize += sizeof lEOF ;
}








//------------- N E E D   W O R K -----------------------
/*
#
# DESTROY()
#
# Close the workbook if it hasn't already been explicitly closed.
#
sub DESTROY {

    my $self = shift;

    $self->close() if not $self->{_fileclosed};
}

*/

//------------- N E E D   W O R K -----------------------
/*
#
# worksheets()
#
# An accessor for the _worksheets[] array
#
# Returns: an array reference
#
sub worksheets {

    my $self = shift;

    return $self->{_worksheets};
    }
}
*/


struct ss__WorkSheet *wbAddWorkSheet(wb,name,maxbytes)
  struct ss__WorkBook *wb ;
  char *name ;
  int maxbytes ;
{
	if (wb->wsCount >= WB_WorkSheetMax)
	 { printf("Exceeded maximum (%d) number of worksheets... appending to current",WB_WorkSheetMax) ; }
	 else { wb->wsCount ++ ; } ;
	wb->ws[wb->wsCount-1] = wsNew(name,wb->wsCount-1,wb->activesheet,wb->firstsheet,wb->urlformat,wb->parser,maxbytes) ;
	wb->ws[wb->wsCount-1]->wb = wb ;
	return(wb->ws[wb->wsCount-1]) ;
/*
# addworksheet()
#
# Add a new worksheet to the Excel workbook.
# TODO: add accessor for $self->{_sheetname} to mimic international
# versions of Excel.
#
# Returns: reference to a worksheet object
#
sub addworksheet {

    my $self      = shift;
    my $name      = $_[0] || "";
    my $index     = @{$self->{_worksheets}};
    my $sheetname = $self->{_sheetname};

    if ($name eq "" ) { $name = $sheetname . ($index+1) }

    my @init_data = (
                        $name,
                        $index,
                        \$self->{_activesheet},
                        \$self->{_firstsheet},
                        $self->{_url_format},
                        $self->{_parser},
                    );

    my $worksheet = Spreadsheet::WriteExcel::Worksheet->new(@init_data);
    $self->{_worksheets}->[$index] = $worksheet;
    return $worksheet;
*/
}


struct ss__Format *wbAddFormat(wb)
  struct ss__WorkBook *wb ;
{ 

	if (wb->formatCount >= WB_FormatMax)
	 { printf("Exceeded max of %d format specifications\n",WB_FormatMax) ; exit(EXITABORT) ; } ;
	wb->formatCount++ ;
	wb->format[wb->formatCount-1] = formatNew(wb->xf_index+wb->formatCount-1,NULL) ;
	return(wb->format[wb->formatCount-1]) ;
/*
###############################################################################
#
# addformat()
#
# Add a new format to the Excel workbook. This adds an XF record and
# a FONT record.
#
sub addformat {

    my $self      = shift;

    my $format = Spreadsheet::WriteExcel::Format->new($self->{_xf_index});
    $self->{_xf_index} += 1;

    push @{$self->{_formats}}, $format;

    return $format;
*/
}


/*
###############################################################################
#
# set_1904()
#
# Set the date system: 0 = 1900 (the default), 1 = 1904
#
sub set_1904{

    my $self      = shift;

    if (defined($_[0])) {
        $self->{_1904} = $_[0];
    }
    else {
        $self->{_1904} = 1;
    }
}


###############################################################################
#
# get_1904()
#
# Return the date system: 0 = 1900, 1 = 1904
#
sub get_1904{

    my $self = shift;

    return $self->{_1904};
}
*/


/*	Do we need this ??????

###############################################################################
#
# write()
#
# Calls write method on first worksheet for backward compatibility.
# Adds first worksheet as necessary.
#
# Returns: return value of the worksheet->write() method
#
sub write {

    my $self    = shift;

    if (@{$self->{_worksheets}} == 0) { $self->addworksheet() }
    carp("Calling write() methods on a workbook object is deprecated," .
         " use write() in conjunction with a worksheet object instead"
        ) if $^W;
    return $self->{_worksheets}[0]->write(@_);
}


###############################################################################
#
# write_string()
#
# Calls write_string method on first worksheet for backward
# compatibility. Adds first worksheet as necessary.
#
# Returns: return value of the worksheet->write_string() method
#
sub write_string {

    my $self    = shift;

    if (@{$self->{_worksheets}} == 0) { $self->addworksheet() }
    carp("Calling write() methods on a workbook object is deprecated," .
         " use write() in conjunction with a worksheet object instead"
        ) if $^W;
    return $self->{_worksheets}[0]->write_string(@_);
}


###############################################################################
#
# write_number()
#
# Calls write_number method on first worksheet for backward
# compatibility. Adds first worksheet as necessary.
#
# Returns: return value of the worksheet->write_number() method
#
sub write_number {

    my $self    = shift;

    if (@{$self->{_worksheets}} == 0) { $self->addworksheet() }
    carp("Calling write() methods on a workbook object is deprecated," .
         " use write() in conjunction with a worksheet object instead"
        ) if $^W;
    return $self->{_worksheets}[0]->write_number(@_);
}
*/


/*
###############################################################################
#
# _tmpfile_warning()
#
# Check that tmp files can be created for use in Worksheet.pm. A CGI, mod_perl
# or IIS might not have permission to create tmp files. The test is here rather
# than in Worksheet.pm so that only one warning is given.
#
sub _tmpfile_warning{

    my $fh = IO::File->new_tmpfile();

    if ((not defined $fh) && ($^W)) {
        carp("Unable to create tmp files via IO::File->new_tmpfile(). " .
             "Storing data in memory ")
    }
}
*/


void wbCalcSheetOffsets(wb)
  struct ss__WorkBook *wb ;
{ int i ;
  int bof, eof, offset ;

	bof = 11 ; eof = 4 ; offset = wb->datasize ;
	for(i=0;i<wb->wsCount;i++)
	 { offset += bof + strlen(wb->ws[i]->name) ; } ;
	offset += eof ;
	for(i=0;i<wb->wsCount;i++)
	 { wb->ws[i]->offset = offset ;
	   offset += wb->ws[i]->datasize ;
	 } ;
	wb->biffsize = offset ;
/*
# _calc_sheet_offsets()
#
# Calculate offsets for Worksheet BOF records.
#
sub _calc_sheet_offsets {

    my $self    = shift;
    my $BOF     = 11;
    my $EOF     = 4;
    my $offset  = $self->{_datasize};

    foreach my $sheet (@{$self->{_worksheets}}) {
        $offset += $BOF + length($sheet->{name});
    }

    $offset += $EOF;

    foreach my $sheet (@{$self->{_worksheets}}) {
        $sheet->{_offset} = $offset;
        $offset += $sheet->{_datasize};
    }

    $self->{_biffsize} = $offset;
*/
}


void wbStoreWorkbook(wb)
  struct ss__WorkBook *wb ;
{ struct ss__OLE *ole ;
  int i ;

	for(i=0;i<wb->wsCount;i++)
	 { wsClose(wb->ws[i]) ; } ;
	wbStoreBOF(wb,0x0005) ;
	wbStoreWindow1(wb) ;
	wbStore1904(wb) ;
	wbStoreAllFonts(wb) ;
	wbStoreAllNumFormats(wb) ;
	wbStoreAllXFS(wb) ;
	wbStoreAllStyles(wb) ;
	wbCalcSheetOffsets(wb) ;
	for(i=0;i<wb->wsCount;i++) { wbStoreBoundSheet(wb,wb->ws[i]->name,wb->ws[i]->offset) ; } ;
	wbStoreEOF(wb) ;
	ole = wb->ole ;
	if (oleSetSize(ole,wb->biffsize))
	 { oleWriteHeader(ole) ;
	   oleWrite(ole,wb->data,wb->datasize) ;
	   for(i=0;i<wb->wsCount;i++)
	    { oleWrite(ole,wb->ws[i]->data,wb->ws[i]->datasize) ;
	    } ;
	 } ;
/*
# _store_workbook()
#
# Assemble worksheets into a workbook and send the BIFF data to an OLE
# storage.
#
sub _store_workbook {

    my $self = shift;
    my $OLE  = $self->{_OLEwriter};

    # Call the finalization methods for each worksheet
    foreach my $sheet (@{$self->{_worksheets}}) {
        $sheet->_close();
    }

    # Add Workbook globals
    $self->_store_bof(0x0005);
    $self->_store_window1();
    $self->_store_1904();
    $self->_store_all_fonts();
    $self->_store_all_num_formats();
    $self->_store_all_xfs();
    $self->_store_all_styles();
    $self->_calc_sheet_offsets();

    # Add BOUNDSHEET records
    foreach my $sheet (@{$self->{_worksheets}}) {
        $self->_store_boundsheet($sheet->{name}, $sheet->{_offset});
    }

    # End Workbook globals
    $self->_store_eof();

    # Write Worksheet data if data <~ 7MB
    if ($OLE->set_size($self->{_biffsize})) {
        $OLE->write_header();
        $OLE->write($self->{_data});

        foreach my $sheet (@{$self->{_worksheets}}) {
            while (my $tmp = $sheet->get_data()) {
                $OLE->write($tmp);
            }
        }
    }
*/
}


void wbStoreAllFonts(wb)
  struct ss__WorkBook *wb ;
{ struct ss__Format *format ;
  struct ss__Font *font ;
//#define KHMax 255
//  struct {
//    int Count ;
//    int Key[KHMax], Index[KHMax] ;
//   } kh ;
  int i,j,key ;
//  int index ;

//	kh.Count = 0 ;
//	index = 6 ;
	format = wb->tmpformat ;
	font = formatGetFont(format) ;

	for(i=1;i<=5;i++)
	 { wbAppend(wb,font,UP2(font->length)+4) ; } ;
//	   memcpy(wb->data+wb->datasize,font,sizeof *font) ; wb->datasize += sizeof *font ; } ;
	key = formatGetFontKey(format) ;
//	kh.Key[kh.Count] = key ; kh.Index[kh.Count] = 0 ; kh.Count++ ;
	wb->fe[wb->fontcount].Key = key ; wb->fe[wb->fontcount].Index = 0 ;
	wb->fe[wb->fontcount].font = NULL ; wb->fontcount ++ ;
	for(i=0;i<wb->formatCount;i++)
	 { key = formatGetFontKey(wb->format[i]) ;
	   for(j=0;j<wb->fontcount;j++)
	    { if (wb->fe[j].Key != key) continue ;
	      wb->format[i]->font_index = wb->fe[j].Index ; break ;
	    } ; if (j < wb->fontcount) continue ;
	   wb->fe[wb->fontcount].Key = key ; wb->fe[wb->fontcount].Index = (wb->nextfontindex++) ;
	   wb->format[i]->font_index = wb->fe[wb->fontcount].Index ;
	   font = formatGetFont(wb->format[i]) ;
	   wb->fe[wb->fontcount].font = v4mm_AllocChunk(sizeof *font,FALSE) ;
	   memcpy(wb->fe[wb->fontcount].font,font,sizeof *font) ;
	   wb->fontcount++ ;
//	   wbAppend(wb,font,UP2(font->length)+4) ;
//	   memcpy(wb->data+wb->datasize,font,sizeof *font) ; wb->datasize += sizeof *font ;
	 } ;

	for(i=0;i<wb->fontcount;i++)
	 { if (wb->fe[i].font != NULL) wbAppend(wb,wb->fe[i].font,UP2(font->length)+4) ;
	 } ;

/*
# _store_all_fonts()
#
# Store the Excel FONT records.
#
sub _store_all_fonts {

    my $self   = shift;

    # _tmp_format is added by new(). We use this to write the default XF's
    my $format = $self->{_tmp_format};
    my $font   = $format->get_font();

    # Note: Fonts are 0-indexed. According to the SDK there is no index 4,
    # so the following fonts are 0, 1, 2, 3, 5
    #
    for (1..5){
        $self->_append($font);
    }


    # Iterate through the XF objects and write a FONT record if it isn't the
    # same as the default FONT and if it hasn't already been used.
    #
    my %fonts;
    my $key;
    my $index = 6;                  # The first user defined FONT

    $key = $format->get_font_key(); # The default font from _tmp_format
    $fonts{$key} = 0;               # Index of the default font


    foreach $format (@{$self->{_formats}}) {
        $key = $format->get_font_key();

        if (exists $fonts{$key}) {
            # FONT has already been used
            $format->{_font_index} = $fonts{$key};
        }
        else {
            # Add a new FONT record
            $fonts{$key}           = $index;
            $format->{_font_index} = $index;
            $index++;
            $font = $format->get_font();
            $self->_append($font);
        }
    }
*/
}

//------------- N E E D   W O R K -----------------------
void wbStoreAllNumFormats(wb)
  struct ss__WorkBook *wb ;
{ struct ss__Format *format ;

  struct {
    int Count ;
    struct {
      char mask[64] ;
     } Entry[200] ;
   } anf ;
  int i,j ; int index ;

	anf.Count = 0 ;
	index = 164 ;
	for(i=0;i<wb->formatCount;i++)
	 { format = wb->format[i] ;
	   if (format->num_format != 0) continue ;	/* Built in numeric format */
	   if (strlen(format->mask) == 0) { format->num_format = 0 ; continue ; } ; /* If no mask then default to "General" */
	   for (j=0;j<anf.Count;j++)
	    { if (strcmp(format->mask,anf.Entry[j].mask) != 0) continue ;
	      format->num_format = j + index ; break ;
	    } ; if (j < anf.Count) continue ;
	   strcpy(anf.Entry[anf.Count].mask,format->mask) ; format->num_format = index + anf.Count ;
	   anf.Count++ ;
	 } ;
/*	Now write out all formats in anf */
	for(i=0;i<anf.Count;i++)
	 { wbStoreNumFormat(wb,anf.Entry[i].mask,i+index) ;
	 } ;
/*
# _store_all_num_formats()
#
# Store user defined numerical formats i.e. FORMAT records
#
sub _store_all_num_formats {

    my $self   = shift;

    # Leaning num_format syndrome
    my %num_formats;
    my @num_formats;
    my $num_format;
    my $index = 164;

    # Iterate through the XF objects and write a FORMAT record if it isn't a
    # built-in format type and if the FORMAT string hasn't already been used.
    #
    foreach my $format (@{$self->{_formats}}) {
        my $num_format = $format->{_num_format};
        
        # Check if $num_format is an index to a builtin format.
        # Also check for a string of zeros, which is a valid format string
        # but would evaluate to zero
        #
        if ($num_format !~ m/^0+\d/) {
            next if $num_format =~ m/^\d+$/; # builtin
        }

        if (exists($num_formats{$num_format})) {
            # FORMAT has already been used
            $format->{_num_format} = $num_formats{$num_format};
        }
        else{
            # Add a new FORMAT
            $num_formats{$num_format} = $index;
            $format->{_num_format}    = $index;
            push @num_formats, $num_format;
            $index++;
        }
    }

    # Write the new FORMAT records starting from 0xA4
    $index = 164;
    foreach $num_format (@num_formats) {
        $self->_store_num_format($num_format, $index);
        $index++;
    }
*/
}

void wbStoreAllXFS(wb)
  struct ss__WorkBook *wb ;
{ struct ss__Format *format ;
  struct ss__XF *xf ;
  int i ;

	format = wb->tmpformat ;
	for(i=0;i<=14;i++)			/* Style XF */
	 { xf = formatGetXF(format,0xfff5) ;
	   wbAppend(wb,xf,sizeof *xf) ;
//	   memcpy(wb->data+wb->datasize,xf,sizeof *xf) ; wb->datasize += sizeof xf ;
	 } ;
	xf = formatGetXF(format,0x0001) ;	/* Cell XF */
	wbAppend(wb,xf,sizeof *xf) ;
//	memcpy(wb->data+wb->datasize,xf,sizeof *xf) ; wb->datasize += sizeof xf ;

	for(i=0;i<wb->formatCount;i++)
	 { xf = formatGetXF(wb->format[i],0x0001) ;
	   wbAppend(wb,xf,sizeof *xf) ;
//	   memcpy(wb->data+wb->datasize,xf,sizeof *xf) ; wb->datasize += sizeof xf ;
	 } ;
/*
# _store_all_xfs()
#
# Write all XF records.
#
sub _store_all_xfs {

    my $self    = shift;

    # _tmp_format is added by new(). We use this to write the default XF's
    # The default font index is 0
    #
    my $format = $self->{_tmp_format};
    my $xf;

    for (0..14) {
        $xf = $format->get_xf(0xFFF5); # Style XF
        $self->_append($xf);
    }

    $xf = $format->get_xf(0x0001);     # Cell XF
    $self->_append($xf);


    # User defined XFs
    foreach $format (@{$self->{_formats}}) {
        $xf = $format->get_xf(0x0001);
        $self->_append($xf);
    }
*/
}

void wbStoreAllStyles(wb)
  struct ss__WorkBook *wb ;
{

	wbStoreStyle(wb) ;
/*
# _store_all_styles()
#
# Write all STYLE records.
#
sub _store_all_styles {

    my $self    = shift;

    $self->_store_style();
*/
}


void wbStoreWindow1(wb)
  struct ss__WorkBook *wb ;
{
  struct lcl__StoreWindow {
    short record,length ;
    short xWn, yWn, dxWn, dyWn ;
    short grbit ;
    short itabCur, itabFirst, ctabsel, wTabRatio ;
   } sw ;

	sw.record = P2(0x003d) ; sw.length = P2(0x0012) ;
	sw.xWn = P2(0x0000) ; sw.yWn = P2(0x0000) ;
	sw.dxWn = P2(0x25bc) ; sw.dyWn = P2(0x1572) ;
	sw.grbit = P2(0x0038) ; sw.ctabsel = P2(0x0001) ; sw.wTabRatio = P2(0x0258) ;
	sw.itabFirst = P2(wb->firstsheet) ;
	sw.itabCur = P2(wb->activesheet) ;
	wbAppend(wb,&sw,sizeof sw) ;
//	memcpy(wb->data+wb->datasize,&sw,sizeof sw) ; wb->datasize += sizeof sw ;
/*
# _store_window1()
#
# Write Excel BIFF WINDOW1 record.
#
sub _store_window1 {

    my $self      = shift;
    my $record    = 0x003D;  # Record identifier
    my $length    = 0x0012;  # Number of bytes to follow

    my $xWn       = 0x0000;  # Horizontal position of window
    my $yWn       = 0x0000;  # Vertical position of window
    my $dxWn      = 0x25BC;  # Width of window
    my $dyWn      = 0x1572;  # Height of window

    my $grbit     = 0x0038;  # Option flags
    my $ctabsel   = 0x0001;  # Number of workbook tabs selected
    my $wTabRatio = 0x0258;  # Tab to scrollbar ratio

    my $itabFirst = $self->{_firstsheet};  # 1st displayed worksheet
    my $itabCur   = $self->{_activesheet}; # Selected worksheet

    my $header    = pack("vv",        $record, $length);
    my $data      = pack("vvvvvvvvv", $xWn, $yWn, $dxWn, $dyWn,
                                      $grbit,
                                      $itabCur, $itabFirst,
                                      $ctabsel, $wTabRatio);

    $self->_append($header, $data);
*/
}


void wbStoreBoundSheet(wb,name,offset)
  struct ss__WorkBook *wb ;
  char *name ;
  int offset ;
{
#pragma pack(1)
  struct lcl__BoundSheet {
    short record,length ;
    int offset ;
    short grbit ;
    unsigned char cch ;
    char sheetname[128] ;
   } bs ;
#pragma pack()

	bs.record = P2(0x0085) ; bs.length = P2(0x07 + strlen(name)) ;
	bs.offset = P2(offset) ; bs.grbit = P2(0x0000) ; bs.cch = strlen(name) ;
	strcpy(bs.sheetname,name) ;
	wbAppend(wb,&bs,UP2(bs.length)+4) ;
//	memcpy(wb->data,&bs,sizeof bs) ; wb->datasize += sizeof bs ;

/*
# _store_boundsheet()
#
# Writes Excel BIFF BOUNDSHEET record.
#
sub _store_boundsheet {

    my $self      = shift;

    my $record    = 0x0085;               # Record identifier
    my $length    = 0x07 + length($_[0]); # Number of bytes to follow

    my $sheetname = $_[0];                # Worksheet name
    my $offset    = $_[1];                # Location of worksheet BOF
    my $grbit     = 0x0000;               # Sheet identifier
    my $cch       = length($sheetname);   # Length of sheet name

    my $header    = pack("vv",  $record, $length);
    my $data      = pack("VvC", $offset, $grbit, $cch);

    $self->_append($header, $data, $sheetname);
*/
}

void wbStoreStyle(wb)
  struct ss__WorkBook *wb ;
{
#pragma pack(1)
  struct lcl__Style {
    short record,length ;
    short ixfe ;
    unsigned char BuildIn,iLevel ;
   } ls ;
#pragma pack()

	ls.record = P2(0x0293) ; ls.length = P2(0x0004) ;
	ls.ixfe = P2(0x8000) ; ls.BuildIn = 0x00 ; ls.iLevel = 0xff ;
	wbAppend(wb,&ls,sizeof ls) ;
//	memcpy(wb->data+wb->datasize,&ls,sizeof ls) ; wb->datasize += sizeof ls ;
/*

# _store_style()
#
# Write Excel BIFF STYLE records.
#
sub _store_style {

    my $self      = shift;

    my $record    = 0x0293; # Record identifier
    my $length    = 0x0004; # Bytes to follow

    my $ixfe      = 0x8000; # Index to style XF
    my $BuiltIn   = 0x00;   # Built-in style
    my $iLevel    = 0xff;   # Outline style level

    my $header    = pack("vv",  $record, $length);
    my $data      = pack("vCC", $ixfe, $BuiltIn, $iLevel);

    $self->_append($header, $data);
*/
}


void wbStoreNumFormat(wb,mask,index)
  struct ss__WorkBook *wb ;
  char *mask ; int index ;
{
#pragma pack(1)
  struct lcl__NumFormat {
    short record,length ;
    short ifmt ;
    unsigned char cch ;
    char mask[128] ;
   } nf ;
#pragma pack()

	nf.record = P2(0x041e) ; nf.length = P2(0x03 + strlen(mask)) ;
	nf.ifmt = P2(index) ; nf.cch = P2(strlen(mask)) ; strcpy(nf.mask,mask) ;
	wbAppend(wb,&nf,UP2(nf.length)+4) ;
//	memcpy(wb->data+wb->datasize,&nf,sizeof nf) ; wb->datasize += sizeof nf ;
//	memcpy(wb->data+wb->datasize,mask,nf.cch) ; wb->datasize += nf.cch ;

/*
# _store_num_format()
#
# Writes Excel FORMAT record for non "built-in" numerical formats.
#
sub _store_num_format {

    my $self      = shift;

    my $record    = 0x041E;                 # Record identifier
    my $length    = 0x03 + length($_[0]);   # Number of bytes to follow

    my $format    = $_[0];                  # Custom format string
    my $ifmt      = $_[1];                  # Format index code
    my $cch       = length($format);        # Length of format string

    my $header    = pack("vv", $record, $length);
    my $data      = pack("vC", $ifmt, $cch);

    $self->_append($header, $data, $format)
*/
}

void wbStore1904(wb)
  struct ss__WorkBook *wb ;
{
#pragma pack(1)
  struct lcl__1904 {
    short record,length ;
    short f1904 ;
   } lr ;
#pragma pack()

	lr.record = P2(0x0022) ; lr.length = P2(0x0002) ;
	lr.f1904 = P2(wb->D1904) ;
	wbAppend(wb,&lr,sizeof lr) ;
//	memcpy(wb->data+wb->datasize,&lr,sizeof lr) ; wb->datasize += sizeof lr ;
/*
# _store_1904()
#
# Write Excel 1904 record to indicate the date system in use.
#
sub _store_1904 {

    my $self      = shift;

    my $record    = 0x0022;         # Record identifier
    my $length    = 0x0002;         # Bytes to follow

    my $f1904     = $self->{_1904}; # Flag for 1904 date system

    my $header    = pack("vv",  $record, $length);
    my $data      = pack("v", $f1904);

    $self->_append($header, $data);
*/
}

struct ss__OLE *oleNew(filename)
  char *filename ;
{ struct ss__OLE *ole ;

	ole = (struct ss__OLE *)v4mm_AllocChunk(sizeof *ole,TRUE) ;
	strcpy(ole->FileName,filename) ;
	ole->block_count = 4 ;
	oleInitialize(ole) ;
	return(ole) ;
/*
# new()
#
# Constructor
#
sub new {

	
    my $class  = shift;
    my $self   = {
                    _OLEfilename   => $_[0],
                    _filehandle    => "",
                    _fileclosed    => 0,
                    _biff_only     => 0,
                    _size_allowed  => 0,
                    _biffsize      => 0,
                    _booksize      => 0,
                    _big_blocks    => 0,
                    _list_blocks   => 0,
                    _root_start    => 0,
                    _block_count   => 4,
                 };

    bless $self, $class;
    $self->_initialize();
    return $self;
*/
}


void oleInitialize(ole)
  struct ss__OLE *ole ;
{

	ole->fp = fopen(ole->FileName,"wb") ;
	if (ole->fp == NULL)
	 { printf("? Error (%s) opening file %s\n",v_OSErrString(errno),ole->FileName) ; exit(EXITABORT) ; } ;
/*
# _initialize()
#
# Check for a valid filename and store the filehandle.
# Filehandle "-" writes to STDOUT
#
sub _initialize {

    my $self    = shift;
    my $OLEfile = $self->{_OLEfilename};

    # Check for filename
    if ($OLEfile eq "") {
        croak('Filename required in WriteExcel("Filename")');
    }

    # Open file for writing
    my $fh = FileHandle->new("> $OLEfile");
    if (not defined $fh) {
        croak "Can't open $OLEfile. It may be in use.";
    }

    # binmode file whether platform requires it or not
    binmode($fh);

    # Store filehandle
    $self->{_filehandle} = $fh;
*/
}


int oleSetSize(ole,biffsize)
  struct ss__OLE *ole ;
{
	if (biffsize > 7087104)
	 { printf("? Exceeded max OLE/BIFF size\n") ; exit(EXITABORT) ; } ;
	ole->biffsize = biffsize ;
	if (biffsize > 4096) { ole->booksize = biffsize ; }
	 else { ole->booksize = 4096 ; } ;
	 ole->size_allowed = 1 ;
	 return(ole->size_allowed) ;
/*
# set_size($biffsize)
#
# Set the size of the data to be written to the OLE stream
#
#   $big_blocks = (109 depot block x (128 -1 marker word)
#                 - (1 x end words)) = 13842
#   $maxsize    = $big_blocks * 512 bytes = 7087104
#
sub set_size {

    my $self    = shift;

    my $maxsize = 7_087_104; # TODO: extend max size

    if ($_[0] > $maxsize) {
        # croak() won't work here if close() called via DESTROY
        carp "Maximum file size, $maxsize, exceeded.";
        return $self->{_size_allowed} = 0;
    }

    $self->{_biffsize} = $_[0];

    # Set the min file size to 4k to avoid having to use small blocks
    if ($_[0] > 4096) {
        $self->{_booksize} = $_[0];
    }
    else {
        $self->{_booksize} = 4096;
    }

    return $self->{_size_allowed} = 1;

*/
}

oleCalculateSizes(ole)
  struct ss__OLE *ole ;
{ int datasize ;

	datasize = ole->booksize ;
	if (datasize % 512 == 0) { ole->big_blocks = datasize / 512 ; }
	 else { ole->big_blocks = 1 + (datasize / 512) ; } ;
	ole->list_blocks = (ole->big_blocks / 127) + 1 ;
	ole->root_start = ole->big_blocks ;

/*
# _calculate_sizes()
#
# Calculate various sizes needed for the OLE stream
#
sub _calculate_sizes {

    my $self     = shift;
    my $datasize = $self->{_booksize};

    if ($datasize % 512 == 0) {
        $self->{_big_blocks} = $datasize/512;
    }
    else {
        $self->{_big_blocks} = int($datasize/512) +1;
    }
    # There are 127 list blocks and 1 marker blocks for each big block
    # depot + 1 end of chain block
    $self->{_list_blocks} = int(($self->{_big_blocks})/127) +1;
    $self->{_root_start}  = $self->{_big_blocks};

    #print $self->{_biffsize},    "\n";
    #print $self->{_big_blocks},  "\n";
    #print $self->{_list_blocks}, "\n";

*/
}

void oleClose(ole)
  struct ss__OLE *ole ;
{

	if (!ole->size_allowed) return ;
	if (!ole->biff_only)
	 { oleWritePadding(ole) ;
	   oleWritePropertyStorage(ole) ;
	   oleWriteBigBlockDepot(ole) ;
	 } ;
	fclose(ole->fp) ;
	ole->fileclosed = TRUE ;
/*
# close()
#
# Write root entry, big block list and close the filehandle.
# This routine is used to explicitly close the open filehandle without
# having to wait for DESTROY.
#
sub close {

    my $self = shift;

    return if not $self->{_size_allowed};
    $self->_write_padding()          if not $self->{_biff_only};
    $self->_write_property_storage() if not $self->{_biff_only};
    $self->_write_big_block_depot()  if not $self->{_biff_only};

    CORE::close($self->{_filehandle});
    $self->{_fileclosed} = 1;
*/
}


/*
###############################################################################
#
# DESTROY()
#
# Close the filehandle if it hasn't already been explicitly closed.
#
sub DESTROY {

    my $self = shift;
    if (not $self->{_fileclosed}) { $self->close() }
}
*/

void oleWrite(ole,data,bytes)
  struct ss__OLE *ole ;
  char *data ; int bytes ;
{

	ole->bytes_written += bytes ;
	fwrite(data,bytes,1,ole->fp) ;

/*
# write($data)
#
# Write BIFF data to OLE file.
#
sub write {

    my $self = shift;

    print {$self->{_filehandle}} $_[0];
*/
}


void oleWriteHeader(ole)
  struct ss__OLE *ole ;
{
#pragma pack(1)
  struct lcl__WriteHeader {
    int id[2], unknown1[4] ;
    short unknown2[2], unknown3, unknown4 ;
    int unknown5[3] ;
    int num_bbd_blocks, root_startblock, unknown6[2], sbd_startblock, unknown7[3] ;
   } wh ;
#pragma pack()
  int unused,i,V,rootstart ;

	memset(&wh,0,sizeof wh) ;
	oleCalculateSizes(ole) ;
	wh.id[0] = PN(0xD0CF11E0) ; wh.id[1] = PN(0xA1B11AE1) ;
	wh.unknown2[0] = P2(0x3e) ; wh.unknown2[1] = P2(0x03) ;
	wh.unknown3 = P2(-2) ; wh.unknown4 = P2(0x09) ;
	wh.unknown5[0] = P4(0x06) ; wh.unknown5[1] = P4(0) ; wh.unknown5[2] = P4(0) ;
	wh.num_bbd_blocks = P4(ole->list_blocks) ;
	wh.root_startblock = P4(ole->root_start) ;
	wh.unknown6[0] = P4(0x00) ; wh.unknown6[1] = P4(0x1000) ;
	wh.sbd_startblock = P4(-2) ;
	wh.unknown7[0] = P4(0x00) ; wh.unknown7[1] = P4(-2) ; wh.unknown7[2] = P4(0x00) ;
//	fwrite(&wh,sizeof wh,1,ole->fp) ;
	oleWrite(ole,&wh,sizeof wh) ;
	rootstart = ole->root_start ;
	for(i=1;i<=ole->list_blocks;i++)
	 { V = P4(++rootstart) ; oleWrite(ole,&V,sizeof V) ; /* fwrite(&V,sizeof V,1,ole->fp) ; */
	 } ;
	
	unused = -1 ;
	for(i=ole->list_blocks;i<=108;i++) { oleWrite(ole,&unused,sizeof unused) ; /*fwrite(&unused,sizeof unused,1,ole->fp) ; */ } ;
/*

# write_header()
#
# Write OLE header block.
#
sub write_header {

    my $self            = shift;

    return if $self->{_biff_only};
    $self->_calculate_sizes();

    my $root_start      = $self->{_root_start};
    my $num_lists       = $self->{_list_blocks};

    my $id              = pack("NN",   0xD0CF11E0, 0xA1B11AE1);
    my $unknown1        = pack("VVVV", 0x00, 0x00, 0x00, 0x00);
    my $unknown2        = pack("vv",   0x3E, 0x03);
    my $unknown3        = pack("v",    -2);
    my $unknown4        = pack("v",    0x09);
    my $unknown5        = pack("VVV",  0x06, 0x00, 0x00);
    my $num_bbd_blocks  = pack("V",    $num_lists);
    my $root_startblock = pack("V",    $root_start);
    my $unknown6        = pack("VV",   0x00, 0x1000);
    my $sbd_startblock  = pack("V",    -2);
    my $unknown7        = pack("VVV",  0x00, -2 ,0x00);
    my $unused          = pack("V",    -1);

    print {$self->{_filehandle}}  $id;
    print {$self->{_filehandle}}  $unknown1;
    print {$self->{_filehandle}}  $unknown2;
    print {$self->{_filehandle}}  $unknown3;
    print {$self->{_filehandle}}  $unknown4;
    print {$self->{_filehandle}}  $unknown5;
    print {$self->{_filehandle}}  $num_bbd_blocks;
    print {$self->{_filehandle}}  $root_startblock;
    print {$self->{_filehandle}}  $unknown6;
    print {$self->{_filehandle}}  $sbd_startblock;
    print {$self->{_filehandle}}  $unknown7;

    for (1..$num_lists) {
        $root_start++;
        print {$self->{_filehandle}}  pack("V", $root_start);
    }

    for ($num_lists..108) {
        print {$self->{_filehandle}}  $unused;
    }
*/
}

void oleWriteBigBlockDepot(ole)
  struct ss__OLE *ole ;
{ int V,i,totalblocks,usedblocks ;

	for(i=1;i<=ole->big_blocks-1;i++)
	 { V = P4(i) ; oleWrite(ole,&V,sizeof V) ; /* fwrite(&V,sizeof V,1,ole->fp) ; */ } ;
	V = P4(-2) ; oleWrite(ole,&V,sizeof V) ; /* fwrite(&V,sizeof V,1,ole->fp) ; */		/* end of chain */
	V = P4(-2) ; oleWrite(ole,&V,sizeof V) ; /* fwrite(&V,sizeof V,1,ole->fp) ; */			/* end of chain */
	V = P4(-3) ;
	for(i=1;i<=ole->list_blocks;i++)
	 { oleWrite(ole,&V,sizeof V) ; /* fwrite(&V,sizeof V,1,ole->fp) ; */	/* marker */
	 } ;
	totalblocks = ole->list_blocks * 128 ;
	usedblocks = ole->big_blocks + ole->list_blocks + 2 ;
	V = P4(-1) ;
	for(i=usedblocks;i<=totalblocks;i++)
	 { oleWrite(ole,&V,sizeof V) ; /* fwrite(&V,sizeof V,1,ole->fp) ; */ /* unused */
	 } ;
/*
# _write_big_block_depot()
#
# Write big block depot.
#
sub _write_big_block_depot {

    my $self         = shift;
    my $num_blocks   = $self->{_big_blocks};
    my $num_lists    = $self->{_list_blocks};
    my $total_blocks = $num_lists *128;
    my $used_blocks  = $num_blocks + $num_lists +2;

    my $marker       = pack("V", -3);
    my $end_of_chain = pack("V", -2);
    my $unused       = pack("V", -1);


    for my $i (1..$num_blocks-1) {
        print {$self->{_filehandle}}  pack("V",$i);
    }

    print {$self->{_filehandle}}  $end_of_chain;
    print {$self->{_filehandle}}  $end_of_chain;

    for (1..$num_lists) {
        print {$self->{_filehandle}}  $marker;
    }

    for ($used_blocks..$total_blocks) {
        print {$self->{_filehandle}}  $unused;
    }
*/
}

void oleWritePropertyStorage(ole)
  struct ss__OLE *ole ;
{

	oleWritePPS(ole,"Root Entry", 0x05, 1, -2, 0x00) ;
	oleWritePPS(ole,"Workbook", 0x02, -1, 0x00, ole->booksize) ;
	oleWritePPS(ole,"", 0x00, -1, 0x00, 0x0000) ;
	oleWritePPS(ole,"", 0x00, -1, 0x00, 0x0000) ;
/*
# _write_property_storage()
#
# Write property storage. TODO: add summary sheets
#
sub _write_property_storage {

    my $self     = shift;

    my $rootsize = -2;
    my $booksize = $self->{_booksize};

    #################  name         type   dir start size
    $self->_write_pps('Root Entry', 0x05,   1,   -2, 0x00);
    $self->_write_pps('Book',       0x02,  -1, 0x00, $booksize);
    $self->_write_pps('',           0x00,  -1, 0x00, 0x0000);
    $self->_write_pps('',           0x00,  -1, 0x00, 0x0000);
*/
}

void oleWritePPS(ole,name,type,dir,sb,size)
  struct ss__OLE *ole ;
  char *name ;
  int type,dir,sb,size ;
{
#pragma pack(1)
  struct lcl__PPS {
    short uname[32] ;
    short length, type ;
    int prev, next, dir, unknown1[5], ts1s, ts1d, ts2s, ts2d, sb, size, unkown2 ; 
   } pps ;
#pragma pack()
  int i ;

	memset(&pps,0,sizeof pps) ;
	for(i=0;;i++) { pps.uname[i] = name[i] ; if (name[i] == '\0') break ; } ;
	i++ ; pps.length = P2(2*i) ;
	pps.type = P2(type) ; pps.prev = P4(-1) ; pps.next = P4(-1) ; pps.dir = P4(dir) ;
	pps.sb = P4(sb) ; pps.size = P4(size) ;
	oleWrite(ole,&pps,sizeof pps) ; /* fwrite(&pps,sizeof pps,1,ole->fp) ; */

/*
# _write_pps()
#
# Write property sheet in property storage
#
sub _write_pps {

    my $self            = shift;

    my $name            = $_[0];
    my @name            = ();
    my $length          = 0;

    if ($name ne '') {
        $name   = $_[0] . "\0";
        # Simulate a Unicode string
        @name   = map(ord, split('', $name));
        $length = length($name) * 2;
    }

    my $rawname         = pack("v*", @name);
    my $zero            = pack("C",  0);

    my $pps_sizeofname  = pack("v",  $length);    #0x40
    my $pps_type        = pack("v",  $_[1]);      #0x42
    my $pps_prev        = pack("V",  -1);         #0x44
    my $pps_next        = pack("V",  -1);         #0x48
    my $pps_dir         = pack("V",  $_[2]);      #0x4c

    my $unknown1        = pack("V",  0);

    my $pps_ts1s        = pack("V",  0);          #0x64
    my $pps_ts1d        = pack("V",  0);          #0x68
    my $pps_ts2s        = pack("V",  0);          #0x6c
    my $pps_ts2d        = pack("V",  0);          #0x70
    my $pps_sb          = pack("V",  $_[3]);      #0x74
    my $pps_size        = pack("V",  $_[4]);      #0x78


    print {$self->{_filehandle}}  $rawname;
    print {$self->{_filehandle}}  $zero x (64 -$length);
    print {$self->{_filehandle}}  $pps_sizeofname;
    print {$self->{_filehandle}}  $pps_type;
    print {$self->{_filehandle}}  $pps_prev;
    print {$self->{_filehandle}}  $pps_next;
    print {$self->{_filehandle}}  $pps_dir;
    print {$self->{_filehandle}}  $unknown1 x 5;
    print {$self->{_filehandle}}  $pps_ts1s;
    print {$self->{_filehandle}}  $pps_ts1d;
    print {$self->{_filehandle}}  $pps_ts2d;
    print {$self->{_filehandle}}  $pps_ts2d;
    print {$self->{_filehandle}}  $pps_sb;
    print {$self->{_filehandle}}  $pps_size;
    print {$self->{_filehandle}}  $unknown1;
*/
}

void oleWritePadding(ole)
  struct ss__OLE *ole ;
{ int i,minsize,padding ; char null ;

	if (ole->biffsize < 4096) { minsize = 4096 ; }
	 else { minsize = 512 ; } ;
	if (ole->biffsize % minsize != 0)
	 { padding = minsize - (ole->biffsize % minsize) ;
	   null = '\0' ; for(i=1;i<=padding;i++) { oleWrite(ole,&null,1) ; /* fwrite(&null,1,1,ole->fp) ; */ } ;
	 } ;
/*
# _write_padding()
#
# Pad the end of the file
#
sub _write_padding {

    my $self     = shift;
    my $biffsize = $self->{_biffsize};
    my $min_size;

    if ($biffsize < 4096) {
        $min_size = 4096;
    }
    else {
        $min_size = 512;
    }

    if ($biffsize % $min_size != 0) {
        my $padding  = $min_size - ($biffsize % $min_size);
        print {$self->{_filehandle}}  "\0" x $padding;
    }
*/
}


struct ss__biff *biffNew()
{ struct ss__biff *biff ;

	biff = (struct ss__biff *)v4mm_AllocChunk(sizeof *biff,TRUE) ;
	return(biff) ;

/*
# new()
#
# Constructor
#
sub new {

    my $class  = $_[0];

    my $self   = {
                    _byte_order    => '',
                    _data          => '',
                    _datasize      => 0,
                 };

    bless $self, $class;
    $self->_set_byte_order();
    return $self;
*/
}

biffSetByteOrder(biff)
  struct ss__biff *biff ;
{

/*
###############################################################################
#
# _set_byte_order()
#
# Determine the byte order and store it as class data to avoid
# recalulating it for each call to new().
#
sub _set_byte_order {

    my $self    = shift;

    if ($byte_order eq ''){
        # Check if "pack" gives the required IEEE 64bit float
        my $teststr = pack "d", 1.2345;
        my @hexdata =(0x8D, 0x97, 0x6E, 0x12, 0x83, 0xC0, 0xF3, 0x3F);
        my $number  = pack "C8", @hexdata;

        if ($number eq $teststr) {
            $byte_order = 0;    # Little Endian
        }
        elsif ($number eq reverse($teststr)){
            $byte_order = 1;    # Big Endian
        }
        else {
            # Give up. I'll fix this in a later version.
            croak ( "Required floating point format not supported "  .
                    "on this platform. See the portability section " .
                    "of the documentation."
            );
        }
    }
    $self->{_byte_order} = $byte_order;
*/
}


struct ss__Format *formatNew(xf_index,formatbase)
  int xf_index ;
  struct ss__Format *formatbase ;
{ struct ss__Format *format ;

	format = (formatbase == NULL ? (struct ss__Format *)v4mm_AllocChunk(sizeof *format,TRUE) : formatbase) ;
	format->xf_index = xf_index ;
	strcpy(format->font,"Arial") ; format->size = 10 ;
	format->bold = 0x0190 ; format->color = 0x7fff ;
	format->fg_color = 0x40 ; format->bg_color = 0x41 ;
	format->bottom_color = 0x40 ; format->top_color = 0x40 ; format->left_color = 0x40 ; format->right_color = 0x40 ;
	return(format) ;
/*
# new()
#
# Constructor
#
sub new {

    my $class  = shift;

    my $self   = {
                    _xf_index       => $_[0] || 0,

                    _font_index     => 0,
                    _font           => 'Arial',
                    _size           => 10,
                    _bold           => 0x0190,
                    _italic         => 0,
                    _color          => 0x7FFF,
                    _underline      => 0,
                    _font_strikeout => 0,
                    _font_outline   => 0,
                    _font_shadow    => 0,
                    _font_script    => 0,
                    _font_family    => 0,
                    _font_charset   => 0,

                    _num_format     => 0,

                    _text_h_align   => 0,
                    _text_wrap      => 0,
                    _text_v_align   => 2,
                    _text_justlast  => 0,
                    _rotation       => 0,

                    _fg_color       => 0x40,
                    _bg_color       => 0x41,

                    _pattern        => 0,

                    _bottom         => 0,
                    _top            => 0,
                    _left           => 0,
                    _right          => 0,

                    _bottom_color   => 0x40,
                    _top_color      => 0x40,
                    _left_color     => 0x40,
                    _right_color    => 0x40,
                 };

    bless  $self, $class;
    return $self;
*/
}


//------------- N E E D   W O R K -----------------------
formatCopy(format)
  struct ss__Format *format ;
{

/*
###############################################################################
#
# copy($format)
#
# Copy the attributes of another Spreadsheet::WriteExcel::Format object.
#
sub copy {
    my $self  = shift;
    my $other = $_[0];

    return if not defined $other;
    return if (ref($self) ne ref($other));

    my $xf    = $self->{_xf_index};
    my $font  = $self->{_font_index};

    %$self = %$other;

    $self->{_xf_index}   = $xf;
    $self->{_font_index} = $font;
*/
}

struct ss__XF *formatGetXF(format,style)
  struct ss__Format *format ;
  int style ;
{ static struct ss__XF xf ;
  int attr_prot, ha ;

//struct ss__XF {
//  short record, length ;
//  short ifnt, ifmt, style, align, icv, fill, border1, border2 ;
	memset(&xf,0,sizeof xf) ;
	xf.record = P2(0x00E0) ; xf.length = P2(0x0010) ;
	xf.ifnt = P2(format->font_index) ; xf.ifmt = P2(format->num_format) ;
	xf.style = P2(style) ;
	switch (format->text_h_align)
	 { default:	ha = format->text_h_align ; break ;
	   case 20:	ha = 1 ; break ;	/* Convert 20 to left align */
	   case 21:	ha = 3 ; break ;	/* Convert 21 to right align */
	 } ;
	xf.align = ha | (format->text_wrap << 3) | (format->text_v_align << 4) |
			(format->text_justlast << 7) | (format->rotation << 8) ;
	if (format->num_format != 0) xf.align |= (1<<10) ;
	if (format->font_index != 0) xf.align |= (1<<11) ;
	if (format->text_wrap != 0) xf.align |= (1<<12) ;
	if (format->bottom || format->top || format->left || format->right) xf.align |= (1<<13) ;
	if (format->fg_color || format->bg_color || format->pattern) xf.align |= (1<<14) ;
//	if (format->size_to_fit) only in BIFF8!!!!
	attr_prot = 0 ;
	xf.align |= (attr_prot << 15) ;
	xf.icv = P2(format->fg_color | (format->bg_color<<7)) ;
//xf.icv = P2(0x100a) ;
	xf.fill = P2(format->pattern | (format->bottom << 6) | (format->bottom_color << 9)) ;
	xf.border1 = P2(format->top | (format->left << 3) | (format->right << 6) | (format->top_color << 9)) ;
	xf.border2 = P2(format->left_color | (format->right_color << 7)) ;
	return(&xf) ;
/*
# get_xf($style)
#
# Generate an Excel BIFF XF record.
#
sub get_xf {

    my $self      = shift;

    my $record;     # Record identifier
    my $length;     # Number of bytes to follow

    my $ifnt;       # Index to FONT record
    my $ifmt;       # Index to FORMAT record
    my $style;      # Style and other options
    my $align;      # Alignment
    my $icv;        # fg and bg pattern colors
    my $fill;       # Fill and border line style
    my $border1;    # Border line style and color
    my $border2;    # Border color

    # Flags to indicate if attributes have been set
    my $atr_num     = ($self->{_num_format} != 0);
    my $atr_fnt     = ($self->{_font_index} != 0);
    my $atr_alc     =  $self->{_text_wrap};
    my $atr_bdr     = ($self->{_bottom}   ||
                       $self->{_top}      ||
                       $self->{_left}     ||
                       $self->{_right});
    my $atr_pat     = ($self->{_fg_color} ||
                       $self->{_bg_color} ||
                       $self->{_pattern});
    my $atr_prot    = 0;

    # Zero the default border colour if the border has not been set
    $self->{_bottom_color} = 0 if $self->{_bottom} == 0;
    $self->{_top_color}    = 0 if $self->{_top}    == 0;
    $self->{_right_color}  = 0 if $self->{_right}  == 0;
    $self->{_left_color}   = 0 if $self->{_left}   == 0;

    $record         = 0x00E0;
    $length         = 0x0010;

    $ifnt           = $self->{_font_index};
    $ifmt           = $self->{_num_format};
    $style          = $_[0];


    $align          = $self->{_text_h_align};
    $align         |= $self->{_text_wrap}     << 3;
    $align         |= $self->{_text_v_align}  << 4;
    $align         |= $self->{_text_justlast} << 7;
    $align         |= $self->{_rotation}      << 8;
    $align         |= $atr_num                << 10;
    $align         |= $atr_fnt                << 11;
    $align         |= $atr_alc                << 12;
    $align         |= $atr_bdr                << 13;
    $align         |= $atr_pat                << 14;
    $align         |= $atr_prot               << 15;


    $icv            = $self->{_fg_color};
    $icv           |= $self->{_bg_color}      << 7;


    $fill           = $self->{_pattern};
    $fill          |= $self->{_bottom}        << 6;
    $fill          |= $self->{_bottom_color}  << 9;


    $border1        = $self->{_top};
    $border1       |= $self->{_left}          << 3;
    $border1       |= $self->{_right}         << 6;
    $border1       |= $self->{_top_color}     << 9;


    $border2        = $self->{_left_color};
    $border2       |= $self->{_right_color}   << 7;

    my $header      = pack("vv",       $record, $length);
    my $data        = pack("vvvvvvvv", $ifnt, $ifmt, $style, $align,
                                       $icv, $fill,
                                       $border1, $border2);

    return($header . $data);
*/
}


struct ss__Font *formatGetFont(format)
  struct ss__Format *format ;
{ static struct ss__Font font ;

	font.record = P2(0x31) ; font.length = P2(0x0f + strlen(format->font)) ;
	font.dyHeight = P2(format->size * 20) ;
	font.grbit = 0 ;
	  if (format->italic) font.grbit |= 0x02 ;
	  if (format->font_strikeout) font.grbit |= 0x08 ;
	  if (format->font_outline) font.grbit |= 0x10 ;
	  if (format->font_shadow) font.grbit |= 0x20 ;
	font.icv = format->color ; font.bls = format->bold ; font.sss = format->font_script ;
	font.bFamily = format->font_family ; font.bCharSet = format->font_charset ;
	font.cch = strlen(format->font) ; font.reserved = 0 ; strcpy(font.rgch,format->font) ;
	return(&font) ;
/*
# get_font()
#
# Generate an Excel BIFF FONT record.
#
sub get_font {

    my $self      = shift;

    my $record;     # Record identifier
    my $length;     # Record length

    my $dyHeight;   # Height of font (1/20 of a point)
    my $grbit;      # Font attributes
    my $icv;        # Index to color palette
    my $bls;        # Bold style
    my $sss;        # Superscript/subscript
    my $uls;        # Underline
    my $bFamily;    # Font family
    my $bCharSet;   # Character set
    my $reserved;   # Reserved
    my $cch;        # Length of font name
    my $rgch;       # Font name


    $dyHeight   = $self->{_size} * 20;
    $icv        = $self->{_color};
    $bls        = $self->{_bold};
    $sss        = $self->{_font_script};
    $uls        = $self->{_underline};
    $bFamily    = $self->{_font_family};
    $bCharSet   = $self->{_font_charset};
    $rgch       = $self->{_font};

    $cch        = length($rgch);
    $record     = 0x31;
    $length     = 0x0F + $cch;
    $reserved   = 0x00;

    $grbit      = 0x00;
    $grbit     |= 0x02 if $self->{_italic};
    $grbit     |= 0x08 if $self->{_font_strikeout};
    $grbit     |= 0x10 if $self->{_font_outline};
    $grbit     |= 0x20 if $self->{_font_shadow};


    my $header  = pack("vv",         $record, $length);
    my $data    = pack("vvvvvCCCCC", $dyHeight, $grbit, $icv, $bls,
                                     $sss, $uls, $bFamily,
                                     $bCharSet, $reserved, $cch);

    return($header . $data. $self->{_font});
*/
}

/*VEH030930 - Old code seems to cause problems - fix temporarily */
int formatGetFontKey(format)
  struct ss__Format *format ;
{ int key,i,c ; unsigned char *b ;

  static int newkey=0 ;

	key = 0 ; b = (char *)format ;
	for(i=1;i<=sizeof *format;i++,b++)
	 { switch(i%6)
	    { case 0:		c = *b ; c = c << 19 ; break ;
	      case 1:		c = *b ; c = c << 3 ; break ;
	      case 2:		c = *b ; c = c << 23 ; break ;
	      case 3:		c = *b ; c = c << 11 ; break ;
	      case 4:		c = *b ; break ;
	      case 5:		c = *b ; c = c << 7 ; break ;
	    } ; key += c ;
	 } ;
//	return(key) ;
	return(++newkey) ;
/*
# get_font_key()
#
# Returns a unique hash key for a font. Used by Workbook->_store_all_fonts()
#
sub get_font_key {

    my $self    = shift;

    # The following elements are arranged to increase the probability of
    # generating a unique key. Elements that hold a large range of numbers
    # eg. _color are placed between two binary elements such as _italic
    #
    my $key = "$self->{_font}$self->{_size}";
    $key   .= "$self->{_font_script}$self->{_underline}";
    $key   .= "$self->{_font_strikeout}$self->{_bold}$self->{_font_outline}";
    $key   .= "$self->{_font_family}$self->{_font_charset}";
    $key   .= "$self->{_font_shadow}$self->{_color}$self->{_italic}";
    $key    =~ s/ /_/g; # Convert the key to a single word

    return $key;
*/
}

formatGetXFIndex(format)
  struct ss__Format *format ;
{

	return(format->xf_index) ;

/*
# get_xf_index()
#
# Returns the used by Worksheet->_XF()
#
sub get_xf_index {
    my $self   = shift;

    return $self->{_xf_index};
*/
}


formatGetColor(format,color)
  struct ss__Format *format ;
  char *color ;
{ int i ; char tcolor[64] ;

	for(i=0;i<sizeof tcolor;i++) { tcolor[i] = tolower(color[i]) ; if (tcolor[i] == '\0') break ; } ;

#define C(name,value) if (strcmp(name,tcolor) == 0) return(value) ;
	C("aqua",0x0F) C("black",0x08) C("blue",0x0C) C("fuchsia",0x0E) C("gray",0x17) C("grey",0x17) C("green",0x11)
	C("lime",0x0B) C("navy",0x12) C("orange",0x1D) C("purple",0x24) C("red",0x0A) C("silver",0x16) C("white",0x09)
	C("yellow",0x0D)
	return(0x7fff) ;

/*
# _get_color()
#
# Used in conjunction with the set_xxx_color methods to convert a color
# string into a number. Color range is 0..63 but we will restrict it
# to 8..63 to comply with Gnumeric. Colors 0..7 are repeated in 8..15.
#
sub _get_color {

    my %colors = (
                    aqua    => 0x0F,
                    black   => 0x08,
                    blue    => 0x0C,
                    fuchsia => 0x0E,
                    gray    => 0x17,
                    grey    => 0x17,
                    green   => 0x11,
                    lime    => 0x0B,
                    navy    => 0x12,
                    orange  => 0x1D,
                    purple  => 0x24,
                    red     => 0x0A,
                    silver  => 0x16,
                    white   => 0x09,
                    yellow  => 0x0D,
                 );

    # Return: 1. The default color, 0x7FFF, if undef,
    #         2. Color string converted to an integer,
    #         3. The default color if string is unrecognised,
    #         4. The default color if arg is outside range,
    #         5. An integer in the valid range
    #
    return 0x7FFF if not $_[0];
    return $colors{lc($_[0])} if exists $colors{lc($_[0])};
    return 0x7FFF if ($_[0] =~ m/\D/);
    return 0x7FFF if (($_[0] < 8) || ($_[0] > 63));
    return $_[0];
*/
}

formatSetColor(format,color)
  struct ss__Format *format ;
  char *color ;
{
	format->color = formatGetColor(format,color) ;
}


void formatSetAlign(format,cmode)
  struct ss__Format *format ;
  char *cmode ;
{ char mode[32] ;
  int i ;

	for(i=0;;i++) { mode[i] = tolower(cmode[i]) ; if (mode[i] == '\0') break ; } ;
#define H(num,str) if (strcmp(mode,str) == 0) { format->text_h_align = num ; return ; } ;
	H(1,"left") H(2,"centre") H(2,"center") H(3,"right") H(4,"fill") H(5,"justify") H(6,"merge") H(0,"top")
#define V(num,str) if (strcmp(mode,str) == 0) { format->text_v_align = num ; return ; } ;
	V(0,"top") V(1,"vcentre") V(1,"vcenter") V(2,"bottom") V(3,"vjustify")
	
/*
# Set cell alignment.
#
sub set_align {

    my $self     = shift;
    my $location = $_[0];

    return if not defined $location;  # No default
    return if $location =~ m/\d/;     # Ignore numbers

    $location = lc($location);

    $self->set_text_h_align(1) if ($location eq 'left');
    $self->set_text_h_align(2) if ($location eq 'centre');
    $self->set_text_h_align(2) if ($location eq 'center');
    $self->set_text_h_align(3) if ($location eq 'right');
    $self->set_text_h_align(4) if ($location eq 'fill');
    $self->set_text_h_align(5) if ($location eq 'justify');
    $self->set_text_h_align(6) if ($location eq 'merge');
    $self->set_text_v_align(0) if ($location eq 'top');
    $self->set_text_v_align(1) if ($location eq 'vcentre');
    $self->set_text_v_align(1) if ($location eq 'vcenter');
    $self->set_text_v_align(2) if ($location eq 'bottom');
    $self->set_text_v_align(3) if ($location eq 'vjustify');
*/
}


formatSetMerge(format)
  struct ss__Format *format ;
{

	format->text_h_align = 6 ;
/*
# set_merge()
#
# This is an alias for the unintuitive set_align('merge')
#
sub set_merge {

    my $self     = shift;

    $self->set_text_h_align(6);
*/
}


formatSetBold(format,weight)
  struct ss__Format *format ;
  int weight ;
{
	if (weight < 0) { format->bold = 0x2bc ; }
	 else if (weight == 1) { format->bold = 0x2bc ; }
	 else if (weight == 0) { format->bold = 0x190 ; }
	 else { format->bold = 0x190 ; }
/*
# set_bold()
#
# Bold has a range 0x64..0x3E8.
# 0x190 is normal. 0x2BC is bold. So is an excessive use of AUTOLOAD.
#
sub set_bold {

    my $self   = shift;
    my $weight = $_[0];

    $weight = 0x2BC if not defined $weight; # Bold text
    $weight = 0x2BC if $weight == 1;        # Bold text
    $weight = 0x190 if $weight == 0;        # Normal text
    $weight = 0x190 if $weight <  0x064;    # Lower bound
    $weight = 0x190 if $weight >  0x3E8;    # Upper bound

    $self->{_bold} = $weight;
*/
}

formatSetBorder(format)
  struct ss__Format *format ;
{

/*
# set_border($style)
#
# Set cells borders to the same style
#
sub set_border {

    my $self  = shift;
    my $style = $_[0];

    $self->set_bottom($style);
    $self->set_top($style);
    $self->set_left($style);
    $self->set_right($style);
*/
}

formatSetBorderColor(format,color)
  struct ss__Format *format ;
  int color ;
{
	format->bottom_color = color ;format->top_color = color ;
	format->left_color = color ; format->right_color ;

/*
# set_border_color($color)
#
# Set cells border to the same color
#
sub set_border_color {

    my $self  = shift;
    my $color = $_[0];

    $self->set_bottom_color($color);
    $self->set_top_color($color);
    $self->set_left_color($color);
    $self->set_right_color($color);
*/
}

formatSetFormat(format)
  struct ss__Format *format ;
{

/*
# set_format()
#
# Backward compatible alias for set_num_format(). set_format() was a poor
# choice of method name. It is now deprecated and will disappear.
#
sub set_format {
    my $self = shift;
    $self->set_num_format(@_);
    carp("set_format() is deprecated, use set_num_format() instead") if $^W;
*/
}


/*

# AUTOLOAD. Deus ex machina.
#
sub AUTOLOAD {

    my $self = shift;

    # Ignore calls to DESTROY
    return if $AUTOLOAD =~ /::DESTROY$/;

    # Check for a valid method names, ie. "set_xxx_yyy".
    $AUTOLOAD =~ /.*::set(\w+)/ or croak "Unknown method: $AUTOLOAD";

    # Match the attribute, ie. "_xxx_yyy".
    my $attribute = $1;

    # Check that the attribute exists
    exists $self->{$attribute}  or croak "Unknown method: $AUTOLOAD";

    # The attribute value
    my $value;

    # Determine the value of the attribute
    if ($AUTOLOAD =~ /.*::set\w+color$/) {
        $value =  _get_color($_[0]); # For "set_xxx_color" methods
    }
    elsif (not defined($_[0])) {
        $value = 1; # Default is 1
    }
    else {
        $value = $_[0];
    }

    $self->{$attribute} = $value;
}

*/
