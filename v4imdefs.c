/*	V4IMDEFS.C - Locals definitions for V4IM & Friends

	Created 11-Feb-97 by Victor E. Hansen			*/

#ifndef NULLID
#include "v4defs.c"
#endif
#include <setjmp.h>
#include <float.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#define INITTCB if (tcb == NULL) tcb = (struct V4LEX__TknCtrlBlk *)v4mm_AllocChunk(sizeof *tcb,FALSE) ; v4lex_InitTCB(tcb,V4LEX_TCBINIT_NoStdIn) ;
#define GA(DST,SRC) \
  { if (SRC->Value.AlphaVal[0] <= V4PT_MaxCharLenIn0) \
     { strncpy(DST,&SRC->Value.AlphaVal[1],SRC->Value.AlphaVal[0]) ; DST[SRC->Value.AlphaVal[0]] = 0 ; } \
     else { strcpy(DST,&SRC->Value.AlphaVal[1]) ; } ; }

#define V4IM_DblArrayMaxInit 1000	/* Initial Max when allocating below */
#define V4IM_DblArrayIncFactor 0.5	/* Factor to use for incrementing array */
#define V4IM_DblArrayIncMax 50000	/*   until reach this size */

struct V4IM_DblArray			/* Temp structure to hold cash payments for n periods */
{ int Max ;				/* Maximum number in array below */
  int Count ;				/* Number of periods */
  double Pay[1] ;			/* This actually is dynamic */
} ;


#define PI 3.1415926535897932384626433832795028841971693993751058209749445
#define Enatural  2.71828182845904523536028747135266249775724709369995

#define V_MAX_LITERAL_DECIMALS 15
#define V_MAX_ILITERAL_DECIMALS 9
static double powers[] = { 1.0, 10.0, 100.0, 1000.0, 10000.0, 100000.0, 1000000.0, 10000000.0, 100000000.0, 1000000000.0, 10000000000.0, 100000000000.0, 1000000000000.0, 10000000000000.0, 100000000000000.0, 1000000000000000.0 } ;
static double vRound[] = { 0.5, 0.05, 0.005, .0005, .00005, 0.000005, 0.0000005, 0.00000005, 0.000000005, 0.0000000005, 0.00000000005, 0.000000000005, 0.0000000000005, 0.00000000000005, 0.000000000000005, 0.0000000000000005 } ;
static int ipowers[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 } ;
static int iround[] = { 0, 5, 50, 500, 5000, 50000, 500000, 5000000, 50000000, 500000000 } ;

#define SCALEFIX(FIX,FIXNUM,NEWNUM)	/* Scales fixed point FIX from FIXNUM to NEWNUM implied decimal places */ \
 if (FIXNUM != NEWNUM) \
  { if (FIXNUM < NEWNUM) { FIX *= ipowers[NEWNUM-FIXNUM] ; } \
     else { FIX = (FIX + (FIX < 0 ? -iround[FIXNUM-NEWNUM] : iround[FIXNUM-NEWNUM])) / ipowers[FIXNUM-NEWNUM] ; } ; \
  } ;
#define FIXNORMALIZE(LI1,LI2,PT1,PT2)	/* Updates LI1 & LI2 with fixed values from points PT1 & PT2 & normalizes values */ \
 memcpy(&LI1,&PT1->Value.FixVal,sizeof(B64INT)) ; memcpy(&LI2,&PT2->Value.FixVal,sizeof(B64INT)) ; \
 if (PT1->LHSCnt > PT2->LHSCnt) { LI2 *= ipowers[PT1->LHSCnt - PT2->LHSCnt] ; } \
  else if (PT2->LHSCnt > PT1->LHSCnt) { LI1 *= ipowers[PT2->LHSCnt - PT1->LHSCnt] ; } ;


