/*	V3SOBU.C - SEMANTIC OBJECT ROUTINES FOR VICTIM III

	LAST EDITED 6/10/84 BY VICTOR E. HANSEN		*/

#include <time.h>
#include "v3defs.c"

/*	External linkages				*/
extern struct db__process *process ;
extern struct db__psi *psi ;

/*	Junk to calculate sizes of objects based on pointer differences */
#define PRSLEN(L1,L2) ((char *)L1-(char *)L2)

/*	Declare some hash/cache tables		*/

static int sob_find_name[V3_SOB_FIND_HASH] ;
static int sob_find_obj[V3_SOB_FIND_HASH] ;
static int sob_hash_src_obj[V3_SOB_HAS_HASH] ;		/* Source object for caching */
static int sob_hash_has_name[V3_SOB_HAS_HASH] ;		/* Has name */
static int sob_hash_has_obj[V3_SOB_HAS_HASH] ;		/* Object that source "has" */

struct ob__desc *sobu_pointer() ;
char *xctu_pointer() ;

/*	N A M E  /  O B J E C T   S E A R C H   U T I L I T I E S  */

/*	sobu_calculate_name_ob_list - Figures out proper order to search
		name lists after assertions have changed or a package
		has been added/deleted		*/
void sobu_calculate_name_ob_list()
{ struct {
     char count ;		/* Number below */
     struct db__name_ob_pairs *oklist[V3_PROCESS_ASSERTION_MAX] ; /* List of ok tables */
    } asrt_num[V3_ASSERTIONS_PER_TABLE_MAX] ;
   struct db__package *pckp ;
   short int i,j,pi,noi,noai,pai ;
   int asrt_hash ;
   struct db__name_ob_pairs *not ;

/*	First scan thru all package name-ob tables & update list */
	for (i=0;i<V3_ASSERTIONS_PER_TABLE_MAX;i++) asrt_num[i].count = 0 ;
	for (pi=0;pi<V3_PACKAGE_MAX;pi++)
	 { if ((pckp = process->package_ptrs[pi]) == NULLV) continue ;
/*	   Have a loaded package, look at all name-ob tables */
	   for (noi=0;noi<V3_PACKAGE_NAME_OB_TABLE_MAX;noi++)
	    { if ((not = pckp->name_ob_ptrs[noi]) == NULLV) continue ;
	      if (not->num_names == 0) continue ;	/* Nothing in this table ! */
/*	      Have a name table, make sure each of its assertions is in process assertion list */
	      for (noai=0;noai<not->num_assertions;noai++)
	       { asrt_hash = not->assertion_hash[noai] ;
		 for (pai=0;pai<process->assertion_count;pai++)
		  { if (asrt_hash != process->assertion_hash[pai]) continue ;
		    if (strcmp(not->assertions[noai],process->assertions[pai]) != 0) continue ;
		    goto this_assertion_ok ; /* Got a match */
		  } ;
/*		 If here then no match for assertion - go on to next package table */
		 goto next_package_table ;
this_assertion_ok: ;
	       } ;
/*	      Here then got valid table - insert into matrix */
	      if (asrt_num[noai].count >= V3_PROCESS_ASSERTION_MAX-1)
	       { printf("Max number of assertions noai=%d, count=%d, package=%d\n",noai,asrt_num[noai].count,pi) ;
	         continue ;
	       } ;
	      asrt_num[noai].oklist[asrt_num[noai].count++] = not ;
next_package_table: ;
	    } ;
	 } ;
/*	At this point have table(s) of assertions, load into process */
	process->name_ob_count = 0 ;
	for (i=V3_ASSERTIONS_PER_TABLE_MAX-1;i>=0;i--)
	 { for (j=0;j<asrt_num[i].count;j++)
	    { lip:
	       if (process->name_ob_count >= V3_PROCESS_ASSERTION_MAX-1)
	        { printf("Max name_ob_ptrs for process\n") ; continue ; } ;
	       process->name_ob_ptrs[process->name_ob_count++] = asrt_num[i].oklist[j] ;
	    } ;
	 } ;
/*	Before returning, best to flush out the cache/hash lists */
	for(i=0;i<V3_SOB_FIND_HASH;i++) { sob_find_name[i] = 0 ; } ;
	for(i=0;i<V3_SOB_HAS_HASH;i++) { sob_hash_src_obj[i] = 0 ; } ;
	return ;
}

/*	N A M E - O B J E C T  T A B L E   R O U T I N E S	*/

/*	sobu_insert_name_ob - Inserts name-object pair into proper table */

void sobu_insert_name_ob(tobp,package_id)
  struct db__dcl_object *tobp ;		/* Pointer to temp object descriptor to be inserted */
  int package_id ;			/* Package id */
{ struct db__name_ob_pairs *nopp ;	/* Points to name-ob table */
   struct db__package *pckp ;
   short int i,j,noti ;

	if ((pckp = process->package_ptrs[package_id]) == NULLV)
	 v3_error(V3E_PCKNOTLOADED,"OBJ","INSERT_NAME_OB","PCKNOTLOADED","Package not loaded",(void *)package_id) ;
/*	Loop thru tables for this package looking for one with same assertions */
	for (noti=0;noti<V3_PACKAGE_NAME_OB_TABLE_MAX;noti++)
	 { if ((nopp = pckp->name_ob_ptrs[noti]) == NULLV) continue ;
	   if (nopp->num_names == 0) continue ;
	   if (tobp->num_assertions != nopp->num_assertions) continue ;
/*	   Number of assertions match, see if strings do */
	   for (i=0;i<tobp->num_assertions;i++)
	    { for (j=0;j<tobp->num_assertions;j++)
	       { if (strcmp(nopp->assertions[i],tobp->assertions[j]) == 0) goto assertion_match ;
	       } ;
/*	      If here then no match on one of the assertions, continue */
	      goto next_assertion ;
assertion_match: ;
	    } ;
/*	   If here then got a match - insert into this table */
	   goto insert_name_ob ;
	   return ;
next_assertion: ;
	 } ;
/*	If here then no matches - have to create another table */
no_table_match:
/*	Look for free slot in package */
	for (noti=0;noti<V3_PACKAGE_NAME_OB_TABLE_MAX;noti++)
	 { if (pckp->name_ob_ptrs[noti]->num_names == 0) goto got_slot ; } ;
	v3_error(V3E_PCKFULL,"OBJ","INSERT_NAME_OB","PCKFULL","No more table slots in package",(void *)V3_PACKAGE_NAME_OB_TABLE_MAX) ;
got_slot:
	nopp = pckp->name_ob_ptrs[noti] ;
/*	Init the new table */
	nopp->package_id = package_id ;
	for (i=0;i<tobp->num_assertions;i++)
	 { strcpy(nopp->assertions[i],tobp->assertions[i]) ;
	   nopp->assertion_hash[i] = tobp->assertion_hash[i] ;
	 } ;
	nopp->num_assertions = tobp->num_assertions ;
/*	Here to insert name-ob pair into table */
insert_name_ob:
	if (nopp->num_names >= V3_NAME_OB_PAIR_MAX)
	 v3_error(V3E_TBLFULL,"OBJ","INSERT_NAME_OB","TBLFULL","Table is full",(void *)V3_NAME_OB_PAIR_MAX) ;
	nopp->name_ob_pair[nopp->num_names].name_id.all = tobp->name_id.all ;
	nopp->name_ob_pair[nopp->num_names++].ob_id.all = tobp->ob_id.all ;
	nopp->length = PRSLEN(&nopp->name_ob_pair[nopp->num_names+1],nopp) ;
/*	Finally see if this table belongs in process search list */
	if (nopp->num_names == 1) sobu_calculate_name_ob_list() ;
	return ;
}

