

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

/* 
Note: Run this script with aggsortkeypos variable set. This variable informs D-Trace to sort the output based on first index (size).

Usage:dtrace -s <script.d>  <Time> <Tag> -x  aggsortkey -x aggsortkeypos=1 

Example for tracking KSec leaks: dtrace -s PoolTrackingSummary.d "450s" 0x6365734b -x aggsortkey -x aggsortkeypos=1 

Output: Script runs for 30 seconds and outputs the KSec allocation/free summary. You can set this to whatever time necessary.

*/


#pragma D option quiet
#pragma D option dynvarsize=240m 
#pragma D option bufsize=120m
#pragma D option aggsize=120m 


fbt:nt:ExAllocatePoolWithTag:entry
{	
    /* This is E100 in reserve. Convert ASCII to Hex => Hex('001E').*/
    if (arg2 == $2) 
    { 
		self->size = (unsigned int) arg1;
		@allocstack[stack(), self->size] = count();
    }
}

fbt:nt:ExAllocatePoolWithTag:return
/self->size/
{
    @mem[(uintptr_t) arg1] = sum(1);
    addr[(uintptr_t) arg1] = 1;
    /* printf("%Y: Execname %s allocated size %d bytes return ptr %x", walltimestamp, execname, (uint64_t) self->size, (uintptr_t) arg1 );*/

    size[(uintptr_t) arg1] = self->size;
    @sizealloc[self->size] = count();
    @delta[self->size] = sum(1);
    
    self->size = 0;
}

fbt:nt:ExFreePoolWithTag:entry
/addr[(uintptr_t) arg0]/
{
    @mem[(uintptr_t) arg0] = sum (-1);
    addr[(uintptr_t) arg0] -= 1;

    @sizefree[size[(uintptr_t) arg0]] = count();
    @delta[size[(uintptr_t) arg0]] = sum(-1);
}

tick-$1
{	
    exit(0);
}

END 
{

   printf("%10s %10s %10s %10s\n", "SIZE", "ALLOC", "FREE", "DELTA");
   printa("%10d %@10d %@10d %@10d\n", @sizealloc, @sizefree, @delta);

   printf("Printing stacks \n");
   printa (@allocstack);
    
   printf("== REPORT ==\n\n");
   printa("0x%x => %@u\n",@mem);
}