#define V4IM_Tag_Unk 0		/* Point is of unknown type */
#define V4IM_Tag_If 1		/* Point is If:xxx */
#define V4IM_Tag_While 2
#define V4IM_Tag_Hold 3
#define V4IM_Tag_Context 4
#define V4IM_Tag_Dim 5
#define V4IM_Tag_By 6
#define V4IM_Tag_Reverse 7
#define V4IM_Tag_Index 8
#define V4IM_Tag_Select 9
#define V4IM_Tag_Union 10
#define V4IM_Tag_XUnion 11
#define V4IM_Tag_Subtract 12
#define V4IM_Tag_Intersect 13
#define V4IM_Tag_Head 14
#define V4IM_Tag_Tail 15
#define V4IM_Tag_Nth 16
#define V4IM_Tag_Position 17
#define V4IM_Tag_Sample 18
#define V4IM_Tag_Append 19
#define V4IM_Tag_Remove 20
#define V4IM_Tag_Product 21
#define V4IM_Tag_To 22
#define V4IM_Tag_Number 23
#define V4IM_Tag_Font 24
#define V4IM_Tag_StyleName 25
#define V4IM_Tag_Style 26
#define V4IM_Tag_Format 27
#define V4IM_Tag_FontSize 28
#define V4IM_Tag_Special 29
#define V4IM_Tag_QIsct 30		/* Point is a quoted isct */
#define V4IM_Tag_List 31		/* Point is a list */
#define V4IM_Tag_Action 32
#define V4IM_Tag_Rows 33
#define V4IM_Tag_Columns 34
#define V4IM_Tag_Values 35
#define V4IM_Tag_RowCap 36
#define V4IM_Tag_ColCap 37
#define V4IM_Tag_Label 38
#define V4IM_Tag_Reset 39
#define V4IM_Tag_Acceptor 40
#define V4IM_Tag_Displayer 41
#define V4IM_Tag_Default 42
#define V4IM_Tag_Unique 43
#define V4IM_Tag_CoMingle 44
#define V4IM_Tag_ColDim 45
#define V4IM_Tag_Name 46
#define V4IM_Tag_ColumnId 47
#define V4IM_Tag_Row 48
#define V4IM_Tag_Column 49
#define V4IM_Tag_Sum 50
#define V4IM_Tag_Sub 51
#define V4IM_Tag_Minimum 52
#define V4IM_Tag_Maximum 53
#define V4IM_Tag_StdDev 54
#define V4IM_Tag_Average 55
#define V4IM_Tag_ListOf 56
#define V4IM_Tag_Count 57
#define V4IM_Tag_Bind 58
#define V4IM_Tag_ByList 59
#define V4IM_Tag_Table 60
#define V4IM_Tag_IfOnce 61
#define V4IM_Tag_Do 62
#define V4IM_Tag_Skip 63
#define V4IM_Tag_Page 64
#define V4IM_Tag_NE 65
#define V4IM_Tag_LT 66
#define V4IM_Tag_LE 67
#define V4IM_Tag_EQ 68
#define V4IM_Tag_GE 69
#define V4IM_Tag_GT 70
#define V4IM_Tag_Y 71
#define V4IM_Tag_X 72
#define V4IM_Tag_Slope 73
#define V4IM_Tag_Intercept 74
#define V4IM_Tag_ChiSq 75
#define V4IM_Tag_Enum 76
#define V4IM_Tag_Next 77
#define V4IM_Tag_At 78
#define V4IM_Tag_Method 79
#define V4IM_Tag_AbsDev 80
#define V4IM_Tag_DSN 81
#define V4IM_Tag_User 82
#define V4IM_Tag_Password 83
#define V4IM_Tag_DBC 84
#define V4IM_Tag_SQL 85
#define V4IM_Tag_Statement 86
#define V4IM_Tag_Connection 87
#define V4IM_Tag_Trace 88
#define V4IM_Tag_Type 89
#define V4IM_Tag_Length 90
#define V4IM_Tag_Precision 91
#define V4IM_Tag_Scale 92
#define V4IM_Tag_Agg 93
#define V4IM_Tag_FAgg 94
#define V4IM_Tag_BFAgg 95
#define V4IM_Tag_SetOf 96
#define V4IM_Tag_FV 97
#define V4IM_Tag_PV 98
#define V4IM_Tag_Pmt 99
#define V4IM_Tag_Rate 100
#define V4IM_Tag_Periods 101
#define V4IM_Tag_IPmt 102
#define V4IM_Tag_PPmt 103
#define V4IM_Tag_SPeriod 104
#define V4IM_Tag_EPeriod 105
#define V4IM_Tag_Settlement 106
#define V4IM_Tag_Maturity 107
#define V4IM_Tag_Frequency 108
#define V4IM_Tag_Basis 109
#define V4IM_Tag_DaysBS 110
#define V4IM_Tag_DaysS 111
#define V4IM_Tag_DaysNC 112
#define V4IM_Tag_DateAS 113
#define V4IM_Tag_DateBS 114
#define V4IM_Tag_NumSM 115
#define V4IM_Tag_Price 116
#define V4IM_Tag_Redemption 117
#define V4IM_Tag_Macaulay 118
#define V4IM_Tag_Modified 119
#define V4IM_Tag_Coupon 120
#define V4IM_Tag_Yield 121
#define V4IM_Tag_First 122
#define V4IM_Tag_Periodic 123
#define V4IM_Tag_Investment 124
#define V4IM_Tag_Issue 125
#define V4IM_Tag_Par 126
#define V4IM_Tag_OddFirst 127
#define V4IM_Tag_OddLast 128
#define V4IM_Tag_BetaPDF 129
#define V4IM_Tag_Binomial 130
#define V4IM_Tag_Last 131
#define V4IM_Tag_Exponential 132
#define V4IM_Tag_F 133
#define V4IM_Tag_Gamma 134
#define V4IM_Tag_HyperGeo 135
#define V4IM_Tag_LogNorm 136
#define V4IM_Tag_NegBinom 137
#define V4IM_Tag_Normal 138
#define V4IM_Tag_NormalStd 139
#define V4IM_Tag_Poisson 140
#define V4IM_Tag_StudentT 141
#define V4IM_Tag_Weibull 142
#define V4IM_Tag_Alpha 143
#define V4IM_Tag_Beta 144
#define V4IM_Tag_Cumulative 145
#define V4IM_Tag_DoF 146
#define V4IM_Tag_Lambda 147
#define V4IM_Tag_Mean 148
#define V4IM_Tag_Num 149
#define V4IM_Tag_NumF 150
#define V4IM_Tag_NumP 151
#define V4IM_Tag_NumS 152
#define V4IM_Tag_ProbS 153
#define V4IM_Tag_Samples 154
#define V4IM_Tag_Trials 155
#define V4IM_Tag_Prob 156
#define V4IM_Tag_Z 157
#define V4IM_Tag_Discount 158
#define V4IM_Tag_A 159
#define V4IM_Tag_B 160
#define V4IM_Tag_Day 161
#define V4IM_Tag_DayOfWeek 162
#define V4IM_Tag_UDate 163
#define V4IM_Tag_UMonth 164
#define V4IM_Tag_UTime 165
#define V4IM_Tag_UYear 166
#define V4IM_Tag_UWeek 167
#define V4IM_Tag_UQuarter 168
#define V4IM_Tag_BaseDate 169
#define V4IM_Tag_Month 170
#define V4IM_Tag_DayOfYear 171
#define V4IM_Tag_UPeriod 172
#define V4IM_Tag_Cost 173
#define V4IM_Tag_Factor 174
#define V4IM_Tag_Life 175
#define V4IM_Tag_Partial 176
#define V4IM_Tag_Period 177
#define V4IM_Tag_Salvage 178
#define V4IM_Tag_DDB 179
#define V4IM_Tag_FDB 180
#define V4IM_Tag_SLN 181
#define V4IM_Tag_SYD 182
#define V4IM_Tag_Compounds 183
#define V4IM_Tag_Effective 184
#define V4IM_Tag_Inflation 185
#define V4IM_Tag_Nominal 186
#define V4IM_Tag_TaxRate 187
#define V4IM_Tag_BaseMonth 188
#define V4IM_Tag_Year 189
#define V4IM_Tag_Echo 190
#define V4IM_Tag_Parent 191
#define V4IM_Tag_PIf 192
#define V4IM_Tag_And 193
#define V4IM_Tag_Or 194
#define V4IM_Tag_Not 195
#define V4IM_Tag_AndC 196
#define V4IM_Tag_XOR 197
#define V4IM_Tag_Width 198
#define V4IM_Tag_Round 199
#define V4IM_Tag_Point 200
#define V4IM_Tag_Before 201
#define V4IM_Tag_After 202
#define V4IM_Tag_Begin 203
#define V4IM_Tag_End 204
#define V4IM_Tag_Area 205
#define V4IM_Tag_Open 206
#define V4IM_Tag_Share 207
#define V4IM_Tag_KeyNum 208
#define V4IM_Tag_Lock 209
#define V4IM_Tag_Key 210
#define V4IM_Tag_Put 211
#define V4IM_Tag_Get 212
#define V4IM_Tag_Field 213
#define V4IM_Tag_With 214
#define V4IM_Tag_FileRef 215
#define V4IM_Tag_Calc 216
#define V4IM_Tag_Minute 217
#define V4IM_Tag_Menu1 218
#define V4IM_Tag_Menu2 219
#define V4IM_Tag_Evaluate 220
#define V4IM_Tag_Error 221
#define V4IM_Tag_Shell 222
#define V4IM_Tag_Collapse 223
#define V4IM_Tag_ByIf 224
#define V4IM_Tag_Header 225
#define V4IM_Tag_Footer 226
#define V4IM_Tag_LMargin 227
#define V4IM_Tag_RMargin 228
#define V4IM_Tag_TMargin 229
#define V4IM_Tag_BMargin 230
#define V4IM_Tag_Portrait 231
#define V4IM_Tag_Hour 232
#define V4IM_Tag_AWhile 233
#define V4IM_Tag_Has 234
#define V4IM_Tag_Trim 235
#define V4IM_Tag_UC 236
#define V4IM_Tag_RowGap 237
#define V4IM_Tag_ColGap 238
#define V4IM_Tag_AutoFit 239
#define V4IM_Tag_WBind 240
#define V4IM_Tag_Seed 241
#define V4IM_Tag_Id 242
#define V4IM_Tag_MatchUndef 243
#define V4IM_Tag_StdIn 244
#define V4IM_Tag_Macro 245
#define V4IM_Tag_Cell 246
#define V4IM_Tag_Every 247
#define V4IM_Tag_Library 248
#define V4IM_Tag_Module 249
#define V4IM_Tag_Update 250
#define V4IM_Tag_All 251
#define V4IM_Tag_Sheet 252
#define V4IM_Tag_RowColor 253
#define V4IM_Tag_ColColor 254
#define V4IM_Tag_CellColor 255
#define V4IM_Tag_Include 256
#define V4IM_Tag_Object 257
#define V4IM_Tag_Note 258
#define V4IM_Tag_Message 259
#define V4IM_Tag_IFa 260
#define V4IM_Tag_IFb 261
#define V4IM_Tag_IFc 262
#define V4IM_Tag_Cache 263
#define V4IM_Tag_NoError 264
#define V4IM_Tag_Binding 265
#define V4IM_Tag_PageBreak 266
#define V4IM_Tag_Result 267
#define V4IM_Tag_Week 268
#define V4IM_Tag_Description 269
#define V4IM_Tag_Try 270
#define V4IM_Tag_UCount 271
#define V4IM_Tag_None 272
#define V4IM_Tag_Bits 273
#define V4IM_Tag_Sin 274
#define V4IM_Tag_Cos 275
#define V4IM_Tag_Tan 276
#define V4IM_Tag_Degrees 277
#define V4IM_Tag_Radians 278
#define V4IM_Tag_Dot 279
#define V4IM_Tag_Define 280
#define V4IM_Tag_Attributes 281
#define V4IM_Tag_XML 282
#define V4IM_Tag_Group 283
#define V4IM_Tag_Metric 284
#define V4IM_Tag_NestRCV 285
#define V4IM_Tag_Then 286
#define V4IM_Tag_Else 287
#define V4IM_Tag_ElseIf 288
#define V4IM_Tag_URL 289
#define V4IM_Tag_ConnectTime 290
#define V4IM_Tag_CPUTime 291
#define V4IM_Tag_Replace 292
#define V4IM_Tag_Mask 293
#define V4IM_Tag_Fail 294
#define V4IM_Tag_Optimize 295
#define V4IM_Tag_Until 296
#define V4IM_Tag_Value 297
#define V4IM_Tag_Fractional 298
#define V4IM_Tag_Integral 299
#define V4IM_Tag_Range 300
#define V4IM_Tag_RCV 301
#define V4IM_Tag_Sign 302
#define V4IM_Tag_Prime 303
#define V4IM_Tag_Negate 304
#define V4IM_Tag_Abs 305
#define V4IM_Tag_Left 306
#define V4IM_Tag_Right 307
#define V4IM_Tag_Center 308
#define V4IM_Tag_Log 309
#define V4IM_Tag_Log10 310
#define V4IM_Tag_Exp 311
#define V4IM_Tag_Exp10 312
#define V4IM_Tag_POW 313
#define V4IM_Tag_TextColor 314
#define V4IM_Tag_Exists 315
#define V4IM_Tag_Body 316
#define V4IM_Tag_Form 317
#define V4IM_Tag_Pop 318
#define V4IM_Tag_Push 319
#define V4IM_Tag_Sleep 320
#define V4IM_Tag_Break 321
#define V4IM_Tag_Exp2 322
#define V4IM_Tag_Log2 323
#define V4IM_Tag_Bytes 324
#define V4IM_Tag_Directory 325
#define V4IM_Tag_Path 326
#define V4IM_Tag_Extension 327
#define V4IM_Tag_Access 328
#define V4IM_Tag_FullPathName 329
#define V4IM_Tag_Source 330
#define V4IM_Tag_Close 331
#define V4IM_Tag_Distance 332
#define V4IM_Tag_Threshold 333
#define V4IM_Tag_Fill 334
#define V4IM_Tag_HTML 335
#define V4IM_Tag_RegExp 336
#define V4IM_Tag_Interval 337
#define V4IM_Tag_Target 338
#define V4IM_Tag_Overflow 339
#define V4IM_Tag_Pairs 340
#define V4IM_Tag_Edge 341
#define V4IM_Tag_OneWay 342
#define V4IM_Tag_Depth 343
#define V4IM_Tag_URLBase 344
#define V4IM_Tag_Create 345
#define V4IM_Tag_Grid 346
#define V4IM_Tag_Out 347
#define V4IM_Tag_Environment 348
#define V4IM_Tag_Nest 349
#define V4IM_Tag_DataId 350
#define V4IM_Tag_Title 351
#define V4IM_Tag_Icon 352
#define V4IM_Tag_Host 353
#define V4IM_Tag_Port 354
#define V4IM_Tag_Reply 355
#define V4IM_Tag_AutoHead 356
#define V4IM_Tag_Submit 357
#define V4IM_Tag_Hash32 358
#define V4IM_Tag_LC 359
#define V4IM_Tag_Shift 360
#define V4IM_Tag_Rotate 361
#define V4IM_Tag_Easter 362
#define V4IM_Tag_YomKippur 363
#define V4IM_Tag_Passover 364
#define V4IM_Tag_Direction 365
#define V4IM_Tag_Dawn 366
#define V4IM_Tag_Dusk 367
#define V4IM_Tag_TimeZone 368
#define V4IM_Tag_Country 369
#define V4IM_Tag_Language 370
#define V4IM_Tag_Calendar 371
#define V4IM_Tag_HTTP_HTML 372
#define V4IM_Tag_HTTP_Other 373
#define V4IM_Tag_FileName 374
#define V4IM_Tag_Literal 375
#define V4IM_Tag_Data 376
#define V4IM_Tag_Delete 377
#define V4IM_Tag_Frames 378
#define V4IM_Tag_DaylightSavings 379
#define V4IM_Tag_Force 380
#define V4IM_Tag_Quote 381
#define V4IM_Tag_DotDotToList 382
#define V4IM_Tag_ADPoint 383
#define V4IM_Tag_SourceFile 384
#define V4IM_Tag_Distribution 385
#define V4IM_Tag_Offset 386
#define V4IM_Tag_Linear 387
#define V4IM_Tag_Epsilon 388
#define V4IM_Tag_Wait 389
#define V4IM_Tag_IPAddress 390
#define V4IM_Tag_Ignore 391
#define V4IM_Tag_Soundex 392
#define V4IM_Tag_Hash64 393
#define V4IM_Tag_Surname 394
#define V4IM_Tag_GivenName 395
#define V4IM_Tag_MiddleName 396
#define V4IM_Tag_NickName 397
#define V4IM_Tag_Suffix 398
#define V4IM_Tag_Duplicate 399
#define V4IM_Tag_Rename 400
#define V4IM_Tag_Session 401
#define V4IM_Tag_Ruler 402
#define V4IM_Tag_Window 403
#define V4IM_Tag_CommandLine 404
#define V4IM_Tag_Active 405
#define V4IM_Tag_DHTML 406
#define V4IM_Tag_BackgroundImage 407
#define V4IM_Tag_BackgroundColor 408
#define V4IM_Tag_Server 409
#define V4IM_Tag_LogoImage 410
#define V4IM_Tag_LogoURL 411
#define V4IM_Tag_Stack 412
#define V4IM_Tag_RDB 413
#define V4IM_Tag_Continue 414
#define V4IM_Tag_HashMD5 415
#define V4IM_Tag_Copy 416
#define V4IM_Tag_ASin 417
#define V4IM_Tag_ACos 418
#define V4IM_Tag_ATan 419
#define V4IM_Tag_ATan2 420
#define V4IM_Tag_TanH 421
#define V4IM_Tag_CosH 422
#define V4IM_Tag_SinH 423
#define V4IM_Tag_CSS 424
#define V4IM_Tag_Project 425
#define V4IM_Tag_Problem 426
#define V4IM_Tag_Empty 427
#define V4IM_Tag_Add 428
#define V4IM_Tag_Exact 429
#define V4IM_Tag_Class 430
#define V4IM_Tag_Top 431
#define V4IM_Tag_Height 432
#define V4IM_Tag_ExcelIni 433
#define V4IM_Tag_Capitalize 434
#define V4IM_Tag_Prefetch 435
#define V4IM_Tag_OS 436
#define V4IM_Tag_CPU 437
#define V4IM_Tag_ComFileExt 438
#define V4IM_Tag_Endian 439
#define V4IM_Tag_Processors 440
#define V4IM_Tag_URL2 441
#define V4IM_Tag_URL3 442
#define V4IM_Tag_URL4 443
#define V4IM_Tag_URL5 444
#define V4IM_Tag_URLBase2 445
#define V4IM_Tag_URLBase3 446
#define V4IM_Tag_URLBase4 447
#define V4IM_Tag_URLBase5 448
#define V4IM_Tag_Drawer 449
#define V4IM_Tag_Intersection 450
#define V4IM_Tag_Volume 451
#define V4IM_Tag_LunarPhase 452
#define V4IM_Tag_SolarLongitude 453
#define V4IM_Tag_LunarAltitude 454
#define V4IM_Tag_Child 455
#define V4IM_Tag_Enter 456
#define V4IM_Tag_Return 457
#define V4IM_Tag_Leave 458
#define V4IM_Tag_Bottom 459
#define V4IM_Tag_New 460
#define V4IM_Tag_Swap 461
#define V4IM_Tag_Escape 462
#define V4IM_Tag_Filter 463
#define V4IM_Tag_AutoCommit 464
#define V4IM_Tag_Lazy 465
#define V4IM_Tag_Catch 466
#define V4IM_Tag_Throw 467
#define V4IM_Tag_Segment 468
#define V4IM_Tag_UTF8 469
#define V4IM_Tag_Key2 470
#define V4IM_Tag_UNUSED2 471
#define V4IM_Tag_UNUSED3 472
#define V4IM_Tag_IC 475
#define V4IM_Tag_Evaluations 476
#define V4IM_Tag_Token 477
#define V4IM_Tag_Tree 478
#define V4IM_Tag_UNUSED6 479
#define V4IM_Tag_Exclude 480
#define V4IM_Tag_Nodes 481
#define V4IM_Tag_BindEval 482
#define V4IM_Tag_Decimals 483
#define V4IM_Tag_DotIndex 484
#define V4IM_Tag_DotList 485
#define V4IM_Tag_Entries 486
#define V4IM_Tag_HasIsa 487
#define V4IM_Tag_IsA 488
#define V4IM_Tag_MMDDYY 489
#define V4IM_Tag_MMYY 490
#define V4IM_Tag_YMDOrder 491
#define V4IM_Tag_Normalize 492
#define V4IM_Tag_Overload 493
#define V4IM_Tag_Multiple 494
#define V4IM_Tag_Structure 495
#define V4IM_Tag_UOMId 496
#define V4IM_Tag_Hierarchy 497
#define V4IM_Tag_Local 498
#define V4IM_Tag_BindIf 499
#define V4IM_Tag_Anchor 500
#define V4IM_Tag_Threads 501
#define V4IM_Tag_AsIs 502
#define V4IM_Tag_Listen 503
#define V4IM_Tag_Send 504
#define V4IM_Tag_Spawn 505
#define V4IM_Tag_Sequence 506
#define V4IM_Tag_Peer 507
#define V4IM_Tag_NoPrefix 508
#define V4IM_Tag_Route 509
#define V4IM_Tag_UOM 510
#define V4IM_Tag_TTL 511
#define V4IM_Tag_Heading 512
#define V4IM_Tag_Line 513
#define V4IM_Tag_Recap 514
#define V4IM_Tag_Version 515
#define V4IM_Tag_Disable 516
#define V4IM_Tag_Embed 517
#define V4IM_Tag_RDBw 518
#define V4IM_Tag_Any 519
#define V4IM_Tag_Age 520
#define V4IM_Tag_UTC 521
#define V4IM_Tag_Array 522
#define V4IM_Tag_JSON 523
#define V4IM_Tag_References 524
#define V4IM_Tag_Weight 525
#define V4IM_Tag_Sort 526
#define V4IM_Tag_Patronymic 527
#define V4IM_Tag_Comment 528
#define V4IM_Tag_Decimal 529
#define V4IM_Tag_FixedLength 530
#define V4IM_Tag_Missing 531
#define V4IM_Tag_Start 532
#define V4IM_Tag_Delimiter 533
#define V4IM_Tag_AggKey 534
#define V4IM_Tag_Quoted 535
#define V4IM_Tag_Release 536
#define V4IM_Tag_Hidden 537
#define V4IM_Tag_AreaAll 538
#define V4IM_Tag_AreaRename 539
#define V4IM_Tag_AreaUpdate 540
#define V4IM_Tag_Files 541
#define V4IM_Tag_HTTPGet 542
#define V4IM_Tag_HTTPPut 543
#define V4IM_Tag_OSFileListOf 544
#define V4IM_Tag_OSFileInfo 545
#define V4IM_Tag_ProjectArea 546
#define V4IM_Tag_Redirection 547
#define V4IM_Tag_Unlock 548
#define V4IM_Tag_EvalNest 549
#define V4IM_Tag_BucketSize 550
#define V4IM_Tag_Dictionary 551
#define V4IM_Tag_Rebuild 552
#define V4IM_Tag_Sequential 553
#define V4IM_Tag_Expression 554
#define V4IM_Tag_Link 555
#define V4IM_Tag_Ending 556
#define V4IM_Tag_Java 557
#define V4IM_Tag_Hash 558
#define V4IM_Tag_DES 559
#define V4IM_Tag_Arg 560
#define V4IM_Tag_Occurs 561
#define V4IM_Tag_Encode 562
#define V4IM_Tag_Decode 563
#define V4IM_Tag_Move 564
#define V4IM_Tag_Random 565
#define V4IM_Tag_Logical 566
#define V4IM_Tag_Second 567
#define V4IM_Tag_Job 568
#define V4IM_Tag_Privileges 569
#define V4IM_Tag_Setup 570
#define V4IM_Tag_Tag 571
#define V4IM_Tag_Memo 572
#define V4IM_Tag_RDBId 573
#define V4IM_Tag_Image 574
#define V4IM_Tag_Progress 575
#define V4IM_Tag_History 576
#define V4IM_Tag_Commit 577
#define V4IM_Tag_Section 578
#define V4IM_Tag_Idle 579
#define V4IM_Tag_Split 580
#define V4IM_Tag_Elevate 581
#define V4IM_Tag_Other 582
#define V4IM_Tag_Cookie 583
#define V4IM_Tag_Zip 584
#define V4IM_Tag_SessionKey 585
#define V4IM_Tag_XDBAccess 586
#define V4IM_Tag_XDBXCt 587
#define V4IM_Tag_Text 588
#define V4IM_Tag_CaseSensitive 589
#define V4IM_Tag_Root 590
#define V4IM_Tag_Javascript 591
#define V4IM_Tag_UserOptions 592
#define V4IM_Tag_UnusedXXXXXXX 593
#define V4IM_Tag_Test 594
#define V4IM_Tag_Plus 595
#define V4IM_Tag_Minus 596
#define V4IM_Tag_Set 597
#define V4IM_Tag_ContextP 598
#define V4IM_Tag_UNUSED4 599
#define V4IM_Tag_As 600
#define V4IM_Tag_Post 601
#define V4IM_Tag_File 602
#define V4IM_Tag_Remote 603
#define V4IM_Tag_Attach 604
#define V4IM_Tag_From 605
#define V4IM_Tag_Subject 606
#define V4IM_Tag_Exit 607
#define V4IM_Tag_Level 608
#define V4IM_Tag_InitialLevel 609
#define V4IM_Tag_Parse 610
#define V4IM_Tag_Word 611
#define V4IM_Tag_Certificate 612
#define V4IM_Tag_UserAgent 613
#define V4IM_Tag_ArgFile 614
#define V4IM_Tag_Buffer 615
#define V4IM_Tag_POSIX 616
#define V4IM_Tag_Retry 617
#define V4IM_Tag_DisplayerTrace 618
#define V4IM_Tag_DateTime 619
#define V4IM_Tag_CSV 620
#define V4IM_Tag_Warning 621
#define V4IM_Tag_Web 622
#define V4IM_Tag_Function 623
#define V4IM_Tag_Bearer 634
#define V4IM_Tag_CC 635

