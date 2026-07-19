#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2026, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Python script to build the class diagram of a given class.
#
#  The input is the doxygen-generated tag file. The output are a Graphviz
#  "dot" file and the corresponding PNG file in bin/diagrams.
#
#-----------------------------------------------------------------------------

import os, sys, shutil, argparse, subprocess
import xml.etree.ElementTree as etree

# Calling script name, project root.
SCRIPT     = os.path.basename(sys.argv[0])
SCRIPTDIR  = os.path.dirname(os.path.abspath(sys.argv[0]))
ROOTDIR    = os.path.dirname(os.path.dirname(SCRIPTDIR))
DOXYDIR    = os.sep.join([ROOTDIR, 'bin', 'doxy'])
OUTDIR     = os.sep.join([ROOTDIR, 'bin', 'diagrams'])
TAGFILE    = os.sep.join([DOXYDIR, 'tags.xml'])

# Values of attribute "kind" in structure "compound" which are considered as classes.
CLASS_KINDS = {'class', 'struct', 'interface'}

# Prefixes of classes to ignore (Java and Python classes in doxygen styles).
IGNORE_PREFIXES = {'io::tsduck::', 'io.tsduck.', 'tsduck::', 'tsduck.'}

# Remove that prefix from all class names.
REMOVE_PREFIX = 'ts::'

# Search an executable in PATH. On Windows, also search a list of additional glob pattern.
def which(cmd, winglob=None):
    path = shutil.which(cmd)
    if path is None and os.name == 'nt' and winglob is not None:
        if not isinstance(winglob, list):
            winglob = [winglob]
        for dir in winglob:
            files = glob.glob(os.path.join(dir, cmd + '.exe'))
            if files is not None:
                path = files[0]
                break
    return path

# Build a dot identifier from a class name.
def node_id(name):
    return name if name.isidentifier() else '"' + name.replace('"', r'\"').replace(' ', '') + '"'

# Parse input XML file.
# Return (classes, edges).
# - classes: list of class names.
# - edges: list of tuples (derived, base, protection, virtualness).
def parse_tagfile(path):
    root = etree.parse(path).getroot()
    classes = []
    edges = []
    # Loop on all <compound> structures.
    for compound in root.findall('compound'):
        # Ignore <compound> which are not classes.
        if compound.get('kind') not in CLASS_KINDS:
            continue
        # Get the full class name.
        name_elem = compound.find('name')
        if name_elem is None or not name_elem.text:
            continue
        name = name_elem.text.strip()
        # Ignore classes without expected prefixes.
        if name.startswith(tuple(IGNORE_PREFIXES)):
            continue
        # Finally keep that class name.
        name = name.removeprefix(REMOVE_PREFIX)
        classes.append(name)
        # Add all derivations.
        for base in compound.findall('base'):
            if not base.text:
                continue
            basename = base.text.strip().removeprefix(REMOVE_PREFIX)
            edges.append((name, basename, base.get('protection', 'public'), base.get('virtualness', 'non-virtual')))
    return classes, edges

# Generate the output dot file.
def generate_dot(title, classes, edges, output):
    # External classes, not in the list of classes.
    known = set(classes)
    external = sorted({base for _, base, _, _ in edges if base not in known})
    # Generate output text: header.
    print('digraph %s {' % title, file=output)
    print('', file=output)
    print('    rankdir = BT;', file=output)
    print('    graph [fontname = "Helvetica"];', file=output)
    print('    node  [fontname = "Helvetica", shape = box, style = filled, fillcolor = "#eef3fa"];', file=output)
    print('    edge  [dir = forward, arrowhead = empty, fontname = "Helvetica", fontsize = 10];', file=output)
    print('', file=output)
    # All classes which are defined in the project.
    for name in sorted(classes):
        print('    %s;' % node_id(name), file=output)
    print('', file=output)
    # External classes.
    if external:
        for name in external:
            print('    %s [style = "filled,dashed", fillcolor = "#f2f2f2"];' % node_id(name), file=output)
        print('', file=output)
    # Derivations.
    for derived, base, protection, virtualness in sorted(edges):
        labels = []
        if protection in ('protected', 'private'):
            labels.append(protection)
        if virtualness == 'virtual':
            labels.append('virtual')
        attrs = ' [label = "%s"]' % ' '.join(labels) if labels else ''
        print('    %s -> %s%s;' % (node_id(derived), node_id(base), attrs), file=output)
    # Final text.
    print('}', file=output)

# Main function.
def main():
    # Decode command line.
    parser = argparse.ArgumentParser(description='Generate a dot class diagram from a doxygen tagfile.')
    parser.add_argument('classname', help='class name (without "ts::" prefix)')
    args = parser.parse_args()

    # Check if the doxygen-generated tag file is present. If not, run doxygen.
    if not os.path.exists(TAGFILE):
        print('Tag file %s not found, running doxygen' % (TAGFILE), file=sys.stderr)
        subprocess.run([os.sep.join([SCRIPTDIR, 'build-doxygen.py'])])
        if not os.path.exists(TAGFILE):
            sys.exit('Tag file %s still not found after running doxygen, aborting' % TAGFILE)

    # Parse input XML file.
    try:
        classes, edges = parse_tagfile(TAGFILE)
    except etree.ParseError as e:
        sys.exit('XML error in %s: %s' % (args.tagfile, e))
    except OSError as e:
        sys.exit('Read error: %s' % e)

    # Create output directory.
    os.makedirs(OUTDIR, exist_ok=True)

    # Build output file names.
    title = args.classname.translate(str.maketrans(':<> ', '____'))
    outfile_prefix = os.sep.join([OUTDIR, title])
    outfile_dot = outfile_prefix + '.dot'
    outfile_png = outfile_prefix + '.png'

    # Generate output dot file.
    with open(outfile_dot, 'w', encoding='utf-8') as output:
        print('Generating %s' % outfile_dot, file=sys.stderr)
        generate_dot(title, classes, edges, output)

    # Search dot executable.
    dot = which('dot', r'C:\Program Files*\Graphviz*\bin')
    if dot is None:
        sys.exit("Graphviz not installed, dot not found")

    # Generate a PNG file.
    print('Generating %s' % outfile_png, file=sys.stderr)
    subprocess.run([dot, outfile_dot, '-Tpng', '-o', outfile_png])
    if not os.path.exists(outfile_png):
        sys.exit('PNG file %s not found, dit failed' % outfile_png)

# Main entry point.
if __name__ == '__main__':
    main()
