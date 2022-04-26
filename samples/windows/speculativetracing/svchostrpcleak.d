

/*++

Copyright (c) Microsoft Corporation

Module Name:

    svchostrpcleak.d

Abstract:

    This script was used authored by Microsoft support to root-cause a hard to debug RPC leak issue.

    The leaks had a consistent pattern. Essentially, a Create view event was followed by multiple Create views without a corresponding Send event on the same thread.

    Support authored this script to detect & instrument the leak condition. Upon successful leak condition detection, the script speculatively traces every function traversed by the thread in RPCRT4 between consecutive create section calls.

    Speculative tracing is a powerful DTrace facility which provides the ability to tentatively trace data and then later decide whether to commit the data to a tracing buffer or discard it.

    An analogy would be, enabling ETW tracing for all ALPC, capturing it in memory all the time and then only writing or capturing those contents which are relevant for analysis. Such capability to dynamically instrument and write code doesn’t exist with ETW. At the end of 10 seconds, it dumps all the unfreed pointers.

Requirements:

    This script has no special requirements.

Usage:

     dtrace -s svchostrpcleak.d

--*/


#pragma D option nspec=80

inline ULONG ALPC_FLG_MSG_SEC_ATTR        = 0x80000000;
inline ULONG ALPC_FLG_MSG_DATAVIEW_ATTR   = 0x40000000;
inline ULONG ALPC_FLG_MSG_CONTEXT_ATTR    = 0x20000000;
inline ULONG ALPC_FLG_MSG_HANDLE_ATTR     = 0x10000000;

uint32_t exitReq;

BEGIN {
    self->spec = 0;
    exitReq = 0;
}
/*
#define ALPC_VIEWFLG_UNMAP_EXISTING        0x00010000
#define ALPC_VIEWFLG_AUTO_RELEASE          0x00020000
#define ALPC_VIEWFLG_SECURED_ACCESS        0x00040000
*/

tick-1s
/exitReq/
{
    exit(0);
}

/*  Print the speculative buffer when error condition is met.
    Free Speculative buffers regardless by zero assignment. */

syscall::NtAlpcCreateSectionView:entry
/pid==$1/
{
/*++
    __in HANDLE PortHandle,                      arg0
    __reserved ULONG Flags,                      arg1
    __inout PALPC_DATA_VIEW_ATTR ViewAttributes  arg2
--*/
    self->ViewAttributes = arg2;

    if (self->spec) {
        speculate(self->spec);
        ustack();
        commit(self->spec);
        exitReq = 1;
    }

    self->spec = 0;
}

/* Continue speculative tracing */
syscall::NtAlpcDeletePortSection:entry
/self->spec/
{
    speculate(self->spec);
    printf("%s", probefunc);
    ustack();
}

/* Continue speculative tracing */
syscall::NtAlpcCreateSectionView:return
/self->ViewAttributes/
{

    if (arg0 == 0) {

        /*
          VIEW base = offset 0x10 from ALPC_DATA_VIEW_ATTR
        */

        self->spec = speculation();
        speculate(self->spec);

        this->viewbase = (void**)
            copyin(self->ViewAttributes + 0x10, sizeof(uintptr_t));
        this->viewsize = (uint32_t*)
            copyin(self->ViewAttributes + 0x10 + 0x8, sizeof(uint32_t));
        this->sectionhandle = (void**)
            copyin(self->ViewAttributes + 0x8, sizeof(uintptr_t));

        printf("%Y: process %d created view 0x%p, size (0x%x) on section (0x%p)\n",
               walltimestamp,
               pid,
               *this->viewbase,
               *this->viewsize,
               *this->sectionhandle);

        ustack();
    } else {
        /*fuction failed nothing to see here*/
    }

    self->ViewAttributes = 0;
}

/* Continue speculative tracing */
pid$1:rpcrt4::entry
/self->spec/
{
    speculate(self->spec);
    printf("%s\n", probefunc);
}

/* Discard speculative buffers when a matching Send View has been found. */
syscall::NtAlpcSendWaitReceivePort:entry
/self->spec/
{
/*++
__in HANDLE PortHandle,                                                    arg0
__in ULONG Flags,                                                          arg1
__in_bcount_opt(SendMessage->u1.s1.TotalLength) PPORT_MESSAGE SendMessage, arg2
__inout_opt PALPC_MESSAGE_ATTRIBUTES SendMessageAttributes,                arg3
__out_bcount_opt(*BufferLength,*BufferLength) PPORT_MESSAGE ReceiveMessage,arg4
__inout_opt PSIZE_T BufferLength,                                          arg5
__inout_opt PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes,             arg6
__in_opt PLARGE_INTEGER Timeout                                            arg7
--*/

    if ((0 != arg2) && (0 != arg3)) {

        /*
        Grab Valid flags from 2nd ULONG in ALPC_MESSAGE_ATTRIBUTES struct
        of sent msg attributes
        */

        this->msgav = (uint32_t*)
            copyin(arg3 + 4, sizeof(uint32_t));

        if (*this->msgav & ALPC_FLG_MSG_DATAVIEW_ATTR) {

            discard(self->spec);
            self->spec = 0;
        }
    }
}

