#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
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

 .PARAMETER User

  Generate the user guide only. By default, generate all documents.

 .PARAMETER Developer

  Generate the developer guide only. By default, generate all documents.

 .PARAMETER Html

  Generate the HTML files only. By default, generate all output formats.

 .PARAMETER Pdf

  Generate the PDF files only. By default, generate all output formats.

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
    [switch]$User = $false,
    [switch]$Developer = $false,
    [switch]$Html = $false,
    [switch]$Pdf = $false,
    [switch]$NoOpen = $false,
    [switch]$NoPause = $false
)

# By default, generate all guides in all formats.
if (-not $User -and -not $Developer) {
    $User = $true
    $Developer = $true
}
if (-not $Html -and -not $Pdf) {
    $Html = $true
    $Pdf = $true
}

# Get the project directories.
$RootDir      = (Split-Path -Parent $PSScriptRoot)
$SrcDir       = "$RootDir\src"
$ImagesDir    = "$RootDir\images"
$DocRoot      = "$RootDir\doc"
$AdocDir      = "$DocRoot\adoc"
$UserGuideDir = "$DocRoot\user"
$DevGuideDir  = "$DocRoot\developer"
$BinRoot      = "$RootDir\bin"
$BinDoc       = "$BinRoot\doc"
$BinDocInfo   = "$BinRoot\docinfo"

# Common themes.
$CssFile   = "$AdocDir\tsduck.css"
$Theme     = "tsduck"
$ThemeFile = "$AdocDir\${Theme}-theme.yml"
$RougeHtml = "thankful_eyes"
$RougePdf  = "github"

# Specific doc files.
$UserSiXml = “$UserGuideDir\20D-app-si-xml.adoc”

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
$ADocFlags     = @("-v", "-a", "revnumber=$Version", "-a", "revdate=$Date", "-a", "imagesdir=$ImagesDir", "-a", "includedir=$AdocDir")
$ADocFlagsHtml = $ADocFlags + @("-a", "stylesheet=$CssFile", "-a", "rouge-style=$RougeHtml", "-a", "data-uri", "-a", "docinfo=shared", "-a", "docinfodir=$BinDocInfo")
$ADocFlagsPdf  = $ADocFlags + @("-a", "pdf-themesdir=$AdocDir", "-a", "pdf-theme=$Theme", "-a", "rouge-style=$RougePdf")

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
if ($Html) {
    # Load TSDuck as a string (SVG is XML text).
    $logo = [IO.File]::ReadAllText("$ImagesDir\tsduck-logo.svg")
    $info = "$BinDocInfo\docinfo.html"
    $footer = "$BinDocInfo\docinfo-footer.html"
    # Generate docinfo.html (will go into <head>).
    Get-Content "$AdocDir\docinfo.in.html" | Out-File $info -Encoding utf8
    "<style>`n" +
        "img.tsduck-logo {content: url(data:image/svg+xml;base64," +
        [System.Convert]::ToBase64String([System.Text.Encoding]::UTF8.GetBytes($logo)) +
        ");}`n" +
        "</style>" | Out-File $info -Encoding utf8 -Append
    # Generate docinfo-footer.html (will go at end of document).
    "<script>" | Out-File $footer -Encoding utf8
    Get-Content "$AdocDir\tocbot.min.js" | Out-File $footer -Encoding utf8 -Append
    "</script>" | Out-File $footer -Encoding utf8 -Append
    Get-Content "$AdocDir\docinfo-footer.in.html" | Out-File $footer -Encoding utf8 -Append
}

# Generate a .adoc file which includes all .adoc in a given subdirectory.
function Build-IncludeAll($OutFile, $DocDir, $SubDir)
{
    Get-ChildItem "$DocDir\$SubDir\*.adoc" |
        ForEach-Object { "include::{docdir}/$Subdir/" + $_.Name + "[]" } |
        Sort-Object |
        Out-File $OutFile -Encoding utf8
}

# Generate subdoc files for all commands and all plugins in user guide.
if ($User) {
    Build-IncludeAll "$UserGuideDir\.all.commands.adoc" $UserGuideDir "commands"
    Build-IncludeAll "$UserGuideDir\.all.plugins.adoc" $UserGuideDir "plugins"
}

# Generate adoc files for all standards for which tables or descriptors are defined.
if ($User) {
    foreach ($Type in @("tables", "descriptors")) {
        $Standards = Get-ChildItem "$SrcDir\libtsduck\dtv\$Type\*\*.adoc" | ForEach-Object { Split-Path -Leaf $_.DirectoryName } | Sort-Object -Unique
        foreach ($Std in $Standards) {
            $OutName = ".all.$Std.$Type.adoc"
            $OutFile = "$UserGuideDir\$OutName"
            # Check that this file is correctly referenced in the user guide.
            # If someone adds a new standard, created a new directory, and forgot to update the main document.
            if (-not (Select-String -Path $UserSiXml -Pattern “^include::$OutName\[]” -Quiet)) {
                Write-Error "File $OutName not included in $UserSiXml"
            }
            # Generate the file.
            $InFiles = @{}
            Get-ChildItem "$SrcDir\libtsduck\dtv\$Type\$Std\*.adoc" | Select-String -pattern "^==== " | ForEach-Object {
               $file = $_ -replace ':[0-9]*:====.*',''
               $key = $_ -replace '.*:[0-9]*:==== *',''
               $InFiles[$key] = $file
            }
            ''| Out-File $OutFile -Encoding utf8
            $InFiles.Keys | Sort-Object -Unique -Culture ([CultureInfo]::InvariantCulture) | ForEach-Object {
                Get-Content $InFiles[$_] | Out-File $OutFile -Encoding utf8 -Append
                '' | Out-File $OutFile -Encoding utf8 -Append
            }
        }
    }
}

# Generate subdoc files for all tables and all descriptors in developer guide.
if ($Developer) {
    python "$DevGuideDir\build-sigref.py" tables "$DevGuideDir\.all.tables.adoc"
    python "$DevGuideDir\build-sigref.py" descriptors "$DevGuideDir\.all.descriptors.adoc"
}

# Generate one document in HTML and/or PDF formats.
function Build-Document($Dir, $BaseName)
{
    if ($Html) {
        Write-Output "Generating $BaseName.html ..."
        asciidoctor @ADocFlagsHtml "$Dir\$BaseName.adoc" -D $BinDoc -o "$BaseName.html"
        Open-Doc "$BinDoc\$BaseName.html"
    }
    if ($Pdf) {
        Write-Output "Generating $BaseName.pdf ..."
        asciidoctor-pdf @ADocFlagsPdf "$Dir\$BaseName.adoc" -D $BinDoc -o "$BaseName.pdf"
        Open-Doc "$BinDoc\$BaseName.pdf"
    }
}

# Generate guides
if ($User) {
    Build-Document $UserGuideDir "tsduck"
}
if ($Developer) {
    Build-Document $DevGuideDir "tsduck-dev"
}

Exit-Script
