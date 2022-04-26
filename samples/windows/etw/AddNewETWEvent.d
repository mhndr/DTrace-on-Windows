

/*++

Copyright (c) Microsoft Corporation

Module Name:

    AddNewEtwEvent.d

Abstract:

    This script generates ETW event when syscall routine returns  0xc0000001 - STATUS_UNSUCCESSFUL.

Requirements:

    None.

Usage:

    dtrace -s AddNewEtwEvent.d

FAQ:
    DTrace has the capability to output ETW events. This is helpful for scenarios where there is existing ETW pipeline to report, collect and analyze.
    Etw trace events can be created by calling the etw_trace macro.
    Events will only be logged if there is an active listener for the specified trace provider, otherwise they'll be skipped.

--*/


syscall:::return
/(uint32_t)arg0 == 0xc0000001UL/
{
    etw_trace
        (
         "Tools.DTrace.Platform", /* Provider Name */
         "AAD330CC-4BB9-588A-B252-08276853AF02", /* Provider GUID */
         "My custom event from DTrace", /* Event Name */
         1, /* Event Level (0 - 5) */
         0x0000000000000020, /* Flag */
         "etw_int32", /* Field_1 Name */
         "PID",/* Field_1 Type */
         (int32_t)pid, /* Field_1 Value  */
         "etw_string", /* Field_2 Name */
         "Execname", /* Field_2 type */
         execname, /* Field_2 Value */
         "etw_string", /* Field_3 Name */
         "Probefunc", /* Field_3 type */
         probefunc /* Field_3 Value */
        );
}
