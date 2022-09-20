/*	V3DEFS.C - DEFINITIONS FOR VICTIM III

	LAST EDITED 6/15/84 BY VICTOR E. HANSEN		*/

#include <stdio.h>
#include <setjmp.h>
#define INHIBIT_V4UNICODE 1
#include "vconfig.c"	/* Pull in platform specific parameters */

#ifdef UNIX
#include <sys/times.h>
#endif

#ifdef VMSOS
#include rms
#endif

/*	V4 Interface				*/

#ifdef v3v4
#define V3_V4_OPOFFSET 2000	/* Offset for V4 "opcodes" */
#define v4is 1
#include "v4defs.c"
#endif

/*	V3_xxx - G L O B A L   P A R A M E T E R S	*/

#define V3_ASSERTION_NAME_MAX 31	/* Max chars in assertion */
#define V3_ASSERTIONS_PER_TABLE_MAX 5	/* Max number of assertions allowed in name table */
#define V3_BRANCH_PATH_MAX 500		/* Max number of BRANCH/PATHs allowed */
#define V3_CHECKSUM_TABLE_MAX 500	/* Max number of module/strucs/objects in checksum table */
#define V3_COMMAND_SEARCH_LIST_MAX 5	/* Max number of command to search for at single time */
#define V3_COMMAND_NAME_MAX 31		/* Max number characters in command name */
#define V3_DBG_BP_MAX 10		/* Max number of breakpoints per package */
#define V3_FLAG_NAME_MAX 20		/* Max number of characters in flag symbol name */
#define V3_IO_COMPACT_BUF 20000		/* Number of bytes in internal I/O compaction buffer */
#define V3_IO_FILENAME_MAX 100		/* Max characters in file name spec */
#define V3_IO_UNIT_LIST V4MM_Area_Max	/* Max number of units in ^P list */
#define V3_INTERPRETIVE_PACKAGE_ID 1	/* Package id to be used for interpretive statements */
#define V3_PICMOD_PACKAGE_ID 0		/* Special package id for use with dynamically loaded V3PIC modules */
#define V3_KERNEL_PACKAGE_ID 0		/* Package id for kernel package */
#define V3_LITERAL_STRING_MAX 1000	/* Max number characters in string literal */
#define V3_MODULE_COMMAND_MAX 50	/* Max number of commands per module */
#define V3_MODULE_DYNAMIC_MAX 20000	/* Max bytes of dynamic storage for each module */
#define V3_MODULE_LEVEL_ID_MAX 600	/* Max number of levels allowed per module */
#define V3_MODULE_MAX 50		/* Maximum number of modules per package */
#define V3_MTH_PD_LEN 15		/* Length of temp packed-decimal buffers (bytes) */
#define V3_MTH_PD_LIST_MAX 20		/* Max number of packed-decimal buffers for temp results */
#define V3_NAME_OB_PAIR_MAX 1000		/* Max number of name-object pairs in a table */
#define V3_PACKAGE_LONG_DESC_MAX 200	/* Max chars in long package description */
#define V3_PACKAGE_MAX 250		/* Maximum number of packages per process */
#define V3_PACKAGE_NAME_MAX 31		/* Max chars in package name */
#define V3_PACKAGE_NAME_OB_TABLE_MAX 10	/* Max number of name tables per package */
#define V3_PACKAGE_SHORT_DESC_MAX 50	/* Max chars in short package description */
#define V3_PARSER_ERROR_MAX 50		/* Max number of parser errors before compiler shuts down */
#define V3_PARSER_INPUT_LEVEL_MAX 8	/* Number of nested parser input streams */
#define V3_PARSER_MODULE_PRECEDENCE 50	/* Precedence of package modules */
#define V3_PARSER_PROMPT_MAX 20		/* Max chars in parser user input prompt */
#define V3_PROCESS_ASSERTION_MAX 150	/* Max number of assertions per process */
#define V3_PROCESS_FORK_MAX 31		/* Max number of process forks */
#define V3_PROCESS_INTERRUPT_MAX 20	/* Max number of interrupts in process table */
#define V3_PROCESS_NAME_TABLE_MAX 60	/* Max number of name/object tables in process search list */
#define V3_PROCESS_PCK_FILENAME_MAX 40	/* Max characters in system package name */
#define V3_PROCESS_STACK_SIZE 80000	/* Number of entries in process push-down stack */
#define V3_PROCESS_SYSTEM_PACKAGES_MAX 50 /* Number of system package slots in process def */
#define V3_PROCESS_MODULE_MAX 2003	/* Max number of modules defined in a process */
#define V3_PRS_CODE_BUF_MAX 40000	/* Max chars in package code buffer */
#define V3_PRS_CONSTANT_BUF_SIZE 15000	/* Total number of bytes for constants */
#define V3_PRS_CONSTANT_MAX 2500	/* Total number of constants */
#define V3_PRS_IMPURE_BUF_SIZE 16380	/* Number of chars in impure parse buffer */
#define V3_PRS_INPUT_LINE_MAX 1000	/* Max chars in parser input line */
#define V3_PRS_LEVEL_NAME_MAX 31	/* Max chars in module level name */
#define V3_PRS_LITERAL_MAX 1009		/* Max number of literals in package */
					/* Note: above number s/b prime */
#define V3_PRS_MAX_STRING_LEN 0xFFFFF	/* Max length of V3 string */
#define V3_PRS_MODULE_NAME_MAX 31	/* Max chars in module name */
#define V3_PRS_MODULE_LIST_MAX 500	/* Max number of modules referenced within a module */
#define V3_PRS_PURE_BUF_SIZE 16300	/* Number of chars in parser pure buffer */
#define V3_PRS_SKELETON_BUF_MAX 100000	/* Size of package buffer for skeleton entries */
#define V3_PRS_SKELETON_ELEMENT_MAX 300	/* Max number of elements within skeleton */
#define V3_PRS_SKELETON_ENTRY_MAX 100	/* Max number of skeleton defs within package */
#define V3_PRS_SOB_NAME_MAX 31		/* Max chars in name/assertion */
#define V3_PRS_STRUCT_BUF_SIZE 50000	/* Number of bytes in structure element descriptor buffer */
#define V3_PRS_STRUCT_LEVEL_MAX 15	/* Max number of nested structures */
#define V3_PRS_SUBSCRIPT_DIM_MAX 10	/* Max number of subscript dimensions */
#define V3_PRS_SYM_REF_MAX 15		/* Max number of symbolic x.x.x... we can handle */
#define V3_PRS_SYMBOL_MAX 31		/* Max chars in package local/global symbol */
#define V3_PRS_SYMBOL_TABLE_MAX_BIG 7507 /* Max number of symbols in a big table */
					/*  *** Note - Above value should be prime !! *** */
#define V3_PRS_SYMBOL_TABLE_MAX_SMALL 400 /* Max number of symbols in a small table */
#define V3_PRS_TEMP_STACK_SIZE 100	/* Max number elements (words) in parse temp stack */
#define V3_PRS_USER_FLAG_MAX 400	/* Max number of user defined flags */
#define V3_PRS_VALUE_NEST_MAX 5		/* Max number of nested value/macros */
#define V3_PREDEFINED_SYM_LEN_MAX 32	/* Max chars in predefined symbol */
#define V3_SOB_BUCKET_BUF_MAX 130000	/* Max bytes in object bucket buffer */
#define V3_SOB_FIND_HASH 107		/* Number of entries in find cache/hash table */
#define V3_SOB_HAS_HASH 107		/* Number of entries in "HAS" hash table */
#define V3_SOB_HASS_PER_OBJECT_MAX 300	/* Max number of has's per object */
#define V3_SOB_ISAS_PER_OBJECT_MAX 5	/* Max isa's per object */
#define V3_SOB_NAME_ID_OFFSET 500 	/* Offset to start local names */
#define V3_SOB_NAME_TABLE_MAX 1000	/* Max number of entries in name table */
#define V3_SOB_OBJECT_NESTING_MAX 5	/* Max number of nested objects in DCL OBJ */
#define V3_SOB_OBS_PER_BUCKET_MAX 2500	/* Max number of objects per package bucket */
#define V3_SOB_TEMP_NAME_TABLE_MAX 10	/* Max number of names in parsed name table */
#define V3_SOB_VALS_PER_OBJECT_MAX 200	/* Max number of val's per object */
#define V3_STR_CIRCULAR_BUF_SIZE 0x10000	/* Num bytes in circular buffer for string routines */
#define V3_SYMBOL_MAX_LEN 31		/* Max chars in symbol name */
#define V3_TTY_TERMINATOR_MAX 80	/* Max chars in tty_terminator() buffer */
#define V3_VERSION_MAJOR 1		/* Major version number */
#define V3_VERSION_MINOR 104		/* Minor version number */

/*	V 3   M A C R O S			*/

#define NULLV 0				/* Null pointer */
#define STD_HEADER int length ;

#define POPF(what) what = *((psi->stack_ptr)++)		/* Pop format */
#define POPVI(what) what = *((psi->stack_ptr)++)	/* Pop value immediate */
#define POPVP(what,CAST) what = CAST*((psi->stack_ptr)++)	/* Pop value pointer */
#define POPIJ j=xctu_popint() ; i=xctu_popint()

#define PUSHF(what) *(--(psi->stack_ptr)) = (V3STACK)(what)		/* Push format entry */
#define PUSHVI(what) *(--(psi->stack_ptr)) = (V3STACK)(what)
#define PUSHMP(what) *(--(psi->stack_ptr)) = (V3STACK)(what)	/* Push machine pointer onto stack */
#define PUSHVP(what) *(--(psi->stack_ptr)) = (V3STACK)(what)
#define PUSHINT(what) *(--(psi->stack_ptr)) = (V3STACK)(what) ; *(--(psi->stack_ptr))=V3_FORMAT_INTEGER ;
#define PUSHTRUE *(--(psi->stack_ptr)) = 1 ; *(--(psi->stack_ptr))=V3_FORMAT_INTEGER ;
#define PUSHFALSE *(--(psi->stack_ptr))=0 ; *(--(psi->stack_ptr)) = V3_FORMAT_INTEGER ;

#define SIZEOFSTACKENTRY 2
#define POPFV psi->stack_ptr += SIZEOFSTACKENTRY	/* Pop format/ptr off stack */
#define REPUSH psi->stack_ptr -= SIZEOFSTACKENTRY	/* Re-push item on stack */
#define NEXTSF (V3STACK)*(psi->stack_ptr + SIZEOFSTACKENTRY)	/* Format of next format on stack */
#define NNEXTSF (V3STACK)*(psi->stack_ptr + (2*SIZEOFSTACKENTRY))	/* Format of next-next format on stack */

/*	V3_XCT_xxx - E X E C U T I O N   O F F S E T S		*/

