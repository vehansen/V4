//V4 interface
LENMAX cbX ;			/* Current index into output buffer */
char *cbB ;			/* Pointer to output buffer */
LENMAX cbMax ;			/* Maximum size of output buffer */

#define putbyte(B) (cbX >= cbMax ? EOF : ((cbB[cbX++] = B),0))

//MODERN
#        define __ARGS__(x) x
//END MODERN

//ZALLOC
#  define zalloc(p,n,s) calloc((long)(n),(s));
#  define zfree(p)      free((void huge *)(p));
//END ZALLOC

//ZIPDEFS
#ifndef WSIZE
#  define WSIZE 0x8000
#endif
/* Maximum window size = 32K. If you are really short of memory, you may
 * compile with a smaller WSIZE but this reduces the compression ratio for
 * files of size > WSIZE.
 * Note, the above notice valid for deflation (compression) process only.
 * Inflation (decompression) process always requires at least 32K window.
 * WSIZE must be a power of two in the current implementation.
 */

/* Types centralized here for easy modification */
typedef unsigned char uch;  /* unsigned 8-bit value */
typedef unsigned short ush; /* unsigned 16-bit value */
typedef unsigned long ulg;  /* unsigned 32-bit value */
//END ZIPDEFS


//OSCODE
#  define OS_CODE 11
//END OSCODE

//LZPIPE
#define ZIP_ANY 0
#define ZIP_PKW 1

//ZIPGUTS
#define OF __ARGS__

#define MIN_MATCH  3
#define MAX_MATCH  258
/* The minimum and maximum match lengths */

#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)
/* Minimum amount of lookahead, except at the end of the input file.
   See deflate.c for comments about the MIN_MATCH+1. */

#define MAX_DIST  (WSIZE-MIN_LOOKAHEAD)
/* In order to simplify the code, particularly on 16 bit machines, match
   distances are limited to MAX_DIST instead of WSIZE. */

int deflate_level=6;
extern ulg compressed_len;


/* Diagnostic functions */
#ifdef DEBUG
# ifdef OS2
#  undef  stderr
#  define stderr stdout
# endif
#  define Assert(cond,msg) {if(!(cond)) error(msg);}
#  define Trace(x) fprintf x
#  define Tracev(x) {if (verbose) fprintf x ;}
#  define Tracevv(x) {if (verbose>1) fprintf x ;}
#  define Tracec(c,x) {if (verbose && (c)) fprintf x ;}
#  define Tracecv(c,x) {if (verbose>1 && (c)) fprintf x ;}
#else
#  define Assert(cond,msg)
#  define Trace(x)
#  define Tracev(x)
#  define Tracevv(x)
#  define Tracec(c,x)
#  define Tracecv(c,x)
#endif

/* Public function prototypes */

void error OF((char *h));


        /* in deflate.c */
int lm_init OF((void));
int fast_deflate OF((char*, unsigned));
int lazy_deflate OF((char*, unsigned));

        /* in trees.c */
int  ct_init  OF((void));
int  ct_tally OF((int dist, int lc));
ulg  flush_block OF((char *buf, ulg stored_len, int eof));

        /* in bits.c */
void bi_init   OF((void));
int send_bits  OF((unsigned value, int length));
int bi_windup  OF((void));
int bi_putsh   OF((unsigned short));
int copy_block OF((char *buf, unsigned len, int header));
//END ZIPGUTS

#define LZW_ANYSIZE 0x7fffffffL



#define ZWRITE 9 /* Error writing output file */
//END LZPIPE

//	C PROGRAMS

//begin bits
static unsigned bitbuff;
static int boffset;

#ifdef DEBUG
ulg bits_sent;   /* bit length of the compressed data */
#endif

void bi_init() /* Initialize the bit string routines. */
{
   bitbuff = 0;
   boffset = 0;
#ifdef DEBUG
   bits_sent = 0L;
#endif
}

int send_bits(value, length) /* Send a value on a given number of bits. */
unsigned value; /* value to send */
int length;     /* number of bits: length =< 16 */
{
#ifdef DEBUG
   Tracevv((stderr," l %2d v %4x ", length, value));
   Assert(length > 0 && length <= 15, "invalid length");
   Assert(boffset < 8, "bad offset");
   bits_sent += (ulg)length;
#endif
   bitbuff |= value << boffset;
   if ((boffset += length) >= 8) {
      if (putbyte(bitbuff) == EOF) return -1;
      value >>= length - (boffset -= 8);
      if (boffset >= 8) {
         boffset -= 8;
         if (putbyte(value) == EOF) return -1;
         value >>= 8;
      }
      bitbuff = value;
   }
   return 0;
}

/* Write out any remaining bits in an incomplete byte. */
int bi_windup()
{
   Assert(boffset < 8, "bad offset");
   if (boffset) {
      if (putbyte(bitbuff) == EOF) return -1;
      boffset = 0;
      bitbuff = 0;
#ifdef DEBUG
      bits_sent = (bits_sent+7) & ~7;
#endif
   }
   return 0;
}

int bi_putsh(x)
unsigned short x;
{
   return (putbyte(x&255)==EOF || putbyte((x>>8)&255)==EOF) ? -1 : 0;
}

/* Copy a stored block to the zip file, storing first the length and its
   one's complement if requested. */
int copy_block(buf, len, header)
char *buf; /* the input data */
unsigned len;  /* its length */
int header;    /* true if block header must be written */
{
   /* align on byte boundary */
   if (bi_windup() != 0) return -1;

   if (header) {
      if (bi_putsh(len) != 0 || bi_putsh(~len) != 0) return -1;
#ifdef DEBUG
      bits_sent += 2*16;
#endif
   }
   while (len--) {
      if (putbyte(*buf++) == EOF) return -1;
   }
#ifdef DEBUG
   bits_sent += (ulg)len<<3;
#endif
   return 0;
}


//begin deflate
/*
 The following sorce code is derived from Info-Zip 'zip' 2.01
 distribution copyrighted by Mark Adler, Richard B. Wales,
 Jean-loup Gailly, Kai Uwe Rommel, Igor Mandrichenko and John Bush.
*/

/*
 *  deflate.c by Jean-loup Gailly.
 *
 *  PURPOSE
 *
 *      Identify new text as repetitions of old text within a fixed-
 *      length sliding window trailing behind the new text.
 *
 *  DISCUSSION
 *
 *      The "deflation" process depends on being able to identify portions
 *      of the input text which are identical to earlier input (within a
 *      sliding window trailing behind the input currently being processed).
 *
 *      The most straightforward technique turns out to be the fastest for
 *      most input files: try all possible matches and select the longest.
 *      The key feature of this algorithm is that insertions into the string
 *      dictionary are very simple and thus fast, and deletions are avoided
 *      completely. Insertions are performed at each input character, whereas
 *      string matches are performed only when the previous match ends. So it
 *      is preferable to spend more time in matches to allow very fast string
 *      insertions and avoid deletions. The matching algorithm for small
 *      strings is inspired from that of Rabin & Karp. A brute force approach
 *      is used to find longer strings when a small match has been found.
 *      A similar algorithm is used in comic (by Jan-Mark Wams) and freeze
 *      (by Leonid Broukhis).
 *         A previous version of this file used a more sophisticated algorithm
 *      (by Fiala and Greene) which is guaranteed to run in linear amortized
 *      time, but has a larger average cost, uses more memory and is patented.
 *      However the F&G algorithm may be faster for some highly redundant
 *      files if the parameter max_chain_length (described below) is too large.
 *
 *  ACKNOWLEDGEMENTS
 *
 *      The idea of lazy evaluation of matches is due to Jan-Mark Wams, and
 *      I found it in 'freeze' written by Leonid Broukhis.
 *      Thanks to many info-zippers for bug reports and testing.
 *
 *  REFERENCES
 *
 *      APPNOTE.TXT documentation file in PKZIP 1.93a distribution.
 *
 *      A description of the Rabin and Karp algorithm is given in the book
 *         "Algorithms" by R. Sedgewick, Addison-Wesley, p252.
 *
 *      Fiala,E.R., and Greene,D.H.
 *         Data Compression with Finite Windows, Comm.ACM, 32,4 (1989) 490-595
 *
 *  INTERFACE
 *
 *      int lm_init(void)
 *          Initialize the "longest match" routines for a new file
 *
 *      int fast_deflate(char*, unsigned)
 *      int lazy_deflate(char*, unsigned)
 *          Processes a new input file and return its compressed length. Sets
 *          the compressed length, crc, deflate flags and internal file
 *          attributes.
 */



/* Compile with MEDIUM_MEM to reduce the memory requirements or
 * with SMALL_MEM to use as little memory as possible. Use BIG_MEM if the
 * entire input file can be held in memory (not possible on 16 bit systems).
 * Warning: defining these symbols affects HASH_BITS (see below) and thus
 * affects the compression ratio. The compressed output
 * is still correct, and might even be smaller in some cases.
 */

#ifdef SMALL_MEM
#   define HASH_BITS  13  /* Number of bits used to hash strings */
#endif
#ifdef MEDIUM_MEM
#   define HASH_BITS  14
#endif
#ifndef HASH_BITS
#   define HASH_BITS  15
   /* For portability to 16 bit machines, do not use values above 15. */
#endif

#define HASH_SIZE (unsigned)(1<<HASH_BITS)
#define HASH_MASK (HASH_SIZE-1)
#define WMASK     (WSIZE-1)
/* HASH_SIZE and WSIZE must be powers of two */

#define NIL 0
/* Tail of hash chains */

#ifndef TOO_FAR
#  define TOO_FAR 4096
#endif
/* Matches of length 3 are discarded if their distance exceeds TOO_FAR */

#ifdef ATARI_ST
   /* (but the Atari should never define MSDOS anyway ...) */
#endif

/* Local data used by the "longest match" routines. */

#if defined(BIG_MEM) || defined(MMAP)
  typedef unsigned Pos; /* must be at least 32 bits */
#else
  typedef ush Pos;
