/*	V3PCKU.C - PACKAGE ROUTINES FOR VICTIM III

	LAST EDITED 7/23/84 BY VICTOR E. HANSEN		*/

#include <time.h>
#include "v3defs.c"
#ifdef V3PCKSHRSEG
#ifdef POSIX
#ifdef INCSYS
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#else
#include <types.h>
#include <ipc.h>
#include <shm.h>
#endif /* INCSYS */
#endif /* POSIX */
#ifdef WINNT
#include <windows.h>
#endif
#endif /* V3PCKSHRSEG */

/*	Link this up to the rest of VICTIM III via globals	*/
extern struct db__process *process ;		/* Current process */
extern struct db__psi *psi ;
struct db__module_entry *pcku_module_defined(/* name */) ;

/*	P A C K A G E  /  P A R S E   R O U T I N E S		*/

/*	pcku_parse_init - Inits for a new (empty) package	*/
void pcku_parse_init(package_id,name)
  int package_id ;		/* The id to use */
  char *name ;		/* Pointer to package name */
{ struct db__package *pckp ;
   struct db__parse_info *prsp ;
   struct db__breakpoints *bpl ;
   short int i ; char tname[V3_PACKAGE_NAME_MAX+1] ;

/*	Make sure package id is valid */
	if (package_id < 0 || package_id >= V3_PACKAGE_MAX)
	 v3_error(V3E_BADID,"PCK","PARSE_INIT","BADID","Package id number is out of range",(void *)V3_PACKAGE_MAX) ;
/*	Make sure package slot not already in use */
	if ((pckp = process->package_ptrs[package_id]) != NULLV)
	 v3_error(V3E_INUSE,"PCK","PARSE_INIT","INUSE","Package slot already in use",(void *)pckp->package_name) ;
/*	Convert name to upper case */
	for (i=0;i<V3_PACKAGE_NAME_MAX;i++)
	 { tname[i] = (*(name+i) >= 'a' ? *(name+i)-32 : (*(name+i) == '-' ? '_' : *(name+i))) ;
	   if (tname[i] == NULLV) break ;
	 } ;
/*	Allocate package & parse stuff */
	process->package_ptrs[package_id] = (pckp = (struct db__package *)v4mm_AllocChunk(sizeof *pckp,TRUE)) ;
	pckp->parse_ptr = (prsp = (struct db__parse_info *)prsu_init_new()) ;
/*	Link package pointers to parse */
	pckp->package_id = (prsp->package_id = package_id) ;
	strcpy(pckp->package_name,tname) ; strcpy(prsp->package_name,tname) ;
	pckp->code_ptr = (V3OPEL *)&prsp->code ; pckp->impure_ptr = prsp->impure_buf ; pckp->pure_ptr = prsp->pure_buf ;
	pckp->name_table_ptr = &prsp->name_table ; pckp->ob_bucket_ptr = &prsp->ob_bucket ;
	pckp->bp_ptr = (struct db__breakpoints *)v4mm_AllocChunk(sizeof *bpl,TRUE) ;
	pckp->checksum_ptr = &prsp->checksums ; pckp->skeleton_ptr = &prsp->skeletons ;
	for (i=0;i<V3_PACKAGE_NAME_OB_TABLE_MAX;i++) { pckp->name_ob_ptrs[i] = &prsp->name_ob_pairs[i] ; } ;
	return ;
}

/*	pcku_parse_load - Loads package with prior save parse info */

void pcku_parse_load(file_name)
  char *file_name ;		/* File to be loaded */
{ struct db__parse_info *prsp ;
   struct db__package *pckp ;
   struct pf__package_header ph ;
   struct db__module_entry *me ;
   struct db__breakpoints *bpl ;
   FILE *file_ptr ;
   short int i ; char tstr[200] ;

/*	Try to open the file */
	if ((file_ptr = fopen((char *)v3_logical_decoder(file_name,TRUE),"rb")) == NULLV)
	 { strcpy(tstr,file_name) ; strcat(tstr,".v3p") ;
	   if ((file_ptr = fopen((char *)v3_logical_decoder(tstr,TRUE),"rb")) == NULLV)
	    { sprintf(tstr,"Error (%s) accessing package (%s) to load",v_OSErrString(errno),tstr) ;
	      v3_error(V3E_PLOADNOPACK,"PCK","PARSE_LOAD","PLOADNOPACK",tstr,(void *)errno) ;
	    } ;
	 } ;
/*	Read in package header */
	fread(&ph,sizeof ph,1,file_ptr) ;
/*	Allocate parse buffer & read in info from file */
	prsp = (struct db__parse_info *)v4mm_AllocChunk(sizeof *prsp,TRUE) ;
	if (fread(prsp,sizeof *prsp,1,file_ptr) == NULLV)
	 { fclose(file_ptr) ;
	   v3_error(V3E_FILEIO,"PCK","PARSE_LOAD","FILEIO","Error(s) reading package information",(void *)tstr) ;
	 } ;
	fclose(file_ptr) ;
/*	Make sure this looks link a valid package */
	if (!ph.is_parse)
	 v3_error(V3E_NOTPRSHDR,"PCK","PARSE_LOAD","NOTPRSHDR","File contents not valid for parse load",tstr) ;
	if (ph.v3_version_major != V3_VERSION_MAJOR)
	 v3_error(V3E_BADVER,"PCK","PARSE_LOAD","BADVER","Version mismatch",(void *)ph.v3_version_major) ;
	if (process->package_ptrs[prsp->package_id] != NULLV)
	 { v4mm_FreeChunk(prsp) ; v3_error(V3E_INUSE,"PCK","PARSE_LOAD","INUSE","Package slot already in use",(void *)prsp->package_id) ;
	 } ;
/*	Looks ok - Allocate package info & link to parse */
	process->package_ptrs[prsp->package_id] = (pckp = (struct db__package *)v4mm_AllocChunk(sizeof *pckp,TRUE)) ;
	pckp->package_id = prsp->package_id ; pckp->parse_ptr = prsp ;
	strcpy(pckp->package_name,ph.package_name) ; pckp->id_obj = ph.id_obj ;
	strcpy(pckp->startup_module,ph.startup_module) ;
	pckp->code_ptr = (V3OPEL *)&prsp->code ; pckp->impure_ptr = prsp->impure_buf ; pckp->pure_ptr = prsp->pure_buf ;
	pckp->name_table_ptr = &prsp->name_table ; pckp->ob_bucket_ptr = &prsp->ob_bucket ;
	pckp->bp_ptr = (struct db__breakpoints *)v4mm_AllocChunk(sizeof *bpl,TRUE) ;
	for (i=0;i<V3_PACKAGE_NAME_OB_TABLE_MAX;i++) { pckp->name_ob_ptrs[i] = &prsp->name_ob_pairs[i] ; } ;
/*	Now take care of any global/external linkages */
	pcku_load_package(pckp) ;
/*	Maybe redo object search list */
	sobu_calculate_name_ob_list() ;
	return ;
}