#define V3_XCT_ADDRESS 35
#define V3_XCT_ADDRESSSKEL 262
#define V3_XCT_AND 66
#define V3_XCT_ASSERTION_ADD 93
#define V3_XCT_ASSERTION_DELETE 94
#define V3_XCT_ASSERTION_TEST 95
#define V3_XCT_BIT_AND 69
#define V3_XCT_BIT_CLEAR 88
#define V3_XCT_BIT_COMP 70
#define V3_XCT_BIT_EQV 71
#define V3_XCT_BIT_GET_BYTE 102
#define V3_XCT_BIT_INVERT 233
#define V3_XCT_BIT_OR 72
#define V3_XCT_BIT_PACK2 5
#define V3_XCT_BIT_ROTATE 73
#define V3_XCT_BIT_SET 89
#define V3_XCT_BIT_SHIFT 74
#define V3_XCT_BIT_TEST_ALL 90
#define V3_XCT_BIT_TEST_ANY 91
#define V3_XCT_BIT_TEST_NONE 92
#define V3_XCT_BIT_TEST_NTH 235
#define V3_XCT_BIT_UNPACK_LH 133
#define V3_XCT_BIT_UNPACK_RH 6
#define V3_XCT_BIT_XOR 75
#define V3_XCT_BITMAP_AND 195
#define V3_XCT_BITMAP_BYTES 270
#define V3_XCT_BITMAP_CLEAR 196
#define V3_XCT_BITMAP_COMP 202
#define V3_XCT_BITMAP_COUNT 236
#define V3_XCT_BITMAP_DCL 197
#define V3_XCT_BITMAP_EXTEND 269
#define V3_XCT_BITMAP_NEXT_CLEAR 198
#define V3_XCT_BITMAP_NEXT_SET 199
#define V3_XCT_BITMAP_OR 200
#define V3_XCT_BITMAP_SET 201
#define V3_XCT_BITMAP_TEST 208
#define V3_XCT_BITMAP_TESTDF 268
#define V3_XCT_BRANCH 15
#define V3_XCT_CB_PUT 272
#define V3_XCT_CMD_ARG_NUM 103
#define V3_XCT_CMD_ARG_VALUE 104
#define V3_XCT_CMD_TEST 20
#define V3_XCT_CMD_XCT 21
#define V3_XCT_DATE_INFO 28
#define V3_XCT_DBG_BREAK 147
#define V3_XCT_DBG_CLEARBP 145
#define V3_XCT_DBG_SETBP 146
#define V3_XCT_DBG_SHOW_CALLS 149
#define V3_XCT_DIVIDEEQUAL 0
#define V3_XCT_DOLOOP1 134
#define V3_XCT_DOLOOP2 135
#define V3_XCT_DOLOOP3 136
#define V3_XCT_DUMMYREF 12
#define V3_XCT_DUMMYREF_CMD 148
#define V3_XCT_DUMMYREFX 96
#define V3_XCT_EOS 4
#define V3_XCT_EOSLN 106
#define V3_XCT_EQUAL_EQUAL 8
#define V3_XCT_EVAL_SKELETON 225
#define V3_XCT_EVAL_SKELETON_SS 226
#define V3_XCT_FORK_DELETE 128
#define V3_XCT_FORK_NEW 129
#define V3_XCT_FORK_SWITCH 130
#define V3_XCT_FORK_SWITCH1 141
#define V3_XCT_HEAP_ALLOC 44
#define V3_XCT_HEAP_FREE 45
#define V3_XCT_IF 13
#define V3_XCT_INT_DISABLE 209
#define V3_XCT_INT_ENABLE 210
#define V3_XCT_INT_IGNORE 214
#define V3_XCT_INT_SET_TIMER 211
#define V3_XCT_IO_CLOSE 76
#define V3_XCT_IO_GET 77
#define V3_XCT_IO_MISC 78
#define V3_XCT_IO_OPEN 79
#define V3_XCT_IO_PUT 80
#define V3_XCT_ITX_ALPHA 180
#define V3_XCT_ITX_DATE 122
#define V3_XCT_ITX_DATE_TIME 223
#define V3_XCT_ITX_GEN 227
#define V3_XCT_ITX_INT 123
#define V3_XCT_ITX_LOGICAL 124
#define V3_XCT_ITX_MONTH 191
#define V3_XCT_ITX_POINTER 256
#define V3_XCT_ITX_TIME 125
#define V3_XCT_JUMP 14
#define V3_XCT_JUMPC 19
#define V3_XCT_LOAD 18
#define V3_XCT_LOOP_TEST 24
#define V3_XCT_LOOP_TEST_TRUE 107
#define V3_XCT_MOD_APPLY 105
#define V3_XCT_MODCALL 9
#define V3_XCT_MOD_CURRENT_PSI 142
#define V3_XCT_MOD_DEFINED 205
#define V3_XCT_MOD_ENTRY_HANDLE 259
#define V3_XCT_MOD_ITEM_DELETE 137
#define V3_XCT_MOD_ITEM_FIND 138
#define V3_XCT_MOD_ITEM_PUSH 139
#define V3_XCT_MOD_NAME 206
#define V3_XCT_MOD_PRIOR_PSI 255
#define V3_XCT_MOD_UNLOAD 252
#define V3_XCT_MOD_WATCHDOG 221
/* unused 222 */
#define V3_XCT_MTH_DIGITS 204
#define V3_XCT_MTH_DIV 150
#define V3_XCT_MTH_EQUAL 152
#define V3_XCT_MTH_FTOI 234
#define V3_XCT_MTH_GREATER 154
#define V3_XCT_MTH_GREATEREQUAL 176
#define V3_XCT_MTH_LESS 156
#define V3_XCT_MTH_LESSEQUAL 158
#define V3_XCT_MTH_LOGE 263
#define V3_XCT_MTH_LOG10 216
#define V3_XCT_MTH_MINUS 160
#define V3_XCT_MTH_MINUSEQUAL 162
#define V3_XCT_MTH_MULTEQUAL 164
#define V3_XCT_MTH_MULTIPLY 166
#define V3_XCT_MTH_NOTEQUAL 168
#define V3_XCT_MTH_PERCENT 213
#define V3_XCT_MTH_PLUS 170
#define V3_XCT_MTH_PLUSEQUAL 172
#define V3_XCT_MTH_POWER 217
#define V3_XCT_MTH_SQRT 248
#define V3_XCT_MTH_UNARYMINUS 174
#define V3_XCT_MTH_ABS 81
/*	82 & 83 unused */
#define V3_XCT_MTH_MAX 266	/* + 267 */
#define V3_XCT_MTH_MIN 264	/* + 265 */
#define V3_XCT_MTH_MOD 84
#define V3_XCT_MTH_MOD1 85
#define V3_XCT_MTH_RANDOM 86
#define V3_XCT_MTH_RANDOM_SEED 87
#define V3_XCT_MULTEQUAL 0
#define V3_XCT_NIL 143
#define V3_XCT_NOT 67
#define V3_XCT_NULL 144
#define V3_XCT_OBJ_BIND_SKELETON 230
#define V3_XCT_OBJ_FIND 98
#define V3_XCT_OBJ_HAS_FIND 100
#define V3_XCT_OBJ_ID 27
#define V3_XCT_OBJ_NAME 108
#define V3_XCT_OBJ_VALUE 99
#define V3_XCT_OBJ_VALUE_NUM 101
#define V3_XCT_OBJREF 34
#define V3_XCT_OBJREFSS 127
#define V3_XCT_OR 68
#define V3_XCT_PARTIAL 31
#define V3_XCT_PARTIAL_SKELETON 231
#define V3_XCT_PCK_COMPILE 120
#define V3_XCT_PCK_DCL_FILENAME 194
#define V3_XCT_PCK_ENTER 37
#define V3_XCT_PCK_LOAD 38
#define V3_XCT_PCK_NEW 39
#define V3_XCT_PCK_OLD_PARSE 40
#define V3_XCT_PCK_OLD_SYSTEM 193
#define V3_XCT_PCK_OLD_XCT 41
#define V3_XCT_PCK_SAVE 42
#define V3_XCT_PCK_UNLOADA 43
#define V3_XCT_PCK_UNLOADB 121
#define V3_XCT_PRINT 2
#define V3_XCT_PUSHEOA 3
#define V3_XCT_REPEAT1 22
#define V3_XCT_REPEAT2 23
#define V3_XCT_REPEAT3 25
#define V3_XCT_RETURN 10
#define V3_XCT_RETURN1 140
#define V3_XCT_RPT_COLWIDTH 7
#define V3_XCT_STOPXCT 11	/* Special code to return to interpretive from "run" */
#define V3_XCT_SCR_DISPLAY_ROWS 239
#define V3_XCT_SCR_SET_FIELD 240
#define V3_XCT_SEG_CREATE 241
#define V3_XCT_SEG_LOCK 242
#define V3_XCT_SEG_SHARE 243
#define V3_XCT_SEG_UNLOCK 244
#define V3_XCT_SEG_UPDATE 245
#define V3_XCT_STR_APPEND 55
#define V3_XCT_STR_BE 56
#define V3_XCT_STR_BE2 218
#define V3_XCT_STR_BL 57
#define V3_XCT_STR_BREAK 117
#define V3_XCT_STR_BREAK_RIGHT 212
#define V3_XCT_STR_COMPARE 58
#define V3_XCT_STR_COMPRESS 186
#define V3_XCT_STR_CONCAT 59
#define V3_XCT_STR_EBCTOASC 190
#define V3_XCT_STR_EXPAND 187
#define V3_XCT_STR_HASH 203
#define V3_XCT_STR_ITOS 65
#define V3_XCT_STR_LEN 61
#define V3_XCT_STR_LIST 220
#define V3_XCT_STR_LOGICAL 182
#define V3_XCT_STR_MATCH 62
#define V3_XCT_STR_NULLS 113
#define V3_XCT_STR_PRIOR 60
#define V3_XCT_STR_SPACES 114
#define V3_XCT_STR_STOI 111
#define V3_XCT_STR_TRIM 183
#define V3_XCT_STR_TYPE 271
#define V3_XCT_STR_VALUE 253
#define V3_XCT_STRUCTPTR 33
#define V3_XCT_SUBSCRIPT 32
#define V3_XCT_SUBSCRIPT_OBJ 97
#define V3_XCT_SYS_CHAIN 53
#define V3_XCT_SYS_DELETE 132
#define V3_XCT_SYS_EXIT 51
#define V3_XCT_SYS_INFO 54
#define V3_XCT_SYS_LOGOUT 258
#define V3_XCT_SYS_PRINT 237
#define V3_XCT_SYS_RENAME 131
#define V3_XCT_SYS_SET_PARAMETER 219
#define V3_XCT_SYS_SLEEP 181
#define V3_XCT_SYS_SPAWN 52
#define V3_XCT_SYS_SUBMIT 238
#define V3_XCT_TKN_NEXT 249
#define V3_XCT_TKN_PARSE 250
#define V3_XCT_TKN_PARSE_OPT 254
#define V3_XCT_TKN_SET 251
#define V3_XCT_TTY_GET 46
#define V3_XCT_TTY_GETC 47
#define V3_XCT_TTY_MISC 48
#define V3_XCT_TTY_PUT 49
#define V3_XCT_TTY_SCR_ANSI 215
#define V3_XCT_TTY_SCR_GET 109
#define V3_XCT_TTY_TERMINATOR 110
#define V3_XCT_UPDATE 188
#define V3_XCT_V3_ARG_NUM 63
#define V3_XCT_V3_ARG_VALUE 64
#define V3_XCT_V3_CLEAR 26
#define V3_XCT_V3_CLONE 246
#define V3_XCT_V3_COPY 261
#define V3_XCT_V3_COPYSWAP 273
#define V3_XCT_V3_ERROR 115
#define V3_XCT_V3_INFO 50
#define V3_XCT_V3_MATCH 247
#define V3_XCT_V3_SET_PARAMETER 207
#define V3_XCT_V3_XCTRPPEXP 260
#define V3_XCT_VEC_ADD 1
#define V3_XCT_VEC_ADD_CONSTANT 17
#define V3_XCT_VEC_COPY 36
#define V3_XCT_VEC_FIND_CONSTANT 185
#define V3_XCT_VEC_SET 29
#define V3_XCT_VEC_SPAN 30
#define V3_XCT_VEC_SUM 232
#define V3_XCT_VEC_SWAP 184
#define V3_XCT_XTI_ALPHA 116
#define V3_XCT_XTI_DATE 119
#define V3_XCT_XTI_DATE_TIME 224
#define V3_XCT_XTI_GEN 228
#define V3_XCT_XTI_INT 112
#define V3_XCT_XTI_LOGICAL 118
#define V3_XCT_XTI_MONTH 192
#define V3_XCT_XTI_POINTER 257
#define V3_XCT_XTI_TIME 126
#define V3_XCT_ZAP_OBJ_SKEL_OBJLR 229

/*	F L A G   V A L U E S				*/

#define V3_FLAGS_SCR_BLINK 1
#define V3_FLAGS_SCR_BOLD 2
#define V3_FLAGS_SCR_DOUBLE_HEIGHT 16
#define V3_FLAGS_SCR_DOUBLE_WIDTH 32
#define V3_FLAGS_SCR_REVERSE_VIDEO 4
#define V3_FLAGS_SCR_UNDER_LINE 8

#define V3_FLAGS_STR_EXACT 1
#define V3_FLAGS_STR_KEYWORD 2
#define V3_FLAGS_STR_PATTERN 4
#define V3_FLAGS_STR_SCAN 32
#define V3_FLAGS_STR_NOCASE 16
#define V3_FLAGS_STR_NOSYN 32
#define V3_FLAGS_STR_BEGINS 64
#define V3_FLAGS_STR_COMPARE 128

/*	MODULE Flags					*/
#define V3_FLAGS_MOD_NOP 0
#define V3_FLAGS_MOD_FORK_LEAVE 1
#define V3_FLAGS_MOD_FORK_REENTER 2
#define V3_FLAGS_MOD_HEAP_FREE 3
#define V3_FLAGS_MOD_HEAP_FREE_LIST 6
#define V3_FLAGS_MOD_MODULE_EXIT 4
#define V3_FLAGS_MOD_UNIT_CLOSE 5
#define V3_FLAGS_MOD_WATCHDOG 7
#define V3_FLAGS_MOD_HANDLE_DELETE 8

/*	TTY Flags */
#define V3_FLAGS_TTY_CLEAR_AFTER_FIRST 1
#define V3_FLAGS_TTY_NOECHO 2
#define V3_FLAGS_TTY_PASSALL 2048
#define V3_FLAGS_TTY_QUESTION_MARK 8
#define V3_FLAGS_TTY_TRUNCATE 256
#define V3_FLAGS_TTY_TYPEAHEAD_COUNT 1024
#define V3_FLAGS_TTY_SET_DEVICE 4096
#define V3_FLAGS_TTY_FLUSH 8192

/*	File Info Flags	*/
#define V3_FLAGS_IOF_APPEND 1
#define V3_FLAGS_IOF_C 256
#define V3_FLAGS_IOF_CLIENT 0x20000
#define V3_FLAGS_IOF_CREATE 1024
#define V3_FLAGS_IOF_CREATE_IF 2048
#define V3_FLAGS_IOF_DELETE 2
#define V3_FLAGS_IOF_DELETE_FILE 8192
#define V3_FLAGS_IOF_DISCONNECTonCLOSE 0x400000
#define V3_FLAGS_IOF_EXECUTABLE 0x100000
#define V3_FLAGS_IOF_EXISTS 4096
#define V3_FLAGS_IOF_GET 4
#define V3_FLAGS_IOF_HASHED 0x800000
#define V3_FLAGS_IOF_INDEXED 512
#define V3_FLAGS_IOF_INETSOCKET 0x80000
#define V3_FLAGS_IOF_NEW_FILE 0x200000
#define V3_FLAGS_IOF_NOHANDLER 0x1000000
#define V3_FLAGS_IOF_PUT 16
#define V3_FLAGS_IOF_RELATIVE 32
#define V3_FLAGS_IOF_SEQUENTIAL 64
#define V3_FLAGS_IOF_SERVER 0x40000
#define V3_FLAGS_IOF_UNIXSOCKET 0x10000
#define V3_FLAGS_IOF_NOBLOCK 8
#define V3_FLAGS_IOF_UPDATE 128
#define V3_FLAGS_IOF_V4IS 16384

/*	Record Info Flags */
#define V3_FLAGS_IOR_CARRIAGE_RETURN 1
#define V3_FLAGS_IOR_FIXED_LENGTH 2
#define V3_FLAGS_IOR_FORTRAN 4
#define V3_FLAGS_IOR_SOS 32
#define V3_FLAGS_IOR_VARIABLE_LENGTH 64
#define V3_FLAGS_IOR_COMPRESSED 4096		/* Must be larger than 0xFF for io_misc call */

/*	Share Info Flags				*/
#define V3_FLAGS_IOS_DELETE V3_FLAGS_IOF_DELETE
#define V3_FLAGS_IOS_GET V3_FLAGS_IOF_GET
#define V3_FLAGS_IOS_NONE 0x10000
#define V3_FLAGS_IOS_PUT V3_FLAGS_IOF_PUT
#define V3_FLAGS_IOS_UPDATE V3_FLAGS_IOF_UPDATE

/*	Put/Get Info Flags				*/
#define V3_FLAGS_IOPG_BOF 32
#define V3_FLAGS_IOPG_CACHE 131072
#define V3_FLAGS_IOPG_DELETE V3_FLAGS_IOF_DELETE
#define V3_FLAGS_IOPG_DATAONLY 8192
#define V3_FLAGS_IOPG_EOF 1
#define V3_FLAGS_IOPG_FILETEXT 1048576
#define V3_FLAGS_IOPG_FILEBINARY 2097152
#define V3_FLAGS_IOPG_IGNORE_LOCK 4096
#define V3_FLAGS_IOPG_KEYED 512		/* Should be same as IOF_INDEXED ! */
#define V3_FLAGS_IOPG_LOCK_WAIT 64
#define V3_FLAGS_IOPG_NEW_RECORD 128
#define V3_FLAGS_IOPG_NEW_EOF 2048
#define V3_FLAGS_IOPG_NEXT 8
#define V3_FLAGS_IOPG_NEXTNUM1 65536
#define V3_FLAGS_IOPG_NOLOCK 16
#define V3_FLAGS_IOPG_OBSOLETE 524288
#define V3_FLAGS_IOPG_PARTIAL 1024
#define V3_FLAGS_IOPG_POSITION 4
#define V3_FLAGS_IOPG_RELOCK 1024
#define V3_FLAGS_IOPG_RESET 262144
#define V3_FLAGS_IOPG_TRUNCATE V3_FLAGS_TTY_TRUNCATE
#define V3_FLAGS_IOPG_UPDATE_EXISTING 16384
#define V3_FLAGS_IOPG_VMSRFA 32768

/*	Flags for Pragmas				*/
#define V3_FLAGS_IOPRAG_GET_SEQ 1
#define V3_FLAGS_IOPRAG_PUT_SEQ 2
#define V3_FLAGS_IOPRAG_PUT_MASS 4

/*	Flags for IO_MISC				*/
#define V3_FLAGS_IOM_CHECKPOINT 1
#define V3_FLAGS_IOM_SET_KEYS 2
#define V3_FLAGS_IOM_UNLOCK 3
#define V3_FLAGS_IOM_INIT 4
#define V3_FLAGS_IOM_IS_OPEN 5
#define V3_FLAGS_IOM_SET_KEYSV 6
#define V3_FLAGS_IOM_RECONNECT 7
#define V3_FLAGS_IOM_MANUAL_LOCK 8
#define V3_FLAGS_IOM_MANUAL_LOCKWAIT 9
#define V3_FLAGS_IOM_MANUAL_UNLOCK 10
#define V3_FLAGS_IOM_HANDLER_MODULE 11
#define V3_FLAGS_IOM_GET_BUF 12
#define V3_FLAGS_IOM_PUT_BUF 13
#define V3_FLAGS_IOM_GET_MODE 14
#define V3_FLAGS_IOM_PUT_MODE 15
#define V3_FLAGS_IOM_FILL_BUF_TEXT 16
#define V3_FLAGS_IOM_FILL_BUF_BINARY 17
#define V3_FLAGS_IOM_BLOCKING 18
#define V3_FLAGS_IOM_PEERNAME 19
#define V3_FLAGS_IOM_BOF V3_FLAGS_IOPG_BOF	/* Must be the same (V3_FLAGS_IOPG_BOF = 32) */

#define V3_FLAGS_IOM_SEQ_INPUT 0x10000
#define V3_FLAGS_IOM_SEQ_OUTPUT 0x20000
#define V3_FLAGS_IOM_TEXT_INPUT 0x40000
#define V3_FLAGS_IOM_TEXT_OUTPUT 0x80000
#define V3_FLAGS_IOM_EXECUTABLE V3_FLAGS_IOF_EXECUTABLE

/*	Flags for Data/Column Types			*/
#define V3_COLTYP_INT V4DPI_PntType_Int
#define V3_COLTYP_REAL V4DPI_PntType_Real
#define V3_COLTYP_ALPHA V4DPI_PntType_Char
#define V3_COLTYP_DATE V4DPI_PntType_UDate
#define V3_COLTYP_DATE_TIME V4DPI_PntType_UDT
#define V3_COLTYP_LOGICAL V4DPI_PntType_Logical
#define V3_COLTYP_MONTH V4DPI_PntType_UMonth
#define V3_COLTYP_TIME V4DPI_PntType_UTime

