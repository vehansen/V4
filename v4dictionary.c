/*	v4dictionary.c - Common Keyword/Command Dictionary		*/

#define ALLDICTENTRIES \
	DICTIONARYENTRY(EqualSign_, "*=*" ) \
	DICTIONARYENTRY(32, "32" ) \
	DICTIONARYENTRY(64, "64" ) \
	DICTIONARYENTRY(Abort, "Abort" ) \
	DICTIONARYENTRY(AbortRetryIgnore, "AbortRetryIgnore" ) \
	DICTIONARYENTRY(Acceptor, "Acceptor" ) \
	DICTIONARYENTRY(AcceptorFail, "AcceptorFail" ) \
	DICTIONARYENTRY(AcceptorTele, "AcceptorTele" ) \
	DICTIONARYENTRY(Add, "Add" ) \
	DICTIONARYENTRY(AddContext, "AddContext" ) \
	DICTIONARYENTRY(ADPoint, "ADPoint" ) \
	DICTIONARYENTRY(ADV, "ADV" ) \
	DICTIONARYENTRY(AF, "AF" ) \
	DICTIONARYENTRY(After, "After" ) \
	DICTIONARYENTRY(Agg, "Agg" ) \
	DICTIONARYENTRY(AggKey, "AggKey" ) \
	DICTIONARYENTRY(AggRef, "AggRef") \
	DICTIONARYENTRY(Aggregate, "Aggregate" ) \
	DICTIONARYENTRY(AggUpdDel, "AggUpdDel" ) \
	DICTIONARYENTRY(AggVal, "AggVal") \
	DICTIONARYENTRY(AJAX, "AJAX") \
	DICTIONARYENTRY(All, "All" ) \
	DICTIONARYENTRY(AllCnf, "AllCnf" ) \
	DICTIONARYENTRY(Alll, "Alll" ) \
	DICTIONARYENTRY(AllValue, "AllValue" ) \
	DICTIONARYENTRY(Alpha, "Alpha") \
	DICTIONARYENTRY(Ampersand, "Ampersand" ) \
	DICTIONARYENTRY(Any, "Any" ) \
	DICTIONARYENTRY(Append, "Append" ) \
	DICTIONARYENTRY(Application, "Application" ) \
	DICTIONARYENTRY(Area, "Area" ) \
	DICTIONARYENTRY(AreaAll, "AreaAll" ) \
	DICTIONARYENTRY(AreaName, "AreaName" ) \
	DICTIONARYENTRY(AreaRename, "AreaRename" ) \
	DICTIONARYENTRY(AreaUpdate, "AreaUpdate" ) \
	DICTIONARYENTRY(Arg, "Arg" ) \
	DICTIONARYENTRY(Argument, "Argument" ) \
	DICTIONARYENTRY(Arguments, "Arguments" ) \
	DICTIONARYENTRY(Arith, "Arith" ) \
	DICTIONARYENTRY(Array, "Array" ) \
	DICTIONARYENTRY(ASCII, "ASCII" ) \
	DICTIONARYENTRY(AsIs, "AsIs" ) \
	DICTIONARYENTRY(AtSign, "AtSign" ) \
	DICTIONARYENTRY(Attributes, "Attributes" ) \
	DICTIONARYENTRY(AutoContext, "AutoContext" ) \
	DICTIONARYENTRY(AutoIsct, "AutoIsct" ) \
	DICTIONARYENTRY(AutoStep, "AutoStep" ) \
	DICTIONARYENTRY(BackQuote, "BackQuote" ) \
	DICTIONARYENTRY(BackSlash, "BackSlash" ) \
	DICTIONARYENTRY(Base64, "Base64" ) \
	DICTIONARYENTRY(BaseDate, "BaseDate" ) \
	DICTIONARYENTRY(BaseYear, "BaseYear" ) \
	DICTIONARYENTRY(BC, "BC" ) \
	DICTIONARYENTRY(Before, "Before" ) \
	DICTIONARYENTRY(BFAggregate, "BFAggregate" ) \
	DICTIONARYENTRY(Big, "Big") \
	DICTIONARYENTRY(BigText, "BigText") \
	DICTIONARYENTRY(Binary, "Binary") \
	DICTIONARYENTRY(Bind, "Bind" ) \
	DICTIONARYENTRY(BindContext, "BindContext" ) \
	DICTIONARYENTRY(BindEval, "BindEval" ) \
	DICTIONARYENTRY(Binding, "Binding" ) \
	DICTIONARYENTRY(BindingUpd, "BindingUpd" ) \
	DICTIONARYENTRY(BitMap, "BitMap" ) \
	DICTIONARYENTRY(BK, "BK" ) \
	DICTIONARYENTRY(Blank, "Blank" ) \
	DICTIONARYENTRY(Blanks, "Blanks" ) \
	DICTIONARYENTRY(BM, "BM" ) \
	DICTIONARYENTRY(BO, "BO" ) \
	DICTIONARYENTRY(Body, "Body" ) \
	DICTIONARYENTRY(BOF, "BOF" ) \
	DICTIONARYENTRY(Bold, "Bold" ) \
	DICTIONARYENTRY(Bottom, "Bottom" ) \
	DICTIONARYENTRY(Break, "Break" ) \
	DICTIONARYENTRY(BS, "BS" ) \
	DICTIONARYENTRY(Bucket, "Bucket" ) \
	DICTIONARYENTRY(BucketSize, "BucketSize" ) \
	DICTIONARYENTRY(Buffer, "Buffer" ) \
	DICTIONARYENTRY(Bytes, "Bytes" ) \
	DICTIONARYENTRY(C, "C" ) \
	DICTIONARYENTRY(CA, "CA" ) \
	DICTIONARYENTRY(CAggregate, "CAggregate" ) \
	DICTIONARYENTRY(Calendar, "Calendar" ) \
	DICTIONARYENTRY(Cancel, "Cancel" ) \
	DICTIONARYENTRY(CaseSensitive, "CaseSensitive" ) \
	DICTIONARYENTRY(CC, "CC" ) \
	DICTIONARYENTRY(CDATA, "CDATA" ) \
	DICTIONARYENTRY(CDRom, "CDRom" ) \
	DICTIONARYENTRY(Cell, "Cell" ) \
	DICTIONARYENTRY(CellFormat, "CellFormat" ) \
	DICTIONARYENTRY(Center, "Center" ) \
	DICTIONARYENTRY(CG, "CG" ) \
	DICTIONARYENTRY(Characters, "Characters" ) \
	DICTIONARYENTRY(Child, "Child" ) \
	DICTIONARYENTRY(Chinese, "Chinese" ) \
	DICTIONARYENTRY(CK, "CK" ) \
	DICTIONARYENTRY(CL, "CL" ) \
	DICTIONARYENTRY(Classic, "Classic" ) \
	DICTIONARYENTRY(Clipboard, "Clipboard" ) \
	DICTIONARYENTRY(Close, "Close" ) \
	DICTIONARYENTRY(Closest, "Closest" ) \
	DICTIONARYENTRY(CMask, "CMask" ) \
	DICTIONARYENTRY(CN, "CN" ) \
	DICTIONARYENTRY(CNumLiteral, "CNumLiteral" ) \
	DICTIONARYENTRY(CO, "CO" ) \
	DICTIONARYENTRY(CodedRange, "CodedRange" ) \
	DICTIONARYENTRY(Col, "Col" ) \
	DICTIONARYENTRY(ColFormat, "ColFormat" ) \
	DICTIONARYENTRY(Colon, "Colon" ) \
	DICTIONARYENTRY(ColonColon, "ColonColon" ) \
	DICTIONARYENTRY(ColonColonColon, "ColonColonColon" ) \
	DICTIONARYENTRY(ColonEqual, "ColonEqual" ) \
	DICTIONARYENTRY(Color, "Color") \
	DICTIONARYENTRY(Column, "Column" ) \
	DICTIONARYENTRY(ColumnInfo, "ColumnInfo" ) \
	DICTIONARYENTRY(Columns, "Columns" ) \
	DICTIONARYENTRY(Com, "Com" ) \
	DICTIONARYENTRY(Comma, "Comma" ) \
	DICTIONARYENTRY(CommaComma, "CommaComma" ) \
	DICTIONARYENTRY(Comment, "Comment" ) \
	DICTIONARYENTRY(Comments, "Comments" ) \
	DICTIONARYENTRY(Compares, "Compares" ) \
	DICTIONARYENTRY(Compiling, "Compiling" ) \
	DICTIONARYENTRY(Complexx, "Complex") \
	DICTIONARYENTRY(CondEvalFail, "CondEvalFail" ) \
	DICTIONARYENTRY(Console, "Console" ) \
	DICTIONARYENTRY(Contents, "Contents" ) \
	DICTIONARYENTRY(Context, "Context" ) \
	DICTIONARYENTRY(Continent, "Continent" ) \
	DICTIONARYENTRY(Continue, "Continue" ) \
	DICTIONARYENTRY(Continued, "Continued" ) \
	DICTIONARYENTRY(Country, "Country") \
	DICTIONARYENTRY(CRC, "CRC" ) \
	DICTIONARYENTRY(Create, "Create" ) \
	DICTIONARYENTRY(CreateIf, "CreateIf" ) \
	DICTIONARYENTRY(CS, "CS" ) \
	DICTIONARYENTRY(CSelect, "CSelect" ) \
	DICTIONARYENTRY(CSpan, "CSpan" ) \
	DICTIONARYENTRY(CSV, "CSV" ) \
	DICTIONARYENTRY(Currency, "Currency" ) \
	DICTIONARYENTRY(Current, "Current" ) \
	DICTIONARYENTRY(CV, "CV" ) \
	DICTIONARYENTRY(CW, "CW" ) \
	DICTIONARYENTRY(D, "D" ) \
	DICTIONARYENTRY(DashDashRangle, "DashDashRangle" ) \
	DICTIONARYENTRY(Dashed, "Dashed" ) \
	DICTIONARYENTRY(DashRangle, "DashRangle" ) \
	DICTIONARYENTRY(Data, "Data" ) \
	DICTIONARYENTRY(DataEl, "DataEl") \
	DICTIONARYENTRY(DateTime, "DateTime") \
	DICTIONARYENTRY(DayOfWeek, "DayOfWeek") \
	DICTIONARYENTRY(DB, "DB") \
	DICTIONARYENTRY(DC, "DC") \
	DICTIONARYENTRY(DDMMMYY, "DDMMMYY" ) \
	DICTIONARYENTRY(DDMMYY, "DDMMYY" ) \
	DICTIONARYENTRY(Debug, "Debug" ) \
	DICTIONARYENTRY(Decimal, "Decimal" ) \
	DICTIONARYENTRY(Decimals, "Decimals" ) \
	DICTIONARYENTRY(Decode, "Decode" ) \
	DICTIONARYENTRY(Decomposition, "Decomposition" ) \
	DICTIONARYENTRY(Default, "Default" ) \
	DICTIONARYENTRY(Defer, "Defer" ) \
	DICTIONARYENTRY(Defined, "Defined" ) \
	DICTIONARYENTRY(Definition, "Definition" ) \
	DICTIONARYENTRY(Delete, "Delete" ) \
	DICTIONARYENTRY(Delimiter, "Delimiter" ) \
	DICTIONARYENTRY(Delta, "Delta") \
	DICTIONARYENTRY(DES, "DES" ) \
	DICTIONARYENTRY(Desc, "Desc" ) \
	DICTIONARYENTRY(Description, "Description" ) \
	DICTIONARYENTRY(Detail, "Detail" ) \
	DICTIONARYENTRY(DF, "DF" ) \
	DICTIONARYENTRY(DI, "DI" ) \
	DICTIONARYENTRY(Dict, "Dict" ) \
	DICTIONARYENTRY(Dictionary, "Dictionary" ) \
	DICTIONARYENTRY(Digit, "Digit" ) \
	DICTIONARYENTRY(Dim, "Dim" ) \
	DICTIONARYENTRY(Dimension, "Dimension" ) \
	DICTIONARYENTRY(Dimensions, "Dimensions" ) \
	DICTIONARYENTRY(DimSynonym, "DimSynonym" ) \
	DICTIONARYENTRY(Disable, "Disable" ) \
	DICTIONARYENTRY(Displayer, "Displayer" ) \
	DICTIONARYENTRY(DisplayerTele, "DisplayerTele" ) \
	DICTIONARYENTRY(DisplayerTrace, "DisplayerTrace" ) \
	DICTIONARYENTRY(Distance, "Distance" ) \
	DICTIONARYENTRY(Div, "Div" ) \
	DICTIONARYENTRY(Div0, "Div0" ) \
	DICTIONARYENTRY(DL, "DL" ) \
	DICTIONARYENTRY(DMY, "DMY" ) \
	DICTIONARYENTRY(Do, "Do" ) \
	DICTIONARYENTRY(DolDolLParen, "DollarDollarLParen" ) \
	DICTIONARYENTRY(DollarLParen, "DollarLParen" ) \
	DICTIONARYENTRY(DollarSign, "DollarSign" ) \
	DICTIONARYENTRY(DollarSym, "DollarSym" ) \
	DICTIONARYENTRY(Dot, "Dot" ) \
	DICTIONARYENTRY(DotDot, "DotDot" ) \
	DICTIONARYENTRY(DotDotDot, "DotDotDot" ) \
	DICTIONARYENTRY(DotDotToList, "DotDotToList" ) \
	DICTIONARYENTRY(DotIndex, "DotIndex" ) \
	DICTIONARYENTRY(DotList, "DotList" ) \
	DICTIONARYENTRY(Dotted, "Dotted" ) \
	DICTIONARYENTRY(Double, "Double" ) \
	DICTIONARYENTRY(Down, "Down" ) \
	DICTIONARYENTRY(Drawer, "Drawer" ) \
	DICTIONARYENTRY(DS, "DS" ) \
	DICTIONARYENTRY(DSI, "DSI" ) \
	DICTIONARYENTRY(DSN, "DSN" ) \
	DICTIONARYENTRY(Dump, "Dump" ) \
	DICTIONARYENTRY(Duplicates, "Duplicates" ) \
	DICTIONARYENTRY(DYM, "DYM" ) \
	DICTIONARYENTRY(E, "E" ) \
	DICTIONARYENTRY(EBind, "EBind" ) \
	DICTIONARYENTRY(Echo, "Echo" ) \
	DICTIONARYENTRY(EchoE, "EchoE" ) \
	DICTIONARYENTRY(EchoS, "EchoS" ) \
	DICTIONARYENTRY(Element, "Element" ) \
	DICTIONARYENTRY(Else, "Else" ) \
	DICTIONARYENTRY(ElseIf, "ElseIf" ) \
	DICTIONARYENTRY(EMail, "EMail" ) \
	DICTIONARYENTRY(Embedded, "Embedded" ) \
	DICTIONARYENTRY(Empty, "Empty" ) \
	DICTIONARYENTRY(EmptyLine, "EmptyLine" ) \
	DICTIONARYENTRY(Enable, "Enable" ) \
	DICTIONARYENTRY(Encode, "Encode" ) \
	DICTIONARYENTRY(End, "End" ) \
	DICTIONARYENTRY(EndIf, "EndIf" ) \
	DICTIONARYENTRY(EndLog, "EndLog" ) \
	DICTIONARYENTRY(EndOfLine, "EndOfLine" ) \
	DICTIONARYENTRY(EndTable, "EndTable" ) \
	DICTIONARYENTRY(Entries, "Entries" ) \
	DICTIONARYENTRY(Environment, "Environment" ) \
	DICTIONARYENTRY(EOF, "EOF" ) \
	DICTIONARYENTRY(EOL, "EOL" ) \
	DICTIONARYENTRY(EP, "EP" ) \
	DICTIONARYENTRY(EQ, "EQ" ) \
	DICTIONARYENTRY(Equal, "Equal" ) \
	DICTIONARYENTRY(EqualEqual, "EqualEqual" ) \
	DICTIONARYENTRY(EqualRangle, "EqualRangle" ) \
	DICTIONARYENTRY(Err, "Err" ) \
	DICTIONARYENTRY(Error, "Error" ) \
	DICTIONARYENTRY(Errors, "Errors" ) \
	DICTIONARYENTRY(ES, "ES" ) \
	DICTIONARYENTRY(Escape, "Escape" ) \
	DICTIONARYENTRY(EV, "EV" ) \
	DICTIONARYENTRY(Eval, "Eval" ) \
	DICTIONARYENTRY(EvalAE, "EvalAE" ) \
	DICTIONARYENTRY(EvalAEVal, "EvalAEVal" ) \
	DICTIONARYENTRY(EvalCmd, "EvalCmd" ) \
	DICTIONARYENTRY(EvalLE, "EvalLE" ) \
	DICTIONARYENTRY(EvalLEStar, "EvalLEStar" ) \
	DICTIONARYENTRY(EvalLEVal, "EvalLEVal" ) \
	DICTIONARYENTRY(EvalLEValStar, "EvalLEValStar" ) \
	DICTIONARYENTRY(EvalPt, "EvalPt" ) \
	DICTIONARYENTRY(EvalStream, "EvalStream" ) \
	DICTIONARYENTRY(EvalToContext, "EvalToContext" ) \
	DICTIONARYENTRY(Evaluate, "Evaluate" ) \
	DICTIONARYENTRY(Evaluations, "Evaluations" ) \
	DICTIONARYENTRY(EX, "EX" ) \
	DICTIONARYENTRY(Examine, "Examine" ) \
	DICTIONARYENTRY(Exclamation, "Exclamation" ) \
	DICTIONARYENTRY(Exists, "Exists" ) \
	DICTIONARYENTRY(Exxit, "Exit" ) \
	DICTIONARYENTRY(Exponent, "Exponent" ) \
	DICTIONARYENTRY(Expression, "Expression" ) \
	DICTIONARYENTRY(ExtDictUpd, "ExtDictUpd" ) \
	DICTIONARYENTRY(External, "External" ) \
	DICTIONARYENTRY(FAggregate, "FAggregate" ) \
	DICTIONARYENTRY(Fail, "Fail" ) \
	DICTIONARYENTRY(False, "False" ) \
	DICTIONARYENTRY(FastAdd, "FastAdd" ) \
	DICTIONARYENTRY(Fax, "Fax" ) \
	DICTIONARYENTRY(FC, "FC" ) \
	DICTIONARYENTRY(FI, "FI" ) \
	DICTIONARYENTRY(Fields, "Fields" ) \
	DICTIONARYENTRY(FIFO, "FIFO" ) \
	DICTIONARYENTRY(File, "File" ) \
	DICTIONARYENTRY(FileName, "FileName" ) \
	DICTIONARYENTRY(FileOpen, "FileOpen" ) \
	DICTIONARYENTRY(Files, "Files" ) \
	DICTIONARYENTRY(FileSave, "FileSave" ) \
	DICTIONARYENTRY(Fill, "Fill" ) \
	DICTIONARYENTRY(Filter, "Filter" ) \
	DICTIONARYENTRY(First, "First" ) \
	DICTIONARYENTRY(Fit, "Fit" ) \
	DICTIONARYENTRY(Fixed, "Fixed") \
	DICTIONARYENTRY(FixedLength, "FixedLength" ) \
	DICTIONARYENTRY(FM, "FM" ) \
	DICTIONARYENTRY(FO, "FO" ) \
	DICTIONARYENTRY(Footer, "Footer") \
	DICTIONARYENTRY(Foreign, "Foreign") \
	DICTIONARYENTRY(Form, "Form" ) \
	DICTIONARYENTRY(Format, "Format" ) \
	DICTIONARYENTRY(FR, "FR" ) \
	DICTIONARYENTRY(Frame, "Frame" ) \
	DICTIONARYENTRY(FS, "FS" ) \
	DICTIONARYENTRY(FWidth, "FWidth" ) \
	DICTIONARYENTRY(GE, "GE" ) \
	DICTIONARYENTRY(GeoCoord, "Geocoordinate") \
	DICTIONARYENTRY(GeoLbl, "GeoLbl") \
	DICTIONARYENTRY(Global, "Global") \
	DICTIONARYENTRY(Goto, "Goto") \
	DICTIONARYENTRY(Gregorian, "Gregorian" ) \
	DICTIONARYENTRY(GT, "GT" ) \
	DICTIONARYENTRY(GUI, "GUI" ) \
	DICTIONARYENTRY(HA, "HA" ) \
	DICTIONARYENTRY(Hairline, "Hairline" ) \
	DICTIONARYENTRY(Half, "Half" ) \
	DICTIONARYENTRY(Hand, "Hand" ) \
	DICTIONARYENTRY(Hashed, "Hashed") \
	DICTIONARYENTRY(HasIsA, "HasIsA" ) \
	DICTIONARYENTRY(HdrCell, "HdrCell" ) \
	DICTIONARYENTRY(HdrRow, "HdrRow" ) \
	DICTIONARYENTRY(Header, "Header" ) \
	DICTIONARYENTRY(Heading, "Heading" ) \
	DICTIONARYENTRY(Hebrew, "Hebrew" ) \
	DICTIONARYENTRY(Hex, "Hex" ) \
	DICTIONARYENTRY(Hex4, "Hex4" ) \
	DICTIONARYENTRY(Hex8, "Hex8" ) \
	DICTIONARYENTRY(Hexadecimal, "Hexadecimal" ) \
	DICTIONARYENTRY(Hierarchy, "Hierarchy" ) \
	DICTIONARYENTRY(Hindu, "Hindu" ) \
	DICTIONARYENTRY(History, "History" ) \
	DICTIONARYENTRY(HL, "HL" ) \
	DICTIONARYENTRY(Home, "Home" ) \
	DICTIONARYENTRY(Host, "Host" ) \
	DICTIONARYENTRY(HT, "HT" ) \
	DICTIONARYENTRY(HTML, "HTML" ) \
	DICTIONARYENTRY(HTMLGet, "HTMLGet" ) \
	DICTIONARYENTRY(HTMLPut, "HTMLPut" ) \
	DICTIONARYENTRY(I, "I" ) \
	DICTIONARYENTRY(IB, "IB" ) \
	DICTIONARYENTRY(IC, "IC" ) \
	DICTIONARYENTRY(Id, "Id" ) \
	DICTIONARYENTRY(Ids, "Ids" ) \
	DICTIONARYENTRY(If, "If" ) \
	DICTIONARYENTRY(If1, "If1" ) \
	DICTIONARYENTRY(Ignore, "Ignore" ) \
	DICTIONARYENTRY(IM, "IM" ) \
	DICTIONARYENTRY(Image, "Image" ) \
	DICTIONARYENTRY(IMArg, "IMArg") \
	DICTIONARYENTRY(IN, "IN" ) \
	DICTIONARYENTRY(Include, "Include" ) \
	DICTIONARYENTRY(Indent, "Indent" ) \
	DICTIONARYENTRY(Indent1, "Indent1" ) \
	DICTIONARYENTRY(Indent2, "Indent2" ) \
	DICTIONARYENTRY(Indent3, "Indent3" ) \
	DICTIONARYENTRY(Index, "Index" ) \
	DICTIONARYENTRY(Indexes, "Indexes" ) \
	DICTIONARYENTRY(Information, "Information" ) \
	DICTIONARYENTRY(Input, "Input" ) \
	DICTIONARYENTRY(Insert, "Insert" ) \
	DICTIONARYENTRY(Int, "Int") \
	DICTIONARYENTRY(Int2, "Int2") \
	DICTIONARYENTRY(IntDictUpd, "IntDictUpd" ) \
	DICTIONARYENTRY(Integer, "Integer") \
	DICTIONARYENTRY(Internal, "Internal" ) \
	DICTIONARYENTRY(Interpreting, "Interpreting" ) \
	DICTIONARYENTRY(Intersection, "Intersection") \
	DICTIONARYENTRY(IntMod, "IntMod" ) \
	DICTIONARYENTRY(IsA, "IsA" ) \
	DICTIONARYENTRY(IsctFail, "IsctFail" ) \
	DICTIONARYENTRY(iSeries, "iSeries" ) \
	DICTIONARYENTRY(Islamic, "Islamic" ) \
	DICTIONARYENTRY(ISO, "ISO" ) \
	DICTIONARYENTRY(IT, "IT" ) \
	DICTIONARYENTRY(Italic, "Italic" ) \
	DICTIONARYENTRY(Java, "Java" ) \
	DICTIONARYENTRY(Javascript, "Javascript" ) \
	DICTIONARYENTRY(JN, "JN" ) \
	DICTIONARYENTRY(JS, "JS" ) \
	DICTIONARYENTRY(JSON, "JSON" ) \
	DICTIONARYENTRY(Julian, "Julian" ) \
	DICTIONARYENTRY(Justify, "Justify" ) \
	DICTIONARYENTRY(KAPairs, "KAPairs" ) \
	DICTIONARYENTRY(KE, "KE" ) \
	DICTIONARYENTRY(Keep, "Keep" ) \
	DICTIONARYENTRY(Key, "Key" ) \
	DICTIONARYENTRY(Keyed, "Keyed" ) \
	DICTIONARYENTRY(KeyedPartial, "KeyedPartial" ) \
	DICTIONARYENTRY(LA, "LA" ) \
	DICTIONARYENTRY(Label, "Label" ) \
	DICTIONARYENTRY(LangId, "LangId" ) \
	DICTIONARYENTRY(Langle, "Langle" ) \
	DICTIONARYENTRY(LangleBang, "LangleBang" ) \
	DICTIONARYENTRY(LangleBangDashDash, "LangleBangDashDash" ) \
	DICTIONARYENTRY(LangleDash, "LangleDash" ) \
	DICTIONARYENTRY(LangleEqual, "LangleEqual" ) \
	DICTIONARYENTRY(LangleLangle, "LangleLangle" ) \
	DICTIONARYENTRY(LangleQuestion, "LangleQuestion" ) \
	DICTIONARYENTRY(LangleRangle, "LangleRangle" ) \
	DICTIONARYENTRY(LangleSlash, "LangleSlash" ) \
	DICTIONARYENTRY(Last, "Last" ) \
	DICTIONARYENTRY(Layer, "Layer" ) \
	DICTIONARYENTRY(LazyList, "LazyList" ) \
	DICTIONARYENTRY(LBrace, "LBrace" ) \
	DICTIONARYENTRY(LBraceSlash, "LBraceSlash" ) \
	DICTIONARYENTRY(LBracket, "LBracket" ) \
	DICTIONARYENTRY(LDOW, "LDOW" ) \
	DICTIONARYENTRY(LE, "LE" ) \
	DICTIONARYENTRY(LeadingZero, "LeadingZero" ) \
	DICTIONARYENTRY(Left, "Left" ) \
	DICTIONARYENTRY(LeftJustify, "LeftJustify" ) \
	DICTIONARYENTRY(Length, "Length" ) \
	DICTIONARYENTRY(Lexical, "Lexical" ) \
	DICTIONARYENTRY(LHS, "LHS" ) \
	DICTIONARYENTRY(LI, "LI" ) \
	DICTIONARYENTRY(LIFO, "LIFO" ) \
	DICTIONARYENTRY(Line, "Line" ) \
	DICTIONARYENTRY(Linear, "Linear" ) \
	DICTIONARYENTRY(LineLine, "LineLine" ) \
	DICTIONARYENTRY(Lines, "Lines" ) \
	DICTIONARYENTRY(Link, "Link" ) \
	DICTIONARYENTRY(List, "List") \
	DICTIONARYENTRY(Lists, "Lists" ) \
	DICTIONARYENTRY(Little, "Little" ) \
	DICTIONARYENTRY(LM, "LM" ) \
	DICTIONARYENTRY(LMOY, "LMOY" ) \
	DICTIONARYENTRY(Local, "Local" ) \
	DICTIONARYENTRY(Location, "Location" ) \
	DICTIONARYENTRY(Locks, "Locks" ) \
	DICTIONARYENTRY(Log, "Log" ) \
	DICTIONARYENTRY(Logical, "Logical" ) \
	DICTIONARYENTRY(LogToIsct, "LogToIsct" ) \
	DICTIONARYENTRY(Loop, "Loop" ) \
	DICTIONARYENTRY(Lowercase, "Lowercase" ) \
	DICTIONARYENTRY(LParen, "LParen" ) \
	DICTIONARYENTRY(LRBracket, "LRBracket" ) \
	DICTIONARYENTRY(LSpan, "LSpan" ) \
	DICTIONARYENTRY(LT, "LT" ) \
	DICTIONARYENTRY(Macro, "Macro" ) \
	DICTIONARYENTRY(MacroBind, "MacroBind" ) \
	DICTIONARYENTRY(MacroCall, "MacroCall" ) \
	DICTIONARYENTRY(MacroSave, "MacroSave" ) \
	DICTIONARYENTRY(Mask, "Mask" ) \
	DICTIONARYENTRY(MassUpdates, "MassUpdates" ) \
	DICTIONARYENTRY(MaxAllowedErrors, "MaxAllowedErrors" ) \
	DICTIONARYENTRY(MaxNest, "MaxNest" ) \
	DICTIONARYENTRY(MaxPointOutput, "MaxPointOutput" ) \
	DICTIONARYENTRY(MD5, "MD5" ) \
	DICTIONARYENTRY(MDY, "MDY" ) \
	DICTIONARYENTRY(ME, "ME") \
	DICTIONARYENTRY(Medium, "Medium") \
	DICTIONARYENTRY(MemPtr, "MemPtr") \
	DICTIONARYENTRY(Meta, "Meta" ) \
	DICTIONARYENTRY(MIDAS, "MIDAS" ) \
	DICTIONARYENTRY(Middle, "Middle" ) \
	DICTIONARYENTRY(Minimum, "Minimum" ) \
	DICTIONARYENTRY(Minus, "Minus" ) \
	DICTIONARYENTRY(MinusEqual, "MinusEqual" ) \
	DICTIONARYENTRY(Missing, "Missing" ) \
	DICTIONARYENTRY(MMDDYY, "MMDDYY" ) \
	DICTIONARYENTRY(MMM, "MMM" ) \
	DICTIONARYENTRY(MMYY, "MMYY" ) \
	DICTIONARYENTRY(Module, "Module" ) \
	DICTIONARYENTRY(Money, "Money" ) \
	DICTIONARYENTRY(Month, "Month" ) \
	DICTIONARYENTRY(MoreRecent, "MoreRecent" ) \
	DICTIONARYENTRY(MQ, "MQ" ) \
	DICTIONARYENTRY(MS, "MS" ) \
	DICTIONARYENTRY(MSAccess, "MSAccess" ) \
	DICTIONARYENTRY(MSSQL, "MSSQL" ) \
	DICTIONARYENTRY(Multiple, "Multiple" ) \
	DICTIONARYENTRY(MV, "MV" ) \
	DICTIONARYENTRY(MYD, "MYD" ) \
	DICTIONARYENTRY(MySQL, "MySQL") \
	DICTIONARYENTRY(N, "N" ) \
	DICTIONARYENTRY(NA, "NA" ) \
	DICTIONARYENTRY(Name, "Name" ) \
	DICTIONARYENTRY(NBLBindings, "NBLBindings" ) \
	DICTIONARYENTRY(NCColon, "NCColon" ) \
	DICTIONARYENTRY(NCDot, "NCDot" ) \
	DICTIONARYENTRY(NCLBrace, "NCLBrace" ) \
	DICTIONARYENTRY(NCLBracket, "NCLBracket" ) \
	DICTIONARYENTRY(NCLParen, "NCLParen" ) \
	DICTIONARYENTRY(NCStar, "NCStar" ) \
	DICTIONARYENTRY(NE, "NE" ) \
	DICTIONARYENTRY(Negative, "Negative" ) \
	DICTIONARYENTRY(Network, "Network" ) \
	DICTIONARYENTRY(New, "New" ) \
	DICTIONARYENTRY(NewLine, "NewLine" ) \
	DICTIONARYENTRY(NewMacro, "NewMacro" ) \
	DICTIONARYENTRY(Next, "Next" ) \
	DICTIONARYENTRY(NL, "NL" ) \
	DICTIONARYENTRY(No, "No" ) \
	DICTIONARYENTRY(NoAutoCreate, "NoAutoCreate" ) \
	DICTIONARYENTRY(NoBind, "NoBind" ) \
	DICTIONARYENTRY(NoBindingUpd, "NoBindingUpd" ) \
	DICTIONARYENTRY(NoBreak, "NoBreak" ) \
	DICTIONARYENTRY(NoCase, "NoCase" ) \
	DICTIONARYENTRY(NoCreate, "NoCreate" ) \
	DICTIONARYENTRY(Node, "Node" ) \
	DICTIONARYENTRY(NoDuplicate, "NoDuplicate" ) \
	DICTIONARYENTRY(NoEcho, "NoEcho" ) \
	DICTIONARYENTRY(NoEval, "NoEval" ) \
	DICTIONARYENTRY(NoExtDictUpd, "NoExtDictUpd" ) \
	DICTIONARYENTRY(NoIC, "NoIC" ) \
	DICTIONARYENTRY(NoIntDictUpd, "NoIntDictUpd" ) \
	DICTIONARYENTRY(None, "None" ) \
	DICTIONARYENTRY(NoPrefix, "NoPrefix" ) \
	DICTIONARYENTRY(NoResults, "NoResults" ) \
	DICTIONARYENTRY(Normal, "Normal" ) \
	DICTIONARYENTRY(Normalize, "Normalize" ) \
	DICTIONARYENTRY(NoStatistics, "NoStatistics" ) \
	DICTIONARYENTRY(Not, "Not" ) \
	DICTIONARYENTRY(NotEqual, "NotEqual" ) \
	DICTIONARYENTRY(NoTrace, "NoTrace" ) \
	DICTIONARYENTRY(NoValue, "NoValue" ) \
	DICTIONARYENTRY(Now, "Now" ) \
	DICTIONARYENTRY(NoXML, "NoXML" ) \
	DICTIONARYENTRY(NPTable, "NPTable" ) \
	DICTIONARYENTRY(NT, "NT" ) \
	DICTIONARYENTRY(Null, "Null" ) \
	DICTIONARYENTRY(Num, "Num" ) \
	DICTIONARYENTRY(Num000, "Num000" ) \
	DICTIONARYENTRY(Numeric, "Numeric" ) \
	DICTIONARYENTRY(Octal, "Octal") \
	DICTIONARYENTRY(ODBC, "ODBC") \
	DICTIONARYENTRY(ODBCConnect, "ODBCConnect" ) \
	DICTIONARYENTRY(ODBCXct, "ODBCXct" ) \
	DICTIONARYENTRY(Off, "Off" ) \
	DICTIONARYENTRY(Offset, "Offset" ) \
	DICTIONARYENTRY(OK, "OK" ) \
	DICTIONARYENTRY(On, "On" ) \
	DICTIONARYENTRY(Once, "Once" ) \
	DICTIONARYENTRY(OnError, "OnError" ) \
	DICTIONARYENTRY(Only, "Only" ) \
	DICTIONARYENTRY(Open, "Open" ) \
	DICTIONARYENTRY(Optimize, "Optimize" ) \
	DICTIONARYENTRY(Oracle, "Oracle" ) \
	DICTIONARYENTRY(OrdSfx, "OrdSfx" ) \
	DICTIONARYENTRY(ORead, "ORead" ) \
	DICTIONARYENTRY(OSFileAll, "OSFileAll" ) \
	DICTIONARYENTRY(OSFileInfo, "OSFileInfo" ) \
	DICTIONARYENTRY(OSFileListOf, "OSFileListOf" ) \
	DICTIONARYENTRY(OSHandle, "OSHandle") \
	DICTIONARYENTRY(Other, "Other" ) \
	DICTIONARYENTRY(Others, "Others" ) \
	DICTIONARYENTRY(Out, "Out" ) \
	DICTIONARYENTRY(OutFormat, "OutFormat" ) \
	DICTIONARYENTRY(Outline, "Outline" ) \
	DICTIONARYENTRY(Output, "Output" ) \
	DICTIONARYENTRY(Overload, "Overload" ) \
	DICTIONARYENTRY(Page, "Page" ) \
	DICTIONARYENTRY(Parameter, "Parameter" ) \
	DICTIONARYENTRY(Parent, "Parent" ) \
	DICTIONARYENTRY(ParseRelation, "ParseRelation" ) \
	DICTIONARYENTRY(Parts, "Parts" ) \
	DICTIONARYENTRY(Pattern, "Pattern" ) \
	DICTIONARYENTRY(PB, "PB" ) \
	DICTIONARYENTRY(PCurrent, "PCurrent" ) \
	DICTIONARYENTRY(PE, "PE" ) \
	DICTIONARYENTRY(Percent, "Percent" ) \
	DICTIONARYENTRY(Periods, "Periods" ) \
	DICTIONARYENTRY(PL, "PL" ) \
	DICTIONARYENTRY(Plain, "Plain" ) \
	DICTIONARYENTRY(Plus, "Plus" ) \
	DICTIONARYENTRY(PlusEqual, "PlusEqual" ) \
	DICTIONARYENTRY(Point, "Point" ) \
	DICTIONARYENTRY(PointCreate, "PointCreate" ) \
	DICTIONARYENTRY(PointMax, "PointMax" ) \
	DICTIONARYENTRY(PointReference, "PointReference") \
	DICTIONARYENTRY(Pop, "Pop" ) \
	DICTIONARYENTRY(PopAny, "PopAny" ) \
	DICTIONARYENTRY(Position, "Position" ) \
	DICTIONARYENTRY(Possessive, "Possessive" ) \
	DICTIONARYENTRY(Pound, "Pound" ) \
	DICTIONARYENTRY(Prefix, "Prefix" ) \
	DICTIONARYENTRY(Print, "Print" ) \
	DICTIONARYENTRY(Process, "Process" ) \
	DICTIONARYENTRY(Processes, "Processes" ) \
	DICTIONARYENTRY(Progress, "Progress" ) \
	DICTIONARYENTRY(ProjectArea, "ProjectArea" ) \
	DICTIONARYENTRY(Prompt, "Prompt" ) \
	DICTIONARYENTRY(PT, "PT" ) \
	DICTIONARYENTRY(Punctuation, "Punctuation" ) \
	DICTIONARYENTRY(Push, "Push" ) \
	DICTIONARYENTRY(Q1, "Q1" ) \
	DICTIONARYENTRY(Q2, "Q2" ) \
	DICTIONARYENTRY(Q3, "Q3" ) \
	DICTIONARYENTRY(Q4, "Q4" ) \
	DICTIONARYENTRY(QParameter, "QParameter" ) \
	DICTIONARYENTRY(Qtr, "Qtr" ) \
	DICTIONARYENTRY(Quarter, "Quarter" ) \
	DICTIONARYENTRY(Question, "Question" ) \
	DICTIONARYENTRY(QuestionRangle, "QuestionRangle" ) \
	DICTIONARYENTRY(Queue, "Queue" ) \
	DICTIONARYENTRY(Quota, "Quota" ) \
	DICTIONARYENTRY(Quoted, "Quoted" ) \
	DICTIONARYENTRY(R, "R" ) \
	DICTIONARYENTRY(RamDisk, "RamDisk" ) \
	DICTIONARYENTRY(Range, "Range" ) \
	DICTIONARYENTRY(Rangle, "Rangle" ) \
	DICTIONARYENTRY(RangleEqual, "RangleEqual" ) \
	DICTIONARYENTRY(RangleRangle, "RangleRangle" ) \
	DICTIONARYENTRY(Raw, "Raw" ) \
	DICTIONARYENTRY(RBrace, "RBrace" ) \
	DICTIONARYENTRY(RBraceCross, "RBraceCross" ) \
	DICTIONARYENTRY(RBraceSlash, "RBraceSlash" ) \
	DICTIONARYENTRY(RBracket, "RBracket" ) \
	DICTIONARYENTRY(RDB, "RDB" ) \
	DICTIONARYENTRY(RDBNumeric, "RDBNumeric" ) \
	DICTIONARYENTRY(Read, "Read" ) \
	DICTIONARYENTRY(Real, "Real") \
	DICTIONARYENTRY(Rebuild, "Rebuild" ) \
	DICTIONARYENTRY(Recap, "Recap" ) \
	DICTIONARYENTRY(Receive, "Receive" ) \
	DICTIONARYENTRY(Recognize, "Recognize" ) \
	DICTIONARYENTRY(Recursion, "Recursion" ) \
	DICTIONARYENTRY(Redirection, "Redirection" ) \
	DICTIONARYENTRY(Ref, "Ref") \
	DICTIONARYENTRY(Region, "Region") \
	DICTIONARYENTRY(RegularExpression, "RegularExpression") \
	DICTIONARYENTRY(Release, "Release" ) \
	DICTIONARYENTRY(RelTime, "RelTime" ) \
	DICTIONARYENTRY(Removable, "Removable" ) \
	DICTIONARYENTRY(Rename, "Rename" ) \
	DICTIONARYENTRY(Repeat, "Repeat" ) \
	DICTIONARYENTRY(Request, "Request" ) \
	DICTIONARYENTRY(Reset, "Reset" ) \
	DICTIONARYENTRY(Response, "Response" ) \
	DICTIONARYENTRY(Restrict, "Restrict" ) \
	DICTIONARYENTRY(Results, "Results" ) \
	DICTIONARYENTRY(Retry, "Retry" ) \
	DICTIONARYENTRY(RetryCancel, "RetryCancel" ) \
	DICTIONARYENTRY(Return, "Return" ) \
	DICTIONARYENTRY(Review, "Review" ) \
	DICTIONARYENTRY(RHS, "RHS" ) \
	DICTIONARYENTRY(Right, "Right" ) \
	DICTIONARYENTRY(RightArrow, "RightArrow" ) \
	DICTIONARYENTRY(RightJustify, "RightJustify" ) \
	DICTIONARYENTRY(RM, "RM" ) \
	DICTIONARYENTRY(RN, "RN" ) \
	DICTIONARYENTRY(Row, "Row" ) \
	DICTIONARYENTRY(RowAndData, "RowAndData" ) \
	DICTIONARYENTRY(RowFormat, "RowFormat" ) \
	DICTIONARYENTRY(Rows, "Rows" ) \
	DICTIONARYENTRY(RParen, "RParen" ) \
	DICTIONARYENTRY(RPC, "RPC" ) \
	DICTIONARYENTRY(RSpan, "RSpan" ) \
	DICTIONARYENTRY(RT, "RT" ) \
	DICTIONARYENTRY(S, "S" ) \
	DICTIONARYENTRY(Same, "Same" ) \
	DICTIONARYENTRY(Sample, "Sample" ) \
	DICTIONARYENTRY(Save, "Save" ) \
	DICTIONARYENTRY(SC, "SC" ) \
	DICTIONARYENTRY(Scale, "Scale" ) \
	DICTIONARYENTRY(Scan, "Scan" ) \
	DICTIONARYENTRY(ScanForDict, "ScanForDict" ) \
	DICTIONARYENTRY(Schema, "Schema" ) \
	DICTIONARYENTRY(SDOW, "SDOW" ) \
	DICTIONARYENTRY(SE, "SE" ) \
	DICTIONARYENTRY(Section, "Section" ) \
	DICTIONARYENTRY(Secure, "Secure" ) \
	DICTIONARYENTRY(Self, "Self" ) \
	DICTIONARYENTRY(Semi, "Semi" ) \
	DICTIONARYENTRY(Send, "Send" ) \
	DICTIONARYENTRY(Set, "Set" ) \
	DICTIONARYENTRY(SetOutput, "SetOutput" ) \
	DICTIONARYENTRY(SG, "SG" ) \
	DICTIONARYENTRY(SHA1, "SHA1") \
	DICTIONARYENTRY(SHA256, "SHA256") \
	DICTIONARYENTRY(Shadow, "Shadow") \
	DICTIONARYENTRY(Sheet, "Sheet") \
	DICTIONARYENTRY(Shell, "Shell") \
	DICTIONARYENTRY(Silent, "Silent") \
	DICTIONARYENTRY(Site, "Site") \
	DICTIONARYENTRY(SizeToFit, "SizeToFit") \
	DICTIONARYENTRY(SK, "SK" ) \
	DICTIONARYENTRY(Skip, "Skip" ) \
	DICTIONARYENTRY(Slash, "Slash" ) \
	DICTIONARYENTRY(SlashEqual, "SlashEqual" ) \
	DICTIONARYENTRY(SlashRangle, "SlashRangle" ) \
	DICTIONARYENTRY(SlashSlash, "SlashSlash" ) \
	DICTIONARYENTRY(SlashStar, "SlashStar" ) \
	DICTIONARYENTRY(SLLAD, "SLLAD" ) \
	DICTIONARYENTRY(SLLS, "SLLS" ) \
	DICTIONARYENTRY(SMOY, "SMOY" ) \
	DICTIONARYENTRY(SN, "SN" ) \
	DICTIONARYENTRY(SNC, "SNC" ) \
	DICTIONARYENTRY(SNums, "SNums" ) \
	DICTIONARYENTRY(Socket, "Socket" ) \
	DICTIONARYENTRY(Source, "Source" ) \
	DICTIONARYENTRY(SP, "SP" ) \
	DICTIONARYENTRY(Space, "Space" ) \
	DICTIONARYENTRY(Spawn, "Spawn" ) \
	DICTIONARYENTRY(Special, "Special") \
	DICTIONARYENTRY(Speed, "Speed") \
	DICTIONARYENTRY(Spreadsheet, "Spreadsheet" ) \
	DICTIONARYENTRY(SR, "SR" ) \
	DICTIONARYENTRY(SSVal, "SSVal") \
	DICTIONARYENTRY(Stack, "Stack" ) \
	DICTIONARYENTRY(Star, "Star" ) \
	DICTIONARYENTRY(StarEqual, "StarEqual" ) \
	DICTIONARYENTRY(StarStar, "StarStar" ) \
	DICTIONARYENTRY(Start, "Start" ) \
	DICTIONARYENTRY(Statistics, "Statistics" ) \
	DICTIONARYENTRY(Status, "Status" ) \
	DICTIONARYENTRY(StdOut, "StdOut" ) \
	DICTIONARYENTRY(Step, "Step" ) \
	DICTIONARYENTRY(StepInto, "StepInto" ) \
	DICTIONARYENTRY(Stop, "Stop" ) \
	DICTIONARYENTRY(Strict, "Strict" ) \
	DICTIONARYENTRY(StrikeThrough, "StrikeThrough" ) \
	DICTIONARYENTRY(String, "String" ) \
	DICTIONARYENTRY(StructEl, "StructEl") \
	DICTIONARYENTRY(Structure, "Structure" ) \
	DICTIONARYENTRY(SU, "SU" ) \
	DICTIONARYENTRY(Sub, "Sub" ) \
	DICTIONARYENTRY(Subnet, "Subnet" ) \
	DICTIONARYENTRY(Subscript, "Subscript" ) \
	DICTIONARYENTRY(Substitution, "Substitution" ) \
	DICTIONARYENTRY(Suffix, "Suffix" ) \
	DICTIONARYENTRY(Superscript, "Superscript" ) \
	DICTIONARYENTRY(TA, "TA" ) \
	DICTIONARYENTRY(Tab, "Tab" ) \
	DICTIONARYENTRY(Table, "Table" ) \
	DICTIONARYENTRY(TableDefine, "TableDefine" ) \
	DICTIONARYENTRY(TableSave, "TableSave" ) \
	DICTIONARYENTRY(Tag, "Tag") \
	DICTIONARYENTRY(TagVal, "TagVal") \
	DICTIONARYENTRY(Tally, "Tally" ) \
	DICTIONARYENTRY(TallyBind, "TallyBind" ) \
	DICTIONARYENTRY(TallyM, "TallyM" ) \
	DICTIONARYENTRY(TC, "TC" ) \
	DICTIONARYENTRY(Telephone, "Telephone") \
	DICTIONARYENTRY(TelephoneNumber, "TelephoneNumber") \
	DICTIONARYENTRY(Tens2090, "Tens2090" ) \
	DICTIONARYENTRY(Test, "Test" ) \
	DICTIONARYENTRY(Text, "Text" ) \
	DICTIONARYENTRY(TextBind, "TextBind" ) \
	DICTIONARYENTRY(TextTable, "TextTable" ) \
	DICTIONARYENTRY(Then, "Then") \
	DICTIONARYENTRY(Thick, "Thick") \
	DICTIONARYENTRY(Thin, "Thin") \
	DICTIONARYENTRY(TI, "TI" ) \
	DICTIONARYENTRY(Tilde, "Tilde" ) \
	DICTIONARYENTRY(TildeEqual, "TildeEqual" ) \
	DICTIONARYENTRY(Time, "Time") \
	DICTIONARYENTRY(TimeStamp, "TimeStamp" ) \
	DICTIONARYENTRY(TimeZone, "TimeZone" ) \
	DICTIONARYENTRY(Title, "Title" ) \
	DICTIONARYENTRY(TLS, "TLS" ) \
	DICTIONARYENTRY(TM, "TM" ) \
	DICTIONARYENTRY(Token, "Token" ) \
	DICTIONARYENTRY(Top, "Top" ) \
	DICTIONARYENTRY(TopOfPage, "TopOfPage" ) \
	DICTIONARYENTRY(Trace, "Trace" ) \
	DICTIONARYENTRY(Tree, "Tree" ) \
	DICTIONARYENTRY(Trim, "Trim" ) \
	DICTIONARYENTRY(True, "True" ) \
	DICTIONARYENTRY(Try, "Try" ) \
	DICTIONARYENTRY(TV, "TV" ) \
	DICTIONARYENTRY(TY, "TY" ) \
	DICTIONARYENTRY(TZones, "TZones" ) \
	DICTIONARYENTRY(UB, "UB" ) \
	DICTIONARYENTRY(UDate, "UDate") \
	DICTIONARYENTRY(UDim, "UDim" ) \
	DICTIONARYENTRY(UDT, "UDT") \
	DICTIONARYENTRY(UL, "UL") \
	DICTIONARYENTRY(UMinus, "UMinus" ) \
	DICTIONARYENTRY(UMonth, "UMonth") \
	DICTIONARYENTRY(UN, "UN" ) \
	DICTIONARYENTRY(Undefined, "Undefined" ) \
	DICTIONARYENTRY(Underline, "Underline" ) \
	DICTIONARYENTRY(UCChar, "Unicode" ) \
	DICTIONARYENTRY(Unique, "Unique" ) \
	DICTIONARYENTRY(Unknown, "Unknown" ) \
	DICTIONARYENTRY(Unrestricted, "Unrestricted" ) \
	DICTIONARYENTRY(Unused, "Unused" ) \
	DICTIONARYENTRY(UOM, "UOM" ) \
	DICTIONARYENTRY(UOMCoerce, "UOMCoerce" ) \
	DICTIONARYENTRY(UOMId, "UOMId" ) \
	DICTIONARYENTRY(UOMInitialize, "UOMInitialize" ) \
	DICTIONARYENTRY(UOMPer, "UOMPer") \
	DICTIONARYENTRY(UOMPUOM, "UOMperUOM") \
	DICTIONARYENTRY(Up, "Up" ) \
	DICTIONARYENTRY(Update, "Update" ) \
	DICTIONARYENTRY(UPeriod, "UPeriod") \
	DICTIONARYENTRY(Uppercase, "Uppercase" ) \
	DICTIONARYENTRY(UQuarter, "UQuarter") \
	DICTIONARYENTRY(UR, "UR" ) \
	DICTIONARYENTRY(URL, "URL" ) \
	DICTIONARYENTRY(URLOnError, "URLOnError" ) \
	DICTIONARYENTRY(Used, "Used") \
	DICTIONARYENTRY(User, "User") \
	DICTIONARYENTRY(UTF16, "UTF16") \
	DICTIONARYENTRY(UTF16be, "UTF16be") \
	DICTIONARYENTRY(UTF16le, "UTF16le") \
	DICTIONARYENTRY(UTF8, "UTF8") \
	DICTIONARYENTRY(UTF8nh, "UTF8nh") \
	DICTIONARYENTRY(UTime, "UTime") \
	DICTIONARYENTRY(UWeek, "UWeek") \
	DICTIONARYENTRY(UYear, "UYear") \
	DICTIONARYENTRY(V3Mod, "V3Mod") \
	DICTIONARYENTRY(V3ModRaw, "V3ModRaw") \
	DICTIONARYENTRY(V3PicMod, "V3PicMod") \
	DICTIONARYENTRY(V4, "V4" ) \
	DICTIONARYENTRY(V4IS, "V4IS" ) \
	DICTIONARYENTRY(V4ISCon, "V4ISCon" ) \
	DICTIONARYENTRY(V4R, "V4R" ) \
	DICTIONARYENTRY(V6, "V6" ) \
	DICTIONARYENTRY(VA, "VA" ) \
	DICTIONARYENTRY(ValidPoint, "ValidPoint" ) \
	DICTIONARYENTRY(Value, "Value" ) \
	DICTIONARYENTRY(ValueList, "ValueList" ) \
	DICTIONARYENTRY(Values, "Values" ) \
	DICTIONARYENTRY(ValuesList, "ValuesList" ) \
	DICTIONARYENTRY(ValuesTree, "ValuesTree" ) \
	DICTIONARYENTRY(ValueTree, "ValueTree" ) \
	DICTIONARYENTRY(VBottom, "VBottom" ) \
	DICTIONARYENTRY(VCenter, "VCenter" ) \
	DICTIONARYENTRY(VE, "VE" ) \
	DICTIONARYENTRY(Version, "Version" ) \
	DICTIONARYENTRY(Volumes, "Volumes" ) \
	DICTIONARYENTRY(VTop, "VTop" ) \
	DICTIONARYENTRY(W, "W" ) \
	DICTIONARYENTRY(WA, "WA" ) \
	DICTIONARYENTRY(Warn, "Warn" ) \
	DICTIONARYENTRY(Web, "Web" ) \
	DICTIONARYENTRY(WhiteSpace, "WhiteSpace" ) \
	DICTIONARYENTRY(WI, "WI" ) \
	DICTIONARYENTRY(Width, "Width" ) \
	DICTIONARYENTRY(Work, "Work" ) \
	DICTIONARYENTRY(Wrap, "Wrap" ) \
	DICTIONARYENTRY(Write, "Write" ) \
	DICTIONARYENTRY(XDB, "XDB" ) \
	DICTIONARYENTRY(XLInfo, "XLInfo" ) \
	DICTIONARYENTRY(XML, "XML" ) \
	DICTIONARYENTRY(XMLEval, "XMLEval" ) \
	DICTIONARYENTRY(XMLEvalEnd, "XMLEvalEnd" ) \
	DICTIONARYENTRY(XOR, "XOR" ) \
	DICTIONARYENTRY(XOR8, "XOR8" ) \
	DICTIONARYENTRY(XXLoop, "XXLoop" ) \
	DICTIONARYENTRY(YDM, "YDM" ) \
	DICTIONARYENTRY(Year, "Year" ) \
	DICTIONARYENTRY(Yes, "Yes" ) \
	DICTIONARYENTRY(YesNo, "YesNo" ) \
	DICTIONARYENTRY(YesNoCancel, "YesNoCancel" ) \
	DICTIONARYENTRY(YMD, "YMD" ) \
	DICTIONARYENTRY(YMDOrder, "YMDOrder" ) \
	DICTIONARYENTRY(YYMM, "YYMM" ) \
	DICTIONARYENTRY(YYMMDD, "YYMMDD" ) \
	DICTIONARYENTRY(EndOfLine_, "Z01" ) \
	DICTIONARYENTRY(Comma_, "Z02" ) \
	DICTIONARYENTRY(LBracket_, "Z03" ) \
	DICTIONARYENTRY(LBraceSlash_, "Z05" ) \
	DICTIONARYENTRY(NoCommand_, "Z06") \
	DICTIONARYENTRY(SemiColon_, "Z07") \
	DICTIONARYENTRY(Zipcode, "Zipcode") \


#define DE(sym) (enum DictionaryEntries)_##sym
#define DICTIONARYENTRY(sym,value) _##sym,
enum DictionaryEntries { ALLDICTENTRIES } ;	/* Just declare an enum of all of the above entries */
#undef DICTIONARYENTRY
