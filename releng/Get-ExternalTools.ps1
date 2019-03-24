$Url = "https://github.com/lexxmark/winflexbison/releases/download/v2.5.17/winflexbison-2.5.17.zip"
$ExpectedHash = "3DC27A16C21B717BCC5DE8590B564D4392A0B8577170C058729D067D95DED825"

[IO.Directory]::CreateDirectory('external\tools\bin') | Out-Null
Invoke-WebRequest -UseBasicParsing $Url -OutFile 'winflexbison.zip'
$Results = Get-FileHash 'winflexbison.zip' -Algorithm SHA256
if($Results.Hash -ne $ExpectedHash)
{
    Write-Output "Expected hash: $ExpectedHash"
    Write-Output "Actual hash  : $($Results.Hash)"
    Write-Error "Downloaded file hash does not match expected hash"
}

Expand-Archive -Path 'winflexbison.zip' -DestinationPath 'external\tools\bin' -Force
Remove-Item 'winflexbison.zip'