/*	P A C K A G E  /  E X E C U T E - O N L Y   R O U T I N E S	*/

/*	pcku_xct_load - Loads package for execute-only (no parse) */
/*	  (returns pointer to new package)			*/

struct db__package *pcku_xct_load(file_name,flag)
  char *file_name ;		/* Name of file to be loaded */
  int flag ;			/* If TRUE then no error if package already loaded- just ignore this request */
{ struct db__package *pckp ;
   struct pf__package_header ph ;	/* Package header */
   struct db__module_entry *me ;
#ifdef V3PCKSHRSEG
#ifdef POSIX
   key_t segkey ;			/* Sharable segment key */
   struct shmid_ds seginfo ;		/* Segment information buffer */
   int segid ;				/* Segment ID */
#define SEGIDLISTMAX 256
   static int segCount = 0 ;
   static key_t segKeyList[SEGIDLISTMAX] ;	/* Holds list of loaded segment keys (to prevent duplicates) */
   int tries ;
#endif
#ifdef WINNT
   HANDLE hMapObject ;
   char map[200] ;
#endif
#endif
   FILE *file_ptr ; char *filename ;
   HANGLOOSEDCL int hangcount ;
   char ebuf[200],tstr[200] ; char *buf_ptr,*bp ; int i ;

#ifdef VMSOS
	return((struct db__package *)vax_xct_load(file_name,flag)) ;
#endif
/*	Open the file	*/
	strcpy(tstr,file_name) ; bp = (char *)strrchr(tstr,'.') ;
	if (bp == NULLV ? TRUE : (strncmp(bp,".v3x",4) != 0 && strncmp(bp,".V3X",4) != 0))
	 strcat(tstr,".v3x") ;	/* Assume we need v3x extension */
	if ((file_ptr = fopen((filename = (char *)v3_logical_decoder(tstr,TRUE)),"rb")) == NULLV)
	 { sprintf(ebuf,"Error (%s) accessing package (%s) to load",v_OSErrString(errno),tstr) ;
	   v3_error(V3E_XLOADNOPACK,"PCK","XCT_LOAD","XLOADNOPACK",ebuf,(void *)errno) ;
	 } ;
/*	First load in package header to find out what we got */
	fread(&ph,sizeof ph,1,file_ptr) ;
	if ((pckp = process->package_ptrs[ph.package_id]) != NULLV)
	 { fclose(file_ptr) ; if (flag) return(pckp) ;
	   v3_error(V3E_INUSE,"PCK","XCT_LOAD","INUSE","Package slot already in use",(void *)0) ;
	 } ;
	if (ph.v3_version_major != V3_VERSION_MAJOR)
	 v3_error(V3E_BADVER,"PCK","XCT_LOAD","BADVER","Version mismatch",(void *)ph.v3_version_major) ;
	if (ph.is_parse)
	 v3_error(V3E_HDRNOTXCT,"PCK","XCT_LOAD","HDRNOTXCT","File not in valid xct load format",(void *)tstr) ;
/*	Package looks ok, allocate & start filling in the pieces */
	process->package_ptrs[ph.package_id] = (pckp = (struct db__package *)v4mm_AllocChunk(sizeof *pckp,TRUE)) ;
/*	Copy info from package header */
	pckp->package_id = ph.package_id ; strcpy(pckp->package_name,ph.package_name) ;
	pckp->id_obj = ph.id_obj ; strcpy(pckp->startup_module,ph.startup_module) ;
	pckp->total_xct_bytes = ph.total_xct_bytes ; pckp->total_nonshare_bytes = ph.total_nonshare_bytes ;
	strcpy(pckp->file_name,filename) ;
#define PHL(PTR,FO,CAST) pckp->PTR = (CAST *)(buf_ptr + ph.FO.loc - sizeof ph) ;
#ifdef V3PCKSHRSEG
/*	Now have to allocate & read non-sharable stuff */
	buf_ptr = (char *)v4mm_AllocChunk(ph.total_nonshare_bytes,TRUE) ; fread(buf_ptr,ph.total_nonshare_bytes-sizeof ph,1,file_ptr) ;
	PHL(impure_ptr,impure,int) PHL(skeleton_ptr,skeletons,struct db__pck_skeletons)
#ifdef POSIX
	for(tries=0;tries<100;tries++)
	 { i = ((ph.package_checksum+tries) & 0xFF) ; if (i == 0) i = 1 ;	/* 0 not allowed on OSF */
	   segkey = ftok(filename,i) ;				/* Create unique segment key for this package */
	   if (segkey == -1) segkey = ftok(filename,1) ;
/*	   Look to see if we already got this one */
	   for(i=0;i<segCount;i++) { if (segkey == segKeyList[i]) break ; } ;
	   if (i >= segCount)
	    { if (segCount < SEGIDLISTMAX) { segKeyList[segCount++] = segkey ; } ;
	      break ;
	    } ;
	 } ;
	if ( (segid = shmget(segkey,ph.total_xct_bytes-ph.total_nonshare_bytes,0440)) >= 0)
	 { for(hangcount=0;hangcount<2;hangcount++)
	    { if ((int)(buf_ptr = shmat(segid,(char*)0,SHM_RDONLY)) != -1) break ;
	      if (hangcount < 1) { HANGLOOSE(100) ; continue ; } ;
	      sprintf(ebuf,"Error (%s) attaching to segment (%d) for package (%s)",v_OSErrString(errno),segid,filename) ;
	      v3_error(V3E_NOSHMAT,"PCK","XCT_LOAD","NOSHMAT",ebuf,errno) ;
	    } ;
	   for(hangcount=0;hangcount<10;hangcount++)
	    { if ((shmctl(segid,IPC_STAT,&seginfo)) == -1)
	       v3_error(V3E_NOSHMCTL,"PCK","XCT_LOAD","NOSHMCTL","Could not get(1) segment status info",errno) ;
	      if (seginfo.shm_perm.mode == 0440) break ;			/* If not read-only then not yet init */
	      HANGLOOSE(10) ;							/* (see below - sets to RO after fread) */
	    } ;
	 } else	/* Could not find segment - have to create it */
	 { if ((segid = shmget(segkey,ph.total_xct_bytes-ph.total_nonshare_bytes,IPC_CREAT+0660)) < 0)
	    v3_error(V3E_NOSEG,"PCK","XCT_LOAD","NOSEG","Could not get sharable segment",errno) ;
	   if ((shmctl(segid,IPC_STAT,&seginfo)) == -1)
	    v3_error(V3E_NOSHMCTL,"PCK","XCT_LOAD","NOSHMCTL","Could not get(2) segment status info",errno) ;
	   seginfo.shm_perm.mode = 0660 ; shmctl(segid,IPC_SET,&seginfo) ;	/* Set segment to read-write (temp) */
	   if ((int)(buf_ptr = shmat(segid,(char*)0,0)) == -1)
	    v3_error(V3E_NOSHMAT,"PCK","XCT_LOAD","NOSHMAT","Could not attach segment to process",errno) ;
	   fread(buf_ptr,ph.total_xct_bytes-ph.total_nonshare_bytes,1,file_ptr) ;
	   seginfo.shm_perm.mode = 0440 ;
	   if(shmctl(segid,IPC_SET,&seginfo) == -1)	/* Set segment to read-only */
	    v3_error(V3E_NOSHMAT,"PCK","XCT_LOAD","NOSHMAT","Could not set segment to ReadOnly",errno) ;
	 } ;
	pckp->shmaddr = buf_ptr ; pckp->shmid = segid ;	/* Save segment ID so we can maybe release it */
#endif
#ifdef WINNT
	sprintf(map,"%s-%d",ph.package_name,ph.package_checksum) ;
	hMapObject =
	 CreateFileMapping((HANDLE)0xFFFFFFFF,NULL,PAGE_READWRITE,0,ph.total_xct_bytes-ph.total_nonshare_bytes,map) ;
	if (hMapObject == NULL)
	 { sprintf(ebuf,"Error (%s) in CreateFileMapping of %s",v_OSErrString(GetLastError()),ph.package_name) ;
	   v3_error(V3E_NOCREFILMAP,"PCK","XCT_LOAD","NOCREFILMAP",ebuf,0) ;
	 } ;
	if (GetLastError() != ERROR_ALREADY_EXISTS)
	 { buf_ptr = (char *)MapViewOfFile(hMapObject,FILE_MAP_WRITE,0,0,0) ;
	   fread(buf_ptr,ph.total_xct_bytes-ph.total_nonshare_bytes,1,file_ptr) ;
	   UnmapViewOfFile(buf_ptr) ;
	 } ;
	buf_ptr = (char *)MapViewOfFile(hMapObject,FILE_MAP_READ,0,0,0) ;
	if (buf_ptr == NULL)
	 { sprintf(ebuf,"Error (%s) in MapViewOfFile of %s",v_OSErrString(GetLastError()),ph.package_name) ;
	  v3_error(V3E_NOMAPVIEWFILE,"PCK","XCT_LOAD","NOMAPVIEWFILE",ebuf,0) ;
	 } ;
	pckp->shmaddr = buf_ptr ; pckp->shmid = hMapObject ;
#endif
#define PHS(PTR,FO,CAST) pckp->PTR = (CAST *)(buf_ptr + ph.FO.loc - ph.total_nonshare_bytes) ;
	PHS(code_ptr,code,V3OPEL) ; PHS(pure_ptr,pure,int) ;
	PHS(checksum_ptr,checksum,struct db__checksum_table) ;
	PHS(ob_bucket_ptr,ob_bucket,struct db__ob_bucket) ; PHS(name_table_ptr,name_table,struct sob__table) ;
	for (i=0;i<V3_PACKAGE_NAME_OB_TABLE_MAX;i++)
	 { if (ph.name_ob_pairs[i].bytes == 0) { pckp->name_ob_ptrs[i] = NULL ; }
	    else { PHS(name_ob_ptrs[i],name_ob_pairs[i],struct db__name_ob_pairs) ; } ;
	 } ;
#else
/*	Read all info in one chunk & then figure out pointers */
	buf_ptr = (char *)v4mm_AllocChunk(ph.total_xct_bytes,TRUE) ;
	fread(buf_ptr,ph.total_xct_bytes,1,file_ptr) ;
	PHL(code_ptr,code,short int) ; PHL(impure_ptr,impure,int) ; PHL(pure_ptr,pure,int) ;
	PHL(checksum_ptr,checksum,struct db__checksum_table) ;
	PHL(ob_bucket_ptr,ob_bucket,struct db__ob_bucket) ; PHL(name_table_ptr,name_table,struct sob__table) ; PHL(skeleton_ptr,skeletons,struct db__pck_skeletons) ;
	for (i=0;i<V3_PACKAGE_NAME_OB_TABLE_MAX;i++)
	 { if (ph.name_ob_pairs[i].bytes == 0) { pckp->name_ob_ptrs[i] = NULL ; }
	   else { PHL(name_ob_ptrs[i],name_ob_pairs[i],struct db__name_ob_pairs) ; } ;
	 } ;
#endif
/*	Got everything loaded - check out global/external symbols */
	pcku_load_package(pckp) ;
/*	Maybe redo object search list */
	sobu_calculate_name_ob_list() ;
	fclose(file_ptr) ;
/*	Twiddle the skeletons */
	for(i=0;i<pckp->skeleton_ptr->count;i++)
	 { sobu_load_skeleton(pckp->skeleton_ptr,(struct db__prs_skeleton *)&pckp->skeleton_ptr->buf[pckp->skeleton_ptr->index[i]],NULLV) ; } ;
	return(pckp) ;
}

