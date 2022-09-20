# define gettext(msgid) (msgid)
#define gettext_noop(String) String


#define V4REG_NumChars 256
static char re_syntax_table[V4REG_NumChars];
#define ISWORD 1
#define SYNTAX(c) re_syntax_table[c]
static void vregexp_InitSynOnce ()
{ 
  static int doneSetup = FALSE;
  int i;

	if (doneSetup) return;
	memset(re_syntax_table,0,sizeof re_syntax_table);
	for (i='a';i <= 'z';i++) { re_syntax_table[i] = ISWORD ; } ;
	for (i='A';i <= 'Z';i++) { re_syntax_table[i] = ISWORD ; } ;
	for (i='0';i <= '9';i++) { re_syntax_table[i] = ISWORD ; } ;
	re_syntax_table['_'] = ISWORD ; 
	doneSetup = TRUE ;
}


#define ISBLANK(c) (isascii(c) && (c == ' ' || c == '\t'))
#define ISGRAPH(c) (isascii(c) && isgraph (c))
#define ISPRINT(c) (isascii(c) && isprint (c))
#define ISDIGIT(c) (isascii(c) && isdigit (c))
#define ISALNUM(c) (isascii(c) && isalnum (c))
#define ISALPHA(c) (isascii(c) && isalpha (c))
#define ISCNTRL(c) (isascii(c) && iscntrl (c))
#define ISLOWER(c) (isascii(c) && islower (c))
#define ISPUNCT(c) (isascii(c) && ispunct (c))
#define ISSPACE(c) (isascii(c) && isspace (c))
#define ISUPPER(c) (isascii(c) && isupper (c))
#define ISXDIGIT(c) (isascii(c) && isxdigit (c))

/* We remove any previous definition of `SIGN_EXTEND_CHAR',
   since ours (we hope) works properly with all combinations of
   machines, compilers, `char' and `unsigned char' argument types.
   (Per Bothner suggested the basic approach.)	*/
#undef SIGN_EXTEND_CHAR
#if __STDC__
#define SIGN_EXTEND_CHAR(c) ((signed char) (c))
#else  /* not __STDC__ */
/* As in Harbison and Steele.  */
#define SIGN_EXTEND_CHAR(c) ((((unsigned char) (c)) ^ 128) - 128)
#endif


#define REGEX_ALLOCATE malloc
#define REGEX_REALLOCATE(source, osize, nsize) realloc (source, nsize)
#define REGEX_FREE(ptr) free((void *)ptr)


#define REGEX_ALLOCATE_STACK malloc
#define REGEX_REALLOCATE_STACK(source, osize, nsize) realloc (source, nsize)
#define REGEX_FREE_STACK free

/* True if `size1' is non-NULL and PTR is pointing anywhere inside
   `string1' or just past its end.  This works if PTR is NULL, which is
   a good thing.  */
#define FIRST_STRING_P(ptr)  (size1 && string1 <= (ptr) && (ptr) <= string1 + size1)

/* (Re)Allocate N items of type T using malloc, or fail.  */
#define TALLOC(n, t) ((t *) malloc ((n) * sizeof (t)))
#define RETALLOC(addr, n, t) ((addr) = (t *) realloc (addr, (n) * sizeof (t)))
#define RETALLOC_IF(addr, n, t) \
  if (addr) RETALLOC((addr), (n), t); else (addr) = TALLOC ((n), t)
#define REGEX_TALLOC(n, t) ((t *) REGEX_ALLOCATE ((n) * sizeof (t)))

#define BYTEWIDTH 8 /* In bits.  */

#define STREQ(s1, s2) ((strcmp (s1, s2) == 0))

#undef MAX
#undef MIN
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

//typedef char boolean;
#undef boolean
#define boolean char
static int vregexp_Match2Int ();

/* These are the command codes that appear in compiled regular
   expressions.  Some opcodes are followed by argument bytes.  A
   command code can specify any interpretation whatsoever for its
   arguments.  Zero bytes may appear in the compiled regular expression.  */

typedef enum
{
  no_op = 0,

  VRE_Succeed,		  /* Succeed right away--no more backtracking.	*/


	/* Followed by one byte giving n, then by n literal bytes.  */
  VRE_ExactN,

	/* Matches any (more or less) character.  */
  VRE_MatchAny,

	/* Matches any one char belonging to specified set.  First
	   following byte is number of bitmap bytes.  Then come bytes
	   for a bitmap saying which chars are in.  Bits in each byte
	   are ordered low-bit-first.  A character is in the set if its
	   bit is 1.  A character too large to have a bit in the map is
	   automatically not in the set.  */
  VRE_MatchCharSet,

	/* Same parameters as VRE_MatchCharSet, but match any character that is
	   not one of those specified.	*/
  VRE_MatchNotCharSet,

	/* Start remembering the text that is matched, for storing in a
	   register.  Followed by one byte with the register number, in
	   the range 0 to one less than the pattern buffer's re_nsub
	   field.  Then followed by one byte with the number of groups
	   inner to this one.  (This last has to be part of the
	   VRE_StartMatchMem only because we need it in the VRE_JumpOnFail
	   of vregexp_Match2.)	*/
  VRE_StartMatchMem,

	/* Stop remembering the text that is matched and store it in a
	   memory register.  Followed by one byte with the register
	   number, in the range 0 to one less than `re_nsub' in the
	   pattern buffer, and one byte with the number of inner groups,
	   just like `VRE_StartMatchMem'.  (We need the number of inner
	   groups here because we don't have any easy way of finding the
	   corresponding VRE_StartMatchMem when we're at a VRE_StopMatchMem.)  */
  VRE_StopMatchMem,

	/* Match a VRE_MatchDupInReg of something remembered. Followed by one
	   byte containing the register number.  */
  VRE_MatchDupInReg,

	/* Fail unless at beginning of line.  */
  VRE_MatchBOL,

	/* Fail unless at end of line.	*/
  VRE_MatchEOL,

	/* Succeeds if at beginning of buffer (if emacs) or at beginning
	   of string to be matched (if not).  */
  VRE_MatchBOS,

	/* Analogously, for end of buffer/string.  */
  VRE_MatchEOS,

	/* Followed by two byte relative address to which to VRE_JumpTo.  */
  VRE_JumpTo,

	/* Same as VRE_JumpTo, but marks the end of an alternative.  */
  VRE_JumpToAlt,

	/* Followed by two-byte relative address of place to resume at
	   in case of failure.	*/
  VRE_JumpOnFail,

	/* Like VRE_JumpOnFail, but pushes a placeholder instead of the
	   current string position when executed.  */
  VRE_PushPlaceHolder,

	/* Throw away latest failure point and then VRE_JumpTo to following
	   two-byte relative address.  */
  VRE_PopFailure,

	/* Change to VRE_PopFailure if know won't have to backtrack to
	   match; otherwise change to VRE_JumpTo.  This is used to VRE_JumpTo
	   back to the beginning of a repeat.  If what follows this VRE_JumpTo
	   clearly won't match what the repeat does, such that we can be
	   sure that there is no use backtracking out of repetitions
	   already matched, then we change it to a VRE_PopFailure.
	   Followed by two-byte address.  */
  VRE_MaybePopFailure,

	/* Jump to following two-byte address, and push a dummy failure
	   point. This failure point will be thrown away if an attempt
	   is made to use it for a failure.  A `+' construct makes this
	   before the first repeat.  Also used as an intermediary kind
	   of VRE_JumpTo when compiling an alternative.  */
  VRE_DmyFailJump,

	/* Push a dummy failure point and continue.  Used at the end of
	   alternatives.  */
  VRE_PushDmyFail,

	/* Followed by two-byte relative address and two-byte number n.
	   After matching N times, VRE_JumpTo to the address upon failure.  */
  VRE_JumpAfterNSuccess,

	/* Followed by two-byte relative address, and two-byte number n.
	   Jump to the address N times, then fail.  */
  VRE_JumpNTimes,

	/* Set the following two-byte relative address to the
	   subsequent two-byte number.	The address *includes* the two
	   bytes of number.  */
  VRE_SetAddress,

  VRE_MatchWordChar,	/* Matches any word-constituent character.  */
  VRE_MatchNotWordChar,	/* Matches any char that is not a word-constituent.  */

  VRE_MatchWordBgn,	/* Succeeds if at word beginning.  */
  VRE_MatchWordEnd,	/* Succeeds if at word end.  */

  VRE_MatchWordBnd,	/* Succeeds if at a word boundary.  */
  VRE_MatchNotWordBnd	/* Succeeds if not at a word boundary.	*/

} re_opcode_t;

/* Common operations on the compiled pattern.  */

/* Store NUMBER in two contiguous bytes starting at DESTINATION.  */

#define STORE_NUMBER(destination, number)				\
  do {									\
    (destination)[0] = (number) & 0377; 				\
    (destination)[1] = (number) >> 8;					\
  } while (0)

/* Same as STORE_NUMBER, except increment DESTINATION to
   the byte after where the number is stored.  Therefore, DESTINATION
   must be an lvalue.  */

#define STORE_NUMBER_AND_INCR(destination, number)			\
  do {									\
    STORE_NUMBER (destination, number); 				\
    (destination) += 2; 						\
  } while (0)

/* Put into DESTINATION a number stored in two contiguous bytes starting
   at SOURCE.  */

#define EXTRACT_NUMBER(destination, source)				\
  do {									\
    (destination) = *(source) & 0377;					\
    (destination) += SIGN_EXTEND_CHAR (*((source) + 1)) << 8;		\
  } while (0)

/* Same as EXTRACT_NUMBER, except increment SOURCE to after the number.
   SOURCE must be an lvalue.  */

#define EXTRACT_NUMBER_AND_INCR(destination, source)			\
  do {									\
    EXTRACT_NUMBER (destination, source);				\
    (source) += 2;							\
  } while (0)



/* Set by `vregexp_SetSyntax' to the current regexp syntax to recognize.  Can
   also be assigned to arbitrarily: each pattern buffer stores its own
   syntax, so it can be changed between regex compilations.  */
/* This has no initializer because initialized variables in Emacs
   become read-only after dumping.  */
reg_syntax_t re_syntax_options;


/* Specify the precise syntax of regexps for compilation.  This provides
   for compatibility for various utilities which historically have
   different, incompatible syntaxes.

   The argument SYNTAX is a bit mask comprised of the various bits
   defined in regex.h.	We return the old syntax.  */

reg_syntax_t
vregexp_SetSyntax (syntax)
    reg_syntax_t syntax;
{
  reg_syntax_t ret = re_syntax_options;

  re_syntax_options = syntax;
  return ret;
}

/* This table gives an error message for each of the error codes listed
   in regex.h.	Obviously the order here has to be same as there.
   POSIX doesn't require that we do anything for REG_NOERROR,
   but why not be nice?  */

static char *re_error_msgid[] =
  {
    gettext_noop ("RegExp Matched"),	/* REG_NOERROR */
    gettext_noop ("RegExp Failed - no match"),	/* REG_NOMATCH */
    gettext_noop ("RegExp Failed - malformed regular expression"), /* REG_BADPAT */
    gettext_noop ("RegExp Failed - not a valid collation character"), /* REG_ECOLLATE */
    gettext_noop ("RegExp Failed - not a valid character class name"), /* REG_ECTYPE */
    gettext_noop ("RegExp Failed - trailing backslash"), /* REG_EESCAPE */
    gettext_noop ("RegExp Failed - invalid back reference"), /* REG_ESUBREG */
    gettext_noop ("RegExp Failed - mismatched [ or [^"),	/* REG_EBRACK */
    gettext_noop ("RegExp Failed - mismatched ( or \\("), /* REG_EPAREN */
    gettext_noop ("RegExp Failed - mismatched \\{"), /* REG_EBRACE */
    gettext_noop ("RegExp Failed - \\{contents invalid\\}"), /* REG_BADBR */
    gettext_noop ("RegExp Failed - range end"), /* REG_ERANGE */
    gettext_noop ("RegExp Failed - no more memory"), /* REG_ESPACE */
    gettext_noop ("RegExp Failed - preceding regular expression malformed"), /* REG_BADRPT */
    gettext_noop ("RegExp Failed - hit end of regular expression?"), /* REG_EEND */
    gettext_noop ("RegExp Failed - regular expression too big"), /* REG_ESIZE */
    gettext_noop ("RegExp Failed - mismatched ) or \\)"), /* REG_ERPAREN */
  };

/* Avoiding alloca during matching, to placate r_alloc.  */

/* When using GNU C, we are not REALLY using the C alloca, no matter
   what config.h may say.  So don't take precautions for it.  */
#ifdef __GNUC__
#undef C_ALLOCA
#endif


/* Failure stack declarations and macros; both vregexp_CompFastMap and
   vregexp_Match2 use a failure stack.	These have to be macros because of
   REGEX_ALLOCATE_STACK.  */


/* Number of failure points for which to initially allocate space
   when matching.  If this number is exceeded, we allocate more
   space, so it is not a hard limit.  */
#ifndef INIT_FAILURE_ALLOC
#define INIT_FAILURE_ALLOC 5
#endif

/* Roughly the maximum number of failure points on the stack.  Would be
   exactly that if always used MAX_FAILURE_ITEMS items each time we failed.
   This is a variable only so users of regex can assign to it; we never
   change it ourselves.  */


int re_max_failures = 20000;

union fail_stack_elt
{
  unsigned char *pointer;
  int integer;
};

typedef union fail_stack_elt fail_stack_elt_t;

typedef struct
{
  fail_stack_elt_t *stack;
  unsigned size;
  unsigned avail;			/* Offset of next open position.  */
} fail_stack_type;


#define FAIL_STACK_EMPTY()     (fail_stack.avail == 0)
#define FAIL_STACK_PTR_EMPTY() (fail_stack_ptr->avail == 0)
#define FAIL_STACK_FULL()      (fail_stack.avail == fail_stack.size)


/* Define macros to initialize and free the failure stack.
   Do `return -2' if the alloc fails.  */

#define INIT_FAIL_STACK()						\
  do {									\
    fail_stack.stack = (fail_stack_elt_t *)				\
      REGEX_ALLOCATE_STACK (INIT_FAILURE_ALLOC * sizeof (fail_stack_elt_t));	\
									\
    if (fail_stack.stack == NULL)					\
      return -2;							\
									\
    fail_stack.size = INIT_FAILURE_ALLOC;				\
    fail_stack.avail = 0;						\
  } while (0)

#define RESET_FAIL_STACK()  REGEX_FREE_STACK (fail_stack.stack)


/* Double the size of FAIL_STACK, up to approximately `re_max_failures' items.

   Return 1 if succeeds, and 0 if either ran out of memory
   allocating space for it or it was already too large.

   REGEX_REALLOCATE_STACK requires `destination' be declared.	*/

#define DOUBLE_FAIL_STACK(fail_stack)					\
  ((fail_stack).size > (unsigned) (re_max_failures * MAX_FAILURE_ITEMS) \
   ? 0									\
   : ((fail_stack).stack = (fail_stack_elt_t *) 			\
	REGEX_REALLOCATE_STACK ((fail_stack).stack,			\
	  (fail_stack).size * sizeof (fail_stack_elt_t),		\
	  ((fail_stack).size << 1) * sizeof (fail_stack_elt_t)),	\
									\
      (fail_stack).stack == NULL					\
      ? 0								\
      : ((fail_stack).size <<= 1,					\
	 1)))


/* Push pointer POINTER on FAIL_STACK.
   Return 1 if was able to do so and 0 if ran out of memory allocating
   space to do so.  */
#define PUSH_PATTERN_OP(POINTER, FAIL_STACK)				\
  ((FAIL_STACK_FULL ()							\
    && !DOUBLE_FAIL_STACK (FAIL_STACK)) 				\
   ? 0									\
   : ((FAIL_STACK).stack[(FAIL_STACK).avail++].pointer = POINTER,	\
      1))

/* Push a pointer value onto the failure stack.
   Assumes the variable `fail_stack'.  Probably should only
   be called from within `PUSH_FAILURE_POINT'.	*/
#define PUSH_FAILURE_POINTER(item)					\
  fail_stack.stack[fail_stack.avail++].pointer = (unsigned char *) (item)

/* This pushes an integer-valued item onto the failure stack.
   Assumes the variable `fail_stack'.  Probably should only
   be called from within `PUSH_FAILURE_POINT'.	*/
#define PUSH_FAILURE_INT(item)					\
  fail_stack.stack[fail_stack.avail++].integer = (item)

/* Push a fail_stack_elt_t value onto the failure stack.
   Assumes the variable `fail_stack'.  Probably should only
   be called from within `PUSH_FAILURE_POINT'.	*/
#define PUSH_FAILURE_ELT(item)					\
  fail_stack.stack[fail_stack.avail++] =  (item)

/* These three POP... operations complement the three PUSH... operations.
   All assume that `fail_stack' is nonempty.  */
#define POP_FAILURE_POINTER() fail_stack.stack[--fail_stack.avail].pointer
#define POP_FAILURE_INT() fail_stack.stack[--fail_stack.avail].integer
#define POP_FAILURE_ELT() fail_stack.stack[--fail_stack.avail]


/* Push the information about the state we will need
   if we ever fail back to it.

   Requires variables fail_stack, regstart, regend, reg_info, and
   num_regs be declared.  DOUBLE_FAIL_STACK requires `destination' be
   declared.

   Does `return FAILURE_CODE' if runs out of memory.  */

#define PUSH_FAILURE_POINT(pattern_place, string_place, failure_code)	\
  do {									\
    /* char *destination; */							\
    /* Must be int, so when we don't save any registers, the arithmetic \
       of 0 + -1 isn't done as unsigned.  */				\
    /* Can't be int, since there is not a shred of a guarantee that int \
       is wide enough to hold a value of something to which pointer can \
       be assigned */							\
    s_reg_t this_reg;							\
									\
									\
    /* Ensure we have enough space allocated for what we will push.  */ \
    while (REMAINING_AVAIL_SLOTS < NUM_FAILURE_ITEMS)			\
      { 								\
	if (!DOUBLE_FAIL_STACK (fail_stack))				\
	  return failure_code;						\
									\
      } 								\
									\
    /* Push the info, starting with the registers.  */			\
									\
    if (1)								\
      for (this_reg = lowest_active_reg; this_reg <= highest_active_reg; \
	   this_reg++)							\
	{								\
	  PUSH_FAILURE_POINTER (regstart[this_reg]);			\
	  PUSH_FAILURE_POINTER (regend[this_reg]);			\
	  PUSH_FAILURE_ELT (reg_info[this_reg].word);			\
	}								\
    PUSH_FAILURE_INT (lowest_active_reg);				\
    PUSH_FAILURE_INT (highest_active_reg);				\
    PUSH_FAILURE_POINTER (pattern_place);				\
									\
    PUSH_FAILURE_POINTER (string_place);				\
  } while (0)

/* This is the number of items that are pushed and popped on the stack
   for each register.  */
#define NUM_REG_ITEMS  3

/* Individual items aside from the registers.  */
#define NUM_NONREG_ITEMS 4

/* We push at most this many items on the stack.  */
/* We used to use (num_regs - 1), which is the number of registers
   this regexp will save; but that was changed to 5
   to avoid stack overflow for a regexp with lots of parens.  */