#define V4IM_Tag_MaxValue 635	/* **** MUST BE >= HIGHEST TAG VALUE **** */

#if V4IM_Tag_MaxValuelocal < V4IM_Tag_MaxValue
#error V4IM_Tag_MaxValuelocal is less than V4IM_Tag_MaxValue
#endif



/*	P A R S I N G   M O D U L E S				*/

/*	v4im_DictList is list of internal modules for modules below */

#define V4IM_OpCode_Echo 1			/* Echo second argument */
#define V4IM_OpCode_EvalCmd 2			/* Pass second argument to V4EVAL */
#define V4IM_OpCode_ListSize 3			/* Get number of elements in a point */
#define V4IM_OpCode_Locale 4			/* Locale */
#define V4IM_OpCode_ODBCError 5
//#define V4IM_OpCode_XMLRPC 6			/* XMLRPC() */
#define V4IM_OpCode_EvalPt 7			/* EvalPt() */
#define V4IM_OpCode_PRUpd 8			/* PRUpd() */
#define V4IM_OpCode_EnumX 9			/* EnumX() */
#define V4IM_OpCode_Timer 10			/* Timer() */
#define V4IM_OpCode_Names 11			/* Names() */
#define V4IM_OpCode_Flatten 12			/* Flatten() */
#define V4IM_OpCode_Counter 13			/* Counter() */
#define V4IM_OpCode_Plus 14			/* Plus */
#define V4IM_OpCode_Minus 15			/* Minus */
#define V4IM_OpCode_Mult 16			/* Multiply */
#define V4IM_OpCode_Remove_Point 17		/* Remove a point (usually a binding) */
#define V4IM_OpCode_And 18			/*  "AND" */
#define V4IM_OpCode_Or 19			/*  "Or" */
#define V4IM_OpCode_Not 20			/*  "Not" */
#define V4IM_OpCode_If 21			/* Logical "If" handler */
#define V4IM_OpCode_Def 22
#define V4IM_OpCode_Error 23			/* Error() */
#define V4IM_OpCode_Sum 24			/* Calculate sum of arguments */
#define V4IM_OpCode_Min 25			/* Pick minimum out of a list */
#define V4IM_OpCode_Max 26			/* Pick maximum out of a list */
#define V4IM_OpCode_BindEE 27			/* Bind intersection to value */
#define V4IM_OpCode_Tree 28			/* Tree() */
#define V4IM_OpCode_EchoP 29			/* Echo prompt (no newline at end) */
#define V4IM_OpCode_EnumCL 30			/* Enumerate, Push Context, Create List */
#define V4IM_OpCode_MakeLC 31			/* Make List Conditional */
#define V4IM_OpCode_Trig 32			/* Trig - trigonometric functions */
#define V4IM_OpCode_Sort 33			/* Sort items in a list */
#define V4IM_OpCode_MakeP 34			/* Make a point from a dimension & value */
#define V4IM_OpCode_EQ 35			/* Compare Equal */
#define V4IM_OpCode_NE 36
#define V4IM_OpCode_MakeL 37			/* MakeL( pt pt pt ) */
#define V4IM_OpCode_GT 38
#define V4IM_OpCode_LT 39
#define V4IM_OpCode_GE 40
#define V4IM_OpCode_LE 41
#define V4IM_OpCode_EvalArithExp 42		/* Eval Arithmetic Expression */
#define V4IM_OpCode_IsctVals 43
#define V4IM_OpCode_ContextL 44			/* Set Context Point (in local/current frame) */
#define V4IM_OpCode_Context 45			/* Set Context Point (GLOBAL) */
#define V4IM_OpCode_EchoT 46
#define V4IM_OpCode_Pack1616 47			/* Pack 16+16 bits into 32 */
#define V4IM_OpCode_Output 48
#define V4IM_OpCode_Tally 49			/* Tally( [list] (value list) (combo list) ) */
#define V4IM_OpCode_SSFormat 50			/* SSFormat( specs ... ) */
#define V4IM_OpCode_Drawer 51
#define V4IM_OpCode_Div 52
#define V4IM_OpCode_Percent 53			/* Percent( num1 num2 ) */
#define V4IM_OpCode_Self 54			/* Self( point ) */
#define V4IM_OpCode_MakePm 55
#define V4IM_OpCode_DTInfo 56			/* DTInfo( udate what ) */
#define V4IM_OpCode_AggVal 57			/* AggVal( keypt dimpt index ) */
#define V4IM_OpCode_Dim 58			/* Dim() */
#define V4IM_OpCode_UOM 59			/* UOM( x x x ) */
#define V4IM_OpCode_In 60			/* In( pt pt ) */
#define V4IM_OpCode_Project 61			/* Project() */
#define V4IM_OpCode_Eval 62			/* Eval( Hold:pt Context:pt (list) or [isct] */
#define V4IM_OpCode_MakeIsct 63			/* MakeIsct( pt pt ... ) */
#define V4IM_OpCode_Parse 64
#define V4IM_OpCode_EQk 65			/* EQk( pt1 pt2 ) */
#define V4IM_OpCode_List 66			/* List ( ... ) */
#define V4IM_OpCode_Array 67			/* Array() */
#define V4IM_OpCode_Message 68			/* Message() */
#define V4IM_OpCode_Transform 69		/* Transform( list [test] [transform] ) */
#define V4IM_OpCode_JSON 70
#define V4IM_OpCode_EvalBL 71			/* EvalBL( @[isct] ) */
#define V4IM_OpCode_StatProduct 72
#define V4IM_OpCode_Fail 73			/* Fail() */
#define V4IM_OpCode_Format 74			/* Format( num format-options ) */
#define V4IM_OpCode_Table 75			/* Table() */
#define V4IM_OpCode_PCChg1 76			/* PCChg1( num1 num2 ) */
#define V4IM_OpCode_PCChg2 77			/* PCChg2( num1 num2 ) */
#define V4IM_OpCode_Secure 78			/* Secure() */
#define V4IM_OpCode_StatAdjCosSim 79		/* StatAdjCosSim() */
#define V4IM_OpCode_NOp 80			/* NOp( logical ) */
#define V4IM_OpCode_RCVi 81			/* Trace( @isct ) */
#define V4IM_OpCode_NDefQ 82
#define V4IM_OpCode_MakeIn 83			/* MakeIn() */
#define V4IM_OpCode_LEG 84			/* LEG( num1 num2 @isct @isct @isct ) */
#define V4IM_OpCode_EchoS 85			/* EchoS( list ) */
#define V4IM_OpCode_SSDim 86			/* SSDim( dim attributes ) */
#define V4IM_OpCode_Num 87			/* xxx */
#define V4IM_OpCode_MakeQIsct 88		/* MakeQIsct( pt pt ... ) */
#define V4IM_OpCode_EchoD 89
#define V4IM_OpCode_BindQE 90			/* Bind intersection to value */
#define V4IM_OpCode_Lock 91			/*  */
#ifdef WANTIMCACHE
#define V4IM_OpCode_Cache 92			/* Cache( pt pt ... ) */
#endif
#define V4IM_OpCode_Enum 93			/* same as EnumCE */
#define V4IM_OpCode_Do 94			/* ptn = Do( pt pt ... ptn ) */
#define V4IM_OpCode_FinCvtDec 95		/* FinCvtDec */
#define V4IM_OpCode_FinCvtFrac 96
#define V4IM_OpCode_FinDep 97
#define V4IM_OpCode_BindQQ 98			/* Bind intersection to value */
#define V4IM_OpCode_DeferDo 99
#define V4IM_OpCode_FinIntRate 100
#define V4IM_OpCode_Bits 101
#define V4IM_OpCode_RCV 102
#define V4IM_OpCode_FinNPV 103
#define V4IM_OpCode_FinIRR 104
#define V4IM_OpCode_FinTVM 105
#define V4IM_OpCode_StatDist 106
#define V4IM_OpCode_StatDistInv 107
#define V4IM_OpCode_V4ISCon 108
#define V4IM_OpCode_V4ISVal 109
#define V4IM_OpCode_FinFVSched 110
#define V4IM_OpCode_FinPrice 111
#define V4IM_OpCode_V4ISOp 112
#define V4IM_OpCode_FinCoupon 113
#define V4IM_OpCode_FinSecDisc 114
#define V4IM_OpCode_FinSecDur 115
#define V4IM_OpCode_FinSecInt 116
#define V4IM_OpCode_BindEQ 117
#define V4IM_OpCode_FinSecPrice 118
#define V4IM_OpCode_FinSecRcvd 119
#define V4IM_OpCode_FinSecYield 120
#define V4IM_OpCode_LList 121
#define V4IM_OpCode_MakeT 122
#define V4IM_OpCode_FinTBill 123
#define V4IM_OpCode_Set 124
#define V4IM_OpCode_GuiAlert 125
#define V4IM_OpCode_FinDays360 126
#define V4IM_OpCode_FinDisc 127
#define V4IM_OpCode_StatAvgDev 128
#define V4IM_OpCode_StatAvg 129
#define V4IM_OpCode_StatDevSq 130
#define DISABLETALLYM 1				/* 071005 - Disable TallyM until I decide what I want to do with it */
#define V4IM_OpCode_TallyM 131
#define V4IM_OpCode_StatFisher 132
#define V4IM_OpCode_StatFisherInv 133
#define V4IM_OpCode_StatGammaLn 134
#define V4IM_OpCode_StatGeoMean 135
#define V4IM_OpCode_StatHarMean 136
#define V4IM_OpCode_StatKurtosis 137
#define V4IM_OpCode_Area 138
#define V4IM_OpCode_DefQ 139
#define V4IM_OpCode_StatMedian 140
#define V4IM_OpCode_StatMode 141
#define V4IM_OpCode_NIn 142
#define V4IM_OpCode_Str 143
#define V4IM_OpCode_Spawn 144
#define V4IM_OpCode_MakeQL 145
#define V4IM_OpCode_Is1 146
#define V4IM_OpCode_V4 147
#define V4IM_OpCode_StatPercentile 148
#define V4IM_OpCode_StatPCRank 149
#define V4IM_OpCode_StatPermute 150
#define V4IM_OpCode_DDiv 151
#define V4IM_OpCode_StatQuartile 152
#define V4IM_OpCode_StatRank 153
#define V4IM_OpCode_StatRSQ 154
#define V4IM_OpCode_StatSkew 155
#define V4IM_OpCode_StatVar 156
#define V4IM_OpCode_StatVarP 157
#define V4IM_OpCode_EvalLE 158
#define V4IM_OpCode_StatRan 159
#define V4IM_OpCode_Abs 160
#define V4IM_OpCode_StatMax 161
#define V4IM_OpCode_StatMin 162
#define V4IM_OpCode_StatStandardize 163
#define V4IM_OpCode_StatSlope 164
#define V4IM_OpCode_StatStdDev 165
#define V4IM_OpCode_StatStdDevP 166
#define V4IM_OpCode_StatStdErrYX 167
#define V4IM_OpCode_StatSumSq 168
#define V4IM_OpCode_StatSumX2mY2 169
#define V4IM_OpCode_StatSumX2pY2 170
#define V4IM_OpCode_StatSumXmY2 171
#define V4IM_OpCode_StatTrimMean 172
#define V4IM_OpCode_Optimize 173
#define V4IM_OpCode_StatPearson 174
#define V4IM_OpCode_StatCovar 175
#define V4IM_OpCode_StatCritBinom 176
#define V4IM_OpCode_EvalUM 177
#define V4IM_OpCode_EchoN 178
#define V4IM_OpCode_StatFTest 179
#define V4IM_OpCode_StatLinEst 180
#define V4IM_OpCode_StatLogEst 181
#define V4IM_OpCode_Coerce 182
#define V4IM_OpCode_SSExp 183
#define V4IM_OpCode_MthMod 184
#define V4IM_OpCode_StatTTest 185
#define V4IM_OpCode_StatTTestM 186
#define V4IM_OpCode_StatZTest 187
#define V4IM_OpCode_StatZTestM 188
#define V4IM_OpCode_StatCorrel 189
#define V4IM_OpCode_OSExt 190
#define V4IM_OpCode_EchoE 191
#define V4IM_OpCode_StatChiTest 192
#define V4IM_OpCode_StatConfidence 193
#define V4IM_OpCode_StatForecast 194
#define V4IM_OpCode_StatLinFit 195
#define V4IM_OpCode_AggUpd 197
#define V4IM_OpCode_SSULC 198
#define V4IM_OpCode_StatFrequency 199
#define V4IM_OpCode_StatProb 200
#define V4IM_OpCode_StatErrF 201
#define V4IM_OpCode_StatErrFC 202
#define V4IM_OpCode_dbConnect 203
#define V4IM_OpCode_dbGet 204
#define V4IM_OpCode_dbVal 205
#define V4IM_OpCode_dbFree 206
#define V4IM_OpCode_dbInfo 207
#define V4IM_OpCode_AggPut 208
#define V4IM_OpCode_SSCol 210
#define V4IM_OpCode_SSVal 211
#define V4IM_OpCode_SSRow 212
#define V4IM_OpCode_AggDel 213
#define V4IM_OpCode_NEk 214
#define V4IM_OpCode_Rpt 215
#define V4IM_OpCode_TEQ 216
#define V4IM_OpCode_Average 217
#define V4IM_OpCode_Geo 218
#define V4IM_OpCode_Sqrt 219
#define V4IM_OpCode_Log 220
#define V4IM_OpCode_OSFile 221
//#define V4IM_OpCode_HTML 222
#define V4IM_OpCode_OSInfo 223
#define V4IM_OpCode_DMCluster 224
#define V4IM_OpCode_ReAllocate 225
#define V4IM_OpCode_ODBCXct 226
#define V4IM_OpCode_GraphConnect 227
#define V4IM_OpCode_GuiMsgBox 228
#define V4IM_OpCode_Socket 229
#define V4IM_OpCode_StatScale 230
#define V4IM_OpCode_ChemOpt 231
#define V4IM_OpCode_EchoW 232
#define V4IM_OpCode_EchoA 233
#define V4IM_OpCode_Try 234
#define V4IM_OpCode_Throw 235
#define V4IM_OpCode_Macro 236
#define V4IM_OpCode_db 237
#define V4IM_OpCode_Dbg 238
#define V4IM_OpCode_Quote 239
#define V4IM_OpCode_UnQuote 240
#define V4IM_OpCode_ListNE 241
#define V4IM_OpCode_Zip 242
#define V4IM_OpCode_MT 243
#define V4IM_OpCode_nMT 244
#define V4IM_OpCode_Len 245
#define V4IM_OpCode_HTTP 246
#define V4IM_OpCode_FTP 247
#define V4IM_OpCode_SendMail 248
#define V4IM_OpCode_NGram 249
#define V4IM_OpCode_Same 250
#define V4IM_OpCode_nSame 251
#define V4IM_OpCode_nStr 252
#define V4IM_OpCode_JSONRef 253
#define V4IM_OpCode_JSONRefDim 254
#define V4IM_OpCode_Ifnot 255
#define V4IM_OpCode_Unicode 256
#define V4IM_OpCode_IsAll 257

