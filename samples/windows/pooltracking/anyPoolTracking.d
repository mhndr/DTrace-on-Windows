/*

Usage:
	dtrace -s <script.d>  <Check Time> <Exit Time> > out.txt
Example for tracking a buffer is unfreed even after 60 seconds and exits tracing after 600 seconds:
	dtrace -s anyPoolTracking.d 60 600 > anyPoolTrackingOutput.txt

*/


#pragma D option dynvarsize=1000m
#pragma D option bufsize=240m
#pragma D option aggsize=240m


BEGIN
{
    tracing = 0;
    checkinterval = 0;

}

fbt:nt:ExAllocatePoolWithTag:entry
/tracing == 0/
{
    tracing = 1;
    ts = timestamp;

    /* Start speculating */
    spec = speculation();
    speculate(spec);

    stack();
    self->size = (unsigned int) arg1;
    self->tag = arg2;
}

fbt:nt:ExAllocatePoolWithTag:return
/self->size/
{

    addr[(uintptr_t) arg1] = 1;
    speculate(spec);
    printf("%Y: Execname %s allocated size %d bytes return ptr %x in Pool ID %x", walltimestamp, execname, (uint64_t) self->size, (uintptr_t) arg1, self->tag);
    self->size = 0;
    self->tag = 0;
}

fbt:nt:ExFreePoolWithTag:entry
/addr[(uintptr_t) arg0]/
{
    discard(spec);
    spec = 0;
    tracing = 0;
    checkcount = 0;
    addr[(uintptr_t) arg0] = 0;
}

tick-$1sec
/tracing == 1/
{
    if (checkinterval == 0)
    {
        checkinterval++;
    }
    else if (checkinterval > 0)
    {
        commit(spec);
        spec = 0;
        checkcout = 0;
        tracing == 0;
    }
    else
    {
        /* wierd! Error...*/
    }
}

tick-$2sec
{
    exit(0);
}

END
{
    printf("Exiting \n");
}

