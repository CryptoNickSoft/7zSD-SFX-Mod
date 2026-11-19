$ErrorActionPreference = "Stop"

$outRoot = Join-Path $env:TEMP "gigatool\port_all"
$modDir = "C:\Users\Admin\Documents\MultiTool\HomeChats\Chat-10\7zsfx\port_2606\output"
$seven = "C:\Program Files\7-Zip\7z.exe"

Remove-Item $outRoot -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory $outRoot | Out-Null

function MakeArchive([string]$method, [string]$path)
{
    $src = Join-Path $outRoot ("src_" + $method)
    New-Item -ItemType Directory $src -Force | Out-Null
    Set-Content (Join-Path $src "hello.txt") ("hello " + $method) -Encoding ASCII
    if (Test-Path $path) { Remove-Item $path -Force }
    Push-Location $src
    try
    {
        switch ($method)
        {
            "LZMA2"   { & $seven a -t7z -m0=lzma2 $path "hello.txt" | Out-Null }
            "LZMA"    { & $seven a -t7z -m0=lzma $path "hello.txt" | Out-Null }
            "Deflate" { & $seven a -t7z -m0=deflate -mhc=off $path "hello.txt" | Out-Null }
            "PPMd"    { & $seven a -t7z -m0=ppmd -mhc=off $path "hello.txt" | Out-Null }
        }
    }
    finally { Pop-Location }
    if (-not (Test-Path $path)) { throw ("archive failed: " + $method) }
}

function Write-Config([string]$path, [string]$dir, [string]$extra)
{
    $dirEsc = $dir.Replace("\", "\\")
    $cfg = @(
        ";!@Install@!UTF-8!",
        'Title="Port test"',
        ('InstallPath="' + $dirEsc + '"'),
        'RunProgram="cmd.exe /c exit"'
    )
    if ($extra) { $cfg += $extra }
    $cfg += ";!@InstallEnd@!"
    [IO.File]::WriteAllText($path, ($cfg -join "`r`n") + "`r`n", [Text.Encoding]::ASCII)
}

function Run-Test([string]$exe, [string]$arguments, [string]$dir, [string]$expect, [string]$content)
{
    Remove-Item $dir -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory $dir -Force | Out-Null
    if ($arguments) { $p = Start-Process -FilePath $exe -ArgumentList $arguments -PassThru }
    else { $p = Start-Process -FilePath $exe -PassThru }
    for ($i = 0; $i -lt 30; $i++)
    {
        Start-Sleep -Milliseconds 300
        if ($p.HasExited) { break }
    }
    if (-not $p.HasExited)
    {
        [void]$p.CloseMainWindow()
        Start-Sleep -Milliseconds 500
        if (-not $p.HasExited) { $p.Kill(); return "TIMEOUT" }
    }
    $file = Join-Path $dir $expect
    if (-not (Test-Path $file)) { return ("FAIL exit=" + $p.ExitCode + " no file") }
    $text = (Get-Content $file -Raw).Trim()
    if ($content -and $text -ne $content) { return ("FAIL content='" + $text + "'") }
    if ($p.ExitCode -ne 0) { return ("FAIL exit=" + $p.ExitCode) }
    return "PASS"
}

$mods = @(
    @{ name = "7zsd.sfx";            method = "LZMA" },
    @{ name = "7zsd_x64.sfx";        method = "LZMA" },
    @{ name = "7zsd_LZMA.sfx";       method = "LZMA" },
    @{ name = "7zsd_LZMA_x64.sfx";   method = "LZMA" },
    @{ name = "7zsd_LZMA2.sfx";      method = "LZMA2" },
    @{ name = "7zsd_LZMA2_x64.sfx";  method = "LZMA2" },
    @{ name = "7zsd_Deflate.sfx";    method = "Deflate" },
    @{ name = "7zsd_Deflate_x64.sfx"; method = "Deflate" },
    @{ name = "7zsd_PPMd.sfx";       method = "PPMd" },
    @{ name = "7zsd_PPMd_x64.sfx";   method = "PPMd" },
    @{ name = "7zsd_All.sfx";        method = "LZMA2" },
    @{ name = "7zsd_All_x64.sfx";    method = "LZMA2" }
)

$archives = @{}
foreach ($m in @("LZMA", "LZMA2", "Deflate", "PPMd"))
{
    $archives[$m] = Join-Path $outRoot ("payload_" + $m + ".7z")
    MakeArchive $m $archives[$m]
}

$results = New-Object System.Collections.Generic.List[string]
foreach ($mod in $mods)
{
    $exePath = Join-Path $modDir $mod.name
    $method = $mod.method
    $arch = $archives[$method]
    $base = [IO.Path]::GetFileNameWithoutExtension($mod.name)

    $dirClassic = Join-Path $outRoot ("out_" + $base + "_classic")
    $cfg = Join-Path $outRoot ("cfg_" + $base + ".txt")
    Write-Config $cfg $dirClassic ""
    $exeCopy = Join-Path $outRoot ($base + "_classic.exe")
    $copyCmd = "copy /b `"$exePath`" + `"$cfg`" + `"$arch`" `"$exeCopy`""
    cmd /c $copyCmd | Out-Null
    $r1 = Run-Test $exeCopy "" $dirClassic "hello.txt" ("hello " + $method)

    $dirLoader = Join-Path $outRoot ("out_" + $base + "_loader")
    $lcfg = Join-Path $outRoot ("lcfg_" + $base + ".txt")
    Write-Config $lcfg $dirLoader ""
    $loaderExe = Join-Path $outRoot ($base + "_loader.exe")
    $copyCmd2 = "copy /b `"$exePath`" + `"$lcfg`" `"$loaderExe`""
    cmd /c $copyCmd2 | Out-Null
    $r2 = Run-Test $loaderExe ("`"" + $arch + "`"") $dirLoader "hello.txt" ("hello " + $method)

    $line = ("{0,-22} classic={1,-16} loader={2}" -f $mod.name, $r1, $r2)
    $results.Add($line)
}

# password test on the All module
$dirPwd = Join-Path $outRoot "out_password"
$pwdArch = Join-Path $outRoot "payload_pwd.7z"
$srcPwd = Join-Path $outRoot "src_pwd"
New-Item -ItemType Directory $srcPwd -Force | Out-Null
Set-Content (Join-Path $srcPwd "hello.txt") "hello pwd" -Encoding ASCII
Push-Location $srcPwd
& $seven a -t7z -m0=lzma2 -mhe=on -p"PortTest123" $pwdArch "hello.txt" | Out-Null
Pop-Location
$cfgPwd = Join-Path $outRoot "cfg_pwd.txt"
Write-Config $cfgPwd $dirPwd 'Password="PortTest123"'
$exePwd = Join-Path $outRoot "pwd_test.exe"
$allMod = Join-Path $modDir "7zsd_All.sfx"
$copyCmd3 = "copy /b `"$allMod`" + `"$cfgPwd`" + `"$pwdArch`" `"$exePwd`""
cmd /c $copyCmd3 | Out-Null
$rp = Run-Test $exePwd "" $dirPwd "hello.txt" "hello pwd"
$results.Add(("{0,-22} classic={1}" -f "Password (All)", $rp))

$results | ForEach-Object { $_ }
"---"
"total: " + $results.Count
$bad = @($results | Where-Object { $_ -match "FAIL|TIMEOUT" }).Count
"failed: " + $bad