/*	P A C K A G E   S A V E   R O U T I N E S 		*/

/*	pcku_save - Saves "parse" package into parse and/or xct file */

void pcku_save(pckp,file_name)
  struct db__package *pckp ;		/* Pointer to package to be saved */
  char *file_name ;			/* Name of file(s) to be saved */
{ struct pf__package_header ph ;	/* Temp package header for xct file */
   struct db__parse_info *prsp ;
   int fo=0 ;				/* Keeps track of file byte offset */
   int i ;
   char tstr[200] ;			/* For constructing full file names */
   char buffer[20] ;
   FILE *file_ptr1,*file_ptr2 ;		/* For parse/xct save files */

/*	Update package header info	*/
	ph.package_id = pckp->package_id ; strcpy(ph.package_name,pckp->package_name) ;
	ph.id_obj = pckp->id_obj ; strcpy(ph.startup_module,pckp->startup_module) ;
	ph.v3_version_major = V3_VERSION_MAJOR ; ph.v3_version_minor = V3_VERSION_MINOR ;
/*	Make sure we have parse info to save */
	if ((prsp = pckp->parse_ptr) == NULLV)
	 v3_error(V3E_NOSAVE,"PCK","SAVE","NOSAVE","Cannot save xct-only package",pckp->package_name) ;
/*	If any parser errors, generate error but clear count so next call will work */
	if (prsp->error_flag > 0)
	 { prsp->error_flag = 0 ;
	   v3_error(V3E_PRSERRS,"PCK","SAVE","PRSERRS","Errors detected during compilation (count reset by this error)",
		    (void *)prsp->error_cnt) ;
	 } ;
/*	Get package checksum (total of all subelement checksums) */
	prsp->checksums.package_checksum = 0 ;
	for(i=0;i<prsp->checksums.count;i++) { prsp->checksums.package_checksum += prsp->checksums.item[i].checksum ; } ;
/*	Now open the file(s) - if no extension then open both, otherwise just xct-only */
	if (strchr(file_name,'.') == 0)
	 { strcpy(tstr,file_name) ; strcat(tstr,".v3p") ;
	   if ((file_ptr1 = fopen((char *)v3_logical_decoder(tstr,FALSE),"wb")) == NULLV)
	    v3_error(V3E_CREATEPARSE,"PCK","SAVE","CREATEPARSE","Could not save into parse file",(void *)tstr) ;
	   strcpy(tstr,file_name) ; strcat(tstr,".v3x") ;
	   if ((file_ptr2 = fopen((char *)v3_logical_decoder(tstr,FALSE),"wb")) == NULLV)
	    v3_error(V3E_CREATEXCT,"PCK","SAVE","CREATEXCT","Could not save into xct file",(void *)tstr) ;
	 } else
	 { if ((file_ptr2 = fopen((char *)v3_logical_decoder(file_name,FALSE),"wb")) == NULLV)
	    v3_error(V3E_CREATEXCT,"PCK","SAVE","CREATEXCT","Could not save into xct file",tstr) ;
	    file_ptr1 = NULLV ;		/* Only save xct-only */
	 } ;
/*	Do we save parse info ? */
	if (file_ptr1 != NULLV)
	 { ph.is_parse = TRUE ; fwrite(&ph,sizeof ph,1,file_ptr1) ;
	   fwrite(prsp,sizeof *prsp,1,file_ptr1) ; fclose(file_ptr1) ;
	 } ;
/*	   Do we save xct-only ? */
	if (file_ptr2 != NULLV)
	 { fo = (sizeof ph + ALIGN_MAX) & ~ALIGN_MAX ;		/* Figure out where everything is to go first */
#define PHU(PHN,PRN,B) if (prsp->PRN==0)prsp->PRN=20 ; ph.PHN.loc = fo ; fo+=(ph.PHN.bytes = (((prsp->PRN*B)+ALIGN_MAX)&~ALIGN_MAX))
/*	   Write out impure areas first so we can write-protect pages later on */
/*	   prsp->skeletons.length +=
		 (512 - ((sizeof ph + prsp->impure_free*SIZEofINT + prsp->skeletons.length) % 512)) ; */
	   PHU(impure,impure_free,SIZEofINT) ; PHU(skeletons,skeletons.length,1) ;
#ifdef V3_VMS_PAGE_SIZE
	   i = V3_VMS_PAGE_SIZE - (fo % V3_VMS_PAGE_SIZE) ;		/* Want to align pure on page boundary */
#else
	   i = 512 - (fo % 512) ;		/* Want to align pure on page boundary */
#endif
	   fo += i ; prsp->skeletons.length += i ; ph.skeletons.bytes += i ;
	   ph.total_nonshare_bytes = fo ;	/* Record number bytes nonsharable */
/*	   PHU(code,code_offset,1) ; fo += prsp->code_offset ; */
	   if (prsp->code_offset < sizeof (struct db__module_entry)) prsp->code_offset = sizeof (struct db__module_entry) ;
	   ph.code.loc = fo ; fo += (ph.code.bytes = (((2*prsp->code_offset)+ALIGN_MAX)&~ALIGN_MAX)) ;
	   PHU(pure,pure_free,SIZEofINT) ; PHU(checksum,checksums.length,1) ;
	   PHU(ob_bucket,ob_bucket.length,1) ; PHU(name_table,name_table.length,1) ;
	   for (i=0;i<V3_PACKAGE_NAME_OB_TABLE_MAX;i++)
	    { ph.name_ob_pairs[i].loc = fo ; fo += (ph.name_ob_pairs[i].bytes = ((prsp->name_ob_pairs[i].length+ALIGN_MAX)&~ALIGN_MAX)) ;
	    } ;
/*	    { PHU(name_ob_pairs[i],(name_ob_pairs[i].length+ALIGN_MAX)&~ALIGN_MAX) ; } ; */
/*	   Now write out everything, starting with header */
	   ph.package_checksum = time(NULLV) ;	/* Blow in a dummy (but unique) checksum */
	   fo += sizeof buffer ;
	   ph.is_parse = FALSE ; ph.total_xct_bytes = fo ;
	   fwrite(&ph,(sizeof ph+ALIGN_MAX)&~ALIGN_MAX,1,file_ptr2) ;
#define PHW(LOC,LEN) fwrite(&prsp->LOC,(prsp->LEN+ALIGN_MAX)&~ALIGN_MAX,1,file_ptr2)
	   PHW(impure_buf,impure_free*SIZEofINT) ; PHW(skeletons,skeletons.length) ;
	   fwrite(&prsp->code,(2*(prsp->code_offset)+ALIGN_MAX)&~ALIGN_MAX,1,file_ptr2) ;
	   PHW(pure_buf,pure_free*SIZEofINT) ; PHW(checksums,checksums.length) ;
	   PHW(ob_bucket,ob_bucket.length) ; PHW(name_table,name_table.length) ;
	   for (i=0;i<V3_PACKAGE_NAME_OB_TABLE_MAX;i++)
	    { PHW(name_ob_pairs[i],name_ob_pairs[i].length) ; } ;
	   memset(buffer,0,sizeof buffer) ; fwrite(buffer,sizeof buffer,1,file_ptr2) ;
/*	   Looks ok, close up shop	*/
	   fclose(file_ptr2) ;
	 } ;
	return ;
}