#define MAX_FAILURE_ITEMS (5 * NUM_REG_ITEMS + NUM_NONREG_ITEMS)

/* We actually push this many items.  */
#define NUM_FAILURE_ITEMS				\
  (((0							\
     ? 0 : highest_active_reg - lowest_active_reg + 1)	\
    * NUM_REG_ITEMS)					\
   + NUM_NONREG_ITEMS)

/* How many items can still be added to the stack without overflowing it.  */
#define REMAINING_AVAIL_SLOTS ((fail_stack).size - (fail_stack).avail)


/* Pops what PUSH_FAIL_STACK pushes.

   We restore into the parameters, all of which should be lvalues:
     STR -- the saved data position.
     PAT -- the saved pattern position.
     LOW_REG, HIGH_REG -- the highest and lowest active registers.
     REGSTART, REGEND -- arrays of string positions.
     REG_INFO -- array of information about each subexpression.

   Also assumes the variables `fail_stack' and (if debugging), `bufp',
   `pend', `string1', `size1', `string2', and `size2'.	*/

#define POP_FAILURE_POINT(str, pat, low_reg, high_reg, regstart, regend, reg_info)\
{									\
  s_reg_t this_reg;							\
  unsigned char *string_temp;					\
									\
									\
  /* Remove failure points and point to how many regs pushed.  */	\
									\
									\
  /* If the saved string location is NULL, it came from an		\
     VRE_PushPlaceHolder opcode, and we want to throw away the	\
     saved NULL, thus retaining our current position in the string.  */ \
  string_temp = POP_FAILURE_POINTER (); 				\
  if (string_temp != NULL)						\
    str = (char *) string_temp;					\
									\
  pat = (unsigned char *) POP_FAILURE_POINTER ();			\
									\
  /* Restore register info.  */ 					\
  high_reg = (active_reg_t) POP_FAILURE_INT (); 			\
									\
  low_reg = (active_reg_t) POP_FAILURE_INT ();				\
									\
  if (1)								\
    for (this_reg = high_reg; this_reg >= low_reg; this_reg--)		\
      { 								\
									\
	reg_info[this_reg].word = POP_FAILURE_ELT ();			\
									\
	regend[this_reg] = (char *) POP_FAILURE_POINTER ();	\
									\
	regstart[this_reg] = (char *) POP_FAILURE_POINTER ();	\
      } 								\
  else									\
    {									\
      for (this_reg = highest_active_reg; this_reg > high_reg; this_reg--) \
	{								\
	  reg_info[this_reg].word.integer = 0;				\
	  regend[this_reg] = 0; 					\
	  regstart[this_reg] = 0;					\
	}								\
      highest_active_reg = high_reg;					\
    }									\
									\
  set_regs_matched_done = 0;						\
} /* POP_FAILURE_POINT */



/* Structure for per-register (a.k.a. per-group) information.
   Other register information, such as the
   starting and ending positions (which are addresses), and the list of
   inner groups (which is a bits list) are maintained in separate
   variables.

   We are making a (strictly speaking) nonportable assumption here: that
   the compiler will pack our bit fields into something that fits into
   the type of `word', i.e., is something that fits into one item on the
   failure stack.  */


/* Declarations and macros for vregexp_Match2.	*/

typedef union
{
  fail_stack_elt_t word;
  struct
  {
      /* This field is one if this group can match the empty string,
	 zero if not.  If not yet determined,  `MATCH_NULL_UNSET_VALUE'.  */
#define MATCH_NULL_UNSET_VALUE 3
    unsigned match_null_string_p : 2;
    unsigned is_active : 1;
    unsigned matched_something : 1;
    unsigned ever_matched_something : 1;
  } bits;
} register_info_type;

#define REG_MATCH_NULL_STRING_P(R)  ((R).bits.match_null_string_p)
#define IS_ACTIVE(R)  ((R).bits.is_active)
#define MATCHED_SOMETHING(R)  ((R).bits.matched_something)
#define EVER_MATCHED_SOMETHING(R)  ((R).bits.ever_matched_something)


/* Call this when have matched a real character; it sets `matched' flags
   for the subexpressions which we are currently inside.  Also records
   that those subexprs have matched.  */
#define SET_REGS_MATCHED()						\
  do									\
    {									\
      if (!set_regs_matched_done)					\
	{								\
	  active_reg_t r;						\
	  set_regs_matched_done = 1;					\
	  for (r = lowest_active_reg; r <= highest_active_reg; r++)	\
	    {								\
	      MATCHED_SOMETHING (reg_info[r])				\
		= EVER_MATCHED_SOMETHING (reg_info[r])			\
		= 1;							\
	    }								\
	}								\
    }									\
  while (0)

/* Registers are set to a sentinel when they haven't yet matched.  */
static char reg_unset_dummy;
#define REG_UNSET_VALUE (&reg_unset_dummy)
#define REG_UNSET(e) ((e) == REG_UNSET_VALUE)

/* Subroutine declarations and macros for vregexp_Compile.  */

static reg_errcode_t vregexp_Compile _RE_ARGS ((char *pattern, size_t size,
					      reg_syntax_t syntax,
					      struct re_pattern_buffer *bufp));
static void vregexp_StoreOp1 _RE_ARGS ((re_opcode_t op, unsigned char *loc, int arg));
static void vregexp_StoreOp2 _RE_ARGS ((re_opcode_t op, unsigned char *loc,
				 int arg1, int arg2));
static void vregexp_InsOp1 _RE_ARGS ((re_opcode_t op, unsigned char *loc,
				  int arg, unsigned char *end));
static void vregexp_InsOp2 _RE_ARGS ((re_opcode_t op, unsigned char *loc,
				  int arg1, int arg2, unsigned char *end));
static boolean vregexp_AtBgnLineLocP _RE_ARGS ((char *pattern, char *p,
					   reg_syntax_t syntax));
static boolean vregexp_AtEndLineLocP _RE_ARGS ((char *p, char *pend,
					   reg_syntax_t syntax));
static reg_errcode_t vregexp_CompRange _RE_ARGS ((char **p_ptr,
					      char *pend,
					      char *translate,
					      reg_syntax_t syntax,
					      unsigned char *b));

/* Fetch the next character in the uncompiled pattern---translating it
   if necessary.  Also cast from a signed character in the constant
   string passed to us by the user to an unsigned char that we can use
   as an array index (in, e.g., `translate').  */
#ifndef PATFETCH
#define PATFETCH(c)							\
  do {if (p == pend) return REG_EEND;					\
    c = (unsigned char) *p++;						\
    if (translate) c = (unsigned char) translate[c];			\
  } while (0)
#endif

/* Fetch the next character in the uncompiled pattern, with no
   translation.  */
#define PATFETCH_RAW(c) 						\
  do {if (p == pend) return REG_EEND;					\
    c = (unsigned char) *p++;						\
  } while (0)

/* Go backwards one character in the pattern.  */
#define PATUNFETCH p--


/* If `translate' is non-null, return translate[D], else just D.  We
   cast the subscript to translate because some data is declared as
   `char *', to avoid warnings when a string constant is passed.  But
   when we use a character as a subscript we must make it unsigned.  */
#ifndef TRANSLATE
#define TRANSLATE(d) \
  (translate ? (char) translate[(unsigned char) (d)] : (d))
#endif


/* Macros for outputting the compiled pattern into `buffer'.  */

/* If the buffer isn't allocated when it comes in, use this.  */
#define INIT_BUF_SIZE  32

/* Make sure we have at least N more bytes of space in buffer.	*/
#define GET_BUFFER_SPACE(n)						\
    while ((unsigned long) (b - bufp->buffer + (n)) > bufp->allocated)	\
      EXTEND_BUFFER ()

/* Make sure we have one more byte of buffer space and then add C to it.  */
#define BUF_PUSH(c)							\
  do {									\
    GET_BUFFER_SPACE (1);						\
    *b++ = (unsigned char) (c); 					\
  } while (0)


/* Ensure we have two more bytes of buffer space and then append C1 and C2.  */
#define BUF_PUSH_2(c1, c2)						\
  do {									\
    GET_BUFFER_SPACE (2);						\
    *b++ = (unsigned char) (c1);					\
    *b++ = (unsigned char) (c2);					\
  } while (0)


/* As with BUF_PUSH_2, except for three bytes.	*/
#define BUF_PUSH_3(c1, c2, c3)						\
  do {									\
    GET_BUFFER_SPACE (3);						\
    *b++ = (unsigned char) (c1);					\
    *b++ = (unsigned char) (c2);					\
    *b++ = (unsigned char) (c3);					\
  } while (0)


/* Store a VRE_JumpTo with opcode OP at LOC to location TO.  We store a
   relative address offset by the three bytes the VRE_JumpTo itself occupies.  */
#define STORE_JUMP(op, loc, to) \
  vregexp_StoreOp1 (op, loc, (int) ((to) - (loc) - 3))

/* Likewise, for a two-argument VRE_JumpTo.  */
#define STORE_JUMP2(op, loc, to, arg) \
  vregexp_StoreOp2 (op, loc, (int) ((to) - (loc) - 3), arg)

/* Like `STORE_JUMP', but for inserting.  Assume `b' is the buffer end.  */
#define INSERT_JUMP(op, loc, to) \
  vregexp_InsOp1 (op, loc, (int) ((to) - (loc) - 3), b)

/* Like `STORE_JUMP2', but for inserting.  Assume `b' is the buffer end.  */
#define INSERT_JUMP2(op, loc, to, arg) \
  vregexp_InsOp2 (op, loc, (int) ((to) - (loc) - 3), arg, b)


/* This is not an arbitrary limit: the arguments which represent offsets
   into the pattern are two bytes long.  So if 2^16 bytes turns out to
   be too small, many things would have to change.  */
/* Any other compiler which, like MSC, has allocation limit below 2^16
   bytes will have to use approach similar to what was done below for
   MSC and drop MAX_BUF_SIZE a bit.  Otherwise you may end up
   reallocating to 0 bytes.  Such thing is not going to work too well.
   You have been warned!!  */
#if defined(_MSC_VER)  && !defined(WIN32)
/* Microsoft C 16-bit versions limit malloc to approx 65512 bytes.
   The REALLOC define eliminates a flurry of conversion warnings,
   but is not required. */
#define MAX_BUF_SIZE  65500L
#define REALLOC(p,s) realloc ((p), (size_t) (s))
#else
#define MAX_BUF_SIZE (1L << 16)
#define REALLOC(p,s) realloc ((p), (s))
#endif

/* Extend the buffer by twice its current size via realloc and
   reset the pointers that pointed into the old block to point to the
   correct places in the new one.  If extending the buffer results in it
   being larger than MAX_BUF_SIZE, then flag memory exhausted.	*/
#define EXTEND_BUFFER() 						\
  do {									\
    unsigned char *old_buffer = bufp->buffer;				\
    if (bufp->allocated == MAX_BUF_SIZE)				\
      return REG_ESIZE; 						\
    bufp->allocated <<= 1;						\
    if (bufp->allocated > MAX_BUF_SIZE) 				\
      bufp->allocated = MAX_BUF_SIZE;					\
    bufp->buffer = (unsigned char *) REALLOC (bufp->buffer, bufp->allocated);\
    if (bufp->buffer == NULL)						\
      return REG_ESPACE;						\
    /* If the buffer moved, move all the pointers into it.  */		\
    if (old_buffer != bufp->buffer)					\
      { 								\
	b = (b - old_buffer) + bufp->buffer;				\
	begalt = (begalt - old_buffer) + bufp->buffer;			\
	if (fixup_alt_jump)						\
	  fixup_alt_jump = (fixup_alt_jump - old_buffer) + bufp->buffer;\
	if (laststart)							\
	  laststart = (laststart - old_buffer) + bufp->buffer;		\
	if (pending_exact)						\
	  pending_exact = (pending_exact - old_buffer) + bufp->buffer;	\
      } 								\
  } while (0)


/* Since we have one byte reserved for the register number argument to
   {start,stop}_memory, the maximum number of groups we can report
   things about is what fits in that byte.  */
#define MAX_REGNUM 255

/* But patterns can have more than `MAX_REGNUM' registers.  We just
   ignore the excess.  */
typedef unsigned regnum_t;


/* Macros for the compile stack.  */

/* Since offsets can go either forwards or backwards, this type needs to
   be able to hold values from -(MAX_BUF_SIZE - 1) to MAX_BUF_SIZE - 1.  */
/* int may be not enough when sizeof(int) == 2.  */
typedef long pattern_offset_t;

typedef struct
{
  pattern_offset_t begalt_offset;
  pattern_offset_t fixup_alt_jump;
  pattern_offset_t inner_group_offset;
  pattern_offset_t laststart_offset;
  regnum_t regnum;
} compile_stack_elt_t;


typedef struct
{
  compile_stack_elt_t *stack;
  unsigned size;
  unsigned avail;			/* Offset of next open position.  */
} compile_stack_type;


#define INIT_COMPILE_STACK_SIZE 32

#define COMPILE_STACK_EMPTY  (compile_stack.avail == 0)
#define COMPILE_STACK_FULL  (compile_stack.avail == compile_stack.size)

/* The next available element.	*/
#define COMPILE_STACK_TOP (compile_stack.stack[compile_stack.avail])


/* Set the bit for character C in a list.  */
#define SET_LIST_BIT(c) 			      \
  (b[((unsigned char) (c)) / BYTEWIDTH] 	      \
   |= 1 << (((unsigned char) c) % BYTEWIDTH))


/* Get the next unsigned number in the uncompiled pattern.  */
#define GET_UNSIGNED_NUMBER(num)					\
  { if (p != pend)							\
     {									\
       PATFETCH (c);							\
       while (ISDIGIT (c))						\
	 {								\
	   if (num < 0) 						\
	      num = 0;							\
	   num = num * 10 + c - '0';					\
	   if (p == pend)						\
	      break;							\
	   PATFETCH (c);						\
	 }								\
       }								\
    }

#if defined _LIBC || (defined HAVE_WCTYPE_H && defined HAVE_WCHAR_H)
/* The GNU C library provides support for user-defined character classes
   and the functions from ISO C amendement 1.  */
# ifdef CHARCLASS_NAME_MAX
#  define CHAR_CLASS_MAX_LENGTH CHARCLASS_NAME_MAX
# else
/* This shouldn't happen but some implementation might still have this
   problem.  Use a reasonable default value.  */
#  define CHAR_CLASS_MAX_LENGTH 256
# endif

# define IS_CHAR_CLASS(string) wctype (string)
#else
# define CHAR_CLASS_MAX_LENGTH	6 /* Namely, `xdigit'.	*/

# define IS_CHAR_CLASS(string)						\
   (STREQ (string, "alpha") || STREQ (string, "upper")			\
    || STREQ (string, "lower") || STREQ (string, "digit")		\
    || STREQ (string, "alnum") || STREQ (string, "xdigit")		\
    || STREQ (string, "space") || STREQ (string, "print")		\
    || STREQ (string, "punct") || STREQ (string, "graph")		\
    || STREQ (string, "cntrl") || STREQ (string, "blank"))
#endif


static boolean vregexp_GrpInCompStack _RE_ARGS ((compile_stack_type
						 compile_stack,
						 regnum_t regnum));

/* `vregexp_Compile' compiles PATTERN (of length SIZE) according to SYNTAX.
   Returns one of error codes defined in `regex.h', or zero for success.

   Assumes the `allocated' (and perhaps `buffer') and `translate'
   fields are set in BUFP on entry.

   If it succeeds, results are put in BUFP (if it returns an error, the
   contents of BUFP are undefined):
     `buffer' is the compiled pattern;
     `syntax' is set to SYNTAX;
     `used' is set to the length of the compiled pattern;
     `fastmap_accurate' is zero;
     `re_nsub' is the number of subexpressions in PATTERN;
     `not_bol' and `not_eol' are zero;

   The `fastmap' and `newline_anchor' fields are neither
   examined nor set.  */

/* Return, freeing storage we allocated.  */
#define FREE_STACK_RETURN(value)		\
  return (free (compile_stack.stack), value)

static reg_errcode_t
vregexp_Compile (pattern, size, syntax, bufp)
     char *pattern;
     size_t size;
     reg_syntax_t syntax;
     struct re_pattern_buffer *bufp;
{
  /* We fetch characters from PATTERN here.  Even though PATTERN is
     `char *' (i.e., signed), we declare these variables as unsigned, so
     they can be reliably used as array indices.  */
  register unsigned char c, c1;

  /* A random temporary spot in PATTERN.  */
  char *p1;

  /* Points to the end of the buffer, where we should append.  */
  register unsigned char *b;

  /* Keeps track of unclosed groups.  */
  compile_stack_type compile_stack;

  /* Points to the current (ending) position in the pattern.  */
  char *p = pattern;
  char *pend = pattern + size;

  /* How to translate the characters in the pattern.  */
  RE_TRANSLATE_TYPE translate = bufp->translate;

  /* Address of the count-byte of the most recently inserted `VRE_ExactN'
     command.  This makes it possible to tell if a new exact-match
     character can be added to that command or if the character requires
     a new `VRE_ExactN' command.  */
  unsigned char *pending_exact = 0;

  /* Address of start of the most recently finished expression.
     This tells, e.g., postfix * where to find the start of its
     operand.  Reset at the beginning of groups and alternatives.  */
  unsigned char *laststart = 0;

  /* Address of beginning of regexp, or inside of last group.  */
  unsigned char *begalt;

  /* Place in the uncompiled pattern (i.e., the {) to
     which to go back if the interval is invalid.  */
  char *beg_interval;

  /* Address of the place where a forward VRE_JumpTo should go to the end of
     the containing expression.  Each alternative of an `or' -- except the
     last -- ends with a forward VRE_JumpTo of this sort.  */
  unsigned char *fixup_alt_jump = 0;

  /* Counts open-groups as they are encountered.  Remembered for the
     matching close-group on the compile stack, so the same register
     number is put in the VRE_StopMatchMem as the VRE_StartMatchMem.  */
  regnum_t regnum = 0;


  /* Initialize the compile stack.  */
  compile_stack.stack = TALLOC (INIT_COMPILE_STACK_SIZE, compile_stack_elt_t);
  if (compile_stack.stack == NULL)
    return REG_ESPACE;

  compile_stack.size = INIT_COMPILE_STACK_SIZE;
  compile_stack.avail = 0;

  /* Initialize the pattern buffer.  */
  bufp->syntax = syntax;
  bufp->fastmap_accurate = 0;
  bufp->not_bol = bufp->not_eol = 0;
  bufp->want_shortest = 0 ;

  /* Set `used' to zero, so that if we return an error, the pattern
     printer (for debugging) will think there's no pattern.  We reset it
     at the end.  */
  bufp->used = 0;

  /* Always count groups, whether or not bufp->no_sub is set.  */
  bufp->re_nsub = 0;

  /* Initialize the syntax table.  */
   vregexp_InitSynOnce ();