/*	Flags for Data Editing 				*/
#define V3_FLAGS_EDT_AM_PM 8192
#define V3_FLAGS_EDT_ASTERISK_FILL 8192
#define V3_FLAGS_EDT_BLANK_IF_ZERO 512
#define V3_FLAGS_EDT_CENTURY 512
#define V3_FLAGS_EDT_COMMAS 32
#define V3_FLAGS_EDT_CONCAT 1
#define V3_FLAGS_EDT_CENTER 2
#define V3_FLAGS_EDT_DASH_IF_ZERO 1024
#define V3_FLAGS_EDT_DATE_TIME 16384
#define V3_FLAGS_EDT_DDMMMYY 128
#define V3_FLAGS_EDT_DDMMYY 32
#define V3_FLAGS_EDT_EXTEND 4
#define V3_FLAGS_EDT_FLOATING_DOLLAR 2048
#define V3_FLAGS_EDT_HEX 32768
#define V3_FLAGS_EDT_JULIAN 8192
#define V3_FLAGS_EDT_LEFT_JUSTIFY 8
#define V3_FLAGS_EDT_LONG_DATE 256
#define V3_FLAGS_EDT_LOWER_CASE 32
#define V3_FLAGS_EDT_MDY 1024
#define V3_FLAGS_EDT_MMDDYY 8
#define V3_FLAGS_EDT_NOERROR 1
#define V3_FLAGS_EDT_NOPAD 16
#define V3_FLAGS_EDT_NORMALIZE 1024
#define V3_FLAGS_EDT_NOYEAR 32768
#define V3_FLAGS_EDT_OCTAL 16384
#define V3_FLAGS_EDT_ONE_DATE_TIME_DAY 86400		/* 60*60*24 - number of seconds in day */
#define V3_FLAGS_EDT_OVERPUNCH 4
#define V3_FLAGS_EDT_PAREN_IF_NEG 128
#define V3_FLAGS_EDT_POSITIVE  8
#define V3_FLAGS_EDT_PUNCTUATION 16
#define V3_FLAGS_EDT_QUOTED 4096
#define V3_FLAGS_EDT_RIGHT_HAND_SIGN 256
#define V3_FLAGS_EDT_RIGHT_JUSTIFY 64
#define V3_FLAGS_EDT_RPTCAT 0xFF
#define V3_FLAGS_EDT_SECONDS 2048
#define V3_FLAGS_EDT_TRIM_LEFT 128
#define V3_FLAGS_EDT_TRIM_RIGHT 256
#define V3_FLAGS_EDT_TRUE_FALSE 128
#define V3_FLAGS_EDT_UNIVERSAL 2048
#define V3_FLAGS_EDT_UNTIL_DELIMITER 32
#define V3_FLAGS_EDT_UPPER_CASE 512
#define V3_FLAGS_EDT_UPPER_CASE_ALT 2048
#define V3_FLAGS_EDT_X_FALSE 256
#define V3_FLAGS_EDT_X_TRUE 512
#define V3_FLAGS_EDT_YES_NO 1024
#define V3_FLAGS_EDT_YYMMDD 4
#define V3_FLAGS_EDT_ZERO_IF_ERROR 64
#define V3_FLAGS_EDT_ZERO_FILL 4096
#define V3_FLAGS_EDT_HISTORY 16			/* Date must be in the past */

/*	Flags for SYS_INFO & SYS_SET_PARAMETERS calls	*/
#define V3_FLAGS_SYS_BOX 19
#define V3_FLAGS_SYS_COMMAND_LINE 18
#define V3_FLAGS_SYS_CPU_TIME 6
#define V3_FLAGS_SYS_DATE 1
#define V3_FLAGS_SYS_DIRECTORY_PATH 10
#define V3_FLAGS_SYS_DISK_AVAILABLE 13
#define V3_FLAGS_SYS_DISK_USAGE 14
#define V3_FLAGS_SYS_ENVIRONMENT 23
#define V3_FLAGS_SYS_IO_COUNT 7
#define V3_FLAGS_SYS_IS_ACTIVE_USER 17
#define V3_FLAGS_SYS_IS_BATCH_JOB 3
#define V3_FLAGS_SYS_IS_GUI 21
#define V3_FLAGS_SYS_NEXT_FILE 22
#define V3_FLAGS_SYS_OS 20
#define V3_FLAGS_SYS_OS_ID 15
#define V3_FLAGS_SYS_PROCESS_ID 8
#define V3_FLAGS_SYS_PROCESS_SIZE 9
#define V3_FLAGS_SYS_REAL_DATE 11
#define V3_FLAGS_SYS_REAL_TIME_OF_DAY 12
#define V3_FLAGS_SYS_TIME_OF_DAY 2
#define V3_FLAGS_SYS_USER_ID 16
#define V3_FLAGS_SYS_USER_NAME 4
#define V3_FLAGS_SYS_TERMINAL_NAME 5

/*	Flags for V3_INFO Calls					*/
#define V3_FLAGS_V3_ASSERTION_CHANGES 14
#define V3_FLAGS_V3_ASTERISK_COUNT 15
#define V3_FLAGS_V3_CHECKSUM_TABLE 12
#define V3_FLAGS_V3_COUNTRY_INFO 18
#define V3_FLAGS_V3_CURRENT_PACKAGE 7
#define V3_FLAGS_V3_DATA_TYPE 4
#define V3_FLAGS_V3_DECIMAL_PLACES 1
#define V3_FLAGS_V3_ERROR 19
#define V3_FLAGS_V3_MAX_LENGTH 2
#define V3_FLAGS_V3_OBJECT_PACKAGE 16
#define V3_FLAGS_V3_OPEN_FILES 11
#define V3_FLAGS_V3_PACKAGE_FILENAME 13
#define V3_FLAGS_V3_PACKAGE_ID 10
#define V3_FLAGS_V3_PACKAGE_LOADED 8
#define V3_FLAGS_V3_PACKAGE_NAME 9
#define V3_FLAGS_V3_PARSE_ERRORS 6
#define V3_FLAGS_V3_STARTUP_OPTIONS 17
#define V3_FLAGS_V3_VERSION 5

/*	Flags for V3_SET_PARAMETER Calls		*/
#define V3_FLAGS_V3_SET_DATE V3_FLAGS_SYS_DATE
#define V3_FLAGS_V3_SET_JOBID 52
#define V3_FLAGS_V3_SET_TIME_OF_DAY V3_FLAGS_SYS_TIME_OF_DAY
#define V3_FLAGS_V3_SET_UNDEF_MODULE 50
#define V3_FLAGS_V3_SET_WATCH_LOCATION 51

/*	Flags for INT_ENABLE / INT_DISABLE		*/
#define V3_FLAGS_INT_BROADCAST_RCV 1
#define V3_FLAGS_INT_CTRLC 2
#define V3_FLAGS_INT_CTRLP 3
#define V3_FLAGS_INT_CTRLY 4
#define V3_FLAGS_INT_ERROR 5
#define V3_FLAGS_INT_MAILBOX_RCV 6
#define V3_FLAGS_INT_TIMER 7

/*	Flags for Token Generator			*/

#define V3_FLAGS_TKN_PROMPT 20
#define V3_FLAGS_TKN_PUSH_FILE V4LEX_InpMode_File
#define V3_FLAGS_TKN_PUSH_STDIN V4LEX_InpMode_Stdin
#define V3_FLAGS_TKN_PUSH_STRING V4LEX_InpMode_String
#define V3_FLAGS_TKN_FORCE_NEW_LINE 21

#define V3_FLAGS_TKN_FORCE_KW V4LEX_Option_ForceKW
#define V3_FLAGS_TKN_PUSH_CUR V4LEX_Option_PushCur
#define V3_FLAGS_TKN_RET_EOL V4LEX_Option_RetEOL

/*	Flags for Parser				*/

#define V3_FLAGS_RPP_EOL V4LEX_RPPFlag_EOL
#define V3_FLAGS_RPP_SEMI V4LEX_RPPFlag_Semi
#define V3_FLAGS_RPP_PUSHPARENS V4LEX_RPPFlag_PushParens
#define V3_FLAGS_RPP_IMPLYAND V4LEX_RPPFlag_ImplyAnd
#define V3_FLAGS_RPP_COLONKW V4LEX_RPPFlag_ColonKW

/*	Flags for Handles				*/

#define V3_FLAGS_HAN_PARENT 0x10000
#define V3_FLAGS_HAN_INFO 0x20000
#define V3_FLAGS_HAN_POINTER 0x40000
#define V3_FLAGS_HAN_FREEMEMORY 0x80000
#define V3_FLAGS_HAN_LINK 0x100000
#define V3_FLAGS_HAN_TYPE 0x200000
#define V3_FLAGS_HAN_BEFORE 1
#define V3_FLAGS_HAN_AFTER 8
#define V3_FLAGS_HAN_ENDOFLIST 4

/*	E R R O R   S T R U C T U R E S			*/

/*	db__error - Format of error block passed to error handlers */

struct db__error {
  char subsystem[10+1] ;		/* Subsystem name (ex: PCK) */
  char v3_module[20+1] ;		/* V3 Module (ex: SAVE) */
  char code[20+1] ;			/* Error code (ex: CREATEXCT) */
  char message[150+1] ;			/* Error message */
  int parameter ;			/* Error dependent parameter (may be pointer */
  char package_name[V3_PACKAGE_NAME_MAX+1] ; /* Package generating error */
  char module_name[V3_PRS_MODULE_NAME_MAX+1] ; /* Module generating error */
  int line_number ;			/* Line number within module source file */
  int v3_opcode ;			/* Last V3 opcode */
  int errnum ;				/* Error Number */
 } ;

/*	V A L U E S		*/

/*	SOT_xxx - List of Semantic Object Constants */
#define SOT_NAME 1		/* Name Object */
#define SOT_OBJECT 2		/* Actual Object */
#define SOT_NULL 5		/* The null/nil value */

/*	V A L U E   I D S			*/

#define VAL_MP 1		/* Machine pointer */
#define VAL_MPX 12
#define VAL_STATIMP 4		/* Static (impure) */
#define VAL_STACK 6		/* Module stack variable */
#define VAL_STATIMPLM 8		/* Static (impure local to module) */
#define VAL_STATPURE 2		/* Static (pure) */
#define VAL_SOB 13		/* Semantic Object ** NOTE: Must match VFT_OBJECT !! ** */
#define VAL_STATOB 10		/* Static (object) */
#define VAL_ARG 5		/* Module argument */
#define VAL_XSTACK 3		/* Prior (nested) stack */
#define VAL_XARG 7		/* Prior (nested) argument */
#define VAL_STRUCT 14		/* Offset into structure buffer */

/*	struct - MACHINE POINTER */
struct val__mp {
  char *ptr ;			/* Use 32 bit pointer */
 } ;

/*	struct val__statimp - IMPURE STATIC LOCATION */
struct val__statimp {
#ifndef ISLITTLEENDIAN
  unsigned id : 4 ;
  unsigned filler : 4 ;
  unsigned package_id : 8 ;	/* Package id code (runtime only) */
  unsigned offset : 16 ;	/* Offset into package static space */
#endif
#ifdef ISLITTLEENDIAN
  unsigned offset : 16 ;	/* Offset into package static space */
  unsigned package_id : 8 ;	/* Package id code (runtime only) */
  unsigned filler : 4 ;
  unsigned id : 4 ;
#endif
 } ;

/*	struct val__statpure - PURE STATIC LOCATION */
struct val__statpure {
#ifndef ISLITTLEENDIAN
  unsigned id : 4 ;
  unsigned filler : 4 ;
  unsigned package_id : 8 ;	/* Package id code (runtime only) */
  unsigned offset : 16 ;	/* Offset into package static space */
#endif
#ifdef ISLITTLEENDIAN
  unsigned offset : 16 ;	/* Offset into package static space */
  unsigned package_id : 8 ;	/* Package id code (runtime only) */
  unsigned filler : 4 ;
  unsigned id : 4 ;
#endif
 } ;

/*	struct val__stack - STACK LOCATION */
struct val__stack {
#ifndef ISLITTLEENDIAN
  unsigned id : 4 ;
  unsigned filler : 4 ;
  BIGFIELD offset : 24 ;	/* Offset into current stack frame */
#endif
#ifdef ISLITTLEENDIAN
  BIGFIELD offset : 24 ;	/* Offset into current stack frame */
  unsigned filler : 4 ;
  unsigned id : 4 ;
#endif
 } ;

/*	struct val__xstack - PRIOR STACK LOCATION */
struct val__xstack {
#ifndef ISLITTLEENDIAN
  unsigned id : 4 ;
  unsigned frames : 2 ;		/* Number of frames back */
  unsigned module_id : 7 ;	/* Module ID to search for */
  BIGFIELD offset : 19 ;	/* Offset into stack frame */
#endif
#ifdef ISLITTLEENDIAN
  BIGFIELD offset : 19 ;	/* Offset into stack frame */
  unsigned module_id : 7 ;	/* Module ID to search for */
  unsigned frames : 2 ;		/* Number of frames back */
  unsigned id : 4 ;
#endif
 } ;

/*	struct val__arg - MODULE ARGUMENT POINTER */
struct val__arg {
#ifndef ISLITTLEENDIAN
  unsigned id : 4 ;
  unsigned filler : 12 ;
  unsigned offset : 16 ;	/* Offset from psi->arg_ptr */
#endif
#ifdef ISLITTLEENDIAN
  unsigned offset : 16 ;	/* Offset from psi->arg_ptr */
  unsigned filler : 12 ;
  unsigned id : 4 ;
#endif
 } ;

/*	struct val__xarg - NESTED module argument */
struct val__xarg {
#ifndef ISLITTLEENDIAN
  unsigned id : 4 ;
  unsigned frames : 4 ;		/* Number of frames back from current */
  unsigned module_id : 8 ;	/* Module id of "owner" module */
  unsigned offset : 16 ;	/* Offset from psi->arg_ptr */
#endif
#ifdef ISLITTLEENDIAN
  unsigned offset : 16 ;	/* Offset from psi->arg_ptr */
  unsigned module_id : 8 ;	/* Module id of "owner" module */
  unsigned frames : 4 ;		/* Number of frames back from current */
  unsigned id : 4 ;
#endif
 } ;

/*	struct val__sob - SEMANTIC OBJECT REFERENCE */
union val__sob {
  int all ;
  struct {
#ifndef ISLITTLEENDIAN
    unsigned id : 5 ;		/* (VAL_SOB) */
    unsigned type : 3 ;
    unsigned package_id : 8 ;	/* Semantic package_id - see xxx */
    unsigned id_num : 16 ;	/* Unique id number (within class) */
#endif
#ifdef ISLITTLEENDIAN
    unsigned id_num : 16 ;	/* Unique id number (within class) */
    unsigned package_id : 8 ;	/* Semantic package_id - see xxx */
    unsigned type : 3 ;
    unsigned id : 5 ;		/* (VAL_SOB) */
#endif
   } fld ;
 } ;

/*	val__mempos - V3 Memory Position via Stack/Pure/Impure... */
union val__mempos {
  int all ;			/* Used to reference "entire" stack entry */
  struct {
#ifdef ISLITTLEENDIAN
    BIGFIELD remainder : 28 ;
    unsigned value_type : 4 ;
#else
    unsigned value_type : 4 ;
    BIGFIELD remainder : 28 ;
#endif
   } fld ;
  struct val__statimp statimp ;
  struct val__statpure statpure ;
  struct val__stack stack ;
  struct val__xstack xstack ;
  struct val__arg arg ;
  struct val__xarg xarg ;
  union val__sob sob ;
 } ;

/*	v3_udt - Defines Date Format 		*/

struct v3__udt {
  int date ;
  int time ;
 } ;

/*	v3__date_table - Format of date_info table			*/

struct v3__date_table {
  int inp_universal ;		/* Input is universal */
  int inp_mmddyy ;		/* Ditto for mm/dd/yy */
  int inp_julian ;
  int inp_yymmdd ;
  int inp_umonth ;		/* Input is universal "month" */
  int inp_date_time ;		/* Input is a date & time */
  int universal ;		/* Input date converted to universal */
  int mmddyy,julian,yymmdd,umonth,date_time ;
  short century,year,month,day ;/* Year, month, day broken out */
  short day_of_year ;		/* 1 thru 365 */
  short day_of_week ;		/* 1 = Monday, ..., 7 = Sunday */
  short days_in_year ;		/* Number of days in year */
 } ;