/*	P A C K A G E   U N L O A D				*/

/*	pcku_unload - Removes a package from V3 process		*/

void pcku_unload(pckp)
  struct db__package *pckp ;		/* Pointer to package to remove */
{ struct db__package *xpckp ;	/* Another package to scan */
   short int pi,xi ;

	if (pckp->package_id == psi->package_index)
	 v3_error(V3E_NOTCURRENT,"PCK","UNLOAD","NOTCURRENT","Cannot unload current package",pckp->package_name) ;
/*	If load for parsing then only have to free the parse block */
	if (pckp->parse_ptr != NULLV) { v4mm_FreeChunk(pckp->parse_ptr) ; }
	 else
	 {
	   v4mm_FreeChunk(pckp->code_ptr) ; v4mm_FreeChunk(pckp->impure_ptr) ; v4mm_FreeChunk(pckp->pure_ptr) ; v4mm_FreeChunk(pckp->checksum_ptr) ;
	 } ;
/*	Scan thru all other packages external tables & zap any references to this package */
	for (pi=0;pi<V3_PACKAGE_MAX;pi++)
	 { if ((xpckp = process->package_ptrs[pi]) == NULLV) continue ;
	   if (pi == pckp->package_id) continue ;
/*	   Scan all externals in this package */
	 } ;
/*	And finally remove from process	*/
	process->package_ptrs[pckp->package_id] = NULLV ;
	v4mm_FreeChunk(pckp) ;
/*	And maybe we have to redo object search list */
	sobu_calculate_name_ob_list() ;

