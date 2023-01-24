/*++

Copyright (c) Microsoft Corporation

Module Name:

    capturelkd.d

Abstract:

    This script retrieves live kernel dump when syscall routine returns  0xc0000001 - STATUS_UNSUCCESSFUL.

Requirements:

    None.

Usage:

    dtrace -s capturelkd.d

FAQ:
Where is my live dump?
        It's typically found in C:\Windows\LiveKernelReports (OR) the path specified by the regkey hklm\system\currentcontrolset\control\crashcontrol\livekernelreports

Where can i find livedump related events?
        Look at the system events in event viewer Microsoft-Windows-Kernel-LiveDump/Operational
        Open Event Viewer:
                Go to: Applications and Services Logs->Microsoft->Windows->Kernel-Livedump->Operational
        Don't find any logs?
                Enable the analytic channel
                        wevtutil sl Microsoft-Windows-Kernel-LiveDump/Analytic /e:true
        Hitting disk space issues (Event ID 202 -Error Text: Live Dump Write Deferred Dump Data API ended. NT Status: 0xC000007F.)
                This means the disk space is insufficient. Update the below regkey to change the path for live dump creation.
                Change the folder location and reboot the machine.
                        reg add hklm\system\currentcontrolset\control\crashcontrol\livekernelreports /v "LiveKernelReportsPath" /t reg_sz /d
                        Set this to "\??\c:\livedumps" (as an example)
        Still facing issues?
                        Check live dump throttle settings and if necessary retry by disabling throttling by setting SystemThrottleThreshold & ComponentThrottleThreshold keys to zero

Here is a sample registry settings
        reg add "HKLM\System\CurrentControlSet\Control\CrashControl\FullLiveKernelReports" /f /t REG_DWORD /v FullLiveReportsMax /d 50
        reg add "HKLM\System\CurrentControlSet\Control\CrashControl" /f /t REG_DWORD /v AlwaysKeepMemoryDump /d 1
        reg add "HKLM\System\CurrentControlSet\Control\CrashControl\FullLiveKernelReports" /f /t REG_DWORD /v SystemThrottleThreshold /d 0
        reg add "HKLM\System\CurrentControlSet\Control\CrashControl\FullLiveKernelReports" /f /t REG_DWORD /v ComponentThrottleThreshold /d 0
        reg add hklm\system\currentcontrolset\Control\CrashControl\LiveKernelReports /v "LiveKernelReportsPath" /t reg_sz /d "\??\c:\livedumps"

--*/

#pragma D option destructive

inline NTSTATUS STATUS_UNSUCCESSFUL = 0xc0000001UL;

syscall:::return
{
    self->status = (NTSTATUS)arg0;

    if (self->status == STATUS_UNSUCCESSFUL)
    {
        printf ("Return value arg0:%x \n", self->status);
        printf ("Triggering LiveDump \n");
        lkd(1);
        exit(0);
    }
}