  if (bufp->allocated == 0)
    {
      if (bufp->buffer)
	{ /* If zero allocated, but buffer is non-null, try to realloc
	     enough space.  This loses if buffer's address is bogus, but
	     that is the user's responsibility.  */
	  RETALLOC (bufp->buffer, INIT_BUF_SIZE, unsigned char);
	}
      else
	{ /* Caller did not allocate a buffer.	Do it for them.  */
	  bufp->buffer = TALLOC (INIT_BUF_SIZE, unsigned char);
	}
      if (!bufp->buffer) FREE_STACK_RETURN (REG_ESPACE);

      bufp->allocated = INIT_BUF_SIZE;
    }

  begalt = b = bufp->buffer;

  /* Loop through the uncompiled pattern until we're at the end.  */
  while (p != pend)
    {
      PATFETCH (c);

      switch (c)
	{
	case '^':
	  {
	    if (   /* If at start of pattern, it's an operator.  */
		   p == pattern + 1
		   /* If context independent, it's an operator.  */
		|| syntax & RE_CONTEXT_INDEP_ANCHORS
		   /* Otherwise, depends on what's come before.  */
		|| vregexp_AtBgnLineLocP (pattern, p, syntax))
	      BUF_PUSH (VRE_MatchBOL);
	    else
	      goto normal_char;
	  }
	  break;


	case '$':
	  {
	    if (   /* If at end of pattern, it's an operator.  */
		   p == pend
		   /* If context independent, it's an operator.  */
		|| syntax & RE_CONTEXT_INDEP_ANCHORS
		   /* Otherwise, depends on what's next.  */
		|| vregexp_AtEndLineLocP (p, pend, syntax))
	       BUF_PUSH (VRE_MatchEOL);
	     else
	       goto normal_char;
	   }
	   break;


	case '+':
	case '?':
	  if ((syntax & RE_BK_PLUS_QM)
	      || (syntax & RE_LIMITED_OPS))
	    goto normal_char;
	handle_plus:
	case '*':
	  /* If there is no previous pattern... */
	  if (!laststart)
	    {
	      if (syntax & RE_CONTEXT_INVALID_OPS)
		FREE_STACK_RETURN (REG_BADRPT);
	      else if (!(syntax & RE_CONTEXT_INDEP_OPS))
		goto normal_char;
	    }

	  {
	    /* Are we optimizing this VRE_JumpTo?  */
	    boolean keep_string_p = FALSE;

	    /* 1 means zero (many) matches is allowed.	*/
	    char zero_times_ok = 0, many_times_ok = 0;

	    /* If there is a sequence of repetition chars, collapse it
	       down to just one (the right one).  We can't combine
	       interval operators with these because of, e.g., `a{2}*',
	       which should only match an even number of `a's.	*/

	    for (;;)
	      {
		zero_times_ok |= c != '+';
		many_times_ok |= c != '?';

		if (p == pend)
		  break;

		PATFETCH (c);
		
		if (many_times_ok && c == '?') 
		 { bufp->want_shortest = TRUE ; }

		else if (c == '*'
		    || (!(syntax & RE_BK_PLUS_QM) && (c == '+' || c == '?')))
		  ;

		else if (syntax & RE_BK_PLUS_QM  &&  c == '\\')
		  {
		    if (p == pend) FREE_STACK_RETURN (REG_EESCAPE);

		    PATFETCH (c1);
		    if (!(c1 == '+' || c1 == '?'))
		      {
			PATUNFETCH;
			PATUNFETCH;
			break;
		      }

		    c = c1;
		  }
		else
		  {
		    PATUNFETCH;
		    break;
		  }

		/* If we get here, we found another repeat character.  */
	       }

	    /* Star, etc. applied to an empty pattern is equivalent
	       to an empty pattern.  */
	    if (!laststart)
	      break;

	    /* Now we know whether or not zero matches is allowed
	       and also whether or not two or more matches is allowed.	*/
	    if (many_times_ok)
	      { /* More than one repetition is allowed, so put in at the
		   end a backward relative VRE_JumpTo from `b' to before the next
		   VRE_JumpTo we're going to put in below (which jumps from
		   laststart to after this VRE_JumpTo).

		   But if we are at the `*' in the exact sequence `.*\n',
		   insert an unconditional VRE_JumpTo backwards to the .,
		   instead of the beginning of the loop.  This way we only
		   push a failure point once, instead of every time
		   through the loop.  */

		/* Allocate the space for the VRE_JumpTo.  */
		GET_BUFFER_SPACE (3);

		/* We know we are not at the first character of the pattern,
		   because laststart was nonzero.  And we've already
		   incremented `p', by the way, to be the character after
		   the `*'.  Do we have to do something analogous here
		   for null bytes, because of RE_DOT_NOT_NULL?	*/
		if (TRANSLATE (*(p - 2)) == TRANSLATE ('.')
		    && zero_times_ok
		    && p < pend && TRANSLATE (*p) == TRANSLATE ('\n')
		    && !(syntax & RE_DOT_NEWLINE))
		  { /* We have .*\n.  */
		    STORE_JUMP (VRE_JumpTo, b, laststart);
		    keep_string_p = TRUE;
		  }
		else
		  /* Anything else.  */
		  STORE_JUMP (VRE_MaybePopFailure, b, laststart - 3);

		/* We've added more stuff to the buffer.  */
		b += 3;
	      }

	    /* On failure, VRE_JumpTo from laststart to b + 3, which will be the
	       end of the buffer after this VRE_JumpTo is inserted.  */
	    GET_BUFFER_SPACE (3);
	    INSERT_JUMP (keep_string_p ? VRE_PushPlaceHolder
				       : VRE_JumpOnFail,
			 laststart, b + 3);
	    pending_exact = 0;
	    b += 3;

	    if (!zero_times_ok)
	      {
		/* At least one repetition is required, so insert a
		   `VRE_DmyFailJump' before the initial
		   `VRE_JumpOnFail' instruction of the loop. This
		   effects a skip over that instruction the first time
		   we hit that loop.  */
		GET_BUFFER_SPACE (3);
		INSERT_JUMP (VRE_DmyFailJump, laststart, laststart + 6);
		b += 3;
	      }
	    }
	  break;


	case '.':
	  laststart = b;
	  BUF_PUSH (VRE_MatchAny);
	  break;


	case '[':
	  {
	    boolean had_char_class = FALSE;

	    if (p == pend) FREE_STACK_RETURN (REG_EBRACK);

	    /* Ensure that we have enough space to push a VRE_MatchCharSet: the
	       opcode, the length count, and the bitset; 34 bytes in all.  */
	    GET_BUFFER_SPACE (34);

	    laststart = b;

	    /* We test `*p == '^' twice, instead of using an if
	       statement, so we only need one BUF_PUSH.  */
	    BUF_PUSH (*p == '^' ? VRE_MatchNotCharSet : VRE_MatchCharSet);
	    if (*p == '^')
	      p++;

	    /* Remember the first position in the bracket expression.  */
	    p1 = p;

	    /* Push the number of bytes in the bitmap.	*/
	    BUF_PUSH ((1 << BYTEWIDTH) / BYTEWIDTH);

	    /* Clear the whole map.  */
	    memset (b, 0, (1 << BYTEWIDTH) / BYTEWIDTH);

	    /* VRE_MatchNotCharSet matches newline according to a syntax bit.  */
	    if ((re_opcode_t) b[-2] == VRE_MatchNotCharSet
		&& (syntax & RE_HAT_LISTS_NOT_NEWLINE))
	      SET_LIST_BIT ('\n');

	    /* Read in characters and ranges, setting map bits.  */
	    for (;;)
	      {
		if (p == pend) FREE_STACK_RETURN (REG_EBRACK);

		PATFETCH (c);

		/* \ might escape characters inside [...] and [^...].  */
		if ((syntax & RE_BACKSLASH_ESCAPE_IN_LISTS) && c == '\\')
		  {
		    if (p == pend) FREE_STACK_RETURN (REG_EESCAPE);

		    PATFETCH (c1);
		    SET_LIST_BIT (c1);
		    continue;
		  }

		/* Could be the end of the bracket expression.	If it's
		   not (i.e., when the bracket expression is `[]' so
		   far), the ']' character bit gets set way below.  */
		if (c == ']' && p != p1 + 1)
		  break;

		/* Look ahead to see if it's a range when the last thing
		   was a character class.  */
		if (had_char_class && c == '-' && *p != ']')
		  FREE_STACK_RETURN (REG_ERANGE);

		/* Look ahead to see if it's a range when the last thing
		   was a character: if this is a hyphen not at the
		   beginning or the end of a list, then it's the range
		   operator.  */
		if (c == '-'
		    && !(p - 2 >= pattern && p[-2] == '[')
		    && !(p - 3 >= pattern && p[-3] == '[' && p[-2] == '^')
		    && *p != ']')
		  {
		    reg_errcode_t ret
		      = vregexp_CompRange (&p, pend, translate, syntax, b);
		    if (ret != REG_NOERROR) FREE_STACK_RETURN (ret);
		  }

		else if (p[0] == '-' && p[1] != ']')
		  { /* This handles ranges made up of characters only.	*/
		    reg_errcode_t ret;

		    /* Move past the `-'.  */
		    PATFETCH (c1);

		    ret = vregexp_CompRange (&p, pend, translate, syntax, b);
		    if (ret != REG_NOERROR) FREE_STACK_RETURN (ret);
		  }

		/* See if we're at the beginning of a possible character
		   class.  */

		else if (syntax & RE_CHAR_CLASSES && c == '[' && *p == ':')
		  { /* Leave room for the null.  */
		    char str[CHAR_CLASS_MAX_LENGTH + 1];

		    PATFETCH (c);
		    c1 = 0;

		    /* If pattern is `[[:'.  */
		    if (p == pend) FREE_STACK_RETURN (REG_EBRACK);

		    for (;;)
		      {
			PATFETCH (c);
			if (c == ':' || c == ']' || p == pend
			    || c1 == CHAR_CLASS_MAX_LENGTH)
			  break;
			str[c1++] = c;
		      }
		    str[c1] = '\0';

		    /* If isn't a word bracketed by `[:' and:`]':
		       undo the ending character, the letters, and leave
		       the leading `:' and `[' (but set bits for them).  */
		    if (c == ':' && *p == ']')
		      {
#if defined _LIBC || (defined HAVE_WCTYPE_H && defined HAVE_WCHAR_H)
			boolean is_lower = STREQ (str, "lower");
			boolean is_upper = STREQ (str, "upper");
			wctype_t wt;
			int ch;

			wt = wctype (str);
			if (wt == 0)
			  FREE_STACK_RETURN (REG_ECTYPE);

			/* Throw away the ] at the end of the character
			   class.  */
			PATFETCH (c);

			if (p == pend) FREE_STACK_RETURN (REG_EBRACK);

			for (ch = 0; ch < 1 << BYTEWIDTH; ++ch)
			  {
			    if (iswctype (btowc (ch), wt))
			      SET_LIST_BIT (ch);

			    if (translate && (is_upper || is_lower)
				&& (ISUPPER (ch) || ISLOWER (ch)))
			      SET_LIST_BIT (ch);
			  }

			had_char_class = TRUE;
#else
			int ch;
			boolean is_alnum = STREQ (str, "alnum");
			boolean is_alpha = STREQ (str, "alpha");
			boolean is_blank = STREQ (str, "blank");
			boolean is_cntrl = STREQ (str, "cntrl");
			boolean is_digit = STREQ (str, "digit");
			boolean is_graph = STREQ (str, "graph");
			boolean is_lower = STREQ (str, "lower");
			boolean is_print = STREQ (str, "print");
			boolean is_punct = STREQ (str, "punct");
			boolean is_space = STREQ (str, "space");
			boolean is_upper = STREQ (str, "upper");
			boolean is_xdigit = STREQ (str, "xdigit");

			if (!IS_CHAR_CLASS (str))
			  FREE_STACK_RETURN (REG_ECTYPE);

			/* Throw away the ] at the end of the character
			   class.  */
			PATFETCH (c);

			if (p == pend) FREE_STACK_RETURN (REG_EBRACK);

			for (ch = 0; ch < 1 << BYTEWIDTH; ch++)
			  {
			    /* This was split into 3 if's to
			       avoid an arbitrary limit in some compiler.  */
			    if (   (is_alnum  && ISALNUM (ch))
				|| (is_alpha  && ISALPHA (ch))
				|| (is_blank  && ISBLANK (ch))
				|| (is_cntrl  && ISCNTRL (ch)))
			      SET_LIST_BIT (ch);
			    if (   (is_digit  && ISDIGIT (ch))
				|| (is_graph  && ISGRAPH (ch))
				|| (is_lower  && ISLOWER (ch))
				|| (is_print  && ISPRINT (ch)))
			      SET_LIST_BIT (ch);
			    if (   (is_punct  && ISPUNCT (ch))
				|| (is_space  && ISSPACE (ch))
				|| (is_upper  && ISUPPER (ch))
				|| (is_xdigit && ISXDIGIT (ch)))
			      SET_LIST_BIT (ch);
			    if (   translate && (is_upper || is_lower)
				&& (ISUPPER (ch) || ISLOWER (ch)))
			      SET_LIST_BIT (ch);
			  }
			had_char_class = TRUE;
#endif	/* libc || wctype.h */
		      }
		    else
		      {
			c1++;
			while (c1--)
			  PATUNFETCH;
			SET_LIST_BIT ('[');
			SET_LIST_BIT (':');
			had_char_class = FALSE;
		      }
		  }
		else
		  {
		    had_char_class = FALSE;
		    SET_LIST_BIT (c);
		  }
	      }

	    /* Discard any (non)matching list bytes that are all 0 at the
	       end of the map.	Decrease the map-length byte too.  */
	    while ((int) b[-1] > 0 && b[b[-1] - 1] == 0)
	      b[-1]--;
	    b += b[-1];
	  }
	  break;


	case '(':
	  if (syntax & RE_NO_BK_PARENS)
	    goto handle_open;
	  else
	    goto normal_char;


	case ')':
	  if (syntax & RE_NO_BK_PARENS)
	    goto handle_close;
	  else
	    goto normal_char;


	case '\n':
	  if (syntax & RE_NEWLINE_ALT)
	    goto handle_alt;
	  else
	    goto normal_char;


	case '|':
	  if (syntax & RE_NO_BK_VBAR)
	    goto handle_alt;
	  else
	    goto normal_char;


	case '{':
	   if (syntax & RE_INTERVALS && syntax & RE_NO_BK_BRACES)
	     goto handle_interval;
	   else
	     goto normal_char;


	case '\\':
	  if (p == pend) FREE_STACK_RETURN (REG_EESCAPE);

	  /* Do not translate the character after the \, so that we can
	     distinguish, e.g., \B from \b, even if we normally would
	     translate, e.g., B to b.  */
	  PATFETCH_RAW (c);

	  switch (c)
	    {
	    case '(':
	      if (syntax & RE_NO_BK_PARENS)
		goto normal_backslash;

	    handle_open:
	      bufp->re_nsub++;
	      regnum++;

	      if (COMPILE_STACK_FULL)
		{
		  RETALLOC (compile_stack.stack, compile_stack.size << 1,
			    compile_stack_elt_t);
		  if (compile_stack.stack == NULL) return REG_ESPACE;

		  compile_stack.size <<= 1;
		}

	      /* These are the values to restore when we hit end of this
		 group.  They are all relative offsets, so that if the
		 whole pattern moves because of realloc, they will still
		 be valid.  */
	      COMPILE_STACK_TOP.begalt_offset = begalt - bufp->buffer;
	      COMPILE_STACK_TOP.fixup_alt_jump
		= fixup_alt_jump ? fixup_alt_jump - bufp->buffer + 1 : 0;
	      COMPILE_STACK_TOP.laststart_offset = b - bufp->buffer;
	      COMPILE_STACK_TOP.regnum = regnum;

	      /* We will eventually replace the 0 with the number of
		 groups inner to this one.  But do not push a
		 VRE_StartMatchMem for groups beyond the last one we can
		 represent in the compiled pattern.  */
	      if (regnum <= MAX_REGNUM)
		{
		  COMPILE_STACK_TOP.inner_group_offset = b - bufp->buffer + 2;
		  BUF_PUSH_3 (VRE_StartMatchMem, regnum, 0);
		}

	      compile_stack.avail++;

	      fixup_alt_jump = 0;
	      laststart = 0;
	      begalt = b;
	      /* If we've reached MAX_REGNUM groups, then this open
		 won't actually generate any code, so we'll have to
		 clear pending_exact explicitly.  */
	      pending_exact = 0;
	      break;


	    case ')':
	      if (syntax & RE_NO_BK_PARENS) goto normal_backslash;

	      if (COMPILE_STACK_EMPTY)
		if (syntax & RE_UNMATCHED_RIGHT_PAREN_ORD)
		  goto normal_backslash;
		else
		  FREE_STACK_RETURN (REG_ERPAREN);

	    handle_close:
	      if (fixup_alt_jump)
		{ /* Push a dummy failure point at the end of the
		     alternative for a possible future
		     `VRE_PopFailure' to pop.  See comments at
		     `VRE_PushDmyFail' in `vregexp_Match2'.	*/
		  BUF_PUSH (VRE_PushDmyFail);

		  /* We allocated space for this VRE_JumpTo when we assigned
		     to `fixup_alt_jump', in the `handle_alt' case below.  */
		  STORE_JUMP (VRE_JumpToAlt, fixup_alt_jump, b - 1);
		}

	      /* See similar code for backslashed left paren above.  */
	      if (COMPILE_STACK_EMPTY)
		if (syntax & RE_UNMATCHED_RIGHT_PAREN_ORD)
		  goto normal_char;
		else
		  FREE_STACK_RETURN (REG_ERPAREN);

	      /* Since we just checked for an empty stack above, this
		 ``can't happen''.  */
	      {
		/* We don't just want to restore into `regnum', because
		   later groups should continue to be numbered higher,
		   as in `(ab)c(de)' -- the second group is #2.  */
		regnum_t this_group_regnum;

		compile_stack.avail--;
		begalt = bufp->buffer + COMPILE_STACK_TOP.begalt_offset;
		fixup_alt_jump
		  = COMPILE_STACK_TOP.fixup_alt_jump
		    ? bufp->buffer + COMPILE_STACK_TOP.fixup_alt_jump - 1
		    : 0;
		laststart = bufp->buffer + COMPILE_STACK_TOP.laststart_offset;
		this_group_regnum = COMPILE_STACK_TOP.regnum;
		/* If we've reached MAX_REGNUM groups, then this open
		   won't actually generate any code, so we'll have to
		   clear pending_exact explicitly.  */
		pending_exact = 0;

		/* We're at the end of the group, so now we know how many
		   groups were inside this one.  */
		if (this_group_regnum <= MAX_REGNUM)
		  {
		    unsigned char *inner_group_loc
		      = bufp->buffer + COMPILE_STACK_TOP.inner_group_offset;

		    *inner_group_loc = regnum - this_group_regnum;
		    BUF_PUSH_3 (VRE_StopMatchMem, this_group_regnum,
				regnum - this_group_regnum);
		  }
	      }
	      break;


	    case '|':					/* `\|'.  */
	      if (syntax & RE_LIMITED_OPS || syntax & RE_NO_BK_VBAR)
		goto normal_backslash;
	    handle_alt:
	      if (syntax & RE_LIMITED_OPS)
		goto normal_char;

	      /* Insert before the previous alternative a VRE_JumpTo which
		 jumps to this alternative if the former fails.  */
	      GET_BUFFER_SPACE (3);
	      INSERT_JUMP (VRE_JumpOnFail, begalt, b + 6);
	      pending_exact = 0;
	      b += 3;

	      /* The alternative before this one has a VRE_JumpTo after it
		 which gets executed if it gets matched.  Adjust that
		 VRE_JumpTo so it will VRE_JumpTo to this alternative's analogous
		 VRE_JumpTo (put in below, which in turn will VRE_JumpTo to the next
		 (if any) alternative's such VRE_JumpTo, etc.).  The last such
		 VRE_JumpTo jumps to the correct final destination.  A picture:
			  _____ _____
			  |   | |   |
			  |   v |   v
			 a | b	 | c

		 If we are at `b', then fixup_alt_jump right now points to a
		 three-byte space after `a'.  We'll put in the VRE_JumpTo, set
		 fixup_alt_jump to right after `b', and leave behind three
		 bytes which we'll fill in when we get to after `c'.  */

	      if (fixup_alt_jump)
		STORE_JUMP (VRE_JumpToAlt, fixup_alt_jump, b);

	      /* Mark and leave space for a VRE_JumpTo after this alternative,
		 to be filled in later either by next alternative or
		 when know we're at the end of a series of alternatives.  */
	      fixup_alt_jump = b;
	      GET_BUFFER_SPACE (3);
	      b += 3;

	      laststart = 0;
	      begalt = b;
	      break;


	    case '{':
	      /* If \{ is a literal.  */
	      if (!(syntax & RE_INTERVALS)
		     /* If we're at `\{' and it's not the open-interval
			operator.  */
		  || ((syntax & RE_INTERVALS) && (syntax & RE_NO_BK_BRACES))
		  || (p - 2 == pattern	&&  p == pend))
		goto normal_backslash;

	    handle_interval:
	      {
		/* If got here, then the syntax allows intervals.  */

		/* At least (most) this many matches must be made.  */
		int lower_bound = -1, upper_bound = -1;

		beg_interval = p - 1;

		if (p == pend)
		  {
		    if (syntax & RE_NO_BK_BRACES)
		      goto unfetch_interval;
		    else
		      FREE_STACK_RETURN (REG_EBRACE);
		  }

		GET_UNSIGNED_NUMBER (lower_bound);

		if (c == ',')
		  {
		    GET_UNSIGNED_NUMBER (upper_bound);
		    if (upper_bound < 0) upper_bound = RE_DUP_MAX;
		  }
		else
		  /* Interval such as `{1}' => match exactly once. */
		  upper_bound = lower_bound;

		if (lower_bound < 0 || upper_bound > RE_DUP_MAX
		    || lower_bound > upper_bound)
		  {
		    if (syntax & RE_NO_BK_BRACES)
		      goto unfetch_interval;
		    else
		      FREE_STACK_RETURN (REG_BADBR);
		  }

		if (!(syntax & RE_NO_BK_BRACES))
		  {
		    if (c != '\\') FREE_STACK_RETURN (REG_EBRACE);

		    PATFETCH (c);
		  }

		if (c != '}')
		  {
		    if (syntax & RE_NO_BK_BRACES)
		      goto unfetch_interval;
		    else
		      FREE_STACK_RETURN (REG_BADBR);
		  }

		/* We just parsed a valid interval.  */

		/* If it's invalid to have no preceding re.  */
		if (!laststart)
		  {
		    if (syntax & RE_CONTEXT_INVALID_OPS)
		      FREE_STACK_RETURN (REG_BADRPT);
		    else if (syntax & RE_CONTEXT_INDEP_OPS)
		      laststart = b;
		    else
		      goto unfetch_interval;
		  }

		/* If the upper bound is zero, don't want to succeed at
		   all; VRE_JumpTo from `laststart' to `b + 3', which will be
		   the end of the buffer after we insert the VRE_JumpTo.  */
		 if (upper_bound == 0)
		   {
		     GET_BUFFER_SPACE (3);
		     INSERT_JUMP (VRE_JumpTo, laststart, b + 3);
		     b += 3;
		   }

		 /* Otherwise, we have a nontrivial interval.  When
		    we're all done, the pattern will look like:
		      VRE_SetAddress <VRE_JumpTo count> <upper bound>
		      VRE_SetAddress <VRE_JumpAfterNSuccess count> <lower bound>
		      VRE_JumpAfterNSuccess <after VRE_JumpTo addr> <VRE_JumpAfterNSuccess count>
		      <body of loop>
		      VRE_JumpNTimes <VRE_JumpAfterNSuccess addr> <VRE_JumpTo count>
		    (The upper bound and `VRE_JumpNTimes' are omitted if
		    `upper_bound' is 1, though.)  */
		 else
		   { /* If the upper bound is > 1, we need to insert
			more at the end of the loop.  */
		     unsigned nbytes = 10 + (upper_bound > 1) * 10;

		     GET_BUFFER_SPACE (nbytes);

		     /* Initialize lower bound of the `VRE_JumpAfterNSuccess', even
			though it will be set during matching by its
			attendant `VRE_SetAddress' (inserted next),
			because `vregexp_CompFastMap' needs to know.
			Jump to the `VRE_JumpNTimes' we might insert below.  */
		     INSERT_JUMP2 (VRE_JumpAfterNSuccess, laststart,
				   b + 5 + (upper_bound > 1) * 5,
				   lower_bound);
		     b += 5;

		     /* Code to initialize the lower bound.  Insert
			before the `VRE_JumpAfterNSuccess'.  The `5' is the last two
			bytes of this `VRE_SetAddress', plus 3 bytes of
			the following `VRE_JumpAfterNSuccess'.  */
		     vregexp_InsOp2 (VRE_SetAddress, laststart, 5, lower_bound, b);
		     b += 5;

		     if (upper_bound > 1)
		       { /* More than one repetition is allowed, so
			    append a backward VRE_JumpTo to the `VRE_JumpAfterNSuccess'
			    that starts this interval.

			    When we've reached this during matching,
			    we'll have matched the interval once, so
			    VRE_JumpTo back only `upper_bound - 1' times.  */
			 STORE_JUMP2 (VRE_JumpNTimes, b, laststart + 5,
				      upper_bound - 1);
			 b += 5;

			 /* The location we want to set is the second
			    parameter of the `VRE_JumpNTimes'; that is `b-2' as
			    an absolute address.  `laststart' will be
			    the `VRE_SetAddress' we're about to insert;
			    `laststart+3' the number to set, the source
			    for the relative address.  But we are
			    inserting into the middle of the pattern --
			    so everything is getting moved up by 5.
			    Conclusion: (b - 2) - (laststart + 3) + 5,
			    i.e., b - laststart.

			    We insert this at the beginning of the loop
			    so that if we fail during matching, we'll
			    reinitialize the bounds.  */
			 vregexp_InsOp2 (VRE_SetAddress, laststart, b - laststart,
				     upper_bound - 1, b);
			 b += 5;
		       }
		   }
		pending_exact = 0;
		beg_interval = NULL;
	      }
	      break;

	    unfetch_interval:
	      /* If an invalid interval, match the characters as literals.  */
	       p = beg_interval;
	       beg_interval = NULL;

	       /* normal_char and normal_backslash need `c'.  */
	       PATFETCH (c);

	       if (!(syntax & RE_NO_BK_BRACES))
		 {
		   if (p > pattern  &&	p[-1] == '\\')
		     goto normal_backslash;
		 }
	       goto normal_char;


	    case 'w':
	      if (re_syntax_options & RE_NO_GNU_OPS)
		goto normal_char;
	      laststart = b;
	      BUF_PUSH (VRE_MatchWordChar);
	      break;


	    case 'W':
	      if (re_syntax_options & RE_NO_GNU_OPS)
		goto normal_char;
	      laststart = b;
	      BUF_PUSH (VRE_MatchNotWordChar);
	      break;


	    case '<':
	      if (re_syntax_options & RE_NO_GNU_OPS)
		goto normal_char;
	      BUF_PUSH (VRE_MatchWordBgn);
	      break;

	    case '>':
	      if (re_syntax_options & RE_NO_GNU_OPS)
		goto normal_char;
	      BUF_PUSH (VRE_MatchWordEnd);
	      break;

	    case 'b':
	      if (re_syntax_options & RE_NO_GNU_OPS)
		goto normal_char;
	      BUF_PUSH (VRE_MatchWordBnd);
	      break;

	    case 'B':
	      if (re_syntax_options & RE_NO_GNU_OPS)
		goto normal_char;
	      BUF_PUSH (VRE_MatchNotWordBnd);
	      break;

	    case '`':
	      if (re_syntax_options & RE_NO_GNU_OPS)
		goto normal_char;
	      BUF_PUSH (VRE_MatchBOS);
	      break;

	    case '\'':
	      if (re_syntax_options & RE_NO_GNU_OPS)
		goto normal_char;
	      BUF_PUSH (VRE_MatchEOS);
	      break;

	    case '1': case '2': case '3': case '4': case '5':
	    case '6': case '7': case '8': case '9':
	      if (syntax & RE_NO_BK_REFS)
		goto normal_char;

	      c1 = c - '0';

	      if (c1 > regnum)
		FREE_STACK_RETURN (REG_ESUBREG);

	      /* Can't back reference to a subexpression if inside of it.  */
	      if (vregexp_GrpInCompStack (compile_stack, (regnum_t) c1))
		goto normal_char;

	      laststart = b;
	      BUF_PUSH_2 (VRE_MatchDupInReg, c1);
	      break;


	    case '+':
	    case '?':
	      if (syntax & RE_BK_PLUS_QM)
		goto handle_plus;
	      else
		goto normal_backslash;

	    default:
	    normal_backslash:
	      /* You might think it would be useful for \ to mean
		 not to translate; but if we don't translate it
		 it will never match anything.	*/
	      c = TRANSLATE (c);
	      goto normal_char;
	    }
	  break;


	default:
	/* Expects the character in `c'.  */
	normal_char:
	      /* If no VRE_ExactN currently being built.  */
	  if (!pending_exact

	      /* If last VRE_ExactN not at current position.  */
	      || pending_exact + *pending_exact + 1 != b

	      /* We have only one byte following the VRE_ExactN for the count.  */
	      || *pending_exact == (1 << BYTEWIDTH) - 1

	      /* If followed by a repetition operator.	*/
	      || *p == '*' || *p == '^'
	      || ((syntax & RE_BK_PLUS_QM)
		  ? *p == '\\' && (p[1] == '+' || p[1] == '?')
		  : (*p == '+' || *p == '?'))
	      || ((syntax & RE_INTERVALS)
		  && ((syntax & RE_NO_BK_BRACES)
		      ? *p == '{'
		      : (p[0] == '\\' && p[1] == '{'))))
	    {
	      /* Start building a new VRE_ExactN.  */

	      laststart = b;

	      BUF_PUSH_2 (VRE_ExactN, 0);
	      pending_exact = b - 1;
	    }

	  BUF_PUSH (c);
	  (*pending_exact)++;
	  break;
	} /* switch (c) */
    } /* while p != pend */