	return ;
}

/*	P A C K A G E   G L O B A L  /  E X T E R N A L   R E S O L U T I O N	*/

/*	pcku_load_package - Called on package load to resolve global/external symbols	*/

void pcku_load_package(pckp)
  struct db__package *pckp ;		/* Pointer to newly loaded package */
{ struct db__module_entry *mep ;
  HANGLOOSEDCL

/*	Just loop thru all modules in the package, linking each one */
	mep=(struct db__module_entry *)pckp->code_ptr ;
	if (mep->name[0] == 0) { HANGLOOSE(100) ; } ;	/* If no module then maybe wait for other process to init global segment! */
	for(;mep->name[0]!=0;mep=(struct db__module_entry *)((char *)mep+mep->next_module_offset))
	 { pcku_load_module(mep) ;
	   if (mep->next_module_offset == 0) break ;
	 } ;
}

/*	pcku_load_module - Links a module into global module-hash-entry tables for process */

void pcku_load_module(mep)
  struct db__module_entry *mep ;
{ struct db__module_runtime_entry *mre ;
  struct db__module_link_list *mll ;
  int hash,hx,rx,count,bytes ;

	if (mep->module_link_offset == 0) { count = 0 ; }
	 else { mll = (struct db__module_link_list *)((char *)mep + mep->module_link_offset) ;
		count = mll->count ;				/* Find number of modules, defer binding until later */
	      } ;
	if (process->mht.count >= V3_PROCESS_MODULE_MAX)
	 v3_error(V3E_MAXPROC,"PCKU","LOADMOD","MAXPROC","Exceeded max number of modules per process",(void *)V3_PROCESS_MODULE_MAX) ;
	hash = prsu_str_hash(mep->name,-1) ;
	for(hx=hash;;hx++)
	 { hx = hx % V3_PROCESS_MODULE_MAX ;
	   if (process->mht.entry[hx].module_hash == 0)
	    { process->mht.entry[hx].module_hash = hash ; strcpy(process->mht.entry[hx].name,mep->name) ;
	      process->mht.count ++ ; break ;		/* Empty slot - new module */
	    } ;
	   if (process->mht.entry[hx].module_hash != hash) continue ;
	   if (strcmp(process->mht.entry[hx].name,mep->name) == 0) break ;	/* Module already in table */
	 } ;
	if ((rx = process->mht.entry[hx].runtime_index) == 0)
	 { rx = (process->mht.entry[hx].runtime_index = (++process->mht.last_module_rt_id)) ;	/* Allocate new module id */
	   bytes = sizeof *mre - ( (V3_PRS_MODULE_LIST_MAX-count) * sizeof mre->ext_mre[0] ) ;
	   mre = (process->mre[rx] = (struct db__module_runtime_entry *)v4mm_AllocChunk(bytes,TRUE)) ;
	   mre->module_rt_id = process->mht.last_module_rt_id ;
/*	   Allocate module specific impure if necessary */
	   if (mep->impure_words != 0) mre->impurelm_base = (int *)v4mm_AllocChunk(mep->impure_words*SIZEofINT,TRUE) ;
	 } else { printf("\n[WARNING: package load is redefining module (%s) - MAY CAUSE PROBLEMS!]\n\n",mep->name) ;
		  printf("mep->name=%s, mep->package_id=%d, rx=%d, hx=%d, process->mht.entry[hx].runtime_index=%d, mht.entry[hx].name=%s, mht->count=%d\n",
				mep->name,mep->package_id,rx,hx,process->mht.entry[hx].runtime_index,process->mht.entry[hx].name,process->mht.count) ;
		} ;
	mre = process->mre[rx] ;				/* Link to (new) runtime entry block */
	mre->mentry = mep ; mre->count = count ; strcpy(mre->name,mep->name) ;
}

