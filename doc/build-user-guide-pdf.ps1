#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
#
#-----------------------------------------------------------------------------

<#
 .SYNOPSIS

  Build the PDF version of the TSDuck user's guide on Windows.

 .DESCRIPTION

  Invoke Microsoft Word, load the Word version of the user's guide and
  save a PDF version.

 .PARAMETER Date

  Specify the reference date for the documentation (current day by default).

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.
#>
[CmdletBinding()]
param([string]$Date = "", [switch]$NoPause = $false)

# Reference date.
if ($Date -ne "") {
    $RefDate = (Get-Date -Date $Date)
}
else {
    $RefDate = (Get-Date)
}

# Get the project directories.
$RootDir = (Split-Path -Parent $PSScriptRoot)
$DocDir = $PSScriptRoot
$SrcDir = "$RootDir\src"

# Input and output file names.
$DocIn = "$DocDir\tsduck.docx"
$DocOut = "$DocDir\tsduck.pdf"

# Get TSDuck version. Increment commit count since we are going to create a new version.
$Version = (& "$RootDir\scripts\get-version-from-sources.py" --main)
$Commit = [int](& "$RootDir\scripts\get-version-from-sources.py" --commit) + 1
$Version = "${Version}-${Commit}"

# The following properties are set in the document:
# - Version (e.g. 3.9-575)
# - RevisionDate (e.g. March 2018)
# - DateOfCopyright (e.g. 2005-2018)
$PropVersion = "Version"
$PropDate = "RevisionDate"
$PropCopyright = "DateOfCopyright"

# Get current month and year (force English language)
$en = New-Object System.Globalization.CultureInfo("en-US")
$InitialCopyrightYear = "2005"
$CurrentYear = [String]($RefDate.Year)
$CurrentMonth = $en.TextInfo.ToTitleCase($en.DateTimeFormat.MonthNames[$RefDate.Month - 1])

# Load types from Microsoft Word automation.
Add-Type -AssemblyName Microsoft.Office.Interop.Word
$BindingFlags = "System.Reflection.BindingFlags" -as [Type]

# A function to set the value a custom document property.
# Warning, there is a lot a black magic inside !
function Set-CustomDocumentProperties($doc, $name, $value)
{
    $properties = $doc.CustomDocumentProperties
    try {
        $propertiesType = $properties.GetType()
        $prop = $propertiesType.InvokeMember("Item", $BindingFlags::GetProperty, $null, $properties, @($name))
    }
    catch {
        $prop = [System.__ComObject].InvokeMember("Item", $BindingFlags::GetProperty, $null, $properties, @($name))        
    }
    if ($prop -ne $null) {
        try {
            $propType = $prop.GetType()
            $propType.InvokeMember("Value", $BindingFlags::SetProperty, $null, $prop, @($value))
        }
        catch {
            [System.__ComObject].InvokeMember("Value", $BindingFlags::SetProperty, $null, $prop, @($value))
        }
    }
}

# Open Word document.
Write-Output "Loading $DocIn"
$word = New-Object -ComObject Word.Application
$word.Visible = $False
$doc = $word.Documents.Open($DocIn)

# Update the document properties.
Write-Output "New document version is $Version"
Write-Output "Reference date is $CurrentMonth $CurrentYear"
Write-Output "Updating document fields"
Set-CustomDocumentProperties $doc $PropVersion $Version
Set-CustomDocumentProperties $doc $PropDate "${CurrentMonth} ${CurrentYear}"
Set-CustomDocumentProperties $doc $PropCopyright "${InitialCopyrightYear}-${CurrentYear}"

# Update main content of the document.
[void]$doc.Fields.Update()

# Iterate through sections and update header and footer.
Write-Output "Updating headers and footers"
foreach ($sect in $doc.Sections) {
    [void]$sect.Headers.Item(1).Range.Fields.Update()
    [void]$sect.Footers.Item(1).Range.Fields.Update()
}

# Update tables of contents.
Write-Output "Updating tables of contents"
foreach ($table in $doc.TablesOfContents) {
    [void]$table.Update()
}
foreach ($table in $doc.TablesOfFigures) {
    [void]$table.Update()
}

# Save as PDF. Make sure true type fonts are embedded in the document.
$doc.EmbedTrueTypeFonts = $true
Write-Output "Saving $DocOut"
$doc.ExportAsFixedFormat($DocOut.ToString(),
                         [Microsoft.Office.Interop.Word.WdExportFormat]::wdExportFormatPDF,
                         $false,  # open after export
                         [Microsoft.Office.Interop.Word.WdExportOptimizeFor]::wdExportOptimizeForOnScreen,
                         [Microsoft.Office.Interop.Word.WdExportRange]::wdExportAllDocument,
                         0,       # from (unused with "all document")
                         0,       # to
                         [Microsoft.Office.Interop.Word.WdExportItem]::wdExportDocumentContent,
                         $true,   # include document properties
                         $true,   # keep IRM (XPS only)
                         [Microsoft.Office.Interop.Word.WdExportCreateBookmarks]::wdExportCreateHeadingBookmarks,
                         $true,   # include doc structure tags
                         $false,  # include missing fonts as bitmaps
                         $false)  # restrict to PDF/A (ISO 19005-1)

# Save as Word document. Do not embed true type fonts here.
$doc.EmbedTrueTypeFonts = $false
Write-Output "Saving $DocIn"
$doc.Close([Microsoft.Office.Interop.Word.WdSaveOptions]::wdSaveChanges,
           [Microsoft.Office.Interop.Word.WdOriginalFormat]::wdOriginalDocumentFormat)

# Quite Word and cleanup spurious process.
$word.Quit()
[void][System.Runtime.InteropServices.Marshal]::ReleaseComObject([System.__ComObject]$word)
[gc]::Collect()
[gc]::WaitForPendingFinalizers()
Remove-Variable word

# Wait for user input before exiting (and closing the PowerShell window).
if (-not $NoPause) {
    pause
}