#endif
typedef unsigned IPos;
/* A Pos is an index in the character window. We use short instead of int to
 * save space in the various tables. IPos is used only for parameter passing.
 */

  uch    window[2L*WSIZE];
  /* Sliding window. Input bytes are read into the second half of the window,
   * and move to the first half later to keep a dictionary of at least WSIZE
   * bytes. With this organization, matches are limited to a distance of
   * WSIZE-MAX_MATCH bytes, but this ensures that IO is always
   * performed with a length multiple of the block size. Also, it limits
   * the window size to 64K, which is quite useful on MSDOS.
   * To do: limit the window size to WSIZE+BSZ if SMALL_MEM (the code would
   * be less efficient since the data would have to be copied WSIZE/BSZ times)
   */
  Pos    prev[WSIZE];
  /* Link to older string with same hash index. To limit the size of this
   * array to 64K, this link is maintained only for the last 32K strings.
   * An index in this array is thus a window index modulo 32K.
   */
  Pos    head[HASH_SIZE];
  /* Heads of the hash chains or NIL. If your compiler thinks that
   * HASH_SIZE is a dynamic value, recompile with -DDYN_ALLOC.
   */
#  define p_win  window
#  define p_prev prev
#  define p_head head

#define WINDOW_SIZE ((ulg)2*WSIZE)

long block_start;
/* window position at the beginning of the current output block. Gets
 * negative when the window is moved backwards. */

static unsigned ins_h;  /* hash index of string to be inserted */
static char uptodate;   /* hash preparation flag */

#define H_SHIFT  ((HASH_BITS+MIN_MATCH-1)/MIN_MATCH)
/* Number of bits by which ins_h and del_h must be shifted at each
 * input step. It must be such that after MIN_MATCH steps, the oldest
 * byte no longer takes part in the hash key, that is:
 *   H_SHIFT * MIN_MATCH >= HASH_BITS */

unsigned int prev_length;
/* Length of the best match at previous step. Matches not greater than this
 * are discarded. This is used in the lazy match evaluation. */

       unsigned strstart;    /* start of string to insert */
       unsigned match_start; /* start of matching string */
static unsigned lookahead;   /* number of valid bytes ahead in window */
       unsigned minlookahead;

unsigned max_chain_length;
/* To speed up deflation, hash chains are never searched beyond this length.
 * A higher limit improves compression ratio but degrades the speed. */

static unsigned int max_lazy_match;
/* Attempt to find a better match only when the current match is strictly
 * smaller than this value. This mechanism is used only for compression
 * levels >= 4. */

#define max_insert_length  max_lazy_match
/* Insert new strings in the hash table only if the match length
 * is not greater than this length. This saves time but degrades compression.
 * max_insert_length is used only for compression levels <= 3. */

unsigned good_match;
/* Use a faster search when the previous match is longer than this */

/* Values for max_lazy_match, good_match and max_chain_length, depending on
 * the desired pack level (0..9). The values given below have been tuned to
 * exclude worst case performance for pathological files. Better values may
 * be found for specific files. */

typedef struct config {
   ush good_length; /* reduce lazy search above this match length */
   ush max_lazy;    /* do not perform lazy search above this match length */
   ush nice_length; /* quit search above this match length */
   ush max_chain;
} config;

#ifdef  FULL_SEARCH
# define nice_match MAX_MATCH
#else
  int nice_match; /* Stop searching when current match exceeds this */
#endif

static config configuration_table[10] = {
/*      good lazy nice chain */
/* 0 */ {0,    0,  0,    0},  /* store only */
/* 1 */ {4,    4,  8,    4},  /* maximum speed, no lazy matches */
/* 2 */ {4,    5, 16,    8},
/* 3 */ {4,    6, 32,   32},

/* 4 */ {4,    4, 16,   16},  /* lazy matches */
/* 5 */ {8,   16, 32,   32},
/* 6 */ {8,   16, 128, 128},
/* 7 */ {8,   32, 128, 256},
/* 8 */ {32, 128, 258, 1024},
/* 9 */ {32, 258, 258, 4096}}; /* maximum compression */

/* Note: the deflate() code requires max_lazy >= MIN_MATCH and max_chain >= 4
 * For deflate_fast() (levels <= 3) good is ignored and lazy has a different
 * meaning. */

/* Prototypes for local functions. */
static unsigned fill_window OF((char*, unsigned));
static void     init_hash   OF((void));

extern int longest_matchZ OF((IPos cur_match));
#ifdef DEBUG
static void check_match OF((IPos start, IPos match, int length));
#endif

/* Update a hash value with the given input byte
 * IN  assertion: all calls to to UPDATE_HASH are made with consecutive
 *    input characters, so that a running hash key can be computed from the
 *    previous key instead of complete recalculation each time. */
#define UPDATE_HASH(h,c) (h = (((h)<<H_SHIFT) ^ (c)) & HASH_MASK)

/* Insert string s in the dictionary and set match_head to the previous head
 * of the hash chain (the most recent string with same hash key). Return
 * the previous length of the hash chain.
 * IN  assertion: all calls to to INSERT_STRING are made with consecutive
 *    input characters and the first MIN_MATCH bytes of s are valid
 *    (except for the last MIN_MATCH-1 bytes of the input file). */
#define INSERT_STRING(s, match_head) \
   (UPDATE_HASH(ins_h, window[(s) + MIN_MATCH-1]), \
    prev[(s) & WMASK] = match_head = head[ins_h], \
    head[ins_h] = (s))

static void init_hash()
{
   register unsigned j;

   for (ins_h=0, j=0; j<MIN_MATCH-1; j++) UPDATE_HASH(ins_h, window[j]);
   /* If lookahead < MIN_MATCH, ins_h is garbage, but this is
      not important since only literal bytes will be emitted. */
}


/* A block of local deflate process data to be saved between
 * sequential calls to deflate functions */
static int match_available = 0; /* set if previous match exists */
static unsigned match_length = MIN_MATCH-1; /* length of best match */

/* Initialize the "longest match" routines for a new file */
int lm_init(void)
{
#ifdef ZMEM
   register unsigned j;
#endif
   /* Initialize the hash table (avoiding 64K overflow for 16 bit systems).
    * prev[] will be initialized on the fly. */
#ifdef ZMEM
   j = 0; do head[j] = NIL; while (++j < HASH_SIZE);
#else
   head[HASH_SIZE-1] = NIL;
   memset((char*)head, NIL, (unsigned)(HASH_SIZE-1)*sizeof(*head));
#endif
   /* Set the default configuration parameters: */
   max_lazy_match   = configuration_table[deflate_level].max_lazy;
   good_match       = configuration_table[deflate_level].good_length;
#ifndef FULL_SEARCH
   nice_match       = configuration_table[deflate_level].nice_length;
#endif
   max_chain_length = configuration_table[deflate_level].max_chain;

   strstart = 0;
   block_start = 0L;
   lookahead = 0;
   uptodate  = 0;
   minlookahead = MIN_LOOKAHEAD-1;
   match_available = 0;
   prev_length = MIN_MATCH-1;
   return 0;
}

/* Set match_start to the longest match starting at the given string and
 * return its length. Matches shorter or equal to prev_length are discarded,
 * in which case the result is equal to prev_length and match_start is
 * garbage.
 * IN assertions: cur_match is the head of the hash chain for the current
 *   string (strstart) and its distance is <= MAX_DIST, and prev_length >= 1
 */

/* For MSDOS, OS/2 and 386 Unix, an optimized version is in match.asm or
 * match.s. The code is functionally equivalent, so you can use the C version
 * if desired.  A 68000 version is in amiga/match_68.a -- this could be used
 * with other 68000 based systems such as Macintosh with a little effort.
 */
int longest_matchZ(cur_match)
    IPos cur_match;                             /* current match */
{
   unsigned chain_length = max_chain_length;   /* max hash chain length */
   register uch *scan = window + strstart;     /* current string */
   register uch *match;                        /* matched string */
   register int len;                           /* length of current match */
   int best_len = prev_length;                 /* best match length so far */
   IPos limit = strstart > (IPos)MAX_DIST ? strstart - (IPos)MAX_DIST : NIL;
   /* Stop when cur_match becomes <= limit. To simplify the code,
      we prevent matches with the string of window index 0. */

/* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
 * It is easy to get rid of this optimization if necessary. */
#if HASH_BITS < 8 || MAX_MATCH != 258
   #error Code too clever
#endif

#ifdef UNALIGNED_OK
   /* Compare two bytes at a time. Note: this is not always beneficial.
      Try with and without -DUNALIGNED_OK to check. */
   register uch *strend = window + strstart + MAX_MATCH - 1;
   register ush scan_start = *(ush*)scan;
   register ush scan_end   = *(ush*)(scan+best_len-1);
#else
   register uch *strend = window + strstart + MAX_MATCH;
   register uch scan_end1 = scan[best_len-1];
   register uch scan_end  = scan[best_len];
#endif

   /* Do not waste too much time if we already have a good match: */
   if (prev_length >= good_match) {
       chain_length >>= 2;
   }
   Assert(strstart <= WINDOW_SIZE-MIN_LOOKAHEAD, "insufficient lookahead");

   do {
       Assert(cur_match < strstart, "no future");
       match = window + cur_match;

       /* Skip to next match if the match length cannot increase
        * or if the match length is less than 2:
        */
#if (defined(UNALIGNED_OK) && MAX_MATCH == 258)
       /* This code assumes sizeof(unsigned short) == 2. Do not use
        * UNALIGNED_OK if your compiler uses a different size.
        */
       if (*(ush*)(match+best_len-1) != scan_end ||
           *(ush*)match != scan_start) continue;

       /* It is not necessary to compare scan[2] and match[2] since they are
        * always equal when the other bytes match, given that the hash keys
        * are equal and that HASH_BITS >= 8. Compare 2 bytes at a time at
        * strstart+3, +5, ... up to strstart+257. We check for insufficient
        * lookahead only every 4th comparison; the 128th check will be made
        * at strstart+257. If MAX_MATCH-2 is not a multiple of 8, it is
        * necessary to put more guard bytes at the end of the window, or
        * to check more often for insufficient lookahead.
        */
       scan++, match++;
       do {
       } while (*(ush*)(scan+=2) == *(ush*)(match+=2) &&
                *(ush*)(scan+=2) == *(ush*)(match+=2) &&
                *(ush*)(scan+=2) == *(ush*)(match+=2) &&
                *(ush*)(scan+=2) == *(ush*)(match+=2) &&
                scan < strend);
       /* The funny "do {}" generates better code on most compilers */

       /* Here, scan <= window+strstart+257 */
       Assert(scan <= window+(unsigned)(WINDOW_SIZE-1), "wild scan");
       if (*scan == *match) scan++;

       len = (MAX_MATCH - 1) - (int)(strend-scan);
       scan = strend - (MAX_MATCH-1);

#else /* UNALIGNED_OK */

       if (match[best_len]   != scan_end  ||
           match[best_len-1] != scan_end1 ||
           *match            != *scan     ||
           *++match          != scan[1])      continue;

       /* The check at best_len-1 can be removed because it will be made
        * again later. (This heuristic is not always a win.)
        * It is not necessary to compare scan[2] and match[2] since they
        * are always equal when the other bytes match, given that
        * the hash keys are equal and that HASH_BITS >= 8.
        */
       scan += 2, match++;

       /* We check for insufficient lookahead only every 8th comparison;
        * the 256th check will be made at strstart+258.
        */
       do {
       } while (*++scan == *++match && *++scan == *++match &&
                *++scan == *++match && *++scan == *++match &&
                *++scan == *++match && *++scan == *++match &&
                *++scan == *++match && *++scan == *++match &&
                scan < strend);

       len = MAX_MATCH - (int)(strend - scan);
       scan = strend - MAX_MATCH;

#endif /* UNALIGNED_OK */

       if (len > best_len) {
           match_start = cur_match;
           best_len = len;
           if (len >= nice_match) break;
#ifdef UNALIGNED_OK
           scan_end = *(ush*)(scan+best_len-1);
#else
           scan_end1  = scan[best_len-1];
           scan_end   = scan[best_len];
#endif
       }
   } while ((cur_match = prev[cur_match & WMASK]) > limit
            && --chain_length != 0);

   return best_len;
}