/*	V A L U E   F O R M A T   D E S C R I P T O R	*/

/*	Most parameters in VICTIM III are passed as 2 32-bit words:
	 word 0: describes the value as per format_dsc
	 word 1: points to or begins the value as per val__xxx descriptors

	The format.mode field describes how the value is to be found:
	  VFM_PTR	The next word is a pointer to the value;
	  VFM_OFFSET	The next word (or subscript result) is an offset;
	  VFM_IMMEDIATE	The next word begins the actual value.

	If format.subs is nonzero then subscripting is to be performed and
	the next format.subs words are the offsets for the subscripts:
	  	max_subscript,,byte_multiplier

*/

/*	T H E S E   M A Y   N E V E R   C H A N G E	*/
/*	(V4 has a copy of these defs - if you have to change one, be sure and check v4defs.c also !!!) */

#define VFT_V3INT 0		/* Internal V3 Value (see VFTV3_xxx) */
#define VFT_BININT 1		/* Binary integer */
#define VFT_FLOAT 2		/* Floating point */
#define VFT_FIXSTR 3		/* Fixed length string */
#define VFT_VARSTR 4		/* Variable length string */
#define VFT_PACDEC 5		/* Packed decimal */
#define VFT_STRINT 6		/* String integer (e.g. PIC 999) */
#define VFT_MODREF 7		/* Module call/reference */
				/* NOTE: If VFM_PTR then a pointer to db__module_objentry
					  else if VFM_OFFSET then index into module's db__module_link_list
					  else if VFM_MEP then pointer to db__module_entry
					  else if VFM_IMMEDIATE then V3 Opcode index to v3_operations() */
#define VFT_V3NUM 8		/* A V3 internal number (see val__v3num) */
#define VFT_OBJREF 9		/* An object reference (next word points to object-id/ptr) */
#define VFT_STRUCTPTR 10	/* A structure pointer */
#define VFT_INDIRECT 11		/* Indirect pointer to string */
#define VFT_POINTER 12		/* Value is a machine pointer */
#define VFT_OBJECT VAL_SOB	/* Value is an object */
#define VFT_OBJNAME 14		/* Value is pointer to struct db__dcl_objref */
#define VFT_BINWORD 15		/* A 16 bit binary integer */
#define VFT_BINLONG 16		/* A 64 bit binary integer */
#define VFT_FIXSTRSK 17		/* Fixed length string (returned by SKELs- for special error checks) */

#define VFM_PTR 0		/* Next word is pointer to value */
#define VFM_OFFSET 1		/* Address calculation is offset from base */
#define VFM_IMMEDIATE 2		/* Value immediately follows */
#define VFM_MEP 3		/* Next word is pointer to mep (used only to reference module from interpretive) */
#define VFM_INDIRECT 3		/* Pointer to "real" format/value immediately follows */
				/* Used for subscripted object values */

/*	VFTV3_xxx - Values in val__format.decimals field for internal V3 Values */

#define VFTV3_NULL 0		/* Null value */
#define VFTV3_EOA 1		/* End of Argument List */

/*	val__format - DESCRIPTION OF VALUE FORMAT WORD	*/
union val__format {
  struct {
#ifdef ISLITTLEENDIAN
  unsigned filler : 2 ;
  BIGFIELD length : 20 ;	/* Length of string/int/whatever */
  unsigned decimals : 3 ;	/* Number of decimal places */
  unsigned mode : 2 ;		/* Mode of value (see VFM_xxx) */
  unsigned type : 5 ;		/* Type of value (see VFT_xxx) */
#else
  unsigned type : 5 ;		/* Type of value (see VFT_xxx) */
  unsigned mode : 2 ;		/* Mode of value (see VFM_xxx) */
  unsigned decimals : 3 ;	/* Number of decimal places */
  BIGFIELD length : 20 ;	/* Length of string/int/whatever */
  unsigned filler : 2 ;
#endif
  } fld ;
  int all ;			/* Used to reference "entire" stack entry */
 } ;

/*	Some symbols for routine calls	*/
#define NODPS 0		/* No decimal places */
#define NOLENGTH 0	/* No length */
#define NOSUBS 0	/* No subscripts */

#define V3_FORMAT_NULL 0x4000000
#define V3_FORMAT_EOA 0x4400000
#define V3_FORMAT_FLOAT 0x10000010
#define V3_FORMAT_POINTER 0x64000010
#define V3_FORMAT_INTEGER 0xc000010
#define V3_FORMAT_INTEGER2 0xc800010
#define V3_FORMAT_V3NUM 0x40000000
#define V3_FORMAT_FIXSTR 0x18000000
#define V3_FORMAT_MODULE 0x38000000
#define V3_FORMAT_OBJNAME 0x70000000
#define V3_FORMAT_PD 0x28000078
#define V3_FORMAT_STRUCTPTR 0x50000000
#define V3_FORMAT_VARSTR 0x20003ffc

/*	Format of a V3 Internal number	*/

#define V3NUM_FPMARK 2000000000		/* Marker for Floating Point */
#define V3NUM_MAX 1000000000		/* If this big then too big */

union val__v3num
 { struct { int big,num,dps ; } fd ;	/* Fixed decimal: big*10^9+num+dps*10^-9 */
   struct { double dbl ; int marker ; } fp ; /* Floating point - marker = 2_000_000_000 */
 } ;

/*	cnv__buf - temporary buffer to convert to/from v3num */
union cnv__buf
 { struct { char buf[27] ; } all ;
   struct { char big[9],num[9],dps[9] ; } v3n ;		/* Bytes for each of the components in v3num */
 } ;

/*	P A C K A G E   C H E C K S U M   T A B L E		*/

/*	db__checksum_table - Defines checksum table for a package */

struct db__checksum_table {
  short count ;			/* Number defined below */
  short length ;		/* Number of bytes in this table */
  int package_checksum ;	/* Package checksum (total of below) */
  struct {
    int checksum ;		/* The entry's checksum */
    char type ;			/* Type of entry: 'M'odule, 'O'bject, 'S'tructure */
    char name[V3_PRS_SYMBOL_MAX+1] ; /* The name of the entry */
   } item[V3_CHECKSUM_TABLE_MAX] ;
 } ;

/*	P A C K A G E   F I L E   D E S C R I P T O R S		*/

/*	pf__locsize - Defines package file location/size	*/
struct pf__locsize {
  int loc ;			/* Location (byte) within package file */
  int bytes ;			/* Number of bytes */
 } ;

/*	P A C K A G E   H E A D E R   S T R U C T U R E		*/

struct pf__package_header {
  char package_name[V3_PACKAGE_NAME_MAX+1] ;
  char startup_module[V3_PRS_SYMBOL_MAX+1] ; /* Name of module to invoke on startup */
  unsigned char v3_version_major,	/* Must match current V3_VERSION_MAJOR */
		v3_version_minor ;
  unsigned char is_parse ;	/* If nonzero then a parse package save, otherwise xct-only */
  short int package_id ;	/* The package id */
  int package_checksum ;	/* Unique checksum for this package */
  int total_xct_bytes ;		/* Total number of bytes in next record for all xct info */
  int total_nonshare_bytes ;	/* Total number of bytes starting package which are nonsharable */
  union val__sob id_obj ;	/* Object id of "dcl package_obj" object */
  struct pf__locsize
	code,			/* Package code */
	impure,			/* Impure storage */
	pure,			/* Pure storage */
	locals,			/* Local symbol table */
	ob_bucket,		/* Object bucket */
	checksum,		/* Checksum table */
	skeletons,		/* Skeleton Tables */
	name_ob_pairs[V3_PACKAGE_NAME_OB_TABLE_MAX],	/* Name/object tables */
	name_table ;		/* Name dictionary */
 } ;

/*	R U N T I M E   I N F O R M A T I O N		*/

#define DB_MODULE 1		/* A module descriptor block id */
#define DB_PACKAGE 2
#define DB_PROCESS 3
#define DB_TERMINATION 4

#define CODE_OPERATION 0	/* A VICTIM III Operator ** NOTE: check OPINST() & v3oper.c if u change this value! */
#define CODE_INT_LITERAL 1	/* A (small) integer literal */
#define CODE_PURE 2		/* A pure value reference */
#define CODE_IMPURE 3		/* An impure value reference */
#define CODE_PUREIV 4		/* A pure value- immediate */
#define CODE_PUREPV 5		/* A pure value- convert to pointer */
#define CODE_IMPUREIV 6		/* Impure value reference- immediate */
#define CODE_IMPUREPV 7		/* Impure- convert to pointer */
#define CODE_FREQOP 8		/* A very frequently used operation */
#define CODE_PUREPVIO 9		/* Descriptor in pure space referencing impure byte offset */
#define CODE_PUREPVLMO 10	/* Descriptor in pure space referencing impurelm byte offset */
#define CODE_PUREPP 11		/* Descriptor in pure space referencing db__formatptr (for interpretive module refs) */

/*	M O D U L E   C O D E		*/

typedef unsigned short V3OPEL ;	/* Note all this stuff has to jive ! */
#define V3_CODEVAL_MAX 0xffff	/* Max code value offset */
#define PUSHCODE(TYP,OFT) {parse->code[parse->code_offset++]= (TYP<<12 | (OFT & 0xFFF)) ;}
#define OPINST(INST) { CODE_OPERATION<<12 | INST }

union db__module_code_element {
  V3OPEL all ;		/* Used to assign from short int value */
  struct {
#ifdef ISLITTLEENDIAN
  V3OPEL offset : 12 ;	/* Offset to literal, operation, value... */
  V3OPEL type : 4 ;		/* Code element type (see CODE_xxx) */
#else
  unsigned type : 4 ;		/* Code element type (see CODE_xxx) */
  unsigned offset : 12 ;	/* Offset to literal, operation, value... */
#endif
  } fld ;
 } ;

/*	Subscripting Information			*/

struct db__sym_dims {
  union val__format format ;
  int offset ;
  struct {
     unsigned short max ;				/* Subscript info - max dim value */
     unsigned short dimlen ;				/* Length of dimension */
   } ssentry[V3_PRS_SUBSCRIPT_DIM_MAX] ;
 } ;

struct db__dim_info {			/* Used in code to point to dimension info */
   short count ;		/* Number below */
   unsigned short dimlen[V3_PRS_SUBSCRIPT_DIM_MAX] ;
 } ;

/*	F O R M A T   O F   F O R M A T  -  P O I N T E R   ( S T A C K )  E N T R Y	*/

struct db__formatmempos
{ union val__format format ;
  union val__mempos mempos ;
} ;

struct db__formatmemposdims
{ union val__format format ;
  union val__mempos mempos ;
  struct db__dim_info dims ;
} ;

struct db__formatptr
{ union val__format format ;
  char *ptr ;
} ;

/*	P R O C E S S O R   S T A T U S   I N F O R M A T I O N	  */

struct db__psi {
  V3STACK *stack_ptr ;		/* Current stack pointer */
  V3STACK *arg_ptr ;		/* Pointer to argument list (first word is count) */
  char *stack_space ;		/* Pointer to begin of module invocation stack space */
  struct db__mod_item *item_list ;	/* Pointer to end of item list */
  struct db__module_runtime_entry *mre ;	/* Pointer to module runtime entry block for this module */
  V3OPEL *code_ptr ;		/* Current code "pc" in package */
  V3OPEL *pic_base ;		/* If not NULL then base pointer for module for position-independent-code */
  int *pure_base ;		/* If not NULL then base pointer to pure space immediately after module */
  int *impurelm_base ;		/* If not NULL then base pointer for module impure space */
  V3STACK *reset_stack_ptr ;	/* Stack Pointer to reset to at ";" */
  struct db__psi *command_psi ;	/* If not NULL then psi of module where current command was found */
  struct db__module_entry *mep ; /* Module entry pointer for this invocation */
  struct db__psi *prior_psi_ptr ; /* Pointer to prior psi */
  V3STACK *command_arg_ptr ;	/* Pointer to command args (valid only if in command) */
  short line_number ;		/* Last module line number */
  unsigned char package_index ;	/* Current package */
  char command_arg_cnt ;	/* Number of command args */
  char interrupt_index ;	/* If non-zero then number of interrupt this psi is handling */
  char arg_cnt ;		/* Number of arguments to current module */
  char command_index ;		/* If nonzero then command number within current module-entry (for nested command detection) */
 } ;

/*	P A C K A G E   D E S C R I P T O R	*/

struct db__package {
  char package_name[V3_PACKAGE_NAME_MAX+1] ;
  char startup_module[V3_PRS_SYMBOL_MAX+1] ; /* Name of module to invoke on startup */
  char file_name[V3_IO_FILENAME_MAX+1] ;	/* Package filename */
  short int package_id ;	/* Package id - used for insertion into process table */
  V3OPEL *code_ptr ;		/* Pointer to begin of package code */
  int *impure_ptr ;		/* Pointer to impure values */
  int *pure_ptr ;		/* Pointer to pure values */
  int total_xct_bytes ;		/* Total number of bytes in package */
  int total_nonshare_bytes ;	/* Total number of bytes nonsharable at beginning of package */
  struct db__parse_info *parse_ptr ;	/* Points to parsing info on parse load */
/*
  struct db__prs_sym_table_small *globals_ptr ;
  struct db__prs_sym_table_small *externals_ptr ;
*/
  struct sob__table *name_table_ptr ;	/* Pointer to semantic name table for package */
  struct db__name_ob_pairs *name_ob_ptrs[V3_PACKAGE_NAME_OB_TABLE_MAX] ;
  struct db__ob_bucket *ob_bucket_ptr ;	/* Pointer to object bucket */
  struct db__pck_skeletons *skeleton_ptr ;	/* Pointer to package skeletons */
  struct db__breakpoints *bp_ptr ;	/* Pointer to breakpoint info */
  union val__sob id_obj ;	/* Ojbect id of "dcl package_obj" object */
  struct db__checksum_table *checksum_ptr ; /* Pointer to checksum table */
#ifdef WINNT
  HANDLE shmid ;			/* Shared memory ID for package (if non-zero) */
#else
  int shmid ;			/* Shared memory ID for package (if non-zero) */
#endif
  char *shmaddr ;		/* Shared memory base address for package */
#ifdef vms
  int vms_addresses[2] ;	/* Holds address range of V3X package */
  unsigned char vms_slot ;	/* Which virtual memory slot for this package */
  struct FAB vms_fab ;		/* Holds V3X fab/channel/... */
#endif
 } ;

/*	M O D U L E   E N T R Y		*/
/*	Note: Module Entry Block at begin of each module in code space */

struct db__module_entry {
  union V4IS__KeyPrefix kp ;		/* Key Prefix for saving PIC module as V4 point */
  int Key ;				/* Key Value for V4 point */
  unsigned short Bytes ;		/* Total number of bytes in me & following code, pure, etc. */
  short int package_id ;		/* Package id of this module */
  int stack_bytes ;			/* Number of stack bytes required on entry */
					/* NOTE: if negative then use abs_value & enable PIC for module (temp condition) */
					/* NOTE: following offsets are byte offsets from base address psi->mep */
  unsigned short code_offset ;		/* Offset (via package code) for this module */
  unsigned short pure_offset ;		/* Offset to pure space for this module */
  unsigned short next_module_offset ;	/* Offset to next module (0 if last) for package loading */
  unsigned short level_id_offset ;	/* Offset (from begin of module) to level id list */
  unsigned short command_offset ;	/* Offset (ditto) to command list */
  unsigned short module_link_offset ;	/* Offset to link list for other modules in this module */
  unsigned short impure_words ;		/* Number of words (32-bit) to allocate for local impure for module */
  unsigned char module_id ;		/* Module id as assigned by parser (for XSTACK) */
  char name[V3_PRS_MODULE_NAME_MAX+1] ; /* Module/Command name */
 } ;

/*	db__module_link_list - Info to link modules referenced within a module */

struct db__module_link_list
{ unsigned short count ;		/* Number defined below */
  struct {
    char name[V3_PRS_MODULE_NAME_MAX+1] ;	/* Module name (current module is always in slot 0) */
   } entry[V3_PRS_MODULE_LIST_MAX] ;
} ;

/*	db__module_hash_table - Run Time Table of all modules	*/

