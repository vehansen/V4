/*	v4proto.c - Module Prototypes		*/



/*	v c o m m o n		*/


int data_compress(void *,void *,int,int,int) ;
int data_expand(void *, void *, int) ;


UCCHAR *v4lex_TknPtr(struct V4LEX__TknCtrlBlk *,int,UCCHAR *,int *) ;
UCCHAR *v4lex_NextTkn(struct V4LEX__TknCtrlBlk *,int) ;
LOGICAL v4lex_ReadNextLine(struct V4LEX__TknCtrlBlk *,struct V4LEX__TablePrsOpt *tpo,UCCHAR *,UCCHAR *) ;
int v4lex_ReadNextLine_Table(UCCHAR *,int,UCCHAR *,UCCHAR *,UCCHAR *,struct V4LEX__Table *,UCCHAR **,int,struct V4LEX__TknCtrlBlk *,UCCHAR *,int) ;
ETYPE v4lex_NextCSVField(UCCHAR *,UCCHAR *,LENMAX,UCCHAR **,LOGICAL,struct V4LEX__TknCtrlBlk *) ;
ETYPE v4lex_NextTABField(UCCHAR *,UCCHAR *,LENMAX,UCCHAR **) ;
int v4lex_ReadXMLFile(struct V4C__Context *,UCCHAR *,struct UC__File *,int,int *,int *) ;
LOGICAL v4lex_CloseOffLevel(struct V4LEX__TknCtrlBlk *,int) ;
LOGICAL v4lex_GetStdInput(UCCHAR *,UCCHAR *,int) ;

UCCHAR *vlex_FullPathNameOfFile(UCCHAR *,UCCHAR *,int) ;
LOGICAL v4lex_TknArray_Setup(char *,char *,char) ;
void v4lex_NestInput(struct V4LEX__TknCtrlBlk *,struct UC__File *,UCCHAR *,int) ;
LOGICAL v4lex_BindArgs(struct V4LEX__TknCtrlBlk *) ;
struct V4LEX__TknCtrlBlk *v4lex_InitTCB(struct V4LEX__TknCtrlBlk *,int) ;
void v4lex_ResetTCB(struct V4LEX__TknCtrlBlk *) ;
void v4lex_FreeTCB(struct V4LEX__TknCtrlBlk *) ;
void v4lex_SetPrompt(struct V4LEX__TknCtrlBlk *,UCCHAR *) ;
LOGICAL v4lex_RevPolParse(struct V4LEX__RevPolParse *,struct V4LEX__TknCtrlBlk *,int,struct V4C__Context *) ;
#ifdef WANTREVPOLOPT
void v4lex_OptRevPolSimplify(void *,int) ;
void v4lex_OptRevPolDropLeft(void *,int,int) ;
LOGICAL v4lex_OptRevPolMarkMap(void *,int) ;
void v4lex_OptRevPolCvtTree(void *,struct V4LEX__RevPolParse *,struct V4LEX__RevPolParse *,int) ;
void v4lex_Show(void *,struct V4LEX__RevPolParse *,int) ;
void v4lex_OptRevPol(struct V4LEX__RevPolParse *,struct V4LEX__RevPolParse *) ;
#endif
LOGICAL v_ipLookup(struct V4C__Context *,INTMODX,UCCHAR *,UCCHAR *,int,ETYPE,SOCKETNUM *,UCCHAR *,P *) ;
int v_CvtToPrime(int) ;

int v_AvailableProcessors() ;
LOGICAL v_SpawnSort(UCCHAR *,int,UCCHAR *) ;
//LOGICAL v_SpawnProcess(UCCHAR *,UCCHAR *,int,UCCHAR *,struct V4DPI__Point *,int *,FILEID) ;
LOGICAL v_SpawnProcess(struct V__SpawnArgs *) ;

#ifdef WINNT
int v_OSH_CreateRef(int,int,HANDLE,HANDLE,UCCHAR *) ;
#else
int v_OSH_CreateRef(int,int,int,int,UCCHAR *) ;
#endif
LOGICAL v_OSH_IsProcThreadActive(int) ;
LOGICAL v_OSH_VerifyProcThread(struct V4OSH__Table *,int) ;
UCCHAR *v_OSH_TypeDisplayer(int) ;
#ifdef WINNT
int v_OSH_WinNTProcHandleList(HANDLE) ;
#endif

//LOGICAL v_IsBatchJob() ;
LOGICAL v_IsAConsole(FILE *) ;

FILE *v_MakeOpenTmpFile(UCCHAR *,UCCHAR *,int,char *,UCCHAR *) ;
char *v3_logical_decoderx(char *,LOGICAL,int,UCCHAR *) ;
char *v3_logical_decoder(char *,LOGICAL) ;
UCCHAR *v_UCLogicalDecoder(UCCHAR *,LOGICAL,int,UCCHAR *) ;

LOGICAL v_UCGetEnvValue(UCCHAR *,UCCHAR *,int) ;
LOGICAL vlog_StoreSymValue(UCCHAR *,UCCHAR *) ;
UCCHAR *vlog_GetSymValue(UCCHAR *) ;
UCCHAR *v_GetV4HomePath(UCCHAR *) ;

char *v_OSErrString(int) ;


UCCHAR *vout_FileName(int) ;
void vout_Init() ;
FILEID vout_PntIdToFileId(struct V4C__Context *,struct V4DPI__LittlePoint *) ;
INDEX vout_PntIdToFileX(struct V4C__Context *,struct V4DPI__LittlePoint *) ;
INDEX vout_FileIdToFileX(int) ;
INDEX vout_StreamToFileX(int) ;
FILEID vout_FileTypeToFileId(int) ;
INDEX vout_FileTypeToFileX(int) ;
LOGICAL vout_FileIdViaThread(FILEID,LOGICAL) ;
FILEID vout_OpenStreamFile(struct V4DPI__LittlePoint *,UCCHAR *,UCCHAR *,struct V4LEX__TknCtrlBlk *,int,int,int,UCCHAR *) ;
FILEID vout_OpenStreamBigText(struct V4DPI__LittlePoint *,struct V4DPI__Point *,UCCHAR *) ;
FILEID vout_OpenStreamBuffer(struct V4DPI__LittlePoint *,UCCHAR *,LOGICAL) ;
COUNTER vout_GetStreamBuffer(FILEID,VSTREAM,COUNTER,UCCHAR *,UCCHAR *) ;
UCCHAR *vout_GetOutputBuffer(FILEID,VSTREAM,UCCHAR *) ;
COUNTER vout_CountForFile(FILEID,VSTREAM,LOGICAL,UCCHAR *) ;
LOGICAL vout_CloseFile(int,int,UCCHAR *) ;
LOGICAL vout_BindStreamFile(FILEID,int,int,UCCHAR *) ;
void vout_rriSet(int,struct V4RPT__RptInfo *) ;
struct V4RPT__RptInfo *vout_rriGet(int) ;
void vout_rriClose(struct V4RPT__RptInfo *) ;
void vout_CloseAjaxStream(int) ;
void vout_CloseDeferStream(int) ;
LOGICAL vout_Text(int,int,char *) ;
LOGICAL vout_UCText(int,int,UCCHAR *) ;
void vout_NL(int) ;
LOGICAL vout_TextFileX(int,int,char *) ;
LOGICAL vout_UCTextFileX(int,int,UCCHAR *) ;
LOGICAL vout_UCTextFileXCont(int,int,UCCHAR *) ;
void vout_NLFileX(int) ;

void v_ParseFileName(struct V4LEX__TknCtrlBlk *,UCCHAR *,UCCHAR *,UCCHAR *,int *) ;

UCCHAR *v4mm_AllocUC(int) ;
void *v4mm_AllocChunk(int, int) ;
void v4mm_FreeChunk(void *) ;
void v4mm_MemoryExit() ;
void v4mm_FreeResources(struct V4C__ProcessInfo *) ;

void sleepms(int) ;

struct han__Master *han_PtrToMaster() ;
int han_Make() ;
//int han_Close(int) ;
//int han_FreeOnClose(int) ;
int han_SetPointer(int,int,void *) ;
void *han_GetPointer(int,int) ;
//int han_SetParent(int,int) ;
//int han_GetParent(int) ;
//int han_SetUpdate(int) ;
//int han_IsUpdated(int) ;
int han_SetInfo(int,int) ;
int han_GetInfo(int) ;
//int han_SetType(int,int) ;
//int han_GetType(int) ;
//LOGICAL han_AddHandleToList(int,int,int) ;
//LOGICAL han_RemoveHandleFromList(int) ;
//int han_GetNextHandle(int) ;
struct V4C__ProcessInfo *v_GetProcessInfo() ;


int mscu_ymd_to_ud(int,int,int) ;
int vcal_UDateFromYMD(LOGICAL,int,int,int,UCCHAR *) ;
int vcal_UDTFromYMD(int,int,int,int,int,int,UCCHAR *) ;
int mscu_udate_to_yyyymmdd(int) ;
int mscu_udate_to_yyyyddd(int) ;
int mscu_udate_to_uweek(int,int) ;
int mscu_uweek_to_udate(int,int) ;
UCCHAR *mscu_udate_to_ddmmmyy(int) ;
UCCHAR *mscu_udt_to_ddmmmyyhhmmss(int) ;
int mscu_msdos_to_udt(int) ;
int mscu_udt_to_msdos(int) ;
int mscu_minutes_west() ;
char *v_FormatHTTPDateTime(time_t,int) ;
double v_ConnectTime() ;
double v_CPUTime() ;
//B64INT v_psuedoRandomNum64() ;
void vRan64_Seed(UB64INT) ;
void vRan64_ArraySeed(UB64INT [],UB64INT) ;
UB64INT vRan64_RandomU64() ;
B64INT v4stat_ran3() ;
double vRan64_RandomDbl() ;

int vcal_CalToUDate(double,int,LOGICAL *) ;
int vcal_CalToUDT(double,int,LOGICAL *) ;


int v_FormatLInt(B64INT,int,UCCHAR *,UCCHAR *) ;
int v_FormatInt(int,UCCHAR *,UCCHAR *,UCCHAR *) ;
LOGICAL v_HundredsToText(int,UCCHAR *,struct V4LI_LanguageInfo *,int) ;
int v_ParseInt(UCCHAR *,UCCHAR **,int) ;
LOGICAL v_ParseLog(UCCHAR *,UCCHAR **) ;
int v_FormatDate(void *,int,int,int,UCCHAR *,UCCHAR *) ;
void v_NewLatLonFromLLAD(double *,double *,double,double,double,double) ;
double v_DistBetween2LL(double,double,double,double,int) ;
int v_FormatGeoCoord(struct V4C__Context *,struct V4DPI__Value_GeoCoord *,UCCHAR *,UCCHAR *,int) ;
LOGICAL v_ParseGeoCoord(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__DimInfo *,UCCHAR *,UCCHAR *) ;
int v_FormatTele(struct V4C__Context *,struct V4DPI__Value_Tele *,UCCHAR *,UCCHAR *) ;
int v_FormatDbl(double,UCCHAR *,UCCHAR *, UCCHAR *) ;
double v_ParseDbl(UCCHAR *,UCCHAR **) ;
double nint(double) ;

void v_Sleepms(int) ;

LOGICAL v_ParseFormatSpecs(struct V4C__Context *,struct V4SS__FormatSpec *,int,UCCHAR *) ;
void v_VFStoCSS(struct V4C__Context *,struct V4SS__FormatSpec *,UCCHAR *,int) ;

void v_LoadCountryInfo() ;
int v_CountryNameToRef(UCCHAR *) ;
void v_setCurrentCountry(struct V4CI__CountryInfo *,int) ;
UCCHAR *v_CountryRefToName(int) ;
LOGICAL v_parseTeleNum(struct V4C__Context *,struct V4DPI__DimInfo *,struct V4DPI__Point *,UCCHAR *,UCCHAR *,UCCHAR *) ;
LOGICAL v_IsNoneLiteral(UCCHAR *) ;

UCCHAR *v_URLAddressLookup(UCCHAR *,UCCHAR *) ;
LOGICAL v_FileExtToContentType(UCCHAR *,UCCHAR *,int,UCCHAR *) ;
UCCHAR v4xml_LookupISONamedEntity(UCCHAR *) ;
struct V4HTML_Entities *v_HTMLEntityTableLoad() ;