#ifdef DEBUG
/* Check that the match at match_start is indeed a match. */
static void check_match(start, match, length)
IPos start, match;
int length;
{
#if ZMEM
   register j;
   for(j=0; j<length && window[match+j] == window[start+j]; j++);
   if (j < length)
#else
   if (memcmp((char*)window + match, (char*)window + start, length) != 0)
#endif
   {
      fprintf(stderr, " start %d, match %d, length %d\n",
         start, match, length);
      error("invalid match");
   }
   if (verbose > 1) {
      fprintf(stderr,"\\[%d,%d]", start-match, length);
      do { putc(window[start++], stderr); } while (--length != 0);
   }
}
#else
#  define check_match(start, match, length)
#endif

/* Add a block of data into the window. Updates strstart and lookahead.
 * IN assertion: lookahead < MIN_LOOKAHEAD.
 * Note: call with either lookahead == 0 or length == 0 is valid
 */
static unsigned fill_window(buffer, length)
char *buffer; unsigned length;
{
   register unsigned n, m;
   unsigned more = length;

   /* Amount of free space at the end of the window. */
   if (WINDOW_SIZE - lookahead - strstart < more) {
      more = (unsigned)(WINDOW_SIZE - lookahead - strstart);
   }
   /* If the window is almost full and there is insufficient lookahead,
    * move the upper half to the lower one to make room in the upper half.
    */
   if (strstart >= WSIZE+MAX_DIST) {
#ifdef ZMEM
      n = 0; do window[n] = window[n+WSIZE]; while (++n < WSIZE);
#else
      memcpy((char*)window, (char*)window+WSIZE, (unsigned)WSIZE);
#endif
      match_start -= WSIZE;
      strstart    -= WSIZE; /* we now have strstart >= MAX_DIST: */

      block_start -= (long) WSIZE;

      for (n = 0; n < HASH_SIZE; n++) {
         m = head[n];
         head[n] = (Pos)(m >= WSIZE ? m-WSIZE : NIL);
      }
      for (n = 0; n < WSIZE; n++) {
         m = prev[n];
         prev[n] = (Pos)(m >= WSIZE ? m-WSIZE : NIL);
         /* If n is not on any hash chain, prev[n] is garbage but
            its value will never be used. */
      }
      if ((more += WSIZE) > length) more = length;
   }
   if (more) {
#ifdef ZMEM
      register char *p = (char*)window+strstart+lookahead;

      n = more; do *p++ = *buffer++; while (--n);
#else
      (void)memcpy((char*)window+strstart+lookahead, buffer, more);
#endif
      lookahead += more;
   }
   return more;
}

/* Flush the current block, with given end-of-file flag.
   IN assertion: strstart is set to the end of the current match. */
#define FLUSH_BLOCK(eof) flush_block(block_start >= 0L ?\
        (char*)&window[(unsigned)block_start] : \
        (char*)NULL, (long)strstart - block_start, (eof))

/* Processes a new input block.
 * This function does not perform lazy evaluationof matches and inserts
 * new strings in the dictionary only for unmatched strings or for short
 * matches. It is used only for the fast compression options. */
int fast_deflate(buffer, length)
char *buffer; unsigned length;
{
   IPos hash_head; /* head of the hash chain */
   int flush;      /* set if current block must be flushed */
   unsigned accepted = 0;

   do {
      /* Make sure that we always have enough lookahead, except
       * at the end of the input file. We need MAX_MATCH bytes
       * for the next match, plus MIN_MATCH bytes to insert the
       * string following the next match. */
      accepted += fill_window(buffer+accepted, length-accepted);
      if (lookahead <= minlookahead) break;
      if (!uptodate) {
         match_length = 0; init_hash(); uptodate = 1;
      }
      while (lookahead > minlookahead) {
         /* Insert the string window[strstart .. strstart+2] in the
          * dictionary, and set hash_head to the head of the hash chain:
          */
         INSERT_STRING(strstart, hash_head);

         /* Find the longest match, discarding those <= prev_length.
          * At this point we have always match_length < MIN_MATCH */
         if (hash_head != NIL && strstart - hash_head <= MAX_DIST) {
            /* To simplify the code, we prevent matches with the string
             * of window index 0 (in particular we have to avoid a match
             * of the string with itself at the start of the input file).
             */
            match_length = longest_matchZ(hash_head);
            /* longest_matchZ() sets match_start */
            if (match_length > lookahead) match_length = lookahead;
         }
         if (match_length >= MIN_MATCH) {
            check_match(strstart, match_start, match_length);

            flush = ct_tally(strstart-match_start, match_length - MIN_MATCH);

            lookahead -= match_length;

            /* Insert new strings in the hash table only if the match length
             * is not too large. This saves time but degrades compression.
             */
            if (match_length <= max_insert_length) {
                match_length--; /* string at strstart already in hash table */
                do {
                    strstart++;
                    INSERT_STRING(strstart, hash_head);
                    /* strstart never exceeds WSIZE-MAX_MATCH, so there are
                     * always MIN_MATCH bytes ahead. If lookahead < MIN_MATCH
                     * these bytes are garbage, but it does not matter since
                     * the next lookahead bytes will be emitted as literals.
                     */
                } while (--match_length != 0);
                strstart++;
            } else {
                strstart += match_length;
                match_length = 0;
                ins_h = window[strstart];
                UPDATE_HASH(ins_h, window[strstart+1]);
#if MIN_MATCH != 3
                Call UPDATE_HASH() MIN_MATCH-3 more times
#endif
            }
         } else {
            /* No match, output a literal byte */
            Tracevv((stderr,"%c",window[strstart]));
            flush = ct_tally (0, window[strstart]);
            lookahead--;
            strstart++;
         }
         if (flush) {
            if (FLUSH_BLOCK(0) == -1L) goto error;
            block_start = strstart;
         }
      }
   } while (accepted < length);
   if (!minlookahead) {/* eof achieved */
      if (FLUSH_BLOCK(1) == -1L) goto error;
   }
   return accepted;
error:
   return -1;
}

/* Same as above, but achieves better compression. We use a lazy
 * evaluation for matches: a match is finally adopted only if there is
 * no better match at the next window position.  */
