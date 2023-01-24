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
    /* This is E100 in reverse. Convert ASCII to Hex => Hex('001E').*/
    if (arg2 == $2)
    {
		self->size = (unsigned int) arg1;
		@allocstack[stack(), self->size] = count();
    }
}

fbt:nt:ExAllocatePoolWithTag:return
/ self->size /
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
/ addr[(uintptr_t) arg0] /
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
