# OpenDTrace

The `opendtrace` repository contains the unified, cross platform, source code for the OpenDTrace system including kernel components and tools for all of the platforms currently supported by the OpenDTrace system.

## Install

Prerequisites:
* Windows 10 x64 Build 18342 or higher

Limitations:
* Only available for 64-bit platforms
* Only captures traces for 64-bit processes

Steps:
1. Enable `dtrace` in the Boot Configuration Data (BCD) store. (`bcdedit /set dtrace on`)

2. Download and execute [the dtrace installer](https://download.microsoft.com/download/B/D/4/BD4B95A5-0B61-4D8F-837C-F889AAD8DAA2/DTrace.amd64.msi).

3. [Configure the `_NT_SYMBOL_PATH` environment variable](https://docs.microsoft.com/en-us/windows/desktop/dxtecharts/debugging-with-symbols#using-the-microsoft-symbol-server) for local symbol caching.

4. (Optional) [Configure Windows for kernel-mode debugging](https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/getting-started-with-windbg--kernel-mode-). This is only required if you want to trace kernel events using `fbt` or other providers.

5. Reboot the target machine.

## Examples

Note: DTrace on Windows leverages [additional Windows security features]((https://techcommunity.microsoft.com/t5/Windows-Kernel-Internals/DTrace-on-Windows/ba-p/362902)) that may impact your experience.

```c
// Syscall summary by program for 5 seconds: 
dtrace -Fn "tick-5sec { exit(0);} syscall:::entry{ @num[pid,execname] = count();} "
 
// Summarize timer set/cancel program for 3 seconds: 
dtrace -Fn "tick-3sec { exit(0);} syscall::Nt*Timer*:entry { @[probefunc, execname, pid] = count();}"
 
// Dump System Process kernel structure: (requires symbol path to be set)
dtrace -n "BEGIN{print(*(struct nt`_EPROCESS *) nt`PsInitialSystemProcess);exit(0);}"
 
// Tracing paths through NTFS when running notepad.exe (requires KD attach): Run below command and launch notepad.exe
dtrace -Fn "fbt:ntfs::/execname==\"notepad.exe\"/{}"
```

## Learn more

* [DTrace on Windows](https://techcommunity.microsoft.com/t5/Windows-Kernel-Internals/DTrace-on-Windows/ba-p/362902)
* [Compiling OpenDTrace for Windows](COMPILING.md)
* [OpenDTrace Documentation Repository](https://github.com/opendtrace/documentation)
* [Dynamic Tracing Guide](http://dtrace.org/guide/preface.html)

## License

OpenDTrace is under the CDDL license, see the `LICENSE` file in this repository for details.