int lazy_deflate(buffer, length)
char *buffer; unsigned length;
{
   IPos hash_head;          /* head of hash chain */
   IPos prev_match;         /* previous match */
   int flush;               /* set if current block must be flushed */
   register unsigned ml = match_length; /* length of best match */
#ifdef DEBUG
   extern ulg isize;        /* byte length of input file, for debug only */
#endif
   unsigned accepted = 0;

   /* Process the input block. */
   do {
      /* Make sure that we always have enough lookahead, except
       * at the end of the input file. We need MAX_MATCH bytes
       * for the next match, plus MIN_MATCH bytes to insert the
       * string following the next match. */
      accepted += fill_window(buffer+accepted, length-accepted);
      if (lookahead <= minlookahead) break;
      if (!uptodate) {
         ml = MIN_MATCH-1; /* length of best match */
         init_hash();
         uptodate = 1;
      }
      while (lookahead > minlookahead) {
         INSERT_STRING(strstart, hash_head);

         /* Find the longest match, discarding those <= prev_length. */
         prev_length = ml, prev_match = match_start;
         ml = MIN_MATCH-1;

         if (hash_head != NIL && prev_length < max_lazy_match &&
             strstart - hash_head <= MAX_DIST) {
            /* To simplify the code, we prevent matches with the string
             * of window index 0 (in particular we have to avoid a match
             * of the string with itself at the start of the input file).
             */
            ml = longest_matchZ (hash_head);
            /* longest_matchZ() sets match_start */
            if (ml > lookahead) ml = lookahead;

            /* Ignore a length 3 match if it is too distant: */
            if (ml == MIN_MATCH && strstart-match_start > TOO_FAR){
               /* If prev_match is also MIN_MATCH, match_start is garbage
                  but we will ignore the current match anyway. */
               ml--;
            }
         }
         /* If there was a match at the previous step and the current
            match is not better, output the previous match: */
         if (prev_length >= MIN_MATCH && ml <= prev_length) {

            check_match(strstart-1, prev_match, prev_length);

            flush = ct_tally(strstart-1-prev_match, prev_length - MIN_MATCH);

            /* Insert in hash table all strings up to the end of the match.
             * strstart-1 and strstart are already inserted.
             */
            lookahead -= prev_length-1;
            prev_length -= 2;
            do {
               strstart++;
               INSERT_STRING(strstart, hash_head);
               /* strstart never exceeds WSIZE-MAX_MATCH, so there are
                * always MIN_MATCH bytes ahead. If lookahead < MIN_MATCH
                * these bytes are garbage, but it does not matter since the
                * next lookahead bytes will always be emitted as literals.
                */
            } while (--prev_length != 0);
            match_available = 0;
            ml = MIN_MATCH-1;
            strstart++;
            if (flush) {
               if (FLUSH_BLOCK(0) == -1L) goto error;
               block_start = strstart;
            }

         } else if (match_available) {
            /* If there was no match at the previous position, output a
             * single literal. If there was a match but the current match
             * is longer, truncate the previous match to a single literal.
             */
            Tracevv((stderr,"%c",window[strstart-1]));
            if (ct_tally (0, window[strstart-1])) {
                FLUSH_BLOCK(0), block_start = strstart;
            }
            strstart++;
            lookahead--;
         } else {
            /* There is no previous match to compare with,
               wait for the next step to decide. */
            match_available = 1;
            strstart++;
            lookahead--;
         }
         Assert(strstart <= isize && lookahead <= isize, "a bit too far");
      }
   } while (accepted < length);
   if (!minlookahead) {/* eof achieved */
      if (match_available) ct_tally (0, window[strstart-1]);
      if (FLUSH_BLOCK(1) == -1L) goto error;
   }
   match_length = ml;
   return accepted;
error:
   return -1;
}

//begin trees
/*
 The following sorce code is derived from Info-Zip 'zip' 2.01
 distribution copyrighted by Mark Adler, Richard B. Wales,
 Jean-loup Gailly, Kai Uwe Rommel, Igor Mandrichenko and John Bush.
*/

/*
 *  trees.c by Jean-loup Gailly
 *
 *  This is a new version of im_ctree.c originally written by Richard B. Wales
 *  for the defunct implosion method.
 *
 *  PURPOSE
 *
 *      Encode various sets of source values using variable-length
 *      binary code trees.
 *
 *  DISCUSSION
 *
 *      The PKZIP "deflation" process uses several Huffman trees. The more
 *      common source values are represented by shorter bit sequences.
 *
 *      Each code tree is stored in the ZIP file in a compressed form
 *      which is itself a Huffman encoding of the lengths of
 *      all the code strings (in ascending order by source values).
 *      The actual code strings are reconstructed from the lengths in
 *      the UNZIP process, as described in the "application note"
 *      (APPNOTE.TXT) distributed as part of PKWARE's PKZIP program.
 *
 *  REFERENCES
 *
 *      Lynch, Thomas J.
 *          Data Compression:  Techniques and Applications, pp. 53-55.
 *          Lifetime Learning Publications, 1985.  ISBN 0-534-03418-7.
 *
 *      Storer, James A.
 *          Data Compression:  Methods and Theory, pp. 49-50.
 *          Computer Science Press, 1988.  ISBN 0-7167-8156-5.
 *
 *      Sedgewick, R.
 *          Algorithms, p290.
 *          Addison-Wesley, 1983. ISBN 0-201-06672-6.
 *
 *  INTERFACE
 *
 *      int ct_init (void)
 *          Allocate the match buffer and initialize the various tables.
 *
 *      int ct_tally(int dist, int lc);
 *          Save the match info and tally the frequency counts.
 *          Return true if the current block must be flushed.
 *
 *      long flush_block (char *buf, ulg stored_len, int eof)
 *          Determine the best encoding for the current block: dynamic trees,
 *          static trees or store, and output the encoded block to the zip
 *          file. Returns the total compressed length for the file so far.
 */


#define MAX_BITS 15
/* All codes must not exceed MAX_BITS bits */

#define MAX_BL_BITS 7
/* Bit length codes must not exceed MAX_BL_BITS bits */

#define LENGTH_CODES 29
/* number of length codes, not counting the special END_BLOCK code */

#define LITERALS  256
/* number of literal bytes 0..255 */

#define END_BLOCK 256
/* end of block literal code */

#define L_CODES (LITERALS+1+LENGTH_CODES)
/* number of Literal or Length codes, including the END_BLOCK code */

#define D_CODES   30
/* number of distance codes */

#define BL_CODES  19
/* number of codes used to transfer the bit lengths */

static int extra_lbits[LENGTH_CODES] /* extra bits for each length code */
   = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};

static int extra_dbits[D_CODES] /* extra bits for each distance code */
   = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

static int extra_blbits[BL_CODES]/* extra bits for each bit length code */
   = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,3,7};

#define STORED_BLOCK 0
#define STATIC_TREES 1
#define DYN_TREES    2
/* The three kinds of block type */

#ifndef LIT_BUFSIZE
#  ifdef SMALL_MEM
#    define LIT_BUFSIZE  0x2000
#  else
#  ifdef MEDIUM_MEM
#    define LIT_BUFSIZE  0x4000
#  else
#    define LIT_BUFSIZE  0x8000
#  endif
#  endif
#endif
#define DIST_BUFSIZE  LIT_BUFSIZE
/* Sizes of match buffers for literals/lengths and distances.  There are
 * 4 reasons for limiting LIT_BUFSIZE to 64K:
 *   - frequencies can be kept in 16 bit counters
 *   - if compression is not successful for the first block, all input data is
 *     still in the window so we can still emit a stored block even when input
 *     comes from standard input.  (This can also be done for all blocks if
 *     LIT_BUFSIZE is not greater than 32K.)
 *   - if compression is not successful for a file smaller than 64K, we can
 *     even emit a stored file instead of a stored block (saving 5 bytes).
 *   - creating new Huffman trees less frequently may not provide fast
 *     adaptation to changes in the input data statistics. (Take for
 *     example a binary file with poorly compressible code followed by
 *     a highly compressible string table.) Smaller buffer sizes give
 *     fast adaptation but have of course the overhead of transmitting trees
 *     more frequently.
 *   - I can't count above 4
 * The current code is general and allows DIST_BUFSIZE < LIT_BUFSIZE (to save
 * memory at the expense of compression). Some optimizations would be possible
 * if we rely on DIST_BUFSIZE == LIT_BUFSIZE.
 */

#define REP_3_6      16
/* repeat previous bit length 3-6 times (2 bits of repeat count) */

#define REPZ_3_10    17
/* repeat a zero length 3-10 times  (3 bits of repeat count) */

#define REPZ_11_138  18
/* repeat a zero length 11-138 times  (7 bits of repeat count) */

/* Data structure describing a single value and its code string. */
typedef struct ct_data {
    union {
        ush  freq;       /* frequency count */
        ush  code;       /* bit string */
    } fc;
    union {
        ush  dad;        /* father node in Huffman tree */
        ush  len;        /* length of bit string */
    } dl;
} ct_data;

#define Freq fc.freq
#define Code fc.code
#define Dad  dl.dad
#define Len  dl.len

#define HEAP_SIZE (2*L_CODES+1)
/* maximum heap size */

static ct_data dyn_ltree[HEAP_SIZE];   /* literal and length tree */
static ct_data dyn_dtree[2*D_CODES+1]; /* distance tree */

static ct_data static_ltree[L_CODES+2];
/* The static literal tree. Since the bit lengths are imposed, there is no
 * need for the L_CODES extra codes used during heap construction. However
 * The codes 286 and 287 are needed to build a canonical tree (see ct_init
 * below).
 */

static ct_data static_dtree[D_CODES];
/* The static distance tree. (Actually a trivial tree since all codes use
 * 5 bits.)
 */

static ct_data bl_tree[2*BL_CODES+1];
/* Huffman tree for the bit lengths */

typedef struct tree_desc {
    ct_data *dyn_tree;      /* the dynamic tree */
    ct_data *static_tree;   /* corresponding static tree or NULL */
    int     *extra_bits;    /* extra bits for each code or NULL */
    int     extra_base;          /* base index for extra_bits */
    int     elems;               /* max number of elements in the tree */
    int     max_length;          /* max bit length for the codes */
    int     max_code;            /* largest code with non zero frequency */
} tree_desc;

static tree_desc l_desc =
{dyn_ltree, static_ltree, extra_lbits, LITERALS+1, L_CODES, MAX_BITS, 0};

static tree_desc d_desc =
{dyn_dtree, static_dtree, extra_dbits, 0,          D_CODES, MAX_BITS, 0};

static tree_desc bl_desc =
{bl_tree, (ct_data *)0, extra_blbits, 0,     BL_CODES, MAX_BL_BITS, 0};

static ush bl_count[MAX_BITS+1];
/* number of codes at each bit length for an optimal tree */

static uch bl_order[BL_CODES]
   = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
/* The lengths of the bit length codes are sent in order of decreasing
 * probability, to avoid transmitting the lengths for unused bit length codes.
 */

static int heap[2*L_CODES+1]; /* heap used to build the Huffman trees */
static int heap_len;               /* number of elements in the heap */
static int heap_max;               /* element of largest frequency */
/* The sons of heap[n] are heap[2*n] and heap[2*n+1]. heap[0] is not used.
 * The same heap array is used to build all trees.
 */

static uch depth[2*L_CODES+1];
/* Depth of each subtree used as tie breaker for trees of equal frequency */

static uch length_code[MAX_MATCH-MIN_MATCH+1];
/* length code for each normalized match length (0 == MIN_MATCH) */

static uch dist_code[512];
/* distance codes. The first 256 values correspond to the distances
 * 3 .. 258, the last 256 values correspond to the top 8 bits of
 * the 15 bit distances.
 */

