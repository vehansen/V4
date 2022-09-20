v4mm_GrabLockTable(lockid)
  int *lockid ;
{
	return(++(*lockid) == 0) ;
}