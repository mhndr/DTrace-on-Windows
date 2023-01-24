/*++

Copyright (c) Microsoft Corporation

Module Name:

    newprocesstracker.d

Abstract:

    This script tracks and reports all new process created in the system.

Requirements:

    This script needs symbols.

Usage:

     dtrace.exe -s newprocesstracker.d

--*/

struct ustr{uint16_t buffer[256];};

syscall::NtCreateUserProcess:entry
{
    this->ProcessParameters = (nt`_RTL_USER_PROCESS_PARAMETERS*)
        copyin(arg8, sizeof(nt`_RTL_USER_PROCESS_PARAMETERS));

    this->fname = (uint16_t*)
        copyin((uintptr_t) this->ProcessParameters->ImagePathName.Buffer,
               this->ProcessParameters->ImagePathName.Length);

    printf("Process %s PID %d created %*ws \n", execname,pid,
           this->ProcessParameters->ImagePathName.Length / 2,
           ((struct ustr*)this->fname)->buffer);
}