static int base_length[LENGTH_CODES];
/* First normalized length for each code (0 = MIN_MATCH) */

static int base_dist[D_CODES];
/* First normalized distance for each code (0 = distance of 1) */

  static uch l_buf[LIT_BUFSIZE];  /* buffer for literals/lengths */
  static ush d_buf[DIST_BUFSIZE]; /* buffer for distances */
#  define p_l_buf l_buf
#  define p_d_buf d_buf

static uch flag_buf[(LIT_BUFSIZE/8)];
/* flag_buf is a bit array distinguishing literals from lengths in
 * l_buf, and thus indicating the presence or absence of a distance.
 */

static unsigned last_lit;    /* running index in l_buf */
static unsigned last_dist;   /* running index in d_buf */
static unsigned last_flags;  /* running index in flag_buf */
static uch flags;            /* current flags not yet saved in flag_buf */
static uch flag_bit;         /* current bit used in flags */
/* bits are filled in flags starting at bit 0 (least significant).
 * Note: these flags are overkill in the current code since we don't
 * take advantage of DIST_BUFSIZE == LIT_BUFSIZE.
 */

static ulg opt_len;        /* bit length of current block with optimal trees */
static ulg static_len;     /* bit length of current block with static trees */

       ulg compressed_len; /* total bit length of compressed file */

static ulg input_len;      /* total byte length of input file */
/* input_len is for debugging only since we can get it by other means. */

#ifdef DEBUG
extern ulg bits_sent;  /* bit length of the compressed data */
extern ulg isize;      /* byte length of input file */
#endif

extern long block_start;       /* window offset of current block */
extern unsigned strstart; /* window offset of current string */

static unsigned reverse    OF((unsigned, int));
static void init_block     OF((void));
static void pqdownheap     OF((ct_data *tree, int k));
static void gen_bitlen     OF((tree_desc *desc));
static void gen_codes      OF((ct_data *tree, int max_code));
static void build_tree     OF((tree_desc *desc));
static void scan_tree      OF((ct_data *tree, int max_code));
static int  send_tree      OF((ct_data *tree, int max_code));
static int  build_bl_tree  OF((void));
static int  send_all_trees OF((int lcodes, int dcodes, int blcodes));
static int  compress_block OF((ct_data *ltree, ct_data *dtree));

#define send_code(c, tree) send_bits(tree[c].Code, tree[c].Len)
/* Send a code of the given tree. c and tree must not have side effects */

#define d_code(dist) \
   ((dist) < 256 ? dist_code[dist] : dist_code[256+((dist)>>7)])
/* Mapping from a distance to a distance code. dist is the distance - 1 and
 * must not have side effects. dist_code[256] and dist_code[257] are never
 * used.
 */

#undef MAX
#define MAX(a,b) (a >= b ? a : b)
/* the arguments must not have side effects */


static unsigned reverse(code, len)
/* Reverse the first len bits of a code. */
unsigned code; /* the value to invert */
int len;       /* its bit length: 1 =< len =< 15 */
{
   register unsigned res = 0;
   do res = (res << 1) | (code & 1), code>>=1; while (--len);
   return res;
}

/* Allocate the match buffer and initialize the various tables. */
int ct_init()
{
   register int n;    /* iterates over tree elements */
   int bits;      /* bit counter */
   int length;    /* length value */
   register int code; /* code value */
   int dist;      /* distance index */

    compressed_len = input_len = 0L;

   if (static_dtree[0].Len != 0) return 0; /* ct_init already called */

   /* Initialize the mapping length (0..255) -> length code (0..28) */
   length = 0;
   for (code=0; code < LENGTH_CODES-1; code++) {
      base_length[code] = length;
      for (n=0; n < (1<<extra_lbits[code]); n++) {
         length_code[length++] = (uch)code;
      }
   }
   Assert (length == 256, "ct_init: length != 256");
    /* Note that the length 255 (match length 258) can be represented
       in two different ways: code 284 + 5 bits or code 285, so we
       overwrite length_code[255] to use the best encoding:     */
   length_code[length-1] = (uch)code;

   /* Initialize the mapping dist (0..32K) -> dist code (0..29) */
   dist = 0;
   for (code=0 ; code < 16; code++) {
      base_dist[code] = dist;
      for (n=0; n < (1<<extra_dbits[code]); n++) {
         dist_code[dist++] = (uch)code;
      }
   }
   Assert (dist == 256, "ct_init: dist != 256");
   dist >>= 7; /* from now on, all distances are divided by 128 */
   for (; code < D_CODES; code++) {
      base_dist[code] = dist << 7;
      for (n=0; n < (1<<(extra_dbits[code]-7)); n++) {
         dist_code[256 + dist++] = (uch)code;
      }
   }
   Assert (dist == 256, "ct_init: 256+dist != 512");

   /* Construct the codes of the static literal tree */
   for (bits=0; bits <= MAX_BITS; bits++) bl_count[bits] = 0;
   n = 0;
   while (n <= 143) static_ltree[n++].Len = 8, bl_count[8]++;
   while (n <= 255) static_ltree[n++].Len = 9, bl_count[9]++;
   while (n <= 279) static_ltree[n++].Len = 7, bl_count[7]++;
   while (n <= 287) static_ltree[n++].Len = 8, bl_count[8]++;
   /* Codes 286 and 287 do not exist, but we must include them in the tree
      construction to get a canonical Huffman tree (longest code all ones) */
   gen_codes((ct_data *)static_ltree, L_CODES+1);

   /* The static distance tree is trivial: */
   for (n=0; n < D_CODES; n++) {
      static_dtree[n].Len = 5;
      static_dtree[n].Code = reverse(n, 5);
   }

   /* Initialize the first block of the first file: */
   init_block();
   return 0;
}

/* Initialize a new block. */
static void init_block()
{
   register int n; /* iterates over tree elements */

   /* Initialize the trees. */
   for (n=0; n < L_CODES;  n++) dyn_ltree[n].Freq = 0;
   for (n=0; n < D_CODES;  n++) dyn_dtree[n].Freq = 0;
   for (n=0; n < BL_CODES; n++) bl_tree[n].Freq = 0;

   dyn_ltree[END_BLOCK].Freq = 1;
   opt_len = static_len = 0L;
   last_lit = last_dist = last_flags = 0;
   flags = 0; flag_bit = 1;
}

#define SMALLEST 1
/* Index within the heap array of least frequent node in the Huffman tree */

/*
 * Remove the smallest element from the heap and recreate the heap with
 * one less element. Updates heap and heap_len.
 */
#define pqremove(tree, top) \
{\
    top = heap[SMALLEST]; \
    heap[SMALLEST] = heap[heap_len--]; \
    pqdownheap(tree, SMALLEST); \
}

/*
 * Compares to subtrees, using the tree depth as tie breaker when
 * the subtrees have equal frequency. This minimizes the worst case length.
 */
#define smaller(tree, n, m) \
   (tree[n].Freq < tree[m].Freq || \
   (tree[n].Freq == tree[m].Freq && depth[n] <= depth[m]))

/*
 * Restore the heap property by moving down the tree starting at node k,
 * exchanging a node with the smallest of its two sons if necessary, stopping
 * when the heap property is re-established (each father smaller than its
 * two sons).
 */
static void pqdownheap(tree, k)
    ct_data *tree;  /* the tree to restore */
    int k;               /* node to move down */
{
    int v = heap[k];
    int j = k << 1;  /* left son of k */
    int htemp;       /* required because of bug in SASC compiler */

    while (j <= heap_len) {
        /* Set j to the smallest of the two sons: */
        if (j < heap_len && smaller(tree, heap[j+1], heap[j])) j++;

        /* Exit if v is smaller than both sons */
        htemp = heap[j];
        if (smaller(tree, v, htemp)) break;

        /* Exchange v with the smallest son */
        heap[k] = htemp;
        k = j;

        /* And continue down the tree, setting j to the left son of k */
        j <<= 1;
    }
    heap[k] = v;
}

/*
 * Compute the optimal bit lengths for a tree and update the total bit length
 * for the current block.
 * IN assertion: the fields freq and dad are set, heap[heap_max] and
 *    above are the tree nodes sorted by increasing frequency.
 * OUT assertions: the field len is set to the optimal bit length, the
 *     array bl_count contains the frequencies for each bit length.
 *     The length opt_len is updated; static_len is also updated if stree is
 *     not null.
 */
static void gen_bitlen(desc)
    tree_desc *desc; /* the tree descriptor */
{
    ct_data *tree  = desc->dyn_tree;
    int *extra     = desc->extra_bits;
    int base            = desc->extra_base;
    int max_code        = desc->max_code;
    int max_length      = desc->max_length;
    ct_data *stree = desc->static_tree;
    int h;              /* heap index */
    int n, m;           /* iterate over the tree elements */
    int bits;           /* bit length */
    int xbits;          /* extra bits */
    ush f;              /* frequency */
    int overflow = 0;   /* number of elements with bit length too large */

    for (bits = 0; bits <= MAX_BITS; bits++) bl_count[bits] = 0;

    /* In a first pass, compute the optimal bit lengths (which may
     * overflow in the case of the bit length tree).
     */
    tree[heap[heap_max]].Len = 0; /* root of the heap */

    for (h = heap_max+1; h < HEAP_SIZE; h++) {
        n = heap[h];
        bits = tree[tree[n].Dad].Len + 1;
        if (bits > max_length) bits = max_length, overflow++;
        tree[n].Len = bits;
        /* We overwrite tree[n].Dad which is no longer needed */

        if (n > max_code) continue; /* not a leaf node */

        bl_count[bits]++;
        xbits = 0;
        if (n >= base) xbits = extra[n-base];
        f = tree[n].Freq;
        opt_len += (ulg)f * (bits + xbits);
        if (stree) static_len += (ulg)f * (stree[n].Len + xbits);
    }
    if (overflow == 0) return;

    Trace((stderr,"\nbit length overflow\n"));
    /* This happens for example on obj2 and pic of the Calgary corpus */

    /* Find the first bit length which could increase: */
    do {
        bits = max_length-1;
        while (bl_count[bits] == 0) bits--;
        bl_count[bits]--;      /* move one leaf down the tree */
        bl_count[bits+1] += 2; /* move one overflow item as its brother */
        bl_count[max_length]--;
        /* The brother of the overflow item also moves one step up,
         * but this does not affect bl_count[max_length]
         */
        overflow -= 2;
    } while (overflow > 0);

    /* Now recompute all bit lengths, scanning in increasing frequency.
     * h is still equal to HEAP_SIZE. (It is simpler to reconstruct all
     * lengths instead of fixing only the wrong ones. This idea is taken
     * from 'ar' written by Haruhiko Okumura.)
     */
    for (bits = max_length; bits != 0; bits--) {
        n = bl_count[bits];
        while (n != 0) {
            m = heap[--h];
            if (m > max_code) continue;
            if (tree[m].Len != (unsigned) bits) {
                Trace((stderr,"code %d bits %d->%d\n", m, tree[m].Len, bits));
                opt_len += ((long)bits-(long)tree[m].Len)*(long)tree[m].Freq;
                tree[m].Len = bits;
            }
            n--;
        }
    }
}