/*	N A M E   C R E A T I O N  &  L O O K U P		*/

/*	sobu_name_create - Creates a new name from a string for package */

int sobu_name_create(name,package_id)
  char *name ;				/* Pointer to name string */
  int package_id ;			/* Package id */
{ short int i ;
   int hash ;
   struct sob__table *tp ;		/* Pointer to name table */
   union val__sob tob ;		/* Temp semantic object */

/*	First see if name already exists	*/
	if ((tob.all = sobu_name_lookup(name,package_id)) != NULLV) return(tob.all) ;
/*	It don't - have to create it in specified package */
/*	xxx - check to see if package open for update */
	if ((tp = process->package_ptrs[package_id]->name_table_ptr) == NULLV)
	 v3_error(V3E_PCKNOTLOADED,"OBJ","NAME_CREATE","PCKNOTLOADED","Package not loaded",(void *)package_id) ;
/*	Make sure we can fit it in */
	if ((i = tp->count) >= V3_SOB_NAME_TABLE_MAX)
	 v3_error(V3E_NAMETBLFULL,"OBJ","NAME_CREATE","NAMETBLFULL","Name table is full",(void *)V3_SOB_NAME_TABLE_MAX) ;
	tp->tentry[i].hash = prsu_str_hash(name,-1) ;
	strcpy(tp->tentry[i].name,name) ;
/*	Construct a new name object */
	tob.all = 0 ; tob.fld.id = VAL_SOB ; tob.fld.type = SOT_NAME ;
	tob.fld.package_id = package_id ; tob.fld.id_num = i + V3_SOB_NAME_ID_OFFSET ;
	tp->tentry[tp->count++].sob_val.all = tob.all ;
/*	Update table length for package routines */
	tp->length = PRSLEN(&tp->tentry[tp->count],tp) ;
/*	Now return it		*/
	return(tob.all) ;
}

/*	sobu_name_lookup - Does name lookup for string/package	*/
/*	returns NULLV if no name found				*/

int sobu_name_lookup(name,package_id)
  char *name ;				/* Pointer to name string */
  int package_id ;			/* Package id */
{ short int i,pi ;
   int hash ;
   struct sob__table *tp ;		/* Pointer to name table */
   struct db__package *pckp ;
   union val__sob tob ;		/* Temp semantic object */

/*	First hash the string */
	hash = prsu_str_hash(name,-1) ;
/*	If argument package_id is null then search all loaded ones */
	if (package_id == NULLV)
	 { for (pi=0;pi<V3_PACKAGE_MAX;pi++)
	    { if ((pckp = process->package_ptrs[pi]) == NULLV) continue ;
	      if ((tp = pckp->name_table_ptr) == NULLV) continue ;
/*	      Have something to search */
NL1:
	      for (i=0;i<tp->count;i++)
	       { if (hash != tp->tentry[i].hash) continue ;
		 if (strcmp(name,tp->tentry[i].name) == 0) return(tp->tentry[i].sob_val.all) ;
	       } ;
	    } ;
	   return(NULLV) ;	/* Name not found anywhere */
	 } ;
/*	Now look in kernel package name table */
	if ((pckp = process->package_ptrs[V3_KERNEL_PACKAGE_ID]) == NULLV) goto no_kernel ;
	if ((tp = pckp->name_table_ptr) != NULLV)
	 { for (i=0;i<tp->count;i++)
	    { if (hash != tp->tentry[i].hash) continue ;
	      if (strcmp(name,tp->tentry[i].name) == 0) return(tp->tentry[i].sob_val.all) ;
	    } ;
	 } ;
no_kernel:
/*	Not in kernel package - Search the given package */
	if ((tp = (struct sob__table *)process->package_ptrs[package_id]->name_table_ptr) == NULLV)
	 v3_error(V3E_NOTBL,"OBJ","NAME_LOOKUP","NOTBL","Package has no name table",(void *)package_id) ;
	for (i=0;i<tp->count;i++)
	 { if (hash != tp->tentry[i].hash) continue ;
	   if (strcmp(name,tp->tentry[i].name) == 0) return(tp->tentry[i].sob_val.all) ;
	 } ;
/*	Can't find the name anywhere - return NULLV */
	return(NULLV) ;
}

/*	N A M E   T R A N S L A T I O N   B E T W E E N   P A C K A G E S	*/

/*	sobu_name_tran - Returns new name id for different package, NULLV if no translation */