  /* Through the pattern now.  */

  if (fixup_alt_jump)
    STORE_JUMP (VRE_JumpToAlt, fixup_alt_jump, b);

  if (!COMPILE_STACK_EMPTY)
    FREE_STACK_RETURN (REG_EPAREN);

  /* If we don't want backtracking, force success
     the first time we reach the end of the compiled pattern.  */
  if (syntax & RE_NO_POSIX_BACKTRACKING)
    BUF_PUSH (VRE_Succeed);

  free (compile_stack.stack);

  /* We have succeeded; set the length of the buffer.  */
  bufp->used = b - bufp->buffer;


  return REG_NOERROR;
} /* vregexp_Compile */

/* Subroutines for `vregexp_Compile'.  */

/* Store OP at LOC followed by two-byte integer parameter ARG.	*/

static void
vregexp_StoreOp1 (op, loc, arg)
    re_opcode_t op;
    unsigned char *loc;
    int arg;
{
  *loc = (unsigned char) op;
  STORE_NUMBER (loc + 1, arg);
}


/* Like `vregexp_StoreOp1', but for two two-byte parameters ARG1 and ARG2.  */

static void
vregexp_StoreOp2 (op, loc, arg1, arg2)
    re_opcode_t op;
    unsigned char *loc;
    int arg1, arg2;
{
  *loc = (unsigned char) op;
  STORE_NUMBER (loc + 1, arg1);
  STORE_NUMBER (loc + 3, arg2);
}


/* Copy the bytes from LOC to END to open up three bytes of space at LOC
   for OP followed by two-byte integer parameter ARG.  */

static void
vregexp_InsOp1 (op, loc, arg, end)
    re_opcode_t op;
    unsigned char *loc;
    int arg;
    unsigned char *end;
{
  register unsigned char *pfrom = end;
  register unsigned char *pto = end + 3;

  while (pfrom != loc)
    *--pto = *--pfrom;

  vregexp_StoreOp1 (op, loc, arg);
}


/* Like `vregexp_InsOp1', but for two two-byte parameters ARG1 and ARG2.  */

static void
vregexp_InsOp2 (op, loc, arg1, arg2, end)
    re_opcode_t op;
    unsigned char *loc;
    int arg1, arg2;
    unsigned char *end;
{
  register unsigned char *pfrom = end;
  register unsigned char *pto = end + 5;

  while (pfrom != loc)
    *--pto = *--pfrom;

  vregexp_StoreOp2 (op, loc, arg1, arg2);
}


/* P points to just after a ^ in PATTERN.  Return TRUE if that ^ comes
   after an alternative or a begin-subexpression.  We assume there is at
   least one character before the ^.  */

static boolean
vregexp_AtBgnLineLocP (pattern, p, syntax)
    char *pattern, *p;
    reg_syntax_t syntax;
{
  char *prev = p - 2;
  boolean prev_prev_backslash = prev > pattern && prev[-1] == '\\';

  return
       /* After a subexpression?  */
       (*prev == '(' && (syntax & RE_NO_BK_PARENS || prev_prev_backslash))
       /* After an alternative?  */
    || (*prev == '|' && (syntax & RE_NO_BK_VBAR || prev_prev_backslash));
}


/* The dual of vregexp_AtBgnLineLocP.  This one is for $.  We assume there is
   at least one character after the $, i.e., `P < PEND'.  */

static boolean
vregexp_AtEndLineLocP (p, pend, syntax)
    char *p, *pend;
    reg_syntax_t syntax;
{
  char *next = p;
  boolean next_backslash = *next == '\\';
  char *next_next = p + 1 < pend ? p + 1 : 0;

  return
       /* Before a subexpression?  */
       (syntax & RE_NO_BK_PARENS ? *next == ')'
	: next_backslash && next_next && *next_next == ')')
       /* Before an alternative?  */
    || (syntax & RE_NO_BK_VBAR ? *next == '|'
	: next_backslash && next_next && *next_next == '|');
}


/* Returns TRUE if REGNUM is in one of COMPILE_STACK's elements and
   FALSE if it's not.  */

static boolean
vregexp_GrpInCompStack (compile_stack, regnum)
    compile_stack_type compile_stack;
    regnum_t regnum;
{
  int this_element;

  for (this_element = compile_stack.avail - 1;
       this_element >= 0;
       this_element--)
    if (compile_stack.stack[this_element].regnum == regnum)
      return TRUE;

  return FALSE;
}


/* Read the ending character of a range (in a bracket expression) from the
   uncompiled pattern *P_PTR (which ends at PEND).  We assume the
   starting character is in `P[-2]'.  (`P[-1]' is the character `-'.)
   Then we set the translation of all bits between the starting and
   ending characters (inclusive) in the compiled pattern B.

   Return an error code.

   We use these short variable names so we can use the same macros as
   `vregexp_Compile' itself.  */

static reg_errcode_t
vregexp_CompRange (p_ptr, pend, translate, syntax, b)
    char **p_ptr, *pend;
    RE_TRANSLATE_TYPE translate;
    reg_syntax_t syntax;
    unsigned char *b;
{
  unsigned this_char;

  char *p = *p_ptr;
  unsigned int range_start, range_end;

  if (p == pend)
    return REG_ERANGE;

  /* Even though the pattern is a signed `char *', we need to fetch
     with unsigned char *'s; if the high bit of the pattern character
     is set, the range endpoints will be negative if we fetch using a
     signed char *.

     We also want to fetch the endpoints without translating them; the
     appropriate translation is done in the bit-setting loop below.  */
  /* The SVR4 compiler on the 3B2 had trouble with unsigned char *.  */
  range_start = ((unsigned char *) p)[-2];
  range_end   = ((unsigned char *) p)[0];

  /* Have to increment the pointer into the pattern string, so the
     caller isn't still at the ending character.  */
  (*p_ptr)++;

  /* If the start is after the end, the range is empty.  */
  if (range_start > range_end)
    return syntax & RE_NO_EMPTY_RANGES ? REG_ERANGE : REG_NOERROR;

  /* Here we see why `this_char' has to be larger than an `unsigned
     char' -- the range is inclusive, so if `range_end' == 0xff
     (assuming 8-bit characters), we would otherwise go into an infinite
     loop, since all characters <= 0xff.  */
  for (this_char = range_start; this_char <= range_end; this_char++)
    {
      SET_LIST_BIT (TRANSLATE (this_char));
    }

  return REG_NOERROR;
}

/* vregexp_CompFastMap computes a ``fastmap'' for the compiled pattern in
   BUFP.  A fastmap records which of the (1 << BYTEWIDTH) possible
   characters can start a string that matches the pattern.  This fastmap
   is used by vregexp_Search to skip quickly over impossible starting points.

   The caller must supply the address of a (1 << BYTEWIDTH)-byte data
   area as BUFP->fastmap.

   We set the `fastmap', `fastmap_accurate', and `can_be_null' fields in
   the pattern buffer.

   Returns 0 if we succeed, -2 if an internal error.   */

int
vregexp_CompFastMap (bufp)
     struct re_pattern_buffer *bufp;
{
  int j, k;
  fail_stack_type fail_stack;
  /* We don't push any register information onto the failure stack.  */
  unsigned num_regs = 0;

  register char *fastmap = bufp->fastmap;
  unsigned char *pattern = bufp->buffer;
  unsigned char *p = pattern;
  register unsigned char *pend = pattern + bufp->used;

  /* Assume that each path through the pattern can be null until
     proven otherwise.	We set this FALSE at the bottom of switch
     statement, to which we get only if a particular path doesn't
     match the empty string.  */
  boolean path_can_be_null = TRUE;

  /* We aren't doing a `VRE_JumpAfterNSuccess' to begin with.  */
  boolean succeed_n_p = FALSE;


  INIT_FAIL_STACK ();
  memset (fastmap, 0, 1 << BYTEWIDTH);  /* Assume nothing's valid.	*/
  bufp->fastmap_accurate = 1;	    /* It will be when we're done.  */
  bufp->can_be_null = 0;

