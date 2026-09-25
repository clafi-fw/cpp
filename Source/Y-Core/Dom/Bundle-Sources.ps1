# Bundle-Sources.ps1
# Scans this script's folder for .cpp / .cppm files and writes them into one
# bundle file with clear separators.
#
# Usage:
#   Right-click -> Run with PowerShell, or double-click Bundle-Sources.cmd
#   .\Bundle-Sources.ps1 -NoRecurse          (top-level folder only)
#   .\Bundle-Sources.ps1 -OutFile my.txt

[CmdletBinding()]
param(
    [string]   $OutFile    = "bundle.txt",
    [switch]   $NoRecurse,
    [string[]] $Extensions = @('*.cpp', '*.cppm')
)

$ErrorActionPreference = 'Stop'

$root = if ($PSScriptRoot) { $PSScriptRoot } else { (Get-Location).Path }
$root = (Resolve-Path -LiteralPath $root).Path.TrimEnd('\')
$outPath = if ([System.IO.Path]::IsPathRooted($OutFile)) { $OutFile } else { Join-Path $root $OutFile }

# Works on Windows PowerShell 5.1 (no [IO.Path]::GetRelativePath there)
function Get-RelPath([string]$Base, [string]$Full) {
    if ($Full.StartsWith($Base, [StringComparison]::OrdinalIgnoreCase)) {
        return $Full.Substring($Base.Length).TrimStart('\')
    }
    return $Full
}

# Note: -Include needs a wildcard path, not a bare directory
$files = Get-ChildItem -Path (Join-Path $root '*') -Include $Extensions -File -Recurse:(-not $NoRecurse) |
         Where-Object { $_.FullName -ne $outPath } |
         Sort-Object FullName

if (-not $files) {
    Write-Host "No .cpp or .cppm files found in $root" -ForegroundColor Yellow
    return
}

$sb  = [System.Text.StringBuilder]::new()
$bar = '=' * 78

[void]$sb.AppendLine($bar)
[void]$sb.AppendLine("SOURCE BUNDLE")
[void]$sb.AppendLine("Root      : $root")
[void]$sb.AppendLine("Generated : $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')")
[void]$sb.AppendLine("Files     : $($files.Count)")
[void]$sb.AppendLine($bar)
[void]$sb.AppendLine()
[void]$sb.AppendLine("TABLE OF CONTENTS")
$i = 0
foreach ($f in $files) {
    $i++
    $rel = Get-RelPath $root $f.FullName
    [void]$sb.AppendLine(("  {0,3}. {1}" -f $i, $rel))
}
[void]$sb.AppendLine()

$i = 0
foreach ($f in $files) {
    $i++
    $rel  = Get-RelPath $root $f.FullName
    $text = Get-Content -LiteralPath $f.FullName -Raw -Encoding UTF8
    if ($null -eq $text) { $text = '' }
    $lines = $text -split "`r?`n"

    [void]$sb.AppendLine()
    [void]$sb.AppendLine($bar)
    [void]$sb.AppendLine("===== FILE $i/$($files.Count): $rel")
    [void]$sb.AppendLine("===== SIZE: $($f.Length) bytes | LINES: $($lines.Count) | MODIFIED: $($f.LastWriteTime.ToString('yyyy-MM-dd HH:mm:ss'))")
    [void]$sb.AppendLine("===== BEGIN $rel")
    [void]$sb.AppendLine($bar)
    [void]$sb.AppendLine(($lines -join "`n"))
    [void]$sb.AppendLine($bar)
    [void]$sb.AppendLine("===== END $rel")
    [void]$sb.AppendLine($bar)
}

[void]$sb.AppendLine()
[void]$sb.AppendLine($bar)
[void]$sb.AppendLine("END OF BUNDLE - $($files.Count) file(s)")
[void]$sb.AppendLine($bar)

# UTF-8 without BOM
[System.IO.File]::WriteAllText($outPath, $sb.ToString(), (New-Object System.Text.UTF8Encoding $false))

Write-Host "Wrote $($files.Count) file(s) to $outPath" -ForegroundColor Green
