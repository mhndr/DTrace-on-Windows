
#pragma D option quiet
#pragma D option destructive

inline ULONG MEM_COMMIT = 0x00001000;
inline ULONG MEM_PHYSICAL = 0x00400000;
inline ULONG MEM_RESERVE = 0x00002000;
inline ULONG MEM_DECOMMIT = 0x00004000;
inline ULONG MEM_RELEASE = 0x00008000;

this SIZE_T Size;

syscall::NtAllocateVirtualMemory:entry
/* execname == $1 */
{
    self->trace = 1;

    if (arg3 > 0)
        this->Size = *(PSIZE_T)copyin(arg3, sizeof(PSIZE_T));
    else
        this->Size = *args[3];

    if ( (args[4] & MEM_COMMIT) && (args[4] & MEM_RESERVE) )
        @a["committed_reserved"] = sum(this->Size);
    else if ((args[4] & MEM_PHYSICAL)  && (args[4] & MEM_RESERVE) )
        @a["physical_reserved"] = sum(this->Size);
    else if (args[4] & MEM_RESERVE)
        @a["reserved"] = sum(this->Size);
    else if (args[4] & MEM_COMMIT)
        @a["committed"] = sum(this->Size);
}

syscall::NtFreeVirtualMemory:entry
/* execname == $1 */
{
    if (arg2 > 0)
        this->Size = *(PSIZE_T)copyin(arg2, sizeof(PSIZE_T));
    else
        this->Size = *args[2];

    if (args[3] & MEM_DECOMMIT )
        @f["DeCommitted"] = sum(this->Size);
    else if (args[3] & MEM_RELEASE )
        @f["memoryrelease"] = sum(this->Size);
}

tick-3sec
{
    system("cls");
    printa(@a);
    printa(@f);
}