void v_LoadColorTable() ;
int v_ColorNumToRef(int) ;
int v_ColorRefRange(int *,int *) ;
int v_ColorRGBNameToRef(UCCHAR *,int) ;
int v_ColorNameToRef(UCCHAR *) ;
UCCHAR *v_ColorRefToName(int) ;
int v_ColorRefToRGB(int) ;
UCCHAR *v_ColorRefToHTML(int) ;
int v_ColorRefToId(int) ;
int v_ColorRefToXL(int,int) ;

int VUC_Encode64(UCCHAR *,int,UCCHAR *,int,UCCHAR *) ;
int V_Encode64(unsigned char *,int,unsigned char *,int,UCCHAR *) ;
int V_Decode64(UCCHAR *,int,UCCHAR *,int,UCCHAR *) ;

HASH32 v_Hash32(char *,int,int) ;
UB64INT v_Hash64(UCCHAR *) ;
UB64INT v_Hash64b(unsigned char *,int) ;

void v_MD5Start(VMD5__Context *) ;
void v_MD5Update(VMD5__Context *,UINT8 *,UINT32) ;
void v_MD5Finish(VMD5__Context *,UINT8 []) ;

int v_StringLit(UCCHAR *,UCCHAR *,int,int,int) ;

int KDayOnOrBefore(int,int) ;
int KDayOnOrAfter(int,int) ;
int NthKDay(int,int,int,int,int) ;
int GregorianLeapYear(int) ;
LOGICAL VerifyGregorianYMD(int,int,int,UCCHAR *) ;
int FixedFromGregorian(int,int,int) ;
int GregorianYearFromFixed(int) ;
LOGICAL GregorianFromFixed(int,struct V4Cal__Args *) ;
int JulianLeapYear(int) ;
int FixedFromJulian(int,int,int) ;
LOGICAL JulianFromFixed(int,struct V4Cal__Args *) ;
LOGICAL VerifyISOYWD(int,int,int,UCCHAR *) ;
int FixedFromISO(int,int,int) ;
LOGICAL ISOFromFixed(int,struct V4Cal__Args *) ;
LOGICAL VerifyIslamicYMD(int,int,int,UCCHAR *) ;
int FixedFromIslamic(int,int,int) ;
LOGICAL IslamicFromFixed(int,struct V4Cal__Args *) ;
int HebrewLeapYear(int) ;
int LastMonthOfHebrewYear(int) ;
double Molad(int,int) ;
int HebrewCalendarElapsedDays(int) ;
int HebrewNewYearDelay(int) ;
int HebrewNewYear(int) ;
int LastDayOfHebrewMonth(int,int) ;
int LongMarheshvan(int) ;
int ShortKislev(int) ;
int DaysInHebrewYear(int) ;
LOGICAL VerifyHebrewYMD(int,int,int,UCCHAR *) ;
int FixedFromHebrew(int,int,int) ;
LOGICAL HebrewFromFixed(int,struct V4Cal__Args *) ;
int Easter(int) ;
int YomKippur(int) ;
int Passover(int) ;
double Poly(double,int,double []) ;
double ArcTan2(double,int) ;
double Direction(double,double,double,double) ;
double UniversalFromLocal(double,double) ;
double LocalFromUniversal(double,double) ;
double StandardFromUniversal(double,int) ;
double UniversalFromStandard(double,int) ;
double StandardFromLocal(double,double,int) ;
double LocalFromStandard(double,double,int) ;
double Abberation() ;
double Nutation() ;
double EphemerisCorrection(double) ;
double DynamicalFromUniversal(double) ;
double UniversalFromDynamical(double) ;
double JulianCenturies(double) ;
double Obliquity(double) ;
double SiderealFromMoment(double) ;
double SolarLongitude(double) ;
double SolarLongitudeAfter(double,double) ;
double SolarLongitudeBefore(double,double) ;
double Nutation(double) ;
double Abberation(double) ;
double EquationOfTime(double) ;
double ApparentFromLocal(double) ;
double LocalFromApparent(double) ;
double MomentFromDepression(LOGICAL *,double,double,double,double) ;
double Dawn(LOGICAL *,int,double,double,int,double) ;
double Sunrise(LOGICAL *,int,double,double,int,int) ;
double Dusk(LOGICAL *,int,double,double,int,double) ;
double Sunset(LOGICAL *,int,double,double,int,int) ;
double NthNewMoon(int) ;
double LunarPhase(double) ;
double NewMoonBefore(double) ;
double NewMoonAfter(double) ;
double LunarLongitude(double) ;
double LunarPhaseBefore(double,double) ;
double LunarPhaseAfter(double,double) ;
double LunarLatitude(double) ;
double LunarAltitude(double,double,double) ;
int v_mctAlloc(struct V4C__ProcessInfo *,int,void **) ;
struct V4MSG__MsgInfo *v_LoadMsgFile(struct V4C__ProcessInfo *) ;
void v_LoadSiteIni(struct V4C__ProcessInfo *,UCCHAR *,int) ;
UCCHAR *vIni_Lookup(struct V4C__Context *,UCCHAR *) ;


#ifdef ALTVARGS
LOGICAL v_Msg(va_alist) ;
#else
LOGICAL v_Msg(struct V4C__Context *,UCCHAR *,char *,...) ;
#endif

void vjson_SubParamsInJSON(UCCHAR *,LENMAX,char *,UCCHAR *,UCCHAR *) ;
int v_ParseUPeriod(struct V4C__Context *,UCCHAR *,struct V4DPI__DimInfo *,UCCHAR *) ;
double v_ParseCalendar(struct V4C__Context *,UCCHAR *,int,int,int,int,LOGICAL) ;
int v_ParseUDate(struct V4C__Context *,UCCHAR *,int,int,LOGICAL) ;
UCCHAR *v_CalCalc(struct V4C__Context *,UCCHAR *,int,PNTTYPE,int *,PNTTYPE *,LOGICAL) ;
int v_CalCalcMonth(int,int,UCCHAR,UCCHAR *,struct V4CI__CountryInfo *,INDEX) ;
UCCHAR *v_ParseRelativeUDate(struct V4C__Context *,UCCHAR *,int *,int,LOGICAL,PNTTYPE *,ETYPE *) ;
int v_ParseUDT(struct V4C__Context *,UCCHAR *,int,int,int,int *,LOGICAL) ;
double v_ParseUTime(struct V4C__Context *,UCCHAR *) ;
int v_ParseUMonth(struct V4C__Context *,UCCHAR *,int) ;
int v_Soundex(UCCHAR *,UCCHAR *) ;
int v_Soundex2(UCCHAR *,UCCHAR *) ;
char *v_MD5ChecksumFile(struct V4C__Context *,UCCHAR *,char *,int) ;


LOGICAL v_UCFileOpen(struct UC__File *,UCCHAR *,int,int,UCCHAR *,int) ;
int v_UCFileClose(struct UC__File *) ;
int v_UCReadLine(struct UC__File *,int,void *,int,UCCHAR *) ;

int v_Read8BitStreamTo8Bit(char *,int,FILE *,LOGICAL,int *) ;
int v_ReadASCStreamtoUC(UCCHAR *,int,struct UC__File *) ;
int v_Read16BitStreamto16Bit(UCCHAR *,int,FILE *,LOGICAL,int *) ;

int UTF16LEToUTF8(unsigned char *,int,wchar_t *,int) ;
int UTF8ToUTF16LE(wchar_t *,int,unsigned char *,int) ;
int UTF16BEToUTF8(unsigned char *,int,wchar_t *,int) ;
int UTF8ToUTF16BE(wchar_t *,int,unsigned char *,int) ;
char *v_ConvertUCtoASC(UCCHAR *) ;
UCCHAR *v_ConvertASCtoUC(char *) ;
int vuc_ToUpper(UCCHAR) ;
int vuc_ToLower(UCCHAR) ;
//LOGICAL vuc_IsContId(UCCHAR) ;
int vuc_StrCmpIC(UCCHAR *,UCCHAR *,int) ;
UCCHAR *vuc_StrStrIC(UCCHAR *,UCCHAR *) ;
LOGICAL vuc_Cmp2PtsStr(struct V4DPI__Point *,struct V4DPI__Point *) ;
LOGICAL v_LoadUnicodeData(UCCHAR *) ;

int vdes_Encrypt(UWORD *input, UWORD *,struct VED__desKeySched *, int) ;
int vdes_SetKey(struct VED__desKeyBlock *,struct VED__desKeySched *) ;
int vdes_ECBEncrypt(struct VED__desKeyBlock *, struct VED__desKeyBlock *,struct VED__desKeySched *, int) ;
LENMAX v_EncodeDecode(BYTE *,LENMAX,LENMAX,ETYPE,LOGICAL,BYTE *,LENMAX,UCCHAR *) ;
UWORD vcrc_Calculate (BYTE *,LENMAX) ;

struct VZIP__Master *vzip_InitZipFile(UCCHAR *,UCCHAR *) ;
size_t vzip_AppendFile(struct VZIP__Master *,UCCHAR *,LENMAX,BYTE *,char *,LOGICAL,UDTVAL) ;
LOGICAL vzip_CloseZipFile(struct VZIP__Master *,UCCHAR *) ;
LENMAX v_zipCompress(char *,LENMAX,char *,LENMAX,INDEX,UCCHAR *) ;






/*	v c t x			*/

void v4ctx_Initialize(struct V4C__Context *,struct V4C__Context *,struct V4MM__MemMgmtMaster *) ;
int v4ctx_InitTempAgg(struct V4C__Context *) ;
int v4ctx_AreaAdd(struct V4C__Context *,struct V4IS__ParControlBlk *,struct V4C__AreaHInfo *, struct V4DPI__LittlePoint *) ;
void v4ctx_AreaClose(struct V4C__Context *,int) ;
//void v4ctx_AreaCheckPoint(struct V4C__Context *,int) ;
int v4ctx_FramePush(struct V4C__Context *,UCCHAR *) ;
//int v4ctx_FrameHan(struct V4C__Context *) ;
LOGICAL v4ctx_FramePop(struct V4C__Context *,int,UCCHAR *) ;
void v4ctx_FrameCacheClear(struct V4C__Context *,LOGICAL)	;
LOGICAL v4ctx_FrameAddDim(struct V4C__Context *,int,struct V4DPI__Point *,int,int) ;
void v4ctx_ExamineCtx(struct V4C__Context *,LOGICAL,int) ;

ETYPE v4l_ListPoint_Modify(struct V4C__Context *,struct V4L__ListPoint *,int,struct V4DPI__Point *,int) ;
int v4l_ListPoint_Value(struct V4C__Context *,struct V4L__ListPoint *,int,struct V4DPI__Point *) ;
LOGICAL v4l_ListClose(struct V4C__Context *ctx,struct V4L__ListPoint *) ;
LOGICAL v4l_XMLHandleEOF(struct V4C__Context *,struct V4L__ListToken *,P *,VTRACE) ;
LOGICAL v4l_ListPartition(struct V4C__Context *,struct V4L__ListPoint *,struct V4L__Partitions *,int,int) ;

struct V4L__ListBMData1 *v4l_BM1And(struct V4L__ListBMData1 *,struct V4L__ListBMData1 *) ;
struct V4L__ListBMData1 *v4l_BM1Or(struct V4L__ListBMData1 *,struct V4L__ListBMData1 *) ;
struct V4L__ListBMData1 *v4l_BM1Not(struct V4L__ListBMData1 *) ;
struct V4L__ListBMData1 *v4l_BM1And(struct V4L__ListBMData1 *,struct V4L__ListBMData1 *) ;
struct V4L__ListBMData1 *v4l_BM1And(struct V4L__ListBMData1 *,struct V4L__ListBMData1 *) ;
LOGICAL v4l_BitMapAnd(struct V4C__Context *,int,struct V4DPI__Point *,struct V4DPI__Point *,struct V4DPI__Point *) ;
LOGICAL v4l_BitMapOr(struct V4C__Context *,int,struct V4DPI__Point *,struct V4DPI__Point *,struct V4DPI__Point *) ;
LOGICAL v4l_BitMapMinus(struct V4C__Context *,int,struct V4DPI__Point *,struct V4DPI__Point *,struct V4DPI__Point *) ;
LOGICAL v4l_BitMapNot(struct V4C__Context *,int,struct V4DPI__Point *,struct V4DPI__Point *) ;
int v4l_BitMapCount(struct V4C__Context *,struct V4DPI__Point *,struct V4L__ListPoint *) ;
LOGICAL v4l_BitMapTest(struct V4C__Context *,INTMODX,INDEX,struct V4DPI__Point *) ;

