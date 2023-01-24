/*
Note: Run this script with aggsortkeypos variable set. This variable informs D-Trace to sort the output based on first index (size).

Usage:dtrace -s <script.d> -x aggsortkey -x aggsortkeypos=1

Output: Script runs for 30 seconds and outputs the KSec allocation/free summary. You can set this to whatever time necessary.

*/

#pragma D option destructive
#pragma D option quiet
#pragma D option dynvarsize=0x10000000

union tag
{
	string tags;
	unsigned long tagl;
	char tagc[4];
};

this union tag myTag;

fbt:nt:ExAllocatePoolWithTag:entry
{
    /* This is KSec in reserve. Convert ASCII to Hex => Hex('ceSK').*/
    if (arg2 == 0x6365734b)
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

tick-30s
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