/*
 * Generate the codes for a given tree and bit counts (which need not be
 * optimal).
 * IN assertion: the array bl_count contains the bit length statistics for
 * the given tree and the field len is set for all tree elements.
 * OUT assertion: the field code is set for all tree elements of non
 *     zero code length.
 */
static void gen_codes (tree, max_code)
    ct_data *tree;        /* the tree to decorate */
    int max_code;              /* largest code with non zero frequency */
{
    ush next_code[MAX_BITS+1]; /* next code value for each bit length */
    ush code = 0;              /* running code value */
    int bits;                  /* bit index */
    int n;                     /* code index */

    /* The distribution counts are first used to generate the code values
     * without bit reversal.
     */
    for (bits = 1; bits <= MAX_BITS; bits++) {
        next_code[bits] = code = (code + bl_count[bits-1]) << 1;
    }
    /* Check that the bit counts in bl_count are consistent. The last code
     * must be all ones.
     */
    Assert (code + bl_count[MAX_BITS]-1 == (1<<MAX_BITS)-1,
            "inconsistent bit counts");
    Tracev((stderr,"\ngen_codes: max_code %d ", max_code));

    for (n = 0;  n <= max_code; n++) {
        int len = tree[n].Len;
        if (len == 0) continue;
        /* Now reverse the bits */
        tree[n].Code = reverse(next_code[len]++, len);

        Tracec(tree != static_ltree, (stderr,"\nn %3d %c l %2d c %4x (%x) ",
             n, (isgraph(n) ? n : ' '), len, tree[n].Code, next_code[len]-1));
    }
}

/*
 * Construct one Huffman tree and assigns the code bit strings and lengths.
 * Update the total bit length for the current block.
 * IN assertion: the field freq is set for all tree elements.
 * OUT assertions: the fields len and code are set to the optimal bit length
 *     and corresponding code. The length opt_len is updated; static_len is
 *     also updated if stree is not null. The field max_code is set.
 */
static void build_tree(desc)
    tree_desc *desc; /* the tree descriptor */
{
    ct_data *tree   = desc->dyn_tree;
    ct_data *stree  = desc->static_tree;
    int elems            = desc->elems;
    int n, m;          /* iterate over heap elements */
    int max_code = -1; /* largest code with non zero frequency */
    int node = elems;  /* next internal node of the tree */

    /* Construct the initial heap, with least frequent element in
     * heap[SMALLEST]. The sons of heap[n] are heap[2*n] and heap[2*n+1].
     * heap[0] is not used.
     */
    heap_len = 0, heap_max = HEAP_SIZE;

    for (n = 0; n < elems; n++) {
        if (tree[n].Freq != 0) {
            heap[++heap_len] = max_code = n;
            depth[n] = 0;
        } else {
            tree[n].Len = 0;
        }
    }

    /* The pkzip format requires that at least one distance code exists,
     * and that at least one bit should be sent even if there is only one
     * possible code. So to avoid special checks later on we force at least
     * two codes of non zero frequency.
     */
    while (heap_len < 2) {
        int new = heap[++heap_len] = (max_code < 2 ? ++max_code : 0);
        tree[new].Freq = 1;
        depth[new] = 0;
        opt_len--; if (stree) static_len -= stree[new].Len;
        /* new is 0 or 1 so it does not have extra bits */
    }
    desc->max_code = max_code;

    /* The elements heap[heap_len/2+1 .. heap_len] are leaves of the tree,
     * establish sub-heaps of increasing lengths:
     */
    for (n = heap_len/2; n >= 1; n--) pqdownheap(tree, n);

    /* Construct the Huffman tree by repeatedly combining the least two
     * frequent nodes.
     */
    do {
        pqremove(tree, n);   /* n = node of least frequency */
        m = heap[SMALLEST];  /* m = node of next least frequency */

        heap[--heap_max] = n; /* keep the nodes sorted by frequency */
        heap[--heap_max] = m;

        /* Create a new node father of n and m */
        tree[node].Freq = tree[n].Freq + tree[m].Freq;
        depth[node] = (uch) (MAX(depth[n], depth[m]) + 1);
        tree[n].Dad = tree[m].Dad = node;
#ifdef DUMP_BL_TREE
        if (tree == bl_tree) {
            fprintf(stderr,"\nnode %d(%d), sons %d(%d) %d(%d)",
                    node, tree[node].Freq, n, tree[n].Freq, m, tree[m].Freq);
        }
#endif
        /* and insert the new node in the heap */
        heap[SMALLEST] = node++;
        pqdownheap(tree, SMALLEST);

    } while (heap_len >= 2);

    heap[--heap_max] = heap[SMALLEST];

    /* At this point, the fields freq and dad are set. We can now
     * generate the bit lengths.
     */
    gen_bitlen((tree_desc *)desc);

    /* The field len is now set, we can generate the bit codes */
    gen_codes ((ct_data *)tree, max_code);
}

/* ===========================================================================
 * Scan a literal or distance tree to determine the frequencies of the codes
 * in the bit length tree. Updates opt_len to take into account the repeat
 * counts. (The contribution of the bit length codes will be added later
 * during the construction of bl_tree.)
 */
static void scan_tree (tree, max_code)
    ct_data *tree; /* the tree to be scanned */
    int max_code;       /* and its largest code of non zero frequency */
{
    int n;                     /* iterates over all tree elements */
    int prevlen = -1;          /* last emitted length */
    int curlen;                /* length of current code */
    int nextlen = tree[0].Len; /* length of next code */
    int count = 0;             /* repeat count of the current code */
    int max_count = 7;         /* max repeat count */
    int min_count = 4;         /* min repeat count */

    if (nextlen == 0) max_count = 138, min_count = 3;
    tree[max_code+1].Len = (ush)-1; /* guard */

    for (n = 0; n <= max_code; n++) {
        curlen = nextlen; nextlen = tree[n+1].Len;
        if (++count < max_count && curlen == nextlen) {
            continue;
        } else if (count < min_count) {
            bl_tree[curlen].Freq += count;
        } else if (curlen != 0) {
            if (curlen != prevlen) bl_tree[curlen].Freq++;
            bl_tree[REP_3_6].Freq++;
        } else if (count <= 10) {
            bl_tree[REPZ_3_10].Freq++;
        } else {
            bl_tree[REPZ_11_138].Freq++;
        }
        count = 0; prevlen = curlen;
        if (nextlen == 0) {
            max_count = 138, min_count = 3;
        } else if (curlen == nextlen) {
            max_count = 6, min_count = 3;
        } else {
            max_count = 7, min_count = 4;
        }
    }
}

/* Send a literal or distance tree in compressed form,
   using the codes in bl_tree. */
static int send_tree (tree, max_code)
ct_data *tree; /* the tree to be scanned */
int max_code;       /* and its largest code of non zero frequency */
{
   int n;                     /* iterates over all tree elements */
   int prevlen = -1;          /* last emitted length */
   int curlen;                /* length of current code */
   int nextlen = tree[0].Len; /* length of next code */
   int count = 0;             /* repeat count of the current code */
   int max_count = 7;         /* max repeat count */
   int min_count = 4;         /* min repeat count */

   /* tree[max_code+1].Len = -1; */  /* guard already set */
   if (nextlen == 0) max_count = 138, min_count = 3;

   for (n = 0; n <= max_code; n++) {
      curlen = nextlen; nextlen = tree[n+1].Len;
      if (++count < max_count && curlen == nextlen) {
         continue;
      } else if (count < min_count) {
         do {
            if (send_code(curlen, bl_tree) != 0) goto error;
         } while (--count != 0);
      } else if (curlen != 0) {
         if (curlen != prevlen) {
            if (send_code(curlen, bl_tree) != 0) goto error;
            count--;
         }
         Assert(count >= 3 && count <= 6, " 3_6?");
         if (send_code(REP_3_6, bl_tree) != 0 ||
             send_bits(count-3, 2) != 0) goto error;
      } else if (count <= 10) {
         if (send_code(REPZ_3_10, bl_tree) != 0 ||
             send_bits(count-3, 3) != 0) goto error;
      } else {
         if (send_code(REPZ_11_138, bl_tree) != 0 ||
             send_bits(count-11, 7) != 0) goto error;
      }
      count = 0; prevlen = curlen;
      if (nextlen == 0) {
         max_count = 138, min_count = 3;
      } else if (curlen == nextlen) {
         max_count = 6, min_count = 3;
      } else {
         max_count = 7, min_count = 4;
      }
   }
   return 0;
error:
   return ZWRITE;
}

/* Construct the Huffman tree for the bit lengths and return the index in
   bl_order of the last bit length code to send. */
