
/*++

Copyright (c) Microsoft Corporation

Module Name:

    listvirtualmemoryallocfree.d

Abstract:

    This script tracks virtual memmory alloc.

Usage:

     dtrace -s MemoryPerNumaNode.d

--*/


inline ULONG MEM_COMMIT = 0x00001000;
inline ULONG MEM_PHYSICAL = 0x00400000;
inline ULONG MEM_RESERVE = 0x00002000;

this SIZE_T Size;
this string Mode;
this string Caller;

syscall::NtAllocateVirtualMemory:entry
/ execname != $1 /
{

    if (arg3 > 0) {
        this->Size = *(PSIZE_T)copyin(arg3, sizeof(PSIZE_T));
        this->Caller = "user";
    } else {
        this->Size = *args[3];
        this->Caller = "kernel";
    }

    if ( (args[4] & MEM_COMMIT) && (args[4] & MEM_RESERVE) )
        this->Mode = "reserved & commited";
    else if ( (args[4] & MEM_PHYSICAL)  && (args[4] & MEM_RESERVE) )
        this->Mode = "physical";
    else if (args[4] & MEM_RESERVE)
        this->Mode = "reserved";
    else if (args[4] & MEM_COMMIT)
        this->Mode = "commited";
    else
        this->Mode = "<unknown operation>";

    printf("Bytes %s (%s mode) %d", this->Mode, this->Caller, this->Size);
}

