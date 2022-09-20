/*	TO BUILD ON WINDOWS
  1) Create \program files\curl directory
  2) Move include directory from curl relase to above directory
  3) Add \program files\curl\include to VS configuration properties->C->General->Additional Include Directories
  4) Create \program files\curl\lib directory
  5) Move lib directory from curl release to above
  6) Add c:\program files\curl\lib\libcurldll.a to VS configuration properties->C->Linker->Additional Dependencies
  7) Copy contents of curl\bin to windows\system32 (for 32-bit)
*/

#include <curl.h>

/*	This must be done ONLY ONCE */
//size_t function( char *ptr, size_t size, size_t nmemb, void *userdata)
size_t curlWriteFunc(dataPtr,dataSize,dataNum,vhr)
  char *dataPtr ;
  size_t dataSize, dataNum ;
  struct vHTTP__Result *vhr ;
{ size_t dataBytes ;

/*	If writing to a file then just do it */
	if (vhr->dstFile != NULL)
	 { return(fwrite(dataPtr,dataSize,dataNum,vhr->dstFile->fp)) ;
	 } ;
	dataBytes = dataSize * dataNum ;
	if (vhr->bytesAlloc == 0)
	 { vhr->bytesAlloc = (vhr->contentLength > dataBytes ? vhr->contentLength : (dataBytes * 2)) ; vhr->dstBuffer = v4mm_AllocChunk(vhr->bytesAlloc,FALSE) ; } ;
	if (vhr->bytes+dataBytes > vhr->bytesAlloc)
	 { vhr->bytesAlloc = (int)(vhr->bytes*1.2) ; vhr->dstBuffer = realloc(vhr->dstBuffer,vhr->bytesAlloc+16) ; } ; 
	memcpy(vhr->dstBuffer+vhr->bytes,dataPtr,dataBytes) ;
	vhr->bytes += dataBytes ;
	if (vhr->dstBuffer != NULL) vhr->dstBuffer[vhr->bytes] = '\0' ;
	return(dataBytes) ;
}
size_t curlWriteHeader(dataPtr,dataSize,dataNum,vhr)
  char *dataPtr ;
  size_t dataSize, dataNum ;
  struct vHTTP__Result *vhr ;
{ 
  size_t hdrBytes ; char *hdrs, *b, *b2 ;

	hdrBytes = dataSize * dataNum ;
/*	Should have headers as one chunk, no need to allocate - just scan for info */
	hdrs = dataPtr ;
	for(b=hdrs;;)
	 { b2 = strchr(b,'\n') ; if (b2 == NULL) b2 = strchr(b,'\r') ; if (b2 != NULL) *b2 = UCEOS ;
	   if (strnicmp("CONTENT-TYPE",b,12) == 0)
	    { for(b=b+12;!vuc_IsWSpace(*b);b++) {} ;
	      if (strnicmp(b,"TEXT/HTML",9) == 0) { vhr->contentType = V4HTML_ContentType_HTML ; }
	       else if (strnicmp(b,"TEXT/PLAIN",10) == 0)  {vhr->contentType = V4HTML_ContentType_Text ; }
	       else if (strnicmp(b,"APPLICATION/",12) == 0)  {vhr->contentType = V4HTML_ContentType_Appl ; }
	       else if (strnicmp(b,"IMAGE/",6) == 0)  {vhr->contentType = V4HTML_ContentType_Image ; }
	       else if (strnicmp(b,"AUDIO/",6) == 0)  {vhr->contentType = V4HTML_ContentType_Sound ; }
	       else if (strnicmp(b,"TEXT/XML",8) == 0)  {vhr->contentType = V4HTML_ContentType_XML ; } ;
	    } else if (strnicmp("CONTENT-LENGTH",b,14) == 0)
	    { for(;*b!=' ';b++) {} ;
	      vhr->contentLength = strtol(b,&b,10) ; if (*b > ' ') vhr->contentLength = UNUSED ;
	    } ;
	   if (b2 == NULL) break ;
	   for(b=b2+1;!vuc_IsLetter(*b);b++) {} ;
	 } ;
	return(hdrBytes) ;
}