int sobu_name_tran(nameob,package_id)
  union val__sob nameob ;		/* Name to be translated */
  int package_id ;			/* Package id */
{ short int i ; short int count ;
   struct sob__table *tp ;		/* Pointer to name table */
   struct sob__table_entry *tep ;	/* Points to entry within a table */

/*	If name is from kernel then don't do anything */
	if (nameob.fld.package_id == V3_KERNEL_PACKAGE_ID) return(nameob.all) ;
/*	If translating to same package then don't do anything */
	if (nameob.fld.package_id == package_id) return(nameob.all) ;
/*	Have to translate - first get original package for corresponding hash/text info */
	if ((tp = process->package_ptrs[nameob.fld.package_id]->name_table_ptr) == NULLV)
	 return(NULLV) ;
/*	Find the name in this table */
	for (i=0;i<tp->count;i++)
	 { if (tp->tentry[i].sob_val.all == nameob.all) goto found_current ; } ;
	return(NULLV) ;	/* Could not find in current ? */
/*	Now try to find same thing in other package	*/
found_current:
	tep = &(tp->tentry[i]) ;
	if ((tp = process->package_ptrs[package_id]->name_table_ptr) == NULLV) return(NULLV) ;
	for (i=0;i<tp->count;i++)
	 { if (tep->hash != tp->tentry[i].hash) continue ;
	   if (strcmp(tep->name,tp->tentry[i].name) == 0) return(tp->tentry[i].sob_val.all) ;
	 } ;
/*	Can't find it - return NULLV */
	return(NULLV) ;
}

/*	H A S   F I N D   					*/

/*	sobu_has_find - Returns obj2 if (obj1 has name) exists, NULLV otherwise */

int sobu_has_find(objall,nameall)
  int objall,nameall ;
{ union val__sob tname ;	/* Temp name object */
  union val__sob obj ;		/* First object */
  union val__sob name ;	/* Name to look for */
  struct ob__desc *obp ;	/* Pointer to object descriptor */
   struct db__dcl_objref *objrp ;
   struct db__dcl_hasref *hasrp ;
   int toball ;			/* Temp object "all" */
   int i,j ;

	obj.all = objall ; name.all = nameall ;
/*	First see if we can get a cache hit */
/*	Hash & make sure i stays positive (yes we got burned BIG time!) */
	i =  ((0x7fffffff & (obj.all + name.all)) % V3_SOB_HAS_HASH) ;
	if (sob_hash_src_obj[i] == obj.all ? sob_hash_has_name[i] == name.all : FALSE) return(sob_hash_has_obj[i]) ;
/*	Get pointer to object's descriptor table */
	if ((obp = sobu_pointer(obj.all)) == NULLV) return(NULLV) ;
/*	Translate name for package of obj */
	if ((tname.all = sobu_name_tran(name,obp->ob_id.fld.package_id)) == NULLV) goto not_in_this_object ;
/*	Look thru all has's for this object */
	hasrp = (struct db__dcl_hasref *)&obp->info[obp->ss_hass] ;
	for (i=0;i<obp->num_hass;(hasrp += 1,i++))
	 { if (tname.all == hasrp->has_name.all) { toball = hasrp->has_ob.all ; goto got_has ; } ; } ;
/*	Not in this object - try any/all of it's "isa's" */
not_in_this_object:
	for (i=0;i<obp->num_isas;i++)
	 { objrp = (struct db__dcl_objref *)&obp->info[obp->ss_isas[i]] ;
/*	   objrp = Pointer to isa reference - is it object or name ? */
	   if (objrp->count == 0)
/*	      It's an object - see if it "has" what we want */
	    { if ((toball = sobu_has_find(objrp->name_ids[0].all,name.all)) != NULLV) goto got_has ;
	    } else
/*	      It's one or more names - evaluate to object if possible */
	    { if ((toball = sobu_object_find(objrp->name_ids[0].all)) == NULLV) continue ;
/*	      Found first object, now try for hass */
	      for (j=1;j<objrp->count;j++)
	       { if ((toball = sobu_has_find(toball,objrp->name_ids[j].all)) == NULLV) break ;
	       } ;
/*	      Did we find it ? */
	      if (objrp->count == 1 || j >= objrp->count)
	       { if ((toball = sobu_has_find(toball,name.all)) != NULLV) goto got_has ;
	       } ;
	     } ;
	 } ;
/*	Can't find it anywhere	*/
	return(NULLV) ;
/*	Here we got a match, save in cache */
got_has:
/*	Hash & make sure i stays positive (yes we got burned BIG time!) */
	i =  ((0x7fffffff & (obj.all + name.all)) % V3_SOB_HAS_HASH) ;
	sob_hash_src_obj[i] = obj.all ; sob_hash_has_name[i] = name.all ; sob_hash_has_obj[i] = toball ;
	return(toball) ;
}

/*	O B J E C T   C R E A T I O N				*/

/*	sobu_object_create - Creates a new object from dcl temp object descriptor */
/*	 (returns pointer to newly created object id) */