struct db__module_hash_table
{ unsigned short count ;			/* Number entered below */
  int last_module_rt_id ;			/* Last runtime ID */
  struct {
    int module_hash ;				/* Hash of name below */
    char name[V3_PRS_MODULE_NAME_MAX+1] ;	/* Module name */
    unsigned short runtime_index ;		/* Index linking to runtime entry (0 if module currently undefined) */
   } entry[V3_PROCESS_MODULE_MAX] ;
} ;

/*	db__module_runtime_entry - Runtime entry for a module */

struct db__module_runtime_entry
{ int module_rt_id ;				/* Unique ID assigned on module load */
  struct db__module_entry *mentry ;		/* Pointer to module entry information (& rest of module) */
  int *impurelm_base ;				/* If not NULL then pointer to module's impure base */
  unsigned short count ;			/* Number below */
  char name[V3_PRS_MODULE_NAME_MAX+1] ;		/* Module name */
  struct db__module_runtime_entry		/* Pointer to module runtime entry for all defined modules referenced */
     *ext_mre[V3_PRS_MODULE_LIST_MAX] ;	/* within this module (NULL if undefined) */
} ;

/*	db__module_objentry - Entry from object value to module */

struct db__module_objentry
{ int module_hash ;				/* Hash of module name below */
  char name[V3_PRS_MODULE_NAME_MAX+1] ;
} ;

struct cmd__args {
  char count ;		/* Number below */
  struct db__formatptr arglist[20] ;
 } ;			/* Holds arguments for command invocation */

/*	N A M E   T A B L E   E N T R Y  */

struct sob__table_entry {
  char name[V3_PRS_SOB_NAME_MAX+1] ;	/* Actual name */
  int hash ;				/* Hashed value of name */
  union val__sob sob_val ;		/* Semantic value for name */
 } ;

/*	N A M E   T A B L E	*/

struct sob__table {
  STD_HEADER
  short int count ;			/* Number below */
  struct sob__table_entry tentry[V3_SOB_NAME_TABLE_MAX] ;
 } ;

/*	N A M E  /  O B J E C T   T A B L E		*/

struct db__name_ob_pairs {
  STD_HEADER
  short int num_names ;			/* Number below */
  short int package_id ;		/* Package id for this table */
  char num_assertions ;			/* Number below */
  char assertions[V3_ASSERTIONS_PER_TABLE_MAX][V3_ASSERTION_NAME_MAX+1] ;
					/* Assertions required for this table */
  int assertion_hash[V3_ASSERTIONS_PER_TABLE_MAX] ;
  char table_sorted ;			/* If TRUE then table below is sorted for binary search */
  struct {
    union val__sob name_id ;		/* Name (sorted for binary search) */
    union val__sob ob_id ;		/* Corresponding object */
   } name_ob_pair[V3_NAME_OB_PAIR_MAX] ;
 } ;

/*	C O M M A N D   S E A R C H   B L O C K		*/

struct db__command_search_list {
  char repeatable ;		/* If TRUE (default) then command can be nest-repeated */
  unsigned char count ;		/* Number below */
  struct {
    int hash ;			/* Hash value of command name */
    char name[V3_COMMAND_NAME_MAX+1] ; /* Command name */
   } com[V3_COMMAND_SEARCH_LIST_MAX] ;
 } ;

/*	C O M M A N D   E X E C U T I O N   B L O C K		*/

struct db__xct_cmd_info {
  struct db__psi *command_psi ;	/* Psi of module defining command */
  char command_index ;			/* Command index for this command */
  unsigned short cmd_code_offset ;		/* Offset (via parent module's pic_base) to begin of code for this command */
 } ;

/*	M O D U L E   L E V E L   I D S   L I S T	*/

struct db__level_ids {
  unsigned short count ;			/* Number of levels below */
  struct {
    unsigned short level_id ;			/* Level id number */
    unsigned short begin_code_offset ;		/* Start of code for level */
    unsigned short end_code_offset ;		/* End of code (includes nested levels) */
   } info[V3_MODULE_LEVEL_ID_MAX] ;
 } ;

/*	C O M M A N D   S E A R C H   L I S T		*/

struct db__command_list {
  unsigned char count ;		/* Number of commands below */
  struct {
    int hash ;			/* Hash value of command string */
    char name[V3_COMMAND_NAME_MAX+1] ; /* Command name string */
    unsigned short begin_code_offset ;	/* Begin code offset of level defining this command */
    unsigned short end_code_offset ;	/* End code offset */
    unsigned short level_id ;		/* Level id of level defining command */
    unsigned short cmd_code_offset ;	/* Offset (via parent module's pic_base) to begin of code for this command */
   } com[V3_MODULE_COMMAND_MAX] ;
 } ;

/*      D E F I N I T I O N S   F O R   C O U N T R Y   S P E C I F I C   I N F O       */

struct V3CI__CountryInfo
{ char NumericFldDelim ;                /* Numeric Field Delimiter (",") */
  char DecimalPlaceDelim ;              /* Delimits decimal place (".") */
  char NegNumPrefix ;                   /* Prefix for negative numbers ("-") */
  char NegNumSuffix ;                   /* Suffix for negative numbeers ("-") */
  char NegNumLeft ;                     /* Left hand of enclosed negative number ("(") */
  char NegNumRight ;                    /* Right hand (")") */
  char NumOverflow ;                    /* Character to fill for overflow ("*") */
  char NumFill ;                        /* Fill character ("*") */
  char DashIfZero ;                     /* Special character if zero ("-") */
  char TimeDelim ;                      /* Time delimiter (":") */
  char LeadingDollar[8] ;               /* Leading money indicator ("$") */
  int DefaultDateMask ;                 /* Default flags for itx_date mask */
  int DefaultDTMask ;                   /* Default flags for itx_date_time */
  int DefaultMonthMask ;
  char UnixMonthList[36+1] ;            /* Unix list of months ("JanFebMar...") */
  char Months[12][4] ;                  /* List of Month codes ("Jan" "Feb" ... ") */
  char ucMonths[12][4] ;                /* Same in upper case */
  char LongMonths[12][16] ;             /* List of months, long ("January", ..., "December") */
  char now[12] ;                        /* "now" */
  char NOW[12] ;
  char today[12] ;                      /* "today" */
  char TODAY[12] ;                      /* "TODAY" */
  char tomorrow[12] ;                   /* "tomorrow */
  char TOMORROW[12] ;
  char yesterday[12] ;                  /* "yesterday" */
  char YESTERDAY[12] ;
  char yes[12] ;                        /* "yes" */
  char YES[12] ;
  char no[12] ;                         /* "no" */
  char NO[12] ;
  char lTRUE[12] ;
  char lFALSE[12] ;
  char none[12] ;                       /* "none" */
  char NONE[12] ;
} ;

/*	P R O C E S S   D E S C R I P T O R	*/

#define INTERRUPT_ACTIVE 2		/* Interrupt is active */
#define INTERRUPT_DISABLED 0
#define INTERRUPT_ENABLED 1
#define INTERRUPT_IGNORE 3

struct db__process
{ struct db__package *package_ptrs[V3_PACKAGE_MAX] ; /* Pointers to resident packages */
  short int name_ob_count ;		/* Number below */
  struct db__name_ob_pairs *name_ob_ptrs[V3_PROCESS_NAME_TABLE_MAX] ;
					 /* Name/object search list */
  int asterisk_count ;			/* Number of times overflow (itx_xxx) resulted in asterisk fill */
  unsigned short assertion_changes ;	/* Number of changes to table below */
  short int assertion_count ;		/* Number below */
  char assertions[V3_PROCESS_ASSERTION_MAX][V3_ASSERTION_NAME_MAX+1] ;
					/* Current process assertions */
  int assertion_hash[V3_PROCESS_ASSERTION_MAX] ;
   unsigned char current_fork ;		/* Index into current fork */
  short int parser_errors ;		/* Total number of parser errors for this process */
  struct {
     char name[V3_SYMBOL_MAX_LEN] ;	/* Fork name */
     struct db__psi *current_psi ;	/* Pointer to psi if this fork is idle */
     struct db__psi *startup_psi ;	/* Initial fork psi */
     V3STACK *stack_base ;		/* Pointer to stack buffer */
   } fork[V3_PROCESS_FORK_MAX] ;
  struct {
     char file_name[V3_PROCESS_PCK_FILENAME_MAX+1] ;	/* Filename of system package */
   } system_packages[V3_PROCESS_SYSTEM_PACKAGES_MAX] ;
  struct {
     char command_to_xct[V3_COMMAND_NAME_MAX+1] ;	/* Command to xct on this interrupt */
     char status ;			/* Status of interrupt - see INTERRUPT_xxx */
   } interrupts[V3_PROCESS_INTERRUPT_MAX+1] ;
  struct db__module_runtime_entry *mre[V3_PROCESS_MODULE_MAX] ;	/* Pointers to all loaded modules in this process */
  struct db__module_hash_table mht ;			/* Module hash table of all known/unknown modules in process */
  int minutes_west ;					/* Number of minutes this process is "west of GMT" (with DST adjust) */
  char have_V3intercept ;				/* If true then intercept errors via below */
  char have_V4intercept ;				/* If true then intercept errors via below */
  jmp_buf error_intercept ;				/* Jump buffer for any/all error interceptions */
  struct V4C__Context *ctx ;				/* Pointer to current V4 context */
  struct V3CI__CountryInfo ci ;				/* Link to country specific information */
  char v4_eval_on_undefmod[250] ;			/* If set then V4 command to execute on undefined module */
  char v3_start_command_line[4096] ;			/* Holds command line used to start V3 */
  char v3_startup_option_list[200] ;			/* Holds -o switch value */
  int last_v3_opcode ;					/* Updated with last v3 opcode (for error dumps) */
  int last_errnum ;					/* Last V3 Error number */
  int exit_value ;					/* How we are exit'ing from V3 */
  int MouseX,MouseY ;					/* Last mouse click position */
  int Clicks ;						/* 1 = single click, 2 = double click, ... */
  int Button ;						/* 1 = left button, 2 = right button, 3 = middle */
  int OtherKeys ;					/* 1 = shift, 2 = control, 4 = alt */
  char v4_error[300] ;					/* Last v4 error message */
  jmp_buf v3_error_jmp ;				/* Where to go to recover from error */
#ifdef UNIX
  int delta_time ;					/* Delta time for ^P */
  struct tms delta_tms ;
#endif
#ifdef vms
  char vms_vm_slots[V3_PACKAGE_MAX] ; /* If nonzero then have a package in slot */
#endif
} ;

/*	B U F F E R   ( C O D E ,  P U R E , ... )		*/

struct db__buf {
  STD_HEADER
  char buffer[1] ;
 } ;

/*	MODULE / ITEM  ENTRY				*/
struct db__mod_item {
  unsigned char v3_exit_func ;		/* Internal function on module exit: see V3_FLAG_MOD_xxx */
  char name[V3_PRS_SYMBOL_MAX+1] ;	/* Item name */
  struct db__psi *owner_psi ;		/* Pointer to owner psi */
  struct db__mod_item *prior ;		/* Pointer to prior item for psi */
  union val__format format ;		/* Value format */
  char *ptr ;
 } ;

/*	B R E A K P O I N T   I N F O				*/

struct db__breakpoints {
  unsigned char count ;			/* Number in use below */
  struct {
    int offset ;			/* Offset into code of breakpoint */
    union db__module_code_element restart[10] ; /* Restart code from breakpoint */
   } bp[V3_DBG_BP_MAX] ;
 } ;

/*	O B J E C T   D E S C R I P T O R	*/

struct ob__desc {
  unsigned ss_vals : 16 ;
  unsigned length : 13 ;	/* Length (bytes) of this object */
  unsigned val_is_list : 1 ;	/* If TRUE then value below defined as list */
  unsigned val_needs_parent : 1 ; /* Value is a module which takes parent's value as argument */
  unsigned val_is_deferred : 1 ;  /* Value is a module which should be pushed, not evaluated */
  unsigned char num_isas ;	/* Number of ISA's */
  unsigned char num_hass ;
  unsigned char num_vals ;
  unsigned char ss_isas[V3_SOB_ISAS_PER_OBJECT_MAX] ;
  unsigned char ss_hass ;
  union val__sob ob_id ;	/* Objects id */
  union val__sob name_id ;	/* Object's name (may not be unique) */
  char info[1] ;		/* Object information */
 } ;


/*	db__ob_bucket - Holds all objects in a package */

struct db__ob_bucket {
  STD_HEADER
  short int count ;		/* Number of objects */
  unsigned short free_offset ;	/* Offset in buf to first free location */
  unsigned short offsets[V3_SOB_OBS_PER_BUCKET_MAX] ; /* Offset to each object */
  int waste ;			/* Number of bytes wasted below due to updates */
  int buf[V3_SOB_BUCKET_BUF_MAX/SIZEofINT] ;
 } ;

/*	Temp structures for parsing "DCL OBJects" 	*/

struct db__dcl_objref {
  char count ;				/* If nonzero then number of name_ids below */
					/* NOTE: If zero then first element below is actual object id */
  union val__sob name_ids[V3_SOB_TEMP_NAME_TABLE_MAX] ;
 } ;

struct db__dcl_hasref {
  union val__sob has_name ;		/* HAS object name */
  union val__sob has_ob ;		/* HAS object id */
 } ;

struct db__dcl_valref {
  union val__format format ;		/* Value type format code */
  union val__mempos mempos ;		/* "Pointer" to value */
  char valid ;				/* Value id */
 } ;
struct db__dcl_object {
  struct db__dcl_object *parent ;	/* Pointer to parent object (NULL if top level) */
  char obname[V3_ASSERTION_NAME_MAX+1] ;	/* Objects text name */
  union val__sob name_id ;		/* Updated with name id */
  union val__sob ob_id ;		/* Updated with object id when object is created */
  char nesting ;			/* Number of nested (0 if top level) */
  char num_assertions ;			/* Number assertions below */
  char assertions[V3_ASSERTIONS_PER_TABLE_MAX][V3_ASSERTION_NAME_MAX+1] ;
  int assertion_hash[V3_ASSERTIONS_PER_TABLE_MAX] ;
  char val_is_list ;			/* If TRUE then value is a list */
  char val_needs_parent ;		/* If TRUE then value is form: module(PARENT) */
  char val_is_deferred ;		/* If TRUE then value is form: module(DEFERRED) */
  char num_isas ;			/* Number of isa's below */
  struct db__dcl_objref isarefs[V3_SOB_ISAS_PER_OBJECT_MAX] ;
  unsigned char num_hass ;		/* Number of has's below */
  struct db__dcl_hasref hass[V3_SOB_HASS_PER_OBJECT_MAX] ;
  unsigned char num_vals ;		/* Number of val's below */
  struct db__dcl_valref vals[V3_SOB_VALS_PER_OBJECT_MAX] ;
 } ;

/*	S K E L E T O N   D E F I N I T I O N S			*/

/*	Define Package Skeleton Block		*/

struct db__pck_skeletons {
  STD_HEADER
  int count ;					/* Number defined below */
  int index[V3_PRS_SKELETON_ENTRY_MAX] ;	/* Index into buffer for n'th skeleton */
  int free_byte ;				/* Index to next free byte below */
  char buf[V3_PRS_SKELETON_BUF_MAX] ;		/* Holds skeletons */
 } ;

/*	Define Skeleton Structure		*/

struct db__prs_skeleton {
  char skel_name[V3_PRS_SYMBOL_MAX+1] ;		/* Skeleton name */
  char skel_type ;				/* Type of skeleton: SKELETON_xxx */
  unsigned short count ;			/* Number of entries below */
  union val__sob master_object  ;		/* Name of master object */
  int alloc_bytes ;				/* Number of bytes required for top level structure */
  struct {
    union val__sob el_name ;			/* Skeleton element name */
    unsigned level : 8 ;			/* Level within structure */
    unsigned defined : 1 ;			/* If TRUE then element is fully defined */
    unsigned object : 1 ;			/* If TRUE then element is object to be bound at package load time */
    unsigned skelref : 1 ;			/* If TRUE then element is reference to another skeleton */
    unsigned all : 1 ;				/* If TRUE then indicates "ALL" reference */
    unsigned redefines : 1 ;			/* If TRUE then indicates redefined reference */
    struct db__dim_info dims ;
    union val__format format ;			/* Value format */
    int offset ;				/* Value offset */
    union val__sob el_object ;			/* Object for this element */
    unsigned char skel_index,el_index ;		/* Indexes to parent reference if this is skelref */
   } el[V3_PRS_SKELETON_ELEMENT_MAX] ;
 } ;