static int build_bl_tree()
{
    int max_blindex;  /* index of last bit length code of non zero freq */

    /* Determine the bit length frequencies for literal and distance trees */
    scan_tree((ct_data *)dyn_ltree, l_desc.max_code);
    scan_tree((ct_data *)dyn_dtree, d_desc.max_code);

    /* Build the bit length tree: */
    build_tree((tree_desc *)(&bl_desc));
    /* opt_len now includes the length of the tree representations, except
     * the lengths of the bit lengths codes and the 5+5+4 bits for the counts.
     */

    /* Determine the number of bit length codes to send. The pkzip format
     * requires that at least 4 bit length codes be sent. (appnote.txt says
     * 3 but the actual value used is 4.)
     */
    for (max_blindex = BL_CODES-1; max_blindex >= 3; max_blindex--) {
        if (bl_tree[bl_order[max_blindex]].Len != 0) break;
    }
    /* Update opt_len to include the bit length tree and counts */
    opt_len += 3*(max_blindex+1) + 5+5+4;
    Tracev((stderr, "\ndyn trees: dyn %ld, stat %ld", opt_len, static_len));

    return max_blindex;
}

/* Send the header for a block using dynamic Huffman trees: the counts, the
 * lengths of the bit length codes, the literal tree and the distance tree.
 * IN assertion: lcodes >= 257, dcodes >= 1, blcodes >= 4. */
static int send_all_trees(lcodes, dcodes, blcodes)
int lcodes, dcodes, blcodes; /* number of codes for each tree */
{
   int rank;                    /* index in bl_order */

   Assert (lcodes >= 257 && dcodes >= 1 && blcodes >= 4, "not enough codes");
   Assert (lcodes <= L_CODES && dcodes <= D_CODES && blcodes <= BL_CODES,
           "too many codes");
   Tracev((stderr, "\nbl counts: "));
   if (send_bits(lcodes-257, 5) != 0) goto error;
   /* not +255 as stated in appnote.txt 1.93a or -256 in 2.04c */
   if (send_bits(dcodes-1,   5) != 0) goto error;
   /* not -3 as stated in appnote.txt */
   if (send_bits(blcodes-4,  4) != 0) goto error;
   for (rank = 0; rank < blcodes; rank++) {
      Tracev((stderr, "\nbl code %2d ", bl_order[rank]));
      if (send_bits(bl_tree[bl_order[rank]].Len, 3) != 0) goto error;
   }
   Tracev((stderr, "\nbl tree: sent %ld", bits_sent));

   /* send the literal tree */
   if (send_tree((ct_data *)dyn_ltree, lcodes-1) != 0) goto error;
   Tracev((stderr, "\nlit tree: sent %ld", bits_sent));

   /* send the distance tree */
   if (send_tree((ct_data *)dyn_dtree, dcodes-1) != 0) goto error;
   Tracev((stderr, "\ndist tree: sent %ld", bits_sent));
   return 0;
error:
   return ZWRITE;
}

/* ===========================================================================
 * Determine the best encoding for the current block: dynamic trees, static
 * trees or store, and output the encoded block to the zip file. This function
 * returns the total compressed length for the file so far.
 */
ulg flush_block(buf, stored_len, eof)
char *buf;    /* input block, or NULL if too old */
ulg stored_len;   /* length of input block */
int eof;          /* true if this is the last block for a file */
{
   ulg opt_lenb, static_lenb; /* opt_len and static_len in bytes */
   int max_blindex;  /* index of last bit length code of non zero freq */

   flag_buf[last_flags] = flags; /* Save the flags for the last 8 items */

   /* Construct the literal and distance trees */
   build_tree((tree_desc *)(&l_desc));
   Tracev((stderr, "\nlit data: dyn %ld, stat %ld", opt_len, static_len));

   build_tree((tree_desc *)(&d_desc));
   Tracev((stderr, "\ndist data: dyn %ld, stat %ld", opt_len, static_len));
   /* At this point, opt_len and static_len are the total bit lengths of
    * the compressed block data, excluding the tree representations.
    */

   /* Build the bit length tree for the above two trees, and get the index
    * in bl_order of the last bit length code to send.
    */
   max_blindex = build_bl_tree();

   /* Determine the best encoding. Compute first the block length in bytes */
   opt_lenb = (opt_len+3+7)>>3;
   static_lenb = (static_len+3+7)>>3;
   input_len += stored_len; /* for debugging only */

   Trace((stderr, "\nopt %lu(%lu) stat %lu(%lu) stored %lu lit %u dist %u ",
           opt_lenb, opt_len, static_lenb, static_len, stored_len,
           last_lit, last_dist));

   if (static_lenb <= opt_lenb) opt_lenb = static_lenb;

#ifdef FORCE_METHOD
   if (level == 2 && buf) /* force stored block */
#else
   if (stored_len+4 <= opt_lenb && buf) /* 4: two words for the lengths */
#endif
   {
       /* The test buf != NULL is only necessary if LIT_BUFSIZE > WSIZE.
        * Otherwise we can't have processed more than WSIZE input bytes since
        * the last block flush, because compression would have been
        * successful. If LIT_BUFSIZE <= WSIZE, it is never too late to
        * transform a block into a stored block.
        */
       /* send block type */
       if (send_bits((STORED_BLOCK<<1)+eof, 3) != 0) goto error;
       compressed_len = (compressed_len + 3 + 7) & ~7L;
       compressed_len += (stored_len + 4) << 3;
       /* with header */
       if (copy_block(buf, (unsigned)stored_len, 1) != 0) goto error;
   }
#ifdef FORCE_METHOD
   else if (level == 3) /* force static trees */
#else
   else if (static_lenb == opt_lenb)
#endif
   {
       if (send_bits((STATIC_TREES<<1)+eof, 3) != 0 ||
           compress_block((ct_data *)static_ltree,
                          (ct_data *)static_dtree) != 0) goto error;
       compressed_len += 3 + static_len;
   } else {
       if (send_bits((DYN_TREES<<1)+eof, 3) != 0 ||
           send_all_trees(l_desc.max_code+1,
                          d_desc.max_code+1,
                          max_blindex+1) != 0 ||
           compress_block((ct_data *)dyn_ltree,
                          (ct_data *)dyn_dtree) != 0) goto error;
       compressed_len += 3 + opt_len;
   }
   Assert (compressed_len == bits_sent, "bad compressed size");
   init_block();

   if (eof) {
      Assert (input_len == isize, "bad input size");
      if (bi_windup() != 0) goto error;
      compressed_len += 7;  /* align on byte boundary */
   }
   Tracev((stderr,"\ncomprlen %lu(%lu) ", compressed_len>>3,
          compressed_len-7*eof));

   return compressed_len >> 3;
error:
   return -1L;
}

/* Save the match info and tally the frequency counts.
   Return true if the current block must be flushed. */
int ct_tally (dist, lc)
int dist; /* distance of matched string */
int lc;   /* match length-MIN_MATCH or unmatched char (if dist==0) */
{
   l_buf[last_lit++] = (uch)lc;
   if (dist == 0) {
      /* lc is the unmatched char */
      dyn_ltree[lc].Freq++;
   } else {
      /* Here, lc is the match length - MIN_MATCH */
      dist--;             /* dist = match distance - 1 */
      Assert((ush)dist < (ush)MAX_DIST &&
             (ush)lc <= (ush)(MAX_MATCH-MIN_MATCH) &&
             (ush)d_code(dist) < (ush)D_CODES,  "ct_tally: bad match");

      dyn_ltree[length_code[lc]+LITERALS+1].Freq++;
      dyn_dtree[d_code(dist)].Freq++;

      d_buf[last_dist++] = dist;
      flags |= flag_bit;
   }
   flag_bit <<= 1;

   /* Output the flags if they fill a byte: */
   if ((last_lit & 7) == 0) {
      flag_buf[last_flags++] = flags;
      flags = 0, flag_bit = 1;
   }
   /* Try to guess if it is profitable to stop the current block here */
   if (deflate_level > 2 && (last_lit & 0xfff) == 0) {
      /* Compute an upper bound for the compressed length */
      ulg out_length = (ulg)last_lit*8L;
      ulg in_length = (ulg)strstart-block_start;
      int dcode;
      for (dcode = 0; dcode < D_CODES; dcode++) {
         out_length += (ulg)dyn_dtree[dcode].Freq*(5L+extra_dbits[dcode]);
      }
      out_length >>= 3;
      Trace((stderr,"\nlast_lit %u, last_dist %u, in %ld, out ~%ld(%ld%%) ",
            last_lit, last_dist, in_length, out_length,
            100L - out_length*100L/in_length));
       if (last_dist < last_lit/2 && out_length < in_length/2) return 1;
   }
   return (last_lit == LIT_BUFSIZE-1 || last_dist == DIST_BUFSIZE);
   /* We avoid equality with LIT_BUFSIZE because of wraparound at 64K
    * on 16 bit machines and because stored blocks are restricted to
    * 64K-1 bytes. */
}

/* Send the block data compressed using the given Huffman trees */
static int compress_block(ltree, dtree)
ct_data *ltree; /* literal tree */
ct_data *dtree; /* distance tree */
{
   unsigned dist;      /* distance of matched string */
   int lc;             /* match length or unmatched char (if dist == 0) */
   unsigned lx = 0;    /* running index in l_buf */
   unsigned dx = 0;    /* running index in d_buf */
   unsigned fx = 0;    /* running index in flag_buf */
   uch flag = 0;       /* current flags */
   unsigned code;      /* the code to send */
   int extra;          /* number of extra bits to send */

   if (last_lit != 0)
      do {
         if ((lx & 7) == 0) flag = flag_buf[fx++];
         lc = l_buf[lx++];
         if ((flag & 1) == 0) {
            /* send a literal byte */
            if (send_code(lc, ltree) != 0) goto error;
            Tracecv(isgraph(lc), (stderr," '%c' ", lc));
         } else {
            /* Here, lc is the match length - MIN_MATCH */
            code = length_code[lc];
            /* send the length code */
            if (send_code(code+LITERALS+1, ltree) != 0) goto error;
            if ((extra = extra_lbits[code]) != 0) {
               lc -= base_length[code];
               /* send the extra length bits */
               if (send_bits(lc, extra) != 0) goto error;
            }
            dist = d_buf[dx++];
            /* Here, dist is the match distance - 1 */
            code = d_code(dist);
            Assert(code < D_CODES, "bad d_code");

            /* send the distance code */
            if (send_code(code, dtree) != 0) goto error;
            if ((extra = extra_dbits[code]) != 0) {
               dist -= base_dist[code];
               /* send the extra distance bits */
               if (send_bits(dist, extra) != 0) goto error;
             }
         } /* literal or match pair ? */
         flag >>= 1;
     } while (lx < last_lit);

   return send_code(END_BLOCK, ltree);
error:
  return ZWRITE;
}


