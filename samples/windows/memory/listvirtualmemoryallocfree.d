/*
* CDDL HEADER START
*
* The contents of this file are subject to the terms of the
* Common Development and Distribution License (the "License").
* You may not use this file except in compliance with the License.
*
* You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
* or http://www.opensolaris.org/os/licensing.
* See the License for the specific language governing permissions
* and limitations under the License.
*
* When distributing Covered Code, include this CDDL HEADER in each
* file and include the License file at usr/src/OPENSOLARIS.LICENSE.
* If applicable, add the following below this CDDL HEADER, with the
* fields enclosed by brackets "[]" replaced with your own identifying
* information: Portions Copyright [yyyy] [name of copyright owner]
*
* CDDL HEADER END
*/

/*++

Copyright (c) Microsoft Corporation

Module Name:

    listvirtualmemoryallocfree.d

Abstract:

    This script tracks virtual memmory alloc. 

Requirements: 

    This script has no special requirements.

Usage: 

     dtrace -s MemoryPerNumaNode.d 

--*/


/*enum alloctype {
	MEM_COMMIT = 0x00001000;
	MEM_PHYSICAL = 0x00400000;
	MEM_RESERVE = 0x00002000;
};*/




syscall::NtAllocateVirtualMemory:entry 
/execname != $1 / 
{  
	MEM_COMMIT = 0x00001000;
	MEM_PHYSICAL = 0x00400000;
	MEM_RESERVE = 0x00002000;

	/* check if the arg3 is in user land */
	if (arg3 > 0) 
	{	
		if ( (arg4 & MEM_COMMIT) && (arg4 & MEM_RESERVE) )
			printf(" Bytes reserved & commited %d ",  * (nt`PSIZE_T) copyin(arg3, sizeof (nt`PSIZE_T)));
		else if ( (arg4 & MEM_PHYSICAL)  && (arg4 & MEM_RESERVE) )
			printf(" Bytes physical %d ",  * (nt`PSIZE_T) copyin(arg3, sizeof (nt`PSIZE_T)));
		else if (arg4 & MEM_RESERVE)
			printf(" Bytes reserved %d ",  * (nt`PSIZE_T) copyin(arg3, sizeof (nt`PSIZE_T)));
		else if (arg4 & MEM_COMMIT)
			printf(" Bytes commited %d ",  * (nt`PSIZE_T) copyin(arg3, sizeof (nt`PSIZE_T)));	
	} 
	else 
	{ 	
		
		if ( (arg4 & MEM_COMMIT) && (arg4 & MEM_RESERVE) )
			printf(" Bytes committed %d ",  * (nt`PSIZE_T) arg3);
		else if ( (arg4 & MEM_PHYSICAL)  && (arg4 & MEM_RESERVE) )
			printf(" Bytes physical %d ",  * (nt`PSIZE_T) arg3);
		else if (arg4 & MEM_RESERVE)
			printf(" Bytes reserved %d ",  * (nt`PSIZE_T) arg3);
		else if (arg4 & MEM_COMMIT)
			printf(" Bytes commited %d ",  * (nt`PSIZE_T) arg3);
	}
}