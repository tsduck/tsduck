#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  This script builds the project files for "Qt Creator" and "Visual Studio"
#  (also used by MSBuild) for all TSDuck commands and plugins.
#
#  Each time a new source file appears for a new command or plugin, specific
#  project files shall be created to make it visible to the IDE. This is
#  a repetitive task which is automated here.
#
#  On Linux and macOS, Qt Creator is used as a generic C++ IDE, even without
#  using Qt. The project files are only used to edit and debug TSDuck. Real
#  builds are processed using "make" and makefiles.
#
#  On Windows, Visual Studio is used to edit and debug TSDuck. The PowerShell
#  build scripts directly invoke MSBuild without using Visual Studio. The same
#  project files are used by Visual Studio and MSBuild. Note that source files
#  in libtsduck are referenced using wildcards in the project files. This is
#  supported by MSBuild but not by Visual Studio. In practice, its works with
#  Visual Studio with the following constraint: each time a new source file
#  is added, Visual Studio must be restarted after deleting the directory
#  msvc\.vs (this is only a file cache, it does not invalidate the previously
#  compiled object files).
#
#-----------------------------------------------------------------------------

import tsbuild, os, binascii
import xml.etree.ElementTree as xmlet

# Get the project directories.
script_name = os.path.basename(__file__)
src_dir = tsbuild.repo_root() + '/src'
qt_dir = tsbuild.scripts_dir() + '/qtcreator'
ms_dir = tsbuild.scripts_dir() + '/msvc'

# Get the list of .cpp files in a directory, without .cpp extension.
def get_cpp(dirname):
    sources = []
    for name in os.listdir(dirname):
        ext = os.path.splitext(name)
        if ext[1] == '.cpp':
            sources.append(ext[0])
    sources.sort()
    return sources

# Get the list of all tools and plugins.
tools = get_cpp(src_dir + '/tstools')
plugins = get_cpp(src_dir + '/tsplugins')

# Projects with manual setup, do not overwrite project files.
manual_setup = ['tsdektec', 'tsplugin_dektec']

# "Other" MSBuild projects (ie. not tools, not plugins).
others = ['config', 'utests-tsduckdll', 'utests-tsducklib',
          'tscoredll', 'tscorelib', 'tsduckdll', 'tsducklib', 'tsdektecdll', 'tsdekteclib',
          'tsp_static', 'tsprofiling', 'tsmux', 'setpath']

# MSBuild / Visual Studio solution description.
cxx_project_guid = '8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942'
tsduck_sln_guid = '55E5A8EA-215E-45C2-9471-AD5CC5A925A0'

# A list of MSBuild project dependencies and property files.
ms_deps = {
    'tsp': {'deps': list(plugins)},
    'tsswitch': {'deps': ['tsplugin_dvb', 'tsplugin_hides']},
    # all "others" must be listed below, at least for tsduckdll vs. tsducklib.
    'tscoredll': {'deps': ['config']},
    'tscorelib': {'deps': ['config']},
    'tsduckdll': {'deps': ['tscoredll', 'config']},
    'tsducklib': {'deps': ['tscorelib', 'config']},
    'tsdektecdll': {'deps': ['tsduckdll', 'config']},
    'tsdekteclib': {'deps': ['tsducklib', 'config']},
    'utests-tsduckdll': {'deps': ['tsduckdll', 'tsplugin_merge']},
    'utests-tsducklib': {'deps': ['tsducklib']},
    'tsdektec': {'deps': ['tsdektecdll']},
    'tsplugin_dektec': {'deps': ['tsdektecdll']},
    'tsp_static': {'deps': ['tsdekteclib', 'tsducklib', 'tscorelib']},
    'tsprofiling': {'deps': ['tsduckdll']},
    'tsmux': {'deps': ['tsduckdll'] + plugins},
    'setpath': {'deps': ['tscorelib']}
}

# Build other MSBuild projects descriptions with empty fields.
for name in tools + plugins + others:
    if name not in ms_deps.keys():
        ms_deps[name] = {}
    if 'deps' not in ms_deps[name].keys():
        ms_deps[name]['deps'] = []
    if 'props' not in ms_deps[name].keys():
        ms_deps[name]['props'] = []

# Dependency to 'tsduckdll' is implicit for all tools and plugins.
for name in tools + plugins:
    if 'tsduckdll' not in ms_deps[name]['deps']:
        ms_deps[name]['deps'].append('tsduckdll')

# Build a set of Qt Creator project files.
def build_qt_files(names, config):
    for name in names:
        if name not in manual_setup:
            os.makedirs(qt_dir + '/' + name, 0o755, True)
            with open(qt_dir + '/' + name + '/' + name + '.pro', 'w') as f:
                f.write('# Automatically generated file, see %s\n' % script_name)
                f.write('CONFIG += %s\n' % config)
                f.write('TARGET = %s\n' % name)
                f.write('include(../tsduck.pri)\n')

# Build QT Creator project files.
build_qt_files(tools, 'tstool')
build_qt_files(plugins, 'tsplugin')

# Get a random hexa string.
def random_hex(byte_count):
    return binascii.hexlify(os.urandom(byte_count)).decode('utf-8').upper()

# Generate a random GUID.
def random_guid():
    return '%s-%s-%s-%s-%s' % (random_hex(4), random_hex(2), random_hex(2), random_hex(2), random_hex(6))

# Build the name of an MSBuild project file.
def ms_file(name):
    return ms_dir + '/' + name + '.vcxproj'

# Get the GUID of an MSBuild project.
def get_guid(name):
    if (os.path.exists(ms_file(name))):
        return xmlet.parse(ms_file(name)).find('.//{*}ProjectGuid').text.strip('{}')
    else:
        return None