//begin zippipe

#ifdef DEBUG
       ulg isize;
#endif

LOGICAL zipwrite(buffer, length)
char *buffer; unsigned length;
{ INDEX k ;

   k = (deflate_level > 3 ? lazy_deflate(buffer, length) : fast_deflate(buffer, length)) ;
   if (k == -1) return(FALSE) ;
   return(TRUE) ;
}

LENMAX v_zipCompress(srcBuf,srcLen,dstBuf,dstLenMax,zcLevel,errBuf)
  char *srcBuf ; LENMAX srcLen ;
  char *dstBuf ; LENMAX dstLenMax ;
  INDEX zcLevel ;				/* Compression level */
  UCCHAR *errBuf ;
{
  LENMAX cmpLen ;
/*	Initialize everything */
	if (zcLevel < 1 || zcLevel > 9)
	 { v_Msg(NULL,errBuf,"") ; return(0) ; } ;
	deflate_level = zcLevel ;
	bi_init() ;
	if (ct_init() != 0)
	 { v_Msg(NULL,errBuf,"") ; return(0) ; } ;
	if (lm_init() != 0)
	 { v_Msg(NULL,errBuf,"") ; return(0) ; } ;
	cbX = 0 ; cbB = dstBuf ; cbMax = dstLenMax ;
/*	Compress the sucker */
	if (!zipwrite(srcBuf,srcLen))
	 { v_Msg(NULL,errBuf,"") ; return(0) ; } ;
/*	Close up shop */
	minlookahead = 0;		/* indicate end of input */
	/* Flush out any remaining bytes */
	if (!zipwrite(NULL,0))
	 { v_Msg(NULL,errBuf,"") ; return(0) ; } ;
	cmpLen = (compressed_len >> 3);
	return(cmpLen) ;
}

#ifdef COMPILECOMMENTS

Contributor: PHIL KATZ                


System of Origin : IBM

Original author : Phil Katz

FILE FORMAT
-----------

Files stored in arbitrary order.  Large zipfiles can span multiple
diskette media. 
 
          Local File Header 1 
                    file 1 extra field 
                    file 1 comment 
               file data 1 
          Local File Header 2 
                    file 2 extra field 
                    file 2 comment
               file data 2
          . 
          . 
          . 
          Local File Header n 
                    file n extra field 
                    file n comment 
               file data n 
     Central Directory 
               central extra field
               central comment
          End of Central Directory
                    end comment
EOF


LOCAL FILE HEADER
-----------------

OFFSET LABEL       TYP  VALUE        DESCRIPTION
------ ----------- ---- ----------- ---------------------------------- 
00     ZIPLOCSIG   HEX  04034B50    ;Local File Header Signature 
04     ZIPVER      DW   0000        ;Version needed to extract 
06     ZIPGENFLG   DW   0000        ;General purpose bit flag 
08     ZIPMTHD     DW   0000        ;Compression method 
0A     ZIPTIME     DW   0000        ;Last mod file time (MS-DOS) 
0C     ZIPDATE     DW   0000        ;Last mod file date (MS-DOS) 
0E     ZIPCRC      HEX  00000000    ;CRC-32
12     ZIPSIZE     HEX  00000000    ;Compressed size 
16     ZIPUNCMP    HEX  00000000    ;Uncompressed size
1A     ZIPFNLN     DW   0000        ;Filename length
1C     ZIPXTRALN   DW   0000        ;Extra field length 
1E     ZIPNAME     DS   ZIPFNLN     ;filename 
--     ZIPXTRA     DS   ZIPXTRALN   ;extra field 
 
CENTRAL DIRECTORY STRUCTURE
--------------------------- 
 
OFFSET LABEL       TYP  VALUE        DESCRIPTION
------ ----------- ---- ----------- ----------------------------------
00     ZIPCENSIG   HEX  02014B50    ;Central file header signature 
04     ZIPCVER     DB   00          ;Version made by 
05     ZIPCOS      DB   00          ;Host operating system 
06     ZIPCVXT     DB   00          ;Version needed to extract 
07     ZIPCEXOS    DB   00          ;O/S of version needed for extraction 
08     ZIPCFLG     DW   0000        ;General purpose bit flag 
0A     ZIPCMTHD    DW   0000        ;Compression method 
0C     ZIPCTIM     DW   0000        ;Last mod file time (MS-DOS)
0E     ZIPCDAT     DW   0000        ;Last mod file date (MS-DOS) 
10     ZIPCCRC     HEX  00000000    ;CRC-32
14     ZIPCSIZ     HEX  00000000    ;Compressed size
18     ZIPCUNC     HEX  00000000    ;Uncompressed size 
1C     ZIPCFNL     DW   0000        ;Filename length 
1E     ZIPCXTL     DW   0000        ;Extra field length 
20     ZIPCCML     DW   0000        ;File comment length 
22     ZIPDSK      DW   0000        ;Disk number start
24     ZIPINT      DW   0000        ;Internal file attributes 
 
       LABEL       BIT        DESCRIPTION
       ----------- --------- -----------------------------------------
       ZIPINT         0       if = 1, file is apparently an ASCII or 
                              text file 
                      0       if = 0, file apparently contains binary 
                              data 

                     1-7      unused in version 1.0.
 
26     ZIPEXT      HEX  00000000    ;External file attributes, host 
                                    ;system dependent
2A     ZIPOFST     HEX  00000000    ;Relative offset of local header 
                                    ;from the start of the first disk 
                                    ;on which this file appears
2E     ZIPCFN      DS   ZIPCFNL     ;Filename or path - should not 
                                    ;contain a drive or device letter, 
                                    ;or a leading slash. All slashes 
                                    ;should be forward slashes '/' 
--     ZIPCXTR     DS   ZIPCXTL     ;extra field
--     ZIPCOM      DS   ZIPCCML     ;file comment


END OF CENTRAL DIR STRUCTURE
---------------------------- 
 
OFFSET LABEL       TYP  VALUE        DESCRIPTION 
------ ----------- ---- ----------- ---------------------------------- 
00     ZIPESIG     HEX  06064B50    ;End of central dir signature
04     ZIPEDSK     DW   0000        ;Number of this disk 
06     ZIPECEN     DW   0000        ;Number of disk with start central dir 
08     ZIPENUM     DW   0000        ;Total number of entries in central dir 
                                    ;on this disk 
0A     ZIPECENN    DW   0000        ;total number entries in central dir 
0C     ZIPECSZ     HEX  00000000    ;Size of the central directory
10     ZIPEOFST    HEX  00000000    ;Offset of start of central directory 
                                    ;with respect to the starting disk
                                    ;number 
14     ZIPECOML    DW   0000        ;zipfile comment length 
16     ZIPECOM     DS   ZIPECOML    ;zipfile comment
 
 
ZIP VALUES LEGEND
-----------------
 
       HOST O/S 
 
       VALUE  DESCRIPTION               VALUE  DESCRIPTION 
       ----- -------------------------- ----- ------------------------
       0      MS-DOS and OS/2 (FAT)     5      Atari ST 
       1      Amiga                     6      OS/2 1.2 extended file sys 
       2      VMS                       7      Macintosh 
       3      *nix                      8 thru 
       4      VM/CMS                    255    unused 

 
       GENERAL PURPOSE BIT FLAG 
 
       LABEL       BIT        DESCRIPTION 
       ----------- --------- -----------------------------------------
       ZIPGENFLG      0       If set, file is encrypted 
          or          1       If file Imploded and this bit is set, 8K 
       ZIPCFLG                sliding dictionary was used. If clear, 4K
                              sliding dictionary was used.
                      2       If file Imploded and this bit is set, 3 
                              Shannon-Fano trees were used. If clear, 2 
                              Shannon-Fano trees were used. 
                     3-4      unused 
                     5-7      used internaly by ZIP
 
       Note:  Bits 1 and 2 are undefined if the compression method is 
              other than type 6 (Imploding). 
 

       COMPRESSION METHOD
 
       NAME        METHOD  DESCRIPTION 
       ----------- ------ -------------------------------------------- 
       Stored         0    No compression used 
       Shrunk         1    LZW, 8K buffer, 9-13 bits with partial clearing 
       Reduced-1      2    Probalistic compression, L(X) = lower 7 bits 
       Reduced-2      3    Probalistic compression, L(X) = lower 6 bits 
       Reduced-3      4    Probalistic compression, L(X) = lower 5 bits 
       Reduced-4      5    Probalistic compression, L(X) = lower 4 bits
       Imploded       6    2 Shanno-Fano trees, 4K sliding dictionary
       Imploded       7    3 Shanno-Fano trees, 4K sliding dictionary 
       Imploded       8    2 Shanno-Fano trees, 8K sliding dictionary
       Imploded       9    3 Shanno-Fano trees, 8K sliding dictionary 

 
       EXTRA FIELD 

       OFFSET LABEL       TYP  VALUE       DESCRIPTION
       ------ ----------- ---- ---------- ----------------------------
       00     EX1ID       DW   0000        ;0-31 reserved by PKWARE
       02     EX1LN       DW   0000
       04     EX1DAT      DS   EX1LN       ;Specific data for individual
       .                                   ;files. Data field should begin
       .                                   ;with a s/w specific unique ID
       EX1LN+4
              EXnID       DW   0000
              EXnLN       DW   0000

              EXnDAT      DS   EXnLN       ;entire header may not exceed 64k
#endif