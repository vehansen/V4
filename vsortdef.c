/*	VSORTDEFS.C - Sort Definitions for VSort

	Created 19-Apr-94 by Victor E. Hansen		*/

#define VSRT_KeyType_Int 1		/* Integer key */
#define VSRT_KeyType_Long 2		/* Long (64 bits on alpha) key */
#define VSRT_KeyType_Alpha 3		/* Alpha String */
#define VSRT_KeyType_VAlpha 4		/* Null terminated alpha string */
#define VSRT_KeyType_Double 5		/* Floating point key */
#define VSRT_KeyType_Short 6		/* Short (16 bits) integer */
#define VSRT_KeyType_Key 7		/* A V4IS DataIdKey */
#define VSRT_KeyType_Fixed 8		/* A 64-bit fixed decimal (implied) number */
#define VSRT_KeyType_UCChar 9		/* Unicode string */
#define VSRT_KeyType_V4TallyList 10	/* V4 Tally-List structure (2 32-bit ints treated as one 64-bit */

#define VSRT_CB_KeyMax 32		/* Max number of sort keys */

#define V_IFT_FixBin 1			/* Input is fixed length binary, sorting entire file */
#define V_IFT_Text 2			/* Input is newline delimited text file */
#define V_IFT_V4Seq 3			/* Input is V4IS Sequential format */
#define V_IFT_V4IS 4			/* Input is V4IS Index area */
#define V_IFT_FixBinBig 5		/* Input is fixed binary, sorting only keys */
#define V_IFT_V4Key 6			/* Input is V4 Key format (series of V4IS__DataIdKey records) */

#define V_Text_MaxBytes 1024		/* Max number of bytes in a text file record */
#define V_V4Seq_MaxBytes 0xffff		/* Max number of bytes in a V4IS sequential record */

struct VSRT__ControlBlock
{ int TotalBytes ;			/* Total bytes in sort buffer */
  int TotalCompares ;			/* Total number of comparisons needed */
  int InputBytes ;			/* Number of bytes in an input record */
  int EntryBytes ;			/* Number bytes in each sort entry */
  int EntryCount ;			/* Number of entries to be sorted */
  int MaxEntriesToSort ;		/* Maximum entries to be sorted */
  int StartEntryToSort ;		/* Starting entry to sort */
  UCCHAR InputFileName[V_FileName_Max] ;/* Name of the input file */
  UCCHAR OutputFileName[V_FileName_Max] ; /* Name of the output file */
  int InputFileType ;			/* Type of input file: V_IFT_xxx */
  int DeleteInput ;			/* if TRUE then delete input file when done sort */
  int Quiet ;				/* If TRUE then run silently */
  int NoIO ;				/* If TRUE then vsort (as subroutine) has been passed records to sort in buffer */
  FILE *ifp ;				/* Input file pointer for reading text files */
  int ifd,ofd ;
#ifdef WINNT
  HANDLE ihandle,ohandle ;
#endif
  char *InternalBuffer ;		/* Pointer to internal buffer */
  int EntryPosOffset ;			/* Offset into internal buffer entry of record key */
  int WorkSetMax ;			/* Maximum working size before sorting just keys */
  int OffsetBase ;			/* Offset for key positions (usually 0 or 1) */
  int KeyCount ;			/* Number of keys below */
  struct {
   int Type ;				/* Type of key: VSRT_KeyType_xxx */
   int Alignment ;			/* Alignment for key type */
   int Bytes ;				/* Number of bytes in key (characters if Unicode) */
   int Offset1 ;			/* Offset (bytes) to the key in the input file */
   int Offset2 ;			/* Offset to the key in an entry (may be different then above if only tracking keys) */
   int Decending ;			/* TRUE if sorting in reverse order */
  } Key[VSRT_CB_KeyMax] ;
} ;

struct VSRT__KeyList {
  int FreeOffset ;			/* Next free byte in record */
  int Count ;				/* Number of keys */
  struct {
    int Type ;				/* Key type- VSRT_KeyType_xxx */
    int Offset ;			/* Offset (bytes) to key */
    int Bytes ;				/* Number of bytes in key */
    int Descending ;			/* if TRUE then sort descending order */
   } Keys[10] ;
} ;
