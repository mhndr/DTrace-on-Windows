# DTrace on Windows

Here at Microsoft, we are always looking to engage with open source communities to produce better solutions for the community and our customers. One of the more useful debugging advances that have arrived in the last decade is DTrace. DTrace of course needs no introduction: it's a dynamic tracing framework that allows an admin or developer to get a real-time look into a system either in user or kernel mode. 

DTrace has a C-style high level and powerful programming language that allows you to dynamically insert trace points. Using these dynamically inserted trace points, you can filter on conditions or errors, write code to analyze lock patterns, detect deadlocks, etc. ETW, while powerful, is static and does not provide the ability to programmatically insert trace points at runtime.

There are a lot of websites and resources from the community to learn about DTrace. One comprehensive option is the [Dynamic Tracing Guide](http://dtrace.org/guide). This book describes DTrace in detail and is the authoritative guide for DTrace. We also have Windows specific examples below.

Starting in 2016, the OpenDTrace effort began on GitHub that tried to ensure a portable implementation of DTrace for different operating systems. We decided to add support for DTrace on Windows using this OpenDTrace port. This is a fork of the 'opendtrace' repository and contains the unified, cross platform, source code for the OpenDTrace system including kernel components.

## Install

Follow [MSDN instructions](https://docs.microsoft.com/windows-hardware/drivers/devtest/dtrace).

Prerequisites:
* Windows 10 x64 Build 1903 or higher

Limitations:
* Only available for 64-bit platforms
* Only captures traces for 64-bit processes

Steps:
1. Enable `dtrace` in the Boot Configuration Data (BCD) store. (`bcdedit /set dtrace on`) You will need to repeat this step every time you install a newer build of Windows.

2. Download and execute [the dtrace installer](https://download.microsoft.com/download/7/9/d/79d6b79a-5836-4118-a9b7-60bc77c97bf7/DTrace.amd64.msi).

3. [Configure the `_NT_SYMBOL_PATH` environment variable](https://docs.microsoft.com/en-us/windows/desktop/dxtecharts/debugging-with-symbols#using-the-microsoft-symbol-server) for local symbol caching.

4. Reboot the target machine.

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

* [MSDN](https://docs.microsoft.com/windows-hardware/drivers/devtest/dtrace)
* [DTrace on Windows](https://techcommunity.microsoft.com/t5/Windows-Kernel-Internals/DTrace-on-Windows/ba-p/362902)
* [Compiling OpenDTrace for Windows](COMPILING.md)
* [OpenDTrace Documentation Repository](https://github.com/opendtrace/documentation)
* [Dynamic Tracing Guide](http://dtrace.org/guide/preface.html)

## License

OpenDTrace is under the CDDL license, see the `LICENSE` file in this repository for details.
