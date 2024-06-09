#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Windows PowerShell common utilities.
#
#  List of exported functions:
#
#  - Exit-Script [[-Message] <String>] [-NoPause]
#  - Split-DeepString [-Data] <Object> [-Separator] <String> [-Trim] [-IgnoreEmpty]
#  - Search-File [-File] <String> [[-SearchPath] <Object>]
#  - Get-FileInPath [[-File] <String>] [[-SearchPath] <Object>]
#  - Join-MultiPath [-Segments] <String[]>
#  - Get-DotNetVersion
#  - New-Directory [-Path] <Object>
#  - New-TempDirectory
#  - New-ZipFile [-Path] <String> [[-Root] <String>] [-Input <Object>] [-Force
#  - Find-MSBuild
#  - Find-Java
#
#-----------------------------------------------------------------------------


<#
 .SYNOPSIS
  Exit this script. If optional variable $NoPause is $false, pause.

 .PARAMETER Message
  Optional error message. If not empty, exit with error code 1.

 .PARAMETER NoPause
  Do not wait for the user to press <enter> at end of execution. By default,
#>
function Exit-Script
{
    param(
        [Parameter(Mandatory=$false,Position=1)][String] $Message = "",
        [switch]$NoPause = $false
    )

    $Code = 0
    if ($Message -ne "") {
        Write-Host "ERROR: $Message"
        $Code = 1
    }
    if (-not $NoPause) {
        pause
    }
    exit $Code
}
Export-ModuleMember -Function Exit-Script

<#
 .SYNOPSIS
  Split an arbitrary object in multiple strings.

 .PARAMETER Data
  The data to split. Can be a string, an array or anything.

 .PARAMETER Separator
  The separator string to use for the split.

 .PARAMETER Trim
  If specified, all returned strings are trimmed.

 .PARAMETER IgnoreEmpty
  If specified, empty strings are omitted in the result array.

 .OUTPUTS
  An array of strings.
#>
function Split-DeepString
{
    [CmdletBinding()]
    param(
        [Parameter(Mandatory=$true,Position=1)] $Data,
        [Parameter(Mandatory=$true,Position=2)] [String] $Separator,
        [switch] $Trim,
        [switch] $IgnoreEmpty
    )

    # Create an array of strings with zero elements.
    $result = New-Object String[] 0

    # Add elements in the array, based on the parameter type.
    if ($Data.GetType().IsArray) {
        # Recursively split the array elements.
        foreach ($element in $Data) {
            $parameters = @{Data = $element; Separator = $Separator; Trim = $Trim; IgnoreEmpty = $IgnoreEmpty}
            $result += Split-DeepString @parameters
        }
    }
    else {
        # Convert parameter into a string and split.
        foreach ($element in $($Data.toString() -split $Separator)) {
            if ($Trim) {
                $element = $element.Trim()
            }
            if (-not $element -eq "" -or -not $IgnoreEmpty) {
                $result += $element
            }
        }
    }
    return $result
}
Export-ModuleMember -Function Split-DeepString

<#
 .SYNOPSIS
  Search a file in a search path.

 .PARAMETER File
  Name of the file to search.

 .PARAMETER SearchPath
  A list of directories. Can be a string or an array of strings. Each string may
  contain a semi-colon-separated list of directories. By default, use the value
  of the environment variable Path ($env:Path).

 .OUTPUTS
  Return the first file which is found or $null if none is found.
#>
function Search-File
{
    [CmdletBinding()]
    param(
        [Parameter(Mandatory=$true,Position=1)] [String] $File,
        [Parameter(Mandatory=$false,Position=2)] $SearchPath=$env:Path
    )

    # Loop on all provided directories...
    foreach ($dir in $(Split-DeepString $SearchPath ";" -Trim -IgnoreEmpty)) {
        $path = (Join-Path $dir $File)
        if (Test-Path $path) {
            return $path
        }
    }
    # No suitable file found.
    return $null
}
Export-ModuleMember -Function Search-File

<#
 .SYNOPSIS
  Search a file in a search path and and exit the script if not found.

 .PARAMETER File
  Name of the file to search.

 .PARAMETER SearchPath
  A list of directories. Can be a string or an array of strings. Each string may
  contain a semi-colon-separated list of directories. By default, use the value
  of the environment variable Path ($env:Path).

 .OUTPUTS
  Return the first file which is found or exit script if none is found.