void v4l_Purgevbe(struct V4C__Context *) ;
struct V4L__BlockEntry *v4l_Getvbe(struct V4C__Context *,int,int) ;
void v4l_Purgevbeg(struct V4C__Context *) ;
void v4l_Putvbeg(struct V4C__Context *,int,int,int) ;
LOGICAL v4l_Putvbe(struct V4C__Context *,int,int,int) ;
struct V4L__BlockEntryGeneric *v4l_Getvbeg(struct V4C__Context *,int,int) ;

V4MSEGID v4seg_PutSegments(struct V4C__Context *,void *,int,LOGICAL,LOGICAL) ;
void *v4seg_GetSegments(struct V4C__Context *,V4MSEGID,int *,int,AREAID) ;


int v4prj_RemoveProjectionInfo(struct V4C__Context *,int,int,int) ;
int v4prj_GetProjectionInfo(struct V4C__Context *,int,int,struct V4DPI__ProjectionInfo **) ;
void v4prj_CacheProjectionInfo(struct V4C__Context *,int,int,struct V4DPI__ProjectionInfo *) ;

void *v4wh_InitWH_IntInt(struct V4C__Context *,struct V4DPI__WormHoleIntInt *,int,struct V4DPI__Point *,struct V4DPI__Point *) ;
struct V4DPI__WormHoleIntInt *v4wh_PutWH_IntInt(struct V4C__Context *,struct V4DPI__WormHoleIntInt *,int,int) ;
void *v4wh_InitWH_IntDbl(struct V4C__Context *,struct V4DPI__WormHoleIntDbl *,int,struct V4DPI__Point *,struct V4DPI__Point *) ;
void *v4wh_InitWH_IntAgg(struct V4C__Context *,struct V4DPI__WormHoleIntAgg *,int,struct V4DPI__Point *,struct V4DPI__Point *) ;
void *v4wh_InitWH_IntUOMPer(struct V4C__Context *,struct V4DPI__WormHoleIntUOMPer *,int,struct V4DPI__Point *) ;
void *v4wh_InitWH_IntGeo(struct V4C__Context *,struct V4DPI__WormHoleIntGeo *,int,struct V4DPI__Point *) ;
struct V4DPI__WormHoleIntDbl *v4wh_PutWH_IntDbl(struct V4C__Context *,struct V4DPI__WormHoleIntDbl *,int,double) ;
struct V4DPI__WormHoleIntAgg *v4wh_PutWH_IntAgg(struct V4C__Context *,struct V4DPI__WormHoleIntAgg *,int,int,int) ;
struct V4DPI__WormHoleIntUOMPer *v4wh_PutWH_IntUOMPer(struct V4C__Context *,struct V4DPI__WormHoleIntUOMPer *,int,struct V4DPI__Value_UOMPer *) ;
struct V4DPI__WormHoleIntGeo *v4wh_PutWH_IntGeo(struct V4C__Context *,struct V4DPI__WormHoleIntGeo *,int,struct V4DPI__Value_GeoCoord *) ;
void *v4wh_InitWH_LIntLInt(struct V4C__Context *,struct V4DPI__WormHoleLIntLInt *,int,struct V4DPI__Point *,struct V4DPI__Point *) ;
struct V4DPI__WormHoleLIntLInt *v4wh_PutWH_LIntLInt(struct V4C__Context *,struct V4DPI__WormHoleLIntLInt *,B64INT,B64INT) ;



struct V4IM__BaA *v4ctx_FrameBaAInfo(struct V4C__Context *) ;
#ifdef NEWBAFSTUFF
struct V4IM__BaA *v4ctx_FrameBaASet(struct V4C__Context *,LOGICAL,LENMAX,LENMAX,LENMAX) ;
#else
struct V4IM__BaA *v4ctx_FrameBaASet(struct V4C__Context *,LOGICAL) ;
#endif
struct V4DPI__Point *v4ctx_DimValue(struct V4C__Context *,int,struct V4DPI__DimInfo **) ;
struct V4DPI__Point *v4ctx_DimPValue(struct V4C__Context *,int) ;
int v4ctx_DimBaseDim(struct V4C__Context *,int,int) ;
LOGICAL v4ctx_DimHasBindInfo(struct V4C__Context *,int,int) ;
void v4ctx_Resynch(struct V4C__Context *) ;
int v4ctx_HasHaveNBL(struct V4C__Context *,int) ;


/*	V 4 D P I			*/

void intPNTvMod(struct V4DPI__Point *,int) ;
void v4dpi_DimInitOnType(struct V4C__Context *,struct V4DPI__DimInfo *) ;
int v4dpi_DimMake(struct V4C__Context *,struct V4DPI__DimInfo *) ;
UCCHAR *v4dpi_DimUniqueName(struct V4C__Context *,UCCHAR *,UCCHAR *) ;
LOGICAL v4dpi_LocalDimAddEndBlock(struct V4C__Context *,LOGICAL) ;
int v4dpi_DimGet(struct V4C__Context *,UCCHAR *, int) ;
struct V4DPI__DimInfo *v4dpi_DimInfoGet(struct V4C__Context *,int) ;
int v4dpi_DimShellDimId(LOGICAL *,struct V4C__Context *,struct V4DPI__Point *) ;
int v4dpi_DimIndexDimId(LOGICAL *,struct V4C__Context *,struct V4DPI__Point *,INDEX *) ;
DIMID v4dpi_PointToDimId(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__DimInfo **,INTMODX,INDEX) ;
int v4dpi_DimUnqToCache(struct V4C__Context *,struct V4DPI__DimInfo *) ;
int v4dpi_DimUnqRmvFromCache(struct V4C__Context *,struct V4DPI__DimInfo *) ;

int v4dpi_DimUnqPntToCache(struct V4C__Context *,struct V4DPI__DimInfo *) ;
LOGICAL v4dpi_AddDUPoint(struct V4C__Context *,struct V4DPI__DimInfo *,struct V4DPI__Point *,int,ETYPE) ;
int v4dpi_DimUnique(struct V4C__Context *,struct V4DPI__DimInfo *,LOGICAL *) ;
struct V4DPI__Point *v4dpi_DimUniqueToList(struct V4DPI__Point *,struct V4C__Context *,int,int,struct V4DPI__Point *,LOGICAL) ;
int v4dpi_DimUniqueNumPoints(struct V4C__Context *,int) ;

void v4dpi_FlushXDict(struct V4C__Context *) ;
int v4dpi_XDictDimGet(struct V4C__Context *,UCCHAR *) ;
int v4dpi_XDictEntryGet(struct V4C__Context *,UCCHAR *,struct V4DPI__DimInfo *,int) ;
int v4dpi_XDictEntryPut(int,UCCHAR *,struct V4DPI__DimInfo *,int) ;
UCCHAR *v4dpi_RevXDictEntryGet(struct V4C__Context *,int,int) ;

int v4dpi_DictEntryGet(struct V4C__Context *,int,UCCHAR *,struct V4DPI__DimInfo *,int *) ;
int v4dpi_DictEntryEnum(struct V4C__Context *,int,struct V4DPI__DictList *,int) ;
int v4dpi_DictEntryPut(struct V4C__Context *,int,UCCHAR *,int,int,struct V4DPI__DimInfo *) ;
UCCHAR *v4dpi_RevDictEntryGet(struct V4C__Context *,int) ;

LOGICAL v4dpi_PointParse(struct V4C__Context *,struct V4DPI__Point *,struct V4LEX__TknCtrlBlk *,int) ;
LOGICAL v4dpi_ParsePointSlashPoint(struct V4C__Context *,struct V4DPI__Point *,struct V4LEX__TknCtrlBlk *,int) ;
LOGICAL v4dpi_ParsePointDashRangle(struct V4C__Context *,struct V4DPI__Point *,struct V4LEX__TknCtrlBlk *,int) ;
LOGICAL v4dpi_PointParseExp(struct V4C__Context *,struct V4DPI__Point *,struct V4LEX__TknCtrlBlk *,int) ;
LOGICAL v4dpi_ParseJSONSyntax(struct V4C__Context *,struct V4DPI__Point *,struct V4LEX__TknCtrlBlk *,FLAGS32,LOGICAL) ;
LOGICAL v4dpi_IsctParse(struct V4DPI__Point *,struct V4LEX__TknCtrlBlk *,struct V4C__Context *,struct V4DPI__Point *,int,int) ;
COUNTER v4dpi_NewLevelLocalSym() ;
LOGICAL v4dpi_PushLocalSym(struct V4C__Context *,UCCHAR *,struct V4DPI__Point *,struct V4DPI__Point *,ETYPE) ;
void v4dpi_PopLocalSyms(INDEX) ;

struct V4DPI__Point *v4dpi_MakeBigIsct(struct V4C__Context *,struct V4DPI__BigIsct *,struct V4DPI__Point *,int,struct V4DPI__Point *,int *) ;
//LOGICAL v4dpi_IsctParseSetNested(struct V4DPI__Point *) ;
LOGICAL v4dpi_ParseArray(struct V4DPI__Point *,struct V4LEX__TknCtrlBlk *,struct V4C__Context *,UCCHAR *,LOGICAL) ;
LOGICAL v4dpi_PointAccept(struct V4DPI__Point *,struct V4DPI__DimInfo *,struct V4LEX__TknCtrlBlk *,struct V4C__Context *,int) ;
LOGICAL v4dpi_PointAccept1(struct V4DPI__Point *,struct V4DPI__DimInfo *,struct V4LEX__TknCtrlBlk *,struct V4C__Context *,int,UCCHAR *) ;
ETYPE v4dpi_PointAcceptTreeOrList(struct V4DPI__Point *,struct V4DPI__DimInfo *,struct V4LEX__TknCtrlBlk *,struct V4C__Context *,FLAGS32) ;
LOGICAL v4dpi_ParseBigTextAttributes(struct V4C__Context *,struct V4LEX__TknCtrlBlk *,struct V4LEX__TablePrsOpt *,struct V4DPI__Point *point,UCCHAR *,UCCHAR *,UCCHAR *) ;

LOGICAL v4dpi_SaveBigTextPoint(struct V4C__Context *,struct V4LEX__BigText *,int,struct V4DPI__Point *,int,LOGICAL) ;
LOGICAL v4dpi_SaveBigTextPoint2(struct V4C__Context *,UCCHAR *,struct V4DPI__Point *,int,LOGICAL) ;

UCCHAR *v4dpi_PointToString(UCCHAR *,struct V4DPI__Point *,struct V4C__Context *,int) ;
UCCHAR *v4dpi_PointToStringML(UCCHAR *,struct V4DPI__Point *,struct V4C__Context *,int,int) ;
void v4dpi_IsctToString(UCCHAR *,struct V4DPI__Point *,struct V4C__Context *,int,int) ;

UCCHAR *v4_BigTextCharValue(struct V4C__Context *,struct V4DPI__Point *) ;

struct V4DPI__Point *v4dpi_ScopePnt_Alloc(int) ;
void v4dpi_ScopePnt_Free(int) ;

int v4dpi_PntIdx_AllocPnt() ;
void v4dpi_PntIdx_Shutdown() ;
struct V4DPI__Point *v4dpi_PntIdx_CvtIdxPtr(int) ;

LOGICAL v4dpi_UOMInitialize(struct V4C__Context *,int) ;
LOGICAL v4dpi_UOMAttemptCoerce(struct V4C__Context *,int,int,int,struct V4DPI__Point *,double *) ;

UCCHAR *v4dbg_FormatLineNum(int) ;
UCCHAR *v4dbg_FormatBP(int) ;
UCCHAR *v4dbg_FormatVBP(struct V4C__Context *,struct V4DBG__BreakPoint *,union V4DPI__IsctSrc *,int,LOGICAL) ;
void v4dbg_bplInit() ;
int v4dbg_BreakPoint(struct V4C__Context *,struct V4DBG__BreakPoint *,struct V4DPI__Point *,int,LOGICAL,struct V4DPI__Point *) ;

void v4dpi_IsctEvalCacheClear(struct V4C__Context *,LOGICAL) ;
struct V4DPI__Point *v4dpi_IsctEval(struct V4DPI__Point *,struct V4DPI__Point *,struct V4C__Context *,int,int *,struct V4DPI__EvalList *) ;
struct V4DPI__Point *v4dpi_IsctEvalFail(struct V4DPI__Point *,struct V4DPI__Point *,struct V4C__Context *,int,int *,struct V4DPI__EvalList *) ;
struct V4DPI__Point *v4dpi_MaxNesting(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *,LOGICAL *) ;
struct V4DPI__Point *v4dpi_IsctContEval(int,int,struct V4DPI__Point *,struct V4DPI__Point *,struct V4C__Context *,int,int *,struct V4DPI__EvalList *) ;
int v4dpi_IsctEvalDoit(struct V4DPI__EvalList *,struct V4DPI__Point *,struct V4C__Context *,int) ;