#define V4IM_OpCode_LastOpcode 257	/* Value of highest opcode */

#if V4BUILD_OpCodeMax < V4IM_OpCode_LastOpcode
#error V4BUILD_OpCodeMax is less than V4IM_OpCode_LastOpcode
#endif


#ifdef V4IMTags

#define TAGLIST \
  TAGENTRY(V4IM_Tag_A, "A") \
  TAGENTRY(V4IM_Tag_Abs, "Abs") \
  TAGENTRY(V4IM_Tag_AbsDev, "AbsDev") \
  TAGENTRY(V4IM_Tag_Acceptor, "Acceptor") \
  TAGENTRY(V4IM_Tag_Access, "Access") \
  TAGENTRY(V4IM_Tag_ACos, "ACos") \
  TAGENTRY(V4IM_Tag_Action, "Action") \
  TAGENTRY(V4IM_Tag_Active, "Active") \
  TAGENTRY(V4IM_Tag_Add, "Add") \
  TAGENTRY(V4IM_Tag_ADPoint, "ADPoint") \
  TAGENTRY(V4IM_Tag_After, "After") \
  TAGENTRY(V4IM_Tag_Age, "Age") \
  TAGENTRY(V4IM_Tag_Agg, "Agg") \
  TAGENTRY(V4IM_Tag_AggKey, "AggKey") \
  TAGENTRY(V4IM_Tag_All, "All") \
  TAGENTRY(V4IM_Tag_Alpha, "Alpha") \
  TAGENTRY(V4IM_Tag_Anchor, "Anchor") \
  TAGENTRY(V4IM_Tag_And, "And") \
  TAGENTRY(V4IM_Tag_AndC, "AndC") \
  TAGENTRY(V4IM_Tag_Any, "Any") \
  TAGENTRY(V4IM_Tag_Append, "Append") \
  TAGENTRY(V4IM_Tag_Area, "Area") \
  TAGENTRY(V4IM_Tag_AreaAll, "AreaAll") \
  TAGENTRY(V4IM_Tag_AreaRename, "AreaRename") \
  TAGENTRY(V4IM_Tag_AreaUpdate, "AreaUpdate") \
  TAGENTRY(V4IM_Tag_Arg, "Arg") \
  TAGENTRY(V4IM_Tag_ArgFile, "ArgFile") \
  TAGENTRY(V4IM_Tag_Array, "Array") \
  TAGENTRY(V4IM_Tag_As, "As") \
  TAGENTRY(V4IM_Tag_AsIs, "AsIs") \
  TAGENTRY(V4IM_Tag_ASin, "ASin") \
  TAGENTRY(V4IM_Tag_At, "At") \
  TAGENTRY(V4IM_Tag_ATan, "ATan") \
  TAGENTRY(V4IM_Tag_ATan2, "ATan2") \
  TAGENTRY(V4IM_Tag_Attach, "Attach") \
  TAGENTRY(V4IM_Tag_Attributes, "Attributes") \
  TAGENTRY(V4IM_Tag_AutoCommit, "AutoCommit") \
  TAGENTRY(V4IM_Tag_AutoFit, "AutoFit") \
  TAGENTRY(V4IM_Tag_AutoHead, "AutoHead") \
  TAGENTRY(V4IM_Tag_Average, "Average") \
  TAGENTRY(V4IM_Tag_Average, "Avg") \
  TAGENTRY(V4IM_Tag_AWhile, "AWhile") \
  TAGENTRY(V4IM_Tag_B, "B") \
  TAGENTRY(V4IM_Tag_BackgroundColor, "BackgroundColor") \
  TAGENTRY(V4IM_Tag_BackgroundImage, "BackgroundImage") \
  TAGENTRY(V4IM_Tag_BaseDate, "BaseDate") \
  TAGENTRY(V4IM_Tag_BaseMonth, "BaseMonth") \
  TAGENTRY(V4IM_Tag_Basis, "Basis") \
  TAGENTRY(V4IM_Tag_Bearer, "Bearer") \
  TAGENTRY(V4IM_Tag_Before, "Before") \
  TAGENTRY(V4IM_Tag_Begin, "Begin") \
  TAGENTRY(V4IM_Tag_Beta, "Beta") \
  TAGENTRY(V4IM_Tag_BetaPDF, "BetaPDF") \
  TAGENTRY(V4IM_Tag_BFAgg, "BFAgg") \
  TAGENTRY(V4IM_Tag_Bind, "Bind") \
  TAGENTRY(V4IM_Tag_BindEval, "BindEval") \
  TAGENTRY(V4IM_Tag_BindIf, "BindIf") \
  TAGENTRY(V4IM_Tag_Binding, "Binding") \
  TAGENTRY(V4IM_Tag_Binomial, "Binomial") \
  TAGENTRY(V4IM_Tag_Bits, "Bits") \
  TAGENTRY(V4IM_Tag_BMargin, "BMargin") \
  TAGENTRY(V4IM_Tag_Body, "Body") \
  TAGENTRY(V4IM_Tag_Bottom, "Bottom") \
  TAGENTRY(V4IM_Tag_Break, "Break") \
  TAGENTRY(V4IM_Tag_BucketSize,"BucketSize") \
  TAGENTRY(V4IM_Tag_Buffer, "Buffer") \
  TAGENTRY(V4IM_Tag_By, "By") \
  TAGENTRY(V4IM_Tag_ByIf, "ByIf") \
  TAGENTRY(V4IM_Tag_ByList, "ByList") \
  TAGENTRY(V4IM_Tag_Bytes, "Bytes") \
  TAGENTRY(V4IM_Tag_Cache, "Cache") \
  TAGENTRY(V4IM_Tag_Calc, "Calc") \
  TAGENTRY(V4IM_Tag_Calendar, "Calendar") \
  TAGENTRY(V4IM_Tag_Capitalize, "Capitalize") \
  TAGENTRY(V4IM_Tag_CaseSensitive, "CaseSensitive") \
  TAGENTRY(V4IM_Tag_Catch, "Catch") \
  TAGENTRY(V4IM_Tag_CC, "CC") \
  TAGENTRY(V4IM_Tag_Cell, "Cell") \
  TAGENTRY(V4IM_Tag_CellColor, "CellColor") \
  TAGENTRY(V4IM_Tag_Center, "Center") \
  TAGENTRY(V4IM_Tag_Certificate, "Certificate") \
  TAGENTRY(V4IM_Tag_Child, "Child") \
  TAGENTRY(V4IM_Tag_ChiSq, "ChiSq") \
  TAGENTRY(V4IM_Tag_Class, "Class") \
  TAGENTRY(V4IM_Tag_Close, "Close") \
  TAGENTRY(V4IM_Tag_ColCap, "ColCap") \
  TAGENTRY(V4IM_Tag_ColColor, "ColColor") \
  TAGENTRY(V4IM_Tag_ColDim, "ColDim") \
  TAGENTRY(V4IM_Tag_ColGap, "ColGap") \
  TAGENTRY(V4IM_Tag_Collapse, "Collapse") \
  TAGENTRY(V4IM_Tag_Column, "Column") \
  TAGENTRY(V4IM_Tag_ColumnId, "ColumnId") \
  TAGENTRY(V4IM_Tag_Columns, "Columns") \
  TAGENTRY(V4IM_Tag_ComFileExt, "ComFileExt") \
  TAGENTRY(V4IM_Tag_CoMingle, "CoMingle") \
  TAGENTRY(V4IM_Tag_CommandLine, "CommandLine") \
  TAGENTRY(V4IM_Tag_Comment, "Comment") \
  TAGENTRY(V4IM_Tag_Commit, "Commit") \
  TAGENTRY(V4IM_Tag_Compounds, "Compounds") \
  TAGENTRY(V4IM_Tag_Connection, "CONNECTION") \
  TAGENTRY(V4IM_Tag_ConnectTime, "ConnectTime") \
  TAGENTRY(V4IM_Tag_Context, "Context") \
  TAGENTRY(V4IM_Tag_ContextP, "ContextP") \
  TAGENTRY(V4IM_Tag_Continue, "Continue") \
  TAGENTRY(V4IM_Tag_Cookie, "Cookie") \
  TAGENTRY(V4IM_Tag_Copy, "Copy") \
  TAGENTRY(V4IM_Tag_Cos, "Cos") \
  TAGENTRY(V4IM_Tag_CosH, "CosH") \
  TAGENTRY(V4IM_Tag_Cost, "Cost") \
  TAGENTRY(V4IM_Tag_Count, "Count") \
  TAGENTRY(V4IM_Tag_Country, "Country") \
  TAGENTRY(V4IM_Tag_Coupon, "Coupon") \
  TAGENTRY(V4IM_Tag_CPU, "CPU") \
  TAGENTRY(V4IM_Tag_CPUTime, "CPUTime") \
  TAGENTRY(V4IM_Tag_Create, "Create") \
  TAGENTRY(V4IM_Tag_CSS, "CSS") \
  TAGENTRY(V4IM_Tag_CSV, "CSV") \
  TAGENTRY(V4IM_Tag_Cumulative, "Cumulative") \
  TAGENTRY(V4IM_Tag_Data, "Data") \
  TAGENTRY(V4IM_Tag_DataId, "DataId") \
  TAGENTRY(V4IM_Tag_DateAS, "DateAS") \
  TAGENTRY(V4IM_Tag_DateBS, "DateBS") \
  TAGENTRY(V4IM_Tag_DateTime, "DateTime") \
  TAGENTRY(V4IM_Tag_Dawn, "Dawn") \
  TAGENTRY(V4IM_Tag_Day, "Day") \
  TAGENTRY(V4IM_Tag_DaylightSavings, "DaylightSavings") \
  TAGENTRY(V4IM_Tag_DayOfWeek, "DayOfWeek") \
  TAGENTRY(V4IM_Tag_DayOfYear, "DayOfYear") \
  TAGENTRY(V4IM_Tag_DaysBS, "DaysBS") \
  TAGENTRY(V4IM_Tag_DaysNC, "DaysNC") \
  TAGENTRY(V4IM_Tag_DaysS, "DaysS") \
  TAGENTRY(V4IM_Tag_DBC, "DBC") \
  TAGENTRY(V4IM_Tag_DDB, "DDB") \
  TAGENTRY(V4IM_Tag_Decimal, "Decimal") \
  TAGENTRY(V4IM_Tag_Decimals, "Decimals") \
  TAGENTRY(V4IM_Tag_Decode, "Decode") \
  TAGENTRY(V4IM_Tag_Default, "Default") \
  TAGENTRY(V4IM_Tag_Define, "Define") \
  TAGENTRY(V4IM_Tag_Degrees, "Degrees") \
  TAGENTRY(V4IM_Tag_Delete, "Delete") \
  TAGENTRY(V4IM_Tag_Delimiter, "Delimiter") \
  TAGENTRY(V4IM_Tag_Depth, "Depth") \
  TAGENTRY(V4IM_Tag_DES, "DES") \
  TAGENTRY(V4IM_Tag_Description, "Description") \
  TAGENTRY(V4IM_Tag_DHTML, "DHTML") \
  TAGENTRY(V4IM_Tag_Dictionary,"Dictionary") \
  TAGENTRY(V4IM_Tag_Dim, "Dim") \
  TAGENTRY(V4IM_Tag_Direction, "Direction") \
  TAGENTRY(V4IM_Tag_Directory, "Directory") \
  TAGENTRY(V4IM_Tag_Disable, "Disable") \
  TAGENTRY(V4IM_Tag_Discount, "Discount") \
  TAGENTRY(V4IM_Tag_Displayer, "Displayer") \
  TAGENTRY(V4IM_Tag_DisplayerTrace, "DisplayerTrace") \
  TAGENTRY(V4IM_Tag_Distance, "Distance") \
  TAGENTRY(V4IM_Tag_Distribution, "Distribution") \
  TAGENTRY(V4IM_Tag_Do, "Do") \
  TAGENTRY(V4IM_Tag_DoF, "DoF") \
  TAGENTRY(V4IM_Tag_Dot, "Dot") \
  TAGENTRY(V4IM_Tag_DotDotToList, "DotDotToList") \
  TAGENTRY(V4IM_Tag_DotIndex, "DotIndex") \
  TAGENTRY(V4IM_Tag_DotList, "DotList") \
  TAGENTRY(V4IM_Tag_Drawer, "Drawer") \
  TAGENTRY(V4IM_Tag_DSN, "DSN") \
  TAGENTRY(V4IM_Tag_Duplicate, "Duplicate") \
  TAGENTRY(V4IM_Tag_Dusk, "Dusk") \
  TAGENTRY(V4IM_Tag_Easter, "Easter") \
  TAGENTRY(V4IM_Tag_Echo, "Echo") \
  TAGENTRY(V4IM_Tag_Edge, "Edge") \
  TAGENTRY(V4IM_Tag_Effective, "Effective") \
  TAGENTRY(V4IM_Tag_Elevate, "Elevate") \
  TAGENTRY(V4IM_Tag_Else, "Else") \
  TAGENTRY(V4IM_Tag_ElseIf, "ElseIf") \
  TAGENTRY(V4IM_Tag_Embed, "Embed") \
  TAGENTRY(V4IM_Tag_Empty, "Empty") \
  TAGENTRY(V4IM_Tag_Encode, "Encode") \
  TAGENTRY(V4IM_Tag_End, "End") \
  TAGENTRY(V4IM_Tag_Endian, "Endian") \
  TAGENTRY(V4IM_Tag_Ending, "Ending") \
  TAGENTRY(V4IM_Tag_Enter, "Enter") \
  TAGENTRY(V4IM_Tag_Entries, "Entries") \
  TAGENTRY(V4IM_Tag_Enum, "Enum") \
  TAGENTRY(V4IM_Tag_Environment, "Environment") \
  TAGENTRY(V4IM_Tag_EPeriod, "EPeriod") \
  TAGENTRY(V4IM_Tag_Epsilon, "Epsilon") \
  TAGENTRY(V4IM_Tag_EQ, "EQ") \
  TAGENTRY(V4IM_Tag_Error, "Error") \
  TAGENTRY(V4IM_Tag_Escape, "Escape") \
  TAGENTRY(V4IM_Tag_Evaluate, "Evaluate") \
  TAGENTRY(V4IM_Tag_Evaluations, "Evaluations") \
  TAGENTRY(V4IM_Tag_Every, "Every") \
  TAGENTRY(V4IM_Tag_Exact, "Exact") \
  TAGENTRY(V4IM_Tag_ExcelIni, "ExcelIni") \
  TAGENTRY(V4IM_Tag_Exclude, "Exclude") \
  TAGENTRY(V4IM_Tag_Exists, "Exists") \
  TAGENTRY(V4IM_Tag_Exit, "Exit") \
  TAGENTRY(V4IM_Tag_Exp, "Exp") \
  TAGENTRY(V4IM_Tag_Exp10, "Exp10") \
  TAGENTRY(V4IM_Tag_Exp2, "Exp2") \
  TAGENTRY(V4IM_Tag_Exponential, "Exponential") \
  TAGENTRY(V4IM_Tag_Expression, "Expression") \
  TAGENTRY(V4IM_Tag_Extension, "Extension") \
  TAGENTRY(V4IM_Tag_F, "F") \
  TAGENTRY(V4IM_Tag_Factor, "Factor") \
  TAGENTRY(V4IM_Tag_FAgg, "FAgg") \
  TAGENTRY(V4IM_Tag_Fail, "Fail") \
  TAGENTRY(V4IM_Tag_FDB, "FDB") \
  TAGENTRY(V4IM_Tag_Field, "Field") \
  TAGENTRY(V4IM_Tag_File, "File") \
  TAGENTRY(V4IM_Tag_FileName, "FileName") \
  TAGENTRY(V4IM_Tag_FileRef, "FileRef") \
  TAGENTRY(V4IM_Tag_Files, "Files") \
  TAGENTRY(V4IM_Tag_Fill, "Fill") \
  TAGENTRY(V4IM_Tag_Filter, "Filter") \
  TAGENTRY(V4IM_Tag_First, "First") \
  TAGENTRY(V4IM_Tag_FixedLength, "FixedLength") \
  TAGENTRY(V4IM_Tag_Font, "Font") \
  TAGENTRY(V4IM_Tag_FontSize, "FontSize") \
  TAGENTRY(V4IM_Tag_Footer, "Footer") \
  TAGENTRY(V4IM_Tag_Force, "Force") \
  TAGENTRY(V4IM_Tag_Form, "Form") \
  TAGENTRY(V4IM_Tag_Format, "Format") \
  TAGENTRY(V4IM_Tag_Fractional, "Fractional") \
  TAGENTRY(V4IM_Tag_Frames, "Frames") \
  TAGENTRY(V4IM_Tag_Frequency, "Frequency") \
  TAGENTRY(V4IM_Tag_From, "From") \
  TAGENTRY(V4IM_Tag_FullPathName, "FullPathName") \
  TAGENTRY(V4IM_Tag_Function, "Function") \
  TAGENTRY(V4IM_Tag_FV, "FV") \
  TAGENTRY(V4IM_Tag_Gamma, "Gamma") \
  TAGENTRY(V4IM_Tag_GE, "GE") \
  TAGENTRY(V4IM_Tag_Get, "Get") \
  TAGENTRY(V4IM_Tag_GivenName, "GivenName") \
  TAGENTRY(V4IM_Tag_Grid, "Grid") \
  TAGENTRY(V4IM_Tag_Group, "Group") \
  TAGENTRY(V4IM_Tag_GT, "GT") \
  TAGENTRY(V4IM_Tag_Has, "Has") \
  TAGENTRY(V4IM_Tag_Hash, "Hash") \
  TAGENTRY(V4IM_Tag_Hash32, "Hash32") \
  TAGENTRY(V4IM_Tag_Hash64, "Hash64") \
  TAGENTRY(V4IM_Tag_HashMD5, "HashMD5") \
  TAGENTRY(V4IM_Tag_HasIsa, "HasIsA") \
  TAGENTRY(V4IM_Tag_Head, "Head") \
  TAGENTRY(V4IM_Tag_Header, "Header") \
  TAGENTRY(V4IM_Tag_Heading, "Heading") \
  TAGENTRY(V4IM_Tag_Height, "Height") \
  TAGENTRY(V4IM_Tag_Hidden, "Hidden") \
  TAGENTRY(V4IM_Tag_Hierarchy, "Hierarchy") \
  TAGENTRY(V4IM_Tag_History, "History") \
  TAGENTRY(V4IM_Tag_Hold, "Hold") \
  TAGENTRY(V4IM_Tag_Host, "Host") \
  TAGENTRY(V4IM_Tag_Hour, "Hour") \
  TAGENTRY(V4IM_Tag_HTML, "HTML") \
  TAGENTRY(V4IM_Tag_HTTPGet, "HTTPGet") \
  TAGENTRY(V4IM_Tag_HTTPPut, "HTTPPut") \
  TAGENTRY(V4IM_Tag_HTTP_HTML, "HTTP_HTML") \
  TAGENTRY(V4IM_Tag_HTTP_Other, "HTTP_Other") \
  TAGENTRY(V4IM_Tag_HyperGeo, "HyperGeo") \
  TAGENTRY(V4IM_Tag_IC, "IC") \
  TAGENTRY(V4IM_Tag_Icon, "Icon") \
  TAGENTRY(V4IM_Tag_Id, "Id") \
  TAGENTRY(V4IM_Tag_Idle, "Idle") \
  TAGENTRY(V4IM_Tag_If, "If") \
  TAGENTRY(V4IM_Tag_IFa, "IFa") \
  TAGENTRY(V4IM_Tag_IFb, "IFb") \
  TAGENTRY(V4IM_Tag_IFc, "IFc") \
  TAGENTRY(V4IM_Tag_IfOnce, "IfOnce") \
  TAGENTRY(V4IM_Tag_Ignore, "Ignore") \
  TAGENTRY(V4IM_Tag_Image, "Image") \
  TAGENTRY(V4IM_Tag_Include, "Include") \
  TAGENTRY(V4IM_Tag_Index, "Index") \
  TAGENTRY(V4IM_Tag_Inflation, "Inflation") \
  TAGENTRY(V4IM_Tag_InitialLevel, "InitialLevel") \
  TAGENTRY(V4IM_Tag_Integral, "Integral") \
  TAGENTRY(V4IM_Tag_Intercept, "Intercept") \
  TAGENTRY(V4IM_Tag_Intersect, "Intersect") \
  TAGENTRY(V4IM_Tag_Intersection, "Intersection") \
  TAGENTRY(V4IM_Tag_Interval, "Interval") \
  TAGENTRY(V4IM_Tag_Investment, "Investment") \
  TAGENTRY(V4IM_Tag_IPAddress, "IPAddress") \
  TAGENTRY(V4IM_Tag_IPmt, "IPmt") \
  TAGENTRY(V4IM_Tag_IsA, "IsA") \
  TAGENTRY(V4IM_Tag_Issue, "Issue") \
  TAGENTRY(V4IM_Tag_Java, "Java") \
  TAGENTRY(V4IM_Tag_Javascript, "Javascript") \
  TAGENTRY(V4IM_Tag_Job, "Job") \
  TAGENTRY(V4IM_Tag_JSON, "JSON") \
  TAGENTRY(V4IM_Tag_Key, "Key") \
  TAGENTRY(V4IM_Tag_Key2, "Key2") \
  TAGENTRY(V4IM_Tag_KeyNum, "KeyNum") \
  TAGENTRY(V4IM_Tag_Label, "Label") \
  TAGENTRY(V4IM_Tag_Lambda, "Lambda") \
  TAGENTRY(V4IM_Tag_Language, "Language") \
  TAGENTRY(V4IM_Tag_Last, "Last") \
  TAGENTRY(V4IM_Tag_Lazy, "Lazy") \
  TAGENTRY(V4IM_Tag_LC, "LC") \
  TAGENTRY(V4IM_Tag_LE, "LE") \
  TAGENTRY(V4IM_Tag_Leave, "Leave") \
  TAGENTRY(V4IM_Tag_Left, "Left") \
  TAGENTRY(V4IM_Tag_Length, "Length") \
  TAGENTRY(V4IM_Tag_Level, "Level") \
  TAGENTRY(V4IM_Tag_Library, "Library") \
  TAGENTRY(V4IM_Tag_Life, "Life") \
  TAGENTRY(V4IM_Tag_Line, "Line") \
  TAGENTRY(V4IM_Tag_Linear, "Linear") \
  TAGENTRY(V4IM_Tag_Link, "Link") \
  TAGENTRY(V4IM_Tag_Listen, "Listen") \
  TAGENTRY(V4IM_Tag_ListOf, "ListOf") \
  TAGENTRY(V4IM_Tag_Literal, "Literal") \
  TAGENTRY(V4IM_Tag_LMargin, "LMargin") \
  TAGENTRY(V4IM_Tag_Local, "Local") \
  TAGENTRY(V4IM_Tag_Lock, "Lock") \
  TAGENTRY(V4IM_Tag_Log, "Log") \
  TAGENTRY(V4IM_Tag_Log10, "Log10") \
  TAGENTRY(V4IM_Tag_Log2, "Log2") \
  TAGENTRY(V4IM_Tag_Logical, "Logical") \
  TAGENTRY(V4IM_Tag_LogNorm, "LogNorm") \
  TAGENTRY(V4IM_Tag_LogoImage, "LogoImage") \
  TAGENTRY(V4IM_Tag_LogoURL, "LogoURL") \
  TAGENTRY(V4IM_Tag_LT, "LT") \
  TAGENTRY(V4IM_Tag_LunarAltitude, "LunarAltitude") \
  TAGENTRY(V4IM_Tag_LunarPhase, "LunarPhase") \
  TAGENTRY(V4IM_Tag_Macaulay, "Macaulay") \
  TAGENTRY(V4IM_Tag_Macro, "Macro") \
  TAGENTRY(V4IM_Tag_Mask, "Mask") \
  TAGENTRY(V4IM_Tag_MatchUndef, "MatchUndef") \
  TAGENTRY(V4IM_Tag_Maturity, "Maturity") \
  TAGENTRY(V4IM_Tag_Maximum, "Max") \
  TAGENTRY(V4IM_Tag_Maximum, "Maximum") \
  TAGENTRY(V4IM_Tag_Mean, "Mean") \
  TAGENTRY(V4IM_Tag_Memo, "Memo") \
  TAGENTRY(V4IM_Tag_Menu1, "Menu1") \
  TAGENTRY(V4IM_Tag_Menu2, "Menu2") \
  TAGENTRY(V4IM_Tag_Message, "Message") \
  TAGENTRY(V4IM_Tag_Method, "Method") \
  TAGENTRY(V4IM_Tag_Metric, "Metric") \
  TAGENTRY(V4IM_Tag_MiddleName, "MiddleName") \
  TAGENTRY(V4IM_Tag_Minimum, "Min") \
  TAGENTRY(V4IM_Tag_Minimum, "Minimum") \
  TAGENTRY(V4IM_Tag_Minus, "Minus") \
  TAGENTRY(V4IM_Tag_Minute, "Minute") \
  TAGENTRY(V4IM_Tag_Missing, "Missing") \
  TAGENTRY(V4IM_Tag_MMDDYY, "MMDDYY") \
  TAGENTRY(V4IM_Tag_MMYY, "MMYY") \
  TAGENTRY(V4IM_Tag_Modified, "Modified") \
  TAGENTRY(V4IM_Tag_Module, "Module") \
  TAGENTRY(V4IM_Tag_Month, "Month") \
  TAGENTRY(V4IM_Tag_Move, "Move") \
  TAGENTRY(V4IM_Tag_Multiple, "Multiple") \
  TAGENTRY(V4IM_Tag_Name, "Name") \
  TAGENTRY(V4IM_Tag_NE, "NE") \
  TAGENTRY(V4IM_Tag_Negate, "Negate") \
  TAGENTRY(V4IM_Tag_NegBinom, "NegBinom") \
  TAGENTRY(V4IM_Tag_Nest, "Nest") \
  TAGENTRY(V4IM_Tag_EvalNest, "NestEval") \
  TAGENTRY(V4IM_Tag_NestRCV, "NestRCV") \
  TAGENTRY(V4IM_Tag_New, "New") \
  TAGENTRY(V4IM_Tag_Next, "Next") \
  TAGENTRY(V4IM_Tag_NickName, "NickName") \
  TAGENTRY(V4IM_Tag_Nodes, "Nodes") \
  TAGENTRY(V4IM_Tag_NoError, "NoError") \
  TAGENTRY(V4IM_Tag_Nominal, "Nominal") \
  TAGENTRY(V4IM_Tag_None, "None") \
  TAGENTRY(V4IM_Tag_NoPrefix, "NoPrefix") \
  TAGENTRY(V4IM_Tag_Normal, "Normal") \
  TAGENTRY(V4IM_Tag_Normalize, "Normalize") \
  TAGENTRY(V4IM_Tag_NormalStd, "NormalStd") \
  TAGENTRY(V4IM_Tag_Not, "Not") \
  TAGENTRY(V4IM_Tag_Note, "Note") \
  TAGENTRY(V4IM_Tag_Nth, "Nth") \
  TAGENTRY(V4IM_Tag_Num, "Num") \
  TAGENTRY(V4IM_Tag_Number, "Number") \
  TAGENTRY(V4IM_Tag_NumF, "NumF") \
  TAGENTRY(V4IM_Tag_NumP, "NumP") \
  TAGENTRY(V4IM_Tag_NumS, "NumS") \
  TAGENTRY(V4IM_Tag_NumSM, "NumSM") \
  TAGENTRY(V4IM_Tag_Object, "Object") \
  TAGENTRY(V4IM_Tag_Occurs, "Occurs") \
  TAGENTRY(V4IM_Tag_OddFirst, "OddFirst") \
  TAGENTRY(V4IM_Tag_OddLast, "OddLast") \
  TAGENTRY(V4IM_Tag_Offset, "Offset") \
  TAGENTRY(V4IM_Tag_OneWay, "OneWay") \
  TAGENTRY(V4IM_Tag_Open, "Open") \
  TAGENTRY(V4IM_Tag_Optimize, "Optimize") \
  TAGENTRY(V4IM_Tag_Or, "Or") \
  TAGENTRY(V4IM_Tag_OS, "OS") \
  TAGENTRY(V4IM_Tag_OSFileInfo, "OSFileInfo") \
  TAGENTRY(V4IM_Tag_OSFileListOf, "OSFileListOf") \
  TAGENTRY(V4IM_Tag_Other, "Other") \
  TAGENTRY(V4IM_Tag_Out, "Out") \
  TAGENTRY(V4IM_Tag_Overflow, "Overflow") \
  TAGENTRY(V4IM_Tag_Overload, "Overload") \
  TAGENTRY(V4IM_Tag_Page, "Page") \
  TAGENTRY(V4IM_Tag_PageBreak, "PageBreak") \
  TAGENTRY(V4IM_Tag_Pairs, "Pairs") \
  TAGENTRY(V4IM_Tag_Par, "Par") \
  TAGENTRY(V4IM_Tag_Parent, "Parent") \
  TAGENTRY(V4IM_Tag_Parse, "Parse") \
  TAGENTRY(V4IM_Tag_Partial, "Partial") \
  TAGENTRY(V4IM_Tag_Passover, "Passover") \
  TAGENTRY(V4IM_Tag_Password, "Password") \
  TAGENTRY(V4IM_Tag_Path, "Path") \
  TAGENTRY(V4IM_Tag_Patronymic, "Patronymic") \
  TAGENTRY(V4IM_Tag_Peer, "Peer") \
  TAGENTRY(V4IM_Tag_Period, "Period") \
  TAGENTRY(V4IM_Tag_Periodic, "Periodic") \
  TAGENTRY(V4IM_Tag_Periods, "Periods") \
  TAGENTRY(V4IM_Tag_PIf, "PIf") \
  TAGENTRY(V4IM_Tag_Plus, "Plus") \
  TAGENTRY(V4IM_Tag_Pmt, "Pmt") \
  TAGENTRY(V4IM_Tag_Point, "Point") \
  TAGENTRY(V4IM_Tag_Poisson, "Poisson") \
  TAGENTRY(V4IM_Tag_Pop, "Pop") \
  TAGENTRY(V4IM_Tag_Port, "Port") \
  TAGENTRY(V4IM_Tag_Portrait, "Portrait") \
  TAGENTRY(V4IM_Tag_Position, "Position") \
  TAGENTRY(V4IM_Tag_POSIX, "POSIX") \
  TAGENTRY(V4IM_Tag_Post, "Post") \
  TAGENTRY(V4IM_Tag_POW, "POW") \
  TAGENTRY(V4IM_Tag_PPmt, "PPmt") \
  TAGENTRY(V4IM_Tag_Precision, "Precision") \
  TAGENTRY(V4IM_Tag_Prefetch, "Prefetch") \
  TAGENTRY(V4IM_Tag_Price, "Price") \
  TAGENTRY(V4IM_Tag_Prime, "Prime") \
  TAGENTRY(V4IM_Tag_Privileges, "Privileges") \
  TAGENTRY(V4IM_Tag_Prob, "Prob") \
  TAGENTRY(V4IM_Tag_Problem, "Problem") \
  TAGENTRY(V4IM_Tag_ProbS, "ProbS") \
  TAGENTRY(V4IM_Tag_Processors, "Processors") \
  TAGENTRY(V4IM_Tag_Product, "Product") \
  TAGENTRY(V4IM_Tag_Progress, "Progress") \
  TAGENTRY(V4IM_Tag_Project, "Project") \
  TAGENTRY(V4IM_Tag_ProjectArea, "ProjectArea") \
  TAGENTRY(V4IM_Tag_Push, "Push") \
  TAGENTRY(V4IM_Tag_Put, "Put") \
  TAGENTRY(V4IM_Tag_PV, "PV") \
  TAGENTRY(V4IM_Tag_Quote, "Quote") \
  TAGENTRY(V4IM_Tag_Quoted, "Quoted") \
  TAGENTRY(V4IM_Tag_Radians, "Radians") \
  TAGENTRY(V4IM_Tag_Random, "Random") \
  TAGENTRY(V4IM_Tag_Range, "Range") \
  TAGENTRY(V4IM_Tag_Rate, "Rate") \
  TAGENTRY(V4IM_Tag_RCV, "RCV") \
  TAGENTRY(V4IM_Tag_RDB, "RDB") \
  TAGENTRY(V4IM_Tag_RDBId, "RDBId") \
  TAGENTRY(V4IM_Tag_RDBw, "RDBw") \
  TAGENTRY(V4IM_Tag_Rebuild,"Rebuild") \
  TAGENTRY(V4IM_Tag_Recap, "Recap") \
  TAGENTRY(V4IM_Tag_Redemption, "Redemption") \
  TAGENTRY(V4IM_Tag_Redirection, "Redirection") \
  TAGENTRY(V4IM_Tag_References, "References") \
  TAGENTRY(V4IM_Tag_RegExp, "RegExp") \
  TAGENTRY(V4IM_Tag_Release, "Release") \
  TAGENTRY(V4IM_Tag_Remote, "Remote") \
  TAGENTRY(V4IM_Tag_Remove, "Remove") \
  TAGENTRY(V4IM_Tag_Rename, "Rename") \
  TAGENTRY(V4IM_Tag_Replace, "Replace") \
  TAGENTRY(V4IM_Tag_Reply, "Reply") \
  TAGENTRY(V4IM_Tag_Reset, "Reset") \
  TAGENTRY(V4IM_Tag_Result, "Result") \
  TAGENTRY(V4IM_Tag_Retry, "Retry") \
  TAGENTRY(V4IM_Tag_Return, "Return") \
  TAGENTRY(V4IM_Tag_Reverse, "Reverse") \
  TAGENTRY(V4IM_Tag_Right, "Right") \
  TAGENTRY(V4IM_Tag_RMargin, "RMargin") \
  TAGENTRY(V4IM_Tag_Root, "Root") \
  TAGENTRY(V4IM_Tag_Rotate, "Rotate") \
  TAGENTRY(V4IM_Tag_Round, "Round") \
  TAGENTRY(V4IM_Tag_Route, "Route") \
  TAGENTRY(V4IM_Tag_Row, "Row") \
  TAGENTRY(V4IM_Tag_RowCap, "RowCap") \
  TAGENTRY(V4IM_Tag_RowColor, "RowColor") \
  TAGENTRY(V4IM_Tag_RowGap, "RowGap") \
  TAGENTRY(V4IM_Tag_Rows, "Rows") \
  TAGENTRY(V4IM_Tag_Ruler, "Ruler") \
  TAGENTRY(V4IM_Tag_Salvage, "Salvage") \
  TAGENTRY(V4IM_Tag_Sample, "Sample") \
  TAGENTRY(V4IM_Tag_Samples, "Samples") \
  TAGENTRY(V4IM_Tag_Scale, "Scale") \
  TAGENTRY(V4IM_Tag_Second, "Second") \
  TAGENTRY(V4IM_Tag_Section, "Section") \
  TAGENTRY(V4IM_Tag_Seed, "Seed") \
  TAGENTRY(V4IM_Tag_Segment, "Segment") \
  TAGENTRY(V4IM_Tag_Select, "Select") \
  TAGENTRY(V4IM_Tag_Send, "Send") \
  TAGENTRY(V4IM_Tag_Sequence, "Sequence") \
  TAGENTRY(V4IM_Tag_Sequential,"Sequential") \
  TAGENTRY(V4IM_Tag_Server, "Server") \
  TAGENTRY(V4IM_Tag_Session, "Session") \
  TAGENTRY(V4IM_Tag_SessionKey, "SessionKey") \
  TAGENTRY(V4IM_Tag_Set, "Set") \
  TAGENTRY(V4IM_Tag_SetOf, "SetOf") \
  TAGENTRY(V4IM_Tag_Settlement, "Settlement") \
  TAGENTRY(V4IM_Tag_Setup, "Setup") \
  TAGENTRY(V4IM_Tag_Share, "Share") \
  TAGENTRY(V4IM_Tag_Sheet, "Sheet") \
  TAGENTRY(V4IM_Tag_Shell, "Shell") \
  TAGENTRY(V4IM_Tag_Shift, "Shift") \
  TAGENTRY(V4IM_Tag_Sign, "Sign") \
  TAGENTRY(V4IM_Tag_Sin, "Sin") \
  TAGENTRY(V4IM_Tag_SinH, "SinH") \
  TAGENTRY(V4IM_Tag_Skip, "Skip") \
  TAGENTRY(V4IM_Tag_Sleep, "Sleep") \
  TAGENTRY(V4IM_Tag_SLN, "SLN") \
  TAGENTRY(V4IM_Tag_Slope, "Slope") \
  TAGENTRY(V4IM_Tag_SolarLongitude, "SolarLongitude") \
  TAGENTRY(V4IM_Tag_Sort, "Sort") \
  TAGENTRY(V4IM_Tag_Soundex, "Soundex") \
  TAGENTRY(V4IM_Tag_Source, "Source") \
  TAGENTRY(V4IM_Tag_SourceFile, "SourceFile") \
  TAGENTRY(V4IM_Tag_Spawn, "Spawn") \
  TAGENTRY(V4IM_Tag_Special, "Special") \
  TAGENTRY(V4IM_Tag_SPeriod, "SPeriod") \
  TAGENTRY(V4IM_Tag_Split, "Split") \
  TAGENTRY(V4IM_Tag_SQL, "SQL") \
  TAGENTRY(V4IM_Tag_Stack, "Stack") \
  TAGENTRY(V4IM_Tag_Start, "Start") \
  TAGENTRY(V4IM_Tag_Statement, "Statement") \
  TAGENTRY(V4IM_Tag_StdDev, "StdDev") \
  TAGENTRY(V4IM_Tag_StdIn, "StdIn") \
  TAGENTRY(V4IM_Tag_Structure, "Structure") \
  TAGENTRY(V4IM_Tag_StudentT, "StudentT") \
  TAGENTRY(V4IM_Tag_Style, "Style") \
  TAGENTRY(V4IM_Tag_StyleName, "StyleName") \
  TAGENTRY(V4IM_Tag_Sub, "Sub") \
  TAGENTRY(V4IM_Tag_Subject, "Subject") \
  TAGENTRY(V4IM_Tag_Submit, "Submit") \
  TAGENTRY(V4IM_Tag_Subtract, "Subtract") \
  TAGENTRY(V4IM_Tag_Suffix, "Suffix") \
  TAGENTRY(V4IM_Tag_Sum, "Sum") \
  TAGENTRY(V4IM_Tag_Surname, "Surname") \
  TAGENTRY(V4IM_Tag_Swap, "Swap") \
  TAGENTRY(V4IM_Tag_SYD, "SYD") \
  TAGENTRY(V4IM_Tag_Table, "Table") \
  TAGENTRY(V4IM_Tag_Tag, "Tag") \
  TAGENTRY(V4IM_Tag_Tail, "Tail") \
  TAGENTRY(V4IM_Tag_Tan, "Tan") \
  TAGENTRY(V4IM_Tag_TanH, "TanH") \
  TAGENTRY(V4IM_Tag_Target, "Target") \
  TAGENTRY(V4IM_Tag_TaxRate, "TaxRate") \
  TAGENTRY(V4IM_Tag_Test, "Test") \
  TAGENTRY(V4IM_Tag_Text, "Text") \
  TAGENTRY(V4IM_Tag_TextColor, "TextColor") \
  TAGENTRY(V4IM_Tag_Then, "Then") \
  TAGENTRY(V4IM_Tag_Threads, "Threads") \
  TAGENTRY(V4IM_Tag_Threshold, "Threshold") \
  TAGENTRY(V4IM_Tag_Throw, "Throw") \
  TAGENTRY(V4IM_Tag_TimeZone, "TimeZone") \
  TAGENTRY(V4IM_Tag_Title, "Title") \
  TAGENTRY(V4IM_Tag_TMargin, "TMargin") \
  TAGENTRY(V4IM_Tag_To, "To") \
  TAGENTRY(V4IM_Tag_Token, "Token") \
  TAGENTRY(V4IM_Tag_Top, "Top") \
  TAGENTRY(V4IM_Tag_Trace, "Trace") \
  TAGENTRY(V4IM_Tag_Tree, "Tree") \
  TAGENTRY(V4IM_Tag_Trials, "Trials") \
  TAGENTRY(V4IM_Tag_Trim, "Trim") \
  TAGENTRY(V4IM_Tag_Try, "Try") \
  TAGENTRY(V4IM_Tag_TTL, "TTL") \
  TAGENTRY(V4IM_Tag_Type, "Type") \
  TAGENTRY(V4IM_Tag_UC, "UC") \
  TAGENTRY(V4IM_Tag_UCount, "UCount") \
  TAGENTRY(V4IM_Tag_UDate, "UDate") \
  TAGENTRY(V4IM_Tag_UMonth, "UMonth") \
  TAGENTRY(V4IM_Tag_Union, "Union") \
  TAGENTRY(V4IM_Tag_Unique, "UNIQUE") \
  TAGENTRY(V4IM_Tag_Unlock, "Unlock") \
  TAGENTRY(V4IM_Tag_Until, "Until") \
  TAGENTRY(V4IM_Tag_UOM, "UOM") \
  TAGENTRY(V4IM_Tag_UOMId, "UomID") \
  TAGENTRY(V4IM_Tag_Update, "Update") \
  TAGENTRY(V4IM_Tag_UPeriod, "UPeriod") \
  TAGENTRY(V4IM_Tag_UQuarter, "UQuarter") \
  TAGENTRY(V4IM_Tag_URL, "URL") \
  TAGENTRY(V4IM_Tag_URL2, "URL2") \
  TAGENTRY(V4IM_Tag_URL3, "URL3") \
  TAGENTRY(V4IM_Tag_URL4, "URL4") \
  TAGENTRY(V4IM_Tag_URL5, "URL5") \
  TAGENTRY(V4IM_Tag_URLBase, "URLBase") \
  TAGENTRY(V4IM_Tag_URLBase2, "URLBase2") \
  TAGENTRY(V4IM_Tag_URLBase3, "URLBase3") \
  TAGENTRY(V4IM_Tag_URLBase4, "URLBase4") \
  TAGENTRY(V4IM_Tag_URLBase5, "URLBase5") \
  TAGENTRY(V4IM_Tag_User, "User") \
  TAGENTRY(V4IM_Tag_UserAgent, "UserAgent") \
  TAGENTRY(V4IM_Tag_UserOptions, "UserOptions") \
  TAGENTRY(V4IM_Tag_UTC, "UTC") \
  TAGENTRY(V4IM_Tag_UTF8, "UTF8") \
  TAGENTRY(V4IM_Tag_UTime, "UTime") \
  TAGENTRY(V4IM_Tag_UWeek, "UWeek") \
  TAGENTRY(V4IM_Tag_UYear, "UYear") \
  TAGENTRY(V4IM_Tag_Value, "Value") \
  TAGENTRY(V4IM_Tag_Values, "Values") \
  TAGENTRY(V4IM_Tag_Version, "Version") \
  TAGENTRY(V4IM_Tag_Volume, "Volume") \
  TAGENTRY(V4IM_Tag_Wait, "Wait") \
  TAGENTRY(V4IM_Tag_Warning, "Warning") \
  TAGENTRY(V4IM_Tag_WBind, "WBind") \
  TAGENTRY(V4IM_Tag_Web, "Web") \
  TAGENTRY(V4IM_Tag_Week, "Week") \
  TAGENTRY(V4IM_Tag_Weibull, "Weibull") \
  TAGENTRY(V4IM_Tag_Weight, "Weight") \
  TAGENTRY(V4IM_Tag_While, "While") \
  TAGENTRY(V4IM_Tag_Width, "Width") \
  TAGENTRY(V4IM_Tag_Window, "Window") \
  TAGENTRY(V4IM_Tag_With, "With") \
  TAGENTRY(V4IM_Tag_Word, "Word") \
  TAGENTRY(V4IM_Tag_X, "X") \
  TAGENTRY(V4IM_Tag_XDBAccess, "XDBAccess") \
  TAGENTRY(V4IM_Tag_XDBXCt, "XDBXct") \
  TAGENTRY(V4IM_Tag_XML, "XML") \
  TAGENTRY(V4IM_Tag_XOR, "XOR") \
  TAGENTRY(V4IM_Tag_XUnion, "XUnion") \
  TAGENTRY(V4IM_Tag_Y, "Y") \
  TAGENTRY(V4IM_Tag_Year, "Year") \
  TAGENTRY(V4IM_Tag_Yield, "Yield") \
  TAGENTRY(V4IM_Tag_YMDOrder, "YMDOrder") \
  TAGENTRY(V4IM_Tag_YomKippur, "YomKippur") \
  TAGENTRY(V4IM_Tag_Z, "Z") \
  TAGENTRY(V4IM_Tag_Zip, "Zip") \
  TAGENTRY(V4IM_Tag_List, "[a list point]") \
  TAGENTRY(V4IM_Tag_QIsct, "[an intersection]") \

