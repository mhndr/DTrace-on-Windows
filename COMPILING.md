# Compiling OpenDTrace for Windows (user-mode components)

## Requirements

* [Windows WDK and SDK](https://docs.microsoft.com/windows-hardware/drivers/download-the-wdk) (version 1903 or later)
* [Git for Windows](https://git-scm.com/download/win)
* [Windows 10 Anniversary Update](https://blogs.windows.com/windowsexperience/2016/08/02/how-to-get-the-windows-10-anniversary-update/#GD97Eq04wJA7S4P7.97) (or newer)

## Steps

1. Install WDK/SDK following instructions in https://docs.microsoft.com/windows-hardware/drivers/download-the-wdk

2. Install Git for Windows.

3. Clone the repository (`git clone https://github.com/microsoft/DTrace-on-Windows`).

4. Execute `releng\Get-ExternalTools.ps1` in PowerShell. (Right-click the script and select Run with PowerShell.)

5. Open `opendtrace.sln` in Visual Studio.

6. Change the target platform as needed and build the solution.