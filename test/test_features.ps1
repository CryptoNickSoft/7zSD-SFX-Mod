$ErrorActionPreference = "Stop"

$outRoot = Join-Path $env:TEMP "gigatool\port_features"
$modDir = "C:\Users\Admin\Documents\MultiTool\HomeChats\Chat-10\7zsfx\port_2606\output"
$seven = "C:\Program Files\7-Zip\7z.exe"

Remove-Item $outRoot -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory $outRoot | Out-Null

function Write-Cfg([string]$path, [string]$dir, [string[]]$extra)
{
    $dirEsc = $dir.Replace("\", "\\")
    $cfg = @(";!@Install@!UTF-8!", 'Title="Feature test"', ('InstallPath="' + $dirEsc + '"'), 'RunProgram="cmd.exe /c exit"')
    $cfg += $extra
    $cfg += ";!@InstallEnd@!"
    [IO.File]::WriteAllText($path, ($cfg -join "`r`n") + "`r`n", [Text.Encoding]::ASCII)
}

function Run-Exe([string]$exe, [string]$arguments, [string]$dir, [string]$expect)
{
    Remove-Item $dir -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory $dir -Force | Out-Null
    if ($arguments) { $p = Start-Process -FilePath $exe -ArgumentList $arguments -PassThru }
    else { $p = Start-Process -FilePath $exe -PassThru }
    for ($i = 0; $i -lt 40; $i++)
    {
        Start-Sleep -Milliseconds 300
        if ($p.HasExited) { break }
    }
    if (-not $p.HasExited) { [void]$p.CloseMainWindow(); Start-Sleep -Milliseconds 500; if (-not $p.HasExited) { $p.Kill(); return "TIMEOUT" } }
    $file = Join-Path $dir $expect
    if (-not (Test-Path $file)) { return ("FAIL exit=" + $p.ExitCode + " no file") }
    if ($p.ExitCode -ne 0) { return ("FAIL exit=" + $p.ExitCode) }
    return "PASS"
}

$src = Join-Path $outRoot "src"
New-Item -ItemType Directory $src -Force | Out-Null
Set-Content (Join-Path $src "hello.txt") "feature test" -Encoding ASCII

# 1. Archive parameter: loader without a command-line argument
$archPlain = Join-Path $outRoot "payload_archive.7z"
Push-Location $src
& $seven a -t7z -m0=lzma2 $archPlain "hello.txt" | Out-Null
Pop-Location
$dirArchive = Join-Path $outRoot "out_archive"
$cfgArchive = Join-Path $outRoot "cfg_archive.txt"
$archEsc = $archPlain.Replace("\", "\\")
Write-Cfg $cfgArchive $dirArchive @(('Archive="' + $archEsc + '"'))
$loaderExe = Join-Path $outRoot "archive_loader.exe"
cmd /c ("copy /b `"" + (Join-Path $modDir "7zsd_All.sfx") + "`" + `"" + $cfgArchive + "`" `"" + $loaderExe + "`"") | Out-Null
$rArchive = Run-Exe $loaderExe "" $dirArchive "hello.txt"


"Archive:   " + $rArchive