#define V4IMTag_TagCount 621

#define TAGENTRY(int, str) { int , UClit(str) },

struct V4IM__TagEntries {
  int Value ;			/* Tag value: V4IMTag_xxx */
//  char UCName[20] ;		/* Tag name (upper case) */
  UCCHAR ULName[20] ;		/*  & upper/lower case version */
 } ;
struct V4IM__TagEntries Tags[] = { TAGLIST } ;

#endif /* V4IMTags */


#define OCLIST \
	OCENTRY(V4IM_OpCode_Abs, "Abs", 1 ) \
	OCENTRY(V4IM_OpCode_AggDel, "AggDel", 1) \
	OCENTRY(V4IM_OpCode_AggPut, "AggPut", 2 ) \
	OCENTRY(V4IM_OpCode_AggUpd, "AggUpd", 4) \
	OCENTRY(V4IM_OpCode_AggVal, "AggVal", 2) \
	OCENTRY(V4IM_OpCode_And, "And" , 0 ) \
	OCENTRY(V4IM_OpCode_Area, "Area", 1) \
	OCENTRY(V4IM_OpCode_Array, "Array", 1 ) \
	OCENTRY(V4IM_OpCode_Average, "Average", 0) \
	OCENTRY(V4IM_OpCode_Average, "Avg", 0) \
	OCENTRY(V4IM_OpCode_BindEE, "BindEE", 2 ) \
	OCENTRY(V4IM_OpCode_BindEQ, "BindEQ", 2 ) \
	OCENTRY(V4IM_OpCode_BindQE, "BindQE", 2 ) \
	OCENTRY(V4IM_OpCode_BindQQ, "BindQQ", 2 ) \
	OCENTRY(V4IM_OpCode_Bits, "Bits", 2 ) \
	OCENTRY(V4IM_OpCode_ChemOpt, "ChemOpt", 2) \
	OCENTRY(V4IM_OpCode_Coerce, "Coerce", 2) \
	OCENTRY(V4IM_OpCode_Context, "Context", 0 ) \
	OCENTRY(V4IM_OpCode_ContextL, "ContextL", 0 ) \
	OCENTRY(V4IM_OpCode_Counter, "Counter", 1) \
	OCENTRY(V4IM_OpCode_db, "DB", 1 ) \
	OCENTRY(V4IM_OpCode_dbConnect, "dbConnect", 2 ) \
	OCENTRY(V4IM_OpCode_ODBCError, "dbError", 0) \
	OCENTRY(V4IM_OpCode_dbFree, "dbFree", 0 ) \
	OCENTRY(V4IM_OpCode_Dbg, "Dbg", 0 ) \
	OCENTRY(V4IM_OpCode_dbGet, "DBGet", 1 ) \
	OCENTRY(V4IM_OpCode_dbInfo, "DBInfo", 1 ) \
	OCENTRY(V4IM_OpCode_dbVal, "DBVal", 2 ) \
	OCENTRY(V4IM_OpCode_ODBCXct, "DBXct", 1 ) \
	OCENTRY(V4IM_OpCode_DDiv, "DDiv", 2 ) \
	OCENTRY(V4IM_OpCode_Def, "Def" , 1 ) \
	OCENTRY(V4IM_OpCode_DeferDo, "DeferDo" , 1 ) \
	OCENTRY(V4IM_OpCode_DefQ, "DefQ" , 1 ) \
	OCENTRY(V4IM_OpCode_Dim, "Dim", 1) \
	OCENTRY(V4IM_OpCode_Div, "Div", 2 ) \
	OCENTRY(V4IM_OpCode_DMCluster, "DMCluster", 3 ) \
	OCENTRY(V4IM_OpCode_Do, "Do", 1) \
	OCENTRY(V4IM_OpCode_Drawer, "Drawer", 1 ) \
	OCENTRY(V4IM_OpCode_DTInfo, "DTInfo", 1) \
	OCENTRY(V4IM_OpCode_Echo, "Echo" , 0 ) \
	OCENTRY(V4IM_OpCode_EchoA, "EchoA", 1 ) \
	OCENTRY(V4IM_OpCode_EchoD, "EchoD" , 0 ) \
	OCENTRY(V4IM_OpCode_EchoE, "EchoE", 1) \
	OCENTRY(V4IM_OpCode_EchoN, "EchoN" , 0 ) \
	OCENTRY(V4IM_OpCode_EchoP, "EchoP" , 0 ) \
	OCENTRY(V4IM_OpCode_EchoS, "EchoS", 0) \
	OCENTRY(V4IM_OpCode_EchoT, "EchoT" , 0 ) \
	OCENTRY(V4IM_OpCode_EchoW, "EchoW", 1 ) \
	OCENTRY(V4IM_OpCode_Enum, "Enum", 1 ) \
	OCENTRY(V4IM_OpCode_EnumCL, "EnumCL", 2 ) \
	OCENTRY(V4IM_OpCode_EnumX, "EnumX", 2) \
	OCENTRY(V4IM_OpCode_EQ, "EQ" , 2 ) \
	OCENTRY(V4IM_OpCode_EQk, "EQk", 2 ) \
	OCENTRY(V4IM_OpCode_Error, "Error", 0) \
	OCENTRY(V4IM_OpCode_Eval, "Eval", 1 ) \
	OCENTRY(V4IM_OpCode_EvalArithExp, "EvalAE" , 1 ) \
	OCENTRY(V4IM_OpCode_EvalBL, "EvalBL", 1) \
	OCENTRY(V4IM_OpCode_EvalCmd, "EvalCmd" , 0 ) \
	OCENTRY(V4IM_OpCode_EvalLE, "EvalLE" , 1 ) \
	OCENTRY(V4IM_OpCode_EvalPt, "EvalPt", 1) \
	OCENTRY(V4IM_OpCode_EvalUM, "EvalUM", 1 ) \
	OCENTRY(V4IM_OpCode_Fail, "Fail", 0) \
	OCENTRY(V4IM_OpCode_FinCoupon, "FinCoupon", 3 ) \
	OCENTRY(V4IM_OpCode_FinCvtDec, "FinCvtDec", 2 ) \
	OCENTRY(V4IM_OpCode_FinCvtFrac, "FinCvtFrac", 2 ) \
	OCENTRY(V4IM_OpCode_FinDays360, "FinDays360", 2 ) \
	OCENTRY(V4IM_OpCode_FinDep, "FinDep", 3 ) \
	OCENTRY(V4IM_OpCode_FinDisc, "FinDisc", 4 ) \
	OCENTRY(V4IM_OpCode_FinFVSched, "FinFVSched", 2 ) \
	OCENTRY(V4IM_OpCode_FinIntRate, "FinIntRate", 2 ) \
	OCENTRY(V4IM_OpCode_FinIRR, "FinIRR", 1 ) \
	OCENTRY(V4IM_OpCode_FinNPV, "FinNPV", 2 ) \
	OCENTRY(V4IM_OpCode_FinPrice, "FinPrice", 6 ) \
	OCENTRY(V4IM_OpCode_FinSecDisc, "FinSecDisc", 5 ) \
	OCENTRY(V4IM_OpCode_FinSecDur, "FinSecDur", 5 ) \
	OCENTRY(V4IM_OpCode_FinSecInt, "FinSecInt", 5 ) \
	OCENTRY(V4IM_OpCode_FinSecPrice, "FinSecPrice", 5 ) \
	OCENTRY(V4IM_OpCode_FinSecRcvd, "FinSecRcvd", 5 ) \
	OCENTRY(V4IM_OpCode_FinSecYield, "FinSecYield", 5 ) \
	OCENTRY(V4IM_OpCode_FinTBill, "FinTBill", 3 ) \
	OCENTRY(V4IM_OpCode_FinTVM, "FinTVM", 3 ) \
	OCENTRY(V4IM_OpCode_Flatten, "Flatten", 2) \
	OCENTRY(V4IM_OpCode_Format, "Format", 2) \
	OCENTRY(V4IM_OpCode_FTP, "FTP", 3) \
	OCENTRY(V4IM_OpCode_GE, "GE" , 2 ) \
	OCENTRY(V4IM_OpCode_Geo, "Geo", 2) \
	OCENTRY(V4IM_OpCode_GraphConnect, "GraphConnect", 2) \
	OCENTRY(V4IM_OpCode_GT, "GT" , 2 ) \
	OCENTRY(V4IM_OpCode_GuiAlert, "GuiAlert", 1) \
	OCENTRY(V4IM_OpCode_GuiMsgBox, "GuiMsgBox", 1) \
	OCENTRY(V4IM_OpCode_HTTP, "HTTP", 1) \
	OCENTRY(V4IM_OpCode_If, "If" , 2 ) \
	OCENTRY(V4IM_OpCode_Ifnot, "Ifnot" , 2 ) \
	OCENTRY(V4IM_OpCode_In, "In", 2) \
	OCENTRY(V4IM_OpCode_Is1, "Is1", 1 ) \
	OCENTRY(V4IM_OpCode_IsAll, "IsAll", 1 ) \
	OCENTRY(V4IM_OpCode_IsctVals, "IsctVals", 1 ) \
	OCENTRY(V4IM_OpCode_JSON, "JSON", 1) \
	OCENTRY(V4IM_OpCode_JSONRef, "JSONRef", 1) \
	OCENTRY(V4IM_OpCode_JSONRefDim, "JSONRefDim", 1) \
	OCENTRY(V4IM_OpCode_LE, "LE" , 2 ) \
	OCENTRY(V4IM_OpCode_LEG, "LEG", 5) \
	OCENTRY(V4IM_OpCode_Len, "Len", 1) \
	OCENTRY(V4IM_OpCode_List, "List", 1 ) \
	OCENTRY(V4IM_OpCode_ListNE, "ListNE", 1 ) \
	OCENTRY(V4IM_OpCode_ListSize, "ListSize" , 1 ) \
	OCENTRY(V4IM_OpCode_LList, "LList", 2 ) \
	OCENTRY(V4IM_OpCode_Locale, "Locale", 1 ) \
	OCENTRY(V4IM_OpCode_Lock, "Lock", 1) \
	OCENTRY(V4IM_OpCode_Log, "Log", 1 ) \
	OCENTRY(V4IM_OpCode_LT, "LT" , 2 ) \
	OCENTRY(V4IM_OpCode_Macro, "Macro", 1) \
	OCENTRY(V4IM_OpCode_MakeLC, "MakeCL", 2) \
	OCENTRY(V4IM_OpCode_MakeIsct, "MakeI", 0) \
	OCENTRY(V4IM_OpCode_MakeIn, "MakeIn", 2) \
	OCENTRY(V4IM_OpCode_MakeL, "MakeL", 0) \
	OCENTRY(V4IM_OpCode_MakeLC, "MakeLC", 2) \
	OCENTRY(V4IM_OpCode_MakeP, "MakeP", 1) \
	OCENTRY(V4IM_OpCode_MakePm, "MakePm", 2) \
	OCENTRY(V4IM_OpCode_MakeQIsct, "MakeQI", 0) \
	OCENTRY(V4IM_OpCode_MakeQL, "MakeQL", 0) \
	OCENTRY(V4IM_OpCode_MakeT, "MakeT", 1 ) \
	OCENTRY(V4IM_OpCode_Max, "Max", 1 ) \
	OCENTRY(V4IM_OpCode_Max, "Maximum", 1 ) \
	OCENTRY(V4IM_OpCode_Message, "Message", 1) \
	OCENTRY(V4IM_OpCode_Min, "Min", 1 ) \
	OCENTRY(V4IM_OpCode_Min, "Minimum", 1 ) \
	OCENTRY(V4IM_OpCode_Minus, "Minus", 1 ) \
	OCENTRY(V4IM_OpCode_MT, "MT", 0 ) \
	OCENTRY(V4IM_OpCode_MthMod, "MthMod", 2 ) \
	OCENTRY(V4IM_OpCode_Mult, "Mult", 2 ) \
	OCENTRY(V4IM_OpCode_Names, "Names", 2 ) \
	OCENTRY(V4IM_OpCode_NDefQ, "nDefQ" , 1 ) \
	OCENTRY(V4IM_OpCode_NE, "NE" , 2 ) \
	OCENTRY(V4IM_OpCode_NEk, "NEk" , 2 ) \
	OCENTRY(V4IM_OpCode_NGram, "NGram", 1) \
	OCENTRY(V4IM_OpCode_NIn, "nIn", 2) \
	OCENTRY(V4IM_OpCode_nMT, "nMT", 0 ) \
	OCENTRY(V4IM_OpCode_NOp, "NOp", 0 ) \
	OCENTRY(V4IM_OpCode_Not, "Not" , 1 ) \
	OCENTRY(V4IM_OpCode_nSame, "nSame", 2) \
	OCENTRY(V4IM_OpCode_nStr, "nStr", 1) \
	OCENTRY(V4IM_OpCode_Num, "Num", 1) \
	OCENTRY(V4IM_OpCode_dbConnect, "ODBCConnect", 2 ) \
	OCENTRY(V4IM_OpCode_ODBCError, "ODBCError", 0) \
	OCENTRY(V4IM_OpCode_dbFree, "ODBCFree", 0 ) \
	OCENTRY(V4IM_OpCode_dbGet, "ODBCGet", 1 ) \
	OCENTRY(V4IM_OpCode_dbInfo, "ODBCInfo", 1 ) \
	OCENTRY(V4IM_OpCode_dbVal, "ODBCVal", 2 ) \
	OCENTRY(V4IM_OpCode_ODBCXct, "ODBCXct", 1 ) \
	OCENTRY(V4IM_OpCode_Optimize, "Optimize", 2) \
	OCENTRY(V4IM_OpCode_Or, "Or" , 0 ) \
	OCENTRY(V4IM_OpCode_OSExt, "OSExt", 1) \
	OCENTRY(V4IM_OpCode_OSFile, "OSFile", 1) \
	OCENTRY(V4IM_OpCode_OSInfo, "OSInfo", 1 ) \
	OCENTRY(V4IM_OpCode_Output, "Output", 1) \
	OCENTRY(V4IM_OpCode_Pack1616, "Pack1616", 2) \
	OCENTRY(V4IM_OpCode_Parse, "Parse", 1) \
	OCENTRY(V4IM_OpCode_PCChg1, "PCChg1", 2 ) \
	OCENTRY(V4IM_OpCode_PCChg2, "PCChg2", 2 ) \
	OCENTRY(V4IM_OpCode_Percent, "Percent", 2 ) \
	OCENTRY(V4IM_OpCode_Plus, "Plus", 0 ) \
	OCENTRY(V4IM_OpCode_Project, "Project", 2) \
	OCENTRY(V4IM_OpCode_PRUpd, "PRUpd", 2) \
	OCENTRY(V4IM_OpCode_Quote, "Quote", 1) \
	OCENTRY(V4IM_OpCode_RCV, "RCV", 0 ) \
	OCENTRY(V4IM_OpCode_RCVi, "RCVi", 0 ) \
	OCENTRY(V4IM_OpCode_ReAllocate, "ReAllocate", 6 ) \
	OCENTRY(V4IM_OpCode_Remove_Point, "Remove_Point" , 0 ) \
	OCENTRY(V4IM_OpCode_Rpt, "Rpt", 0) \
	OCENTRY(V4IM_OpCode_Same, "Same", 2) \
	OCENTRY(V4IM_OpCode_Secure, "Secure", 1) \
	OCENTRY(V4IM_OpCode_Self, "Self", 1) \
	OCENTRY(V4IM_OpCode_SendMail, "SendMail", 3) \
	OCENTRY(V4IM_OpCode_Set, "Set", 2) \
	OCENTRY(V4IM_OpCode_Socket, "Socket", 1) \
	OCENTRY(V4IM_OpCode_Sort, "Sort" , 2 ) \
	OCENTRY(V4IM_OpCode_Spawn, "Spawn", 1 ) \
	OCENTRY(V4IM_OpCode_Sqrt, "Sqrt", 1 ) \
	OCENTRY(V4IM_OpCode_SSCol, "SSCol", 1) \
	OCENTRY(V4IM_OpCode_SSDim, "SSDim", 1) \
	OCENTRY(V4IM_OpCode_SSExp, "SSExp", 2) \
	OCENTRY(V4IM_OpCode_SSFormat, "SSFormat", 1 ) \
	OCENTRY(V4IM_OpCode_SSRow, "SSRow", 1) \
	OCENTRY(V4IM_OpCode_SSULC, "SSULC", 2) \
	OCENTRY(V4IM_OpCode_SSVal, "SSVal", 1) \
	OCENTRY(V4IM_OpCode_StatAdjCosSim, "StatAdjCosSim", 2) \
	OCENTRY(V4IM_OpCode_StatAvg, "StatAvg", 1 ) \
	OCENTRY(V4IM_OpCode_StatAvgDev, "StatAvgDev", 1 ) \
	OCENTRY(V4IM_OpCode_StatChiTest, "StatChiTest", 3 ) \
	OCENTRY(V4IM_OpCode_StatConfidence, "StatConfidence", 3 ) \
	OCENTRY(V4IM_OpCode_StatCorrel, "StatCorrel", 2 ) \
	OCENTRY(V4IM_OpCode_StatCovar, "StatCovar", 2 ) \
	OCENTRY(V4IM_OpCode_StatCritBinom, "StatCritBinom", 3 ) \
	OCENTRY(V4IM_OpCode_StatDevSq, "StatDevSq", 1 ) \
	OCENTRY(V4IM_OpCode_StatDist, "StatDist", 2 ) \
	OCENTRY(V4IM_OpCode_StatDistInv, "StatDistInv", 2 ) \
	OCENTRY(V4IM_OpCode_StatErrF, "StatErrF", 1 ) \
	OCENTRY(V4IM_OpCode_StatErrFC, "StatErrFC", 1 ) \
	OCENTRY(V4IM_OpCode_StatFisher, "StatFisher", 1 ) \
	OCENTRY(V4IM_OpCode_StatFisherInv, "StatFisherInv", 1 ) \
	OCENTRY(V4IM_OpCode_StatForecast, "StatForeCast", 3 ) \
	OCENTRY(V4IM_OpCode_StatFrequency, "StatFrequency", 2 ) \
	OCENTRY(V4IM_OpCode_StatFTest, "StatFTest", 2 ) \
	OCENTRY(V4IM_OpCode_StatGammaLn, "StatGammaLn", 1 ) \
	OCENTRY(V4IM_OpCode_StatGeoMean, "StatGeoMean", 1 ) \
	OCENTRY(V4IM_OpCode_StatHarMean, "StatHarMean", 1 ) \
	OCENTRY(V4IM_OpCode_StatKurtosis, "StatKurtosis", 1 ) \
	OCENTRY(V4IM_OpCode_StatLinEst, "StatLinEst", 3 ) \
	OCENTRY(V4IM_OpCode_StatLinFit, "StatLinFit", 2 ) \
	OCENTRY(V4IM_OpCode_StatLogEst, "StatLogEst", 3 ) \
	OCENTRY(V4IM_OpCode_StatMax, "StatMax", 1 ) \
	OCENTRY(V4IM_OpCode_StatMedian, "StatMedian", 1 ) \
	OCENTRY(V4IM_OpCode_StatMin, "StatMin", 1 ) \
	OCENTRY(V4IM_OpCode_StatMode, "StatMode", 1 ) \
	OCENTRY(V4IM_OpCode_StatPCRank, "StatPCRank", 2 ) \
	OCENTRY(V4IM_OpCode_StatPearson, "StatPearson", 2 ) \
	OCENTRY(V4IM_OpCode_StatPercentile, "StatPercentile", 2 ) \
	OCENTRY(V4IM_OpCode_StatPermute, "StatPermute", 2 ) \
	OCENTRY(V4IM_OpCode_StatProb, "StatProb", 2 ) \
	OCENTRY(V4IM_OpCode_StatProb, "StatProb", 3 ) \
	OCENTRY(V4IM_OpCode_StatProduct, "StatProduct", 1) \
	OCENTRY(V4IM_OpCode_StatQuartile, "StatQuartile", 2 ) \
	OCENTRY(V4IM_OpCode_StatRan, "StatRan", 0 ) \
	OCENTRY(V4IM_OpCode_StatRank, "StatRank", 3 ) \
	OCENTRY(V4IM_OpCode_StatRSQ, "StatRSQ", 2 ) \
	OCENTRY(V4IM_OpCode_StatScale, "StatScale", 2) \
	OCENTRY(V4IM_OpCode_StatSkew, "StatSkew", 1 ) \
	OCENTRY(V4IM_OpCode_StatSlope, "StatSlope", 2 ) \
	OCENTRY(V4IM_OpCode_StatStandardize, "StatStandardize", 2 ) \
	OCENTRY(V4IM_OpCode_StatStdDev, "StatStdDev", 1 ) \
	OCENTRY(V4IM_OpCode_StatStdDevP, "StatStdDevP", 1 ) \
	OCENTRY(V4IM_OpCode_StatStdErrYX, "StatStdErrYX", 2 ) \
	OCENTRY(V4IM_OpCode_StatSumSq, "StatSumSq", 1 ) \
	OCENTRY(V4IM_OpCode_StatSumX2mY2, "StatSumX2mY2", 2 ) \
	OCENTRY(V4IM_OpCode_StatSumX2pY2, "StatSumX2pY2", 2 ) \
	OCENTRY(V4IM_OpCode_StatSumXmY2, "StatSumXmY2", 2 ) \
	OCENTRY(V4IM_OpCode_StatTrimMean, "StatTrimMean", 2 ) \
	OCENTRY(V4IM_OpCode_StatTTest, "StatTTest", 3 ) \
	OCENTRY(V4IM_OpCode_StatTTestM, "StatTTestM", 5 ) \
	OCENTRY(V4IM_OpCode_StatVar, "StatVar", 1 ) \
	OCENTRY(V4IM_OpCode_StatVarP, "StatVarP", 1 ) \
	OCENTRY(V4IM_OpCode_StatZTest, "StatZTest", 3 ) \
	OCENTRY(V4IM_OpCode_StatZTestM, "StatZTestM", 3 ) \
	OCENTRY(V4IM_OpCode_Str, "Str", 0 ) \
	OCENTRY(V4IM_OpCode_Sum, "Sum" , 1 ) \
	OCENTRY(V4IM_OpCode_Table, "Table", 1) \
	OCENTRY(V4IM_OpCode_Tally, "Tally", 2) \
	OCENTRY(V4IM_OpCode_TallyM, "TallyM", 2 ) \
	OCENTRY(V4IM_OpCode_TEQ, "TEQ", 2) \
	OCENTRY(V4IM_OpCode_Throw, "Throw", 1) \
	OCENTRY(V4IM_OpCode_Timer, "Timer", 1 ) \
	/* OCENTRY(V4IM_OpCode_Trace, "Trace", 1) \
	OCENTRY(V4IM_OpCode_TraceF, "TraceF", 1) */ \
	OCENTRY(V4IM_OpCode_Transform, "Transform", 2) \
	OCENTRY(V4IM_OpCode_Tree, "Tree", 1) \
	OCENTRY(V4IM_OpCode_Trig, "Trig", 1) \
	OCENTRY(V4IM_OpCode_Try, "Try", 1) \
	OCENTRY(V4IM_OpCode_Unicode, "Unicode", 1) \
	OCENTRY(V4IM_OpCode_UnQuote, "UnQuote", 1) \
	OCENTRY(V4IM_OpCode_UOM, "UOM", 1) \
	OCENTRY(V4IM_OpCode_V4, "V4", 0 ) \
	OCENTRY(V4IM_OpCode_V4ISCon, "V4ISCon", 2 ) \
	OCENTRY(V4IM_OpCode_V4ISOp, "V4ISOp", 1 ) \
	OCENTRY(V4IM_OpCode_V4ISVal, "V4ISVal", 2 ) \
	OCENTRY(V4IM_OpCode_Zip, "Zip", 1) \

#define V4IM_DictList_Count (257+7)		/* Must equal number of entries above (+7 for duplicates of dbXXX & ODBCXXX) */

//struct kwlist
// { int value ; char entry[25] ; char display[25] ; int minargs ; } ;
struct kwlist
 { int value ; UCCHAR display[25] ; int minargs ; } ;

#define OCENTRY(ocnum, oclc, minarg) {ocnum, UClit(oclc), minarg},

#ifdef V4IMMods
struct kwlist v4im_DictList[] = { OCLIST } ;

#endif /* V4IMMods */