/*	Format of skeleton reference in code	*/

struct db__skeleton_ref {
  unsigned short skel_index ;			/* Index into package skeleton block for skeleton */
  short el_index ;				/* Element within skeleton */
  union val__mempos base_where ;		/* Location to base pointer */
 } ;

/*	P A R S I N G   S Y M B O L S			*/

/*	SYMBOL_xxx - Symbol table entry types		*/

#define SYMBOL_NORMAL 1			/* Normal symbol */
#define SYMBOL_SEMANTIC 2		/* Symbol to be bound to object */
#define SYMBOL_STRUCTURE 3		/* Symbol is name of a structure */
#define SYMBOL_STRUCTREF 4		/* Symbol is a structure instantiation */
#define SYMBOL_MODULE 5			/* Symbol is a module reference */
#define SYMBOL_DUMMY 6			/* Symbol is module dummy argument */
#define SYMBOL_LABEL 7			/* Symbol is a label */
#define SYMBOL_UNDEF_LABEL 8		/* Symbol is as of now undefined label */
#define SYMBOL_STRUCTPTR 9		/* Symbol is a structure pointer */
#define SYMBOL_OBJREF 10		/* Symbol is an object reference */
#define SYMBOL_DUMMY_CMD 11		/* Dummy argument for command */
#define SYMBOL_SKELETON 12		/* A skeleton structure */
#define SYMBOL_SKELETONREF 13		/* A skeleton instantiation */

/*	Token Generation Results			*/
#define TKN_LITERAL 1			/* Got a string/numeric literal */
#define TKN_SYMBOL 2			/* Got a user symbol */
#define TKN_PREDEFINED 3		/* Got a V3 predefined symbol -
					   tkn.sym_ptr points to it */
#define TKN_ENDINPUT 4			/* Hit end of input - return */

/*	PRS_xxx - Parsing symbols		*/

#define PRS_GLOBAL_SYM 1	/* For adding a global symbol */
#define PRS_LOCAL_SYM 2		/* For adding a local symbol */
#define PRS_EXTERNAL_SYM 3	/* For adding an external symbol */
#define PRS_LABEL_SYM 4		/* For adding a label */
#define PRS_ELEMENT_SYM 5	/* For adding a structure element */
#define PRS_STRUCTDEF_SYM 6	/* For adding a structure definition (global) */

/*	RES_xxx - Module Results				*/

#define RES_NONE 0		/* Module returns no result */
#define RES_UNK 1		/* Module returns unknown result */
#define RES_INT 2		/* Module returns integer result */
#define RES_ARGS 3		/* Module returns result based on args */
				/* This for handling of "+", "-", etc. */
#define RES_EOA 4		/* This is the end-of-arg-list marker */

/*	SKELETON_xxx - Types of Skeletons		*/

#define SKELETON_PARTIAL 1		/* A partial skeleton (missing elements from main STRUCT) */
#define SKELETON_FULL 2			/* A fully defined skeleton, bound at package load time */

/*	Token generator result structure	*/
struct db__tknres {
  unsigned char type ;		/* Token result - see TKN_xxx */
  char symbol[V3_SYMBOL_MAX_LEN+1] ;	/* If a symbol then the symbol string */
  unsigned char symbol_len ;		/* Length of symbol */
  unsigned char actual_symbol_len ;	/* Actual length before possible truncation */
  unsigned char radix ;			/* Radix (usually 10) of literal integer */
  struct db__opr_info *sym_ptr ;	/* Pointer to predefined symbol */
  short int literal_len ;		/* If a literal then length */
  union val__format lit_format ;	/* Literal value format */
  union {
    int int_lit ;			/* Value if literal integer */
    double float_lit ;
    char char_lit[V3_LITERAL_STRING_MAX] ;
   } value ;
 } ;

/*	L I T E R A L   T A B L E			*/

struct db__lit_table {
  STD_HEADER
  short int count ;		/* Number of literals below */
  unsigned short offsets[V3_PRS_LITERAL_MAX] ; /* Offset to each literal (in static pure area) */
 } ;

/*	Definition of symbol table entry		*/

struct db__prs_sym {
  char name[V3_PRS_SYMBOL_MAX+1] ;	/* Symbol name */
  union val__mempos mempos ;		/* Where symbol descriptor can be found (e.g. package pure/impure) */
  int hash ;				/* Symbol's hash value */
  unsigned short level_id ;		/* Level id of level (or module level if a label) creating symbol */
  unsigned short prior_sym_index ;	/* Index to prior symbol
					   (allows for skipping over inactive levels) */
  unsigned short structure_id ;		/* If nonzero then structure id this symbol belongs to (see NOTE below) */
  unsigned short search_struct_id ;	/* STRUCT id to search for next ".xxx" symbol (see NOTE below) */
  unsigned referenced : 1 ;		/* TRUE if symbol has been referenced */
  unsigned initialized : 1 ;		/* TRUE if symbol has been initialized */
  unsigned type : 6 ;			/* Symbol type - see SYMBOL_xxx */
  unsigned module_id : 8 ;		/* 0 or module_id of owner module */
					/* If this is module name then this is its id ! */
  char ss_count ;			/* Number of subscripts with this symbol */
  unsigned char align_mask ;		/* Address alignment mask (ex: 3 for word alignment) */
 } ;
/*	NOTE: for skeletonrefs, structure_id is starting element index+1, search_struct_id is skeleton index */

/*	Symbol table formats				*/

struct db__prs_sym_table_big {
  STD_HEADER
  unsigned short count ;		/* Number in table */
  struct db__prs_sym sym[V3_PRS_SYMBOL_TABLE_MAX_BIG] ;
 } ;

struct db__prs_sym_table_small {
  STD_HEADER
  unsigned short count ;		/* Number in table */
  struct db__prs_sym sym[V3_PRS_SYMBOL_TABLE_MAX_SMALL] ;
 } ;

/*	Definition of "Constants" Symbol Table	*/

struct db__constants {
  char args[10][132+1] ;		/* Holds macro arguments */
  char expansion[1000] ;		/* Holds macro expansion */
  short int count ;			/* Number defined below */
  struct {
     char name[V3_PRS_SYMBOL_MAX+1] ;	/* Constant's name */
     int hash ;				/* Its hash value */
     char is_macro ;			/* If nonzero then got a macro, not literal value */
     unsigned short offset ;		/* Offset into buf below for value */
   } sym[V3_PRS_CONSTANT_MAX] ;
  unsigned short free_offset ;		/* Offset to next free byte */
  char buf[V3_PRS_CONSTANT_BUF_SIZE] ;	/* Value buffer */
 } ;

/*	Definition of parse "level"			*/

struct db__prs_level
{ unsigned short valx ;		/* Index into value stack below */
  unsigned char val_stack[100] ;	/* Value stack - contains RES_xxx items for keeping track of parser */
  struct db__prs_sym *sym_stack[100] ;  /* Sym pointer stack for checking updates before references */
  char name[V3_PRS_LEVEL_NAME_MAX+1] ;	/* Optional level name */
  unsigned short level_id ;		/* Level id code (unique for this level) */
  struct db__prs_level *prior_level ;	/* Pointer to prior parse level */
  int bottom_of_level,			/* Index into code buffer of first linked
					   list for bottom of level */
	end_of_level,			/* More or less the same */
	top_of_level ;			/* Ditto */
  unsigned short module_entry_offset ;	/* Offset (pure) of module entry block if this is a module */
  struct db__level_ids *module_level_list_ptr ; /* Pointer to list of levels */
  struct db__command_list *module_command_list_ptr ; /* Pointer to list of module commands */
  short int command_level_id ;		/* Level id of command level (if in command */
  unsigned short prior_sym_index ;	/* Offset in sym table to prior symbol */
  unsigned short prior_tmp_index ;	/* Prior operator temp stack index */
  unsigned short if_code_offset ;	/* Offset into code for jump-around of then/else code */
  unsigned short end_level_list ;	/* If nonzero then begin of linked-list for end_of_level */
  unsigned short branch_fall_thru_list ; /* If nonzero then begin of list for prior PATH FALL_THRU */
  unsigned short branch_info_offset ;	/* Code offset for index for branch_info */
  unsigned short branch_top_offset ;	/* Code offset of begin of BRANCH */
  struct db__branch_info *branch_info_ptr ;
  unsigned short loop1_jump_offset ;	/* Loc of jump for LOOP1 initial skip over test */
  unsigned short loop_test_offset ;	/* Offset to begin of LOOP test code */
  unsigned short loop_inc_offset ;	/* Offset to begin of LOOP increment code */
  unsigned short loop_test_ok_offset ;	/* Offset to end of LOOP test (for linkage to begin of loop code) */
  unsigned iterative : 1 ;		/* If TRUE then an iterative construct level (e.g. IF/LOOP/...) */
  unsigned interpretive : 1 ;		/* If TRUE then parser running interpretively */
  unsigned in_if : 1 ;			/* If TRUE then parsing IF statement */
  unsigned have_then : 1 ;		/* If TRUE then in the THEN portion */
  unsigned have_else : 1 ;		/* If TRUE then in the ELSE portion */
  unsigned in_branch : 1 ;		/* If TRUE then in BRANCH */
  unsigned in_loop : 1 ;		/* If TRUE then in LOOP */
  unsigned in_loop1 : 1 ;		/* If TRUE then have LOOP1 (in_loop also set) */
  unsigned in_loop_setup : 1 ;		/* If TRUE then parsing the xxx of "LOOP( xxx )" */
  unsigned is_loop_forever : 1 ;	/* If TRUE then have endless loop */
  unsigned is_loop_do : 1 ;		/* If TRUE then in "do loop" */
  unsigned have_loop_do_inc : 1 ;	/* If TRUE then have explicit increment in "do loop" */
  unsigned have_brace : 1 ;		/* If TRUE then have nested the "{" */
  unsigned is_command : 1 ;		/* If TRUE then this is a command level */
  unsigned in_command : 1 ;		/* If TRUE then is/in command level */
  unsigned is_module : 1 ;		/* If TRUE then this is a module level */
 } ;

/*	Definition of main parser control block		*/

struct db__parse_info
{ unsigned short package_id ;	/* Package id we are parsing for */
  char package_name[V3_PACKAGE_NAME_MAX+1] ;
  union val__sob id_obj ;		/* Object id of "dcl package_id" object */
  int pic_base ;			/* Offset to base of module for "position-independant-code" generation */
  int code_offset ;			/* Offset to next code element */
  unsigned short tmp_index ;		/* Temp stack index */
  unsigned short last_level_id ;	/* Last level_id assigned */
  unsigned char last_module_id ;	/* Last used module id */
  struct db__prs_level *level ;		/* Pointer to current parse level */
  struct db__opr_info *tmp[V3_PRS_TEMP_STACK_SIZE] ;	/* Temp parse stack */
  unsigned short code[V3_PRS_CODE_BUF_MAX] ; /* Code buffer */
  char module_name[V3_PRS_MODULE_NAME_MAX+1] ;	/* Current module name */
  int module_stack_bytes ;		/* Max bytes required for module dynamic storage */
  unsigned short begin_module_offset ;		/* Offset in code space to begin of current module (& module_entry) */
  unsigned short module_level_id ;	/* If nonzero then level id of enclosing module */
  unsigned short last_struct_level_id ;	/* Last STRUCT level id used */
  struct db__module_link_list mod_ll ;		/* Module link list for current module (0 is current module) */
  struct db__module_entry *last_mep ;		/* If not NULL then pointer to last defined module mep */

  struct db__prs_sym_table_small globals ;	/* global symbol table */
  struct db__prs_sym_table_small externals ;	/* external symbol table */
  struct db__prs_sym_table_small structures ;	/* global structure definitions */
  struct db__prs_sym_table_big locals ;	/* Local symbol table */
  struct db__prs_sym_table_big elements ;	/* Structure elements */
  struct db__prs_sym_table_small labels ;	/* Label symbol table */
  struct db__constants constants ;	/* Constants symbols/values */
  struct db__ob_bucket ob_bucket ;	/* Object bucket */
  struct sob__table name_table ;	/* Name table */
  struct sob__table assertion_table ;	/* Assertion table */
  struct db__name_ob_pairs name_ob_pairs[V3_PACKAGE_NAME_OB_TABLE_MAX] ; /* Name-object pair table */
  unsigned char dcl_assertions_cnt ;	/* Number of assertions below */
  char dcl_assertions[V3_ASSERTIONS_PER_TABLE_MAX][V3_PRS_SYMBOL_MAX+1] ;
  short int user_flag_count ;		/* Number below */
  struct {
    char name[V3_PRS_SYMBOL_MAX+1] ;	/* User defined flag value */
    int value ;				/* Value of the flag */
   } user_flags[V3_PRS_USER_FLAG_MAX] ;
  char enable_checksums ;		/* If TRUE then enable checksum calculations */
  char in_pck_compile ;			/* If TRUE then parsing from "pck_compile()" */
  struct db__checksum_table checksums ;	/* Module/Structure/Object checksum table */
  int current_checksum ;		/* Current lexical checksum */
  int token_count ;			/* Current token count (used to calculate above) */
  struct db__pck_skeletons skeletons ;	/* Current package skeletons */
  unsigned short glbl_xtrnal_count ;	/* Sum of globals & externals (for check on call to xctu_undef_resolve) */
  FILE *v4b_fileptr ;			/* Points to output file for V4 bindings */
  short int error_cnt ;			/* Number of parser errors */
  char error_flag ;			/* Set to TRUE on any parser errors (reset by pck_save) */
  unsigned char dcl_nest ;		/* If nonzero then number of nested {'s in DCL */
  char ilx ;				/* Index to below */
  unsigned char need_input_line ;	/* Nonzero if need new input line */
  struct {
    FILE *file ;			/* Input file desc pointer (NULL if for user tty) */
    char prompt[V3_PARSER_PROMPT_MAX] ;	/* Prompt if input is user tty */
    char input_str[V3_PRS_INPUT_LINE_MAX] ; /* Current input line */
    char *input_ptr ;			/* Pointer to next byte in input_str */
    char *free_ptr ;			/* If not null then call cfree with this ptr when exiting level */
    short int statement_start_line ;	/* Line number current statement started on */
    unsigned short current_line ;	/* Current line number */
    unsigned short total_lines ;	/* Total lines in this file */
    char is_sos_file ;			/* TRUE if an SOS line numbered file */
    char is_not_sos_file ;		/* TRUE if not an SOS line numbered file */
    struct db__prs_level *level_on_entry ; /* Pointer to level on entry of this file */
    char file_name[128] ;		/* File name (or nothing) for this level */
    short file_page ;			/* Current page number (if sos) */
    short last_page_printed ;		/* Set to current page on error (to avoid extra print-out) */
    short condcomp_nest ;		/* If nonzero then compiling within conditional (#{ ... }#) */
    short condcomp_ignore ;		/* If nonzero then ignore code until 0 */
   } ilvl[V3_PARSER_INPUT_LEVEL_MAX] ;
  char constant_bkp_count ;		/* Number of nested value/macros */
  char *constant_input_ptr_bkp[V3_PRS_VALUE_NEST_MAX+1] ;	/* Backup of input_ptr during lexical analysis of constant */
  char *constant_start_ptr[V3_PRS_VALUE_NEST_MAX+1] ;		/* Starting pointer for constant/macro */
  char *prior_input_ptr ;		/* Pointer to begin of token (for lookahead */
  char have_lookahead ;			/* If true then re-tokenize prior token */
  char have_value ;			/* If true then last token returns a value (for unary minus) */
  char in_comment ;			/* If true then in a slash-star ... star-slash comment */
  char check_initialized ;		/* If true then check for initialized stack variables */
  char generate_pic ;			/* If true then generate position-independent-code */
#ifdef v3v4
  struct V4C__Context *ctx ;		/* Pointer to context if V4 referenced via "dcl v4eval" */
  char is_module_bind ;			/* If true then blow out module via V4 binding */
  char module_bind[200] ;		/* Module binding command string */
  char dflt_module_bind[200] ;		/* Default binding command string */
  char dflt_module_v4eval[250] ;	/* If given then one or more V4Eval commands to execute with each module def */
  char eval_on_undefstruct[250] ;	/* Specifies intersection to eval for defining structure macro */
  int aid ;				/* Aid of area where everything is to go */
  int V4ModId ;				/* Module Id assigned to current module (used to form key to save PIC stuff) */
#endif
  unsigned char default_radix ;		/* Default numeric literal radix */
  struct db__lit_table lit ;		/* Literal table */
  struct db__lit_table oblit ;		/* Literal table for object values */
  int pure_free ;			/* Index to free below */
  int impure_free ;
  int impurelm_free ;
  int struct_bytes ;			/* Number of bytes in use below */
  double fill1 ;			/* Force alignment to double */
  int impure_buf[V3_PRS_IMPURE_BUF_SIZE/4] ;
  double fill2 ;			/* Force alignment to double */
  int pure_buf[V3_PRS_PURE_BUF_SIZE/4] ;
  double fill3 ;			/* Force alignment to double */
  int impurelm_buf[V3_PRS_IMPURE_BUF_SIZE/4] ;
  double fill4 ;			/* Force alignment to double */
  char struct_buf[V3_PRS_STRUCT_BUF_SIZE] ; /* Holds symbol descriptors for structure elements */
 } ;