struct ob__desc *sobu_object_create(tobp,package_id)
  struct db__dcl_object *tobp ;		/* Temp object descriptor */
  int package_id ;			/* Package id */
{ struct ob__desc *obp ;		/* Points to newly created object */
   struct db__package *pckp ;
   struct db__ob_bucket *bptr ;		/* Points to package object bucket */
   struct db__dcl_objref *objref ;	/* Pointer to object ref (ISA) */
   struct db__dcl_hasref *hasref ;
   struct db__dcl_valref *valref ;
   short int i,obx,bytes ;

/*	Get pointers to package & bucket */
	if ((pckp = process->package_ptrs[package_id]) == NULLV)
	 v3_error(V3E_PCKNOTLOADED,"OBJ","OBJECT_CREATE","PCKNOTLOADED","Package not loaded",(void *)package_id) ;
	if (pckp->parse_ptr == NULLV)
	 v3_error(V3E_PCKNOUPDATE,"OBJ","OBJECT_CREATE","PCKNOUPDATE","Package not loaded for updates",(void *)pckp->package_name) ;
	bptr = pckp->ob_bucket_ptr ;
	if (bptr->count >= V3_SOB_OBS_PER_BUCKET_MAX)
	 v3_error(V3E_TOOMANYOBS,"OBJ","OBJECT_CREATE","TOOMANYOBS","Too many objects in package bucket",(void *)V3_SOB_OBS_PER_BUCKET_MAX) ;
/*	Create a new object id */
	obp = (struct ob__desc *)&bptr->buf[bptr->offsets[bptr->count] = bptr->free_offset] ;
	obp->ob_id.fld.id = VAL_SOB ; obp->ob_id.fld.type = SOT_OBJECT ;
	obp->ob_id.fld.package_id = package_id ; obp->ob_id.fld.id_num = bptr->count ;
	tobp->ob_id.all = obp->ob_id.all ;
/*	Create corresponding name */
	tobp->name_id.all = (obp->name_id.all = sobu_name_create(tobp->obname,package_id)) ;
/*	Set various indicators on value */
	obp->val_needs_parent = tobp->val_needs_parent ; obp->val_is_deferred = tobp->val_is_deferred ;
	obp->val_is_list = tobp->val_is_list ;
	obx = 0 ;		/* obx = offset into info buffer */
/*	Now copy the isas */
	for (i=0;i<tobp->num_isas;i++)
	 { objref = &tobp->isarefs[i] ; bytes = PRSLEN(&objref->name_ids[objref->count+1],objref) ;
	   memcpy(&obp->info[obx],objref,bytes) ; obp->ss_isas[i] = obx ; obx += bytes ;
	 } ; obp->num_isas = tobp->num_isas ;
/*	And the hass */
	obp->num_hass = tobp->num_hass ; obp->ss_hass = obx ;
	for (i=0;i<tobp->num_hass;i++)
	 { memcpy(&obp->info[obx],&tobp->hass[i],sizeof *hasref) ; obx += sizeof *hasref ; } ;
/*	And finally the vals */
oc1:
	obp->num_vals = tobp->num_vals ; obp->ss_vals = obx ;
	for (i=0;i<tobp->num_vals;i++)
	 { memcpy(&obp->info[obx],&tobp->vals[i],sizeof *valref) ; obx += sizeof *valref ; } ;
/*	Figure out the length of this object & update bucket header info */
	obp->length = PRSLEN(&obp->info[obx+1],obp) ;
	if ((bptr->free_offset += ((obp->length + SIZEofINT-1)/SIZEofINT)) >= V3_SOB_BUCKET_BUF_MAX)
	 v3_error(V3E_BCKTFULL,"OBJ","OBJECT_CREATE","BCKTFULL","Object bucket overflow",(void *)V3_SOB_BUCKET_BUF_MAX) ;
	bptr->length = PRSLEN(&obp->info[obx+1],bptr) ;
	bptr->count++ ;		/* Up the object count for bucket */
/*	Return with object id */
	return(obp) ;
}


/*	O B J E C T   F I N D   R O U T I N E			*/

/*	sobu_object_find - Finds object based on name depending on current assertion status */

int sobu_object_find(nameall)
  int nameall ;
{ struct db__name_ob_pairs *notp ;	/* Pointer to name-object table */
  union val__sob name ;		/* Object name */
   union val__sob tname ;
   int i,j,k,tx ;

	name.all = nameall ;
/*	First check out the local cache */
	i = name.all % V3_SOB_FIND_HASH ;
	if (sob_find_name[i] == name.all) return(sob_find_obj[i]) ;
/*	Scan thru process's list of valid name-object tables */
	for (tx=0;tx<process->name_ob_count;tx++)
	 { notp = process->name_ob_ptrs[tx] ;
/*	   Translate name for current package */
	   if ((tname.all = sobu_name_tran(name,notp->package_id)) == NULLV) continue  ;
/*	   If table has not been sorted then have to do linear search */
	   if (!notp->table_sorted)
	    { for (i=0;i<notp->num_names;i++)
	       { if (tname.all == notp->name_ob_pair[i].name_id.all)
		  { j = name.all % V3_SOB_FIND_HASH ; sob_find_name[j] = name.all ; 
		    sob_find_obj[j] = notp->name_ob_pair[i].ob_id.all ;
		    return(sob_find_obj[j]) ;
		  } ;
	       } ;
	      continue ;	/* Name not found - try next table */
	    } ;
/*	   Now do binary search on table */
	   i=0 ; j=notp->num_names ;
	   for (;;)
	    { if (i > j) break ;	/* Not in this table */
	      k = (i+j)/2 ;
	      if (tname.all < notp->name_ob_pair[k].name_id.all)
		{ j = k-1 ; continue ; } ;
	      if (tname.all = notp->name_ob_pair[k].name_id.all)
		return(notp->name_ob_pair[k].ob_id.all) ;
	      i = k+1 ;
	     } ;
/*	    Not in this table - advance to next in process list */
	 } ;
/*	If here then not to be found */
	return(NULLV) ;
}

/*	sobu_object_dereference - Converts string to object 	*/

int sobu_object_dereference(tlobjall,sref)
  int tlobjall ;
  char *sref ;			/* Pointer to string representation */
{ union val__sob ob,nameob ;
  union val__sob tlobj ;	/* Top level object id or NULLV */
   char tbuf[200] ;
   short int i ;

	tlobj.all = tlobjall ;
/*	Get top level object name & try to find it (if tlobj == NULLV)	*/
	if ((ob.all = tlobj.all) == NULLV)
	 { i = strcspn(sref,".") ;
	   strncpy(tbuf,sref,i) ; tbuf[i] = NULLV ; sref += i ;
	   if ((nameob.all = sobu_name_lookup(tbuf,0)) == 0) return(0) ;
	   if ((ob.all = sobu_object_find(nameob.all)) == 0) return(0) ;
	   if (*(sref++) == NULLV) return(ob.all) ;
	 } ;
/*	Found top level object - now try for all hass */
	for (;;)
	 { i = strcspn(sref,".") ;
	   strncpy(tbuf,sref,i) ; tbuf[i] = NULLV ; sref += i ;
	   if ((nameob.all = sobu_name_lookup(tbuf,0)) == 0) return(0) ;
	   if ((ob.all = sobu_has_find(ob.all,nameob.all)) == 0) return(0) ;
	   if (*(sref++) == '\0') return(ob.all) ;
	 } ;
}

/*	O B J E C T   S E A R C H   R O U T I N E		*/

/*	sobu_pointer - Searches for specific semantic object
	  returns pointer to it					*/