#>
function Get-FileInPath ([string]$File, $SearchPath)
{
    $Path = Search-File $File $SearchPath
    if (-not $Path) {
        Exit-Script -NoPause:$NoPause "$File not found"
    }
    return $Path
}
Export-ModuleMember -Function Get-FileInPath

<#
 .SYNOPSIS
  Join multiple segments of a file path, just like Join-Path, but not limited to two parts.

 .PARAMETER Segments
  An array of strings containing the parts of the path to join.

 .OUTPUTS
  The full path as a string.
#>
function Join-MultiPath
{
    [CmdletBinding()]
    param(
        [Parameter(Mandatory=$true,Position=1)][String[]] $Segments
    )

    $path = ""
    foreach ($seg in $Segments) {
        if ($path) {
            $path = Join-Path $path $seg
        }
        else {
            $path = $seg
        }
    }
    return $path
}
Export-ModuleMember -Function Join-MultiPath

<#
 .SYNOPSIS
  Get the version of the .NET framework as an integer in the form 100 * Major + Minor.
#>
function Get-DotNetVersion()
{
    (@(([Environment]::Version.Major * 100) + [Environment]::Version.Minor) +
    (Get-ChildItem 'HKLM:\SOFTWARE\Microsoft\NET Framework Setup\NDP' -Recurse |
        Get-ItemProperty -Name Version -ErrorAction SilentlyContinue |
        Where { $_.PSChildName -match '^([Cc]lient|[Ff]ull|[Vv]\d)'} |
        ForEach-Object {
            $fields = ($_.Version -split '.',0,"SimpleMatch") + (0, 0)
            (($fields[0] -as [int]) * 100) + ($fields[1] -as [int])
        }) |
    Measure-Object -Maximum).Maximum
}
Export-ModuleMember -Function Get-DotNetVersion

<#
 .SYNOPSIS
  Create a directory.

 .PARAMETER Path
  The path of the directory to create. Can be either a string (the path itself)
  or an array of strings (joined to form a full path).

 .OUTPUTS
  The full path of the directory as a string.
#>
function New-Directory
{
    [CmdletBinding()]
    param(
        [Parameter(Mandatory=$true,Position=1)] $Path
    )

    # Get the path of the directory to create.
    if ($Path.GetType().IsArray) {
        $Path = Join-MultiPath $Path
    }

    # Create the directory and return full name.
    (New-Item -Path $Path -ItemType Directory -Force).FullName
}
Export-ModuleMember -Function New-Directory

<#
 .SYNOPSIS
  Create a temporary directory.

 .OUTPUTS
  The full path of the directory as a string.
#>
function New-TempDirectory
{
    # The function GetTempFileName() creates a temporary file.
    $TempFile = [System.IO.Path]::GetTempFileName()
    try {
        # Derive a name for a temporary directory.
        $TempDir = [System.IO.Path]::ChangeExtension($TempFile, ".dir")
        # Create the temporary directory.
        [void] (New-Item -ItemType Directory -Force $TempDir)
        return $TempDir
    }
    finally {
        # Delete the temporary file, now useless.
        if (Test-Path $TempFile) {
            Remove-Item $TempFile -Force
        }
    }
}
Export-ModuleMember -Function New-TempDirectory

<#
 .SYNOPSIS
  Create a zip file from any files piped in.

 .PARAMETER Path
  The name of the zip archive to create.

 .PARAMETER Root
  Store directory names in the zip entries. Use the same hierarchy as input
  files but strip the root from their full name. If unspecified, create a
  flat archive of files without hierarchy.

 .PARAMETER Force
  If specified, delete the zip archive if it already exists.