/*	db__opr_info - Predefined symbol table entry format	*/
struct db__opr_info {
  unsigned precedence : 6 ;		/* Operator precedence */
  unsigned on_push_do : 6 ;		/* See V3_PUSH_xxx */
  unsigned on_pop_do : 4 ;		/* See V3_POP_xxx */
  unsigned xct_immediate : 1 ;		/* Push on xct immediate */
  unsigned xct_immed_end : 1 ;		/* Append opr immediately after a few checks */
  unsigned throw_out : 1 ;		/* Throw out opr */
  unsigned right_to_left : 1 ;		/* If set then parse right to left */
  unsigned end_of_arg_list : 1 ;	/* If set then push end-of-args */
  unsigned push_immediate : 1 ;		/* Push immediate */
  unsigned returns_value : 1 ;		/* Operator returns a value */
  unsigned check_paren : 1 ;		/* ??? - xxx */
  unsigned loop1 : 1 ;			/* Got LOOP1 (versus LOOP) */
  unsigned arithmetic : 1 ;		/* Do special aritmetic test (for integer versus non-integer */
  unsigned update_arg1 : 1 ;		/* Module updates first argument */
  unsigned update_arg2 : 1 ;
  unsigned noreference : 1 ;		/* Module does not "reference" argument value (sys_address()) */
  unsigned update_arg3 : 1 ;		/* Module updates third argument */
  unsigned filler : 2 ;
  unsigned reference : 16 ;		/* Index into V3 runtime system for execution (0 if nop) */
  unsigned results : 8 ;		/* Module results: see RES_xxx */
  unsigned args : 8 ;			/* Number of arguments (or min if EOA) */
 } ;

/*	Definition of branch_info			*/

struct db__branch_info {
  short count ;		/* Number of PATHs  (negative if "PATH(value)" */
  int min_value ;	/* Minimum value if "PATH(value)" */
  unsigned short path_offsets[V3_BRANCH_PATH_MAX] ; /* Code offset for each PATH */
  short value_max ;	/* Number of PATH(value)'s below */
  int values[V3_BRANCH_PATH_MAX] ;
  unsigned short value_offsets[V3_BRANCH_PATH_MAX] ;
 } ;

/*	DO - LOOP  Structure				*/
struct db__do_loop_info {
  union {
    int *int_val ;
    short *short_val ;
    double *double_val ;
   } var ;				/* Pointer to variable being updated */
  union {
    float float_val ;
    int int_val ;
   } max ;				/* Maximum value */
  union {
    float float_val ;
    int int_val ;
   } inc ;				/* Incremental value */
  char type ;				/* Format of "do variable" (VFT_xxx) */
 } ;

/*	E X T E N D E D   D A T A   T Y P E S		*/

/*	flag__ref - Defines format of /n.n,xxx/ constants	*/

union flag__ref {
     struct {
#ifdef ISLITTLEENDIAN
       short int mask ;		/* Bit Mask */
       unsigned char byte2 ;
       unsigned char byte3 ;
#else
       unsigned char byte3 ;
       unsigned char byte2 ;
       short int mask ;
#endif
      } fld ;
  int all ;
 } ;

/*	str__ref - Internal representation for strings	*/

struct str__ref {
  union val__format format ;
  char *ptr ;
 } ;

/*	iou__openlist - Format of internal list of open units	*/

struct iou__openlist {
  int count ;
  struct {
    char file_name[79+1] ;
    struct iou__unit *unitp ;
   } unit[V3_IO_UNIT_LIST] ;
} ;

/*	iou__unit - Format of V3 I/O unit			*/

struct iou__unit {
  char v3_name[15+1] ;		/* User name for generating I/O errors */
  char file_name[79+1] ;		/* File name set by user, updated on open */
  union val__format name_dflt_format ; /* Defaults for file name at open time */
  char *name_dflt_ptr ;
  union val__format dflt_put_format ;
  char *dflt_put_ptr ;
  union val__format dflt_get_format ;
  char *dflt_get_ptr ;
  union val__format dflt_key_format ;
  char *dflt_key_ptr ;
  union val__format put_format ;
  char *put_ptr ;
  union val__format get_format ;
  char *get_ptr ;
  union val__format key_format ;
  char *key_ptr ;
  int file_info ;		/* File open/creation information */
  int record_info ;		/* Record information */
  int share_info ;		/* File sharing information */
  unsigned short file_record_length ; /* Max bytes in record */
  int desc_object ;		/* Optional descriptor object for file */
  char *last_pointer ;		/* Pointer to last GET/PUT buffer */
  char *open_specs_ptr ;		/* Pointer to additional open specifications */
  struct db__module_entry *handler_mep ; /* If not NULL then pointer to handler module for this unit */
  short is_static ;		/* Set to TRUE by user if static */
				/* Bits 1 & 2 used to designate static GET/PUT */
  short shadow_index ;		/* If non-zero then unit is being shadowed to another device */
  short shadow_flag ;		/* Set to shadow_flag in io_put, must be cleared by shadow module otherwise error in next io_put */
  int vms_rfa4 ;		/* First 4 bytes of vms/rms RFA */
  short vms_rfa2 ;		/* Last 2 bytes */
  short pragmas ;		/* Holds pragmatic flags */
  FILE *file_ptr ;		/* Internal file pointer */
#ifdef VMSOS
  struct RAB *rab_pointer ;	/* If not NULL then pointer to RMS rab */
  struct XAB *xabp ;		/* NULL or pointer to first "SET_KEYS" xab */
#endif
  int dflt_fileref ;		/* Default fileref for current access */
  int fileref ;			/* Local fileref (overrides above for "next" io_xxx operation) */
  char lock_ind ;		/* If TRUE then current record is locked (needed by C-ISAM) */
  int PrimarySocket ;		/* Primary socket number */
  int ConnectSocket ;		/* If non-zero then connected socket number */
  int *keydesc_ptr ;		/* Pointer to file's key descriptors (needed by C-ISAM) */
  short last_get_key ;		/* Last get_key (needed by C-ISAM) */
  short over_length ;		/* If nonzero then cutoff record size for overflow file */
  short over_ind ;		/* Index (first byte=0) into overflow indicator byte within record */
#ifdef WINNTxxvv
  HANDLE hfile ;		/* File handle */
#else
  int unitunused1 ;
#endif
  short int header_bytes ;	/* Number of bytes in record header for compaction */
  int dflt_get_key ;		/* Default GET key number */
  int dflt_put_mode ;		/* Default PUT mode */
  int dflt_get_mode ;
  int get_key ;			/* GET key number */
  int put_mode ;		/* PUT mode */
  int get_mode ;
  int get_length ;		/* Updated on GET to length of record */
  int put_count ;		/* Number of PUTs or SOS line number */
  int get_count ;		/* Number of GETs or SOS line number */
  int max_gets ;		/* If nonzero then max number of gets (V3 injects EOF error) */
  int skip_gets ;		/* If nonzero then number of records to skip on each get */
  int user1,user2 ;		/* User defined parameters */
 } ;

/*	iou__fileinfo - format of structure used by sys_info(/next_file/,&xxx)	*/

struct iou__fileinfo
{ 
#ifdef WINNT
  HANDLE internal1 ;
#else
  int internal1 ;		/* Internal use (like handle in WINNT) */
#endif
  char *internal2 ;		/* Internal use (like &glob_t in Unix) */
  char filename[512+1] ;
  char directorypath[128+1] ;
  int updatedt ;
  int filebytes ;
  int filetype ;		/* Type of file, 0 = normal, 1 = directory */
} ;

/*	rpt__master - Report Master				*/

struct rpt__master {
  struct iou__unit unit ;
  char *aux_info ;			/* Pointer to optional info */
  int file_info,record_info ;		/* If nonzero then to set up special report files */
  short int rpt_type ;			/* Type of report (0=normal, 1=spreadsheet format) */
  short int bottom_of_page ;		/* Bottom of page */
  short int page_size ;			/* Lines in page */
  short int line_number ;		/* Current line in page */
  short int page_number ;		/* Current page number */
  short int page_width ;		/* Width of page (default is 132) */
  char bottom_cmd_str[32] ;		/* Command to xct on bottom-of-page */
  char top_cmd_str[32] ;		/* Command to xct on top-of-page */
  short col_count ;			/* Number of columns in list below */
  short col_list[40] ;			/* List of columns below in order of definition */
  int rpt_columns[5] ;			/* BITMAP 100 of columns to output */
  struct {
    short int start[40] ;		/* Start position for column */
    short int length[40] ;		/* Length of column */
    short int coltype[40] ;		/* Type of column - see V3_COLTYP_xxx */
   } col[5] ;
 } ;

/*	rpt__line - Defines a report line			*/

struct rpt__line {
  char flag_byte ;			/* Set to 0XFF as indicator */
  struct rpt__master *master ;		/* Pointer to master for line */
  short int col_set ;			/* Column set to use */
  short int current_index ;		/* Last used report position */
  char buf[250] ;			/* Line buffer */
 } ;

/*	v3__queue_info - Defines Queue Entry	*/

struct v3__queue_info
{ char filename[101] ;			/* File to be queued */
   char queuename[51] ;			/* Queue name */
   char job_name[51] ;			/* Job name */
   char log_name[101] ;			/* Log file name (optional) */
   int submit_date_time ;		/* When to process (0 for immediately) */
   int copies ;				/* Number of copies to print */
   int start_page,end_page ;
   short int notify,burst,banner,passall,paging,restart,delete,hold ;
   char operator[101] ;			/* Special operator message/request */
   char setup[101] ;			/* List of setup modules */
   char forms[51] ;			/* Forms name */
   char notes[101] ;			/* Note to appear on banner */
   char parameters[8][101] ;		/* Job setup parameters */
} ;
/*	num__ref - Internal representation for strings	*/

struct num__ref {
  union val__format format ;
  LONGINT *ptr ;
  double *dbl_ptr ;
  LONGINT value ;			/* Holds actual value if immediate */
 } ;

/*	pd__ref - Internal representation for packed decimal numbers */

struct pd__ref {
  union val__format format ;
  int *ptr ;
  char str_buf[16] ;
 } ;

/*	E R R O R   C O D E S		*/

