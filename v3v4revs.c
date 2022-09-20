/*	V3V4Revs.c - Lists Revision History

File		Ver	Who	When		What
--------------	-----	------	------------	-----------------------------------------------------------------------------------
v4extern.c	1.683	VEH	9/20/22		v4mysql_Xct() - retry on errors 1205, 1206 & 1223 (deadlock/lock table full/conflicting read lock)
v4extern.c	1.683	VEH	9/6/22		HTTP() return data - if Text? then check for UTF-8 encoding
v4imu.c		1.683	VEH	8/31/22		Str(xxx Has::yyy) - throw error if yyy is empty or if yyy is list and ALL elements in list are empty
vcommon.c	1.682	VEH	8/23/22		Fixed problems handling +nnn on telephone parsing
v4imu.c		1.681	VEH	8/17/22		Added Rpt(Default::n) to pass onto xv4rpp (_DF) for default formatting of HTML reports
v4extern.c	1.680	VEH	8/4/22		Fixed bug in HTTP(Header::list) handling
v4dpi.c		1.679	VEH	8/2/22		$(x.dim1) returns UV4:null if dim1:null BUT $$(x.dim1) returns error
vcommon.c	1.678	VEH	7/11/22		Assume date is yyyy-mm-dd if yyyy part is > 1000 even if default order is mdy
v4dpi.c		1.677	VEH	6/30/22		Changes V4DPI_SYMDEFVAL_MAX from 10007 to 7001 & added hash conflict handling lcl__SymDefValues updating
v4dpi.c		1.676	VEH	6/28/22		Increased V4DPI_SYMDEFVAL_MAX from 3001 to 10007
v4extern.c	1.675	VEH	6/23/22		HTTP(Header::dim) works even if Create/Append tags also used
vcommon.c	1.674	VEH	5/31/22		Add v4xdb_FreeStuff() in v4mm_FreeResources() to close all external database connections
v4dpi.c		1.674	VEH	5/24/22		$() - now allow $(x.y*) where Dim:y is external dictionary entry
v4imu.c		1.673	VEH	5/8/22		Rpt() now sends new code: LA<country-code> to v4rpp
v4Extern.c	1.672	VEH	5/04/22		HTTP(Retry:n) added
v4extern.c	1.671	VEH	4/20/22		HTTP() - header buffers too small - extended to 2048 bytes
vcommon.c	1.670	VEH	3/7/22		String literal - add '\xnn' to handle 2-digit hex number
v4imu.c		1.670	VEH	3/7/22		Parse() - added As::ASCII to convert special characters to regular ASCII letters
v4dpi.c		1.669	VEH	2/22/22		Tweak to JSON() parser to handled unexpected end-of-string
v4extern.c	1.668	VEH	12/29/21	OSFile() added Hash::xxx (similar to Hash::xxx in Str())
v4imu.c		1.668	VEH	11/16/21	Added Array([x] Release?)
vcommon		1.668	VEH	11/11/21	Tweaks to phone number parsing
v4defs.c	1.668	VEH	9/30/21		Increased V4DPI_EvalList_AreaMax from 50 to 100
v4extern.c	1.668	VEH	9/29/21		v4mysql_Fetch() - test for non-EOF but error return from mysql_stmt_fetch()
v4extern.c	1.668	VEH	9/1/21		HTTP() - explicitly set CURLOPT_POSTFIELDSIZE = 0 if no POST data (to keep it from hanging)
v4extern.c	1.667	VEH	7/28/21		Added SendMail(CC:xxx)
v4dpi.c		1.666	VEH	7/20/21		mySQL->V4 - convert 8 byte integer to dict/xdict
v4dpi.c		1.666	VEH	6/1/21		Enforce If() has only Then/Else/ElseIf tags after 1st argument
v4imu.c		1.666	VEH	5/31/21		Added: Parse(As::JSON) and JSON(Parse::list)
vcommon.c	1.666	VEH	5/25/21		UDate parsing - if yy then assume 2000+yy unless Dim is History in which case try 1900 if 2000 past current date
v4extern.c	1.665	VEH	5/19/21		Added Bearer::string tag to HTTP()
v4im.c		1.664	VEH	4/20/21		Issue with If(xxx @yyy @zzz) with @yyy being evaluated to see if it is Then (w/o "::")
v4imu.c		1.663	VEH	1/19/21		JSON(Name::xxx UV4:Null) returns 'xxx:null'
v4imu.c		1.662	VEH	12/30/20	Fixed MakeP(Dim:x Special::NE y) where Dim:x was color/external/country
v4extern.c	1.661	VEH	10/20/20	Rewrite curlWriteHeader to get around long-running bug
v4imu.c		1.660	VEH	10/12/20	Format("xxxx" RDB:mysql) - change UTF16 singe/double quotes & apostrophe to ASCII equiv.
v4imu.c		1.659	VEH	9/10/20		Format("xxxx" RDB::mysql)- change 127 limit to 255 for characters
v4im.c		1.658	VEH	9/3/20		If(xxx Then xxx) - generate error (s/b Then::xxx)
v4defs.c	1.657	VEH	8/24/20		Problem in expanding max number of Trees - directory size exceeded max V4IS bucket max
v4defs.c	1.656	VEH	8/11/20		Bug in DTINfo(UDT:xxx Day:xx), doubled number of Tree()s, flipped to 64 bit
v4dpi.c		1.655	VEH	7/21/20		JSON(Parse::xxx) - handle "xxx:[  ]" (array with nothing but spaces in it)
v4*.c		1.654	VEH	6/1/20		Added MSSQL support (mostly in Format(xxx RDB::MSSQL))
vcommon.c	1.653	VEH	5/21/20		vout_UCTextFileXCont - don't backup unless last character is UCEOS
v4im.c		1.652	VEH	4/13/20		Added IsAll() - checks for all (much like Isa())
v4dpi.c		1.651	VEH	4/8/20		JSON(Parse::xxx) where xxx starts with "top" (quoted, instead of just top)
v4defs.c	1.650	VEH	2/7/20		Moved XDB from di->ds.XDB to di->XDB (union was screwing di->ds.INT.IFormat via memory sharing)
vcommon.c	1.649	VEH	1/09/20		Token parser - if paring JSON then auto-convert long fixed point to floating point
v4imu.c		1.648	VEH	1/02/20		Bug in JSON parse with tcb buffer release
v4defs.c	1.647	VEH	12/19/19	DIMVAL() now checks for dim == 0
v4imu.c		1.646	VEH	12/6/19		Str(xxx Split::yyy) - elements of list converted to BigTxt as necessary
v4imu.c		1.645	VEH	10/31/19	List(() Key::xx) - if key:xxx is string then do UCstrcmp() instead of memcmp()
v4extern.c	1.645	VEH	10/30/19	Added HTTP(Put? / Delete?) options and allow for redirects
v4dpi.c		1.644	VEH	10/1/19		JSON(Parse::xxx) - maintain case of elements
v4imu.c		1.644	VEH	9/26/19		Added List(list KeyNum::key)
v4dpi.c		1.644	VEH	9/25/19		Added $xxx+=xxx and $xxx-=xxxx
vcommon.c	1.643	VEH	9/4/19		Added v_Soundex2 to duplicate mySQL SOUNDEX function
v4imu.c		1.643	VEH	9/4/19		Added Str("xxxxx" Soundex::{Alpha,mySQL,Numeric})
v4imu.c		1.642	VEH	8/6/19		Added JSON(Function::name function) - function not enclosed in quotes
v4imu.c		1.641	VEH	8/1/19		Fixed JSON(number) where number is not single value - enclose in quotes
v4imu.c		1.640	VEH	4/18/19		Added JSON(Attributes:logical) to handle XML tag attributes
v4extern.c	1.639	VEH	4/5/19		Changed handling of SendMail with libCurl to handle html+text+attachments
v4imu.c		1.638	VEH	3/22/19		Commented out code to check max length of Rpt(Title::xxx)
v4extern.c	1.637	VEH	12/21/18	Added v4im_MessageSendQueue() & v4im_MessageSendFlushQueue() to improve webSocket response
v4imu.c		1.637	VEH	12/14/18	Added Minute? & Second? to DTInfo()
v4websocket.js	1.637	VEH	12/5/18		Created v4WebSocket.js to handle webSocket communications between V4 web process & web client
v4extern.c	1.637	VEH	12/5/18		Extended v4im_MessageSend() to handle alternal calling arguments
vcommon.c	1.637	VEH	12/5/18		Added new input mode to tcb - from webSocket.js process
v4imu.c		1.637	VEH	12/5/18		Added Dbg(Web:(id url port seskey)) to handle V4 debugging from web
vcommon.c	1.636	VEH	10/16/18	Added routine (vjson_SubParamsInJSON) to format AJAX result strings with additional info
v4dpi.c		1.635	VEH	9/12/18		Added data conversions from integer -> bigtext
v4Extern.c	1.635	VEH	9/10/18		Changed sendmail() to use application/pdf MIME for pdf attachments
v4dpi.c		1.635	VEH	9/5/18		Changed V4DPI_SYMDEFVAL_MAX to 3001
v4test.c	1.634	VEH	8/16/18		Properly increment error count in v4_UCError()
v4extern.c	1.633	VEH	6/25/18		EchoE updates gpi->ErrCount
v4defs.c	1.633	VEH	6/15/18		Add point-type TREE and UTree to v4kernel.v4i. Replaced intPNTv() with treePNTv() where appropriate.
v4defs.c	1.633	VEH	6/14/18		Increased Tree() limit to 0x800
v4imu.c		1.632	VEH	6/7/18		Error(Warning?) - returns (# message), Error(Warning::n) - sets warning count to n
v4imu.c		1.631	VEH	3/15/18		Added Format(xxx CSV?)
		1.630	VEH	1/5/18		Moved to Visual Studio 2017
v4imu.c		1.629	VEH	10/20/17	Drawer(xxx ListOf?) not setting correct size for lp->Bytes (misplaced ())
v4imu.c		1.628	VEH	10/10/17	Changed EvalCmd() to increase tcb buffer to V4LEX_Tkn_SrcFileLineMax, also increased V4LEX_Tkn_SrcFileLineMax to 1MB
vcommon.c	1.627	VEH	9/21/17		Fixed next-token to properly flip from int to double on large numeric literals
v4extern.c	1.626	VEH	9/13/17		Added HTTP(Header::(name value))
v4imu.c		1.625	VEH	8/25/17		Changed Fail() (no arguments) so that it fails, previously returned with last error. Should use Fail(Message?) instead.
v4imu.c		1.624	VEH	7/6/17		Fixed bug with SSDim() colors having hex leading 0s  (ex: green & blue)
v4defs.c	1.623	VEH	5/11/17		Increased number of report columns from 256 to 512
v4extern.c	1.622	VEH	4/19/17		Add wait-and-retry to mysql DBXct (to handle 1213 errors (deadlock)
v4imu.c		1.621	VEH	2/22/17		Fixed bug in JSON() with handling multiple arguments some of which are lists
v4imu.c		1.620	VEH	1/31/17		Took out YC conversion code to convert 1/1/2001 -> NULL in Format(xx RDB::mySQL)
v4defs.c	1.619	VEH	1/20/17		Added V4DPI_UCVAL_MaxSafe and replaced multiple references to (V4DPI_UCVal_Max - xxx) with single value
v4imu.c		1.618	VEH	12/27/16	Fixed bug in JSON() when result close to BigText size cutover
v4dpi.c		1.617	VEH	12/2/16		Fixed bug in v4dpi_PointToStringML() that allowed LIST point to exceed buffer max
v4extern.c	1.616	VEH	11/21/16	Added call to v4l_ListClose() in Tally() if it failed (causing very-hard-to-find bug in dbx routines with dimhash filling up)
v4dpi.c		1.615	VEH	11/15/16	Added update to internal JSON structures: $(x.y.z=value)
v4imu.c		1.614	VEH	11/10/16	Added HMAC to MD5 hashing - Str("string" Key::"xxx" Hash::MD5)
v4extern.c	1.613	VEH	10/19/16	Fixed problem in Message(Host:xxx Listen?) where Host:xxx being ignored
v4imu.c		1.613	VEH	10/18/16	Added Unicode() module
v4extern.c	1.612	VEH	9/22/16		Added OSInfo(Directory::newWorkingDirectory)
v4extern.c	1.611	VEH	9/12/16		Think I finally fixed bug in HTTP() that caused random memory error
v4dpi.c		1.610	VEH	7/14/16		Fixed bug in v4dpi_AcceptValue() with string very close to V4DPI_UCVal_Max
v4extern.c	1.609	VEH	7/6/16		Moved HTTP reply length check so we don't fail on long replies going to Output stream
v4dpi.c		1.608	VEH	6/15/16		Fixed bug: time:="nn:nn" - to return point on time instead of Dim:Num
v4imu.c		1.608	VEH	6/9/16		Added special check so Format(Date:1/1/2001 RDB::mysql) returns null
vcommon.c	1.608	VEH	6/1/16		Fixed bug when parsing JSON string literal that does not have ending quote
v4imu.c		1.607	VEH	5/20/16		Fixed bug in Tally(... While:xxx) - not closing list handler which caused mySQL sequence error
v4imu.c		1.606	VEH	4/26/16		Modified Drawers & Drawers -> Lists to handle 100K+ entries in drawer
v4dpi.c		1.605	VEH	4/21/16		Added conversion from string to fixed in v4dpi_PointAccept()
v4extern.c	1.604	VEH	4/11/16		Had to add CURLOPT(CURLOPT_UPLOAD,1) to get SendMail() to work with new CURL libraries
v4extern.c	1.604	VEH	4/7/16		Added HTTP(Wait:seconds) option
---		1.604	VEH	4/6/16		Build with new curl library
v4Extern.c	1.603	VEH	3/29/16		Added Output(file IC::logical) to allow for case sensitive file name creation
vcommon.c	1.603	VEH	3/28/16		Increased size of input buffers to read BIG JSON inputs
v4eval.c	1.602	VEH	2/19/16		Added new Dim:Alpha/BigText flag- JSON. Context ADV dim:xxxx - immediately parses xxxx as JSON string. Can handle very long strings.
v4imu.c		1.601	VEH	2/18/16		Added Num(xxx Round::{up,down})
v4imu.c		1.601	VEH	2/15/16		Coerce - added UDelta point type (same as Int)
v4eval.c	1.601	VEH	2/10/16		Changed handling of Context ADV with AsIs to check for '<' and convert to "&lt;"
v4imu.c		1.600	VEH	1/26/16		Fixed bug in Str(xxx Replace::xx With::xx) - handle IC? properly
v4imu.c		1.599	VEH	12/17/15	Fixed bug in MakeP(Tag:xxxx value) - trying to UNQUOTE non-quoted point
v4extern.c	1.598	VEH	11/23/15	Changed mysql_stmt_fetch() to not terminate on MYSQL_DATA_TRUNCATED result code
v4defs.c	1.598	VEH	11/17/15	Increased number of compile-time trees to 1024
v4eval.c	1.597	VEH	11/2/15		Flipped "Point external-dict-entry" from UCkeyword to UCstring so that upper/lower case preserved
v4imu.c		1.596	VEH	10/16/15	Fixed problem with nested Enum(xxx End::()) so that the nested End::xxx does not trigger footer - only the top level one
v4extern.c	1.596	VEH	10/9/15		Added HTTP(DateTime::xxx) to create Date: header line
v4imu.c		1.596	VEH	10/8/15		Added Str(xxxx Key::xxx Hash::SHA1) - to handle SHA1 hashing and HMAC-SHA1 hashing
v4extern.c	1.595	VEH	10/2/15		Added OSFile(xxx Move:::toFile) so that toFile is overwritten on 3-colon version
v4imu.c		1.595	VEH	9/1/15		Fixed MakePm() - was returning all alpha points on Dim:Alpha
v4imu.c		1.595	VEH	8/21/15		Fixed MakePm() - was returning 'range' point (s/b single) with MakePm(Dim:xxx value UV4:none)
v4dpi.c		1.595	VEH	8/21/15		Fixed problem in reading UPeriod data back from database
v4ctx.c		1.595	VEH	8/20/15		Fixed problem in parsing fractional floating point numbers in JSON string
v4eval.c	1.595	VEH	8/12/15		Put code in Context ADV to auto-flip Alpha to BigText if string exceeds Alpha/UCChar max
v4defs.c	1.594	VEH	8/6/15		Increased V4LEX_Tkn_ParamMax from 255 to 1024
v4imu.c		1.593	VEH	7/27/15		Added &apos; and &quot; to Str(xxx HTML?) conversion
v4imu.c		1.593	VEH	7/13/15		Changed Rpt(Source::) to Rpt(Libary::url) and hooked it up with xv4rpp
v4common.c	1.592	VEH	7/8/15		Fixed bug when Alpha table value exceeded max alpha length
v4dpi.c		1.592	VEH	7/5/15		Added UPeriod support to v4dpi_AcceptValue()
v4extern.c	1.591	VEH	6/29/15		Added extra checks to test for vout_GetOutputBuffer() returning NULL
v4eval.c	1.590	VEH	6/5/15		Modified handling of Ajax errors - enclose error message in double quotes per JSON specs
v4extern.c	1.590	VEH	5/26/15		Fixed bug in Echo(Buffer::xxx) that was null-terminating lines in buffer
v4imu.c		1.590	VEH	5/21/15		Added Rpt(Scale::{num,FIT})
v4imu.c		1.589	VEH	4/30/15		Added JSON(Quoted::logical) and JSON(Quoted?) to control forced quoting of name in name:value pair
v4imu.c		1.589	VEH	4/27/15		Added Dimension xxx type DisplayerTrace and [UV4:displayerTrace xxx...] to handle output of dimension in traces and errors  (replaces DisplayCTX)
v4imu.c		1.589	VEH	4/27/15		Fixed realloc() in projections - apparently you can't realloc off of stack space any more
v4imu.c		1.589	VEH	4/24/15		Changed regexp handling in Str() so that IC? ignores case (used to be UC?)
v4extern.c	1.588	VEH	4/9/15		Fixed problem in Zip() when openning non-zip files
v4eval.c	1.588	VEH	3/12/15		Added warning if INCLUDE file is empty
vcommon.c	1.588	VEH	3/10/15		Fixed problems with showing filename & linenumber on some TABLE parsing errors
vcommon.c	1.587	VEH	2/27/15		Reworked v4lex_NestInput() to clear out entire ilvl[] block instead of selectively resetting elements
v4imu.c		1.586	VEH	2/24/15		Fixed problems with Str(xxx Start::xx End::xx) when Start index greater than length of string
v4imu.c		1.586	VEH	2/16/15		Fixed several places where GETSPINLOCK used improperly
v4imu.c		1.585	VEH	2/12/15		Cleaned up MakePm() - removed tags, added support for compound source points
v4eval.c	1.584	VEH	2/3/15		Named stream (UV4:EvalStream) on 'Eval [xxx] > yyy' so it may be referenced later
v4imu.c		1.583	VEH	12/1/14		Added Str(xxx ListOf::Integer) - returns integer list corresponding to ASCII/Unicode characters in string
v4imu.c		1.583	VEH	12/1/14		Added Str(xxx ListOf::Characters) - returns list of characters in the string xxx
v4eval.c	1.583	VEH	11/24/14	Fixed minor issue with output of error when returning AJAX/JSON
v4imu.c		1.583	VEH	11/19/14	Tweaked XDBSAVECURROW macro, added XDBSAVEROW macro to EnumCL so xdb points are saved
v4imu.c		1.582	VEH	11/12/14	Added Bits(Else::xxx) to make handling of Result:: tag a little cleaner
v4extern.c	1.582	VEH	11/6/14		Added xdb->con[].isActive and DBConnect(Id::xxx Active?) to track/determine if connection is active
v4imu.c		1.582	VEH	11/6/14		Added Area(xxx Retry:n)
v4eval.c	1.582	VEH	11/6/14		Added support to retry in 'Area Read xxx' command
v4imu.c		1.582	VEH	10/23/14	Added Error(ListOf::{CONTEXT,STACK}), also added gpi->ved to v4extern for support
v4isu.c		1.581	VEH	10/20/14	Added FILE_SHARE_DELETE to area open on read (so that another process can rename even if open)
v4imu.c		1.580	VEH	10/14/14	Fixed bug in MakePm() - return as single point if only one value argument
vcommon.c	1.580	VEH	10/2/14		Fixed bug in auto-seed for vRan64_RandomU64() - bit-rotate not correct with unsigned b64 ints
v4imu.c		1.580	VEH	9/24/14		Fixed bug in XML->JSON parser when XML is mal-formed
v4extern.c	1.580	VEH	9/23/14		Added Timer(Id::xx, Context::xx, At::xx, Remove?, ListOf?, Value::xxx)
v4imu.c		1.579	VEH	9/18/14		Added Drawer(xxx Filter::@isct) - filters out points and returns as list
v4imu.c		1.578	VEH	8/21/14		Added DTInfo(POSIX::nnn) - converts system date-time (nnn) to UDT format
v4imu.c		1.578	VEH	8/21/14		Added JSON(Parse::buffer), JSON(Top:xxx Parse:yyy) - takes top-level name of xxx
vcommon.c	1.578	VEH	8/4/14		Fixed v4lex_NextCSVField() to properly handle columns with embedded newline characters
v4dpi.c		1.577	VEH	7/31/14		Fixed date parser so that EvalPt("UDate:") returns UDate:none
v4eval.c	1.577	VEH	7/31/14		Added AsIs attribute to BIGTEXT points - Context ADV bt:xxxx - literal xxxx taken as-is (use for passing JSON string)
v4im.c		1.576	VEH	7/23/14		Fixed problem in MT() - not returning TRUE for 'none' values
vcommon.c	1.576	VEH	7/17/14		Fixed problem in getnameinfo() routine with Windows 2003 and prior
v4imu.c		1.575	VEH	7/2/14		Added V4(Dim:xx Mask::"xxx")
v4imu.c		1.574	VEH	6/2/14		Added RPT(Row:xxx and Select::xxx) with interfaces to v4rpp
v4dpi.c		1.573	VEH	5/22/14		Tweaked JSON dereference to handle compound points (a..b, a,b,c  etc.)
v4extern.c	1.573	VEH	5/20/14		HTTP(... Out::stream) - writes result to named stream
v4imu.c		1.573	VEH	5/20/14		JSON(... XML::stream) - read from named buffered stream
v4extern.c	1.572	VEH	5/6/14		Echo(Buffer::xx) - takes contents of Output(Buffer? Id::xx) and Echos it, works with JSON & Javascript options
v4imu.c		1.572	VEH	5/6/14		JSON names not quoted unless necessary
v4imu.c		1.572	VEH	5/1/14		Added List((...) Key2::xxx), also - List((...) Key/Key2::xxx) returns entire sublist if length <> 2
v4extern.c	1.572	VEH	4/16/14		Modified SendMail() so that Attach/Include/Text/HTML tags accept stream name from Output(... Buffer?)
v4dpi.c		1.571	VEH	4/1/14		List.xxx compiles into List(List* Key::xxx)
v4imu.c		1.571	VEH	4/1/14		Added List(((key val) (key val) ...) Key::key)
v4imu.c		1.571	VEH	3/28/14		Added JSON(Array::(xxx xxx)) option to convert multiple identical XML tags to a single JSON array
vcommon.c	1.570	VEH	2/26/14		Allow phone numbers to have "." (999.123.1234)
v4imu.c		1.570	VEH	2/5/14		Beefed up JSON(XML::xxx) to handle lists (files) and very large xml strings
v4imu.c		1.569	VEH	1/28/14		Added V4(XML::{asis, uppercase, lowercase})
v4...		1.569	VEH	1/27/14		Added type & extension to telephone, mods to acceptor, displayer, MakeP and other spots
v4imu.c		1.568	VEH	1/23/14		Added Str(xxx Hash::SHA256) (using tomcrypt freeware)
v4im.c		1.567	VEH	12/30/13	Added Ifnot() opcode
vcommon.c	1.567	VEH	12/16/13	Added Output(Buffer?) handlers
v4extern.c	1.567	VEH	12/16/13	Added Output(Buffer?, Length?, Get::num)
vcommon.c	1.566	VEH	12/5/13		Changed (fixed) handling of FixedLength & Default options in Table so that huge empty strings not being written out
v4eval.c	1.566	VEH	12/5/13		ditto
v4eval.c	1.565	VEH	11/14/13	Added Table Prefix/Suffix, Added Echo(NoPrefix::table) to generate using column names without prefix/suffix
v4imu.c		1.564	VEH	11/6/13		Fixed problem in List(Group::xxx) where context trashing itself
vcommon.c	1.564	VEH	10/29/13	Fixed problem in Include Table with FixedLength fields
v4imu.c		1.564	VEH	10/9/13		V4(xxx Type?) - returns actual point type, not type associated with Dim:xxx, V4(xxx Segment?) returns UV4:List if xxx is a list
v4imu.c		1.564	VEH	10/3/13		Added handling of V4DPI_PntType_SymDef to v4im_GetPointXXX() modules
v4dpi.c		1.563	VEH	8/19/13		Added JSON parse flag to tread name:0nnnnn as string not integer
v4imu.c		1.563	VEH	8/19/13		JSON(Style::xxx) expanded to handle: Comma and LeadingZeo and list (ex: JSON(Style::(Comma LeadingZero) ....))
v4imu.c		1.562	VEH	8/16/13		Removed Set() intmod
v4extern.c	1.562	VEH	8/1/13		Moved curl_easy_cleanup() to top of routine in vHTTP_SendToServer() to see if it helps in dealing with Access violations
v4im.c		1.561	VEH	7/26/13		Fixed semantics of MT()
vcommon.c	1.561	VEH	7/24/13		Changed OSInfo() & v_ipLookup() to return list of ip addresses with IPAddress::v4/v6
v4extern.c	1.560	VEH	6/27/13		Fixed bug in HTTP() when result too big or not valid UTF-8 (really nasty one)
v4extern.c	1.559	VEH	6/19/13		SendMail(Reply::address) option added
v4dpi.c		1.559	VEH	6/19/13		Fixed problem with JSON parser when handling quoted object names - 'xxxxx':value 
v4eval.c	1.559	VEH	6/19/13		Fixed macro argument parse error (ex: cnfP(xxxx,<<Str( .... )>>,'desc'))
v4imu.c		1.559	VEH	6/18/13		Drawer() - Add, Empty, Head, Pop?, Remove, Tail tags now chain
v4imu.c		1.559	VEH	6/18/13		Error() - Count? returns error count, Count::n sets error count to n
v4extern.c	1.558	VEH	6/3/13		SendMail() - added Name::xxx & "xxxx" (value) option to pass extra MIME arguments
v4imu.c		1.557	VEH	5/6/13		Added JSON(Parse? Ignore::list XML::xmlString) - converts XML to JSON
v4test.c	1.556	VEH	4/28/13		Added logic to prevent call to v4_error from within signal trap - just exit
v4dpi.c		1.555	VEH	4/17/13		Fixed problems with: $$(udate:mmddyy), HTTP(Name::xxx value)
v4imu.c		1.554	VEH	4/2/13		Format(XML::{encode,decode,rpc}), removed Format(XML?)
v4extern.c	1.553	VEH	3/7/13		SendMail(Attch::file OR Attach::(file filename))
v4imu.c		1.553	VEH	3/1/13		List.Dim:xxx same as List(List* Dim:xxx)
v4extern.c	1.553	VEH	1/21/13		Fixed memory allocation in HTTP
v4imu.c		1.553	VEH	1/20/13		Added Str(xxxx Word::xxxx), better bounds checking in Split::xxx with huge strings
vcommon.c	1.552	VEH	1/7/13		Added ISO 2-character country code to v4countryinfo.v4i & internal stuff
vcommon.c	1.551	VEH	12/18/12	v_Spawn - flipped from multiple arguments to single argument block, added max-output-byte limit
v4imu.c		1.551	VEH	12/18/12	Spawn(Bytes::max-output)
v4extern.c	1.551	VEH	12/17/12	Added Zip(Open::file.zip ListOf?)
v4imu.c		1.550	VEH	11/19/12	Fixed bug in one-to-many projections (duplicating first entry)
v4extern.c	1.550	VEH	11/16/12	Added Lock(Life:{process,release})
v4imu.c		1.549	VEH	9/17/12		Fixed bug in v4l_ListPoint_Modify() when flipping Int2 list to V4L_ListType_HugeInt
v4imu.c		1.548	VEH	9/6/12		Names() - Added Mask::"xxx", LC?, UC?, Capitalize?
v4imu.c		1.548	VEH	9/6/12		Made Delete synonomous with Remove in multiple intmods - long term want to get rid of Remove
v4dpi.c/v4imu.c	1.548	VEH	9/3/12		Implemented JSON parsing and referencing - Parse(json-string JSON?), $(x.y.z) and $$(x.y.z)
v4extern.c	1.547	VEH	8/20/12		Fixed a couple of problems in HTTP() module (Include::file)
v4dpi.c		1.546	VEH	8/10/12		Read of (X)Dict values from database - if comma separated then parse as multiple points
v4imu.c		1.545	VEH	7/19/12		Added Flatten(Target::value dim dim ...) to expand target value into list of component points
v4imu.c		1.544	VEH	7/15/12		Added Same() & nSame(), added nStr(), changed V4(Epsilon::xxx) to handle list of 2 values, returns list if values are different
v4extern.c	1.543	VEH	6/22/12		Fixed problem with Socket(Include::file) option
v4dpi.c		1.542	VEH	6/6/12		Fixed problems with $xxx scoping, allowed for updating (ex: $x:={$x+1})
v3pcku.c	1.104	VEH	5/30/12		Turned off use of shared segments for v3x when compiling under LINUX486
various		1.541	VEH	5/29/12		Added preOffset & postOffset to UOM conversions (to handle, for example, celcius -> fahrenheit)
vcommon.c	1.540	VEH	5/21/12		Fixed screwy stuff in UPeriod acceptor
v4ctx.c		1.540	VEH	4/18/12		Fixed issue with Parse() not freeing allocated tcb at end-of-list
v4imu.c		1.539	VEH	3/23/12		Added Rpt(Key::xxx) to output KExxx to v4rpp
v4imu.c		1.539	VEH	3/21/12		Changed Fail() module around to save its message
v4imu.c		1.538	VEH	3/3/12		Added (implemented) Rpt(Footer::())
v4imu.c		1.538	VEH	2/20/12		Fixed problems in v4im_GetPointXXX() with V4DPI_PntType_Special points- was not always evaluating them
v4imu.c		1.537	VEH	2/16/12		Added UV4:none logic to List(list Intersect::xxx) and List(Union? xxx) to ignore UV4:none arguments
v4imu.c		1.537	VEH	2/6/12		Fixed problem in Str(RegExp::xxx Num? With::xxx) - failed if pattern not match & should not fail
v4extern.c	1.537	VEH	1/27/12		Modified db() modules and dim:() handling to use UDBSpec:"xxx"/UDBArgs:() instead of Alpha/List dimensions
v4extern.c	1.536	VEH	1/21/12		Added EchoW(Exit::code)
v4imu.c		1.536	VEH	1/20/12		Added NGram() module (testing, may not keep it)
v4eval		1.536	VEH	1/18/12		Added "Set QParameter x value" (to match -rpxQ startup switch)
v4extern.c	1.536	VEH	1/11/12		Fixed up Message() to handle replies better. Added Reply option, removed Ack option.
v4extern.c	1.536	VEH	1/11/12		Fixed up Message() to use Windows events to nofify main thread when listener thread recieves message
v4extern.c	1.535	VEH	1/6/12		Added dbConnect(Port::nnn) option
v4dpi.c		1.535	VEH	12/23/11	Fixed bug when comparing 0-length string to another non-0 string in IsctEval()
v4extern.c	1.535	VEH	12/22/11	Redefined TXTEOL for non-windows systems (take out when all running v18 of v4server
v4imu.c		1.535	VEH	12/21/11	Added Secure(Value::"section.name", Value::logical, Value?)
vextern.c	1.534	VEH	12/14/11	Modified Zip() routines to return file name/compression ratio instead of just TRUE
vextern.c	1.534	VEH	12/14/11	Added SendMail() call using libCURL
v4extern.c	1.534	VEH	12/14/11	Modified FTP/HTTP to use WinINet instead of WinHTTP because Windows is pain-in-the-ass
v4extern.c	1.534	VEH	12/8/11		Added FTP() call using libCURL
v4extern.c	1.534	VEH	12/6/11		Added HTTP() call with Windows linkage via WinHTTPxxx and Linux linkage via libCURL
v4imu.c		1.533	VEH	11/29/11	Fixed Str(xxx RegExp::list) to work properly
v4imu.c		1.533	VEH	11/25/11	Added Parse(xxx As::what) options
v4xxx.c		1.533	VEH	11/18/11	Added FIXEDtoDOUBLE() macro to convert with rounding
v4imu.c		1.532	VEH	11/11/11	Changed Error(Reset?) to additionally reset gpi->ErrorCount
v4imu.c		1.531	VEH	11/9/11		Enhanced Index::dim to accept Index::dim:start-value (e.g. index starts at 0 instead of default of 1)
v4extern.c	1.531	VEH	11/5/11		Added Output(ASCII?)
v4imu.c		1.531	VEH	11/4/11		Added ContextP::xxx to Enum/Tally/Sort/EnumCL, normalized Context::xxx handling
v4extern.c	1.530	VEH	11/3/11		Added Socket(Wait::max) to alter response wait from 30 seconds to max
v4imu.c		1.530	VEH	10/31/11	Added Tally(Num::xxxx)
v4imu.c		1.530	VEH	10/31/11	Fixed problem with Enum/Tally Context::[isct] - if evaled to List then was not expanding out to component points
v4im.c		1.529	VEH	10/24/11	Added Len() module, List(... Length?)
v4dpi.c		1.529	VEH	10/24/11	Added $sym:=value & $sym constructs
vcommon.c	1.528	VEH	10/20/11	Fixed Windows deadlock problem with GetModuleFileName & changed window v_spawnSort to run old spawn logic (sans pipes) to avoid other apparent deadlocks
v4imu.c		1.527	VEH	10/13/11	Added Area(NoError? and Error::logical) options to match V4()
vcommon.c	1.526	VEH	10/10/11	Fixed bug in parse of Alpha Table Column - allocate space for trailing EOS if string length > 255
v4imu.c		1.526	VEH	10/7/11		Added Dbg(Trace::Minus/Plus/Set/Test::trace-options)
v4extern.c	1.526	VEH	10/4/11		Added dbConnect(Reset?) & code to force reset of connect of mysql if out-of-synch error
v4imu.c		1.525	VEH	9/23/11		Added Array([xxx] ListOf::n) - returns list of dimension values for n'th array dimension
v4imu.c		1.525	VEH	9/23/11		Added Tally(xxxx (Sum::xxx By::(a b c) Array::name)) - creates Array([name]) with values
v3stru.c	1.103	VEH	9/23/11		str_trim() - return pointer to null-terminated string if trim results in zero-length string
v3mthu.c	1.103	VEH	9/10/11		Fixed bug in xctu_popint() when converting dec to int (used to ignore 10th digit)
v4im.c		1.524	VEH	9/7/11		Added v4xdb_FreeSavedInfo() & changed AuxVal of saved xdb rows from dimId to XDBIDPART(xdbId)
v4dpi.c		1.523	VEH	9/6/11		Added [xxxxx],,[error-trap] - to ignore error trap
v4im.c		1.523	VEH	9/1/11		Added MT(xxx) & nMT(xxx)
v4imu.c		1.522	VEH	8/29/11		Added V4(Test::xxxx) & V4(Test?) - sets/returns 'test' point which can be used for anything
v4imu.c		1.522	VEH	8/29/11		Added Secure(Test::logical) & Secure(Test?) - restricts use of V4(Test::xx)
v4dpi.c		1.522	VEH	8/24/11		Added dbtable:(+jointable+jointable... <optional column/field list> ... ) -> [UV4:ValuesTree Dim:dbtable,jointable,jointable...  Int:xxx | (field list)]
v4dpi.c		1.522	VEH	8/22/11		Added syntactic sugaring: array[Dim:xxx limits] & array[index index ...]
v4imu.c		1.521	VEH	8/19/11		Added Parse(Currency::string)
v4imu.c		1.521	VEH	8/18/11		Added startup -ro xxxx, V4(UserOptions?) returns, V4(UserOptions:xxx) sets
v4imu.c		1.520	VEH	8/4/11		Fixed DTInfo() to save copy of first argument so it does not get trashed if updated
v4imu.c		1.520	VEH	8/2/11		Added/implemented Enum/Tally Skip::num option
v4eval.c	1.520	VEH	7/29/11		Expanded macro(x x x ,... => macro x x x , ... options => <macro Javascript and/or Indent=none, etc>
v4extern.c	1.520	VEH	7/29/11		Added Echo(Javascript::file) that does same as below
v4common.c	1.520	VEH	7/29/11		Added BigText option: Javascript that removes extraneous text/comments from javascript BigText
v4dpi.c		1.520	VEH	7/26/11		Fixed allocation of bibuf's so that only 1 per stack frame is allocated
v4ctx.c		1.520	VEH	7/26/11		Fixed EOF of Parse() list to properly free up allocated tcb
v4extern.c	1.519	VEH	6/28/11		Changed handling of dbError() so that we don't lose the error message from MIDAS SQL server
v4extern.c	1.519	VEH	6/22/11		Fixed bug in Tree(Dim:int Create?), added Tree(xxx Root?), added chaining & link to List/Str when result is list/string
v4common.c	1.518	VEH	6/14/11		Added V4LEX_Option_Space / V_OpCode_Space to return white spaces as token
v4imu.c		1.518	VEH	6/14/11		Parse(xxxx Token::Space)
v4imu.c		1.517	VEH	6/10/11		Fixed bug in verifyList() - was not copying to respnt when recursing and arg is list - just returning original pointer, screwed up when recursing from verifyList()
v4eval.c	1.517	VEH	6/7/11		Set Trace [+/-]TimeStamp, Dbg(Trace::TimeStamp), xv4 -S startup option
vcommon.c	1.517	VEH	6/7/11		Include time/cpu in status output lines
v4extern	1.516	VEH	6/3/11		Modified OSFile(dirname) to always end directory with slash
v4dpi.c		1.516	VEH	5/26/11		Added VDTYPE_MIDAS_Zip to handle MIDAS 5/9 digit zip codes, v4dpi_AcceptValue() accepts "nnnnn.xxx" as integer (takes up to decimal point)
v4extern.c	1.515	VEH	5/24/11		Added better check in MIDAS/SQL when MIDAS returns invalid data stream
vcommon.c	1.515	VEH	5/24/11		Modified hash64b to use unsigned char
v4imu.c		1.515	VEH	5/23/11		Changed Str(xxx Replace::(list list ...) so that it works when replacement string contains matched string (i.e. does not expand ad infinitum)
v3prsdef.c	1.102	VEH	5/16/11		Increased prsu_v4b_elMax from 200 -> 300, V3_PRS_SKELETON_ENTRY_MAX from 60 -> 100, V3_SOB_HASS_PER_OBJECT_MAX from 200 -> 300, V3_PRS_SKELETON_ELEMENT_MAX from 200 -> 300
v4imu.c		1.515	VEH	4/29/11		Added code to finish fetching mysql rows if Enum(table If::xxx Then::xxx) terminates before all rows fetched (error 2014)
v4imu.c		1.515	VEH	4/29/11		Added Enum(isct/list) to enumerate through, returns number of iterations
v4imu.c		1.515	VEH	4/28/11		Added Drawer(Remove:::xxx) & Str(Before/After:::xxx)
v4dpi.c		1.515	VEH	4/28/11		Added tag:::xxx capability
v4imu.c		1.514	VEH	4/20/11		New baf->xxx stuff to make it dynamic and faster, Enum(... Calc:max Columns:max Recap:max)
v4imu.c		1.513	VEH	4/20/11		Added Str(xxx All::(list))
v4imu.c		1.513	VEH	4/15/11		Added Array([xxx] index index ... Add::number) - adds number into array slot
v4dpi.c		1.513	VEH	4/8/11		Added Country parsing to v4dpi_AcceptValue()
v4imu.c		1.513	VEH	4/8/11		Added Array( . . . Add::value)
v4imu.c		1.512	VEH	3/17/11		Added Rpt(Bottom::xxx, Top::xxx) with corresponding section BSMETA IB/IT handlers in v4rpp.c
v4eval.c	1.511	VEH	2/24/11		Do not close ajax stream after Evaluate if called from EvalCmd() module
vcommon.c	1.511	VEH	2/24/11		Added v_ParseUDate() and moved parsing from v4dpi to vcommon
v4imu.c		1.510	VEH	2/23/11		Removed Summary() module (commented it out)
v4extern.c	1.510	VEH	2/17/11		Added option to xxx_FreeStuff() to only free up statements under connection
v4dpi.c		1.510	VEH	2/11/11		parse of int:none was not setting dimension
v4imu.c		1.510	VEH	2/9/11		Rpt((x UV4:none)) - outputs blank column
v4imu.c		1.510	VEH	2/4/11		Fixed bug in Counter() - Counters->Count not being incremented on new counter, went into endless loop when counter max exceeded
v4extern.c	1.510	VEH	2/1/11		Implemented dbXct(... Key?) for inserts to mySQL via ODBC connection
vcommon.c	1.510	VEH	1/31/11		Fixed UDate:current.month+1.current to pull last day of month if 'current' is 31 and next month does not have 31 days
v4imu.c		1.510	VEH	1/28/11		Made wait time for vsort from within V4 to be infinite (problem with no more resources if spawned multiple times?)
vcommon.c	1.509	VEH	1/21/11		Added SSDim(... Style::{indent1/2/3})
vcommon.c	1.508	VEH	1/7/11		Rewrote stream handlers to not bump-down structure on close so that multi-treading would work properly
v4imu.c		1.508	VEH	1/04/11		Spawn(Maximum::seconds Create::file Append::file) added
v4defs.c	1.508	VEH	1/03/11		Increased Tree() limit from 256->512
v4extern.c	1.507	VEH	12/30/10	OSFile(xxx Directory::{Create, CreateIf, Exists, Remove})
v4test.c	1.507	VEH	12/22/10	v4_error() - don't auto-exit if batch job until 5 errors
v4imu.c		1.507	VEH	12/20/10	Fixed bug in MakePm() - when single value - not setting pt->Bytes
vcommon.c	1.506	VEH	12/8/10		v_FormatTele() - format mask now normal;no-area-code;no-number
v4imu.c		1.506	VEH	12/7/10		Added UV4:Skip to Context:: in Tally(), Enum(), Sort()
v4zip,vcommon	1.505	VEH	12/2/10		Added zip compression routine
v4defs.c	1.504	VEH	11/24/10	Fixed problem with SetCALisNOW() macro
v4imu.c		1.504	VEH	11/23/10	Added Summary() module, -S startup option
v4extern.c	1.504	VEH	11/23/10	Added V4(Log?) - parses Summary() output
v4extern.c	1.503	VEH	11/4/10		dbConnect - changed options to ignore UV4:none, etc.
v4extern.c	1.502	VEH	10/24/10	Added Echo(JSON::name ....) to handle extremely long lines out JSON string output
v4imu.c		1.502	VEH	10/22/10	Added V4(Sort?, Sort::seconds)
vcommon.c	1.502	VEH	10/22/10	Added timeout handling to v_SpawnProcess()
v4imu.c		1.502	VEH	10/20/10	Fixed bug in In(point Alpha:<>xxxx)
v4imu.c		1.501	VEH	10/5/10		Fixed problem in Ajax output routines - had to increase buffer sizes (also in v_Msg())
v4imu.c		1.501	VEH	10/2/10		Fixed problem in Tree() handling - memory issues in updating previously saved (to Area) tree
v4extern.c	1.500	VEH	9/13/10		Added Socket(Text::logical) to handle binary (Text::FALSE)
v4imu.c		1.500	VEH	9/10/10		Fixed problem in Project() with one->many (call to list modify altering get-next sequence thus losing values)
v4imu.c		1.500	VEH	9/8/10		Bug in Project() - have to normalize UCCHAR points -> Alpha so lookups work
v4imu.c		1.500	VEH	9/7/10		Fixed problem in Transform() due to new SETBYTESGRPINT macro implementation
v4extern.c	1.499	VEH	8/14/10		Added Zip() module
v4imu.c		1.499	VEH	8/4/10		Tweaked List()/ListNE() logic in eval-ing results
v4dpi.c		1.499	VEH	8/4/10		uom_init now returns T/F, parse logic checks
v4extern.c	1.498	VEH	7/31/10		Added Secure(Data::logical) - control redirection of Data stream
v4imu.c		1.497	VEH	7/16/10		Added Rpt(Link:{none,external,secure,user}) to map to xv4rpp -l{n,e,s,u} options
v4imu.c		1.497	VEH	7/16/10		Fixed bug in In() - determining if something in list had problems
v4imu.c		1.496	VEH	7/9/10		Added Rpt(Columns::(. . . (redef start end) ... ))
v4eval.c	1.495	VEH	7/8/10		Fixed problem in v4dpi_BindListMake() - unquote binding intersection before creating new binding
v4imu.c		1.495	VEH	7/8/10		Added Image::() to EchoS()
v4imu.c		1.494	VEH	7/1/10		Added Rpt(Image::(url localfile [rows columns]) Image::"url/file")
v4imu.c		1.493	VEH	6/11/10		Modified Index::xxx in Tally() to do what all other enumerating Index tags do
v4dpi.c		1.492	VEH	6/6/10		Added '#define NEWQUOTE' & related code to implement
vcommon.c	1.491	VEH	6/4/10		Added loop to vout_OpenStreamFile() to retry if output file is locked
vcommon.c	1.491	VEH	6/1/10		Added v_ipLookup() routine to handle IPv4/IPv6 lookups
v4imu.c		1.490	VEH	5/17/10		Added List(xxx Num::n Remove::pt)
vcommon.c	1.489	VEH	5/5/10		Fixed bug with v_StringLit() - dstlen was UCCHAR s/b LENMAX
v4extern	1.488	VEH	4/21/10		Fixed problem with dbGet(xxx Table::xxx) & mySQL
v4imu.c		1.488	VEH	4/21/10		Bug in Table(Acceptor::xxx), added Table(Scale:num & Scale?)
v4imu.c		1.487	VEH	4/6/10		MakePM() now ignores UV4:none points
v4mm.c		1.487	VEH	4/6/10		Rewrote v4mm_OpenCloseLock() to use semaphore(Linux) / mutex(Windows)
v4extern.c	1.487	VEH	4/6/10		New stuff to handle domain name resolution, IPV6
v4test.c	1.486	VEH	4/2/10		Added -rpXQ & -rpXq to enclose #X# value in double(Q) or single(q) quotes
vcommon		1.486	VEH	4/2/10		Fixed problem with 'multi-line' iscts with embedded #x# values
v4dpi.c		1.485	VEH	3/26/10		Bug in v4dpi_PointToStringML() - not truncating at max on some character strings
v4imu.c		1.485	VEH	3/26/10		v4im_GetPointUC() - handle char & ucchar within module instead of calling v4dpi_PointToStringML() (faster)
v4imu.c		1.484	VEH	3/5/10		Fixed bug in Counter hash logic ('&' -> '%') to prevent endless looping
v4imu.c		1.483	VEH	2/15/10		Fixed problem in MakeP(dim:x x:value) - set Grouping to _Single, not 1
v4imu.c		1.482	VEH	2/11/10		Fixed problem with MakeP(Dim:telephone "string")
v4dpi.c		1.481	VEH	2/8/10		Problem with Format(dim.. Point?) - only showing ".."
v4imu.c		1.480	VEH	2/3/10		Added Rpt(Cookie::"name=value;name=value") support
v4imu.c		1.479	VEH	1/29/10		Tweaked v4im_GetPointLog() to handle V4DPI_PntType_Fixed
v4extern	1.478	VEH	1/6/10		Fixed bug hashing negative tree node ids
v4imu.c		1.477	VEH	1/5/10		Added Rpt(If::logical) (same as EchoS(If::logical))
v4extern.c	1.476	VEH	1/2/10		Tree(Remove::{Child,Node}, bug fixes in tree->rpp routine
v4stat.c	1.475	VEH	12/29/09	Added 64-bit random number generator, hooked up to Str(xxx Random::n) routine
v4*.c		1.474	VEH	12/21/09	Removed NEWV4RFORMAT and WANTNEWMACROS conditionals
v4dpi,v4imu.c	1.474	VEH	12/21/09	Prevent Dbg() from going interactive if stdin not attched to console
v4eval,v4imu.c	1.473	VEH	12/10/09	Added 'Table name Other xxxx' and Table(xxx Other?/Other::"xxxxx")
v4eval.c	1.472	VEH	12/9/09		Macro calls = arg=<<xxxxx>> supported to handle embedded '>' with xxxxx
v4extern.c	1.472	VEH	12/9/09		Tweaks to GraphConnect
v4imu.c		1.471	VEH	12/8/09		Added Bits(xxx ListOf?), recoded Bits() to handle 64 bit ints, better error checking
v4ctx.c		1.471	VEH	12/3/09		Added [UV4:displayerCTX dim..] handling to v4ctx_ExamineCtx()
vcommon.c	1.470	VEH	11/18/09	Move v_ParseDbl/Int/Log from v4rpp to vcommon & changed V4/V4RPP to use them globally
v4im.c		1.469	VEH	11/16/09	dbVal(stmtId dim index) - now supported (stmtId is Statement::xx id)
v4extern.c	1.469	VEH	11/16/09	Fixed problems with MIDAS SQL returning multiple rows
v4extern.c	1.469	VEH	11/16/09	dbInfo(Columns? & ColCap?) supported for MIDAS SQL
v4imu.c		1.468	VEH	11/15/09	Dim(xxx) - returns dimension of xxx (xxx is not evaluated)
v4extern.c	1.468	VEH	11/15/09	Tree(node Elevate?) - elevates children of node to node's parent
v4imu.c		1.467	VEH	11/10/09	Fixed bug in Format(xxx rdbID::MIDAS) (doing UCstrcat instead of UCstrcpy)
v4extern.c	1.467	VEH	11/10/09	EchoT() now buffers all arguments and outputs all at endo
v4eval.c	1.467	VEH	11/10/09	Fixed problems with Column Point option (now takes string and expands in macro properly)
v4imu.c		1.466	VEH	11/4/09		Fixed problem with parsing UQtr - using wrong macro
v4imu.c		1.465	VEH	11/3/09		Added URL::(list) to include Target::xxx option, mods to Rpt() & EchoS()
vcommon.c	1.464	VEH	10/28/09	Fixed bug with Include Filter and macros (not calling filter appropriately)
v4imu.c		1.463	VEH	10/22/09	Added extra support for Rpt() & EchoS() with shell dimensions (pass dim:pntType in CV)
v4eval.c	1.462	VEH	10/5/09		Added extra error check for macro argument starting with "<" but no ending ">"
v4eval.c	1.461	VEH	10/3/09		Fixed bug in macro parsing when there is a huge (too big) btArg
v4defs.c	1.460	VEH	9/30/09		Fixed YYMMtoUQTR() macro, increased V4DPI_PointIntMix_Max to 125
v4imu.c		1.459	VEH	9/21/09		Added V4(Context::{Local,Global})
v4eval.c	1.458	VEH	9/6/09		Made a couple of string buffers more local, eliminate overrun on large alpha inputs
v4dpi.c		1.457	VEH	9/1/09		Added support for multi-user update of External dictionary
v4eval.c	1.457	VEH	9/1/09		Added Area 'LOCKS' to trigger the above






Module		Ver	Who	When		What
--------------	----	------	---------	-----------------------------------------------------------------
v3iou.c		1.4	VEH	02/07/94	Upped listen() limit from 5 to 10
v4mm.c		1.3	VEH	02/07/94	Upped open-close lock tries from 1000 to 3000
v4mm.c		1.4	VEH	02/08/94	Upped GLOBALBUFTRYMAX from 109 to 509
v4isu.c		1.5	VEH	02/15/94	Added v4is_RebuildAreaIndex module
v4isu.c		1.5				Put code in v4is_RestoreArea to handle fast rebuild, multiple files
						 & V4IS files as input
v4test.c	1.5				Added -of, -osh, -osd, -k options
v3iou.c		1.5	VEH	02/15/94	Added support for V4IS_PCP_GP_DataOnly
v4is.c		1.6	VEH	02/15/94	Added DataOnly put mode (does not write keys)
v4is.c		1.7	VEH	02/17/94	Removed error in Delete if key was already gone/missing
v4mm.c		1.7	VEH	02/17/94	Upped GLOBALBUFTRYMAX from 509 to 1509
v4mm.c		1.7	VEH	02/17/94	Added V4IS_RETRY_PANIC logic to better control panic lockouts
v4mm.c		1.8	VEH	02/18/94	Added try1-7 to locksomething to trace failure reasons
v4test.c	1.9	VEH	02/21/94	Added -px commands to xv4
v4isu.c		1.9	VEH	02/21/94	Added WindowsNT support to RebuildAreaIndex, RestoreArea, ...
v3driver	1.6	VEH	02/22/94	Added SIGTERM support (also to V4IS trapping)
v4mm.c		1.10	VEH	02/23/94	Added extra checks in ShareSegInit before reset of area global segment
v4test.c	1.11	VEH	02/27/94	Added -lu switch
v4isu.c		1.11	VEH	02/27/94	Added support for -lu switch to v4is_DumpArea
v4mm.c		1.12	VEH	02/28/94	v4mm_ShareSegInit - changed test for "0" keyflag
v3driver.c	1.7	VEH	02/28/94	Overhauled xv3 switch handler, added new options for V3-V4 integration
v4mm.c		1.13	VEH	03/01/94	Added REREAD flag to v4mm_BktPtr
v4isu.c		1.13	VEH	03/01/94	If dataid not found, force REREAD of bucket to try & fix corrupted data
v4isu.c		1.14	VEH	03/02/94	Added lt->LocksTree/IDX to bugchk dump,
v4mm.c		1.14	VEH	03/02/94	Added code to re-synch LocksIDX if lock waiting
vconfig.c	1.14	VEH	03/02/94	Dropped panic lockout interval from 300 to 150 (V4IS_RETRY_PANIC)
v4test.c	1.14	VEH	03/02/94	Added -pS to re-synch LocksIDX
v3mscu.c	1.8	VEH	03/02/94	Added /OS/ & /BOX/
v3oper.c	1.8	VEH	03/02/94	Added sys_info(/OS/ & /BOX/) via vconfig:V3_CONFIG_BOX, _OS
v4im.c		1.15	VEH	03/06/94	Fixed bug in Plus() intmod
v4dpi.c		1.15	VEH	03/06/94	Fixed bug in DimGet for dims with AutoCtxPnt
v4dpi.c		1.16	VEH	03/07/94	Ability to have (dim:x :y :z ...)
v4im.c		1.16	VEH	03/07/94	Added EnumCE (old ForEachPushAndEval), EnumCL, ListModify
vcommon.c	1.16	VEH	03/07/94	Ability to have logical=logical:,logical:,...
v4mm.c		1.17	VEH	03/07/94	Fixed bug in LockSomething when releasing lock of defunct pid
v4mm.c		1.17	VEH	03/07/94	Added RELLTE to release lock table in case of nested error
v4dpi.c		1.18	VEH	03/11/94	Added check to IsctEval for max areas in EvalList
v4eval.c	1.18	VEH	03/11/94	Fixed bug so Context PUSH works even with "Set NoResult"
v3iou.c		1.9	VEH	03/16/94	iou_get_tty for WINNT- ignore CR's
v3prsb.c	1.9	VEH	03/16/94	Added check in *objectname* for package_slot 0
v3oper.c	1.9	VEH	03/16/94	Changed bigbuf from 1024 to 4096 bytes
v3vax.c		1.9	VEH	03/16/94	Coded nint() properly for VAX
v4im.c		1.19	VEH	03/16/94	Changed def of PntType_Char to handle 0-length strings
v3oper.c	1.10	VEH	03/17/94	Added VFT_FIXSTRSK & check in sys_address()
v3mthu.c	1.11	VEH	03/21/94	Fixed bug in cvtvs with negative numbers & 0
v4test.c	1.20	VEH	03/21/94	Added -bsr (override bucket & max record) to xv4
v4mm.c		1.21	VEH	03/21/94	TotalTreeLocks only updated on Write lock, not sure we need it at all!
v4oper.c	1.12	VEH	03/21/94	Commented out VFT_FIXSTRSK in EVAL_SKELETON so we can move to new stuff to live
v3driver.c	1.12	VEH	03/22/94	Added -o optionlist switch
v3oper.c	1.12	VEH	03/22/94	Added v3_info(/startup_options/)
v4mm.c		1.22	VEH	03/22/94	Added extras checks to v4mm_BktWrite for data bucket integrity
v4mm.c		1.22	VEH	03/22/94	Added v4mm_ZapBucket to quickly clear out bad bucket
v4test.c	1.23	VEH	03/25/94	Added -lud, -pCW/-pCE options
v4is.c		1.23	VEH	03/25/94	Added IgnoreLock to BktPtr of root bucket on open
v3oper.c	1.13	VEH	03/25/94	Added mth_log() function
v4mm.c		1.24	VEH	03/25/94	Added extra arg (lockareaid) to LockRelease for problem from LockSomething2
v3driver.c	1.14	VEH	03/26/94	Fixed switch handler
v3v4.c		1.15	VEH	03/27/94	Added v4_DictionaryGet()
v4eval.c	1.25	VEH	03/29/94	Added "> or >> file" to EVALUATE command
v3oper.c	1.16	VEH	03/29/94	Fixed problem with mth_min/mth_max with non-integer arguments
v4mm.c		1.26	VEH	03/31/94	Changed retry in v4mm_BktPtr, added wait-for-lock if bucket_in_pool > 10
v4mm.c		1.27	VEH	03/31/94	Beefed up RELLT - does bugchk & generates v4_error if problem
v3iou.c		1.17	VEH	04/04/94	Added io_misc(xxx,/manual_lock,unlock,lockwait/)
v4mm.c		1.28	VEH	04/05/94	MakeNewAId - updates acb with area id
v4isu.c		1.28	VEH	04/05/94	Added start of v4is_RootInfoCheckSum - dummy routine for now
v4mm.c		1.29	VEH	04/07/94	Added closeflag arg to AreaFlush() to force lock waint on area close
v4dpi.c		1.30	VEH	04/08/94	Added [? ... ] & auto trace in IsctEval
v4is.c		1.31	VEH	04/10/94	Added V4IS_PCB_OM_MustBeNew to v4is_Open
v3iou.c		1.18	VEH	04/10/94	Added interface to above via /new_file/
v4test.c	1.32	VEH	04/11/94	Added Summary Info to end of "xv4"
v4im.c		1.33	VEH	04/12/94	Added new floating routines, changed return on numerical intmods
vcommon.c	1.33	VEH	04/12/94	Added InpMode_MacArg so args don't return EOL when expanded
v4defs.c	1.34	VEH	04/14/94	Added DataId2 to handle 6 byte RMS-RFA
v4server.c	1.1	VEH	04/14/94	Got it to work on VMS, handle DataID access across platforms, ...
v4mm.c		1.35	VEH	04/15/94	Commented out lock wait (v 1.29) - seems to cause deadlock
v4mm.c		1.36	VEH	04/16/94	Changed handling of closeflag in AreaFlush, took out lock wait in BktPtr also
v4mm.c		1.37	VEH	04/18/94	Took out HANGLOOSE in BktPtr retry logic
v4dpi.c		1.37	VEH	04/18/94	Changed Minimum/Maximum to ...mum( list , @[isct] )
v4dpi.c		1.37	VEH	04/18/94	Added ListHead() & ListTail(), started Sort()
v3iou.c		1.19	VEH	04/18/94	Added qi->passall to sys_queue() routine
v4mm.c		1.38	VEH	04/19/94	Added retry logic to NewBktPtr to we don't have to wait for lock
v4mm.c		1.39	VEH	04/20/94	Added test to wait queue to grab lock if waited too long
v4mm.c		1.39	VEH	04/20/94	Panic lockout bugchk only after first panic
vconfig.c	1.39	VEH	04/20/94	PROCESS_GONE treats pids <= 0 as gone
v4is.c		1.39	VEH	04/20/94	Added mesaages/bugchks to PosToPar/RmvIdxEntry/IdxStshLk for demon problem
v4im.c		1.40	VEH	04/21/94	Got Sort() to work
v4mm.c		1.41	VEH	04/26/94	Added failure in GlobalBufGrab if any lock for bucket exists
v4defs.c	1.41	VEH	04/26/94	Added pcb->AreaFlags
v4is.c		1.41	VEH	04/26/94	Added shutdown in v4is_Close() for V4IS_PCB_OF_DisconnectOnClose
v3iou.c		1.20	VEH	04/26/94	Added mapping between /DisconnectOnCLose/ and pcb->AreaFlags
v3defs.c	1.20	VEH	04/26/94	Increased PURE_BUF max from 15000 to 16300
v4ctx.c		1.41	VEH	04/27/94	Added heap-free call in ctx add-point when overlaying existing point
v3xctu.c	1.21	VEH	04/29/94	Added watch_location & watch_value
v3oper.c	1.21	VEH	04/29/94	Added v3_set_parameter(/watch_location/,sys_address(int))
v3oper.c	1.22	VEH	05/04/94	Fixed (pointer+int) in PLUS+1 (was adding 1 not i)
v3iou.c		1.22	VEH	05/04/94	Rearranged io_xxx calls so VMS can handle RMS & V4IS
v4mm.c		1.42	VEH	05/04/94	Moved call to OpenCloseLock in seg_init below increment of PidCount
v3xxx		1.23	VEH	05/10/94	Minor changes for NT486
v4xxx		1.43	VEH	05/10/94	Minor changes for NT486
v4mm.c		1.44	VEH	05/12/94	Changed count from 5 to 25 in PROCESS_GONE check in Lock on Write locks
v4mm.c		1.44	VEH	05/12/94	Changed PROCESS_GONE check in LOCKLT from all after n to every 27'th
v4mm.c		1.44	VEH	05/12/94	Added bugchk if SegShareInit detects defunct pid
v4is.c		1.44	VEH	05/12/94	Added ReLock at end of v4is_Put to relock record
v3xxx		1.24	VEH	05/12/94	Added interface for relock
v4im.c		1.45	VEH	05/13/94	Added ListSelect() & CompXX() modules
v4mm.c		1.45	VEH	05/17/94	Commented out v4_error in RELLT (cure worse than disease?)
v4mm.c		1.45	VEH	05/18/94	Added locnum to LOCKLT & check for ActivePid
v4im.c		1.46	VEH	05/19/94	Fixed Sort() to read/write out "wb" instead of "w"
v4dpi.c		1.46	VEH	05/19/94	Changed PIntMod from []module() to module( ... ... [-])
v4isu.c		1.47	VEH	05/20/94	Changed fopen in bugchk from "w" to "a"
v4test.c	1.47	VEH	05/20/94	Changed -pLnum so num is unique lockid, not lock position
v4ctx.c		1.48	VEH	05/23/94	Added support for Context Add dim:{undefined}
v4mm.c		1.48	VEH	05/25/94	Added sleep for LONGTIME if too many pids per lock
v4eval.c	1.48	VEH	05/25/94	Added EBIND command
v4test.c	1.49	VEH	06/01/94	Added 'pJnum' option to test out locking
v4mm.c		1.50	VEH	06/02/94	Added ENOENT error check in ShareSegInit before create_seg
v4is.c		1.51	VEH	06/06/94	Changed LockRec <= 0 to != UNUSED
v4isu.c		1.51	VEH	06/06/94	Added extra ()'s in BugChk to get more than 1 line entry (i hope)
v4mm.c		1.51	VEH	06/06/94	Only "Force clear of lock..." if no other processes logged into area
v4is.c		1.52	VEH	06/06/94	Commented out LockRec in v4is_Put/Update - causing problems!
v3pcku.c	1.25	VEH	06/14/94	Added check in pcku_load_module() for module redefinition
v3v4.c		1.26	VEH	06/15/94	Fixed problem with V4 returing a floating point number to V3
v4mm.c		1.53	VEH	06/30/94	Flipped to READ locks before all BktWrites
v4isu.c		1.54	VEH	07/03/94	Added -m num (megabytes wasted) to -lu switch
v3prsc.c	1.27	VEH	07/05/94	Fixed AlphaOSF qualifier on struct definition
v4is/u.c	1.54	VEH	07/06/94	Changed v4is_RemoteFileRef, added v4is_ConnectFileRef(), v4is_DisconnectFileRef()
v3driver.c	1.28	VEH	07/06/94	Added -C & -h switches to command line
v4isu.c		1.55	VEH	07/07/94	Fixed bug in v4is_ConnectFileRef
v4is.c		1.55	VEH	07/07/94	Added Client-Server support for locking, various get modes, actual key byte length
v4cli.c		1.56	VEH	07/08/94	v4_error now updates with VStdErr_xxx code
v4is.c		1.56	VEH	07/08/94	Fixed bug in client-server CopyData routines, added better error reporting
v4is.c		1.56	VEH	07/10/94	Added support for remote v4is_Put in client-server
v3defs.c	1.29	VEH	07/10/94	Increased fork max from 5 to 7
v4dpi.c		1.57	VEH	07/12/94	Added "dim.." to mean "dim:{all}"
v4is.c		1.58	VEH	07/19/94	Added more V4CS functionality-fixes: Get(First), open errors, io_puts
v4is.c		1.59	VEH	07/20/94	Inhibit link to V4SERVER if new file
v3iou.c		1.30	VEH	07/20/94	^P handler now shows aid of file on server link side
v4mm.c		1.60	VEH	07/22/94	Fixed ProcessGone for WINNT (check for invalid hProcess)
v4mm.c		1.60	VEH	07/22/94	Added more info to bugchk on "Could not allocate global buf"
v4dpi.c		1.60	VEH	07/22/94	Added AllDims logical: Bind [xxx xxx *] = [xxx [ * ]]
v4eval.c	1.61	VEH	07/23/94	Added bind weight adjustment in BIND/EBIND- Bind (+/-n) ...
v4dpi.c		1.61	VEH	07/23/94	Added bind weight adjustment to BindListMake()
v4dpi.c		1.61	VEH	07/26/94	Changed AllDims to Dim xxx Decomposition & xxx?
v4isu.c		1.61	VEH	07/26/94	Added key display to v4is_ExamineBkt
v4mm.c		1.62	VEH	07/28/94	Reset lock to _Write in BktFlush routine (see 1.53)
v4mm.c		1.63	VEH	08/13/94	Added extra info to "Could not locate area" errors
v4is.c		1.63	VEH	08/13/94	Added V4Server support for Put:Update/Insert/Delete
v4mm.c		1.64	VEH	08/15/94	Added better recovery logic to BktPtr, BufGrab, check in AreaFlush for updates
v4is.c		1.65	VEH	08/18/94	Fixed error cleanup to ignore sequential-only files
v4mm.c		1.66	VEH	08/19/94	Fixed bugchk in AreaFlush so only called when current process is last one attached
v4mm.c		1.67	VEH	08/24/94	Changed sequence in BktPtr (after read) set Updated before access allowed to bucket
v4defs.c	1.67	VEH	08/24/94	Padded BktHdr to word boundary (so VAX compitico with Alpha/486)
v4dpi.c		1.68	VEH	08/28/94	Allowed for match on dim:{UNDEFINED} if no point in context
v3iou.c		1.30	VEH	08/29/94	Changed shutdown() calls to close()
v3vax.c		1.30	VEH	08/29/94	Added SJC_LOG_SPECIFICATION into vax_queue for batch log files
v4mm.c		1.69	VEH	08/31/94	Made changes in BktPtr (re-added LOCK after read)
v4is.c		1.70	VEH	09/06/94	Added extra argument to v4is_Open() call (OptControlBlk)
v4isu.c		1.70	VEH	09/06/94	Added support for index-only
v4mm.c		1.70	VEH	09/06/94	Added support for index-only
v4test.c	1.70	VEH	09/06/94	Added support for index-only, -bo switch
vconfig.c	1.71	VEH	09/08/94	Defined SOCKETCLOSE() to handle NT/Unix socket closes
v3iou.c		1.31	VEH	09/08/94	Added SOCKETCLOSE, changed calls to v4is_Open for extra argument
v4isu.c		1.72	VEH	09/11/94	Fixed error message & core dump problem due to uninited pointer
vcommon.c	1.73	VEH	09/12/94	Fixed v4mm_FreeChunk to return 0 if OK
v3iou.c		1.32	VEH	09/19/94	Added SOCKETCLOSE(termSocket) to iou_close_all
v4is.c		1.74	VEH	09/20/94	Added unlock to v4is_Put & v4server
v4mm.c		1.75	VEH	09/21/94	Changed WinNT_ProcessGone to closehandle() when done
v3pcku.c	1.33	VEH	09/25/94	Use name-checksum for WINNT CreateFileMap instead of just name
v4is.c		1.76	VEH	09/26/94	LockRelease in v4is_Put sets LockRec = UNUSED before call in case of trouble
v4is.c		1.77	VEH	09/27/94	Added v4is_DisconnectAreaId to V4Server close logic
v4isu.c		1.77	VEH	09/27/94	Fixed up v4is_DisconnectAreaId to close socket after last areaid left
v4mm.c		1.77	VEH	09/27/94	Fixed ReadBkt to work on WINNT (missing IndexOnly logic)
v4defs.c	1.78	VEH	09/28/94	Upped LockTable_Max from 50 to 75
v4defs.c	1.79	VEH	09/30/94	Added LockMax to RootInfo, dropped expansion from 8 to 7
v4test.c	1.79	VEH	09/30/94	Added -cb & -cl switches & LockMax to -l printout
v4is.c		1.79	VEH	09/30/94	Upped lock max in calls to SegInit from 50 to 75 (temp for CDF)
v4isu.c		1.80	VEH	09/30/94	Sequential dump/restore preserves BufCount/LockMax
v4test.c	1.80	VEH	09/30/94	-or sets GlobalBufCount/LockMax in RootInfo
v4is.c		1.80	VEH	09/30/94	Open of existing file sets BufCount/LockMax according to RootInfo
v3driver.c	1.34	VEH	09/30/94	Added WSACleanup() to exit_handler, output PID on -w startup
v4mm.c		1.80	VEH	10/03/94	Added some CloseHandle() calls (CreateFileMap)
v4isu.c		1.80	VEH	10/03/94	Added some CloseHandle() calls (CreateProcess)
v3prsc.c	1.35	VEH	10/03/94	Fixed bug in alignment of CLONE dcl's
v3iou.c		1.36	VEH	10/08/94	Added error codes to messages on C I/O errors
v4mm.c		1.81	VEH	10/08/94	Took out CloseHandle in OpenClose for shared memory segment
v4defs.c	1.82	VEH	10/08/94	Added "short unused" to  FreeDataInfo to int-align substructure (on VAX)
v4mm.c		1.83	VEH	10/16/94	Added check for Alloc lock after BktFlush in BufGrab
						Added check for duplicate buckets in glb after BktRead in BktPtr
v4isu.c		1.83	VEH	10/16/94	Took out FreeChunk(data) call at end of RebuildIndex
v4isu.c		1.83	VEH	10/16/94	Added checks for divide-by-zero in -lu results
v4oper.c	1.37	VEH	10/18/94	Added break to sys_info(/process_id/) for WINNT
v3iou.c		1.37	VEH	10/18/94	Added FreeChunk to pcb (unitp->file_ptr) in iou_close
v4isu.c		1.84	VEH	10/18/94	Fixed ReadFile's in utilities for WINNT
v4sort		1.5	VEH	10/18/94	Added (missing) code for -kk sort on WINNT
v4is.c		1.84	VEH	10/18/94	Added FreeChunk to AuxDataPtr in v4is_Close
v4mm.c		1.84	VEH	10/18/94	Fixed WINNT problems with PROCESS_GONE()
v4mm.c		1.84	VEH	10/18/94	Added bugchks to ShareSegInit, fixed WINNT problems
v4is.c		1.85	VEH	10/19/94	Added FreeChunk(dr) on v4server Gets()
v4test.c	1.86	VEH	10/20/94	Added -d num to dump out a bucket
v4test.c	1.87	VEH	10/27/94	-pR does not mark bkt as UNUSED if Updated flag is set
v4isu.c		1.87	VEH	10/27/94	RebuildAreaIndex - ignores bad buckets, sets ri->NextAvailBkt to LastOK+1
v3oper.c	1.38	VEH	10/27/94	sys_logout justs exits if xv3 process started with -w switch
v4mm.c		1.88	VEH	10/31/94	Added extra bugchks to ShareSegInit
vconfig.c	1.88	VEH	10/31/94	Added WINNT_PID- use <processId>,,<threadId> as Pid in V3/V4
v4im.c		1.89	VEH	11/01/94	Added EvalArithExp()
vcommon.c	1.89	VEH	11/01/94	Added to RevPolParse to handle iscts & points
v3iou.c		1.39	VEH	11/04/94	Added xvrc to WINNT sys_print handler
v4im.c		1.90	VEH	11/04/94	Added dim:point|(context list) to EvalArithExp
v4test.c	1.91	VEH	11/04/94	Took out old test junk
v3oper.c	1.40	VEH	11/06/94	Added v3_set_parameter(/jobid/,jobid)
v4mm.c		1.92	VEH	11/06/94	Added v4mm_SetUserJobId() call, stuff for LockExtra
v4is.c		1.92	VEH	11/06/94	Added for LockExtra
v4is.c		1.92	VEH	11/06/94	Added LockExtra=LockIDX in SplitIndex routine
v3v4.c		1.40	VEH	11/06/94	v4_error() - if first arg=0 then call v4is_ErrorCleanUp()
v3iou.c		1.41	VEH	11/07/94	Added io_get(/nextnum1/)
v4is.c		1.93	VEH	11/07/94	Added support for v4is_Get with NextNum1
v4test.c	1.93	VEH	11/07/94	Added -pN num to set NextNum1 on an area
vsort.c		1.6	VEH	11/07/94	Fixed so zero-record sort works
v3oper.c	1.42	VEH	11/08/94	Changed WinNT CreateProcess to keep same window
v4isu.c		1.94	VEH	11/08/94	Changed call to CreateProcess to keep same window
v4mm.c		1.94	VEH	11/08/94	Added check at end of LockSomething to ensure pid not in wait queue
v4test.c	1.94	VEH	11/08/94	Added -pK to force clear of locktable on area
v4is.c		1.94	VEH	11/08/94	Added support for V4IS_PCB_OF_ForceClear
v4mm.c		1.94	VEH	11/08/94	Added clearflag argument to ShareSegInit to handle above
v4oper.c	1.43	VEH	11/10/94	Fixed sys_info(/is_active_user/) & sys_info(/user_id/) for VAXVMS
v4is.c		1.95	VEH	11/13/94	Added IFTRACEAREA to various spots to find index problem
v4is.c		1.96	VEH	11/14/94	Added few lines to update ri->RecordCount
v4test.c	1.96	VEH	11/14/94	Added check for nested levels in -v
v3driver.c	1.44	VEH	11/14/94	Added call to v4is_ErrorCleanUp on ^C trap to free any locks
v4im.c		1.96	VEH	11/14/94	Added ListFromPoint() & changed ArithEval to handle
v4mm.c		1.97	VEH	11/14/94	Added several IFTRACEAREA's to v4mm
v3iou.c		1.45	VEH	11/16/94	Changed from +4 to +10 for WINNT submit NOW
v3oper.c	1.45	VEH	11/16/94	sys_rename(old,new,/noerror/) & sys_delete(old,/noerror/)
v4eval.c	1.98	VEH	11/16/94	Allowed for NId:a..b (used to be only int's)
v4defs.c	1.99	VEH	11/21/94	Added defs for hashed area
v4is.c		1.99	VEH	11/21/94	Added links in Get/Put/Open for hashed areas
v4hash.c	1.99	VEH	11/21/94	Created new for hashed areas
v3mscu.c	1.46	VEH	11/21/94	Added /hashed/, /cache/, /reset/ for new hashed IO
v3iou.c		1.46	VEH	11/21/94	Support for hashed areas in iou_open
v3vax.c		1.46	VEH	11/21/94	Added support for qe->delete (SJC$_DELETE_FILE)
v4mm.c		1.100	VEH	11/23/94	Added auto-grab to timouts in LOCKLT & OpenClose lock module
v3edtu.c	1.47	VEH	11/23/94	Fixed Yest/Today/Tom with minutes_west, added "NOW" to xti_date_time
v4im.c		1.101	VEH	11/23/94	MakePoint( Dim:xxx ) - creates new point on that dim
v4im.c		1.102	VEH	11/27/94	IsctVals( @[isct] n ) - returns n'th possible value
v3oper.c	1.48	VEH	11/29/94	Added sys_info(/next_file/,v3__fileinfo)
v4mm.c		1.103	VEH	11/29/94	LockRelease - if not found does bugchk but no error
v4is.c		1.104	VEH	11/30/94	Fixed problem in v4is_Open() with FreeData & hashed files
v4defs.c	1.105	VEH	12/01/94	Added V4CI__CountryInfo
v3edtu.c	1.49	VEH	12/01/94	Added process->ci.xxx to xti/itx handlers
v3oper.c	1.49	VEH	12/01/94	Added v3_info(/country_info/)
v4defs.c	1.106	VEH	12/02/94	Added structs, etc. for DISPLAY windows in v4gui
v4cli.c		1.106	VEH	12/02/94	Added beginnings of new v4gui_xxx modules for DISPLAY windows
v3oper.c	1.50	VEH	12/05/94	Added /next_file/ for VAXVMS
vconfig.c	1.51	VEH	12/06/94	Added RS6000AIX
v3...		1.51	VEH	12/06/94	RS6000 additions to v3driver, v3xctu, v3oper (all minor)
v3...		1.52	VEH	12/07/94	Changed some error mnemonics
v4...		1.107	VEH	12/07/94	Changed some error mnemonics
v3iou.c		1.53	VEH	12/12/94	Added /NEXT:day to sys_queue for WINNT to handle submits in future
v4is.c		1.108	VEH	12/12/94	Changed IndexAppendLink to handle split root properly
v3prsb.c	1.54	VEH	12/12/94	Added [isct] => macro expansion
v4is.c		1.109	VEH	12/14/94	Changed OpenFile to CreateFile so new files don't end up in system directory
v3...		1.55	VEH	12/15/94	Added V3E_xxx, changed all v3_error() calls to include
v3oper.c	1.55	VEH	12/15/94	Added v3_info(/error/) to return last error
v4...		1.110	VEH	12/15/94	Added V4E_xxx, changed all v4_error() calls to include
v4...		1.111	VEH	12/22/94	Changed casts, moved modules for MACTHINK
v3...		1.56	VEH	12/22/94	Changed casts for MACTHINK
v3oper/x.c	1.56	VEH	12/22/94	Split v3oper into v3oper/v3operx
v4im.c		1.112	VEH	12/27/94	Changed CTXADDLIST to ..CTXADDLIST, added Context()/ContextL()
v4is.c		1.113	VEH	12/29/94	Fixed RecordCount problem in StashData
v4...		1.114	VEH	01/01/95	Added MAXATTACH, GETGLB(ax) macro, AttachLastId/AttachId to mmm
v4isu.c		1.114	VEH	01/01/95	Added v4vms_xxx routine to see why V4/VMS so slow (not currently in use)
v4mm.c		1.115	VEH	01/03/95	Added v4mm_GetGLB & v4mm_SegControl for MAXATTACH option
v4mm.c		1.116	VEH	01/05/95	Fixes to v4mm_SegControl to make it work
v3iou.c		1.57	VEH	01/05/95	Added RS6000AI to sys_queue
v3operx.c	1.57	VEH	01/05/95	Added RS6000AIX to sys_info(/disk_space/)
v3edtu.c	1.58	VEH	01/06/95	Added /mdy/ to itx_date(), fixed bug in AIX/C compiler (unsigned char)
v4mm.c		1.117	VEH	01/07/95	Fixes to v4mm_SegControl
vcommon.c	1.117	VEH	01/07/95	Fixed sleep routine via UNIX interval timer
v4isu.c		1.118	VEH	01/09/95	Fixed bug in -f switch on -oo rebuild (not setting fileref in key's AuxVal)
v4isu.c		1.119	VEH	01/11/95	Added LIB$SPAWN call in rebuild/rebuild-index for VMS call to vsort
v3xctu.c	1.59	VEH	01/13/95	Changed v3_error to printf/exit on error restart in xctu_call_parser()
v4isu.c		1.120	VEH	01/13/95	Added dup-key detection to -v (Verify) module
v4im.c		1.121	VEH	01/15/95	Added check for number of arguments
						CompXX no longer needs Dim:Logical at end
						LogicalAnd/Or don't need Dim:Lgical at end, take any number of args
						Fixed bug in ListSelect, bug in DimUnique
v4im.c		1.122	VEH	01/16/94	All Logicalxx() -> xx(), Compxx() -> xx() (ex: LogicalAnd now And)
						Took out result-dim from EnumCL
v3v4.c		1.60	VEH	01/18/95	Added v4_handle() and associated modules
v3mscu.c	1.60	VEH	01/18/95	Added /info/, /parent/, /pointer/, /freememory/
v4test.c	1.123	VEH	01/18/95	Added -vf to verify foreign-key files
						Added better messages in -pR if area already locked down
v3mscu.c	1.61	VEH	01/19/95	Added /link/, /before/, /after/, /EndOfList/
v3xctu.c	1.61	VEH	01/19/95	Added mod_item_push(/handle_delete/,handle,psi)
v3v4.c		1.61	VEH	01/19/95	Added han_Set/GetType & han_xxx for list manipulation
vcommon.c	1.124	VEH	01/20/95	Moved han_xxx() from v3v4 to vcommon & v4defs
v3iou.c		1.62	VEH	01/23/95	Fixed bug in tty screen input- reset input_ptr if truncate at end-of-field
vsort.c		1.6	VEH	01/25/95	Generates error if "-l 0" given
v4dpi.c		1.125	VEH	01/25/95	Added "dim.point" => [dim* point]
v4im.c		1.125	VEH	01/25/95	Added EchoT()
v4test.c	1.125	VEH	01/25/95	Added key information to "-l" output
v4im.c		1.126	VEH	01/26/95	Bind() returns second argument as value to call
v4ctx.c		1.126	VEH	01/26/95	Put check in FrameAddDim to return immediately if point already in context
						Changed v4ctx_HasHaveNBL() to re-read from area(s) to re-synch
v4dpi.c		1.127	VEH	01/27/95	Check in DimUnique() to prevent update to read-only area
						Check for max number of nested IsctEval calls
v4im.c		1.127	VEH	01/27/95	Check for max number of nested IntMod calls
v4im.c		1.128	VEH	01/30/95	Beefed up traceback handling in EvalArithExp
v4defs.c	1.129	VEH	02/01/95	Added more GUI stuff
v4dpi.c		1.129	VEH	02/01/95	Got rid of unnecessary junk in IsctEval() call & upped nest limit
v4test.c	1.129	VEH	02/01/95	Added -oc to copy one area into another
						Added -pCR to allow reads, hang if write
v4is.c		1.129	VEH	02/01/95	Put code to handle -pCR
v4im.c		1.130	VEH	02/06/95	Added Pack1616() & Pack248()
v4defs.c	1.130	VEH	02/06/95	Added more GUI stuff
v4is.c		1.131	VEH	02/09/95	Impletented Obsolete via v4is_ObsoleteData()
v4isu.c		1.131	VEH	02/09/95	Added V4IS_FileRef_Obsolete support to various dump/restore/reformat utilities
v3iou.c		1.63	VEH	02/09/95	Added /obsolete/ support
v3vax.c		1.63	VEH	02/09/95	Added /obsolete/ support (same as delete)
v4dpi.c		1.132	VEH	02/12/95	Added [x y z]>Dim:a to IsctParse, print & IsctEval
v4im.c		1.132	VEH	02/12/95	Changed EnumCE( list eval dim:xxx ) to add list element as dim:xxx point
v4dpi.c		1.133	VEH	02/13/95	Changed handling of "remain" context point to exclude points after "|"
vsort.c		1.8	VEH	02/21/95	Added VSORT_AS_SUBROUTINE, changed vsortdefs.c to vsortdef.c
vsort.c		1.8	VEH	02/21/95	  added POSIXMT, MACTHINK so works on MAC
v4defs.c	1.134	VEH	02/21/95	Changed a few structs with ": n" to align consistantly
v4im.c		1.134	VEH	02/21/95	Added vsort_entry for MACTHINK, ListFromxxx() - default to Dim:List
v4test.c	1.134	VEH	02/21/95	Added quick test of struct/union sizes
v4isu.c		1.134	VEH	02/21/95	Added vsort_entry for MACTHINK
v3prsc.c	1.63	VEH	02/27/95	Added check to significant digits in ib/iw/il/ip dcl statement
v3prsc.c	1.64	VEH	03/01/95	Changed output of V4 bindings for struct to FLDx macro calls
v3prsc.c	1.64	VEH	03/01/95	Added abilility to incorporate field "comment" in dcl (see above)
v4dpi.c		1.135	VEH	03/02/95	Changed [x x]>Dim:x to [x x]->Dim:x (conflict with Eval [x x] > "file")
v4eval.c	1.136	VEH	03/14/95	Added NoAutoCreate to Dimension specification
v4dpi.c		1.136	VEH	03/14/95	Added support for above in DictEntryGet
v4dpi.c		1.137	VEH	03/21/95	Added dim:=[isct] & [isct]->[isct], changed IsctParse, IsctEval
v4ctx.c		1.137	VEH	03/21/95	Added "huge" list support to PointList modules
v4isu.c		1.137	VEH	03/21/95	Added dataid to parameter list of StashIndexData to reuse dataid's
v4eval.c	1.138	VEH	03/24/95	Added "Set Trace EvalToContext"
v4test.c	1.138	VEH	03/24/95	Added conditionals for building v4.dll under WINNT
v3iou.c		1.65	VEH	03/31/95	Pass back max record length on io_Open via file_record_length
v4im.c		1.139	VEH	03/31/95	Fixed bug in EnumCL with optional context
v3oper.c	1.67	VEH	04/10/95	Added bitmap_testdf, bitmap_extend, bitmap_bytes
v3driver.c	1.68	VEH	04/17/95	Changed stuff around for NT DLL (moved to v3xctu)
v3defs.c	1.68	VEH	04/17/95	Added globals to process->xxx for NT DLL
v4defs.c	1.140	VEH	04/20/95	Added V4C__ProcessInfo to handle DLL
v4ctx.c		1.140	VEH	04/20/95	Added v4ctx_GetProcessInfo()
v3defs.c	1.69	VEH	04/20/95	Added to db__process to get rid of global statics (for NT DLL)
v4im.c		1.141	VEH	04/24/95	Fixed bugs in Sort() (variable length lists, intptr)
v4im.c		1.142	VEH	04/27/95	Added Tally() routine
v3xctu.c	1.70	VEH	04/27/95	Fixed time offset to handle daylight savings under NT
v3iou.c		1.70	VEH	04/27/95	Spelling mistake: THURSADAY -> THURSDAY in NT batch submission
v4isu.c		1.142	VEH	05/04/95	Added better recovery checks for really trashed files
v4defs.c	1.142	VEH	05/04/95	Changed sizes of HUGE lists (smaller bucket size)
v4im.c		1.143	VEH	05/06/95	Added multiple list args to Tally() for bindings & bind-lists
v4im.c		1.144	VEH	05/09/95	Minor bugs in Tally()
v4eval.c	1.144	VEH	05/09/95	Added "Set MaxAllowedErrors n"
v4eval.c	1.144	VEH	05/09/95	Added "Area Rename old new"
v4is.c		1.145	VEH	05/14/95	RemoveIndexEntry()- call IndexConsolidate even if keycnt == 0
v4eval.c	1.145	VEH	05/14/95	Added SET Trace Tally/TallyBind
v4im.c		1.145	VEH	05/14/95	Fixed bugs in Tally()
v4eval.c	1.146	VEH	05/16/95	Added "Set MassUpdates" to Eval & MakeBindList (significant speedup!)
v4eval.c	1.147	VEH	05/18/95	Added support for Dimension type UDate/UMonth
v4dpi.c		1.147	VEH	05/18/95	PntType_UDate/UMonth support
vcommon.c	1.147	VEH	05/18/95	Pulled date routines from v3edtu/v3mscu
v4is.c		1.148	VEH	05/20/95	Added KeyMode_Int2 support
vsort.c		1.9	VEH	05/20/95	Added KeyMode_Int2 support
v4dpi.c		1.148	VEH	05/20/95	Added PntType_Int2 support
v4eval.c	1.149	VEH	05/21/95	Added stuff (V4IS_KeyMode_Binding) in prep for Int2 binding keys
v4eval.c	1.150	VEH	05/22/95	Took out 1.149 stuff & did it right with BindListShell
v4ctx.c		1.150	VEH	05/22/95	Fixed bug in List_Append (not checking max in all cases)
vconfig.c	1.150	VEH	05/22/95	Added ALPHAVMS to config, changed most VAXVMS's to VMSOS in v*.c
v4im.c		1.150	VEH	05/23/95	Added Pack4x16(int1 int2 int3 int4 optdim) intmod
v4eval.c	1.150	VEH	05/23/95	Added Real/Int2/Char support for Bind n (keys) in Dimensions
v4im.c		1.151	VEH	05/29/95	Added Tally:AVERAGE/COUNT to Tally(), changed ListPoint_xxx -> ListEl & ListSize()
v4eval.c	1.151	VEH	05/29/95	"Include V4" -> v3_syslib:v4kernel.inc
v4im.c		1.151	VEH	05/30/95	Sort() - uses vsort_entry(internalbuf) if possible
vsort.c		1.10	VEH	05/30/95	Added support for internal sort buffer using vsort_entry()
v4eval.c	1.152	VEH	05/30/95	Added "Hashed" point type
v3iou.c		1.71	VEH	05/31/95	Added -k to "at" & "batch" commands in sys_queue
v4im.c		1.153	VEH	06/02/95	Added Div() & EchoF(), IfTrue() no longer needs "else"
v4server.c	1.11	DMM	05/30/95	Added dual port, user/node to file, status commands, fileref->filename list
v4srvcli.c	1.11	DMM	05/30/95	v4server client program- change fileref->filename & get status
v4eval.c	1.154	VEH	06/06/95	Added "if(1) NOBIND [isct]"
v4im.c		1.154	VEH	06/06/95	Error checking: divide by zero, EvalList arg must be list
v4im.c		1.155	VEH	06/08/95	Added Percent()
v4defs.c	1.156	VEH	06/12/95	Upped nested dim max, nested isct max, intmod max
v4defs.c	1.156	VEH	06/12/95	V4DPI__Binding - dropped buffer from Bucket_Max to its own max (about 20K)
v4im.c		1.157	VEH	06/13/95	Added 4th arg to Tally() to create list of Combo/By points
v3iou.c		1.72	VEH	06/14/95	Changed flip from + minutes to + hours at 999
v3defs.c	1.72	VEH	06/14/95	Raised V3_PROCESS_MODULE_MAX to 1200 (from 1000)
vcommon.c	1.157	VEH	06/15/95	Added V4LEX_EchoEveryStartNum to echo every n'th input line (via Set ECHO)
v4eval.c	1.158	VEH	06/18/95	Flipped V4 data from Index to Data buckets to speed up Bind processing
v4dpi.c		1.158	VEH	06/18/95	ditto (processing of >1million binds reduced by factor of 10+)
v4eval.c	1.159	VEH	06/20/95	Added macro caching if "Set MassUpdates"
v4ctx.c		1.159	VEH	06/20/95	Fixed problem in HasHaveNBL where not releasing allocated nlb's properly
v4eval.c	1.160	VEH	06/25/95	Dimension attribute: PointCreate New/Point
v4eval.c	1.160	VEH	06/25/95	Point dim integer - adds point to internal dimension list
v4im.c		1.160	VEH	06/25/95	Added ListOf() as synonym for DimUnique(), also works with PointCreate Point
vsort.c		1.11	VEH	06/26/95	Fixed bug in sort of (double)
v4im.c		1.161	VEH	06/26/95	Bugs in sort of (double), added Identity() IntMod
v4...		1.162	VEH	06/30/95	Support for MULTINET tcp/ip, ALPHA-VMS
v3...		1.73	VEH	06/30/95	  ditto
v4server.c	1.12	VEH	06/30/95	Support for MULTINET, Alpha-VMS, bugs in 1.11 finally tested & fixed
v4server.c	1.12	VEH	06/30/95	Got rid of V4 evals for FileRef->name evaluation
v3...		1.74	VEH	07/05/95	More bugs/changes for Alpha-VMS (prsu_str_hash, V3VAX, ...)
v4eval.c	1.163	VEH	07/05/95	Check command for "(" (ex: macro PO(...) not confused with POINT command)
v4dpi.c		1.163	VEH	07/05/95	Added dim name to errors in PointAccept to make debugging easier
v4im.c		1.164	VEH	07/08/95	Fixed Tally() bug when generating more that one list (multiple sort files)
v4im.c		1.164	VEH	07/08/95	EchoT() - expands list argument as multiple args
v4im.c		1.164	VEH	07/09/95	Added Matrix( rows cols eval frow fcol ulhand )
v4mm.c		1.165	VEH	07/13/95	Changed segment control- HPUX only allows one attach per process
v4im.c		1.166	VEH	07/14/95	Added DateInfo() intmod, forced all ints in Sort() to double
v4im.c		1.166	VEH	07/14/95	Added ListSample() intmod
v4im.c		1.167	VEH	07/20/95	Added AggVal() intmod
v4eval.c	1.167	VEH	07/20/95	Added Aggregate< keypt val1 val2 ... valn > command
v4im.c		1.168	VEH	07/23/95	Added EnumAL( @[isct] pt1 pt2 ... ptn ) => list
v4dpi.c		1.168	VEH	07/23/95	Added `[isct] to force an evaluation (like in Tally())
v4dpi.c		1.168	VEH	07/23/95	Dim S Shell, Context Add S:[isct], = [S*] works now
v3iou.c		1.75	VEH	07/24/95	Added HPUX code to sys_queue for print queue handling
v4eval.c	1.169	VEH	07/25/95	Added macro( arg=<x,y,z...> ), fixed  Point dict problems
v4im.c		1.169	VEH	07/25/95	Fixed Or() intmod & Plush()
v4im.c		1.169	VEH	07/25/95	Added MatrixT()
v4dpi.c		1.169	VEH	07/25/95	Can now handle & eval [isct],Intmod() & [isct],point
v4eval.c	1.169	VEH	07/26/95	Added <> to macro arg to indicate "no argument"
v4ctx.c		1.169	VEH	07/26/95	Added check to Context to prevent dim*  from going into context
v4im.c		1.169	VEH	07/26/95	When pulling isct from context, eval if not quoted
v4dpi.c		1.169	VEH	07/26/95	Decomp point -> context is now a quoted isct
v4eval.c	1.170	VEH	07/27/95	Added Dim attribute: Numeric so numeric Dict entries would ignore leading 0's
v4eval.c	1.170	VEH	07/27/95	Bug in BindListMake when nbl overflowed- now v4_error with offending Bind
v3iou.c		1.76	VEH	08/10/95	Added forms control to HP spooling
v4im.c		1.171	VEH	08/12/95	Added In( pt1 pt2 ) intmod
v4test.c	1.172	VEH	08/16/95	Added support for _OM_Partial in -pR, -pl, -pK
v4isu.c		1.172	VEH	08/16/95	Added _OM_Partial to allow partial open & link to glb/lt if problems
v4im.c		1.173	VEH	08/16/95	EnumCE( list eval optdim ) - added optdim
v4eval.c	1.173	VEH	08/16/95	Agg < ... <> .. > - "<>" denoted undefined value
v4dpi.c		1.173	VEH	08/16/95	PointParse: "nnn" & "-nnn" assumed to be Int:nnn/-nnn
v4im.c		1.174	VEH	08/17/95	Added MatVal( matrix row column rowid colid ) intmod
v4im.c		1.174	VEH	08/17/95	Added process->Matrix to track current matrix so above would work
v4eval.c	1.174	VEH	08/20/95	Added parsing of Aggregate< ... dim* ... >
v3iou.c		1.77	VEH	08/22/95	Added xvrc to sys_queue for Unix systems
v4eval.c	1.175	VEH	08/23/95	Position to EOF on EchoOut append for WinNT
v4im.c		1.175	VEH	08/23/95	Added check for list arg in Enum...
v4defs.c	1.175	VEH	08/23/95	Changed short to int on defs of list entry-counts (lists > 65k)
v4im.c		1.176	VEH	09/04/95	Added list capability to evalpt in Matrix() (i.e. eval more than one isct)
v4im.c		1.177	VEH	09/07/95	Added ListPos( pt list ) intmod
v4im.c		1.177	VEH	09/07/95	Added ContextHold( pt isct/list ) intmod
v4ctx.c		1.177	VEH	09/07/95	Added support for V4C_FrameId_Hold in FrameAddDim to support above
v4im.c		1.178	VEH	09/09/95	Added IsctMake() intmod
v4im.c		1.178	VEH	09/09/95	Added Revserse:[] shell dimension to Sort (also v4kernel.inc)
v4im.c		1.178	VEH	09/09/95	Added Covers( @[isct] list ) intmod
v4dpi.c		1.178	VEH	09/09/95	Added dim:{sample}
v4kernel.inc	1.178	VEH	09/09/95	Has:ValExp: Covers( @[dim1:pt] (dim2:x) ) -> [Has:ValExp dim2:{sample} Dim:dim1]
v4im.c		1.178	VEH	09/10/95	Added "p5" or local selection to Tally()
v4im.c		1.178	VEH	09/10/95	Added Sum, Average, Count, ListOnly, By, ListBy, Select shell dims to Tally()
v4im.c		1.178	VEH	09/10/95	And() now takes a list as an argument
v4im.c		1.178	VEH	09/10/95	Added EQk() (equal kind-of), EQ now checks dimension & value
v4eval.c	1.179	VEH	09/12/95	Added "Include file MACRO name TAB HEADER CSV" to accept spreadsheet file via macro
vcommon.c	1.179	VEH	09/12/95	Change to ReadNextLine to support above
v4im.c		1.180	VEH	09/14/95	Added global Select:xxx to Tally()
v4im.c		1.181	VEH	09/19/95	List( Dim:xxx ), List( dim:a..b ), List( (list) (list) ... ), List( list )=size
						List( dim:a dim:b ), List( dim:a n ), List( list NTH:n )
						List( list SELECT:[] ), List( list POSITION:pt ), List( list SAMPLE:n)
						List( list HEAD:n ), List( list TAIL:n ), List( list APPEND:list )
v4im.c		1.181	VEH	09/19/95	Deleted ContextHold( )
v4im.c		1.181	VEH	09/19/95	Added Eval( Hold:pt Context:pt (list) (list) ... or @[isct] )
v4im.c		1.182	VEH	09/25/95	List( @[isct] ) returns ListType_Isct
v4ctx.c		1.182	VEH	09/25/95	Support for ListType_Isct
v4isu.c		1.182	VEH	09/26/95	Changed errno to NETERROR in RemoteFileRef handlers, tweaked BugChk format
v3prsc.c	1.78	VEH	09/28/95	Added check for subscripting > 0xffff
v4eval.c	1.183	VEH	09/30/95	Added Aggregate & FastAdd options to Area command (aggs->new area)
v4isu.c		1.183	VEH	09/30/95	Fixed "-k" option to work with areas without foreign key info
v4eval.c	1.183	VEH	10/01/95	Added "Set Trace Prompt" with subcommands: Context, Point, Fail, Trace
v4im.c		1.183	VEH	10/01/95	Added stuff to support calls to v4trace_PromptHandler
v4dpi.c		1.183	VEH	10/01/95	Added stuff in IsctEval to support above
v4ctx.c		1.184	VEH	10/02/95	Added Cache to prevent duplication of v4is_Gets on dims with no binds
v4eval.c	1.184	VEH	10/02/95	Added save of last AggGet buffer to speed up AggGetValue
v4dpi.c		1.185	VEH	10/04/95	Added lcl_Cache to DimGet() - 15% boost in parsing speed!
v4isu.c		1.186	VEH	10/06/95	Fixed -pR (no longer flags root bucket as updated)
v4isu.c		1.186	VEH	10/06/95	Fixed -or & -k (-or doubling up freed data block, -k getting screwed)
v4dpi.c		1.187	VEH	10/10/95	Added cache to DimUnique so don't rewrite AvailNum until AreaClose
v4dpi.c		1.187	VEH	10/10/95	Added cache to IsctEval (up to 16k evals/sec in Tally())
v4eval.c	1.188	VEH	10/19/95	Set Trace: Bindings, Macros, Lists, Frames, Points. Got rid of Set Results
v4im.c		1.188	VEH	10/19/95	All intmods- if arg is dim.. then convert to list of points on that dim
v4isu.c		1.189	VEH	10/19/95	Added better checks for trashed data bucket in RebuildIndex & DumpArea
v4im.c		1.190	VEH	10/22/95	Added EvalL() - evals and returns list of results
v4im.c		1.190	VEH	10/23/95	Added Sub:xxx to Tally()
v4dpi.c		1.191	VEH	10/24/95	Added support for CBIND (Bind with cache-enabled)
v4eval.c	1.191	VEH	10/24/95	CBIND - sets Dim & CacheIt on value point of bind
v4im.c		1.192	VEH	10/26/95	MakePoint(Dim:dim (list)) returns dim:list
v4dpi.c		1.192	VEH	10/26/95	PointToString now does multiple dates/months
v4im.c		1.193	VEH	10/27/95	Added MakeIsct() IntMod
v4im.c		1.193	VEH	10/27/95	Added SelectOnce:xxx to Tally()
v4im.c		1.193	VEH	10/27/95	MakePoint - handles Dim:xxx where xxx is Shell dimension properly
v4im.c		1.193	VEH	10/27/95	IntMod args- if `[xxx] evals to list then list expanded into multiple args
v4im.c		1.193	VEH	10/27/95	IsctMake - if arg is quoted isct then quoted = FALSE
v4im.c		1.194	VEH	11/01/95	Added MINIMUM/MAXIMUM to Tally(), BY to Sort(), WHILE to List()
v4eval.c	1.195	VEH	11/06/95	Added Set TallyMax command, fixed bug with compressed recs in v4dpi_BindRec
v4im.c		1.195	VEH	11/06/95	Added Tally- ListOf:pt to generate list of points
v4im.c		1.195	VEH	11/16/95	Matrix() - no longer requires Values:xx - takes from Columns if necessary
v4dpi.c		1.196	VEH	11/19/95	DimGet - verifies name is really a dim
v4eval.c	1.197	VEH	11/21/95	Added Area Index/Rebuild commands, better error checks
v4is.c		1.198	VEH	11/26/95	ReplaceDataId - added retry logic, changed bugchk from %d to %x
v4im.c		1.199	VEH	12/04/95	Added ISVALIDDIM() checks
v4dpi.c		1.199	VEH	12/04/95	UDate & UMonth of 0 print as "none"
v4eval.c	1.200	VEH	12/05/95	Added DateTime, OSHandle, AggVal dimension types
v4dpi.c		1.200	VEH	12/05/95	Added support for above
v4im.c		1.200	VEH	12/05/95	Ditto, AggVal( aggval ... ) supported
v4im.c		1.201	VEH	12/12/95	Eval(Context/Hold)- evals isct, Tally()- operation (Sum), & Bind optional
						Added calls for IsctEvalPH throughout
v4dpi.c		1.201	VEH	12/12/95	V4TRACE_Return added
v4eval.c	1.202	VEH	12/13/95	Added Dim name LOCAL & BindEval option
v4im.c		1.202	VEH	12/13/95	Implemented IsctVal( @[xxx] ) - returns list
v4im.c		1.202	VEH	12/13/95	Deleted 22 old/replaced intmods (Listxxx, MakeSeq_xx, ForEach...)
v4im.c		1.202	VEH	12/13/95	Renamed IsctMake() to MakeIsct() to be consistent with other Make...()
v4eval.c	1.202	VEH	12/13/95	Area Rename tries to delete target file (problem on WinNT)
v4im.c		1.203	VEH	12/15/95	Added Default:num to Matrix()
various		1.203	VEH	12/15/95	added "= NULL" to various static pointer declarations
v4is.c		1.204	VEH	12/18/95	Added DataOnly mode to v4is_Get() (much faster than sequential!)
v4server.c	1.13	VEH	12/18/95	Added DataOnly support
v4eval.c	1.204	VEH	12/18/95	Finished BindEval logic, added Set Trace BindEval
v4im.c		1.204	VEH	12/19/95	Added Totals to MatrixT
v4im.c		1.205	VEH	12/20/95	Added List( Set:Union/XUnion/Subract/Intersect ... )
v4...		1.206	VEH	01/07/96	Changed  all \n to \r\n for WinNT text end-of-line
v4eval.c	1.207	VEH	01/14/96	Now check for .v4 & .v4b extensions
v4dpi.c		1.208	VEH	02/16/96	Added handler for dim:>n, dim:<n, etc.
v4im.c		1.208	VEH	02/16/96	In() handles relational point specs
v4eval.c	1.209	VEH	02/18/96	Added NoExpand to Dimension (so lists not expanded when put in context)
v4dpi.c		1.209	VEH	02/18/96	Changed handled of Decomp points - can't match if pt in context
v4im.c		1.209	VEH	02/18/96	MakePoint( dim Special:Current,Undefined,All,... )
v4im.c		1.209	VEH	02/18/96	Added EnumALC() intmod
v4im.c		1.210	VEH	02/19/96	Added Transform() & EnumAE() intmods
v4im.c		1.211	VEH	02/20/96	Added EvalBL(), added 3rd arg to transform: Transform( test trans ftrans )
v4dpi.c		1.211	VEH	02/20/96	Added ctx->bpm to EvalDoit to handle EvalBL
v4im.c		1.212	VEH	02/25/96	IfDefine( list ... ) - all elements of list must be defined
v4dpi.c		1.212	VEH	02/25/96	Moved havedecomp=false in EvalDoit()
v4dpi.c		1.212	VEH	02/25/96	No match if decomp point is empty
v4eval.c	1.212	VEH	02/25/96	Pop off context frames on return from error trap
v3stru.c	1.79	VEH	02/27/96	Added str_type('string')
v4im.c		1.213	VEH	03/01/96	Added RowDim & ColDim to Matrix(), EchoF() takes list
v4*.c		1.214	VEH	03/03/96	Got rid of Grouping_Binding & Grouping_List
v4eval.c	1.215	VEH	03/04/96	Added AutoIsct to Dimension command
v4im.c		1.215	VEH	03/04/96	@dim* does not eval if context holds isct as value for dim
v4im.c		1.215	VEH	03/04/96	GetPointDbl() handles AutoIsct, DoMatrix(evalpt) handles AutoIsct
v4im.c		1.215	VEH	03/05/96	Added StdDev() intmod, Tally() supports StdDev operation
v4im.c		1.215	VEH	03/05/96	Fixed problems with Count/Min/Max in Tally()
v4test.c	1.216	VEH	03/08/96	Added -rq -rs -re -rt, got rid of -s -e -t
v4test.c	1.216	VEH	03/08/96	Added If Section name in conjunction with -rs name-list
v4test.c	1.216	VEH	03/08/96	Added -s logical=name (just like v3)
v4im.c		1.217	VEH	03/13/96	Added StdOut() & Format(), bug in Ifdefined (with Trace_Prompt)
v4im.c		1.218	VEH	03/15/96	Sum() now works with only 1 argument
v4im.c		1.218	VEH	03/15/96	MakePoint converts int <-> double
v4im.c		1.218	VEH	03/15/96	Percent()- 3rd arg can be Dim:xx, Tally() accepts global/local Dim:xx
vcommon.c	1.219	VEH	03/19/96	Added "Set Param letter number" & #A+# which increments number
v3v4.c		1.80	VEH	03/20/96	Added updated of process->v4_error on v4_error() call
v3prsc.c	1.80	VEH	03/20/96	Print out v4_error when get v4_error() in prs_attempt_structure_load
v4test.c	1.220	VEH	03/20/96	Added -rpX value startup option
v4eval.c	1.221	VEH	03/20/96	IsctEval of a point (not an isct) returns that point, dim* returns context value
v4ctx.c		1.221	VEH	03/24/96	HugeInt lists now stored in area available for update
v4is.c		1.222	VEH	03/28/96	Added a bunch of IFTRACEs, added KeyEntryNum to TRACE in BktWrite
v4im.c		1.222	VEH	03/30/96	Added EnumCT()
v4...		1.223	VEH	04/05/96	Added UWeek, UQuarter, and UPeriod point types, handlers, etc.
v4im.c		1.223	VEH	04/05/96	EnumCL( list list1 ) - if 2nd arg list1 then evaluates each pt in list and appends
v4im.c		1.223	VEH	04/08/96	Added PC(), PCChg1(), & PCChg2()
v4im.c		1.224	VEH	04/13/96	Added Special:LT/GT/... to MakePoint()
v4im.c		1.225	VEH	04/18/96	Added Aggregate( list test ) & EvalTL( list )
v4eval.c	1.226	VEH	04/24/96	Added Pattern command
v4dpi.c		1.226	VEH	04/26/96	Added v4dpi_MatchPattern() and associated hooks
v4ctx.c		1.227	VEH	04/28/96	Added v4ctx_DimBaseDim() & extra arg to FrameAddDim() to track it
v4eval.c	1.227	VEH	04/28/96	Added HasIsA attribute to DIMENSION command
v4dpi.c		1.227	VEH	04/28/96	Added logic to handle IsA links in IsctEval & IsctEvalDoit
v3iou.c		1.81	VEH	05/03/96	Added /handler_module/ & /nohandler/ logic to open/close/get/put
v3iou.c		1.81	VEH	05/03/96	Added io_misc(unit,/handler_module/,'module')
v3iou.c		1.81	VEH	05/03/96	Added io_misc(unit,/get_buf, put_buf, get_mode, & put_mode/)
v4dpi.c		1.228	VEH	05/20/96	Added IsA handling to Pattern lookups
v4dpi.c		1.229	VEH	05/29/96	Zero'ed out resdim in IsctEval() to get rid of funky errors
v4im.c		1.229	VEH	06/01/96	Plus() - rewritten with recursive module- expand out lists
v4...		1.230	VEH	06/20/96	Added trace argument to ListPoint_Value()
v4im.c		1.231	VEH	06/29/96	Added NOp() and EnumCE() now takes While:xxx as third argument
v4im.c		1.232	VEH	07/07/96	Div( num num dimension) - for floating point result!
v4eval.c	1.232				EXIT FILE - exits command file - stays in V4
v4im.c		1.232				Logicals coerced to integer 0/1, List(... Position:dim*) now handled
v4im.c		1.233	VEH	07/13/96	MakePoint(Dim:xxx a b) -> xxx:a..b
v4im.c		1.233				Changed handling of Tally hashing- added better warnings & errors
v4...		1.233				Added CASEofINT throughout
v4im.c		1.234	VEH	07/20/96	And() & Or() now take quoted iscts - won't eval extras args if unnecessary
v4im.c		1.235	VEH	07/23/96	Added Hold:xxx & Context:xxx to Tally()
v4im.c		1.235				Fixed several bugs in Sort()
v4dpi.c		1.235				Fixed several bugs in PointToString()
v4im.c		1.236	VEH	07/28/96	Added Trace() & TraceF()
v4im.c		1.236				MakePoint( Dim:Dim Dim:base ) now creates new dimension point
v4dpi.c		1.236				Fixed bug in caching, minor changes to tracing
v4dpi.c		1.237	VEH	07/30/96	Can parse points: Ord.BillTo.SlSRep.Mgr.Name
v4im.c		1.238	VEH	08/05/96	LEG( num1 num2 @isctL @isctE @isctG )
v4im.c		1.238				EnumCP( list Dim:xxx Index:Dim:xxx @isct )
v4im.c		1.238				IntMod now fails if any eval of args fail, use "Set Trace Prompt" to debug
v3operx.c	1.82	VEH	08/19/96	Added cb_Put(text) module
v3iou.c		1.82				Added mouse events to WINNT, tty_misc(112/113/114/115/116) to return mouse info
v4eval.c	1.239	VEH	08/20/96	Added CLIPBOARD command, v4eval_FileToClipboard(fn) module
v3iou/xctu.c	1.83	VEH	08/24/96	Changed exit-handler - turned off trapping, trace units closed
v4im.c		1.240	VEH	09/05/96	Added In() support for REAL dimension types
v4dpi.c		1.240				Added Range/Multiple support for REAL dim types
vcommon.c	1.241	VEH	09/28/96	Added vout_Text() & vout_NL() and changed bow-coo printf() calls
v4im.c		1.241				Added EchoS() to output for spreadsheet xface
v4dpi.c		1.242	VEH	10/01/96	Changed IsctEval to all dim.. in isct lookup
v4im.c		1.242				In() does not expand dim.. to list of points
v4ctx.c		1.243	VEH	10/04/96	Put loop in v4ctx_HasHavenbl() to go thru all points in ctx for dimension
v4im.c		1.243				Added MassUpdates to v4im_MakeTallyBind() for better performance
v4im.c		1.243				Added SSDim() and beefed up EchoS() to use info
v4cli.c		1.244	VEH	10/10/96	Added v4cli_AggRead() to read in aggregate file
v4dpi.c		1.244				Changed tracing in IsctEval_Doi() to be must more readable
v4dpi.c		1.244				Use ``isct in intmod arg to force fail if undefined (rather than error)
v3operx.c	1.84	VEH	10/13/96	sys_sleep(-n) sleeps for n milliseconds
v3operx.c	1.85	VEH	10/25/96	Fixed bug in WINNT sys_info(/next_file/)
v4ctx.c		1.245	VEH	10/27/96	Better handling of lists if points are not of same dimension
v4...		1.245				vout_Text() to handle different modes & channels of output
v4dpi.c		1.245				New argument to DictEntryGet() to control dimension on auto-create points
						IsctEval() argument to force eval of quoted isct (no more pt->Quoted=FALSE)
						DimUnique() - generates temp sequence of points even if area ReadOnly
						Better trace messages throughout IsctEval() & IsctEvalDoit()
v4im.c		1.245				MatrixS() to output to spreadsheet server
						Added MakeQIsct(), EchoD(), BindQ()
						`@[isct] - Eval isct, if undefined, fail parent isct, don't generate v4_error()
						MatrixXXX() - Rows/Columns can be specified as "dim.."
						relationals (LE, GE, IN, ...) accept 4 args: LE(1 3 100 200) = 100
v4dpi.c		1.246	VEH	11/15/96	Added hashed cache to DictEntryGet, added dynamic hashing to preven thrashing
						Added PointType_AggRef, AggRef:area:index,
						can load multi-aggregates & get lists correct
v4eval.c					FAggregate, BFAggregate
						Include option: Embedded - where macro name & args are separate line in text file
						Don't need to set up Dim Macro if Set MassUpdates
v4im.c		1.246				EnumEU()
v4is.c		1.246				Added _NoError option on v4is_Get, GetLen set to UNUSED if error
v4im.c		1.247	VEH	11/29/96	Added Cache() intmod
v4im.c		1.247	VEH	11/29/96	Added < p1 p2 ...> as equiv to Cache( p1 p2 ... )
v4im.c		1.248	VEH	12/05/96	v4im_VerifyList converts dim:a..b to list (ex: EnumCE(int:1..10 ...) now OK)
						Changed Set Trace Tally message to show percentage, etc.
						Added Select:xxx to EnumCE
						Should be able to use dim* in Rows:xxx RowCap:xxx, etc.
v4ctx.c		1.249	VEH	12/10/96	Changed DimValue() for a few extra % increase in performance
v4eval.c	1.249				Added caching to GetBindBuf - saves about 10-15% on big runs
vcommon.c	1.250	VEH	12/25/96	Added v_SpawnProcess to handle creation of subprocesses
v4isu.c		1.250				updated to use above
v4im.c		1.250				updated to use above
						aded v4im_CheckPtArg and changed handlers to use
						Enum() added as equiv to EnumCE()
						Tally() accepts While:xxx, Sort accepts While:xxx
						Select:xxx changed to If:xxx
v3operx.c	1.86	VEH	12/25/96	sys_spawn() changed to use v_SpawnProcess
v3iou.c		1.86				sys_print/sys_queue changed to use above
v4im.c		1.251	VEH	12/30/96	Added Self(), MakeP/I/QI/L() to replace Identity, MakePoint/Isct/QIsct/List
v4dpi.c		1.252	VEH	01/06/97	dim:=point same as MakeP(Dim:dim point)
v4defs.c	1.253	VEH	01/12/97	Added ctx->pi (ProcessInfo) & pi->ctx
v4defs.c	1.253				Defined DfltMaxAllowedErrs (=100) as default for max # errors before quitting
v4dpi.c		1.253				Added tag::pt capability
v4im.c		1.253				MakeP( Tag:xxx pt ) -> xxx::pt
v4im.c		1.253				Do( ) replaces Eval( ), does not eval arguments up front
v4im.c		1.253				EnumEU( list action:pt relation:pt ) where action=Sum/Sub... relation=LT/NE/EQ...
v4im.c		1.253				List( pt To:pt/Number:pt ) (ex: List(5 To:10) => (5 6 7 8 9 10)
v4im.c		1.253				List( list set:list) where set=Union/XUnion/Subtract/Intersect...
vcommon.c	1.254	VEH	1/20/97		Added v_IsBatchJob() module
v4dpi.c		1.254				UPeriods handle periods 90..99 for each year (adjustment periods)
						Cleaned up PointToString to make results easier to read
v4eval.c	1.254				Added If-Dimension-End (executes only if dimension is defined)
						Added cache clear to GetBindBuf() to resolve funky binding-not-being-seen problems
v4test.c	1.254				v4_error() outputs evaluation stack when possible as debugging aid
v4im.c		1.254				Sort() now takes Do:: argument: Sort(list By::xxx Do::isct)
						EchoS() now takes Count::Dim:xx - updates line count via that point in ctx
v4stat/v4fin	1.254				Added mess of Finxxx() & Statxxx() modules
v4dpi.c		1.255	VEH	2/11/97		Adding better cache purging, handling of new points to read-only dimension
		1.255				Handle both a.b.c and a.b .c .d properly
		1.255				Added v4dpi_ScopePntAlloc()
v4im.c		1.255				IfDefine now Def()
						EchoS( Skip::n ), took out FORMULA in StyleName::
						MakeP( dim pt To::pt pt Number::pt )
						LT, LE, EQ, IN, ... now take 2, 3, & 4 arguments
v4eval.c	1.256	VEH	2/25/97		Added V4DPI_PntType_ODBC & related parse/display/list handlers
						Added V4DPI_PntType_SSVal & related parse/display
v4dpi.c		1.256				Numeric literals of form: 0.nnn or nnnEnn become points on Default dimension
						Now handle "tag?" construct
v4imu.c		1.256				AggPut(Agg/FAgg/BFAgg:key pt pt ... ) added
						ODBCConnect/Val/Free/Get/ColInfo() modules added
						FinTVM & FinCoupon added to replace many smaller modules
						Tally(Count::n) now supported (temporarily lost in conversion to tags!)
v4im.c		1.257	VEH	3/11/97		DateInfo() -> DTInfo() & now uses tags instead of NId points, greater functionalilty
						Added FinDep(), StatDist(), StatDistInv() & took out individual intmods
						Def() handles single argument, returns T/F if arg defined/not-defined
						StatMode() fails if no mode found in list
v4dpi.c		1.258	VEH	4/25/97		Added V4L_ListTypeCmpndRangeDBL for floating point ranges, added Increment to double/int
						Fixed bug in PointToString for Dict:<>value
						Int/Double range lists display as start..end..increment if increment <> 1
v4im.c		1.258				List(start To::/Number:: By::) - By::increment added
							if Number:: given then end determined by increment
						List(list Dim::dim) - returns first point in list belonging to dimension dim
						Fixed StatQuartile, problems with BetaDist, NormalDist,
v4imu.c		1.258				VerifyList now converts single point to list of 1 point
v4isu.c		1.258				Added setvbuf() calls to disable buffers after fopen() calls
v4eval.c	1.259	VEH	5/31/97		Added "Set EchoS {SpreadSheed,Text}"
						Dimension types Delta & UYear
						All macros now held in local cache (don't need to Set MassUpdates)
						Better handling of multiple open aggregates
						Set Trace Prompt handler now takes "= [xxx]" command
v4ctx.c		1.259				Added support for long lists of doubles (like HugeInt)
						Increased number of active dimensions to > 1000
v4is.c		1.259				Added checks for unused buckets beyond end of file, better checks on unused buckets
						Checks for DataId overflow (only 12 bits)
v4mm.c		1.259				Added WriteVerify logic & global segments (file.wvb)
v4dpi.c		1.259				Took out automatic caching (not being called??)
						PointParse: -nnn now assumed to be UDelta:-nnn (not Int:-nnn)
v4im.c		1.259				Added "respt" logic to many routines (got almost 20% performance boost!)
						Added Bits(num {And,Or,Not,AndC,XOr} ...) intmod
						SSDim(Page::) now accepted
						EchoS(Rows::n Width::nn Width::9999 (for auto-size))
						Format(Round:: & Point?)
						Tally(Sample::n) now taken
						Tally(list (SetOf::xxx)) - same as ListOf except points not duplicated
						Tally(list (Values::[] ...)) - enumerates through sub list of points
						List(list Sample::) - code fixed up to work better
						DTInfo(dt {Year:: Month:: Day::} ...) - adjusts before returning result
						DTInfo(date Day::31 UDate?) - returns last day of month
v3prsc.c	1.87	VEH	7/1/97		V3 object names converted to V4 dimensions in output of FLD() macros for V4
v4ctx.c		1.261	VEH	7/1/97		Added V4IS list handling (see below)
v4defs.c	1.261				Added format #define for output of doubles: "%.15g"
						Added stuff for V4IS- structures, pnttype, listtype
v4dpi.c		1.261				Caching via: <a.b> or <[isct]> is handled
v4eval.c	1.261				Added Structure/Element commands
						Set Trace V4IS added
						Dim xxx V4IS added
v4im.c		1.261				Echox(): works with Before/After/Begin/End
						Enum(): takes Before/After/Begin/End tags
						Enum(): takes Index::Dim:xx option
						EnumCT(): works with 2,3, or 4 arguments (3rd is true-value, 4th is false-value)
						EnumCL(): takes Index::Dim:x
						Tally(): Values::num to set max number of tally combinations
						Tally(): better handling of dim* as tag values (e.g. By::IM*)
						RCV(): Added with various options
						V4ISCon/Val/Op: added for V4IS interface
						Matrix(): Calc:(list) added for "invisible" columns
						Matrix(): takes Before/After/Begin/End tags
						List(To::n): generates list of Int:1..n
						List(V4IS If::rec [If::subrec] Sample:: Number:: Length:: KeyNum:: Count::..): added
v4is.c		1.261				Better & more extensive handling of V4IS_PCB_NoError flag
v4mm.c		1.261				Added WriteVerBuf handling on bucket write/read with retry logic in bucket read
v3iou.c		1.88	VEH	7/24/97		iou_closeall now traps all errors & continues with next file
v4ctx		1.262	VEH	7/24/97		hash routine for dims now uses "&" instead of "%"
						nbl caching added to context add point
						context add - "slam" logic put in
						CLEARCACHE thrown in throughout, redefined to do nothing
v4dpi.c		1.262				{ exp } now parsed and converted to infix form
						date-time now accepts time of form hh:mm:ss
						x.y now displays as x.y (instead of [x* y])
						cache added IsctEval on grab of isct value
v4eval.c	1.262				better error recovery, advances to next command line (no indentation)
						Enum() returns Logical:True if 1st argument is empty list
						temp files used for Sort() & Tall() (instead of v4imsort.t, ...)
						DTInfo(udt Minutes::n) rounds to nearest n minutes
						DTInfo() returns argument if no tag? given
v4mm.c		1.262				Converted BktRead to use lseek/read
vcommon.c	1.262				vout_Text() now buffers when output to file
vconfig.c	1.262				typedef B64INT defined as __int64 or long depending on system
v4dpi.c		1.263		8/25/97		Added BigIsct point type to handle large iscts, hooks in IsctParse, IsctToString, IsctEval
						If isct eval fails then attempt [UV4:IsctFail] in context
v4isu.c		1.263				Dropped hash max to 250K & made it dynamic
v4defs.c	1.263				Default v4a bucket size now 0x4000 (16k)
v4im.c		1.263				EchoS() - Menu1/Menu2, Evaluate, Echo, Error
						SSDIM() - Justify::L/R/C
						Tally() - Parent::point & PIf::test
						Tally() - By:: is now optional
						EnumCL(... SetOf?) - returns set, not list
v4isu.c		1.263				Use tmpnam() for all temp files
vcommon.c	1.263				Windows "Set sym=value" now treated as logicals before testing registry
v4test.c	1.263				CPU now shown to 100th of second
v4eval.c	1.263				Cleaned up error handling, continuation of Binds must be indented
						Took out all Traceback stuff
vsort.c		1.12	VEH	9/8/97		Added support for Fixed keys
v4ctx.c		1.264	VEH	9/8/97		Added v4ctx_FrameHan() to allocate handles per frame & close on frame pop
v4im/u.c	1.264				LList(Dim:xxx/list If:: While:: Maximum:: Parent::xxx PIf:xxx Sample:: Trace::n/(n isct))
						EchoS() - Scale:: Page:: Header:: Footer:: LMargin:: RMargin:: TMargin:: BMargin:: Portrait::
						MakeT(IntMod:xxx arg1 ... argn)
						EnumCL() - Shell::Dim:x
						TallyM(Area:: Values:: (...))
						WHLink(Dim:xx Values::n Error::isct Point::pt)
						Tally() - Area::file
						List() - Shell::Dim:x with If/While
						List() - Collapse? added
						Enum() - Shell:: added
						MakeP(Dim:wh point) - converts point to corresponding point
						wh:=point (same as above)
						Added GetPointDbl/Int/UD/Log
v4eval.c	1.264				Dim xxx WormHole
						Dim xxx Fixed Decimals n
v4is.c		1.264				Added support for keys of type Fixed
v3prsb.c	1.89	VEH	10/1/97		Added #nn.n comment types
v4ctx.c		1.265	VEH	10/1/97		AreaAdd now tracks area file name
v4dpi.c		1.265				v4dpi_DictEntryEnum() added
						Added better parsing & displaying of months, quarters, periods, weeks
v4eval.c	1.265				Table name macro xxx Header n CSV Tab Embedded Comment x
						EndTable
						Column name dim Default pt Error pt Start n End n Width n Ignore Table xx
							FixedLength n Decimals n Format xxx
						Include file Table name
						If ValidPoint pt
						If Not xxx
						If MoreRecent file1 file2
v4im.c		1.265				DefQ() added, Def() no longer assumes first arg quoted
						EchoS() - Row::n Rows::n AutoFit::n Columns::n RowGap::n ColGap::n
						Spawn(command)
						Str(string Begin::n End::n Length::n)
						Str(string Dim:xxx) - true/false if string valid point in dim
						Str(string Has::substring) - position of sub in string or 0 if not found
						Str(string Trim? UC? Length?)
						Str(string Head::str/Tail::str) - true if string starts/end with str
						MakeP(Dim:dict alpha) - converts alpha to dict entry
						V4(Agg::file/Area::file Open::mode Share::mode)
						V4(Agg? or Area?) - list of open
						V4(ListOf::Dim:Dim, Dim:Nid, Dim:xxx)
						Tally() - Shell::dim, AWhile::test, Sum::pt
						Tally() sub - ByIf::test
						List() - Shell::dim, AWhile::test, Sum::pt
						DTInfo() - Hour? UTime?
						Enum() - AWhile::test, Shell::dim, Sum::pt
						EnumCL() - Shell::dim, AWhile::test, Sum::pt
v4eval.c	1.266	VEH	10/30/97	Column Format YYMMDD, MMDDYY, & DDMMMYY
						Column Width n - will determine Start position from prior if not given
						Column Ignore - now works
						Column Table - now works, table name is column name + column value
v4im.c		1.266				Is1(pt) - True/False if pt specifies single point
						Is1 - supports 2 & 3 argument modes
						EvalAE - points evaluated as [UV4:EvalAE pt]
						Str(dict ...) - converts dict to string
						List(list Select::xxx) - selects subset of list
						List(list Select::num..num) - range based on ge/le
						List(list Select::dict..dict) - range based on exact boundary mathces
v4ctx.c		1.267	VEH	12/10/97	Added FramePush() optimizations, ListType_BitMap routines added
v4defs.c	1.267				DIMVAL & DIMINFO macros added to cut down on proc calls
						Added UTime dimension (made same as Time), Time now the more abstract time
v4dpi.c		1.267				"//" & "%" added to { point } parsing
						dim:.. now allowed (same as dim..)
						UTime:hh:mm:ss parsing & display
v4eval.c	1.267				Dim name List Entries dime
						Dim name,name,... now allowed
						Set Parameter x += y
v4im.c		1.267				DDiv added (returns result as double)
						And/Or/Not now operate on lists
						EvalAE - 2nd argument now optional
						EvalLE - added
v4imu.c		1.267				In(pt bitmap-list) supported
						In(udt utime) now supported
						DTInfo(utime Minutes::n) supported
						V4ISOp() - returns bitmap list if V3 field is alpha
						 & dim is list with Entries attribute
						V4ISOp() - Alpha/Int/Dict V4 types -> V3 alpha keys properly
v4isu.c		1.267				TEMPFILE() macro added for handling temps on different OS's
v4defs.c	1.268	VEH	1/7/98		Added UTime dim type to differentiate from TIME
						DIMINFO/DIMVAL macros added to cut down on calls
						BitMap list defs
v4dpi.c		1.268				Added dictionary file support
						// -> DDiv, % -> Percent in {...} parsing
						dim:.. now treated same as dim..
						UTime parsing
v4eval.c	1.268				Dimension name Entries dim - for bitmap lists
						Area xxx name Dictionary - creates/read dictionary
						Better filename parsing
v4im.c		1.268				DDiv() - returns double/floating result
						And/Or/Not() now support bitmaps
						AggVal( ... Nth::/LE::/.../GT::n ) now supported
						EvalLE('bitmap expression) - similar to EvalAE but for bitmaps
v4imu.c		1.268				TEMPFILE macros added
						Bits( ... Num::value ) - returns value if result to-date is <> 0
						V4ISOp() - alpha keys - Ints converted to zero-fill, Dicts converted to alpha
						Enum(Context::pt)
						EnumCL(Column::n) - sets current column to n - for RCV( +/- )
v4isu.c		1.268				TEMPFILE support
vcommon.c	1.268				Added routines for filename parsing
vconfig.c	1.268				TEMPFILE macros defined
vsort		1.13				Fixed bugs in sorting variable length text files
v4ctx.c		1.269	VEH	2/5/98		Added handlers for list type TextTable
v4defs.c	1.269				TextAgg definitions
v4dpi.c		1.269				isct/pt -> EnumCL(pt @isct)
v4eval.c	1.269				Table - Date type DDMMMYY handled
						Set OutputFile file & Set Console to redirect output
v4im.c		1.269				SSDim - can now handle multiple dims in same call
						Mult/Div/DDiv/Percent/PCChgx - optional 3rd argument: Round::n
						Abs() added
						Sign() added
						StatRan(To::n Type::n Seed::n) added
						MakeP(aggval pt1 pt2 ...) now supported
						List(Dim:AggRef Area::xxx Table::xxx) returns TextTable list
						List(list Minimum/Maximum::pt)
						List(list To::pt) - returns elements of list up thru pt
						Tally() supports With::isct & WBind::isct
v4test.c	1.269				-i file & -ii (defaults to v4.ini)
v4ctx.c		1.270	VEH	3/3/98		IsA points now handled via UV4:IsA
v4dpi.c		1.270				New PntType- IMArg - default is Arg:n
v4eval.c	1.270				Added optimization: Set ScanForDict (works fast with tables)
						(BF)Agg< xxx > parsing - is xxx is isct then evaluate
v4imu.c		1.270				Added GetPointChar routine
						Matrix() - RowCap::(list) - each element of list becomes row caption column
						List(xxx Append::yyy) - now creates copy of list
						Tally( Id::name Do::isct ) - 2 new arguments added
v4ctx.c		1.271	VEH	5/15/98		Added FrameCacheClear
						Support for Huge Generic Lists
v4defs.c	1.271				V4DPI__Point - got rid of CacheIt, added AltDim
v4dpi.c		1.271				Got rid of internal Dim_Dim - now declared in v4kernel.inc
						Dimension synonyms - [UV4:DimSynonym NId:xxx]
						{? xxx } now supported
						{xxx}/list -> EnumCL()
						dim's point -> dim.. point on LH, dim.point on RH
						FChar support added for formating within Excel formulas
						mm/dd/yy parsing of dates
						NoCtxIsAll flag added to IsctEval (if not in ctx then assume dim..)
v4eval.c	1.271				Loop #tablename#, Loop n added
						LogToIsct command added
						EndLog added
						Smart End (End-> End, EndTable, EndLog as needed)
						File name parsing vastly improved
						CBind command taken out
						In "Point dim xxx" if xxx is current table column name then substitute
						Support for AggUpd, AggDel
						Aggregates- < #tablename# >
v4fin.c		1.271				FinIRR - support for Enum:: First:: lists & Values::
v4im.c		1.271				EchoE(), EchoN() added
						Echo & EchoS - If:: added
						SSVal(), SSULC(), SSRow(), SSCol(), SSExp() added
						EchoS() - If::log, X::offset, Y::offset, Macro::name, UpdateOK?, UpdateOK::n
						EchoS() - FChar support
						Context() - takes multiple args, last one returned as value
						Plus/Minus() - last arg is dim of result unles Dim:Int
						MthMod() added
						OSExt(Library::name Module::name ...) added
						Sum() - dim of result based on calculated result
						MakeP - alpha->point, point->alpha
						MakeP() - now accepts multiple dims- MakeP(Dim:x,y "xxx")
						EvalAE() - if first arg not alpha then EvalAE(x dim:y) -> MakeP(dim:y x)
						EvalUM() - see NoCtxIsAll
						EvalBL - added Number::n Nth::n & MatchUndef?
						Matrix - handling of row captions improved
						List() - Added Every::n, Begin::pt
						DTInfo() - added uperiod support
						Sort() - Shell::dim added
						Enum() - added Num::n, Calc::([x] [y] ...), Every::n
						EnumCL() - added Num::n
						Tally() - added Dim:x to global & local for results
						Str() - Head/Tail::(list) - returns index matching entry
vcommon.c	1.271				Table Columns - context points, Int accepts ranges, Real accepts nEn notation
v3iou.c		1.90	VEH	8/15/98		Fixed bug in TCP/IP recv() of zero length messages
						tty_misc(108,width) - flips font size on WinNT
v4dpi.c		1.272	VEH	8/15/98		<> is now set to NEk(), not NE() in {xxx} parsing
v4im.c		1.272				EchoS() - Sheet/RowColor/ColColor/CellColor/X/Y tags now supported
						Transform(pt First/Last/All)
						tagged arguments: tag::`isct - force eval if prefix with "`"
						Tally() - Count/Skip/Index to select points for debugging
v3defs.c	1.91	VEH	12/23/98	Added extra field (FileType) to FileInfo structure
v3edtu.c	1.91				Minor changes to force 2 digit year on itx_xxx date routines
v3iou.c		1.91				Changed WSAStartup to work under NT 4.0
v3mthu.c	1.91				Added better floating point support
v4defs.c	1.274	VEH	12/23/98	LockTablePidMax increased from 150 to 300 - BEWARE on upgrades
v4dpi.c		1.274				dim:=pt now via Coerce()
v4eval.c	1.274				Added CAggregate command (constant points in area)
						increased bucket size of aggregate areas to 0xFA00
						Added Column attribute "AggKey" (ignored in Loop #table# xxx)
						Added Area attribute "Scan" to optimize seq scan of agg area
						Set Parameter x `isct - isct is evaluated
v4im.c		1.274				SSExp(Cell::@isct) - if isct eval fails then SSExp fails
						EchoS(Object::file)
						EchoS(RowColor, ColColor, CellColor) - if arg < 0 then ignored
						Div(), Percent(), Mod(), PCCHgx() - added Error::value
						Coerce() added to coerce from one dim to another (MakeP() calls it)
						AggVal(pt Dim:xxx) - if only 2 arguments then agg constant value
v4imu.c		1.274				GetPointChar - now coerces list to multi-line char
						RCV(Dim:xxx) - returns value as point on dim
						LList(Include::Dim:xx) - points on Dim:xx also included in list
v4isu.c		1.274				v4is_Open() now uses CreatFile() (Windows only) for more options
v4mm.c		1.274				Bucket read/writes on Windows now support 64 bit disk offset addressing
v4ctx.c		1.275	VEH	1/7/99		WHInit/Put - handles IntDbl in addition to IntInt
v4dpi.c		1.275				Fixed problems with Dictionary load, optimized by replacing sprintf() calls
						dim+ - similar to dim:{new} except new point created with each reference
v4im.c		1.275				Added BindQQ() - both arguments quoted
						WHLink(Hold?) - if WH added to context then added to global context
vcommon.c	1.275				logical decoding - handle multiple directories for wild card searches
						logical decoding - looks for dir/file even with new file
v3defs.c	1.92	VEH	3/29/99		Increased assertion max (causing random blowups for those loading lots of pcks)
v3iou.c						Added /lock_wait/ xface to V4IS
						Changed handling of date-time for future batch submits under UNIX
v3v4.c		1.92				Added unit_EXISTS error trapping
v3vax.c		1.92				Added unit_EXISTS error trapping
v4dpi.c		1.276	VEH	3/29/99		Better handling of integer literals (check for 32 bit overflow)
v4eval.c	1.276				Area command now accepts list of files
v4im.c		1.276				Better handling of SSExp values (e.g. embedded quotes)
						EchoS supports End::string  /  Note::string  / Message::string
						TQ(test value test value ... value)
						Minimum/Maximum() - if 1 arg then list else min/max of arguments
						Bind() -> BindQQ/QE/EE/EQ()
v4imu.c		1.276				GetPointDbl/Char/Int/... now handles dim*
						Enum(Num::x) - set limit of x even if list is specified
						EnumCL(Context::)
						Tally() entries support Cache::num or Cache::0.num
						Tally() entries support IfA/B/C::test & IfA/B/C?
v4is.c		1.276				Lock waiting now supported in v4is_Get()
v4isu.c		1.276				Added FSEEKINT typedef to support >2^32 byte addresses in UNIX fseek()
v4mm.c		1.276				Changed "Lock table appears hung" warning, fseek() byte addressing
v4stat.c	1.276				Fixed floating overflow problem in Poisson distribution
						Added Poisson? to StatDistInv module
vcommon.c	1.276				Better handling of numeric literals (overflow checking)
						Better error handling in Table parsing
v3defs.c	1.93	VEH	7/1/99		Increased Branch path max to 500
v3iou.c		1.93				Handle ip address (n.n.n.n) as host name
						io_misc(/fill_binary/, /fill_text/)
v3oper.c	1.93				sys_info(/os/) returns WINDOWS/WINNT
v4dpi		1.277	VEH	7/1/99		Lists are not expanded when entered into context unless forceval(`)
						dim** - returns prior context value
						Added Telephone point type
						Dimension - Acceptor / Displayer flags now enabled
						[UV4:Acceptor Dim:xxx Alpha:xxx] - when pointaccept fails
						Dimension xxx IsA dim
						Dimension - NoExpand attribute gone
						Table Delimiter "x"
						Table Defaults - supplies defaults by dimension
						Column NPTable - does not include column name as part of table
						Column Trim - trims spaces
						Column BaseYear yyyy - Sets base year for UPeriod
						Column Prefix/Suffix "xx"
						Column Offset n Scale n.n
						Column Skip n - skips n columns (this one is also ignored)
						Column Format DDMMYY
v4im.c		1.277				SSDim(PageBreak) (Page deleted)
						EchoS(Id::xxx Include::file PageBreak:n)
						Do(End::logical) - jumps out if true
						IsctVals(isct 0) - returns no-expanded value of isct
						TEQ(value test res test res .... res)
						Minimum/Maximum - handles single list arg or multiple regular args
						MakeP(dim:dict pt pt pt...) - works
						Int:=dict - converts to numeric if possible MakeP(int dict) - copies dict interal value
v4imu.c		1.277				Transform(First? or Last?) - overrides default first/last
						DTInfo() - UWeek support
						Bits(Result::xxx) - instead of Num:xxx
						Sort(Num::max) - returns max number of points
						Enum(First/Last::xxx) - evaluates xxx on first/last point
						V4(Agg::xxx NoError?) - does not generate error if xxx not found
						V4(Dim:xxx Binding:nn) - sets binding value locally
v4isu.c		1.277				Added check for deadly embrace
v3edtu.c	1.93	VEH	9/17/99		Rounding problem for TODAY/TOMORROW xti
v3iou.c		1.93				Added /File_Text/ for io_put to Inet
v4ctx.c		1.278	VEH	9/17/99		Bind key now be AggRef
						Huge Lists support AggRefs
						Multi-Append lists supported (append n lists without appending each element)
v4dpi.c		1.278				Support for UV4:DimToList (convert dim.. to list via evaluation)
v4eval.c	1.278				DIMENSION command - Added DotDotToList & Description, removed Macro
						COLUMN - added description
						#x# in Loop = column description
v4im.c		1.278				Plus() with no args returns empty string
						Sort() supports Parent/PIf tags
						Tally() supports Shell tag
						V4(Dim:xxx Description?) - returns dimenson description or null
v3edtu.c	1.93	VEH	10/1/99		Added "#include <time.h> to correct bug in OpenVMS handling of time()
v4eval.c	1.279	VEH	10/15/99	Changed logic in AllValue so that only used if no other points have bind potential
v4im.c		1.279				TEQ() handles Dim:<value, Dim:>value, ...
						List(list Nth::(list of numbers)) now supported
						RCV() defaults to level 2 unless in End:: then defaults to 1
						Sort(Parent/PIf) supported
						Enum(Do::isct) - instead of @isct
						Enum(Try::isct) - replaces EnumCP(), enums until isct evals
						Enum(None::isct) - if no points in first argument, then returns isct
						EnumCL(Do::isct)
						EnumCL(Evaluate?) - forces additional eval of 2nd argument if in list
						Tally/TallyM(UCount::isct) - returns count of uniqe entries
v4mm.c		1.279				Bucket reads past 2^31 bytes supported on Alpha/Unix
vcommon.c	1.279				Change to handling of Set Echo n command
v3prsb.c	1.94	VEH	03/01/00	Added #{(symbol xxx)}# & #{(module xxx)}#
v4dpi.c		1.280	VEH	03/01/00	Fixed bugs in {arith-exp} parsing, ~ (not) now handled
						Added optimizations for logical ands/ors/plus/... (multi arg, quoted args)
						XL:Row/Column added to reference current row/column
						Fixed problem with dimension with Displayer in trace statements (recurse to oblivion)
						Decomp point will now match on empty set in Isct eval
v4eval.c	1.280				Dimension FILENAME attribute - string parsed as filename
						If {TRUE, FALSE, EXISTS file} now supported
						Set Trace DECOMPOSITION/ISCTFAIL added (PULLCONTEXT removed)
						Tracking of Aggregate I/O gets/puts
v4im/u.c	1.280				EchoX() started
						Moved EchoS() to v4imu
						Do(XML::id) supported
						Minus() - now handles unary arithmetic/logical operations
						MatrixX() intmods deleted- use EchoX
						TEQ() - beefed up support to handle more PntTypes in more variations
						Average() - added
						MakeP(Dim:Dim dim:{special}) - now works
						MakeP(Dim:shell ...) - now works
						UDate:=integer - if yyyymmdd then converts otherwise assumes internal format
						Pack4x16 removed, PackInt2 added
						WHLink(Dim:dim pt) - same as WHLink(Dim:dim Error::pt)
						Tally(Bits::num) - number of bits for UList/UCount operations
						Tally(Bind::pt) - if pt not an isct, will convert
						List(list Bits::n) - returns sub list of entries with bits set in n
						Trig(Sin::x, Sin?, Cos::x, Cos?, Tan::, Tan?, Degrees::, Degrees?, Radians::, Radians?)
						Enum(XML::tag) - supported
						Str() - concats args like Plus()
						V4(pt All?) - returns true if pt is dim..
						V4(pt Type?) - returns type of point (e.g. INTEGER, UDATE, REAL, ...)
						V4(pt Attributes?) - returns Dimension attributes for pt
v3iou.c		1.94	VEH	05/10/00	Added /NOBLOCK/ to unit file_info & io_misc(unit,/BLOCKING/,yes/no) for IP sockets
v4defs.c	1.281	VEH	05/10/00	Added UOM stuff, got rid of dead wood in DimInfo block
v4dpi.c		1.281				Process shutdown routines to release allocated memory
						UOM mods in PointAccept, PointDisplay
						UOM init/coercion routines
						  [UV4:UOMInitialize Int:num] & [UV4:UOMCoerce UOMRef:n UOMIndex:n UOMpt:xxx]
						dim:=point now uses Project()
						handler for MaxNest [UV4:MaxNest UDim:isct]
v4eval.c	1.281				Dimension ADPoint pt - Point used in Acceptor/Displayer calls (for tables)
						Dimension UOMId num - associates uom id number with dimension
						Column UOM num/name
						Column Default - now pads out so defaults don't have to match FixedLength parameter
						Column Format Hex4
						New dimension types UOM & UOMPer
v4im.c		1.281				Many module changes to support UOM/UOMPer (+ - * / // relationals, min/max)
						Pack1616/Pack248 - only take 2 arguments now
						MakeP(Dim:uom num id index [casecount])
						MakeP(Dim:uomper num uom)
						Project module added (replaces Coerce)
						date:=month, year:=date, month:=year, .... - now creates list of points
						PackInt2() - 2, 3, and 4 argument modes
v4imu.c		1.281				Various mods for UOM/UOMPer
						List(Group::isct) builds up temp list & tests isct
						List(Head::-n) now supported
						Bits(string Nth::>32) - supports bit strings (see ^4dddd & Hex4)
						Tally( (Bits::n)) - specifies max number of bits for caches
						UOM() handler
						Str(ListOf::"breakcharacters")
						V4(NoError?) - as single arg used with NestMax trapping
						V4(pt All?) - True if pt is dim..
						V4(Context?) - returns list of current context
						V4(Type::pt)/V4(dim Type?) - returns V4 point type (e.g. INTEGER) of point
						V4(Attributes::pt)/V4(dim Attribute?) - dimension attributes
						V4(Binding::pt)/V4(dim Binding?) - binding number
						EchoS(UpdateOK -> Update)
						EchoX() module added
v4mm.c		1.281				Memory release logic
vcommon.c	1.281				Radix ^4ddd (binary string as list of 4 byte hex numbers)
						Table parsing - better error reporting
						UOM/UOMPer in Table parsing
						Hex4 parsing
						Memory allocate/deallocate - use MemDebug compile option for tracking memory leaks & overruns
v3iou.c		1.95	VEH	7/24/01		Added setsockopt(SO_REUSEADDR) on open of server internet socket
v3iou.c		1.96	VEH	3/20/01		Added io_misc(unit,BOF) to set C files to BOF
v4eval		1.290	VEH	3/20/01		Modified Aggs to handle compression - ALL Agg files MUST BE REBUILT
v3edtu.c	1.97	VEH	4/23/03		Made call to mscu_minutes_west dynamic to take care of daylight savings time changes

v4mm.c		1.304	VEH	4/23/03		Put back some locksomething code to take care of hung locks
v4imu.c		1.307	VEH	9/7/03		Fixed problem in aligning sort records on Alpha, added PRUpd() module
vcommon.c	1.308	VEH	9/20/03		Modified token generate to treat 'nxxx' as keyword, not numeric error
v4imu.c		1.308	VEH	9/22/03		Modified XMLSTART/XMLEND to ignore 0-length entries
v4eval.c	1.308	VEH	9/24/03		Added "Context ADV dim value" command & changed v4wwwfesever to use it
v4imu.c		1.308	VEH	9/27/03		Added Enum::list option to Enum()
v4stat.c	1.308	VEH	9/28/03		Added Prob::(list) option to StatRan()
v4im.c		1.308	VEH	10/04/03	Added EchoE(URL::xxx) option (also to vrestohtml)
v4imu.c		1.309	VEH	10/20/03	Added support for SegBitMap - List(), Tally(), Project()
v4imu.c		1.309	VEH	10/20/03	Added support for List(Union?/Intersect?... list list list...)
v4imu.c		1.309	VEH	10/23/03	Added List(list Columns::n)
v4stat.c	1.310	VEH	11/19/03	Fixed problems when StdDev=0 in several stat modules
v4imu.c		1.310	VEH	11/19/03	Added Break:: to Str()
v4stat.c	1.311	VEH	12/15/03	Various problems with StatLinFit()
vcommon.c	1.311	VEH	12/15/03	Added VCAL_BadVal instead of UNUSED for bad dates
v4dpi.c		1.311	VEH	12/16/03	Ability to define special point names/value (e.g. UDate:TBD)
v4im.c		1.312	VEH	12/30/03	Changed comparison logic (< <= = > >= <>) to differentiate between int, real, fixed
v4dpi.c		1.313	VEH	2/2/04		Added "Set Classic", changed handling of keyword from prefereddim:keyword to NId:keyword ALWAYS
v4imu.c		1.313	VEH	2/2/04		Added links between Str() & List() module so args can be from both
vcommon.c	1.313	VEH	2/2/04		Added Filter option to "Include table" command to preprocess input lines
v3operx.c	1.98	VEH	1/05/04		Added sys_info(/environment/,"xxx") to return environment variables
v3oper.c	1.99	VEH	2/20/04		Added v3_copyswap() module for port from HP to Linux/PC
v4dpi.c		1.314	VEH	3/05/04		Can now do dim*,another-point
v4imu.c		1.315	VEH	4/04/04		Added Names() module, v4NameInfo.v4i
v*.c		1.315	VEH	4/04/04		Added CHARPNTBYTES2(pnt,len) & CHARPNTBYTES1(pnt) macros for setting up CHAR points
v4dpi.c		1.316	VEH	4/12/04		Added DotIndex attribute to DIMENSION command, enables list.pt to convert to List(list* Nth::pt)
v4imu.c		1.316	VEH	4/14/04		Spawn() & EvalCmd() concatenate multiple args into single string
v4defs.c	1.318	VEH	8/13/04		Increased file max from 50 -> 75
v4eval.c	1.319	VEH	9/11/04		Context ADV handles int:<return> and num:<return> as int:none/num:none
v4imu		1.319	VEH	9/11/04		int:none/num:none return Dim NONE xxx values
v4common.c	1.320	VEH	9/20/04		Expanded allowed date/udt syntax, incorporated in v4languageinfo.v4i
v4dpi.c		1.320	VEH	9/20/04		Expanded date-time syntax
vcommon.c	1.321	VEH	10/18/04	Added v_LogicalDecoder (returns error instead of v4_error())
v4imu.c		1.321	VEH	10/18/04	List(list Alpha? / Alpha::"xxx") - converts list to Alpha string
v4im.c		1.321	VEH	10/18/04	Added EchoW() / EchoA() modules
v4eval.c	1.321	VEH	10/18/04	Added Area Reset for reset of dictionary entries
v4eval.c	1.322	VEH	11/06/04	Added v4im_GetPointFileName() to suck up files (if multiple points only take first)
v4eval.c	1.322	VEH	11/06/04	Allow Columns to be specified with just Start (pull width from next Start)
v4im.c		1.323	VEH	12/1/04		Added new capabilities to SSDim()
v4eval.c	1.323	VEH	12/10/04	Include [isct] implemented
v4feserver	1.323	VEH	12/11/04	Handling of _V4=Menu to cut down on unnecessary V4 work in displaying top-level menu
v4imu.c		1.324	VEH	12/17/04	Fixed bug in Tally() hashing of entries during calculations
v4imu.c		1.325	VEH	1/4/05		Added EnumX & supporting code, took Else::pt out of Enum()
v4eval.c	1.326	VEH	1/15/05		Added OnError option to Area command
v4extern.c	1.327	VEH	2/14/05		Added MIDAS-ODBC support
v4imu.c		1.327	VEH	2/14/05		Removed IsctVals() module & replaced with Eval() module
v4mm.c		1.328	VEH	3/3/05		Fixed AreaFlush to prevent attempts to flush already empty bucket
v4imu.c		1.328	VEH	3/3/05		Cleaned up various date-time routines - added parameters, check bounds, etc.
v4imu.c		1.329	VEH	3/15/05		Added Flatten() command & update to bind routines
v4extern.c	1.329	VEH	3/16/05		Changed OSFile() to return confirmation message instead of Logical:True
v4dpi.c		1.329	VEH	3/21/05		Handled embedded BigIsct points within Isct - parsing & IsctEval
v4common.c	1.330	VEH	3/24/05		Wrote v_MakeOpenTmpFile() to replace TEMPFILE macro
vconfig.c	1.330	VEH	3/24/05		Got rid of TEMPFILE macro
v4imu.c		1.330	VEH	3/24/05		Better handling of int/real:NONE values
v4test.c	1.331	VEH	4/06/05		Created v4_ExitStats so that error abort will still output processing statistics
v4extern.c	1.331	VEH	4/06/05		Fixed up MIDAS-SQL handlers, wrote vsqlserver.c (taken from KBD feserver
vcommon.c	1.331	VEH	4/06/05		Modified WINNT v_IsBatchJob so that piped process not considered batch
vcommon.c	1.331	VEH	4/06/05		Modified v_MakeOpenTmpFile so that V4 always assigns temp files, keep track of them & delete in V4 process exit
v4imu.c		1.332	VEH	5/10/05		Added URL2/3/4/5 & URLBase2/3/4/5 to EchoS()
v4extern.c	1.332	VEH	5/10/05		Added Bits/Name/OS/ComFileExt/Endian/Processors to OSInfo()
v4imu.c		1.332	VEH	5/10/05		Added Counter() module
v4imu.c		1.332	VEH	5/10/05		Added Prefetch to Tally() (still needs some work I think)
v4imu.c		1.336	VEH	10/6/05		Converted to new EnumCL(), Geo() additions, UOMperUOM pointtype
*/
