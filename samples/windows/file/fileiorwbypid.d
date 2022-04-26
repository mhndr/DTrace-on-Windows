
/*++

Copyright (c) Microsoft Corporation

Module Name:

    fileiorwbypid.d

Abstract:

    This script tracks all file read and write I/O operations. Data is reported every 5 seconds by PID.

Requirements:

    This script has no special requirements.

Usage:

     dtrace.exe -s fileiorwbypid.d

--*/


#pragma D option quiet
#pragma D option destructive
#pragma D option aggrate=1000us


BEGIN {
    ts = timestamp;
}

syscall::NtReadFile:entry
{
    @read[execname, pid] = sum(args[6]);
    @write[execname, pid] = sum(0);
}

syscall::NtWriteFile:entry
{
    @write[execname, pid] = sum(args[6]);
    @read[execname, pid] = sum(0);
}

tick-3s {
    system("cls");
    trunc(@read, 10);
    trunc(@write, 10);
    fact = 1024 * (timestamp - ts) / 1000000000;
    normalize(@read, fact);
    normalize(@write, fact);

    printf("\n %-16s %16s %10s %10s\n", "Executable", "PID", "read[kb/s]", "write[kb/s]");
    printa(" %-16s %16d %@10d %@10d \n",@read, @write);

    clear(@read);
    clear(@write);

    ts = timestamp;
}
