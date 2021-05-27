#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2021, Thierry Lelegard
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
#
#  This script builds the Visual Studio and MSBuild project files for all
#  TSDuck commands and plugins. Each time a new source file appears for a
#  new command or plugin, an MSVC project file shall be created. This is
#  a repetitive task which can be automated. 
#
#-----------------------------------------------------------------------------


# Get the project directories.
BUILDDIR=$(cd $(dirname ${BASH_SOURCE[0]}); pwd)
ROOTDIR=$(cd "$BUILDDIR/.."; pwd)
SRCDIR="$ROOTDIR/src"
MSVCDIR="$BUILDDIR/msvc"

# List of tools and plugins.
TOOLS=$(cd "$SRCDIR/tstools"; ls ts*.cpp 2>/dev/null | sed 's/\.cpp$//')
PLUGINS=$(cd "$SRCDIR/tsplugins"; ls tsplugin_*.cpp 2>/dev/null | sed 's/\.cpp$//')
OTHER="utests-tsduckdll utests-tsducklib tsduckdll tsducklib tsp_static tsprofiling setpath"

# Visual Studio solution description.
SLNFILE="$MSVCDIR/tsduck.sln"
CXX_PROJECT_GUID=8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942
TSDUCK_SLN_GUID=55E5A8EA-215E-45C2-9471-AD5CC5A925A0

# Project dependencies (tsduckdll is implicit for all tools and plugins).
declare -A DEP
DEP[utests-tsduckdll]="tsduckdll tsplugin_merge"
DEP[utests-tsducklib]="tsducklib"
DEP[tsp_static]="tsducklib"
DEP[tsp]="$PLUGINS"
DEP[tsswitch]="tsplugin_dvb tsplugin_hides"
DEP[tsprofiling]="tsduckdll"
DEP[setpath]="tsducklib"

# Get the GUID of a project.
get-guid() {
    grep -i '<ProjectGuid>' "$MSVCDIR/$1.vcxproj" | sed -e 's/.*{//' -e 's/}.*$//' | tr a-f A-F
}

# Generate a random GUID
random-guid() {
    head -c16 /dev/urandom | xxd -p | tr a-f A-F | sed 's/^\(........\)\(....\)\(....\)\(....\)\(............\)$/\1-\2-\3-\4-\5/'
}

# Build non-existent project files. Do not modify existing ones.
for name in $TOOLS; do
    file="$MSVCDIR/$name.vcxproj"
    guid=
    [[ -e "$file" ]] && guid=$(get-guid $name)
    [[ -z "$guid" ]] && guid=$(random-guid)
    cat <<EOF >"$file"
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" ToolsVersion="15.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">

  <ImportGroup Label="PropertySheets">
    <Import Project="msvc-common-begin.props" />
  </ImportGroup>

  <ItemGroup>
    <ClCompile Include="..\\..\\src\\tstools\\$name.cpp" />
  </ItemGroup>

  <PropertyGroup Label="Globals">
    <ProjectGuid>{$guid}</ProjectGuid>
    <Keyword>Win32Proj</Keyword>
    <RootNamespace>$name</RootNamespace>
  </PropertyGroup>

  <ImportGroup Label="PropertySheets">
    <Import Project="msvc-target-exe.props" />
    <Import Project="msvc-use-tsduckdll.props" />
    <Import Project="msvc-common-end.props" />
  </ImportGroup>

</Project>
EOF
    unix2dos -q "$file"
done

for name in $PLUGINS; do
    file="$MSVCDIR/$name.vcxproj"
    guid=
    [[ -e "$file" ]] && guid=$(get-guid $name)
    [[ -z "$guid" ]] && guid=$(random-guid)
    cat <<EOF >"$file"
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" ToolsVersion="15.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">

  <ImportGroup Label="PropertySheets">
    <Import Project="msvc-common-begin.props" />
  </ImportGroup>

  <ItemGroup>
    <ClCompile Include="..\\..\\src\\tsplugins\\$name.cpp" />
  </ItemGroup>

  <PropertyGroup Label="Globals">
    <ProjectGuid>{$guid}</ProjectGuid>
    <Keyword>Win32Proj</Keyword>
    <RootNamespace>$name</RootNamespace>
  </PropertyGroup>

  <ImportGroup Label="PropertySheets">
    <Import Project="msvc-target-dll.props" />
    <Import Project="msvc-use-tsduckdll.props" />
    <Import Project="msvc-common-end.props" />
  </ImportGroup>

</Project>
EOF
    unix2dos -q "$file"
done

# Generate solution file prolog.
printf '\xef\xbb\xbf' >"$SLNFILE"
cat <<EOF >>"$SLNFILE"

Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 16
VisualStudioVersion = 16.0.29020.237
MinimumVisualStudioVersion = 10.0.40219.1
EOF

# Generate a project description. Syntax: gen-project project dependencies ...
gen-project() {
    local project=$1
    shift
    local guid=$(get-guid $project)
    echo 'Project("{'$CXX_PROJECT_GUID'}") = "'$project'", "'$project'.vcxproj", "{'$guid'}"'
    if [[ $# -gt 0 ]]; then
        echo -e '\tProjectSection(ProjectDependencies) = postProject'
        for dep in $*; do
            guid=$(get-guid $dep)
            echo -e '\t\t{'$guid'} = {'$guid'}'
        done
        echo -e '\tEndProjectSection'
    fi
    echo "EndProject"
}

# Generate all project descriptions.
for project in $OTHER; do
    gen-project $project ${DEP[$project]}
done >>"$SLNFILE"
for project in $PLUGIN $TOOLS; do
    gen-project $project tsduckdll ${DEP[$project]}
done >>"$SLNFILE"

# Generate solution file epilog.
cat <<EOF >>"$SLNFILE"
Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
		Debug|Win32 = Debug|Win32
		Debug|x64 = Debug|x64
		Release|Win32 = Release|Win32
		Release|x64 = Release|x64
	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution
EOF

for project in $OTHER $PLUGIN $TOOLS; do
    guid=$(get-guid $project)
    cat <<EOF >>"$SLNFILE"
		{$guid}.Debug|Win32.ActiveCfg = Debug|Win32
		{$guid}.Debug|Win32.Build.0 = Debug|Win32
		{$guid}.Debug|x64.ActiveCfg = Debug|x64
		{$guid}.Debug|x64.Build.0 = Debug|x64
		{$guid}.Release|Win32.ActiveCfg = Release|Win32
		{$guid}.Release|Win32.Build.0 = Release|Win32
		{$guid}.Release|x64.ActiveCfg = Release|x64
		{$guid}.Release|x64.Build.0 = Release|x64
EOF
done

cat <<EOF >>"$SLNFILE"
	EndGlobalSection
	GlobalSection(SolutionProperties) = preSolution
		HideSolutionNode = FALSE
	EndGlobalSection
	GlobalSection(ExtensibilityGlobals) = postSolution
		SolutionGuid = {$TSDUCK_SLN_GUID}
	EndGlobalSection
EndGlobal
EOF
unix2dos -q "$SLNFILE"