  while (1)
    {
      if (p == pend || *p == VRE_Succeed)
	{
	  /* We have reached the (effective) end of pattern.  */
	  if (!FAIL_STACK_EMPTY ())
	    {
	      bufp->can_be_null |= path_can_be_null;

	      /* Reset for next path.  */
	      path_can_be_null = TRUE;

	      p = fail_stack.stack[--fail_stack.avail].pointer;

	      continue;
	    }
	  else
	    break;
	}

      /* We should never be about to go beyond the end of the pattern.	*/

      switch ((re_opcode_t) *p++)
	{

	/* I guess the idea here is to simply not bother with a fastmap
	   if a backreference is used, since it's too hard to figure out
	   the fastmap for the corresponding group.  Setting
	   `can_be_null' stops `vregexp_Search2' from using the fastmap, so
	   that is all we do.  */
	case VRE_MatchDupInReg:
	  bufp->can_be_null = 1;
	  goto done;


      /* Following are the cases which match a character.  These end
	 with `break'.	*/

	case VRE_ExactN:
	  fastmap[p[1]] = 1;
	  break;


	case VRE_MatchCharSet:
	  for (j = *p++ * BYTEWIDTH - 1; j >= 0; j--)
	    if (p[j / BYTEWIDTH] & (1 << (j % BYTEWIDTH)))
	      fastmap[j] = 1;
	  break;


	case VRE_MatchNotCharSet:
	  /* Chars beyond end of map must be allowed.  */
	  for (j = *p * BYTEWIDTH; j < (1 << BYTEWIDTH); j++)
	    fastmap[j] = 1;

	  for (j = *p++ * BYTEWIDTH - 1; j >= 0; j--)
	    if (!(p[j / BYTEWIDTH] & (1 << (j % BYTEWIDTH))))
	      fastmap[j] = 1;
	  break;


	case VRE_MatchWordChar:
	  for (j = 0; j < (1 << BYTEWIDTH); j++)
	    if (SYNTAX (j) == ISWORD)
	      fastmap[j] = 1;
	  break;


	case VRE_MatchNotWordChar:
	  for (j = 0; j < (1 << BYTEWIDTH); j++)
	    if (SYNTAX (j) != ISWORD)
	      fastmap[j] = 1;
	  break;


	case VRE_MatchAny:
	  {
	    int fastmap_newline = fastmap['\n'];

	    /* `.' matches anything ...  */
	    for (j = 0; j < (1 << BYTEWIDTH); j++)
	      fastmap[j] = 1;

	    /* ... except perhaps newline.  */
	    if (!(bufp->syntax & RE_DOT_NEWLINE))
	      fastmap['\n'] = fastmap_newline;

	    /* Return if we have already set `can_be_null'; if we have,
	       then the fastmap is irrelevant.	Something's wrong here.  */
	    else if (bufp->can_be_null)
	      goto done;

	    /* Otherwise, have to check alternative paths.  */
	    break;
	  }


	case no_op:
	case VRE_MatchBOL:
	case VRE_MatchEOL:
	case VRE_MatchBOS:
	case VRE_MatchEOS:
	case VRE_MatchWordBnd:
	case VRE_MatchNotWordBnd:
	case VRE_MatchWordBgn:
	case VRE_MatchWordEnd:
	case VRE_PushDmyFail:
	  continue;


	case VRE_JumpNTimes:
	case VRE_PopFailure:
	case VRE_MaybePopFailure:
	case VRE_JumpTo:
	case VRE_JumpToAlt:
	case VRE_DmyFailJump:
	  EXTRACT_NUMBER_AND_INCR (j, p);
	  p += j;
	  if (j > 0)
	    continue;

	  /* Jump backward implies we just went through the body of a
	     loop and matched nothing.	Opcode jumped to should be
	     `VRE_JumpOnFail' or `VRE_JumpAfterNSuccess'.	Just treat it like an
	     ordinary VRE_JumpTo.  For a * loop, it has pushed its failure
	     point already; if so, discard that as redundant.  */
	  if ((re_opcode_t) *p != VRE_JumpOnFail
	      && (re_opcode_t) *p != VRE_JumpAfterNSuccess)
	    continue;

	  p++;
	  EXTRACT_NUMBER_AND_INCR (j, p);
	  p += j;

	  /* If what's on the stack is where we are now, pop it.  */
	  if (!FAIL_STACK_EMPTY ()
	      && fail_stack.stack[fail_stack.avail - 1].pointer == p)
	    fail_stack.avail--;

	  continue;


	case VRE_JumpOnFail:
	case VRE_PushPlaceHolder:
	handle_on_failure_jump:
	  EXTRACT_NUMBER_AND_INCR (j, p);

	  /* For some patterns, e.g., `(a?)?', `p+j' here points to the
	     end of the pattern.  We don't want to push such a point,
	     since when we restore it above, entering the switch will
	     increment `p' past the end of the pattern.  We don't need
	     to push such a point since we obviously won't find any more
	     fastmap entries beyond `pend'.  Such a pattern can match
	     the null string, though.  */
	  if (p + j < pend)
	    {
	      if (!PUSH_PATTERN_OP (p + j, fail_stack))
		{
		  RESET_FAIL_STACK ();
		  return -2;
		}
	    }
	  else
	    bufp->can_be_null = 1;

	  if (succeed_n_p)
	    {
	      EXTRACT_NUMBER_AND_INCR (k, p);	/* Skip the n.	*/
	      succeed_n_p = FALSE;
	    }

	  continue;


	case VRE_JumpAfterNSuccess:
	  /* Get to the number of times to succeed.  */
	  p += 2;

	  /* Increment p past the n for when k != 0.  */
	  EXTRACT_NUMBER_AND_INCR (k, p);
	  if (k == 0)
	    {
	      p -= 4;
	      succeed_n_p = TRUE;  /* Spaghetti code alert.  */
	      goto handle_on_failure_jump;
	    }
	  continue;


	case VRE_SetAddress:
	  p += 4;
	  continue;


	case VRE_StartMatchMem:
	case VRE_StopMatchMem:
	  p += 2;
	  continue;


	default:
	  abort (); /* We have listed all the cases.  */
	} /* switch *p++ */

      /* Getting here means we have found the possible starting
	 characters for one path of the pattern -- and that the empty
	 string does not match.  We need not follow this path further.
	 Instead, look at the next alternative (remembered on the
	 stack), or quit if no more.  The test at the top of the loop
	 does these things.  */
      path_can_be_null = FALSE;
      p = pend;
    } /* while p */

  /* Set `can_be_null' for the last path (also the first path, if the
     pattern is empty).  */
  bufp->can_be_null |= path_can_be_null;

 done:
  RESET_FAIL_STACK ();
  return 0;
} /* vregexp_CompFastMap */

/* Set REGS to hold NUM_REGS registers, storing them in STARTS and
   ENDS.  Subsequent matches using PATTERN_BUFFER and REGS will use
   this memory for recording register information.  STARTS and ENDS
   must be allocated using the malloc library routine, and must each
   be at least NUM_REGS * sizeof (regoff_t) bytes long.

   If NUM_REGS == 0, then subsequent matches should allocate their own
   register data.

   Unless this function is called, the first search or match using
   PATTERN_BUFFER will allocate its own register data, without
   freeing the old data.  */

void vregexp_SetRegs (bufp, regs, num_regs, starts, ends)
    struct re_pattern_buffer *bufp;
    struct re_registers *regs;
    unsigned num_regs;
    regoff_t *starts, *ends;
{
	if (num_regs)
	 { bufp->regs_allocated = REGS_REALLOCATE; regs->num_regs = num_regs; regs->start = starts; regs->end = ends;
	 } else { bufp->regs_allocated = REGS_UNALLOCATED; regs->num_regs = 0; regs->start = regs->end = (regoff_t *) 0; }
}

/*	vregexp_Search - Like vregexp_Search2, below, but only one string is specified, and doesn't let you say where to stop matching. */

int vregexp_Search (bufp, string, size, startpos, range, regs)
     struct re_pattern_buffer *bufp;
     char *string;
     int size, startpos, range;
     struct re_registers *regs;
{
  return(vregexp_Search2(bufp, NULL, 0, string, size, startpos, range, regs, size)) ;
}


/* Using the compiled pattern in BUFP->buffer, first tries to match the
   virtual concatenation of STRING1 and STRING2, starting first at index
   STARTPOS, then at STARTPOS + 1, and so on.

   STRING1 and STRING2 have length SIZE1 and SIZE2, respectively.

   RANGE is how far to scan while trying to match.  RANGE = 0 means try
   only at STARTPOS; in general, the last start tried is STARTPOS +
   RANGE.

   In REGS, return the indices of the virtual concatenation of STRING1
   and STRING2 that matched the entire BUFP->buffer and its contained
   subexpressions.

   Do not consider matching one past the index STOP in the virtual
   concatenation of STRING1 and STRING2.

   We return either the position in the strings at which the match was
   found, -1 if no match, or -2 if error (such as failure
   stack overflow).  */

int vregexp_Search2 (bufp, string1, size1, string2, size2, startpos, range, regs, stop)
     struct re_pattern_buffer *bufp;
     char *string1, *string2;
     int size1, size2;
     int startpos;
     int range;
     struct re_registers *regs;
     int stop;
{
  int val;
  char *fastmap = bufp->fastmap;
  RE_TRANSLATE_TYPE translate = bufp->translate;
  int total_size = size1 + size2;
  int endpos = startpos + range;

  /* Check for out-of-range STARTPOS.  */
  if (startpos < 0 || startpos > total_size) return -1;

  /* Fix up RANGE if it might eventually take us outside
     the virtual concatenation of STRING1 and STRING2.
     Make sure we won't move STARTPOS below 0 or above TOTAL_SIZE.  */
  if (endpos < 0) range = 0 - startpos;
  else if (endpos > total_size)
    range = total_size - startpos;
  /* If the search isn't to be a backwards one, don't waste time in a
     search for a pattern that must be anchored.  */
  if (bufp->used > 0 && (re_opcode_t) bufp->buffer[0] == VRE_MatchBOS && range > 0)
    { if (startpos > 0)	return -1;  else range = 1; }
  /* Update the fastmap now if not correct already.  */
  if (fastmap && !bufp->fastmap_accurate)
    if (vregexp_CompFastMap (bufp) == -2)
      return -2;

  /* Loop through the string, looking for a place to start matching.  */
  for (;;)
    {
      /* If a fastmap is supplied, skip quickly over characters that
	 cannot be the start of a match.  If the pattern can match the
	 null string, however, we don't need to skip characters; we want
	 the first null string.  */
      if (fastmap && startpos < total_size && !bufp->can_be_null)
	{
	  if (range > 0)	/* Searching forwards.	*/
	    { char *d; int lim = 0; int irange = range;
	      if (startpos < size1 && startpos + range >= size1) lim = range - (size1 - startpos);
	      d = (startpos >= size1 ? string2 - size1 : string1) + startpos;
	      /* Written out as an if-else to avoid testing `translate'
		 inside the loop.  */
	      if (translate)
		while (range > lim
		       && !fastmap[(unsigned char)
				   translate[(unsigned char) *d++]])
		  range--;
	      else
		while (range > lim && !fastmap[(unsigned char) *d++])
		  range--;

	      startpos += irange - range;
	    }
	  else				/* Searching backwards.  */
	    { char c = (size1 == 0 || startpos >= size1 ? string2[startpos - size1] : string1[startpos]);
	      if (!fastmap[(unsigned char) TRANSLATE (c)]) goto advance;
	    }
	}

      /* If can't match the null string, and that's all we have left, fail.  */
      if (range >= 0 && startpos == total_size && fastmap  && !bufp->can_be_null) return -1;
      val = vregexp_Match2Int (bufp, string1, size1, string2, size2,startpos, regs, stop);
      if (val >= 0) return startpos;
      if (val == -2) return -2;
advance:
      if (!range) break;
      else if (range > 0)
	{ range--; startpos++;}
      else
	{ range++; startpos--; }
    }
  return -1;
} /* vregexp_Search2 */

/* This converts PTR, a pointer into one of the search strings `string1'
   and `string2' into an offset from the beginning of that string.  */
#define POINTER_TO_OFFSET(ptr)			\
  (FIRST_STRING_P (ptr) 			\
   ? ((regoff_t) ((ptr) - string1))		\
   : ((regoff_t) ((ptr) - string2 + size1)))

/* Macros for dealing with the split strings in vregexp_Match2.  */

#define MATCHING_IN_FIRST_STRING  (dend == end_match_1)

/* Call before fetching a character with *d.  This switches over to
   string2 if necessary.  */
#define PREFETCH()							\
  while (d == dend)							\
    {									\
      /* End of string2 => fail.  */					\
      if (dend == end_match_2)						\
	goto fail;							\
      /* End of string1 => advance to string2.	*/			\
      d = string2;							\
      dend = end_match_2;						\
    }


/* Test if at very beginning or at very end of the virtual concatenation
   of `string1' and `string2'.	If only one string, it's `string2'.  */
#define AT_STRINGS_BEG(d) ((d) == (size1 ? string1 : string2) || !size2)
#define AT_STRINGS_END(d) ((d) == end2)


/* Test if D points to a character which is word-constituent.  We have
   two special cases to check for: if past the end of string1, look at
   the first character in string2; and if before the beginning of
   string2, look at the last character in string1.  */
#define WORDCHAR_P(d)							\
  (SYNTAX ((d) == end1 ? *string2					\
	   : (d) == string2 - 1 ? *(end1 - 1) : *(d))			\
   == ISWORD)

/* Disabled due to a compiler bug -- see comment at case VRE_MatchWordBnd */
#if 0
/* Test if the character before D and the one at D differ with respect
   to being word-constituent.  */
#define AT_WORD_BOUNDARY(d)						\
  (AT_STRINGS_BEG (d) || AT_STRINGS_END (d)				\
   || WORDCHAR_P (d - 1) != WORDCHAR_P (d))
#endif

/* Free everything we malloc.  */
#define FREE_VAR(var) if (var) REGEX_FREE (var); var = NULL
#define FREE_VARIABLES()						\
  do {									\
    REGEX_FREE_STACK (fail_stack.stack);				\
    FREE_VAR (regstart);						\
    FREE_VAR (regend);							\
    FREE_VAR (old_regstart);						\
    FREE_VAR (old_regend);						\
    FREE_VAR (best_regstart);						\
    FREE_VAR (best_regend);						\
    FREE_VAR (reg_info);						\
    FREE_VAR (reg_dummy);						\
    FREE_VAR (reg_info_dummy);						\
  } while (0)

/* These values must meet several constraints.	They must not be valid
   register values; since we have a limit of 255 registers (because
   we use only one byte in the pattern for the register number), we can
   use numbers larger than 255.  They must differ by 1, because of
   NUM_FAILURE_ITEMS above.  And the value for the lowest register must
   be larger than the value for the highest register, so we do not try
   to actually save any registers when none are active.  */
#define NO_HIGHEST_ACTIVE_REG (1 << BYTEWIDTH)
#define NO_LOWEST_ACTIVE_REG (NO_HIGHEST_ACTIVE_REG + 1)

/* Matching routines.  */

/* vregexp_Match is like vregexp_Match2 except it takes only a single string.  */

int
vregexp_Match (bufp, string, size, pos, regs)
     struct re_pattern_buffer *bufp;
     char *string;
     int size, pos;
     struct re_registers *regs;
{
  int result = vregexp_Match2Int (bufp, NULL, 0, string, size,
				    pos, regs, size);
  return result;
}

static boolean vregexp_GrpMatchNullStrP _RE_ARGS ((unsigned char **p,
						    unsigned char *end,
						register_info_type *reg_info));
static boolean vregexp_AltMatchNullStrP _RE_ARGS ((unsigned char *p,
						  unsigned char *end,
						register_info_type *reg_info));
static boolean vregexp_CommOpMatchNullStrP _RE_ARGS ((unsigned char **p,
							unsigned char *end,
						register_info_type *reg_info));
static int vregexp_BCmpTran _RE_ARGS ((char *s1, char *s2,
				     int len, char *translate));

/* vregexp_Match2 matches the compiled pattern in BUFP against the
   the (virtual) concatenation of STRING1 and STRING2 (of length SIZE1
   and SIZE2, respectively).  We start matching at POS, and stop
   matching at STOP.c

   If REGS is non-null and the `no_sub' field of BUFP is nonzero, we
   store offsets for the substring each group matched in REGS.	See the
   documentation for exactly how many groups we fill.

   We return -1 if no match, -2 if an internal error (such as the
   failure stack overflowing).	Otherwise, we return the length of the
   matched substring.  */

int
vregexp_Match2 (bufp, string1, size1, string2, size2, pos, regs, stop)
     struct re_pattern_buffer *bufp;
     char *string1, *string2;
     int size1, size2;
     int pos;
     struct re_registers *regs;
     int stop;
{
  int result = vregexp_Match2Int (bufp, string1, size1, string2, size2,
				    pos, regs, stop);
  return result;
}

/* This is a separate function so that we can force an alloca cleanup
   afterwards.	*/