/*	pcku_module_unload - Unloads a module from the V3/V4 Environment	*/

int pcku_module_unload(mep)
  struct db__module_entry *mep ;
{
  int hash,hx,ttlbytes ;

	hash = prsu_str_hash(mep->name,-1) ;
	for(hx=hash;;hx++)
	 { hx = hx % V3_PROCESS_MODULE_MAX ;
	   if (process->mht.entry[hx].module_hash == 0) return(0) ;		/* Module NOT defined */
	   if (process->mht.entry[hx].module_hash != hash) continue ;
	   if (strcmp(process->mht.entry[hx].name,mep->name) == 0) break ;	/* Module already in table */
	 } ;
	ttlbytes = mep->impure_words * SIZEofINT + mep->Bytes ;
	if (mep->impure_words != 0)
	 v4mm_FreeChunk(process->mre[process->mht.entry[hx].runtime_index]->impurelm_base) ; /* Free any impure associated with module */
	v4mm_FreeChunk(process->mre[process->mht.entry[hx].runtime_index]) ;		/* Free mre */
	v4mm_FreeChunk(mep) ;							/* Free mep & following PIC/pure/etc */
	process->mht.entry[hx].module_hash = 0 ; process->mht.entry[hx].name[0] = 0 ;
	process->mht.entry[hx].runtime_index = 0 ; process->mht.count -- ;
	return(ttlbytes) ;						/* Return # bytes freed */
}

/*	pcku_module_defined - Returns mep if string module defined within process */

struct db__module_entry *pcku_module_defined(name)
  char *name ;
{ int hx,hash ;

	hash = prsu_str_hash(name,-1) ;
	for(hx=hash;;hx++)
	 { hx = hx % V3_PROCESS_MODULE_MAX ;
	   if (process->mht.entry[hx].module_hash == 0) return(NULLV) ;		/* Module NOT defined */
	   if (process->mht.entry[hx].module_hash != hash) continue ;
	   if (strcmp(process->mht.entry[hx].name,name) == 0) break ;	/* Module already in table */
	 } ;
	if (process->mht.entry[hx].runtime_index == 0) return(NULLV) ;		/* Module recorded, but not defined */
	return(process->mre[process->mht.entry[hx].runtime_index]->mentry) ;
}

/*	P A C K A G E   U T I L I T I E S			*/

/*	pcku_find - Looks for package based on name, returns NULLV or pointer to it */

struct db__package *pcku_find(name,flag)
  char *name ;			/* Package name to search for */
  int flag ;			/* If TRUE then generate error if no match, otherwise return NULLV */
{ struct db__package *pckp ;
   short int i ; char tname[V3_PACKAGE_NAME_MAX+1] ;

/*	Convert name to all upper case */
	for (i=0;i<V3_PACKAGE_NAME_MAX;i++)
	 { tname[i] = (*(name+i) >= 'a' ? *(name+i)-32 : (*(name+i) == '-' ? '_' : *(name+i))) ;
	   if (tname[i] == NULLV) break ;
	 } ;
/*	First look for special package name: "SELF"	*/
	if (strcmp("SELF",tname) == 0) return(process->package_ptrs[psi->package_index]) ;
/*	Scan thru all loaded packages */
	for (i=0;i<V3_PACKAGE_MAX;i++)
	 { if ((pckp = process->package_ptrs[i]) == NULLV) continue ;
	   if (strcmp(tname,pckp->package_name) == 0) return(pckp) ;
	 } ;
/*	None found - Either generate error or return NULLV */
	if (flag) v3_error(V3E_NOTLOADED,"PCK","FIND","NOTLOADED","Package not currently loaded",name) ;
	return(NULLV) ;
}

/*	P R O C E S S   F O R K   R O U T I N E S		*/

/*	fork_delete - Deletes an existing fork			*/

void fork_delete()
{ char tname[V3_PRS_SYMBOL_MAX+1] ;
   int i ;

/*	Pick up fork name from stack */
	stru_popcstr(tname) ; stru_csuc(tname) ;
	for (i=0;i<V3_PROCESS_FORK_MAX;i++)
	 { if (strcmp(tname,process->fork[i].name) == 0)
	    { if (i == 0 || i == process->current_fork)
	       v3_error(V3E_NOTCUR,"FORK","DELETE","NOTCUR","Cannot delete current or BOOT fork",(void *)i) ;
/*	      Got index of fork to delete - free storage */
	      process->fork[i].name[0] = NULLV ; process->fork[i].current_psi = NULLV ;
	      return ;
	    } ;
	 } ;
/*	If here then could not find fork */
	v3_error(V3E_NOFORK,"FORK","DELETE","NOFORK","No such fork",(void *)tname) ;
}

/*	fork_new - Creates a new fork within process		*/

void fork_new()
{ char tname[V3_PRS_SYMBOL_MAX+1] ;
   int i ;
   struct db__psi *tpsi ;

/*	Pick up the name & see if already exists */
	stru_popcstr(tname) ; stru_csuc(tname) ;
	for (i=0;i<V3_PROCESS_FORK_MAX;i++)
	 { if (strcmp(tname,process->fork[i].name) == 0)
	    v3_error(V3E_FORKEXISTS,"FORK","NEW","FORKEXISTS","Fork already exists",tname) ;
	 } ;
/*	No such name, find free slot */
	for (i=0;i<V3_PROCESS_FORK_MAX;i++)
	 { if (process->fork[i].current_psi == 0)
	    { strcpy(process->fork[i].name,tname) ;
	      if (process->fork[i].stack_base == NULLV) process->fork[i].stack_base = (V3STACK *)v4mm_AllocChunk(SIZEofSTACK*V3_PROCESS_STACK_SIZE,TRUE) ;
	      if (process->fork[i].startup_psi == NULLV) process->fork[i].startup_psi = (struct db__psi *)v4mm_AllocChunk(sizeof *tpsi,TRUE) ;
/*	      Create a base psi and link it up */
	      process->fork[i].current_psi = (tpsi = process->fork[i].startup_psi) ;
	      tpsi->prior_psi_ptr = (psi->command_psi != NULLV ? psi->command_psi : psi) ;
	      tpsi->reset_stack_ptr = (tpsi->stack_ptr = process->fork[i].stack_base + V3_PROCESS_STACK_SIZE - 2) ;
	      return ;
	    } ;
	 } ;
	v3_error(V3E_NOFREE,"FORK","NEW","NOFREE","No more available FORK slots",(void *)V3_PROCESS_FORK_MAX) ;
}

