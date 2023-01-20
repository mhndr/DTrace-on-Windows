/*++

Copyright (c) Microsoft Corporation

Module Name:

    workingsetinfobypid.d

Abstract:

    This script provides the working set for a given process ID (PID).

Requirements:

    This script needs symbols to be configured.

Usage:

     dtrace -s workingsetinfobypid.d <pid>

--*/

#pragma D option quiet
#pragma D option destructive

PLIST_ENTRY head;
PLIST_ENTRY curptr;
struct nt`_EPROCESS *eprocess_ptr;
int found;

BEGIN
{
    head = (PLIST_ENTRY)&nt`PsActiveProcessHead;
    curptr = head->Flink;
    found = 0;
    printf("Looking for %d...\n", $1);
}

END
/ !found /
{
    printf("No matching process found for %d\n", $1);
}

tick-10s
/ !found /
{
    exit(0);
}

tick-1ms
/ !found /
{
    /* use this for pointer parsing */

    if (curptr == head)
    {
        exit(0);
    }
    else
    {
        eprocess_ptr = (struct nt`_EPROCESS*)((intptr_t)curptr - offsetof(nt`_EPROCESS, ActiveProcessLinks));
        processid = (int) eprocess_ptr->UniqueProcessId;

        if ($1 == processid)
        {
            found = 1;
        }
        else
        {
            curptr = curptr->Flink;
        }
    }
}


tick-1s
/ found /
{
    system ("cls");

    printf("Process name: %s\n", eprocess_ptr->ImageFileName);
    printf("Hard fault count %u (Mb)\n", eprocess_ptr->Vm.Instance.HardFaultCount*4096/(1024*1024));
    printf("Page fault count %u (Mb)\n", eprocess_ptr->Vm.Instance.PageFaultCount*4096/(1024*1024));
    printf("Working Set Size %llu (Kb) \n", (uint64_t)eprocess_ptr->Vm.Instance.WorkingSetSize*4*1024/1024 );
    printf("Working Set Size (Private) %llu (Kb)\n", (uint64_t)eprocess_ptr->Vm.Instance.WorkingSetLeafPrivateSize*4*1024/1024  );
    printf("Minimum Working Set Size %llu (Kb) \n", (uint64_t)eprocess_ptr->Vm.Instance.MinimumWorkingSetSize*4*1024/1024 );
    printf("Maximum Working Set Size %llu (Kb) \n", (uint64_t)eprocess_ptr->Vm.Instance.MaximumWorkingSetSize*4*1024/1024  );
    printf("Peak Set Size %llu (Kb)\n", (uint64_t)eprocess_ptr->Vm.Instance.PeakWorkingSetSize*4*1024/1024  );

    if (eprocess_ptr->Vm.Instance.Flags.MemoryPriority )
    {
        printf ("MemoryPriority: Foreground\n");
    }
    else
    {
        printf ("MemoryPriority: Background\n");
    }

    printf("Commit charge %llu \n", (uint64_t)eprocess_ptr->CommitCharge*4*1024/1024  );
    printf("Virtual Size %llu (in Mega bytes) \n", (uint64_t)eprocess_ptr->VirtualSize/(1024*1024) );
    printf("Peak Virtual Size %llu (in Mega bytes) \n", (uint64_t)eprocess_ptr->PeakVirtualSize/(1024*1024) );
}