static int
vregexp_Match2Int (bufp, string1, size1, string2, size2, pos, regs, stop)
     struct re_pattern_buffer *bufp;
     char *string1, *string2;
     int size1, size2;
     int pos;
     struct re_registers *regs;
     int stop;
{
  /* General temporaries.  */
  int mcnt;
  unsigned char *p1;

  /* Just past the end of the corresponding string.  */
  char *end1, *end2;

  /* Pointers into string1 and string2, just past the last characters in
     each to consider matching.  */
  char *end_match_1, *end_match_2;

  /* Where we are in the data, and the end of the current string.  */
  char *d, *dend;

  /* Where we are in the pattern, and the end of the pattern.  */
  unsigned char *p = bufp->buffer;
  register unsigned char *pend = p + bufp->used;

  /* Mark the opcode just after a VRE_StartMatchMem, so we can test for an
     empty subpattern when we get to the VRE_StopMatchMem.  */
  unsigned char *just_past_start_mem = 0;

  /* We use this to map every character in the string.	*/
  RE_TRANSLATE_TYPE translate = bufp->translate;

  /* Failure point stack.  Each place that can handle a failure further
     down the line pushes a failure point on this stack.  It consists of
     restart, regend, and reg_info for all registers corresponding to
     the subexpressions we're currently inside, plus the number of such
     registers, and, finally, two char *'s.  The first char * is where
     to resume scanning the pattern; the second one is where to resume
     scanning the strings.  If the latter is zero, the failure point is
     a ``dummy''; if a failure happens and the failure point is a dummy,
     it gets discarded and the next next one is tried.	*/
  fail_stack_type fail_stack;

  /* We fill all the registers internally, independent of what we
     return, for use in backreferences.  The number here includes
     an element for register zero.  */
  size_t num_regs = bufp->re_nsub + 1;

  /* The currently active registers.  */
  active_reg_t lowest_active_reg = NO_LOWEST_ACTIVE_REG;
  active_reg_t highest_active_reg = NO_HIGHEST_ACTIVE_REG;

  /* Information on the contents of registers. These are pointers into
     the input strings; they record just what was matched (on this
     attempt) by a subexpression part of the pattern, that is, the
     regnum-th regstart pointer points to where in the pattern we began
     matching and the regnum-th regend points to right after where we
     stopped matching the regnum-th subexpression.  (The zeroth register
     keeps track of what the whole pattern matches.)  */
  char **regstart, **regend;

  /* If a group that's operated upon by a repetition operator fails to
     match anything, then the register for its start will need to be
     restored because it will have been set to wherever in the string we
     are when we last see its open-group operator.  Similarly for a
     register's end.  */
  char **old_regstart, **old_regend;

  /* The is_active field of reg_info helps us keep track of which (possibly
     nested) subexpressions we are currently in. The matched_something
     field of reg_info[reg_num] helps us tell whether or not we have
     matched any of the pattern so far this time through the reg_num-th
     subexpression.  These two fields get reset each time through any
     loop their register is in.  */
  register_info_type *reg_info;

  /* The following record the register info as found in the above
     variables when we find a match better than any we've seen before.
     This happens as we backtrack through the failure points, which in
     turn happens only if we have not yet matched the entire string. */
  unsigned best_regs_set = FALSE;
  char **best_regstart, **best_regend;

  /* Logically, this is `best_regend[0]'.  But we don't want to have to
     allocate space for that if we're not allocating space for anything
     else (see below).	Also, we never need info about register 0 for
     any of the other register vectors, and it seems rather a kludge to
     treat `best_regend' differently than the rest.  So we keep track of
     the end of the best match so far in a separate variable.  We
     initialize this to NULL so that when we backtrack the first time
     and need to test it, it's not garbage.  */
  char *match_end = NULL;

  /* This helps SET_REGS_MATCHED avoid doing redundant work.  */
  int set_regs_matched_done = 0;

  /* Used when we pop values we don't care about.  */
  char **reg_dummy;
  register_info_type *reg_info_dummy;


  INIT_FAIL_STACK ();

  /* Do not bother to initialize all the register variables if there are
     no groups in the pattern, as it takes a fair amount of time.  If
     there are groups, we include space for register 0 (the whole
     pattern), even though we never use it, since it simplifies the
     array indexing.  We should fix this.  */
  if (bufp->re_nsub)
    {
      regstart = REGEX_TALLOC (num_regs, char *);
      regend = REGEX_TALLOC (num_regs, char *);
      old_regstart = REGEX_TALLOC (num_regs, char *);
      old_regend = REGEX_TALLOC (num_regs, char *);
      best_regstart = REGEX_TALLOC (num_regs, char *);
      best_regend = REGEX_TALLOC (num_regs, char *);
      reg_info = REGEX_TALLOC (num_regs, register_info_type);
      reg_dummy = REGEX_TALLOC (num_regs, char *);
      reg_info_dummy = REGEX_TALLOC (num_regs, register_info_type);

      if (!(regstart && regend && old_regstart && old_regend && reg_info
	    && best_regstart && best_regend && reg_dummy && reg_info_dummy))
	{
	  FREE_VARIABLES ();
	  return -2;
	}
    }
  else
    {
      /* We must initialize all our variables to NULL, so that
	 `FREE_VARIABLES' doesn't try to free them.  */
      regstart = regend = old_regstart = old_regend = best_regstart
	= best_regend = reg_dummy = NULL;
      reg_info = reg_info_dummy = (register_info_type *) NULL;
    }

  /* The starting position is bogus.  */
  if (pos < 0 || pos > size1 + size2)
    {
      FREE_VARIABLES ();
      return -1;
    }

  /* Initialize subexpression text positions to -1 to mark ones that no
     VRE_StartMatchMem/VRE_StopMatchMem has been seen for. Also initialize the
     register information struct.  */
  for (mcnt = 1; (unsigned) mcnt < num_regs; mcnt++)
    {
      regstart[mcnt] = regend[mcnt]
	= old_regstart[mcnt] = old_regend[mcnt] = REG_UNSET_VALUE;

      REG_MATCH_NULL_STRING_P (reg_info[mcnt]) = MATCH_NULL_UNSET_VALUE;
      IS_ACTIVE (reg_info[mcnt]) = 0;
      MATCHED_SOMETHING (reg_info[mcnt]) = 0;
      EVER_MATCHED_SOMETHING (reg_info[mcnt]) = 0;
    }

  /* We move `string1' into `string2' if the latter's empty -- but not if
     `string1' is null.  */
  if (size2 == 0 && string1 != NULL)
    {
      string2 = string1;
      size2 = size1;
      string1 = 0;
      size1 = 0;
    }
  end1 = string1 + size1;
  end2 = string2 + size2;

  /* Compute where to stop matching, within the two strings.  */
  if (stop <= size1)
    {
      end_match_1 = string1 + stop;
      end_match_2 = string2;
    }
  else
    {
      end_match_1 = end1;
      end_match_2 = string2 + stop - size1;
    }

  /* `p' scans through the pattern as `d' scans through the data.
     `dend' is the end of the input string that `d' points within.  `d'
     is advanced into the following input string whenever necessary, but
     this happens before fetching; therefore, at the beginning of the
     loop, `d' can be pointing at the end of a string, but it cannot
     equal `string2'.  */
  if (size1 > 0 && pos <= size1)
    {
      d = string1 + pos;
      dend = end_match_1;
    }
  else
    {
      d = string2 + pos - size1;
      dend = end_match_2;
    }


  /* This loops over pattern commands.	It exits by returning from the
     function if the match is complete, or it drops through if the match
     fails at this starting point in the input data.  */
  for (;;)
    {

      if (p == pend)
	{ /* End of pattern means we might have succeeded.  */

	  /* If we haven't matched the entire string, and we want the
	     longest match, try backtracking.  */
//VEH051117 - Added extra check in case match goes to end of string
//	  if (d != end_match_2)
	  if (d != end_match_2 || bufp->want_shortest)
	    {
	      /* 1 if this match ends in the same string (string1 or string2)
		 as the best previous match.  */
	      boolean same_str_p = (FIRST_STRING_P (match_end)
				    == MATCHING_IN_FIRST_STRING);
	      /* 1 if this match is the best seen so far.  */
	      boolean best_match_p;

	      /* AIX compiler got confused when this was combined
		 with the previous declaration.  */
	      if (same_str_p)
		best_match_p = (bufp->want_shortest ? d < match_end : d > match_end);
//VEH	      if (same_str_p)
//VEH		best_match_p = d < match_end;
	      else
		best_match_p = !MATCHING_IN_FIRST_STRING;


	      if (!FAIL_STACK_EMPTY ())
		{ /* More failure points to try.  */

		  /* If exceeds best match so far, save it.  */
		  if (!best_regs_set || best_match_p)
		    {
		      best_regs_set = TRUE;
		      match_end = d;


		      for (mcnt = 1; (unsigned) mcnt < num_regs; mcnt++)
			{
			  best_regstart[mcnt] = regstart[mcnt];
			  best_regend[mcnt] = regend[mcnt];
			}
		    }
		  goto fail;
		}

	      /* If no failure points, don't restore garbage.  And if
		 last match is real best match, don't restore second
		 best one. */
	      else if (best_regs_set && !best_match_p)
		{
		restore_best_regs:
		  /* Restore best match.  It may happen that `dend ==
		     end_match_1' while the restored d is in string2.
		     For example, the pattern `x.*y.*z' against the
		     strings `x-' and `y-z-', if the two strings are
		     not consecutive in memory.  */

		  d = match_end;
		  dend = ((d >= string1 && d <= end1)
			   ? end_match_1 : end_match_2);

		  for (mcnt = 1; (unsigned) mcnt < num_regs; mcnt++)
		    {
		      regstart[mcnt] = best_regstart[mcnt];
		      regend[mcnt] = best_regend[mcnt];
		    }
		}
	    } /* d != end_match_2 */

	succeed_label:

	  /* If caller wants register contents data back, do it.  */
	  if (regs && !bufp->no_sub)
	    {
	      /* Have the register data arrays been allocated?	*/
	      if (bufp->regs_allocated == REGS_UNALLOCATED)
		{ /* No.  So allocate them with malloc.  We need one
		     extra element beyond `num_regs' for the `-1' marker
		     GNU code uses.  */
		  regs->num_regs = MAX (RE_NREGS, num_regs + 1);
		  regs->start = TALLOC (regs->num_regs, regoff_t);
		  regs->end = TALLOC (regs->num_regs, regoff_t);
		  if (regs->start == NULL || regs->end == NULL)
		    {
		      FREE_VARIABLES ();
		      return -2;
		    }
		  bufp->regs_allocated = REGS_REALLOCATE;
		}
	      else if (bufp->regs_allocated == REGS_REALLOCATE)
		{ /* Yes.  If we need more elements than were already
		     allocated, reallocate them.  If we need fewer, just
		     leave it alone.  */
		  if (regs->num_regs < num_regs + 1)
		    {
		      regs->num_regs = num_regs + 1;
		      RETALLOC (regs->start, regs->num_regs, regoff_t);
		      RETALLOC (regs->end, regs->num_regs, regoff_t);
		      if (regs->start == NULL || regs->end == NULL)
			{
			  FREE_VARIABLES ();
			  return -2;
			}
		    }
		}
	      else
		{
		  /* These braces fend off a "empty body in an else-statement"
		     warning under GCC when assert expands to nothing.	*/
		}

	      /* Convert the pointer data in `regstart' and `regend' to
		 indices.  Register zero has to be set differently,
		 since we haven't kept track of any info for it.  */
	      if (regs->num_regs > 0)
		{
		  regs->start[0] = pos;
		  regs->end[0] = (MATCHING_IN_FIRST_STRING
				  ? ((regoff_t) (d - string1))
				  : ((regoff_t) (d - string2 + size1)));
		}

	      /* Go through the first `min (num_regs, regs->num_regs)'
		 registers, since that is all we initialized.  */
	      for (mcnt = 1; (unsigned) mcnt < MIN (num_regs, regs->num_regs);
		   mcnt++)
		{
		  if (REG_UNSET (regstart[mcnt]) || REG_UNSET (regend[mcnt]))
		    regs->start[mcnt] = regs->end[mcnt] = -1;
		  else
		    {
		      regs->start[mcnt]
			= (regoff_t) POINTER_TO_OFFSET (regstart[mcnt]);
		      regs->end[mcnt]
			= (regoff_t) POINTER_TO_OFFSET (regend[mcnt]);
		    }
		}

	      /* If the regs structure we return has more elements than
		 were in the pattern, set the extra elements to -1.  If
		 we (re)allocated the registers, this is the case,
		 because we always allocate enough to have at least one
		 -1 at the end.  */
	      for (mcnt = num_regs; (unsigned) mcnt < regs->num_regs; mcnt++)
		regs->start[mcnt] = regs->end[mcnt] = -1;
	    } /* regs && !bufp->no_sub */


	  mcnt = d - pos - (MATCHING_IN_FIRST_STRING
			    ? string1
			    : string2 - size1);

	  FREE_VARIABLES ();
	  return mcnt;
	}

      /* Otherwise match next pattern command.	*/
      switch ((re_opcode_t) *p++)
	{
	/* Ignore these.  Used to ignore the n of VRE_JumpAfterNSuccess's which
	   currently have n == 0.  */
	case no_op:
	  break;

	case VRE_Succeed:
	  goto succeed_label;

	/* Match the next n pattern characters exactly.  The following
	   byte in the pattern defines n, and the n bytes after that
	   are the characters to match.  */
	case VRE_ExactN:
	  mcnt = *p++;

	  /* This is written out as an if-else so we don't waste time
	     testing `translate' inside the loop.  */
	  if (translate)
	    {
	      do
		{
		  PREFETCH ();
		  if ((unsigned char) translate[(unsigned char) *d++]
		      != (unsigned char) *p++)
		    goto fail;
		}
	      while (--mcnt);
	    }
	  else
	    {
	      do
		{
		  PREFETCH ();
		  if (*d++ != (char) *p++) goto fail;
		}
	      while (--mcnt);
	    }
	  SET_REGS_MATCHED ();
	  break;


	/* Match any character except possibly a newline or a null.  */
	case VRE_MatchAny:

	  PREFETCH ();

	  if ((!(bufp->syntax & RE_DOT_NEWLINE) && TRANSLATE (*d) == '\n')
	      || (bufp->syntax & RE_DOT_NOT_NULL && TRANSLATE (*d) == '\000'))
	    goto fail;

	  SET_REGS_MATCHED ();
	  d++;
	  break;


	case VRE_MatchCharSet:
	case VRE_MatchNotCharSet:
	  {
	    register unsigned char c;
	    boolean not = (re_opcode_t) *(p - 1) == VRE_MatchNotCharSet;

	    PREFETCH ();
	    c = TRANSLATE (*d); /* The character to match.  */

	    /* Cast to `unsigned' instead of `unsigned char' in case the
	       bit list is a full 32 bytes long.  */
	    if (c < (unsigned) (*p * BYTEWIDTH)
		&& p[1 + c / BYTEWIDTH] & (1 << (c % BYTEWIDTH)))
	      not = !not;

	    p += 1 + *p;

	    if (!not) goto fail;

	    SET_REGS_MATCHED ();
	    d++;
	    break;
	  }


	/* The beginning of a group is represented by VRE_StartMatchMem.
	   The arguments are the register number in the next byte, and the
	   number of groups inner to this one in the next.  The text
	   matched within the group is recorded (in the internal
	   registers data structure) under the register number.  */
	case VRE_StartMatchMem:

	  /* Find out if this group can match the empty string.  */
	  p1 = p;		/* To send to vregexp_GrpMatchNullStrP.  */

	  if (REG_MATCH_NULL_STRING_P (reg_info[*p]) == MATCH_NULL_UNSET_VALUE)
	    REG_MATCH_NULL_STRING_P (reg_info[*p])
	      = vregexp_GrpMatchNullStrP (&p1, pend, reg_info);

	  /* Save the position in the string where we were the last time
	     we were at this open-group operator in case the group is
	     operated upon by a repetition operator, e.g., with `(a*)*b'
	     against `ab'; then we want to ignore where we are now in
	     the string in case this attempt to match fails.  */
	  old_regstart[*p] = REG_MATCH_NULL_STRING_P (reg_info[*p])
			     ? REG_UNSET (regstart[*p]) ? d : regstart[*p]
			     : regstart[*p];

	  regstart[*p] = d;

	  IS_ACTIVE (reg_info[*p]) = 1;
	  MATCHED_SOMETHING (reg_info[*p]) = 0;

	  /* Clear this whenever we change the register activity status.  */
	  set_regs_matched_done = 0;

	  /* This is the new highest active register.  */
	  highest_active_reg = *p;

	  /* If nothing was active before, this is the new lowest active
	     register.	*/
	  if (lowest_active_reg == NO_LOWEST_ACTIVE_REG)
	    lowest_active_reg = *p;

	  /* Move past the register number and inner group count.  */
	  p += 2;
	  just_past_start_mem = p;

	  break;


	/* The VRE_StopMatchMem opcode represents the end of a group.  Its
	   arguments are the same as VRE_StartMatchMem's: the register
	   number, and the number of inner groups.  */
	case VRE_StopMatchMem:

	  /* We need to save the string position the last time we were at
	     this close-group operator in case the group is operated
	     upon by a repetition operator, e.g., with `((a*)*(b*)*)*'
	     against `aba'; then we want to ignore where we are now in
	     the string in case this attempt to match fails.  */
	  old_regend[*p] = REG_MATCH_NULL_STRING_P (reg_info[*p])
			   ? REG_UNSET (regend[*p]) ? d : regend[*p]
			   : regend[*p];

	  regend[*p] = d;

	  /* This register isn't active anymore.  */
	  IS_ACTIVE (reg_info[*p]) = 0;

	  /* Clear this whenever we change the register activity status.  */
	  set_regs_matched_done = 0;

	  /* If this was the only register active, nothing is active
	     anymore.  */
	  if (lowest_active_reg == highest_active_reg)
	    {
	      lowest_active_reg = NO_LOWEST_ACTIVE_REG;
	      highest_active_reg = NO_HIGHEST_ACTIVE_REG;
	    }
	  else
	    { /* We must scan for the new highest active register, since
		 it isn't necessarily one less than now: consider
		 (a(b)c(d(e)f)g).  When group 3 ends, after the f), the
		 new highest active register is 1.  */
	      unsigned char r = *p - 1;
	      while (r > 0 && !IS_ACTIVE (reg_info[r]))
		r--;

	      /* If we end up at register zero, that means that we saved
		 the registers as the result of an `VRE_JumpOnFail', not
		 a `VRE_StartMatchMem', and we jumped to past the innermost
		 `VRE_StopMatchMem'.  For example, in ((.)*) we save
		 registers 1 and 2 as a result of the *, but when we pop
		 back to the second ), we are at the VRE_StopMatchMem 1.
		 Thus, nothing is active.  */
	      if (r == 0)
		{
		  lowest_active_reg = NO_LOWEST_ACTIVE_REG;
		  highest_active_reg = NO_HIGHEST_ACTIVE_REG;
		}
	      else
		highest_active_reg = r;
	    }

	  /* If just failed to match something this time around with a
	     group that's operated on by a repetition operator, try to
	     force exit from the ``loop'', and restore the register
	     information for this group that we had before trying this
	     last match.  */
	  if ((!MATCHED_SOMETHING (reg_info[*p])
	       || just_past_start_mem == p - 1)
	      && (p + 2) < pend)
	    {
	      boolean is_a_jump_n = FALSE;

	      p1 = p + 2;
	      mcnt = 0;
	      switch ((re_opcode_t) *p1++)
		{
		  case VRE_JumpNTimes:
		    is_a_jump_n = TRUE;
		  case VRE_PopFailure:
		  case VRE_MaybePopFailure:
		  case VRE_JumpTo:
		  case VRE_DmyFailJump:
		    EXTRACT_NUMBER_AND_INCR (mcnt, p1);
		    if (is_a_jump_n)
		      p1 += 2;
		    break;

		  default:
		    /* do nothing */ ;
		}
	      p1 += mcnt;

	      /* If the next operation is a VRE_JumpTo backwards in the pattern
		 to an VRE_JumpOnFail right before the VRE_StartMatchMem
		 corresponding to this VRE_StopMatchMem, exit from the loop
		 by forcing a failure after pushing on the stack the
		 VRE_JumpOnFail's VRE_JumpTo in the pattern, and d.	*/
	      if (mcnt < 0 && (re_opcode_t) *p1 == VRE_JumpOnFail
		  && (re_opcode_t) p1[3] == VRE_StartMatchMem && p1[4] == *p)
		{
		  /* If this group ever matched anything, then restore
		     what its registers were before trying this last
		     failed match, e.g., with `(a*)*b' against `ab' for
		     regstart[1], and, e.g., with `((a*)*(b*)*)*'
		     against `aba' for regend[3].

		     Also restore the registers for inner groups for,
		     e.g., `((a*)(b*))*' against `aba' (register 3 would
		     otherwise get trashed).  */

		  if (EVER_MATCHED_SOMETHING (reg_info[*p]))
		    {
		      unsigned r;

		      EVER_MATCHED_SOMETHING (reg_info[*p]) = 0;

		      /* Restore this and inner groups' (if any) registers.  */
		      for (r = *p; r < (unsigned) *p + (unsigned) *(p + 1);
			   r++)
			{
			  regstart[r] = old_regstart[r];

			  /* xx why this test?	*/
			  if (old_regend[r] >= regstart[r])
			    regend[r] = old_regend[r];
			}
		    }
		  p1++;
		  EXTRACT_NUMBER_AND_INCR (mcnt, p1);
		  PUSH_FAILURE_POINT (p1 + mcnt, d, -2);

		  goto fail;
		}
	    }

	  /* Move past the register number and the inner group count.  */
	  p += 2;
	  break;


	/* \<digit> has been turned into a `VRE_MatchDupInReg' command which is
	   followed by the numeric value of <digit> as the register number.  */
	case VRE_MatchDupInReg:
	  {
	    register char *d2, *dend2;
	    int regno = *p++;	/* Get which register to match against.  */
	    /* Can't back reference a group which we've never matched.	*/
	    if (REG_UNSET (regstart[regno]) || REG_UNSET (regend[regno]))
	      goto fail;

	    /* Where in input to try to start matching.  */
	    d2 = regstart[regno];

	    /* Where to stop matching; if both the place to start and
	       the place to stop matching are in the same string, then
	       set to the place to stop, otherwise, for now have to use
	       the end of the first string.  */

	    dend2 = ((FIRST_STRING_P (regstart[regno])
		      == FIRST_STRING_P (regend[regno]))
		     ? regend[regno] : end_match_1);
	    for (;;)
	      {
		/* If necessary, advance to next segment in register
		   contents.  */
		while (d2 == dend2)
		  {
		    if (dend2 == end_match_2) break;
		    if (dend2 == regend[regno]) break;

		    /* End of string1 => advance to string2. */
		    d2 = string2;
		    dend2 = regend[regno];
		  }
		/* At end of register contents => success */
		if (d2 == dend2) break;

		/* If necessary, advance to next segment in data.  */
		PREFETCH ();

		/* How many characters left in this segment to match.  */
		mcnt = dend - d;

		/* Want how many consecutive characters we can match in
		   one shot, so, if necessary, adjust the count.  */
		if (mcnt > dend2 - d2)
		  mcnt = dend2 - d2;

		/* Compare that many; failure if mismatch, else move
		   past them.  */
		if (translate
		    ? vregexp_BCmpTran (d, d2, mcnt, translate)
		    : memcmp (d, d2, mcnt))
		  goto fail;
		d += mcnt, d2 += mcnt;

		/* Do this because we've match some characters.  */
		SET_REGS_MATCHED ();
	      }
	  }
	  break;


	/* VRE_MatchBOL matches the empty string at the beginning of the string
	   (unless `not_bol' is set in `bufp'), and, if
	   `newline_anchor' is set, after newlines.  */
	case VRE_MatchBOL:

	  if (AT_STRINGS_BEG (d))
	    {
	      if (!bufp->not_bol) break;
	    }
	  else if (d[-1] == '\n' && bufp->newline_anchor)
	    {
	      break;
	    }
	  /* In all other cases, we fail.  */
	  goto fail;


	/* VRE_MatchEOL is the dual of VRE_MatchBOL.  */
	case VRE_MatchEOL:

	  if (AT_STRINGS_END (d))
	    {
	      if (!bufp->not_eol) break;
	    }

	  /* We have to ``prefetch'' the next character.  */
	  else if ((d == end1 ? *string2 : *d) == '\n'
		   && bufp->newline_anchor)
	    {
	      break;
	    }
	  goto fail;


	/* Match at the very beginning of the data.  */
	case VRE_MatchBOS:
	  if (AT_STRINGS_BEG (d))
	    break;
	  goto fail;


	/* Match at the very end of the data.  */
	case VRE_MatchEOS:
	  if (AT_STRINGS_END (d))
	    break;
	  goto fail;


	/* VRE_PushPlaceHolder is used to optimize `.*\n'.  It
	   pushes NULL as the value for the string on the stack.  Then
	   `pop_failure_point' will keep the current value for the
	   string, instead of restoring it.  To see why, consider
	   matching `foo\nbar' against `.*\n'.	The .* matches the foo;
	   then the . fails against the \n.  But the next thing we want
	   to do is match the \n against the \n; if we restored the
	   string value, we would be back at the foo.

	   Because this is used only in specific cases, we don't need to
	   check all the things that `VRE_JumpOnFail' does, to make
	   sure the right things get saved on the stack.  Hence we don't
	   share its code.  The only reason to push anything on the
	   stack at all is that otherwise we would have to change
	   `VRE_MatchAny's code to do something besides goto fail in this
	   case; that seems worse than this.  */
	case VRE_PushPlaceHolder:

	  EXTRACT_NUMBER_AND_INCR (mcnt, p);

	  PUSH_FAILURE_POINT (p + mcnt, NULL, -2);
	  break;


	/* Uses of VRE_JumpOnFail:

	   Each alternative starts with an VRE_JumpOnFail that points
	   to the beginning of the next alternative.  Each alternative
	   except the last ends with a VRE_JumpTo that in effect jumps past
	   the rest of the alternatives.  (They really VRE_JumpTo to the
	   ending VRE_JumpTo of the following alternative, because tensioning
	   these jumps is a hassle.)

	   Repeats start with an VRE_JumpOnFail that points past both
	   the repetition text and either the following VRE_JumpTo or
	   VRE_PopFailure back to this VRE_JumpOnFail.  */
	case VRE_JumpOnFail:
	on_failure:

	  EXTRACT_NUMBER_AND_INCR (mcnt, p);

	  /* If this VRE_JumpOnFail comes right before a group (i.e.,
	     the original * applied to a group), save the information
	     for that group and all inner ones, so that if we fail back
	     to this point, the group's information will be correct.
	     For example, in \(a*\)*\1, we need the preceding group,
	     and in \(zz\(a*\)b*\)\2, we need the inner group.	*/

	  /* We can't use `p' to check ahead because we push
	     a failure point to `p + mcnt' after we do this.  */
	  p1 = p;

	  /* We need to skip no_op's before we look for the
	     VRE_StartMatchMem in case this VRE_JumpOnFail is happening as
	     the result of a completed VRE_JumpAfterNSuccess, as in \(a\)\{1,3\}b\1
	     against aba.  */
	  while (p1 < pend && (re_opcode_t) *p1 == no_op)
	    p1++;

	  if (p1 < pend && (re_opcode_t) *p1 == VRE_StartMatchMem)
	    {
	      /* We have a new highest active register now.  This will
		 get reset at the VRE_StartMatchMem we are about to get to,
		 but we will have saved all the registers relevant to
		 this repetition op, as described above.  */
	      highest_active_reg = *(p1 + 1) + *(p1 + 2);
	      if (lowest_active_reg == NO_LOWEST_ACTIVE_REG)
		lowest_active_reg = *(p1 + 1);
	    }

	  PUSH_FAILURE_POINT (p + mcnt, d, -2);
	  break;


	/* A smart repeat ends with `VRE_MaybePopFailure'.
	   We change it to either `VRE_PopFailure' or `VRE_JumpTo'.  */
	case VRE_MaybePopFailure:
	  EXTRACT_NUMBER_AND_INCR (mcnt, p);
	  {
	    register unsigned char *p2 = p;

	    /* Compare the beginning of the repeat with what in the
	       pattern follows its end. If we can establish that there
	       is nothing that they would both match, i.e., that we
	       would have to backtrack because of (as in, e.g., `a*a')
	       then we can change to VRE_PopFailure, because we'll
	       never have to backtrack.

	       This is not TRUE in the case of alternatives: in
	       `(a|ab)*' we do need to backtrack to the `ab' alternative
	       (e.g., if the string was `ab').	But instead of trying to
	       detect that here, the alternative has put on a dummy
	       failure point which is what we will end up popping.  */

	    /* Skip over open/close-group commands.
	       If what follows this loop is a ...+ construct,
	       look at what begins its body, since we will have to
	       match at least one of that.  */
	    while (1)
	      {
		if (p2 + 2 < pend
		    && ((re_opcode_t) *p2 == VRE_StopMatchMem
			|| (re_opcode_t) *p2 == VRE_StartMatchMem))
		  p2 += 3;
		else if (p2 + 6 < pend
			 && (re_opcode_t) *p2 == VRE_DmyFailJump)
		  p2 += 6;
		else
		  break;
	      }

	    p1 = p + mcnt;
	    /* p1[0] ... p1[2] are the `VRE_JumpOnFail' corresponding
	       to the `maybe_finalize_jump' of this case.  Examine what
	       follows.  */

	    /* If we're at the end of the pattern, we can change.  */
	    if (p2 == pend)
	      {
		/* Consider what happens when matching ":\(.*\)"
		   against ":/".  I don't really understand this code
		   yet.  */
		p[-3] = (unsigned char) VRE_PopFailure;
	      }

	    else if ((re_opcode_t) *p2 == VRE_ExactN
		     || (bufp->newline_anchor && (re_opcode_t) *p2 == VRE_MatchEOL))
	      {
		register unsigned char c
		  = *p2 == (unsigned char) VRE_MatchEOL ? '\n' : p2[2];

		if ((re_opcode_t) p1[3] == VRE_ExactN && p1[5] != c)
		  {
		    p[-3] = (unsigned char) VRE_PopFailure;
		  }

		else if ((re_opcode_t) p1[3] == VRE_MatchCharSet
			 || (re_opcode_t) p1[3] == VRE_MatchNotCharSet)
		  {
		    int not = (re_opcode_t) p1[3] == VRE_MatchNotCharSet;

		    if (c < (unsigned char) (p1[4] * BYTEWIDTH)
			&& p1[5 + c / BYTEWIDTH] & (1 << (c % BYTEWIDTH)))
		      not = !not;

		    /* `not' is equal to 1 if c would match, which means
			that we can't change to VRE_PopFailure.  */
		    if (!not)
		      {
			p[-3] = (unsigned char) VRE_PopFailure;
		      }
		  }
	      }
	    else if ((re_opcode_t) *p2 == VRE_MatchCharSet)
	      {

#if 0
		if ((re_opcode_t) p1[3] == VRE_ExactN
		    && ! ((int) p2[1] * BYTEWIDTH > (int) p1[5]
			  && (p2[2 + p1[5] / BYTEWIDTH]
			      & (1 << (p1[5] % BYTEWIDTH)))))
#else
		if ((re_opcode_t) p1[3] == VRE_ExactN
		    && ! ((int) p2[1] * BYTEWIDTH > (int) p1[4]
			  && (p2[2 + p1[4] / BYTEWIDTH]
			      & (1 << (p1[4] % BYTEWIDTH)))))
#endif
		  {
		    p[-3] = (unsigned char) VRE_PopFailure;
		  }

		else if ((re_opcode_t) p1[3] == VRE_MatchNotCharSet)
		  {
		    int idx;
		    /* We win if the VRE_MatchNotCharSet inside the loop
		       lists every character listed in the VRE_MatchCharSet after.  */
		    for (idx = 0; idx < (int) p2[1]; idx++)
		      if (! (p2[2 + idx] == 0
			     || (idx < (int) p1[4]
				 && ((p2[2 + idx] & ~ p1[5 + idx]) == 0))))
			break;

		    if (idx == p2[1])
		      {
			p[-3] = (unsigned char) VRE_PopFailure;
		      }
		  }
		else if ((re_opcode_t) p1[3] == VRE_MatchCharSet)
		  {
		    int idx;
		    /* We win if the VRE_MatchCharSet inside the loop
		       has no overlap with the one after the loop.  */
		    for (idx = 0;
			 idx < (int) p2[1] && idx < (int) p1[4];
			 idx++)
		      if ((p2[2 + idx] & p1[5 + idx]) != 0)
			break;

		    if (idx == p2[1] || idx == p1[4])
		      {
			p[-3] = (unsigned char) VRE_PopFailure;
		      }
		  }
	      }
	  }
	  p -= 2;		/* Point at relative address again.  */
	  if ((re_opcode_t) p[-1] != VRE_PopFailure)
	    {
	      p[-1] = (unsigned char) VRE_JumpTo;
	      goto unconditional_jump;
	    }
	/* Note fall through.  */


	/* The end of a simple repeat has a VRE_PopFailure back to
	   its matching VRE_JumpOnFail, where the latter will push a
	   failure point.  The VRE_PopFailure takes off failure
	   points put on by this VRE_PopFailure's matching
	   VRE_JumpOnFail; we got through the pattern to here from the
	   matching VRE_JumpOnFail, so didn't fail.  */
	case VRE_PopFailure:
	  {
	    /* We need to pass separate storage for the lowest and
	       highest registers, even though we don't care about the
	       actual values.  Otherwise, we will restore only one
	       register from the stack, since lowest will == highest in
	       `pop_failure_point'.  */
	    active_reg_t dummy_low_reg, dummy_high_reg;
	    unsigned char *pdummy;
	    char *sdummy;

	    POP_FAILURE_POINT (sdummy, pdummy,
			       dummy_low_reg, dummy_high_reg,
			       reg_dummy, reg_dummy, reg_info_dummy);
	  }
	  /* Note fall through.  */

	unconditional_jump:
	  /* Note fall through.  */

	/* Unconditionally VRE_JumpTo (without popping any failure points).  */
	case VRE_JumpTo:
	  EXTRACT_NUMBER_AND_INCR (mcnt, p);	/* Get the amount to VRE_JumpTo.  */
	  p += mcnt;				/* Do the VRE_JumpTo.  */
	  break;


	/* We need this opcode so we can detect where alternatives end
	   in `vregexp_GrpMatchNullStrP' et al.  */
	case VRE_JumpToAlt:
	  goto unconditional_jump;


	/* Normally, the VRE_JumpOnFail pushes a failure point, which
	   then gets popped at VRE_PopFailure.  We will end up at
	   VRE_PopFailure, also, and with a pattern of, say, `a+', we
	   are skipping over the VRE_JumpOnFail, so we have to push
	   something meaningless for VRE_PopFailure to pop.  */
	case VRE_DmyFailJump:
	  /* It doesn't matter what we push for the string here.  What
	     the code at `fail' tests is the value for the pattern.  */
	  PUSH_FAILURE_POINT (0, 0, -2);
	  goto unconditional_jump;


	/* At the end of an alternative, we need to push a dummy failure
	   point in case we are followed by a `VRE_PopFailure', because
	   we don't want the failure point for the alternative to be
	   popped.  For example, matching `(a|ab)*' against `aab'
	   requires that we match the `ab' alternative.  */
	case VRE_PushDmyFail:
	  /* See comments just above at `VRE_DmyFailJump' about the
	     two zeroes.  */
	  PUSH_FAILURE_POINT (0, 0, -2);
	  break;

	/* Have to succeed matching what follows at least n times.
	   After that, handle like `VRE_JumpOnFail'.  */
	case VRE_JumpAfterNSuccess:
	  EXTRACT_NUMBER (mcnt, p + 2);

	  /* Originally, this is how many times we HAVE to succeed.  */
	  if (mcnt > 0)
	    {
	       mcnt--;
	       p += 2;
	       STORE_NUMBER_AND_INCR (p, mcnt);
	    }
	  else if (mcnt == 0)
	    {
	      p[2] = (unsigned char) no_op;
	      p[3] = (unsigned char) no_op;
	      goto on_failure;
	    }
	  break;

	case VRE_JumpNTimes:
	  EXTRACT_NUMBER (mcnt, p + 2);

	  /* Originally, this is how many times we CAN VRE_JumpTo.  */
	  if (mcnt)
	    {
	       mcnt--;
	       STORE_NUMBER (p + 2, mcnt);
	       goto unconditional_jump;
	    }
	  /* If don't have to VRE_JumpTo any more, skip over the rest of command.  */
	  else
	    p += 4;
	  break;

	case VRE_SetAddress:
	  {

	    EXTRACT_NUMBER_AND_INCR (mcnt, p);
	    p1 = p + mcnt;
	    EXTRACT_NUMBER_AND_INCR (mcnt, p);
	    STORE_NUMBER (p1, mcnt);
	    break;
	  }

#if 0
	/* The DEC Alpha C compiler 3.x generates incorrect code for the
	   test  WORDCHAR_P (d - 1) != WORDCHAR_P (d)  in the expansion of
	   AT_WORD_BOUNDARY, so this code is disabled.	Expanding the
	   macro and introducing temporary variables works around the bug.  */

	case VRE_MatchWordBnd:
	  if (AT_WORD_BOUNDARY (d))
	    break;
	  goto fail;

	case VRE_MatchNotWordBnd:
	  if (AT_WORD_BOUNDARY (d))
	    goto fail;
	  break;
#else
	case VRE_MatchWordBnd:
	{
	  boolean prevchar, thischar;

	  if (AT_STRINGS_BEG (d) || AT_STRINGS_END (d))
	    break;

	  prevchar = WORDCHAR_P (d - 1);
	  thischar = WORDCHAR_P (d);
	  if (prevchar != thischar)
	    break;
	  goto fail;
	}

      case VRE_MatchNotWordBnd:
	{
	  boolean prevchar, thischar;

	  if (AT_STRINGS_BEG (d) || AT_STRINGS_END (d))
	    goto fail;

	  prevchar = WORDCHAR_P (d - 1);
	  thischar = WORDCHAR_P (d);
	  if (prevchar != thischar)
	    goto fail;
	  break;
	}
#endif

	case VRE_MatchWordBgn:
	  if (WORDCHAR_P (d) && (AT_STRINGS_BEG (d) || !WORDCHAR_P (d - 1)))
	    break;
	  goto fail;

	case VRE_MatchWordEnd:
	  if (!AT_STRINGS_BEG (d) && WORDCHAR_P (d - 1)
	      && (!WORDCHAR_P (d) || AT_STRINGS_END (d)))
	    break;
	  goto fail;

	case VRE_MatchWordChar:
	  PREFETCH ();
	  if (!WORDCHAR_P (d))
	    goto fail;
	  SET_REGS_MATCHED ();
	  d++;
	  break;

	case VRE_MatchNotWordChar:
	  PREFETCH ();
	  if (WORDCHAR_P (d))
	    goto fail;
	  SET_REGS_MATCHED ();
	  d++;
	  break;

	default:
	  abort ();
	}
      continue;  /* Successfully executed one pattern command; keep going.  */


    /* We goto here if a matching operation fails. */
    fail:
      if (!FAIL_STACK_EMPTY ())
	{ /* A restart point is known.	Restore to that state.	*/
	  POP_FAILURE_POINT (d, p,
			     lowest_active_reg, highest_active_reg,
			     regstart, regend, reg_info);

	  /* If this failure point is a dummy, try the next one.  */
	  if (!p)
	    goto fail;

	  /* If we failed to the end of the pattern, don't examine *p.	*/
	  if (p < pend)
	    {
	      boolean is_a_jump_n = FALSE;

	      /* If failed to a backwards VRE_JumpTo that's part of a repetition
		 loop, need to pop this failure point and use the next one.  */
	      switch ((re_opcode_t) *p)
		{
		case VRE_JumpNTimes:
		  is_a_jump_n = TRUE;
		case VRE_MaybePopFailure:
		case VRE_PopFailure:
		case VRE_JumpTo:
		  p1 = p + 1;
		  EXTRACT_NUMBER_AND_INCR (mcnt, p1);
		  p1 += mcnt;

		  if ((is_a_jump_n && (re_opcode_t) *p1 == VRE_JumpAfterNSuccess)
		      || (!is_a_jump_n
			  && (re_opcode_t) *p1 == VRE_JumpOnFail))
		    goto fail;
		  break;
		default:
		  /* do nothing */ ;
		}
	    }

	  if (d >= string1 && d <= end1)
	    dend = end_match_1;
	}
      else
	break;	 /* Matching at this starting point really fails.  */
    } /* for (;;) */

  if (best_regs_set)
    goto restore_best_regs;

  FREE_VARIABLES ();

  return -1;				/* Failure to match.  */
} /* vregexp_Match2 */