# Build a set of MSBuild project files.
def build_ms_files(names, srcdir, props):
    for name in names:
        guid = get_guid(name)
        if guid is None:
            guid = random_guid()
        all_props = [props]
        if 'tsdektecdll' in ms_deps[name]['deps']:
            all_props.append('msvc-use-tsdektecdll')
        elif 'tsduckdll' in ms_deps[name]['deps']:
            all_props.append('msvc-use-tsduckdll')
        elif 'tsducklib' in ms_deps[name]['deps']:
            all_props.append('msvc-use-tsducklib')
        all_props.extend(ms_deps[name]['props'])
        if name not in manual_setup:
            with open(ms_file(name), 'w', newline = '\r\n') as f:
                f.write('<?xml version="1.0" encoding="utf-8"?>\n')
                f.write('<Project DefaultTargets="Build" ToolsVersion="15.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">\n')
                f.write('  <!-- Automatically generated file, see %s -->\n' % script_name)
                f.write('  <ImportGroup Label="PropertySheets">\n')
                f.write('    <Import Project="msvc-common-begin.props"/>\n')
                f.write('  </ImportGroup>\n')
                f.write('  <ItemGroup>\n')
                f.write('    <ClCompile Include="..\\..\\src\\%s\\%s.cpp"/>\n' % (srcdir, name))
                f.write('  </ItemGroup>\n')
                f.write('  <PropertyGroup Label="Globals">\n')
                f.write('    <ProjectGuid>{%s}</ProjectGuid>\n' % (guid))
                f.write('    <Keyword>Win32Proj</Keyword>\n')
                f.write('    <RootNamespace>%s</RootNamespace>\n' % (name))
                f.write('  </PropertyGroup>\n')
                f.write('  <ImportGroup Label="PropertySheets">\n')
                for prop in all_props:
                    f.write('    <Import Project="%s.props"/>\n' % (prop))
                f.write('    <Import Project="msvc-common-end.props"/>\n')
                f.write('  </ImportGroup>\n')
                f.write('</Project>\n')

# Build MSBuild project files.
build_ms_files(tools, 'tstools', 'msvc-target-exe')
build_ms_files(plugins, 'tsplugins', 'msvc-target-dll')

# Build MSBuild solution file.
with open(ms_dir + '/tsduck.sln', 'w', encoding = 'utf-8-sig', newline = '\r\n') as f:
    f.write('\n')
    f.write('Microsoft Visual Studio Solution File, Format Version 12.00\n')
    f.write('# Automatically generated file, see %s\n' % script_name)
    f.write('# Visual Studio Version 16\n')
    f.write('VisualStudioVersion = 16.0.29020.237\n')
    f.write('MinimumVisualStudioVersion = 10.0.40219.1\n')

    for name in others + plugins + tools:
        guid = get_guid(name)
        f.write('Project("{%s}") = "%s", "%s.vcxproj", "{%s}"\n' %(cxx_project_guid, name, name, guid))
        deps = ms_deps[name]['deps']
        deps.sort()
        if len(deps) > 0:
            f.write('\tProjectSection(ProjectDependencies) = postProject\n')
            for dep in deps:
                dep_guid = get_guid(dep)
                f.write('\t\t{%s} = {%s}\n' % (dep_guid, dep_guid))
            f.write('\tEndProjectSection\n')
        f.write('EndProject\n')

    f.write('Global\n')
    f.write('\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n')
    f.write('\t\tDebug|Win32 = Debug|Win32\n')
    f.write('\t\tDebug|x64 = Debug|x64\n')
    f.write('\t\tDebug|ARM64 = Debug|ARM64\n')
    f.write('\t\tRelease|Win32 = Release|Win32\n')
    f.write('\t\tRelease|x64 = Release|x64\n')
    f.write('\t\tRelease|ARM64 = Release|ARM64\n')
    f.write('\tEndGlobalSection\n')
    f.write('\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n')

    for name in others + plugins + tools:
        guid = get_guid(name)
        f.write('\t\t{%s}.Debug|Win32.ActiveCfg = Debug|Win32\n' % (guid))
        f.write('\t\t{%s}.Debug|Win32.Build.0 = Debug|Win32\n' % (guid))
        f.write('\t\t{%s}.Debug|x64.ActiveCfg = Debug|x64\n' % (guid))
        f.write('\t\t{%s}.Debug|x64.Build.0 = Debug|x64\n' % (guid))
        f.write('\t\t{%s}.Debug|ARM64.ActiveCfg = Debug|ARM64\n' % (guid))
        f.write('\t\t{%s}.Debug|ARM64.Build.0 = Debug|ARM64\n' % (guid))
        f.write('\t\t{%s}.Release|Win32.ActiveCfg = Release|Win32\n' % (guid))
        f.write('\t\t{%s}.Release|Win32.Build.0 = Release|Win32\n' % (guid))
        f.write('\t\t{%s}.Release|x64.ActiveCfg = Release|x64\n' % (guid))
        f.write('\t\t{%s}.Release|x64.Build.0 = Release|x64\n' % (guid))
        f.write('\t\t{%s}.Release|ARM64.ActiveCfg = Release|ARM64\n' % (guid))
        f.write('\t\t{%s}.Release|ARM64.Build.0 = Release|ARM64\n' % (guid))

    f.write('\tEndGlobalSection\n')
    f.write('\tGlobalSection(SolutionProperties) = preSolution\n')
    f.write('\t\tHideSolutionNode = FALSE\n')
    f.write('\tEndGlobalSection\n')
    f.write('\tGlobalSection(ExtensibilityGlobals) = postSolution\n')
    f.write('\t\tSolutionGuid = {%s}\n' % (tsduck_sln_guid))
    f.write('\tEndGlobalSection\n')
    f.write('EndGlobal\n')