#ifdef WANTPATTERNMATCH
struct V4DPI__Point *v4dpi_MatchPattern(struct V4DPI__Point *,struct V4DPI__BindList *,struct V4DPI__BindList_Value *,struct V4C__Context *,int,int) ;
#endif

LOGICAL v4dpi_FlattenIsct(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *,int) ;

P *v4dpi_AcceptValue(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__DimInfo *,ETYPE,void *,LENMAX) ;
LOGICAL v4dpi_ValSpclAcptr(struct V4C__Context *,struct V4DPI__DimInfo *,char *,UCCHAR *,struct V4DPI__Point *) ;

DICTID vjson_ParseString(struct V4C__Context *,struct V4LEX__TknCtrlBlk *,UCCHAR *,FLAGS32) ;
struct VJSON__Value vjson_GetValue(struct VJSON__ParseInfo *,LOGICAL *,FLAGS32) ;
INDEX vjson_StashValue(struct VJSON__ParseInfo *,void *,LENMAX) ;
LENMAX vjson_SerializeValue(struct V4C__Context *,struct VJSON__Blob *,struct VJSON__Value *,UCCHAR *,LENMAX) ;
LENMAX vjson_SerializeBlob(struct V4C__Context *,struct VJSON__Blob *,UCCHAR *,LENMAX) ;
P *vjson_Dereference(struct V4C__Context *,struct V4DPI__Point *,struct VJSON__Blob *,struct VJSON__Value *,struct V4DPI__Point *[],COUNTER,INDEX argStart,INTMODX intmodx,LOGICAL) ;
struct VJSON__Blob *vjson_LocateBlob(struct V4C__Context *,struct V4DPI__Point *,struct VJSON__Value *,DICTID *) ;
LOGICAL vjson_DeleteBlob(struct V4C__Context *,struct V4DPI__Point *) ;
LOGICAL vjson_ReplaceValue(struct VJSON__Blob **,struct VJSON__Value *,ETYPE,void *,COUNTER) ;
P *vjson_TypeOf(struct V4C__Context *,struct V4DPI__Point *,struct VJSON__Value *vjval) ;


/*	V 4 E V A L			*/

int v4eval_Eval(struct V4LEX__TknCtrlBlk *,struct V4C__Context *,LOGICAL,int,LOGICAL,LOGICAL,LOGICAL,struct V4DPI__Point *) ;
LOGICAL v4eval_ChkEOL(struct V4LEX__TknCtrlBlk *) ;
LOGICAL v4eval_NextAsString(struct V4C__Context *,struct V4LEX__TknCtrlBlk *,int,LOGICAL) ;
int v4eval_SetIfLevel(struct V4C__Context *,struct V4LEX__TknCtrlBlk *) ;
LOGICAL v4eval_CheckIfLevel(struct V4C__Context *,struct V4LEX__TknCtrlBlk *) ;
int v4eval_GetFileUpdateDT(struct V4C__Context *,UCCHAR *) ;
struct V4LEX__Table *v4eval_GetTable(struct V4C__Context *,UCCHAR *,LOGICAL *) ;
LOGICAL v4dpi_BindListMake(struct V4DPI__Binding *,struct V4DPI__Point *,struct V4DPI__Point *,struct V4C__Context *,int *,int,int,INDEX) ;
LOGICAL v4dpi_BindListRemove(struct V4C__Context *,struct V4DPI__BindPointVal *) ;
void v4dpi_ExamineBindList(struct V4DPI__BindList *,struct V4C__Context *,int) ;
char *v4dpi_GetBindBuf(struct V4C__Context *,int,struct V4IS__IndexKeyDataEntry  *,LOGICAL,int *) ;
struct V4IM__AggLookAhead *v4im_ALA_InitALA(struct V4C__Context *,struct V4IM__AggLookAhead *,struct V4DPI__Point *) ;
void v4im_ALA_Cleanup(struct V4IM__AggLookAhead *) ;
struct V4IM__AggShell *v4im_ALA_ConsumeNextAggs(struct V4IM__AggLookAhead *) ;
void v4im_ALA_ProducerThread(struct V4IM__AggLookAhead *) ;
LOGICAL v4im_ALA_GrabNextBuffer(struct V4IM__AggLookAhead *,int) ;
int v4im_AggLoadArea(struct V4C__Context *,struct V4IS__ParControlBlk *, struct V4DPI__LittlePoint *) ;
void v4im_AggInit(struct V4IM__AggShell *,struct V4IM__AggHdr *,struct V4IM__AggData *,struct V4DPI__Point *) ;
LOGICAL v4im_AggAppendVal(struct V4C__Context *,struct V4IM__AggShell *,struct V4IM__AggHdr *,struct V4IM__AggData *,struct V4DPI__Point *,struct V4DPI__Point *) ;
int v4im_AggBuild(struct V4C__Context *,struct V4IM__AggShell *,struct V4IM__AggHdr *,struct V4IM__AggData *,LOGICAL,LOGICAL,INDEX) ;
LOGICAL v4im_AggPutBlocked(struct V4C__Context *,struct V4IM__AggShell *,INDEX) ;
char *v4im_AggGetBlocked(struct V4C__Context *,struct V4IM__LocalVHdr *,struct V4DPI__Point *,int,struct V4IM__AggShell **,int *) ;
struct V4DPI__Point *v4im_AggGetValue(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *,int,struct V4DPI__Point *) ;
LOGICAL v4im_AggDelete(struct V4C__Context *,struct V4DPI__Point *) ;
LOGICAL v4im_AggUpdValue(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *,int,struct V4DPI__Point *) ;
struct V4DPI__Point *v4im_AggUpdRes(struct V4C__Context *,struct V4DPI__Point *,int,void *,int,int,int) ;
int v4im_AggPut(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int,LOGICAL,LOGICAL,LOGICAL,INDEX) ;
int v4eval_AggCAggPut(struct V4C__Context *,struct V4DPI__Point *) ;
struct V4DPI__Point *v4eval_AggCAggGet(struct V4C__Context *,struct V4DPI__Point *,int,int) ;
struct V4IM__AggVHdr *v4im_AggGetVHdr(struct V4C__Context *,int) ;
int v4eval_AggParse(struct V4LEX__TknCtrlBlk *,struct V4C__Context *,int,struct V4DPI__Point *,LOGICAL,LOGICAL) ;
void v4trace_ExamineState(struct V4C__Context *,struct V4DPI__Point *,FLAGS32,int) ;
struct V4LEX__CompileDir *v4trace_LoadVCDforHNum(struct V4C__Context *,int,LOGICAL) ;
void v4trace_RegisterError(struct V4C__Context *) ;
struct V4V4IS__StructEl *v4v4is_LoadFileRef(struct V4C__Context *,int) ;
void v4v4is_FlushStEl(struct V4C__Context *) ;
int v4eval_ScanAreaForBinds(struct V4C__Context *,int,int,struct V4DPI__Point *,struct V4DPI__Point *,struct V4DPI__Point *) ;
int v4eval_ScanForDim(struct V4C__Context *,int,int,struct V4DPI__Point *,struct V4DPI__Point *) ;
struct V4DPI__BindListShell *v4eval_ReadNextNBL(struct V4C__Context *,int,int,union V4IS__KeyPrefix *) ;
int v4eval_NextDimInBind(struct V4C__Context *,int,struct V4DPI__BindListShell *,int,int,struct V4DPI__Point *,struct V4DPI__Point *) ;
int v4eval_OptPoint(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *,int,struct V4EVAL_OptDimList *) ;
LOGICAL v4eval_TryDefineMacro(struct V4C__Context *,struct V4LEX__TknCtrlBlk *,struct V4Eval__MacroCache **,UCCHAR *,UCCHAR *,int *,LOGICAL,UCCHAR *,int) ;
struct V4LEX__MacroEntry *v4eval_ParseMacroDef(struct V4C__Context *,struct V4LEX__TknCtrlBlk *,VTRACE) ;
struct V4LEX__MacroEntry *v4eval_SaveMacroBodyOnly(struct V4C__Context *,UCCHAR *,UCCHAR *,ETYPE,INTMODX) ;
struct V4LEX__MacroCallArgs *v4lex_ParseMacroCall(struct V4C__Context *,struct V4LEX__TknCtrlBlk *,struct V4LEX__MacroEntry *) ;
LOGICAL v4lex_InvokeMacroCall(struct V4C__Context *,struct V4LEX__TknCtrlBlk *,struct V4LEX__MacroEntry *,struct V4LEX__MacroCallArgs *) ;
LOGICAL v4eval_SaveMacroDir(struct V4C__Context *) ;
struct V4LEX__MacroEntry *v4eval_GetMacroEntry(struct V4C__Context *,UCCHAR *,ETYPE) ;
struct V4LEX__MacroDirectory *v4eval_GetMacroDir(struct V4C__Context *) ;



/*	V 4 E X T E R N			*/

LOGICAL v4html_GetNextPiece(struct V4HTML__Page *,UCCHAR *) ;
LOGICAL v4html_SectionBounds(struct V4HTML__Page *,int,UCCHAR *) ;
LOGICAL v4html_GetCurrentId(struct V4HTML__Page *,UCCHAR *,UCCHAR *) ;
LOGICAL v4html_GetCurrentSrc(struct V4HTML__Page *,UCCHAR *,UCCHAR *) ;
LOGICAL v4html_GetHyperLink(struct V4HTML__Page *,UCCHAR *,UCCHAR *) ;
UCCHAR *v4html_GetFormValue(struct V4HTML__Page *,UCCHAR *,UCCHAR *) ;
LOGICAL v4html_SetFormValue(struct V4HTML__Page *,UCCHAR *,UCCHAR *,UCCHAR *) ;
LOGICAL v4html_LoadFormInfo(struct V4HTML__Page *,UCCHAR *) ;
struct V4HTML__Page *v4html_PostForm(struct V4HTML__Page *,UCCHAR *) ;
void v4html_MakeBoundary(struct V4HTML__Page *,UCCHAR *,int) ;
LOGICAL v4html_PositionInPage(struct V4HTML__Page *,int,int,UCCHAR *,UCCHAR *) ;
struct V4HTML__Page *v4html_GoBackPage(struct V4HTML__Page *,UCCHAR *) ;
struct V4HTML__Page *v4html_PushSection(struct V4HTML__Page *,UCCHAR *) ;
LOGICAL v4html_PositionRowColumn(struct V4HTML__Page *,int,int,UCCHAR *) ;
struct V4HTML__Page *v4html_FollowHyperLink(struct V4HTML__Page *,UCCHAR *,UCCHAR *,UCCHAR *,UCCHAR *,int,struct V4HTML__Page *) ;
int v4html_RequestPage(struct V4HTML__Page *,int,UCCHAR *,int,UCCHAR *,UCCHAR *,int,UCCHAR *,UCCHAR *) ;
int v4html_ParseURL(struct V4HTML__Page *,UCCHAR *,UCCHAR *,UCCHAR *,UCCHAR *,int,int *) ;
int v4html_KeywordValuePair(struct V4HTML__Page *,int,int,UCCHAR *,UCCHAR *,UCCHAR *) ;


struct V4DPI__Point *v4im_Init_ltf(struct V4C__Context *,int,struct V4DPI__Point *,struct V4DPI__Point *,UCCHAR *,int,int) ;

void lzrw1_compress(UBYTE *,ULONG,UBYTE *,ULONG *) ;
void lzrw1_decompress(UBYTE *,ULONG,UBYTE *,ULONG *,ULONG) ;


int v_ReadChunk(int,void *,int) ;
int v_FileCompress(UCCHAR *,UCCHAR *,int,LOGICAL,UCCHAR *,double *) ;
int v_FileExpand(UCCHAR *,UCCHAR *,UCCHAR *) ;


