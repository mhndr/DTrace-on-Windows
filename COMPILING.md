# Compiling OpenDTrace for Windows (user-mode components)

## Requirements

* [Visual Studio 2017](https://visualstudio.microsoft.com/vs/community/) (or newer)
* [Git for Windows](https://git-scm.com/download/win)
* [Windows 10 Anniversary Update](https://blogs.windows.com/windowsexperience/2016/08/02/how-to-get-the-windows-10-anniversary-update/#GD97Eq04wJA7S4P7.97) (or newer)

## Steps

1. Install Visual Studio with the `Desktop development with C++` and `Universal Windows Platform development` workloads. Ensure the optional `Windows 10 SDK (10.0.17763.0)` component is also selected.

2. Install Git for Windows.

3. Clone the repository (`git clone https://github.com/microsoft/DTrace-on-Windows`).

4. Execute `releng\Get-ExternalTools.ps1` in PowerShell. (Right-click the script and select Run with PowerShell.)

5. Open `opendtrace.sln` in Visual Studio.

6. Change the target platform as needed and build the solution.