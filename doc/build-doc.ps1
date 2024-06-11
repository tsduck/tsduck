#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Windows PowerShell script to build the documentation.
#
#-----------------------------------------------------------------------------

<#
 .SYNOPSIS

  Build the project documentation.

 .DESCRIPTION

  Build the project documentation.

 .PARAMETER NoOpen

  Do not open the generated documentation. By default, run the default HTML
  browser on the generated documentation home page.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.

 .PARAMETER Version

  Version of the product. The default is extracted from the source file
  tsVersion.h.
#>
param(
    [Parameter(Mandatory=$false)][string]$Version,
    [switch]$NoOpen = $false,
    [switch]$NoPause = $false
)

# Get the project directories.
$RootDir      = (Split-Path -Parent $PSScriptRoot)
$ImagesDir    = "$RootDir\images"
$DocRoot      = "$RootDir\doc"
$CssDir       = "$DocRoot\css"
$TocBotDir    = "$DocRoot\tocbot"
$ThemesDir    = "$DocRoot\themes"
$UserGuideDir = "$DocRoot\user"
$DevGuideDir  = "$DocRoot\developer"
$BinRoot      = "$RootDir\bin"
$BinDoc       = "$BinRoot\doc"
$BinDocInfo   = "$BinRoot\docinfo"

# Common themes.
$CssFile   = "$CssDir\tsduck.css"
$Theme     = "tsduck"
$ThemeFile = "$ThemesDir\${Theme}-theme.yml"
$RougeHtml = "thankful_eyes"
$RougePdf  = "github"

# Enforce English names.
[System.Threading.Thread]::CurrentThread.CurrentUICulture = "en-US"
[System.Threading.Thread]::CurrentThread.CurrentCulture = "en-US"

# Publication date (month + year).
$Date = Get-Date -format "MMMM yyyy"

# Get the product version.
if (-not $Version) {
    $Version = (python "$RootDir\scripts\get-version-from-sources.py")
}

# Asciidoctor flags
$ADocFlags     = @("-v", "-a", "revnumber=$Version", "-a", "revdate=$Date", "-a", "imagesdir=$ImagesDir")
$ADocFlagsHtml = $ADocFlags + @("-a", "stylesheet=$CssFile", "-a", "rouge-style=$RougeHtml", "-a", "data-uri", "-a", "docinfo=shared", "-a", "docinfodir=$BinDocInfo")
$ADocFlagsPdf  = $ADocFlags + @("-a", "pdf-themesdir=$ThemesDir", "-a", "pdf-theme=$Theme", "-a", "rouge-style=$RougePdf")

# Exit the script.
function Exit-Script($Message = "")
{
    if ($Message -ne "") {
        Write-Host "ERROR: $Message"
    }
    if (-not $NoPause) {
        pause
    }
}

# Open a document (HTML, PF, etc.)
function Open-Doc($File)
{
    if (-not (Test-Path $File)) {
        Exit-Script "$File not found"
    }
    if (-not $NoOpen) {
        Invoke-Item $File
    }
}

# Create output directories
[void](New-Item -ItemType Directory -Force $BinDoc)
[void](New-Item -ItemType Directory -Force $BinDocInfo)

# Generate docinfo files.
Copy-Item "$TocBotDir\docinfo.html" "$BinDocInfo\docinfo.html"
("<script>`n" + (Get-Content "$TocBotDir\tocbot.min.js") + "`n</script>`n" + (Get-Content "$TocBotDir\docinfo-footer.in.html")) |
    Out-File "$BinDocInfo\docinfo-footer.html" -Encoding utf8

# Generate a .adoc file which includes all .adoc in a given subdirectory.
function Build-IncludeAll($OutFile, $DocDir, $SubDir)
{
    Get-ChildItem "$DocDir\$SubDir\*.adoc" |
        ForEach-Object { "include::{docdir}/$Subdir/" + $_.Name + "[]" } |
        Sort-Object |
        Out-File $OutFile -Encoding utf8
}

# Generate subdoc files for all commands and all plugins.
Build-IncludeAll "$UserGuideDir\.all.commands.adoc" $UserGuideDir "commands"
Build-IncludeAll "$UserGuideDir\.all.plugins.adoc" $UserGuideDir "plugins"

# Generate one document in HTML and PDF formats.
function Build-Document($Dir, $BaseName)
{
    Write-Output "Generating $BaseName.html ..."
    asciidoctor @ADocFlagsHtml "$Dir\$BaseName.adoc" -D $BinDoc -o "$BaseName.html"
    Open-Doc "$BinDoc\$BaseName.html"

    Write-Output "Generating $BaseName.pdf ..."
    asciidoctor-pdf @ADocFlagsPdf "$Dir\$BaseName.adoc" -D $BinDoc -o "$BaseName.pdf"
    Open-Doc "$BinDoc\tsduck.pdf"
}

# Generate guides
Build-Document $UserGuideDir "tsduck"
Build-Document $DevGuideDir "tsduck-dev"

Exit-Script