/* Subroutine definitions for vregexp_Match2.  */


/* We are passed P pointing to a register number after a VRE_StartMatchMem.

   Return TRUE if the pattern up to the corresponding VRE_StopMatchMem can
   match the empty string, and FALSE otherwise.

   If we find the matching VRE_StopMatchMem, sets P to point to one past its number.
   Otherwise, sets P to an undefined byte less than or equal to END.

   We don't handle duplicates properly (yet).  */

static boolean
vregexp_GrpMatchNullStrP (p, end, reg_info)
    unsigned char **p, *end;
    register_info_type *reg_info;
{
  int mcnt;
  /* Point to after the args to the VRE_StartMatchMem.  */
  unsigned char *p1 = *p + 2;

  while (p1 < end)
    {
      /* Skip over opcodes that can match nothing, and return TRUE or
	 FALSE, as appropriate, when we get to one that can't, or to the
	 matching VRE_StopMatchMem.	*/

      switch ((re_opcode_t) *p1)
	{
	/* Could be either a loop or a series of alternatives.	*/
	case VRE_JumpOnFail:
	  p1++;
	  EXTRACT_NUMBER_AND_INCR (mcnt, p1);

	  /* If the next operation is not a VRE_JumpTo backwards in the
	     pattern.  */

	  if (mcnt >= 0)
	    {
	      /* Go through the on_failure_jumps of the alternatives,
		 seeing if any of the alternatives cannot match nothing.
		 The last alternative starts with only a VRE_JumpTo,
		 whereas the rest start with VRE_JumpOnFail and end
		 with a VRE_JumpTo, e.g., here is the pattern for `a|b|c':

		 /VRE_JumpOnFail/0/6/VRE_ExactN/1/a/VRE_JumpToAlt/0/6
		 /VRE_JumpOnFail/0/6/VRE_ExactN/1/b/VRE_JumpToAlt/0/3
		 /VRE_ExactN/1/c

		 So, we have to first go through the first (n-1)
		 alternatives and then deal with the last one separately.  */


	      /* Deal with the first (n-1) alternatives, which start
		 with an VRE_JumpOnFail (see above) that jumps to right
		 past a VRE_JumpToAlt.	*/

	      while ((re_opcode_t) p1[mcnt-3] == VRE_JumpToAlt)
		{
		  /* `mcnt' holds how many bytes long the alternative
		     is, including the ending `VRE_JumpToAlt' and
		     its number.  */

		  if (!vregexp_AltMatchNullStrP (p1, p1 + mcnt - 3,
						      reg_info))
		    return FALSE;

		  /* Move to right after this alternative, including the
		     VRE_JumpToAlt.  */
		  p1 += mcnt;

		  /* Break if it's the beginning of an n-th alternative
		     that doesn't begin with an VRE_JumpOnFail.  */
		  if ((re_opcode_t) *p1 != VRE_JumpOnFail)
		    break;

		  /* Still have to check that it's not an n-th
		     alternative that starts with an VRE_JumpOnFail.  */
		  p1++;
		  EXTRACT_NUMBER_AND_INCR (mcnt, p1);
		  if ((re_opcode_t) p1[mcnt-3] != VRE_JumpToAlt)
		    {
		      /* Get to the beginning of the n-th alternative.	*/
		      p1 -= 3;
		      break;
		    }
		}

	      /* Deal with the last alternative: go back and get number
		 of the `VRE_JumpToAlt' just before it.  `mcnt' contains
		 the length of the alternative.  */
	      EXTRACT_NUMBER (mcnt, p1 - 2);

	      if (!vregexp_AltMatchNullStrP (p1, p1 + mcnt, reg_info))
		return FALSE;

	      p1 += mcnt;	/* Get past the n-th alternative.  */
	    } /* if mcnt > 0 */
	  break;


	case VRE_StopMatchMem:
	  *p = p1 + 2;
	  return TRUE;


	default:
	  if (!vregexp_CommOpMatchNullStrP (&p1, end, reg_info))
	    return FALSE;
	}
    } /* while p1 < end */

  return FALSE;
} /* vregexp_GrpMatchNullStrP */


