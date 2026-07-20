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
SCRIPT    = os.path.basename(sys.argv[0])
SCRIPTDIR = os.path.dirname(os.path.abspath(sys.argv[0]))
ROOTDIR   = os.path.dirname(os.path.dirname(SCRIPTDIR))
DOXYDIR   = os.sep.join([ROOTDIR, 'bin', 'doxy'])
OUTDIR    = os.sep.join([ROOTDIR, 'bin', 'diagrams'])
TAGFILE   = os.sep.join([DOXYDIR, 'tags.xml'])

# Set of values of attribute "kind" in structure "compound" which are considered as classes.
CLASS_KINDS = {'class', 'struct', 'interface'}

# Set of prefixes of classes to ignore (Java and Python classes in doxygen styles).
IGNORE_PREFIXES = {'io::tsduck::', 'io.tsduck.', 'tsduck::', 'tsduck.'}

# Remove that prefix from all class names.
REMOVE_PREFIX = 'ts::'

# Set of "final" classes, which should not pull other, possibly unrelated, classes.
FINAL_CLASSES = {'Object', 'OwnedObject', 'Report', 'ReporterBase', 'SubscriptionBase'}

# Background color of class boxes.
BOX_COLOR = '#eef3fa'

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

# Cleanup a class name.
def clean_name(name):
    return name.strip().replace(' ', '').removeprefix(REMOVE_PREFIX)

# Build a dot identifier from a class name.
def node_id(name):
    return name if name.isidentifier() else '"' + name.replace('"', r'\"') + '"'

# Check if a class is final and should not be explored further.
def is_final(name):
    return name in FINAL_CLASSES or name.endswith('Interface')

# Class containing the description of classes and edges.
class Graph:

    # Constructor. Parse input XML file.
    def __init__(self, filename):
        self.classes = [] # list of class names
        self.edges = []   # list of tuples (derived, base, protection, virtualness)
        # Load and parse the XML file.
        root = etree.parse(filename).getroot()
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
            name = clean_name(name)
            self.classes.append(name)
            # Add all derivations.
            for base in compound.findall('base'):
                if not base.text:
                    continue
                self.edges.append((name, clean_name(base.text), base.get('protection', 'public'), base.get('virtualness', 'non-virtual')))

    # Display a list of classes.
    def list_classes(self, output):
        for n in sorted(self.classes):
            print(n, file=output)

    # Private method to explore a class in superclass and subclass directions.
    def _explore_class(self, classname, up_only=False):
        if classname not in self._explored:
            self._explored.add(classname)
            up_only = up_only or is_final(classname)
            for derived, base, protection, virtualness in self.edges:
                if derived == classname:
                    self._explore_class(base, up_only)
                    self._new_edges.add((derived, base, protection, virtualness))
                if not up_only and base == classname:
                    self._explore_class(derived)
                    self._new_edges.add((derived, base, protection, virtualness))

    # Reduce the graph to all classes which are related to a given class.
    # Return False on error.
    def reduce(self, classname):
        # Check that the class exists in the graph.
        if classname not in self.classes:
            print('Class %s is unknown, try %s -l' % (classname, SCRIPT), file=sys.stderr)
            return False
        # Explore all classes which are related to classname.
        self._explored = set()
        self._new_edges = set()
        self._explore_class(classname)
        self.classes = sorted(self._explored)
        self.edges = sorted(self._new_edges)
        del self._explored
        del self._new_edges
        return True

    # Generate the output dot file.
    def generate_dot(self, title, output):
        # External classes, not in the list of classes.
        known = set(self.classes)
        external = sorted({base for _, base, _, _ in self.edges if base not in known})
        # Generate output text: header.
        print('digraph %s {' % title, file=output)
        print('', file=output)
        print('    rankdir = BT;', file=output)
        print('    graph [fontname = "Helvetica"];', file=output)
        print('    node  [fontname = "Helvetica", shape = box, style = filled, fillcolor = "%s"];' % BOX_COLOR, file=output)
        print('    edge  [dir = forward, arrowhead = empty, fontname = "Helvetica", fontsize = 10];', file=output)
        print('', file=output)
        # All classes which are defined in the project.
        for name in sorted(self.classes):
            print('    %s;' % node_id(name), file=output)
        print('', file=output)
        # External classes.
        if external:
            for name in external:
                print('    %s [style = "filled,dashed", fillcolor = "#f2f2f2"];' % node_id(name), file=output)
            print('', file=output)
        # Derivations.
        for derived, base, protection, virtualness in sorted(self.edges):
            labels = []
            if protection in ('protected', 'private'):
                labels.append(protection)
            if virtualness == 'virtual':
                labels.append('virtual')
            attrs = ' [label = "%s"]' % ' '.join(labels) if labels else ''
            print('    %s -> %s%s;' % (node_id(derived), node_id(base), attrs), file=output)
        # Final text.
        print('}', file=output)

# Main entry point.
if __name__ == '__main__':
    # Decode command line.
    parser = argparse.ArgumentParser(description='Generate a dot class diagram from a doxygen tagfile.')
    parser.add_argument('classname', nargs='?', help='class name (without "ts::" prefix)')
    parser.add_argument('-a', '--all-classes', action='store_true', help='Build the graph of all classes.')
    parser.add_argument('-l', '--list-classes', action='store_true', help='List known classes in the project.')
    args = parser.parse_args()
    if args.classname is not None:
        args.classname = clean_name(args.classname)
    elif not args.list_classes and not args.all_classes:
        parser.error('classname is required (unless -a or -l is specified)')

    # Check if the doxygen-generated tag file is present. If not, run doxygen.
    if not os.path.exists(TAGFILE):
        print('Tag file %s not found, running doxygen' % (TAGFILE), file=sys.stderr)
        subprocess.run([os.sep.join([SCRIPTDIR, 'build-doxygen.py'])])
        if not os.path.exists(TAGFILE):
            sys.exit('Tag file %s still not found after running doxygen, aborting' % TAGFILE)

    # Parse input XML file.
    try:
        graph = Graph(TAGFILE)
    except etree.ParseError as e:
        sys.exit('XML error in %s: %s' % (TAGFILE, e))
    except OSError as e:
        sys.exit('Read error: %s' % e)

    # Display class list if required.
    if args.list_classes:
        graph.list_classes(sys.stdout)
        exit(0)

    # Reduce the graph around the specified class.
    if not args.all_classes and not graph.reduce(args.classname):
        exit(1)

    # Create output directory.
    os.makedirs(OUTDIR, exist_ok=True)

    # Build output file names.
    title = 'Classes' if args.classname is None else args.classname.translate(str.maketrans(':<>', '___'))
    outfile_prefix = os.sep.join([OUTDIR, title])
    outfile_dot = outfile_prefix + '.dot'
    outfile_png = outfile_prefix + '.png'

    # Generate output dot file.
    with open(outfile_dot, 'w', encoding='utf-8') as output:
        print('Generating %s' % outfile_dot, file=sys.stderr)
        graph.generate_dot(title, output)

    # Search dot executable.
    dot = which('dot', r'C:\Program Files*\Graphviz*\bin')
    if dot is None:
        sys.exit("Graphviz not installed, dot not found")

    # Generate a PNG file.
    print('Generating %s' % outfile_png, file=sys.stderr)
    subprocess.run([dot, outfile_dot, '-Tpng', '-o', outfile_png])
    if not os.path.exists(outfile_png):
        sys.exit('PNG file %s not found, dot failed' % outfile_png)
