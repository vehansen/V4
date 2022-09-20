
/*  Error handler for callbacks */
void err_handler(OCI_Error *err)
{ printf("code  : ORA-%05i\n" "msg   : %s\n" "sql   : %s\n", OCI_ErrorGetOCICode(err), OCI_ErrorGetString(err), OCI_GetSql(OCI_ErrorGetStatement(err)) );
  gpi->xdb->con[cx].lastErrCode = 
  gpi->xdb->con[cx].lastErrMsg = 
}

/* Initialization */
   if (!OCI_Initialize(err_handler, NULL, OCI_ENV_DEFAULT)) return EXIT_FAILURE;

/* Connect to database */
    OCI_Connection *cn;
    cn = OCI_ConnectionCreate("db", "usr", "pwd", OCI_SESSION_DEFAULT);		//NULL on fail

/*  Free up connection */
    OCI_ConnectionFree(OCI_Connection *con)	

/*  Commit or roll back transactions */
OCI_EXPORT boolean OCI_API OCI_Commit(OCI_Connection *con)	
OCI_API OCI_Rollback(OCI_Connection *con)	

/*  Executing statements */
OCI_Statement  *st;

    st = OCI_StatementCreate(cn);	/* Create statement */
    OCI_Prepare(st, "delete from test_fetch where code > 10");		//Returns TRUE/FALSE
    OCI_Execute(st);							//Returns TRUE/FALSE

    OCI_ExecuteStmt(st, "delete from test_fetch where code > 1"); /* Combine above two into 1 */ 	//Returns TRUE/FALSE

/*  Fetching data */
    OCI_Resultset  *rs;
    OCI_ExecuteStmt(st, "select * from products");
    rs = OCI_GetResultset(st);
   
    while (OCI_FetchNext(rs)) {
        printf("code: %i, name %s\n", OCI_GetInt(rs, 1)  , OCI_GetString(rs, 2));
     } ;
    printf("\n%d row(s) fetched\n", OCI_GetRowCount(rs));

(B64INT)OCI_GetBigInt(OCI_Resultset *rs,unsigned int index )			//64 bit

(dtext *)OCI_API OCI_GetString(OCI_Resultset *rs,unsigned int index )		//

double OCI_API OCI_GetDouble(OCI_Resultset *rs,unsigned int index )	

 OCI_Date* OCI_API OCI_GetDate(OCI_Resultset *rs,unsigned int index )	

 OCI_Timestamp* OCI_API OCI_GetTimestamp(OCI_Resultset *rs,unsigned int	index )		

 boolean OCI_API OCI_IsNull(OCI_Resultset *rs,unsigned int index )		

 /* Determining column info */
    n  = OCI_GetColumnCount(rs);
    for(i = 1; i <= n; i++)
    { OCI_Column *col = OCI_GetColumn(rs, i);
      printf("Field #%i : name '%s' - size %i\n", i, OCI_ColumnGetName(col), OCI_ColumnGetSize(col));
    }

//Column type
 unsigned int OCI_API OCI_ColumnGetType(OCI_Column *col)

OCI_CDT_NUMERIC : short, int, long long, float, double
OCI_CDT_DATETIME : OCI_Date *
OCI_CDT_TEXT : dtext *
OCI_CDT_LONG : OCI_Long *
OCI_CDT_CURSOR : OCI_Statement *
OCI_CDT_LOB : OCI_Lob *
OCI_CDT_FILE : OCI_File *
OCI_CDT_TIMESTAMP : OCI_Timestamp *
OCI_CDT_INTERVAL : OCI_Interval *
OCI_CDT_RAW : void *
OCI_CDT_OBJECT : OCI_Object *
OCI_CDT_COLLECTION : OCI_Coll *
OCI_CDT_REF : OCI_Ref *

//Column size
unsigned int OCI_API OCI_ColumnGetSize(OCI_Column *col)	

//Decoding dates
OCI_DateGetDate	(OCI_Date *date,int *year,int *month,int *day )		
OCI_DateGetDateTime(OCI_Date *date,int *year,int *month,int *day,int *hour,int *min,int *sec )	

/*  Release all resources */
    OCI_Cleanup();