/*	fork_switch - Switches forks around			*/

void fork_switch()
{ struct db__psi *tpsi ;
   char tname[V3_PRS_SYMBOL_MAX+1] ;
   int i ;

/*	Look for the name */
	stru_popcstr(tname) ; stru_csuc(tname) ;
	for (i=0;i<=V3_PROCESS_FORK_MAX;i++)
	 { if (strcmp(tname,process->fork[i].name) == 0) goto found_fork ; } ;
	v3_error(V3E_NOFORK,"FORK","SWITCH","NOFORK","No such fork",tname) ;
/*	Found the fork - Make sure not the current */
found_fork:
	if (i == process->current_fork)
	 v3_error(V3E_INCUR,"FORK","SWITCH","INCUR","Cannot SWITCH to current fork",tname) ;
/*	Looks ok, "push" another psi on new fork as go-between */
	tpsi = (struct db__psi *)((char *)process->fork[i].current_psi->stack_ptr - ((sizeof *tpsi + ALIGN_MAX) & ~ALIGN_MAX)) ;
	*tpsi = *psi ;
	tpsi->reset_stack_ptr = (tpsi->stack_ptr = (V3STACK *)tpsi) ;	/* Stack is that of new fork */
	tpsi->prior_psi_ptr = process->fork[i].current_psi ;
	process->fork[process->current_fork].current_psi = psi ;
	process->current_fork = i ; psi = tpsi ;
/*	Return & see what happens ! */
	return ;
}

/*	D E B U G G I N G   R O U T I N E S			*/

/*	dbg_break - Here to handle break instruction		*/

void dbg_break()
{ struct db__breakpoints *bpl ;
   short int i,bpx,offset ;
   char package[50],module[50] ;

/*	Figure out which breakpoint we are at			*/
	bpl = process->package_ptrs[psi->package_index]->bp_ptr ;
	offset = psi->code_ptr - process->package_ptrs[psi->package_index]->code_ptr - 1 ;
	for (bpx=0;bpx<V3_DBG_BP_MAX;bpx++)
	 { if (offset == bpl->bp[bpx].offset) goto found_bp ; } ;
	v3_error(V3E_UNKBREAK,"DBG","BREAK","UNKBREAK","Encountered BREAK instruction from unknown location",(void *)offset) ;
found_bp:
/*	Tell user where we are */
	strcpy(package,process->package_ptrs[psi->package_index]->package_name) ;
	if (psi->mep == NULLV) { strcpy(module,"*interactive*") ; }
	 else { strcpy(module,psi->mre->name) ; } ;
	printf("\012V3.BP.%d - %s.%s.%d\015\012",bpx+1,package,module,psi->line_number) ;
/*	Reset psi->code_ptr for proper restart */
	psi->code_ptr = (V3OPEL *)&bpl->bp[bpx].restart ;
/*	Now enter interpretive mode */
	xctu_call_parser(psi->package_index,NULLV,"V3DBG>",TRUE) ;
/*	That's all there is to it */
	return ;
}

/*	dbg_clearbp - Clears one or more breakpoints in package */

void dbg_clearbp()
{ struct db__breakpoints *bpl ;
   union val__format format ;
   union db__module_code_element *code_ptr ;
   struct db__psi *tpsi ;
   short int i,minx,maxx ;
   int offset ;

/*	Get pointer to breakpoint list */
	bpl = process->package_ptrs[psi->package_index]->bp_ptr ;
/*	Pick up argument - either a code location or number */
	POPF(format.all) ;
	if (format.fld.type == VFT_MODREF)
	 { POPVI(offset) ;
/*	   Look for breakpoint at location */
	   for (i=0;i<bpl->count;i++)
	    { if (bpl->bp[i].offset == offset) goto found_slot ; } ;
	   v3_error(V3E_BPNOTSET,"DBG","CLEARBP","BPNOTSET","No breakpoint at specified location or with number",(void *)offset) ;
	 } ;
/*	Argument best be a number */
	PUSHF(format.all) ; i = xctu_popint() ;
	if (i < 0 || i > V3_DBG_BP_MAX) v3_error(V3E_BADBPNUM,"DBG","CLEARBP","BADBPNUM","Invalid breakpoint number",(void *)i) ;
	if (i != 0 && bpl->bp[i-1].offset == 0)
	 v3_error(V3E_BPNOTSET,"DBG","CLEARBP","BPNOTSET","No breakpoint at specified location or with number",(void *)offset) ;
/*	Here to clear bp(s) denoted by i */
found_slot:
/*	Here to clear all/some bps */
	minx = (i == 0 ? 0 : i-1) ; maxx = (i == 0 ? V3_DBG_BP_MAX : i) ;
	for (i=minx;i<maxx;i++)
	 { if (bpl->bp[i].offset == 0) continue ;
	   code_ptr = (union db__module_code_element *)(process->package_ptrs[psi->package_index]->code_ptr + bpl->bp[i].offset) ;
	   code_ptr->all = bpl->bp[i].restart[0].all ; bpl->bp[i].offset = 0 ;
/*	   Now have to loop thru psi levels & reset any levels at break point */
	   for (tpsi=psi;tpsi!=NULLV;tpsi=tpsi->prior_psi_ptr)
	    { if (tpsi->code_ptr == (V3OPEL *)&bpl->bp[i].restart) tpsi->code_ptr = (V3OPEL *)code_ptr ; } ;
	 } ;
	return ;
}

/*	dbg_inst_length - Returns number of code_elements required by an instruction */