struct V4DPI__Point *v4db_DodbConnect(struct V4C__Context *,struct V4DPI__Point *,int,int,struct V4DPI__Point *[],int) ;
struct V4DPI__Point *v4db_Error(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
void v4xdb_FreeStuff(struct V4C__Context *,COUNTER,FLAGS32) ;
LENMAX v4xdb_SaveXDBRow(struct V4C__Context *,struct V4XDB__Master *,INDEX) ;
struct V4XDB_SavedRow *v4xdb_GetXDBRow(struct V4C__Context *,struct V4XDB__Master *,struct V4XDB__SaveStmtInfo *,XDBRECID) ;
LOGICAL v4xdb_NewStmtInitCommon(struct V4C__Context *,struct V4XDB__Master *,XDBID,ETYPE,UCCHAR *,INTMODX,INDEX *,INDEX *,XDBID *,VTRACE) ;
void v4xdb_NewStmtFinish(struct V4C__Context *,struct V4XDB__Master *,INDEX,XDBID) ;
LOGICAL v4xdb_FreeSavedInfo(DIMID) ;

#ifdef WANTODBC
LOGICAL v4db_GetCurColVal(struct V4C__Context *,INDEX,INDEX,DIMID,P *) ;
COUNTER v4odbc_Connect(struct V4C__Context *,struct V4DPI__DimInfo *,int,UCCHAR *,UCCHAR *,UCCHAR *,UCCHAR *,LOGICAL,LOGICAL *) ;
INDEX v4odbc_NewStmt(struct V4C__Context *,COUNTER,UCCHAR *,int,int) ;
struct V4DPI__Point *v4odbc_NewTableStmt(struct V4C__Context *,struct V4DPI__Point *,COUNTER,UCCHAR *,UCCHAR *,int,int) ;
INDEX v4odbc_Fetch(struct V4C__Context *,COUNTER,XDBROW,LOGICAL *,int, struct V4DPI__Point *) ;
//struct V4DPI__Point *v4odbc_GetRowColVal(struct V4C__Context *,struct V4DPI__Point *,COUNTER,struct V4DPI__DimInfo *,int,int,struct V4DPI__Point *,int,int) ;
LOGICAL v4odbc_SetupRowCol(struct V4C__Context *,struct V4XDB__Master *,int,int) ;
void v4odbc_FreeStuff(struct V4C__Context *,COUNTER,FLAGS32) ;
struct V4DPI__Point *v4odbc_MIDASRowColVal(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__DimInfo *,int,int,struct V4DPI__Point *,int,int) ;
struct V4DPI__Point *v4odbc_Xct(struct V4C__Context *,struct V4DPI__Point *,LOGICAL,COUNTER,UCCHAR *,enum DictionaryEntries,int,int) ;
#endif

#ifdef WANTV4IS
void v4midas_FreeStuff(struct V4C__Context *,COUNTER,FLAGS32) ;
COUNTER v4odbc_MIDASConnect(struct V4C__Context *,int,int,struct V4DPI__DimInfo *,UCCHAR *,UCCHAR *,UCCHAR *,UCCHAR *,LOGICAL *) ;
struct V4DPI__Point *v4odbc_MIDASXct(struct V4C__Context *,struct V4DPI__Point *,LOGICAL,int,UCCHAR *,enum DictionaryEntries,int,int) ;
INDEX v4odbc_MIDASNewStmt(struct V4C__Context *,XDBID,UCCHAR *,INTMODX,VTRACE) ;
//INDEX v4mysql_NewTableStmt(struct V4C__Context *ctx, COUNTER, UCCHAR *, INTMODX, VTRACE) ;
LOGICAL v4odbc_MIDASSendRcvRequest(struct V4C__Context *,int,struct V4XDB_MIDASTable *,int,char *) ;
INDEX v4midas_Fetch(struct V4C__Context *,COUNTER,XDBROW,LOGICAL *,int, struct V4DPI__Point *) ;
#endif

#ifdef WANTMYSQL
XDBID v4mysql_Connect(struct V4C__Context *,struct V4DPI__DimInfo *,INTMODX,UCCHAR *,int,UCCHAR *,UCCHAR *,UCCHAR *,LOGICAL,LOGICAL *) ;
INDEX v4mysql_NewStmt(struct V4C__Context *,XDBID,UCCHAR *,INTMODX,VTRACE,COUNTER) ;
P *v4mysql_Xct(struct V4C__Context *,struct V4DPI__Point *,LOGICAL,XDBID,UCCHAR *,enum DictionaryEntries,INTMODX,VTRACE,COUNTER) ;
INDEX v4mysql_Fetch(struct V4C__Context *,XDBID,XDBROW,LOGICAL *,INTMODX,struct V4DPI__Point *) ;
void v4mysql_FreeStuff(struct V4C__Context *,XDBID,FLAGS32) ;
#endif

#ifdef WANTORACLE
XDBID v4oracle_Connect(struct V4C__Context *,struct V4DPI__DimInfo *,INTMODX,UCCHAR *,int,UCCHAR *,UCCHAR *,UCCHAR *,LOGICAL,LOGICAL *) ;
INDEX v4oracle_NewStmt(struct V4C__Context *,XDBID,UCCHAR *,INTMODX,VTRACE,COUNTER) ;
P *v4oracle_Xct(struct V4C__Context *,struct V4DPI__Point *,LOGICAL,XDBID,UCCHAR *,enum DictionaryEntries,INTMODX,VTRACE,COUNTER) ;
INDEX v4oracle_Fetch(struct V4C__Context *,XDBID,XDBROW,LOGICAL *,INTMODX,struct V4DPI__Point *) ;
void v4oracle_FreeStuff(struct V4C__Context *,XDBID,FLAGS32) ;
#endif

#if defined WANTODBC || defined WANTMYSQL || defined WANTORACLE
struct V4DPI__Point *v4db_Info(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
#endif

struct V4DPI__Point *vxml_RetrieveValueAsV4Point(struct V4C__Context *,struct VXML__Message *,struct V4DPI__Point *,UCCHAR *) ;
int vxml_RPCArg(struct V4C__Context *,struct V4DPI__Point *,UCCHAR *,int) ;
int vxml_ParseNext(struct VXML__Message *,int) ;
void vxml_MessageSetup(struct VXML__Message *,UCCHAR *) ;
int vxml_MessageNext(struct VXML__Message *) ;
LOGICAL v4sxi_SpecialListOfKW(struct V4C__Context *,struct V4DPI__Point *,int,int) ;
int v4sxi_SpecialAddKW(struct V4C__Context *,int,struct V4__PTBitMap *,double,UCCHAR *) ;
LOGICAL v4sxi_SpecialDelEntry(struct V4C__Context *,int,int) ;
LOGICAL v4sxi_SpecialAcceptor(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__DimInfo *,UCCHAR *) ;
LOGICAL v4sxi_SpecialDisplayer(struct V4C__Context *,struct V4DPI__Point *,int,UCCHAR *) ;


struct V4Tree__Node *v4tree_MakeTree(struct V4C__Context *,DIMID,int) ;
LOGICAL v4tree_SaveTrees(struct V4C__Context *) ;
struct V4Tree__Master *v4tree_TreeMaster(struct V4C__Context *,TREEID) ;
struct V4Tree__Node *v4tree_NewNode(struct V4C__Context *,struct V4Tree__Master *,LOGICAL) ;
BYTE *v4tree_AllocChunk(struct V4C__Context *,struct V4Tree__Master *,int) ;
struct V4Tree__Node *v4tree_FindNode2(struct V4Tree__Master *,struct V4Tree__Node *,NODEID,int) ;
struct V4Tree__Node *v4tree_FindNode(struct V4C__Context *,struct V4Tree__Master *,NODEID) ;
struct V4Tree__Node *v4tree_ParentNode(struct V4Tree__Master *,struct V4Tree__Node *) ;
LOGICAL v4tree_RemoveNode(struct V4C__Context *,struct V4Tree__Master *,NODEID) ;
struct V4Tree__Node *v4tree_ElevateChildren(struct V4C__Context *,struct V4Tree__Master *,NODEID) ;
LOGICAL v4tree_SwapNodes(struct V4C__Context *,struct V4Tree__Master *,NODEID,NODEID) ;
LOGICAL v4tree_FellTree(struct V4C__Context *,TREEID) ;
LOGICAL v4tree_SetTreeHash64(struct V4C__Context *,TREEID) ;
TREEID v4tree_FindIdenticalTree(struct V4C__Context *,TREEID) ;
LOGICAL v4tree_ChildList(struct V4C__Context *,struct V4Tree__Master *,struct V4Tree__Node *,struct V4L__ListPoint *,struct V4DPI__Point *) ;
struct V4Tree__Node *v4tree_RelNode(struct V4C__Context *,struct V4Tree__Master *,NODEID,int) ;
struct V4Tree__Node *v4tree_SproutNode(struct V4C__Context *,struct V4Tree__Master *,NODEID,int,struct V4Tree__Node *) ;
int v4tree_Hash64(struct V4C__Context *,struct V4Tree__Master *,struct V4Tree__Node *,B64INT *,COUNTER,LOGICAL *) ;
int v4tree_Enum(struct V4C__Context *,struct V4Tree__Master *,struct V4Tree__Node *,struct V4DPI__Point *,struct V4DPI__Point *,struct V4DPI__Point *,struct V4DPI__Point *,struct V4L__ListPoint *,int,int) ;
struct V4Tree__Node *v4tree_FindNodeDepth(struct V4C__Context *,struct V4Tree__Master *,struct V4Tree__Node *,struct V4DPI__Point *,struct V4L__ListPoint *) ;
struct V4Tree__Node *v4tree_FindNodeBreadth(struct V4C__Context *,struct V4Tree__Master *,struct V4Tree__Node *,int,int,int *,struct V4DPI__Point *,struct V4L__ListPoint *, LOGICAL *) ;
struct V4Tree__Node *v4im_RPPtoTree(struct V4C__Context *,struct V4LEX__RevPolParse *,LOGICAL) ;



struct V4DPI__Point *v4im_DoOSExt(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoGuiAlert(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoGuiMsgBox(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoHTML(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoHTTP(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoFTP(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoSendMail(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoOSFile(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
LOGICAL v_NextFile(struct V4L__ListFilesInfo *) ;
LOGICAL v_GetFileInfo(struct V4C__Context *,FLAGS32,UCCHAR *,UCCHAR *,UCCHAR *,UCCHAR *,LOGICAL *,int *,int *,double *,UCCHAR *) ;
struct V4DPI__Point *v4im_DoGraphConnect(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
int v4graph_NextEdge2(struct V4C__Context *,struct V4GRAPH__Connect *,int) ;
LOGICAL v4graph_ConnectSuccess(struct V4C__Context *,struct V4GRAPH__Connect *) ;
void vts_Random(struct VTS__Graph *,struct VTS__Path *) ;
LOGICAL vts_Improve(struct VTS__Graph *,struct VTS__Path *) ;
LOGICAL vts_ImproveCross(struct VTS__Graph *,struct VTS__Path *) ;
void vts_GetLength(struct VTS__Graph *,struct VTS__Path *) ;
void vts_LocalOptimize(struct VTS__Graph *,struct VTS__Path *) ;
void vts_Optimize(struct VTS__Graph *,struct VTS__Path *,int) ;
struct V4DPI__Point *vts_Driver(struct V4C__Context *,struct V4DPI__Point *,int,int,LOGICAL,struct V4L__ListPoint *,struct V4DPI__Point *,struct V4DPI__Point *,int,int) ;
struct V4DPI__Point *v4im_DoOutput(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
int v4im_OutputGetStream(struct V4C__Context *,struct V4DPI__Point *) ;
struct V4DPI__Point *v4im_DoOSInfo(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoSocket(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoTimer(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoTree(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_V4ISCon(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_V4ISOp(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_V4ISVal(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoXMLRPC(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;

struct V4PL__ProcLock *v4pl_linkToSharedSegment(struct V4C__Context *,UCCHAR *,LENMAX,LENMAX) ;
UNIQUEID v4pl_RegisterProc(struct V4C__Context *,struct V4PL__ProcLock *,UCCHAR *) ;
LOGICAL v4pl_removeDefunctPids(struct V4C__Context *,struct V4PL__ProcLock *,UNIQUEID) ;
UNIQUEID v4pl_grabLockMultiple(struct V4PL__grabLockArgs *) ;
UNIQUEID v4pl_grabLock(struct V4PL__grabLockArgs *) ;
UNIQUEID v4pl_grabSingleLock(struct V4PL__grabLockArgs *,UCCHAR *) ;
UNIQUEID v4pl_getLockId(struct V4C__Context *,struct V4PL__ProcLock *,UCCHAR *) ;
LOGICAL vrpl_releaseLock(struct V4C__Context *,struct V4PL__ProcLock *,UNIQUEID,UNIQUEID) ;
LOGICAL v4pl_isProcessGone(PID) ;
struct V4DPI__Point *v4im_DoLock(struct V4C__Context *,P *,P *[],COUNTER,INTMODX,VTRACE) ;
struct V4DPI__Point *v4im_DoEcho(struct V4C__Context *,P *,P *[],COUNTER,INTMODX,VTRACE) ;
struct V4DPI__Point *v4im_DoEchoE(struct V4C__Context *,P *,P *[],COUNTER,INTMODX,VTRACE) ;
struct V4DPI__Point *v4im_DoEchoT(struct V4C__Context *,P *,P *[],COUNTER,INTMODX,VTRACE) ;
struct V4DPI__Point *v4im_DoSSDim(struct V4C__Context *,P *,P *[],COUNTER,INTMODX,VTRACE) ;
INDEX v4im_SSDimSetMask(struct V4C__Context *,struct V4DPI__DimInfo *,UCCHAR *,INTMODX) ;
struct V4DPI__Point *v4im_DoZip(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int) ;



/*	V 4 F I N			*/

struct V4IM_DblArray *v_InitDblArray(int) ;
struct V4IM_DblArray *v_EnlargeDblArray(struct V4IM_DblArray *,int) ;
int v4fin_CouponDateCalc(int,int,int,int,int,LOGICAL *) ;
int v4fin_CDCDate(int,int,int,int) ;
double v4fin_CvtDec(double,double) ;
double v4fin_CvtFrac(double,double) ;
int v4fin_Days360(int,int) ;
int v4fin_DaysDif(int,int,int) ;
double v4fin_DepFDB(double,double,double,double,double,LOGICAL *) ;
double v4fin_DepDDB(double,double,double,double,double,double,LOGICAL *) ;
double v4fin_DepSLN(double,double,double,double,double,LOGICAL *) ;
double v4fin_DepSYD(double,double,double,double,double,LOGICAL *) ;
double v4fin_Disc(int,int,double,double,int,LOGICAL *) ;
double v4fin_FVSched(double,int,double []) ;
double v4fin_IntAccPeriodic(int,int,int,double,double,int,int,LOGICAL *) ;
double v4fin_IntAccrMat(int,int,double,double,int) ;
double v4fin_IntRateEff(double,double) ;
double v4fin_IntRateEffCont(double) ;
double v4fin_IntRateNom(double,double) ;
double v4fin_IntRateTaxInfl(double,double,double) ;
double v4fin_IntRecSec(double,double,int,int,int,LOGICAL *) ;
struct V4DPI__Point *v4fin_IRR(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point * [],int,int,int) ;
double v4fin_IRRCalc(int,double [],LOGICAL *) ;
double v4fin_Macaulay(int,int,int,int,double,double,double,int,int,LOGICAL *) ;
double v4fin_NPV(double,int,double []) ;
double v4fin_Price(int,int,double,double,double,int,int) ;
struct V4DPI__Point *v4fin_TBill(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point * [],int,int,int) ;
double v4fin_TVMFV(double,double,double,double,int) ;
double v4fin_TVMIPPMT(int,double,double,double,double,double,int,LOGICAL *) ;
double v4fin_TVMPMT(double,double,double,double,int) ;
double v4fin_TVMPeriods(double,double,double,double,int,LOGICAL *) ;
double v4fin_TVMPV(double,double,double,double,int) ;
double v4fin_TVMRate(double,double,double,double,int,LOGICAL *) ;
struct V4DPI__Point *v4fin_TVM(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point * [],int,int,int) ;
struct V4DPI__Point *v4fin_Coupon(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point * [],int,int,int) ;
struct V4DPI__Point *v4fin_Dep(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point * [],int,int,int) ;
struct V4DPI__Point *v4fin_IntRate(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point * [],int,int,int) ;
struct V4DPI__Point *v4fin_FinSecDisc(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point * [],int,int,int) ;
struct V4DPI__Point *v4fin_FinSecDur(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point * [],int,int,int) ;
struct V4DPI__Point *v4fin_FinSecInt(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point * [],int,int,int) ;
struct V4DPI__Point *v4fin_FinSecPrice(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point * [],int,int,int) ;
struct V4DPI__Point *v4fin_FinSecRcvd(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point * [],int,int,int) ;
struct V4DPI__Point *v4fin_FinSecYield(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point * [],int,int,int) ;


/*	V 4 H A S H		*/

int v4h_PutHash(struct V4IS__ParControlBlk *,struct V4IS__AreaCB *,struct V4IS__Key *,char *,int) ;
LOGICAL v4h_GetHashKey(struct V4IS__ParControlBlk *,struct V4IS__AreaCB *,char *,int,struct V4IS__Key *,int) ;
LOGICAL v4h_GetHashDataId(struct V4IS__ParControlBlk *,struct V4IS__AreaCB *,char *,int,struct V4IS__Key *,int) ;
void v4h_CheckPoint(struct V4IS__ParControlBlk *,struct V4IS__AreaCB *) ;


/*	V 4 I M				*/

LOGICAL v4im_InitConstVals(struct V4C__Context *) ;
struct V4DPI__Point *v4im_IntModHandler(struct V4DPI__Point *,struct V4DPI__Point *,struct V4C__Context *,int) ;








/*	V 4 I M U			*/

enum DictionaryEntries v4im_GetDictToEnumVal(struct V4C__Context *,struct V4DPI__Point *) ;
enum DictionaryEntries v4im_GetUCStrToEnumVal(UCCHAR *,int) ;
enum DictionaryEntries v4im_GetTCBtoEnumVal(struct V4C__Context *,struct V4LEX__TknCtrlBlk *,LOGICAL) ;
int v4im_GetEnumToDictVal(struct V4C__Context *,enum DictionaryEntries,int) ;
UCCHAR *v4im_GetEnumToUCVal(enum DictionaryEntries) ;

LOGICAL v4im_ListCache_BuildMap(struct V4C__Context *,struct V4IM__ListCache *,int,int,struct V4DPI__Point *,int) ;
struct V4DPI__Point *v4im_ListCache_BuildList(struct V4C__Context *,struct V4IM__ListCache *,struct V4DPI__Point *,int) ;
UCCHAR *v4im_PTName(int) ;
int v4im_PTId(UCCHAR *) ;
int v4im_TagValue(UCCHAR *) ;
void v4im_TagRange(int*,int*) ;
UCCHAR *v4im_TagName(int) ;
//LOGICAL v4im_CacheIsctIntModOK(int) ;
INTMODX v4im_Accept(UCCHAR *) ;
UCCHAR *v4im_Display(INTMODX) ;
void v4im_IntModRange(int *,int *) ;
UCCHAR *v4im_ModFailStr(int) ;
void v4im_InitStuff(char []) ;
LOGICAL v4im_CouldBeList(struct V4C__Context *,struct V4DPI__Point *) ;
struct V4L__ListPoint *v4im_VerifyList(struct V4DPI__Point *,struct V4C__Context *,struct V4DPI__Point *,int) ;

double v4im_EvalArithExp(struct V4C__Context *,struct V4LEX__TknCtrlBlk *,int,LOGICAL *,struct V4DPI__Point *,LOGICAL) ;
struct V4DPI__Point *v4im_EvalListExp(struct V4C__Context *,struct V4DPI__Point *,struct V4LEX__TknCtrlBlk *,int,LOGICAL *,int,LOGICAL) ;

double v4im_GetPointDbl(LOGICAL *,struct V4DPI__Point *,struct V4C__Context *) ;
B64INT v4im_GetPointFixed(LOGICAL *,struct V4DPI__Point *,struct V4C__Context *,int) ;
LOGICAL v4im_GetPointFileName(LOGICAL *,UCCHAR *,int,struct V4DPI__Point *,struct V4C__Context *,UCCHAR *) ;
int v4im_GetPointChar(LOGICAL *,char *,int,struct V4DPI__Point *,struct V4C__Context *) ;
int v4im_GetPointUC(LOGICAL *,UCCHAR *,int,struct V4DPI__Point *,struct V4C__Context *) ;
int v4im_GetPointInt(LOGICAL *,struct V4DPI__Point *,struct V4C__Context *) ;
int v4im_GetPointUD(LOGICAL *,struct V4DPI__Point *,struct V4C__Context *) ;
double v4im_GetPointDur(LOGICAL *,struct V4DPI__Point *,struct V4C__Context *) ;
double v4im_GetPointCal(LOGICAL *,struct V4DPI__Point *,struct V4C__Context *) ;
LOGICAL v4im_GetPointLog(LOGICAL *,struct V4DPI__Point *,struct V4C__Context *) ;

void v4im_SetPointValue(struct V4C__Context *,struct V4DPI__Point *,double) ;

void *v4im_tallyAllocAndCopy(struct V4IM__TallyMaster *,void *,LENMAX) ;
LOGICAL v4im_TallyListEnum(struct V4IM_TallyEnumEnvironment *) ;
LOGICAL v4im_MTGrabLock(struct V4C__Context *,DCLSPINLOCK *) ;
int v4im_TallyEntryInsert(struct V4C__Context *,struct V4IM_TallyEnumEnvironment *,struct V4IM__TallyEntry *,struct V4IM__TallyEntryWith *,int) ;
void v4im_TallyEntryAlloc(struct V4IM__TallyMaster *) ;
int v4im_TallyMakeBinds(struct V4C__Context *,struct V4IM__TallyMaster *,int,struct V4DPI__Point *) ;
int v4im_TallyMakeProjections(struct V4C__Context *,struct V4IM__TallyMaster *,int,struct V4DPI__Point *) ;
int v4im_TallyList(struct V4C__Context *,struct V4DPI__Point *,struct V4IM_TallyEnumEnvironment *,int,struct V4IM__TallyEntry *) ;
LOGICAL v4im_TallyListMake(struct V4C__Context *,struct V4IM__TallyMaster *,int,int,struct V4DPI__Point *) ;
LOGICAL v4im_TallyUCount(struct V4C__Context *,struct V4IM__TallyMaster *,int,int) ;
LOGICAL v4im_TallyMakeByList(struct V4C__Context *,struct V4IM__TallyMaster *,int,struct V4DPI__Point *) ;
LOGICAL v4im_TallyMakeArray(struct V4C__Context *,struct V4IM__TallyMaster *,INDEX,INTMODX) ;
void v4im_TallyAreaWrite(struct V4C__Context *,FILE *,struct V4IM__TallyMaster *,int,int) ;


LOGICAL v4im_ListHandCollapse(struct V4C__Context *,struct V4L__ListPoint *,struct V4DPI__Point *,int,int,int) ;
int v4im_SetPointLoad(struct V4C__Context *,struct V4IM__SetPoint *,struct V4L__ListPoint *,int) ;
struct V4DPI__Point *v4im_SetPointMakeList(struct V4C__Context *,struct V4DPI__Point *,struct V4IM__SetPoint *,int,LOGICAL) ;

int v4im_TagId(struct V4DPI__Point *) ;
int v4im_CheckPtArgNew(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point **,struct V4DPI__Point *) ;
void v4im_BaAParse(struct V4C__Context *,struct V4IM__BaA *,int,int,struct V4DPI__Point *,int,LOGICAL *) ;
int v4im_BaATest(struct V4C__Context *,struct V4IM__BaA *,struct V4DPI__Point *,int,int,int) ;
void v4im_BaAIncrement(struct V4C__Context *,struct V4IM__BaA *,struct V4DPI__Point *,int,int) ;

struct V4DPI__Point *v4im_ThrowException(struct V4C__Context *,struct V4DPI__Point *,int,UCCHAR *,int,struct V4DPI__Point *[]) ;
struct V4DPI__Point *v4im_ThrowLastErrorMnemonic(struct V4C__Context *,struct V4DPI__Point *,int) ;

struct V4DPI__Point *v4imu_EchoTable(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int,TAGVAL) ;
int v4im_DoCoerce(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__DimInfo *,struct V4DPI__Point *,struct V4DPI__Point *) ;
void v4im_CoerceIsctToList(struct V4C__Context *,struct V4L__ListPoint *,struct V4DPI__Point *) ;

struct V4DPI__Point *v4imu_NestToModule(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int,struct V4DPI__Point *(),int) ;
UCCHAR *v4im_LastTagName() ;





struct V4DPI__Point *v4im_DoArray(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4imu_Bits(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int) ;
//struct V4DPI__Point *v4im_Cache(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *,int,int) ;
#ifdef WANTIMCACHE
struct V4DPI__Point *v4im_IMCache(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
#endif
struct V4DPI__Point *v4im_DoCounter(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoDo(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int) ;
ETYPE v4imu_XMLSTART(struct V4C__Context *,struct V4IM__XMLNest *,int,struct V4DPI__Point *,LOGICAL,INTMODX,UCCHAR *) ;
LOGICAL v4imu_XMLEND(struct V4C__Context *,struct V4IM__XMLNest *,INDEX,INTMODX,UCCHAR *) ;
struct V4DPI__Point *v4im_DoDrawer(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int) ;
struct V4IM__Drawer *v4im_GetDrawerPtr(int,int) ;
struct V4IM__Drawer *v4im_ReallocDrawer(struct V4IM__Drawer *,LENMAX) ;
struct V4DPI__Point *v4im_DoDTInfo(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int) ;
int v4imu_CompareTime(struct V4C__Context *,int,struct V4DPI__Point *,struct V4DPI__Point *,LOGICAL *) ;
struct V4DPI__Point *v4im_DoEchoS(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoEnum(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int,int) ;
LOGICAL v4im_DoEnum_CheckTags(struct V4DPI__Point *,struct V4DPI__Point *,struct V4DPI__Point *,int) ;
struct V4DPI__Point *v4im_DoEnumCL(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int,struct V4L__ListLazyInfo *,LOGICAL *) ;
struct V4DPI__Point *v4im_DoError(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoFail(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoDim(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoMessage(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
void v4im_MessageSendFlushQueue() ;
LOGICAL v4im_MessageSendQueue(struct V4C__Context *,int,UCCHAR *,int,int,struct V4DPI__Point *,UCCHAR *,LOGICAL,UCCHAR *,int,UCCHAR *,CALENDAR,int) ;
LOGICAL v4im_MessageSend(struct V4C__Context *,int,UCCHAR *,int,int,struct V4DPI__Point *,UCCHAR *,LOGICAL,UCCHAR *,int,UCCHAR *,CALENDAR,int) ;
void v4im_MessageListen(struct V4Msg_Listener *) ;
void v4im_MessageListenMC(struct V4Msg_Listener *) ;
struct V4DPI__Point *v4im_MessageToPoint(struct V4C__Context *,struct V4DPI__Point *,struct V4Msg_Entry *,INTMODX) ;
UCCHAR *v4im_MessageNextMessageText() ;
struct V4DPI__Point *v4im_DoEval(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoUnicode(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoEvalBL(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoEvalCmd(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoEvalPt(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoFlatten(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoFormat(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int) ;
int v4im_FormatMask(struct V4C__Context *,int,struct V4DPI__Point *,UCCHAR *,UCCHAR *) ;
struct V4DPI__Point *v4im_DoGeo(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
LOGICAL v4im_DoIn(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *,int,int) ;
DIMID v4xdb_VDTYPEtoDim(ETYPE) ;
struct V4DPI__Point *v4im_DodbXct(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DodbGet(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoDbg(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_Dodb(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoList(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_ListSetHandler(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoLocale(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoLog(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoMakeI(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoMakeT(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
//void v4im_DoSummaryOutput(struct V4C__Context *, struct V4DPI__Point *,LOGICAL,UCCHAR *) ;
//struct V4DPI__Point *v4im_DoSummary(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoMakePm(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int) ;
struct V4DPI__Point *v4im_DoMakeP(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,LOGICAL *) ;
LOGICAL v4im_MakePTeleAccept(struct V4C__Context *,struct V4DPI__DimInfo *,struct V4DPI__Point *,UCCHAR *,struct V4DPI__Point *,struct V4DPI__Point *) ;
struct V4DPI__Point *v4im_DoNames(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
UCCHAR *v4im_NameCompRemoveCode(UCCHAR *,LOGICAL) ;
void v4im_NameToComponents(LOGICAL,UCCHAR *,int *,UCCHAR *,UCCHAR *,UCCHAR *,UCCHAR *,UCCHAR *,UCCHAR *) ;
int v4im_RawNameToComponents(struct V4C__Context *,UCCHAR *,int,int,UCCHAR *,UCCHAR *,UCCHAR *,UCCHAR *,UCCHAR *,UCCHAR *) ;
struct V4DPI__Point *v4im_DoNum(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoOptimize(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
LOGICAL v4im_DoOptimizeIter(struct V4C__Context *,struct V__Optimize *,int) ;
struct V4DPI__Point *v4im_DoParse(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
int v4im_ParseFlagsParse(LOGICAL *,struct V4C__Context *,struct V4DPI__Point *,int,int,int) ;
P *v4im_CoerceStringToPoint(struct V4C__Context *,P *,INTMODX,UCCHAR *,struct V4DPI__DimInfo *) ;
struct V4DPI__Point *v4im_DoProject(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,LOGICAL *) ;
LOGICAL v4imu_ProjectValidate(struct V4C__Context *,int,int,int) ;
struct V4DPI__Point *v4im_DoRCV(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,DIMID) ;

struct V4DPI__Point *v4im_DoSet(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoSort(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoSpawn(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoStr(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoNGram(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_StrBreak(struct V4C__Context *,struct V4DPI__Point *,int,UCCHAR *,struct vstr__StringBreak *,int) ;
struct V4DPI__Point *v4im_DoTally(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int,struct V4DPI__Point *) ;
struct V4DPI__Point *v4im_DoTallyM(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int,struct V4DPI__Point *) ;
struct V4DPI__Point *v4im_DoThrow(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoTransform(struct V4C__Context *,struct V4DPI__Point *,int,int,struct V4DPI__Point *[]) ;
struct V4DPI__Point *v4im_DoTrig(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoTry(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoUOM(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoV4(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
void v4_UniqueDictPoint(struct V4C__Context *,struct V4DPI__Point *) ;
void v4thread_WatchDog(void *) ;
UCCHAR *v4rpt_AllocUC(struct V4RPT__RptInfo *,LENMAX,int) ;
LOGICAL v4rpt_ParseURLSpec(struct V4C__Context *,struct V4RPT__URLSpec *,struct V4DPI__Point *,INTMODX) ;
void *v4rpt_AllocChunk(struct V4RPT__RptInfo *,LENMAX) ;
LOGICAL v4im_RptUpdVFS(struct V4C__Context *,struct V4SS__FormatSpec *,struct V4DPI__Point *) ;
LOGICAL v4im_RptContext(struct V4C__Context *,struct V4RPT__RptInfo *,INDEX,INDEX,struct V4DPI__Point *,struct V4IM__BaA *) ;
LOGICAL v4im_RptColList(struct V4C__Context *,struct V4L__ListPoint *,struct V4RPT__RptInfo *,struct V4IM__BaA *,INTMODX,LOGICAL,INDEX *) ;
void v4rpt_XMLHTMLInit(struct V4C__Context *,struct V4RPT__RptInfo *,LOGICAL *,INDEX) ;
void v4rpt_vfsToCSS(struct V4C__Context *,struct V4SS__FormatSpec *,UCCHAR *) ;
struct V4DPI__Point *v4im_DoRpt(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoJSON(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoJSONRef(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,LOGICAL) ;
LOGICAL v4im_JSONValString(struct V4C__Context *,struct V4DPI__Point *,UCCHAR **,LENMAX *,INTMODX,INDEX,INDEX,LOGICAL) ;
struct V4DPI__Point *v4im_DoTable(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
LOGICAL v4im_vltDoTable(struct V4C__Context *,struct V4LEX__Table *) ;
LOGICAL v4im_vltcolDoTable(struct V4C__Context *,struct V4LEX__Table *,INDEX) ;
LOGICAL v4im_vltEndTable(struct V4C__Context *,struct V4LEX__Table *,LOGICAL) ;
LOGICAL v4im_vltParseOther(struct V4C__Context *,struct V4LEX__Table *,struct V4LEX__TknCtrlBlk *) ;
struct V4DPI__Point *v4im_DoEvalAE(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int,struct V4DPI__Point *) ;
struct V4DPI__Point *v4im_DoEvalLE(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int,struct V4DPI__Point *) ;
#ifdef V4_BUILD_SECURITY
struct V4DPI__Point *v4im_DoSecure(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int,struct V4DPI__Point *) ;
struct V4DPI__Point *v4im_DoArea(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int,struct V4DPI__Point *) ;
LOGICAL v4im_AreaResetExt(struct V4C__Context *,UCCHAR *) ;
LOGICAL v4im_AreaRebuild(struct V4C__Context *,UCCHAR *,UCCHAR *) ;
LOGICAL v4im_AreaBuildIndex(struct V4C__Context *,UCCHAR *) ;
LOGICAL v4im_AreaOpen(struct V4C__Context *,UCCHAR *,struct V4DPI__LittlePoint *,ETYPE,INDEX,enum DictionaryEntries,LOGICAL,LOGICAL,struct V4DPI__Point *) ;
struct V4DPI__Point *v4im_DoMakeIn(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int) ;
struct V4DPI__Point *v4im_DoMacro(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int,struct V4DPI__Point *) ;
#endif

/*	V 4 I S			*/

LOGICAL v4is_Put(struct V4IS__ParControlBlk *,UCCHAR *) ;
int v4is_PositionKey(int,struct V4IS__Key *,struct V4IS__Key **,int,int) ;
LOGICAL v4is_PositionRel(int,int,int,struct V4IS__Key **) ;
int v4is_PositionToParentNode(int,struct V4IS__Key *,int,int,int) ;
int v4is_SearchBucket(struct V4IS__Bkt *,struct V4IS__Key *) ;
LOGICAL v4is_Get(struct V4IS__ParControlBlk *) ;
LOGICAL v4is_FillDataIdList(int,struct V4IS__AreaCB *,struct V4IS__DataIdList *) ;
struct V4IS__DataBktEntryHdr *v4is_Getdbeh(int,int,int) ;
void v4is_ErrorCleanup(struct V4IS__ParControlBlk *) ;
int v4is_GetData(int,void *,int,int) ;
int v4is_Insert(int,struct V4IS__Key *,void *,int,int,int,int,LOGICAL,LOGICAL,int,UCCHAR *) ;
void v4is_Replace(int,struct V4IS__Key *,void *,void *,int,int,int,int,int) ;
int v4is_StashData(int,void *,int,int,int,int,int,int) ;
int v4is_DataBktFree(int,struct V4IS__AreaCB *,struct V4IS__DataBktHdr *,LOGICAL) ;
int v4is_DataBktAvail(int ,struct V4IS__AreaCB *,int,int) ;
LOGICAL v4is_CopyData(int,void *,int,void *,int,int,struct V4IS__ParControlBlk *,int) ;
int v4is_RemoveData(int,struct V4IS__Bkt *,int,int) ;
LOGICAL v4is_StashIndexKey(int,void *,LOGICAL,int) ;
int v4is_IndexSplit(int,void *,int,int *,int) ;
void v4is_IndexNewParent(struct V4IS__Bkt *,int) ;
void v4is_IndexConsolidate(struct V4IS__Bkt *,int) ;
LOGICAL v4is_IndexStashLink(int,int,struct V4IS__Key *,int,char *,int *) ;
LOGICAL v4is_ReplaceDataId(int,struct V4IS__Key *,int,int) ;
void v4is_RemoveIndexEntry(struct V4IS__ParControlBlk *,int,int,int,struct V4IS__Key *) ;
int v4is_FFKeyCount(struct V4IS__AreaCB *,int) ;
LOGICAL v4is_FFKeyDups(struct V4IS__AreaCB *,int,int) ;
struct V4IS__Key *v4is_MakeFFKey(struct V4IS__AreaCB *,int,void *,int,void *,void *,LOGICAL) ;
int v4is_BytesInFFKey(struct V4IS__AreaCB *,int,int) ;


/*	V 4 I S U		*/


LOGICAL v4is_Open(struct V4IS__ParControlBlk *,struct V4IS__OptControlBlk *,UCCHAR *) ;
void v4is_Close(struct V4IS__ParControlBlk *cb) ;
void v4is_Delete(int,struct V4IS__ParControlBlk *,int) ;
void v4is_ObsoleteData(int,int) ;
void v4is_Append(int,struct V4IS__Key *,void *,int,int,int,LOGICAL,int) ;
int v4is_IndexStashAppendLink(int,int,struct V4IS__Key *) ;
struct V4IS__Bkt *v4is_MakeBkt(int,int) ;
LOGICAL v4is_StashIndexData(int,struct V4IS__Key *,void *,int,int,LOGICAL,int) ;
int v4is_Reformat(struct V4IS__ParControlBlk *,struct V4IS__ParControlBlk *,struct V4IS__RefResults *,LOGICAL,int) ;
int v4is_DumpArea(FILE *,FILE *,int,int,int,LOGICAL,LOGICAL,LOGICAL,int) ;
int v4is_RestoreArea(struct V4IS__SeqFileList *,UCCHAR *,LOGICAL,int,LOGICAL,int,int,LOGICAL,int) ;
int v4is_RebuildAreaIndex(struct V4IS__ParControlBlk *) ;
char *v4is_FormatKey(struct V4IS__Key *) ;
void v4is_BugChk(int,char *) ;
void v4is_SearchForLink(int,int) ;
void v4is_ExamineBkts(int,int) ;
void v4is_ExamineBkt(int,int) ;
int v4is_RemoteFileRef(int,char *,int,int) ;
int v4is_ConnectFileRef(int,struct V4IS__AreaCB *) ;
LOGICAL v4is_DisconnectAreaId(int,int) ;










/*	v 4 m m			*/

struct V4MM__MemMgmtMaster *v4mm_MemMgmtInit(struct V4MM__MemMgmtMaster *) ;
void v4mm_MemMgmtShutdown(struct V4MM__MemMgmtMaster *) ;
LOGICAL v4mm_copyToNewMMM(struct V4MM__MemMgmtMaster *,struct V4MM__MemMgmtMaster *,AREAID,LOGICAL) ;
struct V4MM__MemMgmtMaster *v4mm_GetMMMPtr() ;
struct V4IS__AreaCB *v4mm_ACBPtr(int) ;
struct V4IS__AreaCB *v4mm_ACBPtrRE(int) ;
int v4mm_SetUserJobId(int,struct V4MM__MemMgmtMaster *) ;
void v4mm_spurious_alarm_handler() ;
#ifdef WINNT
int v4mm_MakeNewAId(struct V4IS__AreaCB *,int,HANDLE,UCCHAR *) ;
#else
int v4mm_MakeNewAId(struct V4IS__AreaCB *,int,FILE *,UCCHAR *) ;
#endif
SEGID v4mm_AreaSegId(int) ;
void v4mm_AreaCheckPoint(int) ;
void v4mm_AreaFlush(int,LOGICAL) ;
void v4mm_ZapBucket(int,int) ;
LOGICAL v4mm_BktFlush(int,int) ;
int v4mm_MakeDataId(struct V4IS__Bkt *) ;
int v4mm_MakeBId(int,int) ;
struct V4IS__Bkt *v4mm_NewBktPtr(int,int,int,int) ;
struct V4IS__Bkt *v4mm_BktPtr(int,int,int) ;
int v4mm_GlobalBufGrab(int,int) ;
void v4mm_AreaFlushOld(int) ;
void v4mm_TempLock(int,int,int) ;
int v4mm_BktUpdated(int,int) ;
void v4mm_BktRead(int,int, void *) ;
void v4mm_BktWrite(int,int, void *,char *,int) ;
int v4mm_LockSomething(int,int,int,int,int,int *,int) ;
void v4mm_LockRelease(int,int,PID,int,char *) ;
void v4mm_OpenCloseLock(LOGICAL) ;
void v4mm_ShareSegInit(int,UCCHAR *,int,int,int,int,int) ;

#ifdef WINNT
LOGICAL v4mm_WinNTProcessGone(PID,struct V4MM__LockTable *) ;
#endif

LOGICAL v4mm_GrabLockTable(int *) ;

/*	V S O R T		*/

LOGICAL vsort_entry(UCCHAR *,void *,int,UCCHAR *) ;
int vsrt_KeyCompare(const void *,const void *) ;
int vsrt_KeyCmpTally(const void *,const void *) ;
void vsort_MakeInternalKey(struct VSRT__ControlBlock *,void *,void *) ;




/*	V 4 S T A T		*/

void nrerror(char *) ;
void avevar(double [],unsigned long,double *,double *) ;
double betacf(double,double,double) ;
double betai(double,double,double) ;
void chsone( double [],  double [],  int ,  int ,  double *, double *, double *) ;
double v4errff(double) ;
double erffc(double) ;
void ftest( double [],unsigned long,double [],unsigned long,double *,double *) ;
void fit(double [],double [],int,double [],  int, double *,double *,double *,double *,double *,double *) ;
double gammln(double) ;
double gammp(double,double) ;
double gammq(double,double) ;
void gcf(double *,double,double,double *) ;
void gser(double *,double,double,double *) ;
void medfit(double [],double [],int,double *,double *,double *) ;
double rofunc(double) ;
double nrselect(unsigned long,unsigned long,double []) ;
void ttest(double [], unsigned long,double [],unsigned long,double *,double *) ;
void tptest(double [],double [],unsigned long,double *,double *) ;
void tutest(double [],unsigned long, double [],unsigned long,double *,double *) ;
double *vector(long,long) ;
int *ivector(long,long) ;
unsigned char *cvector(long,long) ;
unsigned long *lvector(long,long) ;
double *dvector(long,long) ;
double **matrix(long,long,long,long) ;
double **dmatrix(long,long,long,long) ;
int **imatrix(long,long,long,long) ;
double **submatrix(double **,long,long,long,long,long,long) ;
double **convert_matrix(double *,long,long,long,long) ;
double ***f3tensor(long,long,long,long,long,long) ;
void free_vector(double *,long,long) ;
void free_ivector(int *,long,long) ;
void free_lvector(unsigned long *,long,long) ;
void free_dvector(double *,long,long) ;
void free_matrix(double **,long,long,long,long) ;
void free_dmatrix(double **,long,long,long,long) ;
void free_imatrix(double **,long,long,long,long) ;
void free_submatrix(double **,long,long,long,long) ;
void free_convert_matrix(double **,long,long,long,long) ;
void free_f3tensor(double ***,long,long,long,long,long,long) ;
void v4stat_RanSeed(int,UB64INT) ;
double v4stat_ran0() ;
double v4stat_ran1() ;
double v4stat_ran2() ;
double v4stat_StatAdjCosSim(int,double [],int,double [],int,double [],LOGICAL *) ;
double v4stat_AvgDev(int,double [],LOGICAL *) ;
double v4stat_Avg(int,double [],LOGICAL *) ;
double v4stat_BetaDist(double,double,double,double,double,int) ;
double v4stat_BetaInv(double,double,double,double,double,LOGICAL *) ;
double v4stat_BinomDist(double,double,double,LOGICAL,LOGICAL *) ;
double v4stat_ChiDist(double,double) ;
double v4stat_ChiInv(double,double,LOGICAL *) ;
double v4stat_ChiTest(double,int,double [],int,double []) ;
double v4stat_Confidence(double,double,double,LOGICAL *) ;
double v4stat_Correl(int,double [],int,double [],LOGICAL *) ;
double v4stat_Covar(int,double [],int,double [],LOGICAL *) ;
double v4stat_CritBinom(double,double,double,LOGICAL *) ;
double v4stat_DevSq(int,double [],LOGICAL *) ;
double v4stat_ErrF(double) ;
double v4stat_ErrFC(double) ;
double v4stat_ExponDist(double,double,LOGICAL) ;
double v4stat_FDist(double,double,double) ;
double v4stat_FInv(double,double,double,LOGICAL *) ;
double v4stat_Forecast(double,int,double [],int,double []) ;
double v4stat_Frequency(double,int,double []) ;
double v4stat_FTest(int,double [],int,double [],LOGICAL *) ;
double v4stat_Fisher(double) ;
double v4stat_FisherInv(double) ;
double v4stat_GammaDist(double,double,double,LOGICAL) ;
double v4stat_GammaInv(double,double,double,LOGICAL *) ;
double v4stat_Gamma(double) ;
double v4stat_GammaLn(double) ;
double v4stat_GeoMean(int,double [],LOGICAL *) ;
double v4stat_HarMean(int,double [],LOGICAL *) ;
double v4stat_Kurtosis(int,double [],LOGICAL *) ;
void v4stat_LinFit(struct V4IM__StatLinFit *,LOGICAL *) ;
double v4stat_LogInv(double,double,double,LOGICAL *) ;
double v4stat_LogNormDist(double,double,double) ;
double v4stat_HyperGeo(double,double,double,double,LOGICAL *) ;
double v4stat_Max(int,double [],LOGICAL *) ;
int v4stat_DoubleCompare(const void *,const void *) ;
double v4stat_Median(int,double [],LOGICAL *) ;
double v4stat_Min(int,double [],LOGICAL *) ;
double v4stat_Mode(int,double [],LOGICAL *) ;
//double v4stat_NCombR(int,int,LOGICAL *) ;
double v4stat_logNCombR(int,int,LOGICAL *) ;
double v4stat_NegBinomDist(double,double,double,LOGICAL *) ;
double v4stat_NormDist(double,double,double,LOGICAL) ;
double v4stat_NormInv(double,double,double,LOGICAL *) ;
double v4stat_NormStdDist(double) ;
double v4stat_NormStdInv(double,LOGICAL *) ;
double v4stat_Pearson(int,double [],int,double [],LOGICAL *) ;
double v4stat_Percentile(double,int,double []) ;
double v4stat_PercentRank(double,int,double []) ;
double v4stat_Permute(double,double) ;
double v4stat_Poisson(double,double,LOGICAL) ;
double v4stat_PoissonInv(double,double) ;
double v4stat_Prob() ;
double v4stat_Product(int,double [],LOGICAL *) ;
double v4stat_Quartile(int,int,double [],LOGICAL *) ;
struct V4DPI__Point *v4stat_DoStatRan(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int) ;
double v4stat_Rank(int,double [],int,int) ;
double v4stat_RSQ(int,double [],int,double [],LOGICAL *) ;
struct V4DPI__Point *v4im_DoStatScale(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
double v4stat_Skew(int,double [],LOGICAL *) ;
double v4stat_Slope(int,double [],int,double [],LOGICAL *) ;
double v4stat_Standardize(double,double,double) ;
double v4stat_StdDev(int,double [],LOGICAL *) ;
double v4stat_StdDevP(int,double [],LOGICAL *) ;
double v4stat_StdErrYX(int,double [],int,double [],LOGICAL *) ;
double v4stat_SumSq(int,double [],LOGICAL *) ;
double v4stat_SumX2mY2(int,double [],int,double [],LOGICAL *) ;
double v4stat_SumX2pY2(int,double [],int,double [],LOGICAL *) ;
double v4stat_SumXmY2(int,double [],int,double [],LOGICAL *) ;
double v4stat_TDist(double,double,int) ;
double v4stat_TInv(double,double,LOGICAL *) ;
int v4stat_Trend() ;
double v4stat_TrimMean(int,double [],double,LOGICAL *) ;
double v4stat_TTest(int,double [],int,double [],int,int) ;
double v4stat_Var(int,double [],LOGICAL *) ;
double v4stat_VarP(int,double [],LOGICAL *) ;
double v4stat_Weibull(double,double,double,LOGICAL) ;
double v4stat_ZTest(int,double [],double,double,LOGICAL *) ;
struct V4DPI__Point *v4stat_Arg1List(struct V4C__Context *,struct V4DPI__Point *,double(int,double[],LOGICAL *),int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4stat_Arg2List(struct V4C__Context *,struct V4DPI__Point *,double(int,double[],int,double [],LOGICAL *),int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4stat_Arg3List(struct V4C__Context *,struct V4DPI__Point *,double(int,double[],int,double [],int,double [],LOGICAL *),int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4stat_Dist(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4stat_DistInv(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4stat_Fit(struct V4C__Context *,struct V4DPI__Point *,int,struct V4DPI__Point *[],int,int) ;
struct V4DPI__Point *v4im_DoDMCluster(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;
struct V4DPI__Point *v4im_DoReAllocate(struct V4C__Context *,struct V4DPI__Point *,struct V4DPI__Point *[],int,int,int) ;


/*	V 4 R E G E X P  */

int vregexp_RegComp (regex_t *,char *,int) ;
size_t vregexp_Error (int,regex_t *,UCCHAR *,size_t) ;
void vregexp_Free(regex_t *) ;
int vregexp_RegExec (regex_t *,char *,size_t,regmatch_t[],int) ;
int vregexp_Search2 (struct re_pattern_buffer *,char *,int,char *,int,int,int,struct re_registers *,int) ;



/*	v 4 t e s t		*/

void ctrlp() ;
void v4_ExitStats(struct V4C__Context *,struct V4LEX__TknCtrlBlk *) ;
#ifdef ALPHAOSF
void v_signal_bus(int,int,struct sigcontext *) ;
void v_signal_fpe(int,int,struct sigcontext *) ;
void v_signal_ill(int,int,struct sigcontext *) ;
void v_signal_fpe(int,int,struct sigcontext *) ;
#else
void v_signal_bus(int) ;
void v_signal_fpe(int) ;
void v_signal_ill(int) ;
#endif

void v4_error(int,struct V4LEX__TknCtrlBlk *,char *,char *,char *,char *,...) ;
void v4_UCerror(int,struct V4LEX__TknCtrlBlk *,char *,char *,char *,UCCHAR *) ;
