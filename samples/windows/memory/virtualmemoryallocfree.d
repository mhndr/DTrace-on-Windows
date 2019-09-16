

#pragma D option quiet
#pragma D option destructive

/*enum alloctype {
	MEM_COMMIT = 0x00001000;
	MEM_PHYSICAL = 0x00400000;
	MEM_RESERVE = 0x00002000;
};*/

syscall::NtAllocateVirtualMemory:entry 
/execname == $1 / 
{  
	self->trace = 1;
	MEM_COMMIT = 0x00001000;
	MEM_PHYSICAL = 0x00400000;
	MEM_RESERVE = 0x00002000;


	/* check if the arg3 is in user land */
	if (arg3 > 0) 
	{	
		/* print( * (nt`PSIZE_T) copyin(arg3, sizeof (nt`PSIZE_T)));*/

		if ( (arg4 & MEM_COMMIT) && (arg4 & MEM_RESERVE) )
			@a["committed_reserved"] = sum( * (nt`PSIZE_T) copyin(arg3, sizeof (nt`PSIZE_T)));
		else if ( (arg4 & MEM_PHYSICAL)  && (arg4 & MEM_RESERVE) )
			@a["physical_reserved"] = sum( * (nt`PSIZE_T) copyin(arg3, sizeof (nt`PSIZE_T)));
		else if (arg4 & MEM_RESERVE)
			@a["reserved"] = sum( * (nt`PSIZE_T) copyin(arg3, sizeof (nt`PSIZE_T)));
		else if (arg4 & MEM_COMMIT)
			@a["committed"] = sum( * (nt`PSIZE_T) copyin(arg3, sizeof (nt`PSIZE_T)));
	} 
	else 
	{ 	
		
		if ( (arg4 & MEM_COMMIT) && (arg4 & MEM_RESERVE) )
			@a["committed_reserved"] = sum( * (nt`PSIZE_T) arg3 );
		else if ( (arg4 & MEM_PHYSICAL)  && (arg4 & MEM_RESERVE) )
			@a["physical_reserved"] = sum( * (nt`PSIZE_T) arg3 );
		else if (arg4 & MEM_RESERVE)
			@a["reserved"] = sum( * (nt`PSIZE_T) arg3 );
		else if (arg4 & MEM_COMMIT)
			@a["committed"] = sum( * (nt`PSIZE_T) arg3 );
	}
}


syscall::NtFreeVirtualMemory:entry 
/execname == $1/ 
{  
	MEM_DECOMMIT = 0x00004000;
	MEM_RELEASE = 0x00008000;

	/*print(probefunc);*/
	/* check if the arg2 is in user land */
	if (arg2 > 0) 
	{	
		/* print( * (nt`PSIZE_T) copyin(arg3, sizeof (nt`PSIZE_T))); */

		if ( arg3 & MEM_DECOMMIT ) 
			@f["DeCommitted"] = sum( * (nt`PSIZE_T) copyin(arg2, sizeof (nt`PSIZE_T)));
		else if ( arg3 & MEM_RELEASE ) 
			@f["memoryrelease"] = sum( * (nt`PSIZE_T) copyin(arg2, sizeof (nt`PSIZE_T)));
	} 
	else 
	{ 	
		
		if ( arg3 & MEM_DECOMMIT ) 
			@f["DeCommitted"] = sum( * (nt`PSIZE_T) arg2 );
		else if ( arg3 & MEM_RELEASE ) 
			@f["memoryrelease"]  = sum( * (nt`PSIZE_T) arg2 );
	}
}



tick-3sec
{
	system("cls");
	printa(@a); 
	printa(@f);
}