#define V3E_ARG1NOTSTRING 100000		/* First argument must be SV/STRING */
#define V3E_ARGCNT 100010			/* v4_PointMake requires 3 or 4 arguments */
#define V3E_ARGNOTMOD 100020		/* First arg to MOD_APPLY NOT a module reference */
#define V3E_ARGNOTPTR 100030		/* Second argument must be pointer/V4Point */
#define V3E_ARGTOOLONG 100040		/* Assertion name list too long */
#define V3E_ARITH 100050			/* buf */
#define V3E_ASRTMAX 100060			/* Exceeded maximum number of assertions */
#define V3E_BADAMHOUR 100070		/* Invalid AM hour specification */
#define V3E_BADAMPM 100080			/* Invalid seconds or AM/PM specificat */
#define V3E_BADBPNUM 100090		/* Invalid breakpoint number */
#define V3E_BADCMDARG 100100		/* Argument to CMD_ARG_VALUE is out of range */
#define V3E_BADCODE 100110			/* Invalid code value in v3_operations */
#define V3E_BADDAY 100120			/* Invalid day */
#define V3E_BADESC 100130			/* Invalid input escape sequence */
#define V3E_BADFLAGS 100140		/* Invalid flags argument to prsu_error */
#define V3E_BADFUNC 100150			/* Invalid function code */
#define V3E_BADHOUR 100160			/* Invalid hour specification */
#define V3E_BADID 100170			/* Package id number is out of range */
#define V3E_BADIMPUREPV 100180		/* Invalid address type for IMPUREPV */
#define V3E_BADINFOARG 100190		/* Invalid argument to V3_INFO */
#define V3E_BADKEY 100200			/* Invalid datatype for key */
#define V3E_BADLISTINDEX 100210		/* Index to str_list() cannot be less than one */
#define V3E_BADLOCREF 100220		/* Invalid location reference */
#define V3E_BADLOG 100230			/* Expecting Y/ */
#define V3E_BADMIN 100240			/* Invalid minute specification */
#define V3E_BADMODARG 100250		/* Argument to MOD_ARG_VALUE is out of range */
#define V3E_BADMODE 100260			/* Invalid mode for C handler */
#define V3E_BADMONTH 100270		/* Invalid month */
#define V3E_BADPUREPV 100280		/* Invalid address type for PUREPV */
#define V3E_BADSEC 100290			/* Invalid seconds specification */
#define V3E_BADSETPARAMARG 100300		/* Invalid argument to V3_SET_PARAMETER */
#define V3E_BADSYSINFOARG 100310		/* Invalid argument to SYS_INFO */
#define V3E_BADSYSSETPARAMVAL 100320		/* Invalid argument to SYS_SET_PARAMETER */
#define V3E_BADUPDATE 100330		/* Invalid datatype for update */
#define V3E_BADVAL 100340			/* Invalid value type */
#define V3E_BADVER 100350			/* Version mismatch */
#define V3E_BADYEAR 100360			/* Year must be between 1850 & 2050 */
#define V3E_BCKTFULL 100370		/* Object bucket overflow */
#define V3E_BPNOTSET 100380		/* No breakpoint at specified location or with number */
#define V3E_BUS 100390			/* errbuf */
#define V3E_CHECKPOINT 100400		/* mscu_vms_errmsg(rms_res */
#define V3E_CISAMNYI 100410		/* /new_eof/ function not implemented in C-ISAM */
#define V3E_CLOSEFAIL 100420		/* errbuf */
#define V3E_CLOSESOCKETFAIL 100430		/* Could not close socket */
#define V3E_CMDMAX 100440			/* Too many commands in list */
#define V3E_CREATEPARSE 100450		/* Could not save into parse file */
#define V3E_CREATEXCT 100460		/* Could not save into xct file */
#define V3E_CRESEGFILE 100470		/* Could not create segment file */
#define V3E_CRMPSCFAIL 100480		/* mscu_vms_errmsg(flags */
#define V3E_CRMPSC_HDR 100490		/* mscu_vms_errmsg(vms_res */
#define V3E_CRMPSC_NONSHARE 100500		/* mscu_vms_errmsg(vms_res */
#define V3E_CRMPSC_SHARE 100510		/* mscu_vms_errmsg(vms_res */
#define V3E_DATETOOSHORT 100520		/* Invalid date format") ; * */
#define V3E_DELFAIL 100530			/* mscu_vms_errmsg(rms_res */
#define V3E_DIMNOTINCTX 100540		/* Could not locate {BINDING} point in Isct or Context")  */
#define V3E_DTBADYEAR 100550		/* Year must be between 1850 & 2050 */
#define V3E_DTINVSEP 100560		/* Date portion must be followed by space of colon...") * */
#define V3E_DTTOOSHORT 100570		/* Invalid date format- too short") ; * */
#define V3E_EOF 100580			/* End of file reached */
#define V3E_EXISTS 100585		/* Record already exists */
#define V3E_FILEIO 100590			/* Error(s) reading package information */
#define V3E_FILEOPEN 100600		/* errbuf */
#define V3E_FLDTOOSMALL 100610		/* Field too small to hold pointer */
#define V3E_FORKEXISTS 100620		/* Fork already exists */
#define V3E_GET 100630			/* mscu_vms_errmsg(rms_res */
#define V3E_GETHOSTADDR 100640		/* errbuf */
#define V3E_HDRNOTXCT 100650		/* File not in valid xct load format */
#define V3E_ILL 100660			/* buf */
#define V3E_INCNSDIM 100670		/* Dimension inconsistent with dimension in point */
#define V3E_INCUR 100680			/* Cannot SWITCH to current fork */
#define V3E_INDEXTOOBIG 100690		/* Index to str_list() is too large */
#define V3E_INSDYNMEM 100700		/* Insufficient heap memory for dynamic module invocation */
#define V3E_INUSE 100710			/* Package slot already in use */
#define V3E_INVBIT 100720			/* Bit index out of range for map */
#define V3E_INVCHRRADIX 100730		/* Invalid digit for specified radix */
#define V3E_INVDAYOFMTH 100740		/* Day of month is not valid for specified month */
#define V3E_INVDES 100750			/* Invalid des/v4point length */
#define V3E_INVDEVNAM 100760		/* Invalid device name */
#define V3E_INVDICT 100770			/* Invalid dictionary entry")  */
#define V3E_INVDIM 100780			/* Invalid dimension ID */
#define V3E_INVDIR 100790			/* Invalid directory name specification")  */
#define V3E_INVDISIZE 100800		/* Argument to \'date_info\' module is incorrect length */
#define V3E_INVDTD 100810			/* Invalid universal date in date_time */
#define V3E_INVFORMATMODE 100820		/* Invalid format mode for module call */
#define V3E_INVFUNC 100830			/* Invalid TTY_MISC function */
#define V3E_INVGET 100840			/* Invalid io_get mode specified */
#define V3E_INVHEAPSIZE 100850		/* Argument to HEAP_ALLOC less than 1 */
#define V3E_INVINTUPD 100860		/* Invalid datatype for integer update */
#define V3E_INVITXGENDT 100870		/* Cannot use ITX_GEN on this value type */
#define V3E_INVLEN 100880			/* Invalid length of pointer */
#define V3E_INVMEMCHUNK 100890		/* Invalid pointer to memory chunk */
#define V3E_INVMODE 100900			/* Invalid value mode */
#define V3E_INVMODREF 100910		/* Invalid module reference */
#define V3E_INVMODX 100920			/* Invalid module index */
#define V3E_INVMONTH 100930		/* Month must be between 1 and 12 */
#define V3E_INVNAME 100940			/* Context frame id must be integer */
#define V3E_INVNUMCHAR 100950		/* Invalid character in number */
#define V3E_INVNUMTYPE 100960		/* Invalid datatype for pop_v3_num */
#define V3E_INVNUMVAR 100970		/* Cannot update variable with numeric result")  */
#define V3E_INVPACDEC 100980		/* Invalid value for IP field")  */
#define V3E_INVPCB 100990			/* Invalid size for PCB argument */
#define V3E_INVPCKNUM 101000		/* Package number is out of range */
#define V3E_INVPNTLEN 101010		/* Invalid length for point */
#define V3E_INVPOINTER 101020		/* Invalid format for pointer */
#define V3E_INVPOINTVAL 101030		/* Point value must be string or integer */
#define V3E_INVPT 101040			/* Argument not point/pointer */
#define V3E_INVPUT 101050			/* Invalid io_put mode specified */
#define V3E_INVREALUPD 101060		/* Invalid value for REAL update")  */
#define V3E_INVRPPLEN 101070		/* Invalid length for parse-block */
#define V3E_INVRPPSIZE 101080		/* Invalid size of argument */
#define V3E_INVSETARG 101090		/* Invalid set-code (arg #2) in TKN_SET */
#define V3E_INVSETBPARG 101100		/* Argument to DBG_SETBP is not a code location */
#define V3E_INVSQRTARG 101110		/* Invalid value for REAL argument */
#define V3E_INVSTRINT 101120		/* Invalid value for DEC field")  */
#define V3E_INVSTRPTRVAL 101130		/*  */
#define V3E_INVSYM 101140			/* Invalid character in symbol */
#define V3E_INVSYSADDRESSDT 101150		/* Cannot sys_address() a skeleton! */
#define V3E_INVTCBLEN 101160		/* Invalid length for token-control-block */
#define V3E_INVUD 101170			/* Invalid universal date */
#define V3E_INVV3TOV4DT 101180		/* Invalid resulting datatype */
#define V3E_INVV4TYPE 101190		/* Cannot use v4_MakePoint to make special {xxx} points")  */
#define V3E_INVVECDT 101200		/* Invalid datatype for vector operation */
#define V3E_INVXTIGENDT 101210		/* Cannot use XTI_GEN on this value type */
#define V3E_IOERR 101220			/* ebuf */
#define V3E_LIMIT 101230			/* Reached limit as per unit's max_gets */
#define V3E_LOCKED 101240			/* Record is locked */
#define V3E_LOOPMAX 101250			/* Terminal input loop error */
#define V3E_MAXCPU 101260			/* Exceeded maximum CPU as per mod_watchdog() call */
#define V3E_MAXPROC 101270			/* Exceeded max number of modules per process */
#define V3E_MODHTBAD 101280		/* Module hash table for process is trashed! */
#define V3E_MONTHTOOSHORT 101290		/* Invalid month format")  */
#define V3E_NAMETBLFULL 101300		/* Name table is full */
#define V3E_NEGLEN 101310			/* Length cannot be negative */
#define V3E_NOCHAIN 101320			/* Could not chain to progam */
#define V3E_NOCMD 101330			/* tmp) ; */
#define V3E_NOCREFILMAP 101340		/* ebuf */
#define V3E_NODATE 101350			/* No date given in any of the \'inp_xxx\' elements */
#define V3E_NODCLNAME 101360		/* No filename has been declared for this package slot */
#define V3E_NODELETE 101370		/* Error in sys_delete */
#define V3E_NODP 101380			/* Single decimal point allowed on decimal numbers */
#define V3E_NOEVAL 101390			/* Could not evaluate Isct argument to V3 mod */
#define V3E_NOFILE 101400			/* Could not access include file */
#define V3E_NOFILEREF 101410		/* Could not locate FileRef in file table */
#define V3E_NOFILETOCOMP 101420		/* Could not access V3 file */
#define V3E_NOFILETOLOAD 101430		/* Could not LOAD file */
#define V3E_NOFIND 101440			/* Could not locate object reference */
#define V3E_NOFINDOBJ 101450		/* Cannot find requested object */
#define V3E_NOFORK 101460			/* No such fork */
#define V3E_NOFREE 101470			/* No more available FORK slots */
#define V3E_NOFREEBPS 101480		/* No more breakpoint slots available */
#define V3E_NOGETBUF 101490		/* No get record buffer has been specified */
#define V3E_NOGETMODE 101500		/* No get_mode (/bof */
#define V3E_NOIDOBJ 101510			/* This package has no DCL PACKAGE_ID object */
#define V3E_NOISCT 101520			/* Can only use {BINDING} from within nested intersection evaluation")  */
#define V3E_NOLOAD 101530			/* Cannot LOAD from xct-only package */
#define V3E_NOLOCK 101540			/* msgbuf */
#define V3E_NOLOGICAL 101550		/* npath */
#define V3E_NOLOGOUT 101560		/* Could not logout process family */
#define V3E_NOMACINV4 101570		/* Could not load macro from V4 database */
#define V3E_NOMAPVIEWFILE 101580		/* ebuf */
#define V3E_NOMASTER 101590		/* buf */
#define V3E_NOMODINSTACK 101600		/* Could not find module in current frame stack")  */
#define V3E_NOMODINV4 101610		/* Could not load module from V4 database */
#define V3E_NOMOREMEM 101620		/* Virtual memory mapping or paging quota exceeded */
#define V3E_NONAME 101630			/* No file name has been specified */
#define V3E_NONAMEDOBJ 101640		/* Could not locate named object */
#define V3E_NONESTEDLOGICAL 101650		/* ffn */
#define V3E_NOPACKCISAM 101660		/* Packed decimal keys not supported in C-ISAM */
#define V3E_NOPACKV4 101670		/* Packed decimal keys not supported in V4 */
#define V3E_NOPARENT 101680		/* No parent object for module argument */
#define V3E_NOPCK 101690			/* Package not loaded or invalid object name */
#define V3E_NOPUTBUF 101700		/* No put record buffer has been specified */
#define V3E_NOREC 101710			/* Could not locate/access BIGBUF record */
#define V3E_NORENAME 101720		/* Error in sys_rename */
#define V3E_NOSAVE 101730			/* Cannot save xct-only package */
#define V3E_NOSEG 101740			/* Could not get sharable segment */
#define V3E_NOSEGMENT 101750		/* Segment not entered into V3 table via SEG_SHARE */
#define V3E_NOSHMAT 101760			/* Could not attach segment to process */
#define V3E_NOSHMCTL 101770		/* Could not get segment status info */
#define V3E_NOSKEL 101780			/* No such skeleton STRUCT declared within this package */
#define V3E_NOSKELOBJ 101790		/* Could not locate *object* */
#define V3E_NOSPAWN 101800			/* SYS_SPAWN not implemented on MAC */
#define V3E_NOSTART 101810			/* No startup module declared for package */
#define V3E_NOSTRUCT 101820		/* Do not have pointer to structure buffer??? */
#define V3E_NOSUBS 101830			/* Invalid attempt to subscript non-repeating object")  */
#define V3E_NOTBL 101840			/* Package has no name table */
#define V3E_NOTCUR 101850			/* Cannot delete current or BOOT fork */
#define V3E_NOTCURRENT 101860		/* Cannot unload current package */
#define V3E_NOTDEF 101870			/* tmpstr1 */
#define V3E_NOTENA 101880			/* Cannot call INT_SET_TIMER without enabling /int_timer/ interrupt */
#define V3E_NOTIMP 101890			/* Feature not imnplemented */
#define V3E_NOTINMOD 101900		/* Cannot reference module index outside of module (i.e. from interpretive) */
#define V3E_NOTISCT 101910			/* First argument is not of type ISCT */
#define V3E_NOTLOADED 101920		/* Package not currently loaded */
#define V3E_NOTMODREF 101930		/* Argument to MOD_NAME is not a module reference */
#define V3E_NOTOBJ 101940			/* Argument to OBJ_xxx not object/objref */
#define V3E_NOTOBJREF 101950		/* First argument to OBJ_FIND/OBJ_HAS_FIND must be OBJREF */
#define V3E_NOTOPEN 101960			/* Unit is not open */
#define V3E_NOTOPENRMS 101970		/* Unit not openned as RMS */
#define V3E_NOTPARSE 101980		/* Package not installed with parse info */
#define V3E_NOTPOINT 101990		/* First argument must be a point */
#define V3E_NOTPOS 102000			/* Numeric must be positive */
#define V3E_NOTPRSHDR 102010		/* File contents not valid for parse load */
#define V3E_NOTPSI 102020			/* Third arg must be PS */
#define V3E_NOTPTR 102030			/* Value not a POINTER */
#define V3E_NOTREF 102040			/* Second argument to STR_BREAK(_RIGHT) must be STRING_REF */
#define V3E_NOTSHADOW 102050		/* Attempted io_put without first shadowing */
#define V3E_NOTSR 102060			/* Argument to str_value must be \"dcl sr\" */
#define V3E_NOTSTRING 102070		/* Expecting a string argument */
#define V3E_NOTV4IS 102080			/* Function only supported on V4IS files */
#define V3E_NOTVARSTR 102090		/* Can only \'/concat/\' into STRING */
#define V3E_NOTYETSUP 102100		/* Not yet supported on this version of V3 */
#define V3E_NOTYN 102110			/* Argument to STR_LOGICAL not \'Y\' or \'N\' */
#define V3E_NOTZERO 102120			/* Indexes cannot be zero */
#define V3E_NOUPDATE 102130		/* mscu_vms_errmsg(flags */
#define V3E_NOVALUE 102140			/* Cannot find value for object */
#define V3E_NOWORD 102150			/* Keys of type short are not supported in V4 */
#define V3E_NTHVALUE 102160		/* Object does not have n\'th value */
#define V3E_NYI 102170			/* Feature not yet implemented */
#define V3E_ONEMINUS 102180		/* Only one sign allowed */
#define V3E_ONEPLUS 102190			/* Only one sign allowed */
#define V3E_ONLYCMD 102200			/* CMD_xxx only allowed within command */
#define V3E_ONLYSR 102210			/* The \'==\' operator only allowed on SR/STRING_REF variables */
#define V3E_PCKFULL 102220			/* No more table slots in package */
#define V3E_PCKLOADNOFILE 102230		/* Could not access file to load */
#define V3E_PCKNOTLOADED 102240		/* Package not loaded */
#define V3E_PCKNOUPDATE 102250		/* Package not loaded for updates */
#define V3E_PLOADNOPACK 102260		/* tstr */
#define V3E_PRSERRS 102270			/* Errors detected during compilation (count reset by this error) */
#define V3E_PTRZERO 102280			/* Pointer cannot be equal to 0 */
#define V3E_PUTERR 102290			/* mscu_vms_errmsg(rms_res */
#define V3E_PUTERRC 102300			/* errbuf */
#define V3E_RECONNECT 102310		/* Cannot reconnect non-Server socket */
#define V3E_RECTOOBIG 102320		/* Record too big for buffer */
#define V3E_ROTATENYI 102330		/* BIT_ROTATE not yet implemented */
#define V3E_SOCKETACCEPT 102340		/* errbuf */
#define V3E_SOCKETBIND 102350		/* errbuf */
#define V3E_SOCKETCONNECT 102360		/* errbuf */
#define V3E_SOCKETLISTEN 102370		/* errbuf */
#define V3E_SOCKETMAKE 102380		/* errbuf */
#define V3E_SOCKETNOTSUP 102390		/* Sockets not supported in this version of V3 */
#define V3E_SOCKETRECV 102400		/* errbuf */
#define V3E_SOCKETSEND 102410		/* errbuf */
#define V3E_SRCLONGER 102420		/* Source string longer than destination */
#define V3E_STARTUPMOD 102430		/* Startup module not found in package */
#define V3E_STRINDEXES 102440		/* Start index cannot be greater than end index */
#define V3E_STRUCTPTRZERO 102450		/* Pointer cannot be equal to 0 */
#define V3E_SUBLEZERO 102460		/* Subscript cannot be less than 1 */
#define V3E_SYSERR 102470			/* Error in CreateProcess */
#define V3E_TBLFULL 102480			/* Table is full */
#define V3E_TOOBIG 102490			/* Number too big for integer update */
#define V3E_TOOBIGFORINT 102500		/* Number too big to convert to integer */
#define V3E_TOOBIGTOUPD 102510		/* Number too big for integer update */
#define V3E_TOOLONG 102520			/* String to long to \'/concat/\' */
#define V3E_TOOMANYOBS 102530		/* Too many objects in package bucket */
#define V3E_TOOMANYRETS 102540		/* Too many RETURNs called */
#define V3E_TOOMNYBITS 102550		/* Too many bits for length of string in BITMAP_DCL */
#define V3E_TOOSHORT 102560		/* First argument not long enough */
#define V3E_TRNFAIL 102570			/* mscu_vms_errmsg(rms_res */
#define V3E_UNDEDEL 102580			/* Undefined skeleton element reference */
#define V3E_UNDEFMOD 102590		/* Undefined module reference */
#define V3E_UNKBREAK 102600		/* Encountered BREAK instruction from unknown location */
#define V3E_UNLOCK 102610			/* mscu_vms_errmsg(rms_res */
#define V3E_UPDOBJREF 102620		/*  */
#define V3E_V3OPREF 102630			/* Cannot reference V3 operator from module call */
#define V3E_VALCHANGE 102640		/* Watched variable changed */
#define V3E_VMSERR 102650			/* mscu_vms_errmsg(i */
#define V3E_XLOADNOPACK 102660		/* ebuf */
#define V3E_ZEROSTART 102670		/* Start index cannot be zero */


#include "v3prsdef.c"
#include "v3proto.c"