struct ob__desc *sobu_pointer(ob_idall)
  int ob_idall ;
{ struct db__package *pckp ;
  union val__sob ob_id ;
   struct db__ob_bucket *bktp ;

	ob_id.all = ob_idall ;
/*	Make sure package identified by object is loaded into process */
	if ((pckp = process->package_ptrs[ob_id.fld.package_id]) == NULLV)
	 v3_error(V3E_PCKNOTLOADED,"OBJ","POINTER","PCKNOTLOADED","Package not loaded",(void *)ob_id.fld.package_id) ;
	bktp = pckp->ob_bucket_ptr ;
/*	All is well, return pointer to object	*/
	return((struct ob__desc *)&bktp->buf[bktp->offsets[ob_id.fld.id_num]]) ;
}

/*	O B J E C T   P R I N T   N A M E   R O U T I N E	*/

/*	sobu_name_str - Converts object/name to print string	*/

char *sobu_name_str(ob_idall)
  int ob_idall ;
{ struct ob__desc *obp ;
  union val__sob ob_id ;
   struct sob__table *tblp ;
   short int i ;

	ob_id.all = ob_idall ;
/*	If passed an object then have to get its name */
	if (ob_id.fld.type == SOT_OBJECT)
	 { obp = sobu_pointer(ob_id.all) ; ob_id.all = obp->name_id.all ; } ;
/*	Now find package's table */
	if (process->package_ptrs[ob_id.fld.package_id] == 0)
	 v3_error(V3E_NOPCK,"OBJ","NAME_STR","NOPCK","Package not loaded or invalid object name",(void *)ob_id.all) ;
	tblp = process->package_ptrs[ob_id.fld.package_id]->name_table_ptr ;
	for (i=0;i<tblp->count;i++)
	 { if (ob_id.all == tblp->tentry[i].sob_val.all) return(tblp->tentry[i].name) ; } ;
/*	Could not find - return appropriate string rather then error */
	return("*unknown*") ;
}

/*	A S S E R T I O N   U T I L I T I E S			*/

/*	sobu_assertion_add - Adds assertion to process list	*/
/*	 (returns TRUE if added, FALSE if already in list)	*/

int sobu_assertion_add(name)
  char *name ;
{ int hash ; short int i ;
   char tname[V3_ASSERTION_NAME_MAX+1] ;

/*	First convert to upper case */
	for (i=0;;i++)
	 { if ((tname[i] = (*(name+i) >= 'a' ? *(name+i)-32 : (*(name+i) == '-' ? '_' : *(name+i)))) == 0) break ;
	 } ;
/*	Now see if already in list */
	hash = prsu_str_hash(tname,-1) ;
	for (i=0;i<process->assertion_count;i++)
	 { if (hash != process->assertion_hash[i]) continue ;
	   if (strcmp(tname,process->assertions[i]) == 0) return(FALSE) ;
	 } ;
/*	Not found - have to add it */
	if (process->assertion_count >= V3_PROCESS_ASSERTION_MAX)
	 v3_error(V3E_ASRTMAX,"OBJ","ASSERTION_ADD","ASRTMAX","Exceeded max number of process assertions",(void *)V3_PROCESS_ASSERTION_MAX) ;
	strcpy(process->assertions[process->assertion_count],tname) ;
	process->assertion_hash[process->assertion_count++] = hash ;
	process->assertion_changes++ ;
/*	Now recalculate name-ob search lists */
	sobu_calculate_name_ob_list() ;
	return(TRUE) ;
}

/*	sobu_assertion_delete - Deletes assertion in process list */
/*	 (returns TRUE if deleted, FALSE if not in list)	*/

int sobu_assertion_delete(name)
  char *name ;
{ int hash ; short int i ;
   char tname[V3_ASSERTION_NAME_MAX+1] ;

/*	First convert to upper case */
	for (i=0;;i++)
	 { if ((tname[i] = (*(name+i) >= 'a' ? *(name+i)-32 : (*(name+i) == '-' ? '_' : *(name+i)))) == 0) break ;
	 } ;
/*	Now see if in list */
	hash = prsu_str_hash(tname,-1) ;
	for (i=0;i<process->assertion_count;i++)
	 { if (hash != process->assertion_hash[i]) continue ;
	   if (strcmp(tname,process->assertions[i]) != 0) continue ;
/*	   Found it - remove it from list */
	   process->assertion_hash[i] = process->assertion_hash[--process->assertion_count] ;
	   strcpy(process->assertions[i],process->assertions[process->assertion_count]) ;
	   process->assertion_changes++ ;
	   sobu_calculate_name_ob_list() ;
	   return(TRUE) ;
	 } ;
/*	If here then could not find it */
	return(FALSE) ;
}

/*	sobu_assertion_test - Test to see if one or more assertions in process list */
/*	 (returns TRUE or FALSE) */

int sobu_assertion_test(name_list)
  char *name_list ;
{ char *ap,tname[500] ; int hash ;
   short int i,len ;

/*	First convert to upper case */
	if (strlen(name_list) >= sizeof tname)
	 v3_error(V3E_ARGTOOLONG,"OBJ","ASSERTION_TEST","ARGTOOLONG","Assertion name list too long",(void *)sizeof tname) ;
	for (i=0;;i++)
	 { if ((tname[i] = (*(name_list+i) >= 'a' ? *(name_list+i)-32 : (*(name_list+i) == '-' ? '_' : *(name_list+i)))) == 0) break ;
	 } ;
/*	Now scan thru the list */
	ap = tname ;
	for (;;ap+=(len+1))
	 { if ((len = strcspn(ap,",+")) == 0) return(FALSE) ;
	   hash = prsu_str_hash(ap,len) ;
/*	   Look for this assertion */
	   for (i=0;i<process->assertion_count;i++)
	    { if (hash != process->assertion_hash[i]) continue ;
	      if (strncmp(ap,process->assertions[i],len) != 0) continue ;
/*	      Got a match ! */
	      if (*(ap+len) != '+') return(TRUE) ;
	      goto next_assertion ;
	    } ;
/*	   If here then could not find current assertion */
	   if (*(ap+len) == '+') return(FALSE) ;
next_assertion: ;
	 } ;
}