int dbg_inst_length(ocarg)
  V3OPEL ocarg ;
{ static unsigned char first_time=TRUE ;
   static unsigned char op_lens[250] ;
   union db__module_code_element op_code ;
   short int i ;

	op_code.all = ocarg ;
/*	If first call then have to set up some arrays */
	if (first_time)
	 { first_time = FALSE ;
	   op_lens[V3_XCT_BRANCH] = 2 ;
	   op_lens[V3_XCT_DOLOOP1] = 2 ;
	   op_lens[V3_XCT_DOLOOP2] = 2 ;
	   op_lens[V3_XCT_DOLOOP3] = 2 ;
	   op_lens[V3_XCT_DUMMYREF] = 3 ;
	   op_lens[V3_XCT_DUMMYREFX] = 3 ;
	   op_lens[V3_XCT_IF] = 3 ;
	   op_lens[V3_XCT_JUMPC] = 4 ;
	   op_lens[V3_XCT_LOOP_TEST] = 2 ;
	   op_lens[V3_XCT_LOOP_TEST_TRUE] = 2 ;
	   op_lens[V3_XCT_EOSLN] = 2 ; op_lens[V3_XCT_IF] = 3 ;
	   op_lens[V3_XCT_JUMP] = 3 ;
	 } ;
	switch (op_code.fld.type)
	 { default: return(1) ;
	   case CODE_FREQOP:
	   case CODE_OPERATION:
		return(op_lens[op_code.fld.offset] == 0 ? 1 : op_lens[op_code.fld.offset]) ;
	 } ;
}

/*	dbg_setbp - Sets a breakpoint in package code		*/

void dbg_setbp()
{ union val__format format ;
   union db__module_code_element *code_ptr ;
   struct db__breakpoints *bpl ;
   short int i,j,offset,ins_len ;

/*	Pop off location reference for loc of breakpoint */
	POPF(format.all) ; POPVI(offset) ;
	if (format.fld.type != VFT_MODREF || format.fld.mode != VFM_OFFSET)
	 v3_error(V3E_INVSETBPARG,"DBG","SETBP","INVSETBPARG","Argument to DBG_SETBP is not a code location",(void *)format.all) ;
/*	See if break point already set */
	bpl = process->package_ptrs[psi->package_index]->bp_ptr ;
	for (i=0;i<V3_DBG_BP_MAX;i++)
	 { if (bpl->bp[i].offset == offset)
	    { PUSHINT(i+1) ; return ; } ;
	 } ;
/*	Next see if we have a free slot */
	for (i=0;i<V3_DBG_BP_MAX;i++)
	 { if (bpl->bp[i].offset == 0) goto free_slot ; } ;
	v3_error(V3E_NOFREEBPS,"DBG","SETBP","NOFREEBPS","No more breakpoint slots available",(void *)V3_DBG_BP_MAX) ;
/*	Set breakpoint in slot i */
free_slot:
	bpl->bp[i].offset = offset ;
	code_ptr = (union db__module_code_element *)(process->package_ptrs[psi->package_index]->code_ptr + offset) ;
/*	Save existing code element in bp table */
	bpl->bp[i].restart[0].all = code_ptr->all ; code_ptr->all = V3_XCT_DBG_BREAK ;
/*	Now copy additional bytes as is needed for instruction length */
	ins_len = dbg_inst_length(bpl->bp[i].restart[0].all) ;
	for (j=1;j<ins_len;j++)
	 { bpl->bp[i].restart[j].all = (++code_ptr)->all ; } ;
/*	Now add jump back into main code */
	bpl->bp[i].restart[j++].all = V3_XCT_JUMP ;
	bpl->bp[i].restart[j++].all = offset + ins_len ;
/*	Finally return breakpoint number */
	PUSHINT(i+1) ; return ;
}

/*	dbg_show_calls - Shows nesting of module calls		*/

void dbg_show_calls()
{ struct db__psi *tpsi ;
   union val__format format ;
   char package[50],module[50] ;
   short int i,cnt ; char *strp ;

/*	See how many calls */
	POPF(format.all) ;
	if (format.all == V3_FORMAT_EOA)
	 { POPVI(cnt) ; cnt = 999 ; }
	 else { PUSHF(format.all) ; cnt = xctu_popint() ; } ;
/*	If count is negative then user wants particular stack frame loc returned as string */
	if (cnt < 0) goto return_string ;
/*	Show requested number of calls */
	for (tpsi=psi->prior_psi_ptr;tpsi!=NULLV && cnt>0;(tpsi=tpsi->prior_psi_ptr,cnt--))
	 { if (process->package_ptrs[tpsi->package_index] == NULLV) { sprintf(package,"PSLOT(%d)",tpsi->package_index) ; }
	    else strcpy(package,process->package_ptrs[tpsi->package_index]->package_name) ;
	   if (tpsi->mep == NULLV) { strcpy(module,"*interactive*") ; }
	    else { strcpy(module,tpsi->mre->name) ; } ;
/*	   Print out the location */
	   printf("   %s.%s.%d\015\012",package,module,tpsi->line_number) ;
	 } ;
	return ;
/*	Here to return string representation of frame */
return_string:
	POPF(i) ; POPVI(i) ;	/* Pop of the eoa */
	for (tpsi=psi->prior_psi_ptr;tpsi!=NULLV && cnt<-1;(tpsi=tpsi->prior_psi_ptr,cnt++)) ;
/*	If end of stack then return zero length string */
	if ((tpsi == NULLV ? TRUE : tpsi->code_ptr == NULLV))
	 { PUSHVI(NULLV) ; PUSHF(V3_FORMAT_FIXSTR) ; return ; } ;
/*	Have to generate a string, need to allocate */
	strp = (char *)stru_alloc_bytes(100) ;
	strcpy(package,process->package_ptrs[tpsi->package_index]->package_name) ;
	if (tpsi->mep == NULLV) { strcpy(module,"*interactive*") ; }
	 else { strcpy(module,tpsi->mre->name) ; } ;
	sprintf(strp,"%s.%s.%d",package,module,tpsi->line_number) ;
	PUSHMP(strp) ; PUSHF(V3_FORMAT_VARSTR) ;
	return ;
}