/* Similar to vregexp_GrpMatchNullStrP, but doesn't deal with alternatives:
   It expects P to be the first byte of a single alternative and END one
   byte past the last. The alternative can contain groups.  */

static boolean
vregexp_AltMatchNullStrP (p, end, reg_info)
    unsigned char *p, *end;
    register_info_type *reg_info;
{
  int mcnt;
  unsigned char *p1 = p;

  while (p1 < end)
    {
      /* Skip over opcodes that can match nothing, and break when we get
	 to one that can't.  */

      switch ((re_opcode_t) *p1)
	{
	/* It's a loop.  */
	case VRE_JumpOnFail:
	  p1++;
	  EXTRACT_NUMBER_AND_INCR (mcnt, p1);
	  p1 += mcnt;
	  break;

	default:
	  if (!vregexp_CommOpMatchNullStrP (&p1, end, reg_info))
	    return FALSE;
	}
    }  /* while p1 < end */

  return TRUE;
} /* vregexp_AltMatchNullStrP */


/* Deals with the ops common to vregexp_GrpMatchNullStrP and
   vregexp_AltMatchNullStrP.

   Sets P to one after the op and its arguments, if any.  */

static boolean
vregexp_CommOpMatchNullStrP (p, end, reg_info)
    unsigned char **p, *end;
    register_info_type *reg_info;
{
  int mcnt;
  boolean ret;
  int reg_no;
  unsigned char *p1 = *p;

	 switch ((re_opcode_t) *p1++)
	  {
	    default:		/* All other opcodes mean we cannot match the empty string.  */
	      return FALSE;
	    case no_op:
	    case VRE_MatchBOL:
	    case VRE_MatchEOL:
	    case VRE_MatchBOS:
	    case VRE_MatchEOS:
	    case VRE_MatchWordBgn:
	    case VRE_MatchWordEnd:
	    case VRE_MatchWordBnd:
	    case VRE_MatchNotWordBnd:	break;
	    case VRE_StartMatchMem:
	      reg_no = *p1; ret = vregexp_GrpMatchNullStrP (&p1, end, reg_info);
	      if (REG_MATCH_NULL_STRING_P (reg_info[reg_no]) == MATCH_NULL_UNSET_VALUE)	REG_MATCH_NULL_STRING_P (reg_info[reg_no]) = ret;
	      if (!ret)	return(FALSE) ;
	      break;
	    case VRE_JumpTo:	/* If this is an optimized VRE_JumpAfterNSuccess for zero times, make the VRE_JumpTo.  */
	      EXTRACT_NUMBER_AND_INCR (mcnt, p1);
	      if (mcnt >= 0) p1 += mcnt; else return FALSE;
	      break;
	    case VRE_JumpAfterNSuccess:      /* Get to the number of times to succeed.  */
	      p1 += 2;  EXTRACT_NUMBER_AND_INCR (mcnt, p1);
	      if (mcnt == 0) { p1 -= 4; EXTRACT_NUMBER_AND_INCR (mcnt, p1); p1 += mcnt; }
	       else return FALSE;
	      break;
	    case VRE_MatchDupInReg:
	      if (!REG_MATCH_NULL_STRING_P (reg_info[*p1])) return FALSE;
	      break;
	    case VRE_SetAddress:
	      p1 += 4; return(FALSE) ;
	  }
	*p = p1;
	return TRUE;
} /* vregexp_CommOpMatchNullStrP */


/* Return zero if TRANSLATE[S1] and TRANSLATE[S2] are identical for LEN
   bytes; nonzero otherwise.  */

static int vregexp_BCmpTran (s1, s2, len, translate)
     char *s1, *s2;
     register int len;
     RE_TRANSLATE_TYPE translate;
{
  unsigned char *p1 = (unsigned char *) s1;
  unsigned char *p2 = (unsigned char *) s2;

	for(;len>0;len--) { if (translate[*p1++] != translate[*p2++]) return(1) ; } ;
	return(0) ;
}

/* Entry points for GNU code.  */

/* vregexp_CompilePattern is the GNU regular expression compiler: it
   compiles PATTERN (of length SIZE) and puts the result in BUFP.
   Returns 0 if the pattern was valid, otherwise an error string.

   Assumes the `allocated' (and perhaps `buffer') and `translate' fields
   are set in BUFP on entry.

   We call vregexp_Compile to do the actual compilation.  */

char *
vregexp_CompilePattern (pattern, length, bufp)
     char *pattern;
     size_t length;
     struct re_pattern_buffer *bufp;
{
  reg_errcode_t ret;
  /* GNU code is written to assume at least RE_NREGS registers will be set
     (and at least one extra will be -1).  */
  bufp->regs_allocated = REGS_UNALLOCATED;
  /* And GNU code determines whether or not to get register information
     by passing null for the REGS argument to vregexp_Match, etc., not by
     setting no_sub.  */
  bufp->no_sub = 0;
  /* Match anchors at newline.	*/
  bufp->newline_anchor = 1;
  ret = vregexp_Compile (pattern, length, re_syntax_options, bufp);
  if (!ret) return NULL;
  return gettext (re_error_msgid[(int) ret]);
}

/* Entry points compatible with 4.2 BSD regex library.	We don't define
   them unless specifically requested.	*/

#if defined (_REGEX_RE_COMP) || defined (_LIBC)

/* BSD has one and only one pattern buffer.  */
static struct re_pattern_buffer re_comp_buf;

char *
#ifdef _LIBC
/* Make these definitions weak in libc, so POSIX programs can redefine
   these names if they don't use our functions, and still use
   vregexp_RegComp/vregexp_RegExec below without link errors.  */
weak_function
#endif
vregexp_Comp (s)
    char *s;
{
  reg_errcode_t ret;

  if (!s)
    {
      if (!re_comp_buf.buffer)
	return gettext ("No previous regular expression");
      return 0;
    }

  if (!re_comp_buf.buffer)
    {
      re_comp_buf.buffer = (unsigned char *) malloc (200);
      if (re_comp_buf.buffer == NULL)
	return gettext (re_error_msgid[(int) REG_ESPACE]);
      re_comp_buf.allocated = 200;

      re_comp_buf.fastmap = (char *) malloc (1 << BYTEWIDTH);
      if (re_comp_buf.fastmap == NULL)
	return gettext (re_error_msgid[(int) REG_ESPACE]);
    }

  /* Since `vregexp_Exec' always passes NULL for the `regs' argument, we
     don't need to initialize the pattern buffer fields which affect it.  */

  /* Match anchors at newlines.  */
  re_comp_buf.newline_anchor = 1;

  ret = vregexp_Compile (s, strlen (s), re_syntax_options, &re_comp_buf);

  if (!ret) return NULL;

  /* Yes, we're discarding `const' here if !HAVE_LIBINTL.  */
  return (char *) gettext (re_error_msgid[(int) ret]);
}


int
#ifdef _LIBC
weak_function
#endif
vregexp_Exec (s)
    char *s;
{
  const int len = strlen (s);
  return
    0 <= vregexp_Search (&re_comp_buf, s, len, 0, len, (struct re_registers *) 0);
}

#endif /* _REGEX_RE_COMP */

/* POSIX.2 functions.  Don't define these for Emacs.  */


/* vregexp_RegComp takes a regular expression as a string and compiles it.

   PREG is a regex_t *.  We do not expect any fields to be initialized,
   since POSIX says we shouldn't.  Thus, we set

     `buffer' to the compiled pattern;
     `used' to the length of the compiled pattern;
     `syntax' to RE_SYNTAX_POSIX_EXTENDED if the
       REG_EXTENDED bit in CFLAGS is set; otherwise, to
       RE_SYNTAX_POSIX_BASIC;
     `newline_anchor' to REG_NEWLINE being set in CFLAGS;
     `fastmap' and `fastmap_accurate' to zero;
     `re_nsub' to the number of subexpressions in PATTERN.

   PATTERN is the address of the pattern string.

   CFLAGS is a series of bits which affect compilation.

     If REG_EXTENDED is set, we use POSIX extended syntax; otherwise, we
     use POSIX basic syntax.

     If REG_NEWLINE is set, then . and [^...] don't match newline.
     Also, vregexp_RegExec will try a match beginning after every newline.

     If REG_ICASE is set, then we considers upper- and lowercase
     versions of letters to be equivalent when matching.

     If REG_NOSUB is set, then when PREG is passed to vregexp_RegExec, that
     routine will report only success or failure, and nothing about the
     registers.

   It returns 0 if it succeeds, nonzero if it doesn't.	(See regex.h for
   the return codes and their meanings.)  */

int
vregexp_RegComp (preg, pattern, cflags)
    regex_t *preg;
    char *pattern;
    int cflags;
{
  reg_errcode_t ret;
  reg_syntax_t syntax
    = (cflags & REG_EXTENDED) ?
      RE_SYNTAX_POSIX_EXTENDED : RE_SYNTAX_POSIX_BASIC;

  /* vregexp_Compile will allocate the space for the compiled pattern.	*/
  preg->buffer = 0;
  preg->allocated = 0;
  preg->used = 0;

  /* Don't bother to use a fastmap when searching.  This simplifies the
     REG_NEWLINE case: if we used a fastmap, we'd have to put all the
     characters after newlines into the fastmap.  This way, we just try
     every character.  */
  preg->fastmap = 0;

  if (cflags & REG_ICASE)
    {
      unsigned i;

      preg->translate
	= (RE_TRANSLATE_TYPE) malloc (V4REG_NumChars
				      * sizeof (*(RE_TRANSLATE_TYPE)0));
      if (preg->translate == NULL)
	return (int) REG_ESPACE;

      /* Map uppercase characters to corresponding lowercase ones.  */
      for (i = 0; i < V4REG_NumChars; i++)
	preg->translate[i] = ISUPPER (i) ? tolower (i) : i;
    }
  else
    preg->translate = NULL;

  /* If REG_NEWLINE is set, newlines are treated differently.  */
  if (cflags & REG_NEWLINE)
    { /* REG_NEWLINE implies neither . nor [^...] match newline.  */
      syntax &= ~RE_DOT_NEWLINE;
      syntax |= RE_HAT_LISTS_NOT_NEWLINE;
      /* It also changes the matching behavior.  */
      preg->newline_anchor = 1;
    }
  else
    preg->newline_anchor = 0;

  preg->no_sub = !!(cflags & REG_NOSUB);

  /* POSIX says a null character in the pattern terminates it, so we
     can use strlen here in compiling the pattern.  */
  ret = vregexp_Compile (pattern, strlen (pattern), syntax, preg);

  /* POSIX doesn't distinguish between an unmatched open-group and an
     unmatched close-group: both are REG_EPAREN.  */
  if (ret == REG_ERPAREN) ret = REG_EPAREN;

  return (int) ret;
}


/* vregexp_RegExec searches for a given pattern, specified by PREG, in the
   string STRING.

   If NMATCH is zero or REG_NOSUB was set in the cflags argument to
   `vregexp_RegComp', we ignore PMATCH.  Otherwise, we assume PMATCH has at
   least NMATCH elements, and we set them to the offsets of the
   corresponding matched substrings.

   EFLAGS specifies `execution flags' which affect matching: if
   REG_NOTBOL is set, then ^ does not match at the beginning of the
   string; if REG_NOTEOL is set, then $ does not match at the end.

   We return 0 if we find a match and REG_NOMATCH if not.  */

int vregexp_RegExec (preg, string, nmatch, pmatch, eflags)
    regex_t *preg;
    char *string;
    size_t nmatch;
    regmatch_t pmatch[];
    int eflags;
{
  int ret;
  struct re_registers regs;
  regex_t private_preg;
  int len = strlen (string);
  boolean want_reg_info = !preg->no_sub && nmatch > 0;

  private_preg = *preg;

  private_preg.not_bol = !!(eflags & REG_NOTBOL);
  private_preg.not_eol = !!(eflags & REG_NOTEOL);

  /* The user has told us exactly how many registers to return
     information about, via `nmatch'.  We have to pass that on to the
     matching routines.  */
  private_preg.regs_allocated = REGS_FIXED;

  if (want_reg_info)
    {
      regs.num_regs = nmatch;
      regs.start = TALLOC (nmatch, regoff_t);
      regs.end = TALLOC (nmatch, regoff_t);
      if (regs.start == NULL || regs.end == NULL)
	return (int) REG_NOMATCH;
    }

  /* Perform the searching operation.  */
  ret = vregexp_Search (&private_preg, string, len,
		   /* start: */ 0, /* range: */ len,
		   want_reg_info ? &regs : (struct re_registers *) 0);

  /* Copy the register information to the POSIX structure.  */
  if (want_reg_info)
    {
      if (ret >= 0)
	{
	  unsigned r;

	  for (r = 0; r < nmatch; r++)
	    {
	      pmatch[r].rm_so = regs.start[r];
	      pmatch[r].rm_eo = regs.end[r];
	    }
	}

      /* If we needed the temporary register info, free the space now.	*/
      free (regs.start);
      free (regs.end);
    }

  /* We want zero return to mean success, unlike `vregexp_Search'.  */
  return ret >= 0 ? (int) REG_NOERROR : (int) REG_NOMATCH;
}


/* Returns a message corresponding to an error code, ERRCODE, returned
   from either vregexp_RegComp or vregexp_RegExec.   We don't use PREG here.  */

size_t vregexp_Error (errcde, preg, errbuf, errbuf_size)
    int errcde;
    regex_t *preg;
    UCCHAR *errbuf;
    size_t errbuf_size;
{ char *msg;
  size_t msg_size;

	msg = gettext (re_error_msgid[errcde]);
	msg_size = strlen (msg) + 1;		 /* Includes the null.  */
	if (errbuf_size != 0)
	 { if (msg_size > errbuf_size)
	    { int i ; for(i=0;i<errbuf_size-1;i++) { errbuf[i] = msg[i] ; } ; errbuf[errbuf_size - 1] = 0;
//	      strncpy (errbuf, msg, errbuf_size - 1); errbuf[errbuf_size - 1] = 0;
	    } else { UCstrcpyAtoU (errbuf, msg); } ;
	 } ;
	return(msg_size) ;
}


/*	vregexp_Free - Free dynamically allocated space used by PREG.  */

void vregexp_Free(preg)
  regex_t *preg;
{
	if (preg->buffer != NULL) free (preg->buffer);
	preg->buffer = NULL; preg->allocated = 0; preg->used = 0;
	if (preg->fastmap != NULL) free (preg->fastmap);
	preg->fastmap = NULL; preg->fastmap_accurate = 0;
	if (preg->translate != NULL) free (preg->translate);
	preg->translate = NULL;
}