/*	O B J E C T   V A L U E   R O U T I N E S		*/

/*	sobu_object_value - Returns pointer to object's value (db__dcl_valref) or NULLV if object has no valud */

struct db__dcl_valref *sobu_object_value(objall)
   int objall ;
{ struct ob__desc *obp ;		/* Pointer to object description */
   union val__sob obj ;			/* Object in question */
   struct db__dcl_objref *objrp ;
   union val__sob obid ;
   struct db__dcl_valref *valp ;
   int i,j ;

	obj.all = objall ;
/*	If the object has a value then just return it */
	obp = sobu_pointer(obj.all) ;
	if (obp->num_vals > 0) return((struct db__dcl_valref *)&obp->info[obp->ss_vals]) ;
/*	Object has no value (directly), see if any isas do */
	for (i=0;i<obp->num_isas;i++)
	 { objrp = (struct db__dcl_objref *)&obp->info[obp->ss_isas[i]] ;
/*	   objrp = Pointer to isa reference - is it object or name ? */
	   if (objrp->count == 0)
/*	      It's an object - see if it "has" what we want */
	    { obid.all = objrp->name_ids[0].all ;
	    } else
/*	      It's one or more names - evaluate to object if possible */
	    { if ((obid.all = sobu_object_find(objrp->name_ids[0].all)) == NULLV) continue ;
/*	      Found first object, now try for hass */
	      for (j=1;j<objrp->count;j++)
	       { if ((obid.all = sobu_has_find(obid.all,objrp->name_ids[j].all)) == NULLV) break ;
	       } ;
/*	      Did we find it ? */
	      if (j < objrp->count) continue ;
	     } ;
/*	   Have an object, see if it has any values */
	   if ((valp = (struct db__dcl_valref *)sobu_object_value(obid.all)) != NULLV) return(valp) ;
	 } ;
/*	Can't find a value anywhere */
	return(NULLV) ;
}

/*	sobu_push_val - Pushes value of object onto value stack */