#>
function New-ZipFile
{
    [CmdletBinding()]
    param(
        [Parameter(Mandatory=$true,Position=1)][String] $Path,
        [Parameter(Mandatory=$false,Position=2)][String] $Root = $null,
        [Parameter(ValueFromPipeline=$true)] $Input,
        [Switch] $Force
    )

    Set-StrictMode -Version 3

    # Check if the file exists already.
    $ZipName = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Path)
    if (Test-Path $ZipName) {
        if ($Force) {
            Remove-Item $ZipName -Force
        }
        else {
            # Zip file exists and no -Force option, generate an error.
            throw "$ZipName already exists."
        }
    }

    # Build the root path, i.e. the path to strip from entries directory path.
    if ($Root) {
        $Root = (Get-Item $Root).FullName
    }

    # Add the DLL that helps with file compression.
    # This requires .NET 4.5 (FileNotFoundException on previous releases).
    Add-Type -Assembly System.IO.Compression.FileSystem

    try {
        # Open the Zip archive
        $archive = [System.IO.Compression.ZipFile]::Open($ZipName, "Create")

        # Go through each file in the input, adding it to the Zip file specified
        foreach ($file in $Input) {
            $item = $file | Get-Item
            # Skip the current file if it is the zip file itself
            if ($item.FullName -eq $ZipName) {
                continue
            }
            # Skip directories
            if ($item.PSIsContainer) {
                continue
            }
            # Compute entry name in archive.
            if ($Root -and $item.FullName -like "$Root\*") {
                $name = $item.FullName.Substring($Root.Length + 1)
            }
            else {
                $name = $item.Name
            }
            # Add the file to the archive.
            $null = [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $item.FullName, $name)
        }
    }
    finally {
        # Close the file
        $archive.Dispose()
        $archive = $null
    }
}
Export-ModuleMember -Function New-ZipFile

<#
 .SYNOPSIS
  Find MSBuild.exe, regardless of Visual Studio version.

 .OUTPUTS
  Return the MSBuild.exe with the highest version or exit script if none is found.
#>
function Find-MSBuild
{
    # Try fast path first
    $Path = Get-ChildItem "C:\Program Files\Microsoft Visual Studio\*\*\MSBuild\Current\Bin\MSBuild.exe" | Select-Object -Last 1
    if ($Path -ne $null) {
        $Path = $Path.FullName
    }
    else {
        # Fallback: search everywhere
        $MSRoots = @("C:\Program Files*\MSBuild", "C:\Program Files*\Microsoft Visual Studio")
        $Path = Get-ChildItem -Recurse -Path $MSRoots -Include MSBuild.exe -ErrorAction Ignore |
                ForEach-Object { (Get-Command $_).FileVersionInfo } |
                Sort-Object -Unique -Property FileVersion |
                ForEach-Object { $_.FileName} |
                Select-Object -Last 1
    }
    if (-not $Path) {
        Exit-Script -NoPause:$NoPause "MSBuild not found"
    }
    return $Path
}
Export-ModuleMember -Function Find-MSBuild

<#
 .SYNOPSIS
  Find MSVC runtime redistributable package, regardless of Visual Studio version.

 .PARAMETER
  Wildcard for package name, typically "vc*redist*x64.exe" or "vc*redist*86.exe".
  The name of the package has slightly changed over the years and it is dangerous
  to specify a fixed name.

 .OUTPUTS
  Return the package exe with the highest version or exit script if none is found.
#>
function Find-VCRedist
{
    [CmdletBinding()]
    param(
        [Parameter(Mandatory=$true,Position=1)][String] $Name
    )

    # Try fast path first
    $Path = Get-ChildItem "C:\Program Files\Microsoft Visual Studio\*\*\VC\Redist\MSVC\*\$Name" | Select-Object -Last 1
    if ($Path -ne $null) {
        $Path = $Path.FullName
    }
    else {
        # Fallback: search everywhere
        $Path = Get-ChildItem -Recurse -Path "C:\Program Files*\Microsoft Visual Studio" -Include $Name -ErrorAction Ignore |
                 ForEach-Object { (Get-Command $_).FileVersionInfo } |
                 Sort-Object -Unique -Property FileVersion  |
                 ForEach-Object { $_.FileName} | Select-Object -Last 1
    }
    if (-not $Path) {
        Exit-Script -NoPause:$NoPause "MSVC Redistributable Libraries Installer $Name not found"
    }
    return $Path
}
Export-ModuleMember -Function Find-VCRedist

<#
 .SYNOPSIS
  Find the Java environment.

 .OUTPUTS
  Return the path of the directory containing java.exe, javac.exe and jar.exe.
  Return the empty directory if not found.
#>
function Find-Java
{
    $jh = $env:JAVA_HOME
    if (-not -not $jh -and (Test-Path "$jh\bin\java.exe") -and (Test-Path "$jh\bin\javac.exe") -and (Test-Path "$jh\bin\jar.exe")) {
        return "$jh\bin"
    }
    else {
        return ""
    }
}
Export-ModuleMember -Function Find-Java
