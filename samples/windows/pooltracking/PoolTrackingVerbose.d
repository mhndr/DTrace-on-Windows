/*

Usage:dtrace -s <script.d>  <Time> <Tag> -x aggsortkey -x aggsortkeypos=1 > out.txt
Example for tracking KSec: dtrace -s PoolTrackingVerbose.d "450s" 0x6365734b -x aggsortkey -x aggsortkeypos=1 > PoolTrackingVerboseOutput.txt

*/



#pragma D option dynvarsize=240m 
#pragma D option bufsize=120m
#pragma D option aggsize=120m 


fbt:nt:ExAllocatePoolWithTag:entry
{	
    if (arg2 == $2) 
    { 
        stack();
        self->size = (unsigned int) arg1;
    }
}

fbt:nt:ExAllocatePoolWithTag:return
/self->size/
{
    @mem[(uintptr_t) arg1] = sum(1);
    addr[(uintptr_t) arg1] = 1;
    printf("%Y: Execname %s allocated size %d bytes return ptr %x", walltimestamp, execname, (uint64_t) self->size, (uintptr_t) arg1 );
    self->size = 0;
}

fbt:nt:ExFreePoolWithTag:entry
/addr[(uintptr_t) arg0]/
{
    @mem[(uintptr_t) arg0] = sum (-1);
    addr[(uintptr_t) arg0] -= 1;
    printf("%Y: Execname %s Free ptr %x", walltimestamp, execname, (uintptr_t) arg0 );
}

tick-$1
{	
    exit(0);
}

END 
{
    printf("== REPORT ==\n\n");
    printa("0x%x => %@u\n",@mem);
}