int sobu_push_val(objall,val_index,ptr,hasp,nest_flag,ssinfo_flag)
  int objall ;
  int val_index ;		/* Which value to return - 0 = default, n = n'th */
  char *ptr ;			/* Base value pointer (usually from OBJREF) */
  struct db__dcl_objref *hasp ;	/* NULLV or list of object "has" name ids */
  int nest_flag ;		/* If TRUE then sobu_push_val called from sobu_push_val, normally FALSE */
  int ssinfo_flag ;		/* If TRUE then also push pointer to subscripting info */
{ union val__sob obid ;
  union val__sob obj ;		/* Object id (usually from OBJREF) */
   union val__sob parent_obid ;	/* NULLV or id of parent */
   struct ob__desc *obp ;	/* Pointer to located object desc */
   struct db__dcl_objref *objrp ;
   struct db__dcl_valref *valp ; /* Pointer to value in object */
   struct db__formatptr fw ;
   union val__format format ;
   struct db__formatmemposdims *sep ;
  extern union val__sob obj_skel_objlr ;
  short int i,j ;

	obj.all = objall ;
/*	Scan "has" list to get final object */
	obid.all = obj.all ; parent_obid.all = NULLV ;
	if (hasp == NULLV) goto no_hass ;
	for (i=0;i<hasp->count;i++)
	 { if ((obid.all = sobu_has_find((parent_obid.all = obid.all),hasp->name_ids[i].all)) == NULLV)
	    v3_error(V3E_NOFINDOBJ,"OBJ","PUSH_VAL","NOFINDOBJ","Cannot find requested object",NULL) ;
	 } ;
no_hass:
	obj_skel_objlr.all = obid.all ;	/* Save for possible use by itx/xti_gen() */
/*	Now get the pointer to actual object def */
	obp = sobu_pointer(obid.all) ;
/*	See if the object has a value */
	if (obp->num_vals > 0) goto got_value ;
/*	Object has no value(s) - See if any isas do */
	for (i=0;i<obp->num_isas;i++)
	 { objrp = (struct db__dcl_objref *)&obp->info[obp->ss_isas[i]] ;
/*	   objrp = Pointer to isa reference - is it object or name ? */
	   if (objrp->count == 0)
/*	      It's an object - see if it "has" what we want */
	    { obid.all = objrp->name_ids[0].all ;
	    } else
/*	      It's one or more names - evaluate to object if possible */
	    { if ((obid.all = sobu_object_find(objrp->name_ids[0].all)) == NULLV) continue ;
/*	      Found first object, now try for hass */
	      for (j=1;j<objrp->count;j++)
	       { if ((obid.all = sobu_has_find(obid.all,objrp->name_ids[j].all)) == NULLV) break ;
	       } ;
/*	      Did we find it ? */
	      if (j < objrp->count) continue ;
	     } ;
/*	   Have an object, see if it has any values */
	   if (sobu_push_val(obid.all,val_index,ptr,NULL,TRUE,ssinfo_flag)) return(TRUE) ;
	 } ;
/*	Can't find a value anywhere */
	if (nest_flag) return(FALSE) ;
	v3_error(V3E_NOVALUE,"OBJ","PUSH_VAL","NOVALUE","Cannot find value for object",(void *)obid.all) ;
got_value:
/*	If value is list & val_index is 0 then return object */
	if (obp->val_is_list && val_index == 0)
	 { PUSHMP(ptr) ; PUSHF(obid.all) ; return(TRUE) ; } ;
/*	Want a single value - see what to do with it */
	valp = (struct db__dcl_valref *)&obp->info[obp->ss_vals] ;
/*	Are we to return n'th value ? */
	if (val_index > 0)
	 { if (val_index > obp->num_vals)
	    v3_error(V3E_NTHVALUE,"OBJ","PUSH_VAL","NTHVALUE","Object does not have n\'th value",(void *)obp->num_vals) ;
	   valp += (val_index-1) ;	/* Update pointer */
	 } ;
	switch (valp->format.fld.type)
	 { default: v3_error(V3E_BADVAL,"OBJ","PUSH_VAL","BADVAL","Invalid value type",(void *)valp->format.fld.type) ;
	   case VFT_BININT:
	   case VFT_BINLONG:
	   case VFT_BINWORD:
	   case VFT_FLOAT:
	   case VFT_V3NUM:
	   case VFT_FIXSTRSK:
	   case VFT_FIXSTR:
	   case VFT_VARSTR:
	   case VFT_STRINT:
	   case VFT_PACDEC:
	   case VFT_INDIRECT:
	   case VFT_OBJREF:
/*		See if we need subscripting info */
		if (ssinfo_flag)
		 { if (valp->format.fld.mode != VFM_INDIRECT)
		    v3_error(V3E_NOSUBS,"OBJ","PUSH_VAL","NOSUBS","Invalid attempt to subscript non-repeating object",0) ;
		   sep = (struct db__formatmemposdims *)xctu_pointer(valp->mempos.all) ; /* Pointer to symbol format/value */
pss1:
		   PUSHMP(&sep->dims) ; PUSHF(0) ;
		 } else
		 { if (valp->format.fld.mode == VFM_INDIRECT)
		    { sep = (struct db__formatmemposdims *)xctu_pointer(valp->mempos.all) ; }
		    else { sep = (struct db__formatmemposdims *)&valp->format.all ; } ;
		 } ;
/*		See if we need to add in pointer */
		switch (sep->format.fld.mode)
		 { default: v3_error(V3E_INVMODE,"OBJ","PUSH_VAL","INVMODE","Invalid value mode",(void *)sep->format.fld.mode) ;
		   case VFM_PTR:		PUSHMP(xctu_pointer(sep->mempos.all)) ; break ;
		   case VFM_OFFSET:		PUSHMP(sep->mempos.all + ptr) ; break ;
		   case VFM_IMMEDIATE:		PUSHVI(sep->mempos.all) ; break ;
/*		   case VFM_INDIRECT:	*/
		 } ;
		PUSHF(sep->format.all) ; break ;
	   case VFT_MODREF:
/*		Have to see if a user module or V3 primitive */
		if (valp->format.fld.mode == VFM_IMMEDIATE || valp->format.fld.mode == VFM_PTR)
		 { sep = (struct db__formatmemposdims *)&valp->format ; }
		 else { sep = (struct db__formatmemposdims *)xctu_pointer(valp->mempos.all) ; } ;
/*		If this is to be a deferred module call then defer it */
		if (obp->val_is_deferred)
		 { if (sep->format.fld.mode == VFM_IMMEDIATE) { PUSHVP(sep->mempos.all) ; }
		    else { PUSHMP(xctu_pointer(sep->mempos.all)) ; } ;
		   PUSHF(sep->format.all) ; break ;
		 } ;
/*		Push EOA onto stack for good measure */
		PUSHVI(0) ; PUSHF(V3_FORMAT_EOA) ;
/*		If this value is a module requiring parent value then do it */
		if (obp->val_needs_parent)
		 { if (parent_obid.all == NULLV)
		    v3_error(V3E_NOPARENT,"OBJ","PUSH_VAL","NOPARENT","No parent object for module argument",(void *)obid.all) ;
		   PUSHMP(ptr) ; PUSHF(parent_obid.all) ;
		 } ;
		fw.format.all = sep->format.all ;
		if (sep->format.fld.mode == VFM_PTR) { fw.ptr = xctu_pointer(sep->mempos.all) ; }
		 else { fw.ptr = (char *)sep->mempos.all ; } ;
		xctu_call_defer(&fw) ; break ;
	   case VFT_OBJECT:
		PUSHMP(ptr) ; PUSHF(valp->format.all) ; break ;
	   case VFT_OBJNAME:
		objrp = (struct db__dcl_objref *)xctu_pointer(valp->mempos.all) ;
/*		Have pointer to name list - convert to object */
		if ((obid.all = sobu_object_find(objrp->name_ids[0].all)) == NULLV)
		 v3_error(V3E_NOFIND,"OBJ","PUSH_VAL","NOFIND","Could not locate object reference",(void *)obid.all) ;
/*		Found first object, now try for hass */
		for (j=1;j<objrp->count;j++)
		 { if ((obid.all = sobu_has_find(obid.all,objrp->name_ids[j].all)) == NULLV) break ;
		 } ;
/*		Did we find it ? */
		if (j < objrp->count)
		 v3_error(V3E_NOFIND,"OBJ","PUSH_VAL","NOFIND","Could not locate object reference",(void *)obid.all) ;
		PUSHMP(ptr) ; PUSHF(obid.all) ;
		break ;
	 } ;
	return(TRUE) ;
}

/*	sobu_val_num - Returns the number of values belonging to an object */

int sobu_val_num(objall)
  int objall ;
{ struct ob__desc *obp ;	/* Pointer to it */
  union val__sob obj ;		/* The object of interest */

	obj.all = objall ;
/*	Get pointer to the object */
	obp = sobu_pointer(obj.all) ;
/*	And simply return the number of values */
	return(obp->num_vals) ;
}

/*	O B J E C T  /  S K E L E T O N   R O U T I N E S		*/

/*	sobu_bind_skeleton - (Re)binds object to skeleton		*/