LOGICAL vHTTP_SendToServer(la,url,httpVerb,objectName,contentType,cookies,authorization,data,dataBytes,vhr,errbuf)
  struct lcl__args *la ;
  UCCHAR *url ;
  ETYPE httpVerb ;
  UCCHAR *objectName ;
  UCCHAR *contentType ;
  UCCHAR *cookies ;
  UCCHAR *authorization ;
  BYTE *data ;
  LENMAX dataBytes ;
  struct vHTTP__Result *vhr ;
  UCCHAR *errbuf ;
{

  CURL *curl ; static LOGICAL curlNeedInit = TRUE ;
  char curlErrs[CURL_ERROR_SIZE], *curlEMne ;
  struct curl_slist *curlHList = NULL ;
  struct curl_httppost *firstArg, *lastArg ;
  ETYPE res ; LOGICAL ok ;

	if (curlNeedInit) { curl_global_init(CURL_GLOBAL_ALL) ; curlNeedInit = FALSE ; } ;
	curl = curl_easy_init() ;

#define CURLOPT(_OPT,_ARG) res = curl_easy_setopt(curl,_OPT,_ARG) ; if (res != CURLE_OK) { curlEMne = #_OPT ; goto opt_fail ; } ;
/*	Set session options with as many of these as necessary */

	CURLOPT(CURLOPT_NOPROGRESS,1)				/* Shut off progress meter */
	CURLOPT(CURLOPT_WRITEFUNCTION,curlWriteFunc)		/* curlWriteFunc(char *ptr, size_t size, size_t nmemb, void *userdata) */
	  CURLOPT(CURLOPT_WRITEDATA,vhr) ;			/*  (want to pass this as extra argument) */
	CURLOPT(CURLOPT_HEADERFUNCTION,curlWriteHeader)		/* Same as above but to receive header info */
	  CURLOPT(CURLOPT_WRITEHEADER,vhr) ;
	CURLOPT(CURLOPT_ERRORBUFFER,curlErrs)			/* To receive readable error message */

/*	If objectName not NULL then append to end of URL */
	if (objectName == NULL) { CURLOPT(CURLOPT_URL,UCretASC(url)) }
	 else { UCCHAR urlObj[4096] ;
		UCstrcpy(urlObj,url) ; if (urlObj[UCstrlen(urlObj)-1] != UClit('/')) UCstrcat(urlObj,UClit("/")) ; UCstrcat(urlObj,objectName) ;
		CURLOPT(CURLOPT_URL,UCretASC(urlObj)) ;
	      } ;

//	CURLOPT(CURLOPT_SSLVERSION,CURL_SSLVERSION_DEFAULT)
	CURLOPT(CURLOPT_SSLVERSION,CURL_SSLVERSION_TLSv1_2)
	CURLOPT(CURLOPT_SSL_VERIFYPEER,0)

	CURLOPT(CURLOPT_USERPWD,UCretASC(authorization))	/* Authorization password- [user name]:[password] */
	 CURLOPT(CURLOPT_HTTPAUTH,CURLAUTH_DIGEST) ;		/* (use CURLAUTH_BASIC if DIGEST gives us trouble) */

	CURLOPT(CURLOPT_COOKIE,UCretASC(cookies))		/* Sets outgoing cookies */
	switch (httpVerb)
	 { default:			CURLOPT(CURLOPT_HTTPGET,1) ; break ;
	   case VHTTP_Verb_Post:	CURLOPT(CURLOPT_POST,1) ; break ;
	 } ;

	firstArg = NULL ; lastArg = NULL ;
	if (la != NULL)						/* Do we have explicit POST arguments - if so then handle, otherwise assume data/dataBytes has already-formatted POST data */
	 { INDEX i ;
	   for(i=0;i<la->argCnt;i++)
	    { if (la->arg[i].isFile)
	       { curl_formadd(&firstArg,&lastArg,CURLFORM_PTRNAME,la->arg[i].name,CURLFORM_FILE,la->arg[i].data,CURLFORM_END) ;
	       } else
	       { curl_formadd(&firstArg,&lastArg,CURLFORM_PTRNAME,la->arg[i].name,CURLFORM_PTRCONTENTS,la->arg[i].data,CURLFORM_END) ;
	       } ;
	    } ;
	   CURLOPT(CURLOPT_HTTPPOST,firstArg)			/* Linked list of curl_httppost structures for multipart/formdata posts */

	 } else
	 { CURLOPT(CURLOPT_POSTFIELDS,data)			/* Post data */
	   CURLOPT(CURLOPT_POSTFIELDSIZE,dataBytes)		/* Defines length of above data */
	 } ;


	//To pass headers
	{ char hbuf[2048] ;
//	  curlHList = curl_slist_append(curlHList, const char * string );	/* For each header */
	  sprintf(hbuf,"Content-Type: %s",UCretASC(contentType)) ; curlHList = curl_slist_append(curlHList,hbuf);	/* For each header */
	}
	if (curlHList != NULL)
	 { CURLOPT(CURLOPT_HTTPHEADER,curlHList) ; } ;

	if (vhr->dstFile != NULL) vhr->dstBuffer = NULL ;
	vhr->bytes = 0 ; vhr->bytesAlloc = 0 ;

/*	Actually perform HTTP request */
	res = curl_easy_perform(curl) ; if (res != CURLE_OK) { curlEMne = "curl_easy_perform" ; goto opt_fail ; } ;

	ok = TRUE ; goto curl_cleanup ;

opt_fail:
	v_Msg(NULL,errbuf,"@libcurl %1s failed-  (%2d) %3s",curlEMne,res,curlErrs) ;
	ok = FALSE ; goto curl_cleanup ;

/*	When all done */
curl_cleanup:
	if (curlHList != NULL) curl_slist_free_all(curlHList) ; 
	if (firstArg != NULL) curl_formfree(firstArg) ;
	curl_easy_cleanup(curl);
	return(ok) ;
}