void sobu_bind_skeleton()
{ union val__sob topobj ;
   union val__format format ;
   struct db__pck_skeletons *pckskel ;
   struct db__prs_skeleton *skel ;
   char tmpstr[300] ; int i ;

/*	See if second argument is name literal (i.e. BININT) */
	POPF(format.all) ;
	if (format.all == V3_FORMAT_NULL) { POPF(format.all) ; topobj.all = NULLV ; goto got_object ; } ;
	PUSHF(format.all) ;
	if (format.fld.type == VFT_BININT)
	 { if ((topobj.all = sobu_object_find(xctu_popint())) == NULLV)
	    v3_error(V3E_NOSKELOBJ,"OBJ","BIND_SKELETON","NOSKELOBJ","Could not locate *object*",NULL) ;
	   goto got_object ;
	 } ;
/*	If not number or nil then hope it's a string */
	stru_popcstr(tmpstr) ;
/*	Convert to upper case */
	for (i=0;tmpstr[i] != 0;i++)
	 { if (tmpstr[i] >= 'a') { tmpstr[i] -= 32 ; } else { if (tmpstr[i] == '-') tmpstr[i] = '_' ; } ;
	 } ;
	if ((topobj.all = sobu_object_dereference(0,tmpstr)) == NULLV)
	 v3_error(V3E_NONAMEDOBJ,"OBJ","BIND_SKELETON","NONAMEDOBJ","Could not locate named object",(void *)tmpstr) ;
got_object:
/*	Now look for the skeleton */
	stru_popcstr(tmpstr) ;
	for (i=0;tmpstr[i] != 0;i++)
	 { if (tmpstr[i] >= 'a') { tmpstr[i] -= 32 ; } else { if (tmpstr[i] == '-') tmpstr[i] = '_' ; } ;
	 } ;
	pckskel = process->package_ptrs[psi->package_index]->skeleton_ptr ;
	for(i=0;i<=pckskel->count;i++)
	 { skel = (struct db__prs_skeleton *)&(pckskel->buf[pckskel->index[i]]) ; if (strcmp(skel->skel_name,tmpstr) == 0) break ; } ;
	if (i >= pckskel->count)
	 v3_error(V3E_NOSKEL,"OBJ","BIND_SKELETON","NOSKEL","No such skeleton STRUCT declared within this package",NULL) ;
/*	Zip-zop thru the skeleton & undefine everything */
	for(i=0;i<skel->count;i++) { if (skel->el[i].object) skel->el[i].defined = FALSE ; } ;
/*	Found everything, call below to do the work */
	sobu_load_skeleton(pckskel,skel,topobj.all) ;
}

/*	sobu_load_skeleton - Load/Binds Skeleton at package load time	*/

void sobu_load_skeleton(pckskel,skel,objectall)
   struct db__pck_skeletons *pckskel ;	/* Pointer to all package skeletons */
   struct db__prs_skeleton *skel ;	/* Pointer to one to be twiddled */
   int objectall ;
{ union val__sob tlobj,elobj ;
   union val__sob object ;		/* Optional top-level skeleton object */
   struct ob__desc *obp ;
   struct db__dcl_valref *valp ;
   struct db__ob_bucket *bptr ;		/* Points to package object bucket */
   struct db__prs_skeleton *skelp ;
   struct db__formatmemposdims *sep ;
   int ex,i,offset ;
   char buf[200] ;

	object.all = objectall ;
/*	If FULL skeleton then don't look for master object */
	if (skel->skel_type == SKELETON_FULL) goto skip_master ;
/*	If given an explicit object then use it, otherwise find it */
	if (object.all != NULLV) { tlobj.all = object.all ; }
	 else { if (skel->master_object.all == NULLV) return ;
		if ((tlobj.all = sobu_object_find(skel->master_object.all)) == NULLV)
		 { sprintf(buf,"Could not locate master object (%s) for skeleton %s",
				sobu_name_str(skel->master_object.all),skel->skel_name) ;
		   return ;	/* Don't generate error (VEH as of 9/30/87) */
		   v3_error(V3E_NOMASTER,"OBJ","LOAD_SKELETON","NOMASTER",buf,NULL) ;
		 } ;
	      } ;
/*	If top level object has a value then use it as length of skeleton */
	if ((valp = (struct db__dcl_valref *)sobu_object_value(tlobj.all)) != NULLV)
	 { if (valp->format.fld.mode == VFM_INDIRECT)
	    { bptr = process->package_ptrs[valp->mempos.statpure.package_id]->ob_bucket_ptr ;
	      sep = (struct db__formatmemposdims *)(&bptr->buf[valp->mempos.statpure.offset]) ;
	      valp = (struct db__dcl_valref *)&sep->format.all ;
	    } ;
	   skel->alloc_bytes = valp->format.fld.length ;
	 } ;
skip_master:
/*	Now loop thru each of the elements & pick out value */
	for(i=0;i<skel->count;i++)
	 { if (skel->el[i].object)
	    {
	      if ((elobj.all = sobu_has_find(tlobj.all,skel->el[i].el_name.all)) == NULLV) continue ;
	      skel->el[i].el_object.all = elobj.all ;
/*	      Got element object, now get value */
	      if ((valp = (struct db__dcl_valref *)sobu_object_value(elobj.all)) == NULLV) continue ;
/*	      See if we got subscripting info or not */
	      if (valp->format.fld.mode == VFM_INDIRECT)
	       { bptr = process->package_ptrs[valp->mempos.statpure.package_id]->ob_bucket_ptr ;
	         sep = (struct db__formatmemposdims *)(&bptr->buf[valp->mempos.statpure.offset]) ;
		 skel->el[i].offset = sep->mempos.all ; skel->el[i].format.all = sep->format.all ; skel->el[i].dims = sep->dims ;
	       } else
	       { skel->el[i].offset = valp->mempos.all ; skel->el[i].format.all = valp->format.all ;
	       } ;
	      skel->el[i].defined = TRUE ;
	    } ;
	   if (skel->el[i].skelref)
	    { skelp = (struct db__prs_skeleton *)&(pckskel->buf[pckskel->index[skel->el[i].skel_index]]) ; ex = skel->el[i].el_index ;
	      if (!skelp->el[ex].defined) continue ;	/* Parent not defined (yet) */
	      skel->el[i].offset = skelp->el[ex].offset ; skel->el[i].format.all = skelp->el[ex].format.all ;
	      skel->el[i].defined = TRUE ; skel->el[i].el_object = skelp->el[ex].el_object ;
	    } ;
	 } ;
/*	If this is a FULL Skeleton then bind all elements to get complete structure */
	if (skel->skel_type == SKELETON_FULL)
	 { for(i=0,offset=0;i<skel->count;i++)
/*	      If an element is not yet defined then quit */
	    { if (!skel->el[i].defined) { offset = 0 ; break ; } ;
/*	      Either add up offsets or handle "all" or redefines */
	      if (skel->el[i].all)
	       { skel->el[i].offset = 0 ; skel->el[i].format.fld.length = offset ; continue ; } ;
	      if (skel->el[i].redefines)
	       { skel->el[i].offset = skel->el[skel->el[i].el_index].offset ; continue ; } ;
	      skel->el[i].offset = offset ; offset += skel->el[i].format.fld.length ;
	    } ;
	   skel->alloc_bytes = offset ;
	 } ;
}
